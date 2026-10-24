# Tasks: Hunter mode core (event ranking)

**Input**: Design documents from `specs/029-hunter-core/`

**Prerequisites**: plan.md, spec.md, research.md (PD-01 to PD-20),
data-model.md (E-01 to E-11, invariants I-1 to I-8), contracts/hunt-api.md,
contracts/hunt-cli.md, contracts/hunt-json.md, contracts/report-schema-key.md,
quickstart.md

**Tests**: mandatory. The verification tables in the four contract
files and the Test Plan table in plan.md name the test file for every decision,
so every implementation task below stands beside a test task. Coverage is 100
percent line and 100 percent branch with no exclusion marker and no waiver
(FR-091), and no test sleeps, reads a clock or assumes a CPU count (FR-092).

**Organization**: tasks are grouped by user story so each story can be
implemented and tested on its own. Plan phase numbers (P1 to P10) and gate
numbers (V-1 to V-10) are named in each phase heading so the mapping to plan.md
stays auditable.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: runs in parallel with the other **[P]** tasks of its phase once the
  unmarked tasks above it have landed. The write sets are disjoint and no
  dependency on an open task is left open. Two tasks that edit the same file
  never both carry **[P]**.
- **[Story]**: `US1` to `US4`, from spec.md. Setup, Foundational, the shared-list
  phase and Polish carry no story label.
- Every task names the exact file it creates or edits.

## Path Conventions

Single project, repository root layout (plan.md, Project Structure):
`include/speedgun-ng/` public headers, `source/harness/` harness translation
units with internals under `source/harness/detail/`, `source/counters/` the
counters library, `test/source/` one translation unit per test executable with
shared fixtures under `test/source/detail/`, `example/docs/` the documentation
examples with their recorded runs, `docs/` the schema and the guide page,
`tools/docs/` the documentation gates.

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: the numbers, the new files and the test registrations, so the
`ci-rocky` build carries the whole feature tree before any behaviour exists.
**Plan phase**: P1, gate V-1, decisions PD-20 and PD-18.

- [x] T001 [P] Edit `CMakeLists.txt`: the `VERSION 0.16.0` line at line 9 to `0.19.0` with the comment that states the specs/027 `0.17.0` and specs/028 `0.18.0` reservations and the rule that rebase takes the next free minor (FR-090, PD-20); the archive `SOVERSION 3` at line 53 to `4` and the harness `SOVERSION 10` at line 261 to `11`, each with a new comment line in the form the blocks at `CMakeLists.txt:43-52` and `CMakeLists.txt:238-260` state, naming `hunt.hpp` plus `BenchmarkResult::hunt` plus `ReportData::hunt` for the harness and the scripted refusal inside the exported `FakeProvider` for the archive (hard rule 12)
- [x] T002 Create `include/speedgun-ng/hunt.hpp`: the guard, license and export-macro form copied from the head of `include/speedgun-ng/benchmark.hpp`, `namespace sg::hunt`, the forward declarations `enum class MetricMethod : std::uint8_t;` and `class BenchmarkHandle;` in `namespace sg` (the pattern `source/harness/detail/pass_plan.hpp:27` carries), and the declarations of E-02 `HuntNorm` and `HuntRank`, E-03 `HuntOptions` with `std::optional<HuntNorm> norm` and `HuntBudget`, E-01 `CatalogAddress` and `EventCatalog`, E-04 `HuntColumn`, `HuntStage` and `HuntPlan`, E-05 `HuntSample` and `EventSamples`, E-06 `SkippedKind`, `SkippedEvent`, `RankedEvent` and `EntryHunt`, E-08 `HuntStageRange` and `RunHunt`, plus the `planHunt`, `rankEvents` and `runHunt` signatures, each with its `\pre`, `\post` and `\invariant` doc lines copied from contracts/hunt-api.md in the `source/harness/detail/pass_plan.hpp:63-107` form so `dbc-gate` pairs one macro per doc line (FR-093, hard rules 5 and 6, D-15: no `detail/` type in any signature)
- [x] T003 [P] Edit `include/speedgun-ng/benchmark.hpp`: include `speedgun-ng/hunt.hpp` and add `std::optional<hunt::EntryHunt> hunt;` to `BenchmarkResult` and `std::optional<hunt::RunHunt> hunt;` to `ReportData` (the block at `benchmark.hpp:624-641`), both empty for a run that carried no hunt (PD-10, I-8)
- [x] T004 [P] Edit `source/harness/detail/internal.hpp`: add `std::optional<hunt::HuntOptions> hunt;` to `RunOptions` (the struct at line 434) with the E-09 comment that states it stands empty when the run carries no hunt, and include `speedgun-ng/hunt.hpp`
- [x] T005 [P] Create the four new harness translation units `source/harness/statistics.cpp`, `source/harness/hunt_plan.cpp`, `source/harness/hunt.cpp` and `source/harness/hunt_cli.cpp` so they are picked up by the `CONFIGURE_DEPENDS` glob at `CMakeLists.txt:196-200` with no source-list edit, each holding the includes and the namespace of the definitions its later task adds (plan.md, Physical view)
- [x] T006 [P] Create the seven test translation units `test/source/harness_hunt_plan_test.cpp`, `test/source/harness_hunt_rank_test.cpp`, `test/source/harness_hunt_collect_test.cpp`, `test/source/harness_hunt_cli_test.cpp`, `test/source/harness_hunt_report_test.cpp`, `test/source/harness_hunt_json_test.cpp` and `test/source/harness_cli_table_test.cpp` in the assertion-helper form of `test/source/harness_pass_report_test.cpp`, each empty of cases until its story fills it
- [x] T007 Edit `test/CMakeLists.txt`: one `add_executable` with `target_link_libraries`, `target_include_directories`, `target_compile_features` and `add_test` per new test, the block pattern at `test/CMakeLists.txt:1079-1083`, plus `target_compile_definitions` with `SG_CLI_SOURCE="${CMAKE_SOURCE_DIR}/source/harness/cli.cpp"` on `harness_cli_table_test` and `SG_SCHEMA_PATH="${CMAKE_SOURCE_DIR}/docs/schema/report-v1.json"` on `harness_hunt_json_test`, the compiled-in path pattern at `test/CMakeLists.txt:607`
- [x] T008 [P] Create `test/source/detail/hunt_fixture.hpp`: the scripted host built on `test/source/detail/pass_fixture.hpp`, registering the core device objects `cpu`, `cpu_core` and `cpu_atom`, an `armv8_pmuv3` device, one uncore device, 200 catalog events with known values, `cpu-cycles` and `instructions` on each core object, and the scripted `programmableCounters` limits 1, 3, 4, 6 and 8 plus the no-limit shape (PD-02, FR-003: no sysfs seam, no host dependency)

**Checkpoint (V-1)**: `cmake --preset=ci-rocky` then `cmake --build build` builds
with every new file present, and `ctest --test-dir build -j 2 --output-on-failure`
passes with the seven new test executables registered and empty.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: the two shared changes every story reads. The statistics helper moves
out of `source/harness/runner.cpp` so a hunt CV stands at one hundred times the
printed `cv=` column by construction (PD-05), and the scripted group refusal
enters the exported `FakeProvider` so the retry of D-18 is reachable from a
harness-level test (PD-09, spec PC-3).

**Plan phases**: P2 and P3, gates V-2 and V-3.

**CRITICAL**: no story test compiles or runs until this phase is complete.

- [x] T009 Create `source/harness/detail/statistics.hpp` and fill the `source/harness/statistics.cpp` that T005 created: move `statisticsOf` out of the anonymous namespace of `source/harness/runner.cpp:49-82` into `sg::detail` unchanged in behaviour, keeping the even-count average-of-two median and the `n - 1` deviation, keeping both guards exactly as `runner.cpp:52-55` and `runner.cpp:74-76` hold them so every caller keeps its own empty-list and single-value guard, and writing it as a named function with its doc lines so `tools/dbc/dbc_pair_gate.py` pairs one macro per line (PD-05, FR-093; a header-plus-translation-unit pair is the house shape of `detail/percentiles.hpp` with `percentiles.cpp`)
- [x] T010 Edit `source/harness/runner.cpp`: include `detail/statistics.hpp`, delete the TU-local copy, and repoint the three call sites at `runner.cpp:2569`, `runner.cpp:2601` and `runner.cpp:2634` while each keeps its `empty()` guard that answers the zero record. Verification for this pair is the existing `test/source/harness_statistics_test.cpp` plus `test/source/harness_pass_report_test.cpp`, both of which must pass with no edited expectation, including its `byte-identity` entry at `test/CMakeLists.txt:1089-1091` (V-2, plan Test Plan Statistics row)
- [x] T011 [P] Edit `include/speedgun-ng/counters_fake.hpp`: declare `auto setGroupLeafLimit(std::string_view objectPath, std::size_t leaves) -> FakeProvider&;` in the public setter block at lines 135-248 with the `\pre` and `\post` doc lines of contracts/hunt-api.md, and add the scripted leaf count beside `programmableCounters` in `ObjectSeedData` at line 268 (E-10)
- [x] T012 [P] Add to `test/source/counters_fake_test.cpp`: a wide leaf set refused for the scripted object with the object path and the leaf count in the message, the three-leaf set of one event with both anchors admitted, an object with no scripted limit keeping its present behaviour, and the last call for an object winning (V-3)
- [x] T013 [P] Edit `source/counters/fake_provider.cpp`: refuse in `open` a leaf set that carries more leaves of the scripted object than its scripted count, naming the object path and the leaf count; the scripted member moves the exported layout of the `final` class, which is the archive `SOVERSION` 3 to 4 of T001 (hard rule 12)
- [x] T014 Read the foundation gates: `ctest --test-dir build -R 'harness_statistics_test|harness_pass_report_test|counters_fake_test|counters_objects_limit_test' --output-on-failure`, and the `ci-ubuntu` analyzer build once here because this is the first library change: `cmake --preset=ci-ubuntu` then `cmake --build build` (Principle IX)

**Checkpoint (V-2, V-3)**: the statistics helper serves the three runner call
sites and both existing report tests pass unchanged with the byte-identity golden
still matching; `counters_fake_test` refuses a wide leaf set and admits a narrow
one; `counters_objects_limit_test` still passes; the `ci-ubuntu` analyzer build is
clean.

---

## Phase 3: User Story 1 - Find the events that move the benchmark (Priority: P1) 🎯 MVP

**Plan phases**: P4, P5 and P8, gates V-4, V-5 and V-8, decisions D-1 to D-13,
D-15, D-18, D-19, D-20, D-23 with PD-01 to PD-08, PD-11, PD-12, PD-14, PD-16,
PD-17. T018, T019, T020, T027, T028, T029 and T030 carry the option seam, the
console printers and the table audit of plan P8 and gate V-8; the
`harness_hunt_report_test.cpp` clause of V-8 runs at T037; the `cli_shape.sh`
row of T030 is V-8's own check.

**Goal**: `--hunt=all` lists the core catalog events, plans one column per event
with the two anchors of the same device through the specs/024 planner, runs them
through the specs/024 pass loop for `--hunt-repeats` repeats, normalizes every
event against its own pass, and prints the ranked table with the marked tail and
the skipped list.

**Independent Test**: `harness_hunt_plan_test` drives the limit rows of AC-1 and
AC-2, `harness_hunt_rank_test` drives the scripted rank table of AC-3, and
`harness_hunt_collect_test` drives the skipped kinds of AC-4 and the all-refused
host of AC-1 scenario 4. No option, no JSON and no documentation is needed for
those three, and none of them needs PMU access.

### Tests for User Story 1

- [x] T015 [P] [US1] Fill `test/source/harness_hunt_plan_test.cpp` with the discovery and plan tables on `test/source/detail/hunt_fixture.hpp`: one column per event holding the event plus the two anchor leaves of its own device and no third column shape (I-1); the limit rows 1, 3, 4, 6 and 8 plus no limit; 200 events at limit 6 giving 50 passes of 4 events with `cpu-cycles` and `instructions` claimed exactly once per pass (AC-1, SC-01); limit 3 giving one event per pass; a device publishing a limit below 3 with both anchors present answered by the recoverable error naming the device and the limit; an empty event list answered by zero columns, empty passes, `anchorsPerPass` 0 and `normalized` false and no error (the D-12 shape, FR-041); catalog order and deduplication by address with the repeat keeping its first position (FR-002); the `--hunt-seed` permutation equal for two runs of one seed and different for two seeds (D-9, PD-06); `isCoreDevice` true for `cpu`, `cpu_core`, `cpu_atom` and the `armv8_pmuv3` fixture device and false for the uncore device (D-1, PD-02); `--hunt-norm=cycle` and `--hunt-norm=instr` answered by the refusal when that anchor stands refused while the unset norm plans `iter` with `normalized` false (D-13); one uncore address through a list planning on its own device (D-1)
- [x] T016 [P] [US1] Fill `test/source/harness_hunt_rank_test.cpp` with the rank decision table over literal `EventSamples`: `instr` dividing by the `instructions` count of the same pass times 1000, `cycle` by the `cpu-cycles` count of the same pass, `iter` reporting the raw per-iteration median (D-6, PD-08); a sample whose anchor read zero contributing no rate for that pass; a sample carrying a disclosure contributing none at all; `rate` the `statisticsOf` median and `cv` one hundred times the `coefficientOfVariation` of the same list, checked against `statisticsOf` itself (I-6, FR-030); `noisy` read on the unrounded `cv` against `--hunt-noise` (D-7); rank order `rate` descending and `cv` ascending with the tie broken by address ascending (PD-07); at most `--hunt-top` ranks, `1` to `k` with no gap, and no rank for a noisy or unrepeated entry (I-5); one sample giving `cv` `0.0`, `repeated` false and no rank (PD-14); `multiplexed` from the settled method; entries ranked first then in address order; two calls over equal arguments equal (I-7)
- [x] T017 [P] [US1] Fill `test/source/harness_hunt_collect_test.cpp` with the scripted runs: a high-rate scripted event ranking first and a flat scripted event above the CV threshold marked noisy, kept out of the top N and kept in the record (AC-3, SC-03); one scripted event per refused `Availability` standing in the skipped list with its kind and the run exiting 0 (AC-4, SC-04, D-10); nineteen clean windows plus one gapped window standing in `entries` and in `skipped` with kind `gap` (FR-022); the all-refused host printing the plan line with `events=0`, no ranked row, the skipped list and the summary `events=0/N` and exiting 0 (D-12, FR-041); the no-limit device settling `MULTIPLEX` and marking every event multiplexed (AC-2, SC-02, D-19); `setGroupLeafLimit` refusing the wide pass, the run retrying its events one at a time inside the same pass loop, `retriedPasses` counting the pass, an event that still refuses joining the skipped list with `EINVAL` and `EOPNOTSUPP` mapped to `not-encodable` and `EACCES` and `EPERM` mapped to `permission-blocked`, and the run exiting 0 (D-18, FR-021); a pass-0 seat failure keeping the present failure route with exit 1 (spec PC-9); the two-context threaded run reading window-open counts from the scripted provider at one per context per pass with no sleep, no clock and the CPU set from `sched_getaffinity` (FR-092)
- [x] T018 [P] [US1] Fill `test/source/harness_hunt_cli_test.cpp` with one case per refusal row of contracts/hunt-cli.md, each asserting one stderr line naming the value and status 2 through in-process `speedgunMain` and through a forked child where the refusal path calls `std::exit`, the pattern of `test/source/harness_cli_test.cpp:800`, `:1166` and `:1417` (FR-081, AC-5, SC-05): `cpu/nope`, a pattern that matches nothing, a missing `@file`, a `@file` that holds no address, `r4a03` bare and inside a list and inside a pattern and on a file line, `--hunt-norm=ipc`, the exit-2 refusal naming the device whose published limit stands below 3 (the binding rule that no planning is attempted there), `--hunt-rank=sensitivity`, `delta` and `score`, `--hunt-top=0` and `10001`, `--hunt-noise=0` and negative, `--hunt-repeats=0` and `1001`, `--hunt-budget` of `120`, `0s`, `0p`, `-5p` and `abc`, `--hunt-seed=abc`, and `--hunt` with `--counter`, `--metric` or `--compare` in both command-line orders (D-16); the accepted forms `all`, `none`, a comma and newline list, a glob, `@file` with comments and blank lines, and a duplicate across a list and a file keeping one column (FR-001, FR-006); a bare `--hunt` selecting the whole core catalog (D-1); `--hunt=none` behaving exactly like a run with no `--hunt` (spec Assumptions); `--hunt` with `--metric-method=multiplex` running every event multiplexed with the word in the plan line, the rows and the document (D-16, D-19); `--dry-run` with `--hunt` running one iteration per pass and printing one unrepeated row per column (contracts/hunt-cli.md accepted combinations); `--hunt` with `--list`, `--catalog` or `--topology` printing and stopping with no `hunt:` line; `--hunt` with `--filter` applying to the filtered instances; a hunt option given twice keeping the last value
- [x] T019 [P] [US1] Fill `test/source/harness_cli_table_test.cpp` to read `cli.cpp` through `SG_CLI_SOURCE`, parse the one `option longOptions[]` table and the named value constants, and assert that no option name repeats, that no option value repeats counting the named constants as their numbers, that every hunt value lies in 270 to 279 with the eight hunt constants at 270 to 277, and that the fixture twin of T020 is reported for both its planted clash and its planted duplicate (D-21, FR-083, PD-18)
- [x] T020 [P] [US1] Create `test/source/detail/cli_table_fixture.hpp`: the fixture twin of the option table with one planted duplicate name and one planted value clash, in the source spelling T019 parses (PD-18)

