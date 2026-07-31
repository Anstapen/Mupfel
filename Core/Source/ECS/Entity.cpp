#include "Entity.h"
#include <iostream>

using namespace Mupfel;

Entity EntityManager::CreateEntity()
{
	uint32_t index;

	if (!freeList.empty())
	{
		/*
			Use recycled indices if available.
		*/
		index = freeList.back();
		freeList.pop_back();
	}
	else
	{
		/*
			No recycled indices left, use a new one.
		*/
		index = next_entity_index++;
	}

	/* Check if the alive vector is large enough. */
	if (index >= alive.size())
	{
		alive.resize((static_cast<size_t>(index) + 1) * 2, false);
	}

	alive[index] = true;

	current_entities++;

	return Entity(index);
}

bool Mupfel::EntityManager::IsAlive(Entity e) const { return (e.Index() < alive.size()) && alive[e.Index()]; }

void Mupfel::EntityManager::DestroyEntity(Entity e)
{
	/* Prevent double-destroy. */
	if (!IsAlive(e))
	{
		return;
	}

	alive[e.Index()] = false;

	/* Add the entity index to the recycled indices. */
	freeList.push_back(e.Index());

	/* Update the Entities count */
	current_entities--;
}

uint32_t Mupfel::EntityManager::GetCurrentEntities() const { return current_entities; }
