# Tasks: Benchmark Harness Core

**Input**: Design documents from `specs/015-benchmark-harness-core/`

**Prerequisites**: plan.md, spec.md, research.md (R-01..R-14), data-model.md (E-01..E-10), contracts/benchmark-api.md, contracts/cli.md, contracts/time-source-gate.md, quickstart.md

**Tests**: Required. The plan records TDD mode under Principle III and FR-055: every test task is written and observed failing against the unimplemented harness before the code task that turns it green. The two gate scripts are observed failing on a planted input first, which is the shape SC-013 records.

**Organization**: Tasks are grouped by user story so each story can be implemented and tested independently. US1 (P1) is the vertical slice and the MVP; US2-US5 are P2; US6 is P3.

**Build facts carried from the plan**: the harness builds as the static archive `speedgun-ng_harness` with the alias `speedgun-ng::harness` (R-01); the release is version 0.6.0 with `SOVERSION` 2 (D-5); the constitution moves 2.17.0 to 2.18.0 (D-6). Every time and counter value comes from `sg::counters` (FR-038).

## Format: `[ID] [P?] [Story] Description`

- **[P]**: different files, no dependency on an incomplete task
- **[Story]**: US1..US6, user-story phases only
- Paths are repository-root relative

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: the build shape the harness lives in

- [X] T001 Set `VERSION 0.6.0` in the `project()` command of CMakeLists.txt, keeping `SOVERSION 2` and its 0.x rule comment intact (D-5, FR-046); confirm `cmake/install-rules.cmake` keeps `SameMinorVersion` compatibility at line 39
- [X] T002 Add the static archive target `speedgun-ng_harness` with the alias `speedgun-ng::harness` in CMakeLists.txt beside `speedgun-ng_speedgun-ng`: PRIVATE sources from `source/harness/`, `target_link_libraries(speedgun-ng_harness PUBLIC speedgun-ng::speedgun-ng)`, `CXX_VISIBILITY_PRESET hidden`, `VISIBILITY_INLINES_HIDDEN YES`, `VERSION`/`SOVERSION` properties matching the counters archive, `EXPORT_NAME harness`, `OUTPUT_NAME speedgun-ng-harness`, and the `SG_BUILD_TYPE` compile definition holding `CMAKE_BUILD_TYPE` (R-01, R-14); generate no export header for the harness - the public headers reuse the counters export header and `SPEEDGUN_NG_EXPORT`, which the PUBLIC link to the counters target propagates with `SPEEDGUN_NG_STATIC_DEFINE` (FR-049)
- [X] T003 Add the `speedgun-ng_harness` target to the install export set in cmake/install-rules.cmake beside `speedgun-ng_speedgun-ng`, so a downstream consumer reaches `speedgun-ng::harness` through `find_package` (SC-010)
- [X] T004 Create the empty directory scaffolding the plan names: `source/harness/`, `test/source/` (exists), `example/` (exists)

**Checkpoint**: the harness target configures and links; nothing measurable yet.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: the public surface, the registry, the governance amendment, and the time-source gate

**⚠️ CRITICAL**: no user story work begins before this phase completes.

