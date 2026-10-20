# Contract: The threaded report disclosure and noise warning

`source/harness/report.cpp` prints the policy disclosure for threaded
results and one noise warning when spinning contexts are oversubscribed
(FR-025 to FR-028). The report reads no clock and takes no sample for
either (FR-027, FR-039).

## The disclosure line (FR-025)

```text
result rows of a threaded instance, unchanged fields ...
  contexts=N  ctx0=<MODE>/<PLACEMENT>/<APPLIED>  ctx1=<MODE>/<PLACEMENT>/<APPLIED> ...
```

`<APPLIED>` is `applied` where the installed placement function reported its
slot as landed and `unapplied` where it did not (FR-005), so a pin that never
landed shows under whatever text its own function returned. `<PLACEMENT>`
stands cut to the width of the longest account the default function
produces, the `cpu@group` pair of a landed pin, with `~` marking the cut
(FR-025, feature 022 FR-021).

| Rule | Requirement |
| --- | --- |
| For a result run on more than one context, exactly a result whose `contexts` vector is non-empty, the console report shall print the number of contexts and the policy of each context: its wait mode (`SPIN`, `YIELD`, `BLOCK`) and the placement each context ran under, the record its placement function returned (`NO_PIN`, the `cpu@group` pair of the slot it landed on, or `UNAPPLIED` where the slot never landed). | FR-025, FR-021, feature 022 FR-021 |
| A result run on one context shall keep the bytes it prints today; no disclosure line appears. | FR-025, Assumptions |
| The disclosure data shall ride the result (`BenchmarkResult::contexts`) and the printer shall add nothing to it, the standing report separation. | FR-025, D-08 |

## The noise warning (FR-026, FR-028)

| Rule | Requirement |
| --- | --- |
| When two spinning contexts of one result name the same cpu index, or one result has more spinning contexts than the cpus this process may run on, the report shall print one noise warning line after that result, one line for each such result. | FR-026 |
| A run with one context per cpu, pinned, shall print no warning. | FR-026, FR-028 |
| A run whose contexts block shall print no warning. | FR-026, FR-028 |
| The runnable cpu count shall be one host fact the report reads once: the cpus in the process affinity mask as `sched_getaffinity` reports them, or `std::thread::hardware_concurrency` when that call fails. The disclosure and the warning shall read no clock and add no sampling action to any run. | FR-026, FR-027, D-08 |

## Gate extension

The `report_shape` and `report_shape_fixtures` checks extend to the
disclosure line and the warning in the same commit as the printer, the
way 019's contracts extended them to the label field; every token the
built-in fields print today keeps its bytes. The "Reading the report"
section of `README.md` and the example that owns it show a `/threads:N`
row with its disclosure (FR-032).

## Consumers

`test/source/harness_context_report_test.cpp` covers all four FR-026
cases with policy vectors it builds directly (two pinned spinners on
one cpu, a spinner count above the online count, one spinner pinned per
cpu, all blockers), reading the same `System` fact the warning reads;
`example/docs/threads.cpp` and `example/docs/reading_the_report.cpp`
carry the printed shape CI runs.
