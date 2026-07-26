#pragma once
//
// Shared scaffolding for the Mupfel ECS benchmarks.
//
// The registry + view + component-access paths need nothing but an EventSystem and a Registry (both
// plain host-memory objects), so `World` below builds a fully populated ECS in isolation -- no
// Application, window, or Vulkan device is required. (The one exception is ParallelForEach, which
// reaches into the Application singleton for its thread pool; see Bench_ParallelForEach.cpp.)

#include "Core/EventSystem.h"
#include "ECS/Registry.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Movement.h"

#include <nanobench.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ostream>
#include <random>
#include <vector>

namespace MupfelBench {

/**
 * Applies the reliability knobs every benchmark group shares, then returns the bench for chaining
 * (`ApplyDefaults(bench).title(...).unit(...)`):
 *   - warmup(20)                : a warmup pass so caches / CPU clock ramp are settled before timing.
 *   - minEpochTime(50ms)        : each epoch runs at least 50ms worth of iterations, so even the
 *                                 fastest cases accumulate hundreds of samples and report a stable
 *                                 median instead of nanobench's "Unstable, increase iterations" warning.
 * Runtime vs. stability is a single dial here: lower minEpochTime for a faster suite, raise it for
 * tighter error bars.
 */
inline ankerl::nanobench::Bench& ApplyDefaults(ankerl::nanobench::Bench& bench)
{
	return bench.warmup(20).minEpochTime(std::chrono::milliseconds(50));
}

/**
 * An isolated ECS instance: an EventSystem, a Registry that fires into it, and the list of entities
 * created so far (in creation order). Lives entirely on the CPU heap -- construct as many as you like.
 */
struct World
{
	Mupfel::EventSystem			events;
	Mupfel::Registry			registry{ events };
	std::vector<Mupfel::Entity> entities;
};

/**
 * Creates `count` entities, each with a Transform. Every `movement_stride`-th entity additionally
 * gets a Movement component, so the fraction of entities matching a Movement query is 1/stride
 * (stride 1 => all match, 2 => half, 10 => a tenth). `movement_stride == 0` means no entity gets a
 * Movement component. Entities are recorded in creation order in `world.entities`.
 */
inline void Populate(World& world, uint32_t count, uint32_t movement_stride = 1)
{
	world.entities.clear();
	world.entities.reserve(count);

	for (uint32_t i = 0; i < count; ++i)
	{
		Mupfel::Entity e = world.registry.CreateEntity();

		Mupfel::Transform t;
		t.pos_x = static_cast<float>(i);
		t.pos_y = static_cast<float>(i) * 0.5f;
		world.registry.AddComponent<Mupfel::Transform>(e, t);

		if (movement_stride != 0 && (i % movement_stride) == 0)
		{
			Mupfel::Movement m;
			m.velocity_x = 1.0f;
			m.velocity_y = 2.0f;
			world.registry.AddComponent<Mupfel::Movement>(e, m);
		}

		world.entities.push_back(e);
	}
}

/** How many entities `Populate(count, stride)` gives a Movement component to. */
inline uint32_t MovementMatches(uint32_t count, uint32_t movement_stride)
{
	if (movement_stride == 0)
		return 0;
	return (count + movement_stride - 1) / movement_stride;
}

/**
 * A fixed-seed shuffled copy of `world.entities`, for random-access benchmarks: iterating these
 * instead of the creation-ordered list measures sparse-set indirection under cache-unfriendly access
 * rather than the best case where the dense array is walked in order.
 */
inline std::vector<Mupfel::Entity> ShuffledEntities(const World& world, uint32_t seed = 0xC0FFEEu)
{
	std::vector<Mupfel::Entity> shuffled = world.entities;
	std::mt19937				rng(seed);
	std::shuffle(shuffled.begin(), shuffled.end(), rng);
	return shuffled;
}

/**
 * If `csv` is non-null, appends this bench's results to it in CSV form (one row per benchmarked case)
 * for machine-readable regression tracking. The human-readable markdown table always goes to stdout.
 */
inline void RenderCsv(ankerl::nanobench::Bench& bench, std::ostream* csv)
{
	if (csv)
		bench.render(ankerl::nanobench::templates::csv(), *csv);
}

} // namespace MupfelBench
