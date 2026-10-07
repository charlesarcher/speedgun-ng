# Phase 0 research: counters defect follow-up

Every NEEDS CLARIFICATION marker in the technical context of `plan.md`
is resolved below. Each defect the specification names was re-read at its
cited site before this file was written, and each verdict records what the
lines show.

## Method

A defect is **confirmed** where the cited lines at the audit point show
it. Every issue this feature carries is confirmed by reading, and the
specification records the evidence for each under its precondition
PC-3. No issue rests on a run.

The kernel's two authorities were read directly. The interface header
`include/uapi/linux/perf_event.h` on the reference host carries the
enabled and running time computation, the short-counter form, and the
fast-read capability bits. The table generator
`tools/perf/pmu-events/jevents.py` on the kernel's own source tree
carries the register-index map and the key-to-term map. Neither file is
vendored in this repository, so each was fetched from the kernel's
upstream source and the commit it was read at is recorded under D-05.

The pinned event tables under `external/pmu-events` were parsed row by
row. Every figure this file publishes was measured over that tree at the
audit point, and the measurement script is recorded under
`### Figures this plan schedules`.

The request that opened this feature carried four figures that did not
survive the audit point. Each is corrected under `### Corrections to the
request` below, and each correction narrows or widens scope against a
measurement. No figure rests on a reading.

## Defect confirmation

### I-05, a fold subtracts across a gap point (confirmed)

`source/counters/fold.cpp:45` computes `column[ctx.j] - column[ctx.i]`
on the raw count column and casts the result to a double. The disclosure
column is a separate column. `leaf_ratio` at `fold.cpp:116-126` reads the
disclosure column at the end point alone, and the fold at
`fold.cpp:150-154` and `fold.cpp:180-182` falls back to a ratio of 1.0
when the disclosure is absent or unreadable. A window whose end point
carries the gap mark therefore reports a delta computed from a zero
count, and a window whose start point carries it reports the whole
cumulative count as its delta. Both are wrong values with no disclosure.

### F-01, only the last provider writes the disclosure column (confirmed)

`source/counters/plan.cpp:542-544` resolves the disclosure column to the
one window that owns the plan's last managed leaf, and `plan.cpp:552`
takes that window alone. `source/counters/linux_pmu/group_io.cpp:714-716`
returns early when a leaf set names no disclosure column, so every window
but that one writes no mark at all. The clock provider writes its mark
unconditionally at `clock_provider.cpp:236-238`. The read groups follow
provider registration order at `plan.cpp:515-541`, so a plan mixing PMU
and clock leaves loses every PMU gap whenever the clock provider
registered second.

### F-02, a fold result carries no gap state (confirmed)

`struct metric_result` at `include/speedgun-ng/counters_core.hpp:265-270`
holds a value, a running ratio, and a scaled flag. Nothing names the gap.
`include/speedgun-ng/counters_measurement.hpp:391-401` defines
`points_view` with the same omission. The only reader of the disclosure
column outside the library is
`test/source/counters_recorder_test.cpp:247`, which addresses it by the
hard-coded index 2.

### I-04(d), the fast pair omits the time extrapolation (confirmed)

`source/counters/linux_pmu/fast_read.cpp:374-375` copies
`time_enabled` and `time_running` out of the event page under its
sequence, and `fast_read.cpp:385-386` hands both to the caller unchanged.
No file under `source/counters/` reads `cap_user_time`, `time_offset`,
`time_mult`, or `time_shift`. The kernel header documents the
computation at the capability bit `cap_user_time`: the page's cycle
counter, the shift, the offset, and the multiplier produce a delta that
the recipe adds to the enabled pair, and to the running pair where the
page index is non-zero. The header documents the narrow-counter form
under `cap_usr_time_short`, where the cycle value is corrected by the
page's cycle and mask fields before the delta is computed. Spec 012's
research decision D-02 stated that the header names no scaling rule.
That statement is false, and the successor log corrects it against this
requirement.