### Implementation for User Story 1

- [x] T021 [P] [US1] Edit `source/harness/detail/hunt_plan.hpp`: the internal records `StageSplit` with its cut verdict and `RetriedPass`, and the named internal declarations for the discovery, plan, split and rank cores behind the public calls, each with its doc lines and no `detail/` type reaching a public signature (D-15, data-model preamble)
- [x] T022 [US1] Implement `EventCatalog` in `source/harness/hunt.cpp`: the `children()` walk of `source/harness/catalog.cpp:83-92` under the `object("machine")` lookup at `:90`, the address composed as `Object::path() + "/" + entry.name` (spec PC-1), `expand` for `all`, one glob and a comma or newline list with the raw-code shape `r[0-9a-fA-F]+` refused before the catalog is consulted (PD-16, FR-005), deduplication by address in catalog order (FR-002), `split` dividing on `Availability::COUNTABLE` into columns and skipped records with that kind (FR-010), the `splitAddress` split of `runner.cpp:39-47` and the `isCoreDevice` rule of the three exact parts plus the `armv8_` prefix written hunt-side (PD-02), and the in-repo 64-bit linear congruential Fisher-Yates over the deduplicated order seeded by `--hunt-seed` (PD-06, FR-014); no clock, no counter read, no sysfs and no CPUID anywhere in the file (FR-003, I-7)
- [x] T023 [US1] Implement `planHunt` in `source/harness/hunt_plan.cpp`: one `HuntColumn` per event with the `cpu-cycles` and `instructions` anchors of the event's own device and both empty when the device lacks either (PD-04, I-1); the `std::vector<std::vector<ColumnLeaves>>` emission in the shape of `source/harness/detail/pass_plan.hpp:41-107` and the unchanged `planPasses` call with `sourceLimits` built per device from `Object::programmableCounters()` (D-19, D-23, spec PC-2); `anchorsPerPass` 2 or 0 and `normalized` false exactly then; the `max(L - 2, 0)` limit rule of I-3 stated in the post-condition; the refusals of a limit below 3 and of an out-of-range `source` index; `method` from `settleMethod` with `MULTIPLEX` exactly when no needed device publishes a limit (FR-011); `planHunt` adding no pass loop of its own and reading no clock (FR-020)
- [x] T024 [US1] Implement `rankEvents` in `source/harness/hunt_plan.cpp`: the per-pass normalization of PD-08 before the median, one `statisticsOf` call per event for the median and the coefficient of variation with the times-one hundred for `cv` (I-6, FR-030), the one-sample event held out of the call and reported at `cv` `0.0` (PD-14), the noise comparison on the unrounded value, the rank order and the address tie break of PD-07, the top N of I-5, and the two exclusion rules of contracts/hunt-api.md
- [x] T025 [P] [US1] Edit `source/harness/runner.cpp`: `seatPass` at `runner.cpp:1400` keeps its `std::expected` answer and the pass loop at `runner.cpp:2286-2292` branches on it, taking the retry route only when the run carries hunt columns and keeping the `SG_INVARIANT` route byte for byte for every other run (PD-11, R-1); the pass column list of `runner.cpp:1363-1367` becomes growable so a failed pass appends one pass per event of that pass, each one event with the two anchors, and the same loop runs them with no nested loop (PD-12, D-18, hard rule 15); the pass-0 seat of `runner.cpp:1434` keeps its failure route; a hunt run marks a lone event that still fails as unavailable the way an unavailable column is marked today and continues to exit 0
- [x] T026 [US1] Implement `runHunt` in `source/harness/hunt.cpp`: the plan through the existing `Runner` and pass loop with no second counter path (D-23), one `HuntSample` per measured repetition of one column of one pass with its own anchor values, its iteration count and its disclosure (E-05), the `SkippedKind` mapping from `Availability` and from the refusal error through one named function (D-10, D-18), `EntryHunt` assembly with `passCount`, `top`, `entries`, `skipped` and `retriedPasses`, and `ReportData::hunt` filled with the E-08 `RunHunt` from the plan and the stages that ran (PD-10)
- [x] T027 [P] [US1] Edit `source/harness/report.cpp`: append `printHuntPlan`, `printHuntRanking` and `printHuntSkips`, printing the seven line shapes of data-model E-07 behind the `hunt:` prefix on stdout in the PD-15 order after `printResult` at `report.cpp:421`, reusing `availabilityName` at `report.cpp:38` and `metricMethodName` at `report.cpp:218` so no second name table enters the tree (D-20, FR-040), with the plan line before the first row and `budget=none` until the budget of US2 lands, the marked tail at `rank=-` with `noisy`, `unrepeated` or `beyond-top`, and the summary line `events=<ran>/<planned>` (PD-14, PD-15)
- [x] T028 [P] [US1] Implement in `source/harness/hunt_cli.cpp`: the eight value parsers with their ranges, the refusal texts of contracts/hunt-cli.md printed by this file as one line to stderr before `std::exit(2)` in the form of `rejectPercentileList` at `cli.cpp:73-81` (PD-16, FR-081), the `@file` reader that treats `#` as a comment start and ignores blank lines (FR-006), the case-sensitive glob matcher over catalog addresses (`*` any run, `?` one character), the raw-code shape refusal that runs before the catalog is consulted (FR-005, D-11), the expansion that runs after the option scan where the providers stand registered (`cli.cpp:326-339`), and `validateHuntCombination(const RunOptions&)` reading `counterAddresses`, `metricNames` and the compare baseline members of `source/harness/detail/internal.hpp:434-476` (E-09, D-16)
- [x] T029 [P] [US1] Edit `source/harness/cli.cpp`: the eight named constants beside the comparison constants at `cli.cpp:69-71`, `kHuntOption` 270, `kHuntNormOption` 271, `kHuntRankOption` 272, `kHuntTopOption` 273, `kHuntNoiseOption` 274, `kHuntRepeatsOption` 275, `kHuntBudgetOption` 276, `kHuntSeedOption` 277 with the comment form of `cli.cpp:66-68`, the eight rows of the one `option longOptions[]` table at `cli.cpp:346` with `{"hunt", optional_argument, nullptr, kHuntOption}` the tree's first optional-argument row and the other seven `required_argument`, eight switch arms that call one `hunt_cli` function each, and one `validateHuntCombination` call after the scan: table rows and calls only, no parse body here (D-22, FR-082, PD-17)
- [x] T030 [US1] Read the story gates: `ctest --test-dir build -R 'hunt|cli_table' --output-on-failure`, `bash test/cli_shape.sh` clean with one interrupt line and no second flag read, `ctest --test-dir build -R harness_pass_report_test` green with the byte-identity entry proving the non-hunt seat route did not move, and the `ci-ubuntu` analyzer build once more because this is the first harness behaviour change (Principle IX, V-4, V-5, V-8)

**Checkpoint**: a scripted host of 200 events at limit 6 plans 50 passes of four
events with both anchors once each, the scripted high-rate event ranks first, the
flat high-CV event is marked noisy and stands out of the top N, every refused
event stands in the skipped list with its kind, and a host that refuses
everything prints the plan, the skipped list and the summary and exits 0.

---

## Phase 4: User Story 2 - Bound the time and see the plan (Priority: P2)

**Plan phase**: P6, gate V-6, decisions D-2, D-6, D-16, D-17, D-21, PD-13, PD-15.

**Goal**: `--hunt-budget` bounds the collection, the plan prints before anything
runs with its passes, method and estimate, and a two-stage plan runs stage 1 over
every event at one repeat and stage 2 over the top `3 x top` events at
`--hunt-repeats`, dropping stage 2 whole when the two-stage estimate does not fit.

**Independent Test**: `harness_hunt_plan_test` drives the budget parse rows and
the stage rule of AC-7 against a fixed seed and a fixed estimate, and
`harness_hunt_report_test` drives the printed plan, estimate and cut lines from
captured stdout on the scripted provider.

### Tests for User Story 2

- [x] T031 [P] [US2] Add the budget and stage tables to `test/source/harness_hunt_plan_test.cpp`: the parse rows `120s`, `8p`, `0s`, `1p`, `0p`, a value with no suffix, a negative value and a non-numeric value (FR-012); the AC-7 row of 200 events at limit 6 with `--hunt-top=4` giving stage 1 over every column at one repeat and stage 2 over the top 12 columns at `options.repeats` (SC-07); a `3 x top` set capped at the event count; an estimate that does not fit the budget answering one stage with the cut verdict naming the reason and stage 2 absent, not shortened (D-2); stage 1 never shrinking whatever the budget (FR-013); `estimateNs` the calibration figure times passes times repeats with the stage-1 figures on a split plan, and empty when no calibration answered (FR-012)
- [x] T032 [P] [US2] Fill `test/source/harness_hunt_report_test.cpp` with the console tables: the plan line before the first row with the quickstart figures `events=200 passes=50 method=PASSES estimate=96000000ns budget=8p` beside the `budget cut` line that names the dropped stage (quickstart.md US2 walkthrough); a `60p` run carrying both stages and no cut line; `estimate=not available` when no calibration answered; the method word `MULTIPLEX` in the plan line of a no-limit host (D-19); the PD-15 order of plan line, benchmark tables, ranked table, marked tail, skipped list and plan summary; every line shape of data-model E-07 character for character; a two-stage run whose stage-1-only address reaching the printed table through the record built from its stage 1 samples (PD-13); `--hunt=none` printing no `hunt:` line at all (FR-040)

### Implementation for User Story 2

- [x] T033 [US2] Implement `splitStages` in `source/harness/hunt_plan.cpp` returning the `StageSplit` of T021: one stage at `options.repeats` when no budget stands, otherwise stage 1 over every column at one repeat and stage 2 over the top `3 x top` columns by stage-1 rate at `options.repeats`, with the cut verdict answering one stage when the two-stage estimate exceeds the budget in the unit the caller named, converting seconds to passes by the calibration figure (D-2, FR-012, FR-013)
- [x] T034 [US2] Add the `--hunt-budget` parse to `source/harness/hunt_cli.cpp`: the `<n>s` and `<n>p` forms with `<n>` above zero, the refusal texts `--hunt-budget: expected <n>s or <n>p, got '<text>'` and `--hunt-budget: expected a count above 0, got '<text>'`, and the settled `HuntBudget` in the E-09 field of `detail/internal.hpp` (FR-012, contracts/hunt-cli.md)
- [x] T035 [US2] Edit `source/harness/report.cpp`: the `budget=` field of the plan line and of the plan summary line in the unit the caller named, the estimate line naming the seconds budget and both stages with their event counts, and the one `budget cut` line naming the reason the cut happened (FR-012, FR-013, D-2)
- [x] T036 [US2] Edit `source/harness/hunt.cpp`: one `Runner::run` call per stage over the same instance, the stage 2 result merged with the stage 1 records per address with stage 2 winning, and a column stage 2 did not measure keeping its row through the `RankedEvent` and `EntryHunt` entry built from its stage 1 samples, so one printed row set and one document stand for the run (PD-13, I-4)
- [x] T037 [US2] Read the story gates: `ctest --test-dir build -R 'harness_hunt_plan_test|harness_hunt_cli_test|harness_hunt_report_test' --output-on-failure` and `python3 tools/prose/prose_gate.py --check prose --mode tree` over the files this phase touched (V-6)

**Checkpoint**: the plan prints before the first row with its passes, method and
estimate; a fit budget runs stage 1 over all events and stage 2 over the top
`3N`; an unfit budget prints the plan, the `budget cut` line and stage 1 alone;
a stage-1-only event still reaches the table and the record.

---

## Phase 5: User Story 3 - Read the hunt from the report (Priority: P3)

**Plan phase**: P7, gate V-7 (conformance half), decisions D-17, D-19, PD-10,
hard rules 7, 8, 11 and 12. The schema keys of T047 ride in this phase: the
benchmark entry node of `docs/schema/report-v1.json` is closed and `json_load`
turns a key a closed node does not list into a hard refusal, so the writer of
T039 needs the key present for the document to stay readable and conformant.
The `schemaVersion` stamp and the rows that name that version stay in Phase 7,
which closes the version half of V-7.

**Goal**: a hunt run writes the top-level `hunt` object and the per-entry `hunt`
object through the existing writer, `loadReport` reads both back field for field,
and a run without `--hunt` writes no `hunt` key anywhere.

**Independent Test**: `harness_hunt_json_test` round-trips a hunt document and
asserts both objects against the tables of contracts/hunt-json.md; the existing
`harness_json_load_test` proves a document with no `hunt` key still loads with
both members empty.

### Tests for User Story 3

- [x] T038 [US3] Fill `test/source/harness_hunt_json_test.cpp`: both objects carrying every key of the two tables of contracts/hunt-json.md in the `x-order` of the schema; a field-by-field round trip through `loadReport` including a `rank` that is null, a `budget` that is null, an `estimate_ns` that is null and a `skipped` stage that is null (FR-051, AC-6); one case per refused reader code naming the pointer, a number where a string stands, a string where a number stands, a boolean where a number stands, an unknown word for `method`, `norm`, `rank` or `kind`, an integer too wide for its field, an array where an object stands and a missing key of a present hunt object (hard rule 7); a document with no hunt keys loading with both members empty; a document written before the feature loading unchanged; a two-stage document holding two `stages` objects, the planned addresses in `events` and a stage-1-only address in `entries`; conformance through `SG_SCHEMA_PATH` with a planted out-of-order hunt key and a planted key outside a closed hunt node both reported as findings naming the pointer, green once T047 lands the schema keys; a run with no `--hunt` writing bytes identical to the master capture apart from `schemaVersion` with the same console and the same status (FR-052, SC-006)

### Implementation for User Story 3

