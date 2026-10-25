# Phase 6 Per-Lane `ctest` List Difference

**Surface**: the five lane jobs that run `ctest` in
`.github/workflows/ci.yml` (`coverage`, `sanitize`, `tsan`, `test`,
`test-rocky`), the entry list `ctest -N` prints from each lane's build
tree, and the configure preset each lane names.
**Head sha**: 680356a3be5e523454ef64eb35ae5f8860762533, the head
`git rev-parse HEAD` gave while the ten configures ran. Both registration
blobs are unchanged since `489480f`, so every head after that commit
holds the two values quoted under The two trees.
**Run id**: local
**Command**: the tree commands under The two trees, the configure
command under Ten configures, and the listing command under The
listings, each printed with its exit status
**Numbers**: the counts table under Entry counts, the five hunks under
Differences, and the account under Where every line comes from

A `ctest -N` list carries entry names. The labels, budgets,
`PROCESSORS` values and locks that the other tasks of this feature set
sit inside each entry, so they reach no line of a list difference. What
this difference can move is a name: the five `dbc_matrix_*` entries T033
registers at `test/CMakeLists.txt:161` to `:181` under the guard at
`:158`, the single `dbc_semantics_matrix` entry they replace, and one
audit entry that an earlier commit of this branch added. Every changed
name line of every hunk below lands in one of those three groups
(FR-053, SC-004).

Three further jobs run `ctest` with a `-R` filter over a subset
(`dbc-gate`, `prose-lint`, `downstream-consumer`). They run no lane
listing, so they sit outside the five lanes this record compares.

## The two trees

```text
$ git clone --shared /home/archerc/code/speedgun-ng /tmp/ph6-list-diff-master
exit 0
$ git -C /tmp/ph6-list-diff-master checkout --detach f507b8f
HEAD is now at f507b8f Docs: Restore jitter probe facts
$ git -C /tmp/ph6-list-diff-master rev-parse HEAD
f507b8fe5e18f1aa4749dc5412ef6b072fcae9b1
exit 0
$ cp -a /home/archerc/code/speedgun-ng/external/. /tmp/ph6-list-diff-master/external/
exit 0
```

The detach runs before the vendored trees are copied in. A copied
submodule `.git` file names a git directory that exists only in the
origin repository, and `git checkout` in the shared clone aborts on it
with exit 128 while those files stand in the way. The same gitfile makes
a later git read inside the clone fail, so every git read quoted here
ran in `/home/archerc/code/speedgun-ng`, and the only git write in this
task is the detach above, inside `/tmp/ph6-list-diff-master`.

The clone holds one checkout, so the branch tree is an extracted copy
inside it, built with read-only git and two file copies:

```text
$ git ls-files -z | tar --null -T - -cf - | tar -xf - -C /tmp/ph6-list-diff-master/branch
$ cp -a test/CMakeLists.txt /tmp/ph6-list-diff-master/branch/test/CMakeLists.txt
$ cp -a CMakePresets.json /tmp/ph6-list-diff-master/branch/CMakePresets.json
exit 0
```

The extraction reads the index, so the two copies carry the rows T033
and T034 landed in those files. An earlier attempt copied those two
files by name into the tree root, which clobbered the root
`CMakeLists.txt` and failed its configure; that tree was removed and
rebuilt with the sequence above. The branch head took both files in
`bcc16bf test: Split the matrix row into five cells` and
`489480f CMake: Register the matrix on three lanes`. Neither blob moved
after `489480f`, so the branch tree holds the bytes that `git show HEAD:`
printed for it at every head after that commit:

```text
$ git show HEAD:test/CMakeLists.txt | sha256sum
d17238d81e05a71f314864f289752c04ebd8d9dacffd393511280589c645e40e  -
$ sha256sum /tmp/ph6-list-diff-master/branch/test/CMakeLists.txt
d17238d81e05a71f314864f289752c04ebd8d9dacffd393511280589c645e40e  /tmp/ph6-list-diff-master/branch/test/CMakeLists.txt
$ git show HEAD:CMakePresets.json | sha256sum
6178dccd0f6b1da3c5598a5effdc7773220b53011c06ea6e723c2dd18b78d174  -
$ sha256sum /tmp/ph6-list-diff-master/branch/CMakePresets.json
6178dccd0f6b1da3c5598a5effdc7773220b53011c06ea6e723c2dd18b78d174  /tmp/ph6-list-diff-master/branch/CMakePresets.json
exit 0
```

Both trees carry the same seven vendored trees: the file counts match
per directory, 196, 773, 544, 507, 749, 395 and 263 for
`hdrhistogram_c`, `hwloc`, `pmu-events`, `quill`, `simdjson`, `yaml-cpp`
and `zlib`. A configure reads the compiler and the presets, so nothing
else differs between the two sides of a lane.

## The preset each lane configures

