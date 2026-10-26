# The Final Exceptions Table and the Two Timeout Knobs Left Alone (030 FR-032 to FR-034, SC-008, T047)

**Surface**: the exceptions list at the end of `test/CMakeLists.txt` and
the empty list beside the budget loop of `example/CMakeLists.txt`,
beside the ctest invocations of `.github/workflows/ci.yml` and
`tools/ci/prepush.sh` and the `timeout-minutes` key of the jobs of
`.github/workflows/ci.yml`.
**Head sha**: ffcd00d07cfa99dff2c2b5b8043bbf750cce1a2c. The working copy
carries uncommitted edits to `.github/workflows/ci.yml`,
`test/CMakeLists.txt`, `example/CMakeLists.txt` and
`test/tier_labels.sh` on top of it, so the line numbers below are the
working copy's. The same
nine `TIMEOUT` rows and the same nine exception rows stand at
`e77b713` and at `49c12ec`, the two heads cited here.
**Run id**: GitHub Actions run 40239619604 of branch `030-ci-speedup`,
head `49c12ecee775ed6fafa66e18d341c081edd194b9`, is the cited run: its
five lane jobs are the readings the exception rows in the code carry.
GitHub Actions run 40242340926, head
`e77b713f5e86f6137bf81f004ee891171be48b52`, is the cross-check. Runs
40241027505 and 40242327268 were canceled, so they supply no timing.
**Command**: the greps under The rows in the tree, Negative one and
Negative two, each run from the repository root, plus the max merge
under How the numbers were re-checked.
**Numbers**: the nine rows under The exceptions table, the job ids under
The two runs and the jobs behind them, the row lines under The rows in
the tree, and the greps under the two negative sections.

## The exceptions table

Nine entries keep a value other than the 300 s the budget loop gives.
Eight hold the 120 s master sets and `example_run` holds the 300 s
master sets. Each number below is a passing duration: the cited run
names one red, `harness_jitter_cli_test`, the base red PC-10 records,
and no entry of either run ends in a timeout.

| entry | keeps | slowest lane, run 40239619604 | s | slowest lane, run 40242340926 | s | worst of both | s of headroom left |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `harness_calibration_test` | 120 s | `coverage` | 0.85 | `coverage` | 0.73 | `coverage` | 119.15 |
| `base_cli_matrix` | 120 s | `sanitize` | 26.54 | `sanitize` | 26.39 | `sanitize` | 93.46 |
| `report_shape` | 120 s | `coverage` | 1.09 | `tsan` | 1.47 | `tsan` | 118.53 |
| `report_shape_guide_sample` | 120 s | `tsan` | 9.85 | `test` | 8.98 | `tsan` | 110.15 |
| `report_shape_percentiles_sample` | 120 s | `coverage` | 0.22 | `sanitize` | 0.18 | `coverage` | 119.78 |
| `report_shape_json_reports_sample` | 120 s | `coverage` | 0.18 | `coverage` | 0.15 | `coverage` | 119.82 |
| `report_shape_jitter_sample` | 120 s | `test` | 1.17 | `test` | 1.10 | `test` | 118.83 |
| `report_shape_hunt_rank_sample` | 120 s | `tsan` | 2.21 | `test` | 2.72 | `test` | 117.28 |
| `example_run` | 300 s | `coverage` | 7.78 | `coverage` | 7.60 | `coverage` | 292.22 |

Each seconds column is the slowest of the five lanes of that run for
that entry. The headroom column is the ceiling minus the worst reading
of the two runs. Every entry sits inside the ceiling it keeps, the
tightest by a wide margin being `base_cli_matrix`, which reaches 26.54 s
of the 120 s it keeps on the `sanitize` lane, a headroom of 93.46 s.

The entry outside the list worth naming is `harness_cli_test`, which
takes 300 s from the loop and holds no row. `budget_rows.md` settles it:
the entry needs 26.55 s on the `coverage` lane of run 40239619604 and
11.57 s on the same lane of run 40242340926, and its cost on a host
granted the PMU is the slow tier's own reading, carried at
`test/CMakeLists.txt:1636`. A ceiling above the 1500 s CTest default
master left in force would be the only one to cover that host, which
FR-034 keeps out of this change.

