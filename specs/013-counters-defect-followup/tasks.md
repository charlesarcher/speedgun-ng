# Tasks: Counters Defect Follow-Up

**Input**: Design documents from
`/specs/013-counters-defect-followup/`

**Prerequisites**: plan.md (required), spec.md (required for user
stories), research.md, data-model.md, contracts/, quickstart.md

**Tests**: Every task that corrects a defect ships a test. FR-023
requires each covering test to fail at the pre-fix head `0dea082` and
pass after the correction, and plan.md records TDD mode as in force
(FR-035, Principle III). Test tasks come first inside every story
phase, and each names the command that observes the red state. Every
red state is observed at the current head with the correction
unapplied. No task checks out the pre-fix head to observe red, because
that commit holds no copy of the test being observed. The exception is
T001, which builds the pre-fix head in a separate worktree to capture
the sampling-cost baseline, and that task reads a binary where a test
would read a source.

**Organization**: Tasks are grouped by user story to enable independent
implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (US1 through US7)
- Include exact file paths in descriptions

## Path Conventions

Single project. Paths are repository-root relative.

- Library headers: `include/speedgun-ng/`
- Library sources: `source/counters/`, `source/counters/linux_pmu/`
- Tests: `test/source/`, registered in `test/CMakeLists.txt`
- Build and package: `CMakeLists.txt`, `CMakePresets.json`,
  `cmake/install-rules.cmake`
- Continuous integration: `.github/workflows/ci.yml`
- Documentation: `docs/pages/`
- Successor log: `specs/007-counters-and-timers/citations-log.md`

### Reading order for every task

1. `specs/013-counters-defect-followup/spec.md` for the requirement
2. `specs/013-counters-defect-followup/research.md` for the decision
   behind it, cited by `D-NN`
3. `specs/013-counters-defect-followup/contracts/` for the doxygen
   clause and its paired enforcement site
4. `specs/013-counters-defect-followup/plan.md` for the verification
   matrix row that names the covering test
5. `specs/013-counters-defect-followup/quickstart.md` for the
   verification command

### Rules that bind every task

- One commit per task, or per coherent group the task names. The commit
  carries `<Section>: <imperative>` at 50 characters or fewer, a
  why-body at 72 columns, `Approved-by:`, and
  `Refs: specs/013-counters-defect-followup`.
- Inside a test-then-code pair, the failing test lands first and the
  correction second.
- Every changed line traces to a requirement (FR-025, Principle X.3).
- This feature adds no coverage-exclusion marker in
  `source/counters/`, and the pre-existing clang-tidy and
  coverage-exclusion backlog on lines this feature does not touch stays
  out of scope (FR-033).
- Verify after every task with
  `cmake --build --preset=dev && ctest --preset=dev --output-on-failure`.
- A task closed only against the dev preset stays open. Each feature
  builds once in its release preset before its tasks are called done
  (Principle IX).

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Capture every pre-fix figure the specification names, so
each later task has a baseline to compare against and no baseline is
written after the change it gates.

- [X] T001 [P] Build the release preset in a separate worktree at the
  pre-fix head `0dea082`, run `./build/test/counters_overhead` there,
  and record the plan median and the clock-leaf median into the step 8
  table in `specs/013-counters-defect-followup/quickstart.md`. Take
  each median under the run method the overhead page already documents
  and record that method beside both figures, because T040 measures
  under the same method and the two figures cannot be compared
  otherwise (FR-004, FR-027, Principle VII)
- [X] T002 [P] Record the build tree's own catalog figures at `0dea082`
  by running `./build/test/counters_pmu_test` in the T001 worktree and
  copying its `pmu catalog`, `pmu availability`, and `target masks`
  lines into the step 5 table in
  `specs/013-counters-defect-followup/quickstart.md`. Those three
  lines are what the target-mask and availability assertions compare
  against (FR-013, FR-016, SC-005, SC-009)
- [X] T003 [P] Count the coverage-exclusion markers and the clang-tidy
  warnings in `source/counters/` at `0dea082` with
  `grep -rc 'LCOV_EXCL' source/counters/` and
  `cmake --build build/dev -t dbc-gate -j 2`, and record both counts in
  the step 9 table in
  `specs/013-counters-defect-followup/quickstart.md`. FR-033 holds each
  count flat, and a count written after the change would measure the
  change
- [X] T004 [P] Confirm the audit point's public surface is what the
  specification describes by running
  `ctest --test-dir build/dev -R 'counters_recorder_test|counters_objects_test|counters_linux_pmu_seam_test|counters_pmu_test' --output-on-failure`
  and recording that all four pass at `0dea082`. This is the green
  baseline SC-010 names as the gate that was missing: each of the four
  passes today, and each fails once rewritten (FR-023, SC-010)

**Checkpoint**: Every baseline figure the later tasks compare against is
written down, and each is written before the change it gates.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Give every window that owns a leaf its own disclosure
column, and give a fold result and a raw view a public availability
field. US1, US2, and US7 all read the mechanism this phase adds.

