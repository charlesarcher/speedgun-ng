# Tasks: JSON Reports

**Input**: Design documents from `specs/023-json-reports/`

**Prerequisites**: [spec.md](spec.md), [plan.md](plan.md),
[research.md](research.md), [data-model.md](data-model.md),
[contracts/](contracts/), [quickstart.md](quickstart.md)

**Tests**: every capability carries a test, because Principle VI sets 100%
line and 100% branch coverage as a hard gate (FR-045, SC-008).

**Organization**: phases 3 to 8 follow the user stories of the spec, so each
story is independently testable.

**Revision**: written by `/speckit.tasks` on 2026-10-20; the
`/speckit.analyze` pass of the same day added T012 (the read-side key-order
test of FR-009) and T024 (the loader precondition of the spec's
contract-boundary constraint), and the list was renumbered to keep the IDs
in execution order.

## Path Conventions

Harness sources `source/harness/`, public headers `include/speedgun-ng/`,
tests `test/source/`, docs examples `example/docs/`, gates `test/` and
`tools/`, schema `docs/schema/`.

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: the types, the schema file and the build registration every
later phase reads.

- [X] T001 Add `RawSetting`, `RowRaw` and `ReportData` to `include/speedgun-ng/benchmark.hpp` per `data-model.md` E-01 and E-02 (FR-018, FR-022, D-07, D-08)
- [X] T002 Add `jsonPath` and `jsonRaw` to `RunOptions` in `source/harness/detail/internal.hpp`, and declare `writeReport` and `checkReport` in `source/harness/detail/json_report.hpp` per `contracts/report-writer.md` and `contracts/schema-file.md` (FR-008, FR-028, FR-029)
- [X] T003 [P] Write `docs/schema/report-v1.json` as a JSON Schema 2020-12 document with the key order of `data-model.md` E-03 to E-11 and the enum lists of E-06 (FR-007, FR-008, D-05)
- [X] T004 [P] Add `PRIVATE $<BUILD_INTERFACE:simdjson::simdjson>` to the harness target in `CMakeLists.txt` and create the empty `source/harness/json_report.cpp` with the license header (FR-042, FR-047, FR-048, D-01, D-02)

**Checkpoint**: the types compile, the new unit builds against the vendored
headers, and no behavior exists yet.

---

## Phase 2: Foundation (Blocking Prerequisites)

**Purpose**: the writer and the reader themselves. Every story reads them.

**CRITICAL**: no story work begins before this phase is complete.

- [X] T005 [P] Add the six enum name tables of `data-model.md` E-06 to `source/harness/json_report.cpp`, each with the `static_assert` pairing of the last enumerator that `source/harness/report.cpp:35` uses (FR-012, FR-013, FR-014, FR-016, D-06)
- [X] T006 [P] Write the document tests in `test/source/harness_json_report_test.cpp`: the top-level key order, the two header fields against the compile definitions, and the absence of any key naming a date, a time, a host or a pid, on the fake provider with no sleep and no clock read (FR-001, FR-005, FR-007, FR-046, SC-001)
- [X] T007 Implement the scalar and cell writers in `source/harness/json_report.cpp` through `simdjson::builder::string_builder`: the number path, the escaping path, the integer fields, and the `{"available": .., "value": ..}` cell (FR-011, FR-015, FR-021, D-03, D-12)
- [X] T008 Implement the row writer in `source/harness/json_report.cpp` in the key order of E-05, with the nullable fields the row leaves empty (FR-013, FR-015)
- [X] T009 Implement the benchmark writer in `source/harness/json_report.cpp` in the key order of E-04, including the labels, the counter statistics, the statistic values, the context records and the noise warning (FR-012, FR-014, FR-016, FR-017)
- [X] T010 Implement the document writer and the file write in `source/harness/json_report.cpp`: the header object, the `benchmarks` array, one write, and the false answer with the one-line error naming the path (FR-001, FR-002, FR-030, FR-033, D-09)
- [X] T011 [P] Write the loader tests in `test/source/harness_json_load_test.cpp`: the error forms of `contracts/report-loader.md`, each naming its JSON pointer, and the driver that answers status 2 for a loader error, over planted files with no sleep and no clock read (FR-023, FR-024, FR-027, FR-046, SC-005)
- [X] T012 [P] Add the read-side key-order test to `test/source/harness_json_load_test.cpp`: a hand-written document whose keys stand in a different order loads to the same value (FR-009)
- [X] T013 Implement the DOM read skeleton in `source/harness/json_report.cpp`: one parse, the `schemaVersion` major gate, the unknown-key skip and the pointer-named parse error (FR-007, FR-009, FR-010, FR-023, D-04)
- [X] T014 Implement the benchmark and row readers in `source/harness/json_report.cpp`, answering the public result types field for field (FR-012, FR-013, FR-014, FR-016, FR-022)
- [X] T015 Add `loadReport` to `include/speedgun-ng/benchmark.hpp` with the contract of `contracts/report-loader.md`, its `\pre` on a non-empty path, and its definition in `source/harness/json_report.cpp` (FR-022, FR-044, D-08)

