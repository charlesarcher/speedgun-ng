# Research: Execution Graphs

Feature: specs/037-execution-graphs (roadmap N11). Every decision below
resolves an open point of the spec; the audit point of the spec names
the tree the findings read (master `5443448`).

## D-1: the reader pipeline

- **Decision**: one fixed pipeline in one unit: read the file, parse
  with the vendored simdjson, check the document against
  `docs/schema/graph-v1.json` through the standing checker `checkReport`
  (`source/harness/json_load.cpp:2151`), then the node rules in the
  FR-010 order, first fault alone.
- **Rationale**: the spec fixes simdjson as the reader (D-1 of the
  spec) and requires the shape check to finish before the node rules
  (FR-007). `checkReport` already reads a schema file and compares the
  key set, the key order and the value kinds of a document, the shape
  `docs/schema/compare-v1.json` proves general (specs/026 D-14), so one
  checker covers the graph file and the report record with no second
  validator. The FR-010 order is the fixed check order: the ten faults
  run in the listed sequence, and the run prints the first fault it
  meets alone and stops (the clarify answer of 2026-10-29).
- **Alternatives considered**: a hand-written key walk beside the
  schema file: a second checker to keep in step with the schema, the
  pattern specs/026 rejected for the comparison document; a streaming
  simdjson pass: the file is small, the DOM form is the standing form
  of `json_load.cpp`, and the pipeline needs the whole document for the
  order check; rejected.

## D-2: the graph model and the slot-to-node map

- **Decision**: one node carries a name, one context slot and a
  dependency list; one edge is one dependency pair. The slot-to-node
  map is the bijection from the run's context indexes to the nodes.
  The four outcome words `finished`, `failed`, `held`, `stopped` stand
  in the model, and the model owns them; the work items stay
  `noexcept`.
- **Rationale**: the node count equals the context count per threaded
  run (FR-012) and every slot stands inside the count (FR-010), so the
  map is one node per slot exactly when the slots stand distinct. A
  repeated slot leaves a slot no node names, and the FR-010 fault
  `a dependency the run cannot satisfy` names it with the line naming
  the node and the slot (the Edge Cases entry
  `a slot the run does not start`). The words live in the graph because
  the spec fixes the state there (D-5): the items stay `noexcept`
  (FR-018) and the merged row names no node on its own.
- **Alternatives considered**: a slot-uniqueness fault of its own:
  FR-010 names a closed fault list and the repeated slot reads as the
  unsatisfiable-dependency fault the edge cases already name; an
  outcome word on the work item: the spec puts it out of scope
  (Out of scope, D-5); rejected.

## D-3: the start gate

- **Decision**: one `GraphGate` per threaded attempt, owned by
  `threadedSampleRun` beside the two barriers. The release fact is the
  completion state of specs/020 FR-049: a dependency stands finished
  when its dispatched work node reads `DONE`, and the gate reads that
  state with acquire order. A condition variable carries the wake; the
  finishing context notifies, and every waiter's predicate re-reads the
  completion states. Among ready nodes the gate releases the lowest slot
  first (FR-013).
- **Rationale**: FR-013 names the completion state the fact the gate
  reads, and FR-049 fixes the release/acquire chain, so the acquire
  read of `DONE` gives the gated context every write the dependency's
  work did. The notify-as-hint shape keeps the state the fact: a lost
  or spurious wake re-reads, and no clock, timeout or sleep stands in
  the gate (FR-016, FR-026). The lowest-slot-first release fixes which
  ready node starts next (FR-013), and FR-023 fixes the one order the
  console and the record print (SC-002).
- **Alternatives considered**: a dispatch chain, each finished body
  dispatching its dependents' items: the `threadRunner` ledger of
  specs/020 FR-021 requires every body index dispatched once before the
  runner callable returns, and a held context would receive no item and
  strand both rounds; a spin on the completion state: legal under the
  no-clock rule and wasteful on a 2-cpu host, and the condition
  variable is the standing wake shape; a per-node semaphore: it cannot
  express the lowest-slot-first order among ready nodes; rejected.

## D-4: the barrier arrival of a held context

- **Decision**: the gate stands inside the body, after
  `startBarrier.arriveAndWait()` and before the callable, beside the
  standing setup-failure skip at `source/harness/runner.cpp:2085`.
  `awaitStart` answers `run` or `skip`; a held or stopped node answers
  `skip`, and the context arrives at the stop round with no body run
  and no measured window open.
