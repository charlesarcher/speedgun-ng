# Contract: the jitter probe public API

Phase 1 output. The names `include/speedgun-ng/benchmark.hpp` gains
beside `BenchmarkHandle`, with the contracts every caller and every
implementation carries (FR-002, FR-003, FR-015, FR-007, FR-009, FR-012;
the naming rules of V.1).

## The surface

| Name | Kind | Requirement |
| --- | --- | --- |
| `JitterProbeMode` | enum: `ALONGSIDE`, `PRECHECK`, `BOTH` | FR-012, R-10 |
| `JitterProbeOptions` | aggregate of ten fields | FR-003, R-15, specs/043 FR-005 |
| `runJitterProbe(const JitterProbeOptions&)` | suite-level free function | FR-015 |
| `BenchmarkHandle::jitterProbe(const JitterProbeOptions&)` | fluent member beside the setters (`benchmark.hpp:1620` and below) | FR-015 |
| `JitterStats` | aggregate of sixteen fields | FR-002, R-05 |
| `JitterMethod` | enum: `TSC`, `FWQ`, `FTQ` | specs/043 FR-001, FR-019 |
| `JitterSeriesStats` | aggregate of 19 fields, plus the period list | specs/043 FR-007, FR-011, FR-019, FR-020 |

No new public header and no new target: the names join the header beside
`BenchmarkHandle` (FR-015, R-01). No entry point takes a bool (FR-003):
the strict flag is one field of the options (R-15). The types take
PascalCase, the functions camelBack, the constants the `k` prefix, and
the public members take camelBack prefix free (V.1: N-1, N-2, N-8, N-12;
R-15).

## JitterProbeOptions

```text
struct JitterProbeOptions {
  std::vector<std::uint32_t> cpus = {};               // empty names the idle preset
  std::optional<std::uint64_t> thresholdNs = std::nullopt;  // the default rule
  JitterProbeMode mode        = JitterProbeMode::ALONGSIDE;
  std::uint32_t   gapCount    = kDefaultGapCount;     // 65,536
  std::uint32_t   precheckMs  = kDefaultPrecheckMs;   // 200
  bool            strict      = false;
  JitterMethod      method       = JitterMethod::TSC;      // one method per run
  std::uint32_t     quantumUs    = kDefaultQuantumUs;      // 100 us
  std::uint32_t     workCount    = 0;                      // 0 names the calibration
  std::uint32_t     noisePercent = kDefaultNoisePercent;   // 1 percent
};
```

| Field | Contract | Requirement |
| --- | --- | --- |
| `cpus` | the probe cpu request: an empty vector names the `idle` preset (FR-016); a non-empty vector names the probe cpus inside the process affinity mask, in the order given; the command line parses the cpu list or the word `idle` into it | FR-011, FR-016, D-4 |
| `thresholdNs` | a value at or above 1 stands as given, an explicit 200 ns staying exactly 200 ns; no value names the default rule: the 200 ns default, or 10 times the measured loop period when that stands larger | FR-007, FR-008, R-05 |
| `mode` | one of the three enumerators; `ALONGSIDE` matches the option default | FR-012, FR-014 |
| `gapCount` | inside `kMinGapCount` (1) through `kMaxGapCount` (16,777,216) | FR-009 |
| `precheckMs` | at or above 1 | FR-014, D-13 |
| `strict` | false by default; true matches `,strict` on the option value | FR-037 |
| `method` | one of the three enumerators; `TSC` matches the option default and stands as specs/027 defines it | specs/043 FR-001, D-1 |
| `quantumUs` | inside 10 through 100,000 us; the FTQ quantum, in microseconds | specs/043 FR-002, FR-015 |
| `workCount` | 0 names the calibration of FR-010; a value stands inside 1 through 16,777,216 | specs/043 FR-002, FR-010 |
| `noisePercent` | inside 0 through 100; the strict refusal reads it in the pre-check phase | specs/043 FR-002, FR-021 |

Validation: the command line rejects a value outside these bounds with
one line and status 2 before the options stand up (`cli-options.md`); a
library caller who passes an options object outside them violates a
precondition of the entry point, and the contract facility aborts at the
call (Principle II). A default-constructed `JitterProbeOptions` names
the `idle` preset: an empty `cpus` field lets the resolver choose the
cpus, and a non-empty field states them (D-4, FR-016).

## runJitterProbe

```text
runJitterProbe(const JitterProbeOptions& options)
  \pre the run has not started for any benchmark (E-02, FR-015)
  \pre `options.cpus` carries the probe cpu request: an empty vector
       names the `idle` preset, a non-empty vector names cpus inside
       the process affinity mask (FR-011, FR-016, D-4)
  \pre `options` carries a `thresholdNs` value at or above 1 when the
       field carries one, `gapCount` inside the `kMinGapCount` through
       `kMaxGapCount` range, and `precheckMs` at or above 1 (FR-008,
       FR-009, D-13)
  The parser `parseJitterProbeValue` stands the enforcement site for the
  three ranges: it refuses a `quantumUs` value outside 10 through 100,000,
  a `workCount` outside 1 through 16,777,216, and a `noisePercent` outside
  0 through 100, each with one quoted line at exit status 2 (specs/043
  FR-002, FR-003).
  \post the suite's next run starts the probe before its first selected
        instance and stops, drains and joins it after the last, over
        these options, the way the command-line option does (FR-006, R-01)
  \post the command-line value wins when both stand (FR-015, R-01)
```

