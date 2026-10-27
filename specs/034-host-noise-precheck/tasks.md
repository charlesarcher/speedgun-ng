# Tasks: Host Noise Pre-Check

**Input**: Design documents from `/specs/034-host-noise-precheck/`

**Prerequisites**: [plan.md](./plan.md) (required), [spec.md](./spec.md)
(required for user stories), [research.md](./research.md),
[data-model.md](./data-model.md), [contracts/](./contracts/),
[quickstart.md](./quickstart.md)

**Tests**: TDD mode (plan.md, Execution mode): the test task of each
wave stands written and observed failing before the code task that
turns it green.

**Organization**: tasks stand grouped by user story so each story can
stand implemented and tested independently.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: can run in parallel (different files, no dependencies)
- **[Story]**: the user story this task serves (`US1` to `US5`)
- Every description names the exact file path and the requirements it
  closes

---

## Phase 1: Setup (the surface, the version and the grammar)

**Purpose**: the option grammar, the public field and the records
every story stands on.

- [X] T001 [P] Write the failing grammar tests in
  `test/source/harness_jitter_cli_test.cpp`: `precheck=serial` stands
  accepted from the command line and from an option file; a
  misspelled serial word, a mixed token, `precheck=0`, and a value at
  or above 2³² each print one line and exit 2; the option-value
  uniqueness test stands over the whole table (FR-001, FR-002,
  FR-003, AC shape)
- [X] T002 Add the public field `bool precheckSerial = false;`
  appended after `strict` on `JitterProbeOptions` in
  `include/speedgun-ng/benchmark.hpp:1937` with its doxygen contract
  (`\pre` none, `\post` the pre-check runs one measured cpu per
  window when set); the commit moves `CMakeLists.txt:15` to `0.21.0`
  and `CMakeLists.txt:298` to `SOVERSION 13`, and its body records
  the ABI check with the archive `SOVERSION` staying `4` (FR-004,
  FR-005, R-10) (depends on T001)
- [X] T003 Add the `precheck=serial` parse at the
  `kJitterProbeOptionValue` case of `source/harness/options.cpp`
  (`options.cpp:1763`): the token after `precheck=` stands either the
  word `serial` or the standing number, the field carries the result,
  and a token that is neither stands the standing one-line status-2
  error (FR-003, R-09) (depends on T001, T002; the tests of T001 turn
  green)
- [X] T004 [P] Update the grammar row and the error table in
  `specs/027-jitter-probe/contracts/cli-options.md`: the grammar row
  reads `precheck=<ms>|serial`, the error table gains the three rows
  of [contracts/cli-options.md](./contracts/cli-options.md), and the
  record shape, the `idle` preset and the overlap rule rows change
  none; the edit rides the commit of T003 (FR-002, R-14) (depends on
  T003)

**Checkpoint**: the grammar stands green; the surface carries the
feature with no new option row and no lane move.

---

## Phase 2: Foundational (the seam, the measured set and the serial shape)

**Purpose**: the blocking prerequisites every story stands on. No
story work begins before this phase closes.

- [ ] T005 [P] Write the failing seam tests in a new
  `test/source/harness_precheck_test.cpp`, registered in
  `test/CMakeLists.txt`: `installTestProcRoot()` installs a temporary
  directory as the process-global root, `procRoot()` answers it, and
  the default stands restored at scope exit; a `/proc` path read
  under the installed root resolves inside it (FR-019, FR-020, R-02)
- [ ] T006 Implement the file-root seam in
  `source/harness/detail/proc_root.hpp`: `procRoot()` answers the
  process-global root (`/` by default) and
  `installTestProcRoot(const std::filesystem::path&)` installs a root
  for the test's life, the contract comments of the standing seams
  (R-02) (depends on T005; the tests turn green)
- [ ] T007 [P] Write the failing measured-set tests in
  `test/source/harness_precheck_test.cpp`: the set carries the
  planned cpus from the execution-context plans and the candidates
  from the `idle` preset resolution, computed once before the window
  opens; an empty candidate set stands disclosed and the run
  continues (FR-010, R-01, R-10)
