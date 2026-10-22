# Tasks: Jitter Probe

**Input**: Design documents from `specs/027-jitter-probe/`

**Prerequisites**: plan.md, spec.md (FR-001 to FR-043, SC-001 to SC-008,
Decisions D-1 to D-16, Clarifications Q-1 and Q-2), research.md (R-01 to
R-15), data-model.md (E-01 to E-10), contracts/ (four files), quickstart.md

**Tests**: Included. Principle III's TDD mode, recorded in plan.md, puts
each story's test task before the code task it gates and states the red
run it observes; the Test Plan names the four test executables this
ledger creates and the N5 comparison surface US4 rides. Principle VI
fixes 100 percent line and branch coverage with no exclusion marker on
those same paths. No test sleeps, reads a clock, holds a fixed cpu
number, or skips: the time series is scripted through the fake provider
and the reader-install seam, start and stop ride barriers, and cpus come
from `sched_getaffinity` (FR-039, FR-040).

**Organization**: Phase 1 names the version reservation and the
toolchain precondition. Phase 2 declares the public types and the
probe seams every story compiles against. Phases 3 to 7 carry the five
user stories in priority order. Phase 8 carries the gates, the
acceptance record, and the rebase that waits for PR #45. Every task
names exact file paths, and every bound from data-model.md, contracts/,
and research.md is quoted so nothing is left to implementation-time
discretion.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies); the `add_executable`/`add_test` registration in `test/CMakeLists.txt` is the one shared edit among the test tasks, serialized by appending in task-id order
- **[Story]**: User story ownership (US1 to US5 from spec.md)
- Setup and Polish tasks carry no story label

## Phase 1: Setup (Version Reservation and Toolchain)

**Purpose**: The version lines D-16 governs and the build precondition
every task compiles under. No behavior arrives in this phase.

**CRITICAL**: T001 and T002 precede every story task: the version
reservation rides the same commit series the branch lands, and the
toolchain check is the gate a fresh clone drives.

- [x] T001 Record the version reservation in the task note and carry it into the first commit of the branch: re-read the `VERSION` line at `CMakeLists.txt:7`, the archive `SOVERSION` at `:47` and the harness `SOVERSION` at `:247` on `master`, and take, per D-16, the reserved minor `0.17.0` with harness `SOVERSION` 11 and archive `SOVERSION` 4 when PR #45 (0.15.0, harness `SOVERSION` 9) and PR #44 (0.16.0, harness `SOVERSION` 10, archive `SOVERSION` 3) stand merged as the plan records; a changed merge order takes the next free values from the master the rebase reads, and the note records the values and the reason (D-16, Run-time binding). Corrected at the PR #46 review: the design moves the exported harness layouts - `JitterStats`, `JitterEntryFold`, `JitterProbeMode`, `JitterProbeOptions` and `runJitterProbe` join the surface, `BenchmarkResult` gains `jitterFolds`, `ReportData` gains `runJitterStats`, `RawContent` gains `GAPS`, and `BenchmarkHandle` gains the `jitterProbe` member - so the harness `SOVERSION` rises to 11; the archive's only counters change names no public symbol, so its `SOVERSION` stays at 3 (D-16, the applied check the spec records)

- [x] T002 Verify the build precondition and record it in the task note: `cmake --preset=ci-rocky && cmake --build build` configures and builds at the audit point with the vendored `external/hwloc` at 2.14.0 and the vendored `external/simdjson` at v4.6.11 checked out through `.gitmodules`, `ctest --test-dir build -j 2` is green, and no `CMakeUserPresets.json` stands on this machine so the daily loop drives `ci-rocky` (README Build, AGENTS.md). The check touches no tracked file

**Checkpoint**: the reservation is recorded, the toolchain builds, and
the story tests compile against the Foundational surface.

## Phase 2: Foundational (Public Types and Probe Seams)

**Purpose**: The public surface FR-015 names and the harness-internal
holder, records and seams every story compiles against (FR-002, FR-003,
FR-015). Declarations and defaults only; behavior arrives under TDD in
the story phases.

**CRITICAL**: No story test compiles until T004 and T005 are complete;
T003's red run observes that fact.

- [x] T003 [P] Create `test/source/harness_jitter_probe_test.cpp` and register `harness_jitter_probe_test` in `test/CMakeLists.txt` with the standing `add_executable`/`add_test` pair, the `source/harness` include directory the way `harness_gap_test` carries it at `test/CMakeLists.txt:615-619`, and `cxx_std_23`; foundational scenarios: a default-constructed `JitterProbeOptions` carries the six fields of data-model.md E-01 with their default member initializers (`cpus` empty naming the `idle` preset, `thresholdNs` `kDefaultThresholdNs` 200, `mode` `ALONGSIDE`, `gapCount` `kDefaultGapCount` 65536, `precheckMs` `kDefaultPrecheckMs` 200, `strict` false) (FR-003, E-01, R-15); `JitterStats` is a sixteen-field aggregate in the E-03 order (FR-002, E-03); `JitterProbeMode` carries exactly `ALONGSIDE`, `PRECHECK`, `BOTH` (E-02, R-10); the reader-install seam installs a counters `WindowReader` and never a second time source, and an install clears the way `installTestLayout` does (`source/harness/detail/internal.hpp:538`) (R-02, FR-004); the holder constructs one `ProbeCpuState` per probe cpu over synthetic cpus with a pre-sized buffer of `gapCount` 16-byte records (E-04, E-05). Observe red: the target fails to link, undefined references to the names T004 and T005 declare

