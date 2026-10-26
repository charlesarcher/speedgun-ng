# The Slowest Entry of Each Lane Beside the Budget It Carries (030 SC-008, T046)

**Surface**: the ctest per-entry duration lines of the five CI lanes
named `test`, `test-rocky`, `coverage`, `sanitize` and `tsan`, read from
the job logs of the last completed run of branch `030-ci-speedup`,
beside the time budget each of those entries carries in
`test/CMakeLists.txt`.
**Head sha**: 7a7dc1f26612f20b8bcb4dd7fcc16fb9498d278b. The working copy
of `test/CMakeLists.txt` carries an uncommitted edit on top of that
head, so the line numbers named below are the working copy's. The same
ten `TIMEOUT` rows, count and text, stand at `e77b713` and at `49c12ec`,
the two heads cited here, with the budget loop one line earlier at 1661
in both.
**Run id**: GitHub Actions run 40242340926 of branch `030-ci-speedup`,
head `e77b713f5e86f6137bf81f004ee891171be48b52`, is the newest run of
the branch and every one of its five lane jobs has completed, which
makes it the last completed run and the cited run. Its jobs are `test`
121316717757, `test-rocky` 121316717877, `coverage` 121316717683,
`sanitize` 121316717684 and `tsan` 121316717698. Run 40239619604 at head
`49c12ecee775ed6fafa66e18d341c081edd194b9` is the cross-check, with jobs
121308544920, 121308544974, 121308544923, 121308545010 and 121308544961
for the same five lanes. Runs 40241027505 and 40242327268 were canceled,
so they supply no timing.
**Command**: the log read under Readings and the four greps under
Budgets, each run from the repository root.
**Numbers**: the five tables under Lanes, the tightest row of each lane
under Each lane's tightest row, and the twenty rows under Cross-check.

## What the audit asks

Each lane table holds the four slowest passing entries of that lane and,
beside each one, the ceiling its own row sets and the headroom left
between the two. The budgets are read the way `test/tier_labels.sh`
reads them, from the `TIMEOUT` property the registration gives the
entry.

## Budgets

The budget loop gives 300 s to every entry whose registration names no
`TIMEOUT`, and an entry that names its own value keeps it. Only
`base_cli_matrix` of the entries below names its own value.

```text
$ grep -nE 'PROPERTIES TIMEOUT' test/CMakeLists.txt example/CMakeLists.txt
test/CMakeLists.txt:658:set_tests_properties(harness_calibration_test PROPERTIES TIMEOUT 120)
test/CMakeLists.txt:820:  set_tests_properties(base_cli_matrix PROPERTIES TIMEOUT 120)
test/CMakeLists.txt:1503:  set_tests_properties(report_shape PROPERTIES TIMEOUT 120)
test/CMakeLists.txt:1519:  set_tests_properties(report_shape_guide_sample PROPERTIES TIMEOUT 120)
test/CMakeLists.txt:1536:  set_tests_properties(report_shape_percentiles_sample PROPERTIES TIMEOUT 120)
test/CMakeLists.txt:1551:  set_tests_properties(report_shape_json_reports_sample PROPERTIES TIMEOUT 120)
test/CMakeLists.txt:1567:  set_tests_properties(report_shape_jitter_sample PROPERTIES TIMEOUT 120)
test/CMakeLists.txt:1584:  set_tests_properties(report_shape_hunt_rank_sample PROPERTIES TIMEOUT 120)
test/CMakeLists.txt:1598:  set_tests_properties(example_run PROPERTIES TIMEOUT 300)
test/CMakeLists.txt:1662:    set_tests_properties(${speedgun_test} PROPERTIES TIMEOUT 300)
example/CMakeLists.txt:88:    set_tests_properties(${speedgun_example} PROPERTIES TIMEOUT 300)
```

The exception list beside that loop carries one row per entry that keeps
a value other than 300 s, and the row that governs `base_cli_matrix` is
the one below. The other eight rows of the list name entries that never
reach the four slowest of any lane, which is what `budget_rows.md`
records.

