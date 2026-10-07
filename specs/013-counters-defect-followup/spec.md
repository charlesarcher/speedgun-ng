# Feature Specification: Counters Defect Followup

**Feature Branch**: `013-counters-defect-followup`

**Created**: 2026-10-05

**Status**: Draft

**Input**: User description, held at `speedgun-ng-013-specify-prompt.md`
, quoted verbatim:
<!-- prose-lint: allow reason="XI.5 verbatim quotation of the request, marked as quoted" -->
"Counters defect resolution follow-up, the last counters correction
before the benchmark harness. Spec 012 corrected ten defects that an
audit of the counters library found. A verification of the merged code
at 0dea082 found five of those corrections incomplete. It also found
five new defects in the 012 changes. Some of these defects hand a caller
a wrong count with no disclosure. Others turn a countable event into a
permanent gap. This feature closes them before the harness reports a PMU
figure. Principle VI states that a wrong benchmark is worse than no
benchmark."

The rest of that document holds the thirteen issues with their source
sites, the priorities, the scope, the constraints, the success criteria,
and four open questions. It stays the audit record this specification
answers.

## Audit point

| Field | Value |
| --- | --- |
| Audit point | `0dea082c825c52349598ae4ebc147b9e0ad7644e` |
| Audit-point date | 2026-10-05 19:34 |
| Default branch at run time | `master` |
| Feature number at run time | 013, the next free number under `specs/` |
| Short name at run time | `counters-defect-followup` |
| Pre-fix head | the audit point |

Every issue below reproduces at the audit point. Every test this feature
adds fails at the pre-fix head and passes after the fix. The
`file:line` citations throughout this specification were refreshed at the
audit point, and the refreshed values are collected in
`## Verified citations at the audit point`.

## Purpose

The counters library hands the benchmark harness every value it reports.
Each value is a correct count, or the library discloses that no count is
available. A value that is wrong and undisclosed is the failure Principle
VI names.

Five corrections from spec 012 are incomplete, and five new defects sit
in the 012 changes. Five of the thirteen hand a caller a wrong value or
hide a defect from the continuous-integration gate. Those land first.
Five make a countable event unusable or mislabel its availability. The
remaining three cover the package, the version, and the gate itself.

Each issue corrects behaviour the shipped providers already claim. This
feature adds no event source, no object kind, and no provider. Each
correction restores a requirement that spec 007, spec 012, or this
specification states, and each correction lands in the successor log at
`specs/007-counters-and-timers/citations-log.md`.

Eight issues keep their spec 012 identifier. Each identifier from `I-01`
through `I-10` names the defect that identifier carries in
`specs/012-counters-defect-resolution/research.md`. A defect the audit
found in the 012 changes takes a new `F` identifier, running from `F-01`
through `F-05`.

A defect is **confirmed** where the cited lines show it. Every issue in
this specification is confirmed by reading at the audit point. An issue
that no longer reproduces leaves the scope, and the specification records
the evidence (012 FR-039). The `### Precondition results` section records
the reproduction check for each issue.

The camelCase rename of identifiers and the benchmark harness follow this
feature. Both stay out of scope.

## Precondition results

Each precondition was checked at the audit point. The result stands for
the tree at `0dea082`.

### PC-1, spec 012 is complete on the default branch

**Result: holds.** The directory `specs/012-counters-defect-resolution/`
is present in the tree of the default branch, and `tasks.md` at that tree
carries 131 checked task lines and 0 unchecked task lines.

### PC-2, no unmerged camelCase or benchmark-harness work

**Result: holds.** No branch name and no commit subject on any branch
names the camelCase rename or the benchmark harness, so no branch
carries unmerged work for either.

Ten branches hold commits the default branch lacks. Every one of them
diverged before the default branch reached the audit point, and every one
holds counters source and test content the default branch already carries
in a later revision, so none of them holds unmerged work for this
feature. The branches that touch the files this feature touches are
`origin/007-counters-and-timers`, `origin/008-timestamp-counter`,
`origin/010-linux-only`, `origin/011-nanosecond-counter-ssc-mark`,
`origin/coverage-gate-audit`, and `origin/fix-asan-and-timing`. The
remaining four (`origin/005-vendor-hdrhistogram`,
`origin/009-vendor-quill`, `origin/012-counters-defect-resolution`, and
`origin/meta-review-fixes`) touch vendoring, packaging, or prose only.

The two branches that follow this feature do not exist yet. The plan
records their base commit as this feature's tip, so each starts from the
corrections.

### PC-3, each issue reproduces at the audit point

**Result: holds, with four sub-claims corrected.** Every issue
reproduces. Four sub-claims carried by the request do not survive the
audit point, and the specification states each correction where the issue
appears.

| Issue | Reproduces | Correction the audit point carries |
| --- | --- | --- |
| I-01 | yes | no pinned Intel row carries the offcore format as a key, and 260 skylake, 71 icelake, 46 alderlake, and 71 sapphirerapids rows name an offcore index through the paired spelling. The figure 447 counts the parseable rows. |
| I-02 | yes | the function spans `provider.cpp:367-397`, and the table unit is case-folded before the comparison |
| I-03 | yes | none |
| I-04(a) | yes | `pmu_open_window` does select the group read for a device in three named arms, so the correction narrows to the read-mode decision only |
| I-04(d) | yes | a window the kernel scheduled for part of its length reports a ratio below 1.0, and the fold flags it as scaled |
| I-05 | yes | none |
| I-07 | yes | none |
| F-01 | yes | none |
| F-02 | yes | none |
| F-03 | yes | the repository holds no changelog file, and the version lineage lives in the spec 012 artifacts |
| F-04 | yes | none |
| F-05 | yes | the multiplex test sets two page time fields, and sets no field of the extrapolation recipe |

### PC-4, the spec 012 clarification decisions still hold

**Result: holds.** All four decisions the request names are present at
the audit point.

| Decision | Site at the audit point | Result |
| --- | --- | --- |
| 012 D-04, the disclosure travels through the unchanged one-integer `point_sink::put` | `include/speedgun-ng/counters_provider.hpp:212` | holds, one `std::uint64_t` parameter |
| 012 D-10, the availability enumeration carries `scope_refused` and `gap` beside the target bitmask | `include/speedgun-ng/counters_core.hpp:87-95` and `:107-113` | holds |
| 012 D-03, a refused fast read discloses a gap and issues no syscall read of the same event | `source/counters/linux_pmu/group_io.cpp:420-423` and `:470-473` | holds, and the only `::read` in the counters library is at `:285`, inside the other window type |

