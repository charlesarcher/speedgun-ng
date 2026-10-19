# Tasks: Execution Contexts Core

**Input**: Design documents from `specs/020-execution-contexts-core/`

**Prerequisites**: plan.md, spec.md, research.md (D-01..D-10),
data-model.md (E-01..E-09), contracts/ (four files), quickstart.md

**Tests**: Included. FR-042 fixes TDD mode on the fake provider: each
capability's test task is written first and its failing run is stated
in the task, so no test passes before its implementation exists
(Principle III). No test needs a PMU, a sleep for measurement, or a
wall-clock bound for a number.

**Organization**: Tasks are grouped by user story. Phase 1 carries the
contract and data-model naming the plan needs as a seam (the
`threadRunner` seam, the `RegistryEntry` fields, the disclosure
record). The context core, the barrier, the threaded runner and merge,
the disclosure, the docs and examples, then the gates, the measured
page-cap move, and the version-table entry follow in that order. Every
task carries exact file paths; bounds from data-model.md, contracts/,
and research.md are quoted so no bound is left to implementation-time
discretion.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: User story ownership (US1..US5 from spec.md)
- Setup, barrier, and Polish tasks carry no story label

## Phase 1: Setup (Contract and Data-Model Naming)

**Purpose**: The named seams every story compiles against (FR-045,
FR-046). Declarations, doxygen contracts, and default member values
only; behavior arrives under TDD in the story phases.

**CRITICAL**: No story test compiles until this phase is complete.

- [X] T001 [P] Create `include/speedgun-ng/execution_context.hpp` with the full named surface of `contracts/execution-context.md`, namespace `sg`: `ContextId`; `currentContext()` returning `std::optional<ContextId>` (FR-002); `enum class WaitMode { SPIN, YIELD, BLOCK }` (FR-004); `PlacementSlot` with `noPin()` and `cpu(index)`, the default `NO_PIN` (FR-005); `ContextPolicy { WaitMode waitMode = WaitMode::BLOCK; PlacementSlot placement = PlacementSlot::noPin(); }` so a caller-created context with no policy defaults to `BLOCK` while a context the harness creates to run a benchmark uses `SPIN` (FR-004, Q-2); `class ExecutionContext` with default and policy constructors, `id()`, `policy()`, `requestStop()`, `drain()`, `join()` (FR-001, FR-006, FR-007); `StopToken` and `WorkContext`, the running item's view of the stop state and the context id (FR-011); `enum class WorkNodeState { PENDING, RUNNING, DONE, STOPPED }` (FR-009, FR-049); `template <WorkItem Item> class WorkNode` with the `noexcept` `state()` accessor read with acquire order (FR-009); `concept WorkItem` as a `noexcept` invocable taking `WorkContext&` (FR-008); `PlacementResult`, the `PlacementFn` signature, and `setPlacementFunction` over the one harness-level slot (FR-005, SC-05); `template <WorkItem Item> auto dispatch(ExecutionContext&, WorkNode<Item>&) -> void` (FR-010); `class ContextBarrier` with `explicit ContextBarrier(int participants)` and `arriveAndWait()` (FR-014). Every interface documents `\pre`, `\post`, and `\invariant` (FR-037, Principle II), and every name follows N-1, N-2, N-4, N-12 with no new macro (FR-036)

- [X] T002 [P] Name the harness-side data model without behavior: in `include/speedgun-ng/benchmark.hpp` declare `BenchmarkHandle::threads(int)`, `threadRange(minCount, maxCount)`, `denseThreadRange(minCount, maxCount, step)`, and `threadRunner(ThreadRunnerFn)` per `contracts/threaded-runs.md` behind the standing setter precondition (FR-016, FR-021); `State::threadIndex()` and `State::threads()` over two fields defaulting to `0` and `1` (FR-017, E-05); `BenchmarkResult::contexts`, the vector of `{waitMode, placement}` disclosure records, empty for a single-context run (FR-025, E-07). In `source/harness/detail/internal.hpp` give `RegistryEntry` `std::vector<int> threadCounts` defaulting to `{1}` and the optional `ThreadRunnerFn` runner (D-09, E-05). No shipped signature changes type or shape (PC-0)

- [X] T003 Create `source/harness/execution_context.cpp` (the `speedgun-ng_harness` target collects it; no new target, FR-043) with the named internal seams: the pause-hint spin helper as one documented `asm volatile("pause" ::: "memory")` site in the shape of the shipped `barrier.hpp`, naming no intrinsic header, since `<immintrin.h>` and siblings stay out of the tree (D-01, FR-039, plan Complexity Tracking); the harness-level placement slot with its `setPlacementFunction` installer and the default `PlacementFn`, the Linux one-cpu pin through `pthread_setaffinity_np` on the started thread returning the printable `PlacementResult`, the signature H10 later replaces (D-04, FR-005, SC-05); the thread-current `ContextId` install-and-clear helper behind `currentContext()` (FR-002)

