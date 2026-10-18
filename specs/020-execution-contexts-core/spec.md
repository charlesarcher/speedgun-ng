# Feature Specification: Execution Contexts Core

**Feature Branch**: `020-execution-contexts-core`

**Created**: 2026-10-15

**Status**: Draft

**Input**: User description, opening quoted, the full text held by the
specify invocation record:

"Execution contexts core, spec H5 of the speedgun harness roadmap. Keep
this spec simple. An execution context (EC) is one thread with a simple
event loop. Work goes to it through a public `dispatch()`. The Google
Benchmark thread features are built on ECs. This spec builds no graph
engine. Decisions by Charles (2026-10-15), recorded as decided: D-1 an
execution context is one thread with an event loop of dispatched work,
configured by a policy, and a benchmark test case is one kind of work it
hosts; D-2 a work item is strongly typed, a C++23 concept over a
type-erased `fn(void*)`-style path; D-3 `ExecutionContext` and
`dispatch()` are public API, for example a noise load runs on one
execution context while a benchmark runs on another. Capabilities C-1 to
C-7: the context, its policy, typed work and dispatch, a context
barrier, Google Benchmark threads on contexts, the docs and examples,
and the policy disclosure in the report. Hooks kept for later execution
graphs (H16). Scope, constraints, and success criteria SC-01 to SC-12.
[...]"

## Clarifications

### Session 2026-10-15

- Q: Which library target, project version, and harness `SOVERSION` does
  the context core ship under? → A: the harness archive carries the
  context core, with no new target; version 0.10.0; harness `SOVERSION`
  from 5 to 6; a separate `speedgun-ng::exec` target can come with H16.
- Q: Which wait mode is the default? → A: `SPIN` for a context that runs
  a benchmark; `BLOCK` for a context a caller creates with no policy.
- Q: What waiting mechanism does `ContextBarrier` use? → A: a small
  atomic counter that waits in the context's own wait mode, so a
  spinning context never sleeps inside a barrier; `std::barrier` is the
  fallback for `BLOCK`.
- Q: Where do `setUp` and `tearDown` run for a threaded run? → A: once
  per run on context 0, outside both barriers; every difference from the
  cited Google Benchmark revision is recorded in the feature's records.
- Q: How does the page line budget absorb the two new sections? → A:
  `PAGE_LINE_CAP` rises by the measured size of the two new sections and
  their quick-reference rows, recorded in this spec's budget note at
  implementation time; `SECTION_LINE_CAP` stays 25; prose is trimmed
  first; no snippet is dropped; no earlier spec is edited. The decision
  is the rule and its measurement point; the number itself is measured
  when the sections land.
- Q: Does any command-line option land in this feature? → A: none; a
  `--threads` override waits for H9.

## Decisions

Charles recorded these three on 2026-10-15 as decided. They are
governance for this feature and for every later roadmap spec that builds
on the context.

- **D-1 The context is a thread with an event loop of dispatched work.**
  A policy configures it. A benchmark test case is one kind of work the
  loop hosts, beside any other kind a caller dispatches.
- **D-2 A work item is strongly typed.** The type is a C++23 concept
  over a type-erased `fn(void*)`-style path, so the loop runs items of
  any approved type through one erased path with no run-time type test.
- **D-3 `ExecutionContext` and `dispatch()` are public API.** A caller
  creates a context, hands it work, and reads where that work ran. The
  example the request names: a noise load runs on one context while a
  benchmark runs on another.

## Audit point

| Field | Value |
| --- | --- |
| Audit point | `64f2ab553fee9753a56f9fba08d42a506aac3af0` |
| Audit-point date | 2026-10-15 02:47:01 -0500 (commit date; the author date is 2026-10-15 02:18:01 -0500) |
| Default branch at run time | `master`, tip `64f2ab5`, equal to the `origin/master` tip |
| Working tree at run time | clean at that commit, so the read saw the audit-point content byte for byte |
| Feature number at run time | 020, assigned by `.specify/scripts/bash/create-new-feature.sh` as the next free number under `specs/` |
| Short name at run time | `execution-contexts-core` |
| Git branch at run time | none created: `.specify/extensions.yml` carries no `before_specify` hook, so this command created the directory on `master` and the orchestrator owns every commit |
| Project version at the audit point | 0.9.0 (the `project()` VERSION declaration in `CMakeLists.txt`) |
| Harness `SOVERSION` at the audit point | 5 (the harness target `SOVERSION` in `CMakeLists.txt`) |
| Counters `SOVERSION` at the audit point | 2 (the counters target `SOVERSION` in `CMakeLists.txt`) |
| Package compatibility at the audit point | `SameMinorVersion` (the `COMPATIBILITY` clause in `cmake/install-rules.cmake`) |
| Main page at the audit point | `README.md` at 570 lines, the page cap of the drift gate (PC-4) |
| Constitution version at the audit point | 2.20.0, lineage row and footer equal |

Every citation below was read at that content. The request names the
same audit point, and `git merge-base --is-ancestor 64f2ab5 HEAD` exits
zero at it.

## Preconditions

Each precondition was checked in the code and the cited artifacts at the
audit point. The result and its evidence follow.

### PC-0: H4 has merged. PASS.

- H4 is `specs/019-harness-user-counters-and-statistics`, and the audit
  point is the exact commit the request records as its merge: `64f2ab5`
  ("Harness: Drop the fold-sink assertion") is both the audit point and
  an ancestor of it, so the check passes at the head of `master` and of
  `origin/master`.
- The H4 artifact set stands at the audit point with `spec.md`,
  `plan.md`, `research.md`, `data-model.md`, `contracts/`,
  `quickstart.md`, `tasks.md`, and `checklists/`, and its task ledger
  carries zero open `- [ ]` boxes.
- The names this spec builds on were confirmed at the audit point, with
  file and symbol:
  - `State`, `BenchmarkHandle`, `ResultRow`, `BenchmarkResult`,
    `CounterFlag` with `AVG_THREADS`, and `speedgunMain`
    (`include/speedgun-ng/benchmark.hpp`).
  - `Runner::run` (`source/harness/runner.cpp`; the declaration in
    `source/harness/detail/internal.hpp`).
  - `appendCounterColumns` (`source/harness/runner.cpp`; declared in
    `source/harness/detail/internal.hpp`).
  - `printResult` (`source/harness/report.cpp`).
  - `add_docs_example`, the `run_docs_` targets, and `run-examples`
    (`example/CMakeLists.txt`).

This spec extends that surface. It replaces no shipped signature.

### PC-1: the thread-target plan and the rule that no fold crosses threads. PASS.

