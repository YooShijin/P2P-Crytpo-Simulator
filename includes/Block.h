#pragma once

#include "Types.h"
#include "Transaction.h"
#include <vector>

// A block in the tree. Points at its parent by id, and height is the distance
// from genesis, which is what decides the longest chain.
//
// Size rule from the assignment: an empty block (coinbase only) is 1 KB and
// each extra transaction adds 1 KB, so size in KB == number of transactions.

struct Block
{
    BlockId id = -1;
    BlockId parentId = -1;
    NodeId miner = -1;
    int height = 0;
    std::vector<Transaction> txns;

    int sizeInKB() const
    {
        int kb = static_cast<int>(txns.size());
        if (kb < 1)
        {
            kb = 1;
        }
        return kb;
    }

    long long sizeInBits() const
    {
        return static_cast<long long>(sizeInKB()) * 8000LL;
    }
};
