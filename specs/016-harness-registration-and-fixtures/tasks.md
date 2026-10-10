# Tasks: Harness Registration and Fixtures

**Input**: Design documents from `specs/016-harness-registration-and-fixtures/`

**Prerequisites**: plan.md, spec.md (US1..US6, FR-001..FR-030), research.md (R-01..R-17), data-model.md (E-01..E-09), contracts/registration-api.md, contracts/fixtures.md, contracts/callbacks-and-state.md, contracts/result-fields.md, contracts/selection-and-cli.md, quickstart.md

**Tests**: Required. The plan records TDD mode under Principle III and FR-029: every test task is written and observed failing before the code task that turns it green. For an interface that does not exist yet, the compile failure of the new translation unit against the absent API is the red state; record it in the Execution Log.

**Organization**: Tasks are grouped by user story so each story can be implemented and tested independently. US1 (P1) is the vertical slice and the MVP. US2-US5 are P2. US6 is P3.

**Build facts carried from the plan**: the project version moves `0.6.0` to `0.7.0` and the `speedgun-ng_harness` archive `SOVERSION` moves `2` to `3`; the counters archive stays at `SOVERSION` 2 and the constitution stays 2.18.0 (FR-026, R-16). The harness gains one private source file, `source/harness/family.cpp` (D-1). No new third-party dependency, no new preset, no new CI job (FR-025). The cited upstream revision is `google/benchmark` `main` at `e662de9a`, and every family rule carries the semantics of that revision (FR-030).

## Format: `[ID] [P?] [Story] Description`

- **[P]**: different files, no dependency on an incomplete task
- **[Story]**: US1..US6, user-story phases only
- Paths are repository-root relative

**Write-scope law for this feature** (it decides parallelism more than the
dependency graph does): `include/speedgun-ng/benchmark.hpp`,
`source/harness/detail/internal.hpp`, `source/harness/registry.cpp`,
`source/harness/family.cpp`, `source/harness/runner.cpp`,
`source/harness/cli.cpp`, `source/harness/report.cpp`,
`test/CMakeLists.txt` and `CMakeLists.txt` are each owned by exactly one
task at a time. Two tasks that name the same path never run together.

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: the version effect and the new translation unit the plan names

- [x] T001 Set `VERSION 0.7.0` in the `project()` command of CMakeLists.txt:7 and raise the `speedgun-ng_harness` `SOVERSION` from 2 to 3 in the `set_target_properties(speedgun-ng_harness ...)` block of CMakeLists.txt:211-218, rewriting that block's comment so it states the reason the ABI number moves: `State` gains argument storage and `BenchmarkResult` gains two fields (FR-026, R-16). Leave the counters archive at `SOVERSION` 2 and leave its 0.x-rule comment at CMakeLists.txt:41-47 intact
- [x] T002 Create `source/harness/family.cpp` with its doxygen file block, including `source/harness/detail/internal.hpp` so the translation unit is non-empty, and the contract blocks (`\pre`, `\post`, `\invariant`) for every interface it will own — `expandRegistry()`, the instance-name builder, the suite and case derivation, the duplicate-instance check, the family-size check, `createRange`, `createDenseRange`. No CMakeLists.txt edit is needed: the harness target collects `source/harness/*.cpp` through `file(GLOB_RECURSE speedgun-ng_harness_sources CONFIGURE_DEPENDS ...)` at CMakeLists.txt:191-193, so a reconfigure picks the new file up. This keeps the DBC completeness gate of `cmake/dbc-gate.cmake` seeing every new interface from the first build (D-1, Principle II)

**Checkpoint**: the tree still configures and builds; the version and ABI numbers are the ones the release will ship.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: the family record, the instance record, and the seam that keeps every later increment bisectable

**CRITICAL**: no user story work begins before this phase completes.

- [x] T003 Extend `RegistryEntry` in `source/harness/detail/internal.hpp` with the E-01 family fields: the argument lists, the `argName` labels, the range multiplier, the setup callback, the teardown callback, and the fixture factory `std::function<std::unique_ptr<FixtureBase>()>` whose definition type is the internal base of `sg::Fixture` (R-01, R-06, E-01); add the two named constants `kDefaultRangeMultiplier` = 8 and `kMaxFamilySize` = 100 with the `k` prefix on the shape of `kDefaultMinTimeNs` in `source/harness/runner.cpp:22-24` (FR-002, FR-007, R-17)
- [x] T004 Add the `Instance` record and the `expandRegistry()` declaration to `source/harness/detail/internal.hpp`, and change `detail::Runner::run` from `run(RegistryEntry&)` to `run(Instance&)` (R-02, E-01). `Instance` carries the callable, the family name, the instance name, the suite, the case, the argument vector the instance owns, the fixture factory, and the two callbacks
- [x] T005 Give `expandRegistry()` in `source/harness/family.cpp` its minimal body — one `Instance` per `RegistryEntry`, the instance name equal to the family name, zero arguments — and wire the call sites in `source/harness/cli.cpp` and `source/harness/runner.cpp` so `speedgunMain` expands the registry and the runner walks the resulting one-instance-per-family list. This is the seam every later increment bisects against, so the tree must build and the eight feature-015 suites must stay green when it closes (R-02, R-03)
- [x] T006 Record the green baseline the refactor starts from: `cmake --build --preset=dev` then `ctest --preset=dev` over every target registered in `test/CMakeLists.txt`, and paste the pass count into this file's Execution Log. Every feature-015 `harness_*` suite, `loop_shape`, `cli_shape`, `barrier_shape` and `time_source_gate` must pass at this point

**Checkpoint**: the family record and the instance record exist, the run path walks instances, and the H1 suite is green on unchanged behavior. User stories can now proceed.

---

## Phase 3: User Story 1 - Sweep one function over a family of arguments (Priority: P1) 🎯 MVP

**Goal** (capabilities C-1 argument families and C-3 argument access): a registration is followed by family calls; the family expands into instances before the run starts; each instance reads its own arguments.

**Independent Test**: `ctest --test-dir build -R 'harness_family_test|harness_argument_test' --output-on-failure` passes, and the instance set of every family call matches the cited revision (quickstart §2, §4, SC-001, SC-002).

### Tests for User Story 1 (write first, observe failing)

- [x] T007 [P] [US1] Write `test/source/harness_family_test.cpp`: for each call of FR-001 and FR-002, assert the instance set matches the cited revision — `arg`, `args` as an initializer list and as a vector, `range`, `rangeMultiplier`, `ranges`, `denseRange`, `argsProduct`, `apply`, `argName`, `argNames`; quote and enforce the FR-006 preconditions "a multiplier below 2, a low bound above a high bound, and a `denseRange` step below 1" and "an arity or label count unequal to the family arity", each observed through the self re-exec plus nonzero-status pattern of `test/source/harness_registry_test.cpp:209-212`; assert `range` and `ranges` "shall grow by powers of the range multiplier, default 8, with both bounds included" and that negatives follow `AddRange` at `benchmark_register.h:61`; assert `denseRange` "shall step from the low bound to the high bound inclusive, default step 1"; assert `createRange(low, high)` and `createDenseRange(low, high, step)` return a `std::vector<std::int64_t>` that feeds a family call; assert a family above `kMaxFamilySize` "shall draw one warning naming `kMaxFamilySize`, keep its instances, and continue the run" (FR-001, FR-002, FR-004, FR-005, FR-006, FR-007, R-14, R-15, R-17); register the target in `test/CMakeLists.txt` on the `add_executable` plus `speedgun-ng::harness` plus `add_test` pattern of lines 476-570, and observe it failing (FR-029)
- [x] T008 [P] [US1] Write `test/source/harness_argument_test.cpp`: `range(index)` returns the argument of the running instance and `rangeCount()` returns that count for every instance of a two-argument family; "a read shall allocate nothing", asserted with the allocation-counting `operator new` pattern already in `test/source/harness_capture_test.cpp`; "a family with no family call has one instance with zero arguments; `rangeCount()` reports 0 and `range(0)` is a precondition violation"; an index at or above the argument count is a precondition violation, observed through the re-exec pattern; the arguments reach the callback state too, so the read is legal there (FR-008, FR-006, R-04); register the target in `test/CMakeLists.txt` and observe it failing (C-3)

### Implementation for User Story 1

- [x] T009 [US1] Declare the family surface in `include/speedgun-ng/benchmark.hpp`: the ten `BenchmarkHandle` members of contracts/registration-api.md — `arg`, the two `args` overloads, `range`, `rangeMultiplier`, `ranges`, `denseRange`, `argsProduct`, the `apply` template, `argName`, `argNames` — each returning `BenchmarkHandle&` for chaining, plus the free builders `createRange` and `createDenseRange` and the `kDefaultRangeMultiplier` constant, every declaration carrying doxygen `\pre`, `\post`, `\invariant` and the `SPEEDGUN_NG_EXPORT` annotation the neighboring declarations use (FR-001, FR-002, FR-004, FR-005, FR-024). Touch no H1 signature: `registerBenchmark`, `SG_BENCHMARK`, `speedgunMain`, the five H1 setters and the H1 `State` methods stay byte-identical (FR-022)
- [x] T010 [US1] Implement `createRange` and `createDenseRange` in `source/harness/family.cpp` with the semantics of `CreateRange` and `CreateDenseRange` at `benchmark_register.cc:544,550`, each guarded by the FR-006 `SG_REQUIRE` preconditions (FR-002, FR-006, R-15)
- [x] T011 [US1] Implement the ten family calls in `source/harness/registry.cpp`: each writes the E-01 fields of the one family record the handle points at, each carries the `SG_REQUIRE` that the run has not started on the shape of `source/harness/registry.cpp:55`, and each carries the FR-006 `SG_REQUIRE` preconditions with the message naming the violated bound (FR-001, FR-006, R-01, R-15)
- [x] T012 [US1] Implement the expansion rules in `source/harness/family.cpp`: the `AddRange` growth at `benchmark_register.cc:250-372` — bounds included, powers of the multiplier strictly between, negatives mirrored — the `denseRange` step, the `argsProduct` order, and `apply`'s arity check; then the family-size check that "shall draw one warning naming `kMaxFamilySize`, keep its instances, and continue the run" (FR-003, FR-004, FR-005, FR-007, R-03, R-14)
- [x] T013 [US1] Add the argument view to `State` in `include/speedgun-ng/benchmark.hpp`: a `std::span<const std::int64_t> m_arguments` member placed with the other data members at the tail of the class, `range(std::size_t index) const -> std::int64_t` carrying `SG_REQUIRE(index < m_arguments.size())`, and `rangeCount() const noexcept -> std::size_t`; the read holds no allocation, no lock and no recorder work, and the constructor takes the storage the instance owns (FR-008, R-04, FR-026). This is the layout change that makes the `SOVERSION` rise of T001 true
- [x] T014 [US1] Lock the expansion ordering guarantee in `source/harness/cli.cpp`: `expandRegistry()` runs before the filter and before the first run, no expansion work happens during a run, and the walk covers every instance of every family (R-03, FR-003). T005 already put the call in place; this task is the guarantee, and `test/source/harness_family_test.cpp` from T007 carries the assertion that expansion precedes selection
- [x] T015 [US1] Run one instance in `source/harness/runner.cpp`: construct `State` with the instance's argument span, keep the timed loop byte-for-byte in the H1 shape — no added work, no allocation, no lock, no contract check — and confirm `ctest --test-dir build -R loop_shape --output-on-failure` stays green (FR-009, R-04, R-06)
- [x] T016 [US1] Turn the suites of T007 and T008 green in `test/source/harness_family_test.cpp` and `test/source/harness_argument_test.cpp`: `ctest --test-dir build -R 'harness_family_test|harness_argument_test' --output-on-failure`, and record both runs in this file's Execution Log

**Checkpoint**: US1 works end to end: one family call, many instances, each reading its own arguments. This is the MVP.

---

## Phase 4: User Story 2 - Name, filter, and list the instances (Priority: P2)

**Goal** (capability C-2 instance names): every instance carries a predictable name; the filter and list mode select instance names; a duplicate name is reported and the run continues.

