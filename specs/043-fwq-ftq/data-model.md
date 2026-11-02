# Data Model: Fixed Work Quanta and Fixed Time Quanta Probes

Phase 1 output of `/speckit.plan` for `specs/043-fwq-ftq/spec.md`. The
entities below extend the standing probe data model
(`specs/027-jitter-probe/data-model.md`), the pre-check data model
(`specs/034-host-noise-precheck/data-model.md`) and the tracer data model
(`specs/036-osnoise-timerlat-tracers/data-model.md`). Nothing there
changes shape (FR-031). Each entity names its fields, its JSON key
mapping, its carrier and its invariants.

Entity numbering continues across features. E-01 to E-10 stand in
`specs/027-jitter-probe/data-model.md:36` to `:230`, E-11 to E-15 in
`specs/034-host-noise-precheck/data-model.md:30` to `:92`, and E-16 to
E-20 in `specs/036-osnoise-timerlat-tracers/data-model.md:34` to `:106`.
The last carried entity stands E-20, so this file starts at E-21.

JSON key names are camelCase, the style `docs/schema/report-v1.json`
carries (`docs/schema/report-v1.json:119-135`). The method word stands
SCREAMING_CASE in JSON and lower case on the console (FR-019). The key
order of every JSON object below is the `x-order` the schema rows carry
(`specs/027-jitter-probe/data-model.md:7-11`).

The record type names stand as this file's choice. FR-005 and FR-020
name the carriers: `JitterProbeOptions`
(`include/speedgun-ng/benchmark.hpp:2248`), `JitterEntryFold`
(`include/speedgun-ng/benchmark.hpp:645`) and `ReportData`
(`include/speedgun-ng/benchmark.hpp:984`).

Every cite reads the working tree at `d454bcff` (2026-11-01), the base
of branch `043-fwq-ftq`. The FR ids this file leans on stand in
`specs/043-fwq-ftq/spec.md`: FR-001 `:812`, FR-002 `:816`, FR-003
`:826`, FR-005 `:841`, FR-006 `:850`, FR-008 `:880`, FR-009 `:884`,
FR-010 `:894`, FR-011 `:904`, FR-012 `:912`, FR-014 `:924`, FR-015
`:932`, FR-017 `:947`, FR-018 `:955`, FR-019 `:969`, FR-020 `:983`,
FR-021 `:998`, FR-022 `:1022`, FR-025 `:1041`, FR-031 `:1113`, FR-034
`:1128`, SC-004 `:1184`, SC-009 `:1196`, Key Entities `:1131`.

## Entity map

```text
JitterMethod (new, public enum) ......................... E-21
  └─ TSC | FWQ | FTQ
JitterProbeOptions (public, +4 fields) .................. E-27
  ├─ method: JitterMethod                TSC | FWQ | FTQ
  ├─ quantumUs: uint32_t                 100 us default
  ├─ workCount: optional<uint32_t>       empty names the calibration
  └─ noisePercent: uint32_t              1 percent default
ProbeCpuState (standing) ── gains ──▶ the series buffer .. E-26
  ├─ series: vector<uint64_t>, gapCount entries, 8 bytes each
  ├─ kept: count
  └─ droppedSamples: count
JitterEntryFold (public, +1 member) ..................... E-23
  └─ series: optional<JitterSeriesStats>
        ├─ calibration: five keys of the record ......... E-22
        └─ periods: vector<JitterPeriodRecord> .......... E-24
ReportData (public, +1 member) .......................... E-25
  └─ jitterRun: optional<JitterRunRecord>
root `jitter` object (schema, +2 keys) .................. E-28
  ├─ method
  └─ series[]: 21 keys, closed
        └─ periods[]: 2 keys, closed
per-benchmark `jitter` object (open, +1 key) ............ E-28
  └─ series[]: 20 keys, no `method`, no `periods`
```

## E-21: `JitterMethod`

The method one probe run carries. It is the first new public enum since
`TracerEngine` closed the engine set
(`include/speedgun-ng/benchmark.hpp:2206`).

| Value | Console word | JSON word | Set by | Meaning |
| --- | --- | --- | --- | --- |
| `TSC` | `tsc` | `"TSC"` | a value with no `method` key | spec 027 as shipped: each gap above a threshold (FR-001) |
| `FWQ` | `fwq` | `"FWQ"` | `method=fwq` | one calibrated unit, timed per unit (FR-012) |
| `FTQ` | `ftq` | `"FTQ"` | `method=ftq` | whole units counted in each quantum (FR-015) |

