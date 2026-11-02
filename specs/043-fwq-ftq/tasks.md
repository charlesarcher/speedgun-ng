# Tasks: Fixed Work Quanta and Fixed Time Quanta Probes

**Input**: Design documents from `/specs/043-fwq-ftq/`

**Prerequisites**: [plan.md](./plan.md) (required), [spec.md](./spec.md)
(required for user stories), [research.md](./research.md),
[data-model.md](./data-model.md), [contracts/](./contracts/),
[quickstart.md](./quickstart.md)

**Tests**: TDD mode (plan.md, Execution mode): the test task of each
wave stands written and observed failing before the code task that
turns it green. The test and its code land in one commit. A red stands
as a compile failure while a row names a symbol the tree lacks, and as a
run-time failure once that symbol stands there.

**Organization**: tasks stand grouped by user story behind the module
landing order plan.md fixes (M-A to M-I): the public surface and the
numbers stand first, the value grammar and the spectrum kernel land in
one parallel block, the probe body and the folds follow, the console
and the record follow, the docs and the evidence close, and the gates
close the lane. Every task names its binary verify: a ctest name with
an exit status, a gate command with its exit status, a grep with a
match count, a byte fold, or a schema shape.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: can run in parallel. Two [P] tasks touch disjoint files and
  share no build directory, no CMake target, no schema file, no README
  section, no docs page, no option table, no rule table, no version
  block and no coverage configuration. A chain that shares one of these
  stands sequential and carries no [P]
- **[Story]**: the user story this task serves (`US1` to `US7`)
- Every description names the exact file path, the requirements it
  closes, and the verify command

---

## Phase 1: Setup and Foundational - the public surface and the numbers (module M-A, wave 1)

**Purpose**: the types every story stands on, and the version record
the lane stands on. Nothing else builds before this phase closes.

- [x] T001 [P] [US4] Register the test executable `harness_fwq_ftq_test`
  in `test/CMakeLists.txt` after the `harness_jitter_preset_test` block (
  `:973-984`), in the shape of `:874-880`, then write the failing
  public-surface tests in `test/source/harness_fwq_ftq_test.cpp`:
  `JitterMethod` names `TSC`, `FWQ`, `FTQ`; `JitterProbeOptions`
  default-constructs with `JitterMethod::TSC`, `quantumUs` at 100,
  `workCount` at 0, the value naming the calibration of FR-010, and
  `noisePercent` at 1; a default `JitterEntryFold` leaves `series` empty;
  a default `ReportData` leaves `jitterRun` empty; the bound rows read 10
  to 100,000 us, 1 to 16,777,216 with 0 naming the calibration, and 0 to
  100 percent (FR-001, FR-002, FR-005, FR-020, R-20).
  Verify: `cmake --build build --target harness_fwq_ftq_test` exits non-zero with the new rows failing to compile (red observed as a compile failure), and `ctest --test-dir build -R harness_fwq_ftq_test` reports the entry as `Not Run`
- [x] T002 [P] [US4] Move the version rows in
  `test/source/harness_cli_table_test.cpp:1382-1393` from `0.28.0`,
  `SOVERSION 19` and `SOVERSION 5` to `0.29.0`, `SOVERSION 20` and
  `SOVERSION 5`, and keep the value-block rows green with no new row,
  because the keys ride the standing value 260 (
  `source/harness/detail/options.hpp:87`) (FR-005, FR-034, SC-009).
  Verify: `ctest --test-dir build -R harness_cli_table_test --output-on-failure` exits non-zero (red observed)
- [x] T003 Add the public surface in `include/speedgun-ng/benchmark.hpp`
  : `JitterMethod` after `JitterProbeMode` (`:2182-2187`); the bounds
  after `kDefaultPrecheckMs` (`:2195`), naming `kDefaultQuantumUs` 100,
  `kMinQuantumUs` 10, `kMaxQuantumUs` 100000, `kMaxWorkCount` 16777216,
  `kTargetUnitTimeNs` 1000, `kCalibrationTrials` 25, `kCalibrationSteps`
  24, `kDefaultNoisePercent` 1; `JitterPeriodRecord`,
  `JitterSeriesStats` and `JitterRunRecord` after `JitterStats` (
  `:512-530`); `series` in `JitterEntryFold` after `stats` (`:648`);
  `series` in `PrecheckCpuRecord` after `stats` (`:576`); `jitterRun` in
  `ReportData` after `graph` (`:1001`); the four fields in
  `JitterProbeOptions` after `tracerCpus` (`:2258`), `workCount` and
  `noisePercent` each a `std::uint32_t`; every new type and
  member carries its doxygen contract; the three bounds name the option
  parser of T007 as their enforcement site, and `runJitterProbe` carries
  no new documented precondition, because the parser refuses an
  out-of-range value at the command-line boundary (FR-001, FR-002, FR-003,
  FR-005, FR-007, FR-019, FR-020, II, D-1, D-7, R-20). This commit lands
  the `public-api.md` delta of
  `specs/027-jitter-probe/contracts/` (FR-030).
  (depends on T001, which writes the rows this task turns green)
  Verify: `ctest --test-dir build -R harness_fwq_ftq_test --output-on-failure` exits 0
- [x] T004 [US4] Move the numbers in `CMakeLists.txt`: the project
  `VERSION` at `:45` stands `0.29.0`; the harness `SOVERSION` at `:375`
  stands `20` with the comment naming the four layout moves, the new
  fields of `JitterProbeOptions`, the `series` of `JitterEntryFold`, the
  `series` of `PrecheckCpuRecord`, and the `jitterRun` of `ReportData`;
  the archive `SOVERSION` at `:110` stays `4`, because no public counters
  header moves; the commit message records the version check and the ABI
  check with the four moves named (FR-005, FR-006, D-8, SC-009). The
  schema check rides the T030 commit, the one that writes `1.12`.
  The three rows of `test/source/views_test.cpp:241-255` pin the
  same numbers and move in this commit. (depends on T002, T003)
  Verify: `ctest --test-dir build -R "harness_cli_table_test|views_test" --output-on-failure` exits 0, and `grep -c "SOVERSION 20" CMakeLists.txt` reads one line

**Checkpoint**: the surface stands green: the types carry the three
methods, and the version names the four layout moves.

---

## Phase 2: User Story 5 - the value grammar and the 16 refusals (Priority: P2) (module M-B, wave 1)

**Goal**: a `--jitter-probe` value with a method key settles
into the four option fields; every value of the error table
refuses one quoted line on standard error with exit status 2
before anything runs.

**Independent Test**: with each value of
[contracts/cli-options.md](./contracts/cli-options.md), the run
prints one quoted line naming the fault and exits 2 with no
benchmark output.

### Tests for User Story 5 (write first, observe failure)

- [x] T005 [P] [US5] Write the failing grammar tests in
  `test/source/harness_jitter_cli_test.cpp`, in the block at `:2660-2825`
  over `captureError` (`:265-301`) and `checkOneLine` (`:303`):
  `scenarioMethodWordAccepted`, `scenarioHeadStandsFirst`,
  `scenarioHeadCarriesEquals`, `scenarioModesRunEveryMethod`, and the
  range rows of `scenarioQuantumRange`, `scenarioWorkRange`,
  `scenarioNoiseRange` (FR-001, FR-002, FR-004, R-02).
  (depends on T003, which declares the four fields)
  Verify: `ctest --test-dir build -R harness_jitter_cli_test --output-on-failure` exits non-zero (red observed)
- [x] T006 [US5] Write the 16 refusal rows of the test plan in
  `test/source/harness_jitter_cli_test.cpp`, each checking one line and
  status 2: `scenarioMethodWordRefused` over `idle,method=x`;
  `quantum=0`; `quantum=x`; `quantum=100001`; `quantum=10.5`; `work=0`;
  `work=16777217`; `work=x`; `work=10.5`; `noise=101`; `noise=x`;
  `noise=0.5`; a `quantum` key with `method=fwq`; a `work` key with
  `method=ftq`; `method` standing twice; and the sixteenth row, the same
  value on an option file line naming the path, the line and the token.
  The three fractional rows close the fractional clause of FR-003, and
  every row stands covered at status 2 (FR-003, SC-005).
  (depends on T005, same file, behind a disjoint scenario block)
  Verify: `ctest --test-dir build -R harness_jitter_cli_test --output-on-failure` exits non-zero (red observed)
- [x] T007 [US5] Add the four key arms in
  `source/harness/jitter_probe.cpp` beside the `mode` arm (`:866-877`):
  `method`, `quantum`, `work`, `noise`, each read once on the standing
  number grammar of `parseProbeNumber` (`:698-719`), with the ranges of
  FR-002; add the cross-method pairing check and the repeated-key check
  beside the standing cpu-duplicate check (`:910-917`); a head token that
  carries `=` answers the standing cpu-token line (`:735-737`) (FR-001,
  FR-002, FR-003, D-1, R-02, R-03). This commit lands the `cli-options.md`
  delta of `specs/027-jitter-probe/contracts/`, the four keys and the head
  rule (FR-030).
  (depends on T005, T006; the grammar rows and the 16 refusal rows turn
  green)
  Verify: `ctest --test-dir build -R harness_jitter_cli_test --output-on-failure` exits 0, and each refusal row stands one line on standard error at exit 2 with no benchmark output ahead of it

