# Quickstart: Benchmark Harness Core

Phase 1 output. Runnable validation for `specs/015-benchmark-harness-core/`.
The contract details live in [contracts/benchmark-api.md](contracts/benchmark-api.md),
[contracts/cli.md](contracts/cli.md), and
[contracts/time-source-gate.md](contracts/time-source-gate.md); the
entities live in [data-model.md](data-model.md).

## Prerequisites

Linux on GCC or Clang, CMake ≥ 3.20, and the repository submodules in
their merged state (PC-1). Tests run unprivileged at
`perf_event_paranoid` 2 (FR-054).

## 1. Build and test the dev loop

```bash
cmake --preset=dev
cmake --build --preset=dev
ctest --preset=dev
```

Expected: every test passes, including the six `harness_*` suites, the
`time_source_gate` scan, and the `barrier_shape` codegen gate.

## 2. Run the example suite (US1, SC-008)

```bash
cmake --build --preset=dev --target benchmark_example
./build/dev/example/benchmark_example
```

Expected: context lines with the version and build type, then one row
with real time per iteration in ns, the overhead floor in ns, and the
`instructions / cycles` metric with its running ratio, scaled flag,
and gap state. On a host without a countable `cpu/instructions` leaf,
the row reports the metric unavailable with its refusal kind and the
exit status is zero.

```bash
echo $?
```

Expected: `0`.

## 3. Run control (US2, SC-007)

```bash
./build/dev/example/benchmark_example --filter 'BM_.*' --list   # names only, exit 0
./build/dev/example/benchmark_example --dry-run                 # one iteration, one repetition
./build/dev/example/benchmark_example --iterations 100          # calibration skipped
./build/dev/example/benchmark_example --repetitions 5           # five rows plus aggregates
./build/dev/example/benchmark_example --min-time 0              # recoverable error, exit nonzero
```

Expected: the list line prints the matching names and runs nothing;
the dry run reports one iteration; the aggregates rows carry mean,
median, standard deviation, coefficient of variation, min, and max;
the invalid `--min-time` reports a recoverable error, runs no
benchmark, and exits nonzero.

## 4. Catalog listing (US3, SC-009)

```bash
./build/dev/example/benchmark_example --catalog
```

Expected: one line per published counter with the object path, the
name, the description, the unit, the read mode, and the availability
state with its refusal kind; the machine clock leaves appear, the
`machine/tsc` leaf appears on x86 builds alone; no benchmark function
runs; the exit status is zero.

## 5. Exact values on scripted time (SC-001, SC-002, SC-014)

```bash
ctest --preset=dev -R harness_calibration_test --output-on-failure
ctest --preset=dev -R harness_capture_test --output-on-failure
```

Expected: the scripted per-iteration counts equal the scripted delta
divided by N; the calibration walk matches the iteration count of
every step and the step that qualifies; each reported value equals the
value the scripted deltas and the plan produce. The tests drive the
fake provider through the substitution of R-03.

## 6. Failure isolation (US4, SC-006)

```bash
ctest --preset=dev -R harness_cli_test --output-on-failure
```

Expected: the throwing suite leaves the process descriptor count and
mapping count at their start values across 1,000 runs, and the next
benchmark runs and reports. An interrupted suite exits nonzero with the
benchmark skipped (SC-017).

## 7. The time-source gate (SC-013)

```bash
ctest --preset=dev -R time_source_gate          # clean tree: exit 0
```

Plant a banned call, confirm the failure, remove it, confirm the pass:

```bash
cp source/harness/runner.cpp /tmp/runner.cpp.bak
printf '\n#include <chrono>\nauto probe() -> long { return std::chrono::steady_clock::now().time_since_epoch().count(); }\n' >> source/harness/runner.cpp
ctest --preset=dev -R time_source_gate          # planted: exit 1, file:line printed
cp /tmp/runner.cpp.bak source/harness/runner.cpp
ctest --preset=dev -R time_source_gate          # removed: exit 0
```

Expected: the planted run fails with the `file:line` printed; the
removal returns the gate to a pass. Record both runs for SC-013. The
clean scan covers `tools/` and allows the one D-6 exception.

Plant `std::chrono::system_clock` in `tools/dbc/overhead.cpp`, run the
gate (expect exit 1), remove it, and run the gate again (expect exit 0).
Record that pair too (SC-013).

## 8. Barrier codegen gate (US6)

```bash
ctest --preset=dev -R barrier_shape --output-on-failure
```

Expected: the -O2 build from GCC and from Clang keeps the measured work
when the benchmark calls `doNotOptimize`, and eliminates it without the
barrier.

## 9. Release configuration and the downstream consumer (IX, SC-010, SC-012)

```bash
cmake --preset=ci-ubuntu
cmake --build build
ctest --test-dir build
```

Expected: the release preset builds clean, the suite passes, the
downstream-consumer job builds and runs a suite against the installed
package, and the installed package reports version 0.6.0 with
`SOVERSION` 2. A consumer that requests 0.5 rejects the package
(`SameMinorVersion`).

## 10. Constitution state (SC-016)

```bash
grep -n 'counters library' .specify/memory/constitution.md | head
grep -n '2.18.0' .specify/memory/constitution.md
```

Expected: the Additional Constraints entry states the counters-only
rule, the banned list, the counters-change route, and the
amendment-only exception route; the Principle VIII gate list names the
time-source gate; the lineage table ends at 2.18.0 and the footer
equals it.