- [X] T005 Rewrite the multiplex-window fixture in
  `test/source/counters_linux_pmu_seam_test.cpp` so its event-page
  writer gains the fields the kernel time recipe reads, `cap_user_time`,
  `time_enabled`, `time_running`, `time_shift`, `time_mult`,
  `time_offset`, `cap_usr_time_short`, `time_cycles`, and `time_mask`.
  Drive two pages: one stating the full form with a non-trivial scale,
  offset, and multiplier, and one stating `cap_usr_time_short` with a
  cycle value needing the short-counter correction. Observe the red
  state with
  `cmake --build --preset=dev && ./build/dev/test/counters_linux_pmu_seam_test`,
  which fails on the first ratio assertion because
  `fast_context_time_pair` copies the raw pair and the fixture today
  has no field to set (FR-007, FR-008, FR-009, FR-023, SC-004, SC-010,
  D-04, D-13)
- [X] T006 Rewrite the recorder gap fixture in
  `test/source/counters_recorder_test.cpp` so the leaves under
  `register_everything` script cumulative counts above zero before any
  action gaps, using `fake_provider::set_gap_actions` at
  `include/speedgun-ng/counters_fake.hpp:224`. Script three windows: a
  gap at the end point, a gap at the start point, and a gap strictly
  inside. Observe the red state with
  `cmake --build --preset=dev && ./build/dev/test/counters_recorder_test`,
  which fails because the fixture today starts every count at zero, so
  the zero the gap writes equals the first real point and no assertion
  can distinguish them (FR-001, FR-003, FR-023, SC-001, SC-010, D-01,
  D-13)
- [X] T007 Add one disclosure column per read group to the compiled
  layout in `source/counters/detail/core.hpp` and
  `source/counters/plan.cpp:542-568`, replacing the single
  `disclosure_slot` with one slot per group, and update `leaf_count` so
  it counts every managed column. Add the doxygen clause from
  `contracts/gap-state.md` beside the field and its `SG_ENSURE`. Keep
  `point_sink::put` at `include/speedgun-ng/counters_provider.hpp:212`
  one integer wide, which is FR-035's obligation. Verify with
  `cmake --build --preset=dev && ctest --preset=dev -R counters --output-on-failure`,
  which stays green because the wider per-window state lives in the
  layout and in one extra column per window (FR-002, FR-024, FR-035,
  Principle II, Principle VII, D-03)
- [X] T008 Add the `availability` field to `metric_result` at
  `include/speedgun-ng/counters_core.hpp:265-270` and to `points_view`
  at `include/speedgun-ng/counters_measurement.hpp:388-403`, with the
  doxygen clause from `contracts/gap-state.md` and an `SG_REQUIRE`
  guarding each. The field holds the same value the disclosure column
  holds for the same action, and the enumeration itself takes no new
  value, which is FR-035's second obligation. Verify with
  `cmake --build --preset=dev && ctest --preset=dev --output-on-failure`
  and
  `ctest --test-dir build/dev -R counters_header_purity --output-on-failure`,
  the latter holding without an edit because `availability` is already
  declared in `counters_core.hpp` (FR-004, FR-005, FR-024, FR-035,
  Principle II, D-02)

**Checkpoint**: Every window owns a disclosure column, a caller reads
the gap state through the public surface without naming a column index,
and the two rewritten tests T005 and T006 are red. US1, US2, and US7
depend on this phase and on nothing else in it.

---

## Phase 3: User Story 1 - Receive a fold whose value no gap point
computes (Priority: P1)

**Goal**: A fold whose start point or whose end point carries the gap
mark reports no value computed from the zero that mark wrote, and
discloses the gap state. A gap strictly inside a window leaves both end
points measured and the delta between them correct, because the
recorded counts are cumulative.

**Why this priority**: Every harness benchmark folds over millions of
points, so every wrong value reaches every report.

**Independent test**: `./build/dev/test/counters_recorder_test` fails
before T009 and passes after T010, and the three gap windows report the
states FR-001 names.

### Tests for User Story 1

- [X] T009 [US1] Add the three gap-window assertions to the rewritten
  fixture in `test/source/counters_recorder_test.cpp`, reading each
  fold result's `availability` field from T008 and each raw view's
  field. Assert that the two end-point folds report no value and
  disclose `availability::gap`, and that the inside fold reports the
  hand-computed delta between its measured end points. Observe the red
  state with
  `cmake --build --preset=dev && ./build/dev/test/counters_recorder_test`,
  which fails on the end-point folds because the fold subtracts across
  the gap and reads the mark at the end point alone (FR-001, FR-002,
  FR-003, FR-004, SC-001)

### Implementation for User Story 1

- [X] T010 [US1] Make every delta path in `source/counters/fold.cpp`
  read the disclosure mark at both the start point and the end point,
  through the per-window column T007 added, and report
  `availability::gap` with no value when either end point carries it.
  Add the `SG_ENSURE` that forbids a value over a window carrying an
  end-point gap. Verify with
  `cmake --build --preset=dev && ctest --preset=dev -R counters_recorder_test --output-on-failure`,
  which turns green on T009's three assertions (FR-001, FR-003,
  FR-004, Principle II, Principle VII, D-01)
