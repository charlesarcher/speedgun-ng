# Tasks: Harness User Counters and Statistics

**Input**: Design documents from `specs/019-harness-user-counters-and-statistics/`

**Prerequisites**: plan.md, spec.md, research.md (D-01..D-14),
data-model.md (E-01..E-08), contracts/ (five files), quickstart.md

**Tests**: Included. FR-050 fixes TDD mode: each story's test file is
written first and observed failing before its implementation tasks.

**Organization**: Tasks are grouped by user story; the H3 leftover
fixes (Q-7) form the first working phase, before the counter work that
depends on them. Every task carries exact file paths; constraints from
data-model.md and contracts/ are quoted so no bound is left to
implementation-time discretion.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: User story ownership (US1..US8 from spec.md)
- Setup and Polish tasks carry no story label

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: The version effect the feature head must carry from the
first build (FR-048, plan.md version table).

- [X] T001 In `CMakeLists.txt`, raise `project(speedgun-ng VERSION)` from 0.8.0 to 0.9.0 and the `speedgun-ng_harness` archive `SOVERSION` from 4 to 5; the `speedgun-ng` archive `SOVERSION` stays 2 (FR-048)

**Checkpoint**: `cmake --preset=ci-rocky && cmake --build build`
configures and builds at 0.9.0.

## Phase 2: H3 leftover fixes (Q-7)

**Purpose**: User Story 3, the two promises the audit point shows as
false. Q-7 fixes them before any feature work, so this phase precedes
Foundational and every story that cites the fixed behavior.

**Goal**: The per-run pause allowance dies, the recorder bound follows
the loop, and the default unit resolves when the run starts, making the
shipped main-page sentences true (spec US3, PC-3, PC-4).

**Independent Test**: "run a calibrated benchmark that pauses in every
iteration on the fake provider and read its completed row; call
`setDefaultTimeUnit` at the top of `main` before `speedgunMain` and
check a `SG_BENCHMARK`-registered benchmark that calls no `unit` prints
in the new unit" (spec.md US3).

### Tests for the fixes (rewrite first, observe failure)

- [X] T002 [P] [US3] Rewrite `test/source/harness_pause_test.cpp`: delete the allowance-bound scenarios and the `ninthPause` re-exec mode; add a scenario whose body closes a `ScopedPauseTiming` in every iteration under scripted calibration and completes with every column netted; add a scenario with two closed pairs per iteration over a ten-thousand-iteration calibrated run that completes with the reservation at or below `kMaxPauseSamplesPerLeaf` points per leaf column (FR-051, SC-11); add a non-pausing scenario asserting the reservation carries no pause room (FR-053, SC-12); add a scenario checking the fold path's netted columns equal the raw path's for the same scripted pauses (FR-052, SC-13); assert the source tree contains no `kMaxPausePerRun` constant or guard (FR-024, FR-025, FR-026)

- [X] T003 [P] [US3] Rewrite `test/source/harness_time_unit_test.cpp`: `setDefaultTimeUnit(TimeUnit::MICROSECONDS)` called in `main` before `speedgunMain` reaches macro registrations that call no `unit`; a `unit(TimeUnit::SECONDS)` call still wins; the test-hook path without `speedgunMain` reads the process default at run start (FR-027)

### Implementation for the fixes

- [X] T004 [US3] Delete `kMaxPausePerRun` from `include/speedgun-ng/benchmark.hpp` together with its `SG_REQUIRE` guard in `State::pauseTiming` (the same header) and its recorder capacity term, the capacity argument of the `plan->recorder` call in `Runner::run` in `source/harness/runner.cpp`; add `kMaxPauseSamplesPerLeaf = 32'768` beside `kIterationCap` in `source/harness/detail/calibration.hpp`; size each leaf column's recorder at plan build between attempts from the endpoint shape plus, from a family's second attempt, the pause room its previous attempt observed (two points per closed pair per iteration), capped at the constant so "the pause room of any reservation shall not exceed `kMaxPauseSamplesPerLeaf` raw points per leaf column"; without history or over the cap, fold each closed pause pair into the row's running totals at the pair's commit from the same two raw points and set `BenchmarkResult::pauseSamplesFolded`; an open pause at loop exit stays the existing contract violation (FR-024 to FR-026, FR-051 to FR-054, research D-10, Q-10)

- [X] T005 [US3] Change `RegistryEntry::timeUnit` to `std::optional<TimeUnit>` in `source/harness/detail/internal.hpp`; delete the registration-time capture in `addRegistryEntry` (`source/harness/registry.cpp`); snapshot `getDefaultTimeUnit()` into the batch at `speedgunMain` entry; resolve each result's unit as the entry value, else the batch snapshot, else the process default read at run start (FR-027, research D-11)

- [X] T006 [US3] In the same commit range as T004 and T005, update the sentences the fixes make stale: the pause-allowance sentence in the README "Pause and resume" section, the "Pause and resume" texts in `docs/pages/harness.md` that name the constant and its bound, and that page's re-exec mode list naming the ninth-pause scenario; the captured-at-registration unit text in the README "The time unit" section and the `docs/pages/harness.md` "Time units" paragraph; the `BenchmarkResult::timeUnit` doxygen block in `include/speedgun-ng/benchmark.hpp`; and state the fold fallback beside the pause-per-iteration rule in the "Pause and resume" section of `docs/pages/harness.md` and beside the fold-over-the-raw-point-pair sentence in its "Two time columns" section (FR-039, FR-041, FR-042, FR-054, contracts/run-fixes.md)