- Fields: the one enum member of `JitterProbeOptions` (E-27).
- JSON key mapping: the root `jitter` key `method`, a string in the
  closed set `TSC`, `FWQ`, `FTQ` (FR-006). The standing `mode` key shows
  the shape: `"type": "string"` with an `enum` list
  (`docs/schema/report-v1.json:67-70`).
- Carrier and file: `enum class JitterMethod : std::uint8_t` in
  `include/speedgun-ng/benchmark.hpp` beside `JitterProbeMode`
  (`include/speedgun-ng/benchmark.hpp:2182-2187`). The JSON word table
  joins `kJitterModeWords` (`source/harness/detail/json_report.hpp:77-83`),
  with the `static_assert` that ties the table size to the last
  enumerator. The console word table joins the console writer
  (`source/harness/report.cpp:726-813`).
- Invariants:
  - One run carries one method (FR-001, D-1).
  - `TSC` stands the default, and a `TSC` run writes no `method` key
    (FR-006, SC-004).
  - The console spells the word in lower case and the JSON in upper
    case (FR-019), the standing split between the two surfaces
    (`docs/pages/harness.md:1829` against
    `docs/schema/report-v1.json:69`).
  - A word outside the three refuses one quoted line with exit status 2
    (FR-003), the standing mode refusal shape
    (`source/harness/jitter_probe.cpp:874-875`).
  - The per-benchmark `jitter` object carries no method word (FR-020).

## E-22: the calibration record

The five figures one probe cpu measures before its window. The console
and the JSON disclose them (FR-010, FR-011, AC-c2).

| Field | Type | Unit | Absent when |
| --- | --- | --- | --- |
| `workCount` | `std::uint32_t` | loop bodies | a `TSC` run |
| `unitTimeNs` | `std::uint64_t` | ns | a `TSC` run |
| `targetNs` | `std::uint64_t` | ns | a `TSC` run |
| `quantumNs` | `std::uint64_t` | ns | a `TSC` run, and a `FWQ` run |
| `calibrationSpanNs` | `std::uint64_t` | ns | a `TSC` run |

- Fields: the five names above, one struct per probe cpu.
- JSON key mapping: the five keys ride inside that cpu's `series` item
  (FR-011, FR-006), beside the series figures of E-23.
- Carrier and file: five keys inside `JitterSeriesStats` (E-23), public in
  `include/speedgun-ng/benchmark.hpp` beside `JitterStats`
  (`include/speedgun-ng/benchmark.hpp:512-530`). The measured values
  stand in the probe state first, beside `loopPeriodNs` and
  `thresholdNs` (`source/harness/detail/jitter_probe.hpp:71-72`).
- Invariants:
  - The search runs once per probe cpu, on that cpu, before that cpu's
    window (FR-010).
  - The target stands 1,000 ns for `FWQ` and one thousandth of the
    quantum for `FTQ` (FR-010); a 10 us quantum sets a 10 ns target.
  - Doubling runs from 1 body in at most 25 trials, counts 1 through
    16,777,216 bodies, bisection runs at most 24 steps, and each trial
    reads the time source twice, so the search costs at most 98 reads
    (FR-010).
  - The count cap stands 16,777,216 bodies (FR-002, FR-010).
  - The search ends at the trial cap, at the count cap, or when the
    bisection interval holds one count (FR-010).
  - A search that reaches a cap keeps the closest count it measured and
    discloses it, and the window runs (FR-010, Edge Cases).
  - `calibrationSpanNs` stands outside the window's `seriesRuntimeNs`
    (FR-010).
  - `quantumNs` carries the quantum in ns: 100 us stands 100,000 ns
    (FR-015).
  - The console prints the calibration lines before the window starts
    (FR-019, SC-003).
  - The record carries five keys: FR-006
    (`specs/043-fwq-ftq/spec.md:853`) and FR-011 (`:904`) name them,
    and the Key Entities row names all five (`:1146-1149`).

## E-23: the series record

The per-cpu statistics of one method's series. It joins the public
surface beside `JitterStats` (FR-020, D-7).

