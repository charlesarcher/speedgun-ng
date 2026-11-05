---

description: "Task list for specs/046-hunter-topdown"
---

# Tasks: Hunter top-down areas

**Input**: Design documents from `specs/046-hunter-topdown/`

**Prerequisites**: `spec.md` (FR-001 to FR-037), `plan.md`, `research.md`,
`data-model.md`, `contracts/`, `quickstart.md`

**Tests**: Required. FR-029 puts a new unit in the same commit as the code it
covers, and the unit must read red at the base commit before the code lands.

**Base**: `b1dd7670` on branch `046-hunter-topdown`. The base answers three
failing names, all in files the specs/045 lane owns (AP-1).

## Format: `[ID] [P?] [Story] Description`

- **[P]**: different files from every other task in its phase, and no unfinished
  task ahead of it.
- **[USn]**: the user story the task serves, from `spec.md`.
- Every task names an exact file path, and most name the command that proves the
  line. `Prove at the line` runs after the edit.

## Path conventions

- Library sources: `source/harness/`, `source/harness/detail/`,
  `source/counters/linux_pmu/`
- Public headers: `include/speedgun-ng/`
- Units: `test/source/`, registered in `test/CMakeLists.txt`
- Examples: `example/docs/`, registered in `example/CMakeLists.txt`
- Docs: `README.md`, `docs/pages/harness.md`, `docs/schema/report-v1.json`
- Gates: `tools/docs/`, `tools/dbc/`, `tools/ci/`, `test/tier_labels.sh`

## No build-list task

`CMakeLists.txt:279-285` globs `source/harness/*.cpp` and `source/views/*.cpp`
with `CONFIGURE_DEPENDS`, and `:1034-1040` globs the counters tree. Public headers
install by directory rule. A new source or public header therefore needs no
listing line. A new unit needs a `test/CMakeLists.txt` block, and a new example
needs one `add_docs_example` line.

## Commit boundaries

The constitution asks for atomic, bisectable commits. Three boundaries carry
weight here, and the task lines name them.

1. **The ABI commit** carries the moved exported surface and the numbers that
   record it: `RankedEvent::topdownArea` in `include/speedgun-ng/hunt.hpp`, the
   record and the accessor of D-16 in `include/speedgun-ng/counters_topdown.hpp`
   and `source/counters/linux_pmu/topdown_select.cpp`, `CMakeLists.txt:59`,
   `:134` and `:407`, and both version-binding readers. A number move alone makes
   `soVersionNumbers` read `6 20` and fails both readers (FR-028). Both units ride
   it (FR-029), and the two DBC hand gates answer 347 interfaces with 0 gaps at it.
2. **The document commit** carries the schema move to 1.14 with its writer, the
   reader, the schema file and the later-minor plants (FR-020). It shares no file
   with the ABI commit, so either order rebases clean.
3. **The example commit** carries `example/docs/hunt_topdown.cpp`, its recorded
   region and its shape-gate registration together, because an unregistered
   recorded region has no machine (FR-025).

Each commit passes the prose gate in its own range (`--check commit`), and the
format gate before it is pushed. The polish phase re-runs every gate over the
whole head.

## Phase 1: Setup

None. The presets, the globs and the fixture helpers stand at the base, and
`quickstart.md` states the build in two commands.

## Phase 2: Foundational (blocking prerequisites)

- The mapper pair, its seam, its unit skeleton and its registration, then the counters accessor and the ABI commit.

- [X] T001 [P] [P2] Declare the mapper pair in the new internal header `source/harness/detail/hunt_topdown.hpp`, in the shape of `source/harness/detail/hunt_ranks.hpp`: namespace `sg::detail`, the one asked-level constant that holds 6 (FR-001, D-17), and the four tagged declarations `topdownDeepestNode`, `topdownPathOf`, `topdownArea` and `stampTopdownAreas` of `specs/046-hunter-topdown/contracts/area-mapper.md`, each with `\pre`, `\post` and `\invariant` sections (FR-001, FR-006, FR-007, FR-009). No `include/speedgun-ng/` header joins: the mapper is internal, and `tools/dbc/dbc_doc_gate.py:9-12` keeps a nested `detail` namespace out of the counted interfaces (FR-008, D-1). The pair is pure: it takes the three inputs FR-001 names and nothing else, it reads no `RunOptions` and resolves no identity, and its tags state facts about its arguments alone (D-1, D-7). Blocks every story: these four declarations are the only source of the area value, and every story reads one.

- [X] T002 [P2] Add the one seam entry to `source/harness/detail/internal.hpp`: the declaration of `topdownAreaSelection(const RunOptions&) -> sg::counters::TopdownSelection` beside `printTopdownRefusal` at `:1525`, with the tags of the pair. It answers the three fields of one ask: the identity of `RunOptions::topdownIdentity` (`:775`) or `sg::counters::cpuIdentity()`, the table of `RunOptions::topdownTable` (`:759`), and the level of the pair constant. It reads no `RunOptions::topdownLevel` (`:751`), so a `--topdown` level cannot move the area ask (FR-034, D-17). This gatherer is the only declaration of the feature that reads `RunOptions::topdownTable` and `RunOptions::topdownIdentity`: the pair of T001 above takes the address, the tree and the listing alone (FR-001, FR-008). Blocks every story: FR-033 asks a unit to plant the identity and the table in process on the path that feeds the area, and today only `source/harness/cli.cpp:306-312` and `source/harness/runner.cpp:1519-1523` read those two seams, both at the option's own level. Blocks every story again: it resolves the identity once, so the area ask and the level-1 ask of US3 hold one identity and one table (D-17).

- [X] T003 [P2] Add the new translation unit `source/harness/hunt_topdown.cpp`: it includes `detail/hunt_topdown.hpp` and `detail/internal.hpp` at the head of `source/harness/hunt_topdown.cpp` and holds no body yet. This is the build join for the lane, and it edits no build file. The harness target globs `source/harness/*.cpp` and `source/views/*.cpp` with `CONFIGURE_DEPENDS` at `CMakeLists.txt:279-285`, the counters archive globs its tree at `:1035-1039`, and `source/CMakeLists.txt` does not exist at the base, so a new source needs no listing line. New units do need registration, and that is the task two lines below. Blocks every story: no story can run one case until the pair compiles into `speedgun-ng_harness`. Prove at the line: `cmake --preset=ci-rocky && cmake --build build --target speedgun-ng_harness -j "$(nproc)" && ls build/CMakeFiles/speedgun-ng_harness.dir/source/harness/hunt_topdown.cpp.o`.

- [X] T004 [P2] Add the frameworkless unit `test/source/harness_hunt_topdown_test.cpp` with no case body yet: the file header comment, the includes, the isolated-scenario `main` of `test/source/harness_topdown_report_test.cpp` (tail), where `argv[1]` selects one entry of the `Scenario` table, and one `scenario-placeholder` entry so the binary exits 0. Add the six synthetic identities this file needs as its own helpers, copied in the shape of `test/source/counters_topdown_select_test.cpp`: `intelIdentity(6, 85, 4)` at `:159`, `amdIdentity(25, 17)` and `amdIdentity(26, 17)` at `:174`, the unsupported part `amdIdentity(25, 15)`, the Arm `MIDR` record of `:1184-1191` with `kHandedMidr` at `:142`, and `unmatchedIdentity()` at `:150`, because no shared header holds them (`ls test/source/detail`: no identity fixture). Add the two planted forms and the listing form every later case reads: a `RunOptions` with `topdownTable` set from `plantedTable` (`test/source/counters_topdown_select_test.cpp:250-277`) over `topdownIntelRows()` and `kIntelDirectory` of `test/source/detail/topdown_fixture.hpp`, a `RunOptions` with `topdownIdentity` set beside it, and a hand-built `std::vector<sg::counters::TopdownEventGroup>` that holds two names in two groups, the shape the accessor answers for one identity. Blocks every story: FR-029 states the synthetic set that covers the vendors, and every later case of every story drives the mapper through these two records. Prove at the line: no ctest entry exists yet, so prove the compile alone with `cmake --build build --target harness_hunt_topdown_test` after T005 below.

- [X] T005 [P2] Register the new unit in `test/CMakeLists.txt` beside the specs/042 block, that is after `:2258` and ahead of the tier section at `:2260`: `add_executable`, `target_link_libraries` on `speedgun-ng::harness`, `target_include_directories` on `${CMAKE_SOURCE_DIR}/source/harness` so the unit reaches `detail/hunt_topdown.hpp`, `target_compile_features` at `cxx_std_23`, and `add_test`. Keep it out of `SPEEDGUN_SLOW_TESTS` (`:2269`), so the rule at `:2291-2294` gives it the `quick` label and the two-cpu lane runs it (FR-030). Blocks every story: no case of any story reaches ctest until one entry names this binary. Prove at the line: `cmake --build build && ctest --test-dir build -R harness_hunt_topdown_test --output-on-failure`.

- [X] T006 [P2] Add the member `topdownArea` to `RankedEvent` in
  `include/speedgun-ng/hunt.hpp`, after `delta` in the record at `:454-472`, with the
  comment that names the area it holds (FR-014, FR-019, D-8). It is a plain
  `std::string`, empty when no hunt stamped the row, and it joins no constructor
  argument, so the records that build a row by hand keep their bytes (FR-010). The
  exported layout moves, so this task rides the ABI commit with the three numbers at
  `CMakeLists.txt:59`, `:134` and `:407` and both version-binding readers (FR-028).
  `RankedEvent` already holds a DBC row, so the counted interfaces still grow by two
  and not by three (SC-006). Prove at the line: no cell scenario compiles at the
  base, which is the red state FR-029 asks for, and
  `cmake --build build --target harness_hunt_report_test` reads green once the cell
  task of User Story 1 lands.

- [X] T007 Declare the record `TopdownEventGroup` in
  `include/speedgun-ng/counters_topdown.hpp` between the `topdownTable`
  declaration at `:205-206` and the `TopdownNode` block at `:208`: one
  `SPEEDGUN_NG_EXPORT` aggregate, `std::string event` first and `std::string
  group` second, no member initializer, `@brief` text naming specs/046 FR-004,
  FR-008 and D-16, and the three tags `\pre none`, `\post none` and
  `\invariant none` that every record of this header carries
  (`:58-60`, `:97-99`). `event` stands first because a designated initializer
  needs declaration order, the shape of `TableFile` at
  `source/counters/linux_pmu/topdown_select.cpp:124-126`. No new include: the
  header already carries `<string>` and `<vector>` at `:31-32`.

- [X] T008 Declare `topdownEventGroups(const CpuIdentity&) ->
  std::vector<TopdownEventGroup>` in `include/speedgun-ng/counters_topdown.hpp`
  in the same gap, in the `[[nodiscard]] SPEEDGUN_NG_EXPORT auto` form of
  `:84`, `:205-206` and `:436-438`, with the `///` block `topdownTable` carries
  at `:188-204`: `\pre none`, one `\post` that names one pair per event name,
  the order the loaded table holds, the empty answer of an identity that selects
  no directory, and the rule that the call reads no counter, no clock, no path
  and no `MIDR` beyond the directory (D-16), plus `\invariant none`. The
  one `SG_ENSURE` that answers that `\post` stands in the definition, because
  the pair gate reads enforcement per region.

