# Research: Execution Contexts Core

Phase 0 output for `specs/020-execution-contexts-core/spec.md`.
Every entry follows Decision, Rationale, Alternatives considered. The
cited revision is `google/benchmark` at
`e662de9aab8e705ecf4fa4bd41a207e5a0acfd0c` (D-10); every repository
anchor was read at the audit point `64f2ab5`.

The request's input names sit outside this repository and are absent
from this machine: the roadmap document and the research notes name no
local file, and neither exists here. This research derives from the
audit-point repository reads and the pinned Google Benchmark fetch of
D-10, and claims nothing those absent inputs would say (spec
Assumptions).

## D-01: A Vyukov MPSC inbox and three clock-free wait modes

**Decision**: The inbox is an intrusive multi-producer single-consumer
queue in Vyukov's link-swap shape: each node carries one
`std::atomic<WorkNode*>` link, every thread enqueues by swapping its
node into the `tail` and publishing `node->next` after, and the
context thread alone drains a reversed chain (FR-003). Enqueue touches
two atomics and allocates nothing (FR-010, FR-040). The loop reads one
inbox flag: a non-empty inbox takes the oldest node and runs it; an
empty inbox waits by the policy (FR-001). `SPIN` spins on the flag
with a pause hint, one `asm volatile("pause" ::: "memory")` site in a
named helper, the shape `include/speedgun-ng/barrier.hpp` already
ships for the optimization barrier. `YIELD` spins with
`std::this_thread::yield()`. `BLOCK` waits in `std::atomic`'s standard
wait on the flag and `dispatch()` wakes it with `notify_one()`
(FR-004). No wait path reads a clock or takes a timeout, so
`test/time_source_gate.sh` sees no hit and the spin helper names no
header, keeping the banned `<immintrin.h>` family out of the tree.

**Rationale**: The link-swap shape is the one MPSC where enqueue is
two atomic operations on a caller-owned node with no allocation and no
lock, which is exactly what FR-003 and FR-040 require. The per-producer
FIFO it gives is what FR-013's order rule and its test assert. The
pause hint is the only part of the spin that needs something beyond
`std::atomic`: the shipped barrier header establishes that one
documented asm site is the house shape, and a compiler barrier alone
turns the spin into a memory-order busy loop that starves the sibling
core.

**Alternatives considered**: a mutex-and-condition-variable queue,
rejected by FR-003 (the link lives in the node and enqueue allocates
nothing: a lockword per context would hold, but the wake path needs the
CV the `BLOCK` mode already gets from `std::atomic` wait, so the mutex
would buy nothing); a bounded array ring, rejected because a ring with
a MPSC head needs either tags or CAS loops over slots, more machinery
than the linked shape (X.2); `__builtin_ia32_pause`, the GCC and Clang
builtin spelling of the same instruction, kept as the fallback if a
CI compiler rejects the asm operand form; the intrinsic headers
`<immintrin.h>` and friends, forbidden by the counters-only rule's
header list of Additional Constraints.

## D-02: A typed node with one erased run path

**Decision**: `WorkItem` is a concept: `noexcept` invocable with a
`WorkContext&` (FR-008). `WorkNode<Item>` is the caller-owned template:
it holds the item, the inbox link, the completion state
(`pending`, `done`, `stopped`), and one function-pointer slot written
at construction from `Item`, the erased run path the loop calls as
`fn(void*)`-style with the node address (FR-009, FR-011). The loop
calls the stored slot; it runs no run-time type test (D-2).
`dispatch(context, node)` links the node and returns without waiting
(FR-010).

**Rationale**: One virtual-like slot per node gives D-2's single
erased path with no `std::function` per node and no RTTI: the type
erasure happens at the typed construction the concept already
validates, so an unapproved type is a compile error at the
`WorkNode<Item>` or `dispatch()` line, refused before the program
builds (FR-008). The completion state is one atomic word the loop
writes after the erased call returns, which is the shape H16 reads for
continuations (FR-045).

