# Tasks: Osnoise and Timerlat Tracer Integration

**Input**: Design documents from `/specs/036-osnoise-timerlat-tracers/`

**Prerequisites**: [plan.md](./plan.md) (required), [spec.md](./spec.md)
(required for user stories), [research.md](./research.md),
[data-model.md](./data-model.md), [contracts/](./contracts/),
[quickstart.md](./quickstart.md)

**Tests**: TDD mode (plan.md, Execution mode): the test task of each
wave stands written and observed failing before the code task that
turns it green.

**Organization**: tasks stand grouped by user story behind the module
landing order plan.md fixes (M-A to M-G): the public surface stands
first, the three independent modules land in one parallel block, the
integration follows, the docs and the evidence close. Every task names
its binary verify: a command with an exit status, a test name, a byte
fold, a schema shape, or a grep with a match count.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: can run in parallel (different files, or one file behind
  disjoint scenario blocks)
- **[Story]**: the user story this task serves (`US1` to `US5`)
- Every description names the exact file path, the requirements it
  closes, and the verify command

---

## Phase 1: Setup and Foundational - the public surface and the version move (module M-A)

**Purpose**: the types every story stands on, and the version record
the lane stands on. No other module builds before this phase closes.

- [X] T001 [P] Register the test executable `harness_noise_test` in
  `test/CMakeLists.txt` beside the standing harness tests
  (`add_executable` with the harness link, `add_test`), then write the
  failing public-surface tests in `test/source/harness_noise_test.cpp`:
  `JitterProbeOptions` default-constructs with `TracerEngine::NONE` and
  an empty `tracerCpus`; a default `PrecheckRecord` leaves
  `PrecheckRecord::tracer` empty; the source-order binding rows settle
  `tracerEngine` and `tracerCpus` from the command line first, then the
  option file, then the suite call, the order spec 027 fixes
  (FR-005, D-6, R-06, R-10).
  Verify: `ctest --test-dir build -R harness_noise_test --output-on-failure` exits non-zero with the new checks failing (red observed)
- [X] T002 Add the public surface in `include/speedgun-ng/benchmark.hpp`:
  the closed enum `TracerEngine` (`NONE`, `OSNOISE`, `TIMERLAT`);
  `JitterProbeOptions::tracerEngine` and `JitterProbeOptions::tracerCpus`
  appended after `precheckSerial`; `PrecheckTracerRecord` and
  `PrecheckTracerCpuRecord` beside the pre-check records; and
  `PrecheckRecord::tracer` as `std::optional<PrecheckTracerRecord>`
  appended after `skippedGroups`; each new type and member carries its
  doxygen contract, the empty `tracerCpus` naming the candidate set and
  a non-empty list naming cpus inside the process affinity mask and the
  online set (FR-005, D-6, R-06, R-10) (depends on T001; the
  struct-default rows of T001 turn green, and the source-order binding
  rows stay red until T007 stands the parser and the option row up).
  Verify: `ctest --test-dir build -R harness_noise_test --output-on-failure` exits 0
- [X] T003 Move the version in `CMakeLists.txt`: the project `VERSION`
  at `CMakeLists.txt:19` stands `0.23.0`; the harness `SOVERSION` at
  `CMakeLists.txt:302` stands `14` with the comment naming the ABI
  growth (`TracerEngine`, the two `JitterProbeOptions` fields,
  `PrecheckRecord::tracer`); the archive `SOVERSION` at
  `CMakeLists.txt:84` stays `4`; the commit message records the ABI
  check beside the numbers, the way the standing `SOVERSION` comments
  record every lane that moved one (FR-004, D-6) (depends on T002).
  Verify: `ctest --test-dir build -R harness_cli_table_test --output-on-failure` exits 0 (the version expectation reads the `project(` block) and `grep -n "SOVERSION 14" CMakeLists.txt` names one line

**Checkpoint**: the surface stands green: the types carry the feature,
and the version names the ABI move.

---

## Phase 2: User Story 3 - the value grammar and the option row (Priority: P2) (module M-B, wave 1)

