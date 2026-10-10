# Tasks: Harness Timing Modes

**Input**: Design documents from `specs/017-harness-timing-modes/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/ (state-timing.md, timing-selection.md, report-columns.md, sampling-capacity.md), quickstart.md

**Tests**: Included. FR-024 records TDD mode: each test file is written and observed red before the implementation that turns it green. Every timing assertion runs on the counters fake provider with scripted `machine/monotonic`, `machine/thread_cpu`, and `machine/process_cpu` sequences, so no test needs a PMU, a sleep, or a wall-clock bound.

**Organization**: Tasks are grouped by user story so each story is an independently testable increment. The stories share the hot files (`include/speedgun-ng/benchmark.hpp`, `source/harness/runner.cpp`, `source/harness/registry.cpp`, `source/harness/report.cpp`, `source/harness/detail/internal.hpp`, `source/harness/detail/calibration.hpp`, `test/CMakeLists.txt`), so tasks in different stories that edit one file are chained, not parallel. The `[P]` marker appears only where the write scopes are provably disjoint.

**Execution mode**: one feature branch `017-harness-timing-modes` from `master` at `29f7982`. Each task or chained pair lands as an atomic commit in the constitution format (`<Section>: <one-line imperative>`, a why-body, `Refs: specs/017-harness-timing-modes`). Every commit builds and passes `ctest` on its own.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1 through US6, the user stories of spec.md
- Exact file paths in every description

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: The branch and the version effect of FR-022.

- [x] T001 Create the feature branch `017-harness-timing-modes` from `master` at `29f7982` and commit the spec 017 artifacts (the staged `specs/017-harness-timing-modes/` tree) with a `Docs:` message
- [x] T002 Raise the project version to `0.8.0` (`CMakeLists.txt:7`) and the harness archive `SOVERSION` from 3 to 4 (`CMakeLists.txt:219`); the counters archive keeps `SOVERSION 2` (`CMakeLists.txt:47`); record both in the plan version table, per FR-022
- [x] T003 [P] Read the harness test registration block of `test/CMakeLists.txt` and note the exact shape each new `harness_<area>_test` executable copies (target, link to `speedgun-ng::harness`, `cxx_std_23`, the CTest name equal to the target name, and the `target_include_directories(... ${CMAKE_SOURCE_DIR}/source/harness)` line that `harness_growth_test` and `harness_gap_test` carry)

**Checkpoint**: Branch exists, version table records 0.8.0 and SOVERSION 4, the registration pattern is known.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: The types and record fields every story reads. No behavior changes yet: each task keeps the dev build and the existing suites green.

**⚠️ CRITICAL**: No user story work can begin until this phase is complete.

- [x] T004 Add `enum class TimeUnit : std::uint8_t { NANOSECONDS, MICROSECONDS, MILLISECONDS, SECONDS }` and `inline constexpr std::size_t kMaxPausePerRun = 8` in namespace `sg` in `include/speedgun-ng/benchmark.hpp`, beside `kDefaultRangeMultiplier` at line 42, with doxygen blocks (E-02, FR-014, FR-015)
- [x] T005 Add the four timing fields to `RegistryEntry` in `source/harness/detail/internal.hpp`: `processCpuTime`, `realTime`, `manualTime` (each `bool`, default `false`) and `timeUnit` (`TimeUnit`) (E-01)
- [x] T006 Add the run-record fields to `RunRecord` in `source/harness/detail/calibration.hpp`: `basePoint` (`std::size_t`), `samples` (`std::uint64_t`), `pausePairs` (`std::vector<std::pair<std::uint64_t, std::uint64_t>>`), `cpuNs` (`std::int64_t`), `manual` (`bool`), keeping the H1 names `realNs`, `decisionNs`, `skipped`, `reason` (E-05)
- [x] T007 Add the private `State` timing members in `include/speedgun-ng/benchmark.hpp`: `m_pauseDepth` (`std::uint8_t`), `m_pausePairs` (`std::vector<std::pair<std::uint64_t, std::uint64_t>>`), `m_pauseCount` (`std::uint64_t`), `m_samplesTaken` (`std::uint64_t`), `m_manualNs` (`std::int64_t`) (E-03)

**Checkpoint**: Dev build green, `ctest --preset=dev` green, `cmake --build build/dev -t dbc-gate` green. Foundation ready.

---

## Phase 3: User Story 1 - Read real time and CPU time per iteration (Priority: P1) 🎯 MVP

**Goal**: Every measured row prints `real time/iter (ns)=` and `cpu time/iter (ns)=`, each a counters fold exact to the scripted delta; the CPU column defaults to `machine/thread_cpu`; a run with no pause adds one sample pair and prints `endpoint samples=2`; the aggregate line reads `primary time/iter (ns)` and summarizes the CPU selection.

**Independent Test**: Register one benchmark on the fake provider with scripted `machine/monotonic`, `machine/thread_cpu`, and `machine/process_cpu` sequences, run the suite, and read both time columns. Each column equals its scripted delta divided by the iteration count. (spec US1)

### Tests for User Story 1 ⚠️

> **NOTE: Write these tests FIRST, ensure they FAIL before implementation (FR-024).**

- [x] T008 [US1] Write the failing suite `test/source/harness_time_columns_test.cpp` for C-1: both columns exact to the scripted deltas, the CPU column following `machine/thread_cpu`, `endpoint samples=2` for a no-pause run, an attached metric keeping its H1 fold beside the two columns, and the aggregate line `primary time/iter (ns) mean=` summarizing the CPU selection (quickstart section 2); register it in `test/CMakeLists.txt` in the pattern noted by T003 and observe it red

### Implementation for User Story 1

- [x] T009 [US1] In `source/harness/runner.cpp`: resolve the CPU leaf (`machine/thread_cpu` by default, the resolution-failure line of `:176-187` for a missing leaf), add the file-local netted fold helper `fold(window) − Σ fold(pause interval)` reading `record.basePoint`, `record.samples`, `record.pausePairs` (D-07, D-08, contracts/sampling-capacity.md), replace the `nextPair` counter at `:311` with a point cursor advanced by the samples that run took, fill `RunRecord.realNs`, `cpuNs`, `decisionNs` (the CPU selection as the default primary, FR-002), and fill the `ResultRow` fields `cpuTimePerIterationNs`, `endpointSamples`, `timeAvailability` (D-01, D-13)
- [x] T010 [US1] In `include/speedgun-ng/benchmark.hpp`: add the `ResultRow` fields `cpuTimePerIterationNs` (`double`), `endpointSamples` (`std::uint64_t`), `timeAvailability` (`sg::counters::Availability`) and keep `timePerIterationNs` as the real column (E-06, FR-018)
- [x] T011 [US1] In `source/harness/report.cpp`: print `real time/iter (<U>)=`, `cpu time/iter (<U>)=`, `overhead floor (<U>)=`, and `endpoint samples=<S>` in the repetition row, and `primary time/iter (<U>) mean=...` in the aggregate row, with the aggregate collecting the primary time and skipping a row whose `timeAvailability` is not `COUNTABLE` (D-13, D-14, contracts/report-columns.md); in the same commit move the field token in `test/source/harness_calibration_test.cpp:296,375,404`, `test/source/harness_callback_test.cpp:464`, `test/source/harness_capture_test.cpp:330,372`, `test/source/harness_fixture_test.cpp:488`, and `test/source/harness_statistics_test.cpp:199` to the exact column token each reader means, so every commit of the chain keeps the suite green
- [x] T012 [US1] Make `ctest --test-dir build/dev -R harness_time_columns_test` pass, then the full dev `ctest` green including `loop_shape`, `cli_shape`, `barrier_shape`, and `time_source_gate`; commit as one atomic change

**Checkpoint**: US1 works on its own: two exact columns, the real sample count, the CPU-driven aggregate. This is the MVP.

---

## Phase 4: User Story 2 - Switch the CPU leaf and the primary time (Priority: P2)

**Goal**: `measureProcessCpuTime` moves the CPU column to `machine/process_cpu`; `useRealTime` makes real time the primary time for the qualification test, the growth rule, and the aggregate; the two combine legally (FR-004, FR-005, FR-008).

**Independent Test**: Register one benchmark with the process CPU call and one with the real-time call, each on scripted leaves, and run. Check the CPU column source and the clock the growth rule reads. (spec US2)

### Tests for User Story 2 ⚠️

- [x] T013 [US2] Write the failing suite `test/source/harness_real_time_test.cpp` for C-3: the scripted growth sequence follows the monotonic fold, differing from the CPU-driven walk of `test/source/harness_calibration_test.cpp:276-304`, and the aggregate summarizes the real column; register it in `test/CMakeLists.txt` and observe it red (quickstart section 4)
- [ ] T014 [US2] Write the failing suite `test/source/harness_cpu_leaf_test.cpp` for C-2: the switch moves the CPU column to the scripted `machine/process_cpu` fold, the switch combines with `useRealTime`, and a provider that publishes no `machine/process_cpu` leaves that benchmark failed with the resolution line; register it in `test/CMakeLists.txt` and observe it red (quickstart section 3)

### Implementation for User Story 2

- [ ] T015 [US2] In `include/speedgun-ng/benchmark.hpp` and `source/harness/registry.cpp`: declare and define `BenchmarkHandle::measureProcessCpuTime()` and `BenchmarkHandle::useRealTime()`, each returning `BenchmarkHandle&`, each behind the `m_entry != nullptr && !m_entry->runStarted` `SG_REQUIRE` the existing setters use, `useRealTime` carrying `SG_REQUIRE(!m_entry->manualTime, ...)` for the FR-008 exclusion, with doxygen `\pre` and `\post` blocks whose kinds pair inside the interface scope (D-03, D-04, contracts/timing-selection.md)
- [ ] T016 [US2] In `source/harness/runner.cpp`: build the plan with the chosen CPU core once per benchmark (D-02) and select `decisionNs` as the real fold when `realTime` stands (FR-005); make T013 and T014 pass and the full dev `ctest` green; commit as one atomic change

**Checkpoint**: US1 and US2 both work independently.

---

## Phase 5: User Story 3 - Report a manually measured time (Priority: P2)

**Goal**: `useManualTime` selects manual time; `State::setIterationTime(std::int64_t)` sums the accepted nanosecond counts; the real column shows the sum over N, the growth rule reads the sum, and the fivefold real-time stop stays exempt (FR-006, FR-007, FR-008).

**Independent Test**: Register a manual-time benchmark whose function calls the accept method once per iteration with fixed values, run on the fake provider, and check the reported column and the calibration clock against the scripted sum. (spec US3)

### Tests for User Story 3 ⚠️

- [ ] T017 [US3] Write the failing suite `test/source/harness_manual_time_test.cpp` for C-4: the accepted sum over N in the real column, the growth rule reading the accepted sum, the fivefold stop exempt, a benchmark that accepts nothing reporting `0.000` and qualifying at the iteration cap, and `useRealTime` with `useManualTime` ending the re-exec of the test binary in either order with the parent reading the signal (the pattern of `test/source/harness_callback_test.cpp:593-621`); register it in `test/CMakeLists.txt` and observe it red (quickstart section 5)

### Implementation for User Story 3

- [ ] T018 [US3] In `include/speedgun-ng/benchmark.hpp` and `source/harness/registry.cpp`: declare and define `BenchmarkHandle::useManualTime()` (chaining, the setter `SG_REQUIRE`, `SG_REQUIRE(!m_entry->realTime, ...)`) and `State::setIterationTime(std::int64_t nanoseconds)` adding to `m_manualNs`, legal only in a run state (the R-05 rule of H2 extended, D-15), with the contract blocks of contracts/state-timing.md
- [ ] T019 [US3] In `source/harness/runner.cpp` and `source/harness/detail/calibration.hpp`: under `manualTime` set `RunRecord.realNs` and `decisionNs` from `m_manualNs` and set `manual = true`; exempt a manual run from the fivefold stop in `qualifies` (`calibration.hpp:67-71`, D-05); make T017 pass and the full dev `ctest` green; commit as one atomic change

**Checkpoint**: US1 through US3 work independently.

---

## Phase 6: User Story 4 - Pause and resume the clock (Priority: P2)

**Goal**: `pauseTiming` and `resumeTiming` each take one sample of every leaf; every time column and every attached metric fold subtracts each paused interval; `ScopedPauseTiming` pauses for its lifetime; the breaches of FR-011 and the FR-014 allowance end the process through the contract facility; the row prints the run's real sample count (FR-009 to FR-014).

**Independent Test**: Register a benchmark that pauses and resumes once inside the loop, with scripted leaves, and run. The reported time equals the window fold minus the scripted pause fold, exact to the nanosecond, and the attached metric fold shows the same subtraction. (spec US4)

### Tests for User Story 4 ⚠️

- [ ] T020 [US4] Write the failing suite `test/source/harness_pause_test.cpp` for C-5: each column equals the window fold minus the pause fold to the scripted nanosecond, an attached metric carries the same subtraction, `endpoint samples=4` prints, a `ScopedPauseTiming` region reports the same value as the explicit pair, and a second `pauseTiming` before the resume, a `resumeTiming` with no open pause, an open pause at loop exit, a pause in a setup callback, and a ninth interval in one run each end the re-exec through the contract facility; register it in `test/CMakeLists.txt` with the `source/harness` include directory and observe it red (quickstart section 6)

### Implementation for User Story 4

- [ ] T021 [US4] In `include/speedgun-ng/benchmark.hpp`: define `State::pauseTiming()` (`SG_REQUIRE` run state, open window, `m_pauseDepth == 0`, `m_pauseCount < kMaxPausePerRun` before the sample), `State::resumeTiming()` (`SG_REQUIRE` `m_pauseDepth == 1`, records the ordinal pair), the `SG_ASSERT(m_pauseDepth == 0, ...)` in `closeWindow()` (D-10), `topUpWindow()` closing an open pause pair before the exit sample (D-11), the `State` friend access for `ScopedPauseTiming`, and the `ScopedPauseTiming` class pausing in the constructor, resuming in the destructor, and skipping the resume when the run was skipped (FR-012, contracts/state-timing.md)
- [ ] T022 [US4] In `source/harness/runner.cpp`: raise the recorder capacity to `2 * (1 + kMaxPausePerRun) * (kRunBound + kRunBound + repetitions)` (FR-014, contracts/sampling-capacity.md), take `basePoint`, `samples`, and `pausePairs` from the `State` after each run, and route the real core, the CPU core, and every metric core through the netted fold helper (FR-010); make T020 pass and the full dev `ctest` green including `loop_shape`; commit as one atomic change

**Checkpoint**: US1 through US4 work independently.

---

## Phase 7: User Story 5 - Scale the printed time to a unit (Priority: P2)

**Goal**: `unit(TimeUnit)` sets the printed unit of one benchmark; `setDefaultTimeUnit`/`getDefaultTimeUnit` set the process default, captured at registration so a later call leaves an earlier benchmark alone; every printed time value carries the scaled value and its label (FR-015, FR-016).

**Independent Test**: Register benchmarks with each unit call and one with none, set the default, and compare the printed value and label against the scaled scripted delta. (spec US5)

### Tests for User Story 5 ⚠️

- [ ] T023 [US5] Write the failing suite `test/source/harness_time_unit_test.cpp` for C-6: each unit prints the scaled value with its label in both columns and in the overhead floor, `setDefaultTimeUnit` scales a benchmark that sets none, and a default set after a registration leaves that benchmark at its captured unit; register it in `test/CMakeLists.txt` and observe it red (quickstart section 7)

### Implementation for User Story 5

- [ ] T024 [US5] In `include/speedgun-ng/benchmark.hpp` and `source/harness/registry.cpp`: declare and define `BenchmarkHandle::unit(TimeUnit)` (chaining, the setter `SG_REQUIRE`), the free `setDefaultTimeUnit(TimeUnit)` and `getDefaultTimeUnit()` with the process default starting at `TimeUnit::NANOSECONDS`, and write the captured default into `RegistryEntry::timeUnit` at entry creation (D-12, contracts/timing-selection.md)
- [ ] T025 [US5] In `include/speedgun-ng/benchmark.hpp`, `source/harness/runner.cpp`, and `source/harness/report.cpp`: add `BenchmarkResult::timeUnit`, copy it from the entry in the runner, and scale the two time columns, the overhead floor, and the aggregate line by the unit divisor with the `ns`, `us`, `ms`, `s` labels (E-02, FR-015); make T023 pass and the full dev `ctest` green; commit as one atomic change

**Checkpoint**: US1 through US5 work independently.

---

## Phase 8: User Story 6 - Examples and documentation (Priority: P3)

**Goal**: The example suite shows one benchmark each for C-2 through C-6; `docs/pages/harness.md` gains one section per capability C-1 through C-7 (FR-017).

**Independent Test**: Build and run the example suite; check that one benchmark shows each capability. Read the documentation page; check one section per capability. (spec US6)

### Implementation for User Story 6

- [ ] T026 [P] [US6] Add to `example/benchmark_example.cpp` one benchmark each for the process CPU switch, the real-time switch, manual time, pause and resume, and the millisecond unit (FR-017, quickstart section 8)
- [ ] T027 [P] [US6] Add to `docs/pages/harness.md` one section for each capability C-1 through C-7 beside the ten sections of feature 016, in the page's existing voice and the Principle XI prose rules (FR-017)
- [ ] T028 [US6] Build and run `build/example/benchmark_example --dry-run` and check the five new rows (the paused row prints `endpoint samples=4`, the millisecond row carries the `ms` label) and the exit status `0`; run the prose gate and the spell gate over the touched files; commit as one atomic change

**Checkpoint**: All user stories complete.

---

## Phase 9: Polish & Cross-Cutting Concerns

**Purpose**: The hard gates of Principle VIII at the feature head (SC-006, FR-023).

- [ ] T029 Run the gate matrix at the head: `ctest --test-dir build/dev` full, `cmake --build build/dev -t dbc-gate`, `cmake -P cmake/prose-lint.cmake`, `cmake -P cmake/spell.cmake`, and the name check; fix every finding
- [ ] T030 Build `ci-sanitize` into `build/sanitize` and run its `ctest`; build `ci-tsan` into `build/tsan` and run its `ctest`; fix every sanitizer report
- [ ] T031 Build `ci-coverage` into `build/coverage`, run `cmake --build build/coverage -t coverage`, and confirm 100% line and branch coverage with zero `LCOV_EXCL` markers under `source/harness/` (FR-023)
- [ ] T032 Build the release preset once per the contribution rule: `cmake --preset=ci-ubuntu` then `cmake --build build`, and run `ctest --test-dir build -R harness` plus the `loop_shape` and `time_source_gate` checks in that tree (quickstart sections 1 and 9)
- [ ] T033 Run quickstart.md sections 1 through 11 end to end against the head, record each result in the Execution Log below, and confirm the version and `SOVERSION` state of FR-022 (SC-007)

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: start immediately.
- **Foundational (Phase 2)**: after Setup; BLOCKS all stories. T004 before T005, T006, T007 (the fields name `TimeUnit`).
- **User Stories**: all depend on Foundational. They share the hot files, so run them in priority order US1 → US2 → US3 → US4 → US5; US6 after US5 (the examples call the whole surface).
- **Polish (Phase 9)**: after all desired stories.

### Within Each User Story

- The test task is written first and observed red (FR-024), then the implementation tasks, then the green run and the commit.
- Tasks editing one file chain: `benchmark.hpp` edits order T004 → T007 → T010 → T015 → T018 → T021 → T024; `runner.cpp` edits order T009 → T016 → T019 → T022 → T025; `report.cpp` edits order T011 → T025.

### Parallel Opportunities

- T003 runs beside T002.
- T026 and T027 touch disjoint files.
- Test suites of one story may be drafted in parallel only when they touch disjoint test files and the shared `test/CMakeLists.txt` edit is taken by one of them.
- Stories themselves do not run in parallel: the shared hot files make the write scopes overlap, so the chain above is the plan.

## Parallel Example: User Story 6

```text
Task: "Add the five capability benchmarks to example/benchmark_example.cpp" (T026)
Task: "Add the seven capability sections to docs/pages/harness.md" (T027)
```

## Implementation Strategy

### MVP First (User Story 1 only)

1. Phase 1 Setup, Phase 2 Foundational.
2. Phase 3 US1: red test, runner and report change, green suite. STOP and validate: `ctest --test-dir build/dev -R harness_time_columns_test`.
3. The two-column row is the base every later capability switches, pauses, or scales.

### Incremental Delivery

1. US1 lands the two columns and the real sample count.
2. US2 lands the two selection calls and the primary-time rule.
3. US3 lands the manual accumulator and the fivefold exemption.
4. US4 lands pause, resume, the guard, and the capacity rule.
5. US5 lands the unit capture and the printed scaling.
6. US6 lands the examples and the documentation; Phase 9 lands the gate evidence.

## Notes

- The old row token `time/iter (ns)=` is a suffix of both new column tokens, so T011 moves each reader to the exact token in the same commit that changes `report.cpp`; the list-mode checks of `harness_cli_test.cpp`, `harness_family_test.cpp`, and `harness_instance_name_test.cpp` stay green untouched.
- The pair gate pairs a documented `\pre`/`\post` with its enforcement inside the interface scope; a method block carries no `\invariant` (016 convergence rule).
- Precondition-violation scenarios re-exec the test binary and the parent reads the signal; one scripted provider per process (the convention of `harness_calibration_test.cpp:487`).
- Codespell takes American spellings; the prose gate binds this file and the code prose alike (FR-025).

## Deviations

- None at authoring time. Record each deviation from plan.md here as it happens, with its reason.

## Execution Log

- (append one entry per completed task: the task id, the command run, the captured result, the commit)
- T001: `git switch -c 017-harness-timing-modes 29f7982` -> branch `017-harness-timing-modes` exists, ten spec 017 artifacts land with 1734 insertions (3392331)
- T002: `grep -n 'VERSION 0.8.0' CMakeLists.txt`, `grep -n SOVERSION CMakeLists.txt` -> line 7 `VERSION 0.8.0`, the counters archive at `SOVERSION 2` on line 47, the harness archive at `SOVERSION 4` on line 219 (405f833)
- T003: `grep -n harness_time_columns_test test/CMakeLists.txt` -> the four-line block to copy: `add_executable`, the link to `speedgun-ng::harness`, `cxx_std_23`, and `add_test` named after the target. Read-only task, no commit.
- T004: `grep -n 'enum class TimeUnit' include/speedgun-ng/benchmark.hpp` -> the enumeration on line 53 and `kMaxPausePerRun = 8` on line 69 (d16b8d5)
- T005: `grep -n processCpuTime source/harness/detail/internal.hpp` -> the four timing fields on lines 68 to 71 (718c40b)
- T006: `grep -n basePoint source/harness/detail/calibration.hpp` -> the five run-record fields on lines 36 to 40 (2d1c392)
- T007: `grep -n m_pauseDepth include/speedgun-ng/benchmark.hpp` -> the five state members on lines 533 to 539 (d72830b)
- T008: `git show --stat d8c1f40` -> `test/source/harness_time_columns_test.cpp` 311 lines plus the `test/CMakeLists.txt` block; the commit body records the red as a compile failure on the two missing `ResultRow` fields (d8c1f40)
- T009: `git show --stat d81a2ad` -> `source/harness/runner.cpp` 79 insertions and 29 deletions: the CPU leaf choice, the file-local netted fold, the point cursor, and the row time fields (d81a2ad)
- T010: `grep -n cpuTimePerIterationNs include/speedgun-ng/benchmark.hpp` -> the three `ResultRow` fields on lines 147, 150 and 154 (eb45a1f)
- T011: `grep -n 'real time/iter' source/harness/report.cpp` -> the aggregate token on line 63 and the measured row tokens on lines 94 to 96; the five reader files move to the exact token in that same commit (a17ef7c)
- T012: `ctest --test-dir build/dev -R harness_time_columns_test --output-on-failure` -> `100% tests passed out of 1`, exit 0; `ctest --test-dir build/dev` -> `100% tests passed out of 69`, exit 0 (a17ef7c)
- T013: `git show --stat 327de40` -> `test/source/harness_real_time_test.cpp` 342 lines land; the suite is absent from `test/CMakeLists.txt`, so it stays outside the CTest set until T016 registers it (327de40)
- Checkpoint at `a17ef7c` on 2026-10-10: `cmake --build --preset=dev` exit 0; `ctest --test-dir build/dev -R harness_time_columns_test --output-on-failure` exit 0; `ctest --test-dir build/dev` 69 of 69 passed in 130.87 s, exit 0; `cmake --build build/dev -t dbc-gate` -> `doc-gate: 187 interfaces, 0 gaps` and `pair-gate: 187 interfaces, 0 gaps`, exit 0; `python3 tools/prose/prose_gate.py --check prose --mode tree --paths` over the 26 files of `29f7982..HEAD` -> `26 sources, 3991 units examined, 0 findings`, exit 0; `cmake -P cmake/spell.cmake` exit 0
