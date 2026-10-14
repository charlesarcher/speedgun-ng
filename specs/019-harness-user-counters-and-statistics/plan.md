# Implementation Plan: Harness User Counters and Statistics

**Feature branch**: `019-harness-user-counters-and-statistics` | **Spec**: [spec.md](./spec.md)

**Audit point**: `a7776b6`, the D1 merge the spec cites; the working
checkout is content-identical to it.
**Date**: 2026-10-14 | **Constitution**: 2.19.0 at the audit point,
2.20.0 at the feature head (the FR-036 amendment) | **Reference
revision**: `google/benchmark` `main` at `e662de9a`

## Summary

H4 puts user counters on the state, the flag rules after the run, and
the printed columns, the trailing label field, the custom statistic
fields, the aggregate-only modes, and the complexity fit on the report.
The counter store is a fixed sixteen-slot array inside the state, so a
body that opens names inside the timed loop allocates nothing (R-01).
The runner applies the flags in the cited `Finish` order with the row's
primary time as the rate divisor (R-02). Two H3 leftovers land as the
first tasks: the per-run pause allowance dies and its recorder keeps
bounded memory, reserving pause room from the family's observed
history and folding closed pairs beyond the cap (R-10), and the
default time unit resolves when
`speedgunMain` starts so the shipped main-page promise becomes true
(R-11). The fit is one least-squares curve per family over its
instances, printed per fitted quantity (R-07). Every section of the
main page gains a runnable example behind its snippet, and the drift
gate learns to check that a snippet is a region of a file CI runs
(R-12, R-13), with constitution IV.1 amended to the same obligation
(R-14).

## Technical Context

**Language/Version**: C++23, the pinned standard of Principle I. No
compiler extension enters.

**Primary Dependencies**: the H1 to H3 harness at the audit point (the
registry, the family record, the runner, the report, the timed window,
the calibration rule, the timing selection), the counters library (the
plan, the recorder, the fold, the availability disclosure), and the
counters fake provider (`include/speedgun-ng/counters_fake.hpp`) as the
test substitution. Google Benchmark is a reference read; nothing links
against it.

**Storage**: N/A. Counters live in one run's state, the statistics, the
modes, and the fit selection live in the process registry, and the fit
result lives in the runner's batch.

**Testing**: CTest executables in `test/`, the fake provider for every
counter, statistic, and fit assertion, the re-exec pattern for the
precondition violations, the gate scripts `test/loop_shape.sh`,
`test/cli_shape.sh`, `test/barrier_shape.sh`, and
`test/time_source_gate.sh`, and the new `example_run` check that runs
every main-page example.

**Target Platform**: Linux, the CI matrix of Principle VIII: GCC and
Clang, `ci-sanitize`, `ci-tsan`, and `ci-coverage`.

**Project Type**: library plus the harness archive the library exports.
No new target; the archive gains one source file.

**Performance Goals**: the timed loop keeps its H1 instruction count, so
`loop_shape` stays green; opening and updating a counter is a scan over
sixteen slots and two stores, with no allocation (R-01); the flag pass,
the statistics, and the fit run after the loop, on the cold path.

**Constraints**: every time value stays a counters fold, so the
time-source gate stays clean (FR-050); 100% line and branch coverage
with no `LCOV_EXCL` marker under `source/harness/` (FR-047);
single-threaded, as H1 left it, so the thread-average divisor is one
(FR-004); the page cap moves to 550 while the 25-line section cap holds
(FR-044).

**Scale/Scope**: 54 requirements, 9 capabilities, one public header
extended, five harness sources edited plus one new, six new test
executables, two rewrites, one new gate script, eighteen example files under
`example/docs/`, six new main-page sections, and one constitution
amendment.

## Constitution Check

*(Gate: principles I to XI of 2.19.0 pass, and the FR-036 amendment
lands with the feature. Any **NO** needs an entry in Complexity
Tracking.)*