**Checkpoint**: A pause-per-iteration calibrated run completes and the
main-page unit promise holds; `ctest -R 'harness_pause_test|harness_time_unit_test'`
passes.

## Phase 3: Foundational (Blocking Prerequisites)

**Purpose**: The counter types, the fixed store, and the result-model
extensions every P1 story reads.

**CRITICAL**: No counter-story work begins until this phase is
complete; the Phase 2 H3 fixes stand deliberately apart from it.

- [X] T007 In `include/speedgun-ng/benchmark.hpp`, add the public counter vocabulary with doxygen blocks: `enum class CounterFlag : std::uint32_t { NONE = 0, RATE = 1u << 0, AVG_THREADS = 1u << 1, ITERATION_INVARIANT = 1u << 2, AVG_ITERATIONS = 1u << 3, INVERT = 1u << 31 }` with a `constexpr operator|`, `enum class CounterBase : std::uint8_t { K1000, K1024 }`, class `Counter` (value, flags, base; defaults `0.0`, `NONE`, `K1000`), `inline constexpr std::size_t kMaxUserCounters = 16`, and `inline constexpr std::size_t kMaxCounterNameBytes = 32` (E-02, FR-001, FR-002, research D-01)

- [X] T008 In `source/harness/runner.cpp` (state detail) implement `State::setCounter(std::string_view, const Counter&)` over a `std::array<CounterSlot, kMaxUserCounters>` of fixed slots with inline 32-byte name buffers, opened inside the timed loop with no allocation. Enforce, with reasons that state the bound: "A benchmark shall carry at most `kMaxUserCounters` (16) named counters; a further name fails the run with a reason stating the cap", "A name longer than `kMaxCounterNameBytes` minus one shall fail the run with a reason stating the bound", "A second `setCounter` for a used name shall carry equal flags and base; a difference fails the run naming that name", and the metric-label collision rule "A counter name equal to an attached metric label shall fail the run naming the collision" (contracts/user-counters.md, FR-001, FR-007, FR-008, spec Edge Cases; depends on T007)

- [X] T009 In `include/speedgun-ng/benchmark.hpp`, extend the result model: `RowKind` gains `BIG_O` and `RMS`; `ResultRow` gains `std::string label` and `std::vector<double> counters`; `BenchmarkResult` gains `std::vector<std::string> counterLabels`, `std::vector<Statistics> counterStats`, `bool hideRepetitionRows`, and `bool pauseSamplesFolded`; no shipped field changes type (E-07, FR-006, FR-011, FR-022, FR-046, FR-054)

**Checkpoint**: The header types compile and the store opens names on a
scripted run; user stories can proceed in parallel.

## Phase 4: User Story 1 - Report user counters with flags (Priority: P1) - MVP

**Goal**: A body sets named counters inside the timed loop; the runner
applies the flags after the loop in the cited `Finish` order and the
report prints one column per counter plus per-counter aggregate
statistics (spec US1).

**Independent Test**: "register a benchmark that sets two named counters
on a scripted provider whose primary time is a fixed per-iteration
value, run it, and read the printed columns against the values the
cited `Finish` order produces for each flag, with the inverted form
last" (spec.md US1).

### Tests for User Story 1 (write first, observe failure)

- [X] T010 [P] [US1] Create `test/source/harness_user_counters_test.cpp` on the fake provider: flag-neutral column; rate over the primary time; iteration-invariant times iterations before the rate division; average-over-iterations divided by the count; invert applied last; combination flags in one run; re-exec modes for the seventeenth-name cap and the flag-change violation. Register `harness_user_counters_test` in `test/CMakeLists.txt` with the `${CMAKE_SOURCE_DIR}/source/harness` include directory and a CTest name equal to the target (contracts/user-counters.md, FR-001 to FR-008, SC-01, SC-03)

### Implementation for User Story 1

- [X] T011 [US1] In `source/harness/runner.cpp`, add the once-per-repetition flag pass after the loop, in the order rate, thread average, iteration-invariant, average-over-iterations, invert: "The rate divisor shall be the row's primary time in seconds: the CPU fold, the real fold under `useRealTime`, the manual sum under `useManualTime`"; "The thread-average divisor shall be one while threads are out of scope"; on a zero divisor or gap disclosure "the row prints the counter's flag-neutral value and a disclosure naming the zero divisor, never an infinity" (FR-003, FR-004, FR-005, research D-02, D-03)

- [X] T012 [US1] In `source/harness/report.cpp`, print counter columns after the metric fields in `counterLabels` order, "each printed as the flag-applied value divided by its base", with the disclosure token beside each counter column of a disclosed row (contracts/report-shape.md, FR-005, FR-006; depends on T011)

- [X] T013 [US1] In `source/harness/runner.cpp`, fill `BenchmarkResult::counterLabels` (name order fixed at the first row built) and `counterStats` with one `Statistics` per named counter "over the clean repetitions", excluding disclosed rows (E-07, FR-006; depends on T011)

- [X] T014 [P] [US1] Extend the report-shape gate to the counter columns: teach the comparator `tools/docs/report_shape.py` (registered in `test/CMakeLists.txt`) and its fixture runner `test/example-drift-fixture/run_report_shape_fixtures.py` so counter columns pass while the built-in fields keep their exact bytes; the README sample report block keeps its exact bytes, because its example registers no counter and the comparator must still pass the block untouched (contracts/report-shape.md, FR-006, SC-02)

