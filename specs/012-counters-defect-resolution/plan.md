# Implementation Plan: Counters Defect Resolution

**Branch**: `012-counters-defect-resolution` | **Date**: 2026-10-04 |
**Spec**: [spec.md](spec.md)

**Input**: Feature specification from
`/specs/012-counters-defect-resolution/spec.md`

## Summary

Ten defects in the counters library are corrected so that every value
the library hands a caller is a correct value, or the library discloses
that the value is unavailable. Four P1 defects land first: the fast
read publishes a masked value and a stale retry (I-04), a short group
read is indistinguishable from a zero count (I-05), catalog resolution
writes shared maps without synchronization (I-06), and a fast-mode plan
releases no descriptor and no mapping (I-08). Six P2 defects widen the
hosts and the deployments the harness serves: Intel table rows carry an
encoding obligation no kernel format names (I-01), vendored rows reach
one device (I-02), every availability probe asks as one target (I-03),
the tables resolve through a build-time path into the source tree
(I-07), the published overhead floor carries the clock reads that
bracket it (I-09), and two clock leaves document an order their clock
does not provide (I-10).

The technical approach is set by three constraints. A refused,
unstable, or failed read discloses a gap through one managed column
written through the existing `point_sink::put`, and keeps no earlier
value. The availability state gains a value that separates a scope
refusal from an encoding refusal, and a fixed-size bitmask beside it
names the target kinds an entry can be counted on. The vendored event
tables travel inside the library archive as static bytes that the
existing simdjson parse path decodes at run time, so an installed
package publishes the catalog the build tree publishes.

Every covering test is written and observed failing at `6aafd2d` before
the correction that turns it green. **TDD mode is in force** (FR-033,
Principle III).

The technical context below names no unresolved item. The decisions
behind it are in [research.md](research.md), and each one records its
rationale and the alternatives it rejected.

## Technical Context

**Language/Version**: C++23, `CMAKE_CXX_EXTENSIONS=OFF`, GCC and Clang

**Primary Dependencies**: Zero external runtime dependencies, and this
feature adds none. The counters library reaches the kernel through the
kernel's perf interface header and the vendored simdjson parse path. The
vendored tree under `external/pmu-events` supplies the event tables and
is re-pinned by nothing in this feature.

**Storage**: The vendored event tables compile into static data inside
`libspeedgun-ng.a` as raw JSON bytes. This is new in this feature and
replaces the `SG_PMU_EVENTS_DIR` build-time path into the source tree.
An installed archive has no run-time location, so the data travels
inside it.

**Testing**: CTest over frameworkless test binaries that use the
`check()` and `fail()` convention in `test/source/`. Synthetic sysfs,
table, and event-page fixtures stand in for the kernel. One new
`ci-tsan` preset and one new `tsan` CI job carry the thread sanitizer,
which stays apart from the existing `ci-sanitize` preset because the
compilers reject the pairing.

**Target Platform**: Linux on GCC and Clang, on Ubuntu for the main
jobs and Rocky Linux for the container job. Every test runs
unprivileged at `perf_event_paranoid` 2.

**Project Type**: library, a static archive published as a CMake
package through `speedgun-ng::speedgun-ng`

**Performance Goals**: `recorder::sample()` stays `noexcept`,
allocation-free, and lock-free. Two gated plans hold their release-build
median within 5 percent of the figure measured at `6aafd2d`: the core
PMU group over `cpu/instructions` and `cpu/cpu-cycles` with one read per
leader per action, and the clock-leaf plan over `machine/monotonic`
beside it (FR-008). The published per-plan overhead floor excludes the
cost of the two clock reads that bracket each sampling action (FR-025).
The added archive and executable sizes are measured and recorded
(FR-023, SC-012). The 5 percent bound is a recorded measurement on the
reference host under one run method, and a registered test asserts only
what a CI runner can decide, that the sampling action is `noexcept`,
allocates nothing, and takes no lock, because Constitution VI requires
every test to be deterministic.

**Constraints**: 100 percent line, branch, and contract coverage on
every changed line (FR-040). The clang-tidy warning count of each
touched translation unit does not rise (FR-041). The count of
coverage-exclusion markers in `source/counters/` does not rise (FR-027),
and this feature removes one region. No public header gains a platform
term (FR-036). No benchmarking-framework code enters the library
(FR-035). Every corrected line traces to a requirement in this
specification (FR-042, X.3).

