# Phase 6 Compiler Records

**Surface**: the configure log of the tree each of the three registering
lanes owns under `build/comp-*`, the header lines at
`tools/dbc/semantics_matrix.sh:134` to `:137` that a cell run prints,
and PC-6 of `specs/030-ci-speedup/spec.md:189` to `:193`.
**Head sha**: 49c12ecee775ed6fafa66e18d341c081edd194b9
**Run id**: local, with CI run 40235334432 for the two cross-check
tables
**Command**: the three configure commands and the three cell commands
under The readings
**Numbers**: the table under Cross-check, one row per lane

FR-023 asks every registering lane to record the compiler version of its
own configure log, and D-5 names the three lanes that register the
matrix: `test`, `test-rocky` and `tsan`, which are the `ci-ubuntu-test`,
`ci-rocky` and `ci-tsan` presets. Each lane below is a tree configured
with its own preset, and each lane ran one cell, so both readings of a
lane come from that lane. The header the script prints at `:135` to
`:137` is the second reading, and it is the line FR-023 wants in the log
of the lane that ran the cell.

## The readings

### The three lane trees

```text
$ cmake --preset=ci-ubuntu-test -B build/comp-ci-ubuntu-test
-- The CXX compiler identification is GNU 16.2.1
-- Build files have been written to: /home/archerc/code/speedgun-ng/build/comp-ci-ubuntu-test
exit 0

$ cmake --preset=ci-rocky -B build/comp-ci-rocky
-- The CXX compiler identification is GNU 16.2.1
-- Build files have been written to: /home/archerc/code/speedgun-ng/build/comp-ci-rocky
exit 0

$ CXX=clang++ cmake --preset=ci-tsan -B build/comp-ci-tsan
-- The CXX compiler identification is Clang 23.1.3
-- Build files have been written to: /home/archerc/code/speedgun-ng/build/comp-ci-tsan
exit 0
```

`tsan` carries `CXX: clang++` at job level, at
`.github/workflows/ci.yml:196`, so `CXX` is set for the configure step
and the test step of that lane, and it was set here for both. `test` and
`test-rocky` set no `CXX`, so those two lanes take the host default. The
`tsan` log keeps `-- The C compiler identification is GNU 16.2.1`, which
is the same split CI shows.

### The three cache switches

```text
$ for p in ci-ubuntu-test ci-rocky ci-tsan; do grep -E \
    '^CMAKE_CXX_COMPILER:|^SPEEDGUN_DBC_MATRIX:' build/comp-$p/CMakeCache.txt
  done
CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/c++
SPEEDGUN_DBC_MATRIX:UNINITIALIZED=ON
CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/c++
SPEEDGUN_DBC_MATRIX:UNINITIALIZED=ON
CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++
SPEEDGUN_DBC_MATRIX:UNINITIALIZED=ON
```

The rows are in the order of the three commands above, so all three
lanes register the matrix, which is the switch FR-023 names.

### The three cell runs

```text
$ ctest --test-dir build/comp-ci-ubuntu-test -R '^dbc_matrix_off_o0$' -V
21: semantics_matrix.sh: cell=dbc_matrix_off_o0
21: semantics_matrix.sh: jobs=2
21: semantics_matrix.sh: compiler=g++ (GCC) 16.2.1 20260810
1/1 Test #21: dbc_matrix_off_o0 ................   Passed  134.59 sec
exit 0

$ ctest --test-dir build/comp-ci-rocky -R '^dbc_matrix_off_o0$' -V
21: semantics_matrix.sh: cell=dbc_matrix_off_o0
21: semantics_matrix.sh: jobs=2
21: semantics_matrix.sh: compiler=g++ (GCC) 16.2.1 20260810
1/1 Test #21: dbc_matrix_off_o0 ................   Passed  133.45 sec
exit 0

$ CXX=clang++ ctest --test-dir build/comp-ci-tsan -R '^dbc_matrix_off_o0$' -V
21: semantics_matrix.sh: cell=dbc_matrix_off_o0
21: semantics_matrix.sh: jobs=2
21: semantics_matrix.sh: compiler=clang version 23.1.3
1/1 Test #21: dbc_matrix_off_o0 ................   Passed  140.84 sec
exit 0
```

