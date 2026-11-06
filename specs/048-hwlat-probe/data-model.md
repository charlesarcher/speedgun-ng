# Data Model: Hardware Latency Probe with SMI Attribution

Phase 1 output of `/speckit.plan` for `specs/048-hwlat-probe/spec.md`.
The entities below extend the standing probe data model
(`specs/027-jitter-probe/data-model.md`), the pre-check data model
(`specs/034-host-noise-precheck/data-model.md`), the tracer data model
(`specs/036-osnoise-timerlat-tracers/data-model.md`) and the series data
model (`specs/043-fwq-ftq/data-model.md`). Nothing there changes shape.
Each entity names its fields, their types, their units, the condition
under which a field holds a value, the document key each field maps to,
and its owner.

Entity ids here start at E-01, the way
`specs/044-fix-rocky-pmu-stack/data-model.md` and
`specs/047-loader-cross-field/data-model.md` start. One id names one
record in the file that carries it.
`specs/048-hwlat-probe/plan.md:121` states this file's range as E-01 to
E-08, and this file holds that range.

One word carries two surfaces (spec D-6). `hwlat` names a method of
`--jitter-probe` in one grammar (`source/harness/jitter_probe.cpp:1665`)
and an engine of `--os-noise` in the other
(`source/harness/noise_tracer.cpp:48`). No console line, no key and no
test mixes the two.

JSON key names stay camelCase, the style of
`docs/schema/report-v1.json:166-241`. The method word stands upper case
in the document, the shape of `kJitterMethodWords`
(`source/harness/detail/json_report.hpp:89-93`), and lower case on the
console, the rule specs/043 FR-019 states. The engine word stands lower
case in both, the shape of `kTracerEngineWords` at
`source/harness/detail/json_report.hpp:103-107`. The key order of every
object below is the `x-order` the schema row carries, and the order the
writer's lines keep governs it (spec FR-029). In each table below, the
type names stand short: `include/speedgun-ng/benchmark.hpp` spells each
one with its `std::` qualifier.

Every cite reads the working tree of branch `048-hwlat-probe`, cut from
master `858c55d9`, read 2026-11-05. Kernel cites read the copies of the
upstream files the lane keeps outside the repository, and name them
`kernel/trace/<file>:<line>`.

## Entity map

```text
JitterMethod (public enum, +1 value) .................. E-01
  └─ TSC | FWQ | FTQ | HWLAT
TracerEngine (public enum, +1 value) .................. E-01
  └─ NONE | OSNOISE | TIMERLAT | HWLAT
JitterStats (public, +3 fields) ....................... E-02
  ├─ smiDelta: optional<uint64_t>       count
  ├─ nmiDelta: optional<uint64_t>       count
  └─ smiGapCount: uint64_t              count
GapRecord (internal, unchanged, 16 bytes) ............. E-03
  └─ stamp, gapNs
ProbeCpuState (internal, +1 ring) ..................... E-03
  └─ smiChanges: vector<uint64_t>, one span change per gap
PrecheckTracerRecord (public, +3 fields) .............. E-04
  └─ windowUs, widthUs, thresholdUs     microseconds
PrecheckTracerCpuRecord (public, +3 fields) ........... E-04
  └─ maxGapNs ns, gapCount, nmiCount    counts
JitterReportCpu (internal, +1 list) ................... E-05
  └─ smiChanges: vector<uint64_t>, one per kept gap
JitterReportRecord (internal, unchanged) .............. E-05
The report keys (the schema governs) .................. E-06
NoiseTracerSession (internal, + the saved set) ........ E-07
  └─ saved: vector<TracerSetting>, six {path, value} pairs
The comparison names .................................. E-08
  ├─ 3 names in kJitterFields, 32 rows to 35
  └─ 3 names behind the new `hwlat:` head word
```

## E-01: `JitterMethod` and `TracerEngine`

The two closed sets that gain one value each. One run carries one
method and one engine (specs/027 FR-001, specs/036 FR-002), and the two
words answer different grammars (spec D-6).

| Enum | Value | Console word | JSON word | Set by |
| --- | --- | --- | --- | --- |
| `JitterMethod` | `TSC` | `tsc` | `"TSC"` | no `method` key |
| `JitterMethod` | `FWQ` | `fwq` | `"FWQ"` | `method=fwq` |
| `JitterMethod` | `FTQ` | `ftq` | `"FTQ"` | `method=ftq` |
| `JitterMethod` | `HWLAT` | `hwlat` | `"HWLAT"` | `method=hwlat` |
| `TracerEngine` | `NONE` | none | none, no key | no `--os-noise` |
| `TracerEngine` | `OSNOISE` | `osnoise` | `"osnoise"` | `osnoise` |
| `TracerEngine` | `TIMERLAT` | `timerlat` | `"timerlat"` | `timerlat` |
| `TracerEngine` | `HWLAT` | `hwlat` | `"hwlat"` | `hwlat` |

- Meaning: `HWLAT` of the method names the standing gap loop with the
  two counter reads (spec FR-002, FR-007, FR-009). `HWLAT` of the
  engine names the kernel's hardware-latency detector (spec FR-018).
  `NONE` names no tracer and writes no key (specs/036 FR-015).
