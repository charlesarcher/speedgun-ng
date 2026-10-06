# Data model: counters defect resolution

The entities below are the values this feature corrects, extends, or
introduces. Field names follow the shipped code, and every entity names
the requirement that governs it. Validation rules state the check a
caller or a test can observe.

## Managed column layout

A compiled plan records `leaf_count` managed columns of `stride` rows.
Every sampling action writes one point into each managed column. The
ratio pair occupies two ordinary columns today, and this feature adds a
third ordinary column for the disclosure.

| Column index | Content | Written by | Requirement |
| --- | --- | --- | --- |
| `n` | cumulative 64-bit count of member leaf `n` | the window's read thunk | FR-004 |
| `ratio_enabled` | `time_enabled` of the pair that leaf `n` belongs to | the same thunk | FR-005 |
| `ratio_running` | `time_running` of that pair | the same thunk | FR-005 |
| `disclosure` | the countability state of that sampling action | the same thunk, through `point_sink::put` | FR-007 |

The ratio columns are linked by `link_ratio_slots` at
`source/counters/plan.cpp:135-158`, which resolves each slot's pair once
and leaves both at `no_ratio_slot` when the leaf carries no pair. The
disclosure column is resolved the same way, from the entry the slot
resolved to. A slot without a pair still writes its count and its
disclosure, so no action leaves a managed column unwritten.

**Validation**: `point_sink::check_action` asserts one write per managed
column at the row commit. A read path that skips a write fails that
assertion in every configuration that emits contract code.

## Availability state

`sg::counters::availability` at
`include/speedgun-ng/counters_core.hpp:80`. One enumeration holds the
countability state. This feature adds one value so a refusal that comes
from the entry's scope reads differently from a refusal that comes from
its encoding.

| Enumerator | Present | Meaning |
| --- | --- | --- |
| `countable` | yes | the entry can be counted on the targets its bitmask names |
| `permission_blocked` | yes | the kernel refused the event for want of permission |
| `not_encodable` | yes | the running kernel's formats lack a field the row needs |
| `absent` | yes | the host publishes no such event |
| `scope_refused` | new | the entry's own scope refuses this target kind |

**Validation**: a row publishes `not_encodable` only where the running
kernel's formats lack a field the row needs (FR-018). A row whose fields
the kernel publishes never publishes it. An entry publishes
`scope_refused` for a target kind its scope refuses, whatever its
encoding says (FR-021).

## Supported target kinds

A fixed-size bitmask over `sg::counters::target_kind`. It travels
beside the countability state on `catalog_entry`.

| Bit | Target kind | Meaning |
| --- | --- | --- |
| 0 | `target_kind::thread` | a per-task event, bound to the calling thread |
| 1 | `target_kind::cpu` | a cpu-targeted event, bound to one processor |

The mask is a fixed-size unsigned integer type. It allocates no memory,
so a caller reads the supported targets without a container (FR-021).
A new kernel target kind takes the next free bit. No enumerator value
changes and no stored bit moves.

**Validation**: a bit is set only where a probe over that target kind
returned a verdict other than a scope refusal. An entry with an empty
mask is unresolvable for every target, and a compile over it reports its
own error.

## catalog_entry

`include/speedgun-ng/counters_core.hpp:217`. Two fields change shape in
this feature; every other field keeps its meaning.

| Field | Type | Change | Requirement |
| --- | --- | --- | --- |
| `name` | `std::string_view` | unchanged | |
| `description` | `std::string_view` | unchanged | |
| `unit` | `sg::counters::unit` | unchanged | |
| `avail` | `availability` | gains the `scope_refused` value | FR-021 |
| `mode` | `read_mode` | published per entry, from a probe over the entry's own formats | FR-001 |
| `frequency_hz` | `std::uint64_t` | unchanged | |
| `scaled` | `bool` | unchanged | |
| `targets` | target-kind bitmask | new field beside `avail` | FR-021 |

`read_mode` is set on an entry only where the fast read can succeed for
that entry. A host-wide capability verdict does not set it on an entry
whose event the fast instruction cannot read (FR-001).

**Validation**: the strings stay immutable once the system is open.
Reading an entry from any number of threads at once is safe after open
(FR-010).

## Fast-path point

The value one sampling action hands to the sink for one managed leaf.

