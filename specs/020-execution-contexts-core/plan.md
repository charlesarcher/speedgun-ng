# Implementation Plan: Execution Contexts Core

**Feature branch**: `020-execution-contexts-core` | **Spec**: [spec.md](./spec.md)

**Audit point**: `64f2ab553fee9753a56f9fba08d42a506aac3af0`
(`64f2ab5`, 2026-10-15), the head the spec's audit section names.
**Date**: 2026-10-15 | **Constitution**: 2.20.0, unchanged by this
feature | **Reference revision**: `google/benchmark` at
`e662de9aab8e705ecf4fa4bd41a207e5a0acfd0c`, fetched and read for this
plan (research D-10; the revision 019 pinned for its reads, verified
by fetch on 2026-10-15).

## Summary

H5 ships the execution context core: a context owns one thread and one
event loop, its policy picks a wait mode and a placement slot, typed
work nodes ride an intrusive MPSC inbox through one erased run path,
and a context barrier joins contexts in rounds that release all
waiters at once. The Google Benchmark thread calls land on top:
`threads(n)`, `threadRange`, and `denseThreadRange` run each instance
they select on N library-owned contexts, the start and stop barriers
stand outside every timed window, each context folds its own
thread-target plan, and one merged row carries the cited aggregation
and the flag pass at the real `N`. The report discloses each threaded
run's context count and policies and warns once when spinning contexts
are oversubscribed. No graph engine ships: the four H16 hooks are the
four names FR-045 lists, kept exactly as the spec states them. The
owner's note of 2026-10-15, that this should stay simple for now,
binds the design: one research entry per open question, no graph
machinery, no speculative alternatives.

## Technical Context

**Language/Version**: C++23, the pinned standard of Principle I. One
documented inline-asm site enters, the spin's pause hint, beside the
shipped `barrier.hpp` precedent and recorded in Complexity Tracking as
the 011 SSC-marker practice shows. No other extension enters.

**Primary Dependencies**: the counters library at the audit point
(`Target`, `Plan`, `Recorder`, the fold, and the `FakeProvider` the
tests script per context), the H1 to H4 harness (the runner, the
family record, the report, the calibration rule, the pause path), and
the C++ standard library (`std::thread`, `std::atomic` and its wait,
`std::barrier` as the recorded barrier fallback). Google Benchmark is
a reference read; nothing links against it and none is vendored.

**Storage**: N/A on the filesystem. Nodes live in caller storage
between dispatch and completion; a context holds its inbox over those
nodes; threaded run data lives in per-context recorders and the merged
row, all within one run's lifetime.

**Testing**: CTest executables in `test/`, every time figure scripted
on the fake provider's per-context leaves, an allocation-hook test for
the dispatch path, the re-exec pattern for the `SG_REQUIRE`
violations, and the `ci-tsan` and `ci-sanitize` presets over every new
path (FR-041, SC-10).

**Target Platform**: Linux, the CI matrix of Principle VIII: GCC and
Clang, `ci-sanitize`, `ci-tsan`, and `ci-coverage`.

**Project Type**: library plus the harness archive the library
exports. No new target: the context core compiles into
`speedgun-ng_harness`, and `speedgun-ng::exec` can come with H16
(FR-043).

**Performance Goals**: the timed loop keeps its H1 instruction count,
so `loop_shape` stays green; `dispatch()` is two atomic operations on
a caller-owned node with zero allocation (FR-003, FR-010, SC-04); an
idle loop reads one inbox flag; the barrier is one counter and one
generation word; the merge, the disclosure, and the warning run cold,
after the stop barrier.

**Constraints**: every time and counter value stays in
`sg::counters`, so the time-source gate stays clean (FR-039); no wait
mode, no barrier round, no disclosure, and no warning reads a clock
(FR-004, FR-014, FR-027); 100% line and branch coverage with no
`LCOV_EXCL` marker under `source/harness/` (FR-038); a benchmark that
calls no thread method keeps today's bytes and behavior (FR-022,
FR-025); no command-line option lands (FR-044).

