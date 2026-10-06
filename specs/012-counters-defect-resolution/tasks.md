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

- [X] T053 [P] Record one entry per correction in
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
- [X] T056 Run the coverage gate with
  `cmake --preset=ci-coverage && cmake --build build/coverage -j 2 && ctest --test-dir build/coverage && cmake --build build/coverage -t coverage`,
  and confirm 100 percent line, branch, and contract coverage on every
  changed line, `source/counters/linux_pmu/group_io.cpp` among them
  (FR-040)
- [X] T057 Confirm the clang-tidy warning count of each translation
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
- [X] T061 Run the address and undefined-behavior sanitizers with
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
- [X] T065 Walk every step of
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

---

## Phase 11: Convergence

**Purpose**: The work the specification, the plan, and Phases 1 to 10
still call for, found by reading the tree against the artifacts after
`/speckit.implement` closed every earlier task. One figure was never
measured, six obligations are only partly met, and four records disagree
with the tree they describe. Each task below names the file it changes.
No task above is rewritten, renumbered, or reordered.

- [X] T076 Measure the encodable-row count the reference host's own
  `/sys/bus/event_source/devices/cpu/format/` list yields for each of
  `skylake`, `icelake`, `alderlake`, `sapphirerapids`, `amdzen4`, and
  `amdzen5` over the pinned tree, and add that column beside the
  synthetic-list column in the measured encodable-row counts table in
  `specs/012-counters-defect-resolution/data-model.md`. The list the
  count measures against is already named in that file, and T040 asked
  for six figures where four landed. Verify with
  `ctest --test-dir build/dev -R counters_linux_pmu_seam_test`
  (FR-020, SC-005, T040) (missing)

- [X] T077 Add the doxygen clause and the paired enforcement site to each
  of `group_read_short`, `scope_reaches`, and
  `entry_read_selection_for` at `source/counters/detail/pmu.hpp`, and add
  the missing doxygen clause to `fast_pair_disclosed` beside the
  `SG_ENSURE` it already carries. Each of the four is a corrected decision
  on a path only a granted `perf_event_open` enters, and FR-046 requires
  each to carry both. `build/dev/dbc-gate/doc-matrix.json` holds no row
  for any of them today, so the gate cannot see what they lack. Verify
  with `cmake --build build/dev -t dbc-gate` and with
  `ctest --test-dir build/dev -R counters_linux_pmu_seam_test`
  (FR-046, FR-037, T072) (partial)

- [X] T078 Drive a device-scoped entry through the catalog to publish
  `scope_refused` for a per-task target, and assert that the published
  state is separable from `not_encodable`, in
  `test/source/counters_pmu_test.cpp`. The check at line 316 compares two
  enumerators, which no catalog state can falsify, and the real catalog
  counts `scope_refused` without asserting the count. Verify with
  `ctest --test-dir build/dev -R counters_pmu_test`
  (FR-021, SC-007, T033) (partial)

- [X] T079 Compile a cpu-target plan over an entry whose availability is
  `scope_refused`, and assert the compile succeeds where the kernel grants
  the cpu-targeted event, in `test/source/counters_pmu_test.cpp`. The
  scenario at line 828 selects an entry already marked `countable`, so
  the scope refusal the requirement names is never the entry under test.
  Verify with `ctest --test-dir build/dev -R counters_pmu_test`
  (FR-022, T033) (partial)

- [X] T080 Replace the vacuous assertion at
  `test/source/counters_linux_pmu_seam_test.cpp:581-584`, which compares
  two `constexpr` literals, with a fold driven across an action a short
  group read marked, and assert that no delta exceeds the counts the
  fixture drove. Verify with
  `ctest --test-dir build/dev -R counters_linux_pmu_seam_test`
  (FR-006, SC-002, T009) (partial)

- [X] T081 Write an uncore device fixture directory beside the hybrid
  per-core ones at
  `test/source/counters_linux_pmu_seam_test.cpp:1875-1895`, load it
  through `load_device`, and assert that no core-scoped row reaches it
  while an uncore-scoped row does. Today the uncore placement is asserted
  only through the `scope_reaches` predicate, so the loader's uncore path
  runs in no test. Verify with
  `ctest --test-dir build/dev -R counters_linux_pmu_seam_test`
  (FR-019, SC-006, T032) (partial)

- [X] T082 Recount the coverage-exclusion markers under
  `source/counters/`, correct the per-file breakdown recorded beside the
  T002 baseline in
  `specs/012-counters-defect-resolution/quickstart.md`, and record one
  authoritative post-change figure. The breakdown at line 475 sums to 371,
  the tree holds 374, and line 593 already says 374; `provider.cpp`
  records 119 against 122, `group_io.cpp` 89 against 91, and
  `table_parse.cpp` 17 against 15. The count has not risen over the
  pre-fix 377, so FR-027 holds; the record disagrees with itself. Verify
  with `grep -rn 'LCOV_EXCL' source/counters/ | wc -l`
  (FR-027, FR-042, T048, T068) (partial)

- [X] T083 Correct the archive growth figure in the prose at
  `specs/012-counters-defect-resolution/quickstart.md:311` to the
  +24622203 bytes the step 10 table already carries. The two totals the
  table names subtract to 24622203, and the prose carries a third
  figure. Verify by subtracting the two totals the table records
  (FR-023, SC-012, T045) (partial)

- [X] T084 Delete the sentence at
  `specs/012-counters-defect-resolution/data-model.md:199-200` that says
  the rightmost column stays empty until the fixture supplies the
  numbers. The table directly below it holds those numbers, so a reader
  meets an unmet gate that was met
  (FR-020, T040) (partial)

- [X] T085 Correct the AMD pre-fix figures in the comment at
  `test/source/counters_linux_pmu_seam_test.cpp:822-823`. It names 339
  and 348, which `specs/012-counters-defect-resolution/data-model.md`
  withdraws as figures from the model that dropped `UMASK_EXT`; the
  parser's figures are 321 and 317, and the fixture pins 326 and 322
  (FR-020, T031) (partial)

- [X] T086 Verified that no YAML block in
  `specs/007-counters-and-timers/citations-log.md` repeats a key, so the
  I-09 entry's `claim_as_written` appears once and a YAML reader keeps it.
  The convergence finding that named a duplicate key was wrong. This task
  records the check that closes it, and changes no line of the log
  (FR-038) (unrequested)

