# Budget Check Closure (030 FR-031, FR-033, SC-008)

**Surface**: the budget check inside `test/tier_labels.sh`, the
exceptions block at the end of `test/CMakeLists.txt` that T044 landed,
the empty list beside the budget loop of `example/CMakeLists.txt`, and
the `TIMEOUT` property the ctest listing reports per entry.
**Head sha**: 7a7dc1f26612f20b8bcb4dd7fcc16fb9498d278b, with this
task's uncommitted edit to `test/tier_labels.sh` on top of it.
**Run id**: GitHub Actions run 40239619604 of branch `030-ci-speedup`,
head 49c12ecee775ed6fafa66e18d341c081edd194b9, is the cited run. Its
suite carries one red, `harness_jitter_cli_test`, the base red PC-10
records, so per-entry seconds stay valid. Run 40242340926, head
e77b713f5e86f6137bf81f004ee891171be48b52, completed with all five test
lanes closed while this task read, and its seconds table already lives
in results/ph8/budget_rows.md. Runs 40241027505 and 40242327268 were
canceled, so they supply no timing.
**Command**: the runs under Verification and The planted gap, the
census under What the check confirms, and the log read under CI
Readings, each run from the repository root.
**Numbers**: the read lines, fault lines and exit statuses under
Verification and The planted gap, the timeout census under What the
check confirms, and the per-lane seconds under CI Readings.

## What this task closed

FR-031 (`specs/030-ci-speedup/spec.md:740`) gives 300 s to every entry
that holds no `TIMEOUT` and keeps every lower value master sets.
FR-033 (`specs/030-ci-speedup/spec.md:747`) asks that no entry exceed
its budget in any job. T044 landed the nine exception rows that hold
the master values, eight entries at 120 s and `example_run` at 300 s.
This task closed the audit that reads those rows.

The audit already failed an entry that reported neither a `TIMEOUT`
property nor an exception row, the no-budget fault. This task added the
wrong-budget fault: an entry whose row lists a number of seconds must
carry that number as its `TIMEOUT` in the listing. Before the change an
exception row exempted its entry outright, so an entry that kept its
row while its property stood hidden still passed the audit. The second
plant under The planted gap shows that shape at head.

## What the check confirms

The rows open at `test/CMakeLists.txt:1680`, the line that reads
"# budget exception harness_calibration_test keeps the 120 s master",
and close at `test/CMakeLists.txt:1704`, the line that reads
"# budget exception example_run keeps the 300 s master". Each row
states the seconds the entry keeps in the words the audit's
`EXCEPTION_ROW` pattern reads.

The json listing reports a `TIMEOUT` as a number, so the audit
compares both sides as numbers. A census of the two configured trees,
each read by the same command, shows the nine values the rows list:

```
$ cat > /tmp/t045/census.py <<'EOF'
import json
import sys

def prop(e, w):
    for x in e.get("properties", []):
        if x.get("name") == w:
            return x.get("value")

suite = json.load(sys.stdin)["tests"]
from collections import Counter
c = Counter(prop(e, "TIMEOUT") for e in suite)
print("entries", len(suite), dict(c))
print(sorted(e["name"] for e in suite if prop(e, "TIMEOUT") == 120.0))
print("example_run", prop(next(e for e in suite if e["name"] == "example_run"), "TIMEOUT"))
EOF
$ for d in build/prepush build/tiers-on; do
    echo "== $d =="
    ctest --test-dir "$d" -N --show-only=json-v1 | python3 /tmp/t045/census.py
  done
== build/prepush ==
entries 134 {300.0: 126, 120.0: 8}
['base_cli_matrix', 'harness_calibration_test', 'report_shape', 'report_shape_guide_sample', 'report_shape_hunt_rank_sample', 'report_shape_jitter_sample', 'report_shape_json_reports_sample', 'report_shape_percentiles_sample']
example_run 300.0
== build/tiers-on ==
entries 139 {300.0: 131, 120.0: 8}
['base_cli_matrix', 'harness_calibration_test', 'report_shape', 'report_shape_guide_sample', 'report_shape_hunt_rank_sample', 'report_shape_jitter_sample', 'report_shape_json_reports_sample', 'report_shape_percentiles_sample']
example_run 300.0
```

The eight 120 s names are the eight entries master gives 120 s, and
`example_run` carries the 300 s its row lists. The `ok` line of each
Verification run means the audit found no wrong-budget fault against
those nine rows in either tree. The example directory registers no
entry, so its empty list contributes no row and no entry, the reading
budget_rows.md records with its own grep of master.

## Verification

Both trees pass and the registered entry passes:

```
$ bash test/tier_labels.sh build/prepush ctest
read: 134 entries, 132 quick, 2 slow
ok: every entry carries one label and a budget, and neither registration breaks FR-048
exit=0

$ bash test/tier_labels.sh build/tiers-on ctest
read: 139 entries, 132 quick, 7 slow
ok: every entry carries one label and a budget, and neither registration breaks FR-048
exit=0

$ ctest --test-dir build/prepush -R test_tier_labels --output-on-failure
Test project /home/archerc/code/speedgun-ng/build/prepush
    Start 23: test_tier_labels
1/1 Test #23: test_tier_labels .................   Passed    0.17 sec

100% tests passed out of 1

Label Time Summary:
quick    =   0.17 sec*proc (1 test)

Total Test time (real) =   0.18 sec
exit=0
```

