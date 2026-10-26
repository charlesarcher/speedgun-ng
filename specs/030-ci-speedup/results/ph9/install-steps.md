# Phase 9 Install-Step Seconds, Run by Run

**Surface**: the package-install steps of `.github/workflows/ci.yml`, the
`actions/cache` steps that feed them, and the six runs FR-052 counts (T051,
T049, T050, FR-035, FR-036, FR-037, SC-002).
**Head sha**: `fea69c5`, `bd10d76` and `9976768` for the three master pushes;
`e77b713`, `5ccd69a` and `80de3f9` for the three pull-request pushes.
**Run id**: 40042928839, 39708015373, 39659845534, 40242340926, 40252253175 and
40255220576.
**Command**: `gh api
repos/charlesarcher/speedgun-ng/actions/runs/<run>/jobs --paginate` through
`/tmp/instasum.js`, which sums each job's steps whose name matches `Install`,
`toolchain` or `bootstrap`, and `gh run view --log --job=<job>` for the
`actions/cache` lines quoted under What the cache lines say.
**Numbers**: install sums 432 s, 1332 s and 370 s on master against 378 s, 677 s
and 552 s on the branch; one lane's dnf step alone spans 45 s, 991 s and 67 s
across three master runs; every restore step in every run reads 0 or 1 s.

## The six runs, job by job

Package-install steps only. `Install` with no argument builds the install tree
and `Install library` builds the consumer; neither downloads a package, so
neither is counted. `Prepare`, `Restore` and `Post Restore` of the cache steps
are counted nowhere here and are treated under Why a pull-request run cannot be
warm. `docs` is missing from the branch columns because it waits on four jobs
the run never completed (`results/ph10/cancel.md` and
`results/ph11/docs-needs.md`).

| Job | 40042928839 | 39708015373 | 39659845534 | 40242340926 | 40252253175 | 40255220576 |
| --- | --- | --- | --- | --- | --- | --- |
| `lint` | 4 | 3 | 1 | 1 | 2 | 1 |
| `test` | 12 | 16 | 16 | 19 | 172 | 134 |
| `test-rocky` | 45 | 991 | 67 | 58 | 196 | 34 |
| `coverage` | 34 | 13 | 15 | 15 | 12 | 16 |
| `sanitize` | 128 | 16 | 11 | 12 | 12 | 14 |
| `tsan` | 13 | 26 | 15 | 11 | 12 | 12 |
| `static-analysis (library)` | n/a | n/a | n/a | 17 | 24 | 11 |
| `static-analysis (tests)` | n/a | n/a | n/a | 11 | 11 | 20 |
| `static-analysis (examples)` | n/a | n/a | n/a | 13 | 12 | 11 |
| `consumer-release` | 17 | 11 | 17 | 13 | 14 | 139 |
| `downstream-consumer` | 135 | 162 | 163 | 170 | 168 | 110 |
| `shared-audit` | 12 | 16 | 16 | 15 | 13 | 12 |
| `dbc-gate` | 12 | 14 | 16 | 11 | 15 | 21 |
| `prose-lint` | 11 | 17 | 14 | 12 | 14 | 17 |
| `docs` | 9 | 47 | 19 | skipped | skipped | skipped |
| **sum** | **432** | **1332** | **370** | **378** | **677** | **552** |

## What the spread says

The same package list on the same image costs ten times more in one run than in
another. `test-rocky`'s dnf work reads 45 s, 991 s and 67 s across three master
pushes of the same workflow, and `consumer-release` reads 13 s then 14 s then
139 s across three branch pushes. `downstream-consumer`, the largest package set
on either side, sits in a narrow band of 110 to 170 s the whole time.

Two consequences for FR-052's table. A single run's install sum measures the
mirror, and says nothing about the workflow: 1332 s and 370 s are the same twelve install steps
under different network conditions. And the branch's install work is not what
the phase changed: the branch's sums (378 s, 677 s, 552 s) sit inside the master
range (370 s, 1332 s, 432 s), so the wall time the feature bought came from the
`Build` and `Test` steps that `results/ph11/tables.md` tabulates.

## Why a pull-request run cannot be warm

`Restore apt package cache` and `Restore ccache` run on every event. The matching
writes are gated:

```yaml
    - name: Save ccache
      if: github.event_name == 'push'
      uses: actions/cache/save@v4
```

That line is `.github/workflows/ci.yml:141`, and the same condition stands on
all eleven save steps of the workflow. `on.push.branches` names `master`, so a
push to `030-ci-speedup` produces only a `pull_request` run, and a
`pull_request` run never writes. Every restore of every branch run therefore
reads a miss, and the log prints it:

```text
$ gh run view --log --job=121346611499 | grep -a 'input keys'
Cache not found for input keys: apt-test-ubuntu-26.04-5eb3e7b8574589450e878a7dfafc0d0360eb21ae6f0c8b603d51d2c92afed9a8, apt-test-ubuntu-26.04-
Cache not found for input keys: ccache-test-ci-ubuntu-5498f26b0340c839dd0e4bee03ec3b2f020e6bbfe884e5d95c9cad1364f35888-eda120b6654407e5374ceecd80e0fc5a251dd170325d5723223
```

Run 40255220576, two hours later on the same ref, prints the same two shapes for
the same lane, and its rocky job prints the miss for the dnf bucket too:

```text
$ gh run view --log --job=121355646962 | grep -a 'input keys'
Cache not found for input keys: apt-test-ubuntu-26.04-2d2c6bb512bc012fde58090aac3b79769545d273fc6d3e84ac28ef7f093ae7ab, apt-test-ubuntu-26.04-
Cache not found for input keys: ccache-test-ci-ubuntu-5498f26b0340c839dd0e4bee03ec3b2f020e6bbfe884e5d95c9cad1364f35888-eda120b6654407e5374ceecd80e0fc5a251dd170325d5723223
$ gh run view --log --job=121355647032 | grep -aE 'key:|input keys'
  key: dnf-rockylinux10-14743b10cb3e6167
Cache not found for input keys: dnf-rockylinux10-14743b10cb3e6167, dnf-rockylinux10-
```

An earlier draft of this record read the rocky lane's 196 s to 34 s as the dnf
cache arriving. The miss line above is the correction: the bucket was never
restored in either run, so 162 s of that difference is the mirror. Nothing in
this record credits a cache for a step time.

The `dbc-gate` job of 40255220576 shows the same gate from the other side. It
ended `success`, and its step list still ends with `Save ccache completed
skipped`: a green job on a `pull_request` event writes nothing either.

## Where the warm reading lives

The cold-and-warm pair FR-052 and SC-003 ask for cannot come from a
`pull_request` run of this ref, so it comes from the `push` runs the phase made
for it: `results/ph3/cold.md` holds the forced-cold run 40242954391, where every
one of thirteen lanes reports `Hits: 0 / n (0.00%)`, and `results/ph3/warm.md`
holds the warm pair of the same tree, where the lane that prints its counters
reports 43 hits of 125 cacheable calls. `results/ph3/cache-stats.md` carries the
step-shape audit behind both. This record keeps the package side, and no number
of either kind is summed into the other.
