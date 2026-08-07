#pragma once

#include "ECS/Entity.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>

namespace Mupfel
{
class ComponentIndex
{
public:
	template <typename T> static size_t Index() noexcept
	{
		static const size_t id = Claim();
		return id;
	}

private:
	static size_t Claim() noexcept
	{
		const size_t id = comp_counter.fetch_add(1, std::memory_order_relaxed);

		if (id >= Entity::MAX_COMPONENTS_TYPES)
		{
			std::fprintf(
				stderr, "Mupfel: more than %zu distinct component types registered. ", Entity::MAX_COMPONENTS_TYPES);
			std::abort();
		}
		return id;
	}

	static inline std::atomic<size_t> comp_counter{0};
};
} // namespace Mupfel