# T018: two warm runs of one commit, and the same commit with ccache off

**Surface**: the `Build` step and the job wall of every job of two warm runs,
each job's `ccache -s` hit rate, the same readings for one run with
`CCACHE_DISABLE=1`, and the per-test verdict of the five suite lanes in all
four runs.
**Head sha**: `3726e10` for the warm pair. That commit is the feature head
`e77b713` plus `3b6b1fa CI: Bump the cache keys for one cold run` plus
`3726e10 CI: Let this branch schedule its run`, so its tree is the branch's
tree. The ccache-off run sits at `c6e5aa5`, which is `e77b713` plus
`11a25ba CI: Disable ccache for one run` plus `c6e5aa5 CI: Let this branch
schedule its run`. `git diff --name-only e77b713 c6e5aa5` names
`.github/workflows/ci.yml` and nothing else, and the same read at `3726e10`
names that file alone too, so all four runs compiled the same C++ tree and the
difference between them is the workflow text.
**Run id**: run 40242954391, `push` event on branch `030-cold-cache-evidence`,
attempt 2 and attempt 3. Both are runs of `3726e10` that start from the buckets
attempt 1 left. Run 40242956401, `push` event on branch `030-nocache-evidence`
at `c6e5aa5`, is the `CCACHE_DISABLE=1` run. Attempt 1 of run 40242954391 is
the forced-cold run recorded in `results/ph3/cold.md`. Runs 40241027505 and
40242327268 were canceled, so no number below comes from either one.
**Command**: `gh run view 40242954391 --json jobs` for the step seconds and job
walls of the newest attempt, `gh run view 40242954391 --attempt 2 --log`
measured between the first and the last line of each `Build` step for the older
attempt, `gh api …/runs/40242954391/attempts/N/jobs` for the walls of attempts 1
and 2, and `gh run view --job <job id> --log` for the `ccache -s` block and the
ctest result lines. Each is printed under Commands.
**Numbers**: 14 jobs ran in each of the four runs, 13 of them own a `Build`
step, and the verdict of every job is the same in all four.

## Why the pair is warm

A step behind a failed step is skipped, and `Save ccache` carries
`if: github.event_name == 'push'`. The five lanes whose suite went red saved
nothing; the three analyzer shards, `shared-audit`, `consumer-release` and
`downstream-consumer` finished green and saved. Attempts 2 and 3 therefore
start from a bucket that holds those six lanes and nothing else, which makes
the warm pair measure two situations at once: a lane that saved and a lane that
never could.

## Build seconds and hit rate, warm twice

| Job | `Build` attempt 2 | Hits attempt 2 | `Build` attempt 3 | Hits attempt 3 | Job verdict |
| --- | --- | --- | --- | --- | --- |
| `test` | 286 | 0 / 265 (0.00%) | 286 | 0 / 265 (0.00%) | failure |
| `test-rocky` | 304 | 0 / 265 (0.00%) | 262 | 0 / 265 (0.00%) | failure |
| `coverage` | 754 | 0 / 265 (0.00%) | 747 | 0 / 265 (0.00%) | failure |
| `sanitize` | 262 | 0 / 265 (0.00%) | 466 | 0 / 265 (0.00%) | failure |
| `tsan` | 394 | 0 / 261 (0.00%) | 377 | 0 / 261 (0.00%) | failure |
| `static-analysis (library)` | 630 | 142 / 142 (100.0%) | 474 | 142 / 142 (100.0%) | success |
| `static-analysis (tests)` | 1329 | 233 / 233 (100.0%) | 1841 | 233 / 233 (100.0%) | success |
| `static-analysis (examples)` | 889 | 174 / 174 (100.0%) | 936 | 174 / 174 (100.0%) | success |
| `shared-audit` | 49 | 142 / 142 (100.0%) | 47 | 142 / 142 (100.0%) | success |
| `consumer-release` | 60 then 37 | 142 / 142 (100.0%) | 48 then 46 | 142 / 142 (100.0%) | success |
| `downstream-consumer` | 1 | 142 / 142 (100.0%) | 1 | 142 / 142 (100.0%) | success |
| `dbc-gate` | 26 | no statistics step | 14 | no statistics step | success |
| `prose-lint` | 3 | no statistics step | 4 | no statistics step | success |

