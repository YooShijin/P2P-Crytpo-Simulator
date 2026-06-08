#pragma once

// Every knob for one run. main.cpp fills this from argv.

struct Config
{
    int numNodes;              // n : number of peers
    double slowPercentage;     // z0 : percent of peers that are Slow
    double lowCpuPercentage;   // z1 : percent of peers that are Low CPU

    double meanTransactionInterval; // Ttx : mean gap between a peer's transactions (seconds)
    double meanBlockInterval;       // I : average block interarrival time (seconds)

    double simulationDuration; // how long to run the event loop (seconds)

    unsigned int randomSeed;   // seed so runs are reproducible
};