- [X] T011 [US1] Replace the unconditional ratio fallback at
  `source/counters/fold.cpp:291` with a read of the public
  `running_ratio` beside the new `availability` field, and add the
  doxygen clause recording that a gap window publishes no ratio claim.
  Verify with
  `cmake --build --preset=dev && ctest --preset=dev -R counters_recorder_test --output-on-failure`
  and the full `ctest --preset=dev` (FR-004, FR-005, D-02)

**Checkpoint**: US1 is fully functional and testable independently. A
caller receives either a correct count or a disclosed gap, and every
gate in `source/counters/fold.cpp` holds.

---

## Phase 4: User Story 2 - Read the gap state of any window through the
public surface (Priority: P1)

**Goal**: A caller reads the availability field of a fold result and of
a raw view without naming a column index in its source, and a plan
holding PMU leaves and clock leaves reaches the fold result under both
provider registration orders.

**Why this priority**: The harness reports each value beside its gap
state, so a value without a state is unreadable.

**Independent test**: `./build/dev/test/counters_objects_test` fails
before T012 and passes after T013, under both registration orders.

### Tests for User Story 2

- [X] T012 [US2] Extend the fixture in
  `test/source/counters_objects_test.cpp:416-464` to hold a PMU leaf
  under `package-1/core-3` and a clock leaf under `machine`, drive a PMU
  gap with `fake_provider::set_gap_actions`, and fold the pair under
  both registration orders: PMU first and clock second, then clock
  first and PMU second. Read the fold result's `availability` field in
  both orders and read a raw view's field with no column index in the
  test's source. Observe the red state with
  `cmake --build --preset=dev && ./build/dev/test/counters_objects_test`,
  which fails in the clock-second order because the compile names the
  one window owning the plan's last managed leaf as the disclosure
  writer, so the clock's mark overwrites the PMU's (FR-002, FR-004,
  FR-005, FR-006, SC-002, SC-003, D-03)

### Implementation for User Story 2

- [X] T013 [US2] Resolve each group's disclosure column from that
  group's own offset, with no read of the plan's last managed column, in
  `source/counters/plan.cpp:542-568`, and add the `SG_REQUIRE` that a
  group's disclosure column sits inside its own slot range. Verify with
  `cmake --build --preset=dev && ctest --preset=dev -R counters_objects_test --output-on-failure`,
  which turns green on both registration orders in T012 (FR-002,
  FR-006, Principle II, D-03)

**Checkpoint**: US2 is fully functional and testable independently, and
it holds on T008's public field without naming a column index.

---

## Phase 5: User Story 3 - Read the multiplex ratio the kernel recipe
produces (Priority: P1)

**Goal**: The disclosed ratio over a multiplexed window follows the
kernel's own time computation, including the short-counter correction,
so a caller reads the value the kernel produced.

**Why this priority**: Every multiplexed fast-mode metric reports its
running ratio to the caller.

**Independent test**: `./build/dev/test/counters_linux_pmu_seam_test`
fails on T005's fixture and passes after T015, and the disclosed ratio
matches the kernel header's own computation within one tick.

### Tests for User Story 3

- [X] T014 [US3] Add the short-counter-form assertion to the rewritten
  multiplex-window scenario in
  `test/source/counters_linux_pmu_seam_test.cpp`, taking the page whose
  `cap_usr_time_short` is set and computing the expected pair by hand
  from the header's own short-counter correction. Observe the red
  state with
  `cmake --build --preset=dev && ./build/dev/test/counters_linux_pmu_seam_test`,
  which fails because the reader copies the raw pair and applies no
  correction (FR-008, SC-004, D-04)
- [X] T015 [US3] Add the full-form ratio assertion to the same
  scenario, computing the expected pair by hand from the header's own
  documented computation, and assert the disclosed ratio within one
  tick. Observe the red state with
  `cmake --build --preset=dev && ./build/dev/test/counters_linux_pmu_seam_test`,
  which fails because the reader copies the raw pair and applies no
  documented computation (FR-007, FR-009, SC-004, D-04)

### Implementation for User Story 3

- [X] T016 [US3] Make the fast-window reader in
  `source/counters/linux_pmu/fast_read.cpp` take the page's cycle
  counter and the scale, offset, and shift fields inside the sequence
  loop, apply the header's documented computation, add the computed
  delta to the enabled pair, add it to the running pair where the page
  index is non-zero, and apply the short-counter correction first where
  the page states it. Add the `SG_ENSURE` beside the function and the
  doxygen clause from `contracts/register-filter.md`. Verify with
  `cmake --build --preset=dev && ctest --preset=dev -R counters_linux_pmu_seam_test --output-on-failure`,
  which turns green on T014 and T015 (FR-007, FR-008, FR-009, FR-024,
  Principle II, Principle VII, D-04)

**Checkpoint**: US3 is fully functional and testable independently. The
disclosed ratio is the kernel's own computation. A raw pair is no longer
what the caller receives.

---

## Phase 6: User Story 4 - Count an event the table filters through a
model-specific register (Priority: P1)

