# Contract: the start rule

Phase 1 output (specs/037). The gate, the fan-out, the fan-in, the
held node and the stopped node (FR-013 to FR-017, D-4). The gate
stands in the body of each context, after the start round of
`source/harness/runner.cpp:2073` and before the callable, beside the
standing setup-failure skip, part of neither round, and every context
of a graph run starts its thread and stands both barrier rounds of
specs/020 FR-019.

## The gate

| Rule | Requirement |
| --- | --- |
| A node starts when every node it depends on has finished, and the completion state of specs/020 FR-049 stands the fact the gate reads: a dependency stands finished when its dispatched work node reads `DONE`, and the gate reads that state with acquire order. | FR-013 |
| A node with no dependency starts at the standing start of the run: the point where the run has dispatched every body index and every context has arrived at the start round. | FR-013 |
| Among the nodes ready to start the gate releases the lowest slot first. FR-013 fixes the release order, and FR-023 fixes the printed order. | FR-013, FR-023 |
| A finished node releases every node that depends on it alone. | FR-014 |
| A node with several dependencies starts after the last of them finishes. | FR-014 |
| The gate stands between the two barrier rounds, is part of neither round, and the graph gates the start of a context and touches nothing inside it. | FR-015, FR-016 |
| The graph reads no clock, opens no measured window, closes none, shifts none, and moves no barrier round inside a window. | FR-016 |
| The graph changes no placement, no pin, no memory bind, no wait mode, no plan and no recorder. | FR-017 |

## Fan-out and fan-in

```text
fan-out: a finished source releases every node that depends on it alone
fan-in:  a node starts after the last of its dependencies finishes
order:   among ready nodes the gate releases the lowest slot first
         (FR-013); FR-023 fixes the printed order
```

## Held

| Rule | Requirement |
| --- | --- |
| A node whose dependency settled `failed` never starts and settles `held`; the transitive closure of dependents holds with them. | FR-020 |
| The context of a held node arrives at both barrier rounds with no body run and no measured window open, so no round strands. | FR-015 |
| The record names each held node with the failed node that gates it as its reason, the nearest failed node on the dependency chain. | FR-020, FR-023 |
| A node whose dependency stands held settles `held` too, and the reason names the failed node that gates the chain. | FR-020, FR-023 |

## Stopped

| Rule | Requirement |
| --- | --- |
| A node the stop request refuses settles `stopped`, the standing shape of specs/020 FR-050. | FR-019 |
| The dependents of a stopped node settle `stopped` with it. | FR-019 |
| The context of a stopped node arrives at both barrier rounds, the held shape. | FR-015 |
| The record names each stopped node with no reason. | FR-023, the clarify answer of 2026-10-29 |

## The raw path

The work items stay `noexcept`, no exception crosses the timed path,
and the graph adds no error state to a work item (FR-018). The
dispatch of every body index stands unchanged, the `threadRunner`
ledger of specs/020 FR-021 included: the graph gates the body, and the
context thread starts as the run starts it today.

## Consumers

- `test/source/harness_graph_gate_test.cpp` drives the chain, the
  fan-out, the fan-in, the diamond, the held arms, the stopped arm and
  the barrier arrival through `installTestGraph`.
- `source/harness/graph_gate.cpp` and `source/harness/runner.cpp`
  carry the gate.
