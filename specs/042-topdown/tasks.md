# Tasks: Vendor-neutral Top-down Analysis

**Input**: Design documents from `specs/042-topdown/`

**Prerequisites**: [spec.md](spec.md) (FR-001 to FR-058, SC-01 to SC-13, user
stories US1 to US5, decisions D-1 to D-21, the audit point at `65c48be9`),
[plan.md](plan.md), [research.md](research.md) (D-01 to D-16),
[data-model.md](data-model.md) (E-01 to E-18, invariants I-1 to I-7),
[quickstart.md](quickstart.md), [contracts/](contracts/) (five files)

**Tests**: Included and mandatory. FR-048 puts each new behavior's test in the
same commit as the code it covers, so a test task and the code task that turns
it green are one commit and run in that order. FR-051 keeps line and branch
coverage at 100 percent with the branch allowance at 21 and no new exclusion
marker, and every unit of this feature runs without the kernel event interface,
without a sleep and without a clock read (FR-048, SC-08).

**Organization**: Phase 1 carries the tier-label rows the two new counters units
need. Phase 2 lands the counters and harness facts every story reads. Phases 3
to 7 carry US1 to US5 in spec priority order. Phase 8 carries the acceptance
record, the coverage evidence and the CI gate set. Within a phase a test task
precedes the code task it gates, and the tasks that must move together are named
as commit sets in the dependencies section.

**Paths and ids**: paths are repo-relative. The line numbers quoted below are
the ones read from the tree at the audit point `65c48be9`, the hash the five
files under `contracts/` and the evidence note of `data-model.md` name; a line
that moves because an earlier commit of this lane edited the same file is named
by the text it stands beside. `754867de` is the last commit that changed a
source file, so a test marked red there is red on the tree this list lands on. A
decision cited `spec D-n` stands in the decision list of `spec.md`; one cited
`research D-0n` stands in `research.md`.

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
explicit block in `test/CMakeLists.txt`, which the commit that writes its first
real cases lands.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: can run in parallel with other [P] tasks: a different file and no
  dependency on an open task, and no file another [P] task of the same phase
  names
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

- [x] T001 [P] In `test/tier_labels.sh`, add the rows
  `counters_topdown_select_test` and `counters_topdown_source_test`, each at
  skip code 2, to the `MASTER_SKIP` baseline at `:77-86`, so the tier gate
  accepts the skip rule the two counters units of T006 and T010 carry. Each unit
  takes its `SKIP_RETURN_CODE 2` row from the same commit that writes its cases,
  so this row and that registration land together (FR-048).

**Checkpoint**: `bash test/tier_labels.sh build ctest` reports no
`skip-rule`
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

- [x] T002 [P] In `test/source/counters_linux_pmu_seam_test.cpp`, extend
  `pageGateScenario` with the fixed-counter case: `fastIndexValid(0x40000002)`
  answers true, `0x40000000 + kRnpmcMaxIndex + 1` and `0` still refuse, and a
  `fastDecode` at the marked index answers OK, with no event opened (FR-053; red
  at `754867de`). This case lands in the commit of T013 (FR-048).

- [x] T003 [P] In `test/source/harness_cli_test.cpp`, add
  `pmuProviderIsOfferedOncePerProcess`: the entry point's offer helper in
  `source/harness/detail/internal.hpp` answers offered on the first call and
  already offered on the second in one process, so a second entry point call
  builds no provider (FR-055; red at `754867de`). This case lands in the commit
  of T014 (FR-048).

- [x] T004 In `test/source/counters_metric_trees_test.cpp`, add
  `smtStateFallsBackToTheSiblingList`: over the coreless tree with a fake root
  from `installTestProcRoot`, `#SMT_on` answers 1 from a
  `cpu<N>/topology/core_cpus_list` of `0,12`, 0 from a list naming one id, and
  nothing from an unreadable list, while a tree that answers a core object keeps
  priority (FR-054; red at `754867de`). This case lands with T015 and the moved
  header of T012 in one commit.

- [x] T005 In `test/source/counters_metric_trees_test.cpp`, add
  `coreWideAnswersFromTheProcessCpuSet`: from the process's own affinity and a
  fake sibling list, `#core_wide` answers 1 when the mask holds every sibling of
  every cpu it holds, 0 when one stands outside, and nothing when a list is
  unreadable, and it answers no slot width (FR-042 as settled at `754867de`; red
  there). This case lands in the commit of T016 (FR-048).

- [x] T006 [P] In `test/source/counters_topdown_source_test.cpp`, a new file,
  write the source record over a two-file fixture directory: the reference is
  the manifest `ref:` line, each contributing file answers its `TableFile` name
  and `manifestHash` from the `RECORD` line beside it, the files stand in the
  embedder's sorted order, and a row of one file names only that file (FR-015;
  red at `754867de`). The same commit registers the file in
  `test/CMakeLists.txt` in the standing six-line shape of
  `test/CMakeLists.txt:515-522`, with `SKIP_RETURN_CODE 2` matching the
  `test/tier_labels.sh` row of T001 and with a `SG_TOPDOWN_TABLE_DIR` define
  this task adds beside the `SG_SEAM_TABLE_DIR` define of
  `test/CMakeLists.txt:484-489`, each naming its own build-tree fixture
  directory. This case lands with T018 and T019 in one commit (FR-048).

- [x] T007 In `test/source/counters_linux_pmu_seam_test.cpp`, add
  `thresholdTextReachesThePublishedView`: a seam table file carrying
  `MetricThreshold` shows the text reaching the parsed entry and the published
  `MetricView` byte for byte, and a row that carries no key answers the empty
  string (FR-017; red at `754867de`). This case lands in the commit of T020
  (FR-048).

- [x] T008 In `test/source/counters_metric_trees_test.cpp`, add
  `coreWideAnswersThroughThePublishedQuery`: both readings of `#core_wide` reach
  the caller through `coreWideCoverage()` over the fake root from
  `installTestProcRoot`, one answering `WHOLE_CORE` and one `ONE_THREAD`, on the
  pattern of the `metricByName` cases, and the answer stands unchanged over a
  second call, so no caller needs a reader of its own (FR-042; red before the
  query lands). This case lands in the commit of T017 (FR-048).

- [x] T009 [P] In `test/source/detail/topdown_fixture.hpp`, a new file, write
  the synthetic tables the selection reads: Intel level-1 to level-6 rows with
  both parent spellings, one group-free helper row, a level whose rows print
  under two objects, refused rows with kind and reason, threshold text, and both
  AMD pipeline tables of FR-052 with their eight differing level-2 names. Give
  event rows their scope: the parse reads a row's `Unit` text into
  `PmuTableEntry::unit` at `source/counters/detail/pmu.hpp:45`, filled at
  `source/counters/linux_pmu/table_parse.cpp:419` for a metric row and `:515`
  for an event row, and the resolution carries it as `ParsedMetricRow::scope` at
  `source/counters/linux_pmu/metric_formula.hpp:107-114`; `fixtureEvent` of
  `test/source/detail/metric_fixture.hpp:50-59` sets that same field, so one
  shape serves both fixtures. Hold one directory whose rows carry the two scopes
  `cpu_core` and `cpu_atom` and one that carries a single scope, so a two-scope
  directory stands in data with no host to read and no mapping-file row moves
  (FR-004 to FR-017, FR-052, FR-006, spec D-13). The header holds no assertion,
  so it carries no case of its own and lands in the commit of T010, the first
  unit that reads it.

- [x] T010 In `test/source/counters_topdown_select_test.cpp`, a new file, write
  `cpuIdentityCarriesTheReadersFourFields` and `selectsTheTableOfTheIdentity`:
  model 85 stepping 4 selects `arch/x86/skylakex/`, family 25 model 17 selects
  `amdzen4`, family 26 model 17 selects `amdzen5`, the four production fields
  equal `pmuIdentCurrent()`, and the core type and `MIDR` are handed in by the
  test; add `oneCallReadsNoSourceOfItsOwn`, where one call over the seam reads
  no counter, opens no event, reads no clock and calls no `CPUID` reader of its
  own (FR-001 to FR-003; red at `754867de`). The same commit registers the file
  in `test/CMakeLists.txt` in the standing six-line shape of
  `test/CMakeLists.txt:515-522`, with `SKIP_RETURN_CODE 2` matching the
  `test/tier_labels.sh` row of T001 and with the `SG_TOPDOWN_TABLE_DIR` define
  T006 adds. This case lands with T022 in one commit (FR-048).

- [x] T011 In `test/source/counters_topdown_select_test.cpp`, write the tree
  cases over the fixture table: `levelOneCarriesTheSharedNames`,
  `parentsComeFromTheGroupText`, `levelDeeperThanTheTableClamps`,
  `refusedRowsStayInTheDocument`, `rowsUnderTwoObjectsBothReach`,
  `thresholdRidesAsTextUnchanged` and
  `levelTwoNamesDifferBetweenTheTwoAmdTables`; add
  `aNodeCarriesItsFormulaAndItsEventList` so each node reaches the published
  view with the formula of its row and the events that formula needs (FR-010);
  and add `twoCallsOnOneIdentityAnswerOneTree`, where two calls with one
  identity and one table answer equal trees (FR-009; FR-008 to FR-017, FR-020,
  FR-052; red at `754867de`). These cases land with T024 in one commit (FR-048).

### Implementation for this phase The same commit publishes the declaration of `topdownTree()` in `include/speedgun-ng/counters_topdown.hpp`, the publication `T022` held back so that no commit carries a documented clause with no check beside it.
- [x] T012 [P] Move `source/harness/detail/proc_root.hpp` to
  `source/detail/proc_root.hpp`, a new file at the lower path that holds the
  same reader, and change the four includes in
  `source/harness/jitter_probe.cpp`, `source/harness/noise_tracer.cpp`,
  `test/source/harness_noise_test.cpp` and
  `test/source/harness_precheck_test.cpp` to the new path; no behaviour moves,
  so no case is red here and `smtStateFallsBackToTheSiblingList` of T004 covers
  the path together with T015, whose commit carries this move (FR-054, spec
  D-19).

- [x] T013 [P] In `source/counters/linux_pmu/fast_read.cpp`, bound the counter
  number in `fastIndexValid` with the fixed-counter mark masked off, keep the
  unmarked operand for the instruction, and correct the comment above the bound
  (FR-053, spec D-18; turns `pageGateScenario` of T002 green).

- [x] T014 In `source/harness/cli.cpp`, offer `counters::PmuProvider` once per
  process behind the helper in `source/harness/detail/internal.hpp`, beside the
  clock registration, and drop the per-call construction at
  `source/harness/cli.cpp:579` (FR-055, spec D-21; turns
  `pmuProviderIsOfferedOncePerProcess` of T003 green).

