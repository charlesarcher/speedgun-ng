# Tasks: Host Validation

**Input**: Design documents from `specs/031-host-validation/`

**Prerequisites**: [plan.md](./plan.md) (required), [spec.md](./spec.md)
(required for user stories), [research.md](./research.md) (R-01 to R-13),
[data-model.md](./data-model.md) (E-21 to E-27),
[contracts/](./contracts/), [quickstart.md](./quickstart.md),
[results/plan.md](./results/plan.md)

**Tests**: TDD mode (plan.md, Execution mode): the test task of each
wave stands written and observed failing against the standing tree
before the code task that turns it green. Each test task names its wave
of plan.md's Test Plan; each code task names the test tasks it closes.

**Organization**: tasks stand grouped by user story so each story can
stand implemented and tested independently.

**Build surface**: the two programs build through `add_executable` lines
in `test/CMakeLists.txt`, the pattern that file sets for
`../tools/quill/quill_dependency_check.cpp`, the `quill_dependency_check`
target, so the binaries stand at `build/test/host_record_write` and
`build/test/host_record_check`. `add_subdirectory(test)` stands at
`cmake/dev-mode.cmake:5`, so both programs, the five `test` entries and
the `validate` target exist in developer mode only.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: can run in parallel with the other marked tasks of the phase
  (different files, no dependency between them)
- **[Story]**: the user story this task serves (`US1` to `US5`)
- **M-x**: the module of plan.md the task belongs to (`M-A` to `M-H`)
- Every description names the exact file path and the requirements it
  closes

## Path Conventions

- Repository root: `$PWD`, the working tree root the run started in, the
  substitution the record set uses for every absolute path; branch
  `031-host-validation`.
- Library: `source/harness/`, private headers under
  `source/harness/detail/`. Programs: `tools/hostcheck/`. Built
  programs: `build/test/`.
- Tests: `test/source/`, registered in `test/CMakeLists.txt`.
- Evidence: `specs/031-host-validation/results/`. Committed records:
  `validation/`.

---

## Phase 1: Setup (the pinned schema, the two headers, the test surface)

**Purpose**: M-A. The document's guard, the plain-data types of both
sides, and the CMake surface that compiles the schema test unit. Each
registration stands in the task that writes the source it compiles, so
every `add_executable` lands beside a file the configure step can find.

- [x] T002 [P] Write the failing schema tests in a new
  `test/source/harness_host_record_schema_test.cpp`: the file reads the
  pinned schema through `SG_SCHEMA_PATH` and asserts the root key set, the
  `x-order` of [contracts/record-schema-file.md](./contracts/record-schema-file.md),
  the kinds of every key, the `type` lists of the two nullable keys
  `cpu.stepping` and `machine.nodes`, each naming its kind and `null`, and
  the kind label `host-record` and the version `1.0`, the text
  `kHostRecordVersion` names in `source/harness/detail/host_record.hpp`
  and the test plants as its own oracle; each
  planted document fails
  `sg::detail::checkReport`, one root key missing, one key added, two
  root keys swapped, one value retyped, one root key twice (FR-019,
  FR-020, FR-021, SC-005) (wave 1, M-A)
  Recorded in `results/quickstart.md`: "six entries pass in 0.02 s, one process
  each".
- [x] T001 Register the schema test in `test/CMakeLists.txt`, once T002
  has written its source: the `add_executable` line for
  `harness_host_record_schema_test` over
  `test/source/harness_host_record_schema_test.cpp`, linked to
  `speedgun-ng::harness` with `${CMAKE_SOURCE_DIR}/source/harness` on the
  include path, the shape of the `harness_precheck_test` target; the
  `SG_SCHEMA_PATH` define aimed at
  `${CMAKE_SOURCE_DIR}/docs/schema/host-record-v1.json`, the shape of
  `harness_json_report_test`; and
  `add_test(NAME harness_host_record_schema_test ...)`, so the label loop
  and the timeout loop over the directory `TESTS` property hand the entry
  the `quick` label and the 300 s ceiling. The two remaining units
  register in T006 and T007, each beside the source that task writes
  (FR-021, FR-024, FR-035, FR-037) (depends on T002; it stands the first
  red run) (M-A)
  Recorded in `results/taskset.md`: "133 harness_host_record_schema_test".
- [x] T003 State the record in a new `docs/schema/host-record-v1.json`:
  the document kind `host-record` as its `title` label, which no checker
  keyword reads, the version `1.0` that `kHostRecordVersion` names, the four
  root keys with their
  `x-order`, the keys and kinds of `cpu`, `machine`, `machine.l3[]`,
  `machine.nodes[]` and `evidence[]`, the closed-object rule on every
  object, and `null` on `cpu.stepping` and `machine.nodes`. A `required`
  array stands on every object the schema describes: the root, `cpu`,
  `machine`, each `machine.l3[]` row, each `machine.nodes[]` row and each
  `evidence[]` row, because the guard reports an absent key only through
  that list (the `node.required` loop of `checkNode` in
  `source/harness/json_load.cpp`), where a closed
  object catches a key present and wrong, never one absent
  (FR-019, FR-020, FR-021) (depends on T002; the tests turn green) (M-A)
  Recorded in `results/quickstart.md`: "read against the schema that root
  carries".
- [x] T004 [P] Declare the record model in a new
  `source/harness/detail/host_record.hpp`: `HostIdent`, whose `key` carries
  the record key `mapfileKey` renders with its stepping segment and whose
  `stepping` reads `null` where the host answers no leaf 1, `HostMachine`,
  `EvidenceRow`, `HostRecord` and `kHostRecordVersion`, the text `1.0` the
  renderer and staleness rule 1 both read, `kHostRecordCapBytes`, the one
  figure of 16 KiB that the writer's refusal and the reader's guard both read,
  ending the file-local copy each side carried, then
  `hostMachineFromLayout` and
  `hostRecordWriteMain(argc, argv, facts)`, which answers `int` and takes
  the host facts through `facts`, a `HostFactsSeam` of that header asked
  only once `--root` and `--out` answer, the shape FR-037 and the
  Clarifications of 2026-10-29 state for both entry points. Each name
  carries the `\pre`, `\post` and `\invariant` comments Principle II asks
  for, and no host read stands
  inside the header (E-21 to E-24; FR-004, FR-007, FR-008, FR-012, FR-015,
  FR-019, FR-032, FR-037) (M-A)
  Recorded in `results/quickstart.md`: "1 package, 1 L3 group, 20 cores, 20
  threads, mask 0-19".
- [x] T005 [P] Declare the reader model in a new
  `source/harness/detail/host_record_check.hpp`: `RecordVerdict`,
  `RecordReading`, `DirectoryAnswer`, `DirectoryChoice`,
  `DirectoryChooser`, `DirectoryChooserFactory`, `RecordCheck`,
  `kHostRecordSchemaRelPath`, `readHostRecord`, `staleReasons`,
  `checkRecords` and `hostRecordCheckMain(argc, argv, factory)`, whose third
  parameter `factory` carries that alias, in the shapes
  `contracts/record-check.md:11-56` writes, each
  with its Principle II comments and the invariant that no entry point
  reads a clock, a cpu, `/proc` or `/sys` (E-26; FR-024 to FR-030) (M-A)
  Recorded in `results/quickstart.md`: "stale GenuineFake-9-9-9 selects no
  catalog directory".

**Checkpoint**: the schema file and the two headers stand, and the one
registered test unit configures, builds, and runs a `main` that reports
red.

---

## Phase 2: Foundation (the machine block, the bytes, the two skeletons)

**Purpose**: M-B and M-C groundwork. The pieces both stories share: the
machine block of an installed layout, the byte renderer, the schema
guard call, and the line and exit shape. T006 and T007 each register the
unit whose source they write, so the phase ends with three test units in
the suite.

- [x] T006 [P] [US1] Write the failing machine-block tests in a new
  `test/source/harness_host_record_test.cpp`: `installTestLayout` hands
  `hostMachineFromLayout` a planted layout, and the block answers the four
  counts, the mask in the `appendRangeText` spelling
  (`source/harness/topology.cpp:529`), one row per L3 group, and `null`
  for `nodes` where `memoryReadable` of
  `source/harness/detail/internal.hpp` stands false (FR-012, FR-013, FR-014,
  FR-032, SC-001) the task ends by registering `harness_host_record_test` in
  `test/CMakeLists.txt` beside T001's line, the shape of the
  `harness_precheck_test` target, so its red run builds and runs
  (wave 2, M-B)
  Recorded in `results/taskset.md`: "those units passed under the mask in both
  readings".
- [x] T007 [US2] Write the failing read tests in a new
  `test/source/harness_host_record_check_test.cpp`: `readHostRecord`
  answers a record for planted valid bytes and no record for each planted
  key-set, order and kind break, and each empty answer carries one line
  naming the file, the `broken` verdict and the key it read. The planted
  document that drops `cpu.key` names its CPU type by the entry's own name
  with the `.json` suffix removed (FR-020, FR-024, FR-030, SC-003); the
  task ends by appending the `add_executable` line for
  `harness_host_record_check_test` after T006's, the shape of the
  `harness_precheck_test` target, and that target carries the
  `SG_SCHEMA_PATH` define the way `harness_json_schema_test` carries it, so
  its red run builds and runs and the
  test can reach the pinned schema file (wave 3, M-C)
  Recorded in `results/taskset.md`: "None of them skipped, none of them failed".
- [x] T008 [P] Implement `hostMachineFromLayout` in a new
  `source/harness/host_record.cpp`: the four counts read the installed
  layout's vectors, the mask text runs through `appendRangeText`, and the
  node rows stand `null` where `memoryReadable` stands false (FR-012,
  FR-013, FR-014, R-05) (depends on T004, T006; the tests turn green)
  (M-B)
  Recorded in `results/quickstart.md`: "1 package, 1 L3 group, 2 cores, 2
  threads, mask 0-1".
- [x] T009 [P] Implement `readHostRecord` in a new
  `source/harness/host_record_check.cpp`: the bytes reach
  `sg::detail::checkReport` (`source/harness/detail/json_report.hpp:322`)
  against the pinned schema, and no other reader touches JSON (FR-020,
  FR-022, FR-024, FR-030, R-03) (depends on T005, T007; the tests turn
  green) (M-C)
  Recorded in `results/ci-run.md`: "host_record_check.cpp answers 100.0% lines
  over 235".
