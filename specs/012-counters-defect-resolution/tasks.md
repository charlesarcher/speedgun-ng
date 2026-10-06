# Tasks: Counters Defect Resolution

**Input**: Design documents from
`/specs/012-counters-defect-resolution/`

**Prerequisites**: plan.md (required), spec.md (required for user
stories), research.md, data-model.md, contracts/, quickstart.md

**Tests**: Every task that corrects a defect ships a test. FR-032
requires each covering test to fail at `6aafd2d` and pass after the
correction, and plan.md records TDD mode as in force (FR-033,
Principle III). Test tasks come first inside every story phase, and
each names the command that observes the red state. Every red state is
observed at the current head with the correction unapplied. No task
checks out the pre-fix head to observe red, because that commit holds
no copy of the test being observed.

**Organization**: Tasks are grouped by user story to enable independent
implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (US1 through US7)
- Include exact file paths in descriptions

## Path Conventions

Single project. Paths are repository-root relative.

- Library headers: `include/speedgun-ng/`
- Library sources: `source/counters/`
- Tests: `test/source/`, registered in `test/CMakeLists.txt`
- Build: `CMakeLists.txt`, `CMakePresets.json`, `cmake/`
- CI: `.github/workflows/ci.yml`

### Reading order for every task

1. `specs/012-counters-defect-resolution/spec.md` for the requirement
2. `specs/012-counters-defect-resolution/research.md` for the decision
   behind it, cited by `D-NN`
3. `specs/012-counters-defect-resolution/contracts/` for the doxygen
   clause and its paired enforcement site
4. `specs/012-counters-defect-resolution/quickstart.md` for the
   verification command

### Rules that bind every task

- One commit per task, or per coherent group the task names. The commit
  carries `<Section>: <imperative>` at 50 characters or fewer, a
  why-body at 72 columns, `Approved-by:`, and
  `Refs: specs/012-counters-defect-resolution`.
- Inside a test-then-code pair, the failing test lands first and the
  correction second.
- Every changed line traces to a requirement (X.3, FR-042).
- This feature adds no coverage-exclusion marker, removes the
  calibration region and the release-arm markers, and leaves every
  marker over a kernel-facing wrapper in place, because no single host
  reaches the coverage gate alone (FR-027, FR-046).
- Verify after every task with
  `cmake --build --preset=dev && ctest --preset=dev --output-on-failure`.
- A task closed only against the dev preset stays open. Each feature
  builds once in the release preset before its tasks are called done
  (Principle IX).

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Capture every pre-fix figure the specification names, so
each later task has a baseline to compare against and no baseline is
written after the change it gates.

- [X] T001 Build the release preset in a separate worktree at the
  pre-fix head `6aafd2d`, run `./build/test/counters_overhead` there,
  and record the core PMU group median and the clock-leaf median into
  the step 8 table in
  `specs/012-counters-defect-resolution/quickstart.md`. Take each
  median as the median over 64 repeats of 1000 actions on a pinned
  processor, and record that run method beside both figures, because
  T019 measures under the same method and the two cannot be compared
  otherwise (FR-008)
- [X] T002 [P] Count the `LCOV_EXCL` markers under `source/counters/`
  and record the figure in
  `specs/012-counters-defect-resolution/quickstart.md` (FR-027)
- [X] T003 [P] Record the `clang-tidy` warning count for each of
  `source/counters/plan.cpp`, `source/counters/system.cpp`,
  `source/counters/fold.cpp`,
  `source/counters/linux_pmu/provider.cpp`,
  `source/counters/linux_pmu/table_parse.cpp`,
  `source/counters/linux_pmu/fast_read.cpp`, and
  `source/counters/linux_pmu/group_io.cpp` into
  `specs/012-counters-defect-resolution/quickstart.md` (FR-041)
- [X] T004 [P] Record `size` for the release archive at
  `build/libspeedgun-ng.a` and for the executable
  `test/consumer/main.cpp` builds, into the step 10 table in
  `specs/012-counters-defect-resolution/quickstart.md`. Run this after
  T001, so the release tree exists. The measurement needs the release
  configuration from `cmake --preset=ci-ubuntu`, because the dev preset
  writes into `build/dev` (FR-023, SC-012)
- [X] T005 [P] Count the encodable rows the pinned tree yields for
  `amdzen4` and `amdzen5` at the pre-fix head, in the same separate
  worktree T001 builds, measured against the same named synthetic
  sysfs format list the fixture supplies, and write both figures into
  the measured encodable-row counts table in
  `specs/012-counters-defect-resolution/data-model.md`, so one
  baseline figure gates every host (FR-020, SC-005)

**Checkpoint**: Every baseline figure the specification names exists in
writing. A later task that compares against a baseline has one. T001
runs first, because it builds a separate worktree. T002 through T004
read the working tree, and T005 measures in that same worktree at the
pre-fix head.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Add the shared surface more than one story builds on.
These three tasks are blocking for the stories named below them, and no
other story depends on them.