- [x] T015 In `source/counters/linux_pmu/metric_tables.hpp`, give `readSmtState`
  its sibling-list fallback over `procRootPath` from
  `source/detail/proc_root.hpp`: `core_cpus_list`, then `thread_siblings_list`,
  the cpu from `::sched_getcpu()`, and nothing when that read fails (FR-054;
  turns `smtStateFallsBackToTheSiblingList` of T004 green). The moved header of
  T012 rides this commit.

- [x] T016 In `source/counters/linux_pmu/metric_tables.hpp`, add the fifth
  `kHostFacts` row `core_wide` over the standing `bool(System&, double&)`
  reader, reading the mask with `sched_getaffinity` and each sibling list
  through the fallback reader of T015, and move `#core_wide` out of the header
  comment that lists the four refusing constants. The same reading moves the
  list in `test/source/counters_metric_formula_test.cpp:395-411`, which pins
  `#core_wide` among the constants that answer no source and refuses the row
  that names it: the row now has a source, so the name leaves that list and the
  case that reads it keeps its other three (FR-042, FR-043; turns
  `coreWideAnswersFromTheProcessCpuSet` of T005 green).

- [x] T017 In `include/speedgun-ng/counters_metrics.hpp`, publish
  `coreWideCoverage()` beside `metricByName` at `:81` with the same
  `SPEEDGUN_NG_EXPORT` mark its neighbors carry: it answers
  `std::optional<CoreCoverage>`, over the `CoreCoverage` enumeration this header
  gains as a closed `enum class : std::uint8_t` holding `WHOLE_CORE` and
  `ONE_THREAD`, the shape `Availability` uses at
  `include/speedgun-ng/counters_core.hpp:87-94`, and an absent answer keeps the
  standing refusal of `#core_wide`. Principle II needs the contract wording with
  the name, the three rows `specs/042-topdown/contracts/topdown-selection.md:55`
  states: `\pre` none, `\post` nothing before the first read of a metric table
  and one value for the life of the process afterwards, and `\invariant` that a
  settled value never moves inside one process; the value states no slot width,
  because the width is a literal of the table. Define the query in
  `source/counters/linux_pmu/metric_formula.cpp` over the host-fact row T016
  lands, and hold no private reader beside it (FR-042, FR-043, spec D-6; turns
  `coreWideAnswersThroughThePublishedQuery` of T008 green). The `\post` and `\invariant` tags read `none` and the three claims stand in the paragraph above them, because `tools/dbc/dbc_pair_gate.py` pairs a documented postcondition with a check of that kind and pairs a function row on precondition, postcondition and assertion only: a tag on a call that settles one value per process would ask for a check that repeats the value the call just settled.
- [x] T018 [P] In `cmake/embed_pmu_blob.cmake` and `cmake/EmbedPmuEvents.cmake`,
  pass the `RECORD` path to the generator, read the `ref:` line and each file's
  sha256, emit both into the index and the registry, and refuse a file that
  holds no hash line; in `source/counters/linux_pmu/embedded_tables.hpp`, give
  `EmbeddedFile` its `sha256` and `EmbeddedDir` its `reference` beside
  `embeddedFindDir` (FR-015, spec D-12; turns the source record of T006 green).

- [x] T019 In `source/counters/detail/pmu.hpp` and
  `source/counters/linux_pmu/table_parse.cpp`, give `PmuTableEntry` a `fileName`
  and tag every entry `pmuLoadTable` appends with the embedded file it came
  from, so a published row can name its file (PC-12). No case of its own stands
  here: the row of `counters_topdown_source_test` that names each contributing
  file, T006, is its test, so this change rides that case's commit with T018.

- [x] T020 In `source/counters/detail/pmu.hpp`,
  `source/counters/linux_pmu/table_parse.cpp`,
  `source/counters/linux_pmu/metric_formula.hpp`,
  `source/counters/linux_pmu/metric_formula.cpp` and
  `include/speedgun-ng/counters_metrics.hpp`, carry the threshold text from the
  table file to the parsed entry and to the published `MetricView` unchanged,
  with no evaluation anywhere; the same commit raises the counters archive
  `SOVERSION` from 4 to 5 at `CMakeLists.txt:110` with the comment row of the
  chain at `CMakeLists.txt:102-109` that gives the reason, and moves both pins
  that read that number with it: the archive check of
  `theVersionBindingsReadTheMovedNumbers` at
  `test/source/harness_cli_table_test.cpp:1391` and the archive check of the
  version block at `test/source/views_test.cpp:254`, each with the comment that
  quotes it, the check-map entry at
  `test/source/harness_cli_table_test.cpp:128`, the prose line at `:1377` and
  the block comment at `test/source/views_test.cpp:244`. Both checks search
  `CMakeLists.txt` text, so each must name the line it reads before the number
  moves: `SOVERSION 4` stands at `CMakeLists.txt:110` and again in the four
  comments at `:352`, `:357`, `:363` and `:374`, and those four record earlier
  lanes, so they stay and the read narrows (FR-017, spec D-11, FR-047; turns
  T007 green).

- [x] T021 In `include/speedgun-ng/benchmark.hpp`, add `TopdownNode`,
  `TopdownRefusal` and `TopdownRecord` as `specs/042-topdown/data-model.md` E-16
  types them: `supported` and `reason`, `reference` and `directory`, `files` as
  the `std::vector<TableFile>` of contributing files with E-02's `name` and
  `manifestHash` in the embedder's sorted order, `askedLevel` and `level`, and
  the two node and refusal vectors, no vendor flag anywhere, because the fifth
  level-1 value and the 100 percent statement of FR-028 are printed and never
  published. Hold them as `std::optional<TopdownRecord> topdown` on
  `BenchmarkResult` beside `hunt` at `include/speedgun-ng/benchmark.hpp:747`, no
  value meaning the option never stood. Principle II needs a `\pre`, a `\post`
  and an `\invariant` on each of the three published types, the `\invariant`
  stating the shape no member may break: a node's level stands one below its
  parent's level, and an absent record carries no node. The same commit raises
  the harness archive `SOVERSION` from 19 to 20 at `CMakeLists.txt:375` with the
  comment row of the chain at `:369-374`, and moves both pins that read that
  number with it: the harness check of `theVersionBindingsReadTheMovedNumbers`
  at `test/source/harness_cli_table_test.cpp:1389` with its check-map entry at
  `:126` and its prose line at `:1376`, and the harness check of the version
  block at `test/source/views_test.cpp:252` with its block comment at `:243`
  (FR-032, FR-033, E-16, spec D-4, FR-047). The member answers in the commit of
  T035, where `theNodeFigureStandsInTheRecord` of T028 reads it. The `\invariant` tag on the three record types reads `none` and each type states its shape in the paragraph above the tags, the way the eleven types of `include/speedgun-ng/counters_topdown.hpp` do: the pairing gate reads a class invariant only from a member definition of that class, and a data-only record has no member definition to hold a check that is not a tautology.
- [x] T022 In `include/speedgun-ng/counters_topdown.hpp` and
  `source/counters/linux_pmu/topdown_select.cpp`, each a new file, publish
  `CpuIdentity`, `TableFile`, `TopdownSource`, `TopdownRow`, `TopdownTable`,
  `TopdownNode`, `TopdownRefusal`, `TopdownTree`, `TopdownSelection`,
  `TopdownUnsupport` and `TopdownUnsupported` as
  `specs/042-topdown/data-model.md` E-01 to E-11 types them, the one name
  `TopdownRefusal` serving the library's refused row and E-16's record row the
  way `TopdownNode` serves both node carriers, and publish `cpuIdentity()`,
  `topdownTable(const CpuIdentity&)` and `topdownTree(const TopdownSelection&)`
  answering `std::expected<TopdownTree, TopdownUnsupported>` as
  `specs/042-topdown/contracts/topdown-selection.md:14-39` spells them. Each
  type and each call carries its `\pre`, `\post` and `\invariant` from that
  contract's table at `:48-55`, and the header joins `NAME_SCAN_HEADERS` with
  T023. Implement `cpuIdentity()` over `pmuIdentCurrent()` at
  `source/counters/detail/pmu.hpp:84` and `topdownTable()` over
  `pmuSelectDirectory` at `:99-100` (E-01 to E-05; turns both cases of T010
  green). `topdownTree()` and `TopdownUnsupported::message()` do not stand in this header: `T024` publishes the tree call with the `SG_REQUIRE` its `\pre` names and `T075` publishes the one line with its `\post`, because the pairing gate reads a documented clause with no check beside it as drift, and a public header that declares a call with no definition is a link error waiting for a caller.
- [x] T023 [P] In `tools/docs/example_drift.py`, add `"counters_topdown.hpp"` to
  `NAME_SCAN_HEADERS` at `:530-538`, so a quick-reference name row for the new
  header answers no `quick-reference name ... is not in` finding (FR-044; spec
  Constraints). No unit test covers the tool:
  `python3 tools/docs/example_drift.py .` reads it, and this change lands after
  T022 writes the header it scans.

- [x] T024 In `source/counters/linux_pmu/topdown_select.cpp`, the new file of
  T022, implement `topdownTree()`: the `SG_REQUIRE` that `options.level` stands
  in 1 to 6, the enforcement site
  `specs/042-topdown/contracts/topdown-selection.md:50` names and
  `tools/dbc/dbc_pair_gate.py:17-23` pairs with the documented `\pre`; the level
  token over `TopdownL1` to `TopdownL6` and `PipelineL1` and `PipelineL2`; the
  `_group` parent rule over both spellings; the four neutral level-1 names with
  `topdown.smt_contention` on an AMD table alone;
  `topdown.<vendor>.<table name>` below level 1; each node's formula and the
  events that formula needs; the clamp to the table's own depth; and the refused
  rows, with the five unsupported texts of
  `specs/042-topdown/contracts/topdown-selection.md` left to T075 in Phase 6
  (FR-008 to FR-016, FR-020; turns every case of T011 green).

**Checkpoint**: `cmake --preset=ci-rocky && cmake --build build` builds with
the
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