**Checkpoint**: `cmake --preset=ci-rocky && cmake --build build` compiles
the named surface and the story tests compile against it. The
clang-tidy name check needs the analyzer-bearing `ci-ubuntu` preset and
runs at the feature head under T041.

## Phase 2: User Story 1 - Run work on a context you own (Priority: P1) - MVP

**Goal**: A caller creates one context, dispatches typed work, reads
where each item ran, chooses the wait mode and placement slot, and
stops, drains, and joins the context (spec US1).

**Independent Test**: "create a context under each wait mode, dispatch
a scripted series of work items from the calling thread and from a
second thread, and read the context id each item observed, the order
the items ran, and the completion state of each node. Then stop,
drain, and join and check that every queued item ran exactly once. No
benchmark runs in this story." (spec.md US1)

### Tests for User Story 1 (write first, observe failure)

- [X] T004 [P] [US1] Create `test/source/harness_execution_context_test.cpp` and register `harness_execution_context_test` in `test/CMakeLists.txt` (target name equal to CTest name, `cxx_std_23`); scenarios: work runs on the context's thread and `currentContext()` names it inside the item, and reports no context on a foreign thread (SC-01, FR-001, FR-002); the same dispatch test passes under `SPIN`, `YIELD`, and `BLOCK`, with the `BLOCK` thread idle-sleeping and waking on a dispatch with no poll interval (SC-02, FR-004); the lifecycle trio runs every queued node exactly once and the re-exec shows the destructor aborting with an `SG_REQUIRE` violation on an unjoined context (SC-03, FR-006); a node dispatched after `requestStop()` reads `STOPPED` and never runs (FR-050; its scenario stands in T007), and no item runs twice when a `BLOCK` wake races the stop (Edge Cases); single-dispatcher FIFO rows and, for two dispatchers, set membership plus per-dispatcher order (FR-013); work dispatched from inside a running item runs after that item ends, on the same context, and the running item receives the context id and a `StopToken` that reads true after `requestStop()` (FR-010, FR-011); the allocation-hook test counts zero allocations across the dispatch path (SC-04, FR-040); the compile-fixture pattern of `test/source/dbc_standalone.cpp` refuses an unapproved item type before the program builds (FR-008); a replaced placement function records one call per thread at thread start, and an invalid cpu index aborts its re-exec at the call (SC-05, FR-005); destroying a live node early aborts under the enforce semantic (FR-012); `join()` from the context's own thread aborts (Edge Cases). Observe red: the registered test fails against the named-only surface before T010 and T011 exist (FR-042)

- [X] T005 [US1] Append the completion-order scenario to `test/source/harness_execution_context_test.cpp`: the item writes a value, the caller polls `state()` until it reads `DONE` with acquire order, and destroys the node immediately after that read; under the `ci-tsan` preset run (T038) the destroy-after-`DONE` sequence reports no race, which holds the loop to FR-049: the release store is the loop's last access to the node and the next-link read and notify precede it (FR-009, FR-012, FR-049). Observe red against the named-only surface before T010 and T011 exist (FR-042)

- [X] T006 [US1] Append the accepted-then-drained scenario to `test/source/harness_execution_context_test.cpp`: with nodes queued, `requestStop()`, `drain()`, and `join()` run every node accepted before the stop request exactly once during `drain()`, and each completion state reads `DONE` (FR-006, FR-050; SC-03). Observe red before T010 and T011 exist (FR-042)

- [X] T007 [US1] Append the refused-after-stop scenario to `test/source/harness_execution_context_test.cpp`: after `requestStop()` stands, a `dispatch()` observes the stopped state, refuses its node, and completes it itself: `state()` reads `STOPPED`, the item never runs, and the loop never touches that node, the single-writer-per-node rule of FR-050 (FR-050, E-03). Observe red before T010 and T011 exist (FR-042)

- [X] T008 [US1] Append the own-thread drain scenario to `test/source/harness_execution_context_test.cpp`: a work item that calls `drain()` on the context running it trips the debug-precondition `SG_REQUIRE` violation, caught by the re-exec pattern in a debug build (FR-010, FR-050). Observe red before T010 and T011 exist (FR-042)

- [X] T009 [US1] Append the drain-without-stop scenario to `test/source/harness_execution_context_test.cpp`: with no stop request, `drain()` blocks the caller until the inbox is empty and the loop is idle, the loop keeps running, a later dispatch still runs, and the lifecycle trio joins cleanly afterwards (FR-006, FR-050). Observe red before T010 and T011 exist (FR-042)

### Implementation for User Story 1