- [x] T047 [US3] Edit `docs/schema/report-v1.json`: append `hunt` to the root `x-order` at line 7 and to the root `properties`, append `hunt` as the 22nd entry of the entry `x-order` at lines 28-50 and to the entry `properties`, add the two closed hunt nodes of contracts/report-schema-key.md with their own `x-order`, `required` and `additionalProperties` false and their nested closed nodes for `stages`, `entries` and `skipped`, give the null-accepting kinds to `budget`, `estimate_ns`, `entries[].rank` and `skipped[].stage`, and change neither `required` list of the root and the entry node (D-17, FR-053)
- [x] T039 [P] [US3] Edit `source/harness/json_report.cpp`: add `writeHunt(builder, const RunHunt&)` called from `writeReport` after the `benchmarks` array at `json_report.cpp:583-598` and `writeEntryHunt(builder, const EntryHunt&)` as the last act of `writeEntry` at `json_report.cpp:500-510`, both through the `Builder` with the `JsonObjectWriter` and `JsonArrayWriter` helpers at `json_report.cpp:43-78` and the `put`, `putOrNull` and `enumWord` helpers the 023 and 024 keys use, `null` written through `putOrNull` for an absent budget, estimate, rank and stage, no raw string assembly and no new `LCOV_EXCL` marker (FR-050, hard rule 8, FR-091)
- [x] T040 [P] [US3] Edit `source/harness/json_load.cpp`: read the top-level key in `loadReport` at `json_load.cpp:1190` before the `benchmarks` array and the per-entry key in `readEntry` at `json_load.cpp:654`, both through `optionalField` at `json_load.cpp:103` so an absent key leaves the member empty and the rest of the document reads as it does today, with every key of a present hunt object required and a missing key refused by name and pointer in the `settled` and `missing` form of `json_load.cpp:81-118`, every value read through the `.get(out)` error-code form, and the schema-major gate at `json_load.cpp:1214-1219` untouched (FR-051, hard rules 7 and 11)
- [x] T041 [US3] Read the story gates: `ctest --test-dir build -R 'harness_hunt_json' --output-on-failure` for the round trip, the refusals and the empty-member cases, `ctest --test-dir build -R harness_json_schema_test --output-on-failure` for the conformance of the keys T047 lands (plan P7 names this test for the schema move), and `ctest --test-dir build -R 'harness_json_load_test|harness_json_report_test' --output-on-failure` unchanged

**Checkpoint**: a hunt document carries both objects, reads back field for field
including every null shape, refuses every bad code by pointer, and a run without
`--hunt` writes no `hunt` key at either level; the document conforms at both hunt
nodes, proved by `harness_json_schema_test` and the planted findings of T038, so
the conformance half of V-7 closes here.

---

## Phase 6: User Story 4 - Read the page and copy a working block (Priority: P3)

**Plan phase**: P9, gate V-9, decisions D-11, PD-19, IV.1, FR-070 to FR-072.

**Goal**: the main page carries a `Hunter mode` section whose snippets come from
`example/docs/hunt_rank.cpp`, that example builds and runs on every host, and its
recorded run matches its golden on every host because its provider is scripted.

**Independent Test**: `run_docs_hunt_rank` passes on this host and its output
matches `example/docs/hunt_rank.expected` on every host (AC-9); the copied block
builds against the canonical includes and runs (AC-8). The drift, snippet and
shape gates that read the page and the tool lists run after Phase 7 lands the
README section and the tool lists.

- [x] T042 [P] [US4] Create `example/docs/hunt_rank.cpp`: the marked regions that own the main-page section, benchmarks that register and print rows on every host, a scripted provider under `sg::counters` whose events stand not countable so the skipped list prints the same on every host (PD-19), and a hunt run through the public `sg::hunt` surface with the canonical include block the snippet gate copies (FR-070)
- [x] T043 [US4] Edit `example/CMakeLists.txt`: `add_docs_example(hunt_rank)` after the `regression_gate` line at line 74, the registration that creates `docs_hunt_rank` and `run_docs_hunt_rank` and that `test/example_run.sh:35-36` needs, because a docs example with no built binary fails that gate (FR-070)
- [x] T044 [US4] Create `example/docs/hunt_rank.expected`: the recorded run of `docs_hunt_rank` in the `# sg-example:begin report` form the other recorded goldens use, with the plan line, the marked tail and the skipped list as the scripted host prints them (FR-070, AC-9)
- [x] T045 [P] [US4] Edit `docs/pages/harness.md`: the `Hunter mode` option detail with the eight options, their defaults and their refusals, the two hunt objects with their key lists, the `MULTIPLEX` interaction of D-16, and the statement that a budgeted run measures everything once and the top set again (FR-071)
- [x] T046 [US4] Read the story gates: `cmake --build build --target run_docs_hunt_rank` with its output compared against `example/docs/hunt_rank.expected` the way `tools/docs/report_shape.py` compares it, `bash test/example_run.sh` for the example, and the AC-8 copy check that the section's C++ block builds against the canonical includes and runs (V-9 part 1)

**Checkpoint**: the example builds, runs, records and matches its golden on this
host, the skipped list it prints is host independent, and the guide page carries
the detail a reader needs before copying the block.

---

## Phase 7: Shared-List Integration

**Purpose**: the plan's Shared-list ledger closed here, the schema keys of T047
excepted because they ride with Phase 5 where a closed schema node needs them:
the version stamp and the rows that name that version, the README section
pointer and its option and quick-reference rows, the documentation tool lists,
the shape sample registration, and the audit that every ledger row landed. These are the files
specs/027 and specs/028 also edit, so they are read from master at rebase time
and carried together (R-2, hard rules 11 and 12). The test registrations and the
version numbers rode with Phase 1 because plan P1 and gate V-1 name them there,
and `example/CMakeLists.txt` rode with Phase 6 because the example does not build
without it.

**Plan phases**: P9 and the version half of P7; gates V-9 and the version rows
of V-7. PD-19, PD-20, FR-053, FR-070 to FR-073, FR-090.

- [x] T048 [P] Edit `source/harness/json_report.cpp`: `kSchemaVersion` at `json_report.cpp:31` from `1.1` to the reserved minor `1.4`, with the comment that names the two hunt keys and the rebase rule of taking the next free minor at rebase (FR-053, hard rule 11)
- [x] T049 Move the three rows that name the written version to the new value, once T048 has moved `kSchemaVersion`: `test/source/harness_json_load_test.cpp:113` the current-version document the loader test builds, `test/source/harness_json_load_test.cpp:529` the `schemaVersion` kind refusal row beside it, and `docs/pages/harness.md:1067` the sentence that states the version a run writes. `test/data/gate_schema_11.json` is a frozen `1.1` document: `harness_compare_cli_test.cpp:516` asserts that fixture's own header line and must stay, as must `harness_compare_cli_test.cpp:492`, `harness_compare_test.cpp:1191-1202` and the three `1.0` fixtures at `harness_json_load_test.cpp:158`, `:177` and `:507` (contracts/report-schema-key.md, "What the bump costs")
- [x] T050 [P] Edit `test/CMakeLists.txt`: the `report_shape_hunt_rank_sample` entry guarded by `if(TARGET docs_hunt_rank)` in the pattern of `test/CMakeLists.txt:1185-1192`, with `--binary $<TARGET_FILE:docs_hunt_rank>`, `--readme ${CMAKE_SOURCE_DIR}/docs/pages/harness.md` and `--example hunt_rank`, and `set_tests_properties(... TIMEOUT 120)` (PD-19)
- [x] T051 Edit `README.md`: the `### Hunter mode` section after `### Regression gate` at line 621 at or under the 25-line `SECTION_LINE_CAP`, sourced from the marked regions of `example/docs/hunt_rank.cpp` through its `<!-- sg-example: -->` markers and closed by a `See [Hunter mode](docs/pages/harness.md#hunter-mode)` pointer; the eight hunt option rows in the option table that ends at line 766; one quick-reference name row for each new public name in `include/speedgun-ng/hunt.hpp` (FR-070, FR-072, IV.1)
- [x] T052 Edit `tools/docs/example_drift.py`: `"Hunter mode"` in `SECTIONS` at line 32, `"Hunter mode": ("hunt_rank.cpp",)` in `SECTION_OWNERS` at line 63, and `PAGE_LINE_CAP` at line 155 from `773` to the measured length of the page after T051, with the move recorded in the comment block at lines 100-155; `SECTION_LINE_CAPS` gains no entry because 25 holds (FR-072)
- [x] T053 Audit the plan's Shared-list ledger row by row against the working tree: `CMakeLists.txt`, `test/CMakeLists.txt`, `example/CMakeLists.txt`, `README.md`, `docs/pages/harness.md`, `docs/schema/report-v1.json`, `tools/docs/example_drift.py`, `test/source/harness_json_load_test.cpp`, `test/source/harness_compare_cli_test.cpp`, `test/source/harness_cli_table_test.cpp` and `test/source/harness_pass_report_test.cpp`, and confirm that `harness_pass_report_test.cpp` and `harness_compare_cli_test.cpp` carry no edit: the byte-identity golden of the first at `test/CMakeLists.txt:1089-1091` is the covering check that a run with no hunt changed nothing, and line 516 of the second asserts the console header of the frozen `gate_schema_11.json` fixture, so the plan ledger row that once moved that value stands carried as no edit, recorded in the merge notes with its reason
- [x] T054 Read the documentation chain and the whole suite: `ctest --test-dir build -R 'example_drift|example_drift_fixtures|snippet_compile|report_shape|report_shape_fixtures|example_run|example_run_fixtures' --output-on-failure`, `python3 tools/prose/prose_gate.py --check prose --mode tree`, and `ctest --test-dir build -j 2 --output-on-failure` (V-7, V-9)

**Checkpoint (V-7 version rows, V-9)**: a non-hunt document differs from the
master capture in the `schemaVersion` value alone and the whole suite at T054
keeps the Phase 5 conformance green; the option table, the README option
rows, the quick-reference rows, the page budget and the section lists are
consistent; `run_docs_hunt_rank` matches `hunt_rank.expected`; the full suite is
green.

---

## Phase 8: Polish and Cross-Cutting Concerns

**Plan phase**: P10, gate V-10, FR-091 to FR-094, Principles VI, VII and VIII,
IV.1.

- [ ] T055 [P] Prove the coverage bar: `cmake --preset=ci-coverage`, `cmake --build build/coverage --target coverage`, 100 percent line, branch and DBC postconditions across `source/harness`, `include/speedgun-ng` and `example/docs`, `git grep -n 'LCOV_EXCL' -- source/harness include/speedgun-ng example test` answering no new marker, and a `gcov -b` line for any gap with the case that closes it (FR-091)
- [x] T056 [P] Prove determinism and no host dependency: `grep -nE 'sleep|steady_clock|system_clock|high_resolution_clock|hardware_concurrency' test/source/harness_hunt_*.cpp test/source/harness_cli_table_test.cpp test/source/detail/hunt_fixture.hpp` answering nothing that is not a pinned literal, every CPU-set dependent case reading `sched_getaffinity`, and `ctest --test-dir build -R 'hunt'` green on a two-CPU and a four-CPU set (FR-092, quickstart Step 6)
- [x] T057 [P] Run the analyzers and the format gate once more on the finished tree: `cmake --preset=ci-ubuntu` then `cmake --build build` clean of clang-tidy and cppcheck findings, and `cmake --build build --target format-check` clean (Principle VIII)
- [x] T058 [P] Run the sanitizer lanes: the `ci-sanitize` and `ci-tsan` routes with `--output-on-failure`, the two-context hunt case of T017 being the new thread-sanitizer entry, with the whole-suite timeout above 3600 s the sanitizer jobs need (plan.md, Verification Strategy)
- [x] T059 Run the quickstart end to end as the user-facing check of Step 1 to Step 8 of `specs/029-hunter-core/quickstart.md`, including the two greps that prove a non-hunt document carries no `hunt` key and no `hunt:` line and the four version-naming files holding no stale `1.1`
- [x] T060 [P] Record the AC-10 hardware run on `corsairone` into `specs/029-hunter-core/results/`: the `--hunt=all` invocation, `perf_event_paranoid`, the device parts the catalog listed, the published limits, the plan line with the measured passes and the top 20 with sample counts and CVs, together with the quickstart Step 7 scripted refusal that the rank equals the printed table within rounding (FR-093)
- [x] T061 [P] Record the second AC-10 host: the same invocation on `gary` (Zen 5) when the N7 lane leaves it free, so two microarchitecture families stand in the record (FR-093)
- [x] T062 Rebase onto `master` and retake the four reserved values from what master states: `CMakeLists.txt` `VERSION`, the harness and archive `SOVERSION` lines, `kSchemaVersion` with `docs/schema/report-v1.json`, carrying the keys of the lanes that merged before this one in the same schema document and recording each value with its reason in the merge notes (PD-20, hard rules 11 and 12, FR-094)
- [ ] T063 Re-measure `tools/docs/example_drift.py` `PAGE_LINE_CAP` and the section budgets against the rebased `README.md`, run every Principle VIII gate on the rebased tree, push once with `--force-with-lease`, record `git ls-remote origin 029-hunter-core` beside the gate output, and read every CI job after the push including the coverage artifact (FR-094, V-10)
- [x] T064 Make the main-page impact check of IV.1 and write its answer into the merge notes: the `Hunter mode` section teaches the capability with a snippet CI builds and runs, the quick-reference rows name the new public surface, and nothing on the page states a fact the code does not keep

**Checkpoint (V-10)**: 100 percent line, branch and DBC coverage with no marker
and no waiver, the rank equal to the printed table on the scripted run and on
both hardware records, no test sleeping or reading a clock, the full CI matrix
green on the pushed state including sanitizers, and the remote branch at the
recorded commit.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 (Setup)**: blocks everything. The version numbers, `hunt.hpp`, the
  `benchmark.hpp` fields, the four new translation units, the seven test
  registrations and the shared fixture.
- **Phase 2 (Foundational)**: blocks every story. `statisticsOf` is the rank's
  only source of a median and a CV (FR-030), and `setGroupLeafLimit` is the only
  way any harness test reaches a group-open refusal (spec PC-3).
- **Phase 3 (US1)**: the MVP. `EventCatalog` before `planHunt`, `planHunt` before
  `rankEvents`, both before `runHunt`; the seat route of T025 before the retry in
  T026; the printers of T027 and the option seam of T028 and T029 after the
  records they print. US2, US3 and US4 all build on it.
- **Phase 4 (US2)**: needs US1's plan and rank. `splitStages` before the two-stage
  run, the budget parser before the plan line's budget field.
- **Phase 5 (US3)**: needs US1's `EntryHunt` and `RunHunt` and US2's stage
  records; T047 lands the schema keys before the writers of T039 and T040 emit
  and read them. Independent of US4.
- **Phase 6 (US4)**: needs the CLI of US1 and the plan line of US2, and the writers
  of US3 for the recorded document shape. Independent of Phase 7 tasks in content,
  and its gates close in Phase 7.
- **Phase 7 (Shared-List Integration)**: needs US3 for the written keys and US4 for
  the example whose regions the README quotes and whose page length the budget
  measures. `README.md` before `example_drift.py`, then the ledger audit, then the
  chain.
- **Phase 8 (Polish)**: needs every story. Coverage before the rebase, the rebase
  before the push, the push before reading the CI jobs.

### Story Independence

- **US1** is the feature: discovery, plan, run, rank, skipped list. It stands with
  no budget, no JSON and no documentation.
- **US2** bounds and announces what US1 built. Its tests read the plan object
  and captured stdout; none of them reads the JSON.
- **US3** publishes what US1 and US2 decided. Its round trip needs only the
  records, and its conformance rows need the schema keys of T047, which ride in
  this story's phase.
- **US4** teaches the finished shape and owns its example.

### Within Each User Story

- Tests first, and they fail on the missing behaviour before the code lands.
- Catalog before planner, planner before ranker, ranker before driver, driver
  before printers, printers before the page.
- Story complete before the next priority starts.

### Parallel Opportunities

- Phase 1: T001 runs beside T002; once the header of T002 stands, T003, T004,
  T005, T006 and T008 run together on five disjoint files; T007 follows T006
  because it registers the files T006 creates.
- Phase 2: T011, T012 and T013 run together; T009 then T010 run in sequence on the
  statistics pair and its call sites.
- Phase 3: the six test tasks T015 to T020 run together, one file each; in
  `source/harness/hunt.cpp` T022 then T026 run in sequence, in
  `source/harness/hunt_plan.cpp` T023 then T024, and T021, T025, T027, T028 and
  T029 run beside those pairs on their own files.
- Phase 4: T031 and T032 run together on their two test files; T033 to T036 each
  own one file and run together; T037 closes the phase.
- Phase 5: T047 lands the schema keys first, then T039 and T040 run together,
  `json_report.cpp` and `json_load.cpp` being one writer and one reader; T038's
  cases go green with them.