**Checkpoint**: one unit writes a document and reads one back; nothing is
wired to the command line yet.

---

## Phase 3: User Story 1 - Get one machine-readable file out of a run (Priority: P1) MVP

**Goal**: `--json=<path>` leaves one validating document with every
benchmark of the run, and the console keeps its bytes.

**Independent Test**: `harness_json_report_test` and
`harness_json_schema_test` over a scripted suite, plus the file the
quickstart prints.

### Tests for User Story 1

- [X] T016 [P] [US1] Add the field-equality tests to `test/source/harness_json_report_test.cpp`: every field of one written benchmark equal to the same run's `BenchmarkResult`, over a scripted `FakeProvider` suite (FR-003, SC-001)
- [X] T017 [P] [US1] Add the schema test to `test/source/harness_json_schema_test.cpp`: a written document against `docs/schema/report-v1.json`, and the five planted disagreements of `contracts/schema-file.md` (FR-008, D-05)

### Implementation for User Story 1

- [X] T018 [US1] Add `--json=<path>` to the option table and the path parser in `source/harness/cli.cpp`, with the one-line usage error and `std::exit(2)` on an empty path (FR-028, FR-030)
- [X] T019 [US1] Collect the batch's `ReportData` in `source/harness/runner.cpp` and `source/harness/detail/internal.hpp` beside the results the console prints (FR-002, FR-018, D-07)
- [X] T020 [US1] Call `writeReport` once after the last fit row of the batch in `speedgunMain` of `source/harness/cli.cpp`, and write no file when the batch produced no result (FR-001, FR-006, FR-033, D-09)
- [X] T021 [P] [US1] Add the console-unchanged test to `test/source/harness_json_report_test.cpp`: the console bytes of a run with `--json=` equal to the same run without it (FR-003, FR-031, SC-003)

**Checkpoint**: one option, one file, one schema check, console bytes
unchanged.

---

## Phase 4: User Story 2 - Load the file back (Priority: P1)

**Goal**: `loadReport` answers the run's own values, and a loaded value
writes back byte for byte.

**Independent Test**: `harness_json_load_test` over files the writer
produced and over planted broken files.

### Tests for User Story 2

- [X] T022 [P] [US2] Add the round-trip tests to `test/source/harness_json_load_test.cpp`: write, load, write, byte compare, for the raw setting at `none` and at `chunks` (FR-011, FR-026, SC-002)
- [X] T023 [P] [US2] Add the loaded-value equality tests to `test/source/harness_json_load_test.cpp`: each loaded result equal field by field to the run that wrote it, including the percentile cells, the statistics and the context records (FR-022, SC-010)

### Implementation for User Story 2