- [X] T010 [US1] In `source/harness/execution_context.cpp`, implement the core: the event loop on the one owned thread takes the oldest node when the inbox holds one, runs it, and looks again, and waits by policy when the inbox stands empty (FR-001); the inbox is the Vyukov link-swap intrusive MPSC queue with atomic `tail`, private drain `head`, the link living in the node, and enqueue "allocates nothing" (FR-003); the three wait modes: `SPIN` on the inbox flag through the pause helper, `YIELD` through `std::this_thread::yield()`, `BLOCK` through the standard atomic wait with `dispatch()` as the `notify_one()` waker, no mode reading a clock or taking a timeout (FR-004); `dispatch(context, node)` performs the two-atomic link-swap enqueue, returns without waiting, and is legal from any thread including work already running on that context (FR-010, SC-04); the loop installs and clears the thread-current `ContextId` around the node run (FR-002), marks the node `RUNNING` when it takes it, and writes the completion state `DONE` with release order as its last access to the node after the erased call returns, reading the next queue link and performing any notify before that store (FR-009, FR-049)

- [X] T011 [US1] In `include/speedgun-ng/execution_context.hpp` and `source/harness/execution_context.cpp`, complete the caller-facing rules: `WorkNode<Item>` stamps its one erased run path at construction from the concept-checked `Item`, so an unapproved type is a compile error at the `WorkNode<Item>` or `dispatch()` line (FR-008, D-02); `dispatch()` that observes the stopped state refuses its node and itself completes it `STOPPED`, the loop never touching a node it never accepted (E-03, FR-050); destroying a live node trips `SG_REQUIRE` under the enforce semantic (FR-012); `WorkContext` hands the running item the context id and the `StopToken` (FR-011); the constructor applies the placement slot once at thread start through the installed `PlacementFn` (D-04) and an invalid cpu index is an `SG_REQUIRE` violation at the call (FR-005, SC-05); the policy is fixed at creation and readable through `policy()` (FR-007); the lifecycle runs `requestStop()`, `drain()`, `join()` so "a node enqueued before `requestStop()` runs during `drain()`, and no node runs after `join()` returns", the destructor requires a joined context through `SG_REQUIRE`, `join()` from the context's own thread violates the contract, `drain()` blocks the caller until the inbox is empty and the loop is idle so a `drain()` without a prior `requestStop()` flushes while the loop keeps running, and `drain()` from the context's own thread trips the debug precondition (FR-006, FR-050, SC-03)

**Checkpoint**: `ctest --test-dir build -R harness_execution_context_test` passes; the context, policy, typed work, and dispatch MVP stands.

## Phase 3: Context Barrier (C-4, prerequisite for threaded runs)

**Purpose**: The rendezvous the threaded run's start and stop rounds
need (FR-014, FR-019). No story label; US2 consumes it directly.

- [X] T012 [P] Create `test/source/harness_context_barrier_test.cpp` and register `harness_context_barrier_test` in `test/CMakeLists.txt`; scenarios: one round releases every participant at once (FR-014); a spinning participant waits in its own wait mode and never sleeps inside the barrier, proven by the mode's own counter (D-03, Q-3, the SC-08 mechanism); a barrier is legal from inside running work and outside it, where `currentContext()` reports no context and the wait stands in `BLOCK` (FR-014, FR-015); a participant that leaves a round early aborts its re-exec (FR-015); the barrier reads no clock and no round times out (FR-014). Observe red against the named `ContextBarrier` before T013 (FR-042)

- [X] T013 In `source/harness/execution_context.cpp`, implement `ContextBarrier`: one atomic arrival count, one generation word, and the expected count fixed at construction; the arrival that reaches the expected count resets the count, advances the generation, and releases the round for all (FR-014, D-03); a waiting participant watches the generation word in the wait mode of its own policy, the same three paths as the context loop, with `std::barrier` standing as the recorded fallback for the `BLOCK` path (FR-014, Q-3); a participant that arrives with no context waits in `BLOCK` (FR-014); leaving a round early trips the `SG_REQUIRE` contract (FR-015); a zero participant count trips the contract at the call (E-04, FR-014)

**Checkpoint**: `ctest --test-dir build -R harness_context_barrier_test` passes; the threaded rounds have their mechanism.

## Phase 4: User Story 2 - Run one benchmark on N contexts (Priority: P1)

**Goal**: `threads(4)`, `threadRange`, and `denseThreadRange` run each
instance they select on N library-owned contexts with the barriers
outside every window, `threadIndex()` and `threads()` visible to the
body, and one merged row following the cited aggregation (spec US2).

**Independent Test**: "register a benchmark on a fake provider whose
per-context leaves are scripted, run it with `threads(4)`, and compare
the merged row against the aggregation rule of FR-020, context by
context. A start barrier and a stop barrier stand outside every
measured window, so each scripted window holds only the body's work."
(spec.md US2)

### Tests for User Story 2 (write first, observe failure)