- **Rationale**: the hazard comment at `source/harness/runner.cpp:2030`
  names the rule: a body that never arrives strands the round, and a
  stranded round is a hung run. The setup-failure skip is the standing
  precedent in the same seat: it leaves every context out of the body,
  opens no window, and arrives at both rounds. The dispatch of every
  body index stays unchanged, so the `threadRunner` ledger, the default
  dispatch and the context lifecycle keep their bytes (FR-015).
- **Alternatives considered**: gating before the start round: a held
  context blocks before arriving and strands the round; a third
  barrier: FR-019 fixes the two rounds outside every window, and a
  third round adds a synchronizer the spec does not ask (X.2);
  dispatching the held context's item with a skip item: the item is
  already the body, and the gate's answer needs no second node;
  rejected.

## D-5: the record shape

- **Decision**: a top-level `graph` object with one key `runs`, an
  array with one entry per threaded run in batch order. Each entry
  names its run by the benchmark name and carries `nodes` (name, slot,
  outcome) in ascending slot order, `edges` (from, to),
  `held` (node, reason) and `stopped` (names). The record stands after
  `precheck` and before `benchmarks` in the top-level `x-order`; the
  schema minor takes `1.7`. `loadReport` reads it into the public
  `graph` member of `ReportData`.
- **Rationale**: the pre-check record set the pattern (specs/034
  FR-024): a top-level object, an array of entries, the writer
  parameter, the reader arm, the moved `x-order`, the byte identity
  for a run that names the feature absent. The benchmark name keys the
  entry so a reader folds it against the console block of its own run
  (the clarify answer of 2026-10-29: one entry per threaded run, one
  console block per threaded run). An array keeps the normative order
  the schema language carries; an object keyed by benchmark name would
  put the order in host map order.
- **Alternatives considered**: one entry per run keyed by benchmark
  name at the object level: no normative key order over dynamic names,
  and the checker's order rule has nothing to read; a per-benchmark
  record inside each benchmark object: the spec fixes the record
  top-level (FR-023); a timing figure beside the record: FR-024 bans
  it; rejected.

## D-6: the console block

- **Decision**: `printGraphReport` prints beside `printContextDisclosure`
  (`source/harness/report.cpp:167`), directly after the run's
  `contexts=` line: one line naming `nodes=` in ascending slot order and
  `edges=` in file order, one `graph held=<node>(<gating failed node>)`
  line naming every held node in ascending slot order when a node stands
  held, one `graph stopped=<names>` line naming every stopped node in
  ascending slot order when a node stands stopped. The entry point threads
  the run's record entry to `printResult` the way the hunt record rides
  `entryHuntRecords`.
- **Rationale**: FR-022 fixes the block beside the `contexts=` line,
  once per threaded run. The two-space field shape, the `=`-joined
  lists and the comma separation follow the standing disclosure
  (`contexts=2  ctx0=SPIN/0@0@0/applied`, `docs/pages/harness.md:971`).
  The hunt record's threading through `printResult` is the standing
  data path for a per-result fact, so `BenchmarkResult` grows by
  nothing and the public surface stays the record's (D-8).
- **Alternatives considered**: a block before the benchmark tables,
  the pre-check pattern: the spec fixes the block beside the
  `contexts=` line of each run (the clarify answer); a member on
  `BenchmarkResult`: a public growth the record's contract does not
  name; rejected.

## D-7: the seam

- **Decision**: one named detail seam `installTestGraph`, a value
  install of `std::optional<detail::Graph>` with the clear-on-null
  shape, declared in `detail/internal.hpp` beside `installTestMemoryBinder`
  (`:802`) and defined in `graph.cpp`. The entry point installs the
  graph it read once into the same slot `graphInput()`, and the runner
  reads the slot per threaded attempt.
- **Rationale**: FR-025 names the pattern: the way `installTestOnlineCpus`
  replaces the online set and `installTestMemoryBinder` replaces the
  binder. The layout seam installs a value (`topology.cpp:336`). The
  graph is a value the run reads, so the value form fits and no ops
  struct stands between the model and the gate. One slot serves the entry point's read and the tests'
  injection, so the runner reads one source and the parse-once rule of
  FR-011 stands at one call site.
- **Alternatives considered**: an ops struct like `MemoryBindOps`: the
  binder needs two calls and a denial arm; the graph needs one read,
  and the file arms run through planted files, so an ops table would
  serve no caller (X.2); a runner constructor parameter: the runner
  builds per instance and the graph is a batch fact, and the seam the
  spec names is an install seam; rejected.