- [X] T015 [P] [US1] Extend `test/loop_shape.sh` with a compiled body that calls `state.setCounter` inside `for (auto _ : state)` and keep the gate green: no allocation call and no out-of-line call inside the backward jump beyond the body's own call (FR-007)

**Checkpoint**: US1 prints correct columns for every flag on scripted
clocks; `ctest -R harness_user_counters_test` passes.

## Phase 5: User Story 2 - Read throughput as bytes and items per second (Priority: P1)

**Goal**: `setBytesProcessed` and `setItemsProcessed` open the two
preset rate counters and print base-scaled rates over the primary time
in all three timing modes (spec US2).

**Independent Test**: "set `n` bytes per iteration on a scripted run
whose primary time is fixed, and check the printed rate equals
`n x iterations / primarySeconds`, scaled by the base the cited
`SetBytesProcessed` chooses" (spec.md US2).

### Tests for User Story 2 (write first, observe failure)

- [X] T016 [P] [US2] Create `test/source/harness_throughput_test.cpp`: the bytes rate over the scripted primary time; a `useRealTime` family dividing by the real column; a manual-time family reporting 2000 ns per iteration and 500 items dividing by the manual sum; the 1024 and 1000 base scaling on the printed value. Register `harness_throughput_test` in `test/CMakeLists.txt` (contracts/user-counters.md, FR-009, FR-010)

### Implementation for User Story 2

- [X] T017 [US2] Add `State::setBytesProcessed(std::int64_t)` and `State::setItemsProcessed(std::int64_t)` to `include/speedgun-ng/benchmark.hpp` and their definitions on the runner store: `bytes_per_second` with the `RATE` flag and the `K1024` base, `items_per_second` with the `RATE` flag and the `K1000` base, each setting the value the body passes (E-03, FR-009, FR-010; depends on T008)

**Checkpoint**: US2 prints exact rates in all three timing modes;
`ctest -R harness_throughput_test` passes.

## Phase 6: User Story 4 - Mark a row with a label (Priority: P2)

**Goal**: `setLabel` writes one string per row printed as the trailing
` label=<text>` field, with the cited shared-label rule for the
aggregate (spec US4).

**Independent Test**: "set a label during the run of one scripted
benchmark and check the printed row ends with its label in the trailing
field; run repetitions whose labels disagree and check the aggregate
line carries no label" (spec.md US4).

### Tests for User Story 4 (write first, observe failure)

- [X] T018 [P] [US4] Create `test/source/harness_label_test.cpp`: the row ends with ` label=avx2`; the name column keeps its H1 bytes when no label stands; disagreeing repetition labels leave the aggregate unlabeled; a label containing a newline aborts the re-exec at `setLabel`. Register `harness_label_test` in `test/CMakeLists.txt` (contracts/statistics-and-labels.md, FR-011, FR-012, SC-03)

### Implementation for User Story 4

- [X] T019 [US4] Add `State::setLabel(std::string_view)` to `include/speedgun-ng/benchmark.hpp` and its definition on the state detail, with the precondition "A label containing a newline character is a precondition violation at the call" and last-write-wins within a repetition; copy the string to `ResultRow::label` at row build in `source/harness/runner.cpp` (E-04, FR-011, research D-04; depends on T009)

- [X] T020 [US4] In `source/harness/report.cpp`, print ` label=<text>` after the name column, separated by two spaces, present only when the row carries a label; keep the label on the aggregate only "when every repetition's label is equal"; extend the `tools/docs/report_shape.py` comparator with the field; the README sample report block keeps its exact bytes, its example registering no label (FR-011, FR-012, SC-02, contracts/report-shape.md; depends on T019)

**Checkpoint**: Labels print and the shared-label rule holds;
`ctest -R harness_label_test` passes.

## Phase 7: User Story 5 - Add custom statistics to the aggregate (Priority: P2)

**Goal**: `addStatistic` registers a (name, unit, function) triple; the
aggregate prints it among the built-in fields in the declared unit
(spec US5).

**Independent Test**: "add a statistic that returns the range of the
repetition values on a scripted three-repetition run and check the
printed value against the scripted times, in the declared unit; add a
percentage statistic and check it prints with a percent meaning and
without unit scaling" (spec.md US5).

### Tests for User Story 5 (write first, observe failure)

- [X] T021 [P] [US5] Create `test/source/harness_custom_stats_test.cpp`: a `range` statistic of `StatUnit::TIME` prints among `mean=` and `sd=` in the row unit; a `StatUnit::PERCENTAGE` statistic prints unscaled; a function returning false prints no value while the run still reports; a name equal to `mean`, `median`, `sd`, `cv`, `min`, or `max` aborts the re-exec at the call. Register `harness_custom_stats_test` in `test/CMakeLists.txt` (contracts/statistics-and-labels.md, FR-013, FR-014, FR-015, SC-03)

### Implementation for User Story 5