- [X] T024 [US2] Add the loader's `\pre` guard on an empty path and its nullable-field reads in `source/harness/json_report.cpp`, so a `null` loads to a value whose availability is false (FR-021, FR-025)
- [X] T025 [US2] Implement the raw column reader and the `ReportData` assembly in `source/harness/json_report.cpp` (FR-018, FR-022, E-01, E-02)
- [X] T026 [P] [US2] Add the version tests to `test/source/harness_json_load_test.cpp`: `1.0` and `1.7` accepted, `2.0` refused with the version named, and a missing or non-string version refused (FR-010, FR-023)

**Checkpoint**: the format closes on itself; a file is a value and a value
is a file.

---

## Phase 5: User Story 3 - Keep the chunk columns (Priority: P2)

**Goal**: `--json-raw=chunks` carries the chunk values of every row and
context, including a run that printed no percentile line.

**Independent Test**: `harness_capture_test` on a scripted run with the
percentile output off, and the `raw2.json` command of the quickstart.

- [X] T027 [P] [US3] Add the retention tests to `test/source/harness_capture_test.cpp`: the chunk columns of a finished run stay readable in the `ReportData`, one per reported column, each holding `chunkCount` values in the percentile column order (FR-018, FR-019, SC-010)
- [X] T028 [US3] Keep `ContextFolds::chunkValues` and the single-context chunk values in the runner's `ReportData` in `source/harness/runner.cpp` (FR-018, D-07)
- [X] T029 [US3] Add `--json-raw=` to the option table and the value-list parser in `source/harness/cli.cpp`, with the exit-2 forms of `contracts/report-options.md` (FR-029, FR-030, SC-004)
- [X] T030 [P] [US3] Widen the capture gate in `source/harness/runner.cpp` to read the percentile output or the raw setting, and add its test to `test/source/harness_capture_test.cpp`: `--json-raw=chunks` with the percentile output off captures, writes the `raw` key and prints no percentile line (FR-020, FR-035, SC-006)

**Checkpoint**: the residue N1 left becomes a column of the file.

---

## Phase 6: User Story 4 - Trust the run's exit status (Priority: P2)

**Goal**: the option errors, the unwritable path and the failed benchmark
each answer the status the contract names.

**Independent Test**: `harness_cli_test` drives the forms.

- [X] T031 [P] [US4] Add the status tests to `test/source/harness_cli_test.cpp`: no option writes no file, a failed write answers 2, a failed write with a failed benchmark answers 2, an unwritable path prints the console report first, and an option error answers 2 with no benchmark run (FR-004, FR-006, FR-030, SC-004)
- [X] T032 [US4] Set the status to 2 on a false `writeReport` answer in `speedgunMain` of `source/harness/cli.cpp` (FR-004, FR-030, D-10)
- [X] T033 [US4] Extend the status postcondition in `source/harness/cli.cpp` with the report-failure case of `contracts/report-options.md` (FR-050, D-10)
- [X] T034 [US4] Add the write-failure row to the exit-status table of `specs/015-benchmark-harness-core/contracts/cli.md` (FR-050)
- [X] T035 [P] [US4] Add the interrupt test to `test/source/harness_cli_test.cpp`: an interrupted batch still writes the file, and the status stays the one the interrupt earns (FR-001, FR-004)

**Checkpoint**: the status table of 015 and the entry point agree, and the
report failure is visible.

---

## Phase 7: User Story 5 - Diff two runs of the same suite (Priority: P3)

**Goal**: two runs of one scripted suite write byte-identical files, and no
allocation stands in a timed window.

**Independent Test**: the `a.json` and `b.json` commands of the quickstart,
and the allocation counter.

- [X] T036 [P] [US5] Add the byte-identity test to `test/source/harness_json_report_test.cpp`: two runs of one scripted suite with the same options write byte-identical files (FR-009, FR-011, FR-046, SC-009)
- [X] T037 [US5] Add the no-volatile-key test to `test/source/harness_json_report_test.cpp`: no key of the document names a date, a time, a host or a pid, and the header carries only the two compile-definition values (FR-005, FR-021, D-13)
- [X] T038 [P] [US5] Add the window test to `test/source/harness_capture_test.cpp`: the counting allocation operator reads zero across a timed window with the raw setting at `chunks`, and no I/O stands in that window (FR-032, SC-006)

