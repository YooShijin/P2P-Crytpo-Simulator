#pragma once

#include "Types.h"
#include "Transaction.h"
#include "Block.h"
#include <vector>
#include <map>
#include <set>

// A single peer: its identity, its links, its own view of the block tree, the
// transactions it knows about, and the "seen" sets that keep forwarding loop
// free. Anything to do with time or scheduling belongs to the Simulator.

class Node
{
public:
    NodeId id;
    NodeSpeed speed;
    CpuType cpu;
    double hashPower;

    std::vector<NodeId> peers;

    std::map<BlockId, Block> tree;
    std::map<BlockId, double> arrivalTime;
    BlockId longestTip;
    int longestHeight;

    std::map<TransactionId, Transaction> txnPool;
    std::set<TransactionId> seenTxns;
    std::set<BlockId> seenBlocks;

    // Blocks that arrived before their parent, keyed by the missing parent id.
    std::map<BlockId, std::vector<Block>> orphans;

    Node(NodeId id, NodeSpeed speed, CpuType cpu);

    bool isFast() const;
    bool isHighCpu() const;

    void addGenesis(const Block &genesis, double time);

    bool hasSeenTxn(TransactionId txnId) const;
    void rememberTxn(const Transaction &txn);

    bool hasSeenBlock(BlockId blockId) const;

    // Balances after replaying the chain that ends at 'tip'.
    std::map<NodeId, long long> balancesAt(BlockId tip) const;

    // Transactions already in the chain ending at 'tip', so we do not mine the
    // same one twice.
    std::set<TransactionId> txnIdsInChain(BlockId tip) const;

    // Parent must exist and no balance may go negative.
    bool isValidBlock(const Block &block) const;

    // Adds a validated block. Returns true if the longest chain moved.
    bool addBlock(const Block &block, double time);
};
