# Tasks: Metric Formulas

**Input**: Design documents from `specs/021-metric-formulas/`

**Prerequisites**: plan.md, spec.md (FR-001 to FR-042, SC-001 to SC-009,
user stories US1 to US4, clarifications of 2026-10-19), research.md
(D-01 to D-14), data-model.md, quickstart.md, contracts/ (four files)

**Tests**: Included and mandatory. Principle III sets TDD order: each test
file is written first, observed failing for the missing behaviour, and the
observation line is recorded in the task. FR-038 needs a verdict for every
metric row, FR-039 keeps the fake provider as the only source of counts,
FR-040 needs 100% line and branch coverage with no exclusion marker, and
FR-041 forbids sleeping and clock reads in tests.

**Organization**: Phase 1 is scaffolding. Phase 2 lands the selection fix
that every host demonstration depends on. Phases 3 and 4 carry US1, the
metric-by-name path. Phases 5 and 6 carry US2 and US3. Phase 7 carries US4
and the documentation budget. Phase 8 carries the whole-tree sweep, coverage
evidence, the version move and the gates. Within a phase, test tasks precede
implementation tasks.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel with other [P] tasks (different files, no
  dependency)
- **[Story]**: US1 to US4, or C-1 to C-6 for capability work shared by
  stories
- Every description names the exact file path it touches

## Phase 1: Scaffolding (contract naming and registration)

**Purpose**: The named seams every later test compiles against: the public
header, the internal grammar header, the two data tables, the node kind
codes and the CTest registration.

**CRITICAL**: no story test compiles until this phase is complete.

- [ ] T001 [C-2] Add `include/speedgun-ng/counters_metrics.hpp` with the
  `MetricView` shape and the `metricByName` declaration in namespace
  `sg::counters`, per `contracts/metric-by-name.md`, including the header
  guard, the export macro and the doc comment the neighbouring headers use
- [ ] T002 [C-1] Add `source/counters/linux_pmu/metric_formula.hpp` with the
  `detail` token kinds, the parse-result shape and the nine `MetricRefusal`
  kinds of `data-model.md`
- [ ] T003 [C-1] Append the two node kind codes to `detail::ExprCore` in
  `include/speedgun-ng/counters_measurement.hpp`, extend the kind-code
  comment, and add the minimum and maximum arms to `detail::foldPairsCore`,
  `detail::metricCore` and `compileCore`
- [ ] T004 [C-1] Add `source/counters/linux_pmu/metric_tables.hpp` with the
  `ScaleUnit` conversion table and the literal-constant fact table, empty
  entries included so the sweep task fills them from measurement
- [ ] T005 [P] Add `source/counters/linux_pmu/metric_formula.cpp` with the
  namespace and the one-line file comment, and add the file to the counters
  sources in `CMakeLists.txt`
- [ ] T006 [P] Register `test/source/counters_metric_formula_test.cpp` and
  `test/source/counters_metric_lookup_test.cpp` in `test/CMakeLists.txt` on
  the existing `counters_*_test` pattern so both link against the counters
  objects, the fake provider and the test main
- [ ] T007 [P] Add `test/source/detail/metric_fixture.hpp` holding the small
  fixture tables the tests share: a cycle pair, a duplicated name pair, a
  conditional pair and a no-metric host identity, on the pattern of
  `test/source/detail/gcov_flush.hpp`

**Checkpoint**: `cmake --preset=ci-rocky && cmake --build build` configures
and builds with the new files present and the two new tests registered and
failing on their first assertion.

## Phase 2: Host table reachability (C-6, prerequisite for host work)

**Purpose**: Put the stepping into the mapping-file key so a model-55 part
selects a directory (FR-034 to FR-036). Without this phase the feature lists
no metric on the machine it is developed on.

**Independent Test**: run the selection fixtures for both sides of the
model-55 boundary and the class sweep over the pinned mapping file.

### Tests for this phase (write first, observe failure)

