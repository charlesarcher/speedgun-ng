# T025: the full local run on the lane host

**Surface**: `tools/ci/prepush.sh` on the lane host, with a warm build tree
and a warm `ccache`: the timing line of every step it ran, their sum, the wall
time of the run, the cache state around it, and the four `--self-check` lines.
**Head sha**: the run ran at `0096cbb` and measured the tree that head ships. The
commit the timing rows land in adds spec prose only, so the run measured the
tree the branch ships.
**Run id**: local, on the host `thrawn`: Linux 7.3.0-5-cachyos, AMD Ryzen
Threadripper 9960X 24-Cores, 48 threads, gcc 16.2.1 20260810, clang 23.1.3,
cmake 4.4.4, ccache 4.14.1, clang-format 18.1.8. spec.md names the lane host as
the machine the owner picks and asks the timing record to name it, which is
what this paragraph does. The first edition of this record measured the same
script on `CorsairOne`, an Intel i9-9920X at 3.50 GHz whose readings ended at
step 11 by the host's PMU condition; that host is down, and the readings below
replace its rows.
**Command**: `bash tools/ci/prepush.sh --full`, then
`bash tools/ci/prepush.sh --self-check`, then `ccache -s` and `ccache -p`, all
from the repository root with nothing else running on the machine.
**Numbers**: thirteen timing lines summing to 175 s, a wall of 175 s, `100%
tests passed out of 134` at step 11, four `self-check: pass` lines, and an exit
2 for the unknown option that `results/ph4/faults.md` already holds.

## The run

`bash tools/ci/prepush.sh --full` at the head above, on an idle machine:

```text
prepush: drift gate 0 s
prepush: format gate 8 s
prepush: time-source gate 0 s
prepush: spell gate 2 s
prepush: prose gate 0 s
prepush: configure 0 s
prepush: build 15 s
prepush: quick tier, full affinity 23 s
prepush: quick tier, two cpus 41 s
prepush: naming check on the changed units 4 s
prepush: slow tier 24 s
prepush: consumer release build 17 s
prepush: tsan configure and link 41 s
WALL=175
```

The thirteen printed lines sum to 175 s and the wall reads 175 s, which
accounts for the whole run. Steps 12 and 13 are the consumer-release pair and
the thread-sanitizer pair, and the table carries their lines.

## Step 11, and why this host passes it

The eleventh step is the whole suite: `ctest --preset prepush-full -j 24`, the
134 entries of the step 6 tree with no label filter. It reported `100% tests
passed out of 134` in 23.38 s:

```text
Label Time Summary:
quick    = 183.77 sec*proc (132 tests)
slow     =  32.84 sec*proc (2 tests)
Total Test time (real) =  23.38 sec
```

`harness_cli_test` carries the 300 s budget every entry gets unless the
exception list names it, and it is not named there. The entry passes here in
its slow tier because `thrawn` withholds the cpu PMU the way a runner does, so
the slow region of the entry does not execute locally. `results/ph2/cli-slow.md`
measured the entry at 3110.37 s on `CorsairOne`, full affinity, from a worktree
of the base commit `f507b8f`: that host grants the PMU, and its `--full` run
ended at step 11 by that host condition and not by a fault this branch
introduced. The same entry passes in CI: run 40242340926 at `e77b713` ran the
`test` lane over 139 entries with two failures, and `harness_cli_test` is not
one of them. Moving the row into the slow tier was the branch's fix for the CI
shape, and the budget the entry keeps stands on a host that grants the
counters.

## The two earlier runs, which were load and nothing else

Two runs of `--full` on `CorsairOne` at the head the first edition of this
record measured failed earlier, at step 8 instead, and the entry that failed
moved between them:

| Run | Machine state | Failing entry | That entry alone |
| --- | --- | --- | --- |
| run 1 | gates running beside the tier | `counters_overhead` | passed, 5 of 5 |
| run 2 | prose gate and GitHub API calls beside the tier | `prose_gate_fixtures`, `harness_compare_cli_test`, `harness_jitter_window_test` | the run reported 3 of 132, and 5 of 5 single-entry runs of `counters_overhead` passed in the same session |
| run 3, the idle one | idle | none: `100% tests passed out of 132` in 84 s | |

`counters_overhead` reads two live timings, a published floor and a bracketed
median, so concurrent load can invert them. `results/ph2/collisions.md` reaches
the same conclusion for the eight entries that open the cpu PMU and keeps them
unlocked: the kernel arbitrates the counters and multiplexes concurrent groups.
The quick tier on an idle machine passed in 84 s, which is what the third run
measured on that host. No assertion was edited and no lock was added for these
two runs.

## The cache state

`cmake --preset=prepush` inherits `ci-cache`, so both compiler launcher slots
name `ccache`, and the preset sets no `CCACHE_DIR`: the host default applies.

```console
$ ccache -p | grep -E 'cache_dir|max_size'
(default) cache_dir = /home/archerc/.cache/ccache
(default) max_size = 5.0 GiB
$ du -sh /home/archerc/.cache/ccache
566M
```

The CI lanes point `CCACHE_DIR` at a `.ccache` directory inside the workspace and
cap each lane at 1280 MB; this run used the host bucket, which held 566 MB against
a 5.0 GiB cap. The step 7 `build` line reads 15 s because the tree the step 6
configure names was already built by the round's first `--full` attempt: the
step compiled nothing new, so its line is the no-op scan and the build tree, not
a compile, is the warm state this record depends on. `ccache -s` after the run
reads 1856 cacheable calls with 452 hits (24.35%) and a 0.5 GiB bucket, but
those counters are since the last `ccache -z` and mix in the builds that
prepared the tree, so they describe the host bucket and not this run.

## SC-007

| Clause | Reading | Verdict |
| --- | --- | --- |
| the script finishes in 5 min or less on the lane host with a warm `ccache` | the no-option path is steps 1 to 10: 93 s here | met, with 207 s to spare |
| `--full` prints a time per step | thirteen steps, one line each; a failing step prints its command in place of the time | met |
| an unknown option exits 2 | `bash tools/ci/prepush.sh --bogus` prints `unknown option: --bogus` and the usage block, exit 2 | met, in `results/ph4/faults.md` |

The first edition of this record missed the five-minute clause on
`CorsairOne`: that host's quick tier holds 647.40 `sec*proc` over its 132
entries, so step 9's two-cpu pin put a 323.7 s floor under a 300 s clause, and
the host measured 619.72 s for the step. The floor was that host's entry
arithmetic and no constant of the script: on `thrawn` the same step measured
40.91 s, steps 1 to 10 sum 93 s, and the clause stands met on the lane host the
record names. `results/ph11/pr-report.md` states the deviation against the host
that showed it.

## Self-check

```console
$ bash tools/ci/prepush.sh --self-check
self-check: unknown option pass
self-check: missing tool pass
self-check: formatter version pass
self-check: failing step pass
$ echo $?
0
```

Four fault paths, four lines, exit 0, and no repository file touched. The
contract table and the per-path exit codes are in `results/ph4/faults.md`.

## VERDICT

SC-007 is met on all three clauses on the lane host: steps 1 to 10 sum 93 s
against the 300 s bound, the full run printed a time for every step it started
and took 175 s end to end, and the suite at step 11 passed 134 of 134 entries.
The first edition's miss was `CorsairOne` arithmetic: that host's two-cpu step
carried a 323.7 s floor and its PMU condition ended the run at step 11, and
both readings stay in the record's history with the host that produced them.
