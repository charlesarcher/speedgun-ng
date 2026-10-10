# Quickstart: Harness Timing Modes

Phase 1 output. Runnable validation for
`specs/017-harness-timing-modes/`. The requirements live in
[spec.md](spec.md); the design and the test plan live in [plan.md](plan.md);
the field and record rules live in [data-model.md](data-model.md).

## Prerequisites

Linux on GCC or Clang, and CMake 3.20 or newer (`CMakeLists.txt:1`). The
committed presets are `ci-ubuntu` to `build`, `ci-sanitize` to
`build/sanitize`, `ci-tsan` to `build/tsan`, and `ci-coverage` to
`build/coverage` (`CMakePresets.json:135,110-111,119-121,106,93-94`). The
`dev` preset is machine-local and never committed, so no scenario assumes
it.

`ci-ubuntu` inherits `dev-mode`, which sets `speedgun-ng_DEVELOPER_MODE` to
`ON` (`CMakePresets.json:25-30`), and `BUILD_EXAMPLES` follows that variable
(`CMakeLists.txt:866`), so the example lands at
`build/example/benchmark_example`. The codegen gates compile with `g++` and
`clang++` and skip an absent one (`test/loop_shape.sh:237-238`).

Every timing scenario below runs inside a test executable on the counters
fake provider, so no scenario needs a PMU, a root shell, or a wall-clock
bound. The scripted `machine/monotonic`, `machine/thread_cpu`, and
`machine/process_cpu` sequences make each reported figure hand-computable,
the pattern `test/source/harness_calibration_test.cpp:226-238` sets.

## 1. Build and test (SC-006, SC-007)

```bash
cmake --preset=ci-ubuntu                                # configure into build
cmake --build build                                     # library, tests, example
ctest --test-dir build -R harness --output-on-failure   # every harness_* suite
```

Expected: CTest reports every match passed. The six new `harness_*` suites of
the plan's Test Plan run beside the fourteen of features 015 and 016
(`test/CMakeLists.txt:476-643`), and the `time_source_gate`, `barrier_shape`,
`loop_shape`, and `cli_shape` scans stay green.

```bash
grep -n 'VERSION 0.7.0' CMakeLists.txt   # the project() line, CMakeLists.txt:7
grep -n 'SOVERSION 3' CMakeLists.txt     # the harness archive, CMakeLists.txt:219
```

Expected: one line each before this feature lands. After it, the project
version reads `0.8.0` and the harness archive reads `SOVERSION 4`, and the
counters archive keeps `SOVERSION 2` (`CMakeLists.txt:47`, FR-022).

## 2. C-1 the two time columns (FR-001, FR-002, FR-003, FR-013, SC-001, SC-004)

```bash
ctest --test-dir build -R harness_time_columns_test --output-on-failure
```

Expected: CTest reports the suite passed. Each measured row prints
`real time/iter (ns)=` and `cpu time/iter (ns)=`, each exact to the scripted
delta over the iteration count, the CPU column follows
`machine/thread_cpu`, and the row prints `endpoint samples=2` for a run with
no pause (FR-001, FR-003, FR-013). The aggregate line reads
`primary time/iter (ns) mean=`, and its statistics summarize the CPU
selection (FR-002).

## 3. C-2 the process CPU leaf (FR-004, SC-001)

```bash
ctest --test-dir build -R harness_cpu_leaf_test --output-on-failure
```

Expected: CTest reports the suite passed. `measureProcessCpuTime` moves the
CPU column to the scripted `machine/process_cpu` fold, the switch combines
with `useRealTime` and with `useManualTime`, and a provider that publishes no
`machine/process_cpu` leaves that benchmark failed with the resolution line
the runner already prints for a missing leaf (FR-004,
`source/harness/runner.cpp:176-187`).

## 4. C-3 real time as the primary time (FR-005, SC-001)

```bash
ctest --test-dir build -R harness_real_time_test --output-on-failure
```

Expected: CTest reports the suite passed. The scripted sequence makes the
growth rule and the qualification test follow the monotonic fold, so the run
count of the walk differs from the CPU-driven walk of
`test/source/harness_calibration_test.cpp:276-304`, and the aggregate line
summarizes the real column (FR-005).

## 5. C-4 manual time (FR-006, FR-007, SC-001)

```bash
ctest --test-dir build -R harness_manual_time_test --output-on-failure
```

