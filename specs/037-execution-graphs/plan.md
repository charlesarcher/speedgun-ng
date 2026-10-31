# Implementation Plan: Execution Graphs

**Branch**: `037-execution-graphs` | **Date**: 2026-10-29 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `/specs/037-execution-graphs/spec.md`

## Summary

The threaded run gains the item N11 execution graph. The contexts of a
threaded run stand as nodes of a graph the operator writes in one JSON
file, and the run starts each node when the nodes it depends on have
finished. The graph file rides the vendored simdjson reader, and
`docs/schema/graph-v1.json` names its shape with a normative key order,
the way `docs/schema/compare-v1.json` names the comparison document's.
The gate stands in the body of each context, after the start round and
before the stop round at `source/harness/runner.cpp:2013`, beside the
standing setup-failure skip, part of neither round, so every context
arrives at both rounds and no round strands. The gate reads the
completion state of specs/020 FR-049 as the fact of a finished
dependency, and among ready nodes the gate releases the lowest slot
first. Each node settles one outcome word:
`finished`, `failed`, `held` or `stopped`. A failed node holds its
dependents, and the record names each held node with the failed node
that gates it. The console names the nodes in ascending slot order in one
block beside the `contexts=` line of each threaded run (FR-022), and the JSON
gains a top-level `graph` record at schema minor `1.7`, standing after
`precheck` and before `benchmarks`, which `loadReport` reads back into
`ReportData`. The `--graph` row joins the option table with the value
`297`. The harness `SOVERSION` rises to `14` because the record joins
the public loaded-report struct; the archive `SOVERSION` stays `4`.

## Technical Context

**Language/Version**: C++23 (`cxx_std_23` on every target; the
constitution pins the language)

**Primary Dependencies**: the standing threaded run (the two barrier
rounds at `source/harness/runner.cpp:2013`, arrived at `:2073` and
`:2096`, the hazard comment at `:2030`); the four H16 hooks of specs/020
FR-045 (`ContextId`, `PlacementSlot`, `WorkNodeState` with the
completion state at `include/speedgun-ng/execution_context.hpp:494`,
`dispatch()`, `ContextBarrier`), unchanged; the vendored simdjson
reader and the standing schema checker `checkReport`
(`source/harness/json_load.cpp:2151`); the option table
(`source/harness/options.cpp:544`) and its value blocks
(`source/harness/detail/options.hpp:76`); the report machinery
(`printContextDisclosure` at `source/harness/report.cpp:167`,
`json_report.cpp`, `json_load.cpp`, `docs/schema/report-v1.json`); the
seam pattern of `installTestLayout`, `installTestOnlineCpus` and
`installTestMemoryBinder` (`source/harness/topology.cpp:336`, `:352`,
`:363`). No new dependency joins: simdjson carries the file read.

**Storage**: the graph file the operator names through `--graph`, one
JSON document read once before the first context starts. The one
document the feature writes is the report file `--json` names. The
acceptance record stands under `specs/037-execution-graphs/results/`.

**Testing**: the repository's own harness (`speedgun-ng_test` pattern:
one executable per area plus `add_test`), five new test executables
`harness_graph_model_test`, `harness_graph_file_test`,
`harness_graph_gate_test`, `harness_graph_report_test` and
`harness_cli_graph_test`; synthetic graphs installed through
`installTestGraph`; planted files under a temporary directory for every
file arm; the cpus through `installTestOnlineCpus`; no test sleeps and
no test reads a clock; tests run with `ctest -j 2` and pass under
`taskset -c 0,1` and on a 2-cpu host.

**Target Platform**: Linux only (`ci-ubuntu`, `ci-rocky`). The daily
loop drives `ci-rocky`; once per feature, `ci-ubuntu` builds with
clang-tidy and cppcheck wired in. The acceptance shape stands on the
request's lane host: a 16-core, 32-thread box, beside the 24-core,
48-thread record of specs/035. Every test passes on a 2-cpu host with
no fixed node count (FR-027).

**Project Type**: library feature: one option row, one file reader, one
gate inside the threaded run, one report record, one detail seam. No
new target beyond test executables and the example.

**Performance Goals**: the gate stands outside every measured window:
one condition-variable wait per context between the two barrier rounds,
beside the standing setup-failure skip, and no timed path reads the
graph. The timed path stays raw (FR-016). The timed path is the
designated critical path of Principle VII, the measured window of a
context, unchanged by this feature. The gate stands outside every
window. The baseline for the threaded start path stands deferred with
the constitution's Open deferrals entry: Principle VII baseline
infrastructure absent, VII mandates it, a future spec delivers it.

**Constraints**: 100% line and branch coverage with no `LCOV_EXCL`
marker and no new `--ignore-errors` entry; no sleep and no clock in the
graph and in a test; no barrier round inside a window; no lambda inside
an `SG_*` macro; simdjson as the only JSON reader; the public surface
grows only where the record's contract names it; the graph adds no
runtime dependency; one line and exit status 2 for every refusal the
spec names; no fixed cpu number and no fixed node count.

**Scale/Scope**: one new option row, one reader unit with one schema
file, one model header with one seam, one gate unit and the threaded
run extended, the console block, the JSON writer, reader and schema,
the public record types, five new test executables, one example with
its `.expected` file, the acceptance script and record, and the docs
rows.

**Execution mode**: TDD (Principle III). Every test task is written and
observed failing before the code task that turns it green exists; the
Test Plan records the waves, the red step of each, and the gates each
runs.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1
design.*