- Phase 6: T042 and T045 run together; T043 then T044 run in sequence because the
  golden is recorded from the built binary.
- Phase 7: T048 and T050 run together on their two disjoint files; T049 follows
  T048 because it repoints the version rows at the value T048 writes; T051 then
  T052 run in sequence because the page budget measures the page T051 wrote;
  T053 and T054 close the phase.
- Phase 8: T055 to T058 run together, T060 and T061 run together on their two
  records, and T062 to T064 run in sequence.

---

## Parallel Example: User Story 1

```bash
# Six test files, six tasks, all at once:
Task: "Add the limit-row, discovery, dedup, isCoreDevice and refusal tables to test/source/harness_hunt_plan_test.cpp"
Task: "Add the normalization, CV factor, threshold, order, tie-break and single-sample tables to test/source/harness_hunt_rank_test.cpp"
Task: "Add the skipped-kind, gap, all-refused, retry, pass-0 and threaded cases to test/source/harness_hunt_collect_test.cpp"
Task: "Add the refusal-row, conflict and selection-form cases to test/source/harness_hunt_cli_test.cpp"
Task: "Add the option-table uniqueness cases and their planted fixture twin"
# Then the decision units, one file each, in dependency order:
Task: "Implement EventCatalog in source/harness/hunt.cpp"
Task: "Implement planHunt in source/harness/hunt_plan.cpp"
Task: "Implement rankEvents in source/harness/hunt_plan.cpp"
Task: "Open the seat error route and the growable pass list in source/harness/runner.cpp"
Task: "Implement runHunt in source/harness/hunt.cpp"
# Then the surface, three files at once:
Task: "Append the three hunt printers to source/harness/report.cpp"
Task: "Implement the eight parsers in source/harness/hunt_cli.cpp"
Task: "Add the eight constants, rows and switch arms to source/harness/cli.cpp"
```

---

## Implementation Strategy

### MVP First (User Story 1)

1. Complete Phase 1: the numbers, `hunt.hpp`, the two published fields, the test
   registrations, the shared fixture.
2. Complete Phase 2: the statistics move, the scripted group refusal.
3. Complete Phase 3: catalog, planner, ranker, seat route, driver, printers,
   option seam.
4. **Stop and validate**: AC-1 to AC-4 hold on the scripted provider with no
   budget, no JSON and no documentation, and a run without `--hunt` still prints
   and writes master's bytes.

### Incremental Delivery

1. Phase 1 and 2 move nothing a user sees.
2. US1 ranks events and lists what it skipped.
3. US2 bounds the run and prints the plan before it.
4. US3 puts both objects in the document.
5. US4 and Phase 7 teach it on the page and close the shared lists.
6. Phase 8 records the evidence and lands the branch.

### Rebase Note for the Three Parallel Lanes

specs/027 (N4) and specs/028 (N7) run in parallel on other machines. The shared
files are `CMakeLists.txt` with the version and the two `SOVERSION` lines,
`README.md` with its section, option and quick-reference rows, its page budget in
`tools/docs/example_drift.py`, `docs/schema/report-v1.json` with the schema minor,
`example/CMakeLists.txt`, `test/CMakeLists.txt`, `include/speedgun-ng/benchmark.hpp`,
`source/harness/detail/internal.hpp`, `source/harness/runner.cpp`,
`source/harness/report.cpp` and, hardest, `source/harness/cli.cpp`, which specs/028
rebuilds: every hunt line there is one table row, one named constant or one
switch arm calling a function that lives in `source/harness/hunt_cli.cpp`, so the
merge stays mechanical (D-22, R-2). Rebase onto `master`, keep both lanes' lines,
retake the version, the two `SOVERSION` numbers and the schema minor to the next
free values, carry the earlier lanes' schema keys, re-measure the page budget, run
every gate, then push once with `--force-with-lease` (FR-094).

---

## Notes

- [P] marks disjoint write sets with no dependency on an open task of the phase.
- Two tasks that edit the same file never both carry [P]: `CMakeLists.txt`,
  `test/CMakeLists.txt`, `README.md`, `example_drift.py`, `hunt.cpp`,
  `hunt_plan.cpp`, `hunt_cli.cpp`, `report.cpp` and each test translation unit
  each have one owner at a time.
- Every hunt branch is reachable from the scripted provider, so no task adds an
  exclusion marker and no task waits for PMU access (FR-091, FR-092).
- Stop at any checkpoint and validate the story on its own command line.

---

## Requirement coverage

Every FR of `spec.md` and every Success Criterion maps to at least one task.

| Requirement | Tasks |
| --- | --- |
| FR-001 to FR-003, FR-009, FR-010 | T015, T017, T022 |
| FR-004, FR-005, FR-007, FR-008 | T015, T018, T022, T028 |
| FR-006 | T018, T028 |
| FR-011, FR-020, FR-016 and FR-017 of specs/025 | T015, T023, T146 |
| FR-012, FR-013 | T031, T033, T034, T035 |
| FR-014 | T016, T022, T024 |
| FR-015, FR-018, FR-019 | T016, T024 |
| FR-021, FR-022, FR-025, FR-026 | T017, T025, T026 |
| FR-023 | T017, T027 |
| FR-024, FR-029 | T015, T017, T026 |
| FR-027, FR-028 | T017, T026, T036 |
| FR-030 | T009, T010, T016, T024 |
| FR-031 to FR-033 | T002, T021, T022 to T026, T030 |
| FR-040 to FR-043 | T017, T018, T027, T032, T035 |
| FR-050, FR-051 | T038 to T040 |
| FR-052 | T038, T047, T048, T054 |
| FR-053 | T047 to T049, T054 |
| FR-060, FR-061 | T018, T028, T029 |
| FR-062 | T018, T028 |
| FR-063, FR-064 | T018, T034 |
| FR-065 | T018, T028, T032 |
| FR-066 to FR-068 | T019, T020, T029, T033, T034 |
| FR-070 to FR-073 | T042 to T045, T050 to T052, T054, T064 |
| FR-080 to FR-083 | T018 to T020, T028, T029, T030 |
| FR-090 | T001, T048, T049, T062 |
| FR-091 | T012, T039, T040, T055 |
| FR-092 | T017, T018, T056 |
| FR-093 | T002, T009, T021, T030, T060, T061 |
| FR-094 | T062, T063 |
| SC-001, SC-002 | T015, T017 |
| SC-003, SC-004 | T016, T017 |
| SC-005 | T018, T030 |
| SC-006 | T038, T054 |
| SC-007 | T031, T032 |
| SC-008 | T017, T025, T026 |
| SC-009 | T042 to T046, T051, T054 |
| SC-010 | T017, T026, T058 |
| Scaffolding, shared registration and gate runs | T003 to T008, T014, T037, T041, T046, T053, T057 to T059, T064 |

## Phase 9: Convergence

- [x] T065 Pair every documented EventCatalog and runHunt contract with SG_REQUIRE, SG_ENSURE or SG_INVARIANT in source/harness/hunt.cpp and document its bare SG_ASSERT calls per source/harness/hunt.cpp (contradicts)
- [x] T066 Create example/docs/hunt_rank.cpp with marked report regions, a scripted not-countable provider, benchmarks that print rows on every host and the canonical include block per example/docs/hunt_rank.cpp (missing)
- [x] T067 Rewrite the 15 prose-gate findings in specs/029-hunter-core/tasks-review.md, tasks-review-2.md and the three plan-review files per specs/029-hunter-core/tasks-review-2.md (contradicts)
- [x] T068 Add SG_INVARIANT for the pass-coverage rule in planHunt and the uniqueness rule in rankEvents in source/harness/hunt_plan.cpp and document its bare SG_ASSERT per source/harness/hunt_plan.cpp (contradicts)
- [x] T069 Restate the HuntBudget, HuntOptions and RankedEvent contracts of include/speedgun-ng/hunt.hpp in the record form the other public records use per include/speedgun-ng/hunt.hpp (contradicts)
- [x] T070 Give EventCatalog a private expansion member in include/speedgun-ng/hunt.hpp and use it instead of the translation-unit static in source/harness/hunt.cpp per include/speedgun-ng/hunt.hpp (contradicts)
- [x] T071 Restore the single pass-0 seat failure route in source/harness/runner.cpp and pin the refused-group exit-1 case in test/source/harness_hunt_collect_test.cpp per source/harness/runner.cpp (contradicts)
- [x] T072 Print the two --hunt-norm anchor refusals of contracts/hunt-cli.md at exit 2 from source/harness/hunt_cli.cpp and add both rows to test/source/harness_hunt_cli_test.cpp per source/harness/hunt_cli.cpp (contradicts)
- [x] T073 Carry the solo retry counters::Error in source/harness/detail/internal.hpp and source/harness/runner.cpp and map it through skippedKindOf in source/harness/hunt.cpp per source/harness/detail/internal.hpp (contradicts)
- [x] T074 us1-4 and us2-5: print the hunt plan line before the benchmark rows from source/harness/report.cpp or source/harness/cli.cpp and pin that order in the collect test per source/harness/report.cpp (contradicts). Landed: printHuntPlan runs at report.cpp:455, above the rows, and the ranking and skip lines follow at 611-616. The order is pinned by `scenarioPlanLineStandsAboveTheRows` at test/source/harness_hunt_cli_test.cpp:942; the collect test pins the presence and the count of the plan lines it captures (harness_hunt_collect_test.cpp:670), and the order needs a full console capture, which the collect seam does not take
- [x] T075 us1-7 and us2-12: keep every settled hunt value, budget included, across option order in source/harness/hunt_cli.cpp and pin one option-before-hunt case per source/harness/hunt_cli.cpp (partial)
- [x] T076 Print the hunt plan budget cut line from printHuntPlan in source/harness/report.cpp when splitStages sets cutStage2 and pin its exact bytes per source/harness/report.cpp (missing)
- [x] T077 Fill the empty scenario list at test/source/harness_hunt_report_test.cpp:51 with the E-07 console shapes, the PD-15 order, the MULTIPLEX line and the --hunt=none silence per test/source/harness_hunt_report_test.cpp (missing)
- [x] T078 Add the T031 budget and stage tables to test/source/harness_hunt_plan_test.cpp: suffix refusals, the AC-7 limit-6 split, the 3 x top cap and the cut verdict per test/source/harness_hunt_plan_test.cpp (missing)
- [x] T079 Add a two-stage scripted run to test/source/harness_hunt_collect_test.cpp asserting one merged row set with a stage-1-only address present per test/source/harness_hunt_collect_test.cpp (partial)
- [x] T080 Feed the runner calibration figure into the runHuntNamed call at source/harness/cli.cpp so a seconds budget can weigh the estimate and print a figure per source/harness/cli.cpp (partial)
- [x] T081 Read the root hunt object and the per-entry hunt object in source/harness/json_load.cpp through optionalField, refusing a missing key by name and pointer per source/harness/json_load.cpp (missing)
- [x] T082 Fill test/source/harness_hunt_json_test.cpp with the round trip through loadReport including null rank, budget and estimate_ns, extending sameResult in json_fixture.hpp per test/source/harness_hunt_json_test.cpp (missing)
- [x] T083 Add one scenario per refused reader code, each naming the JSON pointer, to test/source/harness_hunt_json_test.cpp per test/source/harness_hunt_json_test.cpp (missing)
- [x] T084 Add the no-hunt byte-identity case with its committed master capture under test/data to test/source/harness_hunt_json_test.cpp per test/source/harness_hunt_json_test.cpp (missing)
- [x] T085 Add add_docs_example(hunt_rank) after the regression_gate line of example/CMakeLists.txt so docs_hunt_rank and run_docs_hunt_rank exist per example/CMakeLists.txt (missing)
- [x] T086 Record example/docs/hunt_rank.expected from the built docs_hunt_rank run in the sg-example report form, with the plan line, marked tail and skipped list per example/docs/hunt_rank.expected (missing)
- [x] T087 Move kSchemaVersion in source/harness/json_report.cpp, the two rows in test/source/harness_json_load_test.cpp and the harness.md version sentence to the reserved minor per source/harness/json_report.cpp (missing)
- [x] T088 Add the at-most-25-line Hunter mode section with its marked snippet region and its quick-reference rows to README.md after the Regression gate section per README.md (missing)
- [x] T089 Add the eight hunt option rows to the option table at README.md:745 so the example_drift option check stops reporting one finding per option per README.md (missing)
- [x] T090 Add one quick-reference name row per new public type of include/speedgun-ng/hunt.hpp to the name table of README.md per README.md (missing)
- [x] T091 Add hunt.hpp to the header tuple of header_names in tools/docs/example_drift.py so the name scan accepts the new public types per tools/docs/example_drift.py (partial)
- [x] T092 Add Hunter mode to SECTIONS and its hunt_rank.cpp owner to SECTION_OWNERS in tools/docs/example_drift.py per tools/docs/example_drift.py (missing)
- [x] T093 Add --timeout 3600 to the sanitize and tsan ctest steps in .github/workflows/ci.yml so the suite fits the budget T058 states per .github/workflows/ci.yml (partial)
- [x] T094 Write the corsairone --hunt=all AC-10 record with paranoid value, device parts, limits, plan line and top 20 counts and CVs into specs/029-hunter-core/results/ per specs/029-hunter-core/results/ (missing)
- [ ] T095 Run the coverage, analyzer, format and sanitizer lanes in lane-owned build directories, push once with --force-with-lease and read every CI job per specs/029-hunter-core/tasks.md T055, T057, T058, T063 (missing)
- [x] T096 Add SPEEDGUN_NG_EXPORT to the planHunt, rankEvents and runHunt declarations in include/speedgun-ng/hunt.hpp for the hidden-visibility shared build per include/speedgun-ng/hunt.hpp (partial)
- [x] T097 Write the resolved selection back for every --hunt form, not only @file, in source/harness/hunt_cli.cpp and pin the list-plus-glob case in the CLI test per source/harness/hunt_cli.cpp (partial)
- [x] T098 State the anchor rule once, in expandHuntSelection in source/harness/hunt_cli.cpp or appendAll in source/harness/hunt.cpp, and pin the all event count in the plan test per source/harness/hunt_cli.cpp (contradicts)
- [x] T099 Give RunHunt a spend figure in include/speedgun-ng/hunt.hpp, pass it at source/harness/report.cpp:602 and pin one spend figure line in the collect test per include/speedgun-ng/hunt.hpp (partial) Closed by T120 on the other option its row offers: the summary line states no spend figure, so the record holds none. The time source rule (D-6) reserves the monotonic clock for the counters layer, so no record of this layer can carry a wall-clock figure honestly.
- [x] T100 Print both stage event counts in printHuntPlan in source/harness/report.cpp from the per-stage records of RunHunt::stages per source/harness/report.cpp (partial)
- [x] T101 Re-run the T037 gate command over test/source/harness_hunt_report_test.cpp and test/source/harness_hunt_plan_test.cpp once their scenario lists land per test/CMakeLists.txt (partial)
- [x] T102 Add the planted out-of-order hunt key case and the planted key inside a closed hunt node case to test/source/harness_hunt_json_test.cpp per test/source/harness_hunt_json_test.cpp (missing)
- [x] T103 Add the two-stage document scenario to test/source/harness_hunt_json_test.cpp using the scripted host in test/source/detail/hunt_fixture.hpp per test/source/harness_hunt_json_test.cpp (missing)
- [x] T104 Add the no-hunt-keys and pre-feature-document loading scenarios to test/source/harness_hunt_json_test.cpp per test/source/harness_hunt_json_test.cpp (missing)
- [x] T105 Assert both hunt objects key by key and in schema x-order against contracts/hunt-json.md from test/source/harness_hunt_json_test.cpp per test/source/harness_hunt_json_test.cpp (missing)
- [x] T106 Guard main in test/source/harness_hunt_json_test.cpp against an empty scenario list, then run the three T041 ctest gates per test/source/harness_hunt_json_test.cpp (partial)
- [x] T107 Run the US4 gates once the example lands: build run_docs_hunt_rank, fold its stdout against the golden and run bash test/example_run.sh per test/example_run.sh (missing)
- [x] T108 Raise PAGE_LINE_CAP in tools/docs/example_drift.py to the measured README.md length and record the move in the comment block above it per tools/docs/example_drift.py (missing)
- [x] T109 Add the docs_hunt_rank-guarded report_shape_hunt_rank_sample entry with its TIMEOUT 120 to test/CMakeLists.txt per test/CMakeLists.txt (missing)
- [x] T110 Write the Zen 5 --hunt=all record for gary into specs/029-hunter-core/results/ once that lane is free per specs/029-hunter-core/results/ (missing)
- [x] T111 Fix the version grep at specs/029-hunter-core/quickstart.md:156, add the two non-hunt document greps and number the walkthrough steps per specs/029-hunter-core/quickstart.md (contradicts)
- [x] T112 Declare the lastRefusalMessage accessor beside setGroupLeafLimit in specs/029-hunter-core/contracts/hunt-api.md per specs/029-hunter-core/contracts/hunt-api.md (unrequested)
- [x] T113 Give rankEvents the settled method in include/speedgun-ng/hunt.hpp and source/harness/hunt_plan.cpp and pin a true multiplexed case in the rank test per source/harness/hunt_plan.cpp (partial)
- [x] T114 Resolved by spec, no change owed: the hunt ranking and skip lines print at source/harness/report.cpp:611-616, above the return at :621, and that return guards only `printContextDisclosure` at :624 and the per-context percentile loop below it, which FR-025 keeps off a single-context result, so a single-context hunt run prints every hunt line it has and the code carries the FR-025 citation at :618-620. The order is specified and pinned by `scenarioPlanLineStandsAboveTheRows` at test/source/harness_hunt_cli_test.cpp:942, registered at :1223, which captures a real hunt run and asserts the plan line once, `plan + 1` as the first row line, and the rank lines after the rows.
- [x] T115 Repoint the US4 citations at lines 216, 228, 229 and 231 of specs/029-hunter-core/tasks.md at the FR-070 and FR-071 the spec defines per specs/029-hunter-core/tasks.md (contradicts)
- [x] T116 Record the restricted-CPU-set ctest -R hunt results and reword the three prose matches in test/source/harness_hunt_collect_test.cpp and the CLI test per test/source/harness_hunt_collect_test.cpp (partial)
- [x] T117 Record the four reserved values of CMakeLists.txt and kSchemaVersion with their reasons in the merge notes, retaking them if master moved per CMakeLists.txt (partial)