- [X] T014 [P] [US2] Create `test/source/harness_threads_test.cpp` and register `harness_threads_test` in `test/CMakeLists.txt` with `target_include_directories(... ${CMAKE_SOURCE_DIR}/source/harness)` as the calibration tests do (the test scripts internal records); scenarios: `threads(4)` gives `threadIndex()` values 0 to 3, `threads()` 4, and a row name ending in `/threads:4` (SC-06, FR-016, FR-017); with scripted per-context leaves the merged row equals the summed real time, the summed cpu time, and the summed iterations, the manual-time sum standing in the real column under manual timing, counters summed before the flag pass, and every counter flag equals the cited `Finish` result at N = 4 (SC-07, FR-020); `threadRange(min, max)` selects the doubling counts and `denseThreadRange(min, max, step)` the strided counts, one instance per count, each named `/threads:N` (FR-016); window bounds recorded around the barrier arrives show no barrier wait inside a window (SC-08, FR-019); `threads(0)` and any count the call cannot honor abort at the call (FR-024); the threaded instance creates its N contexts once and reuses them across its calibration attempts and repetitions (D-05); a benchmark calling no thread method keeps today's single-thread behavior and today's row bytes, and single-context `State` reads index 0 and count 1 (FR-017, FR-022); `setUp` and `tearDown` run once per run on context 0, outside both barriers (FR-023); a custom thread runner's dispatches carry the bodies while the harness keeps the barriers and the merge (FR-021); a runner that returns with a body undispatched or dispatched twice aborts its re-exec at that point (FR-021, Edge Cases); a context whose body skips leaves the row carrying the skip with no partial row published (Edge Cases); a threaded run whose per-context iteration count is zero keeps the calibration bounds and reaches no division by zero in the flag pass (Edge Cases). Observe red against the T002 declarations before T018 through T020 (FR-042)

- [X] T015 [US2] Append the default-timing decision scenario to `test/source/harness_threads_test.cpp`: with `N > 1` on scripted per-context leaves, the merged row's real column carries the summed real time, the cpu column the summed cpu time, and the `RATE` counter divides by the summed cpu time divided by `N`, the decision seconds of the default mode (FR-020, D-06; SC-07). Observe red before T020 exists (FR-042)

- [X] T016 [US2] Append the real-time-mode decision scenario to `test/source/harness_threads_test.cpp`: with `N > 1` under real-time timing, the real column carries the summed real time and the `RATE` counter divides by that summed real time divided by `N` (FR-020, D-06; SC-07). Observe red before T020 exists (FR-042)

- [X] T017 [US2] Append the manual-mode decision scenario to `test/source/harness_threads_test.cpp`: with `N > 1` under manual timing, the real column carries the summed manual time and the `RATE` counter divides by that summed manual time divided by `N` (FR-020, D-06; SC-07). Observe red before T020 exists (FR-042)

### Implementation for User Story 2

- [X] T018 [US2] Define the handle calls in `source/harness/registry.cpp` behind the standing setter precondition: `threads` stores `{n}`, `threadRange` stores `min, min * 2, ...` up to `max`, `denseThreadRange` stores `min, min + step, ...` up to `max`, mirroring the cited `ThreadRange` and `DenseThreadRange`, and `threadRunner` attaches the user callable (FR-016, FR-021, D-09); a count below one trips the standing setter precondition plus the `SG_REQUIRE` at the call (FR-024); in `source/harness/family.cpp` expand instances over the argument set crossed with the thread counts and have `instanceName` append the `/threads:N` segment for a count above one, so "the default `{1}` keeps every untouched family byte-identical" (FR-016, FR-022); define `State::threadIndex()` and `State::threads()` in `include/speedgun-ng/benchmark.hpp` over the two fields (FR-017); `catalog.cpp` and `cli.cpp` gain nothing and no command-line option lands (FR-044)

- [X] T019 [US2] In `source/harness/runner.cpp`, add the threaded instance beside the single-context path: create the N contexts with policy `{SPIN, NO_PIN}` once for the instance, reuse them for every calibration attempt and repetition, and join them when the instance finishes, thread start and join outside every measured window (FR-004, Q-2, D-05); dispatch each body to its context as one erased work node that opens its own thread-target plan and recorder on its own thread, arrives at the start barrier outside the window, runs the unchanged timed cursor, arrives at the stop barrier outside the window, folds its own plan, and publishes per-context values, so "no fold shall cross contexts: no value of one context's row shall be read from or written into another context's plan" (FR-018, FR-019, SC-08, PC-1); run `setUp` and `tearDown` once per run on context 0 outside both barriers through the existing macro slot written once before any dispatch, and replace the stale fixture-slot comment beside the macro machinery in `include/speedgun-ng/benchmark.hpp` with the stated rule (FR-023, D-07); wait the N completion states with acquire order (FR-049), publish the merged row, and when the instance finishes `requestStop()`, `drain()`, and `join()` every context (FR-006, FR-009); a custom thread runner performs the dispatches through the `ThreadRun` surface (contracts/threaded-runs.md) while the barriers, plans, and merge stay the harness's; the harness builds the body nodes, keeps them alive across the call, and checks the dispatch ledger when the runner returns, a body left undispatched or dispatched twice tripping the `SG_REQUIRE` contract at that point (FR-021, Edge Cases)