| Lane | Branch preset | Base preset | Base line |
| --- | --- | --- | --- |
| `coverage` | `ci-coverage` | `ci-coverage` | `:64` |
| `sanitize` | `ci-sanitize` | `ci-sanitize` | `:107` |
| `tsan` | `ci-tsan` | `ci-tsan` | `:141` |
| `test` | `ci-ubuntu-test` | `ci-ubuntu` | `:166` |
| `test-rocky` | `ci-rocky` | `ci-rocky` | `:242` |

The line numbers come from `git show f507b8f:.github/workflows/ci.yml`,
which holds the `cmake --preset=` line each base job ran. The branch
column comes from the same file at the head, where those five lines sit
at `:83`, `:159`, `:224`, `:401` and `:505`.

The `test` lane names two different presets. `ci-ubuntu-test` is new on
this branch: `git show f507b8f:CMakePresets.json` lists no such preset,
and the base `test` job configured `ci-ubuntu`, so this record
configures `ci-ubuntu` on the base side to read the tree that job ran.
The two presets share `ci-build`, `ci-linux`, `dev-mode` and
`ci-cache`. `ci-ubuntu` adds the two analyzer launchers, and the branch
preset adds `SPEEDGUN_DBC_MATRIX`. The launchers reach compile
commands, and the flag reaches registration, which the hunks show.

## Ten configures

One command shape, run once per lane per side through
`/tmp/ph6-list-diff-master/t037-configure.sh`. The `-B` argument names
this node's directory in each tree, so no lane tree is shared and
nothing under the repository `build` path is touched:

```text
$ cmake --preset=<preset> -S <tree> -B <tree>/build/t037-list-<lane>-<side>

cfg rc=0 lane=coverage side=base preset=ci-coverage
cfg rc=0 lane=sanitize side=base preset=ci-sanitize
cfg rc=0 lane=tsan side=base preset=ci-tsan
cfg rc=0 lane=test side=base preset=ci-ubuntu
cfg rc=0 lane=test-rocky side=base preset=ci-rocky
cfg rc=0 lane=coverage side=branch preset=ci-coverage
cfg rc=0 lane=sanitize side=branch preset=ci-sanitize
cfg rc=0 lane=tsan side=branch preset=ci-tsan
cfg rc=0 lane=test side=branch preset=ci-ubuntu-test
cfg rc=0 lane=test-rocky side=branch preset=ci-rocky
ALL_CONFIGURES_DONE
exit 0
```

The ten trees are `/tmp/ph6-list-diff-master/build/t037-list-*` on the
base side and
`/tmp/ph6-list-diff-master/branch/build/t037-list-*` on the branch side.
The compilers are the machine default `/usr/bin/c++`, the same on both
sides of every lane.

## The listings

One command shape per tree, run through
`/tmp/ph6-list-diff-master/t037-list.sh`, which writes each raw listing
and each sorted list under `/tmp/ph6-list-diff-master/logs/`:
```text
$ ctest --test-dir <dir> -N
$ grep -E '^ *Test +#[0-9]+: ' | sed -E 's/^ *Test +#[0-9]+: //' | sort
$ diff <base list> <branch list>

list rc=0 lane=coverage side=base entries=134
list rc=0 lane=sanitize side=base entries=134
list rc=0 lane=tsan side=base entries=130
list rc=0 lane=test side=base entries=134
list rc=0 lane=test-rocky side=base entries=134
list rc=0 lane=coverage side=branch entries=134
list rc=0 lane=sanitize side=branch entries=134
list rc=0 lane=tsan side=branch entries=135
list rc=0 lane=test side=branch entries=139
list rc=0 lane=test-rocky side=branch entries=139
exit 0
```

### Entry counts

| Lane | Base | Branch | Delta |
| --- | --- | --- | --- |
| `coverage` | 134 | 134 | 0 |
| `sanitize` | 134 | 134 | 0 |
| `tsan` | 130 | 135 | +5 |
| `test` | 134 | 139 | +5 |
| `test-rocky` | 134 | 139 | +5 |

`tsan` lists four fewer entries than its peers on both sides:
`counters_noalloc_test`, `harness_argument_test`,
`harness_capture_test` and `harness_jitter_probe_alloc_test` stand in
the other four trees of each side. That gap sits inside each side and
moves no diff line.

## Differences

Sorted listings, base first. `diff` exits 1 on every lane because every
lane differs.

`coverage`:

```text
37d36
< dbc_semantics_matrix
129a129
> test_tier_labels
```

`sanitize`:

```text
37d36
< dbc_semantics_matrix
129a129
> test_tier_labels
```

`tsan`:

```text
36c36,40
< dbc_semantics_matrix
---
> dbc_matrix_observe_off_o0
> dbc_matrix_off_o0
> dbc_matrix_off_o2
> dbc_matrix_on_o0
> dbc_matrix_on_o2
125a130
> test_tier_labels
```