- [X] T009 Add the one banner sentence that names the grouping listing to the header
  banner of `include/speedgun-ng/counters_topdown.hpp` at `:1-23` (FR-032,
  Principle IV). The drift tool scans names from a quick-reference row to a
  header only (`tools/docs/example_drift.py:789-797`), so no docs row follows.

- [X] T010 Create the unit `test/source/counters_topdown_groups_test.cpp` in the
  frameworkless shape of `test/source/counters_topdown_select_test.cpp`: the
  `check()` and `fail()` pair of `:73-84`, one `main()` skip arm of the shape of
  `:1683-1688` on `embeddedFindDir("arch/x86") == nullptr` with exit 2 and no
  `registerMetricHost()` call, so this binary answers 0 where that unit answers 2
  at `:1691-1696`, because the accessor registers no host and resolves no planted
  row, the `Scenario` table
  selected by `argv[1]` of `test/source/harness_topdown_run_test.cpp:3128-3200`,
  the `PASS` line and exit 0 of `:1719-1722`, and the private-seam include
  `../source/counters/detail/pmu.hpp` of `:63`. Plant `groups-carry-the-vendors-file-stems`
  and `accessor-reads-the-identitys-directory` (FR-004): the Skylake-X identity
  `GenuineIntel`, 6, 85, 4 answers `LONGEST_LAT_CACHE.MISS` with `cache`,
  `BR_INST_RETIRED.ALL_BRANCHES` with `pipeline`,
  `MEM_TRANS_RETIRED.LOAD_LATENCY_GT_4` with `memory`,
  `UNC_CHA_AG0_AD_CRD_ACQUIRED.TGR0` with `uncore-cache` and `tma_backend_bound`
  with `skx-metrics`, and the answer equals row for row and in order the stems of
  the rows `detail::pmuLoadTable("arch/x86/skylakex/")` answers, deduplicated by
  first row. Measured on the pinned tree: 4,057 rows over 15 files. Plant
  `one-call-reads-no-source-of-its-own` on the `/proc/self/fd` and
  `/proc/self/maps` shape of `test/source/counters_topdown_select_test.cpp:197-231`,
  the reading of the case `oneCallReadsNoSourceOfItsOwn` at `:1243-1306` (FR-030).

- [X] T011 Define `topdownEventGroups` in `source/counters/linux_pmu/topdown_select.cpp`
  after `topdownTable` at `:158-199` and ahead of the anonymous namespace at
  `:201`: the `#ifdef __linux__` guard inside the body as at `:161-194`, the
  `detail::PmuIdent` build copied from `:165-170`, `detail::pmuSelectDirectory`,
  `detail::pmuLoadTable`, the group as
  `std::filesystem::path{row.fileName}.stem().string()`, one documented
  insert-if-absent line so the answer holds one pair per name (D-16), and one
  `SG_ENSURE` that names that fact about the listing alone and names no fact
  about a vendor file (D-16, ruling R3). Add `<filesystem>` and `<unordered_set>`
  to the include list at `:46-56`. Every row reaches a pair, event rows and
  metric rows alike (ruling R4), and no `MIDR` and no `coreType` enters the key
  (D-16, ruling R2). A suffix test would put a branch here that no pinned file
  answers, and FR-031 forbids a new exclusion marker.

- [X] T012 Register `counters_topdown_groups_test` in `test/CMakeLists.txt` beside the
  two specs/042 counters units, in the block at `:521-563`: `add_executable` as
  at `:550-551`, `target_link_libraries(... PRIVATE speedgun-ng::speedgun-ng)` as
  at `:552-553`, `target_compile_features(... PRIVATE cxx_std_23)` as at `:554`,
  `add_test` as at `:560-561` and `set_tests_properties(... SKIP_RETURN_CODE 2)`
  as at `:562-563`. It needs no `SG_TOPDOWN_TABLE_DIR` define, because it writes
  no fixture tree and plants no directory. The loop at `:2289-2310` gives the
  entry the quick label and the 300 s ceiling with no further line.

- [X] T013 [P] Add the row `"counters_topdown_groups_test": "2"` to the `MASTER_SKIP`
  baseline of `test/tier_labels.sh` at `:80-92`, because the rule at `:219-226`
  faults a registration that sets a `SKIP_RETURN_CODE` the baseline does not
  state. The row costs nothing on a name no registration lists yet, which the
  comment at `:77-79` states.

- [X] T014 Move the three numbers in `CMakeLists.txt` in one pass: `VERSION 0.30.0` to
  `0.31.0` at `:59`; the archive `SOVERSION 5` to `6` at `:134`, with one new
  comment line in the chain at `:116-133` that names `TopdownEventGroup`,
  `topdownEventGroups` and FR-004, FR-008 and D-16; and the harness `SOVERSION 20`
  to `21` at `:407`, with one new comment line in the chain at `:366-406` that
  names the `RankedEvent` member of D-8. Move every comment that states a current
  number, which covers `:49-50`, `:55-57` and `:406`, and leave the lines that
  record an earlier move. The archive move cannot land alone: `soVersionNumbers`
  keeps one number per property line in file order, so `:134` on its own answers
  `6 20` and fails both readers at `test/source/harness_cli_table_test.cpp:1459-1461`
  and `test/source/views_test.cpp:285-287`. Prove with
  `ctest --test-dir build -R 'harness_cli_table_test|views_test' --output-on-failure`.

- [X] T015 Move the first version-binding reader in
  `test/source/harness_cli_table_test.cpp`: `theVersionBindingsReadTheMovedNumbers`
  at `:1448-1462` reads `0.31.0`, `SOVERSION 21`, `SOVERSION 6` and the pair
  `6 21`, and the comment above it at `:1413-1418` and `:1439-1444` moves with it.
  Leave `soVersionNumbers` at `:1419-1437` alone: it keeps only a line whose first
  word is `SOVERSION `, which is the property-line rule FR-028 asks for. Replace
  the two comment-settleable substring tests at `:1455` and `:1457` with reads of
  the property line beside their own target, so no sentence can settle them.

- [X] T016 Move the second version-binding reader in `test/source/views_test.cpp`: the
  block at `:276-288` reads `0.31.0`, `SOVERSION 21`, `SOVERSION 6` and the pair
  `6 21`, and the comment at `:268-275` moves with it. Leave the helper
  `soVersionNumbers` at `:119-137` alone, and replace the two substring tests at
  `:281` and `:283` with property-line reads of the same shape.

- [X] T017 Answer both DBC hand gates over the head and read 347 interfaces with 0
  gaps in each matrix, two more interfaces than the 345 the base answers, because
  `include/speedgun-ng/counters_topdown.hpp` gains one record row and one function
  row (`tools/dbc/dbc_doc_gate.py:511-537`). Run, in order: `doxygen` over the
  XML-only Doxyfile whose `INPUT` is `include/speedgun-ng` and whose
  `OUTPUT_DIRECTORY` is `build/dbc-gate`, then
  `/usr/bin/python3 tools/dbc/dbc_doc_gate.py --xml build/dbc-gate/xml --registry tools/dbc/macros.yaml --out /tmp/046-dbc/matrix.json`,
  then
  `/usr/bin/python3 tools/dbc/dbc_pair_gate.py --src include/speedgun-ng --src2 source tools --registry tools/dbc/macros.yaml --doc-matrix /tmp/046-dbc/matrix.json --out /tmp/046-dbc/pair.json`.
  The `dbc-gate` target stays out of the loop, because it cached a Python with no
  PyYAML (AP-2). The move from 345 to 347 stands at this commit, and the polish phase
  re-reads both matrices at the final head. The analyzer gate stands in the polish
  phase with the docs-and-gates lane, run from a fresh tree against
  `/tmp/046-logs/findings-base.txt`.

- [X] T018 Clear the prose gate over the three paths this lane writes. A new file
  needs a tracked path first, so run `git add -N test/source/counters_topdown_groups_test.cpp`,
  then `/usr/bin/python3 tools/prose/prose_gate.py --check prose --mode tree --paths include/speedgun-ng/counters_topdown.hpp --paths source/counters/linux_pmu/topdown_select.cpp --paths test/source/counters_topdown_groups_test.cpp`,
  then `git reset -q test/source/counters_topdown_groups_test.cpp`. A run that
  reports 0 sources examined is void (AP-2b).
## Phase 3: User Story 1 - Name the area of each culprit (Priority: P1)

- Test first, then the code that answers it. The stamp, the cell and the guide text.

## Phase 3: User Story 1 - Name the area of each culprit (Priority: P1)

- Test first, then the code that answers it. The stamp, the cell and the guide text.

- [X] T019 [US1] Plant the node arm in `test/source/harness_hunt_topdown_test.cpp`, over a hand-built `sg::counters::TopdownTree` whose nodes carry `name`, `parent`, `level` and `events` as `include/speedgun-ng/counters_topdown.hpp:232-241` spells them: `area-takes-the-deepest-node` (a level-3 node and a level-2 node name one address, and the path reads the level-3 name after its two parents, FR-001, FR-007, D-4, D-10), `level-one-node-answers-one-name` (FR-007), `tie-goes-to-the-first-row-in-table-order` (two nodes of one level name one address, the lower index in `nodes` wins, and the case swaps that order to show table order decides and not the name text, FR-003, D-4), and `second-spelling-names-no-node` (one address planted twice, once as `cpu/branch-instructions` and once as `cpu/BR_INST_RETIRED.ALL_BRANCHES` in the AP-4 shape, where only the spelling a node carries reaches a node, FR-002).

- [X] T020 [US1] Define the node arm in `source/harness/hunt_topdown.cpp`: `topdownDeepestNode` with the index postcondition and the one `SG_ASSERT` of `contracts/area-mapper.md`, `topdownPathOf` with its `SG_ENSURE`, and arm 2 of `topdownArea` (FR-001, FR-002, FR-003, FR-007). Read the node list through `counters::TopdownNode::events` alone, and no other field of the selection (FR-008, D-1). Prove at the line: `cmake --build build --target harness_hunt_topdown_test && ctest --test-dir build -R harness_hunt_topdown_test --output-on-failure`.