- [X] T020 [US2] In `source/harness/runner.cpp`, implement the merged row and the real-N flag pass (cold code after the stop barrier): "the real column carries the summed manual time under manual timing and the summed real time otherwise, the CPU column carries the summed CPU time, the iterations are the summed per-context counts, and the user counters of the contexts are summed before the flag pass" (FR-020, E-06); inside the existing `appendCounterColumns` run the flag pass with the real `N` in the cited order: `RATE` divides by `(primaryNs / 1e9) / N`, the `primaryNs` the summed decision seconds of the run's timing mode in nanoseconds (cpu by default, manual under manual timing, real under real-time timing), `AVG_THREADS` divides by `N`, `ITERATION_INVARIANT` multiplies the merged iteration total, `AVG_ITERATIONS` divides by it, `INVERT` applies last (D-06, PC-2); with `N == 1` the formulas reduce to the shipped single-thread pass, so an untouched family prints untouched bytes (FR-022)

**Checkpoint**: `ctest --test-dir build -R harness_threads_test` passes; the threads feature the roadmap names stands, and `harness_instance_name_test` and the family tests stay green untouched (FR-022).

## Phase 5: User Story 3 - Run a noise load beside a measurement (Priority: P2)

**Goal**: A load runs continuously on one context while a benchmark
runs on another, and the benchmark's plan reads only its own context
(the D-3 example; spec US3). The scenario test rides the shipped
surface: this story adds no implementation.

**Independent Test**: "script the provider, start a load on one
context that never stops on its own, run one benchmark on a second
context, and check that the benchmark completes and that its plan
reads only the leaves of its own context." (spec.md US3)

### Tests for User Story 3 (write first, observe failure or pass on the shipped surface)

- [X] T021 [US3] Append the noise-load scenarios to `test/source/harness_threads_test.cpp`: a load context running continuously beside the benchmark lets the benchmark complete and print its row (SC-09); every value of the benchmark's row comes from the plan its own context opened, with the load context's leaves scripted to distinct values so no merged value can come from them (FR-018, SC-09, PC-1); a second dispatch thread proves "work dispatched from inside a running item runs after that item ends, on the same context" end to end (FR-010, FR-013). These scenarios test behavior T010 through T020 shipped: they pass once the shipped surface holds and fail against any build lacking it, which is the TDD pair recorded in those tasks (FR-042)

**Checkpoint**: `ctest --test-dir build -R harness_threads_test` passes with the noise-load scenarios; the public API answers the D-3 example.

## Phase 6: User Story 4 - Read the contexts in the report (Priority: P2)

**Goal**: Threaded rows disclose the context count and each policy, and
an oversubscribed spinning setup prints one noise warning line (spec
US4).

**Independent Test**: "run benchmarks under a scripted policy set and
compare the printed disclosure for each case: two spinning contexts on
one cpu, more spinning contexts than online cpus, one pinned context
per cpu, and a blocking context. The warning appears in the first two
cases and in no others." (spec.md US4)

### Tests for User Story 4 (write first, observe failure)

- [X] T022 [P] [US4] Create `test/source/harness_context_report_test.cpp` and register `harness_context_report_test` in `test/CMakeLists.txt`, building policy vectors directly as `contracts/report-disclosure.md` states; scenarios: a threaded result prints the context count and each context's wait mode and placement after its rows, and a single-context result "keeps the bytes it prints today" with no disclosure line (SC-12, FR-025); the four FR-026 cases, "a shared-cpu spin pair, a spinning count above the online cpu count, a pinned setup with one context per cpu, and a blocking setup", with the warning line appearing after the result whose condition holds, once for each such result, and nowhere else (FR-026, FR-028, SC-12); the test reads the same online-cpu fact the warning reads (D-08). Observe red before T024 and T025 (FR-042)

- [X] T023 [P] [US4] Extend the report-shape gate in the same commit range as T025, per `contracts/report-disclosure.md`: teach `tools/docs/report_shape.py` and its fixture runner under `test/` the disclosure line and the noise warning, "every token the built-in fields print today keeps its bytes", with fixture cases that pass the line and the warning and fail a planted mismatch; the README sample report block keeps its exact bytes (FR-025, FR-026, SC-11)

### Implementation for User Story 4

- [X] T024 [US4] Fill the disclosure data: in `include/speedgun-ng/benchmark.hpp` complete the `BenchmarkResult::contexts` record `{waitMode, placement}` (E-07); in `source/harness/runner.cpp` fill one record per context from the contexts' fixed policies when the threaded instance joins its contexts (FR-025, FR-007), leaving the vector empty for a single-context run (FR-022); every threaded result carries one record for every context it ran, so a non-empty vector names exactly a threaded result (FR-025, FR-021)