- [X] T022 [US5] Add `enum class StatUnit : std::uint8_t { TIME, PERCENTAGE }`, `using StatisticFunc = std::function<bool(const std::vector<double>& values, double& result)>`, and `BenchmarkHandle::addStatistic(std::string_view, StatUnit, const StatisticFunc&) -> BenchmarkHandle&` to `include/speedgun-ng/benchmark.hpp` and `source/harness/registry.cpp`, appending the triple to `RegistryEntry::statistics` in `source/harness/detail/internal.hpp` behind the standing precondition `m_entry != nullptr && !m_entry->runStarted` (FR-013, FR-046, research D-05)

- [X] T023 [US5] In `source/harness/runner.cpp`, compute each statistic over "the clean repetition values of the primary time, the same list the built-in `Statistics` consumes" after the built-in set is built, dropping a statistic whose function returns false; in `source/harness/report.cpp`, print the statistic fields after the built-ins in `addStatistic` order, `TIME` values in the row unit and `PERCENTAGE` values unscaled (FR-013, FR-014, FR-015; depends on T022)

**Checkpoint**: Custom statistics print beside the built-ins in the
declared unit; `ctest -R harness_custom_stats_test` passes.

## Phase 8: User Story 6 - Show only the aggregates (Priority: P2)

**Goal**: Two handle calls restrict the report of one family to
aggregate rows while a single-run row never disappears (spec US6).

**Independent Test**: "set each mode on a scripted three-repetition
family and check the output it governs: the first mode shows no
repetition rows, the second trims the console while the per-repetition
rows remain in the other output channel the mode is defined against
(today: with only the console alive, both modes trim the console)"
(spec.md US6).

### Tests for User Story 6 (write first, observe failure)

- [X] T024 [P] [US6] Create `test/source/harness_aggregates_only_test.cpp`: `reportAggregatesOnly` prints only the aggregate row for a three-repetition family; `displayAggregatesOnly` trims the console the same way while every stored row stays intact; a family with one run prints its single row under either mode. Register `harness_aggregates_only_test` in `test/CMakeLists.txt` (contracts/complexity-and-modes.md, FR-016, FR-017, FR-018)

### Implementation for User Story 6

- [X] T025 [US6] Add `BenchmarkHandle::reportAggregatesOnly()` and `BenchmarkHandle::displayAggregatesOnly()` to `include/speedgun-ng/benchmark.hpp` and `source/harness/registry.cpp`; store the two booleans on `RegistryEntry` in `source/harness/detail/internal.hpp` behind the standing setter precondition; set `BenchmarkResult::hideRepetitionRows` when either mode stands (E-07, FR-016, FR-017, FR-046, research D-06)

- [X] T026 [US6] In `source/harness/report.cpp`, skip the repetition rows of a result whose `hideRepetitionRows` stands "unless no aggregate row exists", so "Neither mode shall hide a row that has no aggregate: a single-run instance prints its one row under either mode" (FR-016, FR-017, FR-018; depends on T025)

**Checkpoint**: Both modes trim the console and single-run rows survive;
`ctest -R harness_aggregates_only_test` passes.

## Phase 9: User Story 7 - Fit the time against the problem size (Priority: P2)

**Goal**: One least-squares fit per family prints, per fitted quantity,
a BigO row and an RMS row: the primary time, each named user counter,
each attached metric (spec US7, clarification 2026-10-14).

**Independent Test**: "script a family whose per-iteration time is a
known quadratic in N and check the reported class, coefficient, and RMS
against the computed fit; check a user-supplied curve function is
honored" (spec.md US7).

### Tests for User Story 7 (write first, observe failure)

- [X] T027 [P] [US7] Create `test/source/harness_complexity_test.cpp`: a scripted quadratic family with `AUTOMATIC` reports the planted class, coefficient, and RMS for the time, for each named counter, and for each attached metric; a family fixed to the constant or N-cubed curve honors it whatever the RMS; a user-supplied `BigOFun` curve is honored; a one-instance family prints the reason naming the two-instance floor; an instance with a non-positive N prints the reason. Register `harness_complexity_test` in `test/CMakeLists.txt` with the `source/harness` include directory (contracts/complexity-and-modes.md, FR-019..FR-023, SC-04)

### Implementation for User Story 7

- [X] T028 [US7] Create `source/harness/detail/complexity.hpp` (curve table for `ONE`, `N`, `N_SQUARED`, `N_CUBED`, `LOG_N`, `N_LOG_N`) and `source/harness/complexity.cpp` with the closed-form fit: "The coefficient shall minimize the relative-error least squares with the closed form `k = sum(f(N)/y) / sum((f(N)/y)^2)`" and "The RMS shall be the root-mean relative error times 100, printed unscaled by the unit multiplier"; `AUTOMATIC` "shall try each curve and keep the least RMS, with the constant curve as the fallback"; the harness target's `GLOB_RECURSE` source collection in `CMakeLists.txt` collects the new file at the next configure (FR-021, research D-07)

- [X] T029 [US7] Add `State::setComplexityN(std::int64_t)` to `include/speedgun-ng/benchmark.hpp` and the state detail; add `enum class BigO : std::uint8_t { NONE, ONE, N, N_SQUARED, N_CUBED, LOG_N, N_LOG_N, AUTOMATIC }`, `using BigOFun = std::function<double(double n)>`, and both `BenchmarkHandle::complexity` overloads to the header and `source/harness/registry.cpp`, storing the selection on `RegistryEntry` behind the standing setter precondition (FR-019, FR-020, FR-046)