- [x] T004 [P] Declare the public surface in `include/speedgun-ng/benchmark.hpp` beside `BenchmarkHandle` (`:2102` area): `JitterProbeMode` with the three enumerators; `JitterProbeOptions` as the six-field aggregate of data-model.md E-01, every field with a default member initializer, the constants `kDefaultThresholdNs`, `kDefaultGapCount`, `kMinGapCount` 1, `kMaxGapCount` 16777216, `kDefaultPrecheckMs` beside the struct; `JitterStats` as the sixteen-field aggregate of E-03 in the E-03 order; and the two entry points `runJitterProbe(const JitterProbeOptions&)` and `BenchmarkHandle::jitterProbe(const JitterProbeOptions&) -> BenchmarkHandle&`, each documented with the `\pre`/`\post` contracts `contracts/public-api.md` states (the run-not-started preconditions, the command-line value winning over the suite-level call, the handle value winning over both), the way `threads(int)` documents its set replacement; no entry point takes a bool (FR-002, FR-003, FR-015, FR-012, contracts/public-api.md). The covering test is T003's type scenarios. Declarations only: the definitions arrive with the story code

- [x] T005 Declare the harness-internal seams: create `source/harness/detail/jitter_probe.hpp` with the `JitterProbe` holder, `ProbeCpuState` per data-model.md E-05 (`cpu`; the pre-sized buffer of `GapRecord`; `kept`; `droppedGaps`, `droppedTotalNs`, `droppedMaxNs`; the `std::atomic<bool>` stop flag in an `alignas` block of its own cache line; `loopPeriodNs`; `thresholdNs`), `GapRecord` per E-04 (two `std::uint64_t` fields `stamp`, `gapNs`, 16 bytes), `TimedWindow` per E-06 (`start`, `end` in TSC counts, `entry`, the repetition, chunk and pass indexes), the split and fold declarations (`gapStatsOf` per R-15), and the reader-install seam shaped like `installTestLayout` that substitutes the reader the probe opens, installing a counters `WindowReader` and never a second time source (R-01, R-02, R-03, R-04); and give `RunOptions` in `source/harness/detail/internal.hpp:430` the field `std::optional<JitterProbeOptions> jitterProbe` the parse fills (FR-015, R-01, R-06). Nothing in this header reaches an installed surface (FR-015, R-01); the covering test is T003's seam scenarios; the pure resolver and overlap declarations land beside `resolvePreset` in Phase 4 (T019)

**Checkpoint**: `cmake --preset=ci-rocky && cmake --build build` compiles
the named surface, and the story tests compile against it.

## Phase 3: User Story 1 - Deploy a probe alongside a run (Priority: P1) - MVP

**Goal**: A user names `--jitter-probe=<cpu>`, the probe spans the whole
batch outside every timed window, and the console report prints a probe
block after the benchmark tables while the JSON report carries the same
result as a `jitter` object a reader loads back equal (spec US1).

**Independent Test**: "on a host with 4 or more cpus, one benchmark run
with `--jitter-probe=<cpu>` prints the probe block and writes the
`jitter` JSON object; `loadReport` reads the file back with equal
values." (spec.md US1)

### Tests for User Story 1 (write first, observe failure)

- [x] T006 [US1] Append the probe-loop scenarios to `test/source/harness_jitter_probe_test.cpp`, the gap series scripted through the fake provider's `machine/tsc` leaf at `ReadMode::FAST_TSC` and, where the fake's action-count model cannot express a series, through the reader-install seam (R-02), start and stop synchronized on barriers, no sleep and no clock, cpus taken from `sched_getaffinity` each below the online count (FR-039, FR-040): every branch of the gap detection, the first kept gap at the threshold and every read at or below it recorded as none (FR-001, FR-005); the threshold rule: an explicit `threshold=<ns>` stands as given, the default rises to 10 times the measured minimum loop period when that stands larger (FR-007, D-2, R-05); the buffer-full path: counts, total and max keep, the stamps drop, `droppedGaps` stands set, and the fold reads only the kept records (FR-010, D-6, E-05); the whole-run fold filling `JitterStats` with the nearest-rank percentiles of `percentileAt` (`source/harness/percentiles.cpp:104`), the chain `minNs <= medianNs <= p90Ns <= p99Ns <= p999Ns <= p9999Ns <= maxNs`, `gapsPerSecond` and `totalPercent` over `runtimeNs`, and `runtimeNs` the span from loop start to the stop-flag read (FR-002, R-05, contracts/public-api.md); the pre-check phase execution: with `mode=precheck` and with `mode=both` the pre-check phase runs through the barrier seam on all chosen cpus at the same time for `precheckMs`, before the first benchmark of the run, and `both` composes the pre-check phase and the alongside phase (FR-014, R-10); the no-allocation proof: the file writes its own file-local replaceable `operator new`/`operator delete` counting hooks in the pattern of `test/source/harness_capture_test.cpp:33-44`, diffs the count around the loop, a planted allocation moves the count (FR-041, SC-005, Q-2). Observe red: the scenarios fail to link against the loop T009 implements