## Clarifications

Three questions were answered on 2026-10-06. Each answer is recorded
below against the requirements it settles, and the `## Open questions for
clarify` section carries the answers forward.

- **Gap state's public shape (FR-004, FR-005, SC-003)**: the fold result
  carries an availability field, the same enumeration the disclosure
  column carries, so the result names the reason for the gap. The raw
  view carries the same field. No accessor is added, and no boolean is
  added.
- **The removed declarations and the version (FR-020, FR-021,
  SC-011)**: the version lineage records both removals, the release is
  0.4.0, and the 0.x shared-object-version rule is corrected in the same
  change. The camelCase rename lands next and renames the whole surface,
  so restoring two declarations now buys nothing.
- **A row with a register filter (FR-010, FR-016, SC-05)**: the row
  encodes through the format its register index names. A paired index
  publishes under the first index of the pair, and the plan verifies
  that the kernel's own generator takes that rule before the
  implementation follows it. A row whose format the running kernel does
  not publish is `not_encodable`.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Receive a fold whose value no gap point computes (Priority: P1)

A caller folds a recorded window over a counter. One sampling action in
the middle of the window failed to read its group, so the action wrote a
zero count and marked itself. The caller receives a delta. The delta is a
correct count, or the fold discloses the gap and reports no value.

Today a fold that ends at the marked action subtracts a real count from
the zero the action wrote, and the modular delta runs near 2^64. A fold
that starts at the marked action reports the whole cumulative count as
its delta. Neither outcome is disclosed.

**Why this priority**: Every harness benchmark folds over millions of
sampling actions. A single failed read among them corrupts every
per-iteration fold whose start point or end point is that action. The
harness reports the corrupted value with no disclosure, which is the
failure Principle VI names.

**Independent Test**: Drive a recorded window whose counts start at a
value above zero, mark one interior action, and fold each of the three
pairs that touch it. Every fold whose start point or end point is the
marked action reports no value and discloses the gap. Every fold whose
two end points lie on measured actions reports the count.

**Acceptance Scenarios**:

1. **Given** a plan whose counts have advanced above zero, **When** one
   sampling action marks itself a gap, **Then** a fold that ends at that
   action reports no value computed from the zero the action wrote, and
   discloses the gap.
2. **Given** the same record, **When** a fold starts at the marked
   action, **Then** that fold reports no value computed from the zero and
   discloses the gap.
3. **Given** a marked action strictly inside a fold window, **When** the
   caller folds the window, **Then** both end points remain measured and
   the delta stays correct, because the recorded counts are cumulative.

---

### User Story 2 - Read the gap state of any window through the public surface (Priority: P1)

A caller holds a fold result and a raw view of one counter's points. The
caller learns whether the window carried a gap. The caller's code names
no column index and knows nothing about how the recorder lays out its
buffer.

Today only the window that owns the plan's last managed leaf receives the
disclosure column. A plan mixing PMU and clock leaves loses every PMU gap
whenever the clock provider registered after the PMU provider. A fold
over a gapped window reports a running ratio of 1.0, the same figure as a
clean window that ran at full rate. No public accessor names the mark.

**Why this priority**: The harness reports each value beside its gap
state and adds no gapped sample to its statistics. Without a public
shape the harness reads the mark by hard-coded column index, and a change
to the layout silently moves that index. A mix of PMU and clock leaves is
the ordinary case, because every harness benchmark reports real time
beside its PMU metrics.

**Independent Test**: Build a plan that holds both a PMU leaf and a clock
leaf, under each of the two provider registration orders. Drive a PMU
gap. The gap reaches the fold result under each order. A caller reads the
gap state of a fold result and of a raw view through the public surface,
with no column index in the caller's code.

**Acceptance Scenarios**:

1. **Given** a plan holding a PMU leaf and a clock leaf, **When** the
   clock provider registered after the PMU provider, **Then** a driven
   PMU gap reaches the fold result.
2. **Given** the same plan, **When** the PMU provider registered after the
   clock provider, **Then** the same driven gap reaches the fold result.
3. **Given** a fold result and a raw view, **When** a caller reads their
   availability field, **Then** the field holds the value the disclosure
   column holds for the same action, and the caller's source names no
   column index.

---

### User Story 3 - Read the multiplex ratio the kernel recipe produces (Priority: P1)

A caller reads a fast-path window that the kernel multiplexed. The caller
receives the enabled and running pair the kernel's own interface header
defines, including the extrapolation the header names. The ratio the fold
reports matches the kernel's recipe.

Today the fast path copies the two raw page fields and computes nothing
further. No file under `source/counters/` reads the page's scale, offset,
or shift field, and no file applies the extrapolation. A window the
kernel scheduled for part of its length reports a ratio the header's
recipe would improve.

**Why this priority**: Every multiplexed fast-mode metric reports its
running ratio. A ratio no kernel recipe produced is a wrong disclosure
beside a correct count.

**Independent Test**: Drive an event-page fixture whose scale, offset, and
shift fields are set, with the page reporting that enabled time differs
from running time. The disclosed pair matches the header recipe within
one tick. A second fixture sets the short-counter form and reaches the
same result.

**Acceptance Scenarios**:

1. **Given** an event page whose enabled time differs from its running
   time, **When** the page states that the scale, offset, and shift fields
   apply, **Then** the reader computes the cycle counter and those three
   fields inside its sequence loop. The reader adds the computed delta to
   the enabled pair and to the running pair where the page index is
   non-zero.
2. **Given** a page whose hardware clock is narrower than 64 bits and
   which states that the short-counter form applies, **When** the reader
   takes the pair, **Then** the reader corrects the cycle value by the
   page's cycle and mask fields before it computes the delta.
3. **Given** a multiplexed window, **When** the fold reports its running
   ratio, **Then** that ratio matches the kernel recipe within one tick.

---

### User Story 4 - Count an event the table filters through a model-specific register (Priority: P1)

A caller counts an Intel offcore, load-latency, or frontend event. The
library encodes the event with the filter the table records, so the count
the kernel returns belongs to the event the caller asked for.