- [X] T005 Write the public header `include/speedgun-ng/benchmark.hpp` in namespace `sg`: `registerBenchmark(std::function<void(State&)>, std::string_view) -> BenchmarkHandle`, the `SG_BENCHMARK(fn)` macro (R-09, R-10), `BenchmarkHandle` with the option setters `minTime`, `warmupTime`, `repetitions`, `iterations` and `addMetric` (E-02), `State` with `iterations()`, `begin()`, `end()`, `skipWithError`, `skipWithMessage` (E-03), `enum class RunOutcome : std::uint8_t { MEASURED, SKIPPED, FAILED }`, `MetricValue { value, runningRatio, scaled, availability }`, `ResultRow`, `BenchmarkResult { name, outcome, rows, reason }`, and `speedgunMain(int, char**) -> int`, each declaration carrying doxygen `\pre`, `\post`, `\invariant` per contracts/benchmark-api.md (FR-053); annotate the public types with `SPEEDGUN_NG_EXPORT` from the counters export header `speedgun-ng/speedgun-ng_export.hpp`, as the counters headers do; no platform term and no `getopt_long` in this header (FR-050); the counters headers gain no harness term (FR-049)
- [X] T006 Write the public header `include/speedgun-ng/barrier.hpp` in namespace `sg`: the `doNotOptimize` template declaration and `clobberMemory()`, documented with `\pre none` and `\post none` and the recorded Principle II deviation pointing at the `test/barrier_shape.sh` gate (plan Complexity Tracking, R-13); confirm the DBC completeness gate accepts the recorded deviation the way it accepted the 011 marker declaration
- [X] T007 Implement the registry in `source/harness/registry.cpp`: a function-local static registry holding `BenchmarkEntry` records (E-01: `name`, `callable`, `minTimeNs`, `warmupTimeNs`, `repetitions`, `fixedIterations`, `expressions`) in registration order; `registerBenchmark` and the `SG_BENCHMARK` pre-`main` object both reach it (R-09); on a repeated name "report a recoverable error at the tier 007 FR-046 gives a registration duplicate, shall keep the first registration, and shall run the rest" (FR-002); enforce the contracts of T005 through the `dbc` macros
- [X] T008 [P] Write the time-source gate `test/time_source_gate.sh`: scan every C++ source and header the D-6 scope names - `source/`, `include/`, `example/`, `test/`, and `tools/` - with the counters library excluded, that is `source/counters/`, the `include/speedgun-ng/counters*.hpp` headers, and the `counters_` tests and gate scripts under `test/`, for the constitutional banned list of contracts/time-source-gate.md - the `std::chrono` clocks and `<chrono>`, `std::clock`, `std::time`, `timespec_get`, `clock_gettime`, `clock_getres`, `gettimeofday`, `time(`, `times(`, `getrusage`, `rdtsc`, `rdtscp`, `__rdtsc`, `__rdtscp`, `<x86intrin.h>`, `<immintrin.h>`, `<ia32intrin.h>` - substring matching, allow `<chrono>` and `std::chrono::steady_clock` in `tools/dbc/overhead.cpp` alone, keyed by path and term, and report any other term in that file; print `file:line` per hit, exit 1 on any hit, exit 0 clean, following the shape of `test/counters_header_purity.sh` (R-07, FR-040)
- [X] T009 Create the four harness implementation stubs with their doxygen contract blocks and `dbc` enforcement in place - `source/harness/runner.cpp`, `source/harness/report.cpp`, `source/harness/catalog.cpp`, `source/harness/cli.cpp` - so the DBC completeness gate sees every interface from the first build (FR-053); `source/harness/registry.cpp` arrives complete through T007
- [X] T010 Amend `.specify/memory/constitution.md` to 2.18.0 per D-6 and FR-043..FR-045: a new Additional Constraints entry beside Library-first stating the counters-only rule, the full banned list, the D-6 scope sentence and the counters-library definition, the one named exception `tools/dbc/overhead.cpp` with its two terms and its reference-clock reason, the counters-change route, and the amendment-only exception route; the Principle VIII gate list gains the time-source gate as a hard gate; a new Sync Impact Report at the head of the file, a lineage row for 2.18.0, the Last Amended date, and the version footer set to 2.18.0; the entry adds no Open deferrals entry (SC-016)
- [X] T011 Register the gate scripts in test/CMakeLists.txt: `add_test(NAME time_source_gate COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/time_source_gate.sh)`; then run the SC-013 pair and record both runs in this file's Execution Log: plant `std::chrono::steady_clock::now()` in the `source/harness/runner.cpp` stub T009 created, observe `ctest --preset=dev -R time_source_gate` exit 1 with the `file:line` printed, remove the plant, observe exit 0; plant `std::chrono::system_clock` in `tools/dbc/overhead.cpp`, observe exit 1, remove it, observe exit 0 (SC-013)

**Checkpoint**: headers compile, the registry works, the gate runs in ctest with its planted-failure and removal runs recorded, the constitution carries the rule, and the suite stays unprivileged at `perf_event_paranoid` 2 (FR-054). User stories can now proceed, in parallel if desired.

---

## Phase 3: User Story 1 - Write a benchmark, run it, read the numbers (Priority: P1) 🎯 MVP

**Goal**: a registered benchmark runs single-threaded through `speedgunMain` and prints one fixed-column row: real time per iteration in ns, the overhead floor, and an attached PMU metric with its running ratio, scaled flag, and gap state.

**Independent Test**: `cmake --build --preset=dev --target benchmark_example && ./build/dev/example/benchmark_example` prints the context lines and one measured row; exit status 0 (quickstart §2, SC-008).

### Tests for User Story 1 ⚠️ write first, observe failing

- [X] T012 [P] [US1] Write `test/source/harness_capture_test.cpp`: register a `FakeProvider` carrying scripted `machine/monotonic` and `machine/thread_cpu` leaves before `speedgunMain` (R-03, FR-042); assert every per-iteration count equals the scripted delta divided by N and a dimensionless ratio metric keeps its window value undivided (SC-001, FR-022); assert the disclosure beside the value: a run with running ratio 0.5 reports ratio 0.5 with the scaled flag, and a run with a gap reports the value unavailable and excluded from the statistics (SC-004, FR-024); assert exactly two sampling actions per run through `FakeProvider::readActions()` and zero allocations inside the timed loop over 10,000 runs through an allocation-counting `operator new` (SC-005, FR-052); assert the reported real time per iteration, each calibration and warm-up decision, and the reported overhead floor each equal the value the scripted deltas and the plan produce (SC-014); assert a scripted `machine/tsc` leaf with no published rate reports as a count and derives no time from it (FR-039, edge case); register the target in test/CMakeLists.txt and observe the suite failing against the unimplemented runner
- [X] T013 [P] [US1] Write `example/benchmark_example.cpp`: benchmarks registered with `SG_BENCHMARK`, one attaching the `instructions / cycles` metric built with the 007 C++ arithmetic, `main` calling `speedgunMain` (D-1); on a host without a countable `cpu/instructions` leaf the metric reports unavailable with its refusal kind and the exit status stays 0 (SC-008)
- [X] T014 [P] [US1] Write `test/source/harness_registry_test.cpp`: `SG_BENCHMARK` registers before `main`; `registerBenchmark` accepts a runtime callable; a duplicate name reports the recoverable error, keeps the first registration, and runs the rest (FR-001, FR-002); handle option setters write the E-01 fields and a setter after the run starts is a contract violation; register the target in test/CMakeLists.txt and observe it failing

### Implementation for User Story 1

