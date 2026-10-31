# Contract: the `graph` JSON record

Phase 1 output (specs/037). The top-level record the JSON document
gains (FR-023, D-7), standing after `precheck` and before
`benchmarks` in the top-level `x-order`, the placement pattern the
pre-check record set (specs/034 Assumptions). The schema
`docs/schema/report-v1.json` carries the keys and the moved `x-order`
in the same commit; the minor takes `1.7`, the reservation the Spec
identity section of spec.md states. `loadReport` reads the
record back into the public `graph` member of `ReportData`.

## Key order

```json
["schemaVersion", "speedgun", "run", "jitter", "precheck", "graph", "benchmarks", "hunt"]
```

## The `graph` object

| Key | Kind | Meaning | Requirement |
| --- | --- | --- | --- |
| `runs` | array | one entry per threaded run, in batch order | FR-023 |

## One `runs` entry

| Key | Kind | Meaning | Requirement |
| --- | --- | --- | --- |
| `benchmark` | string | the run's benchmark name, the key the reader folds the entry by | FR-023 |
| `nodes` | array | one entry per node in ascending slot order (FR-023) | FR-008, FR-019 |
| `edges` | array | one entry per dependency in file order | FR-014 |
| `held` | array | one entry per held node, in ascending slot order | FR-020, FR-023 |
| `stopped` | array of strings | the stopped node names, in ascending slot order | FR-019 |

The entry object's `x-order` stands
`["benchmark", "nodes", "edges", "held", "stopped"]`.

| Sub-object | Keys (`x-order`) | Rule |
| --- | --- | --- |
| a `nodes` entry | `["name", "slot", "outcome"]` | `outcome` carries one of `finished`, `failed`, `held`, `stopped` (FR-019) |
| an `edges` entry | `["from", "to"]` | one dependency pair of node names (FR-014) |
| a `held` entry | `["node", "reason"]` | `reason` names the failed node that gates the held node (FR-020, FR-023) |

## Rules

- The record carries one entry per threaded run of a graph run, each
  naming its run by the benchmark name; a batch of two threaded runs
  gives two entries (the clarify answer of 2026-10-29).
- A single-context run gains no entry (FR-012).
- The reason field names a failed node alone; a stopped node carries no
  reason (FR-023, the clarify answer).
- The record carries no timing figure (FR-024).
- A run with no `--graph` writes no `graph` key, and its document
  matches master's bytes apart from the `schemaVersion` value and the
  `speedgun.version` value the version move carries (FR-006).
- `loadReport` reads the record into `ReportData::graph`; the empty
  optional stands for a document with no record (FR-023).
- The schema test fails a document that carries a duplicate key or an
  order the `x-order` lists do not state, the standing rule.
- The console block of the same run folds equal against the entry
  (SC-002, SC-006).
- The node array lists the nodes in the ascending slot order FR-023
  fixes, and the console block prints that order (FR-022).

## Public struct

`ReportData` gains `std::optional<GraphRecord> graph`
(`include/speedgun-ng/benchmark.hpp`), beside the `precheck` member at
`:786`. The record's types (`GraphRecord`, `GraphRunRecord`,
`GraphNodeRecord`, `GraphEdgeRecord`, `GraphHeldRecord`) stand in the
public header, and the harness `SOVERSION` rises to `14` with the
version move (FR-004, R-13).

## Consumers

- `test/source/harness_graph_report_test.cpp` drives the keys, the
  order, the read-back and the byte identity.
- `source/harness/json_report.cpp` writes the record;
  `source/harness/json_load.cpp` reads it.
- `docs/schema/report-v1.json` carries the keys and the moved
  top-level `x-order`.