**Checkpoint**: The reference host's own-format counts exist, each
extracted decision carries the clause and the check FR-046 names, the
three scenarios prove what their names claim, and every recorded figure
matches the tree it describes.

---

## Phase 12: Convergence

**Purpose**: What the Phase 11 pass could not close, because the gap sits
in the source the earlier phases corrected. Two requirements the
specification states as satisfied are contradicted by the code that
implements them. The tests written to prove them take a skip on every
host. One recorded measurement does not meet the bound it is recorded
against, and one recorded figure still contradicts the tree.

- [X] T087 [P] Publish the scope refusal at `probe_device` in
  `source/counters/linux_pmu/provider.cpp:174-195`. A device-scoped entry
  binds one processor for every task, so its own scope refuses the
  per-task kind, and the else-if arm at line 192 overwrites the refusal
  with the cpu probe's verdict, so no entry ever publishes
  `availability::scope_refused`. The reference host publishes none:
  `pmu availability: 10 countable, 0 permission_blocked, 30 not_encodable,
  0 scope_refused, 8 fast_rdpmc`. Keep the refusal for a device-scoped
  device whose cpu probe does not grant, and publish the cpu probe's
  verdict only where the device is not device scoped (FR-021, US4/AC8)

- [X] T088 Compile a cpu-target plan over an entry the catalog publishes
  as scope-refused, by consulting the requested target in the gate at
  `source/counters/plan.cpp:480`, which today refuses every non-countable
  entry without reading the target kind. The refusal must name the
  window error where the kernel grants the cpu-targeted event and the
  availability error where it does not (FR-022, SC-007)

- [X] T089 [P] Carry the availability probe's per-kind verdicts into
  `catalog_entry::targets` at `source/counters/system.cpp:533`, which
  derives the bitmask from `avail == availability::countable` alone, so
  the mask states the countability twice and names no target kind of its
  own. The probe already runs once per kind and discards the per-kind
  answers. Name the cpu bit for an entry the cpu
  probe settled and no bit for one it refused (FR-021)

- [X] T090 Replace the delta assertion in `test_disclosure_column` at
  `test/source/counters_recorder_test.cpp` that cannot fail. The checks
  above it already pin the two endpoint rows and `fail` exits the binary,
  so `driven_cycles <= 100 && driven_instructions <= 400` is decided
  before it runs. Assert against the fold's own reported value, or drop
  the bound and keep the fold comparison alone (FR-006, SC-002)

- [X] T091 Reconcile the recorded per-sample medians in the step 8 table
  of `specs/012-counters-defect-resolution/quickstart.md` with the bound
  FR-008 states. The pre-fix core PMU group read 60.0 ns on all six runs
  and the post-fix tree read 40.0 ns on four of seven, which is 33 percent
  below the baseline, and the text then argues that no post-fix run rises
  above it. The requirement bounds the figure in both directions, and the
  same passage records that one tick cannot resolve a 5 percent bound
  before declaring the bound met. Either re-measure until both medians
  agree within the bound, or record that the figure moved and state the
  bound the evidence meets (FR-008, SC-011)

- [X] T092 Correct the marker prose that the T082 pass left stale at
  `specs/012-counters-defect-resolution/quickstart.md:488-491`. It names
  `provider.cpp` falling from 122 to 119 and `group_io.cpp` rising from
  82 to 89, and the table the same pass corrected records 122 and 91,
  which is what the tree holds. Count the markers per file again and make
  the prose and the table agree (FR-027, FR-042, T082)

- [X] T093 Place a vendored row on the device its scope names in a
  registered test. `merge_vendored` at
  `source/counters/linux_pmu/provider.cpp:113` is reached only from the
  provider constructor at line 676, inside a coverage-excluded region, so
  no test observes a row landing on a device or staying off one, and the
  uncore fixture `uncore_device_fixture_scenario` loads a device and then
  asserts the pure `scope_reaches` predicate instead. Expose the placement
  step the way `probe_device` and `load_device` are exposed in
  `source/counters/detail/pmu.hpp`, and drive it over a synthetic device
  tree (FR-019, SC-006, T037, T081)

- [X] T094 [P] Correct the citations this feature recorded in
  `specs/007-counters-and-timers/citations-log.md` to that file's own
  addressing convention, which says to cite a target by its section
  heading and never by a line number. Eight citations carry a line number,
  and `provider.cpp:148` names a closing brace where the I-03 entry
  means `pmu_probe`, which the file holds at line 555 (FR-038)

- [X] T095 [P] Correct the marker population the I-09 entry recorded at
  `specs/007-counters-and-timers/citations-log.md`. It reports 382 tokens
  over 12 files at this feature's head, and the command it names reads
  384. The count still falls from the 387 at the base, so the direction
  the entry claims holds and only the figure is stale (FR-038)

**Checkpoint**: The catalog publishes the scope refusal its enumeration
names, a cpu-target plan compiles over the entry that state names, the
bitmask carries the probe's per-kind verdicts, every assertion in the
suite can fail, and every recorded figure states a bound the evidence
meets.

---

## Phase 13: Convergence

**Purpose**: What the Phase 12 pass introduced and left unmeasured. Two
branches that pass added carry no coverage-exclusion marker and no test
reaches them, so the branch gate the specification records as passing
would fail on the runner that grants no event. The rest is a missing
contract, a conflated refusal, an assertion an earlier check decides, and
three records the tree does not support.

- [X] T096 [P] Drive the device-scoped arm of `probe_device` at
  `source/counters/linux_pmu/provider.cpp:193` from a registered test. No
  test sets `device_scoped` true before calling it;
  `unpublished_device_probe_scenario` sets it false, and the uncore
  fixture loads a device-scoped one without probing it. The arm carries
  no marker, so on a runner at `perf_event_paranoid` 2 the branch goes
  unmeasured and the gate fails (FR-040, FR-034, Principle VIII)

- [X] T097 [P] Drive the device-scoped arm of `settled_targets` at
  `source/counters/system.cpp:122` from a registered test. The arm needs
  a countable entry on a device-scoped object, which needs a granted
  cpu-targeted event, so the runner reaches neither side of that ternary
  and the branch goes unmeasured (FR-040, FR-021)

