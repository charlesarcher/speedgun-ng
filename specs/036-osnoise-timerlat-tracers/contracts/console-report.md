# Contract: the tracer lines of the pre-check console block

Phase 1 output. The delta over the standing pre-check console block of
`specs/034-host-noise-precheck/contracts/console-report.md`:
`printPrecheckReport` prints the tracer lines beside the standing
block, and every standing line stands byte-identical (spec FR-012,
FR-013, D-3). The standing contract's field names, block order and
rules stand unchanged; the amendment this delta names joins the
standing file in the commit that lands the code (spec FR-024).

## Line shape

```text
[after the host-settings lines, before the warning lines]
  tracer: engine <word> cpus <list>
  tracer cpu <n> nmi: <figure>
  tracer cpu <n> irq: <figure>
  tracer cpu <n> softirq: <figure>
  tracer cpu <n> thread: <figure>
  tracer cpu <n> hardware: <figure>
      [one line per source per tracer cpu, the sources in the NMI,
       IRQ, softirq, thread, hardware order, cpus ascending]
[the not-available case]
  tracer: not available
```

## Rules

- The header line names the engine word the value carried and the cpus
  the tracer ran on (spec FR-013, D-3).
- One figure line stands per source per tracer cpu, the sources in the
  NMI, IRQ, softirq, thread, hardware order, cpus ascending (spec
  FR-013).
- A figure the engine states nothing for prints the standing
  `not available` wording in the figure position, for that source
  alone (spec FR-010, the standing rule of specs/034).
- The not-available case stands one line, the tracer and the
  disclosure: no tracefs mount at the standing path, no tracer engine,
  or no rights for the setup writes (spec FR-016, D-5, SC-004). The
  standing block stands unchanged around it, and the exit status stays
  0.
- A run that requests no tracer prints no tracer line, and every
  standing line stands byte-identical to master's (spec FR-012,
  FR-015).
- The wording of the message lines stays unpinned; the tests assert
  the presence, the order and the named figures, the standing rule of
  the pre-check block.
- The strict refusal never reaches this printer: the missing-window
  refusal stands the one line of the standing error surface on
  standard error at status 2, ahead of everything (spec FR-011), and
  the strict gate keeps gating the planned cpus' gap figures alone,
  the tracer figures gating nothing (spec Out of scope).
