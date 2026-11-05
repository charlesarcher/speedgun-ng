# Gate sweep: F1-007 (specs/045-merge-fixes)

Head: `5b4a3df8` on `045-merge-fixes`. Host: thrawn, kernel `7.3.0-5-cachyos`,
gcc `16.2.1 20260810`, cmake `4.4.4`, lcov `2.6-0`, 48 cpus. Date: 2026-11-04.

| Gate | Command | Result |
| --- | --- | --- |
| format and lint | `cmake -P cmake/lint.cmake` | exit 0 |
| spell | `cmake -P cmake/spell.cmake` | exit 0 |
| example drift | `python3 tools/docs/example_drift.py .` | exit 0 |
| prose | `python3 tools/prose/prose_gate.py --base origin/master` | 69 sources, 4426 units, 0 findings |
| DBC doc-gate | `cmake --build build -t dbc-gate` | 345 interfaces, 0 gaps |
| DBC pair-gate | same target | 345 interfaces, 0 gaps |
| markers | `git grep -c LCOV_EXCL -- source include` | 84 |
| dev suite | `ctest --test-dir build --output-on-failure -j 24` | 178 of 178 |
| two-cpu suite | `taskset -c 0,1 ctest --test-dir build --output-on-failure` | 178 of 178, load 3.36 before |
| ASan and UBSan | `ctest --test-dir build/sanitize --output-on-failure -j 24` | 173 of 173, no `AddressSanitizer` line, no `runtime error` line |
| tsan | `ctest --test-dir build/tsan --output-on-failure -j 24` | 174 of 174, zero `WARNING: ThreadSanitizer` lines, load 3.70 before and 2.97 after |
| coverage | `ci-coverage`, launcher empty, `ctest -j 4`, `coverage` target | lines 100.0% (17653 of 17653), functions 98.4% (1832 of 1862), branches 99.4% (8699 of 8751) |

## Conditions

The coverage tree stands configured with `CMAKE_CXX_COMPILER_LAUNCHER` and
`CMAKE_C_COMPILER_LAUNCHER` empty, because ccache fronting gcov 16.2.1 leaves
`table_parse.cpp.gcda` with a stamp that mismatches its notes. Every `.gcda`
stands deleted before the suite. The host carries `~/.lcovrc` with
`check_data_consistency = 0`. None of these measures stands in the repository.

The coverage suite ran at a 1-minute load of 3.83 falling to 1.07. The two-cpu
suite ran at 3.36. The tsan suite ran at 3.70 before and 2.97 after.

## One reading to note

The first tsan pass, run while the host stood at a 1-minute load of 22.8, ended
with one entry failed out of 174. The command kept only the summary line, so the
entry name stands uncaptured. The re-run at a quiet host read 174 of 174 with
zero race reports. The loaded reading stands recorded here and no code fault
stands claimed for it.

## Branch shortfall

The gate passes the line check and fails the branch check: `branch shortfall 52
rows exceeds the D-14 recorded allowance of 21`. Raw `BRDA` rows with a zero
count in the trace read 342; lcov folds them to 52. The ten files with the most
zero-count rows: `cli.cpp` 33, `json_load.cpp` 30, `jitter_probe.cpp` 28,
`options.cpp` 28, `runner.cpp` 28, `topdown_select.cpp` 25, `group_layout.cpp`
17, `report.cpp` 16, `noise_tracer.cpp` 15, `table_parse.cpp` 11. The gate's own
sizing note states that the allowance counts the union of the CI coverage job,
where every entry runs to its end, and that a developer host reads higher
because an entry there can stop early. The number stands reported for the owner.
No exclusion, no gate change and no allowance change stands in this lane.