**Scale/Scope**: 50 requirements, 7 capabilities, one new public
header, one new harness source, five edited harness sources and one
edited public header, four new test executables, two new example
files and one edited example, two new main-page sections plus two
edited ones, one measured page-cap move, and one version-table entry.

## Constitution Check

*(Gate: principles I to XI of 2.20.0 pass, under the P0 to P3 priority
model of Principle I: P0 techniques stand on the designated critical
paths only, P1 conventions bind, P2 exceptions carry written
justification, P3 defaults hold wherever no exception is recorded. Any
**NO** needs an entry in Complexity Tracking.)*

| Principle | Verdict and note |
| --- | --- |
| I | PASS with one recorded exception. C++23 throughout; `std::thread`, `std::atomic` with its wait, and `std::barrier` carry the new machinery. The spin's pause hint is one `asm volatile("pause" ::: "memory")` site, the shape the shipped `barrier.hpp` and the 011 SSC marker already document; it stands in Complexity Tracking with the site-level justification, registered under Principle I in spec.md's Constraints as this feature's P2 exception. The P0 paths (timed loop, dispatch, wait, barrier) stay inside their budgets (research D-01, D-03). |
| II | PASS. Every new public interface documents `\pre`, `\post`, and `\invariant`, and the source enforces each clause through `SG_REQUIRE`: the destructor on an unjoined context, `join()` from the context's own thread, `drain()` from it in a debug build, the invalid cpu index, the early-destroyed node under the enforce semantic, the barrier participant that leaves a round early, and the unhonorable thread count (FR-005, FR-006, FR-012, FR-015, FR-024, FR-037, FR-050). |
| III | PASS. Each capability carries its requirement, design view, source file, and test executable, and this plan records TDD mode on the fake provider with every test seen failing first (FR-042). The logical and physical views are ASCII diagrams and a table, the recorded design-view shape the harness series has carried since 015. |
| IV | PASS. The new public declarations carry doxygen blocks, the main page gains its two sections from marked regions of files CI runs, and `docs/pages/harness.md` gains the aggregation table and the lifecycle rules (FR-029, FR-030, FR-033). |
| V | PASS. N-1 covers `ExecutionContext`, `ContextId`, `ContextPolicy`, `WaitMode`, `PlacementSlot`, `WorkNode`, `WorkContext`, `WorkItem`, and `ContextBarrier`; the enumerators `SPIN`, `YIELD`, `BLOCK`, and `NO_PIN` follow N-4; `dispatch`, `currentContext`, `requestStop`, `drain`, `join`, `arriveAndWait`, `threadIndex`, `threads`, `threadRange`, and `denseThreadRange` follow N-2; struct members follow N-12. No new macro enters, so the `SG_` rule of FR-036 stands satisfied by absence. |
| VI | PASS. Four new executables cover the lifecycle branches, the wait modes, the barrier rounds, the merge arms, the disclosure line, and the four warning cases; the 100% line and branch gates stay measured on the same paths with no `LCOV_EXCL` marker. The row reads PASS when this branch's coverage job prints 100.0% line and 100.0% branch (FR-028, FR-038). |
| VII | PASS. The critical paths are the loop, the dispatch, and the wait, each bounded by D-01 and D-03. The context creation, the barrier construction, the merge, the disclosure, and the warning all sit cold. FR-026's warning is FR-VII's measurement-noise concern expressed as a report rule; the default `SPIN` for measured contexts answers it at the policy level (Q-2). |
| VIII | PASS. No gate weakens. Every new path runs under `ci-tsan` and `ci-sanitize` (FR-041, SC-10), the version and `SOVERSION` table records the rise FR-043 requires, and the docs gates gain two sections within the FR-034 budget rule. The row reads PASS when this branch's coverage job prints 100.0% line and 100.0% branch. |
| IX | PASS. Spec, plan, research, data model, contracts, and quickstart live in `specs/020-execution-contexts-core/`, and this plan records TDD mode. |
| X | PASS. The design carries no graph machinery, no reserved parameter, and no seam beyond the three the spec itself names: the four H16 hooks of FR-045, the FR-005 placement function, and the FR-021 custom runner. Each stands in Complexity Tracking with its FR as the justification, because X.2 bars hooks the spec does not ask for and these the spec does. |
| XI | PASS. The artifacts and the code prose obey XI.1 to XI.7 (FR-048); this plan was written to that rule. |

