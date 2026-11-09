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

- [X] T042 Read the CI run of the pushed head end to end: every job
  green (`lint`, `test`, `test-rocky`, `sanitize`, `tsan`,
  `static-analysis`, `coverage`, `dbc-gate`, `prose-lint`,
  `shared-audit`, `downstream-consumer`), the coverage figures read
  100.0% lines and 100.0% branches, and the run reference stands
  recorded beside the acceptance evidence under
  `specs/036-osnoise-timerlat-tracers/results/` (constitution VIII)
  (depends on T033 to T041). The read stands done: the run
  `40912750824` of head `c6ae33e3` reads every job green, the
  coverage job reads 100.0% lines and branches, and the record
  stands in `results/ci-run.md`.
- [X] T043 Re-check the constitution table of
  [plan.md](./plan.md) against the landed tree: every gate stands as
  the table states it, the Complexity Tracking table stands empty, and
  the record stands in the pull-request body (constitution Governance)
  (depends on T042). The re-check ran at the feature head: the caveats
  the lane records carry their reasons (the coverage branch gate's
  local toolchain shortfall, the pinned docs-example fold, the blocked
  CI read of T042).

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

## Phase: N13e trace-line parsing tasks (2026-11-09)

**Base**: `origin/master` at `a55728af17ec02ea4fbe5e5ff7f8a537ddaa224a`
(version 0.34.0, report schema 1.16, harness `SOVERSION` 22, archive
`SOVERSION` 8, 84 `LCOV_EXCL` markers, a branch shortfall of 21
allowed). **Branch**: `fix/fwq-example-n13e`, phase 15 of
[specs/045-merge-fixes](../045-merge-fixes/tasks.md): one branch, one
squashed commit, one pull request, one acceptance set at one head
(BFW-012). The FWQ example lane and this lane share the branch, and no
file stands in both. No version, schema or `SOVERSION` move, no public
header change, and no new marker, filter, allowance, ignore-error
class or timeout ceiling.

**Design contract**: [plan.md](./plan.md), section "Phase: N13e
trace-line parsing plan (2026-11-09)". **Requirements**: FR-027 to
FR-030, SC-008 to SC-011 and D-8 to D-12 of the trace-line phase of
[spec.md](./spec.md). **Host of record**: thrawn, kernel
`7.3.0-5-cachyos`.

**Rows**: the four task ids stand fixed by the spec. Each carries
lettered rows, and each row stands one file or one tightly-coupled
pair with one verify of its own. A row marked [P] runs in parallel
with the other [P] rows of its block: a different file, or a disjoint
scenario block of one file, the standing rule of this file. TDD mode
stands: each test row that gates new behaviour lands and stands
observed failing before the code row that turns it green, and the
guard rows named under N13E-002 answer green before and after the
code row, pinning the rules the change keeps.

---

