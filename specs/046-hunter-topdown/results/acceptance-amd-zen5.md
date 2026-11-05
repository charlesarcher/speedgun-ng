# Master acceptance on the AMD Zen 5 host

**Spec**: [specs/046-hunter-topdown/spec.md](../spec.md), SC-009 and SC-007
**Host**: `gary` (the name `uname -n` answers), AMD Zen 5
**Master measured**: `858c55d9d036e74284dc48cd6fb7e42988cc8cff` (`858c55d9`),
2026-11-05
**Date**: 2026-11-05
**Branch**: `046-amd-acceptance`, one commit, records only

SC-009 asks one thing: on an AMD host, the hunt run names AMD top-down nodes and
no Intel node. This host answers that question, and it answers a second question
nobody asked: the hunt command this record ran first aborts on master, on the
passes path, before it prints a row. Both readings stand here, and the second
names a defect outside this feature.

## The host

| Item | Answer |
|---|---|
| Host name | `gary` |
| Vendor | AuthenticAMD |
| Model name | AMD Ryzen 9 9950X3D 16-Core Processor |
| CPU family | 26 (0x1A) |
| Model | 68 |
| Stepping | 0 |
| Logical cpus | 32 |
| Kernel | `Linux 7.3.1-1-cachyos x86_64` |
| Compiler | gcc (GCC) 16.2.1 20260810 |
| `kernel.perf_event_paranoid` | 2, the value the host carries |
| `kernel.nmi_watchdog` | 0 |
| Version line | `speedgun-ng 0.31.0 build=Release` |
| Master sha measured | `858c55d9` |
| Schema, `SOVERSION`s | 1.14, harness 21, archive 6 |
| `LCOV_EXCL` markers, `README.md` lines | 84, 1001 |

The build is `cmake --preset=ci-rocky` and `cmake --build build`, a Release
developer-mode build, into `build`.

## The commands

`results/acceptance-hunt.md` names the hunt commands of the Intel acceptance:

```sh
./build/example/memory_walk --hunt --hunt-budget 120s
./build/example/first_benchmark --hunt --hunt-budget 120s
```

Three runs of each, in that shape, answered the same thing every time:

```text
exit 134
[assertion] the ratio pair is written together or left together (FR-004)
(predicate: entry.ratioRunning != PlanImpl::kNoRatioSlot)
at source/counters/fold.cpp:166
```

No run printed a version line, a plan line, a ranked row, an area cell, a
level-1 block or an unmapped count. The run died before it planned on the
console. Section "The abort" states what the abort is and what it is not.

Because that command cannot answer SC-009, the same two benchmarks ran again
with one option added, `--metric-method multiplex`, which is the collection
method of specs/024 that the run below settled on its own when the passes path
was not chosen. Three runs of each completed with exit 0 and 730 lines each.

```sh
./build/example/memory_walk --hunt --hunt-budget 120s --metric-method multiplex
./build/example/first_benchmark --hunt --hunt-budget 120s --metric-method multiplex
```

## SC-009: the AMD reading

Every one of the six completed runs answered the same three counts and the same
table. The plan line of each run:

```text
hunt: plan events=353 passes=89 method=MULTIPLEX estimate=119260ns budget=120s stage1=353 stage2=60
```

| Rows | Kind | Values the cell answers |
|---|---|---|
| 11 | A rank that names a node of the selected AMD table | `topdown.bad_speculation > topdown.amd.bad_speculation_from_mispredicts` 5, `topdown.backend_bound > topdown.amd.backend_bound_by_memory` 3, `topdown.frontend_bound` 1, `topdown.retiring > topdown.amd.retiring_from_fastpath` 1, `topdown.smt_contention` 1 |
| 334 | A rank that names a group of the listing | `floating-point` 135, `load-store` 86, `l2-cache` 44, `execution` 27, `branch-prediction` 15, `decode` 15, `inst-cache` 12 |
| 8 | A rank that neither the tree nor the listing names | `unmapped` |

- The selected table, from the run's own line: `table=arch/x86/amdzen5/`, with
  `reference=linux-7.2.4`. The family (26) and the model (68) above are the
  identity that selection read.
- No Intel node: the string `intel` appears 0 times in any of the six logs, and
  every tree-node cell spells an AMD node (`topdown.amd.*` below its level-1
  node). No `tma_` spelling appears either.
- The 8 `unmapped` rows are the ranks neither the Zen 5 tree nor the listing
  names. The run prints the cell `unmapped` for them; it prints no reason beside
  the cell, so this record gives the reason the tool states: FR-008 of this spec
  settles the value, and no refusal line accompanies it.

