// ParallelForEach vs. single-threaded View iteration.
//
// Registry::ParallelForEach splits the base component's dense array across the engine thread pool. It
// obtains that pool from Application::GetCurrentThreadPool(), so this benchmark is the one place that
// pulls in the Application singleton. Constructing it only default-constructs (Application::Init() is
// never called), so no window is opened and no Vulkan device is created -- it just spins up the worker
// threads. We prime the singleton once, outside the timed region, so first-touch thread creation does
// not land inside a measured epoch.
//
// Both variants do identical per-entity work (a movement integration step writing into Transform), so
// the comparison isolates threading speedup/overhead. Small entity counts are expected to favor the
// single-threaded view (dispatch overhead dominates); large counts should favor ParallelForEach.

#include "BenchCommon.h"
#include "Benchmarks.h"

#include "Core/Application.h"

#include <vector>

using namespace Mupfel;

namespace MupfelBench {

namespace {

// One movement-integration step; identical in the serial and parallel variants.
inline void Integrate(Transform& t, const Movement& m, float dt)
{
	t.pos_x += m.velocity_x * dt;
	t.pos_y += m.velocity_y * dt;
	t.rotation += m.angular_velocity * dt;
}

} // namespace

void RunParallelForEachBenchmarks(std::ostream* csv)
{
	constexpr float dt = 1.0f / 60.0f;

	// Force construction of the Application singleton (and its thread pool) up front, off the clock.
	ThreadPool& pool = Application::GetCurrentThreadPool();

	ankerl::nanobench::Bench bench;
	ApplyDefaults(bench)
		.title("MovementSystem integration: ParallelForEach vs single-threaded View (" + std::to_string(pool.GetThreadCount()) + " threads)")
		.unit("entity");

	for (uint32_t count : { 10000u, 50000u, 200000u })
	{
		World world;
		Populate(world, count, 1); // every entity has both Transform and Movement
		const uint32_t matched = count;

		// Single-threaded baseline: a plain View doing the integration inline.
		bench.batch(matched).run("view<Transform, Movement>  serial   " + std::to_string(count),
			[&]
			{
				for (auto [e, t, m] : world.registry.view<Transform, Movement>())
					Integrate(t, m, dt);
			});

		// Parallel: same work split across the thread pool. The functor returns false (no entity is
		// reported as "changed"), and we reuse one changed-list to avoid per-run allocation.
		bench.batch(matched).run("ParallelForEach<Transform, Movement>  " + std::to_string(count),
			[&]
			{
				world.registry.ParallelForEach<Transform, Movement>(
					[dt](Entity, Transform& t, Movement& m) -> void
					{
						Integrate(t, m, dt);
					});
			});
	}

	RenderCsv(bench, csv);
}

} // namespace MupfelBench