- Fields: `method` at
  `include/speedgun-ng/benchmark.hpp:2525` and `tracerEngine` at
  `include/speedgun-ng/benchmark.hpp:2523`, both members of
  `JitterProbeOptions` at `include/speedgun-ng/benchmark.hpp:2514`.
- JSON key mapping: `jitter.method` for the method, a string in the
  closed list of `docs/schema/report-v1.json:75-78`, and
  `precheck.tracer.engine` for the engine, a string in the closed list
  of `docs/schema/report-v1.json:371`. `HWLAT` joins both lists. The
  writer writes no method key for `TSC`
  (`source/harness/json_report.cpp:1183-1187`), and the tracer object
  stands only when a tracer ran
  (`source/harness/json_report.cpp:1325`).
- Carrier and file: `JitterMethod` at
  `include/speedgun-ng/benchmark.hpp:551-556` and `TracerEngine` at
  `include/speedgun-ng/benchmark.hpp:2458-2463`. `HWLAT` stands last in
  each list, at index 3 in each.
- The word tables, their words and their pins, quoted at the current
  lines:
  - `kJitterMethodWords` stands at
    `source/harness/detail/json_report.hpp:89-93` with `TSC`, `FWQ`,
    `FTQ`, and the `static_assert` at
    `source/harness/detail/json_report.hpp:94-95` ties its size to
    `sg::JitterMethod::FTQ`.
  - `kTracerEngineWords` stands at
    `source/harness/detail/json_report.hpp:103-107` with `NONE`,
    `osnoise`, `timerlat`, and the `static_assert` at
    `source/harness/detail/json_report.hpp:108-109` ties its size to
    `sg::TracerEngine::TIMERLAT`.
  - Each table grows to four words and each `static_assert` moves to
    `HWLAT`, the new last enumerator. The assert is the whole of the
    pin: a table that misses the new word fails the build.
- Invariants:
  - The loader reads both words through the same tables, at
    `source/harness/json_load.cpp:1457-1463` for the method and at
    `source/harness/json_load.cpp:1093-1099` for the engine, so a word
    outside the list refuses and names itself.
  - A method word outside the four refuses with one quoted line at exit
    status 2. The standing text reads `'…' is not tsc, fwq or ftq` at
    `source/harness/jitter_probe.cpp:1677` (spec FR-005).
  - An engine word outside the three refuses the same way. The standing
    text reads `'…' is not osnoise or timerlat` at
    `source/harness/noise_tracer.cpp:55-56` (spec FR-005).
  - One run carries one method and one engine, and a `hwlat` method
    writes the `method` key (spec FR-002, specs/043 FR-006).

## E-02: `JitterStats`

The result of one probe cpu over one set of covered windows: the
sixteen standing fields of specs/027 E-03 in the E-03 order
(`include/speedgun-ng/benchmark.hpp:516-534`), and the three new
figures beside them (spec FR-025, plan P-5). One record feeds
`jitter.stats[]`, `precheck.cpus[].stats` and the per-entry
`jitter.stats[]` items. Two write sites cover the three shapes: the
shared per-cpu writer at `source/harness/json_report.cpp:332-364`, which
serves the run-level and the per-entry items, and the inline pre-check
block at `source/harness/json_report.cpp:1242-1262`, which repeats the
standing sixteen keys.

| Field | Type | Unit | Holds a value when | Key |
| --- | --- | --- | --- | --- |
| `gapCount` | `uint64_t` | count | always | same |
| `gapsPerSecond` | `double` | 1/s | always | same |
| `minNs` | `uint64_t` | ns | always | same |
| `medianNs` | `uint64_t` | ns | always | same |
| `meanNs` | `double` | ns | always | same |
| `p90Ns` | `uint64_t` | ns | always | same |
| `p99Ns` | `uint64_t` | ns | always | same |
| `p999Ns` | `uint64_t` | ns | always | same |
| `p9999Ns` | `uint64_t` | ns | always | same |
| `maxNs` | `uint64_t` | ns | always | same |
| `totalNs` | `uint64_t` | ns | always | same |
| `totalPercent` | `double` | percent | always | same |
| `runtimeNs` | `uint64_t` | ns | always | same |
| `thresholdNs` | `uint64_t` | ns | always | same |
| `loopPeriodNs` | `uint64_t` | ns | always | same |
| `droppedGaps` | `uint64_t` | count | always | same |
| `smiDelta` | `optional<uint64_t>` | count | the read answered | same |
| `nmiDelta` | `optional<uint64_t>` | count | the line stood read | same |
| `smiGapCount` | `uint64_t` | count | the method reads | same |

- Fields: the last three rows. `smiDelta` counts the change of
  `MSR_SMI_COUNT` across the cpu's window, `nmiDelta` counts the change
  of the `NMI` column across the same window, and `smiGapCount` counts
  the kept gaps whose span carried a change above 0 (spec FR-016, spec
  D-7). The window stands the
  pre-check window in the pre-check phase and the probe's own run span
  in the alongside phase, and each delta answers the two reads at its
  edges (spec FR-016). `Key` reads `same` for every field: each key
  carries the field's own name.
- Units: `smiDelta`, `nmiDelta` and `smiGapCount` stand counts. The
  standing `Ns` fields stand nanoseconds, `gapsPerSecond` stands
  inversed seconds, and `totalPercent` stands a percentage.
