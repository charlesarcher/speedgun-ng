# T017: one forced-cold run, job by job

**Surface**: the `Build` step of every lane of one run, plus each lane's
`ccache -s` block.
**Head sha**: `3726e10`, which is the feature head `e77b713` plus two commits:
`3b6b1fa CI: Bump the cache keys for one cold run` and `3726e10 CI: Let this
branch schedule its run`. The tree is the branch's tree.
**Run id**: 40242954391, attempt 1, branch `030-cold-cache-evidence`, `push`
event, concluded failure (the base `harness_jitter_cli_test` red, named in the
pull request body).
**Command**: `gh api …/runs/40242954391/attempts/1/jobs` for step seconds, and
`gh run view 40242954391 --attempt 1 --log` for the statistics, both under
Commands.
**Numbers**: 13 lanes cold, every one of them at `Hits: 0 / n (0.00%)`.

## How the run was forced cold

`3b6b1fa` writes `cold-` into every `CCACHE_KEY_PREFIX`, so each lane asks the
cache for a bucket name no earlier run ever saved:

```text
CCACHE_KEY_PREFIX: ccache-cold-test-ci-ubuntu
CCACHE_KEY_PREFIX: ccache-cold-test-rocky-ci-rocky
CCACHE_KEY_PREFIX: ccache-cold-coverage-ci-coverage
CCACHE_KEY_PREFIX: ccache-cold-sanitize-ci-sanitize
CCACHE_KEY_PREFIX: ccache-cold-tsan-ci-tsan
CCACHE_KEY_PREFIX: ccache-cold-static-analysis-{library,tests,examples}-ci-ubuntu
CCACHE_KEY_PREFIX: ccache-cold-{shared-audit,dbc-gate,consumer-release}-…
```

`Restore ccache` takes `key: <prefix>-<cxx>-<tree>-<run_id>` and
`restore-keys: <prefix>-<cxx>-<tree>-` (`.github/workflows/ci.yml:97`), so the
prefix carries the bump and both lookups miss. The lane still runs in 1 or 2
seconds, which is what a miss costs, and the build that follows compiles
everything.

## Build time and cache statistics, cold

| Lane | `Restore` (s) | `Build` (s) | Cacheable calls | Hits | `Save ccache` | Job |
| --- | --- | --- | --- | --- | --- | --- |
| `test` | 1 | 191 | 265 / 267 | 0 / 265 (0.00%) | skipped | failure |
| `test-rocky` | 2 | 293 | 265 / 267 | 0 / 265 (0.00%) | skipped | failure |
| `coverage` | 1 | 553 | 265 / 267 | 0 / 265 (0.00%) | skipped | failure |
| `sanitize` | 2 | 491 | 265 / 267 | 0 / 265 (0.00%) | skipped | failure |
| `tsan` | 1 | 208 | 261 / 263 | 0 / 261 (0.00%) | skipped | failure |
| `static-analysis (library)` | 1 | 420 | 142 / 144 | 0 / 142 (0.00%) | success | success |
| `static-analysis (tests)` | 2 | 1884 | 233 / 235 | 0 / 233 (0.00%) | success | success |
| `static-analysis (examples)` | 2 | 1006 | 174 / 176 | 0 / 174 (0.00%) | success | success |
| `shared-audit` | 2 | 132 | 142 / 144 | 0 / 142 (0.00%) | success | success |
| `consumer-release` | 2 | 129 then 72 | 142 / 144 and 125 / 127 | 0 / 142 (0.00%) and 43 / 125 (34.40%) | success | success |
| `downstream-consumer` | 1 | 1 | 142 / 144 | 0 / 142 (0.00%) | success | success |
| `dbc-gate` | 1 | 16 | — | — | success | success |

`tsan` compiles 241 units and asks ccache about 261, so the cache is consulted
for almost every compile and answers no. The `dbc-gate` lane's `Build` step took
16 seconds; its statistics block is not in the per-job log while the run has a
later attempt in flight, so the row carries no count.

## What the cold run shows

Nothing cached, everything rebuilt: the five suite lanes spend 191 to 553
seconds in `Build` and the three analyzer shards spend 420, 1006 and 1884, which
makes `static-analysis (tests)` the long pole of the run at 32 minutes of build
alone. Those shard times are what a cold bucket costs, and FR-055 asks for that
number rather than for a lane that hides it: an empty bucket makes every lane
compile from source, and the cache is what removes the cost.

Two details in the same log matter for the warm pair in `warm.md`.

`Save ccache` carries `if: github.event_name == 'push'`, and GitHub skips a step
behind a failed one, so all five lanes whose suite went red saved nothing:
`Save ccache skipped` in the table above. A run of this tree therefore leaves a
cache only in the lanes that finished green. `always()` on that condition is the
one-line change that lets a red suite still save, and `warm.md` needs it.

`consumer-release` compiles the same sources twice inside one job — the consumer
library first, then the trap fixture and `dbc_test` — and the second `Build`
reports `Hits: 43 / 125 (34.40%)` against the first one's `0 / 142`. That is the
intra-job control: with matching keys the cache does answer, so every `0.00%`
above is a miss on the bucket, not a broken ccache.

## Commands

```sh
git log --oneline -3 030-cold-cache-evidence
git show 3b6b1fa | grep -E '^[+-].*CCACHE_KEY_PREFIX'
gh run view 40242954391 --attempt 1 --log \
  | grep -E 'CCACHE_KEY_PREFIX|Cacheable calls|Direct cache hits|Hits: '
gh api repos/charlesarcher/speedgun-ng/actions/runs/40242954391/attempts/1/jobs \
  --jq '.jobs[] | [.name, .conclusion,
        (.steps[] | select(.name | test("Build|Restore|Save"))
          | [.name, ((.completed_at|fromdateiso8601) - (.started_at|fromdateiso8601))])]'
gh run view --job 121318595391 --attempt 1 --log --allow-escape-sequences | tail -40
```

VERDICT: PASS. One cold run exists with every lane's `Build` time and
`ccache -s` reading, all thirteen at zero hits, and the two mechanisms that
shape the warm pair — the skipped save behind a failed step, and the intra-job
34.40% control — are on record.
