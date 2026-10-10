# Research: Harness Timing Modes

Phase 0 output for `specs/017-harness-timing-modes/spec.md`. The spec
carries no open clarification: its Clarifications session settled the
column set (Q-1), the primary time (Q-2), the pause contract (Q-3), the
metric folds (Q-4), the unit scope (Q-5), and the breach reporting (Q-6).
The open questions that remain belong to the plan, and each one below is
settled here with the reference revision `google/benchmark` `main` at
`e662de9a` read beside the audit-point code.

## D-01: Two stored columns, one primary time

**Decision**: `ResultRow` keeps `timePerIterationNs` as the real-time
column and gains `cpuTimePerIterationNs` beside it. A third value, the
primary time, is a selection the runner makes per benchmark over the real
fold, the CPU fold, and the manual sum, and it drives the growth rule,
the qualification tests, and the aggregate `timeStats`.

**Rationale**: FR-018 states the new fields as additions and forbids
replacing an existing signature, and `timePerIterationNs` is a public
field of a public struct in `include/speedgun-ng/benchmark.hpp:114`.
FR-002 asks for one primary time that drives the growth rule, the
qualification tests, and the aggregate statistics, which is a selection
rule, a stored value. Keeping the selection in the runner leaves one
statistic set, the one FR-002 names.

**Alternatives considered**: renaming the field to
`realTimePerIterationNs`, rejected because FR-018 forbids replacing an
existing signature and the rename reaches every consumer of the public
struct; storing three `MetricValue` entries for the times, rejected
because it drops the named fields FR-018 states and re-keys the
aggregate builder for no rule that needs it.

## D-02: The CPU leaf is one leaf, chosen at plan build

**Decision**: the run plan carries exactly one CPU core:
`machine/thread_cpu` by default, `machine/process_cpu` when
`measureProcessCpuTime` stands on the family record. The choice is made
once in `Runner::run` beside the existing `machine/monotonic` resolution
at `source/harness/runner.cpp:176-187`, and a leaf that does not resolve
fails that benchmark with the existing resolution line.

**Rationale**: FR-004 states a switch between two leaves, and the
audit-point code already holds the resolution-and-failure path for one
leaf. The cited revision reads one `CpuTimer` per thread and sums the
threads; the single-threaded rule of the spec leaves one reading.

**Alternatives considered**: carrying both leaves and reporting both,
rejected because FR-001 states two columns and FR-004 states a switch;
resolving the leaf per run, rejected because the plan is built once per
benchmark and a per-run resolve repeats a catalog lookup inside the run
loop.

## D-03: The four handle calls and their names

**Decision**: `BenchmarkHandle` gains `measureProcessCpuTime()`,
`useRealTime()`, `useManualTime()`, and `unit(TimeUnit)`, each returning
`BenchmarkHandle&` and each standing behind the same
`m_entry != nullptr && !m_entry->runStarted` `SG_REQUIRE` the existing
setters use (`source/harness/registry.cpp:59` and its siblings).

**Rationale**: the setters of H1 and H2 already establish the shape, the
precondition, and the chaining rule, so the new calls add one field write
each. V.1 N-2 asks for camelBack, and N-1 spells an acronym as one word
inside a name, which gives `measureProcessCpuTime` for the cited
revision's `MeasureProcessCPUTime`.

**Alternatives considered**: one call taking a mode enumerator, rejected
because FR-004, FR-005, and FR-006 state three independent selections and
the FR-008 exclusion is clearer as two calls that cannot both stand;
keeping the cited revision's `Unit` spelling, rejected under N-2.

## D-04: The FR-008 exclusion is a precondition on both calls

**Decision**: `useRealTime` carries
`SG_REQUIRE(!m_entry->manualTime, ...)` and `useManualTime` carries
`SG_REQUIRE(!m_entry->realTime, ...)`, so the second of the two calls is
the violation, whichever order the source uses.

**Rationale**: Q-3 and FR-011 make a pause-state breach a precondition
violation, and Q-6 makes every contract breach report like the others.
The exclusion is a call precondition, so the check belongs on the call
that breaks it, and both calls need a check for the two orders to behave
alike.

**Alternatives considered**: one check in `Runner::run` before the first
run, rejected because it reports a call-site mistake at run time with no
call site in the message; a runtime error row, rejected by Q-6, which
keeps contract breaches out of the recoverable error path.

## D-05: Manual time is an accumulator on the state

**Decision**: `State::setIterationTime(std::int64_t nanoseconds)` adds to
`m_manualNs`, and the runner reads the sum after the run. The reported
real column shows the sum divided by the iteration count, and the
decision value shows the sum itself. `RunRecord` gains a `manual` flag
that exempts the run from the fivefold stop in `qualifies`
(`source/harness/detail/calibration.hpp:56-61`).

**Rationale**: FR-007 states summation, and the cited revision's
`SetIterationTime` (`include/benchmark/state.h:84`) adds into
`manual_time` with the same one-value-per-iteration rule. A nanosecond
integer keeps the exact arithmetic the constitution asks for, where the
cited revision's `double` argument would round. The fivefold exemption
needs one flag because the fivefold test at
`source/harness/detail/calibration.hpp:59-71` compares a clock fold
against the target, and a manual run has no clock fold to compare.

