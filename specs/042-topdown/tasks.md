# Tasks: Vendor-neutral Top-down Analysis

**Input**: Design documents from `specs/042-topdown/`

**Prerequisites**: [spec.md](spec.md) (FR-001 to FR-058, SC-01 to SC-13, user
stories US1 to US5, decisions D-1 to D-21, the audit point at `754867de`),
[plan.md](plan.md), [research.md](research.md) (D-01 to D-16),
[data-model.md](data-model.md) (E-01 to E-18, invariants I-1 to I-7),
[quickstart.md](quickstart.md), [contracts/](contracts/) (five files)

**Tests**: Included and mandatory. FR-048 puts each new behavior's test in the
same commit as the code it covers, so a test task and the code task that turns
it green are one commit and run in that order. FR-051 keeps line and branch
coverage at 100 percent with the branch allowance at 21 and no new exclusion
marker, and every unit of this feature runs without the kernel event interface,
without a sleep and without a clock read (FR-048).

**Organization**: Phase 1 carries the tier-label rows the two new counters units
need. Phase 2 lands the counters and harness facts every story reads. Phases 3
to 7 carry US1 to US5 in spec priority order. Phase 8 carries the acceptance
record, the coverage evidence and the gate set. Within a phase a test task
precedes the code task it gates, and the tasks that must move together are named
as commit sets in the dependencies section.

**Paths and ids**: paths are repo-relative. The line numbers quoted below are
the ones read from the tree at `754867de`; a line that moves because an earlier
commit of this lane edited the same file is named by the text it stands beside.

## Path Conventions

Public headers `include/speedgun-ng/`, counters sources `source/counters/` with
the PMU internals under `source/counters/linux_pmu/` and
`source/counters/detail/`, harness sources `source/harness/` with internal
headers under `source/harness/detail/`, the shared proc-root header
`source/detail/`, tests `test/source/` with shared fixtures under
`test/source/detail/`, docs examples `example/docs/`, the report schema
`docs/schema/`, the guide page `docs/pages/`, host-facing scripts `tools/` and
gate scripts `test/`. Both archives glob their sources recursively
(`CMakeLists.txt:256` for the harness, `CMakeLists.txt:1003` for counters), so a
new translation unit needs no source-list edit, while a new test needs an
explicit block in `test/CMakeLists.txt`.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: can run in parallel with other [P] tasks: a different file and no
dependency on an open task
- **[Story]**: US1 to US5, carried by the tasks of that story's phase. Setup,
foundational and polish tasks carry no story label
- Every description names the file it touches by path, and a case name in
backticks names the test case the task writes

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: The tier-label rows the two new counters units carry. Nothing else
needs shared scaffolding: both archives glob their sources, no build file gains
a source entry, and each new test file takes its registration in the commit that
writes its first real cases.

- [ ] T001 [P] In `test/tier_labels.sh`, add the rows
  `counters_topdown_select_test` and `counters_topdown_source_test`, each at
  skip code 2, to the `MASTER_SKIP` baseline at `:77-86`, so the tier gate
  accepts the skip rule the new counters units carry (no other gate change).

**Checkpoint**: `bash test/tier_labels.sh build ctest` reports no `skip-rule`
fault for the two new names.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: The facts every story reads. The three carried repairs the plan
names (the fast-read bound FR-053, the SMT state FR-054, the one-provider entry
point FR-055), the moved proc root, the settled `#core_wide` flag and the query
that publishes it, the threshold text from the table file to the published view,
the record the console and the report carry, the source record of the selected
directory, and the public selection surface with the fixture it is tested over.

**CRITICAL**: no user story work begins before this phase is complete. A live
level-1 value needs FR-053, FR-054 and FR-042, the suite budget needs FR-055,
and every story calls `topdownTree`.

### Tests for this phase (write first, observe failure)

- [ ] T002 [P] In `test/source/counters_linux_pmu_seam_test.cpp`, extend
  `pageGateScenario` with the fixed-counter case: `fastIndexValid(0x40000002)`
  answers true, `0x40000000 + kRnpmcMaxIndex + 1` and `0` still refuse, and a
  `fastDecode` at the marked index answers OK, with no event opened (FR-053; red
  at `754867de`).
- [ ] T003 [P] In `test/source/harness_cli_test.cpp`, add
  `pmuProviderIsOfferedOncePerProcess`: the entry point's offer helper in
  `source/harness/detail/internal.hpp` answers offered on the first call and
  already offered on the second in one process, so a second entry point call
  builds no provider (FR-055; red at `754867de`).
- [ ] T004 [P] Move `source/harness/detail/proc_root.hpp` to
  `source/detail/proc_root.hpp`, a new file at the lower path that holds the
  same reader, and change the four includes in
  `source/harness/jitter_probe.cpp`, `source/harness/noise_tracer.cpp`,
  `test/source/harness_noise_test.cpp` and
  `test/source/harness_precheck_test.cpp` to the new path; no behaviour moves
  (FR-054, D-19).
- [ ] T005 In `test/source/counters_metric_trees_test.cpp`, add
  `smtStateFallsBackToTheSiblingList`: over the coreless tree with a fake root
  from `installTestProcRoot`, `#SMT_on` answers 1 from a
  `cpu<N>/topology/core_cpus_list` of `0,12`, 0 from a list naming one id, and
  nothing from an unreadable list, while a tree that answers a core object keeps
  priority (FR-054; red at `754867de`).
- [ ] T006 In `test/source/counters_metric_trees_test.cpp`, add
  `coreWideAnswersFromTheProcessCpuSet`: from the process's own affinity and a
  fake sibling list, `#core_wide` answers 1 when the mask holds every sibling of
  every cpu it holds, 0 when one stands outside, and nothing when a list is
  unreadable, and it answers no slot width (FR-042 as settled at `754867de`; red
  there).
- [ ] T007 [P] In `test/source/counters_topdown_source_test.cpp`, a new file,
  write the source record over a two-file fixture directory: the reference is
  the manifest `ref:` line, each contributing file names itself with the
  `RECORD` sha256 beside it, the files stand in the embedder's sorted order, and
  a row of one file names only that file (FR-015; red at `754867de`).
- [ ] T008 In `test/source/counters_linux_pmu_seam_test.cpp`, add
  `thresholdTextReachesThePublishedView`: a seam table file carrying
  `MetricThreshold` shows the text reaching the parsed entry and the published
  `MetricView` byte for byte, and a row that carries no key answers the empty
  string (FR-017; red at `754867de`).
- [ ] T009 In `test/source/counters_metric_trees_test.cpp`, add
  `coreWideAnswersThroughThePublishedQuery`: both answers of `#core_wide` reach
  the caller through the published query over the fake root from
  `installTestProcRoot`, on the pattern of the `metricByName` cases, so a run
  can read the settled flag with no private reader (FR-042; red before the query
  lands).
- [ ] T010 [P] In `test/source/detail/topdown_fixture.hpp`, a new file, write
  the synthetic tables the selection reads: Intel level-1 to level-6 rows with
  both parent spellings, one group-free helper row, a level whose rows print
  under two objects, refused rows with kind and reason, threshold text, and both
  AMD pipeline tables of FR-052 with their eight differing level-2 names (FR-004
  to FR-017, FR-052).