- [X] T098 [P] Extract the target-mask decision the way FR-046 names it.
  `settled_targets` sits in an anonymous namespace in `system.cpp`
  rather than the private seam header, carries a `//` block instead of a
  doxygen clause, and carries no paired enforcement site of its own; the
  `SG_ENSURE` that follows it is in the caller. Declare the decision in
  `source/counters/detail/pmu.hpp` beside the other three, with the
  clause and the enforcement site the requirement names, and let
  `object::counters()` call it (FR-046, FR-037, Principle II)

- [X] T099 [P] Keep an encoding refusal separable from a scope refusal
  on a device-scoped device. The new arm at
  `source/counters/linux_pmu/provider.cpp:193` publishes
  `scope_refused` whenever the cpu probe did not grant, so a
  device-scoped entry whose event the cpu-targeted form cannot encode
  publishes a scope refusal and loses the encoding cause FR-021 requires
  the caller to read apart (FR-021, US4/AC8)

- [X] T100 [P] Drop the mask assertion at
  `test/source/counters_pmu_test.cpp:423` that the check above it
  decides. The expectation at line 418 is built only from the two
  defined target bits, so a published mask equal to it carries no bit
  outside that pair, and the subset check cannot fail (FR-021, X.3)

- [X] T101 [P] Re-measure the coverage gate and record the head it was
  measured at. The line and branch figures in
  `specs/012-counters-defect-resolution/quickstart.md` were measured at
  `d2c590e`, which precedes the branches Phase 12 added, so the record
  of FR-040 passing describes a tree this feature no longer has
  (FR-040, SC-011)

- [X] T102 [P] Name the two figures the step 10 prose compares against,
  or withdraw the comparison. `quickstart.md:317` states the archive
  moved and the consumer moved by byte counts no artifact holds, because
  the figures T004 recorded were superseded and never written down
  (FR-023, SC-012)

- [X] T103 [P] Record the contradiction between acceptance scenario 2
  and FR-017 in this artifact. Scenario 2 and the Edge Cases list name
  `CounterMask`, `Invert`, and `EdgeDetect` among the keys that carry no
  encoding obligation, while FR-017 requires an encoding key to reach
  the kernel format it names including those three. The parser follows
  FR-017, and the acceptance scenario is the text in error (FR-016,
  FR-017, US4/AC2)

**Checkpoint**: Every branch this feature added is measured by a
registered test or carries a marker whose reason holds, the mask
decision carries the contract FR-046 names, a device-scoped entry keeps
an encoding refusal separable, every assertion in the suite can fail,
and every recorded figure names a head the tree had.

**On T103, the specification is the text in error.** The parser records
`CounterMask`, `Invert`, and `EdgeDetect` as the kernel formats `cmask`,
`inv`, and `edge`, which is what FR-017 requires and what the pinned
tables need to encode. Acceptance scenario 2 and the Edge Cases list name
those three among the keys that carry no encoding obligation, which no
reading of FR-017 supports. This artifact records the
discrepancy; `spec.md` names the text to correct, and a clarify pass
corrects it. No code changes with it.

---

## Phase 14: Convergence

**Purpose**: What the Phase 13 pass left standing. The target bitmask
names a target kind whose probe the running kernel refused, one
coverage marker's stated reason no longer holds, and three recorded
figures still describe a head or an arithmetic this feature no longer
has.

- [X] T104 [P] Name in `catalog_entry::targets` only the target kinds
  the probe settled. `settled_targets` at
  `source/counters/detail/pmu.hpp` derives the mask from the object's
  path and kind alone, so a countable entry on the core event source
  publishes both target kinds even where the cpu-targeted probe was
  refused. On the reference host that probe answers `EINVAL` and settles
  as `not_encodable`, so every one of the ten countable entries names a
  kind the kernel refused on that host, which is what FR-021 forbids and
  what T089 asked for. Carry the probe's per-kind verdicts to the mask
  instead of re-deriving them (FR-021, US4/AC9)

- [X] T105 Retire the coverage-exclusion region around `merge_vendored`
  at `source/counters/linux_pmu/provider.cpp:112`. Its stated reason is
  that the constructor reaches it only on a host that grants
  `perf_event_open`, and T093 gave a registered test its own call on every
  host. FR-046 keeps a marker over a wrapper only a granted event can
  enter and drops one a test now reaches, and this is the second shape.
  Removing it must leave the marker count below the recorded 374 and the
  coverage gate at 100 percent (FR-046, FR-027, FR-040)

- [X] T106 [P] Correct the linked consumer's delta in the step 10 table
  of `specs/012-counters-defect-resolution/quickstart.md`. The two totals
  the row names subtract to 25576571 and the cell reads 25276571
  (FR-023, SC-012)

- [X] T107 [P] Re-measure the clang-tidy count for the four units the
  convergence passes added code to and record the head. The table's
  current column names head `e307d39`, which precedes every convergence
  commit, and the passes touched `source/counters/plan.cpp`,
  `source/counters/system.cpp`, `source/counters/linux_pmu/provider.cpp`,
  and `source/counters/detail/pmu.hpp`. FR-041 holds per unit at the
  feature's head (FR-041, FR-042)

- [X] T108 [P] Record that the six contract checks the convergence
  passes added restate the expression on the line above them and so
  cannot fail. FR-046 asks each extracted decision for a paired
  enforcement site, and each has one, so the requirement holds in form.
  What the record must not leave implied is that a check which restates
  its own input adds nothing at run time. State it beside the coverage
  note in `quickstart.md` and leave the contracts in place (FR-046,
  Principle II)

- [X] T109 [P] Record what CTest reports for the two binaries that
  register `SKIP_RETURN_CODE 2`. `counters_pmu_test` runs eight
  scenarios and their assertions before `fast_window_lifetime_scenario`
  returns 2, so a green matrix records the test as skipped on a host
  that grants no event although the assertions ran. No assertion is
  skipped, and a failure exits 1 and is reported. State it beside the
  coverage note so a reader of the matrix does not read a skip as an
  absence of verification (SC-004, SC-011)

**Checkpoint**: The mask names only the kinds the probe settled, every
marker left in the tree has a reason that holds, and every recorded
figure states a head and an arithmetic the tree supports.