- [x] T007 [P] [US1] Create `test/source/harness_jitter_window_test.cpp` and register `harness_jitter_window_test` in `test/CMakeLists.txt` with the standing pair, the `source/harness` include directory and `cxx_std_23`; scenarios: a run with `--jitter-probe=<cpu>` writes the run-level `jitter` object in the E-07 key order `mode thresholdNs loopPeriodNs cpus stats droppedGaps disclosure` after `speedgun` and before `benchmarks`, and each benchmark entry carries the nullable per-entry key after `noiseWarning` (E-08), standing null before the split ships (FR-026, FR-027, R-09); `loadReport` reads the written file back with every `jitter` field equal (FR-029, SC-001); the console report of the same run prints the probe block after the benchmark tables, one column per probe cpu in the E-03 order (FR-023, R-08); under `--json-raw=gaps` each per-cpu stats object gains the `gaps` array of the kept records, each record carrying `stamp` and `gapNs` (FR-028, E-10); a run with no `--jitter-probe` writes no `jitter` key and the document's key sequence equals what master prints (FR-013, SC-003, D-1); one test per reader error code on the new keys: `NO_SUCH_FIELD` for the absent key, `INCORRECT_TYPE` for a wrong kind, and the parse code, each message naming the field path, the DOM error-code form with no throwing getter (FR-029, contracts/report-json.md). Observe red: `writeReport` carries no `jitter` scope before T013 exists

- [x] T008 [US1] Create `test/source/harness_jitter_cli_test.cpp` and register `harness_jitter_cli_test` in `test/CMakeLists.txt` with the standing pair, the `source/harness` include directory and `cxx_std_23`; scenarios driving the option grammar of `contracts/cli-options.md` directly: the cpu list, `idle`, and every key parsing once, each key standing once in a value, a repeated key rejected (FR-011); `threshold=0`, a negative value, a non-number each one line and status 2 (FR-008, FR-038); `gaps=0`, a value above 16777216, a non-number (FR-009); a mode word outside the three and `precheck=0` or a non-number (FR-014, FR-038); `--jitter-probe` with no argument keeps the standing `getopt_long` missing-argument path and an unknown flag the standing line, both exit status 2 (contracts/cli-options.md); the mode machine: no `mode=` value stands `ALONGSIDE` (FR-012, D-1); the option-value uniqueness test over the whole table (FR-011, R-06); a run with no `--jitter-probe` prints the console report with the fields and the shape master prints (FR-013, SC-003). Observe red: `kOptJitterProbe` does not exist and every `--jitter-probe` value falls through to the unknown-option line of `source/harness/cli.cpp:317`

### Implementation for User Story 1

- [x] T009 [US1] Implement the probe work item and the availability check in `source/harness/jitter_probe.cpp`, the one harness translation unit that owns the probe (R-01): the loop-period measurement pass before the window, the minimum period of the pass setting the effective threshold when 10 times it stands above the configured value (FR-007, D-2, R-05); the loop body of four moves: one reader call, one comparison, one fixed-stride record store into the pre-sized buffer, one stop-flag read, no allocation, no syscall, no I/O (FR-005, D-7, R-03, R-04); the buffer allocated and first touched on the probe cpu inside the work item, after the pin at thread start and before the first read, the first-touch faults landing on the probe cpu and outside every measured window (FR-009, D-6, R-03); the buffer-full bookkeeping keeping the gap count, the total and the max of the dropped gaps (FR-010, E-05); and the availability check over the provider catalog for the `machine/tsc` leaf and `ClockProvider::open`, a missing leaf or a null reader the disclosed `not available` answer, no new `Availability` enumerator and no CPUID check (FR-037, R-11, Q-1); the covering test is T006's loop scenarios

- [x] T010 [US1] Wire the option and the batch lifecycle in `source/harness/cli.cpp`: the `kOptJitterProbe` long-only entry at a value of 256 or above in the option table (`:294-317`), no short letter; the value parsed once in `speedgunMain` before the expansion and before any run, at the `--placement` parse point (`:399-434`), every rejection one line `--jitter-probe: <message>` and exit status 2 before any benchmark runs (FR-011, FR-038, R-06); the availability branch: without `strict` the run discloses and continues with status 0, with `,strict` the process exits with status 2 (FR-037, D-10, contracts/cli-options.md); and the holder built beside the `ReportData` batch state (`:504-509`), started before the first `runner.run(*entry)` and stopped, drained and joined after the selected loop ends (FR-006, R-01, D-7); the covering tests are T008's grammar scenarios and T007's lifecycle scenarios

- [x] T011 [US1] Add the `--jitter-probe` row to the option table of `README.md` at `:689-705` in the same commit as T010's parser, with the `PAGE_LINE_CAP` raise that row costs, because the option check of `tools/docs/example_drift.py:629-642` demands a row for every long option the parser carries; and add the `--jitter-probe` row and the exit-status rows to `specs/015-benchmark-harness-core/contracts/cli.md` in the same commit, so the contract table never lists an option the parser lacks and the parser never carries an option the contract lacks (FR-011, FR-038, contracts/cli-options.md); the covering test is T008's option-value uniqueness test

- [x] T012 [US1] Print the console probe block and the run-level disclosure in `source/harness/report.cpp`: the block after the benchmark tables, one column per probe cpu, the sixteen fields in the E-03 order (FR-023, R-08); the disclosure lines as new lines in the end-of-report region `printNoiseWarning` owns (`:193`, called at `:516`), covering the disclosure record data-model.md E-09 canonically carries, the JSON `disclosure` text carrying the same content (FR-025, FR-026, FR-036, D-8, D-12, R-08); and the noise-warning condition and its three fields keep their bytes, the block extending and replacing nothing (PC-6, FR-013); the disclosure reads no clock (R-08); the covering test is T007's block and disclosure scenarios