**Alternatives considered**: a `double` argument matching the cited
revision, rejected under the P0 exactness habit and because the harness
states nanoseconds everywhere else; a per-iteration count check that
rejects a second call in one iteration, rejected under X.2 because the
cited revision sums freely and no requirement states a count rule.

## D-06: A manual run keeps the plan and its samples

**Decision**: a manual benchmark still opens a window, still takes the
entry and exit samples, and still carries the monotonic core in its plan.
The reported real column shows the manual sum.

**Rationale**: FR-003 states one sample pair for a run with no pause in
every mode, and FR-017 keeps the loop discipline of H1. One plan shape
keeps one capacity rule, one sample count, and one fold helper in
`runner.cpp`. The CPU column of a manual benchmark still needs its leaf,
so the plan cannot drop a core.

**Alternatives considered**: a plan without the monotonic core for manual
benchmarks, rejected in Complexity Tracking of `plan.md`; skipping
`begin()` for a manual benchmark, rejected because it splits the loop
discipline of FR-017 into two paths and leaves the printed sample count
without a definition.

## D-07: Pause pairs are sample ordinals inside the run

**Decision**: `State` keeps `m_samplesTaken`, `m_pauseDepth`, and
`m_pausePairs` as pairs of zero-based sample ordinals inside the run. The
runner keeps a point cursor in place of the pair counter at
`source/harness/runner.cpp:311`, and a fold over a run reads
`base` to `base + samples - 1`, with each pause pair folded at
`base + p0` to `base + p1`.

**Rationale**: the recorder appends one point per `sample()` call
(`include/speedgun-ng/counters_measurement.hpp:504`), so a run with
pauses occupies more than two points, and the existing `2 * pair`
arithmetic at `source/harness/runner.cpp:118` and `:330` can no longer
name the window. Ordinals inside the run keep `State` free of the
recorder layout, and the runner adds the base when it folds. With no pause the rule
reduces to the H1 two-point fold, which is what FR-003 and SC-004 require.

**Alternatives considered**: absolute point indices in `State`, rejected
because the state would have to know the recorder layout the runner owns;
a second recorder per paused interval, rejected for a second allocation
per run and a fold that could no longer state the window.

## D-08: One netted fold serves every column

**Decision**: one file-local helper in `runner.cpp` computes
`fold(window) − Σ fold(pause interval)` for a core, and the real core,
the CPU core, and each metric core go through it. The helper reports the
availability of the worst fold it reads.

**Rationale**: FR-010 states the subtraction for the two time columns and
for attached metrics, and Q-4 confirms attached metrics exclude paused
work. One helper keeps one rule, and the availability of the worst fold
is what the gap edge case needs.

**Alternatives considered**: subtracting the paused intervals from the
point columns before folding, rejected because it would rewrite recorder
points the recorder owns and break the repeatability of a fold; a
per-column copy of the arithmetic, rejected under X.1 because three copies
of one rule is where a rule drifts.

## D-09: The per-run pause allowance is 8

**Decision**: `kMaxPausePerRun = 8` in namespace `sg` in
`include/speedgun-ng/benchmark.hpp`, beside `kDefaultRangeMultiplier` at
line 42. The recorder capacity becomes
`2 * (1 + kMaxPausePerRun) * (kRunBound + kRunBound + repetitions)`
points, and `pauseTiming` carries
`SG_REQUIRE(m_pauseCount < kMaxPausePerRun, ...)` before it samples.

**Rationale**: FR-014 requires the capacity rule to cover the pause pairs
and requires the breach to be named before the run overruns the recorder.
The recorder is a hard-stop handle (`counters_measurement.hpp:507`), so an
uncovered overrun loses points silently. The H1 rule sizes
`2 * 193 = 386` points per leaf; the allowance sizes 3474, which at eight
bytes per point and four leaves is about 110 KB per benchmark, allocated
once per benchmark. Eight intervals covers the cited revision's use, where
a benchmark pauses around one setup and one teardown region per run. The
constant is public because the guard in the header and the capacity rule
in `runner.cpp` read one value, the pattern `kDefaultRangeMultiplier`
already sets.

**Alternatives considered**: an allowance of 2, rejected because a
benchmark with a paused setup, a paused teardown, and a paused I/O region
would abort on its third interval; growing the recorder when a pause
exceeds the allowance, rejected for an allocation on a run path and a
second capacity rule; an unbounded allowance with a runtime error row,
rejected by FR-014, which names a precondition violation.

## D-10: The open-pause-at-exit check stands in `closeWindow`

**Decision**: `State::closeWindow()` (`benchmark.hpp:448`), the
`[[gnu::cold]]` path the cursor calls when the count reaches zero, carries
`SG_ASSERT(m_pauseDepth == 0, ...)`. `pauseTiming` and `resumeTiming`
carry their own `SG_REQUIRE` pair for the double pause and the resume with
no pause.

