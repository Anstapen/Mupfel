#pragma once
#include "Entity.h"
#include "ComponentArray.h"
#include <typeindex>
#include <memory>
#include "Core/EventSystem.h"
#include <future>
#include <functional>
#include "Core/ThreadPool.h"
#include <vector>
#include <algorithm>

#include "Archetype.h"

namespace Mupfel {

	template<typename... Components> class View;

	class CollisionSystem;
	class Application;
	class MovementSystem;

	class ComponentAddedEvent : public Event {
	public:
		ComponentAddedEvent() = default;
		ComponentAddedEvent(Entity in_e, size_t in_comp_id, Entity::Signature in_sig) : e(in_e), comp_id(in_comp_id), sig(in_sig) {};
		virtual ~ComponentAddedEvent() = default;

	public:
		Entity e;
		Entity::Signature sig;
		size_t comp_id;
	};

	class ComponentRemovedEvent : public Event {
	public:
		ComponentRemovedEvent() = default;
		ComponentRemovedEvent(Entity in_e, size_t in_comp_id, Entity::Signature in_sig) : e(in_e), comp_id(in_comp_id), sig(in_sig) {};
		virtual ~ComponentRemovedEvent() = default;

	public:
		Entity e;
		Entity::Signature sig;
		size_t comp_id;
	};

	class Registry
	{
		template<typename... Components> friend class View;
		friend class CollisionSystem;
		friend class MovementSystem;
		friend class Renderer;
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

		template<typename... Components>
		Archetype SetArchetype();

		template<typename T, typename... Args>
		void AddComponent(Entity e, Args&&... args);

		template<typename T>
		void AddComponent(Entity e, T component);

		template<typename T>
		void RemoveComponent(Entity e);

		template<typename T>
		T GetComponent(Entity e);

		template<typename T>
		void SetComponent(Entity e, T comp);

		template<typename T>
		bool HasComponent(Entity e);
		
	private:
		template<typename T>
		ComponentArray<T>& GetComponentArray();

		template<typename T>
		void CreateLinkedComponentArray(StorageType t);

		template<typename ...Components>
		static inline std::bitset<128> ComponentSignature()
		{
			std::bitset<128> sig;
			(sig.set(ComponentIndex<Components>()), ...);
			return sig;
		}

		template<typename T>
		static size_t ComponentIndex() noexcept {
			static const size_t id = comp_counter++;
			return id;
		}

		template<typename T>
		void resizeComponentBuffer();

		template<typename T>
		void AddArchetypeToComponent(Archetype::Signature sig);

		void UpdateEntityArchetypes(Entity e);

	private:
		EventSystem& evt_system;
		EntityManager entity_manager;
		static inline size_t comp_counter = 0;
		std::vector<Entity::Signature> signatures;
		Entity::Signature linked_components;
		std::vector<SafeComponentArrayPtr> component_buffer;

		/* Archetype related data structures */

		/* a vector of the currently registered archetypes */
		std::vector<Archetype> archetypes;

		/* a vector for each archetype that holds the entities that currently have this archetype */
		std::vector<std::vector<Entity>> archetypeToEntities;

		/* This vector holds the Archetype signature for each entity */
		std::vector<Archetype::Signature> entityToArchetype;

		std::vector<Archetype::Signature> componentToArchetype;
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


		const auto required = Registry::ComponentSignature<Components...>();

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

	template<typename ...Components>
	inline Archetype Registry::SetArchetype()
	{
		
		/* Create a new Archetype object */
		Entity::Signature sig = ComponentSignature<Components...>();
		Archetype arch{ sig, archetypes.size()};

		/* Add it to the archetype vector */
		archetypes.push_back(arch);

		/* Add a new entity list for the new archetype */
		archetypeToEntities.emplace_back();

		/* Add the archetype to the components */
		(..., (AddArchetypeToComponent<Components>(arch.arch_signature), 0));

		return arch;
	}

	template<typename T, typename ...Args>
	inline void Registry::AddComponent(Entity e, Args && ...args)
	{
		AddComponent(e, T(std::forward<Args>(args)...));
	}

	template<typename T>
	inline void Registry::AddComponent(Entity e, T component)
	{

		ComponentArray<T>& storage = GetComponentArray<T>();
		storage.Insert(e, component);

		/* Update the Entity Signature */
		uint32_t id = ComponentIndex<T>();
		signatures[e.Index()].set(id);

		/* Check Archetypes */
		UpdateEntityArchetypes(e);

		/* Send a ComponentAdded Event */
		evt_system.AddImmediateEvent<ComponentAddedEvent>({ e, id, signatures[e.Index()] });

	}

	template<typename T>
	inline void Registry::RemoveComponent(Entity e)
	{
		uint32_t id = ComponentIndex<T>();
		/* Send a ComponentRemoved Event */
		evt_system.AddImmediateEvent<ComponentRemovedEvent>({ e, id, signatures[e.Index()] });

		ComponentArray<T>& storage = GetComponentArray<T>();
		storage.Remove(e);

		/* Update the Entity Signature */
		signatures[e.Index()].reset(id);

		/* Check Archetypes */
		UpdateEntityArchetypes(e);
		
	}

	template<typename T>
	inline T Registry::GetComponent(Entity e)
	{
		return GetComponentArray<T>().Get(e);
	}

	template<typename T>
	inline void Registry::SetComponent(Entity e, T comp)
	{
		GetComponentArray<T>().Set(e, comp);
	}

	template<typename T>
	inline bool Registry::HasComponent(Entity e)
	{
		return GetComponentArray<T>().Has(e);
	}

	template<typename T>
	inline ComponentArray<T>& Registry::GetComponentArray()
	{
		size_t comp_index = ComponentIndex<T>();
		
		resizeComponentBuffer<T>();

		/* Create a new Component Array for the given Type if there is none */
		if (!component_buffer[comp_index])
		{
			SafeComponentArrayPtr new_array = std::make_unique<ComponentArray<T>>(StorageType::CPU);
			component_buffer[comp_index] = std::move(new_array);
		}

		return *static_cast<ComponentArray<T>*>(component_buffer[comp_index].get());
	}

	template<typename T>
	inline void Registry::CreateLinkedComponentArray(StorageType t)
	{
		size_t comp_index = ComponentIndex<T>();

		resizeComponentBuffer<T>();

		assert(!component_buffer[comp_index]);

		SafeComponentArrayPtr new_array = std::make_unique<ComponentArray<T>>(StorageType::GPU);
		component_buffer[comp_index] = std::move(new_array);

	}

	template<typename T>
	inline void Registry::resizeComponentBuffer()
	{
		size_t comp_index = ComponentIndex<T>();

		if (comp_index >= component_buffer.size())
		{
			component_buffer.resize(comp_index + 1);
		}

		if (comp_index >= componentToArchetype.size())
		{
			componentToArchetype.resize(comp_index + 1);
		}
	}

	template<typename T>
	inline void Registry::AddArchetypeToComponent(Archetype::Signature sig)
	{
		size_t comp_index = ComponentIndex<T>();
		
		/* resize the component vectors if they are not big enough */
		resizeComponentBuffer<T>();

		componentToArchetype[comp_index] |= sig;
	}

}

#include "ECS/View.h"