- [X] T025 [US4] In `source/harness/report.cpp`, print after the rows of a threaded result, which is exactly a result whose `contexts` vector is non-empty (FR-025), one disclosure line naming the count and each context's mode and placement, `NO_PIN` or one cpu index, the printer adding nothing to the data (FR-025); compute the noise warning from that vector plus the online cpu count read once per report through `sg::counters::System::local()`, printing the one warning line after a result exactly "when two spinning contexts of that result name one cpu index, or the result has more spinning contexts than the host has online cpus" (FR-026, FR-028, D-08); neither the disclosure nor the warning reads a clock or adds a sampling action (FR-027, FR-039)

**Checkpoint**: `ctest --test-dir build -R 'harness_context_report_test|report_shape'` passes; every threaded row is interpretable.

## Phase 7: User Story 5 - Read the page, copy the block, run the example (Priority: P3)

**Goal**: Two D1-shaped main-page sections, their owning runnable
examples, quick-reference rows, a `/threads:N` row in "Reading the
report", and the guide detail (spec US5, IV.1).

**Independent Test**: "build and run the two new example programs, run
the drift check over the page, and paste each block into a
translation unit of your own." (spec.md US5)

### Implementation for User Story 5

- [X] T026 [P] [US5] Create `example/docs/execution_contexts.cpp` teaching "a context, a policy, and the dispatch of a noise load", and `example/docs/threads.cpp` teaching `threads(4)` and `threadIndex()`, each a complete program with the canonical includes and a `main` calling `sg::speedgunMain`, each carrying the marked region comment the page copies, every new call reached through the public `speedgun-ng` headers alone (FR-029, FR-030, FR-047)

- [X] T027 [US5] In `example/CMakeLists.txt`, wire both files through the shipped `add_docs_example(NAME)` shape so each builds `docs_<name>` with a `run_docs_<name>` target on `run-examples`, which puts both under the `example_run` gate where "an undeclared skip, a `FAILED` row, and a registered benchmark whose row never prints shall each fail the gate" (FR-030, FR-035); edit `example/docs/reading_the_report.cpp` and its `.expected` to gain a `/threads:N` row whose FR-025 disclosure is legible (FR-032)

- [X] T028 [US5] In `README.md`, add the "Execution contexts" and "Threads" sections in the D1 shape of "one purpose sentence, one marked snippet, and one result sentence", copying the T026 regions verbatim (FR-029); add the quick-reference row for each new public name this feature ships, in the table's own columns, and add no option-table row, since this feature adds no command-line option and any option that later lands must fail closed with one line and exit status 2 (FR-031, FR-044); show the `/threads:N` row with its disclosure in "Reading the report" (FR-032)

- [X] T029 [P] [US5] In `docs/pages/harness.md`, add the detail the page keeps its distance from: the FR-020 aggregation table and the FR-006 lifecycle rules, "in the reader voice the rest of that page keeps and with no spec numbers" (FR-033, FR-048)

**Checkpoint**: Both example programs build and run; the page carries the two sections once T030 moves the cap.

## Phase 8: Polish, Gates, and Version

**Purpose**: The measured page-cap move, the version-table entry, the
H16 and reference-record audits, and the run of every gate at the
feature head (FR-034, FR-041 to FR-048, Principle VIII, SC-10, SC-11).

- [X] T030 In `tools/docs/example_drift.py`, add the two new section titles to `SECTIONS` and their owner files to `SECTION_OWNERS`; move `PAGE_LINE_CAP` from 570 by the measured size of the two new sections and their quick-reference rows, recorded in the "Budget note" section of `specs/020-execution-contexts-core/spec.md`; "SECTION_LINE_CAP shall stay 25"; trim prose first and drop no snippet to fit; edit no file under `specs/019-harness-user-counters-and-statistics/`; then `python3 tools/docs/example_drift.py .` exits 0 with the nineteenth and twentieth sections in budget (FR-034, SC-11; depends on T026 through T029)

- [X] T031 [P] Land the plan's version-table entry in `CMakeLists.txt`: `project(speedgun-ng VERSION)` from 0.9.0 to 0.10.0 and the `speedgun-ng_harness` archive `SOVERSION` from 5 to 6, while the `speedgun-ng` archive `SOVERSION` stays 2 and `cmake/install-rules.cmake` keeps `SameMinorVersion`; the context core ships in the harness archive with no new target, and `speedgun-ng::exec` can come with H16 (FR-043; verify: `grep -n 'VERSION 0.10.0' CMakeLists.txt`, `grep -n 'SOVERSION 6' CMakeLists.txt`, `grep -n 'SOVERSION 2' CMakeLists.txt`)

- [X] T032 [P] Audit the four H16 hooks against `contracts/execution-context.md` at the feature head: the node completion state (`PENDING`, `RUNNING`, `DONE`, `STOPPED`), `dispatch()` from inside running work, `ContextId`, and the placement slot "ship as part of the public surface, shaped for this feature alone", each declaration byte-equal to the contract block, no second use or second meaning anywhere in the tree, and "H16 shall add continuations and dependencies on top of them with these signatures unchanged" (FR-045)