### Tests for User Story 3 (write first, observe failure) The commit also carries that declaration, which `T022` held back. That pass also fills `TopdownTable::rows`: the verdict of each row comes from the table once it is parsed, and the claim that the tree stays pure (FR-002) rests on a resolution reading that parsed table, which `T024` measures rather than assumes. That pass re-parents one row of `test/source/detail/topdown_fixture.hpp`: `tma_memory_bandwidth` stood under a row of its own level, which no vendored table does, measured over 98 group tokens of `arch/x86/skylakex/` and 111 of `arch/x86/sapphirerapids/` that stem to a row of the same table, every one of them one level above. The fixture followed the tables and the shape the contract states, because no part of the library may bend the shape to a scripted table.
- [x] T025 [P] [US1] In `test/source/harness_topdown_option_test.cpp`, a new
  file, write the option cases on the re-exec shape of
  `test/source/harness_hunt_cli_test.cpp`: `theBareWordAsksLevelOne` and
  `anAttachedLevelAsksThatLevel` read the level `detail::scanCommandLine`
  settles at `source/harness/detail/options.hpp:410` (FR-019),
  `aSpaceSeparateWordIsNoLevel` holds level 1 with the word `2` left positional
  (spec D-5), and each of `aBadLevelAnswersOneLine`, `aFileLineRefusesTheRow`
  and `noEnvironmentNameExists` runs the entry point once and checks the stream,
  the status and that no benchmark ran: `--topdown: expected 1 to 6, got %s`
  verbatim for `0`, `7`, `x` and the empty text at status 2, the origin-prefixed
  `the option 'topdown' cannot stand in an option file`, and no
  `SPEEDGUN_TOPDOWN` reaching the run (FR-019, FR-050). FR-050 and SC-05 count
  four refusals: the three values that carry a level and the rule that names no
  known node, whose line T056 drives. The empty text and the option file line
  reach the same arm under spec D-5, and
  `specs/042-topdown/contracts/topdown-option.md:70-82` states all five, so this
  case tests five lines and FR-050 counts four. Add the new file's
  `add_executable` plus `add_test` block to `test/CMakeLists.txt` in the same
  commit, on the shape of `test/CMakeLists.txt:1579-1583` with the
  `source/harness` include directory. This case lands with T033 in one commit
  (FR-048). The unit registers without `SKIP_RETURN_CODE` and with no row in `test/tier_labels.sh`: it answers 0 or 1, and `test_tier_labels` passes with it registered. Five arm mutations ran, one per arm, and each prints the case it breaks: the bare word asks level 2, the range admits 7, the attached text is dropped, `applySourceValue` gains an arm for the row, and the row's environment flag turns on. The environment flag mutation moves the table unit as well, which pins that flag in two readers. The tree was restored byte for byte after each cycle.
- [x] T026 [P] [US1] In `test/source/harness_cli_table_test.cpp`, move the table
  oracle to the 43rd row: `kContract` holds 43 rows and ends
  `{"topdown", kOptional, 350}` (`:244`); `theTableHoldsFortyTwoRows` becomes
  `theTableHoldsFortyThreeRows` with its entry in the check-map comment at `:55`
  and its fault text at `:740` reading 43; `inNewBlock` takes the block 350 to
  359 beside the 340 bounds (`:817`); `theTopdownRowStandsInItsBlock` joins on
  the shape of theLayoutRowStandsInItsBlock (`:1101`); the partition loop stops
  reading the tail index as a hunt row (`:941`, `kRowsBeforeHunt` at `:295`);
  and `theHelpAnswerPrintsOneLinePerRow` reads the new help line
  `--topdown [<value>]` with its text
  `Prints the top-down tree of the running cpu at that level` (`:1397`, FR-019).
  This case lands with T032 in one commit, because that commit is where the live
  table gains its 43rd row (FR-048).

- [x] T027 [US1] In `test/source/harness_topdown_run_test.cpp`, a new file,
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
  `source/harness` include directory. This case lands with T034 in one commit
  (FR-048).
  The four cases stand as scenarios of one re-exec parent, each over its
  own scripted provider, and the registration carries `SKIP_RETURN_CODE 2`
  for a host this build vendors no table for. Each case printed its own fault
  against a tree where the option's field stood with no collection behind it,
  and prints it again under one mutation of that collection.

- [x] T028 [US1] In `test/source/harness_topdown_run_test.cpp`, the new file of
  T027, write the record and strip cases: `theNodeFigureStandsInTheRecord` holds
  each collected figure in `result.topdown` with the availability the merge
  published, `theStripKeepsEveryMetricViewAligned` walks `metricLabels`, each
  row's `metrics`, `metricStats`, `percentiles` and the retained raw columns,
  with and without `--percentiles`, and `theStripChangesNoTimeColumn` holds
  every time column and every row field byte for byte against the same run
  without the option (FR-023, FR-024, FR-032). This case lands with T035 in one
  commit (FR-048).
  The three cases ride the same re-exec parent as the four of T027, one scripted
  run each: the record, then the widths of every metric view with `--percentiles`
  on and off, then the bytes of every row field with no percentile reported. The
  overhead floor, the chunk pair and the pass tag stand outside that byte bar
  because the plan of the run names them, and each case printed its own fault with
  the strip's call taken out.

- [x] T029 [US1] In `test/source/harness_topdown_run_test.cpp`, add
  `anUnsupportedHostCollectsNothingAndStaysItself`: a planted refusal answers no
  node column, no block and an unsupported record with its reason, while the
  run's status equals the same run without the option (FR-040, FR-041; the one
  message line is T080's). This case lands with T036 in one commit (FR-048).
  The scenario runs both arms over one scripted provider and reads the refusal's
  two streams out of files, the way `harness_aggregates_only_test.cpp` reads a
  report, so the claim that nothing prints is read and not assumed. The planted
  table holds four published rows and no directory, so the case shows a refusal
  outranking the rows it carries.

- [x] T030 [US1] In `test/source/harness_topdown_run_test.cpp`, write the block
  cases: `theBlockStandsAfterTheResultTables` holds one block per benchmark
  under its own tables, threaded runs included (FR-025);
  `theBlockStatesTheLevelThePassesAndTheTable` reads
  `topdown: level=1 passes=%zu table=%s` with the pinned reference and the file
  hash (FR-026, FR-015); `theFourLevelOneNamesPrintWithTheirVendorNames` reads
  the four neutral names, their percent and their vendor names in table order
  (FR-027); `eachFigureEqualsWhatTheFixtureScripts` holds the four at the
  scripted figures (SC-03); `theAmdBlockPrintsTheFifthValueAndTheStatement`
  reads `topdown.smt_contention` beside the four and the line that says the four
  add to less than 100 percent without it (FR-028, FR-056);
  `aRunWithNoMetricColumnStillStatesItsPasses` reads the pass line of a
  tree-only run (FR-021); and `theBareWordPrintsATreeAtStatusZero` runs the
  entry point with the bare word, reads a level-1 tree and answers exit status
  0, the fifth case SC-05 names beside its four refusal statuses. This case
  lands with T037 and T038 in one commit (FR-048). The reference line and the file lines a production run prints stand with `T101`, so the block cases pin the print rule over a planted record.
  The block cases drive `detail::printResult` over runs the fixture scripts, so the
  figures, the names and the block's place in the report come off the report the run
  produced. The AMD record and the level-2 record with a gapped child stand built by
  hand, because a planted table answers one availability for every node of its run,
  and `the-deeper-tree-prints-its-indentation-and-its-gap` holds D-12's two-space
  indentation and FR-030's `not available` beside the case list of this task. The
  bare word re-executes this binary through `sg::speedgunMain` over the scripted
  provider, reads the header its own record states and answers status 0, so no case
  depends on which table the host selects. Nine arms, one per case, each printed its
  own claim and left the tree byte-identical.
- [x] T031 [US1] In `test/source/harness_topdown_run_test.cpp`, the case
  `theRunStatesTheCoreWideScopeBeforeTheFirstMeasurement`: a top-down run states
  once on the console, ahead of its first measurement, which way the flag
  settled, whole core or one thread of it, and a run without the option prints
  that line not at all (FR-042). This case lands with T039 in one commit
  (FR-048).

### Implementation for User Story 3
  The case re-executes this binary through `sg::speedgunMain` over two benchmarks,
  because the claim is an order on the console: it reads the whole console, the line
  the child probes from its own `coreWideCoverage()` call, and holds that a run with
  the option states the settled reading once for the process, behind the banner the
  entry point prints and ahead of the row of the first run that measured. The probe
  is what keeps the case host-independent: one call settles the cached value D-6
  describes, so the probe and the stated line are one reading, and a host whose cpu
  set or sibling lists stand unreadable answers nothing, which the case reads as no
  line at all. The variant without the option states neither the line nor a block.
  Four arms on `source/harness/cli.cpp` moved the line into the walk, moved it behind
  the batch, swapped the two readings and dropped the option guard, and each printed
  its own claim.

- [x] T032 [US1] In `source/harness/detail/options.hpp`, add
  `kTopdownOptionValue = 350` after `kGroupLayoutOptionValue` at `:97`; in
  `source/harness/options.cpp`, append the `topdown` row after `hunt-baseline`
  at `:765` inside the array that closes at `:771`, with
  `OptionArgument::OPTIONAL`, `fromEnvironment = false` and the owner column
  reading `specs/042`, where the row letter column holds a switch code and never
  a short option because the scan's option string carries none
  (`source/harness/options.cpp:478-486`); in `docs/pages/harness.md` add the
  option row after the `hunt-baseline` row at `:709`, the last of the rows
  :668-709 of the table whose header stands at `:666`, the environment column
  left empty the way the `hunt` row leaves it at `:699`, so that table answers
  43 rows to `thePageAndTheTableAgreeInBothDirections` at
  `test/source/harness_cli_table_test.cpp:1710`; and in `README.md` add the
  option row after the `--hunt-baseline` row at `:997` of the table opened at
  `:954`, spelled `--topdown[=<level>]` there, paid by the one named trim of the
  split blank at `README.md:946`, so the page holds the `PAGE_LINE_CAP` of 1004
  that `python3 tools/docs/example_drift.py .` reads (FR-019, FR-044; turns the
  43-row oracle of T026 green). This row, `T026`, `T033` and `T025` land in one commit: `source/harness/options.cpp` reads an option name from a row, so the arm of `T033` answers nothing before the row stands, and the row moves the table oracle to 43 rows, asks the guide page for its row and asks `README.md` for a row and a trim inside the 1004-line page budget. The row spells `--topdown`, the exact long option the drift gate compares every row against, and the trim is the blank line that split the quick-reference names table, which now stands as one.
- [x] T033 [US1] In `source/harness/detail/internal.hpp`, give `RunOptions` the
  one member `std::optional<std::uint64_t> topdownLevel` beside `graphPath` at
  `:701`, an engaged value being the option standing, so no second presence flag
  joins the record; in `source/harness/options.cpp`, give the row its own arm
  beside `case kHuntOption:` at `:920`, where a null `optarg` is level 1 and any
  other text reads 1 to 6 or answers the one refusal line at status 2, and add
  no arm to `applySourceValue`, so a file line keeps the `default:` refusal at
  `:1517-1528` (FR-019, FR-050, spec D-5; turns T025 green).

- [x] T034 [US1] In `source/harness/runner.cpp`, call
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
  FR-021, FR-023; turns T027 green).
  Four arms each printed their own fault under one mutation: the two sets
  standing as alternatives, the node columns at the head of the vector, a
  refused row taking a column, and the new source's limit reaching no plan.
  The tree came back byte for byte after each.

