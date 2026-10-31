# Feature Specification: Execution Graphs

**Feature Branch**: `037-execution-graphs`

**Created**: 2026-10-29

**Status**: Draft

**Input**: User description, opening quoted:

> Give the run a work graph, item N11 of the roadmap (section 4,
> origin ruling H16): the contexts of a run stand as nodes of a graph
> with dependencies, fan-out and fan-in, and the harness starts each
> node when the nodes it depends on have finished. Today the run knows
> two shapes, the independent contexts of a threaded benchmark and the
> groups of spec 020's barrier; a benchmark whose stages feed each
> other cannot say so.

The request fixes the rulings G-1 to G-4; they stand in Decisions as
D-5 (the node state), D-6 (the seam), D-7 (the record) and D-9 (the
evidence). The request names the file format as this spec's first
decision; it stands as D-1.

## Spec identity and run-time binding

- Spec number: `037`. `specs/` on master `5443448` holds `001` through
  `030`, `032`, `034` and `035` (read 2026-10-29). The request names
  `031` as reserved for N0b host validation and `036` as reserved for
  N13e. No repository file records the `036` reservation; the `031`
  reservation stands in `specs/034-host-noise-precheck/spec.md:25` and
  `specs/035-numa-placement/spec.md:25`. `033`
  stands reserved for N14a-2 hunter budgets, and the remote carries
  `refs/heads/033-hunter-budgets` with pull request #57 open.
  `git ls-remote --heads` names `refs/heads/036-osnoise-timerlat-tracers`
  and no ref carrying `037` or `graph`, and `gh pr list --state open`
  names two open pull requests, #57 and #58 (read 2026-10-29).
- Branch: `037-execution-graphs`, cut from master `5443448` on
  2026-10-29. Master stands on that hash at branch time, so no
  moved-base record applies.
- Version: master stands at `0.22.0` (`CMakeLists.txt:19`), harness
  `SOVERSION 13` (`CMakeLists.txt:302`), archive `SOVERSION 4`
  (`CMakeLists.txt:84`). The lane plans `0.23.0`, the next free minor
  at branch time. A planned number is a reservation: at rebase the
  lane reads `CMakeLists.txt` on master and takes the next free minor,
  the rule specs/029 records (`specs/029-hunter-core/spec.md:44-49`).
  Pull request #57 takes `0.23.0` at its rebase, so this lane takes
  `0.24.0` when #57 lands first. The graph record joins the public
  loaded-report struct `ReportData`
  (`include/speedgun-ng/benchmark.hpp:753`, whose `precheck` member
  stands at `:786`), so the public ABI changes and the harness
  `SOVERSION` rises by one to `14`; the archive `SOVERSION` stays `4`;
  the check is recorded beside the version move, in the commit that
  moves it (FR-003, FR-004, D-8). The #57 branch carries harness
  `SOVERSION 14` (`origin/033-hunter-budgets:CMakeLists.txt:304`), so
  the lane takes `15` when #57 lands first.
- Report schema: master stands at `1.6` beside
  `source/harness/json_report.cpp:36`, the top-level `x-order` at
  `docs/schema/report-v1.json:7`. The report gains a graph record, so
  the schema minor plans the next free value `1.7`, a reservation under
  the same rebase rule, and it moves in the writer's commit, the commit
  that carries `source/harness/json_report.cpp`; the top-level `x-order`
  moves in the schema commit that carries the record's keys (FR-005, D-7). The #57 branch writes `1.7`
  (`origin/033-hunter-budgets:source/harness/json_report.cpp:38`), so
  the lane takes `1.8` when #57 lands first.
- Option values: one new row `--graph` joins the standing table and
  takes the value `297`, the first free value of the block `297` to
  `319` recorded at `source/harness/detail/options.hpp:76`, a
  reservation under the same rebase rule: at rebase the lane reads that
  block on master and takes the first free value. Pull request #58
  takes `297` for its `os-noise` row
  (`origin/036-osnoise-timerlat-tracers`), so the lane takes `298` when
  #58 lands first. The uniqueness checks over the table names and
  values, `test/source/harness_cli_table_test.cpp:780`, cover the row
  (FR-001, FR-002).

## Roadmap position

- Item N11 of the harness roadmap, section 4, origin ruling H16. N4
  (the topology placement, spec 025), N12 (the NUMA memory-node
  placement, spec 035) and N13b (the host noise pre-check, spec 034)
  stand merged. N9 (context groups) stands unstarted: the lane order
  at `specs/025-topology-placement/spec.md:228-229` puts N9 between
  N12 and N11, and the owner's choice takes N11 now.
  `specs/025-topology-placement/spec.md:726` names execution graphs,
  continuations and dependencies as N11's own scope.
- Spec 020 FR-045 (`specs/020-execution-contexts-core/spec.md:715`)
  ships the four H16 hooks: the node completion state, `dispatch()`
  from inside running work, `ContextId`, and the placement slot, with
  the meanings H16 reads and cannot change. This feature is H16's use
  of them and changes no signature of the four.
- Roadmap section 10.6 stays in force in full.

## Lane identity

