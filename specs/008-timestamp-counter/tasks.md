# Tasks: Raw Time-Stamp Counter

**Feature**: `008-timestamp-counter` | **Branch**: `008-timestamp-counter`

**Input**: Design documents from `specs/008-timestamp-counter/`

**Prerequisites**: plan.md (required), spec.md (required for user stories), research.md (R-001..R-009 and the correction record), data-model.md, contracts/ (system, provider), quickstart.md

**Tests**: INCLUDED. The plan's Test Plan records **TDD mode** (Principle III): every test task below was written before the implementation it covers and verified RED, and the implementation task that follows turned it GREEN. For `test/source/counters_tsc_test.cpp` the RED state was a compile failure, because the accessor the whole executable calls did not exist yet.

**Record status**: written after the implementation landed, so the order below is the order the edits were made and every task carries `[X]`. Where a design was corrected before the code, the Corrections section records the rejected reading next to the chosen one (Principle X.1). Where a step cost time, the Lessons section records it.

**Organization**: Tasks grouped by user story (US1..US3 per spec.md priorities) after a setup phase. Namespace `sg::counters` throughout. Frameworkless test convention: hand-rolled `check()`/`fail()` executables in `test/source/`, plain `add_test` in `test/CMakeLists.txt` (no test framework may be added, since the dependency-scan test forbids it).

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies on incomplete tasks)
- **[Story]**: US1..US3; absent on Setup and Gates tasks
- Every task names its exact file path and the requirement ids it satisfies

## Path Conventions

Per plan.md Project Structure: the public surface is flat in `include/speedgun-ng/counters*.hpp`; the implementation lives in `source/counters/` and is attached through the root `CMakeLists.txt` glob, so a new translation unit needs no edit; tests are flat in `test/source/counters_*.cpp`; the measured budget is published in `docs/pages/counters-overhead.md`. Line references cite the tree as 007 left it, which is the state the change starts from.

## Phase 0: Setup (Build Wiring Audit)

**Purpose**: establish that 008 needs no build-configuration edit for its library surface, so the only CMake work in the feature is a test registration. No public behavior yet.

- [X] T001 Audit the build wiring and record the result: 008 adds neither a source file nor a header, and neither would have needed an edit. `CMakeLists.txt:658` globs `source/counters/*.cpp` with `GLOB_RECURSE` and `CONFIGURE_DEPENDS`, and `cmake/install-rules.cmake:15` installs the whole `include/` directory. The export macro is class-level, so the new `system` member carries none. The one CMake edit in the feature is the four-line registration in T002 (FR-001, FR-004, plan "Build targets and link relationships", Principle IX)

**Checkpoint**: the wiring question is settled by reading the build, and the only build file the feature touches is `test/CMakeLists.txt`.

## Phase 1: User Story 1 - Publish the raw entry on every host that executes the instruction (Priority: P1)

**Goal**: the catalog lists the time-stamp entry wherever the build executes the instruction, and the entry carries a count with no rate attached. The runtime presence gate, the sysfs frequency read, the instruction-identifier read, and the calibration struct behind them all go.

**Independent Test**: `ctest --preset=dev -R "counters_tsc|counters_clock_push"` - an unprivileged process enumerates the `machine` catalog, finds `tsc` on a host that publishes no counter frequency, and opens a reader for it.

### Tests for User Story 1 (TDD - write FIRST, verify RED)

- [X] T002 [US1] Write `test/source/counters_tsc_test.cpp` and verify it RED. Its first scenario, `test_absent_provider`, calls `system::local().tsc()` before any provider is registered, because `system::local()` is a process singleton and the unregistered branch is reachable only in a fresh process; moving that call later silently loses the branch. The file then runs `register_fixture` (the clock provider, then a fake counted source at 1000 points), `test_raw_entry` (the entry is listed wherever the build executes the instruction, at `read_mode::fast_tsc` with `availability::countable`, `unit::none`, `unit_name` `none`, `frequency_hz` 0, `scaled` false, and a description stating the count is raw), `test_accessor_matches_lookup`, `test_accessor_reads_nothing` (a thousand calls leave every catalog entry identical), and `test_counter_composes`. Register the executable in `test/CMakeLists.txt` inside the 007 counters section with the four lines the section already uses: `add_executable`, `target_link_libraries` against `speedgun-ng::speedgun-ng`, `target_compile_features` for `cxx_std_23`, and `add_test` (FR-001..FR-008, US1 scenarios 1-3, US2 scenarios 1-3, SC-001, SC-002)