- [x] T035 [US1] In `source/harness/detail/internal.hpp`, declare
  `stripTopdownColumns` with the `\pre` that every stripped position names a
  column of that run and the `\post` that the labels, every row's metrics and
  every row's statistics and percentiles hold equal length; in
  `source/harness/runner.cpp`, define it and call it once at the end of the
  result-finishing stage, past both merge paths at `:3098` and `:3112` and after
  the aggregate row at `:3185`, so each node figure moves into `result.topdown`
  and its position leaves every metric view, the two bounds held by `SG_REQUIRE`
  (FR-023, FR-032, PC-9; turns T028 green).
  The call stands after the aggregate row and ahead of the fit note, and both
  bounds ride `SG_REQUIRE`: the post bound printed the arm that left a percentile
  slot standing, and the arm that stripped one slot early printed the label and the
  content faults of three cases. `detail/internal.hpp` stands outside the doc
  gate's `INPUT`, so the pair gate counts neither the tags nor these checks.

- [x] T036 [US1] In `source/harness/runner.cpp`, take the refusal side of the
  `std::expected` answer at T034's call: append no column, record the reason and
  no node, print nothing and change no status, leaving the one line to
  `source/harness/cli.cpp` (FR-040, FR-041; turns T029 green).
  The refusal branch stands beside the branch that collects, and it stores the
  refusal's `detail` in `TopdownRecord::reason` with no sentence composed in the
  harness. The reason text for a kind whose detail stands empty arrives when T079
  implements `message()`, which replaces that store with the library's line and
  pins the whole line in its own case, so no reader mistakes an empty `reason` in a
  mid-feature document for a defect: FR-033 stands satisfied at the end of the
  feature, because T079 and T080 land in this branch. Five arms each printed their
  own claim, and no arm left the refusal printing or moving a status.

- [x] T037 [US1] In `source/harness/report.cpp`, add
  `printTopdownBlock(const BenchmarkResult&)` in the anonymous namespace that
  closes at `:334`, beside `printPassDisclosure` at `:301`, printing with
  `std::printf` in the house field shape of `:312`, and call it from
  `printResult` between `printPassDisclosure(result)` at `:651` and the jitter
  note at `:657`, so the block stands after the tables and every later line
  keeps its bytes (FR-025, FR-026, FR-027, FR-028; turns every case of T030
  green except the pass line, which T038 turns in the same commit).
  `printTopdownBlock` stands in the anonymous namespace that closes above it, and its
  call stands between that disclosure and the jitter note, so a threaded run reaches
  the block on the path a single-context run already took. Every line carries the
  `topdown:` tag, the header holds the three fields FR-026 fixes, the source lines
  follow one per entry the record states, each node line carries two spaces per level
  below one, and a record that states no support prints no block, because the line an
  unsupported host owes the reader belongs to `source/harness/cli.cpp` at T080. The
  statement of FR-028 keys on the node named `topdown.smt_contention`, and the reason
  the record carries no vendor flag stands in the comment beside that branch.

- [x] T038 [US1] In `source/harness/report.cpp`, let the guard of
  `printPassDisclosure` at `:303-307` read `&& !result.topdown.has_value()`, so
  a `--topdown` run with no `--metric` column still states the passes its one
  plan produced, one branch and no other change (FR-021; turns
  `aRunWithNoMetricColumnStillStatesItsPasses` of T030 green in the commit of
  T037).
  The guard reads the added clause, so a run that collected the tree and no metric
  column states its passes above its block. One case of T030 holds both halves, and
  the arm that restores the guard as it stood moves that case alone.

- [x] T039 [US1] In `source/harness/cli.cpp`, print that one scope line once
  from `speedgunMain`, ahead of the walk that calls `runner.run` at
  `source/harness/cli.cpp:1544`, naming the settled reading `coreWideCoverage()`
  T017 publishes and printing nothing when it answers nothing (FR-042, spec D-6;
  turns T031 green).

**Checkpoint**:
`ctest --test-dir build`
`-R 'harness_topdown_option_test|harness_topdown_run_test'`
passes. US1 stands on its
own: the option, one plan, one block, one record.

---

## Phase 4: User Story 2 - Drill down to the level I ask for (Priority: P2)

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

### Tests for User Story 4 (write first, observe failure)
  The line reads `topdown: core-wide coverage=whole_core` and
  `topdown: core-wide coverage=one_thread`, the tag the block already carries and the
  enumerator's own spelling, so a reader greps one tag and a value matches the enum
  and the document's snake case. The call stands ahead of the walk, and the comment
  beside it states that the query is the one reader of the flag: that call settles the
  cached value, the conditional over the constant folds to the same value when a table
  resolves, and the reading is taken from the mask this process holds before any
  narrowing a live probe applies inside the walk (FR-042, FR-043, spec D-6). The line as landed reads `topdown: core-wide coverage=whole_core` or `topdown: core-wide coverage=one_thread`, with no line for a reading that stands absent, and the page task that documents the console carries those words as they stand.
- [x] T040 [US2] In `test/source/counters_topdown_select_test.cpp`, add the
  level-token cases of FR-014: rows grouped `TopdownL1` to `TopdownL6` answer
  levels 1 to 6, rows grouped `PipelineL1` and `PipelineL2` answer 1 and 2, and
  a row whose group carries no level token among its `;` tokens, the AMD
  `total_dispatch_slots` helper, yields no node and no refusal (FR-014, research
  D-02, SC-01). This case lands with T047 in one commit (FR-048). The case prints green against the state it starts from, so it pins the rule `T047` records rather than turning new code green.
- [x] T041 [US2] In `test/source/counters_topdown_select_test.cpp`, add the
  multi-level selection case over the fixture: every node below level 1 names
  the parent its own `_group` token states, the Intel `tma_<name>_group` and the
  AMD `<name>_group` spellings both resolve, and a level-2 row whose stem names
  no row of the directory yields no node and no invented parent (FR-013, SC-02).
  This case lands with T047 in one commit (FR-048). The case prints green against the state it starts from, so it pins the rule `T047` records rather than turning new code green.
- [x] T042 [US2] In `test/source/counters_topdown_select_test.cpp`, add the
  clamp case: over the fixture table whose deepest level is 4, an asked level of
  6 answers `TopdownTree::depth` 4 and holds no node below level 4, and an asked
  level of 4 answers the same node set (FR-008, FR-009, FR-020). This case lands
  with T048 in one commit (FR-048). The case prints green against the state it starts from, so it pins the rule `T048` records rather than turning new code green.
- [x] T043 [US2] In `test/source/counters_topdown_select_test.cpp`, add
  `theAmdLevelTwoRowsStandRowForRow`: name the eight level-2 nodes of each AMD
  pipeline table the fixture T009 wrote, `backend_bound_cpu` of Zen 4 against
  `backend_bound_by_cpu` of Zen 5 with the seven that differ the same way, each
  under its own level-1 parent (FR-013, FR-014, FR-052, spec D-2). Phase 2 wrote
  these rows and `levelTwoNamesDifferBetweenTheTwoAmdTables` already names the
  two tables, so this case adds the row-for-row pairing and the parent of each
  row. It lands with T047 in one commit (FR-048). The case prints green against the state it starts from, so it pins the rule `T047` records rather than turning new code green.
- [x] T044 [US2] In `test/source/harness_topdown_run_test.cpp`, add the
  multi-level run over the fixture: at level 2 and at level 3 the collected set
  holds every node of that level and of every level above, the block states the
  level shown and indents each deeper node under its own parent with its vendor
  name beside it, one plan covers the set and states its pass count, and a run
  asking 6 against the depth-4 fixture records `askedLevel` 6 with `level` 4 and
  prints `topdown: the table holds no level deeper than 4` (FR-018, FR-020,
  FR-021, FR-023, FR-026, FR-027). This case lands with T049, T050 and T051 in
  one commit (FR-048). The collection, the parent chain, the two levels on the record and the pass count print green at the base, and the case's own red is the line naming the deepest level the table holds.
- [x] T045 [US2] In `test/source/harness_topdown_run_test.cpp`, add SC-03's
  first tolerance: a run carrying a node's formula as a `--metric` column and
  asking that node's level reports node and column within 0.05 percentage point,
  keeps the column it reported before the option, and prints that row's
  threshold text unchanged with no verdict (FR-017, FR-023, FR-029, SC-03). This
  case lands with T049, T050 and T051 in one commit (FR-048). Both FR-023 claims print green at the base, and the case's own red is the node line carrying no threshold text.
- [x] T046 [US2] In `test/source/harness_topdown_run_test.cpp`, add SC-03's
  second tolerance over the AMD fixture tree with the scripted source: each of
  the five level-1 values answers its fixture figure within 0.05 percentage
  point and the five sum to 100 percent within 0.1 percentage point (FR-011,
  FR-056, SC-03, spec D-3). This case lands with T049, T050 and T051 in one
  commit, the three code changes whose run it reads (FR-048).

### Implementation for User Story 4 This case could not be authored as written: the runner handed the selection the host identity, so a planted Zen table reached it under Intel rules and `topdown.smt_contention` fell to the rule at `topdown_select.cpp:412`, which is right for the identity it was given. `RunOptions::topdownIdentity`, disengaged for production, is the seam that lets a run hand the selection its identity, and the five level-1 figures now close 100 through a measured run over leaves scripted inside this unit.
- [x] T047 [US2] In `source/counters/linux_pmu/topdown_select.cpp`, read level
  and parent from the group text: split on `;`, take the level from the one
  token matching `TopdownL([1-6])` or `PipelineL([12])`, take the parent from
  the remaining `*_group` token whose stem names another row of the directory,
  and hold no node for a row with no level token (FR-013, FR-014, research D-02;
  turns T040, T041 and T043 green). No code change landed with these cases. The level token of each group spelling, the parent a deeper row's own `_group` token states over both vendor spellings, and the clamp of an ask deeper than the table arrived with `T024` at `28b5bb6a`, whose ledger text already names all three. Each claim is still proven live: a break is compiled into a copy of `topdown_select.cpp` outside the tree and linked ahead of the archive, and each break prints the case it breaks. Five of the seven print an older Phase 2 line first, because those cases read the same rule over the selected directory.
- [x] T048 [US2] In `source/counters/linux_pmu/topdown_select.cpp`, settle
  `TopdownTree::depth` to the deepest level the selected table holds whatever
  the asked level, and collect only the levels at or below it (FR-008, FR-009,
  FR-020; turns T042 green). No code change landed with it, for the reason `T047` states. One arm of this task, collecting only the levels at or below the depth the answer states, is pinned by `levelDeeperThanTheTableClamps` of Phase 2 rather than by the new case, and a break of that arm prints that older case's claim.
- [x] T049 [US2] In `source/harness/runner.cpp`, append one metric column per
  node of the clamped level and of every level above it after the `--metric`
  loop at `:1315-1326`, hand the whole set to the standing planner, and record
  the asked level and the level shown on the result (FR-018, FR-020, FR-021,
  FR-023, E-17; turns T044 green). The columns of every level at or above the clamped level join the standing plan and the record carries both the asked level and the level shown, so the case reads what the run settled, with no second accounting of its own.