**Checkpoint**: the grammar stands green: every refusal prints one
quoted line at status 2, and the option table gains no row.

---

## Phase 3: User Story 3 - the spectrum kernel (Priority: P2) (module M-E, wave 2)

**Goal**: the transform is a pure function over a sample span: it
names the strongest periods, keeps a bin above three times the
median of bins 1 through N/2, reads at most 65,536 points, and
resamples the FWQ unit times at the median unit time.

**Independent Test**: the kernel needs no probe, no seam and no
provider, so a test scripts a sample span and reads the period
list back.

### Tests for User Story 3 (write first, observe failure)

- [x] T060 [US3] Register the test executable `harness_spectrum_test` in
  `test/CMakeLists.txt` after the `harness_jitter_preset_test` block (
  `:973-984`), the `add_executable` and `add_test` pair of the shape at
  `:874-880`, and open `test/source/harness_spectrum_test.cpp` with the
  frameworkless `main` and the `check()` and `fail()` shape of
  `test/source/harness_json_schema_test.cpp:16` and `:301-312`, so the
  entry configures and links before a row stands written (FR-018, FR-025).
  Verify: `cmake --build build --target harness_spectrum_test` exits 0, and `ctest --test-dir build -R harness_spectrum_test --show-only` lists one entry
- [x] T008 [P] [US3] Write the failing transform tests in
  `test/source/harness_spectrum_test.cpp`, the frameworkless shape of
  `test/source/harness_json_schema_test.cpp:16` and `:301-312`:
  `aPureToneNamesItsPeriod`, `twoTonesRankStrongestFirst`,
  `aConstantSeriesAnswersAnEmptyList`, `oneSampleAnswersAnEmptyList`,
  `theCapReadsEveryPoint`, `aLongerSeriesDisclosesTheSkippedCount`,
  `sixTonesKeepFiveEntries`, `theMedianStandsOverBinsOneToHalf`,
  `aBinBelowTheThresholdStandsDropped`; the entry names no `TIMEOUT`, so
  the loop at `test/CMakeLists.txt:2123-2128` gives it 300 s, and the tone
  cases stay at or below 4,096 points (FR-018, FR-025, SC-002).
  (depends on T060, which registers the executable and opens the file)
  Verify: `cmake --build build --target harness_spectrum_test` exits non-zero with the rows failing to compile (red observed as a compile failure)
- [x] T009 [US3] Write the failing resample tests in
  `test/source/harness_spectrum_test.cpp`: `theResampleAveragesEachBin`,
  `anEmptyBinTakesThePreviousSample`, `theResampleStepStandsAtTheMedian`
  (FR-017).
  (depends on T008, same file, behind a disjoint scenario block)
  Verify: `ctest --test-dir build -R harness_spectrum_test --output-on-failure` exits non-zero (red observed)
- [x] T010 [US3] Add `source/harness/detail/spectrum.hpp` and
  `source/harness/spectrum.cpp`: `kSpectrumCap` 65536, `kSpectrumTop` 5,
  `kSpectrumMedianFactor` 3.0, `SpectrumPeriod`, `SpectrumResult`,
  `resampleUnitTimes` and `dftPeriods` in `sg::detail`, each with its
  `\pre` and `\post`; the kernel builds each bin sine and cosine by
  recurrence with no library call in the inner loop, reads the last
  `min(samples.size(), kSpectrumCap)` points, names the skipped count,
  keeps a bin above `3.0 * median` over bins 1 through N/2, sorts by power
  with ties in period order, and takes five; the target list needs no
  edit, because `CMakeLists.txt:255-260` globs `source/harness/*.cpp` with
  `CONFIGURE_DEPENDS` (FR-017, FR-018, D-5, R-12, R-13).
  (depends on T008, T009; the tone rows and the resample rows turn
  green)
  Verify: `ctest --test-dir build -R harness_spectrum_test --output-on-failure` exits 0

- [x] T063 [US3] Remove the window mean in `dftPeriods` in
  `source/harness/spectrum.cpp`, ahead of the bin loop. Add
  `aNonzeroConstantAnswersAnEmptyList` to
  `test/source/harness_spectrum_test.cpp`. A constant at 1000.0 leaves
  every bin at a rounding floor. The median reads that floor, so the
  strict compare keeps 113 bins at 1,024 points. Removing the mean leaves
  bins 1 through N/2 equal in value and drops the floor to zero. The
  constant case then answers an empty list at any level (FR-018, SC-002,
  R-7).
  Verify: `ctest --test-dir build -R harness_spectrum_test --output-on-failure` exits 0, and the new row passes at the level 1000.0

**Checkpoint**: the kernel stands green as a pure function: the tone,
constant, one-sample, cap, truncation, six-tone and resample rows
all pass, and no probe runs.

---

## Phase 4: User Stories 1 and 2 - the calibration, the FWQ loop, the FTQ loop (Priority: P1) (module M-C, wave 3)

**Goal**: each probe cpu calibrates its own work count on its own
cpu before its own window; the FWQ arm times one unit with one
read; the FTQ arm counts whole units per quantum; one pre-sized
buffer holds the series and counts every lost record.

**Independent Test**: with the scripted `WindowReader` installed
through `installTestReader`, the read counts match the scripted
layout, the figures match the scripted series, and the armed
spans record zero allocations.

### Tests for User Stories 1 and 2 (write first, observe failure)

- [x] T011 [US1] Write the failing calibration tests in
  `test/source/harness_fwq_ftq_test.cpp`, the shape of
  `test/source/harness_jitter_probe_test.cpp` with `check()`, `fail()`,
  `pickCpus` over `sched_getaffinity` (`:100-122`), `passSeries` over
  1,024 reads (`:233-241`) and a per-thread reader in the shape of
  `PhasedReader` (`:297-333`): `calibrationStopsAtTheFirstTrial`,
  `calibrationStopsAtTheTrialCap`, `calibrationStopsAtTheCountCap`,
  `calibrationStopsAtTheOneCountInterval`,
  `calibrationKeepsTheClosestCount`, `calibrationRunsOncePerProbeCpu`,
  `calibrationStaysOutOfTheWindowRuntime`; every builder takes the prefix
  length as a parameter, the two known cases standing 2 reads for a delta
  at or above the target and 98 reads for a delta below it, and each test
  asserts its own read count (FR-010, FR-011, D-2, R-04, R-05).
  (depends on T003, which declares the option fields; the same file as
  T001, behind a disjoint scenario block, so one worker runs the two in
  order)
  Verify: `ctest --test-dir build -R harness_fwq_ftq_test --output-on-failure` exits non-zero (red observed)
- [x] T012 [US1] Write the failing FWQ series tests in
  `test/source/harness_fwq_ftq_test.cpp`:
  `fwqCleanSeriesAnswersEqualFigures`, `fwqOneSlowUnitMovesTheMaximum`,
  `fwqNoiseFractionOfASlowUnit`, `fwqBufferFullCountsDrops`,
  `seriesPercentilesReadTheKeptSamples`, `oneSampleSeriesReadsThatSample`,
  `fwqResampleReachesThePeriodList`, `precheckModeRunsTheSelectedMethod`;
  each row carries its read-count check
  over the scripted layout, the clean 1,000-unit case standing 2,027 reads
  behind a prefix of 2, the 10,000-unit case with one unit at ten times
  standing 11,027 reads, the `gaps=2` case keeping 2 and dropping 2, and
  the dip case standing 66,563 reads over 65,536 units, and the
  `mode=precheck` row running the selected method (FR-004, FR-007, FR-008,
  FR-009, FR-012, FR-014, FR-022, SC-001, R-08, R-09, R-10, R-14).
  (depends on T011, same file, behind a disjoint scenario block)
  Verify: `ctest --test-dir build -R harness_fwq_ftq_test --output-on-failure` exits non-zero (red observed)
- [x] T013 [US2] Write the failing FTQ tests in
  `test/source/harness_fwq_ftq_test.cpp`:
  `ftqCleanCountsAnswerEqualFigures`, `ftqOneLowCountMovesTheMinimum`,
  `ftqNoiseFractionOfALowQuantum`, `ftqZeroMaximumAnswersOneHundred`,
  `ftqEdgesStandFromTheFirstStamp`, `ftqTrailingOpenQuantumRecordsNothing`
  , `ftqBufferFullCountsDrops`, `ftqQuantumOfTenNanosecondsTarget`,
  `ftqDipEveryMillisecondNamesThePeriod`; the 100-quanta case at a 100 us
  quantum stands 101,027 reads and keeps 100 counts, the 1,000-quanta case
  with one quantum at half the best count keeps 1,000 counts, and the dip
  row plants a dip every 1 ms over 1,000 samples at a 100 us quantum and
  asserts the period inside 990,000 to 1,010,000 ns (FR-009, FR-015,
  FR-016, FR-022, FR-025, SC-002, D-4, R-07, R-08).
  (depends on T012, same file, behind a disjoint scenario block)
  Verify: `ctest --test-dir build -R harness_fwq_ftq_test --output-on-failure` exits non-zero (red observed)
