# Implementation Plan: Benchmark Harness Core

**Branch**: `015-benchmark-harness-core` | **Date**: 2026-10-08 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/015-benchmark-harness-core/spec.md`

## Summary

The feature delivers the harness vertical slice: registration of a
benchmark function, a timed loop that samples once at its entry and
once at its exit, the Google Benchmark calibration rule read at the
recorded revision, metrics folded from the counters library,
statistics over repetitions, a `getopt_long` command line with a
catalog listing, and a fixed-column console report. The harness lives
in the `speedgun-ng` library as a new static archive target beside the
counters archive, and every time and counter value it measures or
reports comes from `sg::counters`. The feature amends the constitution
to 2.18.0 with the counters-only time rule and the time-source gate,
and releases project version 0.6.0 with `SOVERSION` 2.

## Technical Context

**Language/Version**: C++23, `CMAKE_CXX_EXTENSIONS=OFF`. The one
compiler extension is the extended-assembly barrier statement of D-4,
carried as a P2 under Principle I with the justification recorded in
research.md R-13, on the 011 precedent (FR-055).

**Primary Dependencies**: the `speedgun-ng` counters library as merged
after specs 012, 013, and 014 (PC-1), and the 007 fake provider at
`include/speedgun-ng/counters_fake.hpp` (FR-042). The standard library
and the POSIX platform remain the only other sources. No new runtime
dependency, no vendored library (FR-051).

**Storage**: none. The feature writes no file (spec Scope).

**Testing**: the existing CTest registration in `test/CMakeLists.txt`;
fake-provider suites for every exact value (FR-054), script gates for
the codegen and time-source checks.

**Target Platform**: Linux on GCC and Clang, x86-64 and aarch64. The
`machine/tsc` leaf is an x86 catalog fact with no API difference
(FR-039).

**Project Type**: library plus command-line executable: each suite
links the harness target and becomes one speedgun executable (D-1).

**Performance Goals**: the timed loop allocates nothing, takes no
lock, and performs exactly two sampling actions per run (FR-052,
SC-005). The calibration step count stays within the 95-step bound of
FR-016.

**Constraints**: single-threaded execution (Assumptions); the
iteration cap is 10^12 iterations; every harness decision reads a
counters fold (FR-038); tests run unprivileged at
`perf_event_paranoid` 2 (FR-054).

**Scale/Scope**: FR-001 through FR-055; two public headers, six new
library sources, one example suite, six test executables, two gate
scripts, one constitution amendment, one version bump.

## Constitution Check

*GATE: evaluated before Phase 0 research and re-evaluated after Phase 1
design.*

### Pre-research evaluation

| Principle | Gate | Verdict |
|-----------|------|---------|
| I | Standard-first | PASS with one recorded P2. C++23 throughout. The `doNotOptimize` barrier is one extended-assembly statement per overload (D-4); its P2 justification is recorded in research.md R-13, on the precedent of the 011 `__asm__` marker recorded in that plan's Constitution Check. No other non-standard construct enters. |
| II | Design By Contract | PASS. Every new public interface documents `\pre`, `\post`, and `\invariant` and enforces its contracts through the `dbc` macros (FR-053). The barrier functions carry a recorded deviation, in Complexity Tracking below, on the 011 precedent. |
| III | R-DCUT | PASS. spec, plan, research, data model, contracts, quickstart, then tasks. TDD mode is in use for the harness lanes and is recorded here per Principle III and FR-055. |
| IV | Documentation | PASS. The public headers carry doxygen with the contracts; a documentation page states the Principle X.2 barrier rule (FR-030); the example suite is the runnable documentation of the slice. |
| V | Style and formatting | PASS. New identifiers follow N-1 through N-12 (FR-047); the harness copies no old spelling (PC-10). New files fall inside the formatter's glob, so `format-check` covers them with no configuration change. |
| VI | Test-Backed Code and Coverage | PASS. Every capability arrives with its test (FR-054); exact-value tests run on the fake provider and stay deterministic. `cmake/coverage.cmake` already measures `include/speedgun-ng/*` and `source/*`, so the new files enter the 100% gates with no configuration change. |
| VII | Performance Discipline | PASS. The timed loop is the designated critical path, and SC-005 is its performance test case: two sampling actions and zero allocations over 10,000 runs. The report carries the distribution form of VII (FR-027), and the overhead floor rides every row (FR-026). The Open deferrals baseline entry stays untouched. |
| VIII | CI Quality Gates | PASS through the amendment route. The gate set grows by one item, the time-source gate of D-6, and Principle VIII names the constitution amendment as the route for a gate-set change. The amendment lands inside this feature (FR-043 to FR-045) and extends the V.2 macro-family entry for the build-written `SPEEDGUN_NG_BUILD_TYPE` (R-14). No gate is removed or weakened. |
| IX | Spec-driven development | PASS. The change touches public API and build configuration, so the full workflow runs. One release-configuration build with `cmake --preset=ci-ubuntu` then `cmake --build build` closes the feature. |
| X | Anti-slop discipline | PASS. The design carries no seam the spec does not name: timing modes, threads, JSON, and recorder reuse stay in the later roadmap specs named in the Out list. Assumptions are recorded in research.md with the candidates each decision displaced. |
| XI | Prose standards | PASS, verified mechanically by `prose-lint` over this feature's artifacts. |

**Gate weakening requested**: no. The gate set grows by one item
through the D-6 amendment, which is the change itself.

## Project Structure

### Documentation (this feature)

```text
specs/015-benchmark-harness-core/
├── spec.md                              # /speckit.specify output
├── plan.md                              # This file (/speckit.plan output)
├── research.md                          # Phase 0 output: R-01..R-14
├── data-model.md                        # Phase 1 output: entities and state transitions
├── quickstart.md                        # Phase 1 output: validation commands and outcomes
├── contracts/
│   ├── benchmark-api.md                 # Phase 1 output: the public C++ surface
│   ├── cli.md                           # Phase 1 output: options, report columns, exit statuses
│   └── time-source-gate.md              # Phase 1 output: the SC-013 gate contract
└── tasks.md                             # Phase 2 output (/speckit.tasks; not created here)
```

### Source Code (repository root)

Files this feature adds:

```text
include/speedgun-ng/benchmark.hpp        # public: registration, State, handle, result value
include/speedgun-ng/barrier.hpp          # public: doNotOptimize, clobberMemory
source/harness/registry.cpp              # the registry and the duplicate-name rule
source/harness/runner.cpp                # calibration, runs, capture, folds, statistics
source/harness/report.cpp                # the fixed-column console report
source/harness/catalog.cpp               # the catalog listing option
source/harness/cli.cpp                   # speedgunMain, getopt_long, exit statuses
example/benchmark_example.cpp            # ns per iteration and instructions per cycle
test/source/harness_registry_test.cpp    # registration, duplicates, handle options
test/source/harness_calibration_test.cpp # the D-3 sequence on scripted clocks
test/source/harness_capture_test.cpp     # two points per run, zero allocation, folds
test/source/harness_statistics_test.cpp  # the exact aggregate fixture
test/source/harness_cli_test.cpp         # filter, list, dry run, invalid values, SIGINT
test/source/harness_catalog_test.cpp     # the listing fields on a fake system
test/time_source_gate.sh                 # the SC-013 gate scan
test/barrier_shape.sh                    # the codegen gate for the barriers
```

Files this feature edits:

```text
CMakeLists.txt                           # version 0.6.0; the harness archive target
cmake/install-rules.cmake                # the harness target in the export set
example/CMakeLists.txt                   # the example suite on the harness target
test/CMakeLists.txt                      # six test targets and two gate registrations
test/consumer/CMakeLists.txt             # the consumer suite links the harness target
test/consumer/main.cpp                   # one registered benchmark and speedgunMain
.github/workflows/ci.yml                 # the gate step and the consumer-count fix
.clang-tidy                              # the macro-name exemption anchored for SPEEDGUN_NG_BUILD_TYPE
.specify/memory/constitution.md          # the D-6 amendment to 2.18.0
docs/                                    # the barrier rule page
```

**Structure Decision**: the harness lives in the `speedgun-ng` library
under the Library-first entry of Additional Constraints. Its public
interface is the two new headers under `include/speedgun-ng/`, its
implementation is `source/harness/`, and it builds as the static
archive target `speedgun-ng_harness` with the alias
`speedgun-ng::harness`, beside the existing `speedgun-ng_speedgun-ng`
archive (research.md R-01). The split keeps the counters surface free
of harness terms (FR-049) and keeps `getopt_long` inside the
implementation alone (FR-050).

## Design

### Logical view: the objects the run moves through

```text
Registry ──owns──> BenchmarkEntry { name, callable, options, metrics }
                     │  registerBenchmark(callable, name) or SG_BENCHMARK(fn)
                     ▼
speedgunMain ──parses──> RunOptions { filter, listMode, repetitions, minTime,
                                      fixedIterations, warmupTime, dryRun,
                                      leafAddresses, listCatalog }
                     │
                     ▼  per selected entry, in order
BenchmarkHandle ──compiles──> Plan { monotonic, thread_cpu, metric exprs }
                     │             │
                     │             └─ recorder(2 × (95 + 95 + R))  (D-2, FR-019)
                     ▼
State { iterations, skipWithError, skipWithMessage, range-for }
                     │  the timed loop: sample at entry, sample at exit
                     ▼
Run { two points } ──fold──> MetricResult { value, runningRatio,
                                            availability, scaled }
                     ▼
BenchmarkResult { name, outcome, rows[repetitions], aggregates,
                  overheadFloorNs, reason }
                     ▼
report.cpp formats BenchmarkResult as fixed-column rows; nothing is
added to the value (FR-036).
```

### Logical view: one benchmark, start to finish

```text
main        registry        runner                plan/recorder        counters
  | speedgunMain(argc,argv)   |                        |                  |
  |-------------------------->| parse, select          |                  |
  |  for each selected entry: |                        |                  |
  |                | compile(monotonic, thread_cpu,    |                  |
  |                |        metric exprs) ------------>| compile          |
  |                |                                   |----------------->|
  |                | mint recorder(capacity) --------->| arena alloc      |
  |                | read leaf availability (FR-023)   |                  |
  |                | warm-up loop (FR-011)             |                  |
  |                |   run(N): sample, loop, sample--->| two points       |
  |                |   fold decision time <------------| fold             |
  |                |   growth rule (D-3)               |                  |
  |                | calibration loop (FR-008..FR-012) |                  |
  |                |   same shape; first repetition    |                  |
  |                |   only; later repetitions reuse N |                  |
  |                | measured fold per repetition      |                  |
  |                | aggregates (FR-027)               |                  |
  | report rows    |                                  |                  |
  |<--------------------------|                        |                  |
```

The decision time of a run is the `machine/thread_cpu` fold over that
run's two points (FR-009, FR-018); the real-time condition reads the
`machine/monotonic` fold (FR-008). The reported time per iteration is
the monotonic fold divided by the iteration count (FR-020). The
overhead floor is `Plan::sampleOverheadNsMedian()` (FR-026). Every
number on that path is a counters value (FR-038).

### Logical view: benchmark outcome states

```text
registered ──select──> running ──qualify──> measured ──next repetition──> running
    │                     │                                      │
    │                     ├── skipWithError / skipWithMessage ──> skipped
    │                     ├── SIGINT ─────────────────────────> skipped(interrupt)
    │                     └── exception ──────────────────────> failed
    └── filter miss ─────────────────────────────────────────> not run
```

A skipped outcome prints its reason and no statistics (FR-031). A
failed outcome prints the exception text, releases the plan, the
recorder, and every descriptor and mapping the counters library
opened for that benchmark, and the run continues (FR-032, PC-6). The
plan and recorder are RAII objects of the runner frame, so the release
rides the unwind.

### Physical view: duties per file

| File | Duty | Requirements |
| --- | --- | --- |
| `include/speedgun-ng/benchmark.hpp` | `registerBenchmark`, `SG_BENCHMARK`, `BenchmarkHandle`, `State`, `BenchmarkResult`, `RunOutcome` | FR-001..FR-005, FR-013..FR-015, FR-021, FR-031, FR-036 |
| `include/speedgun-ng/barrier.hpp` | `doNotOptimize` overloads, `clobberMemory` | FR-029, FR-055 |
| `source/harness/registry.cpp` | the registry, duplicate-name rule at the 007 FR-046 tier | FR-001, FR-002 |
| `source/harness/runner.cpp` | plan compile, recorder minting, warm-up, calibration, runs, folds, statistics | FR-006..FR-027, FR-038 |
| `source/harness/report.cpp` | context lines and fixed-column rows over `BenchmarkResult` | FR-026, FR-035, FR-036 |
| `source/harness/catalog.cpp` | the catalog listing over `Object::counters()` | FR-037, PC-9 |
| `source/harness/cli.cpp` | `speedgunMain`, `getopt_long`, option validation, exit statuses, SIGINT handler | FR-033, FR-034, FR-036, FR-042, FR-050 |
| `example/benchmark_example.cpp` | the SC-008 suite: ns per iteration and instructions per cycle | Scope In |
| `test/time_source_gate.sh` | the banned-source scan over harness code | FR-040, SC-013 |
| `test/barrier_shape.sh` | compile, disassemble, and count the measured work with and without the barrier | FR-029, US6 |

### Build targets and link relationships

```text
speedgun-ng_speedgun-ng (static archive, unchanged)
        ▲
        │ links
speedgun-ng_harness (static archive, new; alias speedgun-ng::harness)
        ▲              ▲
        │              │
example/benchmark_example   test/consumer/consumer   suite executables
```

The harness archive carries `speedgunMain` and the runner. A suite
links `speedgun-ng::harness` and becomes one executable (D-1,
FR-033). The target joins the install export set beside the counters
archive, so the downstream consumer test reaches it through
`find_package` (SC-010). The harness reuses the counters export header
and the `SPEEDGUN_NG_EXPORT` macro rather than generating its own: the
harness links the counters target PUBLIC, so the export-header
directory and `SPEEDGUN_NG_STATIC_DEFINE` propagate to it and to every
suite. Target names sit outside the C++ identifier
set, so the naming rules do not reach them (FR-033).

### Public API surface added

`include/speedgun-ng/benchmark.hpp` in namespace `sg`:
`registerBenchmark`, the `SG_BENCHMARK` macro, `BenchmarkHandle`,
`State`, `RunOutcome`, `BenchmarkResult`, `MetricValue`.
`include/speedgun-ng/barrier.hpp` in namespace `sg`: `doNotOptimize`,
`clobberMemory`. The counters headers gain nothing (FR-049). No
existing signature or record layout changes, so `SOVERSION` stays 2
(D-5).

### Version table

| Field | At audit point | At feature head | Basis |
| --- | --- | --- | --- |
| Project version | 0.5.0 | 0.6.0 | D-5, FR-046 |
| `SOVERSION` | 2 | 2 | D-5: additions only |
| Package compatibility | `SameMinorVersion` | `SameMinorVersion` | `cmake/install-rules.cmake:39` |
| Constitution | 2.17.0 | 2.18.0 | D-6, FR-045 |

## Test Plan

### Execution mode: TDD (recorded per Principle III and FR-055)

The harness lanes run in TDD mode: each test task is written and
observed failing against the unimplemented harness before the code
task that turns it green. The gate scripts (`time_source_gate.sh`,
`barrier_shape.sh`) are observed failing on a planted input first,
which is the shape SC-013 records.

### Scenario and check mapping

| Scenario | Check | Mode |
| --- | --- | --- |
| US1 / SC-001 / SC-014 | `harness_capture_test`: scripted counts and scripted clock deltas; every reported value equals the scripted fold | fake provider |
| US2 / SC-002 / SC-007 | `harness_calibration_test` walks the D-3 sequence step by step; `harness_cli_test` covers filter, list, dry run, fixed N, benchmark-wins-over-command-line, invalid values | fake provider |
| US3 / SC-009 | `harness_catalog_test` on a fake system with a known leaf set | fake provider |
| US4 / SC-006 | `harness_cli_test` throwing suite: descriptor and mapping counts return to their start values; the next benchmark runs | real host |
| US5 / SC-003 | `harness_statistics_test` matches every aggregate against the fixture | fixture |
| US6 | `barrier_shape.sh` compiles the same work with and without the barrier and counts the surviving instructions | codegen gate |
| FR-040 / SC-013 | `time_source_gate.sh` fails on a planted `std::chrono::steady_clock::now()` and passes after its removal; both runs recorded | gate |
| SC-005 | `harness_capture_test` counts sampling actions through `FakeProvider::readActions()` and allocation through an allocation-counting `operator new` over 10,000 runs | fake provider |
| SC-008 | `benchmark_example` run in CI on the reference host | real host |
| SC-010 | the downstream consumer job builds and runs the consumer suite against the installed package | job |

### Determinism and regression

Every exact-value test drives scripted points through the fake
provider (FR-054). The calibration test scripts
`machine/monotonic` and `machine/thread_cpu` deltas with
`FakeProvider::setPoints`, which the fake supports for machine leaves
through `addCounter` auto-creation (research.md R-03). The suite stays
unprivileged at `perf_event_paranoid` 2.

### Deliberately untouched

The counters library sources and headers, which need no change at the
audit point (Assumptions; FR-041 stays a route with no route taken);
`CMakePresets.json` and `CMakeUserPresets.json`; the vendored trees
under `external/`; every file under `specs/001` through `specs/014`;
the closed `Unit` and `ReadMode` vocabularies; and the three PC-10
rename gaps, which await their separate fix.

## Constitution Check

### Post-design re-evaluation

| Principle | Change after design | Verdict |
|-----------|--------------------|---------|
| I | The barrier statement shape was settled at one extended `__asm__` per overload with the D-4 operand form; the P2 text is recorded in research.md R-13. | PASS with the recorded P2 |
| II | The two-point capture was settled at a `hardStop` recorder of capacity 2 per benchmark; a `Scope` per run was considered and rejected. The recorder is the mechanism D-2 and FR-019 name. | PASS, research.md R-02 |
| VI | The fake-provider substitution was settled at tolerating the duplicate-machine refusal when a test registers its fake first, so no counters signature changes and FR-042 holds on the existing surface. | PASS, research.md R-03 |
| VII | The context-line fields were settled at build-written macros plus catalog facts, so no context value bypasses the counters library. | PASS, research.md R-08, R-14 |
| X | The address-attached leaf was settled at resolving through the catalog unit's dimension, which removes a second resolution API the design first considered. | PASS, research.md R-04 |
| XI | The artifacts were self-checked against XI.1, XI.2, XI.5, and XI.7 during drafting. | PASS |

No violation requires an amendment beyond the D-6 amendment the
feature itself carries. One deviation from Principle II is recorded in
Complexity Tracking.

## Complexity Tracking

| Deviation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| `doNotOptimize` and `clobberMemory` carry doxygen `\pre none` and `\post none` with no runtime `SG_ENSURE` (Principle II asks that the header document and the source enforce) | The barriers have no observable runtime state to assert: their postcondition is a property of the generated code, and the runtime cannot see it. The codegen gate `test/barrier_shape.sh` enforces the property instead, and the Test Plan scenario/check table row for US6 names it. This is the same shape the 011 plan recorded for the marker declaration. | A runtime check would assert nothing about the emitted code, and a counter inside the barrier would defeat the barrier itself. |