---

## Phase 15: Convergence

**Purpose**: What the Phase 14 pass left standing. Three gates the
feature's own record calls red sit behind closed tasks, four public
contracts document a precondition no implementation enforces, four
compiler extensions and one diagnostic suppression carry no registered
P2, one coverage marker's stated reason is false, and two recorded
figures describe heads this tree no longer has. The suite itself is
green: `ctest --preset=dev` passes 45 of 45, and no finding below is a
behavioral defect in the counters subsystem. Every CRITICAL item is
bookkeeping the constitution makes a hard gate.

### CRITICAL

- [X] T110 [P] Reformat the nineteen files `format-check` names and
  reopen T062. `cmake -D FORMAT_COMMAND=clang-format-18 -P cmake/lint.cmake`
  exits 1 and lists `include/speedgun-ng/counters_measurement.hpp`,
  `counters_provider.hpp`, `counters_system.hpp`,
  `source/counters/clock_provider.cpp`, `detail/pmu.hpp`,
  `fake_provider.cpp`, `fold.cpp`, `plan.cpp`, `push_provider.cpp`,
  `system.cpp`, `linux_pmu/embedded_tables.hpp`, `encode.cpp`,
  `fast_read.cpp`, `group_io.cpp`, `provider.cpp`, `table_parse.cpp`,
  `test/source/counters_clock_push_test.cpp`, `counters_noalloc_test.cpp`,
  and `counters_provider_ext_test.cpp`. Sixteen of the nineteen are in
  counters scope, which is the scope this feature owns. T062 is `[X]`
  (Constitution VIII, Constitution IX)

- [X] T111 Settle the clock-branch coverage discrepancy the feature
  records as unmet, and reopen T056. `quickstart.md:395-400` records
  `99.9 percent branch coverage on 786 of 787` and `:410` records
  FR-040 as unmet on the branch half. The stated cause is that the
  second arm of `if (disclosure_column != leaf_set::no_disclosure_column)`
  at `source/counters/clock_provider.cpp:237` reads as uncovered while
  `clock_disclosure_scenario` reaches it. The branch is real and the
  test is real; one of the two measurements is wrong. Settle it by
  measurement. Adding a marker settles nothing (Constitution VI,
  Constitution IX,
  FR-040)

- [X] T112 Discharge or formally defer FR-008, and reopen T019.
  `quickstart.md:190-215` records the core PMU group median falling from
  60.0 ns to 40.0 ns, about 33 percent against a 5 percent two-sided
  bound, and states that FR-008 remains undischarged on this host. T019
  is `[X]`. Either measure on a method that resolves the bound, which
  the same passage names as a finer clock or a run method that aggregates
  off the 10 ns tick, or record the deferral the way FR-039 records a
  suspected defect (Constitution IX, FR-008)

- [X] T113 Enforce the precondition `set_gap_actions` documents, or drop
  the clause. `include/speedgun-ng/counters_fake.hpp:220` documents
  `\pre the counter was declared.` `source/counters/fake_provider.cpp:242-244`
  asserts only that the object path and leaf name are non-empty, then
  calls `counter()` at `:253-255`, which reaches `std::map::at` on both
  the object and the leaf and throws `std::out_of_range` for an
  undeclared counter. The public seam reports an undeclared name through
  an exception that skips the contract facility, and the
  sibling `set_points` at `source/counters/fake_provider.cpp:169-171`
  enforces the identical precondition (Constitution II, FR-037)

- [X] T114 Enforce the address half of the precondition `push_provider::open`
  documents. `include/speedgun-ng/counters_push.hpp:88-90` documents
  `\pre Every address in leaves names a counter this provider declared,
  and all of them were declared on one thread (FR-035).` The
  implementation returns `nullptr` for the unknown-address half at
  `source/counters/push_provider.cpp:116-118` and enforces only the
  one-thread half at `:128`. The pair gate reports `open` as enforcing
  its precondition because one keyword matched (Constitution II, FR-037)

- [X] T115 Restate each of the six self-satisfying `SG_ENSURE`s over an
  input the decision did not consume, or remove the check and keep the
  doxygen clause. The six sit at `source/counters/plan.cpp:693`,
  `source/counters/linux_pmu/provider.cpp:392`, `:416`, `:448`, `:476`,
  and `source/counters/system.cpp:632`. Each compares a variable against
  the exact expression that produced it, so no configuration can report.
  `quickstart.md:373-381` records the shape and leaves the checks in
  place on the ground that the pairing gate wants a site. FR-046 asks the
  site to carry the guarantee, and a site that cannot fire does not
  (Constitution II, FR-046)

- [X] T116 Enforce the contract `set_thunk` documents, or withdraw the
  gate's exemption for protected members so the pair is visible.
  `include/speedgun-ng/counters_provider.hpp:329-333` documents
  `\pre fn is non-null` and `\post resolve_thunk returns fn`; the body is
  `{ m_thunk = fn; }`. `tools/dbc/dbc_doc_gate.py:427` exempts protected
  members, so the contract ships documented and unenforced with nothing
  reporting the gap (Constitution II, FR-037)

- [X] T117 Register a P2 for every compiler extension the counters scope
  uses beyond the one feature 007 registered. The registered P2 at
  `specs/007-counters-and-timers/plan.md:456` names `__rdtsc`,
  `_rdpmc`, and the `static_cast` of a `void*` mapping base, inside
  `source/counters/linux_pmu/fast_read.cpp`. Uncovered:
  `<cpuid.h>` and `__get_cpuid` at
  `source/counters/linux_pmu/table_parse.cpp:45`, `:70`, `:88`;
  `_mm_lfence` at `source/counters/linux_pmu/fast_read.cpp:334`, `:349`,
  `:375`, `:378`; `<immintrin.h>` at `source/counters/clock_provider.cpp:24`.
  Principle I admits an extension only against a registered P2 carrying
  written justification, and `grep -rn "cpuid\|_mm_lfence\|immintrin" specs/`
  finds no such entry (Constitution I)