- [ ] T008 Extend the measured set in
  `source/harness/jitter_probe.cpp`: the pre-check phase measures the
  planned cpus and the candidates, the candidates taken by the
  standing `idle` resolution reused unchanged; the tests of T007
  assert the parallel window covers every measured cpu at once and
  the standing 200 ms default stands (FR-008, FR-010, R-01) (depends
  on T006, T007; the tests turn green)
- [ ] T009 [P] Write the failing serial and mode tests in
  `test/source/harness_precheck_test.cpp`: `precheck=serial` runs one
  window per measured cpu, ascending cpu order, the planned cpus
  before the candidates, every window the same span, the window
  closed by the scripted reader of `installTestReader` with no wait
  and no clock read; the mode matrix stands: `mode=precheck` runs the
  pre-check before the first benchmark, `mode=both` runs the
  pre-check and then the alongside phase, `mode=alongside` alone runs
  no pre-check, and no option runs none (FR-007, FR-009, FR-011,
  R-06, R-8 of research.md)
- [ ] T010 Implement the serial sequencer in
  `source/harness/jitter_probe.cpp`: the measurement pass of every
  item stands first, then one `ContextBarrier` round per window, the
  item whose turn it is spins for the span, the others wait; the
  parallel shape stands unchanged (R-06) (depends on T008, T009; the
  tests turn green)

**Checkpoint**: foundation ready. `ctest -R harness_precheck` passes
under `taskset -c 0,1`; the seam, the measured set and both window
shapes stand testable independently of the rules.

---

## Phase 3: User Story 1 - CI refuses a noisy host before it records a run (Priority: P1) 🎯 MVP

**Goal**: in strict mode a noisy planned cpu stops the run with one
line and status 2 before any benchmark starts.

**Independent Test**: with fake gap figures above the threshold on a
planned cpu, a run with `mode=precheck,strict` exits 2 after one
line, and no benchmark row prints ([spec.md](./spec.md) US1).

### Tests for User Story 1 (write first, observe failure)

- [ ] T011 [P] [US1] Write the failing refusal tests in
  `test/source/harness_precheck_test.cpp`: the pre-check completes
  its full window (every window in serial mode), one refusal line
  names the first noisy planned cpu in cpu order with the gap rate
  and the max gap, the exit status is 2, the pre-check block stands
  unprinted, and no benchmark runs; a quiet planned set continues
  with status 0; a noisy probe candidate never refuses (FR-014,
  FR-015, the clarify answers of 2026-10-27)

### Implementation for User Story 1

- [ ] T012 [US1] Implement the noisy rule and the strict refusal in
  `source/harness/jitter_probe.cpp`: a planned cpu stands noisy when
  its window recorded at least one above-threshold gap (`maxNs` and
  `gapsPerSecond` stand above the threshold together, R-05); in
  strict mode the run stops after the full window with one line and
  status 2 before the first benchmark; the candidates stand exempt
  (R-05) (depends on T010, T011; the tests turn green)

**Checkpoint**: the strict case stands testable end to end; the
non-strict path stands untouched and green.

---

## Phase 4: User Story 2 - The pre-check warns and names the pinning candidates (Priority: P1)

**Goal**: each noisy planned cpu gets one warning line, the pinning
line ranks the planned cpus, and the run continues.

**Independent Test**: with fake figures one planned cpu above the
threshold and one below, the run prints one warning line, then the
pinning line, then the benchmark tables, and exits 0
([spec.md](./spec.md) US2).

### Tests for User Story 2 (write first, observe failure)

- [ ] T013 [P] [US2] Write the failing warning and pinning tests in
  `test/source/harness_precheck_test.cpp`: one warning line per
  noisy planned cpu names the cpu and the two figures; the run
  continues with status 0; the pinning line stands after the warnings
  and names every planned cpu with its gap rate, lowest first, tied
  cpus in cpu order; no warning line prints when every planned cpu
  stands quiet (FR-012, FR-013, SC-002, the clarify answers)

### Implementation for User Story 2

- [ ] T014 [US2] Implement `printPrecheckReport` in
  `source/harness/report.cpp`: the block prints before the benchmark
  tables, one block per measured cpu with the sixteen field names the
  console probe block uses; the warning lines stand after the blocks
  and the pinning line after the warnings (R-07) (depends on T012,
  T013; the tests turn green)