| Principle | Verdict and note |
| --- | --- |
| I | PASS. C++23 throughout. `std::array`, `std::string_view`, `std::function`, `std::optional`, and `std::vector` carry the new state, so no new non-standard construct enters. |
| II | PASS. Every new interface documents `\pre` and `\post`, and the source enforces each clause: the sixteen-name cap, the thirty-two-byte name bound, the label newline bound, the statistic-name bound, the counter-flag agreement rule, and the existing before-the-first-run guard (FR-046). |
| III | PASS. Requirements, design, code, and unit tests each carry their artifact, and `plan.md` records TDD mode (FR-050). |
| IV | PASS. The new public declarations carry doxygen blocks, `README.md` gains six feature sections with runnable snippets, and `docs/pages/harness.md` gains the detail per capability (FR-028, FR-031). |
| V | PASS. `Counter`, `BigO`, `StatUnit`, and `ScopedPauseTiming`-style PascalCase and `UPPER_SNAKE_CASE` enumerators per N-4, `setCounter`, `setLabel`, `addStatistic`, and siblings in camelBack per N-2, `kMaxUserCounters` and `kMaxCounterNameBytes` with the N-8 prefix. No new macro enters. |
| VI | PASS. Each capability gains its own test executable, and the 100% line and branch gates stay measured on the same paths, with no `LCOV_EXCL` marker (FR-047). |
| VII | PASS. The critical path stays the timed loop: the flag pass, the statistics, the fit, and the report all run after the loop exits (R-02, R-07). |
| VIII | PASS with an amendment. No gate weakens. The requested strengthening moves IV.1 from compiles to compiles and runs, adds the `example_run` gate, and raises the page-line budget of the drift tool from 400 to 550 (FR-036, FR-044). The version and `SOVERSION` table records the rise FR-048 requires. |
| IX | PASS. Spec, plan, and tasks each live in `specs/019-harness-user-counters-and-statistics/`. |
| X | PASS. Threads, JSON reports, and histograms stay out: each is a roadmap spec (the Out list of the spec Scope). The four design choices that need a written justification stand in Complexity Tracking. |
| XI | PASS. The artifacts and the code prose obey XI.1 to XI.7, the internal guide notes are rewritten as reader sentences (FR-031, Q-11), and the prose gate reports zero findings (FR-050). |

**Gate weakening requested**: no. **Gate strengthening requested**:
yes, the IV.1 and Principle VIII amendment of FR-036, recorded at
constitution 2.20.0 with the lineage row and the amendment record the
precedent of 2.19.0 shows.

## Project Structure

### Documentation (this feature)

```text
README.md                              # Edited: six new feature sections,
                                       #   refreshed reading-the-report,
                                       #   quick-reference rows, drops six
                                       #   shipped entries from planned work,
                                       #   FR-037 to FR-043 leftovers
docs/pages/harness.md                  # Edited: detail sections for the six
                                       #   new capabilities, the pause bound
                                       #   and capture-point texts rewritten,
                                       #   the five internal Coverage
                                       #   paragraphs rewritten (Q-11)
.specify/memory/constitution.md        # Edited: IV.1 build-and-run
                                       #   obligation, VIII gate names the
                                       #   example_run check, 2.20.0
```

### Source (this feature)

