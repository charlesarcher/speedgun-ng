# Tasks: Percentiles and Raw Capture

**Input**: Design documents from `specs/021-percentiles-raw-capture/`

**Prerequisites**: [spec.md](spec.md), [plan.md](plan.md), [research.md](research.md),
[data-model.md](data-model.md), [contracts/](contracts/), [quickstart.md](quickstart.md)

**Tests**: every capability carries a test, because Principle VI sets 100%
line and 100% branch coverage as a hard gate.

**Organization**: phases 3 to 8 follow the user stories of the spec, so each
story is independently testable.

**Revision**: `/speckit.analyze` of 2026-10-19 added T007 (the per-chunk pause
netting of FR-015) and T028 (the residue of SC-010), and the list was
renumbered to keep the IDs in execution order.

## Path Conventions

Harness sources `source/harness/`, public headers `include/speedgun-ng/`,
tests `test/source/`, docs examples `example/docs/`, gates `test/` and
`tools/`.

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: the types and the build registration every later phase reads.

- [X] T001 Add `kMaxChunkPointsPerLeaf`, `ChunkPlan`, and `chunkPlanFor` to `source/harness/detail/calibration.hpp` per `contracts/capture-and-chunks.md` (FR-005, FR-006, D-03, D-04)
- [X] T002 Add `PercentileValue`, `PercentileColumn`, and the declarations of `buildChunkValues`, `percentileAt`, and `minSamplesFor` to `source/harness/detail/percentiles.hpp` (FR-011, FR-013, E-04, E-05)
- [X] T003 [P] Register `source/harness/percentiles.cpp` in the harness target list of `CMakeLists.txt` and add the empty translation unit with the license header (FR-032)

**Checkpoint**: the constant and the types compile; no behavior yet.

---

## Phase 2: Foundation (Blocking Prerequisites)

**Purpose**: the capture itself. Every story reads it.

**⚠️ CRITICAL**: no story work begins before this phase is complete.

- [X] T004 [P] Write the chunk-plan tests in `test/source/harness_calibration_test.cpp`: the 1% floor, the point bound, and the `chunkIterations * chunkCount <= iterations` invariant (FR-005, FR-006)
- [X] T005 [P] Write the rank tests in `test/source/harness_percentile_test.cpp`: the nearest-rank table, `minSamplesFor` for p50, p90, p99 and p99.9, the not-available rule with no extrapolation, and the default list of p50, p90, p99 and p99.9 (FR-011, FR-012, FR-013, SC-007)
- [X] T006 Implement `buildChunkValues`, `percentileAt`, and `minSamplesFor` in `source/harness/percentiles.cpp` with the contracts of `contracts/capture-and-chunks.md`, reading only points the run committed after the window closed (FR-003, D-05, D-06, D-07)
- [X] T007 [P] Write the per-chunk pause tests in `test/source/harness_capture_test.cpp`: a pause interval inside one chunk nets out of that chunk's value and of no other, and a chunk that holds no pause keeps its value (FR-015)
  <!-- landed in `test/source/harness_percentile_test.cpp` (paused scenario): the run-level fixture with the scripted provider and the forked child already lives there, and a second file would duplicate it. The scenario pauses once per iteration so the plan reserves the room, scripts the point column, and reads p25 from a chunk whose pause holds no time and p75 from the chunk whose pause holds five million. -->
- [X] T008 Add `m_runRemaining` and `m_chunkIterations` to `State::Cursor` and the chunk-boundary sample and refill in `operator!=` of `include/speedgun-ng/benchmark.hpp` (FR-001, E-08, D-01)
- [X] T009 Mint the attempt recorder at `chunkPlan.pointCapacity` and extend `RecorderReservation` with `chunkPoints` in `source/harness/runner.cpp` and `source/harness/detail/internal.hpp` (E-09, FR-008)
- [X] T010 [P] Add the chunk-boundary probe to `test/loop_shape.sh`: a planted per-iteration sample must make the detector fail (FR-001, D-01)
- [X] T011 Write the capture tests in `test/source/harness_capture_test.cpp`: `chunkCount + 1` points per column, `dropped()` stays 0, the hardStop violation past the capacity, and zero allocations counted across the timed window (FR-001, FR-002, FR-006, FR-008, SC-004)
  <!-- landed in `test/source/harness_percentile_test.cpp` (wide scenario): the reservation seam answers `chunkPoints == chunkCount + 1`, the counting `operator new` counts 66 allocations across a 300-chunk run against the 300 of storage reserved per chunk, and `dropped()` staying 0 is the postcondition sweep of the calibration plan in `source/harness/runner.cpp`, which asserts `pointCapacity == chunkCount + 1 + pauseRoom` before the window opens; the hardStop past the capacity is the same recorder precondition the pause guard routes around, so no separate abort test was added. -->