| Field | Type | Unit | Absent when |
| --- | --- | --- | --- |
| `cpu` | `std::uint32_t` | cpu index | never |
| `sampleCount` | `std::uint64_t` | samples | never |
| `minValue` | `std::uint64_t` | ns for `FWQ`, counts for `FTQ` | never |
| `medianValue` | `std::uint64_t` | ns for `FWQ`, counts for `FTQ` | never |
| `meanValue` | `double` | ns for `FWQ`, counts for `FTQ` | never |
| `p90Value` | `std::uint64_t` | ns for `FWQ`, counts for `FTQ` | never; the field stands 0 when the series keeps no sample |
| `p99Value` | `std::uint64_t` | ns for `FWQ`, counts for `FTQ` | never; the field stands 0 when the series keeps no sample |
| `p999Value` | `std::uint64_t` | ns for `FWQ`, counts for `FTQ` | never; the field stands 0 when the series keeps no sample |
| `p9999Value` | `std::uint64_t` | ns for `FWQ`, counts for `FTQ` | never; the field stands 0 when the series keeps no sample |
| `maxValue` | `std::uint64_t` | ns for `FWQ`, counts for `FTQ` | never |
| `noisePercent` | `double` | percent, 0 to 100 | never |
| `seriesRuntimeNs` | `std::uint64_t` | ns | never |
| `droppedSamples` | `std::uint64_t` | samples | never; the field stands 0 while the buffer never fills |
| `resampleStepNs` | `std::uint64_t` | ns | a `TSC` run, and an `FTQ` run |
| `spectrumPoints` | `std::uint64_t` | points | a `TSC` run |
| `periods` | array of E-24 | periods | a `TSC` run, and every per-benchmark copy |
| calibration keys | E-22 | see E-22 | a `TSC` run |

- Fields: the 12 series figures of FR-006, the 2 spectrum keys, the cpu
  index, the nested period list, and the 5 calibration keys of E-22.
- JSON key mapping: one `series` item carries 21 keys in this order:
  `cpu`, `workCount`, `unitTimeNs`, `targetNs`, `quantumNs`,
  `calibrationSpanNs`, `sampleCount`, `minValue`, `medianValue`,
  `meanValue`, `p90Value`, `p99Value`, `p999Value`, `p9999Value`,
  `maxValue`, `noisePercent`, `seriesRuntimeNs`, `droppedSamples`,
  `resampleStepNs`, `spectrumPoints`, `periods` (FR-006).
- Carrier and file: `JitterSeriesStats` with the five calibration keys,
  the 12 figures, the 2 spectrum keys and a
  `std::vector<JitterPeriodRecord>`. It joins `JitterEntryFold` beside
  `JitterStats` (`include/speedgun-ng/benchmark.hpp:645-649`), so the
  run-level read and the per-benchmark read share one type (FR-020).
- Invariants:
  - The array carries one item per probe cpu, ascending (FR-006).
  - The method sets the unit: ns for `FWQ`, counts for `FTQ`
    (FR-013, FR-016).
  - The percentiles use the N1 nearest-rank code over the series in
    place, with the standing minimum sample count of each point, and the
    code copies no buffer (FR-008,
    `source/harness/percentiles.cpp:96-102`, `:104-126`). The standing
    fold answers the rank `ceil(point x n / 100)` below that count, and
    a fold of an empty kept set leaves every field at 0
    (`source/harness/jitter_probe.cpp:346-357`,
    `specs/027-jitter-probe/data-model.md:97-98`).
  - The statistics and the spectrum read the kept samples only (FR-022).
  - `noisePercent` divides the kept unit times minus the sample count
    times the minimum by the kept unit times, for `FWQ`; it divides the
    sample count times the maximum count minus the sum of the counts by
    that product, for `FTQ`; a maximum count of 0 answers 100 percent
    (FR-009).
  - `seriesRuntimeNs` carries the window runtime, and `runtimeNs` keeps
    its gap meaning in the standing `stats` item
    (`include/speedgun-ng/benchmark.hpp:526`), so no name stands twice
    in the rule table (FR-021).
  - The whole record stands absent for a `TSC` run (FR-006).
  - Each benchmark entry's `jitter` object gains the same `series`
    array, folded over that benchmark's timed windows on
    `BenchmarkResult::jitterFolds`
    (`include/speedgun-ng/benchmark.hpp:733`), with no `method` key and
    no `periods` key, and `null` stands where the windows kept no
    sample (FR-020).
  - The fold reads the one series buffer of E-26, the way the gap fold
    reads the one gap buffer (`source/harness/jitter_probe.cpp:614-688`),
    so no second buffer joins (FR-020).
  - 16 names join the rule table `kJitterFields`
    (`source/harness/compare.cpp:241-292`), which grows from 16 to 32
    entries, and the fault text reads `names none of the thirty-two
    jitter fields` (FR-021, `source/harness/compare.cpp:366-369`).
  - A bare rule name reads the per-benchmark series and `jitter:run:`
    reads the run-level series (FR-021,
    `source/harness/compare.cpp:1462-1465`).
  - The `any` reading stays the uniform maximum over every field, so a
    noise rule over an `FTQ` run names `noisePercent` with `higher`
    (FR-021, `source/harness/compare.cpp:1485-1491`).

