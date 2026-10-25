# Phase 6 Dropping the Matrix Cells From the Coverage Lane

**Surface**: `tools/dbc/semantics_matrix.sh`, read at four points: its
own configure command, the compiler variable it reads, the directory its
build trees live in, and the coverage flag list it passes. The coverage
side is `test/CMakeLists.txt`, `CMakePresets.json` and
`cmake/coverage.cmake`, plus a configured `ci-coverage` tree at
`build/drop-cov`.
**Head sha**: 49c12ecee775ed6fafa66e18d341c081edd194b9
**Run id**: local
**Command**: the commands under The readings and under Gates over this
record, each run from the repository root. No cell was run: the task
reads the script.
**Numbers**: the table under Checks, one row per command, with the line
number, count or exit each produced.

FR-024 asks whether the five matrix cells can move a coverage number.
They cannot, on two independent grounds. The `coverage` lane registers
no matrix entry, so no cell runs there. And a cell that runs in another
lane compiles nothing with a coverage flag into a directory that the
coverage capture reads. The four readings below establish the second
ground from the script and the first from the registration.

## Where the task's line numbers sit at head

T032 grew the script from 362 to 436 lines, so every pre-T032 anchor
moved. The task line names `:181`, `:182`, `:54` and `:76`, written
against the file as it stood at `2de72a6^`.

| Task anchor | Line content | Line at head | Shift |
| --- | --- | --- | --- |
| `:181` | `-DCMAKE_BUILD_TYPE=None \` | `:231` | plus 50 |
| `:182` | `-DCMAKE_CXX_FLAGS="$flags" \` | `:232` | plus 50 |
| `:54` | `CXX=${CXX:-g++}` | `:62` | plus 8 |
| `:76` | `MATRIX_ROOT=$(mktemp -d /tmp/sg-dbc-matrix.XXXXXX)` | `:119` | plus 43 |

```text
$ git show '2de72a6^:tools/dbc/semantics_matrix.sh' | wc -l
362
$ wc -l tools/dbc/semantics_matrix.sh
436
$ git show '2de72a6^:tools/dbc/semantics_matrix.sh' \
    | awk 'NR==54||NR==76||NR==181||NR==182{printf "%d:%s\n", NR, $0}'