- [x] T050 [US2] In `source/harness/runner.cpp`, move `stripTopdownColumns` to
  drop only the node positions from every metric view of the result, so a user
  metric column whose label equals a node's vendor name keeps its column and its
  value (FR-023, FR-032, E-17; turns T045 green). No behavior change stands with this task: the erasure works by position, the rule arrived with `T035`, and the shared-label clause now stands in the `\post` of `stripTopdownColumns`. Arm A4 makes the erasure take the label's first match instead, and the strip's own postcondition is what fails.
- [x] T051 [US2] In `source/harness/report.cpp`, print each deeper node indented
  under its own parent with its vendor name and its threshold text, print a
  gapped node with no value, and print the one line naming the deepest level the
  table holds when the asked level stands deeper (FR-020, FR-026, FR-027,
  FR-029, FR-030, spec D-9; turns T044, T045 and T046 green, the three cases
  that read a printed node, a printed gap, a printed threshold and the clamp
  line).

**Checkpoint**:
`ctest --test-dir build`
`-R 'counters_topdown_select_test|harness_topdown_run_test'`
passes and SC-03's two tolerances hold over the scripted source.

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

### Tests for User Story 5 (write first, observe failure) The two lines this task adds read `topdown: the table holds no level deeper than 4` under the header of a clamped run, and the node line carries the row's threshold text after its figure.
- [x] T052 [US3] In `test/source/harness_topdown_report_test.cpp`, a new file,
  add `theRecordRoundTrips`: a written tree with its source, its two levels, a
  gapped node with its availability and threshold, and its refused rows reads
  back field for field unchanged through the report reader, the two file entries
  keeping their `name` and `manifest_hash` pair, and an unsupported record comes
  back with `supported` false, its reason and empty arrays (FR-032, FR-033,
  FR-034, SC-07). Add the new file's `add_executable` plus `add_test` block to
  `test/CMakeLists.txt` in the same commit, on the shape of
  `test/CMakeLists.txt:1483-1487` with the `source/harness` include directory.
  This case lands with T065, T066 and T067 in one commit (FR-048). One scenario, `aDocumentWithoutTheKeyLoads`, stood green at the baseline and says so in its own text: it pins that a document without the key loads with the member empty.
- [x] T053 [US3] In `test/source/harness_topdown_report_test.cpp`, add
  `aDocumentWithoutTheKeyLoads` and `aRunWithoutTheOptionWritesNoKey`: a
  document with no `topdown` key loads with the member absent, and a run without
  the option writes no key, so its bytes differ from the same run before this
  feature in the schema version alone (FR-034, FR-035, SC-06). This case lands
  with T065 and T066 in one commit (FR-048).

- [x] T054 [US3] In `test/source/harness_topdown_report_test.cpp`, add
  `theWrittenTreePassesTheSchemaWithTheKeyAfterHunt`: hand the writer's own
  document to the schema checker the way
  `test/source/harness_json_schema_test.cpp:57` does, and read `topdown`
  standing after `hunt` in the entry order with no out-of-order finding (FR-032,
  FR-036). This case lands with T065 and T067 in one commit (FR-048).

- [x] T055 [US3] In `test/source/harness_json_schema_test.cpp`, add the
  planted-finding rows the check at `:111` reads: a `topdown` key written before
  `hunt` answers out of the order the schema states, and an inner key the object
  does not list answers unlisted (FR-032, FR-036). This case lands with T067 in
  one commit (FR-048).

- [x] T056 [US3] In `test/source/harness_topdown_report_test.cpp`, add the gate
  cases: `topdown:backend_bound:higher:abs=5` and
  `topdown:intel.tma_l1_bound:lower:10%` each compare that node's value in both
  reports, while a node the contender lacks, a contender with no `topdown`
  object, and a baseline written without the option each answer one quoted line
  naming the rule, the value, the benchmark and the report at status 2 (FR-037
  to FR-039, FR-050, SC-13). This case lands with T074 in one commit (FR-048). The gate scenario carries a fourth fault document, which is what reads the gap claim, and it is the case that kills a planted gap answering zero.
- [x] T057 [P] [US3] In `test/source/harness_compare_test.cpp`, extend
  `theGrammarFaultsEveryUnreadableCorner` (`:2468`) with `topdown:` naming
  nothing and with `topdown:backend_bound@median:lower:5%` refusing an
  aggregate, and `theGrammarAcceptsTheRemainingShapes` (`:2531`) with the two
  SC-13 rules read with the dotted node name and the threshold whole (FR-037).
  This case lands with T074 in one commit (FR-048).

- [x] T058 [P] [US3] In `test/source/harness_compare_cli_test.cpp`, drive
  through the `oneFaultLine` helper (`:770`) the two exits: a rule naming an
  absent node and a rule against a report with no top-down object each print one
  line, print no table and exit with status 2, the third and fourth of the four
  refusal statuses SC-05 counts (FR-038, FR-039, SC-05). This case lands with
  T074 in one commit (FR-048).

- [x] T059 [P] [US3] In `test/source/harness_cli_table_test.cpp`, move the
  project row of `theVersionBindingsReadTheMovedNumbers` at `:1387` to the
  version `0.29.0` T073 lands, with the check-map entry at `:125` and the prose
  line at `:1376` that quote it, and move the project row of the version block
  of `test/source/views_test.cpp:250` with its block comment at `:242` to the
  same number, the two archive rows having moved with their own Phase 2 commits
  (FR-047, spec D-4). This case lands with T073 in one commit (FR-048). The two blocks had drifted, to `:128` and to `:267`, and each moved with its check-map line, its prose line and its block comment.
- [x] T060 [P] [US3] In `test/source/harness_json_report_test.cpp`, move the
  stamp check of `theDocumentStampsTheMinorThisFeatureRequires` at `:546`, the
  case at `:543`, and its prose line at `:547` from `1.11` to `1.12`, the minor
  T068 stamps and the table at
  `specs/042-topdown/contracts/topdown-report-key.md:118-143` names for this
  move (FR-036, spec D-17). These lines land with T068 in one commit, the writer
  being the code that stamps the minor they read (FR-048).

- [x] T061 [US3] In `test/source/harness_json_schema_test.cpp`, move the stamp
  check of `entryHuntKeysPairAtStampedMinor` at `:186`, the case at `:158`, and
  the two prose lines at `:157` and `:187` from `1.11` to `1.12`, the minor T068
  stamps and the table at
  `specs/042-topdown/contracts/topdown-report-key.md:118-143` names for this
  move (FR-036, spec D-17). These lines land with T068 in one commit, the writer
  being the code that stamps the minor they read (FR-048).

- [x] T062 [P] [US3] In `test/source/harness_json_load_test.cpp`, move the first
  `Mutation` entry at `:829` of the list opened at `:828`, which
  `brokenDocumentsRefuseWithPointers` reads at `:1080`, and move both
  later-minor plants to `1.13` so they keep gating a later minor: the stamp
  `versionRuleGatesTheReader` writes at `:107`, the case at `:92`, and the
  rewrite at `:1199` with its check at `:1202` in
  `theHeaderFieldsAndTheStoredWarningsLoad`, the case at `:1172` from `1.11` to
  `1.12`, the minor T068 stamps and the table at
  `specs/042-topdown/contracts/topdown-report-key.md:118-143` names for this
  move (FR-036, spec D-17). These lines land with T068 in one commit, the writer
  being the code that stamps the minor they read (FR-048).

- [x] T063 [P] [US3] In `test/source/harness_hunt_json_test.cpp`, move the
  `schemaConformance` stamp check at `:1537`, the case at `:1534`, and its prose
  line at `:1538` from `1.11` to `1.12`, the minor T068 stamps and the table at
  `specs/042-topdown/contracts/topdown-report-key.md:118-143` names for this
  move (FR-036, spec D-17). These lines land with T068 in one commit, the writer
  being the code that stamps the minor they read (FR-048).

- [x] T064 [P] [US3] In `test/source/harness_noise_report_test.cpp`, move the
  stamp check of `theWrittenTracerDocumentPassesTheSchema` at `:426`, the case
  at `:415`, the file-scope `kDocumentFixture` bytes at `:456-461`, which
  `theTracerLessDocumentMatchesTheFixtureBytes` compares at `:463`, and the
  three prose lines at `:413`, `:427` and `:455` from `1.11` to `1.12`, the
  minor T068 stamps and the table at
  `specs/042-topdown/contracts/topdown-report-key.md:118-143` names for this
  move (FR-036, spec D-17). These lines land with T068 in one commit, the writer
  being the code that stamps the minor they read (FR-048).

### Implementation for User Story 5

- [x] T065 [US3] In `source/harness/json_report.cpp`, write the object after the
  `hunt` key block that closes at `:718`, inside the entry writer that closes at
  `:719`, with the nine keys
  `specs/042-topdown/contracts/topdown-report-key.md:33-64` lists in their
  `x-order`: `supported`, `reason`, `reference`, `directory`, `files`,
  `asked_level`, `level`, `nodes` in parent-before-child order and `refused`,
  each item object closing itself with no key the schema lacks; write `files` as
  one object per contributing record with `name` and `manifest_hash`, the pair
  the carrier holds as `name` and `manifestHash`; write the node value as a JSON
  number with no exponent and `null` for a gap node; and write the availability
  as the exact JSON word of `kAvailabilityWords` at
  `source/harness/detail/json_report.hpp:57-64` (FR-032, FR-033, FR-056; turns
  T052 and T054 green). This set lands as one commit with `T066`, `T067`, `T068` and the eleven pin sites, because the writer, the schema, the reader and the pins cannot be green apart.
- [x] T066 [US3] In `source/harness/json_load.cpp`, declare the loaded record
  and node types beside the hunt load helpers and define the reader in the same
  translation unit, taking the key through `optionalField` at `:85-93` the way
  `hunt` does at `:2342-2349`, so an absent key leaves the member disengaged
  (FR-035); read the nine keys in the order the schema states, `files` items as
  name and hash pairs and `nodes` items with a `std::optional` value that
  answers nothing for `null`; answer `expected an object, found <kind>` for a
  key of the wrong kind as `wrongKind` does at `:52-58`, and keep the checker's
  own refusals intact: the missing key at `:2526-2529`, the unlisted key at
  `:2534-2542` and the out-of-order key at `:2545-2556` (FR-034, FR-050; turns
  T052 and T053 green).

