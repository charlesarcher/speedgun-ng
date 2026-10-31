# Tasks: Execution Graphs

**Input**: Design documents from `/specs/037-execution-graphs/`

**Prerequisites**: [plan.md](./plan.md) (required), [spec.md](./spec.md)
(required for user stories), [research.md](./research.md),
[data-model.md](./data-model.md), [contracts/](./contracts/),
[quickstart.md](./quickstart.md)

**Tests**: TDD mode (plan.md, Execution mode). In every module the test
task stands written, run, and observed failing before the code task
that turns it green. Each test task names its red step: run it, observe
the failure, record the failure line in the task record.

**Organization**: tasks stand grouped by the eight modules of the plan's
module table, in the plan's four waves. The waves carry disjoint file
sets, so one worker takes one module and touches no other worker's
file. Each task names its module, its wave, its parallel set, the exact
files it creates or edits, its covering test with its scenario, the
verification command that proves it, and the FRs it closes.

**Gate tasks**: the hard gates of Principle VIII run at the feature
head, the tip commit of the branch. They stand as Phase 7, the phase
that runs once every module has landed.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: the task runs in parallel with the tasks its parallel set
  names: disjoint files, no dependency between them, and the set omits
  the task itself. The two pairs that share `test/CMakeLists.txt` run in
  task order, and the shared-file table names them
- **[Story]**: the user story the task serves (`US1` to `US6`)
- Every description names the exact file path and the FRs it closes
- Each task carries five labeled lines: `Module` with its wave and
  parallel set, `Test` with its file and scenario, `Verify` with its
  command, `Depends`, and `Commit` with its title

## Shared files and labeled blocks

Twelve files stand outside the module table's disjoint sets. Each task
that edits one of them writes only inside its own labeled block, and the
block lands in the commit the task names. A test file that several tasks
append to stands listed with its tasks in task order, and each task
lands in its own commit.

| File | Block label | Tasks | Commit that carries it |
| --- | --- | --- | --- |
| `test/CMakeLists.txt` | `Execution graphs: model (M1)` | T001 | T001 |
| `test/CMakeLists.txt` | `Execution graphs: file (M2)` | T005 | T005 |
| `test/CMakeLists.txt` | `Execution graphs: gate (M4)` | T010 | T010 |
| `test/CMakeLists.txt` | `Execution graphs: cli (M3)` | T020 | T020 |
| `test/CMakeLists.txt` | `Execution graphs: record (M5)` | T024 | T024 |
| `test/CMakeLists.txt` | `Execution graphs: example (M7)` | T035 | T035 |
| `docs/schema/report-v1.json` | `Execution graphs (M5)` | T029 | T029 |
| `README.md` | `Execution graphs (M7)` | T033 | T033 |
| `docs/pages/harness.md` | `Execution graphs (M7)` | T034 | T034 |
| `tools/docs/example_drift.py` | `Execution graphs (M7)` | T036 | T036 |
| `test/source/harness_graph_model_test.cpp` | one file, appended in task order | T001, T003 | one commit per task |
| `test/source/harness_graph_file_test.cpp` | one file, appended in task order | T005 to T007 | one commit per task |
| `test/source/harness_graph_gate_test.cpp` | one file, appended in task order | T010, T011 to T016 | one commit per task |
| `test/source/harness_cli_graph_test.cpp` | one file, appended in task order | T020, T021 | one commit per task |
| `test/source/harness_cli_table_test.cpp` | standing file, appended in task order | T019, T051 | one commit per task |
| `test/source/harness_graph_report_test.cpp` | one file, appended in task order | T024 to T027 | one commit per task |
| `test/source/harness_cli_test.cpp` | standing file, appended in task order | T052 | T052 |

Two pairs share `test/CMakeLists.txt` in labeled blocks and run in task
order: T005 with T010, and T020 with T024. Each block lands in the commit
of its own task.

`source/harness/detail/internal.hpp` stands named by the plan as a
shared file and M1 owns every line this feature adds, in wave 1; no
later module edits it. `source/harness/detail/graph.hpp` stands owned
by M1 alone, and M4 defines the gate in its own unit and never edits
the header.

## Verification commands

The tasks use the project's own commands. The daily loop drives the
release developer-mode build:

- `cmake --preset=ci-rocky`, `cmake --build build -j 24`,
  `ctest --test-dir build --output-on-failure`
- `cmake --preset=ci-ubuntu`, `cmake --build build`: the release build
  with clang-tidy and cppcheck wired in, once per feature
- `cmake --preset=ci-sanitize`, `cmake --build build/sanitize -j 24`,
  `ctest --test-dir build/sanitize --output-on-failure`
- `cmake --preset=ci-tsan`, `cmake --build build/tsan -j 24`,
  `ctest --test-dir build/tsan --output-on-failure`
- `cmake --preset=ci-coverage`, `cmake --build build/coverage -j 24`,
  `ctest --test-dir build/coverage -j 1`,
  `cmake --build build/coverage -t coverage`
- `ctest --test-dir build -R <name>` for one executable, and
  `taskset -c 0,1 ctest --test-dir build --output-on-failure`
- `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake`,
  `cmake -P cmake/spell.cmake`, `cmake -P cmake/prose-lint.cmake`

The lcov run needs `PERL5LIB=/home/archerc/perl5/lib/perl5` exported.
The suite runs `ctest -j 2` and passes under `taskset -c 0,1` and on a
2-cpu host (FR-027).

## Wave layout

| Wave | Modules | Tasks | Parallel sets |
| --- | --- | --- | --- |
| 1 | M1 `graph-model` | T001 to T004 | T002 with T003 (T001 runs first: the red run stands recorded) |
| 2 | M2 `graph-reader`, M4 `start-gate` | T005 to T018 | T006 to T009 run beside T010 to T018; T005 and T010 land their `test/CMakeLists.txt` blocks in task order; each module's tasks run in task order |
| 3 | M3 `cli-row`, M5 `record-disclosure` | T019 to T031, T051 | T019, T021 to T023 run beside T024 to T031; T020 and T024 land their `test/CMakeLists.txt` blocks in task order; each module's tasks run in task order; T051 runs beside T024 to T031 |
| 4 | M6 `version-abi`, M7 `docs-example`, M8 `evidence` | T032 to T038, T052 | T032 runs first; T033, T034 and T052 run beside each other; T035 runs after T032, beside T037, T038 and T052; T036 runs after T033, T034 and T035; T037 runs beside T032, T033, T034, T035, T036 and T052; T038 runs after T037, beside T032, T033, T034, T035, T036 and T052 |
| head | the gates, the records | T039 to T050 | T039 to T047, T050 |

A wave closes when its gate stands green: wave 1
`ctest --test-dir build -R harness_graph_model_test`; wave 2
`ctest --test-dir build -R harness_graph_file_test` and
`ctest --test-dir build -R harness_graph_gate_test` under
`taskset -c 0,1`; wave 3
`ctest --test-dir build -R "harness_(cli_graph|graph_report)_test"`;
wave 4 the full `ctest` matrix and the Phase 7 gates.
---

## Phase 1: Wave 1, module M1 `graph-model`

**Purpose**: the model, the four words, the order rules, the gate type,
  the seam and the slot stand alone; every later module codes against
  them.

**Independent test**: `harness_graph_model_test` passes with no run
  started, the pure rules read from the model alone.

- [X] T001 [US1] Register `harness_graph_model_test` in
  `test/CMakeLists.txt` under the block `Execution graphs: model (M1)`, then
  write the failing model tests in
  `test/source/harness_graph_model_test.cpp`: `nodeNamesSlotAndDeps` states
  the name, the slot counted from `0`, and the dependency list of one node;
  `fourOutcomeWords` states the four words `finished`, `failed`, `held`,
  `stopped` and that one node settles exactly one;
  `topologicalOrderRespectsSlots` states the start order of a chain and of
  two disconnected components, the lower slot first; `slotMapBijection`
  states the slot-to-node map and names the fault when two nodes name one
  slot (FR-008, FR-013, FR-019, FR-020, R-04, R-06). Red step:
  `cmake --build build -j 24` fails to build the executable, because the
  include of `detail/graph.hpp` fails and `detail::Graph` stands undeclared
  behind it; the failure line stands recorded in the task record before
  T002 lands.
  Module M1 graph-model, wave 1, parallel set: none (T002 runs after this
  task: the red run stands recorded).
  Test: `test/source/harness_graph_model_test.cpp`: `nodeNamesSlotAndDeps`,
  `fourOutcomeWords`, `topologicalOrderRespectsSlots`, `slotMapBijection`.
  Verify: the red step `cmake --build build -j 24`, then
  `ctest --test-dir build -R harness_graph_model_test`.
  Depends: none.
  Commit: `Harness: Add the graph model tests`, carrying the
  `test/CMakeLists.txt` block.

- [X] T002 [P] [US1] Declare the model in `source/harness/detail/graph.hpp`:
  `detail::GraphNode` with `name`, `slot`, `deps`; `detail::Graph` with the
  node list, the edge list in file order and the slot map; the record
  lists nodes in the ascending slot order FR-023 fixes, so the model
  stores no order; `detail::NodeOutcome` with the four words;
  `detail::GraphGate` declared with a constructor over a graph,
  `awaitStart`, `nodeFinished`, `nodeFailed`, `settleStopped`, `outcomes`,
  defined by M4 in its own unit;
  the `\pre`, `\post` and `\invariant` comments of Principle II on every member; no lambda
  inside any `SG_*` macro (FR-008, FR-013, FR-019, FR-020, FR-029, R-04,
  R-06). The task turns T001 green.
  Module M1 graph-model, wave 1, parallel set: T003 (T001 runs first: the
  red run stands recorded).
  Test: the scenarios of T001.
  Verify: `cmake --build build -j 24` compiles the header into every target
  that includes it. The model executable links and passes from T004, the task
  that defines `startOrder` and `slotMap`.
  Depends: T001 (the red run stands recorded).
  Commit: `Harness: Declare the graph model`.

