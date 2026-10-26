# Quickstart: CI Speed-Up

**Date**: 2026-10-25 · **Spec**: [spec.md](./spec.md) ·
**Plan**: [plan.md](./plan.md)

This guide runs the feature locally, then reads the CI numbers the
acceptance criteria ask for. Every command names a checked-in preset, so
a fresh clone with no `CMakeUserPresets.json` runs the whole guide.

## Prerequisites

- Linux, GCC or Clang with C++23, CMake 3.20 or newer.
- clang-format 18.1.8, the version CI runs. `tools/ci/prepush.sh` stops at
  any other version.
- `ccache` on the path, `lcov`, `python3`, `codespell`, and `gh`
  authenticated on `charlesarcher/speedgun-ng`.
- `nproc` at least 4 on the lane host, which is what the 5-minute bound of
  SC-007 is read on.

## First build, and the two tiers

```sh
cmake --preset=ci-rocky
cmake --build build
ctest --test-dir build --output-on-failure
```

Then the tier pair and the local preset:

```sh
cmake --preset=prepush
cmake --build build/prepush -j "$(nproc)"
ctest --preset prepush-quick -j "$(nproc)"
ctest --preset prepush-full -j "$(nproc)"
ctest --test-dir build/prepush -N -L quick | tail -2
ctest --test-dir build/prepush -N -L slow | tail -2
ctest --test-dir build/prepush -R test_tier_labels --output-on-failure
```

Expected: the quick tier finishes in minutes while the full suite spans
longer, `test_tier_labels` passes, the two label lists are disjoint, and
their union equals `ctest -N`. On a `ci-coverage` tree `ctest -N` shows no
`dbc_matrix_*` entry, and on a `ci-ubuntu-test` tree it shows five.

## The pre-push check

```sh
tools/ci/prepush.sh
tools/ci/prepush.sh --full
tools/ci/prepush.sh --nope; echo "$?"
```

Expected: one `prepush: <step> <seconds> s` line per step, ten lines for
the plain run and thirteen with `--full`, and exit 2 for the unknown
option with no step run. The plain run on the named lane host with a warm
`ccache` is the SC-007 reading: it prints 740 s of step seconds there, and
step 9 pins a tier holding 647.40 `sec*proc` to two cpus, so 323.7 s is its
floor (`results/ph4/timing.md`).

## Cache behavior

```sh
ccache -z
cmake --build build/prepush -j "$(nproc)"
ccache -s
```

Expected: a warm cache reports a hit rate above zero after a prior build of
the same tree, and `ccache -s` shows `basedir` and `sloppiness` unset
(FR-011). In CI each building job prints the same two readings around its
build (FR-009).

## The gates, in the order the script mirrors

```sh
python3 tools/docs/example_drift.py .
cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake
bash test/time_source_gate.sh .
cmake -P cmake/spell.cmake
python3 tools/prose/prose_gate.py --base origin/master
ctest --test-dir build -R 'loop_shape|cli_shape' --output-on-failure
```

Expected: all pass. `example_drift.py` reads `PAGE_LINE_CAP`, which moves
with the README block (P-09), and the two shape gates print the base
readings, because no source file changes.

## Sanitizer and coverage lanes

```sh
cmake --preset=ci-sanitize && cmake --build build/sanitize
ctest --preset ci-sanitize
cmake --preset=ci-tsan && cmake --build build/tsan
ctest --preset ci-tsan
cmake --preset=ci-coverage && cmake --build build/coverage
cd build/coverage && ctest -j 1 --output-on-failure --no-tests=error && cd ../..
cmake --build build/coverage -t coverage
```

Expected: the `sanitize` and `coverage` lanes run their entry set apart from
the five matrix entries, which `ci-sanitize` and `ci-coverage` leave unset,
while `tsan` configures `SPEEDGUN_DBC_MATRIX` on and registers all five
(FR-046, `CMakePresets.json:138`, `:164`, `:171`). The coverage lane reaches
the gate. The gate wants 100% lines and 100% branches over the application
sources (`tools/dbc/coverage_gate.sh:45`, `:55`); `spec.md:218` records a
base reading of 99.4% lines (9752 of 9815) and 97.5% branches (4772 of
4893), the shortfall SC-003 says this branch must not widen and spec 032
owns. The gate exits 0 for a pass, 1 for a shortfall, 2 for a usage fault
and 3 for a refused trace (FR-043).