**Scale/Scope**: 46 requirements, 12 success criteria, 7 user stories,
10 defects. The vendored table tree holds 542 JSON files totaling
24.5 MB across 40 architecture directories, of which every executable
that links the library carries the embedded copy.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1
design.*

**Simulated at**: 2026-10-04, against constitution 2.12.0

### Pre-design gates

| Principle | Gate | Status |
| --- | --- | --- |
| I | C++23 only, no compiler extension, no new dependency | PASS, no new dependency |
| II | Every changed interface carries a doxygen contract paired with a registered enforcement counterpart, and `dbc-gate` proves the pairing | PASS, the plan names the pairing per changed interface |
| III | R-DCUT through Spec Kit, and the plan records TDD mode where a feature uses it | PASS, TDD mode recorded below |
| IV | No content-free comments, no TODO, every interface documented | PASS |
| V | `format-check` passes at the pinned formatter, formatting-only changes commit apart | PASS |
| VI | Every fix ships with a test that fails at `6aafd2d` and passes after, and coverage holds at 100 percent on all three gates | PASS, FR-032 and FR-040 |
| VII | Critical-path cost tracked, results reported as distributions, no P0 technique without documentation | PASS, FR-008 gates two medians against the pre-fix figures |
| VIII | Every hard gate passes, including the sanitizer presets, the coverage gates, `dbc-gate`, `format-check`, `spell-check`, and `prose-lint` | PASS, subject to the 2.13.0 amendment recorded below |
| IX | The workflow runs before code, and each feature builds once in the release preset | PASS |
| X | Minimal diffs, no speculative generality, every changed line traces to a requirement, goal-driven checks | PASS, the Complexity Tracking table below is empty |
| XI | Generated prose satisfies the discourse and prose standards | PASS, this plan is written to them and the gate runs over it |

### Gates this feature changes

Principle VIII holds the gate list. FR-012 and SC-003 require a
thread-sanitizer preset and a CI job, so a gate that did not exist
becomes one CI enforces on every change.

Principle VIII's final item states that changing the gate set itself
requires a constitution amendment. That clause is unqualified. It
governs an addition, and it governs a removal, so adding the gate triggers
it.

The amendment is `.specify/memory/constitution.md` at version 2.13.0,
classified MINOR because guidance expanded inside an existing principle
and no obligation was withdrawn. Its precedent is amendment 2.5.0,
which added a hard gate item to Principle VIII and shipped as MINOR.

The gate item names `ci-tsan` and `tsan`. Tasks T020 and T024 add
them, and the amendment lands in the same change, because a gate
naming a preset and a job that do not exist is a gate no change can
pass.

No existing gate, threshold, warning class, analyzer invocation,
runner, or preset changes. `ci-sanitize` keeps address and undefined
behavior alone, because the compilers reject the pairing.

### Principle VII baseline infrastructure

Principle VII mandates per-platform baseline metrics and records their
absence as an open deferral. FR-008 does not close that deferral. It
records two figures per gated plan, the pre-fix median and the
post-fix median, under one run method, and holds the post-fix figure
within 5 percent of the pre-fix figure. That is a regression bound
across one change, and it is the bound this feature needs. A
cross-platform baseline threshold file remains absent, and a later
specification delivers it.

### TDD mode

**Recorded as required by FR-033 and Principle III.** TDD mode is in
force for this feature. The covering test for each of I-01 through I-10
is written first, and the plan records that it is observed failing at
`6aafd2d` before the correction lands (FR-032). A test passing before
its implementation exists does not satisfy the requirement. The task
artifact orders every test task before the code task it gates.

### Version lineage

The 0.4.0 bump records two removals commit `cd5cbd1` made. It removed
the non-member multiplication of an expression by a double, and it
removed the provider concept. Both declarations remain absent from the
public headers. The project version moves from 0.3.0 to 0.4.0 with
this record (FR-020, FR-032).

The release is 0.4.0. `CMakeLists.txt` carries that version, and
`SOVERSION` is 1. The shared-object version is stated, because the major
position on the 0.x line is 0 and a consumer links against the
shared-object version (FR-021, FR-032, SC-011).

### Deferred confirmation

