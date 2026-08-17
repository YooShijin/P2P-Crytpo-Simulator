#include "Simulator.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <vector>
#include <filesystem>

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

    writeTreeFiles();
    printSummary();
}

void Simulator::writeTreeFiles() const
{
    std::filesystem::create_directories("output");

    for (const Node &node : nodes)
    {
        std::set<BlockId> longestChain;
        BlockId walk = node.longestTip;
        while (walk != -1 && node.tree.count(walk) > 0)
        {
            longestChain.insert(walk);
            walk = node.tree.at(walk).parentId;
        }

        std::string filename = "output/node_" + std::to_string(node.id) + "_tree.txt";
        std::ofstream file(filename);
        file << "# Blockchain tree seen by node " << node.id << "\n";
        file << "# columns: blockId parentId miner height arrivalTime numTxns sizeKB inLongestChain\n";

        for (const auto &entry : node.tree)
        {
            const Block &block = entry.second;
            int inChain = longestChain.count(block.id) > 0 ? 1 : 0;
            file << block.id << " "
                 << block.parentId << " "
                 << block.miner << " "
                 << block.height << " "
                 << node.arrivalTime.at(block.id) << " "
                 << block.txns.size() << " "
                 << block.sizeInKB() << " "
                 << inChain << "\n";
        }
        file.close();
    }

    const Node &sample = nodes[0];
    std::ofstream dot("output/node_0_tree.dot");
    dot << "digraph blockchain {\n";
    dot << "  rankdir=LR;\n";
    dot << "  node [shape=box, style=rounded];\n";
    BlockId tip = sample.longestTip;
    std::set<BlockId> longestChain;
    while (tip != -1 && sample.tree.count(tip) > 0)
    {
        longestChain.insert(tip);
        tip = sample.tree.at(tip).parentId;
    }
    for (const auto &entry : sample.tree)
    {
        const Block &block = entry.second;
        std::string color = longestChain.count(block.id) > 0 ? "lightgreen" : "lightgrey";
        dot << "  b" << block.id << " [label=\"Blk " << block.id
            << "\\nminer " << block.miner << "\", style=\"rounded,filled\", fillcolor=" << color << "];\n";
        if (block.parentId != -1)
        {
            dot << "  b" << block.parentId << " -> b" << block.id << ";\n";
        }
    }
    dot << "}\n";
    dot.close();
}

void Simulator::printSummary() const
{
    const Node &reference = nodes[0];

    std::set<BlockId> longestChain;
    BlockId walk = reference.longestTip;
    while (walk != -1 && reference.tree.count(walk) > 0)
    {
        longestChain.insert(walk);
        walk = reference.tree.at(walk).parentId;
    }

    std::vector<int> inChainByNode(config.numNodes, 0);
    for (BlockId id : longestChain)
    {
        const Block &block = reference.tree.at(id);
        if (block.miner >= 0)
        {
            inChainByNode[block.miner] = inChainByNode[block.miner] + 1;
        }
    }

    std::cout << "\n=============== SIMULATION SUMMARY ===============\n";
    std::cout << "Nodes: " << config.numNodes
              << "   Slow%: " << config.slowPercentage
              << "   LowCPU%: " << config.lowCpuPercentage << "\n";
    std::cout << "Ttx: " << config.meanTransactionInterval
              << "s   BlockInterval I: " << config.meanBlockInterval
              << "s   Duration: " << config.simulationDuration << "s\n";
    std::cout << "Total blocks in tree (node 0): " << reference.tree.size() << "\n";
    std::cout << "Longest chain length (node 0): " << reference.longestHeight << "\n\n";

    std::cout << "Per node (from node 0's view of the longest chain):\n";
    std::cout << std::left
              << std::setw(5) << "id"
              << std::setw(7) << "speed"
              << std::setw(6) << "cpu"
              << std::right
              << std::setw(10) << "hashPower"
              << std::setw(8) << "mined"
              << std::setw(9) << "inChain"
              << std::setw(8) << "ratio" << "\n";
    for (const Node &node : nodes)
    {
        int mined = minedByNode[node.id];
        int inChain = inChainByNode[node.id];
        double ratio = mined > 0 ? static_cast<double>(inChain) / mined : 0.0;
        std::cout << std::left
                  << std::setw(5) << node.id
                  << std::setw(7) << (node.isFast() ? "Fast" : "Slow")
                  << std::setw(6) << (node.isHighCpu() ? "High" : "Low")
                  << std::right << std::fixed
                  << std::setw(10) << std::setprecision(4) << node.hashPower
                  << std::setw(8) << mined
                  << std::setw(9) << inChain
                  << std::setw(8) << std::setprecision(2) << ratio << "\n";
    }

    int offChain = static_cast<int>(reference.tree.size()) - static_cast<int>(longestChain.size());
    std::cout << "\nBlocks off the longest chain (forks) at node 0: " << offChain << "\n";
    std::cout << "Tree files written to the output/ folder.\n";
    std::cout << "Visualize node 0 with:  dot -Tpng output/node_0_tree.dot -o tree.png\n";
    std::cout << "=================================================\n";
}
