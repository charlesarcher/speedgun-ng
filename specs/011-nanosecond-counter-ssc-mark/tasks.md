---
description: "Task list for the nanosecond counter and simulation-start marker"
---

# Tasks: Nanosecond Counter and Simulation-Start Marker

**Input**: Design documents from `/specs/011-nanosecond-counter-ssc-mark/`

**Prerequisites**: [plan.md](plan.md), [spec.md](spec.md), [research.md](research.md), [data-model.md](data-model.md), [contracts/](contracts/), [quickstart.md](quickstart.md)

**Tests**: Test tasks are included. FR-026, FR-027, FR-036, and the coverage
gates require tests in the same change as the code, and the plan records TDD
for the counter lane.

**Organization**: Tasks are grouped by user story so each story is
independently implementable, independently testable, and independently
deliverable as an increment.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (US1 through US4)
- Include exact file paths in descriptions

## Path Conventions

Single project. Library headers in `include/speedgun-ng/`, implementation in
`source/`, tests in `test/source/` with gate scripts beside them in `test/`,
examples in `example/`, published documentation in `docs/pages/`.

Two rules govern the split, taken from the plan's Task Decomposition section:

1. One task writes one file. The exception is a file plus the registration
   that names it, which merge because the registration cannot be verified
   without the file.
2. `test/CMakeLists.txt` is the one shared registration file. Three tasks
   edit it, so those three never run in parallel and their order is fixed.

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Establish a green baseline before any edit, so a later failure
is attributable to this feature.

- [X] T001 Run the baseline gate and record the result: `cmake --preset=dev`, then `cmake --build --preset=dev`, then `ctest --preset=dev`. Record any pre-existing failure verbatim in the commit body and stop until it is classified. Verify: all three commands exit 0, or the commit body names the pre-existing failure. Touches no file.

**Checkpoint**: Baseline green. Every later task's verification is then
attributable to this feature.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core infrastructure that MUST be complete before ANY user story
can be implemented.

**None.** The plan's Task Decomposition rule 5 states there is no
foundational phase, and the reason holds here: the two lanes share no file,
no symbol, and no registration. The counter lane edits the already-globbed
`source/counters/` tree and needs no build change. The marker lane adds one
`target_sources` line inside its own implementation task. No provider, macro,
dispatch table, base class, or vocabulary change is shared, because FR-002
and FR-034 forbid all of them.

**Checkpoint**: Skipped by design. The three tasks with no dependency, T001,
T002, and T010, start together.

---

## Phase 3: User Story 1 - Open an instruction trace at a chosen point (Priority: P1) 🎯 MVP

**Goal**: A public `simulation_start` call that emits the Intel SDE SSC
marker sequence, preserves every general-purpose register, and returns on a
build targeting a processor family with no marker instruction.

**Independent Test**: Place one call in an existing example benchmark, build,
run the binary under Intel SDE with the documented tag, and confirm the trace
begins at the marked region. The marker assertion runs in the ordinary test
suite with no tracer present; the Intel SDE run is task T014.

### Implementation for User Story 1

Implementation comes before the test in this story, and the reason is
recorded so the ordering survives review. A test translation unit calling an
undefined function fails at link time, and that link error takes the whole
suite's build down with it, so no other test runs at the intermediate commit.
The constitution's Pull Request Quality section requires every commit to
compile and pass its tests so history stays bisectable.

- [X] T002 [P] [US1] Create `include/speedgun-ng/simulation.hpp`: include `speedgun-ng/speedgun-ng_export.hpp`, open `namespace sg`, and declare `SPEEDGUN_NG_EXPORT void simulation_start() noexcept;` plus `inline constexpr std::uint32_t simulation_start_tag = 0xFACEU;` (FR-011, FR-013). Give the function doxygen with `\pre none` and `\post none`, the project's spelling for a contract with nothing to assert (FR-030). In prose, publish the tag's value, the tracer option that consumes it as `-start_ssc_mark FACE`, the hex form as most-significant nibble first with no `0x` prefix, and the marker's three properties: no architectural effect, safe with no tracer attached, and perturbs a measurement it sits inside (FR-014, FR-019). Name no register, no clock, and no platform function, so the header-vocabulary scan stays clean (FR-009). Verify: `cmake --build build/dev -t dbc-gate` reports zero gaps and `cmake --build build/dev -t format-check` exits 0.
- [X] T003 [US1] Create `source/simulation/marker.cpp` and register it: guard the body with `#if defined(__x86_64__) || defined(__i386__)`, leaving an empty body with an `LCOV_EXCL` marker in the spelling `source/counters/clock_provider.cpp` uses, because no supported build can execute that arm (FR-018). Add `static_assert(simulation_start_tag <= 0xFFFFFFFFU)` so truncation fails the build in every configuration (FR-016). Emit one `__asm__ __volatile__` statement, `movl $0xFACE, %%ebx` followed by `.byte 0x64, 0x67, 0x90`, naming `ebx` in the clobber list and nothing else, so the compiler saves and restores the callee-saved register and every general-purpose register stays bit-identical (FR-012, FR-015). Leave the memory clobber off and record the reasoning in a comment: an attached tracer observes executed-instruction order directly (FR-017, FR-022). Build the project, read the analyzer output the compile launchers produce for this unit, and add the one suppression the statement needs with its check name and reason on the same line (FR-021). Add `target_sources(speedgun-ng_speedgun-ng PRIVATE source/simulation/marker.cpp)` beside the other explicit additions in the root `CMakeLists.txt`; the `source/counters/` glob does not reach this path. Verify: `cmake --build --preset=dev` exits 0, the object's text holds the eight bytes `bb ce fa 00 00 64 67 90` once, and the analyzer names exactly one suppressed check whose reason sits on the same line.

### Tests for User Story 1

- [X] T004 [P] [US1] Create `test/source/simulation_test.cpp` following the hand-rolled `check()`/`fail()` convention with its own `main` as `test/source/counters_tsc_test.cpp` does, add no test framework, then register it in `test/CMakeLists.txt` with the `add_executable`, `target_link_libraries(speedgun-ng::speedgun-ng)`, `target_compile_features(cxx_std_23)`, and `add_test` block every other test uses. Assert `simulation_start_tag == 0xFACEU` (FR-013); load a known 64-bit value whose upper 32 bits are set into RBX, call `simulation_start()`, and assert RBX is bit-identical afterwards (FR-026, the case an implementation preserving only the low half fails); call it in a loop and assert normal termination (FR-027). Include `speedgun-ng/simulation.hpp`. Verify: `ctest --preset=dev -R simulation` exits 0.

