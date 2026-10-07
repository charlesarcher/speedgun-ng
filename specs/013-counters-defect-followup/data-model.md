# Data model: counters defect follow-up

Every entity here already exists in the merged tree at the audit point.
This feature changes fields, states, and counts on existing entities, and
adds no entity.

## `metric_result`

**Location**: `include/speedgun-ng/counters_core.hpp:265-270`

A fold result. One per fold, returned to the caller, carrying no
allocation.

| Field | Type | Meaning | Validation |
| --- | --- | --- | --- |
| `value` | `double` | the delta over the window, or no value when the window carries a gap at either end point | no value published when the state is `gap` |
| `running_ratio` | `double` | the fraction of the window that ran, over `(0, 1]` | 1.0 when no source multiplexed; `value_or(1.0)` at `fold.cpp:291` stops being the whole story once the state is public |
| `scaled` | `bool` | the figure was scaled to a declared unit | unchanged |
| `availability` | `availability` | **new**: the state the disclosure column holds for the window's end point, and `gap` when the start point carries it too | the value the disclosure column holds for the same sampling action |

**Rules**: the field is additive, so no existing reader breaks and no
stored bit moves. The enumeration gains no value. A window whose start
or end point carries `gap` publishes `gap` and publishes no `value`
(FR-001, FR-004).

**Transitions**: a window is measured with a state, gapped with a state,
or absent. There is no third outcome, and a gapped window never carries a
value.

## `points_view`

**Location**: `include/speedgun-ng/counters_measurement.hpp:391-401`

The raw view of one counter's points, a caller-facing view over the
recorder's buffer.

| Field | Meaning | Validation |
| --- | --- | --- |
| `object_path`, `name`, `description`, `unit` | identity of the counter | unchanged |
| `slot` | point identity, the column index in the plan | unchanged |
| `points`, `count` | the raw cumulative points | unchanged |
| `ratio` | the fraction that ran | unchanged |
| `availability` | **new**: the state the disclosure column holds for the same window | the value the disclosure column holds, `gap` at either end point |

**Rules**: the caller reads the state here and names no column index.
That removes the only layout knowledge outside the library, which today
sits at `counters_recorder_test.cpp:247` as the literal index 2 (FR-005).

## `leaf_set`

**Location**: `include/speedgun-ng/counters_provider.hpp:137-145`

The resolved leaf set a provider offers, with the disclosure column the
window writes its mark through.

| Field | Type | Meaning |
| --- | --- | --- |
| `addresses` | `std::vector<std::string>` | the managed leaves, in order |
| `disclosure_column` | `std::size_t` | **changed**: a column per window, not one for the plan. A window with no leaf of its own receives `no_disclosure_column` |

**Rules**: `no_disclosure_column` keeps its meaning. What changes is that
a window serving leaves of its own no longer shares the plan's last
leaf's column, so a plan's column set no longer depends on provider
registration order (FR-006, FR-007).

## `availability`

**Location**: `include/speedgun-ng/counters_core.hpp:87-95`

The countability state of one catalog entry, reused here as the state of
one fold result and one raw view. Unchanged in this feature.

| Enumerator | Reports |
| --- | --- |
| `countable` | the entry can be counted on the targets its mask names |
| `permission_blocked` | the kernel refused the event at open |
| `not_encodable` | no format on the device encodes the row's fields |
| `absent` | the device publishes no such entry |
| `scope_refused` | the device accepts no event for this target |
| `gap` | a sampling action failed and wrote a zero count beside the mark |

**Rules**: the enumeration gains no value in this feature. A fold that
suppresses its value needs no new state, and a row whose format the
device publishes under no name already reports `not_encodable`.

## Register index map

**New**: the parser's copy of the kernel's map, holding no field of its
own beyond the map itself. Source: `tools/perf/pmu-events/jevents.py`
lines 248 to 257, read at kernel commit
`eaab2eb09dc2f86f41e8fa55243c31a274978233`.

| Index | Format | Published range | Rows in the pinned tree |
| --- | --- | --- | --- |
| `0x1A6` | `offcore_rsp` | `config1:0-63` | 810 as `0x1A6`, 60 as `0x1a6`, 5045 paired, 64 paired with a space |
| `0x1A7` | `offcore_rsp` | `config1:0-63` | 18 as `0x1a7`, and the second index of every paired row |
| `0x3E0` to `0x3E3` | `offcore_rsp` | `config1:0-63` | 0 |
| `0x3F6` | `ldlat` | `config1:0-15` | 379 |
| `0x3F7` | `frontend` | `config1:0-23` | 307 |

