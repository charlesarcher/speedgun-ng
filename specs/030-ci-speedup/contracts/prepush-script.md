# Contract: Pre-Push Script

**Surface**: `tools/ci/prepush.sh`, its options, its step order, the line
each step prints, and its exit statuses. **Closes**: FR-029, FR-030,
FR-050.

## Options

| Option | Effect |
| --- | --- |
| none | the steps below through the quick tier, the two cpu pinning runs and the naming check |
| `--full` | adds the slow tier, the `consumer-release` build and the tsan configure and link |
| `--help` | prints the usage and exits 0 |
| anything else | exits 2 before any step runs (FR-030) |

## Steps, in order

| # | Step | Mirrors in CI | Command |
| --- | --- | --- | --- |
| 1 | drift gate | `lint` | `python3 tools/docs/example_drift.py .` |
| 2 | format gate | `lint` | `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake` |
| 3 | time-source gate | `lint` | `bash test/time_source_gate.sh .` |
| 4 | spell gate | `lint` | `cmake -P cmake/spell.cmake` |
| 5 | prose gate | `prose-lint` | `python3 tools/prose/prose_gate.py --base origin/master` |
| 6 | configure | `test` | `cmake --preset=prepush` |
| 7 | build | `test` `Build` | `cmake --build --preset prepush -j "$(nproc)"` |
| 8 | quick tier, full affinity | `test` `Test` | `ctest --preset prepush-quick -j "$(nproc)"` |
| 9 | quick tier, two cpus | the same tier under a runner's pinning | `taskset -c 0,1 ctest --preset prepush-quick -j 2` |
| 10 | naming check on the changed units | `static-analysis` | `clang-tidy --checks=-*,readability-identifier-naming` over the units `git diff --name-only origin/master` names |
| 11 | validate | the committed-record check of `test` | `cmake --build build/prepush -t validate` |
| 12 | slow tier | the CI suite | `ctest --preset prepush-full -j "$(nproc)"` (with `--full`) |
| 13 | consumer release build | `consumer-release` | the job's configure and build (with `--full`) |
| 14 | tsan configure and link | `tsan` | `cmake --preset=ci-tsan` and its build (with `--full`) |

Step 11 stands in every run; steps 12 to 14 run only under `--full`.

## Output line

Each step prints one line when it finishes:

```text
prepush: <step name> <seconds> s
```

A failing step prints its own command and the command's exit status, then
the script stops with that status. A step that cannot start, because a
tool is missing or the formatter version differs, prints the version found
and exits 1 before formatting anything (FR-030).

## Formatter gate

The script reads `clang-format --version` and requires 18.1.8, the version
CI runs. Any other version stops the run at step 2 (FR-030).

## Exit statuses

| Status | Meaning |
| --- | --- |
| 0 | every step passed |
| 1 | a step failed, or a required tool is missing or the wrong version |
| 2 | an unknown option |

## Timing obligation

SC-007 is read on the host the owner names, with a warm `ccache`: the run
finishes in 5 min or less, and the printed lines sum to within the wall
time of the script. `--full` prints a line for each of its three extra
steps (FR-029).

## Docs obligation

The `## Quick start` section of `README.md` names the tier names and
this script's path in 25 lines or fewer, and links the page of
`docs/pages/` that carries the command block and the step map (P-09,
FR-050). The drift gate checks the page and the linked page alike.

## Verify

```sh
tools/ci/prepush.sh --nope; echo "$?"        # prints 2
tools/ci/prepush.sh                          # one timing line per step
tools/ci/prepush.sh --full                   # the three extra lines
```

The unknown option exits 2 and runs nothing. The plain run prints one line
for each of steps 1 to 11, and `--full` adds three more. The bound of
SC-007 is read on the named lane host with a warm cache.