`lint` owns no `Build` step and `docs` did not run: it waits for a push to
master.

The five suite lanes ask ccache about every compile and are answered no in both
warm attempts, exactly as the cold attempt was answered no. Their `Build`
seconds are three independent full compiles of one tree: 191, 286 and 286 for
`test`, 491, 262 and 466 for `sanitize`. The spread inside one commit and one
preset is runner speed, and it is wide enough that no single reading of a suite
lane should be quoted alone.

The six lanes that saved answer every question with a hit, and only two of them
get faster. `shared-audit` falls from 132 s cold to 49 and 47, which is the
cache doing its job on a lane whose `Build` step is compilation. The analyzer
shards keep their cold cost: 420, 630 and 474 for `library`, 1884, 1329 and
1841 for `tests`, 1006, 889 and 936 for `examples`, at 100.0% hits.
`results/ph5/cache-ratio.md` settled why: one ccache launch serves one compile,
the analyzer runs sit outside ccache, and a shard's `Build` step is mostly
clang-tidy time, which no compiler cache shortens.

## The same tree with ccache off

Run 40242956401 sets `env: CCACHE_DISABLE: "1"` for the whole workflow and
restores the un-bumped `CCACHE_KEY_PREFIX` values, so its lanes ask for the
production bucket names and never reach a cache.

| Job | `Build` off | `Build` best warm | Job wall off | Job wall best warm |
| --- | --- | --- | --- | --- |
| `test` | 238 | 286 | 495 | 609 |
| `test-rocky` | 267 | 262 | 640 | 597 |
| `coverage` | 748 | 747 | 1007 | 1004 |
| `sanitize` | 458 | 262 | 598 | 354 |
| `tsan` | 354 | 377 | 768 | 795 |
| `static-analysis (library)` | 382 | 474 | 416 | 616 |
| `static-analysis (tests)` | 2037 | 1329 | 2071 | 1391 |
| `static-analysis (examples)` | 580 | 889 | 616 | 935 |
| `shared-audit` | 126 | 47 | 171 | 81 |
| `consumer-release` | 122 then 85 | 48 then 46 | 242 | 139 |
| `downstream-consumer` | 2 | 1 | 159 | 102 |
| `dbc-gate` | 18 | 14 | 53 | 59 |
| `prose-lint` | 2 | 3 | 39 | 48 |

The best-warm column takes the quicker of the two warm attempts, because the
spread inside the pair is runner speed. Two readings fall out of the table. The
suite lanes and the analyzer shards cost with the switch off what they cost in a
warm run at 0.00% hits, so those seconds are compile time that no cache touched
in any of the four runs. `shared-audit` and `consumer-release` are the opposite
pair: 126 s and 122 plus 85 s with no cache against 47 s and 48 plus 46 s with a
full bucket, and 171 s against 81 s and 242 s against 139 s of job wall. Those
are the measured size of the cache on a lane that saves.

## Three `sanitize` and three `tsan` readings at full affinity

Neither lane pins the process to a core. `taskset` appears in
`tools/ci/prepush.sh:196`, where the quick tier asks for two cpus, and nowhere
in `.github/workflows/ci.yml`. Every run below ran
`ctest --output-on-failure --no-tests=error -C Release -j "$(nproc)"` over the
whole runner.

| Lane | run | `Build` (s) | Job wall (s) | ctest result | Verdict |
| --- | --- | --- | --- | --- | --- |
| `sanitize` | 40242954391 attempt 1, cold | 491 | 658 | 132 passed, 1 failed of 134 | failure |
| `sanitize` | 40242954391 attempt 2, warm | 262 | 354 | 132 passed, 1 failed of 134 | failure |
| `sanitize` | 40242954391 attempt 3, warm | 466 | 730 | 132 passed, 1 failed of 134 | failure |
| `sanitize` | 40242956401, ccache off | 458 | 598 | 132 passed, 1 failed of 134 | failure |
| `tsan` | 40242954391 attempt 1, cold | 208 | 456 | 133 passed, 2 failed of 135 | failure |
| `tsan` | 40242954391 attempt 2, warm | 394 | 835 | 133 passed, 2 failed of 135 | failure |
| `tsan` | 40242954391 attempt 3, warm | 377 | 795 | 133 passed, 2 failed of 135 | failure |
| `tsan` | 40242956401, ccache off | 354 | 768 | 133 passed, 2 failed of 135 | failure |