**Goal**: A row whose register value is not zero encodes that filter, a
paired index publishes under its first index, and the counter-constraint
and deprecation keys carry no encoding obligation. On a device that
publishes the format, the row counts; on a device that publishes no such
format, the row publishes `not_encodable`.

**Why this priority**: On an Intel host the harness reports offcore,
load-latency, and frontend metrics, and today each is either unencoded
or misrouted.

**Independent test**: `./build/dev/test/counters_linux_pmu_seam_test`
fails before T017 and passes after T021 and T022, with the
per-directory encodable counts matching the pinned figures.

### Tests for User Story 4

- [X] T017 [US4] Grow the synthetic core device at
  `test/source/counters_linux_pmu_seam_test.cpp` with the `ldlat` and
  `frontend` entries, at the bit ranges the kernel publishes for them,
  and extend the encodable-row helper to recount per directory. Pin the
  corrected counts: skylake 581, icelake 346, alderlake 563,
  sapphirerapids 1993, amdzen4 326, amdzen5 322. Measured over the
  pinned tree at this head. Two figures in the prediction this line
  carried were wrong. alderlake was predicted at 564 and measures 563;
  the alderlake tree publishes 563 rows in all, so no count above that
  is reachable. sapphirerapids was predicted at 2141 and measures
  1993. The 148-row gap is two keys outside this feature's scope: 293
  rows name a `PortMask` and 250 name an `FCMask`, and both correctly
  publish `not_encodable` after both decisions land. Observe the red state
  with
  `cmake --build --preset=dev && ./build/dev/test/counters_linux_pmu_seam_test`,
  which fails the register-filter assertions because no filter reaches
  the encoding, so every row naming a register publishes `countable`
  with no filter, and fails the count assertions because the fixture
  publishes one register format and not the other two (FR-010, FR-012,
  FR-013, SC-005, SC-006, D-05, D-07)
- [X] T018 [US4] Add the paired-index assertion to the same file,
  resolving a paired register value through its first index alone, and
  assert that no row's encoding differs between the two spellings the
  pinned tree carries. Observe the red state with
  `cmake --build --preset=dev && ./build/dev/test/counters_linux_pmu_seam_test`,
  which fails because the parser holds no index map and encodes
  nothing from either field (FR-010, SC-005, D-05)
- [X] T019 [US4] Add the no-obligation-key recount to the same file,
  asserting that no row fails to encode because of a counter-constraint
  key or a deprecation key, and that the four Intel directory counts
  rise to the figures T017 pins. Observe the red state with
  `cmake --build --preset=dev && ./build/dev/test/counters_linux_pmu_seam_test`,
  which fails on the rows whose encoding a counter-constraint key or a
  deprecation key withholds today (FR-012, FR-013, SC-006, D-06)
- [X] T020 [US4] Add the encoding-key coverage assertion to the same
  file, asserting that every encoding key the kernel's own table
  generator maps to an event field is encodable, using the pinned
  register-value spellings the tree carries across its four Intel
  directories. Observe the red state with
  `cmake --build --preset=dev && ./build/dev/test/counters_linux_pmu_seam_test`,
  which fails on every key the parser resolves to nothing today
  (FR-011, SC-005, D-05)

### Implementation for User Story 4

- [X] T021 [US4] Add the register index map and the no-obligation key
  set to `source/counters/linux_pmu/table_parse.cpp`, record the
  format name beside the register value and index on the parsed row, and
  apply the map at the parse site so a pair lands on its first index.
  Add the doxygen clause from `contracts/register-filter.md` and the
  `SG_ENSURE` that a stored row names a format rather than a range.
  Verify with
  `cmake --build --preset=dev && ctest --preset=dev -R counters_linux_pmu_seam_test --output-on-failure`,
  which turns green on T017, T018, and T020 (FR-010, FR-011, FR-024,
  Principle II, D-05)
- [X] T022 [US4] Carry the resolved filter into the encoding in
  `source/counters/linux_pmu/group_io.cpp`, replacing the branch that
  skips a row carrying a register filter, and resolve a format name
  against the device's own published format list through
  `pmu_compose_config` at `source/counters/linux_pmu/encode.cpp:132-159`.
  A device publishing no such format publishes the row as
  `not_encodable`, and the counter-constraint and deprecation keys
  carry no encoding obligation. Add the `SG_ENSURE` beside the
  composition and the doxygen clause recording that the range belongs
  to the device. Verify with
  `cmake --build --preset=dev && ctest --preset=dev -R counters_linux_pmu_seam_test --output-on-failure`,
  which turns green on T017 and T019 (FR-010, FR-012, FR-013, FR-024,
  Principle II, Principle VII, D-05, D-06, D-07)

**Checkpoint**: US4 is fully functional and testable independently. No
row with a non-zero register value publishes as countable without its
filter, and the pinned counts hold on any host.

---

## Phase 7: User Story 5 - Count an uncore event on every instance its
unit names (Priority: P2)

**Goal**: An uncore row reaches every instance the generator's unit map
names, a core device receives no uncore row, and the vendor-named AMD
devices receive their own rows.

