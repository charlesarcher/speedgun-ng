# Phase 10 The Superseded Run Gives Up Its Slot

**Surface**: the top-level `concurrency` key of `.github/workflows/ci.yml:3` to
`:5`, the two pushes to the `030-ci-speedup` ref that exercised it, and the
master runs that stand outside it (T053, FR-039, SC-002, story 8).
**Head sha**: `324f4fd` for the canceled run, `88fc715` for the run that
replaced it, `fea69c5` for the master run.
**Run id**: 40259173012 canceled, 40259383390 completed (two red lanes, both
named in `results/ph11/tables.md`), 40042928839 stands
outside the rule.
**Command**: `gh run list --branch 030-ci-speedup` for the push pair,
`gh run view <run> --json jobs` for the job conclusions, `gh api` on
`repos/charlesarcher/speedgun-ng/actions/runs/<run>` for the two stamps, and
`gh run list --branch master` for the master count. Each listing asks for
`databaseId,headSha,status,conclusion,createdAt`.
**Numbers**: 205 s of life for the canceled run, 11 of its 15 jobs canceled mid
flight, 3 jobs already finished, 0 canceled runs among the last 25 master runs.

## The rule as it stands

```yaml
concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: ${{ github.event_name == 'pull_request' }}
```

One group per workflow and ref, so a second push to the same ref finds the first
run still in the group. The cancel is switched on for the `pull_request` event
and off for everything else, which leaves a `push` run of master to finish: the
measurement of the default branch is never the one that gets thrown away.

## The pair on one ref

Both runs are `pull_request` events of `refs/pull/51/merge`, sixteen pushes apart
in wall terms and one history rewrite apart in commit terms: the first carried
the two fix commits, the second carried those two commits with compliant commit
messages plus the record of the skip.

| Run | Head | Created | Concluded |
| --- | --- | --- | --- |
| 40259173012 | `324f4fd` | 2026-10-26T02:27:19Z | `cancelled` |
| 40259383390 | `88fc715` | 2026-10-26T02:30:00Z | the run every reading below comes from |

The canceled run started at 02:27:19Z and its last step moved at 02:30:44Z, so it
held a runner for 205 s before the cancel reached it. Its job list shows what the
cancel took:

```text
lint completed success
dbc-gate completed success
shared-audit completed success
prose-lint completed failure
coverage, sanitize, tsan, test, test-rocky, downstream-consumer,
static-analysis (library), static-analysis (tests), static-analysis (examples),
docs   completed cancelled
```

Eleven of fifteen jobs stopped where they stood, including the four suite lanes
that had a runner. Three had already finished on their own, and `prose-lint` had
failed on its own: it read the two commit messages that carried a 74-column body
line and a section token the rules file does not list, which is the finding the
rewritten head fixed. The canceled run reached no measurement that anything uses.

## Master stands to its end

No master run cancels, because the cancel is gated to the `pull_request` event.
Among the last 25 `push` runs of `master` the count of `cancelled` conclusions is
zero. The one named here ran the whole graph:

```text
$ gh api repos/charlesarcher/speedgun-ng/actions/runs/40042928839 \
    --jq '"\(.run_started_at) \(.updated_at)"'
2026-10-24T00:02:20Z 2026-10-24T00:47:17Z
```

That is 2757 s from first start to last step at `fea69c5`, twelve jobs, `success`.
It is also the newest master run that reaches a `ctest` step, which is why
`results/ph11/jitter-skip.md` has to reach further back for a base reading.

## Nothing canceled enters a table

`results/ph11/tables.md` takes its branch column from 40242340926, 40252253175,
40255220576 and 40259383390, all `completed`. The three canceled runs of the
branch, 40241027505 at `37e068d`, 40242327268 at `43dd811` and 40259173012 at
`324f4fd`, are excluded from every median, every wall and every step table, and
they appear in this record alone. FR-052's six runs are three completed master
pushes (`40042928839`, `39708015373`, `39659845534`) and three completed pull
request pushes of the branch, so no canceled run supplies a number anywhere in
the pull request.