- [x] T014 [US1] [US2] Add the two armed spans to
  `test/source/harness_jitter_probe_alloc_test.cpp`, the shape of the
  armed span at `:212-266` with the counting `operator new` and
  `operator delete` at `:63-127`: one span over the FWQ unit loop and one
  over the FTQ quantum loop, each proving zero allocations inside the loop
  (FR-014, D-6, R-08).
  (depends on T011)
  Verify: `ctest --test-dir build -R harness_jitter_probe_alloc_test --output-on-failure` exits non-zero (red observed)
- [x] T015 [US1] [US2] Add `SeriesRecord` as the 8-byte series element,
  one `std::uint64_t` per sample, with its `static_assert` after
  `GapRecord` in `source/harness/detail/jitter_probe.hpp:35-41`, and the
  series fields in `ProbeCpuState` before the `alignas` stop word (`:73`):
  the pre-sized `std::vector<std::uint64_t>` at `gapCount` entries, the
  kept count and the dropped count (FR-014, FR-016, FR-022, D-6, R-08).
  (depends on T011, T012, T013, T014)
  Verify: `cmake --build build --target speedgun-ng_harness` exits 0 with no warning line, and the wave's test binaries stay red until T018
- [x] T016 [US1] Add `calibrateUnit` in
  `source/harness/jitter_probe.cpp` before `ProbeItem` (`:160`), called
  inside `ProbeItem::operator()` (`:197`) after the buffer first touch (
  `:209`) and before the 1,024-read measurement pass (`:243`, `kPassReads`
  at `:50`): doubling from 1 in at most 25 trials, bisection in at most 24
  steps, two reads per trial around a body closed on `sg::doNotOptimize` (
  `include/speedgun-ng/barrier.hpp:59`, `:78`), the count cap at
  16,777,216, the target at 1,000 ns for FWQ and one thousandth of the
  quantum for FTQ, the closest measured count kept, the span standing out
  of the window runtime (FR-010, FR-011, D-2, R-04, R-05).
  (depends on T015)
  Verify: `cmake --build build` exits 0, and the calibration rows of T011 turn green
- [x] T017 [US1] Add `keepSeriesOrDrop` beside `keepOrDrop` (
  `source/harness/jitter_probe.cpp:106-120`), writing into the pre-sized
  buffer and counting each lost record, and add the FWQ arm of the
  alongside loop (`:318-329`) and of the pre-check loop (`:266-298`): one
  read per unit, the unit time standing at the delta between two
  consecutive reads, the loop allocating nothing, reading no file, taking
  no lock and reading no clock outside `sg::counters` (FR-012, FR-014,
  FR-022, D-3, D-6, R-06, R-08).
  (depends on T015, T016)
  Verify: `cmake --build build` exits 0, and the FWQ rows of T012 turn green
- [x] T018 [US2] Add the FTQ arm of the two loops: edge k stands at the
  window first stamp plus k times the quantum, the read compares against
  the next edge, the pass stores the count at the edge, resets it and
  advances the edge by one quantum, and a trailing open quantum records no
  count (FR-015, FR-016, FR-022, D-4, R-07, R-08).
  (depends on T017)
  Verify: `ctest --test-dir build -R "harness_fwq_ftq_test|harness_jitter_probe_alloc_test" --output-on-failure` exits 0, and `bash test/time_source_gate.sh .` prints `time_source_gate: clean` and exits 0

- [x] T064 [US2] Add the FTQ arm to the pre-check phase of
  `source/harness/jitter_probe.cpp`, beside the FWQ arm. Edge k stands at
  the phase first stamp plus k quanta. The read compares against the next
  edge, and the phase ends at its span. Add
  `ftqPrecheckRunsTheSelectedMethod` to
  `test/source/harness_fwq_ftq_test.cpp`. FR-004 says every mode runs the
  selected method. A `method=ftq` run answers the standing gap pass today,
  so its pre-check record carries gaps and an empty series. The noise gate
  of T038 then reads nothing (FR-004, FR-015, FR-021, D-9, R-15).
  Verify: `ctest --test-dir build -R harness_fwq_ftq_test --output-on-failure` exits 0, and the new row names counts in the pre-check series

- [x] T069 [US1] Read `options.workCount` in `calibrateUnit`
  (`source/harness/jitter_probe.cpp:249`): a count the value names stands
  as the unit count, and the search does not run for that cpu. The
  calibration span reads 0, and the measured unit time reads the first
  unit of that cpu's window. Add the row that names a `work=1000` run
  keeping 1000 (FR-002, FR-010, FR-011, D-2).
  Verify: `ctest --test-dir build -R harness_fwq_ftq_test --output-on-failure` exits 0, and the new row stands green

**Checkpoint**: the probe body stands green behind the seam: the five
calibration stops, the FWQ unit times, the FTQ edges, the drop rule
and the two armed spans all pass, and the time-source gate stands
clean.

---

## Phase 5: User Stories 1, 2 and 4 - the series fold (Priority: P1) (module M-D, wave 4)

**Goal**: one fold reads the one series buffer twice: once for the
run record, and once per benchmark over that benchmark half-open
timed windows.

**Independent Test**: with a scripted series and scripted windows,
the run fold answers the figures of the kept samples, and the
per-benchmark fold stands null where the windows keep no sample.

### Tests for User Stories 1, 2 and 4 (write first, observe failure)

- [x] T019 [P] [US4] Write the failing fold tests in
  `test/source/harness_jitter_window_test.cpp`, over the standing
  `captureRun` (`:285-319`) and `checkStatsRoundTrip` (`:472-512`)
  helpers: `thePerEntrySeriesFoldsTheWindows` and
  `thePerEntrySeriesStandsNullWithoutSamples` (FR-020, R-11).
  (depends on T003, which declares `JitterEntryFold::series`; this task
  opens the file for T023, T027 and T033, which follow it in one worker)
  Verify: `ctest --test-dir build -R harness_jitter_window_test --output-on-failure` exits non-zero (red observed)
- [x] T020 [US4] Declare the two folds in
  `source/harness/detail/jitter_probe.hpp` after `gapStatsOf` (`:121`,
  `:136`): `seriesStatsOf(const ProbeCpuState&)` and
  `seriesStatsOf(const ProbeCpuState&, std::span<const TimedWindow>)`,
  each with the contracts of
  [contracts/public-api.md](./contracts/public-api.md) (FR-007, FR-008,
  FR-020, R-09, R-11).
  (depends on T019)
  Verify: `cmake --build build` exits 0 with no warning line
- [x] T021 [US1] [US2] Define `seriesStatsOf(const ProbeCpuState&)` in
  `source/harness/jitter_probe.cpp` after `gapStatsOf` (`:688`): copy the
  kept values into a sorted `std::vector<double>`, the shape `:527-535`
  uses, read the five points through `percentileField` (`:346`) over
  `percentileAt` (`source/harness/percentiles.cpp:104`) with the standing
  minimum sample count, and answer the two noise fractions of FR-009, the
  FWQ fraction over the kept unit times and the FTQ fraction over the
  counts, a maximum count of 0 answering 100 (FR-007, FR-008, FR-009,
  SC-001, R-09, R-10).
  (depends on T020)
  Verify: `cmake --build build` exits 0, and the figure rows of T011 and T012 turn green
- [x] T022 [US4] Define
  `seriesStatsOf(const ProbeCpuState&, std::span<const TimedWindow>)`
  beside the window fold (`source/harness/jitter_probe.cpp:614`), keeping
  the values whose stamp falls in a half-open window, the rule at
  `:636-643`, and fill `JitterEntryFold::series` in the split loop (
  `source/harness/cli.cpp:1819-1825`) and in the pre-check record loop (
  `:1214-1223`); no second buffer joins (FR-020, R-11).
  (depends on T021)
  Verify: `ctest --test-dir build -R harness_fwq_ftq_test --output-on-failure` exits 0, and the fold rows of T019 turn green

- [x] T067 [US4] Add the ordering postcondition to both series folds of
  `source/harness/jitter_probe.cpp:993` and `:1158`, the chain the gap
  folds state at `:935` and `:1077`. `cmake/coverage.cmake:101` omits
  contract lines from the trace, so the check costs no branch position
  (FR-008, FR-020, contracts/public-api.md).
  Verify: `cmake --build build` exits 0, and `ctest --test-dir build -R harness_fwq_ftq_test --output-on-failure` exits 0

**Checkpoint**: the folds stand green: the run record reads the kept
samples, the per-benchmark fold reads the windows, and no buffer
copy joins.

---

## Phase 6: User Story 4 - the record, the schema and the rule names (Priority: P2) (module M-G, wave 4)

**Goal**: the document carries `method` and one closed `series` item
per probe cpu at schema `1.12`, `loadReport` reads every key back,
the rule table grows to 32 names, and a run with no `method` key
stands byte-identical to master apart from the version figures.

**Independent Test**: a scripted run writes the document, the schema
checker passes it against `docs/schema/report-v1.json`,
`loadReport` reads every new key back, and the fixture bytes fold
equal for a `tsc` run.

### Tests for User Story 4 (write first, observe failure)