- [ ] T011 In `test/source/counters_topdown_select_test.cpp`, a new file, write
  `cpuIdentityCarriesTheReadersFourFields` and `selectsTheTableOfTheIdentity`:
  model 85 stepping 4 selects `arch/x86/skylakex/`, family 25 model 17 selects
  `amdzen4`, family 26 model 17 selects `amdzen5`, the four production fields
  equal `pmuIdentCurrent()`, and the core type and `MIDR` are handed in by the
  test (FR-001 to FR-003; red at `754867de`).
- [ ] T012 In `test/source/counters_topdown_select_test.cpp`, write the tree
  cases over the fixture table: `levelOneCarriesTheSharedNames`,
  `parentsComeFromTheGroupText`, `levelDeeperThanTheTableClamps`,
  `refusedRowsStayInTheDocument`, `rowsUnderTwoObjectsBothReach`,
  `thresholdRidesAsTextUnchanged` and
  `levelTwoNamesDifferBetweenTheTwoAmdTables` (FR-008 to FR-017, FR-020, FR-052;
  red at `754867de`).

Observation: each case above fails on a name the tree does not carry yet, and
the three repair cases fail at `754867de`.

### Implementation for this phase

- [ ] T013 [P] In `source/counters/linux_pmu/fast_read.cpp`, bound the counter
  number in `fastIndexValid` with the fixed-counter mark masked off, keep the
  unmarked operand for the instruction, and correct the comment above the bound
  (FR-053, D-18; turns `pageGateScenario` green).
- [ ] T014 [P] In `source/harness/cli.cpp`, offer `counters::PmuProvider` once
  per process behind the helper in `source/harness/detail/internal.hpp`, beside
  the clock registration, and drop the per-call construction at
  `source/harness/cli.cpp:579` (FR-055, D-21; turns
  `pmuProviderIsOfferedOncePerProcess` green).
- [ ] T015 In `source/counters/linux_pmu/metric_tables.hpp`, give `readSmtState`
  its sibling-list fallback over `procRootPath` from
  `source/detail/proc_root.hpp`: `core_cpus_list`, then `thread_siblings_list`,
  the cpu from `::sched_getcpu()`, and nothing when that read fails (FR-054;
  turns `smtStateFallsBackToTheSiblingList` green).
- [ ] T016 In `source/counters/linux_pmu/metric_tables.hpp`, add the fifth
  `kHostFacts` row `core_wide` over the standing `bool(System&, double&)`
  reader, reading the mask with `sched_getaffinity` and each sibling list
  through the fallback reader of T015, and move `#core_wide` out of the header
  comment that lists the four refusing constants (FR-042, FR-043; turns
  `coreWideAnswersFromTheProcessCpuSet` green).
- [ ] T017 In `include/speedgun-ng/counters_metrics.hpp`, publish the query for
  the settled `#core_wide` flag beside `metricByName`, and define it in
  `source/counters/linux_pmu/metric_formula.cpp` over the row T016 lands,
  answering whole core or one thread of a core with no width (FR-042, FR-043;
  turns `coreWideAnswersThroughThePublishedQuery` green).
- [ ] T018 [P] In `cmake/embed_pmu_blob.cmake` and `cmake/EmbedPmuEvents.cmake`,
  pass the `RECORD` path to the generator, read the `ref:` line and each file's
  sha256, emit both into the index and the registry, and refuse a file that
  holds no hash line; in `source/counters/linux_pmu/embedded_tables.hpp`, give
  `EmbeddedFile` its `sha256` and `EmbeddedDir` its `reference` beside
  `embeddedFindDir` (FR-015, D-12; turns the source record green).
- [ ] T019 [P] In `source/counters/detail/pmu.hpp` and
  `source/counters/linux_pmu/table_parse.cpp`, give `PmuTableEntry` a `fileName`
  and tag every entry `pmuLoadTable` appends with the embedded file it came
  from, so a published row can name its file (PC-12).
- [ ] T020 In `source/counters/detail/pmu.hpp`,
  `source/counters/linux_pmu/table_parse.cpp`,
  `source/counters/linux_pmu/metric_formula.hpp`,
  `source/counters/linux_pmu/metric_formula.cpp` and
  `include/speedgun-ng/counters_metrics.hpp`, carry the threshold text from the
  table file to the parsed entry and to the published `MetricView` unchanged,
  with no evaluation anywhere; the same commit raises the counters archive
  `SOVERSION` from 4 to 5 at `CMakeLists.txt:110` with the comment row that
  gives the reason, and moves `theVersionBindingsReadTheMovedNumbers` at
  `test/source/harness_cli_table_test.cpp:1382-1393` to the number that moved
  (FR-017, D-11, FR-047).
- [ ] T021 In `include/speedgun-ng/benchmark.hpp`, add `TopdownNode`,
  `TopdownRefusal` and `TopdownRecord` as `specs/042-topdown/data-model.md` E-16
  types them, with the source record, the asked level and the level shown, the
  AMD table flag and the unsupported reason, and hold them as
  `std::optional<TopdownRecord> topdown` on `BenchmarkResult` beside `hunt` at
  `:747`, no value meaning the option never stood; the same commit raises the
  harness archive `SOVERSION` from 19 to 20 at `CMakeLists.txt:375` with its
  comment row and moves `theVersionBindingsReadTheMovedNumbers` to that number
  too (FR-032, FR-033, E-16, D-4, FR-047).
- [ ] T022 In `include/speedgun-ng/counters_topdown.hpp` and
  `source/counters/linux_pmu/topdown_select.cpp`, each a new file, publish
  `CpuIdentity`, `TableFile`, `TopdownSource`, `TopdownRow`, `TopdownTable`,
  `TopdownNode`, `TopdownRefused`, `TopdownTree`, `TopdownUnsupport` and
  `TopdownUnsupported` as `specs/042-topdown/data-model.md` types them, each
  with its `\pre`, `\post` and `\invariant`, and implement `cpuIdentity()` over
  `pmuIdentCurrent()` and `topdownTable()` over `pmuSelectDirectory` (E-01 to
  E-05).
- [ ] T023 [P] In `tools/docs/example_drift.py`, add `"counters_topdown.hpp"` to
  `NAME_SCAN_HEADERS` at `:530-538`, so a quick-reference name row for the new
  header answers no `quick-reference name ... is not in` finding (FR-044; spec
  Constraints).
- [ ] T024 In `source/counters/linux_pmu/topdown_select.cpp`, the new file of
  T022, implement `topdownTree()`: the level token over `TopdownL1` to
  `TopdownL6` and `PipelineL1` and `PipelineL2`, the `_group` parent rule over
  both spellings, the four neutral level-1 names with `topdown.smt_contention`
  on an AMD table alone, `topdown.<vendor>.<table name>` below level 1, the
  clamp to the table's own depth and the refused rows, with the five unsupported
  texts of `specs/042-topdown/contracts/topdown-selection.md` left to T076 in
  Phase 6 (FR-008 to FR-016, FR-020).
- [ ] T025 In `test/CMakeLists.txt`, add the two counters registration blocks
  `counters_topdown_select_test` and `counters_topdown_source_test`, the new
  files of T011 and T007, in the standing six-line shape of
  `test/CMakeLists.txt:515-522`, each with `SKIP_RETURN_CODE 2` matching its
  `test/tier_labels.sh` row and with the `SG_SEAM_TABLE_DIR` define of
  `test/CMakeLists.txt:484-489` for the two-file fixture directory.

