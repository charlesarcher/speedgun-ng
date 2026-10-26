# Phase 4 Script Steps Against the Contract Table

**Surface**: `tools/ci/prepush.sh` at head, the thirteen rows of the
contract table at `specs/030-ci-speedup/contracts/prepush-script.md`, and the
plain run of the script measured on this host.
**Head sha**: b1f8edaba7a7a0e75bbc8465e2b08fc18361b34b, the head the script and the contract both stand at
**Run id**: local, run A of this record, under Run A below
**Command**: `bash tools/ci/prepush.sh` with no option, and
`bash tools/ci/prepush.sh --bogus` for the option path
**Numbers**: the table under Rows, the ten timing lines quoted under Run A,
and the two exits under Fault paths

T020 adds steps 1 to 5, T021 adds steps 6 to 9, T022 adds step 10 and T023
adds the three steps behind the full option. This record is the row by row
proof that the script holds all thirteen, in the order the contract names,
with the command the contract names.

## The commands the contract names

```text
 1 python3 tools/docs/example_drift.py .
 2 cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake
 3 bash test/time_source_gate.sh .
 4 cmake -P cmake/spell.cmake
 5 python3 tools/prose/prose_gate.py --base origin/master
 6 cmake --preset=prepush
 7 cmake --build --preset prepush -j "$(nproc)"
 8 ctest --preset prepush-quick -j "$(nproc)"
 9 taskset -c 0,1 ctest --preset prepush-quick -j 2
10 clang-tidy --checks=-*,readability-identifier-naming over the changed units
11 ctest --preset prepush-full -j "$(nproc)"
12 the consumer-release job's configure and build
13 cmake --preset=ci-tsan and its build
```

## Rows

| Row | Step name the script prints | Line in the script | CI job it mirrors | Command match |
| --- | --- | --- | --- | --- |
| 1 | drift gate | `:165` | `lint` | yes |
| 2 | format gate | `:182` | `lint` | yes, after the version stop at `:167` to `:181` |
| 3 | time-source gate | `:183` | `lint` | yes |
| 4 | spell gate | `:184` | `lint` | yes |
| 5 | prose gate | `:185` | `prose-lint` | yes |
| 6 | configure | `:193` | `test` | yes |
| 7 | build | `:194` | `test` Build | one deviation, under Row 7 |
| 8 | quick tier, full affinity | `:195` | `test` Test | yes |
| 9 | quick tier, two cpus | `:196` | the same tier pinned | yes |
| 10 | naming check on the changed units | `:231` | `static-analysis` | yes, as a function |
| 11 | slow tier | `:256` | the CI suite | yes, behind the full option |
| 12 | consumer release build | `:257` | `consumer-release` | yes, as a function |
| 13 | tsan configure and link | `:258` | `tsan` | yes, as a function |

The order the script runs is the order of the table: the ten steps of the
plain option run in file order, and the naming check sits at `:231` because
its body is a function defined between steps 9 and 10. Steps 11 to 13 sit
inside the block that the full option guards, in the contract's order.

## Row 7

The contract's row 7 asks for a build preset named the same as the
configure preset. No such preset exists, and plan.md adds none: the build
presets of `CMakePresets.json` are `ci-sanitize` and `ci-tsan`, and the test
presets are `ci-tsan`, `ci-sanitize`, `prepush-quick` and
`prepush-full`. `specs/030-ci-speedup/contracts/presets.md` carries that
shape, and its own verify line builds the tree by directory. The script
builds the same way:

```text
step 'build' cmake --build build/prepush -j "$(nproc)"
```

The directory is the one the row 6 configure writes, so the tree that step
8 tests is the tree step 7 built. A second deviation falls out of the same
reading: a reader who wants the preset form has to add a build preset, and
that is a change to a file this spec's scope keeps to the presets it names.

## Run A

The plain run at head, on the lane host with a warm cache:

```text
prepush: drift gate 0 s
prepush: format gate 20 s
prepush: time-source gate 1 s
prepush: spell gate 2 s
prepush: prose gate 2 s
prepush: configure 1 s
prepush: build 0 s
prepush: quick tier, full affinity 87 s
prepush: quick tier, two cpus 622 s
prepush: naming check on the changed units 6 s
WALL=741
```

Ten lines for ten steps, and the ten seconds sum to the wall time the run
printed. `results/ph4/timing.md` carries the cache state around those lines
and the reading of the 622 s pinning against SC-007.

## Fault paths, in brief

T024 owns these; two readings are quoted here because they are the exits
the contract's verify block names.

```text
$ bash tools/ci/prepush.sh --bogus >/dev/null 2>&1; echo "rc=$?"
rc=2

$ bash tools/ci/prepush.sh --self-check
self-check: unknown option pass
self-check: missing tool pass
self-check: formatter version pass
self-check: failing step pass
```

The unknown option exits 2 and reaches no step. The self-check mode drives
each fault path in a scratch directory and exits 0 only when all four
behave, which is the proof route the plan's Test Plan names for shell code.

## Gates over this record

```text
$ python3 /tmp/chk.py specs/030-ci-speedup/results/ph4/steps.md
evaluate_prose -> (1, 111, 0, 0)

$ cmake -P cmake/spell.cmake
Used config files:
    1: .codespellrc
```

The first command drives the same rule engine and the same rule file the
prose gate uses, over this record alone: the tuple is sources, units
examined, findings, skipped. Tree mode of the gate itself reads tracked
files through `git ls-files`, so an unstaged record sits outside its count.
`cmake -P cmake/spell.cmake` walks the working tree and reported no hit in
this file.

VERDICT: PASS. Thirteen rows, thirteen script lines, one deviation named
with its cause and the file that keeps it in place.