## Phase 10: Convergence

- [x] T118 Carry the errno or availability word of a refused solo retry from seatPass to skippedKindOf in source/harness/hunt.cpp so a retry refusal maps to the kind the host answered with ABSENT reserved for a refusal that names no cause, state the errno-to-kind rule once beside the discovery switch in source/counters/linux_pmu/provider.cpp, and pin the mapping on a message the counters layer emits [BLOCKING] (round 2 code F1) The availability word of the refusal crosses that seam through `availabilityOfRefusal`, the errno-to-state rule stands once beside the discovery probe switch, and the mapping is pinned against the sentence `source/counters/plan.cpp:485-490` composes
- [x] T119 Feed the planner a figure that measures one hunt pass in nanoseconds, one the per-sample overhead floor cannot stand in for, so a seconds budget bounds the run, and pin one printed estimate figure plus the budget pair (fits, so no cut line; over, so the cut line) at the command-line surface (FR-012, FR-013, SC-007, closes T080) The pair stands pinned at the library seam (scenarioSecondsBudgetWeighsThePassFigure in test/source/harness_hunt_collect_test.cpp), where the caller names the figure and both arms answer the same way on every host; the command-line surface pins the figure relationally (scenarioPlanWeighsTheMeasuredPass), since a command-line arm would weigh its budget against a figure that very run times, which measures the machine rather than the rule [BLOCKING] (round 2 code F2)
- [x] T120 Land the spend figure the summary line prints or restate the summary shape at specs/029-hunter-core/data-model.md:202 and the console example at specs/029-hunter-core/contracts/hunt-cli.md:164 to the bytes the run prints, so the contract names one figure the tree can emit (FR-040, closes T099) The shape route stands taken: the summary line prints the counts and the budget only, the contract states that a hunt carries no wall-clock reading, and the record field that route needed never landed, because the time source gate (D-6) reserves the monotonic clock for the counters layer [BLOCKING] (round 2 docs F1)
- [x] T121 Fill the empty scenario list at test/source/harness_hunt_report_test.cpp:51 with the E-07 console shapes, the PD-15 order, the MULTIPLEX plan line, the normalized=iter case and the --hunt=none silence, then repoint the stage-1-only verification row at specs/029-hunter-core/contracts/hunt-api.md:386 at the scenario that lands and re-run the T037 gate command over that file and the plan test (FR-012, FR-040, FR-041, SC-008, closes T077, unblocks T101) [BLOCKING] (round 2 code F9, docs F3)
- [x] T122 Strike the BENCHMARK_NO_RUN step from specs/029-hunter-core/results/corsairone-hunt-all.md:384-386 and restate the usage-route check as the one refusal line on stderr with status 2 that source/harness/hunt_cli.cpp answers (FR-081) [BLOCKING] (round 2 docs F2)
- [x] T123 Either fill SkippedEvent::stage from the sample whose window disclosed the skip or delete the field with its console branch, writer line, required schema key and reader arm, so the skipped record carries what FR-050 lists (FR-050, FR-053) (round 2 code F3) The delete route stands taken: the field, its console branch, the writer line, the schema key, the reader arm and the plant table entry left together, so the line reads `hunt: skipped <address> <kind>`
- [x] T124 Lift the selection rules (raw-code shape, element splitter, glob matcher, catalog walk, anchor-leaf rule) into one detail header that EventCatalog::expand and expandHuntSelection both call, delete the second copy in source/harness/hunt_cli.cpp, and cut the per-run catalog walks from four to one (FR-002, FR-005, FR-006, FR-007, FR-082) (round 2 code F4)
- [x] T125 Drop the inert stageOneRates parameter and its precondition from splitStages, state the stage-2 selection rule in the contract's words, and note in HuntStageRange that the range form holds only while stage 2 is a leading set (FR-013, FR-014) (round 2 code F5)
- [x] T126 Return the run-level record from runHuntNamed and pass it to printResult and ReportData::hunt through the call chain and retire the process-wide slot at source/harness/hunt.cpp:829-833, so a filter that selects several instances cannot leave the last record standing beside another result (FR-040, FR-050, PD-10) (round 2 code F6)
- [x] T127 Give the reserved --hunt-rank words their own one-line refusal that names N14b, keep the generic wording for malformed words, and reword the specs/031 forward note in include/speedgun-ng/hunt.hpp and specs/029-hunter-core/contracts/hunt-api.md:86 to the N14b name the spec uses (FR-032, D-3, FR-081) (round 2 code F7, code F11, docs F17)
- [x] T128 Derive a refused retry column's published availability from the recorded refusal, dropping the fixed NOT_ENCODABLE at source/harness/runner.cpp:2367-2369, and pin the row token beside the skipped line, so one address names one kind on both surfaces (FR-021, FR-040) (round 2 code F8) The published state comes from the recorded refusal through `availabilityOfRefusal`, and `scenarioSoloRefusalMapsItsKind` pins the row token beside the skipped line of the same address
- [x] T129 Split the address in expandHuntSelection's device check with the shared devicePartOf rule, dropping the local first-slash find, so the sub-3-slot refusal and the anchor-leaf test read the real device and leaf (FR-081, PC-1) (round 2 code F10)
- [x] T130 Build rowOf, loneEventOf and measuringOfColumn only when huntRun holds, so a run with no hunt allocates what it allocated before the hunt existed (plan VII, I-8) (round 2 code F12)
- [x] T131 Scope the selection precondition at specs/029-hunter-core/contracts/hunt-api.md:89 to the hunt the record drives, so it matches the empty selection the header documents and the parked record source/harness/hunt_cli.cpp answers (FR-001) (round 2 docs F4)
- [x] T132 Add the cutReason row to the E-08 field table at specs/029-hunter-core/data-model.md:211-221 beside the stage fields that feed the budget cut line (FR-013) (round 2 docs F5)
- [x] T133 State the two-stage plan-line variant with its stage1 and stage2 fields in the E-07 table at specs/029-hunter-core/data-model.md:195 and in the console example at specs/029-hunter-core/contracts/hunt-cli.md:136, matching source/harness/report.cpp:774-778 (FR-012, FR-013) (round 2 docs F6)
- [x] T134 Restate the example_drift ledger row at specs/029-hunter-core/merge-notes.md:127 with the cap line and page length the tree carries (round 2 docs F7)
- [x] T135 Re-read the README.md, harness.md and test/CMakeLists.txt line numbers in the ledger rows at specs/029-hunter-core/merge-notes.md:122, 124 and 125 so each cited line holds the text the row names (round 2 docs F8)
- [x] T136 Repoint the citations at specs/029-hunter-core/contracts/hunt-api.md:298, 302 and 348 at source/harness/statistics.cpp, the ReportData range in include/speedgun-ng/benchmark.hpp and source/harness/report.cpp:38 (round 2 docs F9)
- [x] T137 Repoint the five writer and reader citations at specs/029-hunter-core/contracts/hunt-json.md:135, 136, 143, 144 and 147 to the lines that hold those definitions now (round 2 docs F10)
- [x] T138 Repoint the citations at specs/029-hunter-core/contracts/hunt-cli.md:12, 74, 116 and 123-124 to the option table, the percentile refusal, the RunOptions members in source/harness/detail/internal.hpp and the dry-run one-iteration sites in source/harness/runner.cpp (round 2 docs F11)
- [x] T139 Re-read the internal.hpp, counters_fake.hpp and report.cpp line citations in specs/029-hunter-core/data-model.md:186, 225-226, 236-237 and 242 against the tree (round 2 docs F12)
- [x] T140 Re-check rows T027, T070, T074, T076 and T096 of specs/029-hunter-core/tasks.md against the tree, tick the rows whose code landed and leave a note where a row still owes its test (FR-012, FR-013, FR-040, FR-090) (round 2 docs F13)
- [x] T141 Narrow specs/029-hunter-core/contracts/hunt-cli.md:104-106 and docs/pages/harness.md:1415-1416 to say that a valid selection prints no hunt line beside the listing modes while an invalid one still refuses at status 2 before the listing, per source/harness/cli.cpp:599 (FR-081, FR-083) (round 2 docs F14)
- [x] T142 Correct specs/029-hunter-core/results/corsairone-hunt-all.md:391-392 to cite specs/029-hunter-core/results/gary-hunt-all.md and settle whether T061 and T110 are still owed (SC-010) (round 2 docs F15)
- [x] T143 Re-read the report.cpp and harness_hunt_cli_test.cpp line reads inside tasks.md rows T027 and T114 so each cited line holds the symbol the row names (round 2 docs F16)
- [x] T144 Widen the finding text at tools/docs/example_drift.py:696-698 to the header tuple header_names scans, so a hunt:: name failure names hunt.hpp (FR-070) (round 2 docs F18)

## Phase 11: Convergence

Round 3 audited the code surface and the artifact surface in a detached
worktree at `d6d60f2`. Every citation in a row below was opened there; the
tip moved to `6749455` while the round ran, so an address inside a row is
the address the auditor read and needs one re-read by whoever takes the
row. Rows T124, T126 and T129 were excluded as in flight in other lanes,
as was the coverage bar. Two rows are BLOCKING.

- [x] T145 Make a seconds budget that no calibration figure can price cut
      stage 2 and disclose the cut, the way FR-013 scenario 4, PD-18 and I-7
      require: in `splitStages` treat an absent `allowedPasses` answer for a
      seconds budget as a budget the plan does not fit, keep the reason word
      the printers spell reachable from that route, state the case in the
      `\post` at `source/harness/detail/hunt_plan.hpp:68`, restate the pin at
      `test/source/harness_hunt_plan_test.cpp:1086`, which today reads "a
      seconds budget with nothing to convert by keeps both stages", and carry
      the word into the E-07 and E-08 tables at
      `specs/029-hunter-core/data-model.md:199` and `:224`, where
      `RunHunt::cutReason` already defaults to the one word the printers spell
      at `include/speedgun-ng/hunt.hpp:467` (FR-012, FR-013, PD-18, I-7)
      [BLOCKING] (round 3 code F1)
      Measured at `d6d60f2`: `--hunt=all --hunt-budget=1s --dry-run` answered
      exit 0 with `hunt: plan events=489 passes=245 method=PASSES
      estimate=not available budget=1s stage1=489 stage2=60` and no cut line.
      The figure reaches the planner only through the probe at
      `source/harness/hunt.cpp:982-999`, which `--dry-run` skips, and
      `hunt_plan.cpp:453` reads `overBudget = allowed.has_value() && runs >
      *allowed`, so an unpriceable budget looks unlimited.
- [x] T146 Refuse `--hunt` given with the scope option the tree actually
      carries: `--placement` (`source/harness/cli.cpp:404`, five preset names)
      has no branch in `validateHuntCombination` at
      `source/harness/hunt_cli.cpp:687-701`, so `--hunt=cpu/cpu-cycles
      --placement=core` ran a hunt and answered 0. Add the one-line stderr
      refusal with status 2 beside the `--counter`, `--metric` and `--compare`
      branches, pin it in `test/source/harness_hunt_cli_test.cpp` beside the
      three existing conflict cases, and record in
      `contracts/hunt-cli.md` and `docs/pages/harness.md` that FR-016's
      `--scope` is the `--placement` option of this tree, which no file in
      this branch spells `--scope` (FR-016, FR-081, D-16, PD-15) [BLOCKING]
      (round 3 code F2)
- [x] T147 Re-anchor the nine JSON citations in
      `specs/029-hunter-core/contracts/hunt-json.md` at 64, 134, 135, 142,
      143, 146 and 158: `metricMethod` is written at `json_report.cpp:586`,
      the `putOrNull` path of `passSpread` at `:589`, `writeReport` at `:726`,
      `readEntry` at `json_load.cpp:1031`, `optionalField` at `:107`,
      `loadReport` at `:1580` and the schema-major gate at `:1604-1610`. Name
      each item by its symbol in the sentence and keep the number as a second
      address, the way the spec states its own citation rule (FR-050, FR-051)
- [x] T148 Re-anchor `checkNode` at `source/harness/json_load.cpp:1450`,
      `checkReport` at `:1546`, `optionalField` at `:107` and the `putOrNull`
      path at `source/harness/json_report.cpp:589` in
      `specs/029-hunter-core/contracts/report-schema-key.md` at 30, 31, 48 and
      95, and drop the declaration read at `:32`: line 219 of
      `source/harness/detail/json_report.hpp` declares `checkReport`, and
      `checkNode` is a definition inside `json_load.cpp` (FR-053, D-17)
- [x] T149 Re-read the line reads in `specs/029-hunter-core/contracts/hunt-cli.md`
      at 12, 19, 108, 109 and 128: `longOptions[]` opens at `cli.cpp:391`,
      `kMetricMethodOption` at `:390`, `expandHuntSelection` is called at
      `:602`, the listing modes answer at `:641`, `:646` and `:676`, and
      `counterAddresses`, `metricNames` and `compareBaseline` stand at
      `detail/internal.hpp:523`, `:534` and `:552` (FR-080, FR-081, FR-083)
- [x] T150 Repoint the `HuntOptions` precondition read at
      `specs/029-hunter-core/contracts/hunt-api.md:89` from
      `source/harness/hunt.cpp:1181-1185` at the `SG_REQUIRE` of `runHunt` at
      `source/harness/hunt.cpp:1213-1216`, which is the require the sentence
      names (FR-001)
- [x] T151 Re-read `cutReason` at `specs/029-hunter-core/data-model.md:224`
      against `include/speedgun-ng/hunt.hpp:467` and `ObjectSeedData` at `:245`
      against `include/speedgun-ng/counters_fake.hpp:315` (FR-013, FR-090)