54:CXX=${CXX:-g++}
76:MATRIX_ROOT=$(mktemp -d /tmp/sg-dbc-matrix.XXXXXX)
181:    -DCMAKE_BUILD_TYPE=None \
182:    -DCMAKE_CXX_FLAGS="$flags" \
exit 0
```

The four lines carry the same text at both anchors, so the shift moved
positions and changed no construct.

## The readings

### One: the cell configures on its own terms

```text
$ grep -n 'cmake -S' tools/dbc/semantics_matrix.sh
225:  cmake -S "$REPO_ROOT" -B "$dir" \
$ grep -n preset tools/dbc/semantics_matrix.sh
exit 1
$ awk 'NR>=225&&NR<=236{printf "%d:%s\n", NR, $0}' tools/dbc/semantics_matrix.sh
225:  cmake -S "$REPO_ROOT" -B "$dir" \
226:    -Dspeedgun-ng_DEVELOPER_MODE=ON \
227:    -Dspeedgun-ng_CONTRACTS="$contracts" \
228:    -DCMAKE_CXX_STANDARD=23 \
229:    -DCMAKE_CXX_STANDARD_REQUIRED=ON \
230:    -DCMAKE_CXX_EXTENSIONS=OFF \
231:    -DCMAKE_BUILD_TYPE=None \
232:    -DCMAKE_CXX_FLAGS="$flags" \
233:    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
234:    -DBUILD_MCSS_DOCS=OFF \
235:    -DBUILD_EXAMPLES=OFF \
236:    -DCMAKE_CXX_COMPILER="$CXX"
exit 0
```

The word `preset` appears nowhere in 436 lines: the grep exits 1. The
cell names its generator input by hand. `CMAKE_BUILD_TYPE` is `None` at
`:231`, and the flags the cell compiles with are its own
`CMAKE_CXX_FLAGS` value at `:232`, built at `:215` to `:218` from the
cell's optimization level and its `-DNDEBUG` choice.

### Two: the compiler comes only from `CXX`

```text
$ grep -n 'CXX=' tools/dbc/semantics_matrix.sh
62:CXX=${CXX:-g++}
$ grep -n 'CMAKE_CXX_COMPILER' tools/dbc/semantics_matrix.sh
236:    -DCMAKE_CXX_COMPILER="$CXX"
$ grep -nE 'CMAKE_C_COMPILER|\$\{?CC\}' tools/dbc/semantics_matrix.sh
exit 1
```

`$CXX` is read once at `:62`, defaults to `g++`, and is the only
compiler the script names. It hands that value to CMake at `:236`,
prints it at `:138`, takes its version at `:134`, and drives the three
probe compiles at `:315`, `:326` and `:346` with the same variable. No
`CC` and no `CMAKE_C_COMPILER` appear anywhere, so a lane that exports
one value controls the whole cell.

### Three: the build trees come from `mktemp -d`

```text
$ grep -n -e mktemp -e 'MATRIX_ROOT=' -e 'local dir=' tools/dbc/semantics_matrix.sh
119:MATRIX_ROOT=$(mktemp -d /tmp/sg-dbc-matrix.XXXXXX)
212:  local dir="$MATRIX_ROOT/$name"
$ grep -n -e KEEP_MATRIX -e 'trap cleanup' -e 'rm -rf' tools/dbc/semantics_matrix.sh
122:  if [ "${KEEP_MATRIX:-0}" = 1 ]; then
123:    echo "semantics_matrix.sh: KEEP_MATRIX=1, dirs left at $MATRIX_ROOT"
125:    rm -rf "$MATRIX_ROOT"
129:trap cleanup EXIT
```

`:119` makes one fresh root per invocation under `/tmp`. `:212` puts
each cell in a subdirectory of it, and `:225` configures into that path
with `-B "$dir"`. The exit trap at `:129` removes the root unless
`KEEP_MATRIX=1` asks for the trees. A cell tree is a throwaway directory
outside `build/`. `ls /tmp | grep -c sg-dbc-matrix` prints `0` and exits
1 on this machine right now, because no cell has run here today.

### Four: no cell count reaches a coverage trace

The registration gate at `test/CMakeLists.txt:155` to `:158` decides
which lanes carry the five entries:

```text
$ awk 'NR>=155&&NR<=158{printf "%d:%s\n", NR, $0}' test/CMakeLists.txt
155:  # Only the lanes whose preset sets SPEEDGUN_DBC_MATRIX carry the matrix
156:  # entries: each configures and builds a tree of its own, so coverage and
157:  # sanitize leave them unregistered (030 FR-019, D-5, P-01).
158:  if(SPEEDGUN_DBC_MATRIX)
$ grep -n -B 6 'SPEEDGUN_DBC_MATRIX' CMakePresets.json | grep -E 'name|SPEEDGUN_DBC_MATRIX'
138:        "SPEEDGUN_DBC_MATRIX": "ON"
161-      "name": "ci-ubuntu-test",
164:        "SPEEDGUN_DBC_MATRIX": "ON"
168-      "name": "ci-rocky",
171:        "SPEEDGUN_DBC_MATRIX": "ON"
exit 0
```

Three presets set the variable: `ci-tsan` at `:138`, `ci-ubuntu-test` at
`:164`, `ci-rocky` at `:171`. `ci-coverage` sits at `:113` and sets
none. A configured coverage tree shows what that leaves:

```text
$ cmake --preset=ci-coverage -B build/drop-cov
-- Configuring done (4.2s)
-- Generating done (0.3s)
-- Build files have been written to: /home/archerc/code/speedgun-ng/build/drop-cov
exit 0

$ ctest --test-dir build/drop-cov -N -R dbc_matrix
Test project /home/archerc/code/speedgun-ng/build/drop-cov

Total Tests: 0
exit 0

$ ctest --test-dir build/drop-cov -N | tail -2

Total Tests: 134
exit 0

