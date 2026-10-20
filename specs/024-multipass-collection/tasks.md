# Tasks: Multi-pass Collection

**Input**: Design documents from `specs/024-multipass-collection/`

**Prerequisites**: [spec.md](spec.md) (FR-001 to FR-037, SC-01 to SC-10,
user stories US1 to US4, clarifications of 2026-10-20, decisions D-1 to
D-12), [plan.md](plan.md), [research.md](research.md) (D-01 to D-15),
[data-model.md](data-model.md) (E-01 to E-08, invariants I-1 to I-6),
[quickstart.md](quickstart.md), [contracts/](contracts/) (four files)

**Tests**: Included and mandatory. Principle IX pairs every code deliverable
with a covering test task that lands before the code task it gates. FR-032
sets 100 percent line and 100 percent branch coverage with no new exclusion
marker, FR-033 puts every split, merge, disclosure and warning branch on the
fake provider with a scripted limit, and FR-037 keeps `ctest -j 2` green with
`-fprofile-update=atomic` and no `-D_GLIBCXX_ASSERTIONS`.

**Organization**: Phase 1 is setup on the shared build files. Phase 2 lands
the counters limit fact that every story reads. Phases 3 to 6 carry US1 to
US4 in spec priority order. Phase 7 carries coverage evidence, the recorded
real-PMU run and the whole gate set. Within a phase the test tasks precede
the code tasks they gate. Declarative scaffolding can land ahead of its tests
when it holds no behavior: the header declarations of T004, the translation
unit of T005, the test registrations of T006 and T007 and the fixture
constants of T007 are that scaffolding.

## Path Conventions

Public headers `include/speedgun-ng/`, counters sources
`source/counters/`, harness sources `source/harness/` with internal headers
under `source/harness/detail/`, tests `test/source/` with shared fixtures
under `test/source/detail/`, docs examples `example/docs/`, host-facing
scripts `tools/`, gate scripts `test/`. Both archives glob their sources
recursively (`CMakeLists.txt:192` for the harness, `CMakeLists.txt:851` for
counters), so a new translation unit needs no CMake source-list edit; a new
test needs an explicit block in `test/CMakeLists.txt`.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel with other [P] tasks (different files, no
  dependency on an open task)
- **[Story]**: US1 to US4, carried by the tasks of that story's phase. Setup,
  foundational and polish tasks carry no story label.
- Every description names the file it touches, by path or by the
  `In the same file` back reference that `specs/022-metric-formulas/tasks.md`
  also uses

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: The shared build files and the empty seams every later task
fills: the version and the two `SOVERSION` numbers, the new harness
translation unit and its internal header, the five new test executables and
the fixture they share.

**CRITICAL**: no story test compiles until this phase is complete.

- [X] T001 Move `VERSION 0.12.0` to `0.13.0` at `CMakeLists.txt:7`, the next
  free minor at the audit point, with the one-line comment naming
  `specs/024-multipass-collection` (FR-031, plan D-12)
- [X] T002 Raise the counters archive `SOVERSION` from 2 to 3 at
  `CMakeLists.txt:47` and state above it that `ObjectSeed` gains
  `programmableCounters`, `Object` answers it, and `FakeProvider` gains the
  scripted fact, in the voice of the notes at `CMakeLists.txt:215-224`
  (FR-031, plan D-12)
- [X] T003 Raise the harness archive `SOVERSION` from 7 to 8 at
  `CMakeLists.txt:225` and state above it that `BenchmarkHandle` gains
  `metricMethod`, `BenchmarkResult` gains `metricMethod`, `passCount` and
  `passSpread`, and `MetricValue` gains `pass` (FR-031, plan D-12)
- [X] T004 [P] Add `source/harness/detail/pass_plan.hpp` with the E-03 pass
  plan shape and the declarations of `settleMethod`, `planPasses`,
  `mergePasses` and `passSpread` in the shape
  `source/harness/detail/percentiles.hpp` uses, each with the Doxygen
  contract the header style requires (D-05, D-07, D-08)
- [X] T005 Add the translation unit `source/harness/pass_plan.cpp` with
  the license header, the include of its own detail header and the one-line
  file comment (FR-032)
- [X] T006 Register `test/source/counters_counter_limits_test.cpp` and
  `test/source/counters_objects_limit_test.cpp` in `test/CMakeLists.txt` with
  one `add_executable` plus `add_test` block each on the pattern of
  `test/CMakeLists.txt:433-439`, and create both files holding the suite
  skeleton so the build stays green before the first assertion lands
- [X] T007 Register `test/source/harness_pass_plan_test.cpp`,
  `test/source/harness_pass_collect_test.cpp` and
  `test/source/harness_pass_report_test.cpp` in `test/CMakeLists.txt` with
  one `add_executable` plus `add_test` block each against
  `speedgun-ng::harness` with the `source/harness` include directory, on the
  pattern of `test/CMakeLists.txt:561-565`, and create the three files with
  the same skeleton rule