**Checkpoint**: stories 1 and 2 stand independently functional: the
strict case refuses, the non-strict case warns, ranks and continues.

---

## Phase 5: User Story 3 - The pre-check attributes the noise without root (Priority: P2)

**Goal**: the attribution deltas and the host settings stand
disclosed beside the gap figures, each naming its source, and
`not available` stands where the host does not state it.

**Independent Test**: with fake `/proc` files installed through the
seam, each disclosed delta matches the fake before-and-after
difference and names its source; a deleted source reads the
`not available` wording and the exit status stays 0
([spec.md](./spec.md) US3).

### Tests for User Story 3 (write first, observe failure)

- [ ] T015 [P] [US3] Write the failing attribution tests in
  `test/source/harness_precheck_test.cpp` over fake files through the
  seam: the interrupt delta stands the sum of the cpu column over
  every row of `/proc/interrupts`; one delta per softirq kind the
  file states, the kind named as the file states it; the
  `/proc/schedstat` mapping (the run-queue delay figure names the
  switch figure, the timeslices figure the schedule count); the host
  settings print once per run (`isolcpus`, `nohz_full`, `rcu_nocbs`
  from `/proc/cmdline`, the governor per cpu, the IRQ affinity list
  form with the mask fallback); every missing, unreadable and
  unparsable shape reads `not available` for that source alone and
  never fails the run (FR-016, FR-017, FR-018, SC-003, R-03, R-04,
  R-4 to R-6 of research.md)

### Implementation for User Story 3

- [ ] T016 [US3] Implement the attribution reader and the
  host-settings reader in `source/harness/jitter_probe.cpp`: the
  snapshots stand read before the window opens and after it closes,
  never inside the loop; each figure carries its source name; the
  re-confirmation of the schedstat mapping against the roadmap 10.1
  row stands recorded, a mismatch goes to the pull request as a
  question (R-03, R-04) (depends on T006, T015; the tests turn green)
- [ ] T017 [US3] Extend the console block in
  `source/harness/report.cpp` with the attribution lines beside the
  gap figures and the host-settings disclosure once per run, the
  `not available` wording of
  [contracts/console-report.md](./contracts/console-report.md)
  (R-07) (depends on T014, T016)

**Checkpoint**: the disclosure stands complete and root-free; the
gaps, the deltas and the host settings stand one contract.

---

## Phase 6: User Story 4 - The report carries the pre-check (Priority: P2)

**Goal**: the JSON document carries the `precheck` record at schema
`1.6`, `loadReport` reads it back, and a pre-check-free run matches
master byte for byte apart from `schemaVersion`.

**Independent Test**: the JSON of a pre-check run round-trips through
`loadReport`; the byte comparison of a pre-check-free run against
master differs in the `schemaVersion` value alone
([spec.md](./spec.md) US4).

### Tests for User Story 4 (write first, observe failure)

- [ ] T018 [P] [US4] Write the failing JSON tests in
  `test/source/harness_precheck_test.cpp` and the loader tests in
  `test/source/harness_json_load_test.cpp`: the `precheck` object
  stands after `jitter` and before `benchmarks` with the keys of
  [contracts/report-json.md](./contracts/report-json.md); the schema
  carries the moved top-level `x-order` and minor `1.6`; the round
  trip through `loadReport` reads equal values; a run without the
  pre-check writes no key, and its document matches master's apart
  from the `schemaVersion` value (FR-006, FR-024, SC-004, R-08, R-9
  of research.md)

### Implementation for User Story 4

- [ ] T019 [US4] Extend `docs/schema/report-v1.json`: the `precheck`
  object's keys with their kinds, the `not available` alternative on
  the optional figures, and the top-level `x-order` moved to carry
  `precheck` after `jitter`; the minor moves to `1.6` in the same
  commit as the writer (FR-006, R-08) (depends on T018)
- [ ] T020 [US4] Implement the writer in
  `source/harness/json_report.cpp` (the `precheck` object after
  `jitter`) and the reader in `source/harness/json_load.cpp`
  (`loadReport` reads the record back through the standing simdjson
  path); the compare major-version rule stands untouched (R-08)
  (depends on T017, T018, T019; the tests turn green)

