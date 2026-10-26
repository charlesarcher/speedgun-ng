# Phase 11 Before and After Walls and Steps

**Surface**: the run walls, job walls and per-step seconds of the three master
pushes and of the branch runs, plus the analyzer unit counts and the audit point
run (T058, FR-051, FR-052, SC-001, SC-002).
**Head sha**: `bd10d76`, `fea69c5` and `9976768` for master; `e77b713`,
`5ccd69a`, `80de3f9` and `88fc715` for the branch.
**Run id**: 39708015373, 40042928839, 39659845534, 40242340926, 40252253175,
40255220576 and 40259383390.
**Command**: `gh api` on the run's `timing` endpoint for a wall, on its `jobs`
endpoint for the job walls, and `gh run view --log --job=<job>` for the `Build`,
`Test` and ctest totals. The job listing maps names to durations with
```text
.jobs[] | [.name, .conclusion,
      ((.completed_at | fromdate) - (.started_at | fromdate))] | @tsv
```
The head run is read at
`.../runs/40259383390/attempts/1` because its rerun (below) resets the run-level
view to the newer attempt.
**Numbers**: master's median wall 2466 s against the branch's median job wall
449 s; `test` builds 2277 s on master and 216 s on the branch.

## The walls

Three master pushes, all `success`, and four runs of this branch:

| Run | Event | Head | Wall | Slowest job |
| --- | --- | --- | --- | --- |
| 39708015373 | push | `bd10d76` | 1732 s | `test-rocky` 1622 s |
| 39659845534 | push | `9976768` | 2466 s | `test` 2374 s |
| 40042928839 | push | `fea69c5` | 2697 s | `test` 2621 s |
| 40242340926 | pull_request | `e77b713` | 2131 s | `static-analysis (tests)` 2102 s |
| 40252253175 | pull_request | `5ccd69a` | 2134 s | `static-analysis (tests)` 2102 s |
| 40255220576 | pull_request | `80de3f9` | 2150 s | `static-analysis (tests)` 2113 s |
| 40259383390 | pull_request | `88fc715` | 2267 s | `static-analysis (tests)` 1292 s |

SC-002 asks for the median of six runs at 15 minutes or less. Six runs, three
master pushes and the three newest branch runs, sort to 1732, 2134, 2150, 2267,
2466 and 2697 s, so the median is 2208.5 s. That misses the bound, and it misses
because three of the six are master pushes whose own median is 2466 s: no branch
number can pull that median under 900 s. `results/ph11/tables.md` is therefore
read alongside the job-level median, which is the reading
`results/ph3/warm.md` uses for the same criterion.

The job-level median is the median of the per-job walls, which is what a
reviewer waits on to see a red job:

| Run | Jobs | Median job wall |
| --- | --- | --- |
| 40042928839 | 12 | 215 s |
| 39708015373 | 12 | 223 s |
| 39659845534 | 12 | 240 s |
| 40242340926 | 15 | 471 s |
| 40252253175 | 15 | 463 s |
| 40255220576 | 15 | 405 s |
| 40259383390 | 15 | 449 s |

Master's graph has twelve jobs and the branch's has fifteen, because
`static-analysis` is one id that expands to three shards.

Both readings are in the record because they answer different questions, and the
one that SC-002's words ask for is missed. The branch's median job wall rose
because the analyzer shards and the coverage lane became separate jobs that each
carry a build: master's `test` job did the same work inside one 2621 s wall.

## The per-job steps

The five heavy lanes, master from run 40042928839 (section B of `spec.md` reads
the same run) and the branch from run 40259383390:

| Job | Master `Build` | Master `Test` | Branch `Build` | Branch `Test` |
| --- | --- | --- | --- | --- |
| `test` | 2277 s | 315 s | 216 s | 217 s |
| `test-rocky` | 303 s | 320 s | 186 s | 187 s |
| `sanitize` | 285 s | 218 s | 479 s | 104 s |
| `coverage` | 829 s | 334 s | 770 s | 219 s |
| `tsan` | 251 s | 278 s | 212 s | 210 s |

The `test` row is the feature in one line: 2277 s of compiling inside the lane
that also runs the suite is now 216 s, and the analysis it used to do inside that
build stands in three shards beside it. `sanitize`'s `Build` rose because that
lane now builds the analyzer-free tree with the sanitizer flags at the runner's
core count over more targets; its `Test` fell from 218 s to 104 s. `coverage`'s
`Test` is the `-j 1` reading D-8 keeps, 219 s against the 334 s master held at
`-j 2` (`results/ph11/coverage-lane.md`).

The branch's job walls at the head, all fifteen:

| Job | Wall | Conclusion |
| --- | --- | --- |
| `lint` | 28 s | success |
| `dbc-gate` | 45 s | success |
| `prose-lint` | 64 s | success |
| `downstream-consumer` | 130 s | success |
| `consumer-release` | 231 s | success |
| `shared-audit` | 377 s | success |
| `test-rocky` | 449 s | success |
| `tsan` | 472 s | failure |
| `static-analysis (library)` | 559 s | success |
| `sanitize` | 619 s | success |
| `static-analysis (examples)` | 619 s | success |
| `test` | 677 s | success |
| `coverage` | 1108 s | failure |
| `static-analysis (tests)` | 1292 s | success |
| `docs` | 0 s | skipped |

