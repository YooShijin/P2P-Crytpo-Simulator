#pragma once

#include <random>

// Wrapper over <random>. One instance is shared by the whole run, so a single
// seed reproduces the run exactly.

class Random
{
private:
    std::mt19937 engine;

public:
    explicit Random(unsigned int seed);

    double exponential(double mean);
    double uniformReal(double low, double high);

    // Both ends included.
    int uniformInt(int low, int high);
};