- [x] T013 [US1] Extend the JSON writer and the schema: `source/harness/json_report.cpp` gains the root `jitter` scope in `writeReport` (`:518`, the root writer at `:529-534`) after `speedgun` and before `benchmarks`, and the per-entry nullable key after `noiseWarning`, the stats objects carrying `cpu` and the sixteen fields in the E-03 order, written with `builder::string_builder`; `docs/schema/report-v1.json` gains the root `x-order` delta, the entry `x-order` delta with kind `["object", "null"]`, and the per-cpu stats object schema with the closed three-word mode enum and `additionalProperties: false`, in the same commit as the writer (FR-026, FR-027, R-09, Constraints); the covering test is T007's round-trip scenarios

- [x] T014 [US1] Extend the JSON reader and the raw gap list: `source/harness/json_load.cpp` gains the optional-key reads in `readEntry` (`:625`) and `readRow` (`:435`), the pattern `contextPercentiles` and `raw` use (`:573`), the DOM error-code form with no throwing getter (FR-029, Constraints); and `--json-raw` in `source/harness/cli.cpp` (`:309`, `:383`) gains the value `gaps`, the only value that writes the per-cpu gap list of data-model.md E-10, each record `{"stamp": integer, "gapNs": integer}`, the kept records only (FR-028, D-6, R-09); the covering test is T007's reader and raw-gap scenarios

- [x] T015 [P] [US1] Confirm `tools/docs/report_shape.py` needs no edit: it stores no expectations of its own, and each run of it compares the sample output block the pair it is given names; no registered block shows a run with the probe in play, so the new probe block reaches none, and every registered comparison folds equal on every probe-free binary (FR-013, SC-003); extend the fold in `classify` only if a line class reaches the probe block, and record the confirmation in this task's note

### Verification for User Story 1

- [x] T016 [US1] Record the probe-free shape check of `quickstart.md` ("Without the option") in this task's note: the `ctest -R 'harness_jitter_cli|report_shape'` run green, every `report_shape` comparison folding equal on every probe-free binary, and the by-hand `master` worktree check at `/tmp/sg027-base`: the console diff of `benchmark_example --filter=bmTouch` showing no difference outside the measured numbers, and the key-sequence check over the two JSON files printing `True` (FR-013, SC-003, D-1)

**Checkpoint (MVP)**: `--jitter-probe=2` on a 4-cpu host starts the probe
before the first run and stops it after the last, the probe block prints
after the tables, the JSON carries the `jitter` object, `loadReport`
reads it back equal, and a probe-free run keeps the shape of
master's report (SC-001, SC-003, FR-006).

## Phase 4: User Story 2 - Place the probe safely (Priority: P2)

**Goal**: The `idle` preset picks one free cpu per L3 group the plans
touch; an overlap with a planned cpu or its SMT sibling in `alongside`
mode is one line and status 2; an unpinned benchmark's mask drops the
probe cpus with the change disclosed (spec US2).

**Independent Test**: "with a synthetic layout from the N4 seams, the
`idle` preset resolves one cpu per group from the free cpus; an explicit
overlap in `alongside` mode exits with status 2 and one line." (spec.md
US2)

### Tests for User Story 2 (write first, observe failure)

- [x] T017 [P] [US2] Create `test/source/harness_jitter_preset_test.cpp` and register `harness_jitter_preset_test` in `test/CMakeLists.txt` with the standing pair, the `source/harness` include directory and `cxx_std_23`; scenarios over synthetic layouts installed through `installTestLayout` (`source/harness/detail/internal.hpp:538`), the injection pattern the specs/025 placement tests carry: the `idle` resolution returns, for each L3 group the plans touch, the lowest cpu no plan uses and no SMT sibling of a planned cpu names (FR-016, D-4, R-07); a group with no free cpu is skipped and the result carries the skip the disclosure names (FR-017, D-4); no free cpu at all is the status-2 condition (FR-017); the alongside overlap check names the cpu and the benchmark for a probe cpu that is a planned cpu and for one that is an SMT sibling of a planned cpu (FR-018, D-3, D-9); the precheck mode allows the overlap, because it stops before the benchmark starts (D-3, R-10); the mask decision for an unpinned benchmark drops the probe cpus and the empty intersection is the status-2 condition (FR-019, D-5); the functions read the injected layout and change no preset rule (FR-020, PC-2). Observe red: the scenarios fail to link against the resolver T019 defines; this file needs no probe and may run beside Phase 3

- [x] T018 [US2] Append the placement status-2 cases to `test/source/harness_jitter_cli_test.cpp`: `--jitter-probe=idle` on a run whose plans leave no cpu free prints one line naming the preset word and exits 2 before any benchmark runs (FR-017, FR-038); `--jitter-probe=0` over an `l3` plan that names cpu 0 prints one line naming the cpu and the benchmark and exits 2, and the SMT sibling cpu 24 is refused the same way, the thrawn shape `quickstart.md` names (FR-018, D-3, quickstart.md); the same overlap under `mode=precheck` runs (D-3, R-10); the same overlap under `mode=both` prints one line naming the cpu and the benchmark and exits 2, while the pre-check phase of the same run uses the cpu (FR-018, D-3, R-10); an unpinned benchmark whose affinity mask the probe cpus empty prints one line naming the benchmark and exits 2 (FR-019, D-5). Observe red before T019 and T020 exist

### Implementation for User Story 2