| Principle | Gate | Status |
| --- | --- | --- |
| I. Standard-first | C++23 surface. The gate rides the standing barrier rounds, the completion state of FR-049 and the standard condition variable; the reader rides the vendored simdjson; no compiler extension joins | PASS |
| II. Design by contract | The seam, the reader, the gate and the record carry `\pre`, `\post` and `\invariant` and the `SG_*` checks at their edges; the work items stay `noexcept` (FR-018), so no contract sits on a throwing path | PASS |
| III. R-DCUT | Requirements FR-001 to FR-034 map to the decisions R-01 to R-16 below, to the module table, and to `tasks.md` (landing with `/speckit.tasks`). Execution mode: TDD, recorded above and in the Test Plan | PASS |
| IV. Documentation | README graph section of at most 25 lines with its quick-reference rows (IV.1), `docs/pages/harness.md` option and record detail, one runnable example with its `.expected` file (FR-032 to FR-034) | PASS |
| V. Style | clang-format, clang-tidy, cppcheck clean at the feature head; every new name follows V.1: PascalCase types (`Graph`, `GraphNode`, `GraphRecord`), camelBack functions (`readGraphFile`, `installTestGraph`), `k`-prefixed constants (`kGraphOptionValue`), lower_case file names (`graph.cpp`, `graph.hpp`) | PASS |
| VI. Coverage | 100% line and branch on new paths, no exclusion marker; every refusal arm of FR-010, every gate arm of release, held, failed and stopped, the settlement reads, the console block and the record round trip run under injected graphs and planted files (the Test Plan names each) | PASS |
| VII. Performance | The gate stands between the two barrier rounds, outside every measured window; the graph reads no clock; the timed path keeps its bytes (FR-016, FR-024); the timed path is the designated critical path, the measured window of a context; the baseline for the threaded start path stands deferred with the constitution's Open deferrals entry | PASS |
| VIII. Hard CI gates | Every gate stands as a task: format T039, spell T040, prose T041, clang-tidy, cppcheck and name-check T042, sanitizers T043, tsan T044, coverage T045, docs gates T046, two-cpu and time-source T047, DBC gate T050, the wave gates for the builds and `ctest`; the performance metric run stands under the Open deferrals, and the debug-build and recorded-results items stand carried by the standing CI job set, with no feature task | PASS |
| IX. Spec-driven | Spec, plan and research stand in `specs/037-execution-graphs/`; `tasks.md` lands with `/speckit.tasks` before code | PASS |
| X. Anti-slop | One row, one reader, one gate, one record, one seam; the Out of scope list of the spec names what a larger design would add, and this plan adds none of it; no priorities, no weights, no cycle repair, no graph editing, no public graph type beyond the record | PASS |
| XI. Prose | The gate walks the tracked tree; these artifacts land with the branch's commits and the gate clears them at the head, the way specs/034 and specs/035 recorded | PASS at the head |

**Gate weakening requested**: no. **Gate strengthening requested**:
no. No budget figure moves: the README section lands inside the
standing 25-line section cap.

No gate fails, so **Complexity Tracking stays empty**.

## Project Structure

### Documentation (this feature)

```text
specs/037-execution-graphs/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output (/speckit.plan command)
├── data-model.md        # Phase 1 output (/speckit.plan command)
├── quickstart.md        # Phase 1 output (/speckit.plan command)
├── contracts/           # Phase 1 output (/speckit.plan command)
│   ├── cli-graph.md     # The option row, the grammar, the refusals
│   ├── graph-file.md    # The file grammar and its schema
│   ├── graph-record.md  # The JSON record, keys, order, read-back
│   ├── start-rule.md    # The gate, fan-out, fan-in, held, stopped
│   ├── node-state.md    # The four words and their causes
│   └── seams.md         # The injected graph and the synthetic layouts
├── results/             # The acceptance record (FR-030, D-9)
└── tasks.md             # Phase 2 output (/speckit.tasks command - NOT created by /speckit.plan)
```

### Source (this feature)

```text
include/speedgun-ng/benchmark.hpp        # + GraphRecord types, ReportData::graph
source/harness/detail/graph.hpp          # new: the model, the words, the gate type
source/harness/detail/internal.hpp       # + installTestGraph, graphInput, RunOptions.graphPath
source/harness/graph.cpp                 # new: the model rules, the seam slot
source/harness/graph_file.cpp            # new: the reader pipeline (simdjson, schema)
source/harness/graph_gate.cpp            # new: the gate
source/harness/runner.cpp                # + the gate between the two barrier rounds
source/harness/options.cpp               # + the `graph` row and its settle arm
source/harness/detail/options.hpp        # + kGraphOptionValue = 297, the block comment
source/harness/cli.cpp                   # + the parse-once read and the refusal wiring
source/harness/report.cpp                # + printGraphReport beside printContextDisclosure
source/harness/json_report.cpp           # + the `graph` record, schemaVersion 1.7
source/harness/json_load.cpp             # + the record reader
docs/schema/graph-v1.json                # new: the file schema, normative x-order
docs/schema/report-v1.json               # + the record keys, the moved x-order
docs/pages/harness.md                    # + the graph section
README.md                                # + the graph section (25-line cap)
example/docs/execution_graphs.cpp        # new: the recorded graph run
example/docs/execution_graphs.expected   # new: its shape-golden block
example/CMakeLists.txt                   # + add_docs_example(execution_graphs)
tools/docs/example_drift.py              # + the SECTIONS and SECTION_OWNERS rows
tools/graph/acceptance.py                # new: the acceptance record script (FR-031)
test/source/harness_graph_model_test.cpp # new executable
test/source/harness_graph_file_test.cpp  # new executable
test/source/harness_graph_gate_test.cpp  # new executable
test/source/harness_graph_report_test.cpp # new executable
test/source/harness_cli_graph_test.cpp   # new executable
test/source/harness_cli_table_test.cpp   # + the row in `kContract` and the row count
test/source/harness_cli_test.cpp         # + the version scenario of FR-003
test/CMakeLists.txt                      # + the five registrations
CMakeLists.txt                           # + version 0.23.0, harness SOVERSION 14
```