**Checkpoint**: `cmake --preset=ci-rocky && cmake --build build` builds with the
new header, the new translation unit and the two new units, and
`ctest --test-dir build`
`-R 'counters_topdown|counters_metric_trees|counters_linux_pmu_seam_test'`
passes.

---

## Phase 3: User Story 1 - Read the level-1 breakdown of my benchmark (Priority:
P1) MVP

**Goal**: One option turns the breakdown on. A run with `--topdown` collects the
level-1 nodes inside the plan the run already builds, prints one block per
benchmark under its own tables with the level-1 values in percent and the vendor
name each came from, and carries the same nodes on the result record while every
metric view keeps its bytes.

**Independent Test**: spec US1: run one benchmark with the option and no level
against a host whose table carries level-1 rows; the block prints the level-1
names in percent, each with the vendor name it came from, and the values are
exact for a fake event source. Here the fixture tree of
`test/source/detail/topdown_fixture.hpp` and the scripted source of
`test/source/detail/pass_fixture.hpp` stand in for the host, driven through
`harness_topdown_option_test` and `harness_topdown_run_test`. No report key, no
rule and no page is needed.

### Tests for User Story 1 (write first, observe failure)

- [ ] T026 [P] [US1] In `test/source/harness_topdown_option_test.cpp`, a new
  file, write the option cases: `theBareWordAsksLevelOne` and
  `anAttachedLevelAsksThatLevel` read the level `detail::scanCommandLine`
  settles at `source/harness/detail/options.hpp:410` (FR-019),
  `aSpaceSeparateWordIsNoLevel` holds level 1 with the word `2` left positional
  (D-5), and each of `aBadLevelAnswersOneLine`, `aFileLineRefusesTheRow` and
  `noEnvironmentNameExists` runs the entry point once and checks the stream, the
  status and that no benchmark ran: `--topdown: expected 1 to 6, got %s`
  verbatim for `0`, `7`, `x` and the empty text at status 2, the origin-prefixed
  `the option 'topdown' cannot stand in an option file`, and no
  `SPEEDGUN_TOPDOWN` reaching the run (FR-019, FR-050). Add the new file's
  `add_executable` plus `add_test` block to `test/CMakeLists.txt` in the same
  commit, on the shape of `test/CMakeLists.txt:1483-1487` with the
  `source/harness` include directory.
- [ ] T027 [P] [US1] In `test/source/harness_cli_table_test.cpp`, move the table
  oracle to the 43rd row: `kContract` holds 43 rows and ends
  `{"topdown", kOptional, 350}` (`:244`), `theTableHoldsFortyTwoRows` counts and
  messages 43 (`:735`, `:740`), `inNewBlock` takes the block 350 to 359 beside
  the 340 bounds (`:817`), `theTopdownRowStandsInItsBlock` joins on the shape of
  `theLayoutRowStandsInItsBlock` (`:1101`), the partition loop stops reading the
  tail index as a hunt row (`:941`, `kRowsBeforeHunt` at `:295`), and
  `theHelpAnswerPrintsOneLinePerRow` reads the new help line
  `--topdown [<value>]` with its text
  `Prints the top-down tree of the running cpu at that level` (`:1397`, FR-019).
- [ ] T028 [US1] In `test/source/harness_topdown_run_test.cpp`, a new file,
  write the collection cases over the fixture tree of
  `test/source/detail/topdown_fixture.hpp` and the scripted source of
  `test/source/detail/pass_fixture.hpp`, driving `detail::Runner` the way
  `test/source/harness_pass_collect_test.cpp:312-316` does:
  `nodesShareOnePlanWithTheMetricColumns` holds one planner call and one pass
  set for the columns and the nodes together (FR-021, FR-023),
  `theMetricColumnsKeepTheirBytes` compares that run with the same run without
  the option, `aRefusedRowCollectsNoColumn` keeps the fixture refusal out of the
  column set (FR-016), and `aNewSourceReadsItsLimitOnce` counts the limit reads
  through `Object::programmableCounters()` alone (FR-022). Add the new file's
  `add_executable` plus `add_test` block to `test/CMakeLists.txt` in the same
  commit, on the shape of `test/CMakeLists.txt:1483-1487` with the
  `source/harness` include directory.
- [ ] T029 [US1] In `test/source/harness_topdown_run_test.cpp`, the new file of
  T028, write the record and strip cases: `theNodeFigureStandsInTheRecord` holds
  each collected figure in `result.topdown` with the availability the merge
  published, `theStripKeepsEveryMetricViewAligned` walks `metricLabels`, each
  row's `metrics`, `metricStats`, `percentiles` and the retained raw columns,
  with and without `--percentiles`, and `theStripChangesNoTimeColumn` holds
  every time column and every row field byte for byte against the same run
  without the option (FR-023, FR-024, FR-032).
- [ ] T030 [US1] In `test/source/harness_topdown_run_test.cpp`, add
  `anUnsupportedHostCollectsNothingAndStaysItself`: a planted refusal answers no
  node column, no block and an unsupported record with its reason, while the
  run's status equals the same run without the option (FR-040, FR-041; the one
  message line is T077's).
- [ ] T031 [US1] In `test/source/harness_topdown_run_test.cpp`, write the block
  cases: `theBlockStandsAfterTheResultTables` holds one block per benchmark
  under its own tables, threaded runs included (FR-025);
  `theBlockStatesTheLevelThePassesAndTheTable` reads
  `topdown: level=1 passes=%zu table=%s` with the pinned reference and the file
  hash (FR-026, FR-015); `theFourLevelOneNamesPrintWithTheirVendorNames` reads
  the four neutral names, their percent and their vendor names in table order
  (FR-027); `eachFigureEqualsWhatTheFixtureScripts` holds the four at the
  scripted figures (SC-03); `theAmdBlockPrintsTheFifthValueAndTheStatement`
  reads `topdown.smt_contention` beside the four and the line that says the four
  add to less than 100 percent without it (FR-028, FR-056); and
  `aRunWithNoMetricColumnStillStatesItsPasses` reads the pass line of a
  tree-only run (FR-021).
- [ ] T032 [US1] In `test/source/harness_topdown_run_test.cpp`, the case
  `theRunStatesTheCoreWideScopeBeforeTheFirstMeasurement`: a top-down run states
  once on the console, ahead of its first measurement, which way the flag
  settled, whole core or one thread of it, and a run without the option prints
  that line not at all (FR-042).

### Implementation for User Story 1

- [ ] T033 [US1] In `source/harness/detail/options.hpp`, add
  `kTopdownOptionValue = 350` after `kGroupLayoutOptionValue` at `:97`; in
  `source/harness/options.cpp`, append the `topdown` row after `hunt-baseline`
  at `:765` inside the array that closes at `:771`, with
  `OptionArgument::OPTIONAL`, no environment name and the owner column reading
  `specs/042`, the short form the hunt rows use; in `docs/pages/harness.md` add
  the option row at `:710`, the line after the `hunt-baseline` row at `:709`
  that closes the table opened at `:666`, which
  `thePageAndTheTableAgreeInBothDirections` at
  `test/source/harness_cli_table_test.cpp:1710` reads against the live row; and
  in `README.md` add the option row at `:998`, the line after the
  `--hunt-baseline` row at `:997` that closes the table opened at `:954`, paid
  by the one named trim of the split blank at `README.md:946`, so the page holds
  the `PAGE_LINE_CAP` of 1004 that `python3 tools/docs/example_drift.py .` reads
  (FR-019, FR-044; turns T027 green).