- [X] T003 [P] [US1] Append the seam scenarios to
  `test/source/harness_graph_model_test.cpp`: `seamInstallsAndClears`
  installs a synthetic graph through `installTestGraph` and an empty install
  restores the file read, the clear-on-null shape of `installTestLayout` at
  `source/harness/topology.cpp:336`; `graphInputAnswersInstall` answers
  the installed graph when one stands, and the graph the entry point read
  once when none stands (FR-011, FR-025, R-12, D-7 of research.md). Red
  step: `cmake --build build -j 24`
  fails, because `installTestGraph` stands undeclared in
  `source/harness/detail/internal.hpp`; the failure line stands recorded
  before T004 lands.
  Module M1 graph-model, wave 1, parallel set: T002.
  Test: `test/source/harness_graph_model_test.cpp`: `seamInstallsAndClears`,
  `graphInputAnswersInstall`.
  Verify: the red step `cmake --build build -j 24`, then
  `ctest --test-dir build -R harness_graph_model_test`.
  Depends: T001 (one file, appended in task order).
  Commit: `Harness: Add the graph seam tests`.

- [X] T004 [US1] Define the model rules in `source/harness/graph.cpp`: the
  order rules over a chain, a fan-out, a fan-in, a diamond and a
  disconnected component, the lowest ready slot first; the slot map as the
  bijection from the run's context indexes to the nodes, naming the node and
  the slot when two nodes name one slot; the seam definition
  `installTestGraph` and the slot `graphInput()`. Add to
  `source/harness/detail/internal.hpp` the seam declaration beside
  `installTestMemoryBinder` at `:802`, the slot, and the
  `RunOptions.graphPath` field. M1 owns every line of `internal.hpp` this
  feature adds, and no later module edits it (FR-008, FR-011, FR-013,
  FR-020, FR-025, R-04, R-12). The task turns T003 green.
  Module M1 graph-model, wave 1, parallel set: none (the wave's last code
  task).
  Test: the scenarios of T001 and T003.
  Verify: `ctest --test-dir build -R harness_graph_model_test`, then
  `ctest --test-dir build --output-on-failure`.
  Depends: T002, T003.
  Commit: `Harness: Define the graph model and the seam`.
---

## Phase 2: Wave 2, module M2 `graph-reader`

**Purpose**: the file, its schema file and the ten refusal arms stand
  at the reader.

**Independent test**: each planted file under a temporary directory
  prints one line and exits 2 before any context starts, and the clean
  chain file passes every rule (spec US3).

- [X] T005 [P] [US3] Register `harness_graph_file_test` in
  `test/CMakeLists.txt` under the block `Execution graphs: file (M2)`, then
  write the failing shape-arm tests in
  `test/source/harness_graph_file_test.cpp` over planted files under a
  temporary directory: `unreadableFileRefuses` plants no file and names the
  path; `parserRejectRefuses` plants a truncated document and names the path
  and the fault; `schemaShapeBreakRefuses` plants a key the schema admits
  never, a key out of the `x-order`, and a `slot` spelled as a string,
  naming the key, the order break and the kind;
  `readerChecksShapeAgainstSchemaFile` states that the shape check of
  `docs/schema/graph-v1.json` finishes before the node rules; each arm
  asserts one line, exit status 2, and no context started (FR-007, FR-010
  arms 1 to 3, SC-003, R-03, D-1 of research.md). Red step:
  `cmake --build build -j 24` fails, because `detail::readGraphFile` stands
  undeclared; the failure line stands recorded before T009 lands.
  Module M2 graph-reader, wave 2, parallel set: T011 to T018 (M4 owns
  disjoint files; T010 appends to `test/CMakeLists.txt` in task order).
  Test: `test/source/harness_graph_file_test.cpp`: `unreadableFileRefuses`,
  `parserRejectRefuses`, `schemaShapeBreakRefuses`,
  `readerChecksShapeAgainstSchemaFile`.
  Verify: the red step `cmake --build build -j 24`, then
  `ctest --test-dir build -R harness_graph_file_test`.
  Depends: M1 (T004).
  Commit: `Harness: Add the graph file shape tests`, carrying the
  `test/CMakeLists.txt` block.

- [X] T006 [P] [US3] Append the node-rule arm tests to
  `test/source/harness_graph_file_test.cpp`: `repeatedNameRefuses` names the
  repeated name; `missingSlotRefuses` names the node;
  `slotOutOfRangeRefuses` names the node and the slot against the run's
  context count; `edgeNamesNoNodeRefuses` names the edge; `selfEdgeRefuses`
  names the node. Each arm plants its file, asserts one line, exit status 2,
  and no context started (FR-010 arms 4 to 8, SC-003, R-03). Red step: each
  arm fails at the tree without the reader, and every failure line stands
  recorded in the task record before T009 lands.
  Module M2 graph-reader, wave 2, parallel set: T010 to T018 (M4 owns
  disjoint files).
  Test: `test/source/harness_graph_file_test.cpp`: `repeatedNameRefuses`,
  `missingSlotRefuses`, `slotOutOfRangeRefuses`, `edgeNamesNoNodeRefuses`,
  `selfEdgeRefuses`.
  Verify: `ctest --test-dir build -R harness_graph_file_test` (each arm red
  before T009).
  Depends: T005 (one file, appended in task order).
  Commit: `Harness: Add the graph node-rule refusal tests`.

- [X] T007 [P] [US3] Append the last file arms to
  `test/source/harness_graph_file_test.cpp`: `cycleRefuses` plants two nodes
  naming each other and names the cycle; `unsatisfiableDependencyRefuses`
  plants two nodes naming one slot and names the node and the slot;
  `firstFaultPrintsAlone` plants a file with a repeated name beside a cycle
  and asserts the repeated-name line prints alone; `fileCarriesNoExtraKeys`
  states the clean file carries no key beside `nodes`, `name`, `slot`,
  `deps`; the clean chain file passes every rule and reaches the gate
  (FR-008, FR-010 arms 9 and 10, SC-003, R-03, D-2 of research.md). Red
  step: each arm fails at the tree without the reader, and every failure
  line stands recorded in the task record before T009 lands.
  Module M2 graph-reader, wave 2, parallel set: T010 to T018 (M4 owns
  disjoint files).
  Test: `test/source/harness_graph_file_test.cpp`: `cycleRefuses`,
  `unsatisfiableDependencyRefuses`, `firstFaultPrintsAlone`,
  `fileCarriesNoExtraKeys`.
  Verify: `ctest --test-dir build -R harness_graph_file_test` (each arm red
  before T009).
  Depends: T005 (one file, appended in task order).
  Commit: `Harness: Add the cycle and order refusal tests`.

- [X] T008 [P] [US3] Write `docs/schema/graph-v1.json`: the top level with
  the one key `nodes`, the node object with `name`, `slot`, `deps`, the
  value kinds the data model states, `additionalProperties: false` at both
  levels, the description naming the file, and the normative `x-order` lists
  `nodes` at the top level and `name`, `slot`, `deps` in the node object,
  the shape `docs/schema/compare-v1.json` names for the comparison document
  (FR-007, FR-008, R-03).
  Module M2 graph-reader, wave 2, parallel set: T010 to T018 (T005 runs
  first and T009 runs after this task, on one file).
  Test: `test/source/harness_graph_file_test.cpp`:
  `readerChecksShapeAgainstSchemaFile`, `schemaShapeBreakRefuses`,
  `fileCarriesNoExtraKeys`.
  Verify: `ctest --test-dir build -R harness_graph_file_test`, then
  `ctest --test-dir build -R harness_json_schema_test`.
  Depends: T005 (the red run stands recorded).
  Commit: `Docs: Name the graph file schema`.

- [X] T009 [P] [US3] Implement `detail::readGraphFile(path, contextCounts)` in
  `source/harness/graph_file.cpp`, answering
  `std::expected<detail::Graph, std::string>` with the one-line fault: read
  the file, parse with the vendored simdjson, check the document against
  `docs/schema/graph-v1.json` through the standing `checkReport` at
  `source/harness/json_load.cpp:2151`, then run the node rules in the FR-010
  order, the first fault alone. simdjson stands the only JSON reader the
  change uses (FR-007, FR-009, FR-010, FR-029, R-03, D-1 of research.md).
  The task turns T005, T006 and T007 green.
  Module M2 graph-reader, wave 2, parallel set: T010 to T018 (M4 owns
  disjoint files).
  Test: every scenario of T005, T006 and T007.
  Verify: `ctest --test-dir build -R harness_graph_file_test`, then
  `ctest --test-dir build --output-on-failure`.
  Depends: T005, T006, T007, T008.
  Commit: `Harness: Read the graph file`.
---

## Phase 3: Wave 2, module M4 `start-gate`

**Purpose**: the gate orders the starts of one threaded run between
  the two barrier rounds, settles the four words, and strands no round.

**Independent test**: an injected chain, fan-out, fan-in and diamond
  over installed cpus start in the graph's order, and the held, failed
  and stopped arms arrive at both rounds (spec US1, US4).