**Structure Decision**: the feature stands inside the harness library
the way specs/034 and specs/035 built theirs. The model, the reader,
the gate and the seam each own one unit; the threaded run, the option
table, the report machinery and the public header each gain one arm.
The module table below names the owner of every file, and the wave
layout keeps the file sets of concurrent modules disjoint.

## Design

### Module table

Every module owns the files named in its row; no two concurrent
modules own one file. The seam column names what the module exposes to
the others. FR-028 (the coverage and DBC gate) is a property of the whole
change: it stands in no module row, and the head gates T045 and T050 own it.

| Module | Files owned | Seam exposed | Depends on | Built beside | FRs |
| --- | --- | --- | --- | --- | --- |
| M1 `graph-model` | `source/harness/detail/graph.hpp`, `source/harness/graph.cpp`, `source/harness/detail/internal.hpp` (the seam declaration, the `graphInput` slot, the `RunOptions.graphPath` field), `test/source/harness_graph_model_test.cpp`, `test/CMakeLists.txt` (its block) | the types `detail::Graph`, `detail::GraphNode`, `detail::NodeOutcome`, `detail::GraphGate` (declared); the seam `installTestGraph`; the slot `graphInput()` | none | built on by M2 and M4 in wave 2 | FR-008, FR-013, FR-020, FR-025 |
| M2 `graph-reader` | `source/harness/graph_file.cpp`, `docs/schema/graph-v1.json`, `test/source/harness_graph_file_test.cpp`, `test/CMakeLists.txt` (its block) | `detail::readGraphFile(path, contextCounts)` answering `std::expected<detail::Graph, std::string>` with the one-line fault | M1 | M4 (wave 2) | FR-007, FR-009, FR-010, FR-011, FR-012, FR-029 |
| M3 `cli-row` | `source/harness/options.cpp`, `source/harness/detail/options.hpp`, `source/harness/cli.cpp`, `test/source/harness_cli_graph_test.cpp`, `test/source/harness_cli_table_test.cpp` (the row joins `kContract` and the row count), `test/CMakeLists.txt` (its block) | the `graph` row of the option table; the settled `RunOptions.graphPath`, the bare-row mark, and the record entry threaded to `printResult` | M1, M2 | M5 (wave 3) | FR-001, FR-002, FR-009, FR-011 |
| M4 `start-gate` | `source/harness/graph_gate.cpp`, `source/harness/runner.cpp`, `test/source/harness_graph_gate_test.cpp`, `test/CMakeLists.txt` (its block) | `detail::GraphGate` (defined): the constructor over a graph, `awaitStart`, `nodeFinished`, `nodeFailed`, `settleStopped`, `outcomes` | M1 | M2 (wave 2) | FR-013 to FR-021, FR-026, FR-027 |
| M5 `record-disclosure` | `include/speedgun-ng/benchmark.hpp`, `source/harness/report.cpp`, `source/harness/json_report.cpp`, `source/harness/json_load.cpp`, `docs/schema/report-v1.json`, `test/source/harness_graph_report_test.cpp`, `test/CMakeLists.txt` (its block) | the public types `sg::GraphRecord`, `sg::GraphRunRecord`, `sg::GraphNodeRecord`, `sg::GraphEdgeRecord`, `sg::GraphHeldRecord`; `ReportData::graph`; `printGraphReport`; the `writeReport` graph parameter | M1, M4 | M3 (wave 3) | FR-005, FR-006, FR-022, FR-023, FR-024 |
| M6 `version-abi` | `CMakeLists.txt` (version, harness `SOVERSION`), `test/source/harness_cli_test.cpp` (the version scenario of FR-003) | the version and `SOVERSION` record with the ABI check beside the move | M5 | M8 (wave 4) | FR-003, FR-004 |
| M7 `docs-example` | `README.md`, `docs/pages/harness.md`, `example/docs/execution_graphs.cpp`, `example/docs/execution_graphs.expected`, `example/CMakeLists.txt`, `tools/docs/example_drift.py` | the README section, the guide rows, the recorded example run | M3, M5, M6 (the recorded run carries the moved version line) | M8 (wave 4) | FR-032, FR-033, FR-034 |
| M8 `evidence` | `tools/graph/acceptance.py`, `specs/037-execution-graphs/results/` | the acceptance record of the lane run | M4, M5 | M6, M7 (wave 4) | FR-030, FR-031 |

### Wave layout

- **Wave 1**: M1. The model, the words, the gate type, the seam and the
  slot stand alone; every later module codes against them.
- **Wave 2**: M2 and M4. Each depends on M1 alone. M2 owns the reader,
  the schema file and its test; M4 owns the gate unit, `runner.cpp` and
  its test. The file sets are disjoint apart from `test/CMakeLists.txt`,
  where each module appends its own labeled block in its own commit.
- **Wave 3**: M3 and M5. M3 wires the row, the parse-once read and the
  refusals; M5 owns the public record, the console block, the writer,
  the reader and the schema. The file sets are disjoint apart from
  `test/CMakeLists.txt`, where each module appends its own labeled
  block in its own commit.
- **Wave 4**: M6, M7 and M8. The version move, the docs and the
  evidence each own disjoint files and run their gates.

A worker takes any module in a wave without touching another worker's
files. Three shared files stand named:

- `test/CMakeLists.txt`: every module appends its own registration
  block. The appends stand in disjoint regions, and each module lands
  its block in its own commit, so the pair serializes by commit order.
- `source/harness/detail/internal.hpp`: M1 owns every line of it this
  feature adds (the seam declaration, the slot, the `RunOptions`
  field), and M1 lands it in wave 1. No later module touches it.
