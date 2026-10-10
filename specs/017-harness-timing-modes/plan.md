# Implementation Plan: Harness Timing Modes

**Feature branch**: `017-harness-timing-modes` | **Spec**: [spec.md](./spec.md)
**Audit point**: `adc3b92`, the H2 merge the spec cites. This plan reads the tree at `29f7982`.
**Date**: 2026-10-10 | **Constitution**: 2.18.0 | **Reference revision**: `google/benchmark` `main` at `e662de9a`

## Summary

H3 puts the timing selection on the family record and the pause and
manual machinery on the run. One time column becomes two: real time per
iteration and CPU time per iteration, each a fold over the run's sample
span. A handle call switches the CPU leaf to `machine/process_cpu`, a
second makes real time the primary time, a third selects manual time,
and a fourth sets the printed unit. `State` gains `pauseTiming`,
`resumeTiming`, `setIterationTime`, and the pause bookkeeping the
subtraction needs; `sg::ScopedPauseTiming` wraps the pair for its
lifetime. Every reported value stays a fold under one rule: the window
fold minus the fold of each paused interval, applied to both time
columns and to every metric column.

The run keeps the H1 loop shape. Pause and resume are explicit calls in
the benchmark body, each costing one sampling action, and the recorder
capacity rule gains a per-run pause allowance. The plan sets that
allowance at 8 intervals per run (R-07), and it settles the primary-time
availability field that carries the gap rule into the aggregate
statistics (R-09).

## Technical Context

**Language/Version**: C++23, the pinned standard of Principle I. No
compiler extension enters (FR-026).

**Primary Dependencies**: the H1 and H2 harness at the audit point (the
registry, the family record, the runner, the report, the timed window,
the calibration rule), the counters library (the plan, the recorder, the
fold, the availability disclosure), and the counters fake provider
(`include/speedgun-ng/counters_fake.hpp`) as the test substitution of H1
FR-042. Google Benchmark is a reference read; nothing links against it.

**Storage**: N/A. The timing selection lives in the process registry, and
the pause pairs and the manual accumulator live in one run's `State`.

**Testing**: CTest executables in `test/`, the fake provider for every
timing assertion, the re-exec abort pattern of the existing harness tests
for the precondition violations, and the gate scripts `test/loop_shape.sh`,
`test/cli_shape.sh`, and `test/time_source_gate.sh`.

**Target Platform**: Linux, the CI matrix of Principle VIII: GCC and
Clang, `ci-sanitize`, `ci-tsan`, and `ci-coverage`.

**Project Type**: library plus the harness archive the library exports.
No new target.

**Performance Goals**: the timed loop keeps the H1 instruction count, so
`loop_shape` stays green (FR-003); a run with no pause adds exactly one
sample pair and prints `endpoint samples=2` (SC-004); one pause or one
resume costs exactly one sampling action of the run's plan (FR-009).

**Constraints**: every time value stays a counters fold, so the
time-source gate stays clean (FR-021); no compiler extension and no new
runtime dependency (FR-026); 100% line and branch coverage with no
`LCOV_EXCL` marker under `source/harness/` (FR-023); single-threaded, as
H1 left it.

**Scale/Scope**: 26 requirements, 7 capabilities, one public header, four
harness sources and one harness header edited, six new test executables,
five existing test files updated for the row labels, one example edit, and
one documentation page extended by seven sections.

## Constitution Check

*(Gate: principles I to XI of 2.18.0 pass. Any **NO** needs an entry in
Complexity Tracking.)*