The Intel confirmation of I-01 and I-04 on hardware is recorded as
deferred (FR-039). The reference host is an AMD Ryzen 9 9950X3D
running Linux 7.2.4-1-cachyos with glibc 2.44. The host class that
settles both parts is an Intel host whose kernel grants
`cap_user_rdpmc` and whose PMU publishes the core event formats the
vendored Intel tables name. The repository owner expects to add an
Intel host later. No requirement, test, or gate depends on the
confirmation, because SC-005 measures the encodable-row count over the
pinned tree against a named synthetic format list, so one number gates
every host on the matrix. The suspected parts of both defects stay in
scope until that host answers.

### Post-design gates

Re-evaluated after Phase 1, on the evidence of
[data-model.md](data-model.md), [contracts/](contracts/), and
[quickstart.md](quickstart.md).

| Principle | Gate | Post-design status |
| --- | --- | --- |
| II | Contract paired with enforcement on every changed interface | PASS. Three public headers change. Each contract in `contracts/` names the doxygen clause and the `SG_REQUIRE`, `SG_ENSURE`, or `SG_INVARIANT` site beside it. |
| II | No contract check emits code in release | PASS. FR-045's cpu-target pinning precondition is a semantic-gated `SG_REQUIRE`; a release build configured `ignore` emits no code for it. |
| VII | The hot path takes no new branch from a contract | PASS. The disclosure column adds one managed-column write per sampling action, and FR-008 measures it. |
| X.2 | No new extension hook, injection seam, or configurability | PASS. D-15 routes the everywhere-runnable lifecycle test through the release function the correction already needs, so no seam enters production code. The table embedding carries no build option, on the terms of the clarification of 2026-10-03. |
| X.2 | No coverage exclusion added | PASS. The calibration region at `source/counters/plan.cpp:256` is removed, because a registered test now reaches the calibration. The release-arm markers at `source/counters/linux_pmu/fast_read.cpp:353-372` are removed, because a registered test now reaches those arms with a descriptor and a mapping the test opened itself. The remaining markers stay. A marker over a wrapper only a granted `perf_event_open` can enter stays, because commit `f963fda` records that the CI runner refuses every `perf_event_open` while a developer host grants it. No single host reaches 100 percent, so the markers over both halves together cover what neither host covers alone. Removing such a marker fails the coverage gate on both hosts. This feature therefore adds no marker and removes none from a kernel-facing wrapper. |
| VI | A corrected decision on a kernel-granted path is measured | PASS. FR-046 requires each such decision to be a seam-declared pure function, and the repository already holds two of that shape: one deciding whether a fast read is permitted, and one deciding whether a seqlock read is stable. A registered test covers both arms of each. |
| VI | Every changed line covered by a registered test | PASS. The test plan in `quickstart.md` names a registered test per success criterion. |
| IX | All artifacts produced | PASS. `spec.md`, `plan.md`, `research.md`, `data-model.md`, `contracts/`, `quickstart.md`, and `tasks.md`. |

## Project Structure

### Documentation (this feature)

```text
specs/012-counters-defect-resolution/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output (/speckit.plan command)
├── data-model.md        # Phase 1 output (/speckit.plan command)
├── quickstart.md        # Phase 1 output (/speckit.plan command)
├── contracts/           # Phase 1 output (/speckit.plan command)
│   ├── availability.md
│   ├── fast-read-fold.md
│   └── clock-order.md
└── tasks.md             # Phase 2 output (/speckit.tasks command)
```

### Source Code (repository root)

Only the paths this feature touches appear below.

```text
include/speedgun-ng/
├── counters_core.hpp        # availability enumeration, catalog_entry
├── counters_provider.hpp    # point_sink disclosure column
└── counters_clock.hpp       # per-leaf order guarantees

source/counters/
├── plan.cpp                 # plan compile, overhead calibration (I-09)
├── system.cpp               # catalog resolution, open flag (I-06)
├── fold.cpp                 # leaf_ratio disclosure (I-04)
└── linux_pmu/
    ├── provider.cpp         # device scope, target probe (I-02, I-03)
    ├── table_parse.cpp      # encoding rule, embedded (I-01, I-07)
    ├── fast_read.cpp        # decode recipe, pair read (I-04)
    └── group_io.cpp         # group read, window release (I-05, I-08)

source/counters/detail/
└── pmu.hpp                  # fast_context release (I-08)

test/source/
├── counters_linux_pmu_seam_test.cpp   # decode, table, device, lifetime
├── counters_pmu_test.cpp              # real-host halves, availability
├── counters_overhead.cpp              # FR-008 figures, floor test
└── counters_recorder_test.cpp         # disclosure column, concurrency

cmake/
└── EmbedPmuEvents.cmake               # new: static table data (I-07)

CMakePresets.json            # ci-tsan preset (I-06)
CMakeLists.txt               # 0.3.0 (FR-024), embedding, ci-tsan
.github/workflows/ci.yml     # tsan job (I-06)
specs/007-counters-and-timers/citations-log.md  # corrections (FR-038)
```