| Property | Value | Requirement |
| --- | --- | --- |
| Width | 64 bits, cumulative | FR-004 |
| Decode | sign extend the instruction's value from the published `pmc_width`, then add the page's `offset` | FR-004 |
| Mask | none; the point carries no width mask | FR-004 |
| On a refused, unstable, or failed read | zero, with the action marked in the disclosure column | FR-002, FR-003, FR-006, FR-007 |
| On a retry that also fails | zero; no earlier value survives | FR-003 |
| Delta across two points | modular over 64 bits, so a crossing of the published counter width stays correct | FR-004 |

The kernel's own interface header states the decode at
`include/uapi/linux/perf_event.h:654-663`, the read order at `:621-630`,
and the offset at `:637`.

## Enabled and running pair

Two ordinary columns carrying `time_enabled` and `time_running` as the
page holds them. The extrapolation is applied where the ratio is
computed, in `leaf_ratio` at `source/counters/fold.cpp:126-130`, which
divides the running delta by the enabled delta.

A pair read that discloses no stable value writes a zero pair and marks
the action in the disclosure column. A fold then knows the ratio it
reads was not measured, and a caller reading the disclosure column knows
the same (FR-005).

## Event table row

One event selector as the parser reads it from a table file.

| Field | Type | Meaning | Change |
| --- | --- | --- | --- |
| `name` | `std::string` | the event name the table gives | unchanged |
| `fields` | ordered key/value pairs | the encoding fields the row needs | rule corrected |
| `description` | `std::string` | the first description key the row carries | unchanged |
| `unit` | `std::string` | the table's scope label | now routes the row to a device |
| `scope` | device scope | the device class the row belongs to | new |
| `encodable` | `bool` | every field reached a published kernel format | unchanged, now reachable for Intel rows |

The rule that fills `fields` changes. A numeric key becomes an encoding
field only where the key names a format the running device publishes.
A sampling key or a metadata key carries no encoding obligation
(FR-016). The kernel's own spellings `cmask`, `inv`, `edge`, and
`offcore_rsp` map onto `CounterMask`, `Invert`, `EdgeDetect`, and
`OffcoreRsp` (FR-017).

A metric definition row, one carrying `MetricExpr` or `MetricName`,
is catalog data for a later specification and is never a countable. That
rule stands.

**Validation**: a row whose fields the kernel publishes encodes. A row
that needs a field the running kernel does not publish reports
`not_encodable` and never a fabricated config word (FR-018).

## Device placement

| Table scope | Device that receives the row |
| --- | --- |
| core | each core device the scope applies to, which on a hybrid host is every core and atom device the scope covers |
| uncore | the uncore device of the matching class |
| a device the host does not publish | none; the row stays out of the catalog |

No uncore row appears under the core device, and a row scoped to an
absent device runs no probe (FR-019).

## Per-plan overhead floor

The published minimum, median, and maximum nanoseconds one sampling
action costs, computed by `calibrate` at
`source/counters/plan.cpp:267-296`.

| Property | Value |
| --- | --- |
| Bracket | two clock reads taken around the sampling action |
| Bracket cost | measured under the identical bracketing and subtracted |
| Published figure | the sampling action's cost with the bracket excluded (FR-025) |
| Floor at zero | reachable on a host whose clock costs more than the action; the figure then names the condition |
| Coverage | no exclusion region hides the calibration from the registered test (FR-026) |

## Embedded table data

The vendored JSON compiled into static data inside the archive.

| Property | Value |
| --- | --- |
| Form | raw JSON bytes in static data (FR-023) |
| Source | `external/pmu-events`, pinned by its `RECORD` manifest; this feature re-pins nothing |
| Decoder | the existing simdjson parse path at `source/counters/linux_pmu/table_parse.cpp` |
| Run-time lookup | none; no configured data path is added |
| Build option | none; the embedding is unconditional |
| Size cost | measured on the reference host and recorded in `quickstart.md` step 10 |

## Measured encodable-row counts

SC-005 and FR-020 gate four Intel directories against exact numbers,
measured over the pinned tree against a named synthetic sysfs format
list. The counts below are the baseline this implementation measures and
the fixture pins. The rightmost column stays empty until the fixture
supplies the numbers, and the fixture fails while it is empty.