Today the parser drops the register value and the register index with no
encoding obligation, so a row carrying a non-zero register value encodes
its base event with no filter and publishes as countable. The counts
belong to a different event. The counter-constraint and deprecation keys
carry the same obligation today and the kernel publishes no format of
either name.

**Why this priority**: On an Intel host the harness reports offcore,
load-latency, and frontend metrics. A count that belongs to a different
event is a wrong benchmark with no disclosure.

**Independent Test**: Over the pinned tree, assert that no row with a
non-zero register value publishes as countable without its filter. Pin
the per-directory counts of rows that encode their filter and of rows
that publish as not encodable. Assert that no row fails to encode because
of a counter-constraint or a deprecation key, and that the AMD counts do
not fall.

**Acceptance Scenarios**:

1. **Given** a table row carrying a non-zero register value and a
   register index, **When** the catalog publishes the row, **Then** the
   row encodes through the kernel format that its register index names.
2. **Given** a table row whose register index names a pair, **When** the
   catalog publishes the row, **Then** the row publishes under the first
   index of the pair, matching what the kernel's own generator takes.
3. **Given** a table row whose named format the running kernel does not
   publish, **When** the catalog publishes the row, **Then** the row
   publishes as not encodable, with no encoding attempt.
2. **Given** a table key the kernel's own generator maps to a format, such
   as the any-thread, the port-mask, or the function-call mask key, **When**
   a row carries it, **Then** the key reaches the kernel format the
   generator names.
3. **Given** a table row carrying a counter-constraint key or a
   deprecation key, **When** the row is parsed, **Then** the key carries
   no encoding obligation.

---

### User Story 5 - Count an uncore event on every instance its unit names (Priority: P2)

A caller asks for a memory-bandwidth event on an AMD host or a cache event
on an Intel host. The catalog publishes that event on every device
instance whose name the table's unit identifies, and no uncore event
appears on a core device.

Today the routing compares the table unit against the device name, or
against the vendor prefix followed by the unit, as exact text. The kernel
numbers most uncore instances, and AMD devices take vendor names. The
AMD tables spell the three units in the vendor's own spelling. No AMD
uncore row and no numbered Intel uncore row reaches a device.

**Why this priority**: Uncore metrics, memory bandwidth per package among
them, are unavailable today on the reference host, which is AMD. The event
is countable, and the catalog keeps it out of reach.

**Independent Test**: Build synthetic devices named for a numbered cache
instance, a numbered memory instance, and the three AMD vendor devices.
Each device receives the rows its unit names. No uncore row appears on a
core device.

**Acceptance Scenarios**:

1. **Given** a synthetic device named for the first and second instances
   of a cache unit, **When** the catalog publishes, **Then** each device
   receives the rows its unit names.
2. **Given** the three AMD vendor devices, **When** the catalog
   publishes, **Then** each device receives the rows the kernel's own
   generator maps its unit to.
3. **Given** a core device, **When** the catalog publishes, **Then** no
   uncore row appears on it.

---

### User Story 6 - Count a model-specific register event on a per-thread target (Priority: P2)

A caller measures the aperf, mperf, or timestamp counter entry of the
model-specific-register performance monitoring unit on a per-thread
target. The catalog publishes the thread target bit on that entry.

Today the provider decides an entry is device-scoped by name, marking
every device other than the three core names, and skips the per-thread
probe for those entries. The kernel registers a per-task context for that
unit and accepts per-thread events, so the catalog publishes a scope
refusal on a host that grants the event.

**Why this priority**: A countable per-thread event is published as out of
scope, so a caller cannot reach it.

**Independent Test**: Build a fixture with a model-specific-register
device whose per-thread probe succeeds. Its entries publish the thread
target bit.

**Acceptance Scenarios**:

1. **Given** a device the kernel publishes with a per-task context,
   **When** the per-thread probe over one of its entries succeeds,
   **Then** the entry publishes the thread target bit.
2. **Given** a device for which the probe refuses a per-thread event,
   **When** the catalog publishes, **Then** the scope decision rests on
   data the kernel publishes for that device or on the probe verdict.

---

### User Story 7 - Install the package and take the count the build tree took (Priority: P3)

A caller links the installed package and requests a version. A
continuous-integration step compares the installed consumer's catalog
entry count with the build tree's count on the same runner. A caller who
writes the removed multiplication operator or names the removed provider
concept reads a version note naming each removal.

Today the package config accepts a request for 0.2 on a 0.3 release. Two
public declarations sit removed with no version note. The
continuous-integration step runs the installed consumer and compares its
printed count with nothing.

**Why this priority**: The harness links the installed package and
requests a version, and harness code and user benchmarks write the
multiplication with the scalar on the right. A silent major-version
acceptance and an unrecorded removal reach the harness as a link error or
as a wrong install.

**Independent Test**: Assert the package config reads the minor-version
compatibility while the major version stays 0. Assert the project version
reads 0.4.0 and that the version lineage names each removed declaration
and its bump, with neither declaration restored. Break the consumer's
count and assert the continuous-integration step fails.

**Acceptance Scenarios**:

1. **Given** the major version is 0, **When** the package config is
   written, **Then** it reads the minor-version compatibility.
2. **Given** a caller building against 0.4, **When** the caller writes the
   removed multiplication operator or names the removed provider concept,
   **Then** the version lineage names both removals and the bump that
   records them.
2. **Given** a runner whose build tree publishes a different catalog entry
   count from its installed consumer, **When** the step runs, **Then** it
   exits non-zero.
3. **Given** a runner that publishes no performance monitoring unit
   device, **When** the step runs, **Then** it runs unprivileged and
   holds on that runner.

---

### Edge Cases

- **Gap at the first recorded point**: A fold whose start point is the
  first recorded action and carries the mark reports no value. The
  recorder's arena begins at zero, so the mark's zero and the arena's
  initial zero are equal by coincidence. The disclosure decides, and the
  coincidence never decides alone.
- **Gap strictly inside a window**: Both end points remain measured, and
  the delta stays correct because the recorded counts are cumulative.
- **Gap in one window and no gap in a sibling window**: Each window
  discloses its own state. A gap in one never marks another.
- **A provider that owns no leaf**: The plan lays out one read group per
  provider that owns at least one leaf. A provider that owns none receives
  no group and writes no mark.
