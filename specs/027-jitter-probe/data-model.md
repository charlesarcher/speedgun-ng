# Data Model: Jitter Probe

Phase 1 output. The public surface is three names in
`include/speedgun-ng/benchmark.hpp`: `JitterProbeOptions`, `JitterProbeMode`,
and `JitterStats`, plus the two entry points plan.md lists under "Public API
surface added" (FR-015). Every other type here is harness-internal behind
`source/harness/detail/jitter_probe.hpp` (R-01). JSON key names are camelCase,
the style `docs/schema/report-v1.json` carries (`noiseWarning`,
`runnableCpus`, `overheadFloorNs`); enum words in JSON are the SCREAMING_CASE
the schema's `waitMode` and `outcome` enums carry. The key order of every
JSON object below is the `x-order` the schema rows carry.

## Entity map

```text
JitterProbeOptions ── the one argument of every public entry point (FR-003)
      │
      └── mode: JitterProbeMode, three closed values, default ALONGSIDE

JitterProbe (harness-internal holder) 1 ──── * ProbeCpuState (one per probe cpu)
                                                  │
                                                  ├── buffer of GapRecord,
                                                  │   pre-sized gapCount, 16 bytes each
                                                  └── stop flag on its own cache line

ProbeCpuState ── kept records ──▶ JitterStats (per probe cpu, FR-002)
                                      ▲
TimedWindow (TSC [start, end) per     │
repetition, chunk, pass) ── split ────┘ per-benchmark JitterStats
                                      │
                                      ▼
console block, per-benchmark line, root `jitter` object,
per-entry `jitter` object, `jitter:` rule names (FR-023, FR-024, FR-026, FR-027, FR-030)
```

## E-01: `JitterProbeOptions`

The one argument of every public entry point (FR-003, FR-015). Every field
carries a default member initializer (R-15); no entry point takes a bool.

| Field | Type | Default | Rule |
| --- | --- | --- | --- |
| `cpus` | ordered vector of `std::uint32_t` | empty | The probe cpu request. An empty vector names the `idle` preset (FR-016); a non-empty vector names the probe cpus in the order given. The option value parses a cpu list or the name `idle` into this field (FR-011). A bad cpu or an unknown preset name prints one line and exits with status 2 (FR-038) |
| `thresholdNs` | `std::optional<std::uint64_t>` | no value | The gap threshold in ns. `threshold=<ns>` sets it; a value of 0, a negative value, or a value that is not a number exits with status 2 (FR-008); the negative value is a rule of the option grammar, where the string carries the sign, because the field's value type is unsigned. The default rule applies when the field names no value: the 200 ns default rises to 10 times the measured minimum loop period when that stands larger (FR-007, D-2); a value stands as given, an explicit 200 ns staying exactly 200 ns |
| `mode` | `JitterProbeMode` | `ALONGSIDE` | The default when the option names no `mode=` value (FR-012, D-1) |
| `gapCount` | `std::uint32_t` | `kDefaultGapCount` = 65536 | The per-cpu buffer size in records. `gaps=<n>` sets it; the range runs from 1 (`kMinGapCount`) to 16,777,216 (`kMaxGapCount`), and a value outside the range exits with status 2 (FR-009) |
| `precheckMs` | `std::uint32_t` | `kDefaultPrecheckMs` = 200 | The pre-check duration in ms; `precheck=<ms>` sets it (FR-014, D-13). A value that is not a number exits with status 2 (FR-038) |
| `strict` | `bool` | false | `,strict` on the option value. When the TSC stands unavailable, strict exits with status 2 and the non-strict run continues with status 0 (FR-037, D-10) |

Validation: the option parses once in `speedgunMain`, before the expansion
and before any run (R-06), so every rejection costs no measured time. The
cpu list is a request the probe resolves against the plans and the layout at
start; the `idle` resolution is the pure function `resolveIdleCpus` beside
`resolvePreset` (R-07), and it holds the rules of FR-016 and FR-017.

## E-02: `JitterProbeMode`

An enumeration with three closed values (R-10).

| Value | JSON word | Meaning |
| --- | --- | --- |
| `ALONGSIDE` | `"ALONGSIDE"` | The probe spans the batch, from before the first benchmark to after the last (FR-006); the overlap check of FR-018 binds this phase |
| `PRECHECK` | `"PRECHECK"` | The probe runs only before the first benchmark, on all chosen cpus at the same time, for `precheckMs` (FR-014); the probe may use the benchmark cpus here (D-3) |
| `BOTH` | `"BOTH"` | The pre-check phase and the alongside phase, over one probe cpu set and one buffer set per cpu (R-10) |