| Principle | Verdict and note |
| --- | --- |
| I | PASS. C++23 throughout. `std::vector`, `std::pair`, `std::span`, and `std::optional` carry the new state, so no new non-standard construct and no new P2 enters. |
| II | PASS. Every new interface documents `\pre` and `\post`, and the source enforces each clause through `SG_REQUIRE`, `SG_ENSURE`, or `SG_ASSERT` (FR-020). The documented kinds match the enforced kinds inside each interface scope, the shape `tools/dbc/dbc_pair_gate.py` pairs; a class-level `\invariant` stands on the class block with its enforcement in the members. |
| III | PASS. Requirements, design, code, and unit tests each carry their artifact, and `plan.md` records TDD mode (FR-024). |
| IV | PASS. The new public declarations carry doxygen blocks, and `docs/pages/harness.md` gains one section per capability C-1 to C-7 (FR-017). |
| V | PASS. N-1 to N-12 hold: `TimeUnit` and `ScopedPauseTiming` take PascalCase, `pauseTiming`, `resumeTiming`, `setIterationTime`, `measureProcessCpuTime`, `useRealTime`, `useManualTime`, and `unit` take camelBack, `kMaxPausePerRun` takes the `k` prefix of N-8, and the enumerators of `TimeUnit` take `UPPER_SNAKE_CASE` under N-4. No new macro enters, so N-3 has nothing to prefix (FR-019). |
| VI | PASS. Each capability C-1 to C-6 gains its own test executable, and the 100% line, branch, and DBC gates stay measured on the same paths (FR-023). |
| VII | PASS. The critical path stays the timed loop: the pause and manual bookkeeping stands outside it, in the calls the benchmark makes and in the cold exit path (R-06). |
| VIII | PASS. No gate weakens and no gate is added, so no amendment is needed. The version and `SOVERSION` table records the rise FR-022 requires. |
| IX | PASS. Spec, plan, and tasks each live in `specs/017-harness-timing-modes/`. |
| X | PASS. Threads, user counters, JSON reports, and the `--time_unit` command-line option stay out: each is a roadmap spec (the Out list of the spec Scope). The two design choices that need a written justification stand in Complexity Tracking. |
| XI | PASS. The artifacts and the code prose obey XI.1 to XI.7, and the prose gate reports zero findings (FR-025). |

**Gate weakening requested**: no.

## Project Structure

### Documentation (this feature)

```text
docs/
├── pages/
│   ├── barrier-rule.md            # unchanged (015)
│   └── harness.md                 # Edited: seven new sections, one per
│                                  #   capability C-1 to C-7 of this spec (FR-017)
└── Doxyfile.in                    # unchanged
```

### Source (this feature)

```text
include/speedgun-ng/
├── benchmark.hpp                  # Edited: TimeUnit, kMaxPausePerRun, the four
│                                  #   handle calls, State pause and manual members,
│                                  #   ScopedPauseTiming, the ResultRow and
│                                  #   BenchmarkResult time fields, the default-unit
│                                  #   library calls
└── counters*.hpp                  # unchanged

source/harness/
├── detail/internal.hpp            # Edited: the timing fields of the family record
├── detail/calibration.hpp         # Edited: RunRecord time and sample fields, the
│                                  #   manual exemption of the fivefold stop
├── runner.cpp                     # Edited: the CPU leaf choice, the netted folds,
│                                  #   the point cursor, the row time fields
├── registry.cpp                   # Edited: the four handle calls, the default time
│                                  #   unit, the unit captured at registration
├── report.cpp                     # Edited: two time columns, the real sample count,
│                                  #   the unit scaling and labels
├── family.cpp, cli.cpp            # unchanged
└── catalog.cpp                    # unchanged

CMakeLists.txt                     # Edited: version 0.8.0, harness SOVERSION 4
test/
├── CMakeLists.txt                 # Edited: six new harness test executables
├── source/harness_time_columns_test.cpp   # Added: C-1
├── source/harness_cpu_leaf_test.cpp       # Added: C-2
├── source/harness_real_time_test.cpp      # Added: C-3
├── source/harness_manual_time_test.cpp    # Added: C-4
├── source/harness_pause_test.cpp          # Added: C-5
├── source/harness_time_unit_test.cpp      # Added: C-6
├── source/harness_calibration_test.cpp    # Edited: the two-column row labels
├── source/harness_callback_test.cpp       # Edited: the two-column row labels
├── source/harness_capture_test.cpp        # Edited: the two-column row labels
├── source/harness_fixture_test.cpp        # Edited: the two-column row labels
├── source/harness_statistics_test.cpp     # Edited: the two-column row labels
└── loop_shape.sh, cli_shape.sh, time_source_gate.sh   # unchanged, all stay green
example/
└── benchmark_example.cpp          # Edited: one benchmark each for C-2, C-3,
                                   #   C-4, C-5, and C-6 (FR-017)
```