```text
include/speedgun-ng/
├── benchmark.hpp                      # Edited: Counter, CounterFlag,
│                                      #   CounterBase, kMaxUserCounters,
│                                      #   kMaxCounterNameBytes, the State
│                                      #   counter/label/complexity-n calls,
│                                      #   BigO, StatUnit, StatisticFunc,
│                                      #   BigOFun, the five new handle
│                                      #   calls, RowKind BIG_O and RMS,
│                                      #   ResultRow label and counters,
│                                      #   BenchmarkResult counterLabels
│                                      #   and counterStats
└── counters*.hpp                      # unchanged

source/harness/
├── detail/internal.hpp                # Edited: the statistics vector, the
│                                      #   two mode flags, the fit selection,
│                                      #   and the optional time unit of
│                                      #   RegistryEntry
├── detail/complexity.hpp              # Added: the curve table and the
│                                      #   closed-form relative-error fit
├── complexity.cpp                     # Added: per-family fit assembly and
│                                      #   the synthetic fit rows
├── runner.cpp                         # Edited: the counter store open and
│                                      #   update, the flag pass, the counter
│                                      #   columns and disclosures, the
│                                      #   statistic assembly, the pause
│                                      #   point bound, the unit resolution,
│                                      #   the name-collision check
├── registry.cpp                       # Edited: the five handle calls, the
│                                      #   statistic-name bound, the default
│                                      #   unit snapshot
├── report.cpp                         # Edited: the trailing label field,
│                                      #   the counter columns and statistic
│                                      #   lines, the statistic fields, the
│                                      #   fit rows, the aggregate filter
├── family.cpp, cli.cpp, catalog.cpp   # unchanged
CMakeLists.txt (repository root)       # Edited: the project() VERSION
                                       #   0.9.0, the harness target
                                       #   SOVERSION 5

test/
├── CMakeLists.txt                     # Edited: six new executables, two
│                                      #   rewrites, and the example_run gate
├── source/harness_user_counters_test.cpp     # Added: C-1
├── source/harness_throughput_test.cpp        # Added: C-2
├── source/harness_label_test.cpp             # Added: C-4
├── source/harness_custom_stats_test.cpp      # Added: C-5
├── source/harness_aggregates_only_test.cpp   # Added: C-6
├── source/harness_complexity_test.cpp        # Added: C-7
├── source/harness_pause_test.cpp             # Edited: the bound tests die,
│                                             #   every-iteration pause lives
├── source/harness_time_unit_test.cpp         # Edited: resolution moves to
│                                             #   the start of the run
├── example_run.sh                     # Added: runs every docs example and
│                                      #   fails on FAILED and on undeclared
│                                      #   SKIPPED rows (D-15)
└── loop_shape.sh, time_source_gate.sh # unchanged, both stay green

example/
├── docs/                              # Added: eighteen snake_case files,
│                                      #   one per main-page section (D-12),
│                                      #   each with a main that calls
│                                      #   speedgunMain
├── CMakeLists.txt                     # Edited: add_docs_example(NAME)
│                                      #   builds docs/${NAME}.cpp as
│                                      #   docs_${NAME} + run_docs_${NAME};
│                                      #   the docs_snippets block dies
└── docs_snippets.cpp                  # Deleted: its regions move into
                                       #   example/docs files

tools/docs/example_drift.py            # Edited: six new section titles, the
                                       #   550-line page cap, the region
                                       #   ownership check
```

**Structure decision**: the harness archive `speedgun-ng_harness` gains
`complexity.cpp` and keeps one archive target; the test executables link
`speedgun-ng::harness` as the existing block of `test/CMakeLists.txt`
does, and each `example/docs/<slug>.cpp` becomes a `run_` target through
the existing `add_example` mechanism.

## Design

### Decision index

