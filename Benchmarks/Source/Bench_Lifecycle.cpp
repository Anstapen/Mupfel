// Entity / component lifecycle benchmarks: the mutation paths (create, add, remove, destroy).
//
// These are measured as balanced round-trips so the registry returns to (roughly) its starting state
// after each timed invocation and steady-state numbers are stable:
//   * create N then destroy N,
//   * add a component to N entities then remove it from all N.
//
// Each timed invocation ends with a single events.Update(): CreateEntity / AddComponent /
// DestroyEntity each fire an immediate event, and without a per-"frame" drain those buffers would
// grow across every epoch and pollute both timing and memory. One Update() bounds them to two
// invocations' worth, matching how a real frame drains its events.

#include "BenchCommon.h"
#include "Benchmarks.h"

#include <vector>

using namespace Mupfel;

namespace MupfelBench {

void RunLifecycleBenchmarks(std::ostream* csv)
{
	constexpr uint32_t count = 10000;

	ankerl::nanobench::Bench bench;
	ApplyDefaults(bench).title("Entity / component lifecycle").unit("entity");

	// ---- Create + Destroy round-trip (bare entities, no components) ----
	{
		World				world;
		std::vector<Entity> es;
		es.reserve(count);

		bench.batch(count).run("CreateEntity + DestroyEntity",
			[&]
			{
				es.clear();
				for (uint32_t i = 0; i < count; ++i)
					es.push_back(world.registry.CreateEntity());
				for (Entity e : es)
					world.registry.DestroyEntity(e);
				world.events.Update();
			});
	}

	// ---- Create + AddComponent<Transform> + Destroy round-trip ----
	{
		World				world;
		std::vector<Entity> es;
		es.reserve(count);
		Transform			t;
		t.pos_x = 1.0f;

		bench.batch(count).run("CreateEntity + AddComponent<Transform> + DestroyEntity",
			[&]
			{
				es.clear();
				for (uint32_t i = 0; i < count; ++i)
				{
					Entity e = world.registry.CreateEntity();
					world.registry.AddComponent<Transform>(e, t);
					es.push_back(e);
				}
				for (Entity e : es)
					world.registry.DestroyEntity(e);
				world.events.Update();
			});
	}

	// ---- AddComponent / RemoveComponent pair on existing entities ----
	// Isolates the sparse-set insert + O(1) swap-remove cost from entity creation/destruction.
	{
		World world;
		Populate(world, count, 0); // all entities have Transform only; none have Movement yet
		Movement m;                // Movement isn't an aggregate (private padding member), so assign fields
		m.velocity_x = 1.0f;

		bench.batch(count).run("AddComponent<Movement> + RemoveComponent<Movement>",
			[&]
			{
				for (Entity e : world.entities)
					world.registry.AddComponent<Movement>(e, m);
				for (Entity e : world.entities)
					world.registry.RemoveComponent<Movement>(e);
				world.events.Update();
			});
	}

	RenderCsv(bench, csv);
}

} // namespace MupfelBench
