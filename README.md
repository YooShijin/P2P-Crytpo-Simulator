# P2P Cryptocurrency Network Simulator

A discrete event simulator for a peer to peer cryptocurrency network in C++17.
It generates transactions, gossips them over a random connected topology with
realistic latency, mines blocks with a Proof of Work delay, and resolves forks
by longest chain.

## Build

```
make
```

Or without make:

```
g++ -std=c++17 -I includes src/*.cpp -o simulator
```

## Run

```
./simulator <z0_slow_%> <z1_lowcpu_%> [numNodes] [Ttx] [I] [duration] [seed]
```

- `z0_slow_%` percent of peers that are slow (required)
- `z1_lowcpu_%` percent of peers that are low CPU (required)
- `numNodes` peers in the network, at least 4 (default 15)
- `Ttx` mean seconds between a peer's transactions (default 5)
- `I` average block interarrival time in seconds (default 60)
- `duration` simulation length in seconds (default 2000)
- `seed` random seed (default 42)

```
./simulator 40 50
./simulator 50 50 20 3 2 800 7      # short block interval, lots of forks
```

## Output

A summary is printed at the end: each node's type, hashing power, blocks mined
and how many of them survived on the longest chain.

One file per node is written to `output/`:

```
output/node_0_tree.txt
```

Columns are:

```
blockId parentId miner height arrivalTime numTxns sizeKB inLongestChain
```

`arrivalTime` is when that node first heard about the block.

Node 0's tree is also dumped as `output/node_0_tree.dot`. With Graphviz:

```
dot -Tpng output/node_0_tree.dot -o tree.png
```

Green is the longest chain, grey is the forks that lost.

`make clean` removes the binary and everything in `output/`.

## Layout

```
includes/    headers, one per module
src/         implementations
output/      generated tree files
DESIGN.md    how the modules fit together
```
