#pragma once
#include "Entity.h"
#include "ComponentArray.h"
#include <unordered_map>
#include <typeindex>
#include <memory>
#include "Core/GUID.h"

namespace Mupfel {

	template<typename... Components> class View;

	class Registry
	{
		template<typename... Components> friend class View;
	public:
		using SafeComponentArrayPtr = std::unique_ptr<IComponentArray>;
	public:
		Entity CreateEntity();

		void DestroyEntity(Entity e);

		uint32_t GetCurrentEntities() const;

		size_t GetFreeSIZE() const { return entity_manager.GetFreeListSize(); }

		Entity::Signature GetSignature(uint32_t index) const;

		template<typename... Components>
		View<Components...> view() {
			return View<Components...>(*this);
		}

		template<typename T, typename... Args>
		T& AddComponent(Entity e, Args&&... args);

		template<typename T>
		T& AddComponent(Entity e, const T& component);

		template<typename T>
		T& GetComponent(Entity e);

		template<typename T>
		void RemoveComponent(Entity e);

		template<typename T>
		bool HasComponent(Entity e);

	private:
		//template<typename... Components> friend class View;
		template<typename T>
		ComponentArray<T>& GetComponentArray();

	private:
		EntityManager entity_manager;
		std::vector<Entity::Signature> signatures;
		std::unordered_map<std::type_index, SafeComponentArrayPtr> component_map;
	};


	template<typename T, typename ...Args>
	inline T& Registry::AddComponent(Entity e, Args && ...args)
	{
		ComponentArray<T>& storage = GetComponentArray<T>();
		storage.Insert(e, T(std::forward<Args>(args)...));

		/* Update the Entity Signature */
		signatures[e.Index()].set(CompUtil::GetComponentTypeID<T>());

		return storage.Get(e);
	}

	template<typename T>
	inline T& Registry::AddComponent(Entity e, const T& component)
	{
		ComponentArray<T>& storage = GetComponentArray<T>();
		storage.Insert(e, component);

		/* Update the Entity Signature */
		uint32_t id = CompUtil::GetComponentTypeID<T>();
		signatures[e.Index()].set(id);

		return storage.Get(e);
	}

	template<typename T>
	inline T& Registry::GetComponent(Entity e)
	{
		ComponentArray<T>& storage = GetComponentArray<T>();
		return storage.Get(e);
	}

	template<typename T>
	inline void Registry::RemoveComponent(Entity e)
	{
		ComponentArray<T>& storage = GetComponentArray<T>();
		storage.Remove(e);

		/* Update the Entity Signature */
		signatures[e.Index()].reset(CompUtil::GetComponentTypeID<T>());
	}

	template<typename T>
	inline bool Registry::HasComponent(Entity e)
	{
		/*If there is no component array for the given component type, the entity does not have it. */
		auto it = component_map.find(typeid(T));

		if (it == component_map.end())
		{
			return false;
		}

		/* look into the component array */
		return it->second->Has(e);
	}

	template<typename T>
	inline ComponentArray<T>& Registry::GetComponentArray()
	{
		/* We index into the map using the type id of the given type */
		auto index = std::type_index(typeid(T));
		auto it = component_map.find(index);

		if (it == component_map.end())
		{
			/* The component map does not exist yet, create it */
			SafeComponentArrayPtr new_array = std::make_unique<ComponentArray<T>>();
			component_map[index] = std::move(new_array);
			return *static_cast<ComponentArray<T>*>(component_map[index].get());
		}

		return *static_cast<ComponentArray<T>*>(it->second.get());
	}

}