- The target a plan samples on is `sg::counters::Target`
  (`include/speedgun-ng/counters_provider.hpp`), a `TargetKind` of
  `THREAD` or `CPU` plus one cpu index, with `THREAD` as the default and
  `cpu` at `-1` (the `enum class TargetKind` and the `Target` struct in
  that header).
- The API that opens a plan for a target is the public `compile`
  overload that takes a `System`, a `Target`, and the expression cores,
  and returns a `Plan`
  (`include/speedgun-ng/counters_measurement.hpp`). The harness reaches
  the same route through `detail::compileCore(sys, Target {}, cores)` in
  `Runner::run` (`source/harness/runner.cpp`), so every plan the harness
  opens today carries the thread target.
- The rule that no fold crosses threads stands in two places, and the
  time-stamp entry states it as the caller's precondition: the value of
  `machine/tsc` "is ordered only where one thread takes both endpoints
  of the window it measures, and that is a precondition on the caller:
  one thread takes both endpoints", the leaf "claims no order across
  threads and no order across processors", and the harness is named as
  the caller that "takes both endpoints on one thread"
  (`include/speedgun-ng/counters_clock.hpp`). `machine/thread_cpu`
  states the same ownership of its counter: a new thread's first sample
  can fall below an earlier sample taken on another thread, because the
  counter belongs to the thread (same header).
- The plan rule beside it: "Plans and recorders are per-thread objects
  (FR-031)" (`include/speedgun-ng/counters_measurement.hpp`, the `Plan`
  documentation).
- CI proves the rule refuses cross-thread use: the `push-cross-thread`,
  `recorder-cross-thread`, and `scope-cross-thread` fixture modes
  (`test/source/counters_trap_fixture.cpp`,
  `test/source/counters_trap_checked_test.cpp`) run in the
  `consumer-release` job (`.github/workflows/ci.yml`).
- Consequence for this feature: one context runs one plan, and the fold
  of a context's row reads that plan and nothing else (FR-018).

### PC-2: the run loop, calibration, and the pause reservation; the thread count in the flag pass. PASS.

- The run loop is `Runner::run` (`source/harness/runner.cpp`). It
  compiles the plan once per instance, reserves the recorder of each
  attempt through `plan->recorder(...)`, and samples through its
  `sampleRun` lambda, which opens the timed loop, the state, and the
  fold sink of one run.
- Calibration bounds stand in `source/harness/detail/calibration.hpp`:
  `kIterationCap` at 1'000'000'000'000 iterations and `kRunBound` at 96
  runs of one phase.
- The per-leaf pause reservation is `kMaxPauseSamplesPerLeaf` at 32'768
  raw points per leaf column (`source/harness/detail/calibration.hpp`).
  `Runner::run` applies it: the attempt earns pause room of
  `2 * history * iterations` only while `2 * history` and
  `kMaxPauseSamplesPerLeaf / (2 * history)` both leave room, and an
  attempt over the ceiling reserves none and folds (the fold path of H4,
  FR-051 and FR-052 of `specs/019-harness-user-counters-and-statistics`).
- The counter flag pass reads the thread count nowhere today:
  `appendCounterColumns` (`source/harness/runner.cpp`) applies the flags
  in the order rate, thread average, iteration-invariant, average over
  iterations, invert, and its rule comment states "The thread average
  divides by one while threads stand out of scope (FR-004, Q-3), so that
  flag carries no divisor here", where both citations belong to H4. No
  thread-count divisor exists in the source. FR-020 supplies the real
  one.

### PC-3: the constitution carries the gates. PASS.

- Constitution 2.20.0 (the lineage table and the footer of
  `.specify/memory/constitution.md`).
- Counters-only rule: the Additional Constraints entry states that
  "every time and counter value that speedgun code outside the counters
  library measures or reports comes from the `sg::counters` library",
  binds `source/`, `include/`, `example/`, `test/`, and `tools/`, lists
  the forbidden time sources, and names one exception outside this
  feature's reach.
- Time-source gate: Principle VIII lists `test/time_source_gate.sh` as a
  hard gate; the `lint` job runs it and `test/CMakeLists.txt` registers
  it as the `time_source_gate` test.
- Naming: Principle V.1 carries N-1 to N-12, and Principle VIII requires
  zero findings from `readability-identifier-naming`.
- Contracts: Principle II requires the doxygen contract on every
  interface and its runtime enforcement. The pair gate is
  `tools/dbc/dbc_pair_gate.py`, which fills `enforced_kinds` from the
  enforced macro tokens and fails both drift directions:
  documented-not-enforced and enforced-not-documented. The `dbc-gate`
  job runs it (`.github/workflows/ci.yml`), and the H4 receipt records
  `pair-gate: 211 interfaces, 0 gaps`
  (`specs/019-harness-user-counters-and-statistics/tasks.md`).
- Main page: IV.1 obliges every feature that changes user-visible
  behavior to carry its main-page section, its `example/` region, and
  its quick-reference rows, and Principle VIII's page-drift-clean gate
  names `example_drift`, `example_drift_fixtures`, `snippet_compile`,
  `report_shape`, `report_shape_fixtures`, `example_run`, and
  `example_run_fixtures`.
- Coverage: Principle VI gates 100% line, 100% branch, and 100% DBC, the
  `coverage` job runs it, and the audit point holds zero `LCOV_EXCL`
  markers under `source/harness/` (a scan of that tree reports no match).
- Thread sanitizer: the `tsan` job configures `cmake --preset=ci-tsan`,
  builds `build/tsan`, and runs the suite there
  (`.github/workflows/ci.yml`), apart from `ci-sanitize`, because the
  compilers reject the pairing (Principle VIII).
- Commit lint: the `prose-lint` job enforces Principle XI and the Pull
  Request Quality template through `tools/prose/prose_gate.py`.

### PC-4: the docs budget. PASS, and the page has no headroom.

- `README.md` is 570 lines (`wc -l README.md`).
- `PAGE_LINE_CAP = 570`, `SECTION_LINE_CAP = 25`, and
  `SECTION_LINE_CAPS = {"Registration": 38, "Fixtures": 43}` stand in
  `tools/docs/example_drift.py`; the section check reads
  `SECTION_LINE_CAPS.get(title, SECTION_LINE_CAP)`.
- `SECTIONS` lists 18 feature sections, each with one owning example
  file, and each section must hold a marked block within its cap.
- The page therefore sits at 570 of 570. Two new sections and their
  quick-reference rows cannot land without moving the page cap, which is
  this spec's own work (FR-034, Q-5).

## Roadmap position

