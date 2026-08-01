#include "ECS/View.h"
#include "ECS/Components/Texture.h"
#include "ECS/Components/Transform.h"
#include <iterator>
#include <ranges>

using TestView = Mupfel::View<Mupfel::Transform, Mupfel::Texture>;

static_assert(std::forward_iterator<TestView::Iterator>);
static_assert(std::sentinel_for<std::default_sentinel_t, TestView::Iterator>);
static_assert(std::ranges::forward_range<TestView>);
static_assert(std::ranges::view<TestView>);
static_assert(std::ranges::viewable_range<TestView>);