- [X] T021 [US1] Plant the refusal arm in `test/source/harness_hunt_topdown_test.cpp`: five cases, one per kind of `include/speedgun-ng/counters_topdown.hpp:329-336`, each planted from data, so no case states a fact about the machine that runs it (FR-005, FR-030). Carried-over helper lines are those of `test/source/counters_topdown_select_test.cpp`, fixture rows are those of `test/source/detail/topdown_fixture.hpp`: `arm-midr-answers-unmapped` with `armIdentity()` at `test/source/counters_topdown_select_test.cpp:1447` answers `ARM`; `unknown-vendor-answers-unmapped` with `unmatchedIdentity()` at `test/source/counters_topdown_select_test.cpp:150` answers `UNKNOWN_VENDOR`; `no-directory-answers-unmapped` with `intelIdentity(15, 6, 5)` answers `NO_DIRECTORY`; `no-topdown-rows-answers-unmapped` with `zen3Identity()` at `test/source/counters_topdown_select_test.cpp:1470` answers `NO_TOPDOWN_ROWS`; `two-scope-rows-answer-unmapped` with `hybridIdentity()` at `test/source/counters_topdown_select_test.cpp:1456` over `plantedTable(kHybridDirectory, topdownHybridRows())`, that is `kHybridDirectory` at `:102` and `topdownHybridRows()` at `:733`, answers `HYBRID`. Name the kind each recipe answers in that case's comment. State in the `two-scope-rows-answer-unmapped` comment why that directory needs rows of its own to reach `HYBRID`: the no-rows rule answers ahead of the scope rule over a directory that carries both scopes and no row of any top-down group, the reading `test/source/counters_topdown_select_test.cpp:1647-1668` pins, so a mixed-scope directory without level rows answers `NO_TOPDOWN_ROWS` and the planted rows are what settle the scope kind. Each case asks `sg::counters::topdownTree` at the pair's level, reads no tree, hands the empty optional to the mapper, and reads `unmapped` for two addresses. The mapper reads no refusal value and no `MIDR`: it sees the empty optional alone (FR-001).

- [X] T022 [US1] Define arm 1 of `topdownArea` in `source/harness/hunt_topdown.cpp`: an absent tree answers `unmapped` for every address, and it answers before the grouping listing is read, so a refused host reads no listing at all (FR-005, FR-006, D-16). Prove at the line: `ctest --test-dir build -R harness_hunt_topdown_test --output-on-failure`.

- [X] T023 [US1] Plant the grouping arm in `test/source/harness_hunt_topdown_test.cpp`: `area-falls-back-to-the-vendor-file-stem` (an address no node names answers the stem a planted `sg::counters::TopdownEventGroup` row carries, and the planted stems use the vendor's own spellings, `cache` for Skylake-X and `l2-cache` for Zen 5 in the AP-7 shape, FR-004, D-3), `index-drops-the-device-prefix` (the row names `cache-misses` and the address names `cpu/cache-misses`, and the case reads one group, which is the strip D-16 states and not the cut `leafPartOf` of `source/harness/detail/hunt_selection.hpp:99-104` makes, FR-037), and no case here repeats a name. The pair rule of a repeated name needs its machine where the rule stands: T095 proves it in the counters unit over `amdzen4/recommended.json`, where `umc_cas_cmd_rate` and `umc_cas_cmd_read_ratio` each stand twice in one file, and FR-037 keeps the index free of a second copy (FR-037, D-16).

- [X] T024 [US1] Define the grouping index and arm 3 in `source/harness/hunt_topdown.cpp`:
  one build per run over the listing the D-16 accessor answers, which already holds one
  pair per event name, so the index adds no rule of its own and no second first-row
  branch. Key it on the group, and strip the device prefix ahead of the lookup, the
  order FR-037 states. Keep no event list and no vendor word of the harness's own, and
  cite in the comment the measured basis that puts the rule in the accessor: six of the
  46 pinned `arch/x86` directories repeat a name inside one file, and
  `amdzen4/recommended.json` repeats `umc_cas_cmd_rate` and `umc_cas_cmd_read_ratio`, so
  the rule needs no hybrid host (FR-004, FR-037, D-3, D-16). Prove at the line:
  `ctest --test-dir build -R harness_hunt_topdown_test --output-on-failure`.

- [X] T025 [US1] Plant the closing arm and the purity claims in `test/source/harness_hunt_topdown_test.cpp`: `no-node-and-no-row-answers-unmapped` for US1 AC-5, which no other case settles, `two-calls-answer-one-area` for the same three inputs twice (FR-006), and `oneCallReadsNoSourceOfItsOwn` in the shape of `test/source/counters_topdown_select_test.cpp:1243` with the `/proc/self/fd` and `/proc/self/maps` reading of `:200-231` (FR-001, D-7, FR-030).

- [X] T026 [US1] Define arm 4 and the closing tags in `source/harness/hunt_topdown.cpp`: the answer falls to `unmapped` where no node and no row names the address, and the `SG_ENSURE` of `contracts/area-mapper.md` states that the answer stands never empty (FR-001, FR-005, D-3). Keep the function free of any counter, event, clock and kernel interface (FR-001, D-7). Prove at the line: `./build/test/harness_hunt_topdown_test oneCallReadsNoSourceOfItsOwn`.

- [X] T027 [US1] Plant the two seam asks in `test/source/harness_hunt_topdown_test.cpp`: `planted-run-asks-the-area-at-level-six` (a `RunOptions` with `topdownIdentity` and `topdownTable` set, where the table comes from `plantedTable(kIntelDirectory, topdownIntelRows())` of `test/source/detail/topdown_fixture.hpp`, and the ask answers level 6, that identity and that table while `topdownLevel` holds a value of its own, FR-001, FR-033, FR-034, D-17), `unplanted-run-asks-the-area-at-level-six` (no planted value: the level reads 6, the table reads absent, and the identity reads `sg::counters::cpuIdentity()`, which states no host claim), `level-six-ask-clamps-to-a-shallow-table` (`topdownIntelShallowRows()` answers its own depth, and the mapper still settles a path, FR-001, D-17), and `area-and-companion-asks-differ-in-level-alone` (two `TopdownSelection` records of one option set, equal but for `level`, FR-033).

- [X] T028 [US1] Define the seam in `source/harness/hunt_topdown.cpp`: `topdownAreaSelection(const RunOptions&)` fills the identity, the level constant and the planted table, and reads no other field of the record (FR-001, FR-033, FR-034, D-17). Prove at the line: `ctest --test-dir build -R harness_hunt_topdown_test --output-on-failure`, which is also the machine for the FR-033 claim that one identity and one selection feed both uses.

- [X] T029 [US1] Plant the stamp in `test/source/harness_hunt_topdown_test.cpp`: `stamp-sets-every-entry-including-unranked` (a run record whose `entries` hold one ranked row and one row with no rank, both stamped, while `skipped` gains nothing, FR-009, D-8) and `stamp-leaves-an-unhunted-result-alone` (a result with no hunt record changes by zero field, FR-010). Build the records by hand in the shape of `test/source/harness_topdown_report_test.cpp:257`.

- [X] T030 [US1] Define `stampTopdownAreas` in `source/harness/hunt_topdown.cpp`: one loop over `entries` of every hunt record, the ranked rows and the unranked rows alike, no other field moved, no record added, removed or reordered, and no read of `rank`, any sort key, the `cv` threshold, the budget or either emit function (FR-009, FR-010). The call site in the batch pass of `source/harness/cli.cpp:1983` stands in the console lane, ahead of the deferred console at `:2036-2042` (FR-009, research item 10). Prove at the line: `ctest --test-dir build -R harness_hunt_topdown_test --output-on-failure`.

- [X] T031 [US1] Clear the prose gate over the two new source files and the new unit, in the form the brief sets: `git add -N source/harness/detail/hunt_topdown.hpp source/harness/hunt_topdown.cpp test/source/harness_hunt_topdown_test.cpp`, then `/usr/bin/python3 tools/prose/prose_gate.py --check prose --mode tree --paths source/harness/detail/hunt_topdown.hpp`, then the same line for each other path, then `git reset -q` for each. ASD-STE100 holds in every line, comment and scenario name (FR-032).

- [X] T032 [US1] Prove the DBC count for the new pair: the header stands under `source/harness/detail/`, so it joins no counted interface and the count moves only by the counters accessor of D-16, from 345 at the base to 347 at the head with 0 gaps (research item 11). Run both gates by hand, since a configured tree may cache a Python without PyYAML: `doxygen build/dbc-gate/Doxyfile`, then `/usr/bin/python3 tools/dbc/dbc_doc_gate.py --xml build/dbc-gate/xml --registry tools/dbc/macros.yaml --out build/dbc-gate/doc-matrix.json`, then `/usr/bin/python3 tools/dbc/dbc_pair_gate.py --src include/speedgun-ng --src2 source tools --registry tools/dbc/macros.yaml --doc-matrix build/dbc-gate/doc-matrix.json --out build/dbc-gate/pair-matrix.json`.

- [X] T033 [US1] In `test/source/harness_hunt_report_test.cpp`, add the scenario
  `area-cell-after-delta-and-before-the-flag` to the table at `:1265-1290`. Hand
  one `RankedEvent` an area of
  `topdown.backend_bound > topdown.intel.tma_l1_bound` and print the record
  through the `printHuntRanking` declaration at `:90-93`. One ` area=` cell
  stands after the ` delta=` cell and before the ` multiplexed` flag, and the
  `mark=` row prints the same cell before ` mark=` (FR-013, D-13). Prove it red
  with `"build/test/harness_hunt_report_test"
  area-cell-after-delta-and-before-the-flag`.

- [X] T034 [US1] In `test/source/harness_hunt_report_test.cpp`, add the scenario
  `no-area-cell-for-an-empty-member`: a record whose `topdownArea` stands empty
  prints no cell and no separator, and its line keeps every byte
  `example/docs/hunt_rank.expected:6-10` pins (FR-014). Same file, so this row
  follows the row above.

- [X] T035 [US1] In `source/harness/report.cpp`, add
  `printTopdownAreaCell(const sg::hunt::RankedEvent&)` in the anonymous namespace
  beside `printSensitivityCells` at `:1183-1200`. Its post states one ` area=`
  cell holding the record's area, and no output at all where that member stands
  empty. The format string reads `" area=%s"` with one leading space and no field
  width, the shape of `:1186` and `:1196-1198` (FR-013, FR-014, D-13). No call
  yet.

- [X] T036 [US1] In `source/harness/report.cpp`, call that helper twice in
  `printHuntRanking`: after `printSensitivityCells(entry)` at `:1349`, ahead of
  the ` multiplexed` flag, and after `printSensitivityCells(entry)` at `:1363`,
  ahead of ` mark=`. The cell stays out of `printSensitivityCells`, whose post at
  `:1179-1182` names exactly three cells (FR-013). Green:
  `ctest --test-dir build -R harness_hunt_report_test --output-on-failure`.

- [X] T037 [US1] In `source/harness/cli.cpp`, stamp the areas at the end of the
  batch pass: one `detail::stampTopdownAreas` call after the rank
  stamp at `:1983` and ahead of the move back at `:1985-1987`. The pass spans
  `:1950-1994`, so the deferred console at `:2036-2042`, the writer, and the two
  emit paths at `:2182-2204` all read stamped records (FR-009, research item 10).
  The stamp body and its two scenarios stand in the mapper lane.

- [X] T038 [US1] In `test/source/harness_hunt_report_test.cpp`, add the scenario
  `every-ranked-row-of-a-hunt-run-carries-an-area-cell`. Drive one run of the
  shape at `:974-975`: `captureRun` over `withCase` with `--hunt=` set to
  `selection({0, 1, 2, kRefusedEvent})` and with `--hunt-top=2`. Expect one
  ` area=` cell on every `hunt: rank` row, the `mark=` rows included. The scripted catalog
  names no vendor event, so each cell reads ` area=unmapped` on every host
  (FR-005, FR-009). The recorded region of `example/docs/hunt_rank.cpp` cannot
  carry this proof, because that example builds rows through `rankEvents` and
  stamps nothing.

- [X] T039 [P] [US1] Prove the run gains no option. The command
  `git diff --name-only b1dd7670..HEAD` names neither
  `source/harness/detail/options.hpp` nor any writer of the emit paths, and the command
  `/usr/bin/python3 tools/docs/example_drift.py --options .` answers 0 findings.
  `--hunt` stays the only switch a reader types, and the option record stays where it
  stands (FR-012, D-2).

- [X] T040 [US1] Prove the recorded block that quotes a hunt row keeps its
  bytes:
  `ctest --test-dir build -R 'report_shape_hunt_rank_sample' --output-on-failure`
  over `example/docs/hunt_rank.expected` and the region
  `docs/pages/harness.md:2269` quotes. No re-bless, and no rule added to
  `tools/docs/report_shape.py` (FR-014, FR-027). State beside it that
  `example/docs/hunt_ranks.expected` changes by zero lines: its rows are
  hand-built, so the area member stands empty and prints no cell (FR-014). That
  file holds no `report_shape_*_sample` entry, and `test/example_run.sh` weighs
  rows, failures and declared skips, so no byte machine covers it. Record that
  gap as pre-existing (AP-2), and add no registration, because a gate change
  needs its own justification.

- [X] T041 [P] [US1] Write `example/docs/hunt_topdown.cpp` as a complete program
  modeled on `example/docs/hunt_rank.cpp` (read at the base: it declares
  `sg::detail::printHuntPlan`, `printHuntRanking` and `printHuntSkips` at :23-30,
  scripts `EventSamples` rows the file writes itself at :32-102, and carries the
  marked region `// sg-example:begin hunt-topdown`-style markers at :104-118):
  the new file builds its ranked rows the same way, sets `topdownArea` on each
  row with one path value, one vendor file stem and one `unmapped` so all three
  kinds of answer print, hands the record to `sg::detail::printHuntRanking` so
  the `area=` cell prints, and carries one marked region
  `// sg-example:begin hunt-topdown` / `// sg-example:end hunt-topdown` around
  the record builder (FR-025, FR-027; no host word and no real level-1 block
  enters anything the page quotes).

- [X] T042 Register the example: in `example/CMakeLists.txt`, add
  `add_docs_example(hunt_topdown)` after `add_docs_example(hunt_ranks)` at :88 —
  this file holds no `GLOB` (measured at the base: the globs stand at the root
  `CMakeLists.txt:280` and `:1035` over the harness and counters trees, and the
  `add_docs_example` function at `example/CMakeLists.txt:46` is the only way a
  `docs/` program builds), so target `docs_hunt_topdown` and runner
  `run_docs_hunt_topdown` exist only after this line, and the all-examples gate
  `example_run` (`test/CMakeLists.txt:2100`, driven by `test/example_run.sh`)
  then runs the new binary with no further registration.

- [X] T043 Write `example/docs/hunt_topdown.expected` with the recorded run of
  `docs_hunt_topdown`, bounded by `# sg-example:begin report` and
  `# sg-example:end report` exactly as `example/docs/hunt_rank.expected` is
  (17 lines, read at the base), from one real run of the example at the head,
  in the commit that lands T041 of `example/docs/hunt_topdown.cpp`; the ranked
  rows carry the `area=` cell, and the file holds no host word (FR-025).
  `example/docs/hunt_rank.expected` and `example/docs/hunt_ranks.expected` keep
  their bytes: the empty member prints no cell (`contracts/console-cell.md`).

- [X] T044 Register the shape gate: in `test/CMakeLists.txt`, after the
  `hunt_rank` precedent block at :2074-2089, add the guarded block below, copied
  in the shape of that precedent. FR-025 asks a machine, and
  `tools/docs/report_shape.py:313` resolves the owner `hunt_topdown.expected`.
  `tools/docs/report_shape.py` gains no fold rule.

  ```text
  if(TARGET docs_hunt_topdown)
    add_test(NAME report_shape_hunt_topdown_sample
      COMMAND python3 ${CMAKE_SOURCE_DIR}/tools/docs/report_shape.py
        --binary $<TARGET_FILE:docs_hunt_topdown>
        --readme ${CMAKE_SOURCE_DIR}/docs/pages/harness.md
        --example hunt_topdown)
    set_tests_properties(report_shape_hunt_topdown_sample PROPERTIES TIMEOUT 120)
  endif()
  ```

- [X] T045 In `test/CMakeLists.txt`, add the budget-exception comment
  `# budget exception report_shape_hunt_topdown_sample keeps the 120 s master
  sets for the shape fold of the hunt top-down example: ...` after the
  `report_shape_hunt_rank_sample` entry at :2347-2349, naming the measured
  slowest run and its lane, in the words that list already carries.

- [X] T046 [US1] In `docs/pages/harness.md`, write `### The area cell` in the
  `## Hunter mode` subsection between `### The hunt lines` (:2382) and
  `### The norm and the rank` (:2442): the cell reads `area=`, it stands beside
  the specs/040 cells on each ranked row, and the three kinds of answer are the
  path of node names joined by ` > ` that the deepest naming node settles, the
  vendored row's file stem, and `unmapped` (FR-024, FR-025; the tie rule of
  D-4: of two nodes at one depth that both name the event, the one first in
  table order names the area).