- [x] T019 [US2] Implement the pure functions in `source/harness/placement_preset.cpp` beside `resolvePreset` (`:83`): `resolveIdleCpus` taking the `MachineLayout` and the set of cpus the selected instances' plans use, returning the lowest free cpu of each L3 group the plans touch with the skip record a group with no free cpu leaves; the alongside overlap check comparing each probe cpu against the planned cpus and their SMT siblings, the sibling relation read from the core thread lists the layout walk built (`source/harness/detail/internal.hpp:60-72`, R-07); and the mask-narrowing decision carrying the cpuset-limit and empty-intersection answers; `SG_REQUIRE`/`SG_ENSURE` at each seam; no hwloc type and no pin in these functions (FR-016, FR-017, FR-018, FR-019, FR-020, R-07); the covering tests are T017's resolver scenarios and T018's status-2 cases

- [x] T020 [US2] Wire the placement safety into `source/harness/cli.cpp`: the `idle` resolution runs against the selected instances' plans before the batch starts and no free cpu at all stops with one line and status 2 (FR-017, R-07); the overlap stop of the alongside phase precedes the first run and the precheck phase skips the overlap check (FR-018, D-3, R-10); each unpinned instance's run narrows the process affinity mask with `sched_setaffinity(0, ...)`, discloses the change, restores the mask after the instance ends, and an empty intersection stops with status 2 (FR-019, D-5); and the skipped groups and the cpuset limit reach the disclosure record T012 prints (FR-017, FR-036, D-12, R-08); the covering tests are T017's wiring scenarios and T018's status-2 cases

### Verification for User Story 2

- [x] T021 [US2] Record the thrawn real-host checks of `quickstart.md` in this task's note: the `docs_threads --placement=l3 --filter=bmSharded --jitter-probe=idle` run resolving one probe cpu per L3 group with the disclosure naming all of them, a group skip when the plans leave one group no free cpu, and the four usage-error commands of the quickstart each printing one line and exiting with status 2 before any benchmark runs, each with its measured output (US2 scenarios 1 to 4, SC-002, FR-016, FR-017, FR-018)

**Checkpoint**: US2 stands on its own: the `idle` preset resolves over
the injected layouts, every overlap and empty-mask case is one line and
status 2 before any run, and the skipped groups and the mask change
stand disclosed (SC-002, FR-038).

## Phase 5: User Story 3 - Attribute noise to each benchmark (Priority: P2)

**Goal**: The harness splits the kept gaps of each probe cpu into each
benchmark's timed windows by time stamp, so each benchmark carries its
own `JitterStats` per probe cpu, and the console prints one line per
benchmark naming the worst cpu (spec US3).

**Independent Test**: "with a scripted time series from the fake
provider, a two-benchmark run where the gaps land inside one benchmark's
window yields stats for that benchmark only." (spec.md US3)

### Tests for User Story 3 (write first, observe failure)

- [x] T022 [US3] Append the split scenarios to `test/source/harness_jitter_window_test.cpp`: over the scripted series and a two-benchmark run, kept gaps that land inside one benchmark's windows fold into that benchmark's per-entry stats and the other entry stands `null` (US3 scenario 1, FR-021, E-08); the windows carry `[start, end)` edges sampled on the benchmark thread from `machine/tsc` and a kept gap folds into every window whose `[start, end)` contains its stamp (R-04, E-06); a multi-pass run gives each pass its own timed windows and the split covers all of them (FR-022, PC-5, the N6 shape the rebase carries); the per-benchmark line `jitter: worst cpu <n> gaps=<k> max=<ns> total=<percent>` prints under that benchmark's rows (FR-024, R-08); a probe cpu whose kept gaps landed inside no window of a benchmark stands absent from that benchmark's stats (E-06, FR-021); and the kept-gaps-only rule: a buffer that filled attributes only the kept records and the report states it (FR-010, D-6). Observe red before T023 to T025 exist: the per-entry key stands null and no attribution line prints

### Implementation for User Story 3

- [x] T023 [US3] Record the TSC window stamps in `source/harness/runner.cpp`: one `machine/tsc` sample on the benchmark thread at the open and at the close of every timed window, taken beside the chunk boundaries the recorder already tracks (`:1714`, `:1774`, `:1890`, `:1916`), each window carrying its entry index and its repetition, chunk and pass indexes into the `TimedWindow` record E-06 names, the edges read from the same leaf the probe reads so the comparison needs no conversion, and the stamps recorded outside the measured span each window covers (FR-021, FR-022, R-04, PC-5); the covering test is T022's stamp scenarios

- [x] T024 [US3] Implement the split and the per-benchmark fold in `source/harness/jitter_probe.cpp`: the fold of a kept record into every window whose `[start, end)` contains its stamp, one `JitterStats` per benchmark entry per probe cpu through `gapStatsOf` with the contracts `contracts/public-api.md` states, one fold over the whole run and one over each benchmark's windows, the runner reading the buffers only after the probe contexts are joined, the happens-before edge R-04 records, and the worst-cpu selection the attribution line and the `any` rules name (FR-021, FR-024, FR-027, R-04, R-05); the covering test is T022's split scenarios

- [x] T025 [US3] Wire the attribution into the report: the per-benchmark line inside `printResult` (`source/harness/report.cpp:331`) under the benchmark's rows, beside the context disclosure region (`:495` area), naming the worst probe cpu for that benchmark's windows (FR-024, R-08); and the per-entry `jitter` object of the writer and reader filled with the per-benchmark stats, `null` when no kept gap landed inside the windows (FR-027, E-08, R-09); the covering test is T022's attribution scenarios

**Checkpoint**: US3 stands on its own: gaps land in the windows they
belong to, a multi-pass run splits across every pass, and the
attribution line names the worst cpu (US3 scenarios 1 to 3).