Blocked: US1 and US4 need the availability surface from T006. US1, US3,
and US4 need the synthetic fixtures from T007. US1, US3, and US4 need
the extracted decisions from T072.

- [X] T006 [P] Add the `scope_refused` and `gap` enumerators to
  `enum class availability`, add the fixed-size target-kind bitmask
  typedef, and add the `targets` member to `catalog_entry` in
  `include/speedgun-ng/counters_core.hpp`, each with the doxygen clause
  and the paired `SG_ENSURE` from
  `specs/012-counters-defect-resolution/contracts/availability.md`.
  The `gap` clause states that the value names one action and that the
  catalog never publishes it for an entry (FR-021, FR-024, FR-007)
- [X] T007 [P] Add the synthetic event-page builder and the
  `/proc/self/fd` and `/proc/self/maps` counting helpers to
  `test/source/counters_linux_pmu_seam_test.cpp`, following the
  existing `check()` and `fail()` convention in that file (FR-034)
- [X] T072 Add the four corrected decisions as small pure functions
  declared in `source/counters/detail/pmu.hpp`, each with a doxygen
  clause and its paired enforcement site, so the coverage gates
  measure each decision directly and no coverage-exclusion marker is
  added for any of them (FR-046). The four decisions are whether a
  group read returned fewer bytes than the group header, whether the
  leader's page disclosed a stable enabled and running pair, which
  read mode and time pair one catalog entry publishes, and whether a
  granted mapping and descriptor are released. The fourth needs no
  extraction, because the release forwards the address, the length,
  and the descriptor and reads nothing else. Define each pure function
  beside the code that calls it, and leave the kernel-facing wrapper
  around each call under its existing marker.

**Checkpoint**: The availability surface, the shared fixtures, and the
extracted decisions exist. US1, US3, and US4 can now begin.

---

## Phase 3: User Story 1 - Receive a fast-path sample that is a count
or a disclosed gap (Priority: P1) MVP

**Goal**: Every fast-path sampling action hands the caller a correct
cumulative count, or discloses through one managed column that the
value is unavailable. I-04 and I-05.

**Independent Test**: Drive the fast read over synthetic event pages and
synthetic groups: a page whose capability is absent, a page whose read
is refused, a page whose sequence moves, a counter near its width
boundary, a multiplexed window, and a group whose read returns fewer
bytes than its header. Compare every result with the recipe the kernel's
own interface header documents. The fixture runs unprivileged. Verify
with the two commands under the tests below.

### Tests for User Story 1

> **NOTE**: Write these tests FIRST and observe them FAIL at
> `6aafd2d` before the correction lands.

- [X] T008 [P] [US1] Add the decode scenarios to
  `test/source/counters_linux_pmu_seam_test.cpp`: a page whose
  capability bit is clear, a page whose read is refused, a page whose
  sequence moves, a count crossing the published width, and a
  multiplexed window checked against the kernel's documented enabled
  and running time recipe to within one nanosecond (FR-002, FR-003,
  FR-004, FR-005, SC-001). Observe red at the current head with the
  correction unapplied, using
  `ctest --test-dir build/dev -R counters_linux_pmu_seam_test`.
  Checking out the pre-fix head is wrong here, because that commit
  holds no copy of this test.
- [X] T009 [P] [US1] Add the short-group-read scenario to
  `test/source/counters_linux_pmu_seam_test.cpp`: a leader answering
  with fewer bytes than the header marks the action, and no fold across
  it reports a delta above the counts the fixture drove (FR-006, SC-002)
- [X] T010 [P] [US1] Add the disclosure-column read to
  `test/source/counters_recorder_test.cpp`: a recorded row carries the
  countability value beside the count and the ratio pair, a failed
  action carries `availability::gap` beside a zero count, and a
  measured zero carries the entry's own countability value beside it
  (FR-007). Observe red with
  `ctest --test-dir build/dev -R counters_recorder_test`
- [X] T011 [P] [US1] Add the allocation and lock-freedom assertions to
  `test/source/counters_recorder_test.cpp`: one sampling action is
  `noexcept`, allocates nothing, and takes no lock, checked with no
  recorded constant so the assertion decides the same on any host
  (FR-008). The 5 percent median bound stays a recorded measurement in
  the step 8 table of
  `specs/012-counters-defect-resolution/quickstart.md`, because a
  timing constant measured on the reference host cannot decide a test
  on a CI runner
- [X] T070 [P] [US1] Add the migrated-thread scenario to
  `test/source/counters_linux_pmu_seam_test.cpp`: a fast-mode
  cpu-target plan whose sampling thread has migrated to another
  processor, checked against the cpu-target pinning precondition in a
  contract-emitting configuration, where the check aborts the read
  (FR-045). Observe red at the current head with the correction
  unapplied, using
  `ctest --test-dir build/dev -R counters_linux_pmu_seam_test`