- [X] T030 [US7] In `source/harness/complexity.cpp` and `source/harness/runner.cpp`, assemble one fit per family after its instances report, per quantity (primary time, each named counter present in every instance, each attached metric), with "The instance N shall be the `setComplexityN` value when set, else the instance's range value"; carry the rows in one synthetic `BenchmarkResult` named after the family, pairs in the order time, counters in open order, metrics in attach order; a quantity missing from any instance ends that quantity's pair with the named reason (FR-022, FR-023, research D-08, D-09; depends on T028, T029, and on US1 for the counter quantities)

- [X] T031 [US7] In `source/harness/report.cpp`, print the fit pairs after the family's aggregate row in the exact shape `<family>  BIG_O  <quantity> <curve> (<coefficient>)` and `<family>  RMS    <quantity> <rms-percentage>%`, the time coefficient in the row unit and a counter or metric coefficient in that quantity's value (FR-022, contracts/report-shape.md; depends on T030)

**Checkpoint**: A swept scripted family reports class, coefficient, and
RMS per quantity; `ctest -R harness_complexity_test` passes.

## Phase 10: User Story 8 - Read the page, copy the block, run the example (Priority: P3)

**Goal**: Eighteen runnable example files own every main-page snippet,
six new sections teach the new capabilities, and a CTest gate runs
every example (spec US8).

**Independent Test**: "for every main-page section, old and new,
extract its region, run the example that owns it, and check every
registered benchmark in that file prints a result row with no
`FAILED` row; every skipped row matches an expected-skip marker of
the example file; check the drift gate passes over the
page" (spec.md US8).

### Implementation for User Story 8

- [X] T032 [US8] Create the six feature example files `example/docs/user_counters.cpp`, `example/docs/throughput.cpp`, `example/docs/labels.cpp`, `example/docs/custom_statistics.cpp`, `example/docs/aggregates_only.cpp`, and `example/docs/complexity_fit.cpp`, each a complete program with the canonical includes and a `main` calling `speedgunMain`, each carrying the marked region comment the page copies (FR-032, FR-033, FR-049)

- [X] T033 [US8] Create the twelve D1-section example files under `example/docs/`, named for the `SECTIONS` titles of `tools/docs/example_drift.py` in snake_case: `run_loop.cpp`, `registration.cpp`, `argument_families.cpp`, `fixtures.cpp`, `run_control.cpp`, `timing_modes.cpp`, `pause_and_resume.cpp`, `time_unit.cpp`, `counter_metrics.cpp`, `skips.cpp`, `command_line.cpp`, `reading_the_report.cpp`; move each marked region of `example/docs_snippets.cpp` verbatim into the file for its section, wrapped in a complete program; apply the C-8 fixes inside the example files: the fixtures region gains the `TableFixture` with its `setUp` and `tearDown` overrides and a timed loop in every method it registers, the registration file carries an `SG_BENCHMARK_CAPTURE` region and an `SG_BENCHMARK_TEMPLATE` region, and the "Pause and resume" region drops the pause-limit text (T006 rewrites the README sentences); the "Skips" file registers one instance that runs and prints a row beside the expected skip instance, and carries the `// example-run: expect-skip <instance>: <message>` marker beside the registration that skips (FR-032, FR-037, FR-038, research D-12, D-15)

- [X] T034 [US8] In `example/CMakeLists.txt`, add the sibling helper `add_docs_example(NAME)`: it builds `docs/${NAME}.cpp` as target `docs_${NAME}`, links `speedgun-ng::speedgun-ng` and `speedgun-ng::harness`, requires `cxx_std_23`, and registers `run_docs_${NAME}` on `run-examples` (research D-12); wire all eighteen files through it and delete `example/docs_snippets.cpp` together with its compile-only `add_executable` block (FR-033; depends on T032, T033)

- [X] T035 [US8] Create `test/example_run.sh`: run every `docs_` target of `example/docs/` through its `run_docs_` target with `--min-time=0.01`, "fails when a registered benchmark of a file prints no result row, or prints `FAILED`"; collect each file's `// example-run: expect-skip <instance>: <message>` comment markers and pair them one-to-one with the printed `SKIPPED` rows by instance name and message, so an undeclared skip fails, a declared matching skip passes, and an unprinted expectation fails; add the fixture cases for the four outcomes and register the `example_run` CTest test in `test/CMakeLists.txt` (FR-034, FR-035, SC-10, SC-14, research D-12, D-15; depends on T034)

- [X] T036 [P] [US8] In `README.md`, add the six feature sections in the D1 shape (one purpose sentence, one snippet region, one result sentence); add the Quick reference row per new API name (FR-029); update the "Reading the report" section to show the counter columns, the label field, the custom statistic fields, and the per-quantity fit rows (FR-030); the sample report block inside that section keeps its exact bytes, the new columns and rows told in the sentences around it; copy the fixture, capture, template, and pause regions T033 produced: this task copies region bytes and edits no snippet text on its own, the example file owns each snippet and the drift check enforces it; and drop the six shipped entries from the planned-work list while it keeps structured machine-readable output and histogram output (FR-028, FR-030, FR-037, FR-038, FR-040, FR-042, FR-043, FR-049; depends on T033 and on the features of the sections they teach)

- [X] T037 [P] [US8] In `docs/pages/harness.md`, add the detail section per new capability, and rewrite the internal "Coverage:" paragraphs under the "Two time columns", "Real time as the primary time", "Manual time", "Pause and resume", and "Time units" sections as reader-facing sentences (FR-028, FR-031, Q-11)