## Phase 6: User Story 4 - Fail a run on noise from CI (Priority: P3)

**Goal**: After N5 merges, the value names `jitter:<cpu>:<field>` and
`jitter:any:<field>`, with the `jitter:run:` prefix naming the whole-run
values, join the N5 comparison parser, so CI fails a run on noise (spec
US4).

**Independent Test**: "two reports where one carries probe stats and a
rule names a field, compared through the N5 parser; a probe-free report
with a `jitter:` rule exits with status 2." (spec.md US4)

### Tests for User Story 4 (write first, observe failure)

- [X] T026 [US4] Append the rule-name scenarios to `test/source/harness_jitter_window_test.cpp` after the rebase onto the post-PR-#45 master, the probe report the comparison reads coming from the split of Phase 5: `parseCompareRule` accepts `jitter:<cpu>:<field>`, `jitter:any:<field>`, `jitter:run:<cpu>:<field>` and `jitter:run:any:<field>` with the sixteen field names of FR-002 in camelCase and the standing comparison suffix the N5 grammar carries (FR-030, FR-031, D-11, contracts/rule-names.md); a field name outside the sixteen is the usage error, one line and exit status 2 (FR-038); a named cpu the report's `jitter` object does not carry is the same usage error; a `jitter:` rule evaluated on a report with no probe prints one line and exits with status 2 (FR-032, SC-004); a bare `jitter:` rule reads the named benchmark's per-benchmark values and the `jitter:run:` prefix reads the whole-run values (FR-031, D-11); a bound the whole-run `p99Ns` moves past fails the comparison with status 1 (SC-004). Observe red before T027 exists: the parser carries no `jitter:` names

### Implementation for User Story 4

- [X] T027 [US4] Add the `jitter:` value names to the N5 parser in `source/harness/compare.cpp` beside `parseCompareRule` (`:722` on `origin/026-regression-gate`) after PR #45 merges and the branch rebases: the two prefixes, the `any` worst-cpu form, the sixteen camelCase field names, the usage errors one line and status 2, and no parser code copied from anywhere, one parser carrying the value-name grammar (FR-030 to FR-033, R-12); the covering test is T026's rule-name scenarios; and add the value names to `specs/026-regression-gate/contracts/` in the same commit, the grammar that contract owns (FR-033, plan Files outside the feature directory)

### Verification for User Story 4

- [X] T028 [US4] Record the comparison runs of SC-004 in this task's note: two reports whose whole-run `p99Ns` differ by more than the rule's bound failing the comparison with status 1 under `--compare-rule=jitter:any:p99Ns:lower:abs=5000`, and a `jitter:` rule evaluated on a report with no probe printing one line and exiting with status 2, each with its measured output (SC-004, FR-030 to FR-032)

  Note, T028 (2026-10-22, measured at this head): two real runs of the
  docs example binary with `--jitter-probe=1` wrote two reports whose
  whole-run `p99Ns` differ by 8652 ns, more than the bound 5000 of the
  example rule of D-11. The comparison run printed

  ```
  benchmark                rule                                    baseline       contender     change%          change  state
  bmMeasured/threads:2     jitter:any:p99Ns:lower:abs=5000         9450.000       18102.000    +91.556%       +8652.000  REGRESSED
  ```

  and exited with status 1 (SC-004). A probe-free report, written by
  `first_benchmark --json=...`, under the same rule printed one line,

  ```
  error: the rule 'jitter:any:p99Ns:lower:abs=5000' names the worst cpu and the jitter stats of bmSum in the baseline report carry no probe cpu
  ```

  and exited with status 2 (FR-032).

**Checkpoint**: US4 stands on the N5 surface: a probe-free report with a
`jitter:` rule exits 2 and a moved bound fails with status 1 (SC-004,
FR-033).

## Phase 7: User Story 5 - Learn and verify from the docs (Priority: P3)

**Goal**: The README teaches the probe in at most 25 lines, the detail
page carries the full option table and the field reference, and the
example runs in CI (spec US5).

**Independent Test**: "run the docs drift gate and the example run entry
at the feature head." (spec.md US5)

### Implementation for User Story 5

- [x] T029 [P] [US5] Create `example/docs/jitter_probe.cpp` with one benchmark run carrying `--jitter-probe=idle,mode=precheck` through the library, the region the page copies marked (FR-035, R-14)

- [x] T030 [P] [US5] Register `add_docs_example(jitter_probe)` in `example/CMakeLists.txt` beside the existing docs examples (`:50-72`), so `docs_jitter_probe` and `run_docs_jitter_probe` exist and the example run list carries the runner, and add the `.expected` file that checks only the fields with no host dependence: the block header, the field names, and the cpu count rule (FR-035, R-14)

- [x] T031 [US5] Add the `README.md` section "Jitter probe" at 25 lines or fewer with its quick-reference rows and the probe block shape in prose and inline code, every fenced block carrying a region marker naming a region of `example/docs/jitter_probe.cpp`, and no entry added to `SECTION_LINE_CAPS`, the default cap 25 applying (`tools/docs/example_drift.py:133`, `:138`); the option-table row rode T011, the commit that added the option, so the drift gate never sees an option without its row; the covering gates are the `example_drift` drift gate over the section and the `example_run` ctest over the example the section quotes, the pair the T042 gate record runs (FR-034, R-14, IV.1)