- [X] T010 [P] [US1] Register `harness_graph_gate_test` in
  `test/CMakeLists.txt` under the block `Execution graphs: gate (M4)`, then
  write the failing start-order tests in
  `test/source/harness_graph_gate_test.cpp` over graphs installed through
  `installTestGraph` and cpus installed through `installTestOnlineCpus`:
  `chainStartsInOrder` starts `a`, `b`, `c` over three contexts and states
  that each gate saw its dependency finished;
  `independentStartLowestSlotFirst` starts two nodes with no dependency
  between them and states the gate releases the lowest ready slot first
  (FR-013, FR-025, SC-001, R-06, D-3 of
  research.md). Red step: `cmake --build build -j 24` fails, because
  `detail::GraphGate` stands declared never defined; the failure line stands
  recorded before T017 lands.
  Module M4 start-gate, wave 2, parallel set: T006 to T009 (M2 owns
  disjoint files; T005 appends to `test/CMakeLists.txt` in task order).
  T011 to T016 append to the same test file in task order, and T017 and
  T018 follow them; none of the pairs runs in parallel.
  Test: `test/source/harness_graph_gate_test.cpp`: `chainStartsInOrder`,
  `independentStartLowestSlotFirst`.
  Verify: the red step `cmake --build build -j 24`, then
  `ctest --test-dir build -R harness_graph_gate_test`.
  Depends: M1 (T004).
  Commit: `Harness: Add the graph start-order tests`, carrying the
  `test/CMakeLists.txt` block.

- [X] T011 [P] [US1] Append the fan-out, fan-in and diamond tests to
  `test/source/harness_graph_gate_test.cpp`: `fanOutReleasesAll` releases
  both dependents of one finished source; `fanInStartsAfterLast` starts one
  node after the last of its two dependencies finishes;
  `diamondStartsAfterBoth` starts `d` after `b` and `c`, drives the gate
  predicate with a wake whose predicate reads false (the second dependency
  of the diamond), and states the ascending slot order of `b` and `c`
  (FR-014, FR-013, SC-002, R-06). Red step: the scenarios fails at the tree
  without the gate, and every failure line stands recorded in the task
  record before T017 lands.
  Module M4 start-gate, wave 2, parallel set: T005 to T009 (M2 owns disjoint
  files).
  Test: `test/source/harness_graph_gate_test.cpp`: `fanOutReleasesAll`,
  `fanInStartsAfterLast`, `diamondStartsAfterBoth`.
  Verify: `ctest --test-dir build -R harness_graph_gate_test` (red before
  T017).
  Depends: T010 (one file, appended in task order).
  Commit: `Harness: Add the fan-out and fan-in tests`.

- [X] T012 [P] [US1] Append the barrier and raw-path tests to
  `test/source/harness_graph_gate_test.cpp`: `heldContextArrivesBothRounds`
  states that a context whose node never starts arrives at the start round
  and the stop round with no body run and no measured window open, the
  hazard of `source/harness/runner.cpp:2030`; `gateOpensNoWindow` states
  that no window opens, closes or shifts because of the gate;
  `placementBytesUnchangedBesideGraph` states the placement, pin, bind, wait
  mode, plan and recorder bytes of specs/025 and specs/035 stand unchanged
  beside a graph; `noexceptItemConceptHolds` states the work items stay
  `noexcept` and the gate adds no error state (FR-015, FR-016, FR-017,
  FR-018, R-07, R-08, D-4 of research.md). Red step: the scenarios fails at
  the tree without the runner wiring, and every failure line stands recorded
  in the task record before T018 lands.
  Module M4 start-gate, wave 2, parallel set: T005 to T009 (M2 owns disjoint
  files).
  Test: `test/source/harness_graph_gate_test.cpp`:
  `heldContextArrivesBothRounds`, `gateOpensNoWindow`,
  `placementBytesUnchangedBesideGraph`, `noexceptItemConceptHolds`.
  Verify: `ctest --test-dir build -R harness_graph_gate_test` (red before
  T018), then `ctest --test-dir build -R time_source_gate`.
  Depends: T010 (one file, appended in task order).
  Commit: `Harness: Add the barrier and raw path tests`.

- [X] T013 [P] [US4] Append the settlement tests to
  `test/source/harness_graph_gate_test.cpp`: `measuredRowSettlesAllFinished`
  settles every node of a measured run `finished`;
  `skippedRowSettlesFinished` settles every node of a skipped run `finished`
  and starts every dependent; `bodyErrorSettlesNodeFailed` settles the node
  of the context whose body recorded an error `failed` and the other nodes
  `finished`; `setupFailureSettlesSlotZero` settles the node at slot `0` on
  a fixture setup failure (FR-019, SC-004, R-09, D-5 of the spec). Red step:
  the scenarios fails at the tree without the gate, and every failure line
  stands recorded in the task record before T017 lands.
  Module M4 start-gate, wave 2, parallel set: T005 to T009 (M2 owns disjoint
  files).
  Test: `test/source/harness_graph_gate_test.cpp`:
  `measuredRowSettlesAllFinished`, `skippedRowSettlesFinished`,
  `bodyErrorSettlesNodeFailed`, `setupFailureSettlesSlotZero`.
  Verify: `ctest --test-dir build -R harness_graph_gate_test` (red before
  T017).
  Depends: T010 (one file, appended in task order).
  Commit: `Harness: Add the node settlement tests`.

- [X] T014 [P] [US4] Append the held path to
  `test/source/harness_graph_gate_test.cpp`:
  `failedDependentsHoldWithReason` holds the transitive closure of a failed
  node's dependents, starts none of them, and names each with the nearest
  failed node on its chain as its reason; `twoFailuresNameTheirGater` fails
  two nodes and names for each held node the failed node that gates it; a
  node whose dependency stands held settles `held` too (FR-020, FR-015,
  SC-004, R-09). Red step: the scenarios fails at the tree without the gate,
  and every failure line stands recorded in the task record before T017
  lands.
  Module M4 start-gate, wave 2, parallel set: T005 to T009 (M2 owns disjoint
  files).
  Test: `test/source/harness_graph_gate_test.cpp`:
  `failedDependentsHoldWithReason`, `twoFailuresNameTheirGater`.
  Verify: `ctest --test-dir build -R harness_graph_gate_test` (red before
  T017).
  Depends: T010 (one file, appended in task order).
  Commit: `Harness: Add the held-node path tests`.

- [X] T015 [P] [US4] Append the stopped path to
  `test/source/harness_graph_gate_test.cpp`:
  `stopSettlesStoppedWithDependents` settles a node the stop request refuses
  `stopped`, the standing shape of specs/020 FR-050, and settles its
  dependents `stopped` with it; the record's reason rule stands at
  `reasonNamesFailedNode` in T025;
  `independentFinishAndStatusFollowsRunFailure` finishes the nodes that
  stand on their own and reads the standing run-failure status of specs/015,
  the status-2 shape standing reserved for what the run refuses before it
  starts (FR-019, FR-021, SC-004, R-09). Red step: the scenarios
  fails at the tree without the gate, and every failure line stands recorded
  in the task record before T017 lands.
  Module M4 start-gate, wave 2, parallel set: T005 to T009 (M2 owns disjoint
  files).
  Test: `test/source/harness_graph_gate_test.cpp`:
  `stopSettlesStoppedWithDependents`,
  `independentFinishAndStatusFollowsRunFailure`.
  Verify: `ctest --test-dir build -R harness_graph_gate_test` (red before
  T017).
  Depends: T010 (one file, appended in task order).
  Commit: `Harness: Add the stopped-node path tests`.

- [X] T016 [P] [US1] Append the suite-rule test to
  `test/source/harness_graph_gate_test.cpp`: `suitePassesOnTwoCpus` names
  the cpus through `installTestOnlineCpus` at
  `source/harness/topology.cpp:352`, carries no fixed node count, and runs a
  chain and a diamond over the two-cpu shape (FR-026, FR-027, SC-008, R-16).
  Red step: the scenario fails at the tree without the gate, and every
  failure line stands recorded in the task record before T017 lands.
  Module M4 start-gate, wave 2, parallel set: T005 to T009 (M2 owns disjoint
  files).
  Test: `test/source/harness_graph_gate_test.cpp`: `suitePassesOnTwoCpus`.
  Verify: `taskset -c 0,1 ctest --test-dir build -R harness_graph_gate_test`
  (red before T017).
  Depends: T010 (one file, appended in task order).
  Commit: `Harness: Add the two-cpu suite test`.

- [X] T017 [P] [US1] Define `detail::GraphGate` in
  `source/harness/graph_gate.cpp`: the constructor over a graph,
  `awaitStart(slot)` answering `run` or `skip`, `nodeFinished`,
  `nodeFailed`, `settleStopped`, `outcomes`. One mutex carries the ready
  counts and the settled words; a condition variable carries the wake and
  every predicate re-reads the state; the release fact is the completion
  state of specs/020 FR-049 read with acquire order; the gate releases the
  lowest ready slot first; a failed or held dependency settles the
  transitive closure `held` with the gating failed node; the stop request
  settles gated nodes `stopped` with their dependents. No sleep, no clock, no timeout (FR-013,
  FR-014, FR-016, FR-019, FR-020, FR-021, FR-026, R-06, R-09, D-3 and D-10
  of research.md). The task turns T010, T011, T013, T014, T015 and T016
  green.
  Module M4 start-gate, wave 2, parallel set: T005 to T009 (M2 owns
  disjoint files; T018 runs after this task: the runner edit rides its own
  commit).
  Test: the gate scenarios of T010, T011, T013, T014, T015, T016.
  Verify: `ctest --test-dir build -R harness_graph_gate_test`.
  Depends: T010 to T016 (each red run stands recorded).
  Commit: `Harness: Define the graph start gate`.

