// Component-access benchmarks: the cost of the sparse-set indirection for a single random or
// sequential lookup, isolated from view iteration.
//
//   sequential -- lookups follow creation order, so the dense array is touched roughly in order.
//   random     -- lookups follow a shuffled entity list, defeating prefetching; this is the cost you
//                 pay when touching entities by handle (e.g. from a collision pair) rather than iterating.

#include "BenchCommon.h"
#include "Benchmarks.h"

using namespace Mupfel;
using ankerl::nanobench::doNotOptimizeAway;

namespace MupfelBench {

void RunComponentAccessBenchmarks(std::ostream* csv)
{
	constexpr uint32_t count = 50000;

	World world;
	Populate(world, count, 2); // all have Transform; half also have Movement
	const std::vector<Entity> shuffled = ShuffledEntities(world);

	ankerl::nanobench::Bench bench;
	ApplyDefaults(bench).title("Component access (50k entities)").unit("op");

	// GetComponent<Transform> walked in creation order (cache-friendly best case).
	bench.batch(count).run("GetComponent<Transform>  sequential",
		[&]
		{
			float sum = 0.0f;
			for (Entity e : world.entities)
				sum += world.registry.GetComponent<Transform>(e).pos_x;
			doNotOptimizeAway(sum);
		});

	// GetComponent<Transform> in random handle order (cache-unfriendly).
	bench.batch(count).run("GetComponent<Transform>  random",
		[&]
		{
			float sum = 0.0f;
			for (Entity e : shuffled)
				sum += world.registry.GetComponent<Transform>(e).pos_x;
			doNotOptimizeAway(sum);
		});

	// HasComponent<Movement> in random order (present on half the entities).
	bench.batch(count).run("HasComponent<Movement>  random",
		[&]
		{
			uint32_t hits = 0;
			for (Entity e : shuffled)
				hits += world.registry.HasComponent<Movement>(e) ? 1u : 0u;
			doNotOptimizeAway(hits);
		});

	RenderCsv(bench, csv);
}

} // namespace MupfelBench
