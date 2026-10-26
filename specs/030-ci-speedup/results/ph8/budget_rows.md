# The Budget Exceptions and the Numbers Behind Them (030 FR-031 to FR-034, SC-008)

**Surface**: the time budget loop and the exceptions list at the end of
`test/CMakeLists.txt`, the same pair at the end of
`example/CMakeLists.txt`, and the `TIMEOUT` property the CI lanes report
for each entry.
**Head sha**: 24d981434c6d2284880561a970fd86509cf81215, with this task's
uncommitted edit to the two registration files on top of it.
**Run id**: GitHub Actions run 40239619604 of branch `030-ci-speedup`,
head 49c12ecee775ed6fafa66e18d341c081edd194b9, is the cited run.
40242340926, head e77b713f5e86f6137bf81f004ee891171be48b52, is the
cross-check. Runs 40241027505 and 40242327268 were canceled, so they
supply no timing.
**Command**: the greps under Greps and the log read under Readings, each
run from the repository root, plus the four commands under Checks.
**Numbers**: the table under Rows, the per-lane seconds under Readings,
and the cross-check table under Cross-check.

## What the two files carry

Nine entries sit on an exception row. Eight hold the 120 s master names
for them and `example_run` holds the 300 s master names for it, the same
number the budget loop gives. `test/CMakeLists.txt` carries all nine rows
under its budget loop and `example/CMakeLists.txt` carries an empty list,
because that directory registers no test entry.

The audit reads the rows the same way `test/tier_labels.sh` reads them,
through the marker at the head of each row, so a row and a property are
tied together. Every entry of a configured tree carries a `TIMEOUT`
value: 134 of 134 in `build/prepush` (126 at 300 s, 8 at 120 s) and 139
of 139 in `build/tiers-on`, the tree with the matrix registered (131 at
300 s, 8 at 120 s). Nine entries sit on an exception row.

FR-033 holds in both runs: no entry passed above 300 s in any lane, and
the slowest entry outside the exceptions list is `dbc_matrix_on_o0` at
138.05 s on the `tsan` lane of run 40239619604.

## Greps

The list master names, read from `origin/master` at
`f507b8fe5e18f1aa4749dc5412ef6b072fcae9b1`. It holds eight 120 s values
and the 300 s `example_run`, which is the nine-row set D-7 names.

```text
$ git show origin/master:test/CMakeLists.txt | grep -n 'TIMEOUT'
580:set_tests_properties(harness_calibration_test PROPERTIES TIMEOUT 120)
657:# whole entry. Two slots keep a sibling off that core; CI needs no TIMEOUT,
742:  set_tests_properties(base_cli_matrix PROPERTIES TIMEOUT 120)
1392:  set_tests_properties(report_shape PROPERTIES TIMEOUT 120)
1408:  set_tests_properties(report_shape_guide_sample PROPERTIES TIMEOUT 120)
1425:  set_tests_properties(report_shape_percentiles_sample PROPERTIES TIMEOUT 120)
1440:  set_tests_properties(report_shape_json_reports_sample PROPERTIES TIMEOUT 120)
1456:  set_tests_properties(report_shape_jitter_sample PROPERTIES TIMEOUT 120)
1473:  set_tests_properties(report_shape_hunt_rank_sample PROPERTIES TIMEOUT 120)
1487:  set_tests_properties(example_run PROPERTIES TIMEOUT 300)

$ git show origin/master:example/CMakeLists.txt | grep -cE 'add_test|TIMEOUT'
0
```

The example directory registers no entry at master, and the same read at
head gives the same answer, so the exceptions list beside its budget loop
is empty. The nine rows at head sit at the lines below, each one the line
master wrote.

