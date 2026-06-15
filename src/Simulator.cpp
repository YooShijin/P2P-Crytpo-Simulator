#include "Simulator.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <vector>

Simulator::Simulator(const Config &config)
    : config(config),
      random(config.randomSeed),
      network(config.numNodes, random),
      currentTime(0.0),
      nextTxnId(0),
      nextBlockId(0)
{
    for (NodeId i = 0; i < config.numNodes; i = i + 1)
    {
        nodes.emplace_back(i, NodeSpeed::Fast, CpuType::High);
    }

    for (NodeId i = 0; i < config.numNodes; i = i + 1)
    {
        nodes[i].peers = network.peersOf(i);
    }

    assignRolesAndHashPower();

    genesis.id = nextBlockId;
    nextBlockId = nextBlockId + 1;
    genesis.parentId = -1;
    genesis.miner = -1;
    genesis.height = 0;
    for (Node &node : nodes)
    {
        node.addGenesis(genesis, 0.0);
    }

    scheduleInitialEvents();
}

void Simulator::assignRolesAndHashPower()
{
    int n = config.numNodes;

    std::vector<NodeId> order(n);
    for (int i = 0; i < n; i = i + 1)
    {
        order[i] = i;
    }
    for (int i = n - 1; i > 0; i = i - 1)
    {
        int j = random.uniformInt(0, i);
        std::swap(order[i], order[j]);
    }
    int numSlow = static_cast<int>(std::round(n * config.slowPercentage / 100.0));
    for (int i = 0; i < numSlow; i = i + 1)
    {
        nodes[order[i]].speed = NodeSpeed::Slow;
    }

    for (int i = n - 1; i > 0; i = i - 1)
    {
        int j = random.uniformInt(0, i);
        std::swap(order[i], order[j]);
    }
    int numLow = static_cast<int>(std::round(n * config.lowCpuPercentage / 100.0));
    for (int i = 0; i < numLow; i = i + 1)
    {
        nodes[order[i]].cpu = CpuType::Low;
    }

    double totalWeight = 0.0;
    for (const Node &node : nodes)
    {
        totalWeight += node.isHighCpu() ? 10.0 : 1.0;
    }
    for (Node &node : nodes)
    {
        double weight = node.isHighCpu() ? 10.0 : 1.0;
        node.hashPower = weight / totalWeight;
    }
}

void Simulator::push(const Event &event)
{
    eventQueue.push(event);
}

void Simulator::scheduleInitialEvents()
{
    for (Node &node : nodes)
    {
        Event txnEvent;
        txnEvent.type = EventType::GenerateTransaction;
        txnEvent.nodeId = node.id;
        txnEvent.time = random.exponential(config.meanTransactionInterval);
        push(txnEvent);
    }
}

// Nothing is created yet, this only keeps the queue alive so the loop can be
// tested on its own.
void Simulator::handleGenerateTransaction(const Event &event)
{
    Event next;
    next.type = EventType::GenerateTransaction;
    next.nodeId = event.nodeId;
    next.time = currentTime + random.exponential(config.meanTransactionInterval);
    push(next);
}

void Simulator::run()
{
    long long handled = 0;

    while (!eventQueue.empty())
    {
        Event event = eventQueue.top();
        if (event.time > config.simulationDuration)
        {
            break;
        }
        eventQueue.pop();
        currentTime = event.time;
        handled = handled + 1;

        switch (event.type)
        {
        case EventType::GenerateTransaction:
            handleGenerateTransaction(event);
            break;
        default:
            break;
        }
    }

    std::cout << "Handled " << handled << " events, clock stopped at "
              << currentTime << "s\n";
}