**Rationale**: FR-011 names loop exit as the moment the rule is broken,
and `closeWindow` is the only path that leaves the loop normally. It
stands outside the shortest backward jump the loop-shape gate measures, so
the check costs nothing per iteration (R-06). `SG_ASSERT` fits an
invariant of the run at that point, where the other two are call
preconditions.

**Alternatives considered**: a check in `sampleRun` after the loop,
rejected because the recorder points of the run would already be folded
and the abort would arrive after the arithmetic; a check in `operator++`,
rejected because it puts work inside the timed loop and breaks the
loop-shape gate.

## D-11: An exception path closes an open pause

**Decision**: `State::topUpWindow()` (`benchmark.hpp:456`), the path the
runner takes when the callable throws or skips, samples once to close an
open pause pair and then takes the exit sample.

**Rationale**: the existing path guarantees the window holds its exit
sample, the rule its comment at `benchmark.hpp:454-455` states, and the fold arithmetic of D-07 needs
every recorded pause pair to hold two points. The run is already doomed on
that path, so the extra sample buys a well-formed record at the cost of
one read.

**Alternatives considered**: dropping the open interval on that path,
rejected because the pause list would carry an incomplete pair the helper
would have to special-case; leaving the pair open, rejected because the
fold precondition `i < j` would fail inside the error path.

## D-12: The unit is captured at registration and scaled in the report

**Decision**: `TimeUnit` lives in `include/speedgun-ng/benchmark.hpp`.
`RegistryEntry` gains `timeUnit`, written when the entry is created from
the process default that `setDefaultTimeUnit` sets and
`getDefaultTimeUnit` reads. `BenchmarkHandle::unit` overwrites the entry's
value. `BenchmarkResult` gains `timeUnit`, the runner copies it from the
entry, and `report.cpp` scales the two time columns, the overhead floor,
and the aggregate line by the unit's divisor and prints its label.

**Rationale**: FR-016 states the capture at registration, which is the
cited revision's rule (`benchmark_register.cc:225` reads
`getDefaultTimeUnit()`), and the spec edge case requires a later default
to leave an earlier benchmark alone. The stored values stay nanoseconds,
so the C++ value keeps one unit and a later JSON report scales from the
same field. Scaling the overhead floor follows FR-015, which states every
printed time value of that benchmark.

**Alternatives considered**: scaling in the runner, rejected because the
stored value would then carry a unit the aggregate and a later report
would have to undo; a command-line option, deferred by FR-016 and the Out
list; a unit on `RunOptions`, rejected because the option layer applies
before registration and cannot honor the capture rule.

## D-13: The primary time drives the aggregate, and the gap rule follows it

**Decision**: `ResultRow` gains `timeAvailability`, the availability of
the primary time fold of that row. The aggregate builder skips a row whose
`timeAvailability` is not `COUNTABLE`, and the aggregate line names the
column it summarizes: `primary time/iter (<unit>)`.

**Rationale**: FR-002 makes one primary time per benchmark, and the gap
edge case states that a fold carrying a gap disclosure stays out of the
aggregate statistics. Today the aggregate builder collects
`row.timePerIterationNs` from every row (`source/harness/runner.cpp:502-506`),
so the rule needs one field to carry the disclosure. Naming the aggregate
column keeps the reader from guessing
which clock the statistics summarize, since FR-002 changes the default
from the real fold to the CPU selection.

**Alternatives considered**: one availability per time column, rejected in
Complexity Tracking of `plan.md`; a `gap` marker printed in the row,
rejected because FR-013 states the row fields and adds no time gap marker.

## D-14: The row labels

**Decision**: the repetition row prints `real time/iter (<unit>)=`,
`cpu time/iter (<unit>)=`, `overhead floor (<unit>)=`, and
`endpoint samples=<count>`. The unit label replaces the fixed `ns` of
`source/harness/report.cpp:93`, and the fixed `endpoint samples=2` becomes
the run's own count.

**Rationale**: FR-013 states the two columns and the sample count, and
FR-015 states the unit label on each printed time value. The word `real`
in the first column keeps the two tokens distinguishable to a reader and
to a test, and the existing tests that read `time/iter (ns)=` move to the
exact new token because the old token is a suffix of both.

**Alternatives considered**: keeping `time/iter` for the primary column
and adding one column for the other, rejected because FR-001 states real
time and CPU time as two columns of every measured row, and a primary-only
label would change meaning with the selection.

## D-15: The callback-state rule extends to the timing calls

**Decision**: `pauseTiming`, `resumeTiming`, and `setIterationTime` carry
the R-05 rule of H2: legal in a run state, an `SG_REQUIRE` violation in a
callback or fixture-hook state.

**Rationale**: R-05 already establishes the shape for `begin()`,
`skipWithError`, and `skipWithMessage`, and the callback state carries no
recorder window (`source/harness/runner.cpp:336`), so a pause there has
no window to pause. The spec's Assumptions state the same reading for the
timing calls.

**Alternatives considered**: allowing `setIterationTime` in a callback
state, rejected because the sum would reach a run that never opened a
window and the reported column would carry work from outside the run.
