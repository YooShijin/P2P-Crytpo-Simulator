#pragma once

#include "Types.h"
#include <string>

// One payment, or the 50 coin mining reward (isCoinbase, no sender).
// Every transaction counts as 1 KB.

struct Transaction
{
    TransactionId id = -1;
    NodeId sender = -1;
    NodeId receiver = -1;
    long long coins = 0;
    bool isCoinbase = false;

    std::string describe() const
    {
        if (isCoinbase)
        {
            return "Txn" + std::to_string(id) + ": " + std::to_string(receiver) + " mines 50 coins";
        }
        return "Txn" + std::to_string(id) + ": " + std::to_string(sender) + " pays " + std::to_string(receiver) + " " + std::to_string(coins) + " coins";
    }
};