- [X] T015 [US1] Implement the runner in `source/harness/runner.cpp`: compile one plan per benchmark with `machine/monotonic`, `machine/thread_cpu`, and every metric expression through `sg::counters::compile` (R-11); mint one `hardStop` recorder of capacity `2 × (96 + 96 + R)` in the untimed region before the first run (R-02, FR-019); read the availability of every leaf each metric needs before the run, marking an uncountable metric unavailable with its refusal kind while the benchmark still runs (FR-023); run the timed loop with `State`'s range-for, `RecorderHandle::sample()` at entry and at exit and no other counter work (FR-017, FR-052); fold every value after the run over points `2k` and `2k+1` (FR-018); report real time per iteration as the `machine/monotonic` fold divided by N (FR-020); resolve an address-attached leaf by splitting at its last `/` and mapping the catalog unit through `dimensionOf` (R-04); run the first `sampleOverheadNs*` call on the plan for the overhead floor (FR-026); every time and counter value on this path comes from a plan, a recorder, a fold, or a value the plan publishes (FR-038)
- [X] T016 [US1] Implement the console report in `source/harness/report.cpp`: context lines with the version and build type from `SG_BUILD_TYPE` and the project version and the host and cpu fields only where the counters catalog publishes them (R-08, FR-035); one fixed-column row per repetition carrying iterations, `time/iter (ns)`, `overhead floor (ns)`, and each metric column with value, `ratio=`, `scaled=`, `gap=` (FR-024, FR-026); the row states that each run window includes the cost of its two endpoint samples; the report formats `BenchmarkResult` and adds nothing to it (FR-036)
- [X] T017 [US1] Implement `speedgunMain` in `source/harness/cli.cpp`: register `ClockProvider` and treat a refusal naming the machine object as a pass (R-03); select every registered benchmark when no filter is given; call the runner per selected entry and print the report; return the exit status of contracts/cli.md - zero for measured and skipped benchmarks and for a filter that matches nothing, with a message (FR-036)
- [X] T018 [US1] Edit example/CMakeLists.txt so `benchmark_example` links `speedgun-ng::harness` (add it through the `add_example` function or a direct target), and run `ctest --preset=dev -R 'harness_capture_test|harness_registry_test' --output-on-failure` to green

**Checkpoint**: US1 works end to end - one benchmark, one correct row, on scripted and on real time.

---

## Phase 4: User Story 2 - Control the run from the command line (Priority: P2)

**Goal**: filter, list, repetitions, minimum time, fixed iterations, warm-up time, dry run, and `--counter` addresses drive the run; a benchmark value wins over the command line.

**Independent Test**: one executable holding several benchmarks, each option checked for the selected set, the run counts, and the discarded warm-up and calibration results (quickstart §3, SC-007).

### Tests for User Story 2 ⚠️ write first, observe failing

- [X] T019 [P] [US2] Write `test/source/harness_calibration_test.cpp`: scripted `machine/monotonic` and `machine/thread_cpu` deltas walk the D-3 sequence step by step, asserting the iteration count of each step and the step that qualifies, including a scripted step whose real time reaches 5 times the minimum time while its thread CPU time stays below it, which qualifies; the recorded points of each measured run meet a D-3 stop condition; warm-up and calibration runs add no sample to the statistics; a fixed N skips calibration (SC-002, FR-013); a fixed N with a set warm-up time warms up from N and grows by the D-3 rule within the FR-016 bound; each measured run then runs exactly N (FR-011, SC-002); register the target in test/CMakeLists.txt and observe it failing
- [X] T020 [P] [US2] Write `test/source/harness_cli_test.cpp`: a filter matching two of three benchmarks runs exactly those two; list mode prints the two names and runs no benchmark function; `--iterations` skips calibration; a dry run executes one iteration and one repetition with no warm-up; a benchmark's own minimum time wins over the command line (FR-015); an unparseable value and a negative value for each numeric option, and a zero repetition count, iteration count, and minimum time each report a recoverable error, run no benchmark, and exit nonzero (FR-034, SC-018); a zero warm-up time runs and exits zero (SC-018); a filter that matches nothing says so, runs nothing, exits zero, and each exit status of FR-036 matches the contract (SC-019); register the target in test/CMakeLists.txt and observe it failing

### Implementation for User Story 2