- [X] T047 [US1] In `docs/pages/harness.md`, extend the
  `hunt::RankedEvent` row of `### The names` at :2594 so its description names
  the area member beside the marks (FR-024).
## Phase 4: User Story 2 - Read the area back from the report (Priority: P1)

- The document commit carries the schema move with its writer (FR-020).

## Phase 4: User Story 2 - Read the area back from the report (Priority: P1)

- The document commit carries the schema move with its writer (FR-020).

- [X] T048 [US2] In `test/source/harness_hunt_json_test.cpp`, stamp a path on
  two records: set `topdownArea` on the first entry of `singleStageHunt()`
  (`:299`) and leave the second empty, then add scenario
  `area-key-stands-after-delta` to `scenarios()` (`:2056-2073`); it asserts the
  written bytes hold `"topdownArea":"<path>"` after the `delta` of the stamped
  record and hold no such key beside the empty record, whose other bytes stand
  unchanged (FR-019, D-11, US2 AC-1).

- [X] T049 [US2] In `test/source/harness_hunt_json_test.cpp`, extend the
  per-event key list of `huntKeysInOrder` at `:938-949` with `"topdownArea"`
  after `"delta"`, so scenario `keys` refuses a written key out of the order the
  schema states (FR-019, FR-020, D-11).

- [X] T050 [US2] In `test/source/harness_hunt_json_test.cpp`, add one `Plant`
  row to `kPlants` beside the other `entries` rows (`:1237-1262`): `.node =
  "entries"`, `.key = "topdownArea"`, `.body = "7"`; scenario `refusals`
  (`readerRefusals`, `:1396`) then reads the refusal at
  `/benchmarks/0/hunt/entries/0/topdownArea`. This row cannot move to the `Mutation`
  table of `test/source/harness_json_load_test.cpp:829-860`, because the document that
  table mutates carries no hunt node (FR-021, US2 AC-3).

- [X] T051 [US2] In `test/source/harness_hunt_json_test.cpp`, move the stamped
  minor in `schemaConformance` (`:1537-1540`) to `1.14` and name the new key of
  the per-event node in that check's message (FR-020, FR-023).

- [X] T052 [US2] In `test/source/harness_json_schema_test.cpp`, teach
  `entryHuntKeysPairAtStampedMinor` (`:204-236`) to find `"topdownArea"` after
  `"delta"` in the node's `x-order` text, move its stamp check at `:232-235` to
  `"schemaVersion":"1.14"`, and move the matching stamp check of
  `theSeriesKeysPairAtTheStampedMinor` at `:377-379` with the two comments at
  `:202` and `:242` (FR-020, FR-023, US2 AC-3).

- [X] T053 [US2] In `test/source/harness_json_load_test.cpp`, set
  `measured.topdownArea` in `theHuntRankFieldsLoadBackEqual` (`:313-388`) and
  check it reads back equal while the unstamped record at `:338-344` reads back
  empty; this file runs its units from `main()` at `:2205-2236`, so the check
  rides that scenario-free list (FR-021, US2 AC-1).

- [X] T054 [US2] In `test/source/harness_json_load_test.cpp`, plant
  `theAreaKeyReadsBackEqual`, the machine `contracts/document-key.md:162` names and no
  task builds: write a hunt document with the standing writer inside the process, hand
  the bytes to `sg::loadReport` beside the reads at `:54` and `:83`, and name the member
  on the loaded record. T053 hand-sets the member on a hand-built record, so no other
  case crosses the writer and the reader in one run (FR-018, SC-002).

- [X] T055 [US2] In `test/source/harness_json_load_test.cpp`, load a hunt document
  stamped `1.13`, one minor behind the new current, and name every area unset on the
  records it loads, beside the older-minor read at `:109`. The reader accepts an older
  minor, so an absent key is the only shape such a document can carry, and FR-024 is the
  rule a reader of an old document meets (FR-024, FR-021, SC-002).

- [X] T056 [US2] In `test/source/harness_json_load_test.cpp`, add units
  `aDocumentWithoutTheAreaKeyLoads` and `anAreaOfAnotherKindRefuses` and call
  both from `main()` beside `theHuntRankFieldsLoadBackEqual` at `:2210`: the
  first strips `"topdownArea":"<path>",` from the written ranks document at
  `:352` and loads with the member empty, the second plants a `null`, a number,
  a bool and an array in turn and names
  `/benchmarks/0/hunt/entries/0/topdownArea` (FR-021, US2 AC-2, AC-3).

- [X] T057 [US2] In `docs/schema/report-v1.json`, add `"topdownArea"` after
  `"delta"` in the `x-order` of the closed per-event node (`:948-958`, the
  `delta` line at `:957`) and `"topdownArea": { "type": "string" }` after
  `"delta"` in its `properties` (`:968-978`, the `delta` line at `:977`); leave
  `required` (`:959-966`) and `additionalProperties: false` (`:967`) alone, and
  change no version value, since `schemaVersion` there is a plain string
  (`:11-13`) and `$id` names the major alone (`:3`) (FR-020, D-11).

- [X] T058 [US2] In `source/harness/json_report.cpp`, move the stamped minor at
  `kSchemaVersion` (`:47`) to `1.14` and give the version-history comment
  (`:31-46`) one clause that names `topdownArea`, the per-event node and
  specs/046, in the shape of the 1.13 clause at `:41-46` (FR-020, D-11).

- [X] T059 [US2] In `source/harness/json_report.cpp`, write the key in
  `writeEntryHunt`: one conditional `put(builder, entry, "topdownArea",
  ranked.topdownArea)` through the plain string form at `:593`, after the
  `delta` write at `:604` and inside the same block; `putOrNull` (`:225-238`)
  takes no part, because a `null` spelling has no writer here (FR-019, D-11).

- [X] T060 [US2] In `source/harness/json_report.cpp`, move the header comment of
  `writeEntryHunt` (`:569-572`), which states that no key of the node is ever
  absent, to the rule FR-019 keeps: the key stands only where the member holds a
  value, and an empty member writes no key (FR-019).