- [ ] T034 [US1] In `source/harness/detail/internal.hpp`, give `RunOptions` the
  one member `std::optional<std::uint64_t> topdownLevel` beside `graphPath` at
  `:701`, an engaged value being the option standing, so no second presence flag
  joins the record; in `source/harness/options.cpp`, give the row its own arm
  beside `case kHuntOption:` at `:920`, where a null `optarg` is level 1 and any
  other text reads 1 to 6 or answers the one refusal line at status 2, and add
  no arm to `applySourceValue`, so a file line keeps the `default:` refusal at
  `:1517-1528` (FR-019, FR-050, D-5; turns T026 green).
- [ ] T035 [US1] In `source/harness/runner.cpp`, call
  `sg::counters::cpuIdentity()` once after the `--metric` loop at `:1315-1326`
  and ahead of the core gather at `:1328`, ask `sg::counters::topdownTree` for
  `*m_options.topdownLevel`, and append one `MetricColumn` per collected node
  through `detail::metricColumnFor` as at `:1323-1325`, the label the node's
  vendor name and the core the leaf reads, a refused row taking no column and no
  position of the vector moving, so the leaf grouping at `:1348`, the limit read
  at `:1364` and `settleMethod` see one set; read the table the tests plant
  through `TopdownSelection::table`, the seam
  `specs/042-topdown/contracts/topdown-selection.md` gives a test, taken from a
  null-by-default member beside the new option in `RunOptions` (FR-018, FR-019,
  FR-021, FR-023; turns T028 green).
- [ ] T036 [US1] In `source/harness/detail/internal.hpp`, declare
  `stripTopdownColumns` with the `\pre` that every stripped position names a
  column of that run and the `\post` that the labels, every row's metrics and
  every row's statistics and percentiles hold equal length; in
  `source/harness/runner.cpp`, define it and call it once at the end of the
  result-finishing stage, past both merge paths at `:3098` and `:3112` and after
  the aggregate row at `:3185`, so each node figure moves into `result.topdown`
  and its position leaves every metric view, the two bounds held by `SG_REQUIRE`
  (FR-023, FR-032, PC-9; turns T029 green).
- [ ] T037 [US1] In `source/harness/runner.cpp`, take the refusal side of the
  `std::expected` answer at T035's call: append no column, record the reason and
  no node, print nothing and change no status, leaving the one line to
  `source/harness/cli.cpp` (FR-040, FR-041; turns T030 green).
- [ ] T038 [US1] In `source/harness/report.cpp`, add
  `printTopdownBlock(const BenchmarkResult&)` in the anonymous namespace that
  closes at `:334`, beside `printPassDisclosure` at `:301`, printing with
  `std::printf` in the house field shape of `:312`, and call it from
  `printResult` between `printPassDisclosure(result)` at `:651` and the jitter
  note at `:657`, so the block stands after the tables and every later line
  keeps its bytes (FR-025, FR-026, FR-027, FR-028; turns T031 green apart from
  the pass line).
- [ ] T039 [US1] In `source/harness/report.cpp`, let the guard of
  `printPassDisclosure` at `:303-307` read `&& !result.topdown.has_value()`, so
  a `--topdown` run with no `--metric` column still states the passes its one
  plan produced, one branch and no other change (FR-021; turns
  `aRunWithNoMetricColumnStillStatesItsPasses` green).
- [ ] T040 [US1] In `source/harness/cli.cpp`, print that one scope line once
  from `speedgunMain`, ahead of the walk that calls `runner.run` at `:1544`,
  naming the settled reading of `#core_wide` read through the query T017
  publishes (FR-042, D-6; turns T032 green).

**Checkpoint**: `ctest --test-dir build`
`-R 'harness_topdown_option_test|harness_topdown_run_test'` passes. US1 stands
on its own: the option, one plan, one block, one record.

---

## Phase 4: User Story 2 - Drill down to the level I ask for (Priority:
P2)

**Goal**: a level on `--topdown` collects that level and every level above it
into the one plan the run already builds, the tree clamps to the deepest level
the table holds and says so, and a deeper node keeps the vendor name under the
parent its own group token names.

**Independent Test**: spec US2: ask for level 2, 3 and 6 against the same table
and check the parent of each printed node, the indent and the pass count the
plan states, then ask for a level deeper than the table holds and read the
statement that names the deepest level it holds. Here
`test/source/counters_topdown_select_test.cpp` drives the level tokens, the
parents and the clamp over the fixture tables, and
`test/source/harness_topdown_run_test.cpp` drives levels 2 and 3, the clamp at 6
and SC-03's two tolerances over the scripted source.

### Tests for User Story 2 (write first, observe failure)

- [ ] T041 [US2] In `test/source/counters_topdown_select_test.cpp`, add the
  level-token cases of FR-014: rows grouped `TopdownL1` to `TopdownL6` answer
  levels 1 to 6, rows grouped `PipelineL1` and `PipelineL2` answer 1 and 2, and
  a row whose group carries no level token among its `;` tokens, the AMD
  `total_dispatch_slots` helper, yields no node and no refusal (FR-014, D-02,
  SC-01).
- [ ] T042 [US2] In `test/source/counters_topdown_select_test.cpp`, add the
  multi-level selection case over the fixture: every node below level 1 names
  the parent its own `_group` token states, the Intel `tma_<name>_group` and the
  AMD `<name>_group` spellings both resolve, and a level-2 row whose stem names
  no row of the directory yields no node and no invented parent (FR-013, SC-02).
- [ ] T043 [US2] In `test/source/counters_topdown_select_test.cpp`, add the
  clamp case: over the fixture table whose deepest level is 4, an asked level of
  6 answers `TopdownTree::depth` 4 and holds no node below level 4, and an asked
  level of 4 answers the same node set (FR-008, FR-009, FR-020).
- [ ] T044 [US2] In `test/source/detail/topdown_fixture.hpp`, add the rows of
  both AMD pipeline tables of FR-052 that Phase 2 left out, and in
  `test/source/counters_topdown_select_test.cpp` name the eight level-2 nodes of
  each table row for row, `backend_bound_cpu` of Zen 4 against
  `backend_bound_by_cpu` of Zen 5 with the seven that differ the same way, each
  under its level-1 parent (FR-013, FR-014, FR-052, D-2).
- [ ] T045 [US2] In `test/source/harness_topdown_run_test.cpp`, add the
  multi-level run over the fixture: at level 2 and at level 3 the collected set
  holds every node of that level and of every level above, the block states the
  level shown and indents each deeper node under its own parent with its vendor
  name beside it, one plan covers the set and states its pass count, and a run
  asking 6 against the depth-4 fixture records `askedLevel` 6 with `level` 4 and
  prints `topdown: the table holds no level deeper than 4` (FR-018, FR-020,
  FR-021, FR-023, FR-026, FR-027).
- [ ] T046 [US2] In `test/source/harness_topdown_run_test.cpp`, add SC-03's
  first tolerance: a run carrying a node's formula as a `--metric` column and
  asking that node's level reports node and column within 0.05 percentage point,
  keeps the column it reported before the option, and prints that row's
  threshold text unchanged with no verdict (FR-017, FR-023, FR-029, SC-03).