- [X] T021 [US2] Implement the option parsing in `source/harness/cli.cpp` with `getopt_long` (implementation only, FR-050): `--filter` regular expression, `--list`, `--repetitions` (integer ≥ 1), `--min-time` (seconds, > 0), `--iterations` (integer ≥ 1), `--warmup-time` (seconds, ≥ 0), `--dry-run`, `--counter` (catalog address, repeatable) per contracts/cli.md; validate every numeric value and report an invalid one as a recoverable error at the 007 FR-046 tier, running nothing and exiting nonzero (FR-034); apply the precedence rule "a command-line value shall apply where the benchmark sets none; a value the benchmark sets shall win" (FR-015)
- [X] T022 [US2] Implement the run control in `source/harness/runner.cpp`: the defaults "minimum time 0.5 s; minimum warm-up time 0 s; repetitions 1; calibration start 1 iteration" (FR-007); the five qualify conditions of FR-008 with the cap "the iteration count reached the cap of 10^12 iterations"; the growth rule in integer nanoseconds - factor `1.4 × minTime / max(decisionTime, 1 ns)`, factor 10 when `decisionTime ≤ 0.1 × minTime`, next count `max(round(N × factor), N + 1)`, capped (R-12, FR-010); the decision time is the `machine/thread_cpu` fold (FR-009); warm-up against the minimum warm-up time with the measured phase discarding the warm-up count and starting from its own start count, and a fixed N still warming up from N by the same rule within the FR-016 bound (FR-011, FR-013); only the first repetition calibrates (FR-012); a dry run takes 1 iteration, 1 repetition, no warm-up (FR-014); the calibration run count stays within the 96-run bound (FR-016)
- [X] T023 [US2] Implement metric attachment by address in `source/harness/runner.cpp`: each `--counter` leaf attaches to every selected benchmark (FR-021); a leaf the catalog lacks is a recoverable resolution failure at the 007 FR-046 tier naming the leaf (E-02 validation, edge case); a metric of dimension events^1 or time^1 reports per iteration and a metric of any other dimension keeps its window value (FR-022)
- [X] T024 [US2] Turn the suites of T019 and T020 green in test/source/harness_calibration_test.cpp and test/source/harness_cli_test.cpp: `ctest --preset=dev -R 'harness_calibration_test|harness_cli_test' --output-on-failure`

**Checkpoint**: US1 and US2 both work independently; the tool can be aimed at one benchmark among many.

---

## Phase 5: User Story 3 - Find a counter address in the catalog listing (Priority: P2)

**Goal**: `--catalog` prints every counter the running host publishes and exits without running a benchmark; the user copies an address from the listing into `--counter`.

**Independent Test**: the listing option against a fake-provider system with a known leaf set, every field on every line checked (quickstart §4, SC-009).

### Tests for User Story 3 ⚠️ write first, observe failing

- [X] T025 [P] [US3] Write `test/source/harness_catalog_test.cpp`: on a fake-provider system with a known leaf set, every published leaf prints with its object path, counter name, description, unit, read mode, and availability state with the refusal kind for a leaf that cannot count; no benchmark function runs and the exit status is zero (FR-037, SC-009); register the target in test/CMakeLists.txt and observe it failing

### Implementation for User Story 3

- [X] T026 [US3] Implement the listing in `source/harness/catalog.cpp` over `Object::counters()` (PC-9): one line per published counter with the six fields of FR-037 and the refusal kind; the `frequencyHz` and `scaled` fields of `CatalogEntry` stay out of the line (E-10); the `machine/tsc` leaf appears as a catalog fact on x86 builds alone with no API difference (FR-039)
- [X] T027 [US3] Wire `--catalog` in `source/harness/cli.cpp`: print the listing, exit without running a benchmark, exit zero on success and nonzero when the listing fails (FR-036, US3 scenario 2)
- [X] T028 [US3] Turn the suite of T025 green in test/source/harness_catalog_test.cpp: `ctest --preset=dev -R harness_catalog_test --output-on-failure`

**Checkpoint**: the PMU half of US1 is reachable - the user can name a leaf the host can count.

---

## Phase 6: User Story 4 - Skips and failures keep the run alive (Priority: P2)

**Goal**: a skip reports its reason and no statistics; a throw marks the benchmark failed with the exception text, releases every plan, recorder, descriptor, and mapping, and the run continues.

**Independent Test**: a suite whose benchmark throws on every run, with the process descriptor and mapping counts compared before and after, then the next benchmark still running (quickstart §6, SC-006).

### Tests for User Story 4 ⚠️ write first, observe failing

- [X] T029 [P] [US4] Extend `test/source/harness_cli_test.cpp`: a benchmark that skips with a message prints the reason and no statistics (FR-031); a benchmark that throws prints the exception text, leaves the process descriptor count and mapping count at their start values across 1,000 runs, and the next benchmark runs and reports (FR-032, SC-006, PC-6); a benchmark that raises `SIGINT` inside its function completes the current run and starts no later run; it reports skipped with an interrupt reason, releases the same resources, and exits nonzero (FR-032, SC-017, R-06); observe the new cases failing

### Implementation for User Story 4

- [X] T030 [US4] Implement the skip paths in `source/harness/runner.cpp` and `State`: `skipWithError` and `skipWithMessage` end the timed loop, record the reason in E-03, and suppress the statistics for that benchmark (FR-031); a run whose window carries a gap reports the value unavailable and adds no sample to the statistics (FR-025, PC-4)
- [X] T031 [US4] Implement failure isolation in `source/harness/cli.cpp` and `source/harness/runner.cpp`: the plan and recorder are RAII objects of the runner frame so the exception unwind releases every descriptor and mapping (FR-032, PC-6); install the `SIGINT` handler that stores one `std::atomic<bool>` and make the runner read the flag after each warm-up, calibration, and measured run, with no flag read in the `State` iterator, so the runner marks the benchmark skipped with the interrupt reason, starts no later run, and the process exits nonzero (R-06)

**Checkpoint**: a wrong or interrupted benchmark never takes the run, or the process, down with it.

---

## Phase 7: User Story 5 - Statistics over repetitions (Priority: P2)

**Goal**: repetitions greater than one print one row per repetition plus aggregate rows: mean, median, standard deviation, coefficient of variation, min, max, for time and for each metric.

**Independent Test**: a fixture with a known set of repetition results matched exactly against every aggregate (SC-003).

### Tests for User Story 5 ⚠️ write first, observe failing