**Gate weakening requested**: no. **Gate strengthening requested**:
no. The page cap moves by the measured amount FR-034 states as its own
budget rule, inside the drift tool, and no constitution text changes.

## Project Structure

### Documentation (this feature)

```text
README.md                              # Edited: "Execution contexts" and
                                       #   "Threads" sections in the D1 shape,
                                       #   quick-reference rows for the new
                                       #   public names, a /threads:N row in
                                       #   "Reading the report", FR-029 to
                                       #   FR-032
docs/pages/harness.md                  # Edited: the FR-020 aggregation table
                                       #   and the FR-006 lifecycle rules, in
                                       #   the page's reader voice (FR-033)
tools/docs/example_drift.py            # Edited: the two new titles in SECTIONS
                                       #   and SECTION_OWNERS, PAGE_LINE_CAP
                                       #   moved by the measured FR-034 amount,
                                       #   SECTION_LINE_CAP held at 25
```

### Source (this feature)

```text
include/speedgun-ng/
├── execution_context.hpp              # Added: ContextId, currentContext,
│                                      #   WaitMode, PlacementSlot,
│                                      #   ContextPolicy, ExecutionContext,
│                                      #   StopToken, WorkItem, WorkNode,
│                                      #   WorkNodeState, WorkContext,
│                                      #   PlacementResult, PlacementFn,
│                                      #   setPlacementFunction, dispatch,
│                                      #   ContextBarrier
├── benchmark.hpp                      # Edited: State::threadIndex and
│                                      #   State::threads, the BenchmarkHandle
│                                      #   threads, threadRange,
│                                      #   denseThreadRange, and threadRunner
│                                      #   calls, the fixture-slot comment
│                                      #   replaced by the threaded rule (D-07)
└── counters*.hpp                      # unchanged

source/harness/
├── detail/internal.hpp                # Edited: RegistryEntry gains the thread
│                                      #   count set and the optional custom
│                                      #   runner
├── execution_context.cpp              # Added: the loop, the three wait modes,
│                                      #   the pause-hint helper, the placement
│                                      #   function, the dispatch, the barrier
├── registry.cpp                       # Edited: the four new handle calls
│                                      #   behind the standing setter
│                                      #   precondition
├── family.cpp                         # Edited: instances expand over the
│                                      #   thread counts, instanceName appends
│                                      #   the /threads:N segment
├── runner.cpp                         # Edited: the threaded instance
│                                      #   (contexts created once per instance,
│                                      #   per-context plans, barriers,
│                                      #   fixture on context 0), the merged
│                                      #   row, and the flag pass at the real N
├── report.cpp                         # Edited: the disclosure line and the
│                                      #   noise warning, the single-context
│                                      #   bytes unchanged
├── catalog.cpp, cli.cpp, complexity.cpp # unchanged; cli gains no option (FR-044)
CMakeLists.txt (repository root)       # Edited: the project() VERSION 0.10.0,
                                       #   the harness target SOVERSION 6, the
                                       #   new source in speedgun-ng_harness

test/
├── CMakeLists.txt                     # Edited: four new executables
├── source/harness_execution_context_test.cpp # Added: C-1, C-2, C-3
├── source/harness_context_barrier_test.cpp   # Added: C-4
├── source/harness_threads_test.cpp           # Added: C-5
├── source/harness_context_report_test.cpp    # Added: C-7
├── loop_shape.sh, barrier_shape.sh, cli_shape.sh,
│   time_source_gate.sh                # unchanged, all stay green
└── example_run.sh                     # unchanged logic, runs the two new
                                       #   docs examples (FR-035)

example/
├── docs/execution_contexts.cpp        # Added: owns the "Execution contexts"
│                                      #   region; teaches a context, a policy,
│                                      #   and a noise-load dispatch (FR-029)
├── docs/threads.cpp                   # Added: owns the "Threads" region;
│                                      #   teaches threads(4) and threadIndex
├── docs/reading_the_report.cpp        # Edited: gains a /threads:N row whose
│   .expected                          #   disclosure is legible (FR-032)
└── CMakeLists.txt                     # Edited: two add_docs_example calls

.specify/memory/constitution.md        # unchanged: 2.20.0 stands
```