**Goal**: a valid `--os-noise` value settles into the option fields;
every invalid value of the error table refuses one line on standard
error with exit status 2 before anything runs.

**Independent Test**: with each invalid value of
[contracts/cli-options.md](./contracts/cli-options.md), the run prints
one line naming the fault and exits 2 with no benchmark output.

### Tests for User Story 3 (write first, observe failure)

- [X] T004 [P] [US3] Write the failing grammar tests in
  `test/source/harness_noise_test.cpp`: `--os-noise=osnoise` and
  `--os-noise=timerlat` settle `tracerEngine`; the optional `cpus=` key
  settles `tracerCpus` through the standing cpu-list reader
  (`parseProbeCpuToken`, `source/harness/jitter_probe.cpp`), the form
  the `--topology` listing prints; a second `cpus` key in one value
  stands the repeated-key refusal; `--os-noise` with no argument takes
  the standing missing-argument path (FR-002, D-4, R-06).
  Verify: `ctest --test-dir build -R harness_noise_test --output-on-failure` exits non-zero (red observed)
- [X] T005 [P] [US3] Write the failing refusal tests in
  `test/source/harness_noise_test.cpp`: an engine word outside the two;
  a `cpus` value that is no cpu index or range, a cpu outside the
  process affinity mask, a cpu the online set does not hold, and a cpu
  standing twice in one list; a repeated key; a bad value from an
  option file line naming the path, the line and the token; each case
  prints one line and exits 2 with no benchmark output (FR-003, SC-003).
  Verify: `ctest --test-dir build -R harness_noise_test --output-on-failure` exits non-zero (red observed)
- [X] T006 [P] [US3] Write the failing option-table row tests in
  `test/source/harness_cli_table_test.cpp`: the
  `noNameAndNoValueRepeat` check and the
  `theValueBlocksStayUniqueAndDisjoint` check carry the `os-noise` row;
  no standing name moves and no standing value moves; the row takes no
  short letter and no environment name (FR-001, D-4).
  Verify: `ctest --test-dir build -R harness_cli_table_test --output-on-failure` exits non-zero (red observed)

### Implementation for User Story 3

- [X] T007 [US3] Add the option row and the value parser:
  `kOsNoiseOptionValue` stands `297` in
  `source/harness/detail/options.hpp` with the block comment amended to
  name `297` taken and `298` to `319` free; the row joins
  `source/harness/options.cpp` after the `--jitter-probe` row;
  `source/harness/detail/noise_tracer.hpp` declares `parseOsNoiseValue`
  with its doxygen contract beside `parseJitterProbeValue`, and
  `source/harness/noise_tracer.cpp` implements it: the engine word,
  then the one optional `cpus` key whose value `parseProbeCpuToken`
  reads whole, a second key standing the standing refusal shape
  (FR-001, FR-002, FR-003, D-4, R-06) (depends on T004, T005, T006;
  the tests of T004 to T006 and the source-order binding rows of T001
  turn green).
  Verify: `ctest --test-dir build -R "harness_noise_test|harness_cli_table_test" --output-on-failure` exits 0

**Checkpoint**: the grammar stands green: every refusal prints one line
at status 2, and the row sits unique in the table.

---

## Phase 3: User Stories 1 and 2 - the tracer engine session (Priority: P1) (module M-C, wave 2)

**Goal**: over a fake tracefs tree the session sets the engine, the
mask and `tracing_on`, reads the five noise-by-source figures per
tracer cpu, and restores the standing values; every host state the
setup cannot write answers not available.

**Independent Test**: with an injected tracefs root, the lifecycle
tests and the figure tests stand the engine up, read the figures and
restore; with a denied tree every path answers the empty session.

### Tests for User Stories 1 and 2 (write first, observe failure)