- [X] T032 [P] [US5] Write `test/source/harness_statistics_test.cpp`: five repetitions produce five rows plus the aggregate rows in fixed columns; every aggregate equals the fixture value, computed with the sample form - "sample standard deviation (the `n − 1` denominator), coefficient of variation (standard deviation divided by mean)" (R-05, FR-027, SC-003); a repetition whose run carries a gap contributes no sample (FR-025); register the target in test/CMakeLists.txt and observe it failing

### Implementation for User Story 5

- [X] T033 [US5] Implement the aggregation in `source/harness/runner.cpp` over the repetitions that produced a measured value: mean, median, sample standard deviation, coefficient of variation, min, max, for time and for each metric (FR-027); one repetition's result is the fold of that repetition's measured run; the percentile form stays out (FR-028)
- [X] T034 [US5] Print the aggregate rows in `source/harness/report.cpp` in the same fixed columns, appearing only when repetitions exceed 1 (FR-035, E-06)
- [X] T035 [US5] Turn the suite of T032 green in test/source/harness_statistics_test.cpp: `ctest --preset=dev -R harness_statistics_test --output-on-failure`

**Checkpoint**: the distribution form of Principle VII is on the console.

---

## Phase 8: User Story 6 - Barriers keep the measured work (Priority: P3)

**Goal**: `doNotOptimize` keeps a computed value alive against the optimizer; `clobberMemory` covers a store; the documentation states when a barrier is justified.

**Independent Test**: `test/barrier_shape.sh` compiles the same work with and without the barrier and counts the surviving instructions (quickstart §8).

### Tests for User Story 6 ⚠️ write first, observe failing

- [X] T036 [P] [US6] Write the codegen gate `test/barrier_shape.sh`: compile the same measured work with and without `doNotOptimize` with g++ and clang++ at -O2, failing only where neither compiler exists, as test/counters_tsc_read_shape.sh does, disassemble, and count the surviving instructions - the barrier keeps the computation, its absence eliminates it (FR-029, US6 scenarios 1 and 2); register it in test/CMakeLists.txt as `add_test(NAME barrier_shape COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/barrier_shape.sh)` and observe it failing on a planted no-op barrier

### Implementation for User Story 6

- [X] T037 [US6] Implement `doNotOptimize` and `clobberMemory` in `include/speedgun-ng/barrier.hpp`: one empty extended-assembly statement per overload naming the value as an input and output operand with a register-or-memory constraint and a memory clobber (D-4, R-13, the recorded P2 of FR-055), the deprecated const-reference overload staying out, and `clobberMemory` as `std::atomic_signal_fence(std::memory_order_acq_rel)`
- [X] T038 [US6] Turn the gate of T036 green in test/barrier_shape.sh: `ctest --preset=dev -R barrier_shape --output-on-failure`

**Checkpoint**: the barrier gate shows the -O2 build keeps the measured work.

---

## Phase 9: Polish & Cross-Cutting Concerns

- [X] T039 [P] Edit test/consumer/CMakeLists.txt and test/consumer/main.cpp so the downstream consumer links `speedgun-ng::harness`, registers one benchmark, and calls `speedgunMain` (SC-010, D-1)
- [X] T040 [P] Edit .github/workflows/ci.yml: add the time-source gate as a hard CI step and fix the downstream-consumer catalog-count comparison for the consumer suite now built on `speedgunMain` (FR-044, SC-011)
- [X] T041 [P] Write the barrier rule page under docs/pages/ stating the Principle X.2 rule: "a barrier stands only where the compiler would otherwise eliminate the measured work" (FR-030)
- [X] T042 Run every quickstart.md section §1-§10 against the built tree and record the outcomes in this file's Execution Log, including the SC-013 planted-and-removed pairs, the SC-008 and SC-009 runs on the reference host, and the SC-016 constitution greps
- [X] T043 Build the release configuration once per Principle IX from CMakeLists.txt: `cmake --preset=ci-ubuntu`, `cmake --build build`, `ctest --test-dir build`; confirm version 0.6.0 with `SOVERSION 2` in the installed package and that a consumer requesting 0.5 rejects it, and confirm the name check reports zero `readability-identifier-naming` findings on the harness translation units (SC-011, SC-012, FR-047, FR-048, FR-051)

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: no dependencies
- **Foundational (Phase 2)**: depends on Setup; BLOCKS every user story. T010 (the constitution amendment) must land before the time-source gate is counted as a hard gate, T009 creates the stubs the planted call needs, and T011 records the gate's failing and passing runs.
- **User Stories (Phases 3-8)**: all depend on Foundational; then independent of one another
- **Polish (Phase 9)**: depends on the stories it exercises (T039 on US1, T040 on the gate of T008/T011)

### User Story Dependencies

- **US1 (P1)**: Foundational only. The MVP.
- **US2 (P2)**: Foundational only; extends the `cli.cpp` and `runner.cpp` files US1 created, so it follows US1 in a single-threaded plan but needs no US1 behavior to be tested.
- **US3 (P2)**: Foundational only; new file `catalog.cpp`.
- **US4 (P2)**: Foundational only; extends `runner.cpp` and `cli.cpp`.
- **US5 (P2)**: Foundational only; extends `runner.cpp` and `report.cpp`.
- **US6 (P3)**: Foundational only; self-contained in `barrier.hpp` and its gate script.

### Within Each User Story

