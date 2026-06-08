#pragma once

// Ids and enums shared by every module.

using NodeId = int;
using TransactionId = int;
using BlockId = int;

enum class EventType
{
    GenerateTransaction,
    ReceiveTransaction,
    ReceiveBlock,
    MiningComplete
};

enum class NodeSpeed
{
    Fast,
    Slow
};

enum class CpuType
{
    High,
    Low
};