**Independent Test**: `ctest --test-dir build -R harness_instance_name_test --output-on-failure` passes, and `--filter` on one full instance name runs exactly that instance (quickstart §3, SC-003, SC-004).

### Tests for User Story 2 (write first, observe failing)

- [x] T017 [P] [US2] Write `test/source/harness_instance_name_test.cpp`: each instance name "shall start with the family name and add one `/`-joined segment per argument" for the single-argument, multi-argument, `argName`-labelled and unlabelled shapes, with the label printed as `label:value`; `--filter` with one full instance name runs exactly that instance and no other; list mode prints instance names, one per line, and the `--catalog` path prints none of them; a duplicate instance name is "a recoverable error found at expansion: one line on the standard error stream naming both names, the earlier instance stays, the later instance does not run, every other instance runs, and the exit status stays as H1 fixes it" (FR-010, FR-011, FR-012, R-02, R-03); reuse the `captureRun`, `lineOf` and `countLines` helper shape already in `test/source/harness_registry_test.cpp`; register the target in `test/CMakeLists.txt` and observe it failing (C-2)

### Implementation for User Story 2

- [x] T018 [US2] Implement the instance-name construction in `source/harness/family.cpp`: the family name plus one `/`-joined segment per argument, each segment carrying its `argName` label as `label:value` where one is set, with the semantics of `benchmark_api_internal.cc:34-51` (FR-010, R-02)
- [x] T019 [US2] Implement the duplicate-instance check in `source/harness/family.cpp`, inside `expandRegistry()`: one line on the standard error stream naming both names, the earlier instance stays, the later instance is dropped from the instance list, and the exit status is untouched (FR-012, R-03)
- [x] T020 [US2] Point selection at instance names in `source/harness/cli.cpp`: the `--filter` regular expression and list mode match the instance name, the option spellings, the parse and the exit statuses of H1 stay unchanged, and the `--catalog` path still returns before selection (FR-011, FR-022)
- [x] T021 [US2] Turn the suite of T017 green in `test/source/harness_instance_name_test.cpp`: `ctest --test-dir build -R harness_instance_name_test --output-on-failure`, and record the run in this file's Execution Log

**Checkpoint**: US1 and US2 work independently: the family is addressable one instance at a time.

---

## Phase 5: User Story 3 - Capture arguments and instantiate a template (Priority: P2)

**Goal** (capability C-4 capture and capability C-5 templates): `SG_BENCHMARK_CAPTURE` and `SG_BENCHMARK_TEMPLATE` register one instance each, named as the cited revision names them.

**Independent Test**: `ctest --test-dir build -R 'harness_capture_macro_test|harness_template_test' --output-on-failure` passes (quickstart §5, §6, SC-001).

### Tests for User Story 3 (write first, observe failing)

- [x] T022 [P] [US3] Write `test/source/harness_capture_macro_test.cpp`: `SG_BENCHMARK_CAPTURE(addTwo, pair, 2, 2)` registers and runs one instance named `addTwo/pair`, and the captured values reach the function, so the reported result follows their sum (FR-013, R-13); register the target in `test/CMakeLists.txt` and observe it failing (C-4)
- [x] T023 [P] [US3] Write `test/source/harness_template_test.cpp`: `SG_BENCHMARK_TEMPLATE(sortOf, int, double)` registers and runs one instance named `sortOf<int, double>`, "the type list stringified as written at the macro site", including a qualified or namespaced type argument written with a space (FR-014, R-13); register the target in `test/CMakeLists.txt` and observe it failing (C-5)

### Implementation for User Story 3

- [x] T024 [US3] Implement `SG_BENCHMARK_CAPTURE(fn, captureName, ...)` in `include/speedgun-ng/benchmark.hpp`: one instance under the family name `fn/captureName` with the behavior of `BENCHMARK_CAPTURE` at `registration.h:69-75`, the captured values bound into the registered callable, and the generated identifier following the `SgBenchmarkRegistrar_##fn` shape of `SG_BENCHMARK` at `benchmark.hpp:582-593` — no leading underscore, no `__` (FR-013, FR-023, R-13)
- [x] T025 [US3] Implement `SG_BENCHMARK_TEMPLATE(fn, ...)` in `include/speedgun-ng/benchmark.hpp`: a function template instantiated over one or more type arguments, named `fn<` plus the stringified type list plus `>`, with the behavior of `BENCHMARK_TEMPLATE` at `registration.h:101-107` and the same generated-identifier shape (FR-014, FR-023, R-13) (FR-028)
- [x] T026 [US3] Turn the suites of T022 and T023 green in `test/source/harness_capture_macro_test.cpp` and `test/source/harness_template_test.cpp`: `ctest --test-dir build -R 'harness_capture_macro_test|harness_template_test' --output-on-failure`, and record the run in this file's Execution Log

**Checkpoint**: the two one-off registration forms work; no family call is needed for a fixed argument set or a type set.

---

## Phase 6: User Story 4 - Fixtures and suite grouping (Priority: P2)

**Goal** (capability C-6 fixtures and capability C-9 suite and case): a class holds the state of the work under measurement, `setUp` and `tearDown` wrap each run outside the timed region, and every instance carries a suite name and a case name.

**Independent Test**: `ctest --test-dir build -R 'harness_fixture_test|harness_instance_name_test' --output-on-failure` passes, and the scripted clock deltas show the pair adds nothing to the reported time (quickstart §7, §10, SC-005, SC-007).

### Tests for User Story 4 (write first, observe failing)

- [x] T027 [P] [US4] Write `test/source/harness_fixture_test.cpp`: each of the seven macros of FR-016 registers and runs, with a fixture instance named `FixtureClass/Method` and a template fixture instance named `BaseClass<types>/Method`; `SG_BENCHMARK_DEFINE_F` followed by `SG_BENCHMARK_REGISTER_F` registers the method later under the same name; the scripted `machine/monotonic` deltas show "the pair runs in the untimed region and its work adds nothing to the reported time"; the pair runs "once per run, the warm-up and calibration runs included", counted through the scripted provider's sampling actions; the factory builds one fixture object per run and destroys it after `tearDown`, asserted with a construction and destruction counter (FR-015, FR-016, FR-017, R-09, SC-005); register the target in `test/CMakeLists.txt` and observe it failing (C-6)
- [x] T028 [US4] Extend `test/source/harness_instance_name_test.cpp` with the C-9 cases: `BenchmarkResult` carries `suite` and `caseName` for every instance; the suite is "the family name up to its first `/`", the fixture class name for a fixture instance, and the case is "the instance name with the leading suite and its `/` removed", with the three derivation rows of contracts/result-fields.md (`fib/bits/1/8/64` to `fib` plus `bits/1/8/64`, `QueueFixture/push/8` to `QueueFixture` plus `push/8`, `plain` to `plain` plus `plain`) asserted literally; the pair is unique per instance; the suite order follows first registration and rows inside one suite keep registration order; the console row keeps the single H1 name column (FR-021, R-07, R-08, R-12, SC-007); observe the new cases failing

### Implementation for User Story 4

- [x] T029 [US4] Add `sg::Fixture` to `include/speedgun-ng/benchmark.hpp`: `SPEEDGUN_NG_EXPORT`, a virtual destructor, and `virtual auto setUp(State&) -> void` and `virtual auto tearDown(State&) -> void` with empty defaults, each documented with `\pre`, `\post`, `\invariant` (FR-015, FR-024)
- [x] T030 [US4] Implement the seven fixture macros in `include/speedgun-ng/benchmark.hpp` — `SG_BENCHMARK_F`, `SG_BENCHMARK_DEFINE_F`, `SG_BENCHMARK_REGISTER_F`, `SG_BENCHMARK_TEMPLATE_F`, `SG_BENCHMARK_TEMPLATE_DEFINE_F`, `SG_BENCHMARK_TEMPLATE_METHOD_F`, `SG_BENCHMARK_TEMPLATE_INSTANTIATE_F` — with the definition, registration and naming behavior of `registration.h:121-163` at the cited revision, and generated identifiers following the `SgBenchmarkRegistrar_##fn` shape (FR-016, FR-023, R-13)
- [x] T031 [US4] Add `std::string suite` and `std::string caseName` to `BenchmarkResult` in `include/speedgun-ng/benchmark.hpp`, leaving `ResultRow` without either field, and state in the doxygen block that a later JSON report reuses the pair (FR-021, R-07). This is the second layout change behind the `SOVERSION` rise of T001
- [x] T032 [US4] Implement the suite and case derivation and the stable suite grouping in `source/harness/family.cpp`: suite is the family name up to its first `/` and the fixture class name for a fixture instance, the case is the instance name with the leading suite and its `/` removed, an instance name equal to its suite keeps that name as its case, and after expansion the instance list groups stably by suite in first-appearance order (FR-021, R-08, R-12)
- [x] T033 [US4] Fill the two fields in `source/harness/report.cpp` and keep the console row in the H1 shape: one name column carrying the full instance name, the fixed H1 columns unchanged, and the suite split reaching the caller through row order and the fields of `BenchmarkResult` (FR-021, R-07, R-12)
- [x] T034 [US4] Implement the fixture pair around each run in `source/harness/runner.cpp`: the factory builds one fixture object inside `sampleRun` before `setUp` and destroys it after `tearDown`, the pair runs in the untimed region, the window pair stays exactly two samples per run, and the timed loop keeps the H1 shape (FR-017, R-09); confirm `ctest --test-dir build -R loop_shape --output-on-failure` stays green
- [x] T035 [US4] Turn the suites of T027 and T028 green in `test/source/harness_fixture_test.cpp` and `test/source/harness_instance_name_test.cpp`: `ctest --test-dir build -R 'harness_fixture_test|harness_instance_name_test' --output-on-failure`, and record the run in this file's Execution Log

**Checkpoint**: US4 works on its own: a fixture runs with its pair outside the measured window, and every row is addressable as a suite and a case.

---

## Phase 7: User Story 5 - Setup and teardown callbacks (Priority: P2)

**Goal** (capability C-7 callbacks): `setup` and `teardown` wrap each run outside the timed region, and the state they receive carries a restricted operation set.

**Independent Test**: `ctest --test-dir build -R harness_callback_test --output-on-failure` passes, with the order asserted as setup, `setUp`, callable, `tearDown`, teardown (quickstart §8, SC-005).

### Tests for User Story 5 (write first, observe failing)

- [x] T036 [P] [US5] Write `test/source/harness_callback_test.cpp`: the pair runs once around each run, the warm-up, calibration and measured runs included, counted through the scripted provider's sampling actions; the order in one run is setup callback, fixture `setUp`, callable, fixture `tearDown`, teardown callback; one callback per slot with the last attachment winning; a setup callback reads `range(0)` of the running instance; the scripted clock deltas show the callback work adds nothing to the reported time; the callback-state rule of R-05 holds — `range(index)`, `rangeCount()` and `iterations()` are legal in a callback state, while `begin()`, `skipWithError` and `skipWithMessage` are `SG_REQUIRE` precondition violations observed through the self re-exec plus nonzero-status pattern, and `State::end()` stays legal because it is static and touches no state (FR-018, FR-019, R-05, R-10); register the target in `test/CMakeLists.txt` and observe it failing (FR-025) (C-7)

### Implementation for User Story 5

- [x] T037 [US5] Declare `setup(std::function<void(State&)>)` and `teardown(std::function<void(State&)>)` on `BenchmarkHandle` in `include/speedgun-ng/benchmark.hpp`, each returning `BenchmarkHandle&`, each documented with `\pre`, `\post`, `\invariant` and the note that the state it hands the callback is restricted by the R-05 rule (FR-018, FR-024)
- [x] T038 [US5] Implement the two calls in `source/harness/registry.cpp`: each writes its slot of the E-01 family record, one callback per slot with the last attachment winning, guarded by the same run-not-started `SG_REQUIRE` shape as the other setters (FR-018, R-10)
- [x] T039 [US5] Run the pair in `source/harness/runner.cpp` in the order setup callback, fixture `setUp`, callable, fixture `tearDown`, teardown callback, with the callback state carrying the instance arguments and no recorder window, and the `SG_REQUIRE` guards that make `begin()`, `skipWithError` and `skipWithMessage` violations in that state; `State::end()` stays static and unguarded (FR-018, R-05, R-10)
- [x] T040 [US5] Turn the suite of T036 green in `test/source/harness_callback_test.cpp`: `ctest --test-dir build -R harness_callback_test --output-on-failure`, and record the run in this file's Execution Log