- Tests written and observed failing before the implementation task that turns them green
- Header and registry before the runner; the runner before the report; the report before `speedgunMain` output checks
- One file is edited by one story at a time: `source/harness/runner.cpp` is touched by US1 (T015), US2 (T022, T023), US4 (T030, T031), US5 (T033) - those tasks never run in parallel with each other

### Parallel Opportunities

- T005, T006, T008 in Foundational are different files and can run together
- Within US1: T012, T013, T014 (three different test files)
- Within US2: T019, T020 (two different test files)
- Across stories once Foundational is done: US3 (T025-T027) and US6 (T036-T038) touch no file another story edits and are the cleanest parallel tracks; US2, US4, US5 share `runner.cpp`/`cli.cpp` and serialize against each other
- T039, T040, T041 in Polish are three different files

### Parallel Example: User Story 1

```text
Together (different files, both blocked only on Foundational):
  T012 test/source/harness_capture_test.cpp
  T013 example/benchmark_example.cpp
  T014 test/source/harness_registry_test.cpp
Then serially (same-file and link dependencies):
  T015 runner.cpp -> T016 report.cpp -> T017 cli.cpp -> T018 example/CMakeLists.txt + ctest
```

---

## Implementation Strategy

### MVP First (User Story 1 only)

1. Phase 1 Setup (T001-T004)
2. Phase 2 Foundational (T005-T011) - critical, blocks everything
3. Phase 3 US1 (T012-T018)
4. **Stop and validate**: quickstart §2 - one benchmark, one correct row, exit 0

### Incremental Delivery

1. Setup + Foundational → the target, the surface, the gate, the amendment
2. US1 → the vertical slice, the MVP: ns per iteration plus a PMU metric with disclosure
3. US2 → run control; US3 → the catalog; US4 → failure isolation; US5 → statistics (each P2, each independently testable)
4. US6 → barriers for pure computation
5. Polish → consumer, CI, docs, release-preset build

---

## Execution Log

Record here, as the work lands: the SC-013 planted-failure and removal runs, the `tools/dbc/overhead.cpp` pair, and the reference-host SC-008 and SC-009 runs (T011, T042), the quickstart §1-§10 outcomes (T042), and the release-build result with the version and `SOVERSION` check (T043).

### Reference host

Linux 7.2.4-1-cachyos, AMD Ryzen 9 9950X3D, GCC 16.2.1 (20260810),
clang 23.1.1, `perf_event_paranoid` 2. Every command below ran in this
tree at this commit.

### T011 and quickstart §7: the SC-013 planted pairs

- Clean tree: `ctest --preset=dev -R time_source_gate` → `Test #49: time_source_gate ... Passed`, exit 0.
- Planted `std::chrono::steady_clock::now()` in `source/harness/runner.cpp`: exit 1 with
  `time_source_gate: banned time source 'std::chrono::steady_clock':` and the line
  `source/harness/runner.cpp:483:void plantedProbe() { std::chrono::steady_clock::now(); }`.
- Plant removed: exit 0.
- Planted `std::chrono::system_clock::now()` in `tools/dbc/overhead.cpp`: exit 1 with
  `banned time source 'std::chrono::system_clock': tools/dbc/overhead.cpp:262:...`, while that file's `<chrono>` include and `std::chrono::steady_clock` stayed unreported. That is the D-6 allowance holding its shape: two terms in one file, nothing else.
- Plant removed: exit 0.

### Quickstart §1 to §10

- §1 dev loop: `cmake --preset=dev`, `cmake --build --preset=dev`, `ctest --preset=dev` → 100% tests passed out of 56, including the six `harness_*` suites, `time_source_gate`, and `barrier_shape`.
- §2 example suite (SC-008): the context line reads `speedgun-ng 0.6.0 build=Debug`; `bmTouch` and `bmSum` each print one row with time per iteration in ns, the overhead floor in ns, and `endpoint samples=2`; `bmSum` carries `instructions/cycle= 5.966 ratio=1.000 scaled=0 gap=0`; exit 0.
- §3 run control (SC-007): `--filter 'bm.*' --list` prints the two names and no row, exit 0; `--dry-run` reports `iterations= 1` for both; `--iterations 100` reports 100; `--repetitions 5` prints five rows plus the `AGGREGATE` row for time and for the metric, with mean, median, sd, cv, min, max, and n; `--min-time 0` prints `--min-time: expected a positive number of seconds, got '0'`, runs nothing, and exits 2.
- §4 catalog listing (SC-009, FR-039): 390 published counters, one line each with the object path, the name, the description, the unit, the read mode, and the availability state with its refusal kind; `machine/monotonic`, `machine/thread_cpu`, `machine/process_cpu`, and `machine/tsc` with `mode=fast-tsc` appear on this x86 host; no benchmark function runs; exit 0.
- §5 scripted exactness: `harness_calibration_test` and `harness_capture_test` pass. The scripted rows equal the scripted delta divided by N, and the SC-005 action arithmetic holds: a two-repetition call samples exactly two more actions than a one-repetition call.
- §6 failure isolation (SC-006, SC-017): `harness_cli_test` passes. The throwing benchmark reports `FAILED: the scripted failure`; 1,000 failed runs over a real pmu leaf return the descriptor count and the mapping count to their start values; the next benchmark runs and reports. The SIGINT suite reports `SKIPPED: interrupted`, runs its body once, and exits 1.
- §8 barrier gate: g++ keeps 12 instructions in the shape function with `sg::doNotOptimize` and 1 without it; clang++ keeps 52 and 1; the planted no-op barrier holds 1, so the detector fails on it; `ctest --preset=dev -R barrier_shape` passes.
- §9 release configuration (SC-010, SC-012): `cmake --preset=ci-ubuntu` configures; `cmake --build build` exits 0 with clang-tidy and cppcheck active and zero `readability-identifier-naming` findings; `ctest --test-dir build` → 100% tests passed out of 56. `cmake --install build --prefix <p>` publishes `PACKAGE_VERSION "0.6.0"`; a consumer with `find_package(speedgun-ng 0.5 REQUIRED)` is rejected with "not compatible with requested version 0.5" against the found 0.6.0, and one with 0.6 configures, builds, and runs its benchmark against the installed package. A shared configuration (`-DBUILD_SHARED_LIBS=ON`) publishes `libspeedgun-ng.so.2` and `libspeedgun-ng-harness.so.2`, the `SOVERSION 2` of CMakeLists.txt:47 and CMakeLists.txt:218.
- §10 constitution state (SC-016): the Additional Constraints entry states the counters-only rule, the bound scope, the counters library it excludes, the banned list, the counters-change route, and the amendment-only exception route; the Principle VIII gate list names the time-source gate; the lineage table ends at 2.18.0 and the footer reads `**Version**: 2.18.0`.