- [x] T152 Repoint the byte-identity `SG_INVARIANT` read at
      `specs/029-hunter-core/quickstart.md:249` from
      `source/harness/runner.cpp:2287-2292` at `source/harness/runner.cpp:2391`
      (FR-090, I-8)
- [x] T153 Correct the reads inside
      `specs/029-hunter-core/results/corsairone-hunt-all.md`: the usage route
      leaves through `rejectHuntSelection` at
      `source/harness/hunt_cli.cpp:67-71`, the range quoted at `:390` holds
      `rejectHuntRankReserved`, and restate the summary-line cell at `:363` to
      the bytes the printer answers, `hunt: plan summary events=<ran>/<planned>
      passes=<p> budget=<text>|none` (`source/harness/report.cpp:857-863`), so
      the fact table of a hardware record quotes no figure the shipped binary
      cannot print (FR-040, FR-081, T120, AC-10)
- [x] T154 Tick the rows whose deliverable stands in the tree and note what
      each one landed behind: T077 and T080 (named closed by the T121 and T119
      rows at `specs/029-hunter-core/tasks.md:553` and `:551`), T082, T083 and
      T084 (the round trip, the per-code reader refusals and the no-hunt
      capture behind `test/data/hunt_no_hunt_document.json` and
      `test/data/hunt_no_hunt_console.txt`), T086
      (`example/docs/hunt_rank.expected`), T087 (`json_report.cpp:34`), T090
      (eleven `hunt::` rows in README.md), T106
      (`test/source/harness_hunt_json_test.cpp:1471`), T109
      (`test/CMakeLists.txt:1300-1308`) and T111 (numbered steps and both
      non-hunt greps at `specs/029-hunter-core/quickstart.md:155-160`)
      (FR-012, FR-040, FR-050, FR-070, FR-090)

Round 3 judged these surfaces clean and reported no row against them: the
test surface (every hunt test guards its scenario list against emptiness,
no scenario list sits behind a capability guard, no test file names a sleep,
a clock or `hardware_concurrency`, one expectation per literal), and the
console shapes of `harness_hunt_report_test.cpp:414-537`, which pin the plan
line, the cut line, the ranked table, the marked tail, the skipped list and
the summary line byte for byte in the order PD-15 gives.

Acceptance reachability, stated plainly: `results/corsairone-hunt-all.md`
belongs to this machine, and the refusal routes and every console and
document shape it quotes were re-checked here, while its `--hunt=all`
figures (489 events, 245 passes, 120531 document bytes, 497 skipped lines)
stand unverified because four lanes build and run on this box at once and
Principle VII forbids restating a figure from a contended system.
`results/gary-hunt-all.md` belongs to `gary`, which this machine cannot
reach, so that record is unjudged and AC-10 scores unreachable on both
hosts from here.

- [x] T155 Replace the CLI estimate assertion that ranked two live
      measurements. `scenarioPlanWeighsTheMeasuredPass` compared the figure of one pass
      behind the estimate with the overhead floor the same run publishes, and an
      instrumented host lifts a measured pass until it costs what a window read costs,
      so the ranking flipped and the sanitizer lane failed at two heads under two
      operators. The scenario now bounds a measured pass below by the floor of its own
      run, and the floor-copy defect the ranking was written for is pinned on the
      library route, where the calibration figure is one the test hands over and the
      product must weigh exactly (`test/source/harness_hunt_cli_test.cpp`,
      `test/source/harness_hunt_collect_test.cpp`; T119, FR-012)

- [x] T156 Read the plan estimate free of the overhead floor. The bound T155
      introduced ordered two readings of one host, and the ThreadSanitizer and
      AddressSanitizer lanes both timed a pass below a floor at that head. The
      comparison is gone; the case keeps the relations the planner owes whatever
      the host answers, an estimate present and divisible by the passes of the
      plan, and the defect stays pinned on the library route, where no clock
      runs (`test/source/harness_hunt_cli_test.cpp`; T119, FR-012)

- [x] T157 Cover the selection shapes and the refusal routes the hunt answers.
      The padded list, the trailing star, the unmatched pattern, the empty
      address line, the address the catalog holds none of and the raw event
      code each stood behind a branch no case reached, and the hunt of an
      unexpanded instance name stood uncovered with two catalog refusals
      beside it. Every refusal now answers a check naming the value its message
      carries, on the plan route and the library route alike. One check that
      read `addresses()` after a refusal aborted on its own precondition, which
      is the contract answering that a refusal leaves no expansion to read
      (`test/source/harness_hunt_plan_test.cpp`,
      `test/source/harness_hunt_collect_test.cpp`; FR-004, FR-005, FR-006,
      FR-007, FR-092, E-01)
## Phase 12: Convergence

Assessed at `48e33ee` (`Meta: Close the Quickstart and Record Rows`, the
branch tip of the `r4b` lane; `origin/029-hunter-core` at `0c7b791`): the
reading began at `efcecc4`, the four rows the integrator landed during it
moved no witness cited below except the two re-cited from
`harness_hunt_collect_test.cpp` and `quickstart.md`, and every path, line
and quote here was re-checked against the tree at `48e33ee`. No CRITICAL
finding: ten rows, four HIGH, five MEDIUM, one LOW. The settled gate facts
were re-checked where this round could reach them: no coverage-exclusion
marker reaches the diff against `bd10d76`, the version and schema audit
comes out clean (the writer states `1.4` at `source/harness/json_report.cpp:34`
and the tree states `0.19.0` at `CMakeLists.txt:14`, and the fresh hunt
capture at `example/docs/hunt_rank.expected:2` agrees), and the quickstart
steps this lane could run ran. The integrator added T168 for the flake
finding this round filed as owned, so the section holds eleven rows.

- [x] T158 Pass the norm the plan settled into the rank route in
      `source/harness/hunt.cpp:1050`, where `rankEvents(measured, options,
      plan.method)` and the rate test at `hunt.cpp:1031` read the caller's
      `options.norm` while `hunt.cpp:949-952` settle `HuntNorm::ITER` into
      the record for a plan that found no anchors. A selection on a device
      that holds no anchor pair then measures, prints `hunt: plan ...
      normalized=iter`, and answers every column with `samples` of `0`, a
      `rate` of `0` and a `skipped` kind of `ABSENT`, so the console names
      the `iter` label at `report.cpp:612` and prints `hunt: rank -
      <address> iter=0.000` for a count the run did read (data-model.md
      E-03 at lines 139-144 and 169, `docs/pages/harness.md:1502-1504`
      "the rate column reads `iter=`", spec AC-4, FR-023).
      `test/source/harness_hunt_collect_test.cpp:1176`, named
      `scenarioUncoreColumnsRankByIterations`, pins the empty table at
      `:1201` and `ranEvents == 0` at `:1212` under a name that states the
      other outcome, so it moves with the fix (AC-4, FR-023, PD-08).

- [x] T159 Send the hunter example through the hunt route. The example
      writes its own printers: `example/docs/hunt_rank.cpp` keeps its own
      skipped-kind table at `:109` (a third spelling beside
      `report.cpp:27` and `json_report.cpp`), its own rank and skip line
      shapes at `:114-136` with `instr=` fixed into the format at `:118`
      and `:126`, and its own method words from `:141`. The recorded run
      the main page quotes, `docs/pages/harness.md:1356-1370` over
      `example/docs/hunt_rank.expected:1-13`, therefore ends at the skipped
      list and carries no `hunt: plan summary` line, which every hunt run
      prints (`report.cpp:857`, and a live
      `--dry-run --hunt='cpu/*'` prints one). The gate that owns that block,
      `report_shape_hunt_rank_sample` at `test/CMakeLists.txt:1294-1307`,
      compares the page against the example's own output, so it cannot see
      the drift it exists to catch (FR-070, FR-093, Principle X.3).

- [x] T160 Land the reading T156 records. The row is ticked and cites
      `docs/pages/analyzer.md:22-41`; `docs/pages/` holds
      `about.dox`, `building.md`, `barrier-rule.md`, `counters-overhead.md`,
      `dbc-migration.md`, `dbc-overhead.md`, `harness.md` and
      `identifier-rename-map.md` and no `analyzer.md`, and no
      `analyzer-gate` target exists in `cmake/` or `CMakeLists.txt`, where
      the gate drivers are `coverage`, `dbc-gate`, `docs`, `lint`,
      `prose-lint` and `spell`. The reading is wrong on its CI half as
      well: the `test` job installs clang-tidy and cppcheck and builds the
      `ci-ubuntu` preset (`.github/workflows/ci.yml:165-174`), which
      inherits the `clang-tidy` and `cppcheck` configure presets that set
      `CMAKE_CXX_CLANG_TIDY` and `CMAKE_CXX_CPPCHECK`. Write the corrected
      reading into `docs/pages/building.md`, the developer page that does
      exist, name the preset that carries each analyzer, and leave ticked
      only what stands there.

- [x] T161 Fix the plan line of the worked hunt example at
      `docs/pages/harness.md:1458`: `hunt: plan events=42 passes=14
      method=PASSES estimate=1260000ns budget=120s` carries no `stage1=` or
      `stage2=`, while the run it shows sets a budget over a plan that
      fits, which `splitStages` splits into two stages and E-07 and
      `contracts/hunt-cli.md:150` print as `... budget=120s stage1=42
      stage2=42`; the same block quotes that budget again on the summary
      line at `docs/pages/harness.md:1465`. The block is a plain `text`
      fence with no `sg-sample-output` tag, so no gate reads it, which is
      how a wrong byte survives a green suite (E-07, FR-070, FR-071).
      Carry the fields or move the block onto a golden the shape gate runs.

- [x] T162 State in `include/speedgun-ng/hunt.hpp:543-550` the condition
      `rankEvents` really asserts. `source/harness/hunt_plan.cpp:249`
      ends the process when an address carries no `skipped` kind and no
      rate, while that `\pre`, its copy at `contracts/hunt-api.md:304` and
      the `\post` at `hunt.hpp:566` together admit a caller whose samples
      are free of a disclosure and whose every anchor read zero, which the
      same `\post` says contributes no rate for that pass. The run route
      escapes only because `hunt.cpp` stamps `ABSENT` over the columns that
      yield no rate before the call (`hunt.cpp:272-280`); the public route
      has no such step, so a program that satisfies the written
      precondition aborts. Name the guarantee the caller must hold, or mark
      the address instead of ending the process (Principle III).

- [x] T163 Re-anchor the two statistics guards of the public `rankEvents`
      precondition at `include/speedgun-ng/hunt.hpp:546` and `:550`, which
      cite `runner.cpp:52-55` and `runner.cpp:74-76`. `statisticsOf` left
      that file for `source/harness/statistics.cpp` in T009 and its guards
      stand at `statistics.cpp:23` and `statistics.cpp:46`, the anchors
      `contracts/hunt-api.md:304` and `data-model.md:271-272` already
      carry, so the header that is the developer's route contradicts the
      contract it owns and sends a reader to unrelated code for a deviation
      guard (FR-093).

- [x] T164 State the device-limit rule once.
      `source/harness/hunt_plan.cpp:114-119` refuses when the published
      limit is at most two and the device names a cycles anchor, in the
      words `publishes a limit of <n>, below the 3 leaves one event and its
      two anchors need`; `source/harness/hunt_cli.cpp:616-619` refuses when
      the published slot count is below three whatever the anchors, in the
      words `publishes <n> slots and a hunt pass needs 3` that
      `contracts/hunt-cli.md:86` and `docs/pages/harness.md:1500` pin;
      `data-model.md` gives the rule as `L` below 3 whatever the anchors, one
      predicate for both routes, the reading T177 settled. It left
      the words no contract pins, and the copies disagree on the anchor
      condition the documents name. Land the planner half and the doc half,
      one predicate and one wording, and reconcile the CLI copy: a live lane
      holds `source/harness/hunt_cli.cpp` this round (FR-011, FR-080,
      Principle X.3).

- [x] T165 Give `test/source/counters_trap_checked_test.cpp:80` a scratch
      file of its own. The checker redirects every fixture run into the
      machine-global `/tmp/counters_trap_checked_out.txt` and reads it back
      after `std::system` at `:84`, so the `RESOURCE_LOCK` registered at
      `test/CMakeLists.txt:296-297` and `:313-317` serializes the two
      entries of one `ctest` run only: two runs on one machine, which the
      five lanes now are, take the file from each other, and a check can
      read another run's bytes and pass without its own child having
      written anything. Name the file after the process id or write into a
      per-run directory; the fork handshake itself is sound and is not the
      fault here (Principle VI, AC-9).

- [x] T166 Name the one line a reader of
      `specs/029-hunter-core/quickstart.md:159-160` meets and the page does
      not explain: `ctest --test-dir build -R 'harness_json' -V` prints
      `error: the report file '/dev/full' could not be written: No space
      left on device` from a case that passes, the refusal the product owes
      `test/source/harness_json_report_test.cpp:447` when it asks for a
      report on a full device. The page drives the verbose form itself, so
      the reader meets the word `error` in a suite that reports success,
      and FR-061 holds every step of the page to what a reader walking it
      sees (FR-061, FR-062). The refusal is right and stays; the step names
      it in one clause.

- [x] T167 Do not name a norm for a plan that measures nothing.
      `source/harness/hunt.cpp:949-952` settles `HuntNorm::ITER` whenever
      `plan.normalized` is unset, and `source/harness/hunt_plan.cpp`
      answers that unset for a plan that holds no columns at all, so
      `--dry-run --hunt='uncore_imc_0/*'` on this host prints `hunt: plan
      events=0 passes=0 method=AUTO estimate=not available budget=none
      normalized=iter` although E-03 makes the fallback answer to a device
      that holds no anchor pair and to nothing else (data-model.md E-03).
      Gate the settle on a plan that holds columns (E-03, E-07).
- [x] T168 Give the JSON suite's scratch files a name per process.
      `test/source/harness_hunt_json_test.cpp:396-397` writes a fixed path
      under `/tmp` that a second concurrent run overwrites, and its two
      `ctest` entries carry no lock, so the suite fails when the machine is
      busy and passes in two seconds alone. The parent's handshake with the
      forked child (`pclose` and `WIFEXITED` at `:1379-1388`) is sound and is
      not the cause. Land the same fix T165 lands for the trap checker, in
      the same shape (Principle VI, AC-9).

## Phase 13: Convergence

Assessed at `d3d89a0` ("Harness: Move the Settle Record Out of the Lambda")
in the detached worktree `/home/archerc/sg029-lanes/ph9-converge`, created for
this round at 01:39 with the submodules initialized, with no write to
`specs/029-hunter-core/tasks.md`, to the index or to any ref. Every ticked row
of Phases 9 to 11 was checked against this tree by reading the code it names or
by running the test it names, and each of the five un-ticked rows (T055, T063,
T095, T159, T161) was confirmed open at this head. Nothing Phases 9 to 11
closed is re-filed here.

The tree moved under this round, so the fact is stated first. At 03:03:48 a
process outside this assessment ran `checkout: moving from d3d89a0... to
9b23fec` in this worktree, recorded in its HEAD reflog, so the working files
now carry the branch tip and two commits stand above the assessed head:
`101d9b6` ("Docs: Send the Hunter Example Through the Route", which routes
`example/docs/hunt_rank.cpp` through `printHuntPlan`, `printHuntRanking` and
`printHuntSkips`, adds `hunt: plan summary events=5/7 passes=3 budget=none` to
`example/docs/hunt_rank.expected` and to the page block, and ticks T159 and
T161) and `9b23fec` ("Harness: Give the Two Refusal Arms Their Own Lines",
which moves `detail::admitsHuntEvent` in `source/harness/hunt_cli.cpp` from
`:617` to `:626`, because it gives each of the two `--hunt` refusals its own
statement lines at `hunt_cli.cpp:520-543` so a gcov trace can count the choice).
`git diff --name-only d3d89a0 9b23fec` answers exactly five paths:
`docs/pages/harness.md`, `example/docs/hunt_rank.cpp`,
`example/docs/hunt_rank.expected`, `source/harness/hunt_cli.cpp` and
`specs/029-hunter-core/tasks.md`. Every claim below about those five paths was
re-read from `d3d89a0`'s own blobs, and every other file this round read is
byte-identical at the two heads, including all of `source/harness/` besides
`hunt_cli.cpp`, every test file, every contract and both hardware records. Each
gate row names the tip as the head to measure, because the merge lands the tip.

