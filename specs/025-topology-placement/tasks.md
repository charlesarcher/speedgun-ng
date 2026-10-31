# Tasks: Topology Placement

**Input**: Design documents from `specs/025-topology-placement/`

**Prerequisites**: plan.md, spec.md (FR-001 to FR-036, SC-01 to SC-12,
Clarifications Q-1 to Q-8), research.md (R-01 to R-13), data-model.md,
contracts/ (four files), quickstart.md

**Tests**: Included. Principle III's TDD mode, recorded in plan.md, puts
each story's test task before the code task it gates and states the red
run it observes; Principle VI fixes 100 percent line and branch coverage
with no exclusion marker on those same paths. No test sleeps, reads a
clock, needs a privilege, performs a
real pin, or depends on the shape of the host beyond the one host read of
T007.

**Organization**: Phase 1 names the seams and the data model every story
compiles against. Phases 2 to 6 carry the five user stories in priority
order. Phase 6 carries the third page-cap raise, and Phase 7 carries the
gates, the version line, and the rebase. Every task names exact file paths, and every bound
from data-model.md, contracts/, and research.md is quoted so nothing is
left to implementation-time discretion.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies); the `add_executable`/`add_test` registration in `test/CMakeLists.txt` is the one shared edit among the test tasks, serialized by appending in task-id order
- **[Story]**: User story ownership (US1 to US5 from spec.md)
- Setup and Polish tasks carry no story label

## Phase 1: Setup (Seams and Data Model)

**Purpose**: The named types and internal seams every story compiles
against (FR-003, FR-006, FR-031). Declarations and default values only;
behavior arrives under TDD in the story phases.

**CRITICAL**: No story test compiles until T001 to T004 and T006 are
complete; T005's capture is the one Setup step that follows T010.

- [x] T001 [P] Declare the internal data model with no behavior in `source/harness/detail/internal.hpp`, per data-model.md E-01 to E-06: `LayoutThread {std::uint32_t cpuIndex; coreOrdinal; groupOrdinal}`, `LayoutCore {groupOrdinal; ordered threads}`, `LayoutGroup {std::uint32_t lowestCpu; ordered cores}`, `MachineLayout {ordered threads; ordered cores; ordered groups; packages; mask; readable}` with the validation rules quoted there (a thread names a core when cores
are described and a core names a group when L3 caches are described, a
description without either leaves those vectors empty, every group holds
at least one core, and when cores exist the thread count equals the sum of the sibling counts) (FR-001, FR-002, FR-003); `enum class Preset { NONE, CORE, L3, SMT, COMPACT }` (FR-007); `PlacementPlan {cpu vector; group vector; places}` with the postcondition that an empty plan means the context count exceeds the place count and otherwise `cpu.size() == contexts` with every entry distinct (FR-008, FR-009). No name reaches an installed header (FR-031)

- [x] T002 Give `RunOptions` in `source/harness/detail/internal.hpp:324`, data-model.md E-07, the fields `std::optional<std::string> placement` and `bool topologyMode = false`, and give `RegistryEntry` and `Instance` the placement carrier data-model.md names: the entry holds `std::optional<std::string>` with the last call winning, and the instance holds the resolved preset with the handle value winning over the command line (FR-015)

- [x] T003 [P] Declare `BenchmarkHandle::placement(std::string_view presetName) -> BenchmarkHandle&` in `include/speedgun-ng/benchmark.hpp` beside `threads(int)` with the standing setter `\pre` (the handle names a registered entry and the run has not started) and the `\post` (the entry carries the name in place of any earlier one, and the returned handle is this handle), the way `threads` documents its set replacement (FR-015, FR-030). No exported layout changes: one member function on an existing class, no new data member (FR-033)

- [x] T004 [P] Give `speedgun-ng_harness` in `CMakeLists.txt` the same private access to the vendored hwloc that `speedgun-ng_speedgun-ng` has at `CMakeLists.txt:66-94`: the vendored include directory and `$<BUILD_INTERFACE:hwloc_vendor>` linked `PRIVATE`, the imported target still unexported; its covering check is the `nm` proof
  of T031, which cannot run before T010 gives the harness archive an hwloc
  reference and has no red run available: without this link
  `source/harness/topology.cpp` does not compile, since `hwloc_vendor`
  reaches the library alone and stays `PRIVATE` at `CMakeLists.txt:94`, so the check is T031's
  extended proof in Phase 7, and what verifies this link inside Phase 2 is T010
  compiling against the exported include directory; the gate script's
  subject list gains the harness archive and `source/hwloc/hwloc_gate.cpp`
  stays untouched (R-01, Q-1, FR-031)

- [x] T005 Record the archive-shape evidence R-01 depends on: build both configurations, the static default and the shared one (`cmake --preset=ci-linux-audit -B build-shared -D BUILD_SHARED_LIBS=ON`, the shape `.github/workflows/ci.yml:262` builds), capture the `nm` evidence from the static tree only, the harness archive holding undefined `sg_hwloc_*` references with zero defined ones and an executable resolving them with the vendored archive on its link line: `add_library(speedgun-ng_harness` at `CMakeLists.txt:194` names no type, so the shared tree builds the harness as a shared object that resolves the vendored static archive at its own link time, and record separately that the shared tree configures, builds and installs with no harness archive present; append the measured output to the R-01 entry of `research.md`; the capture
  runs after T010 writes `source/harness/topology.cpp`, since no harness
  object carries an `sg_hwloc_*` reference before that unit exists