- The acceptance shape stands on the request's lane host: an AMD Ryzen
  9 7950X3D, Zen 4, 16 cores, 32 threads, one socket. The box stands
  outside the fleet the lane records of specs 025, 027, 034 and 035
  name, and those records carry a 24-core, 48-thread shape. The
  acceptance record then carries a 16-core, 32-thread shape beside the
  24-core, 48-thread record of specs/035. The acceptance graph is a
  fan-out and fan-in shape over the 16 cores, and the printed node
  order (the record's node array order, ascending slot) is the
  acceptance axis (FR-030, D-9). The graphs need no
  specific hardware, and the lane takes the first free lane.
- The suite stays host-agnostic: every graph is injected, the cpus
  come through the online-set seam, and the suite passes on a 2-cpu
  host with no fixed node count (FR-025, FR-027). The request names
  the host; the spec carries the shape alone. The reading comes from
  `specs/032-fix-merged-features` FR-024, a rule scoped to the files
  pull requests 46, 47 and 49 touched, and specs/034's Lane identity
  applies the same reading to a lane host.

## Audit point

- Repository: `master` at `5443448` (project version `0.22.0`, report
  schema `1.6`, harness `SOVERSION 13`, archive `SOVERSION 4`), read
  2026-10-29 on the lane host.
- The threaded run starts its contexts behind two barrier rounds:
  `startBarrier` and `stopBarrier` at `source/harness/runner.cpp:2013`
  to `:2014`, arrived at `:2073` and `:2096`, outside every measured
  window (specs/020 FR-019). The comment at
  `source/harness/runner.cpp:2030` names the hazard this spec's start
  rule meets: a body that never arrives strands the round, and a
  stranded round is a hung run.
- The context surface: `ContextId` at
  `include/speedgun-ng/execution_context.hpp:52`, `PlacementSlot` at
  `:114`, `WorkNodeState` (`PENDING`, `RUNNING`, `DONE`, `STOPPED`) at
  `:457`, the release-written completion state at `:494`, `dispatch()`
  at `:630`, `ContextBarrier` at `:656` with its participant count
  fixed at construction (specs/020 FR-014).
- The context disclosure `printContextDisclosure` at
  `source/harness/report.cpp:167` prints `contexts=N` and one field per
  context, two spaces apart, in the form
  `contexts=2  ctx0=SPIN/0@0@0/applied  ctx1=SPIN/1@0@0/applied`
  (`docs/pages/harness.md:971`). A no-pin slot prints the two-field
  form at `:177-180`, and the unapplied marker stands at `:150`. The
  JSON carries the per-context `placement` field at
  `docs/schema/report-v1.json:358`, inside the key order at
  `:350-352`.
- The pre-check record stands as a top-level JSON object at
  `docs/schema/report-v1.json:156` inside the top-level `x-order` at
  `:7`, and `loadReport` (`include/speedgun-ng/benchmark.hpp:2815`)
  reads it into the public `PrecheckRecord` member of `ReportData` at
  `:786` (specs/034 FR-024).
- The option table stands at `source/harness/options.cpp:544`, one
  `OptionRow` per row (`source/harness/detail/options.hpp:116`).
  Twelve rows carry the environment flag, the closed list of
  `specs/028-full-command-line/spec.md:853-856` (FR-020).
- The row outcome words `MEASURED`, `SKIPPED`, `FAILED` stand at
  `include/speedgun-ng/benchmark.hpp:81-83`, the `RunOutcome` enum at
  `:79`.
- The seams a test installs: `installTestLayout` at
  `source/harness/topology.cpp:336` and `installTestOnlineCpus` at
  `:352`, both definitions. `installTestMemoryBinder` stands declared
  at `source/harness/detail/internal.hpp:802` and defined at
  `source/harness/topology.cpp:363`.
- The evidence record shape stands at
  `specs/035-numa-placement/results/`: `plan.md` plus one record per
  state. A record names its state and carries the commands and the
  exit codes of a boot that ran, and names the gap where no boot ran
  (`nps2.md`, `nps4.md`).
- The docs sections the feature joins: `docs/pages/harness.md:842`
  (Execution context lifecycle), `:919` (Topology placement), `:1436`
  (Jitter probe); the README option table at `README.md:836-873`; the
  example shape at `example/docs/numa_placement.cpp` with its
  `.expected` file.

## Standing facts this lane reads

- The context start's pin and bind, the placement records and the
  report machinery of specs 020, 025 and 035 stand unchanged; the
  graph orders the contexts the run already starts.
- The option-value blocks, the refusal shape and the schema rules of
  spec 025 stand in force; this spec amends no standing contract line.
  The environment list of specs/028 FR-020 stays the twelve rows it
  names: the `--graph` row carries no environment name.
- The four H16 hooks of specs/020 FR-045 keep their signatures and
  their meanings, and this feature gives them no second use.
- The `noexcept` work item of specs/020 FR-008 and the completion
  state of FR-049 stand unchanged (D-5).
- The roadmap's section 10.6 rules stay in force in full.

## Clarifications

### Session 2026-10-29

- Q: When a batch holds more than one threaded run, whose node order and
  whose held nodes does the one top-level `graph` record name, and does the
  console block print once per threaded run? -> A: The record carries
  one entry per threaded run, each naming its run by the benchmark name and
  carrying that run's nodes in ascending slot order, its edges, its held
  nodes and its stopped nodes; the console block prints once per threaded
  run, beside that run's
  `contexts=` line.
- Q: When a threaded run's merged row reads `FAILED`, which node settles
  `failed`, and what settles the nodes when a fixture setup fails every
  context? -> A: The row names no node on its own. The run reads the per-
  context outcome the harness records: the context whose body recorded an
  error settles its node failed, and a fixture setup failure settles the
  node at slot 0, the context that runs the setup half.
- Q: For two nodes with no dependency between them, what fixes the start
  order the console and the record print? -> A: FR-023 fixes the printed
  order: the record's node array and the console list nodes in ascending
  slot order. The live order follows the release rule of FR-013, the gate
  releasing the lowest ready slot first, so the file fixes no live order.
- Q: When a node settles `stopped`, what settles the nodes that depend on
  it, and what reason does the record carry for them? -> A: They settle
  `stopped` with it, the standing shape of specs/020 FR-050, and the record
  names each with no reason; the reason field names a failed node alone.
- Q: When one graph file holds several of the faults FR-010 lists, which
  fault does the run print, and how many lines does it print? -> A: One line
  naming the first fault in the order FR-010 lists, and the run stops there.

## Decisions

- **D-1: the graph file is one JSON document (the request's first
  decision).** Candidates: (a) a line-based file, the shape of the standing
  option file and rules file; (b) YAML through the vendored yaml-cpp; (c)
  JSON through the vendored simdjson reader the report machinery uses. (c)
  stands. The file the operator writes and the record the report carries
  then share one shape, so one reader, one schema language and one checker
  cover both. simdjson is the standing JSON reader: specs/032 Constraints
  binds a change to the JSON libraries the repository imports
  (`specs/032-fix-merged-features/spec.md:313`), specs/004-vendor-simdjson
  vendors the reader, and the report machinery reads through it at
  `source/harness/json_report.cpp:28`. A YAML path adds a second parser and
  a second schema language for one feature. The roadmap pairs YAML with the
  N9 context groups (`specs/025-topology-placement/spec.md:725`), and the
  roadmap names no format for N11. The graph rides no placement preset: it
  orders starts, and a preset plans cpus. `docs/schema/graph-v1.json` names
  the file's shape the way `docs/schema/compare-v1.json` names the
  comparison document's. A later spec that moves the input language moves
  the graph file with it.
- **D-2: one graph is one shape over the contexts of a threaded run.**
  Candidates: (a) the contexts of one named benchmark; (b) every
  threaded run of the batch, one graph applied to each; (c) one pool
  over the batch's contexts. (b) stands: it reads as the request's
  "the contexts of a run", keeps the timed path untouched, and needs
  no benchmark-name grammar in the file. A threaded run whose context
  count differs from the node count refuses one line with exit status
  2. A batch with no threaded run refuses one line with exit status 2.
  A single-context run takes no graph: one context has nothing to
  order. Each threaded run gates its own starts, and the record
  carries one entry per threaded run (D-7).
- **D-3: a node names itself, its context slot, and its
  dependencies.** Each node carries a name the operator chooses, one
  context slot (the context index in the run's plan order, counted
  from `0`), and a list of dependencies named by node name. An edge is
  one such dependency pair. The names are unique. The file carries no
  benchmark name, no preset, no cpu and no timing figure.
- **D-4: the start rule.** A node starts when every node it depends on
  has finished, and the completion state of specs/020 FR-049 is the
   fact the gate reads. A node with no dependency starts at the
   standing start of the run. Two nodes with no dependency between them
   follow the release rule of FR-013, and FR-023 fixes the printed order
   of the record and the console. Fan-out: one finished node releases every
  node that depends on it alone. Fan-in: a node with several
   dependencies starts after the last of them finishes. The gate
   stands in the body of each context, after the start round and
   before the stop round, outside every measured window and part of
   neither round, and every context of the run starts its thread and
   stands both rounds:
  a node that never starts settles held, and its context arrives at
  the rounds with no body and no measured window, so no round strands
  (the hazard at `source/harness/runner.cpp:2030`). The graph reads no
  clock and times nothing.
- **D-5: the node state (G-1, the roadmap section 12 risk 3).** The
  work items stay `noexcept` and no exception crosses the timed path.
  The graph carries the state, and the state is the report's: each
  node settles one outcome word, `finished`, `failed`, `held` or
  `stopped`. A threaded run has one merged row, so the row names no
  node on its own. A row that measures settles every node of its run
  finished, and a skipped row settles every node of its run finished.
  When a row reads `FAILED`, the run reads the per-context outcome the
  harness records: the context whose body recorded an error settles its
  node failed, and a fixture setup failure settles the node at slot 0,
  the context that runs the setup half. A node the stop request refuses
  settles stopped, the standing shape of specs/020 FR-050, and the
  dependents of a stopped node settle stopped with it. A failed node's
  dependents never start, the transitive closure holds with them, and
  the record names each held node with the failed node as its reason,
  the way the placement row names a pin the kernel did not apply
  (`source/harness/report.cpp:150`). The run continues with
  the nodes that stand on their own, and the exit status follows the
  standing run-failure status of specs/015. The status-2 shape stays
  reserved for what the run refuses before it starts.
- **D-6: the graph stands behind one named detail seam (G-2).** The
  seam is a named detail seam the tests replace, the way
  `installTestOnlineCpus` replaces the online set and
  `installTestMemoryBinder` replaces the binder. The tests drive every
  arm through synthetic graphs: a chain, a fan-out, a fan-in, a
  diamond, a cycle, an unsatisfiable dependency, a rejected file. CI
  needs no hardware and no fixed node count.
- **D-7: the record (G-3).** The console names the graph's nodes in
  the ascending slot order FR-023 fixes, beside the standing `contexts=`
  line of the run, and names the held nodes with their reasons. The
  block prints once per threaded run. The JSON gains a top-level
  `graph` record with one entry per threaded run, each entry naming its
  run by the benchmark name and carrying that run's nodes, edges, held
  nodes with their reasons and stopped nodes; the record stands after
  `precheck` and
  before `benchmarks` in the top-level `x-order`, the placement pattern
  the pre-check record set (specs/034 Assumptions). `loadReport` reads
  it back. The record carries no timing figure. The schema minor takes
  `1.7`, a reservation that takes `1.8` when pull request #57 lands
  first (Spec identity). A run with no
  `--graph` gives the same console report and the same JSON bytes as
  master apart from the two values FR-006 names.
- **D-8: the version and the ABI.** The project version takes
  `0.23.0` and the harness `SOVERSION` takes `14`, each a reservation
  under the rule of the Spec identity section, which names the values and
  the pull request #57 pairing. The graph record joins the public
  loaded-report struct, so the harness
  public ABI changes and its number rises by one; the archive
  `SOVERSION` stays `4`; the check is recorded beside the version
  move.
  The public surface grows by that record's type. The graph arrives
  through the option row, and the lane adds no handle method.
- **D-9: the evidence (G-4).** The acceptance run stands recorded on
  the lane host under `specs/037-execution-graphs/results/`: a
  fan-out and fan-in graph over the 16 cores, the printed node order,
  and the figures beside it, the way specs/035 recorded the
  NPS evidence. One script under `tools/` produces the record, and no
  CI job runs it.
- **D-10: the docs.** A root `README.md` section of at most 25 lines,
  the option and record detail in `docs/pages/harness.md`, one
  runnable example under `example/docs/` with its `.expected` file,
  and the quick-reference rows.

## Out of scope

- Context groups and the YAML input language: N9. This spec's file
  rides JSON (D-1), and a later spec that moves the input language
  moves the graph file with the group file.
- A graph that names a benchmark, a graph over one benchmark only, and
  a pool over a batch's contexts (D-2).
- Any change to the timed path: no node times anything, no barrier
  round moves inside a measured window, no work item changes shape,
  and no clock enters the graph (FR-016, FR-024).
- A node error state on the work item: the items stay `noexcept`, and
  the state lives in the graph record (D-5).
- Priorities, weights, deadlines, conditional edges, loops,
  sub-graphs, a cycle repair, a work-stealing scheduler, and a graph
  the run edits while it runs.
- A public graph type beyond the record's contract, and a
  `BenchmarkHandle` method for the graph (D-8).
- A CI gate for the acceptance run (D-9).
- The N9, N10, N13d and N13e lanes, and the reserved specs `031`,
  `033` and `036`.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - A graph orders the starts of one threaded run (Priority: P1)

A user writes a graph file with three nodes in a chain and runs a
three-context benchmark with `--graph=stages.json`. Context 2 starts
after context 1 finishes, context 1 after context 0 finishes, and the
console names that order. A benchmark whose stages feed each other can
now say so.

**Why this priority**: the start rule is the feature. Without it the
graph names nothing the run does, and the two standing shapes stay the
only shapes the run knows.

**Independent Test**: with an injected chain graph of three nodes and
a synthetic layout, the run starts the three contexts in the graph's
order, and the console and the record carry that order.

**Acceptance Scenarios**:

1. **Given** an injected chain `a -> b -> c` over three contexts,
   **When** the run starts, **Then** the start order reads `a`, `b`,
   `c`, and each node's gate saw its dependency finished.
2. **Given** the same run, **When** the report prints, **Then** the
   console names the nodes in the ascending slot order FR-023 fixes, beside the standing
   `contexts=` line, and the timed rows keep the bytes a graph-free
   run of the same benchmark gives.

---

### User Story 2 - The report carries the graph (Priority: P1)

A reader of the JSON document sees the graph: the nodes in ascending
slot order, the edges, and the nodes the run held. The
record sits beside the placement and pre-check records, and
`loadReport` reads it back.

**Why this priority**: a run whose order the report hides is a run a
reader cannot check, and the roadmap's disclosure rule makes the graph
part of the record set.

**Independent Test**: write a document for a diamond run, read it back
with `loadReport`, and fold the record's nodes, edges and settled words
against the run's own facts.

**Acceptance Scenarios**:

1. **Given** a diamond graph over four contexts, **When** the run
   writes its document, **Then** the top-level `graph` record names the
   four nodes, the four edges and the four settled words, and it stands
   after `precheck` and before `benchmarks` in the top-level `x-order`.
2. **Given** a run with no `--graph`, **When** its document is
   compared with master's, **Then** the bytes match apart from the two
   values FR-006 names: the `schemaVersion` value `1.7` and the
   `speedgun.version` value `0.23.0`, each a reservation the Spec
   identity section states.
3. **Given** a batch of two threaded runs, **When** the run
   writes its document, **Then** the `graph` record carries one entry
   per run, each naming its benchmark, and each console block stands
   beside the `contexts=` line of its own run.

---

### User Story 3 - A graph the run cannot start refuses before any context starts (Priority: P2)

A user names a file with a cycle, a dependency the run cannot satisfy,
or a shape the parser rejects. The run prints one line and exits with
status 2, and no benchmark output prints.

**Why this priority**: a graph the run cannot honor would order
nothing, and a silent fall-back to the standing shape would record a
run the user did not ask for.

**Independent Test**: drive each refusal through an injected graph and
a planted file, and check one line, status 2, and no benchmark output.

**Acceptance Scenarios**:

1. **Given** an injected graph with a cycle of two nodes, **When** the
   run starts, **Then** one line names the cycle, the exit status is
   2, and no context starts.
2. **Given** a file the parser rejects, **When** the run starts,
   **Then** one line names the file and the fault, the exit status is
   2, and no context starts.
3. **Given** a graph of three nodes over a two-context run, **When**
   the run starts, **Then** one line names the node count and the
   context count, the exit status is 2, and no context starts.
4. **Given** a file with a repeated node name and a cycle, **When** the
   run starts, **Then** one line names the repeated name, the exit
   status is 2, and no context starts.

---

### User Story 4 - A failed node holds its dependents and the record names them (Priority: P2)

A context's body records an error. The nodes that depend on that
context's node never start, and the record names each held node with
the failed node as its reason. The run finishes the nodes that stand on
their own, and the exit status follows the standing run-failure
status.

**Why this priority**: the roadmap's risk 3 is the open question of
the item, and a held node that no record names is a silent gap in a
benchmarking report.

**Independent Test**: with an injected graph and a body that fails,
check that the dependents never start, that the record names each held
node with its reason, and that the independent nodes finish.

**Acceptance Scenarios**:

1. **Given** an injected graph where the body on `a`'s context throws
   and `b` depends on `a`, **When** the run finishes, **Then** `a`
   settles failed, `b` never starts, the record names `b` as held with
   `a` as its reason, and the exit status is the standing run-failure
   status.
2. **Given** a diamond where the shared dependency fails, **When** the
   run finishes, **Then** the node that depends on it is held, the
   node with no dependency finishes, and the console names both facts.
3. **Given** a threaded run whose row is skipped, **When** the run
   finishes, **Then** every node of the run settles finished and every
   dependent starts.
4. **Given** a fixture setup that fails on context 0 of a threaded run,
   **When** the run finishes, **Then** the node at slot 0 settles
   failed, its dependents settle held with that node as their reason,
   and the exit status is the standing run-failure status.

---

### User Story 5 - Fan-out and fan-in over the lane host stand recorded (Priority: P2)

The lane records one acceptance run on the lane host: a fan-out and
fan-in graph over the 16 cores, the printed node order, and the figures
beside it. The record carries a 16-core, 32-thread shape beside the
24-core, 48-thread record of specs/035, the shape of a box outside the
fleet those lane records name. A later lane reads the record and learns
from its fixed order the node set, the dependency edges and the settled
words.

**Why this priority**: the recorded run is the item's acceptance axis
on the lane host, and the record is what the later noise lanes read.

**Independent Test**: the script under `tools/` runs the graph and
writes the record; the record names the boot, the commands, the exit
codes, the printed node order and the figures.

**Acceptance Scenarios**:

1. **Given** the lane host and the recorded graph, **When** the script
   runs, **Then** the record names the boot, the commands, the exit
   codes, the printed node order and the figures beside it.
2. **Given** the record, **When** the lane closes, **Then** the
   results directory links it and no CI job ran the script.

---

### User Story 6 - The docs teach the graph (Priority: P3)

A reader of the README and the guide page learns the `--graph` row,
the file's shape, the start rule and the held-node rule from the
sections that teach the standing run shapes, and the runnable example
shows a graph run whose bytes the shape gate guards.

**Why this priority**: the docs gates bind every feature, and the
drift gates fail a page whose quoted run drifts from what the binary
prints.

**Independent Test**: the example builds and runs, its `.expected`
file folds against the run through the shape gate, and the snippet
compiles.

**Acceptance Scenarios**:

1. **Given** the guide page's recorded example run, **When** the shape
   gate runs the example binary, **Then** the quoted block folds equal
   against the run.
2. **Given** the README section, **When** the prose gate runs,
   **Then** the section stands inside the 25-line cap with no finding.

### Edge Cases

- A graph of one node over a one-context threaded run: the node starts
  at the standing start, and the record names one node and one edge
  list that stands empty.
- A node with no dependency and no dependent: it starts at the
  standing start, the gate releasing the lowest ready slot first
  (FR-013), and the record names it as both a source and a sink.
- A self-edge, a node that depends on itself: one line names the node,
  exit status 2, before any context starts.
- A cycle of two or more nodes: one line names the cycle, exit status
  2, before any context starts.
- A disconnected component: each component orders its own starts, and
  the record lists every node of the run in the ascending slot order FR-023 fixes.
- A dependency the run cannot satisfy, a slot the run does not start:
  one line names the node and the slot, exit status 2, before any
  context starts.
- A repeated node name, a node with no slot, an edge naming no node:
  one line each, exit status 2, before any context starts.
- A file that holds two faults: one line names the first fault in the
  order FR-010 lists, and the run stops there.
- A file that cannot be read, and an empty file with no nodes: one
  line each, exit status 2, before any context starts.
- `--graph` with no path in any source: one line names the row and the
  missing path, exit status 2, before any context starts.
- A batch with a single-context run beside a threaded run: the graph
  orders the threaded run, the single-context run keeps its standing
  shape, and the record names the threaded run alone.
- A batch of two threaded runs: the record carries two entries, each
  naming its benchmark, and each console block stands beside its own
  run.
- A batch with no threaded run at all: one line, exit status 2.
- A stop request while a node stands gated: the node settles stopped,
  the standing shape of specs/020 FR-050, its dependents settle stopped
  with it, and the record names each with no reason.
- A node whose dependency is held: it is held too, and the reason
  names the nearest failed node on the dependency chain.
- A threaded run whose merged row reads `FAILED` after one context's
  body throws: the node at that context's slot settles failed, the
  other nodes of the run settle finished, and the dependents of the
  failed node hold.
- A fixture setup that fails: the setup runs on context 0, the node at
  slot 0 settles failed, and its dependents hold.
- A graph beside a placement preset: the preset plans the cpus and the
  graph orders the starts, and each disclosure keeps its own line.
- A graph beside the jitter probe and the pre-check: the probe windows
  stand before any context starts, and the graph gates context starts
  alone.
- `--topology` beside `--graph`: the listing prints and the run exits
  0, the standing rule that the listing runs before selection.
- A host whose online set holds none of the planned cpus: the pin's
  account reads unapplied, the standing shape, and the record lists the
  graph's nodes in the ascending slot order FR-023 fixes.

## Requirements *(mandatory)*

### The surface, the version and the schema (A)

- **FR-001**: The option table SHALL gain one row named `graph`
  with the argument kind `OPTIONAL` (`--graph` alone, `--graph=<file>`),
  the environment flag off, help text, and the owner `specs/037`; the
  `--help` and `--options` output SHALL print the row.
- **FR-002**: The row SHALL take the internal value `297`, the first
  free value of the free block `297` to `319` recorded at
  `source/harness/detail/options.hpp:76`, and at rebase the lane SHALL
  read that block on master and take the first free value under the
  same reservation rule, with `298` standing when pull request #58
  lands first; the block comment SHALL move in the same commit as the
  row; the table checks over names and values SHALL cover the row.
- **FR-003**: The project version SHALL move to `0.23.0`, the next free
  minor at branch time. At rebase the lane SHALL read master and take
  the next free minor, the reservation rule of specs/029
  (`specs/029-hunter-core/spec.md:44-49`), and `0.24.0` stands the
  value when pull request #57 lands first. The commit that moves the
  version SHALL record the ABI check beside the move.
- **FR-004** (D-8): The harness `SOVERSION` SHALL rise from `13` to
  `14` because the graph record joins the public loaded-report struct.
  At rebase the lane SHALL take the next free number under the same
  reservation rule, and `15` stands the value when pull request #57
  lands first. The archive `SOVERSION` SHALL stay `4`; no `SOVERSION`
  SHALL go down.
- **FR-005** (D-7): The report schema minor SHALL take `1.7`, the next
  free value, and at rebase the lane SHALL take the next free minor
  under the same reservation rule, with `1.8` standing when pull
  request #57 lands first; the graph record's keys, one entry per
  threaded run, SHALL join `docs/schema/report-v1.json`, and the
  top-level `x-order` SHALL move in the same commit as those keys; the
  minor SHALL move in the commit that writes the record.
- **FR-006** (D-7): A run with no `--graph` SHALL produce the same
  console report and the same JSON bytes as master apart from the
  `schemaVersion` value and the project version the version move
  carries, which stands in the `speedgun.version` value and in the
  console version line.

### The file and its refusals (B)

- **FR-007** (D-1): The graph file SHALL be one JSON document read
  through the vendored simdjson reader, and `docs/schema/graph-v1.json`
  SHALL name its shape with a normative key order, the way
  `docs/schema/compare-v1.json` names the comparison document's; the
  reader SHALL check a file's key set, key order and value kinds
  against that schema file. The reader SHALL finish the shape check
  before the node rules of FR-010.
- **FR-008** (D-3): The file SHALL name each node by a unique name, one
  context slot counted from `0`, and a list of dependencies named by
  node name; the file SHALL carry no benchmark name, no preset, no cpu
  index and no timing figure.
- **FR-009**: `--graph=<file>` SHALL read the named file. `--graph`
  alone SHALL take the path from the standing lower source, the option
  file line, and SHALL read no environment variable for the row. When
  no path stands in any source, the run SHALL print one line naming the
  row and the missing path and exit with status 2 before any context
  starts.
- **FR-010** (D-2, D-4): Each fault below SHALL print one line and exit
  with status 2 before any context starts: a file the run cannot read;
  a file the parser rejects; a document that breaks the schema file's
  shape; a repeated node name; a node with no slot; a slot outside the
  run's context count; an edge naming no node; a self-edge; a cycle of
  two or more nodes; a dependency the run cannot satisfy. The run SHALL
  check in the order listed, SHALL print the first fault it meets
  alone, and SHALL stop there.
- **FR-011**: The graph SHALL be parsed once, before the first context
  starts, the reading of the one layout read at specs/025 FR-002
  (`specs/025-topology-placement/spec.md:477-479`). The command line's
  text parses once ahead of any run at `source/harness/cli.cpp:560-563`
  (specs/028 FR-011, FR-038), and the graph follows that shape.
- **FR-012** (D-2): The graph SHALL apply to every threaded run of the
  batch. A threaded run whose context count differs from the node
  count SHALL print one line naming both counts and exit with status
  2. A batch with no threaded run SHALL print one line and exit with
  status 2. A single-context run SHALL take no graph and SHALL gain no
  record entry.

### The start rule (C)

- **FR-013** (D-4): A node SHALL start when every node it depends on
  has finished, and the completion state of specs/020 FR-049 SHALL
  stand the fact the gate reads. A node with no dependency SHALL start
  at the standing start of the run: the point where the run has
  dispatched every body index and every context has arrived at the
  start round. Among the nodes ready to start the gate SHALL release
  the lowest slot first. FR-023 SHALL fix the printed order of the
  record and the console.
- **FR-014** (D-4): A finished node SHALL release every node that
  depends on it alone, and a node with several dependencies SHALL start
  after the last of them finishes.
- **FR-015** (D-4): Every context of a graph run SHALL start its thread
  and SHALL stand both barrier rounds of specs/020 FR-019. A node that
  never starts SHALL settle its context's arrival at both rounds with
  no body run and no measured window open, so no barrier round strands.
- **FR-016** (D-4): The graph SHALL gate the start of a context and
  SHALL touch nothing inside it: no measured window opens, closes or
  shifts because of the graph, no barrier round moves inside a window,
  and the graph reads no clock.
- **FR-017**: The graph SHALL change no placement, no pin, no memory
  bind, no wait mode, no plan and no recorder: the placement rules of
  specs/025 and specs/035 stand unchanged beside it.

### The node state (D)

- **FR-018** (D-5): The work items SHALL stay `noexcept`, no exception
  SHALL cross the timed path, and the graph SHALL add no error state to
  a work item.
- **FR-019** (D-5): Each node SHALL settle exactly one outcome word:
  `finished`, `failed`, `held` or `stopped`. A measured row SHALL
  settle every node of its run finished, and a skipped row SHALL settle
  every node of its run finished. When a threaded run's merged row
  reads `FAILED`, the context whose body recorded an error SHALL settle
  its node failed, and a fixture setup failure SHALL settle the node at
  slot 0, the context that runs the setup half. A node the stop request
  refuses SHALL settle stopped, the standing shape of specs/020 FR-050,
  and its dependents SHALL settle stopped with it.
- **FR-020** (D-5): A failed node's dependents SHALL not start, the
  transitive closure of dependents SHALL hold with them, and the record
  SHALL name each held node with the failed node as its reason, the way
  the placement row names a pin the kernel did not apply. When two
  nodes fail, each held node SHALL name the failed node that gates it.
- **FR-021** (D-5): The run SHALL finish the nodes that stand on their
  own, and the exit status SHALL follow the standing run-failure status
  of specs/015. The status-2 refusal shape SHALL stay reserved for what
  the run refuses before it starts.

### The disclosure (E)

- **FR-022** (D-7): The console SHALL name the graph in one block
  beside the standing `contexts=` line of each threaded run, in the
  printed order FR-023 fixes, and SHALL name each held node with its
  reason. The block SHALL print once per threaded run.
- **FR-023** (D-7): The JSON SHALL gain a top-level `graph` record
  with one entry per threaded run; each entry SHALL name its run by
  the benchmark name and SHALL carry that run's nodes, edges, held nodes
  with their reasons and stopped nodes; the record SHALL stand
  after `precheck` and before `benchmarks` in the top-level `x-order`;
  `loadReport` SHALL read it back into the public loaded-report
  struct. The node array SHALL list the nodes in ascending slot order,
  and the console SHALL print that order. The reason field SHALL name a
  failed node. The stopped list SHALL name the stopped nodes in ascending
  slot order and carries no reason (FR-019, the fourth Clarifications
  answer).
- **FR-024** (D-7): The record SHALL carry no timing figure, and the
  graph SHALL time nothing.

### The seam and the suite (F)

- **FR-025** (D-6): The graph SHALL stand behind one named detail seam
  that tests replace, the way `installTestOnlineCpus` replaces the
  online set and `installTestMemoryBinder` replaces the binder; the
  tests SHALL drive every arm through synthetic graphs: a chain, a
  fan-out, a fan-in, a diamond, a cycle, an unsatisfiable dependency
  and a rejected file.
- **FR-026**: No test SHALL sleep and no test SHALL read a clock: the
  start order SHALL come from the injected graph.
- **FR-027**: The suite SHALL pass under `taskset -c 0,1` and on a
  2-cpu host, with the cpus named through the online-set seam and no
  test carrying a fixed node count.
- **FR-028**: Coverage SHALL read 100% lines and 100% branches, and the
  DBC completeness gate of Principle VI SHALL read complete over every
  new unit; no `LCOV_EXCL` marker SHALL join and no new
  `--ignore-errors` entry SHALL join.
- **FR-029**: No lambda SHALL stand inside an `SG_*` macro, and
  simdjson SHALL stand the only JSON reader the change uses.

### The evidence (G)

- **FR-030** (D-9): The results directory SHALL carry one record of the
  acceptance run on the lane host: a fan-out and fan-in graph over the
  16 cores, the printed node order (the record's node array order,
  ascending slot), and the figures beside it;
  the record SHALL name the boot, the commands and the exit codes, the
  way specs/035 recorded the NPS evidence. The record SHALL name the
  host's 16-core, 32-thread shape, beside the 24-core, 48-thread shape
  specs/035 recorded.
- **FR-031** (D-9): One script under `tools/` SHALL produce that
  record, and no CI job SHALL run it.

### The docs and the example (H)

- **FR-032** (D-10): The root `README.md` SHALL carry a section of at
  most 25 lines teaching the graph, and the quick-reference tables
  SHALL carry the option row and the record rows.
- **FR-033** (D-10): `docs/pages/harness.md` SHALL carry the option and
  record detail beside the placement and pre-check sections.
- **FR-034** (D-10): One runnable example under `example/docs/` SHALL
  carry a graph run with its `.expected` file, and the shape gate SHALL
  fold it against the binary's own run.

## Key Entities *(include if feature involves data)*

- **The graph file** (`docs/schema/graph-v1.json`): the operator's
  input, one JSON document naming the nodes, each with a name, a
  context slot and a dependency list.
- **The graph record** (`docs/schema/report-v1.json`, the writer,
  `loadReport`): the top-level record with one entry per threaded run,
  each naming its run by the benchmark name and carrying that run's
  nodes in ascending slot order, the edges, the held nodes with their
  reasons and the stopped nodes.
- **The node**: one name, one context slot, one dependency list, and
  one outcome word (`finished`, `failed`, `held`, `stopped`).
- **The edge**: one dependency pair of node names, read in the
  direction a finished node releases a gated node.
- **The graph seam** (a named detail seam under `source/harness/`):
  the injected graph the tests install, the way the layout, online-set
  and binder seams stand.
- **The `--graph` row** (`source/harness/options.cpp:544`, value
  `297`): the one option the feature adds.
- **The public record member**: the graph record's type inside
  `ReportData` (`include/speedgun-ng/benchmark.hpp:753`), the public
  surface the ABI check of FR-003 records.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A chain graph of three nodes over three contexts starts
  in the graph's order, and the console names that order.
- **SC-002**: A diamond graph of four nodes and four edges starts each
  node after both its dependencies finish, and the record's node order
  folds equal against the console's order. Independent nodes stand in
  ascending slot order FR-023 fixes in both.
- **SC-003**: Every refusal of group B prints one line and exits with
  status 2, and no benchmark output stands ahead of the refusal. A file
  with several faults prints the first fault in the order FR-010
  lists.
- **SC-004**: A failed node's dependents never start, the record names
  each held node with its reason, the independent nodes finish, and the
  exit status follows the standing run-failure status. A merged row
  that fails names the node at the failing context's slot, and a
  fixture setup failure names the node at slot 0.
- **SC-005**: A run with no `--graph` matches master's console report
  and JSON bytes apart from the `schemaVersion` value and the
  `speedgun.version` value the version move carries.
- **SC-006**: `loadReport` reads the graph record back from a written
  document, and the schema carries the keys with the moved `x-order` at the
  planned minor, `1.7` or the next free minor at rebase. A batch of two
  threaded runs gives two entries, and each entry folds equal against the
  console block of its own run.
- **SC-007**: Coverage reads 100.0% lines and 100.0% branches, the DBC
  gate reads complete, and no `LCOV_EXCL` marker and no new
  `--ignore-errors` entry stands in the tree.
- **SC-008**: The suite passes under `taskset -c 0,1` and on a 2-cpu
  host, with no sleep and no clock in any test and no fixed node count.
- **SC-009**: The acceptance record stands under
  `specs/037-execution-graphs/results/` with the printed node order (the
  record's node array order, ascending slot) and the figures beside it,
  and no CI job ran the script.
- **SC-010**: The docs gates stand green: the example drifts from
  nothing, the snippet compiles, the run gate runs the example, and the
  prose gate reads no finding.

## Constraints

The request's hard rules bind this spec; they restate constitution VI,
VII and VIII:

- Coverage stays 100% lines and 100% branches; no `LCOV_EXCL` marker,
  no waiver, no new `--ignore-errors` entry; no code shaped for
  coverage.
- No sleep and no clock in a test: the start order comes from the
  injected graph.
- No fixed cpu number and no fixed node count: the cpus come through
  the online-set seam, and the suite passes under `taskset -c 0,1` and
  on a 2-cpu host.
- The timed path stays raw: no graph work inside a measured window, no
  barrier round inside a window (specs/020 FR-019), and no new time
  source (the counters-only constraint of Additional Constraints).
- No lambda inside an `SG_*` macro; `[[maybe_unused]]` on
  contract-only helpers; simdjson only for JSON.
- The public surface grows only where the record's contract names it;
  a `SOVERSION` rises only for a public-ABI change; the archive
  `SOVERSION` stays `4`; the check rides the version commit.
- The graph adds no hard runtime dependency: the vendored simdjson
  reader carries the file read (Additional Constraints).
- The roadmap's section 10.6 rules stay in force in full.
- Each new commit follows the commit template and passes `prose-lint`.

## Assumptions

- The bare `--graph` reading (the path from the option file, else a
  status-2 refusal) is an informed default. The request fixes the
  grammar `--graph[=<file>]` and names no bare behavior. Candidates:
  refuse; a default path; the standing lower source. The lower source
  stands, because the option file serves every row (specs/028 FR-035)
  and the environment list stays the twelve rows of FR-020.
- The graph applies to every threaded run of the batch (D-2). The
  request says "the contexts of a run" and names no batch shape.
- The node slot is the context index in the run's plan order. The
  request names nodes and dependencies and fixes no mapping.
- The record's place in the top-level `x-order` (after `precheck`,
  before `benchmarks`) follows the pre-check record's own pattern; the
  schema rule fixes the minor and not the placement.
- The console block's exact line shape is a plan decision; the record's
  keys stand in the schema file, and the console names the same facts.
- The graph gates the body of a context, and the context thread starts
  as the run starts it today (D-4). The plan chooses the mechanism that
  arrives a held context at both barrier rounds.
- The live start order among nodes with no dependency between them
  follows the gate's release policy of FR-013, so the file fixes no
  start order. The printed bytes stand fixed by the ascending slot
  order of the record.
- The roadmap stands outside the repository, the reading
  `specs/025-topology-placement/spec.md:734-737` and
  `specs/034-host-noise-precheck/spec.md:616-617` record, and the file
  stands absent from this machine. Its section 10.6 rules, its section
  12 risk 3, its disclosure rule and its reading that the roadmap names
  no format for N11 are read from the lane record the request carries.
- The request names the lane host; the spec carries the hardware shape
  alone (Lane identity). The reading follows specs/032 FR-024, a rule
  scoped to the files pull requests 46, 47 and 49 touched, and specs
  034 and 035 carry the same reading for a lane host.

## Constitution Checklist

- Main-page impact check (IV.1): the feature changes what the root
  `README.md` teaches, so the page section, its `example/` region and
  the quick-reference rows move in the same change (FR-032, FR-034),
  and the `example_run` gate covers the example.
- Principles VI and VIII: the coverage bar, the determinism rules and
  the gate set stand as binding requirements (Constraints).
- Principle VII: the graph gates context starts outside every measured
  window and adds no timed-path work (FR-016, FR-024).
- Principle IX: the full workflow applies; the change touches behavior,
  the option table, the report schema and one public struct.
- Principle X.1: the open readings (the file format, the batch shape,
  the bare `--graph`, the record's placement) stand recorded with their
  candidates and the chosen one (D-1, D-2, Assumptions). The
  Clarifications section records the five settled readings.
- Principle X.2: the feature adds one option row, one file reader, one
  record and one seam; the Out of scope list names what a larger design
  would add, and this spec adds none of it.
- Principle X.3: each commit touches the files its requirement names,
  and nothing else.
- Principle XI: this file holds no em-dash, no contrastive framing, no
  voucher, no filler and no marketing word, and every code span stands
  on one line.