The round-4 fixes all stand at `d3d89a0`, and the ones that print were
re-exercised live: the norm route of T158 and T167 (`--dry-run` with
`--hunt='uncore_imc_0/*'` answers `hunt: plan events=0 passes=0 method=AUTO
estimate=not available budget=none` with no `normalized=` field, every uncore
address `not-encodable`), the unpriced budget of T145 (`--hunt=all
--hunt-budget=1s --dry-run` prints `hunt: plan budget cut stage=2
reason=over-budget`), the refusal of T146 (exit status 2 on
`--hunt=cpu/cpu-cycles --placement=core`, the status the `--counter` conflict
and the reserved `--hunt-rank=sensitivity` also answer), the one device-limit
predicate of T164 (`source/harness/detail/hunt_selection.hpp`, called at
`source/harness/hunt_plan.cpp:118` and `source/harness/hunt_cli.cpp:617`), the
per-process scratch files of T165 and T168
(`test/source/counters_trap_checked_test.cpp:89`,
`test/source/harness_hunt_json_test.cpp:1482`), and the statistics anchors of
T162 and T163 (`include/speedgun-ng/hunt.hpp:548-561` reads
`source/harness/statistics.cpp:23-25` and `:46-48`).

Gate facts this round reached by hand on this tree: the `ci-rocky` developer
build of `d3d89a0` compiled clean at 02:15, `format-check` answers clean, the
prose gate answers 571 sources and no finding, and the two DBC gate scripts
answer 299 interfaces with 0 gaps (run through `/usr/bin/python3`, because the
`Python3_EXECUTABLE` this box's configure selected carries no PyYAML, which is
the gap T175 files). The hunt, CLI-table, pass-report, statistics and counters
suites ran 14/14 green, and a live hunt against this host's PMU wrote a `hunt`
node at the report root with `norm=instr`, `rank=rate` and 491 entries while a
run without `--hunt` wrote a `schemaVersion` `1.4` document carrying no `hunt`
key at all.