- [ ] T047 [US2] In `test/source/harness_topdown_run_test.cpp`, add SC-03's
  second tolerance over the AMD fixture tree with the scripted source: each of
  the five level-1 values answers its fixture figure within 0.05 percentage
  point and the five sum to 100 percent within 0.1 percentage point (FR-011,
  FR-056, SC-03, D-3).

### Implementation for User Story 2

- [ ] T048 [US2] In `source/counters/linux_pmu/topdown_select.cpp`, read level
  and parent from the group text: split on `;`, take the level from the one
  token matching `TopdownL([1-6])` or `PipelineL([12])`, take the parent from
  the remaining `*_group` token whose stem names another row of the directory,
  and hold no node for a row with no level token (FR-013, FR-014, D-02; turns
  T041 and T042 green).
- [ ] T049 [US2] In `source/counters/linux_pmu/topdown_select.cpp`, settle
  `TopdownTree::depth` to the deepest level the selected table holds whatever
  the asked level, and collect only the levels at or below it (FR-008, FR-009,
  FR-020; turns T043 green).
- [ ] T050 [US2] In `source/harness/runner.cpp`, append one metric column per
  node of the clamped level and of every level above it after the `--metric`
  loop at `:1315-1326`, hand the whole set to the standing planner, and record
  the asked level and the level shown on the result (FR-018, FR-020, FR-021,
  FR-023, E-17; turns T045 green).
- [ ] T051 [US2] In `source/harness/runner.cpp`, move `stripTopdownColumns` to
  drop only the node positions from every metric view of the result, so a user
  metric column whose label equals a node's vendor name keeps its column and its
  value (FR-023, FR-032, E-17; turns T046 green).
- [ ] T052 [P] [US2] In `source/harness/report.cpp`, print each deeper node
  indented under its own parent with its vendor name and its threshold text,
  print a gapped node with no value, and print the one line naming the deepest
  level the table holds when the asked level stands deeper (FR-020, FR-026,
  FR-027, FR-029, FR-030, D-12).

**Checkpoint**: `ctest --test-dir build`
`-R 'counters_topdown_select_test|harness_topdown_run_test'` passes and SC-03's
two tolerances hold over the scripted source.

---

## Phase 5: User Story 3 - Read the tree from the report and gate on it
(Priority: P2)

**Goal**: the tree rides the exported record, one `topdown` key of the entry
object carries it and its reader brings every field back, the schema lists the
key after `hunt` under a raised minor, and a `topdown:` rule compares one node's
value and refuses a node absent.

**Independent Test**: spec US3: write a report with the tree, read it back and
compare the values, then run the same report through the rule set with a rule
that names a node, one that names no node, and one against a report with no
tree. Here `test/source/harness_topdown_report_test.cpp` writes a tree, reads it
back and runs it through the rule set with a rule naming a level-1 node, one
naming a deeper node, one naming no node, and one with no tree.

### Tests for User Story 3 (write first, observe failure)

- [ ] T053 [US3] In `test/source/harness_topdown_report_test.cpp`, a new file,
  add `theRecordRoundTrips`: a written tree with its source, its two levels, a
  gapped node with its availability and threshold, and its refused rows reads
  back field for field unchanged through the report reader, and an unsupported
  record comes back with `supported` false, its reason and empty arrays (FR-032,
  FR-033, FR-034, SC-07). Add the new file's `add_executable` plus `add_test`
  block to `test/CMakeLists.txt` in the same commit, on the shape of
  `test/CMakeLists.txt:1483-1487` with the `source/harness` include directory.
- [ ] T054 [US3] In `test/source/harness_topdown_report_test.cpp`, add
  `aDocumentWithoutTheKeyLoads` and `aRunWithoutTheOptionWritesNoKey`: a
  document with no `topdown` key loads with the member absent, and a run without
  the option writes no key, so its bytes differ from the same run before this
  feature in the schema version alone (FR-034, FR-035, SC-06).
- [ ] T055 [US3] In `test/source/harness_topdown_report_test.cpp`, add
  `theWrittenTreePassesTheSchemaWithTheKeyAfterHunt`: the writer's own document
  with a `topdown` object passes `checkReport`, and the key stands after `hunt`
  in the entry order (FR-032, FR-036).
- [ ] T056 [US3] In `test/source/harness_json_schema_test.cpp`, add the
  planted-finding rows that report a `topdown` key written before `hunt` as out
  of the schema order, and an inner key the schema does not list as unlisted
  (FR-032, FR-036).
- [ ] T057 [US3] In `test/source/harness_topdown_report_test.cpp`, add the gate
  cases: `topdown:backend_bound:higher:abs=5` and
  `topdown:intel.tma_l1_bound:lower:10%` each compare that node's value in both
  reports, while a node the contender lacks, a contender with no `topdown`
  object, and a baseline written without the option each answer one quoted line
  naming the rule, the value, the benchmark and the report at status 2 (FR-037
  to FR-039, FR-050, SC-13).
- [ ] T058 [P] [US3] In `test/source/harness_compare_test.cpp`, extend
  `theGrammarFaultsEveryUnreadableCorner` (`:2468`) with `topdown:` naming
  nothing and with `topdown:backend_bound@median:lower:5%` refusing an
  aggregate, and `theGrammarAcceptsTheRemainingShapes` (`:2531`) with the two
  SC-13 rules read with the dotted node name and the threshold whole (FR-037).
- [ ] T059 [P] [US3] In `test/source/harness_compare_cli_test.cpp`, drive
  through the `oneFaultLine` helper (`:770`) the two exits: a rule naming an
  absent node and a rule against a report with no top-down object each print one
  line, print no table and exit with status 2 (FR-038, FR-039, SC-05).
- [ ] T060 [P] [US3] In `test/source/harness_cli_table_test.cpp`, move
  `theVersionBindingsReadTheMovedNumbers` (`:1382-1393`) to the project version
  `0.29.0` T070 lands, reading the harness `SOVERSION 20` and the archive
  `SOVERSION 5` Phase 2 landed, in the same commit as that version move (FR-047,
  D-4).
- [ ] T061 [P] [US3] In `test/source/harness_json_report_test.cpp`, move the
  stamp check of `theDocumentStampsTheMinorThisFeatureRequires` at `:546` and
  its prose line at `:547` to `1.12` (FR-036, D-17).
- [ ] T062 [US3] In `test/source/harness_json_schema_test.cpp`, move the stamp
  check of `entryHuntKeysPairAtStampedMinor` at `:186` and the prose lines at
  `:157` and `:187` to `1.12` (FR-036).
- [ ] T063 [P] [US3] In `test/source/harness_json_load_test.cpp`, move
  `kMutations[0]` at `:829` to `1.12`, and move both later-minor plants to
  `1.13` so they keep gating: the stamp at `:107` in
  `versionRuleGatesTheReader`, and the rewrite at `:1199` with its check at
  `:1202` in `theHeaderFieldsAndTheStoredWarningsLoad` (FR-036).
- [ ] T064 [P] [US3] In `test/source/harness_hunt_json_test.cpp`, move the
  `schemaConformance` check at `:1537` and its prose line at `:1538` to `1.12`
  (FR-036).
- [ ] T065 [P] [US3] In `test/source/harness_noise_report_test.cpp`, move the
  check at `:426` and the file-scope `kDocumentFixture` at `:457` to `1.12`,
  with the prose lines at `:413`, `:427` and `:455` (FR-036).