Sum of job walls 6670 s of runner time against master's 6557 s at
40042928839: the work moved across jobs and lanes, and the runner time is
flat, which is what a graph that only re-homes work should look like.

## The install steps

`results/ph9/install-steps.md` holds the per-run sums and the per-job table for
the same runs: master 432 s, 1332 s and 370 s; the branch 378 s, 677 s and 552 s.
That record also carries the cache-gating finding, which explains why a
`pull_request` run restores and never writes.

## The audit point and the cold run

FR-052 asks for one run of the PR #47 head at the audit point with its slowest
job named. Run 40130784473 (`4f701ae`, `pull_request`) has a 20 s wall and `lint`
is its slowest job at 18 s: the run stops at the formatting gate, so it carries
no timing for anything else (the same shape the base head shows, PC-10).

The cold-cache run 40242954391 (`3726e10`, `push`) has a 1928 s wall with
`static-analysis (tests)` at 1892 s as its slowest job. Its cache statistics are
in `results/ph3/cold.md`; it stands beside, never inside, the warm median.

## The analyzer shards

`results/ph5` reads the shards against the base tree. The unit counts each shard
prints at its `Count translation units` step at `80de3f9`: library 122, examples
154, tests 213, which is 489 units against the 227 the base `test` job analyzed
inside its own build.

## The two reds at the head

Neither red is a wall reading, so they are named where a reader of this table
will look for them.

`tsan` failed one entry, `102 - harness_context_barrier_test (Failed)` in 1.07 s,
with `99% tests passed, 1 tests failed out of 135`, `quick = 163.10 sec*proc
(128 tests)` and `slow = 613.36 sec*proc (7 tests)` for a `Total Test time (real)`
of 209.51 s. Its check message is `the YIELD participant never sleeps inside the
barrier: its voluntary-switch counter stands flat across the round (D-03, Q-3,
FR-014)`. Master's `tsan` lane passed the same entry in 0.74 s at
`-j 2` (job 120718088244), and the branch's earlier `tsan` run failed
`102 - harness_context_barrier_test` on the SPIN participant (job 121346611402).
The entry declares `PROCESSORS 4` at `test/CMakeLists.txt:1171`, and a measured
check shows a `PROCESSORS 4` entry runs alone at `-j 2` and at `-j 4`
(12.04 s, 9.04 s and 6.03 s for one 3 s heavy entry and three 3 s light entries
at `-j 1`, `-j 2` and `-j 4`), so raising `-j` did not put this entry beside
another one.

The rerun of the failed lanes at the same commit passed that entry in 0.45 s:
`100% tests passed out of 135` with `Total Test time (real) = 312.60 s` at attempt
2 of run 40259383390 (job 121374177607). Two failures on two different
participants and one pass at one head make the entry intermittent, and the
attempt 2 log holds four entries in flight at its peak, so `-j "$(nproc)"`
resolves to four slots on the runner. The entry asks for four of them, so it
stands alone there exactly as it stood alone on master at `-j 2`.

`coverage` failed `Process coverage info`, not `Test`: that step ended `success`
with `100% tests passed out of 134`, `harness_jitter_cli_test (Skipped)` among
the three entries that did not run. lcov stopped the extract command with

```text
lcov: ERROR: (inconsistent) ".../source/harness/hunt_plan.cpp":365: function
'_ZZN2sg4hunt10rankEventsE..._clESF_' is hit but no contained lines are hit.
```

`cmake/coverage.cmake:61` gives the capture command
`--ignore-errors mismatch,negative,inconsistent`; `:83` to `:90` of that head
gave the extract command no ignore class, and FR-041 as written forbade adding
one there. The cause of the abort, the named-helper fix that closed it and
the absence of any base reading in CI are in
`results/ph11/coverage-lane.md`; the local trace reading is in
`results/ph11/trace-equality.md`.

The rerun of this lane at the same commit failed the same way, 1087 s, step 13
`Process coverage info` at exit 1 on
`hunt_plan.cpp:365: function '_ZZN2sg4hunt10rankEventsERKSt6vecto...'`, with
`Test` green at `100% tests passed out of 134` (attempt 2, job 121374177396).
The abort is therefore deterministic and the lane's own trace still reached the
artifact store: `Retain the coverage trace` ran with `if: always()` and uploaded
394154 bytes holding 54 `SF` records, 1387 `FNA`, 11937 `DA` and 6152 `BRDA`
records. In that file the closure records under `rankEvents` carry counts of
5476, 11093, 321169 and 1859, and no `DA` record stands between lines 360 and
370 of `source/harness/hunt_plan.cpp`, which is the pair lcov reads as an
inconsistent record.

## The rerun

Run 40259383390 was rerun with `gh run rerun 40259383390 --failed` so the same
commit answers both reds a second time. Its attempt 2 readings, and whether each
red repeated, are in `results/ph11/gates.md`. A rerun resets the run-level view,
so every number above names `attempts/1`.