### Implementation for User Story 1

- [X] T003 [US1] Add `constexpr bool kTscAvailable` in `source/counters/clock_provider.cpp` beside the existing `kTscIndex` at lines 38 to 43, set to `true` under the `SG_COUNTERS_X86` build guard and to `false` under its `#else`, with a comment naming the two requirements it serves. One named condition then decides both the catalog seed and the reader opening (FR-001, FR-003, R-002)
- [X] T004 [US1] In `source/counters/clock_provider.cpp`, move the catalog seed out of the runtime presence check at line 268 and into the `SG_COUNTERS_X86` preprocessor guard, and rewrite the seed: the description becomes `raw time-stamp counter ticks; a count asserting no rate`, `frequency_hz` takes the zero default, and `scaled` takes `false`. The name, the `none` unit, `availability::countable`, and `read_mode::fast_tsc` are unchanged, and no field is added or removed (FR-001, FR-002, FR-011, R-002)
- [X] T005 [US1] In `source/counters/clock_provider.cpp`, drop the presence condition from the reader at line 309, replacing `m_tsc.present` with `!kTscAvailable`, so the reader opens the entry exactly where the catalog enumerates it and the two sites cannot drift (FR-003, R-002)
- [X] T006 [US1] Delete the private `tsc_calibration` struct at `include/speedgun-ng/counters_clock.hpp` lines 78 to 85 and its `m_tsc` member, and reduce the constructor's doxygen brief to a bare "Constructs the provider", because the class carries no state of its own once the calibration is gone. The `\pre` and `\post` pair stays, so the pairing gate still sees a documented contract (FR-011, FR-009, Principle X.2)
- [X] T007 [US1] Delete the whole guarded constructor block at `source/counters/clock_provider.cpp` lines 206 to 236, which held the `/sys/devices/system/cpu/tsc_khz` read, the integer parse, the CPUID leaf 0x16 nominal-frequency read, and the scaled comparison, then reduce the constructor to `= default`. Every clock-provider construction stops paying a file read whose three products nothing consumes (FR-011, R-002, Principle I)
- [X] T008 [US1] In `source/counters/clock_provider.cpp`, close the `LCOV_EXCL` region that opened at lines 139 to 145. The region existed because the calibration was kernel data no test could write, so the whole time-stamp arm was excluded; with the calibration gone the sampled arm is covered on every host this suite runs. The start marker stays with its reason rewritten to the build without the instruction, and the single `LCOV_EXCL_LINE` stays on the fallback `return 0`, which no fixture can reach (Principle VI, R-008)
- [X] T009 [US1] In `source/counters/clock_provider.cpp`, drop the `LCOV_EXCL_BR_START`, `LCOV_EXCL_BR_LINE`, and `LCOV_EXCL_LINE` markers from the sampled arm of `read_points` and restore the plain `default:` arm, because that arm is live on every host this suite runs and a marker left behind after its reason is gone opens a silent hole in the coverage denominator (Principle VI, R-008)
- [X] T010 [US1] In `source/counters/clock_provider.cpp`, remove the now-orphaned `#include <fstream>` and the nested `#include <cpuid.h>` inside the `SG_COUNTERS_X86` branch, and rewrite the file's header comment so it describes a raw time-stamp counter and cites 007's FR-033 beside 008's FR-001 (Principle I, Principle IV)
- [X] T011 [US1] Repair `test/source/counters_clock_push_test.cpp`, which asserted the behaviour T007 removed and could not be rewritten before it was removed. Replace the `SG_TEST_HAS_CPUID` guard, and the `<cpuid.h>` include behind it, with `SG_TEST_HAS_TSC` tracking the instruction the provider uses. Delete the orphaned `read_file()` and `nominal_core_khz()` helpers along with the `<fstream>` include. Convert the calibration scenario into a raw-entry scenario asserting `read_mode::fast_tsc`, `availability::countable`, `unit::none`, `frequency_hz` 0, `scaled` false, and a description containing `raw`, with a skip line naming the build guard when the entry is absent. Invert the leaf-appearance assertion at the catalog scenario to the build guard, so it is the proof that publication follows the guard and publishes nothing where the instruction is absent. Reword the reader-equivalence assertion to name 008's FR-003 in place of 007's FR-034. Add `using sg::counters::unit;` for the new unit assertion (FR-001, FR-002, FR-003, FR-011, R-008)
- [X] T012 [US1] Run `ctest --preset=dev -R "counters_tsc|counters_clock_push"` GREEN after an explicit rebuild of both test targets, then run the full `ctest --preset=dev`. Rebuilding the named target is the point, and the reason sits in the Lessons section (US1 checkpoint, SC-001)