- [x] T010 [US1] Write the failing byte tests in
  `test/source/harness_host_record_test.cpp`: a written document holds its
  keys in the schema order, two writes of one planted host differ in no
  byte, the file ends with the document and no newline, the bytes carry no
  wall-clock instant, and a write whose document would pass the 16 KiB
  `kHostRecordCapBytes` names
  refuses with one line and exit 2, its standing bytes left alone and an
  empty output root left empty (FR-018, FR-023, FR-033, SC-005,
  SC-007) (wave 2, M-B)
  Recorded in `results/quickstart.md`: "Bytes | 315 after each run".
- [x] T011 [US2] Write the failing line and exit tests in
  `test/source/harness_host_record_check_test.cpp`: `checkRecords` owns the
  listing and answers a `RecordCheck` holding one `RecordReading` per
  `.json` entry directly under the record root, in file-name order, each
  naming its CPU type, and a `valid` reading carries an empty `detail`. The
  planted root holds a stray file without the suffix and a subdirectory
  beside the records, and those two entries answer no reading and no
  refusal. Both a record root that stands absent and one that holds no
  `.json` entry
  answer neither readings nor `refusal`, one path through the `directory`
  read answered by its error code, the answer that stands apart from a
  refused run. The status `hostRecordCheckMain` answers over a set whose
  records all pass, and the absence of any `/proc` or `/sys` read beside it,
  stand as an answer in T026, where that entry point gets its wiring, and
  its run-level lines stand in T040, one owner each, the shape T024 uses for
  the empty root (FR-024, FR-026, FR-027, FR-029, SC-003) (wave 3, M-C)
  (depends on T023)
  Recorded in `results/quickstart.md`: "the README entry there reads as
  nothing".
- [x] T012 Render the record in `source/harness/host_record.cpp`: a
  file-local writer pair over `simdjson::builder::string_builder`, the
  call order of
  [contracts/host-record-json.md](./contracts/host-record-json.md), the
  the `kHostRecordCapBytes` check on the finished text, and one write of the
  whole buffer
  (`source/harness/compare.cpp:765`, `:794`) (FR-018, FR-020, FR-023,
  R-02, R-09) (depends on T008, T010; the tests turn green) (M-B)
  Recorded in `results/ci-run.md`: "100.0% lines over 253 and 100.0% branches
  over 82".
- [x] T013 Print the verdicts in `source/harness/host_record_check.cpp`:
  `checkRecords` reads the `.json` entries directly under the record root
  in file-name order and answers one `RecordReading` per entry, naming the
  path, the CPU type, the verdict and the detail, and ignores every other
  entry there. The line that carries a reading and the status read from the
  readings stand as the mapping in T026, beside the entry point that makes
  them observable, one owner each (FR-024, FR-026, FR-027, FR-029, SC-003)
  (depends on T009, T011, T023; the tests turn green) (M-C)
  Recorded in `results/quickstart.md`: "broken broken host-record: /machine:".

**Checkpoint**: both library sides build into the harness target through
the `source/harness/*.cpp` glob row of `CMakeLists.txt`, the three test units
stand registered,
and the two waves stand green on planted values alone.

---

## Phase 3: User Story 1 - A host proves what it is (Priority: P1)

**Goal**: one run against an empty output root leaves one record named by
the rendered record key with the `.json` suffix, prints one line, and
exits 0.

**Independent Test**: a planted `HostIdent` and an installed layout
produce one record whose name is that key with the `.json` suffix, whose
identity fields equal the planted identity, and whose machine facts equal
the installed layout ([spec.md](./spec.md) US1).

### Tests for User Story 1 (write first, observe failure)

- [x] T014 [US1] Write the failing write tests in
  `test/source/harness_host_record_test.cpp`, where the call
  `hostRecordWriteMain(argc, argv, facts)` with a planted identity and an
  installed layout answered through the `HostFactsSeam` `facts`, run on a
  valid argument vector against an empty output root, leaves one file
  named `cpu.key` with the `.json` suffix, prints one line naming the CPU
  type and the path, answers status 0, and leaves that record as the only
  file in the root
  (FR-003, FR-005, FR-007, FR-008, FR-009, FR-032, SC-001) (wave 2, M-B)
  Recorded in `results/quickstart.md`: "wrote one record named by the rendered
  key with the .json suffix".
- [x] T015 [US1] Write the failing refresh tests in
  `test/source/harness_host_record_test.cpp`: a second write over a
  standing record replaces `cpu` and `machine`, keeps `evidence` row for
  row in the stored order, and leaves one file for that CPU type (FR-006,
  SC-002) (wave 2, M-B)
  Recorded in `results/quickstart.md`: "one file still, 315 bytes, its modified
  time moved".
- [x] T016 [US1] Write the failing evidence tests in
  `test/source/harness_host_record_test.cpp`: three planted rows read back
  with `path`, `command`, `exit` and `line` each, in the order they were
  given (FR-015, FR-018, SC-007) (wave 2, M-B)
  Recorded in `results/quickstart.md`: "six entries pass in 0.02 s, one process
  each".
- [x] T017 [US1] Write the failing round-trip tests in
  `test/source/harness_host_record_schema_test.cpp`: a document the writer
  produced passes the guard, re-reading its bytes yields equal values, and
  the schema's key set and order equal the writer's call order (FR-021,
  FR-023, SC-005) (waves 1 and 2, M-B)
  Recorded in `results/quickstart.md`:
  "$PWD/build-val/validation/GenuineIntel-6-C6-2.json GenuineIntel-6-C6-2
  valid".

### Implementation for User Story 1

- [x] T018 [US1] Implement the write path of
  `hostRecordWriteMain(argc, argv, facts)` in `source/harness/host_record.cpp`:
  the parse of `--root` and `--out` on its success path, where a run that
  names no `--out` resolves the default output root of FR-002 against the
  tree root it read and stands the record at `<root>/validation` (T041 adds
  the refusal arms with the tests that plant them), the host facts asked
  through `facts` only once the arguments answer, a standing record read or
  a fresh start, the kept `evidence`, the render, the cap check, one write
  to the name `cpu.key` with the `.json` suffix, the one line naming the CPU
  type and the path, and the status 0 the thin `main` maps
  (FR-002, FR-005, FR-006, FR-009, FR-023, FR-037, R-08, R-09)
  (depends on T012, T014, T015, T016, T017; the tests turn green) (M-B)
  Recorded in `results/quickstart.md`: "the two lines the programs print carry
  the record path absolute under it".
- [x] T019 [P] [US1] Write the thin writer program in a new
  `tools/hostcheck/host_record_write.cpp`: it calls
  `hostRecordWriteMain(argc, argv, facts)` with `argc`, `argv` and, as
  `facts`, the `HostFactsSeam` that
  `source/harness/detail/host_record.hpp` declares. That seam fills
  `HostIdent` from `pmuIdentCurrent()` and
  `mapfileKey` (`source/counters/detail/pmu.hpp:84`, `:77`), answers
  `directory` through `pmuSelectDirectory(std::istream&, const PmuIdent&)`
  (`source/counters/detail/pmu.hpp:99`) over the
  `<root>/external/pmu-events/arch/x86/mapfile.csv` of the root the parse
  named, and takes `layout()` once, then maps the answered status to the
  process exit, with the two internal headers reached by relative include
  the way `test/source/counters_linux_pmu_seam_test.cpp:48` reaches one
  (FR-004, FR-007, FR-008, FR-012, FR-014, FR-037, R-04) (M-D)
  Recorded in `results/taskset.md`: "host-record: GenuineIntel-6-C6-2
  <planted>/out/GenuineIntel-6-C6-2.json".
- [x] T020 Build the writer in `test/CMakeLists.txt`: an
  `add_executable(host_record_write ../tools/hostcheck/host_record_write.cpp)`
  line linked to `speedgun-ng::harness` and `speedgun-ng::speedgun-ng`,
  the pattern `quill_dependency_check` sets, so the build artifact of
  the program stands at `build/test/host_record_write` in developer mode
  (`cmake/dev-mode.cmake:5`), where the developer build puts every target
  (FR-003, FR-037) (depends on T019) (M-E)
  Recorded in `results/quickstart.md`: "[100%] Built target host_record_write".

**Checkpoint**: one planted host yields one record and one line; the
writer builds at `build/test/host_record_write`. The `validate` target
follows in Phase 4, because its `DEPENDS` names both programs.

---

## Phase 4: User Story 2 - CI reads the committed records (Priority: P1)

**Goal**: the reader names every CPU type whose record breaks or stands
stale, over files alone, and the `validate` target runs both programs.

**Independent Test**: planted valid records exit 0 with one line each;
each planted break and each planted stale cause print one line naming the
CPU type and the rule and exit 2 ([spec.md](./spec.md) US2).

### Tests for User Story 2 (write first, observe failure)

- [x] T021 [US2] Write the failing staleness tests for the first two rules
  in `test/source/harness_host_record_check_test.cpp`: a record whose
  `recordVersion` lags the `1.0` `kHostRecordVersion` names at
  `source/harness/detail/host_record.hpp` names both versions, the test
  planting its own `1.0` oracle, and a record citing an
  absent path names that path. The same task plants the three-row record
  of the US3 row shape over a planted tree, where the reader confirms
  every cited path and every cited line, and the arm that deletes one
  cited file, which names that row's CPU type as stale with the path it
  could not confirm. The planted tree holds its own copy of
  `docs/schema/host-record-v1.json`, the path the reader resolves under the
  root it is given (FR-015, FR-025, SC-003) (wave 3, M-C)
  Recorded in `results/quickstart.md`: "the record speaks 0.9 and the format
  speaks 1.0".
- [x] T022 [US2] Write the failing staleness tests for the last two rules
  in `test/source/harness_host_record_check_test.cpp`: a cited line that
  stands absent in a standing file names that path, and a planted mapfile
  that selects no directory names the key and the identity; the test
  injects its own chooser lambda and plants each `DirectoryAnswer`,
  `DirectoryAnswer::NO_ROW` for the identity rule 4 names and
  `DirectoryAnswer::FOUND` with a planted directory for the rows that match,
  the selection an injected mapfile
  reaches through `pmuSelectDirectory(std::istream&, const PmuIdent&)`
  (`source/counters/detail/pmu.hpp:99`), so no hardware read stands needed.
  The planted root holds the schema copy beside the planted mapfile
  (FR-025, FR-028, FR-035, SC-003, AC-1) (wave 3, M-C)
  Recorded in `results/quickstart.md`: "selects no catalog directory: vendor
  GenuineFake, family 9, model 9".