**Checkpoint**: the untimed region is user-extensible without touching the timed loop.

---

## Phase 8: User Story 6 - Disabled benchmarks, the example, and the documentation (Priority: P3)

**Goal** (capability C-8 the `DISABLED_` prefix and capability C-10 examples and documentation): an instance whose name starts with `DISABLED_` registers and never runs; the example exercises the new surface; one documentation page covers every capability.

**Independent Test**: `ctest --test-dir build -R harness_disabled_test --output-on-failure` passes, and `./build/example/benchmark_example --dry-run` prints one row per family instance plus the fixture row plus the template row with exit status 0 (quickstart §9, §11, SC-006, SC-008).

### Tests for User Story 6 (write first, observe failing)

- [x] T041 [P] [US6] Write `test/source/harness_disabled_test.cpp`: a family named `DISABLED_slow` registers, expands, runs no function, stays out of the filter match and stays out of list mode; a `DISABLED_` family as the only match of a filter leaves the executable reporting no match, running nothing and exiting zero; a fixture method named `DISABLED_x` runs, because that instance name starts with the fixture class (FR-020, R-11); register the target in `test/CMakeLists.txt` and observe it failing (C-8)

### Implementation for User Story 6

- [x] T042 [US6] Implement the `DISABLED_` gate at selection in `source/harness/cli.cpp`: test the prefix on the expanded instance name, exclude a disabled instance from the filter match and from list mode, keep it in the registry and in the expansion, and leave the exit-status table of `specs/015-benchmark-harness-core/contracts/cli.md` unchanged (FR-020, R-11)
- [x] T043 [P] [US6] Extend `example/benchmark_example.cpp` so it exercises the new surface: one argument family with at least two arguments and an `argName` label, one fixture method, and one templated benchmark, each registered through the new macros and calls (FR-027). `example/CMakeLists.txt` already links `benchmark_example` to `speedgun-ng::harness`, so no build edit is needed
- [x] T044 [P] [US6] Write `docs/pages/harness.md` with one section per capability C-1 to C-10, each stating what the surface is and which test covers it, in the prose register Principle XI binds (FR-027)
- [x] T045 [US6] Turn the suite of T041 green in `test/source/harness_disabled_test.cpp` and run the example in `example/benchmark_example.cpp`: `ctest --test-dir build -R harness_disabled_test --output-on-failure`, then `./build/example/benchmark_example --dry-run; echo $?`, and record both in this file's Execution Log

**Checkpoint**: the whole capability set is registered, gated, demonstrated in the example, and documented.

---

## Phase 9: Polish & Cross-Cutting Concerns

- [x] T046 [P] Run the sanitizer preset end to end and record the verdict: `cmake --preset=ci-sanitize`, `cmake --build build/sanitize`, `ctest --test-dir build/sanitize --output-on-failure` — ASan and UBSan report no error on every new suite (Principle VIII). `harness_capture_test` and any new suite that replaces the global `operator new` stay out of the thread-sanitizer preset on the `CMAKE_CXX_FLAGS_RELEASE MATCHES "fsanitize=thread"` guard already used at `test/CMakeLists.txt:497`
- [x] T047 [P] Run the thread-sanitizer preset end to end in `build/tsan` and record the verdict: `cmake --preset=ci-tsan`, `cmake --build build/tsan`, `ctest --test-dir build/tsan --output-on-failure` — no race reported (Principle VIII)
- [x] T048 [P] Run the coverage preset and record the verdict: `cmake --preset=ci-coverage`, `cmake --build build/coverage`, `ctest --test-dir build/coverage`, then the `coverage` target — 100% line, 100% branch and 100% DBC over the changed sources, with every new branch reached by a test and the `SG_*` contract lines excluded by `cmake/coverage.cmake:67-70` (Principle VI, Principle VIII)
- [x] T049 [P] Run the prose, spelling, format and naming gates and record each verdict: `cmake -P cmake/prose-lint.cmake`, the `format-check` and `spell-check` targets, and a clean `ci-ubuntu` compile of the new translation units so `readability-identifier-naming` reports zero findings on every new identifier, including the generated identifiers of the new macros (FR-023, Principle V, Principle XI) (FR-030)
- [x] T050 Audit FR-022 against the feature baseline: diff the declarations of `registerBenchmark`, `SG_BENCHMARK`, `speedgunMain`, the five H1 `BenchmarkHandle` setters and the H1 `State` methods in `include/speedgun-ng/benchmark.hpp` against the merge base with `origin/master`, and record that no existing signature was replaced; record the two layout changes that do justify the `SOVERSION` rise (FR-022, FR-026)
- [x] T051 Run every quickstart.md section §1 to §13 against the built tree and record each outcome in this file's Execution Log, including the two version greps, the eight capability runs, the loop-shape gate and the prose gate (SC-001 to SC-009)
- [x] T052 Build the release configuration once per Principle IX and record it: `cmake --preset=ci-ubuntu`, `cmake --build build`, `ctest --test-dir build --output-on-failure` over the targets of `CMakeLists.txt` and `test/CMakeLists.txt` — the dev preset is unoptimized and closes no task on its own; record the installed `PACKAGE_VERSION` 0.7.0 and the shared-library names `libspeedgun-ng.so.2` beside `libspeedgun-ng-harness.so.3` (FR-026, SC-009)
- [x] T053 Record in the Deviations section of `specs/016-harness-registration-and-fixtures/tasks.md` every place the implementation settled a question the artifacts left open, and every artifact claim that did not survive contact with the code

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: no dependencies. T001 edits `CMakeLists.txt`; T002 creates `source/harness/family.cpp`, which the `CONFIGURE_DEPENDS` glob collects, so the two are independent.
- **Foundational (Phase 2)**: depends on Setup; BLOCKS every user story. T003, T004 and T005 land as one atomic increment: the `Runner::run` signature change of T004 cannot build until the `expandRegistry()` body and the call-site wiring of T005 exist, and the phase checkpoint T006 must be green.
- **User Stories (Phases 3-8)**: depend on Foundational, plus the instance-walk plumbing US1 lands in T014 and T015. US3 to US6 need no US1 family *behavior*, but they cannot run before that plumbing exists.
- **Polish (Phase 9)**: depends on every story it exercises.

### User Story Dependencies

- **US1 (P1)**: Foundational only. The MVP.
- **US2 (P2)**: spec-declared dependency on US1: the names it selects are the names US1's expansion produces.
- **US3 (P2)**: Foundational plus the US1 plumbing; touches `benchmark.hpp` only, so it is the cleanest parallel track.
- **US4 (P2)**: Foundational plus the US1 plumbing; extends `family.cpp`, `runner.cpp`, `report.cpp` and `benchmark.hpp`.
- **US5 (P2)**: Foundational plus US4's fixture pair, because the order it asserts (setup, `setUp`, callable, `tearDown`, teardown) names the fixture hooks.
- **US6 (P3)**: Foundational plus the US1 plumbing for the prefix test; its example and documentation tasks need the whole surface, so they land last.

### Within Each User Story

- Tests written and observed failing before the implementation task that turns them green
- Declaration before definition: `benchmark.hpp` before `registry.cpp` and `family.cpp`
- Expansion before selection: `family.cpp` before `cli.cpp`
- The story's green task runs last

### Same-File Serialization (the binding constraint)

- `include/speedgun-ng/benchmark.hpp`: T009, T013, T024, T025, T029, T030, T031, T037 — one at a time, in that order
- `source/harness/family.cpp`: T002, T005, T010, T012, T018, T019, T032
- `source/harness/cli.cpp`: T005, T014, T020, T042
- `source/harness/runner.cpp`: T005, T015, T034, T039
- `source/harness/registry.cpp`: T011, T038
- `test/CMakeLists.txt`: every test-registration task — T007, T008, T017, T022, T023, T027, T036, T041
- `test/source/harness_instance_name_test.cpp`: T017 then T028
- `CMakeLists.txt`: T001 alone

### Parallel Opportunities

- T007 and T008 are two different test files blocked only on Foundational
- T022 and T023 are two different test files
- T027 and T036 are two different test files, and T041 is a third
- Across stories once US1's plumbing (T014, T015) is in place: US3's macro tasks (T024, T025) touch no file US2 edits, so US2's `family.cpp`/`cli.cpp` work and US3's `benchmark.hpp` work run together
- T043 and T044 are two different files
- T046, T047, T048, T049 are four separate build trees and four separate gates
- Wave width is capped at four runnable nodes at a time by the operator's instruction; a fifth ready node queues and does not run

### Parallel Example: User Story 1

```text
Together (different files, both blocked only on Foundational):
  T007 test/source/harness_family_test.cpp
  T008 test/source/harness_argument_test.cpp
Then serially (same-file and link dependencies):
  T009 benchmark.hpp declarations -> T013 benchmark.hpp State view
  T010 family.cpp builders -> T012 family.cpp expansion
  T011 registry.cpp family calls
  T014 cli.cpp walk -> T015 runner.cpp instance -> T016 ctest green
```

---

## Implementation Strategy

### MVP First (User Story 1 only)

1. Phase 1 Setup (T001-T002)
2. Phase 2 Foundational (T003-T006) - critical, blocks everything
3. Phase 3 US1 (T007-T016)
4. **Stop and validate**: quickstart §2 and §4 - one family call, the instance set of the cited revision, each instance reading its own arguments

### Incremental Delivery

1. Setup + Foundational -> the version effect, the new translation unit, the instance seam, a green H1 baseline
2. US1 -> the MVP: families, expansion, argument access
3. US2 -> names, selection, the duplicate rule
4. US3 -> capture and templates
5. US4 -> fixtures and the suite split
6. US5 -> the callback pair
7. US6 -> the `DISABLED_` gate, the example, the documentation
8. Polish -> sanitizers, coverage, prose, naming, the release preset, the quickstart sweep

---

## Settled differently, recorded at completion

Beyond the entries above, these are the places where this feature settled a question by argument, with the constitution outranking the cited reference implementation, and the gate facts a later reader needs to reproduce the verdicts.