**Checkpoint**: an unprivileged process enumerates the catalog, finds the entry on a host that publishes no frequency, and the reader opens it.

## Phase 2: User Story 2 - Reach the counter through one short name (Priority: P2)

**Goal**: `system::local().tsc()` returns the identical counter type the uniform lookup returns, so the two spellings are interchangeable at the call site and the short one composes with everything else.

**Independent Test**: `ctest --preset=dev -R counters_tsc` - call the accessor, compare it against `machine->counter<events>("tsc")`, and register a provider after the call.

### Tests for User Story 2 (written in T002, verified RED there)

The accessor scenarios share the one executable T002 wrote, because one file covers both stories and the accessor is what the US1 scenarios compose against. Their RED state is the same compile failure, which names the absent member.

- [X] T013 [US2] Map the accessor scenarios already in T002 onto the US2 requirements. `test_absent_provider` covers FR-007, and checks that the error names the absent provider and carries no catalog suggestion. `register_fixture` covers FR-006 by registering the clock provider and a counted source after a call. `test_accessor_matches_lookup` covers FR-004 by checking that both spellings resolve, that both name `tsc`, and that they agree on name and description. `test_accessor_reads_nothing` covers FR-005 by comparing the catalog before and after a thousand calls. `test_counter_composes` closes the story by dividing a counted source by the entry, compiling the quotient, and folding it to a positive ratio disclosing `running_ratio` 1.0 and `scaled` false. The presence assertions guard on `SG_TEST_HAS_TSC`, the same condition the provider uses, and the absent-counter path of FR-008 is reached through the tree, with no second platform test (FR-004..FR-008, US2 scenarios 1-3, SC-002)

### Implementation for User Story 2

- [X] T014 [US2] Declare `[[nodiscard]] auto tsc() const -> std::expected<counter<dim<0, 1>>, error>;` in `include/speedgun-ng/counters_system.hpp` after `objects()` and before `private:`. The doxygen block puts `\pre` and `\post` on their own lines, each declaring `none`, and the prose above the pair states that the call reads nothing, that it does not open the registration boundary, and which recoverable error each condition returns. Documented `none` pairs with a body that carries no contract macro, so the pairing gate reports a clean interface and no finding (FR-004, FR-009, contracts/system-contract.md, R-009)
- [X] T015 [US2] Define it in `source/counters/system.cpp` as a pure tree walk: resolve `m_impl->find("machine")`, scan that node's leaves, and return `counter<dim<0, 1>> {.leaf = leaf.core}` on a name match. Two recoverable-error branches and no contract macro in the body, since a host without a registered provider and a host without the instruction are both states a legal program may meet. A null or leafless machine node returns the error naming the absent provider (FR-007); a machine holding leaves with no `tsc` returns the error naming the absent counter (FR-008). Each message follows the house shape `register_provider` already uses: a lowercase opening, the failing condition named, a rationale clause, and the requirement id in parentheses. The body never calls `ensure_open()`, so a program may call it and then register (FR-004, FR-005, FR-006, FR-007, FR-008, R-004, R-006)
- [X] T016 [US2] Run `ctest --preset=dev -R counters_tsc` GREEN and `cmake --build build/dev -t dbc-gate` clean, with the interface count risen from 135 to 136 and 0 gaps, which is the check that the documented contract and the enforcement sites agree (FR-009, US2 checkpoint)