- [x] T006 Declare the internal seams in `source/harness/detail/internal.hpp`, data-model.md E-01 and E-10: `auto layout() -> const MachineLayout&` (one read per process, cached in one function-local static, so an install must precede the first read), `auto installTestLayout(std::optional<MachineLayout>) -> void` (an installed layout wins over the read, and an empty optional clears it), `auto installTestTopologyDescription(std::optional<std::string>) -> void` (an installed description makes the read call `hwloc_topology_set_synthetic`, and an empty optional clears it), `auto parsePreset(std::string_view) -> std::optional<Preset>`, `auto resolvePreset(Preset, const MachineLayout&, int contexts) -> PlacementPlan`, and `auto printTopologyListing(const MachineLayout&) -> void`, `auto placementText(const PlacementSlot&) -> std::string_view` (the `cpu@group` text of a slot the layout names a cpu for, the cpu index alone when the layout names no group for it, and `NO_PIN` for `kNoPin`; rendering only, no pin and no platform call, with the text in storage that outlives the call), and `auto printContext(bool placementInEffect) -> void` (the declaration at `source/harness/detail/internal.hpp:389` and its call at `source/harness/cli.cpp:369` move with the added parameter, because the selected set is local to `speedgunMain`) (FR-002, FR-006, FR-021, FR-036, Q-5, R-04)

**Checkpoint**: `cmake --preset=ci-rocky && cmake --build build` compiles
the named surface, and the story tests compile against it.

## Phase 2: User Story 1 - Run one benchmark per L3 group (Priority: P1) - MVP

**Goal**: A user names a preset, the harness reads the machine once, and
each context of a threaded run runs on one cpu of its own group (spec
US1).

**Independent Test**: "run a threaded benchmark with the `l3` preset on a
machine whose layout is injected with four groups, and read the cpu each
context got. Four contexts land on four different groups; a fifth
context stops the run." (spec.md US1)

### Tests for User Story 1 (write first, observe failure)

- [x] T007 [P] [US1] Create `test/source/harness_topology_test.cpp` and register `harness_topology_test` in `test/CMakeLists.txt` with the standing `add_executable`/`add_test` pair, the `source/harness` include directory the way `harness_gap_test` carries it at `test/CMakeLists.txt:544`, and `cxx_std_23`; scenarios: one real host read asserts the package and thread counts stand at 1 or above while the core and group counts stand at 0 or above with the thread ordinals unset when the host publishes neither (data-model.md), that every thread of the layout sits inside the process mask, and that groups, cores and threads are ordered by ascending cpu index (FR-001, FR-005, R-11, R-13); an installed layout wins and no host read happens (FR-006, Q-5); an installed layout of one group, four groups and siblings off each reports the counts the injection stated (SC-04); a mask narrowed in-process with `sched_setaffinity` lowers the counts to what the mask holds and the layout's mask equals the narrowed set (FR-005); an installed synthetic description `1` yields no core and no group and a thread count equal to the weight of the intersection of that description's cpu set with the process mask, the count the walk rule of T010 defines (derived from the walk rule: 1 under a mask holding cpu 0, 0 under `taskset -c 4-7`; the first green run of this test, once T010 exists, records the measurement, the way R-04 states), and a malformed description leaves `readable` false (FR-005, R-04, R-13); `layout()` called twice in one process yields the same layout, and an install after the first call changes nothing (FR-002, R-04). Observe red: the `harness_topology_test` target fails to link, an undefined reference to the seams T006 declares, before T010 exists. Each scenario that reads the layout or installs a seam, the host read, the narrowed-mask read and the three injected layouts among them, runs in a fresh process by the fork and execv pattern of `test/source/harness_context_barrier_test.cpp:116-126`, naming the scenario in `argv` and re-installing the seam in the child, because one cached read per process serves exactly one layout (T006, data-model.md)

