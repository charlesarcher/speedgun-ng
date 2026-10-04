# Quickstart: Nanosecond Counter and Simulation-Start Marker

**Feature**: `specs/011-nanosecond-counter-ssc-mark/spec.md`
**Date**: 2026-10-03

A validation guide. Each section names a command, the expected outcome, and
what it proves. Nothing here restates the contracts; the marker byte
contract lives in [contracts/simulation-start.md](contracts/simulation-start.md)
and the catalog entry in
[contracts/monotonic-raw-counter.md](contracts/monotonic-raw-counter.md).

## Prerequisites

| Requirement | Detail |
|-------------|---------|
| Platform | Linux on x86-64. The platform is declared in the constitution and the marker emits its sequence under an x86 guard. |
| Build type | CMake 3.20 or newer, driven by presets. |
| Local preset | `CMakeUserPresets.json` supplies the `dev` preset. It is machine-local and never committed. |
| Toolchain | The compilers the project supports, plus binutils `objdump`, which the gate script invokes and the suite already invokes elsewhere. |
| Optional | Intel SDE, for the one local tracer confirmation at the end. Its absence affects no command in the sections before it. |

## Configure once

```sh
cmake --preset=dev
cmake --build --preset=dev
```

Expected: configure exits zero, the build compiles every target. The
developer preset builds unoptimized, which is the per-task loop.

## Counter lane

### The catalog publishes the leaf

```sh
ctest --preset=dev -R counters_clock_raw -V
```

Expected: exit zero, with named checks for the address resolving on the
machine root, the unit token reading `nanoseconds`, the read-mode label
reading `syscall`, consecutive samples never decreasing on one thread, the
cross-thread pass under a join, the smallest non-zero step staying within the
platform's reported resolution, and a cost distribution printed over ten
million reads.

Before the implementation lands, this entry fails with a named assertion
naming the unresolved address. That red is the point of writing the test
first.

### The overhead figures exist and are printed

```sh
ctest --preset=dev -R counters_overhead -V --repeat until-pass:3
```

Expected: exit zero, and the printed run carries a line for the new counter
in the same field order as the existing clock line, that is minimum, median,
and maximum per read. Compare the printed figures against the row added to
`docs/pages/counters-overhead.md`; that page states it is not a CI gate, so
this comparison is the check.

## Marker lane

### The declaration passes the contract gates

```sh
cmake --build build/dev -t dbc-gate
cmake --build build/dev -t format-check
```

Expected: the contract gate reports zero gaps across all interfaces, with
the new declaration counted among them, and the format check passes. The new
header declares the function and the tag constant with `\pre none` and
`\post none`, which the gate accepts as an explicitly empty contract.

### The marker unit compiles and the analyzer accepts it

```sh
cmake --build --preset=dev
```

Expected: exit zero, and the analyzer launchers report the new unit. The
statement carries one suppression whose comment names the check and the
reason on the same line; the `test` job's build log is where the report lands.

### The runtime properties hold

```sh
ctest --preset=dev -R simulation -V
```

Expected: exit zero, with the register bit-identical check passing for a
known value whose upper 32 bits are set, the repeated-call loop terminating,
and the tag constant equal to the documented value.

### The emitted bytes are the documented bytes

```sh
ctest --preset=dev -R simulation_mark_shape -V
```

Expected: exit zero. The output names each compiler it exercised and each
contract-enforcement setting, reports the marker window found exactly once,
and reports the planted wrong sequence as a detected failure.

To see the failure path yourself, break the sequence in a scratch copy of
the marker unit, run the entry, and confirm it exits non-zero naming the
count. Restore the sequence and confirm it passes again.

To see the skip path, run the script where no compiler targets x86, and
confirm it prints a skip line and exits zero.

## Vocabulary and prose

```sh
ctest --preset=dev -R counters_header_purity -V
cmake -P cmake/spell.cmake
cmake -P cmake/prose-lint.cmake
```

Expected: the purity scan reports clean over the widened header set, the
spell check exits zero, and the prose gate reports zero findings over the
feature's range. Plant a platform token in the new header in a scratch state
and confirm the purity scan fails, which proves the widened set bites.

## Coverage

```sh
cmake --preset=coverage-linux
cmake --build build/coverage -t coverage
```

Expected: the summary reports 100 percent line and 100 percent branch over
the measured set, which already includes every new file, and the target exits
zero. The two non-x86 arms carry exclusion markers in the project's existing
spelling, because no supported build can execute them.

## Release configuration

Once per feature, after the per-task loop:

```sh
cmake --preset=ci-ubuntu
cmake --build build
ctest --preset=dev
```

Expected: all three exit zero. The unoptimized developer build cannot report
a finding an optimizer's analysis produces, so a task closed only against it
stays open.

## Downstream consumer

```sh
cmake --install build --prefix "$PWD/prefix"
```

Then configure `test/consumer` against that prefix, following
`test/consumer/CMakeLists.txt`, build it, and run it.

Expected: configure, build, and run all succeed, the new header resolves
through the package's include path, and the consumer's link manifest names no
tracer library. The CI job `downstream-consumer` performs the same sequence
and audits the manifest, so the CI job is the gate and a local run is a
convenience.

## Local confirmations

Two success criteria need a measurement too slow or too specific for the
ordinary suite. The constitution requires tests to run in every CI job, so
these stay out of it, the same split the specification makes for the tracer
run.

### Rate agreement over sixty seconds

Sample both counters once per second for at least sixty seconds, then divide
the difference of the two elapsed intervals by the interval measured with
`machine/monotonic_raw`.

Expected: the two rates agree to within 20 parts per million. A result
outside that band indicates the measurement crossed a clocksource switch or a
suspend. Repeat the run. Research.md R-005 explains why the
two clocks drift apart at all: the operating system slews
`machine/monotonic` and leaves this counter at the hardware rate.

### Trace collection under Intel SDE

Place one `simulation_start()` call at the boundary of the measured region in
`example/counters_standalone_example.cpp`, build, then run the binary under
Intel SDE with the documented tag:

```sh
sde64 -start_ssc_mark FACE:repeat -- <binary> <args>
```

Expected: the trace's first collected instruction falls inside the marked
region. The tag comes from the public header's documented constant, written
as `FACE` with no prefix.

## Result summary

| Section | Command | Exit code |
|---------|---------|-----------|
| Counter leaf and properties | `ctest --preset=dev -R counters_clock_raw` | 0 |
| Overhead figures | `ctest --preset=dev -R counters_overhead -V` | 0 |
| Contract pairing | `cmake --build build/dev -t dbc-gate` | 0 |
| Marker runtime properties | `ctest --preset=dev -R simulation` | 0 |
| Marker byte fidelity | `ctest --preset=dev -R simulation_mark_shape` | 0 |
| Header vocabulary | `ctest --preset=dev -R counters_header_purity` | 0 |
| Format, spelling, prose | `format-check`, `cmake -P cmake/spell.cmake`, `cmake -P cmake/prose-lint.cmake` | 0 |
| Coverage | `cmake --build build/coverage -t coverage` | 0 |
| Release configuration | `cmake --preset=ci-ubuntu` then `cmake --build build` | 0 |
| Rate agreement, local | sixty-second sampling loop | within 20 ppm |
| Tracer start, local | `sde64 -start_ssc_mark FACE:repeat` | trace begins in the region |