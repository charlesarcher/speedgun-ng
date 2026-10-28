# Tasks: NUMA Memory-Node Placement

**Input**: Design documents from `/specs/035-numa-placement/`

**Prerequisites**: [plan.md](./plan.md) (required), [spec.md](./spec.md)
(required for user stories), [research.md](./research.md),
[data-model.md](./data-model.md), [contracts/](./contracts/),
[quickstart.md](./quickstart.md)

**Tests**: TDD mode (plan.md, Execution mode): the test task of each
wave stands written and observed failing before the code task that
turns it green.

**Organization**: tasks stand grouped by user story so each story can
stand implemented and tested independently.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: can run in parallel (different files, no dependencies)
- **[Story]**: the user story this task serves (`US1` to `US5`)
- Every description names the exact file path and the requirements it
  closes

---

## Phase 1: Setup (the preset grammar and the version)

**Purpose**: the preset name every story plans through, and the
version record the lane stands on.

- [ ] T001 [P] Write the failing grammar tests in
  `test/source/harness_cli_placement_test.cpp`: `--placement=node`
  stands accepted from the command line and from the handle name;
  the six names stand matched in full and nothing else; the standing
  unknown-name refusal covers `nodes`, `node=` and `NODE`
  (FR-001, FR-015)
- [ ] T002 Add the `node` row to `kPresetNames` in
  `source/harness/placement_preset.cpp:46` and the `NODE` row to the
  `Preset` enum in `source/harness/detail/internal.hpp:106`; the
  commit moves `CMakeLists.txt:15` to `0.22.0` with the harness
  `SOVERSION` staying `13` at `CMakeLists.txt:298` and the archive
  `SOVERSION` staying `4` at `:80`, and its body records the ABI
  check: no public type, function or struct changes (FR-003, D-6)
  (depends on T001; the tests of T001 turn green)

**Checkpoint**: the grammar stands green; the surface carries the
feature with no new option row and no public-header move.

---

## Phase 2: Foundational (the node walk and the binder seam)

**Purpose**: the blocking prerequisites every story stands on: the
machine read that states the nodes, and the seam the binding stands
behind. No user story work can begin before this phase closes.

- [ ] T003 [P] Write the failing node-walk tests in
  `test/source/harness_numa_placement_test.cpp`: synthetic layouts of
  one, two and four nodes installed through `installTestLayout`
  (`topology.cpp:306`) state the node numbers and the per-node cpu
  sets the read carries; a layout with no memory information states
  none (FR-005, R-01)
- [ ] T004 Extend the machine read in `source/harness/topology.cpp`:
  the walk at `topology.cpp:175` adds `HWLOC_OBJ_NUMANODE`, and
  `MachineLayout` (`detail/internal.hpp:94`) gains `LayoutNode`
  beside `LayoutGroup` with the node number, the node's cpu set and
  the `memoryReadable` flag; the read stays one time per run
  (FR-005, D-1, R-01) (depends on T003; the tests of T003 turn
  green)
- [ ] T005 [P] Write the failing binder-seam tests in
  `test/source/harness_numa_placement_test.cpp`: the installed fake
  records the node the call names, the fake denies the call on
  demand, and a null installer restores the production binder, the
  clear-on-null shape `installTestLayout` uses (FR-016, R-06)
- [ ] T006 Add the binder seam: the new
  `source/harness/detail/memory_bind.hpp` carries the installer
  contract with its `\pre`/`\post`, and the production binder calls
  the hwloc membind entry points by node, thread scope, inside the
  one `<hwloc.h>` translation unit (`topology.cpp`, spec 025 FR-004)
  (FR-016, D-1, R-06) (depends on T005; the tests of T005 turn
  green)

**Checkpoint**: the foundation stands green: the read states the
nodes, and the binding stands behind the seam with the public surface
untouched.

---

## Phase 3: User Story 1 - A run planned onto the nodes binds and names them (Priority: P1) 🎯 MVP

**Goal**: a run with the `node` preset plans one context per node
inside the process mask, binds each context at start beside the pin,
and names the node in the placement field.

**Independent Test**: with an injected topology of two nodes and a
fake binder, a run with the `node` preset discloses the node of each
context in console and JSON, and the binding call carries the node
the plan states.

