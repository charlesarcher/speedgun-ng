# Data Model: Execution Graphs

Feature: specs/037-execution-graphs. The standing run model of
`specs/020-execution-contexts-core/data-model.md` and the report model
of specs/023 and specs/034 stand unchanged beside this file; this file
names what the feature adds. The spec's decisions D-1 to D-10 own the
shapes, and the plan's R-01 to R-16 settle the seats.

## The graph file (input, `docs/schema/graph-v1.json`)

One JSON document, the operator's input through `--graph`.

| Key | Kind | Rule |
| --- | --- | --- |
| `nodes` | array | one entry per node, in the file's order; the top level carries no key beside it |
| `nodes[].name` | string | the name the operator chooses; unique across the nodes |
| `nodes[].slot` | integer | one context slot counted from `0`; present on every node |
| `nodes[].deps` | array of strings | the dependencies, named by node name; empty stands for a source node |

The normative key order stands in the schema's `x-order`: the top
level `["nodes"]`, the node object `["name", "slot", "deps"]`. The
file carries no benchmark name, no preset, no cpu index and no timing
figure (FR-008), and the schema admits no key beside the four above.

Validation runs in the order FR-010 lists in spec.md, first fault
alone, one line, exit status 2, before any context starts (the clarify
answer of 2026-10-29). The table names the line each fault prints:

| Order | Fault | The line names |
| --- | --- | --- |
| 1 | a file the run cannot read | the path |
| 2 | a file the parser rejects | the path and the fault |
| 3 | a document that breaks the schema's shape | the key, the order break or the kind |
| 4 | a repeated node name | the repeated name |
| 5 | a node with no slot | the node |
| 6 | a slot outside the run's context count | the node and the slot |
| 7 | an edge naming no node | the edge |
| 8 | a self-edge | the node |
| 9 | a cycle of two or more nodes | the cycle |
| 10 | a dependency the run cannot satisfy (a slot the run does not start, the slot map not a bijection) | the node and the slot |

The shape check (3) finishes before the node rules (4 to 10) (FR-007).
The run-relative rules (6, 10) and the count rule of FR-012 run per
threaded run in batch order, and the first fault in batch order prints
alone.

## The graph model (detail, `source/harness/detail/graph.hpp`)

### `GraphNode`

| Field | Kind | Rule |
| --- | --- | --- |
| `name` | string | the operator's name, unique |
| `slot` | `std::uint32_t` | the context index in the run's plan order, counted from `0` |
| `deps` | vector of node names | the dependencies; the edge set is these pairs |

### The slot-to-node map

The bijection from the run's context indexes to the nodes. With the
node count equal to the context count (FR-012) and every slot inside
the count (rule 6), the map stands one node per slot when the slots
stand distinct; a repeated slot leaves a slot no node names, and rule
10 answers it.

### `NodeOutcome` (the four words, FR-019)

| Word | Cause |
| --- | --- |
| `finished` | the node's body ran and settled; a measured row settles every node of its run finished, and a skipped row settles every node finished |
| `failed` | the context's body recorded an error, or a fixture setup failure settles the node at slot 0 |
| `held` | a dependency settled `failed` or `held`; the transitive closure of dependents holds with them (FR-020) |
| `stopped` | the stop request refused the node's start, the standing shape of specs/020 FR-050; the dependents settle `stopped` with it |

Validation: each node settles exactly one word (FR-019). The reason
field names a failed node alone (FR-023): a held node names the
nearest failed node on its dependency chain, and a stopped node names
no reason (the clarify answer of 2026-10-29).

### The gate state (detail, per threaded attempt)

| Field | Rule |
| --- | --- |
| remaining | per node: the count of dependencies not yet finished; zero stands ready |
| settled | per node: the `NodeOutcome` word, empty until the node settles |
| mutex, condition variable | the gate's state behind one mutex; the completion state of specs/020 FR-049 read with acquire order (research D-10) |

Transitions: a node stands gated until ready; release starts it; the
body's settle moves it to `finished` or `failed`; the stop request
moves gated nodes to `stopped`; a failed or held dependency moves
gated dependents to `held`. No node leaves its word, and no word
returns.

## The graph record (output, `docs/schema/report-v1.json`, `ReportData`)

### `GraphRecord` (public, `include/speedgun-ng/benchmark.hpp`)

| Key | Kind | Rule |
| --- | --- | --- |
| `runs` | array | one entry per threaded run in batch order; a run with no `--graph` writes no record |

### One `runs` entry (`GraphRunRecord`)

| Key | Kind | Rule |
| --- | --- | --- |
| `benchmark` | string | the run's benchmark name, the key the reader folds the entry by |
| `nodes` | array | one entry per node in ascending slot order (FR-023): `name`, `slot`, `outcome` |
| `edges` | array | one entry per dependency in file order: `from`, `to` |
| `held` | array | one entry per held node, in ascending slot order (FR-023): `node`, `reason` naming the failed node that gates it |
| `stopped` | array of strings | the stopped node names, in ascending slot order, no reason |

The record stands after `precheck` and before `benchmarks` in the
top-level `x-order` (FR-023, D-7); the schema minor takes `1.7`, the
reservation R-13 states. `loadReport` reads it into
`ReportData::graph`, an empty optional for a document with no record
(FR-023). The record carries no timing figure (FR-024).

## The console block (output, `report.cpp`)

One block per threaded run, directly after that run's `contexts=`
line (FR-022):

| Line | Case |
| --- | --- |
| `graph nodes=<names in ascending slot order> edges=<pairs in file order>` (FR-022, FR-023) | every threaded run of a graph run |
| `graph held=<node>(<gating failed node>)` | a node stands held; one line naming all held nodes in ascending slot order |
| `graph stopped=<names>` | a node stands stopped; one line naming all stopped nodes in ascending slot order |

The names and pairs fold equal against the record's entries of
the same run (SC-002, SC-006).

## The refusal set

| Message shape | Case | Status |
| --- | --- | --- |
| one line naming the row and the missing path | `--graph` with no path in any source (FR-009) | 2 |
| one line naming the fault | each FR-010 fault, first alone | 2 |
| one line naming both counts | a threaded run's context count differs from the node count (FR-012) | 2 |
| one line | a batch with no threaded run (FR-012) | 2 |
| the standing run-failure status | a node failed; the run finished the nodes that stand on their own (FR-021) | the standing status of specs/015 |

## State transitions

The gate's words settle once per node, the table above. The graph
file, the model and the record hold no other lifecycle: the file reads
once before the first context starts (FR-011), the gate lives in one
threaded attempt, and the record lands with the report. A refused run
starts no context, so no partial state survives it.