This is spec H5 of the speedgun harness roadmap. H1 (015) shipped the
single-thread runner with attached counter metrics and repetition
aggregates. H2 (016) added families, fixtures, and callbacks. H3 (017)
added timing modes, pause and resume, and time units. D1 (018) set the
main-page format and its drift gate. H4 (019) added user counters with
flags, custom statistics, aggregate-only modes, and complexity fitting.
This spec gives the harness the threads feature: one owned thread per
execution context, a public `dispatch()`, and the Google Benchmark
thread calls built on top. It builds no graph engine, no topology
placement, no context groups, and no JSON report: those belong to H16,
H10, H11, and H7. The roadmap document and the research notes the
request names sit outside this repository and are absent from this
machine, so the scope boundary here comes from the request itself
(Assumptions).

Every test of this spec runs on the fake provider: the feature needs no
hardware and no PMU.

## Google Benchmark reference read

Google Benchmark is a reference read, nothing more. No vendored copy
enters this feature, and the harness links nothing from it. Three
semantics are taken, each named here by the function of the revision the
plan must pin:

| Semantic taken | Named by |
| --- | --- |
| A benchmark instance runs its body on N threads, one runner per instance, with the thread count and the thread index visible to the body | `ThreadManager` |
| The per-run thread runner starts each thread, holds the start signal outside the timed region, joins the threads, and hands the per-thread results to the aggregation step, which sums each thread's times and iterations into one merged result | `ThreadRunnerBase`; the sum is `RunInThread`, the merged report is `CreateRunReport` |
| The counter flag pass divides `AVG_THREADS` by the real thread count after the per-thread counter values are merged | `internal::Finish` |

This spec records no revision hash: the plan pins the revision read and
cites the file and the function of each semantic, in the form
`specs/019-harness-user-counters-and-statistics/spec.md` shows. The
fixture rule of the cited revision, where `setUp` and `tearDown` run
once per thread group, is a difference this feature states in its own
records (FR-023, Q-4).

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Run work on a context you own (Priority: P1)

A developer wants a thread the library owns and drives: create one
execution context, dispatch work items to it, and read back where each
item ran. They choose the wait mode of an idle context: spin with a
pause hint, yield, or block. They choose a placement slot: no pin, or
one cpu index. When the work is finished they ask the context to stop,
let it drain what is queued, and join it.

**Why this priority**: the context, its policy, and `dispatch()` are the
whole of D-1 and D-3, and every other capability of this feature is
built on them.

**Independent Test**: create a context under each wait mode, dispatch a
scripted series of work items from the calling thread and from a second
thread, and read the context id each item observed, the order the items
ran, and the completion state of each node. Then stop, drain, and join
and check that every queued item ran exactly once. No benchmark runs in
this story.

**Acceptance Scenarios**:

1. **Given** a running context, **When** a caller dispatches work items
   one after another, **Then** each item runs on that context's thread,
   in the order the inbox accepted them, and `currentContext()` names
   that context inside every item.
2. **Given** a context whose wait mode is `BLOCK`, **When** its inbox
   stands empty, **Then** its thread sleeps, and **When** a caller
   dispatches an item, **Then** the item starts without a poll interval.
3. **Given** a context with work queued, **When** the caller calls
   `requestStop()`, `drain()`, and `join()`, **Then** every queued item
   runs exactly once and the join returns.
4. **Given** a context that no caller has joined, **When** its
   destructor runs, **Then** the program aborts with an
   `SG_REQUIRE` violation.
5. **Given** a placement function a test replaced, **When** a context
   starts its thread with one cpu index, **Then** the replacement is
   what ran, once per thread.

### User Story 2 - Run one benchmark on N contexts (Priority: P1)

A developer measures a routine that a real program would run on several
threads at once. They call `threads(4)` on the handle, or state a range
with `threadRange` or `denseThreadRange`, and read one row whose name
ends in `/threads:4`. Inside the body they read `threadIndex()` and
`threads()`. The row's real time is the sum of the contexts' real times,
its CPU time is the sum over the contexts, and its iteration count is
the summed per-context counts. A counter marked `AVG_THREADS` divides by
the real count.

**Why this priority**: this is the threads feature the roadmap names as
the reason the context exists, and the largest part of the user-visible
surface.

**Independent Test**: register a benchmark on a fake provider whose
per-context leaves are scripted, run it with `threads(4)`, and compare
the merged row against the aggregation rule of FR-020, context by
context. A start barrier and a stop barrier stand outside every measured
window, so each scripted window holds only the body's work.

**Acceptance Scenarios**:

1. **Given** a benchmark whose handle calls `threads(4)`, **When** the
   instance runs, **Then** `threadIndex()` reads 0, 1, 2, and 3 across
   the four contexts, `threads()` reads 4 on each, and the row name ends
   in `/threads:4`.
2. **Given** four contexts with scripted per-context leaves, **When**
   the run reports, **Then** real time and CPU time each equal the sum
   over the four, iterations equal the summed per-context counts, and
   each counter flag matches the cited `Finish` result for N = 4.
3. **Given** a benchmark that calls no thread method, **When** it runs,
   **Then** it runs on the calling thread as it does today,
   `currentContext()` reports no context inside the body, and it prints
   the row bytes it prints today.
4. **Given** a user thread runner that receives the four contexts,
   **When** the run starts, **Then** that function dispatches the
   per-thread body and the harness still holds the barriers and the
   samples.
5. **Given** a thread count of zero, **When** the call happens, **Then**
   the program aborts with an `SG_REQUIRE` violation.

### User Story 3 - Run a noise load beside a measurement (Priority: P2)

A developer wants to see what a competing load does to a measurement.
They create one context that runs a noise load and another that runs the
benchmark, and dispatch both. The benchmark's plan reads only its own
context, so its columns stay its own while the load keeps running.

**Why this priority**: it is the example D-3 gives for the public API,
and the reason the API is public at all. It adds no aggregation rule and
no report text, so it follows the two stories above.

**Independent Test**: script the provider, start a load on one context
that never stops on its own, run one benchmark on a second context, and
check that the benchmark completes and that its plan reads only the
leaves of its own context.

**Acceptance Scenarios**:

1. **Given** a load context that runs continuously and a benchmark
   context, **When** the benchmark runs, **Then** it completes and
   reports a row.
2. **Given** the same two contexts, **When** the benchmark's row is
   folded, **Then** every value in it comes from the plan its own
   context opened, and no value comes from the load context.
3. **Given** work dispatched from inside a running item, **When** the
   item returns, **Then** the dispatched item runs on the same context
   after it.

### User Story 4 - Read the contexts in the report (Priority: P2)

A developer reading a report wants to know what a threaded row was run
on. The report names the context count and each context's policy: the
wait mode and the placement, either no pin or one cpu index. When the
setup is oversubscribed, the report says so in one line.