No new build target: the harness archive `speedgun-ng_harness` gains no
source file, and the six test executables link `speedgun-ng::harness` as
the existing block of `test/CMakeLists.txt` does.

## Design

### Decision index

| Id | Decision | Research |
| --- | --- | --- |
| R-01 | One plan shape carries both time cores in every mode, so one capacity rule and one sample count cover all four modes | D-06 |
| R-02 | The plan carries one CPU core, `machine/thread_cpu` or `machine/process_cpu`, chosen once per benchmark | D-02 |
| R-03 | Four handle calls behind the existing setter precondition, with the FR-008 exclusion on both of the two conflicting calls | D-03, D-04 |
| R-04 | The primary time is a selection over the CPU fold, the real fold, and the manual sum | D-01 |
| R-05 | Pause pairs are sample ordinals inside the run, and the runner keeps a point cursor | D-07 |
| R-06 | The new checks stand in `begin()`, the two pause calls, and the cold `closeWindow()`, outside the loop body the gate measures | D-10 |
| R-07 | `kMaxPausePerRun = 8` bounds one run at 18 points, and the capacity rule covers it | D-09 |
| R-08 | The unit is captured at registration, stored as nanoseconds, and scaled at print time | D-12 |
| R-09 | One `timeAvailability` field carries the gap rule for the primary fold | D-13 |

### Logical view: the selection and the clocks

The timing selection is family state, settled before the first run, and
the runner reads it once per benchmark when it builds the run plan.

```text
BenchmarkHandle                          RegistryEntry (internal.hpp:43)
  .measureProcessCpuTime()  ─┐             processCpuTime : bool
  .useRealTime()             ├─ SG_REQUIRE real+manual exclusion   (R-03)
  .useManualTime()           │             realTime       : bool
  .unit(TimeUnit)            ─┘             manualTime     : bool
                                            timeUnit       : TimeUnit  (R-08)
                                                    │
                                                    v
Runner::run (runner.cpp:139)
  machine/monotonic                     real core, always in the plan
  machine/process_cpu or thread_cpu     cpu core, one of the two      (R-02)
  one core per metric column            unchanged (H1)
                                                    │
                                                    v
RunRecord (calibration.hpp:24)
  realNs      cpuNs      decisionNs
  decisionNs = manualNs | realNs | cpuNs by the selection            (R-04)
```

The plan keeps both time cores in every mode, so one plan shape, one
capacity rule, and one sample count cover all four modes (R-01). The
primary time is a selection over the folds the plan already produces,
with the manual sum as its third source.

### Logical view: one run's sample span

A run owns a contiguous block of recorder points. The entry sample opens
the block, the exit sample closes it, and each pause and each resume
inserts one point in the middle.

```text
no pause (H1 shape, FR-003, SC-004)
  base            base+1
  [ entry ........ exit ]                      endpoint samples=2

one pause interval (FR-009, FR-010)
  base      base+1      base+2      base+3
  [ entry ... pause ...... resume ... exit ]   endpoint samples=4
            └── paused interval ──┘

reported value = fold(base, base+samples-1) − Σ fold(base+p0, base+p1)
```