## E-24: the period record

One reported period of the series spectrum, nested inside the cpu's
`series` item (FR-006, D-5).

| Field | Type | Unit | Absent when |
| --- | --- | --- | --- |
| `periodNs` | `std::uint64_t` | ns | never |
| `powerRatio` | `double` | ratio against the strongest bin | never |

- Fields: the two names above, one entry per reported period.
- JSON key mapping: the nested closed `periods` array of the `series`
  item, item keys `periodNs` and `powerRatio` (FR-006).
- Carrier and file: `JitterPeriodRecord`, the element of the period
  vector of `JitterSeriesStats` (E-23), public in
  `include/speedgun-ng/benchmark.hpp`. The transform that fills it is
  private to the harness in `source/harness/detail/spectrum.hpp`
  (FR-017).
- Invariants:
  - The list carries at most 5 entries per probe cpu (FR-018).
  - The list stands sorted by power, strongest first, ties in period
    order (FR-018).
  - A period stands kept only when its power stands above 3 times the
    median of bins 1 through N/2, the DC bin out (FR-018).
  - `powerRatio` divides the bin power by the strongest bin's power
    (FR-018).
  - A constant series yields an empty list (FR-018, User Story 3
    scenario 3).
  - One bin means one frequency bin, `1 / (N x Ts)`, so the tolerance of
    a planted period `P` stands `P^2 / (N x Ts)` (FR-018).
  - The transform reads at most 65,536 points (FR-018).
  - The console prints the list as rank lines `period1Ns` through
    `period5Ns` and `period1Power` through `period5Power`, one column
    per probe cpu, and a rank a cpu does not carry prints 0 (FR-019).
  - The list stands absent from every per-benchmark copy (FR-020).

## E-25: the run record

The run-level record `loadReport` reads back: the method word, the
resample step and the point count (FR-020).

| Field | Type | Unit | Absent when |
| --- | --- | --- | --- |
| `method` | `JitterMethod` | one of the three words | a `TSC` run |
| `resampleStepNs` | `std::uint64_t` | ns | a `TSC` run, and an `FTQ` run |
| `spectrumPoints` | `std::uint64_t` | points | a `TSC` run |

- Fields: the three names above.
- JSON key mapping: the record reads the root `jitter` keys `method`,
  `resampleStepNs` and `spectrumPoints`; the last two ride each cpu's
  `series` item in the document (FR-006, FR-020).
- Carrier and file: `JitterRunRecord`, one `std::optional` member of
  `ReportData` beside `runJitterStats`
  (`include/speedgun-ng/benchmark.hpp:979-984`), the way spec 037 moved
  the harness `SOVERSION` when `ReportData` gained its graph member
  (`CMakeLists.txt:369-374`).
- Invariants:
  - `loadReport` fills it, and the run path never fills it and never
    reads it, the standing reading of `runJitterStats`
    (`include/speedgun-ng/benchmark.hpp:979-983`).
  - The member stands empty for a document with no `jitter` object and
    for a `TSC` run (FR-006).
  - The resample step and the point count ride each cpu's `series` item
    as well (FR-020).
  - The member is the third exported layout move FR-005 names, beside
    the option fields of E-27 and the series record of E-23, and SC-009
    reads the commit that records the three (FR-005, SC-009).
  - The `jitter:run:` scope reads the run-level `series` items, the
    carrier `runJitterStats` carries (FR-021,
    `source/harness/compare.cpp:1462-1465`).

## E-26: the series buffer record

One kept sample of one method's series, and the per-cpu buffer that
holds them inside the measured loop (FR-014, D-6).