- [ ] T008 [C-6] In `test/source/counters_pmu_test.cpp`, add fixtures for
  stepping 3 and stepping 5 of family 6 model 85 and assert `skylakex` and
  `cascadelakex` respectively (FR-035)
- [ ] T009 [C-6] In the same file, add the sweep that walks every row of
  `external/pmu-events/arch/x86/mapfile.csv` carrying a stepping class and
  asserts a fixture key matches it (FR-036)
- [ ] T010 [C-6] In the same file, assert that a row without a class still
  matches the plain key, so the 45 other rows keep behaving as they do today

Observation: the two boundary fixtures fail with an empty selection, which is
the defect the spec records.

### Implementation for this phase

- [ ] T011 [C-6] Add the stepping field to `PmuIdent` in
  `source/counters/detail/pmu.hpp` and fill it where CPUID is read
- [ ] T012 [C-6] Extend `mapfileKey` in `source/counters/linux_pmu/table_parse.cpp`
  to render `<vendor>-<family>-<model>-<stepping>` and teach
  `pmuSelectDirectory` the character-class match `contracts/table-selection.md`
  specifies (FR-034)

**Checkpoint**: `ctest --test-dir build -R counters_pmu_test` passes; the
development host selects `skylakex`.

## Phase 3: User Story 1 - Formula parse (Priority: P1) part 1 of 2

**Goal**: Every construct of the FR-003 to FR-006 grammar parses from real
table text, and every refusal branch is reachable from a crafted input.

**Independent Test**: run the grammar test and the refusal corpus over the
pinned rows quoted in the test.

### Tests for User Story 1 (write first, observe failure)

- [ ] T013 [P] [US1] Create `test/source/counters_metric_formula_test.cpp`
  with one case per literal form of FR-003: decimal, hexadecimal, exponent,
  unary minus, dotted event names and the `:` privilege suffix
- [ ] T014 [P] [US1] In the same file, add the `unit@modifier@` cases
  `cstate_pkg@c6\-residency@` and
  `cpu_core@DTLB_LOAD_MISSES.STLB_HIT\,cmask\=1@`, so the escaped hyphen and
  the escaped comma each hold one token (FR-003)
- [ ] T015 [P] [US1] In the same file, add the operator and parenthesis cases
  of FR-003, including a nested parenthesised quotient and a subtraction that
  yields a negative term
- [ ] T016 [P] [US1] In the same file, add the five functions of FR-004: two
  runtime arguments, one runtime and one constant argument, and the
  `d_ratio` zero-denominator case that must answer through the
  `max(y, 1.0)` lowering
- [ ] T017 [P] [US1] In the same file, add the conditional cases of FR-005:
  `<` and `>` tests, a `has_event` test, a `source_count` test, and a missing
  `else`
- [ ] T018 [P] [US1] In the same file, add the literal-constant cases of
  FR-006: `#SMT_on`, `#num_packages`, `#num_cpus_online`,
  `#SYSTEM_TSC_FREQ` resolve, and `#has_pmem`, `#num_dies`, `#core_wide`
  refuse and name the constant
- [ ] T019 [P] [US1] In the same file, add the refusal corpus of FR-007:
  unbalanced parenthesis, trailing operator, doubled operator, empty
  expression, an unknown construct such as a ternary, and an unterminated
  modifier

Observation: each case fails on the missing parse entry point before any
implementation lands.

### Implementation for User Story 1

- [ ] T020 [C-1] Implement the token scan in
  `source/counters/linux_pmu/metric_formula.cpp`: numbers, identifiers with
  dotted suffixes, the `:` suffix, the `@modifier@` form with escape decoding
  and escaped commas, and the `#` constant (FR-003, D-08)
- [ ] T021 [C-1] Implement the expression parser over that scan: precedence,
  parentheses, unary minus, and the conditional form as a load-time selection
  node (FR-003, FR-005, D-01)
- [ ] T022 [C-1] Implement the five functions: `min` and `max` lower onto the
  new node kinds, `d_ratio` lowers to the guarded quotient with the
  integer-count limit written beside it, and `has_event` and `source_count`
  settle against the host encode set and the image (FR-004, FR-012, D-07)