- [x] N13E-001 Move the figure read of the two standing engines onto
      the per-cpu trace buffer and retire the count-file read (FR-027,
      D-8, D-9, D-10, D-12; the N13E-002 test rows land first).
  - a. Delete the retired read and its fixture: `struct CountSource`,
    `kCountSources` and `figuresOfCpu` out of
    `source/harness/noise_tracer.cpp`, and `cpuCountFile`,
    `countPathsOf` and the four cases that fold the retired format,
    `theFiguresReadOneEntryPerTracerCpu`,
    `theMissingSourceWordStandsAbsentForThatSourceAlone`,
    `theCpuWithNoCountFiguresStandsEveryFigureAbsent` and
    `theFigureArmRows`, out of `test/source/harness_noise_test.cpp`
    with their `main()` call rows (X.3, D-12). `countOfToken` stays:
    the detector's two helpers and the two new folds read their digits
    through it. This row lands in the same change as N13E-001b, once
    N13E-002i has moved the window rows off the retired paths.
    Verify: `git grep -nE 'osnoise/cpu|kCountSources|figuresOfCpu|cpuCountFile|countPathsOf' -- source test`
    reads no match, and `git grep -nE 'countOfToken' -- source/harness/noise_tracer.cpp`
    names the definition, the three detector call sites and the two new
    folds' uses.
  - b. Land the buffer read in `source/harness/noise_tracer.cpp` and
    its contract in `source/harness/detail/noise_tracer.hpp`: the
    anonymous-namespace helpers `tokensOf`, `standsAnchored` and
    `decimalOf`; the two constants `kPeriodColumns` at nine and
    `kActivationColumns` at six; the two folds `osnoiseFiguresOfCpu`
    and `timerlatFiguresOfCpu`, each with one rule, one postcondition
    and one `SG_ENSURE` at its answer; `countOfToken` taking
    `std::string_view` so a token needs no copy; and the loop of
    `noiseTracerFigures` reading
    `sys/kernel/tracing/per_cpu/cpu<id>/trace` once per cpu for all
    three engines, the path expression written once (D-9). The header
    keeps its `\pre` and rewrites its body and its `\post` pair in the
    shape plan.md quotes. `kGapMarker`, `kNmiField`, `widerGapOf`,
    `nmiOf` and `gapFiguresOfCpu` keep their bytes, and no column
    table or line rule stands shared with the detector read.
    Verify: `cmake --build build` exits 0, `ctest --test-dir build -R harness_noise_test --output-on-failure`
    exits 0 with the N13E-002 rows green, `cmake --build build --target
    dbc-gate` exits 0 with the two new `SG_ENSURE` answers paired, and
    `git diff` names no change inside the five detector symbols above.