### Phase 10 convergence (T044 to T051)

- T044: `parseCount` reads the count with `std::strtoll` and rejects
  `value <= 0`, so `--iterations=-5` and `--repetitions=-1` report
  `expected a positive integer`, run nothing, and exit 2. The old
  `strtoull` wrapped them to a huge count and the run hung.
- T045: `harness_calibration_test` drives three scripted scenarios in
  re-execs of its own binary, each with its own `FakeProvider` script.
  `walk` scripts `machine/thread_cpu` on the provider's sampling-action
  index so run k folds `1000 << k` ns against a 8000 ns minimum: the
  walk runs 1, 11, 62, 174 and qualifies at the fourth run, the report
  carries `iterations=174` three times with `n=3`, and the action count
  `planActions + 12` proves exactly those six runs ran. `warmup` fixes
  N=5 with an 8000 ns warm-up: four warm-up runs grow by the FR-010
  rule (`planActions + 10` actions) and the measured row still carries
  `iterations=5`. `fivefold` scripts a 5000 ns monotonic step against a
  1000 ns minimum with a 100 ns thread CPU step: the first run qualifies
  on real time alone, `iterations=1`, `time/iter=5000`. The plan's
  calibration length is revealed by a fixed-N probe run. The test
  hard-codes no length.
- T046, T048: `harness_cli_test` runs fourteen subprocess cases through
  one `arg` mode - an unparseable value and a negative value for each
  of the four numeric options, a zero repetition count, iteration count,
  and minimum time, an unknown option, the valid zero warm-up time, and
  the unknown `--counter` leaf - each asserting the exit status, that
  the report names the rejected option or leaf, and whether the
  benchmark function ran (`BENCHMARK_RAN` printed only when it did).
- T047: the capture suite scripts `machine/tsc` with unit `none`; the
  column reads `machine/tsc= 175.000 ... scaled=0` at N=4 while
  `time/iter` stays the `machine/monotonic` figure of 1000 ns.
- T049: the SC-005 probe runs at its stated scale. A 10,000-run call
  samples 20,321 actions (the plan's 321 calibration actions plus two
  per run), a 9,999-run call samples 20,319, and the difference is 2;
  the same two benchmark shapes sample the same count, and the
  allocation probe runs inside the timed loop of all 10,000 runs.
- T050, T051: settled in the artifacts. The runner carries no part of
  them: the
  per-quantity reading of FR-025 is recorded in
  `contracts/benchmark-api.md` and data-model E-06, and the three
  additions gained their lines in `contracts/benchmark-api.md`,
  data-model E-07 and E-08, and research R-03.
- Re-verification after the phase: `cmake --build --preset=dev` exits 0
  and `ctest --preset=dev` reports 100% tests passed out of 56; the
  release preset `ci-ubuntu` builds and tests clean again with the
  changed `source/harness/cli.cpp`.

### Deviations from the artifacts