The same helper nets every column: the real core, the cpu core, and each
metric core, so attached metrics exclude paused work too (FR-010, Q-4).
`State` records each pause pair as a pair of sample ordinals inside the
run, and the runner adds the run's base point when it folds. The runner's
pair counter becomes a point cursor that advances by the samples the run
took, so the pair index of the next run stays aligned (R-05).

### Logical view: the state timing surface

```text
State (benchmark.hpp:161)
  m_pauseDepth    : std::uint8_t   0 or 1, the FR-011 guard
  m_pausePairs    : vector<pair<uint64,uint64>>  sample ordinals
  m_samplesTaken  : std::uint64_t  the run's sample count
  m_manualNs      : std::int64_t   the manual accumulator
```

| Operation | In a run | In a callback or fixture hook |
| --- | --- | --- |
| `range(index)`, `rangeCount()`, `iterations()` | legal | legal |
| `begin()`, `skipWithError`, `skipWithMessage` | legal | `SG_REQUIRE` violation |
| `pauseTiming`, `resumeTiming`, `setIterationTime` | legal inside the window | `SG_REQUIRE` violation |
| `end()` | legal | legal: it is static and touches no state |

`pauseTiming` requires an open window, a closed pause depth of zero, and
a pause count below `kMaxPausePerRun`. `resumeTiming` requires an open
pause. The open-pause-at-loop-exit check stands in `closeWindow()`, the
`[[gnu::cold]]` path that leaves the loop, so it adds nothing to the loop
body the gate measures (R-06). `ScopedPauseTiming` constructs the pause
and resumes in its destructor, and the destructor skips the resume when
the run was skipped (FR-012).

### Logical view: the printed row

```text
before (H1, report.cpp:93)
  name  iterations=N  time/iter (ns)=X  overhead floor (ns)=Y  endpoint samples=2

after (FR-001, FR-013, FR-015)
  name  iterations=N  real time/iter (U)=X  cpu time/iter (U)=Y
        overhead floor (U)=Z  endpoint samples=S

aggregate (FR-002)
  name  AGGREGATE  primary time/iter (U) mean=... median=... n=...
```

`U` is the unit label of the benchmark: `ns`, `us`, `ms`, or `s`. The
stored values stay nanoseconds, and the report scales them, so the C++
value keeps one unit and a later JSON report scales the same way (R-08).
The aggregate summarizes the primary time, which defaults to the CPU
selection (FR-002).

### Physical view

| File | Namespace | Holds | Public? | Target |
| --- | --- | --- | --- | --- |
| `include/speedgun-ng/benchmark.hpp` | `sg` | `TimeUnit`, `kMaxPausePerRun`, the four handle calls, the `State` timing members, `ScopedPauseTiming`, the result time fields, `setDefaultTimeUnit` and `getDefaultTimeUnit` | yes | `speedgun-ng_harness` |
| `source/harness/detail/internal.hpp` | `sg` | the timing fields of `RegistryEntry` | no | `speedgun-ng_harness` |
| `source/harness/detail/calibration.hpp` | `sg::detail` | the `RunRecord` time and sample fields, the manual exemption of `qualifies` | no | `speedgun-ng_harness` |
| `source/harness/runner.cpp` | `sg::detail` | the leaf choice, the netted folds, the point cursor, the row time fields | no | `speedgun-ng_harness` |
| `source/harness/registry.cpp` | `sg` | the four handle calls, the process default unit, the unit captured at registration | no | `speedgun-ng_harness` |
| `source/harness/report.cpp` | `sg::detail` | the two columns, the sample count, the scaling and labels | no | `speedgun-ng_harness` |
| `test/source/harness_*_test.cpp` | - | the C-1 to C-6 checks | - | six test executables |

### Public API surface added

