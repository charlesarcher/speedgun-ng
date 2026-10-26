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

No analyzer result reaches the cache. The proof is a ratio: `Cacheable calls`
beside the compile lines of a shard whose build succeeds, where two analyzer
launches per unit would triple the call count. The section that reads it sits
below, for the five jobs of run 40242340926.

`specs/030-ci-speedup/results/ph5/finding-diff.md` carries the finding list of
that same run beside master's.

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

## Whether ccache sees the analyzers

FR-017 asks that the cache hold compiles and no analyzer result, on the ground
that CMake runs both analyzers ahead of the compile of every unit, so a cached
compile would still leave the analyzer work undone. The statistics answer that
without any instrumenting, because ccache reports the launches it is asked
about. Run 40242340926 at `e77b713` gives, per job, the compile lines its build
printed and the launches ccache was consulted for:

| Job | `Building CXX object` | `Building C object` | compiles | `Cacheable calls` |
| --- | --- | --- | --- | --- |
| `static-analysis (library)` | 123 | 22 | 145 | 142 / 144 |
| `static-analysis (tests)` | 214 | 22 | 236 | 233 / 235 |
| `static-analysis (examples)` | 155 | 22 | 177 | 174 / 176 |
| `test` | 245 | 22 | 267 | 265 / 267 |
| `tsan` | 241 | 22 | 263 | 261 / 263 |

Each lane is asked about one launch per compile, within three of the compile
count. The shard builds 145 units and ccache hears about 144 calls, of which 142
are cacheable. Three launches per unit would read 435 for that shard, and the
analyzers together add 290 units of work that never appear. The two analyzer
processes per unit run outside ccache, which is the arrangement FR-017 asks for:
the cache stores the object file, and a cache hit still runs both analyzers over
the unit.

## Commands

```sh
gh run view 40243459369 --json attempt,conclusion,jobs
gh run view 40243459369 --json jobs --jq '.jobs[]
  | select(.name=="static-analysis (library)") | .steps[]
  | .name + " | " + .conclusion'
gh run view 40243459369 --log | sed -E 's/^[^Z]*Z //' \
  | grep -a 'invalid case style\|findings on the gate checks\|CCACHE_KEY_PREFIX'
gh run view --job 121316717719 --log | grep -ac 'Building CXX object'
gh run view --job 121316717719 --log | grep -ac 'Building C object'
gh run view 40242340926 --log | sed -E 's/^[^Z]*Z //' \
  | grep -a 'Cacheable calls\|Direct cache hits'
python3 tools/prose/prose_gate.py --check prose --mode tree
```

VERDICT: PASS on all three readings. The analyzer ran inside the build on the
repeat and the shard failed again; and the cache is consulted once per compile
across five jobs, so no analyzer result sits in it (FR-017).
