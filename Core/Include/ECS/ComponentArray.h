#pragma once
#include "Components/ComponentIndex.h"
#include "IComponentArray.h"
#include <cassert>
#include <concepts>
#include "Core/CoreEvents.h"
#include <vector>
#include <optional>
#include <mutex>
#include "Core/EventSystem.h"

namespace Mupfel
{
    template <typename FirstComponent, typename... Components>
    class View;
    class Registry;

    /* Forward declaration of accessor structure used in tests. */
    namespace Testing
    {
        struct ComponentArrayAccess;
    }

    template <typename T>
    concept ComponentType = std::assignable_from<T&, T> && std::swappable<T>;

    /**
     * @brief Host-side component buffer.
     *
     * This class implements the IComponentArray interface to provide a component array to store components of type \a T
     * in a sparse set. Entities that have a component of type \a T can be iterated efficiently using the "dense"
     * buffer.
     * @tparam T The type of the component.
     */
    template <ComponentType T>
    class ComponentArray : public IComponentArray
    {
        template <typename FirstComponent, typename... Components>
        friend class View;
        friend class Registry;
        friend struct Testing::ComponentArrayAccess;

    public:
        /**
         * Construct a component array.
         * @param capacity The initial capacity of the underlying vectors.
         */
        explicit ComponentArray(uint32_t capacity = 1000);

        /**
         * Default destructor.
         */
        ~ComponentArray() override = default;

        /**
         * Inserts a new component for the given entity.
         * If the entity already has a component of this type, it is overridden.
         *
         * \param e The entity to insert the component for.
         * \param component The component to insert.
         * \returns True, if the component was not present before, false otherwise.
         */
        bool Insert(Entity e, T component);

        /**
         * Remove the component from the given entity \a e.
         * @note If the given entity does not have the component, this function returns without
         * modifying any state.
         * @param e The entity whose component shall be removed.
         */
        void Remove(Entity e) final;

        /**
         * Check whether or not the given entity has the component.
         * @param e The entity to check.
         * @return True if \a e has the component, false otherwise.
         */
        [[nodiscard]] bool Has(Entity e) const final;

        /**
         * Return the component id that is associated with this component buffer.
         * @return The component id.
         */
        [[nodiscard]] size_t ComponentID() const final;

        /**
         * Retrieve the number of components that are currently stored in the array.
         * @return The number of currently stored components.
         */
        [[nodiscard]] uint32_t Size() const final;

        /**
         * Retrieve the component of the given entity.
         * @warning This function asserts that the entity actually has the component. In non-assert builds, calling
         * this function on an entity which does not have the component results in a vector out-of-bounds exception!
         * @param e The entity for which the component is retrieved.
         * @return The component of the given entity.
         */
        T& Get(Entity e);

        /**
         * Set the component of the given entity.
        * @warning This function asserts that the entity actually has the component. In non-assert builds, calling
         * this function on an entity which does not have the component results in a vector out-of-bounds exception!
         * @param e The entity for which the component is written.
         * @param val The new component data to write.
         */
        void Set(Entity e, T val);

        /**
         * Queue a component insertion for the given entity.
         * @note This function is intended to be called in multi threaded environments and does not instantly
         * change the underlying sparse set, to not disturb any simultaneous iterations over it. Instead, the operation
         * is queued to be executed in a single threaded context using Apply().
         * @note If the entity already has the component, the data is simply replaced.
         * @param e The entity for which the inserting is queued.
         * @param component The component to be inserted.
         */
        void QueueInsertion(Entity e, T component);

        /**
         * Queue a component removal for the given entity.
         * @note This function is intended to be called in multi threaded environments and does not instantly
         * change the underlying sparse set, to not disturb any simultaneous iterations over it. Instead, the operation
         * is queued to be executed in a single threaded context using Apply().
         * @param e The entity for which the removal shall be queued.
         */
        void QueueRemoval(Entity e);

        /**
         * Apply pending operations on the underlying component buffer.
         * @note This function expects a single threaded context.
         * @param signatures The entity signatures to be updated when changes to the components happen.
         * @param evt The event system to be used when components are added or removed.
         */
        void Apply(std::vector<Entity::Signature>& signatures, EventSystem& evt) final;

    private:
        /**
         * Structure describing a component operation.
         * If the component member exists, the operation is an insertion, otherwise it is handled as a removal.
         */
        struct ComponentOperation
        {
            Entity e;
            std::optional<T> component;
        };

        /**
         * The sparse vector. It is indexed by the entity index and stores invalid_entry in case the entity
         * does not have a component of that specific type. Otherwise, it stores the index into the dense and components
         * arrays where the component of that entity can be found.
         */
        std::vector<size_t> sparse;
        /** The dense vector. Every entry dense[i] stores the entity to which the component components[i] belongs. */
        std::vector<Entity> dense;
        /** The component vector. It stores the component data. */
        std::vector<T> components;
        /** A vector of pending operations to be processed. */
        std::vector<ComponentOperation> pendingOperations;
        /** A lock to protect the insertion of component operations in a multi threaded environment. */
        std::mutex lock;
    };