### Implementation for User Story 3

- [ ] T066 [US3] In `source/harness/json_report.cpp`, add `writeEntryTopdown`
  beside `writeEntryHunt` at `:509` and write the `topdown` key after the `hunt`
  key at `:715-718`, with a gapped node as `"value": null` and no key of the
  object ever absent (FR-030, FR-032, FR-033).
- [ ] T067 [P] [US3] In `source/harness/json_load.cpp`, add `readEntryTopdown`
  beside `readEntryHunt` at `:1799`, reached from the `optionalField` lane at
  `:2342-2349`, so an absent key reads as nothing and a node below level 1 with
  no parent refuses with the standing malformed-document shape (FR-034, FR-035).
- [ ] T068 [P] [US3] In `docs/schema/report-v1.json`, add `"topdown"` to the
  entry `x-order` after `"hunt"` at `:406` and the closed `topdown` property
  after the entry `hunt` property that closes at `:902`, leaving `required` at
  `:408-427` and `additionalProperties` at `:429` unchanged (FR-032, FR-033,
  FR-036).
- [ ] T069 [US3] In `source/harness/json_report.cpp`, move `kSchemaVersion` at
  `:43` from `1.11` to `1.12` (FR-036, FR-047, D-17).
- [ ] T070 [P] [US3] In `CMakeLists.txt`, move `VERSION 0.28.0` at `:45` to
  `0.29.0` with the comment naming the record and the requirement; the counters
  `SOVERSION` at `CMakeLists.txt:110` and the harness `SOVERSION` at
  `CMakeLists.txt:375` moved with their own commits in Phase 2 (FR-047, D-4).
- [ ] T071 [P] [US3] In `source/harness/compare.cpp`, add `"topdown:"` to
  `kLabelPrefixes` at `:55-56`, add `&& prefix != "topdown:"` to
  `statisticsValue` at `:400` so the aggregate refusal at `:431-435` answers a
  written `@median`, and seat one `topdown:` arm between the `metric:` arm at
  `:611-627` and the `stat:` fall-through at `:629-633` that re-joins the
  `topdown.` head, selects the node of `result.topdown` by neutral name and
  answers no value for a gapped node, leaving `missingValueFault` at
  `:1416-1424` to print the one line (FR-037 to FR-039).

**Checkpoint**: `ctest --test-dir build`
`-R 'harness_topdown_report_test|harness_json_schema_test'` passes, and a
document written without the option differs from the pre-feature bytes in the
schema version alone.

---

## Phase 6: User Story 4 - Understand a host that has no source (Priority:
P3)

**Goal**: one message names the vendor, the identity and the reason on a cpu
with no table, the run's exit status says nothing about it, the report records
the state so a later reader knows the tree is absent on purpose, and a refused
row prints once at the level it shows.

**Independent Test**: spec US4: select a tree for each unsupported identity
through the test seam and check the message, the record and the unchanged exit
status. Here `test/source/counters_topdown_select_test.cpp` drives the five
refusal texts over the identity seam, `test/source/harness_topdown_run_test.cpp`
drives the one line and the status, and
`test/source/harness_topdown_report_test.cpp` drives the refused rows and the
unsupported record.

### Tests for User Story 4 (write first, observe failure)

- [ ] T072 [P] [US4] Add the case `unsupportedAnswersQuoteTheIdentity` to
  `test/source/counters_topdown_select_test.cpp`: the five refusal texts of
  `specs/042-topdown/contracts/topdown-selection.md` verbatim over the identity
  seam, one synthetic `CpuIdentity` per kind, each text quoting the vendor, the
  family and model or the `MIDR`, and the reason (FR-004 to FR-007, FR-040,
  D-13, D-14).
- [ ] T073 [P] [US4] Add the case
  `anUnsupportedHostPrintsOneLineAndKeepsTheStatus` to
  `test/source/harness_topdown_run_test.cpp`: an unsupported selection prints
  one message on standard output, the run's exit status equals the same run
  without the option, and the record says top-down is unsupported (FR-040,
  FR-041).
- [ ] T074 [US4] Add the case `refusedRowsPrintAtTheLevelShown` to
  `test/source/harness_topdown_report_test.cpp`: the block lists each refused
  row once at the level it shows, with its UPPER SNAKE kind and its reason
  unchanged, and prints no node for it (FR-016, FR-031).
- [ ] T075 [US4] Add the case
  `theRecordCarriesRefusedRowsAndNamesAnUnsupportedHost` to
  `test/source/harness_topdown_report_test.cpp`: the `refused` array carries
  name, kind and reason per row, and an unsupported host writes `supported`
  false with the reason and two empty arrays (FR-033, FR-034).

### Implementation for User Story 4

- [ ] T076 [P] [US4] Answer those five refusals in
  `source/counters/linux_pmu/topdown_select.cpp`: `UNKNOWN_VENDOR`,
  `NO_DIRECTORY`, `HYBRID`, `ARM` and `NO_TOPDOWN_ROWS`, with `message()` one
  line apiece, turning T072 green (FR-004 to FR-007, FR-040, D-14).
- [ ] T077 [P] [US4] Print that one message from `source/harness/cli.cpp` when
  the selection answers unsupported, and change no exit status on that path,
  turning T073 green (FR-040, FR-041).
- [ ] T078 [P] [US4] Print the refused rows in the block of
  `source/harness/report.cpp`, one line each at the level shown with kind and
  reason, turning T074 green (FR-016, FR-031).
- [ ] T079 [P] [US4] Write the refused entries and the unsupported state of the
  top-down object in `source/harness/json_report.cpp`, turning T075 green
  (FR-033, D-14).
- [ ] T080 [P] [US4] Land the pending-row rule of FR-057 with no unit test,
  since a part with no host is a statement about hardware: one pending row per
  part with no host (`PERF_METRICS` parts, Intel hybrid parts, Arm parts) in the
  acceptance table of `specs/042-topdown/quickstart.md`, each naming no hardware
  claim, the same rows riding the acceptance record of the pull-request body
  (FR-057, SC-11, SC-12, D-16).

**Checkpoint**: `ctest --test-dir build`
`-R 'counters_topdown_select_test|harness_topdown_report_test'` passes with the
refusal fixtures planted, and no exit status moves on an unsupported host.

---

## Phase 7: User Story 5 - Copy a working block from the page (Priority:
P3)

**Goal**: a reader opens the main page, reads one short section about the tree,
copies the block and builds a program that prints level 1 or the unsupported
message, while the guide page carries the longer explanation.

**Independent Test**: spec US5: the drift gate compares the page block with the
region in the example file, the option row with the option table, and the
section list with the page, and the run gate executes the example and fails when
a registered benchmark prints no row. Here that is
`python3 tools/docs/example_drift.py .` and `bash test/example_run.sh build`.

- [ ] T081 [P] [US5] Write `example/docs/topdown.cpp`, a new file: read the host
  identity, print the level-1 values on a host with a source and the unsupported
  message on a host without one, bound the printed body between
  `// sg-example:begin topdown` and `// sg-example:end topdown`, and register
  one benchmark with a `SG_BENCHMARK` line so the run gate of
  `test/example_run.sh` finds a non-empty `--list` and one row per registered
  benchmark (FR-045, FR-041). This file, its registration, its two lists in
  `tools/docs/example_drift.py` and the README block that quotes it land in one
  commit (FR-048).