**Checkpoint**: a program reaches the counter by one call, feeds it into an expression beside a counted source, and the registration that follows still succeeds.

## Phase 3: User Story 3 - Time the library's own sampling path from inside the library (Priority: P3)

**Goal**: the overhead benchmark brackets its sampling action with the library's own counter, closing the artifact whose use of the standard library's clock exposed the withheld entry, and the measured budget reaches the counters overhead page.

**Independent Test**: `ctest --preset=dev -R counters_overhead` - the benchmark brackets a sampling action with the published entry and reports figures consistent with the previous standard-library-bracketed run.

- [X] T017 [US3] In `test/source/counters_overhead.cpp`, bracket a sampling action with the published time-stamp entry and report the tick delta, replacing the standard-library clock the withheld entry had forced the benchmark to reach for. The existing fold-cost and per-plan sections keep their own bracket, so the two costs stay separable (SC-005, Principle VII, US3 scenario 1)
- [X] T018 [US3] In `docs/pages/counters-overhead.md`, publish the release-preset read budget with its measured distribution and the raw-count disclosure: the entry publishes wherever the build executes the instruction, its frequency field carries the zero default, and no rate is attached to the count. The page's statement that the clock provider omits the entry on a host publishing no frequency is replaced, because that is the defect this feature removes (SC-003, SC-004, FR-001, FR-002, Principle IV)

**Checkpoint**: the overhead benchmark brackets its own sampling action with the library's counter and reports figures consistent with its prior run.

## Phase 4: Gates

**Purpose**: the full gate matrix, run in the order the plan's Test Plan sets out, because a pairing or coverage failure means a code change means a re-run (Principle VIII).

- [X] T019 [P] Run `cmake --preset=dev && cmake --build --preset=dev && ctest --preset=dev`, with the test target rebuilt as part of the build step: 39 of 39 targets pass
- [X] T020 [P] Run `cmake --build build/dev -t dbc-gate`, which needs `doxygen` on `PATH`: 136 interfaces with 0 gaps, up from 135
- [X] T021 [P] Run `python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/008-timestamp-counter`, which exits 0 with 0 findings. The `--mode tree` flag is mandatory: the default range mode reads only committed content and reports a green that the working tree does not carry. The whole-tree form `--check all --mode tree` exits 1 on 108 findings, and every one of them sits in 007 and earlier, with none in this feature; that red is the expected state of the tree, and the next run reads it as a decision rather than a regression (T027)
- [X] T022 Run `cmake --preset=ci-ubuntu && cmake --build build`, the release-preset build Principle IX makes mandatory for a public API change. It runs alone, with no other build or measurement in flight, so the budget measurement in T017 and T018 does not contend for the machine. The log's warning count is read against the pre-change baseline, because clang-tidy and cppcheck report without failing the build
- [X] T023 [P] Run `cmake --preset=ci-sanitize && cmake --build build/sanitize`, then `ctest` with the CI `ASAN_OPTIONS` block carrying `detect_leaks=1` and `halt_on_error=1` and the CI `UBSAN_OPTIONS` block
- [X] T024 [P] Run `test/counters_header_purity.sh` and `test/counters_push_atomic_scan.sh`, both clean (FR-010)
- [X] T025 [P] Run `cmake -P cmake/prose-lint.cmake` over the branch range, with the commit-message check, exit 0 (Principle XI)

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 0)**: no dependencies, it reads the build
- **US1 (Phase 1)**: depends on Setup, and blocks US2 because the accessor composes against the published entry
- **US2 (Phase 2)**: depends on US1, and rides its catalog
- **US3 (Phase 3)**: depends on US1, since the entry it brackets with has to be published
- **Gates (Phase 4)**: depends on all three stories

### Within Each User Story

- Tests written FIRST and verified RED (the plan records TDD mode)
- The provider edit precedes the existing-test repair, because the repair rewrites assertions about behaviour the provider edit removes
- The catalog seed and the reader condition move together, so the entry never enumerates without opening
- Story complete before moving to the next priority