```cpp
// The time unit of the printed values (FR-015, FR-016)
enum class TimeUnit : std::uint8_t
{
  NANOSECONDS,
  MICROSECONDS,
  MILLISECONDS,
  SECONDS,
};

// FR-014: the per-run pause allowance of the capacity rule
inline constexpr std::size_t kMaxPausePerRun = 8;

// BenchmarkHandle, each returning BenchmarkHandle& for chaining
auto measureProcessCpuTime() -> BenchmarkHandle&;   // FR-004
auto useRealTime() -> BenchmarkHandle&;             // FR-005
auto useManualTime() -> BenchmarkHandle&;           // FR-006
auto unit(TimeUnit timeUnit) -> BenchmarkHandle&;   // FR-015

// State, the timing surface
auto pauseTiming() -> void;                          // FR-009
auto resumeTiming() -> void;                         // FR-009
auto setIterationTime(std::int64_t nanoseconds) -> void;  // FR-007

// The scoped guard (FR-012)
class SPEEDGUN_NG_EXPORT ScopedPauseTiming
{
public:
  explicit ScopedPauseTiming(State& state);
  ~ScopedPauseTiming();
};

// The library default unit, captured at registration (FR-016)
auto setDefaultTimeUnit(TimeUnit timeUnit) -> void;
[[nodiscard]] auto getDefaultTimeUnit() -> TimeUnit;

// ResultRow (FR-018)
double cpuTimePerIterationNs = 0.0;
std::uint64_t endpointSamples = 0;
sg::counters::Availability timeAvailability = sg::counters::Availability::COUNTABLE;

// BenchmarkResult (FR-018)
TimeUnit timeUnit = TimeUnit::NANOSECONDS;
```

`registerBenchmark`, `speedgunMain`, the H1 and H2 setters, the H1
`State` methods, and `ResultRow::timePerIterationNs` keep their
signatures and names (FR-018).

### Version and `SOVERSION`

| Artifact | At audit point | After H3 | Reason |
| --- | --- | --- | --- |
| `project(speedgun-ng VERSION)` | 0.7.0 | 0.8.0 | FR-022, the next minor |
| `speedgun-ng` archive `SOVERSION` | 2 | 2 | the counters surface changes nothing (FR-022) |
| `speedgun-ng_harness` archive `SOVERSION` | 3 | 4 | `State` gains pause bookkeeping and the manual accumulator, and `ResultRow` and `BenchmarkResult` gain time fields, so three layouts change (FR-022) |
| constitution | 2.18.0 | 2.18.0 | no amendment: no gate changes |

## Test Plan

**Execution mode**: TDD (Principle III, FR-024). Each test file is
written and observed red before the implementation that turns it green.
Every timing assertion runs on the fake provider with scripted
`machine/monotonic`, `machine/thread_cpu`, and `machine/process_cpu`
sequences, so no test needs a PMU, a sleep, or a wall-clock bound
(FR-024).

**Scenario and check mapping**:

| Capability | Scenario | Check |
| --- | --- | --- |
| C-1 time columns | both columns exact to the scripted deltas; the CPU column defaults to `machine/thread_cpu`; a run with no pause adds one sample pair and prints `endpoint samples=2`; an attached metric keeps its H1 fold beside the two columns | `test/source/harness_time_columns_test.cpp`, `test/loop_shape.sh` |
| C-2 process CPU leaf | the switch moves the CPU column to the scripted `machine/process_cpu` fold; the switch combines with the real-time call; a host that publishes no `machine/process_cpu` fails that benchmark with the resolution line | `test/source/harness_cpu_leaf_test.cpp` |
| C-3 real-time primary | the scripted growth sequence follows the real fold, the qualification test reads the real fold, and the aggregate summarizes the real column | `test/source/harness_real_time_test.cpp` |
| C-4 manual time | the accepted sum divided by N prints in the real column; the growth rule reads the accepted sum; the fivefold stop stays exempt; a benchmark that accepts nothing reports zero and qualifies at the iteration cap; `useRealTime` with `useManualTime` aborts | `test/source/harness_manual_time_test.cpp` |
| C-5 pause and resume | each column equals the window fold minus the pause fold to the nanosecond; the metric fold carries the same subtraction; the scoped guard matches the explicit pair; the row prints `endpoint samples=4`; a second pause, a resume with no pause, an open pause at loop exit, and a pause in a callback state each abort the re-exec; the pause allowance aborts at the ninth interval | `test/source/harness_pause_test.cpp` |
| C-6 time units | each unit prints the scaled value with its label in both columns and in the overhead floor; the library default scales a benchmark that sets none; a default set after registration leaves an earlier benchmark at its captured unit | `test/source/harness_time_unit_test.cpp` |
| C-7 examples and docs | the example builds and runs one benchmark each for C-2, C-3, C-4, C-5, and C-6; `docs/pages/harness.md` holds one section per capability C-1 to C-7 | `example/CMakeLists.txt`, prose gate, review |
| whole surface | every hard gate of Principle VIII at the feature head, the time-source gate and the loop-shape gate included | SC-006 run of the CI matrix |

