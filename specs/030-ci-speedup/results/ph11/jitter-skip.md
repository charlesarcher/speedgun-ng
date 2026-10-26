# Phase 11 The Jitter Entry On A Four-Cpu Runner

**Surface**: the `harness_jitter_cli_test` registration and its `main`, and the
skip-rule baseline the tier audit compares against (T058, T060, FR-040, FR-047,
SC-012).
**Head sha**: `80de3f9` for every CI reading below; the fix lands on the head
that follows it.
**Run id**: 40252253175 and 40242340926 for the branch failures, 40203022166 and
40042928839 for what master's CI shows, 121346611508 for the quoted job log.
**Command**: `gh run view --log --job=<job>` for the failure line and the
summary block, `gh run view <run> --json jobs` for the job conclusions,
`git merge-base --is-ancestor` for the base claim, and the reproduction under
The reproduction.
**Numbers**: five lanes of one run and three of another, signal 6, 10.97 s on the
runner against 137.44 s here, cpu index 8, exit 2.

## What CI prints

The entry fails in every lane of every run of this branch, once per lane:

| Run | Lanes with exactly one failing entry |
| --- | --- |
| 40252253175 | `test` 81, `test-rocky` 81, `sanitize` 76, `tsan` 78, `coverage` 76 |
| 40242340926 | `test` 81, `tsan` 78, `coverage` 76 |

The number is the entry's registration index in that lane, which differs because
the lanes register different lists. The line the failure prints is the same in
all of them:

```text
$ gh run view --log --job=121346611508 | grep -a '#76: harness_jitter_cli_test' -A1
 76/134 Test  #76: harness_jitter_cli_test ...... ***Failed   10.97 sec
HARNESS JITTER CLI TEST FAIL: the exec child did not exit normally: mode
--jitter-cli-child=idle-high-index, status 134, signal 6
```

Status 134 is signal 6, an abort. The mode names the scenario: the idle
resolution of the CLI family. `37e068d test: Name the signal the exec child died
on` is the commit that made the line name its mode, which is why the cause is
readable at all.

## The cpu the scenario asks for

`childIdleHighIndex` installs the machine layout the M-5 host reports, two blocks
of cores at 0-1, 2-3, 8-9 and 10-11
(`test/source/harness_jitter_cli_test.cpp:1490` to `:1500`), so the idle
resolution has a second block to resolve against and yields cpu 8. The parent
asserts the child's console carries `jitter probe: cpus 8` (`:2185`) and the probe
pins to that index. A GitHub runner's affinity mask holds cpus 0 to 3; the pin has
no cpu 8, the contract path aborts the child, and the parent reads the death. On
this 24-core host the same binary passes: `Passed 137.44 sec` from
`ctest --test-dir build/prepush -R '^harness_jitter_cli_test$'`, 1 of 1.

Nothing else in the entry needs an index the runner lacks. `pickCpus` takes its
indexes out of the mask (`:127` to `:145`) and its largest call asks for three
(`:1899`). Its three sibling jitter entries pass in the same lane of the same run
(`#77 harness_jitter_preset_test Passed 0.75 sec` and the two beside it), as does
every entry of the hunt family.

## Master's CI never ran this entry

The claim "this was green at base" cannot be made from CI, and the claim "this
branch broke it" cannot either:

```text
$ git log --diff-filter=A --format='%h %ad' --date=short -- test/source/harness_jitter_cli_test.cpp
6226f83 2026-10-21
$ git merge-base --is-ancestor 6226f83 fea69c5; echo exit=$?
exit=1
$ gh run view --log --job=120718088140 | grep -ac jitter
0
```

The newest master run that reached a `ctest` step is 40042928839 at `fea69c5`,
whose tree predates the entry; its `test` job lists 120 entries and prints the
word `jitter` nowhere. The only master run at `f507b8f` is 40203022166, which
stops at `lint` with the other twelve jobs skipped. So this is a red the branch
inherits by being the first head whose CI reaches the entry, and the branch owns
fixing it.

The lane's parallelism is not the mechanism. `pickCpus` reads the mask, which
reads the same at `-j 2` as at `-j "$(nproc)"`, and the failure prints the same
mode and the same signal in a lane that runs one job and a lane that runs four.

## The rule the round added, and its removal

The same `ctest` summary distinguishes failure from inability:

```text
The following tests did not run:
   42 - counters_pmu_test (Skipped)
   48 - counters_overhead (Skipped)
```

`test/CMakeLists.txt:468` and `:546` register those with `SKIP_RETURN_CODE 2`, and
their sources print the reason and answer 2 where the kernel refuses what the
scenario would use. The round gave the jitter entry the same rule: `main`
read the affinity mask before any scenario, named the reason where the mask
held no cpu 8, and answered 2, with `SKIP_RETURN_CODE 2` on the
registration. FR-047 says an entry with a `SKIP_RETURN_CODE` keeps the rule
master sets, and master sets none here, so the deviation was stated in
full: the pin could not succeed on the runner's four cpus at any `-j`, and
the choice stood between a documented capability skip and a lane that
ever goes green.

The owner's review round removed the rule again, and the scenario now
takes its cpus from `sched_getaffinity`: the layout it installs names the
highest cpu of the process mask for the probe to start on, so the contract
abort at `source/harness/execution_context.cpp:659` stands on no runner and
on no `taskset -c 0,1` run, and the entry carries no skip rule. The CI
readings above stay as the record of the abort that produced the rule.

## The reproduction

`taskset -c 0-3` gives the entry the runner's mask on this host, and the binary is
the one the build tree holds:

```text
$ taskset -c 0-3 ./build/prepush/test/harness_jitter_cli_test; echo exit=$?
SKIP harness_jitter_cli_test: the affinity mask holds no cpu 8, so the idle pin
has no cpu to name (FR-040)
exit=2
$ ctest --test-dir build/prepush -R '^harness_jitter_cli_test$'
100% tests passed out of 1
quick    = 141.93 sec*proc (1 test)
```

The first line is the skip path answering 2 where the CI run read signal 6. The
second is the full mask, where cpu 8 exists and every scenario still runs to its
assertion: 141.93 s against the 137.44 s the entry took before the guard
existed, so the guard costs nothing on a host that can run the scenario. The
`test_tier_labels` entry passes with the new skip rule in place, and the
capability entries above it skip exactly as they did.

## The other CI failure

`harness_context_barrier_test` failed once, in the `tsan` lane of run
40252253175:

```text
HARNESS CONTEXT BARRIER TEST FAIL: the SPIN participant never sleeps inside the barrier: its voluntary-switch counter stands flat across the round (D-03, Q-3, F
```

It appears in no other lane of any other run, and master's `tsan` lane passed the
same entry in 0.74 s at `-j 2` (job 120718088244). One observation is recorded
here and left unattributed; if the run of the fixed head repeats it, the
registration lever the suite already carries for an entry that needs cpus to
itself is the first thing to try.