### Parallel Opportunities

- T019, T020, T021, T023, T024, T025 are mutually parallel (distinct gates, distinct tools)
- T022 is serial: the release build runs alone so the measurement does not contend

---

## Corrections (Principle X.1)

Principle X.1 requires each candidate reading of a requirement to be recorded with the chosen one justified. The feature was designed three times and the owner corrected it twice. Both corrections are recorded here because each rejected design names the defect the shipped design fixes.

### Design one, rejected

The first design proposed a reading value type carrying the raw count, its unit, its source, a calibrated flag, the frequency provenance, and a counter-invariance flag. Beside it the design proposed a span type, a difference operator, a nanosecond conversion, and a defaulted virtual on `provider_iface` returning a clock calibration, so `system` could reach the clock provider without a downcast.

The owner rejected it: a time-stamp counter is a counter, it must have the same interface as every other counter to be used interchangeably, and the catalog entry plus FR-019 of 007 already carry the disclosure.

The reading failed on three counts, each a fact about the library as it already stands. FR-034 of 007 makes the calibration provenance catalog data, so a reading restated the catalog in a second place. FR-019 of 007 already attaches the value, the running ratio, and the scaled flag to every fold, so a reading restated the library's own disclosure model and two copies of one model drift apart. A counter carrying a bespoke interface stops being interchangeable with the others, and interchangeability is the property the owner asked for.

### Design two, rejected

The second design revised the first and kept the parallel types, adding a `system` accessor beside them.

The owner rejected it twice: anything dealing with frequency is out of scope, this change counts and does not associate time with the counter, and the read is raw with nothing to calibrate.

Three consequences follow, and they are the shape of the shipped design. The calibration is withdrawn outright, with FR-011 recording the withdrawal as superseding the calibration and publication obligations of FR-034 of 007 for this counter. The invariance reporting goes with it, because invariance interprets a count: it answers whether a difference spanning a thread migration is sound, which is a question about time, and the second correction put time out of scope. The span type, the difference operator, and the nanosecond conversion go too, each existing only to turn a count into a duration, and the duration is what the owner removed.

### What survived from both

One short spelling for the entry, `system::local().tsc()`, returning the library's counter type. The owner's phrase "the read is a counter" means the call hands back what the library already carries, with no new type between the caller and the count.

---

## Lessons

Two findings cost real time during the feature. Both are recorded so the next change in this tree starts with them in hand.

### A green `ctest` run can be a stale test binary

`ctest` reported 100% green against a stale test binary, because the library had been rebuilt while the test executable had not. The prediction that publishing the entry everywhere would break `test/source/counters_clock_push_test.cpp` surfaced only after an explicit `--target` rebuild of that executable, and at that point `std::stoull` on an empty `tsc_khz` string aborted the process.

The rule that follows: rebuild the specific test target, or confirm the binary is current, before trusting a green run. T012 carries the rebuild for that reason.

### `-Werror=float-equal` makes `==` unavailable on an exactness check

The build enables `-Werror=float-equal`, so an exactness assertion on a folded double needs the bit-compare helper `same_double()` that 007 already uses in `test/source/counters_core_test.cpp`. A plain `==` against `1.0` fails the build, which is why `test/source/counters_tsc_test.cpp` carries its own copy of the helper.

---

## Verified final state