The three quoted header lines are lines 16 to 18 of each log, directly
below the test command at line 13 and the 300 s budget at line 15. CTest
prefixes every line the script prints with the test number. Each cell
built in a `mktemp -d` tree of its own and removed it at exit, because
no run set `KEEP_MATRIX`, so each log carries the script's own
`cleaned` line naming the tree it deleted, at line 621 of 632, below the
cell verdict line and above the ctest summary.

## Cross-check

| Lane | Tree | Configure log identification | Cell header compiler | Match |
| --- | --- | --- | --- | --- |
| `test` | `build/comp-ci-ubuntu-test` | GNU 16.2.1 | `g++ (GCC) 16.2.1 20260810` | yes |
| `test-rocky` | `build/comp-ci-rocky` | GNU 16.2.1 | `g++ (GCC) 16.2.1 20260810` | yes |
| `tsan` | `build/comp-ci-tsan` | Clang 23.1.3 | `clang version 23.1.3` | yes |

Three lanes carry both readings and the two agree in every lane, so the
compiler an entry builds with is the compiler its own lane's configure
log named. The `tsan` row is the row where the two could have parted:
that tree holds `CMAKE_CXX_COMPILER` at `/usr/bin/clang++` while its `C`
compiler stays GNU, and the script reads only `CXX` at `:62`, so the
entry followed the C++ side of the lane.

## PC-6 at the head

`git show HEAD:specs/030-ci-speedup/spec.md` at head
`49c12ecee775ed6fafa66e18d341c081edd194b9`, lines 189 to 193:

```text
- **PC-6: compiler families. PASS from the configure logs of run
  39708015373.** `test` and `coverage` use GCC 15.2.0, `test-rocky` uses
  GCC 14.3.1, `sanitize` and `tsan` use Clang 21.1.8 through
  `env: { CXX: clang++ }`. Re-read at the branch audit point from the
  logs of that run before D-5 is applied.
```

The text stands as written. PC-6 asks for a re-read at the branch audit
point, and the reading below is that re-read for the three lanes D-5
registers. CI run 40235334432 (`Continuous Integration`, `pull_request`
event, head `3498874b1bf264fe63e352c93e6b7958079e50c8`, an ancestor of
this head) reads per lane from each lane's own `Configure` step:

| Lane | Configure step line | PC-6 names | Match |
| --- | --- | --- | --- |
| `test` | `-- The CXX compiler identification is GNU 15.2.0` | GCC 15.2.0 | yes |
| `test-rocky` | `-- The CXX compiler identification is GNU 14.3.1` | GCC 14.3.1 | yes |
| `tsan` | `-- The CXX compiler identification is Clang 21.1.8` | Clang 21.1.8 | yes |

The `test` job of that run also ran a matrix cell, and its `Test` step
prints the header the cell wrote:

```text
semantics_matrix.sh: cell=all
semantics_matrix.sh: jobs=2
semantics_matrix.sh: compiler=g++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0
```

PC-6 needs no amendment: every version it names is the version that
lane's own configure log prints at a head of this branch.

## Where the readings stop short

- This host carries its own toolchain (GCC 16.2.1 and Clang 23.1.3) and
  no `rockylinux/rockylinux:10` image, so the `ci-rocky` tree here reads
  GNU 16.2.1 while the rocky lane's own family is the GCC 14.3.1 read
  from that lane's CI configure log above. The host readings prove the
  pair agrees inside a lane; they hold no claim about the runner image.
- Run 40235334432 ran the matrix as one aggregate entry, so its Test
  step prints `cell=all`. That head carries the switch in
  `ci-ubuntu-test` alone and one registration, so its `test-rocky` and
  `tsan` logs hold no cell header line, and their absence is a run that
  predates the split. Run 40239619604 sits at this head, and its three
  lane logs are where both readings sit once it closes.

## Gates over this record

```text
$ python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/030-ci-speedup/results/ph6/compiler-records.md
prose-lint: 0 sources, 0 units examined, 0 findings, 0 skipped
exit 0

$ cmake -P cmake/spell.cmake
Used config files:
    1: .codespellrc
exit 0
```

Tree mode walks tracked files, and this record is unstaged while the
driver holds the commit, so the summary above reads 0 sources. The same
command was re-run at `/tmp/compilers-prose-check`, a throwaway
repository holding a copy of `tools/prose/prose_gate.py`,
`tools/prose/prose_rules.yaml` and this record, staged in that index.
There the summary reads 1 source with 0 findings, which covers this
record. The main working tree keeps its own index: nothing outside this
record was staged or changed here.

VERDICT: PASS
