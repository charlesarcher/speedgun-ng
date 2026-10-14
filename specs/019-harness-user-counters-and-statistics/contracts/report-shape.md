# Contract: The printed row shape

`source/harness/report.cpp` prints the row shapes below. The existing
`report_shape` and `report_shape_fixtures` checks extend to the new
fields in the same commit as the printer, and every token the built-in
fields print today keeps its bytes (FR-006, FR-011, FR-013, FR-022).

## The repetition row

```text
<name>[ label=<text>]  iterations=N  real=...  cpu=...  overhead=...  \
  endpoint samples=S  <metric fields>  <counter name>=<scaled value>...
```

| Rule | Requirement |
| --- | --- |
| The label field shall follow the name, separated by two spaces, present only when the row carries a label. | FR-011 |
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
```

| Rule | Requirement |
| --- | --- |
| The fit pair shall print per fitted quantity after the family's aggregate row, in the order time, counters in open order, metrics in attach order. | FR-022 |
| The aggregate row of a restricted family shall print; its repetition rows shall not, unless no aggregate row exists. | FR-016, FR-018 |

## Consumers

`tools/docs/report_shape.py` compares the README sample report block
against the quick-start binary by shape and
`test/example-drift-fixture/run_report_shape_fixtures.py` pins that
comparator's fixture;
`test/source/harness_aggregates_only_test.cpp` pins the filter;
`test/example_run.sh` runs every docs example and fails on a `FAILED` or
`SKIPPED` row, so a broken shape fails CI on a real run (SC-02, SC-09).
