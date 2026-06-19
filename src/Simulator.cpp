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

        startMining(node.id);
    }
}

void Simulator::startMining(NodeId minerId)
{
    Node &node = nodes[minerId];
    BlockId tip = node.longestTip;

    Block candidate;
    candidate.id = nextBlockId;
    nextBlockId = nextBlockId + 1;
    candidate.parentId = tip;
    candidate.miner = minerId;
    candidate.height = node.tree[tip].height + 1;

    Transaction coinbase;
    coinbase.id = nextTxnId;
    nextTxnId = nextTxnId + 1;
    coinbase.isCoinbase = true;
    coinbase.receiver = minerId;
    coinbase.coins = 50;
    candidate.txns.push_back(coinbase);

    std::map<NodeId, long long> balances = node.balancesAt(tip);
    std::set<TransactionId> inChain = node.txnIdsInChain(tip);

    for (const auto &entry : node.txnPool)
    {
        if (static_cast<int>(candidate.txns.size()) >= 1000)
        {
            break;
        }
        const Transaction &txn = entry.second;
        if (txn.isCoinbase || inChain.count(txn.id) > 0)
        {
            continue;
        }
        if (balances[txn.sender] >= txn.coins)
        {
            candidate.txns.push_back(txn);
            balances[txn.sender] -= txn.coins;
            balances[txn.receiver] += txn.coins;
        }
    }

    double mean = config.meanBlockInterval / node.hashPower;
    double miningTime = random.exponential(mean);

    Event event;
    event.type = EventType::MiningComplete;
    event.nodeId = minerId;
    event.time = currentTime + miningTime;
    event.block = candidate;
    event.minedOnTip = tip;
    push(event);
}

void Simulator::broadcastTransaction(NodeId origin, NodeId skip, const Transaction &txn)
{
    for (NodeId peer : network.peersOf(origin))
    {
        if (peer == skip)
        {
            continue;
        }
        long long bits = 8000;
        double delay = network.latency(nodes[origin], nodes[peer], bits, random);
        Event event;
        event.type = EventType::ReceiveTransaction;
        event.nodeId = peer;
        event.fromNode = origin;
        event.txn = txn;
        event.time = currentTime + delay;
        push(event);
    }
}

void Simulator::broadcastBlock(NodeId origin, NodeId skip, const Block &block)
{
    for (NodeId peer : network.peersOf(origin))
    {
        if (peer == skip)
        {
            continue;
        }
        double delay = network.latency(nodes[origin], nodes[peer], block.sizeInBits(), random);
        Event event;
        event.type = EventType::ReceiveBlock;
        event.nodeId = peer;
        event.fromNode = origin;
        event.block = block;
        event.time = currentTime + delay;
        push(event);
    }
}

void Simulator::handleGenerateTransaction(const Event &event)
{
    Node &node = nodes[event.nodeId];

    std::map<NodeId, long long> balances = node.balancesAt(node.longestTip);
    long long balance = balances.count(node.id) > 0 ? balances[node.id] : 0;

    if (balance >= 1 && config.numNodes >= 2)
    {
        NodeId receiver = node.id;
        while (receiver == node.id)
        {
            receiver = random.uniformInt(0, config.numNodes - 1);
        }
        long long cap = std::min<long long>(balance, 1000000000LL);
        long long amount = random.uniformInt(1, static_cast<int>(cap));

        Transaction txn;
        txn.id = nextTxnId;
        nextTxnId = nextTxnId + 1;
        txn.sender = node.id;
        txn.receiver = receiver;
        txn.coins = amount;

        node.rememberTxn(txn);
        broadcastTransaction(node.id, -1, txn);
    }

    Event next;
    next.type = EventType::GenerateTransaction;
    next.nodeId = node.id;
    next.time = currentTime + random.exponential(config.meanTransactionInterval);
    push(next);
}

void Simulator::handleReceiveTransaction(const Event &event)
{
    Node &node = nodes[event.nodeId];
    if (node.hasSeenTxn(event.txn.id))
    {
        return;
    }
    node.rememberTxn(event.txn);
    broadcastTransaction(event.nodeId, event.fromNode, event.txn);
}

void Simulator::handleReceiveBlock(const Event &event)
{
    Node &node = nodes[event.nodeId];
    Block block = event.block;

    if (node.hasSeenBlock(block.id))
    {
        return;
    }
    node.seenBlocks.insert(block.id);
    broadcastBlock(event.nodeId, event.fromNode, block);

    std::vector<Block> ready;
    ready.push_back(block);

    while (!ready.empty())
    {
        Block current = ready.back();
        ready.pop_back();

        if (node.tree.count(current.parentId) == 0)
        {
            node.orphans[current.parentId].push_back(current);
            continue;
        }
        if (!node.isValidBlock(current))
        {
            continue;
        }

        bool changed = node.addBlock(current, currentTime);
        if (changed)
        {
            startMining(node.id);
        }

        auto waiting = node.orphans.find(current.id);
        if (waiting != node.orphans.end())
        {
            for (const Block &child : waiting->second)
            {
                ready.push_back(child);
            }
            node.orphans.erase(waiting);
        }
    }
}

void Simulator::handleMiningComplete(const Event &event)
{
    Node &node = nodes[event.nodeId];

    if (node.longestTip != event.minedOnTip)
    {
        return;
    }

    Block block = event.block;
    if (!node.isValidBlock(block))
    {
        startMining(event.nodeId);
        return;
    }

    minedByNode[event.nodeId] = minedByNode[event.nodeId] + 1;
    node.addBlock(block, currentTime);
    broadcastBlock(event.nodeId, -1, block);
    startMining(event.nodeId);
}

void Simulator::run()
{
    minedByNode.assign(config.numNodes, 0);

    while (!eventQueue.empty())
    {
        Event event = eventQueue.top();
        if (event.time > config.simulationDuration)
        {
            break;
        }
        eventQueue.pop();
        currentTime = event.time;

        switch (event.type)
        {
        case EventType::GenerateTransaction:
            handleGenerateTransaction(event);
            break;
        case EventType::ReceiveTransaction:
            handleReceiveTransaction(event);
            break;
        case EventType::ReceiveBlock:
            handleReceiveBlock(event);
            break;
        case EventType::MiningComplete:
            handleMiningComplete(event);
            break;
        }
    }

    std::cout << "Done. Node 0 saw " << nodes[0].tree.size()
              << " blocks, longest chain height " << nodes[0].longestHeight << "\n";
}