**Why this priority**: the disclosure is what makes a threaded row
interpretable, and a noisy setup is a measurement problem the reader
must see (Principle VII).

**Independent Test**: build the disclosure record sets of four policy
cases directly, as `contracts/report-disclosure.md` states, print each
through the report path, and compare the printed disclosure: two
spinning contexts on one cpu, more spinning contexts than online cpus,
one pinned context per cpu, and a blocking context. The warning appears
in the first two cases and in no others.

**Acceptance Scenarios**:

1. **Given** a threaded result carrying four disclosure records with
   mixed policies, **When** the report prints the row, **Then** the
   count and each context's wait mode and placement are printed with
   it.
2. **Given** a result with two spinning contexts pinned to one cpu,
   **When** the report prints, **Then** it carries one noise warning
   line after that result.
3. **Given** a result with more spinning contexts than the host has
   online cpus, **When** the report prints, **Then** it carries one
   noise warning line after that result.
4. **Given** a result that meets neither condition, **When** the report
   prints, **Then** no noise warning line appears.

### User Story 5 - Read the page, copy the block, run the example (Priority: P3)

A reader opens the main page, finds an "Execution contexts" section and
a "Threads" section, copies each block over the canonical includes, and
builds. The quick reference names each new call, and "Reading the
report" shows a `/threads:N` row.

**Why this priority**: IV.1 makes the page part of the feature, and the
page is where a new reader meets the feature. It follows the code.

**Independent Test**: build and run the two new example programs, run
the drift check over the page, and paste each block into a
translation unit of your own.

**Acceptance Scenarios**:

1. **Given** the two new sections, **When** the drift check runs,
   **Then** each block equals the region of its example file and each
   section holds a marked block within its cap.
2. **Given** the two new example programs, **When** CI runs the docs
   examples, **Then** every registered benchmark of each prints a row.
3. **Given** each new public name, **When** the quick reference is
   read, **Then** it carries a row for that name.

### Edge Cases

- A dispatch that races `requestStop()`: FR-050 is the rule, and the
  node reads `STOPPED` only when its enqueue fell on the stopped side
  of the linearization point, so no queued work is lost silently.
- A node a caller destroys before its completion state reads `DONE` or
  `STOPPED`: the enforce semantic of the build aborts with an
  `SG_REQUIRE` violation, because the caller owns the node until it
  completes; after a reader observes `DONE` or `STOPPED` the caller may
  destroy or reuse the node (FR-049).
- Work dispatched from inside a running item: it runs after that item
  ends, on the same context.
- A `BLOCK` context woken by a dispatch while a stop request stands: the
  linearization of FR-050 decides, and the item either drains or reads
  `STOPPED`. No item runs twice.
- A `join()` call from the context's own thread: an `SG_REQUIRE`
  violation, because a context cannot wait for itself.
- A cpu index the host does not have, or a thread count of zero: an
  `SG_REQUIRE` violation at the call.
- A thread count above the count a host can place: the run proceeds and
  the report prints the noise warning when its condition holds (C-7).
- A context of a threaded run whose body skips with a message: the skip
  reason reaches the merged row, and no partial row is published for the
  other contexts of that instance.
- A benchmark that runs on the default single context: the row keeps the
  bytes it prints today (FR-025, Assumptions).
- A custom thread runner that returns with a body node undispatched or
  dispatched twice: the run aborts with an `SG_REQUIRE` violation at
  that point, because the barrier rounds of FR-019 expect every body of
  the run and read no clock (FR-021).
- A threaded run whose per-context iteration count is zero: calibration
  bounds decide, as they do today, and no division by zero reaches the
  flag pass.
- The main page at 570 of 570 lines while two sections must join: prose
  is trimmed first and no snippet is dropped to fit (FR-034, Q-5).

## Requirements *(mandatory)*

### Execution context and policy (C-1, C-2)

- **FR-001**: An execution context shall own exactly one thread and run
  one event loop on it: when the inbox holds a work node, the loop shall
  take the oldest node, run it, and look again; when the inbox stands
  empty, the loop shall wait by the context's policy (D-1).
- **FR-002**: Every context shall carry a `ContextId`, and
  `currentContext()` shall name the context that runs the calling code.
  Called on a thread the library owns no context for, it shall report no
  context (Assumptions).
- **FR-003**: The inbox shall be an intrusive multi-producer
  single-consumer queue in the style of Vyukov: many threads enqueue at
  once, the context thread alone dequeues, the link lives in the node,
  and enqueue shall allocate nothing.
- **FR-004**: A context policy shall select one wait mode for an idle
  inbox from `SPIN`, which spins with a pause hint, `YIELD`, which yields
  to the scheduler, and `BLOCK`, which waits on the standard atomic wait
  of the inbox flag. No wait mode shall read a clock or take a timeout.
  Every wait mode shall pass the same dispatch test, and `BLOCK` shall
  sleep while idle and wake on a dispatch. A context the harness creates to
  run a benchmark shall use `SPIN`, and a context a caller creates with
  no policy shall default to `BLOCK`.

- **FR-005**: A context policy shall select a placement slot: `NO_PIN`,
  which is the default, or one cpu index. The placement shall apply once,
  at thread start, through one replaceable placement function. A caller
  that replaces that function shall be what runs in its place, and that
  function's signature is the seam H10 replaces with topology placement
  while this feature's API stays as shipped (SC-05). An invalid cpu index
  shall be an `SG_REQUIRE` violation at the call.
- **FR-006**: The lifecycle shall be `requestStop()`, `drain()`, and
  `join()`. The three together shall run every queued node exactly once:
  a node enqueued before `requestStop()` runs during `drain()`, and no
  node runs after `join()` returns. The destructor shall require a joined
  context through `SG_REQUIRE`. The enqueue rule that decides a dispatch
  racing the stop request stands in FR-050.
- **FR-007**: The policy of a context shall be fixed when the context is
  created and shall be readable for the report of FR-025.

### Typed work and dispatch (C-3)

- **FR-008**: A work item shall satisfy the `WorkItem` concept: a
  `noexcept` invocable that takes a `WorkContext&`. A type that does not
  satisfy the concept shall be impossible to dispatch, refused before the
  program builds (D-2).
- **FR-009**: A work node of type `T` shall hold the item, its queue
  link, one erased run path for the item, and a completion state that
  reads `PENDING` before the loop takes the node, `RUNNING` while the
  item runs, `DONE` once the item ran, or `STOPPED` once a refused
  dispatch completed it (FR-050). The completion state is a hook H16
  builds continuations on, so its meaning and readability shall not
  change later (Hooks for H16, FR-049).