- [X] T071 [US1] Confirm the `consumer-release` job covers the cpu-target
  pinning precondition: that job configures the `ci-linux-ignore` preset
  and reads `nm -C` over `libspeedgun-ng.a`, asserting that no
  `sg::dbc::check_` symbol survives, so a build configured `ignore`
  emits no check for this precondition the way it emits none for any
  other semantic-gated site. Record the confirmation in
  `specs/012-counters-defect-resolution/quickstart.md` (FR-045). Verify
  with `ctest --test-dir build/dev -R counters_trap_checked`, and with the
  `consumer-release` job's archive symbol check over the `ci-linux-ignore`
  build tree

### Implementation for User Story 1

- [X] T013 [US1] Replace the width mask in `fast_decode` at
  `source/counters/linux_pmu/fast_read.cpp` with the sign extension and
  offset addition the kernel's own interface header documents: the
  `pmc_width` sign-extension pair, and the enabled and running time
  computation beside it. The requirement names each recipe by content,
  so no line number in any kernel header is part of it (D-01, FR-004)
- [X] T014 [US1] Inspect all three verdicts in `read_points` at
  `source/counters/linux_pmu/group_io.cpp`: a read that does not
  return `ok` writes a zero count, keeps no earlier value, and marks
  the action; the retry runs once and its failure takes the same arm
  (D-03, FR-002, FR-003)
- [X] T015 [US1] Resolve and write the disclosure column in
  `source/counters/plan.cpp` beside `link_ratio_slots`, and write it
  from every read path in `source/counters/linux_pmu/group_io.cpp`. A
  measured action writes the countability value the catalog publishes
  for the entry, and an action that measured nothing writes
  `availability::gap` (FR-007)
- [X] T016 [US1] Mark the short and the refused group read at
  `source/counters/linux_pmu/group_io.cpp:238-248` and publish no count
  the read never produced (FR-006)
- [X] T017 [P] [US1] Mark a failed enabled and running pair read at
  `source/counters/linux_pmu/group_io.cpp:361-367`, and make
  `leaf_ratio` at `source/counters/fold.cpp:126-130` report the
  measured ratio only for an action the disclosure marks as measured
  (D-02, FR-005)
- [X] T018 [P] [US1] Add the cpu-target pinning precondition as a
  semantic-gated `SG_REQUIRE` on the fast read path in
  `source/counters/linux_pmu/fast_read.cpp`, with the doxygen clause
  from `specs/012-counters-defect-resolution/contracts/fast-read-fold.md`,
  and add no branch to `recorder::sample()` (FR-045)
- [X] T019 [US1] Build the release preset, run
  `./build/test/counters_overhead`, and record both post-fix medians
  beside the T001 figures in
  `specs/012-counters-defect-resolution/quickstart.md` (FR-008, SC-011)
- [X] T073 [US1] Replace the width mask and the verdict handling so
  each corrected decision calls the extracted function T072 declares:
  the width mask in `fast_decode` at
  `source/counters/linux_pmu/fast_read.cpp`, the short and refused
  group read at `source/counters/linux_pmu/group_io.cpp:238-248`, and
  the failed pair read at
  `source/counters/linux_pmu/group_io.cpp:361-367` (FR-004, FR-046)
- [X] T074 [US1] Wire the group-read and pair-read decisions in
  `source/counters/linux_pmu/group_io.cpp` to the functions T072
  declares, and retire the coverage-exclusion markers on the release
  arms at `source/counters/linux_pmu/fast_read.cpp:353-372`, because
  T025 reaches them through the destructor with a descriptor and a
  mapping the test opened itself. Every marker over a kernel-facing
  wrapper stays in place, because commit `f963fda` records that the CI
  runner refuses every `perf_event_open` while a developer host grants
  it, so no single host reaches the coverage gate alone
  (FR-046, FR-027)

**Checkpoint**: US1 is fully functional and testable independently. The
disclosure column exists, and the countability value T006 added is what
a caller reads from it.

---

## Phase 4: User Story 2 - Resolve counters and compile plans from
many threads at once (Priority: P1)

**Goal**: Catalog reads and plan compiles are safe from any number of
threads once the catalog is open, and a thread-sanitizer run proves it.
I-06.

**Independent Test**: Open the catalog, then run several threads that
resolve counters and compile plans concurrently under a thread
sanitizer. The run reports no race.

The thread sanitizer needs a build tree before any test can run there,
so its preset lands first. Every later task in this phase assumes
`build/tsan` resolves.

- [X] T020 [US2] Add the `ci-tsan` configure, build, and test presets
  to `CMakePresets.json`: a configure preset inheriting `ci-linux` and
  `dev-mode` with `binaryDir` at `build/tsan` and
  `-fsanitize=thread` on the project and the vendored trees, plus the
  build and test presets that make `cmake --build build/tsan` and
  `ctest --test-dir build/tsan` resolve. Leave `ci-sanitize`
  configuring address and undefined behavior alone, since the
  compilers reject the pairing (D-05, FR-012, SC-003)

### Tests for User Story 2

