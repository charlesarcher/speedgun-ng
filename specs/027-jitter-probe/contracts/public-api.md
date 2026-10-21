# Contract: the jitter probe public API

Phase 1 output. The names `include/speedgun-ng/benchmark.hpp` gains
beside `BenchmarkHandle`, with the contracts every caller and every
implementation carries (FR-002, FR-003, FR-015, FR-007, FR-009, FR-012;
the naming rules of V.1).

## The surface

| Name | Kind | Requirement |
| --- | --- | --- |
| `JitterProbeMode` | enum: `ALONGSIDE`, `PRECHECK`, `BOTH` | FR-012, R-10 |
| `JitterProbeOptions` | aggregate of six fields | FR-003, R-15 |
| `runJitterProbe(const JitterProbeOptions&)` | suite-level free function | FR-015 |
| `BenchmarkHandle::jitterProbe(const JitterProbeOptions&)` | fluent member beside the setters (`benchmark.hpp:1620` and below) | FR-015 |
| `JitterStats` | aggregate of sixteen fields | FR-002, R-05 |

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
  std::uint64_t   thresholdNs = kDefaultThresholdNs;  // 200
  JitterProbeMode mode        = JitterProbeMode::ALONGSIDE;
  std::uint32_t   gapCount    = kDefaultGapCount;     // 65,536
  std::uint32_t   precheckMs  = kDefaultPrecheckMs;   // 200
  bool            strict      = false;
};
```

| Field | Contract | Requirement |
| --- | --- | --- |
| `cpus` | the probe cpu request: an empty vector names the `idle` preset (FR-016); a non-empty vector names the probe cpus inside the process affinity mask, in the order given; the command line parses the cpu list or the word `idle` into it | FR-011, FR-016, D-4 |
| `thresholdNs` | at or above 1; the effective threshold rises to 10 times the measured loop period when that stands larger | FR-007, FR-008, R-05 |
| `mode` | one of the three enumerators; `ALONGSIDE` matches the option default | FR-012, FR-014 |
| `gapCount` | inside `kMinGapCount` (1) through `kMaxGapCount` (16,777,216) | FR-009 |
| `precheckMs` | at or above 1 | FR-014, D-13 |
| `strict` | false by default; true matches `,strict` on the option value | FR-037 |

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
  \pre `options` carries `thresholdNs` at or above 1, `gapCount` inside
       the `kMinGapCount` through `kMaxGapCount` range, and `precheckMs`
       at or above 1 (FR-008, FR-009, D-13)
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
  \post `thresholdNs` is the effective threshold: the configured value,
        or 10 times `loopPeriodNs` when that stands larger (FR-007, R-05)
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

## Consumers

- `test/source/harness_jitter_probe_test.cpp`: the threshold rule with
  the 10x loop-period floor, the kept-records fold, the no-allocation
  proof (FR-002, FR-007, FR-041).
- `test/source/harness_jitter_window_test.cpp`: the per-benchmark stats
  over that benchmark's windows, the JSON round trip (FR-021, FR-029).
- `docs/pages/harness.md` carries the field reference of `JitterStats`
  (FR-034).