| Directory | Family | Rows in the tree | Encodable, synthetic list | Encodable, reference host formats |
| --- | --- | --- | --- | --- |
| `skylake` | Intel | 587 | 576, pinned by fixture | 0 |
| `icelake` | Intel | 346 | 342, pinned by fixture | 0 |
| `alderlake` | Intel | 563 | 521, pinned by fixture | 0 |
| `sapphirerapids` | Intel | 2693 | 1965, pinned by fixture | 0 |
| `amdzen4` | AMD | 502 | 480, must not fall below 339 | 480 |
| `amdzen5` | AMD | 579 | 557, must not fall below 348 | 557 |

The two named format lists are the ones the fixture supplies:

- the synthetic list is `cmask`, `config`, `config1`, `config2`, `edge`,
  `event`, `inv`, `umask`, the union of the core format spellings the
  pinned Intel and AMD tables reach;
- the reference host list is what the reference host's own
  `/sys/bus/event_source/devices/cpu/format/` publishes, which is
  `cmask`, `edge`, `event`, `inv`, `umask`.

A row is encodable where every numeric key in it either reaches a format
the list publishes, under the kernel spelling the row's key maps to
(FR-017), or carries no encoding obligation (FR-016). A row that needs a
field the list does not publish is `not_encodable` (FR-018).

The pre-fix figures, taken at `6aafd2d` against the same list, are 339
for `amdzen4` and 348 for `amdzen5`, and 0 for every Intel directory.
The Intel count of zero is defect I-01: the pre-fix parser records the
sampling key `SampleAfterValue` as an encoding field under its own name,
so no Intel row reaches a published format and none encodes.

The row counts in the second column count the rows the table parser
yields, which excludes the metric-definition rows the parser drops. The
counts recorded in `research.md` while writing it, 813 for `skylake`,
895 for `alderlake`, and 577 for `amdzen4`, count every JSON object in
the tree, so they are larger. The fixture pins the parser's count,
because the parser's count is what the catalog sees.

## Clock leaf order guarantee

One documented ordering per leaf. The class contract states no order on
a leaf's behalf.

| Leaf | Clock | Documented guarantee | Test |
| --- | --- | --- | --- |
| `machine/monotonic` | `CLOCK_MONOTONIC` through the vDSO | non-decreasing on one thread, across a process's life | a new thread's sample does not fall below an earlier sample on one thread |
| `machine/monotonic_raw` | the same clock, unadjusted | as `monotonic` | the same test, both leaves |
| `machine/thread_cpu` | per-thread CPU time | a new thread's first sample can fall below an earlier sample taken on another thread (FR-030) | a new thread's sample falls below an earlier sample on another thread |
| `machine/process_cpu` | per-process CPU time | non-decreasing across the process's threads, because the counter is the process's | a sample taken on a second thread does not fall below one taken on the first |
| `machine/tsc` | the processor's cycle counter | ordered only when one thread takes both window endpoints (FR-031); no fence, no cross-thread order | the precondition holds on one thread, and the leaf is read with no fence |

Every guarantee a leaf documents has a test that exercises it (FR-029).

## Defect record

One entry per issue, held in the specification and closed by this
implementation.

| Field | Value |
| --- | --- |
| Identifier | `I-01` through `I-10` |
| Priority | P1 for I-04, I-05, I-06, I-08; P2 for the rest |
| Verdict | confirmed, or confirmed with a suspected part, per `research.md` |
| Source sites | the lines `research.md` cites |
| Restored requirement | the 007, 008, or 011 requirement the correction restores |
| Successor-log entry | one entry per correction in `specs/007-counters-and-timers/citations-log.md` |

## Successor-log entry

One dated entry per correction, in the format
`specs/007-counters-and-timers/citations-log.md` defines.

| Field | Content |
| --- | --- |
| `date` | the day the entry was written |
| `task` | the `T` task in `tasks.md` that produced the correction |
| `section` | the heading of the frozen record section the entry corrects |
| `figure_as_written` | the figure or anchor the record holds |
| `figure_measured` | the figure the command returned |
| `command` | the command that measured it |
| `head` | the commit the command ran against |
| `must_not_move` | the lines, tables, closed task lines, and preambles the correction left alone |

The frozen record at `specs/007-counters-and-timers/citations.md` takes
no edit (FR-038).
