#include "Entity.h"

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
    else {
        /*
            No recycled indices left, use a new one.
        */
        index = generations.size();
        generations.push_back(0);
    }

    return Entity(index, generations[index]);
}

void Mupfel::EntityManager::DestroyEntity(Entity e)
{
    /*
        If the entity not alive anymore, we do not need to destroy it.
    */
    if (!isAlive(e))
    {
        return;
    }

    /*
        Add the entity index to the recycled indices.
    */
    freeList.push_back(e.Index());
    generations[e.Index()]++;
}

bool Mupfel::EntityManager::isAlive(Entity e) const
{
    return e.Index() < generations.size() && generations[e.Index()] == e.Generation();
}