- [X] T008 [P] Add `test/source/detail/pass_fixture.hpp` with the scripted
  fake the pass tests share: a `cpu` object with a settable limit, four
  metric columns of two PMU events each, one column of five events, and the
  per-pass reference time sets of the spread-fixture table of
  [data-model.md](data-model.md) E-06, which cover the in-band, the boundary,
  the warning, the two-pass, the equal-time and the zero-median shapes
  (FR-033; the pattern of
  `test/source/detail/metric_fixture.hpp`). The same header records the
  one-pass per-iteration provider read count that the existing no-allocation
  check observes at the audit point as the named constant
  `kAuditPointReadsPerIteration`, so the timed-path tasks compare against one
  recorded number (SC-09)

**Checkpoint**: `cmake --preset=ci-rocky && cmake --build build` configures
and builds with the new translation unit, the five test executables and the
shared fixture present, and `ctest --test-dir build -j2` still passes.

---

## Phase 2: Foundation (Blocking Prerequisites)

**Purpose**: The programmable-counter limit as a published fact of the
counters library (E-02) and as a fake parameter. Every story's split decision
reads these two names.

**CRITICAL**: no story work begins before this phase is complete. Master
publishes no counter count anywhere (`spec.md` PC-3), so a split that probed
for the limit would observe nothing: the kernel grants an oversize group and
rotates it afterwards.

### Tests for this phase (write first, observe failure)

- [X] T009 [P] Write the CPUID decode decision table in
  `test/source/counters_counter_limits_test.cpp` over
  `programmableCountersFromCpuid(maxLeaf, eax, ebx)` with synthetic
  registers: a maximum leaf below 0AH answers nothing, version 0 answers
  nothing, count 0 answers nothing, `EAX=0x07300404` answers 4, and a nonzero
  `EBX` vector leaves the answer unchanged (FR-005, FR-032, research D-01,
  D-04)
- [X] T010 [P] Write the publication test in
  `test/source/counters_objects_limit_test.cpp`: a scripted fake object
  publishes its limit through `Object::programmableCounters()`, an
  unscripted object publishes nothing, a `machine` clock object publishes
  nothing, and two scripted sources carry two different limits (FR-005,
  FR-006, E-02)

Observation: both files fail on the missing names before T011 lands.

### Implementation for this phase

- [X] T011 Add the trailing member `programmableCounters` of type
  `std::optional<std::size_t>` to `ObjectSeed` in
  `include/speedgun-ng/counters_provider.hpp`, so every existing designated
  initializer keeps compiling with the member default-constructed (FR-005,
  research D-02)
- [X] T012 Declare `Object::programmableCounters()` in
  `include/speedgun-ng/counters_system.hpp` and define it in
  `source/counters/system.cpp` beside the accessors that start at line 403,
  answering the published value of the object's event source with no
  precondition (FR-005, research D-02)
- [X] T013 [P] Add `source/counters/linux_pmu/counter_limits.hpp` and
  `source/counters/linux_pmu/counter_limits.cpp` holding the branchless
  `__get_cpuid` probe for leaf 0 and leaf 0AH and the pure decode
  `programmableCountersFromCpuid`, with the measured `EBX=0` host state
  recorded beside the parameter it ignores (FR-005, FR-032, research D-01,
  D-04)
- [X] T014 Fill `programmableCounters` on the `cpu` object and on every PMU
  device object in the enumerate seeds of
  `source/counters/linux_pmu/provider.cpp`, leaving it empty on a host whose
  leaf reads version 0 or count 0 (FR-005, research D-01, D-02)
- [X] T015 Add the fluent `setProgrammableCounters(objectPath, limit)` to
  `FakeProvider` in `include/speedgun-ng/counters_fake.hpp` beside
  `addObject`, record it beside the object's seed data, publish it at
  enumerate in `source/counters/fake_provider.cpp`, and trap a path the fake
  owns no object for (FR-006, research D-03; the `\pre` and `\post` of
  `contracts/programmable-counters-fact.md`)

**Checkpoint**: `ctest --test-dir build -R 'counters_counter_limits_test|counters_objects_limit_test'`
passes; the fake publishes a scripted limit and the PMU provider publishes the
decoded one.

---

## Phase 3: User Story 1 - Measure more metrics than the hardware can count
at once (Priority: P1) MVP

**Goal**: A metric set wider than the published limit splits into passes by
first fit in request order, the benchmark runs once per pass, and one merged
row set carries each column from the pass that measured it.

**Independent Test**: `harness_pass_plan_test` and `harness_pass_collect_test`
over `test/source/detail/pass_fixture.hpp`: every metric column answers from a
pass that ran at ratio 1.0, the time columns come from pass 0, and no metric is
dropped. No option, no report line and no documentation is needed.

### Tests for User Story 1 (write first, observe failure)

- [X] T016 [US1] In `test/source/harness_pass_plan_test.cpp`, add the
  split table: a limit of 4 and four columns of two PMU events each give pass
  0 with columns 0 and 1 and pass 1 with columns 2 and 3, in request order
  (FR-001, FR-002, US1 scenario 1)
