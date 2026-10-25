# T029: master's finding list beside the branch's three shards

**Surface**: the `analyzer-findings-<scope>` artifacts of the branch run and of
the merge-base baseline run, three scopes each.
**Head sha**: `e77b713` for the branch; `5814110` for the baseline, which is
`f507b8f` (the merge base with `origin/master`) plus one commit that adds an
isolated workflow.
**Run id**: 40242340926 (branch, `pull_request`), 40242387830 (baseline,
`push`, concluded success).
**Command**: `gh run download <run> -D <dir>`, then `sha256sum`, `sort -u` and
`comm` over the three pairs, each block under Commands.
**Numbers**: the two tables below, 14338 identities per side.

## Where master's list came from

`master` has no `static-analysis` job, so its list has to be produced. The last
master run whose tree built is 40042928839 at `fea69c5`; its `test` job log
(job 120718088140, 91804 lines, 227 `Building CXX object`) yields 8191
identities for the whole tree, and `example/docs/hunt_rank` appears in it zero
times, because that tree predates the merge base. Every master run at or after
`f507b8f` stops at `lint`, the base red this branch cleared, so no master run
carries an analyzer list at the base.

The list therefore comes from a throwaway branch: `030-baseline-f507b8f` holds
`f507b8f`'s tree, deletes `.github/workflows/ci.yml` and adds
`analyzer-baseline.yml`, which is the `static-analysis` job of the branch head
with its `needs` dropped. Three shards, no suite, and the collector of the
branch head unchanged, so the two lists are collected by the same code.

## The two sets

| Scope | Baseline lines | Branch lines | Baseline sha256 | Branch sha256 | Equal |
| --- | --- | --- | --- | --- | --- |
| `library` | 1933 | 1933 | `f4810371f55812ec` | `f4810371f55812ec` | byte for byte |
| `examples` | 2283 | 2283 | `3f9ddc97a4ccdc32` | `3f9ddc97a4ccdc32` | byte for byte |
| `tests` | 10122 | 10122 | `d6f39647dac784b2` | `206c539511e2ca98` | counts equal, lines differ |

`diff` over the two `library` files and the two `examples` files prints nothing,
in either direction. The `tests` pair differs in 233 identities on each side.

## The tests difference, by reason

Per-file finding counts stand equal on both sides: 289 in
`test/source/harness_jitter_cli_test.cpp`, 110 in
`test/source/harness_cli_table_test.cpp`, 5 in
`test/source/counters_trap_noguard_fixture.cpp`. Every one of the 233 lines
belongs to those three units, split 194, 38 and 1, and each is the same file
with the same check at a shifted line number.

| Class | Lines | Cause |
| --- | --- | --- |
| line shift in the jitter test | 194 | `37e068d test: Name the signal the exec child died on` added 5 lines to the unit, moving every finding below the insertion |
| line shift in the CLI table test | 38 | `615dcf6` and `79e0a05` rewrote 9 lines of the unit, moving the findings below them |
| interleaved collect line | 1 | the baseline list holds `checking for __attribute__(always_inline)... /home/runner/…/counters_trap_noguard_fixture.cpp:21:misc-include-cleaner`, one identity with a configure-log fragment welded in front of it; the branch list holds the clean identity |

Collapse the line number and compare `(file, check)` pairs: `library` and
`examples` stay equal at 534 and 702 pairs. `tests` reads 2115 pairs on the
baseline and 2114 on the branch, and the symmetric difference is three pairs.
Two of them are the two sides of the interleaved line above, which cancels once
the fragment is removed. The third is real and single:

```text
only in the baseline:
test/source/harness_jitter_cli_test.cpp  bugprone-easily-swappable-parameters
```

`37e068d` changed the signature that carried the two adjacent same-type
parameters, so clang-tidy no longer reports the pair. That is a finding the
branch removed on purpose, inside a test unit, and it is the only check the two
lists disagree about.

## The gate on both lists

`readability-identifier-naming`, the one check `.clang-tidy` promotes to an
error, hits zero times in all six files:

```text
library gate hits 0 0
tests   gate hits 0 0
examples gate hits 0 0
```

All six shards therefore passed their gate step, and the planted run in
`ph5/planted.md` shows what happens when the count is one: every shard stops at
`Build <scope> scope` and again at `Fail on gate findings`. SC-011 asks for list
equality with the bite intact; the lists agree at check granularity with the one
deliberate removal above, and the bite is demonstrated.

## What no artifact carries

The analyzer identity stays outside the artifacts. `Record compiler version`
runs `${CXX:-c++} --version | head -n 1 > .ccache-cxx-version`
(`.github/workflows/ci.yml:86`), which feeds the cache key, and no step prints
`clang-tidy --version` into the log or the uploaded file. Both runs install
`clang-tidy cppcheck ccache doxygen python3-yaml lcov autoconf automake libtool
patch` from the same `ubuntu` runner image, so the two lists were produced by
whatever the image shipped on 2026-10-25, and a future divergence between two
images would show up here as an unexplained diff. Naming the analyzer version in
the artifact is a candidate for a follow-up spec.

## Commands

```sh
gh run download 40242387830 -D /tmp/artbl
gh run download 40242340926 -D /tmp/art6
for s in library tests examples; do
  sha256sum /tmp/artbl/analyzer-findings-$s/findings.txt \
            /tmp/art6/analyzer-findings-$s/findings.txt
  diff /tmp/artbl/analyzer-findings-$s/findings.txt \
       /tmp/art6/analyzer-findings-$s/findings.txt | wc -l
  comm -3 <(sort -u /tmp/artbl/analyzer-findings-$s/findings.txt) \
          <(sort -u /tmp/art6/analyzer-findings-$s/findings.txt) | wc -l
done
for s in library tests examples; do
  comm -3 <(sed -E 's#:[0-9]+:##' /tmp/artbl/analyzer-findings-$s/findings.txt | sort -u) \
          <(sed -E 's#:[0-9]+:##' /tmp/art6/analyzer-findings-$s/findings.txt | sort -u)
done
gh run view 40042928839 --log | sed -E 's/^[^Z]*Z //' | grep -ac 'Building CXX object'
git show --stat --format= 37e068d 615dcf6 79e0a05
```

VERDICT: PASS. Two of three scopes are byte-identical; the third differs only by
line numbers inside three test units the branch edited, by one interleaved log
fragment in the baseline's own list, and by one finding the branch removed on
purpose. The gate check is empty on both sides and bites on a planted violation.