- [X] T008 [P] [US1] Write the failing engine-lifecycle tests in
  `test/source/harness_noise_test.cpp` over fake tracefs trees
  installed through `installTestProcRoot`
  (`source/harness/detail/proc_root.hpp`): `noiseTracerSetup` reads the
  standing `current_tracer` word and the standing `tracing_cpumask`
  text, writes the engine word, writes the tracer mask rendered in the
  word width the standing mask carried, and writes `tracing_on` at `1`;
  `noiseTracerRestore` writes `tracing_on` at `0` and returns the
  standing word and mask (FR-007, FR-017, R-02, R-04, R-05).
  Verify: `ctest --test-dir build -R harness_noise_test --output-on-failure` exits non-zero (red observed)
- [X] T009 [P] [US1] Write the failing figure-read tests in
  `test/source/harness_noise_test.cpp` over the same fake trees: one
  entry per tracer cpu in session order carrying the five figures
  `nmi`, `irq`, `softirq`, `thread`, `hardware` read from the per-cpu
  count files; a source the file states no figure for stands absent for
  that source alone; a cpu whose files state nothing at read time stands
  every figure absent; the read never fails the run (FR-010, FR-017,
  R-07).
  Verify: `ctest --test-dir build -R harness_noise_test --output-on-failure` exits non-zero (red observed)
- [X] T010 [P] [US2] Write the failing not-available tests in
  `test/source/harness_noise_test.cpp`: no tracefs mount at the
  standing path, an unwritable `current_tracer`, a failed engine write,
  and denied setup writes each answer the empty session, and no
  exception reaches the run (FR-016, FR-017, D-5, R-02).
  Verify: `ctest --test-dir build -R harness_noise_test --output-on-failure` exits non-zero (red observed)

### Implementation for User Stories 1 and 2

- [X] T011 [US1] Implement the session lifecycle in
  `source/harness/detail/noise_tracer.hpp` and
  `source/harness/noise_tracer.cpp`: the session type carrying
  `engine`, `cpus`, the standing `current_tracer` word and the standing
  `tracing_cpumask` text; `noiseTracerSetup` and `noiseTracerRestore`
  with their `\pre`/`\post` on the tracefs files they touch; any
  failed write answers the empty optional (FR-007, FR-016, D-5, R-02,
  R-04, R-05) (depends on T008, T010, and on T007, which creates the
  two files this task writes; the tests of T008 and T010 turn
  green).
  Verify: `ctest --test-dir build -R harness_noise_test --output-on-failure` exits 0
- [X] T012 [US1] Implement `noiseTracerFigures` in
  `source/harness/noise_tracer.cpp`: one entry per tracer cpu, the five
  counts as `std::optional<std::uint64_t>`, an absent figure standing
  `std::nullopt`, the read answering the session cpu order
  (FR-010, R-07) (depends on T009, T011; the tests of T009 turn green).
  Verify: `ctest --test-dir build -R harness_noise_test --output-on-failure` exits 0

**Checkpoint**: the engine stands green over the fake trees: the setup
writes, the figures read, the restore returns, and every not-available
path answers the empty session.

---

## Phase 4: User Stories 1 and 2 - the record surfaces (Priority: P1) (module M-E, wave 3)

**Goal**: the console prints the tracer lines beside the standing
block, the JSON document carries the optional tracer record at schema
`1.7`, and a run without a tracer stands byte-identical to master
apart from the `schemaVersion` value.

**Independent Test**: built against a hand-built record, the console
fold, the JSON round trip, the schema shape and the byte identity all
stand green with no engine and no integration wired.

### Tests for User Stories 1 and 2 (write first, observe failure)

- [X] T013 [P] [US1] Register the test executable
  `harness_noise_report_test` in `test/CMakeLists.txt` beside the
  standing harness tests, then write the failing console tests in
  `test/source/harness_noise_report_test.cpp` built against a
  hand-built record: the tracer lines stand after the host-settings
  lines and before the warning lines; the header line names the engine
  word and the cpus; one figure line stands per source per tracer cpu
  in the NMI, IRQ, softirq, thread, hardware order, cpus ascending; an
  absent figure prints the standing `not available` wording in its
  figure position; the not-available record prints the one disclosure
  line (FR-013, FR-010, FR-016, R-14).
  Verify: `ctest --test-dir build -R harness_noise_report_test --output-on-failure` exits non-zero (red observed)