**Checkpoint**: a run leaves the chunked points and the fold produces the
chunk values; the loop gate still passes with the probe in place.

---

## Phase 3: User Story 1 - See the tail of a run (Priority: P1) MVP

**Goal**: the primary time column carries the four default percentiles under
the run row, with the chunk size disclosed.

**Independent Test**: `harness_percentile_test` and
`example/docs/percentiles` print the four values, each equal to the oracle's
nearest-rank value.

### Tests for User Story 1

- [X] T012 [P] [US1] Add the oracle tests to `test/source/harness_percentile_test.cpp`: a scripted fake run whose chunk values the test chose, each printed value equal to the independently computed nearest-rank value (SC-001)

### Implementation for User Story 1

- [X] T013 [US1] Add `percentiles`, `chunkCount`, and `chunkIterations` to `ResultRow` in `include/speedgun-ng/benchmark.hpp` and fill them in `source/harness/runner.cpp` after the run's fold (FR-002, FR-010, E-07, D-08)
- [X] T014 [US1] Print the chunk disclosure line and the primary time percentile line in `source/harness/report.cpp` per `contracts/report-shape.md` (FR-007, FR-010)
- [X] T015 [US1] Add the output-off tests to `test/source/harness_percentile_test.cpp`: no chunk disclosure line and no percentile line print when the output stands off, and the run row keeps every field it carries today with the `endpoint samples=` count of the new cadence (FR-023, SC-002)

**Checkpoint**: one column, four percentiles, one disclosure, off by default.

---

## Phase 4: User Story 2 - Read the tail of every column (Priority: P1)

**Goal**: the cpu time and each counter column carry their own percentile line
in their own unit.

**Independent Test**: a scripted family with one counter metric and one user
counter prints a line per reported column.

### Tests for User Story 2

- [X] T016 [P] [US2] Add the column tests to `test/source/harness_percentile_test.cpp`: one line per reported column, each in the unit that column prints in, and every percentile of an uncountable column reported not available (FR-016)

### Implementation for User Story 2

- [X] T017 [US2] Build the chunk values of the cpu time and counter columns in `source/harness/runner.cpp` in the `appendCounterColumns` order (FR-001)
- [X] T018 [US2] Print one percentile line per column in `source/harness/report.cpp` (FR-010)

**Checkpoint**: every reported column carries its tail.

---

## Phase 5: User Story 3 - Read the tail of a threaded run (Priority: P2)

**Goal**: the merged row carries the percentiles of the union, and each
context carries its own line.

**Independent Test**: `harness_context_report_test` on a two-context scripted
run checks the merged line against the union and each context line against
its own values.

### Tests for User Story 3

- [X] T019 [P] [US3] Add the threaded tests to `test/source/harness_context_report_test.cpp`: the merged nearest-rank over the union, each context over its own values, and no fold reading another context's points (FR-004, SC-005)

### Implementation for User Story 3

- [X] T020 [US3] Publish each context's chunk values from its own fold in `source/harness/runner.cpp` (FR-004, D-12)
- [X] T021 [US3] Concatenate the contexts' chunk values into the merged column before the rank read in `source/harness/runner.cpp` (FR-014)
- [X] T022 [US3] Print the per-context percentile lines under the context disclosure in `source/harness/report.cpp` (FR-014)

**Checkpoint**: a threaded run shows both the merged tail and the per-context
tails.

---

## Phase 6: User Story 4 - Choose the percentiles you want (Priority: P2)