- Support gate: the SMI source stands documented only where the
  `vendor_id` line of `/proc/cpuinfo`, resolved under the file root,
  reads `GenuineIntel`, the counter of spec D-3. Any other text, and an
  unreadable file, answer unsupported and the three figures answer not
  available with no substitute named (spec FR-010, plan P-4). The text
  rule keeps the file seam the only dependency: the standing reader
  `readUnderRoot` at `source/harness/jitter_probe.cpp:1778` already
  serves the host settings read of `proc/cmdline` at
  `source/harness/jitter_probe.cpp:2082`.
- Validation: each delta stands at or above 0 (spec FR-016). A source
  that stands missing, unreadable or unparsable leaves its figure with
  no value, the console states the standing wording once per run for
  that source, and the run continues with exit status 0 (spec FR-011).
  An absent figure writes `null`, and no zero stands in its place, the
  rule specs/034 FR-018 carries. `smiGapCount` stands a plain count: 0
  answers a run that marked no gap, a run whose SMI source stood
  unavailable included (spec FR-015). The fold of an empty kept set
  leaves the sixteen standing fields at 0, the rule the record states at
  `include/speedgun-ng/benchmark.hpp:505-509`.
- Lifecycle: the writer writes the three keys for a run of the `hwlat`
  method and writes none of the three for a run of any other method
  (spec FR-025). The writer needs the run's method to decide, and
  `JitterReportRecord.method` carries it at
  `source/harness/detail/json_report.hpp:272`. The standing per-cpu
  writer takes only the entry and the raw-gaps flag
  (`source/harness/json_report.cpp:332-334`).
- Order: the three keys follow `droppedGaps` in each stats object, the
  line the writer keeps open at `source/harness/json_report.cpp:353`,
  and they stand outside the schema's `required` lists at
  `docs/schema/report-v1.json:190-208` and
  `docs/schema/report-v1.json:286-303`.
- Owner: the public struct at
  `include/speedgun-ng/benchmark.hpp:516-534`; the folds of the probe
  unit; the writer `source/harness/json_report.cpp`; the readers
  `source/harness/json_load.cpp:461` and
  `source/harness/json_load.cpp:804`.

## E-03: `GapRecord` and the change ring

The kept gap record, and the ring of per-gap SMI changes that rides
beside it (plan P-3). The record keeps its shape, and the ring carries
what the record cannot hold (spec FR-012).

| Field | Type | Unit | Rule |
| --- | --- | --- | --- |
| `stamp` | `uint64_t` | TSC count | the read that saw the gap |
| `gapNs` | `uint64_t` | ns | the gap above the threshold |

- Fields: the two rows above, unchanged at
  `source/harness/detail/jitter_probe.hpp:41-45`, the specs/027 E-04
  rule. The invariant `static_assert(sizeof(GapRecord) == 16)` stands at
  `source/harness/detail/jitter_probe.hpp:47`, and the feature adds no
  third field to the record (spec FR-012, spec Constraints).
- The ring: one `std::vector<std::uint64_t>` per probe cpu, one unsigned
  change per kept gap, the storage shape of the standing series ring at
  `source/harness/detail/jitter_probe.hpp:145`. The 8-byte element record
  that shape documents stands at
  `source/harness/detail/jitter_probe.hpp:52-58` with its own size
  `static_assert`, and plan P-3 names the new ring by it. The change
  stands a count of SMIs, so the element answers a span and holds no
  stamp.
- The span rule, the rule spec D-7 and spec FR-013 set: an element holds
  this gap's read minus the read before it. A gap stands known only
  after it ended, so no read precedes it, and the read before it stands
  the window's opening read for the first gap of the window and the
  previous gap's read for every later gap. The first element therefore
  covers the span from the opening of the window to the end of the first
  gap, and each later element covers the run of healthy loop between two
  gaps plus the later gap.
- Mark meaning: a change above 0 marks the gap SMI and says the counter
  moved somewhere in that span. A change of 0 marks the gap not SMI, and
  for a gap whose span opened at the previous gap's read, the mark says
  the counter stood still from the moment the probe read it after the
  earlier gap to the moment it read it after this gap (spec FR-014). An
  SMI that the earlier gap already counted stands outside this gap's
  span, and an SMI during the healthy loop between two gaps stands
  inside the next gap's span.
- Capacity rule: the ring pre-sizes to the run's `gapCount` entries, the
  count the gap buffer pre-sizes to at
  `source/harness/detail/jitter_probe.hpp:129`. The value comes from
  `JitterProbeOptions.gapCount` at
  `include/speedgun-ng/benchmark.hpp:2519`, 65536 by default at
  `include/speedgun-ng/benchmark.hpp:2427`, inside the range of 1
  through 16,777,216 at `include/speedgun-ng/benchmark.hpp:2428-2429`.
- Drop rule: the gap path stores one record and one change in one step,
  the step `keepOrDrop` holds today at
  `source/harness/jitter_probe.cpp:109-123`. A full buffer drops the
  pair and raises `droppedGaps` once, the standing counter at
  `source/harness/detail/jitter_probe.hpp:131`. No second drop counter
  stands. The read stands part of the kept-gap step, so a dropped gap
  stands no read and the next kept gap's span covers the dropped gap
  too (spec FR-012).