Expected: CTest reports the suite passed. A benchmark that calls
`setIterationTime` once per iteration reports that sum over N in the real
column, the growth rule reads the accepted sum, and the fivefold stop stays
exempt (FR-006, FR-007). A benchmark that selects manual time and accepts
nothing reports `0.000` in the real column and qualifies at the iteration cap
(spec Edge Cases). `useRealTime` together with `useManualTime` ends the
re-exec of the test binary, and the parent reads the signal, the pattern of
`test/source/harness_callback_test.cpp:593-621` (FR-008).

## 6. C-5 pause and resume (FR-009 to FR-012, SC-002, SC-004)

```bash
ctest --test-dir build -R harness_pause_test --output-on-failure
```

Expected: CTest reports the suite passed. With one paused interval, each
column equals the window fold minus that interval's fold to the scripted
nanosecond, an attached metric column carries the same subtraction, the row
prints `endpoint samples=4`, and a `ScopedPauseTiming` region reports the
same value as the explicit pair (FR-009, FR-010, FR-012). A second
`pauseTiming` before the resume, a `resumeTiming` with no open pause, an open
pause at loop exit, a pause in a setup callback, and a ninth interval in one
run each end the re-exec through the contract facility (FR-011, FR-014).

## 7. C-6 the time unit (FR-015, FR-016, SC-001)

```bash
ctest --test-dir build -R harness_time_unit_test --output-on-failure
```

Expected: CTest reports the suite passed. `unit(TimeUnit::MILLISECONDS)`
prints both columns, the overhead floor, and the aggregate line scaled by
1000000 with the `ms` label, `setDefaultTimeUnit` scales a benchmark that
sets none, and a default set after a registration leaves that benchmark at
the unit it captured (FR-015, FR-016, spec Edge Cases).

## 8. C-7 examples and documentation (FR-017)

```bash
cmake --build build --target benchmark_example   # example/CMakeLists.txt:15
./build/example/benchmark_example --dry-run      # one iteration, one repetition
echo $?                                          # the exit status of that run
```

Expected: the report prints one row each for the process-CPU benchmark, the
real-time benchmark, the manual-time benchmark, the paused benchmark, and the
millisecond benchmark, and the status line reads `0`. The paused row prints
`endpoint samples=4`, and the millisecond row carries the `ms` label. Review
`docs/pages/harness.md` for one section per capability C-1 to C-7 beside the
ten sections of feature 016.

## 9. The loop-shape and time-source gates (FR-003, FR-021)

```bash
ctest --test-dir build -R loop_shape --output-on-failure
ctest --test-dir build -R time_source_gate --output-on-failure
```

Expected: both report passed. The loop gate takes the backward jump with the
shortest span in the function (`test/loop_shape.sh:77-113`) and fails on an
out-of-line `call` inside that body (`:190`) or on a body longer than the
count-down reference (`:210`), so the pause checks must stand in
`pauseTiming`, `resumeTiming`, and the cold `closeWindow()`
(`include/speedgun-ng/benchmark.hpp:448`). The time-source gate scans every
C++ source and header outside the counters library, so the new code reaches
time through `sg::counters` alone (FR-021).

## 10. The contract pair gate and the prose gate (FR-020, FR-025)

```bash
cmake --build build -t dbc-gate       # cmake/dbc-gate.cmake:41
cmake -P cmake/prose-lint.cmake       # range mode from the merge base (cmake/prose-lint.cmake:16)
cmake -P cmake/spell.cmake
```

Expected: each exits zero and prints no finding. The pair gate reports a
documented clause with no matching `SG_*` enforcement inside the interface
scope, so each new `\pre` and `\post` block needs its enforcement in the same
scope, and a class-level `\invariant` needs its enforcement in the members
(FR-020). The prose gate reads `tools/prose/prose_rules.yaml`, and CI runs the
same scripts (`.github/workflows/ci.yml`). A finding in this feature's prose
is a defect at lint parity (Principle XI.6, FR-025).

## 11. Coverage and sanitizers (FR-023, SC-006)

```bash
cmake --preset=ci-sanitize && cmake --build build/sanitize
ctest --test-dir build/sanitize --output-on-failure
cmake --preset=ci-tsan && cmake --build build/tsan
ctest --test-dir build/tsan --output-on-failure
cmake --preset=ci-coverage && cmake --build build/coverage
cmake --build build/coverage -t coverage
```

Expected: every suite passes under the address, undefined, and thread
sanitizers, and the coverage report shows 100% line and branch coverage over
`include/speedgun-ng/*` and `source/*` with no `LCOV_EXCL` marker under
`source/harness/`. The coverage extract omits the contract macro lines
(`cmake/coverage.cmake:76`), so the new `SG_REQUIRE` and `SG_ASSERT` lines
need no test of their own while every selection and pause branch does (FR-023).