- **A plan whose only leaf belongs to the clock provider**: The clock
  window owns the last managed leaf and writes the mark. Every fold over
  that leaf reads it.
- **A plan holding leaves from three providers**: The compile names the
  disclosure column to all three windows, and each window writes its own
  mark on every action.
- **A page that states the scale fields do not apply**: The reader copies
  the raw pair, exactly as the header's own recipe directs.
- **A page whose enabled time equals its running time**: The header's
  recipe reads no cycle counter in that case, and the reader follows it.
- **A page whose hardware clock is wider than 64 bits**: The page states
  no short-counter form, and the reader applies no correction.
- **A register value of zero**: The row encodes its base event with no
  filter, and it publishes as countable. The pinned tree carries no row
  whose register value is present and zero.
- **A register index whose format the running kernel publishes under no
  name**: The row publishes as not encodable, with no encoding attempt.
- **A register index naming a pair**: The row publishes under the first
  index of the pair. A pair names two registers and the row carries one
  value, so the second index publishes nothing.
- **A table key the kernel's generator maps but the running kernel does
  not publish as a format**: The row encodes where the device publishes the
  format and publishes as not encodable where it does not.
- **A counter-constraint key carrying an empty value**: The key carries no
  encoding obligation whatever its value.
- **A unit naming a device the host publishes under a different vendor
  prefix**: The row stays out of the catalog, and no probe runs for it.
- **A unit that already carries its instance suffix**: The row reaches
  the one device that suffix names. The pinned tree holds such rows.
- **A per-thread probe that the host refuses for an uncore device**: The
  entry publishes a scope refusal, and the refusal rests on the probe
  verdict.
- **A device the kernel publishes no per-task context for**: The entry
  publishes a scope refusal.
- **A fast-capable host with a device whose page refuses the fast
  instruction**: The entry on that device takes the group read at compile
  time, and the device's own leaves read through their own mode.
- **A fast window whose layout spans several devices**: The group read
  serves it, and the plan compiles.
- **A member leaf whose encoding needs a second configuration word**: The
  group read serves the window, and the entry encodes through its own
  words.
- **A recording whose enabled time never advances**: The fold reports no
  measured fraction for that window, and the raw view discloses the same
  state.
- **A genuine multiplexed window**: The fold reports a ratio below 1.0 and
  flags the result as scaled. The fallback ratio of 1.0 belongs to the
  windows that measured no fraction alone.
- **A runner at `perf_event_paranoid` 2**: Every test runs unprivileged,
  and a fixture supplies the page, the table, and the device tree.
- **A runner with no performance monitoring unit device at all**: The
  catalog-entry-count step holds on that runner, and every other test runs.
- **A pre-existing finding on a line this feature does not touch**: The
  finding stays out of scope, and the plan records the count it measured
  before and after.

## Requirements *(mandatory)*

The requirement statements below run past the 25-word description limit
that Principle XI.7 sets. Each one names the corrected behaviour, the
defect it restores, and the requirement that governs it, and Principle
III requires that precision in EARS form. Every other section of this
specification meets the 25-word limit.

### Folds and the gap mark (I-05, F-01, F-02)

- **FR-001**: A fold whose start point or whose end point carries the
  gap mark SHALL report no value computed from the zero that mark wrote,
  and SHALL disclose the gap state. A gap strictly inside a window leaves
  both end points measured, and the delta between them stays correct
  because the recorded counts are cumulative. This restores 012 FR-006,
  which marks a failed group read in the disclosure column and writes
  zero counts beside the mark, and it completes the 012 correction of the
  fold side.
- **FR-002**: The disclosure column SHALL name every read group of a
  compiled plan, and every window SHALL write its own mark on every
  sampling action. The current compile names the column to the one window
  that owns the plan's last managed leaf. The clock window always writes
  countable there. A plan that mixes PMU and clock leaves therefore loses
  every PMU gap wherever the clock provider registered after the PMU
  provider. A gap in any window SHALL reach every fold over a leaf of
  that window, for any provider registration order.
- **FR-003**: `point_sink::put` SHALL keep its one-integer signature
  (012 FR-007, 012 D-04), and the disclosure column SHALL remain one
  managed column per sampling action written through it.
- **FR-004**: Every fold result SHALL carry an availability field
  through the public surface, and every raw view SHALL carry the same
  field through the public surface. The field SHALL hold the
  availability value the disclosure column holds for the same sampling
  action, so a gap names its reason and not only its presence.
- **FR-005**: A caller SHALL read the gap state of a fold result and of
  a raw view through that field, with no knowledge of the managed column
  layout, and the caller's source SHALL name no column index. The
  availability enumeration SHALL gain no value to serve this
  requirement. The public surface SHALL
  carry no accessor that requires the layout to interpret.
- **FR-006**: The availability enumeration SHALL carry a value that names
  a gap, beside the value that names a scope refusal, and the fixed-size
  target bitmask SHALL stay beside it (012 FR-021, 012 D-10). This
  feature adds no availability value and moves no stored value.

### Fast-path time computation (I-04(d))

- **FR-007**: The library SHALL compute the enabled and running pair of a
  fast window with the time computation the kernel's own interface header
  documents, including the extrapolation that header names (012 FR-005).
  The requirement names the computation by its content, so no file path
  and no line number in any kernel header is part of it. Where the page
  states that the scale, offset, and shift fields apply, and the enabled
  time differs from the running time, the reader SHALL take the cycle
  counter and those three fields inside the sequence loop. The reader
  SHALL add the computed delta to the enabled pair. The reader SHALL add
  it to the running pair where the page index is non-zero.
- **FR-008**: Where the page states that the hardware clock is narrower
  than 64 bits, the reader SHALL correct the cycle value by the page's
  cycle and mask fields before it computes the delta. The header
  documents that short-counter form as a correction on top of the
  long-counter form.
- **FR-009**: The disclosed running ratio over a multiplexed window SHALL
  match the kernel recipe within one tick. A genuine multiplexed window
  reports a ratio below 1.0 and flags the result as scaled; the fallback
  ratio of 1.0 belongs to a window that measured no fraction alone. The
  012 research decision D-02 stated that the kernel header names no
  scaling rule. That statement is false, and the successor log corrects
  it against this requirement.