**Why this priority**: Uncore metrics, memory bandwidth per package
among them, are the reason a user reads a PMU catalog at all.

**Independent test**: `./build/dev/test/counters_pmu_test` fails before
T023 and passes after T024.

### Tests for User Story 5

- [X] T023 [US5] Add the synthetic device-tree fixtures to
  `test/source/counters_pmu_test.cpp`, naming a numbered cache instance,
  a second numbered cache instance, a numbered memory instance, and the
  three vendor-named AMD devices, and assert each receives the rows its
  unit names while no uncore row appears on a core device. Observe the
  red state with
  `cmake --build --preset=dev && ./build/dev/test/counters_pmu_test`,
  which fails because the provider routes one row to one device by
  name, so the second instance of each unit receives nothing (FR-014,
  FR-015, FR-023, SC-008, D-08, D-09)

### Implementation for User Story 5

- [X] T024 [US5] Resolve a table's unit through a reachability
  predicate in `source/counters/linux_pmu/provider.cpp`, mapping a
  generic unit name to every instance the generator's unit map names
  and a specific instance to that instance alone, and stop deciding
  scope by name. Add the doxygen clause from
  `contracts/device-routing.md` and the `SG_REQUIRE` beside the
  resolution. Keep `provider::catalog()` `noexcept` and the resolution
  allocation-free per entry. Verify with
  `cmake --build --preset=dev && ctest --preset=dev -R counters_pmu_test --output-on-failure`
  (FR-014, FR-015, FR-024, Principle II, Principle VII, D-08, D-09)

**Checkpoint**: US5 is fully functional and testable independently, and
no single host reaches the uncore fixtures.

---

## Phase 8: User Story 6 - Count a model-specific register event on a
per-thread target (Priority: P2)

**Goal**: A model-specific-register device whose per-thread probe
succeeds publishes the thread target bit on its entries, each device
carries its own fast verdict, and a refused fast read discloses a gap
with no syscall read behind it.

**Why this priority**: A countable per-thread event is published as out
of scope today, so a caller loses the metric or reads it wrong.

**Independent test**: `./build/dev/test/counters_pmu_test` fails before
T025 and passes after T027, and the refused-read assertion in the seam
test passes after T026.

### Tests for User Story 6

- [X] T025 [US6] Add the per-thread-target and fast-verdict assertions
  to `test/source/counters_pmu_test.cpp`, naming a
  model-specific-register device whose per-task probe succeeds and
  asserting the thread bit on its entries, then a synthetic host
  holding a core device and an uncore device and asserting the fast
  mode lands on the core entries alone, then a device whose page
  refuses the fast read. Observe the red state with
  `cmake --build --preset=dev && ./build/dev/test/counters_pmu_test`,
  which fails on all three, because the scope decision takes one
  host-wide verdict and the fast verdict names no device
  (FR-016, FR-017, FR-018, FR-023, SC-007, SC-009, D-08, D-10)
- [X] T026 [US6] Add the refused-fast-read assertion to
  `test/source/counters_linux_pmu_seam_test.cpp`, driving a device
  whose page refuses the fast read and asserting a disclosed gap with
  no syscall read behind it. Observe the red state with
  `cmake --build --preset=dev && ./build/dev/test/counters_linux_pmu_seam_test`,
  which fails because the refusal falls through to the group read
  (FR-019, SC-007, D-10)

### Implementation for User Story 6

- [X] T027 [US6] Take the fast verdict per device from that device's
  own event page in `source/counters/linux_pmu/provider.cpp`, publish
  the fast mode on the entries whose device grants it, and take the
  scope decision from the data the kernel publishes for that device or
  from the per-thread probe. Keep a refused fast read a disclosed gap,
  with no syscall read behind it. Add the doxygen clause from
  `contracts/device-routing.md` and its paired `SG_ENSURE`. Verify with
  `cmake --build --preset=dev && ctest --preset=dev -R 'counters_pmu_test|counters_linux_pmu_seam_test' --output-on-failure`
  (FR-016, FR-017, FR-018, FR-019, FR-024, Principle II, D-08, D-10)

**Checkpoint**: US6 is fully functional and testable independently, and
each device carries its own verdict.

---

## Phase 9: User Story 7 - Install the package and take the count the
build tree took (Priority: P3)

**Goal**: The package config reports the minor-version compatibility
while the project major version stays 0, the project version reads
0.4.0, and one continuous-integration step compares the installed
consumer's catalog count with the build tree's count on the same runner.

**Why this priority**: The harness links the installed package, so a
count that differs from the build tree's makes every number in a
published report unattributable.

**Independent test**: `cmake --preset=ci-ubuntu && cmake --build build`
installs the package, and the installed consumer's printed count
matches the build tree's figure from T002.

### Tests for User Story 7

- [X] T028 [US7] Record the observed compatibility value and the
  project version at `0dea082` in the step 7 table in
  `specs/013-counters-defect-followup/quickstart.md`, read from
  `cmake/install-rules.cmake` and `CMakeLists.txt`, and add the
  one-step count comparison to `.github/workflows/ci.yml` beside the
  consumer run step, comparing the installed count against the build
  tree's figure from T002 on the same runner. Observe the red state by
  reading the workflow step and confirming no comparison exists yet, so
  the state to be corrected is a count difference the step would report
  on a runner whose two figures disagree (FR-020, FR-021, FR-022,
  SC-011, SC-012, D-11, D-12)