- `ctest --preset=dev`: 40 of 40 targets pass, the fortieth being `counters_tsc_read_shape` (T026)
- `dbc-gate`: 136 interfaces with 0 gaps, up from 135 before the accessor
- `test/counters_header_purity.sh` and `test/counters_push_atomic_scan.sh`: both clean
- `test/counters_tsc_read_shape.sh`: the optimized read arm holds no call under g++ and clang++ at `SG_CONTRACTS_SEMANTIC` 0 and 2, 15, 25, 4, and 25 instructions respectively, with the negative probe live (T026)
- `python3 tools/prose/prose_gate.py --check prose --mode tree` over `specs/008-timestamp-counter`: 0 findings
- Observed on the development host, which executes the instruction and publishes no counter frequency: the entry reports `read_mode::fast_tsc`, `unit::none`, `frequency_hz` 0, `scaled` false, and the description `raw time-stamp counter ticks; a count asserting no rate`
- The accessor and the uniform lookup both name `tsc` and agree on name and description
- A counted source divided by the entry folds to a positive instructions-per-tick ratio with `running_ratio` 1.0 and `scaled` false
- Bare-read baseline in the release preset, 1000 sampling actions per repeat over 64 repeats at a measured 4.300 GHz TSC: a hand-written `rdtsc` pair costs 28 ticks at the median and the library's sampling path costs 29, so the library adds one tick and 0.2 ns per action (T028)
- The baseline ratio is published and stays unasserted, because the same library costs 29 ticks optimized and 151 unoptimized; a threshold narrow enough to carry the claim would fail on half the presets, so `counters_overhead` asserts only that both measurements are live and ordered (T028, SC-004, Principle VII)

## Phase 5: Convergence

**Purpose**: the four gaps below were found by `/speckit.converge` on
2026-09-29, after T001..T025 landed. Three are traceability defects and one
is a prose reconciliation. None is a functional gap: the 11 functional
requirements, the 6 success criteria, the 3 stories, and the 4 edge cases
are implemented and covered, and every gate in the plan's Test Plan passes.

- [X] T026 Give the raw-read codegen gate a real task identity and correct the citations that name a foreign one, per FR-003 and the plan's Test Plan gate sequence (contradicts). `test/CMakeLists.txt:113` and `test/counters_tsc_read_shape.sh:3` cite `(T026; 008 FR-003, FR-022)`. This feature's task list ends at T025, and T026 in `specs/007-counters-and-timers/tasks.md:89` is the `sample()` core loop, so the reference sends a reader into another feature. Record the gate in this list, then point both citations at the recorded id. The gate itself is sound: it compiles `source/counters/clock_provider.cpp` at `-O2` because the property holds in optimized code only, checks both contract semantics, and carries a negative probe so a clean run cannot come from an extractor that reads nothing. Its scope is a codegen property the artifacts never asked for and SC-006's gate list does not name, so recording it also states why it belongs (FR-003, FR-022)

- [X] T027 Reconcile T021's recorded command with the command the final state verified, per T021 (partial). `tasks.md:95` records `python3 tools/prose/prose_gate.py --check all --mode tree`, which exits 1 today on 108 findings, every one of them pre-existing in 007-and-earlier files and none in this feature's. `tasks.md:174` records the narrower `--check prose --mode tree` scoped to `specs/008-timestamp-counter`, which exits 0. A reader who follows T021 sees a red that the final state calls clean. State the scoped command in T021 and record that the whole-tree run is expected to exit 1 on the pre-existing set, so the next run reads as a decision rather than a regression (Principle XI)

- [X] T028 Record the bare-read baseline measurement in the task list, per SC-004 (unrequested). `test/source/counters_overhead.cpp` now measures a hand-written `rdtsc` pair beside the library's sampling path, both bracketed identically and divided by the action count, and `docs/pages/counters-overhead.md` carries the resulting table. T017 and T018 cover bracketing a sampling action with the entry and publishing the read budget, and both predate this work. The release preset measures 28 ticks bare against 29 through the library. The ratio is reported and not asserted, because the same library costs 29 ticks optimized and 151 unoptimized, so no threshold survives both presets. Add a task recording what the baseline measures and why the ratio stays unasserted (SC-004, Principle VII)

- [X] T029 Reconcile the two tick figures the overhead page gives for the same read, per SC-004 (partial). `docs/pages/counters-overhead.md:318` states a 1-tick minimum against a 42-tick median, and the table at `:344` gives 28 and 29 ticks for the same read. Both are correct: line 318 is an isolated pair measured cold, the table amortizes a thousand actions per repeat. The new subsection already reconciles the 20 ns row above it against the table and does not reconcile this one, and line 318 carries no pointer. Name the steady-state figure at line 318 or cross-reference the subsection, so a reader who meets the 42 first is not left holding two numbers (SC-004, Principle IV)