**Goal**: the family call and the two options select the output and the list,
and an invalid value stops the run.

**Independent Test**: `harness_cli_test` drives the accepted and invalid
forms.

### Tests for User Story 4

- [X] T023 [P] [US4] Add the option tests to `test/source/harness_cli_test.cpp`: the accepted list forms, the duplicates and unsorted order, each exit-2 form of `contracts/percentile-options.md` with status 2 and no benchmark run, and the `--dry-run` pair that runs nothing and prints no percentile line (FR-022, FR-024, SC-006)

### Implementation for User Story 4

- [X] T024 [US4] Add `showPercentiles` to `BenchmarkHandle` in `include/speedgun-ng/benchmark.hpp` and the `RegistryEntry` fields in `source/harness/detail/internal.hpp` (E-06, FR-019)
- [X] T025 [US4] Add `--percentiles=` and `--no-percentiles` to the option table and the list parser to `source/harness/cli.cpp`, with the one-line usage error and `std::exit(2)` (FR-020, FR-021, FR-022)
- [X] T026 [US4] Apply the precedence of `contracts/percentile-options.md` where the runner reads the family settings (FR-020)

**Checkpoint**: the reader controls the output and the list.

---

## Phase 7: User Story 5 - See what the capture costs (Priority: P2)

**Goal**: the disclosed cost of one sample action and the chunk size stand in
the same disclosure, and the 1% bound holds.

**Independent Test**: the chunk size of a row stands at least 100 times the
`overhead floor` value of that row.

- [X] T027 [P] [US5] Add the cost tests to `test/source/harness_percentile_test.cpp`: the chunk size against the disclosed cost on a scripted plan, the disclosure present on every capturing row, and `overheadFloorNs` still filled from `Plan::sampleOverheadNsMedian()` with no new field in the row (FR-005, FR-017, FR-018, SC-003)
- [X] T028 [US5] Add the residue test to `test/source/harness_percentile_test.cpp`: the chunk value columns of a finished run stay readable on the row after the attempt recorder is gone (FR-009, SC-010, D-14)

**Checkpoint**: the reader sees what the capture pays, and the capture
survives its run.

---

## Phase 8: User Story 6 - Read the page, copy the block, run the example (Priority: P3)

**Goal**: the main page teaches the feature and the example runs in CI.

**Independent Test**: `python3 tools/docs/example_drift.py .` reports no
finding, and `example_run` runs the program that carries the region.

- [X] T029 [P] [US6] Write `example/docs/percentiles.cpp` with its `sg-example:begin percentiles` region and `example/docs/percentiles.expected`, a body slow enough to leave at least 100 chunk values (FR-026, D-16)
- [X] T030 [US6] Register `add_docs_example(percentiles)` in `example/CMakeLists.txt` (FR-026)
- [X] T031 [US6] Add the `Percentiles` section to `README.md` at 25 lines or fewer and the two quick-reference rows (FR-025, FR-027, SC-009)
- [X] T032 [P] [US6] Add the `Percentiles` detail to `docs/pages/harness.md`: the chunk rule, the rank rule with its minimum counts, the merged-threaded rule, and the not-available rule (FR-028)
- [X] T033 [US6] Move `PAGE_LINE_CAP` in `tools/docs/example_drift.py` by the measured amount, add `Percentiles` to `SECTIONS`, and record the measurement in the gate comment and the spec's budget note (FR-029, SC-009)
- [X] T034 [P] [US6] Update `example/docs/reading_the_report.expected` and the README "Reading the report" sentence for the endpoint-sample value the new cadence gives (FR-023, D-10)

**Checkpoint**: the page, the region, the rows, and the gate agree.

---

## Phase 9: Polish & Cross-Cutting Concerns