- [X] T017 [US1] In the same file, add the determinism check that two
  calls over the same list, method and limit give an identical assignment, and
  the minimum-count check that the pass count equals the first-fit minimum on
  the fixture limits 1, 4 and 8 (FR-002, SC-02, US1 scenario 4)
- [X] T018 [US1] In the same file, add the over-limit case: one column of
  five events on a limit of 4 stands alone in a pass flagged exclusive while
  the other columns keep their own passes (FR-003, FR-004, US1 scenario 2)
- [X] T019 [US1] In the same file, add the shapes of E-03: an empty column
  set produces no plan, a limit of 1 places one column per pass, the 64-column
  set places every column with no limit on the pass count, and two columns that
  share one leaf address claim that counter once inside their shared pass
  (FR-001, FR-002, E-03 rules 2 and 5, the spec's large-set edge)
- [X] T020 [US1] In the same file, add the source-limit cases: leaves whose
  source publishes no limit are costed against nothing and ride every pass that
  needs them, the two fixed machine clock leaves ride every pass and count
  against none, and a PMU `cycles` event a metric names counts in the pass that
  carries it (FR-005, FR-007, D-4, the two-provider edge)
- [X] T021 [US1] In the same file, add the merge table: each metric column
  and each user-counter column takes the values of its measuring pass at its
  own position, the time columns, the overhead floor, the endpoint samples, the
  context disclosures, the chunk disclosure and the primary percentile columns
  come from pass 0, a metric percentile comes from that metric's own pass, and
  a column whose pass folded a gap keeps `Availability::GAP` (FR-014, FR-015,
  FR-016, E-05, the percentile and gap edges)
- [X] T022 [US1] In `test/source/harness_pass_collect_test.cpp`, add the
  end-to-end fake run: a limit of 4 with the four two-event columns runs two
  passes, every metric column reports its value at running ratio 1.0, no column
  is dropped, calibration runs once, and both passes reuse its iteration count
  (FR-009, FR-010, SC-01, US1 scenario 1). Add the pause-and-resume arm: one
  `pauseTiming` and `resumeTiming` pair folded inside each pass, so each pass
  nets the interval it excludes and the merged row reads the reference pass's
  net time per iteration (FR-009, FR-014, FR-015)
- [X] T023 [US1] In the same file, add the over-limit run: the five-event
  column reports a running ratio below 1.0 from the fake's rotation while the
  other columns keep 1.0 (FR-004, SC-01, US1 scenario 2)
- [X] T024 [US1] In the same file, add the window-lifetime checks: each
  pass opens its plan before its first timed window and closes it after its
  last, the plan is computed before the first window and reads no counter, the
  merge runs after the last pass closes, and a one-pass metric run keeps the
  per-iteration provider read count equal to
  `kAuditPointReadsPerIteration` of `test/source/detail/pass_fixture.hpp`
  (FR-008, FR-012, FR-016, SC-09, spec D-10)
- [X] T025 [US1] In the same file, add the threaded order case: two
  contexts and two passes complete pass 0 in both contexts before pass 1 opens
  in either, the per-pass window open counts show no overlap, the run chooses
  contexts once so every pass reports the same placement, and the assertions
  read the fake's action counts after the joins with no sleep and no clock read
  (FR-011, FR-013, FR-034, SC-10)

Observation: every case above fails on the missing planner before T026 lands.

### Implementation for User Story 1

- [X] T026 [US1] Implement `planPasses` in `source/harness/pass_plan.cpp` with
  the E-03 placement rules 1 to 7: metric columns in `metricLabels` order then
  user-counter columns in open order, a per-source cost over deduplicated leaf
  addresses, first fit under each published limit, an over-limit column alone in
  an exclusive pass, no-limit sources riding every pass they need, and no plan at
  all when no needed source publishes a limit, with the `SG_ASSERT`
  post-conditions of invariants I-1 and I-2 (FR-001, FR-002, FR-003, FR-004,
  FR-005, FR-007, D-1, research D-05)
- [X] T027 [US1] Implement `mergePasses` in `source/harness/pass_plan.cpp` as a
  pure function over the per-pass fold results that reads provenance from the
  plan, records the measuring pass per column, keeps `GAP` and every refusal
  kind unchanged, and re-reads no counter (FR-014, FR-015, FR-016, I-5,
  research D-07)
- [X] T028 [US1] Wrap the measured attempts of `source/harness/runner.cpp` in
  the pass loop: calibration before pass 0, one leaf list per pass built from
  the plan and compiled through the existing `compileCore` site at lines
  1252-1253 and the per-context site at lines 1727-1730, the loop outside the
  threaded dispatch with the existing join between passes, and `mergePasses`
  called after the last pass closes; the existing one-column rule keeps a
  metric attached in code and named again on the command line as one column in
  one pass (FR-008, FR-009, FR-010, FR-011, FR-012, FR-013, FR-016, spec D-10,
  research D-06)

**Checkpoint**: `ctest --test-dir build -R 'harness_pass_plan_test|harness_pass_collect_test'`
passes and `ctest --test-dir build -j2` stays green. US1 stands on its own.

---

## Phase 4: User Story 2 - Choose the collection method (Priority: P2)

**Goal**: `auto`, `passes` and `multiplex` reach the run through the
`metricMethod` handle selector and through `--metric-method`, the command line
wins, one run settles exactly once, and a bad word costs no measured time.

**Independent Test**: `harness_pass_plan_test` drives the whole E-01
resolution table, `harness_cli_test` drives the refusals and the precedence,
and `harness_pass_collect_test` drives one registered benchmark under each
method over the fake provider.

### Tests for User Story 2 (write first, observe failure)

- [ ] T029 [P] [US2] In `test/source/harness_pass_plan_test.cpp`, add every
  row of the E-01 resolution table: no metric column, a host with no limit,
  a forced `multiplex`, a forced `passes`, an `auto` set that fits and an
  `auto` set that does not, each asserting the settled method, whether the
  planner runs and whether the console line prints (FR-023, FR-024, FR-025,
  E-01, research D-10)
- [ ] T030 [US2] In `test/source/harness_cli_test.cpp`, add the refusal
  table: `--metric-method=fast` prints one line naming the quoted word and
  exits with status 2 before any benchmark output appears, an empty word
  prints the same line, `Auto` is a bad word, and
  `--metric-method=passes --metric-method=multiplex` applies the last word
  (FR-026, SC-04, US2 scenario 3, `contracts/metric-method-option.md`)
- [ ] T031 [US2] In the same file, add the precedence case: a family whose
  handle asks for `multiplex` collects by passes when the command line asks for
  `passes`, and the settled method published for the run names `passes`
  (FR-025, US2 scenario 4). Add the early-exit arms: with
  `--metric-method=passes` and `--list`, and again with `--catalog`, the run
  prints its list, prints no `passes` line, and settles no method; with a bad
  word and `--list`, the refusal and status 2 still come first (FR-025, FR-026,
  `contracts/metric-method-option.md`)
- [ ] T032 [US2] In `test/source/harness_pass_collect_test.cpp`, add the
  `multiplex` run: the forced method compiles one plan holding every PMU event
  of the run, the planner stays out of it, and each metric column discloses the
  scripted ratio below 1.0 that master's single-plan path discloses (FR-024,
  US2 scenario 1)