### Integration for User Story 1

- [X] T005 [P] [US1] Add one `simulation_start()` call in `example/counters_standalone_example.cpp` at the region boundary, outside the timed window, with a one-line comment saying why it sits outside (FR-019's perturbation note). Include `speedgun-ng/simulation.hpp`. Verify: `ctest --preset=dev -R counters_standalone_example` exits 0.

**Checkpoint**: User Story 1 is complete and independently testable. The
marker is public, registered, callable, register-preserving, and reachable
from an example binary.

---

## Phase 4: User Story 2 - Measure elapsed time immune to clock frequency adjustment (Priority: P2)

**Goal**: A nanosecond-unit counter published at `machine/monotonic_raw` on
the machine root, read through the platform's fast path, whose rate the
operating system leaves alone.

**Independent Test**: Sample the counter in a loop, assert consecutive
samples never decrease, assert agreement with the existing nanosecond counter
to within a stated rate tolerance, and record the per-read cost distribution
on the reference platform.

### Tests for User Story 2

- [X] T006 [US2] Create `test/source/counters_clock_raw_test.cpp` in the same hand-rolled convention, then register it in `test/CMakeLists.txt` with the four-line block every test uses. Declare the local alias `using time_dim = dim<1, 0>;` as the examples and tests do. Resolve the leaf through `system::local().object("machine")` then `counter<time_dim>("monotonic_raw")` and assert the resolution succeeds with no architecture guard (FR-001, FR-005); assert `unit_token()` reads `nanoseconds` and `object::counters()` reports read mode `syscall` (FR-002); sample ten million consecutive reads on one thread and assert no decrease (FR-006); sample on every logical processor, join, and assert no decrease against the joined samples (FR-007); call `clock_getres` and assert the smallest non-zero step stays within the reported resolution (FR-008); print the per-read cost distribution over ten million reads (FR-008, and the convention in research.md R-006). Verify: `ctest --preset=dev -R counters_clock_raw` FAILS, naming the unresolved address. The failure is the expected first result.
- [X] T007 [US2] Implement the leaf in `source/counters/clock_provider.cpp`: add `machine/monotonic_raw` to the canonical address table beside the existing four, add its case to the address parser and to the read switch, add a `monotonic_raw_ns()` reader in the anonymous namespace that calls `clock_gettime(CLOCK_MONOTONIC_RAW, &stamp)` through `<ctime>` and returns `static_cast<std::uint64_t>(stamp.tv_sec) * 1000000000ULL + static_cast<std::uint64_t>(stamp.tv_nsec)` with integer arithmetic only, keeping the existing failure arm's exclusion marker, and add the seed entry to the provider's entry list with unit `nanoseconds` and mode `read_mode::syscall` (FR-001, FR-003, FR-004, FR-034). Add no enumerator, no provider, no macro, and no dispatch entry; the `source/counters/*.cpp` glob already compiles this file. Verify: `ctest --preset=dev -R counters_clock_raw` exits 0 and `ctest --preset=dev` stays green.

### Implementation for User Story 2

- [X] T008 [P] [US2] Add the new counter to the measurement harness in `test/source/counters_overhead.cpp` so it prints a raw-trial line for `machine/monotonic_raw` in the same field order as the existing clock line, and publish the row in `docs/pages/counters-overhead.md` in the existing five-column format `| plan | min ns | median ns | max ns | read mode |`, once per build-configuration table, with the read-path cell naming the platform fast path and stating that no system call is involved (FR-008, FR-010). Keep the raw trial block beneath the tables in step with the new row. Verify: `ctest --preset=dev -R counters_overhead -V --repeat until-pass:3` exits 0 and its printed minimum, median, and maximum match the published row.

**Checkpoint**: User Story 2 is complete and independently testable. The
counter resolves, samples monotonically, and carries a published cost row.

---

## Phase 5: User Story 3 - Keep the emitted instruction faithful on every build (Priority: P2)

**Goal**: An automated gate that fails the moment the emitted instruction
sequence changes, on both supported compilers, with no tracer installed.

**Independent Test**: Deliberately alter the emitted sequence in a scratch
copy, run the gate, and confirm it fails; restore the sequence and confirm it
passes.

- [X] T009 [P] [US3] Create `test/simulation_mark_shape.sh` modeled on `test/counters_tsc_read_shape.sh`: iterate over each of `g++` and `clang++` found on `PATH`, and for each iterate `SG_CONTRACTS_SEMANTIC` 0 and 2; compile `source/simulation/marker.cpp` at `-O2 -std=c++23` against `include/` and the generated export directory; disassemble the object with `objdump -d`; assert the eight bytes `bb ce fa 00 00 64 67 90` appear exactly once, which is the in-function count because the unit holds exactly one function (FR-012, FR-023). Plant a deliberately wrong window in a scratch copy and require the assertion to fail, so a passing run cannot come from a detector that inspects nothing (FR-024). Read each compiler's target triple and report a skip with exit 0 when none targets x86, because FR-025 speaks of the compiler's target, and the host's architecture is a different question (FR-025). Register it in `test/CMakeLists.txt` with `add_test(NAME simulation_mark_shape COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/simulation_mark_shape.sh ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})` inside the existing Linux-only region, after T004's edit to that file. Verify: `ctest --preset=dev -R simulation_mark_shape` exits 0, and the output names each compiler and each contract setting it exercised.

**Checkpoint**: User Story 3 is complete and independently testable. The
marker bytes are now covered by a gate that runs in every test pass, with a
probe proving the detector bites.

---

## Phase 6: User Story 4 - Keep the new surface inside the existing vocabulary (Priority: P3)

**Goal**: The new counter reads like the existing counters, the new API reads
like the rest of the public surface, and a consumer of the installed package
sees the new header.

**Independent Test**: Run the existing header-vocabulary scan and the
existing documentation-to-enforcement pairing gate; both pass with the new
files added.

- [X] T010 [P] [US4] Widen the scanned set in `test/counters_header_purity.sh` so it covers `include/speedgun-ng/simulation.hpp` alongside the existing `counters*.hpp` glob, keeping the banned-term list at its present spelling: `perf_event`, `clock_gettime`, whole-word `rdpmc`, `rdtsc`, and the acronym `PMU` across the core counter headers (FR-009, FR-029). Add the widened-set probe: plant one banned token in the new header, confirm the scan fails naming the file and line, then restore the header and confirm it passes. Verify: `ctest --preset=dev -R counters_header_purity` exits 0.
- [X] T011 [US4] Prove the installed package exposes the new header: `cmake --install build --prefix "$PWD/prefix"`, then configure, build, and run `test/consumer` against that prefix exactly as `test/consumer/CMakeLists.txt` directs, and confirm the new header resolves through the package's documented include path while the consumer's link manifest names no tracer library (FR-028, User Story 4 scenario 3). Verify: all three commands exit 0 and the manifest names no tracer library.

**Checkpoint**: User Story 4 is complete and independently testable. The
vocabulary scan records a verdict on the new header on every run, and the
downstream consumer links the new surface.

---

## Phase 7: Polish & Cross-Cutting Concerns

- [X] T012 Run the release-configuration build and the full gate sweep, in this order: `cmake --preset=ci-ubuntu`, `cmake --build build`, `ctest --preset=dev`, `cmake --build build/dev -t dbc-gate`, `cmake --build build/coverage -t coverage`, `cmake --build build/dev -t format-check`, `cmake -P cmake/spell.cmake`, `cmake -P cmake/prose-lint.cmake`. Record every exit code in the commit body. Verify: every command exits 0, the coverage summary reports 100 percent line and 100 percent branch over the added code (FR-036), and the prose gate reports zero findings over this feature's range.
- [X] T013 [P] Run the local rate-agreement confirmation from [quickstart.md](quickstart.md): sample `machine/monotonic_raw` and `machine/monotonic` once per second for at least sixty seconds and divide the difference of the two elapsed intervals by the interval measured with `machine/monotonic_raw` (SC-006). Record the command and the parts-per-million result in the commit body. Verify: the ratio is within 20 parts per million, or the commit body records a clocksource switch or a suspend and the run is repeated.
- [X] T014 [P] Run the local tracer confirmation from [quickstart.md](quickstart.md): `sde64 -start_ssc_mark FACE:repeat -- <example binary>` over the marker placed by T005 (SC-001). Record the command and where the trace's first collected instruction falls in the commit body. Verify: the first collected instruction falls inside the marked region, or the commit body records that Intel SDE is absent from this machine and the confirmation is deferred.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies. T001 can start immediately.
- **Foundational (Phase 2)**: Empty by design. No task blocks a lane it does
  not belong to.
- **User Story 1 (Phase 3)**: Depends on T001 only. Starts immediately.
- **User Story 2 (Phase 4)**: Depends on T001 only. Starts immediately and
  runs alongside User Story 1.
- **User Story 3 (Phase 5)**: Depends on User Story 1's marker unit (T003)
  and on T004's edit to `test/CMakeLists.txt`. The dependency on the marker is
  inherent to the story, which exists to verify that marker.
- **User Story 4 (Phase 6)**: T010 depends on T001 and touches a test script
  the header scan owns, so it starts immediately. T011 depends on T002, since
  the header must exist to be installed.
- **Polish (Phase 7)**: T012 depends on every story. T013 depends on T008.
  T014 depends on T005 and T009.

### User Story Dependencies

- **User Story 1 (P1)**: No dependency on another story. Deliverable first.
- **User Story 2 (P2)**: No dependency on another story. Shares no file, no
  symbol, and no registration with User Story 1.
- **User Story 3 (P2)**: Depends on User Story 1's marker existing. Its own
  deliverable, the gate script, is a new file and is independently testable.
- **User Story 4 (P3)**: T010 depends on nothing; T011 depends on User Story
  1's header.

### Within Each User Story

- User Story 2 uses TDD: T006 is written and observed failing before T007
  turns it green.
- User Story 1 is implementation-first, for the reason given in its tests
  section.
- User Story 3's gate observes red before green on its own, because it
  compiles the marker unit itself and finds no window until the statement
  exists.
- `test/CMakeLists.txt` edits happen in T004, then T006, then T009. That order
  is fixed.
- Every task is verified with a build plus `ctest --preset=dev` before the
  next begins.

### Parallel Opportunities

- T001, T002, and T010 carry no dependency and touch four different files.
  They start together.
- T004 and T005 run together once T003 lands; they touch different files.
- T008 and T009 run alongside each other and alongside T007's tail.
- T013 and T014 run alongside each other and alongside T012's build steps.
- The counter lane and the marker lane share nothing, so two workers can take
  one lane each and never meet in a file.

---

## Parallel Example: the two lanes at the start

```bash
# Launch together, four distinct files:
Task: "T001 Run the baseline gate and record the result"
Task: "T002 [P] [US1] Create include/speedgun-ng/simulation.hpp"
Task: "T010 [P] [US4] Widen the scanned set in test/counters_header_purity.sh"

# Launch together once T003 lands, different files:
Task: "T004 [US1] Create test/source/simulation_test.cpp and register it"
Task: "T005 [P] [US1] Add one simulation_start() call in example/counters_standalone_example.cpp"

# Launch together once their own inputs land, different files:
Task: "T008 [P] [US2] Add the new counter to test/source/counters_overhead.cpp and publish the row"
Task: "T009 [P] [US3] Create test/simulation_mark_shape.sh and register it"
```

---

## Requirement Coverage

| Requirement | Tasks |
|-------------|-------|
| FR-001, FR-002, FR-003, FR-004, FR-005 | T006, T007 |
| FR-006, FR-007, FR-008 | T006 |
| FR-009 | T002, T010 |
| FR-010 | T008 |
| FR-011, FR-013, FR-014, FR-019, FR-020 | T002 |
| FR-012, FR-015, FR-016, FR-017, FR-018, FR-021, FR-022, FR-028 | T003 |
| FR-023, FR-024, FR-025 | T009 |
| FR-026, FR-027 | T004 |
| FR-029, FR-030 | T010, T011 |
| FR-032, FR-033, FR-034, FR-035 | T003, T007, T012 |
| FR-036 | T004, T006, T012 |
| SC-001 | T014 |
| SC-006 | T013 |
| SC-004, SC-005 | T006, T008, T013 |

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. T001
2. T002
3. T003
4. T004, T005 together
5. **STOP and VALIDATE**: `ctest --preset=dev -R simulation`, then the Intel
   SDE run from T014 if the tool is installed.

### Incremental Delivery

1. T001, T002, T010, then T003, then T004 and T005: the marker ships, and the
   surface stays inside the existing vocabulary.
2. T006, T007, T008: the counter ships with its published cost row.
3. T009: the marker gains its codegen gate.
4. T011, T012, T013, T014: the consumer proof, the release sweep, and the two
   local confirmations.

### Parallel Team Strategy

With two workers:

1. Both take T001, then split. Worker A takes T002 through T005 and T009.
   Worker B takes T010, T006, T007, T008, and T011.
2. The two workers meet at `test/CMakeLists.txt`: A edits it in T004, B in
   T006, and both again in T009. Hand the file over in that order.
3. Both take T012 through T014 at the end.

---

## Notes

- [P] tasks touch different files and have no unfinished dependency.
- Every task names its verify command and the requirement identifiers it
  discharges.
- Order the task identifiers topologically. The order given here is one valid
  topological order, and the shared registration file fixes the relative order
  of T004, T006, and T009.
- T006 is expected to fail on its first run. A passing T006 means the test
  asserts nothing.
- The plan merged the marker's runtime test and its gate script into one task
  to save a dependency edge. Story-keyed organization takes precedence here,
  so they are T004 and T009, and the cost is one serialization on
  `test/CMakeLists.txt` that rule 2 already imposes.
- All four specification findings in [research.md](research.md) are closed as
  of 2026-10-03: SF-001 by the maintainer's directive permitting build-file
  edits, SF-002 by FR-007's added happens-before condition and User Story 2's
  fourth acceptance scenario, SF-003 by SC-004's named measurement convention
  and SC-006's local-confirmation placement, and SF-004 by the second
  clarification session naming `__SSC_MARK`. No task is blocked.
- Commit after each task or logical group, with the constitution's template.
- Stop at any checkpoint to validate a story independently.

---

## Phase 8: Convergence

Remaining work found by assessing the code against `spec.md`, `plan.md`, and
this file after Phase 7 closed. Ordered CRITICAL and HIGH first.

- [X] T015 Bind the catalog entry vector to a named local before `find_entry`
  returns a pointer into it in
  `test/source/counters_clock_raw_test.cpp:301`: `object::counters()` returns
  `std::vector<catalog_entry>` by value
  (`include/speedgun-ng/counters_system.hpp:90`), so the temporary is destroyed
  at the end of that full expression and line 303 reads freed memory. Copy the
  pattern `counters_tsc_test.cpp:167`, `counters_pmu_test.cpp:231`, and
  `counters_clock_push_test.cpp:245` already use, where a named
  `const auto entries = ...` outlives the pointer. Severity CRITICAL:
  `ctest --test-dir build/sanitize -R counters_clock_raw` exits non-zero with
  `AddressSanitizer: heap-use-after-free ... in main
  counters_clock_raw_test.cpp:303`, and Principle VIII's sanitizer clause is
  hard; it is the only failure among the 45 tests in that preset, and the
  developer and release builds pass because the freed block still holds the
  value. Verify: `cmake --build build/sanitize` exits 0,
  `ctest --test-dir build/sanitize` reports 45 of 45 passed, and
  `ctest --preset=dev -R counters_clock_raw` exits 0
  (Constitution VIII; contradicts)

- [X] T016 Document both sample-ordering guarantees in the library's shipped
  text, since nothing states either one today: state that a sample is greater
  than or equal to the immediately preceding sample of the same counter on the
  same thread, and that a sample is greater than or equal to an earlier sample
  of the same counter whose completion happens-before the present sample's
  start. Record the declaration that carries the wording in the commit body.
  Every counter interface in `include/speedgun-ng/counters_measurement.hpp` and
  `include/speedgun-ng/counters_core.hpp` carries `\post none`, and no page
  under `docs/pages/` mentions sample monotonicity; only
  `specs/011-nanosecond-counter-ssc-mark/contracts/monotonic-raw-counter.md`
  states them, and a specification artifact is not the library's documentation.
  The wording must satisfy FR-009 (no clock source name, clock identifier, or
  clock reading function in a public header), Principle II's rule that a
  contract is stated in one place with no unenforced copy beside it, and
  FR-031 (a compile-time assertion where nothing is observable at run time).
  Record in the commit body which declaration carries the wording and why, given
  that the shared sampling declaration also serves the event-count leaves.
  Verify: `cmake --build build/dev -t dbc-gate` reports zero gaps on
  137 interfaces, `ctest --preset=dev -R counters_header_purity` exits 0,
  `cmake --build build/dev -t format-check` exits 0, and
  `ctest --preset=dev -R prose_gate_fixtures` exits 0
  (FR-006, FR-007; missing)

- [X] T017 Name `monotonic_raw` in the leaf enumeration of the `clock_provider`
  class documentation at `include/speedgun-ng/counters_clock.hpp:27-34`, which
  today names `monotonic`, `thread_cpu`, `process_cpu`, and `tsc`, while
  `clock_provider::enumerate` seeds a fifth leaf at
  `source/counters/clock_provider.cpp:289` that publishes on every supported
  build. Touch no line outside that sentence.
  Verify: `cmake --build build/dev -t dbc-gate` reports zero gaps,
  `ctest --preset=dev -R counters_header_purity` exits 0, and
  `cmake --build build/dev -t format-check` exits 0
  (FR-001, Constitution IV; partial)

- [X] T018 Scale the cross-processor read volume in
  `test/source/counters_clock_raw_test.cpp` to the figure SC-005 names:
  `kReadsPerProcessor` is 10,000 at line 72 and the pass starts one thread per
  `std::thread::hardware_concurrency()`, so the 32-logical-processor reference
  platform takes 320,000 reads where SC-005 states ten million reads spread over
  every logical processor. Derive the per-thread count from the processor count
  so the aggregate reaches ten million on any host, keep every assertion and
  the printed summary, and record the run's wall time in the commit body.
  Verify: `ctest --preset=dev -R counters_clock_raw -V` exits 0, its output
  prints `0 regressions`, and `ctest --preset=dev` stays green (SC-005; partial)

- [X] T019 Measure the bracketing overhead and the p99 in
  `test/source/counters_clock_raw_test.cpp` and report them beside the per-read
  figure, then correct the header comment at lines 16-17 that already claims
  the overhead is measured and printed. Today `test_monotonic_and_cost`
  brackets each `recorder.sample()` between two timestamp-counter reads at
  lines 140-148 and prints only the minimum, the median, the maximum, and the
  calibrated rate at lines 194-199, so SC-004's measurement convention is
  undischarged in code. Take the overhead under the identical bracketing by
  bracketing an empty interval the same way, print the overhead-corrected and
  the uncorrected per-read figures side by side with the p99, and publish the
  same pair beside the `machine/monotonic_raw` row in
  `docs/pages/counters-overhead.md`. Keep SC-004's numeric thresholds out of
  the test's pass or fail verdict, because Principle VI requires the suite to
  run in every CI job. Verify:
  `ctest --preset=dev -R counters_clock_raw -V` exits 0 and its output
  names the bracketing overhead and the p99,
  `ctest --preset=dev -R counters_overhead -V` exits 0,
  `cmake -P cmake/spell.cmake` exits 0, and
  `cmake -P cmake/prose-lint.cmake` reports zero findings
  (SC-004; contradicts)

---

## Phase 9: Convergence

Remaining work found by assessing the code against `spec.md`, `plan.md`, and
this file after Phase 8 closed. Every gate passes on this tree: the developer
and sanitizer suites report 45 of 45, the release build compiles, the contract
gate reports 137 interfaces and 0 gaps, coverage reports 100 percent line and
100 percent branch, and the format, spelling, and prose gates exit 0. No
finding in this pass carries CRITICAL or HIGH severity, so no constitution
principle is at stake and no baseline functionality is blocked. Relative paths
in the verify commands resolve from this feature's directory.

- [X] T020 Restate the smallest-step clause in the three artifacts that carry
  it, so the requirement matches the bound the code asserts and the platform
  reports. `spec.md` FR-008 and User Story 2 acceptance scenario 3 read that
  the smallest observed non-zero step stays within the resolution
  `clock_getres` reports, and `contracts/monotonic-raw-counter.md` guarantee 3
  carries the same sentence, while
  `test/source/counters_clock_raw_test.cpp:187` asserts
  `smallest_step >= resolution_bound`. The shipped run reads a reported
  resolution of 1 ns against a smallest observed step of 70 ns, so the clause
  as written is false on the reference platform. Restate it in `spec.md`
  FR-008, in `spec.md` User Story 2 scenario 3, and in
  `contracts/monotonic-raw-counter.md` guarantee 3 as the granularity bound:
  no observed step is finer than the resolution the platform reports for this
  clock. Record the resolution as a fifth entry in the Specification Findings
  section of `research.md`, naming the measured 1 ns resolution, the 70 ns
  smallest observed step, and the reasoning already carried at
  `test/source/counters_clock_raw_test.cpp:179-189`, and correct the
  registration comment at `test/CMakeLists.txt:416`, which repeats the clause.
  Verify: run from this feature's directory, `grep -c "no difference exceeds
  the resolution" spec.md` prints `0`, `grep -c "stays within the resolution"
  contracts/monotonic-raw-counter.md` prints `0`, `ctest --preset=dev -R
  counters_clock_raw` exits 0 with its output naming `0 decreases`, and
  `cmake -P cmake/prose-lint.cmake` exits 0
  (FR-008, US2/AC3; partial)

- [X] T021 Make the downstream consumer compile the new public header, so the
  install-tree reachability of `include/speedgun-ng/simulation.hpp` carries a
  gate. `test/consumer/main.cpp:12` includes `<speedgun-ng/speedgun-ng.hpp>`
  alone and instantiates `exported_class` only, so the `downstream-consumer`
  job at `.github/workflows/ci.yml:307` compiles and links the installed
  package with the new header absent from every translation unit it builds,
  and User Story 4 scenario 3's reachability clause and SC-009's reachable
  header rest on the install rule in `cmake/install-rules.cmake:15` alone.
  Add `#include <speedgun-ng/simulation.hpp>` and one call to
  `sg::simulation_start()` in `test/consumer/main.cpp`, keep the file's rule
  that it names no vendored library, and extend the file's opening comment so
  it describes both public headers the consumer now compiles. Verify:
  `cmake --install build --prefix "$PWD/prefix"` exits 0,
  `cmake -S test/consumer -B build-consumer -D CMAKE_PREFIX_PATH="$PWD/prefix"`
  exits 0, `cmake --build build-consumer` exits 0, `./build-consumer/consumer`
  exits 0, and `grep -c "speedgun-ng/simulation.hpp" test/consumer/main.cpp`
  prints `1`
  (US4/AC3, SC-009; partial)

- [X] T022 Add the widened-set probe that `plan.md:278` and T010 name for
  FR-029. `test/counters_header_purity.sh:69` and `:72` scan
  `--include='simulation.hpp'`, so the scan records a verdict on the new
  header on every run, and dropping either include leaves the scan exiting 0
  while inspecting nothing, the failure mode FR-024 guards against on the
  marker gate. Copy the negative-probe shape at
  `test/counters_tsc_read_shape.sh:189`: copy `$ROOT/include/speedgun-ng` into
  a `mktemp -d` directory, append the token `clock_gettime` to the copied
  `simulation.hpp`, require the scan over that copy to exit 1 naming the
  copied file and line, then require the scan over the real tree to exit 0.
  Verify: `ctest --preset=dev -R counters_header_purity` exits 0 with its
  output naming the detected planted token,
  `cmake --build build/dev -t format-check` exits 0, and
  `cmake -P cmake/prose-lint.cmake` exits 0
  (FR-029, plan: widened-set probe; partial)

---

## Phase 10: Convergence

Remaining work found by assessing the code against `spec.md`, `plan.md`, and
this file after Phase 9 closed. Every gate passes on this tree: the developer
and sanitizer suites report 45 of 45, the release build compiles, the contract
gate reports 137 interfaces and 0 gaps with `sg::simulation_start` among them,
coverage reports 100 percent line and 100 percent branch, the format,
spelling, and prose gates exit 0, and the installed package compiles the new
header in the downstream consumer. The findings below concern the measurement
record for the per-read cost, where two artifacts name a platform and a read
volume the recorded run does not carry. Relative paths in the verify commands
resolve from this feature's directory.

- [X] T023 Align SC-004's reference platform and read volume with the
  measurement the record holds, per SC-004 and plan: Performance
  Goals (partial). SC-004 names the reference platform as AMD Ryzen 9
  9950X3D, Linux 6.19, glibc 2.44 over ten million consecutive reads, and
  `research.md:165` calls its own run "the reference platform named in
  SC-004" while `research.md:166` records 200,000 samples per row on
  `Linux 7.2.4-1-cachyos`, and `plan.md:67-68` reads "Measured on that
  platform" for the same figures. `uname -r` prints `7.2.4-1-cachyos` and
  `ldd --version` prints `2.44`, so the kernel differs and the other two
  entries hold. Restate the platform and the read volume in `spec.md`
  SC-004 to the machine and the volume `research.md` R-006 records, keep
  the thresholds and the recorded figures as they stand, and correct the
  sentence at `research.md:165` and the sentence at `plan.md:67-68`. Verify:
  `grep -c "Linux 6.19" spec.md` prints `0`,
  `grep -c "the reference platform named in SC-004" research.md` prints `0`,
  `grep -c "Measured on that platform" plan.md` prints `0`,
  `cmake -P cmake/spell.cmake` exits 0, and
  `cmake -P cmake/prose-lint.cmake` exits 0

- [X] T024 Record how FR-008's third clause is discharged, per FR-008 and
  Constitution X.1 (partial). FR-008 requires a test to establish that the
  per-read cost distribution matches the published figure for the reference
  platform, and the Verification Matrix row at `plan.md:257` names
  monotonicity, the resolution bound, and the cost distribution, with the
  matching clause absent and no artifact recording the displacement.
  `docs/pages/counters-overhead.md:326` states that the page's numbers are
  no CI gate, which is the reason an assertion cannot carry the clause.
  Name the reviewer comparison that discharges the clause and that reason in
  the FR-008 row. Verify: `sed -n '/^| FR-008/p' plan.md | grep -c reviewer`
  prints `1` and `cmake -P cmake/prose-lint.cmake` exits 0

- [X] T025 Drop the retired clause wording from the comment at
  `test/source/counters_clock_raw_test.cpp:179-181`, per US2/AC3 and T020
  (partial). The comment opens by quoting "no difference exceeds the
  resolution" as the wording of User Story 2's third acceptance scenario,
  and T020 restated that scenario to the granularity bound, so the comment
  now argues against a clause the specification no longer carries. Keep the
  sentences explaining why the assertion reads `smallest_step >=
  resolution_bound`. Verify:
  `grep -c "no difference exceeds the resolution"
  test/source/counters_clock_raw_test.cpp` prints `0`,
  `ctest --preset=dev -R counters_clock_raw` exits 0, and
  `cmake --build build/dev -t format-check` exits 0

---

## Phase 11: Convergence

Remaining work found by assessing the code against `spec.md`, `plan.md`, and
this file after Phase 10 closed. Every gate passes on this tree: the developer
and sanitizer suites report 45 of 45, the release build compiles, the contract
gate reports 137 interfaces and 0 gaps with `sg::simulation_start` among them,
coverage reports 100 percent line and 100 percent branch, the format,
spelling, and prose gates exit 0, and the installed package compiles the new
header in the downstream consumer. The marker lane is closed end to end: the
gate bites for both compilers under both contract settings, the widened
vocabulary scan bites on a planted token, the marker unit compiles clean under
the pinned analyzer once its one suppression carries its reason, and the
install tree carries the new header. Both findings below concern which cost
quantity the counter's measurement record names. Relative paths in the verify
commands resolve from this feature's directory.

- [X] T026 Name the figure SC-004's bounds judge in the paragraph at
  `docs/pages/counters-overhead.md:195-210`, whose closing sentence reads
  "SC-004's thresholds judge the corrected figure" at lines 207-208 directly
  after publishing the corrected per-sampling-action figure at 40 ns median and
  40 ns at the 99th percentile. `spec.md:451-460` states the opposite scope:
  SC-004's bounds are 20 ns at p50 and 25 ns at p99 on the
  overhead-corrected cost of one read over 200,000 reads, and the
  per-sampling-action figures set no threshold there because they cover the
  library's own machinery. A reader who takes the page's sentence at its word
  checks 40 ns against a 20 ns bound. Name the per-read figure R-006 records
  as the one the bounds judge, carry SC-004's exclusion of the
  per-sampling-action figures into the same paragraph, and touch no table row,
  no figure, and no bracketing sentence. Verify: `grep -c "judge the corrected
  figure" docs/pages/counters-overhead.md` prints `0`,
  `cmake -P cmake/spell.cmake` exits 0, and `cmake -P cmake/prose-lint.cmake`
  exits 0
  (SC-004; contradicts)

- [X] T027 Restate the third clause of FR-008 at `spec.md:299-304` to name the
  cost distribution the test prints, and correct the registration comment at
  `test/CMakeLists.txt:417` that repeats the clause. The clause reads "that the
  per-read cost distribution matches the published figure for the reference
  platform", while `counters_clock_raw_test` prints "per sampling action over
  ten million reads" at
  `test/source/counters_clock_raw_test.cpp:209`, bracketing one
  `recorder.sample()` on a plan holding one leaf, because no public API
  reaches the provider-level read `monotonic_raw_ns()` performs. R-006 and
  SC-004 keep the two quantities apart, so the clause as written names a
  distribution no run measures. Name the per-sampling-action distribution and
  the page row it is checked against, and record in the clause the reason the
  per-read quantity has no instrument in the suite. Touch no other clause of
  FR-008 and no assertion in the test. Verify: `sed -n '/^- \*\*FR-008\*\*/,
  /^- \*\*FR-009\*\*/p' spec.md | grep -c per-read` prints `0`, `grep -c
  "per-read cost distribution" test/CMakeLists.txt` prints `0`,
  `ctest --preset=dev -R counters_clock_raw` exits 0, and
  `cmake -P cmake/prose-lint.cmake` exits 0
  (FR-008; partial)

---

## Phase 12: Convergence

Remaining work found by assessing the code against `spec.md`, `plan.md`, and
this file after Phase 11 closed. Every gate passes on this tree: the developer
and sanitizer suites report 45 of 45, the release build compiles, the contract
gate reports 137 interfaces and 0 gaps, coverage reports 100 percent line and
100 percent branch, the format, spelling, and prose gates exit 0, the marker
gate finds the window once for each of two compilers under each contract
setting and detects its planted wrong window, the widened vocabulary scan
bites on a planted token, and the installed package compiles the new header in
a downstream consumer configured against a fresh build directory. Both marker
findings of the earlier passes stay closed. No finding in this pass carries
CRITICAL or HIGH severity. A verify command naming a file under this feature's
directory runs from that directory; every other one runs from the repository
root.

- [X] T028 Name the overhead-corrected per-read median with the figure the
  measurement record supports, in the paragraph at
  `docs/pages/counters-overhead.md:207-211`, per SC-004 and FR-010
  (contradicts). The sentence reads "SC-004's bounds judge the
  overhead-corrected cost of one read, the figure recorded in
  `specs/011-nanosecond-counter-ssc-mark/research.md` R-006 at 20 ns at the
  median and near 22 ns at the 99th percentile once the 7.36 ns bracketing
  overhead is subtracted", so the page publishes 20 ns as the corrected
  figure. R-006 records the uncorrected median as 20.00 ns, the uncorrected
  99th percentile as 29.77 ns, and the bracketing overhead as 7.36 ns on
  average, and its own sentence calls the 20.00 ns the uncorrected figure
  that meets SC-004's 20 ns bound. Subtracting that overhead from the median
  under the convention R-006 applies to the 99th percentile gives 12.6 ns, so
  a reader checks 20 ns against a 20 ns bound and reads a figure sitting at
  the bound where the record puts it below it. Publish the corrected median
  as 12.6 ns with the two figures it comes from named beside it, keep the
  20.00 ns labeled uncorrected, and touch no table row, no figure inside the
  tables, and no bracketing sentence. Verify:
  `grep -c "at 20 ns" docs/pages/counters-overhead.md` prints 0,
  `grep -c "12.6 ns" docs/pages/counters-overhead.md` prints 1,
  `cmake -P cmake/spell.cmake` exits 0, and
  `cmake -P cmake/prose-lint.cmake` exits 0

- [X] T029 Add the two rows this change made necessary to the Physical view
  table at `plan.md:227-240` and correct the file count at `plan.md:80`, per
  FR-035 and plan: Physical view table (partial). The table names twelve
  files, and the Verification Matrix row at `plan.md:284` names that table as
  the gate proving FR-035, so every changed line traces to a requirement
  through it. Two files the change edited have no row:
  `include/speedgun-ng/counters_clock.hpp`, which T016 and T017 edited to
  carry FR-001, FR-006, and FR-007, and `test/consumer/main.cpp`, which T021
  edited to carry FR-028 and SC-009. Technical Context reads "twelve files
  touched, five of them new", and the change now touches fourteen of them,
  five new. Add one row per file in the table's existing column format,
  carrying the requirement identifiers the tasks recorded, and correct the
  count beside it. Verify: `grep -c "counters_clock.hpp" plan.md` prints 1,
  `grep -c "test/consumer/main.cpp" plan.md` prints 1,
  `grep -c "twelve files touched" plan.md` prints 0, and
  `cmake -P cmake/prose-lint.cmake` exits 0

- [X] T030 Drop the `[[nodiscard]]` attribute from the declaration block at
  `contracts/simulation-start.md:23`, per FR-011 and plan: Complexity Tracking
  (contradicts). The block spells
  `[[nodiscard]] void simulation_start() noexcept;`, FR-011 fixes the
  signature as callable with no arguments, returning nothing, and declared
  `noexcept`, and the shipped declaration at
  `include/speedgun-ng/simulation.hpp:49` carries no such attribute. A
  function returning nothing has no value for a caller to discard, so the
  attribute states nothing the requirement asks for, and the bullet list
  below the block already enumerates the return type, the empty parameter
  list, `noexcept`, and the export attribute without it. Remove the
  attribute from the block and leave the bullet list, the emitted-sequence
  table, and the tracer command-line section untouched. Verify:
  `grep -c "nodiscard" contracts/simulation-start.md` prints 0,
  `cmake -P cmake/spell.cmake` exits 0, and
  `cmake -P cmake/prose-lint.cmake` exits 0

---

## Phase 13: Convergence

Remaining work found by assessing the code against `spec.md`, `plan.md`, and
this file after Phase 12 closed. Every gate passes on this tree: the developer
and sanitizer suites report 45 of 45, the release build compiles, the contract
gate reports 137 interfaces and 0 gaps, coverage reports 100 percent line and
100 percent branch, the format, spelling, and prose gates exit 0, the marker
gate finds the window once for each of two compilers under each contract
setting and detects its planted wrong window, the widened vocabulary scan bites
on a planted token, and the installed package compiles the new header in a
downstream consumer. No finding in this pass carries CRITICAL or HIGH
severity. Each finding is one statement an artifact still carries after the
change corrected that same fact elsewhere. A verify command naming a file under
this feature's directory runs from that directory; every other one runs from
the repository root.

- [X] T031 Bring `plan.md`'s file ledger into agreement with its Physical view
  table: add `include/speedgun-ng/counters_clock.hpp` and
  `test/consumer/main.cpp` to the "Files this feature edits" block at
  `plan.md:150-160`, one line each in that block's existing format. Correct
  the lane counts in the Summary at `plan.md:35-36` as well. That sentence
  reads "The counter lane touches four files, the marker lane four, and two
  files sit outside both", so it accounts for ten of the fourteen files the
  table at `plan.md:227-242` names. The Verification Matrix row for FR-035
  names that table as the gate, so the ledger around it has to reach the same
  fourteen. Verify: `grep -c "counters_clock.hpp" plan.md` prints `2`,
  `grep -c "test/consumer/main.cpp" plan.md` prints `2`,
  `grep -c "the marker lane four, and two files sit outside both" plan.md`
  prints `0`, and `cmake -P cmake/prose-lint.cmake` exits 0
  (plan: Physical view table, FR-035; partial)

- [X] T032 Restate the smallest-step row of the Test Plan table at
  `plan.md:296` to the granularity bound the test asserts. The row reads
  "smallest non-zero step within the platform resolution".
  `test/source/counters_clock_raw_test.cpp:187` asserts
  `smallest_step >= resolution_bound`. `spec.md` FR-008 and User Story 2
  acceptance scenario 3 state that no observed step is finer than the
  resolution the platform reports. A shipped run prints a smallest non-zero
  step of 70 ns against a reported resolution of 1 ns. Write the row in the
  requirement's form, keep its Level, Requirement, and Command cells, and
  touch no other row of the table. Verify:
  `grep -c "within the platform resolution" plan.md` prints `0`,
  `grep -c "finer than the resolution" plan.md` prints `1`, and
  `cmake -P cmake/prose-lint.cmake` exits 0
  (plan: Test Plan, FR-008, US2/AC3; partial)

- [X] T033 Restate the smallest-step clause in the expected-outcome list at
  `quickstart.md:40-45` to the bound the suite's named check prints. The list
  reads "the smallest non-zero step staying within the platform's reported
  resolution". `spec.md` FR-008 and User Story 2 acceptance scenario 3 no
  longer carry that clause. A shipped run falsifies it: the smallest non-zero
  step measured 70 ns against a reported resolution of 1 ns. Name the check
  `counters_clock_raw_test` prints, "no sample-to-sample step is finer than
  the resolution the platform reports for this clock", and touch no other
  entry in the list. Verify:
  `grep -c "staying within the" quickstart.md` prints `0`,
  `grep -c "finer than the resolution" quickstart.md` prints `1`,
  `cmake -P cmake/spell.cmake` exits 0, and
  `cmake -P cmake/prose-lint.cmake` exits 0
  (FR-008, US2/AC3; partial)

- [X] T034 Correct the specification-findings count in the introduction of
  `research.md:18-23`. It reads "Four specification items conflict with the
  constitution or with each other" and "resolved all four on 2026-10-03". The
  section it introduces at `research.md:419-423` opens with "Five items need
  the maintainer's attention", and it carries five findings after T020 added
  the smallest-step one. Write both counts as five, extend the introduction's
  enumeration with that finding, and keep the resolution date on all five.
  Touch no specification finding below the section heading. Verify:
  `grep -c "resolved all four" research.md` prints `0`,
  `grep -c "resolved all five" research.md` prints `1`,
  `grep -c "^### SF-" research.md` prints `5`, and
  `cmake -P cmake/prose-lint.cmake` exits 0
  (FR-008, Constitution IX; partial)

- [X] T035 Correct the research-artifact range in the project-structure
  comment at `plan.md:127`, which reads `SF-001..SF-004` while `research.md`
  carries a fifth specification finding since T020. Write the range as
  `SF-001..SF-005` and touch no other line of that tree. Verify:
  `grep -c "SF-001..SF-004" plan.md` prints `0`,
  `grep -c "SF-001..SF-005" plan.md` prints `1`, and
  `cmake -P cmake/prose-lint.cmake` exits 0
  (plan: Project Structure, FR-008; partial)

---

## Phase 14: Convergence

Remaining work found by assessing the code against `spec.md`, `plan.md`, and
this file after Phase 13 closed. Every gate passes on this tree: the developer
and sanitizer suites report 45 of 45, the release build compiles, the contract
gate reports 137 interfaces and 0 gaps with `sg::simulation_start` among them,
coverage reports 100 percent line and 100 percent branch, the format, spelling,
and prose gates exit 0, the marker gate finds the window once for each of two
compilers under each contract setting and detects its planted wrong window, the
widened vocabulary scan bites on a planted token, and the installed package
compiles the new header in a downstream consumer. A sixty-second run of the
SC-006 rate comparison on this reference machine reads +7.46 ppm, inside the
20 ppm band. Both findings below concern which quantity and which pass the
published measurement record names. A verify command naming a file under this
feature's directory runs from that directory; every other one runs from the
repository root.

- [X] T036 Name the cost quantity `test/source/counters_overhead.cpp` measures
  in the two artifacts that call it a per-read cost, per FR-010, FR-035, and
  research.md R-006 (contradicts). The ledger line at `plan.md:155` reads
  "measure the new counter's per-read cost", and the expected outcome at
  `quickstart.md:59` reads "minimum, median, and maximum per read".
  `research.md:160-163` states that the published figure is a
  per-sampling-action number that carries the library's own machinery, and that
  the per-read cost is a different quantity measured by a different loop.
  `test/source/counters_overhead.cpp:345` prints "sampling cost by regime
  (nanoseconds per sample())". The Physical view row for the same file at
  `plan.md:237` names the harness correctly. Write the per-sampling-action
  quantity in both places. Keep each line's field order. Touch no other line
  of either file. Verify: `grep -c "per-read cost" plan.md` prints `0`,
  `grep -c "maximum per read" quickstart.md` prints `0`,
  `ctest --preset=dev -R counters_overhead -V` exits 0, and
  `cmake -P cmake/prose-lint.cmake` exits 0

- [X] T037 Name the pass that measured `machine/monotonic` in the paragraph at
  `docs/pages/counters-overhead.md:184-193`, per FR-010 and T008
  (contradicts). The sentence at line 188 reads "That pass reads
  `machine/monotonic` at 30, 30, and 40 ns in the release build", and its
  nearest antecedent is "the 2026-09-28 pass" in the preceding sentence. That
  pass is recorded at 40, 40, and 80 ns in the release row at line 167, at 70,
  70, and 130 ns in the correctness row at line 178, and in both
  `clock, syscall (vDSO)` trial lines at lines 243 and 253. The figures the
  sentence gives are the `machine/monotonic_raw` rows at lines 168 and 179,
  which line 184 attributes to the 2026-10-03 pass, and no trial line records
  `machine/monotonic` from that pass. A reader therefore has no row that
  settles which pass the sentence names. Write the pass's date in place of the
  pronoun. Touch no table row, no figure inside a table, and no trial line.
  Verify: `grep -c "That pass reads" docs/pages/counters-overhead.md` prints
  `0`, `grep -c "2026-10-03 pass reads" docs/pages/counters-overhead.md`
  prints `1`, `cmake -P cmake/spell.cmake` exits 0, and
  `cmake -P cmake/prose-lint.cmake` exits 0