### Tests for User Story 1 (write first, observe failure)

- [ ] T007 [P] [US1] Write the failing resolver tests in
  `test/source/harness_numa_placement_test.cpp`: two nodes and eight
  contexts cycle the nodes; a count below the node count fills in
  hwloc's node order; a node whose cpus all stand outside the mask
  takes no context; every planned cpu stands inside the mask, on the
  bound node, and the cpus stand pairwise distinct
  (FR-009, FR-012, R-03, R-04)
- [ ] T008 [P] [US1] Write the failing disclosure tests in
  `test/source/harness_numa_placement_test.cpp`: a bound context's
  field reads `cpu@group@node` in the console row and in the JSON
  context field; a run with no node preset stands byte-identical to
  master apart from the version figures the report carries
  (FR-011, FR-002, FR-004, R-08)
- [ ] T009 [US1] Write the failing context-start tests in
  `test/source/harness_numa_placement_test.cpp`: the bind stands
  beside the pin at the context start, the binder receives the node
  the planned cpu belongs to, and a user-installed `PlacementFn`
  cannot bypass the bind (FR-010, R-05)

### Implementation for User Story 1

- [ ] T010 [US1] Extend the plan resolver in
  `source/harness/placement_preset.cpp`: the `node` plan resolves the
  mask-touching nodes in the read's node order, cycles them when the
  context count exceeds the node count, fills them in order when the
  count sits below, and the postcondition predicate extends beside
  the standing `planCpusDistinct` at `placement_preset.cpp:56`
  (FR-009, FR-012, D-2, R-03, R-04) (depends on T002, T004, T007;
  the tests of T007 turn green)
- [ ] T011 [US1] Extend `placementText` in
  `source/harness/report.cpp:156`: the field composes the node tail
  from the layout's cpu-to-node map where the plan binds one, and
  keeps the standing `cpu@group` bytes where it does not, with no
  key change in `docs/schema/report-v1.json:350`
  (FR-011, FR-004, D-5, R-08) (depends on T004, T008; the tests of
  T008 turn green)
- [ ] T012 [US1] Add the bind call at the context start in
  `source/harness/execution_context.cpp:434`: the bind stands after
  the placement call, outside the replaceable function, driven by the
  plan's node assignment for the context, through the seam of T006
  (FR-010, D-4, R-05) (depends on T006, T009, T010; the tests of
  T009 turn green)

**Checkpoint**: User Story 1 stands functional and testable
independently: the plan resolves, the bind fires, the field names the
node, and the unbound run stands byte-identical.

---

## Phase 4: User Story 2 - The listing names the memory nodes (Priority: P2)

**Goal**: `--topology` prints one line per memory node under the L3
lines, in the L3 lines' shape, truthful to the hwloc read.

**Independent Test**: with an injected topology, the listing prints
one line per node inside the mask, and a read with no memory
information prints `not available` for the node lines alone.

### Tests for User Story 2 (write first, observe failure)

- [ ] T013 [P] [US2] Write the failing listing tests in
  `test/source/harness_cli_placement_test.cpp`: the node lines stand
  under the L3 lines in the `node <n>: <cpu-list>` shape, in the
  read's node order, mask-filtered the way the L3 lines filter; a
  read with no memory information prints `not available` and keeps
  the rest of the listing byte-identical, exit status 0
  (FR-006, FR-007, FR-008, D-3, R-02)

### Implementation for User Story 2

- [ ] T014 [US2] Extend the listing in `source/harness/topology.cpp`
  beside the L3 lines at `topology.cpp:434`: the node family prints
  from the layout's nodes, mask-filtered, and the
  `memoryReadable`-false branch prints the `not available` wording
  with the rest of the bytes standing (FR-006, FR-007, D-3, R-02,
  R-09) (depends on T004, T013; the tests of T013 turn green)

**Checkpoint**: User Stories 1 and 2 stand independently functional:
the plan binds, the listing names, and the unbound paths stand
byte-identical.

---

## Phase 5: User Story 3 - A plan that cannot bind refuses before it runs (Priority: P2)

