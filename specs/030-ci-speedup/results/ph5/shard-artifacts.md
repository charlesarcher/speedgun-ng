# Phase 5 Shard Artifacts

**Surface**: the `static-analysis` job of `.github/workflows/ci.yml`. The job
reaches this tree from T026 and T027, uncommitted at the head below, and the
four steps recorded here sit on top of it in the same working file.
**Head sha**: 898e038dd4ceb9d2761ac746ae43e1933a5e51a8
**Run id**: local
**Command**: the verify block of the task brief, the step driver under Step
readings, the launcher probe under Analyzer readings, and the two gate commands
**Numbers**: the step indices `[10, 11, 12]`, the single artifact name, the
three case readings under Step readings, and the two exit readings under
Analyzer readings

## The four steps

| Step name | What it holds |
| --- | --- |
| `Count translation units` | the `Building CXX object` count of the shard, printed |
| `Collect findings` | `findings.txt`, the sorted unique identities |
| `Fail on findings` | the count, and exit 1 when the file holds a line |
| `Upload findings` | the artifact `analyzer-findings-${{ matrix.scope }}` |

The count line shape is `shard <scope>: <n> translation units`. The library
shard prints `shard library: 41 translation units` for 41 units. The artifact
name pattern is `analyzer-findings-${{ matrix.scope }}`, so the three shards
write three names and no upload overwrites another: the union T029 builds needs
three files.

The log the three steps read is `analyzer-build.log` at the workspace root,
`/home/runner/work/speedgun-ng/speedgun-ng/analyzer-build.log` on a runner. Each
Build step pipes its output through `tee` into that file and starts with
`set -euo pipefail`. The default runner shell is `bash -e`, which carries no
`pipefail`, so a bare pipeline would let `tee` report success over a failed
build.

## Finding identities

`Collect findings` runs two `sed -nE` passes over the log and pipes both into
`LC_ALL=C sort -u > findings.txt`. An identity is `file:line:check`. The check
is the first token inside the trailing brackets, which drops the
`-warnings-as-errors` tag clang-tidy appends to an error-severity finding.

```text
clang-tidy:  /w/speedgun-ng/source/bar.cpp:9:1: error: ... [readability-identifier-naming,-warnings-as-errors]
cppcheck:    [check.cpp:42]: (error) Uninitialized variable: p [uninitvar]
cppcheck:    check.cpp:1:35: error: Null pointer dereference: p [nullPointer]
identity:    /w/speedgun-ng/source/bar.cpp:9:readability-identifier-naming
identity:    check.cpp:42:uninitvar
```

`note:` lines, the caret and source echo lines, `Checking` lines and the
`make` error lines match nothing, so they enter no identity. `Building C
object` lines stay out of the unit count, which counts `Building CXX object`
alone.

## The verdict

`Fail on findings` ends with `test "$n" -eq 0`, so a shard whose
`findings.txt` holds at least one line exits nonzero. The planted
`readability-identifier-naming` violation of FR-016 depends on that reading.

`.clang-tidy:21` lists `readability-identifier-naming` under
`WarningsAsErrors`, so such a finding also stops the compile and the Build step
reddens first. The four steps carry `if: always()` for that reason: the count
line, the finding list and the artifact still land on a shard that failed
inside the compiler. `strategy.fail-fast` is false, so a red shard cannot cancel
its two siblings and leave the union short of a file.

## Step readings

The three report steps were taken out of the workflow as it stands and run in a
scratch directory over three logs, with the scope expression replaced by the
literal `library`.

```text
--- case 1: findings present
[Count translation units] exit=0
shard library: 2 translation units
[Collect findings] exit=0
/home/runner/work/speedgun-ng/speedgun-ng/source/bar.cpp:9:readability-identifier-naming
/home/runner/work/speedgun-ng/speedgun-ng/source/foo.cpp:123:readability-identifier-naming
check.cpp:1:nullPointer
check.cpp:42:uninitvar
check.cpp:7:nullPointerRedundantCheck
[Fail on findings] exit=1
shard library: 5 findings
--- case 2: clean log
shard library: 2 translation units
[Fail on findings] exit=0
shard library: 0 findings
--- case 3: no build log at all
shard library: 0 translation units
[Fail on findings] exit=0
findings.txt written: yes
```

`bash -n` on the six step bodies of the job, the three Build steps and the three
report steps, reports no syntax error. The verify block of the brief prints:

```text
['Count translation units', 'Collect findings', 'Fail on findings'] [10, 11, 12]
['analyzer-findings-${{ matrix.scope }}']
always: True | findings.txt: True
```

## Analyzer readings

A scratch project at `/tmp/laneX` configures `Unix Makefiles` with
`CMAKE_CXX_CLANG_TIDY=clang-tidy` and `CMAKE_CXX_CPPCHECK=cppcheck;--inline-suppr`,
the same pair `ci-ubuntu` carries. clang-tidy is LLVM 23.1.3, cppcheck is
2.22.0.

```text
$ cmake --build build          # one naming violation, WarningsAsErrors set
build-exit=2
/tmp/laneX/src/bad.cpp:1:5: error: invalid case style for function 'bad_name' [readability-identifier-naming,-warnings-as-errors]

$ cmake --build build          # one null pointer finding, cppcheck only
build-exit=0
/tmp/laneX/src/bad.cpp:3:19: error: Null pointer dereference: p [nullPointer]
```

The first line normalizes to
`/tmp/laneX/src/bad.cpp:1:readability-identifier-naming`, and that shard fails
twice over: at the Build step and at `Fail on findings`. The second leaves the
build green at exit 0, so `Fail on findings` carries it alone. That pair is the
FR-016 case for each analyzer, on the same log the shards capture.

No GitHub run backs this record: `Run id` is local, the three scope builds were
never compiled here, and the counts a real shard prints are the counts its own
`Building CXX object` lines carry.