- `__COUNTER__` is not used to make the generated identifiers of `SG_BENCHMARK_TEMPLATE` unique. The cited revision uses it, and the first cut here did too. It is a compiler extension, and constitution.md:442 bars code that needs one; FR-028 says the feature shall add none. The identifier is derived from `__LINE__` through a two-link paste chain instead, because a parameter standing next to `##` is pasted unexpanded and needs a link that expands it first. The stated ceiling is two registrations of one function template on one source line. The check is `clang++ -pedantic-errors`
- The free family builders are header-declared and defined in `source/harness/family.cpp`, and `apply` is header-inline with its guard in `registry.cpp` as `applyGuard`, the same split the existing `addMetric` and `addMetricCore` use. A template cannot live in the library translation unit, and a guard in the header would duplicate the contract text
- `kDefaultRangeMultiplier` is public in namespace `sg` in `include/speedgun-ng/benchmark.hpp` rather than internal, because `createRange`'s default argument needs it at the point of declaration. `source/harness/detail/internal.hpp` no longer declares it
- The suites that script a counter provider register it once per process. The counters system refuses registration once it is open (`source/counters/system.cpp:290-296`, 007 FR-009), so a suite that registers per scenario is invalid from its second scenario onward. The scenarios read deltas of the one provider, which is what `harness_calibration_test.cpp:487` already does by re-exec
- The prose gate cannot see untracked files in either mode: `tools/prose/prose_gate.py:592` enumerates through `git ls-files`. A green `cmake -P cmake/prose-lint.cmake` over an uncommitted feature proves nothing about the files the feature adds. The lead drove the gate's own matchers over them and brought the sweep to zero findings
- `FIX=YES` in `cmake/lint.cmake` and `cmake/spell.cmake` is read at configure time, so `cmake --build <dir> --target format-check -- FIX=YES` does nothing. The working invocation is the configure-time one
- A failed build does not stop `ctest` from reporting Passed: it runs the previous binary. `harness_callback_test` reported Passed twice while its compile was failing. Every gate run in this feature pairs the build exit status with the test run, and a nonzero build status voids the test result
- `consoleRowScenario` asserts the report contains no `suite` substring anywhere, which is wider than the row prefix it means to protect. It holds today because `printContext` never prints that word; if a context line ever does, the assertion has to narrow to the row prefix
- `cmake/coverage.cmake` now passes `inconsistent` to lcov's `--ignore-errors` list beside the two classes already there. lcov 2.3 aborts the whole capture when a function's recorded end line and the last line of its body disagree, and `SG_BENCHMARK_F` plus its six siblings generate a case function whose body spans several source lines while gcov records the generated function at the macro invocation line. The class names that parsing complaint; the capture carries the body's own line and branch counters either way, and every other lcov check stays live
- Three `LCOV_EXCL_LINE` markers sit on the closing braces of `createRange`, `createDenseRange` and `instanceName` in `source/harness/family.cpp`, following the convention feature 007 set at `include/speedgun-ng/counters_provider.hpp:90-96`. Each comment names T048 and the verification: `gcov -b -i` on the coverage tree reports those records with count 0 and no block record at all, while the `return` line above each reports a count. The first draft of this entry also marked `createRange`; the next capture showed that brace covered, so the marker and its claim were removed
- `addRange` in `source/harness/family.cpp` carried a guard `if (high != values.back())` before pushing the high bound. The power sweep above it stops at `high - 1` and `low` stands at the head of the list, so the comparison is true on every path and the false branch is unreachable; the coverage gate named it as an uncovered branch. The guard is deleted and the push is unconditional, with a comment stating why. The exact-list checks of `test/source/harness_family_test.cpp`, which pin the output of coincident pairs, adjacent pairs, negative pairs, pairs with a power inside, pairs with none inside and a bound at the limit of the type, are the regression that proves the deletion changes no list
- The FR-022 audit entry below first read the header diff as removing zero lines. Re-run at the completion head it removes five: three `\pre none` doxygen lines inside one comment block, and two lines of `State`'s constructor, whose recorder parameter went from a reference to a pointer so a callback state can carry no recorder window. That constructor sits under `private:` with `friend class detail::Runner`, so it lies outside the surface FR-022 enumerates, and the `SOVERSION` rise of T001 is what covers it


## Execution Log

### CI-3 the coverage markers, 2026-10-10

Three builders in `source/harness/family.cpp` ended with a named
local, contract checks and `return` of that local. The local is
then the return object itself: nothing runs at the closing brace,
and gcov 16 attaches an unexecutable count-zero line record to it
(`=====` in the text report, no block record). The T048 markers hid
the six brace lines. Each function now returns a separate object,
`std::vector<std::int64_t>(values)` or `std::string(name)`, so the
local's destructor runs at the closing brace, the brace line
executes and its record is covered. A probe on four return shapes
(`-O0 --coverage`, `gcov -b`) separated the cause: named-local and
block-wrapped returns keep the `=====` brace, a copy-expression
return covers it. All `\pre`, `\post` and `\invariant` clauses and
their `SG_REQUIRE`, `SG_ENSURE` and `SG_INVARIANT` checks stand
where they were.

### CI-2 the gap mode aborts, 2026-10-10

The failing case is `harness_gap_test` in the test, sanitize, tsan
and coverage jobs: the `namesListing` child calls
`availabilityName` with a value outside the closed enumeration, the
six-name table is subscripted past its end, and libstdc++'s
`_GLIBCXX_ASSERTIONS` subscript check aborts the child. Locally the
suite stayed green because the mode check read
`WEXITSTATUS(status)` of a status that reports a signal: that read
yields zero, so an aborted child passed. `strace -e wait4` on the
parent shows the child dying of `SIGABRT` while the check compares
zero to zero. The causes are two: the missing tail of
`availabilityName`, restored behind a bounds guard, and the vacuous
mode check, now gated on `WIFEXITED`.

### CI-1 the ignore build warns, 2026-10-10

Under `speedgun-ng_CONTRACTS=ignore` the enforcement macros expand
to nothing and six locals that feed only contract checks became
unused; the consumer-release job builds that semantic with
`-Wunused -Werror` and stopped. The `ci-linux-ignore` Release
configure reproduced it: five named locals and the range-for
binding of the `argsProduct` emptiness check. All six carry
`[[maybe_unused]]` and the ignore build compiles clean.

### CF-4 callback-state rule and end(), 2026-10-10

CF-4 changed no behavior, so no covering test could fail first: the
runner never guarded `end()` and the guard scenario already aborted
only on `begin()`, `skipWithError` and `skipWithMessage`. The wrong
claim lived in the spec alone: the Clarifications entry and FR-018
listed `end()` among the precondition violations. Both now state the
rule as the contracts, the plan, the data model and the C-7 section
already did. The new `bmCbEndLegal` registration and
`endLegalScenario` pin the legality: the setup callback calls `end()`,
returns, and the run prints its row; the suite passed on first run
against the unguarded code, which is the state the corrected spec
describes.

### CF-3 argName replace semantics, 2026-10-10

The covering test went first. With the appending `argName`, the
`bmRelabel` row of `kNameCases` aborted the list-mode child: "a label
count unequal to the family arity violates the bound label count ==
family arity (FR-006)" at `source/harness/family.cpp:222`, and the
binary exited 134. The fix replaced the list in `argName` behind the
unchanged arity check, added the class-row invariant with
`SG_INVARIANT` enforcement in both label members, and switched the
example to one `argNames` call. The suite then passed, the pair gate
returned 187 interfaces with zero gaps, and the example still prints
`bmArgs/width:8/depth:16`.

### CF-2 chained registration macros, 2026-10-10

The covering tests went first. With the registrar-object macros in
place, the four chained sites did not compile: `harness_family_test.cpp
:69` reported "cannot use dot operator on a type", `harness_template_test.cpp
:318` the same, `harness_capture_macro_test.cpp :309` "expected
expression", and `harness_fixture_test.cpp :278` "expected ';' after
top level declarator". The macro rework replaced each registrar struct
with a `[[maybe_unused]] static const ::sg::BenchmarkHandle` initialized
by the registration call, added the three-link `__LINE__` paste chain
to `SG_BENCHMARK` and `SG_BENCHMARK_CAPTURE`, and gave every site its
`;`. The dev build then passed and `ctest` reported 68 of 68, the four
chain suites among them; the example prints `bmArgs/width:8/depth:16`
and `bmArgs/width:16/depth:32` from its chained site.

### CF-1 fixture pair state, measured by the lead, 2026-10-10

The covering tests went first and the red state is recorded: with the
runner handing the run state to the pair, `begin()` in `setUp` entered
the timed loop legally, the child printed "the benchmark entered the
timed loop twice" and exited 0, and the suite failed the check "the
loop or skip operation in the fixture pair is a precondition violation
(FR-017, FR-018)". The fix hands `callbackState` to `setUp` and
`tearDown` in `source/harness/runner.cpp`; the order setup, setUp,
callable, tearDown, teardown stands. The suite then passes: three
re-exec abort modes and the read scenario of `ReadStateFixture/run/7`.

### Convergence pass, measured by the lead, 2026-10-10

T054 to T057 close the four findings of the converge assessment. The dbc
pair gate ran for this feature for the first time and reported 20 of 187
interfaces drifted, every row a feature-016 interface: documented `\post`
and `\invariant` clauses with no macro of that kind in the interface
scope. The gate attributes a member's invariant to the class row, so a
member block cannot pair one; the member blocks now document the `\pre`
and `\post` pair the H1 setters pair, the const accessors mark `\post
none`, and the eight family calls that lacked one gained `SG_ENSURE`
postconditions. `apply` carries its `SG_REQUIRE` and `SG_ENSURE` in its
own scope, and `applyGuard` stays for the run-started check the header
cannot read on the incomplete `RegistryEntry`. The two builders carry
`SG_INVARIANT` for their never-empty rule. `instanceName` skips an empty
label and its colon, the cited revision's rule the spec edge case states,
pinned by the `bmBlank` row of the name table. The fixture suite registers
and runs `SG_BENCHMARK_TEMPLATE_DEFINE_F` with `SG_BENCHMARK_REGISTER_F`
and `SG_BENCHMARK_TEMPLATE_METHOD_F` with
`SG_BENCHMARK_TEMPLATE_INSTANTIATE_F`. Measured: dev build exit 0,
`ctest --test-dir build/dev` 100% tests passed out of 68, doc gate 187
interfaces 0 gaps, pair gate 187 interfaces 0 gaps, prose 0 findings,
format check clean.

Record here, as the work lands: the T006 baseline pass count, each red observation of T007, T008, T017, T022, T023, T027, T028, T036 and T041, each green run of T016, T021, T026, T035, T040 and T045, the four gate verdicts of T046-T049, the FR-022 audit of T050, the quickstart §1-§13 outcomes of T051, and the release-build record of T052.


### T006 baseline, measured by the lead, 2026-10-10

`cmake --build build/agent-016a3 -j 16` exit 0, then `ctest --test-dir build/agent-016a3 --output-on-failure` exit 0 with `100% tests passed out of 60`, total test time 122.16 s. The tree at that point carried T001 through T005: the version and `SOVERSION` rise, the E-01 family fields, the E-02 `Instance`, `expandRegistry()` with one instance per family record, and `Runner::run(Instance&)`. `loop_shape` and `cli_shape` are among the 60 and both Passed, so the codegen gates survived the rewiring.

### Red observations, measured by the lead

The eight new suites are registered by the implementation task that turns each one green (see Deviations), so their red state is observed as compiler diagnostics on the test file itself. Captured on the tree before T009 landed: `no_member` for `arg`, `args` and `argName` on `sg::BenchmarkHandle` in `harness_family_test.cpp`, `harness_argument_test.cpp` and `harness_instance_name_test.cpp`; `undeclared_var_use` for `SG_BENCHMARK_CAPTURE` in `harness_capture_macro_test.cpp` and for `SG_BENCHMARK_TEMPLATE` in `harness_template_test.cpp`; `expected_class_name` where `harness_fixture_test.cpp` derives from `sg::Fixture`, with the `override` and `unknown_typename` diagnostics that follow from it; `no_member` for `suite` and `caseName` in `sg::BenchmarkResult` in `harness_instance_name_test.cpp`.

### T016 green, measured by the lead, 2026-10-10

`cmake --build build/dev -j 16` exit 0, then `ctest --test-dir build/dev -R 'harness_family_test|harness_argument_test' --output-on-failure` exit 0 with `100% tests passed out of 2` — `harness_family_test` 0.46 s, `harness_argument_test` 0.19 s. Before the correction the same pair ran 1 of 2: `harness_argument_test` failed on `range(1) on a one-argument family is a precondition violation (FR-008)` because its re-exec filtered `^bmOneRead$` while a one-argument family names its instance `bmOneRead/8` (FR-010), so the filter matched nothing and the process exited 0 instead of violating. The full-suite run that preceded it was `60/61` with `harness_family_test` the only failure, on five suite rows the lead corrected against the artifacts: `ranges` is a product of grown pairs, `argsProduct` advances the first list fastest, and one `args(list)` call is one instance of that arity.

### T021 and T026 green, measured by the lead, 2026-10-10

`cmake --build build/dev -j 16` exit 0 with zero `error:` lines, then a full `ctest --test-dir build/dev --output-on-failure`: 65 tests, and after the two corrections below every one of the five registered feature-016 suites passes — `harness_family_test` 0.34 s, `harness_capture_macro_test` 0.05 s, `harness_template_test` 0.03 s, `harness_instance_name_test` 0.03 s, `harness_argument_test` 0.13 s. The macros were additionally compiled out of tree at `/tmp/f016_macro_check.cpp` under the repository warning set, exit 0, and the object carries `bmSort<int>`, `bmSort<double>`, `bmSort<std::pair<int, int>>`, `bmSum/pair8_64`, `bmBare/bare` with generated identifiers `sgBenchmarkRegistrar_bmSort_24/_25/_26` (FR-013, FR-014, FR-023, FR-028).