- [x] T008 [P] [US1] Create `test/source/harness_placement_preset_test.cpp` and register `harness_placement_preset_test` in `test/CMakeLists.txt` with the standing pair, the `source/harness` include directory and `cxx_std_23`; scenarios over the injected layouts SC-04 names, fixed here: one group holding one core with no sibling, four groups holding 24 cores in total with two siblings each (48 threads), the same layout with siblings off, and a mask holding part of the machine (the place counts of `contracts/topology-placement.md`'s table apply to the second): `core` gives the lowest-index thread of each distinct core (FR-011), `l3` gives the lowest-index thread of each distinct group with groups ordered by their lowest thread (FR-012), `smt` gives the siblings of the core holding the lowest-index thread and no other core (FR-013), `compact` fills every sibling of a core before the next core and every core of a group before the next group (FR-014), and `none` produces no plan (FR-010); the place count of each preset equals the table for that four-group, 24-core, 48-thread layout: `l3` 4, `smt` 2, `core` 24, `compact` 48 (SC-02); two calls with one layout and one count return equal vectors (SC-03, FR-009); every plan holds pairwise distinct cpus (FR-008); `parsePreset` refuses `l2`, `0-3`, `0`, and an empty text (FR-016, FR-017); `placementText` over an injected layout whose thread names a group returns `cpu@group`, over an injected layout with no group for that cpu returns the cpu index alone, and for `kNoPin` returns `NO_PIN`, which is how the E-09 arms reach coverage without a real pin (E-09, FR-021). Observe red as a link failure against the resolver T011 defines

- [x] T009 [US1] Append the runner scenario to `test/source/harness_placement_preset_test.cpp`: with an installed layout and an installed `PlacementFn` replacement, a run of four contexts under `l3` calls the replacement with slots naming the four planned cpus in context-index order, and a run with no preset calls it with `noPin()` as feature 020 ships (FR-008, FR-010, FR-019, R-05). Observe red as a link failure against `BenchmarkHandle::placement`, which T003 declares and T013 defines; the plan carrier itself arrives with T002 and T013

### Tests carried forward from other stories (write first, observe failure)

- [x] T023 [US4] Append the three-shape comparison to
  `test/source/harness_placement_preset_test.cpp` in Phase 2, before
  `resolvePreset` of T011 exists: on a two-sibling layout, `smt` names the
  two siblings of one core with two contexts, `core` names two different
  cores with two contexts, and `compact` with three contexts names both
  siblings of one core plus the lowest thread of the next core (FR-011,
  FR-013, FR-014, US4 scenarios); `smt` with three contexts on that layout
  returns no plan (FR-018). Observe red: the resolver does not exist yet, so
  the scenarios fail to build beside T008, the same red run T007 and T008
  record as link failures (Principle III)
- [x] T019 [P] [US3] Create `test/source/harness_cli_placement_test.cpp` and register it in `test/CMakeLists.txt` with the standing pair, the `source/harness` include directory and `cxx_std_23`; scenarios: `--placement=l2` and `--placement=0-3` each print the placement line naming the rejected value and exit 2 with no benchmark run (FR-016, FR-017), and `--placement` with no argument keeps the standing `getopt_long` missing-argument path and exit status 2 (contracts/cli-placement.md); a handle preset wins over the command-line preset (FR-015), and a handle name the system does not provide prints one line naming the rejected text and exits with status 2 with no benchmark run (FR-016); an unknown flag keeps the standing line and status 2, the guard for behavior the audit-point tree already has. Observe red: each `--placement` value falls through to the generic `unknown option` line of `source/harness/cli.cpp` at 305 to 309, so the placement line these scenarios name does not exist, and no handle setter exists before T013 (Principle III)

### Implementation for User Story 1

- [x] T010 [US1] Implement `source/harness/topology.cpp`, the one harness translation unit that includes `<hwloc.h>` (FR-004, R-01): initialize the topology, load it, walk processing units, cores, L3 cache objects and packages restricted to the intersection of the `sched_getaffinity` mask and `hwloc_topology_get_topology_cpuset`, because hwloc's own sets follow the machine and the control group while the process affinity is a separate fact the harness reads itself, build `MachineLayout` with the ordering rules of data-model.md, cache the result in one function-local static so the read happens at most once per process on the first need, consulting both install seams before that read and never invalidating the cache, honor `installTestLayout` so an installed layout short-circuits the read, and honor `installTestTopologyDescription` so an installed description reaches `hwloc_topology_set_synthetic` and its return value folds into the readable decision (FR-001, FR-002, FR-005, FR-006). A failed read leaves `readable` false and no partial layout (FR-005). The walk stays branch-light: one loop over the processing units with a parent, core and L3 lookup per unit and no per-object-type switch tree, so the single host read reaches every branch the walk holds (Principle VI, FR-034). The read carries no plain guard on the `hwloc_topology_init` answer: no scenario can make it fail, since an address-space limit low enough to starve it kills the process in the dynamic loader before the call, and that answer rides a one-line contract macro whose line the extract omits with `--omit-lines` at `cmake/coverage.cmake:76`, the way `BenchmarkHandle::threads` at `source/harness/registry.cpp:526` carries its undrivable `m_entry != nullptr` answer. No guard stands ahead of `hwloc_topology_destroy` and none is needed: the enforce semantic of `cmake/dbc.cmake:24` sends a failing predicate through the `[[noreturn]]` dispatch of `include/speedgun-ng/dbc.hpp:198-214`, so the walk is never entered on a null topology and a guard there would hold a branch no scenario can reach. The readable decision stays one `rc` chain over the description and load answers, and T007's malformed-description scenario drives its false arm; record the three pinned-commit anchors of R-04 from the checked-out submodule and confirm the synthetic route through the vendored build, the measurement R-04 names (R-04, FR-005, FR-034, T032, Q-8)

- [x] T011 [US1] Implement `source/harness/placement_preset.cpp`: `parsePreset` matching the five names in full and nothing else, and `resolvePreset` as the pure walk of `contracts/topology-placement.md` with `SG_REQUIRE` on `contexts >= 1` and `SG_ENSURE` on pairwise distinct cpus, returning a plan whose `cpu` stands empty and whose `places` carries the count when `contexts` exceeds the place count (FR-007 to FR-014, R-03). The same file holds `placementText`, the rendering seam T006 declares, which composes the pair from the layout with no pin and no platform call (FR-021, E-09). No hwloc type appears in this file

- [x] T012 [US1] Wire `source/harness/runner.cpp:1246-1252`: when the instance carries a plan, build `ContextPolicy {WaitMode::SPIN, PlacementSlot::cpu(plan.cpu[position])}` per context, and keep `PlacementSlot::noPin()` when no plan stands (FR-008, FR-010, FR-019, D-1)

- [x] T013 [US1] Wire the handle option: define `BenchmarkHandle::placement` in `source/harness/registry.cpp` beside `BenchmarkHandle::threads` at line 524 of that file, pairing the `\pre` and `\post` T003 documents with `SG_REQUIRE` and `SG_ENSURE` the way `threads` pairs its own at `:526` and `:532`, because `tools/dbc/dbc_pair_gate.py` reports a documented precondition with no enforcement as a gap, and store the name on `RegistryEntry`, and copy it onto each `Instance` during `detail::expandRegistry()` the way the thread count set travels (FR-015); `expandRegistry()` parses each stored name with `parsePreset` as it copies it, and a name that parses to nothing prints one line naming the family and the rejected text and exits with status 2 before any instance runs, the handle half of FR-016 (FR-016, data-model.md E-08)

- [x] T014 [US1] Add `--placement` to the option table of `source/harness/cli.cpp` and store the raw text in `RunOptions::placement`; resolve it once before the first run so an unknown name stops with one line and status 2, and add the `--placement` row of the option table at `README.md:640` in the same commit with the `PAGE_LINE_CAP` raise at `tools/docs/example_drift.py:112` that row costs, because `example_drift` is a registered ctest at `test/CMakeLists.txt:905` and its option check at `tools/docs/example_drift.py:629-642` demands a row for every long option the parser carries (FR-015, FR-016, FR-028)

- [x] T022 [US1] Add the `--placement` row to the option table of
  `specs/015-benchmark-harness-core/contracts/cli.md` and the exit-status
  rows the parser adds, in the same commit as T014's parser, so the contract
  table never lists an option the parser lacks (FR-015, FR-016,
  contracts/cli-placement.md)

**Checkpoint (MVP)**: `--placement=l3` on a four-group layout places four
contexts on four groups, and the plan is visible through the installed
placement function (SC-01, SC-03).



## Phase 3: User Story 2 - Read where the contexts ran (Priority: P1)

**Goal**: The report shows the layout once and each context's `cpu@group`
beside its wait mode (spec US2).

**Independent Test**: "run a threaded benchmark with a preset on an
injected layout and read the topology line plus the per-context
placement fields." (spec.md US2)

### Tests for User Story 2 (write first, observe failure or pass on the shipped surface)

- [x] T015 [P] [US2] Create `test/source/harness_placement_report_test.cpp` and register it in `test/CMakeLists.txt` with the standing pair, the `source/harness` include directory and `cxx_std_23`, following the capture pattern of the existing harness report tests; scenarios: with a preset in effect the context block carries exactly one `topology: packages=1 l3-groups=4 cores=24 threads=48` line below the version line (FR-020, R-09); each context prints `ctxN=SPIN/<cpu>@<group>/applied` with the planned pair (FR-021), and the pair `4294967295@0` exceeds `kPlacementFieldWidth` and prints the cut mark (R-06, Risks); a report with no preset prints the block byte for byte as feature 020 ships it, `NO_PIN` included (FR-022, SC-05); a sibling pair from `smt` prints no noise warning (FR-023), and the oversubscribed run that never reaches the printer stands in T042, gated by the fit check of T020; the topology line and the disclosure read no clock (FR-024); an instance run with `--placement=none` prints no topology line and keeps the block byte for byte (FR-010, FR-022). Observe red before T016 and T017 exist for every scenario but the
  no-preset one: the byte-for-byte block of FR-022 and SC-05 passes on the
  shipped surface, the pair 020 records for its US3 tests, and it turns red
  only against a build that prints a topology line where no preset stands

### Tests carried forward from other stories (write first, observe failure)

- [x] T024 [US4] Append the disclosure comparison to
  `test/source/harness_placement_report_test.cpp` in Phase 3, before T016
  changes the printer: the `smt` pair prints two `cpu@group` fields naming
  one group and two cpus of one core, and the noise warning stays silent
  (FR-021, FR-023). Observe red: the scenario fails against the bytes feature
  020 ships, the same red run T015 records (Principle III)

### Implementation for User Story 2

- [x] T016 [US2] Change the default placement function in `source/harness/execution_context.cpp:102` so its `PlacementResult::description` comes from `detail::placementText(slot)`: the pair `cpu@group` for a slot that names a cpu whose pin landed, with the group looked up through `layout()`, the cpu index alone when the layout names no group for that cpu (E-09), and `NO_PIN` for `kNoPin`, while the function keeps the `UNAPPLIED` account of `detail::selectPlacementAccount` when the pin does not land, and move the placement value set of `specs/020-execution-contexts-core/contracts/report-disclosure.md` to `NO_PIN`, `cpu@group`, or `UNAPPLIED` where the slot never landed, in the same commit as this printer (contracts/report-topology.md), and restate that contract's width sentence, `stands cut to the width of the longest account the default function produces`, so the longest account it names is the `cpu@group` pair; the text stays inside `kPlacementFieldWidth` of `source/harness/report.cpp:168` for every cpu index a real host carries (FR-021, R-06, Q-2)

- [x] T017 [US2] Print the topology line from `detail::printContext(bool placementInEffect)` in `source/harness/report.cpp:287`: `speedgunMain` passes true when the walk of T020 found a selected instance carrying a preset other than `none` with a context count above 1, and false otherwise, because the selected set is local to `speedgunMain` and `detail::instances()` alone would answer for families the filter excluded (FR-019), one line per report (FR-020, FR-022, R-09), and restate the comment at `source/harness/report.cpp:161-167` so the width rule it records names the `cpu@group` text and the cut a long pair meets (Principle IV), and restate the same claim in `test/source/harness_context_report_test.cpp:99-104` and the two check messages at `:817-818` and `:827-828`, where `kWidestCpuIndex` is no longer a text the default placement function produces: name it the widest single-index text the field holds whole, and add the pair form `4294967295@0` to `scenarioPlacementField` as a cut case

- [x] T018 [P] [US2] Confirm `tools/docs/report_shape.py` needs no edit: it stores no expectations of its own, and each run of it compares the sample output block named by the pair it is given: `example/first_benchmark.expected#report` at `README.md:80` for the `report_shape` entry at `test/CMakeLists.txt:952`, and `example/docs/reading_the_report.expected#report` at `docs/pages/harness.md:572` for the `report_shape_guide_sample` entry at `:967`; the percentiles lane registered a third comparison, `report_shape_percentiles_sample` at `:984`, over `example/docs/percentiles.expected#report` at `docs/pages/harness.md:857`, and this feature leaves it alone; no block shows a run with a preset in play, so the new topology line reaches neither; extend the fold in `classify` only if a line class does reach that block, and confirm the README "Reading the report" region and `example/docs/reading_the_report.cpp` still match (FR-022, SC-05, contracts/report-topology.md)

**Checkpoint**: US2 is demonstrable on its own: the report names the
layout and each context's placement, and a no-preset report is unchanged
(SC-05, SC-07).

## Phase 4: User Story 3 - Ask for a shape the machine cannot give (Priority: P2)

**Goal**: Every bad request stops with one line and status 2 before any
run starts, and the layout is readable without a benchmark (spec US3).

**Independent Test**: "select each bad input in turn (an unknown name, a
cpu list, an over-subscribed preset) and read the exit status and the
stderr line." (spec.md US3)

