# Contract: The Metric Command-Line Option

**Capability**: C-4 harness use | **Plan**: D-12 | **Owner**: `source/harness/cli.cpp`, `source/harness/runner.cpp`

## Surface

```text
--metric <name>      Add one metric column, repeatable
```

The option joins the long-option table in `source/harness/cli.cpp` beside
`--counter`. It may be repeated. A value with no name, an empty value and an
unknown option keep the behaviour the table already has: one line printed and
exit status 2 (FR-042).

## Guarantees

- Each named metric that the host carries adds one report column through the
  column path the counter option already uses (FR-026). The column label is
  the name as typed, and the row carries the `ratio=`, `scaled=` and `gap=`
  fields the metric columns already print (FR-025).
- Columns appear in command-line order, so `--metric l2_mpi --metric cpi`
  prints `l2_mpi` first (clarify session of 2026-10-19).
- The same name twice yields one column (FR-026). A metric attached in code
  and named again on the command line also yields one column: the name is the
  key.
- A metric that exists and cannot be counted prints its refusal kind in the
  column, the run continues and the exit status is the run's own (FR-027).
- A name the host carries no metric for prints one line and exits with status
  2 before any measurement starts (FR-028, FR-042). No benchmark work runs,
  so a typo costs no measured time. The validation walks the named list
  before the first repetition, in option order, and reports the first name
  that fails.
- The column value comes from the counters library through the plan and the
  recorder, so the timed loop performs no counter read beyond the events the
  plan opens and `time_source_gate.sh` stays green (FR-029).

## Where the two failure kinds differ

| Situation | Output | Status | Rule |
| --- | --- | --- | --- |
| Name matches no metric of the host | one line naming the metric and its near-miss names | 2, before measurement | FR-028 |
| Metric exists, cannot be counted | refusal kind in the column, run proceeds | the run's own | FR-027 |
| Metric exists, counted, window multiplexed | the `runningRatio` disclosure the metric columns print | the run's own | FR-029 |

## Parity obligations

`README.md` gains the option row in the command-line table, and
`test/cli_shape.sh` checks the table against the option list, so the page and
the scanner move together. The option row is part of the shared page budget
the plan's documentation decision carries, and the quick-reference table gains
the public lookup row under the same obligation (FR-032).

## Checks

`test/source/harness_cli_test.cpp` covers one name, two names in order, a
repeated name, an unknown name with its one line and status 2 asserted before
any measured output appears, a missing value, and a metric that exists and
cannot be counted over the fake provider (FR-025 to FR-029, FR-042). The
column value is compared against the same metric composed by hand over
identical fake counts, which is the check that the option path and the
library path agree.
