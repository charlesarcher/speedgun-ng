# Contract: The printed columns

`printResult` (`source/harness/report.cpp:47`) formats the value the
runner produced and adds nothing to it (H1 FR-036). H3 changes the time
part of the line and the sample text, and it keeps the metric part as H1
left it.

## The repetition row (FR-001, FR-013, FR-015)

```text
before, report.cpp:93-94
  <name>  iterations=<N>  time/iter (ns)=<v>  overhead floor (ns)=<v>  endpoint samples=2

after
  <name>  iterations=<N>  real time/iter (<U>)=<v>  cpu time/iter (<U>)=<v>
          overhead floor (<U>)=<v>  endpoint samples=<S>  <metric columns>
```

| Rule | Requirement |
| --- | --- |
| Every measured row shall print real time per iteration and CPU time per iteration. | FR-001 |
| The CPU column shall carry the fold of `machine/thread_cpu` by default and of `machine/process_cpu` after `measureProcessCpuTime`. | FR-001, FR-004 |
| The fixed text `endpoint samples=2` shall become the count of endpoint samples that run took. | FR-013 |
| Each printed time value of the benchmark shall carry the value scaled to its unit and the unit label. | FR-015 |
| The overhead floor shall scale with the same unit, so one line carries one scale. | FR-015, plan R-08 |
| The metric columns, their `ratio`, `scaled`, `gap`, and `refusal` fields, and their order shall stay as H1 printed them. | FR-018 |
| A `FAILED` or `SKIPPED` result shall keep the H1 single line with its reason. | FR-018 |

## The aggregate row (FR-002)

```text
before, report.cpp:63
  <name>  AGGREGATE  time/iter (ns) mean=... median=... sd=... cv=... min=... max=... n=...

after
  <name>  AGGREGATE  primary time/iter (<U>) mean=... median=... sd=... cv=... min=... max=... n=...
```

| Rule | Requirement |
| --- | --- |
| The aggregate shall summarize the primary time of the benchmark: the CPU selection by default, real time after `useRealTime`, the manual sum after `useManualTime`. | FR-002 |
| The word `primary` shall name the summarized column, since the default changes from the real fold to the CPU selection. | FR-002, research D-13 |
| The statistics shall keep the H1 fields and the H1 rule of one aggregate row per benchmark with more than one repetition. | FR-018 |
| A repetition whose primary fold carried a gap disclosure shall stay out of the aggregate statistics. | spec Edge Cases, research D-13 |
| The metric statistics lines shall stay as H1 printed them. | FR-018 |

## Scaling (FR-015, FR-016)

| Unit | Label | Divisor |
| --- | --- | --- |
| `TimeUnit::NANOSECONDS` | `ns` | 1 |
| `TimeUnit::MICROSECONDS` | `us` | 1000 |
| `TimeUnit::MILLISECONDS` | `ms` | 1000000 |
| `TimeUnit::SECONDS` | `s` | 1000000000 |

| Rule | Requirement |
| --- | --- |
| The stored `ResultRow` values shall stay nanoseconds, so the C++ value keeps one unit. | FR-018, research D-12 |
| The report shall apply the divisor of `BenchmarkResult::timeUnit` at print time. | FR-015 |
| The labels shall match the cited revision's console reporter (`src/console_reporter.cc:157-173`). | reference read |
| The scaling shall reach the two time columns, the overhead floor, and the aggregate line. | FR-015 |

## Consumers of the row text

| Consumer | Change |
| --- | --- |
| `test/source/harness_calibration_test.cpp:296, :375, :404` | the field token moves to `real time/iter (ns)=` |
| `test/source/harness_callback_test.cpp:464` | the field token moves to `real time/iter (ns)=` |
| `test/source/harness_capture_test.cpp:330, :372` | the field token moves to `real time/iter (ns)=` |
| `test/source/harness_fixture_test.cpp:488` | the field token moves to `real time/iter (ns)=` |
| `test/source/harness_statistics_test.cpp:199` | the field token moves to `real time/iter (ns)=` |
| `test/source/harness_cli_test.cpp:665`, `harness_family_test.cpp:434`, `harness_instance_name_test.cpp:312` | unchanged: each asserts that list mode prints no `time/iter` text |

The old token `time/iter (ns)=` is a suffix of both new column tokens, so
each reader names the column it means. The move lands in the same commit
as the `report.cpp` change, which keeps every test green at each commit of
the chain.