- [ ] T023 [C-1] Implement the literal-constant resolution against the fact
  table, reading the object tree counts and the calibration frequency the
  library already publishes, and refuse a constant with no reader (FR-006,
  D-06)
- [ ] T024 [C-1] Implement the refusal record with its kind and one-line
  reason, and make the loader keep going over the rest of the table (FR-007)
- [ ] T025 [C-1] Extend `addEntry` in
  `source/counters/linux_pmu/table_parse.cpp` so a row with `MetricName` and
  `MetricExpr` becomes a metric definition while the existing refusal keeps
  metric rows out of the countable event catalog (FR-001, FR-013)
- [ ] T026 [C-1] Keep the parsed definitions beside the event table of the
  same directory with one load per directory and no path lookup (FR-002)

**Checkpoint**: `ctest --test-dir build -R counters_metric_formula_test`
passes; every grammar construct and every refusal branch has a driven test.

## Phase 4: User Story 1 - Resolve and ask by name (Priority: P1) part 2 of 2

**Goal**: References expand, cycles refuse, scales apply, and a caller gets a
foldable expression by name with the group and unit text beside it.

**Independent Test**: ask for `cpi`, `l2_mpi` and `tma_info_thread_ipc` over
the fake provider and compare against the same metrics composed by hand.

### Tests for User Story 1 (write first, observe failure)

- [ ] T027 [P] [US1] Create `test/source/counters_metric_lookup_test.cpp`
  with `cpi` and `l2_mpi` value checks over `FakeScript` counts, compared
  against a hand-composed `Expression` over the same events (FR-015, FR-018,
  FR-039)
- [ ] T028 [P] [US1] In the same file, add `tma_info_thread_ipc`, whose
  expression names `tma_info_thread_clks`, and assert one fold produces the
  value (FR-008)
- [ ] T029 [P] [US1] In the same file, add the cycle fixture from
  `test/source/detail/metric_fixture.hpp` and assert both members refuse with
  the cycle named while a third metric stays available (FR-009)
- [ ] T030 [P] [US1] In the same file, add the duplicated-name fixture and
  assert the first row answers and the second is refused as a duplicate
  (FR-011)
- [ ] T031 [P] [US1] In the same file, add the scale cases: a `100%` row
  carries the factor 100 in the value and the `100%` text in the unit, and an
  unknown spelling refuses the metric and names the text (FR-018, D-05)
- [ ] T032 [P] [US1] In the same file, add the miss cases: no metric table on
  the host, a name no metric carries with its near-miss diagnostics, and a
  metric that loaded and cannot be counted disclosing its kind and reason
  (FR-016, FR-017, FR-020)
- [ ] T033 [P] [US1] In the same file, add the load-once check that counts
  directory loads across two requests (FR-019)
- [ ] T034 [P] [US1] In the same file, add the dimension-refusal case of
  FR-010 and assert the reason names the operands

### Implementation for User Story 1

- [ ] T035 [C-1] Implement the name index and the reference resolution pass
  with the coloured walk in `source/counters/linux_pmu/metric_formula.cpp`
  (FR-008, FR-009, D-03, D-04)
- [ ] T036 [C-1] Implement the `ScaleUnit` conversion against the table in
  `source/counters/linux_pmu/metric_tables.hpp` and apply the factor through
  `scaleAll` exactly once (FR-018, D-05)
- [ ] T037 [C-1] Implement the dimension check on the typed composition rules
  and the duplicate-name rule that keeps the first row (FR-010, FR-011)
- [ ] T038 [C-2] Implement `metricByName` in
  `include/speedgun-ng/counters_metrics.hpp` and its definition in
  `source/counters/linux_pmu/metric_formula.cpp`, returning the view or a
  recoverable one-line error with near-miss diagnostics from the existing
  suggester (FR-015, FR-016, D-10)

**Checkpoint**: `ctest --test-dir build -R 'counters_metric_lookup_test|counters_metric_formula_test'`
passes; US1 stands on its own with no harness change.