- [X] T018 [P] [US1] Wire the gate into `source/harness/runner.cpp`:
  `threadedSampleRun` owns one gate per threaded attempt beside the two
  barriers at `:2013`; the body calls `awaitStart(slot)` after the start
  round at `:2073` and before the callable, beside the standing
  setup-failure skip at `:2085`; the `skip` arm arrives at the stop round at
  `:2096` with no body run and no window open; the body calls `nodeFinished`
  or `nodeFailed` after the callable; the settlement reads the per-context
  outcomes beside the merged row and hands the words to the record. The
  dispatch of every body index and the `threadRunner` ledger of specs/020
  FR-021 stand unchanged (FR-013, FR-015, FR-016, FR-017, FR-019, FR-021,
  R-06, R-07, R-09, D-4 of research.md). The task turns T012 green.
  Module M4 start-gate, wave 2, parallel set: T005 to T009 (M2 owns
  disjoint files; this task runs after T017: the gate unit rides its own
  commit).
  Test: `test/source/harness_graph_gate_test.cpp`:
  `heldContextArrivesBothRounds`, `gateOpensNoWindow`,
  `placementBytesUnchangedBesideGraph`, `noexceptItemConceptHolds`.
  Verify: `ctest --test-dir build -R harness_graph_gate_test`, then
  `ctest --test-dir build --output-on-failure`.
  Depends: T012, T017.
  Commit: `runner: Gate the start between the barrier rounds`.
---

## Phase 4: Wave 3, module M3 `cli-row`

**Purpose**: the row, the bare form, the option file source, the count
  refusals and the parse-once rule stand wired at the entry point.

**Independent test**: each grammar arm and each count arm prints one
  line and exits 2 where the spec says so, and the file reads once
  ahead of the first context (spec US3).

- [X] T019 [P] [US3] Append `graphRowValueBlockAndUniqueness` to
  `test/source/harness_cli_table_test.cpp`: the table carries one row named
  `graph`, its value reads `297`, the value at branch time under the
  reservation rule of FR-002, the value stands in the free block `297`
  to `319` of `source/harness/detail/options.hpp:76`, the standing name and
  value uniqueness checks at `:780` cover the row, and the row carries the
  environment flag off
  (FR-001, FR-002, R-01). Red step: `cmake --build build -j 24` then
  `ctest --test-dir build -R harness_cli_table_test` fails naming the
  missing row; the failure line stands recorded before T022 lands.
  Module M3 cli-row, wave 3, parallel set: T020, T021, T024 to T031 (M5
  owns disjoint files; T022 and T051 run after this task).
  Test: `test/source/harness_cli_table_test.cpp`:
  `graphRowValueBlockAndUniqueness`.
  Verify: the red step `cmake --build build -j 24` and
  `ctest --test-dir build -R harness_cli_table_test`.
  Depends: M1, M2 (the wave gate).
  Commit: `Harness: Cover the graph option row`.

- [X] T020 [P] [US3] Register `harness_cli_graph_test` in
  `test/CMakeLists.txt` under the block `Execution graphs: cli (M3)`, then
  write the failing grammar tests in
  `test/source/harness_cli_graph_test.cpp`: `graphRowPrintsHelpAndOptions`
  prints the row in the `--help` and `--options` output;
  `graphEqualsFileReads` reads the file named by `--graph=<file>`;
  `bareGraphTakesOptionFilePath` takes the path from the option file line of
  specs/028 FR-035 and reads no environment variable;
  `bareGraphNoPathRefuses` prints one line naming the row and the missing
  path and exits 2 before any context starts (FR-001, FR-009, R-01, R-02,
  contracts/cli-graph.md). Red step: `cmake --build build -j 24` then
  `ctest --test-dir build -R harness_cli_graph_test` fails; the failure line
  stands recorded before T022 lands.
  Module M3 cli-row, wave 3, parallel set: T019, T025 to T031 (T021
  appends to the same test file in task order; T022 and T023 run after
  this task; T024 appends to `test/CMakeLists.txt` in task order).
  Test: `test/source/harness_cli_graph_test.cpp`:
  `graphRowPrintsHelpAndOptions`, `graphEqualsFileReads`,
  `bareGraphTakesOptionFilePath`, `bareGraphNoPathRefuses`.
  Verify: the red step `cmake --build build -j 24` and
  `ctest --test-dir build -R harness_cli_graph_test`.
  Depends: M1, M2 (the wave gate).
  Commit: `Harness: Add the graph option grammar tests`, carrying the
  `test/CMakeLists.txt` block.

- [X] T021 [P] [US3] Append the count and parse-once tests to
  `test/source/harness_cli_graph_test.cpp`: `countMismatchRefuses` prints
  one line naming the node count and the context count for a three-node
  graph over a two-context run and exits 2; `noThreadedRunRefuses` prints
  one line for a batch with no threaded run and exits 2;
  `singleContextTakesNoGraph` keeps the standing bytes and the exit status
  `0` for a single-context run and gains no record entry;
  `graphParsesOnceBeforeRun` reads the file once ahead of the first context,
  the parse-once shape of `source/harness/cli.cpp:560-563` (FR-011, FR-012,
  SC-003, R-05, R-11). Red step: each arm fails at the tree without the row
  and the entry point, and every failure line stands recorded in the task
  record before T023 lands.
  Module M3 cli-row, wave 3, parallel set: T019, T024 to T031 (M5 owns
  disjoint files).
  Test: `test/source/harness_cli_graph_test.cpp`: `countMismatchRefuses`,
  `noThreadedRunRefuses`, `singleContextTakesNoGraph`,
  `graphParsesOnceBeforeRun`.
  Verify: `ctest --test-dir build -R harness_cli_graph_test` (red before
  T023).
  Depends: T020 (one file, appended in task order).
  Commit: `Harness: Add the graph count refusal tests`.

- [X] T022 [P] [US3] Add the row: `kGraphOptionValue = 297`, the value at
  branch time under the reservation rule of FR-002, and the moved block
  comment in `source/harness/detail/options.hpp:76` naming `297` taken and
  `298` to `319` free. The commit body records the rule and the value the
  lane took, the way T032 records the version values. Add the `graph` row
  of the option table with its
  settle arm in `source/harness/options.cpp:544`: the argument kind
  `OPTIONAL`, the environment flag off, the help text, the owner
  `specs/037`, the path text of `--graph=<file>` stored in
  `RunOptions.graphPath`, and the bare row marked as given with no path of
  its own (FR-001, FR-002, FR-009, R-01, R-02). The task turns T019 and part
  of T020 green.
  Module M3 cli-row, wave 3, parallel set: T024 to T031 (T019 and T020 run
  first, each on its own file).
  Test: `test/source/harness_cli_table_test.cpp`:
  `graphRowValueBlockAndUniqueness`, the 37-row check and the
  contract-order check of T051;
  `test/source/harness_cli_graph_test.cpp`: `graphRowPrintsHelpAndOptions`,
  `graphEqualsFileReads`.
  Verify: `ctest --test-dir build -R harness_cli_table_test`, then
  `ctest --test-dir build -R harness_cli_graph_test`.
  Depends: T019, T020 (each red run stands recorded).
  Commit: `Harness: Add the graph option row`.

- [X] T051 [P] [US3] Carry the row into the standing table contract in
  `test/source/harness_cli_table_test.cpp`: add
  `{"graph", kOptional, 297}`, the value at branch time under the
  reservation rule of FR-002, to `kContract` at `:205` in the position
  the table prints it, move the count in `theTableHoldsThirtySixRows` at
  `:673` to 37 rows, rename the arm and its label to
  `theTableHoldsThirtySevenRows`, and keep
  `theNamesStandInTheContractOrder` at `:683` green (FR-001, FR-002,
  R-01). The task turns the standing checks of T022 green.
  Module M3 cli-row, wave 3, parallel set: T024 to T031 (M5 owns
  disjoint files).
  Test: `test/source/harness_cli_table_test.cpp`:
  `graphRowValueBlockAndUniqueness`, the 37-row check, the
  contract-order check.
  Verify: `ctest --test-dir build -R harness_cli_table_test`.
  Depends: T019, T022.
  Commit: `test: Carry the graph row in the table contract`.

- [X] T023 [P] [US3] Wire the entry point in `source/harness/cli.cpp`: read the
  graph once ahead of any run beside the parse-once block at `:560-563`,
  install the graph read into `graphInput()`, take the bare row's path from
  the option file line and read no environment variable for the row, run the
  count checks per threaded run in batch order after the expansion, and
  print the first fault alone with exit status 2 before any context starts,
  and thread the run's record entry to `printResult` the way the hunt record
  rides `entryHuntRecords` (FR-009, FR-010, FR-011, FR-012, FR-022, SC-003,
  SC-006, R-02, R-03, R-05, R-10). The task turns T020 and T021 green.
  Module M3 cli-row, wave 3, parallel set: T024 to T031 (M5 owns disjoint
  files).
  Test: `test/source/harness_cli_graph_test.cpp`: every scenario of T020 and
  T021.
  Verify: `ctest --test-dir build -R harness_cli_graph_test`, then
  `ctest --test-dir build --output-on-failure`.
  Depends: T020, T021, T022.
  Commit: `Harness: Read the graph once at the entry point`.
---

## Phase 5: Wave 3, module M5 `record-disclosure`

**Purpose**: the console block, the public record, the writer, the
  reader and the schema carry one fact.

**Independent test**: a diamond run's record round-trips through
  `loadReport`, the console block folds equal against it, and a
  graph-free run matches master's bytes apart from the two values FR-006
  names (spec US2).