- **FR-010**: `dispatch(context, node)` shall link a caller-owned node
  into the inbox of the named context and shall return without waiting
  for the item to run. It shall allocate nothing, and it shall be legal
  from any thread, including from inside work that is already running on
  that same context (Hooks for H16).
- **FR-011**: `WorkContext` shall give the running item the id of the
  context that runs it and a stop token that reads true once the context
  has been asked to stop.
- **FR-012**: The caller shall keep a node it dispatches alive until
  its completion state reads `DONE` or `STOPPED`; the body nodes a
  threaded run constructs are harness-owned and stand outside this rule
  (FR-021). The enforce semantic of the build shall abort with an
  `SG_REQUIRE` violation when a node is destroyed early.
- **FR-013**: Work dispatched to a context shall run on that context's
  thread. Nodes a thread dispatches one after another shall run in that
  order. Nodes two threads dispatch at the same time shall run in the
  order the inbox accepted them.

### Context barrier (C-4)

- **FR-014**: `ContextBarrier` shall join a fixed number of contexts: a
  context that arrives shall wait until every expected context has
  arrived, and one round shall release all of them. A waiting context
  shall wait in the wait mode of its own policy, so a spinning context
  never sleeps inside a barrier; a participant that runs on no context
  shall wait in `BLOCK`. The barrier shall read no clock, so no round
  can time out. A participant count a construction cannot honor, zero
  included, shall be an `SG_REQUIRE` violation at that call. Its
  waiting mechanism shall be a small atomic counter, and `std::barrier`
  shall serve as the fallback for `BLOCK`.
- **FR-015**: A barrier shall be legal to use from inside running work
  and outside it, and a participant that leaves a round early shall be a
  contract violation.

### Google Benchmark threads on contexts (C-5)

- **FR-016**: The handle shall offer `threads(n)`, `threadRange(min, max)`,
  and `denseThreadRange(min, max, step)`, and each shall run every
  instance it selects on `n` contexts. Each reported row of a threaded
  instance shall carry a name ending in `/threads:N`, where `N` is the
  count of contexts that instance ran on.
- **FR-017**: `State::threadIndex()` shall report the index of the
  context that runs the body, in the range 0 to `N - 1`, and
  `State::threads()` shall report `N`.
- **FR-018**: Each context of a threaded run shall open its own
  thread-target plan (PC-1), and every fold of that context's row shall
  read that plan alone. No fold shall cross contexts: no value of one
  context's row shall be read from or written into another context's
  plan, as the time-stamp rule requires of a caller that takes both
  endpoints on one thread.
- **FR-019**: A start barrier and a stop barrier shall stand outside
  every measured window of a threaded run: no barrier wait shall fall
  inside a window the harness folds (SC-08).
- **FR-020**: The merged row of a threaded run shall combine the
  per-context results by summing, as the cited `RunInThread` and
  `CreateRunReport` do: the real column carries the summed manual time
  under manual timing and the summed real time otherwise, the CPU column
  carries the summed CPU time, the iterations are the summed per-context
  counts, and the user counters of the contexts are summed before the
  flag pass. The flag pass shall follow the cited `Finish`, the counters
  path the cited code enters through `CreateRunReport`, and run with the
  real `N`: `CounterFlag::AVG_THREADS` divides by `N`, and the `RATE`
  flag divides by the summed decision seconds of the run's timing mode
  divided by `N` — that sum is the CPU-time sum by default, the manual
  sum under manual timing, and the real-time sum under real-time timing,
  the cited thread-seconds divisor (PC-2, research D-06).
- **FR-021**: A custom thread runner shall be a function the user
  supplies that receives the `N` contexts of the run and dispatches the
  per-thread body itself. The harness shall construct the body nodes,
  keep them alive across the runner call, and check the dispatch ledger
  when the runner returns: the runner shall have dispatched every body
  exactly once, and a body left undispatched or dispatched twice shall
  be an `SG_REQUIRE` violation at that point. The harness shall keep
  the barriers of FR-019 and the samples of FR-020 around a runner it
  did not write.
- **FR-022**: The default shall be one context per instance. A benchmark
  that calls no thread method shall keep today's single-thread behavior
  and today's row bytes: the run keeps the calling thread, creates no
  execution context, and `currentContext()` reports no context inside
  the body (FR-002).
- **FR-023**: `setUp` and `tearDown` of a threaded run shall run once per
  run, on context 0, outside both barriers. Each difference from the
  fixture rule of the cited revision shall be recorded in this feature's
  records (FR-046).
- **FR-024**: A thread count a call cannot honor, zero included, shall be
  an `SG_REQUIRE` violation at that call.

### Policy disclosure in the report (C-7)

- **FR-025**: For every benchmark that runs on more than one context,
  equivalently for every result whose policy-disclosure record set is
  non-empty, the console report shall print the context count and each
  context's policy: the wait mode and the placement, which reads
  `NO_PIN`, one cpu index, or `UNAPPLIED` where the placement function
  landed no slot. A threaded run fills that set with one
  record per context it ran (FR-021), and a benchmark that runs on one
  context shall keep the bytes it prints today (Assumptions).
- **FR-026**: When two spinning contexts of one result name the same cpu
  index, or one result has more spinning contexts than the host has
  online cpus, the report shall print one noise warning line after that
  result, one line for each such result. The line shall appear when
  that condition holds and shall not appear when it does not (SC-12).
- **FR-027**: The disclosure and the warning shall read no clock, and
  neither shall add a sampling action to any run.
- **FR-028**: A test shall cover each case of FR-026: a shared-cpu spin
  pair, a spinning count above the online cpu count, a pinned setup with
  one context per cpu, and a blocking setup.

### Main-page documentation and runnable examples (C-6)

- **FR-029**: The main page shall gain one "Execution contexts" section
  and one "Threads" section, each in the D1 shape of one purpose
  sentence, one marked snippet, and one result sentence. The first shall
  teach a context, a policy, and the dispatch of a noise load. The second
  shall teach `threads(4)` and `threadIndex()`.
- **FR-030**: Each new section's snippet shall be a marked region of its
  own runnable file under `example/docs/`, built by a `run_docs_` target
  and run through an `example_run` entry, in the shape H4 set for every
  main-page section (PC-3).
- **FR-031**: The quick reference shall gain a row for each new public
  name this feature ships, in the table's own columns, and the option
  table shall gain a row for a command-line option only when that option
  lands (FR-044, Q-6).
- **FR-032**: The "Reading the report" section shall show a `/threads:N`
  row, and the disclosure of FR-025 shall be legible in it.
- **FR-033**: `docs/pages/harness.md` shall gain the detail the page
  keeps its distance from: the aggregation table of FR-020 and the
  lifecycle rules of FR-006, in the reader voice the rest of that page
  keeps and with no spec numbers.