- [X] T014 [P] [US1] Write the failing JSON round-trip tests in
  `test/source/harness_noise_report_test.cpp`: the writer emits the
  tracer object inside `precheck` after `skippedGroups` carrying
  `engine` and the `cpus` entries; `loadReport` reads it back with
  equal values; a document written before the key reads as the
  tracer-less record (FR-014, R-08, R-11).
  Verify: `ctest --test-dir build -R harness_noise_report_test --output-on-failure` exits non-zero (red observed)
- [X] T015 [P] [US1] Write the failing schema tests in
  `test/source/harness_noise_report_test.cpp`:
  `docs/schema/report-v1.json` carries the `tracer` key in the
  `precheck` object's `properties` and in its `x-order` after
  `skippedGroups` and stands outside its `required` list; the
  `precheck` `x-order` reads the six keys in
  [contracts/report-json.md](./contracts/report-json.md); the
  top-level `x-order` stands unchanged; `kSchemaVersion` reads `1.7`
  (FR-006, D-3, R-11).
  Verify: `ctest --test-dir build -R harness_noise_report_test --output-on-failure` exits non-zero (red observed)
- [X] T016 [P] [US2] Write the failing byte-identity tests in
  `test/source/harness_noise_report_test.cpp`: a record without the
  tracer member writes a document whose bytes fold equal to the fixture
  bytes, the fixture carrying master's key set with `schemaVersion` at
  `1.7`; a not-available record writes no tracer key and prints the one
  disclosure line; a tracer-free run prints no tracer line and every
  standing line folds byte-identical to the fixture (FR-012, FR-015,
  R-10).
  Verify: `ctest --test-dir build -R harness_noise_report_test --output-on-failure` exits non-zero (red observed)

### Implementation for User Stories 1 and 2

- [X] T017 [US1] Print the tracer lines in `printPrecheckReport`
  (`source/harness/report.cpp:986`) after the host-settings lines and
  before the warning lines: the header line naming the engine and the
  cpus, one figure line per source per tracer cpu in the standing
  source order with cpus ascending, the standing `not available`
  wording in an absent figure position, and the one disclosure line for
  the not-available case; every standing line stands byte-identical
  (FR-012, FR-013, FR-016, R-14) (depends on T013, T016; the console
  rows of T013 and T016 turn green).
  Verify: `ctest --test-dir build -R harness_noise_report_test --output-on-failure` exits 0
- [X] T018 [US1] Write the tracer object in
  `source/harness/json_report.cpp` after `skippedGroups` and stand
  `kSchemaVersion` at `1.7`; read it back in
  `source/harness/json_load.cpp` through the standing `optionalField`
  path, the way `hunt` reads (FR-014, FR-006, R-08, R-11) (depends on
  T014; the round-trip rows of T014 turn green).
  Verify: `ctest --test-dir build -R harness_noise_report_test --output-on-failure` exits 0
- [X] T019 [US1] Extend `docs/schema/report-v1.json`: the `tracer` key
  in the `precheck` object's `properties` and in its `x-order` after
  `skippedGroups`, outside its `required` list, the top-level
  `x-order` unchanged, the per-cpu entry listing `cpu` then the five
  figure keys in the order
  [contracts/report-json.md](./contracts/report-json.md) states
  (FR-006, D-3, R-11) (depends on T015, T018; the schema rows of T015
  turn green).
  Verify: `ctest --test-dir build -R "harness_noise_report_test|report_shape" --output-on-failure` exits 0, and the written fixture document carries the six-key `precheck` order

**Checkpoint**: the record surfaces stand green against a hand-built
record: the console folds, the document round-trips, the schema stands
closed, and the byte identity holds.

---

## Phase 5: User Stories 1, 2 and 3 - the pre-check integration (Priority: P1) (module M-D, wave 4)

**Goal**: the run stands the engine up before the pre-check window,
reads the figures after the measurement ends, restores, and builds the
record; the missing-window refusal refuses ahead of everything.

