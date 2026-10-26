# Phase 11 Every Gate, Run and Recorded

**Surface**: every gate the plan's Implementation Strategy names, run against the
head `88fc715` on this host or read from the lane that measures it, with the exit
status and the seconds of each (T060, Principle VIII, FR-062, SC-012).
**Head sha**: `88fc715` for every local reading and for both attempts of the
branch run; `fea69c5` for the master shape of the same lanes.
**Run id**: 40259383390 attempt 1 and attempt 2 for the CI lanes, 40042928839 for
master's lane walls.
**Command**: the lines quoted in each section, run from the repository root;
`gh api repos/charlesarcher/speedgun-ng/actions/runs/40259383390/attempts/<n>/jobs`
for the CI conclusions and job walls.
**Numbers**: three text gates at exit 0, two contract tests in 1.70 s, fifteen CI
jobs with twelve green, one skipped and two red at attempt 1, thirteen green and
two red at the end of the rerun.

## The text gates

```text
$ cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake; echo $?
0
$ bash tools/spell/check.sh; echo $?
0
$ python3 tools/prose/prose_gate.py --check prose --base origin/master; echo $?
prose-lint: 160 sources, 11077 units examined, 0 findings, 0 skipped
0
```

The prose gate reads the committed range by default, so a record that stands only
in the index is invisible to it. The records of this phase were therefore checked
a second way, and that second reading found six findings the first had never
looked at:

```text
$ python3 tools/prose/prose_gate.py --check prose --mode tree \
    --paths specs/030-ci-speedup/results/ph11/tables.md ... ; echo $?
prose-lint: 7 sources, 715 units examined, 0 findings, 0 skipped
0
```

Before the fix the same command printed three `XI1.DOUBLE-HYHEN` hits and three
contrastive hits. The dash hits were self-inflicted markdown: a hard wrap run
through the middle of an inline code span leaves the span unclosed, so the rest of
the line is prose to the parser and the `--jq` inside it reads as a two-hyphen
dash. The contrastive hits were two clauses of the form `X, not Y` and one of
`X rather than Y`, which Principle XI.2 rejects in favor of stating what the
thing is. Both classes are fixed in the records themselves, and the second reading
is the one that proves it.

## The label and contract gates

```text
$ ctest --test-dir build -R 'tier|dbc_gate_fixtures' --output-on-failure -j 4
100% tests passed out of 2
Total Test time (real) = 1.70 sec
```

`test_tier_labels` is #28 and `dbc_gate_fixtures` is #29 of the Release `build`
tree of this checkout, the `ci-rocky` shape. The label gate is the one FR-030 to
FR-034 are checked by: it fails when a registered entry carries no tier label or
when a budget row and the registered `TIMEOUT` part. `dbc_gate_fixtures` is the
same target the `dbc-gate` job builds and runs (`ci.yml:1284`), so the local
reading and the lane reading are of one thing.

## The coverage gate's own exits

`tools/dbc/coverage_gate.sh` has six recorded cases with their exit codes and
messages in `results/ph11/gate-exits.md`: usage and missing-tool paths exit 2, a
tool that fails to read the trace exits 3 with the tool's own status in the
message, and a measured shortfall exits 1. The exit-0 path and the real-tool
shortfall path cannot run on this host, whose `lcov` installs are both broken, so
they are read in the container that carries lcov 2.3 and in the CI lane, and
`results/ph11/trace-equality.md` carries those readings.

## The invariants the acceptance criteria name

```text
$ git diff --name-only origin/master...HEAD | grep -cE '^(include|source)/'
0
$ git show origin/master:CMakeLists.txt > /tmp/m-CML.txt; cmp -s /tmp/m-CML.txt CMakeLists.txt
$ echo $?
0
$ grep -nE 'VERSION 0\.20\.0|SOVERSION 4|SOVERSION 12' CMakeLists.txt
15:    VERSION 0.20.0
63:    SOVERSION 4
281:    SOVERSION 12
$ git diff -U0 origin/master...HEAD | grep -E '^\+' | grep -cE 'getenv|option\(|cmake_dependent_option'
1
```