- [ ] T033 [US2] In the same file, add the no-limit host cases: on a host
  whose needed sources publish no limit, both the `auto` run and a forced
  `passes` run keep the single kernel-multiplexed plan and settle to
  `MULTIPLEX` (FR-024, spec D-11, the no-limit edge)
- [ ] T034 [US2] In the same file, add the selector case:
  `metricMethod(MetricMethod::PASSES)` on a family with a fitting set runs one
  pass, publishes a pass count of 1 together with the forced record, and chains
  like `showPercentiles` (FR-023, FR-025, US2 scenario 2, E-01)

Observation: the refusal cases fail on the missing option and the table cases
fail on the missing selector before T035 lands.

### Implementation for User Story 2

- [ ] T035 [US2] Add `enum class MetricMethod` with `AUTO`, `PASSES` and
  `MULTIPLEX`, the fluent `BenchmarkHandle::metricMethod` selector with its
  Doxygen contract, and the forced record on the entry in
  `include/speedgun-ng/benchmark.hpp` beside `showPercentiles` at lines
  1782-1795 (FR-023, FR-025, E-01, research D-09,
  `contracts/metric-method-selector.md`)
- [ ] T036 [US2] Add the `--metric-method` row to the long-option table in
  `source/harness/cli.cpp` beside the percentiles pair at lines 303-304, with
  the one-line stderr refusal that names the quoted word, the exit status 2
  taken before any measurement, the last-flag-wins overwrite, and the matching
  `--help` line (FR-023, FR-026, research D-09)
- [ ] T037 [US2] Implement `settleMethod` in `source/harness/pass_plan.cpp` as
  the E-01 table with the command-line-over-entry-over-`auto` priority, and call
  it in `source/harness/runner.cpp` before the first window so one run settles
  once and the `multiplex` path keeps master's single `compileCore` call
  (FR-024, FR-025, I-6, research D-09, D-10)
- [ ] T038 [US2] Run `bash test/cli_shape.sh`, the run-control shape gate over
  the entry point, after the option row lands, so the edited
  `source/harness/cli.cpp` keeps the shape the gate reads (constitution VIII;
  option-table parity is the `example_drift` option check of T053 and T056)

**Checkpoint**: `ctest --test-dir build -R 'harness_pass_plan_test|harness_cli_test|harness_pass_collect_test'`
passes and `bash test/cli_shape.sh` reports the entry point's shape clean.

---

## Phase 5: User Story 3 - Read the method, the pass count and the spread in
the report (Priority: P2)