### Tests for User Story 3 (write first, observe failure)

- [x] T042 [US3] Append to `test/source/harness_cli_placement_test.cpp` the scenarios that need the resolver and the listing: an instance asking 5 contexts under `l3` on a four-group layout prints one line naming the preset, 4, and 5, and exits 2 before any context starts and with no row printed (FR-018); a family carrying `threadRange` counts 2, 4 and 8 under `l3` stops the whole batch before the first run (FR-018, Q-4); `--topology` prints the block of data-model.md and exits 0 with no benchmark run, and its counts equal the topology line of a preset run (FR-036, SC-12); a fresh process by the fork and execv pattern of `test/source/harness_context_barrier_test.cpp:116-126` installs a description the library rejects and runs `--topology`, so the listing prints one line and exits 2 with no benchmark run, the scenario that covers the unreadable-layout branch T021 adds (R-07); `--placement=none` on a threaded run takes the skip path of the fit check, prints no topology line, and prints the block byte for byte as feature 020 ships it (FR-010, FR-022); `--placement=l3` on a run of one context over a readable layout keeps feature 020's bytes with no plan and no pin (FR-019), and the same run over a description the library rejects prints one line and exits 2, the reading Q-6 records (FR-005, Q-6); `--topology` with a filter that matches nothing still prints the listing and exits 0, because the listing precedes selection (FR-036). Observe red before T020 and T021 exist

