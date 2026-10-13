#include "../../Include/Core/PRNG.h"

using namespace Mupfel;

/* The default constructor seeds the engine with the standard random device. */
PRNG::PRNG()
{
    std::random_device rd;
    current_seed = rd();
    engine.seed(current_seed);
}

PRNG::PRNG(uint32_t seed) : engine(seed), current_seed(seed)
{
}

double PRNG::Normal(double mean, double standard_deviation)
{
    assert(standard_deviation > 0.0 && "given standard deviation needs to be > 0!");
    std::normal_distribution dist(mean, standard_deviation);
    return dist(engine);
}

bool PRNG::Bernoulli(double p)
{
    /* p needs to be in [0..1] */
    assert(p <= 1.0 && p >= 0.0);
    std::bernoulli_distribution dist(p);
    return dist(engine);
}