- [X] T038 [US8] In `tools/docs/example_drift.py`, add the six new titles to `SECTIONS`, raise `PAGE_LINE_CAP` from 400 to 550 while `SECTION_LINE_CAP` holds at 25, and add the check that each page snippet region is a marked region of the example file named for its section; update the fixtures under `test/` for the drift tool; the budget change stands recorded by FR-044 in this spec, and no file under `specs/018-docs-main-page/` is edited (FR-044, SC-10, research D-13; depends on T032..T037)

- [X] T039 [US8] Spell out the counter-store bounds where the reader meets them: the README counter section gains the result sentence naming the `kMaxUserCounters` cap of sixteen names and the `kMaxCounterNameBytes` bound of that constant minus one characters, both tripped as `SG_REQUIRE` preconditions on `State::setCounter`; carry the same detail in the counter detail of `docs/pages/harness.md` (FR-001, FR-031, data-model E-01, contracts/user-counters.md; depends on T033, T036)

**Checkpoint**: `ctest -R example_run` passes and the drift gate covers
the eighteen owners.

## Phase 11: Polish & Cross-Cutting Concerns

- [X] T040 Amend `.specify/memory/constitution.md`: Principle IV gains IV.1's compiles-and-runs wording ("A main-page snippet shall come from an example CI builds and runs"), Principle VIII names the `example_run` check in its gate list, and the file records version 2.20.0 with the amendment record row, the lineage row, the footer, and the sync impact line, the format the 2.19.0 record shows (FR-036, research D-14)

- [X] T041 [P] Run quickstart.md sections 1 through 4 from the repository root (build, the two fixes, the six new executables, `example_run`) and record the results (FR-034, FR-035, SC-01..SC-05, SC-08, SC-09)

- [ ] T042 [P] Run quickstart.md sections 5 through 7 (the full CTest suite, which carries the name check behind FR-045, `example_drift.py`, the prose gate, the constitution grep) and record the results (SC-06, SC-07)

- [X] T043 Build the `ci-sanitize` and `ci-tsan` presets from `CMakePresets.json` (`cmake --preset=ci-sanitize`, `cmake --preset=ci-tsan`), run `ctest --test-dir build` on each, and record the results (FR-050, SC-06, Principle VIII)

- [ ] T044 Build the `ci-coverage` preset from `CMakePresets.json`, run `ctest --test-dir build`, and confirm from the `cmake/coverage.cmake` report that "The coverage report keeps 100% line and branch over `include/speedgun-ng/*` and `source/*` with no `LCOV_EXCL` marker under `source/harness/`" (FR-047, SC-06)

- [X] T045 Main-page impact check (IV.1): confirm the six new README sections, their owning example files, and the region-ownership gate all shipped in the same change as the code they teach (FR-028, FR-036)

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: no dependencies
- **H3 fixes (Phase 2)**: depends on Setup only; every later phase
  depends on this phase, and it lands first per Q-7
- **Foundational (Phase 3)**: depends on Setup and on the H3 fixes
  phase; blocks the counter stories
- **User Stories (Phases 4 to 10)**: depend on the H3 fixes phase and
  on Foundational
- **Polish (Phase 11)**: depends on the stories being delivered, and
  through them on the H3 fixes phase

### User Story Dependencies

- **Every story**: depends on the H3 fixes phase (Phase 2), which
  lands first per Q-7; the bullets below state only the
  dependencies among the stories themselves
- **US3 (P1)**: lands first (Phase 2), independent of US1 and US2;
  blocks the shipped counter examples of US8 per Q-7 (pause per
  iteration, default unit)