- [X] T118 Register a P2 for the silent diagnostic suppression at
  `source/counters/linux_pmu/table_parse.cpp:24-26`, which carries
  `#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"`.
  `grep -rn "maybe-uninitialized" specs/` returns nothing. Either the
  initializer the pragma silences is fixed or the suppression is
  registered with its written justification at the site (Constitution VIII,
  Constitution X.2)

- [X] T119 Drop the coverage-exclusion marker at
  `source/counters/clock_provider.cpp:320-327`, and correct the reason
  beside it. The marker spans the in-loop `return nullptr` arm at `:322-326`,
  while its stated reason describes "the loop-exit edge of the enclosing
  `for`", which is not the arm it covers. The arm is reachable:
  `test/source/counters_clock_push_test.cpp:307` and `:310` assert that
  `clock_provider::open({"machine/nosuchclock"})` returns `nullptr`,
  which is this arm. A marker over covered code with a reason that
  misdescribes it fails the gate-honesty clause and raises the count for
  a path the suite already reaches (Constitution II, FR-027)

- [X] T120 Correct the stale coverage-exclusion figures in
  `specs/012-counters-defect-resolution/quickstart.md`. As this pass
  found it, the file recorded `374` lines and `provider.cpp 122` in the
  table headed "Measured at this feature's head", and recorded 372 in
  three other places, so the file contradicted itself. Every other
  per-file figure in that table matched the tree exactly. The
  reconciliation now records 367 and `provider.cpp` 120, which is what
  `grep -r 'LCOV_EXCL' source/counters/ | wc -l` reports after this pass
  dropped the clock-provider marker. The baseline the table is measured
  against still reads 377 with `provider.cpp` 122, and that one is
  correct: it is the figure at `6aafd2d` (FR-027, FR-042)

### HIGH

- [X] T121 [P] Re-measure the archive and consumer sizes at a head that
  carries the convergence passes' code, and correct the table at
  `specs/012-counters-defect-resolution/quickstart.md:308-317`. The
  after-embedding column names head `6904cd1`, which precedes every
  convergence commit, exactly the defect T101 and T107 were raised for
  on the coverage and clang-tidy figures and which this table never got.
  The delta arithmetic itself is correct and needs no change; only the
  head does. Reproduce with `size build/<rel>/libspeedgun-ng.a` and
  `size build/<rel>/test/<consumer>` after a release build
  (FR-023, SC-012)

- [X] T122 [P] Re-measure the two gated plans' post-fix medians at a head
  that carries the convergence passes' code, or state that the recorded
  `d2c590e` measurement is the final one and leave FR-008 open on that
  basis. `quickstart.md:184` records the post-fix column and `:661`
  dates the walk to `d2c590e`, which likewise precedes every convergence
  commit. T112 settles whether the bound is met; this task settles
  whether the figure describes the tree (FR-008)

- [X] T123 State the capacity bound `plan::recorder` imposes, or enforce
  it. `include/speedgun-ng/counters_measurement.hpp:908` documents
  `plan::recorder(std::size_t capacity)` as `\pre none`. A capacity of
  zero mints a recorder that aborts at
  `source/counters/plan.cpp:391`, where `hard_stop_sample_core`'s
  `SG_REQUIRE_ALWAYS` fires on the first sampling action, while the ring
  sibling refuses zero outright at `source/counters/plan.cpp:238`. Two
  overloads of one name disagree on the same input and one of them
  documents no bound (Constitution II)

### MEDIUM

- [X] T124 [P] Remove the four members this scope writes and never reads,
  or put each to the use it was added for. `plan_impl::bound_target` at
  `source/counters/detail/core.hpp:129` is written at
  `source/counters/plan.cpp:501` and read nowhere;
  `tree_node::provider_index` at `source/counters/detail/core.hpp:46` is
  written at `source/counters/system.cpp:356` and read nowhere, the read
  one being `leaf_record::provider_index` at `core.hpp:36`;
  `buffer_state::wrapped` and `buffer_state::dropped` at
  `source/counters/detail/core.hpp:55-56` are hardcoded to `false` and
  `0` at `core.hpp:171-172`; `pmu_window::discloses` and
  `pmu_fast_window::discloses` at
  `source/counters/linux_pmu/group_io.cpp:236` and `:381` are written at
  `:557` and `:647` and read nowhere (Constitution X)

- [X] T125 [P] Delete the two declarations this tree never instantiates.
  `concept provider` at `include/speedgun-ng/counters_provider.hpp:414`
  has no use anywhere in the repository, and the hidden friend
  `operator*(const expression&, const double)` at
  `include/speedgun-ng/counters_measurement.hpp:742-745` has no caller
  and no instantiation, confirmed with `nm` over `build/dev` and
  `build/tsan`. Its sibling `operator*(double, expression)` does have
  instantiations, so the pair is asymmetric. Neither is unused
  (Constitution X)

- [X] T126 [P] Remove the five includes this scope does not use:
  `<cstddef>` at `include/speedgun-ng/counters_core.hpp:4`,
  `<concepts>` at `include/speedgun-ng/counters_measurement.hpp:4`,
  `<cstdint>` at `include/speedgun-ng/counters_system.hpp:6` and at
  `source/counters/system.cpp:6`, and `<string>` at
  `source/counters/clock_provider.cpp:8`. Each header declares the types
  it needs through another include, so the direct include is dead
  (Constitution X)

- [X] T127 [P] Write the justification beside
  `static_cast<void>(context)` at
  `source/counters/linux_pmu/fast_read.cpp:183`, or remove the cast.
  Principle X admits a parameter discard only where a contract makes it
  sound, and only as a P2 with the justification written at the site.
  The two sibling discards at `source/counters/fold.cpp:226-230` and
  `source/counters/fake_provider.cpp:184-187` each carry one; this one
  carries none (Constitution X)

- [X] T128 Correct the `object::parent` clause at
  `include/speedgun-ng/counters_system.hpp:74`, which claims the parent
  is null "only for the machine root". `source/counters/system.cpp:528-530`
  also returns `nullptr` for a non-root object whose intermediate is
  absent, and `test/source/counters_fake_test.cpp:687` tests that case as
  the intended behaviour. The code is right and the clause is wrong
  (Constitution IV)