> **NOTE**: Write these tests FIRST and observe them FAIL at
> `6aafd2d` before the correction lands. T020 precedes this task, so
> the red observation below runs.

- [X] T021 [US2] Add the concurrent resolution scenarios to
  `test/source/counters_recorder_test.cpp`: several threads resolving
  the same and different canonical addresses at once, several threads
  compiling plans at once, and one thread walking a parent and its
  children while another resolves an address (FR-010, FR-011). Observe
  red with `ctest --test-dir build/tsan -R counters_recorder_test`. The
  build tree at `build/tsan` comes from T020, so T020 precedes this
  task.

### Implementation for User Story 2

- [X] T022 [US2] Guard `system::handle_for` and the handle map at
  `source/counters/system.cpp:418-426`, and guard the plan-compile path
  that writes shared state (D-06, FR-010, FR-011)
- [X] T023 [US2] Move the open flag out of the per-call compile path in
  `source/counters/plan.cpp` and set it once at the open boundary
  (FR-011)
- [X] T024 [US2] Add the `tsan` job to
  `.github/workflows/ci.yml`, configuring `ci-tsan`, building
  `build/tsan`, and running `ctest` there (FR-012, SC-003)

**Checkpoint**: US1 and US2 both work independently. The thread
sanitizer reports no race.

---

## Phase 5: User Story 3 - Compile and destroy plans without
exhausting host resources (Priority: P1)

**Goal**: Destroying a plan releases every event descriptor and every
mapping it acquired, including on a partial open. I-08.

**Independent Test**: Open and destroy a fast-mode plan 10,000 times,
then compare the process descriptor count and mapping count with their
values before the loop. A partial open, where a later member fails, runs
the same comparison.

### Tests for User Story 3

> **NOTE**: Write these tests FIRST and observe them FAIL at
> `6aafd2d` before the correction lands.

- [X] T025 [P] [US3] Add the everywhere-runnable lifecycle test to
  `test/source/counters_linux_pmu_seam_test.cpp`: the test opens its
  own descriptor on `/dev/null` and its own anonymous read-only
  mapping, places both in a `fast_context` value with the mapping
  length beside them, and lets the value leave scope, 10,000 times
  plus one partial open, comparing `/proc/self/fd` and
  `/proc/self/maps` with their values before the loop (FR-013,
  FR-014). The destructor releases them, so the test measures a real
  release and not a counter. Observe red at the current head with the
  correction unapplied, using
  `ctest --test-dir build/dev -R counters_linux_pmu_seam_test`
- [X] T026 [P] [US3] Add the host-dependent lifecycle test to
  `test/source/counters_pmu_test.cpp`: open and destroy a real
  fast-mode plan 10,000 times, compare the same two counts, print the
  reason and exit 2 where the kernel refuses the event, and register
  `SKIP_RETURN_CODE 2` for it (FR-013, SC-004)
- [X] T027 [US3] Add the destroy-before-open scenario to
  `test/source/counters_linux_pmu_seam_test.cpp`: nothing is released
  and no check fails (FR-015, SC-004)

### Implementation for User Story 3

- [X] T028 [P] [US3] Add a destructor to `fast_context` at
  `source/counters/detail/pmu.hpp:279-285` that calls
  `fast_context_close` (D-07, FR-013)
- [X] T029 [US3] Extract one internal release function in
  `source/counters/linux_pmu/group_io.cpp`, call it from
  `~pmu_fast_window` at `:327` and from the two partial-open arms at
  `:552` and `:564`. T025 reaches the same release through the
  destructor, so no declaration order binds the two tasks (D-15,
  FR-013, FR-014)

**Checkpoint**: US1, US2, and US3 all work independently. A
fast-mode plan compiles and destroys in a loop without exhausting host
resources.

---

## Phase 6: User Story 4 - Count events on hybrid, uncore, and Intel
hosts (Priority: P2)

**Goal**: Every row of the catalog encodes on an Intel host, every row
lands on the device its scope names, and availability separates a scope
refusal from an encoding refusal. I-01, I-02, I-03.

**Independent Test**: Over the pinned table tree, publish the
encodable-row count for each Intel core architecture directory and
assert the counts for four named directories. Drive synthetic hybrid
and uncore device fixtures and assert each row lands on the device its
scope names. Drive a device-scoped entry and assert the catalog
separates a scope refusal from an encoding refusal.

Depends on: T006 for the availability surface and T072 for the
extracted decisions.

### Tests for User Story 4

> **NOTE**: Write these tests FIRST and observe them FAIL at
> `6aafd2d` before the correction lands.

- [X] T031 [P] [US4] Add the Intel encodable-row count fixture to
  `test/source/counters_linux_pmu_seam_test.cpp`: a named synthetic
  sysfs format list the fixture supplies, the count pinned for
  `skylake`, `icelake`, `alderlake`, and `sapphirerapids`, and the
  `amdzen4` and `amdzen5` counts asserted not to fall below the T005
  figures. Assert that the ten keys carrying no encoding obligation,
  `SampleAfterValue`, `MSRValue`, `MSRIndex`, `CounterMask`, `Invert`,
  `EdgeDetect`, `PEBS`, `Data_LA`, `PerPkg`, and `Experimental`, never
  become encoding fields (FR-016, FR-017, FR-020)
