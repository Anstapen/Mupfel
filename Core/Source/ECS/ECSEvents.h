#pragma once
#include "Core/Event.h"
#include "Entity.h"

/**
 * This header defines some events used by the Registry.
 */

namespace Mupfel
{
/** Fired when the Registry creates an entity. */
class EntityCreatedEvent : public Event
{
public:
	EntityCreatedEvent(Entity in_e) : e(in_e) {}

	/** The entity that was created. */
	Entity e;
};

/**
 * Fired via `EventSystem::AddImmediateEvent` from `Registry::DestroyEntity`, before its components
 * are removed, so listeners can still inspect them.
 */
class EntityDestroyedEvent : public Event
{
public:
	EntityDestroyedEvent(Entity in_e) : e(in_e) {}

	/** The entity being destroyed. */
	Entity e;
};

/**
 * Fired via `EventSystem::AddImmediateEvent` from `Registry::AddComponent`, after the component
 * is inserted and the entity's signature updated.
 */
class ComponentAddedEvent : public Event
{
public:
	ComponentAddedEvent(Entity in_e, Entity::Signature in_sig, size_t in_comp_id)
		: e(in_e), sig(in_sig), comp_id(in_comp_id)
	{
	}

	/** The entity the component was added to. */
	Entity e;
	/** The entity's signature after adding the component. */
	Entity::Signature sig;
	/** `ComponentIndex::Index` of the added component type. */
	size_t comp_id;
};

/**
 * Fired via `EventSystem::AddImmediateEvent` from `Registry::RemoveComponent`, before the
 * component is actually removed, so listeners can still inspect it.
 */
class ComponentRemovedEvent : public Event
{
public:
	ComponentRemovedEvent(Entity in_e, Entity::Signature in_sig, size_t in_comp_id)
		: e(in_e), sig(in_sig), comp_id(in_comp_id)
	{
	}
	/** The entity the component is being removed from. */
	Entity e;
	/** The entity's signature before the component is removed. */
	Entity::Signature sig;
	/** `ComponentIndex::Index` of the component type being removed. */
	size_t comp_id;
};
} // namespace Mupfel