**Structure decision**: the context core is one new public header and
one new source inside the `speedgun-ng_harness` archive; the barrier
lives in the same source because its state is one counter and one
generation (research D-03), and no detail header enters because the
public header carries every type a caller names (FR-043).

## Design

### Decision index

| Id | Decision | Research |
| --- | --- | --- |
| R-01 | The inbox is a Vyukov link-swap MPSC over caller-owned nodes; the loop waits on one inbox flag with a pause-hint asm site, `std::this_thread::yield()`, or the standard atomic wait | D-01 |
| R-02 | `WorkNode<Item>` stamps one erased run path at construction from the concept-checked item; the loop calls the slot and runs no run-time type test | D-02 |
| R-03 | `ContextBarrier` is an arrival count and a generation word; each waiter waits in its own mode on the generation; `std::barrier` is the recorded `BLOCK` fallback | D-03 |
| R-04 | Placement is the installed `PlacementFn` of one harness-level slot, set before any context starts, called once at thread start, defaulting to a Linux cpu pin and returning the printable `PlacementResult` | D-04 |
| R-05 | A threaded instance creates its N `{SPIN, NO_PIN}` contexts once and reuses them for every calibration attempt and repetition; each body runs as one node with the barrier arrives outside the window, each context folding its own plan; the contexts join when the instance finishes, thread start and join outside every window | D-05 |
| R-06 | The merged row sums real time (the manual-time sum under manual timing), cpu time, and iterations, with counters summed before a flag pass that repeats the cited `Finish` divisors at the real N | D-06 |
| R-07 | The fixture macros keep one slot per site, written once before dispatch; one object serves the run and the N reads sit after that edge | D-07 |
| R-08 | The disclosure rides `BenchmarkResult`; the report prints the line for threaded results only, and the warning computes from the disclosure plus the online cpu count read once through `sg::counters::System` | D-08 |
| R-09 | Thread counts join the family expansion; `instanceName` appends `/threads:N`; the default `{1}` keeps every untouched family byte-identical | D-09 |
| R-10 | The reference revision is `e662de9a...`, read from the four cited files, with the fixture, barrier, and body-placement differences recorded | D-10 |

### Risks

| Id | Risk | Cost | Disposition |
| --- | --- | --- | --- |
| K-01 | The pass that holds no node marks the loop idle and parks in the policy wait, so a producer's open window costs an idle signal and, under BLOCK, a park with its wake (D-14) | One phase store, one epoch increment with its notify, and one park with one wake for each open window | Accepted for this feature: the idle signal is the proof `drain()` and the loop's end read wait for. A benchmark of the cost stands outside this feature's scope. |

### Logical view: the context loop

```text
loop (one thread, research D-01, FR-001, FR-004):
  node = inbox.tryPopOldest()
  if node: node.state = RUNNING;
           run node's erased path with WorkContext{id, stopToken};
           read the next link, notify; node.state = DONE (release);
           after that store the loop touches only its own data (FR-049);
           continue
  if stopRequested: run accepted nodes until the inbox is empty and the
                    loop is idle, then exit (FR-006, FR-050)
  wait by policy on the inbox flag:
    SPIN  : spin + pause hint            (one asm site)
    YIELD : std::this_thread::yield() loop
    BLOCK : flag.wait(...); dispatch() notifies             (FR-004)
```