- [ ] T082 [US5] Register the example in `example/CMakeLists.txt`:
  `add_docs_example(topdown)` after `add_docs_example(report_views)` at `:82`,
  which builds `docs_topdown` and hangs `run_docs_topdown` on `run-examples` for
  `test/example_run.sh` (FR-045). Same commit as the new file of T081.
- [ ] T083 [US5] Add `"Top-down analysis"` to `SECTIONS` at
  `tools/docs/example_drift.py:33` and its owner key
  `"Top-down analysis": ("topdown.cpp",)` to `SECTION_OWNERS` at `:73`, so the
  drift gate demands the page section, its owner file and the region T081 wrote;
  the two lists move in one change, the one that also writes the README section,
  because the tool raises `KeyError` at `tools/docs/example_drift.py:683` when
  one moves without the other and reads `no '### Top-down analysis' section`
  until the page carries it (FR-044; spec Constraints).
- [ ] T084 [US5] Write the 34th capability section of `README.md` under
  `## How you do it` between `### Multi-pass collection` at `:326` and
  `### Skips` at `:350`: heading, one purpose sentence, the
  `<!-- sg-example: example/docs/topdown.cpp#topdown -->` marker naming the new
  file of T081, the 15-line region body, one result sentence (21 lines), paid by
  21 named trims, the 20 blank lines before the headings at README.md lines 410,
  428, 446, 468, 492, 513, 537, 562, 578, 601, 620, 635, 657, 675, 693, 740,
  763, 786, 811 and 834 and the blank between purpose and marker at
  `README.md:390`, so the page holds its `PAGE_LINE_CAP` of 1004 that
  `tools/docs/example_drift.py:240` reads and the quoted body compiles alone
  under `test/snippet-compile/run.sh` (FR-044, D-15). Same commit as T081, T082
  and T083.
- [ ] T085 [US5] Add the two quick-reference name rows `topdownTree` and
  `TopdownTree` to `README.md` at the head of the graph block, above the
  `GraphRecord` row that stands at `README.md:947` at the base hash and at
  `:946` once T033's trim and option row land (2 lines), paid by the two
  remaining trims, the `## Planned work` rows at `README.md:1001` and `:1002`,
  so the page again measures 1004 lines and the drift gate finds no
  `long option '--topdown' has no option row`, that row having landed with T033
  (FR-044, D-15).
- [ ] T086 [P] [US5] Write `## Top-down analysis` in `docs/pages/harness.md`
  after the `## Multi-pass collection` section that opens at `:1549`: the names,
  the levels, the source record, the five unsupported reasons and the `topdown:`
  rule form (FR-046). The option row of the table that opens at
  `docs/pages/harness.md:666` landed with T033, which
  `thePageAndTheTableAgreeInBothDirections` compares against the live row in
  both directions.

**Checkpoint**: `python3 tools/docs/example_drift.py .` reports no finding,
`bash test/example_run.sh build` runs `docs_topdown`, and
`bash test/snippet-compile/run.sh .` compiles the page block alone.

---

## Phase 8: Polish and Cross-Cutting Concerns

**Purpose**: The discipline requirements that span the feature: the memory-bound
case FR-058 names, the gate order, the coverage evidence, the acceptance record
D-16 names, and the one open timing bound.

- [ ] T087 [P] Write `example/memory_walk.cpp`, a new file: one memory-bound
  benchmark whose working set stands above the 19.3 MiB last-level cache of the
  acceptance host, where the largest shipped set stands at 64 KiB of
  `example/docs/throughput.cpp:25`; its verification is the acceptance run of
  `specs/042-topdown/quickstart.md`, and `test/example_run.sh` walks
  `example/docs` alone (FR-058, SC-10).
- [ ] T088 Register the benchmark of T087, the new file
  `example/memory_walk.cpp`, in `example/CMakeLists.txt`:
  `add_example(memory_walk)` beside `add_example(first_benchmark)` at `:33`, so
  `./build/example/memory_walk` exists for the acceptance run of
  `specs/042-topdown/quickstart.md` (FR-058).
- [ ] T089 Run the gate order of `specs/042-topdown/quickstart.md` end to end,
  `python3 tools/docs/example_drift.py .`, `cmake -P cmake/prose-lint.cmake`,
  `bash test/time_source_gate.sh .`, `cmake -P cmake/spell.cmake`, the
  `cmake/lint.cmake` format pass, `bash test/snippet-compile/run.sh .`,
  `bash test/example_run.sh build` and the full
  `ctest --test-dir build --output-on-failure`, and record each verdict and cost
  in that page, the `.codespellrc` line for the model number `BA` in place
  (D-16, FR-048, FR-049).
- [ ] T090 [P] Run the coverage build to `tools/dbc/coverage_gate.sh`:
  `lines......: 100.0%` at `:45-48`, the branch shortfall inside the standing
  `allowance=21` at `:102`, and no new exclusion marker anywhere in the tree
  (FR-051, SC-09).
- [ ] T091 Write the acceptance record D-16 names into
  `specs/042-topdown/quickstart.md` and the pull-request body: the host, the
  kernel, the level-1 values, the refused rows, and the test counts, with the
  two-cpu `taskset -c 0,1` run among them (D-16, SC-10, FR-049).
- [ ] T092 State the fixture timing bound in the acceptance record of
  `specs/042-topdown/quickstart.md`:
  `test/prose-gate-fixture/run_prose_gate_fixtures.py:84` bounds one gate run at
  10.0 seconds and this host measures 11.559 over 909 sources, so
  `prose_gate_fixtures` fails at the base hash; two exits stand, move the bound
  in `test/prose-gate-fixture/run_prose_gate_fixtures.py` in its own commit with
  the measured figure in its body, or record the failure as pre-existing beside
  this change's results, and the owner's choice decides which one this lane
  takes (PC-13).

**Checkpoint**: every gate of Principle VIII is green on the pushed state and
the acceptance table of `specs/042-topdown/quickstart.md` carries no pending row
that claims hardware.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1** has no dependencies.
- **Phase 2** depends on Phase 1 for the skip rows its two new units carry, and
blocks every story: the selection surface, the threshold text, the settled
`#core_wide` query and the record member are read by every later phase.
- **Phase 3 (US1)** depends on Phase 2 and is the MVP.
- **Phase 4 (US2)** depends on Phase 3: the levels ride the node columns and the
strip Phase 3 installs.
- **Phase 5 (US3)** depends on Phase 3 for the record the writer publishes and
on Phase 2 for the carrier member and the threshold text a printed node reads.
- **Phase 6 (US4)** depends on Phase 2 for the selection seam, on Phase 3 for
the record and the block, and on Phase 5 for the report object that carries the
refused rows.
- **Phase 7 (US5)** depends on Phases 3 to 6, because the page quotes the
finished block, the finished option row and the finished refusal message.
- **Phase 8** depends on every story the branch ships.

```text
Phase 1 Setup (tier rows)
   |
   v
Phase 2 Facts, carriers, selection surface   blocks every story
   |
   v
Phase 3 US1 option -> one plan -> block      (MVP)
   |--------------|
   v              v
Phase 4 US2    Phase 5 US3
levels+clamp   key + reader + rule
   |--------------|
                v
        Phase 6 US4 refusals
                v
        Phase 7 US5 page + example
                v
        Phase 8 acceptance, coverage, gates
```