An unknown mode word prints one line and exits with status 2 (FR-038).

## E-03: `JitterStats`

The result of one probe cpu over one set of covered windows: the 16 fields
of FR-002 in the sysjitter order (FR-002, R-05). This table is the
canonical enumeration of the field names and their order; the other
artifacts state the E-03 order and quote no second list. The JSON stats
object carries the same names, with the probe cpu index in a leading `cpu` key.

| Field | Type | Unit | Rule |
| --- | --- | --- | --- |
| `gapCount` | `std::uint64_t` | count | The kept gaps of the covered windows (FR-002) |
| `gapsPerSecond` | `double` | 1/s | `gapCount` over the seconds of the covered windows' span: the probe's `runtimeNs` at run level, the benchmark's window total per benchmark, the rule the `totalPercent` row carries (R-05) |
| `minNs` | `std::uint64_t` | ns | The smallest kept gap (FR-002) |
| `medianNs` | `std::uint64_t` | ns | The nearest-rank median over the sorted kept gaps (PC-4) |
| `meanNs` | `double` | ns | The mean of the kept gaps (FR-002) |
| `p90Ns` | `std::uint64_t` | ns | Nearest rank (PC-4) |
| `p99Ns` | `std::uint64_t` | ns | Nearest rank; the field the N5 rule names address (FR-030, D-11) |
| `p999Ns` | `std::uint64_t` | ns | Nearest rank (PC-4) |
| `p9999Ns` | `std::uint64_t` | ns | Nearest rank (PC-4) |
| `maxNs` | `std::uint64_t` | ns | The largest kept gap (FR-002) |
| `totalNs` | `std::uint64_t` | ns | The sum of the kept gaps (FR-002) |
| `totalPercent` | `double` | percent | `totalNs` over the covered duration; the probe's `runtimeNs` at run level, the benchmark's window total per benchmark (R-05) |
| `runtimeNs` | `std::uint64_t` | ns | The span from the loop start to the read of the stop flag, through the same `machine/tsc` reader (R-05) |
| `thresholdNs` | `std::uint64_t` | ns | The effective threshold the run used (FR-007) |
| `loopPeriodNs` | `std::uint64_t` | ns | The minimum loop period of the measurement pass E-05 specifies (FR-007, R-05) |
| `droppedGaps` | `std::uint64_t` | count | The gaps a full buffer dropped; zero when the buffer never filled (FR-010) |

Validation: the percentile fields follow the nearest-rank rule of
`source/harness/percentiles.cpp` over the sorted kept gaps, the rule the
report columns fill through (PC-4, R-05). The fold of an empty kept set
leaves every field at 0. The buffer-full path keeps counting gaps, their
total and their max, and drops the time stamps; the percentiles and the
window split read only the kept gaps (FR-010, D-6).

## E-04: `GapRecord`

One kept gap, 16 bytes, two `std::uint64_t` fields (D-6, R-03). The type is
harness-internal; nothing here is public API (FR-015).

| Field | Type | Rule |
| --- | --- | --- |
| `stamp` | `std::uint64_t` | The TSC count of the read that saw the gap, in the `machine/tsc` time base (R-04) |
| `gapNs` | `std::uint64_t` | The gap above the threshold, in ns (FR-001) |

Validation: the record carries no capacity check beyond the fill test; the
loop stores into a pre-sized buffer (FR-005, R-03). A record whose buffer
filled is dropped: the stamp is lost, and the count, the total and the max
of the dropped gaps stand in the per-cpu counters of E-05 (FR-010).

## E-05: `ProbeCpuState`

The per-cpu probe state the harness-internal holder owns, one instance per
probe cpu (R-01, R-03).

| Field | Type | Rule |
| --- | --- | --- |
| `cpu` | `std::uint32_t` | The pinned cpu index; the context pins through `PlacementSlot::cpu(n)` and the default placement function (PC-1, R-01) |
| `buffer` | pre-sized vector of `GapRecord`, `gapCount` entries | Allocated and first touched on the probe cpu, inside the probe thread's work item, before the window opens (FR-009, D-6) |
| `kept` | count | The number of records written; the kept prefix the fold and the split read (FR-010) |
| `droppedGaps` | `std::uint64_t` | Set when the buffer fills; counts the gaps after the fill (FR-010) |
| `droppedTotalNs` | `std::uint64_t`, ns | The total of the dropped gaps (FR-010) |
| `droppedMaxNs` | `std::uint64_t`, ns | The max of the dropped gaps (FR-010) |
| `stop` | `std::atomic<bool>` in an `alignas` block of its own cache line | Read once per loop pass by the probe, written once by the runner at the stop; the only shared word the loop reads (FR-005, R-04) |
| `loopPeriodNs` | `std::uint64_t`, ns | The minimum loop period of the measurement pass before the window (FR-007, R-05): the pass runs on the same probe cpu before the window opens and takes a fixed 1,024 loop reads the loop discards, and its minimum period sets the 10x threshold floor; the fixed count keeps the pass scripted beside the window (FR-039) |
| `thresholdNs` | `std::uint64_t`, ns | The effective threshold: the default rises to 10 times `loopPeriodNs` when that stands larger; an explicit `threshold=<ns>` stands as given (FR-007, FR-008) |