```text
$ grep -n 'budget exception base_cli_matrix' test/CMakeLists.txt
1683:# budget exception base_cli_matrix keeps the 120 s master sets for the
```

The example directory gives 300 s at `example/CMakeLists.txt:88` and
holds the empty exception list beside it, so no entry below takes a
budget from that directory.

No lane overrides the property from the command line, so the `TIMEOUT`
value above is the ceiling the entry ran under.

```text
$ git show e77b713:.github/workflows/ci.yml | grep -cE 'ctest .*--timeout'
0
```

## The suites the readings come from

Each lane of the cited run ends with one line for the suite and one line
per red or skipped entry. No lane names a timeout, so a duration read
there is a duration the entry needed.

```text
$ gh run view --job <job id> --log | sed -E 's/^[^Z]*Z //' \
    | grep -E 'tests passed.*out of|Total Test time|^[[:space:]]+[0-9]+ - '
```

```text
lane test, job 121316717757: 99% tests passed, 1 tests failed out of 139 |
    Total Test time (real) = 223.13 sec
  47 - counters_pmu_test (Skipped)
  53 - counters_overhead (Skipped)
  81 - harness_jitter_cli_test (Failed) quick

lane test-rocky, job 121316717877: 99% tests passed, 1 tests failed out of
    139 | Total Test time (real) = 300.75 sec
  47 - counters_pmu_test (Skipped)
  53 - counters_overhead (Skipped)
  81 - harness_jitter_cli_test (Failed) quick

lane coverage, job 121316717683: 99% tests passed, 1 tests failed out of
    134 | Total Test time (real) = 228.41 sec
  42 - counters_pmu_test (Skipped)
  48 - counters_overhead (Skipped)
  76 - harness_jitter_cli_test (Failed) quick

lane sanitize, job 121316717684: 99% tests passed, 2 tests failed out of
    134 | Total Test time (real) = 101.77 sec
  42 - counters_pmu_test (Skipped)
  48 - counters_overhead (Skipped)
  57 - harness_calibration_test (Failed) quick
  76 - harness_jitter_cli_test (Failed) quick

lane tsan, job 121316717698: 99% tests passed, 1 tests failed out of 135 |
    Total Test time (real) = 226.50 sec
  46 - counters_pmu_test (Skipped)
  52 - counters_overhead (Skipped)
  78 - harness_jitter_cli_test (Failed) quick
```

The red of `harness_jitter_cli_test` is the base red PC-10 records, in
every lane of both runs. The second red of the `sanitize` lane,
`harness_calibration_test`, is new to this run: that entry passed in
every lane of run 40239619604.

## Readings

Each lane is one job, read with the line below and sorted by the seconds
field. The first four `Passed` lines of each block are the rows of the
matching table.

```text
$ gh run view --job <job id> --log | sed -E 's/^[^Z]*Z //' \
    | grep -E 'Test +#[0-9]+: ' \
    | awk '{ st=$(NF-2); gsub(/^\.+/,"",st);
            if ($(NF-1) ~ /^[0-9.]+$/) printf "%8.2f  %-30s %s\n", $(NF-1), $4, st }' \
    | sort -rn | head -5
```

```text
job 121316717757, lane test:
   85.81  dbc_matrix_off_o2              Passed
   82.25  dbc_matrix_off_o0              Passed
   76.61  dbc_matrix_on_o0               Passed
   71.25  dbc_matrix_on_o2               Passed
   32.20  snippet_compile                Passed

job 121316717877, lane test-rocky:
  118.44  dbc_matrix_off_o2              Passed
  112.11  dbc_matrix_on_o0               Passed
  105.76  dbc_matrix_off_o0              Passed
  102.40  dbc_matrix_on_o2               Passed
   54.17  snippet_compile                Passed

job 121316717683, lane coverage:
   37.29  snippet_compile                Passed
   19.82  base_cli_matrix                Passed
   11.57  harness_cli_test               Passed
   11.40  harness_jitter_cli_test        ***Failed
   10.36  counters_clock_raw_test        Passed

job 121316717684, lane sanitize:
   59.45  snippet_compile                Passed
   26.39  base_cli_matrix                Passed
   18.57  counters_compile_fail          Passed
   18.23  dbc_compile_fail               Passed
   10.92  counters_tsc_read_shape        Passed

job 121316717698, lane tsan:
   79.22  dbc_matrix_off_o2              Passed
   75.08  dbc_matrix_on_o0               Passed
   70.56  dbc_matrix_off_o0              Passed
   70.35  dbc_matrix_on_o2               Passed
   36.89  snippet_compile                Passed
```