Enqueue (any thread): `link = node.next.exchange(head_)` style
link-swap; publish `prev.next = node`; two atomics, zero allocation
(SC-04). The enqueue is the single linearization point against
`requestStop()`: a dispatch that observes the stopped state refuses its
node and completes it `STOPPED` itself, and the loop never touches a
node it never accepted (FR-050). `currentContext()` reads the
thread-current id the loop installs around the node's run (FR-002). Work dispatched from inside a
running item is another enqueue: it lands behind the current
node and runs after that item ends, on the same context (FR-013, Edge
Cases).

### Logical view: the threaded instance

The shape is research D-05; the cold steps are the existing
single-context steps and the new parts are exactly the contexts, the
two barrier rounds, and the merge.

```text
create C[0..N-1] once per instance, reused for every attempt
                            policy {SPIN, NO_PIN}        (FR-004, Q-2, D-05)
per attempt and repetition:
  fixture + setUp on C[0]   once per run, outside both   (FR-023, D-07)
  start.arriveAndWait(), stop.arriveAndWait() : two ContextBarrier(N) objects

  body(i) dispatched to C[i], as ONE erased node         (FR-010, D-02):
    open plan(thread target) + recorder   on this thread  (FR-018, PC-1)
    start.arriveAndWait()                 outside window  (FR-019)
    open window; for (auto _ : state) body; close window  (cursor unchanged)
    stop.arriveAndWait()                  outside window  (FR-019)
    fold own plan; publish per-context values             (FR-018)

  wait the N completion states, acquiring them           (FR-009, FR-049)
  tearDown on C[0]                                       (FR-023)
  merge -> one ResultRow; flag pass at N -> appendCounterColumns (FR-020)
requestStop, drain, join every C[i] when the instance finishes (FR-006)
```

With a custom thread runner (FR-021), the runner function performs the
dispatches of the N body nodes onto the N contexts through the
`ThreadRun` surface, one dispatch per body index; the harness builds
the body nodes and keeps them alive across the call. The barriers sit
inside the body nodes, so they stand around a runner the harness did
not write, and the merge is unchanged. A runner that returns with a
body undispatched or dispatched twice trips the `SG_REQUIRE` contract
at that point, because the rounds expect every body (Edge Cases).
Calibration and the
warm-up structure are unchanged: every context of an attempt runs the
same body with the same calibrated count, and calibration reads the
merged decision value.

### Logical view: the merged row and the report line

```text
merge (cold, research D-06, FR-020):
  real = sum of manual_i under manual timing, else sum of real_i
  cpu = sum cpu_i;  iterations = sum iters_i (N times the per-context
                    count, equal counts per D-05)
  counter[name] = sum_i value_i        then the cited flag order:
    RATE / (decisionNs/1e9)/N   AVG_THREADS / N   INVARIANT * iters
    AVG_ITERATIONS / iters      INVERT last
  any context skipped -> the row carries the skip, no partial row

report (research D-08, FR-025, FR-026):
  <rows, bytes unchanged>
  contexts=4  ctx0=SPIN/NO_PIN  ctx1=SPIN/NO_PIN ...   (threaded only)
  warning: spinning contexts oversubscribed (5 spin, 4 cpus)
      printed after a result exactly when that result shows two pinned
      spinners on one cpu or more spinners than the runnable count
```

The single-thread flag pass of H4 reduces to itself at `N == 1`, so an
untouched family prints untouched bytes (FR-022).

### Physical view