- [X] T024 [P] [US2] Register `harness_graph_report_test` in
  `test/CMakeLists.txt` under the block `Execution graphs: record (M5)`,
  then write the failing console tests in
  `test/source/harness_graph_report_test.cpp`:
  `consoleBlockBesideContextsLine` prints `graph nodes=` and `edges=`
  directly after the run's `contexts=` line beside `printContextDisclosure`
  at `source/harness/report.cpp:167`, and asserts the one printed order of
  FR-023: the nodes in ascending slot order and the edges as `from->to`
  pairs in file order. The scenario prints the
  `graph held=<node>(<gating>)` line only when a node stands held and the
  `graph stopped=` line only when a node stands stopped, and keeps the timed
  rows at the bytes a graph-free run of the same benchmark gives;
  `blockOncePerRun` prints one block per threaded run of a batch of two
  threaded runs (FR-022, SC-001, SC-006, R-10, D-6 of research.md). Red
  step: `cmake --build build -j 24` then
  `ctest --test-dir build -R harness_graph_report_test` fails; the failure
  line stands recorded before T031 lands.
  Module M5 record-disclosure, wave 3, parallel set: T019, T021 to T023,
  T051 (M3 owns disjoint files; T020 appends to `test/CMakeLists.txt` in
  task order).
  Test: `test/source/harness_graph_report_test.cpp`:
  `consoleBlockBesideContextsLine`, `blockOncePerRun`.
  Verify: the red step `cmake --build build -j 24` and
  `ctest --test-dir build -R harness_graph_report_test`.
  Depends: M1, M4 (the wave gate).
  Commit: `Harness: Add the graph console block tests`, carrying the
  `test/CMakeLists.txt` block.

- [X] T025 [P] [US2] Append the record tests to
  `test/source/harness_graph_report_test.cpp`: `recordKeysOrderAndReadBack`
  writes a diamond run, states the top-level `graph` record stands after
  `precheck` and before `benchmarks`, states the entry keys `benchmark`,
  `nodes`, `edges`, `held`, `stopped` with their `x-order`, and
  folds the record `loadReport` read back against the run's facts;
  `nodeOrderAscendingSlots` states the record's node array names the nodes
  in ascending slot order and the console prints that order;
  `reasonNamesFailedNode`
  states the reason names a failed node and a stopped node carries none;
  `recordCarriesNoTiming` states the record carries no timing figure
  (FR-023, FR-024, SC-002, SC-006, R-11, D-5 of research.md). Red step: the
  scenarios fails at the tree without the record, and every failure line
  stands recorded in the task record before T030 lands.
  Module M5 record-disclosure, wave 3, parallel set: T019 to T023, T051
  (M3 owns disjoint files).
  Test: `test/source/harness_graph_report_test.cpp`:
  `recordKeysOrderAndReadBack`, `nodeOrderAscendingSlots`,
  `reasonNamesFailedNode`, `recordCarriesNoTiming`.
  Verify: `ctest --test-dir build -R harness_graph_report_test`, then
  `ctest --test-dir build -R harness_json_load_test` (red before T030).
  Depends: T024 (one file, appended in task order).
  Commit: `Harness: Add the graph record round-trip tests`.

- [X] T026 [P] [US2] Append the schema-arm tests to
  `test/source/harness_graph_report_test.cpp`:
  `schemaCarriesGraphKeysAndOrder` states the record keys, the entry
  `x-order` lists and the moved top-level `x-order` stand in
  `docs/schema/report-v1.json` at the minor the version commit records; a
  document with a missing key, a key of the wrong kind, a duplicate key and
  an order the `x-order` lists state never each fail the standing schema
  check (FR-005, FR-023, FR-028, SC-006, R-11, R-13). Red step: the
  scenarios fails at the tree without the schema file, and every failure
  line stands recorded in the task record before T029 lands.
  Module M5 record-disclosure, wave 3, parallel set: T019 to T023, T051
  (M3 owns disjoint files).
  Test: `test/source/harness_graph_report_test.cpp`:
  `schemaCarriesGraphKeysAndOrder` and the four planted document arms.
  Verify: `ctest --test-dir build -R harness_graph_report_test`, then
  `ctest --test-dir build -R harness_json_schema_test` (red before T029).
  Depends: T024 (one file, appended in task order).
  Commit: `Harness: Add the graph schema arm tests`.

- [X] T027 [P] [US2] Append the byte-identity and ABI tests to
  `test/source/harness_graph_report_test.cpp`: `noGraphBytesMatchMaster`
  writes a run with no `--graph` and folds its console report and its JSON
  bytes against master's, the `schemaVersion` value and the
  `speedgun.version` value standing the only differences;
  `reportDataGraphMemberRaisesAbi` states `ReportData::graph`
  stands in the public loaded-report struct, the fact that moves the harness
  `SOVERSION` (FR-004, FR-006, SC-005, R-11, R-13). Red step: the scenarios
  fails at the tree without the record, and every failure line stands
  recorded in the task record before T030 lands.
  Module M5 record-disclosure, wave 3, parallel set: T019 to T023, T051
  (M3 owns disjoint files).
  Test: `test/source/harness_graph_report_test.cpp`:
  `noGraphBytesMatchMaster`, `reportDataGraphMemberRaisesAbi`.
  Verify: `ctest --test-dir build -R harness_graph_report_test` (red before
  T030).
  Depends: T024 (one file, appended in task order).
  Commit: `Harness: Add the graph byte-identity tests`.

- [X] T028 [P] [US2] Add the public record types to
  `include/speedgun-ng/benchmark.hpp` beside the `precheck` member of
  `ReportData` at `:786`: `GraphRecord` with `runs`; `GraphRunRecord` with
  `benchmark`, `nodes`, `edges`, `held`, `stopped`;
  `GraphNodeRecord` with `name`, `slot`, `outcome`; `GraphEdgeRecord` with
  `from`, `to`; `GraphHeldRecord` with `node`, `reason`; `ReportData::graph`
  as `std::optional<GraphRecord>`; each type carries its `\pre`, `\post` and `\invariant`
  doxygen contract, and no other public name joins (FR-004, FR-023, D-8 of
  the spec, R-11).
  Module M5 record-disclosure, wave 3, parallel set: T019 to T023, T029,
  T051 (M3 owns disjoint files; T030 and T031 run after this task).
  Test: `test/source/harness_graph_report_test.cpp`:
  `reportDataGraphMemberRaisesAbi`, `recordKeysOrderAndReadBack`.
  Verify: `cmake --preset=ci-rocky`, `cmake --build build -j 24`,
  `ctest --test-dir build -R harness_graph_report_test`.
  Depends: T025, T027 (each red run stands recorded).
  Commit: `Harness: Add the graph record types`.

- [X] T029 [P] [US2] Extend `docs/schema/report-v1.json` under the block
  `Execution graphs (M5)`: the `graph` object with its one key `runs`, the
  entry keys and their kinds, the sub-object `x-order` lists `name`, `slot`,
  `outcome` and `from`, `to` and `node`, `reason`, the entry `x-order`
  `benchmark`, `nodes`, `edges`, `held`, `stopped`, and the
  top-level `x-order` moved to carry `graph` after `precheck` and before
  `benchmarks`; the schema minor takes `1.7` in the same commit as the
  writer, the reservation the Spec identity section of spec.md states
  (FR-005, FR-023, R-11, R-13).
  Module M5 record-disclosure, wave 3, parallel set: T019 to T023, T028,
  T051 (M3 owns disjoint files; T030 and T031 run after this task).
  Test: `test/source/harness_graph_report_test.cpp`:
  `schemaCarriesGraphKeysAndOrder`.
  Verify: `ctest --test-dir build -R harness_json_schema_test`, then
  `ctest --test-dir build -R harness_graph_report_test`.
  Depends: T026 (the red run stands recorded).
  Commit: `Docs: Carry the graph record schema`.

- [X] T030 [P] [US2] Write the record in `source/harness/json_report.cpp` and
  read it in `source/harness/json_load.cpp`: the `writeReport` graph
  parameter emits the top-level `graph` record after `precheck` with one
  entry per threaded run in batch order; `loadReport` reads the record into
  `ReportData::graph`, the empty optional standing for a document with no
  record; a run with no `--graph` writes no key and keeps master's bytes
  apart from the `schemaVersion` value and the `speedgun.version` value the
  version move carries; the schema minor moves to `1.7` beside the writer,
  the reservation the Spec identity section of spec.md states (FR-005,
  FR-006, FR-023, FR-024, SC-005, SC-006, R-11, R-13). The task turns T025,
  T026 and T027 green.
  Module M5 record-disclosure, wave 3, parallel set: T019 to T023, T051
  (M3 owns disjoint files; T031 runs after this task: the console printer
  rides its own commit).
  Test: `test/source/harness_graph_report_test.cpp`:
  `recordKeysOrderAndReadBack`, `schemaCarriesGraphKeysAndOrder`,
  `noGraphBytesMatchMaster`.
  Verify: `ctest --test-dir build -R harness_graph_report_test`, then
  `ctest --test-dir build --output-on-failure`.
  Depends: T025, T026, T027, T028, T029.
  Commit: `Harness: Write and read the graph record`.

- [X] T031 [P] [US2] Add `printGraphReport` to `source/harness/report.cpp`
  beside `printContextDisclosure` at `:167`: one block per threaded run
  directly after that run's `contexts=` line, the nodes in ascending slot
  order, the edges as `from->to` pairs in file order, the held
  line naming each held node with its gating failed node in parentheses in
  ascending slot order, the stopped line naming each
  stopped node in ascending slot order, and no block for a run with no
  graph (FR-022, FR-023, SC-001, SC-006, R-10, D-6 of research.md). The
  task turns T024 green.
  Module M5 record-disclosure, wave 3, parallel set: T019 to T023, T051
  (M3 owns disjoint files).
  Test: `test/source/harness_graph_report_test.cpp`:
  `consoleBlockBesideContextsLine`, `blockOncePerRun`.
  Verify: `ctest --test-dir build -R harness_graph_report_test`, then
  `ctest --test-dir build --output-on-failure`.
  Depends: T024, T028, T030.
  Commit: `Harness: Print the graph console block`.