`test`:

```text
37c37,41
< dbc_semantics_matrix
---
> dbc_matrix_observe_off_o0
> dbc_matrix_off_o0
> dbc_matrix_off_o2
> dbc_matrix_on_o0
> dbc_matrix_on_o2
129a134
> test_tier_labels
```

`test-rocky`:

```text
37c37,41
< dbc_semantics_matrix
---
> dbc_matrix_observe_off_o0
> dbc_matrix_off_o0
> dbc_matrix_off_o2
> dbc_matrix_on_o0
> dbc_matrix_on_o2
129a134
> test_tier_labels
```

## Where every line comes from

The five hunks hold 25 changed name lines and 13 marker lines that
carry a hunk position or a separator. Every name line lands in one of
three groups.

| Group | Lines | Lanes |
| --- | --- | --- |
| `< dbc_semantics_matrix` | 5 | all five |
| `> dbc_matrix_*` | 15 | `test`, `test-rocky`, `tsan` |
| `> test_tier_labels` | 5 | all five |

The first group is the aggregate entry leaving the registration, the
second is the five cells arriving, and the third is the tier audit
entry of FR-048, named in its own section below.

`bcc16bf` removed the single `NAME dbc_semantics_matrix` entry that base
held at `test/CMakeLists.txt:139`, inside `if(CMAKE_SYSTEM_NAME STREQUAL
"Linux")`, and put five entries in its place at `:161` to `:181` inside
the `if(SPEEDGUN_DBC_MATRIX)` guard at `:158`. Each entry passes its own
cell name and `SPEEDGUN_TEST_JOBS`, so `ctest` can place the cells apart
(FR-019, FR-021, D-4).

`489480f` set `SPEEDGUN_DBC_MATRIX` to `ON` in `ci-ubuntu-test`,
`ci-rocky` and `ci-tsan`, and left the key out of `ci-coverage` and
`ci-sanitize` (FR-023, D-5). That is why the two dropping lanes show the
aggregate leaving and no cell arriving: the guard reads the variable, an
unset variable registers nothing, and those trees hold neither the
aggregate nor the five. `specs/030-ci-speedup/results/ph6/lane-flag.md`
reads the same three `ON` values and the two absent keys, and
`dropping-lanes.md` carries the evidence that a dropped matrix loses no
check there (FR-024).

The two lanes that keep the aggregate off also keep their count at 134:
one name out, one audit name in. The three registering lanes take one
name out and six in, which is the `+5` in the counts table.

## One line sits outside the matrix moves

`test_tier_labels` belongs to no matrix move. `4340dcd test: Add the
tier audit seen failing` added it, and it sits at
`test/CMakeLists.txt:234` outside the matrix guard. The
`test-registration.md` contract names it for FR-048: it runs the `ctest`
binary against the build directory, compares the listings, and fails on
a label, budget or retry fault. `4340dcd` landed before T033 and
reaches every lane, because no preset gates it, so all five hunks carry
the one line at once.

SC-004 asks for a matrix-only difference. The 20 matrix lines meet it
in all five lanes. These 5 lines sit outside that set, named here with
their commit, their contract clause and their reason for appearing in
every lane. No other line appears anywhere in the five hunks.

## Gates over this record

```text
$ python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/030-ci-speedup/results/ph6/list-diff.md
prose-lint: 1 sources, 292 units examined, 0 findings, 0 skipped
PROSE_RC=0
$ cmake -P cmake/spell.cmake
Used config files:
    1: .codespellrc
SPELL_RC=0
```

Both gates exit 0. Tree mode of the prose gate discovers its files
through `git ls-files`, so the run above reads this record once the
driver's commit tracks it. While the record stood untracked that summary
read 0 sources, so the same rule data and the same evaluation function
were run over the file directly:

```text
$ python3 - <<'PY'
import argparse, importlib.util, pathlib
spec = importlib.util.spec_from_file_location("pg", "tools/prose/prose_gate.py")
pg = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pg)
path = "specs/030-ci-speedup/results/ph6/list-diff.md"
pg.collect_candidates = lambda repo, mode, base, head: ([path], None)
args = pg.parse_args(["--check", "prose", "--mode", "tree", "--paths", path])
data = pg.load_rules(pathlib.Path("tools/prose/prose_rules.yaml"))
print("evaluate_prose ->", pg.evaluate_prose(args, data))
PY
evaluate_prose -> (1, 292, 0, 0)
exit 0
```

The tuple is sources, units examined, findings, skipped. That run
examined every unit of this record against
`tools/prose/prose_rules.yaml` and none hit a rule.
`cmake -P cmake/spell.cmake` walks the working tree, so codespell read
this record and reported nothing.

VERDICT: PASS. Five lanes diffed and all 25 name lines accounted for:
20 matrix moves and 5 tier audit lines named above.