- [x] T023 [US4] Write the failing writer and round-trip rows in
  `test/source/harness_jitter_window_test.cpp`:
  `theMethodWordStandsUpperCase`, `theSeriesItemCarriesEveryKey`,
  `thePeriodsArrayRidesTheCpuItem`, `theSeriesRoundTripsThroughLoadReport`
  , `aTscRunWritesNoNewKey` (FR-006, FR-019, FR-020, FR-031, R-16, R-17).
  (depends on T019, same file, behind a disjoint scenario block)
  Verify: `ctest --test-dir build -R harness_jitter_window_test --output-on-failure` exits non-zero (red observed)
- [x] T024 [US4] Write the failing read-back rows in
  `test/source/harness_json_load_test.cpp`:
  `theSeriesRecordReadsBackEveryKey`, `thePeriodListReadsBack`,
  `aMissingMethodKeyLeavesTheRunRecordEmpty`,
  `aMissingSeriesKeyLeavesTheFoldsEmpty`,
  `eachNewKeyOfTheWrongKindStandsRefused`,
  `aClosedSeriesItemRefusesAnUnknownKey`,
  `aClosedPeriodItemRefusesAnUnknownKey`,
  `thePerEntrySeriesReadsIntoTheFolds` (FR-020, R-17).
  (depends on T023)
  Verify: `ctest --test-dir build -R harness_json_load_test --output-on-failure` exits non-zero (red observed)
- [x] T025 [US4] Write the failing schema rows in
  `test/source/harness_json_schema_test.cpp`, the planted-disagreement
  shape at `:41-117` and the stamped-minor shape at `:158-189`:
  `theSeriesKeysPairAtTheStampedMinor`,
  `aClosedSeriesItemRefusesAnAddedKey`,
  `aSeriesKeyOutOfOrderStandsRefused`,
  `aMethodWordOutsideTheThreeStandsRefused`, `anEmptyPeriodsArrayPasses`,
  `aPeriodItemOfTheWrongKindStandsRefused` (FR-006, R-16).
  (depends on T024)
  Verify: `ctest --test-dir build -R harness_json_schema_test --output-on-failure` exits non-zero (red observed)
- [x] T026 [US4] Register the test executable `harness_jitter_bytes_test`
  in `test/CMakeLists.txt` with the schema define, the shape of
  `:958-968`, then write the failing byte-identity tests in
  `test/source/harness_jitter_bytes_test.cpp`:
  `aTscRunMatchesTheFixtureBytes`, `eachMaskedSpanHoldsANumber`,
  `theNewKeysStandAbsentFromTheDocument`,
  `theStandingConsoleLinesMatchTheRecordedBytes`,
  `aTscRunPassesTheSchemaAtTheStampedMinor`; the fixture carries master
  bytes inline, the shape of
  `test/source/harness_noise_report_test.cpp:456-461`, and each mask
  checks that its span holds a number, the way `maskHostFloor` does at
  `test/source/harness_pass_report_test.cpp:267-281` (FR-026, FR-031,
  SC-004, R-19).
  (depends on T025; the chain T023, T024, T025, T026 runs in one worker,
  one file each, each row reading the answer of the row before)
  Verify: `ctest --test-dir build -R harness_jitter_bytes_test --output-on-failure` exits non-zero (red observed)
- [x] T027 [US4] Write the failing rule-name rows in
  `test/source/harness_jitter_window_test.cpp`, the shape of the name rows
  at `:1180-1441`: `theSixteenNewRuleNamesResolve`,
  `theFaultTextNamesThirtyTwoFields`, `theRunScopedNameReadsTheRunRecord`,
  `theAnyReadingAnswersTheLargestCount` (FR-021, R-18).
  (depends on T023, same file, behind a disjoint scenario block)
  Verify: `ctest --test-dir build -R harness_jitter_window_test --output-on-failure` exits non-zero (red observed)
- [x] T028 [US4] Write the series item: `writeJitterSeriesCpu` after
  `writeJitterCpu` (`source/harness/json_report.cpp:347`), the two root
  keys after `loopPeriodNs` (`:1017`) inside the standing `jitter` object,
  the per-entry `series` array beside the standing `stats` array (
  `:630-644`) with no `method` key and no `periods` key, `kSchemaVersion`
  at `1.12` (`:43`) with its history comment (`:30-42`),
  `kJitterMethodWords` after `kJitterModeWords` (
  `source/harness/detail/json_report.hpp:77-83`), the series field in
  `JitterReportCpu` (`:232-237`), the method field in `JitterReportRecord`
  (`:249-259`), the writer postconditions (`:290-312`), and the method and
  series items in `buildJitterReportRecord` (
  `source/harness/cli.cpp:402-469`) (FR-006, FR-019, FR-020, D-7, R-16).
  The five rows that pin the stamped minor move in this commit. Three
  stand outside the plan's file table:
  `test/source/harness_json_report_test.cpp:546`,
  `test/source/harness_hunt_json_test.cpp:1537` and
  `test/source/harness_noise_report_test.cpp:426`, with its fixture at
  `:457`. The comments that name the minor move with them (FR-006,
  FR-031).
  (depends on T023, T024, T025, T026, T027)
  Verify: `cmake --build build` exits 0 with no warning line
- [x] T029 [US4] Read the keys back: `readJitterSeriesCpu` after
  `readJitterCpu` (`source/harness/json_load.cpp:544`), reading and
  kind-checking each key in the written order the shape at `:494-511`
  uses; the two keys in `readJitterRecord` (`:1243`) between the
  `loopPeriodNs` read (`:1265-1271`) and the `cpus` read (`:1272-1282`);
  `data.jitterRun` at the root arm (`:2695-2701`); the entry series at the
  per-entry arm (`:2178-2207`) (FR-020, R-17).
  (depends on T028)
  Verify: `cmake --build build` exits 0, and the read-back rows of T024 turn green
- [x] T030 [US4] Extend `docs/schema/report-v1.json`: `method` and
  `series` in the `jitter` object `x-order` after `loopPeriodNs` (`:47-55`
  ) and in `properties` (`:72`), out of `required` (`:56-64`),
  `additionalProperties` staying `false` (`:65`), the closed 21-key
  `series` item with its own `x-order`, `required` of 19 names omitting
  `quantumNs` and `resampleStepNs`, and the closed `periods` item of
  `periodNs` and `powerRatio`, the shape the standing `stats` item carries
  at `:78-117` (FR-006, R-16). This commit lands the `report-json.md`
  delta of `specs/027-jitter-probe/contracts/` (FR-030), and its commit
  message records the schema check beside the `1.12` minor (FR-005,
  SC-009).
  (depends on T025, T028; the schema rows of T025 turn green)
  Verify: `ctest --test-dir build -R harness_json_schema_test --output-on-failure` exits 0
- [x] T031 [US4] Grow the rule table in `source/harness/compare.cpp`:
  `kJitterFields` from 16 entries to 32 (`:251-292`) with `sampleCount`,
  `minValue`, `medianValue`, `meanValue`, `p90Value`, `p99Value`,
  `p999Value`, `p9999Value`, `maxValue`, `noisePercent`, `seriesRuntimeNs`
  , `droppedSamples`, `unitTimeNs`, `quantumNs`, `resampleStepNs`,
  `spectrumPoints`; the value reader takes the fold (`:241-245`); the
  fault text reads `names none of the thirty-two jitter fields` (
  `:366-369`); the two call sites pass the fold (`:1477`, `:1489`); the
  bare form reads the per-benchmark series and `jitter:run:` reads the run
  record, the `any` reading staying the uniform maximum (FR-021, R-18).
  This commit lands the `rule-names.md` delta: the 16 names, the count
  word `thirty-two` and the `any` reading of a count figure (FR-030).
  (depends on T027, T028, T029)
  Verify: `ctest --test-dir build -R harness_compare_test --output-on-failure` exits 0, and the rule-name rows of T027 turn green
- [x] T032 [US4] Close the record wave: the byte identity of a run with
  no `method` key stands green, the document writing neither new key and
  every standing console line standing byte-identical (FR-026, FR-031,
  SC-004, R-19).
  (depends on T028, T029, T030, T031)
  Verify:## Phase 7: User Stories 1, 2 and 4 - the console block and the pre-check pair (Priority: P1) (module M-F, waves 4 and 5)

**Goal**: the console gains 30 series lines, one field per line and
one column per probe cpu, the calibration lines print before the
window, the pre-check pair carries the method figures, and the noise
refusal stops the run at status 2.

**Independent Test**: with a scripted run, the block prints the 30
lines in the standing order, the calibration lines stand ahead of
the window, and the pre-check lines carry the method pair.

### Tests for User Stories 1, 2 and 4 (write first, observe failure)

- [x] T033 [US1] [US2] Write the failing console rows in
  `test/source/harness_jitter_window_test.cpp`, the field-line walk shape
  at `:655-662`: `theFwqBlockPrintsTheThirtyNewLines`,
  `theFtqBlockPrintsTheQuantumLines`,
  `theCalibrationLinesStandBeforeTheWindow`, `theRankACpuOmitsPrintsZero`
  (FR-011, FR-013, FR-016, FR-019, FR-022, R-14).
  (depends on T023, T027, same file, behind a disjoint scenario block)
  Verify: `ctest --test-dir build -R harness_jitter_window_test --output-on-failure` exits non-zero (red observed)