---

## Phase 6: Wave 4, modules M6 `version-abi`, M7 `docs-example`, M8 `evidence`

**Purpose**: the version and ABI record, the docs, the example and the
  acceptance record land beside the code.

**Independent test**: the version test reads the value the commit
  records, the example folds against its binary, and the results
  directory carries the acceptance record (spec US5, US6).

- [X] T032 [P] [US2] Move `CMakeLists.txt:19` to `0.23.0` and
  `CMakeLists.txt:302` to harness `SOVERSION 14`, the archive `SOVERSION` at
  `:84` staying `4` and no `SOVERSION` going down. The commit body records
  the ABI check beside the move: the graph record joins the public
  loaded-report struct, so the harness public ABI changes and its number
  rises by one. The body records the reservation rule of specs/029
  (`specs/029-hunter-core/spec.md:44-49`): at rebase the lane reads
  `CMakeLists.txt` and `source/harness/json_report.cpp` on master and takes
  the next free value of each. The body names the values the lane took, and
  the Spec identity section of spec.md carries the rule and the pull
  request #57 pairing (FR-003, FR-004, FR-005, R-13, D-8 of the spec).
  Module M6 version-abi, wave 4, parallel set: T037, T038 (M8 owns
  disjoint files; T033, T034, T035 and T036 run after this task).
  Test: `test/source/harness_cli_test.cpp`: `versionPrintsPlannedMinor`,
  which reads the `VERSION` of the root `CMakeLists.txt` as its expected
  value; `test/source/harness_graph_report_test.cpp`:
  `reportDataGraphMemberRaisesAbi`.
  Verify: `cmake --preset=ci-rocky`, `cmake --build build -j 24`,
  `ctest --test-dir build -R harness_cli_test`,
  `ctest --test-dir build -R harness_graph_report_test`.
  Depends: T028, T030 (the record joins the public struct).
  Commit: `CMake: Move the version and the harness SOVERSION`.

- [X] T052 [P] [US2] Append `versionPrintsPlannedMinor` to
  `test/source/harness_cli_test.cpp`: `--version` prints the project
  version the build hands the harness as `SG_PROJECT_VERSION`, and the
  written document carries the same text in `speedgun.version`; the
  scenario reads the value from the root `CMakeLists.txt` the way
  `projectVersion` at `test/source/harness_cli_table_test.cpp:1146`
  reads it, so the check stands binary at any value the version commit
  records (FR-003, FR-006, R-13).
  Module M6 version-abi, wave 4, parallel set: T033 to T036, T037, T038
  (M7 and M8 own disjoint files).
  Test: `test/source/harness_cli_test.cpp`: `versionPrintsPlannedMinor`.
  Verify: `ctest --test-dir build -R harness_cli_test`.
  Depends: T032.
  Commit: `test: Pin the version the lane records`.

- [X] T033 [P] [US6] Add the graph section to `README.md` under the block
  `Execution graphs (M7)`, at most 25 lines, one purpose sentence, one
  marked snippet from the example region, one result sentence, beside the
  `Memory nodes` and `Host noise pre-check` sections, and add the
  quick-reference rows under their own block: the `--graph=<file>` option
  row and the record rows the drift gate checks against the public headers
  (FR-032, IV.1 of the constitution, R-15).
  Module M7 docs-example, wave 4, parallel set: T034, T037, T038, T052
  (T032 and T035 run before this task; T036 runs after it).
  Test: `ctest -R example_drift` folds the section, the snippet region and
  the quick-reference rows.
  Verify: `ctest --test-dir build -R example_drift`, then
  `ctest --test-dir build -R snippet_compile`.
  Depends: T031, T035 (the page quotes the example region).
  Commit: `Docs: Teach the execution graph`.

- [X] T034 [P] [US6] Add the graph section to `docs/pages/harness.md` under
  the block `Execution graphs (M7)` beside the placement and pre-check
  sections: the `--graph` grammar row with its bare form and its option file
  source, the file shape with its four keys and its normative order, the
  start rule with the lowest ready slot first (FR-013), the held rule with its reason,
  the stopped rule with no reason, the console block lines and the record
  keys (FR-033, R-15).
  Module M7 docs-example, wave 4, parallel set: T033, T037, T038, T052
  (T032 and T035 run before this task; T036 runs after it).
  Test: `ctest -R example_drift` folds the page's quoted block and its
  recorded run.
  Verify: `ctest --test-dir build -R example_drift`, then
  `ctest --test-dir build -R report_shape_execution_graphs_sample`.
  Depends: T031, T035 (the page quotes the example region).
  Commit: `Docs: Detail the graph option and record`.

- [X] T035 [P] [US6] Add `example/docs/execution_graphs.cpp` with a three-node
  chain over three contexts and its shape-golden file
  `example/docs/execution_graphs.expected`, marked with the `sg-example`
  regions the two pages quote; register it in `example/CMakeLists.txt` with
  `add_docs_example(execution_graphs)` and register
  `report_shape_execution_graphs_sample` in `test/CMakeLists.txt` under the
  block `Execution graphs: example (M7)`, the shape fold reading
  `docs/pages/harness.md` and the `.expected` file (FR-034, IV.1, R-15).
  Module M7 docs-example, wave 4, parallel set: T037, T038, T052 (T032 runs
  first: the run the example records).
  Test: `example/docs/execution_graphs.expected` folds against the binary's
  own run.
  Verify: `ctest --test-dir build -R report_shape_execution_graphs_sample`,
  then `ctest --test-dir build -R example_run`.
  Depends: T031, T032 (the run the example records).
  Commit: `Docs: Record the graph example run`.

- [X] T036 [P] [US6] Add the drift rows to `tools/docs/example_drift.py` under
  the block `Execution graphs (M7)`: the section title `Execution graphs` in
  `SECTIONS` beside `Memory nodes` and `Host noise pre-check`, and the
  `SECTION_OWNERS` row naming `execution_graphs.cpp` (FR-032, FR-033,
  FR-034, R-15).
  Module M7 docs-example, wave 4, parallel set: T037, T038, T052 (T033,
  T034 and T035 run before this task).
  Test: `ctest -R example_drift` checks the feature-section list and the
  owner rows.
  Verify: `ctest --test-dir build -R example_drift`, then
  `ctest --test-dir build -R example_drift_fixtures`.
  Depends: T033, T034, T035 (the rows name what landed).
  Commit: `CI: Name the graph docs section owner`.

- [X] T037 [P] [US5] Write `tools/graph/acceptance.py`: build the fan-out and
  fan-in graph over the lane host's 16 cores, run it, and write the record:
  the printed node order, the figures beside it, the boot, the
  commands and the exit codes, the shape specs/035 recorded for the NPS
  evidence. No CI job names the script (FR-030, FR-031, R-14, D-9 of the
  spec).
  Module M8 evidence, wave 4, parallel set: T032, T033 to T036, T052 (M7
  owns disjoint files).
  Test: the record the script writes names the boot, the commands, the exit
  codes and the printed node order.
  Verify: `python3 tools/graph/acceptance.py`, then the CI config inspection
  naming no job for it.
  Depends: T018, T031 (the run and the block the record reads).
  Commit: `CI: Add the graph acceptance script`.

- [X] T038 [P] [US5] Run the script on the lane host and record the result under
  `specs/037-execution-graphs/results/`: the record names the boot, the
  commands, the exit codes, the printed node order and the figures
  beside it, and names the host's 16-core, 32-thread shape beside the
  24-core, 48-thread record of specs/035; the results directory links the
  record and the quickstart links it (FR-030, SC-009, R-14).
  Module M8 evidence, wave 4, parallel set: T032, T033 to T036, T052 (M7
  owns disjoint files).
  Test: `specs/037-execution-graphs/results/` carries the record of the lane
  run.
  Verify: `python3 tools/graph/acceptance.py` on the lane host, then the
  record read.
  Depends: T037.
  Commit: `Docs: Record the graph acceptance run`.
---

## Phase 7: The gates at the feature head

**Purpose**: the hard gates of Principle VIII run at the tip commit of
  the branch: format, spell, prose, clang-tidy, cppcheck, the
  sanitizers, the thread sanitizer, coverage, the DBC completeness gate,
  the docs gates, and the two-cpu run.

**Independent test**: each gate prints its own verdict over the landed
  tree, and every finding stands cleared at lint parity.

- [X] T039 [P] Run the format gate at the feature head:
  `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake`, and clear
  every finding on the files this feature touched (constitution VIII).
  Module head gates, wave head, parallel set: T040 to T047, T050 (each gate owns its
  own build tree and its own tool, and the set omits this task).
  Test: the gate itself, over every file of the branch.
  Verify: `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake`.
  Depends: T001 to T038 landed.
  Commit: `CI: Clear the format gate`.

- [X] T040 [P] Run the spell gate at the feature head:
  `cmake -P cmake/spell.cmake`, and clear every finding, the new names
  joining the project dictionary only where the standing dictionary admits
  them (constitution VIII).
  Module head gates, wave head, parallel set: T039, T041 to T047, T050 (each gate owns its
  own build tree and its own tool, and the set omits this task).
  Test: the gate itself, over every file of the branch.
  Verify: `cmake -P cmake/spell.cmake`.
  Depends: T001 to T038 landed.
  Commit: `Docs: Clear the spell gate`.

