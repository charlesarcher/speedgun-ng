# Data Model: Execution Contexts Core

Phase 1 output for `specs/020-execution-contexts-core/spec.md`. The
entities are the Key Entities of the spec, plus the records the run and
the report move between them. Every anchor is the audit-point line of
`64f2ab5`, and each `+` marked field is new in this spec.

## E-01: Execution context

`ExecutionContext` owns one thread, its event loop, its inbox, and its
policy (`include/speedgun-ng/execution_context.hpp`, new; FR-001 to
FR-007, research D-01).

| Field | Type | Source |
| --- | --- | --- |
| `ContextId` | the context's identity, one value per live context; `currentContext()` reports it on that thread and reports no context on a thread the library owns no context for | + (FR-002) |
| inbox | intrusive MPSC link-swap queue head: an atomic `tail` and a private drain `head`, the link living in each node, enqueue allocating nothing | + (FR-003, D-01) |
| policy | the E-02 value, fixed at construction and readable | + (FR-007) |
| thread | the one `std::thread` running the loop | + (FR-001) |
| stop request | the flag E-03's stop token reads and the loop consults between nodes | + (FR-006, FR-011) |

Relationships: `dispatch()` (E-03) links nodes into the inbox; the loop
takes the oldest node, runs its erased path with the E-03 work context,
marks it `RUNNING` then `DONE`, and looks again; the barrier (E-04)
waits in the policy's mode. `currentContext()` reads the thread-current
context id the loop installs for the node's run and clears after.

Validation and lifecycle (FR-006, FR-050): the constructor starts the
thread and applies the placement once through the installed placement
function (FR-005). The legal sequence is `requestStop()`, `drain()`,
`join()`: a node whose dispatch observed the not-stopped state runs
during `drain()`; a node whose dispatch observed the stop request is
completed `STOPPED` by its dispatcher and never runs; no node runs
after `join()` returns. `drain()` blocks the caller until the inbox is
empty and the loop is idle, and without a prior `requestStop()` it is a
flush that leaves the loop running. The destructor requires a joined
context through `SG_REQUIRE`; `join()` from the context's own thread is
an `SG_REQUIRE` violation and `drain()` from it is a debug-precondition
violation (FR-050, Edge Cases).

State transitions: `running`, to `stop-requested` (`requestStop()`),
to `drained` (`drain()` completes: every queued node ran exactly once),
to `joined` (`join()` returns). The destructor stands only on
`joined`.

## E-02: Context policy

`ContextPolicy` is the two choices made at creation, fixed for the
context's life (FR-005, FR-007; Assumptions).

| Field | Type / values | Default | Source |
| --- | --- | --- | --- |
| `waitMode` | `WaitMode::SPIN` (spin with the pause hint), `YIELD` (yield to the scheduler), `BLOCK` (standard atomic wait on the inbox flag) | `SPIN` for a context the harness creates to run a benchmark; `BLOCK` for a caller-created context with no policy | + (FR-004, Q-2) |
| `placement` | `NO_PIN`, or one cpu index | `NO_PIN` | + (FR-005) |

Relationships: the report discloses both fields (E-07); the barrier
(E-04) waits in `waitMode`; the placement applies once at thread start
through the installed `PlacementFn`, whose signature is the seam H10
replaces (FR-005, SC-05, research D-04).

Validation: a cpu index the host does not have is an `SG_REQUIRE`
violation at the call (FR-005, Edge Cases). No wait mode reads a clock
or takes a timeout (FR-004).

## E-03: Work item, work node, work context

The typed work surface (FR-008 to FR-013, D-02).

| Piece | Shape | Source |
| --- | --- | --- |
| `WorkItem` | concept: `noexcept` invocable with `WorkContext&`; an unapproved type cannot reach `dispatch()` and is refused before the program builds | + (FR-008) |
| `WorkNode<Item>` | caller-owned carrier: the item, the inbox link, one erased run path written at construction from `Item`, and the completion state, read through a `noexcept` `state()` accessor with acquire order | + (FR-009, FR-049, contracts/execution-context.md) |
| completion state | `PENDING`, `RUNNING` (the loop has taken the node), then `DONE` (the item ran) or `STOPPED` (a dispatch that observed the stop request refused the node and its dispatcher completed it); one atomic word, release-written by the completing thread as its last access to the node (FR-049); H16 reads it for continuations, its meaning and readability fixed (FR-045) | + (FR-009, FR-049, FR-050) |
| `dispatch(context, node)` | links the node into that context's inbox and returns without waiting; allocates nothing; legal from any thread and from work already running on that context | + (FR-010) |
| `WorkContext` | what a running item receives: the id of the context running it and a `StopToken` reading true once `requestStop()` stood | + (FR-011, contracts/execution-context.md) |