| Field | Type | Unit | Absent when |
| --- | --- | --- | --- |
| `value` | `std::uint64_t` | ns for `FWQ`, counts for `FTQ` | never |
| `series` (the buffer) | `std::vector<std::uint64_t>` | `gapCount` entries | a `TSC` run keeps the gap buffer |
| `kept` | `std::size_t` | samples | never |
| `droppedSamples` | `std::uint64_t` | samples | never; the field stands 0 while the buffer never fills |

- Fields: one 8-byte value per sample, the buffer span, the kept count
  and the drop count, one set per probe cpu.
- JSON key mapping: no key carries the buffer. The kept prefix folds
  into the figures of E-23, and `droppedSamples` carries the drop count
  (FR-022). No raw sample list joins the document: the closed root
  `jitter` object refuses a key FR-006 does not list
  (`docs/schema/report-v1.json:65`, `source/harness/json_load.cpp:2532-2543`).
- Carrier and file: the fields join `ProbeCpuState`
  (`source/harness/detail/jitter_probe.hpp:63-74`), beside the standing
  gap buffer, harness-internal behind
  `source/harness/detail/jitter_probe.hpp` (spec 027 FR-015).
- Invariants:
  - The size comes from the standing `gaps=<n>` key: 65,536 entries by
    default, in the range 1 to 16,777,216 entries (FR-014,
    `include/speedgun-ng/benchmark.hpp:2192-2194`).
  - One element stays 8 bytes: the buffer stands
    `std::vector<std::uint64_t>`, one `std::uint64_t` per sample
    (FR-014).
  - The buffer allocates and first touches on the probe cpu inside the
    work item, before the window opens, the standing rule
    (`source/harness/jitter_probe.cpp:205-209`).
  - The measured loop allocates nothing, reads no file and takes no lock
    (FR-014, `source/harness/jitter_probe.cpp:313-329`).
  - A full buffer counts each lost record, the run discloses the count,
    and the run keeps going (FR-014, FR-022). The gap path's dropped
    total and dropped max counters
    (`source/harness/jitter_probe.cpp:116-119`) stand absent for the
    series: FR-014 names the count alone.
  - `FWQ` reads the time source once per unit, and the unit time stands
    at the delta between two consecutive reads (FR-012).
  - `FTQ` reads the time source once per unit and compares against the
    next edge; edge k stands at the window's first stamp plus k times
    the quantum; only a closed quantum records a count, and a trailing
    open quantum stands dropped (FR-015).
  - The standing 1,024-read measurement pass runs for every method,
    because it sets `loopPeriodNs` and the default threshold rule
    (FR-012, `source/harness/jitter_probe.cpp:50`, `:243-257`).
  - The record carries no stamp: the fold places a sample by its
    position in the series against the window edges, which the time
    source fixes (FR-015, FR-020). `GapRecord` carries a stamp
    (`source/harness/detail/jitter_probe.hpp:35-41`).
  - The transform reads the last 65,536 points of a longer series and
    discloses the count of skipped points (FR-018).
  - One producer and one consumer stand over each buffer, and the
    runner reads the buffer after the probe context is joined, the
    standing happens-before rule
    (`source/harness/detail/jitter_probe.hpp:107-110`).

## E-27: the option fields

The four fields the `--jitter-probe` value gains. They ride the standing
grammar with no new option row (FR-002, D-1).

| Field | Type | Default | Unit | Absent when |
| --- | --- | --- | --- | --- |
| `method` | `JitterMethod` | `TSC` | one of the three words | never |
| `quantumUs` | `std::uint32_t` | 100 | us | an `FTQ` run carries the value; a `TSC` run and an `FWQ` run leave it unused |
| `workCount` | `std::optional<std::uint32_t>` | no value, which names the calibration | loop bodies | an `FWQ` run carries the value; a `TSC` run and an `FTQ` run leave it unused |
| `noisePercent` | `std::uint32_t` | 1 | percent | a run with no `strict`, and a `TSC` run, leave it accepted and disclosed as unused |

- Fields: the four names above, each with a default member initializer,
  the standing rule of the struct
  (`include/speedgun-ng/benchmark.hpp:2250-2258`).
- JSON key mapping: none. The fields ride the `--jitter-probe` value
  (FR-002), and the option row stays at value 260
  (`source/harness/detail/options.hpp:87`), inside the 260 to 269 block
  spec 027 recorded (`source/harness/detail/options.hpp:77`, FR-034).
