# T030: one planted naming violation, and the shard that bites

Branch `030-planted-violation`, head `63b1e14`, cut from the feature head
`e77b713` with one commit on top. Run 40243459369, attempt 1, workflow
`analyzer-plant.yml`: the `static-analysis` job copied out of `ci.yml` with its
`needs` dropped, so the three shards are the whole run. Reading commands:

```sh
gh run view 40243459369 --json jobs --jq '.jobs[] | .name as $n
  | (.steps[] | select(.conclusion=="failure") | [$n,.name] | @tsv)'
gh run view 40243459369 --log | sed -E 's/^[^Z]*Z //' \
  | grep -a 'readability-identifier-naming\|findings on the gate checks'
```

## The plant

`auto PlantedViolation() -> int` appended to `source/speedgun-ng.cpp`, the
smallest library unit at 28 lines. One function, no call site, no header. The
style it breaks is `readability-identifier-naming.GlobalFunctionCase`, whose
value in `.clang-tidy` is `camelBack`. `WarningsAsErrors` names that one check,
so the compile reports the finding as an error:

```text
source/speedgun-ng.cpp:32:6: error: invalid case style for global function
  'PlantedViolation' [readability-identifier-naming]
```

## What failed, per shard

| Shard | Failed steps | Gate line | Job wall |
| --- | --- | --- | --- |
| `library` | `Build library scope`, `Fail on gate findings` | `shard library: 1 findings on the gate checks (readability-identifier-naming)` | 126 s |
| `tests` | `Build tests scope`, `Fail on gate findings` | `shard tests: 1 findings on the gate checks (readability-identifier-naming)` | 133 s |
| `examples` | `Build examples scope`, `Fail on gate findings` | `shard examples: 1 findings on the gate checks (readability-identifier-naming)` | 149 s |

All three shards bite. Each scope builds the library target as a dependency, so the planted unit is compiled in all three:
the compile stops where `-Werror` is configured for the check, and the finding
list for each scope carries the same identity. The translation-unit counts the
shards printed before the failure are `library: 39`, `tests: 41`,
`examples: 39`.

Two consequences worth carrying forward. A gate on the error checks fails the
whole matrix, because library code reaches every scope. And the
analyzer pass still produced its list before the compile stopped, which is the
property T031 checks on a warm cache.

## Scope note

The plant lives only on `030-planted-violation`. No file under `include/` or
`source/` changes on `030-ci-speedup`, so the branch-wide rule that the
library stays untouched holds while this evidence exists. The branch is deleted
once the warm reading lands.