| Id | Decision | Research |
| --- | --- | --- |
| R-01 | The counter store is a fixed sixteen-slot array with 32-byte name buffers inside the state, so a loop allocates nothing | D-01 |
| R-02 | The flag pass runs after the loop in the cited `Finish` order, with the row's primary time as the rate divisor and one as the thread divisor | D-02 |
| R-03 | A disclosed primary fold flags the whole counter set of its row; flagged rows keep the neutral value and stand aside from the counter statistics | D-03 |
| R-04 | The label is one string on the state and the row, printed in the trailing ` label=` field, shared-label rule for the aggregate | D-04 |
| R-05 | A custom statistic is a (name, unit, function) triple on the family record; a built-in name is a precondition violation at the call | D-05 |
| R-06 | The two modes are booleans on the family record; the report drops repetition rows for a restricted family unless no aggregate exists | D-06 |
| R-07 | One closed-form relative-error least-squares fit per family, in one new source file, printed as synthetic fit rows | D-07 |
| R-08 | Instance N is the `setComplexityN` value, falling back to the family's range value, and a non-positive N ends the fit with a reason | D-08 |
| R-09 | Every fitted quantity (time, counters, metrics) prints one BigO row and one RMS row through one synthetic fit result per family | D-09 |
| R-10 | Pause room is reserved from the family's observed pauses per iteration, capped at `kMaxPauseSamplesPerLeaf` raw points per leaf column (32'768 points, 256 KiB at one `std::uint64_t` per point); a first attempt or an over-cap run folds closed pairs into running totals, so memory stays bounded and the fixed allowance dies | D-10, Q-10 |
| R-11 | The family record stores `std::optional<TimeUnit>`; the run resolves it at run start against the process default | D-11 |
| R-12 | One example file per main-page section under `example/docs/`, with a new `example_run` CTest gate | D-12 |
| R-13 | The drift tool gains the six titles, the 550-line cap, and the region ownership check | D-13 |
| R-14 | Constitution IV.1 and Principle VIII amend to the build-and-run obligation at 2.20.0 | D-14 |
| R-15 | The run check pairs each `SKIPPED` row with a `// example-run: expect-skip <instance>: <message>` marker of the example file, one-to-one, with the four outcomes fixture-pinned | D-15 |

### Logical view: the counter store and the flag pass

```text
State detail (one run)
  slots : std::array<CounterSlot, kMaxUserCounters>          (R-01)
    CounterSlot { name : char[kMaxCounterNameBytes], value,
                  flags, base, used }
  open(name, counter): linear scan, first use or match
    match  -> same flags and base, or the run fails (FR-008)
    free   -> copy name (bounded), set fields
    none   -> the run fails with the cap reason (FR-001)

after the loop, once per repetition (R-02):
  v = slot.value
  RATE               : v = v / (primaryNs / 1e9)
  AVG_THREADS        : v = v / 1                        (FR-004)
  ITERATION_INVARIANT: v = v * iterations
  AVG_ITERATIONS     : v = v / iterations
  INVERT             : v = 1 / v                        (last, always)
  primaryNs <= 0     -> keep neutral value, mark the row's
                        zero-divisor disclosure                          (FR-005)
```

The pass sits in the runner beside the existing fold assembly, cold code
after the loop; the loop body holds only the body's own `setCounter`
calls, so `loop_shape` sees no new instruction in the backward jump.

### Logical view: the printed row

```text
repetition row (after H4)
  name [label=<text>]  iterations=N  real ... cpu ... overhead ...
  endpoint samples=S  <metric fields unchanged>  <name>=<value> ...

aggregate row: unchanged fields, then one statistic line per counter,
then the family's custom statistic lines (mean=... forms unchanged)

fit rows (FR-022, R-09), printed after the family's rows:
  <family>  BIG_O  <quantity> <curve> (<coefficient>)
  <family>  RMS    <quantity> <rms-percentage>%
```

The label field stands after the name column and appears only when the
row carries a label (FR-011, clarification 2026-10-14). Counter columns
print the flag-applied value divided by the declared base, like the
cited console (the counter print with `oneK` in
`src/console_reporter.cc`). The report drops
repetition rows when the family's mode stands and an aggregate row
exists (FR-016 to FR-018).

### Logical view: the fit

```text
family F (complexity selected), instances i with N_i > 0
  y_i = the instance's aggregate value for the quantity
  curve f: ONE, N, N squared, N cubed, log N, N log N, user fun
  relative-error least squares (closed form, GB's rule):
    k = sum(f(N_i)/y_i) / sum((f(N_i)/y_i)^2)
    rms% = sqrt(mean(((y_i - k f(N_i))/y_i)^2)) * 100
  AUTOMATIC: try each curve, keep the least rms, constant as fallback
```

