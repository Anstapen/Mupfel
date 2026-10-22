#include "ECS/Registry.h"
#include <cassert>

using namespace Mupfel;

Registry::~Registry()
{
    for (auto & buffer : componentBuffer)
    {
        delete buffer.load(std::memory_order_relaxed);
    }
}

Entity Registry::CreateEntity()
{
    const Entity e = entity_manager.CreateEntity();
    /* Set the entity state. */
    {
        std::scoped_lock lock(entityStateLock);
        if (entityStates.size() <= e.Index()) [[unlikely]]
        {
            entityStates.resize((e.Index() + 1) * 2, EntityState::DEAD);
        }

        entityStates[e.Index()] = EntityState::PENDING_CREATE;
    }
    {
        std::scoped_lock lock(pendingEntityOperationsLock);
        pendingEntityOperations.push_back({.e = e, .scene = active_scene, .create = true});
    }
    return e;
}

void Registry::DestroyEntity(Entity e)
{
    /* Check the entity generation. */
    if (!entity_manager.IsAlive(e))
    {
        return;
    }
    /* Check the entity state. A destruction of the entity only makes sense if the state is ALIVE or PENDING_CREATE. */
    {
        std::scoped_lock lock(entityStateLock);

        if (entityStates.size() <= e.Index())
        {
            /* The entity was never created. */
            return;
        }

        const EntityState state = entityStates[e.Index()];

        if ((state == EntityState::PENDING_DESTROY) || (state == EntityState::DEAD))
        {
            /* Double destruction does not make sense. */
            return;
        }

        entityStates[e.Index()] = EntityState::PENDING_DESTROY;
    }

    /* Add the operation to execute later. */
    {
        std::scoped_lock lock(pendingEntityOperationsLock);
        pendingEntityOperations.push_back({.e = e, .scene = active_scene, .create = false});
    }
}

void Registry::Apply()
{

    std::vector<EntityOperation> ops;
    while (true)
    {
        {
            std::scoped_lock s_lock(pendingEntityOperationsLock);
            if (pendingEntityOperations.empty()) break;
            ops.swap(pendingEntityOperations);
        }

        for (const auto& [e, scene, create] : ops)
        {
            if (create)
            {
                CompleteEntityCreation(e, scene);
            }
        }

        /* Flush the component buffers. */
        for (const auto &buffer : componentBuffer)
        {
            if (IComponentArray *ptr = buffer.load(std::memory_order_relaxed))
            {
                ptr->Apply(signatures, evt_system);
            }
        }

        for (const auto& [e, scene, create] : ops)
        {
            if (!create)
            {
                CompleteEntityRemoval(e);
            }
        }

        ops.clear();
    }
}

uint32_t Registry::GetCurrentEntities() const { return entity_manager.GetCurrentEntities(); }

Entity::Signature Registry::GetSignature(Entity entity) const
{
    if (entity.Index() >= signatures.size())
    {
        return {0x0};
    }

    return signatures[entity.Index()];
}

Scene::SceneMask Registry::GetSceneMask(Entity entity) const
{
    assert(entity.Index() < sceneMask.size() && "Entity does not have a scene mask!");

    return sceneMask[entity.Index()];
}

void Registry::SetActiveScene(SceneHandle scene)
{
    assert((scene < Scene::MAX_SCENES) && "Scene handle out of range!");

    active_scene = scene;
}

SceneHandle Registry::GetActiveScene() const { return active_scene; }

Scene::SceneMask Registry::GetActiveSceneMask() const { return SceneMask(active_scene); }

void* Registry::ToUserData(Entity e)
{
    uint64_t data = 0;
    /* entity index in the 32 LSBs. */
    data = e.Index();
    /* Generation in the 32 MSBs. */
    data |= static_cast<uint64_t>(e.Gen()) << 32;
    return reinterpret_cast<void*>(data);
}

void Registry::CompleteEntityCreation(const Entity& e, SceneHandle s)
{
    /* Update the Component Signature of the Entity */
    if (signatures.size() <= e.Index()) [[unlikely]]
    {
        signatures.resize((e.Index() + 1) * 2, Entity::Signature(0x0));
    }
    else
    {
        signatures[e.Index()] = 0x0;
    }

    /* Update the scene mask of the Entity */
    if (sceneMask.size() <= e.Index()) [[unlikely]]
    {
        sceneMask.resize((e.Index() + 1) * 2, Scene::SceneMask(0x0));
    }

    sceneMask[e.Index()] = SceneMask(s);

    /* Set the entity state */
    {
        std::scoped_lock lock(entityStateLock);
        if (entityStates[e.Index()] == EntityState::PENDING_CREATE)
        {
            entityStates[e.Index()] = EntityState::ALIVE;
        }
    }

    /* Entity is created successfully, notify everyone */
    evt_system.AddImmediateEvent<EntityCreatedEvent>(e);
}

void Registry::CompleteEntityRemoval(const Entity& e)
{
    /* Check if the entity is alive. */
    if (!entity_manager.IsAlive(e))
    {
        return;
    }

    /* We have to remove the entity from all component lists */
    for (const auto& i : componentBuffer)
    {
        IComponentArray* storage = i.load(std::memory_order_relaxed);

        if (!storage || !storage->Has(e))
        {
            continue;
        }

        evt_system.AddImmediateEvent<ComponentRemovedEvent>({e, signatures[e.Index()], storage->ComponentID()});
        storage->Remove(e);
    }

    entity_manager.DestroyEntity(e);
    signatures[e.Index()].reset();
    sceneMask[e.Index()].reset();

    /* Set the entity state */
    {
        std::scoped_lock lock(entityStateLock);
        entityStates[e.Index()] = EntityState::DEAD;
    }

    /* Create an Entity Destroyed Event. */
    evt_system.AddImmediateEvent<EntityDestroyedEvent>(e);
}