**Structure Decision**: The repository is a single library with one
test directory, and this feature keeps that layout. The counters code
lives in `source/counters/` with the Linux PMU provider in
`source/counters/linux_pmu/` and its shared declarations in
`source/counters/detail/pmu.hpp`, which the seam test already
includes. No new source directory, no new public header, and no new
test binary is required: every corrected unit already has a translation
unit and every covering test has a home. The two additions outside
`source/counters/` are one CMake module for the table embedding and one
CI job, each of which a requirement names.

## Design

The logical view states what each corrected subsystem is and how it
behaves after the correction. The physical view states where the change
lands. The verification matrix maps every requirement to the file that
satisfies it and the test that proves it. The test plan states the
method each requirement's check runs.

### Logical view: one fast-path sampling action

Four components take part. `plan` owns the compiled layout and hands
each sampling action to a window. A window reads one mapped page per
member leaf, plus the leader's enabled and running pair. `point_sink`
writes one point per managed column. `availability` carries the
countability value the action discloses.

The action runs in this order.

1. `plan::recorder()` returns a recorder over the compiled layout. One
   sampling action is one call to `recorder::sample()`.
2. The window's read thunk runs. It is a static function the window
   constructor installed, so the compiled plan takes one indirect call
   and no vtable lookup.
3. Each member's page is read under its seqlock snapshot. The read
   yields one of three verdicts: `ok`, `not_allowed`, or `unstable`.
4. An `ok` member publishes the page's offset added to the value the
   instruction read, sign extended from the published `pmc_width`. Any
   other verdict publishes zero and marks the action.
5. The leader's page publishes `time_enabled` and `time_running`. A
   page that discloses no stable pair publishes a zero pair and marks
   the action.
6. `point_sink::put` writes each managed column in `leaf_set` order:
   the counts, then the ratio pair's two columns, then the disclosure.
7. `point_sink::check_action` asserts one write per managed column.
8. The window's disclosure column carries the entry's countability
   value on a measured action, and the availability state's gap value on
   an action that measured nothing.
9. The disclosure column is one managed column per sampling action. The
   compile-time pass that resolves the ratio pair's indices resolves
   its index too, and the sampling action writes it last, after the
   counts and the ratio pair's two columns.

**State**: the window holds no per-action state that outlives the
action. Every value a column receives comes from the action that wrote
it, so a refused read leaves a zero and never an earlier count. Because
the disclosure column carries a value that differs between a measured
action and an unmeasured one, a caller tells a real zero count from a
gap by reading the column beside it.

**Invariant**: `recorder::sample()` allocates nothing, takes no lock,
and gains no branch from a contract check. The disclosure adds one
managed-column write per action, and FR-008 measures its cost.

**Contract pairing**: the decode recipe carries an `SG_ENSURE` on the
value an `ok` verdict leaves. The cpu-target pinning precondition
carries a semantic-gated `SG_REQUIRE`, which a release build configured
`ignore` does not emit.

### Logical view: catalog resolution and plan compile

Three components take part. `system` owns the handle map and the object
tree. `plan::compile` resolves leaves against the tree and lays out the
compiled plan. The open boundary sets the state both share.

Resolution runs in this order.

1. A caller names a canonical address.
2. The handle map is consulted under its lock. A hit returns the
   published entry.
3. A miss constructs the handle, inserts it under the same lock, and
   returns it. Two threads naming one address both return the same
   entry.
4. A parent or children walk reads the same map under the same lock and
   returns the published relationships.

A plan compile resolves every leaf the same way, builds the layout in
storage the plan owns, and writes no state any other compile reads. The
open flag is written once, at the open boundary, and no compile path
writes it.

**Invariant**: after open, every read of the catalog and every compile
is safe from any number of threads at once. The lock covers the maps;
no contract check guards them, because a lock does.

