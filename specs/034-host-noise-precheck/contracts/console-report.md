# Contract: the pre-check console block

Phase 1 output. The block `printPrecheckReport` prints before the
benchmark tables (spec FR-023, D-8). The field names are the sixteen
the console probe block uses
(`specs/027-jitter-probe` E-03: `gapCount`, `gapsPerSecond`,
`minNs`, `medianNs`, `meanNs`, `p90Ns`, `p99Ns`, `p999Ns`,
`p9999Ns`, `maxNs`, `totalNs`, `totalPercent`, `runtimeNs`,
`thresholdNs`, `loopPeriodNs`, `droppedGaps`). The wording of the
message lines stays unpinned; the tests assert the presence, the
order and the named figures.

## Block order

```text
[per measured cpu, ascending, planned cpus before candidates]
  precheck cpu <n> (planned|candidate)
  <the sixteen stats lines, one value>
  interrupts delta: <figure> (source: /proc/interrupts)
  softirq <kind> delta: <figure> (source: /proc/softirqs)   [per kind]
  switch delta: <figure>, schedule delta: <figure> (source: /proc/schedstat)
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
  pinning candidates: cpu <n> <rate> ... (lowest gap rate first)
[strict and a noisy planned cpu: the block stands unprinted]
  --jitter-probe: <the cpu, the gap rate, the max gap>
  one line on standard error, exit status 2, the error surface
```

## Rules

- The attribution lines stand beside the gap figures in each cpu
  block, each figure naming its source (spec FR-016).
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