Validation: one producer and one consumer over each buffer; the runner reads
the buffer only after the probe context is joined, the happens-before edge
R-04 records. The pre-check phase and the alongside phase share this one
state per cpu (R-10); the kept records of both phases fold into the
run-level stats, and the split places each record by its stamp, so a
pre-check record falls inside no benchmark window.

## E-06: `TimedWindow` and the split

The window record the runner stamps, and the fold that splits the kept gaps
into benchmarks (FR-021, FR-022, R-04).

| Field | Type | Rule |
| --- | --- | --- |
| `start` | `std::uint64_t`, TSC counts | Sampled on the benchmark thread at the window open, from `machine/tsc` (R-04) |
| `end` | `std::uint64_t`, TSC counts | Sampled at the window close; the window is the half-open `[start, end)` (R-04) |
| `entry` | index into the batch's benchmark entries | The entry the window belongs to (FR-021) |
| `pass` | repetition, chunk and pass indexes | A multi-pass run gives each pass its own windows, and the split covers all of them (FR-022, PC-5) |

Validation: the window edges and the gap stamps come from the same leaf in
one time base, so the comparison needs no conversion (R-04). A kept gap
folds into every window whose `[start, end)` contains its stamp. The result
is one `JitterStats` per benchmark entry per probe cpu, the value the
function `gapStatsOf` returns (R-15); a probe cpu whose kept gaps landed
inside no window of a benchmark stands absent from that benchmark's stats
(User Story 3, FR-021).

## E-07: the run-level `jitter` JSON object

The root object of FR-026, placed after `speedgun` and before `benchmarks`
in the document's root `x-order` (R-09). The key stands absent when the run
carries no probe, so the JSON carries the same fields and shape as
master's (FR-013, D-1).

| Key | Kind | Rule |
| --- | --- | --- |
| `mode` | string, one of `"ALONGSIDE"`, `"PRECHECK"`, `"BOTH"` | The mode the run executed (FR-012) |
| `thresholdNs` | integer | The effective threshold in ns (FR-007) |
| `loopPeriodNs` | integer | The measured minimum loop period in ns (FR-007) |
| `cpus` | array of integers, ascending | The probe cpus; the array sorts ascending where the option value preserves the order given (FR-026) |
| `stats` | array of stats objects | One object per probe cpu, each carrying `cpu` and the 16 fields of E-03 in the E-03 order (FR-002, FR-026) |
| `droppedGaps` | integer | The sum of the per-cpu `droppedGaps`; the per-cpu values stand in `stats` (FR-010, FR-026) |
| `disclosure` | string | The disclosure text of E-09 (FR-025, FR-026) |

Validation: when the TSC stands unavailable, the object still appears:
`mode` carries the requested mode, `thresholdNs` the configured
threshold (the 10x loop-period floor stands unmeasured, so no rise
applies), `loopPeriodNs` 0 because the measurement pass never ran,
`cpus` and `stats` empty arrays, `droppedGaps` 0, and the `disclosure`
text carries the `not available` answer; the run continues with
status 0, and `strict` turns it into status 2 (FR-037, D-10).

## E-08: the per-entry `jitter` JSON object

The nullable object each benchmark entry carries, placed after
`noiseWarning` in the entry `x-order` (R-09, FR-027).

| Key | Kind | Rule |
| --- | --- | --- |
| `stats` | array of stats objects | One object per probe cpu whose kept gaps landed inside that benchmark's windows, each carrying `cpu` and the 16 fields of E-03 (FR-021, FR-027) |

Validation: the key stands absent when the run carries no probe, so the
document carries the same fields and shape as master's (FR-013, D-1). An
entry whose
windows caught no gap carries `null` (User Story 3).
The reader takes the optional-key pattern `contextPercentiles` and `raw`
use, reads with the DOM error-code form, and one test covers each error
code; the round-trip test reads the written report back with equal values
(FR-029, SC-001). A `jitter:` compare rule evaluated on a report whose
entries carry no probe stats prints one line and exits with status 2
(FR-032, D-11).