### Logical view: fast-mode window lifetime

Three components take part. `fast_context` owns one descriptor and one
mapping. The member list owns one `fast_context` per leaf. One release
function releases them all.

The lifetime runs in this order.

1. `fast_context_open` binds the event to the plan's target, maps one
   page, and returns the context.
2. Each leaf's context joins the member list. A member whose open fails
   makes the whole window open fail.
3. On a failed open, the release function releases every member the
   window acquired so far, and no window is published.
4. On a successful open, the window serves sampling actions.
5. On destruction, the release function releases every member, and each
   context unmaps its page and closes its descriptor.
6. On destruction, each context unmaps its page and closes its
   descriptor through its own destructor, which delegates to the
   existing release function.
7. A registered test reaches that destructor on any host. It opens its
   own descriptor and its own anonymous mapping, places both in a
   context value, and lets the value leave scope. The destructor
   performs a real release, and the test compares `/proc/self/fd` and
   `/proc/self/maps` with their values before the scope.

The release path reads the mapping address, the length, and the
descriptor, and nothing else, so a test-supplied resource releases
exactly as a granted one does. No new production symbol is required,
and the private header the test includes is one the seam test already
includes today.

**State**: ownership lives in the member list and nowhere else. A
context has one owner at every moment, and its destructor performs the
release, so every exit path releases.

**Invariant**: the descriptor count and the mapping count return to
their starting values after any number of open and destroy cycles,
including a partial open.

### Logical view: catalog contents

Three stages build a catalog entry, and each stage decides one thing.

**Encoding.** The table parser reads one row. A numeric key becomes an
encoding field only where the key names a format the running device
publishes. A sampling key or a metadata key carries no encoding
obligation. `pmu_compose_config` then resolves each remaining field
against the device's `format/` directory. A row whose fields all resolve
encodes. A row that needs an absent field publishes `not_encodable`.

**Placement.** The row's table scope decides the device. A core-scoped
row reaches each core device the scope applies to. An uncore-scoped row
reaches the uncore device. A row scoped to a device the host does not
publish stays out of the catalog, and no probe runs for it.

**Availability.** A probe runs once per target kind the entry's bitmask
admits. The probe's verdict becomes the entry's countability state: a
kernel refusal becomes `permission_blocked`, a missing event becomes
`absent`, a scope refusal becomes `scope_refused`, and a resolvable
event becomes `countable`. The bitmask names the target kinds that
resolved, and it allocates no memory. The availability state also
carries one value naming the per-action gap case, and the catalog never
publishes that value for an entry, because a gap is a property of one
sampling action and an entry spans many.

**State**: an entry is frozen once the system is open. Its strings are
immutable, its state is fixed, and its bitmask is fixed.

### Logical view: the corrected decisions and the seam that reaches
them

Four corrected decisions sit on paths only a granted `perf_event_open`
can enter. The coverage gates exclude them on the runner that refuses
the event and measure them on the developer host that grants it, so
neither host reaches 100 percent alone.

Each decision moves into a small pure function declared in the
project's existing private seam header. The kernel-facing wrapper calls
it. A registered test calls the same function over synthetic input.
The function carries a doxygen clause and a paired enforcement site, so
`dbc-gate` proves the pairing like any other.

| Decision | Kernel call it sits behind | Seam function shape | Covering test |
| --- | --- | --- | --- |
| Whether a group read returned fewer bytes than the group header | `read` on the group leader | a predicate over the returned byte count and the header size | `counters_linux_pmu_seam_test` |
| Whether the leader's page disclosed a stable enabled and running pair | the seqlock read of the leader's page | a step that takes the pair verdict and the two current values, and returns the values a fold may read | `counters_linux_pmu_seam_test` |
| Which read mode and time pair one catalog entry publishes | the availability probe | a selection over the probe's verdict and the fast-capability flag, returning the mode and whether to publish the pair | `counters_pmu_test` |
| Whether a granted mapping and descriptor are released | `munmap` and `close` | no extraction is needed: the release forwards the address, the length, and the descriptor, and reads nothing else, so a test supplies its own | `counters_linux_pmu_seam_test` |

**Invariant**: no corrected decision is measured only through a
coverage-exclusion marker, and this feature adds no marker.

### Logical view: the embedded table data