- [x] T023 [US2] Implement `staleReasons`, declared as
  `contracts/record-check.md:39-42` writes it, in
  `source/harness/host_record_check.cpp`: the four rules of
  [contracts/record-check.md](./contracts/record-check.md) in their
  stated order, rule 1 comparing `recordVersion` against
  `kHostRecordVersion` in `source/harness/detail/host_record.hpp` so the
  two sides cannot disagree, every cause the record carries named, and the
  chooser
  asked exactly one question, whose `DirectoryAnswer::NO_ROW` answer is rule
  4's reason and whose `DirectoryAnswer::FOUND` answer ends that rule.
  Nothing compares a directory: the
  path the answer carries is the selection the writer made beside the same
  key, and `cpu.directory` stands in the record as written
  (FR-025, FR-026, R-06) (depends on T021, T022; the
  tests turn green) (M-C)
  Recorded in `results/ci-run.md`: "100.0% lines over 235 and 97.4% branches
  over 78".
- [x] T024 [US2] Write the failing break, cap and empty-evidence tests in
  `test/source/harness_host_record_check_test.cpp`: a planted break beside
  a planted stale cause reports `broken` alone, and a record over the 16 KiB
  `kHostRecordCapBytes` names reports `broken`, the schema file stating no
  size, which is why the reader owns that check. Every failed check answers
  that verdict through that one branch, a schema that cannot be read among
  them, the read error the `readTextFile` call at the head of
  `checkReport` in `source/harness/json_load.cpp` shows. The `.json` entry
  cannot be opened and the record root that holds no `.json` entry stand
  planted in T011 and printed in T040, one owner each, so this task adds no
  FR-029 arm of its own. A record whose evidence list stands empty
  passes the guard, answers no stale reason and status 0, the shape the lane
  host's
  committed record carries. Each planted root holds the schema
  copy and the mapfile copy the reader resolves under the root (FR-015,
  FR-018, FR-029, FR-030, FR-041, SC-003, SC-006) (wave 3, M-C)
  Recorded in `results/ci-run.md`: "both rebuilt trees at 4552ac0, and had
  passed there at fcaf65d too".

### Implementation for User Story 2

- [x] T025 [US2] Settle the verdict order in
  `source/harness/host_record_check.cpp`: `broken` wins, the size cap answers
  that verdict among the failed checks, and the staleness rules run only over
  a record that passed the guard. An absent root and a root holding no
  `.json` entry both answer `RecordCheck` empty through the one listing path,
  and T044 widens that same channel to the unreadable mapfile. The lines and
  the status stand in T026 beside the entry point body, since a helper of
  this file reaches no test
  (FR-029, FR-030, R-07) (depends on T013, T023, T024;
  the tests turn green) (M-C)
  Recorded in `results/quickstart.md`: "broken host-record: /machine: the key
  'mask' stands missing".
- [x] T026 [P] [US2] Write the thin reader program in a new
  `tools/hostcheck/host_record_check.cpp`, whose `main` parses nothing: it
  hands `argc` and `argv` to `hostRecordCheckMain` and maps the answered
  status to the process exit. That entry point parses `--root` and
  `--records`, resolves `treeRoot / kHostRecordSchemaRelPath`, so a run
  reads `<root>/docs/schema/host-record-v1.json`, and refuses there with one
  line naming that path when it cannot be read, before it calls anything.
  It asks the `DirectoryChooserFactory` it is given for a chooser over that
  root and passes the resolved schema path and that chooser to
  `checkRecords`, which owns the listing
  (`contracts/record-check.md:214-228`). The program supplies the real
  factory: `makeDirectoryChooser(root)`, defined in
  `tools/hostcheck/mapfile.hpp` beside `sg::tools::mapfileText`, the header
  both programs share, opens
  `<root>/external/pmu-events/arch/x86/mapfile.csv` at the first question a
  record asks and reads those rows once. Rows the matcher cannot interpret
  answer `DirectoryAnswer::UNREADABLE_MAPFILE`, the answer an unopenable
  mapfile already gives, the catch standing in that header where T043 puts it
  for both programs. That builder stands in the program,
  because R-04 keeps a counters internal include out of the harness library,
  and the injection copies the writer's `HostFactsSeam`, so the entry point
  stands reachable by a planted factory in the tests of T040 and T044. The
  entry point owns the mapping
  from the `RecordCheck` of T013 to the lines and the status: an answer with
  neither readings nor `refusal` prints the line FR-029 names for the record
  root and answers status 2, a `refusal`, the mapfile or an entry that cannot
  be opened among them, prints its own line and answers status 2, and
  anything else prints one line per reading, that line naming the path, the
  CPU type, the verdict word FR-030 names and the detail. The reading line
  reads as `contracts/record-check.md:151-184` spells it,
  `recordRoot/<cpuType>.json  <cpuType>  <verdict>[  <detail>]`. The three
  run-level sentences stand in `contracts/record-check.md:242-261`, each alone
  on standard error with status 2: the schema sentence for a tree root that
  holds no schema, the run sentence for the path a refusal names, the mapfile
  behind `UNREADABLE_MAPFILE` or a `.json` entry the reader cannot open, and
  the root sentence for a record root that holds no `.json` entry. Over a
  set whose records all pass it answers status 0 while reading no `/proc` or
  `/sys` path, the run T032 registers as the committed-set entry. No `--schema`
  option joins either CLI: the root answers for both paths
  (FR-025, FR-026, FR-027, FR-028, FR-029, FR-030, R-06) (M-D)
  Recorded in `results/quickstart.md`: "validation/GenuineIntel-6-C6-2.json
  GenuineIntel-6-C6-2 valid".
- [x] T027 Build the reader: an `add_executable` line for
  `host_record_check` in `test/CMakeLists.txt` beside the writer's, the
  shape of the `quill_dependency_check` target. The committed-set entry
  waits for
  T032, because a record root holding no `.json` entry answers status 2
  and a registration written here would fail `ctest` on every runner
  until the lane commits its record (FR-026, FR-028, R-11) (depends on
  T020, T026) (M-E)
  Recorded in `results/quickstart.md`: "[100%] Built target host_record_check".
- [x] T028 [P] Write the target script in a new `cmake/validate.cmake`: run
  `host_record_write --root <VALIDATE_ROOT> --out <VALIDATE_OUT>`, then
  `host_record_check --root <VALIDATE_ROOT> --records <VALIDATE_OUT>`,
  stop at the first nonzero status, and read `VALIDATE_ROOT`,
  `VALIDATE_OUT` and `VALIDATE_BIN` with the defaults of
  [contracts/validate-target.md](./contracts/validate-target.md), the
  status mapping of `cmake/prose-lint.cmake:40` (FR-002, FR-003, FR-005,
  R-10) (M-E)
  Recorded in `results/quickstart.md`: "the clone holds
  validation/GenuineIntel-6-C6-2.json at 315 bytes".
- [x] T029 Add `add_custom_target(validate ...)` to `cmake/dev-mode.cmake`
  beside the utility targets that file includes above its `BUILD_TESTING`
  guard: it depends on `host_record_write` and `host_record_check`, runs the
  script through `${CMAKE_COMMAND} -P`, reads the `VALIDATE_OUT` cache variable
  that defaults to `${CMAKE_SOURCE_DIR}/validation`, and stands outside `ALL`,
  filed under `UtilityTargets` by `cmake/folders.cmake`; a failing step fails
  the target; it stands the output root with `file(MAKE_DIRECTORY ...)`, the
  shape `cmake/dbc-gate.cmake:21` sets, before it invokes the writer, and the
  writer creates nothing beyond the record file it names (FR-001, FR-002,
  FR-011, R-10) (depends on T027, T028) (M-E)
  Recorded in `results/quickstart.md`: "exit 0, the writer line beside the
  record path".
- [x] T030 [US1] Run `cmake --preset=ci-rocky` with
  `-D VALIDATE_OUT=$PWD/build/validation`, run
  `cmake --build build --target validate` twice, and record the commands,
  the record line, the line per committed record, the two exit statuses
  and the evidence list the second run left standing in
  `specs/031-host-validation/results/quickstart.md`
  (FR-001, FR-002, FR-006, FR-035, SC-001, SC-002) (wave 5, M-F) (depends
  on T029)
  Recorded in `results/quickstart.md`: "Run 2, exit 0, its log line for line as
  run 1's".
- [x] T031 [US1] Run `cmake --build build --target validate` with the
  default output root, and commit the record the lane host writes as
  `validation/GenuineIntel-6-C6-2.json` with its evidence list standing
  empty as [results/plan.md](./results/plan.md) records (FR-001, FR-009,
  FR-041, SC-006) (wave 5, M-F) (depends on T030)
  Recorded in `results/plan.md`: "committed by run 3 as
  validation/GenuineIntel-6-C6-2.json".
- [x] T032 [US2] Register the committed-set entry and turn it green: the
  `add_test(NAME host_record_check ...)` line for
  `test/CMakeLists.txt`, over `${CMAKE_SOURCE_DIR}` and
  `${CMAKE_SOURCE_DIR}/validation`, the registration shape of
  `add_test(NAME example_drift ...)`, which hands `${CMAKE_SOURCE_DIR}` over
  as an absolute argument the same way. No `WORKING_DIRECTORY` stands in
  the line and none is wanted: an entry runs in the directory of the test
  binary, `build/test`, and not one of the 144 entries of
  `test/CMakeLists.txt` sets that property, so a repo-relative pair of
  arguments would resolve beside the binary, where the tree holds neither
  the record root nor the schema, and the entry would answer for files it
  never meant to read. Absolute arguments settle the run in any starting
  directory, and CMake raises nothing over a missing property. So `test`,
  `test-rocky`, `coverage`, `sanitize` and `tsan` validate the committed
  records from files alone, with no hardware, no token and no workflow
  edit; then
  `ctest --test-dir build -R host_record_check --output-on-failure` reads
  `validation/` alone, prints one `valid` line and exits 0, and the run
  stands recorded in `specs/031-host-validation/results/quickstart.md`.
  The entry lands here, after T031 commits the record it reads, so no
  commit of the branch leaves `ctest` red
  (FR-024, FR-026, FR-027, FR-029, FR-037, SC-006) (wave 5, M-F)
  (depends on T027, T031)
  Recorded in `results/quickstart.md`: "both arguments stand absolute in the
  registration, so the entry passed".