**Independent Test**: with injected seams, a run with the option prints
the record the figure tests pin; a run whose mode carries no pre-check
window refuses one line at exit 2.

### Tests for User Stories 1, 2 and 3 (write first, observe failure)

- [X] T020 [P] [US1] Write the failing window tests in
  `test/source/harness_noise_test.cpp` with the scripted
  `WindowReader` installed through `installTestReader`: the engine
  stands up before the probe items are dispatched and the figures stand
  read after `JitterProbe::start()` returns; the serial mode runs the
  tracer across every window with no engine reset write between them;
  `mode=both` keeps the tracer's stop behind the latch with the
  planned cpus free of the default tracer load (FR-009, R-05, R-06
  of research.md).
  Verify: `ctest --test-dir build -R harness_noise_test --output-on-failure` exits non-zero (red observed)
- [X] T021 [P] [US1] Write the failing tracer-cpu tests in
  `test/source/harness_noise_test.cpp`: a `cpus=` list runs as named, a
  planned cpu included, with no overlap refusal; an absent key names
  the candidate set of the injected topology, one cpu per L3 group the
  plans touch (FR-008, R-09).
  Verify: `ctest --test-dir build -R harness_noise_test --output-on-failure` exits non-zero (red observed)
- [X] T022 [US2] Write the failing whole-run tests in
  `test/source/harness_noise_test.cpp`: a not-available host keeps the
  user-space pre-check whole, the standing block stands byte-identical,
  the tracer lines stand the disclosure, and the run exits 0 (FR-016,
  SC-004, R-10).
  Verify: `ctest --test-dir build -R harness_noise_test --output-on-failure` exits non-zero (red observed)
- [X] T023 [US3] Write the failing missing-window refusal tests in
  `test/source/harness_noise_test.cpp`: a settled `--os-noise` value
  beside a batch that carries no `--jitter-probe` option, and beside a
  batch whose jitter-probe mode stands `ALONGSIDE`, each prints one
  line naming the missing pre-check window and exits 2 before anything
  runs; the compare mode refuses the same way (FR-011, R-07).
  Verify: `ctest --test-dir build -R harness_noise_test --output-on-failure` exits non-zero (red observed)

### Implementation for User Stories 1, 2 and 3

- [X] T024 [US1] Wire the lifecycle in `source/harness/cli.cpp`, the
  `batchProbed` block: resolve the tracer cpus beside the measured set
  (the `cpus=` list as named, else the candidates), run
  `noiseTracerSetup` before the items dispatch, run
  `noiseTracerFigures` and `noiseTracerRestore` after `start()`
  returns, and build `PrecheckRecord::tracer` from the figures; the
  not-available path leaves the member empty and the run whole at exit
  0 (FR-008, FR-009, FR-016, R-05, R-09, R-10) (depends on T007, T011,
  T012, T020, T021, T022; the tests of T020 to T022 turn green).
  Verify: `ctest --test-dir build -R harness_noise_test --output-on-failure` exits 0
- [X] T025 [US3] Wire the missing-window refusal in
  `source/harness/cli.cpp` beside the standing combination check the
  hunt family calls after the gather
  (`validateHuntCombination`, `source/harness/hunt_cli.cpp:660`): the
  check reads the batch's probe options alone and prints the one line
  at exit 2 (FR-011, R-07) (depends on T007, T023; the tests of T023
  turn green).
  Verify: `ctest --test-dir build -R harness_noise_test --output-on-failure` exits 0, and each refusal case stands one line on standard error at exit 2 with no benchmark output ahead of it

**Checkpoint**: the integration stands green: the window, the cpus,
the whole-run disclosure and the refusal set all pass under the
injected seams.

---

## Phase 6: User Story 5 - the docs teach the tracer (Priority: P3) (module M-F, wave 5)

**Goal**: the README, the guide page and the example teach the option,
the disclosure rule and the perturbation note, and the docs gates stand
green over them.

**Independent Test**: `example_drift`, `snippet_compile`,
`example_run` and `report_shape` pass, and the prose gate reads no
finding over the branch range.