### Event table keys and device routing (I-01, I-02, I-03)

- **FR-010**: No row with a non-zero register value SHALL publish as
  countable without its filter. Such a row SHALL encode through the kernel
  format its register index names, and a row whose index names a pair
  SHALL publish under the first index of the pair. A row whose named
  format the running kernel does not publish SHALL publish as not
  encodable. The plan SHALL verify against the kernel's own generator
  that the first index of a pair is the one it takes, before the
  implementation follows that rule.
- **FR-011**: Every encoding key that the kernel's own table generator maps
  to a kernel format SHALL reach the format the generator names. That set
  includes the any-thread, the port-mask, and the function-call-mask keys,
  which the kernel maps to `any`, `ch_mask`, and `fc_mask`, and the
  parser currently maps to no format (012 FR-017).
- **FR-012**: A counter-constraint key and a deprecation key SHALL carry
  no encoding obligation, whatever value they hold. The kernel publishes
  no format of either name (012 FR-016).
- **FR-013**: The catalog SHALL publish the encodable-row count over the
  pinned tree against a synthetic format list, per architecture directory,
  so one number gates every host on the continuous-integration matrix
  (012 FR-020). A fixture SHALL pin the per-directory counts of rows that
  encode their filter and of rows that publish as not encodable. The AMD
  directory counts SHALL NOT fall.
- **FR-014**: Each uncore row SHALL appear on every device instance its
  unit names, through the kernel generator's unit map and its suffix rule.
  The kernel numbers most uncore instances. The AMD tables spell the
  data-fabric, the level-three cache, and the unified-memory unit in the
  vendor's own spelling. The generator maps those three spellings to the
  AMD vendor device names (012 FR-019).
- **FR-015**: No uncore row SHALL appear on a core device, and the core
  routing of 012 FR-019 stays unchanged.
- **FR-016**: Device scope SHALL come from data the kernel publishes for
  each device, or from the probe verdict. A `cpumask` file marks a device
  device-scoped. A `cpus` file does not. `cpu`, `cpu_core`, and `cpu_atom`
  stay per-task capable. An entry on a device whose per-thread probe
  succeeds SHALL publish the thread target bit (012 FR-021, 012 FR-022).

### Fast-read mode per device (I-04(a))

- **FR-017**: Each device SHALL carry its own fast verdict. The current
  probe opens one core hardware event on the host and returns a single
  host-wide verdict. The provider passes that verdict to the probe of
  every device. The read-mode selection then sets the fast mode on every
  countable entry. A fast-mode verdict SHALL NOT be decided for a device
  from a probe taken on another device (012 FR-001).
- **FR-018**: An entry on a device whose page refuses the fast read SHALL
  take the syscall group read at compile time, and a plan that mixes such
  devices SHALL read each device with its own mode. The current compile
  takes the fast window whenever the host verdict holds and every member
  leaf carries the fast mode. That compile narrows per device in three
  arms. A layout spanning several devices narrows. A member leaf whose
  encoding needs a second configuration word narrows. A member whose open
  the kernel refuses narrows. None of those three arms consults the
  device's own page.
- **FR-019**: A refused fast read at sampling time SHALL disclose a gap
  and SHALL issue no syscall read of the same event, so the sampling
  action keeps the cost this library publishes for it (012 FR-002,
  012 D-03). This requirement is unchanged. The correction under FR-018
  applies at compile time and moves no sampling-path cost.

### Package, version, and the continuous-integration gate (F-03, F-04, I-07, F-05)

- **FR-020**: Each public declaration that commit `cd5cbd1` removed SHALL
  have its removal recorded in the version lineage with its version bump.
  Neither declaration SHALL be restored by this feature. The removed
  declarations are the non-member multiplication of an expression by a
  double and the provider concept. The camelCase rename lands next and
  renames the whole surface, so a restoration now would be renamed away.
  The repository holds no changelog file, and the version lineage lives in
  the spec 012 artifacts, where neither removal is named.
- **FR-021**: This feature SHALL ship as 0.4.0, and the package config
  compatibility SHALL read the minor-version compatibility while the
  major version stays 0, because a request for 0.2 currently accepts 0.3
  and the 0.3 release adds availability values and removes the
  declarations FR-020 names. The release takes the minor position because
  the availability field FR-004 adds is additive while the removals
  FR-020 records are breaking. `SOVERSION` is a hand-kept ABI number.
  The maintainer bumps it when a public signature or a public record
  layout changes. A patch that changes neither leaves the number as it
  stands. The plan SHALL record that rule.
- **FR-022**: A continuous-integration step SHALL fail where the
  installed consumer's catalog entry count differs from the build tree's
  count on the same runner (012 SC-008). The step SHALL run unprivileged
  and SHALL hold on a runner that publishes no performance monitoring
  unit device.
- **FR-023**: Every test this feature rewrites SHALL fail at the pre-fix
  head and SHALL pass after the fix (Principle VI, Principle X.4). The
  rewritten multiplex-window test SHALL set the page's scale, offset, and
  shift fields, and a second fixture SHALL set the short-counter form, so
  the first fails while the extrapolation is absent. The rewritten
  recorder gap test SHALL drive cumulative counts above zero before the
  gap point. The zero the gap writes then differs from the first real
  point, so the test fails while a fold subtracts across the gap.
- **FR-024**: This feature SHALL add no event source, no object kind, and
  no provider. It corrects behaviour the shipped providers already claim
  (012 FR-043).

### Verification, traceability, and repository constraints

- **FR-025**: Every fix in this feature SHALL ship with a test that fails
  at the pre-fix head and passes after the fix, and the plan SHALL record
  TDD mode (Principle III, 012 FR-032, 012 FR-033).
- **FR-026**: Every test in this feature SHALL run unprivileged on the
  continuous-integration matrix at `perf_event_paranoid` 2. Where the
  kernel is involved, the test SHALL use synthetic device-tree, table, and
  event-page inputs (012 FR-034).
- **FR-027**: `recorder::sample()` SHALL remain `noexcept`,
  allocation-free, and lock-free (007 FR-026, 012 FR-008). Its
  release-build median on the reference host SHALL stay within 5 percent
  of the median measured at the pre-fix head. That median is the median
  over 64 repeats of 1000 actions at a fixed iteration count on a pinned
  processor. That bound SHALL be a recorded measurement with no test
  asserting it. The measurement SHALL cover the changes FR-002 and FR-017
  make to the compile and sampling paths.