Three readings of one commit at the runner's full affinity is what SC-006 asks
of each lane, and the three attempts of run 40242954391 are those readings. The
fourth row of each block is the ccache-off run, kept because FR-010 asks for the
off case beside the cold and warm ones.

## The verdict of every job, four times over

The per-test result lines came out of each lane's own log with the pattern
`Test #N: <name> ... <status>`, which is how a failure is told apart from the
two skipped entries that every lane reports.

| Job | cold | warm 1 | warm 2 | ccache off |
| --- | --- | --- | --- | --- |
| `test` | `harness_jitter_cli_test` | same | same | same |
| `test-rocky` | `harness_jitter_cli_test` | same | same | same |
| `coverage` | `harness_jitter_cli_test` | same | same | same |
| `sanitize` | `harness_jitter_cli_test` | same | same | same |
| `tsan` | `harness_jitter_cli_test` and `harness_context_barrier_test` | same | same | same |
| `static-analysis (library)`, `static-analysis (tests)`, `static-analysis (examples)`, `shared-audit`, `consumer-release`, `downstream-consumer`, `dbc-gate`, `lint`, `prose-lint` | success | success | success | success |
| `docs` | skipped | skipped | skipped | skipped |

No entry passes in one run and fails in another. `harness_jitter_cli_test` is
red in every suite lane of all four runs, `harness_context_barrier_test` is red
in `tsan` of all four, and the entry counts are 139, 139, 134, 134 and 135 with
two skips in each. FR-010 and SC-006 hold on this evidence: cold, warm twice and
ccache off agree in every job.

The checkpoint of the phase is met on the same numbers. The warm `test` job
`Build` step is 286 s against the base 2277 s that the checkpoint of
`specs/030-ci-speedup/tasks.md` names for it, and its job wall is 609 s, inside
the 15 min median SC-002 sets for the lane.

## What is left as risk

`Save ccache` sits behind the suite step with `if: github.event_name == 'push'`.
A red suite therefore costs five buckets: the run compiles everything and stores
nothing, and the next run of the same commit pays the full compile again, which
is what the 0.00% rows above are. `if: always() && github.event_name == 'push'`
is the one-line change that lets a red suite still save. No task in
`specs/030-ci-speedup/tasks.md` asks for it, so it stays out of this change and
is recorded here as the follow-up it is.

## Commands

```bash
gh run view 40242954391 --json jobs \
  | jq -r '.jobs[] | .name as $n | (.steps[]?
      | select(.name | startswith("Build"))
      | select(.completedAt != null)
      | [$n, .name, (((.completedAt | fromdateiso8601)
                    - (.startedAt | fromdateiso8601)) | tostring)] | join(" | ")'
gh run view 40242954391 --attempt 2 --log > /tmp/a2.log
gh api repos/<owner>/speedgun-ng/actions/runs/40242954391/attempts/1/jobs \
  --paginate | jq -r '.jobs[] | [.name,
      (((.completed_at | fromdateiso8601) - (.started_at | fromdateiso8601))
      | tostring)] | join("=")'
gh run view --job 121327902981 --log | sed -E 's/^[^Z]*Z //'
git diff --name-only e77b713 c6e5aa5
```

The aggregate log of attempt 3 could not be downloaded: `gh` answered
`failed to get run log: stream error: stream ID 1; CANCEL; received from peer`,
so the attempt 3 seconds come from the step timestamps of the first command and
the attempt 2 seconds from the log of the second. A step timestamp counts whole
seconds while a log span counts the milliseconds between the first and last line
of the step, so two columns can differ by a second.

## VERDICT

Fourteen jobs ran in each of the four runs, thirteen of them own a `Build` step,
and the verdict of every job is the same in the cold run, in both warm runs and
in the ccache-off run. The cache engages in full in the six lanes that save,
cuts `shared-audit` from 132 s to 47 s and `consumer-release` from 201 s to 94 s
of `Build`, and reaches nothing else: five lanes never saved, and the analyzer
shards spend their `Build` step outside ccache.