**Alternatives considered**: a `std::function` in the node, rejected
because its non-empty construction allocates and FR-040 forbids
allocation on the dispatch path; a CRTP loop templated on the item
type, rejected because the loop must run items of mixed types through
one path (D-2); `std::any_cast` in the loop, rejected by D-2's ban on
run-time type tests.

## D-03: The barrier is an arrival counter with per-mode waiting

**Decision**: `ContextBarrier(N)` holds one atomic arrival count and
one generation word. An arriving context increments the count; the
arrival that reaches `N` resets the count, advances the generation,
and releases the round, so one round releases all participants at once
(FR-014). A waiting participant waits in the wait mode of its own
policy, on the generation word: spin with pause, yield, or the
standard atomic wait, the same three paths as D-01. `std::barrier`
stands as the recorded fallback for the `BLOCK` path (FR-014, Q-3).
The barrier reads no clock, so no round times out. A participant that
leaves a round early trips the contract of FR-015.

**Rationale**: One counter and one generation is the whole state, and
waiting on that generation in the arrival's own mode is what keeps a
spinning context out of the kernel while it waits (Q-3). Because the
round's release is the generation flip, all three modes watch the same
word; `std::barrier` can replace the `BLOCK` branch without touching
the spin branches, which is the separation the fallback record needs.

**Alternatives considered**: `std::barrier` alone, rejected because a
spinning context would sleep inside its wait, exactly the case Q-3
rules out; a sense-reversing counter with a per-round flag array,
rejected as more state than one generation word for the same release;
a timed wait for a stuck participant, forbidden by FR-014's no-clock
rule.

## D-04: The placement seam is one function pointer defaulting to a cpu pin

**Decision**: The policy's placement slot is `NO_PIN` or one cpu
index. A context applies its slot once, at thread start, by calling
one replaceable placement function: a `std::function` (or function
pointer) the context holds, defaulting to a small Linux pin
implementation (`pthread_setaffinity_np` on the started thread with a
one-cpu set). A test replaces the function at context creation and
records the calls, so the pinning tests need no privileges (FR-005,
Assumptions). The signature is the seam H10 later replaces with
topology placement; nothing else reads it (SC-05).