**Goal**: A multi-pass run states how it was collected: the `passes` line, the
pass count, the spread, the warning above 5 percent and the per-column pass tag,
with the same three values published on the result object, while a one-pass
`auto` run prints master's bytes.

**Independent Test**: `harness_pass_report_test` drives the disclosure shapes,
the warning predicate and the byte-identity fixture over the fake provider.

### Tests for User Story 3 (write first, observe failure)

- [ ] T039 [US3] In `test/source/harness_pass_report_test.cpp`, add the
  two-pass shape: one line naming `method=passes` with `count=2` and the spread,
  where an even pass count takes the lower of its two central times as the
  median, and each metric column carrying `pass=0` or `pass=1` in the existing
  metric field spelling (FR-017, FR-019, US3 scenario 1)
- [ ] T040 [US3] In the same file, add the spread fixtures of the
  spread-fixture table of [data-model.md](data-model.md) E-06: the in-band set,
  the boundary set that prints `spread=5.0%` with no warning line, and the
  three-pass warning set with its `warning:` line (FR-017, FR-018, SC-05, US3
  scenario 2, research D-08)
- [ ] T041 [US3] In the same file, add the arms the formula cannot answer and
  the second warning case: a zero median prints `spread=not available` with no
  warning and no failure, the three-pass fixture 100, 104 and 108 prints
  `spread=7.7%` with the warning line, and the two-pass fixture 100 and 108
  prints `spread=8.0%` with the warning line, the lower-of-two median of D-6
  being what gives the two-pass value its 8.0 (FR-017, FR-018, E-06, the
  zero-median edge, research D-14)
- [ ] T042 [US3] Add the byte-identity fixture
  `test/source/detail/pass_identity.expected` captured from a `ci-rocky` build of
  the audit point for one fitting metric run, and assert in the same file that an
  `auto` run of that fixture prints those bytes field for field with no `passes`
  line and no `pass=` token (FR-021, SC-03, US3 scenario 3, D-9, research D-14).
  The test binary receives the fixture path as its first command-line argument,
  which `add_test` passes through a generator expression over
  `CMAKE_CURRENT_SOURCE_DIR`, and the test reads the file with `std::ifstream`.
- [ ] T043 [US3] In the same file, add the `multiplex` settling shape: the
  disclosure names `method=multiplex` with `count=1`, prints no `spread` field,
  and keeps `ratio=`, `scaled=` and `gap=` in their current spelling (FR-020, US3
  scenario 4)
- [ ] T044 [US3] In the same file, add the print rule: a forced method with
  one pass prints the line, and a run that carries no metric column prints no
  line even when a method is forced (FR-021, clarification Q2, research D-10)
- [ ] T045 [US3] In the same file, add the published-field checks: the
  merged result of a two-pass run, of a `multiplex` settling and of a run with no
  metric column carries the `metricMethod`, `passCount` and `passSpread` values
  the console prints (FR-017, SC-06, clarification Q4, E-07)
- [ ] T046 [US3] In `test/source/harness_context_report_test.cpp`, add the
  untouched check: the context disclosure lines of a threaded two-pass run equal
  the expected bytes, held as an inline literal in the test, so
  `printContextDisclosure` stays byte-identical; no capture step and no history
  read carry those bytes (FR-022, spec D-8)

Observation: each shape fails on the missing `passes` line and on the missing
result fields before T047 lands.

### Implementation for User Story 3

- [ ] T047 [US3] Add `MetricValue::pass` beside `MetricValue` at line 211 and
  the `metricMethod`, `passCount` and `passSpread` fields beside `percentilePoints`
  at line 492 of `include/speedgun-ng/benchmark.hpp`, filled by the merge path
  (FR-017, FR-019, E-07, E-08)
- [ ] T048 [US3] Implement `passSpread` in `source/harness/pass_plan.cpp` as one
  pure decision over the merged reference time per iteration: fewer than two passes
  answers nothing, a zero median answers nothing, otherwise `(max - min) / median
  * 100`, with the warning predicate being a value that exists and exceeds 5.0 on
  the unrounded figure (FR-017, FR-018, E-06, D-6, research D-08)
- [ ] T049 [US3] Add `printPassDisclosure` in `source/harness/report.cpp` in the
  tagged-line format of the percentile line at line 306 and the chunks line at
  lines 358-362, with the warning voice of the `std::printf` inside
  `printNoiseWarning` at lines 251-255, called from `printResult`
  beside the existing disclosure call sites at lines 537 and 558, and printing when
  the run carries a metric column and the settled method is `MULTIPLEX` or the pass
  count exceeds 1 or the caller forced a method (FR-017, FR-018, FR-020, FR-021,
  FR-022, research D-10, research D-11, `contracts/pass-disclosure.md`)
- [ ] T050 [US3] Add the conditional `pass=%zu` token after `gap=` in the metric
  field loop of `source/harness/report.cpp` at lines 489-504, printed when the pass
  count exceeds 1, so a one-pass run keeps the four tokens master prints
  (FR-019, FR-021, research D-11)