The availability rule of FR-037 binds the probe the run starts, whatever
entry point configured it: the run discloses `not available` and
continues with status 0, and `strict` stops it with status 2.

## BenchmarkHandle::jitterProbe

```text
BenchmarkHandle::jitterProbe(const JitterProbeOptions& options)
  -> BenchmarkHandle&
  \pre the handle names a registered entry and the run has not started
       for this benchmark (E-02, FR-015)
  \pre `options` carries the validation the suite-level call states
       (FR-008, FR-009, D-13)
  \post the entry carries the options in place of any set an earlier
        jitterProbe call stated, and the returned handle is this handle
        (FR-015)
  \post the run of that family starts the probe before its first
        instance and stops it after the last (FR-006, FR-015)
```

Where the handle and the command line both stand, the handle wins, the
standing rule for a benchmark value over the command line (FR-015);
where the suite-level call and the command line both stand, the
command-line value wins (FR-015, R-01).

## JitterStats

The sixteen fields of data-model.md E-03, in the E-03 order that table
is the canonical enumeration of; this contract quotes no second list
(spec Assumptions records where the order came from, and the rebase
note re-checks it against the roadmap file when that file is
reachable). The counts and the ns durations carry unsigned integers,
the gap record's own width (D-6); `gapsPerSecond`, `meanNs` and
`totalPercent` carry real values, the kinds the E-03 table states per
field.

The fold that fills the struct (`gapStatsOf`, R-15) carries the
contracts; one fold runs over the whole run, one over each benchmark's
windows (FR-021):

```text
gapStatsOf(...)
  \pre the buffer of one probe cpu is read after that probe context is
       joined (R-04)
  \post the percentile fields follow the nearest-rank rule of
        `source/harness/percentiles.cpp` over the sorted kept gaps
        (PC-4, R-05)
  \post minNs <= medianNs <= p90Ns <= p99Ns <= p999Ns <= p9999Ns <= maxNs
  \post `gapCount` counts the kept gaps and `droppedGaps` counts the
        gaps the buffer dropped (FR-010, D-6)
  \post `totalPercent` divides the kept gaps' total by the covered
        duration: the probe's `runtimeNs` for the run-level fold, the
        benchmark's window total for the per-benchmark fold (E-03, R-05)
  \post `gapsPerSecond` divides `gapCount` by the seconds of the
        covered windows' span: the probe's `runtimeNs` for the
        run-level fold, the benchmark's window total for the
        per-benchmark fold (E-03, R-05)
  \post `thresholdNs` is the effective threshold: the configured value
        when the field carries one, or the larger of the 200 ns default
        and 10 times `loopPeriodNs` when it names none (FR-007, R-05)
  \post `loopPeriodNs` is the minimum loop period the probe measured in
        the fixed measurement pass data-model.md E-05 specifies
        (FR-007, R-05)
  \post `runtimeNs` is the span the loop measured from its start to the
        read of the stop flag, through the same reader (R-05)
```

`JitterProbeMode` is a closed set of three enumerators inside
`JitterProbeOptions`'s scope of naming; the pre-check phase runs when
the mode is `PRECHECK` or `BOTH`, the alongside phase when the mode is
`ALONGSIDE` or `BOTH`, and the overlap check binds the alongside phase
only (FR-014, D-3, R-10).

## JitterSeriesStats

specs/043-fwq-ftq: the per-cpu record of one method's series. It joins
`JitterEntryFold` beside `JitterStats`, so the run-level read and the
per-benchmark read share one type (specs/043 FR-020, Key Entities).

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

The fold that fills the record carries the contracts, the standing shape
of `gapStatsOf`:

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

The field carries microseconds and the JSON key `quantumNs` carries
nanoseconds, one unit per surface (specs/043 FR-002, FR-006, FR-019). The
option field `noisePercent` is a whole number, and the series figure
`noisePercent` is a real, the standing percent shape of `totalPercent`
(specs/043 FR-002, FR-009).

## Consumers

- `test/source/harness_jitter_probe_test.cpp`: the threshold rule with
  the 10x loop-period floor, the kept-records fold, the no-allocation
  proof (FR-002, FR-007, FR-041).
- `test/source/harness_jitter_window_test.cpp`: the per-benchmark stats
  over that benchmark's windows, the JSON round trip (FR-021, FR-029).
- `docs/pages/harness.md` carries the field reference of `JitterStats`
  (FR-034).
