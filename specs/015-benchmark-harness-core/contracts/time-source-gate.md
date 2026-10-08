# Contract: the time-source gate (SC-013)

Phase 1 output. The gate the D-6 amendment adds to Principle VIII as a
hard gate, delivered by `test/time_source_gate.sh` and registered in
CTest (R-07).

## Scanned set

- every harness source under `source/harness/`,
- the harness public headers `include/speedgun-ng/benchmark.hpp` and
  `include/speedgun-ng/barrier.hpp`,
- the example suite `example/benchmark_example.cpp`,
- `speedgunMain`, which lives in `source/harness/cli.cpp`.

The counters library under `source/counters/` keeps its own clock
reads and stays outside the scan (FR-040).

## Banned list (the constitutional list of D-6, FR-040)

| Group | Terms |
| --- | --- |
| `std::chrono` clocks | `std::chrono::system_clock`, `std::chrono::steady_clock`, `std::chrono::high_resolution_clock`, `std::chrono::floor`, and the `<chrono>` include |
| C time entry points | `std::clock`, `std::time`, `timespec_get`, `clock_gettime`, `clock_getres`, `gettimeofday`, `time(`, `times(`, `getrusage` |
| time-stamp instructions | `rdtsc`, `rdtscp`, `__rdtsc`, `__rdtscp`, and the headers that declare them (`<x86intrin.h>`, `<immintrin.h>`, `<ia32intrin.h>`) |

A term matches as a substring, so a qualified or intrinsics spelling
the table names is a hit. The scan follows the matching discipline of
`test/counters_header_purity.sh`: substring terms, printed
`file:line` for each hit, exit 1 on any hit, exit 0 on a clean scan.

## Verdicts

| Scenario | Expected |
| --- | --- |
| clean tree | exit 0 |
| a planted `std::chrono::steady_clock::now()` in a harness source | exit 1, with the `file:line` printed |
| the planted call removed | exit 0 |

SC-013 records both runs: the failing planted run and the passing
removal run. The quickstart names the commands.

## CI binding

The script is registered in CTest, so every job that runs `ctest`
runs the gate, and the `test` job's ctest step makes a hit fail the
job. The D-6 amendment names the gate in the Principle VIII gate list,
which is the governance route Principle VIII requires for a gate-set
change (FR-044).

## Exception route

None inside the scan. An exception needs a constitutional amendment;
a spec, a plan, a local override, or a suppression comment creates
none (D-6).