- Carrier and file: `JitterProbeOptions`
  (`include/speedgun-ng/benchmark.hpp:2248-2259`), the four fields
  appended after `tracerCpus` (`include/speedgun-ng/benchmark.hpp:2258`),
  the way spec 034 appended `precheckSerial` and spec 036 appended the
  tracer fields. The parser is `parseJitterProbeValue`
  (`source/harness/jitter_probe.cpp:795-919`).
- Invariants:
  - The head token stays first and mandatory, and a head token that
    carries `=` answers the standing cpu-token line (FR-002,
    `source/harness/jitter_probe.cpp:804-837`).
  - Each key stands once in one value, and a second occurrence is an
    invalid value (`source/harness/jitter_probe.cpp:844-847`).
  - `quantum` takes the range 10 to 100,000 us, `work` takes the range 1
    to 16,777,216 bodies, and `noise` takes the range 0 to 100 percent
    (FR-002).
  - Each row of FR-003 refuses one quoted line on standard error with
    exit status 2 before anything runs, the standing shape
    (`source/harness/cli.cpp:730-731`): a method word outside the three,
    a `quantum` value outside the range, a `quantum` value that is not a
    number, a `work` count of 0, a `work` count above 16,777,216, a
    `work` value that is not a number, a `noise` value outside the
    range, a `noise` value that is not a number, a `quantum` key with
    `method=fwq`, a `work` key with `method=ftq`, and a repeated key
    (FR-003, SC-005).
  - Numbers parse with no sign and no trailing text, the standing
    integer grammar
    (`source/harness/jitter_probe.cpp:698-719`, FR-002).
  - A bad value from an option file line names the path, the line and
    the token (`source/harness/options.cpp:1820-1841`).
  - The four fields are the first of the three exported layout moves
    FR-005 names.
  - The fields carry no bool parameter (spec Constraints, the standing
    spec-level rule).

## E-28: the `jitter` JSON key sets

The key sets the schema governs: the closed root object and the two
nested closed objects (FR-006, D-7).

- Root `jitter` object, the moved `x-order`, with the two new keys after
  `loopPeriodNs` (FR-006):

```json
["mode", "thresholdNs", "loopPeriodNs", "method", "series",
 "cpus", "stats", "droppedGaps", "disclosure"]
```

  The `required` list keeps the standing seven names
  (`docs/schema/report-v1.json:56-64`), `"additionalProperties": false`
  stays (`docs/schema/report-v1.json:65`), and the document root
  `x-order` stays unchanged (`docs/schema/report-v1.json:7`).
- One `series` item, 21 keys, closed:

```json
{
  "x-order": ["cpu", "workCount", "unitTimeNs", "targetNs", "quantumNs",
              "calibrationSpanNs", "sampleCount", "minValue",
              "medianValue", "meanValue", "p90Value", "p99Value",
              "p999Value", "p9999Value", "maxValue", "noisePercent",
              "seriesRuntimeNs", "droppedSamples", "resampleStepNs",
              "spectrumPoints", "periods"],
  "required": ["cpu", "workCount", "unitTimeNs", "targetNs",
               "calibrationSpanNs", "sampleCount", "minValue",
               "medianValue", "meanValue", "p90Value", "p99Value",
               "p999Value", "p9999Value", "maxValue", "noisePercent",
               "seriesRuntimeNs", "droppedSamples", "spectrumPoints",
               "periods"],
  "additionalProperties": false,
  "cpu": {"type": "integer"},
  "workCount": {"type": "integer"},
  "unitTimeNs": {"type": "integer"},
  "targetNs": {"type": "integer"},
  "quantumNs": {"type": "integer"},
  "calibrationSpanNs": {"type": "integer"},
  "sampleCount": {"type": "integer"},
  "minValue": {"type": "integer"},
  "medianValue": {"type": "integer"},
  "meanValue": {"type": "number"},
  "p90Value": {"type": "integer"},
  "p99Value": {"type": "integer"},
  "p999Value": {"type": "integer"},
  "p9999Value": {"type": "integer"},
  "maxValue": {"type": "integer"},
  "noisePercent": {"type": "number"},
  "seriesRuntimeNs": {"type": "integer"},
  "droppedSamples": {"type": "integer"},
  "resampleStepNs": {"type": "integer"},
  "spectrumPoints": {"type": "integer"},
  "periods": {"type": "array"}
}
```

  The `required` list carries 19 names and omits `quantumNs` and
  `resampleStepNs`, the two keys a method leaves absent, the mechanics
  the standing `stats` item carries by omitting `gaps`
  (`docs/schema/report-v1.json:98-116`).