- [X] T129 [P] Restate the four contrastive clauses so each fact stands on
  its own terms: `source/counters/detail/pmu.hpp:181`, `:555`, `:580`,
  and `source/counters/system.cpp:262`. Each reads `X rather than Y`,
  which XI.2 admits only where the distinction is technical and the
  requirement is that the facts then be stated separately. `prose-lint`
  reads `, not `, ` rather than `, and ` instead of ` mechanically, so
  these four either need the restatement or a recorded exemption
  (Constitution XI.2)

### LOW

- [X] T130 [P] Cut the filler from the five sites that carry it:
  `just` at `source/counters/detail/core.hpp:97`, `and so on` at
  `include/speedgun-ng/counters_fake.hpp:181` and at
  `source/counters/linux_pmu/encode.cpp:25` and `:152`, and the `so that`
  at `include/speedgun-ng/counters_measurement.hpp:980`. XI.5 names
  `just` on its filler list and XI.7 puts `so that` in the reviewer tier
  (Constitution XI.5, Constitution XI.7)

- [X] T131 [P] Split the 122 comment sentences that run past the XI.7
  limit. WITHDRAWN as mis-specified by this pass, and the withdrawal is the
  work. The task this pass wrote asked for all 122 to be split while its own
  text recorded that XI.7 "targets roughly 80 percent compliance, so this is
  a reviewer tier and not a blocker". The constitution settles it.
  `.specify/memory/constitution.md:688-690` sets the obligation at "about
  80 percent compliance with ASD-STE100", and the amendment record at `:859`
  records XI.7 as "reviewer-enforced, XI.1 to XI.6 govern on conflict, no
  rule identifier added". `CANONICAL_IDS` at `tools/prose/prose_gate.py:57-65`
  carries XI.1 through XI.5 and no XI.7 rule, and
  `tools/prose/prose_rules.yaml` defines no XI.7 rule either. XI.7's own
  clause says the four words it names are ones "a reviewer reports" because
  "the gate cannot match them".

  So no gate reads sentence length, the obligation is a proportion and not
  a floor, and rewriting 122 sentences across a subsystem whose comments
  carry the specification's own reasoning would put that reasoning at risk
  for no gate that exists. The count stays recorded here as a measurement,
  the way the marker counts are recorded, and nothing is rewritten. If the
  owner wants the proportion raised, that is a deliberate sweep with its own
  measure before and after

- [X] T132 [P] Narrow the coverage-exclusion marker at
  `source/counters/linux_pmu/encode.cpp:31-50`, or correct its stated
  reason. The marker covers `word_of`'s two statements, which every call
  executes, while the reason names only the multi-word edge. Either the
  region shrinks to the edge or the reason names the region it covers
  (Constitution X.2)

**Checkpoint**: `format-check` and the branch-coverage gate report what
this feature's own record says they report, every public precondition
has an enforcing site that can fire, every compiler extension and every
diagnostic suppression carries a registered P2, every marker left in the
tree covers an arm the suite cannot reach, and every recorded figure
names a head the tree has.

**On T111, the first attempt at this task was wrong and the gate is now
green.** This task asked which of the two measurements was at fault, and
the first attempt here answered that the tracefile attributed every
`BRDA` record `source/counters/clock_provider.cpp` contributes to line 0,
so the unhit arc had no source line and no marker could cover it. That
answer was wrong. The `BRDA` record format is
`BRDA:<line>,<block>,<no>,<taken>`, and the reading took the fourth field
as the line number. The records carry their real lines: 61, 62, 215, 216,
237, and 318, and the single unhit arc is the second arm of
`if (disclosure_column != leaf_set::no_disclosure_column)` at
`source/counters/clock_provider.cpp:236`, which is the arm the feature's
own earlier reading named. That earlier reading was right and this pass
replaced it with a claim the artifact contradicts.

The measurement was stale as well. The tracefile that first attempt read
was dated two hours before the commits it claimed to measure, because the
capture runs through the `coverage` target and this pass had only built
the tree. After a rebuild of the coverage tree, a fresh `ctest` over it,
and a fresh capture, the gate reports 100 percent line coverage on 2086 of
2086 and 100 percent branch coverage on 789 of 789, and
`tools/dbc/coverage_gate.sh` exits 0. FR-040 is met on measurement.

The two markers were the whole of it. T119 dropped the clock-provider
region, whose stated reason named the loop-exit edge while it covered the
body, and whose arm `clock_disclosure_scenario` reaches on every host.
Dropping it raised the reported branch total from 787 to 795, because the
region had been hiding eight records, and left the `||` short-circuit
edge of `word_of` at `source/counters/linux_pmu/encode.cpp:47` as the one
unhit arc: T132's first narrowing had pulled the region back above that
loop's own edge. T132's correct narrowing keeps the region over both
short-circuit edges and drops it from the two statements every call
executes. No marker was added anywhere, and the marker count fell to 366.

**On T114, the header clause was the text in error, and the fix is a
narrowing of the clause.** Three things settled it:
`provider_iface::open` at
`include/speedgun-ng/counters_provider.hpp:398-407` documents `\pre none`
and states that a provider which cannot manage the leaves returns null;
the only call site, `source/counters/plan.cpp:558-582`, turns that null
into a recoverable `std::unexpected`; and the sibling
`include/speedgun-ng/counters_pmu.hpp:72-78` documents the identical
condition as "null when a leaf is not this provider's" under `\pre none`,
with `test/source/counters_clock_push_test.cpp:288-301` pinning the null
return as tested behaviour. Adding a check would have made the push
provider the only shipped provider that aborts where the PMU provider
refuses, and would have broken a passing test. The `\pre` at
`include/speedgun-ng/counters_push.hpp:88-90` now names the one-thread
half alone, which is the half `source/counters/push_provider.cpp:128`
enforces.

**On T123, the bound is stated in the brief, at the minting site.**
`include/speedgun-ng/counters_measurement.hpp:895-904` now states in its
brief that a zero capacity mints a recorder whose first sampling action
reports the bound `hard_stop_sample_core` enforces. Enforcing it at the
minting site instead was tried and reverted: `plan::recorder` has two
overloads, the gate keys on the interface name and skips the overload,
so a `\pre` on one and `\pre none` on the other reads as drift, and
`pair-gate` failed with `enforced-not-documented precondition` until the
check came back out. The ring sibling states its own capacity constraint
in its brief under `\pre none` for the same reason, and the two now match.

