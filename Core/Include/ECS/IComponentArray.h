#pragma once
#include "Entity.h"
#include <cstdint>
#include <cstddef>
#include <limits>

namespace Mupfel
{

/**
 * Type-erased interface every `ComponentArray<T>` implements, so `Registry` can hold a
 * heterogeneous `component_buffer` of them and operate on an entity without knowing `T`.
 */
class IComponentArray
{
public:
	virtual ~IComponentArray() = default;

	/** Removes the component for `e`, if present. A no-op otherwise. */
	virtual void Remove(Entity e) = 0;

	/** Whether `e` currently has a component in this array. */
	virtual bool Has(Entity e) const = 0;

	/** Number of components currently stored. */
	virtual uint32_t Size() const = 0;

	virtual size_t ComponentID() const = 0;

	/** Sentinel used by sparse-set implementations to mark "no component". */
	static constexpr size_t invalid_entry = std::numeric_limits<size_t>::max();
};
} // namespace Mupfel