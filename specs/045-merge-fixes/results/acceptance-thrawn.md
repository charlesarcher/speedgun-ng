# Acceptance record: thrawn (specs/045-merge-fixes T202)

Head: `d214be37` on `045-merge-fixes`.
Host: thrawn, kernel `7.3.0-5-cachyos`, gcc `16.2.1 20260810`, cmake `4.4.4`,
lcov `2.6-0`, 48 cpus.
Date: 2026-11-03.

## 1. Full suite

`ctest --test-dir build --output-on-failure --no-tests=error -j 24`:
`100% tests passed out of 178`. One entry skipped:
`harness_topdown_run_test`, the host reason of its registration (one scenario
states no vendor this build vendors a table for).

## 2. Two-cpu mask

`taskset -c 0,1 ctest --test-dir build --output-on-failure`:
`100% tests passed out of 178`.

## 3. Coverage

The run needed two host-local measures, recorded here and nowhere in the
repository:

- `~/.lcovrc` with `check_data_consistency = 0`: lcov 2.6 calls the lambda
  record shape gcov 16.2.1 emits inconsistent.
- the coverage build with `CMAKE_CXX_COMPILER_LAUNCHER` and
  `CMAKE_C_COMPILER_LAUNCHER` empty: ccache fronting gcov 16 compiles leaves
  `table_parse.cpp.gcda` with a stamp that mismatches its notes. `cmake
  --target clean` removes the notes and leaves the runtime `.gcda`, so the
  counters also stand deleted before each suite run.

Figures at this head: lines `99.9% (17617 of 17618)`, functions
`98.3% (1829 of 1861)`, branches `98.9% (8611 of 8709)`. Dark lines by file:
`source/counters/linux_pmu/topdown_select.cpp` 1 (line 567).

That line is the first `return false` inside the `shapeHolds` guard of the
`SG_ENSURE` at 595. The metric layer refuses the second row of a duplicated
name before the arm can stand, and a hand-composed table that reaches it
aborts on the postcondition. No passing test executes the line, so FR-011's
stop-and-report rule applies and the line stands reported for the owner.

The branch figure is not comparable with the allowance of 21: gcov 16.2.1
counts 8709 branch positions on this host, where the allowance was
calibrated on the CI compiler's count. The binding branch figure comes from
the CI coverage job.

## 4. Marker count by file

`git grep -c LCOV_EXCL -- source include` reads 84:

| File | Markers |
| --- | --- |
| `source/counters/clock_provider.cpp` | 17 |
| `source/counters/plan.cpp` | 13 |
| `source/counters/system.cpp` | 12 |
| `source/counters/linux_pmu/group_io.cpp` | 11 |
| `source/counters/linux_pmu/fast_read.cpp` | 7 |
| `source/counters/fold.cpp` | 5 |
| `source/counters/linux_pmu/table_parse.cpp` | 4 |
| `source/counters/linux_pmu/provider.cpp` | 4 |
| `include/speedgun-ng/dbc.hpp` | 4 |
| `source/simulation/marker.cpp` | 2 |
| `source/counters/detail/core.hpp` | 2 |
| `include/speedgun-ng/counters_provider.hpp` | 2 |
| `source/counters/linux_pmu/encode.cpp` | 1 |

The kept list with one reason per marker stands in
`specs/045-merge-fixes/results/marker-ledger.md`.

## 5. Sanitizers and threads

`ctest --test-dir build/sanitize --output-on-failure -j 24`:
`100% tests passed out of 173`, no `AddressSanitizer` line and no
`runtime error` line.
`ctest --test-dir build/tsan --output-on-failure -j 24`:
`100% tests passed out of 174`, no `WARNING: ThreadSanitizer` line. The race
that ended `harness_jitter_cli_test` in the base tree stands gone.

## 6. The five tools of SC-005

`tools/prose/prose_gate.py --base origin/master`: `0 findings`.
`cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake`: exit 0, clang-format
`18.1.8`.
`cmake -P cmake/spell.cmake`: exit 0.
`tools/docs/example_drift.py`: exit 0, no difference printed.
`cmake --build build -t dbc-gate`: doc-gate 345 interfaces, 0 gaps; pair-gate
345 interfaces, 0 gaps.

## 7. Host-load artifact of the window entry

`harness_jitter_window_test` keeps a residual flake on this host while a
standing user process holds about six cpus: 6 failures in 20 solo runs of the
binary, on two assertions of the wall-bounded window. The full suite passed at
`-j 24` and at `-j 48`, the two-cpu suite passed, and five rounds of the jitter
entries under `taskset -c 0-3` with `-j 4` read 5 of 5. The entry stands
`RUN_SERIAL`, so no sibling test of the suite causes it. The coverage tree shows
the same sensitivity: one failure in a third full-suite run of an unchanged tree
(the two earlier runs of the same binaries read 173 of 173), and 2 failures in 5
standalone runs of the binary with the load average at 14.9 on 48 cpus. The dev,
sanitize, tsan and coverage trees all move on the assertions that bound a probe
window by wall time. The condition stands reported for the owner's ruling.