- [x] T034 [US4] Write the failing pre-check rows in
  `test/source/harness_precheck_test.cpp` over `capturePrecheck` (
  `:234-252`), the exec child shape at `:278-341` and the standing skip
  for a mask below four cpus at `:443-445`:
  `theFwqPrecheckPairCarriesNoiseAndMaximum`,
  `theFtqPrecheckPairCarriesNoiseAndMinimum`,
  `thePinningRankSortsByNoiseFraction`, `theNoiseRefusalStopsAtStatusTwo`,
  `theAlongsideStrictWordKeepsItsMeaning`,
  `anUnusedNoiseValueStandsDisclosed`, `aTscRunKeepsTheGapRefusal`
  (FR-004, FR-021, D-9, R-15).
  (depends on T033)
  Verify: `ctest --test-dir build -R harness_precheck_test --output-on-failure` exits non-zero (red observed)
- [x] T035 [US1] [US2] Print the 30 lines in `printJitterReport` (
  `source/harness/report.cpp:726`) after `droppedGaps` (`:795-796`) in
  three groups: the calibration group of four lines `method`,
  `workCount`, `targetNs` and `calibrationSpanNs`; the 16 figures of
  FR-021; then `period1Ns` through `period5Ns` and `period1Power` through
  `period5Power`, ten lines. The block stands 30 lines, each field
  printing once with one column per probe cpu in the standing cpu order (
  `:738-743`), a rank a cpu carries printing 0, the console spelling
  `tsc`, `fwq`, `ftq`; add the method argument at the two
  `printJitterReport` calls (`source/harness/cli.cpp:1881`, `:1883`)
  (FR-013, FR-016, FR-019, FR-022, D-7, R-14). This commit lands the
  series lines of the `console-report.md` delta of
  `specs/034-host-noise-precheck/contracts/` (FR-030).
  (depends on T033)
  Verify: `cmake --build build` exits 0 with no warning line
- [x] T036 [US1] Print the calibration group ahead of the batch first
  run, after `jitterProbe->start()` (`source/harness/cli.cpp:1129`): the
  four lines `method`, `workCount`, `targetNs` and `calibrationSpanNs`,
  each standing before that cpu window. The measured unit time and the
  quantum ride the 16 figures of FR-021 in the block, so no field prints
  twice, and the JSON carries the five keys of FR-011 (FR-011, FR-019,
  SC-003, D-2, R-05).
  (depends on T035)
  Verify: `cmake --build build` exits 0, and the calibration rows of T033 turn green
- [x] T037 [US4] Swap the pair the pre-check lines read in
  `printPrecheckReport` (`source/harness/report.cpp:1172`): the warning
  line (`:1305-1310`) and the ranking (`:1311-1321`) take the method pair:
  the noise fraction and the maximum unit time for FWQ, the noise fraction
  and the minimum count for FTQ. The pinning line ranks by the noise
  fraction ascending with ties in cpu order, the standing `tsc` figures
  staying (FR-004, FR-021, D-9, R-15). This commit lands the pair of the
  `console-report.md` delta of
  `specs/034-host-noise-precheck/contracts/` (FR-030).
  (depends on T035)
  Verify: `cmake --build build` exits 0, and the pair rows of T034 turn green
- [x] T038 [US4] Fire the noise refusal in the pre-check phase beside
  the standing strict refusal (`source/harness/cli.cpp:1155-1176`): a
  probe cpu at or above the `noise=` threshold answers one line at exit
  status 2 after the pre-check window and before the batch first run;
  `mode=alongside` keeps the `strict` meaning and fires no noise gate; a
  `noise=` value with no `strict`, and a `noise=` value with `method=tsc`,
  stand accepted and disclosed as unused (FR-021, D-9, R-15).
  (depends on T034, T036, T037; the refusal rows of T034 turn green)
  Verify: `taskset -c 0,1 ctest --test-dir build -R "harness_precheck_test|harness_jitter_window_test" --output-on-failure` exits 0

- [x] T065 [US4] Print the series figures in the per-benchmark jitter
  block of `source/harness/report.cpp:657` when the method keeps a
  series. A FWQ run places no gap today, so the block prints
  `gaps=0 max=0 total=0.00` for every entry. The block names the series
  figures in that case, and the gap line stands for a `tsc` run
  (FR-019, FR-020, D-7).
  Verify: `ctest --test-dir build -R "harness_jitter_cli_test|docs_jitter_probe" --output-on-failure` exits 0
- [x] T066 [US1] [US2] Snapshot the kept series count at the latch line
  of `source/harness/jitter_probe.cpp`, and fold that prefix into the
  pre-check record of a `mode=both` run in `source/harness/cli.cpp:1219-1233`.
  The standing precondition bars a read of a live buffer. The prefix
  stands stable once the phase ends at the latch, so the read is safe.
  Add the row that names the pre-check series of a `mode=both` run
  (FR-004, FR-020, E-27, R-04).
  Verify: `ctest --test-dir build -R "harness_fwq_ftq_test|harness_jitter_window_test" --output-on-failure` exits 0

- [x] T068 [US4] Pass the `noise=` threshold into `printPrecheckReport`.
  The declaration stands in `source/harness/detail/internal.hpp`, the
  definition in `source/harness/report.cpp:1172`, and the call in
  `source/harness/cli.cpp`. The warning rule and the strict refusal then
  read the same pair against the same number, the rule the
  `console-report.md` delta states. Add the row that names a run under
  the threshold warning nothing (FR-021, D-9, R-15).
  Verify: `ctest --test-dir build -R harness_precheck_test --output-on-failure` exits 0, and the new row stands green

**Checkpoint**: the console and the pre-check stand green under a
two-cpu mask: the 30 lines print once per field, the calibration
lines stand before the window, and the refusal stops at status 2.

---

d and disclosed as unused (FR-021, D-9, R-15).
  (depends on T034, T036, T037; the refusal rows of T034 turn green)
  Verify: `taskset -c 0,1 ctest --test-dir build -R "harness_precheck_test|harness_jitter_window_test" --output-on-failure` exits 0

**Checkpoint**: the console and the pre-check stand green under a
two-cpu mask: the 30 lines print once per field, the calibration
lines stand before the window, and the refusal stops at status 2.

---

## Phase 8: User Story 6 - the docs, the README budget and the example (Priority: P3) (module M-H, wave 6)

**Goal**: the main page teaches the two methods inside its line
budget, the guide page carries the keys and the 30 field rows, and
one runnable example shows one FWQ family and one FTQ family.

**Independent Test**: `example_drift`, `snippet_compile`,
`example_run` and `report_shape_fwq_ftq_sample` pass, and the prose
gate reads no finding over the branch range.

- [x] T039 [P] [US6] Add the README section `Fixed work and time quanta`
  of 18 lines after `### Noise tracer` (`README.md:764-786`), the three
  parts IV.1 asks for: one purpose sentence naming the two methods, their
  keys and the calibration disclosure; one marked snippet quoting the
  `fwq-ftq` region of `example/docs/fwq_ftq.cpp`; one result sentence
  naming the block the run prints. Add the four quick-reference rows after
  `PrecheckRecord::tracer` (`:938`); extend the `--jitter-probe` row (
  `:966`) with the keys; shorten the quoted regions of the two sections
  the lane owns that do not carry the shared region: add two 8-line
  regions to `example/docs/jitter_probe.cpp` and point `Host noise
  pre-check` and `Noise tracer` at them, the standing region split into
  two complete units, the cap staying 1004 (FR-023, IV.1, R-23). The page total
  lands only after T042 registers the section, and T042 counts the page.
  (depends on T035, which prints the lines the section teaches)
  Verify: `python3 tools/docs/example_drift.py .` exits 0 with no line printed, and `sed -n '/^### Fixed work and time quanta$/,/^### Hunter mode$/p' README.md | head -n -1 | wc -l` reads 18
- [x] T040 [US6] Add the guide-page rows in `docs/pages/harness.md`:
  the value rows after `strict` (`:1832`), the 30 series field rows after
  `droppedGaps` (`:1853`), the names rows after `JitterEntryFold` (`:1866`
  ), the block sample (`:1798-1820`), and the version sentence at `:1399`
  reading `1.12` (FR-029, R-23).
  (depends on T039)
  Verify: `python3 tools/prose/prose_gate.py --base origin/master` reads `0 findings` and exits 0
- [x] T041 [US6] Add `example/docs/fwq_ftq.cpp` with one FWQ family and
  one FTQ family, the shape of `example/docs/jitter_probe.cpp`, and record
  `example/docs/fwq_ftq.expected`. Mark three regions, each a complete
  top-level unit, on six marker lines: `fwq-unit` around the FWQ benchmark
  function and its `SG_BENCHMARK` line, `ftq-unit` around the FTQ
  benchmark function and its `SG_BENCHMARK` line, and `fwq-ftq` around the
  `main` function, each with its `sg-example:begin` and `sg-example:end`
  pair. The `.expected` file carries the `report` region, the shape of
  `example/docs/percentiles.expected`. Add `add_docs_example(fwq_ftq)`
  after `:78` in `example/CMakeLists.txt`, the function standing at
  `:40-48` (FR-029, R-24).
  (depends on T035, T036, which print the lines the example records)
  Verify: `cmake --build build --target docs_fwq_ftq` exits 0, and `./build/example/docs_fwq_ftq --min-time=0.01` exits 0
