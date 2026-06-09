#pragma once

#include "Types.h"
#include "Transaction.h"
#include "Block.h"

// One item in the event queue. Which fields matter depends on 'type'.

struct Event
{
    double time = 0.0;
    EventType type = EventType::GenerateTransaction;

    NodeId nodeId = -1;   // node where this event happens
    NodeId fromNode = -1; // for receive events: which peer sent it

    Transaction txn;      // transaction events
    Block block;          // block / mining events
    BlockId minedOnTip = -1; // MiningComplete: the tip we were mining on
};

// priority_queue is a max heap, so comparing with > gives us the earliest
// event on top.
struct EventCompare
{
    bool operator()(const Event &a, const Event &b) const
    {
        return a.time > b.time;
    }
};