- [x] T067 [US3] In `docs/schema/report-v1.json`, add `topdown` after `hunt` in
  the entry `x-order` that closes at `:407`, add the `topdown` property after
  the `hunt` object that closes at `:902` inside the `properties` block opened
  at `:430`, and leave the entry `required` list at `:408-427` and the
  `additionalProperties` at `:429` as they stand, so the key stays optional for
  every stored document; type the object exactly as
  `specs/042-topdown/contracts/topdown-report-key.md:33-64` spells it, each item
  object closed with `additionalProperties: false` the way the hunt item object
  is at `:890-899`; in `test/source/harness_json_schema_test.cpp`, mirror the
  shape the schema gains (FR-032, FR-036; turns T054 and T055 green).

- [x] T068 [US3] In `source/harness/json_report.cpp`, set `kSchemaVersion` at
  `:43` from `"1.11"` to `"1.12"`, the pair
  `specs/042-topdown/contracts/topdown-report-key.md:118-143` names for a minor
  that adds one object, the value the writer appends at `:992` (spec D-17; turns
  every `stamp_*` case green). Eleven sites across five units moved with the stamp, and the guide page carried an ungated stamp stale at `1.10` beside the `1.11` the code held; `T069` moves it.
- [x] T069 [US3] In `docs/pages/harness.md`, move the figure in the sentence at
  `:1399`, inside the version-rule body at `:1396-1401` under the heading at
  `:1394`, from `1.10` to `1.12` and name the `topdown` object as what the minor
  added; the page reads `1.10` while the writer has stamped `1.11` since
  specs/037, so this move closes a gap that stands today (spec D-17; lands with
  T068 in one commit). The page reads `A run writes 1.12` beside the object the stamp names, and the sentence that named `group` now names the record of the entry.
- [x] T070 [US3] In `docs/pages/harness.md`, add to the null-rule body at
  `:1405-1410`, under the heading at `:1403`, the two top-down facts
  `specs/042-topdown/contracts/topdown-page.md:124` states: an entry written
  without the option carries no `topdown` key at all, and a gap node carries
  `null` in its `value` where a counted node carries a number; leave the
  existing list of keys that stand null otherwise unchanged (FR-033, FR-035;
  lands with T065 in one commit). The null rule gains the one sentence the tree needs: a node whose row the host refuses carries a gap where its figure would stand.
- [x] T071 [US3] In `docs/pages/harness.md`, add the `topdown : <label>`
  alternative to the second line of the `<value>` production at `:1461`, inside
  the grammar fence that runs `:1458-1466`, beside `counter`, `metric` and
  `stat` (FR-037; lands with T074 in one commit). The fence names the production `<node>`. This block wrote `<label>` from an older reading, and the guide page, `contracts/topdown-rule.md` and `specs/026-regression-gate/contracts/rule-parse.md` all carry `<node>`.
- [x] T072 [US3] In `specs/026-regression-gate/contracts/rule-parse.md`, add the
  `topdown : <label>` alternative to the `<value>` production line at `:33`,
  beside `counter`, `metric` and `stat`, the grammar
  `specs/042-topdown/contracts/topdown-rule.md:15-21` settles; in
  `specs/026-regression-gate/contracts/compare-call.md`, name the top-down node
  among the values a report can lack in the fault row at `:54`, the three fault
  lines `specs/042-topdown/contracts/topdown-rule.md:89-94` says ride with the
  prefix (FR-037; lands with T074 in one commit). No gate reads the grammar fence: a planted spelling the parser refuses left the prose gate, codespell and the thirteen docs checks clean, so the line stands on the two compare units and on this contract. The contract also called that name a label while carrying the `<node>` production, and one word moved.
- [x] T073 [US3] In `CMakeLists.txt`, move `VERSION 0.28.0` at `:45` to `0.29.0`
  with the comment at `:40-44` naming the record and the requirement that moves
  it, the counters `SOVERSION` at `:110` and the harness `SOVERSION` at `:375`
  having moved with their own commits in Phase 2 (spec D-4, spec D-17; turns
  T059 green). The comment names the record and the requirement that moves the minor. Both archive numbers already stood at 5 and 20 from their Phase 2 commits, so no archive line moved here.
- [x] T074 [US3] In `source/harness/compare.cpp`, add `"topdown:"` to
  `kLabelPrefixes` at `:55-56`, add `&& prefix != "topdown:"` to
  `statisticsValue` at `:400` so a written aggregate on a node faults, and add
  one `topdown:` arm between the `metric:` arm of the resolver at `:611-627` and
  the `stat:` fall-through at `:629-633`: re-join the `topdown.` head, select
  the node of `result.topdown` by neutral name and answer its value with no
  statistics record, a gap node answering nothing the way the guard at
  `:619-620` does and never an uncountable value; answer the fault text of
  `ruleFault` at `:80-88` for `topdown:` naming nothing and for a written
  aggregate, and the text of `missingValueFault` at `:1416-1424` naming rule,
  value, benchmark and report for a node the contender lacks, a contender with
  no top-down object and a baseline written without the option, the compare mode
  owning the `error: ` line and status 2 at `:1818-1819` (FR-037 to FR-040,
  FR-050; turns T056, T057 and T058 green).

**Checkpoint**:
`ctest --test-dir build`
`-R 'harness_topdown_report_test|harness_json_schema_test'`
passes, and a document written
without the option differs from the pre-feature
bytes in the schema version alone.

---

## Phase 6: User Story 4 - Understand a host that has no source (Priority: P3)

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

### Tests for User Story 6 (write first, observe failure)

- [x] T075 [P] [US4] In `test/source/counters_topdown_select_test.cpp`, add the
  refusal cases: `unsupportedAnswersQuoteTheIdentity` reads the five refusal
  texts of `specs/042-topdown/contracts/topdown-selection.md:81-87` verbatim
  over the identity seam at `source/counters/detail/pmu.hpp:99-100` and `:112`,
  one synthetic `CpuIdentity` per kind, each text quoting the vendor, the family
  and model or the `MIDR`, and the reason; `aHybridDirectoryRefusesByItsRows`
  drives `HYBRID` from the fixture directory whose event rows carry the two
  scopes and no name list; `aOneScopeDirectorySelectsNormally` holds that the
  fixture directory with one scope answers a tree; and
  `noLevelRowRefusesBeforeTheScopeRule` plants a directory that carries both
  scopes and no level row, the shape `pantherlake` answers at
  `external/pmu-events/arch/x86/mapfile.csv:29` with 606 event rows and no level
  row, and reads `NO_TOPDOWN_ROWS`. Five vendored directories publish both core
  types and four of them carry level rows, so the scope rule and the no-rows
  rule each stand on their own. SC-04 counts four of these identities: the AMD
  part with no top-down row, the Intel hybrid identity, the Arm `MIDR` and an
  unknown vendor (FR-004 to FR-007, FR-040, spec D-13, spec D-14, SC-04; red at
  `754867de`). This case lands with T079 in one commit (FR-048). The same commit publishes `TopdownUnsupported::message()` in `include/speedgun-ng/counters_topdown.hpp` with the `\post` the pairing gate pairs with its check, the publication `T022` held back.
- [x] T076 [P] [US4] In `test/source/harness_topdown_run_test.cpp`, add
  `anUnsupportedHostPrintsOneLineAndKeepsTheStatus`: an unsupported selection
  prints one message on standard output naming vendor, identity and reason, the
  run's exit status equals the same run without the option, and the record says
  top-down is unsupported, the console half of the four identities SC-04 counts
  (FR-040, FR-041, SC-04). This case lands with T080 in one commit (FR-048).

- [x] T077 [US4] In `test/source/harness_topdown_run_test.cpp`, add
  `refusedRowsPrintAtTheLevelShown`: the block lists each refused row once at
  the level it shows, with its UPPER SNAKE kind and its reason unchanged, and
  prints no node for it (FR-016, FR-031). This case lands with T081 in one
  commit (FR-048).

- [x] T078 [P] [US4] In `test/source/harness_topdown_report_test.cpp`, add
  `theRecordCarriesRefusedRowsAndNamesAnUnsupportedHost`: the `refused` array
  carries name, kind and reason per row, and an unsupported host writes
  `supported` false with the reason and two empty arrays, the record half of the
  four identities SC-04 counts (FR-033, FR-034, SC-04). This case lands with
  T082 in one commit (FR-048).

### Implementation for User Story 6

- [x] T079 [P] [US4] In `source/counters/linux_pmu/topdown_select.cpp`, answer
  the five refusals: `UNKNOWN_VENDOR`, `NO_DIRECTORY`, `HYBRID`, `ARM` and
  `NO_TOPDOWN_ROWS`, with `message()` one line apiece as
  `specs/042-topdown/contracts/topdown-selection.md:81-87` spells them. The
  hybrid answer reads the scope text of the selected directory's own event rows,
  the `Unit` text `PmuTableEntry::unit` holds at
  `source/counters/detail/pmu.hpp:45`, and the no-rows answer stands ahead of
  it, so a directory that publishes two scopes and no level row answers
  `NO_TOPDOWN_ROWS` (FR-004 to FR-007, FR-040, spec D-13, spec D-14; turns T075
  green). No name list stands anywhere: the hybrid answer reads the scope text the selected directory's own event rows carry, and the no-rows answer stands ahead of it. The record's `reason` carries this message, so an identity that reads no directory names the vendor and the identity in the document (FR-033).
- [x] T080 [P] [US4] In `source/harness/cli.cpp`, print that one message when
  the selection answers unsupported and change no exit status on that path
  (FR-040, FR-041; turns T076 green). `detail::printTopdownRefusal` in `cli.cpp` prints that message once ahead of the walk and changes no status. It repeats the selection the runner repeats, one pure pass for each process that asks the option; the band field that drops that pass waits for the coverage phase, where its cost gets measured.
- [x] T081 [P] [US4] In `source/harness/report.cpp`, print the refused rows in
  the block, one line each at the level shown with kind and reason (FR-016,
  FR-031; turns T077 green). The refused rows print behind the nodes, and the level the block states in its header owns their indent, because `TopdownRefusal` carries no level and the contract closes the item to three keys.
- [x] T082 [P] [US4] In `source/harness/json_report.cpp`, write the refused
  entries and the unsupported state of the top-down object (FR-033, spec D-14;
  turns T078 green). This pair is a pin. `T053` and `T065` already wrote the refused entries and the unsupported state, so `T078` arrived green and no code moved here; the Phase 4 precedent of `T047` and `T048` states the shape. A planted break that drops the `kind` key kills the case.
- [x] T083 [P] [US4] Land the pending-row rule of FR-057 with no unit test,
  since a part with no host is a statement about hardware: one pending row per
  part with no host (`PERF_METRICS` parts, Intel hybrid parts, Arm parts) in the
  acceptance table of `specs/042-topdown/quickstart.md`, each naming no hardware
  claim, the same rows riding the acceptance record of the pull-request body
  (FR-057, SC-11, SC-12, spec D-16).

**Checkpoint**:
`ctest --test-dir build`
`-R 'counters_topdown_select_test|harness_topdown_report_test'`
passes with the refusal fixtures planted, and no exit status moves on an
unsupported host.

---

