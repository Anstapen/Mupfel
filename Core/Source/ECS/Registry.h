#pragma once
#include "Core/EventSystem.h"
#include "Entity.h"
#include <algorithm>
#include <cassert>
#include <functional>
#include <future>
#include <memory>
#include <typeindex>
#include <vector>
#include <concepts>

#include "ComponentArray.h"
#include "Core/Scene.h"
#include "Core/ThreadPool.h"
#include "ECSEvents.h"
#include "ECS/Components/ComponentIndex.h"

namespace Mupfel
{

template <typename FirstComponent, typename... Components> class View;
class CollisionSystem;
class Application;
class MovementSystem;
class RayCastSystem;

/**
 * A Registry holds a complete ECS context. It provides methods to create / destroy
 * entities, add / remove components and means to iterate over entities based on a set
 * of components.
 */
class Registry
{
	template <typename FirstComponent, typename... Components> friend class View;
	friend class CollisionSystem;
	friend class MovementSystem;
	friend class Renderer;
	friend class RayCastSystem;

public:
	/** Helper type for a unique ptr for component arrays. */
	using SafeComponentArrayPtr = std::unique_ptr<IComponentArray>;

public:
	/**
	 * The registry constructor needs an event system (to issue entity and component events) and
	 * a thread pool (for parallel searches).
	 */
	Registry(EventSystem& in_evt_sys, ThreadPool& in_pool) : evt_system(in_evt_sys), thread_pool(in_pool) {}

	/** Create an entity. Additonally, a "EntityCreatedEvent" event is fired to notify everyone. */
	Entity CreateEntity();

	/** Destroy an entity. Additonally, a "EntityDestroyedEvent" event is fired to notify everyone. */
	void DestroyEntity(Entity e);

	/** Return the number of current entities in the registry. */
	uint32_t GetCurrentEntities() const;

	/**
	 * @warning Asserts `index` refers to an entity that was created via `CreateEntity`.
	 * @return The signature (which components it has) of the entity at `index`.
	 */
	Entity::Signature GetSignature(Entity entity) const;

	/**
	 * @warning Asserts `index` refers to an entity that was created via `CreateEntity`.
	 * @return The scene mask (which scenes it belongs to) of the entity at `index`.
	 */
	Scene::SceneMask GetSceneMask(Entity entity) const;

	/**
	 * Sets the scene that `CreateEntity` assigns new entities to and that `view()`/`ParallelForEach`
	 * filter on. Pushed down by `Application` on every scene switch, so the ECS never has to ask the
	 * `Application` which scene is current.
	 */
	void SetActiveScene(SceneHandle scene);

	/** The scene `CreateEntity` currently assigns new entities to. */
	SceneHandle GetActiveScene() const;

	/** `GetActiveScene()` as a mask, for signature-style comparisons against `GetSceneMask`. */
	Scene::SceneMask GetActiveSceneMask() const;

	/** Returns a `View` iterating every entity that has all of `Components...`. */
	template <typename... Components> View<Components...> view() { return View<Components...>(*this); }

	/**
	 * Runs `function(Entity, Components&...)` over every entity with all of `Components...`, split
	 * into chunks and dispatched across the registry's thread pool.
	 */
	template <typename... Components, typename F>
		requires std::invocable<F&, Entity, Components&...>
	void ParallelForEach(F&& function);

	/** Constructs a `T` in place from `args` and adds it to `e` (see the `AddComponent(Entity, T)` overload). */
	template <typename T, typename... Args> void AddComponent(Entity e, Args&&... args);

	/**
	 * Adds `component` to `e`, updates `e`'s signature, and fires `ComponentAddedEvent`.
	 * @warning `e` must not already have a component of type `T`.
	 */
	template <typename T> void AddComponent(Entity e, T component);

	/** Fires `ComponentRemovedEvent`, removes `e`'s component of type `T`, and updates `e`'s signature. */
	template <typename T> void RemoveComponent(Entity e);

	/** @warning Undefined unless `HasComponent<T>(e)` is true. */
	template <typename T> T& GetComponent(Entity e);

	/** @warning Undefined unless `HasComponent<T>(e)` is true. */
	template <typename T> void SetComponent(Entity e, T comp);

	/** Whether `e` currently has a component of type `T`. */
	template <typename T> bool HasComponent(Entity e);

	/** The combined `Entity::Signature` bit for each of `Components...`, for signature comparisons. */
	template <typename... Components> static inline Entity::Signature ComponentSignature()
	{
		Entity::Signature sig;
		(sig.set(ComponentIndex::Index<Components>()), ...);
		return sig;
	}

	static inline std::bitset<Scene::MAX_SCENES> SceneMask(SceneHandle scene)
	{
		std::bitset<Scene::MAX_SCENES> sig;
		sig.set(scene);
		return sig;
	}

private:
	/** Returns (creating on first use) the `ComponentArray<T>` for component type `T`. */
	template <typename T> ComponentArray<T>& GetComponentArray();