### I-01, rows with a register filter count the wrong event (confirmed)

`source/counters/linux_pmu/table_parse.cpp:157-158` drops `MSRValue` and
`MSRIndex` with no encoding obligation. The key map at
`table_parse.cpp:173-188` names four formats: `cmask`, `inv`, `edge`, and
`offcore_rsp`. The key `OffcoreRsp` occurs zero times in the pinned tree,
so the `offcore_rsp` arm at `table_parse.cpp:184-186` is unreachable
today, and a row carrying a register value publishes as countable with no
filter. The measured counts are under `### Figures this plan schedules`.

The same parser has a second defect on the same path. The no-obligation
list at `table_parse.cpp:155-163` does not name `Counter` or
`Deprecated`, so both reach the required-field arm at
`table_parse.cpp:222-223`. A value that parses as a number becomes a
required field, and the kernel publishes no format of either name, so
every such row turns `not_encodable`. Measured over the pinned tree, 447
sapphirerapids rows carry a `Counter` value that parses as a number, 447
in total across the six directories under test.

### I-04(a), one host-wide fast verdict sets every device's mode (confirmed)

`source/counters/linux_pmu/fast_read.cpp:203-229` probes one core
hardware event and sets one verdict.
`source/counters/linux_pmu/provider.cpp:690-691` reads that verdict, and
`provider.cpp:733` passes it to `probe_device` for every device in turn.
`entry_read_selection_for` at `provider.cpp:399-438` then sets the fast
mode on every countable entry at `provider.cpp:411`.
`source/counters/linux_pmu/group_io.cpp:745-780` opens a fast window
whenever every leaf carries the fast mode, and it reaches the group read
only through the three narrowing arms at `group_io.cpp:626-630`,
`group_io.cpp:656-673`, and `group_io.cpp:683-686`. None of those arms
consults the device's own page. An uncore or RAPL page publishes no
`cap_user_rdpmc`, so every fast read of such an entry is refused and every
sampling action discloses a gap.

### I-02, uncore rows reach no numbered or vendor-named device (confirmed)

`scope_reaches` at `source/counters/linux_pmu/provider.cpp:367-397`
compares a table's unit against the device name, or against the name
`uncore_` followed by the unit, by exact text after case folding at
`provider.cpp:374-379`. The kernel numbers most uncore instances, for
example `uncore_cha_0` and `uncore_imc_0`. AMD devices take vendor names,
for example `amd_df`, `amd_l3`, and `amd_umc_0`. The pinned AMD tables
spell the units `DFPMC`, `L3PMC`, and `UMCPMC`. No numbered Intel uncore
row and no AMD uncore row reaches a device at the audit point. The
reference host publishes `amd_iommu_0` and no uncore device at all.

### I-03, the model-specific-register PMU is labeled scope-refused (confirmed)

`source/counters/linux_pmu/provider.cpp:290-291` marks every device whose
path is not `cpu`, `cpu_core`, or `cpu_atom` as device-scoped, by name.
The provider then skips the per-task probe for those entries at
`provider.cpp:175-180` and publishes `scope_refused` at
`provider.cpp:176`. The `msr` PMU registers a per-task context and
accepts per-task events, and the reference host publishes it at
`/sys/bus/event_source/devices/msr` with type 13. Its three counters are
therefore unavailable to every per-thread target on an unprivileged host.

### F-05, two 012 tests pass on the defect they guard (confirmed)

`test/source/counters_linux_pmu_seam_test.cpp:510-561` builds an event
page through the fixture at `seam_test.cpp:126-137`, sets two time fields
at `seam_test.cpp:523-524`, and compares the ratio the pair yields with
the same quotient recomputed from the same two fields at
`seam_test.cpp:538-544`, under a tolerance of 1.0 on a unitless value.
The fixture struct at `seam_test.cpp:111-120` has no field for any of
`cap_user_time`, `time_mult`, `time_shift`, `time_offset`,
`time_cycles`, or `time_mask`, so the test cannot fail while I-04(d) is
present. The recorder gap test at
`test/source/counters_recorder_test.cpp:217-298` sets every count to zero
before the gap point at `counters_recorder_test.cpp:247`, so the zero the
gap writes equals the first real point and the test cannot fail while
I-05 is present.