- **US1 (P1)**: needs only Foundational
- **US2 (P1)**: needs Foundational (T008's store); rides US1's flag
  pass for its rate values, so schedule after US1's T011
- **US4 (P2)**: needs T009's `ResultRow::label`
- **US5 (P2)**: independent of US1, US2, US4
- **US6 (P2)**: independent; reads only the row kinds already shipped
- **US7 (P2)**: the time-quantity fit is independent; the
  counter-quantity rows require US1 (T011, T013)
- **US8 (P3)**: the six feature sections document US1 to US7 and the
  examples run them; the pause and unit sections need the Phase 2 fixes

### Within Each User Story

- Tests are written first and observed failing (FR-050, TDD mode)
- Store and header types before report printing
- A story is complete when its CTest executable passes and the
  existing gates named in plan.md stay green

### Parallel Opportunities

- T002 and T003 are different test files and can land together right
  after Setup
- T010, T014, T015 within US1 touch three different files
- T018, T021, T024, T027 are four separate test files for four separate
  stories and can all be written once Foundational is done
- T036 and T037 are README and guide, different files
- T041 and T042 split the quickstart walkthrough
- After Foundational, whole stories run in parallel across developers;
  within a story the test task and the independent-shape tasks hold the
  [P] marks above

## Parallel Example: User Story 1

```bash
# After Foundational, launch together (different files):
Task: "Create test/source/harness_user_counters_test.cpp (T010)"
# After T011 lands, launch together (different files):
Task: "Teach the report-shape comparator and fixtures the counter columns (T014)"
Task: "Extend test/loop_shape.sh with a setCounter body (T015)"
```

## Implementation Strategy

### MVP First (User Story 1)

1. Complete Phase 1 (T001) and the H3 fixes of Phase 2 (T002..T006),
   the order Q-7 demands.
2. Complete Phase 3, Foundational (T007..T009).
3. Complete Phase 4 (T010..T015) and validate independently: this is
   the MVP, the counter surface the whole feature rides.
4. Note for the first shipped docs: because Phase 2 already carries the
   Q-7 precondition, a shippable documentation milestone needs US1,
   US2, and Phase 2 together.

### Incremental Delivery

1. Phase 2: the two H3 promises become true and their stale texts die.
2. Foundational, then US1: counters print columns (MVP).
3. US2: throughput presets ride the same path.
4. US4, US5, US6: label, statistics, modes, each one CTest executable.
5. US7: the family fit prints per-quantity rows.
6. US8: the page, the eighteen examples, the `example_run` gate.
7. Polish: the constitution amendment, the sanitizer, thread-sanitizer,
   and coverage matrices, the quickstart walkthrough.

## Notes

- [P] tasks touch different files with no dependency on an open task
- Each story's checkpoint is an independent validation point
- Every bound quoted above is a testable precondition; the re-exec
  pattern (plan.md Test Plan) holds the abort cases
- Commit cadence follows the repository rule (`<Section>: <one-line
  imperative>`), one atomic commit per task group or finer

## Phase 12: Convergence

Appended by the converge pass over pull request #37 on 2026-10-14. Each item
traces to a reviewed defect (D-n) or nit (N-n) on the branch head, and the
gap type is the relation between the current code and the artifact that owns
it. Q-10 is unchanged: the decision line still carries both choices, so the
spec entry stays a decision for Charles.

- [X] T046 Enforce the documented pre and post clauses of State::setBytesProcessed and State::setItemsProcessed with SG_REQUIRE and SG_ENSURE in the definitions in source/harness/runner.cpp, and report 0 drifted interfaces from the pair gate per D-1, Constitution II (contradicts)
- [X] T047 Stop dividing counter values by the counter base in counterColumns in source/harness/runner.cpp, so a stored value prints as stored and CounterBase selects only the human-readable suffix per D-3, FR-006 (contradicts)
- [X] T048 Replace the relative fit in fitCurve and relativeSums in source/harness/complexity.cpp with the least-squares form: coefficient equal to the sum of t times f(N) over the sum of f(N) squared, and RMS equal to the root of the mean squared residual over the mean of t, with the least-RMS curve kept and the constant curve as the fallback per D-5, FR-021 (contradicts)
- [X] T049 Rewrite FR-006, data-model.md, contracts/user-counters.md, the CounterBase doxygen, the README quick reference row, and the harness.md User counters and Throughput sections to the suffix-only base, removing the KiB per second claim per D-3 (partial)
- [X] T050 Rewrite FR-021, research D-08, contracts/complexity-and-modes.md, and the harness.md complexity text for the least-squares coefficient and the RMS percentage per D-5 (partial)
- [X] T051 Replace the complexity tests with known-answer vectors: planted N, N squared, and N log N families with exact coefficients, plus one noisy vector whose coefficient and RMS are computed by hand from the formulas and recorded in the test per D-5 (partial)
- [X] T052 Move the setBytesProcessed and setItemsProcessed calls after the timed loop in example/docs/user_counters.cpp and example/docs/throughput.cpp, scale them by state.iterations(), copy the regions to README.md unchanged, and state that the calls take the total work of the run per D-4 (partial)
- [ ] T053 Add a test with a real timed loop of a known per-iteration count that checks the printed rate within a stated factor of count times iterations over the primary time, so a per-iteration value fails per D-4 (missing)
- [X] T054 Reserve state.m_pausePairs to half the pause room before the timed loop opens in the sampleRun lambda in source/harness/runner.cpp, so State::closePausePair never reallocates on the raw path per D-10 (partial)
- [ ] T055 Add the proof that no allocation happens inside the timed loop on both pause paths, as a test/loop_shape.sh case or an allocation-counting hook, and record that the fold-path slot reset copies a plain recorder view per D-10 (missing)
- [ ] T056 Make the command line section of README.md quote example/docs/command_line.cpp, keep the commands in a shell block marked from a region that runs that example binary, and remove the exemption in tools/docs/example_drift.py per D-8 (partial)
- [ ] T057 Make the reading the report section quote example/docs/reading_the_report.cpp, extend that example to print a counter column, a label, one custom statistic and a BIG_O and RMS pair, add a sample output block checked by report_shape as a second sample check, name each new field in the result sentence, and remove the exemption per D-8, FR-030 (partial)
- [ ] T058 Apply ITERATION_INVARIANT and AVG_ITERATIONS even when the primary time is zero, skipping only the time divisor, and add the test that shows it per N-3, FR-005 (contradicts)
- [X] T059 Compare the skip message as well as the instance name in test/example_run.sh and add the mismatching-message case to test/run_example_run_fixtures.sh per N-1 (partial)
- [ ] T060 Show the SG_BENCHMARK_F and the SG_BENCHMARK_DEFINE_F with SG_BENCHMARK_REGISTER_F methods, with their timed loops, beside the fixture class in README.md, and remove the empty body and FAILED sentence per D-6 (partial)
- [ ] T061 Add the capture and template regions of example/docs/registration.cpp to README.md as marked blocks and drop the link wording that replaced them per D-7 (missing)
- [X] T062 Rewrite each of the 21 Coverage paragraphs in docs/pages/harness.md as one reader-facing behaviour sentence or delete it, leaving no test file name, scenario name or re-exec mode on the page per D-9, Q-11, T037 (contradicts)
- [X] T063 Document combined flags built with the bitwise or operator in the CounterFlag doxygen and the harness.md User counters section, standing in for the rate and thread flag names, and add one test per N-2 (missing)
- [X] T064 Document that inverting a zero value keeps zero, where the reference implementation gives infinity, in the CounterFlag doxygen, FR-005 and harness.md per N-4 (missing)
- [X] T065 Record in research D-10 that pause history is held per family through RegistryEntry::pausePairsPerIteration, so a family whose instances pause differently reserves the largest observed room per N-5 (missing)
- [ ] T066 Keep README.md at 550 lines or fewer after the section fixes, trimming prose and never a snippet, and report the final count per N-6, FR-044 (partial)
- [ ] T067 Run quickstart.md sections 5 through 7, record the evidence, and check T042 only when it passes per D-2 (partial)
- [ ] T068 Build the ci-coverage preset, hold 100 percent line and branch coverage, keep zero LCOV_EXCL markers under source/harness, record the evidence, and check T044 only when it passes per D-2 (partial)
- [X] T069 Write /tmp/019-rebase-plan.txt listing every pushed commit that fails the commit lint with its hash, old subject, new subject and the footer it gains, without running the rebase or force-pushing, and hold it for Charles to approve per D-1 (contradicts)
- [ ] T070 Run the verification set on the new head: developer build with the full suite, the pair gate at 0 drift, the prose gate and codespell at 0 findings on changed files, the commit lint on new commits, the coverage, sanitize and tsan presets, the two hand spot checks on cells=64 and the throughput rate, and the CI result per item (missing)

## Convergence Evidence

Recorded as each convergence task passed on the branch head, with the
command that showed it.

- T046: `cmake --build build --target dbc-gate` prints `doc-gate: 211
  interfaces, 0 gaps` and `pair-gate: 211 interfaces, 0 gaps`. The
  clauses of both throughput setters are enforced by `SG_REQUIRE` before
  the write and `SG_ENSURE` after it in `source/harness/runner.cpp`. The
  abort path of the shared cap and the recorded-failure path of a name
  opened with another pair take their tests with T068, where the coverage
  report names the lines that still need them.
- T047: `harness_user_counters_test` and `harness_throughput_test` pass
  after the base division left `counterColumns`; a counter of 64 with no
  flag prints 64 under `K1000` and under `K1024`, and the throughput
  columns print the count over the primary time alone. FR-006, the data
  model, the counter contract, the plan row, the doxygen and both pages
  state the suffix-only base.
- T054: the attempt reserves half its pause room as pair slots before the
  loop opens in `sampleRun`; `harness_pause_test`, `loop_shape`,
  `barrier_shape` and both counter tests pass on that tree. The
  allocation proof for both pause paths stays open as T055.
- T058: the flag pass applies the iteration flags when the primary time
  is zero and skips only the time divisor; the test that pins that
  behaviour is still owed, so the box stays open.
- T048, T050, T051 (`1d1f7f8`): `leastSquaresSums` and `fitCurve` in
  `source/harness/complexity.cpp` now state `k = sum(t * f(N)) /
  sum(f(N)^2)` and the root-mean residual over the mean of the values;
  `harness_complexity_test` carries the planted `N^2`, `N lg N` and
  constant vectors, the fixed `N^3` and constant curves over the
  quadratic series, and the noisy family over 10, 40 and 90 whose
  `k = 75/7` and RMS 15.509 % are computed by hand in the file. FR-021,
  the assumption row, the two contract rows, research D-07 and the
  harness page state the same closed form. The suite on that tree
  passes 86 of 88, the exceptions being the host `counters_pmu_test`
  PMU denial and the doc gate the reverted T060 change caused.
- T049 (`b37a55b`): FR-006, `data-model.md`, `contracts/user-counters.md`,
  the plan row and the doxygen of `CounterBase` and both throughput
  setters say the base selects a suffix and never divides.
- T052 (`6596f90`): `example/docs/user_counters.cpp` and
  `example/docs/throughput.cpp` total after the loop with
  `state.iterations()`; the README blocks are restated from the
  regions, the throughput page states the totals rule, and
  `example_drift`, `snippet_compile`, `example_run` and
  `example_run_fixtures` pass on the new text.
- T062 (`09abb07`): no `Coverage:` paragraph remains in
  `docs/pages/harness.md` (twenty-one rewritten as behaviour prose,
  the page down 29 lines), with the prose gate at 0 findings.
- T063, T064, T065 (`120ee96`): FR-005 carries the combined flag set and
  the zero reciprocal, the harness page spells `operator|` combinations
  against the combined names a Google Benchmark port uses, and research
  D-10 names `RegistryEntry::pausePairsPerIteration` with the function
  that raises it.
- T069: `/tmp/019-rebase-plan.txt` lists every pushed commit that fails
  the commit lint with its old subject, its new subject and the footer
  it gains. No rebase was run and nothing was force-pushed: the plan
  waits on Charles.
- T059: the gate compares the declared instance and its message as one
  pair and names the half that disagreed; example/docs/skips.cpp registers
  a fixture method that skips its zero argument, so a name the fixture
  macros build is covered as well. Corrupting the declared phrase fails the
  gate and restoring it passes both example-run tests.