**Checkpoint**: the MVP stands complete. The target runs the write, then
the check, one record stands written, and the reader keeps it true on any
host with no hardware read.

---

## Phase 5: User Story 3 - The acceptance rows travel with the record (Priority: P2)

**Goal**: each acceptance row rides the record of the CPU type that ran
it, naming path, command, exit status and cited line, a row binds when
its record is written, and each waiting row stands named as a gap in the
results record.

**Independent Test**: a planted record with three evidence entries reads
back with all three, the reader confirms each cited path and each cited
line, and deleting one cited file makes that entry the stale cause the
reader names ([spec.md](./spec.md) US3).

- [x] T033 [US3] Extend the evidence coverage of
  `test/source/harness_host_record_check_test.cpp` over the row shape the
  results table names: three rows carry path, command, exit status and
  cited line through one write and one read, and the reader answers no
  stale reason for the set. Coverage extension over behaviour T021 and
  T023 leave green, so this task adds no arm to observe red, and the behaviour
  it characterizes landed ahead of it: `982d98ff` planted the three-row shape
  over one write, `65c21e7d` gave the write path the standing read that keeps
  those rows, and `f4b647de` put the reader over its record root, so this arm
  answers green the first time it runs and no red run stands behind it
  (FR-015, FR-025, SC-003) (wave 3, M-C) (depends on T021)
  Recorded in `results/ci-run.md`: "the six entries of this feature passed in
  both rebuilt trees at 4552ac0".
- [x] T034 [US3] Name the rows the lane cannot carry in
  `specs/031-host-validation/results/plan.md`, each bound to the CPU type
  whose host ran it: the `--hunt=all` run cited from
  `specs/029-hunter-core/results/gary-hunt-all.md` for the Zen 5 desktop
  type, and the `--hunt=all` run with its top 20 cited from
  `specs/029-hunter-core/results/corsairone-hunt-all.md` and
  `specs/029-hunter-core/results/corsairone-hunt-all-top20.txt` for the
  Intel desktop type that stands down. Each row carries its command, exit
  status, cited line and the record it waits on, because a row binds when
  its host writes, and the lane commits its own record with an empty
  evidence list (FR-016, FR-041, SC-011) (M-F)
  Recorded in `results/plan.md`: "for the 20 ranked rows", the citation
  `:173-200` of `specs/029-hunter-core/results/gary-hunt-all.md`.
- [x] T035 [US3] Add the placement rows and the pre-check rows to the gap
  table of `specs/031-host-validation/results/plan.md`: the runs of
  `specs/035-numa-placement/results/` and
  `specs/034-host-noise-precheck/results/quickstart.md`, each named with
  the host whose run it waits on, under the same reading as the hunter
  rows, and one line stating that a CPU type owning no acceptance row
  carries none (FR-016, FR-017, SC-011) (depends on T034) (M-F)
  Recorded in `results/plan.md`: "./build/example/docs_topology_placement
  `--topology` ".
- [x] T036 [US3] Confirm each cited path and each cited line of those rows
  against the tree and record the file and line number beside each row in
  `specs/031-host-validation/results/plan.md`, so a row authored later
  passes the reader's second and third rules on its first read (FR-015,
  FR-025) (depends on T035) (M-F)
  Recorded in `results/plan.md`: "Every path above opened and every line above
  read as its row states it".
- [x] T037 [US3] Extend the reader's coverage with the record the lane
  commits: the planted mirror of `validation/GenuineIntel-6-C6-2.json`,
  its evidence list empty, reads `valid` with no stale reason across the
  four rules, over a planted root that holds the schema copy and the
  mapfile copy beside it. Coverage extension over the arm T024 plants and
  T025 turns green, so this task adds no arm to observe red (FR-015,
  FR-041, SC-006)
  (wave 3, M-C) (depends on T024)
  Recorded in `results/taskset.md`: "135 harness_host_record_check_test", the
  entry that runs `committedRecordMirrorReadsValid`.

**Checkpoint**: the row shape stands pinned, the gaps stand named, an
empty list reads valid, and the two extension tasks add coverage over
behaviour that stands green from Phase 4.

---

## Phase 6: User Story 4 - An unnamed host refuses (Priority: P2)

**Goal**: every failed read and every unusable argument prints one line on
standard error, exits 2 and leaves no bytes behind.

**Independent Test**: with a planted identity the catalog selects nothing
for, and with the layout read made to fail, the writing entry point prints
one line, exits 2 and leaves the output root empty; a malformed argument
on each new invocation path does the same before any host read
([spec.md](./spec.md) US4).

### Tests for User Story 4 (write first, observe failure)

- [x] T038 [US4] Write the failing usage-arm and identity-refusal tests in
  `test/source/harness_host_record_test.cpp`: run through
  `hostRecordWriteMain(argc, argv, facts)` with a planted `facts`, an
  unknown option, a missing value and an extra
  argument each print one line naming the argument it read and answer
  status 2 before the seam is asked, and a run that names no `--out` answers
  the default output root of FR-002, `<root>/validation`, with one record
  standing there; an empty vendor, an unset family, an unset model, an
  identity whose `directory` stands empty and an identity whose `directory`
  names a catalog directory the planted root does not hold each print one line
  naming the identity read, answer status 2 and leave the output root empty;
  the directory arm names that directory beside the identity, and the planted
  `facts` answers it the way a mapfile row names it, so the arm needs no
  mapfile of its own (FR-002, FR-004, FR-010, FR-032, SC-004)
  (wave 4, M-D)
  Recorded in `results/quickstart.md`: "two entries pass: the unit that plants
  an identity the catalog cannot name".
- [x] T039 [US4] Write the failing read-failure tests in
  `test/source/harness_host_record_test.cpp`: `installTestLoadFailure` of
  `source/harness/detail/internal.hpp` stands for the layout read, a
  read-only output root stands for the failed write, and an unreadable
  standing record keeps its bytes; each case prints one line naming the
  read, answers status 2 and leaves no partial file (FR-011, SC-004)
  (wave 4, M-D)
  Recorded in `results/quickstart.md`: "a layout read made to fail and an output
  root that refuses the write".
- [x] T040 [US4] Write the failing argument and refusal tests in
  `test/source/harness_host_record_check_test.cpp`: run through
  `hostRecordCheckMain`, an unknown option, a missing value and an extra
  argument each print one line on standard error and answer status 2 before
  the factory stands asked and before any record reads. Rule 4 stands planted
  through `staleReasons`, whose injected `DirectoryChooser` answers
  `DirectoryAnswer::NO_ROW` for the identity that rule names. Every printed
  line of a refused run belongs to the entry point, which owns resolution
  and mapping and takes its chooser from the `DirectoryChooserFactory` it
  stands given. Six trees stand planted, each
  holding the schema copy the test reads from the file `${SG_SCHEMA_PATH}`
  names and writes to
  `<root>/docs/schema/host-record-v1.json`, save the two named below that
  hold none. A tree with no mapfile, its record and schema standing, answers
  one line naming the mapfile: the factory T044 plants answers
  `DirectoryAnswer::UNREADABLE_MAPFILE` at the first question and
  `checkRecords` sets `refusal`. A tree with no schema
  file answers one line naming `<root>/docs/schema/host-record-v1.json`, the
  entry point refusing before it calls anything. A tree with neither file
  answers that schema refusal, the arm that demonstrates the tie-break of
  `contracts/record-check.md:206-209`. Two trees hold both files and name a
  record root, one standing absent and one standing present with no `.json`
  entry; each answers the empty listing T011 pins and prints one line naming
  that root. A sixth tree holds the schema and one `.json` entry the reader
  cannot open, and answers one line naming that entry: `checkRecords` sets
  `refusal` to it before the chooser stands asked, so that run opens no
  mapfile either. Every refused tree answers status 2 with no per-record
  line,
  `checkRecords` prints nothing in any of them, and no run with nothing to
  read opens the mapfile, because the chooser answers only per record. Each
  printed line stands pinned in the shape T026 states, the three `error:`
  lines naming the schema path, the refusal path and the record root, and the
  per-record line three columns wide where no detail stands and four where one
  does. Part of these arms characterize behaviour that landed ahead of them:
  `f4b647de` put the mapping from a `RecordCheck` to the lines and the status in
  the library beside the schema refusal and planted its own arms with it, and
  `3f8c3156` landed the mapfile refusal beside the chooser's
  `DirectoryAnswer::UNREADABLE_MAPFILE` answer, both before these trees stood
  planted, so the arms those two cover answer green the first time they run and
  no red run stands behind them
  (FR-004, FR-020, FR-025, FR-029, SC-004) (wave 4, M-C)
  Recorded in `results/quickstart.md`: "error: the record root '/tmp/no-records'
  holds no record".

### Implementation for User Story 4

- [x] T041 [US4] Implement the argument arms and the writer refusals in
  `source/harness/host_record.cpp`: the parse of
  `hostRecordWriteMain(argc, argv, facts)` refuses an unknown option, a
  missing value and an extra argument before it asks `facts`, and a run that
  names no `--out` takes `<root>/validation` as the default output root of
  FR-002 in that same parse. Each refusal prints one `fprintf` line on
  standard error in the shape of the `error: the report file` refusal of
  `source/harness/json_report.cpp` and
  answers the status the thin `main` of T019 maps to exit 2, and the whole
  text renders in memory before the one write, so no failed path leaves
  bytes behind (FR-004, FR-010, FR-011, FR-037, R-09) (depends on T038,
  T039; the tests turn green) (M-D)
  Recorded in `results/quickstart.md`:
  `error: the option '--out' stands without its value`.
- [x] T042 [US4] Implement the argument arms of `hostRecordCheckMain` in
  `source/harness/host_record_check.cpp`: the parse follows the standing
  option rules and answers its status before any chooser question is asked,
  so an unknown option, a missing value and an extra argument ask no
  `DirectoryChoice`; the refusal line names the option it read, and the
  harness option table gains no row (FR-004, FR-028, R-09) (depends on T040;
  the tests turn green) (M-D)
  Recorded in `results/quickstart.md`:
  `error: the option '--root' stands without its value`.
