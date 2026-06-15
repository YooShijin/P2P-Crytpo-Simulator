#include <iostream>
#include <string>

#include "Config.h"
#include "Simulator.h"

// Usage: ./simulator <z0_slow_%> <z1_lowcpu_%> [numNodes] [Ttx] [I] [duration] [seed]
// Only the two percentages are required, everything else falls back to a default.

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cout << "Usage: " << argv[0]
                  << " <z0_slow_%> <z1_lowcpu_%> [numNodes] [Ttx] [I] [duration] [seed]\n";
        std::cout << "Example: " << argv[0] << " 40 50\n";
        return 1;
    }

    Config config;
    config.slowPercentage = std::stod(argv[1]);
    config.lowCpuPercentage = std::stod(argv[2]);

    config.numNodes = (argc > 3) ? std::stoi(argv[3]) : 15;
    config.meanTransactionInterval = (argc > 4) ? std::stod(argv[4]) : 5.0;
    config.meanBlockInterval = (argc > 5) ? std::stod(argv[5]) : 60.0;
    config.simulationDuration = (argc > 6) ? std::stod(argv[6]) : 2000.0;
    config.randomSeed = (argc > 7) ? static_cast<unsigned int>(std::stoul(argv[7])) : 42;

    if (config.numNodes < 4)
    {
        std::cout << "numNodes must be at least 4 so every peer can have 3 to 6 links.\n";
        return 1;
    }

    std::cout << "Starting P2P cryptocurrency simulation...\n";

    Simulator simulator(config);
    simulator.run();

    return 0;
}