**Checkpoint**: stories 1 to 4 stand independently functional; the
two surfaces carry one contract.

---

## Phase 7: User Story 5 - The docs teach the pre-check (Priority: P3)

**Goal**: the README section, the User-guide detail, the example and
its expected rows stand one fact with the code.

**Independent Test**: the example builds and runs, its `.expected`
file matches its printed rows, and the drift, snippet-compile, run
and report-shape gates pass ([spec.md](./spec.md) US5).

### Implementation for User Story 5

- [ ] T021 [P] [US5] Add the pre-check section to `README.md`, at
  most 25 lines, one purpose sentence, one marked snippet from the
  example region, one result sentence, with the quick-reference rows
  for `JitterProbeOptions::precheckSerial` and the `precheck=serial`
  form (FR-026, IV.1, D-9, R-12)
- [ ] T022 [P] [US5] Add the option and field detail to
  `docs/pages/harness.md`: the `precheck=serial` grammar row, the
  field reference of the block, the attribution sources and their
  `not available` rule (FR-026, R-12)
- [ ] T023 [US5] Extend `example/docs/jitter_probe.cpp` with the
  pre-check section the page quotes and re-record
  `example/docs/jitter_probe.expected` from the build that carries
  the feature; the rows the page's result sentence names print
  (FR-026, R-12) (depends on T014, T017, T020)
- [ ] T024 [US5] Run the docs gates and clear every finding:
  `example_drift`, `example_drift_fixtures`, `snippet_compile`,
  `example_run`, `report_shape`, `report_shape_fixtures` (FR-026,
  SC-008) (depends on T021, T022, T023)

**Checkpoint**: all five stories stand functional; the reader path
and the code stand one fact.

---

## Phase 8: Polish and Cross-Cutting Concerns

**Purpose**: the hard gates and the records that outlive the branch.

- [ ] T025 [P] Run the coverage gate over the feature paths: 100%
  lines and 100% branches, no `LCOV_EXCL` marker, no new
  `--ignore-errors` entry, no code shaped for coverage (FR-027,
  SC-005)
- [ ] T026 [P] Run the sanitizer and thread-sanitizer builds
  (`ci-sanitize`, `ci-tsan`) and the naming, format, spell and
  JSON-library gates; every finding stands cleared at lint parity;
  the `sg::NoiseWarning` disclosure lines stand pinned unchanged
  against the standing tests (FR-025, FR-028, SC-008)
- [ ] T027 [P] Run the suite under `taskset -c 0,1` and record the
  result; every timing bound stands an order of magnitude inside the
  contamination signal it discriminates against, the margin recorded
  beside the bound (FR-021, FR-022, SC-006, R-11)
- [ ] T028 Run `cmake --preset=ci-ubuntu` and
  `cmake --build build` once and clear every clang-tidy and cppcheck
  finding (constitution IX, the per-feature release build) (depends
  on T012, T016, T020)
- [ ] T029 Run every scenario of [quickstart.md](./quickstart.md) and
  record the commands, the exit codes and the figures under
  `specs/034-host-noise-precheck/results/` (X.4, SC-001 to SC-007)
  (depends on T024, T025, T026, T027, T028)
- [ ] T030 Verify every new commit carries the commit template footer
  (`Refs: specs/034-host-noise-precheck`, then `Approved-by:`), passes
  `prose-lint`, and names no person, no host and no acceptance
  wording in a file (Pull Request Quality, XI, the file-hygiene rule
  of specs/032 FR-024) (depends on T029)

---

## Phase 9: Convergence

**Purpose**: the closing evidence the lane lands with.

- [ ] T031 Read the CI run of the pushed head end to end: every job
  green (`lint`, `test`, `test-rocky`, `sanitize`, `tsan`,
  `static-analysis`, `coverage`, `dbc-gate`, `prose-lint`,
  `shared-audit`, `downstream-consumer`), the coverage figures read
  100.0% lines and 100.0% branches, and the run reference stands
  recorded beside the acceptance evidence (SC-001 to SC-008) (depends
  on T030)