- **FR-034**: The drift gate's `SECTIONS` list shall gain the two new
  section titles. `SECTION_LINE_CAP` shall stay 25. The page cap shall
  move by the measured size of the two new sections and their
  quick-reference rows, recorded in this spec's own budget note, and no
  file under `specs/019-harness-user-counters-and-statistics/` shall be
  edited. Prose shall be trimmed first, and no snippet shall be dropped
  to fit. The decision is the rule and its
  measurement point; the number itself is measured when the two
  sections land.
- **FR-035**: The `example_run` gate shall cover both new examples: an
  undeclared skip, a `FAILED` row, and a registered benchmark whose row
  never prints shall each fail the gate.

### Surface, version, and discipline

- **FR-036**: Every new identifier shall follow Principle V.1, N-1 to
  N-12, and every new macro shall carry the `SG_` prefix. The name check
  shall report zero findings.
- **FR-037**: Every new public interface shall document `\pre`, `\post`,
  and `\invariant`, and the source shall enforce each of those contracts.
  The dbc pair gate shall report zero drifted interfaces at the feature
  head (Principle II, PC-3).
- **FR-038**: No `LCOV_EXCL` marker shall enter `source/harness/`, and
  coverage shall stay 100% line and 100% branch (Principle VI, PC-3).
- **FR-039**: Every time and counter value of this feature shall come
  from `sg::counters`, by the counters-only clause of the Additional
  Constraints, and the time-source gate shall report no hit at the
  feature head (PC-3).
- **FR-040**: Neither a timed loop nor `dispatch()` shall allocate. A test
  with an allocation hook shall prove the dispatch path, and the
  loop-shape gate shall stay green.
- **FR-041**: Every new path shall run under the thread sanitizer: the
  `ci-tsan` preset and the `tsan` job shall report no race, and the
  `ci-sanitize` preset shall report no error (Principle VIII, PC-3).
- **FR-042**: The feature shall be built test-first on the fake provider,
  with each test observed failing before the code that turns it green,
  and `plan.md` shall record TDD mode (Principle III).
- **FR-043**: The change shall name its library target, its project
  version, and its harness `SOVERSION` in the plan's version table, on the
  hand-kept version rule of the constitution, at the decided values: the
  context core lives in the harness archive with no new target, the
  project version moves from 0.9.0 to 0.10.0, and the harness
  `SOVERSION` moves from 5 to 6. A separate `speedgun-ng::exec` target
  can come with H16.
- **FR-044**: This feature shall add no command-line option; a
  `--threads` override waits for H9.
  Any option that does land shall fail closed on an invalid value: one
  line and exit status 2.
- **FR-045**: The four hooks of Hooks for H16 shall ship as part of the
  public surface, shaped for this feature alone, and nothing in this
  feature shall give them a second meaning: the node completion state,
  `dispatch()` from inside running work, `ContextId`, and the placement
  slot. H16 shall add continuations and dependencies on top of them with
  these signatures unchanged.
- **FR-046**: Every difference this feature keeps from the cited Google
  Benchmark revision shall be recorded in the feature's own records
  beside the semantic it belongs to, including the fixture rule of
  FR-023, the barrier of FR-014, and the per-thread iteration count of
  FR-020: a threaded run of this feature gives every context the settled
  count of the calibration, so the summed count of the merged row is
  `N` times that count, where the cited revision divides the settled
  count among the threads it starts. The plan pins the revision read
  (Google Benchmark reference read).
- **FR-047**: The examples and the documentation shall reach the new
  calls through the public `speedgun-ng` headers alone.
- **FR-048**: All prose of this feature, in the spec, the plan, the tasks,
  the code comments, the page, and the commits, shall obey Principle XI,
  XI.7 included, and the prose and commit gates shall report zero
  findings.
- **FR-049**: The thread that completes a node shall write its
  completion state with release order, and a reader that observes the
  state shall read it with acquire order, so everything the work did
  happens-before that read. The loop writes `DONE` as its last access to
  the node: it shall read the next queue link and perform any notify
  before that write, and after it the loop shall touch only its own
  data, never the node; the dispatcher writes `STOPPED` for a refused
  node (FR-050). After a reader observes `DONE` or `STOPPED`, the caller
  may destroy or reuse the node (FR-009, FR-012, FR-045).
- **FR-050**: The enqueue in `dispatch()` and the stop request of FR-006
  shall linearize against each other, and the enqueue is the single
  linearization point: a `dispatch()` that observes the not-stopped
  state shall append the node and own its delivery, and a `dispatch()`
  that observes the stopped state shall refuse the node and itself
  complete it with `STOPPED`. It is the dispatcher that sets `STOPPED`
  for a refused node, because the node never reaches the loop and the
  loop shall never touch a node it never accepted. Nodes accepted before
  `requestStop()` run during `drain()`; refused nodes read `STOPPED` and
  never run. `drain()` shall block the caller until the inbox is empty
  and the loop is idle; a `drain()` without a prior `requestStop()`
  shall not stop the loop but block the same way, a flush; `drain()`
  from the context's own thread is a contract violation that a debug
  build catches as an `SG_REQUIRE` precondition (FR-006, FR-010).

## Key Entities *(include if feature involves data)*

- **Execution Context**: one thread the library owns, its event loop, its
  inbox, and its policy. Identified by a `ContextId`. It hosts work items
  of any type that satisfies the work-item concept, a benchmark test case
  among them.
- **Context Policy**: the two choices a caller makes at creation: the
  wait mode of an idle inbox (`SPIN`, `YIELD`, `BLOCK`) and the placement
  slot (`NO_PIN` or one cpu index). The report discloses both.
- **Work Item**: one unit of user work, a `noexcept` invocable that takes
  a `WorkContext&`. Any type that satisfies the concept is dispatchable.
- **Work Node**: the caller-owned carrier of one work item. It holds the
  item, the inbox link, the erased run path, and the completion state:
  `PENDING`, `RUNNING`, `DONE`, or `STOPPED`.
- **Work Context**: what a running item is handed: the id of the context
  that runs it and a stop token.
- **Context Barrier**: a rendezvous for a fixed number of contexts, with
  each waiting context waiting in its own wait mode and no clock
  involved. A threaded run uses two of them, one for its start round
  and one for its stop round.
- **Threaded Instance**: one benchmark instance paired with one context
  count. Its reported name carries `/threads:N`.
- **Merged Row**: the one row of a threaded instance: the summed real
  time (the summed manual time under manual timing), the summed CPU
  time, the summed per-context iteration count, and the summed counters
  after the flag pass with the real `N`.
- **Policy Disclosure**: the report fields that name the context count
  and each context's wait mode and placement.