Validation: the caller keeps a node alive until its state reads `DONE`
or `STOPPED`; destroying a live node aborts with an `SG_REQUIRE`
violation under the enforce semantic, and once a reader observes
completion the caller may destroy or reuse the node (FR-012, FR-049,
Edge Cases). A dispatch that observes the stopped state refuses its
node and completes it `STOPPED`; the loop never touches a node it never
accepted (FR-050).

Order (FR-013): nodes one thread dispatches run in that order; nodes
two threads dispatch at once run in the order the inbox accepted them.
Work dispatched from inside a running item runs after that item ends,
on the same context (Edge Cases).

## E-04: Context barrier

`ContextBarrier` joins a fixed number of contexts in rounds
(FR-014, FR-015, D-03).

| Field | Type | Source |
| --- | --- | --- |
| arrivals | atomic count of participants that reached the round | + (FR-014) |
| generation | atomic word; the arrival that completes the expected count resets `arrivals` and advances it, releasing the round | + (D-03) |
| expected | the participant count fixed at construction | + (FR-014) |

Relationships: legal from inside running work and outside it (FR-015).
A waiting participant waits in the wait mode of its own policy on the
generation word, so a spinning context never sleeps inside the barrier;
a participant that runs on no context waits in `BLOCK` (FR-014).
`std::barrier` stands as the fallback for the `BLOCK` path (Q-3). A
threaded run uses two barrier objects, one for its start round and one
for its stop round (E-08).

Validation: the barrier reads no clock, so no round times out
(FR-014). A participant that leaves a round early is a contract
violation (FR-015). A zero participant count at construction trips the FR-014
participant-count contract at the call.

## E-05: Threaded instance and the extended state

One benchmark instance paired with one context count (Key Entities).

| Piece | Change | Source |
| --- | --- | --- |
| `BenchmarkHandle::threads(n)` / `threadRange(min, max)` / `denseThreadRange(min, max, step)` | store the count set `{n}` / `{min, min*2, ...}` / `{min, min+step, ...}` on the family record | + (FR-016, D-09) |
| `RegistryEntry::threadCounts` | `std::vector<int>`, default `{1}` (`source/harness/detail/internal.hpp`) | + (D-09) |
| `RegistryEntry::threadRunner` | optional `ThreadRunnerFn` taking the `ThreadRun` surface of one attempt (E-08, contracts/threaded-runs.md) | + (FR-021) |
| instance expansion | arguments crossed with thread counts; `instanceName` appends the `/threads:N` segment for a count above one (`source/harness/family.cpp`) | + (FR-016) |
| `State::threadIndex()` | the index of the context running the body, `0` to `N - 1`; `0` for a single-context run | + (FR-017) |
| `State::threads()` | the count `N` of the run; `1` for a single-context run | + (FR-017) |

Validation: a thread count a call cannot honor, zero included, is an
`SG_REQUIRE` violation at that call (FR-024), beside the standing
setter precondition (`runStarted`) the other handle calls carry. A
benchmark that calls no thread method keeps today's single-context
behavior and row bytes (FR-022).

## E-06: Merged row

The one published row of a threaded instance (FR-018 to FR-020, D-05,
D-06). Each per-context value comes from that context's own
thread-target plan: the plan, its recorder, and its fold live on that
context's thread and no fold reads another context's plan (FR-018,
PC-1).

| Field of the published `ResultRow` | Rule | Source |
| --- | --- | --- |
| real time | the summed manual time under manual timing, else the summed real time over the contexts | FR-020 |
| cpu time | the sum over the contexts | FR-020 |
| iterations | the sum of the per-context counts (the per-context count times `N` when the bodies ran equal counts) | FR-020 |
| counters | per name, summed over the contexts before the flag pass | FR-020 |
| flag pass | the cited order with the real `N`: rate over the timing mode's summed decision seconds divided by `N` (cpu by default, manual under manual timing, real under real-time timing), `AVG_THREADS` divided by `N`, invariant times the merged iteration total, average over it, invert last | FR-020, D-06 (cited `internal::Finish` of `src/counter.cc`; `RunInThread`, `DoNIterations`, and `CreateRunReport` of `src/benchmark_runner.cc`) |
| skip | when any context's body skipped, the row carries the skip outcome and reason and the instance publishes no partial row | Edge Cases |

