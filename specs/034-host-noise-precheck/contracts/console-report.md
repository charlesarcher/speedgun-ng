# Contract: the pre-check console block

Phase 1 output. The block `printPrecheckReport` prints before the
benchmark tables (spec FR-023, D-8). The field names are the nineteen
the console probe block uses
(`specs/027-jitter-probe` E-03: `gapCount`, `gapsPerSecond`,
`minNs`, `medianNs`, `meanNs`, `p90Ns`, `p99Ns`, `p999Ns`,
`p9999Ns`, `maxNs`, `totalNs`, `totalPercent`, `runtimeNs`,
`thresholdNs`, `loopPeriodNs`, `droppedGaps`, `smiDelta`, `nmiDelta`,
`smiGapCount`). The block prints the sixteen gap names as stats lines,
and the three detector figures as attribution lines of a `hwlat` run
(specs/048 FR-016). The wording of the message lines stays unpinned;
the tests assert the presence, the order and the named figures.

## Block order

```text
[per measured cpu, ascending, planned cpus before candidates]
  precheck cpu <n> (planned|candidate)
  <the sixteen stats lines, one value>
  interrupts delta: <figure> (source: /proc/interrupts)
  softirq <kind> delta: <figure> (source: /proc/softirqs)   [per kind]
  switch delta: <figure>, schedule delta: <figure> (source: /proc/schedstat)
  smi delta: <figure> (source: /dev/cpu/<n>/msr)          [specs/048, per measured
  nmi delta: <figure> (source: /proc/interrupts)           cpu of a `hwlat` run; the
  smi gaps: <count>                                        count names no source]
[once]
  host settings: isolcpus=<...> nohz_full=<...> rcu_nocbs=<...>
  governor: cpu <n> <name> ... (source: /sys .../scaling_governor)
  irq affinity: cpu <n> <irq list> (source: /proc/irq)
  tracer: engine <word> cpus <list>                       [specs/036, a run whose tracer ran]
  tracer cpu <n> nmi: <figure>                             [per source per tracer cpu, the
  tracer cpu <n> irq: <figure>                              sources in the NMI, IRQ, softirq,
  tracer cpu <n> softirq: <figure>                          thread, hardware order, cpus
  tracer cpu <n> thread: <figure>                           ascending]
  tracer cpu <n> hardware: <figure>
  tracer: not available                                    [specs/036, the not-available case]
[after the blocks, the non-strict path]
  warning: cpu <n> gapsPerSecond <figure> maxNs <figure>     [per noisy planned cpu]
  warning: cpu <n> noisePercent <figure> maxValue <figure>   [FWQ, per noisy planned cpu, specs/043]
  warning: cpu <n> noisePercent <figure> minValue <figure>   [FTQ, per noisy planned cpu, specs/043]
  pinning candidates: cpu <n> <rate> ... (lowest gap rate first)
  pinning candidates: cpu <n> <noisePercent> ... (lowest noise fraction first, specs/043)
[the fold block, after the benchmark tables, FWQ or FTQ, specs/043]
  jitter probe: cpus <list>
  <the sixteen gap field lines, at 0>
  <the 26 series lines that follow the calibration group, one column per
   probe cpu>
  jitter: probe on cpus <list>
  jitter: cpu <n> kept <n> dropped <n>       [per probe cpu]
  jitter: split and percentiles use the kept gaps only
[strict and a noisy planned cpu: the block stands unprinted]
  --jitter-probe: <the cpu, the gap rate, the max gap>
  one line on standard error, exit status 2, the error surface
```

## The series lines

A run that names `method=fwq` or `method=ftq` gains 30 lines, one field
each, one column per probe cpu in the standing cpu order, each field
printing once (specs/043 FR-019). The calibration group of four prints
ahead of the batch's first run, behind the probe start's latch, and the
fold block after the benchmark tables prints the 26 that follow
(specs/043 FR-011, FR-019). A `tsc` run keeps no series and prints none
of them, so every standing line of this contract stays byte-identical
for it (specs/043 FR-031, SC-004). The order:

1. The calibration group: `method`, `workCount`, `targetNs` and
   `calibrationSpanNs`.
2. The 16 figures `sampleCount`, `minValue`, `medianValue`, `meanValue`,
   `p90Value`, `p99Value`, `p999Value`, `p9999Value`, `maxValue`,
   `noisePercent`, `seriesRuntimeNs`, `droppedSamples`, `unitTimeNs`,
   `quantumNs`, `resampleStepNs` and `spectrumPoints` (specs/043 FR-021).
3. `period1Ns` through `period5Ns`, then `period1Power` through
   `period5Power`.

The console spells the method `tsc`, `fwq`, `ftq` and `hwlat`, the
words the value takes, and the JSON spells it `TSC`, `FWQ`, `FTQ` and
`HWLAT` (specs/043 FR-019, specs/048 FR-002, FR-004). A rank a cpu
does not carry prints 0, and the drop count stands disclosed for every
method (specs/043 FR-019, FR-022).

## Rules

- The attribution lines stand beside the gap figures in each cpu
  block, each figure naming its source (spec FR-016).
- The field group carries the nineteen names of the console probe
  block in two shapes: the sixteen gap names stand as stats lines, and
  the three detector figures stand as attribution lines of a `hwlat`
  run, the two delta lines naming their file and the count naming none
  (specs/048 FR-016).
- A source that is missing, unreadable or unparsable prints the
  `not available` wording for that source alone; the block never
  fails the run on it (spec FR-018, AC-b2).
- A softirq kind that cannot stand parsed reads `not available` for
  that kind alone (the clarify answer of 2026-10-27).
- The host settings print once per run, `not available` where the
  host does not state the setting (spec FR-017).
- The pinning line names every planned cpu with its gap rate, the
  lowest rate first; tied cpus stand in cpu order (spec FR-013, the
  clarify answer).
- The pre-check pair stands the method's own: the noise fraction and
  the maximum unit time for FWQ, the noise fraction and the minimum
  count for FTQ. The figure's key is `noisePercent`. The warning line
  prints for a planned cpu whose noise fraction stands at or above
  `noise=`, 1 percent by default. The strict refusal reads the same pair
  against the same number, and a cpu under it warns nothing
  (specs/043 FR-021, D-9, R-15).
- The pinning-candidate line ranks by the noise fraction ascending,
  ties in cpu order, for FWQ and FTQ; the standing gap-rate ranking
  stays for `tsc` (specs/043 FR-021).
- The strict refusal stands after the full window (every window in
  serial mode), names the first noisy planned cpu in cpu order with
  the two figures, and exits 2 before any benchmark starts; the
  pre-check block stands unprinted, the refusal standing the one
  line of the standing error surface on standard error, and no
  warning or pinning line prints (spec FR-014, the clarify answer,
  the remediation of 2026-10-27).
- A run with no noisy planned cpu prints no warning line; the
  pinning line prints regardless (spec FR-012, FR-013).
- A run with no candidate cpu stands disclosed once beside the
  host settings; the pinning line names the planned cpus alone
  (spec Edge Cases, the empty-candidate-set answer).
- The tracer lines of specs/036 stand after the host-settings lines
  and before the warning lines: one header line naming the engine
  word and the cpus, then one figure line per source per tracer cpu,
  the sources in the NMI, IRQ, softirq, thread, hardware order, cpus
  ascending; an absent figure stands the `not available` wording for
  that source alone (specs/036 FR-010, FR-013). The not-available
  case stands one line, `tracer: not available`; the exit status
  stays 0 (specs/036 FR-016). A run that requests no tracer prints
  no tracer line, and every standing line stands byte-identical
  (specs/036 FR-012, FR-015).
- The `sg::NoiseWarning` disclosure of the standing report stands
  beside this block unchanged (spec FR-025).
- Each of the 30 series fields prints once, one column per probe cpu in
  the standing cpu order, and a rank a cpu does not carry prints 0
  (specs/043 FR-019).
- The calibration lines print on the probe cpu before that cpu's window
  starts, and the calibration span stays out of the window's runtime
  (specs/043 FR-010, FR-011).
