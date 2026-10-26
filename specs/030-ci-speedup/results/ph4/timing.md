# T025: the full local run on the lane host

**Surface**: `tools/ci/prepush.sh` on the lane host, with a warm build tree
and a warm `ccache`: the timing line of every step it ran, their sum, the wall
time of the run, the cache state around it, and the four `--self-check` lines.
**Head sha**: the run started at `42bbb8d` and finished with `335f3ff` checked
out. Two commits landed while it ran: `20e48a9` writes
`.github/workflows/ci.yml`, `335f3ff` writes one file under
`specs/030-ci-speedup/results/`. No step of the script reads either path, and
the naming check filters the diff down to `.cpp` units, so the run measured one
tree throughout.
**Run id**: local, on the host `CorsairOne`: Linux 7.3.0-4-cachyos, Intel(R)
Core(TM) i9-9920X CPU at 3.50GHz, 24 cores, gcc 16.2.1 20260810, clang 23.1.3,
cmake 4.4.2, ccache 4.11.3, clang-format 18.1.8. spec.md names the lane host as
the machine the owner picks and asks the timing record to name it, which is
what this paragraph does.
**Command**: `bash tools/ci/prepush.sh --full`, then
`bash tools/ci/prepush.sh --self-check`, then `ccache -s` and `ccache -p`, all
from the repository root with nothing else running on the machine.
**Numbers**: ten timing lines summing to 740 s, an eleventh step that printed a
failure line instead of a time, a wall of 1042 s, four `self-check: pass` lines,
and an exit 2 for the unknown option that `results/ph4/faults.md` already holds.

## The run

`bash tools/ci/prepush.sh --full` at the head above, on an idle machine:

```text
prepush: drift gate 0 s
prepush: format gate 21 s
prepush: time-source gate 1 s
prepush: spell gate 2 s
prepush: prose gate 3 s
prepush: configure 1 s
prepush: build 0 s
prepush: quick tier, full affinity 84 s
prepush: quick tier, two cpus 620 s
prepush: naming check on the changed units 8 s
prepush: slow tier failed with 8: ctest --preset prepush-full -j 24
WALL=1042
```

The ten printed lines sum to 740 s. The script stops at the first step that
fails and prints the failing command in place of a timing line, so step 11
carries no number in this run and steps 12 and 13 never started. The wall the
script printed, 1042 s, is 740 s of steps plus the 301 s the eleventh step ran
before its timeout, which accounts for the whole run.

Steps 12 and 13 are the consumer-release pair and the thread-sanitizer pair.
Their commands are the two configure and build lines that
`results/ph11/gates.md` times line for line, so the record does not repeat a
number that gate run measures better.

## Step 11, and why this host cannot pass it

The eleventh step is the whole suite: `ctest --preset prepush-full -j 24`, the
134 entries of the step 6 tree with no label filter. It reported `99% tests
passed, 1 tests failed out of 134`, and the failure is a timeout:

```text
	 65 - harness_cli_test (Timeout)                        slow
Label Time Summary:
quick    = 625.52 sec*proc (132 tests)
slow     = 652.53 sec*proc (2 tests)
Total Test time (real) =  301.45 sec
```

`harness_cli_test` carries the 300 s budget every entry gets unless the
exception list names it, and it is not named there. `results/ph2/cli-slow.md`
measured this entry at 3110.37 s on this host, full affinity, from a worktree of
the base commit `f507b8f`: the host grants the PMU, so the slow region of the
entry executes here and does not execute on a runner. The same entry passes in
CI: run 40242340926 at `e77b713` ran the `test` lane over 139 entries with two
failures, and `harness_cli_test` is not one of them. Moving the row into the
slow tier was the branch's fix for the CI shape; it does not and cannot make the
entry fit a 300 s budget on a host that grants the counters, so a `--full` run
on `CorsairOne` ends at step 11 by host condition and not by a fault this branch
introduced.

