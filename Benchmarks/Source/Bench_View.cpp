// View iteration benchmarks -- the core of what the ECS is asked to do every frame.
//
// Three angles:
//   1. Scaling      -- a single-component view over 1k / 10k / 50k entities (ns/entity should be flat).
//   2. Match density -- a single-component view where 100% / 50% / 10% of entities match (measures the
//                       cost of scanning + signature-filtering entities that get skipped).
//   3. Base ordering -- the same two-component query written both ways, to show why the *rarest*
//                       component should come first (View iterates the first type's dense array).

#include "BenchCommon.h"
#include "Benchmarks.h"

#include <string>

using namespace Mupfel;
using ankerl::nanobench::doNotOptimizeAway;

namespace MupfelBench {

void RunViewBenchmarks(std::ostream* csv)
{
	// ---- 1. Scaling: single-component view over growing entity counts ----
	{
		ankerl::nanobench::Bench bench;
		ApplyDefaults(bench).title("View<Transform> iteration (scaling)").unit("entity");

		for (uint32_t count : { 1000u, 10000u, 50000u })
		{
			World world;
			Populate(world, count, 1); // every entity has a Transform

			bench.batch(count).run("view<Transform>  " + std::to_string(count) + " entities",
				[&]
				{
					float sum = 0.0f;
					for (auto [e, t] : world.registry.view<Transform>())
						sum += t.pos_x;
					doNotOptimizeAway(sum);
				});
		}

		RenderCsv(bench, csv);
	}

	// ---- 2. Match density: only a fraction of the 50k entities match the query ----
	{
		constexpr uint32_t count = 50000;

		ankerl::nanobench::Bench bench;
		ApplyDefaults(bench).title("View<Movement> iteration (50k entities, varying match density)").unit("match");

		struct Case
		{
			const char* label;
			uint32_t	stride;
		};

		for (const Case& c : { Case{ "100% match", 1u }, Case{ "50% match", 2u }, Case{ "10% match", 10u } })
		{
			World world;
			Populate(world, count, c.stride);
			const uint32_t matched = MovementMatches(count, c.stride);

			bench.batch(matched).run(c.label,
				[&]
				{
					float sum = 0.0f;
					for (auto [e, m] : world.registry.view<Movement>())
						sum += m.velocity_x;
					doNotOptimizeAway(sum);
				});
		}

		RenderCsv(bench, csv);
	}

	// ---- 3. Base-component ordering: rarest component should drive iteration ----
	{
		constexpr uint32_t count = 50000;
		World			   world;
		Populate(world, count, 2); // all have Transform; half also have Movement
		const uint32_t matched = MovementMatches(count, 2);

		ankerl::nanobench::Bench bench;
		ApplyDefaults(bench).title("Two-component view: base-component ordering (50k, half match)").unit("match").relative(true);

		// base = Transform (common): scans all 50k, signature-filters down to the 25k matches.
		bench.batch(matched).run("view<Transform, Movement>  (base = common)",
			[&]
			{
				float sum = 0.0f;
				for (auto [e, t, m] : world.registry.view<Transform, Movement>())
					sum += t.pos_x + m.velocity_x;
				doNotOptimizeAway(sum);
			});

		// base = Movement (rare): scans only the 25k that already match -- expected to be faster.
		bench.batch(matched).run("view<Movement, Transform>  (base = rare)",
			[&]
			{
				float sum = 0.0f;
				for (auto [e, m, t] : world.registry.view<Movement, Transform>())
					sum += t.pos_x + m.velocity_x;
				doNotOptimizeAway(sum);
			});

		RenderCsv(bench, csv);
	}
}

} // namespace MupfelBench