| File | Namespace | Holds | Public? | Target |
| --- | --- | --- | --- | --- |
| `include/speedgun-ng/execution_context.hpp` | `sg` | the context core surface and the H16 hooks | yes | `speedgun-ng_harness` |
| `source/harness/execution_context.cpp` | `sg`, `sg::detail` | the loop, wait modes, placement function, dispatch, barrier | no | `speedgun-ng_harness` |
| `source/harness/runner.cpp` | `sg::detail` | the threaded instance (contexts once per instance), the merge, the flag-pass divisor | no | `speedgun-ng_harness` |
| `source/harness/registry.cpp` | `sg` | the four handle calls | no | `speedgun-ng_harness` |
| `source/harness/detail/internal.hpp` | `sg` | the `RegistryEntry` thread selection and runner | no | `speedgun-ng_harness` |
| `source/harness/family.cpp` | `sg::detail` | the thread-count expansion and name suffix | no | `speedgun-ng_harness` |
| `source/harness/report.cpp` | `sg::detail` | the disclosure line and the noise warning | no | `speedgun-ng_harness` |

### Public API surface added

The full contract with signatures and rules stands in
[contracts/execution-context.md](./contracts/execution-context.md)
(context, policy, work, barrier) and
[contracts/threaded-runs.md](./contracts/threaded-runs.md) (handle and
`State`). In brief: `sg` gains `ExecutionContext`, `ContextId`,
`currentContext()`, `ContextPolicy`, `WaitMode`, `PlacementSlot`,
`WorkItem`, `WorkNode`, `WorkContext`, `dispatch()`, and
`ContextBarrier`; `BenchmarkHandle` gains `threads`, `threadRange`,
`denseThreadRange`, and `threadRunner` over `ThreadRunnerFn` with its
`ThreadRun` argument; `State` gains `threadIndex()`
and `threads()` over two fields that default to the single-context
values; `BenchmarkResult` gains the `contexts` disclosure vector. Every
existing signature stands unchanged (PC-0's rule that this spec
replaces no shipped signature).

### Version and `SOVERSION`

The audit-point numbers were read in `CMakeLists.txt` on 2026-10-15
(`VERSION 0.9.0`, counters `SOVERSION 2`, harness `SOVERSION 5`,
`SameMinorVersion` in `cmake/install-rules.cmake`).

| Artifact | At audit point | After H5 | Reason |
| --- | --- | --- | --- |
| `project(speedgun-ng VERSION)` | 0.9.0 | 0.10.0 | FR-043, the decided next minor |
| `speedgun-ng` archive `SOVERSION` | 2 | 2 | the counters surface changes nothing |
| `speedgun-ng_harness` archive `SOVERSION` | 5 | 6 | `State`, `BenchmarkHandle`, and `BenchmarkResult` layouts change and the new public surface joins the archive (FR-043) |
| `install-rules.cmake` compatibility | SameMinorVersion | SameMinorVersion | unchanged |
| constitution | 2.20.0 | 2.20.0 | no amendment in this feature |

## Test Plan

**Execution mode**: TDD (Principle III, FR-042). Each test file is
written and observed red before the implementation that turns it
green, on the fake provider: no test needs a PMU, a sleep for
measurement, or a wall-clock bound for a number. Per-context leaves are
scripted through the existing `FakeProvider` mechanism the 017 and 019
tests use, so the merge and the flag pass assert exact doubles.

**Scenario and check mapping**:

