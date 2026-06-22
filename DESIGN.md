# Design

Notes on how the simulator is put together and how one run actually flows.

## Discrete event model

The clock does not tick. There is a priority queue of future events, each with
a timestamp. The engine pops the earliest one, jumps the clock to it, and
handles it. Handling an event usually pushes more events further into the
future. The run stops when the queue empties or the next event falls past the
requested duration.

Four event types:

* `GenerateTransaction` - a peer creates a payment.
* `ReceiveTransaction` - a peer hears a transaction from a neighbour.
* `ReceiveBlock` - a peer hears a block from a neighbour.
* `MiningComplete` - a peer finished the PoW delay on the block it was building.

## Modules

One header in `includes/` per module, with a matching `.cpp` in `src/` where
there is anything to implement.

* **Types** - id aliases and the three enums.
* **Config** - plain struct of run parameters.
* **Random** - wrapper over `std::mt19937`. One shared instance, so one seed
  reproduces a whole run. Exponential, uniform real, uniform int.
* **Transaction** - a payment or the 50 coin coinbase. 1 KB each.
* **Block** - parent id, miner, height, transaction list. Size in KB equals the
  transaction count, capped at 1 MB.
* **Event** - a queued item plus the comparator that orders the queue.
* **Node** - one peer's state: its block tree, arrival times, transaction pool,
  seen sets, and an orphan buffer for blocks whose parent has not shown up yet.
  It can also compute balances along a chain and validate a block.
* **Network** - the graph. Builds a random connected topology with degree 3 to
  6, fixes a propagation delay per link, and computes latency.
* **Simulator** - the engine: event queue, nodes, network, role assignment,
  hashing power split, the event handlers, and the final output.
* **main** - parses argv into a Config and starts the engine.

## Building the topology

`Network::build` wires each peer to a random 3 to 6 neighbours, then checks
with a BFS that the graph is one connected piece and that every degree is still
in range. If either check fails the whole graph is thrown away and rebuilt.
Once it passes, each link gets a fixed propagation delay drawn uniformly
between 10 ms and 500 ms.

## Latency

```
latency = propagation delay + (message bits / link speed) + queuing delay
```

Link speed is 100 Mbps when both peers are Fast, otherwise 5 Mbps. The queuing
delay is drawn fresh for every message from an exponential with mean
96 kbits / link speed.

## Mining

Every node starts mining on genesis. To start, a node builds a candidate on top
of its current longest chain: coinbase to itself first, then transactions from
its pool that are valid and not already in the chain, stopping at the 1 MB
limit. It schedules `MiningComplete` after an exponential delay with mean
`I / hashPower`. High CPU nodes get ten times the hashing power of low CPU
nodes and the fractions sum to one.

When the event fires the node checks whether its longest chain is still the one
it started mining on. If yes the block is accepted, added, broadcast, and
mining restarts on the new tip. If the tip already moved because a longer chain
arrived, the finished block is dropped. That is how losing forks die.

## Loop free forwarding

Each node keeps the ids of every transaction and block it has already seen. The
first copy gets forwarded to every neighbour except the sender; later copies are
ignored, so nothing circulates forever.

## Fork resolution

Longest chain wins. A node always treats the validated block with the greatest
height as its tip, and restarts mining when the tip moves. Since every node
applies the same rule they converge, apart from the last few blocks still in
flight when the run stops.

## One run, end to end

```
main
  build Config from argv
  Simulator ctor
    build the Network (connected graph + link delays)
    create Nodes, assign Fast/Slow, High/Low CPU, hashing power
    give every Node the same genesis block
    schedule the first transaction and first mining per Node
  Simulator::run
    pop earliest event, advance clock, dispatch
      GenerateTransaction -> maybe create a payment, broadcast, schedule the next
      ReceiveTransaction  -> if new, remember and forward
      ReceiveBlock        -> if new, forward, then validate and attach (buffer orphans)
      MiningComplete      -> if the tip still matches, accept, broadcast, mine again
    then write the tree files and print the summary
```

## Output

`output/node_<id>_tree.txt` per node, listing every block with parent, miner,
height, arrival time, transaction count, size and whether it sits on that
node's longest chain. Node 0's tree also goes to `output/node_0_tree.dot` for
Graphviz.