### Implementation for User Story 7

- [X] T029 [US7] Set the package config compatibility to
  `MinorVersion` in `cmake/install-rules.cmake`, and set the project
  version to 0.4.0 with an explicit shared-object version of 1 at
  `CMakeLists.txt:7` and `:41`, because version 0.4.0 keeps the major
  position at 0 and the 0.x rule D-11 names needs the shared-object
  version stated. The major position supplies nothing. Verify
  with
  `cmake --preset=ci-ubuntu && cmake --build build && grep -F 'PACKAGE_VERSION' build/prefix/lib/cmake/speedgun-ng/speedgun-ngConfigVersion.cmake`
  (FR-020, FR-021, Principle II, D-11)
- [X] T030 [US7] Record the version lineage in the section
  `specs/013-counters-defect-followup/plan.md` holds under
  `Version lineage`, naming each declaration commit `cd5cbd1` removed
  beside the 0.4.0 bump that records its removal, and restoring
  neither. Verify with
  `grep -c 'cd5cbd1' specs/013-counters-defect-followup/plan.md`,
  which returns 2 or more (FR-020, FR-032, SC-011, D-11)
- [X] T031 [US7] Add the comparison step to
  `.github/workflows/ci.yml` after the consumer run step, running both
  figures on the same runner and exiting non-zero on a difference.
  Verify by reading `git diff -- .github/workflows/ci.yml` and
  confirming the step names both counts and compares them
  (FR-022, SC-012, D-12)

**Checkpoint**: US7 is fully functional and testable independently. The
installed consumer's count matches the build tree's on the same runner.

---

## Phase 10: Polish and Cross-Cutting Concerns

**Purpose**: The obligations that span every story. None of them adds
behavior; each one is a gate, a record, or a check.

- [X] T032 [P] Record one entry per correction in
  `specs/007-counters-and-timers/citations-log.md`, in the format that
  file's entry-format section records, each naming the 007 or 012
  requirement it restores, its measuring command, and its head. Leave
  the frozen record at `specs/007-counters-and-timers/citations.md`
  unedited (FR-030, FR-031, D-14)
- [X] T033 [P] Confirm the Intel confirmation deferral is recorded in
  `specs/013-counters-defect-followup/plan.md` and
  `specs/013-counters-defect-followup/research.md`, naming the host
  class that would settle it, and that the availability of the three
  register formats is the one figure that depends on the running kernel
  (FR-023, D-07)
- [X] T034 [P] Run the header purity scan with
  `ctest --test-dir build/dev -R counters_header_purity --output-on-failure`,
  confirming that no public header gained a platform term and that the
  scan needs no edit (FR-029)
- [X] T035 [P] Confirm the counters classes stay embeddable by building
  the standalone example targets with
  `cmake --build build/dev --target counters_standalone_example counters_giraffe_example`,
  and confirming no benchmark-framework code reaches them (FR-028)
- [X] T036 Run the design by contract gate with
  `cmake --build build/dev -t dbc-gate -j 2` and
  `ctest -R dbc_gate_fixtures --output-on-failure --no-tests=error -j 2`,
  confirming every changed interface keeps its doxygen clause paired
  with its registered enforcement (FR-029, FR-025)
- [X] T037 Run the spelling, prose, and format gates with
  `cmake -P cmake/spell.cmake`,
  `python3 tools/prose/prose_gate.py --check prose --mode tree`,
  `cmake --build build/dev -t prose-lint-fixtures`, and
  `ctest --test-dir build/dev -R prose_gate_fixtures --output-on-failure`,
  confirming the whole tree stays at zero findings (FR-034, FR-025)
- [X] T038 Run the address and thread sanitizer presets with
  `cmake --preset=ci-sanitize && cmake --build --preset=ci-sanitize -j 2 && ctest --preset=ci-sanitize`
  and then
  `cmake --preset=ci-tsan && cmake --build --preset=ci-tsan -j 2 && ctest --preset=ci-tsan`,
  confirming the catalog resolution under concurrent routing reports no
  race (FR-025)
- [X] T039 Run the coverage gate with
  `cmake --preset=ci-coverage && cmake --build build/coverage -j 2 && ctest --test-dir build/coverage --output-on-failure --no-tests=error && cmake --build build/coverage -t coverage`,
  confirming 100 percent line, branch, and contract coverage, and
  recounting `grep -rc 'LCOV_EXCL' source/counters/` against the T003
  figure and each touched translation unit's clang-tidy count against
  its T003 figure, so both are held flat (FR-033, FR-025)
