# Contract: Preset Graph

**Surface**: `CMakePresets.json`, the configure presets a CI job or a
lane host configures, the test presets a job or a script runs, and the
cache variables each carries. **Closes**: FR-006 to FR-013, FR-023,
FR-028.

## Configure presets

| Preset | Inherits | Carries | Used by |
| --- | --- | --- | --- |
| `ci-cache` | `dev-mode` | the two launcher cache variables set to `ccache` (P-02) | inherited, never configured directly |
| `ci-ubuntu` | `ci-build`, `ci-linux`, `clang-tidy`, `cppcheck`, `dev-mode`, `ci-cache` | the analyzers | `static-analysis`, `dbc-gate`, `prose-lint` |
| `ci-rocky` | `ci-build`, `ci-linux`, `dev-mode`, `ci-cache`, `SPEEDGUN_DBC_MATRIX=ON` | the rocky lane build | `test-rocky` |
| `ci-ubuntu-test` | `ci-build`, `ci-linux`, `dev-mode`, `ci-cache`, `SPEEDGUN_DBC_MATRIX=ON` | the analyzer-free Ubuntu build | `test` |
| `ci-sanitize` | `ci-linux`, `dev-mode`, `ci-cache` | sanitizer flags | `sanitize` |
| `ci-tsan` | `ci-linux`, `dev-mode`, `ci-cache`, `SPEEDGUN_DBC_MATRIX=ON` | thread-sanitizer flags | `tsan` |
| `ci-coverage` | `coverage-linux`, `dev-mode`, `ci-cache` | instrumentation flags | `coverage` |
| `prepush` | `ci-build`, `dev-mode`, `ci-cache`, `binaryDir` `${sourceDir}/build/prepush` | the local check build (P-06) | `tools/ci/prepush.sh` |

`ci-ubuntu` keeps the inheritance master has, because `dbc-gate`
configures it at `ci.yml:689` and `prose-lint` at `ci.yml:752` (FR-013,
D-3). `ci-cache` is abstract the way `dev-mode` and `ci-linux` are: it
names no generator and no `binaryDir`, so `cmake --preset=ci-cache`
cannot configure.

## Test presets

| Preset | Configures | Filter | `noTestsAction` |
| --- | --- | --- | --- |
| `prepush-quick` | `prepush` | label `quick` | `error` |
| `prepush-full` | `prepush` | none | `error` |
| `ci-sanitize` | `ci-sanitize` | none, unchanged | `error` |
| `ci-tsan` | `ci-tsan` | none, unchanged | `error` |

The name `dev` stays unused in the committed file, because
`CMakeUserPresets.json` is git-ignored at `.gitignore:13` and carries a
`dev` preset on lane hosts, where a duplicate name is a CMake error
(FR-028, D-6).

## Cache variables the graph defines

| Variable | Where | Effect |
| --- | --- | --- |
| `CMAKE_C_COMPILER_LAUNCHER`, `CMAKE_CXX_COMPILER_LAUNCHER` | `ci-cache` | `ccache` fronts every compile of every CI and pre-push build (FR-006) |
| `SPEEDGUN_DBC_MATRIX` | `ci-rocky`, `ci-ubuntu-test`, `ci-tsan` | registers the five matrix entries in that lane (P-01) |
| `SPEEDGUN_TEST_JOBS` | default `2` in `test/CMakeLists.txt` | the compile concurrency one matrix entry may start (P-04) |

The `ccache` environment is a workflow concern and stays in the workflow:
`CCACHE_DIR`, `CCACHE_MAXSIZE` and the key of P-10 are set in `ci.yml`,
and `CCACHE_BASEDIR` and `CCACHE_SLOPPINESS` are set nowhere (FR-011).

## Lane selection, in one reading

| Lane | Preset | Compiler family | Matrix |
| --- | --- | --- | --- |
| `test` | `ci-ubuntu-test` | GCC 15.2.0 | five entries |
| `test-rocky` | `ci-rocky` | GCC 14.3.1 | five entries |
| `tsan` | `ci-tsan` | Clang 21.1.8 | five entries |
| `coverage` | `ci-coverage` | GCC 15.2.0, shared with `test` | none (D-5) |
| `sanitize` | `ci-sanitize` | Clang 21.1.8, shared with `tsan` | none (D-5) |

Each of the three registering lanes prints its compiler version from its
own configure log (FR-023). The two dropping lanes record the evidence of
FR-024: the matrix script configures with no preset, sets
`CMAKE_BUILD_TYPE=None` and its own `CMAKE_CXX_FLAGS`
(`tools/dbc/semantics_matrix.sh:181`, `:182`), reads only `CXX` (`:54`),
builds under `mktemp -d` (`:76`), and never adds a count to a coverage
trace.

## Verify

```sh
cmake --list-presets
cmake --list-presets=test
cmake --preset=ci-rocky && cmake --build build
cmake --preset=prepush && cmake --build build/prepush && ctest --preset prepush-quick
cmake --preset=ci-ubuntu-test
```

The configure list shows the three new names and no name `dev`, the test
list shows `prepush-quick` and `prepush-full`, and
`build/CMakeCache.txt` after a `ci-ubuntu-test` configure shows
`CMAKE_CXX_COMPILER_LAUNCHER:STRING=ccache` and no analyzer launcher.