- [x] T043 [US4] Implement the host reads of `tools/hostcheck/host_record_write.cpp`:
  an `external/pmu-events/arch/x86/mapfile.csv` the root does not hold
  leaves the directory unanswered, and the program prints one line, exits 2
  and writes no file for that identity, for an identity whose `directory`
  stands empty, for an identity whose `directory` names a catalog directory
  the root does not hold, and for a layout that answers unreadable. That
  named directory is checked beside the empty arm, because the reader
  compares the field nowhere and this program is the only place that can
  catch it. Rows the matcher cannot interpret answer the same unreadable
  directory: `sg::tools::mapfileText` and the matcher call stand together in
  the shared `tools/hostcheck/mapfile.hpp`, so `std::regex_error` is caught
  there and answers that unreadable mapfile, which is where T026 places the
  reader's half. A throw that reaches `main` is no refusal this feature
  speaks, so the program's exit stays the mapped status 2. T038 and T039 run
  those arms through `hostRecordWriteMain(argc, argv, facts)`, which answers
  the status this main maps, with a planted identity and planted machine
  facts; the mapfile-open arm of the program itself, and the arm planting
  rows the matcher cannot interpret, run on the lane and stand recorded by
  T050 in `specs/031-host-validation/results/quickstart.md`, because a
  program invocation past its argument parse reads CPUID and no test binary
  reaches a file-local seam (T045 states the entry shape) (FR-004, FR-010,
  FR-011) (depends on T019, T041) (M-D)
  Recorded in `results/quickstart.md`: "the mapfile names no directory, and no
  file stands under the output root".
- [x] T044 [US4] Settle the unreadable-mapfile refusal, its library half in
  `source/harness/host_record_check.cpp` and its arm in
  `test/source/harness_host_record_check_test.cpp`: a chooser answering
  `DirectoryAnswer::UNREADABLE_MAPFILE` makes `checkRecords` set `refusal` to
  that mapfile path and leaves `readings` empty, and `hostRecordCheckMain`
  prints one refusal line naming the mapfile and answers status 2 before any
  per-record line, because rule 4 stands unanswered for every record
  (`contracts/record-check.md:128-149`). The arm plants a
  `DirectoryChooserFactory` answering that choice, the seam T026 opens
  reaching the entry point the way `HostFactsSeam` reaches the writer, so the
  refusal stands asserted as an answer. T040 keeps the planted tree that
  holds no mapfile for its printed line, and injected lambdas still answer
  the per-record cases in the `staleReasons` tests of T022. The chooser the
  program's factory builds stands in that program, R-04 keeping its counters
  include out of the library. The program's own arm runs on the lane and
  stands recorded by T050 under the scenario "The tree that holds no mapfile"
  (FR-004, FR-025) (depends on T026, T042) (M-D)
  Recorded in `results/quickstart.md`: "one refusal line, error: the record run
  cannot read".
- [x] T045 [US4] Register the writer's CTest entry in
  `test/CMakeLists.txt`: one `add_test(NAME host_record_write ...)` line
  that runs one malformed-argument invocation and declares
  `PASS_REGULAR_EXPRESSION` with the refusal line it must print, the shape
  `counters_trap_checked_rejects_unknown_mode` sets; the file holds no
  `WILL_FAIL` row anywhere. The pattern pins the printed line, because
  CMake reads matching output as a pass whatever status the process exits
  with, and the failing status belongs to the refusal path T038 asserts
  in-process. The entry stays one entry, and the suite
  holds five: three test units and the two program entries. The usage arms
  of FR-004 and the refusal arms of FR-010 and FR-011 stand asserted
  in-process by T038 and T039 through
  `hostRecordWriteMain(argc, argv, facts)`, which answers a status the test
  reads (`contracts/validate-target.md:116-129` states that split), and the
  mapfile-open refusal cannot join the entry, because a
  program invocation past its argument parse reads CPUID on the runner
  (FR-035);
  it stands recorded as a lane run beside T050's record
  (FR-004, FR-035, FR-037) (depends on T020, T043) (M-E)
  Recorded in `results/quickstart.md`: "the entry that pins the writer's refusal
  line of the malformed invocation".

**Checkpoint**: the usage arms and the refusal arms run through
`hostRecordWriteMain(argc, argv, facts)` in `harness_host_record_test` with
planted values and through `hostRecordCheckMain` in
`harness_host_record_check_test`, the
parse of each entry point answers its refusal line before any host read,
the writer's single entry pins that line in `test`, `test-rocky`,
`coverage`, `sanitize` and `tsan`, and the mapfile-open arm of each program
stands recorded as a lane run: the writer's beside T050's record, the
reader's under the scenario "The tree that holds no mapfile". The reader's
refusal for that arm stands asserted in-process, planted by T040's
`DirectoryAnswer::UNREADABLE_MAPFILE` answer, and T040 plants the schema
refusal and both empty-listing lines, the absent root and the root holding
no `.json` entry, the same way.

---

## Phase 7: User Story 5 - The docs teach the target (Priority: P3)

**Goal**: a reader finds the target, the reader command and the two exit
statuses in 25 lines or fewer on the root page, and finds the record path
shape and the staleness rules on the build page.

**Independent Test**: the root-page addition counts 25 lines or fewer, it
names the target, the reader and both exit statuses, the build-page
section names the record path shape and the staleness rules, and the docs
gates pass over both pages ([spec.md](./spec.md) US5).

- [x] T046 [P] [US5] Add region `#validate` to `example/quickstart.sh` in
  the marker form of `example/quickstart.sh:27-33`: the
  `cmake --build build --target validate` command and the reader command
  `./build/test/host_record_check --root . --records validation` (FR-036,
  R-13) (M-G)
  Recorded in `results/quickstart.md`: "the shell block of the new region stands
  outside the emitted set".
- [x] T047 [US5] Add the root-page block to `README.md` inside
  `## Quick start` (`README.md:18`): one purpose sentence, one block marked
  `<!-- sg-example: example/quickstart.sh#validate -->`, one result
  sentence naming both exits, and the 25-line discipline of FR-036; no row
  joins the `Name` or the `Option` table. `PAGE_LINE_CAP` and `README.md`
  stand equal before the block: `master` brought both to 964, and this
  feature adds 10 lines on top, so the same commit moves
  `PAGE_LINE_CAP` in `tools/docs/example_drift.py` to the measured 974 and
  adds the dated note beside it that names the FR and the
  arithmetic, the shape of the dated notes above that constant. This task
  holds two files,
  `README.md` and `tools/docs/example_drift.py`, the cap and its note
  standing one fact with the block (FR-036, FR-040, IV.1, SC-010)
  (depends on T046) (M-G)
  Recorded in `results/quickstart.md`: "no finding, the page standing 974 lines
  against the cap of 974 at `32dffcbb"`.
- [x] T048 [P] [US5] Add `### Validating the host record` under
  `## Quality gates` in `docs/pages/building.md` (`docs/pages/building.md:251`):
  one purpose sentence, one command block and one result sentence covering
  the record path shape `validation/<key>.json`, the four staleness rules
  and both exit statuses, in 25 lines or fewer (FR-024, FR-027, FR-036)
  (M-G)
  Recorded in `results/quickstart.md`:
  "counts 25 lines under `## Quality gates`".
- [x] T049 [US5] Run the docs gates over the two pages and clear every
  finding: `example_drift`, `example_drift_fixtures`, `snippet_compile`,
  `example_run`, `report_shape`. `example_drift` reads `PAGE_LINE_CAP` and
  the per-section budgets, so the moved cap and the dated note of T047
  stand checked here
  (FR-036, SC-010) (depends on T047, T048) (M-G)
  Recorded in `results/quickstart.md`: "every gate above read clean on its first
  run at the head named".
- [x] T050 [US5] Run every scenario of [quickstart.md](./quickstart.md)
  and record each command, its exit status and its observation in
  `specs/031-host-validation/results/quickstart.md`, in the
  `| Scenario | Command | Exit | Observation |` shape, the planted mapfile
  arm, the tree that holds no mapfile, the library refusal arms and the
  usage arms of both entry points included. Each row names what the run's
  root holds, because the reader resolves the schema inside the root it is
  given. Three scenarios point `--root` at the repository and read the
  schema there: the planted break, the record root with no record, and the
  planted stale record. Two plant a tree root and copy
  `docs/schema/host-record-v1.json` into it beside the record: the planted
  mapfile that names no row, which also plants that mapfile, and the tree
  that holds no mapfile, which plants every file but the mapfile. No
  scenario plants a root without the schema, since such a root answers the
  schema refusal and the row would record the wrong reason
  (FR-026, FR-035, FR-041, SC-001 to SC-007) (M-F) (depends on T030, T032,
  T045)
  Recorded in `results/quickstart.md`: "six entries pass in 0.02 s, one process
  each".
- [x] T051 [US5] Read both pages against the shipped surface: the commands
  name `build/test/host_record_check`, the target reads `validate`, the
  section counts 25 lines or fewer, and neither page names a host, a
  person or a public name (FR-036, FR-040, FR-042) (depends on T047,
  T049) (M-G)
  Recorded in `results/quickstart.md`: "Every status the two pages state is one
  of these readings".

**Checkpoint**: the reader path and the code stand one fact, and the docs
gates hold over both pages.

---

## Phase 8: Polish and gates

**Purpose**: the hard gates of Principle VIII over the tree this feature
lands in, and the records that outlive the branch.

- [x] T055 Run `cmake --preset=ci-ubuntu` and `cmake --build build`
  once, the same `build` tree the release preset configured with the
  analyzer launchers added, and clear at source, in
  `source/harness/host_record.cpp`,
  `source/harness/host_record_check.cpp`,
  `source/harness/detail/host_record.hpp`,
  `source/harness/detail/host_record_check.hpp`,
  `tools/hostcheck/host_record_write.cpp` and
  `tools/hostcheck/host_record_check.cpp`, every clang-tidy and cppcheck
  finding that can fail the CI gate. The shard step counts the findings
  whose check stands on the `WarningsAsErrors` line of `.clang-tidy` and
  fails when that count rises above zero, so that line is the bar. The
  advisory families stay at master's baseline under the ruling recorded in
  `specs/031-host-validation/results/ci-run.md`, and no suppression marker
  joins a file of this feature. The clear runs first, because every task
  that measures those six files reads what this one leaves
  (constitution IX, FR-039, SC-012) (wave 8, M-H)
  Recorded in `results/ci-run.md`: "Three findings went away during T055, and
  none of them went away behind a suppression".