**Checkpoint**: `ctest --test-dir build -R 'harness_pass_report_test|harness_context_report_test'`
passes; the identity fixture matches the audit-point capture byte for byte.

---

## Phase 6: User Story 4 - Read the page and copy a working block (Priority:
P3)

**Goal**: The main page carries the capability as a section owned by a built and
run example, the quick reference and the option table carry the new names, the
harness page carries the detail, and the page budget records the lines the
section added.

**Independent Test**: copy the block over the canonical includes, build it, run
it, read the printed row; `example_drift` and `example_run` do the same in CI.

### Tests for User Story 4 (write first, observe failure)

- [ ] T051 [P] [US4] Add `example/docs/metric_passes.cpp` with the marked region
  that reads the host's published limit through `Object::programmableCounters`,
  asks for metric columns past it and prints the `passes` disclosure, and add
  `example/docs/metric_passes.expected` recording the run on corsairone with the
  no-PMU shape beside the PMU shape, on the pattern of
  `example/docs/percentiles.expected` (FR-027, research D-13)
- [ ] T052 [P] [US4] Add the `Multi-pass collection` section to `README.md`
  between `Hardware counter metrics` and `Skips`, within the 25 lines of
  `SECTION_LINE_CAP`, with one purpose sentence, the copied region and one result
  sentence (FR-027, spec Budget note, research D-13)
- [ ] T053 [US4] Add the three rows FR-028 names to `README.md`: the
  quick-reference rows for `metricMethod` and
  `Object::programmableCounters` and the `--metric-method` option row, beside
  the `metricByName` row at line 596 and the `--percentiles`
  row at line 654 (FR-028)
- [ ] T054 [P] [US4] Add the reader detail to `docs/pages/harness.md`: the
  first-fit split rule, the merge rule, the spread formula with its warning
  threshold, and the fallback of spec D-11 on a host that
  publishes no limit (FR-029)

### Implementation for User Story 4

- [ ] T055 [US4] Register the example with `add_docs_example(metric_passes)` in
  `example/CMakeLists.txt` beside `add_docs_example(counter_metrics)`, which gives
  the `docs_` target, the `run_docs_` runner and the `run-examples` dependency the
  run gate walks (`example/CMakeLists.txt:40-48`, FR-027)
- [ ] T056 [US4] Add `Multi-pass collection` to `SECTIONS` at
  `tools/docs/example_drift.py:30` and its owner file to `SECTION_OWNERS` at line
  57, then raise `PAGE_LINE_CAP` at line 124 to the measured post-change line count
  of `README.md` with the comment naming this feature and the measured figure, in
  the voice of the raise recorded at lines 115-123 (FR-030, SC-08, spec Budget
  note)
- [ ] T057 [US4] Run `python3 tools/docs/example_drift.py` and
  `sh test/example_run.sh build` and clear every difference between the page block
  and the region it copies, in the same commit as T052 and T056 (FR-028, FR-030,
  SC-08)

**Checkpoint**: `ctest --test-dir build -R 'example_drift|example_run'` passes;
`docs_metric_passes` prints the disclosure the page quotes.

---

## Phase 7: Polish and Cross-Cutting Concerns

**Purpose**: The discipline requirements that span the feature: coverage with
evidence, the determinism scan, the cost rule, the recorded real-PMU comparison,
the analyzers, the gate set and the landing move.

- [ ] T058 [P] Run the capture through the `coverage` target, read `gcov -b` over
  `source/harness/pass_plan.cpp`, `source/harness/report.cpp`,
  `source/harness/cli.cpp`, `source/harness/runner.cpp`,
  `source/counters/linux_pmu/counter_limits.cpp`,
  `source/counters/linux_pmu/provider.cpp`, `source/counters/fake_provider.cpp` and
  `source/counters/system.cpp`, and close every branch gap with a fake fixture; a
  gap that cannot close stops the work and is reported with the `gcov -b` evidence.
  Record the line, branch and DBC counts the target reports for the new files and
  for the files this feature edits, beside that evidence (FR-032, SC-07,
  constitution VI)
- [ ] T059 [P] Confirm `grep -rn 'LCOV_EXCL' include source` still counts 398
  marker lines and that no file this feature adds carries a marker, a waiver or a
  partial row (FR-032, PC-5)
- [ ] T060 [P] Scan `test/source/counters_counter_limits_test.cpp`,
  `test/source/counters_objects_limit_test.cpp`,
  `test/source/harness_pass_plan_test.cpp`,
  `test/source/harness_pass_collect_test.cpp` and
  `test/source/harness_pass_report_test.cpp` for a sleep, a wall-clock read or a
  PMU dependency, then run `ctest --test-dir build -j2 --output-on-failure` under
  the coverage preset that keeps `-fprofile-update=atomic` and passes no
  `-D_GLIBCXX_ASSERTIONS` (FR-033, FR-037, SC-10)
