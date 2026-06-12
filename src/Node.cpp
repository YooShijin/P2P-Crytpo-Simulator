#include "Node.h"
#include <vector>

Node::Node(NodeId id, NodeSpeed speed, CpuType cpu)
    : id(id), speed(speed), cpu(cpu), hashPower(0.0),
      longestTip(-1), longestHeight(-1)
{
}

bool Node::isFast() const
{
    return speed == NodeSpeed::Fast;
}

bool Node::isHighCpu() const
{
    return cpu == CpuType::High;
}

void Node::addGenesis(const Block &genesis, double time)
{
    tree[genesis.id] = genesis;
    arrivalTime[genesis.id] = time;
    seenBlocks.insert(genesis.id);
    longestTip = genesis.id;
    longestHeight = genesis.height;
}

bool Node::hasSeenTxn(TransactionId txnId) const
{
    return seenTxns.count(txnId) > 0;
}

void Node::rememberTxn(const Transaction &txn)
{
    seenTxns.insert(txn.id);
    txnPool[txn.id] = txn;
}

bool Node::hasSeenBlock(BlockId blockId) const
{
    return seenBlocks.count(blockId) > 0;
}

std::map<NodeId, long long> Node::balancesAt(BlockId tip) const
{
    std::vector<BlockId> chain;
    BlockId current = tip;
    while (current != -1 && tree.count(current) > 0)
    {
        chain.push_back(current);
        current = tree.at(current).parentId;
    }

    std::map<NodeId, long long> balances;
    for (int i = static_cast<int>(chain.size()) - 1; i >= 0; i = i - 1)
    {
        const Block &block = tree.at(chain[i]);
        for (const Transaction &txn : block.txns)
        {
            if (txn.isCoinbase)
            {
                balances[txn.receiver] += txn.coins;
            }
            else
            {
                balances[txn.sender] -= txn.coins;
                balances[txn.receiver] += txn.coins;
            }
        }
    }
    return balances;
}

std::set<TransactionId> Node::txnIdsInChain(BlockId tip) const
{
    std::set<TransactionId> ids;
    BlockId current = tip;
    while (current != -1 && tree.count(current) > 0)
    {
        const Block &block = tree.at(current);
        for (const Transaction &txn : block.txns)
        {
            ids.insert(txn.id);
        }
        current = block.parentId;
    }
    return ids;
}

bool Node::isValidBlock(const Block &block) const
{
    if (tree.count(block.parentId) == 0)
    {
        return false;
    }

    std::map<NodeId, long long> balances = balancesAt(block.parentId);
    for (const Transaction &txn : block.txns)
    {
        if (txn.isCoinbase)
        {
            balances[txn.receiver] += txn.coins;
        }
        else
        {
            if (balances[txn.sender] < txn.coins)
            {
                return false;
            }
            balances[txn.sender] -= txn.coins;
            balances[txn.receiver] += txn.coins;
        }
    }
    return true;
}

bool Node::addBlock(const Block &block, double time)
{
    tree[block.id] = block;
    arrivalTime[block.id] = time;
    seenBlocks.insert(block.id);

    if (block.height > longestHeight)
    {
        longestHeight = block.height;
        longestTip = block.id;
        return true;
    }
    return false;
}