## The two runs and the jobs behind them

| run | head | `test` | `test-rocky` | `coverage` | `sanitize` | `tsan` |
| --- | --- | --- | --- | --- | --- | --- |
| 40239619604 | `49c12ec` | 121308544920 | 121308544974 | 121308544923 | 121308545010 | 121308544961 |
| 40242340926 | `e77b713` | 121316717757 | 121316717877 | 121316717683 | 121316717684 | 121316717698 |

The readings come from the job logs, read with the line below and
filtered to the nine entries. The same line produced both seconds
columns, one read per job id above.

```text
$ gh run view --job <job id> --log | sed -E 's/^[^Z]*Z //' \
    | grep -E 'Test +#[0-9]+: (harness_calibration_test|base_cli_matrix|report_shape|report_shape_guide_sample|report_shape_percentiles_sample|report_shape_json_reports_sample|report_shape_jitter_sample|report_shape_hunt_rank_sample|example_run) \.'
```

## The rows in the tree

The nine rows of the list sit at the lines below, one row per entry, in
the order the table above holds them. The numbers in the list are the
run 40239619604 column of the table.

```text
$ grep -n 'budget exception' test/CMakeLists.txt
1666:# The time budget exceptions (030 FR-032, D-7, SC-008): the entries that
1680:# budget exception harness_calibration_test keeps the 120 s master
1683:# budget exception base_cli_matrix keeps the 120 s master sets for the
1686:# budget exception report_shape keeps the 120 s master sets for the
1689:# budget exception report_shape_guide_sample keeps the 120 s master
1692:# budget exception report_shape_percentiles_sample keeps the 120 s
1695:# budget exception report_shape_json_reports_sample keeps the 120 s
1698:# budget exception report_shape_jitter_sample keeps the 120 s master
1701:# budget exception report_shape_hunt_rank_sample keeps the 120 s
1704:# budget exception example_run keeps the 300 s master sets for the
```

The example directory registers no test entry, so the list beside its
budget loop at `example/CMakeLists.txt:88` is empty. The header at
`example/CMakeLists.txt:92` carries the same heading, and no row follows
it.

```text
$ grep -c 'budget exception [a-z]' example/CMakeLists.txt
0
```

## Negative one: no ctest line carries a timeout flag

No ctest invocation of either file passes the ctest timeout flag, so the
`TIMEOUT` property of the registration is the ceiling every entry ran
under in CI and in the pre-push run.

```text
$ grep -nE 'ctest.*--timeout' .github/workflows/ci.yml tools/ci/prepush.sh
<no output>
exit=1

$ grep -n -- '--timeout' .github/workflows/ci.yml tools/ci/prepush.sh
<no output>
exit=1
```

Both searches are empty. The second one matters for the one ctest
command that spans lines: the invocation at
`.github/workflows/ci.yml:1187` continues at line 1188 with
`--output-on-failure`, and a whole-file search finds the flag nowhere in
either file, so no wrapped continuation hides one. Every ctest line the
two files do carry is listed below.

```text
$ grep -n 'ctest' .github/workflows/ci.yml tools/ci/prepush.sh
.github/workflows/ci.yml:114:      run: ctest -j 1 --output-on-failure --no-tests=error
.github/workflows/ci.yml:217:      run: ctest --output-on-failure --no-tests=error -j "$(nproc)"
.github/workflows/ci.yml:296:      run: ctest --output-on-failure --no-tests=error -j "$(nproc)"
.github/workflows/ci.yml:559:      run: ctest --output-on-failure --no-tests=error -C Release -j "$(nproc)"
.github/workflows/ci.yml:632:      run: ctest --output-on-failure --no-tests=error -C Release -j "$(nproc)"
.github/workflows/ci.yml:1187:        ctest --test-dir build/consumer-release-dev -R counters_trap \
.github/workflows/ci.yml:1280:      run: ctest -R dbc_gate_fixtures --output-on-failure --no-tests=error -j "$(nproc)"
.github/workflows/ci.yml:1386:      - run: ctest --test-dir build/dev -R prose_gate_fixtures
tools/ci/prepush.sh:91:  for tool in python3 clang-format cmake ctest taskset git clang-tidy nproc; do
tools/ci/prepush.sh:195:step 'quick tier, full affinity' ctest --preset prepush-quick -j "$(nproc)"
tools/ci/prepush.sh:196:step 'quick tier, two cpus' taskset -c 0,1 ctest --preset prepush-quick -j 2
tools/ci/prepush.sh:256:  step 'slow tier' ctest --preset prepush-full -j "$(nproc)"
```