$ grep -n -e 'SPEEDGUN_DBC_MATRIX' -e 'ENABLE_COVERAGE' -e 'CMAKE_BUILD_TYPE:' build/drop-cov/CMakeCache.txt
37:CMAKE_BUILD_TYPE:STRING=Coverage
430:ENABLE_COVERAGE:BOOL=ON
exit 0
```

The coverage tree is a real one: it carries 134 entries and turns
`ENABLE_COVERAGE` on. It holds no `SPEEDGUN_DBC_MATRIX` line at all,
and `ctest` lists zero matrix entries in it, so no cell runs in that
lane.

Second ground, for a cell that runs in a lane that does carry it. The
script passes no coverage flag:

```text
$ grep -nE -- '--coverage|gcov|profile|ENABLE_COVERAGE|CMAKE_CXX_FLAGS_COVERAGE' tools/dbc/semantics_matrix.sh
exit 1
```

Coverage instrumentation reaches this project through the `Coverage`
build configuration, whose flag list lives at `CMakePresets.json:107`.
A cell pins `CMAKE_BUILD_TYPE=None` at `:231`, so that flag list never
applies to a cell compile. gcov writes a `.gcno` note file only for a
compile that carries `--coverage`, so a cell produces none, in any lane.

The capture would not look where such a file would sit. The trace
command at `cmake/coverage.cmake:58` to `:65` walks the project's own
binary directory, and the extract step at `:85` to `:87` keeps only
paths under `PROJECT_SOURCE_DIR`:

```text
$ awk 'NR>=58&&NR<=65{printf "%d:%s\n", NR, $0}' cmake/coverage.cmake
58:    "${LCOV_EXECUTABLE}" -c -q
59:    --branch-coverage
60:    --no-external
61:    --ignore-errors mismatch,negative,inconsistent
62:    --base-directory "${PROJECT_SOURCE_DIR}"
63:    --rc geninfo_unexecuted_blocks=1
64:    -o "${PROJECT_BINARY_DIR}/coverage.raw.info"
65:    -d "${PROJECT_BINARY_DIR}"
exit 0
```

A cell compiles into `/tmp/sg-dbc-matrix.XXXXXX`, a path `lcov` is
never handed. Its counts cannot join the trace: the note files do not
exist and the directory sits outside the `-d` argument.

## Checks

| Check | Expect | Exit | Decisive line | Verdict |
| --- | --- | --- | --- | --- |
| `grep 'cmake -S'` | the configure line at `:225` | 0 | `cmake -S "$REPO_ROOT" -B "$dir" \` | PASS |
| `grep preset` | no match | 1 | no output line | PASS |
| build type and flags | `None` at `:231`, own flags at `:232` | 0 | `-DCMAKE_BUILD_TYPE=None \` | PASS |
| `grep 'CXX='` | one read at `:62` | 0 | `CXX=${CXX:-g++}` | PASS |
| `grep 'CMAKE_C_COMPILER'` and `CC` | no match | 1 | no output line | PASS |
| `grep mktemp` | root at `:119`, cell dir at `:212` | 0 | `MATRIX_ROOT=$(mktemp -d ...)` | PASS |
| `ls /tmp` for matrix roots | none on disk | 1 | `0` | PASS |
| coverage cache grep | `Coverage` and `ENABLE_COVERAGE:BOOL=ON` | 0 | `430:ENABLE_COVERAGE:BOOL=ON` | PASS |
| configure `ci-coverage` into `build/drop-cov` | 0 | 0 | generating done for `build/drop-cov` | PASS |
| matrix list in that tree | empty, 0 | 0 | `Total Tests: 0` | PASS |
| full list in that tree | 134 | 0 | `Total Tests: 134` | PASS |
| coverage tokens in the script | no match | 1 | no output line | PASS |

## What this task changed

Nothing outside this record. No script, preset, registration or build
tree other than `build/drop-cov` was touched, and that directory is this
node's own. T032, T033 and the lane flag landed as `2de72a6`, `bcc16bf`
and `489480f` during this run, so the head sha above already holds the
gate and the three preset flags quoted here; the script itself stands
unchanged since `2de72a6`, which is the version the four anchors here
cite.

## Gates over this record

```text
$ python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/030-ci-speedup/results/ph6/dropping-lanes.md
prose-lint: 0 sources, 0 units examined, 0 findings, 0 skipped
exit 0
$ cmake -P cmake/spell.cmake
Used config files:
    1: .codespellrc
exit 0
```

Tree mode takes its candidate list from `git ls-files`
(`tools/prose/prose_gate.py:597` to `:599`). This record is unstaged
while the driver holds the commit, so that run examines 0 sources. The
same rule engine was driven over this file by `/tmp/dropping-prose.py`,
which imports `prose_gate`, hands `evaluate_prose` the one path in place
of the candidate list, and prints the same summary line:

```text
$ cd /home/archerc/code/speedgun-ng && python3 /tmp/dropping-prose.py
prose-lint: 1 sources, 223 units examined, 0 findings, 0 skipped
exit 0
```

The spell gate walks the working tree, so the record is inside its scope
and it reports no hit.

VERDICT: PASS