- [X] T061 [US2] In `source/harness/json_load.cpp`, read the key in
  `readRankedEvent` (`:1884-1964`): one `optionalField` arm (`:86-94`) after the
  rank-figure block (`:1941-1962`), with `asString` (`:135-144`) at the pointer
  `where + "/topdownArea"`; the required-key vector (`:1890-1898`), the
  designated initializer (`:1912-1924`), `field`, `settled` and `checkNode`
  stand untouched, and the comment above the rank figures (`:1935-1940`) gains
  one sentence naming the new key. This lane edits `readRankedEvent` alone in
  this file; specs/047 owns the other functions of `source/harness/json_load.cpp`
  (FR-021, D-11).

- [X] T062 [US2] In `test/source/harness_json_load_test.cpp`, move the two
  later-minor plants one minor above the new current: `:108` and `:1200`
  (`versionRuleGatesTheReader`, `theHeaderFieldsAndTheStoredWarningsLoad`) from
  `1.13` to `1.15`, with the read-back check at `:1203`; move the current-minor
  pin of the written document, the `Mutation` row at `:830`, to `1.14`
  (FR-023).

- [X] T063 [US2] In `test/source/harness_json_report_test.cpp`, move the pin in
  `theDocumentStampsTheMinorThisFeatureRequires` (`:546-550`) to `1.14` and name
  specs/046 beside the standing spec pointers (FR-023).

- [X] T064 [US2] In `test/source/harness_jitter_bytes_test.cpp`, move the pin in
  `aTscRunPassesTheSchemaAtTheStampedMinor` (`:345-347`) to `1.14` (FR-023).

- [X] T065 [US2] In `test/source/harness_noise_report_test.cpp`, move the pins
  at `:426-429` and `:458` and the comments at `:413` and `:456` to `1.14`
  (FR-023).

- [X] T066 [US2] In `test/source/harness_topdown_report_test.cpp`, move the pin
  at `:816-818`, which states the minor as the one difference that feature made,
  to `1.14` (FR-023).

- [X] T067 [US2] In `docs/pages/harness.md`, move the version-rule sentence at
  `:1400-1403` from `1.13` to `1.14` and name `topdownArea` beside the two
  figures that sentence already names. No line cap reaches this page: the page
  budget and the 25-line section caps run over `README.md` alone
  (`tools/docs/example_drift.py:781-782`), and this page takes the forbidden-word
  scan at `:785-788` (FR-020, FR-023, D-12).

- [X] T068 [US2] In `test/source/harness_hunt_rank_test.cpp`, add scenario
  `emit-bytes-stand-under-areas` to `scenarios()` (`:2525-2585`): build one
  `EntryHunt` with `rankRow` (`:1094`), read `toCounterSet` and `toCompareRules`,
  set `topdownArea` on every entry, read both again, and check the two answers
  match byte for byte; prove the same rule with
  `git diff b1dd7670 -- source/harness/hunt_plan.cpp source/harness/hunt_cli.cpp`
  naming no hunk, since those two files hold all four emit functions (FR-010,
  FR-011, D-12, SC-008). AP-10 refuses a live run as the proof, so no scenario
  here reads emitted files.

- [X] T069 [US2] In `test/source/harness_hunt_json_test.cpp`, read the two
  fixtures that must stand unchanged: `kCommittedDocument` and
  `kCommittedConsole` (`:395-396`), compared under `identityMasked` (`:786`) by
  scenario `no-hunt-bytes` (`:1650-1706`). Run
  `ctest --test-dir build -R 'harness_hunt_json_test' --output-on-failure`, then
  `git diff b1dd7670 -- test/data/hunt_no_hunt_document.json
  test/data/hunt_no_hunt_console.txt` and read no hunk: each fixture pins a stale
  minor of its own (`schemaVersion` `1.4`, version `0.19.0`) and no reader
  compares it (FR-018, FR-022, D-12, SC-003).

- [X] T070 [US2] In `test/source/harness_json_load_test.cpp`, name the moved
  minor in the reader: `versionRuleGatesTheReader` (`:93-120`) already loads a
  document one minor ahead and refuses a later major, so add the check that a
  document stamped `1.14`, the value `source/harness/json_report.cpp:47` carries,
  loads and answers that text through `schemaVersion` (FR-020, D-12, SC-003).
  SC-003 asks this unit to stand beside the two version-binding readers at
  `test/source/harness_cli_table_test.cpp:1455-1461` and
  `test/source/views_test.cpp:281-287`. The ABI commit moves those two readers under
  FR-028, and this unit moves here under FR-020, so the two stand in different commits
  and share no file.

- [X] T071 [US2] In `docs/pages/harness.md`, in `### The two hunt objects`
  (:2522), state the version rule for the new key: an entry hunt item carries
  `topdownArea` only where the run settled an area, an empty member writes no
  key, a `null` is refused, and a document at 1.13 or older loads with the
  member unset (FR-024, FR-019, FR-021; US2).