The level-1 block of the run, verbatim, once per benchmark:

```text
topdown: level=1 passes=1 table=arch/x86/amdzen5/
topdown: reference=linux-7.2.4
topdown:  topdown.frontend_bound  frontend_bound  64.42%
topdown:  topdown.bad_speculation  bad_speculation  0.52%
topdown:  topdown.backend_bound  backend_bound  24.84%
topdown:  topdown.smt_contention  smt_contention  0.00%
topdown:  topdown.retiring  retiring  10.28%
topdown: the four shared level-1 values add to less than 100 percent without topdown.smt_contention
```

The block carries figures here, where the Intel record's block carried
`not available` on every node.

**SC-009 answer**: pass on AMD Zen 5 for the run that completes. SC-010's AMD Zen
4 row stays pending, and the row of the default-method command of this feature
stays open with the abort below.

## The abort

The command of `results/acceptance-hunt.md` aborts on `858c55d9` on this host.
What was measured, so a reader can see what this is and is not:

| Shape | Runs | Answer |
|---|---|---|
| `--hunt --hunt-budget 120s`, `paranoid` 2 | 6 (3 per benchmark) | abort 134 |
| `--hunt --hunt-budget 120s`, `paranoid` -1 | 4 | abort 134 |
| `--hunt --hunt-budget 30s --metric-method passes` | 1 | abort 134 |
| `--hunt --hunt-budget 120s --metric-method multiplex` | 6 | exit 0, 730 lines |
| `--hunt=cpu/branch-instructions --hunt-budget 5s` | 1 | exit 0, prints the level-1 block |

The trigger is the passes path. It is neither the command-line shape nor the
host gate. `strace` over one failing run at `paranoid` 2 lists 1129
`perf_event_open` calls, 353 refused with `EACCES` (the raw-encoded events the
gate refuses) and 61 refused with `EINVAL`. The same hunt at `paranoid` -1 lists
the same 1129 calls with only the 61 `EINVAL` refusals, and it aborts the same
way, so the gate is not the cause. The host setting was restored to 2 after
that check.

The failing predicate is the project's own contract at
`source/counters/fold.cpp:166`, and the predicate states the state exactly:
`entry.ratioEnabled` resolved to a slot and `entry.ratioRunning` did not.
`linkRatioSlots` (`source/counters/plan.cpp:132-153`) sets those two members in
two independent `if` arms, so a plan that carries `<device>/enabled` and not
`<device>/running` leaves half a pair, and `leafRatio` reads that as the breach
of FR-004 of specs/024. The backtrace of the abort runs
`sg::hunt::runHuntNamed` → `sg::detail::Runner::run` → `nettedFold` →
`sg::counters::detail::foldCore` → `leafRatio` → `sg::dbc::detail::dispatch`.

This feature does not fix it. specs/046 adds an area cell to a rank; the abort
stands in the pass planner and the fold of specs/024, and it needs its own lane.
The missing evidence for it is here: the row counts of `--metric-method passes`
are not reachable at all, because no run of that shape survives the fold.

## ZA-003: the suites

| Command | Entries | Passed | Failed | Skipped | Wall time |
|---|---|---|---|---|---|
| `ctest --test-dir build --output-on-failure` | 181 | 180 | 0 | 1 | 308.12 s |
| `taskset -c 0,1 ctest --test-dir build --output-on-failure` | 181 | 180 | 0 | 1 | 257.39 s |

No test failed on either row. One entry is reported skipped:
`harness_topdown_run_test`, which exits with the skip code the registration
reserves (`SKIP_RETURN_CODE 2`) after one scenario of its own skips with `the
identity of this host names a directory beside the fixture's`. The other 26
scenarios of that entry ran and passed, so ctest's single line hides 26 passing
scenarios; the entry is not a host gap.

The suite is green while the hunt aborts, because no entry of it hunts a full
catalog through the passes path on a host of this class. specs/046's own units
drive the mapper and the emit paths on scripted identities
(`harness_hunt_topdown_test`, `counters_topdown_groups_test`).

## Pending rows after this record

- AMD Zen 4: no host. Pending, no hardware claim.
- The default-method hunt command of `results/acceptance-hunt.md` on AMD Zen 5:
  open with the abort above, and open for the specs/024 pass planner and fold
  that own the ratio pair.
- SC-010's other rows stand as they were: Arm, Intel hybrid, and `PERF_METRICS`
  parts.

## What this record changes in the spec

`SC-009` of [spec.md](../spec.md) now names AMD Zen 5 as measured, with the one
option this host needed, and links here. `SC-010` keeps Zen 4 pending.