- [x] T059 Run `format-check`, `spell-check`, the `prose-lint` target
  and the `simdjson_purity_scan` entry over the tree the feature lands
  in, and clear every finding in the six files T055 names, the three
  units under `test/source/`, `docs/schema/host-record-v1.json`,
  `example/quickstart.sh`, `README.md` and
  `docs/pages/building.md`. It runs beside T055, ahead of the measured
  gates, for the same reason (FR-022, FR-042, SC-012) (wave 8, M-H)
  (depends on T055)
  Recorded in `results/quickstart.md`: "The six files T055 names stand clean
  under every content gate".
- [x] T052 [P] Run the coverage gate over the four files this feature
  adds under `source/`: `cmake --preset=ci-coverage` into
  `build/coverage`, then `tools/dbc/coverage_gate.sh`, which reports
  lines at 100.0% over the measured roots and any branch shortfall
  inside the standing allowance. The figure 21 stands at the `allowance`
  row of `tools/dbc/coverage_gate.sh`: the 19 accepted classes of the 030
  round, whose rule stands at `specs/030-ci-speedup/spec.md:377-380`
  (D-14), plus the 2 rows this feature recorded beside them, and the rows
  themselves are the zero-taken branch positions of the trace, which the
  script prints from its awk row when a shortfall passes the allowance.
  The ledger holds those 21 rows;
  `source/harness/host_record.cpp`, `source/harness/host_record_check.cpp`,
  `source/harness/detail/host_record.hpp` and
  `source/harness/detail/host_record_check.hpp` read 100.0% lines and
  100.0% branches; the extract roots of the `COVERAGE_EXTRACT_COMMAND` row
  of `cmake/coverage.cmake`
  hold the four files; the two `main` functions stay outside the extract
  by the standing globs and carry their CTest entries as their evidence;
  and no `LCOV_EXCL` marker and no new `--ignore-errors` entry joins this
  change, while no allowance row leaves the gate. The markers that stand in
  other files today, `source/counters/clock_provider.cpp` and
  `source/counters/detail/pmu.hpp` among them, stay where they stand
  (FR-037, SC-008) (wave 8, M-H) (depends on T055, T059)
  Recorded in `results/ci-run.md`: "lines.......: 100.0% (14160 of 14160
  lines)".
- [x] T053 Report the keep-set reading in
  `specs/031-host-validation/results/ci-run.md`: which patterns of
  `cmake/coverage.cmake` and which of the 19 allowance classes of
  `specs/030-ci-speedup/spec.md:377-380` no longer match code, read off
  the zero-taken branch positions of the coverage trace named as the
  evidence, with the removal question asked and left unanswered
  (FR-038) (depends on T052) (M-H)
  Recorded in `results/ci-run.md`: "Every pattern in cmake/coverage.cmake still
  matches".
- [x] T054 [P] Run the `ci-sanitize` and `ci-tsan` builds in
  `build/sanitize` and `build/tsan` and the suite under each; ASan,
  UBSan and TSan report nothing over `source/harness/host_record.cpp`,
  `source/harness/host_record_check.cpp`,
  `tools/hostcheck/host_record_write.cpp` and
  `tools/hostcheck/host_record_check.cpp` (FR-039, SC-012) (wave 8, M-H)
  (depends on T055, T059)
  Recorded in `results/ci-run.md`: "No report of any family appears in any log
  of this round".
- [x] T056 [P] Run `test/time_source_gate.sh` and the construct scans
  over the added files: the scan stays green with `tools/hostcheck/` in
  its scanned set, no `SG_*` macro carries a lambda, each contract-only
  helper carries `[[maybe_unused]]`, no new test sleeps, reads a clock or
  names a fixed cpu number, and no added source file under `source/` or
  `tools/` opens an absolute `/proc` or `/sys` path outside
  `procRootPath` (FR-031, FR-033, FR-034, FR-039) (wave 8, M-H)
  (depends on T055, T059)
  Recorded in `results/taskset.md`: "no entry of it sleeps, reads a clock or
  names a fixed cpu number".
- [x] T057 [P] Scan every file this feature adds for a host name and a
  person name, and leave each cited evidence path spelled as the
  repository spells it across `specs/029-hunter-core/results/`,
  `specs/035-numa-placement/results/` and
  `specs/034-host-noise-precheck/results/`; the only files it edits are
  the records under `specs/031-host-validation/results/` (FR-042)
  (depends on T055, T059) (M-H)
  Recorded in `results/plan.md`: "The shipped surface answers nothing".
- [x] T058 Run the whole suite under `taskset -c 0,1`, record the run, the
  entry list and the reading in
  `specs/031-host-validation/results/taskset.md`, and confirm each new read
  resolves under a planted root or an installed layout, that a masked run
  takes an output root under the build directory, and that no new
  file-root seam stands beside `source/harness/detail/proc_root.hpp`
  (FR-031, FR-034, FR-035, SC-009) (wave 6, M-H)
  Recorded in `results/taskset.md`: "reading 2 ran the same command at `fa68c1d`
  over 151 entries".
- [ ] T060 Read the pushed CI run end to end and record the verdict in
  `specs/031-host-validation/results/ci-run.md`: `lint`, `test`,
  `test-rocky`, `sanitize`, `tsan`, `static-analysis`, `coverage`,
  `dbc-gate`, `prose-lint`, `shared-audit`, `consumer-release`,
  `downstream-consumer` and `docs` green, the `VERSION` row and the two
  `SOVERSION` rows of `CMakeLists.txt` unmoved, `kSchemaVersion` unmoved in
  `source/harness/json_report.cpp`, and no option row and no `Name` row
  joined a table (FR-040, SC-012) (depends on T053, T054, T056, T058) (M-H)
  Open: run 41127479202 answers four checks broken. That run tested the head
  the branch held before the replay onto `e3368112`, so the head it measured is
  no longer a citation the branch carries, and the run id is what a reader can
  reach. `lint`,
  `prose-lint`, `test`, `sanitize`, `tsan`, `shared-audit` and
  `downstream-consumer` report green, and `static-analysis` reports green on
  all three of its shards; `coverage`, `dbc-gate`, `consumer-release` and
  `test-rocky` are broken. `coverage` prints `branch shortfall 29 rows exceeds the  D-14
  recorded allowance of 21` beside `100.0% (15035 of 15035 lines)`; `dbc-gate`
  prints `pair-gate: 1 of 319 interfaces drifted` naming
  `sg::speedgunMain: enforced-not-documented assertion`, which answers the
  same over a checkout of the base; `consumer-release` prints 14 errors under
  `-Werror` with the contract macros off, thirteen of them inside
  `source/harness/host_record_check.cpp`; `test-rocky` prints `99% tests
  passed, 2 tests failed out of 158`, and the two entries seal a plant with a
  mode in a container that reads its user as root. `docs` reports nothing and the run
  lists it as `skipping`, because its condition names a push to `master`. Its build step stands
  recorded in `specs/031-host-validation/results/ci-run.md` under "The docs
  build, 2026-10-30, run by `align-3` at `7976b15`": exit 0, cmake 4.4.5,
  Doxygen 1.18.0, and the pinned m.css archive fetched over the network. Its
  gate family reports on this run through `lint` and `test`, and that family
  stands recorded in `specs/031-host-validation/results/quickstart.md` at its
  named head with each command and exit. The three page and version checks hold:
  `CMakeLists.txt` and `source/harness/json_report.cpp` stand untouched
  between the base and that head, so the `VERSION` row reads `0.27.0`, the
  harness `SOVERSION` row reads 18 and the archive `SOVERSION` row reads 4 as
  the base leaves them, `kSchemaVersion` reads `1.10`, and the root page
  gained no table row.
  Since that run the branch replays onto `e3368112`. Over a checkout of that
  base the pair gate prints `pair-gate: 319 interfaces, 0 gaps` and exits 0,
  and the assertion `dbc-gate` named stands nowhere in
  `source/harness/cli.cpp` there, so none of that drift is left to report. The
  two plants behind the `test-rocky` failures now stand as a directory where
  the reader owes a refusal, which binds the container's uid 0 (`c50ceb7c`,
  `b8b92f8c`). `consumer-release` keeps its bar count marked maybe-unused, the
  last error that stood (`12d735ab`). Seven rows of the `coverage` shortfall
  came from the branch commit the replay dropped, whose ordering the new base
  writes with plain comparisons, as `77513d35` records in
  `specs/031-host-validation/results/ci-run.md`. What closes these two rows is
  the run over the replayed branch reading every named check green,
  `dbc-gate` and `coverage` among them.
- [x] T061 Check each commit of the branch against the commit template:
  `<Section>: <one-line imperative>` at 50 characters or fewer, a body
  wrapped at 72 columns, `Refs: specs/031-host-validation`, `Fixes #n`
  where one stands open, `Approved-by:` present, and `prose-lint` green
  over the range (Pull Request Quality, XI)
  Recorded in `results/ci-run.md` under "The commit template over the
  branch": "range holds 218 commits", "0 messages lack the row" for
  both the `Refs` row and the `Approved-by:` footer, "answered no open
  issue" for the `Fixes` row, and `0 findings, 0 skipped` from the prose
  gate over the same range.

**Checkpoint**: every hard gate of Principle VIII stands green over the
release preset build, and the results records carry the evidence.

---

## Dependencies & Execution Order

### Story order

```text
Phase 1 ──► Phase 2 ──► Phase 3 (US1) ──► Phase 4 (US2) ──┬─► Phase 5 (US3) ─┐
                                                          ├─► Phase 6 (US4) ─┤
                                                          └─► Phase 7 (US5) ─┴─► Phase 8
```

The `depends on` clauses drive the order, the numbering groups work by
story, so a clause can name a higher number: T001 follows T002, and T052,
T054, T056 and T057 follow the two clears T055 and T059.

### Module graph (plan.md)

```text
M-A ──► { M-B  M-C  M-G } ──► { M-D  M-E } ──► M-F ──► M-H
        (parallel)             (parallel)
```

- Phase 1 holds M-A: T001 to T005. T002 writes the first failing unit,
  T001 registers it so the red run builds, and T003 turns it green. T004
  and T005 carry types and contracts alone, so no test stands ahead of
  them.
- Phase 2 holds the M-B and M-C groundwork: T006 and T007 stand two red
  runs in two files and register those two units as they write them,
  T008 and T009 answer them, and T010 to T012 carry the bytes, the lines
  and the exit shape, with T011 and T013 standing behind T023.
