#pragma once
#include "Core/Event.h"
#include "Core/EventBuffer.h"
#include <bitset>
#include <cstdint>
#include <vector>
#include <cstddef>

namespace Mupfel
{

/* Forward declaration of classes that need friend acces to the entity. */
class EntityManager;
template <typename FirstComponent, typename... Components> class View;
class Registry;

/**
 * This class defines the entity. It is basically just a wrapper around uint32_t.
 * The properties of an entity are added through components.
 */
class Entity
{
	/* As the Entity Manager provides the factory method for the entity, it is a friend. */
	friend class EntityManager;

	/* The view constructs entities based on the underlying component buffers. */
	template <typename FirstComponent, typename... Components> friend class View;
	
	/* TODO: i do not want that! This is only because of the parallel view... */
	friend class Registry;

public:
	/**
	 * Due to the way how entity component checking is implemented (via a bitmask),
	 * there is a maximum number of component types that can be used.
	 */
	static constexpr size_t MAX_COMPONENTS_TYPES = 256;

	/** Each entity has a signature that shows which components the entity currently has. */
	using Signature = std::bitset<MAX_COMPONENTS_TYPES>;

public:
	/** The raw index this entity refers to. */
	constexpr uint32_t Index() const { return index; }

	/** Spaceship operator for defining ordering operations on entities. */
	friend constexpr auto operator<=>(const Entity&, const Entity&) = default;

private:
	/** Entity creation needs to be done by a factory. */
	constexpr Entity(uint32_t in_index) : index(in_index) {}

private:
	/** The index of the entity. */
	uint32_t index;
};

static_assert((sizeof(Entity) == 4), "Error: Size of Entity Class is not 4 bytes!");

/**
 * This class manages a pool of entities. It provides a factory function to create and
 * destroy entities and other convenience functions.
 */
class EntityManager
{
public:
	/**
	 * Create a new Entity Manager. It allocates memory for 100 entities on creation.
	 *
	 */
	EntityManager() : freeList(), current_entities(0), next_entity_index(1)
	{
		freeList.reserve(100);
		alive.reserve(100);
	}

	/** Returns a recycled index if one is free, otherwise allocates the next unused index. */
	Entity CreateEntity();

	/** Provides information on whether or not the entity is alive. */
	bool IsAlive(Entity e) const;

	/** Recycles `e`'s index for reuse by a future `CreateEntity` call. */
	void DestroyEntity(Entity e);

	/** Number of currently-live entities (created but not yet destroyed). */
	uint32_t GetCurrentEntities() const;

private:
	/** Number of currently-live entities. */
	uint32_t current_entities;
	/** Next never-before-used index to hand out. */
	uint32_t next_entity_index;
	/** Recycled indices from destroyed entities, reused first. */
	std::vector<uint32_t> freeList;
	/** A vector of alive entities. */
	std::vector<bool> alive;
};

} // namespace Mupfel