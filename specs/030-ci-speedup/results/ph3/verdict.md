# T019: cold against warm, job by job

**Surface**: every job of one forced-cold run and two warm runs of one commit,
with its `Build` step, its job wall, its hit rate and its test verdict.
**Head sha**: `3726e10` for all three runs, the feature head `e77b713` plus the
cache-key bump `3b6b1fa` and the branch trigger commit `3726e10`. The tree
outside `.github/workflows/ci.yml` is the branch's tree, so every lane compiled
the same sources.
**Run id**: run 40242954391 on branch `030-cold-cache-evidence`, `push` event:
attempt 1 is the forced-cold run, attempt 2 and attempt 3 are the warm pair. The
ccache-off control is run 40242956401 at `c6e5aa5`. Runs 40241027505 and
40242327268 were canceled and are absent from every table here.
**Command**: `gh run view 40242954391 --json jobs` and
`gh api …/runs/40242954391/attempts/N/jobs` for step and job seconds,
`gh run view --job <job id> --log` for the ctest result lines, and the reads
printed in `results/ph3/cold.md` and `results/ph3/warm.md`, which produced every
number below.
**Numbers**: 14 jobs compared cold against best warm, one verdict table over
three runs, and zero tests that pass in one run and fail in another.

## Job by job

The best-warm column is the quicker of the two warm attempts. The two warm
attempts differ inside a lane by runner speed, and `warm.md` carries both
readings of every row.

| Job | `Build` cold | `Build` best warm | delta | Job wall cold | Job wall best warm | delta |
| --- | --- | --- | --- | --- | --- | --- |
| `test` | 191 | 286 | +95 | 414 | 609 | +195 |
| `test-rocky` | 293 | 262 | -31 | 660 | 597 | -63 |
| `coverage` | 553 | 747 | +194 | 830 | 1004 | +174 |
| `sanitize` | 491 | 262 | -229 | 658 | 354 | -304 |
| `tsan` | 208 | 377 | +169 | 456 | 795 | +339 |
| `static-analysis (library)` | 420 | 474 | +54 | 458 | 616 | +158 |
| `static-analysis (tests)` | 1884 | 1329 | -555 | 1919 | 1391 | -528 |
| `static-analysis (examples)` | 1006 | 889 | -117 | 1044 | 935 | -109 |
| `shared-audit` | 132 | 47 | -85 | 182 | 81 | -101 |
| `consumer-release` | 201 | 94 | -107 | 250 | 139 | -111 |
| `downstream-consumer` | 1 | 1 | 0 | 137 | 102 | -35 |
| `dbc-gate` | 16 | 14 | -2 | 51 | 59 | +8 |
| `lint` | no `Build` step | no `Build` step | — | 27 | 29 | +2 |
| `prose-lint` | not read | 3 | — | 54 | 48 | -6 |
| `docs` | skipped | skipped | — | — | — | — |

Two groups read cleanly out of that table.

The six lanes that save get cheaper, and by the amount the cache predicts.
`shared-audit` gives up 85 s of `Build` and 101 s of wall, `consumer-release`
107 s and 111 s, and `static-analysis (tests)` 555 s of `Build` in the quicker
warm attempt. `library` and `examples` move the other way inside the same 100%
hit rate, by 54 s and 117 s against 420 s and 1006 s of cold cost: the shard
step is clang-tidy time, which the cache cannot shorten, so what remains is
runner speed.

The five suite lanes move in both directions with no cache engaged at all:
0.00% hits in the cold run and in both warm runs, because a red suite skips
`Save ccache`. Their deltas, +95, -31, +194, -229 and +169 seconds of `Build`,
are therefore the spread of a full compile across three runners, and they say
nothing about the cache. The cold run of this tree is the fastest of the three
for `test`, `coverage` and `tsan`.

## The verdicts

| Job | cold, attempt 1 | warm, attempt 2 | warm, attempt 3 | Agreement |
| --- | --- | --- | --- | --- |
| `lint`, `prose-lint` | success | success | success | yes |
| `static-analysis (library)`, `static-analysis (tests)`, `static-analysis (examples)` | success | success | success | yes |
| `shared-audit`, `consumer-release`, `downstream-consumer`, `dbc-gate` | success | success | success | yes |
| `test`, `test-rocky`, `coverage`, `sanitize` | failure: `harness_jitter_cli_test` | same | same | yes |
| `tsan` | failure: `harness_jitter_cli_test`, `harness_context_barrier_test` | same | same | yes |
| `docs` | skipped | skipped | skipped | yes |

No test passes in one run and fails in another. The two reds are
`harness_jitter_cli_test`, red in every suite lane of every run, and
`harness_context_barrier_test`, red in `tsan` of every run. Both are the base
reds that the pull request body names, and the ccache-off control run
40242956401 carries the same two in the same lanes. Each lane reported the same
entry count in all three runs, 139 for `test` and `test-rocky`, 134 for
`coverage` and `sanitize`, 135 for `tsan`, with two skipped entries in each. The
list difference between master and this branch is a separate question, answered
entry by entry in `results/ph5/finding-diff.md`.

## Verdict

FR-010 and SC-006 hold. One cold run and two warm runs of `3726e10` give an
equal verdict in every one of the 14 jobs that ran, and `sanitize` and `tsan`
each ran three times over the whole runner, where `taskset` appears only in
`tools/ci/prepush.sh:196`. The cache explains the improvement in the six lanes
that save and explains nothing in the five that cannot save, and the wide
`Build` spread in those five is runner speed across three machines rather than a
cache effect. The one change that would let the five save anyway, `always()` on
`Save ccache`, is recorded as a follow-up under `results/ph3/warm.md`.