- [x] T042 [US6] Register the title `Fixed work and time quanta` in
  `SECTIONS` (`tools/docs/example_drift.py:33-68`), the owner row
  `("fwq_ftq.cpp",)` in `SECTION_OWNERS` (`:73-108`) and the cap ledger
  comment (`:178-183`). The standing region of
  `example/docs/jitter_probe.cpp:9-25` splits into `probe-unit` and
  `probe-main`, each a complete top-level unit, and the new section
  quotes regions of `example/docs/fwq_ftq.cpp` alone, so every region
  stays a complete top-level unit and `snippet_compile` compiles every
  emitted block. The registration rows stand as the work FR-034 requires,
  and both caps stay (FR-023, FR-034, R-23).
  (depends on T039, T041)
  Verify: `ctest --test-dir build -R "example_drift|snippet_compile|example_run" --output-on-failure` exits 0, and `wc -l < README.md` reads at or below 1004, the cap ledger naming the count
- [x] T043 [US6] Add the shape fold entry `report_shape_fwq_ftq_sample`
  after `report_shape_jitter_sample` (`test/CMakeLists.txt:1905-1922`),
  and the three fold rows of
  [contracts/report-shape.md](./contracts/report-shape.md) in
  `tools/docs/report_shape.py` with the golden pairs in
  `test/example-drift-fixture/run_report_shape_fixtures.py`, the method
  word keeping its bytes and the period figures folding their digits. The
  ctest entry, the three fold rows and the golden pairs stand as the
  registration rows FR-034 requires (FR-029, FR-034, SC-007, R-24). This
  commit lands the three fold rows of the `report-shape.md` delta of
  `specs/034-host-noise-precheck/contracts/` (FR-030).
  (depends on T041)
  Verify: `ctest --test-dir build -R "report_shape_fwq_ftq_sample|report_shape_jitter_sample|report_shape_fixtures" --output-on-failure` exits 0

**Checkpoint**: the docs stand green: the page lands at or below 1004 lines, the
example builds, runs and folds, and the prose gate reads no finding.

---

## Phase 9: User Story 7 - the acceptance records and the standing contracts (Priority: P3) (module M-I, wave 7)

**Goal**: the acceptance runs stand recorded on the lane host, one
record per script run with eight sections, the quickstart links
every record, and the six standing contracts stand amended in the
commit that lands the code.

**Independent Test**: the results directory carries one record per
script run, each naming the host, the kernel, `CONFIG_HZ`, the L3
group, the commands, the exit codes, the calibration figures and
the detected periods, and each showing a drop count of 0.

- [x] T044 [US7] Write `tools/noise/fwq_ftq_acceptance.sh` in the shape
  of `tools/noise/jitter_probe_acceptance.sh:35-52`: eight sections, two
  methods, two L3 groups and two load states, the window from a named
  benchmark run with `--min-time=10` and `--repetitions=1`,
  `gaps=10485760` for FWQ and `gaps=131072` for FTQ, the load a busy loop
  pinned to the SMT sibling thread recorded with its command line and its
  cpu id, and `CONFIG_HZ` read from `/proc/config.gz`, then
  `/boot/config-$(uname -r)`, reading `unknown` when both stand absent
  (FR-027, FR-028, D-10, R-21).
  (depends on T018, T038, which build the binary the script runs)
  Verify: `bash -n tools/noise/fwq_ftq_acceptance.sh` exits 0, and `grep -c "min-time=10" tools/noise/fwq_ftq_acceptance.sh` reads at least the two method commands
- [x] T045 [US7] Record the acceptance run on the lane host in
  `specs/043-fwq-ftq/results/acceptance-<date>.md`: eight sections, each
  naming the command line, the exit status and the captured output, the
  record naming the host, the kernel, `CONFIG_HZ`, the L3 group, the
  calibration count, the measured unit time, the noise fraction, the
  `gaps=` value, the kept span in milliseconds and the detected periods,
  each section showing a drop count of 0 (FR-027, SC-008, R-21).
  (depends on T044)
  Verify: `bash tools/noise/fwq_ftq_acceptance.sh` exits 0, and `grep -c "^## " specs/043-fwq-ftq/results/acceptance-<date>.md` reads eight
- [x] T046 [US7] Link every record from the acceptance section of
  `specs/043-fwq-ftq/quickstart.md`, the standing link rule of the
  quickstart (FR-028, SC-008).
  (depends on T045)
  Verify: `grep -c "results/acceptance" specs/043-fwq-ftq/quickstart.md` counts one link per record
- [x] T047 [US7] Check that the six standing contract deltas stand
  landed in the commits that land the code they record (FR-030):
  `specs/027-jitter-probe/contracts/cli-options.md` gains the four keys
  and the head rule at T007; `public-api.md` gains the four option fields
  and the series record at T003; `report-json.md` gains the two keys at
  T030; `rule-names.md` gains the 16 names, the count word `thirty-two`
  and the `any` reading at T031;
  `specs/034-host-noise-precheck/contracts/console-report.md` gains the
  series lines at T035 and the pair at T037; `report-shape.md` gains the
  three fold rows at T043. No other standing rule moves.
  (depends on T003, T007, T030, T031, T035, T037, T043)
  Verify: `grep -c "method=" specs/027-jitter-probe/contracts/cli-options.md` reads at least one row, the two spec 034 contracts name the series lines and the fold rows, `git diff --name-only origin/master...HEAD -- specs/027-jitter-probe/contracts/ specs/034-host-noise-precheck/contracts/` names the six files and no seventh, and `python3 tools/prose/prose_gate.py --base origin/master` reads `0 findings` and exits 0

**Checkpoint**: the evidence stands recorded, the quickstart links
every record, and the standing contracts carry the deltas.

---

- [x] T071 [P0] Stand a named vector behind the FWQ span in `addSpectrumTo`
  (`source/harness/jitter_probe.cpp:1277`). The line assigns
  `std::span<const double>` from the temporary `std::vector<double>` that
  `resampleUnitTimes` returns, so the temporary dies at the end of the
  full-expression and the span dangles through `dftPeriods`. gdb on the
  release binary stops at `addsd (%rax),%xmm5` inside `dftPeriods`, and
  `--jitter-probe=0,method=fwq,mode=precheck,precheck=2400,gaps=10485760`
  exits 139. The FTQ path binds a named local and stands. Add the row
  that reads the FWQ spectrum figures of a series above the transform cap,
  and clear the path under the sanitizer preset (FR-018, FR-033,
  constitution VIII). `ASAN_OPTIONS=detect_leaks=0` on
  `build/sanitize/test/harness_jitter_window_test` reads
  `heap-use-after-free`, a read of 8 bytes at `spectrum.cpp:139`
  through `addSpectrumTo` at `jitter_probe.cpp:1292`, the region freed
  by the vector's deallocate, so the standing row names it today.
  Verify: `cmake --build build/sanitize --target harness_fwq_ftq_test` exits 0, `ASAN_OPTIONS=detect_leaks=0 ./build/sanitize/test/harness_fwq_ftq_test` exits 0 with no `AddressSanitizer` line, and `./build/example/benchmark_example --filter=bmTouch --min-time=1 --repetitions=1 --jitter-probe=0,method=fwq,mode=precheck,precheck=2400,gaps=10485760` exits 0
- [ ] T072 [P0] Read the whole body run in `calibrationTrial`
  (`source/harness/jitter_probe.cpp:229`). The line returns
  `(spanEnd - first) / count`, the per-iteration figure, and the search
  compares it against the target. FR-010 names one unit, which is one body
  run: a body iteration costs about 0.3 ns, so no count reaches 1,000 ns
  per iteration, the doubling runs to the count cap, and the kept count
  reads 1, beside a window unit near 84 ns. Keep the closest-count rule,
  the trial cap and the count cap, and update the scripted rows whose
  counts the division set (FR-010, FR-011, D-2).
  Verify: `ctest --test-dir build -R "harness_fwq_ftq_test|harness_jitter_cli_test|harness_jitter_window_test" --output-on-failure` exits 0, and a `method=fwq` run names a `workCount` above 1 with a unit time near the 1,000 ns target

- [ ] T073 [US7] Re-record the acceptance run after T071 and T072 land. The
  record on disk carries four FWQ sections at exit 139 and `workCount: 1`,
  the two defects as read. Run `bash tools/noise/fwq_ftq_acceptance.sh`
  again on the lane host, keep the eight sections, and land the new record
  beside the quickstart link (FR-027, FR-028, SC-008).
  Verify: `grep -c "^## " specs/043-fwq-ftq/results/acceptance-<date>.md` reads 8, `grep -c "exit status: 0\|exit: 0" specs/043-fwq-ftq/results/acceptance-<date>.md` reads 8, and each section names a `workCount` above 1 and a drop count of 0