```text
$ grep -nE 'set_tests_properties\((harness_calibration_test|base_cli_matrix|report_shape|report_shape_guide_sample|report_shape_percentiles_sample|report_shape_json_reports_sample|report_shape_jitter_sample|report_shape_hunt_rank_sample|example_run) PROPERTIES TIMEOUT' test/CMakeLists.txt
658:set_tests_properties(harness_calibration_test PROPERTIES TIMEOUT 120)
820:  set_tests_properties(base_cli_matrix PROPERTIES TIMEOUT 120)
1503:  set_tests_properties(report_shape PROPERTIES TIMEOUT 120)
1519:  set_tests_properties(report_shape_guide_sample PROPERTIES TIMEOUT 120)
1536:  set_tests_properties(report_shape_percentiles_sample PROPERTIES TIMEOUT 120)
1551:  set_tests_properties(report_shape_json_reports_sample PROPERTIES TIMEOUT 120)
1567:  set_tests_properties(report_shape_jitter_sample PROPERTIES TIMEOUT 120)
1584:  set_tests_properties(report_shape_hunt_rank_sample PROPERTIES TIMEOUT 120)
1598:  set_tests_properties(example_run PROPERTIES TIMEOUT 300)
```

The two lines that place `harness_cli_test` are named under the entry
that holds no row.

## Rows

The table is the exceptions list. Each row is one entry that keeps a
value other than 300 s, with the reason it holds that ceiling and the
slowest reading of the five lanes of run 40239619604 at head 49c12ec.
The raw lines behind every number stand under Readings.

| entry | keeps | reason the entry holds its own ceiling | slowest lane | s |
| --- | --- | --- | --- | --- |
| `harness_calibration_test` | 120 s | the run-control calibration on the real clock | `coverage` | 0.85 |
| `base_cli_matrix` | 120 s | the base command line replayed over the built example | `sanitize` | 26.54 |
| `report_shape` | 120 s | the shape fold of the quick start report | `coverage` | 1.09 |
| `report_shape_guide_sample` | 120 s | the shape fold of the guide page run | `tsan` | 9.85 |
| `report_shape_percentiles_sample` | 120 s | the shape fold of the percentiles example | `coverage` | 0.22 |
| `report_shape_json_reports_sample` | 120 s | the shape fold of the json report example | `coverage` | 0.18 |
| `report_shape_jitter_sample` | 120 s | the shape fold of the jitter probe example | `test` | 1.17 |
| `report_shape_hunt_rank_sample` | 120 s | the shape fold of the hunt rank example | `tsan` | 2.21 |
| `example_run` | 300 s | every docs example through example_run.sh | `coverage` | 7.78 |

## The suites the readings come from

Each lane of run 40239619604 ends with one line for the suite and one line
per red. No lane names a timeout, so a duration read there is a duration
the entry needed.

```text
lane test (job 121308544920): 99% tests passed, 1 tests failed out of 139 | Total Test time (real) = 298.82 sec
  red: 81 - harness_jitter_cli_test (Failed) quick

lane test-rocky (job 121308544974): 99% tests passed, 1 tests failed out of 139 | Total Test time (real) = 302.10 sec
  red: 81 - harness_jitter_cli_test (Failed) quick

lane coverage (job 121308544923): 99% tests passed, 1 tests failed out of 134 | Total Test time (real) = 236.80 sec
  red: 76 - harness_jitter_cli_test (Failed) quick

lane sanitize (job 121308545010): 99% tests passed, 1 tests failed out of 134 | Total Test time (real) = 105.59 sec
  red: 76 - harness_jitter_cli_test (Failed) quick

lane tsan (job 121308544961): 99% tests passed, 2 tests failed out of 135 | Total Test time (real) = 386.82 sec
  red: 78 - harness_jitter_cli_test (Failed) quick
  red: 102 - harness_context_barrier_test (Failed) quick
```

## Readings

Each lane is one job of run 40239619604, read with the line below and
filtered to the entries that carry a row, beside `harness_cli_test`. The
suite of that run names one red, `harness_jitter_cli_test`, the base red
PC-10 records, and no entry of that run ends in a timeout, so each
reading below is a passing duration.

```text
$ gh run view --job <job id> --log | sed -E 's/^[^Z]*Z //' \
    | grep -E 'Test +#[0-9]+: (harness_calibration_test|base_cli_matrix|report_shape|report_shape_guide_sample|report_shape_percentiles_sample|report_shape_json_reports_sample|report_shape_jitter_sample|report_shape_hunt_rank_sample|example_run|harness_cli_test) \.'
```

