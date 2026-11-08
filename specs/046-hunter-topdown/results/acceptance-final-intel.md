# Intel final acceptance: the hunter part

**Spec**: [spec.md](../spec.md) | **Plan**: [plan.md](../plan.md) | **Date**: 2026-11-07
**Branch**: `docs/intel-final-acceptance` | **Shared plan**: `specs/048-hwlat-probe/plan.md` | **Gates**:
`specs/048-hwlat-probe/results/acceptance-final-intel.md`

## Identity

| Field | Reading | Command |
|---|---|---|
| Host | CorsairOne | `uname -n` |
| Cpu | Intel(R) Core(TM) i9-9920X CPU @ 3.50GHz, family 6, model 85, `pmu_name` `skylake` | `grep -m1 "model name" /proc/cpuinfo`, `cat /sys/devices/cpu/caps/pmu_name` |
| Kernel | Linux 7.3.0-4-cachyos, x86_64 | `uname -a` |
| Library | speedgun-ng 0.34.0 | run header |
| Source base | `615c0964547e4e9768b8e2f2d79991033a0eb5bd` | `git rev-parse origin/master` |
| Measured head | `615c0964547e4e9768b8e2f2d79991033a0eb5bd` | `git rev-parse HEAD` |
| Root session | none | `sudo -n -l`, exit 1 |

No hunt run needs rights: the hunter opens its own events on one core device. Each run
below stood alone, with the load average and a count of other build or test processes
printed at its start; the counts read 0. An earlier pass of the same six runs ran at
14:22 inside the window of a coverage suite, and no figure from it enters this record.
Captures sit under `/tmp/ia/clean/hunt`.

## The two commands, three times each (IA46-001)

```
./build/example/memory_walk --hunt --hunt-budget 120s
./build/example/first_benchmark --hunt --hunt-budget 120s
```

The hunter writes no document for these runs, so each console stood captured by
redirection to its own file. No `--timeout` stood in any command and no method word stood
in any command. A hunt run prints no companion pass-method line and no coverage line, so
the method comes from the plan line alone.

| Run | Exit | Lines | Plan line |
|---|---|---|---|
| `memory_walk` 1 | 0 | 524 | `hunt: plan events=497 passes=249 method=PASSES estimate=293571ns` |
| `memory_walk` 2 | 0 | 524 | `hunt: plan events=497 passes=249 method=PASSES estimate=930264ns` |
| `memory_walk` 3 | 0 | 524 | `hunt: plan events=497 passes=249 method=PASSES estimate=913083ns` |
| `first_benchmark` 1 | 0 | 524 | `hunt: plan events=497 passes=249 method=PASSES estimate=1598580ns` |
| `first_benchmark` 2 | 0 | 524 | `hunt: plan events=497 passes=249 method=PASSES estimate=1622484ns` |
| `first_benchmark` 3 | 0 | 524 | `hunt: plan events=497 passes=249 method=PASSES estimate=1581150ns` |

The estimate moves and the counts do not. Each run printed four skip lines and the same
summary line:

```
hunt: skipped cpu/INST_RETIRED.PREC_DIST gap
hunt: plan summary events=493/497 passes=249 budget=120s
```

## The table the block names (IA46-002.3)

Every run printed the same table, the same reference and the same thirteen file digests:

```
topdown: level=1 passes=4 table=arch/x86/skylakex/
topdown: reference=linux-7.2.4
topdown: file cache.json 2408215a8a087bfe
topdown: file floating-point.json bce821558e518ec5
topdown: file frontend.json 2d746f41732b39ae
topdown: file memory.json 3c3adc0cd8e00e5d
topdown: file other.json b83ffee1f8371e0f
topdown: file pipeline.json 96f9b6b77505b27d
topdown: file skx-metrics.json 1ba7dd5bb986e865
topdown: file uncore-cache.json 8bb02fd054455da3
topdown: file uncore-interconnect.json 5cfdcae4c236db73
topdown: file uncore-io.json 964bd590600065a1
topdown: file uncore-memory.json 5408a7ff43bfbffc
topdown: file uncore-power.json 7f91f28632773d35
topdown: file virtual-memory.json 00f6c5f22164a09f
```