### F-03, two public declarations left with no version note (confirmed)

Commit `cd5cbd1` removed the non-member
`operator*(const expression&, const double)` from
`include/speedgun-ng/counters_measurement.hpp`, leaving only the member
form, and removed `concept provider` from
`include/speedgun-ng/counters_provider.hpp`. A search for `concept
provider` across `include/` returns nothing. Spec 012's requirement
FR-024 records no removal, and the version lineage under
`specs/012-counters-defect-resolution/plan.md` names neither.

### F-04, the package accepts a breaking minor release (confirmed)

`cmake/install-rules.cmake:39` writes `COMPATIBILITY SameMajorVersion`,
and `CMakeLists.txt:7` sets the project version to 0.3.0 with
`CMakeLists.txt:41` setting the shared-object version to 0. On a 0.x line
a request for 0.2 therefore accepts 0.3, and 0.3 adds availability values
and removes the two declarations F-03 names.

### I-07, the installed-package row count has no automated comparison (confirmed)

`test/consumer/main.cpp:47-59` prints the catalog entry count.
`.github/workflows/ci.yml:394-395` runs the consumer and compares that
count with nothing. Spec 012's success criterion SC-008 requires the
count to equal the build tree's count on the same host.

## Corrections to the request

Four claims in the request that opened this feature did not survive the
audit point. Each is recorded here so that the plan rests on measurement.

1. The request stated that no pinned Intel row names an offcore index.
   At the audit point 260 skylake rows, 71 icelake rows, 46 alderlake
   rows, and 71 sapphirerapids rows carry a register value that is not
   zero together with an index naming the offcore register, spelled
   `0x1a6,0x1a7`. The request counted the single-index spellings only.
2. The request stated that the figure 447 counts the rows carrying the
   counter-constraint key in the sapphirerapids directory. That
   directory carries 2693 rows with the key, of which 447 carry a value
   that parses as a number. Only the second figure reaches the required
   field, so the request's figure is right and its explanation is not.
3. The request stated that the parser maps the key `OffcoreRsp` and that
   no file in the pinned tree carries that key. Both hold, and the arm is
   therefore unreachable today. The register-index path beside it is a
   separate route and carries 5997 rows across 34 pinned directories.
4. The request stated that the two branches following this feature exist.
   At the audit point no branch names the camelCase rename or the
   benchmark harness. The plan records their base commit as this
   feature's tip instead.

## Decisions

### D-01 A fold reads the gap mark at both of its end points

**Decision**: `leaf_ratio` and every delta path read the disclosure
column at the start point and at the end point. A window whose start or
end point carries `gap` reports no value, and reports the gap state
through D-02.

**Rationale**: the counts are cumulative, so a gap strictly inside a
window leaves both end points measured and the delta correct. Reading
only the end point, as the code does at `fold.cpp:116-126`, misses a gap
that opens the window, and reading neither end point's mark before
subtracting at `fold.cpp:45` computes a value from a zero count. Two
reads fix both windows and cost nothing on the sampling path, because
the fold runs once per figure and not once per sampling action.

**Alternatives considered**: reading the mark at every point in the
window. Rejected: the gap state is per-action and per-point, and a fold
over a window that reports no value needs no per-point state. Zeroing a
gapped delta and reporting 0.0. Rejected: a zero is a correct count for
an event that did not occur, and the library discloses unavailability
rather than reporting a value it cannot compute.

### D-02 The gap state travels as an availability field

**Decision**: `struct metric_result` gains an `availability` field, and
`struct points_view` gains the same field. Both hold the value the
disclosure column holds for the same sampling action. The enumeration
gains no value for this purpose.