Two corrections the lead made, both in test sources: `harness_capture_macro_test.cpp` accumulated an unsigned product into a `long long`, which `-Wsign-conversion -Werror` rejects, and its FR-012 scenario counted rows matching `bmDup/dup` — a name that scenario never registers — while giving both clashing registrations the same body, so "the earlier instance stays" could not be read from the report at all. The scenario now registers the earlier family with `bmDupEarlier` and the later with `bmDupLater`, and reads four facts: one `bmDup/8` row, the earlier mark present, the later mark absent, and `bmOther/` still running.

### Completion head: the five success criteria, measured by the lead, 2026-10-10

Criterion 2, build and full suite at the head: `cmake --preset=ci-ubuntu` exit 0, `cmake --build build -j 20` exit 0 with zero `error:` lines, `ctest --test-dir build --output-on-failure` exit 0 with `100% tests passed out of 68` (107.75 s). The eight suites this feature adds each report Passed: `harness_family_test` 0.31 s, `harness_capture_macro_test` 0.02 s, `harness_template_test` 0.01 s, `harness_instance_name_test` 0.02 s, `harness_fixture_test` 0.02 s, `harness_callback_test` 0.17 s, `harness_argument_test` 0.13 s, `harness_disabled_test` 0.02 s, tests 61 to 68.

Criterion 3, codegen and governance: in that same run `time_source_gate` Passed 0.06 s (test 49), `barrier_shape` 0.21 s, `loop_shape` 0.88 s, `cli_shape` 0.00 s, and the targeted run `ctest --test-dir build -R 'loop_shape|cli_shape|barrier_shape|time_source_gate'` reports `100% tests passed out of 4`. `grep -n 'VERSION 0.7.0' CMakeLists.txt` gives one hit at line 7, `grep -n 'SOVERSION 3' CMakeLists.txt` one hit at line 219. `cmake -P cmake/prose-lint.cmake` exits 0 with no finding, the tree mode reports 307 sources and 0 findings, and the sweep of the gate's own matchers over the 20 untracked files the feature adds reports 2304 units and 0 findings.

Criterion 4, the real surface: `./build/example/benchmark_example --list` prints `bmTouch`, `bmFill<std::uint64_t>`, `TableFixture/bmTableTouch`, `bmSum`, `bmArgs/width:8/depth:16` and `bmArgs/width:16/depth:32`, with the registered `DISABLED_bmTouch` absent. `./build/example/benchmark_example --dry-run` prints one row for each of those six, so the two family instances, the fixture method and the templated benchmark all appear, and `echo $?` gives 0. `--filter '^DISABLED_bmTouch

Clean sequence at the head: `cmake --build build/coverage -j 8` exit 0 with zero `error:` lines, `find build/coverage -name '*.gcda' -delete` so no stale counter inflates the verdict, `ctest --test-dir build/coverage` exit 0 with `100% tests passed out of 68` (130.50 s), then `cmake --build build/coverage --target coverage` exit 0. The gate reports `lines.......: 100.0% (3084 of 3084 lines)` and `branches....: 100.0% (1200 of 1200 branches)`, and the per-file table reads 100% on both for every file the feature touches: `harness/family.cpp` 112 lines and 57 branches, `harness/registry.cpp` 119 and 28, `harness/cli.cpp` 123 and 77, `harness/runner.cpp` 269 and 113, `benchmark.hpp` 92 lines and 12 branches. `tools/dbc/coverage_gate.sh` is the checker, and it reads those two thresholds; the contract lines of `SG_REQUIRE`, `SG_ENSURE`, `SG_INVARIANT` and `SG_ASSERT` are excluded by `cmake/coverage.cmake:76`, which is the exclusion the task text names. Function coverage reports 98.0% overall and 91.7% of the 36 functions in `benchmark.hpp`; the gate tests no function threshold, and the number is recorded here rather than hidden.

The path from the first capture to that verdict is the work. The first run reported 99.4% lines with `family.cpp` at 84.5% lines and 79.7% branches, and the named gaps were: no test ever swept a negative bound, so `addNegatedPowers` and the zero-crossing branches of `createRange` and `createDenseRange` were dead; the family expansion that drops a clashing instance name (`family.cpp:379`) and the fixture registration that meets a taken name (`registry.cpp:343-344`) had no scenario; one guard in `addRange` was unreachable by construction; and three gcov line records could not be advanced. The fixes, in order: builder checks for negative pairs, coincident pairs, adjacent pairs, pairs with a power inside, pairs with none inside, a pair whose bound sits beyond the last power and a pair whose bound sits at the limit of the type (`createRange(1, INT64_MAX)` stops power growth at `2^60` and still lands the high bound); the `family-clash` and `fixture-clash` re-execs of `test/source/harness_instance_name_test.cpp`; the deleted guard; and three commented `LCOV_EXCL_LINE` markers with the `gcov -b -i` evidence beside them. Two earlier attempts died before any verdict with an lcov `inconsistent` error, which is what led to narrowing that one error class in `cmake/coverage.cmake`.

### T051 quickstart sections 1 to 13, run against the built tree, 2026-10-10

Every section ran. The gates of SC-001 to SC-009 are green: the two version greps, the eight capability runs (`harness_family_test` 0.35 s, `harness_instance_name_test` 0.04 s, `harness_argument_test` 0.20 s, `harness_capture_macro_test` 0.03 s, `harness_template_test` 0.03 s, `harness_fixture_test` 0.07 s, `harness_callback_test` 0.25 s, `harness_disabled_test` 0.06 s, each exit 0), the compile-fail pair (2 tests, exit 0), the example build and dry run (six rows, two family rows, one fixture-method row, one templated row, exit status 0), `loop_shape` 0.96 s, and the prose gate in both modes.

Two sentences of the quickstart describe the tree wrongly and the lead corrected neither artifact, recording the mismatch instead. Section 8 says the negative translation units under `test/compile-fail/` prove the R-05 callback-state rule; that directory holds no feature-016 unit and `test/CMakeLists.txt` registers only `dbc_compile_fail` and `counters_compile_fail`, because R-05 is a runtime `SG_REQUIRE` and no compile-time negative can express it. Section 12 says the loop-shape script prints no `FAIL` line for any compiler present; `test/loop_shape.sh:220-232` runs a liveness probe that requires the analyzer to fail on a planted loop, so the `FAIL g++` and `FAIL clang++` lines are the expected negative control and the verdict line is the pass.

One pre-existing test is load-sensitive: `counters_overhead` (feature 015) failed once in a full run under parallel load, at `test/source/counters_overhead.cpp:231`, where the check is a strict `cost.medianNs < bracketedMedian` and both medians rose to 170.0 ns together. Three direct re-runs and three `ctest -R counters_overhead` re-runs passed, and the next full run reported `100% tests passed out of 68`. The feature head does not touch that code path; the flake is recorded, not fixed.

### T046 sanitizer preset, and T050 the FR-022 signature audit, measured by the lead, 2026-10-10

Sanitizer preset at the head: `cmake --preset=ci-sanitize`, `cmake --build build/sanitize -j 8` exit 0, `ctest --test-dir build/sanitize --output-on-failure` exit 0 with `100% tests passed out of 68`, total 110.02 s, and `grep -cE 'AddressSanitizer|LeakSanitizer|runtime error|ThreadSanitizer'` over the whole ctest log reports 0. The child that ran the same preset first proved the sanitizers were linked rather than merely configured: `build/sanitize/test/CMakeFiles/harness_family_test.dir/flags.make` carries `-fsanitize=address,undefined` and the binary carries 676 `__asan` and 109 `__ubsan` symbols.

FR-022: the declarations of `registerBenchmark`, `SG_BENCHMARK`, `speedgunMain`, the five `BenchmarkHandle` setters and the `State` methods were extracted from the merge base `30f31188167a8215dd636b786128a6a4a1ffe0c2` and from the head, and all nine baseline lines are present verbatim at the head with none missing. The structural form of the same audit was re-run at the completion head and its first reading was too strong: `git diff` of `include/speedgun-ng/benchmark.hpp` against the merge base removes five lines, three of them `\pre none` doxygen lines inside a comment block and two of them the `State` constructor's `counters::RecorderHandle<counters::HardStop>&` parameter and its initializer, where the recorder became a pointer so a callback state can carry none. That constructor sits under a `private:` label with `friend class detail::Runner` at `include/speedgun-ng/benchmark.hpp:414-420`, so it is not part of the H1 surface FR-022 enumerates, and the `SOVERSION` rise of T001 is what covers it. No enumerated signature changed.

### T052 release build and full suite, and T049 governance gates, measured by the lead, 2026-10-10

Release configuration at the head: `cmake --preset=ci-ubuntu`, `cmake --build build -j 24` exit 0 with zero `error:` lines, then `ctest --test-dir build --output-on-failure` exit 0 with `100% tests passed out of 68`, total 119.76 s. The eight feature-016 suites are tests 61 to 68 and each reports Passed: `harness_family_test` 0.36 s, `harness_capture_macro_test` 0.01 s, `harness_template_test` 0.01 s, `harness_instance_name_test` 0.01 s, `harness_fixture_test` 0.02 s, `harness_callback_test` 0.20 s, `harness_argument_test` 0.12 s, `harness_disabled_test` 0.02 s.

Governance gates: `cmake -DPROSE_MODE=tree -P cmake/prose-lint.cmake` exit 0, `307 sources, 48658 units examined, 0 findings`. `format-check` and `spell-check` were red at first and are green now: the format gate listed ten files (the header, `family.cpp`, `registry.cpp` and six feature-016 test files) and the sanctioned fix is the gate's own `clang-format --style=file -i` pass, after which both targets exit 0; the spelling gate listed three British-spelling findings, all in prose, and those are corrected. The naming clause was checked directly: `clang-tidy --checks='-*,readability-identifier-naming' -p build/dev` over `family.cpp`, `registry.cpp`, `runner.cpp`, `cli.cpp` and the four largest new suites reports zero findings.

Two gate facts worth recording. The prose gate enumerates sources through `git ls-files` (`tools/prose/prose_gate.py:592`), so in either mode it cannot see an untracked file, and this feature is uncommitted: nine new files plus the docs page sat outside its 307 sources. The lead drove the gate's own matchers over them (`load_rules`, `compile_prose_matchers`, `build_units`, `evaluate_unit` imported from `tools/prose/prose_gate.py`): 30 sources, 3248 units, 12 findings at first, all in the new test comments and in this file, every one corrected, and the sweep now reports 0 findings. The `FIX=YES` switch of `cmake/lint.cmake` and `cmake/spell.cmake` is read at configure time, so `cmake --build ... -- FIX=YES` does nothing; the working invocation is the configure-time one, or the formatter the gate itself calls.

### T045 green and the example surface proven, measured by the lead, 2026-10-10

`ctest --test-dir build/dev` exit 0: `100% tests passed out of 68`, total 114.48 s, with all eight feature-016 suites Passed, including `harness_disabled_test` 0.06 s. The `DISABLED_` gate stands at `source/harness/cli.cpp:249-261`, one `continue` on the expanded instance name before both the filter match and the list print, so the registry and the expansion keep the instance and the empty-selection report of the 015 contract is untouched.

The lead added a `DISABLED_bmTouch` registration to `example/benchmark_example.cpp` so the criterion-4 proof is not vacuous: the example registers a disabled family, `--list` prints six names with none of them disabled, `--dry-run` prints one row each for `bmArgs/width:8/depth:16`, `bmArgs/width:16/depth:32`, `TableFixture/bmTableTouch` and `bmFill<std::uint64_t>` with exit status 0, and `--filter "^DISABLED_bmTouch$"` reports `no benchmark matches the filter`.

### T040 green, measured by the lead, 2026-10-10: the callback pair, and three defects the child correctly refused to fix

The child landed the implementation and reported the suite as blocked by three defects in the lead-owned test source, which it may not edit. That report is accurate and its artifact citations check out (`source/counters/system.cpp:290-296` refuses registration once the system is open, 007 FR-009; `contracts/callbacks-and-state.md` states the order setup, setUp, callable, tearDown, teardown). The lead fixed the test source: two unused helpers deleted (`-Werror=unused-function`), `scriptedProvider()` made once-per-process with the contract cited in the comment, and the `OrderFixture` body made to log `call` so the order row tests the full contract sequence instead of four events of five.

The child had verified the R-05 guards only out of tree, so the registered suite did not yet cover the last clause of T036. The lead added `guardScenario()`: three registrations whose setup callback calls `begin()`, `skipWithError` and `skipWithMessage`, each run in a fork of the process and read as `SIGABRT` through `waitpid`, the fork-and-read-the-signal convention `test/source/dbc_test.cpp:332-363` already uses.

`ctest --test-dir build/dev -R harness_callback_test --output-on-failure` exit 0, Passed, 0.30 s. The scenario can fail: pointing the guard list at the non-violating `bmCbPair` prints `HARNESS CALLBACK TEST FAIL: the guarded call aborts in a callback state (R-05)` and exits 1. Note the trap this exposed - while the build was failing, `ctest` still reported the suite Passed from the stale binary, so a build failure must never be followed by a test run.

### Test-authoring tasks T007, T008, T017, T022, T023, T027, T028 closed on the lead's own runs

Each of these is the authoring of a suite that is now registered in `test/CMakeLists.txt` and green under a run the lead made personally, so they close on that evidence rather than on a child's report: `harness_family_test` 0.34 s, `harness_argument_test` 0.13 s, `harness_instance_name_test` 0.05 s, `harness_capture_macro_test` 0.05 s, `harness_template_test` 0.03 s, `harness_fixture_test` 0.07 s, all Passed. T028 is the C-9 pair inside `harness_instance_name_test.cpp`: the file carries a table of expected `suite` and `caseName` rows and `suiteCaseScenario` at line 372 asserts both against `BenchmarkResult`, including the fixture row where the suite is the fixture class name (Q-1).

### T035 green, measured by the lead, 2026-10-10

`ctest --test-dir build/dev -R 'harness_fixture_test|harness_instance_name_test|loop_shape' --output-on-failure` exit 0, `100% tests passed out of 3`: `loop_shape` 0.92 s, `harness_instance_name_test` 0.05 s, `harness_fixture_test` 0.07 s. All seven fixture macros are defined in `include/speedgun-ng/benchmark.hpp` and `sg::Fixture` carries the virtual pair. J1 resolved one conflict in its own favor of the artifacts: the fixture suite registered a fake provider per scenario, which `source/counters/system.cpp:290-296` refuses once the system is open (007 FR-009, `specs/007`:228), so the lead verified that citation and accepted the once-per-process guard.

### Gate baseline, measured by the lead


`cmake -P cmake/prose-lint.cmake` → `prose-lint: 5 sources, 759 units examined, 0 findings, 0 skipped`, exit 0, taken before the US3 to US6 nodes touch any prose-bearing file.

### Reference host

Linux 7.2.4-1-cachyos, AMD Ryzen 9 9950X3D, `perf_event_paranoid` = 1. Toolchain as recorded by the lead: `gcc (GCC) 16.2.1 20260810`, `clang version 23.1.1`, `cmake version 4.4.4`.

---

## Notes

- TDD mode is binding (plan Test Plan, Principle III, FR-029): a test task closes only when its failure has been observed
- Every new identifier follows N-1..N-12; the new macros are `SG_`-prefixed (FR-023)
- `State::end()` stays static: making it non-static would replace an H1 signature, which FR-022 forbids, and no precondition can guard it (R-05, FR-022)
- The threads feature stays out of scope: FR-019 defers it to the roadmap, and no task here opens a thread
- No task adds a compiler extension or a runtime dependency (FR-028), and no task adds a preset or a CI job
- Every time and counter value stays inside the counters library, so `test/time_source_gate.sh` stays clean (FR-025)
- Every new test runs on the scripted provider, stays deterministic, and needs no PMU (FR-029)
- Every new public declaration in `include/speedgun-ng/benchmark.hpp` carries a doxygen contract block and its matching `SG_REQUIRE` in the defining source, because `cmake --build build/dev -t dbc-gate` runs `tools/dbc/dbc_doc_gate.py` over the Doxygen XML of `include/speedgun-ng` and `tools/dbc/dbc_pair_gate.py` over that directory plus `source/` (Principle II, Principle VI). Run that target after any task that adds a public interface
- Registration order deviation, recorded deliberately: the eight test-writing tasks each say "register the target in `test/CMakeLists.txt` and observe it failing". The targets are instead registered by the implementation task that turns each suite green (T016 registers the T007 and T008 suites, T021 the T017 suite, and so on). Registering a suite whose API does not exist yet puts a non-compiling target in the tree and breaks `cmake --build` for every task running in parallel, which would hide the red/green signal rather than show it. The red state is still observed: each suite is authored first, its absence of the API is confirmed by the compiler diagnostics on the test file itself, and the implementation task then reports the suite going green
- Commit after each task or logical group, per the constitution's Pull Request Quality rules; the base branch stays linear

---

## Deviations

Accumulating section, kept current as the work proceeds; it is not written at the end. T053 closes it.

- Test-target registration order. The eight test-writing tasks each say "register the target in `test/CMakeLists.txt` and observe it failing". Each target is instead registered by the implementation task that turns its suite green (T016 registers the T007 and T008 suites, T021 the T017 suite, T026 the T022 and T023 suites, T035 the T027 and T028 suites, T040 the T036 suite, T045 the T041 suite). A registered target whose API does not exist yet makes `cmake --build` fail for the whole tree, which would hide the red/green signal from every task running beside it instead of showing it. The red state is still observed and still recorded: it is the compiler diagnostics on the test file itself — `no_member` for `arg`, `args` and `argName` on `sg::BenchmarkHandle`, `undeclared_var_use` for `SG_BENCHMARK_CAPTURE` and `SG_BENCHMARK_TEMPLATE` — and the implementation task then reports the suite going green
- Contract documentation ahead of definition in `source/harness/family.cpp`. `expandRegistry()` and `instances()` landed with doxygen contract blocks for `createRange`, `createDenseRange`, `instanceName`, `deriveSuiteAndCase`, `hasDuplicateInstance` and `checkFamilySize` and no definitions under them. The blocks are the contracts the later tasks implement, and every later task is instructed to put its definition directly under the existing block and keep the block's wording, so the text cannot rot into a comment that describes nothing. The `dbc_pair_gate.py` gate is unaffected: it pairs contract macros in `include/speedgun-ng` and `source/`, and no contract macro stands outside a definition
- FR-008 in the callback state is covered once, not twice. T008 asks that "the arguments reach the callback state too", and `test/source/harness_argument_test.cpp` carried a case for it that called `handle.setup()`, a US5 API (T037), which made a US1 suite unbuildable until US5 landed. `test/source/harness_callback_test.cpp` already covers it as a strict superset: `range(0)` of a one-argument instance at line 315, `rangeCount()` and `iterations()` of a two-argument instance at 322-323, and `rangeCount()` of a zero-argument instance at 327-330. The duplicated case is deleted from the argument suite and the requirement stays covered by the US5 suite, where the API it exercises lives
- The prose gate must be run in tree mode to cover this feature. `cmake -P cmake/prose-lint.cmake` defaults to `PROSE_MODE=range`, which takes its left edge from the merge base with `origin/master` and its right edge from `HEAD`; because this feature is uncommitted, `HEAD` is the merge base, the range is empty, and the reported "5 sources, 759 units" is not coverage of the feature. `tools/prose/prose_gate.py` accepts only `range` and `tree`, so the working-tree check is `cmake -DPROSE_MODE=tree -P cmake/prose-lint.cmake`, which reports `307 sources, 48580 units examined, 0 findings, 0 skipped` at exit 0 and covers every file the feature adds
- No commits. The Notes of this file say to commit after each task or logical group, per the constitution's Pull Request Quality rules. The standing instruction for this session forbids a commit without an explicit request, and the request never arrived, so the whole feature stays in the working tree on `016-harness-registration-and-fixtures` against merge base `30f3118`. The consequence is recorded rather than hidden: there is no per-task bisect history, and the recovery point for a wrecked tracked file is `git checkout -- <path>` against that merge base

## Artifact inconsistency carried forward deliberately

`quickstart.md` §8 and the plan's Test Plan row for C-7 state that the R-05
callback-state rule is covered by negative translation units under
`test/compile-fail/`. It cannot be: R-05 is a runtime `SG_REQUIRE` on a
`State&` whose kind is known only once the run has built it, and the
compile-fail harness at `test/compile-fail/run.sh` detects only
compile-time-decidable violations: its existing units are dimension-tag
mismatches and a `static_assert`-versus-`SG_REQUIRE` case. T036 therefore
observes the three R-05 violations through the self re-exec plus
nonzero-status pattern already used for a contract violation at
`test/source/harness_registry_test.cpp:209-212`, and no new
`test/compile-fail/` unit is invented for R-05. T053 records this as a
deviation against the artifacts.
` prints `no benchmark matches the filter` with exit 0.

