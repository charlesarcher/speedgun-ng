# Contract: the time-source gate (SC-013)

Phase 1 output. The gate the D-6 amendment adds to Principle VIII as a
hard gate, delivered by `test/time_source_gate.sh` and registered in
CTest (R-07).

## Scanned set

Every C++ source and header the D-6 scope names: `source/`,
`include/`, `example/`, `test/`, and `tools/`.

The counters library sits outside the rule: `source/counters/`, the
`include/speedgun-ng/counters*.hpp` headers, and the `counters_` tests
and gate scripts under `test/`. Code under `external/` is outside the
rule. The counters library keeps its own clock reads (FR-040).

## Banned list (the constitutional list of D-6, FR-040)

| Group | Terms |
| --- | --- |
| `std::chrono` clocks | `std::chrono::system_clock`, `std::chrono::steady_clock`, `std::chrono::high_resolution_clock`, and the `<chrono>` include |
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
| a planted `std::chrono::system_clock` in `tools/dbc/overhead.cpp` | exit 1, with the `file:line` printed |
| that plant removed | exit 0 |

SC-013 records both runs: the failing planted run and the passing
removal run. The quickstart names the commands.

## CI binding

The script is registered in CTest, so every job that runs `ctest`
runs the gate, and the `test` job's ctest step makes a hit fail the
job. The D-6 amendment names the gate in the Principle VIII gate list,
which is the governance route Principle VIII requires for a gate-set
change (FR-044).

## Exception route

One allowance, named by D-6: `tools/dbc/overhead.cpp` may hold the
`<chrono>` include and `std::chrono::steady_clock`. It measures the
contract overhead against an independent, well-known reference clock on
purpose. Any other banned term in that file is a hit. Any other
exception needs a constitutional amendment. A spec, a plan, a local
override, or a suppression comment creates none (D-6).