**Rationale**: One call, once, at thread start, keeps placement out of
the loop and out of `dispatch()` entirely. The default calls the
platform only through one named function, so the H10 swap touches one
line per construction site, and the FR-005 test ("the replacement is
what ran, once per thread") is an injected function and a counter.

**Alternatives considered**: calling `sched_setaffinity` inline in the
loop's thread start, rejected by FR-005's one replaceable function;
hwloc-based placement now, rejected: H10 owns topology placement (the
spec Out list); `std::thread::native_handle` juggling exposed to the
caller, rejected by D-1's ownership (the library owns the thread).

## D-05: A threaded attempt runs N contexts with the barriers outside every window

**Decision**: For an instance whose thread count stands above one, the
runner builds one threaded attempt shaped as follows, per attempt and
beside the existing single-context path (FR-022 keeps that path's
bytes).

```text
create contexts C[0..N-1] with policy {SPIN, NO_PIN}   (FR-004, Q-2)
fixture object + setUp run once on context 0            (FR-023)
ContextBarrier start(N) and stop(N)                     (FR-014)
per context i, as one caller-erased body node:
  open plan and recorder on this context's thread       (FR-018)
  start.arrive-and-wait          outside the window     (FR-019)
  open window; timed loop; close window                 (unchanged cursor)
  stop.arrive-and-wait           outside the window     (FR-019)
  fold this context's own plan and publish per-context values
tearDown on context 0                                   (FR-023)
requestStop, drain, join every context                  (FR-006)
merge the N per-context values into the row             (D-06)
```

Every context opens its own thread-target plan on its own thread, so
the plan, its recorder, and its fold all live on one thread, the
precondition `machine/tsc` states (PC-1, FR-018). The calibrated
iteration count is the batch's: every context runs the same body with
the same count (Assumptions), and calibration reads the merged
decision value as today. The default `SPIN` policy keeps a measured
context from sleeping inside the attempt (Q-2). The fixture rule
differs from the cited revision by design; D-07 and D-10 record it.

**Rationale**: The shape reuses every cold step the single-context
path already takes (plan open, recorder reservation, window, fold) and
adds only the contexts, the two barrier rounds, and the merge. The
barriers sit at the body node's first and last step, so no barrier
wait can land inside a folded window (FR-019, SC-08). One thread
creation per context per attempt sits outside every window; thread
start costs nothing the row measures.

**Alternatives considered**: contexts that live for the whole
instance run across attempts, rejected for now: per-attempt contexts
keep lifecycle trivial (create, join, destroy) and thread creation is
cold, an upgrade to long-lived contexts belongs to H16's graph, where
reuse has a caller; pinned contexts for threaded runs, rejected: the
policy's default slot is `NO_PIN` (FR-005) and oversubscription is the
report's warning case (FR-026); running thread 0's body on the runner
thread, the cited `ThreadRunnerDefault::RunThreads` shape, rejected:
the runner thread must stay free to hold the barriers' caller side and
wait on the completion states (FR-021 keeps dispatch the only entry).

## D-06: The merged row and the flag pass with the real N

**Decision**: The merge is cold code after the stop barrier and builds
the published row from the per-context values in one way (FR-020):

```text
real       = max over contexts            (the slowest context)
cpu        = sum over contexts
iterations = per-context iterations * N
counters   = per name, sum over contexts, before the flag pass
flag pass (cited order, internal::Finish of src/counter.cc):
  RATE                 v /= (primaryNs / 1e9) / N
  AVG_THREADS          v /= N
  ITERATION_INVARIANT  v *= iterations        (the merged total)
  AVG_ITERATIONS       v /= iterations
  INVERT               v = 1 / v              (last, always)
```

The merged `iterations` equals the cited `results.iterations`, the
total across the thread group, and the rate divisor repeats the cited
`CreateRunReport`'s `seconds / b.threads()` (D-10). The single-thread
pass of H4 stands: with `N == 1` the formulas reduce to H4's
(`v /= N` by one, the divided rate by itself). The flag pass runs once
per merged row inside the existing `appendCounterColumns`, which gains
the thread-count divisor H4 left at one (PC-2, FR-020). When any
context's body skipped, the merged row carries the skip outcome and
reason, and the instance publishes no partial row (Edge Cases).

**Rationale**: The four aggregation rules are FR-020's list, and the
flag-pass divisors are what makes SC-07's "matches the cited `Finish`
result for the real `N`" true: the cited call divides rate by
thread-seconds (`seconds / threads`) and `kAvgThreads` by the thread
count, read at `src/benchmark_runner.cc` and `src/counter.cc`.
Summing user counters before the pass matches the cited
`internal::Increment`, which adds values as threads finish.

**Alternatives considered**: dividing rate by the merged decision time
alone (no `/ N`), rejected: it would miss the cited result the
scenario pins; merging counters after the flag pass, rejected because
the per-context pass would divide each context by `N` that the sum
should take once (the cited order merges, then finishes); max-ing cpu
time, rejected by FR-020's own words (the cited merge sums cpu).

## D-07: One fixture object per threaded run through the existing macro slot

**Decision**: The fixture macros keep one slot per macro site: the
`sgCurrentCase` static pointer. A threaded run writes the slot once,
before any body node is dispatched, and every context's registered
callable reads it after that write; `dispatch()` is the release-edge,
so the reads are data-race free (FR-041). One object serves the run
because FR-023 runs `setUp` and `tearDown` once per run, on context 0:
one object, one setup pair, N body readers. The stale comment beside
the macro machinery in `include/speedgun-ng/benchmark.hpp` ("the
threads spec of the roadmap moves the slot when that spec lands") is
replaced with the stated rule in this change.

**Rationale**: A thread-local slot would hide the object from the very
threads meant to read it, and a per-context object would contradict
FR-023's one pair per run. The write-once-then-read shape is the same
edge the runner already relies on when it writes instance data before
starting work. ThreadSanitizer sees a write ordered before every read.

**Alternatives considered**: a `thread_local` slot the runner copies
into each context, rejected: the object is one and shared by design,
and per-thread copies would give each body a different state to mutate
with no rule for merging; per-context fixture objects, rejected by
FR-023. The difference the cited revision keeps (its `Fixture::Run`
calls `SetUp` on every thread's `Run` entry, `include/benchmark/benchmark_api.h`)
stands recorded in D-10's difference list.

## D-08: The disclosure rides the result and the warning reads one host fact

**Decision**: `BenchmarkResult` gains one vector of disclosure
records, `{waitMode, placement}`, one per context of a threaded run,
empty for a single-context run. `printResult` prints, after the rows
of a result whose vector is non-empty, one disclosure line naming the
count and each context's mode and placement, and keeps every byte of a
single-context result as it prints today (FR-025, Assumptions). The
noise warning computes from that vector plus the online cpu count:
when two spinning contexts of the report name one cpu index, or the
spinning count of the report exceeds the online count, `printResult`
prints one warning line and otherwise prints none (FR-026, FR-028).
The online cpu count comes from the counters library's `System` host
topology (`sg::counters::System::local()`, the object the runner
already holds), read once per report; a topology count is a host fact
and the disclosure and warning read no clock (FR-027, FR-039).

**Rationale**: The report consumes `BenchmarkResult` values and adds
nothing to them, so the policy data rides the result and the printer
stays a formatter (the standing H1 separation). Computing the warning
from the disclosed policies means the warning follows the same values
the reader sees. `System` is already reachable from the harness
(`Runner::run` compiles plans against it) and works on the fake
provider's tests because topology discovery is independent of the
measurement provider.

**Alternatives considered**: recomputing affinity from the OS at print
time (`sched_getaffinity`), rejected: the disclosure is about the
policies the run asked for, the warning's edge cases (two pinned
spinners on one cpu) come out of the policy vector, and one host read
stays cheaper than one syscall per context; reading `/sys` or
`/proc`, rejected by library-first and by the counters dependency list;
reading the cpu count from hwloc directly, rejected: the harness goes
through `sg::counters`, the standing dependency shape.

## D-09: Thread-count selection extends the family expansion

**Decision**: `threads(n)` fixes one count. `threadRange(min, max)`
selects the counts `min, min * 2, ...` up to `max` and
`denseThreadRange(min, max, step)` selects `min, min + step, ...` up
to `max`, mirroring the cited `ThreadRange` (multiplier 2) and
`DenseThreadRange` (stride) of `include/benchmark/benchmark_api.h`.
The registry stores a `std::vector<int> threadCounts` (default `{1}`)
on the family record; instance expansion runs over the argument set
crossed with the thread counts, and `instanceName` (`source/harness/family.cpp`)
appends the `/threads:N` segment for a count above one, in the shape
H2's `/`-joined naming already builds (FR-016). A count below one trips
the standing setter precondition plus the FR-024 `SG_REQUIRE` at the
call.

**Rationale**: The count is one more dimension of the family the H2
expansion already crosses, so a range stays "one instance per count"
the way the Assumptions state, and the name suffix is one more
`/`-joined segment at the one place names compose. The default `{1}`
keeps every untouched family byte-identical (FR-022).

**Alternatives considered**: expanding thread counts into separate
families, rejected: the threads segment is part of the instance name
and the family stays one; `ThreadPerCpu`, the cited convenience call,
rejected: the spec names three calls (FR-016) and X.2 keeps the fourth
out.

## D-10: The Google Benchmark read: pinned revision, semantics, differences

**Decision**: The pinned revision is `google/benchmark` at
`e662de9aab8e705ecf4fa4bd41a207e5a0acfd0c`, the head of `main` that
019 pinned for its reads, verified by fetch on 2026-10-15 (commit
"Add `State::in_timed_section()` helper (#2319)", 2026-10-08). The
three semantics the spec's reference-read table names were read there:

| Semantic | File and function at the pinned revision |
| --- | --- |
| One runner per instance over N threads, thread count and index visible to the body | `src/thread_manager.h`, `internal::ThreadManager`: constructed with `num_threads`, holds the `start_stop_barrier_`, the mutex-guarded merged `Result`, and `NotifyThreadComplete`; `src/benchmark_runner.cc`, `RunInThread`: runs `b->Run(iters, thread_id, ...)` and adds the thread's numbers into `manager->results` |
| Per-run runner starts the threads, holds the start signal outside the timed region, joins, and hands per-thread results to aggregation | `src/benchmark_runner.cc`, `ThreadRunnerDefault::RunThreads` (declared against `ThreadRunnerBase` of `include/benchmark/benchmark_api.h`): starts `N - 1` `std::thread`s, runs index 0 inline, joins all; `BenchmarkRunner::DoNIterations`: builds the `ThreadManager`, runs the group, takes the merged results under the lock, and hands them to `CreateRunReport`; the start signal is the manager's `StartStopBarrier` (`src/thread_manager.h`), waited before each thread's State opens its timed section |
| The counter flag pass divides `AVG_THREADS` by the real thread count after the per-thread merge | `src/counter.cc`, `internal::Finish`: rate over `cpu_time`, `kAvgThreads` over `num_threads`, invariant times `iterations`, average over `iterations`, invert last; `src/benchmark_runner.cc`, `CreateRunReport`: passes `thread_seconds = seconds / b.threads()` and `b.threads()`, after `internal::Increment` merged the per-thread counters (`RunInThread`) |

The difference list FR-046 opens with, each beside its semantic:

- Fixture rule. The cited `benchmark::Fixture::Run`
  (`include/benchmark/benchmark_api.h`) calls `SetUp`, the case, and
  `TearDown` inside every thread's `Run` entry, and the
  `Benchmark::Setup`/`Teardown` callbacks run once per repetition on
  the calling thread around `DoNIterations`. This feature runs
  `setUp` and `tearDown` once per run on context 0, outside both
  barriers (FR-023, Q-4). D-07 carries the object-slot rule.
- Barrier mechanism. The cited `Barrier` (`src/mutex.h`) is a
  condition-variable rendezvous; a thread inside it sleeps. This
  feature's `ContextBarrier` waits in the arriving context's own wait
  mode on an atomic counter and generation (FR-014, Q-3, D-03).
- Thread bodies. The cited default runner executes index 0's body on
  the calling thread and creates `N - 1` raw threads; this feature
  dispatches all N bodies onto library-owned contexts and gives the
  runner thread no body (D-05, FR-021).

**Rationale**: FR-046 requires the plan to pin what it read and to
record each kept difference beside its semantic. Reading the four
files directly (the `src/thread_manager.h`, `src/benchmark_runner.cc`,
`src/counter.cc`, `include/benchmark/benchmark_api.h` blobs fetched at
the full hash) gives the citations the spec's table asks for in the
form `specs/019-harness-user-counters-and-statistics/spec.md` shows.
Staying on the revision 019 read keeps both features' citations
comparable.

**Alternatives considered**: pinning the current `main` tip at this
plan's run, rejected: an unread hash is a fabricated citation and a
moving `main` breaks cross-spec comparability; reading a vendored
copy, impossible: the spec admits none and this feature links nothing
from Google Benchmark.