### Implementation for User Story 3

- [x] T020 [US3] Add the batch fit check to `speedgunMain` in `source/harness/cli.cpp` after selection and before `detail::printContext()` at `source/harness/cli.cpp:369`: for every selected instance that carries a preset other than `none`, first consult `layout()` and, when `readable` stands false, stop with one line naming the layout failure and exit 2 whatever the context count (FR-005, Q-6); then, only for an instance whose context count stands above 1 (Q-7), ask `resolvePreset` and stop on the first plan whose `cpu` stands empty, with one line naming the preset, the place count, and the requested count, exiting 2; an instance with no preset or with `none` skips the check, which keeps FR-022 byte identity for an ordinary threaded run (FR-005, FR-018, FR-010, R-08)

- [x] T021 [US3] Implement `--topology` in `source/harness/cli.cpp` beside `--catalog`: parse the flag into `RunOptions::topologyMode`, print the listing through `printTopologyListing` in `source/harness/topology.cpp` (the counts line carrying the mask the layout was built from, plus one line per L3 group with its cpu membership inside the mask), and return 0 with no benchmark run; an unreadable layout makes the listing print one line and exit with status 2 (FR-036, Q-3; the unreadable status follows plan decision R-07) and add the `--topology` row of the option table at `README.md:640` in the same commit with the cap raise it costs, for the same drift-gate reason, together with the `--topology` row and exit-status row of `specs/015-benchmark-harness-core/contracts/cli.md` (FR-036, FR-028)

**Checkpoint**: US3 stands on its own: every refusal is one line and
status 2, and `--topology` reads the machine with no benchmark (SC-06,
SC-12).

## Phase 5: User Story 4 - Compare the siblings of one core (Priority: P2)

**Goal**: `smt`, `core` and `compact` give the three shapes a user
compares for SMT contention (spec US4).

**Independent Test**: "run the same benchmark with `smt` and with `core`
on an injected layout with two siblings per core, and read the cpus in
the disclosure." (spec.md US4)

### Verification for User Story 4