## Lanes

Budget is the `TIMEOUT` value the entry carries at head, seconds is the
reading of the cited run, and headroom is the difference in seconds.

### Lane test, job 121316717757

| entry | seconds | budget | headroom | row that sets the budget |
| --- | --- | --- | --- | --- |
| `dbc_matrix_off_o2` | 85.81 | 300 s | 214.19 | loop, `test/CMakeLists.txt:1662` |
| `dbc_matrix_off_o0` | 82.25 | 300 s | 217.75 | loop, `test/CMakeLists.txt:1662` |
| `dbc_matrix_on_o0` | 76.61 | 300 s | 223.39 | loop, `test/CMakeLists.txt:1662` |
| `dbc_matrix_on_o2` | 71.25 | 300 s | 228.75 | loop, `test/CMakeLists.txt:1662` |

### Lane test-rocky, job 121316717877

| entry | seconds | budget | headroom | row that sets the budget |
| --- | --- | --- | --- | --- |
| `dbc_matrix_off_o2` | 118.44 | 300 s | 181.56 | loop, `test/CMakeLists.txt:1662` |
| `dbc_matrix_on_o0` | 112.11 | 300 s | 187.89 | loop, `test/CMakeLists.txt:1662` |
| `dbc_matrix_off_o0` | 105.76 | 300 s | 194.24 | loop, `test/CMakeLists.txt:1662` |
| `dbc_matrix_on_o2` | 102.40 | 300 s | 197.60 | loop, `test/CMakeLists.txt:1662` |

### Lane coverage, job 121316717683

| entry | seconds | budget | headroom | row that sets the budget |
| --- | --- | --- | --- | --- |
| `snippet_compile` | 37.29 | 300 s | 262.71 | loop, `test/CMakeLists.txt:1662` |
| `base_cli_matrix` | 19.82 | 120 s | 100.18 | exception, `test/CMakeLists.txt:820` |
| `harness_cli_test` | 11.57 | 300 s | 288.43 | loop, `test/CMakeLists.txt:1662` |
| `counters_clock_raw_test` | 10.36 | 300 s | 289.64 | loop, `test/CMakeLists.txt:1662` |

The `***Failed` line of `harness_jitter_cli_test` at 11.40 s sits
between the third and the fourth row of this table. A failed duration is
a partial one, so the entry keeps its place in neither the table nor the
ranking.

### Lane sanitize, job 121316717684

| entry | seconds | budget | headroom | row that sets the budget |
| --- | --- | --- | --- | --- |
| `snippet_compile` | 59.45 | 300 s | 240.55 | loop, `test/CMakeLists.txt:1662` |
| `base_cli_matrix` | 26.39 | 120 s | 93.61 | exception, `test/CMakeLists.txt:820` |
| `counters_compile_fail` | 18.57 | 300 s | 281.43 | loop, `test/CMakeLists.txt:1662` |
| `dbc_compile_fail` | 18.23 | 300 s | 281.77 | loop, `test/CMakeLists.txt:1662` |

`harness_calibration_test` is the second red of this lane, at 0.52 s, a
partial reading far inside the 120 s it keeps, so it holds no row here.

### Lane tsan, job 121316717698

| entry | seconds | budget | headroom | row that sets the budget |
| --- | --- | --- | --- | --- |
| `dbc_matrix_off_o2` | 79.22 | 300 s | 220.78 | loop, `test/CMakeLists.txt:1662` |
| `dbc_matrix_on_o0` | 75.08 | 300 s | 224.92 | loop, `test/CMakeLists.txt:1662` |
| `dbc_matrix_off_o0` | 70.56 | 300 s | 229.44 | loop, `test/CMakeLists.txt:1662` |
| `dbc_matrix_on_o2` | 70.35 | 300 s | 229.65 | loop, `test/CMakeLists.txt:1662` |