- [ ] T070 Close the coverage shortfall the lane head reads, from the
  `coverage` target of the `ci-coverage` preset: lines 16,568 of 16,612
  (44 dark) and branches 8,128 of 8,180 (52 short against the allowance
  of 21). The dark lines stand in six lane files: `compare.cpp:311-328`
  (the 16 series-field lambdas of `kJitterFields`, read by no compare
  row); `json_load.cpp:570,584,643,651,711-713,1501,2414,2420` (the
  reader's refusal arms beside a series item, its cpu key, its periods
  and its ratio); `json_report.cpp:710,1103` (the arm that skips a fold
  with no series inside a run that carries one); `spectrum.cpp:100-101`
  (the tail fill of the trailing open bin); `cli.cpp:2033` (the `noise=`
  unused disclosure); `jitter_probe.cpp:1240` (the pre-check arm for a
  planned cpu with no state, or a `tsc` state). The lane's files carry 45
  of the 52 short branch positions: `cli.cpp` 20, `json_load.cpp` 10,
  `jitter_probe.cpp` 8, `report.cpp` 3, `json_report.cpp` 2,
  `compare.cpp` 1, `spectrum.cpp` 1. Add the rows that reach each arm, in
  the test file that owns the surface. No `LCOV_EXCL`, no new
  `--omit-lines` pattern, no new `--ignore-errors` class, no code added
  only to reach a branch, and the allowance stays 21 (constitution VI,
  FR-033).
  Verify: `cmake --build build/coverage --target coverage` exits 0, and `bash tools/dbc/coverage_gate.sh build/coverage/coverage.info` reads lines at `100.0%` and a branch shortfall at 21 rows or fewer


## Phase 10: Polish and Cross-Cutting Concerns

**Purpose**: the hard gates of Principle VIII at the feature head, in
the order of the plan Gate commands table.

- [ ] T048 Run the release build and the full suite in the `build`
  directory: configure `ci-rocky`, build, and run every test, then the
  three-binary and the four-binary selections of the plan Gate commands
  table (constitution VIII, FR-033).
  (depends on T001 to T047)
  Verify: `cmake --preset=ci-rocky` exits 0, `cmake --build build` exits 0 with no warning line, `ctest --test-dir build --output-on-failure` reads `100% tests passed, 0 tests failed`, `ctest --test-dir build -R "harness_fwq_ftq_test|harness_spectrum_test|harness_jitter_bytes_test" --output-on-failure` exits 0 with three `***Passed` lines, and `ctest --test-dir build -R "harness_cli_table_test|harness_json_schema_test|harness_json_load_test|harness_compare_test" --output-on-failure` exits 0
- [ ] T049 Run the full suite under a two-cpu mask, the probe cpus coming
  from `sched_getaffinity` and no test naming a fixed cpu number. The
  `test` CI job of `.github/workflows/ci.yml` supplies the 2-cpu host
  reading, and T058 records it (FR-024, SC-006).
  (depends on T048, the same `build` directory, so the two stand
  sequential)
  Verify: `taskset -c 0,1 ctest --test-dir build --output-on-failure` exits 0, the narrow-mask rows print the standing `SKIPPED` note at `test/source/harness_jitter_window_test.cpp:623-626` and `test/source/harness_precheck_test.cpp:443-445`, `grep -rnE "cpu[A-Za-z]* *= *[0-9]+|cpus=\"[0-9]" test/source/harness_fwq_ftq_test.cpp test/source/harness_spectrum_test.cpp test/source/harness_jitter_bytes_test.cpp` reads no match, and `grep -c "sched_getaffinity" test/source/harness_fwq_ftq_test.cpp` reads at least one
- [ ] T050 [P] Run the coverage contract in the `build/coverage`
  directory: configure the coverage preset, build, run the suite with
  `-j 1`, and read the coverage target. Every new branch position of the
  plan coverage plan stands reached by a named test, the
  `source/harness/compare.cpp` arms included, and no exclusion joins
  (FR-032, SC-006, R-22).
  (depends on T048)
  Verify: `cmake --preset=ci-coverage` exits 0, `cmake --build build/coverage -j "$(nproc)"` exits 0, `ctest --test-dir build/coverage -j 1 --output-on-failure --no-tests=error` exits 0, `cmake --build build/coverage -t coverage` reads `lines......: 100.0%` and `branches...: 100.0%`, the gate prints no `branch shortfall` line so the shortfall stands 0 rows with the allowance unchanged at 21, `grep -rn "LCOV_EXCL" source/harness` reads no match, and `git diff --name-only origin/master...HEAD -- tools/dbc/coverage_gate.sh` reads no line, so no `--omit-lines`, `--ignore-errors` and `--remove` entry joins
- [ ] T051 [P] Run the sanitizer and thread-sanitizer suites in their own
  binary directories `build/sanitize` and `build/tsan`: configure and
  build `ci-sanitize` and `ci-tsan`, and run each suite (constitution
  VIII, FR-033).
  (depends on T048)
  Verify: `cmake --preset=ci-sanitize`, `cmake --build build/sanitize -j "$(nproc)"` and `ctest --test-dir build/sanitize --output-on-failure -j "$(nproc)"` exit 0 with no `AddressSanitizer` line and no `runtime error` line, and `CXX=clang++ cmake --preset=ci-tsan`, `cmake --build build/tsan -j "$(nproc)"` and `ctest --test-dir build/tsan --output-on-failure -j "$(nproc)"` exit 0 with no `WARNING: ThreadSanitizer` line
- [ ] T052 [P] Run the analyzer build once with clang-tidy and cppcheck
  wired in, and the DBC gate: every new name reads clean under
  `readability-identifier-naming`, and every other finding stands fixed
  (constitution V, IX, FR-033). The build stands in its own binary
  directory `build/tidy`, so the task shares no build output with T048,
  T049 and T054.
  (depends on T048)
  Verify: `cmake --preset=ci-ubuntu -B build/tidy` exits 0, `cmake --build build/tidy -j "$(nproc)"` exits 0 with no new finding, and `cmake -S . -B build/dev --preset=ci-ubuntu` then `cmake --build build/dev -t dbc-gate -j "$(nproc)"` exits 0
- [ ] T053 [P] Run the time-source gate and the code-discipline review:
  time stands from `sg::counters` alone, no lambda stands inside an `SG_*`
  macro, contract-only helpers carry `[[maybe_unused]]`, and simdjson
  stands the only JSON reader the change uses (FR-024, FR-033).
  (depends on T048)
  Verify: `bash test/time_source_gate.sh .` prints `time_source_gate: clean` and exits 0, `grep -nE "SG_(REQUIRE|ENSURE|INVARIANT)\(.*\] *\(" source/harness/spectrum.cpp source/harness/jitter_probe.cpp` reads no match, and `grep -rnE "nlohmann|rapidjson" source/harness/spectrum.cpp source/harness/detail/spectrum.hpp` reads no match
- [ ] T054 Run the docs gates, their fixtures and the shape folds over
  the branch, in the `build` directory T048 configures (FR-023, FR-029,
  SC-007).
  (depends on T039, T040, T041, T042, T043, and on T048 for the build
  directory)
  Verify: `ctest --test-dir build -R "example_drift|example_drift_fixtures|snippet_compile|example_run|example_run_fixtures|report_shape_fwq_ftq_sample|report_shape_jitter_sample|report_shape_fixtures" --output-on-failure` exits 0, each entry printing `***Passed` and the fold printing no difference, and `python3 tools/prose/prose_gate.py --base origin/master` reads `0 findings` and exits 0
- [ ] T055 [P] Run the prose gate, the format gate and the spell gate
  over the branch range (constitution V, XI, FR-033).
  (depends on T048)
  Verify: `python3 tools/prose/prose_gate.py --base origin/master` reads `0 findings` and exits 0, `clang-format --version` prints `18.1.8`, the version CI pins and the version `tools/ci/prepush.sh:186-194` requires, then `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake` exits 0 with no file named, and `cmake -P cmake/spell.cmake` exits 0
- [ ] T056 Run the pre-push script over the lane, both its forms, and the
  FR-034 diff-scope check: the lane adds no option row, no option value,
  no dependency, no CI change and no gate-configuration change, and moves
  no cap, threshold, preset, job, exclusion and allowance line
  (constitution VIII, FR-033, FR-034).
  (depends on T048 to T055)
  Verify: `bash tools/ci/prepush.sh` and `bash tools/ci/prepush.sh --full` print every step ok line and exit 0, and `git diff --name-only origin/master...HEAD | grep -cE '^(\.github/workflows/|CMakePresets\.json|CMakeUserPresets\.json|vcpkg\.json|tools/dbc/coverage_gate\.sh|tools/prose/prose_rules\.yaml|tools/noise/jitter_probe_acceptance\.sh|test/time_source_gate\.sh)'` reads zero, with `grep -c "PAGE_LINE_CAP = 1004" tools/docs/example_drift.py`, `grep -c "SECTION_LINE_CAP = 25" tools/docs/example_drift.py` and `grep -c "allowance=21" tools/dbc/coverage_gate.sh` each reading one line
- [ ] T057 Stand the acceptance record beside the lane and keep the
  script off CI: T045 runs the script once, and this task reads the record
  that run wrote (FR-027, FR-028, SC-008).
  (depends on T044, T045)
  Verify: `grep -c "^## " specs/043-fwq-ftq/results/acceptance-<date>.md` reads eight, each section showing a drop count of 0, and `grep -c "fwq_ftq_acceptance" .github/workflows/ci.yml` reads no match
