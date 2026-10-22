#pragma once
#include "Entity.h"
#include <cstdint>
#include <limits>

namespace Mupfel
{
    class EventSystem;

    /**
     * The IComponentArray interface provides functionality for the Registry to issue type-independent
     * commands to component arrays.
     */
    class IComponentArray
    {
    public:
        /**
         * Default constructor.
         */
        virtual ~IComponentArray() = default;

        /**
         * Remove the component that belongs to \a e if it exists.
         * @note If \a e does not have the component, this functions does nothing.
         * @param e The enitity from which the component shall be removed.
         */
        virtual void Remove(Entity e) = 0;

        /**
         * Check whether or not the given entity has the component.
         * @param e The entity to check.
         * @return True if \a e has the component, false otherwise.
         */
        [[nodiscard]] virtual bool Has(Entity e) const = 0;

        /**
         * Retrieve the number of components that are currently stored in the buffer.
         * @return The number of currently stored components.
         */
        [[nodiscard]] virtual uint32_t Size() const = 0;

        /**
         * Return the component id that is associated with this component buffer.
         * @note Classes that implement this interface are required to implement an ID
         * scheme that uniquely identifies a component type!
         * @return The component id.
         */
        [[nodiscard]] virtual size_t ComponentID() const = 0;

        /**
         * Apply pending operations on the underlying component buffer.
         * In a multi threaded environment, classes should defer operations on underlying data structures and
         * process them in this function. In single threaded environments, this function can be implemented as a no-op.
         * @note The callee can expect this function to be called in a single threaded context.
         * @param signatures The entity signatures to be updated when changes to the components happen.
         * @param evt The event system to be used when components are added or removed.
         */
        virtual void Apply(std::vector<Entity::Signature>& signatures, EventSystem& evt) = 0;

        /** Sentinel used by sparse-set implementations to mark "no component". */
        static constexpr size_t invalid_entry = std::numeric_limits<size_t>::max();
    };
} // namespace Mupfel