The published ranges come from `arch/x86/events/intel/core.c` lines 6602,
6606, and 6608, read at kernel commit
`eaab2eb09dc2f86f41e8fa55243c31a274978233`. They are recorded here
because the seam test's synthetic device must publish all three, and
because a range the running kernel publishes under a different width
leaves the row `not_encodable` rather than mis-encoded.

**Rules**: a row's `MSRIndex` is split on the first comma and the leading
index is resolved (FR-010). An index outside the map yields no format and
the row publishes `not_encodable`. A row whose register value is present
and zero encodes its base event with no filter. The pinned tree uses
seven distinct spellings and every one resolves.

## No-obligation keys

The parser's list of keys that name no kernel format. Today it holds
seven keys: `SampleAfterValue`, `MSRValue`, `MSRIndex`, `PEBS`,
`Data_LA`, `PerPkg`, and `Experimental`.

**Changed**: `MSRValue` and `MSRIndex` leave the list, because D-05 routes
them through the register index map instead of dropping them. `Counter`
and `Deprecated` join it, because the kernel's generator names no term
for either (FR-012, FR-013).

**Rules**: a key outside the list whose value parses as a number becomes
a required field, and the row publishes `not_encodable` when the device
publishes no format of that name. That is the arm D-06 empties for
`Counter` and `Deprecated`.

## Device routing

**Location**: `source/counters/linux_pmu/provider.cpp:367-397`

The rule that maps a table's unit to a device, and a device to its target
scope and read mode. Three fields change shape.

| Aspect | At the audit point | After this feature |
| --- | --- | --- |
| unit to device | exact text after case folding, plus the `uncore_` prefix | the kernel generator's unit map, then an instance suffix rule |
| device scope | by name, outside `cpu`, `cpu_core`, `cpu_atom` | from the kernel's published per-task context, or the per-task probe's verdict |
| read mode | one host-wide fast verdict, applied to every countable entry | one verdict per device, from that device's own page |

**Rules**: the core routing of spec 012's FR-019 is unchanged. An entry
whose device page refuses the fast read takes the syscall group read at
compile time, and a refusal at sampling time still discloses a gap and
issues no syscall read (FR-017, FR-018, FR-019).

## Measured row counts over the pinned tree

Every figure is measured at the audit point over
`external/pmu-events/arch/x86`, counting event rows only.

### Register filters

| Directory | Rows | Non-zero register value | Reaches `offcore_rsp` | Reaches `ldlat` | Reaches `frontend` |
| --- | --- | --- | --- | --- | --- |
| skylake | 587 | 287 | 260 | 8 | 19 |
| icelake | 346 | 96 | 71 | 8 | 17 |
| alderlake | 563 | 86 | 46 | 19 | 21 |
| sapphirerapids | 2693 | 101 | 71 | 9 | 21 |
| amdzen4 | 502 | 0 | 0 | 0 | 0 |
| amdzen5 | 579 | 0 | 0 | 0 | 0 |

570 rows across the four Intel directories publish as `countable` with no
filter today. Under D-05 all 570 encode, 448 of them through
`offcore_rsp`.

### No-obligation keys

| Directory | `Counter` parses as a number | `Deprecated` parses as a number |
| --- | --- | --- |
| skylake | 4 | 1 |
| icelake | 2 | 2 |
| alderlake | 13 | 30 |
| sapphirerapids | 447 | 9 |
| amdzen4 | 0 | 0 |
| amdzen5 | 0 | 0 |

466 rows carry a counter-constraint value that parses as a number and 42
carry a deprecation value that parses as a number. All 508 publish as
`not_encodable` today for a field the hardware ignores. Under D-06 all
508 encode.

### Encodable rows, before and after

The seam test pins one count per directory against a synthetic format
list and against the reference host's own published list. The synthetic
list at `counters_linux_pmu_seam_test.cpp:767-795` publishes `event`,
`umask`, `cmask`, `edge`, `inv`, `config`, `config1`, `config2`, and
`offcore_rsp`. It does not publish `ldlat` or `frontend`, and D-05 needs
both, because a row naming `0x3F6` or `0x3F7` resolves to a format the
list would not carry.

The kernel publishes the three at `arch/x86/events/intel/core.c` lines
6602, 6606, and 6608, read at kernel commit
`eaab2eb09dc2f86f41e8fa55243c31a274978233`:

| Format | Published range | In the synthetic list today |
| --- | --- | --- |
| `offcore_rsp` | `config1:0-63` | yes |
| `ldlat` | `config1:0-15` | no |
| `frontend` | `config1:0-23` | no |

So the fixture grows two entries. That is a correction to the seam test's
own fixture, and the plan records it as part of FR-010's test.

| Directory | Pinned today | Measured after D-05 and D-06 | Movement |
| --- | --- | --- | --- |
| skylake | 576 | 581 | plus 4 counter-constraint, plus 1 deprecation |
| icelake | 342 | 346 | plus 2, plus 2 |
| alderlake | 521 | 563 | plus 13, plus 30 |
| sapphirerapids | 1685 | 1993 | plus 447, plus 9, less 250 `FCMask` and 293 `PortMask` |
| amdzen4 | 326 | 326 | none |
| amdzen5 | 322 | 322 | none |

**Two predictions in the table above were wrong, and the measurement
found them.** alderlake was predicted at 564 and measures 563; the
alderlake table publishes 563 rows in all, so no count above 563 is
reachable and the prediction was impossible. sapphirerapids was
predicted at 2141 and measures 1993. The 148-row gap is two keys this
feature does not carry an obligation for: 293 rows name a `PortMask` and
250 name an `FCMask`, and neither is in D-05's or D-06's scope, so both
correctly publish `not_encodable` after both decisions land. Both figures
are measured over the pinned tree with the two keys filtered out of the
field list, which is the state D-06 reaches. T048 maps `AnyThread`,
`PortMask`, and `FCMask` through to `any`, `ch_mask`, and `fc_mask`.
The seam pins measured after that map are skylake 587 and
sapphirerapids 2222 against the synthetic list. The reference-host
list publishes none of the three formats, and its pins stay at 581
and 1993.

**D-05 does not move the encodable count.** The 570 rows carrying a
non-zero register value are counted encodable at the audit point,
because the parser drops both keys and the row encodes on its base fields
alone. D-05 changes what those rows count. The number of encodable rows
stays where it is. What D-05 does move is that 44 rows naming load latency
and 78 rows naming the frontend would flip to `not_encodable` against the
fixture as it stands, because the resolved format is absent. Growing the
fixture with the two entries above keeps them encodable and makes the count
correct.

Every figure under "measured" was confirmed by the seam test, and the two
that moved were corrected in this file. The test is where the number is
enforced, so the correction belongs beside the prediction. A directory
whose measured figure differs sends the correction back to review, because
a prediction that does not hold means the measurement or the decision is
wrong. Two did differ, and
both corrections are recorded above.

The AMD figures hold because neither decision touches an AMD row: no AMD
row carries a register index, and no AMD row carries either
no-obligation key.

The reference host's own list at
`counters_linux_pmu_seam_test.cpp:797-820` publishes `event`, `umask`,
`cmask`, `edge`, and `inv`, and this host publishes no uncore or RAPL
device at all. It therefore publishes no `offcore_rsp`, no `ldlat`, and
no `frontend`, so every one of the 570 rows publishing as countable today
on an AMD core publishes `not_encodable` there under D-07. That is a
measured state, and it is why the two lists are pinned separately.

## Version lineage

| Field | At the audit point | After this feature |
| --- | --- | --- |
| project version | 0.3.0 | 0.4.0 |
| shared-object version | 0 | 1, a hand-kept ABI number |
| package config compatibility | the major-version compatibility | the minor-version compatibility |
| `operator*(const expression&, const double)` | removed, unrecorded | removed, recorded in the lineage |
| `concept provider` | removed, unrecorded | removed, recorded in the lineage |
| `metric_result` | three fields | four fields |
| `points_view` | six fields | seven fields |

**Rules**: the release takes the minor position because the added field is
additive and the removals are breaking. `SOVERSION` is a hand-kept ABI
number. The maintainer bumps it when a public signature or a public
record layout changes. Neither declaration is restored.

## Successor-log entry

One entry per correction in
`specs/007-counters-and-timers/citations-log.md`, in the format the log
already uses.

| Field | Content |
| --- | --- |
| the corrected claim | the figure or the statement at the audit point |
| the correction | what the correction changes it to |
| the requirement | the 007 or 012 identifier it restores |
| the evidence | the file and line, or the measured count, at the audit point |

**Rules**: the frozen 012 record takes no edit. Spec 012's D-02, which
stated the kernel header names no scaling rule, is corrected against this
feature's requirement, because the header names one.
