# Contract: the probe method fields and the series record

Phase 1 output. The delta over the standing public surface of
`specs/027-jitter-probe/contracts/public-api.md`: `JitterProbeOptions`
gains four fields, `JitterMethod` and `JitterSeriesStats` join the
surface, and `JitterEntryFold` gains the series record (spec FR-001,
FR-002, FR-020, FR-030). The two entry points, the mode machine and
every other row of the standing contract stand unchanged. The standing
file is amended in the commit that lands the code (spec FR-030).

The fields join `JitterProbeOptions` at
`include/speedgun-ng/benchmark.hpp:2248`, the series record joins
`JitterEntryFold` at `include/speedgun-ng/benchmark.hpp:645` beside
`JitterStats` at `include/speedgun-ng/benchmark.hpp:512`, and
`ReportData` at `include/speedgun-ng/benchmark.hpp:984` gains the run
record. `PrecheckCpuRecord` at `include/speedgun-ng/benchmark.hpp:576`
gains the series record. The four layout moves, over
`JitterProbeOptions`, `JitterEntryFold`, `PrecheckCpuRecord` and
`ReportData`, are the ABI check of spec FR-005: the
version stands at `0.29.0` (`CMakeLists.txt:45`), the harness
`SOVERSION` at `20` (`CMakeLists.txt:375`), and the archive
`SOVERSION` at `4` (`CMakeLists.txt:110`).

## Surface rows

Two rows join the surface table after line 16, in the standing columns
`| Name | Kind | Requirement |`:

| Name | Kind | Requirement |
| --- | --- | --- |
| `JitterMethod` | enum: `TSC`, `FWQ`, `FTQ` | specs/043 FR-001, FR-019 |
| `JitterSeriesStats` | aggregate of 19 fields, plus the period list | specs/043 FR-007, FR-011, FR-019, FR-020 |

## Options struct lines

Four lines join the struct block after line 34, the block that runs
27 to 36:

```text
JitterMethod      method       = JitterMethod::TSC;      // one method per run
std::uint32_t     quantumUs    = kDefaultQuantumUs;      // 100 us
std::uint32_t     workCount    = 0;                      // 0 names the calibration
std::uint32_t     noisePercent = kDefaultNoisePercent;   // 1 percent
```

## Field rows

Four rows join the field table after line 45, in the standing columns
`| Field | Contract | Requirement |`:

| Field | Contract | Requirement |
| --- | --- | --- |
| `method` | one of the three enumerators; `TSC` matches the option default and stands as specs/027 defines it | specs/043 FR-001, D-1 |
| `quantumUs` | inside 10 through 100,000 us; the FTQ quantum, in microseconds | specs/043 FR-002, FR-015 |
| `workCount` | 0 names the calibration of FR-010; a value stands inside 1 through 16,777,216 | specs/043 FR-002, FR-010 |
| `noisePercent` | inside 0 through 100; the strict refusal reads it in the pre-check phase | specs/043 FR-002, FR-021 |

The field carries microseconds and the JSON key `quantumNs` carries
nanoseconds, one unit per surface (spec FR-002, FR-006, FR-019). The
option field `noisePercent` is a whole number, and the series figure
`noisePercent` is a real, the standing percent shape of `totalPercent`
(spec FR-002, FR-009).

## The enforcement line

One sentence joins the `runJitterProbe` block after line 66, the block
that runs 57 to 71:

```text
The parser `parseJitterProbeValue` stands the enforcement site for the
three ranges: it refuses a `quantumUs` value outside 10 through
100,000, a `workCount` outside 1 through 16,777,216, and a
`noisePercent` outside 0 through 100, each with one quoted line at exit
status 2 (specs/043 FR-002, FR-003).
```

`runJitterProbe` gains no documented precondition, so the pair gate
finds no documented contract with no enforcement site.
`BenchmarkHandle::jitterProbe` keeps its standing line at line 85
unchanged.

## JitterSeriesStats

A new section joins after line 144, before `## Consumers` at line 146.