- [X] T026 [P] [US5] Add the README section of at most 25 lines in
  `README.md` teaching the option row, the disclosure rule and the
  perturbation note for a `cpus=` list naming a planned cpu, with the
  quick-reference rows, beside the pre-check sections spec 034 landed
  (FR-021, IV.1).
  Verify: `ctest --test-dir build -R example_drift --output-on-failure` exits 0 (the quick-reference rows stand inside the drift gate), and a line count over the section (`sed -n` over the section range piped to `grep -c`) reads at most 25
- [X] T027 [P] [US5] Add the option and field detail beside the
  pre-check sections in `docs/pages/harness.md` (FR-022).
  Verify: `ctest --test-dir build -R example_drift --output-on-failure` exits 0, and the prose gate over the branch range exits 0
- [X] T028 [US5] Extend the example region in
  `example/docs/jitter_probe.cpp` with the tracer section, re-record
  `example/docs/jitter_probe.expected`, register the section in
  `tools/docs/example_drift.py` (`SECTIONS` and `SECTION_OWNERS`
  rows), and add the tracer fold rows to
  `tools/docs/report_shape.py`, the host-owned words and the `not
  available` alternative of the tracer lines included (FR-023, FR-024,
  R-14 of the plan risks)
  (depends on T017, T018; the binary
  prints the lines the example quotes).
  Verify: `ctest --test-dir build -R "example_drift|snippet_compile|example_run|report_shape" --output-on-failure` exits 0

**Checkpoint**: the docs stand green: the example drifts from nothing,
the snippet compiles, the run prints, and the shape gate folds.

---

## Phase 7: User Story 4 - the evidence records and the standing contracts (Priority: P2) (module M-G, wave 6)

**Goal**: the acceptance runs on thrawn stand recorded per CCD, the
quickstart links every record, and the standing contracts stand amended
in the commit that lands the code.

**Independent Test**: the results directory carries one record per
engine, each record naming the commands, the exit codes, the engine and
the per-CCD tables, and the quickstart links each one.

- [X] T029 [US4] Write the evidence plan in
  `specs/036-osnoise-timerlat-tracers/results/plan.md`: the command set
  per engine, the record format, the CCD names read from the
  `--topology` listing the same host prints, and the re-confirmation
  step for the per-cpu count-file name and line format (FR-020, D-7,
  R-03, R-14 of research.md).
  Verify: `grep -c "benchmark_example --filter=bmTouch" specs/036-osnoise-timerlat-tracers/results/plan.md` counts at least the two engine commands the quickstart names
- [X] T030 [US4] Record the osnoise acceptance run on thrawn in
  `specs/036-osnoise-timerlat-tracers/results/osnoise.md` (needs the
  lane host thrawn; the FR-020 script records the run): the commands,
  the exit codes, the engine and the per-CCD tables with the CCD named
  beside each figure, one probe cpu per L3 group; re-confirm the
  per-cpu count-file spelling and the line format against the host tree
  during the run and record the confirmed facts beside the figures, a
  mismatch going to the pull request as a question (FR-020, R-03)
  (depends on T024, T029).
  Verify: the recorded command set replays with exit code 0 on the lane host, and the record names the exit codes and the CCD beside every figure
- [X] T031 [US4] Record the timerlat acceptance run on thrawn in
  `specs/036-osnoise-timerlat-tracers/results/timerlat.md` in the shape
  of T030, with the same re-confirmation note (FR-020, D-7, R-03)
  (depends on T024, T029).
  Verify: the recorded command set replays with exit code 0 on the lane host
- [X] T032 [US4] Link every record from the acceptance section of
  `specs/036-osnoise-timerlat-tracers/quickstart.md` (FR-020, SC-006)
  (depends on T030, T031).
  Verify: `grep -c "results/osnoise.md\|results/timerlat.md" specs/036-osnoise-timerlat-tracers/quickstart.md` counts one link per record
