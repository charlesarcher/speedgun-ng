# Implementation Plan: Counters Defect Follow-Up

**Feature**: `013-counters-defect-followup` | **Date**: 2026-10-06 |
**Spec**: `specs/013-counters-defect-followup/spec.md` |
**Audit point**: `0dea082c825c52349598ae4ebc147b9e0ad7644e` (2026-10-05
19:34:51 -0500), the head of `origin/master` when this plan was written,
and the pre-fix head for every test in this feature

**Input**: `specs/013-counters-defect-followup/spec.md`, the successor of
`specs/012-counters-defect-resolution/`, whose merged corrections a
verification pass found incomplete

## Summary

Ten defects and five new ones sit in the shipped counters library at the
audit point. Five hand a caller a wrong value with no disclosure: a fold
subtracts across a gap point (I-05), only the last provider writes the
disclosure column (F-01), a fold result carries no gap state (F-02), the
fast pair omits the kernel's time extrapolation (I-04(d)), and rows with
a register filter count a different event (I-01). Two 012 tests pass on
the defects they guard (F-05), so continuous integration is no gate over
any of it. Four make a countable event unusable or mislabel its
availability (I-04(a), I-02, I-03, and I-01's second defect). Four cover
the package, the version, and continuous integration (F-03, F-04, I-07,
and the F-05 rewrite).

The substrate already carries each mechanism the correction needs. The
disclosure column exists and the clock window already writes it
unconditionally. `pmu_compose_config` already refuses a field the device
publishes no format for. `entry_read_selection_for` already takes a read
mode per entry. The per-task probe already answers the scope question.
Every correction changes a site or a condition. None introduces a new
mechanism, and none adds an event source, an object kind, or a
provider.

The public surface changes twice: `metric_result` and `points_view` gain
an availability field, and the version lineage records the two
declarations commit `cd5cbd1` removed. The release is 0.4.0 with the
shared-object version on the major position of the 0.x line. The
camelCase rename lands next, so no declaration is restored.

## Technical Context

**Language**: C++23 | **Build**: CMake, presets `dev`, `ci-ubuntu`,
`ci-sanitize`, `ci-tsan`, `ci-coverage`, `ci-audit` | **Platform**: Linux
only, GCC and Clang | **License**: BSD 3-Clause

**Dependencies**: none added. The library keeps zero external runtime
dependencies. The vendored event tables under `external/pmu-events` stay
pinned to the recorded kernel tag and this feature re-pins nothing.

**Authority outside this repository**: the kernel's interface header
`include/uapi/linux/perf_event.h`, which carries the enabled and running
time computation, the short-counter form, and the fast-read capability
bits; and the kernel's own table generator
`tools/perf/pmu-events/jevents.py`, which carries the register-index map
and the key-to-term map. Both were read at kernel commit
`eaab2eb09dc2f86f41e8fa55243c31a274978233`. Neither is vendored here, so
neither is a build input; both are reference documents, and D-05 records
the commit so a later re-read can diff against it.

**Reference host**: AMD Ryzen 9 9950X3D, Linux 7.2.4-1-cachyos, glibc
2.44, `perf_event_paranoid` 2. Every figure this plan publishes is
measured on it.

**Unknowns**: none. The three questions the specification carried were
answered on 2026-10-06 and are resolved under D-02, D-05, and D-11. The
one figure this feature cannot measure on the reference host is the
runtime availability of `offcore_rsp`. It depends on the running kernel's
format list. The pinned tree fixes nothing here. Spec 012's FR-039 keeps
the Intel confirmation deferred, and the verification matrix records the
dependency.

**Constitution**: `.specify/memory/constitution.md`, the highest
authority in this repository, which outranks this plan

## Constitution Check

### Pre-design gates

| Principle | Gate | Status |
| --- | --- | --- |
| II | Design By Contract on every changed interface | PASS. Each changed interface names its contract and its enforcement in `contracts/`, and no contract arrives without its `SG_REQUIRE` or `SG_ENSURE`. |
| III | Design By Contract and TDD recorded | PASS. TDD mode is recorded below, and every requirement names the test that fails first. |
| IV | Documentation accuracy | PASS. Every file:line citation was read at the audit point, and D-05 records the kernel commit its claims rest on. |
| V | Code style | PASS. No new style rule is introduced, so the existing `cmake-format.sh` and `format-check` gates hold unchanged. |
| VI | Zero defect tolerance, and a wrong benchmark is worse than no benchmark | PASS, and this is the feature's reason to exist. Every correction ships with a test that fails at the audit point. |
| VII | Benchmark performance discipline | PASS. The sampling path gains no work: D-01, D-02, D-03, and D-10 all land on the fold and the compile. `quickstart.md` gates the release-build median against the pre-fix median within five percent. |
| VIII | Hard gates | PASS. No gate is relaxed, skipped, or narrowed. `ci-audit` gains a consumer-count step rather than losing one. |
| IX | Spec-driven development | PASS. `spec.md` precedes this plan, and `/speckit-tasks` precedes any code. |
| X | Anti-slop discipline | PASS. Every correction is a change at a cited site with a failing test and a commit that names one defect. |
| XI | Prose standards | PASS. The repository prose gate reports zero findings over every artifact this feature adds. |

### Gates this feature changes

None. No gate is relaxed to land this feature. `ci-audit` gains a step,
and the coverage gate holds at 100 percent line, branch, and contract
coverage, which is why D-01, D-02, D-03, D-07, D-09, and D-10 add
branches and each needs its own test. No exclusion marker is added here.
The pre-existing clang-tidy and coverage-exclusion backlog stays out of
scope except on the lines this feature touches.

### Principle VII baseline infrastructure

No baseline infrastructure is added. The counters library already
publishes its own overhead figures in `docs/pages/counters-overhead.md`
and already gates its release-build median against a committed figure,
and this feature re-measures that same figure. No new harness is built.
The overhead page is updated with the figures each correction changes,
because a correction that moves the sampling cost and leaves the page stale
is a wrong document under Principle IV.

### TDD mode

The plan runs in TDD mode under Principle III. The audit point is the
pre-fix head for every test in this feature, and each test is written
before the correction it guards, observed to fail at the audit point,
and observed to pass after the fix. `quickstart.md` gives the command
that observes each failure before any correction lands, and the
verification matrix below names that test for every requirement. No
correction lands in a commit that does not carry its failing test, so
each commit is bisectable and each revert is a defect that CI catches.

### Version lineage

The version lineage records this feature under 0.4.0:

- `metric_result` gains an availability field and `points_view` gains the
  same field. Additive, under the spec 008 precedent for a field added to
  a public record.
- `operator*(const expression&, const double)` stays removed. Commit
  `cd5cbd1` removed it and spec 012's FR-024 recorded no removal, so this
  version records the removal. The declaration stays gone.
- The project version moves from 0.3.0 to 0.4.0.
- The package config compatibility moves from the major-version
  compatibility to the minor-version compatibility while the project
  major version stays 0.
- The shared-object version is stated as 1. Taking it from the major
  position yields 0 on this line, so every 0.x release would claim the
  same shared-object version and a consumer could not tell one from
  another. The value is dormant while `BUILD_SHARED_LIBS` is forced off
  at `CMakeLists.txt:137` and `:232`, and it becomes live the moment that
  is turned on.
- Commit `cd5cbd1` removed `operator*(const expression&, const double)`
  and the `concept provider` declaration. Both stay removed, and this
  version records both removals.

Neither declaration is restored. The camelCase rename lands next and
renames the whole surface, so a restored declaration would be renamed
away before any caller could use it.

### Deferred confirmation

Spec 012 deferred the Intel host confirmation and this feature defers it
again. Every test here runs on synthetic sysfs, table, and event-page
inputs, so no correction needs an Intel host or a privileged event. The
one figure that depends on the running kernel is the availability of
`offcore_rsp`, `ldlat`, and `frontend` in a device's own format list;
D-07 makes that dependency explicit, and the seam test pins the count
against a synthetic format list so the parser is exercised either way.

The host class that would settle it: any x86 host whose running kernel
publishes `offcore_rsp`, `ldlat`, and `frontend` in a core device's own
`format` list. Every Intel part from Nehalem on publishes `offcore_rsp`,
so a host bearing any of them settles the figure, and no AMD host settles
it at all.

### Post-design gates

| Principle | Gate | Status |
| --- | --- | --- |
| I | Standard-first coding, the P0 to P3 model | PASS. No new library and no new dependency; every correction uses the standard library and the substrate 012 already built. |
| II | Contracts paired with enforcement | PASS. `contracts/gap-state.md`, `contracts/register-filter.md`, and `contracts/device-routing.md` each name the enforcement beside the contract. |
| III | TDD mode and the failing test first | PASS. Recorded above, and the verification matrix names the failing test per requirement. |
| V | Style, naming, and formatting | PASS. The one public name this feature adds, `availability` on a public record, matches the existing enumeration's name. |
| VI | Zero defect tolerance | PASS. Fourteen defects close, each with a test that fails at the audit point. |
| VII | Performance discipline | PASS. The sampling path gains no branch, and the overhead page is re-measured. |
| VIII | Hard gates green | PASS. `ctest`, `dbc-gate`, `format-check`, `spell-check`, `prose-lint`, both sanitizer presets, the coverage gates, and clang-tidy with no new warning. |
| X | Anti-slop discipline | PASS. Fourteen commits, one defect each, no drive-by edit. |
| XI | Prose standards | PASS. Zero findings from the repository prose gate over every artifact. |

**Gate verdict**: PASS. No violation is justified by an "unless" clause,
and no complexity tracking entry is warranted.

## Project Structure

### Documentation (this feature)

```
specs/013-counters-defect-followup/
├── spec.md                       # the specification, with its quality checklist
├── plan.md                       # this file
├── research.md                   # Phase 0: fourteen decisions, fourteen verdicts
├── data-model.md                 # entities, states, and the figures the corrections move
├── quickstart.md                 # the commands that observe each defect and each fix
├── contracts/
│   ├── gap-state.md              # the availability field, per-window disclosure, and the fold's reads
│   ├── register-filter.md        # the register index map, the paired index, and the no-obligation keys
│   └── device-routing.md         # per-device fast verdicts, uncore instance routing, and per-task scope
└── checklists/
    └── requirements.md           # specification quality, every item passing
```

### Source Code (repository root)

```
include/speedgun-ng/
├── counters_core.hpp             # the availability enumeration and metric_result gain the field
├── counters_measurement.hpp      # points_view gains the field; the version note names both removals
└── counters_provider.hpp         # leaf_set gains a per-window disclosure column

source/counters/
├── plan.cpp                      # names a disclosure column per window, not one for the plan
├── fold.cpp                      # reads the mark at both end points; carries the state into the result
├── clock_provider.cpp            # unchanged: it already writes its mark unconditionally
└── linux_pmu/
    ├── table_parse.cpp           # the register-index map, the no-obligation keys, and the new terms
    ├── provider.cpp              # per-device fast verdict, uncore instance routing, per-task scope
    ├── fast_read.cpp             # the pair follows the header recipe, short-counter form included
    ├── group_io.cpp              # reads the mark per action; a fast window narrows per device
    └── encode.cpp                # unchanged: it already refuses an unpublished format

test/source/
├── counters_linux_pmu_seam_test.cpp   # the register filter, the pair recipe, two new fixture formats, and the encodable counts
├── counters_recorder_test.cpp         # a gap after non-zero counts, read through the public surface
├── counters_pmu_test.cpp              # per-device fast verdicts and uncore instance routing
├── counters_objects_test.cpp          # per-window disclosure under both registration orders
└── counters_overhead.cpp              # the release-build median this feature gates

test/consumer/main.cpp              # prints the count the continuous-integration step compares
.github/workflows/ci.yml            # the consumer-count step in the audit job
cmake/install-rules.cmake           # the minor-version compatibility
CMakeLists.txt                      # the project version and the shared-object version
docs/pages/counters-overhead.md     # the figures each correction changes
specs/007-counters-and-timers/citations-log.md   # one entry per correction
```

## Design

### Logical view: the gap mark is a per-action fact

The substrate already carries the mark. `group_io.cpp:329-332` writes
`gap` or `countable` into the disclosure column for every action whose
group read was short, and `clock_provider.cpp:236-238` writes
`countable` on every action for every clock leaf. The defect is where the
column is named and where it is read.

`plan.cpp:542-552` names one disclosure column, on the window that owns
the plan's last managed leaf. `group_io.cpp:714-716` returns early for
every window that names none, so every other window writes no mark. Fix
it by naming a column per window: each window that owns at least one leaf
gets its own disclosure column, and each writes its mark in the pass that
already writes its counts. `point_sink::put` keeps its one-integer
signature. The extra state is another column, and the call stays one
integer wide.

Read it at both ends. `fold.cpp:45` subtracts raw counts, and the counts
are cumulative, so a gap strictly inside a window leaves both end points
measured and the delta correct. A window whose end point carries the mark
subtracts a real count from zero; a window whose start point carries it
reports the whole cumulative count as its delta. `leaf_ratio` at
`fold.cpp:116-126` reads the mark at the end point alone and falls back
to 1.0 at `fold.cpp:150-154` and `fold.cpp:180-182`, so a gapped window
reports the running ratio of a clean full-rate window. Two reads fix it:
the fold reads the mark at `ctx.i` and at `ctx.j`, and a mark at either
end suppresses the value and publishes the state instead.

The cost is two reads of a column the fold already has in cache, once
per figure, never on the sampling path.

### Logical view: the state travels as an availability value

The disclosure column carries an `availability`, and
`counters_core.hpp:87-95` already separates `countable`,
`permission_blocked`, `not_encodable`, `absent`, `scope_refused`, and
`gap`. A fold that publishes the column's value publishes the reason. The
reason is more than the presence.

`metric_result` at `counters_core.hpp:265-270` gains the field, and
`points_view` at `counters_measurement.hpp:391-401` gains the same field.
The enumeration gains no value: `gap` already says what the fold needs
to say, and a fold that suppresses its value needs no new state machine.
The field is additive, so no stored bit moves and no existing reader
breaks.

This removes the hard-coded column index at
`counters_recorder_test.cpp:247`, which is the only place outside the
library that knows the layout.

### Logical view: the pair follows the header recipe

`fast_read.cpp:353-389` copies `time_enabled` and `time_running` and
hands them over. The header's `cap_user_time` recipe takes the page's
cycle counter through `time_shift`, subtracts `time_offset`, applies
`time_mult`, and adds the result to the enabled pair, and to the running
pair where the page index is non-zero. `cap_usr_time_short` corrects the
cycle value by `time_cycles` and `time_mask` first.

Every one of those values is read inside the loop that already runs under
the sequence, so the correction costs no additional synchronization. The
alternative, correcting in the fold, is impossible: the fold receives two
`std::uint64_t` values and has no access to the page's scale fields.

The existing seam test cannot see any of this, because its fixture has no
field for any of the six values. F-05's rewrite adds them.

### Logical view: the register filter reaches the catalog

Two defects share this path. One drops `MSRValue` and `MSRIndex` with no
obligation at `table_parse.cpp:157-158`, so 570 rows across the four Intel
directories publish as countable with no filter and count a different
event. The other omits `Counter` and `Deprecated` from the no-obligation
list at `table_parse.cpp:155-163`, so 466 rows carrying a
counter-constraint value and 42 carrying a deprecation value become
required fields for a format the kernel publishes under no name, and
every one of them turns `not_encodable`.

D-05 resolves the first through the kernel's own map. `jevents.py:248-258`
names the format for each index and takes the first index of a pair. The
parser grows the same map and the same first-index rule, and drops both
keys from the encoding path. D-07 then needs no new mechanism:
`encode.cpp:132-159` already refuses a field the device publishes no
format for, so a row whose resolved format is absent publishes
`not_encodable` on its own.

One consequence lands in the test. The parser takes none on. The seam
test's synthetic device publishes `offcore_rsp` and neither `ldlat` nor
`frontend`, so under D-05 the 122 rows naming those two formats would flip
to `not_encodable` against it where they are countable today. The fixture
grows both, at the ranges the kernel publishes at
`arch/x86/events/intel/core.c` lines 6606 and 6608: `config1:0-15` and
`config1:0-23`. The encodable-row count itself does not move under D-05,
because the parser drops the register keys today and such a row already
encodes on its base fields alone. D-05 changes what those rows count, not
how many encode.

The kernel's `msrmap[...]` at `jevents.py:258` has no default arm and
raises on an unmapped index. A library cannot abort a process over one
table row, so the parser returns no format and the row publishes
`not_encodable`. Every index in the pinned tree is mapped, so this arm
covers a future table. The present pinned tree hits no such arm.

### Logical view: one verdict per device

`fast_read.cpp:203-229` probes one core hardware event.
`provider.cpp:690-691` reads that one verdict, `provider.cpp:733` passes
it to `probe_device` for every device, and `entry_read_selection_for` at
`provider.cpp:411` sets the fast mode on every countable entry.
`group_io.cpp:745-780` then opens a fast window whenever every leaf
carries it, reaching the group read only through three narrowing arms
that never consult the device's own page.

An uncore or RAPL page publishes no `cap_user_rdpmc`, so every fast read
of such an entry is refused at sampling time and every sampling action
discloses a gap. The verdict moves into `probe_device`, where the device's
own page is available. The refusal at sampling time stays exactly as it
is, because spec 012's FR-002 and D-03 hold and this correction adds no
sampling-path work.

`scope_reaches` at `provider.cpp:367-397` takes the same shape: it
compares a unit against a device name by exact text after case folding,
so `DFPMC` reaches no `amd_df` and `CHA` reaches no `uncore_cha_0`. The
kernel's generator maps a unit to a device name and ignores a numeric
instance suffix, and the parser takes the same map and the same suffix
rule. 48 distinct units are in the tree and the largest unreachable ones
are `CHA` on 7351 rows and `iMC` on 2785.

Device scope is the third correction on this path.
`provider.cpp:290-291` marks every device outside the three core names as
device-scoped by name, and `provider.cpp:175-180` then skips the
per-task probe and publishes `scope_refused`. The `msr` PMU registers a
per-task context and accepts per-task events. The name rule goes; the
probe verdict stays, and it is the evidence the catalog publishes.

### Logical view: the package, the version, and the gate

`cmake/install-rules.cmake:39` writes `SameMajorVersion`, and on a 0.x
line that accepts every later minor release. The minor-version
compatibility is the right reading while the major version is 0. The
project version moves to 0.4.0 and the shared-object version takes the
major position.

The two declarations `cd5cbd1` removed stay removed, and the version
lineage records both with this bump. Restoring them would be renamed away
by the camelCase rename that lands next.

`test/consumer/main.cpp:47-59` prints the catalog entry count and
`ci.yml:394-395` compares it with nothing. One step in the audit job runs
the consumer and one compares its count with the build tree's count on the
same runner. A runner with no device publishes no entry on either side,
so both counts are zero and the step holds without a special case.

### Physical view: change per file

| File | Change | Requirements |
| --- | --- | --- |
| `include/speedgun-ng/counters_core.hpp` | `metric_result` gains an `availability` field; its doxygen names the state it carries | FR-004 |
| `include/speedgun-ng/counters_measurement.hpp` | `points_view` gains the same field; the version note names both removed declarations | FR-004, FR-020 |
| `include/speedgun-ng/counters_provider.hpp` | `leaf_set` carries a disclosure column per window rather than one for the plan | FR-006, FR-007 |
| `source/counters/plan.cpp` | name a disclosure column per window; the column no longer depends on registration order | FR-006 |
| `source/counters/fold.cpp` | read the mark at both end points; suppress the value and publish the state | FR-001, FR-002, FR-004 |
| `source/counters/linux_pmu/table_parse.cpp` | the register-index map and the first-index rule; `Counter` and `Deprecated` join the no-obligation list; `any`, `ch_mask`, and `fc_mask` reach their formats | FR-010, FR-011, FR-012, FR-013, FR-016 |
| `source/counters/linux_pmu/fast_read.cpp` | the pair takes the cycle, scale, offset, multiplier, and short-counter fields inside the sequence | FR-007, FR-008, FR-009 |
| `source/counters/linux_pmu/provider.cpp` | the fast verdict is per device; the uncore unit map and the instance suffix; scope from the per-task probe | FR-014, FR-015, FR-016, FR-018 |
| `source/counters/linux_pmu/group_io.cpp` | the mark is read per action; a fast window narrows per device | FR-006, FR-017, FR-019 |
| `cmake/install-rules.cmake` | the minor-version compatibility | FR-021 |
| `CMakeLists.txt` | the project version 0.4.0 and the shared-object version on the major position | FR-021 |
| `.github/workflows/ci.yml` | the consumer-count step in the audit job | FR-022 |
| `docs/pages/counters-overhead.md` | the figures each correction changes | FR-024, SC-013 |
| `specs/007-counters-and-timers/citations-log.md` | one entry per correction, against the requirement it restores | FR-030 |

### Verification Matrix

Every requirement names the test that fails at the audit point and passes
after the fix. All of them run unprivileged at `perf_event_paranoid` 2 on
the continuous-integration matrix, and every one that needs kernel
behaviour uses synthetic sysfs, table, and event-page inputs.

| Requirement | Failing test at the audit point | Success criterion |
| --- | --- | --- |
| FR-001, FR-002 | `counters_recorder_test`, the gap fixture driven after non-zero counts | SC-001 |
| FR-003 | `counters_recorder_test`, a window with the mark at its start point | SC-001 |
| FR-004, FR-005 | `counters_recorder_test`, a caller reading the state with no column index | SC-003 |
| FR-006 | `counters_objects_test`, a plan mixing PMU and clock leaves under both registration orders | SC-002 |
| FR-007, FR-008, FR-009 | `counters_linux_pmu_seam_test`, the multiplex fixture with the scale fields and the short-counter fixture | SC-004 |
| FR-010 | `counters_linux_pmu_seam_test`, the register-filter fixture over the pinned tree, with `ldlat` and `frontend` added to the synthetic format list | SC-005 |
| FR-011 | `counters_linux_pmu_seam_test`, a row carrying the any-thread, port-mask, and function-call-mask keys | SC-005 |
| FR-012, FR-013 | `counters_linux_pmu_seam_test`, the recounted encodable rows per directory against 581, 346, 564, and 2141 | SC-006 |
| FR-014 | `counters_pmu_test`, the uncore instance fixtures | SC-008 |
| FR-015 | `counters_pmu_test`, the vendor-named AMD device fixtures | SC-008 |
| FR-016 | `counters_pmu_test`, the per-task probe fixture on a model-specific-register device | SC-009 |
| FR-017 | `counters_pmu_test`, a fast-capable host with a core and an uncore device | SC-007 |
| FR-018 | `counters_pmu_test`, a device whose page refuses the fast read | SC-007 |
| FR-019 | `counters_linux_pmu_seam_test`, the refused fast read disclosing a gap and issuing no syscall read | SC-007 |
| FR-020, FR-021 | `counters_header_purity`, the install rules check, and the version lineage entry | SC-011 |
| FR-022 | the audit job's consumer-count step | SC-012 |
| FR-023 | `counters_linux_pmu_seam_test` and `counters_recorder_test`, each failing at the audit point | SC-010 |
| FR-024 | `counters_noalloc_test`, `counters_overhead`, and the header purity scan | SC-013 |
| FR-025 | the repository gates, each recorded in `quickstart.md`, plus each correction's own failing test | SC-013 |
| FR-026 | every counters test in the matrix job, all unprivileged at `perf_event_paranoid` 2, with synthetic sysfs, table, and event-page inputs | SC-013 |
| FR-027 | `counters_noalloc_test` and the release-build median in `counters_overhead` against the pre-fix median | SC-013 |
| FR-028 | `counters_noalloc_test` and the standalone example targets, with no benchmark-framework symbol in the library | SC-013 |
| FR-029 | `counters_header_purity` and the `dbc-gate` target | SC-013 |
| FR-030 | the successor-log entry beside each correction, checked in review | SC-013 |
| FR-031 | the precondition table in `spec.md`, which records every issue's reproduction | SC-013 |
| FR-032 | the version lineage entry beside the version bump | SC-011 |
| FR-033 | the coverage gate at 100 percent line, branch, and contract, and clang-tidy on each touched unit | SC-013 |
| FR-034 | the repository prose gate over every artifact this feature adds | SC-013 |
| FR-035 | the unchanged `point_sink::put` signature, the unchanged availability enumeration beside the target bitmask, and the unchanged refused-fast-read gap | SC-007 |

### Test Plan

TDD mode. The audit point is the pre-fix head, so every failing test is
observed before its correction lands. `quickstart.md` carries the command
for each row, and the order is the order of the priority model: the five
wrong-value defects first, then the four that make an event unusable or
mislabel it, then the package and gate work.

The two rewritten 012 tests are the gate that was missing. Both pass at
the audit point and both fail once rewritten, which is the state F-05
names and the state SC-010 forbids.

The Intel host confirmation stays deferred. Every row above runs on
synthetic inputs, so the feature lands without it and the deferred
confirmation stays deferred.

## Complexity Tracking

No complexity tracking entry is warranted. Every correction lands at a
cited site, uses a mechanism the substrate already carries, and adds one
mechanism where none existed: the per-window disclosure column. Nothing
here is left unresolved on purpose, and no NEEDS CLARIFICATION marker
remains.