## The planted gap

`/tmp/t045` holds a scratch copy of the audit, scratch copies of the
two registration files, and a wrapper ctest program. The wrapper
answers the json listing with the `TIMEOUT` property of the entry
named by the `T045_PLANT` variable removed, and passes every other
call to the real ctest. The build directory stays `build/prepush`, so
the suite, the labels and the nine rows are the ones the trees carry.
Each plant hides one `TIMEOUT` property, and each fault line names the
entry:

```
$ T045_PLANT=harness_cli_test bash /tmp/t045/test/tier_labels.sh build/prepush /tmp/t045/fake-ctest
read: 134 entries, 132 quick, 2 slow
FAIL: no-budget: harness_cli_test holds no TIMEOUT and no exception row
test_tier_labels: 1 fault
exit=1

$ T045_PLANT=report_shape bash /tmp/t045/test/tier_labels.sh build/prepush /tmp/t045/fake-ctest
read: 134 entries, 132 quick, 2 slow
FAIL: wrong-budget: report_shape carries no TIMEOUT, its exception row lists 120 s
test_tier_labels: 1 fault
exit=1
```

`harness_cli_test` takes 300 s from the loop and holds no row, the
reading budget_rows.md settles under The entry that holds no row, so
hiding its property leaves the entry with neither and the no-budget
fault names it. `report_shape` keeps a row that lists 120 s, so hiding
its property trips the wrong-budget fault while the row still stands.
The audit as it stood at head, read from
`git show HEAD:test/tier_labels.sh` into `/tmp/t045/old`, passes that
second plant. That is the gap this task closed:

```
$ T045_PLANT=report_shape bash /tmp/t045/old/test/tier_labels.sh build/prepush /tmp/t045/fake-ctest
read: 134 entries, 132 quick, 2 slow
ok: every entry carries one label and a budget, and neither registration breaks FR-048
exit=0
```

## CI Readings

The rows stand on measurements from CI. Head sha
49c12ecee775ed6fafa66e18d341c081edd194b9, run 40239619604, the last
completed run of the branch at the time the task brief was written.
The five lanes are the jobs named `test`, `test-rocky`, `coverage`,
`sanitize` and `tsan`, with ids 121308544920, 121308544974,
121308544923, 121308545010 and 121308544961. Each lane is read with:

```
$ gh run view --job <job id> --log | sed -E 's/^[^Z]*Z //' \
    | grep -E 'Test +#[0-9]+: (harness_calibration_test|base_cli_matrix|report_shape|report_shape_guide_sample|report_shape_percentiles_sample|report_shape_json_reports_sample|report_shape_jitter_sample|report_shape_hunt_rank_sample|example_run) \.'
```