- [ ] T061 Push the lane and name the run reference: push the branch, and
  push again with `--force-with-lease` after the first push, the standing
  history rule of the Constraints, then record the run reference the CI
  read of T058 names (constitution History, FR-028).
  (depends on T056, T057)
  Verify: `git push --force-with-lease origin 043-fwq-ftq` exits 0, and `gh run list --branch 043-fwq-ftq --limit 1` prints one run id, which stands recorded in `specs/043-fwq-ftq/results/ci-run.md`

**Checkpoint**: every hard gate stands green at the head, and the pushed
head stands recorded with its run reference.

---

## Phase 11: Convergence

**Purpose**: the closing evidence the lane lands with. T061 pushed the
head, T058 read its CI run, T062 opens the pull request, and T059 re-checks
the landed tree.

- [ ] T058 Read the CI run of the pushed head end to end: every job
  green, the coverage figures reading 100.0% lines and 100.0% branches,
  the 2-cpu host reading of the `test` job standing recorded, and the run
  reference standing beside the acceptance evidence under
  `specs/043-fwq-ftq/results/` (constitution VIII, FR-024, FR-028).
  (depends on T048 to T057, and on T061 for the pushed head)
  Verify: the run reads every job green, naming all 13 jobs of `.github/workflows/ci.yml`: `lint`, `coverage`, `sanitize`, `tsan`, `static-analysis`, `test`, `test-rocky`, `shared-audit`, `downstream-consumer`, `consumer-release`, `dbc-gate`, `prose-lint` and `docs`; `gh run view <run-id> --json jobs --jq '.jobs[].conclusion' | grep -c '^success$'` reads 13; and each verdict stands in `specs/043-fwq-ftq/results/ci-run.md` beside the run reference
- [ ] T062 [US7] Open the pull request for the branch: the body records
  the local counts, the host and the kernel, names the eight acceptance
  sections and the run reference, and links every record (FR-028, SC-008).
  (depends on T058)
  Verify: `gh pr create --head 043-fwq-ftq --title "Docs: Fixed-quanta probe lane" --body-file specs/043-fwq-ftq/results/pr-body.md` exits 0 and prints one pull-request url, and `gh pr view 043-fwq-ftq --json body | grep -c "results/acceptance"` reads at least one
- [ ] T059 Re-check the constitution table of [plan.md](./plan.md)
  against the landed tree: every gate stands as the table states it, and
  the Complexity Tracking table stands empty (constitution Governance).
  (depends on T058, T062)
  Verify: the re-check reads every row green at the feature head, and `grep -c "PASS" specs/043-fwq-ftq/plan.md` reads at least the eleven principle rows

**Checkpoint**: the lane stands closed: the CI run reads green and the
record stands beside the evidence.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 (M-A)**: no dependency; blocks every later phase
- **Parallel block one**: Phase 2 (M-B) and Phase 3 (M-E) depend on
  Phase 1 alone and land in parallel; M-B needs the option fields and
  M-E needs no module
- **Phase 4 (M-C)**: depends on Phase 1 and Phase 2 (the probe body
  reads the method, the quantum, the work count and the noise
  threshold)
- **Phase 5 (M-D)**: depends on Phase 4 (the fold reads the series
  buffer the loops fill)
- **Phase 6 (M-G)**: depends on Phase 1, Phase 5 and Phase 3 (the
  writer reads the fold, and the periods ride the kernel answer)
- **Phase 7 (M-F)**: depends on Phase 4, Phase 5 and Phase 3 (the
  console prints the figures of the fold and the period list)
- **Phase 8 (M-H)**: depends on every module above, because the
  example records the lines the printer emits
- **Phase 9 (M-I)**: T044 needs the built binary and the lane host;
  T045 depends on T044; T046 depends on T045; T047 depends on the
  surface, the parser, the schema, the rule table, the printer, the pair
  and the fold, the seven commits that carry the contract deltas
- **Phase 10**: depends on every phase standing green; T061 pushes the
  head once T056 and T057 stand green
- **Phase 11**: depends on Phase 10; T058 reads the pushed run, T062
  opens the pull request, and T059 re-checks the landed tree

### Parallel Blocks

- **Block one (after Phase 1)**: Phase 2 and Phase 3 stand apart
  behind the seams the standing code owns; Phase 2 stands in the
  parser and the grammar block of `harness_jitter_cli_test.cpp`, and
  Phase 3 stands in the two new kernel files and the new
  `harness_spectrum_test.cpp`, which T060 registers first
- **Parallel pairs**: T001, T002, T005 and T008 run in parallel across
  four files; T019 runs alone in its file; T039 runs alone on the main
  page; T050, T051, T052, T053 and T055 run in parallel once T048
  stands green, each in its own binary directory
- **Sequential chains**: T011, T012 and T013 share
  `harness_fwq_ftq_test.cpp` and run in one worker; T014 follows T011;
  T023, T024, T025 and T026 form a four-file chain, each row reading
  the answer of the row before; T033 then T034; T039 then T040 across
  the two pages; T048 then T049 and T054, the three sharing the
  `build` directory
- **Same-file test tasks**: T006, T009, T011, T012, T013, T023, T027
  and T033 stand behind a disjoint scenario block of a file an earlier
  task opens, so they follow that task in one worker: T001 opens
  `harness_fwq_ftq_test.cpp` for T011, T012 and T013; T005 opens
  `harness_jitter_cli_test.cpp` for T006; T008 opens
  `harness_spectrum_test.cpp` for T009; T019 opens
  `harness_jitter_window_test.cpp` for T023, T027 and T033

---

## Parallel Example: Parallel Block One

```text
Task: "T005 to T007: the value grammar and the 16 refusals (module M-B)"
Task: "T008 to T010: the spectrum kernel as a pure function (module M-E)"
```

The two phases touch disjoint files: M-B stands in the parser arms of
`source/harness/jitter_probe.cpp` and the grammar block of
`test/source/harness_jitter_cli_test.cpp`, and M-E stands in
`source/harness/detail/spectrum.hpp`, `source/harness/spectrum.cpp`
and `test/source/harness_spectrum_test.cpp`. Neither shares a build
output with the other, and both stand behind the Phase 1 surface.

---

## Implementation Strategy

### MVP First (User Stories 1 and 2 only)

1. Complete Phase 1: the public surface and the numbers
2. Complete Phase 2: the grammar and the refusals
3. Complete Phase 4: the calibration, the FWQ loop and the FTQ loop
4. **STOP and VALIDATE**: a run with `--jitter-probe=idle,method=fwq`
   over the scripted reader measures the calibration and the unit
   series, and the figures match the scripted series
   (quickstart.md, the fixed work quanta run)

### Incremental Delivery

1. Phase 1: the types and the numbers stand
2. Parallel block one: the grammar and the kernel stand (two workers)
3. Phase 4 and Phase 5: the probe body and the folds close User
   Stories 1 and 2
4. Phase 6: the record closes User Story 4 in the document
5. Phase 7: the console and the pre-check close User Stories 1, 2 and
   4 on the console
6. Phase 8: the docs and the example teach the two methods
7. Phase 9: the evidence and the standing contracts close User
   Story 7
8. Phase 10 and Phase 11: the gates and the CI run close the lane

---

## Notes

- [P] tasks stand in disjoint files and share no build directory, no
  CMake target, no schema file, no README section, no docs page, no
  option table, no rule table, no version block and no coverage
  configuration; a chain that shares one of these carries no [P]
- Three ids stand appended after T059: T060 registers
  `harness_spectrum_test`, T061 pushes the head, and T062 opens the
  pull request
- [Story] labels map the tasks to the spec user stories
- Every test task stands observed failing before its code lands, and
  the test and its code land in one commit (Principle III, VI)
- Commit after each task or logical group, with the standing commit
  template; the T004 commit message records the version check and the
  ABI check with the four layout moves named, the T030 commit message
  records the schema check beside the `1.12` minor (FR-005, SC-009),
  and the six contract deltas ride the code commits of T003, T007,
  T030, T031, T035, T037 and T043, which T047 checks (FR-030)
- The tasks phase reads the plan waves, the module order and the test
  plan as one graph: wave 1 stands in Phase 1 and Phase 2, wave 2 in
  Phase 3, wave 3 in Phase 4, wave 4 in Phase 5 and Phase 6, wave 5 in
  Phase 7, wave 6 in Phase 8, wave 7 in Phase 9 and Phase 10
- Three readings stand recorded for the implement phase, each read
  from the plan and the contracts, which govern: the public record
  type stands `JitterSeriesStats` (plan.md R-20,
  [contracts/public-api.md](./contracts/public-api.md)), the history
  being that `data-model.md` first named the pair
  `JitterSeriesRecord` and `JitterCalibration` and now uses
  `JitterSeriesStats` for the series entity and titles the calibration
  entity E-22; the option field types stand as the contract block
  states them, `workCount` a `std::uint32_t` at 0 naming the
  calibration and `noisePercent` a `std::uint32_t` at 1, and the
  series figure `noisePercent` stands a real, the two sharing one
  name; the calibration prefix stands at 98 reads at the cap, the plan
  risk reading, and the test builder takes the prefix length as a
  parameter, so each test states its own
- Stop at any checkpoint to validate the stories independently
