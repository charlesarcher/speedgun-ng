# Contract: the graph file and its schema

Phase 1 output (specs/037). The file the operator writes, the shape
`docs/schema/graph-v1.json` names, the way `docs/schema/compare-v1.json`
names the comparison document's (D-1 of the spec). The reader is the
vendored simdjson, the only JSON reader the change uses (FR-029), and
the checker is the standing `checkReport`
(`source/harness/json_load.cpp:2151`): one reader, one schema language,
one checker cover this file and the report record.

## Grammar

```json
{
  "nodes": [
    { "name": "a", "slot": 0, "deps": [] },
    { "name": "b", "slot": 1, "deps": ["a"] },
    { "name": "c", "slot": 2, "deps": ["b"] }
  ]
}
```

| Key | Kind | Rule | Requirement |
| --- | --- | --- | --- |
| `nodes` | array | one entry per node, in the file's order; the top level carries no key beside it | FR-007, FR-008 |
| `name` | string | unique across the nodes | FR-008 |
| `slot` | integer | one context slot counted from `0`, present on every node | FR-008 |
| `deps` | array of strings | dependencies named by node name; empty for a source node | FR-008 |

The `x-order` lists are normative: the top level `["nodes"]`, the node
object `["name", "slot", "deps"]`. The file carries no benchmark name,
no preset, no cpu index and no timing figure, and the schema admits no
key beside the four (FR-008).

## Check order

The reader runs the order FR-010 lists in spec.md and prints the first
fault it meets alone, one line, exit status 2, before any context
starts. The table names the line each fault prints:

| Order | Fault | The line names |
| --- | --- | --- |
| 1 | a file the run cannot read | the path |
| 2 | a file the parser rejects | the path and the fault |
| 3 | a document that breaks the schema's shape (key set, key order, value kinds) | the key, the order break or the kind |
| 4 | a repeated node name | the repeated name |
| 5 | a node with no slot | the node |
| 6 | a slot outside the run's context count | the node and the slot |
| 7 | an edge naming no node | the edge |
| 8 | a self-edge | the node |
| 9 | a cycle of two or more nodes | the cycle |
| 10 | a dependency the run cannot satisfy (a slot the run does not start) | the node and the slot |

The shape check (3) finishes before the node rules (4 to 10) (FR-007).
A file with several faults prints the first in this order and the run
stops there (the clarify answer of 2026-10-29). The run-relative rules
(6, 10) run per threaded run in batch order.

## Schema file

`docs/schema/graph-v1.json` carries the JSON Schema shape with the
`x-order` lists above, the description naming the file, and
`additionalProperties: false` at both levels. The harness checker reads
this file and compares the key set, the key order and the value kinds
of every graph file against it, the standing checker's rule (FR-007).

## Consumers

- `test/source/harness_graph_file_test.cpp` drives every fault arm
  through planted files and the clean file through the gate.
- `source/harness/graph_file.cpp` runs the pipeline.
- `example/docs/execution_graphs.cpp` records a run of one graph file.
