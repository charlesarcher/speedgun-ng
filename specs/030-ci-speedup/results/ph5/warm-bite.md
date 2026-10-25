# T031: the planted violation again, and what a failed shard caches

Branch `030-planted-violation`, head `63b1e14`, run 40243459369 rerun in place:
attempt 1 is the run `ph5/planted.md` records, attempt 2 is this reading. The
workflow is `analyzer-plant.yml`, the `static-analysis` job of `ci.yml` with its
`needs` dropped, so each attempt is three shard jobs.

**Head sha**: 63b1e14
**Run id**: 40243459369, attempts 1 and 2
**Command**: `gh run rerun 40243459369`, then the reads under Commands
**Numbers**: the two tables below

## The shard still fails, the same way

| Shard | Job, attempt 2 | Failed steps | Gate line | Wall 1 to 2 |
| --- | --- | --- | --- | --- |
| `library` | 121322229332 | `Build library scope`, `Fail on gate findings` | `shard library: 1 findings on the gate checks (readability-identifier-naming)` | 126 s to 109 s |
| `tests` | 121322229222 | `Build tests scope`, `Fail on gate findings` | `shard tests: 1 findings on the gate checks (readability-identifier-naming)` | 133 s to 175 s |
| `examples` | 121322229370 | `Build examples scope`, `Fail on gate findings` | `shard examples: 1 findings on the gate checks (readability-identifier-naming)` | 149 s to 123 s |

The finding is emitted again, word for word:

```text
source/speedgun-ng.cpp:32:6: error: invalid case style for global function
  'PlantedViolation' [readability-identifier-naming]
```

`shard library: 39 translation units`, `shard tests: 41`,
`shard examples: 39` again in attempt 2, so the unit list and the finding list
came out identical across attempts.

## Where the analyzer sits in a failing job

The step timeline of job 121322229332 says the order:

```text
Restore ccache | success          Count translation units | success
Configure | success               Collect findings | success
Zero ccache before Build | ok     Fail on gate findings | failure
Build library scope | failure     Upload findings | success
Build tests scope | skipped       Save ccache | skipped
Build examples scope | skipped    ccache stats after Build | skipped
```

CMake launches both analyzers inside the build of each unit, so the analyzer
pass is part of the step that fails: the `error:` line above is clang-tidy
speaking during `Build library scope`. The two collection steps run after that
failure and produce the same list, which is the property FR-017 protects: a
stopping compile costs the object file, never the finding.

No analyzer result reaches the cache. The ratio that proves it is the
`ccache stats after Build` line of a shard whose build succeeds, where
`Cacheable calls` counts the real compiles while the unit count shows two extra
analyzer launches each. That line stays unread at the head of this record: on a
failing shard the step is skipped, and the run that would show it was still
open. The command that closes it:

```sh
gh run view 40242340926 --log | sed -E 's/^[^Z]*Z //' \
  | grep -a 'Cacheable calls\|translation units'
```

`specs/030-ci-speedup/results/ph5/finding-diff.md` carries that reading once run
40242340926 closes.

## What a failed shard leaves behind

`Save ccache` carries `if: github.event_name == 'push'`
(`.github/workflows/ci.yml:118`), and GitHub skips a step whose job has already
failed unless its `if` overrides that. Both conditions apply here, so attempt 1
saved no cache and attempt 2 restored nothing: the key
`ccache-static-analysis-library-ci-ubuntu-5498f26b…-eda120b6…` was the same in
both attempts and the bucket was empty in both. The wall columns are therefore
two cold runs of the same tree, and the spread between them (library 17 s
faster, tests 42 s slower) is queue and runner noise, the only reading a pair of
cold runs can give.

That property belongs to the shard itself, wherever the shard runs. It costs
nothing on `master`, where the job does not exist, and it
costs the feature branch nothing once the analyzer list is clean, because a
clean shard reaches `Save ccache` with a passing build.

## Commands

```sh
gh run view 40243459369 --json attempt,conclusion,jobs
gh run view 40243459369 --json jobs --jq '.jobs[]
  | select(.name=="static-analysis (library)") | .steps[]
  | .name + " | " + .conclusion'
gh run view 40243459369 --log | sed -E 's/^[^Z]*Z //' \
  | grep -a 'invalid case style\|findings on the gate checks\|CCACHE_KEY_PREFIX'
python3 tools/prose/prose_gate.py --check prose --mode tree
```

VERDICT: PASS on the two things T031 asks: the analyzer ran inside the build on
the repeat and the shard failed again. The cache-hit ratio for FR-017 stays
open and names its command above.