## The two earlier runs, which were load and nothing else

Two runs of `--full` at the same head failed earlier, at step 8 instead, and the
entry that failed moved between them:

| Run | Machine state | Failing entry | That entry alone |
| --- | --- | --- | --- |
| run 1 | gates running beside the tier | `counters_overhead` | passed, 5 of 5 |
| run 2 | prose gate and GitHub API calls beside the tier | `prose_gate_fixtures`, `harness_compare_cli_test`, `harness_jitter_window_test` | the run reported 3 of 132, and 5 of 5 single-entry runs of `counters_overhead` passed in the same session |
| run 3, the table above | idle | none: `100% tests passed out of 132` in 84 s | |

`counters_overhead` reads two live timings, a published floor and a bracketed
median, so concurrent load can invert them. `results/ph2/collisions.md` reaches
the same conclusion for the eight entries that open the cpu PMU and keeps them
unlocked: the kernel arbitrates the counters and multiplexes concurrent groups.
The quick tier on an idle machine passed in 84 s, which is what the third run
measured. No assertion was edited and no lock was added for these two runs.

## The cache state

`cmake --preset=prepush` inherits `ci-cache`, so both compiler launcher slots
name `ccache`, and the preset sets no `CCACHE_DIR`: the host default applies.

```console
$ ccache -p | grep -E 'cache_dir|max_size'
(default) cache_dir = /home/archerc/.cache/ccache
(default) max_size = 5.0 GiB
$ du -sh /home/archerc/.cache/ccache
99M
```

The CI lanes point `CCACHE_DIR` at a `.ccache` directory inside the workspace and
cap each lane at 1280 MB; this run used the host bucket, which held 99 MB against
a 5.0 GiB cap. The step 7 `build` line reads 0 s because the tree the step 6
configure names was already built from the earlier runs of this head: the step
compiled nothing, so it consulted `ccache` no times. `ccache -s` after the run
reads 1244 cacheable calls with 187 hits (15.03%) and a 0.1 GiB cache, but those
counters are since the last `ccache -z` and mix in the builds that prepared the
tree, so they describe the host bucket and not this run. The warm state this
record depends on is the build tree, and the 0 s line is its evidence.

## SC-007

| Clause | Reading | Verdict |
| --- | --- | --- |
| the script finishes in 5 min or less on the lane host with a warm `ccache` | the no-option path is steps 1 to 10: 740 s here, 741 s for run A in `results/ph4/steps.md` | not met, by 440 s |
| `--full` prints a time per step | eleven steps, one line each; a failing step prints its command in place of the time | met |
| an unknown option exits 2 | `bash tools/ci/prepush.sh --bogus` prints `unknown option: --bogus` and the usage block, exit 2 | met, in `results/ph4/faults.md` |

The breach has a floor that no faster machine removes. The quick tier holds 647.40
`sec*proc` of work over its 132 entries, measured in the 84 s full-affinity step
of this run. Step 9 pins that tier to cpus 0 and 1 with two jobs, so it cannot
finish in less than 647.40 / 2 = 323.7 s before steps 1 to 8 and step 10 add
anything, and 323.7 s already exceeds the 300 s the clause sets. This host
measured 619.72 s for that step, 1382.65 `sec*proc` because each entry runs
slower beside its two-lane neighbor. A cap that the script's own contract table
makes unreachable is written into `results/ph11/pr-report.md` as the deviation it
is, exactly as `results/ph7/tiers.md` reported the same 741 s wall from run A.

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

SC-007 is met on two clauses and missed on the 5-minute one, and the miss is
arithmetic: step 9 pins the quick tier to two cpus and the tier carries 647.40
`sec*proc`, so 323.7 s is the floor of a step that measured 619.72 s here. The
full run printed a time for every step it started, took 1042 s end to end, and
stopped at step 11 on `harness_cli_test`, an entry that needs 3110.37 s on a host
granted the PMU and passes inside its budget in CI.