- **FR-028**: The counters classes SHALL stay embeddable in
  fixed-iteration, per-thread benchmark loops, and no
  benchmarking-framework code SHALL enter the library (007 FR-049,
  007 FR-050, 012 FR-035).
- **FR-029**: No public header SHALL gain a platform term, verified by
  `test/counters_header_purity.sh` (012 FR-036). Each changed interface
  SHALL keep its doxygen contract paired with a registered enforcement
  counterpart, and the `dbc-gate` target SHALL prove the pairing
  (Principle II, 012 FR-037).
- **FR-030**: Every correction in this feature SHALL be recorded in
  `specs/007-counters-and-timers/citations-log.md` in that file's entry
  format, against the 007 or 012 requirement it restores. The frozen
  record at `specs/007-counters-and-timers/citations.md` takes no edit
  (012 FR-038).
- **FR-031**: An issue that no longer reproduces at the audit point SHALL
  leave the scope, and the specification SHALL record the evidence. Every
  issue in this feature reproduces, and the four corrected sub-claims are
  recorded in `### PC-3, each issue reproduces at the audit point`
  (012 FR-039).
- **FR-032**: A public API change SHALL be versioned deliberately and
  recorded in the version lineage (Refactoring and Evolution). The
  repository holds no changelog file, and the version lineage this
  feature records is the spec 012 plan's section `Version lineage`
  together with the version requirement it states.
- **FR-033**: The code this feature adds or changes SHALL reach 100 percent
  line, branch, and contract coverage (012 FR-040). The count of
  coverage-exclusion markers in `source/counters/` SHALL NOT rise
  (012 FR-027). The clang-tidy warning count of each touched translation
  unit SHALL NOT rise (012 FR-041). The pre-existing backlog stays out of
  scope except on the lines this feature touches (012 FR-042).
- **FR-034**: All prose this feature adds SHALL satisfy Principle XI,
  including XI.7 (012 FR-044).
- **FR-035**: A defect the 012 clarification decisions govern SHALL keep
  those decisions. The one-integer `point_sink::put`, the availability
  enumeration beside the target bitmask, and the refused-fast-read gap
  with no syscall read all stay as they are at the audit point, as
  `### PC-4, the spec 012 clarification decisions still hold` records.

### Key Entities

- **Gap mark**: the availability value one sampling action writes into
  the plan's disclosure column when that action measured nothing. It is a
  property of one action, and an entry spans many actions, so the value
  travels per action and the catalog holds none of it.
- **Disclosure column**: one managed column per sampling action, written
  last, beside the counts and the ratio pair's two columns, and named to
  every window of a compiled plan under FR-002.
- **Fast verdict**: the answer to whether one device's event page permits
  the fast instruction. FR-017 makes it a property of the device.
- **Encoding key**: a table key the kernel's own generator maps to a
  kernel format. A key outside that set carries no encoding obligation.
- **Unit map and suffix rule**: the kernel generator's mapping from a
  table unit to a device name, and its rule for a numbered instance, which
  FR-014 applies to device routing.
- **Target mask**: the fixed-size bitmask of the target kinds an entry can
  be counted on, carried beside the countability state in the catalog.
- **Version lineage**: the version requirement and the plan section that
  name each deliberate public-API change. The repository holds no
  changelog file.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A fixture drives a gap point after cumulative counts above
  zero. No fold that starts or ends at the gap point reports a value
  computed from the zero point. Each such fold discloses the gap state.
- **SC-002**: A plan holds PMU leaves and clock leaves under both provider
  registration orders. A driven PMU gap reaches the fold result under
  each order.
- **SC-003**: A caller reads the availability field of a fold result and
  of a raw view through the public surface. The caller's source names no
  column index. The field holds the same value the disclosure column
  holds for the same action.
- **SC-004**: Event-page fixtures set the scale, offset, and shift fields,
  and a second fixture sets the short-counter form. The disclosed ratio
  over a multiplexed window matches the header recipe within one tick.
- **SC-005**: Over the pinned tree, no row with a non-zero register value
  publishes as countable without its filter. A fixture pins the
  per-directory counts of rows that encode their filter and of rows that
  publish as not encodable. A paired index publishes under its first
  index, and the pinned counts match what the kernel's own generator
  produces for the same rows.
- **SC-006**: No row in the pinned tree fails to encode because of a
  counter-constraint key or a deprecation key. The fixture recounts the
  encodable rows for the skylake, icelake, alderlake, and sapphirerapids
  directories, and the AMD directory counts do not fall.
- **SC-007**: A synthetic device fixture holding a core device and an
  uncore device on a fast-capable host publishes the fast mode on the core
  entries alone. A plan over both devices reads the uncore leaf with no
  gap.
- **SC-008**: Synthetic device fixtures named for a numbered cache
  instance, a second numbered cache instance, a numbered memory instance,
  and the three AMD vendor devices each receive the rows their unit names.
  No uncore row appears on a core device.
- **SC-009**: A fixture with a model-specific-register device whose
  per-thread probe succeeds publishes the thread target bit on its entries.
- **SC-010**: The rewritten multiplex-window test and the rewritten
  recorder gap test each fail at the pre-fix head and pass after the fix.
- **SC-011**: The version lineage names each declaration commit `cd5cbd1`
  removed and the bump that records its removal, and neither declaration is
  restored. The package config reads the minor-version compatibility while
  the major version is 0. The project version reads 0.4.0.
- **SC-012**: The consumer step fails on a catalog entry count that
  differs from the build tree's count on the same runner.
- **SC-013**: `ctest`, `dbc-gate`, `format-check`, `spell-check`,
  `prose-lint`, the address and thread sanitizer presets, and the 100
  percent line, branch, and contract coverage gates pass. The
  coverage-exclusion marker count in `source/counters/` does not rise. The
  clang-tidy warning count of each touched translation unit does not rise.

## Assumptions

- Every value the library hands a caller is a correct value, or the
  library discloses that the value is unavailable. No third state ships.
- An issue marked confirmed rests on the cited lines at the audit point.
  Every issue in this specification is confirmed by reading.