**Rationale**: the disclosure column already carries an `availability`
value, and `availability` at
`include/speedgun-ng/counters_core.hpp:87-95` already separates a
permission refusal, an encoding refusal, an absent entry, and a scope
refusal. Reusing it lets a fold name the reason for a gap instead of
only its presence, and it lets one field serve both the fold result and
the raw view. A boolean would answer only whether a gap exists, and an
accessor on the recorder surface would put the state behind a call the
harness makes once per figure. The field is additive, so it changes no
existing enumerator value and no stored bit.

**Alternatives considered**: a boolean field. Rejected: it discards the
reason, and the harness reports a reason when one exists. An accessor on
the recorder surface. Rejected: it re-introduces the layout knowledge
the field removes, and it costs a call per figure.

### D-03 Every window that owns a leaf owns a disclosure column

**Decision**: the compile names a disclosure column per window, and each
window writes its own mark on every sampling action it serves. A window
with no leaf of its own receives no column.

**Rationale**: `plan.cpp:552` names the column to one window, which makes
the disclosure depend on provider registration order, and
`group_io.cpp:714-716` then skips the mark for every other window. Naming
a column per window removes the order dependence entirely and costs one
`std::size_t` per window. The windows already iterate their own leaves
in order, so each writes its mark in the same pass that writes its
counts.

**Alternatives considered**: one column for the whole plan, with a bit
per window. Rejected: `point_sink::put` writes one integer per leaf per
action, so a per-window mark needs a column per window to keep that
signature. Carrying the mark in the count's sign bit. Rejected: the
count is an unsigned cumulative value the harness reads directly, and a
sign bit would corrupt every reader that does not know the convention.

### D-04 The fast pair follows the header recipe

**Decision**: `fast_context_time_pair` takes the page's cycle counter,
`time_shift`, `time_offset`, `time_mult`, and `time_cycles` and
`time_mask` inside its existing sequence loop, and computes the pair the
header's `cap_user_time` recipe describes. It applies the short-counter
correction where the page states `cap_usr_time_short`. Where the page
states neither, it copies the raw pair.

**Rationale**: the header is the authority (012 D-02 corrected against
it). The sequence loop already runs, so the extra reads cost no
additional synchronization, and the computation is integer arithmetic on
values the page already holds. Without it a short multiplexed window
reports a raw running time that understates the true running time, and the
fold at `fold.cpp:144-145` divides by it and reports a ratio of 1.0 for a
window the kernel multiplexed.

**Alternatives considered**: leaving the extrapolation to the fold.
Rejected: the fold receives two `std::uint64_t` values through
`leaf_ratio` and has no access to the page's scale fields, so the
correction cannot happen there. Reading the page a second time outside
the sequence. Rejected: a torn read of the scale fields is exactly what
the sequence exists to prevent.

### D-05 A register filter encodes through the format its index names, and a pair publishes under its first index

**Decision**: a row whose register value is not zero reads its
`MSRIndex`, takes the first index of a comma-separated pair, resolves
that index through the kernel's map, and encodes the value into the
format the map names. A row whose resolved index names a format the
running kernel does not publish, or names no entry in the map, publishes
as `not_encodable`. A row whose register value is present and zero
encodes its base event with no filter.

**Rationale**: the kernel's own generator settles both halves of this
decision. `tools/perf/pmu-events/jevents.py` at kernel commit
`eaab2eb09dc2f86f41e8fa55243c31a274978233` reads, at line 244,
`"""Converts the msr number, or first in a list to the appropriate
event field."""`, and its body at line 258 is
`return msrmap[int(num.split(',', 1)[0], 0)]` over the map at lines 248
to 257, which names `ldlat` for `0x3F6`, `offcore_rsp` for `0x1A6`,
`0x1A7`, `0x3E0`, `0x3E1`, `0x3E2`, and `0x3E3`, and `frontend` for
`0x3F7`. The kernel takes the first index of a pair, the same rule this
feature adopts, and its base-0 parse accepts every spelling the pinned
tree uses. The line at `jevents.py:410-411` appends the resolved term and
the row's value, so the value lands in the named format.

