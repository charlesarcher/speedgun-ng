# Quickstart: Harness Registration and Fixtures

Phase 1 output. Runnable validation for
`specs/016-harness-registration-and-fixtures/`. The requirements live in
[spec.md](spec.md); the design and the test plan live in [plan.md](plan.md).

## Prerequisites

Linux on GCC or Clang, and CMake 3.20 or newer (`CMakeLists.txt:1`). The
committed presets are `ci-ubuntu` to `build`, `ci-sanitize` to
`build/sanitize`, `ci-tsan` to `build/tsan`, and `ci-coverage` to
`build/coverage` (`CMakePresets.json:93-94,110-111,119-121,130-137`). The
`dev` preset is machine-local and never committed, so no scenario assumes it.

`ci-ubuntu` inherits `dev-mode`, which sets `speedgun-ng_DEVELOPER_MODE` to
`ON` (`CMakePresets.json:25-30`), and `BUILD_EXAMPLES` follows that variable
(`CMakeLists.txt:865-867`), so the example lands at
`build/example/benchmark_example`. The codegen gates compile with `g++` and
`clang++` and skip an absent one (`test/loop_shape.sh:237-238`).

## 1. Build and test (SC-008, SC-009)

```bash
cmake --preset=ci-ubuntu                                # configure into build
cmake --build build                                     # library, tests, example
ctest --test-dir build -R harness --output-on-failure   # every harness_* suite
```

Expected: CTest reports every match passed. The new `harness_*` suites of the
plan's Test Plan run beside the eight of feature 015
(`test/CMakeLists.txt:476-570`), and the `time_source_gate`, `barrier_shape`,
and `cli_shape` scans stay green (`test/CMakeLists.txt:482-485,541-542,553-554`).

```bash
grep -n 'VERSION 0.7.0' CMakeLists.txt    # the project() line, CMakeLists.txt:7
grep -n 'SOVERSION 3' CMakeLists.txt      # the harness archive, CMakeLists.txt:211-218
```

Expected: one line each. The counters archive keeps `SOVERSION` 2 (FR-026, R-16).

## 2. C-1 argument families (FR-001 to FR-007, SC-001)

```bash
ctest --test-dir build -R harness_family_test --output-on-failure
```

Expected: CTest reports the suite passed. The instance set of every family call
matches the cited revision, `createRange` and `createDenseRange` feed a family
call with `kDefaultRangeMultiplier` as the default multiplier, and a family
above `kMaxFamilySize` = 100 instances draws one warning (FR-002, FR-007, R-14,
R-15).

## 3. C-2 instance names (FR-010 to FR-012, SC-003, SC-004)

```bash
ctest --test-dir build -R harness_instance_name_test --output-on-failure
```

Expected: CTest reports the suite passed. Each name carries one `/`-joined
segment per argument, an `argName` label prints that segment as `label:value`,
and a duplicate name prints one line on the standard error stream naming both
names while the earlier instance stays (FR-010, FR-012, R-12).

## 4. C-3 argument access (FR-008, SC-002)

```bash
ctest --test-dir build -R harness_argument_test --output-on-failure
```

Expected: CTest reports the suite passed. `range(index)` returns the argument of
the running instance, `rangeCount()` returns that count, and an instrumented
read allocates nothing (FR-008, R-04). A family with no family call reports a
count of zero, where `range(0)` is a precondition violation (FR-006).

## 5. C-4 capture (FR-013)

```bash
ctest --test-dir build -R harness_capture_macro_test --output-on-failure
```

Expected: CTest reports the suite passed.
`SG_BENCHMARK_CAPTURE(addTwo, pair, 2, 2)` runs one instance named
`addTwo/pair`, with the captured values reaching the function (FR-013, R-13).

## 6. C-5 templates (FR-014)

```bash
ctest --test-dir build -R harness_template_test --output-on-failure
```

Expected: CTest reports the suite passed.
`SG_BENCHMARK_TEMPLATE(sortOf, int, double)` runs one instance named
`sortOf<int, double>`, the type list stringified as written at the macro site
(FR-014, R-13).

## 7. C-6 fixtures (FR-015 to FR-017, SC-005)

```bash
ctest --test-dir build -R harness_fixture_test --output-on-failure
```

Expected: CTest reports the suite passed. The seven macros of FR-016 register,
each instance takes its fixture class as its suite, and the scripted clock
deltas show the pair adds nothing to the reported time. The pair runs once per
run, the warm-up and calibration runs included (FR-017, R-09).

## 8. C-7 setup and teardown callbacks (FR-018, FR-019, SC-005)

```bash
ctest --test-dir build -R harness_callback_test --output-on-failure
ctest --test-dir build -R compile_fail --output-on-failure   # test/compile-fail/run.sh:76
```

Expected: both report passed. The pair runs once around each run in the order
setup, `setUp`, callable, `tearDown`, teardown, and the last attachment wins in
each slot (R-10). Each negative translation unit in `test/compile-fail/` fails
to compile with the diagnostic its `// expect:` line names (R-05).

## 9. C-8 the `DISABLED_` prefix (FR-020, SC-006)

```bash
ctest --test-dir build -R harness_disabled_test --output-on-failure
```

Expected: CTest reports the suite passed. A family named `DISABLED_slow`
registers, runs no function, stays out of the filter match, and stays out of
list mode. A fixture method named `DISABLED_x` runs, because that instance name
starts with the fixture class (R-11).

## 10. C-9 suite and case naming (FR-021, SC-007)

```bash
ctest --test-dir build -R harness_instance_name_test --output-on-failure
```

Expected: CTest reports the suite passed. `BenchmarkResult` carries `suite` and
`caseName` for every instance, the suite order follows first registration, and
rows inside one suite keep registration order (R-07, R-08, R-12). The console
row keeps the single H1 name column.

## 11. C-10 examples and documentation (FR-027)

```bash
cmake --build build --target benchmark_example   # example/CMakeLists.txt:29
./build/example/benchmark_example --dry-run      # one iteration, one repetition
echo $?                                          # the exit status of that run
```

Expected: the report prints one row per instance of the argument family, one row
for the fixture method, and one row for the templated benchmark, and the status
line reads `0`. Review `docs/pages/harness.md` for one section per capability;
that page is new in this feature (`docs/pages/`).

## 12. The loop-shape gate (FR-009, R-06)

```bash
ctest --test-dir build -R loop_shape --output-on-failure
```

Expected: CTest reports the test passed, and the script prints no `FAIL` line
for any compiler present. The gate reads disassembly: it takes the backward jump
with the shortest span in the whole function (`test/loop_shape.sh:96-113`) and
fails on an out-of-line `call` inside that body (`:190`) or on a body longer
than the count-down reference (`:210`). A new contract check must not emit a
shorter backward jump than the timed loop, and `State::end()` stays static at
`include/speedgun-ng/benchmark.hpp:277` (R-05).

## 13. The prose gate (FR-030)

```bash
cmake -P cmake/prose-lint.cmake   # range mode from the merge base with origin/master (cmake/prose-lint.cmake:17)
```

Expected: the command exits zero and prints no finding. The gate reads
`tools/prose/prose_rules.yaml`, and CI runs the same script with an explicit
base and head (`.github/workflows/ci.yml:744`). A finding in this feature's
prose is a defect at lint parity (Principle XI.6, FR-030).