- [ ] T061 [P] Run the existing allocation and overhead checks in
  `test/source/counters_noalloc_test.cpp` and `tools/dbc/overhead.sh` and record
  that the per-iteration provider read count of a one-pass metric run equals
  `kAuditPointReadsPerIteration` of `test/source/detail/pass_fixture.hpp`
  (SC-09)
- [ ] T062 [P] Add `tools/pmu_events/multipass_comparison.sh` and run it on
  corsairone: the six-event over-limit set under `--metric-method=multiplex`,
  `passes` and `auto`, one single-metric reference run per event on a master build,
  wall time per configuration and a quiet-window noise marker, with the output
  recorded under `specs/024-multipass-collection/`; the script stays out of CMake
  and no CI job depends on it (FR-035, SC-01, research D-15)
- [ ] T063 [P] Build `ci-ubuntu` so clang-tidy and cppcheck see the new files,
  and clear every finding (constitution VIII)
- [ ] T064 [P] Run the `format-check`, `spell-check`, `prose-lint` and
  `dbc-gate` build targets, the `dbc_dependency_scan`, `time_source_gate`,
  `cli_shape`, `loop_shape` and `barrier_shape` tests, and the commit-message
  gate over every message on the branch (FR-036, constitution VIII and XI)
- [ ] T065 Rebase onto `master`, keep both lanes' lines in `CMakeLists.txt`,
  `README.md`, `example/CMakeLists.txt` and `test/CMakeLists.txt`, retake the
  project version at `CMakeLists.txt:7` and both `SOVERSION` lines at
  `CMakeLists.txt:47` and `CMakeLists.txt:225` against the then-current master,
  so the merged state carries the next free minor version and the next free
  number for each archive (FR-031), re-measure `README.md` and raise
  `PAGE_LINE_CAP` again by
  this section's own lines, re-capture
  `test/source/detail/pass_identity.expected` against the rebased master, run
  `ctest --test-dir build --output-on-failure` once more, and push with
  `--force-with-lease` (FR-031, spec Constraints)
- [ ] T066 Show `git ls-remote origin 024-multipass-collection`, read the CI logs
  of the pushed commit, fix its failures in this lane, and record in
  `specs/024-multipass-collection/tasks.md` the line, branch and DBC coverage
  counts and the gate list that run reports (SC-07, spec Constraints)

**Checkpoint**: every gate of Principle VIII is green on the pushed state and the
table below has no uncovered row.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1** has no dependencies.
- **Phase 2** depends on Phase 1 and blocks every story: the published limit and
its fake parameter are the input of every split decision (FR-005, FR-006).
- **Phase 3 (US1)** depends on Phase 2 and is the MVP.
- **Phase 4 (US2)** depends on Phase 3: the method selects between the planner
path Phase 3 builds and the single plan master keeps.
- **Phase 5 (US3)** depends on Phase 3 for the pass count and on Phase 4 for the
settled method and the forced record.
- **Phase 6 (US4)** depends on Phases 3 to 5, because the page quotes the
finished disclosure shape.
- **Phase 7** depends on every story the branch ships.

```text
Phase 1 Setup
   |
   v
Phase 2 Limit fact (counters + fake)  --- blocks every story
   |
   v
Phase 3 US1 plan -> passes -> merge   (MVP)
   |--------------|
   v              v
Phase 4 US2     Phase 5 US3
settle+option   spread + disclosure + pass tag
   |--------------|
   v
Phase 6 US4 docs chain
   |
   v
Phase 7 coverage, recorded run, gates, landing
```

### Story Independence

- **US1** is the feature: split, per-pass runs, merge. It stands with no option,
no report line and no documentation.
- **US2** controls what US1 built. Its own tests read the settled method and the
pass count from the result object.
- **US3** states what US1 and US2 decided. Its golden covers the printed method
token and pass count for all three methods, so the printed half of US2's
independent test lands with the printer.
- **US4** documents the finished shape and owns its example.

### Within Each User Story

- Tests first, and they fail on the missing behaviour before the code lands.
- Limit fact before planner, planner before pass loop, merge before report,
report before documentation.
- Story complete before the next priority starts.

### Parallel Opportunities

- Phase 1: T004 and T008 run together; T001 to T003 edit one file and run
in sequence; T005 follows T004, and T006 and T007 share
`test/CMakeLists.txt` and run in sequence.
- Phase 2: T009 and T010 run together; T013 runs beside T011 and T012.
- Phase 3: the tables of `test/source/harness_pass_plan_test.cpp` (T016 to
T021) run beside the tables of `test/source/harness_pass_collect_test.cpp`
(T022 to T025); inside either file the tasks run in sequence.
- Phase 4: three file groups run together: T029 in
`harness_pass_plan_test.cpp`, T030 and T031 in `harness_cli_test.cpp` in
sequence, and T032 to T034 in `harness_pass_collect_test.cpp` in sequence.
- Phase 5: T039 to T045 share `harness_pass_report_test.cpp` and run in
sequence; T046 runs beside them in `harness_context_report_test.cpp`.
- Phase 6: T051 and T054 run together; T052 and T053 share `README.md` and
run in sequence; T055 follows T052, and T056 and T057 close the story after
the page and the tool lists land.
- Phase 7: T059, T060, T062 and T063 run together.