- Phase 3 holds the M-B write path (T014 to T018) and the writer program
  (T019, T020). The `validate` target stands in Phase 4, because its
  `DEPENDS` names both programs and M-E stands after M-D in the module
  graph.
- Phase 4 holds the M-C staleness rules (T021 to T025), the reader
  program (T026, T027), the M-E target (T028, T029) and the M-F runs that
  first fill `validation/` (T030 to T032). The staleness wave runs first in
  the module, because `checkRecords` answers staleness through
  `staleReasons`, so T023 stands before T011 lists and T013 prints verdicts:
  one test binary links only when every symbol its arms reference exist.
  T027 builds
  the reader, and the committed-set entry waits for T032, because a record
  root with no
  `.json` entry answers status 2 and an entry registered before T031
  commits its record would leave that commit red (FR-029).
- Phase 5 (M-F) and Phase 6 (M-D, M-E) stand after the Phase 4
  checkpoint and run beside each other: the results records touch no
  source file the refusal tasks edit.
- Phase 7 (M-G) stands after the target name and the binary path settle
  in Phase 4, because the pages quote both.
- Phase 8 (M-H) stands last. T055 and T059 clear findings in the six
  added source files first, and T052, T053, T054, T056, T057, T058, T060
  and T061 then read the tree every earlier phase landed, so no measured
  file moves while a parallel run reads it.

### Within each story

- The test task stands written and observed failing before the code task
  turns it green (TDD, plan.md Execution mode).
- The machine block before the renderer (T008 before T012); the renderer
  before the write path (T012 before T018); the write path before the
  program that calls it (T018 before T019).
- The guard before the staleness rules (T009 before T023); the staleness
  rules before the verdict order (T023 before T025); the reader program
  before the entry that runs it (T026 before T027).
- The refusal tests before the refusal code (T038 to T040 before T041 to
  T042); the library refusals before the program refusals (T041 before
  T043).
- Each `add_executable` stands in the task that writes the source it
  compiles (T001 after T002, T006 and T007 with their own units, T020 and
  T027 beside their programs), and an entry stands after the record or the
  arm it runs (T032 after T031, T045 after T043).
- The finding clears of Phase 8 (T055, T059) stand before the gates that
  measure the same files (T052, T054, T056).

### Parallel Opportunities

- Phase 1: T002, T004 and T005 stand three independent tasks over three
  files; T001 follows T002, because a registration needs the source its
  target compiles.
- Phase 2: T006 opens `test/CMakeLists.txt` for the phase and T007
  appends its line after it, so the two units gain one writer at a time
  each; then T008 with T009, then T010, then T012 on its own, since T011 and
  T013 both wait for T023. Each pair names two different files.
- Phase 3, Phase 4 and Phase 7: T017 with T019, T026 with T028, and T046
  with T048.
- Phase 8: T052, T054, T056 and T057 stand four independent gate runs
  once T055 and T059 have cleared the files they read.
- The `[P]` mark stands on the first task in a phase that opens a file.
  A later task in that file carries no mark and appends in task order, so
  one file holds one writer at a time. A task bound to a dependency it
  must follow carries no mark either. The five files that hold the tests,
  the two library files, the two programs and
  `test/CMakeLists.txt` each follow that rule.

---

## Parallel Execution Examples

### Phase 1 and Phase 2

```text
At once: T002 test/source/harness_host_record_schema_test.cpp
At once: T004 source/harness/detail/host_record.hpp
At once: T005 source/harness/detail/host_record_check.hpp
Then:    T001 test/CMakeLists.txt (registers T002's unit, the red run)
Then:    T003 docs/schema/host-record-v1.json (closes T002)

At once: T006 machine-block tests (harness_host_record_test.cpp) plus
         its registration in test/CMakeLists.txt
Then:    T007 read tests (harness_host_record_check_test.cpp) plus its
         registration in test/CMakeLists.txt
At once: T008 hostMachineFromLayout    (source/harness/host_record.cpp)
At once: T009 readHostRecord           (source/harness/host_record_check.cpp)
```

### US1 (Phase 3)

```text
One file, in task order: T014, T015, T016 (harness_host_record_test.cpp)
At once:                 T017 (harness_host_record_schema_test.cpp)
Then:                    T018 (source/harness/host_record.cpp)
At once:                 T019 (tools/hostcheck/host_record_write.cpp)
Then:                    T020 (test/CMakeLists.txt)
```

### US2 (Phase 4)

```text
One file, in task order: T021, T022, T024 (harness_host_record_check_test.cpp)
Then:                    T023, then T025 (source/harness/host_record_check.cpp)
At once:                 T026 (tools/hostcheck/host_record_check.cpp)
At once:                 T028 (cmake/validate.cmake)
Then:                    T027 (the reader's add_executable) and
                         T029 (cmake/dev-mode.cmake)
Then:                    T030, T031 (the runs, then the committed record)
Then:                    T032 (the committed-set entry, then the run
                         recorded in results/quickstart.md)
```

### US3 and US4 (Phases 5 and 6, two workers)

```text
Worker A: T034, T035, T036 (specs/031-host-validation/results/plan.md),
          then the two extension tasks T033 and T037 (check tests)
Worker B: T038, T039, T040, then T041, T042, T043, T044, then T045
```

### US5 (Phase 7)

```text
At once: T046 (example/quickstart.sh), T048 (docs/pages/building.md)
Then:    T047 (README.md), T049 (the docs gates), T050 (results/quickstart.md)
Then:    T051 (the read-through)
```

### Phase 8

```text
First:   T055 (the analyzer clear), then T059 (format, spell, prose, purity)
At once: T052 (coverage), T054 (sanitizers), T056 (scans), T057 (name scan)
Then:    T053 (keep-set reading), T058 (the masked run)
Then:    T060 (the CI reading), T061 (the commit template)
```

---

## Implementation Strategy

### MVP First (foundation, US1, US2)

1. Phase 1 and Phase 2: the schema, the two headers and the two
   skeletons (T001 to T013, with T011 and T013 behind T023).
2. Phase 3: the write path and the writer program (T014 to T020).
3. Phase 4: the staleness rules, the reader program, the `validate`
   target and the first record (T021 to T032).
4. **Stop and validate**: `cmake --build build --target validate` writes
   one record, and
   `ctest --test-dir build -R host_record_check --output-on-failure`
   reads the committed set on any host.

### Incremental Delivery

1. Foundation plus US1 plus US2: the MVP above.
2. US3: the acceptance rows and the gap records (T033 to T037).
3. US4: the usage arms and the refusal arms through both entry points, and
   the writer's CTest entry (T038 to T045).
4. US5: the two pages, the region and the quickstart record (T046 to
   T051).
5. Phase 8: the gate sweep and the CI reading (T052 to T061), the full CI
   matrix.

### Parallel Team Strategy

With several workers: one takes the writer lane (T006, T008, T010, T012,
T014 to T020), one the reader lane (T007, T009, T011, T013, T021 to T027),
one the docs lane once T046 and T048 open (T046 to T051), and one the
build surface once both programs build (T028, T029, T045). Each unit
registers in the task that writes its source, so a lane appends its own
`test/CMakeLists.txt` line in task order. The results records (T030,
T031, T034 to T037, T050, T053, T058, T060) stand one worker after the
Phase 4 checkpoint. The two lanes meet at T017, where the
guard test reads the bytes the writer lane produced, and
`host_record_check.cpp` holds one writer at a time through T021 to T025.

---

## Notes

- No task adds a sleep, a clock read, a fixed cpu number, an `LCOV_EXCL`
  marker, a new `--ignore-errors` entry, a lambda inside an `SG_*` macro,
  or a second JSON reader. Contract-only helpers carry
  `[[maybe_unused]]`. No host name and no person name stands authored into
  a repository file (FR-033, FR-034, FR-037 to FR-039, FR-042).
- No public name joins `include/speedgun-ng/`, no option row joins the
  harness option table, and no version, `SOVERSION` or report-schema value
  moves (the `VERSION` row and the two `SOVERSION` rows of
  `CMakeLists.txt`, and `kSchemaVersion` in
  `source/harness/json_report.cpp`) (FR-004, FR-040).
- The counters library stays the only CPUID reader and the only mapfile
  matcher: the writer's seam calls `pmuIdentCurrent`, `mapfileKey` and
  `pmuSelectDirectory`, and the program's `makeDirectoryChooser`, handed to
  the reader entry point as its `DirectoryChooserFactory`, returns a chooser
  that opens the mapfile of the parsed root once, at the first question a
  record asks,
  through the stream overload, and no library entry point reads a host
  (FR-007, FR-008, FR-025, R-04, R-06).
- Both programs and the `validate` target exist in developer mode only,
  because `add_subdirectory(test)` stands at `cmake/dev-mode.cmake:5`. The
  binaries stand at `build/test/host_record_write` and
  `build/test/host_record_check`.
- The directory question answers for the mapfile under the root the run
  names, `<root>/external/pmu-events/arch/x86/mapfile.csv`, through
  `pmuSelectDirectory(std::istream&, const PmuIdent&)`
  (`source/counters/detail/pmu.hpp:99`,
  `source/counters/linux_pmu/table_parse.cpp:695`) on the write side and
  the read side alike, so a planted mapfile drives the mapping on any
  host; the cached overload (`source/counters/linux_pmu/table_parse.cpp:747`)
  opens no path, and plan R-06 states that reason once (FR-008, FR-025,
  AC-1). `DirectoryAnswer::FOUND` answers that this ident selects a
  directory in this tree, and the reader takes the answer only as the
  negation of rule 4. It compares nothing, so a record whose stored
  `cpu.directory` no longer matches the tree reads valid: the routing moved,
  and the record states what was true when the writer wrote it, while a
  reviewer reads the record's own identity fields against the tree
  (FR-008, FR-025).
- The default output root resolves against the tree root the run names, so a
  writer run without `--out` stands its record at `<root>/validation`. The
  reader takes that same root beside its record root, so one tree answers
  both programs whatever directory started the run, and a default read from
  the working directory would let one build write into two trees (FR-002,
  FR-028).