- **Noise Warning**: the one report line that states that a spinning
  setup cannot get the quiet Principle VII asks for.
- Existing entities this feature extends, unchanged in identity: `State`,
  `BenchmarkHandle`, `ResultRow`, `BenchmarkResult`, and the thread-target
  `Target`, `Plan`, and `Recorder` of the counters library.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-01**: Work dispatched to a context runs on that context's thread,
  in the order FR-013 states, and `currentContext()` names that context
  inside the item.
- **SC-02**: Each of the three wait modes passes the same dispatch test.
  The `BLOCK` mode sleeps while idle and wakes on a dispatch.
- **SC-03**: `requestStop()`, `drain()`, and `join()` run every queued
  node exactly once, and the destructor requirement of FR-006 aborts on
  an unjoined context.
- **SC-04**: `dispatch()` performs zero allocations, measured by a test
  with an allocation hook.
- **SC-05**: The placement slot applies one cpu index once at thread
  start through the `PlacementFn` signature, and the function installed
  through `setPlacementFunction` before any context starts is what runs
  in its place (contracts/execution-context.md).
- **SC-06**: `threads(4)` gives `threadIndex()` values 0 to 3,
  `threads()` 4, and a row name ending in `/threads:4`.
- **SC-07**: With scripted per-context leaves, the merged row matches
  FR-020, and every counter flag matches the cited `Finish` result for
  the real `N`.
- **SC-08**: No barrier wait falls inside a measured window.
- **SC-09**: A noise load on one context runs while a benchmark runs on
  another, and the benchmark's plan reads only its own context.
- **SC-10**: The thread sanitizer reports zero findings on every new
  path, and every Principle VIII gate passes at the feature head.
- **SC-11**: Both new README sections and their examples pass every docs
  gate, within the budget recorded under FR-034.
- **SC-12**: The report shows each context's wait mode and placement, and
  the noise warning appears exactly when its condition holds.

## Constraints

- **Time and counters.** Every time and counter value comes from
  `sg::counters`, and the time-source gate stays clean (the
  counters-only clause of the Additional Constraints of the
  constitution).
- **Pause hint: the P2 exception of this feature.** The idle-spin pause
  hint of FR-004 is one documented `asm volatile("pause" :::
  "memory")` site, registered here as this feature's only P2 exception
  under Principle I, on the shipped `barrier.hpp` precedent. FR-004
  names the pause hint, a bare spin starves the sibling hyperthread the
  noise scenarios of SC-09 lean on, and the counters-only clause bans
  the intrinsic headers. plan.md's Complexity Tracking carries the
  site-level justification.
- **No fold crosses contexts.** One context folds its own plan
  (PC-1). A threaded run publishes one merged row built from
  per-context values, and no fold reads another context's plan.
- **No allocation on the hot paths.** Nothing allocates inside a timed
  loop or inside `dispatch()`.
- **Contracts at the boundary.** An invalid thread count or an invalid
  cpu index on an API call is an `SG_REQUIRE` violation. Any new
  command-line option fails closed: one line and exit status 2.
- **Law of the repository.** V.1 naming, N-1 to N-12. Principle II
  contracts with zero drifted interfaces on the dbc pair gate. No
  `LCOV_EXCL` under `source/harness/`; 100% line and 100% branch
  coverage.
- **Test discipline.** Test-first on the fake provider. The
  ThreadSanitizer and the sanitizer presets pass.
- **Prose.** Principle XI and the commit lint bind every word and every
  commit of the feature.

## Hooks for H16

This feature builds no graph engine. Four parts of its surface are the
ground a later execution-graph spec builds on, and each is a signature
H16 cannot change:

- the completion state of a work node, which is what a continuation
  reads before it runs;
- `dispatch()` from inside running work, which is what a dependency
  edge is made of;
- `ContextId`, which is what names a node's home in a graph;
- the placement slot of the policy, which is what a topology-aware
  scheduler of H10 fills.

Nothing else in this feature is a hook: no extension point, no
customization point, and no reserved parameter serves a use this feature
lacks (Principle X.2).

## Scope

In: FR-001 to FR-050: the context, its policy, typed work and dispatch,
the context barrier, the Google Benchmark thread calls on contexts, the
report disclosure, the two main-page sections and their examples, a test
for each capability, ThreadSanitizer coverage of every new path, and the
version effect.

Out, each owned by a later roadmap spec:

- topology placement and presets (H10);
- context groups, presets, and YAML (H11);
- execution graphs, continuations, and dependencies (H16);
- work stealing;
- senders and receivers;
- JSON reports (H7).

## Assumptions

- The request of 2026-10-15 is the owner's pasted text and is the sole
  authority for scope. The roadmap file and the research notes it names
  sit outside this repository and are absent from this machine, so no
  claim below rests on them.
- A benchmark that runs on one context keeps the bytes it prints today,
  because FR-022 requires today's behavior for that case, the shape gate
  pins the quick-start row, and a count of one and a default policy state
  what the reader already knows. FR-025 states the rule. The run keeps
  the calling thread, creates no context, and `currentContext()` reports
  no context inside the body (FR-022).
- `currentContext()` reports no context on a thread the library owns no
  context for; the request states the positive case only (FR-002).
- A context is created with its policy and keeps it for its life; a
  caller that wants a different policy creates another context (FR-007).
- The context count of a threaded run is fixed for that run; a change of
  count is another instance, as a thread range already produces one
  instance per count (FR-016).
- Every context of a threaded run runs the same body with the same
  arguments and the same calibrated iteration count, which is what makes
  the iteration rule of FR-020 hold.
- The online cpu count is a host fact the report reads once for the
  warning of FR-026; it is not a clock, and the disclosure reads no clock
  (FR-027).
- A test replaces the placement function and reads the call the
  replacement recorded, so the pinning tests need no privileges
  (FR-005).
- The page sits at 570 of 570 lines today (PC-4), so two new sections
  with their quick-reference rows cannot fit: the cap moves by the
  measured amount (FR-034, Q-5).
- All tests run on the fake provider, as H4's do: no hardware and no PMU.

## Budget note

The measured page-cap move of FR-034 and Q-5: `README.md` grew by 69 lines
(`git diff --numstat` gives 69 insertions, 0 deletions), in three hunks — the
two new sections at 41 lines, their quick-reference rows at 27, and the one
disclosure result sentence of "Reading the report" at 1. `PAGE_LINE_CAP` in
`tools/docs/example_drift.py` therefore moves 570 to 639, the smallest fitting
amount: the page stands at 639 lines of a 639 cap, no snippet was dropped to
reach it, and `python3 tools/docs/example_drift.py .` reports no finding. T030
of tasks.md carries the same measurement in the gate's own comment.

