#include "Network.h"
#include <algorithm>
#include <queue>
#include <set>

Network::Network(int numNodes, Random &random)
    : numNodes(numNodes)
{
    build(random);
}

std::pair<NodeId, NodeId> Network::linkKey(NodeId a, NodeId b) const
{
    if (a > b)
    {
        std::swap(a, b);
    }
    return std::make_pair(a, b);
}

bool Network::isConnected() const
{
    if (numNodes == 0)
    {
        return true;
    }

    std::vector<bool> visited(numNodes, false);
    std::queue<NodeId> toVisit;
    toVisit.push(0);
    visited[0] = true;
    int reached = 1;

    while (!toVisit.empty())
    {
        NodeId current = toVisit.front();
        toVisit.pop();
        for (NodeId neighbor : adjacency[current])
        {
            if (!visited[neighbor])
            {
                visited[neighbor] = true;
                reached = reached + 1;
                toVisit.push(neighbor);
            }
        }
    }
    return reached == numNodes;
}

void Network::build(Random &random)
{
    bool ok = false;
    while (!ok)
    {
        adjacency.assign(numNodes, std::vector<NodeId>());
        std::vector<std::set<NodeId>> links(numNodes);

        for (NodeId a = 0; a < numNodes; a = a + 1)
        {
            int target = random.uniformInt(3, 6);
            int attempts = 0;
            while (static_cast<int>(links[a].size()) < target && attempts < 200)
            {
                attempts = attempts + 1;
                NodeId b = random.uniformInt(0, numNodes - 1);
                if (b == a)
                {
                    continue;
                }
                if (static_cast<int>(links[b].size()) >= 6)
                {
                    continue;
                }
                links[a].insert(b);
                links[b].insert(a);
            }
        }

        bool degreesOk = true;
        for (NodeId a = 0; a < numNodes; a = a + 1)
        {
            int degree = static_cast<int>(links[a].size());
            if (degree < 3 || degree > 6)
            {
                degreesOk = false;
                break;
            }
        }

        for (NodeId a = 0; a < numNodes; a = a + 1)
        {
            adjacency[a].assign(links[a].begin(), links[a].end());
        }

        if (degreesOk && isConnected())
        {
            ok = true;
        }
    }

    rho.clear();
    for (NodeId a = 0; a < numNodes; a = a + 1)
    {
        for (NodeId b : adjacency[a])
        {
            if (a < b)
            {
                rho[linkKey(a, b)] = random.uniformReal(0.010, 0.500);
            }
        }
    }
}

const std::vector<NodeId> &Network::peersOf(NodeId node) const
{
    return adjacency[node];
}

double Network::linkSpeed(const Node &a, const Node &b) const
{
    if (a.isFast() && b.isFast())
    {
        return 100.0 * 1000000.0;
    }
    return 5.0 * 1000000.0;
}

double Network::latency(const Node &from, const Node &to, long long messageBits, Random &random) const
{
    double propagation = rho.at(linkKey(from.id, to.id));
    double speed = linkSpeed(from, to);
    double transmission = static_cast<double>(messageBits) / speed;
    double queuingMean = 96000.0 / speed;
    double queuing = random.exponential(queuingMean);
    return propagation + transmission + queuing;
}