- The reference host is AMD. Spec 011 names it as an AMD Ryzen 9 9950X3D
  running Linux 7.2.4-1-cachyos with glibc 2.44, and the same host serves
  every figure this specification publishes.
- The continuous-integration matrix runs at `perf_event_paranoid` 2 on
  Linux on GCC and Clang, where no test needs a privileged event the
  kernel grants by default.
- The kernel's own interface header is the authority for the fast-read
  decode recipe and for the enabled and running time computation. Where
  the header and the tree disagree, the header governs.
- A test that needs kernel behaviour uses synthetic device-tree, table, and
  event-page inputs, as spec 007 required and 012 FR-034 carries.
- The kernel's own table generator governs which index of a paired
  register index the format comes from. The plan verifies that rule
  against the generator before the implementation follows it, and a
  generator that takes the second index changes FR-010 to match.
- The counters library keeps zero external runtime dependencies, and this
  feature adds none.
- The successor log at `specs/007-counters-and-timers/citations-log.md`
  keeps accepting corrections in its recorded entry format, and the
  frozen record takes no edit.
- The camelCase rename of identifiers and the benchmark harness, its
  registration API, and its reporting follow this feature. This
  specification corrects the substrate the harness compiles plans against.
- Recorder arenas live for the lifetime of their plan, every
  `plan::recorder()` call allocates a new arena, and a recorder has no
  reset (012 FR-009).
- The installed library is an archive whose code links into the
  consumer's executable. The archive has no location at run time, so the
  vendored tables travel inside it as raw JSON bytes in static data
  (012 FR-023).
- A cpu-target fast-mode plan opens its contexts on its target processor,
  and its sampling thread stays on that processor. The precondition is
  semantic-gated on the caller, so a release build configured `ignore`
  emits no code for it (012 FR-045).
- The read-mode decision under FR-017 and FR-018 is compile-time work. It
  moves no sampling-path cost, and FR-027 holds the per-action cost where
  012 FR-008 left it.
- The disclosure-column change under FR-002 is compile-time work plus one
  store per window per action. FR-027 measures the resulting per-action
  median on the reference host.
- The repository holds no changelog file. The version lineage is the
  version requirement in the spec 012 specification and the plan section
  `Version lineage` at `specs/012-counters-defect-resolution/plan.md`.
  FR-032 names where this feature records its own lineage.

## Out of Scope

- The camelCase rename of identifiers.
- The benchmark harness, its registration API, and its reporting.
- Recorder reuse across repetitions, a recorder reset, and any change to
  a recorder's arena lifetime (012 FR-009).
- A new event source, a topology tree, and metric-expression ingestion.
- Multiplex-aware group scheduling.
- A re-pin of the vendored event tables.
- The Intel host confirmation that 012 deferred. The deferral stays
  deferred, and no requirement, test, or gate here depends on it
  (012 FR-039).
- The pre-existing clang-tidy and coverage-exclusion backlog, except on
  the lines this feature touches (012 FR-042).
- A new availability value. FR-006 reuses the gap value the shipped
  enumeration already carries.

## Open questions for clarify

Three questions were answered on 2026-10-06. The answers stand as
written below, and the requirements they settle name no question.

1. **F-02, which public shape carries the gap state? Answer: an
   availability field on the fold result, and the same field on the raw
   view.** The fold result then names the reason for a gap through the
   enumeration the disclosure column already carries. A boolean would
   answer only that a gap exists, and an accessor would put the state
   behind a call the harness makes once per figure. FR-004, FR-005, and
   SC-003 carry the answer.
2. **F-03, does this feature restore the removed declarations or record
   their removal? Answer: record both removals, and ship as 0.4.0.** The
   camelCase rename lands next and renames the whole surface, so a
   restoration now would be renamed away. A recorded removal is breaking,
   so the release takes the minor position and the shared-object version
   is a hand-kept ABI number. FR-020, FR-021, and SC-011
   carry the answer.
3. **I-01, does a row with a register filter encode through the format
   its register index names, or publish as not encodable? Answer: it
   encodes, and a paired index publishes under the first index of the
   pair.** A row that loses its filter counts a different event, while
   publishing 5997 rows as not encodable discards valid ones. A row whose
   named format the running kernel does not publish is `not_encodable`
   either way. The plan verifies against the kernel's own generator that
   the first index of a pair is the one it takes before the
   implementation follows that rule. FR-010 and SC-005 carry the answer.

## Dependencies

- The kernel's own interface header, which documents the fast-read decode
  recipe, the enabled and running time computation, and the short-counter
  form. FR-007, FR-008, and FR-009 take their content from it.
- The kernel's own table generator, which maps a table unit to a device
  name and an encoding key to a kernel format. FR-011 and FR-014 take
  their content from it.
- The vendored event tables under `external/pmu-events`, pinned by the
  `RECORD` manifest. FR-010 through FR-015 read that tree, and no
  requirement changes the pin.
- The successor log at `specs/007-counters-and-timers/citations-log.md`,
  which accepts every correction this feature records.
- The spec 012 artifacts, whose requirements this feature restores and
  whose research decision D-02 this feature corrects.
- The project's existing gates: the test runner, the coverage gates, the
  contract-pairing gate, the formatter at the pinned version, the prose
  gate, and the installed-consumer job.
- The kernel running on each host, which grants or refuses each event the
  catalog probes and publishes the device tree the routing reads. Where
  the kernel refuses, a fixture supplies the page, the table, or the device
  tree.

## Verified citations at the audit point

Every citation was refreshed at `0dea082`. The table records the value
the audit point carries and the value the request wrote, where the two
differ.

