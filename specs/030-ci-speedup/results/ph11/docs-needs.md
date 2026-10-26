# Phase 11 The Docs Deploy Waits For The Analyzers

**Surface**: the `needs` list of the `docs` job in `.github/workflows/ci.yml`,
which gained `static-analysis` and kept the three entries master had, and the
`if:` condition at `:1416` that the branch left byte-identical to master's `:774`
(T056, T026, FR-038, D-12).
**Head sha**: `fea69c5` for the base line, `42bbb8d` for the change, `88fc715`
for the head the change stands at.
**Run id**: 40255220576 and 40259173012 for the observations, 40042928839 for the
master shape.
**Command**: `git show origin/master:.github/workflows/ci.yml` and the same read
of the head, `git log -S` for the commit that moved the line, and `gh run view
<run> --json jobs` for the conclusion each `docs` job reached.
**Numbers**: three needs on master, four on the branch, four shard lanes under
the one `static-analysis` id, `docs` reached in neither of the two branch runs.

## The line

```text
$ git show origin/master:.github/workflows/ci.yml | grep -n 'docs:' -A 3
759:  docs:
760-    # Deploy docs only when builds succeed
761-    needs: [sanitize, test, test-rocky]
$ grep -n 'docs:' -A 3 .github/workflows/ci.yml
1399:  docs:
1400-    # Deploy docs only when builds succeed
1401-    needs: [sanitize, static-analysis, test, test-rocky]
```

`42bbb8d CI: Make the docs deploy wait for the analyzers` is the commit that
added the one entry, found with `git log -S 'needs: [sanitize, static-analysis,
test, test-rocky]'`. The three master entries stand in the same order, so the
change adds an edge and moves none (FR-038).

## Why one id covers four lanes

`static-analysis` is a matrix job: one id, three shards, `library`, `tests` and
`examples`, printed as `static-analysis (library)` and its siblings in every log.
GitHub resolves a `needs` entry that names a matrix job against every expanded
lane of it, so `docs` waits on all three shards with the one entry, and the shard
count can change without touching `docs` (D-12). The unit counts of the three
shards at `80de3f9` are 122, 154 and 213 translation units, from each shard's
`Count translation units` step.

## What the runs show, and what they cannot show

Every branch run named here is a `pull_request` event of `refs/pull/51/merge`, and
the `docs` job carries a condition the branch never touched:

```text
$ grep -n "github.ref == 'refs/heads/master'" .github/workflows/ci.yml
1416:    if: github.ref == 'refs/heads/master'
      && github.event_name == 'push'
      && github.repository_owner == 'charlesarcher'
```

That condition is false on a pull request, so `docs` ends `skipped` on every run
of this branch whatever its `needs` do. Reading the skip as the waiting edge at
work would be a mistake: in 40255220576 the three analyzer lanes ended `success`
while `test`, `test-rocky` and `sanitize` ended `failure` on the entry
`results/ph11/jitter-skip.md` handles, and `docs` was already held by the
condition; in 40259173012 the analyzers were canceled part way and `docs` ended
`cancelled` having started nothing.

What carries the evidence, then, is the line plus the platform rule plus the one
run that can reach the job. The line adds the fourth entry to the same AND and
moves none of the three master had. GitHub resolves a `needs` entry naming a
matrix job against every expanded lane of it, which is why one entry waits on
three shards. Master's 40042928839 is a `push` to `master`, its `if:` holds, and
`docs` reached `success` there in 38 s with its three needs green: the shape the
branch extends by one edge.

The edge is what T056 asked for: a documentation deploy that a red analyzer would
once have passed unnoticed is now held by the same rule that holds it for a red
suite. FR-038 is satisfied by the line. The observable reading of it belongs to
the first `push` to `master` after the merge, because `docs` runs nowhere else.