**Checkpoint**: the file is a function of the values, and the timed path
pays nothing.

---

## Phase 8: User Story 6 - Read the page, copy the block, run the example (Priority: P3)

**Goal**: the main page teaches the feature and the example runs in CI.

**Independent Test**: `python3 tools/docs/example_drift.py .` reports no
finding, and `run_docs_json_reports` runs the program that carries the
region.

- [X] T039 [P] [US6] Write `example/docs/json_reports.cpp` with its `sg-example:begin json_reports` region and `example/docs/json_reports.expected`, one family with a fixed iteration count so its rows stand still (FR-036, FR-038, D-15)
- [X] T040 [US6] Register `add_docs_example(json_reports)` in `example/CMakeLists.txt` and its `example_run` entry (FR-038)
- [X] T041 [US6] Add the `JSON reports` section to `README.md` at 25 lines or fewer, with the three quick-reference rows of `contracts/page-and-examples.md` (FR-036, FR-037, FR-039, SC-011)
- [X] T042 [P] [US6] Add the `JSON reports` detail to `docs/pages/harness.md`: the field list of the document, the version rule, the `null` rule, the raw column rule and the loader's error rule (FR-040)
- [X] T043 [US6] Move `PAGE_LINE_CAP` in `tools/docs/example_drift.py` by the measured amount, add `JSON reports` to `SECTIONS` and `SECTION_OWNERS`, and record the measurement in the gate comment (FR-037, FR-041, SC-011)

**Checkpoint**: the page, the region, the rows and the gate agree.

---

## Phase 9: Polish & Cross-Cutting Concerns

- [X] T044 [P] Take the next free minor version and move the harness `SOVERSION` from 7 to 8 in `CMakeLists.txt`, leaving the archive `SOVERSION` at 2 (FR-043, FR-044, D-16)
- [X] T045 [P] Run `cmake --preset=ci-sanitize` with its suite and `cmake --preset=ci-tsan` with its suite over every new path (SC-012)
- [X] T046 [P] Run `cmake --preset=ci-coverage`, `ctest --test-dir build/coverage`, and `cmake --build build/coverage -t coverage`, whose summary step runs `tools/dbc/coverage_gate.sh`; report 100% line and 100% branch over `source/harness/json_report.cpp` with no exclusion marker (FR-045, SC-008)
- [X] T047 [P] Run the `ci-ubuntu` build with clang-tidy and cppcheck clean, then `cmake --build build --target format-check` and `spell-check` (SC-012)
- [X] T048 [P] Run `bash test/time_source_gate.sh`, `bash test/loop_shape.sh build`, `bash test/cli_shape.sh`, `ctest --test-dir build -R 'simdjson_purity_scan|simdjson_nm_proof'`, and `cmake --build build -t dbc-gate` (FR-034, FR-035, FR-048, SC-007, SC-012)
- [X] T049 Run `python3 tools/prose/prose_gate.py --check all` over the
  branch's commits and the artifacts, then
  `ctest --test-dir build --output-on-failure`, rebase on `master` keeping
  both features' lines in the shared lists, and push once with
  `--force-with-lease`, showing `git ls-remote origin 023-json-reports`
  (FR-049, SC-012)

  All of it ran on 2026-10-20: the gate half closed at 100.0% line
  and 100.0% branch (the ledger below), the tree landed as five
  commits on `023-json-reports`, the branch rebased onto `master`
  at `7a75e95` when N2 took 0.12.0 first, and the branch pushed as
  a new remote ref; pull request #43 stands against `master`.

---

## Requirement Coverage Map

*The mechanical check that speckit-analyze re-derives: each row
names the tasks that cite the id, generated from the task lines
above on 2026-10-20. Counts: 50 FRs, 12 SCs, 49 tasks, 125 citations.*

