#include "ECS/Entity.h"
#include <iostream>
#include <cassert>

using namespace Mupfel;

Entity EntityManager::CreateEntity()
{
    uint32_t index = 0;
    uint32_t generation = 0;

    {
        std::scoped_lock s_lock(lock);
        if (!freeList.empty())
        {
            /* Use recycled indices if available. */
            index = freeList.back();
            freeList.pop_back();

            /* The recycled index was already in use, it has a valid generation. */
            assert(generationList.size() > index);
        }
        else
        {
            /* No recycled indices left, use a new one. */
            index = next_entity_index++;
        }

        /* Check if the alive vector is large enough. */
        if (index >= alive.size())
        {
            alive.resize((static_cast<size_t>(index) + 1) * 2, false);
        }

        /* Check if the generation vector is large enough. */
        if (index >= generationList.size())
        {
            generationList.resize((static_cast<size_t>(index) + 1) * 2, 0);
        }

        /* We pre-increment to make gen 0 invalid. */
        generation = ++generationList[index];

        alive[index] = true;

        current_entities++;
    }

    return Entity(index, generation);
}

bool EntityManager::IsAlive(Entity e) const
{
    std::scoped_lock s_lock(lock);
    return IsAliveLocked(e);
}

uint32_t EntityManager::GetGen(Entity e) const
{
    std::scoped_lock s_lock(lock);
    if (e.Index() >= generationList.size())
    {
        return 0;
    }

    return generationList[e.Index()];
}

void EntityManager::DestroyEntity(Entity e)
{
    std::scoped_lock s_lock(lock);
    /* Prevent double-destroy. */
    if (!IsAliveLocked(e))
    {
        return;
    }

    alive[e.Index()] = false;

    /* Add the entity index to the recycled indices. */
    freeList.push_back(e.Index());

    /* Update the Entities count */
    current_entities--;
}

uint32_t EntityManager::GetCurrentEntities() const
{
    std::scoped_lock s_lock(lock);
    return current_entities;
}

bool EntityManager::IsAliveLocked(Entity e) const
{
    const uint32_t index = e.Index();
    const bool generationMatch = index < generationList.size() && generationList[index] == e.Gen();
    const bool isAlive = (index < alive.size()) && alive[index];
    return generationMatch && isAlive;
}
