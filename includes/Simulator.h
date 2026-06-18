#pragma once

#include "Config.h"
#include "Types.h"
#include "Node.h"
#include "Network.h"
#include "Event.h"
#include "Random.h"
#include <vector>
#include <queue>

// The engine. Holds the event queue, the nodes and the network, and hands out
// transaction and block ids.

class Simulator
{
private:
    Config config;
    Random random;
    std::vector<Node> nodes;
    Network network;

    std::priority_queue<Event, std::vector<Event>, EventCompare> eventQueue;
    double currentTime;

    int nextTxnId;
    int nextBlockId;
    Block genesis;

    void assignRolesAndHashPower();
    void scheduleInitialEvents();
    void push(const Event &event);

    void handleGenerateTransaction(const Event &event);
    void handleReceiveTransaction(const Event &event);

    void broadcastTransaction(NodeId origin, NodeId skip, const Transaction &txn);

public:
    explicit Simulator(const Config &config);

    void run();
};