One component takes part: a static-data blob carrying the vendored JSON
bytes, produced at build time and decoded at run time.

The flow runs in this order.

1. The build reads every `.json` file under `external/pmu-events`.
2. A generated translation unit holds the concatenated bytes and its
   length.
3. At first use, the parser decodes those bytes with the existing
   simdjson path and caches the parsed table per directory.
4. No run-time path lookup happens at any step.

**Invariant**: the embedding is unconditional. An installed archive
carries the data, so an installed package publishes the catalog the
build tree publishes. The build adds no option that turns the embedding
off.

### Logical view: the per-plan overhead floor

One component takes part: `calibrate`, reached through the plan's three
`sample_overhead_ns_*` accessors.

The calibration runs in this order.

1. The plan reports whether it has calibrated. A second request returns
   the stored figure.
2. Two clock reads bracket the sampling action, and two more bracket
   the identical bracket alone.
3. The bracket's own cost is subtracted from the sampling action's cost.
4. The minimum, median, and maximum are stored and returned.

**Invariant**: no coverage exclusion region covers the calibration, so a
registered test reaches every line of it. A host whose clock costs more
than the sampling action publishes a floor of zero, and the published
figure names that condition.

### Logical view: the clock leaves

One component takes part: the clock provider, which seeds the `machine`
object's leaves.

Each leaf documents its own order guarantee, and the class contract
states no order on any leaf's behalf. A guarantee that a leaf's clock
does not provide is a defect, so each leaf's clause names the clock it
reads and the order that clock keeps.

| Leaf | Clock | Guarantee |
| --- | --- | --- |
| `machine/monotonic` | `CLOCK_MONOTONIC` through the vDSO | non-decreasing on one thread |
| `machine/monotonic_raw` | the same clock, unadjusted | as `monotonic` |
| `machine/thread_cpu` | per-thread CPU time | non-decreasing on the reading thread; a new thread's first sample can fall below an earlier sample from another thread |
| `machine/process_cpu` | per-process CPU time | non-decreasing across the process's threads |
| `machine/tsc` | the processor's cycle counter, read with no fence | ordered only when one thread takes both window endpoints |

**Invariant**: no leaf gains an ordering fence. A fence helps only a
caller that reads across threads, and the harness takes both window
endpoints on one thread.

### Physical view: change per file

| File | Requirements | Change |
| --- | --- | --- |
| `include/speedgun-ng/counters_core.hpp` | FR-021, FR-024 | add `scope_refused`, the target-kind bitmask typedef, and `catalog_entry::targets` |
| `include/speedgun-ng/counters_provider.hpp` | FR-007 | document the disclosure column; `put` keeps one integer |
| `include/speedgun-ng/counters_clock.hpp` | FR-028, FR-030, FR-031 | one order guarantee per leaf, and a class clause that states none |
| `source/counters/plan.cpp` | FR-007, FR-011, FR-025, FR-026 | resolve and store the disclosure column; move the open flag; subtract the bracket; drop the exclusion region |
| `source/counters/system.cpp` | FR-010 | guard the handle map |
| `source/counters/fold.cpp` | FR-005 | report a measured ratio only for an action the disclosure marks measured |
| `source/counters/linux_pmu/fast_read.cpp` | FR-004, FR-045 | sign-extend and add the offset; judge every verdict; state the pinning precondition; release through the context destructor; drop the release-arm coverage-exclusion markers, because a registered test reaches those arms with resources the test opened |
| `source/counters/linux_pmu/group_io.cpp` | FR-002, FR-003, FR-006, FR-007, FR-013, FR-014, FR-046 | disclose a refused or short read; release through one function |
| `source/counters/linux_pmu/provider.cpp` | FR-001, FR-018, FR-019, FR-022, FR-046 | route rows by scope; probe per target kind |
| `source/counters/linux_pmu/table_parse.cpp` | FR-016, FR-017, FR-023 | record an encoding field only for a published format; decode the embedded bytes |
| `source/counters/detail/pmu.hpp` | FR-013 | give `fast_context` a destructor and declare the release function |
| `cmake/EmbedPmuEvents.cmake` | FR-023 | compile the vendored JSON into static data |
| `CMakePresets.json` | FR-012 | add the `ci-tsan` configure, build, and test presets |
| `CMakeLists.txt` | FR-024 | move `VERSION` to `0.3.0`, leave `SOVERSION` at 0 |
| `.github/workflows/ci.yml` | FR-012 | add the `tsan` job |