The per-cpu record of one method's series. It joins `JitterEntryFold`
beside `JitterStats`, so the run-level read and the per-benchmark read
share one type (spec FR-020, Key Entities).

| Field | Kind | Rule | Requirement |
| --- | --- | --- | --- |
| `sampleCount` | integer | the kept samples of the covered windows | specs/043 FR-007 |
| `minValue`, `medianValue`, `meanValue`, `p90Value`, `p99Value`, `p999Value`, `p9999Value`, `maxValue` | integer, `meanValue` real | the series figures in ns for FWQ and in counts for FTQ, the nearest-rank rule of specs/021 over the kept samples, read in place | specs/043 FR-007, FR-008 |
| `noisePercent` | real | the method's noise fraction, 0 through 100, and 100 for a maximum count of 0 | specs/043 FR-009 |
| `seriesRuntimeNs` | integer | the window span the series covers, the calibration span out | specs/043 FR-007, FR-010 |
| `droppedSamples` | integer | the records a full buffer dropped | specs/043 FR-014, FR-022 |
| `workCount`, `unitTimeNs`, `targetNs`, `quantumNs`, `calibrationSpanNs` | integer | the calibration of that probe cpu, measured before its window | specs/043 FR-010, FR-011 |
| `resampleStepNs`, `spectrumPoints` | integer | the FWQ resample step at the median unit time, and the points the transform read | specs/043 FR-017, FR-019 |
| `periods` | array | at most five closed records of `periodNs` and `powerRatio`, strongest first | specs/043 FR-018 |

The fold that fills the record carries the contracts, the standing
shape of `gapStatsOf` at line 114:

```text
seriesStatsOf(...)
  \pre the series buffer of one probe cpu is read after that probe
       context is joined (specs/027 R-04)
  \post the percentile fields follow the nearest-rank rule over the
        kept samples, read in place, and the code copies no buffer
        (specs/043 FR-008)
  \post minValue <= medianValue <= p90Value <= p99Value <= p999Value
        <= p9999Value <= maxValue
  \post the transform reads at most 65,536 points, the last ones of a
        longer series, and the skipped count stands disclosed
        (specs/043 FR-018)
```

## The standing rows this delta changes none of

- `cpus`, `thresholdNs`, `mode`, `gapCount`, `precheckMs` and
  `strict` stand as specs/027 states them, and `JitterStats` keeps its
  sixteen fields (spec FR-031).
- `runJitterProbe` and `BenchmarkHandle::jitterProbe` keep their
  shapes, their precedence rules and their standing preconditions.
- No new public header and no new target joins: the names ride
  `include/speedgun-ng/benchmark.hpp` (specs/027 FR-015).

## Gate

- `harness_fwq_ftq_test` reads the calibration stops, the series
  figures, the noise fraction and the drop rule (spec FR-007 to
  FR-017, FR-022, SC-001).
- `harness_jitter_window_test` reads the record through the JSON
  round trip and the console block (spec FR-011, FR-019, FR-020).
- `harness_cli_table_test` reads the version rows at
  `test/source/harness_cli_table_test.cpp:1382-1393` (spec FR-005,
  SC-009).
- The coverage gate reads 100.0% lines and 100.0% branches, with a
  branch shortfall of 0 rows (spec FR-032, SC-006). The 172 new branch
  positions each stand reached by a named test, so the branch allowance
  stays 21 (`tools/dbc/coverage_gate.sh:102`).

## Consumers

- `test/source/harness_fwq_ftq_test.cpp`: the calibration, the series
  statistics, the noise fraction and the resample path (spec FR-007
  to FR-017).
- `test/source/harness_jitter_window_test.cpp`: the record in the
  document and the fold over one benchmark's windows (spec FR-020).
- `docs/pages/harness.md` carries the field reference beside the
  standing jitter section (spec FR-029).
- `specs/027-jitter-probe/contracts/public-api.md` gains these rows
  in the same commit as the code (spec FR-030).