### Commit sets

These land as single commits. They are one commit each for a reason, so they
never run side by side as [P] tasks.

- Every test task above lands in the commit of the code task that turns it green
(FR-048).
- Threshold chain: T008 with T020, the counters `SOVERSION` rise at
`CMakeLists.txt:110` and the move of `theVersionBindingsReadTheMovedNumbers`.
- Report carrier: T021 carries the harness `SOVERSION` rise at
`CMakeLists.txt:375`, its comment row and the same test move, so no commit ships
the member with an unrisen number.
- Published query: T009 with T017, which Phase 3's scope line T040 reads.
- Option row: T033 carries the live row, the row of the guide table, the row of
the README table and the README trim, because the drift gate and
`thePageAndTheTableAgreeInBothDirections` read all three in one pass.
- Registrations: T025 lands the two counters blocks, and each harness unit lands
with the block of the task that writes its first real cases: T026 and T028 in
Phase 3, T053 in Phase 5.
- Schema stamp: T069 with T061, T062, T063, T064 and T065, because six of the
eight sites assert the stamp the writer carries and the later-minor plants would
pin a minor that has become current.
- Report tree: T066, T067, T068 and T053, because a writer that emits `topdown`
against a schema that omits it fails the `checkReport` call of
`test/source/harness_json_schema_test.cpp:57`, and the round trip compiles
against the reader.
- Version move: T070 with T060, because that one case reads all three numbers
out of `CMakeLists.txt`.
- Page set: T081, T082, T083 and T084, because the drift gate reads red between
the section lists, the owner file and the README block.

### Story independence

- **US1** is the feature: the option, one plan, one block, one record. It stands
with no report key, no rule and no page.
- **US2** deepens what US1 collected and reads nothing US3 builds.
- **US3** exports and gates what US1 recorded, and needs no page.
- **US4** answers the hosts US1 cannot serve and touches no page.
- **US5** documents the finished shape and owns its example.

### Within Each User Story

- Tests first, and they fail on the missing behaviour before the code lands
(FR-048).
- Selection before tree, tree before node columns, columns before block, block
before report key, report key before rule, rule before page.
- Story complete before the next priority starts.

### Parallel Opportunities

- Phase 1 holds one task.
- Phase 2: the five [P] test tasks T002, T003, T004, T007 and T010 run together,
and T013, T014, T019, T018 and T023 run beside them.
`source/counters/linux_pmu/metric_tables.hpp` takes T015 then T016 then T017 in
sequence, and T008, T020 and T021 share `CMakeLists.txt` and the version-binding
case, so they run in sequence.
- Phase 3: T026 and T027 run together at the opening; inside
`test/source/harness_topdown_run_test.cpp` the five tasks T028, T029, T030, T031
and T032 run in sequence, each with its own code task.
- Phase 4: the four tasks in `test/source/counters_topdown_select_test.cpp` run
in sequence, the three in `test/source/harness_topdown_run_test.cpp` run in
sequence, and the two groups run side by side; T052 runs beside both.
- Phase 5: the five [P] stamp and version tasks T060, T061, T063, T064 and T065
each hold one file and run together, with T058 and T059 beside them; T062
follows T056 in `test/source/harness_json_schema_test.cpp`; the tasks of
`test/source/harness_topdown_report_test.cpp` run in sequence.
- Phase 6: `source/counters/linux_pmu/topdown_select.cpp` and
`source/harness/cli.cpp` each take one pair, and the two pairs run together with
T078 and T079.
- Phase 7: T081 and T086 run together; `README.md` takes T084 then T085 in
sequence, and the page set lands in one commit.
- Phase 8: T087 and T090 run together; `example/CMakeLists.txt` takes T088 after
T087, and `specs/042-topdown/quickstart.md` takes T089, T091 and T092 in
sequence.

### The internals page the drafts name

One draft reads an internals page that records two skips over labels beginning
`topdown.` in place of the strip step, and names that page `_core.md`. No such
page exists: `docs/pages/` holds `about.dox`, `barrier-rule.md`, `building.md`,
`counters-overhead.md`, `dbc-migration.md`, `dbc-overhead.md`, `harness.md`,
`identifier-rename-map.md` and `prepush.md`, and none of them carries the string
`topdown` anywhere. No task corrects a page, and the strip stands as T036 and
T051 write it.

---

## Parallel Example: User Story 1

```bash
# Option surface, two files, side by side:
Task: "Write the option cases in test/source/harness_topdown_option_test.cpp"
Task: "Move the 43-row oracle in test/source/harness_cli_table_test.cpp"
# Then the live row with its three page rows, one commit:
Task: "Add the row to options.cpp, README.md and docs/pages/harness.md"
# Then the arm that reads the level, then the run spine:
Task: "Read the level in source/harness/options.cpp"
Task: "Write the collection cases in test/source/harness_topdown_run_test.cpp"
Task: "Append the node columns in source/harness/runner.cpp"
# Then the strip, the refusals, the block and the scope line, in order:
Task: "Define stripTopdownColumns in source/harness/runner.cpp"
Task: "Add printTopdownBlock in source/harness/report.cpp"
Task: "Print the core-wide scope line in source/harness/cli.cpp"
```

---

## Implementation Strategy

### MVP First (User Story 1)

1. Complete Phase 1: the two tier-label rows.
2. Complete Phase 2: the three repairs, the host facts and their query, the
threshold text, the two carriers and the selection surface.
3. Complete Phase 3: the option, the node columns, the strip and the block.
4. **Stop and validate**: `harness_topdown_option_test` and
`harness_topdown_run_test` prove the level-1 block and the record with the
fixture tree and the scripted source. A run without the option prints master's
bytes.

### Incremental Delivery

1. Phases 1 and 2 change no run's output.
2. US1 prints the block and fills the record.
3. US2 takes a level and clamps it to the table.
4. US3 writes the key, reads it back and gates on it.
5. US4 answers the hosts with no source.
6. US5 teaches the shape on the page.
7. Phase 8 records the evidence and runs the gates.

### Rebase Note

Other lanes move the same shared files. `CMakeLists.txt` carries the project
version at `:45` and the two `SOVERSION` lines at `:110` and `:375`; `README.md`
and `PAGE_LINE_CAP` at `tools/docs/example_drift.py:240` carry the page budget;
`docs/pages/harness.md`, `test/CMakeLists.txt`, `example/CMakeLists.txt`,
`docs/schema/report-v1.json` and `.codespellrc` carry the rows this feature
adds. Rebase onto `master`, keep both lanes' lines, retake the version and the
two archive numbers against the merged master, re-measure the page budget, and
run the full gate set once more before the push.

---

## Notes

- [P] marks a task in a different file with no dependency on an open task. A
commit set listed above overrides the label: the tasks it names land together.
- A run without `--topdown` prints what master prints, so every console task
carries a byte rule beside its new shape.
- The four version numbers move in four commits: the counters `SOVERSION` with
T020, the harness `SOVERSION` with T021, the project version with T070 and the
schema minor with T069. Each move carries the check that reads it, the way
FR-047 requires.
- `source/counters/linux_pmu/counter_limits.cpp`, every limit path and every
file under `.github/` stay untouched (FR-022, PC-10, D-16).
- Stop at any checkpoint and validate one story on its own.