- [X] T041 [P] Run the prose gate at the feature head:
  `cmake -P cmake/prose-lint.cmake` over the branch range, and
  `ctest --test-dir build -R prose_gate_fixtures`; clear every finding in
  every comment, page, spec, plan and commit message of the branch
  (constitution VIII, XI).
  Module head gates, wave head, parallel set: T039, T040, T042 to T047, T050 (each gate owns its
  own build tree and its own tool, and the set omits this task).
  Test: the gate itself, over the branch range.
  Verify: `cmake -P cmake/prose-lint.cmake`, then
  `ctest --test-dir build -R prose_gate_fixtures`.
  Depends: T001 to T038 landed.
  Commit: `Docs: Clear the prose gate`.

- [X] T042 [P] Run the release check once for the feature:
  `cmake --preset=ci-ubuntu` and `cmake --build build`, the preset that
  wires clang-tidy and cppcheck into every compile, and clear every finding
  at lint parity, the name check reading zero
  `readability-identifier-naming` findings over the new names of V.1
  (FR-029, constitution V.1, VIII).
  Module head gates, wave head, parallel set: T039 to T041, T043 to T047, T050 (each gate owns its
  own build tree and its own tool, and the set omits this task).
  Test: the gate itself, over every compile of the branch.
  Verify: `cmake --preset=ci-ubuntu`, then `cmake --build build`.
  Depends: T001 to T038 landed.
  Commit: `CI: Clear the tidy and cppcheck gate`.

- [X] T043 [P] Run the sanitizer suite at the feature head:
  `cmake --preset=ci-sanitize`, `cmake --build build/sanitize -j 24`,
  `ctest --test-dir build/sanitize --output-on-failure`; ASan and UBSan
  report no error over the graph suite (constitution VIII).
  Module head gates, wave head, parallel set: T039 to T042, T044 to T047, T050 (each gate owns its
  own build tree and its own tool, and the set omits this task).
  Test: the graph suite under the sanitizers.
  Verify: `cmake --preset=ci-sanitize`,
  `cmake --build build/sanitize -j 24`,
  `ctest --test-dir build/sanitize --output-on-failure`.
  Depends: T001 to T038 landed.
  Commit: `CI: Clear the sanitizer gate`.

- [X] T044 [P] Run the thread-sanitizer suite at the feature head:
  `cmake --preset=ci-tsan`, `cmake --build build/tsan -j 24`,
  `ctest --test-dir build/tsan --output-on-failure`; the gate's mutex, its
  condition variable and the acquire read of the FR-049 completion state
  report no race, the story D-10 of research.md records (FR-013, FR-015,
  constitution VIII).
  Module head gates, wave head, parallel set: T039 to T043, T045 to T047, T050 (each gate owns its
  own build tree and its own tool, and the set omits this task).
  Test: the graph suite under the thread sanitizer.
  Verify: `cmake --preset=ci-tsan`, `cmake --build build/tsan -j 24`,
  `ctest --test-dir build/tsan --output-on-failure`.
  Depends: T001 to T038 landed.
  Commit: `CI: Clear the thread sanitizer gate`.

- [X] T045 [P] Run the coverage target at the feature head:
  `cmake --preset=ci-coverage`, `cmake --build build/coverage -j 24`,
  `ctest --test-dir build/coverage -j 1`,
  `cmake --build build/coverage -t coverage`, with
  `PERL5LIB=/home/archerc/perl5/lib/perl5` exported for the lcov run. The
  figures read 100.0% lines and 100.0% branches over every new unit, no
  `LCOV_EXCL` marker joins, no new `--ignore-errors` entry joins, and the
  coverage plan of the plan stands driven arm by arm (FR-028, SC-007,
  constitution VI).
  Module head gates, wave head, parallel set: T039 to T044, T046, T047, T050 (each gate owns its
  own build tree and its own tool, and the set omits this task).
  Test: the coverage figures over every new unit.
  Verify: `cmake --preset=ci-coverage`,
  `cmake --build build/coverage -j 24`,
  `ctest --test-dir build/coverage -j 1`,
  `cmake --build build/coverage -t coverage`.
  Depends: T001 to T038 landed.
  Commit: `CI: Clear the coverage gate`.

- [X] T046 [P] Run the docs gates at the feature head:
  `ctest --test-dir build -R example_drift`,
  `ctest --test-dir build -R example_drift_fixtures`,
  `ctest --test-dir build -R snippet_compile`,
  `ctest --test-dir build -R example_run`,
  `ctest --test-dir build -R example_run_fixtures`,
  `ctest --test-dir build -R report_shape`,
  `ctest --test-dir build -R report_shape_execution_graphs_sample`,
  `ctest --test-dir build -R report_shape_fixtures`; every finding stands
  cleared (FR-032, FR-033, FR-034, SC-010, constitution IV.1, VIII).
  Module head gates, wave head, parallel set: T039 to T045, T047, T050 (each gate owns its
  own build tree and its own tool, and the set omits this task).
  Test: the page blocks, the snippet, the example run and the shape folds.
  Verify: the eight `ctest -R` runs named in the task text.
  Depends: T033, T034, T035, T036 landed.
  Commit: `CI: Clear the docs gates`.

- [X] T047 [P] Run the suite under two cpus at the feature head:
  `taskset -c 0,1 ctest --test-dir build --output-on-failure`, and record
  the result beside the acceptance record; no test sleeps, no test reads a
  clock, no test carries a fixed node count, and
  `ctest --test-dir build -R time_source_gate` stands green over every new
  unit (FR-026, FR-027, SC-008, constitution VIII).
  Module head gates, wave head, parallel set: T039 to T046, T050 (each gate owns its
  own build tree and its own tool, and the set omits this task).
  Test: the whole suite on two cpus.
  Verify: `taskset -c 0,1 ctest --test-dir build --output-on-failure`, then
  `ctest --test-dir build -R time_source_gate`.
  Depends: T001 to T038 landed.
  Commit: `CI: Clear the two-cpu suite gate`.

- [X] T050 [P] Run the DBC gate at the feature head:
  `cmake --build build -t dbc-gate`, then
  `ctest --test-dir build -R dbc_gate_fixtures`; every `SG_*` check of
  every new unit stands documented and paired, and the matrix the gate
  writes reads complete over the graph units (FR-028, SC-007,
  constitution II, VI, VIII).
  Module head gates, wave head, parallel set: T039 to T047 (each gate
  owns its own build tree and its own tool).
  Test: the gate itself, over every contract of the branch.
  Verify: `cmake --build build -t dbc-gate`, then
  `ctest --test-dir build -R dbc_gate_fixtures`.
  Depends: T001 to T038 landed.
  Commit: `CI: Clear the DBC gate`.
---

## Phase 8: Closing

**Purpose**: the quickstart record and the commit and constitution
  check the lane lands with.

**Independent test**: every quickstart scenario stands recorded with
  its command and its exit code, and every commit of the branch reads
  against the template.

- [X] T048 Run every scenario of [quickstart.md](./quickstart.md) end to end
  at the feature head and record the commands, the exit codes and the
  figures under `specs/037-execution-graphs/results/quickstart.md`, the
  chain run, the record run, the read-back, the three refusals, the held
  run, the byte-identity fold and the example fold each named (SC-001 to
  SC-010, X.4 of the constitution).
  Module closing, wave head, parallel set: none (T049 runs after this
  task).
  Test: the quickstart scenarios, each recorded with its command and exit
  code.
  Verify: `ctest --test-dir build --output-on-failure`, then the quickstart
  runs as the file lists them.
  Depends: T039 to T047 and T050 green.
  Commit: `Docs: Record the quickstart runs`.

- [X] T049 Check the branch's commits against the template of Pull Request
  Quality: every title `<Section>: <one-line imperative>` at 50 characters
  or fewer, every why-body wrapped at 72 columns, every commit carrying
  `Refs: specs/037-execution-graphs`, `Fixes #n` where an issue exists, one
  idea per commit, the history bisectable, no `Approved-by:` name invented
  before the reviewer's own lands. Re-check the constitution table of
  [plan.md](./plan.md) against the landed tree and record the verdict in the
  pull-request body (constitution VIII, IX, XI, Pull Request Quality).
  Module closing, wave head, parallel set: none (this task runs after
  T048).
  Test: the commit range of the branch, read by the prose gate's commit
  check.
  Verify: `cmake -P cmake/prose-lint.cmake`, then `git log --format=%B` over
  the branch range.
  Depends: T048.
  Commit: `Meta: Record the commit and constitution check`.
---

## Dependencies and execution order

### Wave dependencies

- Wave 1 (M1) stands first: every later module codes against the model,
  the words, the gate type, the seam and the slot.
- Wave 2 runs M2 beside M4. M2 depends on M1 alone, and M4 depends on
  M1 alone; the file sets stand disjoint apart from `test/CMakeLists.txt`,
  where each module appends its own labeled block in its own commit.
- Wave 3 runs M3 beside M5. M3 depends on M1 and M2, and M5 depends on
  M1 and M4; the file sets stand disjoint apart from
  `test/CMakeLists.txt`, where each module appends its own labeled block
  in its own commit.
- Wave 4 runs M6 beside M8 and M7 beside M8: the version move, the docs
  and the evidence each own disjoint files, and M7 follows M6.
- Phase 7 runs at the feature head once every module has landed, and
  Phase 8 closes the lane.

### Within each module

- The test task stands written, run, and observed failing before the
  code task turns it green.
- M1: the header before the unit, the seam test before the seam
  definition.
- M2: the arms before the schema file and the reader.
- M4: the scenarios before the gate unit, the gate unit before the
  runner wiring.
- M3: the table and grammar arms before the row, the row before the
  entry point.
- M5: the console, record, schema and byte arms before the public
  header, the schema file, the writer and the printer.

### Parallel sets