## D-8: the ABI effect

- **Decision**: the project version takes `0.23.0`, the harness
  `SOVERSION` takes `14`, the report schema minor takes `1.7`, each a
  reservation: at rebase the lane reads master and takes the next free
  value (the Spec identity section of spec.md names the rule and the
  pull request #57 pairing). The archive `SOVERSION` stays `4`. The
  check stands recorded in the version commit.
- **Rationale**: the graph record joins the public loaded-report struct
  `ReportData` (`include/speedgun-ng/benchmark.hpp:753`), a public-ABI
  change, so the harness number rises by one (FR-004, D-8). The
  reservation rule and the #57 pairing stand in the Spec identity
  section of spec.md, over the specs/029 record
  (`specs/029-hunter-core/spec.md:44-49`): the lane takes the next free
  value of each at rebase. The public surface grows by the record's
  types alone; the graph arrives through the option row, and no handle
  method joins.
- **Alternatives considered**: keeping the record detail-internal and
  printing it: FR-023 requires `loadReport` to read it back into the
  public loaded-report struct; a version move without the ABI record:
  FR-003 requires the check beside the move; rejected.

## D-9: the coverage strategy for the refusal arms

- **Decision**: every FR-010 arm stands driven twice: taken once
  through a planted file under a temporary directory, passed once by
  the clean file that reaches the gate. The CLI arms of FR-009 and
  FR-012 stand driven through the option scanner and the scripted run.
  The gate's arms stand driven through injected graphs: chain,
  fan-out, fan-in, diamond, cycle, unsatisfiable dependency, rejected
  file (FR-025).
- **Rationale**: the coverage gate (VI, FR-028) needs both directions
  of every branch. The planted files reach the reader's real read,
  parse and schema arms, the same shape the pre-check tests use for
  fake `/proc` trees (specs/034 R-11). The injected graphs reach the
  gate's arms with no file, and the cycle and unsatisfiable arms reach
  the reader's rules through the same planted-file path, so each fault
  line, its one-line count and its status 2 stand asserted at the
  reader.
- **Alternatives considered**: driving the refusals only through the
  CLI process: the one-line and status assertions stand at the reader
  unit, and the CLI test adds the batch arms the reader cannot see
  (counts, no threaded run); a fault-injection seam into the reader:
  the file system is the reader's input, and planted files are the
  standing shape; rejected.

## D-10: the tsan story for the gate's atomics

- **Decision**: the gate's own state (the ready counts and the settled
  words) stands behind one mutex, and the condition variable
  carries the wake. The dependency's finish is read through the work
  node's completion state with acquire order, the FR-049 chain. The
  record's node order stands fixed after the attempt's join edge, the
  standing acquire read of the settled node states in `launchBodies`
  (FR-013).
- **Rationale**: the mutex serializes the gate's writers, so tsan sees
  no race on the gate's fields. The completion state is the one fact
  read outside the mutex, and FR-049 fixes its release and acquire
  orders, the chain tsan models natively. The finishing context calls
  `nodeFinished` under the mutex before its item returns, so the
  release store of `DONE` follows the notify and the predicate's
  acquire read orders the dependency's writes before the gated body.
  The `ci-tsan` preset and the `tsan` job run the graph suite at the
  head (Principle VIII).
- **Alternatives considered**: a lock-free ready count with
  fetch-subtract: legal, and it leaves the lowest-slot-first order and
  the record's node order to a second atomic protocol the mutex carries
  for free; relaxed reads of the completion state: FR-049 fixes
  acquire, and a relaxed read would drop the happens-before the gated
  body relies on; rejected.

## The evidence and the lane host

- **Decision**: one script `tools/graph/acceptance.py` produces the
  acceptance record under `specs/037-execution-graphs/results/`: a
  fan-out and fan-in graph over the lane host's 16 cores, the printed
  node order, the figures beside it, the boot, the commands
  and the exit codes. No CI job runs it.
- **Rationale**: specs/035 recorded the NPS evidence in this shape
  (plan.md plus one record per state, each naming the boot, the
  commands and the exit codes), and D-9 of the spec fixes the record
  and the script. The lane host's 16-core, 32-thread shape stands
  beside the 24-core, 48-thread record of specs/035, the shape of a
  box outside the fleet those lane records name.
- **Alternatives considered**: a CI job for the acceptance run: the
  spec puts it out of scope (D-9); the record alone with no script:
  FR-031 names the script; rejected.
