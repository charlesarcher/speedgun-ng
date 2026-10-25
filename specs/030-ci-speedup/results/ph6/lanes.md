# Phase 6 Lane Registration

**Surface**: the five lanes of `.github/workflows/ci.yml` that run
`ctest`, each named by the configure preset its Configure step runs,
and the five `dbc_matrix` entries a `ctest -N` listing shows in that
lane's own tree.
**Head sha**: 680356a3be5e523454ef64eb35ae5f8860762533
**Run id**: local
**Command**: one configure line and two listing lines per lane under
Readings, each run from the repository root
**Numbers**: the table of configure exits, total entry counts and matrix
entry counts under Readings

T033 registers five entries under the `if(SPEEDGUN_DBC_MATRIX)` guard at
`test/CMakeLists.txt:158`. T034 sets that key in three presets and
leaves it out of two. FR-023 and FR-024 ask what each lane's own tree
lists, so this record reads each lane's tree on its own.

## Tree reuse

No existing tree could serve. A grep for `preset` over every
`CMakeCache.txt` under `build/` printed nothing: CMake records no preset
name in a cache, so no cache names the preset a reader needs. Each lane
got its own tree, configured by the preset that lane runs.

## Readings

Five configures, each into a directory of its own, each exit 0:

```sh
cmake --preset=ci-ubuntu-test -B build/lanes-ci-ubuntu-test
cmake --preset=ci-rocky -B build/lanes-ci-rocky
cmake --preset=ci-tsan -B build/lanes-ci-tsan
cmake --preset=ci-coverage -B build/lanes-ci-coverage
cmake --preset=ci-sanitize -B build/lanes-ci-sanitize
```

```text
CFGDONE ci-coverage exit=0
CFGDONE ci-sanitize exit=0
CFGDONE ci-tsan exit=0
CFGDONE ci-ubuntu-test exit=0
CFGDONE ci-rocky exit=0
```

Configure needs no build, so no compile ran for this record. Then two
listings per tree, one for every entry and one filtered to the matrix
names. Both exit 0 in all ten runs:

```sh
ctest --test-dir build/lanes-ci-ubuntu-test -N
ctest --test-dir build/lanes-ci-ubuntu-test -N -R dbc_matrix
ctest --test-dir build/lanes-ci-rocky -N
ctest --test-dir build/lanes-ci-rocky -N -R dbc_matrix
ctest --test-dir build/lanes-ci-tsan -N
ctest --test-dir build/lanes-ci-tsan -N -R dbc_matrix
ctest --test-dir build/lanes-ci-coverage -N
ctest --test-dir build/lanes-ci-coverage -N -R dbc_matrix
ctest --test-dir build/lanes-ci-sanitize -N
ctest --test-dir build/lanes-ci-sanitize -N -R dbc_matrix
```

| Lane | CI job | Configure preset | Configure exit | Total entries | Matrix entries |
| --- | --- | --- | --- | --- | --- |
| ubuntu test | `test` | `ci-ubuntu-test` | 0 | 139 | 5 |
| rocky test | `test-rocky` | `ci-rocky` | 0 | 139 | 5 |
| thread sanitizer | `tsan` | `ci-tsan` | 0 | 135 | 5 |
| coverage | `coverage` | `ci-coverage` | 0 | 134 | 0 |
| sanitizer | `sanitize` | `ci-sanitize` | 0 | 134 | 0 |

The three registering trees print the same five names, numbered 21 to 25
in every one of them, each under `Total Tests: 5`. The names are
`dbc_matrix_off_o0`, `dbc_matrix_off_o2`, `dbc_matrix_on_o0`,
`dbc_matrix_on_o2` and `dbc_matrix_observe_off_o0`. The two dropping
trees print no name line and `Total Tests: 0`.

The listing files sit at `build/lanes-<preset>-list.log` and
`build/lanes-<preset>-matrix.log`, and the configure logs at
`build/lanes-<preset>-configure.log`.

`git status` shows no worktree difference for `test/CMakeLists.txt` or
`CMakePresets.json` at the head above, so the trees hold the
registration of `bcc16bf` and the preset keys of `489480f`.

Two further counts fall out of comparing the name lists, and both belong
to T037. The `ci-coverage` list equals the `ci-sanitize` list entry for
entry, and the `ci-rocky` list equals the `ci-ubuntu-test` list entry
for entry. The `ci-tsan` tree holds the five matrix entries and 130
other entries: `counters_noalloc_test`, `harness_argument_test`,
`harness_capture_test` and `harness_jitter_probe_alloc_test` are absent
there and present in the other four trees, which is a registration gate
under the thread sanitizer and sits outside this task.

## Verdict

PASS. All five lanes configure with exit 0, the three lanes whose preset
sets `SPEEDGUN_DBC_MATRIX` list all five `dbc_matrix` entries, and the
two lanes whose preset omits that key list none, which is the reading
FR-023 and FR-024 ask for and the split P-01 and D-5 state.