**Precondition violations** follow the established pattern: the scenario
re-execs the test binary, the `SG_REQUIRE` or `SG_ASSERT` ends that
process, and the parent reads the signal, as
`test/source/harness_callback_test.cpp:593-621` and
`test/source/harness_family_test.cpp:210` do.

**Existing tests the row layout touches**: `harness_calibration_test.cpp`
(lines 296, 375, 404), `harness_callback_test.cpp` (464),
`harness_capture_test.cpp` (330, 372), `harness_fixture_test.cpp` (488),
and `harness_statistics_test.cpp` (199) read the field token
`time/iter (ns)=`. That token is a suffix of both new column tokens, so
each call site moves to the exact new token, `real time/iter (ns)=` or
`cpu time/iter (ns)=`, in the same commit that changes `report.cpp`. The
list-mode checks of `harness_cli_test.cpp`, `harness_family_test.cpp`, and
`harness_instance_name_test.cpp` look for `time/iter` and stay green.

**Registration pattern**: each new test follows the existing block of
`test/CMakeLists.txt`: a `harness_<area>_test` executable built from
`source/harness_<area>_test.cpp`, linked to `speedgun-ng::harness`,
`cxx_std_23`, with the CTest name equal to the target name. The pause and
real-time tests read `RunRecord` and the netted fold helpers, so they
carry `target_include_directories(... ${CMAKE_SOURCE_DIR}/source/harness)`
as `harness_growth_test` and `harness_gap_test` do.

**Gates that must stay green**: `loop_shape`
(`test/CMakeLists.txt`), `cli_shape`, `barrier_shape`, `time_source_gate`,
`dbc-gate` (`cmake/dbc-gate.cmake`), `prose-lint`, `spell-check`,
`format-check`, the name check, `ci-sanitize`, `ci-tsan`, and the
coverage gates. The coverage target measures 100% line and branch over
`include/speedgun-ng/*` and `source/*`, and it omits the contract macro
lines (`cmake/coverage.cmake:76`), so every new selection branch and
pause branch needs a test while its contract lines stay free (FR-023).
The presets are `ci-ubuntu` to `build`, `ci-sanitize` to `build/sanitize`,
`ci-tsan` to `build/tsan`, and `ci-coverage` to `build/coverage`; the
`dev` preset is machine-local, and Principle IX adds the one release
build per feature.

**Loop-shape exposure (R-06)**: `test/loop_shape.sh` compiles a loop of
`for (auto _ : state)` at `-O2` and fails on an out-of-line call inside
the shortest backward jump, or on a body longer than the hand-written
count-down reference. Pause and resume are calls the benchmark body
makes, so they sit in the caller's loop body and never in the cursor.
The new checks stand in `begin()`, in the two pause calls, and in
`closeWindow()`, all outside the backward jump. The check is
`ctest --test-dir build -R loop_shape` in a `ci-ubuntu` tree.