- [X] T072 [P] [US2] In `README.md`, add one row to the quick-reference name
  table, after `| ReportData::graph |` at :949:
  `| `RankedEvent::topdownArea` | field | The pipeline area one hunt run settled
  for the ranked row | [hunter mode](#hunter-mode) |`, and no marked block:
  the page then reads 1001 lines against `PAGE_LINE_CAP` 1004
  (`tools/docs/example_drift.py:256`), the three-line page room of the measured
  base of 1000, which is the shape of FR-026 ("the block at
  `docs/pages/harness.md` with the row alone reads 0 findings at 1001 lines");
  ONE row and no second line: `SECTION_LINE_CAP` 25 with `SECTION_LINE_CAPS`
  holding only Registration 38 and Fixtures 43
  (`tools/docs/example_drift.py:257,262-265`), `### Hunter mode` running 24
  lines at `README.md:781-804`; a row in the quick-reference table takes none
  of those 24, because `feature_sections` spans `###` sections only and
  `## Quick reference` closes the scan (`tools/docs/example_drift.py:603-623`),
  and a row moved into `### Hunter mode` itself would land the section at
  exactly 25; `SECTION_OWNERS` (`tools/docs/example_drift.py:76`) gains
  nothing, because README quotes no region of the new file.
## Phase 5: User Story 3 - Compare a culprit with its level-1 split (Priority: P2)

- The companion pass, its channel, the block slot and the refusal path.

## Phase 5: User Story 3 - Compare a culprit with its level-1 split (Priority: P2)

- The companion pass, its channel, the block slot and the refusal path.

- [X] T073 [US3] In `test/source/harness_hunt_report_test.cpp`, widen the
  `hunt-block-order` scenario, the body at `:972-1001` and the table row at
  `:1283`, to a hunter run: the block opens after that benchmark's hunt block
  closes, one block prints, and every byte of the plan line, the ranked rows, the
  skipped rows and the summary line stands as it does now (FR-016, FR-033).

- [X] T074 [US3] In `test/source/harness_hunt_report_test.cpp`, add the scenario
  `a-hunter-run-prints-one-topdown-block`: `capturePrint` over `printResult` with
  a result that carries `BenchmarkResult::topdown` beside the carrier answers one
  block, so the standing call at `source/harness/report.cpp:765` stands suppressed
  on that route (FR-016, D-9).

- [X] T075 [US3] In `test/source/harness_hunt_report_test.cpp`, add the scenario
  `entry-run-figures-stand-equal-between-base-and-head`: the entry record of a
  hunter run states its own `pass=` figures and no further pass disclosure, because
  the companion answers a record of its own (FR-015, FR-035). The base-to-head
  reading of those bytes stands in the acceptance report.

- [X] T076 [US3] In `test/source/harness_hunt_cli_test.cpp`, add the scenario
  `a-hunter-run-prints-no-coverage-line-and-no-second-disclosure`: a
  `withFilter` run with `--hunt=` answers neither
  `topdown: core-wide coverage=`, gated at `source/harness/cli.cpp:1584-1593`, nor
  a second `method=` line, the one `source/harness/report.cpp:303-334` prints from
  `result.topdown` (FR-035).

- [X] T077 [US3] In `test/source/harness_hunt_cli_test.cpp`, add the scenario
  `a-topdown-level-names-the-block-of-a-hunter-run`: one run with `--hunt=` beside
  `--topdown=2` answers one block, and its header states level 2, so the implicit
  level 1 applies where no `--topdown` stands (FR-033, FR-034, D-17).

- [X] T078 [US3] In `test/source/harness_hunt_cli_test.cpp`, add the scenario
  `a-refused-host-prints-one-refusal-line-and-keeps-the-status`: the refusal line
  of the widened guard at `source/harness/cli.cpp:303` shows one time, and the
  status equals the status of the same run with no option (FR-017).

- [X] T079 [US3] In `test/source/harness_hunt_cli_test.cpp`, add the scenario
  `the-companion-pass-adds-no-skipped-row`: a `--hunt=` run answers only the
  `hunt: skipped` rows of its own plan, because the pass reaches the runner with
  the handoff of `source/harness/detail/internal.hpp:326-330` clear, so
  `source/harness/runner.cpp:3076` and `:3112` record nothing
  (`source/harness/hunt.cpp:1077-1088`, FR-036).

- [X] T080 [US3] In `test/source/harness_hunt_cli_test.cpp`, add the scenario
  `a-hunter-document-carries-no-topdown-object`: on the shape of `:1269-1272`, a
  run with `--hunt=` and `--json=` writes a document that holds no `topdown` key,
  the key `source/harness/json_report.cpp:867-869` writes only from
  `BenchmarkResult::topdown` (FR-036, D-11).

- [X] T081 [US3] In `source/harness/detail/internal.hpp`, give `RunOptions` one
  internal bit beside `topdownLevel` at `:751`. It names the level-1 ask of the
  companion, stands empty for every other run, and moves no header under
  `include/speedgun-ng/` (FR-035, D-9).

- [X] T082 [US3] In `source/harness/cli.cpp`, widen the guard of
  `printTopdownRefusal` at `:303` from `topdownLevel` alone to that bit as well,
  build its selection through the gatherer, and set the bit for a hunter run on
  the record the standing call at `:1583` reads. Keep the coverage line at
  `:1584-1593` gated on `topdownLevel` alone, so a hunter run prints one refusal
  line and no coverage line (FR-017, FR-035).

- [X] T083 [US3] In `source/harness/hunt_topdown.cpp`, define the companion
  collector and declare it in `source/harness/detail/hunt_topdown.hpp`: one
  `Runner` over a fresh `RunOptions` that carries only the level-1 selection,
  `repetitions` 1, `fixedIterations` 1 and `dryRun`, the shape of
  `source/harness/hunt.cpp:420-427`. That record never reaches the process record
  `source/harness/cli.cpp:756` fills and `:1505` hands to the runner, so the run
  gains neither line of `FR-035` (D-9, FR-033). No `CMakeLists.txt` line is
  needed: the harness target globs `source/harness/*.cpp` at
  `CMakeLists.txt:279-285`.

- [X] T084 [US3] In `source/harness/cli.cpp`, call the collector in the walk
  directly after the hunt answers at `:1774-1784`, and only where the ask
  answered a tree. `source/harness/hunt.cpp:1070` clears the handoff at the end of
  every stage, so that run reaches the runner with no hunt plan (FR-036, D-9).

- [X] T085 [US3] In `source/harness/cli.cpp`, carry the answer to the console on
  both routes: one field on `HeldRow` at `:1522-1538`, and one vector beside
  `entryHuntRecords` at `:1550`, filled at `:1830` and `:1902` and read at
  `:2040`. The immediate print at `:1828` and the deferred print at `:2038` then
  read one block (D-9).

- [X] T086 [US3] In `source/harness/detail/internal.hpp`, add a fourth defaulted
  `const std::optional<sg::TopdownRecord>&` parameter to the `printResult`
  declaration at `:1342-1345`, the third internal carrier beside `run` and
  `graph`. Say in its comment that it reaches the console alone and writes no
  document (FR-033, FR-036).

- [X] T087 [US3] In `source/harness/report.cpp`, split `printTopdownBlock` at
  `:344-442` into a body that reads a `sg::TopdownRecord` and a pass count. Keep
  the wrapper of that name forwarding `*result.topdown` with `result.passCount`
  (`include/speedgun-ng/benchmark.hpp:936`), so every rule of specs/042 stays in
  one body (FR-033).

- [X] T088 [US3] In `source/harness/report.cpp`, take the fourth carrier in the
  `printResult` definition at `:578-580`. Suppress the standing call at `:765`
  where the hunt lines of that benchmark print, and call the printer for the
  carrier after `printHuntSkips` returns at `:815`, inside the arm at `:812-816`.
  Forward the companion's own pass count, so the header at `:365-368` states the
  passes of the collection (FR-016, FR-033, FR-035).

- [X] T089 [US3] Prove the bytes that must stand, with
  the command `ctest --test-dir build -R 'harness_hunt_report_test|harness_hunt_json_test|harness_hunt_cli_test' --output-on-failure`
  answers them: `plan-line-shape` at `:503`, `skipped-list-line` at
  `:1162`, `plan-line-budget-cut` at `:1271` and `plan-line-mux-groups` at `:1279`
  in `test/source/harness_hunt_report_test.cpp`; `no-hunt-bytes` at `:2065` over
  the capture `hunt_no_hunt_console.txt` at `:396` of
  `test/source/harness_hunt_json_test.cpp`; and the new run rows of
  `test/source/harness_hunt_cli_test.cpp`. The mask of
  `contracts/console-cell.md:164-168` covers the standing bytes (FR-015, FR-018,
  D-12).

- [X] T090 [US3] In `docs/pages/harness.md`, in the same subsection, state the
  level-1 companion: a hunter run also prints one level-1 top-down block after
  the hunt block, once per benchmark, in the shape `## Top-down analysis`
  (:1683) already teaches. The companion answers from its own ask, and
  the deep tree the mapper reads (FR-024, FR-033; US3 AC of spec.md).
## Phase 6: User Story 4 - Same column on AMD (Priority: P3)

- Synthetic identities and planted tables only. No task here claims a host (FR-030).

## Phase 6: User Story 4 - Same column on AMD (Priority: P3)

- Synthetic identities and planted tables only. No task here claims a host (FR-030).

- [X] T091 [US4] Plant `zen-four-answers-the-amd-vendor-stems` in
  `test/source/counters_topdown_groups_test.cpp`: the identity `AuthenticAMD`,
  family 25, model 17, no stepping selects `arch/x86/amdzen4/`, answers
  575 pairs from the 577 rows of that directory, and answers no
  `skx-metrics` pair, the Intel-only stem of the acceptance host (FR-029).

- [X] T092 [US4] Plant `zen-five-answers-no-intel-file-stem` in
  `test/source/counters_topdown_groups_test.cpp`: the identity `AuthenticAMD`,
  family 26, model 17, no stepping selects `arch/x86/amdzen5/`, answers
  `l2_request_g1.group2` with `l2-cache`, and answers no pair whose group reads
  `cache`, because that directory carries no `cache.json` (D-3). Measured over the
  pinned directory: 12 files, 656 rows, no `cache.json` and no `cache` stem.

- [X] T093 [US4] Plant `a-refused-directory-still-groups-its-rows` in
  `test/source/counters_topdown_groups_test.cpp`: the identity `AuthenticAMD`,
  family 25, model 1, no stepping selects `arch/x86/amdzen3/` and answers 256
  pairs, beside the `NO_TOPDOWN_ROWS` refusal `topdownTree` answers at
  `source/counters/linux_pmu/topdown_select.cpp:470-475`. The refusal is a tree
  answer, so it empties no listing, and the mapper answers `unmapped` ahead of any
  listing read (FR-005, D-5).

- [X] T094 [US4] Plant `an-arm-identity-answers-no-pair` in
  `test/source/counters_topdown_groups_test.cpp`: the `MIDR` `kHandedMidr` of
  `test/source/counters_topdown_select_test.cpp:142` beside four fields no
  mapping-file row matches answers an empty listing, because no row of the 46 data
  rows of the embedded `arch/x86/mapfile.csv` names a vendor other than
  `GenuineIntel` or `AuthenticAMD` (D-5, D-16). Plant
  `a-handed-midr-changes-no-listing` beside it: the same `MIDR` beside the
  Skylake-X four fields answers the whole Skylake-X listing, because the accessor
  reads no `MIDR` beyond the directory and no mapping-file row reads either field
  (D-16, ruling R2, `include/speedgun-ng/counters_topdown.hpp:191-194`). The tree
  answers its Arm refusal in the same case, which is where `unmapped` comes from.

- [X] T095 [US4] Plant `one-name-answers-one-pair` in
  `test/source/counters_topdown_groups_test.cpp`: the identity `GenuineIntel`,
  6, 198, 0 selects `arch/x86/arrowlake/`, `DL1.DIRTY_EVICTION` answers `cache`
  once, and no name stands twice among the 919 pairs the answer holds over the
  1,166 rows of that directory. Measured at
  `external/pmu-events/arch/x86/arrowlake/cache.json`: the second and the third of
  its 214 rows carry that name, one under `cpu_atom` and one under `cpu_lowpower`,
  the named case of D-16 and FR-031. Plant the non-hybrid shape in the same scenario: the Zen 4 identity
  answers `umc_cas_cmd_rate` with `recommended` once, because
  `external/pmu-events/arch/x86/amdzen4/recommended.json` repeats
  `umc_cas_cmd_rate` and `umc_cas_cmd_read_ratio` inside one file with no `Unit`
  device, so the dedup arm needs no hybrid host (FR-031).

- [X] T096 [US4] Plant `zen-five-area-answers-the-vendor-names` in
  `test/source/harness_hunt_topdown_test.cpp`: `plantedTable` over
  `topdownZen5Rows()` of `test/source/detail/topdown_fixture.hpp`, which stands in the
  base fixture and reaches no task today, with the Zen 4 identity of the standing
  helpers. The case names the Zen 5 spelling of the fallback group, `l2-cache`, where
  the Zen 4 case names its own, so SC-001's per-identity claim covers two AMD
  generations and not one (SC-001, D-3, AP-7). Prove red before the arm lands, then with
  `ctest --test-dir build -R harness_hunt_topdown_test --output-on-failure`.

- [X] T097 [US4] Plant `an-amd-node-names-an-amd-path` in
  `test/source/harness_hunt_topdown_test.cpp`: the Zen 4 identity beside a planted
  `TopdownTable` from `topdownZen4Rows()` and `topdownTableOf()` of
  `test/source/detail/topdown_fixture.hpp:721` and `:750` settles a path whose
  every name is an AMD name, because an identity alone answers zero nodes and the
  refusals on this Intel host (FR-029, US4 scenario 1).

- [X] T098 [US4] Read the lane green with
  `ctest --test-dir build -R 'counters_topdown_groups_test|harness_hunt_topdown_test' --output-on-failure`
  and the aggregate `tools/ci/prepush.sh --full`, and record that the suite answers
  one more entry than the 178 of AP-1 with the three AP-1 failures unchanged
  (FR-030).

- [X] T099 [US4] In `docs/pages/harness.md`, in the same subsection, state the
  AMD spelling: one name, such as `backend_bound_by_memory`, from the table of
  the vendor directory the run reads, and no Intel name on an AMD part
  (FR-024; US4; the table itself is taught at `## Top-down analysis` :1683).
## Phase 7: Polish and gates

- One task per gate, run over the whole head, then the acceptance report at
  `specs/046-hunter-topdown/results/pr-body.md`.

## Phase 7: Polish and gates

- One task per gate, run over the whole head, then the acceptance report at
  `specs/046-hunter-topdown/results/pr-body.md`.

- [X] T100 Capture the base readings the polish deltas compare against, so a fresh
  checkout answers them without this session's files. In a clean worktree of `b1dd7670`,
  run the analyzer block of PLAN whole, with `findings.txt` written to
  `/tmp/046-logs/findings-base.txt`, then `ctest --test-dir build/dev -N` to
  `/tmp/046-logs/ctest-baseline.log`, then the labeled gcovr command of PLAN with
  `--txt /tmp/046-logs/cov-base.txt --json /tmp/046-logs/cov-base.json`. Read the three
  files back and state their counts, because T102, T108 and T111 compare against them
  (SC-004, SC-005, SC-006).
  The base answers all three files. The analyzer list holds 20,862 findings and 0
  naming findings. `ctest -N` lists 173 entries. The labeled gcovr run reads 19,895
  lines with 19,441 executed, 97 percent. Two host facts stand on the road. A clean
  worktree needs `submodule update --init --recursive` with
  `protocol.file.allow=always` before its configure. `/tmp` is a 16 GiB tmpfs. The
  instrumented base
  tree therefore lives at `$HOME/sgng-046-cov/coverage`, because its link
  failed in `/tmp` for want of space. That first capture stands void as
  `cov-base.void-buildfailed.txt`.

- [X] T101 In `docs/pages/harness.md`, quote the example. Add the marked code
  region marker for `example/docs/hunt_topdown.cpp#hunt-topdown`, and the recorded
  output marker for `example/docs/hunt_topdown.expected#report`, in the
  `## Hunter mode` subsection. The recorded block takes the fenced shape that
  :2269 already carries. The drift gate then binds each block to its file:

  ```text
  python3 tools/docs/example_drift.py --sections --names --options --forbidden .
  ```

- [X] T102 Run the suite and name its failures: build at the head with
  `cmake --preset=ci-rocky; cmake --build build -j24; ctest --test-dir build --output-on-failure`
  (PLAN) and answer 3 failures over the entry list, the AP-1 trio
  `counters_kernel_calls_test`, `counters_pmu_provider_test`,
  `harness_jitter_window_test`, with no new name (SC-005, AP-1); then the
  two-cpu lane `taskset -c 0,1 ctest --preset prepush-quick -j 2` (FR-030,
  AP-11), which answers the same three names and no other at the base; write
  both head lists to `/tmp/046-logs/ctest-head.log` beside the base list
  `/tmp/046-logs/ctest-baseline.log`.
  The head answers 178 of 181 entries in 2126 s, and its failures are the AP-1 trio
  with no other name. The entry list reads 181 at the head and 173 at the base, so
  AP-1's 178 matches neither. The two-cpu lane needs a build of `build/prepush`
  first, and the task text omits that step. An unrebuilt tree carries the old version
  string, and `views_test` failed on it. Rebuilt, the head lane fails 4 of 174. The
  trio stands there, beside `prose_gate_fixtures`, whose `timing.whole-tree` unit
  read `wall:10.671s` against a 10.0 s bound and passes when run alone. The base lane
  under the same taskset fails 4 of 171: the trio and `example_run`, which answers
  `docs_topdown registered bmTopdown but printed no row`. The claim that the base
  two-cpu lane answers the trio alone does not hold on this host. Both fourth names
  ride the record.

- [X] T103 [P] Run the format gate at the head: `cmake -P cmake/lint.cmake`
  (PLAN, SC-006) answers 0 findings on the trees the change wrote (AP-2).

- [X] T104 [P] Run the spell gate at the head: `cmake -P cmake/spell.cmake`
  (PLAN, SC-006) answers 0 findings (AP-2).

- [X] T105 [P] Run the docs drift gate at the head:
  `python3 tools/docs/example_drift.py --sections --names --options --forbidden .`
  (SC-006) answers 0: the page cap, the section caps, the example ownership,
  the quick-reference name of `hunt.hpp`, the option table and the forbidden
  words (AP-2, FR-026); the bare invocation checks neither the page budget nor
  a quick-reference row, so the flags are the gate.

- [X] T106 [P] Run the prose gate in the three forms AP-2b names, copied from
  R5 section 5 and PLAN:
  ```sh
  python3 tools/prose/prose_gate.py --base b1dd7670
  python3 tools/prose/prose_gate.py --check commit --base <sha> --head <sha>
  ```
  with the second form run once per commit of the range, and the tree form from
  PLAN verbatim: `cmake -DPROSE_MODE=tree -DPYTHON_COMMAND=/usr/bin/python3 -P cmake/prose-lint.cmake`
  (defs before `-P`). A range run at the branch point is void; the range reads
  `--base b1dd7670` once commits exist. For the two new example files, which
  the tree form reads only once tracked, use the brief's form: `git add -N
  <path>`, then `/usr/bin/python3 tools/prose/prose_gate.py --check prose --mode
  tree --paths <path>`, then `git reset -q <path>`. All three forms answer 0
  findings (FR-032, SC-006, AP-2b).

- [X] T107 Run both DBC hand gates with `/usr/bin/python3`, copied whole from
  R5 section 5 (the `dbc-gate` target cached a python with no PyYAML; the
  Doxyfile stands after any configure, `cmake/dbc-gate.cmake:14`):
  ```sh
  doxygen build/dbc-gate/Doxyfile
  /usr/bin/python3 tools/dbc/dbc_doc_gate.py --xml build/dbc-gate/xml \
  --registry tools/dbc/macros.yaml --out /tmp/046-logs/doc-matrix.json
  /usr/bin/python3 tools/dbc/dbc_pair_gate.py --src include/speedgun-ng \
  --src2 source tools --registry tools/dbc/macros.yaml \
  --doc-matrix /tmp/046-logs/doc-matrix.json \
  --out /tmp/046-logs/pair-matrix.json
  ```
  The head answers 347 interfaces at both gates with 0 gaps, where the base read
  345 (SC-006, AP-2); `--xml` names the directory `build/dbc-gate/xml`, not
  `index.xml`.

- [X] T108 Run the analyzer lane in a fresh tree only, never over `build`,
  copied whole from PLAN:
  ```sh
  cmake -S . -B build/dev --preset=ci-ubuntu
  cmake --build build/dev -j"$(nproc)" 2>&1 | tee /tmp/046-logs/dev-build.log
  log=/tmp/046-logs/dev-build.log
  { sed -nE 's#^([^:]+):([0-9]+):[0-9]+: (warning|error): .* \[([^],]+).*#\1:\2:\4#p' "$log"
  sed -nE 's#^\[([^:]+):([0-9]+)(:[0-9]+)?\]: \((warning|error)\).* \[([^],]+).*#\1:\2:\5#p' "$log"; } | LC_ALL=C sort -u > /tmp/046-logs/findings.txt
  grep -c ':readability-identifier-naming$' /tmp/046-logs/findings.txt   # must be 0
  ```
  and diff `/tmp/046-logs/findings.txt` against `/tmp/046-logs/findings-base.txt`
  (20,862 distinct findings, 0 naming at the base): the naming count reads 0 and
  the diff names no new finding of any check (brief ruling, Principle VIII); the
  lane reads the base only from `build/dev`.

- [X] T109 Run the aggregate local gate, copied whole from PLAN:
  ```sh
  tools/ci/prepush.sh --self-check      # the gate proves its own fault paths
  tools/ci/prepush.sh --full            # one timed line per step, exit 0 or 1
  ```
  Both answer exit 0 at the head; the prose step of `--full` reads VOID at a
  branch point and rides the range rule of AP-2b; `--self-check` proves the
  fault paths of the gate itself.
  The self-check answers exit 0. `missing tool`, `formatter version`, `failing step` and
  `validate step` each read pass. The `--full` run answers exit 8, and every gate step of
  it passes. Drift reads 0 s, format 36 s, time-source 2 s, spell 5 s, and prose 2 s over
  70 sources with 0 findings. Configure reads 2 s and build 16 s. The prose step reads 0
  findings where this row expected VOID at a branch point. The quick tier at full affinity
  then fails 4 of 174: the AP-1 trio and `counters_overhead`. Its line reads `published
  floor median 108.0 ns against 72.0 ns measured` for one `rdpmc` read taken while 24
  entries ran together. The claim that both commands answer exit 0 at the head does not
  hold on this host, because the trio stands red at the base as well.

- [X] T110 Run the sanitizer lane apart from prepush, as PLAN states it:
  `cmake --preset=ci-sanitize`, build, then `ctest --test-dir build/sanitize`
  (PLAN); green save the AP-1 trio only where a base failure reproduces, and
  the report carries what it answered.
  The lane configures and builds at exit 0. `ctest --test-dir build/sanitize` answers 11
  failures of 176 in 3,882 s. Three are the AP-1 trio, and one is `prose_gate_fixtures`.
  Six pass their ceilings under instrumentation: `harness_cli_test`,
  `harness_cli_sources_test`, `base_cli_matrix`, `harness_jitter_cli_test`,
  `harness_hunt_cli_test` and `example_run`. The eleventh name was
  `harness_hunt_topdown_test`, which this branch writes. Its case counts the pages the
  process holds before three calls and after them. The allocator of the sanitizer maps
  pages as it needs them, so that count moved on calls that read nothing. The claim now
  stands outside sanitizer builds, and `ctest --test-dir build/sanitize --rerun-failed`
  answers 10 failures of 11 with the mapper unit gone from the list. The thread sanitizer
  lane of `cmake --preset=ci-tsan` builds at exit 0 and answers 10 failures of 177 beside
  those names. It carries `counters_topdown_groups_test` and `report_shape_fwq_ftq_sample`
  in place of two of them. The groups unit carried the same mapping claim, stands fixed
  the same way, and answers exit 0 in that tree. `report_shape_fwq_ftq_sample` runs
  `example/docs_fwq_ftq`, and specs/042 records that probe race under this lane here. The
  row's condition that only the trio stand red holds once the two mapping claims move out
  of sanitizer builds; the six ceilings and the two host names stand red at the base too.

- [X] T111 Run the coverage lane, copied whole from PLAN:
  ```sh
  cmake --preset=ci-coverage && cmake --build build/coverage -j"$(nproc)"
  ctest --test-dir build/coverage -j 1 --output-on-failure --no-tests=error
  cmake --build build/coverage -t coverage      # needs lcov: broken on this box
  ```
  On this host `tools/dbc/coverage_gate.sh` answers exit 3, the tool-failure
  exit, and states no ratio (AP-6); the ratio of record comes from the coverage
  job of the pull request (SC-004, SC-003). Then the labeled second opinion,
  copied whole from PLAN with only the two output paths moved to
  `/tmp/046-logs/cov-head.txt` and `/tmp/046-logs/cov-head.json` so the base
  readings stand, all other bytes as written:
  ```sh
  $HOME/.prime/agent/kernel-venv/bin/gcovr -r . build/coverage -j"$(nproc)" \
  --filter 'include/speedgun-ng/' --filter 'source/' --filter 'tools/hostcheck/' \
  --exclude-throw-branches --exclude-unreachable-branches \
  --gcov-ignore-parse-errors suspicious_hits.warn_once_per_file \
  --exclude-lines-by-pattern 'SG_(REQUIRE|ENSURE|INVARIANT|ASSERT)(_ALWAYS)?' \
  --exclude-lines-by-pattern 'if \(cpus\.empty\(\)\) \{' \
  --txt /tmp/046-logs/cov-head.txt --json /tmp/046-logs/cov-head.json
  ```
  The files 046 adds read 0 missing lines and about 0 missing branches against
  that command (PLAN); the number is a delta over identical commands, never the
  gate ratio; the run carries no `LCOV_EXCL`, no `--omit-lines`, no
  `--ignore-errors`, no `--remove`. State in the record the line and branch figures this
  command answers, and the fact that Principle VI asks 100 percent branch while
  `tools/dbc/coverage_gate.sh:102` carries `allowance=21` at the base, which specs/031,
  specs/035 and specs/043 cite: the change adds no new shortfall, and the divergence
  predates it (FR-031, Governance).
  The `lcov` tool stands repaired on this host. It died on a perl XS version
  mismatch, and `lcov --version` now answers `LCOV version 2.5.0-beta`. The gate
  therefore answers ratios where the task text expected exit 3. The head reads 98.9
  percent lines, 17,519 of 17,718, and 97.7 percent branches, 8,579 of 8,779. It
  exits 2 with `coverage-gate: line coverage is not 100%`. The base answers 98.6
  percent lines, 17,308 of 17,559, and 97.2 percent branches, 8,461 of 8,701, and
  exits 2 the same way. Both ends leave more branches dark than `allowance=21`
  allows: 200 at the head and 240 at the base, where Principle VI asks 100 percent.
  The branch adds no new shortfall. The head leaves 200 lines dark where the base left
  251, and `source/harness/hunt_topdown.cpp` reads 100 percent lines there, beside
  `hunt.cpp` and `report.cpp` at 100 percent. Two host facts rode the lane. An
  instrumented tree needs its counters zeroed before its suite, because stale counters
  print `libgcov profiling error` lines into exact-output CLI units, and the task text
  omits that step. Four units, `base_cli_matrix`, `harness_jitter_cli_test`,
  `harness_hunt_cli_test` and `example_run`, pass their ceilings under instrumentation
  here at both ends, so their counters never dump. The labeled second opinion reads
  20,072 lines with 19,666 executed at the head, 98 percent, over 83 files. The base
  reads 19,895 with 19,441, 97 percent, over 82 files. Its json carries branches:
  15,342 of 16,785 at the head against 15,116 of 16,592 at the base. That view counts
  lines the gate omits, so it reads the mapper file at 108 of 113. Its four dark lines
  are three closing braces and the first line of one assertion. One added line of
  `source/harness/cli.cpp` reads dark there too, the companion slot at 1941, which the
  four timed-out units would cover.

- [X] T112 [US1] Run the acceptance hunt on this host and record what it answers:
  `./build/example/memory_walk --hunt --timeout 120`, then
  `./build/example/first_benchmark --hunt --timeout 120`. Read the level-1 block the run
  prints and the three counts of the ranked rows, node-mapped, list-group and unmapped,
  and write them with the host identity to
  `specs/046-hunter-topdown/results/acceptance-hunt.md` for the report task to quote.
  This is the only task that measures SC-007 on hardware (SC-007, AP-9, FR-011).

- [X] T113 [P] Write the acceptance report
  `specs/046-hunter-topdown/results/pr-body.md` (the precedent
  `specs/042-topdown/results/pr-body.md` read at the base and confirmed by the
  orchestrator; the spec names the acceptance report but no path), stating the
  measured numbers: the DBC counts
  345 at the base and 347 at the head with 0 gaps (SC-006); both ctest lists and
  both two-cpu lists naming the AP-1 trio and no new name (SC-005); the analyzer
  delta with 0 naming findings and no new finding of any check; the prose gate
  at 0 findings in all three forms (AP-2b); the coverage ratio of the pull
  request coverage job beside the local `coverage_gate.sh` exit 3 with its tool
  line and the labeled gcovr second opinion, with the figures it answers and the
  FR-031 divergence between Principle VI and `allowance=21` (AP-6, SC-004); the TSan
  row of the sanitizer lane and the `docs` target build, each naming the command that
  measured it (Principle VIII); the ranked-row bytes of one hunt base and head run of
  the same binary path, stating the pass count each carries and the row time it costs
  (FR-015); the three counts
  node-mapped, list-group and unmapped of the acceptance run against the AP-9
  figures 58, 412 and 27 (SC-007); the AP-10 refusal, so the reader knows a live
  before-and-after byte comparison proved nothing; the 503 ms first tree read
  the hunter run now pays (AP-12); and the pending rows for the hosts no lane
  holds (SC-009, SC-010).
  The report stands at the path the row names, on the shape of the specs/042 report. It
  states the DBC counts 345 and 347 with 0 gaps, the entry lists at 173 and 181, and both
  two-cpu lists with their fourth names. It states the analyzer delta from 20,862 to
  21,189 with 0 naming findings, and the prose gate at 0 findings in three forms. It
  states both coverage gate readings beside the labeled gcovr opinion, the sanitizer rows
  of both lanes with the command that measured each, and the docs row that stands red at
  both ends here. It carries the ranked-row bytes 62,142 and 76,105 with
  `events=497 passes=249` on each side, the AP-9 counts 58, 412 and 27, the AP-10
  refusal, the 503 ms first table read, and the rows SC-009 and SC-010 leave pending.
## Dependencies

- Phase 2 blocks every story. The ABI commit stands first, because the member,
  the accessor and the numbers move together (FR-028).
- Phase 3 needs the member and the seam. Phase 4 needs the member and the stamp,
  because an unstamped record writes no key (FR-019).
- Phase 5 needs the seam and the accessor of D-16. Phase 6 needs the accessor and
  the index of FR-037.
- Phase 7 runs after every phase, and its report states what each machine
  answered.

Shared files, in the order the phases need them: `test/CMakeLists.txt` (three
registrations, disjoint hunks), `docs/pages/harness.md` (four sections, disjoint),
`source/harness/detail/internal.hpp` (the seam declaration, then the companion bit
and the `printResult` declaration), `source/harness/hunt_topdown.cpp` (the mapper
bodies, then the collector, then the index), `README.md` (one row, after the member
lands). `source/harness/report.cpp` and `source/harness/cli.cpp` stand with one
lane each.

## Dependencies

- Phase 2 blocks every story. The ABI commit stands first, because the member,
  the accessor and the numbers move together (FR-028).
- Phase 3 needs the member and the seam. Phase 4 needs the member and the stamp,
  because an unstamped record writes no key (FR-019).
- Phase 5 needs the seam and the accessor of D-16. Phase 6 needs the accessor, and
  the index of FR-037 lands with the mapper in Phase 3, so Phase 6 only plants the
  identities that exercise it.
- Phase 7 runs after every phase, and its report states what each machine
  answered.

Shared files, in the order the phases need them: `test/CMakeLists.txt` (three
registrations, disjoint hunks), `docs/pages/harness.md` (seven tasks, four sections,
three of them one subsection, disjoint hunks in commit order),
`source/harness/detail/internal.hpp` (the seam declaration, then the companion bit
and the `printResult` declaration), `source/harness/hunt_topdown.cpp` (the index, then
the mapper bodies, then the collector), `README.md` (one row, after the member
lands). `source/harness/report.cpp` and `source/harness/cli.cpp` stand with one
lane each.

## Parallel execution

Within Phase 2, the mapper unit skeleton task and the counters accessor tasks touch
different files, so they run together. Within Phase 3, the prose task and the DBC
task run with anything. The `[P]` marks state what the file sets allow. Two tasks
that name one file never run together, even when they name different functions of
it.

## MVP scope

User Story 1 alone: the mapper, the stamp and the area cell. It answers the
question the request opens with, and it needs no document change, no companion pass and
no example. The example and its machines ride the tail of Phase 3 in the example commit,
so they land beside the story they show and stay outside this cut.

## Implementation strategy

1. Land Phase 2, then the ABI commit, and read both DBC hand gates at 347.
2. Land User Story 1, run its unit, and read the cell on a live hunt
   (`./build/example/memory_walk --hunt`).
3. Land User Stories 2, 3 and 4 in that order. Each story keeps the no-hunt bytes
   unchanged and adds no name to the two-cpu failure list.
4. Run the polish phase, then write the acceptance report at
   `specs/046-hunter-topdown/results/pr-body.md` with every number this file
   states as measured.


## Phase 8: Post-merge fixes (owner ruling, 2026-11-05)

**Base**: `858c55d9`, the merged head of pull requests #78 and #79: version
`0.31.0`, schema `1.14`, harness SOVERSION 21, archive SOVERSION 6. The
branch is `fix/post-merge-78-79`, cut from `origin/master`, and it carries the
spec 045 post-merge rows beside this one. No version, schema or SOVERSION
change. No CI or gate change.

- [ ] PM46-001 Replace each `/home/<user>/` path in this file with `$HOME/` or
      with a path relative to the build tree. The check reads the three lanes
      that hold post-merge records:

      ```sh
      git grep -nE '/home/[a-z]' -- specs/045-merge-fixes \
        specs/046-hunter-topdown specs/047-loader-cross-field
      ```

      The command prints nothing.

## Phase 9: Master acceptance (AMD)

- Records only. ZA-001 to ZA-005 measure SC-009 on the AMD Zen 5 host and write
  what it answered. No source, test, CMake, CI or gate file rides this phase, and
  no new spec number joins. The base is `858c55d9`, dated 2026-11-05: version
  `0.31.0`, schema `1.14`, harness `SOVERSION` 21, archive `SOVERSION` 6, 84
  `LCOV_EXCL` markers, `README.md` 1001 lines.

- [X] ZA-001 Build master in Release and run the hunt commands of
  `results/acceptance-hunt.md` three times each, with no option change:
  `cmake --preset=ci-rocky`, `cmake --build build`,
  `./build/example/memory_walk --hunt --hunt-budget 120s` and
  `build/example/first_benchmark --hunt --hunt-budget 120s`. Record the ranked
  rows, the top-down area of each ranked event, the level-1 block, and the count
  of unmapped events (SC-007, SC-009).
  The build answered exit 0 and 181 registered entries. All six hunt runs answered
  exit 134 with `the ratio pair is written together or left together (FR-004)` at
  `source/counters/fold.cpp:166`, and printed no plan line, no ranked row, no area
  cell, no block and no count. The same two benchmarks ran again with
  `--metric-method multiplex`: six runs, exit 0, 730 lines each, `hunt: plan
  events=353 passes=89 method=MULTIPLEX`, and the same three row counts in every
  run. `results/acceptance-amd-zen5.md` carries both readings.

- [X] ZA-002 Check SC-009: each area names a node of the selected AMD (Zen 5)
  table and no Intel node. Where an event carries no area, give its name and the
  reason the tool prints. If SC-009 fails, record the failure and stop; fix no
  code here.
  The run's own line names the selected table `arch/x86/amdzen5/`
  (`reference=linux-7.2.4`) on the family 26, model 68 part. Of 353 ranked rows,
  11 name a node of that table (`topdown.bad_speculation >
  topdown.amd.bad_speculation_from_mispredicts` 5, `topdown.backend_bound >
  topdown.amd.backend_bound_by_memory` 3, `topdown.frontend_bound` 1,
  `topdown.retiring > topdown.amd.retiring_from_fastpath` 1,
  `topdown.smt_contention` 1), 334 name a group of the listing, and 8 answer
  `unmapped`. No cell names an Intel node, and the string `intel` appears 0 times
  in the six logs. SC-009 therefore stands measured for the run that completes,
  and the abort of the command as `results/acceptance-hunt.md` writes it stands as
  its own open row: the record shows the abort at `paranoid` 2 and at `-1`, the
  1129 opens `strace` listed, and the `linkRatioSlots` arm pair that leaves half a
  ratio pair. AMD Zen 4 stays a pending row.

- [X] ZA-003 Run the full suite and the two-cpu suite, and name every failing
  entry: `ctest --test-dir build --output-on-failure` and
  `taskset -c 0,1 ctest --test-dir build --output-on-failure`.
  Both rows answered 181 entries, 180 passed, 0 failed, 1 reported skipped,
  308.12 s and 257.39 s. The skipped entry is `harness_topdown_run_test`, which
  exits with the registration's skip code after one of its 27 scenarios skips on
  its own host-identity guard; its other 26 scenarios passed. No entry failed on
  either row.

- [X] ZA-004 Write `results/acceptance-amd-zen5.md` with the host, the CPU family
  and model, the kernel, the compiler, the master sha,
  `kernel.perf_event_paranoid`, the commands and every figure above; use
  `$HOME/` or build-relative paths, and name no person. Mark SC-009 of `spec.md`
  measured on AMD Zen 5 with a link here, and keep Zen 4 pending. Keep SC-010
  (Arm, Intel hybrid, `PERF_METRICS` parts) pending.
  Done: the record stands at `results/acceptance-amd-zen5.md` (184 lines), and
  `spec.md` names SC-009 measured on AMD Zen 5 with the link and the option the
  host needed. Zen 4, Arm, Intel hybrid and `PERF_METRICS` rows stand pending.

- [X] ZA-005 Tick ZA-001 to ZA-005, clear the gates over the head, and squash the
  lane into one commit before the pull request (ruling G-23).
  Cleared over the head: `cmake -P cmake/lint.cmake`,
  `cmake -P cmake/spell.cmake`, `tools/docs/example_drift.py`,
  `tools/prose/prose_gate.py --base origin/master`, and
  `tools/dbc/dbc_pair_gate.py`. The pull request body gives the host, the kernel,
  the counts and the head sha (ruling G-20), and the lane merges nothing.