- [X] T032 [P] [US4] Add the device-placement fixture to
  `test/source/counters_linux_pmu_seam_test.cpp`: a hybrid host
  publishing core and atom devices, an uncore device, and a row scoped
  to an absent device, asserting each row lands on the device its scope
  names and no uncore row appears under the core device (FR-019, SC-006)
- [X] T033 [P] [US4] Add the availability scenarios to
  `test/source/counters_pmu_test.cpp`: a device-scoped entry publishes
  `scope_refused` for a per-task target and publishes a state separable
  from an encoding refusal, a cpu-target plan over it compiles where
  the kernel grants the event, and the supported target kinds arrive as
  a fixed-size bitmask that allocates no memory. Add a static
  assertion over the values of the availability enumeration, so adding
  a target kind to the target-kind enumeration leaves every existing
  value unchanged (FR-001, FR-021, FR-022, SC-007). Observe red with
  `ctest --test-dir build/dev -R counters_pmu_test`
- [X] T069 [P] [US4] Add the encoding-refusal scenarios to
  `test/source/counters_linux_pmu_seam_test.cpp`: a row needing a field
  the running kernel's formats do not publish reports `not_encodable`,
  and a row whose fields the kernel does publish never reports it
  (D-08, FR-018). Observe red at the current head with the correction
  unapplied, using
  `ctest --test-dir build/dev -R counters_linux_pmu_seam_test`

### Implementation for User Story 4

- [X] T035 [US4] Record a numeric table key as an encoding field only
  where the key names a format the running device publishes, at
  `source/counters/linux_pmu/table_parse.cpp:176-180`, and map the
  kernel spellings `cmask`, `inv`, `edge`, and `offcore_rsp` onto
  `CounterMask`, `Invert`, `EdgeDetect`, and `OffcoreRsp` (D-08,
  FR-016, FR-017)
- [X] T036 [US4] Publish `not_encodable` only where the running
  kernel's formats lack a field the row needs, at
  `source/counters/linux_pmu/provider.cpp:296` (FR-018)
- [X] T037 [US4] Route each vendored row to the device its table scope
  names at `source/counters/linux_pmu/provider.cpp:266`, keeping a row
  scoped to an absent device out of the catalog (D-09, FR-019)
- [X] T038 [US4] Run the availability probe per target kind the T006
  bitmask admits at `source/counters/linux_pmu/provider.cpp:102-111`,
  and publish the fast read mode on an entry only where the fast read
  can succeed for that entry (D-10, FR-001, FR-022)
- [X] T039 [US4] Bump `VERSION` to `0.3.0` at `CMakeLists.txt:7`,
  leaving `SOVERSION` at `PROJECT_VERSION_MAJOR` (FR-024)
- [X] T040 [US4] Measure each of the four Intel counts over the pinned
  tree against the synthetic format list, measure the counts the
  reference host's own formats yield, and write all six figures into
  the measured encodable-row counts table in
  `specs/012-counters-defect-resolution/data-model.md` (FR-020, SC-005)
- [X] T075 [US4] Wire the per-entry probe decision at
  `source/counters/linux_pmu/provider.cpp:102-111` to the function T072
  declares. No marker over the kernel-facing wrapper is removed, for
  the reason T074 records (FR-046, FR-027)

**Checkpoint**: US1 through US4 all work independently. The catalog
encodes every Intel row whose fields map to published kernel formats.

---

## Phase 7: User Story 5 - Install the package and find the catalog the
build tree holds (Priority: P2)

**Goal**: An installed package publishes the same vendored catalog the
build tree publishes, with no run-time path lookup. I-07.

**Independent Test**: Install the package into a scratch prefix, run
the downstream consumer against it, and compare the published
vendored-row count with the build tree's count on the same host.

Runs after US4: both tasks edit
`source/counters/linux_pmu/table_parse.cpp`.

### Tests for User Story 5

> **NOTE**: Write this test FIRST and observe it FAIL at `6aafd2d`
> before the correction lands.

- [X] T041 [P] [US5] Add the vendored-row count to
  `test/consumer/main.cpp`, printing the count the linked package
  resolves, so the downstream consumer job compares it against the
  build tree's count (FR-023, SC-008). Observe red by installing the
  pre-fix package and running `./build-consumer/consumer`

### Implementation for User Story 5

- [X] T042 [US5] Add `cmake/EmbedPmuEvents.cmake`, which compiles the
  vendored JSON under `external/pmu-events` into static data and
  exposes it to `source/counters/linux_pmu/table_parse.cpp`
  (D-11, FR-023)