- `source/harness/detail/graph.hpp`: M1 owns it, and the gate type
  declaration lands with M1 in wave 1; M4 implements the gate in its
  own unit and never edits the header.

### Decision index

A bare `D-n` names the decision of that number in spec.md's Decisions
section. research.md numbers its own decisions `D-1` to `D-10`, and the
plan names those only with the source spelled out.

- **R-01 (FR-001, FR-002): the option row.** The table gains one
  row named `graph` with the value `297`
  (`kGraphOptionValue`, the first free value of the block `297` to
  `319` at `source/harness/detail/options.hpp:76`), the argument kind
  `OPTIONAL` (the `--graph=<file>` attached form, the precedent of the
  `hunt` and `interleave` rows), the environment flag off, help text,
  and the owner `specs/037`. The block comment moves in the same
  commit: `297` stands taken, `298` to `319` stay free. The standing
  checks over the table names and values
  (`test/source/harness_cli_table_test.cpp:780`) cover the row. The
  standing range check at `test/source/harness_cli_table_test.cpp:763`
  reads the window `290` to `319`, and the row's value stands in the
  free block `297` to `319`.
- **R-02 (FR-009): the bare row.** The settle arm stores the path text
  of `--graph=<file>`. A bare `--graph` marks the row given with no
  path of its own; the entry point then takes the path from the
  standing lower source, the option file line (specs/028 FR-035), and
  reads no environment variable for the row. When no path stands in
  any source, the run prints one line naming the row and the missing
  path and exits 2 before any context starts.
- **R-03 (FR-007, FR-010, D-1): the reader pipeline.** The reader runs
  one fixed pipeline: read the file; parse with the vendored simdjson;
  check the document against `docs/schema/graph-v1.json` through the
  standing checker `checkReport` (`source/harness/json_load.cpp:2151`,
  the same key set, key order and value kind check the report schema
  runs); then the node rules. The schema file names the shape with a
  normative `x-order`: the top level `["nodes"]`, one node object
  `["name", "slot", "deps"]`, the dependency list an array of strings.
  The key names stand `nodes`, `name`, `slot`, `deps`. The shape check
  finishes before the node rules. The node rules run in the FR-010
  order, the ten faults FR-010 lists in spec.md, first fault alone. The
  run prints the first fault it meets alone, one line, exit status 2,
  and stops there.