Criterion 5, adjacent surface: the eight feature-015 suites each report Passed in the criterion-2 run (`harness_capture_test` 0.03 s, `harness_calibration_test` 0.10 s, `harness_growth_test` 0.00 s, `harness_cli_test` 1.10 s, `harness_catalog_test` 0.01 s, `harness_gap_test` 0.11 s, `harness_statistics_test` 0.02 s, `harness_registry_test` 0.06 s, tests 50 to 55, 59 and 60), and the FR-022 audit above shows no enumerated signature changed.

Criterion 1, the ledger: 53 task lines, every ID unique, zero lines without a repository path, 39 lines carrying a `[USn]` tag, all 30 FRs and all ten capabilities traced, and 53 of 53 boxes checked.

Other gates at the head, from the entries above: sanitizers 68 of 68 with zero reports, thread sanitizer 65 of 65 with zero races, coverage 100.0% lines and 100.0% branches, format and spelling gates green through their own fix paths, and `readability-identifier-naming` clean on the new translation units. The work is uncommitted: nine tracked files modified and sixteen untracked paths, on the branch `016-harness-registration-and-fixtures` with merge base `30f31188167a8215dd636b786128a6a4a1ffe0c2`.

### T048 coverage preset, measured by the lead, 2026-10-10

Clean sequence at the head: `cmake --build build/coverage -j 8` exit 0 with zero `error:` lines, `find build/coverage -name '*.gcda' -delete` so no stale counter inflates the verdict, `ctest --test-dir build/coverage` exit 0 with `100% tests passed out of 68` (130.50 s), then `cmake --build build/coverage --target coverage` exit 0. The gate reports `lines.......: 100.0% (3084 of 3084 lines)` and `branches....: 100.0% (1200 of 1200 branches)`, and the per-file table reads 100% on both for every file the feature touches: `harness/family.cpp` 112 lines and 57 branches, `harness/registry.cpp` 119 and 28, `harness/cli.cpp` 123 and 77, `harness/runner.cpp` 269 and 113, `benchmark.hpp` 92 lines and 12 branches. `tools/dbc/coverage_gate.sh` is the checker, and it reads those two thresholds; the contract lines of `SG_REQUIRE`, `SG_ENSURE`, `SG_INVARIANT` and `SG_ASSERT` are excluded by `cmake/coverage.cmake:76`, which is the exclusion the task text names. Function coverage reports 98.0% overall and 91.7% of the 36 functions in `benchmark.hpp`; the gate tests no function threshold, and the number is recorded here rather than hidden.