- Index pairing: index `i` of the change ring answers index `i` of the
  gap ring, and the kept count at
  `source/harness/detail/jitter_probe.hpp:130` ends both prefixes, so
  the two lists hold one prefix each and the fold needs no third list.
  The elements sum to the change from the opening read to the last kept
  gap's read, and `smiDelta` of E-02 adds the span from that read to the
  closing read.
- The read that fills the ring stands on the gap path and at the two
  window edges: one `pread` of 8 bytes per kept gap, outside the measured
  interval (spec FR-012, plan P-2, plan R-1).
- The latch path: the pre-check record folds the latch-time snapshot at
  `source/harness/cli.cpp:1393`, read through
  `source/harness/detail/jitter_probe.hpp:316` and
  `source/harness/detail/jitter_probe.hpp:328`. The snapshot at
  `source/harness/detail/jitter_probe.hpp:86-102` therefore carries the
  change prefix beside the gap prefix, the way it carries `gaps` at
  `source/harness/detail/jitter_probe.hpp:91-92`.
- Owner: the probe unit, the type in
  `source/harness/detail/jitter_probe.hpp`, the code in
  `source/harness/jitter_probe.cpp`.

## E-04: the tracer records

The tracer record one run carries, widened by the three settings the run
wrote and the three figures the detector states (spec FR-027, plan P-7).

`PrecheckTracerRecord` stands at
`include/speedgun-ng/benchmark.hpp:725-729`:

| Field | Type | Unit | Holds a value when | Key |
| --- | --- | --- | --- | --- |
| `engine` | `string` | word | the tracer ran | same |
| `windowUs` | `optional<uint64_t>` | us | the `hwlat` engine | same |
| `widthUs` | `optional<uint64_t>` | us | the `hwlat` engine | same |
| `thresholdUs` | `optional<uint64_t>` | us | the `hwlat` engine | same |
| `cpus` | `vector<PrecheckTracerCpuRecord>` | entries | the tracer ran | same |

`PrecheckTracerCpuRecord` stands at
`include/speedgun-ng/benchmark.hpp:706-714`:

| Field | Type | Unit | Holds a value when | Key |
| --- | --- | --- | --- | --- |
| `cpu` | `uint32_t` | cpu index | always | same |
| `nmi` | `optional<uint64_t>` | count | the file states it | same |
| `irq` | `optional<uint64_t>` | count | the file states it | same |
| `softirq` | `optional<uint64_t>` | count | the file states it | same |
| `thread` | `optional<uint64_t>` | count | the file states it | same |
| `hardware` | `optional<uint64_t>` | count | the file states it | same |
| `maxGapNs` | `optional<uint64_t>` | ns | a line stands read | same |
| `gapCount` | `optional<uint64_t>` | count | a line stands read | same |
| `nmiCount` | `optional<uint64_t>` | count | a line stands read | same |

- Fields: the three new rows of each table. `cpus` stands ascending,
  one entry per tracer cpu (specs/036 FR-014). `maxGapNs` takes the
  larger of the kernel's two sample figures times 1000, the rule
  `kernel/trace/trace_hwlat.c:296` uses for its own maximum. `gapCount`
  counts the detection lines of that cpu's text, and `nmiCount` sums
  their `nmi-count` fields, a field the kernel prints only above zero at
  `kernel/trace/trace_output.c:1538` and `kernel/trace/trace_output.c:1546`
  (plan P-10). `Key` reads `same` for every field.
- Units: `windowUs`, `widthUs` and `thresholdUs` stand microseconds.
  `maxGapNs` stands nanoseconds. `gapCount` and `nmiCount` stand counts.
  The file `tracing_thresh` takes nanoseconds, so the 1,000 us threshold
  of spec A-3 stands `1000000` in the file (spec FR-019).
- Validation: the three settings carry a value in every `hwlat` record,
  because the setup reads each file before it writes it (spec FR-019). A
  detector text that stands missing, unreadable or malformed leaves all
  three cpu figures with no value, the standing rule of `figuresOfCpu`
  at `source/harness/noise_tracer.cpp:258-300`. A text the read answered
  with no detection line leaves all three cpu figures with no value, the
  rule spec FR-027 states, and each of the three writes `null`.
  `nmiCount` answers 0 where lines stand and none of them carries the
  `nmi-count` field, the reading plan P-10 states. No figure writes a
  zero in place of a figure the host could not answer.
- What a `null` cannot mean: the run writes `hwlat_detector/mode` with
  the word `per-cpu` before the engine word, so each tracer cpu carries
  its own detector thread and states figures of its own (spec D-9, plan
  P-14, `kernel/trace/trace_hwlat.c:67`). The standing `round-robin`
  default moves one thread across the mask and leaves the cpus it never
  reached with an empty buffer, so a `null` under the mode the run wrote
  answers a cpu the detector watched with no gap in its buffer, and the
  console states the standing not-available wording for it.
- Reading: no detector line in a cpu's buffer answers `null` for all
  three figures, and a zero stands only for the `nmi-count` case above
  (spec FR-027).
- Lifecycle: the record stands when the tracer ran. A run that asks for
  no tracer and a run whose tracer stands not available leave the member
  empty and write no key (specs/036 FR-015,
  `source/harness/json_report.cpp:1325-1350`).
- Order: the three settings follow `engine` in `precheck.tracer`, and
  the three new cpu figures follow `hardware` in each entry (spec
  FR-029). The standing rows keep their places
  (`docs/schema/report-v1.json:367-403`).
