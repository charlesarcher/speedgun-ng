# Contract: Report Shape

The run row keeps every field it prints today, in the same order. The
percentile lines follow it, and one field's value changes.

## The run row

```text
<name>  iterations=<n>  real time/iter (<unit>)=<v>  cpu time/iter (<unit>)=<v>
        overhead floor (<unit>)=<v>  endpoint samples=<n>  [<counter columns>]
```

The `overhead floor` field keeps its name and keeps holding
`Plan::sampleOverheadNsMedian()`, the median cost of one `sample()` action.
That is the observer cost FR-017 asks for, and no new field enters the row
(D-11).

The `endpoint samples` field keeps its name and now counts the points the
chunked cadence leaves: `chunks + 1` for a run with no pause, plus the pause
points. Its value changes for every run, because the cadence it counts
changed (D-10). The spec's FR-023 and SC-002 claim the report keeps its bytes
with the output off; the claim that holds is that no percentile line prints,
and the analyze pass records the correction.

## The chunk disclosure line

One line per run, printed after the run row when the output stands on:

```text
<name>  <right-aligned tag>  chunks=<n>  iterations/chunk=<n>
```

## The percentile lines

One line per reported column, in the order the row reports its columns: the
primary time, then the time column the primary is not (the real time when the
primary is the cpu fold, the default of FR-002 and D-05, and the cpu time when
the family asked for the real fold), then each counter column.

```text
<name>  <right-aligned tag>  p50=<v>  p90=<v>  p99=<v>  p99.9=<v>
```

A value the run cannot answer prints as `not available` in its field, the
token FR-013 defines, and no number stands beside it. The unit of a value is
the unit that column already prints in, and a primary time value is already
divided by `chunkIterations` (FR-002).

## The aggregate row

Unchanged: `mean=`, `median=`, `sd=`, `cv=`, `min=`, `max=`, `n=`, and the
custom statistics. It carries no percentile line (FR-010).
`test/source/harness_statistics_test.cpp` keeps its assertion that the
percentile form stays out of that row.

## Threaded runs

The merged row prints its own chunk disclosure and percentile lines, built
from the union of the contexts' chunk values. Under the context disclosure
the report already prints, each context prints its own percentile line, built
from its own values (FR-014, D-12).

```text
bmFill/8/threads:2  iterations=...  real time/iter (ns)=...  endpoint samples=177
bmFill/8/threads:2  <tag>  chunks=176  iterations/chunk=34050
bmFill/8/threads:2  <tag>  p50=    0.445  p90=    0.501  p99=not available
  contexts=2  ctx0=SPIN/NO_PIN  ctx1=SPIN/NO_PIN
bmFill/8/threads:2  <tag>  ctx0 p50=    0.444  p90=    0.500  p99=not available
bmFill/8/threads:2  <tag>  ctx1 p50=    0.446  p90=    0.502  p99=not available
```

## Output off

With the output off, no chunk disclosure line and no percentile line prints,
and the run row keeps its fields. The `endpoint samples` value still reflects
the chunked cadence, because the cadence does not depend on the output
setting (FR-001, D-10).

## Consequence for the pinned expectations

`example/docs/reading_the_report.expected` and the README "Reading the
report" sentence carry the endpoint-sample count, so both move with the
cadence. The drift gate and the `example_run` gate are the checks that catch
a missed one.