The path from the first capture to that verdict is the work. The first run reported 99.4% lines with `family.cpp` at 84.5% lines and 79.7% branches, and the named gaps were: no test ever swept a negative bound, so `addNegatedPowers` and the zero-crossing branches of `createRange` and `createDenseRange` were dead; the family expansion that drops a clashing instance name (`family.cpp:379`) and the fixture registration that meets a taken name (`registry.cpp:343-344`) had no scenario; one guard in `addRange` was unreachable by construction; and three gcov line records could not be advanced. The fixes, in order: builder checks for negative pairs, coincident pairs, adjacent pairs, pairs with a power inside, pairs with none inside, a pair whose bound sits beyond the last power and a pair whose bound sits at the limit of the type (`createRange(1, INT64_MAX)` stops power growth at `2^60` and still lands the high bound); the `family-clash` and `fixture-clash` re-execs of `test/source/harness_instance_name_test.cpp`; the deleted guard; and three commented `LCOV_EXCL_LINE` markers with the `gcov -b -i` evidence beside them. Two earlier attempts died before any verdict with an lcov `inconsistent` error, which is what led to narrowing that one error class in `cmake/coverage.cmake`.

### T051 quickstart sections 1 to 13, run against the built tree, 2026-10-10

Every section ran. The gates of SC-001 to SC-009 are green: the two version greps, the eight capability runs (`harness_family_test` 0.35 s, `harness_instance_name_test` 0.04 s, `harness_argument_test` 0.20 s, `harness_capture_macro_test` 0.03 s, `harness_template_test` 0.03 s, `harness_fixture_test` 0.07 s, `harness_callback_test` 0.25 s, `harness_disabled_test` 0.06 s, each exit 0), the compile-fail pair (2 tests, exit 0), the example build and dry run (six rows, two family rows, one fixture-method row, one templated row, exit status 0), `loop_shape` 0.96 s, and the prose gate in both modes.

Two sentences of the quickstart describe the tree wrongly and the lead corrected neither artifact, recording the mismatch instead. Section 8 says the negative translation units under `test/compile-fail/` prove the R-05 callback-state rule; that directory holds no feature-016 unit and `test/CMakeLists.txt` registers only `dbc_compile_fail` and `counters_compile_fail`, because R-05 is a runtime `SG_REQUIRE` and no compile-time negative can express it. Section 12 says the loop-shape script prints no `FAIL` line for any compiler present; `test/loop_shape.sh:220-232` runs a liveness probe that requires the analyzer to fail on a planted loop, so the `FAIL g++` and `FAIL clang++` lines are the expected negative control and the verdict line is the pass.

One pre-existing test is load-sensitive: `counters_overhead` (feature 015) failed once in a full run under parallel load, at `test/source/counters_overhead.cpp:231`, where the check is a strict `cost.medianNs < bracketedMedian` and both medians rose to 170.0 ns together. Three direct re-runs and three `ctest -R counters_overhead` re-runs passed, and the next full run reported `100% tests passed out of 68`. The feature head does not touch that code path; the flake is recorded, not fixed.

### T046 sanitizer preset, and T050 the FR-022 signature audit, measured by the lead, 2026-10-10

Sanitizer preset at the head: `cmake --preset=ci-sanitize`, `cmake --build build/sanitize -j 8` exit 0, `ctest --test-dir build/sanitize --output-on-failure` exit 0 with `100% tests passed out of 68`, total 110.02 s, and `grep -cE 'AddressSanitizer|LeakSanitizer|runtime error|ThreadSanitizer'` over the whole ctest log reports 0. The child that ran the same preset first proved the sanitizers were linked rather than merely configured: `build/sanitize/test/CMakeFiles/harness_family_test.dir/flags.make` carries `-fsanitize=address,undefined` and the binary carries 676 `__asan` and 109 `__ubsan` symbols.

FR-022: the declarations of `registerBenchmark`, `SG_BENCHMARK`, `speedgunMain`, the five `BenchmarkHandle` setters and the `State` methods were extracted from the merge base `30f31188167a8215dd636b786128a6a4a1ffe0c2` and from the head, and all nine baseline lines are present verbatim at the head with none missing. The structural form of the same audit was re-run at the completion head and its first reading was too strong: `git diff` of `include/speedgun-ng/benchmark.hpp` against the merge base removes five lines, three of them `\pre none` doxygen lines inside a comment block and two of them the `State` constructor's `counters::RecorderHandle<counters::HardStop>&` parameter and its initializer, where the recorder became a pointer so a callback state can carry none. That constructor sits under a `private:` label with `friend class detail::Runner` at `include/speedgun-ng/benchmark.hpp:414-420`, so it is not part of the H1 surface FR-022 enumerates, and the `SOVERSION` rise of T001 is what covers it. No enumerated signature changed.

### T052 release build and full suite, and T049 governance gates, measured by the lead, 2026-10-10

Release configuration at the head: `cmake --preset=ci-ubuntu`, `cmake --build build -j 24` exit 0 with zero `error:` lines, then `ctest --test-dir build --output-on-failure` exit 0 with `100% tests passed out of 68`, total 119.76 s. The eight feature-016 suites are tests 61 to 68 and each reports Passed: `harness_family_test` 0.36 s, `harness_capture_macro_test` 0.01 s, `harness_template_test` 0.01 s, `harness_instance_name_test` 0.01 s, `harness_fixture_test` 0.02 s, `harness_callback_test` 0.20 s, `harness_argument_test` 0.12 s, `harness_disabled_test` 0.02 s.

Governance gates: `cmake -DPROSE_MODE=tree -P cmake/prose-lint.cmake` exit 0, `307 sources, 48658 units examined, 0 findings`. `format-check` and `spell-check` were red at first and are green now: the format gate listed ten files (the header, `family.cpp`, `registry.cpp` and six feature-016 test files) and the sanctioned fix is the gate's own `clang-format --style=file -i` pass, after which both targets exit 0; the spelling gate listed three British-spelling findings, all in prose, and those are corrected. The naming clause was checked directly: `clang-tidy --checks='-*,readability-identifier-naming' -p build/dev` over `family.cpp`, `registry.cpp`, `runner.cpp`, `cli.cpp` and the four largest new suites reports zero findings.

Two gate facts worth recording. The prose gate enumerates sources through `git ls-files` (`tools/prose/prose_gate.py:592`), so in either mode it cannot see an untracked file, and this feature is uncommitted: nine new files plus the docs page sat outside its 307 sources. The lead drove the gate's own matchers over them (`load_rules`, `compile_prose_matchers`, `build_units`, `evaluate_unit` imported from `tools/prose/prose_gate.py`): 30 sources, 3248 units, 12 findings at first, all in the new test comments and in this file, every one corrected, and the sweep now reports 0 findings. The `FIX=YES` switch of `cmake/lint.cmake` and `cmake/spell.cmake` is read at configure time, so `cmake --build ... -- FIX=YES` does nothing; the working invocation is the configure-time one, or the formatter the gate itself calls.

### T045 green and the example surface proven, measured by the lead, 2026-10-10

`ctest --test-dir build/dev` exit 0: `100% tests passed out of 68`, total 114.48 s, with all eight feature-016 suites Passed, including `harness_disabled_test` 0.06 s. The `DISABLED_` gate stands at `source/harness/cli.cpp:249-261`, one `continue` on the expanded instance name before both the filter match and the list print, so the registry and the expansion keep the instance and the empty-selection report of the 015 contract is untouched.

The lead added a `DISABLED_bmTouch` registration to `example/benchmark_example.cpp` so the criterion-4 proof is not vacuous: the example registers a disabled family, `--list` prints six names with none of them disabled, `--dry-run` prints one row each for `bmArgs/width:8/depth:16`, `bmArgs/width:16/depth:32`, `TableFixture/bmTableTouch` and `bmFill<std::uint64_t>` with exit status 0, and `--filter "^DISABLED_bmTouch$"` reports `no benchmark matches the filter`.

### T040 green, measured by the lead, 2026-10-10: the callback pair, and three defects the child correctly refused to fix

The child landed the implementation and reported the suite as blocked by three defects in the lead-owned test source, which it may not edit. That report is accurate and its artifact citations check out (`source/counters/system.cpp:290-296` refuses registration once the system is open, 007 FR-009; `contracts/callbacks-and-state.md` states the order setup, setUp, callable, tearDown, teardown). The lead fixed the test source: two unused helpers deleted (`-Werror=unused-function`), `scriptedProvider()` made once-per-process with the contract cited in the comment, and the `OrderFixture` body made to log `call` so the order row tests the full contract sequence instead of four events of five.

The child had verified the R-05 guards only out of tree, so the registered suite did not yet cover the last clause of T036. The lead added `guardScenario()`: three registrations whose setup callback calls `begin()`, `skipWithError` and `skipWithMessage`, each run in a fork of the process and read as `SIGABRT` through `waitpid`, the fork-and-read-the-signal convention `test/source/dbc_test.cpp:332-363` already uses.

`ctest --test-dir build/dev -R harness_callback_test --output-on-failure` exit 0, Passed, 0.30 s. The scenario can fail: pointing the guard list at the non-violating `bmCbPair` prints `HARNESS CALLBACK TEST FAIL: the guarded call aborts in a callback state (R-05)` and exits 1. Note the trap this exposed - while the build was failing, `ctest` still reported the suite Passed from the stale binary, so a build failure must never be followed by a test run.

### Test-authoring tasks T007, T008, T017, T022, T023, T027, T028 closed on the lead's own runs

Each of these is the authoring of a suite that is now registered in `test/CMakeLists.txt` and green under a run the lead made personally, so they close on that evidence rather than on a child's report: `harness_family_test` 0.34 s, `harness_argument_test` 0.13 s, `harness_instance_name_test` 0.05 s, `harness_capture_macro_test` 0.05 s, `harness_template_test` 0.03 s, `harness_fixture_test` 0.07 s, all Passed. T028 is the C-9 pair inside `harness_instance_name_test.cpp`: the file carries a table of expected `suite` and `caseName` rows and `suiteCaseScenario` at line 372 asserts both against `BenchmarkResult`, including the fixture row where the suite is the fixture class name (Q-1).

### T035 green, measured by the lead, 2026-10-10

`ctest --test-dir build/dev -R 'harness_fixture_test|harness_instance_name_test|loop_shape' --output-on-failure` exit 0, `100% tests passed out of 3`: `loop_shape` 0.92 s, `harness_instance_name_test` 0.05 s, `harness_fixture_test` 0.07 s. All seven fixture macros are defined in `include/speedgun-ng/benchmark.hpp` and `sg::Fixture` carries the virtual pair. J1 resolved one conflict in its own favor of the artifacts: the fixture suite registered a fake provider per scenario, which `source/counters/system.cpp:290-296` refuses once the system is open (007 FR-009, `specs/007`:228), so the lead verified that citation and accepted the once-per-process guard.

### Gate baseline, measured by the lead


`cmake -P cmake/prose-lint.cmake` → `prose-lint: 5 sources, 759 units examined, 0 findings, 0 skipped`, exit 0, taken before the US3 to US6 nodes touch any prose-bearing file.

### Reference host

Linux 7.2.4-1-cachyos, AMD Ryzen 9 9950X3D, `perf_event_paranoid` = 1. Toolchain as recorded by the lead: `gcc (GCC) 16.2.1 20260810`, `clang version 23.1.1`, `cmake version 4.4.4`.

---

## Notes

- TDD mode is binding (plan Test Plan, Principle III, FR-029): a test task closes only when its failure has been observed
- Every new identifier follows N-1..N-12; the new macros are `SG_`-prefixed (FR-023)
- `State::end()` stays static: making it non-static would replace an H1 signature, which FR-022 forbids, and no precondition can guard it (R-05, FR-022)
- The threads feature stays out of scope: FR-019 defers it to the roadmap, and no task here opens a thread
- No task adds a compiler extension or a runtime dependency (FR-028), and no task adds a preset or a CI job
- Every time and counter value stays inside the counters library, so `test/time_source_gate.sh` stays clean (FR-025)
- Every new test runs on the scripted provider, stays deterministic, and needs no PMU (FR-029)
- Every new public declaration in `include/speedgun-ng/benchmark.hpp` carries a doxygen contract block and its matching `SG_REQUIRE` in the defining source, because `cmake --build build/dev -t dbc-gate` runs `tools/dbc/dbc_doc_gate.py` over the Doxygen XML of `include/speedgun-ng` and `tools/dbc/dbc_pair_gate.py` over that directory plus `source/` (Principle II, Principle VI). Run that target after any task that adds a public interface
- Registration order deviation, recorded deliberately: the eight test-writing tasks each say "register the target in `test/CMakeLists.txt` and observe it failing". The targets are instead registered by the implementation task that turns each suite green (T016 registers the T007 and T008 suites, T021 the T017 suite, and so on). Registering a suite whose API does not exist yet puts a non-compiling target in the tree and breaks `cmake --build` for every task running in parallel, which would hide the red/green signal rather than show it. The red state is still observed: each suite is authored first, its absence of the API is confirmed by the compiler diagnostics on the test file itself, and the implementation task then reports the suite going green
- Commit after each task or logical group, per the constitution's Pull Request Quality rules; the base branch stays linear