`skx-metrics.json` carries the top-down tree. Each digest is truncated to sixteen hex
characters here and stands complete in the captures. The level-1 block printed four
nodes, each `not available`, because a hunt run maps events and does not compute the
metric:

```
topdown:  topdown.backend_bound  tma_backend_bound  not available  tma_backend_bound > 0.2
topdown:  topdown.bad_speculation  tma_bad_speculation  not available  tma_bad_speculation > 0.15
topdown:  topdown.frontend_bound  tma_frontend_bound  not available  tma_frontend_bound > 0.15
topdown:  topdown.retiring  tma_retiring  not available  tma_retiring > 0.7 | tma_heavy_operations > 0.1
```

The only level any line prints is `level=1`.

## The three counts (IA46-002.1)

Each of the six runs printed 497 `area=` cells, and the split holds in every run:

| Run | Node | List-group | Unmapped | Sum |
|---|---|---|---|---|
| `memory_walk` 1 | 58 | 412 | 27 | 497 |
| `memory_walk` 2 | 58 | 412 | 27 | 497 |
| `memory_walk` 3 | 58 | 412 | 27 | 497 |
| `first_benchmark` 1 | 58 | 412 | 27 | 497 |
| `first_benchmark` 2 | 58 | 412 | 27 | 497 |
| `first_benchmark` 3 | 58 | 412 | 27 | 497 |

A cell names a node when it starts with `topdown.`, a group when it names one of the
seven list groups, and stands unmapped when it reads `unmapped`. SC-007 asks for
`58 / 412 / 27` and this host answered it in all six runs.

## The distinct cell texts (IA46-002.2)

Thirty-nine distinct cell texts in every run. The seven group values carry 412 cells:
`cache` 145, `memory` 113, `pipeline` 74, `frontend` 37, `virtual-memory` 25,
`floating-point` 12, `other` 6. `unmapped` carries 27 cells. The remaining 58 cells hold
node paths, and one run printed them like this:

```
hunt: rank - cpu/ARITH.DIVIDER_ACTIVE instr=0.000 cv=0.00 correlation=n/a slope=n/a
  delta=n/a area=topdown.backend_bound > topdown.intel.tma_core_bound > topdown.intel.tma_divider mark=unrepeated
```

Depth of the 58 node cells, in levels, identical in all six runs: 2 levels 5 cells,
3 levels 21, 4 levels 25, 5 levels 7. The standing audit point AP-9 names the deepest
level the mapper reaches, and this host agrees with it at five.

## Rank order is not a cross-run reading (IA46-002.5)

The 20 ranked rows split differently from run to run:

| Run | Node | Group | Unmapped |
|---|---|---|---|
| `memory_walk` 1 | 1 | 11 | 8 |
| `memory_walk` 2 | 2 | 11 | 7 |
| `memory_walk` 3 | 0 | 12 | 8 |
| `first_benchmark` 1 | 2 | 9 | 9 |
| `first_benchmark` 2 | 3 | 9 | 8 |
| `first_benchmark` 3 | 2 | 9 | 9 |

The order moves too. Run 1 of the walk ranks `cpu/ref-cycles` fourth and run 2 ranks
`cpu/branch-instructions` fourth. Run 3 of the walk ranks `cpu/topdown-fetch-bubbles`
first, where the other five runs rank `cpu/topdown-slots-issued` first. Run 3 of the
benchmark swaps ranks 3 and 4 against runs 1 and 2. The standing note AP-9 carries a
`1 / 10 / 9` split for this row, which is one reading of one run. The three totals above
hold across runs; the ranked split and the order do not.

## The marks (IA46-002.5)

Of the 477 unranked rows, 437 carry `mark=unrepeated` in every run. The remaining 40
divide between `beyond-top` and `noisy`, and that division moves: 25/15, 11/29 and 18/22
for the walk runs, 28/12, 28/12 and 26/14 for the benchmark runs. The 20 ranked rows carry
no mark.

## Mapping and limits