Relationships: the merge runs after the stop barrier, cold code; the
per-context times and iterations are summed, as the cited `RunInThread`
sums each thread's numbers into one merged `Result`. The printed
per-iteration time is the summed time divided by the summed iterations,
the cited `GetAdjustedRealTime` and `GetAdjustedCPUTime` division of
`src/reporter.cc`, so the per-context average is implicit in the summed
iterations. The flag pass runs inside the existing
`appendCounterColumns`, which gains the thread divisor H4 left at one
(PC-2). The merged row feeds the aggregation, statistics, fit, and
report paths unchanged.

## E-07: Policy disclosure and noise warning

Report-side records (FR-025 to FR-028, D-08).

| Piece | Shape | Source |
| --- | --- | --- |
| disclosure record | `{waitMode, placement}` per context | + (FR-025) |
| `BenchmarkResult::contexts` | vector of disclosure records, one per context of a threaded run, empty for a single-context run | + (FR-025) |
| disclosure line | printed after the rows of a result whose vector is non-empty: the count and each context's mode and placement (`NO_PIN` or one cpu index) | + (FR-025) |
| noise warning | one line after a result, printed when two spinning contexts of that result name one cpu index or that result has more spinning contexts than the online cpu count, and absent when neither condition holds | + (FR-026, FR-028) |
| online cpu count | one host fact read once per report through `sg::counters::System::local()` | + (FR-027, Assumptions, D-08) |

Validation: neither the disclosure nor the warning reads a clock or
adds a sampling action (FR-027); a single-context result keeps the
bytes it prints today (FR-025, Assumptions).

## E-08: Threaded run mechanics (internal)

The runner-side records of one threaded attempt (D-05).

| Piece | Shape | Source |
| --- | --- | --- |
| contexts | `N` contexts with policy `{SPIN, NO_PIN}`, created once when the instance's threaded run starts, reused for every calibration attempt and repetition, and joined when the instance finishes, thread start and join outside every window | FR-004, D-05 |
| body node | one erased work item per context index: plan open, start-barrier arrive, window open, loop, window close, stop-barrier arrive, fold own plan | FR-018, FR-019 |
| barriers | two `ContextBarrier(N)` objects, start and stop | FR-014, FR-019 |
| fixture slot | the macro site's static pointer, written once before dispatch, read by every context after that edge | FR-023, D-07 |
| completion wait | the caller waits on the `N` completion states, reading them with acquire order | FR-009, FR-010, FR-049 |
| custom runner | the E-05 `RegistryEntry::threadRunner` callable, when set, performs the dispatches through `ThreadRun`, one per body index; the harness builds the body nodes and keeps them alive across the call; the barriers and the merge stand around it unchanged | FR-021 |

Relationships: `setUp` and `tearDown` run once per run on context 0,
outside both barriers (FR-023); every difference from the cited
revision stands in research D-10's list (FR-046).

Consumers: the merged row (E-06) reads the per-context values; the
disclosure (E-07) reads the policies of the contexts the run made; a
custom runner that returns with a body undispatched or dispatched
twice trips the contract at that point (FR-021, Edge Cases).

## E-09: Existing entities extended, identity kept

| Entity | Change | Source |
| --- | --- | --- |
| `State` | gains `threadIndex()` and `threads()` reads over two fields defaulting to the single-context values | FR-017 |
| `BenchmarkHandle` | gains the three thread calls and the thread-runner attach, all behind the standing setter precondition | FR-016, FR-021 |
| `RegistryEntry` | gains `threadCounts` and `threadRunner` | D-09 |
| `BenchmarkResult` | gains `contexts` | FR-025 |
| `ResultRow` | no field change: the merged row is an ordinary row | E-06 |
| counters `Target`, `Plan`, `Recorder`, `FakeProvider` | unchanged; each threaded context consumes them per thread | PC-1, Dependencies |