The pinned tree uses seven distinct index spellings, and every one
resolves under D-05: the paired form `0x1a6,0x1a7` on 5045 rows, the
single form `0x1A6` on 810, `0x3F6` on 379, `0x3F7` on 307, the paired
form with a space after the comma on 64, `0x1a6` on 60, and `0x1a7`
alone on 18. No index in the tree falls outside the kernel's map.

Publishing a row as `not_encodable` instead would discard 448 valid rows
across the four Intel directories this feature tests, and 5997 across 34
pinned directories in total. A row that loses its filter counts a
different event, which spec 012's own principle rejects.

**Alternatives considered**: taking the second index of a pair.
Rejected: the kernel takes the first, and a row whose count disagrees
with the kernel's own tool is the wrong-count defect this feature
closes. Encoding the value into every format the pair names. Rejected: a
row carries one value and one config word per member, so a second format
has nowhere to land. Hard-failing on an unmapped index, as the kernel
does at `jevents.py:258` where `msrmap[...]` has no default arm.
Rejected: a library cannot abort a process over one table row, and
`not_encodable` is the state the catalog already publishes for a row it
cannot encode.

### D-06 The counter-constraint and deprecation keys carry no encoding obligation

**Decision**: `carries_no_obligation` names `Counter` and `Deprecated`
alongside the seven keys it names today, so a row carrying either key
encodes on its base fields alone.

**Rationale**: the kernel's generator has no term for either key. Its
`event_fields` list at `jevents.py:389-404` maps `AnyThread` to `any`,
`PortMask` to `ch_mask`, `CounterMask` to `cmask`, `EdgeDetect` to
`edge`, `FCMask` to `fc_mask`, `Invert` to `inv`, `SampleAfterValue` to
`period`, `UMask` to `umask`, `NodeType` to `type`, `RdWrMask` to
`rdwrmask`, `EnAllCores` to `enallcores`, `EnAllSlices` to
`enallslices`, `SliceId` to `sliceid`, and `ThreadMask` to
`threadmask`. Neither `Counter` nor `Deprecated` appears. A value that
reaches the required-field arm for a key no format names turns the row
`not_encodable` for a field the hardware ignores.

**Alternatives considered**: mapping `Counter` to `cmask`. Rejected:
`CounterMask` already maps to `cmask`, and the two keys are different
fields with different meanings. Leaving the keys required and publishing
the affected rows as `not_encodable`. Rejected: it removes 466 rows
carrying a counter-constraint value from the catalog, plus 42 carrying a
deprecation value, for fields no format reads.

### D-07 The device's own format list decides whether a format exists

**Decision**: the format a row encodes through is resolved against the
format list the device publishes, and a format the device does not
publish leaves the row `not_encodable`. The parser reads no format from
the vendored tree.

**Rationale**: `pmu_compose_config` at
`source/counters/linux_pmu/encode.cpp:132-159` already refuses a field
the device publishes no format for, and that refusal is spec 012's
FR-037. D-05 and D-06 add the terms and drop the keys, and the existing
resolution decides the rest. Whether `offcore_rsp`, `ldlat`, and
`frontend` are available therefore depends on the running kernel, which is
the answer the running kernel gives, and the reason the row count is
pinned per device. One assertion cannot cover every device.

Two costs follow, and both land in the test. The seam test's synthetic
device publishes `offcore_rsp` and neither of the other two formats, so it
grows two entries with the ranges the kernel publishes at
`arch/x86/events/intel/core.c` lines 6602, 6606, and 6608. And the
reference host's own list publishes none of the three, because this host
is an AMD core publishing `cmask`, `edge`, `event`, `inv`, and `umask`
alone, so every register-filter row publishes `not_encodable` there. That
is a correct answer and not a defect: an AMD kernel publishes no offcore
register format.