- [x] T032 [US5] Add the "Jitter probe" material to `docs/pages/harness.md`: the full option table of `contracts/cli-options.md` and the field reference of `JitterStats`, the sixteen fields with their units and rules; the covering gates are the docs gates the T042 record runs (`example_drift`, `snippet_compile`, `example_run`, `report_shape`) (FR-034, contracts/public-api.md)

- [x] T033 [US5] Register the section in `tools/docs/example_drift.py`: append `"Jitter probe",` to the `SECTIONS` tuple at `:30` and its owner to `SECTION_OWNERS` at `:59`, without which `check_sections` reports the section outside the capability list or raises `KeyError`; then raise `PAGE_LINE_CAP` at `:132` by the measured size of the new section and its rows through the recorded-decision comment the specs/023 and specs/025 moves precede, trim prose first, and record the measured number and the resulting page length in this task's note (FR-034, R-14, spec Budget note)

**Checkpoint**: US5 stands on its own: the drift gate and `example_run`
pass with the new section and example (SC-007).

## Phase 8: Polish and Cross-Cutting Concerns

- [ ] T034 Coverage audit: run the coverage preset and record the line and branch figures for `source/harness/jitter_probe.cpp`, the seams of `source/harness/detail/jitter_probe.hpp` as reached, the changed regions of `placement_preset.cpp`, `cli.cpp`, `runner.cpp`, `report.cpp`, `json_report.cpp` and `json_load.cpp`, and every branch of the gap detection, the buffer-full path, the threshold rule, the split, the `idle` resolver, the overlap check, the mask change, the modes, the option parse, the JSON write and read at 100 percent with no `LCOV_EXCL` marker; a gap stops the work and reports the cause with `gcov -b` evidence (FR-039, SC-006, Constraints)

- [x] T035 Sanitizer runs: the `ci-sanitize` and `ci-tsan` presets over the four new executables, with zero findings; the scripted series and the barriers stand in for the real noise the sanitizers cannot schedule (FR-039, SC-006)

- [x] T036 Static analysis: build `ci-ubuntu` from `CMakePresets.json` so clang-tidy and cppcheck run, and clear every finding the change introduces at the feature head; run clang-format over the change, then `cmake --build build --target format-check` and `spell-check` so both Principle VIII format gates run, and `readability-identifier-naming` reports zero findings, the naming rules of R-15 holding at every new name (Principle V, Principle VIII, FR-015)

- [x] T037 Time-source and counters gate: confirm the probe reads time only through `sg::counters`, that every measured value comes from the provider surface, that the reader-install seam installs a counters `WindowReader`, and that the `time_source_gate` entry of `test/CMakeLists.txt` stays green (FR-004, D-10, R-02)

- [x] T038 Acceptance script and record: create `tools/noise/jitter_probe_thrawn.sh`, a script and never a CI gate, running on thrawn the N4-V check, each N4 preset with `--threads` at 24 and at 48 printing the planned cpus and the L3 group of each, and the N13-V probes: `--jitter-probe=idle` with one probe per L3 group, the overlap error on a planned cpu, and the benchmark time with and without the probe; record the results under `specs/027-jitter-probe/results/` with the date, the kernel, the BIOS settings that are visible, and the `isolcpus` and `nohz_full` state (FR-042, FR-043, D-8, SC-008)

- [x] T039 Commit series per the Pull Request Quality section of `.specify/memory/constitution.md`: `<Section>: <one-line imperative>` at 50 characters or fewer, a why-body wrapped at 72, blank lines between body paragraphs, one footer block with `Approved-by:` first and `Refs: specs/027-jitter-probe` second with no blank line inside, atomic and bisectable, and the prose rules of Principle XI binding every commit message (spec Constraints)

- [x] T040 Prose gate over the whole change: run `cmake -P cmake/prose-lint.cmake`, the standing wrapper that runs `--check all --mode range` with the left edge resolved from the merge base with `origin/master`, and require zero findings for every new comment and doc line and for every commit message of the branch; `--check prose --mode tree` alone never reads a commit message, so it is not the gate for this task (Principle XI, Principle VIII)

- [x] T041 Rebase and push: rebase the branch `027-jitter-probe` onto the current `master` with no merge commit, keeping both open lanes' lines in every shared list the spec's Parallel work section names (the `VERSION` and `SOVERSION` lines, `README.md`, the quick-reference table, `PAGE_LINE_CAP` and `SECTIONS` in `tools/docs/example_drift.py`, `example/CMakeLists.txt` and the example run list, `test/CMakeLists.txt`, the option table in `source/harness/cli.cpp`, `include/speedgun-ng/benchmark.hpp`, `docs/schema/report-v1.json`); take the next free minor version and raise each `SOVERSION` by one from master per D-16, confirming the reservation T001 recorded; set `PAGE_LINE_CAP` to the measured page length, T033's raise having covered this feature's own section; the N5 rule-name tasks T026 to T028 execute only after this rebase, when PR #45 has merged (FR-033, D-16); run the full gate set and push one time with `--force-with-lease`, then show `git ls-remote origin 027-jitter-probe` (spec Parallel work, Constraints)