- [X] T043 [US5] Decode the embedded bytes through the existing
  simdjson path in `source/counters/linux_pmu/table_parse.cpp`,
  replacing the file reads, and remove every `SG_PMU_EVENTS_DIR`
  reference at `:391`, `:403`, and `:430` (FR-023)
- [X] T044 [US5] Add no build option around the embedding in
  `cmake/EmbedPmuEvents.cmake`, and confirm
  `python3 tools/pmu_events/update_pmu_events.py --check` still passes
  with the pin unchanged (FR-023)
- [X] T045 [US5] Install into a scratch prefix, run
  `./build-consumer/consumer`, relocate the prefix, run it again, and
  record the archive and executable sizes beside the T004 figures in
  `specs/012-counters-defect-resolution/quickstart.md` (FR-023,
  SC-008, SC-012)

**Checkpoint**: US1 through US5 all work independently. An installed
package publishes the build tree's catalog.

---

## Phase 8: User Story 6 - Read a per-plan overhead floor that
excludes the clock that measures it (Priority: P2)

**Goal**: The published floor isolates the sampling action from the
clock reads that bracket it, and a registered test exercises the
calibration. I-09.

**Independent Test**: Run a calibration under a test registered with the
test runner and compare the published floor with the cost of the
bracketing clock reads measured under the identical bracketing.

Runs after US2: both tasks edit `source/counters/plan.cpp`.

### Tests for User Story 6

> **NOTE**: Write this test FIRST and observe it FAIL at `6aafd2d`
> before the correction lands.

- [X] T046 [P] [US6] Add the calibration test to
  `test/source/counters_overhead.cpp`: it drives the plan's own
  calibration, measures the bracketing pair under the identical
  bracketing, and fails when the published floor still carries the
  bracket (FR-025, FR-026, SC-009). Observe red with
  `ctest --test-dir build/dev -R counters_overhead`

### Implementation for User Story 6

- [X] T047 [US6] Measure the bracketing pair in `calibrate` at
  `source/counters/plan.cpp:267-296` and subtract it from the
  sampling-action cost (D-12, FR-025)
- [X] T048 [US6] Remove the `LCOV_EXCL_START` and `LCOV_EXCL_STOP`
  region wrapping `source/counters/plan.cpp:256`, so no coverage
  exclusion hides the calibration from the registered test, and record
  the new marker count beside the T002 figure. Count the release-arm
  markers T074 retires in the same figure (FR-027), so the number
  beside the T002 baseline accounts for both removals (FR-026, FR-027)

**Checkpoint**: US1 through US6 all work independently. The published
floor excludes the bracket.

---

## Phase 9: User Story 7 - Read a clock leaf whose documented order the
platform keeps (Priority: P2)

**Goal**: Each clock leaf documents its own order guarantee, and a test
exercises each stated guarantee. I-10.

**Independent Test**: Sample `machine/thread_cpu` on a new thread and
compare the value with a sample taken earlier on another thread. Read
each leaf's documented guarantee and match a test to each one.

### Tests for User Story 7

> **NOTE**: Write these tests FIRST and observe them FAIL at `6aafd2d`
> before the correction lands.

- [X] T049 [P] [US7] Add the per-leaf order tests to
  `test/source/counters_clock_raw_test.cpp`: a new thread's
  `machine/thread_cpu` sample falls below an earlier sample taken on
  another thread, a second sample on one thread does not fall below the
  first, `machine/process_cpu` does not fall across threads of one
  process, a `machine/monotonic` sample does not fall below an earlier
  sample on the same thread, and `machine/monotonic_raw` does not
  either (FR-029, FR-030, SC-010). Observe red with
  `ctest --test-dir build/dev -R counters_clock_raw_test`
- [X] T050 [P] [US7] Extend `test/counters_tsc_read_shape.sh` to assert
  the timestamp-counter leaf reads with no ordering fence (FR-031)

### Implementation for User Story 7

- [X] T051 [US7] Add a per-leaf doxygen clause to each leaf in
  `include/speedgun-ng/counters_clock.hpp`, on the terms in
  `specs/012-counters-defect-resolution/contracts/clock-order.md`, and
  reduce the class clause to state no order on any leaf's behalf
  (D-13, FR-028)
- [X] T052 [US7] State the per-thread CPU clock's guarantee in
  `include/speedgun-ng/counters_clock.hpp` so it permits a new thread's
  sample to fall below an earlier sample taken on another thread, and
  state the timestamp-counter leaf's guarantee there as a precondition
  on the caller, adding no fence (FR-030, FR-031)

**Checkpoint**: All seven stories work independently.

---

## Phase 10: Polish and Cross-Cutting Concerns

**Purpose**: The obligations that span every story. None of them adds
behavior; each one is a gate, a record, or a check.

- [ ] T053 [P] Record one entry per correction in
  `specs/007-counters-and-timers/citations-log.md`, ten entries in the
  file's recorded format, each naming the 007, 008, or 011 requirement
  it restores, its measuring command, and its head. Leave the frozen
  record at `specs/007-counters-and-timers/citations.md` unedited
  (FR-038)