**Alternatives considered**: assuming the format exists because the
table names it. Rejected: an `open` on a format the device does not
publish fails, and the failure would surface as an open error rather than
as a catalog state. Caching the format list across devices. Rejected: the
list is per device, and two devices of different kinds publish different
sets. Reading a format list from the vendored table. Rejected: the
vendored tables publish no format list, and inventing one would encode
against a device that publishes none.

### D-08 Device scope comes from the kernel's published data or the probe

**Decision**: the provider stops deciding scope by name. A `cpumask`
file marks a device device-scoped. A `cpus` file does not. `cpu`,
`cpu_core`, and `cpu_atom` stay per-task capable. A device that
publishes neither file takes the per-task probe.

**Rationale**: the `msr` PMU publishes `perf_sw_context` as its task
context and accepts per-task events, so the entries are countable on a
per-thread target and the current name-based rule denies a count the
kernel would grant. The per-task probe at `provider.cpp:175-180` already
answers the question, and its verdict is the evidence the catalog should
publish. Spec 012's FR-021 and FR-022 already require the target bit
wherever the probe succeeds.

**Alternatives considered**: a hard-coded allow-list of per-task devices.
Rejected: it replaces one name list with another and ages the same way.
Publishing the thread target bit for every device. Rejected: a device
with no per-task context cannot deliver the bit, and publishing it anyway
hands the caller a target the kernel refuses.

### D-09 An uncore row reaches every instance the generator's unit map names

**Decision**: `scope_reaches` resolves a table's unit through the
kernel's generator unit map and then matches every device whose name the
resolved unit reaches, including a numbered instance. The core routing
of spec 012's FR-019 is unchanged.

**Rationale**: the generator maps a unit to a device name and ignores a
numeric instance suffix, so `DFPMC` reaches `amd_df` and every instance
of it, and `CHA` reaches `uncore_cha_0`, `uncore_cha_1`, and every other
numbered cache instance. The exact-text comparison at
`provider.cpp:387-392` reaches none of them. The measured units across
the pinned tree carry 48 distinct units, and the largest are `CHA` on
7351 rows, `iMC` on 2785, `IIO` on 2657, `M3UPI` on 2626, `M2M` on 2583,
`cpu_core` on 1649, `M2PCIe` on 1384, `cpu_atom` on 1317, `HA` on 958,
`CBOX` on 879, `UPI` on 868, and `IRP` on 695. The AMD tables spell their
uncore units `DFPMC` on 376 rows, `IMC` on 354, `L3PMC` on 75, and
`UMCPMC` on 39, and every one of them is currently unreachable.

**Alternatives considered**: prefix matching on `uncore_` alone.
Rejected: it still misses `amd_df`, `amd_l3`, and `amd_umc_0`, and it
admits a device whose class does not match. One representative instance
per unit. Rejected: an uncore instance counts its own socket's
controllers, so a plan that reads instance zero and reports a
package-wide figure understates it.

### D-10 Each device carries its own fast verdict, and the refusal stays a gap

**Decision**: the fast verdict is taken per device from that device's
own page. One host-wide probe of a core event decides nothing. An entry
whose device page refuses the fast read takes the syscall group read at
compile time. A fast read refused at sampling time still discloses a gap
and still issues no syscall read.

**Rationale**: `cap_user_rdpmc` is a per-device capability, and an
uncore or RAPL page publishes no such bit, so a host-wide verdict from a
core event sets a mode the device cannot serve. The compile-time
decision costs nothing on the sampling path and removes a per-action
refusal that a sampling action pays for in a disclosed gap. The
sampling-time behaviour is spec 012's FR-002 and D-03, and this decision
does not touch it.

**Alternatives considered**: probing every device at registration.
Rejected: a probe per device opens a context per device on every host,
including a host whose permission level refuses it. Narrowing the fast
window per device at compile time by consulting the page the open
returns. Accepted, and it is the same decision stated as an
implementation site.

### D-11 The release ships as 0.4.0 with a hand-kept shared-object number