## The analyzer split

```sh
cmake --preset=ci-ubuntu
cmake --build build --target speedgun-ng_speedgun-ng speedgun-ng_harness
cmake --build build/test
cmake --build build/example
```

The `ci-ubuntu` preset configures into `build`, the directory `ci-rocky`
also uses, so run these three in a checkout of their own or after the tier
checks above.

Expected: the three scopes together analyze every translation unit the
default build compiles, the same finding set master's `test` job printed,
and one planted `readability-identifier-naming` violation fails the shard
that builds its unit (FR-016, SC-011). On a warm cache the violation still
fails, because CMake analyzes before the compile on a cache hit (FR-017).

## The CI readings

Read the base and the branch with `gh`, then write the before and after
table (FR-051, SC-001):

```sh
gh api repos/charlesarcher/speedgun-ng/actions/runs/40042928839/timing \
  --jq .run_duration_ms
gh api repos/charlesarcher/speedgun-ng/actions/runs/40042928839/jobs \
  --jq '.jobs[] | [.name, .conclusion,
        ((.completed_at | fromdate) - (.started_at | fromdate))] | @tsv'
gh run list --branch 030-ci-speedup --limit 5
```

Fill the acceptance readings in this order, each from runs that completed:

| Reading | How | Criterion |
| --- | --- | --- |
| median wall time, three master pushes and three pull-request pushes | `gh api .../runs/<id>/timing` on each | SC-002, 15 min or less |
| cold-cache run | push with the cache key forced to miss, then compare verdicts per job | SC-006 |
| install-step time | the install step of each job record | FR-037, SC-002 |
| test-list difference | `ctest -N` per job on master and on the branch | SC-004 |
| finding lists | the two uploaded artifacts, sorted, diffed | SC-005, SC-011 |
| per-entry times against budgets | the `Test` step log of each job | SC-008 |
| version and ABI | `grep -n 'VERSION 0.20.0\\|SOVERSION' CMakeLists.txt` | SC-009 |
| red jobs and their owners | each red job's log, plus the base record | SC-012 |

One run of the PR #47 head at the audit point is timed with its slowest job
named, and that run belongs in the table (FR-052).

## What each story proves on its own

| Story | Reading that closes it |
| --- | --- |
| US1, the 15-minute verdict | the median of six runs, and the cold-cache verdict equality |
| US2, the 5-minute local check | the script's timing lines on the named host, and the exit 2 case |
| US3, the analyzer job | the finding-list diff and the planted violation |
| US4, one row per cell | the `ctest -N` diff per job and the three compiler records |
| US5, the tiers | `test_tier_labels`, and no label filter in any CI `ctest` line |
| US6, the budgets | the per-entry times against the budgets, and the exceptions list |
| US7, the install steps | the recorded install-step times, warm and missed |
| US8, the cancellation | two push runs where the older reports canceled, and a master run that stands |

## Where a run can mislead

A canceled run carries no timing that may enter a table. A cold-cache run
and a warm-cache run must be compared by verdict, and a cold run's wall
time gets its own row beside the warm median. A
`coverage` job that fails on a shortfall still prints its step times, which
is how SC-003 is read while the shortfall itself belongs to spec 032
(FR-064). A `coverage` job whose trace step aborts prints no trace at all, so
its suite times stand and the gate reading is absent;
`results/ph11/coverage-lane.md` shows that shape. A `lint` failure before
FR-065's commit lands skips the other fourteen jobs, so its run carries no
timing at all, the reading PC-10 records at the base.

A `docs` job is skipped on every pull request by a condition on the job itself,
so no run of a branch shows what its `needs` list waits for
(`results/ph11/docs-needs.md`). The prose gate reads the committed range, so
`--mode tree` with `--paths` is how a staged file is examined, and that second
reading is what found six findings in the records of phase 11
(`results/ph11/gates.md`). `harness_jitter_cli_test` prints `Skipped` on a runner
whose affinity mask holds no cpu 8, which reports a capability the machine lacks
(`results/ph11/jitter-skip.md`).