One family with fewer than two instances, or an instance with a
non-positive N, ends the fit with the named reason (FR-023); a quantity
missing from an instance ends that quantity's rows with the same rule
(FR-022 edge). The fit lives in `complexity.cpp` and runs after the
family's instances have reported.

### Logical view: the two fixes

```text
pause (FR-024 to FR-026, FR-051 to FR-054):
  room = endpoints + 2 * pairsPerIteration(prev attempt)
         * iterations, per leaf column
  room <= kMaxPauseSamplesPerLeaf                        (R-10)
  no history, or over the cap: fold closed pairs into
  running totals at commit, from the same two raw points
  kMaxPausePerRun, its guard, its bound tests, and the
  ninthPause re-exec mode die

default unit (FR-027):
  RegistryEntry.timeUnit : std::optional<TimeUnit>       (R-11)
  speedgunMain start: snapshot getDefaultTimeUnit() into the batch
  run:  result.timeUnit = entry.unit, else batch snapshot,
        else the process default read at run start
```

The registration-time capture in `addRegistryEntry`
(`source/harness/registry.cpp`) dies with R-11,
and the README "The time unit" section and the
`docs/pages/harness.md` "Time units" paragraph state the new rule in
the same commit
range (FR-041).

### Physical view

| File | Namespace | Holds | Public? | Target |
| --- | --- | --- | --- | --- |
| `include/speedgun-ng/benchmark.hpp` | `sg` | `Counter`, `CounterFlag`, `CounterBase`, `BigO`, `StatUnit`, `StatisticFunc`, `BigOFun`, the state and handle surface, the result fields, the two constants | yes | `speedgun-ng_harness` |
| `source/harness/detail/internal.hpp` | `sg` | the new `RegistryEntry` fields | no | `speedgun-ng_harness` |
| `source/harness/detail/complexity.hpp` | `sg::detail` | the curve table and the closed-form fit | no | `speedgun-ng_harness` |
| `source/harness/complexity.cpp` | `sg::detail` | the per-family fit assembly and synthetic rows | no | `speedgun-ng_harness` |
| `source/harness/runner.cpp` | `sg::detail` | the counter store, the flag pass, the disclosures, the statistics assembly, the point bound, the unit resolution, the collision check | no | `speedgun-ng_harness` |
| `source/harness/registry.cpp` | `sg` | the five handle calls, the statistic bound, the unit snapshot | no | `speedgun-ng_harness` |
| `source/harness/report.cpp` | `sg::detail` | the label field, the counter columns and lines, the statistic fields, the fit rows, the aggregate filter | no | `speedgun-ng_harness` |

### Public API surface added