| Requirement | Tasks |
|---|---|
| FR-001 | T006, T010, T020, T035 |
| FR-002 | T010, T019 |
| FR-003 | T016, T021 |
| FR-004 | T031, T032, T035 |
| FR-005 | T006, T037 |
| FR-006 | T020, T031 |
| FR-007 | T003, T006, T013 |
| FR-008 | T002, T003, T017 |
| FR-009 | T012, T013, T036 |
| FR-010 | T013, T026 |
| FR-011 | T007, T022, T036 |
| FR-012 | T005, T009, T014 |
| FR-013 | T005, T008, T014 |
| FR-014 | T005, T009, T014 |
| FR-015 | T007, T008 |
| FR-016 | T005, T009, T014 |
| FR-017 | T009 |
| FR-018 | T001, T019, T025, T027, T028 |
| FR-019 | T027 |
| FR-020 | T030 |
| FR-021 | T007, T024, T037 |
| FR-022 | T001, T014, T015, T023, T025 |
| FR-023 | T011, T013, T026 |
| FR-024 | T011 |
| FR-025 | T024 |
| FR-026 | T022 |
| FR-027 | T011 |
| FR-028 | T002, T018 |
| FR-029 | T002, T029 |
| FR-030 | T010, T018, T029, T031, T032 |
| FR-031 | T021 |
| FR-032 | T038 |
| FR-033 | T010, T020 |
| FR-034 | T048 |
| FR-035 | T030, T048 |
| FR-036 | T039, T041 |
| FR-037 | T041, T043 |
| FR-038 | T039, T040 |
| FR-039 | T041 |
| FR-040 | T042 |
| FR-041 | T043 |
| FR-042 | T004 |
| FR-043 | T044 |
| FR-044 | T015, T044 |
| FR-045 | T046 |
| FR-046 | T006, T011, T036 |
| FR-047 | T004 |
| FR-048 | T004, T048 |
| FR-049 | T049 |
| FR-050 | T033, T034 |
| SC-001 | T006, T016 |
| SC-002 | T022 |
| SC-003 | T021 |
| SC-004 | T029, T031 |
| SC-005 | T011 |
| SC-006 | T030, T038 |
| SC-007 | T048 |
| SC-008 | T046 |
| SC-009 | T036 |
| SC-010 | T023, T027 |
| SC-011 | T041, T043 |
| SC-012 | T045, T047, T048, T049 |

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1** has no dependencies.
- **Phase 2** depends on Phase 1 and blocks every story.
- **Phases 3 to 8** depend on Phase 2. US2 depends on US1's written file;
  US3 is independent of US1 and US2 in code and can land in parallel; US4
  depends on US1's write call and on US3's raw option; US5 is a test phase
  over what US1 and US3 already write; US6 lands last, because the section
  quotes the final document shape.
- **Phase 9** depends on every story the branch ships.

### Story Independence

- **US1** is the MVP: one option, one validating document, console bytes
  unchanged.
- **US2** reads that document back.
- **US3** adds one column family to it.
- **US4** closes the status contract around it.
- **US5** proves the format is a function of the values.
- **US6** documents the result.

### Within Each User Story

- Tests first, and they fail before the implementation lands.
- The writer before the CLI wiring, the CLI wiring before the status work.
- The runner retention before the raw option, the raw option before the
  widened gate.
- Story complete before the next priority.

### Parallel Opportunities

- T001 to T004, then T005, T006, T011 and T012 in parallel.
- Within each story, the test task runs in parallel with the implementation
  task of a different file.
- US3 (T027 to T030) runs in parallel with US2.
- Phase 9's T044 to T048 run in parallel.

---

## Parallel Example: User Story 1

```bash
# Tests alongside the writer work (different files):
Task: "Add the field-equality tests to test/source/harness_json_report_test.cpp"
Task: "Add the schema test to test/source/harness_json_schema_test.cpp"

# Then the wiring, which reads the writer:
Task: "Call writeReport once after the last fit row in speedgunMain"
```

---

## Implementation Strategy

### MVP First (User Story 1)

1. Complete Phase 1.
2. Complete Phase 2: the name tables, the scalar writers, the row and
   benchmark writers, the document write, the DOM reader and the public
   load call.
