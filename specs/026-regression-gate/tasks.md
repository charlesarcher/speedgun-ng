# Tasks: Regression Gate

**Input**: Design documents from `specs/026-regression-gate/`

**Prerequisites**: [spec.md](spec.md), [plan.md](plan.md),
[research.md](research.md), [data-model.md](data-model.md),
[contracts/](contracts/), [quickstart.md](quickstart.md)

**Tests**: every capability carries a test, because Principle VI sets
100% line and 100% branch coverage as a hard gate (FR-069, SC-008) and
FR-071 bounds the test surface. The list runs in TDD order: a test task
lands before the code task that turns it green, and the task names the
run that fails first (Principle III, "tests written and observed failing
(red) before the implementation that turns them green").

**Organization**: phases 3 to 8 follow the user stories of the spec, so
each story is independently testable.

**Revision**: written by `/speckit.tasks` on 2026-10-21 as fifty-eight
tasks `T001` to `T058` in the phase counts the plan's Task Planning
Summary commits to: Setup 6, Foundation 12, US1 7, US2 6, US3 6, US4 5,
US5 5, US6 5, Polish 6. The coverage pass found every `FR-nnn` and
`SC-nnn` of the spec cited, so no task was added and no id moved.
`/speckit.analyze` cycle 1 on the same date added no task and moved no
id either: it re-cited T037 and T047 to the two requirements the analyze
pass added to the spec, `FR-073` and `FR-074`, and repaired the
coverage rows beside them. `/speckit.analyze` cycle 2 on the same date
added no task and moved no id either: it rewrote the spec-decision
citations to the `spec D-nn` form, moved the option-scan anchor of T021
to `:394`, and named the percentile point T006 must carry.

## Path Conventions

Harness sources `source/harness/`, public headers `include/speedgun-ng/`,
tests `test/source/`, fixture documents `test/data/`, docs examples
`example/docs/`, gates `test/` and `tools/`, schema `docs/schema/`.
Every anchor below was re-read on `aec8fce` on 2026-10-21.

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: the public types, the extended `ReportData`, the new unit,
the option fields, the schema file and the fixture documents every
later phase reads.

- [X] T001 Add `CompareState`, `Aggregate`, `Direction`, `CompareRule`, `CompareEntry` and `CompareResult` to `include/speedgun-ng/benchmark.hpp` beside `ReportData` (`:553`) with the fields and invariants of `data-model.md` E-01 to E-05: `CompareState` the seven words `OK`, `REGRESSED`, `IMPROVED`, `NOISY`, `ADDED`, `REMOVED`, `SKIPPED` as `std::uint8_t` enumerators, `Aggregate` `MEDIAN`, `MEAN`, `MIN`, `MAX`, `Direction` `LOWER_IS_BETTER`, `HIGHER_IS_BETTER`, and `CompareRule` holding `text`, `valueName`, `aggregate`, `direction`, `relativeThreshold`, `absoluteThreshold` and `filter` under the invariant "At least one of the two thresholds stands in every parsed rule" (FR-012, FR-015, FR-021, FR-022, FR-054, FR-064, D-01, D-08)
- [X] T002 Move `NoiseWarning` from `source/harness/detail/json_report.hpp:137` to `include/speedgun-ng/benchmark.hpp` with `spinning`, `runnableCpus` and `sharesOneCpu` unchanged, keep `noiseWarningOf` internal at `:154` answering `std::optional<sg::NoiseWarning>`, and add the three `ReportData` fields of E-07: `schemaVersion`, `speedgunVersion` and `noiseWarnings` as `std::vector<std::optional<NoiseWarning>>` with the invariant "one entry per entry of `results` when the load call fills it" (FR-064, FR-070, D-04, D-05)
- [X] T003 [P] Create `source/harness/detail/compare.hpp` with `CompareExit { int status; bool failed; }`, `runCompareMode`, `printCompareTable`, `writeCompareReport` and the default rule text constant `time@median:lower:5%`, and create `source/harness/compare.cpp` with the license header and `#include "detail/compare.hpp"`; the `GLOB_RECURSE` at `CMakeLists.txt:191` takes the unit into `speedgun-ng_harness`, so no CMake edit registers it (FR-064, D-02, D-15)
- [X] T004 [P] Add the four fields of E-11 to `RunOptions` at `source/harness/detail/internal.hpp:430`: `compareBaseline`, `compareContender`, `compareRules` as `std::vector<CompareRule>` with the invariant "one entry per `--compare-rule=`, in order; empty means the default rule", and `compareStrict` (FR-045, FR-047, FR-049, FR-050)
- [X] T005 [P] Write `docs/schema/compare-v1.json` as the schema of E-10 in the report schema's conventions: "`x-order` beside `required` at every object, definitions inlined, no `$ref`", the `schemaVersion` const `1.0`, the top-level keys `schemaVersion`, `speedgun`, `inputs`, `rules`, `entries`, `verdict` and the entry keys `name`, `rule`, `baseline`, `contender`, `changePercent`, `changeAbsolute`, `state`, `reason` (FR-057, FR-058, D-14)
- [X] T006 [P] Create the fixture report documents under `test/data/`: `gate_ok_base.json`, `gate_ok_contender.json`, `gate_fail_base.json`, `gate_fail_contender.json`, `gate_fit_base.json`, `gate_fit_contender.json`, `gate_noisy_base.json`, `gate_noisy_contender.json` and `gate_schema_11.json`, each a document `loadReport` accepts, carrying the 3% and 9% median gaps, the `BIG_O` and `RMS` fit rows, the `noiseWarning` record, the `99` percentile point both documents of the ok pair carry for the quickstart's `p99` row, and the `1.1` schema text the quickstart's command table names (FR-071, SC-001, SC-006, SC-007, PC-10)

**Checkpoint**: the types compile, the schema file and the fixtures stand
on disk, and no behavior exists yet.

---

## Phase 2: Foundation (Blocking Prerequisites)

**Purpose**: the grammar, the value resolution, the arithmetic, the
guard, the matching, the validation, the engine, the table, the document
and the reader extension. Every story reads them.

**CRITICAL**: no story work begins before this phase is complete.

- [X] T007 [P] Register `harness_compare_test` and `harness_compare_cli_test` in `test/CMakeLists.txt` with the shape of `harness_json_report_test` at `:569` to `:577`: `add_executable`, `target_link_libraries(... PRIVATE speedgun-ng::harness)`, `target_include_directories(... PRIVATE ${CMAKE_SOURCE_DIR}/source/harness)` for the detail header, `target_compile_features(... PRIVATE cxx_std_23)` and `add_test`, that last command passing `${CMAKE_SOURCE_DIR}/test/data` as its fixture path argument, the shape of `add_test(NAME hwloc_purity_scan ...)` at `:82` (FR-071, SC-007, D-20, PC-10)
- [X] T008 [P] Write the grammar tests in `test/source/harness_compare_test.cpp` over the nine rows of the fault table of `contracts/rule-parse.md`, each naming the field it faults: `time:lower:5` the threshold field, `time@mode:lower:5%` the aggregate word, `p99@median:lower:10%` and `stat:peak@mean:lower:5%` the written aggregate suffix, `time:faster:5%` the direction word, `time@median:lower:abs=12.5,5%` the threshold order, and `time@median:lower:5%:(` the filter regex, beside the two accepted forms `time@median:lower:5%,abs=12.5` and `counter:cache\:misses@mean:higher:2%`; the target builds at T007 and the run fails red before T009 lands (FR-004, FR-005, FR-012, FR-015, FR-016, FR-018, FR-019, FR-020, FR-021, FR-022, FR-023, FR-024)
- [X] T009 Implement `parseCompareRule` in `source/harness/compare.cpp` with the signature `auto parseCompareRule(std::string_view text) -> std::expected<CompareRule, std::string>` and the grammar of `contracts/rule-parse.md`: the parse "reads the value name by prefix", the escapes `\:`, `\@` and `\\` stand inside the label, and the threshold field is "`N%`, or `abs=X`, or `N%,abs=X`: the percent first, then the `abs=` marker, one comma between", with `5%` answering `0.05`; turn T008 green (FR-004, FR-005, FR-012, FR-015, FR-016, FR-018, FR-019, FR-020, FR-021, FR-022, FR-023, FR-024, D-06, D-07, D-08)
- [X] T010 [P] Add the reader-extension tests to `test/source/harness_json_load_test.cpp`: the two header texts load into the new fields, a stored warning answers its three values beside its result, a document that omits the `speedgun` object answers an empty `speedgunVersion` with no fault, and each error code the extension can answer carries its own case, "a non-object `speedgun` header, a non-string `version`, and the noise record's existing field checks"; the run fails red before T011 lands (FR-067, FR-070)
- [X] T011 Implement the reader extension in `source/harness/json_load.cpp` at the noise read of `:729` to `:743` and the header read: store the `schemaVersion` text the version gate of `:1117` to `:1128` already reads, the `version` text under the `speedgun` header, and the warning record it already parses, each read in "the parser's error-code form `x.get(out)`" with no throwing getter; turn T010 green (FR-067, FR-068, FR-070, D-04, D-05)
- [X] T012 [P] Add the value-resolution and row-selection tests to `test/source/harness_compare_test.cpp` over `ReportData` values built in code: the primary column under `time` following `primaryIsCpu`, `real_time` and `cpu_time` naming their columns directly, the percentile cell of column 0 under `p<point>`, the aggregate row read when one stands and the single measured row when none stands, and the single-row run answering "every aggregate with that row's value"; the run fails red before T013 lands (FR-011, FR-013, FR-014, FR-016, FR-017, FR-032)
- [X] T013 Implement the value resolution and the row selection in `source/harness/compare.cpp` per E-08 and D-09: the `AGGREGATE` row when the result carries one and the single measured row when it carries none, the percentile cell read at the point's index in `percentilePoints` for column 0, and a result with no readable row answering `SKIPPED`; turn T012 green (FR-011, FR-013, FR-014, FR-016, FR-017, FR-032, D-09, D-10, spec D-18, spec D-21)
- [X] T014 [P] Add the threshold, delta and verdict tests to `test/source/harness_compare_test.cpp` per E-09: `delta` as `c - b` for `LOWER_IS_BETTER` and `b - c` for `HIGHER_IS_BETTER`, the relative test over `|b|`, the absolute test in the stored unit, "With both thresholds the row regresses when both are exceeded", `delta` below zero answering `IMPROVED` and zero answering `OK`, and a zero baseline magnitude under a relative threshold answering one fault text; the run fails red before T015 lands (FR-025, FR-026, FR-027, FR-028, FR-029, FR-030, FR-031)
- [X] T015 Implement the threshold arithmetic and the verdict words in `source/harness/compare.cpp` per E-09, the absolute test standing in the stored unit of the value, and the zero-baseline rule of D-18: the absolute test answering alone with the token `n/a` in the percent column and `null` in the document's `changePercent`; turn T014 green (FR-025, FR-026, FR-027, FR-028, FR-029, FR-030, FR-031, spec D-4, D-18, spec D-22)
- [X] T016 Implement the noise guard in `source/harness/compare.cpp` per D-11: a carried warning marking the row by itself, `coefficientOfVariation` held against the relative threshold and `sampleStdDev` against the absolute threshold in the stored unit with "either report over its bound" marking the row, a metric column marking the row when either report's `runningRatio` sits below one, and a percentile or statistic value guarded "through the carried warning and the running ratio alone"; its tests land with US4 at T038 (FR-041, FR-042, FR-043, FR-046, D-11, spec D-19)
- [X] T017 Add the matching and validation tests to `test/source/harness_compare_test.cpp` and implement the matching, the validation pass and the version pairing in `source/harness/compare.cpp` in the same task: matching by the full instance name ignoring document order, `ADDED` and `REMOVED` entries with no values, the walk of `contracts/compare-call.md` in its stated fault order with the first fault stopping the call, and the empty pair answering an empty entry list; the tests fail red before the code half of the task lands (FR-002, FR-008, FR-009, FR-010, FR-011, FR-035, FR-036, FR-037, FR-038, FR-039, FR-040, FR-051, SC-013, D-13, D-19, spec D-20)
- [X] T018 Implement the public call, the table and the document writer: `compareReports` declared in `include/speedgun-ng/benchmark.hpp` with the signature and the `\pre` and `\post` block of `contracts/compare-call.md`, defined in `source/harness/compare.cpp`, beside `printCompareTable` with the columns of FR-053, the header of FR-056 and the unit rule of FR-055, and `writeCompareReport` with the seven arguments of `contracts/compare-document.md`; the table and document tests land with US1 at T019 and US5 at T044 (FR-001, FR-003, FR-006, FR-007, FR-033, FR-034, FR-053, FR-054, FR-055, FR-056, FR-057, FR-064, FR-068, D-03, D-14)

**Checkpoint**: one unit parses a rule, resolves a value, compares two
loaded values, prints a table and writes a document; the reader answers
the header texts and the warning; nothing is wired to the command line
yet.

---

## Phase 3: User Story 1 - Get a pass or fail answer over two reports (Priority: P1) MVP

**Goal**: one command loads two documents, matches the benchmarks by
name, applies the rules, prints one line per benchmark and rule, and
answers a status a CI job can branch on.

**Independent Test**: `harness_compare_cli_test` over the fixture pairs
in `test/data/`, plus the two hand runs of the quickstart.

### Tests for User Story 1

- [X] T019 [P] [US1] Add the mode tests to `test/source/harness_compare_cli_test.cpp`: the run over `test/data/gate_ok_base.json` and `test/data/gate_ok_contender.json` answering status 0 with every row `OK`, the fail pair answering status 1 with the row `REGRESSED` and "the change column carries the percent and the absolute delta", and the header naming "the two files, the speedgun version of each report and the schema version of each"; the run fails red before T022 lands (FR-053, FR-056, SC-001, SC-005)

### Implementation for User Story 1

- [X] T020 [US1] Add `compare`, `compare-rule` and `compare-strict` to the `longOptions` table of `source/harness/cli.cpp` at `:293` to `:310` (the terminator stands at `:310`), and fill the four `RunOptions` fields at the scan, where "`--compare` with no comma, or with an empty side, prints one line naming the option and the text read, and answers status 2" (FR-045, FR-047, FR-048, FR-049)
- [X] T021 [US1] Add the compare branch to `speedgunMain` of `source/harness/cli.cpp` after the option scan closes at `:394` and before `detail::expandRegistry` at `:434`, in the form of `contracts/compare-options.md`: `const auto exit = detail::runCompareMode(options);`, then `SG_ENSURE((exit.status != 0) == exit.failed, ...)`, then `return exit.status;`, leaving the closing postcondition at `:566` and the `runner.run(*entry)` anchor of `test/cli_shape.sh:31` to `:41` untouched (FR-052, FR-072, D-15)
- [X] T022 [US1] Implement `runCompareMode` in `source/harness/compare.cpp`: load both documents through `loadReport`, apply the batch filter to the loaded values, call `compareReports`, print the table or the one fault line, write the document when `--json=` stands, and answer `CompareExit` with `status` and `failed` "computed independently so the `SG_ENSURE` carries content"; "When the filtered pair shares no name, the run prints one line saying the pair matched nothing and answers status 0" (FR-006, FR-007, FR-047, FR-050, FR-053, FR-056, FR-057, D-12, D-19)
- [X] T023 [US1] Apply the default rule in `source/harness/compare.cpp` from the constant of `source/harness/detail/compare.hpp` when `RunOptions::compareRules` stands empty: "`time@median:lower:5%` applies when no rule is given. Given rules replace it" (FR-033, FR-034, SC-002, spec D-5)
- [X] T024 [US1] Add the compare rows to the exit-status table of `specs/015-benchmark-harness-core/contracts/cli.md:72` to `:82`: the seven rows of `contracts/compare-options.md`, status 1 for a regression, status 2 for a refused document, a version mismatch, a filter that matches nothing, a missing or uncountable value, a zero baseline under a relative threshold, a scope with no readable row and an unwritable comparison document, and status 0 for a pass or a pair that matched nothing (FR-052)
- [X] T025 [US1] Add the status-mapping tests to `test/source/harness_compare_cli_test.cpp`: `--filter=zzz` and an empty-name pair each answering the matched-nothing line at status 0, and `--compare=test/data/gate_ok_base.json` answering one usage line and status 2 (SC-001, SC-002, FR-048, FR-051, D-19)

**Checkpoint**: one command, one table, one status, over committed
fixture documents, with no benchmark run.

---

## Phase 4: User Story 2 - Name the value and the rule (Priority: P1)

**Goal**: one rule per watched value names the value, the aggregate, the
direction, the threshold pair and an optional filter, and the gate
applies exactly those rules.

**Independent Test**: `harness_compare_test` over `ReportData` values
built in code that carry a percentile column, a counter column, a metric
column and three attached statistics, plus the rule rows of the
quickstart's command table.

- [X] T026 [P] [US2] Add the value-kind matrix tests to `test/source/harness_compare_test.cpp` over `ReportData` values built in code: each name of E-08 (`time`, `real_time`, `cpu_time`, `p<point>`, `counter:<label>`, `metric:<label>`, `stat:<name>`) under each aggregate it accepts, in both directions, with each threshold form, each double read through `sameValue` (`test/source/detail/json_fixture.hpp:48`); the run fails red before T030 and T031 land (SC-003, FR-013, FR-014, FR-015, FR-016, FR-017, FR-019, FR-025, FR-026, FR-027, FR-028)
- [X] T027 [US2] Apply the rule filter inside the validation walk and the comparison of `source/harness/compare.cpp`: "each rule's `filter`, a `std::regex` over the full instance name, the same matching as the batch filter (`source/harness/cli.cpp:453`)", the batch filter applying first and an invalid pattern standing as a rule fault (FR-024, FR-035, FR-039, FR-050, E-12, D-12)
- [X] T028 [P] [US2] Add the rule-option tests to `test/source/harness_compare_cli_test.cpp`: `metric:ipc:higher:3%:bmHash.*` scoping to the matching names, `counter:bytes@mean:higher:abs=1e6` failing only past one million, the fit pair printing `SKIPPED` beside the readable benchmark at status 0, and two written rules applying exactly those two with the default rule absent; the run fails red before T029 lands (SC-002, SC-003, FR-011, FR-034, FR-040, FR-049)
- [X] T029 [US2] Answer the fit entry in `source/harness/compare.cpp`: "a matched benchmark whose rows hold no row a rule can read ... answers a `SKIPPED` entry with no values and does not fail while strict mode stands off; a rule whose whole filtered scope holds only such results is the fault FR-040 states"; turn T028 green (FR-011, FR-040, spec D-20)
- [X] T030 [US2] Carry the stored unit through the absolute test and the printed columns of `source/harness/compare.cpp`: "nanoseconds per iteration for a time column, a percentile cell or a `TIME` statistic, percentage points for a `PERCENTAGE` statistic, and the counter's own unit for a counter or metric column", the table printing "both value columns and the absolute change in the unit the baseline report declares" (FR-027, FR-055, spec D-22)
- [X] T031 [US2] Read the aggregate from the record the report carries for the value in `source/harness/compare.cpp`: `ResultRow::timeStats` for the primary column, `ResultRow::metricStats` at the metric label's index, `BenchmarkResult::counterStats` at the counter label's index, and "An aggregate named on a column with no record in a repeated run is a missing value" (FR-016, FR-017, FR-032, D-09, spec D-18)

**Checkpoint**: every value kind of the name space answers a comparison,
each under the aggregates it accepts.

---

## Phase 5: User Story 3 - Get one clear line when the rule or the file is wrong (Priority: P1)

**Goal**: each fault answers one line naming the fault and a status that
tells CI the run never started.

**Independent Test**: `harness_compare_cli_test` driven against each
malformed rule text, each unknown value name, each empty filter and each
broken file.

- [X] T032 [P] [US3] Add the fault-form tests to `test/source/harness_compare_cli_test.cpp`: each row of the quickstart's command table answering exactly one line and status 2 with no table printed, covering the malformed rule text, the filter that matches nothing, the missing file, the pair `test/data/gate_ok_base.json` against `test/data/gate_schema_11.json` naming both versions, and a relative threshold over a zero baseline naming the benchmark and the value (SC-004, SC-013, FR-051)
- [X] T033 [US3] Print the one fault line in `runCompareMode` of `source/harness/compare.cpp` from the text `compareReports` answered, "the string names the rule, the benchmark and the report of the first fault", and print no partial table (FR-003, FR-005, FR-051)
- [X] T034 [US3] Add the scan-time usage faults to `source/harness/cli.cpp`: the `--compare` split at the first comma with an empty side, the `--compare-rule` text the parse call rejecting through "one line naming the option and the rule text and the fault the parse call named", and the invalid batch filter reusing the run path's line at `:442` to `:446` (FR-024, FR-048, FR-051)
- [X] T035 [US3] Answer the load and value faults in `runCompareMode` of `source/harness/compare.cpp`: a missing file and a document the reader refuses each printing one line naming the path and the reader's text at status 2, and the value-absent, uncountable-metric and percentile-point faults naming "the rule, the benchmark, the report" (FR-006, FR-036, FR-037, FR-038, FR-051)
- [X] T036 [US3] Stand the schema version pairing check first inside `compareReports` of `source/harness/compare.cpp` so a library caller answers the same fault: the fault row "the two `schemaVersion` texts differ" naming "both versions" in the error text the mode prints as its one line (FR-051, SC-013, D-13)
- [X] T037 [US3] Answer the document write failure through `runCompareMode` of `source/harness/compare.cpp`: a false `writeCompareReport` answer setting `CompareExit::failed` and the status 2 row, "the form the report write already answers (`specs/015-benchmark-harness-core/contracts/cli.md:74`)" (FR-051, FR-057, FR-074)

**Checkpoint**: nine fault forms, nine one-line answers, status 2 each,
and no table.

---

## Phase 6: User Story 4 - Refuse to fail on a noisy measurement (Priority: P2)

**Goal**: a carried warning, a spread wider than the rule's own bound,
or a multiplexed metric marks the row `NOISY` with its reason and keeps
the run passing; strict mode turns those rows back into failures.

**Independent Test**: `harness_compare_test` over report pairs with each
noise source, and the noisy fixture pair through the command surface
with strict off and on.

- [X] T038 [P] [US4] Add the noise-guard tests to `test/source/harness_compare_test.cpp`: a carried warning in either report, `coefficientOfVariation` over the relative threshold, `sampleStdDev` over the absolute threshold in the stored unit, a metric `runningRatio` below one in either report, and a value with no statistics record guarded "through the carried warning and the running ratio alone", each double read through `sameValue`; the run fails red against T016 (SC-006, FR-041, FR-042, FR-043, FR-046, spec D-19)
- [X] T039 [US4] Name the reason in the guard of `source/harness/compare.cpp`: the `reason` field of `CompareEntry` standing "empty except on `NOISY`, where it names the bound, the carried warning or the running ratio that fired", and the table printing it beside the state word (FR-044, E-02)
- [X] T040 [US4] Move the verdict only for strict inside `compareReports` of `source/harness/compare.cpp`: `passed` standing "false when any entry is `REGRESSED`, or when `strict` stands and any entry is `NOISY`", the state word staying `NOISY` either way (FR-044, FR-045, E-03)
- [X] T041 [US4] Carry `--compare-strict` from the scan of `source/harness/cli.cpp` into `RunOptions::compareStrict` and from there into the `compareReports` call of `source/harness/compare.cpp` (FR-045, FR-049)
- [X] T042 [P] [US4] Add the noisy-pair runs to `test/source/harness_compare_cli_test.cpp`: the `test/data/gate_noisy_base.json` pair printing `NOISY` with its reason at status 0, and the same pair with `--compare-strict` answering status 1 with the word unchanged (SC-006, FR-044, FR-045)

**Checkpoint**: the guard answers three reasons, and strict moves the
verdict while the word stands.

---

## Phase 7: User Story 5 - Read the comparison as a library, and as a document (Priority: P2)

**Goal**: the two calls answer values a consumer build can read, and
`--json=` in compare mode leaves one validating document beside the two
reports.

**Independent Test**: `harness_compare_test` calling the two calls
directly, and the `cmp` pair of the quickstart over two written
documents.

- [X] T043 [P] [US5] Add the library-surface tests to `test/source/harness_compare_test.cpp`: `compareReports` over two values answering "one entry per matched benchmark and rule, each carrying both values, the change and the state, and one overall verdict", the entries standing in "the run order of the baseline report, with the added names after them in the contender's order", and `parseCompareRule` answering the six fields or one error naming the fault (SC-007, FR-001, FR-002, FR-004, FR-005)
- [X] T044 [P] [US5] Add the document tests to `test/source/harness_compare_cli_test.cpp`: two compare-mode runs over one pair writing byte-identical files, `checkReport` (declared at `source/harness/detail/json_report.hpp:188`) answering no disagreement between the written bytes and `docs/schema/compare-v1.json`, and the key order of E-10 standing in the written document (SC-009, FR-057, FR-058)
- [X] T045 [US5] Write the comparison document from `runCompareMode` of `source/harness/compare.cpp` when `--json=` stands in compare mode, "the mode writes no report document", through `writeCompareReport` with the seven arguments of `contracts/compare-document.md` (FR-057, FR-058)
- [X] T046 [US5] Emit the document in `writeCompareReport` of `source/harness/compare.cpp` through the vendored builder's `string_builder`, carrying "no date, no host and no pid", an empty optional writing `null`, and values standing "in the stored unit, with no printed scaling" (FR-058, FR-068, SC-009, D-14, spec D-22)
- [X] T047 [P] [US5] Add the unknown-key acceptance test to `test/source/harness_json_load_test.cpp`: a document carrying a key the reader's node table does not list inside an open object loads with no fault, the fact that keeps the two lanes loadable in either merge order, "this reader keeps accepting unknown keys inside open objects"; if N6 lands first, this task also adds the test that the comparator ignores the pass fields (FR-073, PC-4)

**Checkpoint**: the call answers values, the file answers the schema, and
two runs write the same bytes.

---

## Phase 8: User Story 6 - Read the page, copy the block, run the example (Priority: P3)

**Goal**: the main page teaches the gate from one short section whose
snippet comes from an example CI runs.

**Independent Test**: `python3 tools/docs/example_drift.py .` reporting
no finding, and `test/example_run.sh` running `docs_regression_gate`
against its recorded file.

- [X] T048 [P] [US6] Write `example/docs/regression_gate.cpp` with one registered family so `test/example_run.sh` finds a non-empty `--list`, and a `main` that "builds two `ReportData` values in code, calls `parseCompareRule` and `compareReports`, prints one line per entry with its state word, then calls `speedgunMain`", the comparison code inside a `// sg-example:begin regression-gate` region, beside `example/docs/regression_gate.expected` recording the printed lines inside a `# sg-example:begin report` region (FR-059, FR-060, D-16)
- [X] T049 [US6] Add `add_docs_example(regression_gate)` to `example/CMakeLists.txt` after `add_docs_example(json_reports)` at `:72`, which wires the `docs_regression_gate` binary, the `run_docs_regression_gate` runner and the `run-examples` dependency (FR-060)
- [X] T050 [US6] Add the `### Regression gate` section to `README.md` after `### JSON reports` at `:579`, at 25 lines or fewer, with the `<!-- sg-example: example/docs/regression_gate.cpp#regression-gate -->` block and one result sentence naming the status answer and the seven state words, beside the quick-reference rows for `compareReports`, `parseCompareRule`, `CompareRule` and `CompareState` in the Name table at `:597` and the `--compare`, `--compare-rule` and `--compare-strict` rows in the Option table at `:688` to `:706` (FR-059, FR-061, SC-011)
- [X] T051 [P] [US6] Add the `## Regression gate` section to `docs/pages/harness.md` after `## JSON reports` at `:1018`: the rule grammar in full, the seven state words with their meanings, the fault lines, the comparison document and its schema path, the shell blocks that drive the mode, and the `<!-- sg-sample-output: example/docs/regression_gate.expected#report -->` block (FR-062, SC-011)
- [X] T052 [US6] Raise `PAGE_LINE_CAP` in `tools/docs/example_drift.py:132` from 713 to the measured page length as the section lands and record the measurement in the comment above it the way the specs/023 move is recorded, add `Regression gate` to `SECTIONS` at `:30` to `:54` and to `SECTION_OWNERS` at `:59` with `regression_gate.cpp` as its owner, and keep `SECTION_LINE_CAP` at 25 with no entry joining `SECTION_LINE_CAPS` (FR-063, SC-011)

**Checkpoint**: the page, the region, the rows and the drift gate agree,
and the example runs in CI.

---

## Phase 9: Polish & Cross-Cutting Concerns

- [X] T053 [P] Take the next free minor version at `CMakeLists.txt:7` (0.15.0 or 0.16.0 by the merge order against N6, which reserves 024 on another machine) and move the harness `SOVERSION` from 8 to 9 at `:247`, leaving the `speedgun-ng` archive `SOVERSION` at 2 at `:47` (FR-065, FR-066, spec D-14, D-17)
- [X] T054 [P] Run `cmake --preset=ci-sanitize` with `cmake --build build/sanitize` and `ctest --test-dir build/sanitize -j 2 --output-on-failure`, then `cmake --preset=ci-tsan` with `cmake --build build/tsan` and `ctest --test-dir build/tsan -j 2 --output-on-failure`, over every new path (SC-012)
- [X] T055 [P] Run `cmake --preset=ci-coverage`, `cmake --build build/coverage -j 2`, `ctest --test-dir build/coverage -j 2 --output-on-failure` and `cmake --build build/coverage -t coverage`, whose summary step runs `tools/dbc/coverage_gate.sh`: report 100% lines and 100% branches over `source/harness/compare.cpp`, the reader extension in `source/harness/json_load.cpp` and the compare branch in `source/harness/cli.cpp`, with no `LCOV_EXCL` marker, keeping `-fprofile-update=atomic` and no `-D_GLIBCXX_ASSERTIONS` (FR-069, SC-008)
- [X] T056 [P] Run `cmake --preset=ci-rocky` with `cmake --build build` and `ctest --test-dir build -j 2 --output-on-failure`, then the `ci-ubuntu` build with clang-tidy and cppcheck reporting no new finding, then `cmake --build build --target format-check`, `cmake -P cmake/spell.cmake` and `cmake -P cmake/prose-lint.cmake` (SC-012)
- [X] T057 [P] Run `bash test/time_source_gate.sh`, `bash test/loop_shape.sh build`, `bash test/cli_shape.sh`, `python3 tools/docs/example_drift.py .`, `ctest --test-dir build -R 'simdjson_purity_scan|simdjson_nm_proof'`, `cmake --build build -t dbc-gate` and `python3 tools/prose/prose_gate.py --check all` over the branch's commits: the loop-shape gate printing the same per-iteration cycle it prints at `aec8fce` under both compilers it drives, and `test/cli_shape.sh` reading its base anchors with the compare branch before the run loop (FR-072, SC-010, SC-011, SC-012)
- [ ] T058 Rebase on `master` keeping both features' lines in every shared list, retake the version per FR-066, re-measure the page budget against the page the rebase leaves, run the full gate set, and push once with `--force-with-lease`, showing `git ls-remote origin 026-regression-gate` (FR-066, SC-012)

---

## Requirement Coverage Map

*The mechanical check that speckit-analyze re-derives: each row names
the tasks that cite the id, generated from the task lines above on
2026-10-21, after the analyze pass. Counts: 74 FRs, 13 SCs, 58 tasks,
227 citations.*

| Requirement | Tasks |
|---|---|
| FR-001 | T018, T043 |
| FR-002 | T017, T043 |
| FR-003 | T018, T033 |
| FR-004 | T008, T009, T043 |
| FR-005 | T008, T009, T033, T043 |
| FR-006 | T018, T022, T035 |
| FR-007 | T018, T022 |
| FR-008 | T017 |
| FR-009 | T017 |
| FR-010 | T017 |
| FR-011 | T012, T013, T017, T028, T029 |
| FR-012 | T001, T008, T009 |
| FR-013 | T012, T013, T026 |
| FR-014 | T012, T013, T026 |
| FR-015 | T001, T008, T009, T026 |
| FR-016 | T008, T009, T012, T013, T026, T031 |
| FR-017 | T012, T013, T026, T031 |
| FR-018 | T008, T009 |
| FR-019 | T008, T009, T026 |
| FR-020 | T008, T009 |
| FR-021 | T001, T008, T009 |
| FR-022 | T001, T008, T009 |
| FR-023 | T008, T009 |
| FR-024 | T008, T009, T027, T034 |
| FR-025 | T014, T015, T026 |
| FR-026 | T014, T015, T026 |
| FR-027 | T014, T015, T026, T030 |
| FR-028 | T014, T015, T026 |
| FR-029 | T014, T015 |
| FR-030 | T014, T015 |
| FR-031 | T014, T015 |
| FR-032 | T012, T013, T031 |
| FR-033 | T018, T023 |
| FR-034 | T018, T023, T028 |
| FR-035 | T017, T027 |
| FR-036 | T017, T035 |
| FR-037 | T017, T035 |
| FR-038 | T017, T035 |
| FR-039 | T017, T027 |
| FR-040 | T017, T028, T029 |
| FR-041 | T016, T038 |
| FR-042 | T016, T038 |
| FR-043 | T016, T038 |
| FR-044 | T039, T040, T042 |
| FR-045 | T004, T020, T040, T041, T042 |
| FR-046 | T016, T038 |
| FR-047 | T004, T020, T022 |
| FR-048 | T020, T025, T034 |
| FR-049 | T004, T020, T028, T041 |
| FR-050 | T004, T022, T027 |
| FR-051 | T017, T025, T032, T033, T034, T035, T036, T037 |
| FR-052 | T021, T024 |
| FR-053 | T018, T019, T022 |
| FR-054 | T001, T018 |
| FR-055 | T018, T030 |
| FR-056 | T018, T019, T022 |
| FR-057 | T005, T018, T022, T037, T044, T045 |
| FR-058 | T005, T044, T045, T046 |
| FR-059 | T048, T050 |
| FR-060 | T048, T049 |
| FR-061 | T050 |
| FR-062 | T051 |
| FR-063 | T052 |
| FR-064 | T001, T002, T003, T018 |
| FR-065 | T053 |
| FR-066 | T053, T058 |
| FR-067 | T010, T011 |
| FR-068 | T011, T018, T046 |
| FR-069 | T055 |
| FR-070 | T002, T010, T011 |
| FR-071 | T006, T007 |
| FR-072 | T021, T057 |
| FR-073 | T047 |
| FR-074 | T037 |
| SC-001 | T006, T019, T025 |
| SC-002 | T023, T025, T028 |
| SC-003 | T026, T028 |
| SC-004 | T032 |
| SC-005 | T019 |
| SC-006 | T006, T038, T042 |
| SC-007 | T006, T007, T043 |
| SC-008 | T055 |
| SC-009 | T044, T046 |
| SC-010 | T057 |
| SC-011 | T050, T051, T052, T057 |
| SC-012 | T054, T056, T057, T058 |
| SC-013 | T017, T032, T036 |

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1** has no dependencies.
- **Phase 2** depends on Phase 1 and blocks every story.
- **Phases 3 to 8** depend on Phase 2. US2 depends on US1's branch and
  mode; US3 depends on US1's branch for its one-line answers and on
  Phase 2's validation pass; US4 depends on US1's mode and on the guard
  of T016; US5 depends on US1's mode for the written document; US6 lands
  last, because the section quotes the final option spellings and the
  recorded output.
- **Phase 9** depends on every story the branch ships.

### Story Independence

- **US1** is the MVP: two fixture documents, one command, one table, one
  status.
- **US2** widens the rule to every value the report holds.
- **US3** closes the fault contract around the same command.
- **US4** refines the verdict without changing the table.
- **US5** deepens what the two calls answer and adds the written form.
- **US6** documents the result.

### Within Each User Story

- The test task lands first and fails red, per Principle III.
- T007 registers the targets before T008 can build a test.
- T016's guard is built in Phase 2 and proven by T038 in US4.
- T018's table and document are built in Phase 2 and proven by T019 and
  T044.
- Story complete before the next priority.

### Parallel Opportunities

- T003, T004, T005 and T006 touch four different artifacts: the detail
  header and unit, `source/harness/detail/internal.hpp`,
  `docs/schema/compare-v1.json` and `test/data/`.
- T007 (the target list), T008 (the first test file) and T010 (the
  loader test file) touch three different files.
- T012 and T014 add cases to `test/source/harness_compare_test.cpp`, so
  they run one at a time; each pairs with a `source/harness/compare.cpp`
  task that cannot run beside the other `compare.cpp` tasks.
- Within US1, US3, US4 and US5 the test task and the implementation task
  sit in different files: `test/source/harness_compare_cli_test.cpp`
  beside `source/harness/cli.cpp` and `source/harness/compare.cpp`.
- US6: T048 (`example/docs/`) and T051 (`docs/pages/harness.md`) touch
  different files; T049, T050 and T052 each own one file of their own.
- Phase 9's T053 to T057 run in parallel: each drives a different build
  directory or gate script and edits nothing.

---

## Parallel Example: User Story 1

```bash
# Register the targets, then write the tests beside the mode work:
Task: "Register harness_compare_test and harness_compare_cli_test in test/CMakeLists.txt"
Task: "Add the mode tests to test/source/harness_compare_cli_test.cpp"

# Different files, so both run while the branch work proceeds:
Task: "Add compare, compare-rule and compare-strict to the longOptions table of source/harness/cli.cpp"
Task: "Implement runCompareMode in source/harness/compare.cpp"
```

---

## Implementation Strategy

### MVP First (User Story 1)

1. Complete Phase 1: the types, the extended `ReportData`, the new unit,
   the option fields, the schema file and the fixtures.
2. Complete Phase 2: the grammar, the resolution, the arithmetic, the
   guard, the matching, the validation, the engine, the table, the
   document and the reader extension.
3. Complete Phase 3: the three options, the branch, the mode, the
   default rule, the contract rows and the mode tests.
4. **Stop and validate**: `harness_compare_cli_test` and the two hand
   runs of the quickstart answer 0 and 1 over the fixture pairs.

### Incremental Delivery

1. Phases 1 and 2 -> the comparison stands on its own and the command
   line answers nothing.
2. US1 -> one command, one table, one status.
3. US2 -> every value kind of the name space.
4. US3 -> the fault contract closes.
5. US4 -> the noise guard refines the verdict.
6. US5 -> the library surface and the written document.
7. US6 -> the page.

### Rebase Note for the Two Parallel Features

N6 multi-pass collection runs on corsairone and reserves 024. The shared
files are the project version, the two `SOVERSION` lines, `README.md`,
the quick-reference table, `PAGE_LINE_CAP`, `example/CMakeLists.txt`,
the `example_run` list, the `test/CMakeLists.txt` target lists, the
option table of `source/harness/cli.cpp` and
`docs/schema/report-v1.json`. Rebase, keep both features' lines, retake
the version per FR-066, re-measure the page budget, run the full gate
set, and push once with `--force-with-lease`. The reader extension
touches the noise record and the header only, and the reader keeps
accepting unknown keys inside open objects, which is what makes either
merge order loadable (FR-068, PC-4).

---

## Commit Plan

Each commit follows the constitution template: `<Section>: <one line>`
at 50 characters or fewer, a why-body wrapped at 72, then one footer
block with `Refs: specs/026-regression-gate`. `Approved-by: OmO` stands
on every commit of this branch, the agent that made the calls here.

| Commit | Section | Tasks | Content |
|---|---|---|---|
| 1 | Harness | T001 to T006 | The public types, the moved warning, the extended `ReportData`, the new unit, the option fields, the schema file, the fixtures |
| 2 | Harness | T007 to T011 | The two test targets, the grammar and its tests, the reader extension and its tests |
| 3 | Harness | T012 to T018 | The resolution, the arithmetic, the guard, the matching, the validation, the engine, the table and the document writer |
| 4 | Harness | T019 to T025 | The three options, the branch, the mode, the default rule, the contract rows and the mode tests |
| 5 | Harness | T026 to T037 | The value kinds, the filters, the fit answer, the unit rule, the fault forms and the one lines |
| 6 | Harness | T038 to T047 | The noise tests, the reason, the strict verdict, the library surface and the written document |
| 7 | Docs | T048 to T052 | The example, its target, the section, the guide page and the budget |
| 8 | CMake | T053 to T058 | The version, the harness `SOVERSION`, and the gate runs |

---

## Notes

- [P] marks a task whose files differ from the task it pairs with, so the
  pair can run at the same time. Two [P] tasks that share one file still
  run one at a time: T012 and T014 both add cases to
  `test/source/harness_compare_test.cpp`, and the
  `source/harness/compare.cpp` chain T009, T013, T015, T016, T017, T018,
  T022, T023, T027, T029, T030, T031, T033, T035, T036, T037, T039, T040,
  T045 and T046 runs serially inside that one file.
- The checkbox stays unchecked here; `/speckit.implement` closes each one
  as `- [X]` when its verification runs green.
- `source/harness/compare.cpp` needs no entry in any CMake target list:
  the harness archive globs `source/harness/*.cpp` at `CMakeLists.txt:191`
  and already links the vendored simdjson `PRIVATE BUILD_INTERFACE`.
- `sameValue` at `test/source/detail/json_fixture.hpp:48` is the house
  double comparison and is reused, never redefined, because
  `-Werror=float-equal` rules out `==` on a double.
- No test sleeps, reads a clock, runs a benchmark, names a cpu count or
  host size, or skips. Fixture documents reach a test through the path
  argument T007 passes (FR-071).
- The spec's Citations table named the option table as
  `source/harness/cli.cpp:293` to `:311`; the table ends at `:310` with
  the terminator there, and T020 uses `:310`. The analyze pass
  corrected the spec to `:310`.
- `contracts/compare-call.md`, `contracts/compare-document.md`,
  `contracts/compare-options.md`, `contracts/reader-extension.md`,
  `contracts/rule-parse.md` and `contracts/page-and-examples.md` cited
  a few ids one position off the spec (a no-clock rule as FR-066, the
  docs rules shifted by one). The tasks followed `spec.md`, and the
  analyze pass repaired the contracts.
- This list runs in TDD order, and `plan.md` now carries the one-line
  execution-mode declaration 025 carries, added by the analyze pass.
- Decision ids run in two numberings: `spec.md` numbers its decisions
  D-1 to D-22, `research.md` numbers the plan's D-01 to D-20, and the
  two collide from D-10 to D-20. A bare `D-nn` names the research
  entry; a spec decision is cited as `spec D-nn`. `/speckit.analyze`
  cycle 2 swept every `D-nn` token of `tasks.md`, `plan.md`,
  `data-model.md`, `quickstart.md` and `contracts/` and rewrote the
  twenty spec-meant ones to that form, beside the seven in
  `research.md` itself; `quickstart.md` cites no decision id.

## Phase 10: Convergence

The `/speckit.converge` pass of 2026-10-21 read the branch head
`d709a87` against `spec.md`, `plan.md`, the contracts and the
constitution, then ran the gates. Two hard gates fail on the pushed
head: the DBC pairing gate names one drifted interface, and the CI
coverage gate reads one count-zero line record the local gcov does
not emit. One requirement carries a CLI-surface test gap.

- [X] T059 Pair the contract of `compareReports` with its enforcement: an `SG_ENSURE` postcondition in the body of the call in `source/harness/compare.cpp` and the default-rule `SG_ASSERT` moved out of that documented body, then `cmake --build build -t dbc-gate` reports zero drifted interfaces per Principle II, Principle VI, SC-012 (contradicts)
- [ ] T060 Restructure the batch-filter precompile of `source/harness/cli.cpp:462` to `:467` so the CI gcov answers no count-zero record at the declaration line, then the `ci-coverage` chain answers 100.0% lines and 100.0% branches on the CI toolchain per FR-069, SC-008 (contradicts)
- [X] T061 Add one `harness_compare_cli_test` case that passes two `--compare-rule=` flags in one run and answers two rule-major rows in flag order per FR-049 (partial)

## Phase 11: Audit round 1

Two read-only audits of the branch tip (test-surface coverage and
constitution discipline) ran while Phase 10 landed. Their findings are
listed here with the code evidence each one carried. One finding is
rejected: a `report_shape` entry over the regression-gate example cannot
work, because that gate folds the whole standard output of a bare run and
the example's recorded block holds the eight comparison lines the run
prints before `speedgunMain` (see `tools/docs/report_shape.py`).

- [X] T062 Drop the entry-count clause of the `compareReports` postcondition, which a rule filter breaks, and enforce the rule text of every entry instead, then one library case with a filtered rule beside an unfiltered rule answers three rows and no abort per FR-002, FR-024, Principle II (contradicts)
- [X] T063 Answer a rule built in code whose filter the regex library refuses as one fault line inside the scope walk, so the public call keeps its no-throw promise per FR-051, D-08 (contradicts)
- [X] T064 Add the pairing case where one name stands at the start of another, so a relaxed match answers a failing check per FR-008, FR-010, SC-003 (partial)
- [X] T065 Add the table case that declares microseconds, milliseconds and seconds, so each divisor of the unit scale answers its own cells per FR-055 (partial)
- [X] T066 Add the library case where a valid rule stands beside a rule whose value no report carries, so the fault precedence across two rules is proved per FR-003, FR-035 (partial)
- [X] T067 Point the compare citations of `source/harness/detail/compare.hpp` at the CLI contract of specs/015 and refresh the line anchors of `source/harness/compare.cpp` and `contracts/compare-options.md` per Principle IV (partial)
- [X] T068 Move the contract block of the noise account from its struct to its function and reword the README option row per Principle IV, Principle XI.2 (contradicts)
- [ ] T069 Enforce the documented preconditions of the internal compare calls of `source/harness/compare.cpp` with `SG_REQUIRE`, the way `source/harness/family.cpp` and `source/harness/percentiles.cpp` enforce theirs, per Principle II (partial)
- [ ] T070 Hoist the object and array writers of `source/harness/compare.cpp` and the unit-scale table into a shared internal header beside `source/harness/json_report.cpp` and `source/harness/report.cpp`, per Principle X.3 (partial)
- [ ] T071 Carry the three compare options in the option table of `specs/015-benchmark-harness-core/contracts/cli.md`, an edit that moves the line anchors four artifacts of specs/016, specs/023 and specs/026 cite (partial)
