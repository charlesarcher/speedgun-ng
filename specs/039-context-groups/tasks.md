# Tasks: Context Groups for the Threaded Run

**Input**: Design documents from `/specs/039-context-groups/`

**Prerequisites**: [plan.md](./plan.md) (required), [spec.md](./spec.md)
(required for user stories),
[contracts/group-partition.md](./contracts/group-partition.md),
[contracts/group-layout.md](./contracts/group-layout.md),
[data-model.md](./data-model.md)

**Tests**: TDD mode (plan.md, Execution mode): the test task of each
wave stands written and observed failing before the implementation task
that turns it green, and the pair lands in one commit.

**Organization**: tasks stand grouped by wave behind the module landing
order plan.md fixes (M-A to M-J). A `[P]` task touches files no other
open task of its wave touches, so the wave's `[P]` tasks run together.

## Format: `[ID] [P?] [Wave] Description`

- **[P]**: can run in parallel (different files, or one file behind
  disjoint scenario blocks)
- **[Wave]**: the wave this task serves (the plan's wave list)
- Every description names the exact file path, the requirements it
  closes, and the verify command

---

## Phase 1: The surface and the numbers (module M-A, wave 1)

**Purpose**: the public record and the version block every later module
builds on. Nothing else lands before this phase closes.

- [ ] T001 [1] Write the failing surface test in
  `test/source/harness_group_report_test.cpp` (the disclosure block)
  with its registration in `test/CMakeLists.txt` beside the placement
  tests: an aggregate `ContextDisclosure` answers an empty group, a
  filled record carries the name beside the wait mode, the placement
  text and the applied flag (FR-005, FR-031, A-1 of the partition
  contract). Verify:
  `ctest --test-dir build -R harness_group_report_test --output-on-failure`
  red on the block.
- [ ] T002 [1] Add `std::string group` to `sg::ContextDisclosure` in
  `include/speedgun-ng/benchmark.hpp` as the record's last member, with
  the doxygen block naming its `\pre`, `\post` and the empty-name
  meaning (FR-005, FR-040). Verify:
  `ctest --test-dir build -R harness_group_report_test --output-on-failure`
  green, and the DBC gate reads no finding.
- [ ] T003 [1] Move the project version `0.25.0` to `0.26.0` in
  `CMakeLists.txt:30` and the harness `SOVERSION` `16` to `17` in
  `CMakeLists.txt:333`, the archive `SOVERSION` staying `4` at
  `CMakeLists.txt:95`, the comment beside the block naming the moved
  public layout (FR-005, FR-032). Verify:
  `cmake --preset=ci-rocky && cmake --build build` green, and the
  configure log names `0.26.0`.
- [ ] T004 [1] Read the wave on the release preset with the analyzers
  wired in: `cmake --preset=ci-ubuntu && cmake --build build` (IX).
  Verify: zero clang-tidy and cppcheck findings.

**Checkpoint**: the field, the version and the `SOVERSION` stand, the
suite green.

---

## Phase 2: The partition and the layout reader in parallel (modules M-B and M-C, wave 2)

**Purpose**: the two pure units. Neither touches a thread, and each
owns its own files, so the module pairs run together.

- [ ] T005 [P] [2] Write the failing partition tests in
  `test/source/harness_group_partition_test.cpp` with their
  registration: the six presets over four installed shapes (four L3
  groups, two nodes over four groups, one group, a narrow mask), the
  `kNoGroup` read, the one-group answer, the one-context answer, the
  `l3-` and `node-` name spellings, the plan-unchanged read, the
  first-appearance order, and `groupOfContext` over a two-group
  partition (FR-001 to FR-010, B-1, B-2, A-1 to A-6). Verify:
  `ctest --test-dir build -R harness_group_partition_test --output-on-failure`
  red.
- [ ] T006 [P] [2] Implement `sg::detail::ContextGroup`,
  `groupPartition` and `groupOfContext` in
  `source/harness/group_partition.cpp` with their declarations and
  contracts in `source/harness/detail/internal.hpp`, reading
  `plan.group` under `core`, `l3`, `smt` and `compact` and `plan.node`
  under `node` (FR-006 to FR-010, D-3, D-4). Verify:
  `ctest --test-dir build -R harness_group_partition_test --output-on-failure`
  green.
- [ ] T007 [P] [2] Write the failing layout-reader tests in
  `test/source/harness_group_layout_test.cpp` with their registration:
  the accept set (explicit names, default names, one entry covering
  every context, unsorted slots) and every fault row of the layout
  contract's table C, each asserting one line naming the path and the
  entry (FR-012 to FR-018, FR-021, B-1, A-1 to A-6). Verify:
  `ctest --test-dir build -R harness_group_layout_test --output-on-failure`
  red.
- [ ] T008 [P] [2] Implement `parseGroupLayout` in
  `source/harness/group_layout.cpp` through `<yaml-cpp/yaml.h>`, attach
  the vendored reader to the harness target beside the simdjson line
  with a PRIVATE `BUILD_INTERFACE` link, and restate the includer
  comment at `CMakeLists.txt:753` (FR-012 to FR-017, R-3, P-3). Verify:
  `ctest --test-dir build -R "harness_group_layout_test|yaml_purity_scan|yaml_nm_proof" --output-on-failure`
  green.
- [ ] T009 [2] Read the two seams' rule: no scenario of the two new
  files reads `/sys` or the host topology, and every exact shape stands
  installed through `installTestLayout` (`internal.hpp:790`) (FR-036,
  R-5). Verify:
  `grep -rn "/sys" test/source/harness_group_*.cpp` answers nothing.

**Checkpoint**: the partition and the grammar stand proven, no run
behavior changed.

---

## Phase 3: The option row, the pairings and the fit walk (module M-D, wave 3)

**Purpose**: the second source reaches the command line, and the standing
fit walk learns to stop a batch the layout cannot serve.

- [ ] T010 [3] Write the failing CLI tests in
  `test/source/harness_group_cli_test.cpp` with their registration: the
  row is recognized, `--help` prints its line, a missing file refuses
  one line at status 2, the pairing beside `--placement=l3` and beside
  `--hunt` refuses one line naming both, and the counts `2,4,8` beside a
  four-slot layout stop the batch with no row printed (FR-011, FR-019,
  FR-020, FR-021, B-3). Verify:
  `ctest --test-dir build -R harness_group_cli_test --output-on-failure`
  red.
- [ ] T011 [3] Move `test/source/harness_cli_table_test.cpp` to the
  thirty-ninth row: the contract literal at `:209-248`, the row-count
  check at `:681-689`, the block-disjointness partition at `:858-939`,
  and the guide table byte-compare at `:1498-1583` (FR-011). Verify:
  `ctest --test-dir build -R harness_cli_table_test --output-on-failure`
  red on the row count.
- [ ] T012 [3] Add `kGroupLayoutOptionValue = 299` beside the free-block
  comment in `source/harness/detail/options.hpp:81-93`, add the row to
  `optionRows()` in `source/harness/options.cpp:470-714` with
  `OptionArgument::REQUIRED`, no environment source and
  `"specs/039"` as its owner, and carry the path through the resolved
  option set (FR-011). Verify:
  `ctest --test-dir build -R "harness_group_cli_test|harness_cli_table_test" --output-on-failure`
  green on the row and the help line.
- [ ] T013 [3] Implement `groupLayoutFitStatus` in
  `source/harness/group_fit.cpp` and call it beside `placementFitStatus`
  at `source/harness/cli.cpp:744`, reading the file once and comparing
  its slot count against every selected instance's context count
  (FR-020, B-3). Verify:
  `ctest --test-dir build -R harness_group_cli_test --output-on-failure`
  green on the batch stop.
- [ ] T014 [3] Add the two pairing refusals beside the standing hunt
  combination line in `source/harness/hunt_cli.cpp:720-748` and the
  placement pairing in `source/harness/cli.cpp`, one line naming both
  names, status 2 (FR-019, D-2). Verify:
  `ctest --test-dir build -R "harness_group_cli_test|base_cli_matrix" --output-on-failure`
  green.
- [ ] T015 [3] Add the option-reference row for `--group-layout` to
  `docs/pages/harness.md` beside the `--placement` row at `:680`, the
  text the table's help string copies (FR-011, FR-039). Verify:
  `ctest --test-dir build -R "harness_cli_table_test|example_drift" --output-on-failure`
  green.
- [ ] T016 [3] Read the wave on the release preset:
  `cmake --preset=ci-ubuntu && cmake --build build`. Verify: zero
  analyzer findings, the full suite green.

**Checkpoint**: both sources reach the run, every refusal answers one
line at status 2, and no measured work has run.

---

## Phase 4: The group's own rounds (module M-E, wave 4)

**Purpose**: the attempt splits its two standing rendezvous points by
group, and the fixture pair keeps its order without a round across
groups.

- [ ] T017 [4] Write the failing run tests in
  `test/source/harness_group_run_test.cpp` with their registration and
  `PROCESSORS`: eight contexts over a four-group shape complete and
  disclose four groups of two; four contexts over a two-node shape
  alternate into `node-0` and `node-1`; a one-group run completes; a
  single-member group completes; an ungrouped run completes; a thread
  runner that leaves a member undispatched aborts at the ledger; a
  fixture setup that fails reaches every body and skips the teardown;
  the teardown follows the last settled body (FR-001, FR-004, FR-010,
  FR-022 to FR-027, FR-034). Verify:
  `ctest --test-dir build -R harness_group_run_test --output-on-failure`
  red.
- [ ] T018 [4] Compute the partition once per instance beside the
  context build in `source/harness/runner.cpp:1626-1644` and carry it
  with `groupOfContext` to the attempt scope, the `node` plan's standing
  slot as the shape to follow (FR-001, P-4). Verify:
  `ctest --test-dir build -R "harness_runner_test|harness_group_partition_test" --output-on-failure`
  green, the new run cases still red.
- [ ] T019 [4] Build one `sg::ContextBarrier` per group per point in
  `threadedSampleRun` (`runner.cpp:2066-2067`) as
  `std::vector<std::unique_ptr<sg::ContextBarrier>>` that outlives
  `launchBodies`, and make the body arrive at its own group's rounds at
  `:2126` and `:2149` (FR-022 to FR-025, P-2). Verify:
  `ctest --test-dir build -R harness_group_run_test --output-on-failure`
  green on the completion cases.
- [ ] T020 [4] Add the per-attempt publication slot beside the outcome
  vector: context 0 writes the setup text then the ready flag with
  release order, every other body waits on the flag with acquire order
  before it copies the text, and the teardown node runs on context 0
  after `launchBodies` returns when the run carries more than one group
  (FR-026, D-7). Verify:
  `ctest --test-dir build -R "harness_group_run_test|harness_fixture_test" --output-on-failure`
  green.
- [ ] T021 [4] Confirm the ledger fault precedes every wait the harness
  performs on a grouped attempt, the stranded-member case answering the
  standing contract message in its own process (FR-027, FR-024). Verify:
  `ctest --test-dir build -R harness_group_run_test --output-on-failure`
  green on the ledger case.
- [ ] T022 [4] Read the wave for the round rules: no arrival, wait or
  dispatch sits inside a measured window, the participant counts come
  from the partition, and no clock joins the attempt
  (`runner.cpp:2062-2065`) (FR-024, VII). Verify:
  `ctest --test-dir build -R "harness_context_barrier_test|harness_group_run_test" --output-on-failure`
  green.

**Checkpoint**: groups pace on their own rounds, the ungrouped attempt
keeps its standing pair, and the fixture pair keeps its order.

---

## Phase 5: The console segment and the report in parallel (modules M-F and M-G, wave 5)

**Purpose**: the pacing a run chose reaches the reader. The console unit
and the report unit own disjoint files and run together.

- [ ] T023 [P] [5] Write the failing console rows in
  `test/source/harness_group_report_test.cpp` (the segment block): the
  group names the line for a pinned and an unpinned context, the
  standing segments keep their order, an ungrouped run's line stands
  byte-identical to the standing shape, the machine line stays absent
  for a layout run, and the per-context percentile rows keep their flat
  shape (FR-028, FR-033, FR-035, C of the partition contract). Verify:
  `ctest --test-dir build -R harness_group_report_test --output-on-failure`
  red on the block.
- [ ] T024 [P] [5] Print the group segment in
  `printContextDisclosure` (`source/harness/report.cpp:168-191`) as the
  line's last field, and keep the machine line's preset-in-effect rule
  (`cli.cpp:744-755`) (FR-028). Verify:
  `ctest --test-dir build -R "harness_group_report_test|harness_context_report_test" --output-on-failure`
  green.
- [ ] T025 [P] [5] Write the failing report rows in
  `test/source/harness_group_report_test.cpp` (the document block) and
  move the `1.8` literals to `1.10` in
  `test/source/harness_json_report_test.cpp:546`,
  `test/source/harness_json_load_test.cpp:707` and its `1.9` plant at
  `:1076`, and `test/source/harness_noise_report_test.cpp:403` and
  `:423`: the writer names `group` per context, the schema accepts the
  document and refuses a group name outside it, the reader answers the
  name, and an ungrouped document carries no key (FR-029 to FR-032,
  R-4). Verify:
  `ctest --test-dir build -R "harness_group_report_test|harness_json_report_test|harness_json_load_test|harness_json_schema_test" --output-on-failure`
  red.
- [ ] T026 [P] [5] Move `kSchemaVersion` to `1.10` with its history
  comment (`source/harness/json_report.cpp:28-39`), write the key in
  `writeEntry` (`json_report.cpp:580-593`), read the optional key in
  `readEntry` (`source/harness/json_load.cpp:1869-1897`), and add the
  optional key to the context item of
  `docs/schema/report-v1.json` (`:386-402`) (FR-029 to FR-031, FR-032).
  Verify:
  `ctest --test-dir build -R "harness_json_schema_test|harness_json_load_test|harness_group_report_test" --output-on-failure`
  green.
- [ ] T027 [5] Re-read the quoted report samples the segment touches:
  the `contexts=` lines in `docs/pages/harness.md` at `:596`, `:979` and
  `:1568` and the `report_shape` goldens (FR-028, IV.1). Verify:
  `ctest --test-dir build -R "report_shape|example_drift" --output-on-failure`
  green.
- [ ] T028 [5] Read the wave on the release preset:
  `cmake --preset=ci-ubuntu && cmake --build build`. Verify: zero
  analyzer findings, the full suite green.

**Checkpoint**: the disclosure carries the group on both channels, and an
ungrouped run answers master's bytes.

---

## Phase 6: The docs (module M-H, wave 6)

**Purpose**: the main page teaches the feature in the same change the
behavior landed in (IV.1).

- [ ] T029 [6] Write `example/docs/context_groups.cpp` with the marked
  region `groups` and its registration so an example target builds and
  runs it, the benchmark printing the rows the section's result sentence
  names (FR-039, IV.1). Verify:
  `ctest --test-dir build -R "example_run|example_run_fixtures" --output-on-failure`
  green.
- [ ] T030 [6] Add the groups section to `README.md`, under the section
  list heading, quoting the region, its quick-reference rows, the title
  in `SECTIONS` (`tools/docs/example_drift.py:33-65`), its owner in
  `SECTION_OWNERS` (`:70-102`), and the page cap move at `:222` with the
  reason beside it (FR-039, IV.1). Verify:
  `ctest --test-dir build -R "example_drift|example_drift_fixtures|snippet_compile" --output-on-failure`
  green.
- [ ] T031 [6] Teach the guide page: the groups paragraph in
  `docs/pages/harness.md` beside the placement section, naming the two
  sources, the refusals and the disclosure segment (FR-039). Verify:
  `ctest --test-dir build -R example_drift --output-on-failure` green.
- [ ] T032 [6] Read the docs target on the grown public header: the new
  member's doxygen and the header's contract block stand processed
  (FR-040, II, IV). Verify: `cmake --build build --target docs` green
  with no doxygen warning for `benchmark.hpp`.

**Checkpoint**: the page, the region, the rows and the guide stand equal,
the gates read no finding.

---

## Phase 7: The evidence (module M-I, wave 7)

**Purpose**: what a later lane reads: one record per acceptance run, the
commands and exit codes named, the artifacts beside them.

- [ ] T033 [7] Write `specs/039-context-groups/results/run_groups.sh`,
  the script that drives the five acceptance runs, captures each exit
  code, and copies the reports it writes into `results/reports/`
  (FR-038). Verify: `bash specs/039-context-groups/results/run_groups.sh`
  exits 0.
- [ ] T034 [7] Record the preset grouping over the multi-group shape in
  `results/preset-l3.md`: the command, the exit code, the console block
  naming the groups, and the report document beside it (FR-038, SC-001).
- [ ] T035 [7] Record the `node` grouping over a multi-node shape in
  `results/preset-node.md`, the alternating memberships named
  (FR-038, SC-002).
- [ ] T036 [7] Record the layout run end to end in `results/layout.md`:
  the file, the command, the exit code, the disclosure, and the report
  (FR-038, SC-005).
- [ ] T037 [7] Record the refusal set in `results/refusals.md`: all
  eighteen cases of FR-018 and FR-019 with their one line and status 2,
  and no report file written (FR-038, SC-004).
- [ ] T038 [7] Record the ungrouped comparison in
  `results/ungrouped.md`: the same fixture run on this branch and on
  `master`, the console bytes equal apart from the schema stamp (FR-038,
  SC-006).
- [ ] T039 [7] Write `specs/039-context-groups/quickstart.md`, the
  record index naming the question each record answers, and the re-run
  commands (FR-038, SC-010). Verify: every file under `results/` stands
  linked from the quickstart.

**Checkpoint**: the lane's readings stand recorded and re-creable.

---

## Phase 8: Polish, gates and convergence (module M-J, wave 8)

**Purpose**: every hard gate of Principle VIII read green, the contested
bindings re-read against the base, the verdict recorded.

- [ ] T040 [8] Build and run the whole suite on both release presets:
  `cmake --preset=ci-rocky && cmake --build build && ctest --test-dir build --output-on-failure`,
  then the same read on `ci-ubuntu` (VIII, IX). Verify: 100 percent of
  the tests pass on both.
- [ ] T041 [P] [8] Run the sanitizer suite: `cmake --preset=ci-sanitize`,
  `cmake --build build`, `ctest --test-dir build --output-on-failure`
  (VIII). Verify: zero ASan and UBSan reports.
- [ ] T042 [P] [8] Run the thread-sanitizer suite on the tsan preset and
  report no race over the grouped scenarios (VIII). Verify: zero tsan
  reports.
- [ ] T043 [P] [8] Run the two-cpu read:
  `taskset -c 0,1 ctest --preset prepush-quick -j 2` (FR-036, specs/035
  FR-017). Verify: every test passes.
- [ ] T044 [P] [8] Run the coverage read: the coverage preset, the full
  suite, then the coverage target (VI). Verify: 100 percent lines and
  100 percent branches over `group_partition.cpp`, `group_layout.cpp`,
  `group_fit.cpp` and the changed hunks, no `LCOV_EXCL` marker and no
  new ignored error class.
- [ ] T045 [P] [8] Read the static-analysis gate on the release preset
  with the analyzers wired in, and the naming gate
  (`readability-identifier-naming`) with it (I, V.1, VIII). Verify: zero
  findings, and every suppression names the V.2 entry it applies.
- [ ] T046 [P] [8] Read the prose, spell, format, DBC and time-source
  gates over the lane range:
  `cmake -DPROSE_BASE=bea6af5b -P cmake/prose-lint.cmake`, the spell and
  format checks, the DBC gate,
  `test/time_source_gate.sh` (VIII, XI). Verify: zero findings on each.
- [ ] T047 [P] [8] Read the vendor proofs the yaml link touches:
  `ctest --test-dir build -R "yaml_purity_scan|yaml_nm_proof|hwloc_nm_proof|simdjson_purity_scan"`,
  and the installed-package consumer read (FR-016, specs/006). Verify:
  each proof passes.
- [ ] T048 [8] Clear every finding the gates report, one commit per
  logical fix, each carrying the commit template (Pull Request Quality).
  Verify: the gate re-reads of T040 to T047 answer zero.
- [ ] T049 [8] Re-read the contested bindings against the tree the base
  stands at: the version line, the harness `SOVERSION`, the schema minor,
  the option value. Move each one step where a landing took it, and name
  the read in the commit that moves it (FR-032).
- [ ] T050 [8] Record `results/verdict.md` with one row per gate and its
  reading, and mark the landed tasks in this file (FR-038, X.4). Verify:
  the verdict names every gate of Principle VIII.

---

## Dependencies & Execution Order

### Phase Dependencies

- Phase 1 (M-A) stands alone at the front: the field and the numbers
  carry no dependency.
- Phase 2 (M-B, M-C) depends on Phase 1 alone. Its two module pairs
  depend on nothing else and run together.
- Phase 3 (M-D) depends on Phase 2: the row stores what the reader
  parses and the fit walk calls.
- Phase 4 (M-E) depends on Phase 3: the runner reads the partition the
  option and fit steps settled.
- Phase 5 (M-F, M-G) depends on Phase 4. Its two modules own
  `report.cpp` and the `json_*` files and run together.
- Phase 6 (M-H) depends on Phase 5: the page quotes the bytes the run
  prints.
- Phase 7 (M-I) depends on Phase 6: the records name the shipped
  behavior.
- Phase 8 (M-J) depends on every phase above. T041 to T047 run
  together.

### Parallel Blocks

- T005 with T007, and T006 with T008: two modules, four files.
- T023 with T025, and T024 with T026: the console block and the
  document block of one test file plus their disjoint sources.
- T041, T042, T043, T044, T045, T046, T047: seven independent gate
  reads.
- Within Phase 7, T034 to T038 record separate runs of one script and
  touch separate files.

## Implementation Strategy

### MVP First (User Stories 1 and 2)

Waves 1 to 4 carry the MVP: a preset groups the contexts, a layout names
them, and each group meets rounds of its own. Stop after wave 4, run
`ctest --test-dir build`, and the item's behavior stands proven with the
disclosure still unwritten.

### Incremental Delivery

Wave 5 makes the pacing readable, wave 6 teaches it, wave 7 records it,
wave 8 clears the gates. Each wave closes with the suite green and one
commit block, so a lane stops cleanly at any checkpoint.

## Notes

- Commit plan: one logical block per phase, each commit compiling and
  passing its tests, the titles following the template with the standing
  sections (`Harness: ...`, `CMake: ...`, `Docs: ...`, `test: ...`,
  `Meta: ...`), the `Refs:` footer naming
  `specs/039-context-groups` and the `Approved-by:` footer in the
  landing commit.
- Every test task names its verify command. Its implementation task
  lands in the same commit, the pair closing one red-to-green step
  (III, TDD).
- The lane host for the wave 7 records stands named in each record: the
  branch host is `gary` (16 cores, 32 threads, two L3 groups), the lane
  prompt names thrawn. The synthetic and installed seams carry the
  four-group and two-node shapes on either host (FR-036, FR-038).
- Contention: specs 031 and 037 stand in flight; T049 re-reads the four
  contested bindings before the lane closes (FR-032).
