/**
 * \file   Entities.h
 * \brief  Entity and component access.
 *
 * Functions related to the Entity-Component-System of Mupfel.
 */
#pragma once
#include "Core/Application.h"
#include "ECS/Entity.h"
#include "ECS/Registry.h"
#include "ECS/View.h"
#include <concepts>
#include <cstdint>
#include <utility>

namespace Mupfel::Entities
{

/** Creates an entity in the active scene and fires an EntityCreatedEvent. */
[[nodiscard]] inline Entity Create() { return Application::GetCurrentRegistry().CreateEntity(); }

/** Destroys \a e and fires an EntityDestroyedEvent. */
inline void Destroy(Entity e) { Application::GetCurrentRegistry().DestroyEntity(e); }

/** The number of live entities in the registry, across all scenes. */
[[nodiscard]] inline uint32_t Count() { return Application::GetCurrentRegistry().GetCurrentEntities(); }

/**
 * Adds \a component to \a e, updates its signature, and fires a ComponentAddedEvent.
 *
 * \note If \a e already has a component of type T, the component is overwritten.
 */
template <typename T> inline void AddComponent(Entity e, T component)
{
	Application::GetCurrentRegistry().AddComponent(e, std::move(component));
}

/** Adds a \a component to \a e by constructing it from the given arguments. */
template <typename T, typename... Args> inline void EmplaceComponent(Entity e, Args&&... args)
{
	Application::GetCurrentRegistry().AddComponent(e, T(std::forward<Args>(args)...));
}

/** Removes the component of type T from \a e. */
template <typename T> inline void RemoveComponent(Entity e) { Application::GetCurrentRegistry().RemoveComponent<T>(e); }

/**
 * Retrieves the component of type T from entity \a e.
 *
 * \warning Calling this function on entities without a component of type T is undefined!
 * If you are unsure, check with Has(e)!
 */
template <typename T> [[nodiscard]] inline T& GetComponent(Entity e)
{
	return Application::GetCurrentRegistry().GetComponent<T>(e);
}

/**
 * Overwrite the component of type T of \a e.
 *
 * \warning Calling this function on entities without a component of type T is undefined!
 * If you are unsure, check with Has(e)!
 */
template <typename T> inline void SetComponent(Entity e, T component)
{
	Application::GetCurrentRegistry().SetComponent<T>(e, std::move(component));
}

/** Whether \a e currently has a component of type T. */
template <typename T> [[nodiscard]] inline bool HasComponent(Entity e)
{
	return Application::GetCurrentRegistry().HasComponent<T>(e);
}

/**
 * Iterate over all entities that have a specified set of components.
 *
 */
template <typename... Components> [[nodiscard]] inline View<Components...> Each()
{
	return Application::GetCurrentRegistry().view<Components...>();
}

/**
 * Runs function(Entity, Components&...) over every matching entity, split into chunks and
 * dispatched across the engine thread pool.
 *
 * \warning \a function runs on worker threads. It must not touch the registry structurally
 *          (no create/destroy/add/remove) and must synchronize any shared state it writes.
 */
template <typename... Components, typename F>
	requires std::invocable<F&, Entity, Components&...>
inline void ParallelEach(F&& function)
{
	Application::GetCurrentRegistry().ParallelForEach<Components...>(std::forward<F>(function));
}

} // namespace Mupfel::Entities