- One `periods` item, closed:

```json
{
  "x-order": ["periodNs", "powerRatio"],
  "required": ["periodNs", "powerRatio"],
  "additionalProperties": false,
  "periodNs": {"type": "integer"},
  "powerRatio": {"type": "number"}
}
```

- The `method` key: `"type": "string"`,
  `"enum": ["TSC", "FWQ", "FTQ"]`, the shape of the standing `mode` key
  (`docs/schema/report-v1.json:67-70`).
- The per-benchmark `jitter` key keeps its open shape
  `"type": ["object", "null"]` with no key list
  (`docs/schema/report-v1.json:479-481`), so no schema edit joins there
  (FR-006). The object gains the `series` key with 20 names, the item
  above without `periods` (FR-020).
- Invariants:
  - The schema minor stands `1.12`, above master's `1.11`
    (`source/harness/json_report.cpp:43`), the minor move a key added
    inside major 1 takes
    (`specs/023-json-reports/contracts/schema-file.md:83-87`).
  - `sg::detail::checkReport` reads every written document against the
    key set, the key order and the value kinds
    (`source/harness/json_load.cpp:2613`), and a closed object refuses
    a key the schema does not list
    (`source/harness/json_load.cpp:2532-2543`).
  - A `TSC` run writes the standing keys and nothing else, so its
    document matches master apart from the `schemaVersion` value and the
    version figures (FR-006, SC-004).
  - An `FWQ` run and an `FTQ` run write `mode`, `cpus` and `disclosure`
    as today, `thresholdNs` and `loopPeriodNs` as the standing default
    rule measured them (`source/harness/jitter_probe.cpp:254-257`),
    `droppedGaps` 0, and one `stats` item per probe cpu with the 16 gap
    fields at 0 (FR-006).
  - `loadReport` reads every new key back (FR-020), beside the standing
    seven keys (`source/harness/json_load.cpp:1243-1316`) and the
    standing per-entry path (`source/harness/json_load.cpp:2178-2207`).
  - The writer keeps one `stats` item per probe cpu, the standing
    precondition (`source/harness/json_report.cpp:983-984`), and the
    standing root placement after `run` and before `precheck`
    (`source/harness/json_report.cpp:1005-1036`).

## State transitions

```text
value bound ──parseJitterProbeValue──▶ parsed | refused (exit 2, FR-003)
parsed ──method key──▶ TSC | FWQ | FTQ        (one method per run, FR-001)
TSC: the standing path of spec 027, unchanged (FR-031)
FWQ | FTQ, per probe cpu:
  idle ──calibration (<= 98 reads, FR-010)──▶ calibrated (disclosed, FR-011)
  calibrated ──measurement pass (1,024 reads, FR-012)──▶ pass measured
  pass measured ──window opens──▶ sampling
  sampling, FWQ: one read per unit, the delta is the sample (FR-012)
  sampling, FTQ: one read per unit, the edge test closes a quantum (FR-015)
  sampling ──buffer full──▶ dropping (each lost record counts, FR-014)
  sampling | dropping ──stop, drain, join──▶ folded
  folded ──run fold + per-benchmark fold (FR-020)──▶ statistics
  statistics ──resample (FWQ only, step = median unit time, FR-017)──▶ span
  statistics ──no resample (FTQ, FR-017)──▶ span
  span ──DFT (<= 65,536 points, FR-018)──▶ period list | empty list
  period list ──console block + JSON keys──▶ reported (FR-019, FR-020)
mode phases, every method (FR-004):
  mode=alongside .. the window spans the batch; strict keeps its
                    standing tsc meaning and no noise gate fires (FR-021)
  mode=precheck ... the window runs before the batch; the noise pair
                    feeds the standing lines (FR-021)
  mode=both ....... both phases over one cpu set and one buffer set
  pre-check window ends ──noise fraction >= noise= threshold, strict──▶
      one line, exit status 2, before the batch's first run (FR-021)
  pre-check window ends ──below the threshold──▶ latch, batch runs
```