**Decision**: the project version becomes 0.4.0, the package config
compatibility becomes the minor-version compatibility while the major
version stays 0, and `SOVERSION` is the hand-kept ABI number 1. The
maintainer bumps that number when a public signature or a public record
layout changes. Neither removed declaration is restored; both removals
are recorded in the version lineage with this bump.

**Rationale**: the availability field D-02 adds is additive, and the two
removals spec 012's `cd5cbd1` made are breaking, so the release takes
the minor position. `SOVERSION` stays a hand-kept number so a 0.x
release can name a new ABI without waiting for major version 1.
Restoring the declarations
now buys nothing: the camelCase rename lands next and renames the whole
surface, so a restored declaration would be renamed away.

**Alternatives considered**: shipping 0.3.1 and restoring the
declarations, keeping the release additive. Rejected: it contradicts the
answer to F-03 and leaves the shared-object question open, because a
0.3.1 consumer still expects the removed declarations. Keeping
`SameMajorVersion`. Rejected: on a 0.x line it accepts every later minor
release, which is the defect F-04 names.

### D-12 The installed consumer's count is compared in continuous integration

**Decision**: one step runs the installed consumer and one step compares
its printed catalog entry count with the build tree's count on the same
runner. A difference fails the job. The step runs unprivileged, and it
holds on a runner that publishes no performance monitoring unit device.

**Rationale**: spec 012's SC-008 requires the equality and nothing
enforces it. The comparison needs both counts on one runner, so it
belongs in one job where the build tree and the install tree are
siblings. A runner with no device publishes no entry on either side, so
both counts are zero and the comparison holds without a special case.

**Alternatives considered**: comparing the counts in a test rather than
in the workflow. Rejected: a test runs against the build tree and cannot
see the installed package's view of the same tree. Comparing the installed
consumer's count against a figure recorded in the repository. Rejected: a
recorded figure is stale the moment a table is re-pinned, and this feature
re-pins nothing while the next one might.

### D-13 Each rewritten test fails at the pre-fix head

**Decision**: the multiplex-window fixture gains the scale, offset, and
shift fields and a second fixture gains the short-counter form, and the
recorder gap fixture drives cumulative counts above zero before the gap
point. Each rewritten test fails at the audit point and passes after its
fix.

**Rationale**: both tests currently pass on the defect they guard, for
the reasons recorded under F-05. A test that cannot fail is not a gate,
and spec 012's SC-010 requires the rewritten pair to fail at the
pre-fix head. Driving the counts above zero separates the zero the gap
writes from the first real point, and setting the scale fields makes the
extrapolation observable in the disclosed ratio.

**Alternatives considered**: adding a new test beside the existing pair.
Rejected: the existing pair would still pass on the defect and would
still read as coverage, which is the state F-05 names.

### D-14 The successor log records each correction against what it restores

**Decision**: every correction records one entry in
`specs/007-counters-and-timers/citations-log.md`, against the 007 or 012
requirement it restores, and the frozen 012 record takes no edit.

**Rationale**: the log is where a reader looks to learn why a cited
figure moved. Spec 012's D-02 is corrected there against this feature's
requirement, because that decision stated the header names no scaling rule
and the header names one.

**Alternatives considered**: recording the D-02 correction in this
specification alone. Rejected: the successor log is the artifact the next
reader consults, and a correction recorded only beside the requirement
that caused it is invisible from the decision it corrects.

## The Intel confirmation stays deferred

Spec 012 deferred the Intel host confirmation, and this feature defers it
again. Every test in this feature runs on synthetic sysfs, table, and
event-page inputs, so the corrections land without an Intel host and
without a privileged event. The one figure this feature cannot measure
on the reference host is the runtime availability of `offcore_rsp`, which
depends on the running kernel's format list. The pinned tree fixes
nothing here. Spec 012's FR-039 keeps that confirmation deferred, and the
verification matrix records the dependency.

