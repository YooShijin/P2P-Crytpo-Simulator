#pragma once

#include "Config.h"
#include "Types.h"
#include "Node.h"
#include "Network.h"
#include "Event.h"
#include "Random.h"
#include <vector>
#include <queue>
#include <string>

// The engine. Holds the event queue, the nodes and the network, hands out
// transaction and block ids, and at the end writes the tree files and the
// summary.

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

    // Blocks each node mined and broadcast.
    std::vector<int> minedByNode;

    void assignRolesAndHashPower();
    void scheduleInitialEvents();
    void push(const Event &event);

    void handleGenerateTransaction(const Event &event);
    void handleReceiveTransaction(const Event &event);
    void handleReceiveBlock(const Event &event);
    void handleMiningComplete(const Event &event);

    void broadcastTransaction(NodeId origin, NodeId skip, const Transaction &txn);
    void broadcastBlock(NodeId origin, NodeId skip, const Block &block);
    void startMining(NodeId minerId);

    void writeTreeFiles() const;
    void printSummary() const;

public:
    explicit Simulator(const Config &config);

    void run();
};