- [X] T033 [P] Audit the Google Benchmark records: the pinned revision `e662de9aab8e705ecf4fa4bd41a207e5a0acfd0c` stands named in research D-10 and `contracts/threaded-runs.md`, every kept difference sits beside its semantic, including "the fixture rule of FR-023 and the barrier of FR-014" and the runner thread running no body (FR-046); confirm the shipped headers carry additions only, no existing signature changed, which is PC-0's rule that this spec replaces no shipped signature, and the `git diff` of `include/speedgun-ng/benchmark.hpp` against the audit point `64f2ab5` touches no shipped declaration (FR-046)

- [X] T034 [P] Confirm "the examples and the documentation reach the new calls through the public `speedgun-ng` headers alone": the two new example files, the README regions, and `docs/pages/harness.md` reference no header under `source/harness/` (FR-047)

- [X] T035 [P] Run quickstart.md sections "Configure and build", "C-1 to C-3: context, policy, and typed work", "C-4: the context barrier", and "C-5: threads on contexts" from the repository root and record the results (FR-042, SC-01 through SC-08)

- [X] T036 [P] Run quickstart.md sections "C-7: disclosure and noise warning", "C-6: docs and examples", and "The version table", including `ctest -R example_run`, `python3 tools/docs/example_drift.py .`, and the three version greps, and record the results (FR-035, FR-043, SC-11, SC-12; depends on T030 and T031)

- [X] T037 [P] Run the full suite and the standing gates at the feature head on the `ci-rocky` release build: `ctest --test-dir build --output-on-failure` passes; `time_source_gate` reports no hit, the counters-only clause keeping every forbidden time source out of the harness (FR-039); `loop_shape` stays green with "neither a timed loop nor `dispatch()`" allocating (FR-040); `barrier_shape` and `cli_shape` stay green untouched, no option landed (FR-044); the dbc pair gate reports zero drifted interfaces (FR-037); no new macro enters, the `SG_` rule of FR-036 standing by absence, the clang-tidy name check being T041's; the prose gate and commit lint report zero findings over the feature's words and commits (FR-048); `spell-check` and `format-check` pass (SC-10)

- [X] T038 [P] Build and run the sanitizer presets: `cmake --preset=ci-sanitize` with its `ctest` run reports no ASan or UBSan error, and `cmake --preset=ci-tsan` with its `ctest` run reports no race on the inbox, the barrier, the lifecycle, the threaded paths, and the disclosure reads, the preset staying apart from `ci-sanitize` because the compilers reject the pairing (FR-041, SC-10, Principle VIII)

- [ ] T039 [P] Build the `ci-coverage` preset, run its suite and coverage target, and confirm 100% line and 100% branch over the new paths with the report's evidence; `grep -rn LCOV_EXCL source/harness` prints no line, "no `LCOV_EXCL` marker shall enter `source/harness/`" (FR-038, SC-10)

  The capture compiles the coverage preset with `-fprofile-update=atomic`, so
  the counters a threaded test updates from several contexts at once
  accumulate without a data race, and `ctest` runs the suite with `-j 2`. Each
  site named across the earlier heads of this branch now carries its own
  account. `source/counters/clock_provider.cpp:60` is the continuation of the
  parse loop that `counters_clock_push_test` opens with the two addresses no
  leaf publishes. The skip rule of the threaded merge at
  `source/harness/runner.cpp:419` stands in `detail::takesSkip`, whose four
  pairs of inputs the `mergeSkipRule` mode of `harness_gap_test` drives, the
  pair where the merged record already carries a skip included; the choice of
  the primary time column moved out of the fold lambda, which the build
  instantiates once per recorder, into `selectPrimaryCore`, so the answer a
  manual-time run gives is read where the fold calls it; the error arm at
  `source/harness/runner.cpp:1465` belongs to `scenarioBodyThrows`, which
  throws through `threads(kThreadCount)`.

  In `source/harness/execution_context.cpp` the guards stood against events a
  test cannot schedule from outside the code that makes them: a dispatch
  standing between the accept store and the link store, and a wake that returns
  while its own round still stands open. Those states have no branch of their
  own now. The walk's decision for one pass stands in `detail::walkStep` over
  the successor link, the tail's position and the walk's position, and the
  drain proof in `detail::inboxIdle` over the tail, the phase and the dispatch
  pair, which the `walk-states` scenario drives row by row: a pass that finds a
  producer mid-enqueue answers `PENDING`, the account holds that accept apart
  from its dispatch there, so the loop turns again (D-11, D-14). The barrier
  tests its round at the foot of its wait through `detail::roundOpen`, where
  both outcomes of that test are ordinary events and the comparison of the
  generation stands apart from the back edge (D-12).

  `grep -rn LCOV_EXCL source/harness` prints no line. The box stands until the
  coverage job of this branch's pull request reports `lines.......: 100.0%` and
  `branches....: 100.0%`, and no marker, threshold or exclusion shapes that
  verdict.