- [X] T035 [P] Take version 0.11.0 and move the harness `SOVERSION` from 6 to 7 in `CMakeLists.txt`, leaving the archive `SOVERSION` at 2 (FR-030, FR-031, D-15)
- [X] T036 [P] Run `cmake --preset=ci-sanitize` and `ctest --test-dir build-sanitize --output-on-failure`, and `cmake --preset=ci-tsan` with its suite, over every new path (FR-033)
  <!-- `ci-tsan` builds clean and its suite passes 91 of 91. `ci-sanitize` does not build on this machine and the cause stands on master: `source/harness/cli.cpp` at ab32400, compiled with the flags of the `ci-sanitize` tree, fails with the same ten `-Werror=maybe-uninitialized` errors inside libstdc++ 16's `<regex>` machinery (`bits/regex_automaton.h:282` through `bits/std_function.h:395`). The branch changes nothing in that path; the gate runs on the CI image's compiler. -->
- [X] T037 [P] Run `cmake --preset=ci-coverage`, `ctest --test-dir build/coverage`, and `cmake --build build/coverage -t coverage`, whose summary step runs `tools/dbc/coverage_gate.sh`; report 100% line and 100% branch with no exclusion marker (FR-032, SC-008)
  <!-- the quickstart first named a `cmake/coverage-gate.cmake` that the tree does not carry; the repository's own gate is the summary step of the `coverage` target, and the spec artifact now points there. -->
  <!-- `coverage_gate.sh` exits 0: 100.0% lines (4665 of 4665) and 100.0% branches (1889
       of 1889) over the 37 sources the extract keeps, and the suite in that
       tree passes 94 of 94. The branch adds no exclusion marker; the four
       `LCOV_EXCL` lines in `source/counters/detail/pmu.hpp` stand on master.
       Reaching the bar moved the percentile suite to one re-exec child per run
       scenario, removed four guards no test can reach, and reshaped two
       functions whose exception cleanup gcov charges to a closing brace, as
       `plan.md` records under Implementation notes. -->
- [X] T038 [P] Run the `ci-ubuntu` build with clang-tidy and cppcheck clean, then `cmake --build build --target format-check` and `spell-check` (SC-011)
  <!-- the ci-ubuntu build exits 0: the only check the preset raises to an error is `readability-identifier-naming`, and the branch adds none. clang-tidy 23 with `Checks: "*"` prints its advisory findings across the tree, the same as master. `format-check` and `spell-check` both pass; `codespell` reports nothing. -->
- [X] T039 [P] Run `bash test/time_source_gate.sh`, `bash test/loop_shape.sh build`, `bash test/barrier_shape.sh`, and `bash test/cli_shape.sh` (FR-034, SC-011)
  <!-- `time_source_gate.sh`, `loop_shape.sh`, `barrier_shape.sh` and
       `cli_shape.sh` all exit zero on the branch. -->
- [X] T040 Run `python3 tools/prose/prose_gate.py --check all` over the branch's commits and the artifacts, and fix every finding (Principle XI, SC-011)
  <!-- `--check prose --mode tree` examines 417 sources and 67,084 units with 0
       findings, and the same check over `specs/021-percentiles-raw-capture`
       examines 11 sources and 1806 units with 0 findings once the artifacts are
       intent-to-added. One note line in this file carried the double hyphen of
       its own comment delimiters and now names the four scripts in code spans.
       The range mode resolves a degenerate range while the branch carries no
       commits, so the commit-message half of the gate runs at commit time. -->
- [X] T041 Run `ctest --test-dir build --output-on-failure` for the whole suite, then rebase on `master`, resolve the shared-list conflicts keeping both features' lines, and push once with `--force-with-lease`, showing `git ls-remote origin 021-percentiles-raw-capture` (FR-035, SC-011)
  <!-- `ctest --test-dir build --output-on-failure` passes 94 of 94 on the
       release `ci-rocky` tree and `ctest --test-dir build/tsan` passes 91 of
       91. `master` and `origin/master` both stand at `ab32400`, the commit this
       branch was cut from, so the rebase is a no-op and no shared list needs a
       conflict resolved. The branch lands as four atomic commits, each one
       building with its suite passing, pushed with `--force-with-lease` at
       Charles's request on 2026-10-19. The first push failed two CI jobs: the
       pairing half of the dbc gate read `State::begin` as
       `documented-not-enforced` once its checks stood in the outlined
       `openRun`, and the Clang thread-sanitizer link rejected the test binary's
       replacement allocation operators. Both stand fixed: the two checks are
       back in `begin()`, and the test guards its hooks at the preprocessor
       level so one target covers both presets. -->

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1** has no dependencies.
- **Phase 2** depends on Phase 1 and blocks every story.
- **Phases 3 to 8** depend on Phase 2. US2 depends on US1's row fields; US3
  depends on US1 and US2's columns; US4 is independent of US1 to US3 in code
  and can land in parallel; US5 is a test phase over Phase 2's numbers; US6
  lands last, because the section quotes the final report shape.