### Verification Matrix

Every changed line traces to a requirement through this table
(FR-042, X.3). The test column names the registered test that proves
the requirement.

| Requirement | Satisfied in | Proved by |
| --- | --- | --- |
| FR-001 | `linux_pmu/provider.cpp` | `counters_pmu_test`, `counters_linux_pmu_seam_test` |
| FR-002 | `linux_pmu/group_io.cpp` | `counters_linux_pmu_seam_test` |
| FR-003 | `linux_pmu/group_io.cpp` | `counters_linux_pmu_seam_test` |
| FR-004 | `linux_pmu/fast_read.cpp` | `counters_linux_pmu_seam_test` |
| FR-005 | `linux_pmu/group_io.cpp`, `fold.cpp` | `counters_linux_pmu_seam_test` |
| FR-006 | `linux_pmu/group_io.cpp` | `counters_linux_pmu_seam_test` |
| FR-007 | `plan.cpp`, `linux_pmu/group_io.cpp`, `counters_provider.hpp` | `counters_recorder_test` |
| FR-008 | `linux_pmu/fast_read.cpp`, `linux_pmu/group_io.cpp` | `counters_overhead` |
| FR-009 | no file changes; a check over `include/speedgun-ng/` | a `grep` over `counters_measurement.hpp` and `counters_core.hpp` |
| FR-010 | `system.cpp` | `counters_recorder_test` under `ci-tsan` |
| FR-011 | `system.cpp`, `plan.cpp` | `counters_recorder_test` under `ci-tsan` |
| FR-012 | `CMakePresets.json`, `.github/workflows/ci.yml` | the `tsan` CI job |
| FR-013 | `detail/pmu.hpp`, `linux_pmu/group_io.cpp` | `counters_linux_pmu_seam_test`, `counters_pmu_test` |
| FR-014 | `linux_pmu/group_io.cpp` | `counters_linux_pmu_seam_test` |
| FR-015 | `linux_pmu/group_io.cpp` | `counters_linux_pmu_seam_test` |
| FR-016 | `linux_pmu/table_parse.cpp` | `counters_linux_pmu_seam_test` |
| FR-017 | `linux_pmu/table_parse.cpp` | `counters_linux_pmu_seam_test` |
| FR-018 | `linux_pmu/provider.cpp` | `counters_linux_pmu_seam_test` |
| FR-019 | `linux_pmu/provider.cpp` | `counters_linux_pmu_seam_test` |
| FR-020 | `linux_pmu/table_parse.cpp`, `linux_pmu/provider.cpp` | `counters_linux_pmu_seam_test` |
| FR-021 | `counters_core.hpp` | `counters_pmu_test` |
| FR-022 | `linux_pmu/provider.cpp` | `counters_pmu_test` |
| FR-023 | `cmake/EmbedPmuEvents.cmake`, `linux_pmu/table_parse.cpp` | `test/consumer/main.cpp` |
| FR-024 | `counters_core.hpp`, `CMakeLists.txt` | the installed package's version |
| FR-025 | `plan.cpp` | `counters_overhead` |
| FR-026 | `plan.cpp`, `counters_overhead.cpp` | `counters_overhead` |
| FR-027 | `plan.cpp` | the coverage gate |
| FR-028 | `counters_clock.hpp` | `counters_clock_raw_test` |
| FR-029 | `counters_clock.hpp`, `test/counters_tsc_read_shape.sh` | `counters_clock_raw_test`, `counters_tsc_read_shape` |
| FR-030 | `counters_clock.hpp` | `counters_clock_raw_test` |
| FR-031 | `counters_clock.hpp` | `counters_tsc_read_shape` |
| FR-032 | `test/source/` | every corrected defect's failing test |
| FR-033 | `plan.md` | this document, TDD mode recorded above |
| FR-034 | `test/source/counters_linux_pmu_seam_test.cpp` | every fixture test in this feature |
| FR-035 | `source/counters/` | no change; a check over the library sources |
| FR-036 | `include/speedgun-ng/` | `counters_header_purity` |
| FR-037 | the three changed public headers | `dbc-gate` |
| FR-038 | `specs/007-counters-and-timers/citations-log.md` | ten recorded entries |
| FR-039 | `plan.md`, `research.md` | this document and the Phase 0 record |
| FR-040 | `source/counters/` | the coverage gate |
| FR-041 | `source/counters/` | the clang-tidy report per unit |
| FR-042 | every changed file | this matrix |
| FR-043 | no file changes; a check over `include/speedgun-ng/` | `counters_header_purity` plus a symbol scan |
| FR-044 | every added Markdown file | `prose-lint`, `spell-check` |
| FR-045 | `linux_pmu/fast_read.cpp` | `counters_linux_pmu_seam_test`, the `consumer-release` archive symbol check |
| FR-046 | `detail/pmu.hpp`, `linux_pmu/group_io.cpp`, `linux_pmu/provider.cpp` | `counters_linux_pmu_seam_test` |