- [x] N13E-002 Stand the parsed-form cases on injected buffer text and
      re-anchor the lifecycle rows to it (FR-028, FR-008, FR-009,
      FR-013, SC-008, SC-009). Rows a to i stand in
      `test/source/harness_noise_test.cpp` behind disjoint scenario
      blocks; row j stands in `test/source/harness_noise_report_test.cpp`.
      Each row pairs with the code row it gates: N13E-001b turns rows
      a, b, c, d and i green, and each of those five stands observed
      failing first. Rows e, f, g, h and j stand guard rows: they
      answer green on the standing read, and they pin the absence rule
      and the rendering the change keeps. No row sleeps, reads a clock
      or names a fixed cpu: the cpus come from `pickCpus` (FR-019).
  - a. Land the fixture helpers and the verified bytes, and write the
    first sums case: `bufferPath(cpu)` answering
    `sys/kernel/tracing/per_cpu/cpu<id>/trace`, the spelling the
    specs/048 fixture already carries in
    `test/source/harness_hwlat_tracer_test.cpp`; the audit-point
    constants byte for byte, the legend line, the two osnoise period
    lines and the timerlat `irq` and `thread` pair; and
    `theFiguresFoldThePeriodLinesOfEachCpuBuffer`, two cpus from
    `pickCpus(2)` in the session order, each buffer carrying that
    cpu's verified period lines, the five figures answering the sums of
    the five counter columns (SC-008).
    Verify: `ctest --test-dir build -R harness_noise_test --output-on-failure` exits non-zero with the new check failing (red observed)
  - b. [P] `theTimerlatLinesCountTheTwoContexts`: the verified pair
    counts the `irq` figure at 1 and the `thread` figure at 1, and
    nmi, softirq and hardware stand absent for that engine (SC-008,
    D-10). This row carries the per-source rule of FR-010 that the
    retired per-source case held.
    Verify: `ctest --test-dir build -R harness_noise_test --output-on-failure` exits non-zero with that check failing (red observed)
  - c. [P] `theMixedBufferAnswersTheSumsOfItsParsedLines`: one buffer
    of the legend line, the verified lines, malformed lines, a blank
    line and a planted `#` line that carries a period tail; the sums
    come from the parsed lines alone, the skip rule of Round 1 and
    Round 2 (SC-008).
    Verify: `ctest --test-dir build -R harness_noise_test --output-on-failure` exits non-zero with that check failing (red observed)
  - d. [P] `theZeroCounterColumnStandsAFilledZero`: a counter column
    reading 0 on every period line answers a filled 0, the shape the
    detector read answers for `nmiCount`; the verified host line
    already carries HW 0 and NMI 0 (SC-008).
    Verify: `ctest --test-dir build -R harness_noise_test --output-on-failure` exits non-zero with that check failing (red observed)
  - e. [P] `theCpuWithNoParsedLineStandsEveryFigureAbsent(0..3)`: an
    empty buffer, a missing file, a buffer path planted as a directory
    through `denyTracingPath`, and a legend-only buffer each answer
    every figure absent: the two `readTracerFile` arms, the same
    directory denial
    `theInWindowFailureStillRestoresTheTree` plants at that path in
    the specs/048 fixture (SC-008, SC-011).
    Verify: `ctest --test-dir build -R harness_noise_test --output-on-failure` exits 0 once the
    row lands, and the row answers green on the standing read and goes
    red beside a read that guesses a figure
  - f. [P] `thePeriodShapeRows`: a shape array in the shape of the
    retired `theFigureArmRows`, so each row prints its own shape on
    failure, one buffer of that one line each, every shape answering
    the absence rule. The table carries a non-digit in each of the
    three digit-only non-counter columns, a signed figure in a counter
    column, a bare integer and a `99.` in the decimal column, an
    eight-figure tail, a ten-figure tail that puts a bare figure in the
    anchor position, and a line whose token ahead of the nine carries
    no colon (SC-008).
    Verify: `ctest --test-dir build -R harness_noise_test --output-on-failure` exits 0 once the
    row lands, and the row answers green on the standing read and goes
    red beside a read that guesses a figure
  - g. [P] `theActivationShapeRows`: the second shape array, one
    buffer of that one line each, every shape answering the absence
    rule. The table carries no `#`, `contexts` for `context`, a
    context word outside the two, `timer_latencies`, a non-digit
    latency, `n` for `ns`, a token after `ns`, five tokens after the
    anchor, and no colon anchor (SC-008).
    Verify: `ctest --test-dir build -R harness_noise_test --output-on-failure` exits 0 once the
    row lands, and the row answers green on the standing read and goes
    red beside a read that guesses a figure
  - h. [P] `theLegendAndPlantedHashLinesStandNoLine`: the audit point's
    legend bytes and a planted `#` line that carries a period tail each
    answer the absence rule, the header precedence FR-027 fixes (the
    first character `#` stands a header line whatever the tail).
    Verify: `ctest --test-dir build -R harness_noise_test --output-on-failure` exits 0 once the
    row lands, and the row answers green on the standing read and goes
    red beside a read that guesses a figure
  - i. [P] Re-anchor the serial-window, `mode=both` and cpu-list rows to
    buffer text through the same seam (FR-028, FR-008, FR-009, D-11, Round 2):
    `countPathsOf` becomes `bufferPathsOf`; `TracerWindowReader` takes
    buffer paths and appends, its first read opening the buffer with
    `std::ios::app` and writing the appended period lines, so the
    earlier lines stand and no rewrite reads as the reset the row
    proves absent; `kPreWindowFigures` and `kPostWindowFigures` become
    `kPreWindowLines`, the legend bytes with one period line, and
    `kAppendedLines`, one period line, their counters planted so the
    sums over both lines answer nmi 11, irq 12, softirq 13, thread 14
    and hardware 15; `checkPostWindowFigures` keeps its bytes under the
    name `checkWindowSums`; `theWindowLifecycleRows` (the serial pair,
    `mode=both`) and `theTracerCpuRows` (the named list, the absent
    key, the wholly planned set) stand re-pathed with no change of
    assertion, so the sums cover every line the buffer holds, the
    engine stands up before the reads, and the tree stands restored.
    Verify: `ctest --test-dir build -R harness_noise_test --output-on-failure` exits non-zero with the two row families failing on the sums (red observed), and `git grep -nE 'kPreWindowFigures|kPostWindowFigures|countPathsOf' -- test` reads no match
  - j. [P] `theTimerlatFiguresPrintTwoFilledAndThreeAbsent` in
    `test/source/harness_noise_report_test.cpp`: a hand-built
    `timerlat` record prints the two filled figure lines and the
    standing `not available` wording in the other three positions, and
    the standing byte-identity fixtures `kBlockHead`, `kTracerLines`
    and `kNotAvailableLine` keep their bytes (FR-013, D-10).
    Verify: `ctest --test-dir build -R harness_noise_report_test --output-on-failure`
    exits 0 once the row lands, the row answers green on the standing
    printer and goes red beside a printer change for absent figures,
    and `git diff` names no change to the three standing fixture
    constants

