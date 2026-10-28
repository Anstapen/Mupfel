#pragma once
#include "Core/EventSystem.h"
#include "Entity.h"
#include <algorithm>
#include <concepts>
#include <future>
#include <memory>
#include <typeindex>
#include <vector>
#include "ComponentArray.h"
#include "Core/Scene.h"
#include "Core/ThreadPool.h"
#include "ECS/Components/ComponentIndex.h"
#include <array>
#include <mutex>
#include <atomic>

namespace Mupfel
{
    template <typename FirstComponent, typename... Components>
    class View;
    class CollisionSystem;
    class Application;

    /**
     * A Registry holds a complete ECS context. It provides methods to create / destroy
     * entities, add / remove components and means to iterate over entities based on a set
     * of components.
     *
     * The registry considers a multi threaded environment, using the following strategy:
     * The state of the component data is not updated when adding entities or components. Instead,
     * it nees to be explicitly synced in a single threaded context using the Sync() function.
     *
     * That also means that when iterating over sets of components using the View class, the iterators
     * cannot be invalidated by the user and the loop always processes all entities and compoenents that exist
     * at the point in time.
     *
     * Every registry object has a current scene. Adding a new entity encodes the current scene into the entity.
     * Subsequent iterations filter out entities that do not belong to the current scene.
     */
    class Registry
    {
        template <typename FirstComponent, typename... Components>
        friend class View;
        friend class CollisionSystem;

    public:
        /**
         * The registry constructor needs an event system (to issue entity and component events) and
         * a thread pool (for parallel searches).
         */
        Registry(EventSystem& in_evt_sys, ThreadPool& in_pool) : evt_system(in_evt_sys), thread_pool(in_pool)
        {
        }

        /**
         * Destructor.
         */
        ~Registry();

        /**
         * Deleted copy constructor.
         */
        Registry(const Registry &other) = delete;

        /**
         * Deleted copy assignment operator.
         */
        Registry& operator=(const Registry &other) = delete;

        /**
         * Create a new entity.
         * @note The returned entity has a valid index and generation, and the user can call AddComponent and friends
         * to attach data to the entity. But the components are not added to the entity until the next Flush().
         * When Flush() is called, the entity creation is finished and a EntityCreatedEvent is published.
         * @return The newly created entity.
         */
        Entity CreateEntity();

        /**
         * Destroy an entity.
         * @note After this function returns, the entity is dead, but components of the entity are not yet destroyed,
         * to not disturb component iteration. The next time Flush() is called, the components are removed from the
         * entity and ComponentRemovedEvents are published. After that, the entity is finally removed and an
         * EntityDestroyedEvent is issued.
         * \param e The entity to be destroyed.
         */
        void DestroyEntity(Entity e);

        /**
         * Returns the number of alive entities.
         * @return The number of currently alive entities.
         */
        uint32_t GetCurrentEntities() const;

        /**
         * Synchronization point to apply pending entity and component operations.
         *
         * @warning This function expects a single threaded context!
         */
        void Apply();

        /**
         * Retrieve the component signature of the given entity.
         * @param entity The entity for which the component signature shall be returned.
         * @return The component signature of the given entity.
         */
        [[nodiscard]] Entity::Signature GetSignature(Entity entity) const;

        /**
         * Retrieve the scene mask of an entity.
         * @param entity The entity for which to retrieve the scene mask.
         * @return The scene mask of the given entity.
         */
        [[nodiscard]] Scene::SceneMask GetSceneMask(Entity entity) const;

        /**
         * Set a new active scene.
         * @param scene The scene that should be active for this registry.
         */
        void SetActiveScene(SceneHandle scene);

        /**
         * Retrieve the currently active scene in the registry.
         * @return The currently active scene.
         */
        [[nodiscard]] SceneHandle GetActiveScene() const;

        /**
         * Retrieves the current scene, encoded as a bitmask.
         * @note If the current scene has an index of N, the returned bitmask has bit N set.
         * @return The bitmask identifying the current scene.
         */
        [[nodiscard]] Scene::SceneMask GetActiveSceneMask() const;

        /**
         * Provide a view over all entities containing the given combination of components.
         * @tparam Components All component types that the wanted entities need to have.
         * @return A view object constructed from this registry.
         */
        template <typename... Components>
        View<Components...> Each() { return View<Components...>(*this); }

        /**
         * Execute the given function for all entities which contain components of the specified type.
         * @tparam Components The component types of the components that should be present.
         * @tparam F The function signature.
         * @param function The function to execute on every matching entity´.
         */
        template <typename... Components, typename F>
            requires std::invocable<F&, Entity, Components&...>
        void ParallelForEach(F&& function);