- **R-04 (FR-008, D-3): the graph model.** One `GraphNode` carries the
  name the operator chooses, one context slot counted from `0` (the
  context index in the run's plan order), and the dependency list of
  node names. One edge is one dependency pair, read in the direction a
  finished node releases a gated node. The file carries no benchmark
  name, no preset, no cpu index and no timing figure, and the schema
  admits no key beside `name`, `slot` and `deps`. The slot-to-node map
  is the bijection from the run's context indexes to the nodes: with
  the node count equal to the context count (FR-012) and every slot
  inside the count (FR-010), the map stands one node per slot when the
  slots stand distinct. A slot two nodes name leaves a slot no node
  names, and the reader answers the FR-010 fault
  `a dependency the run cannot satisfy` with the line naming the node
  and the slot. The four outcome words `finished`, `failed`, `held`,
  `stopped` stand the node's state; the model owns them, and the work
  items stay `noexcept` (FR-018).
- **R-05 (FR-012, D-2): the batch shape.** The graph applies to every
  threaded run of the batch. The entry point reads the selected
  instances' context counts after the expansion, ahead of any run: a
  threaded run whose context count differs from the node count prints
  one line naming both counts and exits 2; a batch with no threaded
  run prints one line and exits 2. A single-context run takes no graph
  and gains no record entry. The run-relative FR-010 checks run per
  threaded run in batch order, and the first fault in batch order
  prints alone.
- **R-06 (FR-013, FR-014, D-4): the start gate.** One `GraphGate`
  stands per threaded attempt, owned by `threadedSampleRun` beside the
  two barriers. The body of a context calls `awaitStart(slot)` after
  the start round and before the callable. The gate's release fact is
  the completion state of specs/020 FR-049: a dependency stands
  finished when its dispatched work node reads `DONE`, and the gate
  reads that state with acquire order, so everything the dependency's
  work did happens-before the release. The gate's condition variable
  carries the wake; the finishing context calls `nodeFinished(slot)`
  and notifies, and the predicate re-reads the completion states, so
  the notify is a hint and the state is the fact. A node with no
  dependency stands ready at the standing start. Among ready nodes the
  gate releases the lowest slot first: a ready node starts when no
  ready node stands at a lower slot. The gate owns no order fact and
  stores no order. Fan-out: one finished node drops the count of
  every node that depends on it alone. Fan-in: a node starts after the
  last of its dependencies finishes.
- **R-07 (FR-015, D-4 of research.md): the barrier arrival of a held
  context.** The
  gate stands in the body of each context, after the start round and
  before the stop round, part of neither round, beside the standing
  setup-failure skip at `source/harness/runner.cpp:2085`, the shape
  that already leaves every context out of the body and arrives at
  both rounds. `awaitStart` answers `run` or `skip`. A node that never
  starts (held by a failed dependency, stopped by the stop request)
  answers `skip`: the context arrives at the stop round with no body
  run and no measured window open, so no round strands (the hazard at
  `source/harness/runner.cpp:2030`). The context thread starts as the
  run starts it today, and the dispatch of every body index stands
  unchanged, the `threadRunner` ledger of FR-021 included.
- **R-08 (FR-016, FR-017, FR-018): the raw timed path.** The graph
  gates the start of a context and touches nothing inside it: no
  measured window opens, closes or shifts because of the graph, no
  barrier round moves inside a window, and the graph reads no clock.
  The work items stay `noexcept`, and the gate adds no error state to
  a work item. The gate reads no placement, plan, pin, bind, wait mode
  or recorder.
- **R-09 (FR-019, FR-020, FR-021, D-5): the settlement.** After the
  attempt the run reads the per-context outcomes the harness records:
  a measured row settles every node of its run `finished`, and a
  skipped row settles every node `finished`. When the merged row reads
  `FAILED`, the context whose body recorded an error settles its node
  `failed`, and a fixture setup failure settles the node at slot 0,
  the context that runs the setup half. The dependents of a failed
  node never start: the transitive closure holds with them, each
  settles `held`, and the record names each with the failed node that
  gates it as its reason, the nearest failed node on the dependency
  chain; the reason field names a failed node alone. When two nodes
  fail, each held node names the failed node that gates it. A node the
  stop request refuses settles `stopped`, the standing shape of
  specs/020 FR-050, and its dependents settle `stopped` with it; the
  record names each with no reason. The run finishes the nodes that
  stand on their own, and the exit status follows the standing
  run-failure status of specs/015. The status-2 shape stays reserved
  for what the run refuses before it starts.
- **R-10 (FR-022, D-7): the console block.** `printGraphReport` prints
  one block per threaded run directly after that run's `contexts=`
  line, beside `printContextDisclosure` (`source/harness/report.cpp:167`).
  The block's line shape:

  ```text
    graph nodes=a,b,c edges=a->b,b->c
    graph held=b(a)
    graph stopped=d
  ```

  The first line names the nodes in ascending slot order, the order
  FR-023 fixes, and the edges as `from->to` pairs in file order. The
  held line prints only when a node stands held, naming each held node
  with its gating failed node in parentheses, in ascending slot order.
  The stopped line names each stopped node in ascending slot order. The
  timed rows keep the bytes a graph-free run of the same benchmark gives.
- **R-11 (FR-005, FR-006, FR-023, FR-024, D-7): the record.** The JSON gains a
  top-level `graph` record standing after `precheck` and before
  `benchmarks` in the top-level `x-order`, the placement pattern the
  pre-check record set. The record carries one entry per threaded run
  in batch order; each entry names its run by the benchmark name, the
  key a reader folds the entry against the console block by, and
  carries that run's nodes in the ascending slot order FR-023 fixes,
  edges in file order, held nodes with
  their reasons and stopped nodes. The keys stand in
  `contracts/graph-record.md` and in `docs/schema/report-v1.json`; the
  schema minor takes `1.7`, the reservation the Spec identity section
  of spec.md states. `loadReport` reads the record back into the public
  `graph` member of `ReportData`. The record carries no timing figure.
  A run with no `--graph` writes no `graph` key, and its document
  matches master's bytes apart from the `schemaVersion` value and the
  `speedgun.version` value the version move carries.
- **R-12 (FR-011, FR-025, D-6): the seam.** One named detail seam
  `installTestGraph(std::optional<detail::Graph>)` installs a synthetic
  graph in place of the file read, the way `installTestOnlineCpus`
  replaces the online set and `installTestMemoryBinder` replaces the
  binder; a null install restores the file read. The entry point
  installs the graph it read once (FR-011) into the same slot
  `graphInput()`, and the runner reads the slot per threaded attempt.
  The tests drive every arm through synthetic graphs: a chain, a
  fan-out, a fan-in, a diamond, a cycle, an unsatisfiable dependency
  and a rejected file. The file arms run through planted files under a
  temporary directory. CI needs no hardware and no fixed node count.
- **R-13 (FR-003, FR-004, FR-005, D-8): the version, schema and
  `SOVERSION` record.** The project version takes `0.23.0`, the harness
  `SOVERSION` takes `14`, and the report schema minor takes `1.7`,
  each a reservation: at rebase the lane reads `CMakeLists.txt` and
  `source/harness/json_report.cpp` on master and takes the next free
  value, the rule specs/029 records (`specs/029-hunter-core/spec.md:44-49`).
  The reservation rule and the pull request #57 pairing stand in the
  Spec identity section of spec.md, which names the next free value of
  each at rebase. The
  graph record joins the public loaded-report struct, so the harness
  public ABI changes and its number rises by one; the archive
  `SOVERSION` stays `4`; no `SOVERSION` goes down; the check stands
  recorded beside the version move, in the commit that moves it. The
  top-level `x-order` moves in the same commit as the record keys.
- **R-14 (FR-030, FR-031, D-9): the evidence.** One script
  `tools/graph/acceptance.py` produces the acceptance record under
  `specs/037-execution-graphs/results/`: a fan-out and fan-in graph
  over the lane host's 16 cores, the printed node order, the
  figures beside it, the boot, the commands and the exit codes, the
  way specs/035 recorded the NPS evidence. The record names the host's
  16-core, 32-thread shape beside the 24-core, 48-thread record of
  specs/035. No CI job runs the script.
- **R-15 (FR-032, FR-033, FR-034, D-10): the docs.** The root
  `README.md` gains a graph section of at most 25 lines with the
  option row and the record rows in the quick-reference tables;
  `docs/pages/harness.md` gains the option and record detail beside
  the placement and pre-check sections; `example/docs/execution_graphs.cpp`
  with its `.expected` file records a graph run the shape gate folds;
  `tools/docs/example_drift.py` gains the section and owner rows.
- **R-16 (FR-026, FR-027, FR-028, FR-029): the suite rules.** No test
  sleeps and no test reads a clock: the start order comes from the
  injected graph. The suite passes under `taskset -c 0,1` and on a
  2-cpu host, the cpus named through the online-set seam, no test
  carrying a fixed node count. Coverage reads 100% lines and 100%
  branches with no `LCOV_EXCL` marker and no new `--ignore-errors`
  entry. No lambda stands inside an `SG_*` macro, and simdjson stands
  the only JSON reader the change uses.

### Risks

- **A stranded round is a hung run.** The gate stands between the
  rounds, and every arm answers `run` or `skip`, so every context
  arrives at both rounds. The chain, diamond, held and stopped
  scenarios each end with the attempt completing, and the suite runs
  under the sanitizer and tsan presets, where a strand shows as a
  hang the CI timeout names.
- **The start order of a live run depends on finish order.** Among
  independent nodes the release order follows finish order, so the live
  start order stands no fixed function of the file. FR-013 fixes the
  release rule and FR-023 fixes the printed order, so the printed bytes
  stand fixed. The tests assert the order through injected graphs with
  fixed iterations, where the finish order stands scripted by the graph's
  own shape.
- **The condition-variable wait runs on a context thread.** The gate
  waits on the standard condition variable. The inbox's wait modes
  serve the empty inbox alone, and the context loop owns no claim on
  the thread between items: the body is the item, and the standing
  barrier rounds already block the thread the same way. The tsan story
  stands in research.md.
- **The record joins the public struct.** The ABI change is the
  feature's only public growth, and the version commit records the
  check; the byte-identity scenario proves a graph-free run writes
  master's bytes apart from the two values FR-006 names.
- **The #57 and #58 collision.** The version, `SOVERSION`, schema and
  option values stand reservations; the rebase rule settles them, the lane
  records the version values in the version commit, and the row's commit
  records the option value it took.

### Logical view: one threaded attempt with a graph

```text
threadedSampleRun
  ├── startBarrier, stopBarrier            # standing (runner.cpp:2013)
  ├── GraphGate graph(gateGraph)           # the injected graph, wave 1 seam
  ├── dispatch every body index            # standing (FR-021 ledger)
  └── body(index) on the context thread
        ├── plan, recorder, state          # standing
        ├── fixture setup on ctx 0         # standing, before the round
        ├── startBarrier.arriveAndWait()   # every context arrives
        ├── gate.awaitStart(slot)
        │     ├── run:   entry.callable(state)   # the window opens inside
        │     ├── held:  skip, reason = gating failed node
        │     └── stopped: skip, dependents settle stopped
        ├── stopBarrier.arriveAndWait()    # every context arrives
        ├── gate.nodeFinished / nodeFailed # the release fact: FR-049 DONE
        └── fold, record                   # standing
  └── settlement: outcomes + gate words → the record entry
```

### Logical view: the reader pipeline

```text
--graph=<file>
  └── readGraphFile(path, contextCounts)
        ├── read the file            # FR-010 fault 1
        ├── simdjson parse           # FR-010 fault 2
        ├── checkReport(graph-v1.json) # FR-010 fault 3
        ├── names unique             # FR-010 fault 4
        ├── slot present             # FR-010 fault 5
        ├── slot in range per run    # FR-010 fault 6
        ├── deps name nodes          # FR-010 fault 7
        ├── no self-edge             # FR-010 fault 8
        ├── acyclic                  # FR-010 fault 9
        └── slot map bijective       # FR-010 fault 10
  first fault: one line, exit 2, no context starts
  the ten faults and their order stand in FR-010 of spec.md
```

### Physical view

The model, the reader and the gate each stand in one new translation
unit inside the harness archive the runner already links. The gate type
stands in the new detail header beside the model; the seam declaration
and the option field stand in `detail/internal.hpp`. The public header
gains the record types and the `ReportData` member alone. The schema
files stand under `docs/schema/`, and the standing checker reads both.

### Public API surface added

| Name | Shape | Contract |
| --- | --- | --- |
| `GraphRecord` | struct: `runs`, one entry per threaded run | the record the document carries; the struct carries the doxygen contract (R-11) |
| `GraphRunRecord` | struct: `benchmark`, `nodes`, `edges`, `held`, `stopped` | one threaded run's facts, named by the benchmark name (R-11) |
| `GraphNodeRecord` | struct: `name`, `slot`, `outcome` | one node and its outcome word (R-11) |
| `GraphEdgeRecord` | struct: `from`, `to` | one dependency pair (R-11) |
| `GraphHeldRecord` | struct: `node`, `reason` | one held node and the failed node that gates it (R-11) |
| `ReportData::graph` | `std::optional<GraphRecord>` | the record `loadReport` read back; empty for a document with no record (R-11) |

No other public name joins: the graph arrives through the option row,
and the lane adds no handle method (D-8).

### Version and `SOVERSION`

The version moves to `0.23.0` and the harness `SOVERSION` rises to `14`
(FR-003, FR-004, R-13), each a reservation under the rule of the Spec
identity section of spec.md. The archive `SOVERSION` stays `4`. The
graph record joins the public loaded-report struct, so the harness
public ABI changes, and the check stands recorded in the commit that
moves the version. The report schema minor moves to `1.7` in the writer's
commit, the commit that carries `source/harness/json_report.cpp` (T030).
The option value `297` is a reservation under the same rule: pull request
#58 takes `297` for its `os-noise` row, so the lane takes `298` when #58
lands first (FR-002).

## Test Plan

TDD, the waves of the module table in dependency order; every wave's
tests stand written and observed failing (red) before the code that
turns them green. The red step of each wave: the wave's scenarios run
against the tree without the wave's code, and each fails for its own
reason (a missing seam, a missing arm, a missing key), observed in the
test output before the code lands.

- **Wave 1 red**: the model test names `detail::Graph`, the words, the
  order rules and `installTestGraph`; it fails to build, the red step.
  Green: M1 lands. Gate: `ctest --test-dir build -R harness_graph_model_test`.
- **Wave 2 red**: the file test plants each FR-010 fault and asserts
  one line and status 2; the gate test installs chains, fan-outs,
  fan-ins, diamonds, held bodies, failures and stops through
  `installTestGraph` and asserts the start order, the barrier arrival
  and the words. Both fail before M2 and M4 land. Gate:
  `ctest --test-dir build -R harness_graph_file_test` and
  `ctest --test-dir build -R harness_graph_gate_test` under
  `taskset -c 0,1`.
- **Wave 3 red**: the CLI test drives the row, the bare form, the
  option-file source, the count refusals and the parse-once rule; the
  report test drives the console block, the record keys, the order, the
  read-back and the byte identity. Both fail before M3 and M5 land.
  Gate: `ctest --test-dir build -R "harness_(cli_graph|graph_report)_test"`.
- **Wave 4 red**: the report-shape and drift gates run the example;
  the coverage, sanitizer, tsan, tidy and prose gates run at the head;
  the acceptance script runs on the lane host. Gate: the full `ctest`
  matrix, then the wave 4 gate list.

### FR to test map

Every FR-001 to FR-034 names its test file, its scenario and the gate
that runs it. No FR lacks a test.

| FR | Test file | Scenario | Gate |
| --- | --- | --- | --- |
| FR-001 | `harness_cli_graph_test.cpp` | `graphRowPrintsHelpAndOptions` | `ctest -R harness_cli_graph_test` |
| FR-002 | `harness_cli_table_test.cpp` | `graphRowValueBlockAndUniqueness` | `ctest -R harness_cli_table_test` |
| FR-003 | `harness_cli_test.cpp` | `versionPrintsPlannedMinor` (the scenario reads the `VERSION` of the root `CMakeLists.txt`) | `ctest -R harness_cli_test`; the ABI check rides the version commit |
| FR-004 | `harness_graph_report_test.cpp` | `reportDataGraphMemberRaisesAbi` (the public struct grew; the `SOVERSION` check stands in the version commit) | `ctest -R harness_graph_report_test`; `cmake --preset=ci-rocky` |
| FR-005 | `harness_graph_report_test.cpp` | `schemaCarriesGraphKeysAndOrder` | `ctest -R harness_graph_report_test`; the schema check |
| FR-006 | `harness_graph_report_test.cpp` | `noGraphBytesMatchMaster` | `ctest -R harness_graph_report_test` |
| FR-007 | `harness_graph_file_test.cpp` | `readerChecksShapeAgainstSchemaFile` | `ctest -R harness_graph_file_test` |
| FR-008 | `harness_graph_model_test.cpp` | `nodeNamesSlotAndDeps`; `harness_graph_file_test.cpp`: `fileCarriesNoExtraKeys` | `ctest -R harness_graph_model_test` and `ctest -R harness_graph_file_test` |
| FR-009 | `harness_cli_graph_test.cpp` | `graphEqualsFileReads`, `bareGraphTakesOptionFilePath`, `bareGraphNoPathRefuses` | `ctest -R harness_cli_graph_test` |
| FR-010 | `harness_graph_file_test.cpp` | `unreadableFileRefuses`, `parserRejectRefuses`, `schemaShapeBreakRefuses`, `repeatedNameRefuses`, `missingSlotRefuses`, `slotOutOfRangeRefuses`, `edgeNamesNoNodeRefuses`, `selfEdgeRefuses`, `cycleRefuses`, `unsatisfiableDependencyRefuses`, `firstFaultPrintsAlone` | `ctest -R harness_graph_file_test` |
| FR-011 | `harness_cli_graph_test.cpp` | `graphParsesOnceBeforeRun` | `ctest -R harness_cli_graph_test` |
| FR-012 | `harness_cli_graph_test.cpp` | `countMismatchRefuses`, `noThreadedRunRefuses`, `singleContextTakesNoGraph` | `ctest -R harness_cli_graph_test` |
| FR-013 | `harness_graph_gate_test.cpp` | `chainStartsInOrder`, `independentStartLowestSlotFirst`; `harness_graph_model_test.cpp`: `topologicalOrderRespectsSlots` | `ctest -R harness_graph_model_test` and `ctest -R harness_graph_gate_test` |
| FR-014 | `harness_graph_gate_test.cpp` | `fanOutReleasesAll`, `fanInStartsAfterLast` | `ctest -R harness_graph_gate_test` |
| FR-015 | `harness_graph_gate_test.cpp` | `heldContextArrivesBothRounds` | `ctest -R harness_graph_gate_test` |
| FR-016 | `harness_graph_gate_test.cpp` | `gateOpensNoWindow`; the graph units carry no time source | `ctest -R harness_graph_gate_test`; `test/time_source_gate.sh` |
| FR-017 | `harness_graph_gate_test.cpp` | `placementBytesUnchangedBesideGraph` | `ctest -R harness_graph_gate_test` |
| FR-018 | `harness_graph_gate_test.cpp` | `noexceptItemConceptHolds` | `ctest -R harness_graph_gate_test` |
| FR-019 | `harness_graph_model_test.cpp`, `harness_graph_gate_test.cpp` | `fourOutcomeWords`, `measuredRowSettlesAllFinished`, `skippedRowSettlesFinished`, `bodyErrorSettlesNodeFailed`, `setupFailureSettlesSlotZero`, `stopSettlesStoppedWithDependents` | `ctest -R harness_graph_model_test` and `ctest -R harness_graph_gate_test` |
| FR-020 | `harness_graph_gate_test.cpp` | `failedDependentsHoldWithReason`, `twoFailuresNameTheirGater` | `ctest -R harness_graph_gate_test` |
| FR-021 | `harness_graph_gate_test.cpp` | `independentFinishAndStatusFollowsRunFailure` | `ctest -R harness_graph_gate_test` |
| FR-022 | `harness_graph_report_test.cpp` | `consoleBlockBesideContextsLine`, `blockOncePerRun` | `ctest -R harness_graph_report_test` |
| FR-023 | `harness_graph_report_test.cpp` | `recordKeysOrderAndReadBack`, `nodeOrderAscendingSlots`, `reasonNamesFailedNode` | `ctest -R harness_graph_report_test`; `ctest -R harness_json` round trip |
| FR-024 | `harness_graph_report_test.cpp` | `recordCarriesNoTiming` | `ctest -R harness_graph_report_test` |
| FR-025 | `harness_graph_model_test.cpp`, `harness_graph_gate_test.cpp` | `seamInstallsAndClears`, `graphInputAnswersInstall`, `chainStartsInOrder`, `fanOutReleasesAll`, `fanInStartsAfterLast`, `diamondStartsAfterBoth`; the cycle, unsatisfiable and rejected-file arms stand in `harness_graph_file_test.cpp` | `ctest -R harness_graph_file_test`, `ctest -R harness_graph_gate_test`, `ctest -R harness_graph_model_test` |
| FR-026 | every graph test | no sleep, no clock call, no `<chrono>` include | `test/time_source_gate.sh`; `ctest -R harness_graph` |
| FR-027 | `harness_graph_gate_test.cpp` | `suitePassesOnTwoCpus` (cpus through `installTestOnlineCpus`, no fixed node count) | `taskset -c 0,1 ctest --test-dir build` |
| FR-028 | the coverage run and the DBC gate | 100.0% lines and branches on every new unit, no marker, and the DBC matrix complete | the `coverage` preset run with `PERL5LIB=/home/archerc/perl5/lib/perl5`, then `cmake --build build -t dbc-gate` |
| FR-029 | standing: `harness_capture_macro_test.cpp` needs no change; the tidy run of T042 | no lambda inside an `SG_*` macro; simdjson only | `cmake --preset=ci-ubuntu` and `cmake --build build` |
| FR-030 | `specs/037-execution-graphs/results/` | the acceptance record names the boot, commands, exit codes, the printed node order, figures, the 16-core shape | the script run on the lane host |
| FR-031 | `tools/graph/acceptance.py` | the script writes the record; no CI job names it | the script run; the CI config inspection |
| FR-032 | `README.md` rows | the section, the option row, the record rows | `ctest -R example_drift` |
| FR-033 | `docs/pages/harness.md` | the option and record detail rows | `ctest -R example_drift` |
| FR-034 | `example/docs/execution_graphs.cpp` + `.expected` | the recorded graph run folds against the binary | `ctest -R example_drift` and `ctest -R example_run`; `ctest -R report_shape` |

### Coverage plan

Every new branch stands named and driven:

- the reader: each of the ten FR-010 arms taken once and passed once by
  the clean file; the bare-path, option-file-path and no-path arms of
  FR-009; the per-run count arms of FR-012 (match, mismatch, no
  threaded run, single-context batch).
- the gate: release, lowest-slot block, held spread, failed spread,
  stop arm, the `run` and `skip` answers, the notify predicate taken
  both ways (a wake with the predicate false stands driven by the
  diamond's second dependency).
- the settlement: measured, skipped, failed, setup failure, stop, the
  two-failure reason choice.
- the disclosure: block printed, block absent, held line, stopped
  line, record written, record absent, read-back present and absent.
- the writer and reader arms: every key present, a missing key, a
  wrong kind, the `x-order` check, the byte-identity fold.

No `LCOV_EXCL` marker joins, and no new `--ignore-errors` entry joins.

### Suite rules

- No test sleeps and no test reads a clock (FR-026); the start order
  comes from the injected graph, and `test/time_source_gate.sh` stays
  green over every new unit.
- The suite passes under `taskset -c 0,1` and on a 2-cpu host
  (FR-027): the cpus come through `installTestOnlineCpus`, the graphs
  come through `installTestGraph`, and no test carries a fixed node
  count.
- The sanitizer runs (`ci-sanitize`) and the thread-sanitizer run
  (`ci-tsan`) run the graph suite at the head; the gate's atomics
  story stands in research.md.
- The coverage run reads 100.0% lines and 100.0% branches (FR-028).
- `cmake --preset=ci-rocky` and `cmake --build build` and
  `ctest --test-dir build --output-on-failure` stand green at every
  wave head; the release check runs `cmake --preset=ci-ubuntu` once
  for the feature.

## Constitution Check

*Re-check after Phase 1 design.*

| Principle | Status |
| --- | --- |
| I. Standard-first | PASS: the gate rides the standing barriers, the FR-049 completion state and the standard condition variable; the reader rides the vendored simdjson |
| II. Design by contract | PASS: the seam, the reader, the gate and the record carry their contracts; the items stay `noexcept` |
| III. R-DCUT | PASS: FR-001 to FR-034 map to R-01 to R-16, the module table and the Test Plan waves; TDD stands |
| IV. Documentation | PASS: the docs artifacts stand in the FR set and the example carries its `.expected` |
| V. Style | PASS: the new names follow V.1; the file names follow N-9 |
| VI. Coverage | PASS: the coverage plan names every new branch and its driving scenario |
| VII. Performance | PASS: the gate stands outside every measured window; the graph reads no clock; the timed path keeps its bytes; the timed path is the designated critical path, the measured window of a context; the baseline gate stands under the constitution's Open deferrals |
| VIII. CI gates | PASS: the wave 4 list runs every hard gate at the head |
| IX. Spec-driven | PASS: the spec, the plan, the research, the data model, the contracts and the quickstart stand in the feature directory |
| X. Anti-slop | PASS: one row, one reader, one gate, one record, one seam; the module table adds no abstraction serving one caller |
| XI. Prose | PASS at the head: the gate clears the artifacts with the branch's commits |

**Complexity Tracking stays empty**: no gate fails and no violation
needs justification.