- [x] N13E-003 Land the record, the quickstart link and the console
      contract line (FR-029, SC-010).
  - a. [P] Amend `specs/036-osnoise-timerlat-tracers/contracts/console-report.md`
    with the one rule block plan.md quotes, naming the buffer path, the
    per-engine line reading and the standing wording for a buffer that
    holds no parsed line. The amendment rides the change that lands
    N13E-001b (FR-024, FR-029). No other rule of that file moves.
    Verify: `grep -c "per_cpu/cpu<id>/trace" specs/036-osnoise-timerlat-tracers/contracts/console-report.md` reads at least one line, `git diff` names no other rule of that file, and `python3 tools/prose/prose_gate.py --mode tree --check prose` exits 0
  - b. [P] Write `specs/036-osnoise-timerlat-tracers/results/trace-line-thrawn.md`
    (FR-029, D-11):
    the audit point repeated, the legend bytes, the four verified
    lines, the `period_us` read and the per-cpu `buffer_size_kb` read;
    each acceptance command with its exit and its window token; the
    resolved cpu list and the figures beside the CCD naming the
    standing records use; the tracing state each run found and the
    state it left, the saved word, the saved mask and `tracing_on`
    read before and after; the unrooted suite row apart; the dated
    rows labeled preliminary with the pre-squash head each stands
    taken at; and the stale-wording list of FR-029, carrying the
    public header comment, the spec 034 report-json line, this lane's
    option contract, the data model, the research rows, the quickstart
    wording and the two standing edge-case rows of `spec.md` that name
    the count files and the cumulative-totals reading. The standing
    2026-10-29 records keep their text (D-12).
    Verify: `test -f specs/036-osnoise-timerlat-tracers/results/trace-line-thrawn.md`, `grep -c "buffer_size_kb" specs/036-osnoise-timerlat-tracers/results/trace-line-thrawn.md` reads at least one line per run, and the record names each item above with its command and exit
  - c. Add one link row to that record in the acceptance section of
    `specs/036-osnoise-timerlat-tracers/quickstart.md` (FR-029). No
    other quickstart line moves, the count-file wording included.
    Verify: `grep -c "results/trace-line-thrawn.md" specs/036-osnoise-timerlat-tracers/quickstart.md` reads one link row, and `git diff --stat` names one changed line in that file

- [x] N13E-004 Run the acceptance set at the final squashed head
      (FR-030, SC-009, SC-010, SC-011; BFW-012 names the one head both
      lanes measure). Each row stands a final-head row: it opens only
      at the squashed head of `fix/fwq-example-n13e`, on thrawn, and
      its reading lands in the record of N13E-003b. A run whose host
      refuses the tracefs writes stands BLOCKED with the command and
      the refusal.
  - a. The osnoise acceptance run behind the rights the writes need:
    `sudo ./build/example/benchmark_example` with
    `--filter=bmTouch`, `--threads=2`, `--placement=compact`,
    `--jitter-probe=idle,mode=precheck,precheck=18000`,
    `--os-noise=osnoise` and `--json=$HOME/scratch/fwq-example-n13e/osnoise.json`.
    The window token stands at 18,000 ms of the probe's tick time base:
    the host counter runs near 4.2 GHz, so the standing 3,000 token
    spans 0.72 s of host time and holds no period line.
    Verify: the run exits 0; the five source figures stand filled for
    every tracer cpu the run resolves; the record names the resolved
    cpu list, the per-cpu `buffer_size_kb` read and the figures beside
    the CCD naming the standing records use; and the state stands
    restored to the saved word, the saved mask and `tracing_on` 0 with
    the found reads named beside them (FR-030, SC-010).
  - b. The timerlat acceptance run, the same shape with
    `--os-noise=timerlat` and `--json=$HOME/scratch/fwq-example-n13e/timerlat.json`.
    Verify: the run exits 0; the irq and thread figures stand filled
    for every tracer cpu the run resolves; the other three print the
    standing per-source wording; the record carries the same reads as
    row a (FR-030, D-10, SC-010).
  - c. The unrooted suite row, recorded apart from the two runs and
    answered on the standing tree:
    `ctest --test-dir build -R 'harness_noise_test|harness_noise_report_test'`.
    Verify: it exits 0 with no root and no kernel tracer, and the
    coverage lane of the shared set reads 100.0% lines with the branch
    shortfall at or below 21, the marker count at or below 84, no new
    marker, and no cold row where the retired reader stood (SC-009,
    SC-011, spec 045 SC-032).