**Determinism**: scripted clock deltas from `FakeProvider::setPoints`
drive every timing figure, and the manual scenarios carry no clock
dependence at all. The scripted action index of a plan includes that
plan's own sampling-cost calibration, the fact the existing calibration
test measures with a fixed-N probe, where no hard-coded constant stands.

**Deliberately untouched**: threads and per-thread CPU aggregation, user
counters and statistics, JSON reports, the full command line including a
time-unit option, chunked capture, and percentiles. Each is a roadmap
spec in the spec Out list, and X.2 keeps it out.

## Constitution Check

*(Post-design pass: does the design as written honor I to XI?)*

| Principle | Verdict and note |
| --- | --- |
| I | PASS. The new surface uses `std::vector`, `std::pair`, `std::uint8_t`, and an `enum class`; no compiler extension and no non-standard construct enters (FR-026). |
| II | PASS. The FR-008 exclusion, the FR-011 pause-state rules, the callback-state rule, and the FR-014 allowance each stand as an enforced clause beside the doxygen clause that states it. The pair gate finds one enforced kind per documented kind. |
| III | PASS. The logical views carry the contracts, and the physical view names the file, namespace, and target of each piece. |
| IV | PASS. The header blocks state each contract in one place, and `docs/pages/harness.md` states the user-facing rule per capability. |
| V | PASS. `TimeUnit::NANOSECONDS` follows N-4, `kMaxPausePerRun` follows N-8, `ScopedPauseTiming` follows N-1, and the six new functions follow N-2. `measureProcessCpuTime` spells the acronym as one word, the N-1 rule for an acronym inside a name. No `.clang-tidy` key needs a new exemption. |
| VI | PASS. Six new executables cover the selection branches, the pause branches, the unit table, and the abort paths; the coverage exclusion list stays unchanged and no `LCOV_EXCL` enters `source/harness/` (FR-023). |
| VII | PASS. The timed loop keeps its H1 instruction count, and the sampling cost of each pause stands printed beside the row it affects (FR-013). |
| VIII | PASS. No gate weakens, and the version table records the `SOVERSION` rise FR-022 requires. |
| IX | PASS. Every artifact of R-DCUT lives in the feature directory, and `plan.md` records TDD mode. |
| X | PASS. No new source file, one new public class, and no abstraction beyond the spec's own nouns. The two choices that need a written justification stand in Complexity Tracking. |
| XI | PASS. The artifacts pass the prose and spell gates at generation (FR-025). |

## Complexity Tracking

| Item | Principle | Why it is needed | Simpler alternative rejected |
| --- | --- | --- | --- |
| The run plan keeps the `machine/monotonic` core when manual time selects, so a manual run still pays two endpoint samples | X.2 | One plan shape, one capacity rule, and one sample count cover all four modes, the CPU column still needs its leaf, and the H1 loop discipline of FR-003 and FR-017 keeps its single definition in `runner.cpp`. | Dropping the monotonic core for a manual benchmark: rejected for the second plan-construction path and the second capacity rule it would add, for a saving of two reads per run and no change to any reported value. |
| `ResultRow` carries one `timeAvailability` field for the primary column | II, X.2 | FR-002 makes one primary time per benchmark, and the gap edge case names the aggregate statistics, which are the primary time statistics. One field carries the rule, and the aggregate builder skips a row whose primary fold reported a gap. | One availability per time column: rejected for a second field no requirement asks for, while the printed value of the secondary column stays what its fold reports. |
| `kMaxPausePerRun` is a fixed allowance of 8 intervals per run | X.1 | FR-014 requires the capacity rule to cover the pause pairs before a run can overrun the hard-stop recorder, and a fixed allowance makes the guard one integer compare in `pauseTiming`. The value bounds one run at 18 points, so the recorder for the 193 runs of the H1 rule holds 3474 points per leaf. | Growing the recorder on demand: rejected for an allocation on a run path and a second capacity rule, and for a bound the row's sample count could no longer state. |
