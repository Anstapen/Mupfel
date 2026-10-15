#pragma once
#include <cassert>
#include <cstdint>
#include <limits>
#include <random>
#include <algorithm>
#include <concepts>
#include <iterator>

namespace Mupfel
{
    template <typename T>
    concept RandomIntegerType =
        std::same_as<T, short> || std::same_as<T, int> ||
        std::same_as<T, long> || std::same_as<T, long long> ||
        std::same_as<T, unsigned short> || std::same_as<T, unsigned int> ||
        std::same_as<T, unsigned long> || std::same_as<T, unsigned long long>;

    template <typename T>
    concept RandomRealType =
        std::same_as<T, float> || std::same_as<T, double> || std::same_as<T, long double>;


    class PRNG
    {
    public:
        PRNG();
        explicit PRNG(uint32_t seed);

        template <RandomIntegerType IntegerType>
        IntegerType
        UniformInt(IntegerType min = std::numeric_limits<IntegerType>::min(),
                   IntegerType max = std::numeric_limits<IntegerType>::max());

        template <RandomRealType RealType>
        RealType UniformReal(RealType min = RealType(0),
                             RealType max = RealType(1));

        double Normal(double mean, double standard_deviation = 1.0);

        template <RandomIntegerType IntegerType>
        IntegerType BinomialInt(IntegerType num_trials, double p = 0.5);

        template <RandomIntegerType IntegerType>
        IntegerType DiscreteInt(std::initializer_list<double> weights);

        bool Bernoulli(double p);

    private:
        std::mt19937 engine;
        uint32_t current_seed;
    };

    template <RandomIntegerType IntegerType>
    inline IntegerType PRNG::UniformInt(IntegerType min, IntegerType max)
    {
        assert(min <= max);
        std::uniform_int_distribution<IntegerType> dist(min, max);
        return dist(engine);
    }

    template <RandomRealType RealType>
    inline RealType PRNG::UniformReal(RealType min, RealType max)
    {
        assert(min < max);
        std::uniform_real_distribution<RealType> dist(min, max);
        return dist(engine);
    }

    template <RandomIntegerType IntegerType>
    inline IntegerType PRNG::BinomialInt(IntegerType num_trials, double p)
    {
        /* p needs to be in [0..1] */
        assert(p <= 1.0 && p >= 0.0);
        assert(num_trials >= 0);
        std::binomial_distribution dist(num_trials, p);
        return dist(engine);
    }

    template <RandomIntegerType IntegerType>
    inline IntegerType PRNG::DiscreteInt(std::initializer_list<double> weights)
    {
        assert(std::ranges::all_of(weights, [](double w) { return w >= 0.0; }));
        assert(std::ranges::any_of(weights, [](double w) { return w > 0.0; }) || std::empty(weights));
        std::discrete_distribution<IntegerType> dist(weights);
        return dist(engine);
    }
}
