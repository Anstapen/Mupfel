#pragma once
#include "Core/EventBuffer.h"
#include <bitset>
#include <cstdint>
#include <vector>
#include <mutex>

namespace Mupfel
{
    /* Forward declaration of classes that need friend access to the entity. */
    class EntityManager;
    template <typename FirstComponent, typename... Components>
    class View;
    class Registry;

    /* Forward declaration of accessor structure used in tests. */
    namespace Testing
    {
        struct EntityAccess;
        struct EntityManagerAccess;
    }

    /**
     * This class defines the entity. An Entity is a 64 bit value, with an index and
     * a generation.
     */
    class Entity
    {
        /* As the Entity Manager provides the factory method for the entity, it is a friend. */
        friend class EntityManager;

        /* The view constructs entities based on the underlying component buffers. */
        template <typename FirstComponent, typename... Components>
        friend class View;

        /** struct defined by the testing framework to have private member access. */
        friend struct Testing::EntityAccess;

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

        /**
         * Return the index of the entity.
         * @return The index of the entity.
         */
        [[nodiscard]] constexpr uint32_t Index() const { return index; }

        /**
         * Retrieve the entity generation.
         * @return The entity generation.
         */
        [[nodiscard]] constexpr uint32_t Gen() const { return generation; }

        /** The default spaceship operator for defining ordering operations on entities. */
        friend constexpr auto operator<=>(const Entity& a, const Entity& b) = default;

        /** The default equals operator. */
        friend constexpr bool operator==(const Entity& a, const Entity& b) = default;

    private:
        /**
         * Private entity constructor. Entity creation is managed by the entity manager.
         * @param in_index The index.
         * @param in_generation The generation.
         */
        explicit constexpr Entity(uint32_t in_index, uint32_t in_generation) : index(in_index),
                                                                               generation(in_generation)
        {
        }

        /** The index of the entity. */
        uint32_t index;
        /** The generation of the entity. */
        uint32_t generation;
    };

    static_assert((sizeof(Entity) == 8), "Error: Size of Entity Class is not 8 bytes!");

    /**
     * This class manages a pool of entities. It provides a factory function to create and
     * destroy entities and other convenience functions. All operations that modify its state
     * are serialized using a mutex.
     */
    class EntityManager
    {
    public:

        /** struct defined by the testing framework to have private member access. */
        friend struct Testing::EntityManagerAccess;

        /**
         * Default constructor of the entity manager. It reserves space for 100 entities.
         */
        EntityManager() : current_entities(0), next_entity_index(1)
        {
            freeList.reserve(100);
            alive.reserve(100);
            generationList.reserve(100);
        }

        /**
         * Create a new entity.
         *
         * @note The entity manager attempts to recycle destroyed entities.
         * @note This function locks the mutex.
         * @return The created entity.
         */
        Entity CreateEntity();

        /**
         * Checks whether or not the given entity is currently alive.
         * @note This function locks the mutex.
         * @param e The entity to check.
         * @return True if the entity is currently alive, false otherwise.
         */
        bool IsAlive(Entity e) const;

        /**
         * Retrieve the current generation of an entity.
         * @note This function locks the mutex.
         * @param e The entity for which to retrieve the generation.
         * @return The generation of the entity.
         */
        [[nodiscard]] uint32_t GetGen(Entity e) const;

        /**
         * Destroy a given entity.
         * @note This function locks the mutex.
         * @param e The entity to destroy.
         */
        void DestroyEntity(Entity e);

        /**
         * Retrieve the amount of entities that are currently alive.
         * @note This function locks the mutex.
         * @return The number of currently alive entities.
         */
        uint32_t GetCurrentEntities() const;

    private:
        /**
         * Check if \a e is alive.
         * @note The caller must hold the lock \ref lock.
         *
         * @param e Entity to check.
         * @return True if the entity is alive, false otherwise.
         */
        bool IsAliveLocked(Entity e) const;

        /** Number of currently-live entities. */
        uint32_t current_entities;
        /** Next never-before-used index to hand out. */
        uint32_t next_entity_index;
        /** Generation counter for every entity. */
        std::vector<uint32_t> generationList;
        /** Recycled indices from destroyed entities, reused first. */
        std::vector<uint32_t> freeList;
        /** A vector of alive entities. */
        std::vector<bool> alive;
        /** The mutex to be used for updating the state. */
        mutable std::mutex lock;
    };
} // namespace Mupfel