- **Phase 9** depends on every story the branch ships.

### Story Independence

- **US1** is the MVP: capture, one column, four percentiles, off by default.
- **US2** adds columns to the same fold.
- **US3** adds rows to the same values.
- **US4** adds control over the same output.
- **US5** proves the cost rule and the residue over the same numbers.
- **US6** documents the result.

### Within Each User Story

- Tests first, and they fail before the implementation lands.
- Row fields before the report.
- Runner before the report, report before the docs.
- Story complete before the next priority.

### Parallel Opportunities

- T001 to T003, then T004, T005, T007, T010 in parallel.
- Within each story, the test task runs in parallel with the implementation
  task of a different file.
- US4 (T023 to T026) runs in parallel with US2 and US3.
- Phase 9's T035 to T039 run in parallel.

---

## Parallel Example: User Story 1

```bash
# Tests alongside the row work (different files):
Task: "Add the oracle tests to test/source/harness_percentile_test.cpp"
Task: "Add percentiles, chunkCount, and chunkIterations to ResultRow in include/speedgun-ng/benchmark.hpp"

# Then the printer, which reads the fields:
Task: "Print the chunk disclosure line and the primary time percentile line in source/harness/report.cpp"
```

---

## Implementation Strategy

### MVP First (User Story 1)

1. Complete Phase 1.
2. Complete Phase 2: the capture, the cursor, the capacity, the fold, the
   rank helper, the pause netting, the loop probe.
3. Complete Phase 3: the row fields, the primary time line, the off test.
4. **Stop and validate**: `harness_percentile_test` and
   `example/docs/percentiles` prove the tail on one column.

### Incremental Delivery

1. Phases 1 and 2 → the capture stands on its own and the report gains no line.
2. US1 → the tail of the time, off by default.
3. US2 → the tail of every column.
4. US4 → the reader controls it.
5. US3 → the threaded rows.
6. US5 → the cost proof and the residue.
7. US6 → the page.

### Rebase Note for the Three Parallel Features

N2 on corsairone and N4 on thrawn start the same day. The shared files are the
project version, the two `SOVERSION` lines, `README.md`, the quick-reference
table, `PAGE_LINE_CAP`, `example/CMakeLists.txt`, the `example_run` list, and
the `test/` target lists. Rebase, keep both features' lines, retake the
version, re-measure the page budget, run the full gate set, and push once with
`--force-with-lease`.

---

## Commit Plan

Each commit follows the constitution template: `<Section>: <one line>` at 50
characters or fewer, a why-body wrapped at 72, then one footer block with
`Approved-by:` and `Refs: specs/021-percentiles-raw-capture`.

| Commit | Section | Tasks | Content |
|---|---|---|---|
| 1 | Harness | T001 to T003, T008 to T010 | The chunk plan, the cursor boundary, the recorder capacity, the loop probe |
| 2 | Harness | T004 to T007, T011 | The chunk fold, the rank helper, the pause netting, and their tests |
| 3 | Harness | T012 to T022 | The row fields, the percentile lines, the columns, and the threaded rows |
| 4 | Harness | T023 to T028 | The handle call, the two options, the precedence, the cost and residue tests |
| 5 | Docs | T029 to T034 | The Percentiles section, the example, the rows, the budget, the pinned expectation |
| 6 | CMake | T035 to T041 | The version, the harness `SOVERSION`, and the gate runs |

---

## Notes

- [P] marks different files with no dependency on an open task.
- The percentile output stays off by default, so the report change is confined
  to the `endpoint samples` value until a reader asks for the tail.
- The raw point columns die with the attempt recorder; the chunk value columns
  on the row are the residue N3 reads (D-14).
- Stop at any checkpoint and validate the story independently.
