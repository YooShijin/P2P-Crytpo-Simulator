#include "Random.h"

Random::Random(unsigned int seed)
    : engine(seed)
{
}

double Random::exponential(double mean)
{
    if (mean <= 0.0)
    {
        return 0.0;
    }
    std::exponential_distribution<double> dist(1.0 / mean);
    return dist(engine);
}

double Random::uniformReal(double low, double high)
{
    std::uniform_real_distribution<double> dist(low, high);
    return dist(engine);
}

int Random::uniformInt(int low, int high)
{
    std::uniform_int_distribution<int> dist(low, high);
    return dist(engine);
}