```cpp
// Counter values with flags and a decimal base (FR-001 to FR-003)
enum class CounterFlag : std::uint32_t
{
  NONE = 0,
  RATE = 1u << 0,
  AVG_THREADS = 1u << 1,
  ITERATION_INVARIANT = 1u << 2,
  AVG_ITERATIONS = 1u << 3,
  INVERT = 1u << 31,
};
constexpr auto operator|(CounterFlag lhs, CounterFlag rhs) -> CounterFlag;

enum class CounterBase : std::uint8_t { K1000, K1024 };

class SPEEDGUN_NG_EXPORT Counter
{
public:
  explicit Counter(double value = 0.0,
                   CounterFlag flags = CounterFlag::NONE,
                   CounterBase base = CounterBase::K1000);
  [[nodiscard]] auto value() const -> double;
  auto setValue(double value) -> void;
  [[nodiscard]] auto flags() const -> CounterFlag;
  [[nodiscard]] auto base() const -> CounterBase;
};

inline constexpr std::size_t kMaxUserCounters = 16;        // FR-001
inline constexpr std::size_t kMaxCounterNameBytes = 32;    // R-01

// State (FR-001, FR-009, FR-011, FR-019)
auto setCounter(std::string_view name, const Counter& counter) -> void;
auto setBytesProcessed(std::int64_t bytes) -> void;   // bytes_per_second, K1024, RATE
auto setItemsProcessed(std::int64_t items) -> void;   // items_per_second, K1000, RATE
auto setLabel(std::string_view label) -> void;
auto setComplexityN(std::int64_t problemSize) -> void;

// Fit selection and statistics (FR-013, FR-016, FR-017, FR-020)
enum class BigO : std::uint8_t
{ NONE, ONE, N, N_SQUARED, N_CUBED, LOG_N, N_LOG_N, AUTOMATIC };
enum class StatUnit : std::uint8_t { TIME, PERCENTAGE };
using StatisticFunc = std::function<bool(const std::vector<double>& values, double& result)>;
using BigOFun = std::function<double(double n)>;

auto addStatistic(std::string_view name, StatUnit unit, const StatisticFunc& function) -> BenchmarkHandle&;
auto reportAggregatesOnly() -> BenchmarkHandle&;
auto displayAggregatesOnly() -> BenchmarkHandle&;
auto complexity(BigO bigO) -> BenchmarkHandle&;
auto complexity(BigOFun function) -> BenchmarkHandle&;

// Result model (FR-006, FR-011, FR-022)
//   RowKind gains BIG_O and RMS
//   ResultRow gains  std::string label;  std::vector<double> counters;
//   BenchmarkResult gains  std::vector<std::string> counterLabels;
//                          std::vector<Statistics> counterStats;
//   BenchmarkResult gains  bool hideRepetitionRows (R-06);
```

`State::setIterationTime`, `pauseTiming`, `resumeTiming`, every H1 and
H2 handle setter, `registerBenchmark`, `speedgunMain`,
`setDefaultTimeUnit`, and `getDefaultTimeUnit` keep their signatures;
only the documented resolution point of the default unit changes
(FR-027, FR-046).

### Version and `SOVERSION`

| Artifact | At audit point | After H4 | Reason |
| --- | --- | --- | --- |
| `project(speedgun-ng VERSION)` | 0.8.0 | 0.9.0 | FR-048, the next minor (Q-6) |
| `speedgun-ng` archive `SOVERSION` | 2 | 2 | the counters surface changes nothing (FR-048) |
| `speedgun-ng_harness` archive `SOVERSION` | 4 | 5 | `State`, `BenchmarkHandle`, `ResultRow`, and `BenchmarkResult` layouts change (FR-048) |
| constitution | 2.19.0 | 2.20.0 | the FR-036 amendment to IV.1 and Principle VIII |

## Test Plan

**Execution mode**: TDD (Principle III, FR-050). Each test file is
written and observed red before the implementation that turns it green.
Every counter, statistic, fit, and mode assertion runs on the fake
provider with scripted `machine/monotonic`, `machine/thread_cpu`, and
`machine/process_cpu` sequences, so no test needs a PMU, a sleep, or a
wall-clock bound.

**Scenario and check mapping**:

| Capability | Scenario | Check |
| --- | --- | --- |
| C-1 user counters | each flag and combination prints the value the cited `Finish` order gives on scripted numbers; the cap aborts at the seventeenth name; a flag change on a name and a name colliding with a metric label each fail the run; the loop-shape gate stays green with counters set in the body | `test/source/harness_user_counters_test.cpp`, `test/loop_shape.sh` |
| C-2 throughput | bytes and items print exact rates over the primary time in all three timing modes, scaled by 1024 and 1000 | `test/source/harness_throughput_test.cpp` |
| C-3 leftovers | a calibrated run pauses in every iteration and completes; a pause-heavy scripted run reserves at most `kMaxPauseSamplesPerLeaf` points per leaf column, a non-pausing run reserves no pause room, and the fold path prints the raw path's net columns on the scripted provider; `setDefaultTimeUnit` in `main` reaches macro registrations while `unit` still wins; the old bound, guard, and tests are gone from the tree | `test/source/harness_pause_test.cpp`, `test/source/harness_time_unit_test.cpp` |
| C-4 labels | the row ends with ` label=<text>`; differing repetition labels leave the aggregate unlabeled; a newline label aborts the re-exec | `test/source/harness_label_test.cpp` |
| C-5 custom statistics | a time-unit statistic prints in the row unit, a percentage statistic prints unscaled, a refusing function prints no value while the run still reports, a built-in name aborts at the call | `test/source/harness_custom_stats_test.cpp` |
| C-6 aggregate-only | each mode hides repetition rows from the console and keeps the aggregate; a single-run row never disappears | `test/source/harness_aggregates_only_test.cpp` |
| C-7 complexity | a scripted quadratic family reports its class, coefficient, and RMS for the time and for each named quantity; a fixed curve and a user function are honored; `AUTOMATIC` picks the planted curve; a one-instance family and a non-positive N state the named reason | `test/source/harness_complexity_test.cpp` |
| C-8 docs and leftovers | the six new sections, the quick-reference rows, the fixed fixture/capture/template snippets, the rewritten pause and unit texts, and the rewritten internal guide paragraphs; the drift gate passes | `tools/docs/example_drift.py`, prose gate, review |
| C-9 runnable examples | every section example runs and prints a result row per registered benchmark, no FAILED row stands, and every SKIPPED row matches an expected-skip marker of its file | `test/example_run.sh` |
| whole surface | every hard gate of Principle VIII at the feature head, the time-source gate included | SC-06 run of the CI matrix |

**Precondition violations** follow the established pattern: the scenario
re-execs the test binary, the `SG_REQUIRE` or `SG_ASSERT` ends that
process, and the parent reads the signal, as `harness_pause_test`
already does for its modes.

**Existing tests the changes touch**: `harness_pause_test.cpp` loses the
bound scenario and the `ninthPause` re-exec mode and gains the
every-iteration scenario (FR-024 to FR-026); `harness_time_unit_test.cpp`
inverts its captured-at-registration scenario to the resolution-at-run
rule (FR-027); `harness_statistics_test.cpp` and the report-shape checks
carry the new row fields while the built-in aggregate fields keep their
tokens; `harness_cli_test.cpp` and the list-mode checks stay green
untouched, since no new option lands (FR-029).

**Registration pattern**: each new test follows the existing block of
`test/CMakeLists.txt`: a `harness_<area>_test` executable linked to
`speedgun-ng::harness`, `cxx_std_23`, CTest name equal to the target
name. The complexity and counters tests read internal records, so they
carry `target_include_directories(... ${CMAKE_SOURCE_DIR}/source/harness)`
as the calibration tests do. `example_run.sh` joins the gate block as a
script test over the `run_` targets.

**Gates that must stay green**: `loop_shape`, `cli_shape`,
`barrier_shape`, `time_source_gate`, `dbc-gate`, `prose-lint`,
`spell-check`, `format-check`, the name check, `ci-sanitize`, `ci-tsan`,
the coverage gates, `example_drift`, `example_drift_fixtures`,
`snippet_compile`, `report_shape`, `report_shape_fixtures`, and the new
`example_run`. The coverage target keeps measuring 100% line and branch
over `include/speedgun-ng/*` and `source/*`, and it omits the contract
macro lines (the contract-macro exclusion of `cmake/coverage.cmake`),
so every new branch needs a
test while its contract lines stay free (FR-047).

**Determinism**: scripted clock points drive every time figure; the fit
tests plant values the closed form reproduces exactly (pure curves with
exact coefficients give RMS 0.0), and the aggregate and statistic
assertions compare exact doubles on scripted inputs.