- [X] T040 [P] Re-measure the sampling cost with
  `cmake --preset=dev && cmake --build --preset=dev -j 2 && ctest --test-dir build/dev -R counters_overhead --output-on-failure`
  under the T001 run method, confirm the release-build median stays
  within five percent of the T001 figure, and record the new figure in
  `docs/pages/counters-overhead.md` beside the one it replaces, with
  the same recipe that page documents. Run
  `ctest --test-dir build/dev -R counters_noalloc_test --output-on-failure`
  in the same step to confirm `recorder::sample()` stayed `noexcept`
  and allocation-free (FR-004, FR-024, FR-027, Principle VII)
- [X] T041 [P] Run the full suite with
  `ctest --test-dir build/dev --output-on-failure --no-tests=error -j 2`
  and confirm every counters test passes on the corrected tree, runs
  unprivileged, and is not skipped by a skip-return code a synthetic
  fixture would have hit (FR-023, FR-026, FR-025)
- [X] T042 Build the release preset once with
  `cmake --preset=ci-ubuntu && cmake --build build -j "$(nproc)" && ctest --test-dir build -C Release --output-on-failure --no-tests=error -j 2`,
  which an unoptimized build cannot substitute for (FR-025,
  Principle IX)
- [X] T043 [P] Confirm the successor-log entry names each
  cross-artifact correction the audit found, including the encodable
  count arithmetic that D-05 leaves unmoved, and that
  `specs/007-counters-and-timers/citations.md` still reports 1385
  lines (FR-030, FR-031, D-14)
- [X] T044 Re-read this task list against
  `specs/013-counters-defect-followup/spec.md` and confirm every one of
  FR-001 through FR-035 and SC-001 through SC-013 names a task above,
  and that every task above names a requirement the specification
  states (FR-031, FR-025, Principle X.3)
  The spelling and format gates pass over every file this feature
  changed. The format gate fails on 18 files across the tree and
  failed on 21 at the audit point, so the residue is pre-existing
  and outside this feature. Measured per file with
  `clang-format --output-replacements-xml`, three files carried
  replacements this feature introduced (the seam test 63 against 11,
  the recorder test 3 against 0, and the new registration-order
  test 5 against 0) and each now reports 0. The four remaining
  counters files report the same count at the audit point as they
  do now (FR-029, FR-030, FR-031, D-14).

---

## Dependencies

### Story completion order

Every story depends on Phase 2 and on nothing in another story, so the
seven stories are independent of each other and may run in any order
after the foundational phase. US1 and US2 read the fold and compile
paths that Phase 2 changes, so neither starts before Phase 2 lands.

```text
Phase 1 (T001..T004)
    |
Phase 2 (T005..T008)
    |
    +-- US1 (T009..T011)  --|
    +-- US2 (T012..T013)  --|
    +-- US3 (T014..T016)  --|  independent of each other,
    +-- US4 (T017..T022)  --|  each needs Phase 2 alone
    +-- US5 (T023..T024)  --|
    +-- US6 (T025..T027)  --|
    +-- US7 (T028..T031)  --|
    |
Phase 10 (T032..T044) needs every story above
```

### Cross-story file touches

`source/counters/plan.cpp` carries T007 and T013, so US2 follows the
phase that lands T007. `test/source/counters_linux_pmu_seam_test.cpp`
carries T005, T014, T015, T017, T018, T019, T020, and T026, so one
owner takes US3 and US4 in sequence and keeps T026 for the same owner.
`test/source/counters_pmu_test.cpp` carries T025, and
`source/counters/linux_pmu/provider.cpp` carries T024 and T027, so one
owner takes US5 and US6 in sequence.
`source/counters/fold.cpp` carries T010 and T011 and belongs to US1
alone. `cmake/install-rules.cmake` and `.github/workflows/ci.yml` carry
US7 alone.

### Parallel execution examples

```text
Phase 2:     T005 and T006 in parallel, since the two fixtures are
             different files; then T007 and T008 in parallel, since
             source/counters/detail/core.hpp and plan.cpp are disjoint
             from include/speedgun-ng/counters_core.hpp and
             counters_measurement.hpp.

US3 + US4:   T014 and T015 in parallel, since the two assertions sit in
             different scenarios of one file and edit different lines;
             then T017, T018, T019, and T020 in parallel, since each is
             a new scenario; then T021 and T022 in sequence, since both
             edit the encoding path.

US5 + US6:   T023 and T025 in parallel, since both add fixtures to
             counters_pmu_test.cpp and edit different regions; then
             T024 and T027 in sequence, since both edit provider.cpp.

Phase 10:    T032, T033, T034, T035, T040, T041, and T043 in parallel;
             then T036, T037, T038, T039, and T042 in parallel; then
             T044 alone, because it re-reads the finished list.
```

### Implementation strategy

**MVP scope**: Phase 1, Phase 2, and US1, which is
`specs/013-counters-defect-followup/quickstart.md` steps 1 through 2.
It delivers a fold that reports either a correct count or a disclosed
gap, with a test that fails at the pre-fix head. Every later story
extends the same mechanism and depends on Phase 2, so the MVP is also
the foundation the rest builds on.

**Incremental delivery**: US1 through US4 are the four P1 stories and
carry the wrong-value defects. US5 and US6 are the two P2 stories and
carry the event-usability defects. US7 is the P3 story and carries the
package and gate defects. Each phase leaves `ctest --preset=dev` green,
so a stopping point between any two phases is a working tree.