- [x] T025 [US4] Record the thrawn real-host check of `quickstart.md` in this task's note: the `--topology` block, the four-context `l3` run of `build/example/docs_threads`, the `smt` refusal of that same family, the two-context `smt` and `core` pair of `build/example/docs_topology_placement`, and the `taskset -c 0-11` narrowing, each with its measured output (SC-01, SC-02, SC-06, SC-11)

  Note: measured on thrawn (one package, four L3 groups, 24 cores, 48
  threads), 2026-10-20. `docs_threads --topology` printed the counts
  line `topology: packages=1 l3-groups=4 cores=24 threads=48 mask=0-47`
  and the group lines `l3 0: 0-5,24-29`, `l3 1: 6-11,30-35`,
  `l3 2: 12-17,36-41`, `l3 3: 18-23,42-47`, exit 0. The four-context
  `l3` run of `docs_threads` printed the topology line and
  `contexts=4 ctx0=SPIN/0@0/applied ctx1=SPIN/6@1/applied
  ctx2=SPIN/12@2/applied ctx3=SPIN/18@3/applied`, one cpu per group,
  exit 0. The `smt` run of that family printed
  `--placement: 'smt' places 2 of the 4 requested contexts` and exit
  2. `docs_topology_placement --filter bmPair --placement=smt` printed
  `contexts=2 ctx0=SPIN/0@0/applied ctx1=SPIN/24@0/applied`, the
  sibling pair of one core, and the same family with
  `--placement=core` printed `contexts=2 ctx0=SPIN/0@0/applied
  ctx1=SPIN/1@0/applied`, two threads of one group. Under
  `taskset -c 0-11` the listing read
  `topology: packages=1 l3-groups=2 cores=12 threads=12 mask=0-11`
  with lines `l3 0: 0-5` and `l3 1: 6-11`, and the four-context `l3`
  run stopped with the line
  `--placement: 'l3' places 2 of the 4 requested contexts` (SC-01,
  SC-02, SC-06, SC-11).

**Checkpoint**: US4 stands on its own: the three-shape scenarios pass
`ctest --test-dir build -R harness_placement_preset_test`, and the thrawn
`smt` and `core` pair of `build/example/docs_topology_placement` prints
two `cpu@group` fields naming one group (SC-07, SC-11).

## Phase 6: User Story 5 - Read the page, copy the block, run the example (Priority: P3)

**Goal**: The main page teaches the presets, and its snippet runs in CI
(spec US5).

**Independent Test**: "run the docs drift gate and the example run entry
at the feature head." (spec.md US5)

### Implementation for User Story 5

- [x] T026 [P] [US5] Create `example/docs/topology_placement.cpp` with two families: `bmCompact` with `threads(2)` and the handle preset `compact`, the widest of the five whose place count equals the mask's thread count, so a bare run places on any host whose mask holds two cpus, and `bmPair` with `threads(2)` and no handle preset so the command line can show a shape; mark the region the page copies (FR-025, FR-029, R-10)

- [x] T027 [P] [US5] Register `add_docs_example(topology_placement)` in `example/CMakeLists.txt` beside the existing docs examples so `docs_topology_placement` and `run_docs_topology_placement` exist and `run-examples` depends on them (FR-025)

- [x] T028 [US5] Add the "Topology placement" section to `README.md` at 25 lines or fewer with the five preset names, the fit idea, and the `--topology` listing stated in prose and inline code, and add the one name row of `contracts/page-and-examples.md`; the two option-table rows ride T014 and T021, the commits that add the options, so the drift gate never sees an option without its row; every fenced block of the section must carry a region marker naming a region of `example/docs/topology_placement.cpp`, and the page holds exactly one sample output block (`example/first_benchmark.expected#report` at `README.md:80`), so no second recorded-output block may join it (FR-025, FR-026)

- [x] T029 [P] [US5] Add the "Topology placement" section to `docs/pages/harness.md` with the preset table, the fit-limit idea, the report shape with the topology line, and the listing output rendered in prose and inline code so the page keeps its two `sg-sample-output` blocks, `example/docs/reading_the_report.expected#report` at `docs/pages/harness.md:572` and `example/docs/percentiles.expected#report` at `:857`, byte for byte, and this feature adds none (FR-027)

- [x] T030 [US5] Register the section in `tools/docs/example_drift.py` and move the page cap: append `"Topology placement",` to the `SECTIONS` tuple at `tools/docs/example_drift.py:30` and `"Topology placement": ("topology_placement.cpp",),` to `SECTION_OWNERS` at `:57`, without which `check_sections` reports `section 'Topology placement' is not one of the 21 capability sections` at `:540` or raises `KeyError` at `:532`; then raise `PAGE_LINE_CAP` at `:112` by the measured size of the new section, the cap having already absorbed the two option rows in T014 and T021, trim prose first, and record the measured number and the resulting page length in this task's note (FR-028, spec Budget note)

  Note: the section measured 24 lines with its quick-reference row
  (heading, prose, region marker, two fences, the 17-line region, the
  blank line and the name row), and the page stands at 692 lines with
  `PAGE_LINE_CAP` at 692; the drift gate passes at that number.

**Checkpoint**: US5 stands on its own: the drift gate and `example_run`
pass with the new section and example (SC-09).

## Phase 7: Polish and Cross-Cutting Concerns

