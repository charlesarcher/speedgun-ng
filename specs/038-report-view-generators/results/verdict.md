# The gate verdict of specs/038

- Branch: `038-report-view-generators`, the tree the records stand at.
- Date: 2026-10-29, the lane host thrawn, the release build at
  `build/`, the coverage build at `build/coverage`.

## The reads

| Gate | Command | Verdict |
| --- | --- | --- |
| The suite | `ctest --test-dir build -j 24` | 100% tests passed out of 147 |
| Coverage | `cmake --build build/coverage --target coverage` | lines 100.0% (13898), branches 98.6%, the shortfall 19 rows within the D-14 recorded allowance of 19, no `LCOV_EXCL` marker and no new `--ignore-errors` entry; the dbc-gate reads the DBC leg at 318 interfaces, 0 gaps |
| Sanitizers | `ctest --test-dir build/sanitize -j 24` | 100% tests passed out of 142 |
| Thread sanitizer | `ctest --test-dir build/tsan -j 16` | 100% tests passed out of 143 |
| The analyzer build | `cmake --preset=ci-ubuntu && cmake --build build -j 32` | the build exits 0; the new translation units carry the standing test idiom's findings, the same categories the standing files carry at `test/source/harness_noise_test.cpp` (232 readings on a forced rebuild), so the readings stand baseline parity; the host's analyzer reads 23.1.3, newer than the pin the CI configuration carries |
| The two-cpu run | `taskset -c 0,1 ctest --test-dir build -j 2` | 100% tests passed out of 147, 192.75 seconds |
| The prose gate | `cmake -P cmake/prose-lint.cmake` | 0 findings |
| The spell gate | `cmake --build build --target spell-check` | 0 findings |
| The format gate | `cmake --build build --target format-check` | 0 findings under the pinned formatter; the standing files the host's newer formatter reads divergent stand the recorded version split of specs/009 T033, no file of this lane among them |
| The dbc gate | `cmake --build build --target dbc-gate` | 318 interfaces, 0 gaps |
| The harness CLI byte identity | `ctest --test-dir build -R harness_cli_table_test` | green; no option row joined, the value block `298` to `319` free |

The coverage readings stand the trace of the whole suite at
`build/coverage/coverage.info`; the shortfall's accepted classes stand
the counters-side rows the D-14 record names, and no view file stands
among them after the resolver's edge rows landed.