- [X] T054 [P] Confirm the Intel confirmation deferral is recorded in
  `specs/012-counters-defect-resolution/plan.md` and
  `specs/012-counters-defect-resolution/research.md`, naming the host
  class that would settle it (FR-039)
- [X] T055 Run `cmake --build build/dev -t dbc-gate` and clear every
  pairing finding on `include/speedgun-ng/counters_core.hpp`,
  `include/speedgun-ng/counters_provider.hpp`, and
  `include/speedgun-ng/counters_clock.hpp` (FR-037, Principle II)
- [ ] T056 Run the coverage gate with
  `cmake --preset=coverage-linux && cmake --build build/coverage -j 2 && ctest --test-dir build/coverage && cmake --build build/coverage -t coverage`,
  and confirm 100 percent line, branch, and contract coverage on every
  changed line, `source/counters/linux_pmu/group_io.cpp` among them
  (FR-040)
- [ ] T057 Confirm the clang-tidy warning count of each translation
  unit named in T003, `source/counters/linux_pmu/fast_read.cpp`
  among them, did not rise, and that the pre-existing backlog outside
  those units' touched lines is untouched (FR-041, FR-042)
- [X] T058 Run `ctest --test-dir build/dev -R counters_header_purity`
  over `test/counters_header_purity.sh` and confirm no file in
  `include/speedgun-ng/` gained a platform term (FR-036)
- [X] T059 [P] Confirm `source/counters/plan.cpp` and
  `include/speedgun-ng/counters_core.hpp` stay embeddable in
  fixed-iteration, per-thread benchmark loops, and that no
  benchmarking-framework code entered `source/counters/` (FR-035)
- [X] T060 Build the release preset with
  `cmake --preset=ci-ubuntu && cmake --build build`, which no
  unoptimized build of `source/counters/plan.cpp` can substitute for
  (Principle IX)
- [ ] T061 Run the address and undefined-behavior sanitizers with
  `cmake --preset=ci-sanitize && cmake --build build/sanitize && ctest --test-dir build/sanitize`
  over every binary registered in `test/CMakeLists.txt`
  (Principle VIII)
- [X] T062 [P] Run `cmake -D FORMAT_COMMAND=clang-format-18 -P cmake/lint.cmake`
  and `cmake --build build/dev -t format-check` (Principle V)
- [X] T063 [P] Run `cmake --build build/dev -t spell-check` and
  `cmake -P cmake/prose-lint.cmake` over every Markdown file this
  feature adds, then review each XI.7 rule the gate cannot match by
  hand: sentence length, one topic per sentence, active voice, and one
  term per concept (FR-044, Principle XI)
- [X] T064 [P] Run
  `python3 tools/pmu_events/update_pmu_events.py --check` over
  `external/pmu-events/RECORD` and confirm no drifted file
  (Principle VIII)
- [ ] T065 Walk every step of
  `specs/012-counters-defect-resolution/quickstart.md` end to end on
  the reference host, fill the two tables step 8 and step 10 name, and
  record the result (SC-001 through SC-012)
- [X] T066 [P] Confirm no recorder entry point was added, changed, or
  removed: `grep` `include/speedgun-ng/counters_measurement.hpp` and
  `include/speedgun-ng/counters_core.hpp` for a reuse, reset, or arena
  member that `6aafd2d` does not hold, and confirm the shipped
  behaviour stands, one arena per `plan::recorder()` call and no reset
  (FR-009)
- [X] T067 [P] Confirm no event source, object kind, provider, or
  public header entered the library: compare the file list under
  `include/speedgun-ng/` and the provider list in
  `source/counters/` against `6aafd2d`, and confirm every added name
  traces to a requirement through the verification matrix in
  `specs/012-counters-defect-resolution/plan.md` (FR-043, FR-042)
- [X] T068 [P] Confirm the pre-existing coverage-exclusion backlog
  outside the touched lines is untouched, by counting the markers per
  file under `source/counters/`, `source/counters/plan.cpp` among
  them, and comparing against `6aafd2d` (FR-042, X.3)

---

## Dependencies and Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: no dependencies, starts immediately. Every later
  comparison needs a baseline it writes.
- **Foundational (Phase 2)**: depends on Setup. T006 blocks US1 and
  US4. T007 blocks US1, US3, and US4. T072 blocks US1, US3, and US4.
  US2, US5, US6, and US7 need none of the three and may start at once
  with Phase 2.
- **User Stories (Phases 3 to 9)**: each depends on the Foundational
  tasks it names, and on nothing else. Stories then proceed in priority
  order P1 to P2.
- **Polish (Phase 10)**: depends on all seven stories.

### User Story Dependencies

- **US1 (P1)**: needs T006, T007, and T072. Blocks nothing.
- **US2 (P1)**: needs neither. Touches `source/counters/plan.cpp`, so
  it runs before US6.
- **US3 (P1)**: needs T007 and T072, because the release path the
  lifecycle test drives is the extracted decision. Touches
  `source/counters/linux_pmu/group_io.cpp`, so it runs after US1.
