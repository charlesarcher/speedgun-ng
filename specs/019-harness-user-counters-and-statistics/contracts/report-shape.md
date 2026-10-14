# Contract: The printed row shape

`source/harness/report.cpp` prints the row shapes below. The existing
`report_shape` and `report_shape_fixtures` checks extend to the new
fields in the same commit as the printer, and every token the built-in
fields print today keeps its bytes (FR-006, FR-011, FR-013, FR-022).

## The repetition row

```text
<name>  iterations=N  real=...  cpu=...  overhead=...  endpoint samples=S  \
  <metric fields>  <counter name>=<scaled value>...  [ label=<text>]
```

| Rule | Requirement |
| --- | --- |
| The label field shall stand last on the row, separated from the field before it by two spaces, present only when the row carries a label (clarified 2026-10-14). | FR-011 |
| The metric fields shall keep their H3 bytes and positions. | FR-046 |
| Counter columns shall follow the metric fields in `counterLabels` order, each printed as the flag-applied value divided by its base. | FR-006; the counter print with `oneK` in `src/console_reporter.cc` |
| A disclosed row shall print the disclosure token beside each counter column of that row. | FR-005 |

## The aggregate row

The repetition row's fields for the aggregate values, then one line per
named counter carrying that counter's `mean=`, `median=`, `sd=`, `cv=`,
`min=`, `max=`, `n=` fields, then the family's custom statistic fields
among the built-in time statistic fields.

| Rule | Requirement |
| --- | --- |
| The built-in time statistic fields shall print unchanged. | FR-046 |
| Each named counter shall carry one statistic line in `counterLabels` order over the clean repetitions. | FR-006 |
| Custom statistic fields shall print after the built-ins, in `addStatistic` order, `TIME` values in the row unit and `PERCENTAGE` values unscaled. | FR-013, FR-014 |
| Rows hidden by a mode shall print neither in the console nor in any later output of the restricted kind. | FR-016, FR-017 |

## The fit rows and the mode filter

```text
<family>  BIG_O  <quantity> <curve> (<coefficient>)
<family>  RMS    <quantity> <rms-percentage>%
<family>  SKIPPED  <quantity>: <reason>
<family>  SKIPPED: <reason>
```

| Rule | Requirement |
| --- | --- |
| The fit pair shall print per fitted quantity after the family's aggregate row, in the order time, counters in open order, metrics in attach order. | FR-022 |
| The quantity of the primary time shall be the token `time/iter`; a counter or a metric shall carry the label its column printed under. | FR-022 |
| The curve shall be one of `1`, `N`, `N^2`, `N^3`, `lgN`, `NlgN`, or `f(N)` for a curve the family supplied. | FR-020, FR-021 |
| The coefficient of the primary time shall print in the unit the family's own rows printed under; a counter coefficient, a metric coefficient and every RMS shall print unscaled. | FR-015, FR-022 |
| A quantity that does not stand in every instance shall print the row form naming it, in place of its pair, beside the pairs of the quantities that do stand. | FR-023, spec Edge Cases |
| A family that holds no fit shall print the outcome form of FR-031 as one synthetic result named after the family, after the rows of its last instance; the rows of its instances shall print unchanged. | FR-023 |
| The aggregate row of a restricted family shall print; its repetition rows shall not, unless no aggregate row exists. | FR-016, FR-018 |

## Consumers

`tools/docs/report_shape.py` compares the README sample report block
against the quick-start binary by shape,
and the block itself stands unchanged through this feature: the
quick-start binary registers no counter, no label, no statistic, and
no fit, so the shipped shape keeps matching.
`test/example-drift-fixture/run_report_shape_fixtures.py` pins that
comparator's fixture;
`test/source/harness_aggregates_only_test.cpp` pins the filter;
`test/example_run.sh` runs every docs example and fails on a `FAILED`
row or a `SKIPPED` row without a matching expect-skip marker
(research D-15), so a broken shape fails CI on a real run
(SC-02, SC-09).