---

## Parallel Example: User Story 1

```bash
# Split and merge decisions, one file, in sequence:
Task: "Add the split, determinism, over-limit, shape and merge tables to test/source/harness_pass_plan_test.cpp"
# Execution over the fake provider, the other file, in sequence:
Task: "Add the two-pass run, the over-limit run, the window lifetime and the threaded order cases to test/source/harness_pass_collect_test.cpp"
# Then the pure planner, then the loop that uses it:
Task: "Implement planPasses in source/harness/pass_plan.cpp"
Task: "Implement mergePasses in source/harness/pass_plan.cpp"
Task: "Wrap the measured attempts in the pass loop in source/harness/runner.cpp"
```

---

## Implementation Strategy

### MVP First (User Story 1)

1. Complete Phase 1: the version move, the new translation unit, the test
registration, the shared fixture.
2. Complete Phase 2: the decoded limit, its publication, the fake parameter.
3. Complete Phase 3: `planPasses`, `mergePasses`, the pass loop.
4. **Stop and validate**: `harness_pass_plan_test` and
`harness_pass_collect_test` prove the split, the runs and the merge with no
option and no report change.

### Incremental Delivery

1. Phases 1 and 2 publish the fact; no run changes behavior.
2. US1 splits, runs and merges; the console stays master's bytes.
3. US2 lets the caller pick the method.
4. US3 discloses the method, the count and the spread.
5. US4 teaches it on the page.
6. Phase 7 records the evidence and lands the branch.

### Rebase Note for the Three Parallel Lanes

N3 (JSON reports) and N4 (topology placement) run on other machines. The shared
files are the project version, the two `SOVERSION` lines, `README.md`, the quick
reference and option rows, `PAGE_LINE_CAP`, `example/CMakeLists.txt`, the example
run list and `test/CMakeLists.txt`. Rebase onto `master`, keep both features'
lines, retake the version and the two `SOVERSION` numbers, re-measure the page
budget, re-capture the byte-identity fixture, run the full gate set, and push
once with `--force-with-lease`. `printContextDisclosure`, `ContextDisclosure` and
the JSON schema stay untouched here: N4 and N3 own them.

---

## Notes

- [P] marks different files with no dependency on an open task.
- A one-pass `auto` run prints what master prints, so every report task carries a
byte rule beside its new shape.
- The pass fields join the JSON schema when that lane lands its own schema file
(FR-017, spec Scope).
- Stop at any checkpoint and validate the story independently.

---

## Requirement coverage

Every FR of `spec.md` and every SC of its Success Criteria maps to at least one
task.

| Requirement | Tasks |
| --- | --- |
| FR-001, FR-002 | T016, T017, T019, T026 |
| FR-003 | T018, T026 |
| FR-004 | T018, T023, T026 |
| FR-005 | T009, T010, T011, T012, T013, T014, T020, T026 |
| FR-006 | T008, T010, T015 |
| FR-007 | T020, T026 |
| FR-008 | T024, T026, T028 |
| FR-009, FR-010 | T022, T028 |
| FR-011 | T025, T028 |
| FR-012 | T024, T028 |
| FR-013 | T025, T028 |
| FR-014, FR-015 | T021, T027 |
| FR-016 | T021, T024, T027, T028 |
| FR-017 | T039, T040, T043, T045, T047, T048, T049 |
| FR-018 | T040, T041, T048, T049 |
| FR-019 | T039, T047, T050 |
| FR-020 | T043, T049 |
| FR-021 | T042, T044, T049, T050 |
| FR-022 | T046, T049 |
| FR-023 | T029, T034, T035, T036 |
| FR-024 | T029, T032, T033, T037 |
| FR-025 | T029, T031, T035, T037 |
| FR-026 | T030, T036 |
| FR-027 | T051, T052, T055 |
| FR-028 | T053, T057 |
| FR-029 | T054 |
| FR-030 | T056, T057 |
| FR-031 | T001, T002, T003, T065 |
| FR-032 | T058, T059 |
| FR-033 | T008, T016 to T025, T029 to T034, T039 to T046, T060 |
| FR-034 | T025 |
| FR-035 | T062 |
| FR-036 | T013, T064 |
| FR-037 | T060 |
| SC-01 | T022, T023, T032, T062 |
| SC-02 | T017 |
| SC-03 | T042 |
| SC-04 | T030 |
| SC-05 | T040, T041, T042 |
| SC-06 | T039, T045, T049, T050 |
| SC-07 | T058, T059, T066 |
| SC-08 | T053, T055, T056, T057 |
| SC-09 | T024, T061 |
| SC-10 | T025, T060 |
| Scaffolding, shared registration and gate runs, no single FR | T004, T005, T006, T007, T038, T063 |