```text
job 121308544920, lane test:
  harness_calibration_test         Passed 0.13
  base_cli_matrix                  Passed 19.98
  report_shape                     Passed 1.04
  report_shape_guide_sample        Passed 8.80
  report_shape_percentiles_sample  Passed 0.11
  report_shape_json_reports_sample Passed 0.09
  report_shape_jitter_sample       Passed 1.17
  report_shape_hunt_rank_sample    Passed 1.99
  example_run                      Passed 3.41
  harness_cli_test                 Passed 2.42

job 121308544974, lane test-rocky:
  harness_calibration_test         Passed 0.13
  base_cli_matrix                  Passed 21.10
  report_shape                     Passed 1.03
  report_shape_guide_sample        Passed 8.54
  report_shape_percentiles_sample  Passed 0.09
  report_shape_json_reports_sample Passed 0.11
  report_shape_jitter_sample       Passed 1.09
  report_shape_hunt_rank_sample    Passed 1.96
  example_run                      Passed 3.74
  harness_cli_test                 Passed 1.82

job 121308544923, lane coverage:
  harness_calibration_test         Passed 0.85
  base_cli_matrix                  Passed 22.74
  report_shape                     Passed 1.09
  report_shape_guide_sample        Passed 9.25
  report_shape_percentiles_sample  Passed 0.22
  report_shape_json_reports_sample Passed 0.18
  report_shape_jitter_sample       Passed 0.70
  report_shape_hunt_rank_sample    Passed 2.00
  example_run                      Passed 7.78
  harness_cli_test                 Passed 26.55

job 121308545010, lane sanitize:
  harness_calibration_test         Passed 0.74
  base_cli_matrix                  Passed 26.54
  report_shape                     Passed 1.04
  report_shape_guide_sample        Passed 9.66
  report_shape_percentiles_sample  Passed 0.19
  report_shape_json_reports_sample Passed 0.16
  report_shape_jitter_sample       Passed 1.06
  report_shape_hunt_rank_sample    Passed 1.80
  example_run                      Passed 6.47
  harness_cli_test                 Passed 6.80

job 121308544961, lane tsan:
  harness_calibration_test         Passed 0.67
  base_cli_matrix                  Passed 22.17
  report_shape                     Passed 1.00
  report_shape_guide_sample        Passed 9.85
  report_shape_percentiles_sample  Passed 0.15
  report_shape_json_reports_sample Passed 0.16
  report_shape_jitter_sample       Passed 1.14
  report_shape_hunt_rank_sample    Passed 2.21
  example_run                      Passed 7.45
  harness_cli_test                 Passed 6.53
```

## Cross-check

Run 40242340926 is the newer run of this branch, head e77b713. Every row
keeps its shape there: the slowest reading of the entry stays far inside
the ceiling it keeps.

| entry | keeps | reason the entry holds its own ceiling | slowest lane | s |
| --- | --- | --- | --- | --- |
| `harness_calibration_test` | 120 s | the run-control calibration on the real clock | `coverage` | 0.73 |
| `base_cli_matrix` | 120 s | the base command line replayed over the built example | `sanitize` | 26.39 |
| `report_shape` | 120 s | the shape fold of the quick start report | `tsan` | 1.47 |
| `report_shape_guide_sample` | 120 s | the shape fold of the guide page run | `test` | 8.98 |
| `report_shape_percentiles_sample` | 120 s | the shape fold of the percentiles example | `sanitize` | 0.18 |
| `report_shape_json_reports_sample` | 120 s | the shape fold of the json report example | `coverage` | 0.15 |
| `report_shape_jitter_sample` | 120 s | the shape fold of the jitter probe example | `test` | 1.10 |
| `report_shape_hunt_rank_sample` | 120 s | the shape fold of the hunt rank example | `test` | 2.72 |
| `example_run` | 300 s | every docs example through example_run.sh | `coverage` | 7.60 |

That run's `sanitize` lane ends with two reds, `harness_jitter_cli_test`
and `harness_calibration_test`, the second at 0.52 s with its own
`walk`, `warm-up`, `fivefold` lines printed. A failed duration is a
partial one, so the `sanitize` column of that lane is left out of the
table and the row for `harness_calibration_test` rests on run
40239619604, where the entry passes in every lane.