**Ordering rationale**: T005 and T006 are the two rewritten tests and
they precede every correction, because FR-023 requires the red state to
be observed at the pre-fix head and a rewritten test is the only place
that state is observable. T007 and T008 land before any story because
US1, US2, and US7 read the mechanism they add.

## Phase 11: Convergence

- [X] T045 Format every file named by
  `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake` so that
  command exits 0 per Constitution V, SC-013 (contradicts)
- [X] T046 Clear the findings from
  `python3 tools/prose/prose_gate.py --check prose --mode tree` so the
  command reports zero findings per T037, FR-034, SC-013 (partial)
- [X] T047 Correct the British spelling in
  `.agents/skills/speckit/speckit-taskstoissues/SKILL.md` so
  `cmake -P cmake/spell.cmake` exits 0 per SC-013 (partial)

## Phase 12: Convergence

- [X] T048 Map `AnyThread`, `PortMask`, and `FCMask` through
  `kernel_spelling` in `source/counters/linux_pmu/table_parse.cpp`
  to `any`, `ch_mask`, and `fc_mask`, add the fixture rows
  `contracts/register-filter.md` names, and re-pin the directory
  counts those rows move per FR-011 (partial)
- [X] T049 Mark a device that publishes no per-task context
  device-scoped in `source/counters/linux_pmu/provider.cpp`, so its
  entries publish `availability::scope_refused` from that published
  data. A probe errno does not decide the scope, per FR-016 (partial)
- [X] T050 Record both `cd5cbd1` removals and the 0.4.0 bump in the
  `Version lineage` section of
  `specs/012-counters-defect-resolution/plan.md` and in the version
  note on `include/speedgun-ng/counters_measurement.hpp` per FR-032
  (partial)

## Phase 13: Convergence

- [X] T051 CRITICAL Rewrite the comment at
  `test/source/counters_registration_order_test.cpp:163` so it does not
  use `rather than`, and re-run
  `python3 tools/prose/prose_gate.py --check prose --mode tree` until
  it reports zero findings per Constitution VIII, FR-034, SC-013
  (contradicts)
- [X] T052 Read the cycle counter, the scale, offset, and shift fields,
  the page index, and the short-counter fields inside the sequence
  snapshot in `fast_context_time_pair` at
  `source/counters/linux_pmu/fast_read.cpp`, before the stability
  check, per FR-007, US3/AC1 (partial)
- [X] T053 Publish a row whose register value is non-zero and whose
  register index names no format as `not_encodable` in
  `source/counters/linux_pmu/table_parse.cpp`, so the row does not
  encode as countable without its filter, per FR-010 (partial)
- [X] T054 Route a table unit that already carries an instance suffix
  to that one device alone in `scope_reaches` at
  `source/counters/linux_pmu/provider.cpp`, including the pinned units
  `cbox_0`, `imc_free_running_0`, and `imc_free_running_1`, per FR-014
  (partial)
- [X] T055 Retire the sentence in the `Version lineage` section of
  `specs/012-counters-defect-resolution/plan.md` that says the release
  ships as 0.3.0 and `SOVERSION` stays at 0, so that section records
  0.4.0 as the release, per FR-032, SC-011 (contradicts)
- [X] T056 Add a requirement-correction entry to
  `specs/007-counters-and-timers/citations-log.md` for each of I-01,
  I-02, I-03, I-04(a), I-04(d), I-05, and I-07, naming the 007 or 012
  requirement the correction restores, per FR-030 (partial)

## Phase 14: Convergence

- [X] T057 Publish a non-zero register value whose index is blank,
  unparseable, or not a string as `not_encodable` in
  `source/counters/linux_pmu/table_parse.cpp`, and stop
  `test/source/counters_linux_pmu_seam_test.cpp` from asserting that
  those rows encode without a filter, per FR-010 (contradicts)
- [X] T058 Map `RdWrMask`, `EnAllCores`, `EnAllSlices`, `SliceId`, and
  `ThreadMask` through `kernel_spelling` in
  `source/counters/linux_pmu/table_parse.cpp` to `rdwrmask`,
  `enallcores`, `enallslices`, `sliceid`, and `threadmask`, per FR-011
  (partial)
- [X] T059 Take each device's fast verdict from that device's own event
  page `cap_user_rdpmc` bit in
  `source/counters/linux_pmu/provider.cpp`, stop gating it on the
  host-wide instructions probe or on the existence of a sysfs `rdpmc`
  file, and correct the I-04(a) citation that claims this read already
  happens, per FR-017 (partial)

## Phase 15: Convergence

- [X] T060 Stop gating `pmu_open_window` on the host-wide instructions
  probe in `source/counters/linux_pmu/group_io.cpp`, so a device whose
  own event page granted the fast read is not downgraded to the group
  read by a probe of another event, per FR-017 (partial)
- [X] T061 Stop appending the host-wide fast-probe sentence to every
  device description in `source/counters/linux_pmu/provider.cpp`, and
  name that device's own page verdict instead, per FR-017 (partial)