    template <ComponentType T>
    bool ComponentArray<T>::Insert(Entity e, T component)
    {
        /* If the sparse array is too small, we resize it using the given entity index as value. */
        if (e.Index() >= sparse.size())
        {
            const std::size_t new_size = (static_cast<std::size_t>(e.Index()) + 2) * 2;
            sparse.resize(new_size, invalid_entry);
        }

        /*
         * A value of "invalid_index" shows that the entity does not have the component yet.
         * If the entity already has a component of this type, we simply override it.
         */
        if (sparse[e.Index()] != invalid_entry)
        {
            components[sparse[e.Index()]] = std::move(component);
            return false;
        }

        /* New entries of the component and dense vectors are always pushed at the end. */
        sparse[e.Index()] = dense.size();

        /* The value at the index stores which entity uses the component */
        dense.push_back(e);
        components.push_back(std::move(component));
        return true;
    }

    template <ComponentType T>
    void ComponentArray<T>::QueueInsertion(Entity e, T component)
    {
        std::scoped_lock s_lock(lock);
        pendingOperations.push_back({.e = e, .component = std::move(component)});
    }

    template <ComponentType T>
    void ComponentArray<T>::QueueRemoval(Entity e)
    {
        std::scoped_lock s_lock(lock);
        pendingOperations.push_back({.e = e, .component = std::nullopt});
    }

    template <ComponentType T>
    void ComponentArray<T>::Apply(std::vector<Entity::Signature>& signatures, EventSystem& evt)
    {
        const size_t id = ComponentID();

        /*
         * If the user registered a listener that adds components of our type,
         * the pendingOperations vector is increased using push_back(), which invalidates the
         * iterator. Thus, we swap the pending operations and process them in a loop.
         */

        std::vector<ComponentOperation> ops;
        while (true)
        {
            {
                std::scoped_lock s_lock(lock);
                if (pendingOperations.empty()) break;
                ops.swap(pendingOperations);
            }

            for (auto& [e, comp] : ops)
            {
                if (comp) {
                    const bool added = Insert(e, std::move(*comp));
                    signatures[e.Index()].set(id);
                    if (added)
                    {
                        evt.AddImmediateEvent<ComponentAddedEvent>({e, signatures[e.Index()], id});
                    }
                } else if (Has(e)) {
                    evt.AddImmediateEvent<ComponentRemovedEvent>({e, signatures[e.Index()], id});
                    Remove(e);
                    signatures[e.Index()].reset(id);
                }
            }

            ops.clear();
        }
    }

    template <ComponentType T>
    ComponentArray<T>::ComponentArray(uint32_t capacity)
    {
        /* The sparse array has random insertion. */
        sparse.resize(capacity, invalid_entry);
        dense.reserve(capacity);
        components.reserve(capacity);
    }

    template <ComponentType T>
    void ComponentArray<T>::Remove(Entity e)
    {
        if (!Has(e))
        {
            return;
        }

        size_t comp_index = sparse[e.Index()];
        size_t last_index = dense.size() - 1;

        /* Swap the element that should be removed with the last one */
        std::swap(dense[comp_index], dense[last_index]);
        std::swap(components[comp_index], components[last_index]);

        /* the component order changed, update the sparse list */
        sparse[dense[comp_index].Index()] = comp_index;

        /* Delete the last component */
        components.pop_back();
        dense.pop_back();

        /* invalidate the component reference */
        sparse[e.Index()] = invalid_entry;
    }

    template <ComponentType T>
    bool ComponentArray<T>::Has(Entity e) const
    {
        if (e.Index() >= sparse.size() || sparse[e.Index()] == invalid_entry)
            return false;
        return dense[sparse[e.Index()]].Index() == e.Index();
    }

    template <ComponentType T>
    size_t ComponentArray<T>::ComponentID() const { return ComponentIndex::Index<T>(); }

    template <ComponentType T>
    uint32_t ComponentArray<T>::Size() const
    {
        return static_cast<uint32_t>(dense.size());
    }

    template <ComponentType T>
    T& ComponentArray<T>::Get(Entity e)
    {
        assert(Has(e) && "Given Entity does not currently have a component of this type!");
        return components[sparse[e.Index()]];
    }

    template <ComponentType T>
    void ComponentArray<T>::Set(Entity e, T val)
    {
        assert(Has(e) && "Given Entity does not currently have a component of this type!");
        components[sparse[e.Index()]] = std::move(val);
    }
} // namespace Mupfel