- Owner: the public structs; `engineWordOf` and the figure read at
  `source/harness/noise_tracer.cpp:306-373`; the writer at
  `source/harness/json_report.cpp:1332-1348`; the readers at
  `source/harness/json_load.cpp:1041-1119`.

## E-05: the records the writer takes

The internal records that carry these figures from the run to the
writer, and the record the writer reads for the tracer.

`JitterReportCpu` stands at
`source/harness/detail/json_report.hpp:244-254`:

| Field | Type | Unit | Rule |
| --- | --- | --- | --- |
| `cpu` | `uint32_t` | cpu index | the probe cpu of the item |
| `stats` | `JitterStats` | mixed | the fold of E-02 |
| `gaps` | `vector<GapRecord>` | records | the kept records |
| `smiChanges` | `vector<uint64_t>` | count | new; the span prefix |
| `series` | `optional<JitterSeriesStats>` | mixed | specs/043 E-23 |

`JitterReportRecord` stands at
`source/harness/detail/json_report.hpp:266-280`:

| Field | Type | Unit | Rule |
| --- | --- | --- | --- |
| `mode` | `JitterProbeMode` | word | the phase the run carried |
| `method` | `JitterMethod` | word | the value that gates E-06 |
| `thresholdNs` | `uint64_t` | ns | specs/027 FR-007 |
| `loopPeriodNs` | `uint64_t` | ns | the measured loop period |
| `cpus` | `vector<uint32_t>` | cpu indexes | ascending |
| `stats` | `vector<JitterReportCpu>` | items | one per probe cpu |
| `droppedGaps` | `uint64_t` | count | the gaps the run dropped |
| `rawGaps` | `bool` | flag | the gate on `gaps` |
| `disclosure` | `string` | text | specs/027 E-09 |

- Fields: `smiChanges` is the one new field, one SMI change per entry of
  `gaps`, index-paired with it (E-03). The writer pairs the two lists to
  write one gap's mark (spec FR-013, spec FR-026).
- Validation: `buildJitterReportRecord` fills the new list beside the
  standing gaps copy at `source/harness/cli.cpp:560-569`. The list stays
  empty when the SMI source stands unavailable, and the writer answers
  `null` for every gap of that cpu, because that cpu's `smiDelta` holds
  no value (spec FR-015, spec FR-026). One source decides both figures.
- The tracer record the writer takes: `writeReport` takes
  `const optional<PrecheckRecord>&` at
  `source/harness/detail/json_report.hpp:353-359`, and the `tracer`
  member at `include/speedgun-ng/benchmark.hpp:748` carries E-04. The
  run builds it at `source/harness/cli.cpp:1369-1374`. No new record
  joins the writer's parameter list, and no new top-level key joins the
  document (spec D-13).
- Owner: `source/harness/detail/json_report.hpp`,
  `source/harness/json_report.cpp`, `source/harness/cli.cpp`.

## E-06: the report keys

The keys this feature adds, their kinds, the condition that writes each
one, and the rule for an unavailable figure. The root objects stand
`jitter` and `precheck`, and the top-level `x-order` of
`docs/schema/report-v1.json:7` stays as it stands. Short names in the
`Object` column: `stats[]` names `jitter.stats[]`, `cpus[].stats` names
`precheck.cpus[].stats`, `gaps[]` names `jitter.stats[].gaps[]`,
`tracer` names `precheck.tracer`, and `tracer.cpus[]` names
`precheck.tracer.cpus[]`.

| Key | Object | Kind | Written when | Absent figure |
| --- | --- | --- | --- | --- |
| `smiDelta` | `stats[]`, `cpus[].stats` | int/null | `hwlat` | `null` |
| `nmiDelta` | `stats[]`, `cpus[].stats` | int/null | `hwlat` | `null` |
| `smiGapCount` | `stats[]`, `cpus[].stats` | int | `hwlat` | none; 0 |
| `smi` | `gaps[]` | string/null | `hwlat`, raw gaps | `null` |
| `windowUs` | `tracer` | int | `hwlat` engine | none |
| `widthUs` | `tracer` | int | `hwlat` engine | none |
| `thresholdUs` | `tracer` | int | `hwlat` engine | none |
| `maxGapNs` | `tracer.cpus[]` | int/null | `hwlat` engine | `null` |
| `gapCount` | `tracer.cpus[]` | int/null | `hwlat` engine | `null` |
| `nmiCount` | `tracer.cpus[]` | int/null | `hwlat` engine | `null` |
| `limits` | `jitter` | string | `hwlat` | none |

- Null rule: a key of a surface the run did not name stands out of the
  document, so a run of another method or another engine writes the
  bytes it writes today (spec FR-025, spec FR-029, spec SC-006). A
  figure of a surface the run named, which the host could not answer,
  stands `null` inside the object that carries the key, the file form of
  the console's `not available` (`source/harness/json_report.cpp:213`
  and `source/harness/json_report.cpp:257-281`). No figure stands a zero
  in place of an unavailable figure. The two counts that answer 0,
  `smiGapCount` and a detector `nmiCount` whose lines carry no
  `nmi-count` field, stand true counts of the run's own records.
