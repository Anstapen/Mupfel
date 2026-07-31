#pragma once

/*
	Random number generation for the test suite.

	Header-only: the generator lives in a thread_local inside an inline function, so every
	translation unit shares one generator per thread. Nothing here allocates or locks.

	Reproducing a failure: every helper draws from the calling thread's generator, so a test that
	needs a repeatable sequence calls Seed() first. Catch2 prints the seed it randomised with on
	every run ("Randomness seeded to: ..."), which only governs *test ordering* -- it does not reach
	the generator below, so pass Catch2's seed to Seed() explicitly if you want the two tied
	together.

	@warning The std distributions are not portable - MSVC and libstdc++ return different values
	from the same seed. The *generators* are specified exactly (mt19937 emits identical bits
	everywhere), so a test asserting on specific drawn values will pass on one platform and fail on
	another. Assert on properties (range, distribution, invariants) rather than on literal draws.
*/

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <random>
#include <ranges>
#include <thread>
#include <type_traits>

namespace MupfelTest::Random
{
/** The generator every helper in this namespace draws from. */
using Engine = std::mt19937;

/**
 * The calling thread's generator.
 *
 * One generator per thread: they are stateful and hold no internal lock, so sharing one across a
 * test that exercises the engine's thread pool would be a data race.
 */
inline Engine& LocalEngine()
{
	thread_local Engine engine = []
	{
		/* random_device yields 32 bits per call but mt19937 has 19937 bits of state, so seed
		   through a seed_seq rather than a single draw - otherwise only 2^32 streams are
		   reachable. The thread id is mixed in because random_device is permitted to be
		   deterministic (and is, on some MinGW builds); without it every worker would then
		   produce an identical sequence. */
		std::random_device dev;
		const auto tid = static_cast<std::uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));

		std::seed_seq seq{static_cast<std::uint32_t>(dev()), static_cast<std::uint32_t>(dev()),
						  static_cast<std::uint32_t>(dev()), static_cast<std::uint32_t>(dev()), tid};
		return Engine(seq);
	}();

	return engine;
}

/** Pins the calling thread's stream to a known sequence. Affects only this thread. */
inline void Seed(Engine::result_type seed) { LocalEngine().seed(seed); }

/** A value in [min, max) - `max` itself is reachable only through rounding. */
template <std::floating_point T> inline T Range(T min, T max)
{
	assert(min <= max && "Random::Range called with an inverted range");
	return std::uniform_real_distribution<T>(min, max)(LocalEngine());
}

/** A value in [min, max] - both ends inclusive, unlike the floating-point overload. */
template <std::integral T> inline T Range(T min, T max)
{
	assert(min <= max && "Random::Range called with an inverted range");

	/* uniform_int_distribution is only defined for short/int/long/long long and their unsigned
	   forms. char, int8_t, uint8_t and bool are undefined behaviour, so widen, draw, narrow back. */
	using Wide = std::conditional_t<std::is_signed_v<T>, long long, unsigned long long>;

	return static_cast<T>(std::uniform_int_distribution<Wide>(
		static_cast<Wide>(min), static_cast<Wide>(max))(LocalEngine()));
}

/** A value in [0, 1). */
template <std::floating_point T = float> inline T Unit()
{
	return std::uniform_real_distribution<T>(T(0), T(1))(LocalEngine());
}

/** True with the given probability, in [0, 1]. */
inline bool Chance(double probability) { return std::bernoulli_distribution(probability)(LocalEngine()); }

/** An index in [0, count) - the form to use when picking an element. */
inline std::size_t Index(std::size_t count)
{
	assert(count > 0 && "Random::Index needs a non-empty range");
	return std::uniform_int_distribution<std::size_t>(0, count - 1)(LocalEngine());
}

/** A reference to a random element of `range`. */
template <std::ranges::random_access_range R> inline decltype(auto) Pick(R&& range)
{
	assert(!std::ranges::empty(range) && "Random::Pick on an empty range");
	return *(std::ranges::begin(range) +
			 static_cast<std::ranges::range_difference_t<R>>(Index(std::ranges::size(range))));
}

/** Shuffles `range` in place. */
template <std::ranges::random_access_range R> inline void Shuffle(R&& range)
{
	std::ranges::shuffle(range, LocalEngine());
}
} // namespace MupfelTest::Random