### Test Plan

Every check runs unprivileged at `perf_event_paranoid` 2, and every test
that needs kernel behaviour supplies a synthetic sysfs, table, or
event-page input (FR-034).

| Requirement group | Method | Command |
| --- | --- | --- |
| FR-002 to FR-006 | Drive `fast_decode` and the window read paths over synthetic event pages: capability clear, read refused, sequence moved, count at the width boundary, multiplexed window, group short by bytes. Compare with the two recipes the kernel's own interface header documents: the `pmc_width` sign-extension pair, and the seqlock read sequence. The requirement names each recipe by content, and no line number in any kernel header is part of it. | `ctest --test-dir build/dev -R counters_linux_pmu_seam_test` |
| FR-007 | Read a recorded row and assert the disclosure column sits beside the count and the ratio pair, and that a failed action carries the state beside a zero count. | `ctest --test-dir build/dev -R counters_recorder_test` |
| FR-008 | Build the release preset on a pinned processor and take the median over 64 repeats of 1000 actions, at `6aafd2d` and again after the correction, under one run method. | `./build/test/counters_overhead` |
| FR-010, FR-011 | Resolve and compile concurrently from several threads after open, under the thread sanitizer. | `ctest --test-dir build/tsan -R counters_recorder_test` |
| FR-013 to FR-015 | Open and destroy a fast-mode plan 10,000 times plus one partial open, over descriptors and mappings the test opened, and compare `/proc/self/fd` and `/proc/self/maps`. A second test measures real event descriptors where the kernel grants the event and exits 2 elsewhere. | `ctest --test-dir build/dev -R counters_linux_pmu_seam_test` |
| FR-016 to FR-020 | Count the encodable rows the pinned tree yields per Intel directory against a named synthetic format list, and drive synthetic hybrid and uncore device fixtures. | `ctest --test-dir build/dev -R counters_linux_pmu_seam_test` |
| FR-021, FR-022 | Assert a device-scoped entry publishes a state separable from an encoding refusal, a cpu-target plan over it compiles, and the bitmask allocates nothing. | `ctest --test-dir build/dev -R counters_pmu_test` |
| FR-023 | Install into a scratch prefix, run the consumer, and compare the published row count with the build tree's on the same host. Relocate the prefix and repeat. | `./build-consumer/consumer` |
| FR-024 | Read the installed package's version. | `cmake --install build --prefix prefix` |
| FR-025, FR-026 | Drive the plan's own calibration from a registered test and compare the published floor with the bracket measured under the identical bracketing. | `ctest --test-dir build/dev -R counters_overhead` |
| FR-028 to FR-030 | Sample each leaf and assert its documented guarantee, including a new thread's per-thread CPU clock sample falling below an earlier sample from another thread. | `ctest --test-dir build/dev -R counters_clock_raw_test` |
| FR-031 | Assert the timestamp-counter leaf reads with no ordering fence. | `ctest --test-dir build/dev -R counters_tsc_read_shape` |
| FR-037 to FR-045 | Run each gate and check each recorded figure. | `dbc-gate`, `format-check`, `spell-check`, `prose-lint`, `coverage`, `counters_header_purity` |

The full run guide, with every command and its expected outcome, is
[quickstart.md](quickstart.md). The task artifact orders each test task
before the code task it gates.

## Complexity Tracking

> **Fill ONLY if Constitution Check has violations that must be
> justified**

No violation is recorded. Every changed line traces to a requirement in
`specs/012-counters-defect-resolution/spec.md`, no diagnostic
suppression is added, no coverage exclusion is added, and no build
option is introduced. The table below stays empty.

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| none | none | none |