        /**
         * Add a component to an entity.
         * @note The component addition is deferred until the next call of Apply().
         * @note If the given entity already has a component of the specified type, the component data is overwritten.
         * @note Only if the component is added to the entity (not replaced), an immediate ComponentAddedEvent is
         * issued.
         * @tparam T The component type.
         * @tparam Args The types of the provided constructor arguments.
         * @param e The entity to which the component should be added.
         * @param args The constructor arguments to construct an object of type \a T.
         */
        template <typename T, typename... Args>
        void AddComponent(Entity e, Args&&... args);

        /**
         * Add a component to an entity.
         * @note The component addition is deferred until the next call of Apply().
         * @note If the given entity already has a component of the specified type, the component data is overwritten.
         * @note Only if the component is added to the entity (not replaced), an immediate ComponentAddedEvent is
         * issued.
         * @tparam T The component type.
         * @param e The entity to which the component should be added.
         * @param component The component data.
         */
        template <typename T>
        void AddComponent(Entity e, T component);

        /**
         * Remove a component from an entity.
         * @note The component removal is deferred until the next call of Apply().
         * @note If the given entity does not have the specified component, nothing is done.
         * @note If the component is removed from the entity, an immediate ComponentRemovedEvent is issued.
         * @tparam T The component type.
         * @param e The entity from which the component shall be removed.
         */
        template <typename T>
        void RemoveComponent(Entity e);

        /**
         * Retrieve a reference to the specified component of the given entity.
         * @warning If the given entity does not have a component of the specified type, this function does an out
         * of bounds read and throws!
         * @tparam T The component type.
         * @param e The entity from which to retrieve the component data.
         * @return A reference to the component data of the given entity.
         */
        template <ComponentType T>
        T& GetComponent(Entity e);

        template <ComponentType T>
        T* TryGetComponent(Entity e);

        /**
         * Write the component data of the given entity.
         * @warning If the given entity does not have a component of the specified type, this function does an out
         * of bounds read and throws!
         * @tparam T The component type.
         * @param e The entity from which to write the component data.
         * @param comp The component data to write.
         */
        template <typename T>
        void SetComponent(Entity e, T comp);

        /**
         * Check whether or not the given entity has a component of the specified type.
         * @tparam T The component type.
         * @param e The entity to check.
         * @return True if the entity has a component of the specified type, false otherwise.
         */
        template <typename T>
        bool HasComponent(Entity e);

        /**
         * Construct a bitmask based on the given component types.
         * @tparam Components The component types to construct the bitmask for.
         * @return A bitmask matching the given component type combination.
         */
        template <typename... Components>
        static constexpr Entity::Signature ComponentSignature()
        {
            Entity::Signature sig;
            (sig.set(ComponentIndex::Index<Components>()), ...);
            return sig;
        }

        /**
         * Construct a bitset for the given scene handle.
         * @param scene The scene handle to construct the bitmask for.
         * @return A bitmask matching the given scene handle.
         */
        static std::bitset<Scene::MAX_SCENES> SceneMask(SceneHandle scene)
        {
            std::bitset<Scene::MAX_SCENES> sig;
            sig.set(scene);
            return sig;
        }

    private:
        /**
         * TODO
         * @tparam T
         * @return
         */
        template <typename T>
        ComponentArray<T>& GetComponentArray();

        static_assert(sizeof(void*) == 8);

        /**
         * TODO
         * @param e
         * @return
         */
        static void* ToUserData(Entity e);

        /**
         * TODO
         * @param data
         * @return
         */
        static constexpr Entity EntityFromData(uint64_t data)
        {
            const auto generation = static_cast<uint32_t>(data >> 32);
            const auto index = static_cast<uint32_t>(data & 0xFFFFFFFF);
            return Entity(index, generation);
        }

        /**
         * TODO
         * @param e
         * @param s
         */
        void CompleteEntityCreation(const Entity& e, SceneHandle s);

        /**
         * TODO
         * @param e
         */
        void CompleteEntityRemoval(const Entity& e);

        /**
         * TODO
         */
        struct EntityOperation
        {
            Entity e;
            SceneHandle scene;
            bool create;
        };

        /**
         * TODO
         */
        enum class EntityState : uint8_t
        {
            DEAD,
            PENDING_CREATE,
            ALIVE,
            PENDING_DESTROY
        };

        /** Where entity/component lifecycle events are fired. */
        EventSystem& evt_system;
        /** Thread pool for the parallel ECS operations. */
        ThreadPool& thread_pool;
        /** Allocates/recycles entity indices. */
        EntityManager entity_manager;
        /** Per-entity-index signature, resized alongside entities. */
        std::vector<Entity::Signature> signatures;
        /** Per-entity-index scene membership, resized alongside entities. */
        std::vector<Scene::SceneMask> sceneMask;
        /** Per-component-type storage. */
        std::array<std::atomic<IComponentArray*>, Entity::MAX_COMPONENTS_TYPES> componentBuffer{};
        std::mutex componentBufferLock;
        /** The currently active scene. */
        SceneHandle active_scene = 0;
        /** Command buffer of pending entity operations. */
        std::vector<EntityOperation> pendingEntityOperations;
        /** Mutex to protect accesses to the entity operation buffer. */
        std::mutex pendingEntityOperationsLock;
        /** Per-entity state. */
        std::vector<EntityState> entityStates;
        /** Mutex to protect entity states. */
        std::mutex entityStateLock;
    };