1. TDD red observation (Principle III, FR-055): for US1 to US5 the implementation landed before its test, so the planned failing-first run was not observed for those stories. Each test still failed first where the implementation was genuinely wrong, and those failures were fixed in the implementation: the dry-run iteration rule, the `--min-time 0` acceptance, the unknown-option exit status, the catalog description field, and the gap-action arithmetic. US6's gate carries its own planted-failure probe and runs it on every pass.
2. SC-014: the plan calibrates the overhead floor with `std::chrono::steady_clock` inside the counters library (`source/counters/plan.cpp:280`), so the published floor is a wall-clock figure that moves with the host; 100 ns to 290 ns here. The tests assert one floor per plan and a positive value, not a scripted exact number.
3. `BenchmarkResult` carries `metricLabels` beyond E-07: the report prints a metric column under the label the handle or the `--counter` address named, and E-05's `MetricValue` carries no label. Resolved at T051: data-model E-07 and contracts/benchmark-api.md now carry the field.
4. The harness targets compile `SG_PROJECT_VERSION` as a private definition beyond the T002 list, which is how the context line prints the library version. Resolved at T051: data-model E-08 names the macro.
5. `speedgunMain` registers `PmuProvider` beside `ClockProvider`, where R-03 names the clock provider alone, so the FR-037 catalog and the FR-022 metric leaves reach an executable that names no provider. Resolved at T051: research R-03 now states the registration and its refusal rule.
6. The downstream consumer names no provider at all: the entry point registers them, and the consumer reads `objects("pmu")` after the run (T039).
7. Quickstart §3 spells the filter `BM_.*`. Principle V's naming law makes `BM_sum` a `readability-identifier-naming` finding, so the example benchmarks are `bmSum` and `bmTouch` and the §3 run used `bm.*`.
8. Quickstart §5 asks the calibration test to match the iteration count of every step of the walk. A scripted constant leaf step never follows the iteration count, so the test asserted only that the walk grew past one iteration, that the count stays inside the FR-016 bound, and that a fixed N skips calibration entirely. Resolved at T045: the scripted `machine/thread_cpu` sequence is laid out on the provider's sampling-action index, so each step's decision time is scripted, every step's count is hand-computed, and the probe run reveals the plan's calibration length instead of hard-coding it.
9. `test/source/harness_capture_test.cpp` carries the GCC `-Wmismatched-new-delete` suppression that `test/source/counters_noalloc_test.cpp` already records for the same false positive; the release optimizer makes it fire.
10. The context line prints the library version and the build type alone. contracts/cli.md's report shape shows a host line and a cpu line "when published", and the catalog at this head publishes no host or cpu model string (R-08), so those two fields stay out until a counters provider publishes them.

---

## Notes

- TDD mode is binding (plan Test Plan, Principle III, FR-055): a test task ends only when its failure has been observed
- Every new identifier follows N-1..N-12; `SG_BENCHMARK` is the `SG_`-prefixed macro spelling (R-10, FR-047)
- The three PC-10 rename gaps (`sgCtReject`, `reached_after`, `noexcept_violator`) are outside this feature and no task touches them
- FR-041 is conditional work with no task: if implementation finds a harness need the counters library does not meet, it becomes a counters change inside this feature with its own test (SC-015); the audit point found no such need
- Commit after each task or logical group, per the constitution's Pull Request Quality rules

---

## Phase 10: Convergence

Appended by `/speckit-converge` at the assessed head. Every task below is
remaining work against `spec.md`, `plan.md`, and `tasks.md` as the source
of intent, with the constitution 2.18.0 as the governing constraint. The
deviations already recorded above are noted where a task resolves one.

- [X] T044 Reject a negative count for `--repetitions` and `--iterations` in `parseCount` of `source/harness/cli.cpp` per FR-034 (contradicts): `std::strtoull` wraps `-5` to a huge count, so `--iterations=-5` and `--repetitions=-1` are accepted and the run hangs instead of reporting a recoverable error, running no benchmark, and exiting nonzero
- [X] T045 Drive the calibration walk of `test/source/harness_calibration_test.cpp` on scripted `machine/monotonic` and `machine/thread_cpu` deltas, asserting the iteration count of each step, the step that qualifies, and a scripted step whose real time reaches 5 times the minimum time while its thread CPU time stays below it, per SC-002 and US2/AC3 (partial): the suite runs on the real clock provider and asserts only that the walk grew past one iteration and stayed inside the FR-016 bound, which is recorded deviation 8; resolving it either meets SC-002 or amends the criterion
- [X] T046 Extend `test/source/harness_cli_test.cpp` to feed every invalid value of FR-034 per SC-018 (partial): an unparseable value and a negative value for each numeric option, plus a zero repetition count, iteration count, and minimum time, each asserting a recoverable error, no benchmark function run, and a nonzero exit; the suite carries three subprocess cases today (`--min-time=0`, `--repetitions=0`, an unknown option)
- [X] T047 Add the scripted `machine/tsc` leaf with no published rate to `test/source/harness_capture_test.cpp` and assert it reports as a count with no time derived from it per FR-039 (partial): T012 names the assertion and the suite carries no tsc leaf; a manual `--counter machine/tsc` run on the reference host shows the count form holding
- [X] T048 Add the unknown-`--counter`-leaf case to `test/source/harness_cli_test.cpp` per T023 and the FR-023 edge case (partial): the resolution failure names the leaf on stderr and the benchmark still runs, but no test observes it
- [X] T049 Scale the sampling-action and allocation probe of `test/source/harness_capture_test.cpp` to 10,000 runs per SC-005 (partial): the difference of two `readActions()` windows proves two actions per run and the in-loop allocation check fires, but the observation covers one run of 8 iterations rather than the stated 10,000
- [X] T050 Settle the gap-exclusion scope in `source/harness/runner.cpp` per FR-025 (partial): the statistics of the gapped quantity drop the repetition while the time aggregate keeps its sample, and `harness_statistics_test` asserts that split; either exclude the gapped run's time sample or record the per-quantity reading against FR-025's run-level wording
- [X] T051 Justify or remove the additions beyond the artifacts: `BenchmarkResult::metricLabels` beyond E-07, the private `SG_PROJECT_VERSION` compile definition beyond T002, and the `PmuProvider` registration in `speedgunMain` where R-03 names the clock provider alone (unrequested): all three are recorded as deviations 3, 4, and 5 above, and each needs a contract line or a removal