**Goal**: every refusal the spec names prints one line and exits 2
before any benchmark output stands.

**Independent Test**: with a fake binder that denies the call, the
run refuses one line, exit status 2, with no benchmark output.

### Tests for User Story 3 (write first, observe failure)

- [ ] T015 [P] [US3] Write the failing refusal tests in
  `test/source/harness_numa_placement_test.cpp`: a denied bind on a
  planned context refuses one line naming the context and the node
  before any benchmark output; the `node` preset over a read with no
  memory information refuses one line before any context starts; the
  standing unfittable-count refusal covers the node plan
  (FR-013, FR-014, FR-015, D-4, R-07, R-10)

### Implementation for User Story 3

- [ ] T016 [US3] Wire the refusal paths: the context start turns a
  failed bind beside the standing pin-failure account path at
  `execution_context.cpp:440` into the one-line status-2 refusal, and
  the preset resolution refuses the no-memory-information case beside
  the standing unfittable-count refusal at `placement_preset.cpp:498`
  (FR-013, FR-014, D-4, R-07, R-10) (depends on T012, T014, T015; the
  tests of T015 turn green)

**Checkpoint**: the refusal set stands green: every refused run
prints one line, exits 2, and stands ahead of the first benchmark.

---

## Phase 6: User Story 4 - The NPS runs stand recorded (Priority: P2)

**Goal**: the results directory carries one record per NPS state the
board's menu offers, each naming the setting, the boot, the commands,
the exit codes and the listing, with the merged pre-check run beside
the placement runs.

**Independent Test**: the results directory carries the records, the
quickstart links each one, and an unreachable state stands recorded
as out of reach with the gap named.

- [ ] T017 [US4] Write the NPS evidence plan in
  `specs/035-numa-placement/results/plan.md`: the BIOS change and
  reboot procedure, the state list the board's menu offers, the
  command set per state, and the record format the states fill
  (FR-019, D-7, R-11)
- [ ] T018 [US4] Record the runs on thrawn: one record per NPS state
  under `specs/035-numa-placement/results/`, each naming the state,
  the boot, the commands, the exit codes and the listing, with the
  merged pre-check run beside the placement runs; a state the menu
  cannot produce stands recorded as out of reach with the gap named
  (FR-019, D-7, R-11) (depends on T012, T017)

**Checkpoint**: the evidence stands recorded: every state the menu
offers carries its record, and the quickstart links them.

---

## Phase 7: User Story 5 - The docs teach the node plans (Priority: P3)

**Goal**: the README, the guide page and the example teach the node
plans, and the docs gates stand green over them.

- [ ] T019 [P] [US5] Add the README section of at most 25 lines and
  the quick-reference rows in `README.md`, and the option and field
  detail in `docs/pages/harness.md` beside the placement sections
  spec 025 landed (FR-020, FR-021, R-12)
- [ ] T020 [US5] Add the runnable example
  `example/docs/numa_placement.cpp` with its `.expected` file, a
  fixed context count of two with the `node` preset; register it in
  `example/CMakeLists.txt` and the example lists, and add the
  `SECTIONS` and `SECTION_OWNERS` rows in
  `tools/docs/example_drift.py` (FR-022, R-12) (depends on T012)
- [ ] T021 [US5] Record the shape-gate rule and the fixture pair for
  the node tail in
  `specs/035-numa-placement/contracts/report-shape.md`'s fixture
  table and in `test/example-drift-fixture/run_report_shape_fixtures.py`:
  `cpu@#` and `cpu@#@#` fold different, checked in both directions
  (FR-022, D-5) (depends on T020)

**Checkpoint**: the docs stand green: the drift, snippet-compile,
example-run and report-shape gates read no finding.

---

## Phase 8: Polish and Cross-Cutting Concerns

**Purpose**: the hard gates and the records that outlive the branch.

- [ ] T022 [P] Run the format, spell, tidy and cppcheck gates at the
  head: `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake`,
  `cmake -P cmake/spell.cmake`, the `ci-ubuntu` build with the
  analyzers wired in; every finding stands fixed
  (constitution V, VIII)