3. Complete Phase 3: the option, the `ReportData` collection, the one call,
   the schema test.
4. **Stop and validate**: `harness_json_report_test`,
   `harness_json_schema_test` and the quickstart file prove the document.

### Incremental Delivery

1. Phases 1 and 2 → the format stands on its own and the CLI writes nothing.
2. US1 → one file per run.
3. US2 → the file is a value again.
4. US3 → the chunk columns ride along.
5. US4 → the status contract closes.
6. US5 → the byte-identity and window proofs.
7. US6 → the page.

### Rebase Note for the Three Parallel Features

N2 on corsairone and N11 on thrawn run at the same time. The shared files are
the project version, the two `SOVERSION` lines, `README.md`, the
quick-reference table, `PAGE_LINE_CAP`, `example/CMakeLists.txt`, the
`example_run` list, and the `test/` target lists. Rebase, keep both
features' lines, retake the version per FR-043, re-measure the page budget,
run the full gate set, and push once with `--force-with-lease`. FR-049 bounds
the rest: no file N2 or N4 owns gets edited.

---

## Commit Plan

Each commit follows the constitution template: `<Section>: <one line>` at 50
characters or fewer, a why-body wrapped at 72, then one footer block with
`Refs: specs/023-json-reports`. `Approved-by: Charles` stands only on a
decision he made.

| Commit | Section | Tasks | Content |
|---|---|---|---|
| 1 | Harness | T001 to T004, T007 to T010 | The report types, the schema file, the link line, the writer |
| 2 | Harness | T005, T006, T011 to T015 | The name tables, the reader, the public load call and their tests |
| 3 | Harness | T016 to T021 | The option, the batch record, the one call, the schema and console tests |
| 4 | Harness | T022 to T026 | The round trip, the raw reader, the version rule |
| 5 | Harness | T027 to T038 | The retained columns, the raw option, the widened gate, the status work, the proofs |
| 6 | Docs | T039 to T043 | The JSON reports section, the example, the rows, the budget |
| 7 | CMake | T044 to T049 | The version, the harness `SOVERSION`, and the gate runs |

---

## Notes

- [P] marks different files with no dependency on an open task.
- The report stays off by default: no option, no file, and the run behaves
  as it does at `c7f1769` (FR-031, SC-003).
- The chunk value columns die with the attempt recorder today; keeping them
  in the runner's `ReportData` is the one storage decision of the feature
  (D-07).
- `source/harness/report.cpp` stands untouched. The noise warning fields the
  file carries come from the shared selector of `detail::selectRunnableCpuCount`,
  and the counting of spinning contexts stands in the new unit, because
  FR-042 and FR-049 keep the console printer out of this feature (D-11).
- Stop at any checkpoint and validate the story independently.

---

## Execution Ledger

Written by `/speckit.implement` on 2026-10-20. Every task above stands
checked except T049, whose gate half ran and whose commit, rebase and push
wait for the owner.

### Where the code answers differently from the artifacts

- E-01 nesting. `ReportData::rowRaw` carries one entry per benchmark and
  one inner entry per row, and `contextRaw` adds one entry per context of
  that row. The data model shows one `RowRaw` per benchmark. A family with
  several rows, and a threaded run whose every repetition captured its own
  chunks, need the extra level (T001, T019, T025, T028).
- Row-level context columns. The per-context percentile columns stand in
  `ResultRow::contextPercentiles`, and the per-context chunk columns in
  `contextRaw[i][k][j]`. The E-04 tree places the context record at the
  benchmark; the capture happens per repetition, so the answer stands at
  the row (T014, T023, T027).
- `cpuTimePerIterationNs` always writes a number. E-05 allows null for a
  manual run; the row field is a plain `double`, so a manual run writes
  `0.0` (T007).
- `pauseSamplesFolded` writes a bool. E-05 lists it as unsigned; the C++
  field at `benchmark.hpp:368` is a bool (T007).