## Phase 5: User Story 2 - See what the host can measure (Priority: P2)

**Goal**: `--catalog` lists the host's metrics with group, unit text,
availability or reason, and accounts for every row of the table.

**Independent Test**: run the listing on the development host and check the
line account against the table.

### Tests for User Story 2 (write first, observe failure)

- [ ] T039 [P] [US2] In `test/source/harness_catalog_test.cpp`, assert metric
  lines appear after the counter lines of the object path, one per row, with
  group, unit text and `mode=metric` (FR-021)
- [ ] T040 [P] [US2] In the same file, assert the counter lines keep their
  current shape, order and fields (FR-022)
- [ ] T041 [P] [US2] In the same file, assert an unavailable metric stays
  listed with its availability and reason, and that a host with no selected
  table prints no metric line and reports no error (FR-023, FR-024)
- [ ] T042 [P] [US2] Update every existing line-count fixture in that file
  that the new lines shift, in the same commit as the implementation task

### Implementation for User Story 2

- [ ] T043 [C-3] Print metric lines from `printObject` in
  `source/harness/catalog.cpp` in table order, refused rows included, with the
  one-line reasons the data model defines (FR-021, FR-023, D-11)

**Checkpoint**: `ctest --test-dir build -R harness_catalog_test` passes; the
listing accounts for the whole directory.

## Phase 6: User Story 3 - Name a metric on the command line (Priority: P2)

**Goal**: `--metric` adds columns in order, deduplicates, fails fast with
status 2 on an unknown name, and prints a refusal kind for a metric that
cannot be counted.

**Independent Test**: run a registered benchmark with one real name, one
repeated name and one unknown name.

### Tests for User Story 3 (write first, observe failure)

- [ ] T044 [P] [US3] In `test/source/harness_cli_test.cpp`, assert one column
  per named metric in command-line order, a repeated name giving one column,
  and a code-attached metric named again giving one column (FR-025, FR-026)
- [ ] T045 [P] [US3] In the same file, assert an unknown name prints one line
  and exits with status 2 before any measured output appears, and that a
  missing value behaves the same way (FR-028, FR-042)
- [ ] T046 [P] [US3] In the same file, assert a metric that exists and cannot
  be counted prints its refusal kind, the run continues, and the status is the
  run's own (FR-027)
- [ ] T047 [P] [US3] In the same file, compare a metric column value against
  the same metric composed by hand over identical fake counts (FR-029)

### Implementation for User Story 3

- [ ] T048 [C-4] Add the `--metric` row to the long-option table in
  `source/harness/cli.cpp` with the one-line status-2 path for a missing or
  unknown value (FR-042)
- [ ] T049 [C-4] Validate the named list before the first repetition in
  `source/harness/cli.cpp`, printing the first failing name and exiting with
  status 2 (FR-028)
- [ ] T050 [C-4] Build the metric columns from the named list in
  `source/harness/runner.cpp` after the attached metrics and before the
  counter addresses, keeping the plan and the recorder as the only read path
  (FR-025, FR-026, FR-029)

**Checkpoint**: `ctest --test-dir build -R harness_cli_test` passes and
`bash test/cli_shape.sh build` reports option parity.

## Phase 7: User Story 4 - Documentation and the page budget (Priority: P3)

**Goal**: The page teaches the capability through a block CI builds and runs,
the quick reference carries the new public symbol, and the detail lives in the
harness page.

**Independent Test**: copy the block over the canonical includes, build it,
run it, read the printed row.

### Tests for User Story 4 (write first, observe failure)

- [ ] T051 [P] [US4] Add the region to `example/docs/counter_metrics.cpp`
  that asks for `cpi` by name and prints the value with the group and unit
  text, so the region is the source of the page block (FR-030)
- [ ] T052 [P] [US4] Extend the hardware counter metrics section of
  `README.md` with that block, the option row and the quick-reference row for
  `metricByName`, keeping the section within `SECTION_LINE_CAP` (FR-030,
  FR-031, FR-032)