- The words of `smi`: `"SMI"` above a change of 0, `"NOT_SMI"` at a
  change of 0, and `null` where the SMI count stands unavailable (spec
  FR-014, spec FR-026). The key follows `gapNs` in the gap item of
  `docs/schema/report-v1.json:232-238`, and stands outside that item's
  `required` list.
- The `--json-raw=gaps` gate: the row stands at
  `source/harness/options.cpp:608`, its word set at
  `source/harness/options.cpp:258-272`, and the flag reaches the record
  at `source/harness/cli.cpp:559`. The writer opens the `gaps` array
  under that flag alone at `source/harness/json_report.cpp:354-363`, so
  the `smi` key stands only in a document the operator asked for. The
  per-entry stats items pass the flag as false at
  `source/harness/json_report.cpp:776`, so no gap list stands there.
- The per-entry object: the schema types the entry's `jitter` key as an
  open object at `docs/schema/report-v1.json:572-574`, so the three
  stats keys reach it with no schema row, and an `hwlat` run writes them
  there too, because one writer function fills both shapes
  (`source/harness/json_report.cpp:763-778`).
- Loader: an absent key reads as no figure, the standing optional path of
  `optionalField` at `source/harness/json_load.cpp:86-94` with
  `readNullableCount` at `source/harness/json_load.cpp:723-734` (spec
  A-7). The gap items keep their standing validate-and-drop shape at
  `source/harness/json_load.cpp:516-542`: the reader kind-checks
  `stamp`, `gapNs` and now `smi`, refuses a value that carries neither
  word, and keeps no public gap record (plan P-6). The stats figures
  read at `source/harness/json_load.cpp:461` and
  `source/harness/json_load.cpp:804`; the tracer figures read at
  `source/harness/json_load.cpp:1041` and
  `source/harness/json_load.cpp:1085`.
- Schema rows that move: the two enum lists at
  `docs/schema/report-v1.json:75-78` and
  `docs/schema/report-v1.json:371`; the `x-order`, the `properties` and
  the `required` lists of `jitter`, `jitter.stats[]`, the gap item,
  `precheck.cpus[].stats`, `precheck.tracer` and
  `precheck.tracer.cpus[]`; and `kSchemaVersion` at
  `source/harness/json_report.cpp:49`, which moves `1.14` to `1.15`
  (spec D-5, spec FR-029). The standing checker reads every written
  document against these rows at
  `source/harness/detail/json_report.hpp:361-376`.
- The eleventh key: `jitter.limits` carries the run's statement that the
  user-space probe runs with the cpu's interrupts enabled (spec FR-031,
  plan P-13). The spec's key table, the plan's file map, the report
  contract and this file count eleven keys.
- Owner: `docs/schema/report-v1.json`,
  `source/harness/json_report.cpp`, `source/harness/json_load.cpp`,
  `specs/048-hwlat-probe/contracts/report-json.md`.

## E-07: the setting snapshot and the saved set

One `{path, value}` pair per saved file: six pairs for the seven files
the run writes, and the vector the session carries across the window
(spec D-8, plan P-8, spec FR-021).

| Field | Type | Unit | Rule |
| --- | --- | --- | --- |
| `path` | `string` | path | the file the run wrote |
| `value` | `string` | text | the standing text, trimmed; for `hwlat_detector/mode`, the bracketed word alone |

- Fields: a `vector<TracerSetting>` beside the standing members of
  `NoiseTracerSession` at `source/harness/detail/noise_tracer.hpp:45-51`,
  which carries two saved texts today, `previousTracer` and
  `previousCpuMask`. The vector holds six pairs in write order, one per
  file of the saved set, and `tracing_on` stands outside it. The spec
  `036` session shape grows to carry the set (spec D-8).
- The paths, as the lane resolves them: each name below goes through
  `procRootPath` at `source/detail/proc_root.hpp:35-41`, the seam
  `installTestProcRoot` at `source/detail/proc_root.hpp:52-56` moves for
  a test (spec FR-001). The tracer's read and write helpers already
  resolve through it at `source/harness/noise_tracer.cpp:144` and at
  `source/harness/noise_tracer.cpp:165`.

| Step | Path under the file root | Value | Saved |
| --- | --- | --- | --- |
| 1 | `sys/kernel/tracing/tracing_cpumask` | the tracer mask | yes |
| 2 | `sys/kernel/tracing/hwlat_detector/mode` | `per-cpu` | yes |
| 3 | `sys/kernel/tracing/hwlat_detector/width` | `5000` us | yes |
| 4 | `sys/kernel/tracing/hwlat_detector/window` | `10000` us | yes |
| 5 | `sys/kernel/tracing/tracing_thresh` | `1000000` ns | yes |
| 6 | `sys/kernel/tracing/current_tracer` | `hwlat` | yes |
| 7 | `sys/kernel/tracing/tracing_on` | `1` | no |

- Write order: the seven steps above, the order spec FR-019 states. The
  mask renders in the word width the standing mask carried, the rule of
  `maskWidthOf` at `source/harness/noise_tracer.cpp:328` and the write at
  `source/harness/noise_tracer.cpp:337-338`. Step 2 writes the word
  `per-cpu`, one of the three words the kernel accepts at
  `kernel/trace/trace_hwlat.c:67`, and the write sets the mode a later
  start applies
  (`kernel/trace/trace_hwlat.c:706-711`), the word outside those three
  answering `EINVAL` (`kernel/trace/trace_hwlat.c:694`). The values of
  steps 3 to 5 stand the named constants of spec A-3 and spec FR-020,
  and step 6 needs the third word of `engineWordOf` at
  `source/harness/noise_tracer.cpp:306-311`.