- **US4 (P2)**: needs T006, T007, and T072. Touches
  `source/counters/linux_pmu/table_parse.cpp`, so it runs before US5.
- **US5 (P2)**: needs T004. Runs after US4 for the shared file.
- **US6 (P2)**: runs after US2 for the shared file.
- **US7 (P2)**: independent of every other story.

### Same-file serialization

Ten files carry work from more than one story. One story at a time
holds each.

| File | Stories | Required order |
| --- | --- | --- |
| `source/counters/linux_pmu/group_io.cpp` | US1, US3 | US1 then US3 |
| `source/counters/linux_pmu/table_parse.cpp` | US4, US5 | US4 then US5 |
| `source/counters/plan.cpp` | US1, US2, US6 | US1 then US2 then US6 |
| `source/counters/detail/pmu.hpp` | seam extractions, US3 | seam extractions then US3 |
| `test/CMakeLists.txt` | all seven | one registration task at a time |
| `test/source/counters_linux_pmu_seam_test.cpp` | US1, US3, US4 | one scenario group at a time |
| `test/source/counters_recorder_test.cpp` | US1, US2 | US1 then US2 |
| `test/source/counters_pmu_test.cpp` | US3, US4 | one test at a time |
| `test/source/counters_overhead.cpp` | US1, US6 | US1 then US6 |
| `specs/012-counters-defect-resolution/quickstart.md` | US1, US5, US6 | one writer at a time |
| `specs/012-counters-defect-resolution/data-model.md` | US4 | one writer at a time |

### Within Each User Story

- Tests first, observed failing at `6aafd2d`, then the correction
- Public surface before the code that reads it
- Registration in `test/CMakeLists.txt` after the test source exists
- The correction lands before the figure that measures it

### Parallel Opportunities

- T001 runs alone, and T002, T003, T004, and T005 run together after it
- T006, T007, and T072 are independent of each other
- Inside a story, the test tasks marked `[P]` are independent, and the
  implementation tasks marked `[P]` are independent of the serial ones
- Every story is independent of every other story except where the
  same-file table above names an order
- T053, T054, T059, T062, T063, T064, T066, T067, and T068 in the
  Polish phase are independent of each other

---

## Parallel Example: User Story 1

```bash
# Launch the independent US1 tests together:
Task: "Add the decode scenarios to test/source/counters_linux_pmu_seam_test.cpp"
Task: "Add the disclosure-column read to test/source/counters_recorder_test.cpp"

# Launch the independent US1 corrections together:
Task: "Add the cpu-target pinning precondition in source/counters/linux_pmu/fast_read.cpp"
Task: "Mark a failed pair read in source/counters/fold.cpp"
```

---

## Parallel Example: User Story 4

```bash
# Launch the independent US4 tests together:
Task: "Add the Intel encodable-row count fixture to test/source/counters_linux_pmu_seam_test.cpp"
Task: "Add the availability scenarios to test/source/counters_pmu_test.cpp"
```

---

## Implementation Strategy

### MVP First (User Story 1 only)

1. Complete Phase 1: Setup, which writes every baseline
2. Complete Phase 2: Foundational, T006 and T007
3. Complete Phase 3: User Story 1
4. **STOP and VALIDATE**: run the US1 independent test
5. The library now hands a caller a correct count or a disclosed gap,
   which is the property every later story depends on

### Incremental Delivery

1. Setup plus Foundational, then the three P1 stories in order. US1
   first, then US2, then US3. Every P1 defect is corrected and the
   suite is green before any P2 work starts.
2. Add US4, then US5, then US6, then US7, each gated on the story
   before it where the same-file table names an order.
3. Run the Polish phase, which records the successor-log entries, proves
   every gate, and walks quickstart.md end to end.
4. Each story adds a correct value or a disclosure without breaking the
   stories already landed.

### Parallel Team Strategy

With two developers, after Setup and Foundational:

1. Developer A takes US1, then US3, since both touch
   `source/counters/linux_pmu/group_io.cpp`
2. Developer B takes US2, then US6, since both touch
   `source/counters/plan.cpp`
3. Developer A takes US4 then US5, Developer B takes US7, once the four
   P1 stories are green
4. The Polish phase is one owner's work, since it records one
   successor-log entry per correction

---

## Notes

- [P] marks a task that touches different files and needs no
  unfinished task
- [Story] maps a task to the user story it serves, for traceability back
  to the requirement
- Every story is independently completable and independently testable
- Verify each test fails at `6aafd2d` before implementing
- Commit after each task or each named coherent group
- Stop at any checkpoint to validate a story on its own
- Avoid: a vague task, two stories editing one file at once, and a
  cross-story dependency the same-file table does not name
- FR-033 carries no task on purpose. Its obligation is discharged at
  plan time: `specs/012-counters-defect-resolution/plan.md` records
  TDD mode as in force, and every test task above is ordered before the
  code task it gates