- [X] T040 Main-page impact check (IV.1): confirm the two new README sections, their owning `example/docs/` files, their quick-reference rows, the `docs/pages/harness.md` detail, and the measured cap move all shipped in the same change as the code they teach, each snippet a region a CI-built example prints (FR-029 to FR-035, SC-11)

- [X] T041 [P] Run the analyzer-bearing release build at the feature head: `cmake --preset=ci-ubuntu && cmake --build build` completes with clang-tidy and cppcheck wired into the build, `readability-identifier-naming` reports zero findings, and no new macro skips the `SG_` rule (FR-036; Principle IX's per-feature release build and Principle VIII's static-analysis gate)

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: no dependencies; the named surface blocks every story test (T001 and T002 are different files; T003 includes T001)
- **User Story 1 (Phase 2)**: depends on Setup; it is the MVP
- **Context barrier (Phase 3)**: depends on Setup and on the wait modes of T010 (participants wait in the context's own mode); blocks US2
- **User Story 2 (Phase 4)**: depends on the barrier phase
- **User Story 3 (Phase 5)**: depends on US2; it adds tests over the shipped surface
- **User Story 4 (Phase 6)**: T022 and T023 depend on Setup; T024 and T025 depend on the threaded attempt of T019 for the fill
- **User Story 5 (Phase 7)**: T026 depends on US1 (the first example) and US2 (the second); the drift and run gates in Phase 8 depend on the sections landing first
- **Polish (Phase 8)**: T030 depends on the sections; T036 depends on T030 and T031; T041 closes the analyzer gate once the head stands; the gate runs close the feature

### User Story Dependencies

- **US1 (P1)**: independent of every other story; everything else rides it
- **US2 (P1)**: needs the barrier phase; FR-004's harness-side `SPIN` default lands inside T019
- **US3 (P2)**: tests only; needs US1 and US2 shipped
- **US4 (P2)**: the disclosure record is named in T002; the fill needs the threaded attempt (US2)
- **US5 (P3)**: the sections teach US1 and US2; the page cap move (T030) must follow the measured landing of the sections, per Q-5 and FR-034

### Within Each User Story

- Tests are written first and observed failing (FR-042, TDD mode, fake provider)
- Named declarations (Phase 1) before tests before behavior
- Runner mechanics before the merge; the merge before the report fill
- A story is complete when its CTest executable passes and the existing gates named in plan.md stay green

### Parallel Opportunities

- T001 and T002: two headers, different files
- T004, T012, and T014: three test files once Phase 1 names their surface; the `test/CMakeLists.txt` registration lines land serially when these run together
- T022 and T023: the report test and the report-shape gate are different files
- T026 and T029: the examples and the guide page are different files
- T031 through T035, T037 through T039, and T041: version, audits, quickstart, sanitizers, coverage, and the analyzer build touch different work and run independently
- After Phase 3, US4's tests (T022, T023) can be written while US2's implementation continues

## Parallel Example: User Story 2

```bash
# After Phase 1 and the barrier, launch together (different files):
Task: "Create test/source/harness_threads_test.cpp (T014)"
Task: "Create test/source/harness_context_report_test.cpp (T022)"
Task: "Extend the report-shape gate and fixtures (T023)"
# T018, T019, T020 share runner-side files and land in order:
Task: "Handle calls, registry, family expansion, State (T018)"
Task: "Threaded attempt, barriers outside windows, fixtures, custom runner (T019)"
Task: "Merged row and the flag pass at the real N (T020)"
```

## Implementation Strategy

### MVP First (User Story 1)

1. Complete Phase 1 (T001..T003): the named seams.
2. Complete Phase 2 (T004..T011): the context, policy, typed work, and
   dispatch. This is the MVP: D-1 and D-3 ship, and a caller can own a
   context today.
3. STOP and VALIDATE: `ctest -R harness_execution_context_test` plus
   the Phase 8 gates on the partial tree.

### Incremental Delivery

1. Phase 3: the barrier lands with its own executable.
2. US2: threads on contexts, the merged row (the roadmap's threads
   feature).
3. US3: the noise-load scenarios prove the public-API example.
4. US4: the disclosure and the noise warning make threaded rows
   interpretable.
5. US5: the page, the two examples, the guide detail.
6. Phase 8: the measured cap move, the version entry, the hook and
   reference audits, and every gate at the feature head.

## Notes

- [P] tasks touch different files with no dependency on an open task
- Each story's checkpoint is an independent validation point
- Every quoted bound is a testable precondition; the re-exec pattern of
  plan.md's Test Plan holds the abort cases, and wait-mode tests
  subscribe to the completion state before triggering, never a fixed
  sleep
- The pause-hint asm site is the one recorded P2 exception (plan.md
  Complexity Tracking); the four H16 hooks, the placement function, and
  `threadRunner` are the three spec-named seams, nothing beyond them
  (Principle X.2)
- Commit cadence follows the repository rule (`<Section>: <one-line
  imperative>`, body, `Approved-by:`, `Refs: specs/020-execution-contexts-core`),
  one atomic commit per task group or finer (FR-048)