- The schema file inlines each entity at its place.
  `contracts/schema-file.md` asks for one `$defs` entry per entity and a
  `$ref` at each use. The checker this feature ships reads no `$ref`, so
  the file carries its definitions in place and stays a JSON Schema
  2020-12 document (T003).
- The reader lives in `source/harness/json_load.cpp`. T013 to T015, T024
  and T025 name `json_report.cpp`. The format splits into a writer unit
  and a reader unit, and the harness links simdjson PRIVATE either way
  (T015).
- The quick-reference rows. `--json=` and `--json-raw=` sit in the Option
  table of the main page and the `loadReport` row sits in the Name table.
  `tools/docs/example_drift.py` admits only an option spelling to the
  Option table, so the third row moves (T041).
- SC-009 byte identity. The two-run test compares the two documents with
  the digits of `overheadFloorNs` pinned. That one number answers the
  machine: `source/harness/runner.cpp:1228` measures the sampling overhead
  of the attempt. A loaded machine moves it and every other byte stands
  still (T036).

### One machine fact worth carrying

`ci-sanitize` cannot compile `source/harness/cli.cpp` on this machine's GCC
16: libstdc++'s `<regex>` trips `-Werror=maybe-uninitialized` under `-O2
-fsanitize=address,undefined`. The same failure stands at the merge base
`c7f1769`, so it predates this feature. The sanitizer suite ran with that
one warning suppressed through a local `CMAKE_CXX_FLAGS` on the build
directory, and nothing in the tree changed for it.

### Verification wave, 2026-10-20 (second session)

A DAG of eight gate lanes re-ran every hard gate over the uncommitted
tree. The earlier checks of T045 to T048 predated the last edit to
`test/source/harness_json_report_test.cpp` (10:55), and the suite was
never re-run after it; the wave found what that edit left behind.

- Crash, fixed at the root. The rich fixture carried two metric columns
  on one row (`fitted.metrics`) beside one `metricLabels` entry, so
  `writeRow` read `result.metricLabels[1]` past the allocation at
  `json_report.cpp:268`: SIGABRT on the bounds assertion, an ASan
  heap-buffer-overflow, and a SIGSEGV through the dangling view under
  coverage. The fixture now carries one label per column. Four lanes
  named the same site independently.
- Reader, writer and schema disagreed on the row-level
  `contextPercentiles` shape. The writer lays down
  `[context][column][cell]`; the reader read one block per row and
  would have lost every context past the first of a threaded run; the
  schema carried neither key. The reader now reads the context level,
  `docs/schema/report-v1.json` carries `contextPercentiles` and
  `contextRaw` as optional row keys after `percentiles`, and E-05 of
  this data model gained the two rows. Three planted refusals
  (`kContextSpans`) guard the reader's new branches.
- The quickstart named `./build/example/docs/json_reports`; the binary
  is `./build/example/docs_json_reports`, the spelling
  `test/example_run.sh` uses. All eight command lines now name it.
- `format-check` failed on the two test files the wave touched;
  clang-format over them cleared it.

Re-proven green on the fixed tree: full ctest 99/99 in `build/`;
ASan/UBSan 99/99 and TSan 96/96; clang-tidy with 0 error-severity
diagnostics and 0 findings in the `WarningsAsErrors` set; cppcheck 0;
`format-check`; `cmake -P cmake/spell.cmake`; `example_drift.py`;
the prose tree gate at 452 sources and 72,240 units with 0 findings;
`dbc-gate` at 258 interfaces and 0 gaps; the purity scans inside
ctest; and the quickstart commands by hand (key order, 120 chunk
values beside a chunk count of 120, the three error forms at status 2,
byte-identical key sequences across two runs).

Coverage, first measurement of this feature to reach the gate (the 10:58
run died at a gcda stamp mismatch caused by the crash, so no figure had
ever been read): 99.4% line, 97.6% branch. The gaps were `cli.cpp:332`
(the `--json-raw=none` spelling, never exercised), the null
`metricStats` entry, the non-null `noiseWarning` branch and the
post-close `stream.fail()` branch in `json_report.cpp`, and 24 reader
lines in `json_load.cpp`. The closing tests landed, and the gate now
exits 0 at 100.0% line (5820 of 5820) and 100.0% branch (2459 of 2459):
`json_report.cpp` 342/342 lines and 85/85 branches, `json_load.cpp`
772/772 and 470/470, `cli.cpp` 195/195 and 122/122. Two reader
fixes rode along: a `try`/`catch (const std::system_error&)`
around the file read, because libstdc++ answers a read error by
throwing from the buffer and the old `stream.bad()` test could
never observe it (the `/proc/self/mem` refusal is now a test), and
the labels loop reuses the parts list's already-fetched elements,
where the re-fetch could never fail.

The owner ruled on the exclusion markers (2026-10-20 fix prompt,
F-3): no exclusion is permitted, and the five markers came out.
The getters that needed them read in simdjson's error-code form
(`code = x.get(out)`), each code routed to an error path the
existing plants already reach: the wrong-kind plants cover the
five helpers, the `kNonBoolAdditional` plant covers the
`additionalProperties` refusal, the enum mutants cover both sides
of the enum-refusal message, and a plant that states items on a
scalar node walks the array branch's not-that-kind path. The builder
failure at `json_report.cpp` stands as one `SG_ENSURE` over
`builder.view()`, the branch removed as the owner directed: the
allocation that grew the buffer already stands in memory, so no
harness reaches the failure. The file read was rewritten too
(round-2 prompt, H-1): GCC 14's `-Werror=null-dereference` rejects
the `istreambuf_iterator` form, and a whole-buffer read cannot tell
a read error from an empty file, so `readFile` loops `stream.read`
over 1024-byte chunks and answers `stream.bad()`; the
`/proc/self/mem` refusal stays a test.

Final re-gate on the closing tree: `build/` 99/99, ASan/UBSan 99/99,
TSan 96/96, the `ci-ubuntu` build with clang-tidy and cppcheck wired in
at 0 error-severity diagnostics, `format-check`, `cmake -P
cmake/spell.cmake`, `example_drift.py` and `dbc-gate` (258 interfaces,
0 gaps) all exit 0, the prose tree gate at 452 sources and 72,361 units
with 0 findings, and the quickstart answers re-run on the closing
binaries (key order, 120 chunk values beside a chunk count of 120, the
three error forms at status 2, the stable diff `True`).

The rebase ran when N2 landed first: `master` moved to `7a75e95`
with 0.12.0 taken, so the branch rebased and the version moved to
0.14.0: PR #42 held 0.13.0, and D-13 records 0.14.0 for the
rebased landing. Three shared files conflicted:
`internal.hpp` keeps both sides' struct members, `example_drift.py`
stacks the page-cap moves (666 from the metric section, 687 with
the JSON section, the measured figure the drift gate confirms), and
`harness.md` keeps the metric section before the JSON section. The
recorded sample output moves to 0.14.0 with the version. A second
rebase ran when PR #42 landed as spec 025: `master` moved to
`e888976` with 0.13.0 taken, so 0.14.0 stands. Four files
conflicted: `internal.hpp` keeps both sides' struct members again,
`cli.cpp` keeps both options tables' entries and both switch
cases, `CMakeLists.txt` keeps both `PRIVATE` link blocks
(`hwloc_vendor` and `simdjson`), and `example_drift.py` stacks the
cap moves again (692 from the topology section, 713 with the JSON
section, the measured figure the drift gate confirms). The
post-rebase chain ran on this machine: ctest 107/107, the drift
gate, prose over the commits (0 findings over 38 sources) and spell
all exit 0; the coverage capture reads 99.8% line (7131 of 7144)
and 100.0% branch (3143 of 3143), and the 13 short lines are all
closing braces
that gcov 16.2.1 marks unexecutable on master's own lines (blame:
`cf82ef3`, `d115344`, `6c7f06c`, and the 025 chain), the record
class D-16 names,
which lcov 2.3.1's brace filter removes as branch records but not
as line records on this toolchain. The CI coverage job, which
passed over the same lines on `master`, is the authoritative gate.