## Each lane's tightest row

- `test`: `dbc_matrix_off_o2`, 214.19 s of headroom under 300 s.
- `test-rocky`: `dbc_matrix_off_o2`, 181.56 s of headroom under 300 s.
- `coverage`: `base_cli_matrix`, 100.18 s of headroom under the 120 s
  its exception row keeps.
- `sanitize`: `base_cli_matrix`, 93.61 s of headroom, the tightest of
  the five lanes and the whole record.
- `tsan`: `dbc_matrix_off_o2`, 220.78 s of headroom under 300 s.

Every reading of every table sits inside the ceiling its row sets, so no
entry reaches its budget on any lane of the cited run. The 120 s
exception of `base_cli_matrix` is the tightest pairing anywhere here, at
22.0 percent of its ceiling on the `sanitize` lane.

## Cross-check

Run 40239619604 at head `49c12ec`, read with the same command, holds the
same shape at a slower speed. Its `tsan` lane carries the slowest
reading either run takes for any entry.

| lane | entry | seconds | budget | headroom |
| --- | --- | --- | --- | --- |
| test | `dbc_matrix_off_o2` | 120.96 | 300 s | 179.04 |
| test | `dbc_matrix_off_o0` | 105.33 | 300 s | 194.67 |
| test | `dbc_matrix_on_o0` | 99.87 | 300 s | 200.13 |
| test | `dbc_matrix_on_o2` | 95.09 | 300 s | 204.91 |
| test-rocky | `dbc_matrix_off_o2` | 120.40 | 300 s | 179.60 |
| test-rocky | `dbc_matrix_on_o0` | 114.79 | 300 s | 185.21 |
| test-rocky | `dbc_matrix_on_o2` | 102.90 | 300 s | 197.10 |
| test-rocky | `dbc_matrix_off_o0` | 102.66 | 300 s | 197.34 |
| coverage | `harness_cli_test` | 26.55 | 300 s | 273.45 |
| coverage | `base_cli_matrix` | 22.74 | 120 s | 97.26 |
| coverage | `snippet_compile` | 20.07 | 300 s | 279.93 |
| coverage | `counters_clock_raw_test` | 16.08 | 300 s | 283.92 |
| sanitize | `snippet_compile` | 61.49 | 300 s | 238.51 |
| sanitize | `base_cli_matrix` | 26.54 | 120 s | 93.46 |
| sanitize | `counters_compile_fail` | 19.97 | 300 s | 280.03 |
| sanitize | `dbc_compile_fail` | 17.62 | 300 s | 282.38 |
| tsan | `dbc_matrix_on_o0` | 138.05 | 300 s | 161.95 |
| tsan | `dbc_matrix_off_o2` | 137.45 | 300 s | 162.55 |
| tsan | `dbc_matrix_off_o0` | 127.14 | 300 s | 172.86 |
| tsan | `dbc_matrix_on_o2` | 119.69 | 300 s | 180.31 |

That run's `coverage` lane holds the four slowest passing entries named
above, with `harness_jitter_cli_test` at 18.20 s left out as the partial
reading of a red. Its `tsan` lane adds a second red,
`harness_context_barrier_test` at 1.07 s, likewise outside the table.

Taking the worst reading of each entry across both runs, the tightest
pairing stays `base_cli_matrix` on the `sanitize` lane at 26.54 s
against 120 s, a headroom of 93.46 s, and the tightest 300 s pairing
stays `dbc_matrix_on_o0` on the `tsan` lane at 138.05 s, a headroom of
161.95 s.

## Checks

The record is untracked at head, so the read below runs in a throwaway
index under `/tmp`, where `git ls-files` finds the one source.

```text
$ python3 tools/prose/prose_gate.py --check prose --mode tree --paths audit.md
prose-lint: 1 sources, 251 units examined, 0 findings, 0 skipped
exit=0
```

VERDICT: PASS