The host class that would settle it: any x86 host whose running kernel
publishes `offcore_rsp`, `ldlat`, and `frontend` in a core device's own
`format` list. Every Intel part from Nehalem on publishes `offcore_rsp`,
so a host bearing any of them settles the figure, and no AMD host settles
it at all.

## Figures this plan schedules

Measured over `external/pmu-events/arch/x86` at the audit point, with the
files this feature touches in the third column.

| Directory | Rows | Non-zero register value | Index names the offcore register | Index names load latency | Index names frontend |
| --- | --- | --- | --- | --- | --- |
| skylake | 587 | 287 | 260 | 8 | 19 |
| icelake | 346 | 96 | 71 | 8 | 17 |
| alderlake | 563 | 86 | 46 | 19 | 21 |
| sapphirerapids | 2693 | 101 | 71 | 9 | 21 |
| amdzen4 | 502 | 0 | 0 | 0 | 0 |
| amdzen5 | 579 | 0 | 0 | 0 | 0 |

Every index in that table is an entry in the kernel's map, so under D-05
no row publishes `not_encodable` on account of its register index on a
kernel that publishes the three formats. The count that changes under
D-05 is the count of rows that publish as `countable` today with no
filter: 570 across the four Intel directories, of which 448 reach the
offcore register. The other count that changes is under D-06: 466 rows
carry a counter-constraint value that parses as a number and 42 carry a
deprecation value that parses as a number, and every one of them
publishes as `not_encodable` today for a field no format reads.

D-05 leaves the encodable-row count unmoved, and the reason is worth
recording. The parser drops both register keys today, so a register-filter
row already encodes on its base fields alone and the seam test already
counts it as encodable. D-05 changes what such a row counts. The number
of encodable rows stays where it is. What D-05 does move is the 122 rows
naming load latency or the frontend: the seam test's synthetic device at
`counters_linux_pmu_seam_test.cpp:767-795` publishes `offcore_rsp` but
neither `ldlat` nor `frontend`, so against it those rows would flip to
`not_encodable` where they are countable today. The kernel publishes all
three at `arch/x86/events/intel/core.c` lines 6602, 6606, and 6608, as
`config1:0-63`, `config1:0-15`, and `config1:0-23`, and the fixture grows
the two missing entries under FR-010's test.

The encodable-row counts the seam test already pins stand at 576 for
skylake, 342 for icelake, 521 for alderlake, 1685 for sapphirerapids, 326
for amdzen4, and 322 for amdzen5. D-06 raises each Intel figure and D-05
leaves it where the fixture grew the two formats, and neither lowers an
AMD figure. The new figures are 581, 346, 564, and 2141, with both AMD
figures unchanged. The plan records them as predictions to be confirmed by
the seam test, and the data model records both columns.

## TDD mode

The plan runs in TDD mode (Principle III). Every correction in this
feature ships with a test that fails at the audit point and passes after
the fix, and the audit point is the pre-fix head for all of them. The
verification matrix in `plan.md` names the test that fails first for each
requirement, and `quickstart.md` gives the command that observes each
failure before any correction lands.

## Repository constraints this plan records

- `point_sink::put` keeps its one-integer signature at
  `include/speedgun-ng/counters_provider.hpp:212`. D-03 satisfies it with
  a column per window. The call stays one integer wide.
- `recorder::sample()` stays `noexcept`, allocation-free, and
  lock-free. D-01, D-02, D-03, and D-10 add work to the fold and to the
  compile, none of which runs on the sampling path, and the overhead
  figures in `quickstart.md` gate the change.
- No public header gains a platform term. The availability field is a
  plain enumeration already declared in `counters_core.hpp`, so
  `test/counters_header_purity.sh` holds without an edit.
- Each changed interface keeps its doxygen contract paired with its
  enforcement. Every new field and every new predicate arrives with the
  `SG_REQUIRE` or `SG_ENSURE` that guards it.
- Every fix is TDD mode and lands on its own commit, so the diff for each
  is bisectable and its test is the proof.