## The entry that holds no row

`harness_cli_test` takes 300 s from the budget loop and holds no row.
Four numbers settle it.

- master names no `TIMEOUT` for the entry.
  `git show origin/master:test/CMakeLists.txt` gives
  `660:set_tests_properties(harness_cli_test PROPERTIES PROCESSORS 2)`, and
  the nine lines under Greps are the whole `TIMEOUT` list of that file, so
  the entry ran under the 1500 s CTest default that `test/tier_labels.sh:10`
  names for the suite.
- CI reads it inside 300 s in every lane of both runs: 26.55 s on the
  `coverage` lane of run 40239619604 and 11.57 s on the `coverage` lane of
  run 40242340926 are the slowest readings either run takes for it.
- the lane host sits in the PMU regime the tier row records.
  `counters_pmu_test` passes there, `100% tests passed out of 1` in 18.00 s
  on 2026-10-25 at head, and `test/CMakeLists.txt:1636` records this entry
  at 52 min on a host granted the PMU.
- the solo run of the built binary on that host, `cd build/prepush/test &&
  ./harness_cli_test` at head, was stopped at 1547 s of wall time with the
  entry still running, so it outruns 300 s there and outruns the 1500 s
  default master left in force. Two scratch reports of an earlier local run
  of pid 600720 carry the same shape: the first opens at 12:26:04 and the
  second closes at 13:17:52, 51 min 48 s apart. The
  `bash tools/ci/prepush.sh --full` reading recorded for that host on
  2026-10-25 says the same through `ctest`: step 11 prints `slow tier
  failed with 8`, the suite names `65 - harness_cli_test (Timeout)` and
  `Total Test time (real) = 301.49 sec`.

300 s stays. FR-031 gives 300 s to an entry master leaves unnamed, and only
a ceiling above the 1500 s default would cover that host, which FR-034 and
hard rule 8 keep out of this change. FR-033 speaks of a job, and no job of
either run comes near the budget. The cost of the entry on a PMU host is
the slow tier's own reading, carried by the row at
`test/CMakeLists.txt:1636`.

## Checks

The two `CPack.cmake has already been included` warnings inside the
configure output come from `external/zlib/CMakeLists.txt:42` and
`external/hdrhistogram_c/CMakeLists.txt:104`. `external/**` is fetched
content: `git show origin/master:external/zlib/CMakeLists.txt` answers
`exists on disk, but not in origin/master`, so neither path is tracked and
this change leaves both untouched.

```text
$ cmake -B build/prepush
-- QUILL_BUILD_MODULE: OFF
-- Configuring done (0.6s)
-- Generating done (0.4s)
-- Build files have been written to: /home/archerc/code/speedgun-ng/build/prepush
exit=0

$ bash test/tier_labels.sh build/prepush ctest
read: 134 entries, 132 quick, 2 slow
ok: every entry carries one label and a budget, and neither registration breaks FR-048
exit=0

$ ctest --test-dir build/prepush -R test_tier_labels --output-on-failure
Test project /home/archerc/code/speedgun-ng/build/prepush
    Start 23: test_tier_labels
1/1 Test #23: test_tier_labels .................   Passed    0.23 sec

100% tests passed out of 1

Label Time Summary:
quick    =   0.23 sec*proc (1 test)

Total Test time (real) =   0.23 sec
exit=0

$ python3 tools/prose/prose_gate.py --check prose --mode tree --paths test/CMakeLists.txt example/CMakeLists.txt
prose-lint: 2 sources, 728 units examined, 0 findings, 0 skipped
exit=0
```

The record itself clears the same gate. It is untracked at head, so the
read runs in a throwaway index under `/tmp`, where `git ls-files` finds it
and tree mode has one source to examine.

```text
$ python3 tools/prose/prose_gate.py --check prose --mode tree --paths budget_rows.md
prose-lint: 1 sources, 261 units examined, 0 findings, 0 skipped
exit 0
```

VERDICT: PASS

