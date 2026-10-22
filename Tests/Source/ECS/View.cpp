#include "ECS/View.h"
#include "ECS/Components/Texture.h"
#include "ECS/Components/Transform.h"
#include "catch_amalgamated.hpp"
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <ranges>
#include <string>
#include <vector>

#include "Core/PRNG.h"

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

using TestView = Mupfel::View<Mupfel::Transform, Mupfel::Texture>;

static_assert(std::forward_iterator<TestView::Iterator>);
static_assert(std::sentinel_for<std::default_sentinel_t, TestView::Iterator>);
static_assert(std::ranges::forward_range<TestView>);
static_assert(std::ranges::view<TestView>);
static_assert(std::ranges::viewable_range<TestView>);

TEST_CASE("Single Component View", "[view_single]")
{
}