- [x] T031 Extend `tools/hwloc/hwloc_nm_proof.sh` so the harness archive joins the library archive as a second subject with its own assertions: the harness archive holds undefined `sg_hwloc_*` references and zero defined ones, an executable that links both archives resolves them; it extends the standing proof to the
  harness archive, the check T004 names, and runs where it stands in Phase 7 as
  the gate with the vendored archive on its link line, no unprefixed `hwloc` symbol reaches an installed artifact, and `tools/hwloc/hwloc_purity_scan.sh` still finds zero hwloc references under `include/`; the library archive's A, B and C assertions keep their current form (R-12, FR-031)

- [x] T032 Coverage audit: run the coverage preset and record the line and branch figures for `source/harness/topology.cpp`, `source/harness/placement_preset.cpp`, and every changed region of `runner.cpp`, `report.cpp`, `cli.cpp`, `registry.cpp` and `execution_context.cpp` at 100 percent with no `LCOV_EXCL` marker; a gap stops the work and reports the cause with `gcov -b` evidence (FR-034, SC-08)

  Note: coverage tree on thrawn, 2026-10-20, re-run at the feature
  head after the load seam. The lcov gate
  (`tools/dbc/coverage_gate.sh` behind the `coverage` target) reports
  100.0 percent lines (5788 of 5788) and 100.0 percent branches (2432
  of 2432) over the extracted trace, with no exclusion marker on any
  new path. Per file after the standing filters: `topology.cpp` 100
  and 100 (60 branches), `placement_preset.cpp` 100 and 100 (46),
  `runner.cpp` 100 and 100 (508), `report.cpp` 100 and 100 (100),
  `cli.cpp` 100 and 100 (131), `registry.cpp` 100 and 100 (40),
  `execution_context.cpp` 100 and 100 (60). The failed-load arm of
  the read stands covered by the `loadFailed` scenario through the
  `installTestLoadFailure` seam (review decision D-1). The six arms
  the gate first found became tests or marker-free restructures.

- [x] T033 Sanitizer runs: the `ci-sanitize` and `ci-tsan` presets of `CMakePresets.json` over the four new executables, with zero findings (SC-08)

  Note: both presets configured with `CXX=clang++` on thrawn, the CI
  parity choice, because GCC 16 libstdc++ carries the known regex
  false positive of PR105749 that CI's clang does not. The sanitize
  tree built clean and passed 103 of 103 tests; the tsan tree built
  clean and passed 100 of 100; the four new executables ran in both
  trees with zero findings. Only pre-existing vendored hwloc C
  warnings appear in the build logs.

- [x] T034 Static analysis: build `ci-ubuntu` from `CMakePresets.json` so clang-tidy and cppcheck run, and clear every finding the change introduces at the feature head; run clang-format over the change, then `cmake --build build --target format-check` and `spell-check` so both Principle VIII format gates run at the feature head, and `readability-identifier-naming` reports zero findings, the name-check-clean gate that `.clang-tidy` raises to an error, the advisory findings standing as master leaves them (Principle V, Principle VIII, spec Constraints)

  Note: clang-format 18.1.8 (the pinned `FORMAT_COMMAND`) reports zero
  diffs over the tree and `format-check` and `spell-check` exit 0 at
  the feature head. The local clang-tidy (LLVM 23.1.1) raises
  findings only on master files (`hwloc_gate.cpp`,
  `counters_trap_noguard_fixture.cpp`, `dbc_standalone.cpp`) and zero
  on every file this branch touches; cppcheck is absent from this
  machine, so the pinned CI pair carries that lane and T039 reads its
  exit code from the CI log.

- [x] T035 Dogfood and time-source gate: confirm no new path reads a clock or a wall clock, that every measured value still comes from `sg::counters`, and that the `time_source_gate` entry of `test/CMakeLists.txt`, which runs `test/time_source_gate.sh`, stays green (FR-024, FR-035)

- [x] T036 Version line: set the project `VERSION` in `CMakeLists.txt` to the next free minor (`0.12.0`, `0.11.0` having been taken at `8eac15d`), keep the harness `SOVERSION` at 7 and the archive `SOVERSION` at 2, record beside the change the exported-layout check that justifies keeping them, and add to `docs/pages/building.md` the 0.10.0 and 0.11.0 paragraphs that page lacks together with the distinction that a member function alone moves no layout: the move to 5 came from the row model, the aggregate statistics and the complexity fit, the move to 6 came from data members of `State` and `BenchmarkResult`, and the move to 7 came from the chunk fields, the chunk cadence, the row percentile columns and the percentile selector, and amend the 0.9.0 paragraph of that page so its items separate the exported layout moves (the row model, the aggregate statistics, the complexity fit and the four new enums) from the member functions `State` and `BenchmarkHandle` gained at the same time, which move no layout (FR-032, FR-033, SC-10)

- [x] T040 Commit series per the Pull Request Quality section of `.specify/memory/constitution.md`: `<Section>: <one-line imperative>` at 50 characters or fewer, a why-body wrapped at 72, one footer block with `Approved-by:` for the decisions the owner made and `Refs: specs/025-topology-placement`, atomic and bisectable (spec Constraints)

- [x] T037 Prose and commit gate over the whole change: run
  `cmake -P cmake/prose-lint.cmake`, the standing wrapper that runs
  `--check all --mode range` with the left edge resolved from the merge
  base with `origin/master`, and require zero findings for every new
  comment and doc line and for every commit message of the branch;
  `--check prose --mode tree` alone never reads a commit message, so it
  is not the gate for this task (Principle XI, Principle VIII)