The whole-suite run (`ctest --test-dir build --output-on-failure --timeout 600`,
124 tests) answers 121 passed and no assertion failure anywhere. Two entries
gave way to this round's own bound: `dbc_semantics_matrix` (#21) and
`harness_cli_test` (#65) each passed 600 s while three other lanes compiled on
the box, and both are fork-heavy suites that stall in that shape; T058 puts the
floor for a whole-suite run above 3600 s and CI's bound is 5400 s, so neither
timeout is filed here. The third failure is this round's artifact and no
defect: `report_shape_hunt_rank_sample` (#122) reported `sample output block
holds 12 lines, the run printed 11` beside
`hunt: plan summary events=5/7 passes=3 budget=none`, which is the 02:15 binary
of `d3d89a0` compared against the page and the golden as they stand at the tip,
both moved at 03:03; at `d3d89a0` the block and the golden stop at the skipped
list, 11 lines, the number the run prints, so the gate holds at the assessed
head and at the tip.

A gate with no completed report is unmet here, whatever a ticked row says.
`waves/ph9/` holds no analyzer report at any head. `sanitize.md:3-4` now reads
`123/124 passed under ASan+UBSan with zero sanitizer reports; one
harness_cli_test resource-accounting failure is being re-run solo` at head
`8914cf0`, and its `## Verdict` still reads `Pending.` The coverage lane at
`d3d89a0` answers `ctest | FAIL rc=8` and `coverage | FAIL rc=2`
(`coverage.md:11-12`): its suite failed on one entry alone
(`logs/ctest.log:455`, `65 - harness_cli_test (Subprocess killed)`, 124 tests in
3796 s) and the coverage target died in one second at
`Can't locate Capture/Tiny.pm in @INC ... at geninfo line 66`
(`logs/coverage.log:2`), the stage the sibling lane records as re-runnable with
`PERL5LIB` supplied. The setup note of the sanitizer report records `8914cf0` as
what `origin/029-hunter-core` stood at, so no wave report
carries a push or a CI read for `d3d89a0` or for the tip, which is T063's and
T095's own remaining clause. No CRITICAL finding: eight rows, two HIGH, four
MEDIUM, two LOW.

- [x] T169 HIGH Run the analyzer build of this work and write its report.
      `specs/029-hunter-core/tasks.md:280` (T057) is ticked for a `ci-ubuntu`
      build "clean of clang-tidy and cppcheck findings", and no analyzer report
      exists in `/home/archerc/sg029-lanes/waves/ph9/` at any head, while
      `git diff --shortstat 48e33ee..9b23fec -- source include test` answers
      901 insertions across 15 files, the whole of it unanalyzed.
      `docs/pages/building.md:266-284` states the route and the trap: no target
      runs either analyzer, they ride the compile, and a `build` tree another
      preset already warmed recompiles nothing and prints nothing at exit 0. In
      a lane-owned directory, `rm -rf build`, `cmake --preset=ci-ubuntu`,
      `cmake --build build`, then write `waves/ph9/tidy.md` with the head, the
      cppcheck verdict, and the count of clang-tidy findings that name the one
      check `.clang-tidy:21` raises to an error
      (`readability-identifier-naming`), reported beside the pre-existing
      warning total the `Checks: "*"` bracket at `.clang-tidy:9-19` produces
      (Principle VIII).
      Settled by the `ci-ubuntu` analyzer run at `a55ea6d`, reported in
      `waves/ph10/tidy.md`: 100 translation units through cppcheck, 0 findings
      for the check `WarningsAsErrors` names, 0 in the hoisted region.

- [x] T170 HIGH Land the sanitizer verdict over the two newest `json_load.cpp`
      commits. `/home/archerc/sg029-lanes/waves/ph9/sanitize.md:1` names the
      measured head as `8914cf0`, three commits behind the tip; `:3-4` report
      123 of 124 passed with zero sanitizer reports and one `harness_cli_test`
      failure re-run solo, and `## Verdict` is `Pending.` The commits it misses
      are the ones the sanitizer suite reads hardest: `d3d89a0` moved the error
      settle into the plain function `keepFirstError` at
      `source/harness/json_load.cpp:135-150`, whose single call site is the
      `keep` predicate at `:149`, and `9b23fec` re-split the two selection
      refusals at `source/harness/hunt_cli.cpp:520-543`. Re-run the
      `ci-sanitize` and `ci-tsan` routes at the tip into the lane's own build
      directory, scan both `--output-on-failure` logs for
      `ERROR: AddressSanitizer`, `runtime error` and
      `WARNING: ThreadSanitizer`, and fill `## Verdict` with the head, the
      passed count and the scan result. The lane filled it at `1b6d2f5`:
      `ci-sanitize` 53 of 53 in 888.80 s and `ci-tsan` 52 of 52 in 957.70 s,
      both scanning zero reports across the three patterns
      (`waves/ph9/sanitize.md`). The full roster at `f5df59d` then failed both
      jobs on the use of freed memory T188 records, so the green pair stands at
      the head that verdict names and the roster at the fixed head is CI's to
      report (T058, Principle VIII).

- [x] T171 MEDIUM Get a coverage trace that ran, and read the moved settle out
      of it. The coverage target has produced no trace at any head: the lane at
      `d3d89a0` fails in one second on the box's moved Perl, with
      `Can't locate Capture/Tiny.pm ... at geninfo line 66` at
      `/home/archerc/sg029-lanes/waves/ph9/logs/coverage.log:2` and `make`
      exiting 2 at `:4`, which leaves the 100 percent line and branch bar of
      T055 (`tasks.md:278`) without evidence and FR-091 unevaluated. The
      sibling lane already names the remedy, `PERL5LIB` over the perl5 trees the
      moved `lcov` and `genhtml` need. Re-run `--target coverage` under that
      environment, then paste two things into the lane's report: the per-file
      line and branch totals, and the `gcov -b` lines for
      `source/harness/json_load.cpp:135-150` with the call at `:149`, which is
      the block `d3d89a0` moved out of the lambda precisely so the trace could
      count it. `9b23fec` asks the same question of the two selection refusals
      it put on their own statement lines at `hunt_cli.cpp:520-543`, so read
      those lines in the same pass. If `keepFirstError` answers zero runs, write
      that there: the restructure bought nothing and T055 needs a route that
      reaches each of its branches from a test. CI's own trace at `f5df59d`
      answers it, and `waves/ph11/coverage-tip.md` holds the numbers: 10571 of
      10571 lines, 5339 of 5438 branches, the predicate at `:145` reached 3536
      times with both arms taken and the settle body at `:148` run 21 times, so
      the restructure bought the counting it was meant to buy. Of the two
      refusals `:521` and `:522` count with both arms and `:529` keeps one arm
      untaken (T055, FR-091).

- [x] T172 MEDIUM Give `harness_cli_test` one recorded green run, or pin what
      the box takes from it. No lane has it green at these heads and each
      reports a different ending: `waves/ph9/logs/ctest.log:455` names it as the
      only failure of the coverage suite at `d3d89a0`, as
      `65 - harness_cli_test (Subprocess killed)` over a 3796 s run at the
      5400 s bound; `sanitize.md:3-4` reports a resource-accounting failure of
      the same entry under ASan and UBSan at `8914cf0`; this round's own run
      passed 600 s on it (`***Timeout 600.03 sec`,
      `/tmp/ph9c-ctest-full.log:2399`). The
      assertions in question are the SC-017 pair at
      `test/source/harness_cli_test.cpp:887-910`, which read the resource counts
      of a run interrupted by a signal, and the entry is registered plainly at
      `test/CMakeLists.txt:654` with no `TIMEOUT`, `PROCESSORS` or
      `RESOURCE_LOCK`, where the file gives properties to other long entries
      (`:580`, `:1322`). Run `ctest --test-dir build -R harness_cli_test` alone
      at the tip, plain and under the sanitizer preset, and record both: if it
      passes alone, give the entry the properties that keep a loaded box from
      killing it, and if it fails alone, the SC-017 numbers say which resource
      the interrupted run still holds. Both runs are recorded: alone at
      `f5df59d` it passed plain in 2387 s read from the log's own stamps, and
      the entry has carried `PROCESSORS 2` with no `TIMEOUT` since 6c05452;
      alone under `ci-sanitize` it ends failed after 3751.28 s on the SC-006
      mapping count, with zero sanitizer reports across 2021 log lines, the
      first `/proc/self/maps` read shifting its own count under ASan. SC-017
      never reached a verdict, so the question lives in the row that owns
      SC-006 (Principle VI, AC-9).

- [x] T173 MEDIUM Re-anchor the citations the newest commits moved. The `+5` in
      `source/harness/json_load.cpp` from `d3d89a0`, the `+1` in
      `source/harness/hunt_plan.cpp` from `20ed572` and the 41 lines added to
      `test/source/harness_json_load_test.cpp` leave eight live citations short
      at `d3d89a0` and unchanged at the tip:
      `specs/029-hunter-core/contracts/hunt-json.md:142` cites the `readEntry`
      body at `json_load.cpp:1031` (it stands at `:1012`), `:146` cites
      `loadReport` at `:1580` (`:1561`) and `:158` cites the version-refusal
      window as `1604-1613` (the block spans `1585-1594`, so the cited window
      ends inside its comment);
      `specs/029-hunter-core/contracts/report-schema-key.md:30` cites
      `checkNode` at `:1450` (`:1431`) and `:31` cites `checkReport` at `:1546`
      (`:1527`); `include/speedgun-ng/hunt.hpp:548` cites `hunt_plan.cpp:220`
      for the settled-option guard (the `SG_REQUIRE` stands at `:221`) and
      `:556` cites `hunt_plan.cpp:249` for the rate invariant (the
      `SG_INVARIANT` stands at `:250`), and
      `specs/029-hunter-core/contracts/hunt-api.md:304` carries both of those
      numbers again; and the `schemaVersion` kind refusal is cited as
      `test/source/harness_json_load_test.cpp:529` at
      `contracts/report-schema-key.md:120`,
      `specs/029-hunter-core/quickstart.md:223` and
      `specs/029-hunter-core/merge-notes.md:94`, while that mutation row stands
      at `:569` in `kMutations[]` and `:529` sits inside the pre-pass-fields
      scenario (FR-093).
      Settled by 32d9f06 and this commit. Every target was re-read in the pushed
      tree, whose addresses differ from the audit's sibling tree: the reader
      bodies stand at `:1012` and `:1561`, the checker at `:1431` and `:1527`, the
      version window spans `1585-1594`, the mutation row at `:569`, the version
      sentence at `docs/pages/harness.md:1124`, and the writer constant at
      `json_report.cpp:38`.

- [x] T174 MEDIUM Note in gary's record that its two summary lines predate
      T120. `specs/029-hunter-core/results/gary-hunt-all.md:119` and `:140`
      quote `hunt: plan summary events=343/355 passes=89 spend=not available
      budget=none` and `... events=341/353 ... spend=not available ...`, and the
      printer of this tree emits no `spend` field:
      `source/harness/report.cpp:856-863` writes `events=`, `passes=` and
      `budget=` and then the newline. The companion record carries the clause
      that tells a reader those bytes came off an older tree, naming the field's
      removal and today's shape at
      `specs/029-hunter-core/results/corsairone-hunt-all.md:399-407`; gary's
      record holds no such clause anywhere, so a reader takes a line the shipped
      binary cannot print as what this tree prints. Add the same one-clause
      provenance note beside the two quoted runs (FR-093, T120).
      Settled by 1c35cfc: one provenance clause beside the two quoted runs, the
      quoted bytes untouched. History confirms the field arrived in 4e6a213 and
      left in a9c3196.

- [x] T175 LOW Name PyYAML where the pages hand out the `dbc-gate` step.
      `specs/029-hunter-core/quickstart.md:19` lists the tool prerequisites a
      reader needs for the gate steps (`doxygen` for `dbc-gate`, `lcov` and
      `gcov` for the coverage target) and `docs/pages/building.md:31` gives the
      command `cmake --build build/dev -t dbc-gate`, while
      `tools/dbc/dbc_gate_common.py:67` refuses the run with
      `dbc gates require PyYAML (install python3-yaml)` when the interpreter
      `find_package(Python3)` selected carries no `yaml`, the shape this round
      hit, which forced the manual run quoted in the preamble. One clause in the
      prerequisite line, naming the module the two gate scripts import, keeps
      FR-061's rule that a reader walking the page sees what the page says
      (FR-061).
      Settled by cb4c875: the prerequisite line names PyYAML and the module the
      two gate scripts import through `dbc_gate_common.py`.

- [x] T176 LOW Rename the round-4 section header.
      `specs/029-hunter-core/tasks.md:723` reads `## Phase 4: Convergence` in a
      ledger that already carries `## Phase 4: User Story 2 - Bound the time and
      see the plan` at `:142`, so the file holds two `Phase 4` headers, and the
      three convergence sections above it are numbered `Phase 9`, `Phase 10` and
      `Phase 11` at `:492`, `:548` and `:578`. A reader who takes the highest
      phase header as the next number reads `11`, the count the round-4 author
      used too. When both sections land, rename the header at `:723` to
      `## Phase 12: Convergence`, the number its position gives it, and carry
      this section to `## Phase 13: Convergence` in the same edit, since the two
      cannot share a phase number; no row number moves (Principle IX).
      Settled by 1b6d2f5: the round-four header reads `## Phase 12: Convergence`
      and this section reads Phase 13.

### Second assessment of the same round

Two lanes assessed this round. The rows below are the second lane's, kept where they
name something the rows above do not. Its summary-time row and its rank-precondition
citation row are settled, and both say so where they stand.

- [x] T177 HIGH State the device-limit refusal without the anchor condition.
      `include/speedgun-ng/hunt.hpp:512` posts the refusal for "an event whose device
      publishes a limit below 3 with both anchors present", and
      `specs/029-hunter-core/contracts/hunt-api.md:194` repeats that wording, while
      `source/harness/hunt_plan.cpp:118` refuses on `!detail::admitsHuntEvent(*limit)`
      whatever the anchors and `specs/029-hunter-core/data-model.md:112` already reads
      "L below 3 admits no event, whatever its anchors". 20ed572 landed the predicate,
      the cause line and the data-model half, leaving the public header and the contract
      it owns teaching a caller that a two-slot device which publishes no anchor pair
      still plans (FR-011, FR-081, T164). Remedy: drop "with both anchors present" from
      both reads and name `detail::admitsHuntEvent` as the predicate in the header.
      Settled by f6def39. `admitsHuntEvent` at
      `source/harness/detail/hunt_selection.hpp:55-59` reads the slot count alone
      and both call sites pass only that count, so the anchor condition was
      invented; header and contract now name the predicate.
- [x] T178 MEDIUM Give `test/source/dbc_trap_checked_test.cpp:40` a scratch file of its
      own. `std::string out = "/tmp/dbc_trap_checked_out.txt";` is machine-global, its
      bytes are read back at `:48-52` after `std::system` at `:46`, and the only
      `RESOURCE_LOCK` entries in `test/CMakeLists.txt` (`:297`, `:315`, both
      `counters_trap_checked_out`) cover the sibling that d9161da renamed at
      `test/source/counters_trap_checked_test.cpp:89`. Two `ctest` runs on one machine
      take the file from each other, so a check can pass on bytes its own child never
      wrote (Principle VI, AC-9, T165). Remedy: the T165 form, the name built from
      `getpid()`, removed before the run and again once read.
      Settled by 3a03bc7: the name carries `getpid()` and both removals stand.
      Two copies run at once both passed, each writing its own file, and a poison
      file at the old global path stayed unread.
- [x] T179 LOW Cut the deleted field out of the `printHuntSkips` comment at
      `source/harness/report.cpp:833` ("the stage that lost the event where a stage
      did"): the printer answers `hunt: skipped %s %s`, address and kind, at `:842`, and
      `specs/029-hunter-core/data-model.md:202` states `hunt: skipped <address> <kind>`,
      so the comment is the last copy of the stage field T123 deleted (FR-041, T123).
      Settled by a27703c: the comment states the address and kind shape the
      printer emits, in the words `data-model.md:202` uses.
- [x] T180 LOW Give the two remaining folded string choices their own statement lines,
      but only if the coverage trace asks: `source/harness/json_report.cpp:613` and
      `source/harness/compare.cpp:1247` each hold one arm of a string choice as the
      middle operand of a concatenation whose statement begins on the line above, the
      shape 9b23fec removed from `source/harness/hunt_cli.cpp:520-546`. Both files stand
      inside the T055 scope. The trace asked for nothing: both files stand at
      100 percent lines in CI's trace at `f5df59d` and neither appears among
      the files carrying a gap, per `waves/ph11/coverage-tip.md`, so the
      condition this row states is unmet and no statement line was split
      (Principle VI, FR-091).
- [x] T181 LOW Say that the three hunt console blocks state a shape and are no captured run,
      or gate them: `docs/pages/harness.md:1428`, `:1458` and `:1511` open plain `text`
      fences with no `sg-sample-output` tag, so `tools/docs/example_drift.py` compares
      none of them. That is how `:1458` carried a plan line with no `stage1=` and
      `stage2=` until 101d9b6 hand-fixed it under T161, whose remedy offered that second
      route and did not take it (FR-070, FR-071, T161).
      Settled by 335f11a: each of the three blocks carries a clause saying it
      states the shape of the output. No sample-output tag was added, so the drift
      gate still compares none of them, the route the row offers as the
      alternative, unchosen here.
- [x] T182 Strike "and the summary adds the time the hunt spent" from
      `docs/pages/harness.md:1532`. Settled by 06b27c4 in this same round, on the same
      evidence the second assessment reached: the summary printer writes `events=`,
      `passes=` and `budget=` and then the newline (FR-040, T120).
- [x] T183 Re-anchor the rank-precondition citations at `include/speedgun-ng/hunt.hpp:548`
      and `:556`. The same gap as T173 above, which carries the wider sweep of eight
      citations (FR-093, T162, T163).

      Settled by this commit: `:830` names `docs/pages/harness.md:1500`, the
      line the refusal prints on, and this row's own number was corrected
      the same way. The live refusal stands at `hunt_plan.cpp:118-120` and
      `hunt_cli.cpp:626-627`; the wording both routes borrow stands at
      `source/harness/detail/hunt_selection.hpp:70-75`.
- [x] T184 MEDIUM Settle two ledger citations that drifted inside this
      file. `:830` cites `docs/pages/harness.md:1441` for the slot-limit
      refusal, and that refusal prints at `docs/pages/harness.md:1500` today,
      `:1441` holding the selection table instead. Both rows describe work
      that landed; only their addresses moved.
- [x] T185 MEDIUM Register `harness_cli_test` with the properties its cost
      needs. `test/CMakeLists.txt:654` holds a bare `add_test`, so the
      `--timeout 3600` of the `sanitize` and `tsan` jobs governs it, and a
      loaded run needed 3802 s. `PROCESSORS` is what shortens the elapsed
      time. Blocked on a measured instrumented wall-clock and CPU pair.
      Settled by 6c05452, with a smaller remedy than this row drafted. CI
      decides the bound: `master` at `fea69c5` carries the identical
      `test/source/harness_cli_test.cpp` (`bec0628` added `kFailedRunWindow`)
      and its `sanitize` and `tsan` jobs read `success` under
      `--timeout 3600 -j 2`, so the entry needs no `TIMEOUT` and the 3802 s
      figure was four copies of one binary sharing this box. What it does
      need is slots: `PROCESSORS 2`, which serializes it against siblings
      under the `-j 2` every CI job uses, so no other test shares its
      working directory while it runs. That reach stops at one `ctest`
      invocation, which is the gap T187 records. `RESOURCE_LOCK`, following
      `:297`, is the named upgrade if a job ever raises `-j`.
- [ ] T186 LOW Decide whether the contract and the header must match byte for
      byte. `contracts/hunt-api.md:194` ends the refusal sentence with a
      period where `include/speedgun-ng/hunt.hpp:512` keeps a semicolon, a
      split that predates this round. The pair gate reads the meaning and
      passes; a gate that compared bytes would flag it.
- [ ] T187 LOW Give the three scratch names in `harness_cli_test.cpp` the pid
      the T165 shape gives the others. `:476` writes
      `harness_cli_column_note.txt`, `:1243` to `:1250` write and read back
      `harness_cli_interrupt.json`, and `:1479` writes
      `harness_cli_options_file.txt`, each as a path relative to the working
      directory, so two `ctest` invocations over one build directory can read
      bytes their own child never wrote. The condition needs two
      invocations: the lanes of this branch hold one build directory each,
      and `PROCESSORS 2` at 6c05452 keeps other tests out of that directory
      while the entry runs, so no CI job meets it. Remedy if taken, the T165
      form T178 used, a name built from `getpid()`, removed before the run
      and again once the bytes are read (Principle VI, AC-9).
- [x] T188 CRITICAL Hold the fixture the twin's two rules read. CI's
      `sanitize` and `tsan` jobs each failed `harness_cli_table_test` with
      `heap-use-after-free` at the head they ran, ASan reading
      `harness_cli_table_test.cpp:965` one statement after the binding that freed the
      memory at `:964`, and `master` runs both jobs green, so the branch introduced it.
      `fixtureRows()` at `:937` returns its rows by value while `repeatedName` at `:782`
      and `repeatedValue` at `:796` return pointers into the vector handed to them, so
      the temporary died at the semicolon before the read. The uninstrumented suite
      cannot see the defect: freed heap still holds the planted bytes, so the gate
      reported what was planted and passed. Remedy taken, the named `rows` local at
      `:956`, which outlives the read at `:967` and the two at `:974` and `:978`. A
      sweep of `source/`, `include/` and `test/source/` for a pointer initialized from a
      call handed a temporary found no second site: the pair at `:834` and `:840` passes
      a named vector, and the range-`for` uses at `:958` and
      `test/source/detail/metric_fixture.hpp:394` are sound because the temporary lives
      for the loop. Verified by a pair on one Clang 23.1.3 `ci-sanitize` build, the
      unfixed head exiting 1 with the report and the fixed tree exiting 0 with zero
      `AddressSanitizer` lines, while the release preset still runs the test green in
      12.62 s.

## Phase 14: Convergence

Assessed at `f5df59d` in the detached worktree
`/home/archerc/sg029-lanes/ph12-audit`, with the tree clean before and after,
with no write to any source, header, contract or doc file, to the index or to
any ref. Reads only, plus the prose gate, which answered `prose-lint: 591
sources, 105825 units examined, 0 findings, 0 skipped`. Of 299 citations in
this file and the five contracts, 91 were opened at the cited line and 45 do
not hold. `test/source/harness_cli_table_test.cpp` carries the T188 fix above
this head, so a cite below its `:955` moves by two lines on integration, and
the twin gate is not reopened below. No coverage row is drafted here: a branch
gap needs the `coverage-info` artifact of a CI run, because a local `lcov`
2.5-beta counts exception and cleanup edges that CI's gcov never emits. The
full ledger is `/home/archerc/sg029-lanes/waves/ph12/converge.md`.

- [ ] T189 HIGH Correct the nine addresses `:634-639` records for the option
      table. Row T149 re-anchored the reads to `cli.cpp:390` and `:391`, `:602`,
      `:641`, `:646`, `:676` and `detail/internal.hpp:523`, `:534`, `:552`. At
      this head the table is the `to_array<OptionRow>` of `optionRows()` at
      `source/harness/options.cpp:496-716`, the eight hunt values are
      `source/harness/detail/options.hpp:91-98`, `expandHuntSelection` is
      called at `source/harness/cli.cpp:357`, the three listing modes return at
      `:396`, `:401` and `:445`, and the settled members stand at
      `detail/internal.hpp:514`, `:525` and `:543`. No `longOptions[]` exists
      in any source file. Restate the row; no code edit follows (Principle IV,
      X.3).
- [ ] T190 MEDIUM Re-anchor the addresses `contracts/hunt-cli.md` states. `:12`
      names the one `option longOptions[]` table at `cli.cpp:375`, where the
      compare-mode branch stands. `:15` names
      `tools/docs/example_drift.py:458-465` as the README option-row reader,
      where `table_rows()` scans the quick reference and `cli_options()` at
      `:493-505` reads `options.cpp`. `:38-40` places the eight constants in
      `cli.cpp` beside the comparison constants, which live at
      `detail/options.hpp:91-98`. `:66` cites `cli.cpp:367-381` for the
      provider registration at `cli.cpp:263` and `:276`. `:74` cites
      `cli.cpp:87-95` for the percentile refusal at `options.cpp:806-810`.
      `:109-111` cite `cli.cpp:586`, `:625`, `:630` and `:660` for the sites at
      `:357`, `:396`, `:401` and `:445`. `:123` and `:193` cite
      `runner.cpp:2184,2394` for one iteration per pass, where `:2184` settles
      repetitions, `:2394` is an `SG_INVARIANT` about seat compilability, and
      `:2405` holds the iteration.
- [ ] T191 MEDIUM Retire the `SG_CLI_SOURCE` reading the contract and three
      rows state. `contracts/hunt-cli.md:198` says the option table test reads
      `cli.cpp` through a compiled-in `SG_CLI_SOURCE` path, the registration
      pattern `SG_SCHEMA_PATH` uses at
      `test/source/harness_json_schema_test.cpp:21-22`. That analogy holds: the
      macro is set at `test/CMakeLists.txt:607`. `SG_CLI_SOURCE` appears
      nowhere under `test/`, CMake included: the test takes the source
      directory as an argument and opens `detail/options.hpp`, `options.cpp`
      and `cli.cpp` at `test/source/harness_cli_table_test.cpp:1670-1672`.
      Rows `:55`, `:118` and `:119` still describe the compiled-in path and the
      source spelling it parses. Restate all three (Principle IV).
- [ ] T192 MEDIUM Rewrite the fixture comment at
      `test/source/detail/cli_table_fixture.hpp:15-23`. It states the gate
      reads the one `option longOptions[]` table of `source/harness/cli.cpp`
      because that table is a function-local `static` array of `speedgunMain`
      at `cli.cpp:346`, and compares the way `cli.cpp:66-71` names the
      comparison constants. `source/harness/cli.cpp` is 702 lines and holds
      neither, and the constants are `detail/options.hpp:91-98`.
- [ ] T193 MEDIUM Correct the frozen-fixture read `:261` rests on. Row T053
      grounds its no-edit decision in
      `test/source/harness_compare_cli_test.cpp:516` asserting the console
      header of `gate_schema_11.json`. That case loads the `gate_ok_*` pair at
      `:496-497` and asserts `schema 1.0` at `:519` and `:524`; the pair that
      carries `schema 1.1` is checked at `:847-848`. The decision survives at
      the second site; restate the address.
- [ ] T194 MEDIUM Re-anchor the ten contract addresses the last sweep left.
      `hunt-api.md:134` cites `counters_system.hpp:47`, which is `path()` where
      `children()` begins near `:112`; `:319` cites `internal.hpp:561`, which
      is `optionsFile` where `Runner` begins at `:578`; `:415` cites
      `harness_hunt_collect_test.cpp:853` for a read at `:857`.
      `hunt-json.md:125` cites `json_report.cpp:43-78` for writers at `:89` and
      `:118`; `:134` cites `:589` for `passSpread` at `:607`; `:135` cites
      `:726` for `writeReport` at `:771`. `report-schema-key.md:26` cites
      `json_report.cpp:31` for the schema constant at `:38`; `:36` cites
      `harness_json_report_test.cpp:278` and `:382`, two blank lines whose
      `checkReport` calls follow at `:282` and `:386`; `:37` cites
      `harness_compare_cli_test.cpp:636` and `:704`, where those calls sit at
      `:968` and `:1036`; `:125` reads the words `schema 1.1` at `:516`, which
      asserts `schema 1.0`.
- [ ] T195 MEDIUM Narrow the exclusion scan T055 runs. FR-091 bars any
      exclusion marker, and the row's command
      `git grep -n 'LCOV_EXCL' -- source/harness include/speedgun-ng example
      test` returns 14 markers in three files this change never touched:
      `include/speedgun-ng/counters_provider.hpp:100` and `:106`,
      `include/speedgun-ng/counters_system.hpp:145-159`, and
      `include/speedgun-ng/dbc.hpp`, each recorded against
      `specs/007-counters-and-timers` Complexity Tracking. As written the scan
      can never read clean. Scope it to the paths this change writes, or
      record the 14 as pre-existing beside the coverage run (FR-091).
- [ ] T196 LOW Drop the property claim `:1040` still makes. Row T172 states
      `test/CMakeLists.txt:654` registers `harness_cli_test` bare.
      `set_tests_properties(harness_cli_test PROPERTIES PROCESSORS 2)` stands at
      `test/CMakeLists.txt:660` from `6c05452`, with no `TIMEOUT` beside it.
- [ ] T197 LOW Move the folded-string address T180 carries. It cites
      `source/harness/json_report.cpp:613`; the folded choice is `:630-631`. The
      `source/harness/compare.cpp:1247` half of the pair reads true.
- [ ] T198 LOW Correct the row count of the table-test comment.
      `test/CMakeLists.txt:663` says the table holds 27 rows in the fixed order
      with the seven new names at 290 to 296. `optionRows()` holds 35 rows: the
      27 of `kRowsBeforeHunt` at `test/source/harness_cli_table_test.cpp:241`
      plus the eight hunt rows `kRows` carries after them.
- [ ] T199 LOW Delete the repeated comment line of the writer.
      `source/harness/json_report.cpp:30` and `:31` hold one sentence twice, a
      residue of the re-anchor that `contracts/report-schema-key.md:26` reads
      past.