| Capability | Scenario | Check |
| --- | --- | --- |
| C-1, C-2 context and policy | work runs on the context's thread and `currentContext()` names it inside the item (SC-01); each wait mode passes the same dispatch test and `BLOCK` wakes on a dispatch (SC-02); the lifecycle trio runs every queued node exactly once and the destructor requirement aborts (SC-03); the harness-created context reports `SPIN` and the caller-created default reports `BLOCK`, through the disclosure and `policy()`; the replaced placement function runs once per thread and an invalid cpu index aborts the re-exec (SC-05); a `drain()` with no prior stop request flushes while the loop keeps running (FR-050) | `test/source/harness_execution_context_test.cpp`, re-exec pattern |
| C-3 typed work | an unapproved item type fails to compile in the compile-fixture pattern of `test/source/dbc_standalone.cpp`; `dispatch()` records zero allocations through the allocation hook (SC-04); the order rows show single-dispatcher FIFO and per-dispatcher order (FR-013); a node destroyed early aborts under the enforce semantic; a dispatch whose enqueue observed the stop request completes its node `STOPPED` while accepted nodes drain (FR-050); own-thread `drain()` trips the debug precondition in a debug build (FR-050); a node destroyed immediately after its state reads `DONE` passes under `ci-tsan` (FR-049) | same executable |
| C-4 barrier | one round releases every participant; a spinning context waits without sleeping (the mode's own counter proves the path); a participant that leaves a round early aborts the re-exec | `test/source/harness_context_barrier_test.cpp` |
| C-5 threads on contexts | `threads(4)` gives indices 0 to 3, count 4, and a `/threads:4` name (SC-06); with scripted per-context leaves the merged row matches FR-020 and every flag matches the cited `Finish` result at `N = 4` (SC-07); the cpu, real, and manual timing modes each pin the summed decision seconds at `N > 1` (FR-020, D-06); `threads(0)` aborts at the call; no barrier wait falls inside a window (SC-08); a noise load on one context leaves the other context's plan the only one its row folds (SC-09); the custom runner's dispatches carry the bodies and the merged row still stands; the instance's N contexts are created once and reused across attempts (D-05) | `test/source/harness_threads_test.cpp` |
| C-6 docs and examples | the two sections in the D1 shape, the quick-reference rows, the `harness.md` detail, the `/threads:N` row in "Reading the report" | `tools/docs/example_drift.py`, `test/example_run.sh`, prose gate |
| C-7 disclosure | threaded results print the count and each policy and single-context results print nothing new; all four FR-026 cases (shared-cpu spin pair, spinner count above the online count, pinned one-per-cpu, all blocking) with the warning appearing exactly when the condition holds (SC-12) | `test/source/harness_context_report_test.cpp`, `report_shape` checks |
| whole surface | every hard gate of Principle VIII at the feature head, the thread sanitizer over every new path included (SC-10) | SC run of the CI matrix |

**Precondition violations** follow the established pattern: the
scenario re-execs the test binary, the `SG_REQUIRE` ends that process,
and the parent reads the signal, as `harness_pause_test` does.

**Existing tests the changes touch**: `example/docs/reading_the_report.cpp`
and its `.expected` gain the threaded row (FR-032); the report-shape
checks extend to the disclosure line and the warning in the same commit
as the printer; `harness_instance_name_test` and the family tests stay
green untouched, since a default `{1}` thread set composes no `/threads`
segment (FR-022); `cli_shape` stays green untouched, since no option
lands (FR-044).

**Registration pattern**: each new test follows the existing block of
`test/CMakeLists.txt`: a `harness_<area>_test` executable linked to
`speedgun-ng::harness`, `cxx_std_23`, CTest name equal to the target
name. The threads test scripts internal records (per-context values),
so it carries `target_include_directories(... ${CMAKE_SOURCE_DIR}/source/harness)`
as the calibration tests do.

**Gates that must stay green**: `loop_shape`, `cli_shape`,
`barrier_shape`, `time_source_gate`, `dbc-gate`, `prose-lint`,
`spell-check`, `format-check`, the name check, `ci-sanitize`, `ci-tsan`,
the coverage gates, `example_drift`, `example_drift_fixtures`,
`snippet_compile`, `report_shape`, `report_shape_fixtures`, and
`example_run` over the two new examples.

**Determinism**: the order tests assert single-dispatcher FIFO
directly and two-dispatcher races as set membership plus per-dispatcher
order, which is what FR-013 promises; every wait-mode test subscribes
to the completion state before triggering, never a fixed sleep; the
merge tests are pure arithmetic on scripted leaf values; the tsan runs
cover the inbox, barrier, and disclosure reads.

**Deliberately untouched**: the execution graph (H16), topology-aware
placement (H10), the `--threads` option (H9), long-lived context
pools, JSON and CSV reports, and the raw-data report. Each is a
roadmap spec in the spec Out list, and X.2 keeps it out.

## Constitution Check

*(Post-design pass: does the design as written honor I to XI?)*

| Principle | Verdict and note |
| --- | --- |
| I | PASS with the one recorded exception: the pause-hint site (Complexity Tracking). Every other construct is standard C++23; the wait, the barrier, and the inbox use `std::atomic` operations alone. |
| II | PASS. Each `\pre` and `\post` on the new surface has its enforcement kind at the named call: the lifecycle trio, the cpu index, the early node destruction (enforce semantic), the early barrier departure, and the thread count (D-01, D-03, data-model E-01 to E-05). |
| III | PASS. The logical views carry the contracts, the physical view names the file, namespace, and target of each piece, and TDD mode stands recorded. |
| IV | PASS. The header blocks state each contract once; the two sections teach the surface and the detail page carries the table and the lifecycle rules (FR-029 to FR-033). |
| V | PASS. N-1, N-2, N-4, and N-12 cover every new name as the pre-design row lists; the name check sees no new macro and no unexpanded acronym. |
| VI | PASS. The four executables plus the report-shape extension cover the wait-mode branches, the lifecycle and stopped-dispatch branches, the barrier arms, the merge and skip arms, the disclosure line, and the four warning cases; contract lines stay free per the coverage rule. The capture compiles the coverage preset with `-fprofile-update=atomic`, and T039 names each arc the earlier heads counted at zero beside the test or the shape that reaches it. The row reads PASS when this branch's coverage job prints 100.0% line and 100.0% branch (FR-038). |
| VII | PASS. The timed loop's shape is untouched (`loop_shape` green); dispatch is two atomics; the wait reads one word; everything else runs cold after the stop barrier (R-05, R-06). |
| VIII | PASS. No gate weakens: no marker, no threshold change, no exclusion enters `source/harness/`. The version table records the `SOVERSION` rise FR-043 requires; `ci-tsan` and `ci-sanitize` carry every new path. The coverage gate holds the full 100.0% bar on both rates, and T039 ticks when the job of this branch prints those two numbers. |
| IX | PASS. Every artifact of R-DCUT lives in the feature directory. |
| X | PASS. One new source, one new header, three spec-named seams recorded in Complexity Tracking, and nothing for a use case that does not exist yet (the spec's own words, "Hooks for H16"). |
| XI | PASS. The artifacts pass the prose and spell gates at generation. |

## Complexity Tracking

| Item | Principle | Why it is needed | Simpler alternative rejected |
| --- | --- | --- | --- |
| The pause hint is one `asm volatile("pause" ::: "memory")` site | I (P2 exception) | FR-004 names the mode "spins with a pause hint"; a bare atomic spin starves the sibling hyperthread the test noise scenario (SC-09) relies on. The shipped `barrier.hpp` and the 011 SSC marker set the documented-one-asm-site precedent; the intrinsic headers that declare the same instruction are banned by the counters-only rule. | An empty `asm` compiler barrier alone, rejected because it decodes to nothing and the spin degrades to a full-rate load loop; `__builtin_ia32_pause`, kept as the fallback spelling if a CI compiler rejects the asm form. |
| The four H16 hooks ship in this surface | X.2 | FR-045 requires the completion state, `dispatch()` from inside running work, `ContextId`, and the placement slot with meanings H16 cannot change; the spec names them itself, and this plan invents no seam here. | Shipping without them, rejected by FR-045; giving any of them a second use now, forbidden by the same requirement. |
| Placement is one replaceable function in one harness-level slot | X.2 | FR-005 requires it by name: the replacement is what runs in its place, the function is the seam H10 replaces, and the replaced function is the only way the pinning tests run without privileges (SC-05). | Calling `pthread_setaffinity_np` inline, rejected by FR-005's text; hwloc placement now, rejected as H10's scope (the spec Out list). |
| `threadRunner` accepts a user dispatch function | X.2 | FR-021 requires a custom thread runner, and the harness keeps its barriers and samples around a runner it did not write. | Omitting it, rejected by FR-021; letting a custom runner bypass the barriers, rejected by the same requirement. |
