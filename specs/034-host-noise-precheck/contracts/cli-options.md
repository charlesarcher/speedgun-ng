# Contract: the `precheck` value grammar

Phase 1 output. The delta over the standing value grammar of
`specs/027-jitter-probe/contracts/cli-options.md`: the `precheck`
key gains the word `serial` beside the millisecond value (spec
FR-002). The option table, the mode list, the lane constants and
every other row of that contract stand unchanged.

## Grammar delta

```text
precheck=<ms>|serial
```

| Form | Meaning | Default | Requirement |
| --- | --- | --- | --- |
| `precheck=<ms>` | the pre-check duration in milliseconds, at or above 1 and below 2³² (standing) | 200 ms | specs/027 FR-014, D-13 |
| `precheck=serial` | the pre-check runs one measured cpu per window, ascending cpu order, the planned cpus before the candidates | absent: all measured cpus run in one window | spec FR-009, D-2 |

The two forms stand exclusive: one `precheck=` key names one form.

## Error-surface delta

Every invalid value prints one line on standard error and exits with
status 2, before the first run starts, the standing error shape of
`specs/027-jitter-probe/contracts/cli-options.md`:

| Case | The line names | Status |
| --- | --- | --- |
| A `precheck` value that is neither a number nor the word `serial` | the value | 2 |
| A `precheck` value out of the standing bounds (`0`, a value at or above 2³²) | the value | 2 |
| A misspelled serial word (for example `searial`) | the value | 2 |

Message shape: the standing `--jitter-probe: <message>` line,
carrying the offending token and the reason; from an option file
line the shape is `<path>:<line>: --jitter-probe: <message>`. Tests
assert the exit status, the one-line count and the prefix; the
wording stays unpinned.

## The standing rows this delta changes none of

- The record shape of the jitter probe, the `idle` preset and the
  overlap rule stand as the contract of specs/027 states them
  (`specs/027-jitter-probe/contracts/cli-options.md`, spec FR-002).
- The option table gains no row; `kJitterProbeOptionValue` stays
  `260`; every value in the option table stays unique.
- `strict` keeps its standing meaning and gates the pre-check noise
  refusal beside the unavailable-TSC refusal (spec D-1).