Twelve lines: eight ctest invocations in the workflow, one tool-presence
check and three ctest invocations in the pre-push script. The workflow
also wraps eleven `timeout 30` shell commands around the trap fixtures
of the `consumer-release` job, at `.github/workflows/ci.yml:1086` and
lines 1110 to 1129. Those are the coreutils wrapper on a direct binary
run, and no ctest command sits under any of them.

## Negative two: no job carries a raised timeout

No job of the workflow sets `timeout-minutes`, and neither does the
workflow itself, so all thirteen jobs run under the default the runner
gives, 360 minutes. Master carries no such key either, so this change
adds none and removes none.

```text
$ grep -n 'timeout-minutes' .github/workflows/ci.yml
<no output>
exit=1

$ git show origin/master:.github/workflows/ci.yml | grep -c 'timeout-minutes'
0
exit=1

$ git rev-parse origin/master
f507b8fe5e18f1aa4749dc5412ef6b072fcae9b1
```

The jobs, with the value each one carries. The read below walks the
`jobs` block and prints the two-space keys of the block beside the
`timeout-minutes` lines found inside each job, so an empty value column
is the file holding the key out of that job.

```text
$ awk '/^jobs:$/ { injobs = 1; next }
       injobs && /^  [a-z][a-z0-9_-]*:$/ { print NR": "substr($0, 3) }' \
    .github/workflows/ci.yml
13: lint:
44: coverage:
143: sanitize:
226: tsan:
309: static-analysis:
452: test:
567: test-rocky:
640: shared-audit:
783: downstream-consumer:
980: consumer-release:
1216: dbc-gate:
1295: prose-lint:
1395: docs:
```

Thirteen jobs, and the count of the key across the whole file is 0, so
each of the thirteen carries `timeout-minutes: none`. The lanes that
carry the numbers of the table above are the five named `test`,
`test-rocky`, `coverage`, `sanitize` and `tsan`; the slowest of them
across both runs is the `tsan` lane of run 40239619604 at 386.82 s of
suite time, far inside the default.

## How the numbers were re-checked

The two seconds columns came from `results/ph8/budget_rows.md`, which
holds the per-lane readings as printed from the job logs. The worst
reading of each entry was re-derived here from the five per-lane lines
of that record, one line per lane, and the max matched the row it
belongs to in all nine cases. `base_cli_matrix` was further checked
against `results/ph8/audit.md`, whose lane tables of run 40242340926
carry 19.82 s on `coverage` and 26.39 s on `sanitize`, and whose
cross-check table of run 40239619604 carries 22.74 s on `coverage` and
26.54 s on `sanitize`. The headroom of 93.46 s for that entry matches
the same pairing named in `audit.md`.

## Checks

The record is untracked at head, so the read below runs in a throwaway
index under `/tmp`, where `git ls-files` finds the one source and tree
mode has something to examine.

```text
$ python3 tools/prose/prose_gate.py --check prose --mode tree --paths exceptions.md
prose-lint: 1 sources, 198 units examined, 0 findings, 0 skipped
exit=0
```

The two negatives hold: the ctest timeout flag appears in neither CI
file, and no job of the workflow raises its `timeout-minutes`. Every
exception entry of both runs sits inside the ceiling its row keeps.

VERDICT: PASS