- [ ] T023 [P] Run the sanitizer, tsan and coverage suites:
  `ctest --test-dir build/sanitize`, `ctest --test-dir build/tsan`,
  and the coverage contract `ctest --test-dir build/coverage -j 1`
  with the gate reading 100.0% lines and a branch shortfall inside
  the D-14 recorded allowance, no `LCOV_EXCL` marker and no new
  `--ignore-errors` entry (FR-018, SC-004, constitution VI)
- [ ] T024 [P] Run the prose, DBC and time-source gates over the
  branch range, and the suite under `taskset -c 0,1` and on a 2-cpu
  single-node host: `ctest --test-dir build -j 1` stands green under
  both (FR-017, constitution VIII, XI)
- [ ] T025 [US4] Verify the quickstart scenarios of
  [quickstart.md](./quickstart.md) end to end on the lane host, and
  record the commands, the exit codes and the figures under
  `specs/035-numa-placement/results/quickstart.md`
  (SC-001 to SC-006) (depends on T016, T018, T021)

**Checkpoint**: every hard gate stands green at the head, and the
evidence stands recorded.

---

## Phase 9: Convergence

**Purpose**: the closing evidence the lane lands with.

- [ ] T026 Read the CI run of the pushed head end to end: every job
  green (`lint`, `test`, `test-rocky`, `sanitize`, `tsan`,
  `static-analysis`, `coverage`, `dbc-gate`, `prose-lint`,
  `shared-audit`, `downstream-consumer`), the coverage figures read
  100.0% lines and a branch shortfall inside the D-14 allowance, and
  the run reference stands recorded beside the acceptance evidence in
  `specs/035-numa-placement/results/quickstart.md` (SC-001 to
  SC-006) (depends on T022 to T025)
- [ ] T027 Re-check the constitution table of [plan.md](./plan.md)
  against the landed tree: every gate stands as the table states it,
  the Complexity Tracking table stands empty, and the record stands
  in the pull-request body (constitution Governance) (depends on
  T026)

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: no dependency; starts immediately
- **Foundational (Phase 2)**: depends on Setup; blocks every user
  story
- **User Story 1 (Phase 3)**: depends on Foundational
- **User Story 2 (Phase 4)**: depends on Foundational; stands
  independent of User Story 1
- **User Story 3 (Phase 5)**: depends on User Story 1's bind and
  User Story 2's listing (the refusal paths wire beside them)
- **User Story 4 (Phase 6)**: depends on User Story 1's runs and the
  plan task T017
- **User Story 5 (Phase 7)**: the example depends on the disclosure;
  the docs rows stand parallel
- **Polish (Phase 8)**: depends on every story standing green
- **Convergence (Phase 9)**: depends on Polish

### Within Each User Story

- Tests stand written and observed failing before the code task that
  turns them green
- The machine read before the plan; the plan before the bind; the
  bind before the disclosure consumers

### Parallel Opportunities

- T001, T003 and T005 run in parallel (different files)
- T007 and T008 run in parallel; T013 runs beside Phase 3's tail
- The docs rows of T019 stand beside the implementation waves

---

## Parallel Example: Phase 2

```text
Task: "Write the failing node-walk tests in test/source/harness_numa_placement_test.cpp"
Task: "Write the failing binder-seam tests in test/source/harness_numa_placement_test.cpp"
```

Both stand in one new executable but touch disjoint scenario blocks,
so the two test tasks and their code tasks stand pairable.

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup
2. Complete Phase 2: Foundational
3. Complete Phase 3: User Story 1
4. **STOP and VALIDATE**: the node plan binds and names, and the
   unbound runs stand byte-identical

### Incremental Delivery

1. Setup + Foundational: the read states the nodes, the seam stands
2. User Story 1: the plan binds and discloses (MVP)
3. User Story 2: the listing names the nodes
4. User Story 3: the refusal set closes
5. User Story 4: the NPS evidence lands
6. User Story 5: the docs stand recorded
7. Polish and convergence: the gates and the CI run close the lane

---

## Notes

- [P] tasks stand in different files or disjoint blocks
- [Story] labels map the tasks to the spec's user stories
- Every test task stands observed failing before its code lands
- Commit after each task or logical group, with the standing commit
  template
- Stop at any checkpoint to validate the story independently