    template <typename... Components, typename F>
        requires std::invocable<F&, Entity, Components&...>
    void Registry::ParallelForEach(F&& function)
    {
        auto& pool = thread_pool;
        const uint32_t num_threads = std::max<uint32_t>(1u, static_cast<uint32_t>(pool.GetThreadCount()));

        using BaseComponent = std::tuple_element_t<0, std::tuple<Components...>>;
        auto& array = GetComponentArray<BaseComponent>();
        const auto& dense = array.dense;
        const auto total = static_cast<uint32_t>(dense.size());

        auto arrays = std::make_tuple(&GetComponentArray<Components>()...);

        if (total == 0)
        {
            return;
        }

        const uint32_t chunk = (total + num_threads - 1) / num_threads;
        std::vector<std::future<void>> jobs;
        jobs.reserve(num_threads);

        const Entity::Signature required = Registry::ComponentSignature<Components...>();
        const Scene::SceneMask required_scene = GetActiveSceneMask();

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
                        const Entity e = dense[i];

                        const auto& sig = GetSignature(e);
                        const Scene::SceneMask scene_mask = GetSceneMask(e);

                        /* check if the entity has all the needed components */
                        if ((sig & required) != required || (scene_mask & required_scene) != required_scene)
                            continue;

                        /* Call given function on the entity */
                        std::apply([&](auto*... arr) { function(e, arr->Get(e)...); }, arrays);
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
        }
        /* If user code caused an exception, throw it now. */
        if (first)
        {
            std::rethrow_exception(first);
        }
    }

    template <typename T, typename... Args>
    void Registry::AddComponent(Entity e, Args&&... args)
    {
        AddComponent(e, T(std::forward<Args>(args)...));
    }

    template <typename T>
    void Registry::AddComponent(Entity e, T component)
    {
        /* Only add a component to an alive entity. */
        if (!entity_manager.IsAlive(e))
        {
            return;
        }

        /* For now, we reject entities that are not alive. */
        {
            std::scoped_lock lock(entityStateLock);
            if (e.Index() >= entityStates.size())
            {
                return;
            }
            const EntityState state = entityStates[e.Index()];

            if ((state != EntityState::ALIVE) && (state != EntityState::PENDING_CREATE))
            {
                return;
            }
        }

        ComponentArray<T>& storage = GetComponentArray<T>();
        storage.QueueInsertion(e, std::move(component));
    }

    template <typename T>
    void Registry::RemoveComponent(Entity e)
    {
        /* For now, we reject entities that are not alive. */
        {
            std::scoped_lock lock(entityStateLock);
            if (e.Index() >= entityStates.size())
            {
                return;
            }
            const EntityState state = entityStates[e.Index()];

            if ((state != EntityState::ALIVE) && (state != EntityState::PENDING_CREATE))
            {
                return;
            }
        }

        if (!entity_manager.IsAlive(e))
        {
            return;
        }

        ComponentArray<T>& storage = GetComponentArray<T>();
        storage.QueueRemoval(e);
    }

    template <ComponentType T>
    T& Registry::GetComponent(Entity e) { return GetComponentArray<T>().Get(e); }

    template <ComponentType T>
    T* Registry::TryGetComponent(Entity e)
    {
        if (HasComponent<T>(e))
        {
            return &GetComponent<T>(e);
        }

        return nullptr;
    }

    template <typename T>
    void Registry::SetComponent(Entity e, T comp) { GetComponentArray<T>().Set(e, comp); }

    template <typename T>
    bool Registry::HasComponent(Entity e)
    {
        return GetComponentArray<T>().Has(e);
    }

    template <typename T>
    ComponentArray<T>& Registry::GetComponentArray()
    {
        const size_t comp_index = ComponentIndex::Index<T>();

        /* Fast path: check if the component array already exists. */
        IComponentArray* arr = componentBuffer[comp_index].load(std::memory_order_acquire);

        if (!arr) [[unlikely]]
        {
            /* The buffer does not yet exist, we need to lock the mutex and re-check. */
            std::scoped_lock lock(componentBufferLock);
            arr = componentBuffer[comp_index].load(std::memory_order_acquire);
            if (!arr)
            {
                /* Component array does not exist, create it. */
                arr = new ComponentArray<T>(10);
                componentBuffer[comp_index].store(arr, std::memory_order_release);
            }
        }

        return *static_cast<ComponentArray<T>*>(arr);
    }
} // namespace Mupfel

#include "ECS/View.h"