| Wave | Sets that run beside each other |
| --- | --- |
| 1 | T002 with T003 (T001 runs first: the red run stands recorded) |
| 2 | T006 to T009 run beside T010 to T018; T005 and T010 land their `test/CMakeLists.txt` blocks in task order; each module's tasks run in task order on one test file |
| 3 | T019, T021 to T023 run beside T024 to T031; T020 and T024 land their `test/CMakeLists.txt` blocks in task order; each module's tasks run in task order on one test file; T051 runs beside T024 to T031 |
| 4 | T032 runs first; T033, T034 and T052 run beside each other; T035 runs after T032, beside T037, T038 and T052; T036 runs after T033, T034 and T035, beside T037, T038 and T052; T037 runs beside T032, T033, T034, T035, T036 and T052; T038 runs after T037, beside T032, T033, T034, T035, T036 and T052 |
| head | T039 to T047 and T050 all run beside each other, each in its own build tree |

Two tasks in one wave never touch one file outside the shared files the
table names. A test task that appends to a test file its predecessor
created runs in task order, and the pair shares no commit.

## Parallel example: wave 2

```text
Worker A (M2 graph-reader): T005, then T006, then T007, then T008, then T009
Worker B (M4 start-gate):   T010, then T011 to T016, then T017, then T018
```

Worker A owns `source/harness/graph_file.cpp`,
`docs/schema/graph-v1.json` and `test/source/harness_graph_file_test.cpp`.
Worker B owns `source/harness/graph_gate.cpp`, `source/harness/runner.cpp`
and `test/source/harness_graph_gate_test.cpp`. Each appends its own
labeled block to `test/CMakeLists.txt` in its own commit, so the pair
serializes by commit order and shares no line.

## Implementation strategy

### MVP first (the start rule)

1. Wave 1: the model, the words, the gate type, the seam (T001 to T004).
2. Wave 2: the reader arms (T005 to T009) and the gate (T010 to T018).
3. **Stop and validate**: `harness_graph_gate_test` passes under
   `taskset -c 0,1`, and the chain of SC-001 starts in the graph's
   order.

### Incremental delivery

1. Wave 1: the model stands testable on its own.
2. Wave 2: the refusals print one line and exit 2, and the gate orders
   one threaded run (the spec's US1 and US3).
3. Wave 3: the row arrives at the command line, and the console and the
   record carry the graph (US2, US3).
4. Wave 4: the version, the docs, the example and the evidence land
   (US5, US6).
5. Phase 7 and Phase 8: the gates and the records close the lane.

### Parallel team strategy

With several workers: one takes M2 and one takes M4 in wave 2, one
takes M3 and one takes M5 in wave 3, and three take M6, M7 and M8 in
wave 4. Each worker stays inside its module's file set and appends its
own labeled block to the shared files.
---

## Commit plan

- Atomic, bisectable commits, one idea each. Each commit compiles and
  passes its own tests, and stands sized to roll back alone.
- Title: `<Section>: <one-line imperative>`, 50 characters or fewer,
  the section naming the area (`Harness`, `runner`, `Docs`, `CMake`,
  `CI`, `Meta`, `test`).
- Body: the why, wrapped at 72 columns, omitted only for a trivial
  change.
- Footers: `Refs: specs/037-execution-graphs` on every commit, and
  `Fixes #n` where an issue exists. `gh issue list` names no open issue
  for the feature at task-writing time, so the lane carries `Refs:`
  alone and adds `Fixes #n` when an issue stands opened.
- No `Approved-by:` name stands invented here: the landing commit
  carries the reviewer's own (Pull Request Quality).
- The test commit stands apart from the code commit that turns it
  green, and the code commit names the tests it turns green.
- Three commits carry their rule's pair in one commit: the option row
  with the moved block comment (FR-002), the record keys with the moved
  top-level `x-order` (FR-005), the version with the harness `SOVERSION`
  and the ABI record (FR-003, FR-004). The schema minor rides the writer's
  commit (T030), the file that carries the value at
  `source/harness/json_report.cpp:36`.
- Linear history: rebase onto master, squash into the logical blocks
  the tasks name, land as a linear sequence.
- The task text names every commit title.
---

## Traceability: FR to task to test

Every FR-001 to FR-034 traces to at least one task and to that task's
test scenario.

| FR | Tasks | Test scenario |
| --- | --- | --- |
| (gate tasks) | T039 format, T040 spell, T041 prose, T043 sanitizers, T049 commit template | constitution VIII and Pull Request Quality, no FR |
| (records task) | T048 | SC-001 to SC-010, constitution X.4 |
| FR-001 | T019, T020, T022, T051 | `graphRowPrintsHelpAndOptions`, `graphRowValueBlockAndUniqueness`, the 37-row check |
| FR-002 | T019, T022, T051 | `graphRowValueBlockAndUniqueness`, the contract-order check |
| FR-003 | T032, T052 | `versionPrintsPlannedMinor` reads the version the root `CMakeLists.txt` carries |
| FR-004 | T027, T028, T032 | `reportDataGraphMemberRaisesAbi` |
| FR-005 | T026, T029, T030, T032 | `schemaCarriesGraphKeysAndOrder` |
| FR-006 | T027, T030, T052 | `noGraphBytesMatchMaster`, `versionPrintsPlannedMinor` |
| FR-007 | T005, T008, T009 | `readerChecksShapeAgainstSchemaFile` |
| FR-008 | T001, T002, T004, T007, T008 | `nodeNamesSlotAndDeps`, `fileCarriesNoExtraKeys` |
| FR-009 | T009, T020, T022, T023 | `graphEqualsFileReads`, `bareGraphTakesOptionFilePath`, `bareGraphNoPathRefuses` |
| FR-010 | T005, T006, T007, T009, T023 | `unreadableFileRefuses`, `parserRejectRefuses`, `schemaShapeBreakRefuses`, `repeatedNameRefuses`, `missingSlotRefuses`, `slotOutOfRangeRefuses`, `edgeNamesNoNodeRefuses`, `selfEdgeRefuses`, `cycleRefuses`, `unsatisfiableDependencyRefuses`, `firstFaultPrintsAlone` |
| FR-011 | T003, T004, T021, T023 | `graphParsesOnceBeforeRun`, `graphInputAnswersInstall` |
| FR-012 | T021, T023 | `countMismatchRefuses`, `noThreadedRunRefuses`, `singleContextTakesNoGraph` |
| FR-013 | T001, T002, T004, T010, T011, T017, T018, T034, T044 | `chainStartsInOrder`, `independentStartLowestSlotFirst`, `topologicalOrderRespectsSlots`, `example_drift` over the page's start-rule row |
| FR-014 | T011, T017 | `fanOutReleasesAll`, `fanInStartsAfterLast` |
| FR-015 | T012, T014, T018, T044 | `heldContextArrivesBothRounds` |
| FR-016 | T012, T017, T018 | `gateOpensNoWindow`, `time_source_gate` |
| FR-017 | T012, T018 | `placementBytesUnchangedBesideGraph` |
| FR-018 | T012 | `noexceptItemConceptHolds` |
| FR-019 | T001, T002, T013, T015, T017, T018 | `fourOutcomeWords`, `measuredRowSettlesAllFinished`, `skippedRowSettlesFinished`, `bodyErrorSettlesNodeFailed`, `setupFailureSettlesSlotZero`, `stopSettlesStoppedWithDependents` |
| FR-020 | T001, T002, T004, T014, T017 | `failedDependentsHoldWithReason`, `twoFailuresNameTheirGater` |
| FR-021 | T015, T017, T018 | `independentFinishAndStatusFollowsRunFailure` |
| FR-022 | T023, T024, T031 | `consoleBlockBesideContextsLine`, `blockOncePerRun` |
| FR-023 | T002, T024, T025, T026, T028, T029, T030, T031 | `recordKeysOrderAndReadBack`, `nodeOrderAscendingSlots`, `reasonNamesFailedNode` |
| FR-024 | T025, T030 | `recordCarriesNoTiming` |
| FR-025 | T003, T004, T010 | `seamInstallsAndClears`, `graphInputAnswersInstall`, `chainStartsInOrder` |
| FR-026 | T016, T017, T047 | `suitePassesOnTwoCpus`, `time_source_gate` |
| FR-027 | T016, T047 | `suitePassesOnTwoCpus`, the `taskset -c 0,1` run |
| FR-028 | T026, T045, T050 | the coverage figures at 100.0% lines and branches, no marker, and the DBC matrix complete |
| FR-029 | T002, T009, T042 | the tidy run over the new units, `harness_capture_macro_test` standing |
| FR-030 | T037, T038 | the record under `specs/037-execution-graphs/results/` |
| FR-031 | T037 | the script run, and the CI config naming no job for it |
| FR-032 | T033, T036, T046 | `example_drift` over the section and the quick-reference rows |
| FR-033 | T034, T036, T046 | `example_drift` over the guide page rows |
| FR-034 | T035, T036, T046 | `report_shape_execution_graphs_sample`, `example_run` |
---

## Notes

- No task adds a sleep, a clock read, a fixed cpu number, a fixed node
  count, an `LCOV_EXCL` marker, a new `--ignore-errors` entry, a lambda
  inside an `SG_*` macro, a second JSON reader, or code shaped for
  coverage (spec Constraints, FR-016, FR-026 to FR-029).
- A task closed only against the unoptimized build stays open: T042
  stands the release build every module needs before its tasks stand
  done (Principle IX).
- Every task closes the FRs its text names, and the traceability table
  above maps every FR-001 to FR-034 to its tasks and its scenarios.
- The version, the `SOVERSION`, the schema minor and the option value stand
  reservations under the rebase rule the Spec identity section of spec.md
  states. T032 records the version values beside the ABI check, and the
  commit that writes the option row records the value it took
  (FR-002 to FR-005, R-01, R-13).