The calibration and the transform stand outside every measured loop: the
calibration precedes the window and the transform follows it (FR-010,
FR-017, spec Constraints). The trailing open quantum records no count
(FR-015). No buffer mutates inside a timed window, and the fold, the
statistics and the spectrum run after the join (FR-014, FR-022).

## What is deliberately absent

- No second probe framework, probe context or timing window: the
  methods join the probe spec 027 owns (spec Out of scope, FR-004).
- No second method in one run, on one cpu or on two cpus of one run
  (FR-001, D-1).
- No new library, windowed transform, filter, chirp transform or
  autocorrelation in the spectrum (FR-017, FR-018, D-5).
- No standalone FWQ or FTQ window with no benchmark batch (spec Out of
  scope).
- No change to the `tsc` gap rules, the pre-check window of spec 034 or
  the tracer of spec 036 (FR-031, FR-004).
- No new option row and no new option value: the keys ride the standing
  value (FR-034, D-1).
- No new schema major and no `report-v2.json`: a key added inside major
  1 moves the minor (FR-006).
- No new key for a `TSC` run: the method, calibration, series and
  period keys stand absent (FR-006, SC-004).
- No second series buffer for the per-benchmark fold (FR-020).
- No `method` key and no `periods` key in the per-benchmark `jitter`
  object (FR-020).
- No raw sample list key: the closed root `jitter` object takes the
  keys FR-006 lists (`docs/schema/report-v1.json:65`).
- No reuse of `droppedGaps` for dropped samples: FR-022 names a
  dropped-sample count for every method, and the standing key keeps its
  gap meaning (FR-006).
- No reuse of `runtimeNs` for the series runtime: the series figure
  takes the name `seriesRuntimeNs` (FR-021).
- No dropped-total and dropped-max series counters: FR-014 names the
  dropped count alone.
- No stamp field in the series record: the fold places a sample by its
  position (FR-015, FR-020).
- No calibration inside the window: the span stays out of the window's
  runtime (FR-010).
- No count for a trailing open quantum (FR-015).
- No allocation, file read, syscall or lock inside the unit loop and
  the quantum loop (FR-014, spec Constraints).
- No method-aware `any` reading in the rule grammar: the uniform
  maximum stays (FR-021).
- No noise gate in `mode=alongside`: the `strict` word keeps its
  standing meaning there (FR-021).
- No sleep, clock read, fixed cpu number, or asserted noise figure of
  the real host in any test (FR-024, FR-025).
- No CI change, no gate-configuration change, no coverage exclusion,
  and no README line-cap raise (FR-034, FR-032, FR-023).
- No CI gate on the acceptance runs: a script records them (FR-028).
- No bool parameter on the public surface: one options struct carries
  the settings (spec Constraints).
- No archive `SOVERSION` move: no public counters header changes
  (FR-005).

## Open readings

`_notes/plan-datamodel.md` records three open flags. Each reading below
closes one and fixes the shared spelling.

- The calibration record carries 5 keys. The note records that the Key
  Entities list once named four and left `calibrationSpanNs` out. The
  list at `specs/043-fwq-ftq/spec.md:1149-1152` names five, and FR-011
  (`:904`) carries the span, so the record carries five keys.
- The closed `series` item carries 21 keys, in the FR-006 order
  (`specs/043-fwq-ftq/spec.md:853`). The per-benchmark copy carries 20,
  with no `method` key and no `periods` key (FR-020, `:983`).
- The item's `required` list carries 19 names and omits `quantumNs` and
  `resampleStepNs`, the two keys a method leaves absent
  (`docs/schema/report-v1.json:98-116`).
- One buffer element stays 8 bytes: `std::vector<std::uint64_t>` at
  `gapCount` entries, sized from `gaps=<n>` (FR-014,
  `include/speedgun-ng/benchmark.hpp:2192-2194`).
- `noisePercent` follows the standing integer grammar on the option
  side: the `noise=` key parses with `parseProbeNumber`
  (`source/harness/jitter_probe.cpp:698-719`), the public field stands
  `std::uint32_t`, and `noise=0.5` refuses one line with exit status 2
  (FR-002, FR-003). The JSON figure follows the standing percent shape
  `totalPercent` at `"type": "number"`
  (`docs/schema/report-v1.json:131`, E-23).
- The acceptance record shows a drop count of 0 with `--repetitions=1`
  (FR-027, `specs/043-fwq-ftq/spec.md:1072`), and the console prints 30
  new lines (FR-019, `:969`).