```
== lane test job 121308544920 ==
 54/139 Test  #62: harness_calibration_test .......................   Passed    0.13 sec
116/139 Test  #75: base_cli_matrix ................................   Passed   19.98 sec
117/139 Test #132: report_shape ...................................   Passed    1.04 sec
118/139 Test #133: report_shape_guide_sample ......................   Passed    8.80 sec
119/139 Test #134: report_shape_percentiles_sample ................   Passed    0.11 sec
120/139 Test #135: report_shape_json_reports_sample ...............   Passed    0.09 sec
121/139 Test #136: report_shape_jitter_sample .....................   Passed    1.17 sec
122/139 Test #137: report_shape_hunt_rank_sample ..................   Passed    1.99 sec
123/139 Test #138: example_run ....................................   Passed    3.41 sec
== lane test-rocky job 121308544974 ==
 52/139 Test  #62: harness_calibration_test .......................   Passed    0.13 sec
116/139 Test #132: report_shape ...................................   Passed    1.03 sec
117/139 Test #134: report_shape_percentiles_sample ................   Passed    0.09 sec
118/139 Test #135: report_shape_json_reports_sample ...............   Passed    0.11 sec
119/139 Test #136: report_shape_jitter_sample .....................   Passed    1.09 sec
120/139 Test #137: report_shape_hunt_rank_sample ..................   Passed    1.96 sec
121/139 Test #138: example_run ....................................   Passed    3.74 sec
123/139 Test #133: report_shape_guide_sample ......................   Passed    8.54 sec
124/139 Test  #75: base_cli_matrix ................................   Passed   21.10 sec
== lane coverage job 121308544923 ==
 57/134 Test  #57: harness_calibration_test .......................   Passed    0.85 sec
 70/134 Test  #70: base_cli_matrix ................................   Passed   22.74 sec
127/134 Test #127: report_shape ...................................   Passed    1.09 sec
128/134 Test #128: report_shape_guide_sample ......................   Passed    9.25 sec
129/134 Test #129: report_shape_percentiles_sample ................   Passed    0.22 sec
130/134 Test #130: report_shape_json_reports_sample ...............   Passed    0.18 sec
131/134 Test #131: report_shape_jitter_sample .....................   Passed    0.70 sec
132/134 Test #132: report_shape_hunt_rank_sample ..................   Passed    2.00 sec
133/134 Test #133: example_run ....................................   Passed    7.78 sec
== lane sanitize job 121308545010 ==
 52/134 Test  #57: harness_calibration_test .......................   Passed    0.74 sec
115/134 Test #127: report_shape ...................................   Passed    1.04 sec
116/134 Test  #70: base_cli_matrix ................................   Passed   26.54 sec
117/134 Test #129: report_shape_percentiles_sample ................   Passed    0.19 sec
118/134 Test #130: report_shape_json_reports_sample ...............   Passed    0.16 sec
120/134 Test #131: report_shape_jitter_sample .....................   Passed    1.06 sec
121/134 Test #132: report_shape_hunt_rank_sample ..................   Passed    1.80 sec
123/134 Test #128: report_shape_guide_sample ......................   Passed    9.66 sec
124/134 Test #133: example_run ....................................   Passed    6.47 sec
== lane tsan job 121308544961 ==
 50/135 Test  #60: harness_calibration_test .......................   Passed    0.67 sec
 84/135 Test  #73: base_cli_matrix ................................   Passed   22.17 sec
111/135 Test #128: report_shape ...................................   Passed    1.00 sec
113/135 Test #130: report_shape_percentiles_sample ................   Passed    0.15 sec
114/135 Test #131: report_shape_json_reports_sample ...............   Passed    0.16 sec
115/135 Test #132: report_shape_jitter_sample .....................   Passed    1.14 sec
116/135 Test #133: report_shape_hunt_rank_sample ..................   Passed    2.21 sec
118/135 Test #129: report_shape_guide_sample ......................   Passed    9.85 sec
120/135 Test #134: example_run ....................................   Passed    7.45 sec
```

The slowest lane of the five, per entry, of run 40239619604 at head
49c12ec read with the command above:

| entry | row lists | slowest lane | s |
| --- | --- | --- | --- |
| `harness_calibration_test` | 120 s | `coverage` | 0.85 |
| `base_cli_matrix` | 120 s | `sanitize` | 26.54 |
| `report_shape` | 120 s | `coverage` | 1.09 |
| `report_shape_guide_sample` | 120 s | `tsan` | 9.85 |
| `report_shape_percentiles_sample` | 120 s | `coverage` | 0.22 |
| `report_shape_json_reports_sample` | 120 s | `coverage` | 0.18 |
| `report_shape_jitter_sample` | 120 s | `test` | 1.17 |
| `report_shape_hunt_rank_sample` | 120 s | `tsan` | 2.21 |
| `example_run` | 300 s | `coverage` | 7.78 |

Every reading sits far inside the ceiling its row lists. The widest
120 s row is `base_cli_matrix` at 26.54 s on the `sanitize` lane of job
121308545010, and `example_run` peaks at 7.78 s on the `coverage` lane
of job 121308544923. FR-033 holds for the nine entries in every lane of
the run. Because the audit now ties each row to its property, a tree
that drifts from a listed value fails the entry the next time the suite
runs.

## The newer run

Run 40242340926 completed while this task read, and every test lane
closed there:

```
$ gh run view 40242340926 --json status,conclusion,headSha,jobs \
    | python3 /tmp/t045/jobs.py
run completed failure e77b713
lint completed success 121316622635
coverage completed failure 121316717683
sanitize completed failure 121316717684
static-analysis (tests) completed success 121316717694
tsan completed failure 121316717698
static-analysis (examples) completed success 121316717708
shared-audit completed success 121316717713
static-analysis (library) completed success 121316717719
test completed failure 121316717757
consumer-release completed success 121316717764
dbc-gate completed success 121316717774
downstream-consumer completed success 121316717808
prose-lint completed success 121316717875
test-rocky completed failure 121316717877
docs completed skipped 121320866439
```

The lane conclusions follow the base red. The per-entry seconds of that
run, head e77b713f5e86f6137bf81f004ee891171be48b52, are tabulated in
results/ph8/budget_rows.md, whose cross-check keeps the same shape:
every row's slowest reading stays far inside the ceiling its row
lists.

## Prose gate

The audit clears the prose gate in tree mode:

```
$ python3 tools/prose/prose_gate.py --check prose --mode tree --paths test/tier_labels.sh
prose-lint: 1 sources, 39 units examined, 0 findings, 0 skipped
exit=0
```

This record is untracked at head, so its read runs in a throwaway git
index under `/tmp`, where `git ls-files` finds it and tree mode has one
source to examine:

```
$ python3 /home/archerc/code/speedgun-ng/tools/prose/prose_gate.py \
    --check prose --mode tree --paths budget_check.md
prose-lint: 1 sources, 263 units examined, 0 findings, 0 skipped
exit=0
```

VERDICT: PASS
