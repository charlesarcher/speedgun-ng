# The acceptance hunt on the standing host

The hunt ran three times at head `cbeb678c` and once at the base. Every run
answered exit 0, ranked 497 events, and printed one `area` cell per ranked row,
so SC-007 has a reading from hardware behind it.

## The commands

T112 names `--timeout 120`. The harness carries no option of that name, and a
run with it answers `unknown option '--timeout'`. The hunt takes its bound from
`--hunt-budget`, so each run passed `--hunt-budget 120s`. The default budget is
`90s`. tasks.md carries the wrong option name, and the correction stands here.

```sh
./build/example/memory_walk --hunt --hunt-budget 120s
./build/example/first_benchmark --hunt --hunt-budget 120s
```

## The host

- Host `CorsairOne`, kernel `Linux 7.3.0-4-cachyos x86_64`, 24 cpus.
- Intel Core i9-9920X, model 85, the part the `arch/x86/skylakex/` table names.
- `kernel.perf_event_paranoid` at -1, set by the owner for this work.
- gcc 16.2.1 and clang 23.1.3.
- Version line: `speedgun-ng 0.31.0 build=Release`.

## The ranked rows

`memory_walk` planned 497 events over 249 passes and kept 493 after four gaps.
`first_benchmark` answered the same counts from the same catalog. Each run
grouped the ranked rows the same way.

| Rows | Kind | Values the cell answers |
|---|---|---|
| 58 | A rank that names a node of the tree | `backend_bound` 27, `frontend_bound` 17, `retiring` 11, `bad_speculation` 3 |
| 412 | A rank that names a group of the listing | `cache` 145, `memory` 113, `pipeline` 74, `frontend` 37, `virtual-memory` 25, `floating-point` 12, `other` 6 |
| 27 | A rank that neither the tree nor the listing names | `unmapped` |

The top four ranks read the slots and bubbles events and answer `unmapped`.
Ranks 8 to 12 read cache events and answer `cache`. The three counts stand at
58, 412 and 27 in all three head runs.

## The level-1 block

The run prints the block after the ranked rows. Its first two lines and its four
node rows follow verbatim, with the 13 table files and their hashes between them.

```text
topdown: level=1 passes=4 table=arch/x86/skylakex/
topdown: reference=linux-7.2.4
topdown:  topdown.backend_bound  tma_backend_bound  not available  tma_backend_bound > 0.2
topdown:  topdown.bad_speculation  tma_bad_speculation  not available  tma_bad_speculation > 0.15
topdown:  topdown.frontend_bound  tma_frontend_bound  not available  tma_frontend_bound > 0.15
topdown:  topdown.retiring  tma_retiring  not available  tma_retiring > 0.7 | tma_heavy_operations > 0.1
```

`not available` is the disclosure `source/harness/cli.cpp:521` sets where a node
carries no figure, so the block states the refusal of each level-1 node beside
the rule that would have set it. The block reaches the console because the
companion pass asks for the tree once per run, and the pass draws its figures
from the passes the hunt already pays for.

## What the base answered

The same hunt at `b1dd7670` printed 505 lines and no top-down block, and its
version line read `0.30.0`. Each head run prints 524 lines, of which the block
and its file list account for 19. The area cell is the second new text, and no
ranked row lost its event or its figure line.

Rank order is not a cross-run invariant on this host. Two head runs of the same
binary differ at 75 of 497 positions, and the head and the base differ at 93, so
no rank claim rests on these readings. FR-010 rests on the committed fixtures:
the emitted sets and the no-hunt console bytes stand byte identical, and the
example goldens hash unchanged.