- Read form of `mode`: the file answers all three words with the active
  one inside brackets, the shape `none [round-robin] per-cpu`, the print
  at `kernel/trace/trace_hwlat.c:629-632` and `:642`. The write handler
  compares one stripped word (`kernel/trace/trace_hwlat.c:692`, `:694`,
  `:706-711`), so the saved value of that pair holds the bracketed word
  alone. The whole read form would answer `EINVAL` at the restore, and no
  other saved file carries a form the write refuses.
- Why this order: the kernel bounds a width write by the standing window
  (`kernel/trace/trace_hwlat.c:731-736`) and bounds a window write by the
  standing width (`kernel/trace/trace_hwlat.c:743-748`), so the two
  values need the low one first, the order spec FR-019 states. The mask, the
  mode, the two knobs and the threshold stand written before the engine
  word, because the detector takes the mask when it starts
  (`kernel/trace/trace_hwlat.c:434`, `kernel/trace/trace_hwlat.c:585`),
  creates one thread per masked cpu under the per-cpu mode
  (`kernel/trace/trace_hwlat.c:575-591`), reads the threshold in its init
  (`kernel/trace/trace_hwlat.c:836`) and starts the sampling from the
  engine word when tracing stands on
  (`kernel/trace/trace_hwlat.c:842-843`). `tracing_on` stands last,
  because that write starts the sampling. The standing engines keep their
  own order, the engine word first
  (`source/harness/noise_tracer.cpp:332-342`).
- Read order: the run reads the standing text of each of the six saved
  files before its first write (spec FR-019), and it reads no
  `tracing_on` text, because that file stands outside the set. The
  restore writes `0` there, the standing step at
  `source/harness/noise_tracer.cpp:379` that spec SC-007 measures, so a
  run that stopped a tracer that stood on leaves tracing off (spec D-8,
  spec FR-021). The kernel saves the threshold the engine word found when
  the tracer inits and writes that value back at the reset
  (`kernel/trace/trace_hwlat.c:836`, `kernel/trace/trace_hwlat.c:857`),
  so the kernel returns the run's own threshold and only the session
  holds the host's. The reset also records that value as the default a
  later run takes when it names none
  (`kernel/trace/trace_hwlat.c:840`, `kernel/trace/trace_hwlat.c:855`),
  the residue spec D-8 names.
- Restore order: `tracing_on` at `0` first, then each saved value in the
  reverse of the FR-019 write order: `current_tracer`, `tracing_thresh`,
  `hwlat_detector/window`, `hwlat_detector/width`,
  `hwlat_detector/mode`, `tracing_cpumask`, so the threshold goes back
  after the engine word (spec FR-021) and the kernel's own reset runs
  before the session puts the host threshold back (plan P-8). The reverse
  walk of the vector carries that order, and it runs on every path the
  process controls: the normal end, a failed read or write partway
  through the setup, a failed read inside the window, and the standing
  `SIGINT` path (spec FR-021, spec D-15).
- Failed write: the restore returns the list of paths whose write failed,
  the run prints one line per path with the saved value beside it, and
  the run keeps its status (spec FR-022, spec D-12). The standing
  restore drops each write result today
  (`source/harness/noise_tracer.cpp:375-384`).
- The figure read, read only: `sys/kernel/tracing/per_cpu/cpu<id>/trace`,
  the file `kernel/trace/trace.c:7672` creates under the `per_cpu`
  directory of `kernel/trace/trace.c:7631` and the cpu directory of
  `kernel/trace/trace.c:7661`. One detection stands on one line that
  begins with `#` and a digit, the shape of
  `kernel/trace/trace_output.c:1531`, with ` nmi-count:` added at
  `kernel/trace/trace_output.c:1546` when the count stands above 0. A
  header legend line begins with `#` and a space
  (`kernel/trace/trace.c:2897-2905`, reached at
  `kernel/trace/trace.c:3310-3319`), so a rule that needs a digit second
  skips it (plan P-10, spec A-6).
- Owner: `source/harness/detail/noise_tracer.hpp` for the types,
  `source/harness/noise_tracer.cpp` for the three functions.

## E-08: the comparison values

The six names the N5 comparison gains (spec FR-028, plan P-11). The
three probe names join the standing grammar
`jitter:[run:]<cpu|any>:<field>`, and the three detector names stand
behind the new head word `hwlat`, in the shape
`hwlat:<cpu|any>:<field>`. Each name reads one record of the loaded
report.

| Name | Record the value reads | Unit |
| --- | --- | --- |
| `smiDelta` | `JitterEntryFold.stats` | count |
| `nmiDelta` | `JitterEntryFold.stats` | count |
| `smiGapCount` | `JitterEntryFold.stats` | count |
| `maxGapNs` | `PrecheckTracerCpuRecord` | ns |
| `gapCount` | `PrecheckTracerCpuRecord` | count |
| `nmiCount` | `PrecheckTracerCpuRecord` | count |

