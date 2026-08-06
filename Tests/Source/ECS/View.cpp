#include "ECS/View.h"
#include "ECS/Components/Texture.h"
#include "ECS/Components/Transform.h"
#include "Random.h"
#include "catch_amalgamated.hpp"
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <ranges>
#include <string>
#include <vector>

/* Create some test components */

struct DoubleComponent
{
	double d;
};

struct StringComponent
{
	std::string s;
};

struct IntComponent
{
	int32_t i;
};

static void						  InsertEntities(Mupfel::Registry& reg, uint32_t n, std::vector<Mupfel::Entity>& vec);
template <typename T> static void AddRndComponent(Mupfel::Registry& reg, std::vector<Mupfel::Entity>& r);
template <typename T> static void AddComponentAll(Mupfel::Registry& reg, std::vector<Mupfel::Entity>& r);

using TestView = Mupfel::View<Mupfel::Transform, Mupfel::Texture>;

static_assert(std::forward_iterator<TestView::Iterator>);
static_assert(std::sentinel_for<std::default_sentinel_t, TestView::Iterator>);
static_assert(std::ranges::forward_range<TestView>);
static_assert(std::ranges::view<TestView>);
static_assert(std::ranges::viewable_range<TestView>);

TEST_CASE("Single Component View", "[view_single]")
{
	Mupfel::EventSystem event_system;
	Mupfel::ThreadPool	thread_pool{std::thread::hardware_concurrency()};
	Mupfel::Registry	registry{event_system, thread_pool};

	std::vector<Mupfel::Entity> entities;

	/* Let's insert 50k entities for these tests. */
	InsertEntities(registry, 50, entities);

	SECTION("Normal Iteration")
	{
		AddComponentAll<DoubleComponent>(registry, entities);

		uint32_t wanted_iteration_count = entities.size();

		uint32_t actual_iteration_count = 0;
		for (auto [e, d] : registry.view<DoubleComponent>())
		{
			d.d += 1.0f;
			actual_iteration_count++;
		}

		REQUIRE(actual_iteration_count == wanted_iteration_count);
	}

	SECTION("Deleting Components Mid-Iteration")
	{
		AddComponentAll<DoubleComponent>(registry, entities);

		uint32_t wanted_iteration_count = entities.size();

		uint32_t actual_iteration_count = 0;
		for (auto [e, d] : registry.view<DoubleComponent>())
		{
			/* Lets pick one random loop iteration for now. */
			REQUIRE((actual_iteration_count + 1) == e.Index());

			if (actual_iteration_count == 5)
			{
				registry.RemoveComponent<DoubleComponent>(e);
			}

			d.d += 1.0f;
			actual_iteration_count++;
		}
	}
}

static void InsertEntities(Mupfel::Registry& reg, uint32_t n, std::vector<Mupfel::Entity>& vec)
{
	for (uint32_t i = 0; i < n; i++)
	{
		vec.push_back(reg.CreateEntity());
	}
}

template <typename T> static void AddRndComponent(Mupfel::Registry& reg, std::vector<Mupfel::Entity>& r)
{
	/* Pick two random indices for the given vector.*/
	size_t i1 = MupfelTest::Random::Index(r.size());
	size_t i2 = MupfelTest::Random::Index(r.size());

	auto subrange = std::ranges::subrange(r.begin() + std::min(i1, i2), r.begin() + std::max(i1, i2));

	for (const Mupfel::Entity& e : subrange)
	{
		reg.AddComponent<T>(e);
	}
}

template <typename T> static void AddComponentAll(Mupfel::Registry& reg, std::vector<Mupfel::Entity>& r)
{
	for (const Mupfel::Entity& e : r)
	{
		reg.AddComponent<T>(e);
	}
}