- [x] T038 Rebase the branch `022-topology-placement` onto the current `master`, keeping both lanes' lines in every shared file of the spec's Parallel work list, take the next free minor version if another lane landed first, confirm `PAGE_LINE_CAP` in `tools/docs/example_drift.py` equals the measured page length after the rebase, T030's raise having covered this feature's own section (FR-028), run the full gate set, and push once with `--force-with-lease`, then show `git ls-remote origin 022-topology-placement` (spec Parallel work)

- [x] T039 Full gate record: `ctest --test-dir build --output-on-failure -j 2`, `tools/docs/example_drift.py`, `tools/docs/report_shape.py`, the `example_run` entry, `tools/hwloc/hwloc_purity_scan.sh` and `tools/hwloc/hwloc_nm_proof.sh`, the DBC gate, `format-check`, `spell-check` and `tools/prose/prose_gate.py`, each with its exit code, read from the CI logs after every push (Principle VIII, spec Constraints)


- [x] T041 Main-page impact check (IV.1): confirm the "Topology placement"
  section, its region in `example/docs/topology_placement.cpp`, the one
  quick-reference name row, the two option-table rows and the
  `docs/pages/harness.md` detail all shipped in the same change as the code
  they teach, each snippet a region a CI-built example prints (FR-025 to
  FR-029, SC-09)

## Dependencies & Execution Order

### Phase Dependencies

- T001, T002, T003, T004 and T006 (Setup) block every story test; T005 is
  the one Setup task deferred past T010, as its own text states.
- T005's `nm` capture runs in Phase 2 after T010, since no harness object
  carries an `sg_hwloc_*` reference before that unit exists; T031's extended
  proof runs where it stands in Phase 7.
- Phase 7 follows the stories; T040's commit series precedes T037's
  commit-message gate and T038's push; T038 is the last task that changes
  the tree, T039 reads the CI logs its push produces, and T041 closes the
  change.

### User Story Dependencies

- US1: T007, T008 before T010, T011; T011 before T012; T013 before T014; T019 runs in Phase 2 before T013 and T014.
- US2 needs T010, T011, T012 (a plan must exist to disclose): T015 before
  T016, T017.
- US3 needs the resolver of T011, the layout of T010 and the CLI test file T019
  creates in Phase 2: T042 before T020, T021.
- US4: T023 runs in Phase 2 beside T008 and T024 in Phase 3 beside T015, both
  red before T011 and T016 exist; T025 records the host runs once the stories
  land.
- US5 needs T021 (the listing the page block of T028 quotes) and T017
  (the line the page shows).

### Within Each User Story

- US1: the test tasks T007 to T009 precede T010 and T011; T019 precedes T013 and T014.
- US2: T015 precedes T016 and T017.
- US3: T019 runs in Phase 2 before T013 and T014; T042 precedes T020 and T021.
- US4: T023 and T024 append to the files T008 and T015 create and run in those
  phases, red before T011 and T016 exist; T025 runs after the stories land.
- US5: T026 and T027 precede T028 and T029.

### Parallel Opportunities

- Setup: T001, T003 and T004 touch three different files; T002 shares the header with T001.
- US1: T007 and T008 are different test files.
- US2: T015 and T018 are different files.
- US3: T042 follows T019, whose file it appends to, and runs before T020 and T021; the earlier-contract edits ride T022 in Phase 2, T016 in Phase 3 and T021 in Phase 4, each in the commit that adds the surface it documents.
- US5: T026 and T027 are different files; T029 is a different file from
  T028.
- Across stories once Setup and US1 land: US2, US3 and US4 touch
  `report.cpp`, `cli.cpp` and the test files, and can proceed together.

## Parallel Example: User Story 1

```bash
# T007 and T008 are two test files; their registrations append to
# test/CMakeLists.txt in task-id order.
cmake --build build --target harness_topology_test harness_placement_preset_test
ctest --test-dir build -R 'harness_topology_test|harness_placement_preset_test'
```

## Implementation Strategy

### MVP First (User Story 1)

Phase 1 plus Phase 2 (US1). At the MVP checkpoint a user names
`l3`, the harness reads the machine once, and four contexts land on four
L3 groups with the plan visible through the installed placement function.

### Incremental Delivery

US2 makes the work visible in the
report, US3 makes the failures clean and adds the listing, US4 completes
the preset shapes, US5 ships the page and the example, and Phase 7
clears the gates and lands the change.

**Delivery note**: the three parallel lanes share only the files the
spec's Parallel work list names. N1 lands first when it is ready, and
this feature rebases onto it before its own merge.

## Notes

- Every task names the file it edits and the requirement it serves, so a
  reviewer can check one task against the spec without reading the next.
- A task that cannot run as written is a defect in this file: fix the
  task before the code.
- T019 runs in Phase 2 beside T023 so its scenarios stand red before T013
  and T014 ship the option surface, the way T023 runs in Phase 2 for US4.
  T042 carries the scenarios that need T020 and T021.
- Setup blocks everything because the seams and the data model are the
  contract every story test compiles against.

## Phase 8: Convergence

- [x] T043 Replace the recorded LCOV_EXCL_BR_LINE on the load-failure arm of the layout read with a seam-backed test, and re-record the T032 note without the approval claim, per FR-034, the Constraints coverage rule and Constitution VI (contradicts)
- [x] T044 Run the coverage target at the feature head after the last source edit and re-record the line and branch figures for topology.cpp and placement_preset.cpp, per SC-08 (partial)