SC-007 carries the three counts and the node paths. SC-010 carries the device identity the
companion record of `specs/041-counter-probe` reads. AP-9 carries the deepest level and the
rank instability. This record makes no claim beyond the printed lines:

- No line prints a level, so no row says the run read one. The level quoted as printed is
  `level=1`.
- No rank order and no ranked split holds across runs.
- No AMD, Arm or Intel hybrid figure stands for an Intel reading. The Arm, Intel hybrid
  and Zen 4 rows of SC-010 stay pending, and no row this host measured contradicts them.
- No base re-run happened. A base build stands out of scope here and is named not run.
- No row carries a completion mark without the output behind it.


## Rerun at head `87d745d6c7e62d4da80198c689ddc7078f08ff3d` (2026-11-08)

**Base and measured head**: `87d745d6c7e62d4da80198c689ddc7078f08ff3d` (`origin/master`). That
head carries `d7e86254 Harness: Repair the contract-closure data fault`, which edits
`source/harness/hunt_topdown.cpp`, `source/harness/runner.cpp` and two headers, so the
hunter rows stand measured again on a tree that carries those edits. Host, cpu, kernel,
library and root state stand as in the identity block above.

Six runs, the same two commands three times each, each run alone with its load witness
printed at its start, each console captured by redirection under `/tmp/ia2/clean/hunt`.

| Run | Exit | Lines | Plan line |
|---|---|---|---|
| `memory_walk` 1 | 0 | 524 | `hunt: plan events=497 passes=249 method=PASSES estimate=927027ns` |
| `memory_walk` 2 | 0 | 524 | `hunt: plan events=497 passes=249 method=PASSES estimate=952176ns` |
| `memory_walk` 3 | 0 | 524 | `hunt: plan events=497 passes=249 method=PASSES estimate=925782ns` |
| `first_benchmark` 1 | 0 | 524 | `hunt: plan events=497 passes=249 method=PASSES estimate=426537ns` |
| `first_benchmark` 2 | 0 | 524 | `hunt: plan events=497 passes=249 method=PASSES estimate=1579407ns` |
| `first_benchmark` 3 | 0 | 524 | `hunt: plan events=497 passes=249 method=PASSES estimate=1648380ns` |

| Measure | Reading, same in all six runs |
|---|---|
| `area=` cells | 497 |
| Node cells | 58 |
| List-group cells | 412 |
| `unmapped` cells | 27 |
| Distinct cell texts | 39 |
| Node depths, in levels | 2 levels 5 cells, 3 levels 21, 4 levels 25, 5 levels 7 |
| Group split | `cache` 145, `memory` 113, `pipeline` 74, `frontend` 37, `virtual-memory` 25, `floating-point` 12, `other` 6 |
| Table block | `topdown: level=1 passes=4`, `topdown: reference=linux-7.2.4`, 13 file digests, identical across the six runs |
| Summary line | `hunt: plan summary events=493/497 passes=249 budget=120s` |
| Unranked rows with `mark=unrepeated` | 437 of 477 |

SC-007 holds at `58 / 412 / 27` on this head. The repair changed nothing about the counts,
the depths, the group split, the digests or the summary line.

The instabilities stand too. The 20 ranked rows split node, list-group and unmapped as
`0 / 11 / 9`, `0 / 12 / 8`, `0 / 12 / 8`, `1 / 10 / 9`, `2 / 9 / 9` and `1 / 11 / 8`, so no
ranked split is a cross-run reading. Rank 1 differs between the two binaries on this head:
the three walk runs rank `cpu/topdown-fetch-bubbles` first and the three benchmark runs
rank `cpu/topdown-slots-issued` first, and the walk's rank 1 moved between heads, so rank 1
is not a stable reading either. The 40 unranked rows that stand outside `unrepeated` divide
between `beyond-top` and `noisy` as `24 / 16`, `21 / 19`, `22 / 18`, `29 / 11`, `26 / 14`
and `24 / 16`.

The claim set stands where it stood: SC-007 carries the counts and the node paths, SC-010
carries the device identity the companion record reads, AP-9 carries the deepest level and
the rank instability, and the Arm, Intel hybrid and Zen 4 rows of SC-010 stay pending.