The diff spans 79 files and none of them stands under `include/` or `source/`, so
no option table, environment read or schema key can have moved: those live in the
files the diff does not touch. `CMakeLists.txt` is byte-identical to master's, and
the three version lines above are the library's `VERSION`, `SOVERSION` for the
shared object and `SOVERSION` for the archive. The one line the last command
matches is prose in a record describing how CMake records an `option()` entry, not
a call to it.

## The CI lanes at the head

Fifteen jobs, attempt 1 of run 40259383390, walls from each job's own
`started_at` and `completed_at`:

| Job | Attempt 1 | Wall | Attempt 2 |
| --- | --- | --- | --- |
| `lint` | success | 28 s | not rerun |
| `dbc-gate` | success | 45 s | not rerun |
| `prose-lint` | success | 64 s | not rerun |
| `downstream-consumer` | success | 130 s | not rerun |
| `consumer-release` | success | 231 s | not rerun |
| `shared-audit` | success | 377 s | not rerun |
| `test-rocky` | success | 449 s | not rerun |
| `tsan` | failure | 472 s | success, 726 s |
| `static-analysis (library)` | success | 559 s | not rerun |
| `sanitize` | success | 619 s | not rerun |
| `static-analysis (examples)` | success | 619 s | not rerun |
| `test` | success | 677 s | not rerun |
| `coverage` | failure | 1108 s | running when this was read |
| `static-analysis (tests)` | success | 1292 s | not rerun |
| `docs` | skipped | 0 s | skipped |

The sum of the job walls is 6670 s against a run wall of 2267 s, which is the
parallelism the feature bought rather than a saving to claim. `docs` is skipped on
every pull-request run by the `if:` at `ci.yml:1416` and appears in this table only
so that the count of fifteen is complete; `results/ph11/docs-needs.md` carries why.

Master's twelve jobs at `fea69c5` read 215 s, 223 s and 240 s as per-job medians
across its three green runs, tabulated job by job against this head in
`results/ph11/tables.md`.

## What this host does not measure

`harness_cli_test` takes 3110.37 s here against its 300 s budget
(`results/ph2/cli-slow.md`), because this host grants the cpu PMU the entry
hammers and the CI runners do not. Every local full-lane run therefore reports
that one entry as a timeout, in every lane. The head run's `test` job reports it
`Passed` in 2.00 s inside `100% tests passed out of 139` with
`Total Test time (real) = 217.64 s` (job 121368562582), and
`quick = 114.86 sec*proc (132 tests)` against `slow = 670.63 sec*proc (7 tests)`
is the tier shape the budgets were set from. The slow tier behind that entry, the
two sanitizer lanes and the two consumer shapes are read from CI for the same
reason: `tools/ci/prepush.sh --full` runs them here at a cost that exceeds what
the host can give a bounded loop, and the lanes carry the same gates at the same
commit.

## The two red lanes

`tsan` failed `harness_context_barrier_test` at attempt 1 and passed it at attempt
2 of the same commit, in 0.45 s, inside `100% tests passed out of 135`. The same
entry failed on a different participant at `5ccd69a`. That is an intermittent
entry, and the numbers around it, including the four-slot measurement, are in
`results/ph11/tables.md`.

`coverage` failed its trace-processing step at both attempts for the lcov reason
in `results/ph11/coverage-lane.md`, and its suite passed at both. The second
attempt held 1087 s and stopped on the same `hunt_plan.cpp:365` record, so the
abort is deterministic (job 121374177396). No `ctest` line in `ci.yml` carries an
exclusion flag today; the lane's 134 entries against 139 elsewhere come from what
the `ci-coverage` preset registers, which is the five matrix entries it leaves
unset. Dropping entries was offered as the route that stays inside FR-041 and it
is measured in `results/ph11/coverage-lane.md`: six entries reach that closure on
their own, so the exclusion that clears the abort is the hunt group, and the hunt
files leave the trace with it. What the lane ends up doing is the owner's call and
is recorded here once it lands.