- [ ] T053 [P] [US4] Add the reader detail to `docs/pages/harness.md`, the
  page `USER_GUIDE_PAGES` already names (FR-033)

### Implementation for User Story 4

- [ ] T054 [C-5] Record the `PAGE_LINE_CAP` raise the added lines need in
  `tools/docs/example_drift.py`, together with the measured section size and a
  one-line comment naming the feature (FR-031, plan D-14)
- [ ] T055 [C-5] Run the drift gate, the snippet compile gate and the example
  run gate locally and fix any mismatch between page text and region text
  before the commit lands

**Checkpoint**: `ctest --test-dir build -R 'example_run|example_run_fixtures|snippet_compile|report_shape'`
passes; the page block prints the row the page names.

## Phase 8: Whole-tree verdict, coverage evidence, version and gates

**Purpose**: The discipline requirements that span the feature.

- [ ] T056 [C-1] Add the sweep test that walks every directory of
  `external/pmu-events/arch/x86` and records a verdict for every metric row,
  printing any row dropped without one (FR-038)
- [ ] T057 [C-1] Fill the `ScaleUnit` conversion table and the literal-constant
  fact table from the sweep's distinct-spelling report, so no spelling in the
  tree is unaccounted for (FR-018, D-05, D-06)
- [ ] T058 [P] Run the coverage capture, read `gcov -b` over the new files and
  close every branch gap with a fixture; a gap that cannot close stops the
  work and is reported with the `gcov -b` evidence (FR-040)
- [ ] T059 [P] Run the fake-provider value tests and the no-sleep, no-clock
  scan across the new test files (FR-039, FR-041)
- [ ] T060 [P] Run the real-PMU check on corsairone, record the run with the
  selected directory and the metric rows it prints, and keep it out of the
  unit suite (FR-039)
- [ ] T061 [P] Build `ci-ubuntu` so clang-tidy and cppcheck see the new files,
  and clear every finding
- [ ] T062 [P] Run `python3 tools/prose/prose_gate.py --check prose` and the
  commit-message gate over the branch's messages
- [ ] T063 Bump `PROJECT_VERSION` in `CMakeLists.txt` to the next free minor
  version at merge time, leave both `SOVERSION` numbers unchanged, and record
  in the commit body that no exported layout moved (FR-037)
- [ ] T064 Rebase onto `master`, run the full `ctest --test-dir build
  --output-on-failure` once more, and push with `--force-with-lease`

**Checkpoint**: every gate of Principle VIII is green on the branch and the
requirement table below has no uncovered row.

## Requirement coverage

| Requirement | Tasks |
| --- | --- |
| FR-001, FR-002, FR-013 | T025, T026 |
| FR-003 | T013, T014, T015, T020, T021 |
| FR-004 | T016, T022 |
| FR-005 | T017, T021, T022 |
| FR-006 | T018, T023, T057 |
| FR-007 | T019, T024 |
| FR-008, FR-009 | T028, T029, T035 |
| FR-010 | T034, T037 |
| FR-011 | T030, T037 |
| FR-012 | T017, T022 |
| FR-014 | T005, T061 |
| FR-015, FR-016 | T027, T032, T038 |
| FR-017 | T032, T060 |
| FR-018 | T031, T036, T057 |
| FR-019 | T033 |
| FR-020 | T032, T043 |
| FR-021 to FR-024 | T039, T040, T041, T042, T043 |
| FR-025 to FR-027 | T044, T046, T048, T050 |
| FR-028, FR-042 | T045, T048, T049 |
| FR-029 | T047, T050 |
| FR-030 to FR-033 | T051, T052, T053, T054, T055 |
| FR-034 to FR-036 | T008, T009, T010, T011, T012 |
| FR-037 | T063 |
| FR-038 | T056, T057 |
| FR-039 | T027, T059, T060 |
| FR-040 | T058 |
| FR-041 | T059 |
| Scaffolding and shared gates, no single FR | T001, T002, T003, T004, T006, T007, T061, T062, T064 |