## Phase 7: User Story 5 - Copy a working block from the page (Priority: P3)

**Goal**: a reader opens the main page, reads one short section about the tree,
copies the block and builds a program that prints level 1 or the unsupported
message, while the guide page carries the longer explanation.

**Independent Test**: spec US5: the drift gate compares the page block with the
region in the example file, the option row with the option table, and the
section list with the page, and the run gate executes the example and fails when
a registered benchmark prints no row. Here that is
`python3 tools/docs/example_drift.py .` and `bash test/example_run.sh build`. Four pending rows stand in the acceptance table, naming AMD level 2, `PERF_METRICS` parts, Intel hybrid parts and Arm parts, and the sentence below the table now reads `those rows`.
- [x] T084 [US5] Write `example/docs/topdown.cpp`, a new file: read the host
  identity, print the level-1 values on a host with a source and the unsupported
  message on a host without one, bound the printed body between
  `// sg-example:begin topdown` and `// sg-example:end topdown`, and register
  one benchmark with a `SG_BENCHMARK` line so the run gate of
  `test/example_run.sh` finds a non-empty `--list` and one row per registered
  benchmark. Hold the region at the 15 lines the README section of T087 budgets,
  the shape `contracts/topdown-page.md:19-34` fixes: the section stands at 21
  lines, four inside the `SECTION_LINE_CAP` of 25 that
  `tools/docs/example_drift.py:241` sets, the neighbor
  `Hardware counter metrics` holding the same shape at 25 (FR-045, FR-041). This
  file, its registration, its two lists in `tools/docs/example_drift.py` and the
  README block that quotes it land in one commit (FR-048). The region takes the identity as its argument, because `TopdownSelection::table` carries no default member initializer and a three-member call will not fit 80 columns, so `main` passes `cpuIdentity()` and the guide names that read.
- [x] T085 [US5] Register the example in `example/CMakeLists.txt`:
  `add_docs_example(topdown)` after `add_docs_example(report_views)` at `:82`,
  which builds `docs_topdown` and hangs `run_docs_topdown` on `run-examples` for
  `test/example_run.sh` (FR-045). Same commit as the new file of T084.

- [x] T086 [US5] Add `"Top-down analysis"` to `SECTIONS` at
  `tools/docs/example_drift.py:33` and its owner key
  `"Top-down analysis": ("topdown.cpp",)` to `SECTION_OWNERS` at `:73`, so the
  drift gate demands the page section, its owner file and the region T084 wrote;
  the two lists move in one change, the one that also writes the README section,
  because the tool raises `KeyError` at `tools/docs/example_drift.py:683` when
  one moves without the other and reads `no '### Top-down analysis' section`
  until the page carries it (FR-044, spec Constraints). Same commit as T084,
  T085 and T087. `SECTIONS` and `SECTION_OWNERS` move together: the tool reads `KeyError` with the owners unmoved and `no '### Top-down analysis' section` with the page unwritten, which is why one commit carries both lists and the page.
- [x] T087 [US5] Write the 34th capability section of `README.md` under
  `## How you do it` between `### Multi-pass collection` at `:326` and
  `### Skips` at `:350`: heading, one purpose sentence, the
  `<!-- sg-example: example/docs/topdown.cpp#topdown -->` marker naming the new
  file of T084, the 15-line region body, one result sentence, six marker lines
  and the body making a 21-line section inside the `SECTION_LINE_CAP` of 25 at
  `tools/docs/example_drift.py:241`, four lines inside the cap the plan's
  25-line budget leaves, paid by 21 named trims, the 20 blank lines before the
  headings at `README.md` lines 410, 428, 446, 468, 492, 513, 537, 562, 578,
  601, 620, 635, 657, 675, 693, 740, 763, 786, 811 and 834, and the blank
  between purpose and marker at `README.md:390`, so the page holds its
  `PAGE_LINE_CAP` of 1004 that `tools/docs/example_drift.py:240` reads and the
  quoted body compiles alone under `test/snippet-compile/run.sh`. State in the
  result sentence that the block comes from the acceptance host run recorded in
  `specs/042-topdown/quickstart.md`, and that `test/example_run.sh` proves the
  program builds and prints its row, the figures being the acceptance run and
  not a promise about every host (FR-044, spec D-15). Same commit as T084, T085
  and T086. The page moves from 1004 lines to 1004. The section adds 21 inside the 25 cap, two quick-reference rows join the table, two `## Planned work` rows naming shipped output give place to one row naming the four pending tables, and 22 blank lines carry the rest.
- [x] T088 [US5] Add the two quick-reference name rows `topdownTree` and
  `TopdownTree` to `README.md` at the head of the graph block, above the
  `GraphRecord` row that stands at `README.md:947` at the base hash and at
  `:946` once T032's trim and option row land (2 lines), paid by the two
  remaining trims, the `## Planned work` rows at `README.md:1001` and `:1002`,
  so the page again measures 1004 lines and the drift gate finds no
  `long option '--topdown' has no option row`, that row having landed with T032
  (FR-044, spec D-15). `counters_topdown.hpp` already stood ahead of `views.hpp` in `NAME_SCAN_HEADERS`, so two lists moved here and not three.
- [x] T089 [P] [US5] Write `## Top-down analysis` in `docs/pages/harness.md`
  after the `## Multi-pass collection` section that opens at `:1549`: the names,
  the levels, the source record, the five unsupported reasons and the `topdown:`
  rule form, with the same reading the acceptance record of
  `specs/042-topdown/quickstart.md` carries (FR-046). Four other duties on this
  page moved earlier, each in the commit that made its claim true: the option
  row with T032, the schema-stamp sentence at `:1399` with T068, the null rule
  at `:1403-1410` with T065 and the grammar production at `:1460` with T074.

**Checkpoint**: `python3 tools/docs/example_drift.py .` reports no
finding,
`bash test/example_run.sh build` runs `docs_topdown`, and
`bash test/snippet-compile/run.sh .` compiles the page block alone.

---

## Phase 8: Polish and Cross-Cutting Concerns

**Purpose**: The discipline requirements that span the feature: the memory-bound
case FR-058 names, the gate order, the two static analyzers, the contract gate,
the two sanitizer builds, the coverage evidence, the acceptance record spec
D-16 names, and the one timing bound PC-13 measures. The block the page quotes prints node names. The figures a host prints carry the PC-7 gap on this machine, where `tma_info_thread_clks` reads zero, so the page states no figure and the repair stands at FR-053 with its acceptance row pending.
- [ ] T090 [P] Write `example/memory_walk.cpp`, a new file: one memory-bound
  benchmark whose working set stands above the 19.3 MiB last-level cache of the
  acceptance host, where the largest shipped set stands at 64 KiB of
  `example/docs/throughput.cpp:25`; its verification is the acceptance run of
  `specs/042-topdown/quickstart.md`, which names it at `:59` and `:60` (FR-058,
  SC-10).

- [ ] T091 Register the benchmark of T090 in `example/CMakeLists.txt`:
  `add_example(memory_walk)` beside `add_example(first_benchmark)` at `:33` with
  `target_link_libraries(memory_walk PRIVATE speedgun-ng::harness)` on the shape
  of `:29-34`, the two lines that make the harness examples one speedgun
  executable, so `./build/example/memory_walk` exists for the acceptance run. No
  run gate covers it: `test/example_run.sh:39` walks `example/docs/*.cpp` alone,
  so a program under `example/` is out of its list because of that glob and not
  because of a missing registration (FR-058).

- [ ] T092 Run the gate order of `specs/042-topdown/quickstart.md` end to end,
  `python3 tools/docs/example_drift.py .`, `cmake -P cmake/prose-lint.cmake`,
  `bash test/time_source_gate.sh .`, `cmake -P cmake/spell.cmake`, the
  `cmake/lint.cmake` format pass, `bash test/snippet-compile/run.sh .`,
  `bash test/example_run.sh build` and the full
  `ctest --test-dir build --output-on-failure`, and record each verdict and cost
  in that page, the `.codespellrc` line for the model number `BA` in place. Read
  the commit template too with
  `python3 tools/prose/prose_gate.py --check commit --base origin/master`, the
  step `tools/ci/prepush.sh:203` runs, which answers `CM.FOOTER-APPROVAL` for a
  message with no `Approved-by:` footer, so every commit of this lane carries
  that footer and its `Refs: specs/042-topdown` line (spec D-16, FR-048,
  FR-049).

- [ ] T093 Build the release developer tree that carries the two static
  analyzers: `cmake --preset=ci-ubuntu` then `cmake --build build`, the preset
  that inherits the clang-tidy and cppcheck configurations `ci-rocky` omits, and
  record that neither tool answers a finding in a file this change writes, the
  gate the `lint` job of `.github/workflows/ci.yml` runs and an unoptimized tree
  cannot report.

- [ ] T094 Run the design-by-contract gate the CI matrix runs:
  `cmake --build build/dev -t dbc-gate` at `.github/workflows/ci.yml:1290` then
  `ctest -R dbc_gate_fixtures --output-on-failure --no-tests=error` at `:1297`,
  and record that every `\pre` and `\post` this change documents stands in the
  pair gate's own table, `tools/dbc/dbc_pair_gate.py:17-23` answering for a
  documented precondition the source does not enforce and for an enforced one no
  header documents (FR-051, PC-9).

- [ ] T095 Run the sanitizer build the matrix runs:
  `cmake --preset=ci-sanitize`, which configures address and undefined behaviour
  sanitizers into `build/sanitize`, then
  `cmake --build build/sanitize -j "$(nproc)"` and
  `ctest --test-dir build/sanitize --output-on-failure --no-tests=error`, the
  `sanitize` job of `.github/workflows/ci.yml:147`, and record that no report
  reaches the new units (FR-051).

- [ ] T096 Run the thread-sanitizer build the matrix runs:
  `cmake --preset=ci-tsan`, which configures `-fsanitize=thread` into
  `build/tsan`, then `cmake --build build/tsan -j "$(nproc)"` and
  `ctest --test-dir build/tsan --output-on-failure --no-tests=error`, the `tsan`
  job of `.github/workflows/ci.yml:230`, and record that the one-provider offer
  of T014 answers no data race (FR-051, spec D-21).

- [ ] T097 [P] Run the coverage build to `tools/dbc/coverage_gate.sh`:
  `lines......: 100.0%` at `:45-48`, the branch shortfall inside the standing
  `allowance=21` at `:102`, and no new exclusion marker anywhere in the tree
  (FR-051, SC-09).