**Deliberately untouched**: threads and per-thread aggregation, JSON and
CSV reports, the raw-data report, the full command line, percentile
histograms, and reloaded pass-or-fail judgements. Each is a roadmap spec
in the spec Out list, and X.2 keeps it out.

## Constitution Check

*(Post-design pass: does the design as written honor I to XI?)*

| Principle | Verdict and note |
| --- | --- |
| I | PASS. `std::array`, `std::string_view`, `std::function`, and `std::optional` carry every new structure; no compiler extension enters. |
| II | PASS. The cap, the name bound, the newline bound, the statistic-name bound, and the flag-agreement rule each stand as an enforced clause beside the doxygen clause that states it; the pair gate finds one enforced kind per documented kind. |
| III | PASS. The logical views carry the contracts, and the physical view names the file, namespace, and target of each piece. |
| IV | PASS. The header blocks state each contract once, and the six guide sections plus the rewritten texts state the user-facing rule per capability (FR-028, FR-031, FR-039, FR-041). |
| V | PASS. N-4 covers the enumerator spellings, N-2 the new functions, N-8 the two constants, and N-1 the `Counter` class name; no acronym sits inside a name unexpanded. |
| VI | PASS. Eight new executables plus the new gate script cover the flag branches, the disclosure branches, the mode branches, the fit branches, and the abort paths; no `LCOV_EXCL` enters `source/harness/` (FR-047). |
| VII | PASS. The loop keeps its instruction count: `setCounter` is body code the benchmark calls, and the flag pass, the statistics, and the fit all run after the loop (R-02, R-07). |
| VIII | PASS. The only gate change is the requested strengthening of IV.1 and Principle VIII (R-14); the version table records the `SOVERSION` rise FR-048 requires. |
| IX | PASS. Every artifact of R-DCUT lives in the feature directory, and this plan records TDD mode. |
| X | PASS. One new source file, one new public class, and no abstraction beyond the spec's own nouns; the four items needing justification stand in Complexity Tracking. |
| XI | PASS. The artifacts pass the prose and spell gates at generation; the internal guide paragraphs become reader sentences (FR-031). |

## Complexity Tracking

| Item | Principle | Why it is needed | Simpler alternative rejected |
| --- | --- | --- | --- |
| The counter store is a fixed sixteen-slot array with 32-byte name buffers | X.1 | FR-001 and Q-2 require a bounded store that opens names inside the timed loop without allocation; a fixed array with inline name bytes is the only shape where `setCounter` inside the loop is two stores and a scan. | A heap map keyed by name, the cited revision's shape: rejected because every new name allocates on the timed path, which the loop discipline of FR-007 forbids. |
| Pause room reserved from observed history, capped per leaf column, folded beyond the cap | X.1 | FR-025 and FR-051 to FR-052: an iterations-proportional reservation is unbounded memory at `kIterationCap`; history sizing reserves what the body takes, the cap costs at most 256 KiB per leaf column, and the fold nets identical columns with no retention. | A recorder that grows when a run overruns: rejected because the growth would allocate while the timed loop is open, exactly the pause-removal's own hazard; an unbounded `2 + 2 × iterations` reservation: rejected at plan review for the same memory bound. |
| Fit rows travel as one synthetic fit result per family | X.2 | FR-022 reports a family-level fit, while the report consumes per-instance results; one synthetic `BenchmarkResult` reuses the whole row pipeline for the two rows per quantity. | A second collection of fit objects beside the results: rejected because report, filter, and future file output would each need a second consumer path for one row type. |
| One example binary per main-page section (eighteen targets) | VIII | FR-032 to FR-035 require every section's snippet to build and run in CI; a file per section keeps each region in the exact shape the page copies. | A single snippets binary: kept as the Q-9 fallback and rejected here because a pasted fragment from a merged file cannot show the canonical includes and `main` the reader must add. |