- Carrier, probe names: `JitterEntryFold` at
  `include/speedgun-ng/benchmark.hpp:763-770`, read through the table of
  `source/harness/compare.cpp:241-245` and its 32 rows at
  `source/harness/compare.cpp:262`. A `run:` scope reads
  `ReportData::runJitterStats` at
  `include/speedgun-ng/benchmark.hpp:1212`, and a bare name reads
  `BenchmarkResult::jitterFolds` at
  `include/speedgun-ng/benchmark.hpp:956`
  (`source/harness/compare.cpp:1546-1584`). These three names read the
  same carrier the standing jitter names read (spec FR-028).
- Carrier, detector names: `ReportData::precheck` at
  `include/speedgun-ng/benchmark.hpp:1223`, filled by the loader at
  `source/harness/json_load.cpp:3232-3240`, whose `tracer` member carries
  the entries of E-04. `source/harness/compare.cpp` reads no pre-check
  record today, so these three names stand the first.
- Table moves: `kJitterFields` moves from 32 rows to 35 at
  `source/harness/compare.cpp:262`, the count word of its refusal line
  moves from `thirty-two` to `thirty-five` at
  `source/harness/compare.cpp:429`, and the contract lines move with it
  at `specs/027-jitter-probe/contracts/rule-names.md:20` and `:29-31`
  (spec FR-028). The three detector names stand in the new `kHwlatFields`
  table of 3 rows with their own refusal line, so a standing jitter
  refusal keeps its words (plan P-11, plan R-7).
- Validation: two cases stand apart. A figure the record carries as
  absent reads 0, the standing rule of a fold with no value. A report that
  carries no tracer record, or no entry for a cpu a rule names, answers
  one fault line naming the rule and what the report lacks, at exit status
  2 (`source/harness/compare.cpp:1563-1576`, the note at
  `source/harness/compare.cpp:1539-1545`). No name gains a status (spec
  FR-028). The standing table reader answers a `double` at
  `source/harness/compare.cpp:244`, so a reader for a nullable figure has
  to answer a missing figure and a number, and M8 carries that change.
- Owner: `source/harness/compare.cpp`,
  `specs/048-hwlat-probe/contracts/rule-names.md`.

## State transitions

```text
run start
  ├─ no method=hwlat ................ no counter read, none of the
  │                                 eleven keys, standing bytes
  └─ method=hwlat
       ├─ vendor_id reads GenuineIntel in proc/cpuinfo, under
       │    the file root
       │    ├─ dev/cpu/<cpu>/msr answers 8 bytes at 0x34
       │    │    ├─ the gap's span carries a change .... mark SMI
       │    │    └─ the span carries none ..... mark NOT_SMI
       │    ├─ proc/interrupts states the NMI line .. nmiDelta holds
       │    └─ a source stands missing, unreadable, unparsable
       │         disclosure once, no mark, smiDelta and nmiDelta
       │         absent, the run continues, exit status 0
       └─ vendor_id reads anything else ..... unsupported, the same
                                             not-available shape
  ├─ --os-noise absent ............. no session, no tracer key
  └─ --os-noise=hwlat
       ├─ the mode runs no pre-check window .. one refusal line, status 2
       ├─ current_tracer already reads hwlat .... no write, one disclosure
       │    line, the tracer lines not available, status 0
       └─ the setup reads six texts, then writes seven files
            ├─ a read or a write fails ... restore what it wrote, then
            │                            not available, status 0
            └─ the seven writes stand through
                 ├─ the window runs, and the detector writes its lines
                 ├─ per_cpu/cpu<id>/trace reads the three figures, one
                 │  entry per tracer cpu, each null with no line
                 ├─ restore: tracing_on at 0, then each saved value,
                 │  last written first; one line per failed write
                 └─ the tracer key stands, and the rows of E-04 stand
  SIGINT in the window ... the standing flag ends the run, and the
                           restore runs the same list
  SIGKILL ................ no handler runs; the record names each file
                           the run wrote and the value it saved (D-15)
```

## What is deliberately absent

- No third field in `GapRecord`: the 16-byte invariant at
  `source/harness/detail/jitter_probe.hpp:47` holds, and the mark rides
  the ring of E-03 (spec FR-012).
- No AMD counter and no second counter of any kind, the rule spec D-4 and
  the spec Out of scope list set. The feature names no substitute.
- No module load, no module parameter, and no write to any `msr` node
  (spec FR-007, spec FR-008).
- No setuid helper, no capability grant, no daemon, and no write outside
  the seven files of E-07 (spec FR-024).
- No stored trace: the run reads the detector's stated figures and keeps
  no ring buffer of its own (spec Out of scope).
- No background sampler: the SMI read stands on the gap path and at the
  two window edges alone (spec D-7, spec Out of scope).
- No per-gap window of its own: no gap carries a read before it, so no
  element names the gap alone, and each answers the span since the read
  before (spec D-7).
- No new loaded record, no new top-level document key, and no new option
  row, option value or short letter (spec D-13, spec Constraints).
- No new comparison status: an absent figure answers the standing fault
  line (spec FR-028).
- No figure for `osnoise` and `timerlat`, and no change to their restore
  order. The shared restore fix of spec D-15 reaches their defect path as
  a consequence, and the lane records it beside the spec `036` contract
  line.
- No kernel version check: the file probe answers the only question the
  run asks, the standing rule of specs/036.
