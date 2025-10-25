#pragma once
#include "Entity.h"
#include "ComponentArray.h"
#include <unordered_map>
#include <typeindex>
#include <memory>
#include "Core/GUID.h"
#include "Core/EventSystem.h"
#include <future>
#include <functional>
#include "Core/ThreadPool.h"
#include <vector>
#include <algorithm>

namespace Mupfel {

	template<typename... Components> class View;

	class CollisionSystem;
	class Application;

	class ComponentAddedEvent : public Event<ComponentAddedEvent> {
	public:
		ComponentAddedEvent() = default;
		ComponentAddedEvent(Entity in_e, size_t in_comp_id) : e(in_e), comp_id(in_comp_id) {};
		virtual ~ComponentAddedEvent() = default;


		static constexpr uint64_t GetGUIDStatic() {
			return Hash::Compute("ComponentAddedEvent");
		}

	public:
		Entity e;
		size_t comp_id;
	};

	class ComponentRemovedEvent : public Event<ComponentRemovedEvent> {
	public:
		ComponentRemovedEvent() = default;
		ComponentRemovedEvent(Entity in_e, size_t in_comp_id) : e(in_e), comp_id(in_comp_id) {};
		virtual ~ComponentRemovedEvent() = default;


		static constexpr uint64_t GetGUIDStatic() {
			return Hash::Compute("ComponentRemovedEvent");
		}

	public:
		Entity e;
		size_t comp_id;
	};

	class Registry
	{
		template<typename... Components> friend class View;
		friend class CollisionSystem;
	public:
		using SafeComponentArrayPtr = std::unique_ptr<IComponentArray>;
	public:
		Registry(EventSystem& in_evt_sys) : evt_system(in_evt_sys) {}
		Entity CreateEntity();

		void DestroyEntity(Entity e);

		uint32_t GetCurrentEntities() const;

		Entity::Signature GetSignature(uint32_t index) const;

		template<typename... Components>
		View<Components...> view() {
			return View<Components...>(*this);
		}

		template <typename... Components, typename F>
		void ParallelForEach(F&& function, std::vector<Entity>& changed_entities);

		template<typename T, typename... Args>
		T& AddComponent(Entity e, Args&&... args);

		template<typename T>
		T& AddComponent(Entity e, const T& component);

		template<typename T>
		void RemoveComponent(Entity e);

		template<typename T>
		ComponentArray<T>& GetComponentArray();

	private:
		EventSystem& evt_system;
		EntityManager entity_manager;
		std::vector<Entity::Signature> signatures;
		std::unordered_map<std::type_index, SafeComponentArrayPtr> component_map;
	};


	template<typename ...Components, typename F>
	inline void Registry::ParallelForEach(F&& function, std::vector<Entity>& changed_entities)
	{
		auto view = this->view<Components...>();

		auto& pool = Application::GetCurrentThreadPool();
		const uint32_t num_threads = pool.GetThreadCount();

		using BaseComponent = std::tuple_element_t<0, std::tuple<Components...>>;
		auto& array = GetComponentArray<BaseComponent>();
		const auto& dense = array.dense;
		const uint32_t total = dense.size();

		auto arrays = std::make_tuple(&GetComponentArray<Components>()...);

		if (total == 0)
		{
			return;
		}

		const uint32_t chunk = (total + num_threads - 1) / num_threads;
		std::vector<std::future<std::vector<Entity>>> jobs;
		jobs.reserve(num_threads);


		const auto required = CompUtil::ComponentSignature<Components...>();

		for (uint32_t t = 0; t < num_threads; t++)
		{
			const uint32_t begin = t * chunk;
			const uint32_t end = std::min(begin + chunk, total);

			if (begin >= end)
			{
				continue;
			}
				
			jobs.push_back(pool.Enqueue([this, begin, end, function, &dense, required, &arrays]() mutable -> std::vector<Entity> {

				std::vector<Entity> local_entities;
				local_entities.reserve(64);

				for (size_t i = begin; i < end; ++i)
				{
					Entity e{ dense[i] };
					const auto& sig = GetSignature(e.Index());

					/* check if the entity has all the needed components */
					if ((sig & required) != required)
						continue;

					/* Call given function on the entity */
					//bool entity_changed = function(e, GetComponent<Components>(e)...);

					std::apply([&](auto*... arr) {
						bool entity_changed = function(e, arr->Get(e)...);
						if (entity_changed) local_entities.push_back(e);
						}, arrays);

				}

				return local_entities;
			}));

		}

		/* Wait for all jobs to finish and collect the entities */
		for (auto& job : jobs)
		{
			auto local = job.get();
			changed_entities.insert(changed_entities.end(),
				std::make_move_iterator(local.begin()),
				std::make_move_iterator(local.end()));
		}
	}

	template<typename T, typename ...Args>
	inline T& Registry::AddComponent(Entity e, Args && ...args)
	{
		ComponentArray<T>& storage = GetComponentArray<T>();
		storage.Insert(e, T(std::forward<Args>(args)...));

		/* Update the Entity Signature */
		size_t id = CompUtil::GetComponentTypeID<T>();
		signatures[e.Index()].set(id);

		/* Send a ComponentAdded Event */
		evt_system.AddImmediateEvent<ComponentAddedEvent>({ e, id });

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

		/* Send a ComponentAdded Event */
		evt_system.AddImmediateEvent<ComponentAddedEvent>({ e, id });

		return storage.Get(e);
	}

	template<typename T>
	inline void Registry::RemoveComponent(Entity e)
	{
		uint32_t id = CompUtil::GetComponentTypeID<T>();
		/* Send a ComponentRemoved Event */
		evt_system.AddImmediateEvent<ComponentRemovedEvent>({ e, id });

		ComponentArray<T>& storage = GetComponentArray<T>();
		storage.Remove(e);

		/* Update the Entity Signature */
		signatures[e.Index()].reset(id);
		
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