**On T112, the bound is now measured in a domain that resolves it, and
one gated plan passes while the other does not.** An earlier revision of
this pass recorded T112 as blocked, on the ground that the medians
quantize to a 10 ns tick and one tick is about a fifth of a 50 ns median.
That ground was wrong. `test/source/counters_overhead.cpp` already reads
the time-stamp counter for its own baseline, in both this tree and the
pre-fix worktree at `/tmp/opencode/sg-prefix`, and a tick at the measured
4.300 GHz is about 0.23 ns. A finer clock was available the whole time; the
earlier pass did not look for it and declared a limit that did not exist.

Both gated plans are now measured in that domain, in one session, pinned
to the same processor, with both trees built in the release preset:

| Plan | pre-fix `6aafd2d` | this head | change |
| --- | --- | --- | --- |
| core PMU group | median 242 t, 56.3 ns | median 219 t, 50.9 ns | -9.5 percent |
| clock leaf `machine/monotonic` | median 99 t, 23.0 ns | median 101 t, 23.5 ns | +2.0 percent |

The clock leaf sits inside the 5 percent two-sided bound and FR-008 is
discharged for it. The core PMU group sits 9.5 percent below its pre-fix
median, which is outside a bound that runs in both directions, so FR-008
stays open for that plan. Nothing regressed: no run at this head stands
above its pre-fix median. Whether a correction that makes the fast path
9.5 percent cheaper should fail a two-sided bound is a question about the
requirement, and restating it belongs to a specification decision.

The 33 percent fall this feature recorded for years was an artifact of the
nanosecond column. It is no property of either plan. That column is kept
above because it is what the earlier passes measured, and it no longer
carries the verdict.

**On T115, the premise holds and the characterization in the earlier
revision was too strong.** Each of the six checks compares a derived value
against the expression that derived it, inside a pure decision function
whose every input that expression consumes. No runtime input can falsify
such a check, because there is no input left over to falsify it with, and
that part of the earlier reasoning stands. What was overstated was calling
the checks vacuous and saying they can never report. They can, and they do.

Inverting the assignment at `source/counters/system.cpp:626` from
`countable ? named : target_mask {}` to `countable ? target_mask {} :
named` makes three registered tests abort, and the report names the
postcondition, the predicate, and the line:
`[postcondition] a state other than `countable` names no target kind ...
(predicate: settled == (countable ? named : target_mask {})) at
source/counters/system.cpp:630`. The fault was reverted and the suite is
back at 45 of 45.

So each check is a drift guard. It cannot be moved by a caller or by an
input, and it fires the moment an edit changes the decision without
changing the check beside it, which is the failure mode a comment copy of
a rule invites and the one Principle II's single-statement rule exists to
catch. Removing the six is also unavailable, because FR-046 requires each
extracted decision to carry a paired enforcement site.

Two facts make the arrangement cheap. The coverage extraction at
`cmake/coverage.cmake:64-73` omits every line matching
`SG_(REQUIRE|ENSURE|INVARIANT|ASSERT)(_ALWAYS)?`, so the six contribute no
line and no branch to the trace and their count is not part of the 100
percent gate either way. And the drift guard costs one comparison on a
path that already writes the value.



**T112 is measured, and the measurement moved it. Only T131 is left open
on purpose.**

- T112 is discharged. The bound is now measured in a domain that resolves
  it, and one of the two gated plans passes while the other does not. The
  tick figures and the verdict are recorded under step 8 in `quickstart.md`
  and under T112's own heading in this phase.
- T122 is discharged by the same re-measurement recorded under T112. The
  post-fix column names head `d2c590e` and now carries a second reading
  taken at this pass's head, so no figure in that table describes a head
  the tree no longer has.
- T131 is left open deliberately. 122 over-length sentences is a
  whole-tree prose sweep and no convergence task. XI.7 targets
  roughly 80 percent compliance, and the principle classes this as the
  reviewer tier. It is filed here and left untaken.

**Two gates this pass found red that no earlier pass had recorded.**
`prose-lint` exited 1 with 20 findings, and `spell-check` exited 1 on two
British spellings in `quickstart.md` that predate this feature. Both are
now clear of file findings: the seven remaining `prose-lint` findings are
all inside the commit messages of four earlier commits, which the
repository owner is handling, and `spell-check` passes. Neither gate had a
task behind it before this pass, which is the same bookkeeping gap T110
through T120 named.

**Four items a review of this pass found against it, and what they were.**

- T129 named four contrastive clauses and three were restated.
  `source/counters/detail/pmu.hpp:181` was not, and the prose gate cannot
  see it: `prose_rules.yaml` matches ` rather than ` with a leading space,
  and the comment-unit extractor emits one unit per comment line after
  stripping `//` and calling `.strip()`, so a contrastive phrase opening a
  line loses the space the pattern needs. Two more instances of the same
  blind spot sat at `test/source/counters_recorder_test.cpp:276` and
  `test/source/counters_pmu_test.cpp:259`. All three are restated, and a
  sweep over the counters scope, the public headers, the counters tests,
  and this feature's two spec directories now reports no blind spot. The
  The extractor's stripping is a defect in the gate itself, and these
  three lines are only where it showed.
- T115's disposition stands for five of the six sites and is corrected at
  `source/counters/linux_pmu/provider.cpp:416`. That clause asserted two
  facts and the tautology tested one: it claimed that only a countable
  entry publishes the enabled/running pair, and no check covered that half.
  The suggested restatement, comparing `publish_pair` against `probed`
  inside the countable arm, is as unfalsifiable as the original, because
  that arm cannot see the switch's other arms. The check's message now
  claims only what it enforces, and the comment above it already carries
  the design statement.
- T132's region was narrowed too far on the first attempt and the correct
  narrowing is in. `source/counters/linux_pmu/encode.cpp` now keeps the
  region over both short-circuit edges, which is what the third category
  of the registered P2 covers, and drops it from the two statements every
  call executes. The reason names both edges and no longer claims that no
  byte sequence reaches the `||`, and a nested comment repeating that
  false claim is gone.
- One commit of this pass failed the prose gate on its section token.
  `Build` is not one of the ten configured sections, so the commit that
  landed to clear a gate introduced a finding. Its subject is `Meta` now,
  and no commit on this branch reports a finding of its own.

