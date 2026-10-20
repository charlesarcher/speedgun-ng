# Contract: the topology line and the placement field

Phase 1 output. What the console report gains, and what stays byte for
byte (FR-020 to FR-024).

## The context block

```text
speedgun-ng 0.12.0 build=Release
topology: packages=1 l3-groups=4 cores=24 threads=48
bmSum                    iterations=   1066261  real time/iter (ns)=  657.575 ...
```

| Rule | Requirement |
| --- | --- |
| The topology line prints once per report, directly below the version and build-type line, when at least one selected instance runs with a preset other than `none` over a run of more than one context (FR-019). | FR-020, R-09 |
| The four counts equal the layout the harness read, and no other line carries them. | FR-020 |
| A report whose instances all run with `none`, or with no preset at all, prints the block exactly as feature 020 ships it: no topology line. | FR-022, SC-05 |
| The line reads no clock and adds no sampling action to any run. | FR-024 |

## The placement field

The disclosure line of feature 020 keeps its shape:

```text
  contexts=4  ctx0=SPIN/0@0/applied  ctx1=SPIN/6@1/applied  ctx2=SPIN/12@2/applied  ctx3=SPIN/18@3/applied
```

| Rule | Requirement |
| --- | --- |
| The placement field renders as `cpu@group`: the cpu index the slot named, `@`, and the L3 group ordinal. | FR-021, Clarifications Q-2 |
| A layout that names no group for a cpu renders the placement field as the cpu index alone, with no `@` part. | FR-021, E-09 |
| `NO_PIN` keeps the bytes feature 020 prints, and a no-pin context still closes its field after the placement name. | FR-021, FR-022 |
| The field count stays three for a pinned context and two for a no-pin context, and the placement value set of `contracts/report-disclosure.md` of feature 020 gains `cpu@group` in the same commit as the printer. | FR-021 |
| The field width stays `kPlacementFieldWidth`, `digits10 + 1` of `std::uint32_t`, and the standing cut mark appears only if a text exceeds it. | R-06, Risks |
| The `applied` and `unapplied` account keeps its meaning: it reports whether the pin landed, whatever the text says. | feature 020 FR-005 |

## The noise warning

The warning condition of feature 020 FR-026 stays: two spinning contexts
naming the same cpu index, or more spinning contexts than the mask holds.
A sibling pair names two cpus, so `smt` prints no warning, and an
oversubscribed preset cannot reach a run at all because the fit check
stops it first (FR-023, FR-018).

## Gate movement

`tools/docs/report_shape.py` needs no edit: it stores no expectations of its
own, and the expectations it compares, `example/first_benchmark.expected#report`
at `README.md:80`, `example/docs/reading_the_report.expected#report` at
`docs/pages/harness.md:572`, and the third comparison over
`example/docs/percentiles.expected#report` at `:857`, come from runs with no
preset in play, so their bytes stand. The README "Reading the report"
region does not move: the drift gate generates it from
`example/docs/reading_the_report.cpp`, which runs with no preset, so its
bytes stand. Every token the built-in fields print today keeps its bytes,
and the new line is one added token row.

## Consumers

- `test/source/harness_placement_report_test.cpp`: the topology line
  present with a preset and absent without one; the `cpu@group` field for
  each context; the no-preset block byte for byte against the standing
  expectation; the noise warning unchanged for a sibling pair.
- `example/docs/reading_the_report.cpp` keeps its region unchanged: that
  example runs with no preset, so its bytes stand.
