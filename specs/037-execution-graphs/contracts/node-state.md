# Contract: the four outcome words

Phase 1 output (specs/037). Each node settles exactly one word:
`finished`, `failed`, `held` or `stopped` (FR-019, D-5). The words
stand in the graph, and the work items stay `noexcept` (FR-018). A
threaded run has one merged row, so the row names no node on its own;
the run reads the per-context outcome the harness records (the clarify
answer of 2026-10-29).

## The words and their causes

| Word | Cause | Requirement |
| --- | --- | --- |
| `finished` | a measured row settles every node of its run finished; a skipped row settles every node of its run finished | FR-019 |
| `failed` | the context whose body recorded an error settles its node failed; a fixture setup failure settles the node at slot 0, the context that runs the setup half | FR-019 |
| `held` | a dependency settled `failed` or `held`; the transitive closure of dependents holds with them and never starts | FR-020 |
| `stopped` | the stop request refuses the node's start, the standing shape of specs/020 FR-050; the dependents settle `stopped` with it | FR-019 |

## The reads

| Read | Rule | Requirement |
| --- | --- | --- |
| the merged row reads `MEASURED` | every node of the run settles `finished` | FR-019 |
| the merged row reads `SKIPPED` | every node of the run settles `finished`, and every dependent starts | FR-019, US4 scenario 3 |
| the merged row reads `FAILED` | the per-context outcome read names the failing context's node `failed`; the other nodes of the run settle `finished`; the failed node's dependents hold | FR-019, FR-020, the Edge Cases entry |
| a fixture setup fails | the node at slot 0 settles `failed`, its dependents settle `held` with that node as their reason | FR-019, the clarify answer |
| two nodes fail | each held node names the failed node that gates it | FR-020 |
| a stop request stands | gated nodes settle `stopped`, their dependents with them, the record names each with no reason | FR-019, FR-023 |

## The run's shape

- The run finishes the nodes that stand on their own (FR-021).
- The exit status follows the standing run-failure status of specs/015
  (FR-021).
- The status-2 refusal shape stays reserved for what the run refuses
  before it starts (FR-021).
- The record names each held node with the failed node that gates it,
  and the reason field names a failed node alone (FR-020, FR-023).

## Consumers

- `test/source/harness_graph_gate_test.cpp` drives every word and
  every read through injected graphs and bodies that fail.
- `test/source/harness_graph_report_test.cpp` drives the words into
  the record and the console block.