- The schema travels with the root: the reader resolves
  `<root>/docs/schema/host-record-v1.json` through the constant
  `kHostRecordSchemaRelPath`, the `k` prefix an exported `inline constexpr
  std::string_view` in a `detail/` header carries, as `kDefaultCompareRule`
  stands in `source/harness/detail/compare.hpp`. No `--schema` option
  joins either CLI, so every caller passes an absolute root, the
  committed-set entry passes `${CMAKE_SOURCE_DIR}` and
  `${CMAKE_SOURCE_DIR}/validation` and needs no `WORKING_DIRECTORY`, and a
  planted root copies the schema beside the record and the mapfile. A test
  that plants a root reads the schema from the file `${SG_SCHEMA_PATH}`
  names, the absolute define `harness_json_report_test` states behind the
  `#ifndef SG_SCHEMA_PATH` compile error of
  `test/source/harness_hunt_json_test.cpp:57-58`, and writes those bytes
  into the planted tree, because a repo-relative path cannot reach inside a
  root the test builds.
- One owner per decision: `checkRecords` owns the listing, reading the
  `.json` entries through the error-code form of
  `std::filesystem::directory_iterator`
  (`source/counters/linux_pmu/provider.cpp:361`), so a record root that
  stands absent and one that holds no `.json` entry share one path and
  answer `RecordCheck { {}, {} }`, and it prints nothing.
  `hostRecordCheckMain` owns resolution and mapping: parse, resolve
  `treeRoot / kHostRecordSchemaRelPath`, refuse an unreadable schema file
  with one line naming it, ask the injected factory for the chooser over the
  parsed root, call `checkRecords`, then map, a `refusal`
  printing its line and answering 2, an answer with neither readings nor
  `refusal` printing the FR-029 line naming the record root and answering
  2, and any other answer printing one line per reading. The chooser answers
  only per record and opens the mapfile at its first question, so a run with
  nothing to read prints the empty-root line, while the quickstart's
  no-mapfile scenario plants a record and prints the mapfile line. Every
  run-level printed refusal therefore stands asserted through
  `hostRecordCheckMain` (FR-004, FR-020, FR-025, FR-029). A root that holds
  neither the schema nor the mapfile answers the schema refusal, because
  resolution comes first.
- Two refusals answer a host the artifacts cannot name. A mapfile whose rows
  the matcher cannot interpret is the unreadable-mapfile answer on both sides,
  caught once in `tools/hostcheck/mapfile.hpp` so no `std::regex_error`
  reaches `main`, and a catalog directory the identity names that the root
  does not hold is refused by the writer alone, beside its empty-directory
  arms, because no reader compares that field (FR-004, FR-010, FR-011, FR-025).
- A task closed only against an unoptimized build stays open: T055 stands
  the release-preset build every story needs before its tasks stand done
  (constitution IX).
- Every commit carries the footer T061 checks, and `prose-lint` holds over
  each one.

---

## Requirement coverage

Every requirement of [spec.md](./spec.md) names the tasks that close
it.

- **FR-001**: T029, T030, T031
- **FR-002**: T018, T028, T029, T030, T038, T041
- **FR-003**: T014, T020, T028
- **FR-004**: T004, T019, T038, T040, T041, T042, T043, T044, T045
- **FR-005**: T014, T018, T028
  T014 keeps the row, and its eight dropped arms return there as one
  characterization pass over the behaviour `8597f680` landed: the success line
  naming the CPU type and the path, an error stream that stays empty, and the
  seam asked once. Nothing else on the row speaks for the streams: the byte
  unit, the cap refusals, the refresh scenario and the CTest entry read the
  file or a refusal line.
- **FR-006**: T015, T018, T030
- **FR-007**: T004, T014, T019
- **FR-008**: T004, T014, T019
- **FR-009**: T014, T018, T031
- **FR-010**: T038, T041, T043, T045
- **FR-011**: T029, T039, T041, T043, T045
- **FR-012**: T004, T006, T008, T019
- **FR-013**: T006, T008
- **FR-014**: T006, T008, T019
- **FR-015**: T004, T016, T021, T024, T033, T036, T037
- **FR-016**: T034, T035
- **FR-017**: T035
- **FR-018**: T010, T012, T016, T024
- **FR-019**: T002, T003, T004
- **FR-020**: T002, T003, T007, T009, T012, T040
- **FR-021**: T001, T002, T003, T017
- **FR-022**: T009, T059
- **FR-023**: T010, T012, T017, T018
- **FR-024**: T001, T005, T007, T009, T011, T013, T032, T048
- **FR-025**: T005, T021, T022, T023, T026, T033, T036, T040, T044
- **FR-026**: T005, T011, T013, T023, T026, T027, T032, T050
- **FR-027**: T005, T011, T013, T026, T032, T048
- **FR-028**: T005, T022, T026, T027, T042
- **FR-029**: T005, T011, T013, T024, T025, T026, T032, T040
- **FR-030**: T005, T007, T009, T024, T025, T026
- **FR-031**: T056, T058
- **FR-032**: T004, T006, T014, T038
- **FR-033**: T010, T056
- **FR-034**: T056, T058
- **FR-035**: T001, T022, T030, T045, T050, T058
- **FR-036**: T046, T047, T048, T049, T051
- **FR-037**: T001, T004, T018, T019, T020, T032, T041, T045, T052
- **FR-038**: T053
- **FR-039**: T054, T055, T056, T062
- **FR-040**: T047, T051, T060, T063
- **FR-041**: T024, T031, T034, T037, T050
- **FR-042**: T051, T057, T059, T065

- **SC-001**: T006, T014, T030, T050
- **SC-002**: T015, T030, T050
- **SC-003**: T007, T011, T013, T021, T022, T024, T033, T050
- **SC-004**: T038, T039, T040, T050
- **SC-005**: T002, T010, T017, T050
- **SC-006**: T024, T031, T032, T037, T050
- **SC-007**: T010, T016, T050
- **SC-008**: T052
- **SC-009**: T058
- **SC-010**: T047, T049
- **SC-011**: T034, T035
- **SC-012**: T054, T055, T059, T060, T062, T063

The commit shape of Pull Request Quality (constitution XI) closes the same
way, and no requirement carries it:

- **XI**: T061

Each decision of [spec.md](./spec.md) closes through the
requirements that carry it, and the tasks that close those:

- D-1 (the record is data and the schema stands checked in; FR-001, FR-009,
  FR-020, FR-021): T001, T002, T003, T007, T009, T012, T014, T017, T018, T029,
  T030, T031, T040
- D-2 (the target writes no benchmark results; FR-003, FR-015, FR-018): T004,
  T010, T012, T014, T016, T020, T021, T024, T028, T033, T036, T037
- D-3 (the check is a reader; FR-024, FR-026, FR-027): T001, T005, T007, T009,
  T011, T013, T023, T026, T027, T032, T048, T050
- D-4 (the identity comes from the counters library; FR-007, FR-008): T004,
  T014, T019
- D-5 (the refusal shape; FR-004, FR-010, FR-011): T004, T019, T029, T038, T039,
  T040, T041, T042, T043, T044, T045
- D-6 (the docs carry the target; FR-004, FR-040): T004, T019, T038, T040, T041,
  T042, T043, T044, T045, T047, T051, T060, T063
- D-7 (the record identity and the tree layout; FR-006, FR-009, FR-016,
  FR-024, FR-042): T001, T005, T007, T009, T011, T013, T014, T015, T018, T030,
  T031, T032, T034, T035, T048, T051, T057, T059, T065
- D-8 (freshness reads content; FR-025, FR-033): T005, T010, T021, T022, T023,
  T026, T033, T036, T040, T044, T056
- D-9 (the seams are the standing ones; FR-031, FR-032, FR-035): T001, T004,
  T006, T014, T022, T030, T038, T045, T050, T056, T058
- D-10 (an empty record set stands a failure; FR-029): T005, T011, T013,
  T024, T025, T026, T032, T040

## Phase 9: Convergence

Appended by `/speckit.converge` on 2026-10-30 at `7c5fbc8e`. Each row is work
the tree and the records do not carry yet. Blocked rows are not repeated here:
the three CPU types waiting on their hosts, the `NPS2` and `NPS4` boots, the
two-cpu arm of SC-009, and the two host entries `counters_linux_pmu_seam_test`
and `harness_cli_test` stand recorded with their reasons in
`specs/031-host-validation/results/plan.md`, `results/taskset.md` and
`results/ci-run.md`.

- [x] T062 Run `cmake --preset=ci-ubuntu` and `cmake --build build`, clear
  at source every clang-tidy and cppcheck finding that can fail the CI gate
  in the six files T055 names, that row stating the bar, and record the
  reading, the toolchain and the cleared count in
  `specs/031-host-validation/results/ci-run.md` under "The static-analyzer
  reading" beside the sanitizer readings. That record states the ruling
  under which the advisory families stay at master's baseline, and that no
  file of this feature carries a suppression marker, per FR-039 and SC-012
  (wave 9, M-H) (depends on T055)
  Recorded in `results/ci-run.md` under "The static-analyzer reading": "No
  finding stood at error severity, so the gate check reads clear across the
  feature".
- [ ] T063 Once the branch is pushed and the Actions run closes, read that run
  end to end and record in `specs/031-host-validation/results/ci-run.md` the
  verdict of `lint`, `test`, `test-rocky`, `sanitize`, `tsan`,
  `static-analysis`, `coverage`, `dbc-gate`, `prose-lint`, `shared-audit`,
  `consumer-release`, `downstream-consumer` and `docs`, beside the standing
  `CMakeLists.txt` and `source/harness/json_report.cpp` lines and the absence
  of a new option or `Name` row, per FR-040 and SC-012 (`missing`) (wave 9,
  M-H) (depends on T060, T062). Read at this head: run 41127479202 closed at
  conclusion `failure` with every check reported, eight of the thirteen job
  names green, four broken, and `docs` skipped by its own condition. The run
  these rows wait on now is the one over the replayed branch, and the row
  stays open until it reports every named check green, because a verdict row
  cannot tick over a broken check.
- [x] T064 Tick each task whose deliverable the tree shows and whose reading
  the four records carry, and leave open only the tasks this phase names, so
  the checklist states what the records state, per Pull Request Quality and
  constitution IX (wave 9, M-B) (depends on T062, T063)
  Recorded here: 63 of the 65 rows carry their record and reading, and the
  two open rows name what they wait on.
- [x] T065 Re-run the name scan over the landing tree and update the name-scan
  section of `specs/031-host-validation/results/plan.md`, which still states
  one account-pattern hit inside `specs/031-host-validation/tasks.md` that
  `b8553882` removed when the root became `$PWD`, per FR-042 (wave
  9, M-B) (depends on T057)
  Recorded in `results/plan.md`: the scan re-ran at `d2c95fa` over 28 added
  files and 324 added lines.