- [ ] T032 Re-check the constitution table of [plan.md](./plan.md)
  against the landed tree: every gate stands as the table states it,
  the Complexity Tracking table stands empty, and the record stands
  in the pull-request body (constitution Governance) (depends on
  T031)

---

## Dependencies & Execution Order

### Phase Dependencies

- Phase 1 stands first: the grammar and the public field carry every
  story. T001 stands the first red run; T002 to T004 turn it green.
- Phase 2 stands on Phase 1: the seam (T005, T006) blocks the
  attribution tests (T015), the measured set (T008) blocks the rules
  (T012), and the serial sequencer (T010) closes the foundation.
- Phases 3 and 4 stand on the foundation; Phase 3 before Phase 4 (the
  refusal and the warning share the rule of R-05, and the block of
  T014 prints what T012 computes).
- Phase 5 stands on the seam and the block; Phase 6 stands on the
  block and the readers; Phase 7 stands on the stories' surfaces.
- Phases 8 and 9 stand last; T028 through T032 run once the stories
  stand green.

### User Story Dependencies

- **US1** (T011, T012): the foundation of Phase 2 alone; no story
  stands before it.
- **US2** (T013, T014): the rule of US1 (T012) computes the state the
  block prints; the stories share the threshold condition by design
  (R-05).
- **US3** (T015 to T017): the seam of T006 and the block of T014;
  the attribution stands printed beside the gap figures.
- **US4** (T018 to T020): the readers of US3 fill the record's
  attribution fields; the schema and the writer stand one commit.
- **US5** (T021 to T024): the shipped surfaces of US2 to US4; the
  docs quote the block and the record the code prints and writes.

### Within Each User Story

- The test task stands written and observed failing before the
  implementation task turns it green (TDD, plan.md).
- The rule before the print (T012 before T014); the reader before the
  line that names its figure (T016 before T017); the schema before
  the writer (T019 before T020).

### Parallel Opportunities

- T001, T005, T007, T009 stand four independent red runs (different
  files); T004, T011, T013, T015, T018, T021, T022, T025, T026, T027
  carry the [P] marker across their phases.
- The test executable of T005 registers once; the later test tasks
  append to the same file in task order, so the [P] marks on tests
  inside one file name the write order; the tasks append in task
  order.

---

## Parallel Example: Phase 2

```text
# Four independent red runs (different files, no shared edit):
Task: "T005 the seam tests in test/source/harness_precheck_test.cpp"
Task: "T007 the measured-set tests in test/source/harness_precheck_test.cpp"
Task: "T009 the serial tests in test/source/harness_precheck_test.cpp"
# (T005 registers the executable; T007 and T009 append in task order)

# Then the implementations, each gated by its own red run:
Task: "T006 the seam header"
Task: "T008 the measured set"
Task: "T010 the serial sequencer"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Phase 1: the grammar and the public field (T001 to T004).
2. Phase 2: the foundation (T005 to T010).
3. Phase 3: the strict refusal (T011, T012).
4. **Stop and validate**: the strict case of quickstart.md stands
   green end to end; CI refuses a noisy host before it records a run.

### Incremental Delivery

1. Foundation → the strict refusal (MVP).
2. US2: the warnings and the pinning line → validate independently.
3. US3: the attribution and the host settings → validate.
4. US4: the JSON record and the byte identity → validate.
5. US5: the docs and the gates → the full CI matrix (Phase 9).

### Parallel Team Strategy

With multiple developers: the foundation pairs one seam owner and one
window owner; after the checkpoint, one developer takes the rules
(US1, US2), one the disclosure and the record (US3, US4), one the
docs (US5); the phases merge in story order.

---

## Notes

- Every task closes the requirements its description names; the
  commit that closes a task carries the footer of T030.
- No task introduces a sleep, a clock read, a fixed cpu number, an
  `LCOV_EXCL` marker, a lambda inside an `SG_*` macro, or a second
  JSON library (spec Constraints, FR-027, FR-028).
- A task closed only against the unoptimized build stays open: T028
  stands the release build every story needs before its tasks stand
  done (constitution IX).