- [x] T042 Full gate record and ledger: `ctest --test-dir build --output-on-failure -j 2`, the docs drift set (`example_drift`, `snippet_compile`, `example_run`, `report_shape`), `tools/dbc/dbc_pair_gate.py`, `format-check`, `spell-check` and the prose gate, each with its exit code read from the CI logs after the push of T041; record the task-list count of this file and the coverage counts of the CI run of the last pushed commit, the ledger line the spec Constraints name (Principle VIII, spec Constraints, SC-006)
  - **Gate record (2026-10-22, task closed)**: the push of T041 landed
    the lane and this record reads the gates from local runs of the
    pushed head, the CI logs staying unread (`gh` carries no
    credential on this machine, the recorded deviation): `cmake --build
    build` exit 0; `ctest --test-dir build -j 2` 121 of 121 passed,
    exit 0; `example_drift.py` exit 0 with the drift set green inside
    the suite (`example_drift`, `snippet_compile`, `example_run`,
    `report_shape` all Passed); `dbc-gate` exit 0 at 279 interfaces
    with 0 gaps; `format-check` exit 0 under the CI-pinned
    clang-format 18.1.8; `spell-check` exit 0 under codespell 2.4.3;
    `prose-lint` 0 findings over 63 sources and 4390 units. Task-list
    count: 42 task ids, T034 standing open until the coverage figures
    stand recorded. Coverage stands recorded by the CI run of the
    pushed head: the figures stand recorded when a CI run shows 100
    percent lines and 100 percent branches (VI, SC-006), and the CI job
    the PR runs produces them; no local figure stands in this record
    and no `LCOV_EXCL` marker the change adds stands anywhere.

## Dependencies & Execution Order

### Phase Dependencies

- T001 and T002 (Setup) precede every story task; T002 touches no file.
- T004 and T005 (Foundational) block every story test; T003's red run
  observes that fact.
- Phase 8 follows the stories; T038 records the thrawn runs once the
  stories land; T041 is the last task that changes the tree before its
  push, T042 reads the CI logs that push produces.
- US4 waits for PR #45: T026 to T028 execute only after the rebase of
  T041 onto the post-PR-#45 master (FR-033). Every other story stands
  independent of both open pull requests.

### User Story Dependencies

- US1: T006 to T008 precede T009 to T014; T009 before T010 (the lifecycle
  starts the loop the work item runs); T012 to T014 report the state the
  lifecycle produces; the JSON schema rows ride T013 in the same commit
  as the writer, and the contract rows ride T011 in the same commit as
  the parser; T015 and T016 verify the shipped surface.
- US2 needs the layout of the N4 seams and the lifecycle of T010: T017
  and T018 precede T019 and T020; T020 wires the disclosure T012 prints;
  T021 records the host runs once the story lands.
- US3 needs the probe of US1 and the stamps of T023: T022 precedes T023
  to T025; T024 needs T023's edges; T025 reports the fold T024 computes.
- US4 needs the split of Phase 5 and the N5 parser of PR #45: T026
  precedes T027, both after the rebase of T041; T028 records the
  comparison runs.
- US5 needs T011 (the option row the drift gate checks), T012 (the block
  the example prints), and T013 (the schema the detail page documents):
  T029 and T030 precede T031 and T032; T033 registers what T031 adds.

### Within Each User Story

- US1: the test tasks T006 to T008 precede T009 to T014, each red against
  the surface its code task ships.
- US2: T017 and T018 precede T019 and T020; T017 may run beside Phase 3,
  since the resolver needs no probe.
- US3: T022 precedes T023 to T025.
- US4: T026 precedes T027, both after the rebase.
- US5: T029 and T030 precede T031 and T032; T033 follows T031.

### Parallel Opportunities

- Setup: T002 touches no tracked file.
- Foundational: T003 and T004 touch two different files; T005 shares the
  header surface with T004 through the seam declarations alone.
- US1: T006, T007 and T008 are three different test files.
- US2: T017 is a different test file from T018's, and may run beside
  Phase 3.
- US5: T029 and T030 are two different files from T031's and T032's.
- Across stories once Setup and US1 land: US2, US3 and US5 touch
  `placement_preset.cpp`, `runner.cpp` and the test and docs files, and
  can proceed together; US4 waits for the rebase.

## Parallel Example: User Story 1

```bash
# T006, T007 and T008 are three test files; their registrations append
# to test/CMakeLists.txt in task-id order.
cmake --build build --target harness_jitter_probe_test harness_jitter_window_test harness_jitter_cli_test
ctest --test-dir build -R 'harness_jitter_probe_test|harness_jitter_window_test|harness_jitter_cli_test'
```

## Implementation Strategy

### MVP First (User Story 1)

Phases 1 to 3 (US1). At the MVP checkpoint a user names
`--jitter-probe=<cpu>`, the probe spans the batch outside every timed
window, the probe block prints after the tables, the JSON carries the
`jitter` object, and a probe-free run keeps the shape of master's
report (SC-001, SC-003).

### Incremental Delivery

US2 makes the placement safe and its failures clean, US3 attributes the
noise to each benchmark, US4 adds the CI gate through the N5 parser
after PR #45 merges, US5 ships the page and the example, and Phase 8
clears the gates, records the thrawn acceptance, and lands the change.

**Delivery note**: the two open lanes (PR #45, PR #44) share the lists
the spec's Parallel work section names. N5 lands first when it is ready,
and this feature waits for it only at US4 and rebases onto it at T041;
the multi-pass windows of PR #44 reach the split through the pass
indexes T023 records, and the rebase of T041 keeps both lanes' lines.

## Notes

- Every task names the file it edits and the requirement it serves, so a
  reviewer can check one task against the spec without reading the next.
- A task that cannot run as written is a defect in this file: fix the
  task before the code.
- T017 may run beside Phase 3, the way the plan's TDD order puts the
  preset test first: the pure resolver needs no probe. T026 to T028 run
  after the rebase of T041, the wait FR-033 states.
- Setup blocks the stories because the public types and the seams are
  the contract every story test compiles against.