**Checkpoint reached for every task except T131.**
`format-check`, `spell-check`, `dbc-gate`, the coverage gate, and `ctest`
all pass; `prose-lint` reports only the commit-message findings the owner
owns; the coverage-exclusion marker count fell from 372 to 366; and every
recorded figure names a head the tree has or names the head it was
measured at and says so.

---

## Phase 16: Convergence

**Purpose**: What a review of Phase 15 found against itself. Three
clauses sat in a blind spot the prose gate cannot see, the coverage
reading Phase 15 recorded rested on a stale capture and a mis-parsed
field, and the one commit Phase 15 landed to clear a gate failed that
same gate. All three are corrected and recorded below. Two items remain
still open and are named at the end.

### CRITICAL

- [X] T133 [P] Close the blind spot in the prose gate's comment-unit
  extraction, and record the blast radius before changing any pattern.
  `extract_c_comment_units` at `tools/prose/prose_gate.py:621-657` emits
  one unit per comment line and strips it: `text = re.sub(r"^\s*\*+\s*",
  "", "".join(collected)).strip()`. The XI2.CONTRASTIVE pattern at
  `tools/prose/prose_rules.yaml` matches ` rather than `, ` instead of `,
  and `, not `, each with a leading space or a comma. After the strip, a
  contrastive phrase that opens a comment line has no leading space, and
  the pattern cannot match it; a phrase wrapped across two lines is split
  across two units and cannot match either. The gate reports green on
  such a clause. Three instances sat in the counters scope and Phase 15
  corrected them by hand, and a sweep found no fourth, but the extractor
  hides the same shape in every C++ comment in the project. The pattern
  needs to tolerate start-of-unit and line-wrap; changing it will newly
  flag existing prose across the tree, so measure that first and record
  it. A gate that reports clean on prose the constitution forbids is a
  weakened gate under Principle VIII, and this one is silent
  (Constitution VIII, Constitution XI.2)

### LOW

- [X] T134 [P] Correct the stale line citations in the coverage P2 that
  feature 007 registered at `specs/007-counters-and-timers/plan.md`.
  The row names `include/speedgun-ng/counters_measurement.hpp:619` for
  the defaulted `expression()` constructor and
  `include/speedgun-ng/counters_provider.hpp:110` and `:281` for the
  defaulted virtual destructors. At this head `:619` is a comment opener,
  `:110` is a defaulted copy constructor, and `:281` is a `\pre none`
  line. The row's prose description is corroborated: the six functions at
  zero are the three `expression<...>` defaulted constructors and the
  three deleting destructors, which is what the row says they are, and
  the functions axis no gate scores. Only the line numbers are wrong.
  Reproduce with `python3 -c` over `build/coverage/coverage.info`
  counting the `FNA` records whose `FN` count is zero
  (Constitution IV)

**T115 is answered with an experiment, and T131 is withdrawn as a task
this feature should not have written.** T115's six
checks stay as T108 dispositioned them, and the one site whose clause
asserted more than its check tested is corrected with a narrower message
rather than a restatement that could never fire. T131 is a whole-tree
prose sweep in the reviewer tier.

**On T133, the extractor now keeps the leading space, and a control
proves the blind spot and the repair.** The earlier revision of this
record filed the defect against another feature and named the rule
contract as the obstacle. Both claims were wrong. There is no feature
that holds this file open; feature 002 closed its 36 tasks, and the last
eight commits to `tools/prose/prose_gate.py` are `Meta:` commits. And the
load-time probe at `tools/prose/prose_gate.py:389-408` runs against the
compiled pattern, never reaching the extractor, so the contract was never
in the path.

The cause is one call. `extract_c_comment_units` at `:654` ended with
`.strip()`, and a line comment hands the extractor its text with the
leading space still attached. `XI2.CONTRASTIVE` needs a space in front of
the pair, so a pair opening a comment line lost the space the pattern
requires and passed unreported. The repair keeps that space and spends
exactly one space on a block comment's asterisk.

The control, on `source/counters/fold.cpp` with one injected line reading
`// rather than the recorded figure, ...`:

| Tree | Result |
| --- | --- |
| injected line, `.strip()` in place | 80 units examined, 0 findings |
| injected line, leading space kept | 80 units examined, 1 finding, reported at line 1 |

Both runs read committed state through `--mode tree`, so both saw the
same injected line. The first run is the defect and the second is the
repair, on the same input. The injected line was reverted and the tree is
back at 45 of 45.

The whole-tree run after the repair reports the same seven findings it
reported before, all of them inside commit messages, and no file-level
finding. The measured size of the defect in this tree is no line
currently carries it, which is why the four-line figure earlier in this
artifact was wrong. It came from a simulation that re-tested the spaced
pattern against text it had already stripped.

One gap remains and this repair does not reach it. The extractor emits one
unit per line, so a pair wrapped across two comment lines sits in two
units and no per-line change joins them. Closing that means emitting a
comment block as one unit and deciding which line a finding reports, which
changes the gate's reporting contract. That is a separate change with its
own measure.

**On T134, four line numbers moved.** The row named
`include/speedgun-ng/counters_measurement.hpp:619` for the defaulted
`expression()` constructor and `include/speedgun-ng/counters_provider.hpp:110`
and `:281` for the defaulted virtual destructors. The constructor is at
`:617`; the three destructors are at `:109`, `:292`, and `:383`. The row's
substance needed no change: those six functions are the six the capture
reports at zero, which is what the row says they are.

**Checkpoint**: every task in this artifact is closed. `format-check`,
`spell-check`, `dbc-gate`, the coverage gate, and `ctest` all pass;
`prose-lint` reports no file-level finding and its seven remaining
findings sit in four commits from before this feature's convergence work,
which are the repository owner's to reword; the marker count stands at 366
against the pre-fix 377; and every recorded figure names a head the tree
has.

Two obligations stay open by their own terms, and neither is code.
FR-008's core PMU group reads 9.5 percent below its pre-fix median, which
a two-sided 5 percent bound does not accept, and whether that requirement
should be restated is a specification decision. The prose gate still
reports nothing for a contrastive pair wrapped across two comment lines,
because its extractor emits one unit per line.