## Dependencies

- The counters library: the thread-target `Target`, the per-thread `Plan`
  and recorder, the fold with its disclosure, and `FakeProvider` with its
  `addCounter` and `setPoints` calls
  (`include/speedgun-ng/counters_provider.hpp`,
  `include/speedgun-ng/counters_measurement.hpp`,
  `include/speedgun-ng/counters_clock.hpp`,
  `include/speedgun-ng/counters_fake.hpp`).
- H4's counter flag pass, whose thread divisor this feature makes real
  (`appendCounterColumns` in `source/harness/runner.cpp`, PC-2).
- H1's run loop and calibration, which this feature runs once per context
  (`Runner::run` in `source/harness/runner.cpp`,
  `source/harness/detail/calibration.hpp`).
- H2's instance naming and family expansion, which the `/threads:N` name
  is built onto.
- H4's console report, which carries the disclosure
  (`printResult` in `source/harness/report.cpp`).
- D1's main-page structure and drift gate, including the section list and
  the line caps (`tools/docs/example_drift.py`, `README.md`), and the
  example run machinery (`add_docs_example` and `run-examples` in
  `example/CMakeLists.txt`).
- The constitution gates of PC-3: the counters-only rule and its scan, the
  naming rules, Principle II with the pair gate, IV.1, the coverage gates,
  the `ci-tsan` preset and `tsan` job, and the prose and commit lint.
- Google Benchmark at the revision the plan pins, as a reference read.
  Nothing links it.

## Citations

Symbol and section anchors, each read at the audit point `64f2ab5`.

| Artifact | Location |
| --- | --- |
| `TargetKind`, `Target` | `include/speedgun-ng/counters_provider.hpp` |
| `Plan` and its per-thread rule | `include/speedgun-ng/counters_measurement.hpp` |
| `compile` for a target | `include/speedgun-ng/counters_measurement.hpp` |
| `machine/tsc` and `machine/thread_cpu` ordering rules | `include/speedgun-ng/counters_clock.hpp` |
| Cross-thread traps and the job that runs them | `test/source/counters_trap_fixture.cpp`, `test/source/counters_trap_checked_test.cpp`, the `consumer-release` job of `.github/workflows/ci.yml` |
| `State`, `BenchmarkHandle`, `ResultRow`, `BenchmarkResult`, `CounterFlag` with `AVG_THREADS`, `speedgunMain` | `include/speedgun-ng/benchmark.hpp` |
| Plan compile and the per-attempt recorder | `Runner::run`, `source/harness/runner.cpp` |
| Pause room rule | the `kMaxPauseSamplesPerLeaf` guards in `Runner::run`, `source/harness/runner.cpp` |
| `kIterationCap`, `kRunBound`, `kMaxPauseSamplesPerLeaf` | `source/harness/detail/calibration.hpp` |
| Counter flag pass and its thread divisor comment | `appendCounterColumns`, `source/harness/runner.cpp`, declared in `source/harness/detail/internal.hpp` |
| Console row printing | `printResult`, `source/harness/report.cpp` |
| Example run machinery | `add_docs_example`, the `run_docs_` targets, and `run-examples` in `example/CMakeLists.txt` |
| Drift gate caps and section list | `SECTIONS`, `PAGE_LINE_CAP`, `SECTION_LINE_CAP`, `SECTION_LINE_CAPS` and the section check of `tools/docs/example_drift.py` |
| Main page structure | the "How you do it", "Quick reference", and "Planned work" sections of `README.md` |
| Harness guide | `docs/pages/harness.md` |
| Gate scripts | `test/time_source_gate.sh`, `test/loop_shape.sh`, `test/barrier_shape.sh` |
| Gate registration and CI jobs | `test/CMakeLists.txt`; the `lint`, `coverage`, `sanitize`, `tsan`, `dbc-gate`, and `prose-lint` jobs of `.github/workflows/ci.yml` |
| DBC pair gate | `tools/dbc/dbc_pair_gate.py` |
| Prose and commit gate | `tools/prose/prose_gate.py` |
| Version, `SOVERSION`, compatibility | the `project()` version and the counters and harness target properties in `CMakeLists.txt`; the `COMPATIBILITY` clause in `cmake/install-rules.cmake` |
| Constitution IV.1, VIII gates, II, V.1, counters-only, lineage, footer | IV.1, Principle VIII, Principle II, Principle V.1, the Additional Constraints, the lineage table, and the footer of `.specify/memory/constitution.md` |
| H4 artifacts and receipts | `specs/019-harness-user-counters-and-statistics/spec.md` and `specs/019-harness-user-counters-and-statistics/tasks.md` |

## Open questions

The request carried six open items. Charles approved each recorded
recommendation on 2026-10-15, and `/speckit-clarify` recorded every
answer as written. No open item remains; each point is decided where it
bites, at the FR carrying the matching record.

- **Q-1 Library target and version**: Clarified 2026-10-15: the context
  core lives in the harness archive, with no new target; version 0.10.0
  and the harness `SOVERSION` from 5 to 6. A separate
  `speedgun-ng::exec` target can come with H16, where a graph engine may
  need it. Recorded at FR-043.
- **Q-2 Default wait mode**: Clarified 2026-10-15: `SPIN` for a context
  that runs a benchmark, and `BLOCK` for a context a caller creates with
  no policy. Recorded at FR-004.
- **Q-3 Barrier mechanism**: Clarified 2026-10-15: a small atomic
  counter that waits in the context's own wait mode, so a spinning
  context never sleeps inside a barrier; `std::barrier` is the fallback
  for `BLOCK`. Recorded at FR-014.
- **Q-4 Fixtures with threads**: Clarified 2026-10-15: `setUp` and
  `tearDown` run once per run on context 0, outside the barriers, with
  each difference from the cited revision recorded. Recorded at FR-023.
- **Q-5 README budget**: Clarified 2026-10-15: raise `PAGE_LINE_CAP` by
  the measured size of the two new sections and their rows, through
  FR-034 and this spec's budget note; keep `SECTION_LINE_CAP` at 25;
  trim prose first; edit no earlier spec. The number is measured when
  the two sections land. The page sits at 570 of 570 today (PC-4).
  Recorded at FR-034.
- **Q-6 Command line**: Clarified 2026-10-15: no new option in this
  feature; a `--threads` override waits for H9. Recorded at FR-044.

## Constitution Checklist

- Main-page impact check (IV.1): YES. The feature changes user-visible
  behavior, so FR-029 to FR-035 carry the two new main-page sections,
  their `example/docs/` regions, their quick-reference rows, and the
  `docs/pages/harness.md` detail in the same change, within the budget
  FR-034 records.
