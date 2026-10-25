# The Two Test Tiers and What Each Costs (030 FR-025, FR-026, SC-007)

**Surface**: the tier labels of `test/CMakeLists.txt`, the tier audit
`test/tier_labels.sh`, steps 8, 9 and 11 of `tools/ci/prepush.sh`, and the
`Test` step of the `test` job of `.github/workflows/ci.yml`.
**Head sha**: e77b713f5e86f6137bf81f004ee891171be48b52
**Run id**: run A of `specs/030-ci-speedup/results/ph4/steps.md`, a plain run
of the mirror script on this host, and GitHub Actions run 40239619604 of
branch `030-ci-speedup`.
**Command**: the block under Commands, each line run from the repository root.
**Numbers**: the two tables below, with the pending line under Timings.

Every registered entry carries one tier label. The quick tier is what
`tools/ci/prepush.sh` runs before a push, once with the whole host and once
pinned to two cpus. The slow tier holds the entries that cost minutes each,
and the full option adds it. No `ctest` line of the workflow names a label,
so a CI job runs both tiers and the labels move local time alone.

## Entries

The audit reads the labels and the budgets off a configured tree. Two trees
stand at head, one per lane flag.

```text
$ bash test/tier_labels.sh build/prepush ctest
read: 134 entries, 132 quick, 2 slow
ok: every entry carries one label and a budget, and neither registration breaks FR-048
exit 0

$ bash test/tier_labels.sh build/tiers-on ctest
read: 139 entries, 132 quick, 7 slow
ok: every entry carries one label and a budget, and neither registration breaks FR-048
exit 0
```

`build/prepush` is the tree the two test presets name, where
`SPEEDGUN_DBC_MATRIX` stays off. `build/tiers-on` is configured by
`ci-ubuntu-test`, the preset the CI `test` job runs, which registers the five
`dbc_matrix` cells. One list decides every label: the `SPEEDGUN_SLOW_TESTS`
rows at `test/CMakeLists.txt:1623` carry `slow` at `test/CMakeLists.txt:1646`
and anything else takes `quick` at `test/CMakeLists.txt:1648`.

| Tier | Entries in `build/prepush` | Entries in `build/tiers-on` | Members at head |
| --- | --- | --- | --- |
| quick | 132 | 132 | every entry the slow list leaves out |
| slow | 2 | 7 | `harness_cli_test`, `snippet_compile`, the five `dbc_matrix` cells |
| registered | 134 | 139 | the two audit read lines quoted above |

## Timings

`tools/ci/prepush.sh:195` and `tools/ci/prepush.sh:196` are the two quick tier
steps, and `tools/ci/prepush.sh:256` is the slow tier step behind the full
option. The CI row is the step row 8 of the contract table mirrors: the
`Test` step of the `test` job, whose command is the `ctest` line at
`.github/workflows/ci.yml:469`. The `steps.md` cites below are
`specs/030-ci-speedup/results/ph4/steps.md`.

| Reading | Command | Seconds | Read from |
| --- | --- | --- | --- |
| quick tier, full affinity | `ctest --preset prepush-quick -j "$(nproc)"` | 87 | `steps.md:90` |
| quick tier, two cpus | `taskset -c 0,1 ctest --preset prepush-quick -j 2` | 622 | `steps.md:91` |
| plain run, ten steps | `bash tools/ci/prepush.sh` with no option | 741 | `steps.md:93` |
| full suite, CI, same step | `ctest --output-on-failure --no-tests=error -C Release -j "$(nproc)"` | 298.82 | run 40239619604, job 121308544920, the ctest summary line |
| quick tier, CI step | the same `ctest` line, unfiltered | 300 | run 40239619604, job 121308544920, the step wall clock |
| slow tier, local | `ctest --preset prepush-full -j "$(nproc)"` | pending | `bash tools/ci/prepush.sh --full` |

The pending line is the reading task T025 owns at
`specs/030-ci-speedup/results/ph4/timing.md`: the full option prints
`prepush: slow tier N s` for step 11, and `grep -rn 'prepush: slow tier'`
over `specs/` and `tools/` finds no such line at head.

The quick tier has no CI counterpart to time. CI names no label on any
`ctest` line, so the step the mirror script's row 8 reflects runs all 139
entries, and its 300 s of step wall covers both tiers. The step ran from
`22:25:09Z` to `22:30:09Z` on 2026-10-25.

Run 40239619604 stands at head `49c12ec`, concluded failure, and its suite
line reads `99% tests passed, 1 tests failed out of 139`, the one failure
named `81 - harness_jitter_cli_test (Failed)`, a quick entry. The two CI
readings are the same whole-suite step, timed by ctest and by the runner.
`specs/030-ci-speedup/results/ph7/no-filter.md:131` records the matching CI
reading, `ctest presets CI uses: []`.

## The budget a plain run breaches

The plain run breaches SC-007, the 5 minute budget of
`specs/030-ci-speedup/spec.md:925`, which asks `tools/ci/prepush.sh` to
finish in 5 min or less on the recorded lane host with a warm `ccache`. The
wall is 741 s against 300 s, and the two cpu pinning step alone accounts for
622 s of it.

## Commands

```sh
cmake --preset=prepush -B build/prepush
cmake --preset=ci-ubuntu-test -B build/tiers-on
bash test/tier_labels.sh build/prepush ctest
bash test/tier_labels.sh build/tiers-on ctest
ctest --test-dir build/prepush -N | tail -1
ctest --test-dir build/prepush -N -L quick | tail -1
ctest --test-dir build/prepush -N -L slow | tail -1
ctest --test-dir build/tiers-on -N | tail -1
ctest --test-dir build/tiers-on -N -L quick | tail -1
ctest --test-dir build/tiers-on -N -L slow | tail -1
gh run view 40239619604 --json headSha,conclusion,jobs
gh run view --job 121308544920 --log
grep -n 'SC-007' specs/030-ci-speedup/spec.md
grep -n 'quick tier, \|slow tier' tools/ci/prepush.sh
```

## Gates

```text
$ python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/030-ci-speedup/results/ph7/tiers.md
prose-lint: 1 sources, 108 units examined, 0 findings, 0 skipped
exit 0

$ cmake -P cmake/spell.cmake
Used config files:
    1: .codespellrc
exit 0
```

Tree mode lists tracked files through `git ls-files`, so the count above
comes from `/tmp/tiers-prose-check`, a throwaway repository that holds a copy
of the gate, its rule file and this page, staged in that index. The main
tree keeps its own index and reports 0 sources for a page the driver has not
committed.

VERDICT: PASS. Six readings, five with a cited number and one pending line
that names its command.