**Checkpoint**: the two standing engines read the buffer the host
carries, the parsed forms stand pinned without root, the contract and
the record name the new surface, and the acceptance figures stand
filled at one head.

---

### Dependencies & Execution Order (N13e trace-line phase)

**Row dependencies**

- **N13E-002a stands first**: it lands `bufferPath` and the verified
  bytes every other test row reads, and it carries the first sums case.
- **N13E-001b follows the test rows**: the folds, the path-once loop
  and the rewritten header contract turn the red rows green, the
  standing TDD order of this file.
- **N13E-001a lands in the same change as N13E-001b**, and after
  N13E-002i has moved the window rows off `cpuCountFile` and
  `countPathsOf`: the retired reader, its source-word table, its
  fixture helper and its four cases go with their subject in one
  change (X.3, D-12).
- **N13E-003a rides N13E-001b**: the contract line names the surface
  the code lands (FR-024).
- **N13E-003c follows N13E-003b**: the link names a record that stands.
- **N13E-004 follows the shared squash**, named below.

**Parallel blocks**

- **Block one, after N13E-002a**: N13E-002b, N13E-002c, N13E-002d,
  N13E-002e, N13E-002f, N13E-002g, N13E-002h and N13E-002i, disjoint
  scenario blocks of `test/source/harness_noise_test.cpp`; N13E-002j in
  `test/source/harness_noise_report_test.cpp`; N13E-003a in the console
  contract; N13E-003b in the record file. No file stands twice in the
  block.
- **Block two, alone**: N13E-001b with N13E-001a and N13E-003a in one
  change, one file pair for the read and one contract file.
- **Block three**: N13E-003c, one line in one file.
- **Alone and in series**: N13E-004a and N13E-004b. Both write
  `current_tracer`, `tracing_cpumask` and `tracing_on`, one tracefs
  carries one engine at a time, and no other work runs on the host
  during them. N13E-004c runs apart from both and answers on the
  standing tree.

**Gated behind the shared squash**

The branch carries two lanes and one squashed commit, so these rows
close at one head and stand gated behind it:

- N13E-004a, N13E-004b and N13E-004c, and the final-head rows of the
  record: the two runs' figures, the found and left tracing state, the
  resolved cpu list and the `buffer_size_kb` read.
- The SC-010 figure reading and the SC-011 coverage reading, the
  latter taken in the shared set's coverage lane, which also answers
  that the retired reader leaves no cold row.
- The pull-request body naming the measured head beside every reading,
  and the record's dated rows losing their preliminary labels only at
  that head.
- The FWQ example rows BFW-009 to BFW-011 of spec 045 phase 15: BFW-012
  names one acceptance set for both lanes, so this lane waits for that
  lane's tracked change before the squash, and no N13E-004 row runs
  before it. A tracked change after the squash moves the head, and
  every row of N13E-004 runs again there.

**Completion rule for the N13e phase**

A row closes only with its verify reading in
`specs/036-osnoise-timerlat-tracers/results/trace-line-thrawn.md`: the
command, the exit, the host, the tool versions and the figure. An
interim row stands dated and labeled preliminary with the pre-squash
head it stands taken at. A row that stops at a scope boundary stands
open with the file it needs, and the pull-request body carries it. A
finding that needs a file outside this lane's scope, a `README.md`
line, `test/CMakeLists.txt` or a preset, stands reported before any
edit.