- [X] T033 [US4] Amend the standing contracts in the commit that lands
  the code (FR-024): the `--os-noise` row joins
  `specs/015-benchmark-harness-core/contracts/cli.md`; the tracer lines
  join
  `specs/034-host-noise-precheck/contracts/console-report.md`; the
  tracer keys join
  `specs/034-host-noise-precheck/contracts/report-json.md`; the
  tracer fold rows join
  `specs/034-host-noise-precheck/contracts/report-shape.md`; no other
  standing rule moves (depends on T007, T017, T018, T019, T028).
  Verify: `grep -c "os-noise" specs/015-benchmark-harness-core/contracts/cli.md` reads at least one row, the spec 034 contracts name the tracer lines, the tracer keys and the tracer fold rows, and the prose gate over the branch range exits 0

**Checkpoint**: the evidence stands recorded, the quickstart links
every record, and the standing contracts carry the deltas.

---

## Phase 8: Polish and Cross-Cutting Concerns

**Purpose**: the hard gates of Principle VIII at the feature head.

- [X] T034 [P] Run the format and spell gates; every finding stands
  fixed (constitution V).
  Verify: `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake` exits 0 and `cmake -P cmake/spell.cmake` exits 0
- [X] T035 [P] Run the sanitizer and tsan suites: configure
  `ci-sanitize` and `ci-tsan`, build, and run each suite; no finding
  stands (constitution VIII).
  Verify: both `ctest` runs exit 0
- [X] T036 [P] Run the analyzer build once with clang-tidy and cppcheck
  wired in; `readability-identifier-naming` reports zero findings on
  the new names (`TracerEngine`, `parseOsNoiseValue`,
  `noiseTracerSetup`, `noiseTracerFigures`, `noiseTracerRestore`,
  `kOsNoiseOptionValue`), and every other finding stands fixed
  (constitution V, IX).
  Verify: `cmake --preset=ci-ubuntu` exits 0 and `cmake --build build` exits 0
- [X] T037 Run the coverage contract: configure the coverage preset,
  build, and run the coverage suite; the gate reads 100.0% lines and
  100.0% branches, no `LCOV_EXCL` marker joins, and no new
  `--ignore-errors` entry joins (FR-025, SC-005, constitution VI).
  Verify: the coverage gate output reads `100.0%` for lines and branches, and `grep -rn "LCOV_EXCL" source/harness test/source/harness_noise_test.cpp test/source/harness_noise_report_test.cpp` reads no match
- [X] T038 [P] Run the full suite under `taskset -c 0,1` and on a 2-cpu
  host, the DBC contract checks green inside the run (FR-018,
  constitution II, VI).
  Verify: `taskset -c 0,1 ctest --test-dir build -j 2` exits 0 on both hosts
- [X] T039 [P] Run the no-sleep and no-clock review over the new tests
  and the time-source gate (FR-019, constitution VIII).
  Verify: `grep -rnE "sleep|chrono|clock" test/source/harness_noise_test.cpp test/source/harness_noise_report_test.cpp` reads no match, and `test/time_source_gate.sh` exits 0
- [X] T040 [P] Run the docs gates and the prose gate over the branch
  range (constitution VIII, XI; FR-021 to FR-023).
  Verify: `ctest --test-dir build -R "example_drift|snippet_compile|example_run|report_shape" --output-on-failure` exits 0 (the fixtures tasks of the same families included), and the prose job exits 0
- [X] T041 [P] Run the code-discipline gates: no lambda stands inside
  an `SG_*` macro, contract-only helpers carry `[[maybe_unused]]`, and
  simdjson stands the only JSON reader the change uses (FR-026).
  Verify: `grep -nE "SG_(REQUIRE|ENSURE|INVARIANT)\(.*\] *\(" source/harness/noise_tracer.cpp` reads no match, `grep -rnE "nlohmann|rapidjson|json.hpp" source/harness/noise_tracer.cpp source/harness/detail/noise_tracer.hpp` reads no match, and the clang-tidy step of T036 stands clean over the new files

**Checkpoint**: every hard gate stands green at the head.

---

## Phase 9: Convergence

**Purpose**: the closing evidence the lane lands with.