## E-09: the disclosure record

The run-level disclosure the report prints and the JSON `disclosure` text
carries (FR-025, FR-026). This table is the canonical enumeration of the
disclosure content; FR-025 states the duty, and the artifacts that print
or carry the text reference it. The console lines stand in the end-of-report
region `printNoiseWarning` owns and replace nothing: the noise-warning
condition and its three fields keep their bytes (PC-6, R-08).

| Field | Type | Rule |
| --- | --- | --- |
| `available` | `bool` | False when the `machine/tsc` leaf is missing or `ClockProvider::open` returns a null reader; the line reads `not available` (FR-037, Q-1) |
| `cpus` | ordered list of `std::uint32_t` | The probe cpus in ascending cpu index, the one order the console lines and the `disclosure` text carry; the option value preserves the order given, and the JSON array of E-07 sorts ascending the same way (FR-025, E-07) |
| `loopPeriodNs`, `thresholdNs` | `std::uint64_t`, ns | The disclosed measurement points (FR-007, FR-025) |
| `droppedGaps` | per-cpu counts | The condition the report states when a buffer filled (FR-010, FR-025) |
| `skippedGroups` | list of L3 group ordinals | The groups the `idle` resolution skipped and disclosed (FR-017, D-4) |
| `maskNarrowed` | `bool` | The affinity-mask change of an unpinned benchmark, disclosed with the benchmark; an empty mask exits with status 2 (FR-019, D-5) |
| `keptGapsOnly` | `bool` | The statement that the split and the percentiles use only the kept gaps (FR-010, FR-025) |
| `cpusetLimited` | `bool` | The disclosure that cpusets limit the probe cpus (FR-036, D-12) |

Validation: the console prints each item as a new line in the noise-warning
region; the JSON `disclosure` text carries the same content as one string
(FR-026). The disclosure reads no clock (R-08).

## E-10: the raw gap list

The optional `gaps` array of each per-cpu stats object, present only
when `--json-raw=gaps` stands given; the value joins `chunks` and
`none`, the closed value list the N3 option carries (FR-028, R-09).

| Key | Kind | Rule |
| --- | --- | --- |
| `gaps` | array of record objects | The kept records of the probe cpu, in stamp order; one record is one object `{"stamp": integer, "gapNs": integer}`, the two fields of E-04 (D-6, FR-028) |

Validation: the list holds the kept records only; a dropped gap appears in
no list, and its count, total and max stand in `droppedGaps` and the
per-cpu counters of E-05 (FR-010). The size bounds at 16,777,216 records
per cpu, the reason the list rides an explicit option value (FR-009,
FR-028).

## State transitions

```text
options:  absent ──--jitter-probe or an entry point──▶ parsed
          (parsed once, before the expansion and before any run, R-06)
parsed ── catalog leaf + open() over machine/tsc ──▶ available | unavailable
  available:   idle ──precheck phase (PRECHECK or BOTH, precheckMs)──▶ running
               running ──alongside phase (ALONGSIDE or BOTH)──▶ running
               running ──stop after the last benchmark──▶ stopped
               stopped ──join edge──▶ folded ──▶ reported (FR-006, D-7)
  unavailable: strict ──▶ status 2 (FR-037)
               otherwise ──▶ disclosed "not available", run continues, status 0
buffer:   filling ──gapCount records kept──▶ full
          (full: stamps dropped, counts, total and max keep,
           droppedGaps set, FR-010, D-6)
```

The overlap check of FR-018 binds the alongside phase only; the pre-check
phase may use the benchmark cpus, because it stops before they start (D-3).
No buffer and no state mutates inside a timed window: the probe starts
before the first benchmark and stops after the last, and the fold and the
percentiles run after the stop (FR-006, D-7).

## What is deliberately absent

- No gap list in the JSON by default; it rides `--json-raw=gaps` alone
  (FR-028).
- No new `Availability` enumerator and no CPUID check; the catalog and the
  null reader carry the availability answer (Q-1, R-11).
- No root privilege and no kernel setting change (D-12, FR-036).
- No per-gap heap allocation and no growable container behind the buffer;
  the loop performs no allocation, no syscall and no I/O (FR-005, R-03).
- No second schema version; the jitter keys ride version 1 of the document
  with the schema rows moved in the same commit (R-09).
- No short option letter; the long-option value stands at 256 or above,
  held as `kOptJitterProbe` (FR-011, R-06).
- No noise attribution on the measured cpus; N13b owns it (D-9).
- No public type beyond the three names and the two entry points; the
  holder, the gap record, the windows and the disclosure live behind
  `source/harness/detail/jitter_probe.hpp` (FR-015, R-01).