---

## Deviations

Accumulating section, kept current as the work proceeds; it is not written at the end. T053 closes it.

- Test-target registration order. The eight test-writing tasks each say "register the target in `test/CMakeLists.txt` and observe it failing". Each target is instead registered by the implementation task that turns its suite green (T016 registers the T007 and T008 suites, T021 the T017 suite, T026 the T022 and T023 suites, T035 the T027 and T028 suites, T040 the T036 suite, T045 the T041 suite). A registered target whose API does not exist yet makes `cmake --build` fail for the whole tree, which would hide the red/green signal from every task running beside it instead of showing it. The red state is still observed and still recorded: it is the compiler diagnostics on the test file itself — `no_member` for `arg`, `args` and `argName` on `sg::BenchmarkHandle`, `undeclared_var_use` for `SG_BENCHMARK_CAPTURE` and `SG_BENCHMARK_TEMPLATE` — and the implementation task then reports the suite going green
- Contract documentation ahead of definition in `source/harness/family.cpp`. `expandRegistry()` and `instances()` landed with doxygen contract blocks for `createRange`, `createDenseRange`, `instanceName`, `deriveSuiteAndCase`, `hasDuplicateInstance` and `checkFamilySize` and no definitions under them. The blocks are the contracts the later tasks implement, and every later task is instructed to put its definition directly under the existing block and keep the block's wording, so the text cannot rot into a comment that describes nothing. The `dbc_pair_gate.py` gate is unaffected: it pairs contract macros in `include/speedgun-ng` and `source/`, and no contract macro stands outside a definition
- FR-008 in the callback state is covered once, not twice. T008 asks that "the arguments reach the callback state too", and `test/source/harness_argument_test.cpp` carried a case for it that called `handle.setup()`, a US5 API (T037), which made a US1 suite unbuildable until US5 landed. `test/source/harness_callback_test.cpp` already covers it as a strict superset: `range(0)` of a one-argument instance at line 315, `rangeCount()` and `iterations()` of a two-argument instance at 322-323, and `rangeCount()` of a zero-argument instance at 327-330. The duplicated case is deleted from the argument suite and the requirement stays covered by the US5 suite, where the API it exercises lives
- The prose gate must be run in tree mode to cover this feature. `cmake -P cmake/prose-lint.cmake` defaults to `PROSE_MODE=range`, which takes its left edge from the merge base with `origin/master` and its right edge from `HEAD`; because this feature is uncommitted, `HEAD` is the merge base, the range is empty, and the reported "5 sources, 759 units" is not coverage of the feature. `tools/prose/prose_gate.py` accepts only `range` and `tree`, so the working-tree check is `cmake -DPROSE_MODE=tree -P cmake/prose-lint.cmake`, which reports `307 sources, 48580 units examined, 0 findings, 0 skipped` at exit 0 and covers every file the feature adds
- Commits were deferred during the implement run. The Notes of this file say to
  commit after each task or logical group, per the constitution's Pull
  Request Quality rules. The implement session forbade commits, so the work
  landed in whole commits on `016-harness-registration-and-fixtures` for
  pull request #33 against merge base `30f3118`; the per-task bisect history
  the Notes call for does not exist for the original run
- Result-field filling file. T033 and the plan's physical view name
  `source/harness/report.cpp` as the file that fills the `suite` and
  `caseName` fields of `BenchmarkResult`. The filling stands in `Runner::run`
  (`source/harness/runner.cpp`), where the `Instance` is at hand, and
  `report.cpp` prints the row and carries no filling. The FR-021 behavior and
  `contracts/result-fields.md` are satisfied and tested; only the file
  attribution differs (T057)

## Artifact inconsistency carried forward deliberately

`quickstart.md` §8 and the plan's Test Plan row for C-7 state that the R-05
callback-state rule is covered by negative translation units under
`test/compile-fail/`. It cannot be: R-05 is a runtime `SG_REQUIRE` on a
`State&` whose kind is known only once the run has built it, and the
compile-fail harness at `test/compile-fail/run.sh` detects only
compile-time-decidable violations: its existing units are dimension-tag
mismatches and a `static_assert`-versus-`SG_REQUIRE` case. T036 therefore
observes the three R-05 violations through the self re-exec plus
nonzero-status pattern already used for a contract violation at
`test/source/harness_registry_test.cpp:209-212`, and no new
`test/compile-fail/` unit is invented for R-05. T053 records this as a
deviation against the artifacts.

## Phase 10: Convergence

Assessment of 2026-10-10 against the committed head `88f69c3` (the
feature is committed and the tree is clean, superseding the No-commits
Deviations entry). Measured green at the head: dev build exit 0 and
`ctest --test-dir build/dev` 68/68, the prose gate in tree mode (330
sources, 0 findings), the version and `SOVERSION` greps, the H1
signatures of FR-022 present verbatim, the `DISABLED_` gate on the
expanded instance name, expansion before the filter, the callback-state
guards, the suite-grouped expansion, the ten-section
`docs/pages/harness.md`, and the example's family, fixture, and template
rows. The tasks below are the remaining work.

- [x] T054 CRITICAL: Pair the contract blocks of the 20 drifted feature-016 interfaces with enforcement in the interface scope, or narrow the blocks to the H1 convention, and bring `cmake --build build/dev -t dbc-gate` to exit 0: the pair gate reports 20 of 187 interfaces drifted with 31 documented-not-enforced clauses, every drifted row a feature-016 interface (`arg`, the two `args`, `range`, `rangeMultiplier`, `ranges`, `denseRange`, `argsProduct`, `apply`, `argName`, `argNames`, `setup`, `teardown`, `Fixture` with `setUp`/`tearDown`, `State::range`, `State::rangeCount`, `createRange`, `createDenseRange`); each documents `\pre`/`\post`/`\invariant` clauses its scope never enforces, and `apply`'s `\pre` stands in `applyGuard` outside the scope the gate scans; the passing H1 shape is `minTime` (`SG_REQUIRE` plus `SG_ENSURE` for its `\pre`/`\post`) and `name()` (`\pre none`/`\post none`); the CI `dbc-gate` job at `.github/workflows/ci.yml:669` fails at the head today, and no Execution Log entry records the target per FR-024, SC-008, Constitution II, VI, VIII (contradicts)
- [x] T055 Register and run the three untested fixture macros `SG_BENCHMARK_TEMPLATE_DEFINE_F`, `SG_BENCHMARK_TEMPLATE_METHOD_F`, and `SG_BENCHMARK_TEMPLATE_INSTANTIATE_F` in `test/source/harness_fixture_test.cpp`, asserting the `BaseClass<types>/Method` name for each path: the macros stand at `include/speedgun-ng/benchmark.hpp:1250,1322,1350` and expand independently of `SG_BENCHMARK_TEMPLATE_F`, and no test or example uses them, so T027's "each of the seven macros of FR-016 registers and runs" and the plan's C-6 row stand unmet for three of seven per FR-016, T027, plan: Test Plan C-6 (partial)
- [x] T056 Guard the label prefix of `instanceName` against an empty label at `source/harness/family.cpp:226-229` — it appends `label:` unconditionally, so `argName("")` names the segment `:value`, while the cited revision guards `if (!arg_name.empty())` in the `BenchmarkInstance` constructor of `benchmark_api_internal.cc` and the spec's edge cases state "an empty label leaves its segment without a label" — and pin the row in a name-table case per the spec Edge Cases, FR-001, FR-010 (contradicts)
- [x] T057 Record in the Deviations section that the `suite` and `caseName` fields are filled in `Runner::run` at `source/harness/runner.cpp:151-152`, not in `source/harness/report.cpp` as T033 and the plan's physical view state: the FR-021 behavior and `contracts/result-fields.md` are satisfied and tested, only the filling file differs, and the deviation is currently unrecorded per plan: physical view, T033 (partial)

## Phase 11: Content fixes

The content-fix pass of pull request #33. Each task is added and
closed in the same commit as its fix, and the Execution Log records
the red state of the covering test before the fix.

- [x] T058 Hand the fixture pair the callback state: the runner passes
  `callbackState` to `setUp` and `tearDown`, so `begin()`,
  `skipWithError` and `skipWithMessage` are precondition violations in
  the pair as FR-017 and `contracts/fixtures.md` state, the argument
  and iteration reads stay legal, and the wrapping order stands; the
  Q-5 entry in the spec records that one rule settled both pairs, and
  the C-6 section of `docs/pages/harness.md` states the state and the
  three rejected operations (FR-017, FR-018, R-05)
- [x] T059 Make the five registration macros yield the handle: each of
  `SG_BENCHMARK`, `SG_BENCHMARK_CAPTURE`, `SG_BENCHMARK_TEMPLATE`,
  `SG_BENCHMARK_REGISTER_F` and `SG_BENCHMARK_TEMPLATE_INSTANTIATE_F`
  expands to a declaration of one `sg::BenchmarkHandle` initialized
  from the registration call, the call is the last token sequence, the
  generated identifier carries the source line through the paste chain
  of `SG_BENCHMARK_EXPAND`, every site ends with `;`, the chained
  family sites of the family, template, capture and fixture suites
  stand, and the spec, both contracts and the C-1, C-4, C-5 and C-6
  sections state the chained form (FR-013, FR-014, FR-016, FR-022)
- [x] T060 Set the label list with `argName`: the call replaces the
  entry's label list with the one label while the arity check stands,
  `argNames` keeps its replace semantics, the class invariant of
  `BenchmarkHandle` carries the rule and both members enforce it with
  `SG_INVARIANT`, the `bmRelabel` row and the `arity-label` re-exec of
  `test/source/harness_instance_name_test.cpp` cover the two shapes,
  and the example states its labels through one `argNames` call
  (FR-006)
- [x] T061 Align the callback-state rule with `end()`: the
  Clarifications entry and FR-018 state that `begin()`,
  `skipWithError` and `skipWithMessage` are precondition violations
  while `end()` is static, touches no state, and stays legal; the
  other artifacts already carried that rule; the `bmCbEndLegal`
  registration and `endLegalScenario` of
  `test/source/harness_callback_test.cpp` pin the legality (FR-018,
  R-05)
- [x] T062 Keep the ignore build warning-free: the six locals of
  `source/harness/registry.cpp` that feed only `SG_ENSURE` and
  `SG_INVARIANT` carry `[[maybe_unused]]`, so the
  `speedgun-ng_CONTRACTS=ignore` build with `-Wunused -Werror` stays
  clean, and the sweep of `source/harness/` and
  `include/speedgun-ng/` found no other contract-only local
  (Principle II, FR-024)
- [x] T063 Fix the CI ctest failures at their causes:
  `availabilityName` answers a value outside the closed enumeration
  with `unknown` behind a bounds guard instead of subscripting the
  six-name table, the shape the gap suite pins for FR-024, and the
  mode check of `test/source/harness_gap_test.cpp` tests
  `WIFEXITED` before `WEXITSTATUS`, so a signalled child can no
  longer read as a passing exit status (FR-024, FR-035)
- [x] T064 Remove every `LCOV_EXCL` marker under `source/harness/`:
  `createRange`, `createDenseRange` and `instanceName` return a
  separate object instead of the named local, so the local's
  destructor runs at the closing brace and gcov records no
  unexecutable line; the six markers and their comments are
  deleted and the coverage target reports 100% line and branch
  coverage (FR-004, FR-005, FR-010, Principle VI)
