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

- [ ] T001 Run the baseline gate and record the result: `cmake --preset=dev`, then `cmake --build --preset=dev`, then `ctest --preset=dev`. Record any pre-existing failure verbatim in the commit body and stop until it is classified. Verify: all three commands exit 0, or the commit body names the pre-existing failure. Touches no file.

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

- [ ] T002 [P] [US1] Create `include/speedgun-ng/simulation.hpp`: include `speedgun-ng/speedgun-ng_export.hpp`, open `namespace sg`, and declare `SPEEDGUN_NG_EXPORT void simulation_start() noexcept;` plus `inline constexpr std::uint32_t simulation_start_tag = 0xFACEU;` (FR-011, FR-013). Give the function doxygen with `\pre none` and `\post none`, the project's spelling for a contract with nothing to assert (FR-030). In prose, publish the tag's value, the tracer option that consumes it as `-start_ssc_mark FACE`, the hex form as most-significant nibble first with no `0x` prefix, and the marker's three properties: no architectural effect, safe with no tracer attached, and perturbs a measurement it sits inside (FR-014, FR-019). Name no register, no clock, and no platform function, so the header-vocabulary scan stays clean (FR-009). Verify: `cmake --build build/dev -t dbc-gate` reports zero gaps and `cmake --build build/dev -t format-check` exits 0.
- [ ] T003 [US1] Create `source/simulation/marker.cpp` and register it: guard the body with `#if defined(__x86_64__) || defined(__i386__)`, leaving an empty body with an `LCOV_EXCL` marker in the spelling `source/counters/clock_provider.cpp` uses, because no supported build can execute that arm (FR-018). Add `static_assert(simulation_start_tag <= 0xFFFFFFFFU)` so truncation fails the build in every configuration (FR-016). Emit one `__asm__ __volatile__` statement, `movl $0xFACE, %%ebx` followed by `.byte 0x64, 0x67, 0x90`, naming `ebx` in the clobber list and nothing else, so the compiler saves and restores the callee-saved register and every general-purpose register stays bit-identical (FR-012, FR-015). Leave the memory clobber off and record the reasoning in a comment: an attached tracer observes executed-instruction order directly (FR-017, FR-022). Build the project, read the analyzer output the compile launchers produce for this unit, and add the one suppression the statement needs with its check name and reason on the same line (FR-021). Add `target_sources(speedgun-ng_speedgun-ng PRIVATE source/simulation/marker.cpp)` beside the other explicit additions in the root `CMakeLists.txt`; the `source/counters/` glob does not reach this path. Verify: `cmake --build --preset=dev` exits 0, the object's text holds the eight bytes `bb ce fa 00 00 64 67 90` once, and the analyzer names exactly one suppressed check whose reason sits on the same line.

### Tests for User Story 1

- [ ] T004 [P] [US1] Create `test/source/simulation_test.cpp` following the hand-rolled `check()`/`fail()` convention with its own `main` as `test/source/counters_tsc_test.cpp` does, add no test framework, then register it in `test/CMakeLists.txt` with the `add_executable`, `target_link_libraries(speedgun-ng::speedgun-ng)`, `target_compile_features(cxx_std_23)`, and `add_test` block every other test uses. Assert `simulation_start_tag == 0xFACEU` (FR-013); load a known 64-bit value whose upper 32 bits are set into RBX, call `simulation_start()`, and assert RBX is bit-identical afterwards (FR-026, the case an implementation preserving only the low half fails); call it in a loop and assert normal termination (FR-027). Include `speedgun-ng/simulation.hpp`. Verify: `ctest --preset=dev -R simulation` exits 0.

### Integration for User Story 1