| Issue | Site at the audit point | Request wrote |
| --- | --- | --- |
| I-01 | `source/counters/linux_pmu/table_parse.cpp:147-165` for the drop list, `:222-223` for the branch arm | `:152-165` |
| I-01 | `source/counters/linux_pmu/table_parse.cpp:167-188` for the key map, `:184-186` for the offcore arm, `:228-233` for the fallback | `:184-186` |
| I-02 | `source/counters/linux_pmu/provider.cpp:367-397`, logic at `:374-396`, call at `:131-134` | `:367-395` |
| I-03 | `source/counters/linux_pmu/provider.cpp:290-291`; probe skip at `:175-180`; field at `source/counters/detail/pmu.hpp:213` | `:290-291` |
| I-04(a) | `source/counters/linux_pmu/fast_read.cpp:198-199` and `:203-229`; `source/counters/linux_pmu/provider.cpp:690-691`, `:733`, `:399-438` with `:411` | `fast_read.cpp:203-228`, `provider.cpp:399-420` |
| I-04(a) | `source/counters/linux_pmu/group_io.cpp:745-780`, branch `:756`, fast arm `:766-775`, group read `:779`; narrowing at `:626-630`, `:656-673`, `:683-686` | `group_io.cpp:745-770` |
| I-04(a) | `source/counters/linux_pmu/group_io.cpp:420-423` refusal, `:470-473` mark; the only `::read` at `:285` | unchanged |
| I-04(d) | `source/counters/linux_pmu/fast_read.cpp:353-389`, raw reads `:374-375`, copies `:385-386` | same range |
| I-04(d) | no hit for the page's scale, offset, or shift field under `source/counters/`, `include/`, or `test/` | unchanged |
| I-04(d) | `source/counters/fold.cpp:144-145` division, `:189-191` scaled flag; a multiplexed window reports below 1.0 | `:139-142` |
| I-05 | `source/counters/fold.cpp:45` delta, `:116-126` mark at the end point alone, `:150-154` and `:180-182` fallback, `:291` raw-view fallback | `:197-243`, `:113-125` |
| I-07 | `test/consumer/main.cpp:47-59`; `.github/workflows/ci.yml:394-395` | same |
| F-01 | `source/counters/plan.cpp:542-544`, `:552`, `:558-559`; groups tile at `:515-541`; sample order at `:40-42` | `:542-560` |
| F-01 | `source/counters/clock_provider.cpp:236-238`; guard at `source/counters/linux_pmu/group_io.cpp:714-716` | same |
| F-01 | `source/counters/linux_pmu/group_io.cpp:285-297` short read, `:329-332` mark, `:546-549` slot registration | `:283-296`, `:450` |
| F-02 | `include/speedgun-ng/counters_core.hpp:265-270`; `include/speedgun-ng/counters_measurement.hpp:391-401`; `include/speedgun-ng/counters_provider.hpp:137-145` | `:265-270` |
| F-03 | `include/speedgun-ng/counters_measurement.hpp:731` for the surviving operator; no provider concept under `include/`; lineage at `specs/012-counters-defect-resolution/plan.md` section `Version lineage` | no line given |
| F-04 | `cmake/install-rules.cmake:39`; `CMakeLists.txt:7` and `:41` | same |
| F-05 | `test/source/counters_linux_pmu_seam_test.cpp:510-561`, fixture struct `:111-120`, page builder `:126-137`, time fields `:523-524`, comparison `:538-544` | `:510-545` |
| F-05 | `test/source/counters_recorder_test.cpp:217-298`, index `:247`, ratio assertion `:269-272` | `:230-297` |

### Measured row counts over the pinned tree

Counts come from the JSON parse of each directory under
`external/pmu-events/arch/x86/`. A row is an array item carrying a string
event name, or a top-level object value that is an object. Metric rows
carry no event name and are not rows.

| Directory | Rows | Rows with a non-zero register value | Rows with the value present and zero | Rows with the value absent |
| --- | --- | --- | --- | --- |
| skylake | 587 | 287 | 0 | 300 |
| icelake | 346 | 96 | 0 | 250 |
| alderlake | 563 | 86 | 0 | 477 |
| sapphirerapids | 2693 | 101 | 0 | 2592 |
| amdzen4 | 502 | 0 | 0 | 502 |
| amdzen5 | 579 | 0 | 0 | 579 |

| Directory | Rows carrying the counter-constraint key | Rows carrying the deprecation key |
| --- | --- | --- |
| skylake | 587 | 1 |
| icelake | 346 | 2 |
| alderlake | 563 | 30 |
| sapphirerapids | 2693 | 9 |
| amdzen4 | 0 | 0 |
| amdzen5 | 0 | 0 |

The request wrote that numeric values under the counter-constraint and
deprecation keys become required fields, and gave 447 rows for the
sapphirerapids directory. The audit point carries 2693 rows carrying the
counter-constraint key and 9 carrying the deprecation key. Of the 2693,
447 carry a counter-constraint value that parses as a number and 2846
carry a value that does not, among them 1836 carrying the list
`0,1,2,3`. A row reaches the required-field path only when its value
parses as a number, so the request's figure counts the parseable rows
rather than the rows carrying the key. FR-012 states the requirement
against the key.

No file under `external/pmu-events/` carries the offcore register format
as a key. The parser maps the key `OffcoreRsp`, and that key occurs zero
times in the whole pinned tree, so no row reaches that arm today. The
token `offcore` itself occurs widely, in event names such as
`OFFCORE_REQUESTS.ALL_DATA_RD` and in metric expressions, which the
parser drops.

A row's register index reaches the encoder only through the `MSRIndex`
key, which the parser also drops. What remains is the register index each
row names, counted over the rows whose register value is non-zero:

| Directory | Non-zero register value | Index names 0x1A6 or 0x1A7 | Index names 0x3F6 | Index names 0x3F7 |
| --- | --- | --- | --- | --- |
| skylake | 287 | 260 | 8 | 19 |
| icelake | 96 | 71 | 8 | 17 |
| alderlake | 86 | 46 | 19 | 21 |
| sapphirerapids | 101 | 71 | 9 | 21 |
| amdzen4 | 0 | 0 | 0 | 0 |
| amdzen5 | 0 | 0 | 0 | 0 |

The kernel's own table generator names the offcore register format for
the index 0x1A6 and for the index 0x1A7, the load-latency format for the
index 0x3F6, and the frontend format for the index 0x3F7. Three index
spellings occur across the tree: the pair `0x1a6,0x1a7` on 5045 rows,
`0x1A6` on 810, and `0x1a6` on 60. Every one of the four Intel
directories above names the offcore index, through the paired spelling,
so the request's claim that no pinned Intel row names an offcore index
does not hold and the specification corrects it here. The largest such
directory is skylake with 260 rows, and 5997 rows across 34 pinned
directories name an offcore index with a non-zero register value.

The paired spelling matters to the requirement. A row names two indices
and carries one register value, so the row publishes under one of them,
and the choice decides which format the value encodes through. FR-010
requires the row to publish as not encodable when that choice is
unresolved.