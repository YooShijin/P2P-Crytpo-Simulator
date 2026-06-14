#pragma once

#include "Types.h"
#include "Node.h"
#include "Random.h"
#include <vector>
#include <map>
#include <utility>

// The graph under the nodes: a random connected topology where every peer has
// 3 to 6 links, a fixed propagation delay per link, and
//
//     latency = rho + (message bits / link speed) + queuing delay
//
// Link speed is 100 Mbps if both ends are Fast, else 5 Mbps.

class Network
{
private:
    int numNodes;
    std::vector<std::vector<NodeId>> adjacency;
    std::map<std::pair<NodeId, NodeId>, double> rho;

    std::pair<NodeId, NodeId> linkKey(NodeId a, NodeId b) const;
    bool isConnected() const;

public:
    Network(int numNodes, Random &random);

    // Keeps rebuilding until the topology is valid.
    void build(Random &random);

    const std::vector<NodeId> &peersOf(NodeId node) const;

    // c_ij in bits per second.
    double linkSpeed(const Node &a, const Node &b) const;

    double latency(const Node &from, const Node &to, long long messageBits, Random &random) const;
};
