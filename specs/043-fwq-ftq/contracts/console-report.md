# Contract: the series lines of the jitter console block

Phase 1 output. The delta over the standing pre-check console block of
`specs/034-host-noise-precheck/contracts/console-report.md`: the
per-cpu block gains the calibration lines, and the fold block after the
benchmark tables gains 30 series lines (spec FR-011, FR-019, FR-021).
Every standing line stands byte-identical for a `tsc` run (spec
FR-031, SC-004). The standing field names, the block order and the
`not available` rules stand unchanged. The standing file is amended in
the commit that lands the printer (spec FR-030).

## Calibration lines

Inside the block order at lines 15 to 39, after line 18, the line
`<the sixteen stats lines, one value>`:

```text
[FWQ or FTQ, per measured cpu]
  method: <tsc|fwq|ftq>  workCount: <n>  targetNs: <n>  calibrationSpanNs: <n>
  unitTimeNs: <n>  quantumNs: <n>  resampleStepNs: <n>  spectrumPoints: <n>
  sampleCount: <n>  minValue: <n>  medianValue: <n>  meanValue: <n>
  p90Value: <n>  p99Value: <n>  p999Value: <n>  p9999Value: <n>  maxValue: <n>
  noisePercent: <n>  seriesRuntimeNs: <n>  droppedSamples: <n>
```

The console spells the method `tsc`, `fwq` and `ftq`, the words the
value takes, and the JSON spells it `TSC`, `FWQ` and `FTQ` (spec
FR-019, [contracts/report-json.md](./report-json.md)).

## The fold block

After line 35, the pinning line:

```text
[the fold block, after the benchmark tables, FWQ or FTQ]
  jitter probe: cpus <list>
  <the sixteen gap field lines, at 0>
  <the 30 series lines, one column per probe cpu>
  period1Ns .. period5Ns   period1Power .. period5Power
  jitter: probe on cpus <list>
  jitter: cpu <n> kept <n> dropped <n>       [per probe cpu]
  jitter: split and percentiles use the kept samples only
```

The 30 lines carry one field each, in this order (spec FR-019,
FR-021):

1. `method`, `workCount`, `targetNs` and `calibrationSpanNs`.
2. The 16 figures `sampleCount`, `minValue`, `medianValue`,
   `meanValue`, `p90Value`, `p99Value`, `p999Value`, `p9999Value`,
   `maxValue`, `noisePercent`, `seriesRuntimeNs`, `droppedSamples`,
   `unitTimeNs`, `quantumNs`, `resampleStepNs` and `spectrumPoints`.
3. `period1Ns` through `period5Ns`, then `period1Power` through
   `period5Power`.

## Rules

Two rules join the `## Rules` list after line 61:

```markdown
- The pre-check pair stands the method's own: the noise fraction and
  the maximum unit time for FWQ, the missing share and the minimum
  count for FTQ. The warning rule and the strict refusal read that
  pair against `noise=`, 1 percent by default (specs/043 FR-021).
- The pinning-candidate line ranks by the noise fraction ascending,
  ties in cpu order, for FWQ and FTQ; the standing gap-rate ranking
  stays for `tsc` (specs/043 FR-021).
```

Two rules join after line 76:

```markdown
- The calibration lines print on the probe cpu before that cpu's
  window starts, and the calibration span stays out of the window's
  runtime (specs/043 FR-010, FR-011).
- Each of the 30 fields prints once, one column per probe cpu in the
  standing cpu order, and a rank a cpu does not carry prints 0
  (specs/043 FR-019).
```

## The standing lines this delta changes none of

- The sixteen stats lines, the attribution deltas, the host settings,
  the tracer lines of specs/036, the warning lines and the pinning
  line keep their shapes (spec FR-031).
- The strict refusal keeps its one line on standard error at status
  2, ahead of the block (specs/034 FR-014).
- The `not available` wording stands for a source the host states
  nothing for, the standing rule of this contract (specs/034 FR-018).
- The wording of the message lines stays unpinned; the tests assert
  the presence, the order and the named figures.

## Gate

- `harness_jitter_window_test` reads the 30 lines, their order, and
  the zero that stands for a rank a cpu omits (spec FR-019, FR-022).
- `harness_precheck_test` reads the pre-check pair, the ranking and
  the noise refusal (spec FR-021).
- `report_shape_fwq_ftq_sample` folds the recorded block against a
  real run of the example, the shape of `report_shape_jitter_sample`
  at `test/CMakeLists.txt:1913-1921` (spec FR-029, SC-007).

## Consumers

- `test/source/harness_jitter_window_test.cpp`: the 30 lines, the
  calibration lines, and the drop disclosure (spec FR-011, FR-019,
  FR-022).
- `test/source/harness_precheck_test.cpp`: the pair, the ranking and
  the refusal (spec FR-021).
- `example/docs/fwq_ftq.expected` records the block for the fold (spec
  FR-029).
- `specs/034-host-noise-precheck/contracts/console-report.md` gains
  these lines in the same commit as the printer (spec FR-030).