	/** Grows `component_buffer` if needed so `T`'s slot (`ComponentIndex::Index<T>()`) is valid. */
	template <typename T> void resizeComponentBuffer();

private:
	/** Where entity/component lifecycle events are fired. */
	EventSystem& evt_system;
	/** Where `ParallelForEach` dispatches its chunks. */
	ThreadPool& thread_pool;
	/** Allocates/recycles entity indices. */
	EntityManager entity_manager;
	/** Per-entity-index signature, resized alongside entities. */
	std::vector<Entity::Signature> signatures;
	/** Per-entity-index scene membership, resized alongside entities. */
	std::vector<Scene::SceneMask> sceneMask;
	/** Per-component-type storage, indexed by `ComponentIndex`. */
	std::vector<SafeComponentArrayPtr> component_buffer;
	/** Scene new entities are created in and that views filter on; owned here, set by `Application`. */
	SceneHandle active_scene = 0;
};

template <typename... Components, typename F>
	requires std::invocable<F&, Entity, Components&...>
inline void Registry::ParallelForEach(F&& function)
{
	auto&		   pool = thread_pool;
	const uint32_t num_threads = std::max<uint32_t>(1u, static_cast<uint32_t>(pool.GetThreadCount()));

	using BaseComponent = std::tuple_element_t<0, std::tuple<Components...>>;
	auto&		   array = GetComponentArray<BaseComponent>();
	const auto&	   dense = array.dense;
	const uint32_t total = static_cast<uint32_t>(dense.size());

	auto arrays = std::make_tuple(&GetComponentArray<Components>()...);

	if (total == 0)
	{
		return;
	}

	const uint32_t								  chunk = (total + num_threads - 1) / num_threads;
	std::vector<std::future<void>> jobs;
	jobs.reserve(num_threads);

	const Entity::Signature required = Registry::ComponentSignature<Components...>();
	const Scene::SceneMask	required_scene = Registry::GetActiveSceneMask();

	for (uint32_t t = 0; t < num_threads; t++)
	{
		const uint32_t begin = t * chunk;
		const uint32_t end = std::min(begin + chunk, total);

		if (begin >= end)
		{
			continue;
		}

		jobs.push_back(pool.Enqueue(
			[this, begin, end, function, &dense, required, required_scene, arrays]() mutable -> void
			{

				for (size_t i = begin; i < end; ++i)
				{
					Entity		e{dense[i]};
					const auto& sig = GetSignature(e);
					const Scene::SceneMask scene_mask = GetSceneMask(e);

					/* check if the entity has all the needed components */
					if (((sig & required) != required) || ((scene_mask & required_scene) != required_scene))
						continue;

					/* Call given function on the entity */
					// bool entity_changed = function(e, GetComponent<Components>(e)...);

					std::apply(
						[&](auto*... arr)
						{
							function(e, arr->Get(e)...);
						},
						arrays);
				}
			}));
	}

	/* Wait for all jobs to finish and collect the entities */
	std::exception_ptr first;
	for (auto& job : jobs)
	{

		try
		{
			job.get();
		}
		catch (...)
		{
			/* Save the first exception that was thrown. */
			if (!first)
			{
				first = std::current_exception();
			}
		}

		/* If user code caused an exception, throw it now. */
		if (first)
		{
			std::rethrow_exception(first);
		}
	}
}

template <typename T, typename... Args> inline void Registry::AddComponent(Entity e, Args&&... args)
{
	AddComponent(e, T(std::forward<Args>(args)...));
}

template <typename T> inline void Registry::AddComponent(Entity e, T component)
{
	/* Do not add a component to a dead entity! */
	if (!entity_manager.IsAlive(e))
	{
		return;
	}

	ComponentArray<T>& storage = GetComponentArray<T>();
	storage.Insert(e, std::move(component));

	/* Update the Entity Signature */
	uint32_t id = static_cast<uint32_t>(ComponentIndex::Index<T>());
	signatures[e.Index()].set(id);

	/* Send a ComponentAdded Event */
	evt_system.AddImmediateEvent<ComponentAddedEvent>({e, signatures[e.Index()], id});
}

template <typename T> inline void Registry::RemoveComponent(Entity e)
{
	/* Check if the entity has the component. */
	if (!HasComponent<T>(e))
	{
		return;
	}

	ComponentArray<T>& storage = GetComponentArray<T>();
	size_t			   id = ComponentIndex::Index<T>();
	/* Send a ComponentRemoved Event */
	evt_system.AddImmediateEvent<ComponentRemovedEvent>({e, signatures[e.Index()], id});

	storage.Remove(e);

	/* Update the Entity Signature */
	signatures[e.Index()].reset(id);
}

template <typename T> inline T& Registry::GetComponent(Entity e) { return GetComponentArray<T>().Get(e); }

template <typename T> inline void Registry::SetComponent(Entity e, T comp) { GetComponentArray<T>().Set(e, comp); }

template <typename T> inline bool Registry::HasComponent(Entity e) { return GetComponentArray<T>().Has(e); }

template <typename T> inline ComponentArray<T>& Registry::GetComponentArray()
{
	size_t comp_index = ComponentIndex::Index<T>();

	resizeComponentBuffer<T>();

	/* Create a new Component Array for the given Type if there is none */
	if (!component_buffer[comp_index])
	{
		SafeComponentArrayPtr new_array = std::make_unique<ComponentArray<T>>(10);
		component_buffer[comp_index] = std::move(new_array);
	}

	return *static_cast<ComponentArray<T>*>(component_buffer[comp_index].get());
}

template <typename T> inline void Registry::resizeComponentBuffer()
{
	size_t comp_index = ComponentIndex::Index<T>();

	if (comp_index >= component_buffer.size())
	{
		component_buffer.resize(comp_index + 1);
	}
}

} // namespace Mupfel

#include "ECS/View.h"