- [ ] T098 Write the acceptance record spec D-16 names into
  `specs/042-topdown/quickstart.md` and the pull-request body: the host, the
  kernel, the level-1 values, the refused rows, and the test counts, with the
  two-cpu run
  `taskset -c 0,1 ctest --test-dir build -L quick -j 2 --timeout 120` among them
  and the counters subset on those two cpus beside it (spec D-16, SC-10,
  FR-049). Record with it the three hygiene facts SC-08 states and the seam
  units prove: no unit of this feature opens the kernel event interface, reads a
  clock, or sleeps, `installTestProcRoot` standing a directory in place of `/`
  and `registerMetricHost` of `test/source/detail/metric_fixture.hpp:311-368`
  answering with no event opened, the audit `test/tier_labels.sh` holds at
  `:77-86` and `:91-96` keeping that honest.

- [ ] T099 Record PC-13 in the acceptance record of
  `specs/042-topdown/quickstart.md` as the measured fact it is:
  `test/prose-gate-fixture/run_prose_gate_fixtures.py:84` bounds one whole-tree
  prose-gate run at 10.0 seconds, the entry run measures 8.911 to 8.989 seconds
  against that bound and `ctest --test-dir build -R '^prose_gate_fixtures$'`
  passes, so no bound moves and no owner choice stands open; the 11.559 seconds
  the same timed run answers inside the loaded two-cpu suite is the host load
  condition PC-13 and PC-8 already record, which this feature does not touch
  (PC-13).
- [x] T100 Pin the sampling thread of the cpu-target scenarios of
      `test/source/counters_pmu_test.cpp` to the cpu whose plan they open, so
      the fast read path those scenarios reach answers its precondition once
      the bound repair of `T013` lets them run at all: the scenario takes the
      first cpu of the affinity mask its process holds, narrows to it, opens
      its window there and hands the mask back once the window has been read
      (FR-045, FR-053, spec PC-14).

- [x] T101 [US1] Fill the source a production table reads. In
  `source/counters/linux_pmu/topdown_select.cpp`, let `topdownTable()` copy the
  manifest reference and one `TableFile` per contributing file, each with its
  `name` and `manifestHash`, out of the record `T018` and `T019` put in the
  index and the registry, in the embedder's sorted order, so a table the host
  itself reads reaches the console block and the report document with the
  reference and the digests the vendored `RECORD` lines state (FR-015, FR-033).
  Measured during `T030`'s console cycle: on this host a production answer
  prints `topdown: level=1 passes=0 table=arch/x86/skylakex/` with no reference
  line and no file line, because `TopdownSource` arrives with `directory` filled
  alone, and no other task of this feature copies the other two fields.
  `test/source/counters_topdown_select_test.cpp` gains the case that asks a
  production table over the identity of the host it runs on and holds the
  reference non-empty with one file entry per contributing file, each carrying a
  digest, so the claim cannot ride a planted record alone.

**Checkpoint**: every gate of Principle VIII is green on the pushed state
and
the acceptance table of `specs/042-topdown/quickstart.md` carries no pending
row that claims hardware.

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

One scenario waits a phase, and it is the report-and-console split. The console
block, the record member and the report key are three separate landings: US1
prints the block and fills the record, US3 writes the key that carries the same
nodes, and only Phase 8 measures the two on real hardware. So the one line of
US5 that waits is the result sentence of the main-page section T087 writes: it
states figures from the acceptance record T098 writes, and it names
`test/example_run.sh` as the gate that proves the program builds and prints a
row, never as the source of a figure.

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

- Every test task above lands in the commit of the code task that turns it green
(FR-048).

- Threshold chain: T020 with T007, the counters `SOVERSION` rise at
`CMakeLists.txt:110`, and both version pins with the comments that quote the
number.

- Report carrier: T021 carries the harness `SOVERSION` rise at
`CMakeLists.txt:375`, its comment row and the same two pins, so no commit ships
the member with an unrisen number.

- Project version: T073 with T059, because both readers check the three numbers
out of `CMakeLists.txt` and quote two of them in prose.

- Published query: T016 with T017, whose reading the Phase 3 scope line T039
prints.

- Option row: T032 carries the live row, the row of the guide table, the row of
the README table and the README trim, because the drift gate and
`thePageAndTheTableAgreeInBothDirections` read all three in one pass.

- Registrations: no task stands alone. Each new test file takes its
`add_executable` plus `add_test` block in the commit that writes its first real
cases: T006 and T010 in Phase 2 beside the tier-label row T001, then T025, T027
and T052 in their own phases.

- Block: T030 with T037 and T038, because the printed block turns every case of
that task green except the pass line, and one branch turns that one green.

- Deeper run: T044, T045 and T046 with T049, T050 and T051 in one commit,
because the three cases read the collected set, the strip and the printed block
as one behaviour and no earlier subset of them is green.

- Schema stamp: T068 with T060, T061, T062, T063, T064 and T069, because six of
the eight sites assert the stamp the writer carries and the two later-minor
plants would otherwise pin a minor that has become current.

- Report tree: T065, T066 and T067 with T052, T053, T054, T055 and T070, because
a writer that emits `topdown` against a schema that omits it fails the
`checkReport` call of `test/source/harness_json_schema_test.cpp:57`, and the
round trip compiles against the reader.

- Rule: T074 with T056, T057, T058, T071 and T072, because the fence, the two
contracts and the faults answer one grammar.

- Page set: T084, T085, T086 and T087, because the drift gate reads red between
the section lists, the owner file and the README block.

- Acceptance evidence: T098 and T099 land together, one record with one set of
measured figures.

### Story independence

- **US1** is the feature: the option, one plan, one block, one record. It stands
with no report key, no rule and no page.

- **US2** deepens what US1 collected and reads nothing US3 builds.

- **US3** exports and gates what US1 recorded, and needs no page.

- **US4** answers the hosts US1 cannot serve and touches no page.

- **US5** documents the finished shape and owns its example, with the one result
sentence that waits for the acceptance record of Phase 8.

### Within Each User Story

- Tests first, and they fail on the missing behaviour before the code lands
(FR-048).

- Selection before tree, tree before node columns, columns before block, block
before report key, report key before rule, rule before page.

- Story complete before the next priority starts.

### Parallel Opportunities

- Phase 1 holds one task.

- Phase 2: the eight [P] tasks T002, T003, T006, T009, T012, T013, T018 and T023
each hold a file no other [P] task names, so they run together. T004, T005, T007
and T008 share a test file with the open task of their pair and run in sequence;
`source/counters/linux_pmu/metric_tables.hpp` takes T015 then T016; T020, T021
and T073 share `CMakeLists.txt` and the two version pins, so they run in
sequence across the phases.

- Phase 3: T025 and T026 run together at the opening; inside
`test/source/harness_topdown_run_test.cpp` the five tasks T027, T028, T029, T030
and T031 run in sequence, each with its own code task.

- Phase 4: the four tasks T040, T041, T042 and T043 share
`test/source/counters_topdown_select_test.cpp` and run in sequence, the three
T044, T045 and T046 share `test/source/harness_topdown_run_test.cpp` and run in
sequence, and those three land in one commit with T049, T050 and T051.

- Phase 5: the seven [P] tasks T057, T058, T059, T060, T062, T063 and T064 each
hold one file no other [P] task of the phase names; T061 stands beside T055 in
`test/source/harness_json_schema_test.cpp`, and T052, T053 and T054 run in
sequence in `test/source/harness_topdown_report_test.cpp`.

- Phase 6: the eight [P] tasks T075, T076, T078, T079, T080, T081, T082 and T083
each hold one file; T077 shares the run unit with T076 and runs with its own
code task T081.

- Phase 7: T084 and T089 run together; `README.md` takes T087 then T088 in
sequence, and the page set lands in one commit.

- Phase 8: T090 and T097 run together; `example/CMakeLists.txt` takes T091 after
T090; `specs/042-topdown/quickstart.md` takes T092, T093, T094, T095, T096, T098
and T099 in sequence, each recording into the same page.

### The internals page the drafts name

One draft reads an internals page that records two skips over labels beginning
`topdown.` in place of the strip step, and names that page `_core.md`. No such
page exists: `docs/pages/` holds `about.dox`, `barrier-rule.md`, `building.md`,
`counters-overhead.md`, `dbc-migration.md`, `dbc-overhead.md`, `harness.md`,
`identifier-rename-map.md` and `prepush.md`, and none of them carries the string
`topdown` anywhere. No task corrects a page, and the strip stands as T035 and
T050 write it.

---

## Parallel Example: User Story 1

```bash
# Option surface, two files, side by side:
Task: "T025: Write the option cases in harness_topdown_option_test.cpp"
Task: "T026: Move the 43-row oracle in harness_cli_table_test.cpp"
# Then the live row with its two page rows and its trim, one commit:
Task: "T032: Add the row to options.cpp, README.md and docs/pages/harness.md"
# Then the arm that reads the level, then the run spine:
Task: "T033: Read the level in source/harness/options.cpp"
Task: "T027: Write the collection cases in harness_topdown_run_test.cpp"
Task: "T034: Append the node columns in source/harness/runner.cpp"
# Then the strip, the refusal side, the block and the scope line, in order:
Task: "T035: Define stripTopdownColumns in source/harness/runner.cpp"
Task: "T036: Answer the refusal side in source/harness/runner.cpp"
Task: "T037: Add printTopdownBlock in source/harness/report.cpp"
Task: "T039: Print the core-wide scope line in source/harness/cli.cpp"
```

## Implementation Strategy

### MVP First (User Story 1)

1. Complete Phase 1: the two tier-label rows.
2. Complete Phase 2: the three repairs, the host facts and their query, the
   threshold text, the two carriers and the selection surface.
3. Complete Phase 3: the option, the node columns, the strip and the block.
4. **Stop and validate**: `harness_topdown_option_test` and
   `harness_topdown_run_test` prove the level-1 block and the record with the
   fixture tree and the scripted source. A run without the option prints
   master's bytes.

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

## Notes

- [P] marks a task in a different file with no dependency on an open task. A
commit set listed above overrides the label: the tasks it names land together.

- A run without `--topdown` prints what master prints, so every console task
carries a byte rule beside its new shape.

- The four version numbers move in four commits: the counters `SOVERSION` with
T020, the harness `SOVERSION` with T021, the schema minor with T068 and the
project version with T073. Each move carries both readers,
`theVersionBindingsReadTheMovedNumbers` and the `views_test.cpp` version block,
with the comments that quote the figures, the way FR-047 and spec D-4 require.

- Three guide-page edits and one contract row of another feature move earlier
than the page section, each in the commit that makes its claim true: the schema
figure with T069, the null rule with T065, the grammar production with T071, and
the prefix row of `specs/026-regression-gate/contracts/rule-parse.md` with T072.

- PC-13 records the fixture bound as measured: the entry run stands at 8.911 to
8.989 seconds against the 10.0 second bound and passes on its own, so no bound
moves and no owner choice stands open (T099).

- `source/counters/linux_pmu/counter_limits.cpp`, every limit path and every
file under `.github/` stay untouched (FR-022, PC-10, spec D-16). The Phase 8
gate tasks run the CI commands and change no workflow file.

- Stop at any checkpoint and validate one story on its own.