- [ ] T042 Read the CI run of the pushed head end to end: every job
  green (`lint`, `test`, `test-rocky`, `sanitize`, `tsan`,
  `static-analysis`, `coverage`, `dbc-gate`, `prose-lint`,
  `shared-audit`, `downstream-consumer`), the coverage figures read
  100.0% lines and 100.0% branches, and the run reference stands
  recorded beside the acceptance evidence under
  `specs/036-osnoise-timerlat-tracers/results/` (constitution VIII)
  (depends on T033 to T041)
- [ ] T043 Re-check the constitution table of [plan.md](./plan.md)
  against the landed tree: every gate stands as the table states it,
  the Complexity Tracking table stands empty, and the record stands in
  the pull-request body (constitution Governance) (depends on T042)

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 (M-A)**: no dependency; blocks every later phase
- **Parallel block one**: Phase 2 (M-B), Phase 3 (M-C) and Phase 4
  (M-E) depend on Phase 1 alone and land in parallel
- **Phase 5 (M-D)**: depends on Phase 2 and Phase 3 (the lifecycle
  wiring needs the parser and the session)
- **Phase 6 (M-F)**: depends on Phase 4 (the example quotes the lines
  the printer emits) and on the binary of Phase 5
- **Phase 7 (M-G)**: T029 stands free after Phase 5; T030 and T031
  need the built binary and the lane host; T032 depends on T030 and
  T031; T033 depends on the parser and the record surfaces
- **Phase 8**: depends on every phase standing green
- **Phase 9**: depends on Phase 8

### Parallel Blocks

- **Block one (after Phase 1)**: Phase 2, Phase 3 and Phase 4 stand
  apart behind the seams the standing code owns; separate workers land
  each phase with fresh context, the phase checkpoints marking the
  hand-off points
- **Within the phases**: the test tasks marked [P] run in parallel
  (different files or disjoint scenario blocks); T026 and T027 run in
  parallel; T034, T035, T036, T038, T039, T040 and T041 run in
  parallel in the polish phase

---

## Parallel Example: Parallel Block One

```text
Task: "T004 to T007: the option row and the value parser (module M-B)"
Task: "T008 to T012: the tracer engine session over the fake trees (module M-C)"
Task: "T013 to T019: the record surfaces against a hand-built record (module M-E)"
```

The three phases touch disjoint files: M-B stands in the option unit
and the new parser declaration, M-C stands in the same parser files
behind the declared surface of M-A, M-E stands in the printer, the
writer, the reader and the schema. M-B and M-C share
`source/harness/detail/noise_tracer.hpp` and
`source/harness/noise_tracer.cpp`; when one worker lands both, M-C
follows M-B, and the parallel block holds across separate workers
through the M-A surface.

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: the public surface and the version move
2. Complete the parallel block one: Phase 2, Phase 3, Phase 4
3. Complete Phase 5: the integration
4. **STOP and VALIDATE**: a run with `--os-noise=osnoise` over the
   fake trees prints the tracer lines beside the standing block, and
   the JSON carries the record

### Incremental Delivery

1. Phase 1: the types and the version stand
2. Parallel block one: the grammar, the engine session and the record
   surfaces stand (three workers)
3. Phase 5: the integration closes User Stories 1, 2 and 3
4. Phase 6: the docs teach the feature
5. Phase 7: the evidence and the standing contracts close User
   Story 4
6. Phase 8 and Phase 9: the gates and the CI run close the lane

---

## Notes

- [P] tasks stand in different files or disjoint blocks
- [Story] labels map the tasks to the spec's user stories
- Every test task stands observed failing before its code lands
- Commit after each task or logical group, with the standing commit
  template; the T003 commit message records the `SOVERSION` check, and
  the T033 amendments ride the code commit (FR-024)
- The tasks phase flags one placement back to the plan: the plan's
  wave table lists FR-011 in wave 1, and the module order of M-D puts
  its green task in Phase 5 beside the `cli.cpp` wiring; the covering
  test task T023 stands with it
- Stop at any checkpoint to validate the stories independently