- [ ] T005 [P] [US1] Add one `simulation_start()` call in `example/counters_standalone_example.cpp` at the region boundary, outside the timed window, with a one-line comment saying why it sits outside (FR-019's perturbation note). Include `speedgun-ng/simulation.hpp`. Verify: `ctest --preset=dev -R counters_standalone_example` exits 0.

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

- [ ] T006 [US2] Create `test/source/counters_clock_raw_test.cpp` in the same hand-rolled convention, then register it in `test/CMakeLists.txt` with the four-line block every test uses. Declare the local alias `using time_dim = dim<1, 0>;` as the examples and tests do. Resolve the leaf through `system::local().object("machine")` then `counter<time_dim>("monotonic_raw")` and assert the resolution succeeds with no architecture guard (FR-001, FR-005); assert `unit_token()` reads `nanoseconds` and `object::counters()` reports read mode `syscall` (FR-002); sample ten million consecutive reads on one thread and assert no decrease (FR-006); sample on every logical processor, join, and assert no decrease against the joined samples (FR-007); call `clock_getres` and assert the smallest non-zero step stays within the reported resolution (FR-008); print the per-read cost distribution over ten million reads (FR-008, and the convention in research.md R-006). Verify: `ctest --preset=dev -R counters_clock_raw` FAILS, naming the unresolved address. The failure is the expected first result.
- [ ] T007 [US2] Implement the leaf in `source/counters/clock_provider.cpp`: add `machine/monotonic_raw` to the canonical address table beside the existing four, add its case to the address parser and to the read switch, add a `monotonic_raw_ns()` reader in the anonymous namespace that calls `clock_gettime(CLOCK_MONOTONIC_RAW, &stamp)` through `<ctime>` and returns `static_cast<std::uint64_t>(stamp.tv_sec) * 1000000000ULL + static_cast<std::uint64_t>(stamp.tv_nsec)` with integer arithmetic only, keeping the existing failure arm's exclusion marker, and add the seed entry to the provider's entry list with unit `nanoseconds` and mode `read_mode::syscall` (FR-001, FR-003, FR-004, FR-034). Add no enumerator, no provider, no macro, and no dispatch entry; the `source/counters/*.cpp` glob already compiles this file. Verify: `ctest --preset=dev -R counters_clock_raw` exits 0 and `ctest --preset=dev` stays green.

### Implementation for User Story 2

- [ ] T008 [P] [US2] Add the new counter to the measurement harness in `test/source/counters_overhead.cpp` so it prints a raw-trial line for `machine/monotonic_raw` in the same field order as the existing clock line, and publish the row in `docs/pages/counters-overhead.md` in the existing five-column format `| plan | min ns | median ns | max ns | read mode |`, once per build-configuration table, with the read-path cell naming the platform fast path and stating that no system call is involved (FR-008, FR-010). Keep the raw trial block beneath the tables in step with the new row. Verify: `ctest --preset=dev -R counters_overhead -V --repeat until-pass:3` exits 0 and its printed minimum, median, and maximum match the published row.

**Checkpoint**: User Story 2 is complete and independently testable. The
counter resolves, samples monotonically, and carries a published cost row.

---

## Phase 5: User Story 3 - Keep the emitted instruction faithful on every build (Priority: P2)

**Goal**: An automated gate that fails the moment the emitted instruction
sequence changes, on both supported compilers, with no tracer installed.

**Independent Test**: Deliberately alter the emitted sequence in a scratch
copy, run the gate, and confirm it fails; restore the sequence and confirm it
passes.

- [ ] T009 [P] [US3] Create `test/simulation_mark_shape.sh` modelled on `test/counters_tsc_read_shape.sh`: iterate over each of `g++` and `clang++` found on `PATH`, and for each iterate `SG_CONTRACTS_SEMANTIC` 0 and 2; compile `source/simulation/marker.cpp` at `-O2 -std=c++23` against `include/` and the generated export directory; disassemble the object with `objdump -d`; assert the eight bytes `bb ce fa 00 00 64 67 90` appear exactly once, which is the in-function count because the unit holds exactly one function (FR-012, FR-023). Plant a deliberately wrong window in a scratch copy and require the assertion to fail, so a passing run cannot come from a detector that inspects nothing (FR-024). Read each compiler's target triple and report a skip with exit 0 when none targets x86, because FR-025 speaks of the compiler's target, and the host's architecture is a different question (FR-025). Register it in `test/CMakeLists.txt` with `add_test(NAME simulation_mark_shape COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/simulation_mark_shape.sh ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})` inside the existing Linux-only region, after T004's edit to that file. Verify: `ctest --preset=dev -R simulation_mark_shape` exits 0, and the output names each compiler and each contract setting it exercised.

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

- [ ] T010 [P] [US4] Widen the scanned set in `test/counters_header_purity.sh` so it covers `include/speedgun-ng/simulation.hpp` alongside the existing `counters*.hpp` glob, keeping the banned-term list at its present spelling: `perf_event`, `clock_gettime`, whole-word `rdpmc`, `rdtsc`, and the acronym `PMU` across the core counter headers (FR-009, FR-029). Add the widened-set probe: plant one banned token in the new header, confirm the scan fails naming the file and line, then restore the header and confirm it passes. Verify: `ctest --preset=dev -R counters_header_purity` exits 0.
- [ ] T011 [US4] Prove the installed package exposes the new header: `cmake --install build --prefix "$PWD/prefix"`, then configure, build, and run `test/consumer` against that prefix exactly as `test/consumer/CMakeLists.txt` directs, and confirm the new header resolves through the package's documented include path while the consumer's link manifest names no tracer library (FR-028, User Story 4 scenario 3). Verify: all three commands exit 0 and the manifest names no tracer library.

**Checkpoint**: User Story 4 is complete and independently testable. The
vocabulary scan records a verdict on the new header on every run, and the
downstream consumer links the new surface.

---

## Phase 7: Polish & Cross-Cutting Concerns

- [ ] T012 Run the release-configuration build and the full gate sweep, in this order: `cmake --preset=ci-ubuntu`, `cmake --build build`, `ctest --preset=dev`, `cmake --build build/dev -t dbc-gate`, `cmake --build build/coverage -t coverage`, `cmake --build build/dev -t format-check`, `cmake -P cmake/spell.cmake`, `cmake -P cmake/prose-lint.cmake`. Record every exit code in the commit body. Verify: every command exits 0, the coverage summary reports 100 percent line and 100 percent branch over the added code (FR-036), and the prose gate reports zero findings over this feature's range.
- [ ] T013 [P] Run the local rate-agreement confirmation from [quickstart.md](quickstart.md): sample `machine/monotonic_raw` and `machine/monotonic` once per second for at least sixty seconds and divide the difference of the two elapsed intervals by the interval measured with `machine/monotonic_raw` (SC-006). Record the command and the parts-per-million result in the commit body. Verify: the ratio is within 20 parts per million, or the commit body records a clocksource switch or a suspend and the run is repeated.
- [ ] T014 [P] Run the local tracer confirmation from [quickstart.md](quickstart.md): `sde64 -start_ssc_mark FACE:repeat -- <example binary>` over the marker placed by T005 (SC-001). Record the command and where the trace's first collected instruction falls in the commit body. Verify: the first collected instruction falls inside the marked region, or the commit body records that Intel SDE is absent from this machine and the confirmation is deferred.

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