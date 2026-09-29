# Tasks: Standalone Counters Library

**Feature**: `007-counters-and-timers` | **Branch**: `007-counters-and-timers`

**Input**: Design documents from `specs/007-counters-and-timers/`

**Prerequisites**: plan.md (required), spec.md (required for user stories), research.md (R-001..R-015), data-model.md (E-01..E-11), contracts/ (system, provider, measurement), quickstart.md (§1..§13)

**Tests**: INCLUDED. The plan's Test Plan records **TDD mode** (Principle III): every test task below is written against the headers/fixtures first, verified RED (or failing), and only then does its implementation task turn it GREEN. Hand-computed expected values are committed with the fake-provider fixtures.

**Organization**: Tasks grouped by user story (US1..US8 per spec.md priorities). Namespace `sg::counters` throughout (R-001). Frameworkless test convention: hand-rolled `check()`/`fail()` executables, plain `add_test` in `test/CMakeLists.txt` (no test framework may be added - the dependency-scan test forbids it).

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies on incomplete tasks)
- **[Story]**: US1..US8; absent on Setup, Foundational, and Polish tasks
- Every task names its exact file path

## Path Conventions

Per plan.md Project Structure: public surface flat in `include/speedgun-ng/counters*.hpp`; implementation in `source/counters/` attached via root `CMakeLists.txt` `target_sources PRIVATE`; vendored data in `external/pmu-events/`; tests flat in `test/source/counters_*.cpp`; examples in `example/`; tool in `tools/pmu_events/`.

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Pre-flight verifications and build wiring. No public behavior yet.

- [X] T001 Verify `std::expected` availability on the dev-preset toolchains (GCC and Clang); if unavailable, add a minimal in-house expected in `source/counters/detail/expected.hpp` with written justification per R-002 and the plan Complexity Tracking registry (the public type interface is fixed either way)
- [X] T002 Run the licensing confirmation pass for the kernel `tools/perf/pmu-events` data (dual MIT/GPL-2.0-or-later) against BSD-3 redistribution; record the verdict in `specs/007-counters-and-timers/plan.md`; if adverse, activate the R-012 fallback (sysfs-discovered catalog only, identical interface) and annotate T044/T045/T057..T061 as fallback-adjusted (R-012, spec Assumptions)
- [X] T003 Create `source/counters/`, `source/counters/detail/`, `source/counters/linux_pmu/` and wire a `target_sources(speedgun-ng_speedgun-ng PRIVATE ...)` bracket for `source/counters/**` into `CMakeLists.txt` following the existing gate-bracket pattern; verify the `cmake/lint.cmake` format glob reaches `source/counters/` and extend it if not (Principle V)
- [X] T004 Create the public header skeletons with doxygen pre/post/invariant contract blocks (dbc-gate pairs documentation with enforcement; bodies may be stubs until their story lands): `include/speedgun-ng/counters.hpp` (the umbrella carries a file-level `@file` brief; it declares no interface, so no contract block applies to it), `counters_core.hpp`, `counters_provider.hpp`, `counters_system.hpp`, `counters_measurement.hpp`, `counters_fake.hpp`, `counters_clock.hpp`, `counters_push.hpp`, `counters_pmu.hpp` (R-001, Constitution II)

**Checkpoint**: `cmake --preset=dev && cmake --build --preset=dev` green with empty wiring; dbc-gate sees the headers.

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core vocabulary every user story rides on: dimensions, units, catalog/availability/metric-result shapes, provider contract, error type.

**CRITICAL**: No user story work can begin until this phase is complete.

- [X] T005 Implement `template <int T, int C> struct dim` with constexpr exponent arithmetic helpers (identical-tag check, quotient subtraction) in `include/speedgun-ng/counters_core.hpp` (R-003, FR-015/016)
- [X] T006 Implement the `unit` enumeration and the closed `unit -> std::expected<dimension, error>` switch in `include/speedgun-ng/counters_core.hpp`: recognized units map (`seconds` to `time^1`, count units to `events^1` per contracts/measurement-contract.md); the error naming the unrecognized unit comes from the `unit_from_token` if-chain, which runs ahead of the switch (FR-017, US1 scenario 6)
- [X] T007 Implement `availability` enum (`countable`, `permission_blocked`, `not_encodable`, `absent`), `read_mode` enum (`fast_tsc`, `fast_rdpmc`, `syscall`, `push_load`), `catalog_entry` struct (name, description, unit, availability, mode), `metric_result` struct (value, running_ratio, scaled; disclosure structurally impossible to omit), and tier-2 `error` struct (message + suggestions vector) in `include/speedgun-ng/counters_core.hpp` (FR-005, FR-006, FR-019, FR-008 shape; E-03/E-09/E-10)
- [X] T008 Implement the provider seam in `include/speedgun-ng/counters_provider.hpp`: C++20 `provider` concept, `provider_iface` registration base (`enumerate(object_sink&) const`, `open(leaf_set, target)` returning a `window_reader`), `window_reader::read_points(point_sink&) noexcept`, per-leaf point yield shape with the unit and metadata as `catalog_seed` fields; virtual calls confined to setup (FR-011, R-004, contracts/provider-contract.md)
- [X] T009 Write `test/source/counters_core_test.cpp` covering dims, the closed unit switch (known units map; unknown unit errors naming the unit), `metric_result` field presence, `error` shape; register in `test/CMakeLists.txt` with plain `add_test` (TDD: RED against skeletons, GREEN after T005..T007)
- [X] T010 [P] Write the header vocabulary-purity scan test (terms `perf_event`, `clock_gettime`, `rdpmc`, `rdtsc` and hardware/clock-syscall names absent from `include/speedgun-ng/counters*`; violations named; exit 0 clean) as a fixture script under `test/`; register in `test/CMakeLists.txt` (FR-010, C-SYS-6)
- [X] T011 Write `include/speedgun-ng/counters.hpp` umbrella (documented single include; deliberately NOT folded into `speedgun-ng.hpp`, keeping the FR-049 standalone claim visible in the include graph); run `cmake --build build/dev -t dbc-gate` and confirm contract/doc pairing passes on the new headers

**Checkpoint**: Foundation ready - user story implementation can now begin. `ctest -R "counters_core|counters_header_purity"` green.

## Phase 3: User Story 1 - Compose and read a derived metric in one measurement window (Priority: P1) MVP

**Goal**: Resolve named counters through a minimal system (machine root + fake provider), compose them under the typed algebra (`instructions / cycles`), measure one window through the scope spelling, and fold a metric with ratio disclosure and full raw provenance - all against hand-driven fake points.

**Independent Test**: `ctest --preset=dev -R "counters_fake|counters_compile_fail"` - a program composes an expression over fake-provider counters, measures with `start`/`finish`, and checks the folded value, ratio fields, and raw provenance against hand-driven point sequences; all assertions exact; negative-dimension cases fail compile (quickstart 3 and 4).

### Tests for User Story 1 (TDD - write FIRST, verify RED)

- [X] T012 [P] [US1] Write negative-compile cases into the existing harness at `test/compile-fail/`: `bytes + monotonic` addition, mismatched-tag subtraction, `time^1 + events^1` addition must FAIL to compile; positive cases (quotient `instructions/cycles` yields `dim<0,0>`, scalar scale preserves the tag) compile under `test/compile-fail/positive/`; register the suite as `counters_compile_fail` (FR-015, US1 scenario 3)
- [X] T013 [P] [US1] Write `test/source/counters_fake_test.cpp`: resolution carries name/description/unit-dimension/availability (scenario 1); one-wrong-character name fails with near-miss suggestions (scenario 2); `a/b` quotient tag algebra (scenario 3); scope `start`/work/`finish`/`.metric()` equals the hand-computed quotient with ratio 1.0 and scaled false for fake sources (scenario 4, FR-019); ten `.metric()` calls identical while a provider read counter stays frozen (scenario 5, FR-021); unrecognized catalog unit errors naming the unit (scenario 6); raw view of a composite leaf reports object path, name, description, unit, raw delta, ratio (scenario 7, FR-020). Hand-computed expected values committed with the test (SC-006, SC-008)

### Implementation for User Story 1

- [X] T014 [US1] Implement the shipped fake provider: control surface in `include/speedgun-ng/counters_fake.hpp` (per-leaf explicit point sequences or seeded deterministic per-sample delta generators; scripted crafted `2^64` walk supported) and `source/counters/fake_provider.cpp` (an ordinary provider through `provider_iface`; registers a machine-root object by default) (FR-036, R-009)
- [X] T015 [US1] Implement the system handle and minimal tree in `include/speedgun-ng/counters_system.hpp` + `source/counters/system.cpp`: `system::local()`, `register_provider` (pre-open only; register-after-open is a recoverable error), open boundary closing registration and freezing the catalog (FR-009), machine root object, path resolution, `object::counter(name)` resolution to a `resolved_leaf` carrying name/description/dimension/availability, near-miss did-you-mean diagnostics from catalog names and descriptions (FR-001..FR-005, FR-008; C-SYS-1/4/5)
- [X] T016 [US1] Implement `counter<D>` (leaf-slot index + compile-time tag) and `expression<D>` in `include/speedgun-ng/counters_measurement.hpp`: `operator+`/`operator-` require identical tags, `operator/` subtracts exponents, scalar multiply unrestricted, all static_assert'd; dimensions erased on the read path (FR-014..FR-016, E-05)
- [X] T017 [US1] Implement `compile(system, expressions...)` in `source/counters/plan.cpp`: flat leaf slot table (slot, provider read descriptor, point-column offset), fold program per composite (column references with algebraic exponents and ops), zero-leaf expression and empty plan are recoverable construction errors, an expression over a non-countable leaf fails with the catalog state in the message, zero hardware reads performed (FR-021/022, E-07; contracts/measurement-contract.md "Plan compile")
- [X] T018 [US1] Implement fold kernels in `source/counters/fold.cpp`: modular `point[j] - point[i]` at 2^64 so a single wrap subtracts out (FR-013), window fold `fold(rec, i, j)` and first-to-last fold with tier-3 `SG_REQUIRE` range check (`i < j` within recorded extent) (FR-018), ratio disclosure per FR-019 (sources without an enabled/running pair disclose ratio 1.0 scaled false; composite ratio = product of constituent ratios each raised to its algebraic exponent), folds pure, repeatable, provider-read-free (FR-021)
- [X] T019 [US1] Implement scope sugar in `include/speedgun-ng/counters_measurement.hpp` (+ `source/counters/plan.cpp` as needed): `scope::start()`/`finish()`/`metric(expr)` over a two-point buffer living in the scope object, semantics identical to the recorder (FR-030); the misuse sequences the type refuses are tier-3 `SG_REQUIRE` violations: `metric` on a window that is not closed, `finish` without `start` including a second `finish`, and a second `start` (FR-046, spec edge cases as T155 amended them); a finished scope is a settled window, so a `metric` after `finish` folds the same two points, and registering a composite into a started scope has no spelling because a composite reaches a window through the plan compiled before that window opens
- [X] T020 [US1] Implement `expression::raw(object_path, leaf)` returning a `points_view` exposing object path (canonical spelling), name, description, unit, raw point column, point identity, multiplex ratio per E-10 (FR-020) in `include/speedgun-ng/counters_measurement.hpp` + `source/counters/fold.cpp`
- [X] T021 [US1] Register `counters_fake_test` and the compile-fail suite in `test/CMakeLists.txt` (plain `add_test`), run `ctest --preset=dev -R "counters_fake|counters_compile_fail"` GREEN, confirm the quickstart 3/4 verdicts for the US1 slice

**Checkpoint**: US1 fully functional and independently testable: fake-provider metric fold with disclosure and provenance end to end, zero tolerance.

## Phase 4: User Story 2 - Sample points in a hot loop and fold them later (Priority: P1)

**Goal**: Compiled plan + fixed-capacity recorder; `rec.sample()` appends one column of cumulative points with zero allocation, zero lock, no fold; window/pair/first-to-last folds; compile-time overflow policy (`hard_stop` default with always-enforced bounds, `ring` opt-in branchless drop-accounted); 2^64 wraps subtract out.

**Independent Test**: `ctest --preset=dev -R "counters_recorder|counters_trap|counters_noalloc"` - fake provider driven through a known point sequence including a crafted 2^64 wrap; window folds, pair folds, wrapped-ring folds, and drop accounting exactly hand-computed; the allocation-counting test proves zero allocation in `sample()` (quickstart 3, 5, 8).

### Tests for User Story 2 (TDD - write FIRST, verify RED)

- [X] T022 [P] [US2] Write `test/source/counters_recorder_test.cpp`: construction with capacity N allocates all N columns, later sampling allocates nothing (scenario 1); `hard_stop` sample past capacity aborts, verified out-of-process via the trap pair (scenario 2); `ring` power-of-two capacity masks branchlessly, records wrapped + dropped, folds consistent with the retained window; a non-power-of-two ring capacity fails construction recoverably (scenario 3); a crafted 2^64 wrap on a PMU-style leaf folds to the true count (scenario 4, SC-006); `fold_pairs` yields one metric per adjacent interval (scenario 5); `i >= j` or out-of-extent fold is a contract violation via trap fixture (scenario 6); two recorders of one plan get independent buffers sharing the compiled layout (scenario 7)
- [X] T023 [P] [US2] Write the cross-process trap pair following the `dbc_trap_fixture`/`dbc_trap_checked_test` precedent: `test/source/counters_trap_fixture.cpp` (aborting fixture, registered indirectly) and `test/source/counters_trap_checked_test.cpp` (checker spawning the fixture; absence-of-marker assertions) covering scope misuse (`metric` before `finish`), `hard_stop` capacity overrun ALSO in a release-configured checker build (contracts `ignore` semantics; FR-027 memory safety never semantic-gated), and invalid fold range; register both in `test/CMakeLists.txt` (FR-027, FR-046, US2 scenarios 2/6; quickstart 5)

### Implementation for User Story 2

- [X] T024 [US2] Implement the recorder factory and handle in `include/speedgun-ng/counters_measurement.hpp`: `hard_stop_t`/`ring_t` constexpr tag objects, `plan::recorder(capacity)` default and `plan::recorder(capacity, ring)` with CTAD and no visible template arguments at call sites (FR-025), `recorder_handle<P>` trivially-copyable value handle (buffer pointer, head index, policy state) (FR-029, E-08, R-006); `sample()` declared `noexcept`
- [X] T025 [US2] Extend `source/counters/plan.cpp`: arena geometry - one `uint64` array of `capacity` slots per leaf column, column-major SoA, allocated inside `plan::recorder` at `source/counters/plan.cpp:223` and `:239`, one arena per call, which is recorder construction, the boundary FR-029 names (untimed region); independent buffers per recorder sharing the compiled layout (FR-029, R-005); `hard_stop` bounds check as `SG_REQUIRE_ALWAYS` (present in every configuration including release) beside the write index; ring: power-of-two capacity validated at construction as a tier-2 `std::expected` error, `idx & (cap - 1)` branchless update, one-shot wrapped promotion, dropped count incremented per overwrite (FR-027/028, R-006)
- [X] T026 [US2] Implement the `sample()` core loop (flat array of read descriptors: push plain loads, clock vDSO calls, provider `read_points` for window groups; zero virtual calls, zero name lookup, zero allocation; one column appended per call) in `include/speedgun-ng/counters_measurement.hpp`/`source/counters/plan.cpp` (FR-026, FR-047 per-column `SG_INVARIANT` one-sampling-action); extend `source/counters/fold.cpp`: `fold_pairs(rec)` per-interval series, wrapped-ring folds consulting `dropped` inside the fold layer (FR-028), capacity-1 fold impossibility tier-3 (spec edge case); the push-monotonicity fold check lands with the push provider in T037 (FR-035)
- [X] T027 [P] [US2] Write `test/source/counters_noalloc_test.cpp`: counting global `operator new`/`delete` installed in the TU; assert `sample()` over a filled recorder records zero allocations (SC-005); register in `test/CMakeLists.txt`
- [X] T028 [US2] Register `counters_recorder_test` in `test/CMakeLists.txt`, run `ctest --preset=dev -R "counters_recorder|counters_trap|counters_noalloc"` GREEN (quickstart 3, 5, 8 verdicts)

**Checkpoint**: US1 AND US2 both work independently: tight-loop sampling idiom with exact wrap/drop accounting and the zero-allocation proof.

## Phase 5: User Story 3 - Counters belong to named things; measure across the machine (Priority: P2)

**Goal**: Full object tree (packages, cores, uncore), canonical structured paths with platform-alias resolution, structural selection, cross-object composition under one shared window, fan-out registration (one expression across many objects, one group read per instance).

**Independent Test**: `ctest --preset=dev -R counters_objects` - fake provider builds a two-package multi-core tree; selection, alias resolution, cross-object fold, fan-out reconciliation, duplicate-name errors, all exact (quickstart 9).

### Tests for User Story 3 (TDD - write FIRST, verify RED)

- [X] T029 [P] [US3] Write `test/source/counters_objects_test.cpp`: enumeration reports kind, canonical path, description, parent, alias, own catalog (scenario 1); `package-1/core-3` and its platform alias resolve to the same object and all output prints the canonical spelling (scenario 2); `objects(kind=core, package=1)` returns exactly the matches (scenario 3); an expression mixing `uncore_imc_0` and machine leaves measured in one scope reads both within one sampling action and folds (scenario 4); one IPC expression registered across every core: per-core results, per-core instruction deltas reconcile against the shared total (scenario 5, SC-007); duplicate canonical path under one parent and duplicate counter name within one object fail registration recoverably with the tree unchanged (scenario 6); register in `test/CMakeLists.txt`

### Implementation for User Story 3

- [X] T030 [US3] Extend `source/counters/system.cpp` + `include/speedgun-ng/counters_system.hpp`: multi-object tree from provider enumeration (kind, structured path, optional platform alias, description, parent link, children range), duplicate path-under-parent and duplicate-counter-in-object detection as recoverable errors leaving the tree unchanged (FR-001, FR-004, FR-008; E-02)
- [X] T031 [US3] Implement alias resolution (canonical path and alias hit the same object; canonical spelling in every API result, provenance line, and diagnostic) and `objects(kind, attribute filters)` selection returning exactly the matches, unknown kind a recoverable error (FR-002, FR-003, FR-008; C-SYS-1/2) in `source/counters/system.cpp`
- [X] T032 [US3] Extend `source/counters/plan.cpp`: cross-object leaf mixing in one plan/window (all leaves read within one sampling action regardless of owning object; US3 scenario 4); fan-out registration - one expression registered per matching object, plan emits per-object metric results, per-instance provider group reads (US3 scenario 5)
- [X] T033 [US3] Run `ctest --preset=dev -R counters_objects` GREEN (quickstart 9 verdict)

**Checkpoint**: Tree, selection, cross-object composition, and fan-out work on fake trees; stories 1-2 unchanged.

## Phase 6: User Story 4 - Measure real time and user events with no privileges (Priority: P2)

**Goal**: Shipped `clock` provider (monotonic, thread CPU, process CPU; `time^1`; zero privileges) with the calibrated fast `tsc` leaf, and shipped `push` provider (thread-confined, plain non-atomic `add(n)`, plain load sample); composites like `bytes / monotonic` mix them freely.

**Independent Test**: `ctest --preset=dev -R "counters_clock|counters_push"` - tolerance-band comparisons over sleep-free CPU-bound windows, exact push totals, a byte-rate composite over fake clock columns plus real push counts, catalog mode disclosure (quickstart 7).

### Tests for User Story 4 (TDD - write FIRST, verify RED)

- [X] T034 [US4] Write `test/source/counters_clock_push_test.cpp`: monotonic/thread-CPU/process-CPU deltas positive and within calibration tolerance of each other on CPU-bound work (scenario 1); `add(1000)` between samples folds to exactly 1000 with the increment and sample-read plain non-atomic (scenario 2); `bytes / monotonic` with push bytes folds to the byte rate with standard disclosure (scenario 3); machine catalog lists clock leaves and push counters `countable` with descriptions and achieved read mode (scenario 4); on a calibrated platform the `tsc` leaf reports frequency provenance and the scaled flag where the platform sets it (scenario 5); cross-thread `add()`/sample and the fold-time push decrement (US4 scenario 6) route to the tier-3 traps (extend `test/source/counters_trap_fixture.cpp` from T023); register in `test/CMakeLists.txt`

### Implementation for User Story 4

- [X] T035 [US4] Implement `source/counters/clock_provider.cpp`: machine-object leaves `monotonic` (`clock_gettime(CLOCK_MONOTONIC)`), `thread_cpu` (`CLOCK_THREAD_CPUTIME_ID`), `process_cpu` (`CLOCK_PROCESS_CPUTIME_ID`), all `time^1`, mode `syscall` (vDSO regime), zero privileges (FR-033, R-007)
- [X] T036 [US4] Add the `tsc` leaf in `source/counters/clock_provider.cpp`: `__rdtsc` (or `rdtscp` variant per spectre posture), frequency calibrated at provider construction from `tsc_khz` sysfs cross-checked against CPUID frequency-invariance data, the last boundary at which the leaf can be seeded because registration is refused after open and the catalog freezes there, achieved mode `fast_tsc` plus calibration provenance and the scaled-TSC flag as catalog fields, platforms without a usable TSC omit the leaf (catalog fact, zero API difference); P2 intrinsic justification written at the site (FR-034, R-007; plan Complexity Tracking)
- [X] T037 [US4] Implement `source/counters/push_provider.cpp` + the push counter handle in `include/speedgun-ng/counters_measurement.hpp`: creating thread recorded; `add(n)` plain `uint64 += n`, sample-time read a plain load, no atomic RMW on any path; cross-thread `add()`/sampling a tier-3 `SG_REQUIRE` (semantic-gated, compiled out in release); a decrement between recorded points detected at fold time as tier-3 (FR-035, R-008)
- [X] T038 [US4] Register `counters_clock_push_test` in `test/CMakeLists.txt`, run `ctest --preset=dev -R "counters_clock|counters_push"` GREEN (quickstart 7 verdict)

**Checkpoint**: An unprivileged process measures wall time, CPU time, and user events; composites fold with full disclosure.

## Phase 7: User Story 5 - Extend the system with a provider the library has never seen (Priority: P2)

**Goal**: Out-of-tree providers are first-class: the giraffe (`menagerie/giraffe-2` counting honks) ships as roughly 100 lines against public headers only; `honks / monotonic` folds to a honk rate from one shared sampling action; the standalone example proves the FR-049 link manifest.

**Independent Test**: build and run both examples (exit 0), `ctest -R counters_provider_ext`; the giraffe source touches no `source/` include; `git status --porcelain source include` clean after building them (quickstart 2 and 6).

### Tests for User Story 5 (TDD - write FIRST, verify RED)

- [X] T039 [P] [US5] Write `test/source/counters_provider_ext_test.cpp`: a mini out-of-tree provider defined inside the test TU (public headers only) registers objects/counters, appears in enumeration with descriptions and availability, measures through a scope, and folds an honks-per-time composite to a rate; a described-but-unavailable entry is branchable on catalog state alone (FR-012, US5 scenarios 1-4); register in `test/CMakeLists.txt`

### Implementation for User Story 5

- [X] T040 [P] [US5] Write `example/counters_giraffe_example.cpp` (roughly 100 documented lines): giraffe provider attached to the system, `menagerie/giraffe-2` object with a `honks` counter, a scope measuring `honks / monotonic` to a honk rate from one shared sampling action; public headers plus standard library only; add to `example/CMakeLists.txt` via `add_example()` (US5, SC-003)
- [X] T041 [P] [US5] Write `example/counters_standalone_example.cpp`: public headers + std only, composes an `instructions/cycles`-shape metric over fake+clock sources and drives it in a fixed-iteration per-thread loop with recorder capacity computed from the known iteration count and fold results feeding per-iteration counter inputs (FR-050), runs, prints the folded metric line with ratio and scaled fields, exit 0; add to `example/CMakeLists.txt`; verify the link manifest (`ldd`/`readelf -d`) demonstrates no third-party dynamic dependency and names the platform C and C++ runtime, and grep finds zero third-party includes (SC-001; quickstart 2)
- [X] T042 [US5] Stabilize the provider surface so T039-T041 compile against installed public headers with zero internal access; confirm the giraffe source includes no `source/` header; run `ctest --preset=dev -R counters_provider_ext` and both example targets GREEN; `git status --porcelain source include` clean after the example builds (SC-003)

**Checkpoint**: The provider abstraction is proven from outside the library; the standalone claim is verified with a link manifest.

## Phase 8: User Story 6 - Count hardware events on Linux from a rich catalog (Priority: P3)

**Goal**: `linux_pmu` provider: vendored per-CPU event tables merged with kernel-discovered aliases (kernel wins), CPUID table selection, encoding composed with sysfs `format/` bit layouts, availability probed per entry (`permission_blocked` at paranoid 2), one group read per PMU leader per action, enabled/running as ordinary leaves.

**Independent Test**: `ctest --preset=dev -R counters_pmu` green unprivileged at `perf_event_paranoid=2` (hardware entries `permission_blocked`, clocks/push `countable`); privileged-host developer evidence: named Intel/AMD events described, a group (instructions+cycles) IPC equals the raw-column quotient (quickstart 10).

### Tests for User Story 6 (TDD - write FIRST, verify RED)

- [X] T043 [P] [US6] Write `test/source/counters_pmu_test.cpp`: merge semantics with kernel-wins conflicts and every entry described (scenario 1); CPUID vendor/family/model selects the matching architecture directory, parsed once lazily (scenario 2); an event whose fields exist in neither the vendored table nor kernel format reports `not_encodable`, no partial encoding (scenario 3); at paranoid 2 unprivileged: hardware entries `permission_blocked`, clocks and push `countable`, suite green (scenario 4, SC-002); a group of resolved events samples in syscall mode: one group read per PMU leader delivers members + enabled/running in one action (scenario 5); a multiplexed group: ratio below 1, scaled set, value is the kernel scaled estimate (scenario 6, developer-privileged evidence); compiling over a leaf the catalog reports as not `countable` fails recoverably with the leaf address and the catalog state named, and compiling over a window the provider refuses to open fails recoverably with one message per read group that names no leaf, no provider, and no kernel reason, both before any hardware read; a plan binds one target, so a target or clock-id mismatch across group members has no spelling (scenario 7, FR-024 single-plan-target design); register in `test/CMakeLists.txt`

### Implementation for User Story 6

- [X] T044 [US6] Create the vendored data tree `external/pmu-events/` (byte-exact path snapshot of kernel `tools/perf/pmu-events` x86 architecture table directories plus `mapfile.csv` at the pinned revision) with the `RECORD` provenance manifest (ref, date, URL, per-file sha256, exclusion list), and add the configure-time gate bracket to `CMakeLists.txt` (recorded gate constant vs `RECORD` ref; configuration fails on drift; yaml-cpp precedent, no git metadata consulted) (FR-043, R-012)
- [X] T045 [US6] Implement `source/counters/linux_pmu/table_parse.cpp`: read CPUID, match `mapfile.csv` regex (first match wins) to the architecture directory, parse that directory's JSON once lazily with vendored simdjson (private seam: these TUs are the sole simdjson includers under `source/counters/`, preserving the 004 privacy-audit boundary) (FR-037/038, R-010)
- [X] T046 [US6] Implement `source/counters/linux_pmu/encode.cpp`: compose JSON semantic `EventCode`/`UMask` with the running kernel's sysfs `format/<field>` bit positions; a required format field missing from the kernel marks the entry `not_encodable`, encoding never half-attempted (FR-037, R-010)
- [X] T047 [US6] Implement `source/counters/linux_pmu/provider.cpp`: enumerate `/sys/bus/event_source/devices/*` (type ids, `format/`, `events/` aliases), merge with vendored entries (kernel-discovered aliases win conflicts), probe availability per entry by `perf_event_open` test-open plus `perf_event_paranoid` read, report `countable`/`permission_blocked`/`not_encodable`/`absent`; Linux-only compile guard, absent cleanly on other platforms behind the identical interface with the reduced catalog (FR-037/039/042, R-010)
- [X] T048 [US6] Implement `source/counters/linux_pmu/group_io.cpp`: syscall-mode window reader - group fds, one `read(PERF_FORMAT_GROUP)` per PMU leader per sampling action delivering all member values plus `time_enabled`/`time_running` into scratch; enabled and running are ordinary cumulative leaves appended to every group's leaf set (FR-026 syscall mode, FR-041, R-010)
- [X] T049 [US6] Extend `source/counters/plan.cpp` group layout: bind one sampling target for the whole plan and open every read group and read mode against it, so a target or clock-identity mismatch across group members is unrepresentable, and make the two construction errors a single target still fails on recoverable before any hardware read, a leaf the catalog reports as not `countable` whose message names the leaf address and the catalog state, and a window a provider refuses to open whose one message names no leaf, no provider, and no kernel reason (FR-024 as T147 amended it, US6 scenario 7); bind thread/cpu targeting at plan open, plan is a per-thread object, multiple plans over one system first-class (FR-031)
- [X] T050 [US6] Run `ctest --preset=dev -R counters_pmu` GREEN on an unprivileged host at paranoid 2 (quickstart 10 verdict; privileged evidence recorded in the PR per the developer-machine protocol)

**Checkpoint**: Real hardware events on Linux from the rich catalog; the suite stays green at paranoid 2 via `permission_blocked` states.

## Phase 9: User Story 7 - Near-single-instruction reads where the platform permits (Priority: P3)

**Goal**: Per-leaf read mode from probe (`fast_tsc`, `fast_rdpmc`, `syscall`, `push_load`), the mapped-page rdpmc protocol, per-plan overhead calibration, and the published fast-vs-syscall regime benchmark.

**Independent Test**: `ctest --preset=dev -R counters_overhead` - on a fast-capable probe-passing host: fast plan tens-of-cycles vs the same plan forced `syscall` in microseconds, distributions min/median/max published side by side; other hosts skip with the failure named (quickstart 12).

### Tests for User Story 7 (TDD - write FIRST, verify RED)

- [X] T051 [P] [US7] Write `test/source/counters_overhead.cpp` as a CTest-registered benchmark executable (`SKIP_RETURN_CODE 2` for probe-gated fast sections; skip reason printed): clock-only plan `sample()` stated nanosecond distribution (min/median/max); fast-mode plan tens-of-cycles regime vs the same plan forced `syscall` microsecond regime published side by side (scenario 6, SC-004); sample-path cost tracks the read sequence with fold cost measured separately off the path (SC-010); achieved modes disclosed per catalog entry, fast where the probe passes / `syscall` where it fails with the reason (scenario 1); register in `test/CMakeLists.txt`

### Implementation for User Story 7

- [X] T052 [US7] Implement `source/counters/linux_pmu/fast_read.cpp`: per-thread `perf_event_open` context, single-page read-only `mmap`; read descriptor implementing the mapped-page protocol in order - seqcount `lock` snapshot + retry, `rmb` fences, `cap_user_rdpmc` capability gate, one-based `index` validity with the stated fallback path when not allowed, `_rdpmc(index - 1)`, kernel offset adjustment, the counter width `pmc_width` publishes on that page as the mask, per-thread same-thread context binding (FR-040, R-011, US7 scenario 2); a standard `static_cast` of the mapping base to the kernel's own `perf_event_mmap_page` from `<linux/perf_event.h>`, which carries no P2 exception + attribution comment citing `jevents/rdpmc.{c,h}` from andikleen/pmu-tools (no file copied) (plan Complexity Tracking)
- [X] T053 [US7] Implement per-leaf mode assignment in the providers' enumeration path, which runs before the catalog freezes at the open boundary and before `source/counters/plan.cpp:469` binds the plan target: probe mechanism availability, namely the `cap_user_rdpmc` capability bit the event page publishes, the one-based counter index, and the `pmc_width` the same page publishes - achieved mode recorded per leaf and disclosed in the catalog; support probed, never assumed (FR-023, R-011, US7 scenario 1)
- [X] T054 [US7] Implement per-plan overhead calibration in `source/counters/plan.cpp`: after finalization, repeatedly run the plan's `sample()` over an empty workload, store min/median/max ns on the plan, expose `sample_overhead_ns_min/median/max()`; fold windows state their endpoints' sample cost (FR-032, R-014)
- [X] T070 [P] [US7] Write the cross-thread trap test extending `test/source/counters_trap_fixture.cpp` and its checker: recorder and plan used from a non-binding thread abort in the dev/CI build (TDD: RED before T055 implements the binding check); register in `test/CMakeLists.txt` (FR-031, FR-047; C-MEA-6; plan Test Plan threading row)
- [X] T055 [US7] Cross-thread misuse contract (test T070 first): recorder/plan/push use from a non-binding thread is a tier-3 `SG_REQUIRE` beside the existing bounds check (cached `thread::id` compare); fast contexts bind same-thread (FR-031/040, US7 scenarios 4-5; R-015); fast-mode off-CPU staleness stays disclosed through the enabled/running ratio leaves in every mode (FR-041, US7 scenario 4)
- [X] T056 [US7] Publish `docs/pages/counters-overhead.md` (following the `docs/pages/dbc-overhead.md` precedent) with the reference-host distributions: fast and syscall regimes side by side, min/median/max (Principle VII, SC-004)

**Checkpoint**: The performance promise is measured and published; every host reports either its regimes or its skip reason.

## Phase 10: User Story 8 - Keep the vendored event tables current (Priority: P3)

**Goal**: First-class upgrade utility `tools/pmu_events/update_pmu_events.py` with `--to <ref>` fetch/strip/validate/replace/record/bump/digest and `--check` drift verification (no network), wired into CI with synthetic fixtures.

**Independent Test**: `python3 tools/pmu_events/update_pmu_events.py --check` exits 0 clean / 1 naming the drifted file; `ctest -R pmu_events_check` passes the synthetic fixture trees in both directions (quickstart 11).

### Tests for User Story 8 (TDD - fixtures FIRST, verify RED)

- [X] T057 [P] [US8] Create `test/pmu-events-gate-fixture/`: tiny synthetic table trees - one clean (check passes), one with a corrupted file hash (check exits 1 naming the file), one with a gate/RECORD disagreement (exits 1); wire both directions into CTest as `pmu_events_check` fixtures following the `prose_gate_fixtures` pattern in `test/CMakeLists.txt`; the trees are built at run time into `pmu-events-gate-fixtures/` under the build dir, and a reviewer reads the drift cases in the source tree as the seven numbered scenarios of `run_pmu_events_fixtures.py`, each carrying its expected exit code and the file it must name; every `RECORD` sha256 is computed by the generator at run time, so a checked-in tree would be a second source of truth (FR-045, US8 scenario 6; the `tools/prose/` precedent)

### Implementation for User Story 8

- [X] T058 [US8] Implement `tools/pmu_events/update_pmu_events.py --check`: verify every tree file's sha256 against `RECORD`, verify the gate constant (root `CMakeLists.txt` bracket) against the recorded ref, exit 1 naming the file on any drift, zero network access ever (FR-045, US8 scenarios 1-2/5, R-013)
- [X] T059 [US8] Implement `--to <kernel-ref>` mode in `tools/pmu_events/update_pmu_events.py`: fetch the `tools/perf/pmu-events` path archive from kernel.org cgit (documented GitHub mirror fallback), strip to the in-scope x86 table directories plus `mapfile.csv` recording the explicit exclusion list, validate every file parses (json) and passes schema sanity (`EventName`/`EventCode` present, mapfile regexes compile), replace byte-exact via a staging directory (replace-last: a failed validation leaves the tree untouched), rewrite `RECORD` (ref, date, URL, per-file sha256, exclusions), bump the gate constant (single bump point), print the per-architecture old-to-new event-count digest (FR-044, US8 scenarios 3-4, R-013)
- [X] T060 [US8] Add the CI step to `.github/workflows/ci.yml`: run `update_pmu_events.py --check` (no network) in the existing test job; counters tests ride every existing test job unchanged; no gate weakened (Principle VIII, FR-045, US8 scenario 5)
- [X] T061 [US8] Add the `pmu-events` re-pinning section to `README.md` beside the existing vendored-dep sections (what the gate is, how to re-pin via `--to`, that `--check` runs in CI) (FR-043/044 documentation, plan Project Structure)

**Checkpoint**: Table currency is a first-class maintenance operation; drift is a red build without network.

## Phase 11: Polish & Cross-Cutting Concerns

**Purpose**: Whole-feature gates and documentation close-out (Principle VIII; quickstart 13).

- [X] T062 [P] Run `cmake --build build/dev -t dbc-gate` clean: every new public interface in `include/speedgun-ng/counters*.hpp` pairs doxygen contracts with runtime enforcement
- [X] T063 [P] Run `cmake -P cmake/prose-lint.cmake` (prose-lint) clean over all added prose: specs, README section, docs page (Principle XI)
- [X] T064 [P] Run `cmake --build build/dev -t format-check` clean; formatting-only fixes in a separate commit (Principle V)
- [X] T065 [P] `cmake --preset=ci-sanitize && cmake --build --preset=ci-sanitize && ctest --preset=ci-sanitize` clean: group-fd lifetimes and the arena under ASan/UBSan (plan "Determinism and regression")
- [X] T066 [P] Coverage preset: 100% line/branch/DBC gates hold for the `include/speedgun-ng/counters*` and `source/counters/` additions; `SG_*` macro lines excluded via the existing `--omit-lines` mechanism; the `SG_REQUIRE_ALWAYS` bounds abort branch covered by the out-of-process trap pair; the mapped-page decode and the config encoder carry synthetic page and index fixtures in `test/source/counters_linux_pmu_seam_test.cpp` (Principle VI). Settled 2026-09-27: the gate reports lines 100.0% (1800 of 1800) and branches 100.0% (676 of 676) on the coverage preset, with 296 `LCOV_EXCL` tokens remaining, each stating a kernel gate, a cited-callers-guaranteed invariant, or a compiler-emitted block at its own site. An audit cleared 91 tokens whose stated reasons were false and fixed the two defects they concealed (the `to_ecma` POSIX-class slice, the `parse_scalar` unreachable arm); a const `find` overload with no callers was deleted rather than excluded. `geninfo_unexecuted_blocks` in `cmake/coverage.cmake` is unchanged, so no threshold moved (quickstart section 13 gate table; plan Complexity Tracking)
- [X] T067 Verify `consumer-release` job semantics: the release artifact carries contract code only at the spec-mandated always-on `hard_stop` bounds site (dbc facility `SG_*_ALWAYS` registry distinguishes it) (plan gate-set note, FR-027)
- [X] T068 Document the cadence idiom in `include/speedgun-ng/counters_measurement.hpp` header docs: chunked sampling every K iterations, capacity `N/K + 1`, `fold_pairs` for the per-interval series, the K=1 observer-effect cost stated numerically from the plan calibration; plus the 2^53 exactness note (FR-032/048, E-09; Principle IV)
- [X] T069 Run quickstart.md sections 1-13 end to end as final validation; record the verdicts against the SC-001..SC-010 index in the PR description (Constitution III/IX)

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies - can start immediately
- **Foundational (Phase 2)**: Depends on Setup - **BLOCKS all user stories**
- **User Stories (Phases 3-10)**: All depend on Foundational completion
  - US1 (Phase 3) then US2 (Phase 4): both P1; US2's recorder extends US1's scope/fold internals (same spine), so one owner or strict order
  - US3 (Phase 5): needs US1 resolution + US2 recorder mechanics in-tree
  - US4 (Phase 6): needs Foundational + US1 scope/fold mechanics
  - US5 (Phase 7): needs US1 + US4 (giraffe composes with `monotonic`)
  - US6 (Phase 8): needs US1-US3 mechanics + the T002 licensing verdict (fallback path if adverse)
  - US7 (Phase 9): needs US6 (fast mode layers on linux_pmu mechanisms)
  - US8 (Phase 10): needs T044 (vendored tree + gate exist to check)
- **Polish (Phase 11)**: Depends on all desired stories complete

### User Story Dependencies

- **US1 (P1)**: After Foundational - no story dependencies
- **US2 (P1)**: After US1 (shares the fold/scope spine) - independently testable once landed
- **US3 (P2)**: After US1 + US2 - rides their mechanics, independently testable on fake trees
- **US4 (P2)**: After US1 - independent of US2/US3
- **US5 (P2)**: After US1 + US4 - independent of US2/US3
- **US6 (P3)**: After US3 + T002/T044
- **US7 (P3)**: After US6
- **US8 (P3)**: After T044 - independent of US1-US7 code paths

### Within Each User Story

- Tests/fixtures written FIRST and verified RED (plan records TDD mode)
- Headers/algebra before plan compile; plan before fold kernels; fold before scope sugar
- Provider implementation before its story's registration turns the suite GREEN
- Story complete before moving to the next priority

### Parallel Opportunities

- T001, T002 parallel; T010 parallel with T009
- US1: T012 + T013 parallel (test-first, different files)
- US2: T022 + T023 parallel; T027 parallel with T024-T026
- US3 and US4 phases run in parallel once US1/US2 land (T029 vs T034 test-first)
- US5: T039, T040, T041 mutually parallel (distinct files)
- US6: T045 + T046 parallel after T044; T047 after T045/T046; T048 parallel with T046/T047
- US7: T051 + T070 parallel (distinct files); T052-T055 sequential after them
- US8: T057 first; T060, T061 parallel
- Polish: T062, T063, T064, T065, T066 mutually parallel

---

## Parallel Example: User Story 6

```bash
# Tests first (RED):
Task: "test/source/counters_pmu_test.cpp - paranoid-2 permission_blocked reporting, merge, encodability, group read (T043)"

# After T044 (vendored tree + gate bracket):
Task: "source/counters/linux_pmu/table_parse.cpp - CPUID mapfile match, lazy simdjson parse (T045)"
Task: "source/counters/linux_pmu/encode.cpp - JSON codes + sysfs format bits, not_encodable (T046)"
# Then:
Task: "source/counters/linux_pmu/provider.cpp - sysfs merge kernel-wins, availability probe (T047)"
Task: "source/counters/linux_pmu/group_io.cpp - one PERF_FORMAT_GROUP read per leader, enabled/running leaves (T048)"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup
2. Complete Phase 2: Foundational (CRITICAL - blocks all stories)
3. Complete Phase 3: User Story 1 (T012-T021; tests RED first)
4. **STOP and VALIDATE**: `ctest -R "counters_fake|counters_compile_fail"` - quickstart 3/4 verdicts
5. Demo: a fake-provider IPC-shape metric with disclosure and provenance

### Incremental Delivery

1. Setup + Foundational - spine ready
2. Add US1 - test independently - measurable derived metrics (MVP!)
3. Add US2 - test independently - hot-loop sampling, wrap/ring/drop exactness, zero-alloc proof
4. Add US3 - test independently - per-core/per-controller cross-object measurement
5. Add US4 - test independently - unprivileged real-time + user events on every platform
6. Add US5 - test independently - out-of-tree extensibility + standalone link proof
7. Add US6 - test independently - real hardware PMU on Linux, green at paranoid 2 via permission_blocked states
8. Add US7 - test independently - published fast/syscall regime budgets
9. Add US8 - test independently - table-currency maintenance in CI
10. Polish - full gate matrix green

### Parallel Team Strategy

With multiple developers:

1. Team completes Setup + Foundational together
2. Once Foundational is done:
   - Developer A: US1 then US2 (the shared spine)
   - Developer B: US8 (vendored tree T044 + tool; unblocks US6/US7 for A later)
3. After US1/US2 land: B takes US4 then US5; A takes US3, then US6-US7

---

## Notes

- [P] tasks = different files, no dependencies
- [Story] label maps a task to a user story for traceability
- Every assertion is a value equality, a command exit, a grep verdict, or an exit-code-gated skip (Constitution X.4); the fake provider removes all timing from the correctness spine
- Verify tests fail before implementing (TDD mode, plan Test Plan)
- Commit after each task or logical group; `<Section>: <one-line imperative>` per the constitution PR template
- Stop at any checkpoint to validate a story independently
- Avoid: vague tasks, same-file conflicts in parallel batches, cross-story dependencies that break independence

---

## Phase 12: Convergence

Appended by `/speckit.converge` on 2026-09-27 after a per-task audit of T001..T070 against
the code. Nothing above this line changed. Every row traces to a `FR-###`, `SC-###`,
user-story acceptance scenario, plan decision, or constitution principle, and carries a
`file:line` site. Constitution violations come first and are marked CRITICAL.

Audit evidence: `cmake --build --preset=dev` exit 0; `ctest --preset=dev` 34/34 exit 0
(`counters_overhead` skipped, probe-gated); `dbc-gate` 0 (134 interfaces, 0 gaps);
`prose-lint` 0; `format-check` 0; `ci-sanitize` CI-equivalent 0/0/0; Release+ignore consumer
build carries 0 `sg::dbc::check_` symbols while the `overrun` trap mode exits 134;
`update_pmu_events.py --check` 0, drifted fixture 1 naming the file; the coverage target
runs and reports `lines 85.1% (1686 of 1982), functions 90.2% (230 of 255), branches 73.9%
(678 of 917)`.

### CRITICAL: constitution MUST violations

- [X] T071 Add `SG_ENSURE(!started && !finished, "a fresh scope awaits start() (FR-030)")` to `scope::scope` at `source/counters/plan.cpp:270-276` and move the uncheckable `\pre compiled outlives the scope` clause out of the enforced contract into the lifetime note beside `include/speedgun-ng/counters_measurement.hpp:946-950`; also state `expression::fold_pairs`'s real two-point precondition at `counters_measurement.hpp:654-655` instead of `\pre none \post none` (CRITICAL, Constitution II, `partial`)
- [X] T072 Write the reason in the same comment for all 8 bare `NOLINTNEXTLINE` directives: `include/speedgun-ng/counters_core.hpp:216` and `:234`, `include/speedgun-ng/counters_provider.hpp:37`, `:62`, `:125`, `:135`, `:150`, and `include/speedgun-ng/counters_measurement.hpp:610` (CRITICAL, Constitution X.2, `partial`)
- [X] T073 Split the contrastive sentence in `source/counters/linux_pmu/table_parse.cpp:18-19` so no unit carries the `X rather than Y` shape, and drop the filler token `actually` from the comment at `include/speedgun-ng/counters_measurement.hpp:147`; both are reviewer-parity defects under `constitution.md:461-462` rather than mechanical prose-lint findings, because `prose_rules.yaml:94` matches a leading space the stripped comment unit does not carry and `prose_gate.py:876-883` exempts a whole unit containing a code span (CRITICAL, Constitution XI.2 and XI.5, `partial`)

### HIGH: core functional requirement and acceptance-criterion gaps

- [X] T074 Stop dereferencing before checking: `include/speedgun-ng/counters_system.hpp:119` evaluates `dimension_of(*unit_from_token(leaf->unit))` and only tests `mapped.has_value()` at `:120-122`; test the inner `std::expected` first (HIGH, FR-017, `partial`)
- [X] T075 Assign `enabled` and `running` on `pmu_fast_window` at `source/counters/linux_pmu/group_io.cpp:269-270` from the leader's `user_access_page::time_enabled`/`time_running` before yielding them at `:296-300`; today fast mode publishes a hardcoded multiplex ratio of 0 (HIGH, FR-041, `contradicts`)
- [X] T076 Probe the two missing fast-read inputs in `source/counters/linux_pmu/fast_read.cpp`: the `perf_user_access` sysctl and the page `version`/`compat_version` fields declared at `:91-92` and `:120-121` and never read; correct T053's claim that mode assignment lives in `source/counters/plan.cpp`, since it happens in `linux_pmu/provider.cpp:193` and `clock_provider.cpp:234` (HIGH, R-011, `partial`)
- [X] T077 Apply the capability gate inside the decode, the ordering the code ships and the header documents: `fast_context_read` reads the page capability bit and issues the instruction under the index test alone at `:281`, and `fast_decode` applies the capability gate as its first check at `:86-88`. The hoist this task originally required was withdrawn by commit `2889608`, which deleted the gate and its comment from `fast_context_read` and moved the gate into the decode; the reason is recorded at `source/counters/detail/pmu.hpp:249-254`, where the header states that the caller reads the instruction only for a nonzero index, so the gate costs a page load and never an instruction (HIGH, FR-040, `contradicts`, amended)
- [X] T078 Either read `event_page::lock` at `source/counters/linux_pmu/fast_read.cpp:122`, which is declared and never read anywhere in the repository, or amend T052's "seqcount `lock` snapshot" wording to name the index field the code compares at `:271` and `:287` (HIGH, FR-040, `partial`)
- [X] T079 Reconcile `specs/007-counters-and-timers/plan.md:43` and `:365`, which register a `reinterpret_cast` P2 exception, with the `static_cast` at `source/counters/linux_pmu/fast_read.cpp:191` and `:264-265`; either spell the cast as the plan records it or amend the plan (HIGH, plan: Complexity Tracking, `contradicts`)
- [X] T080 Add a forced-`syscall` read-mode override to `compile()` so `test/source/counters_overhead.cpp` measures one plan in both regimes, and assert the SC-004 binary check from `quickstart.md:108` that the fast median sits at least 100x below the syscall median; or amend `spec.md:301` and `quickstart.md:108` to the two-plan comparison the benchmark performs, and publish a fast row in `docs/pages/counters-overhead.md` whose `:63-94` section currently records no measurement (HIGH, SC-004, `partial`)
- [X] T081 State FR-047's one-sampling-action obligation as a runtime check at the column-commit site: in `sample_row` at `source/counters/plan.cpp:32-41`, after the read-group loop, assert the points written equal `layout.slots.size()`, and pair the doxygen `\invariant` with it; the reader-side prose already exists at `include/speedgun-ng/counters_provider.hpp:263-265` and no `SG_INVARIANT` exists anywhere in `counters_measurement.hpp` or `plan.cpp` (HIGH, FR-047, `missing`)
- [X] T082 Call `set_thunk` in each shipped provider's window constructor so the read path at `source/counters/plan.cpp:32-41` stops dispatching through `default_thunk` at `include/speedgun-ng/counters_provider.hpp:299-303` to the virtual `read_points` at `:270`; or record in `plan.md` why the default thunk satisfies FR-022's no-dynamic-dispatch clause (HIGH, FR-022, `contradicts`)
- [X] T083 Add the US1 scenario 5 assertion to `test/source/counters_fake_test.cpp`: ten further `.metric()` calls on a finished scope return bit-identical results and the fake provider's read counter does not advance; the nearest existing check is a single fold followed by `read_actions() == 2` at `:281` (HIGH, US1 scenario 5 and FR-021, `missing`)
- [X] T084 Add the assembled provenance record SC-008 names to `test/source/counters_fake_test.cpp` and assert it end to end; today only individual `points_view` fields are checked at `:275-280` and no record is ever built (HIGH, SC-008, `missing`)
- [X] T085 Compute `points_view::ratio` at `source/counters/fold.cpp:215` from `window_ratio` (`:105-137`) instead of the hardcoded `1.0`, so a multiplexed source discloses its real ratio (HIGH, FR-020, `partial`)
- [X] T086 Either validate per-leaf target and clock identity at plan construction and return a recoverable error on mismatch, or amend FR-024 at `spec.md:242` to the single-plan-target design and correct T043's "plan construction over mismatched targets/clock ids fails recoverably", which names a phase that does not exist: `bound_thread` is a default member initializer at `source/counters/detail/core.hpp:98` and provider `open()` runs at compile (`plan.cpp:445`) (HIGH, FR-024, `missing`)
- [X] T087 Add the `perf_event_paranoid` read to the availability probe in `source/counters/linux_pmu/provider.cpp:337-350`, and either emit the `absent` availability state the provider never produces or correct the seam header's own comment at `source/counters/detail/pmu.hpp:88-90` (HIGH, FR-039, `partial`)
- [X] T088 Add the seeded deterministic per-sample delta generator to `fake_script` in `include/speedgun-ng/counters_fake.hpp:38-42`, which today holds only `points` plus a constant `tail_delta`, and exercise it from `test/source/counters_fake_test.cpp`; or amend T014, which is the only artifact requiring it (HIGH, T014, `partial`)
- [X] T089 Either add a scope registration API and guard registering a composite into a started scope, or amend the spec edge case at `spec.md:189` to state that the `start`/`finish`/`metric` spelling at `include/speedgun-ng/counters_measurement.hpp:952-1023` makes the misuse unrepresentable (HIGH, FR-046, `partial`)
- [X] T090 Add an out-of-extent fold trap mode to `test/source/counters_trap_fixture.cpp` and register it in `test/source/counters_trap_checked_test.cpp:22-28` and in the ignore-build greps at `.github/workflows/ci.yml:386-391`; the guard at `include/speedgun-ng/counters_measurement.hpp:632` and `source/counters/fold.cpp:144` is exercised only for `i >= j` (HIGH, US2 scenario 6, `partial`)
- [X] T091 Store a per-composite fold program of slot references in `plan_impl` at `source/counters/detail/core.hpp:76-109`, which today holds slots, groups and arenas only, so the fold stops re-resolving leaf to slot through a string-keyed map at `source/counters/fold.cpp:42` and `:165`; or amend T017's "fold program per composite (column references with algebraic exponents and ops)" (HIGH, FR-022, `partial`)
- [X] T092 Assert `counter::description()` in `test/source/counters_fake_test.cpp` for US1 scenario 1, whose handle accessor at `include/speedgun-ng/counters_measurement.hpp:224` is exercised by no test, and assert `points_view::slot` and `count` from `:381-391` for scenario 7 (HIGH, US1 scenarios 1 and 7, `partial`)
- [X] T093 Record the SC-001..SC-010 verdicts in the tree: fill the Verdict column of the table at `specs/007-counters-and-timers/quickstart.md:122-133` with the observed outcome per row and the command that produced it, following the `.omo/evidence/001-dbc-facility/` convention; T069 is marked complete but points its evidence at the PR description, which is not in the tree (HIGH, T069, `missing`)

### MEDIUM

- [X] T094 Replace the no-op `check(true, ...)` for US6 scenario 4 at `test/source/counters_pmu_test.cpp:226-229` with a real assertion on the reported availability states, and record the paranoid-2 hardware-entry state the task's Phase 8 independent test claims (MEDIUM, US6 scenario 4, `partial`)
- [X] T095 Stage the machine root's leaves in `source/counters/system.cpp:209-215` and insert them after the validation loop, so a later failing seed leaves the tree unchanged as `include/speedgun-ng/counters_system.hpp:180-183` and `include/speedgun-ng/counters_provider.hpp:69-71` promise (MEDIUM, FR-008, `partial`)
- [X] T096 Add a documented action to close a duplicate platform alias, which `source/counters/system.cpp:227` drops silently through `std::map::emplace`, so a second registrant loses the alias and it keeps resolving to the first object (MEDIUM, FR-002, `partial`)
- [X] T097 Support provider-declared attribute keys in `objects(kind, filters)`, which today hardcodes `package` and `core` at `source/counters/system.cpp:283` and has no field to declare them in at `include/speedgun-ng/counters_provider.hpp:60-67`; or amend FR-003 (MEDIUM, FR-003, `partial`)
- [X] T098 Split plan read groups per provider instance as T032's "per-instance provider group reads" states, or amend the task; grouping today is per provider at `source/counters/plan.cpp:419-454`, so a fan-out over many objects is one group read (MEDIUM, T032, `contradicts`)
- [X] T099 Assert canonical spelling in a provenance line and in a diagnostic, not only in `path()` equality, for US3 scenario 2; the test at `test/source/counters_objects_test.cpp:139-145` covers resolution only (MEDIUM, US3 scenario 2, `partial`)
- [X] T100 Add a mechanical gate for FR-035's "no atomic read-modify-write on any path", which today rests on comments alone at `include/speedgun-ng/counters_push.hpp:33-34`, `include/speedgun-ng/counters_measurement.hpp:264` and `:317`, and `source/counters/push_provider.cpp:31` (MEDIUM, FR-035, `partial`)
- [X] T101 Assert the catalog's `scaled` flag for the `tsc` leaf in `test/source/counters_clock_push_test.cpp` for US4 scenario 5; `catalog_entry::scaled` is declared at `include/speedgun-ng/counters_core.hpp:225`, populated at `source/counters/system.cpp:377`, set at `source/counters/clock_provider.cpp:236`, and read by no test (MEDIUM, US4 scenario 5, `partial`)
- [X] T102 Drop the word-boundary match from `test/counters_header_purity.sh:22` so `perf_event` matches `perf_event_open`, and state in the task that the scan covers four terms only, since `CLOCK_MONOTONIC`, `CPUID`, `sysfs` and `pmu` are outside it; do not add `tsc`, which FR-023's `fast_tsc` requires (MEDIUM, FR-010, `partial`)
- [X] T103 Add contract blocks to the `include/speedgun-ng/counters.hpp` umbrella or amend T004, which names the umbrella among the six headers "with doxygen pre/post/invariant contract blocks"; the file carries none, and `\invariant` appears exactly once across the whole header set at `include/speedgun-ng/counters_provider.hpp:227` (MEDIUM, T004, `partial`)
- [X] T104 Make the counters `target_sources` bracket in `CMakeLists.txt:653-670` recursive so a future `source/counters/detail/*.cpp` is not silently dropped, as the two non-recursive GLOBs do today (MEDIUM, T003, `partial`)
- [X] T105 Add a release-configured CTest target so `ctest -R counters_trap` reproduces the FR-027 release proof, which today lives only in the CI shell step at `.github/workflows/ci.yml:343-398` and runs the fixture rather than the checker; or amend T023 and the US2 independent test at `tasks.md:78` (MEDIUM, FR-027, `partial`)
- [X] T106 Add a `ci-sanitize` build preset and a `ci-sanitize` test preset to the committed `CMakePresets.json` so T065's command and `quickstart.md:114` can run; the file currently declares no `buildPresets` and no `testPresets`, and the `dev` presets must stay in the machine-local `CMakeUserPresets.json` (`constitution.md:568-570`) (MEDIUM, T065, `partial`)
- [X] T107 Read the staging tree when replacing in `tools/pmu_events/update_pmu_events.py:325-336`, where the staging directory is written and never used because the tree is rewritten from the in-memory dict; or drop the staging directory and record that the replace is last-writer-wins from memory (MEDIUM, T059, `partial`)
- [X] T108 Replace the full set of allocation functions in `test/source/counters_noalloc_test.cpp:42-76`, which overrides only the plain `operator new(std::size_t)` and leaves `new[]`, the nothrow form and the aligned forms uncounted, so an array-form allocation on the sample path escapes the count (MEDIUM, SC-005, `partial`)
- [X] T109 Delete or repair `test/source/counters_recorder_test.cpp:107`, which asserts that a full `hard_stop` recorder never wraps, a fact `hard_stop_sample_core` at `source/counters/plan.cpp:318-330` can never violate because it never assigns the flag (MEDIUM, T022, `partial`)
- [X] T110 Add a fake leaf carrying an enabled/running pair and assert the composite ratio product at `source/counters/fold.cpp:130-131`, which no test can reach today because no fake leaf sets `has_ratio_pair` (`source/counters/fake_provider.cpp:133-139`) (MEDIUM, FR-019, `partial`)
- [X] T111 Move the unrecognized-unit failure from `register_provider` to resolution as US1 scenario 6 frames it, or amend the spec; today `source/counters/system.cpp:174-177` rejects at registration, leaving the resolution-side switch at `include/speedgun-ng/counters_system.hpp:119-122` unreachable and `object::counters()` degrading an unmappable unit to `unit::none` at `source/counters/system.cpp:373` (MEDIUM, US1 scenario 6, `partial`)
- [X] T112 Register the P2 coverage exclusion that `plan.md:335` promises in the Complexity Tracking table at `plan.md:363-366`, with its written justification, or drop the claim; no `LCOV_EXCL` marker exists in the counters scope (MEDIUM, plan.md:335, `missing`)
- [X] T113 Own the opaque handles with `std::unique_ptr`: `source/counters/system.cpp:260` wraps `new` where `std::make_unique` applies, and `source/counters/system.cpp:124` leaves the destructor defaulted while nothing deletes `m_impl` declared at `include/speedgun-ng/counters_system.hpp:249`, so one heap block per process is never reclaimed (MEDIUM, Constitution I, `partial`)
- [X] T114 Record the `Counters` section token added to `tools/prose/prose_rules.yaml:23` inside the 007 commit range in `specs/002-prose-commit-lint`, which owns that artifact, or revert it; the gate change at `tools/dbc/dbc_pair_gate.py` is already covered by commit `3c67647` with its `Approved-by` footer (MEDIUM, Constitution IX, `unrequested`)
- [X] T115 Remove the Windows behaviour promise from the `pmu_provider` class doc at `include/speedgun-ng/counters_pmu.hpp:31-33`, which claims a syscall-mode group read per leader, where `:34-36` and `source/counters/linux_pmu/provider.cpp:40-49` state the provider seeds no objects off Linux and `spec.md:312` defers Windows and macOS hardware-PMU providers (MEDIUM, FR-042, `contradicts`)
- [X] T116 Record an explicit decision on the three parallel implementations of one sampling point (`scope`'s own buffer and state at `source/counters/plan.cpp:270-276`, separate from `hard_stop_sample_core` at `:318-330` and `ring_sample_core` at `:332-345`) or fold them onto one implementation; FR-030's one-semantics claim currently rests on tests rather than shared code (MEDIUM, Constitution X.2, `unrequested`)
- [X] T117 Attribute `test/source/counters_linux_pmu_seam_test.cpp` to a task, and add the three shipped provider headers to T004's list and to `plan.md:317`'s public API surface; T004 at `tasks.md:30` and `plan.md:46` name six public headers where nine exist, and no artifact names `counters_clock.hpp`, `counters_push.hpp` or `counters_pmu.hpp` (MEDIUM, T004, `partial`)

### LOW

- [X] T118 Add the trivially-copyable `static_assert` for `recorder_handle<P>` to `include/speedgun-ng/counters_measurement.hpp`, whose prose at `:440-441` claims a trivially copyable cursor while the only assertion lives in `test/source/counters_recorder_test.cpp:60-65` (LOW, T024, `partial`)
- [X] T119 Reconcile T006's `std::expected<dim, error>` wording with the `std::expected<dimension, error>` return at `include/speedgun-ng/counters_core.hpp:171-179`, and note that the unit-naming error lives in the token if-chain at `:134-156` rather than the switch (LOW, T006, `partial`)
- [X] T120 Restate FR-049 at `spec.md:276` and SC-001 at `spec.md:298` as "no third-party dynamic dependency": the target is a static archive (`CMakeLists.txt:18-21` declares `add_library` with no type keyword), so `ldd` and `readelf -d` name no `speedgun-ng` entry at all (LOW, FR-049, `partial`)
- [X] T121 Register the standalone example's clock provider alongside the fake one in `example/counters_standalone_example.cpp:38-48`, or amend T041, which says "fake+clock sources"; SC-001's "over fake or clock sources" is satisfied as written (LOW, T041, `partial`)
- [X] T122 Route the probe-failure reason into CTest-visible output for `counters_overhead`: the reason is printed to stdout at `test/source/counters_overhead.cpp:251-258` but CTest swallows it for a skipped test, so `quickstart.md` section 1's "skips with the failure named" needs `-V` (LOW, T051, `partial`)
- [X] T123 Add a generation timestamp and the raw trial inputs to `docs/pages/counters-overhead.md`, which has neither where its named precedent `docs/pages/dbc-overhead.md:9` and `:74-91` has both, and refresh its figures at `:55-56` and the header values derived from them at `include/speedgun-ng/counters_measurement.hpp:43-48` (LOW, T056, `partial`)
- [X] T124 Check the synthetic drift trees into `test/pmu-events-gate-fixture/` or state in T057 that they are generated at run time into the build tree, as they are today, so a reviewer can read the drift cases in the source tree (LOW, T057, `partial`)
- [X] T125 Remove the unused `import shutil` at `tools/pmu_events/update_pmu_events.py:42` (LOW, Constitution X, `unrequested`)
- [X] T126 Fix T025's "allocated at plan finalization" wording: the code allocates per recorder in `plan::recorder` at `source/counters/plan.cpp:181-183` and FR-029 at `spec.md:247` says "allocated at construction" (LOW, T025, `partial`)
- [X] T127 Defer the `tsc` calibration to the system-open boundary FR-034 names, or amend FR-034: the calibration runs in the clock provider constructor at `source/counters/clock_provider.cpp:170-193`, before any open (LOW, FR-034, `contradicts`)
- [X] T128 Correct the comment at `source/counters/clock_provider.cpp:175-176` that calls the CPUID leaf 0x16 read an invariance cross-check: leaf 0x16 reports nominal core frequency, which is what `:187-190` compares (LOW, T036, `contradicts`)
- [X] T129 Confirm the reading of T055's "cached `thread::id` compare", or amend the task: `plan_impl::bound_thread` is cached at `source/counters/detail/core.hpp:98` but three fresh `std::this_thread::get_id()` queries remain on the critical path at `source/counters/plan.cpp:301`, `:326`, and `:340` (LOW, T055, `partial`)
- [X] T130 Remove the unused `cyc2` and `ins2` fixture leaves at `test/source/counters_recorder_test.cpp:187-198`, which are registered and never sampled (LOW, Constitution X, `unrequested`)


## Phase 13: Fast-Path Verification

Appended 2026-09-27 after the sudo grant. US7 was marked complete on the
strength of its code and its synthetic fixtures while the fast read
mechanism had never executed on real silicon, so SC-004's fast side and
the whole of `fast_read.cpp` rested on the kernel's own refusal. These
tasks are closed 2026-09-27: the mechanism now executes on this host, the
fast path runs for real under `ctest`, and `docs/pages/counters-overhead.md`
publishes the measured fast regime. Three findings came out of the work
that the tasks did not anticipate, and each is recorded where a reader
will meet it. The fast path discarded the plan's target, so a cpu-pinned
plan counted the calling thread; it now binds through the same
`leader_pid` the group path uses. A mapped-page enabled/running pair is
quantized by the kernel, which schedules the page rewrite, so a window
with no syscall inside it can read the same value twice. And the SC-004
order check is not met on this host, because the only syscall-mode
counterpart the catalog offers is a clock leaf and a vDSO `clock_gettime`
beats any hardware-counter read; the check is published and reported
instead of asserted, and the verdict row says so.

- [X] T131 ANSWERED, fixed by T137..T139. The diagnosis stands as written: `/usr/include/linux/perf_event.h` publishes the read protocol and it consults no sysfs attribute: the caller maps one page of the `perf_event_open` descriptor and reads `lock`, `index`, `offset`, the `cap_user_rdpmc` bit of `capabilities`, and `pmc_width`, issuing `rdpmc(index - 1)`. The `12..21` band at `source/counters/linux_pmu/fast_read.cpp:260` and `:346` is therefore wrong on every host, and the value `1` this AMD Zen 5 host publishes is not a page-size shift. The deeper defect is at `fast_read.cpp:282`, where the probe `mmap`s the sysfs attribute `/sys/bus/event_source/devices/cpu/rdpmc` and reads it as a perf user-access page; that file is a scalar, so the mapping can never hold a page. `fast_context_read` at `:377` reads its capability bit from the same bogus mapping, so relaxing the band alone would not fix the runtime path. The remaining work it named is done: the capability and the width come from the event's own mapping, the `user_access_page` mirror and the sysfs attribute are gone, the width mask comes from `pmc_width`, and the page type is the kernel's own. See T137..T139 (FR-023, FR-040, R-011, US7 scenario 1)
- [X] T132 ANSWERED. User rdpmc functions on this host. A standalone probe at `/tmp/opencode/rdpmc_probe.cpp`, written to follow the header's protocol and to share no assumption with the library, reported `perf_event_open` ok, one page mapped, `capabilities=0x1e` with `cap_user_rdpmc=1` and `cap_user_time=1`, `pmc_width=48`, `index=1`, `offset=140737488355327`, and a real `rdpmc` reading. `offset` is the standard 47-bit preload for a counting-up 48-bit counter. The host is fast-capable by the kernel's own account, so the exclusion registry's claim that these lines are kernel-gated is wrong and must be withdrawn once T131 lands (FR-040, R-011)
- [X] T137 Replace the hand-mirrored `user_access_page` (`source/counters/linux_pmu/fast_read.cpp:135`) and `event_page` (`:165`) with the kernel's own `perf_event_mmap_page` from `<linux/perf_event.h>`, already included at `:22`. Delete the version-matching arithmetic that justified the mirrors and withdraw the P2 cast entry from the Complexity Tracking table, since the page type is now the kernel's (Constitution I, R-011, plan Complexity Tracking)
- [X] T138 Rewrite `pmu_probe_fast` (`fast_read.cpp:240-307`) to open one real event through `perf_event_open`, map one page of the returned descriptor, and read `cap_user_rdpmc` and `pmc_width` from that mapping. Delete the sysfs `rdpmc` read and the `12..21` shift band at `:260`. The probe must then report the kernel's own account of this host, which T132 measured as fast-capable (FR-023, FR-040, R-011)
- [X] T139 Change `fast_context_open` and `fast_context_read` (`fast_read.cpp:309-444`) to take the capability bit and the value width from the event page, and to mask with `pmc_width` instead of the hardcoded 48-bit constant. The kernel publishes the width per host, so a host whose counter width differs from 48 must measure correctly (FR-040, R-011)
- [X] T140 Delete the 11 `LCOV_EXCL` markers in `fast_read.cpp` and the fast-mode markers in `group_io.cpp` that the withdrawn kernel-gate reason covered, and add real coverage: the probe path now runs on this host under `ctest`, so its branches are measured, and the `fast_context_read` sequence stays covered by the synthetic-page fixtures that already exercise it (Principle VI, plan Complexity Tracking)
- [X] T141 Withdraw the kernel-gate claim from the plan's coverage-exclusion row and from `docs/pages/counters-overhead.md`, which currently states the fast regime is unreachable on this host, and record the probe's actual findings: `capabilities=0x1e`, `cap_user_rdpmc=1`, `pmc_width=48`, `index=1`, `offset=140737488355327` (Constitution VIII, SC-004)
- [X] T133 On a host where T132 passes, run `ctest --preset=dev -R counters_overhead` and record the fast and syscall distributions side by side, then publish a measured fast row in `docs/pages/counters-overhead.md` where line 78 currently reads `unmeasured` in all three columns, and flip SC-004 in the `specs/007-counters-and-timers/quickstart.md` verdict table from PARTIAL to PASS. The pass check is the order comparison T080 settled: the fast-mode median sits below the syscall-mode median (SC-004, SC-010, US7 scenario 6, quickstart section 12)
- [X] T134 Cover the multiplex-ratio scenario 6, which `test/source/counters_pmu_test.cpp` skips on the grounds that it needs a privileged host. Sudo is now available, so the privilege is no longer the obstacle; the remaining question is whether this PMU multiplexes at all once more events are opened than it has counters, with `perf_event_mux_interval_ms` shortened to force it. Record the result either way (US6 scenario 6, US7 scenario 4, FR-041)
- [X] T135 Re-verify SC-002 at `perf_event_paranoid` 2 and record the log, since the sudo grant moved the host to 1 and flipped 358 hardware entries from `permission_blocked` to `countable`. The recorded `sc-002-pmu.log` was captured at 2, so the artifact stands, but the two demonstrations cannot both be reproduced on one host and the handoff must say which setting each needs (SC-002, FR-039, US6 scenario 4)
- [X] T136 Confirm the `tsc` leaf's absence is a kernel configuration fact and record it as such. `/sys/devices/system/cpu/tsc_khz` is absent on this host, so FR-034's calibrated `fast_tsc` leaf stays unpublished regardless of privilege, and no task may claim the fast_tsc regime is reachable here. `docs/pages/counters-overhead.md:140` already states this; the spec's Assumptions should name the kernel configuration that publishes the frequency (FR-034, R-007)

## Phase 14: Convergence

Appended by `/speckit.converge` after an audit of every T001..T141 completion claim
against the code. Nothing above this line changed. Each row traces to a `FR-###`,
`SC-###`, user-story acceptance scenario, plan decision, or constitution clause, and
carries a `file:line` site.

Audit evidence, all produced by this pass on the shared `build/dev` tree:
`cmake --preset=dev` exit 0; `cmake --build --preset=dev` exit 0; `ctest --preset=dev`
35 of 35 passed, 0 failed, 0 skipped, 43.28 s total, which supersedes the 34 of 34
Phase 12 recorded; `dbc-gate` 135 interfaces with 0 gaps; `prose-lint` exits 0.
Coverage of the check: 109 requirement keys
(50 FR, 10 SC, 49 user-story acceptance scenarios) with 17 spec edge cases, 26 plan
decisions, and 14 constitution clauses. Nineteen findings: 4 missing, 9 partial, 6
contradicts; 3 CRITICAL, 7 HIGH, 5 MEDIUM, 4 LOW.

This phase covers four classes the earlier waves left open. One algebra defect where a
scalar multiple of a quotient folds unchanged while reporting `scaled` (T142). Three
prior-wave claims whose resolution is absent from the tree, T082's `set_thunk` call,
T086's target and clock identity validation, and T127's calibration boundary, plus T089
and T126 whose stated artifact amendments never landed (T146, T147, T153, T155, T160).
Two defects in the shipped fast path and object tree, the unbound fast context and the
silently dropped duplicate path (T145, T148). And the test-evidence class, four
assertions that cannot fail and two acceptance criteria with no covering test (T149,
T150, T151, T156).

### CRITICAL: constitution MUST violations and a P1 algebra defect

- [X] T142 Make `scale_all` at `include/speedgun-ng/counters_measurement.hpp:136-143` scale the composite instead of every leaf, so `2.0 * (a / b)` folds to twice the quotient where `source/counters/fold.cpp:41-47` currently returns `a / b`; add the case to `test/source/counters_fake_test.cpp` beside the add-spine scale check at `:856-859`, and make `carries_scale` at `source/counters/fold.cpp:62-70` report `scaled` only for a value the scale actually reached (FR-015, FR-019, `contradicts`)
- [X] T143 Restate the three contrastive code comments so every clause stands on its own terms, at `source/counters/system.cpp:176`, `source/counters/linux_pmu/table_parse.cpp:299-300`, and `test/source/counters_core_test.cpp:149`; `tools/prose/prose_gate.py:875-880` exempts each of them because the unit carries an inline code span, so the mechanical gate cannot catch them and a reviewer must (Constitution XI.2, `partial`)
- [X] T144 Remove the banned vocabulary from the string literals, which `tools/prose/prose_gate.py:572` never reads because a `.cpp` or `.hpp` unit is classified as `c-comment`: drop `` `actually` `` from the catalog description at `source/counters/linux_pmu/provider.cpp:138`, drop `` `honest` `` from the assertion label at `test/source/counters_fake_test.cpp:395`, restate the dimension-mismatch diagnostic at `include/speedgun-ng/counters_system.hpp:145-148` without the contrast, and restate the three failure labels at `test/source/counters_pmu_test.cpp:325-332` and `:643` (Constitution XI.2, XI.3, XI.5, `partial`)

### HIGH: core functional requirement and acceptance-criterion gaps

- [X] T145 Reject a duplicate canonical path inside one provider's seed batch at `source/counters/system.cpp:306`, which consults only the merged `m_impl->objects` and never the `staged` vector, so the non-overwriting `emplace` at `:348` destroys the second node while its alias still lands at `:350-352`; add the `staged` path lookup beside the `staged_aliases` scan at `:334` and add a fixture to `test/source/counters_objects_test.cpp` that registers one hand-written `provider_iface` declaring the same path twice, since `fake_provider::add_object` keys on path at `source/counters/fake_provider.cpp:73` and cannot express the case (US3 scenario 6, FR-001, `missing`)
- [X] T146 Remove the virtual dispatch from the sampling path: `set_thunk` at `include/speedgun-ng/counters_provider.hpp:318` is called from nowhere in the tree, so `m_thunk` stays `&default_thunk` at `:332` and every `sample()` pays an indirect call plus the pure virtual `read_points` at `:292` through `:321-325`; give each shipped provider's window constructor a `set_thunk` naming its own read function, or amend `specs/007-counters-and-timers/plan.md:33` and `:43` so the recorded P0 claim stops calling the loop dispatch-free. T082 is marked done and took neither branch (FR-022, `contradicts`)
- [X] T147 Reconcile FR-024 at `specs/007-counters-and-timers/spec.md:249`, which requires shared target and clock identity validated at construction, with `source/counters/detail/core.hpp:105`, where the plan carries one `bound_target` and no cross-member identity check exists anywhere in `source/counters/`; either validate and return a recoverable error naming the mismatched members, or amend FR-024 and T043 at `specs/007-counters-and-timers/tasks.md:159` to the single-plan-target design. T086 is marked done and took neither branch (FR-024, `missing`)
- [X] T148 Bind the fast mapped-page context to its opening thread: `fast_context::owner` is written at `source/counters/linux_pmu/fast_read.cpp:230`, declared at `source/counters/detail/pmu.hpp:276`, and read nowhere, while `source/counters/detail/pmu.hpp:212` comments a foreign-thread verdict that no code produces; compare `context->owner` inside `fast_context_read` and inside the enabled and running pair read, or delete the comment at `pmu.hpp:212` and state at `pmu.hpp:269-270` that the fast context carries no binding (FR-040, US7 scenario 5, `missing`)
- [X] T149 Replace the assertions that cannot fail: the literal `check(true, ...)` at `test/source/counters_fake_test.cpp:370`, and the three struct-echo tests at `test/source/counters_core_test.cpp:109-143` that read back members of a literal the test itself just wrote; each must assert a fact production code produces, and the move case at `test/source/counters_fake_test.cpp:367-369` must fold a metric over the moved plan (US1 scenario 5, FR-021, `partial`)
- [X] T150 Register the push provider beside the PMU provider in `test/source/counters_pmu_test.cpp`, which references `push_provider` zero times, so the catalog under PMU presence is proven for clocks at `:261-262` and for no push counter; assert that every push leaf stays `countable` with the PMU provider registered (US6 scenario 4, SC-002, `missing`)
- [X] T151 Make the examples a gate: `.github/workflows/ci.yml` names neither `counters_standalone_example` nor `counters_giraffe_example`, and its `ldd` steps at `:193-213` audit the five vendored dependencies alone, so the SC-001 example is compiled by CI, never run, and its link manifest never checked; register both examples with `add_test` and add a link-manifest step asserting the standalone example's `readelf -d` names the platform C and C++ runtime and no `speedgun-ng` entry (SC-001, FR-049, `partial`)

### MEDIUM

- [X] T152 Sum exponents in `leaf_sign` at `source/counters/fold.cpp:78-91` where the function multiplies per-occurrence signs, so a leaf appearing on both sides of a subtraction, as in `(a - b) / (a + b)`, takes exponent zero and the composite ratio product at `:139-166` reports `ratio^0`; add the case to `test/source/counters_fake_test.cpp` beside the ratio-product check at `:710-740` (FR-019, `contradicts`)
- [X] T153 Reconcile FR-034 at `specs/007-counters-and-timers/spec.md:262`, which calibrates the `tsc` leaf at system-open, with `source/counters/clock_provider.cpp:192-205`, where the calibration runs in the provider constructor because the catalog freezes at the open boundary; either move the calibration behind that boundary or amend FR-034 to name the construction boundary. T127 is marked done and took neither branch (FR-034, `contradicts`)
- [X] T154 Remove the platform-concept name from the core vocabulary header, where `include/speedgun-ng/counters_measurement.hpp:45` names a core-PMU group in the cadence idiom, and `test/counters_header_purity.sh:37-46` scans four terms so the one real FR-010 hit inside the core headers stays invisible; extend the scan across the three core headers with the `pmu` term added, or record in the script why they are exempt (FR-010, `partial`)
- [X] T155 Reconcile the scope-misuse edge case at `specs/007-counters-and-timers/spec.md:196`, which lists registering a composite into a started scope as a contract violation, with the shipped `scope` at `source/counters/plan.cpp:311-360`, which exposes `start`, `finish`, `view`, and `metric` and no registration entry point, so the misuse is unrepresentable; amend the edge case to the three enforceable sequences. T089 is marked done and took neither branch (FR-046, `partial`)
- [X] T156 Replace the four assertions in `test/source/counters_linux_pmu_seam_test.cpp` that cannot fail: `:352-355` reads back the declared defaults of a default-constructed context, `:381-382` compares a pure function of the mapfile with itself, `:394` compares a count against a size the parser cannot produce, and `:976-977` prints the fast-window outcome with no assertion at all; each must fail when the behaviour its label names regresses (FR-040, FR-038, US7 scenario 2, `partial`)

### LOW

- [X] T157 Resolve the load-context contradiction in `docs/pages/counters-overhead.md:53-70`, which records a 0.75 load average and four agents building and editing during the pass, against the table captions at `:81` and `:90` that call the same figures three runs on an idle tree; state which load produced the published numbers, because Principle VII bars a contended measurement from standing as a platform baseline (Constitution VII, `partial`)
- [X] T158 Point the umbrella at the fourth shipped provider: `include/speedgun-ng/counters.hpp:14-20` is documented as the single include and omits `counters_pmu.hpp`, which carries no platform guard of its own, so a Linux reader of the standalone entry point finds no route to `pmu_provider`; add the include or a documented sentence naming the header (FR-042, plan: Public API surface, `partial`)
- [X] T159 Carry the SC-002 caveat into the verdict it contradicts: `specs/007-counters-and-timers/quickstart.md:138` records PASS beside a recorded zero `permission_blocked`, while `docs/pages/counters-overhead.md:221-223` records that US6 scenario 4's `permission_blocked` expectation does not reproduce at level 2 on this kernel; the verdict column must name the unexercised path (SC-002, US6 scenario 4, `contradicts`)
- [X] T160 Correct T025's allocation claim at `specs/007-counters-and-timers/tasks.md:88`, which says the arena is allocated at plan finalization, where `source/counters/plan.cpp:223` and `:239` allocate one arena per call inside `plan::recorder` and FR-029 at `specs/007-counters-and-timers/spec.md:254` says construction. T126 is marked done and the wording stands (T025, `contradicts`)

## Phase 15: Convergence

Appended by `/speckit.converge` after an audit of the `8848f4e` completion claims
against the code, with the feature's own requirements as the inventory. Nothing above
this line changed.

Audit evidence, all produced by this pass with the committed `flags-gcc-clang` set
verbatim (`-Werror`, `-Wold-style-cast`, `-Wconversion`, contracts `enforce`): a fresh
`build/c2` tree configured and built to exit 0; `ctest --test-dir build/c2` passes 37
of 37 with 0 failed and 0 skipped in 52.43 s, which includes the two example tests
`8848f4e` registered; `dbc-gate` reports 135 interfaces with 0 gaps in the doc and the
pair gate; `format-check` exits 0; `cmake -P cmake/spell.cmake` exits 1 with 20
findings; `prose_gate.py --check prose --mode tree` collects its files with `git ls-files` and reads them from the working tree, so no commit reproduces its total (`tasks.md:1112-1119`), and its findings over the
sources, 2 of them in `include/speedgun-ng/` and none in a counters header, so that
debt is pre-existing and outside this feature's boundary. A second tree, `build/cov`,
ran the coverage preset: `ctest` passes 37 of 37 and the `coverage` target fails with
lines 1954 of 1956 (99.9%), branches 699 of 703 (99.4%), and functions 289 of 295
(98.0%), naming `source/counters/linux_pmu/group_io.cpp:324`,
`source/counters/linux_pmu/table_parse.cpp:400`, and the branch edges
`source/counters/fold.cpp:175`, `source/counters/linux_pmu/fast_read.cpp:318` and
`:323`, `source/counters/linux_pmu/table_parse.cpp:399`.

Coverage of the check: 109 requirement keys (50 FR, 10 SC, 49 user-story acceptance
scenarios) with 17 spec edge cases, 19 plan decision keys (R-001 through R-015 and the
four registered P2 exceptions), and 20 constitution clauses. Thirteen findings: 6
`contradicts`, 4 `partial`, 3 `missing`; 5 CRITICAL, 1 HIGH, 6 MEDIUM, 1 LOW, the five
CRITICAL ones being the red coverage and spell gates, which Principle VI and Principle
VIII make hard.

This phase covers three classes. The coverage gate, where the test rewrite in
`8848f4e` deleted three covering calls (T161, T164, T165) and one exclusion marker
sits on a line the trace never records (T163), with the configuration carrying the
whole remedy in every case, spell included (T162). One memory-safety regression the
new fold kind introduced, where a scale over an empty spine compiles and then reads
past the end of the node vector (T166). And the amendment class, where the artifact
text this wave added cites line ranges that hold other code (T171), two requirements
promise a diagnostic and a dispatch-free loop the code does not deliver (T167, T168),
the new node kind has one untested position (T169), and the cadence idiom publishes
figures the measurement page does not (T170, T172).

The coverage attribution in the `8848f4e` body is corrected here: its two uncovered
lines are byte-identical to the pre-change tree, and the commit added no uncovered
line, yet the same commit's test rewrite removed the only calls that covered
`source/counters/linux_pmu/table_parse.cpp:399-400` and the two else edges at
`source/counters/linux_pmu/fast_read.cpp:318` and `:323`. The pre-change tree
therefore measured those three sites, and the commit body reports the line axis only.

T154 was open when this pass ran, and the pass re-checked it and left it open. At
that point the platform-concept name stood at
`include/speedgun-ng/counters_measurement.hpp:45`, `test/counters_header_purity.sh:10-19`
scanned four terms and recorded `pmu` as exempt, and the umbrella include added at
`include/speedgun-ng/counters.hpp:18` put the provider header behind the same
documented single include, which strengthens the case for the scan's second branch.
T154 was neither closed nor superseded by this pass.

### CRITICAL: the red hard gates

- [X] T161 Restore the mapfile cache-hit assertion in `mapfile_scenario` at `test/source/counters_linux_pmu_seam_test.cpp:383`, which `8848f4e` replaced with a `pmu_load_table` assertion at `:397-406` and which was the only call covering `source/counters/linux_pmu/table_parse.cpp:399-400`; call `pmu_select_directory(identified)` a second time for the same identification and assert the result equals `directory`, so the line and the branch edge the coverage gate names are measured again and FR-038's "parsed once, lazily" has its covering test (FR-038, US6 scenario 2, `missing`)
- [X] T162 Add `copyable,deque,behaviour,judgement,licence,serialisation` to `ignore-words-list` at `.codespellrc:14`, which clears all 20 findings `cmake -P cmake/spell.cmake` reports (verified against a config copy, whole tree clean), and leave every correct English spelling in place: `copyable` and `deque` are codespell false positives no text change removes, and four of the remaining words sit in `sg_counters.md`, `data-model.md`, and `research.md`, which are closed artifacts, so the gate configuration is the remedy (Constitution VIII, `partial`)
- [X] T163 Re-anchor the existing exclusion for the unstable-retry statement at `source/counters/linux_pmu/group_io.cpp:325`: the marker sits on a line the trace never records while the executable line is `:324` (`DA:324,0` with no `DA:325` entry in `build/cov/coverage.info`), so put the marker where lcov attributes the line or collapse the `static_cast<void>(...)` call onto one line, keeping the kernel-seqlock reason already stated at `:313-318` and adding no new exclusion, because the arm is unreachable for a deterministic test (Constitution VI, `partial`)
- [X] T164 Cover the close of a context that owns nothing in `context_open_refusal_scenario` at `test/source/counters_linux_pmu_seam_test.cpp:336-371`, where `8848f4e` deleted the assertion and left its rationale comment at `:351-352` describing the removed behavior, and where the else edges of `source/counters/linux_pmu/fast_read.cpp:318` and `:323` now stand uncovered; a `fast_context` with `fd == -1` and `map == nullptr` must pass through `fast_context_close` and be asserted unchanged, in an arm that runs whether or not the host grants a mapped page (FR-040, `missing`)
- [X] T165 Cover the full-rate pair beside the frozen and shared fixtures at `test/source/counters_fake_test.cpp:211-243`, adding a scripted leaf whose `enabled` and `running` deltas are equal over the folded window so `source/counters/fold.cpp:175` takes its `*one < 1.0` false edge; the fold must then disclose `running_ratio` 1.0 with `scaled` false, which is the ordinary un-multiplexed disclosure no fixture reaches today (FR-019, `missing`)

### HIGH

- [X] T166 Refuse the zero-leaf spine at construction: `scale_all` at `include/speedgun-ng/counters_measurement.hpp:140-144` adds its kind-4 node unconditionally, so `2.0 * expression<events>{}` builds a one-node spine whose `left` is `-1`, passes the `core->empty()` refusal at `source/counters/plan.cpp:409` because that test reads the node vector, compiles, and then trips the point-sink precondition at `include/speedgun-ng/counters_provider.hpp:192` on the first `sample()` (reproduced, exit 134), while at contracts `ignore` `source/counters/fold.cpp:52-53` evaluates `core.nodes[-1]`; make the construction refusal test `core->leaves.empty()` (or add a precondition in `scale_all`) so a zero-leaf expression gets the construction-time recoverable error the edge case names, and add the case to `test/source/counters_fake_test.cpp` (spec edge case, FR-046, `contradicts`)

### MEDIUM

- [X] T167 Name the member at the open refusal: the amended FR-024 at `specs/007-counters-and-timers/spec.md:252` and its clarification at `:34` both promise that a target the kernel refuses to open is a recoverable construction error "naming the member at fault", while `source/counters/plan.cpp:496-500` emits "provider cannot open a window for its leaves (FR-011)" and the nine nullptr returns in `source/counters/linux_pmu/group_io.cpp` carry no member; either carry the refusing address out of `open` into the message, or amend both artifact sentences to what one diagnostic can say (FR-024, US6 scenario 7, `partial`)
- [X] T168 Reconcile FR-022 at `specs/007-counters-and-timers/spec.md:250` with the constraints at `specs/007-counters-and-timers/plan.md:33` and `:256`, which claim a read path carrying no dynamic dispatch, and with the fallback `8848f4e` documented at `include/speedgun-ng/counters_provider.hpp:245-251` and implemented at `:326-337`: a provider installing no thunk reaches `read_points` through the vtable, and `example/counters_giraffe_example.cpp:46` is such a provider, so amend the requirement to name the seam and its per-action cost while the five shipped windows stay dispatch-free (FR-022, US5 scenario 1, `contradicts`)
- [X] T169 Cover the scaled expression as a spliced operand: `splice` at `include/speedgun-ng/counters_measurement.hpp:157-178` remaps `left` for every node kind and the new kind-4 node created at `:142-143` reaches a non-root position as soon as `operator+` or `operator/` takes it as an operand, yet `test/source/counters_fake_test.cpp:927` and `:957` fold a scaled spine only as its own root; add a case composing `(k * a) op b` and assert the folded value, so a remap regression on the unary node is measured (Constitution VI, US1 scenario 3, `partial`)
- [X] T170 Restate the cadence figures at `include/speedgun-ng/counters_measurement.hpp:43-48`, which publish "60 ns per action for a clock plan and 210 ns for a core-PMU group" and derive 120 ns and 420 ns of K=1 cost, against `docs/pages/counters-overhead.md:91-92` and `:100-101` whose medians are 30 and 50 in release and 70 and 170 in dev, whose fold-window line at `:105-107` is where 60 ns comes from, and whose 210 at `:127` is one run's maximum; re-run the overhead benchmark, cite the regenerated per-action medians with the build they came from, and recompute the K=1 consequence, since the thread checks `8848f4e` added to the mapped-page read also move the dev figures (FR-048, Constitution VII, `contradicts`)
- [X] T171 Correct the five line citations the amendments add, each of which lands on other code: `specs/007-counters-and-timers/spec.md:34` names `source/counters/detail/pmu.hpp:284` for `leader_pid` (declared at `:292`) and `include/speedgun-ng/counters_measurement.hpp:1084` for `compile` (declared at `:1085`); `:35` names `source/counters/clock_provider.cpp:192-205` for the calibration, which runs in the constructor at `:206-235`; `:199` names `source/counters/fold.cpp:270-271` for the metric-not-closed guard, whose `SG_REQUIRE` is at `:285`; and `specs/007-counters-and-timers/quickstart.md:138` names `docs/pages/counters-overhead.md:225-228` for the level-3 refusal stated at `:231-232` and `:206-214` for the 358 countable entries tabulated at `:219`; every amended sentence must cite the line holding its claim (Constitution IV, `contradicts`)

### LOW

- [X] T172 Restate the two comments that still describe the removed per-leaf scaling: `test/source/counters_fake_test.cpp:912-914` says "the scale reaches the leaves and leaves the arithmetic node alone" and `:938` asserts "scales every leaf exactly", while `8848f4e` moved the scale to one node over the spine root at `include/speedgun-ng/counters_measurement.hpp:135-144`; the assertion stands, its stated mechanism must match the fold (Constitution IV, `contradicts`)

## Phase 16: Convergence

Appended by `/speckit.converge` after an audit of the `e40f55e` completion claims and
of the five residuals the second wave left unfixed. Nothing above this line changed.

Audit evidence, all produced by this pass. A fresh `build/c3` tree configured and
built to exit 0 with the committed `flags-gcc-clang` set verbatim (`-Werror`,
`-Wold-style-cast`, `-Wconversion`, `-Wnull-dereference`) at contracts `enforce` in
`Debug`; `ctest --test-dir build/c3` passes 37 of 37 with 0 failed and 0 skipped in
52.73 s; `dbc-gate` reports 135 interfaces with 0 gaps in the doc and the pair gate;
`format-check` exits 0; `cmake -P cmake/spell.cmake` exits 0; `prose_gate.py --check
all` exits 0; the coverage gate run
directly on `build/coverage/coverage.info` exits 0 at lines 100.0% (1955 of 1955) and
branches 100.0% (703 of 703). Functions sit at 98.0% (289 of 295), an axis no gate
scores. No source
is newer than that trace. A second tree, `build/c3-ciubuntu`, configured with the
project's own `ci-ubuntu` preset, reports `CMAKE_BUILD_TYPE=Release` with the full
strict set and contracts `enforce`, and its library build FAILS:
`source/counters/plan.cpp:489:42: error: potential null pointer dereference
[-Werror=null-dereference]`. A third tree, `build/c3-rel` at `Release` with contracts
`ignore`, fails on that same dereference plus two unused locals. Every earlier wave's
evidence was the Debug `dev` preset, and GCC's null-dereference analysis runs from
`-O1` upward, so a red CI preset went three waves unseen.

Coverage of the check: 109 requirement keys (50 FR, 10 SC, 49 user-story acceptance
scenarios) with 17 spec edge cases, 19 plan decision keys (R-001 through R-015 and the
four registered P2 exceptions), and 20 constitution clauses. Nine findings: 5
`contradicts`, 3 `partial`, 1 `missing`; 1 CRITICAL, 3 HIGH, 4 MEDIUM, 1 LOW.

This phase covers the release-configuration class, the sites where a fix from an
earlier wave landed at one occurrence and left its sibling, and the citation class the
last commit re-broke while repairing it. One unchecked nullable dereference at
`source/counters/plan.cpp:489` that stops the project's own `ci-ubuntu` preset from
building (T173). One configuration the committed `Release` measurement depends on,
which no preset and no CI job builds and which does not compile under the committed
warning set (T174). T166's leaf-count refusal, landed at
`source/counters/plan.cpp:413` and absent from its sibling at `:531` (T175). T169's
covering test, whose assertion cannot fail on the remap regression it names (T176).
And the citation class: a coverage exclusion whose justification cites two line ranges
holding other code (T177), two `quickstart.md` references stale against the page the
same commit rewrote (T178), a `spec.md` reference moved onto an empty doxygen line
(T179), two artifact sites still publishing the superseded per-action medians (T180),
and a `coverage` target that exits non-zero on a perl module nothing declares (T181).

Verdicts on the five residuals, each re-derived from the code. Residual 1 is real and
reaches above the second wave's reading: the second wave saw a `-Wnull-dereference`
error at `-O3` and set the site down as unreachable, and the dereference is
unreachable in practice because `source/counters/plan.cpp:428-436` resolves every
address before the frozen tree is read again at `:486-497`, yet
`cmake --preset=ci-ubuntu` configures `Release` with `-Werror` and the `test` job
builds `build --config Release`, so the gate is red and the finding is CRITICAL
(T173). Residual 2 is real and reproduced: the three locals at
`source/counters/fold.cpp:209`, `source/counters/fake_provider.cpp:152`, and
`test/source/dbc_test.cpp:65` are referenced only from contracts `ignore` elides, so
the configuration is owed a preset or a CI job and the three sites owe live operands
(T174). Residual 3 is real and larger than a misleading diagnostic: the node-count test
T166 removed at `:413` still stands at `:531`, so the fix is partial (T175). Residual 4
is an environment fact carrying a documentation debt: the gate runs at step 4 of
`cmake/coverage.cmake:78-88` and prints its verdict before step 5 fails, so the
Principle VI verdict stands and the debt is the undeclared module (T181). Residual 5
holds as the second wave recommended: `specs/007-counters-and-timers/spec.md:30` and
`specs/007-counters-and-timers/quickstart.md:140` still carry 60 ns and 220 ns while
the page publishes 40 and 70 in `Release` and 70 and 180 in the correctness build, so
the amendment is emitted as T180.

### CRITICAL: the primary Linux CI preset does not build

- [X] T173 Remove the unchecked nullable dereference at `source/counters/plan.cpp:489`, where `impl.find(object_path)` returns a `tree_node*` and the loop over `node->leaves` consumes the result with no check, and make `cmake --preset=ci-ubuntu` build to exit 0: that preset configures `CMAKE_BUILD_TYPE=Release` with the committed `flags-gcc-clang` set including `-Werror` and `-Wnull-dereference` at contracts `enforce`, the `test` job in `.github/workflows/ci.yml:116-118` configures and builds it, and the library build currently fails with `plan.cpp:489:42: error: potential null pointer dereference [-Werror=null-dereference]`, which no `Debug` build reaches; the per-provider slot loop must consume the resolution the leaf loop at `:428-436` already performed, so no nullable result crosses the loop boundary (the `pending_leaf` entries at `:400-404` are the only state that crosses), or the loop must carry an explicit check returning the `FR-017` error the first loop emits at `:438-443`, and no warning class may be demoted to reach a green build (Constitution VIII, FR-024, `contradicts`)

### HIGH

- [X] T174 Make `Release` with `speedgun-ng_CONTRACTS=ignore` compile under the committed warning set with nothing demoted, and name the configuration the project owes: `source/counters/fold.cpp:209` (`column`), `source/counters/fake_provider.cpp:152` (`scripted`), and `test/source/dbc_test.cpp:65` (`counting_predicate`) are referenced only from contracts `ignore` elides, so that combination exits 2 on two `-Werror=unused-variable` and one `-Werror=unused-function` error, and the `Release` figures at `docs/pages/counters-overhead.md:30-44` were taken with those classes demoted in a private tree; each of the three sites must keep its operand live outside the elided contract, and `specs/007-counters-and-timers/plan.md`'s Constraints must name the build configurations that must compile with the committed `flags-gcc-clang` set so a preset or a CI job carries them, and no `-Wno-error=` flag and no removed warning class is the remedy (Constitution VIII, plan Technical Context: Constraints, `missing`)
- [X] T175 Apply T166's leaf-count test at the sibling refusal `source/counters/plan.cpp:531`, where `compile_fanout_core` still calls `exemplar.empty()` and `expr_core::empty()` at `include/speedgun-ng/counters_measurement.hpp:117` returns `nodes.empty()`, so a scaled zero-leaf fan-out exemplar passes the node-count guard the way it passed the one T166 fixed at `:413`, falls through to `exemplar_prefix` at `:541`, and returns "fan-out exemplar spans several objects (FR-024)" for a spine holding no leaf; the guard must test `exemplar.leaves.empty()`, and the case must be added to `test/source/counters_fake_test.cpp` beside the zero-leaf refusals at `:543-556`, where no fan-out exemplar case stands today and `compile_fanout` reaches this path unrepresented (T166, spec edge case "Zero-leaf expression or empty plan", FR-046, `partial`)
- [X] T176 Give T169's spliced-scaled-operand case a scaled leaf whose delta differs from every other leaf in its composition, because `test/source/counters_fake_test.cpp:157` scripts `only` and `:159` scripts `denominator` to the identical `{0, 200}` with tail 200, so a kind-4 node whose `left` fails to remap at `include/speedgun-ng/counters_measurement.hpp:174-176` lands on `only`, carries the same delta, and leaves the value asserted at `:1067` unchanged at `5.75`; the assertion must change value when the remap is wrong, which means scaling `numerator` (2100 at `:158`) or adding a fourth leaf with a delta of its own (T169, Constitution VI, US1 scenario 3, `partial`)

### MEDIUM

- [X] T177 Re-anchor the justification at `source/counters/plan.cpp:509-513`, whose `LCOV_EXCL_BR` block excludes the "leaf has no owning provider" arm and cites `plan.cpp:402-415` for the first match and `plan.cpp:461-467` for the re-find, where `:402-415` holds the `pending_leaf` struct tail, the loop head, and the zero-leaf refusal, and `:461-467` holds the loop tail, the `plan_impl` allocation, and the target binding; the two sites the reason names are `:428-436` and `:486-497`, and a coverage exclusion whose stated reason does not hold at the lines it cites is the class the T066 audit removed from this tree (Constitution VI, plan Complexity Tracking, `contradicts`)
- [X] T178 Re-cite the two `docs/pages/counters-overhead.md` references in the SC-002 verdict row at `specs/007-counters-and-timers/quickstart.md:138`, which T171 corrected against the page as it stood before `e40f55e` rewrote it in the same commit: `:219` must become `:303`, the level-2 row of the privilege table now at `:301-304` that tabulates 358 `countable` and 0 `permission_blocked`, and `:231-232` must become `:315-316`, the level-3 refusal now there, where `:219` reads `single:  120.0,130.0,160.0` inside the superseded pass and `:231` is the `## Fast regime` heading (T171, Constitution IV, `contradicts`)
- [X] T179 Re-cite `compile` at `specs/007-counters-and-timers/spec.md:34`, where T171 moved the reference from `include/speedgun-ng/counters_measurement.hpp:1084` to `:1085` and `:1085` is the empty doxygen line ` *`, so the correction moved the citation off the brief at `:1084` and onto a line holding no claim; the declaration is at `:1089-1095` (T171, Constitution IV, `contradicts`)
- [X] T180 Amend the two artifact sites still carrying the superseded per-action medians after T170 restated the header: `specs/007-counters-and-timers/spec.md:30` publishes "60 ns for a clock plan, 220 ns for a core-PMU group" and derives 0.6 ns and 2.2 ns from them, and the SC-004 evidence column at `specs/007-counters-and-timers/quickstart.md:140` reads "clock 60/70/100, pmu single fast_rdpmc 120/130/160, pmu group fast_rdpmc 170/170/200, fold 584.7", which `docs/pages/counters-overhead.md:217-221` now records under the pass that page marks superseded, while its current correctness-build medians at `:159-162` are clock 70, group 180, single 130 and a 520.4 ns fold; both sentences must cite the regenerated per-action medians with the build configuration they came from, matching the cadence comment at `include/speedgun-ng/counters_measurement.hpp:44-56` (FR-048, Constitution VII, T170, `contradicts`)

### LOW

- [X] T181 State the `genhtml` runtime prerequisite the coverage target needs and its configure-time check cannot see: `cmake/coverage.cmake:15-23` finds the `genhtml` executable and fails configuration when the executable is absent, so the `GD.pm` module the executable loads stays invisible to that check, and `cmake --build build/coverage -t coverage` exits 2 on this host after the gate at `:84` has printed its verdict, which a target named for a Principle VI hard gate should not do without a stated cause; the prerequisite belongs in `README.md` beside the vendored-dependency prerequisites the file already documents, and no step may be dropped or skipped to keep the target quiet (Constitution VIII, plan: Determinism and regression, `partial`)

## Phase 17: Convergence

Appended by `/speckit.converge` after an audit of the `afd851e` completion claims and of
the release-configuration class the previous wave closed. Nothing above this line changed.

Audit evidence, all produced by this pass. Two fresh trees in `build/`, each configured
with the committed `flags-gcc-clang` set verbatim from `plan.md:33`. `build/c4-rel` at
`Release` with contracts `ignore` builds the whole tree to exit 0 with zero errors and
zero warnings in the project's own C++ sources; the five warnings in its log are
`-Wdiscarded-qualifiers` in the vendored hwloc C sources, which the project does not own.
`ctest --test-dir build/c4-rel -j 8` passes 34 of 37 in 43.49 s; the three that fail are
`speedgun-ng_test`, `dbc_test` and `dbc_trap_checked`, the DBC facility's own tests, which
assert that a gated check delivers its violation and therefore cannot pass where `ignore`
elides it. `.github/workflows/ci.yml:444` encodes the same fact, asserting
`test "$status" -eq 1` for `dbc_test` under `ignore`, and the job runs a curated subset
(`:424-430`) for the same reason. `build/c4-dbg` at `Debug` with contracts `ignore` and
the same set builds to exit 0; its `dbc-gate` target reports 135 interfaces with 0 doc
gaps and 0 pair gaps, and `format-check` exits 0. A coverage tree configured and built
outside the workspace, then measured, reports lines 100.0% (1955 of 1955), branches
100.0% (705 of 705) and functions 98.0% (289 of 295), and `coverage_gate.sh` exits 0.
`python3 tools/prose/prose_gate.py --check all` exits 0;
`--mode tree` collects its files with `git ls-files` and reads them from the working tree, so no commit reproduces its total (`tasks.md:1112-1119`), and its findings
of which `test/source/dbc_test.cpp:133` and `:437` are the two this feature's range does
not see. `cmake -P cmake/spell.cmake` exits 0.

Two corrections to the record this pass established. The coverage figure Phase 16's
preamble reports at `:608`, branches 100.0% (703 of 703), came from
`build/coverage/coverage.info` at mtime 21:53:14, while the three sources `afd851e`
changed carry mtimes 22:12:52 (`source/counters/plan.cpp`), 22:14:52
(`source/counters/fold.cpp`) and 22:19:08 (`source/counters/fake_provider.cpp`); the
trace's own records carry execution counts on lines that now hold a comment and a closing
brace. The current tree measures 705 of 705 branches and the gate exits 0, so the verdict
stands and the figure on record did not measure this tree. The two prose findings in
`test/source/dbc_test.cpp` are ruled out of this feature: `afd851e` touched
`:58-73` and `:120-130`, Principle XI.1 scopes the rule to the lines a change
touches, and the tree carries findings of that class across its sources, so a
sweep is a formatting-only change under V on its own. The gate's `--mode tree`
form collects its files with `git ls-files` and reads them from the working
tree, so no commit reproduces its total
(`specs/007-counters-and-timers/tasks.md:1112-1119`).

Coverage of the check: 109 requirement keys (50 FR, 10 SC, 49 user-story acceptance
scenarios) with 17 spec edge cases, 20 plan decision keys (R-001 through R-015, the four
registered P2 exceptions, the Constraints configuration enumeration, and the five
Complexity Tracking rows), and 21 constitution clauses. Four findings: 1 `contradicts`,
3 `partial`; 1 CRITICAL, 1 HIGH, 2 MEDIUM.

Three answers the previous wave's CRITICAL demands, each from the job list. First, the
release configuration is covered by gates that run on every change. The `test` job
(`.github/workflows/ci.yml:105`) configures `ci-ubuntu` (`:116`), which is `Release`
(`ci-linux`), contracts `enforce`, the committed set, plus clang-tidy and cppcheck, and
builds the whole tree and its test targets before `ctest` (`:118`, `:163`);
`test-rocky` (`:165`, `:183`) configures `ci-rocky` to the same configuration; and
`sanitize` (`:71`, `:87`) builds `ci-sanitize` at `-O2` with the same set, so an
optimizer-backed analysis runs in three jobs rather than one. The `dbc-gate` (`:465`) and
`prose-lint` (`:528`) jobs configure the same `Release` preset but build gate targets
only. Second, no CI job builds `Release` at contracts `ignore` with the committed set.
The only two configurations at that semantic are the `consumer-release` job's, and
`.github/workflows/ci.yml:301-306` and `:331-336` configure them with a bare
`cmake -S . -B`, which leaves `CMAKE_CXX_FLAGS` empty in both caches and with it the
whole warning set, the fortified release flags and the hardened linker flags; the
dev-mode-on tree then builds four named targets (`:338-340`, `:427-428`). The
pre-correction `source/counters/fold.cpp` at contracts `ignore` with the committed set
fails on `unused variable 'column'`, and the identical source with that set absent
compiles clean, so the three sites T174 repaired can regress with every job green, and
`plan.md:33` names a set of configurations nothing enforces (T182). Third, the `dev`
preset stays `Debug` (`CMakeUserPresets.json:22`) and that is the right default for the
loop it serves: it already carries the complete committed set through `ci-linux`, and the
project's `Release` gate is the `test` job's `ci-ubuntu`, so nothing is lost at `-O0`. The
defect sits in the verification instruction, and the preset keeps its place, because
`.specify/memory/constitution.md:279-287` and `AGENTS.md:43` name `ctest --preset=dev` as the
whole of a task's verification, and that loop cannot see what three waves of this feature
missed (T184).

This phase covers the enforcement class, the measurement record the enforcement gap left
behind, and the verification instruction. One configuration the plan names and CI does
not build, carrying neither the warning set nor the hardening flags the constitution
requires of it (T182). One page that still tells a reader to take the measurement with
three warning classes demoted, and still claims a compile state `afd851e` ended, with two
citations that now name other code (T183). And the per-task verification loop, which
compiles nothing above `-O0` (T184).

### CRITICAL: the release artifact configuration carries no committed flags

- [X] T182 Carry the committed `flags-gcc-clang` set into the two `Release` at contracts `ignore` configurations the `consumer-release` job builds: `.github/workflows/ci.yml:301-306` and `:331-336` configure them with a bare `cmake -S . -B`, so `CMAKE_CXX_FLAGS`, `CMAKE_CXX_FLAGS_RELEASE` and both linker flag variables are empty in the resulting caches and no job in the matrix compiles that semantic with the committed set, which leaves `source/counters/fold.cpp:209`, `source/counters/fake_provider.cpp:152` and `test/source/dbc_test.cpp:69` free to regress into the `-Werror=unused-variable` and `-Werror=unused-function` errors T174 repaired, and leaves the audited release library without the stack protector, control-flow protection, stack-clash protection, fortified release flags and hardened linker flags Additional Constraints requires of it; add one hidden preset to `CMakePresets.json` inheriting `ci-linux` with `speedgun-ng_CONTRACTS: ignore`, use it for both trees with `-B` as `prose-lint` already does at `:528`, and no warning class may be demoted, no `-Wno-error=` flag added and no build step dropped, so `plan.md:33`'s claim names configurations the pipeline builds (Constitution VIII, Additional Constraints: Warnings and hardening, `plan.md:33`, T174, `missing`)

### HIGH: the measurement record still carries the defect and the old citations

- [X] T183 Restate `docs/pages/counters-overhead.md:30-44`, which adds `-Wno-error=unused-variable -Wno-error=unused-function -Wno-error=null-dereference` to the release tree at `:32-33` and still claims at `:33-35` that the committed source carries four findings under `Release` with contracts `ignore`, all four ended by `afd851e` and the pre-correction `source/counters/fold.cpp` verified above to compile clean in that configuration now: the three demotions must leave the recipe, the claim must become that the configuration compiles with the committed set and nothing demoted, and the paragraph must record that the published medians stand because a diagnostic-severity demotion changes no codegen; the citation at `:39` must move to `test/source/dbc_test.cpp:69` where `counting_predicate` now sits, and the sentence at `:40-43` must name the tree lookup at `source/counters/detail/core.hpp:170` on the system impl, because `plan_impl` carries no `find` member and the dereference it describes is gone, while `spec.md:30` and `quickstart.md:140` derive the cadence figures from this page (Constitution IV, VIII, FR-048, `specs/007-counters-and-timers/spec.md:30`, `contradicts`)

### MEDIUM: the per-task verification compiles nothing above `-O0`

- [X] T184 Name a `Release` compile in the per-task verification instruction: `.specify/memory/constitution.md:279-287` and `AGENTS.md:43` both define a task's whole verification as `ctest --preset=dev`, and that preset is `Debug` (`CMakeUserPresets.json:22`), so the optimizer-backed analyses that found the unchecked nullable dereference that kept `ci-ubuntu` from compiling never run in the loop every task is verified through, and three convergence waves reported green gates while the configuration the `test` job builds had never compiled; the instruction must add the `ci-ubuntu` configure and build the `test` job already runs, and the `dev` preset keeps its place as the fast iteration loop because it carries the complete committed set through `ci-linux` (Constitution IX, VIII, `plan.md:33`, T173, `partial`)

## Phase 18: Convergence

Appended by `/speckit.converge` after an audit of the three most recent commits and of the
residue the four waves before them left. Nothing above this line changed.

Audit evidence, all produced by this pass. `build/c5-rel` at `Release` with contracts
`ignore` and the committed `flags-gcc-clang` set verbatim configures and builds to exit 0
with zero warnings in the project's own C++ sources (the six in its log are
`-Wdiscarded-qualifiers` in the vendored hwloc C tree, which the project does not own), and
`ctest --test-dir build/c5-rel` runs all 37 in 51.76 s with 3 failing by design:
`speedgun-ng_test`, `dbc_test` and `dbc_trap_checked` assert that a gated check delivers
its violation, which `ignore` elides, and `.github/workflows/ci.yml:442` encodes the same
fact by asserting `test "$status" -eq 1` for `dbc_test` under that semantic. A second tree
configured through the preset `2606a5a` added, `cmake --preset=ci-linux-ignore` with
`speedgun-ng_DEVELOPER_MODE=OFF`, carries the full set, the fortified release flags, the
hardened linker flags, `CMAKE_BUILD_TYPE:STRING=Release`,
`speedgun-ng_DEVELOPER_MODE:BOOL=OFF` and `speedgun-ng_CONTRACTS:STRING=ignore` in its
cache, so the three assertions at `.github/workflows/ci.yml:309-314` hold unchanged, and
that tree builds to exit 0. `dbc-gate` reports 135 interfaces with 0 gaps in both the doc
gate and the pair gate, `format-check` exits 0, and `cmake -P cmake/spell.cmake` exits 0.
The coverage trace is fresh and was checked before its figures were quoted:
`build/coverage/coverage.info` carries mtime 23:08:17, later than the newest source in the
tree (22:19:08, `source/counters/fake_provider.cpp`), it records execution counts on the
lines `afd851e` created at `source/counters/plan.cpp:492`, `:493` and `:496`, and its
branch total is 705, two edges more than the 703 the pre-`afd851e` tree measured, which is
what the new `continue` guard adds. `bash tools/dbc/coverage_gate.sh
build/coverage/coverage.info` exits 0 at lines 100.0% (1955 of 1955), branches 100.0%
(705 of 705), functions 98.0% (289 of 295) on an axis no gate scores.
`python3 tools/prose/prose_gate.py --check all` exits 0;
`--mode tree` collects its files with `git ls-files` and reads them from the working tree, so no commit reproduces its total (`tasks.md:1112-1119`), and
none of its findings sits in this feature's artifacts.

Verified line by line rather than taken from a commit body. The exclusion `afd851e`
re-anchored at `source/counters/plan.cpp:511-515` cites `:436`, `:442`, `:481`, `:492` and
`:496`, and each carries the statement the reason attributes to it. The build record
`2606a5a` wrote at `docs/pages/counters-overhead.md:35-55` cites
`source/counters/fold.cpp:209` (`column`), `source/counters/fake_provider.cpp:152`
(`scripted`), `test/source/dbc_test.cpp:69` (`counting_predicate`) and
`source/counters/detail/core.hpp:170` (`find`), and each holds it. All 20 `file:line`
citations the live artifacts place into a source or a header resolve to a non-blank line
and each was read, including `source/counters/fold.cpp:289-290` for the closed-window
refusal, `source/counters/plan.cpp:330` and `:344-345` for the scope guards, and
`source/counters/plan.cpp:469` and `:501` for the single-target binding. `readelf -d` and
`ldd` on the standalone example from the release tree name `libstdc++`, `libm`, `libgcc_s`
and `libc` and nothing else, which is what `spec.md:31` resolves FR-049 and SC-001 to. The
`pending_leaf` rewrite holds: both partition loops at `source/counters/plan.cpp:480-484`
and `:491-499` walk `pending` in order under one predicate, so a group's slot order matches
the `addresses` order its window opens on, and the carried pointer is sound because the
catalog is frozen at open and the only call between the loops is the provider's own
`open`. All three zero-leaf guards read the leaf vector (`:89`, `:417`, `:536`), so the
sibling `T175` fixed is the last of the three. Both tests `afd851e` added can fail: a
pre-`T175` fan-out exemplar passes the node-count guard, reaches `exemplar_prefix`
(`:546`), and returns the "spans several objects" message the new case excludes, and the
scaled operand's delta of 400 differs from every other leaf of its composition, so a
dropped remap moves 2.875. The `denominator` leaf the splice case stopped reading is still
read at `test/source/counters_fake_test.cpp:610-620`, so that change left no orphan. A
closed task's citations describe the tree it was written against, so `T184`'s citation
moved with the clause and now names `.specify/memory/constitution.md:279-287` at `:770`.

Coverage of the check: 127 requirement keys (50 FR, 10 SC, 49 user-story acceptance
scenarios, 18 spec edge cases), 32 plan decision keys (R-001 through R-015, the nine
Technical Context decisions, the three Complexity Tracking rows, the Structure Decision
and the gate-set note), and 21 constitution clauses with X.1 through X.4 and XI.1 through
XI.6 read one by one. Four findings: 3 `contradicts`, 1 `partial`; 1 HIGH, 2 MEDIUM,
1 LOW.

The two prose findings at `test/source/dbc_test.cpp:133` and `:437` are ruled out of this
feature on independent grounds. `git log -L` puts both lines at `da18283`, the commit that
created the file under spec-001, so no commit in this feature's range wrote them. The
scoping rule is the principle's own: `XI.1` states that a change brings the lines it
touches into compliance and a tree-wide sweep is a formatting-only change under `V`
scheduled on its own, and the only form the gate runs is the range, which
`.github/workflows/ci.yml`'s prose-lint job and `cmake -P cmake/prose-lint.cmake` both
reproduce. A two-line edit here would also violate `X.3`, and it would split one
sweep across two features while the rest of that class stayed in place. The
`--mode tree` form collects its files with `git ls-files` and reads them from
the working tree, so no commit reproduces its total
(`specs/007-counters-and-timers/tasks.md:1112-1119`).

Answer to the question the previous wave raised and left open. On the platform the amended
clause names, no configuration is unbuilt: `ci-ubuntu` is configured and built by the
`test` (`.github/workflows/ci.yml:116`), `dbc-gate` (`:463`) and `prose-lint` (`:526`)
jobs, and the two trees that previously carried no committed set take it through
`ci-linux-ignore` (`:303`, `:332`), verified above against a real cache. Two
configurations the constitution requires are still built by no job, `ci-macos` and
`ci-windows`, and both absences are recorded in the constitution itself, at the Open
deferral `.specify/memory/constitution.md:59-64` for the macOS runner and inside Principle
VIII at `:232-237` for the MSVC suspension, so they are deferrals with owners and spec-004
and spec-005 tasks already open under them. What the clause leaves behind is a text defect
instead of a missing build: its MUST names no platform while its prescription names one,
and `AGENTS.md:43-44` repeats that single preset four lines above the sentence at `:48-50`
naming macOS and Windows in the matrix (T187).

This phase covers the citation residue the measurement-page rewrite left in two artifacts
(T185), the plan sentence claiming a coverage the matrix does not give it (T186), the
platform-blind release-build clause the amendment added (T187), and the four
coverage-exclusion justifications whose cited lines hold other code (T188). The three
commits' own new code carries no finding: the amendment's Sync Impact Report, its lineage
row and its version line agree at 2.9.0 with the comment opening and closing once, the
preset the `consumer-release` job now names configures and that job's three cache
assertions hold under it, and the page's restated build record cites lines that hold what
it claims.

### HIGH: the measurement page moved and seven inbound citations did not

- [X] T185 Re-anchor the seven citations the two artifacts place into `docs/pages/counters-overhead.md`, which `2606a5a` moved eleven lines when it replaced the build record at `:30-55`, so each now names other code: `specs/007-counters-and-timers/spec.md:30` names `:149-150` for the release rows carrying 40 ns for the clock plan and 70 ns for the core-PMU group, which stand at `:160-161`, `:256-263` for the absence of a syscall-mode counterpart, stated at `:267-274`, and `:296-299` for the re-verification that a `perf_event_paranoid` 2 host probes a fast mechanism, stated at `:307-310`; `specs/007-counters-and-timers/quickstart.md:138` names `:303` for the level-2 row of the privilege table, which is `:314`, and `:315-316` for the level-3 refusal, stated at `:326-327`; and `specs/007-counters-and-timers/quickstart.md:140` names `:159-162` for the dev-configuration figures it quotes as clock 70/70/130, pmu single 129/130/170, pmu group 170/180/240 and fold 520.4, which stand at `:170-173` while the cited lines carry the release rows 40, 70 and 50, and `:203-226` for the superseded pass, which is `:214-237`; every amended sentence must cite the line holding its claim, and no figure, row, or wording of the page may move to make a citation fit, since the page is the record the cadence figures at `include/speedgun-ng/counters_measurement.hpp:44-56` derive from (Constitution IV, FR-048, T177, T178, T180, `contradicts`)

### MEDIUM: the plan claims a coverage two audit trees do not carry

- [X] T186 Make `specs/007-counters-and-timers/plan.md:33` true against `.github/workflows/ci.yml`: the sentence claims the committed `flags-gcc-clang` set governs every configuration the project builds with no class demoted, then enumerates six configurations, while `shared-audit` (`.github/workflows/ci.yml:203`) and `downstream-consumer` (`:264`) each configure a `Release` tree with a bare `cmake -S . -B` that leaves `CMAKE_CXX_FLAGS`, `CMAKE_CXX_FLAGS_RELEASE` and both linker flag variables empty; either extend the enumeration to name those two trees and state why the committed set is not owed there, since they audit the link and symbol surface of the release artifacts at the same contract semantic the `test` and `test-rocky` jobs already compile with it, or add one visible preset inheriting `ci-linux` without `dev-mode` and name it in both configure steps, which adds two non-developer-mode release builds carrying the full set at the roughly a quarter of a second per tree the `2606a5a` measurement recorded, and in either case no warning class may be demoted, no `-Wno-error=` flag added, and no build step dropped (Constitution VIII, Additional Constraints: Warnings and hardening, T174, T182, `partial`)

### MEDIUM: the amended clause names one platform and governs three

- [X] T187 Name the platform's release preset in the per-feature release build `.specify/memory/constitution.md:279-287` requires, where the MUST states no platform and the parenthetical names one: on macOS `cmake --preset=ci-ubuntu` configures the `Unix Makefiles` generator with the GCC and Clang flag set inherited through `ci-linux` and carries a `CMAKE_BUILD_TYPE` the `Xcode` generator of `ci-macos` (`CMakePresets.json:156-158`) ignores, so the build an agent on that platform runs is not the configuration the file calls that platform's release preset and Principle VIII's macOS clause names the other one; `AGENTS.md:43-44` carries the same single-preset wording four lines above the sentence at `:48-50` that names macOS and Windows in the matrix; the clause must name the preset per platform, must keep the Linux command `a6d26bf` measured as the Linux one, and must add no gate, job, or runner, and the macOS runner stays under the Open deferral at `.specify/memory/constitution.md:59-64` with `specs/004` T019 and `specs/005` T024 open under it (Constitution IX, VIII, `contradicts`)

### LOW: four exclusion justifications cite lines holding other code

- [X] T188 Re-anchor the four coverage-exclusion justifications whose cited lines hold other code, the class `T177` removed from this tree: `source/counters/plan.cpp:113` names `plan.cpp:420` for the construction-failure path, while the file's only `availability_name` call stands at `source/counters/plan.cpp:456`; `source/counters/plan.cpp:86` names `plan.cpp:391`, `compile_core`'s opening brace, for the zero-leaf refusal that stands at `source/counters/plan.cpp:417`; `source/counters/fold.cpp:114` names `plan.cpp:120-145` for `link_ratio_slots`, which spans `source/counters/plan.cpp:135-158` and writes the enabled/running pair at `:150-156`; and `source/counters/fold.cpp:158` names `plan.cpp:513-531` for the fan-out instantiation and compile, which stand at `source/counters/plan.cpp:551-572` after the five lines `afd851e` added below `:511`; each comment must cite the lines holding its claim, following the exclusion at `source/counters/plan.cpp:511-515` that `afd851e` re-anchored and this pass verified line by line, and no marker may be added, moved off an executable line, or removed, and no exclusion may be widened (Constitution VI, plan: Complexity Tracking, T177, `contradicts`)

## Phase 19: Convergence

Appended by `/speckit.converge` after an audit of `11bc422` and of the residue the six
waves before it left. Nothing above this line changed.

Audit evidence, all produced by this pass. `cmake --preset=ci-ubuntu` then
`cmake --build build` exits 0 with zero warnings in the project's own C++ sources, and
`ctest --test-dir build` runs all 37 in 43.31 s with 0 failed and 0 skipped.
`dbc-gate` reports 135 interfaces with 0 gaps in the doc gate and 0 in the pair gate,
`format-check` exits 0, and `cmake -P cmake/spell.cmake` exits 0.
`python3 tools/prose/prose_gate.py --check all` exits 0 over 107 sources and 7846 units
with 0 findings and 1 skipped over the range ending at `11bc422`, the commit this pass audited, and the range ending at `0dd797e` exits 0 with 0 findings after the message carrying the finding was rewritten; `--mode tree` collects its files with `git ls-files` and reads them from the working tree, so no commit reproduces its total (`tasks.md:1112-1119`), and its findings, of which
the two inside `test/source/dbc_test.cpp` are ruled out of this feature on the grounds
Phase 18 established. `build/coverage/coverage.info` carries mtime 23:46:53, later than
the newest source in the tree (23:34:53, `source/counters/fold.cpp`), and
`bash tools/dbc/coverage_gate.sh` exits 0 at lines 100.0% (1955 of 1955), branches
100.0% (705 of 705), functions 98.0% (289 of 295). `python3
tools/pmu_events/update_pmu_events.py --check` exits 0. A fresh `ci-linux-audit` tree
configured into `/tmp` carries the committed `flags-gcc-clang` set in
`CMAKE_CXX_FLAGS`, `-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3` in
`CMAKE_CXX_FLAGS_RELEASE`, `speedgun-ng_CONTRACTS:STRING=enforce` and
`speedgun-ng_DEVELOPER_MODE:BOOL=OFF`, which is what `T186` claimed and what no earlier
pass measured.

The code reproduces every claim Phase 18 made about it. The four exclusion
justifications `T188` re-anchored each resolve to the code they describe:
`source/counters/plan.cpp:456` is the `availability_name` call, `:417` and `:536` are
the two zero-leaf refusals, `source/counters/plan.cpp:135-158` is `link_ratio_slots`
with the enabled/running write at `:150-156`, and `:551-572` is the fan-out
instantiation and compile. The exclusion at `source/counters/plan.cpp:511-515` cites
`:436`, `:442`, `:481`, `:492` and `:496`, and each holds the statement the reason
attributes to it. The `LCOV_EXCL_*` count inside this feature's own scope,
`source/counters/**` plus `include/speedgun-ng/counters*.hpp`, is 304, which is the
figure `plan.md:454` records; the 308 a whole-tree count returns differ by the four
tokens in `include/speedgun-ng/dbc.hpp`, which spec 001 owns. `FR-035`'s
push-decrement check is enforced at `source/counters/fold.cpp:211` and driven by
`test/source/counters_trap_fixture.cpp:112`, so US4 scenario 6 and the matching Edge
Case are covered. `FR-010`'s purity scan exempts `fast_rdpmc` for a reason stated at
`test/counters_header_purity.sh:22-36`, and `FR-049`'s standalone example resolves to
`libstdc++`, `libgcc_s`, `libc` and `libm` with zero `speedgun-ng` entries under both
`readelf -d` and `ldd`. `format-check`'s `GLOB_RECURSE` patterns at
`cmake/lint.cmake:12-15` reach `source/counters/**` and `include/speedgun-ng/**`, and
every counters source and header this pass sampled matches `clang-format` exactly.

Coverage of the check: 127 requirement keys (50 FR, 10 SC, 49 user-story acceptance
scenarios, 18 spec edge cases), 32 plan decision keys, and 11 constitution principles
with X.1 through X.4 and XI.1 through XI.6 read one by one. Four findings: 4
`contradicts`; 1 HIGH, 2 MEDIUM, 1 LOW. No requirement is missing and no requirement is
partially built.

All four findings are the one class the constitution's own Sync Impact Report predicted.
The 2.9.1 amendment grew the file from 588 lines to 612, and `:19-20` states the
consequence in its own words: "This report shifts every line in the file, so a citation
into the constitution, including the Phase 18 citations in
`specs/007-counters-and-timers/tasks.md`, needs re-anchoring by the next convergence
pass." The report named the debt and the wave that owes it, and `11bc422` settled three
of the five citations it touched while leaving three others naming lines that hold
different text.

The shift is not one number, and that is what the three hunks of the amendment's diff
record: `2c2,23` grows the comment block, `262,264c283,287` replaces the three-line
release-build clause `11bc422` widened, and `573a597` adds the lineage row. An anchor
below old line 262 therefore moved by 21, one between old lines 265 and 573 by 23, and
one below old line 574 by 24, so `:211` and `:258-264` sit above the widened clause and
`:443`, `:447-448` and `:554-556` sit below it. `T189` and `T190` carry the two
different shifts, and no clause was renumbered, only moved.

What `11bc422` got right, verified anchor by anchor against `HEAD~1`. `T073` moved
`constitution.md:424-425` to `:447-448` and the two ranges hold identical text.
`T106` moved `:531-533` to `:554-556` and they match. `T184` moved `:250` to
`:279-287`, and `:279` is where the per-task verification clause starts.
`prose_rules.yaml:19` moved `:443` to `:466`, and both lines held the `prose-lint`
enforcement bullet, so the shift preserved an anchor that was already wrong, which is
`T190` below. `T187`'s own citations into `AGENTS.md:43-44` and `:48-50` and
`CMakePresets.json:156-158` hold: `AGENTS.md:43-44` carries the per-platform release
presets, `:48-50` carries the machine-local presets sentence and the matrix paragraph
naming macOS and Windows, and `CMakePresets.json:156-158` is the `ci-multi-config`
block whose `CMAKE_CONFIGURATION_TYPES` the `Xcode` generator reads, with the
`ci-macos` preset itself at `:161-164`.

### HIGH: the amendment's own Sync Impact Report named the debt, and two citations still name other text

- [X] T189 Re-anchor the two Phase 18 preamble citations into the constitution that the 2.9.1 Sync Impact Report at `.specify/memory/constitution.md:19-20` names as owed to this pass, where the 23-line header insert moved both: `:855-856` places the MSVC suspension "inside Principle VIII at `:211-216`", and that range holds the P0-techniques bullet, the distributions bullet, and the per-platform baselines bullet, while the suspension stands at `.specify/memory/constitution.md:232-237`; `:829` places the per-task verification clause at `:258-264`, and that range holds the gate-weakening bullet, the `### IX.` heading, and Principle IX's first two lines, while the clause stands at `:279-287` and `T184`'s own body at `:770` already names it correctly, so the preamble at `:829` asserts of `T184` that it "still names `.specify/memory/constitution.md:250`" on a line whose `T184` names `:279-287`; each amended citation must name the line holding its claim, the `:829` sentence must stop asserting a state of `T184` that the same file contradicts, and the constitution, the `Sync Impact Report`, the version, and the lineage row may not move (Constitution IX, IV, T187, `contradicts`)

### MEDIUM: a shifted anchor preserved an error the shift created

- [X] T190 Point `tools/prose/prose_rules.yaml:19` at the line that holds the `runner` section token, which stands at `.specify/memory/constitution.md:515` and names `(`CMake`, `Docs`, `runner`, `dbc`)` as the Section vocabulary; the comment currently cites `:466`, which holds the `prose-lint` enforcement bullet, and `11bc422` reached it by shifting the previous `:443` by the amendment's 23 lines while `:443` already held that same bullet at `HEAD~1` and the token stood at `:492` there, so the shift preserved the error instead of correcting it; the citation must name the line holding the token, the `runner` entry itself at `tools/prose/prose_rules.yaml:28` may not move, and no rule may be added, removed, or renamed (Constitution IV, specs/002 R-11, D4, T114, `contradicts`)

### MEDIUM: the quickstart records a suite size the tree no longer has

- [X] T191 Restate the two suite counts in the SC-002 row at `specs/007-counters-and-timers/quickstart.md:138`, which reads "the suite is 35 tests and all 35 pass, none skipped" and reports "`ctest --test-dir build/dev -R counters` 14 passed, 0 skipped, exit 0", while `test/CMakeLists.txt` registers 37 `add_test` entries, `ctest --test-dir build -N` reports 37, and `-R counters` selects 16, because `8848f4e` registered `counters_standalone_example` and `counters_giraffe_example` after `6e6e769` wrote the row; the counts must match the tree the row reports on, the PASS verdict stands on the 37 of 37 this pass measured, and no SC row's verdict, evidence filename, or `permission_blocked` finding may change (Constitution X.4, plan: Test Plan, quickstart §SC-002, `contradicts`)

### LOW: four citations in the owning spec drifted the same way

- [X] T192 Re-anchor the four citations into the constitution that `specs/002-prose-commit-lint` owns, all correct at `a00b208` and all stale now: `contracts/rule-data.md:25` and `data-model.md:88` and `research.md:413` name `constitution.md:443` for the `runner` section token, and `research.md:248` names `constitution.md:361-388` for the span Principle XI quotes its own banned vocabulary across, where 443 held the token and XI ran 324 to the file's end at `a00b208`, while the token is at `.specify/memory/constitution.md:515` and XI runs `:387-476`; `T114` records that `specs/002` owns `tools/prose/prose_rules.yaml`, so the fix lands in the owning spec's own change and the four citations must each name the line holding its claim, with no rule, token, or decision in `rule-data.md` or `data-model.md` altered (Constitution IV, IX, T114, T190, `contradicts`)

## Phase 20: Convergence

Appended by `/speckit.converge` after an audit of the `162506b` completion claim
and of the residue the eight waves before it left. Nothing above this line changed.

Audit evidence, all produced by this pass. `cmake --preset=dev` then
`cmake --build --preset=dev` exits 0, and `ctest --preset=dev` passes 37 of 37 with
0 failed and 0 skipped in 43.40 s. `cmake --preset=ci-ubuntu` then `cmake --build
build`, the release build Principle IX requires once per feature, exits 0.
`cmake --preset=ci-sanitize` then `cmake --build --preset=ci-sanitize` then
`ctest --preset=ci-sanitize` exit 0 three times. `dbc-gate` reports 135 interfaces
with 0 gaps in the doc gate and 0 in the pair gate, `format-check` exits 0, and
`cmake -P cmake/spell.cmake` exits 0. `build/coverage/coverage.info` carries mtime
2026-09-27 23:46:53, no file under `source/`, `include/`, or `test/` is newer than
it, and `bash tools/dbc/coverage_gate.sh` run on it exits 0 at lines 100.0% (1955
of 1955), branches 100.0% (705 of 705), and functions 98.0% (289 of 295) on an
axis no gate scores. `python3 tools/pmu_events/update_pmu_events.py --check` exits
0. `ctest --test-dir build -N` reports 37 tests, of which 16 match `-R counters`,
the two counts the SC-002 row records.

Two gates are red, and both name the same commit. `python3 tools/prose/prose_gate.py
--check all` exits 1 with one finding, `commit 162506b: XI2.CONTRASTIVE family=XI.2`,
and `cmake -P cmake/prose-lint.cmake` reproduces that verdict, exiting 1 on `Prose
gate raised findings (status 1)` (T193). The gate's `--mode tree` form collects its files with `git ls-files` and reads them from the working tree, so no commit reproduces its total (`tasks.md:1112-1119`), exits 1, and
none of its findings sits in this
feature's artifacts or in `include/speedgun-ng/counters*`, `source/counters/`,
`test/source/counters_*`, `example/counters_*`, `docs/pages/counters-overhead.md`,
or `tools/pmu_events/`, so the one red finding in the range is the commit message
and nothing else. The commit that carries it also records a gate verdict this pass
measured differently (T194).

Coverage of the check: 127 requirement keys (50 FR, 10 SC, 49 user-story acceptance
scenarios, 18 spec edge cases), 29 plan decision keys (R-001 through R-015, the
nine Technical Context decisions, the three Complexity Tracking rows, the Structure
Decision, and the gate-set note), and 11 constitution principles with X.1 through
X.4 and XI.1 through XI.6 read one by one. Three findings: 2 `contradicts`, 1
`partial`; 1 CRITICAL, 1 HIGH, 1 MEDIUM.

The citation class the previous wave predicted is exhausted in the live artifacts.
Every `file:line` citation `spec.md`, `plan.md`, `quickstart.md`, `data-model.md`,
`research.md`, and the three contracts place into a source, a header, a script, or a
second artifact names a file that exists and a line inside it, every single-line
citation names a non-blank line, and each load-bearing range was read against the
claim it carries: `spec.md:30` and the release rows at
`docs/pages/counters-overhead.md:160-161`, `:34` and
`source/counters/detail/core.hpp:104`, `source/counters/plan.cpp:469`, `:501`,
`:448-457`, `:502-504`, `:61-62`, `source/counters/detail/pmu.hpp:292`,
`include/speedgun-ng/counters_measurement.hpp:1089-1095`, `:35` and
`source/counters/clock_provider.cpp:206-236`, `:36` and `:199` and
`source/counters/fold.cpp:289-290` with `source/counters/plan.cpp:330` and `:344-345`,
`:250` and `include/speedgun-ng/counters_provider.hpp:245-251` and `:326-337` with
`example/counters_giraffe_example.cpp:46-57`, and `quickstart.md:138` and `:140`
against `docs/pages/counters-overhead.md:314`, `:326-327`, `:170-173`, and
`:214-237`. Each holds the claim its sentence makes. Four checks no earlier wave
recorded came back clean: `test/CMakeLists.txt:313-316` registers both example
targets with `add_test` and `.github/workflows/ci.yml:147-154` reads the standalone
example's link manifest while `:160` runs the table check, `:303` and `:332`
configure through `ci-linux-ignore`, and `:422-427` run the release-configured
counters trap target; US2 scenario 3's construction refusal stands at
`source/counters/plan.cpp:233-235` with its cases at
`test/source/counters_recorder_test.cpp:168-173`; US1 scenario 5's ten further
metrics assert zero provider reads at `test/source/counters_fake_test.cpp:493-494`;
and FR-003's provider-declared attribute keys ship with the `filter` shape at
`include/speedgun-ng/counters_system.hpp:230-253`. What the exhausted class leaves
behind is one count in the plan's physical-view table (T195).

### CRITICAL: the prose-lint hard gate is red over this feature's range

- [X] T193 Restate the sentence the XI2.CONTRASTIVE pattern at `tools/prose/prose_rules.yaml:94` matches in the body of commit `162506b`, which reads `` `the shift preserved the error instead of correcting it` `` at line 25 of that message, so `python3 tools/prose/prose_gate.py --check all` exits 1 with `commit 162506b: XI2.CONTRASTIVE family=XI.2` and `cmake -P cmake/prose-lint.cmake` exits 1 on `Prose gate raised findings (status 1)`; the sentence must state what the shift did in its own terms, the Pull Request Quality section permits rewriting a commit before merge and the rewrite must carry the template, the gate must exit 0 over the rewritten range, and the constitution, its Sync Impact Report, its version, and its lineage row may not move (CRITICAL, Constitution VIII, XI.2, XI.6, `contradicts`)

### HIGH: the commit records an exit code the gate does not produce

- [X] T194 Correct the recorded gate verdict in the body of commit `162506b` and in the Phase 19 preamble at `specs/007-counters-and-timers/tasks.md:898-899`, which both state that `python3 tools/prose/prose_gate.py --check all` exits 0 over 107 sources and 7846 units with 0 findings, where this pass measured exit 1 over 110 sources and 7987 units with 1 finding because the measurement was taken before the commit carrying it entered the range the gate scans; the recorded claim must be one the scanned range reproduces, the source and unit counts must be re-measured after `T193`'s rewrite, and a commit message asserting a whole-range verdict its own text then changes may not stand (HIGH, Constitution X.4, `contradicts`)

### MEDIUM: the plan's physical-view table still names six public headers

- [X] T195 Correct `specs/007-counters-and-timers/plan.md:298`, whose Files-and-duties row names the public surface as `include/speedgun-ng/counters*.hpp` "(6 files)" while the tree holds nine headers, while `specs/007-counters-and-timers/plan.md:35` states "9 new public headers", while `specs/007-counters-and-timers/plan.md:325` states "The nine public headers carry it" and lists them, and while `T117` amended `tasks.md:30` to name all nine; `T117` removed the claim from two of the three places it stood and left this one, which is the sibling-occurrence class `T175` recorded, and the count must read nine (MEDIUM, plan: Files and their duties, T117, `partial`)

## Phase 21: Convergence

Appended by `/speckit.converge` after an audit of the `2f27800` and `0dd797e`
completion claims and of the citation residue the eight waves before them left.
Nothing above this line changed.

Audit evidence, all produced by this pass. The prose gate run at `0dd797e`,
`python3 tools/prose/prose_gate.py --check all --head 0dd797e`, exits 0 over 111
sources and 8092 units with 0 findings and 1 skipped, and the two figures `2f27800`
recorded reproduce exactly by passing the commit each names: the gate run at
`2f27800` reports 110 sources and 7995 units and the gate run at `11bc422`
reports 107 sources and 7846 units, both exit 0, which is the anchoring that
commit's message claims for them. `cmake -P cmake/spell.cmake` exits 0, and
`python3 tools/pmu_events/update_pmu_events.py --check` exits 0. The debug tree
configures and builds to exit 0 and `ctest --preset=dev` passes 37 of 37 with 0
failed and 0 skipped in 43.50 s. The release build Principle IX requires once per
feature, `cmake --preset=ci-ubuntu` then `cmake --build build`, exits 0 with zero
warnings in the project's own C++ sources, and the `format-check` and `dbc-gate`
targets exit 0.
`build/coverage/coverage.info` carries mtime 2026-09-27 23:46:53, later than the
newest source in the tree (23:34:53, `source/counters/fold.cpp`), and
`bash tools/dbc/coverage_gate.sh` run on it exits 0 at lines 100.0% (1955 of
1955), branches 100.0% (705 of 705), and functions 98.0% (289 of 295) on an axis
no gate scores. `readelf -d` and `ldd` on
`build/example/counters_standalone_example` name the platform C and C++ runtime
alone with zero `speedgun-ng` entries, and `ctest --test-dir build -N` reports 37
tests of which 16 match `-R counters`, the two counts the SC-002 row records.

Coverage of the check: 127 requirement keys (50 FR, 10 SC, 49 user-story
acceptance scenarios, 18 spec edge cases), 29 plan decision keys, and 11
constitution principles with X.1 through X.4 and XI.1 through XI.6 read one by
one. Two findings: 2 `partial`; 0 CRITICAL, 0 HIGH, 2 MEDIUM. No requirement is
missing, no requirement is partially built, and no requirement is contradicted by
the code.

The citation class the previous wave reported exhausted holds in the live
artifacts, verified rather than taken on report. Every `file:line` citation
`spec.md`, `plan.md`, `quickstart.md`, `data-model.md`, `research.md`, and the
three contracts place into a source, a header, or a second artifact names a file
that exists, every single-line citation names a non-blank line, and each
load-bearing range was read against the claim it carries:
`docs/pages/counters-overhead.md:160-161` and `:170-173` and `:326-327` against
the release and correctness-build medians and the level-3 refusal,
`source/counters/plan.cpp:448-457` and `:502-504` and `:61-62` and `:330` and
`:344-345`, `source/counters/fold.cpp:289-290`,
`source/counters/detail/core.hpp:104`, `source/counters/detail/pmu.hpp:292`,
`source/counters/clock_provider.cpp:206-236`,
`include/speedgun-ng/counters_measurement.hpp:1089-1095` and `:981-1053`,
`include/speedgun-ng/counters_provider.hpp:245-251`, and
`example/counters_giraffe_example.cpp:46-57`. Each holds the claim its sentence
makes. The gate's `--mode tree` form, `python3 tools/prose/prose_gate.py --check
all --mode tree`, collects its files with `git ls-files` and reads them from the
working tree, ignoring the range, while its commit half follows `--head`, so its
total mixes a working-tree read, which every edit to this file moves, with a
commit range, and no commit reproduces it. The run exits 1, and none of its
findings sits in this feature's artifacts, in `include/speedgun-ng/counters*`,
in `source/counters/`, in `test/source/counters_*`, in `example/counters_*`, in
`docs/pages/counters-overhead.md`, or in `tools/pmu_events/`.

What is left is the two lines the 2.9.1 citation pass shifted. That pass moved
each cited anchor by the amendment's line count and confirmed the shift
arithmetic, and arithmetic alone cannot tell whether the old anchor held the claim
it was written to support. Two of them named unrelated text, and each names
unrelated text still.

### MEDIUM: two shifted anchors name text the sentence does not claim

- [X] T196 Re-anchor T106's constitution pointer at `specs/007-counters-and-timers/tasks.md:397` per Constitution IV, which names `constitution.md:554-556` to support the claim that the `dev` presets must stay in the machine-local `CMakeUserPresets.json`, where that range holds the tail of the Language bullet and the head of the Warnings-and-hardening bullet and the sentence it names stands at `constitution.md:568-570`; the pre-amendment range `:531-533` held the same unrelated text and the sentence stood at `:545-547`, so the shift in `11bc422` moved the anchor and left it wrong (MEDIUM, `partial`)
- [X] T197 Re-anchor T073's constitution pointer at `specs/007-counters-and-timers/tasks.md:358` per Constitution IV, which names `constitution.md:447-448` to support the claim that both defects are reviewer-parity defects, where that range holds the tail of the XI.5 marketing-vocabulary list and the clause it names stands at `constitution.md:461-462` in XI.6, with XI.2 at `:411-421` and the `actually` filler entry at `:442`; the pre-amendment range `:424-425` held the same unrelated text, so the shift in `11bc422` moved the anchor and left it wrong (MEDIUM, `partial`)

## Phase 22: Convergence

Appended by `/speckit.converge` after an audit of the branch tip at `2c45407`, the third
commit after the two the previous pass audited. Nothing above this line changed.

Audit evidence, all produced by this pass and reproducible by re-running each command
from the repository root. `python3 tools/prose/prose_gate.py --check all` exits 1 and
attributes every finding to one source, the body of commit `2c45407`: family
`XI5.MARKETING` on `industry-leading`, `robust`, `blazing-fast`, `elegant`, and
`powerful`, five findings. `cmake -P cmake/prose-lint.cmake` exits 1 on the same
findings, with `Prose gate raised findings (status 1)` raised at
`cmake/prose-lint.cmake:41`. The gate exits 0 over the range ending at `0dd797e`, and
the two figures `2f27800` records reproduce by passing its commit and `11bc422` to
`python3 tools/prose/prose_gate.py --check all --head`: 110 sources and 7995 units at
`--head 2f27800`, and 107 sources and 7846 units at `--head 11bc422`, both exit 0.
`python3 tools/prose/prose_gate.py --check all --head 0dd797e` reports 111 sources and
8092 units with 0 findings and 1 skipped, exit 0.

Every other gate exits 0 at this tip. `cmake --preset=dev` and
`cmake --build --preset=dev` exit 0; `ctest --preset=dev` exits 0 with 100% of 37 tests
passed, 0 failed, 0 skipped; `cmake --build build/dev -t format-check` exits 0;
`cmake --build build/dev -t dbc-gate` exits 0 with 135 interfaces and 0 gaps in both the
doc gate and the pair gate; `cmake --preset=ci-ubuntu` and `cmake --build build` exit 0;
`bash tools/dbc/coverage_gate.sh build/coverage/coverage.info` exits 0 at lines 100.0%
(1955 of 1955), branches 100.0% (705 of 705), and functions 98.0% (289 of 295);
`python3 tools/pmu_events/update_pmu_events.py --check` exits 0;
`cmake -P cmake/spell.cmake` exits 0. The glob `include/speedgun-ng/counters*.hpp`
matches 9 headers, the count `plan.md:35`, `plan.md:298`, and `plan.md:325` record, and
`ctest --test-dir build -N` reports 37 tests of which 16 match `-R counters`, the counts
the SC-002 row of `quickstart.md` records.

Coverage of the check: 671 `file:line` citations swept, 638 across the feature's
Markdown artifacts and 33 across the bodies of `11bc422`, `2f27800`, `0dd797e`, and
`2c45407`, each one read against the text it holds rather than against the line number
it names. Zero sit on text unrelated to the claim their sentence makes. The rest either
resolve to the text they name or are stale-historical anchors inside Phases 1 through 17,
each correct against the revision it was written against and none a claim about the
present tree. 28 are bare `:NN` forms whose target file is not mechanically derivable
from the sentence, and every one of them holds the text its sentence names when read by
intent. One range runs past end of file: `tasks.md:451` names
`source/counters/linux_pmu/fast_read.cpp:309-444` in a file of 331 lines, a completed
Phase 13 task whose range was correct when the file held 462 lines.
The class the previous two passes reported closed holds: `constitution.md:568-570` still
holds the machine-local `CMakeUserPresets.json` sentence `T106` needs and
`constitution.md:461-462` still holds the lint-parity clause `T073` needs, and
`tools/prose/prose_rules.yaml:19` still names `constitution.md:515` for the `runner`
section token.

Requirement coverage: 127 requirement keys, 50 functional requirements with no gap in
the `FR-001` through `FR-050` numbering, 10 success criteria with no gap in `SC-001`
through `SC-010`, 49 user-story acceptance scenarios across 8 stories, and 18 edge
cases; 29 plan decision keys; and 11 constitution principles with X.1 through X.4 and
XI.1 through XI.6 read one by one. No requirement is missing, partially built, or
contradicted by the code, and no plan decision is unmet. `FR-007` and `SC-009` are the
only requirement keys no task names, and both hold in the code: no public
`include/speedgun-ng/counters*.hpp` header carries platform preprocessor branching, and
`.github/workflows/ci.yml:160` runs the table check on every change while
`test/CMakeLists.txt:249` registers the fixture pair that asserts the drift direction
exits 1.

What is left is the one source a commit message is and no other artifact is. The
commit that reports the gate green is the finding, and it reports the gate green at a tip
the gate no longer agrees with. Nothing above builds anything, so the code is
converged; the prose around it is not.

### CRITICAL: the tip commit's message violates the principle its own commit cites

- [X] T198 Mark the five banned marketing tokens the body of commit `2c45407` quotes
  as a quotation, or drop them, so `python3 tools/prose/prose_gate.py --check all` and
  `cmake -P cmake/prose-lint.cmake` exit 0 over the range ending at the rewritten
  commit; the body names the tail of the XI.5 marketing-vocabulary list
  (`constitution.md:445-447`) as bare prose in the paragraph that re-anchors `T073`, and
  `tools/prose/prose_rules.yaml:62` exempts an inline code span, so the rewrite restores
  both gates while keeping the sentence that carries the evidence, and the branch
  carries no upstream with `2c45407` not an ancestor of `origin/master`, so the
  pre-merge rewrite the Pull Request Quality section permits applies, the precedent
  `162506b` to `2f27800` having set it (CRITICAL, Constitution XI.5, XI.6, VIII,
  `contradicts`)

### HIGH: two recorded gate verdicts name a range the gate does not reproduce

- [X] T199 Anchor the prose-gate verdict the body of commit `2c45407` records to the
  commit whose range reproduces it, the way `2f27800` records 110 sources and 7995 units
  for `--head 2f27800` and 107 sources and 7846 units for `--head 11bc422`; the body
  records that the gate `exits 0` over 111 sources and 8092 units with 0 findings and 1
  skipped `at the branch tip`, and that verdict reproduces only under
  `python3 tools/prose/prose_gate.py --check all --head 0dd797e`, while the range ending
  at `2c45407` exits 1 with 5 findings, so a whole-range figure recorded in a message
  that the same check examines must name its commit or state no number (HIGH,
  Constitution X.4, XI.6, `partial`)
- [X] T200 Re-anchor the prose-gate claim the Phase 21 preamble records at
  `specs/007-counters-and-timers/tasks.md:1068-1069`, which reads that the gate `exits 0
  over 111 sources and 8092 units with 0 findings and 1 skipped` at the branch tip and
  names no commit, so the sentence must name `0dd797e` as the commit whose range
  reproduces those figures, which is the anchoring the next two lines of the same
  paragraph already apply to the figures `2f27800` recorded (HIGH, Constitution X.4,
  `partial`)

### LOW: one recorded unit count in the Phase 21 preamble does not reproduce

- [X] T201 Correct the `--mode tree` unit count the Phase 21 preamble records at
  `specs/007-counters-and-timers/tasks.md:1112`, which reads 108 findings over 226
  sources and 17264 units where `python3 tools/prose/prose_gate.py --check all --mode
  tree` reports 17323 units over `--head 0dd797e` and 17367 units and 227 sources at
  the tip, and the count moved because recording the figure in this file added the units
  to a source the gate scans, so the sentence must name the commit whose run reproduces
  the count (LOW, Constitution X.4, `partial`)

## Phase 23: Convergence

Appended by `/speckit.converge` after an audit of the branch tip at `5e2181e`, the
fourth commit after the one the previous pass audited. Nothing above this line
changed.

Audit evidence, all produced by this pass from the repository root and
reproducible by re-running each command.
`python3 tools/prose/prose_gate.py --check all` and `cmake -P cmake/prose-lint.cmake`
exit 0, and this preamble records no figure for the range that holds it. Passing each of the
six commits from `11bc422` through `22cb41c` to `--head` reports 107 sources and 7846 units
at `11bc422`, 110 sources and 7995 units at `2f27800`, 111 sources and 8092 units at
`0dd797e`, 112 sources and 8200 units at `27a5659`, 113 sources and 8344 units at
`17cb1a6`, and 114 sources and 8433 units at `22cb41c`, all six
exit 0. `cmake --preset=dev` and `cmake --build --preset=dev` exit 0, and
`ctest --preset=dev` exits 0 with 100 percent of 37 tests passed, 0 failed, 0
skipped, in 43.26 s. `cmake --preset=ci-ubuntu` and `cmake --build build` exit
0, and the two warnings in that configure log are CPack messages, with no
compiler warning in the project's own C++ sources.
`cmake --build build/dev -t format-check` exits 0, and `dbc-gate` exits 0 with
135 interfaces and 0 gaps in both the doc gate and the pair gate.
`python3 tools/pmu_events/update_pmu_events.py --check` exits 0, and
`cmake -P cmake/spell.cmake` exits 0. The coverage gate run directly on
`build/coverage/coverage.info` exits 0 at lines 100.0 percent (1955 of 1955),
branches 100.0 percent (705 of 705), and functions 98.0 percent (289 of 295)
on an axis no gate scores, and that trace carries an mtime later than every
source, header, test, and example file in the tree the branch carries at `22cb41c`, whose newest is `source/counters/fold.cpp` at 23:34:53; the newest tracked tool file, `tools/prose/prose_rules.yaml` at 2026-09-28 00:33:42, carries an mtime later than the trace. The gate's
`--mode tree` form collects its files with `git ls-files` and reads them from the working tree, so no commit reproduces its total, exits 1, and none of them
sits in this feature's artifacts, in `include/speedgun-ng/counters*`, in
`source/counters/`, in `test/source/counters_*`, in `example/counters_*`, in
`docs/pages/counters-overhead.md`, or in `tools/pmu_events/`, which is the
reading the Phase 21 preamble records at
`specs/007-counters-and-timers/tasks.md:1112-1119` now states with no number.

The commit-title limit holds for the seven commits from `11bc422` through
`29b5a56`. `tools/prose/prose_rules.yaml:11` sets `title_max: 50`, and the
seven titles measure 41, 37, 44, 50, 45, 44, and 50 characters at `11bc422`,
`2f27800`, `0dd797e`, `27a5659`, `17cb1a6`, `22cb41c`, and `29b5a56`.

The citation residue the previous pass recorded as closed holds. Every anchor
below was read against the text it holds, with the number it names serving as a
pointer. `tools/prose/prose_rules.yaml:19` names `constitution.md:515` for the
`runner` section token, and `constitution.md:515` holds the Title bullet
carrying it. `constitution.md:568-570` holds the machine-local
`CMakeUserPresets.json` sentence `T106` needs, `constitution.md:461-462` holds
the lint-parity clause `T073` needs, `constitution.md:445-447` holds the tail of
the XI.5 marketing-vocabulary list `T198` names, and
`tools/prose/prose_rules.yaml:62` exempts the inline code span, while
`cmake/prose-lint.cmake:41` raises the failure the Phase 22 preamble quotes.
The counts the Phase 22 preamble records reproduce: the glob
`include/speedgun-ng/counters*.hpp` matches 9 headers, the count
`specs/007-counters-and-timers/plan.md:35`,
`specs/007-counters-and-timers/plan.md:298`, and
`specs/007-counters-and-timers/plan.md:325` record, and a
`ctest --test-dir build -N` run reports 37 tests of which 16 match `-R counters`,
the counts the SC-002 row of `quickstart.md` records.
`.github/workflows/ci.yml:160` runs the table check on every change, and
`test/CMakeLists.txt:249` registers the fixture pair, the two sites `FR-007` and
`SC-009` rest on.

Coverage of the check: 127 requirement keys (50 FR, 10 SC, 49 user-story
acceptance scenarios, 18 spec edge cases), 29 plan decision keys, and 11
constitution principles with X.1 through X.4 and XI.1 through XI.6 read one by
one. Every file path `plan.md` names in its Project Structure and its physical
view exists: the 9 public headers, the 11 translation units under
`source/counters/`, the vendored `external/pmu-events` tree with its `RECORD`,
the python tool, both examples, the 11 test executables `plan.md:147-164`
names, the fixture directory, and the overhead page. `cmake/lint.cmake` reaches
`source/counters/**` through `GLOB_RECURSE` on `source/*.cpp` and
`source/*.hpp`, so the format reach plan.md asks to verify holds, and
`format-check` reports zero badly formatted files across it. One finding: 1
`contradicts`. No requirement is missing or partially built, no requirement is
contradicted by the code, and no plan decision is unmet.

What is left is the class the last three passes have been closing, in the one
source a commit message is. The body of `5e2181e` records a whole-range
prose-gate figure that names no commit, in the position of a claim about the
range the command it names scans, and that figure measures the parent's range.

### HIGH: the tip commit's message records a whole-range figure its own command does not produce

- [X] T202 Anchor the prose-gate figure the body of commit `5e2181e` records at lines 34 and 35 of that message, which read that `python3 tools/prose/prose_gate.py --check all` `exits 0 over 112 sources and 8200 units with 0 findings and 1 skipped` and name no commit, where `python3 tools/prose/prose_gate.py --check all --head 5e2181e` reports 113 sources and 8341 units and the pair 112 sources and 8200 units reproduces only at `--head 27a5659`, so the sentence must name `27a5659` as the commit whose range reproduces it or state no number, the anchoring T199 applied to the body of `2c45407`, T200 applied to the Phase 21 preamble, and the next clause of the same paragraph already applies to the figure it names `0dd797e`; the rewrite is pre-merge, which the Pull Request Quality section permits, and the message must keep the template, its `Refs:` and `Approved-by:` footers, and the 50-character title limit per Constitution X.4 (contradicts, HIGH)

## Phase 24: Convergence

Appended by `/speckit.converge` after an audit of the branch tip at `22cb41c`
and of the residue the five waves before it left. Nothing above this line
changed.

Audit evidence, all produced by this pass from the repository root.
`python3 tools/prose/prose_gate.py --check all` exits 0, and `cmake -P
cmake/prose-lint.cmake` reproduces that verdict. Passing each of the seven commits from `11bc422`
through `29b5a56` to `--head` reports 107 sources and 7846 units at `11bc422`, 110 sources and
7995 units at `2f27800`, 111 sources and 8092 units at `0dd797e`, 112 sources and 8200 units at
`27a5659`, 113 sources and 8344 units at `17cb1a6`, 114 sources and 8433 units at `22cb41c`, and
115 sources and 8531 units at `29b5a56`, every one of the seven exit 0. `cmake --preset=dev` and
`cmake --build --preset=dev` exit 0, and `ctest --preset=dev` exits 0 with
100.0 percent of 37 tests passed, 0 failed, 0 skipped. `cmake
--preset=ci-ubuntu` and `cmake --build build` exit 0. `cmake --build build/dev
-t format-check` exits 0, and `dbc-gate` exits 0 with 135 interfaces and 0
gaps in both the doc gate and the pair gate. `python3
tools/pmu_events/update_pmu_events.py --check` exits 0, and `cmake -P
cmake/spell.cmake` exits 0. The coverage gate run directly on
`build/coverage/coverage.info` exits 0 at lines 100.0 percent (1955 of 1955),
branches 100.0 percent (705 of 705), and functions 98.0 percent (289 of 295)
on an axis no gate scores. This preamble records no figure for the range
holding it, and no `--mode tree` total, because that form collects its files
with `git ls-files` and reads them from the working tree, so its total moves
with every edit to any tracked file and no commit reproduces it, which is the
reading the Phase 21 preamble states with no number at
`specs/007-counters-and-timers/tasks.md:1112-1119`.

Coverage of the check: 127 requirement keys (50 FR, 10 SC, 49 user-story
acceptance scenarios, 18 edge cases), 29 plan decision keys, and 11
constitution principles with X.1 through X.4 and XI.1 through XI.6 read one by
one. The `FR-001` through `FR-050` and `SC-001` through `SC-010` numberings
hold with no gap, every file path `plan.md` names in its Project Structure and
its physical view exists, the glob `include/speedgun-ng/counters*.hpp` matches
the 9 headers `plan.md:35`, `plan.md:298`, and `plan.md:325` record, and
`find source/counters -name '*.cpp'` returns the 11 translation units. Five
findings: 4 `contradicts`, 1 `partial`; 2 HIGH, 3 LOW. No requirement is
missing or partially built, no requirement is contradicted by the code, and no
plan decision is unmet. What is left is the newest audit record, and the code
the last six waves converged is untouched by every finding below.

The root cause of the two HIGH findings is one event. The amend that turned
`5e2181e` into `17cb1a6` rewrote the branch after the Phase 23 preamble was
committed, and three sentences in that preamble now describe a commit the
branch no longer carries. `git merge-base --is-ancestor 5e2181e HEAD` exits
non-zero while `git cat-file -t 5e2181e` still reports a commit, so the object
survives in the local database while the range the gate scans, and any fresh
clone, have moved past it. The commit-title limit itself holds on the range
from the merge base with `origin/master`, `65beada`, to `29b5a56`, whose 48
titles measure 50 characters or fewer, and on the seven commits from `11bc422`
through `29b5a56`, whose titles measure 41, 37, 44, 50, 45, 44, and 50.

### HIGH: two recorded figures name a range no commit the branch carries reproduces

- [X] T203 Anchor the prose-gate figure the Phase 23 preamble records at `specs/007-counters-and-timers/tasks.md:1248-1249`, which reads that `python3 tools/prose/prose_gate.py --check all` `exits 0 over 113 sources and 8341 units with 0 findings and 1 skipped` and names no commit, where the command it names carries no `--head` and run at the branch reports 114 sources and 8430 units, and the pair reproduces only at `--head 5e2181e`, a commit the branch no longer carries; the sentence must name the commit whose range reproduces each figure or state no number, the anchoring T199 applied to the body of the `2c45407` tip, T200 applied to the Phase 21 preamble, and T202 applied to the body of `5e2181e`; this preamble records no figure for the range holding it, and the five figures the next paragraph names stand (HIGH, Constitution X.4, XI.6, `contradicts`)
- [X] T204 Restate the two branch inventories the Phase 23 preamble records at `specs/007-counters-and-timers/tasks.md:1250-1253` and `specs/007-counters-and-timers/tasks.md:1274-1277`, which read that passing `each commit on the branch` to `--head` reports figures ending in `113 sources and 8341 units at 5e2181e` and that the title limit holds `for every commit on the branch` with `the five titles` naming the same commit, where the branch carries six commits from `11bc422` whose titles measure 41, 37, 44, 50, 45, and 44 characters, every one within the limit `tools/prose/prose_rules.yaml:11` sets, and `17cb1a6` and `22cb41c` appear in neither list; both sentences must enumerate the commits the branch carries, must name the commit each figure belongs to, and must record no figure and no title length for a commit the branch does not carry (MEDIUM, Constitution X.4, `contradicts`)

### LOW: three claims in the same preamble and one stale header

- [X] T205 Correct the freshness claim the Phase 23 preamble records at `specs/007-counters-and-timers/tasks.md:1265-1266`, which reads that the coverage trace `carries an mtime later than every source, header, test, example, and tool file in the tree`, where `build/coverage/coverage.info` carries mtime 2026-09-27 23:46:53 and `tools/prose/prose_rules.yaml` carries 2026-09-28 00:33:42, so one tracked tool file is newer than the trace; the claim must name the scope it holds for, which is the narrower reading the Phase 21 preamble records at `specs/007-counters-and-timers/tasks.md:1081-1082` as `later than the newest source in the tree (23:34:53, source/counters/fold.cpp)`, and a freshness claim over modification times must state the tree it was measured against because the times move with every checkout and every amend (LOW, Constitution X.4, `contradicts`)
- [X] T206 State the `--mode tree` reading with no number at every Phase preamble that records one, the fix T201 applied at the Phase 21 site and the Phase 23 preamble places against itself one paragraph after it states the rule: `specs/007-counters-and-timers/tasks.md:528`, `:702`, `:801`, `:899`, `:1007`, and `:1267` each record a source or unit total, where the form collects its files with `git ls-files` and reads them from the working tree, so no commit reproduces its total and a run on this tree reports 229 sources and 17602 units with 108 findings and exit 1; each of the six sentences must drop its numbers, must state how the form is read, and must point at the reasoning the Phase 21 preamble records at `specs/007-counters-and-timers/tasks.md:1112-1119` (LOW, Constitution X.4, `contradicts`)
- [X] T207 Drop the stale `(OPEN)` marker from the Phase 13 header at `specs/007-counters-and-timers/tasks.md:427`, which reads `## Phase 13: Fast-Path Verification (OPEN)` while `specs/007-counters-and-timers/tasks.md:433-434` records that `These tasks are closed 2026-09-27`, every task in the phase, `T131` through `T141`, carries a checked box, and no other phase header in the file carries a status marker, so a reader scanning the headers reads an open phase where the body records a closed one; the phase body, its task lines, and every checkbox in the file may not change (LOW, Constitution IV, `partial`)

## Phase 25: Convergence

Appended by `/speckit.converge` after an audit of the branch tip at `29b5a56` and
of the residue the six waves before it left. Nothing above this line changed.

Audit evidence, all produced by this pass from the repository root.
`python3 tools/prose/prose_gate.py --check all` exits 0, and `cmake -P
cmake/prose-lint.cmake` exits 0. Passing each commit from `11bc422` to
`29b5a56` to `--head` reports 107 sources and 7846 units at `11bc422`,
110 sources and 7995 units at `2f27800`, 111 sources and 8092 units at
`0dd797e`, 112 sources and 8200 units at `27a5659`, 113 sources and 8344
units at `17cb1a6`, 114 sources and 8433 units at `22cb41c`, and
115 sources and 8531 units at `29b5a56`, every one of the seven exit 0.
`cmake --preset=dev` and `cmake --build --preset=dev` exit 0, and
`ctest --preset=dev` exits 0 with 100.0 percent of 37 tests passed, 0 failed,
0 skipped. `cmake --preset=ci-ubuntu` and `cmake --build build` exit 0.
`cmake --build build/dev -t format-check` exits 0, and `dbc-gate` exits 0 with
135 interfaces and 0 gaps in both the doc gate and the pair gate.
`python3 tools/pmu_events/update_pmu_events.py --check` exits 0, and
`cmake -P cmake/spell.cmake` exits 0. `bash tools/dbc/coverage_gate.sh
build/coverage/coverage.info` exits 0 at lines 100.0 percent (1955 of 1955),
branches 100.0 percent (705 of 705), and functions 98.0 percent (289 of 295)
on an axis no gate scores. This preamble records no figure for the range
holding it, and no `--mode tree` total, because that form collects its files
with `git ls-files` and reads them from the working tree, so its total moves
with every edit to any tracked file and no commit reproduces it, which is the
reading the Phase 21 preamble states with no number at
`specs/007-counters-and-timers/tasks.md:1112-1119`.

Coverage of the check: 127 requirement keys (50 FR, 10 SC, 49 user-story
acceptance scenarios, 18 edge cases), 29 plan decision keys, and 11
constitution principles with X.1 through X.4 and XI.1 through XI.6 read one by
one. The `FR-001` through `FR-050` and `SC-001` through `SC-010` numberings
hold with no gap, every file path `plan.md` names in its Project Structure and
its physical view exists, the glob `include/speedgun-ng/counters*.hpp` matches
the 9 headers `plan.md:35`, `plan.md:298`, and `plan.md:325` record, and
`find source/counters -name '*.cpp'` returns the 11 translation units. Each of
the 27 `file:line` citations `spec.md` and `quickstart.md` place was read
against the text it holds, and each holds the claim its sentence makes. Two
findings: 2 `contradicts`; 1 HIGH, 1 MEDIUM, 0 CRITICAL. No requirement is
missing or partially built, no requirement is contradicted by the code, and no
plan decision is unmet. What is left is the branch-membership claim the two
newest preambles state, and the code the last seven waves converged is
untouched by both findings below.

The classes the earlier passes recorded as closed hold. Every `--mode tree`
site states how that form is read and carries no number
(`specs/007-counters-and-timers/tasks.md:528`, `:702`, `:801`, `:899`,
`:1007`, and `:1267`), each figure the Phase 23 preamble records names its own
commit, the coverage-trace claim at
`specs/007-counters-and-timers/tasks.md:1265-1266` names the scope it holds
for and the tree it was measured against, and the Phase 13 header at
`specs/007-counters-and-timers/tasks.md:427` carries no status marker. The
commit-title limit holds on the range from the merge base with `origin/master`,
`65beada`, to `29b5a56`: `tools/prose/prose_rules.yaml:11` sets `title_max: 50`,
the 48 titles on that range measure 50 characters or fewer with `29b5a56` at
exactly 50, and the seven titles from `11bc422` measure 41, 37, 44, 50, 45, 44,
and 50.

### HIGH: the newest figure list names five commits where the branch carries seven

- [X] T208 Anchor the branch-membership figure lists the two newest preambles state, `specs/007-counters-and-timers/tasks.md:1331-1335`, which reads that passing `each commit the branch carries` to `--head` reports five figures ending in `113 sources and 8344 units at 17cb1a6` and that `every one of the five exit 0`, where the branch carries seven commits from `11bc422` to `29b5a56` and the sixth, the audit tip the Phase 24 header names, `22cb41c`, reproduces `114 sources and 8433 units` at exit 0, and `specs/007-counters-and-timers/tasks.md:1250`, which reads that `each of the six commits the branch carries` reports six figures, a count that held while `22cb41c` was the tip and that no commit pins now; both sentences must name the commits the set covers, the anchoring T203 applied to the Phase 23 figure at `specs/007-counters-and-timers/tasks.md:1248-1249` and T204 applied to the inventories at `specs/007-counters-and-timers/tasks.md:1274-1277`, so each set stays fixed when a commit lands (contradicts, HIGH, Constitution X.4)

### MEDIUM: the title inventories state their set by reference to the branch and to HEAD

- [X] T209 Anchor the title inventories the two newest preambles state, `specs/007-counters-and-timers/tasks.md:1274-1277`, which reads that `the commit-title limit holds for every commit the branch carries` with `the six titles` naming six lengths, and `specs/007-counters-and-timers/tasks.md:1371-1374`, which reads that `all 47 titles on the range from the merge base with origin/master to HEAD` measure 50 characters or fewer and that `the six titles from 11bc422` measure 41, 37, 44, 50, 45, and 44, where the range from the merge base `65beada` to `29b5a56` carries 48 titles and the branch carries seven titles from `11bc422` measuring 41, 37, 44, 50, 45, 44, and 50; each sentence must name the commit the measurement was taken over with the count that commit gives, must state no count for a range or a set it does not name, and must keep the limit `tools/prose/prose_rules.yaml:11` sets (contradicts, MEDIUM, Constitution X.4)

## Phase 26: Convergence

Appended by `/speckit.converge` after an audit of the branch tip at `33666b1`
and of the residue the seven waves before it left. Nothing above this line
changed.

Audit evidence, all produced by this pass from the repository root.
`python3 tools/prose/prose_gate.py --check all` exits 0, and `cmake -P
cmake/prose-lint.cmake` exits 0. Passing each commit from `11bc422` to
`33666b1` to `--head` reports 107 sources and 7846 units at `11bc422`,
110 sources and 7995 units at `2f27800`, 111 sources and 8092 units at
`0dd797e`, 112 sources and 8200 units at `27a5659`, 113 sources and 8344
units at `17cb1a6`, 114 sources and 8433 units at `22cb41c`, 115 sources and
8531 units at `29b5a56`, and 116 sources and 8620 units at `33666b1`, every
one of the eight exit 0. `cmake --preset=dev` and `cmake --build --preset=dev`
exit 0, and `ctest --preset=dev` exits 0 with 100.0 percent of 37 tests
passed, 0 failed, 0 skipped, in 43.51 s. `cmake --preset=ci-ubuntu` and
`cmake --build build`, the release build Principle IX requires once per
feature, exit 0 with zero compiler warnings in the project's own C++ sources.
`cmake --build build/dev -t format-check` exits 0, and `dbc-gate` exits 0 with
135 interfaces and 0 gaps in both the doc gate and the pair gate. `python3
tools/pmu_events/update_pmu_events.py --check` exits 0, `cmake -P
cmake/spell.cmake` exits 0, and `bash tools/dbc/coverage_gate.sh
build/coverage/coverage.info` exits 0 at lines 100.0 percent (1955 of 1955),
branches 100.0 percent (705 of 705), and functions 98.0 percent (289 of 295)
on an axis no gate scores. That trace carries mtime 2026-09-27 23:46:53, later
than every source, header, test, or example file in the tree the branch
carries at `11bc422`, whose newest is `source/counters/fold.cpp` at 23:34:53,
so the scope the Phase 23 preamble states holds. `ctest --test-dir build -N` reports 37 tests of which 16 match
`-R counters`. This preamble records no `--mode tree` total, because that form
collects its files with `git ls-files` and reads them from the working tree,
so no commit reproduces its total, which is the reading the Phase 21 preamble
states with no number at
`specs/007-counters-and-timers/tasks.md:1112-1119`.

The commit-title limit holds on the range from the merge base with
`origin/master`, `65beada`, to `29b5a56`: `tools/prose/prose_rules.yaml:11`
sets `title_max: 50`, the 48 titles on that range measure 50 characters or
fewer, and the eight titles from `11bc422` measure 41, 37, 44, 50, 45, 44,
50, and 50.

Coverage of the check: 127 requirement keys (50 functional requirements
numbering `FR-001` through `FR-050` with no gap, 10 success criteria
numbering `SC-001` through `SC-010` with no gap, 49 user-story acceptance
scenarios across 8 stories, and 18 spec edge cases), 29 plan decision keys,
and 11 constitution principles with X.1 through X.4 and XI.1 through XI.6
read one by one. Re-measured here: the 9 public headers
`include/speedgun-ng/counters*.hpp` matches, the 11 translation units
`find source/counters -name '*.cpp'` returns, and the 27 `file:line`
citations `spec.md` and `quickstart.md` place, each read against the text it
holds. All 27 name a file that exists and a line inside it, every
single-line citation names a non-blank line, and each load-bearing range
holds the claim its sentence makes, so the citation class holds closed in
the live artifacts. Three findings: 1 `contradicts`, 2 `partial`; 1 HIGH,
1 MEDIUM, 1 LOW. No requirement is missing or partially built, no requirement
is contradicted by the code, and no plan decision is unmet. What is left is
the audit record itself, and the code the last eight waves converged is
untouched by all three findings.

Every `--mode tree` site states how that form is read and carries no number
(`specs/007-counters-and-timers/tasks.md:528`, `:702`, `:801`, `:899`,
`:1007`, and `:1267`), each figure the Phase 23 preamble records names its
own commit, the coverage-trace claim at
`specs/007-counters-and-timers/tasks.md:1265-1266` names the scope it holds
for and the tree it was measured against, and the Phase 13 header at
`specs/007-counters-and-timers/tasks.md:427` carries no status marker. What
is unreached is the moving-set description in its shortest form, the words
`the branch carries` and `at the tip`, and two sites still carry them in the
present tense over a record a later commit has since changed.

### HIGH: the tip before last records two counts for a set that moved

- [X] T210 Anchor the two moving-set counts the body of commit `29b5a56`
  records at lines 13 and 15 of that message, which read `the branch carries
  six` and `both lists now cover all six`, where the branch carries eight
  commits from `11bc422` through `33666b1` and the two lists at
  `specs/007-counters-and-timers/tasks.md:1274-1277` and
  `specs/007-counters-and-timers/tasks.md:1371-1374` name seven, so both
  sentences are false when read again, the class the body of `33666b1`
  defines in its own words as a claim stating a measurement without naming
  what it was measured against; each sentence must name the commit range its
  count belongs to, the pre-merge rewrite the Pull Request Quality section
  permits and T193, T198, and T202 applied to three earlier messages, the
  message must keep its template, its `Refs:` and `Approved-by:` footers, and
  the 50-character title `tools/prose/prose_rules.yaml:11` sets, and the line
  12 predicate `the six figures the branch reproduces` must name the range it
  reproduces for (HIGH, Constitution X.4, XI.6, `contradicts`)

### MEDIUM: a preamble verdict names a moving head beside an anchored clause

- [X] T211 Name the commit for the second gate verdict the Phase 19 preamble
  records at `specs/007-counters-and-timers/tasks.md:899`, which reads that
  `the range ending at the branch tip exits 0 with 0 findings after the
  message carrying the finding was rewritten`, where the sentence's first
  clause names `11bc422` and the tip moves with every commit that lands, so
  the clause states a verdict for a set it leaves unnamed; the sentence must
  name the commit whose range reproduces it or state no number, the anchoring
  T203 applied to the Phase 23 figure and T208 applied to the two figure
  lists, and the line 899 `--mode tree` reading T206 placed there keeps its
  wording (MEDIUM, Constitution X.4, `partial`)

### LOW: an earlier tip commit names the head in the same position

- [X] T212 Name the commit for the gate verdict the body of commit `17cb1a6`
  records at line 31 of that message, which reads `Both prose gates exit 0 at
  the tip again`, where the head moves with every commit that lands and the
  paragraph following it names a commit for every other verdict that message
  records; the sentence must name the commit whose range reproduces it or
  state no number, the pre-merge rewrite the Pull Request Quality section
  permits, and the message must keep its template, its footers, and its
  title (LOW, Constitution X.4, `partial`)

## Phase 27: Convergence

Appended by `/speckit.converge` after an audit of the branch tip at `eaf7ef1`
and of the class the eight waves before it left. Nothing above this line
changed.

Audit evidence, all produced by this pass from the repository root.
`python3 tools/prose/prose_gate.py --check all` exits 0, and `cmake -P
cmake/prose-lint.cmake` exits 0. Passing each of the nine commits from
`11bc422` through `eaf7ef1` to `--head` reports 107 sources and 7846 units at
`11bc422`, 110 sources and 7995 units at `2f27800`, 111 sources and 8092 units
at `0dd797e`, 112 sources and 8200 units at `27a5659`, 113 sources and 8344
units at `17cb1a6`, 114 sources and 8433 units at `22cb41c`, 115 sources and
8531 units at `29b5a56`, 116 sources and 8620 units at `33666b1`, and 117
sources and 8759 units at `eaf7ef1`, every one of the nine exit 0.
`cmake --preset=dev` and `cmake --build --preset=dev` exit 0, and
`ctest --preset=dev` exits 0 with 100.0 percent of 37 tests passed, 0 failed,
0 skipped. `cmake --preset=ci-ubuntu` and `cmake --build build`, the release
build Principle IX requires once per feature, exit 0 with zero compiler
warnings in the project's own C++ sources. `format-check` and `dbc-gate`
exit 0, the pair reporting 135 interfaces with 0 gaps in the doc gate and in
the pair gate. `python3 tools/pmu_events/update_pmu_events.py --check` exits 0,
`cmake -P cmake/spell.cmake` exits 0, and `bash tools/dbc/coverage_gate.sh
build/coverage/coverage.info` exits 0 at lines 100.0 percent (1955 of 1955),
branches 100.0 percent (705 of 705), and functions 98.0 percent (289 of 295)
on an axis no gate scores. `ctest --test-dir build -N` reports 37 tests of
which 16 match `-R counters`. The commit-title limit holds on the range from
the merge base with `origin/master`, `65beada`, to `29b5a56`, whose 48 titles
measure 50 characters or fewer, and on the range from `65beada` to `eaf7ef1`,
whose 50 titles do the same, while the nine titles from `11bc422` measure 41,
37, 44, 50, 45, 44, 50, 50, and 49 characters.

Coverage of the check: 127 requirement keys (50 functional requirements
numbering `FR-001` through `FR-050` with no gap, 10 success criteria numbering
`SC-001` through `SC-010` with no gap, 49 user-story acceptance scenarios
across 8 stories, and 18 spec edge cases), 29 plan decision keys, and 11
constitution principles with X.1 through X.4 and XI.1 through XI.6 read one by
one. Re-measured here: the glob `include/speedgun-ng/counters*.hpp` matches
the 9 headers `specs/007-counters-and-timers/plan.md:35`, `:298`, and `:325`
record, `find source/counters -name '*.cpp'` returns the 11 translation units,
and all 27 `file:line` citations `spec.md`, `plan.md`, `quickstart.md`,
`data-model.md`, `research.md`, and the three contracts place resolve to an
existing file and a line inside it, with every single-line citation naming a
non-blank line, so the citation class holds closed in the live artifacts.
Three findings: 2 `contradicts`, 1 `partial`; 1 HIGH, 1 MEDIUM, 1 LOW, and
none of the three a constitution MUST violation. No requirement is missing or
partially built, no requirement is contradicted by the code, and no plan
decision is unmet. What is left is the audit record itself, and the code the
earlier waves converged is untouched by all three findings.

The class the earlier passes closed is closed everywhere except the one
sub-shape they left. Every `--mode tree` site states how that form is read and
carries no number (`specs/007-counters-and-timers/tasks.md:528`, `:702`,
`:801`, `:899`, `:1007`, and `:1267`), the Phase 19 verdict at `:898-899`
names `11bc422` and `0dd797e`, the coverage-trace claim at `:1265-1266` names
the tree at `22cb41c`, the Phase 13 header at `:427` carries no status marker,
and each of the nine commit bodies records its prose-gate figure against a
named commit or states no figure for the range holding it. Six sentences
state a figure or a modification time for a set they name by the branch, by
the tip, or by nothing, and the measured quantity moved while each sentence
stayed. The newest preamble carries two of them, and its own second paragraph
names `33666b1` for the very pair the first paragraph leaves unanchored. Four
older preambles carry the shape the Phase 24 and the Phase 25 preamble had
already dropped.

### HIGH: the newest preamble's gate verdict names no commit

- [X] T213 Anchor the whole-range prose-gate figure the Phase 26 preamble records at `specs/007-counters-and-timers/tasks.md:1461-1462`, which reads that `python3 tools/prose/prose_gate.py --check all` `exits 0 over 116 sources and 8617 units with 0 findings and 1 skipped` in the position of a claim about the range that command scans, where the command carries no `--head`, its range runs from the merge base `65beada` to the head and moves with every commit that lands, this pass measured the command reporting 117 sources and 8753 units at exit 0, and the pair reproduces at `--head 33666b1` alone, which the rewrite that moved that commit changed to 116 sources and 8620 units; the sentence must name `33666b1` as the commit whose range reproduces it or state no number, the anchoring T203 applied to the Phase 23 figure and T200 applied to the Phase 21 preamble, and this preamble must record no figure for the range holding it (HIGH, Constitution X.4, XI.6, `contradicts`)

### MEDIUM: four older preambles record a figure no commit the branch carries reproduces

- [X] T214 Anchor the four whole-range prose-gate figures the Phase 14, Phase 16, Phase 17, and Phase 18 preambles record at `specs/007-counters-and-timers/tasks.md:469-470`, `:606-607`, `:701`, and `:800-801`, which read `99 sources and 6693 units`, `101 sources, 7246 units`, `103 sources and 7441 units`, and `106 sources and 7666 units` against `prose-lint` and `python3 tools/prose/prose_gate.py --check all` with no commit named in any of the four, where the smallest figure the branch reproduces is the 107 sources and 7846 units `--head 11bc422` reports, so none of the four pairs belongs to a range the branch carries; each of the four sentences must state no number or name a commit the branch carries, the shape T206 applied to the `--mode tree` totals in six preambles (MEDIUM, Constitution X.4, `contradicts`)

### LOW: the newest preamble's freshness claim names its scope and leaves the tree out

- [X] T215 Name the tree the coverage-trace freshness claim the Phase 26 preamble records at `specs/007-counters-and-timers/tasks.md:1480-1483` was measured against, where the sentence reads that the trace `carries mtime 2026-09-27 23:46:53` and that `the newest source, header, test, or example file in the tree is source/counters/fold.cpp at 23:34:53`, and it names the scope while leaving the tree unnamed, and T205 required both halves at the site it fixed, `specs/007-counters-and-timers/tasks.md:1265-1266`, which names `22cb41c`; the claim must name the commit whose tree the modification times were read from (LOW, Constitution X.4, `partial`)

## Phase 28: Convergence

Appended by `/speckit.converge` after an audit of the branch tip at `e95e413`
and of the residue the nine waves before it left. Nothing above this line
changed.

Audit evidence, all produced by this pass from the repository root.
`python3 tools/prose/prose_gate.py --check all` exits 0 with 0 findings and 1
skipped, and `cmake -P cmake/prose-lint.cmake` exits 0 on the same verdict.
This preamble records no figure for the range holding it, and no `--mode tree`
total, because that form collects its files with `git ls-files` and reads them
from the working tree, the reading the Phase 21 preamble states at
`specs/007-counters-and-timers/tasks.md:1112-1119`.
`cmake --preset=dev` and `cmake --build --preset=dev` exit 0, and
`ctest --preset=dev` exits 0 with 100.0 percent of 37 tests passed, 0 failed,
0 skipped. `cmake --preset=ci-ubuntu` and `cmake --build build`, the release
build Principle IX requires once per feature, exit 0 with zero compiler
warnings in the project's own C++ sources. `format-check` and `dbc-gate`
exit 0, the pair reporting 135 interfaces with 0 gaps in the doc gate and in
the pair gate. `python3 tools/pmu_events/update_pmu_events.py --check` exits 0,
`cmake -P cmake/spell.cmake` exits 0, and `bash tools/dbc/coverage_gate.sh
build/coverage/coverage.info` exits 0 at lines 100.0 percent (1955 of 1955),
branches 100.0 percent (705 of 705), and functions 98.0 percent (289 of 295)
on an axis no gate scores. `cmake --preset=ci-sanitize`,
`cmake --build --preset=ci-sanitize`, and `ctest --preset=ci-sanitize` exit 0
three times with 37 of 37. `ctest --test-dir build -N` reports 37 tests of
which 16 match `-R counters`. The commit-title limit holds on the range from
the merge base with `origin/master`, `65beada`, to `e95e413`, whose 51 titles
measure 50 characters or fewer, and the ten titles from `11bc422` measure 41,
37, 44, 50, 45, 44, 50, 50, 49, and 46 characters.

Coverage of the check: 127 requirement keys (50 functional requirements
numbering `FR-001` through `FR-050` with no gap, 10 success criteria numbering
`SC-001` through `SC-010` with no gap, 49 user-story acceptance scenarios
across 8 stories, and 18 spec edge cases), 29 plan decision keys, and 11
constitution principles with X.1 through X.4 and XI.1 through XI.6 read one by
one. Re-measured here: the glob `include/speedgun-ng/counters*.hpp` matches
the 9 headers `specs/007-counters-and-timers/plan.md:35`, `:298`, and `:325`
record, `find source/counters -name '*.cpp'` returns the 11 translation units,
and all 27 `file:line` citations `spec.md`, `plan.md`, `quickstart.md`,
`data-model.md`, `research.md`, and the three contracts place resolve to an
existing file and a line inside it, with every single-line citation naming a
non-blank line, so the citation class holds closed in the live artifacts.
Six findings: 5 `contradicts`, 1 `partial`; 2 HIGH, 2 MEDIUM, 2 LOW, and
none of the six a constitution MUST violation. No requirement is missing or
partially built, no requirement is contradicted by the code, and no plan
decision is unmet. What is left is the audit record and the one measurement
claim four sites share, and the code the earlier waves converged is untouched
by all six findings.

The class the earlier passes closed is closed everywhere except the sites this
pass reached. Every `--mode tree` site outside the two named below states how
that form is read and carries no number, the coverage-trace claims at
`specs/007-counters-and-timers/tasks.md:1265-1266` and `:1480-1483` name their
tree, the Phase 13 header at `:427` carries no status marker, and every commit
body that names a commit for its prose-gate figure names one the branch
carries, apart from the two bodies T219 names. Every sentence the six findings
below name still states a figure without naming a run and a tree a reader
holds, and the measured quantity moved while each sentence stayed.

### HIGH: the newest preamble states a figure for the range holding it

- [X] T216 Drop the whole-range prose-gate figure the Phase 27 preamble records at `specs/007-counters-and-timers/tasks.md:1572-1574`, which reads that `python3 tools/prose/prose_gate.py --check all` `exits 0 over 117 sources and 8753 units with 0 findings and 1 skipped` in the position of a claim about the range that command scans, where the command carries no `--head`, its range runs from the merge base `65beada` to the head, this pass measured the command reporting 118 sources and 8852 units at exit 0, the figure at `eaf7ef1` the rewrite that moved that commit changed to 118 sources and 8860 units, and `specs/007-counters-and-timers/tasks.md:1634` requires the preamble holding the range to record no figure for it; the sentence must state the exit code alone, the shape T213 applied to the Phase 26 preamble, and the same sentence's own second paragraph already names `eaf7ef1` for the per-commit pairs (HIGH, Constitution X.4, XI.6, `contradicts`)

### HIGH: the acceptance row and the page state a multiplex range no recorded run produces

- [X] T217 Name the runs behind the multiplex fraction the SC-004 verdict row states at `specs/007-counters-and-timers/quickstart.md:140`, which reads that with 64 events open against this PMU `the kernel ran them 0.099677 to 0.599634 of the enabled time`, and the round figure the page states at `docs/pages/counters-overhead.md:279` and `:334` and the plan states at `specs/007-counters-and-timers/plan.md:423`, which read `about a tenth of the enabled time`, where neither endpoint appears in the tree or under `.omo/evidence/007-counters-and-timers/`, the 26 values the build logs and the evidence logs carry span 0.415196 to 0.599624, the two evidence logs the row names carry 0.428174 and 0.543263, and `ctest --test-dir build/dev -R counters_pmu -V` measured 0.597782 on this host, so no run behind either sentence is named and the low endpoint is below every value the tree records; each of the four sentences must name the runs and the hosts the fraction came from, or state the range the named runs carry (HIGH, Constitution X.4, `contradicts`)


### MEDIUM: two preambles state a `--mode tree` figure and name no reading

- [X] T218 State how the gate's `--mode tree` form is read at the two sites the earlier passes left, `specs/007-counters-and-timers/tasks.md:716` and `:846`, which read that the tree `carries 108 findings of the same class across 182 sources` and that `106 of the 108 findings` stayed in place, where the form collects its files with `git ls-files` and reads them from the working tree, so no commit reproduces its total, the reading T206 placed at `specs/007-counters-and-timers/tasks.md:528`, `:702`, `:801`, `:899`, `:1007`, and `:1267` and derived at `:1112-1119`; each of the two sentences must carry no number and must point at the reasoning `specs/007-counters-and-timers/tasks.md:1112-1119` records (MEDIUM, Constitution X.4, `contradicts`)

### MEDIUM: two commit bodies anchor their figure to a commit the branch no longer carries

- [X] T219 Name a commit the branch carries for the prose-gate figure the body of commit `33666b1` records and for the gate verdict the body of commit `22cb41c` records, where the first reads that the gate `exits 0 over 115 sources and 8523 units with 0 findings and 1 skipped` at a commit `git rev-list --all` does not reach, and the second reads that the gate `exits 0` at a second such commit, so each body describes a range no branch holds and no reader reproduces, and the shape T210 and T212 applied to the two bodies the earlier rewrite reached named a commit the branch carries; each body must name a commit the branch carries or state the exit code alone, and a message rewrite of either is the pre-merge rewrite the Pull Request Quality section permits, the same operation T210 and T212 record (MEDIUM, Constitution X.4, `contradicts`)

### LOW: the newest preamble's per-commit list names its set by the branch and its last figure by a checkout

- [X] T220 Anchor the two claims the Phase 27 preamble makes at `specs/007-counters-and-timers/tasks.md:1575-1576` and `:1581`, which read that the pass covers `each of the nine commits the branch carries, from 11bc422 to eaf7ef1` and that the run reports `117 sources and 8753 units at eaf7ef1`, where the branch carries ten commits from `11bc422` to `e95e413` and the last commit is the one this preamble lives in, so a count of the branch moves with every commit that lands, and `python3 tools/prose/prose_gate.py --check all --head eaf7ef1` reports 117 sources and 8752 units in this tree while a clean checkout of `eaf7ef1` reports 8753, a pair the rewrite that moved that commit changed to 117 sources and 8758 in this tree and 8759 in a clean checkout, because the gate reads each candidate file from the working tree, so the recorded pair describes a tree the reader does not hold; the sentences must name the commits the set covers, and the figure must be the pair the tree named reproduces or the sentence must state no number, the shape T208 applied to the two earlier lists (LOW, Constitution X.4, `partial`)

### LOW: the newest body counts the shas it names by line and calls the total mentions

- [X] T221 State the unit the count the body of commit `e95e413` records at its lines 27-28 counts, which reads that `The fifteen mentions of the two superseded shas that remain in this file sit inside closed tasks and preambles`, where the file carries the two shas on 15 lines and in 19 occurrences, so the figure the sentence holds by line count is four short of the occurrences a reader counts with `grep -o`, and the second half holds since all 15 lines sit in a phase preamble or a closed task body; the sentence must name the unit it counts (LOW, Constitution X.4, `contradicts`)

## Phase 29: Convergence

Appended by `/speckit.converge` after an audit of the branch tip at `6121416`
and of the residue the ten waves before it left. Nothing above this line changed.

Audit evidence, all produced by this pass from the repository root.
`python3 tools/prose/prose_gate.py --check all` exits 0, and `cmake -P
cmake/prose-lint.cmake` exits 0 on the same verdict. This preamble records no
source or unit figure for a range, because every such figure moves with the
commit that states it, and no `--mode tree` total, because that form collects
its files with `git ls-files` and reads them from the working tree, which is
the reading the Phase 21 preamble states at
`specs/007-counters-and-timers/tasks.md:1112-1119`. `cmake --preset=dev` and
`cmake --build --preset=dev` exit 0, and `ctest --preset=dev` exits 0 with
100.0 percent of 37 tests passed, 0 failed, 0 skipped. `cmake --preset=ci-ubuntu`
and `cmake --build build`, the release build Principle IX requires once per
feature, exit 0 with zero compiler warnings in the project's own C++ sources.
`format-check` and `dbc-gate` exit 0, the pair reporting 135 interfaces with 0
gaps in the doc gate and in the pair gate. `python3
tools/pmu_events/update_pmu_events.py --check` exits 0, `cmake -P
cmake/spell.cmake` exits 0, and `bash tools/dbc/coverage_gate.sh
build/coverage/coverage.info` exits 0 at lines 100.0 percent (1955 of 1955),
branches 100.0 percent (705 of 705), and functions 98.0 percent (289 of 295) on
an axis no gate scores. That trace carries mtime 2026-09-27 23:46:53, later
than the newest source, header, test, or example file in the tree this pass
read, `source/counters/fold.cpp` at 23:34:53. `ctest --test-dir build -N`
reports 37 tests of which 16 match `-R counters`. The commit-title limit holds
on the range from the merge base with `origin/master`, `65beada`, to `6121416`,
whose 52 titles measure 50 characters or fewer, and the eleven titles from
`11bc422` measure 41, 37, 44, 50, 45, 44, 50, 50, 49, 46, and 50 characters.

Coverage of the check: 127 requirement keys (50 functional requirements
numbering `FR-001` through `FR-050` with no gap, 10 success criteria numbering
`SC-001` through `SC-010` with no gap, 49 user-story acceptance scenarios across
8 stories, and 18 spec edge cases), 29 plan decision keys, and 11 constitution
principles with X.1 through X.4 and XI.1 through XI.6 read one by one.
Re-measured here: the glob `include/speedgun-ng/counters*.hpp` matches the 9
headers `specs/007-counters-and-timers/plan.md:35`, `:298`, and `:325` record,
`find source/counters -name '*.cpp'` returns the 11 translation units, and every
`file:line` citation `spec.md`, `plan.md`, `quickstart.md`, `data-model.md`,
`research.md`, and the three contracts place resolves to an existing file and a
line inside it, with every single-line citation naming a non-blank line, so the
citation class holds closed in the live artifacts. The `LCOV_EXCL` counts hold
as well: the feature scope carries 304 tokens, the whole `source/` plus
`include/` tree carries 308, and the 4 tokens in the difference sit in
`include/speedgun-ng/dbc.hpp`, the figures the Phase 19 preamble records at
`specs/007-counters-and-timers/tasks.md:925-928`. Ten findings: 9
`contradicts`, 1 `partial`; 2 HIGH, 3 MEDIUM, 5 LOW, and none of the ten a
constitution MUST violation. No requirement is missing or partially built, no
requirement is contradicted by the code, and no plan decision is unmet. What is
left is the audit record and the design artifacts the settled amendments never
reached, and the code the earlier waves converged is untouched by all ten
findings.

The two classes the earlier passes closed hold in the places they closed them.
Every `--mode tree` site outside this preamble states how that form is read and
carries no number, the coverage-trace claims name their tree, the Phase 13
header at `specs/007-counters-and-timers/tasks.md:427` carries no status marker,
and each of the ten bodies from `11bc422` to `e95e413` states its prose-gate
figure against a commit it names or states no source or unit figure. The one
body that reintroduces the shape is the tip. The second shape those waves
closed, a requirement amended in one artifact and restated unchanged in another,
has ten sites across six artifacts that the amendments of FR-022, FR-024,
FR-029, FR-034, and FR-046 never reached.

### HIGH: the tip body states a figure for the range holding it

- [X] T222 Rewrite the body of commit `6121416` so its prose-gate figure names the commit whose range reproduces it, or states the exit code alone, where the message reads `The prose gate run at the branch tip exits 0 over 118 sources and 8862 units with 0 findings and 1 skipped` and `python3 tools/prose/prose_gate.py --check all --head 6121416` reports 119 sources and 8970 units while the 118 and 8862 pair reproduces at `--head e95e413`, and where it is the one body from `11bc422` to `6121416` that states a whole-range figure without naming a commit, the other ten carrying either a named commit or no figure; the pre-merge rewrite the Pull Request Quality section permits applies, and the message must keep its template, its `Refs:` and `Approved-by:` footers, and the 50-character title `tools/prose/prose_rules.yaml:11` sets (HIGH, Constitution X.4, XI.6, T213, T216, T219, contradicts)

### HIGH: five sites still describe a mismatch FR-024 made unrepresentable

- [X] T223 Restate the five sites that describe a group target or clock mismatch as a construction error, at `specs/007-counters-and-timers/data-model.md:83` and `:90`, `specs/007-counters-and-timers/contracts/measurement-contract.md:48` and `:107`, and `specs/007-counters-and-timers/research.md:81`, where FR-024 at `specs/007-counters-and-timers/spec.md:252` states that a target or clock-identity mismatch across group members is unrepresentable and that the two construction errors a single target can fail on are a leaf the catalog reports as not `countable` and a window a provider refuses to open, where `source/counters/plan.cpp:469` stores the one `bound_target` for the whole plan and `source/counters/detail/core.hpp:104` declares it with no cross-member identity check anywhere in `source/counters/`, and where the Phase 14 preamble recorded that absence before T147 amended FR-024; each of the five sentences must state the single-plan-target design, and no requirement, entity row, or contract clause is renumbered or dropped (HIGH, FR-024, T147, contradicts)

### MEDIUM: six sites still claim a read path free of dynamic dispatch

- [X] T224 Restate the six sites that claim a read path carrying no dynamic dispatch, at `specs/007-counters-and-timers/contracts/provider-contract.md:13`, `specs/007-counters-and-timers/contracts/measurement-contract.md:56`, `specs/007-counters-and-timers/data-model.md:82`, and `specs/007-counters-and-timers/research.md:33`, `:35`, and `:37`, where FR-022 at `specs/007-counters-and-timers/spec.md:250` states that a read group is entered through the direct-call thunk its window installed in its constructor, that the five shipped windows reach `read_points` with no vtable lookup, and that a provider window installing no thunk reaches it through the vtable at one lookup per sampling action, and where `example/counters_giraffe_example.cpp:46` installs no thunk, so that fallback is a live path the giraffe example runs; each of the six sentences must name the seam and its per-action cost (MEDIUM, FR-022, T168, contradicts)

### MEDIUM: the quickstart's closing paragraph names no pass and a withdrawn reason

- [X] T225 Name the pass the closing paragraph of the success-criteria index records at `specs/007-counters-and-timers/quickstart.md:148-153`, which reads `Rows this pass could not close` and gives the gating reasons `/sys/devices/system/cpu/tsc_khz` absent, `perf_event_paranoid` 2, and the `rdpmc` page mode 0400 and root-owned, where the SC-004 row at `specs/007-counters-and-timers/quickstart.md:140` records the fast regime measured with both distributions published, and where T141 withdrew the kernel-gate reason the paragraph names; the paragraph must name the pass and the tree it describes, or state that it records a superseded pass, and no verdict, evidence filename, or figure in the table changes (MEDIUM, SC-004, T141, contradicts)

### MEDIUM: the contract names five scope-misuse sequences where three are enforced

- [X] T226 Restate the scope-misuse list at `specs/007-counters-and-timers/contracts/measurement-contract.md:119`, which names five tier-3 sequences including `use-after-finish` and `registering a composite into a started scope`, where the spec edge case at `specs/007-counters-and-timers/spec.md:199` and the clarification at `specs/007-counters-and-timers/spec.md:36` name three enforceable sequences, record that a finished scope is a settled window, and state that the registration has no spelling, and where `source/counters/plan.cpp:330` refuses a second start, `source/counters/plan.cpp:344-345` refuses `finish` without `start` and a second `finish`, `source/counters/fold.cpp:289-290` refuses `metric` on a window that is not closed, and `test/source/counters_fake_test.cpp:488-494` asserts ten further `metric` calls on a finished scope succeed with zero provider reads; the list must name the three enforced sequences and cite the amended edge case (MEDIUM, FR-046, T155, contradicts)

### LOW: three research-record sentences the settled amendments left behind

- [X] T227 Restate the arena allocation sentence at `specs/007-counters-and-timers/research.md:41`, which reads that the arena allocates at plan finalization, where FR-029 at `specs/007-counters-and-timers/spec.md:257` states that the plan-arena buffer is allocated at construction and `source/counters/plan.cpp:223` and `:239` allocate one arena per `plan::recorder` call, the boundary T126 and T160 settled for `specs/007-counters-and-timers/tasks.md:88` (LOW, FR-029, T126, T160, contradicts)
- [X] T228 Restate the calibration boundary at `specs/007-counters-and-timers/research.md:57`, which reads that the `tsc` frequency is calibrated at system-open, where FR-034 at `specs/007-counters-and-timers/spec.md:265` states provider construction and the rationale that the catalog freezes at the open boundary, the boundary T127 and T153 settled for FR-034 (LOW, FR-034, T153, contradicts)
- [X] T229 Restate the two thread-check sentences at `specs/007-counters-and-timers/research.md:67` and `specs/007-counters-and-timers/research.md:121`, which read that the recorder thread check is one cached `thread::id` compare, where the comment at `source/counters/plan.cpp:52-54` states the design in its own words: the cached `bound_thread` names the one allowed thread and the current thread's identity is read per call, because caching it would cache the answer for the thread that cached it, and `source/counters/plan.cpp:61` makes the fresh `std::this_thread::get_id()` query, which is what T129 recorded (LOW, FR-031, T129, contradicts)

### LOW: a header enumeration and a test count the tree has outgrown

- [X] T230 Extend R-001's header enumeration at `specs/007-counters-and-timers/research.md:9` to the nine public headers the tree carries, naming `counters_clock.hpp`, `counters_push.hpp`, and `counters_pmu.hpp` beside the six it lists, where the glob `include/speedgun-ng/counters*.hpp` matches 9 headers and `specs/007-counters-and-timers/plan.md:35`, `:298`, and `:325` record nine, and where T117 made the same amendment at `specs/007-counters-and-timers/tasks.md:30`; no R-entry, decision, or alternative is renumbered (LOW, T117, R-001, partial)
- [X] T231 Correct the test-executable count the Phase 23 preamble records at `specs/007-counters-and-timers/tasks.md:1312`, which reads `the 11 counters test executables` in a sentence enumerating what the tree holds, where `test/CMakeLists.txt` carries 12 `add_executable(counters_*)` calls at `:145`, `:150`, `:180`, `:187`, `:196`, `:210`, `:220`, `:228`, `:236`, `:263`, `:276`, and `:299`, and where the seam test that makes the twelfth arrived in commit `19902b2`; the count must read twelve, or the sentence must name the eleven project-structure sources `specs/007-counters-and-timers/plan.md:147-164` lists, and the sibling counts the same paragraph records, the 9 headers and the 11 translation units, are correct and do not move (LOW, Constitution X.4, contradicts)

## Phase 30: Convergence

Appended by `/speckit.converge` after an audit of the branch tip at `d522396`
and of the residue the eleven waves before it left. Nothing above this line
changed.

Audit evidence, all produced by this pass from the repository root.
`python3 tools/prose/prose_gate.py --check all` exits 0 and
`cmake -P cmake/prose-lint.cmake` exits 0 on the same verdict. This preamble
records no source or unit figure for the range holding it, and no
`--mode tree` total, because that form collects its files with `git ls-files`
and reads them from the working tree, which is the reading the Phase 21 preamble
states at `specs/007-counters-and-timers/tasks.md:1112-1119`.
`cmake --preset=dev` and `cmake --build --preset=dev` exit 0, and
`ctest --preset=dev` exits 0 with 100.0 percent of 37 tests passed, 0 failed, 0
skipped. `cmake --preset=ci-ubuntu` and `cmake --build build`, the release build
Principle IX requires once per feature, exit 0 with zero compiler warnings in
the project's own C++ sources. `format-check` and `dbc-gate` exit 0, the pair
reporting 135 interfaces with 0 gaps in the doc gate and in the pair gate.
`python3 tools/pmu_events/update_pmu_events.py --check` exits 0, `cmake -P
cmake/spell.cmake` exits 0, and `bash tools/dbc/coverage_gate.sh
build/coverage/coverage.info` exits 0 at lines 100.0 percent (1955 of 1955),
branches 100.0 percent (705 of 705), and functions 98.0 percent (289 of 295) on
an axis no gate scores. `ctest --test-dir build -N` reports 37 tests of which 16
match `-R counters`. The commit-title limit holds on the twelve commits the
branch carries from `11bc422` to `d522396`, whose titles measure 41, 37, 44, 50,
45, 44, 50, 50, 49, 46, 50, and 49 characters against the limit
`tools/prose/prose_rules.yaml:11` sets.

Coverage of the check: 127 requirement keys (50 functional requirements
numbering `FR-001` through `FR-050` with no gap, 10 success criteria numbering
`SC-001` through `SC-010` with no gap, 49 user-story acceptance scenarios across
8 stories, and 18 spec edge cases), 29 plan decision keys, and 11 constitution
principles with X.1 through X.4 and XI.1 through XI.6 read one by one.
Re-measured here: the glob `include/speedgun-ng/counters*.hpp` matches the 9
headers `specs/007-counters-and-timers/plan.md:35`, `:298`, and `:325` record,
`find source/counters -name '*.cpp'` returns the 11 translation units, and the
`LCOV_EXCL_*` counts hold as the Phase 29 preamble records them, 304 tokens in
the feature scope, 308 across `source/` plus `include/`, and 4 in
`include/speedgun-ng/dbc.hpp`. Two findings: 2 `contradicts`; 0 CRITICAL,
2 HIGH, and neither a constitution MUST violation. No requirement is missing or
partially built. The code the earlier waves converged is untouched by both
findings: each sits in the requirement text and in the task records that
describe it.

The classes the earlier passes closed hold where they closed them. The multiplex
range, the FR-024 mismatch claim, the FR-022 dispatch claim, and every
unanchored figure stay closed, and the four re-anchorings `T196`, `T197`,
`T190`, and `T192` landed hold. The class they did not exhaust is the one a
settled amendment leaves behind: a guarantee and a mechanism the T131..T141
work reversed, still written in the requirement that names them, in the
artifacts that restate that requirement, and in the two Phase 9 tasks that
recorded the design the amendment replaced. `T076` and `T079` each named one
half of it and closed without the text moving, the sibling-occurrence class
`T175` recorded.

### HIGH: FR-023 names a boundary the code does not use for read-mode assignment

- [X] T232 Reconcile FR-023 at `specs/007-counters-and-timers/spec.md:251`, which requires the system to assign each leaf a read mode at plan compile, with the code, which assigns it during provider enumeration and nowhere in `compile()`: `source/counters/linux_pmu/provider.cpp:464-465` derives `fast_capable` and `probe_device` writes `entry.mode` at `:118-120` inside the seed loop `enumerate` calls at `:505`, `source/counters/clock_provider.cpp:248-283` and `source/counters/push_provider.cpp:83` set their modes the same way, a search for the token `mode` over `source/counters/plan.cpp` returns no line, and `source/counters/linux_pmu/group_io.cpp:391` reads the mode the catalog entry already carries; the four restatements of the same phrase at `specs/007-counters-and-timers/spec.md:157`, `specs/007-counters-and-timers/data-model.md:47`, `specs/007-counters-and-timers/contracts/provider-contract.md:44`, and `specs/007-counters-and-timers/tasks.md:186` must move with the requirement, `T076` at `specs/007-counters-and-timers/tasks.md:364` recorded the same correction and closed without it landing, the amended text must state where the probe runs relative to the plan target bound at `source/counters/plan.cpp:469` so a cpu-pinned plan's fast-mode eligibility is traceable, and FR-023's disclosure clause and C-PRO-4 at `specs/007-counters-and-timers/contracts/provider-contract.md:76` keep their present wording (HIGH, FR-023, FR-009, FR-031, `contradicts`)

### HIGH: five sentences record the fast-read design the T131..T141 work replaced

- [X] T233 Restate the five sentences that describe the mapped-page read as it stood before T137..T139: `specs/007-counters-and-timers/plan.md:124` names a `reinterpret_cast` at the ABI boundary and `:126` a `48-bit mask`, `specs/007-counters-and-timers/tasks.md:185` names the mask `(val + offset) & 0xFFFFFFFFFFFF` and a `P2 reinterpret_cast`, and `specs/007-counters-and-timers/research.md:89` names the `perf_user_access` sysctl on affected Intel parts beside that same `48-bit counter-width mask`, `:91` names `Every gate (sysctl, version, capability, index) is host state the probe reads at plan compile`, and `:93` names `assuming perf_user_access is on`; the code takes the mask from the width the event page publishes at `source/counters/linux_pmu/fast_read.cpp:271` and `:97`, casts with `static_cast` at `:195`, `:261`, and `:295` over the kernel's own `perf_event_mmap_page` aliased at `:165`, a search for `perf_user_access` over `source/`, `include/`, and `test/` returns no line, the mirrored page fields `T076` named are gone, and `specs/007-counters-and-timers/plan.md:362`, `:365-367`, and `:453` beside `specs/007-counters-and-timers/spec.md:326` and `specs/007-counters-and-timers/contracts/provider-contract.md:67` already carry the settled wording; each of the five sentences must name the page-published width and the standard conversion, the closed journal `specs/007-counters-and-timers/sg_counters.md` is a dated record and keeps its text, and no code, requirement, gate, or exclusion marker may move (HIGH, FR-040, Constitution I, IV, T079, T137, T139, `contradicts`)

## Phase 31: Convergence

Appended by `/speckit.converge` after an audit of the branch tip at `a8ed1d6`
and of the residue the twelve waves before it left. Nothing above this line
changed.

Audit evidence, all produced by this pass from the repository root.
`python3 tools/prose/prose_gate.py --check all` exits 0, and
`cmake -P cmake/prose-lint.cmake` exits 0 on the same verdict. This preamble
records no source or unit figure for the range holding it, and no
`--mode tree` total, because that form collects its files with `git ls-files`
and reads them from the working tree, which is the reading the Phase 21
preamble states at `specs/007-counters-and-timers/tasks.md:1112-1119`.
`cmake --preset=dev` and `cmake --build --preset=dev` exit 0, and
`ctest --preset=dev` exits 0 with 100.0 percent of 37 tests passed, 0 failed,
0 skipped. `cmake --preset=ci-ubuntu` and `cmake --build build`, the release
build Principle IX requires once per feature, exit 0 with zero compiler
warnings in the project's own C++ sources. `format-check` and `dbc-gate`
exit 0, the pair reporting 135 interfaces with 0 gaps in the doc gate and in
the pair gate. `python3 tools/pmu_events/update_pmu_events.py --check` exits 0,
`cmake -P cmake/spell.cmake` exits 0, and `bash tools/dbc/coverage_gate.sh
build/coverage/coverage.info` exits 0 at lines 100.0 percent (1955 of 1955),
branches 100.0 percent (705 of 705), and functions 98.0 percent (289 of 295)
on an axis no gate scores. `ctest --test-dir build -N` reports 37 tests of
which 16 match `-R counters`. The commit-title limit holds on the thirteen
commits the branch carries from `11bc422` to `a8ed1d6`, whose titles measure
41, 37, 44, 50, 45, 44, 50, 50, 49, 46, 50, 49, and 48 characters against
the limit `tools/prose/prose_rules.yaml:11` sets, and every one of the
thirteen titles opens with a section token `tools/prose/prose_rules.yaml:19-29`
lists.

Coverage of the check: 127 requirement keys (50 functional requirements
numbering `FR-001` through `FR-050` with no gap, 10 success criteria numbering
`SC-001` through `SC-010` with no gap, 49 user-story acceptance scenarios across
8 stories, and 18 spec edge cases), 29 plan decision keys, and 11 constitution
principles with X.1 through X.4 and XI.1 through XI.6 read one by one.
Re-measured here: the glob `include/speedgun-ng/counters*.hpp` matches the 9
headers `specs/007-counters-and-timers/plan.md:35`, `:298`, and `:325` record,
`find source/counters -name '*.cpp'` returns the 11 translation units, the
FR-022 thunk seam holds at all five shipped window constructors
(`source/counters/clock_provider.cpp:165`, `source/counters/push_provider.cpp:21`,
`source/counters/fake_provider.cpp:38`,
`source/counters/linux_pmu/group_io.cpp:192` and `:302`), the
`SG_REQUIRE_ALWAYS` bounds site stands at `source/counters/plan.cpp:366`, the
`hard_stop` and `ring` cores allocate at `source/counters/plan.cpp:223` and
`:239`, and the exactness, cadence, and fold-endpoint figures
`include/speedgun-ng/counters_measurement.hpp:44-60` and `:66-72` publish
match `docs/pages/counters-overhead.md:160-163` and `:170-181`. Seven
findings: 7 `contradicts`; 0 CRITICAL, 2 HIGH, 4 MEDIUM, 1 LOW, and none of
the seven a constitution MUST violation. No requirement is missing or
partially built, and no plan decision is unmet. Every finding sits in a
sentence the code contradicts, which is the class the settled amendments
leave behind, and five of the seven name a site no earlier wave reached.

The classes the earlier passes closed hold where they closed them. The
multiplex range, the FR-024 mismatch claim, the FR-022 artifact dispatch
claim, FR-023's read-mode boundary, the `perf_user_access` and mask claims,
and every unanchored figure stay closed, and the four re-anchorings `T196`,
`T197`, `T190`, and `T192` landed hold. What the earlier waves did not reach
is the public surface: four of the seven findings sit in
`include/speedgun-ng/counters_measurement.hpp` and two of those name a
sentence T226 and T224 restated in the contract and the research record while
the header beside them kept the superseded wording.

### HIGH: the syscall group read allocates on the sampling path against FR-026

- [X] T234 Fix the unit mismatch in the group-read scratch guard at `source/counters/linux_pmu/group_io.cpp:213-225`, which compares `scratch.size()`, an element count, against `want`, a byte count, so the guard `scratch.size() < want` holds for every group of six or more members rather than the 65 the exclusion reason at `:215-222` states, and the growth arm then calls `scratch.resize(want)`, which sets the element count to a byte count and over-allocates by a factor of eight; `source/counters/linux_pmu/group_io.cpp:193` sizes the buffer at `kHeaderWords + 64` elements, so a measured sweep of the guard shows it firing at 6, 8, 64, 65, and 100 members, while FR-026 at `specs/007-counters-and-timers/spec.md:254` requires `recorder.sample()` to be `noexcept` with zero allocation, the doc comment at `include/speedgun-ng/counters_measurement.hpp:469-473` states zero allocation and zero lock, and `test/source/counters_pmu_test.cpp:571` already opens 64 member leaves in one plan, so the fast-regime test at `docs/pages/counters-overhead.md:242` and the oversubscription scenario both reach the guard; the comparison must be in one unit, the resize must size the element count from the member count, the allocation must leave the sampling path, and the exclusion reason must state the corrected boundary (HIGH, FR-026, Constitution VII, `contradicts`)

### HIGH: a header still names a scope misuse the code and the test permit

- [X] T235 Restate the scope-misuse list in the public header at `include/speedgun-ng/counters_measurement.hpp:984-987`, which names four sequences and calls each a contract violation, where `source/counters/fold.cpp:289-290` refuses `metric` only on a window that is not closed, so `metric` after `finish` is permitted, where `test/source/counters_fake_test.cpp:486-494` asserts ten further `metric` calls on a finished scope succeed with zero provider reads, where the edge case at `specs/007-counters-and-timers/spec.md:199` and the clarification at `specs/007-counters-and-timers/spec.md:36` name three enforceable sequences and record that a finished scope is a settled window, and where T226 restated `specs/007-counters-and-timers/contracts/measurement-contract.md:122` to the three without reaching the header; the list must name the three enforced sequences and cite the amended edge case, and the same sentence's claim that the API spells no registration entry keeps its place (HIGH, FR-046, T155, T226, `contradicts`)

### MEDIUM: the plan's scope-misuse sentence names a sequence the code permits

- [X] T236 Restate the scope-misuse sentence at `specs/007-counters-and-timers/plan.md:282`, which names `metric` before `finish`, double `start`, and `use-after-finish` as terminal contract violations, where `source/counters/fold.cpp:289-290` permits `metric` on a closed window and `test/source/counters_fake_test.cpp:486-494` asserts ten further calls succeed, so a use after `finish` is the settled window the edge case at `specs/007-counters-and-timers/spec.md:199` describes, and where the sentence omits the second `finish` that `source/counters/plan.cpp:344-345` refuses; the sentence must name the three enforced sequences and cite the amended edge case, and T226's restatement of the contract clause stands (MEDIUM, FR-046, T155, T226, `contradicts`)

### MEDIUM: the read-path header comment claims a seam the shipped fallback contradicts

- [X] T237 Restate the read-path claim at `include/speedgun-ng/counters_measurement.hpp:26-28`, which states the read path holds no dispatch, where `include/speedgun-ng/counters_provider.hpp:324-328` routes a window that installed no thunk through the vtable at one lookup per sampling action, where `example/counters_giraffe_example.cpp:46-57` installs no thunk, so the giraffe example runs on that path, and where FR-022 at `specs/007-counters-and-timers/spec.md:250` states the seam and its per-action cost after T168 amended it; the sentence must name the seam and its per-action cost, and T224's restatement of the six artifact sites stands (MEDIUM, FR-022, T168, T224, `contradicts`)

### MEDIUM: the plan and the measurement contract still claim a link-manifest shape the build does not produce

- [X] T238 Restate the three sentences that say the standalone example's link manifest names this library alone, at `specs/007-counters-and-timers/plan.md:33`, `specs/007-counters-and-timers/plan.md:436`, and `specs/007-counters-and-timers/contracts/measurement-contract.md:126`, where FR-049 at `specs/007-counters-and-timers/spec.md:286` states the target is a static archive whose manifest carries no `speedgun-ng` entry and names the platform C and C++ runtime, where the clarification at `specs/007-counters-and-timers/spec.md:31` records the same, and where T120 amended FR-049 and SC-001 without reaching these three; each sentence must state that the manifest demonstrates no third-party dynamic dependency and names the platform runtime, and T120's amendment to the requirement and the success criterion stands (MEDIUM, FR-049, SC-001, T120, `contradicts`)

### MEDIUM: the research record states a permission outcome the recorded measurement withdraws

- [X] T239 Restate the permission sentence at `specs/007-counters-and-timers/research.md:83`, which states that at paranoid 2 the provider reports `permission_blocked`, where `source/counters/linux_pmu/provider.cpp:439-443` reports that state only on a `perf_event_open` the kernel answers `EACCES`, where `docs/pages/counters-overhead.md:307-324` records the re-verification that at level 2 all 358 hardware entries probe `countable` with 0 `permission_blocked`, and names 3 or above as the level that refuses, where `specs/007-counters-and-timers/quickstart.md:138` records the same for the suite, and where the page states the earlier level-2 claim was never measured; the sentence must name the level the recorded re-verification shows refusing and point at that record, and T232's amendment of FR-023 stands (MEDIUM, FR-039, SC-002, `contradicts`)

### LOW: the exactness note publishes a rate the code contradicts

- [X] T240 Correct the rate at `include/speedgun-ng/counters_measurement.hpp:67-69`, which states that a count reaches `2^53` at roughly 285 events per nanosecond sustained for one second, where `2^53` is 9007199254740992 and one second holds 1000000000 nanoseconds, so the rate that reaches `2^53` in one second is about 9007199 events per nanosecond, and where `specs/007-counters-and-timers/spec.md:328` states the same limit as roughly 104 days at a sustained one-billion-per-second count, which `2^53 / 1e9` seconds confirms; the sentence must state the rate arithmetic supports and keep the conclusion that a counter-backed delta stays exact on a host this feature targets, and the cadence figures beside it stand (LOW, FR-032, Constitution IV, `contradicts`)

## Phase 32: Convergence

Appended by `/speckit.converge` after an audit of the branch tip at `02c7db8`
and of the residue the thirteen waves before it left. Nothing above this line
changed.

Audit evidence, all produced by this pass from the repository root.
`python3 tools/prose/prose_gate.py --check all` exits 0, and
`cmake -P cmake/prose-lint.cmake` exits 0 on the same verdict,
`prose-lint: 123 sources, 9324 units examined, 0 findings, 1 skipped`. This
preamble records no source or unit figure for the range holding that verdict
and no `--mode tree` total, because that form collects its files with
`git ls-files` and reads them from the working tree, the reading the Phase 21
preamble states at `specs/007-counters-and-timers/tasks.md:1112-1119`.
`ctest --test-dir build -N` exits 0 and reports `Total Tests: 37`, of which
`ctest --test-dir build -N -R counters` reports 16. `cmake --preset=dev` and
`cmake --build --preset=dev` exit 0, and `ctest --preset=dev` exits 0 with
100.0 percent of 37 tests passed, 0 failed, 0 skipped, in 43.42 s.
`cmake --preset=ci-ubuntu` and `cmake --build build`, the release build
Principle IX requires once per feature, exit 0 with no line matching `warning`
in the build log. `cmake --build build/dev -t dbc-gate` exits 0, reporting
`doc-gate: 135 interfaces, 0 gaps` and `pair-gate: 135 interfaces, 0 gaps`.
`cmake --build build/dev -t format-check` exits 0. `cmake -P cmake/spell.cmake`
exits 0. `python3 tools/pmu_events/update_pmu_events.py --check` exits 0.
`bash tools/dbc/coverage_gate.sh build/coverage/coverage.info` exits 0 at
lines 100.0 percent (1955 of 1955), branches 100.0 percent (705 of 705), and
functions 98.0 percent (289 of 295) on an axis no gate scores;
`find source include -newer build/coverage/coverage.info` names no file, so
that trace covers this tip.

Coverage of the check: 127 requirement keys (50 functional requirements
numbering `FR-001` through `FR-050` with no gap, 10 success criteria numbering
`SC-001` through `SC-010` with no gap, 49 user-story acceptance scenarios
across 8 stories, and 18 spec edge cases), 45 design keys (15 research
decisions `R-001` through `R-015`, 11 data-model entities `E-01` through
`E-11`, and 19 contract clauses `C-MEA-1` through `C-MEA-7`, `C-PRO-1` through
`C-PRO-6`, `C-SYS-1` through `C-SYS-6`), and 11 constitution principles with
X.1 through X.4 and XI.1 through XI.6 read one by one. Re-measured here: the
glob `include/speedgun-ng/counters*.hpp` matches the 9 headers
`specs/007-counters-and-timers/plan.md:35`, `:298`, and `:325` record;
`find source/counters -name '*.cpp'` returns the 11 translation units;
`readelf -d` on `build/dev/example/counters_standalone_example` and on
`build/dev/example/counters_giraffe_example` names `libstdc++`, `libgcc_s`,
and `libc` with no `speedgun-ng` entry in either; and
`./build/dev/test/counters_pmu_test` reports
`pmu catalog: 581 table-selected entries beyond kernel aliases` and
`pmu availability: 358 countable, 0 permission_blocked, 261 not_encodable, 356 fast_rdpmc`.
The `LCOV_EXCL` token counts hold at 301 in the feature scope and 305 across
`source/` plus `include/`, with 4 in `include/speedgun-ng/dbc.hpp`; T234
removed three with the growth arm it deleted. Eleven findings: 10 `contradicts`
and 1 `unrequested`; 0 CRITICAL, 3 HIGH, 3 MEDIUM, and 5 LOW. No finding is
`missing` or `partial`: every functional requirement this pass inspected has a
realization in the code or in the shipped public surface, and every finding
sits in a sentence an artifact states about that code.

Every finding below was re-derived in this session from the files it cites.
The Phase 12 through Phase 31 preambles were read for task identifiers, phase
grouping, and the file paths each task names, and no closure claim in them was
accepted as evidence. The two commits since `a8ed1d6` were read in full: `b270503`
deletes the sampling-path growth arm and sizes the group scratch by leaf count
(T234), and `02c7db8` restates six artifact sentences (T235..T240). Both hold
where they landed, with one exception recorded as T244 below. The class the
earlier waves did not exhaust has two sources. The first is a requirement whose
letter no implementation can satisfy as written while the read path keeps its
budget, which is T241. The second is the settled amendment's reach: a
requirement restated at one site and left standing at its siblings, which
T242, T243, T245, and T246 record, together with three smaller sites of the
same shape at T247, T248, and T249.

### HIGH: FR-011 names a per-action metadata yield the window contract does not provide

- [X] T241 Reconcile the yield FR-011 at `specs/007-counters-and-timers/spec.md:233` requires of a sampling action, `a cumulative raw uint64 point plus the leaf's unit and metadata (description, availability, caveats such as multiplex times)`, with the contract the library ships, where `point_sink::put` at `include/speedgun-ng/counters_provider.hpp:201` takes one `std::uint64_t` and yields that value alone, where `leaf_set` at `:131-134` carries addresses with nothing beside them, and where the unit, description, and availability reach a caller as `catalog_seed` fields at `:39-52` fixed at registration and carried into a fold through `points_view` at `include/speedgun-ng/counters_measurement.hpp:392-402`; five artifacts repeat a yield that has no realization: `specs/007-counters-and-timers/data-model.md:56`, `specs/007-counters-and-timers/research.md:33`, `specs/007-counters-and-timers/contracts/provider-contract.md:20` and `:76` (C-PRO-2), and T008 at `specs/007-counters-and-timers/tasks.md:43`, which asked for `per-leaf point + unit + metadata yield shapes`; the multiplex caveat FR-011 names is already realized as the ordinary enabled and running leaves at `source/counters/linux_pmu/group_io.cpp:258-262` and `:358-362`, and the unit and description are realized as the same leaves' catalog fields; a per-action string yield collides with FR-026 at `specs/007-counters-and-timers/spec.md:254`, which requires `recorder.sample()` to be `noexcept` with zero allocation, so FR-011 and its restatements must state that the action yields the cumulative point and that the unit, description, availability, and multiplex pair are catalog and plan facts reachable after measurement, and `example/counters_giraffe_example.cpp:49-53` must keep implementing only the point (HIGH, FR-011, FR-019, FR-020, FR-026, C-PRO-2, T008, plan: R-004, Constitution II, X.2, `contradicts`)

### HIGH: the Constitution Check row still registers a P2 exception the fast-read work withdrew

- [X] T242 Restate the Principle I row of the Constitution Check table at `specs/007-counters-and-timers/plan.md:43`, which registers the second P2 exception as `the static_cast of a void* mapping base to the provider-local mirror of the kernel's published perf page` and names `the read of a mapping through a hand-declared struct` as the part carrying the exception, where T137 replaced both hand-mirrored page structs with the kernel's own type, where `source/counters/linux_pmu/fast_read.cpp:165` reads `using event_page = perf_event_mmap_page;`, where the comment at `:234-236` states that no cast of a mirrored layout is involved and that no P2 exception is claimed, and where the three remaining conversions are standard casts of the mapping base at `:195`, `:261`, and `:295`; `specs/007-counters-and-timers/plan.md:124`, `:126`, and `:455` already carry the settled wording, T233 restated the five superseded sentences at `specs/007-counters-and-timers/plan.md:124` and `:126` beside `specs/007-counters-and-timers/tasks.md:185` and `specs/007-counters-and-timers/research.md:89`, `:91`, and `:93` without naming this row, and the closed journal `specs/007-counters-and-timers/sg_counters.md` is a dated record that keeps its text; the row must name the kernel's own `perf_event_mmap_page` and the standard conversion, must keep the `__rdtsc` and `_rdpmc` intrinsic entry whose site justification stands at `source/counters/clock_provider.cpp:149`, and no code, requirement, gate, or exclusion marker may move (HIGH, plan: Constitution Check I, Constitution I, X.2, FR-040, T137, T139, T233, `contradicts`)

### HIGH: two public headers still place read-mode assignment at plan compile

- [X] T243 Restate the two public-header sentences that place read-mode assignment at plan compile, `include/speedgun-ng/counters_core.hpp:89-90`, where the `read_mode` brief reads `The achieved read mechanism for a leaf, probed at plan compile and disclosed per catalog entry (FR-023)`, and `include/speedgun-ng/counters_measurement.hpp:1067-1072`, where the `compile` brief lists `group layout, mode probing, and arena geometry` among the steps that call performs, with the code, which assigns each leaf its mode during provider enumeration: `source/counters/linux_pmu/provider.cpp:505` calls `probe_device`, which writes `entry.mode` at `:118-120` inside that seed loop, `source/counters/clock_provider.cpp:248`, `:255`, `:262`, and `:283` and `source/counters/push_provider.cpp:83` seed their modes the same way, `source/counters/linux_pmu/group_io.cpp:391` reads the mode the catalog entry already carries, and the amended FR-023 at `specs/007-counters-and-timers/spec.md:251` names that boundary together with the plan bind at `source/counters/plan.cpp:469`, which T232 established and applied to `specs/007-counters-and-timers/spec.md:157`, `specs/007-counters-and-timers/data-model.md:47`, and `specs/007-counters-and-timers/contracts/provider-contract.md:46`; each header sentence must name the enumeration-time probe and the open boundary it precedes, and FR-023's disclosure clause together with C-PRO-4 at `specs/007-counters-and-timers/contracts/provider-contract.md:78` keep their present wording (HIGH, FR-023, FR-009, FR-031, T232, `contradicts`)

### MEDIUM: the read-path claim on `sample()` states a seam the shipped fallback contradicts

- [X] T244 Restate the read-path claim in the `sample()` brief at `include/speedgun-ng/counters_measurement.hpp:469-473`, which states `Zero allocation, zero lock, zero virtual call`, where the seam's own documentation at `include/speedgun-ng/counters_provider.hpp:246-251` records that a window supplying no thunk keeps `default_thunk` and pays `one vtable lookup per sampling action`, where the fallback at `:324-328` implements exactly that call, where `example/counters_giraffe_example.cpp:46-57` installs no thunk and runs on the fallback path, and where FR-022 at `specs/007-counters-and-timers/spec.md:250` states the seam and its per-action cost after T168 amended it; T237 restated the file brief at `include/speedgun-ng/counters_measurement.hpp:26-28` and named this method comment neither, and the claim must name the seam and its per-action cost while the five shipped windows stay dispatch-free (MEDIUM, FR-022, T146, T237, `contradicts`)

### MEDIUM: the provider contract still claims the link-manifest shape the build does not produce

- [X] T245 Restate the giraffe acceptance sentence at `specs/007-counters-and-timers/contracts/provider-contract.md:56`, which requires that the `link manifest names this library alone`, where FR-049 at `specs/007-counters-and-timers/spec.md:286` states that the target is a static archive whose manifest carries no `speedgun-ng` entry and names the platform C and C++ runtime, where the clarification at `specs/007-counters-and-timers/spec.md:31` records the same, and where `readelf -d build/dev/example/counters_giraffe_example` names `libstdc++`, `libgcc_s`, and `libc` alone; T238 restated the three sentences at `specs/007-counters-and-timers/plan.md:33` and `:436` beside `specs/007-counters-and-timers/contracts/measurement-contract.md:126` without naming this fourth site, and the sentence must state that the manifest demonstrates no third-party dynamic dependency and names the platform runtime (MEDIUM, FR-049, SC-001, T120, T238, `contradicts`)

### MEDIUM: T053 still names two probe inputs the settled fast read does not consult

- [X] T246 Restate the probe-input clause of T053 at `specs/007-counters-and-timers/tasks.md:186`, which names `the perf_user_access sysctl on affected Intel parts, version, pinning/index validity` as what the mode assignment reads, where a search for the token `perf_user_access` over `source/`, `include/`, and `test/` returns no line, where `docs/pages/counters-overhead.md:303-306` records that the sysctl is absent on this kernel and that `Neither is consulted`, where the capability bit and the counter width come from the event page at `source/counters/linux_pmu/fast_read.cpp:271-272` and the index gate at `:278-281`, and where the version-matching arithmetic that justified the mirrored pages is gone with the mirrors themselves; T232 corrected this same task's boundary sentence to the enumeration path and T233 restated `perf_user_access` at `specs/007-counters-and-timers/research.md:89` and `:93` beside `specs/007-counters-and-timers/tasks.md:185` without naming this occurrence, and the clause must name the capability bit, the one-based index, and the page-published width (MEDIUM, FR-023, FR-040, T053, T137, T233, `contradicts`)

### LOW: the research record names an errno the availability switch does not test

- [X] T247 Correct the permission clause of R-010 at `specs/007-counters-and-timers/research.md:83`, which states that the provider reports `permission_blocked` only on a `perf_event_open` the kernel answers `EACCES`, where the probe's switch at `source/counters/linux_pmu/provider.cpp:432-444` names `EINVAL`, `EOPNOTSUPP`, and `ENOENT` as the errnos that map to `not_encodable` and returns `permission_blocked` from its `default` arm, which covers `EACCES` together with every other refusal, and where the token `EACCES` appears in `source/` only inside the coverage-exclusion comment at `:439`; T239 rewrote this sentence to name the recorded re-verification and left the mechanism clause standing, and the clause must describe the switch the code ships (LOW, FR-039, Constitution IV, T239, `contradicts`)

### LOW: the measurement contract's compile signature names a plural target parameter

- [X] T248 Correct the `compile` pseudocode signature at `specs/007-counters-and-timers/contracts/measurement-contract.md:45`, which reads `std::expected<plan, error> compile(const system&, targets, /* expressions... */);` and names a plural `targets`, where FR-024 at `specs/007-counters-and-timers/spec.md:252` binds exactly one sampling target, where the clarification at `specs/007-counters-and-timers/spec.md:34` records the single-plan-target design, and where the two shipped overloads take one `const target&` at `include/speedgun-ng/counters_measurement.hpp:1081` and `:1098-1100`, which T086 and T147 settled; the signature must name the single `target` the plan binds (LOW, FR-024, T086, T147, `contradicts`)

### LOW: two design artifacts spell an alias accessor the library does not ship

- [X] T249 Reconcile the `object::alias()` return type the design artifacts spell with the one the library ships, where `specs/007-counters-and-timers/contracts/system-contract.md:36` declares `std::optional<std::string_view> alias() const;` and the class diagram at `specs/007-counters-and-timers/plan.md:204` spells the same, where `include/speedgun-ng/counters_system.hpp:49-55` returns `std::string_view` documented as empty when the object carries no alias, where `source/counters/system.cpp:449-452` returns `node->alias` with an empty string for an object that declares none, and where FR-002 at `specs/007-counters-and-timers/spec.md:221` requires that both spellings resolve to one object without naming a wrapper type; either the artifacts adopt the shipped accessor or the accessor adopts the artifacts, and no other interface, test, or gate changes (LOW, FR-001, FR-002, R-001, `contradicts`)

### LOW: the overhead page states two catalog totals that do not reconcile

- [X] T250 State the denominator behind the `589` at `docs/pages/counters-overhead.md:252-253`, which reports that `356 of the 589 core-PMU entries disclose fast_rdpmc`, where the table at `:321` counts `358 countable, 0 permission_blocked` beside `261` not encodable for the same catalog, which sum to 619, where the probe assigns every entry exactly one of those two states at `source/counters/linux_pmu/provider.cpp:425-444`, and where `./build/dev/test/counters_pmu_test` on this host prints `pmu availability: 358 countable, 0 permission_blocked, 261 not_encodable, 356 fast_rdpmc` beside `pmu catalog: 581 table-selected entries beyond kernel aliases`; the page names no denominator that reconciles 589 with 619 or with 581, so each published figure must name the set it counts, and no recorded figure may change (LOW, SC-004, SC-008, Constitution IV, Principle VII, `contradicts`)

### LOW: a public target enumerator has no producer and no consumer

- [X] T251 Justify or remove `target_kind::machine`, the enumerator at `include/speedgun-ng/counters_provider.hpp:139-143`, which no code, test, or example constructs and no code reads: a search over `source/`, `include/`, `test/`, and `example/` finds `target_kind` named at `source/counters/detail/pmu.hpp:295`, where the branch tests `target_kind::cpu` alone, and at `test/source/counters_fake_test.cpp:437` and `test/source/counters_pmu_test.cpp:702`, which both construct the cpu spelling, while `include/speedgun-ng/counters_provider.hpp:154` defaults a target to `thread`; FR-031 at `specs/007-counters-and-timers/spec.md:259` names `thread or cpu` as the two bound targets, and a machine-wide reading is unrepresentable because plans and recorders are per-thread objects, the binding enforced at `source/counters/plan.cpp:61-62`; the enumerator must either state the reading it adds over `thread` or leave the public surface (LOW, FR-031, Constitution X.2, `unrequested`)

## Phase 33: Convergence

Appended by `/speckit.converge` after an audit of the branch tip at `01f905b`
and of the residue the fourteen waves before it left. Nothing above this line
changed.

Audit evidence, all produced by this pass from the repository root.
`python3 tools/prose/prose_gate.py --check all` exits 0, and
`cmake -P cmake/prose-lint.cmake` exits 0 on the same verdict,
`prose-lint: 125 sources, 9529 units examined, 0 findings, 1 skipped`. This
preamble records no source or unit figure for the range holding that verdict and
no `--mode tree` total, because that form collects its files with
`git ls-files` and reads them from the working tree, which is the reading the
Phase 21 preamble states at
`specs/007-counters-and-timers/tasks.md:1112-1119`.
`ctest --test-dir build -N` reports `Total Tests: 37`, of which
`ctest --test-dir build -N -R counters` reports 16. `cmake --preset=dev` and
`cmake --build --preset=dev` exit 0, and `ctest --preset=dev` exits 0 with
100.0 percent of 37 tests passed, 0 failed, 0 skipped, in 43.51 s.
`cmake --build build/dev -t format-check` exits 0.
`cmake --build build/dev -t dbc-gate` exits 0, reporting
`doc-gate: 135 interfaces, 0 gaps` and `pair-gate: 135 interfaces, 0 gaps`.
`cmake -P cmake/spell.cmake` exits 0, and
`python3 tools/pmu_events/update_pmu_events.py --check` exits 0.
`bash tools/dbc/coverage_gate.sh build/coverage/coverage.info` exits 0 at
lines 100.0 percent (1955 of 1955), branches 100.0 percent (705 of 705), and
functions 98.0 percent (289 of 295) on an axis no gate scores, and
`find source include test example -newer build/coverage/coverage.info` names no
file, so that trace covers this tip. `readelf -d` on
`build/dev/example/counters_standalone_example` and on
`build/dev/example/counters_giraffe_example` names `libstdc++`, `libgcc_s`, and
`libc`, with zero `speedgun-ng` entries in either, and
`./build/dev/test/counters_pmu_test` reports
`pmu catalog: 581 table-selected entries beyond kernel aliases` beside
`pmu availability: 358 countable, 0 permission_blocked, 261 not_encodable, 356 fast_rdpmc`.
The `LCOV_EXCL` token counts hold at 301 in the feature scope, 305 across
`source/` plus `include/`, and 4 in `include/speedgun-ng/dbc.hpp`.

The release build, the one Principle IX requires once per feature, is the single
measurement that diverges from the record above. `cmake --preset=ci-ubuntu`
exits 0. `cmake --build build` exits 0 over an incremental tree. Run again with
the project's own object files removed by
`find build/CMakeFiles/speedgun-ng_speedgun-ng.dir -name '*.o' -delete`, the
same command forces a full recompile and produces a log of 3678 lines, of which
759 match the token `warning` and 758 match the compiler prefix `warning:`.
Every one of the 758 carries a bracketed static-analysis check tag; 755 come
from clang-tidy runs over the project's own `source/` and `include/` and 3 from
clang-analyzer runs over `external/simdjson`, and the leading tags are
`readability-identifier-length` at 234,
`cppcoreguidelines-pro-bounds-avoid-unchecked-container-access` at 63,
`misc-include-cleaner` at 49, and
`llvm-prefer-static-over-anonymous-namespace` at 42. Compiler diagnostics
carrying a `-W` tag number 0, and lines matching `error:` number 0, which
follows from the `-Werror` in the committed `flags-gcc-clang` set and from exit
0. A search for the phrase `no line matching` over this file returns the one
line `specs/007-counters-and-timers/tasks.md:2005`, so the class is one site and
T252 covers it whole.

Coverage of the check: 127 requirement keys (50 functional requirements
numbering `FR-001` through `FR-050` with no gap, 10 success criteria numbering
`SC-001` through `SC-010` with no gap, 49 user-story acceptance scenarios across
8 stories, and 18 edge cases), 45 design keys (15 research decisions `R-001`
through `R-015`, 11 data-model entities `E-01` through `E-11`, and 19 contract
clauses `C-MEA-1` through `C-MEA-7`, `C-PRO-1` through `C-PRO-6`, and `C-SYS-1`
through `C-SYS-6`), and 11 constitution principles with X.1 through X.4 and
XI.1 through XI.6 read one by one. Re-measured here: the 9 headers the glob
`include/speedgun-ng/counters*.hpp` matches, the 11 translation units
`find source/counters -name '*.cpp'` returns, the `SG_REQUIRE_ALWAYS` bounds
site at `source/counters/plan.cpp:366`, the `sample_overhead_ns_min` and
`sample_overhead_ns_median` accessors at
`include/speedgun-ng/counters_measurement.hpp:937` and `:946`, the
push-decrement trap mode at `test/source/counters_trap_fixture.cpp:85` and
`:112`, and `counters_noalloc_test` registered at `test/CMakeLists.txt:187`.
Seven findings: 1 `partial` and 6 `contradicts`; 0 CRITICAL, 0 HIGH, 6 MEDIUM,
1 LOW, and none a constitution MUST violation. No finding is `missing`, and no
`unrequested` addition was found. Every functional requirement this pass
inspected has a realization in the code or in the shipped public surface, and
every finding sits in a sentence or a `file:line` anchor an artifact states
about that code.

Every finding below was re-derived in this session from the files it cites. The
Phase 12 through Phase 32 preambles were read for task identifiers, phase
grouping, and the file paths each task names, and no closure claim in them was
accepted as evidence. The two commits at the tip were read in full: `88bbd2c`
removes the `target_kind::machine` enumerator from
`include/speedgun-ng/counters_provider.hpp` and touches no other file, and
`01f905b` restates ten artifact sentences across twelve files and touches no
executable code. Both hold where they landed, with the exceptions recorded
below. Two classes account for all seven findings. The first is the reach of a
settled amendment: five task texts and one plan sentence carry a claim a settled
amendment withdrew, which T254, T255, T256, T257, and T258 record. The second is
citation drift, a `file:line` anchor recorded before an edit that no longer lands
on the code it names, which T253 records; the body of `01f905b` names this class
and assigns it to this pass.

### MEDIUM: the newest preamble's build-log claim no command reproduces

- [X] T252 Record the correction to the release-build log claim the Phase 32
  preamble states at `specs/007-counters-and-timers/tasks.md:2004-2006`, which
  reads that `cmake --preset=ci-ubuntu` and `cmake --build build` exit 0 with no
  line matching the token `warning` in the build log. The claim holds for the
  incremental build and fails for a forced full recompile. Measured on this tip
  from the repository root, `cmake --preset=ci-ubuntu` exits 0, and
  `cmake --build build` over the incremental tree exits 0 over a 26-line log
  holding no line matching the token. A forced full recompile, taken by removing
  the 17 object files under `build/CMakeFiles/speedgun-ng_speedgun-ng.dir` with
  `find build/CMakeFiles/speedgun-ng_speedgun-ng.dir -name '*.o' -delete`, makes
  the same command exit 0 over a 3677-line log in which 759 lines match the
  token `warning`, 758 carry the compiler prefix `warning:`, and 0 match
  `error:`, one line fewer than the count the Phase 33 preamble records for the
  same command over the same deletion. All 758 name a bracketed static-analysis
  check, 755 from clang-tidy over the project's own `source/` and `include/` and
  3 from clang-analyzer over `external/simdjson`, and none carries a compiler
  `-W` tag, which exit 0 and the `-Werror` at `CMakePresets.json:61` establish.
  The leading checks are `readability-identifier-length` at 234,
  `cppcoreguidelines-pro-bounds-avoid-unchecked-container-access` at 63,
  `misc-include-cleaner` at 49, and
  `llvm-prefer-static-over-anonymous-namespace` at 42, and the 759th line
  matching the token is a clang-tidy note at
  `source/counters/linux_pmu/table_parse.cpp:336` carrying no prefix. The scope
  the log covered is the 17 translation units the removed object files belonged
  to, recompiled and relinked into 20 binaries. The corrected claim is that a
  forced release rebuild of this tip exits 0 carrying 758 static-analysis
  diagnostics and 0 compiler diagnostics. The dated text at
  `specs/007-counters-and-timers/tasks.md:1986-2055` keeps its wording, which
  the Pull Request Quality Immutability clause and Principle X.4 require of a
  dated audit record (MEDIUM, Constitution X.4, Constitution XI.6, `contradicts`)

### MEDIUM: thirteen anchors in the two newest phases' task lines name other code

- [X] T253 Re-anchor the thirteen `file:line` anchors inside the Phase 31 and
  Phase 32 task lines that no longer name the code they cite, one clause per
  anchor and with every finding's text, gap type, severity, and disposition left
  in place: `T241` at `specs/007-counters-and-timers/tasks.md:2057` cites
  `include/speedgun-ng/counters_provider.hpp:203` for `point_sink::put`, where
  that line holds `const std::size_t index = m_index;` and the declaration
  stands at `:201`, and cites
  `specs/007-counters-and-timers/contracts/provider-contract.md:74` for C-PRO-2,
  where that line holds a table delimiter row and the clause stands at `:76`;
  `T242` at `:2061` cites `specs/007-counters-and-timers/plan.md:453` for the P2
  Complexity Tracking row, where that line holds the table header and the row
  stands at `:455`; `T243` at `:2065` cites
  `include/speedgun-ng/counters_measurement.hpp:1064-1067` for the `compile`
  brief, where `:1064` holds a closing brace and the brief runs `:1067-1073`;
  `T244` at `:2069` cites `include/speedgun-ng/counters_provider.hpp:326-330`
  for `default_thunk`, where the range opens on that function's opening brace,
  runs past its closing brace, and reaches `check_thunk`, while the definition
  stands at `:324-328`; `T245` at `:2073` cites
  `specs/007-counters-and-timers/contracts/provider-contract.md:54` for the
  giraffe-acceptance link-manifest sentence, where that line holds
  registration item 3 and the sentence stands at `:56`; `T248` at `:2085` cites
  `include/speedgun-ng/counters_measurement.hpp:1076` and `:1093-1095` for the
  two `compile` overloads, where those lines hold a comment terminator, a
  template head, and a `requires(` clause while the declarations stand at `:1081`
  and `:1098-1100`; `T250` at `:2093` cites
  `docs/pages/counters-overhead.md:314` for the privilege table, where that line
  holds prose and the table stands at `:319-322` with the level-2 row at `:321`;
  `T251` at `:2097` cites `include/speedgun-ng/counters_provider.hpp:140-145`
  for the `target_kind` enumeration, where the range holds the enumeration's
  opening brace, the two surviving enumerators, its closing brace, and the
  `target` brief, because `88bbd2c` removed the `machine` enumerator the
  sentence names and the enumeration stands at `:139-143`, and cites `:156` for
  the default, where that line holds a closing brace and the default stands at
  `:154`; `T236` at `:1968` cites `specs/007-counters-and-timers/plan.md:280`
  for the scope-misuse sentence, where that line holds a code fence and the
  sentence stands at `:282`; and `T238` at `:1976` cites
  `specs/007-counters-and-timers/plan.md:434` for the US5 link-manifest row,
  where that line holds the US3 row and the row stands at `:436`; the population
  was bounded by the thirteen files `88bbd2c` and `01f905b` change, which place
  297 distinct anchors in this file, and every anchor outside those two phases'
  task lines that names one of those files either lands on the text it cites or
  describes a revision the tree no longer carries, which the Phase 22 preamble
  records at `specs/007-counters-and-timers/tasks.md:1173-1179`; each
  re-anchored citation must name the line holding its claim (MEDIUM,
  Constitution IV, Constitution X.4, Principle VII, `partial`)

  Re-anchored in this pass: nineteen anchor occurrences across the thirteen
  task lines this finding bounds, the thirteen it names plus six more in the
  same two phases. `T234` at `specs/007-counters-and-timers/tasks.md:1960`
  cited `docs/pages/counters-overhead.md:314` for the fast-regime test, whose
  section now stands at `:242`; `T237` at `:1972` cited
  `include/speedgun-ng/counters_provider.hpp:326-330` for the fallback that
  stands at `:324-328`; `T243` at `:2065` cited
  `specs/007-counters-and-timers/contracts/provider-contract.md:44` for the
  restated disclosure clause, which stands at `:46`, and `:76` for C-PRO-4,
  which stands at `:78`; `T245` at `:2073` carried the
  `specs/007-counters-and-timers/plan.md:434` anchor `T238` names, which now
  stands at `:436`; and `T246` at `:2077` cited
  `docs/pages/counters-overhead.md:296-299` for the `perf_user_access` bullet,
  which stands at `:303-306`.

  The sweep this finding asks for read every `file:line` anchor in this file
  that names one of the thirteen files `88bbd2c` and `01f905b` change, against
  the post-edit tree: 606 anchor tokens over 350 distinct (file, line) pairs,
  80 of the tokens inside the Phase 31 and Phase 32 task lines, which carry 121
  tokens in all, each read at the line it names. The claim above that every
  anchor outside those two phases either lands on the text it cites or
  describes a revision the tree no longer carries does not hold everywhere.
  `T120` at `specs/007-counters-and-timers/tasks.md:414` names `spec.md:276`
  where FR-049 now stands at `:286`, `T126` at `:420` names `spec.md:247` where
  FR-029 now stands at `:257`, and `T147` at `:495` names `spec.md:249` where
  FR-024 now stands at `:252`; `T223` at `:1805` names
  `specs/007-counters-and-timers/contracts/measurement-contract.md:107` for
  the single-target sentence, which stands at `:110`, and that anchor named a
  blank line at `88bbd2c` as well; the Phase 20 and Phase 21 preambles at `:1037`
  and `:1115`, `T171` at `:589`, and `T179` at `:674` name
  `include/speedgun-ng/counters_measurement.hpp:1089-1095` and `:1085` for the
  target-taking `compile`, whose declaration stands at `:1098-1100`; and the
  anchors `T072` at `:357`, `T080` at `:368`, `T100` at `:391`, `T170` at
  `:588`, `T189` at `:976`, and the Phase 20 preamble at `:1034` and `:1042`
  name lines that grew a different sentence or a blank line under them after
  the edits of the waves that followed. Each sits in a dated preamble or a
  closed task text, which the Pull Request Quality Immutability clause keeps as
  written, so this pass moves none of them and a later record carries the
  correction, the reading T252 above follows.

### MEDIUM: a US6 task text still asks for the target validation FR-024 withdrew

- [X] T254 Restate the group-layout clause of T049 at
  `specs/007-counters-and-timers/tasks.md:168`, which asks to validate shared
  target and clock identity across group members at construction and makes a
  mismatch a recoverable construction error, with the single-plan-target design
  the settled FR-024 states: `specs/007-counters-and-timers/spec.md:252`
  requires that a target or clock-identity mismatch across group members is
  unrepresentable and names the two construction errors a single target can
  still fail on, the clarification at `spec.md:34` records the same,
  `source/counters/plan.cpp:469` stores the one `bound_target` for the whole
  plan, `source/counters/detail/core.hpp:104` declares that member, and no
  cross-member identity check exists anywhere in `source/counters/`; T147
  amended the requirement, T086 amended the task that recorded the validation,
  and T223 restated the five artifact sites at
  `specs/007-counters-and-timers/data-model.md:83` and `:90`,
  `specs/007-counters-and-timers/contracts/measurement-contract.md:48` and
  `:110`, and `specs/007-counters-and-timers/research.md:81`, while no prior
  task names T049 and its own line is the remaining site; its second clause on
  thread and cpu binding at plan open keeps its place, as does its `FR-031`
  reference (MEDIUM, FR-024, US6 scenario 7, T147, T223, `contradicts`)

### MEDIUM: the scope task text names two misuse sequences the type cannot express

- [X] T255 Restate the misuse list of T019 at
  `specs/007-counters-and-timers/tasks.md:68`, which names five tier-3 sequences
  including `use-after-finish` and `registering a composite into a started
  scope`, with the three the settled edge case names:
  `specs/007-counters-and-timers/spec.md:199` and the clarification at `spec.md:36`
  name `metric` on a window that is not closed, `finish` without `start`
  including the second `finish`, and a second `start`, and they record that a
  finished scope is a settled window and that the registration has no spelling;
  `source/counters/fold.cpp:289-290` refuses `metric` only on a window that is
  not closed, `source/counters/plan.cpp:330` refuses a second `start`,
  `source/counters/plan.cpp:344-345` refuses `finish` without `start` and a
  second `finish`, and `test/source/counters_fake_test.cpp:486-494` asserts ten
  further `metric` calls on a finished scope succeed with zero provider reads;
  T155 amended the edge case, T226 restated
  `specs/007-counters-and-timers/contracts/measurement-contract.md:122`, T235
  restated `include/speedgun-ng/counters_measurement.hpp:984-987`, and T236
  restated `specs/007-counters-and-timers/plan.md:282`, and no prior task names
  T019, whose own line is the remaining site (MEDIUM, FR-046, T155, T226, T235,
  T236, `contradicts`)

### MEDIUM: two sites still claim the manifest names this library

- [X] T256 Restate the two remaining sites of the link-manifest claim the settled
  FR-049 withdrew, the clause of T041 at
  `specs/007-counters-and-timers/tasks.md:146` which asks to verify that the
  manifest names the library alone, and the Key-properties sentence at
  `specs/007-counters-and-timers/plan.md:323` which reads that the public link
  surface stays `speedgun-ng::speedgun-ng` alone with the standalone example's
  manifest as the evidence, where `specs/007-counters-and-timers/spec.md:286`
  states that the target is a static archive whose manifest carries no
  `speedgun-ng` entry and names the platform C and C++ runtime, where the
  clarification at `spec.md:31` records the same and names `libstdc++`, `libm`,
  `libgcc_s`, and `libc`, where `CMakeLists.txt:18-21` declares `add_library`
  with no type keyword, and where `readelf -d
  build/dev/example/counters_standalone_example` and `readelf -d
  build/dev/example/counters_giraffe_example` each name `libstdc++`, `libgcc_s`,
  and `libc` with zero `speedgun-ng` entries; T120 amended FR-049 and SC-001,
  T121 restated the sources clause of T041, T238 restated
  `specs/007-counters-and-timers/plan.md:33` and `:436` beside
  `specs/007-counters-and-timers/contracts/measurement-contract.md:126`, and
  T245 restated `specs/007-counters-and-timers/contracts/provider-contract.md:56`,
  and neither T238 nor T245 names `plan.md:323` or the T041 clause, so each of
  the two sentences must state that the manifest demonstrates no third-party
  dynamic dependency and names the platform C and C++ runtime (MEDIUM, FR-049,
  SC-001, T120, T238, T245, `contradicts`)

### MEDIUM: a test-writing task still demands a diagnostic the code does not name

- [X] T257 Restate the open-refusal clause of T043 at
  `specs/007-counters-and-timers/tasks.md:159`, which requires that compiling
  over a target the kernel refuses to open fails recoverably with the member at
  fault named, with the amended requirement:
  `specs/007-counters-and-timers/spec.md:151` states that the open message names
  no member, the amended FR-024 at `spec.md:252` states that the open error is
  one diagnostic per read group reporting that the provider could not open a
  window for the leaves it owns and naming no leaf, no provider, and no kernel
  reason, the clarification at `spec.md:34` records the same, and
  `source/counters/plan.cpp:502-504` emits that single message, so a test
  written from the task text as it stands asserts a message the code does not
  produce; T167 settled the choice between carrying the refusing address out of
  `open` and amending the two artifact sentences, and the artifacts took the
  amendment, the second clause of the same T043 sentence already carries the
  settled single-plan-target wording and keeps it, and no prior task names this
  clause (MEDIUM, FR-024, US6 scenario 7, T167, `contradicts`)

### LOW: the TSC task text still places the calibration at system-open

- [X] T258 Restate the calibration boundary of T036 at
  `specs/007-counters-and-timers/tasks.md:127`, which reads that the frequency is
  calibrated at system-open, with provider construction: the amended FR-034 at
  `specs/007-counters-and-timers/spec.md:265` states that the frequency is
  calibrated at provider construction and gives the reason, that registration is
  refused after open and the catalog freezes there, so a calibration deferred to
  open would publish an uncalibrated leaf, the clarification at `spec.md:35`
  records the same, `specs/007-counters-and-timers/research.md:57` carries the
  settled wording after T228, and the calibration runs in the `clock_provider`
  constructor at `source/counters/clock_provider.cpp:206-236`; T127 and T153
  settled the requirement, T228 restated the research record, T128 named this
  task for the CPUID leaf comment alone, and no prior task names this clause,
  while the `__rdtsc` spelling, the `tsc_khz` cross-check, the achieved mode, the
  calibration provenance, the scaled-TSC flag, the omission on a host without a
  usable TSC, and the P2 site justification all keep their place (LOW, FR-034,
  T127, T153, T228, `contradicts`)

## Phase 34: Convergence

Appended by `/speckit.converge` after an audit of the branch tip at `30f6361`
and of the residue the fifteen waves before it left. Nothing above this line
changed.

Audit evidence, all produced by this pass from the repository root.
`python3 tools/prose/prose_gate.py --check all` exits 0, and
`cmake -P cmake/prose-lint.cmake` exits 0 on the same verdict,
`prose-lint: 126 sources, 9919 units examined, 0 findings, 1 skipped`. That
figure takes its candidate set and per-line authorship filter from the range,
the merge base with `origin/master` to `30f6361`, and the text it examines from
the working tree, so an uncommitted line inside a ranged file is examined, and
no commit reproduces the total, which is the reading the Phase 21 preamble
states at `specs/007-counters-and-timers/tasks.md:1112-1119`; this preamble
records no `--mode tree` total. `ctest --test-dir build -N` exits 0 and reports
`Total Tests: 37`, of which `ctest --test-dir build -N -R counters` reports 16.
`cmake --preset=dev` and `cmake --build --preset=dev` exit 0, the build over an
incremental tree in which every target reported `Built target`;
`ctest --preset=dev` exits 0 with 100.0 percent of 37 tests passed, 0 failed,
0 skipped, in 43.76 s. `cmake --preset=ci-ubuntu` exits 0, and
`cmake --build build`, the release build Principle IX requires once per
feature, exits 0 over an incremental 27-line log holding no line matching the
token `warning`. Removing the library object files with
`find build/CMakeFiles/speedgun-ng_speedgun-ng.dir -name '*.o' -delete` and
running the same command forces a full recompile of the 17 translation units
those objects belonged to: exit 0 over a 3677-line log with 759 lines matching
the token, 758 carrying the compiler prefix, 0 carrying a `-W` tag, and 0
matching `error:`. The leading checks are `readability-identifier-length` at
234, `nodiscard` at 117,
`cppcoreguidelines-pro-bounds-avoid-unchecked-container-access` at 63,
`misc-include-cleaner` at 49, and
`llvm-prefer-static-over-anonymous-namespace` at 42.
`cmake --build build/dev -t format-check` exits 0.
`cmake --build build/dev -t dbc-gate` exits 0, reporting
`doc-gate: 135 interfaces, 0 gaps` and `pair-gate: 135 interfaces, 0 gaps`.
`cmake -P cmake/spell.cmake` exits 0, and
`python3 tools/pmu_events/update_pmu_events.py --check` exits 0.
`bash tools/dbc/coverage_gate.sh build/coverage/coverage.info` exits 0 at
lines 100.0 percent (1955 of 1955), branches 100.0 percent (705 of 705), and
functions 98.0 percent (289 of 295) on an axis no gate scores, and
`find source include test example tools/pmu_events -newer
build/coverage/coverage.info` names no file. `readelf -d` on
`build/dev/example/counters_standalone_example` and on
`build/dev/example/counters_giraffe_example` names `libstdc++.so.6`,
`libgcc_s.so.1`, and `libc.so.6`, with zero `speedgun-ng` entries in either.
`./build/dev/test/counters_pmu_test` reports
`pmu catalog: 581 table-selected entries beyond kernel aliases` beside
`pmu availability: 358 countable, 0 permission_blocked, 261 not_encodable, 356 fast_rdpmc`.

Coverage of the check: 127 requirement keys (50 functional requirements
numbering `FR-001` through `FR-050` with no gap, 10 success criteria numbering
`SC-001` through `SC-010` with no gap, 49 user-story acceptance scenarios
across 8 stories, and 18 edge cases), 45 design keys (15 research decisions
`R-001` through `R-015`, 11 data-model entities `E-01` through `E-11`, and 19
contract clauses `C-MEA-1` through `C-MEA-7`, `C-PRO-1` through `C-PRO-6`, and
`C-SYS-1` through `C-SYS-6`), and 11 constitution principles with X.1 through
X.4 and XI.1 through XI.6 read one by one. Re-measured here: the 9 headers
`include/speedgun-ng/counters*.hpp` matches, the 11 translation units
`find source/counters -name '*.cpp'` returns beside the 2 provider-private
headers under `source/counters/detail/`, the 12 `add_executable(counters_)`
calls `test/CMakeLists.txt` carries, and the `LCOV_EXCL` counts at 301 tokens in
the feature scope, 305 across `source/` plus `include/`, and 4 in
`include/speedgun-ng/dbc.hpp`. The constitution checks are mechanical here:
`dbc-gate` pairs 135 interfaces with 0 gaps on the documentation matrix and on
the pairing matrix, the coverage gate holds 100.0 percent on lines and on
branches, every gate in Principle VIII that this host can run exits 0, the
per-feature release build of Principle IX exits 0, a search for `TODO` and
`FIXME` over the feature scope returns no line, the single `NOLINT` directive
at `include/speedgun-ng/counters_measurement.hpp:634` carries its reason in the
same comment as Principle X.2 requires, and a search for `as any`,
`@ts-ignore`, and `-Wno-error` over the feature scope and the root
`CMakeLists.txt` returns no line. Two findings: 1 `contradicts` and 1
`partial`; 0 CRITICAL, 0 HIGH, 2 MEDIUM, 0 LOW, and neither a constitution MUST
violation. No finding is `missing`, no `unrequested` addition was found, every
functional requirement this pass inspected has a realization in the code or in
the shipped public surface, and both findings sit in a sentence the code
contradicts or in a `file:line` anchor that no longer lands on the text it
names.

Every finding below was re-derived in this session from the files it cites. The
Phase 12 through Phase 33 preambles were read for task identifiers, phase
grouping, and the file paths each task names, and no closure claim in them was
accepted as evidence. The three commits at the tip were read in full: `88bbd2c`
removes the `target_kind::machine` enumerator from
`include/speedgun-ng/counters_provider.hpp` and touches no other file, and a
search for `target_kind::machine` over `source/`, `include/`, `test/`, and
`example/` returns no line; `01f905b` restates ten artifact sentences across
twelve files and touches no executable code; `30f6361` restates seven more
across `specs/007-counters-and-timers/plan.md` and `tasks.md` and touches no
executable code. All 38 explicit `file:line` anchors the eight live artifacts
place into a source, a header, a script, or a second artifact were resolved and
read against the claim each sentence makes, and every one lands on the text it
names. The closed journal places none, and it is where the first finding sits.

The two classes the earlier waves drove account for both findings, and the first
is the settled amendment's reach inside the one file the previous wave swept for
a single claim. The link-manifest claim has now taken three waves, and the sweep
the body of `30f6361` records, "a tree-wide sweep finds one site left", was
bounded to that one claim. Bounding the same sweep to the other eight settled
amendments, over `*.md`, `*.hpp`, `*.cpp`, `*.sh`, and `*.py` across the whole
tree with `build/`, `external/`, and `.omo/` excluded, finds four further sites
in the same file, all inside the one section its own header names as the input
to `/speckit.specify` and the spec's Assumptions name as the authoritative
design record. The second is citation drift. `T253` moved 19 anchors, and its own
second sub-paragraph records the remainder as debt "a later record carries";
no such record exists in the tree, and the population no earlier wave covered is
the Phase 1 through Phase 31 task texts. This pass resolved all 740 explicit
`path:line` and `path:NN-MM` anchors those nine artifacts place and read each
landing. The bare `:NN` continuation form is attributed by hand here, because a
line naming two files attributes the continuation to the wrong one and the
automated pass over that form returns false positives. The 740 anchors are the
population as it stood before this phase was appended; a sweep run over the file
as it now stands resolves 762, of which four are the drifted anchors T260
quotes inside this phase, so the same sweep returns those four by design.

### MEDIUM: four further sites of a settled amendment's reach, inside the section the closed journal hands to `/speckit.specify`

- [X] T259 Record the supersession of the five sentences inside the closed journal's Resolved scope statement that carry claims the settled amendments withdrew, without rewriting that dated text, where the statement spans `specs/007-counters-and-timers/sg_counters.md:855-1076` and its header at `:7-12` names it as the input to `/speckit.specify`, and where the five sites are `:959`, which reads that the `tsc` frequency is `calibrated at system-open`, while the amended `FR-034` at `specs/007-counters-and-timers/spec.md:265` states provider construction and the calibration runs in the `clock_provider` constructor at `source/counters/clock_provider.cpp:206-236`; `:981-982`, which names the `perf_user_access` sysctl as a per-kernel gate, where a search for `perf_user_access` over `source/`, `include/`, and `test/` returns no line, `docs/pages/counters-overhead.md:303` records the sysctl absent on this kernel, and the capability bit and the counter width come from the event page at `source/counters/linux_pmu/fast_read.cpp:271-272`; `:983`, which reads `pinning/index constraints enforced at plan compile`, while the amended `FR-023` at `specs/007-counters-and-timers/spec.md:251` places the probe in provider enumeration and the index gate runs in `fast_context_read` at `source/counters/linux_pmu/fast_read.cpp:278-281`; `:1010-1011`, which lists `registering a composite into a started scope` among the tier-3 contract violations, while the amended edge case at `specs/007-counters-and-timers/spec.md:199` and the clarification at `:36` name the three enforceable sequences, `scope` exposes no registration entry point at `include/speedgun-ng/counters_measurement.hpp:992-1064`, and `source/counters/fold.cpp:289-290` permits `metric` on a closed window; and `:1049`, which reads that the `link manifest shows only speedgun-ng`, while the amended `FR-049` at `specs/007-counters-and-timers/spec.md:286` states that the target is a static archive whose manifest carries no `speedgun-ng` entry, and `readelf -d build/dev/example/counters_standalone_example` names `libstdc++.so.6`, `libgcc_s.so.1`, and `libc.so.6` alone. The journal's own precedence rule at `:9-12` settles every site before line 855, because those sit in the older axis headers the rule subordinates to the scope statement; no rule reaches a sentence inside the scope statement itself, which is the reason these five survive three waves of restating the same claims elsewhere. The record belongs in a live artifact, one sentence in the Assumptions paragraph at `specs/007-counters-and-timers/spec.md:321`, which already names the journal as the authoritative design record for intent, stating that where the journal's Resolved scope statement names a boundary, a gate, a mechanism, or a manifest that a later requirement withdrew, the requirement in `spec.md` governs, and the five sites above are the sentences that rule settles. The journal text, every requirement number and position, every gate, every exclusion marker, and every checkbox above this line keep their bytes (MEDIUM, FR-023, FR-034, FR-040, FR-046, FR-049, Constitution IV, Pull Request Quality: Immutability, T120, T137..T139, T153, T226, T232, T233, T246, T255, T256, T258, `contradicts`)

### MEDIUM: the deferred citation correction no record in the tree carries

- [X] T260 Record the corrected anchors for the Phase 1 through Phase 33 task
  lines and phase preambles whose `file:line` no longer lands on the text it
  names, in a new artifact `specs/007-counters-and-timers/citations.md` listed
  beside `tasks.md` in the Project Structure block at
  `specs/007-counters-and-timers/plan.md:61-74`, with every dated preamble and
  every closed task line left byte-for-byte as written, where `T253` at
  `specs/007-counters-and-timers/tasks.md:2290-2313` swept the population its
  two commits change, moved 19 anchors, and records the rest as debt a later
  record carries, and where no later record exists. Verified in this pass over
  the 740 explicit `path:line` anchors those nine artifacts place, the landings
  that no longer name the text their sentence names are `T072` at
  `specs/007-counters-and-timers/tasks.md:357`, which names
  `include/speedgun-ng/counters_measurement.hpp:610` for a `NOLINTNEXTLINE`
  where that line is blank and the directive stands at `:634`;
  `T147` at `:495`, which names `specs/007-counters-and-timers/spec.md:249` for
  `FR-024` where that line is blank and the requirement stands at `:252`;
  `T166` at `:581`, which names `source/counters/plan.cpp:409` for the zero-leaf
  refusal where that line is blank and the refusal stands at `:417` beside
  `:89` and `:536`; the Phase 22 preamble at `:1178`, which names
  `source/counters/linux_pmu/fast_read.cpp:309-444` in a file of 331 lines;
  and the three bare `spec.md` continuations `T253` itself names, `T120` at
  `:414` naming `:276` for `FR-049`, `T126` at `:420` naming `:247` for
  `FR-029`, and `T147` at `:495` naming `:249` for `FR-024`, where all three
  name blank lines and the requirements stand at `:286`, `:257`, and `:252`.
  Each row of the new artifact names the task, the line in `tasks.md`, the
  anchor as written, the line now holding the claim, and the requirement that
  governs, so a reader consulting a closed task body reaches the code; the
  Phase 20 preamble at `:1037` and the Phase 21 preamble at `:1115` also name
  `include/speedgun-ng/counters_measurement.hpp:1089-1095` for the
  target-taking `compile`, whose declaration stands at `:1098-1100` and whose
  doxygen brief starts at `:1088`, and those two rows belong in the same
  artifact. No anchor above this line moves, no checkbox moves, and no source,
  header, test, example, tool, or build file changes (MEDIUM, Constitution IV,
  Constitution X.4, T253, T252, `partial`)

## Phase 35: Convergence

Appended by `/speckit.converge` after an audit of the branch tip at `1827d76`
and of the residue the twenty-three waves before it left. Nothing above this
line changed.

Audit evidence, all produced by this pass from the repository root on Linux.
`python3 tools/prose/prose_gate.py --check all` exits 0, and
`cmake -P cmake/prose-lint.cmake` exits 0 on the same verdict,
`prose-lint: 128 sources, 10250 units examined, 0 findings, 1 skipped`. Both
read the range for its candidates and its line filter, the merge base with
`origin/master`, `65beada`, to `1827d76`, and the working tree for the text it
examines, so no commit reproduces it. The one skipped source is `hwloc.md`; the
`--mode tree` form collects its files with `git ls-files` and reads the same
working tree. Read narrowed to this feature, `python3 tools/prose/prose_gate.py
--check prose --mode tree --paths specs/007-counters-and-timers
docs/pages/counters-overhead.md` exits 0 at `13 sources, 4865 units examined, 0
findings, 0 skipped`, measured against the tree `1827d76`.
`ctest --test-dir build -N` exits 0 and reports `Total Tests: 37`, of which
`ctest --test-dir build -N -R counters` reports 16. `cmake --preset=dev` and
`cmake --build --preset=dev -j` exit 0, and `ctest --preset=dev` exits 0 with
100.0 percent of 37 tests passed, 0 failed, 0 skipped, in 43.66 s of total
test time. `cmake --preset=ci-ubuntu` exits 0 with `CMAKE_BUILD_TYPE:STRING=Release`
and `CMAKE_CXX_FLAGS_RELEASE:STRING=-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3 -O3
-DNDEBUG`, and `cmake --build build` exits 0 over an incremental 26-line log in
which every target reported `Built target`, holding 0 lines matching the token
`warning` and 0 matching `error:`. No forced recompile ran, so this pass states
no static-analysis diagnostic count; the two figures above cover an incremental
build only. `cmake --build build/dev -t format-check` exits 0.
`cmake --build build/dev -t dbc-gate` exits 0, reporting
`doc-gate: 135 interfaces, 0 gaps` and `pair-gate: 135 interfaces, 0 gaps`.
`cmake -P cmake/spell.cmake` exits 0, and
`python3 tools/pmu_events/update_pmu_events.py --check` exits 0.
`bash tools/dbc/coverage_gate.sh build/coverage/coverage.info` exits 0 over
21 source files at lines 100.0 percent (1955 of 1955), branches 100.0 percent
(705 of 705), and functions 98.0 percent (289 of 295) on an axis no gate
scores, and `find source include test example tools/pmu_events -newer
build/coverage/coverage.info` names no file, so the tracefile is newer than
every file the measurement covers. `readelf -d` on
`build/dev/example/counters_standalone_example` and on
`build/dev/example/counters_giraffe_example` names `libstdc++.so.6`,
`libgcc_s.so.1`, and `libc.so.6` in each, with zero `speedgun-ng` entries.
`./build/dev/test/counters_pmu_test` exits 0 reporting
`pmu catalog: 581 table-selected entries beyond kernel aliases` beside
`pmu availability: 358 countable, 0 permission_blocked, 261 not_encodable, 356
fast_rdpmc`. `git status --porcelain` is empty at `1827d76` and
`git log --oneline -8` shows the tip reading `1827d76`, `30f6361`, `01f905b`,
`88bbd2c`, `02c7db8`.

Counting rule for every anchor total named in this preamble, so a reader can
reproduce it: the unit is one occurrence of the regular expression
`(?:[\w./-]+/)*[\w.-]+\.[A-Za-z0-9]+:\d+(-\d+)?` matched over a named line range
of a named file, and a distinct token is the deduplicated form of that unit.
Applied to the nine live artifacts, which
are `spec.md`, `plan.md`, `research.md`, `data-model.md`, `quickstart.md`,
`sg_counters.md`, and the three files under `contracts/`, the sum is 38
occurrences, distributed `spec.md` 28, `quickstart.md` 4, `plan.md` 3,
`contracts/system-contract.md` 2, `research.md` 1, `data-model.md` 0,
`sg_counters.md` 0, `contracts/provider-contract.md` 0, and
`contracts/measurement-contract.md` 0; the journal contributes none, which is
what makes those eight the eight the Phase 34 preamble names. Applied to
`tasks.md` lines 1 through 2422, the population the Phase 1 through Phase 33
record occupies, the count is 702 occurrences and 467 distinct tokens, and the
bare `:NN` continuation form counts 420. Applied to `tasks.md` lines 1 through
2578, the file as it stands, the count is 724 occurrences and 476 distinct
tokens, and the bare `:NN` form counts 449. The Phase 34 preamble's two
totals reproduce exactly under this rule: 740 is the nine artifacts' 38 plus
`tasks.md` 1-2422's 702, and 762 is the same 38 plus the whole file's 724. The
difference of 22 occurrences and 29 continuations between the two readings is
the Phase 34 section itself, appended after the first figure was taken.

Coverage of the check: 127 requirement keys, counted as 50 functional
requirements numbering `FR-001` through `FR-050` with no gap, 10 success
criteria numbering `SC-001` through `SC-010` with no gap, 49 user-story
acceptance scenarios counted as US1 7, US2 7, US3 6, US4 6, US5 4, US6 7, US7 6,
US8 6, and 18 edge cases at `specs/007-counters-and-timers/spec.md:195-212`;
45 design keys, counted as 15 research decisions `R-001` through `R-015`, 11
data-model entities `E-01` through `E-11`, and 19 contract clauses
`C-MEA-1` through `C-MEA-7`, `C-PRO-1` through `C-PRO-6`, and `C-SYS-1` through
`C-SYS-6`; and 11 constitution principles with X.1 through X.4 and XI.1 through
XI.6 read one by one. Every `FR-001` through `FR-050` was checked against the
shipped code, and every one has a realization; no requirement this pass
inspected is `missing`, and no `unrequested` addition was found. Every `SC-0NN`
and every acceptance scenario was checked for a test, example, published page,
or tool exit path that realizes it, and the two the artifacts already dispose
of stay disposed: the `SC-002` row at
`specs/007-counters-and-timers/quickstart.md:139` states that the
`permission_blocked` branch US6 scenario 4 expects at paranoia 2 ran nowhere
on the reference host and awards no verdict for it, and the `SC-004` row at
`:140` states `MEASURED, order check not met` and awards no verdict. Re-measured
here: the 9 headers `include/speedgun-ng/counters*.hpp`, the 11 translation
units `find source/counters -name '*.cpp'` returns beside the 2 provider-private
headers under `source/counters/detail/`, the 12 `add_executable(counters_)`
calls `test/CMakeLists.txt` carries, the `LCOV_EXCL` count at 301 tokens in the
feature scope and 305 across `source/` plus `include/`, no line matching `TODO`
or `FIXME` over the feature scope, and the single `NOLINT` directive in the
feature scope at `include/speedgun-ng/counters_measurement.hpp:634` carrying its
reason in the same comment block at `:630-633` as Constitution X.2 requires.
The four commits at the tip were read in full with `git show`: `88bbd2c` removes
the `target_kind::machine` enumerator from
`include/speedgun-ng/counters_provider.hpp` and touches no other file, and a
search for `target_kind::machine` over `source/`, `include/`, `test/`, and
`example/` returns no line; `01f905b` restates ten artifact sentences and
touches no executable code; `30f6361` restates seven more across
`specs/007-counters-and-timers/plan.md` and `tasks.md` and touches no
executable code; `1827d76` adds `specs/007-counters-and-timers/citations.md`,
one line to the plan's Project Structure block, and the Phase 34 section, and
touches no source, header, test, example, tool, or build file. All 38 anchors
the eight live artifacts place were resolved and read, and every one lies
inside the file it names and none lands on blank lines. No closure claim in the
Phase 12 through Phase 34 preambles was accepted as evidence.

Six findings: 2 `contradicts` and 3 `partial` and 1 further `partial`; 1
CRITICAL, 1 HIGH, 3 MEDIUM, 1 LOW; none is `missing` and none is `unrequested`.
Five of the six sit in a sentence, a checkbox, or an anchor inside a dated
record, and one is a Principle XI.1 violation in a live artifact the shipped
prose gate structurally cannot report.

The residue the implement and commit passes reported was assessed on its own
evidence, and three of the nine items do not survive it. The Phase 34
preamble's two anchor totals reproduce exactly under the rule stated above, so
no finding rests on their irreproducibility; what survives is that
`specs/007-counters-and-timers/citations.md:28-31` quotes the whole-file
figures for a population it defines as the Phase 1 through Phase 33 record, and
states no rule, which is T265. The Phase 34 preamble's claim that all 38
anchors land holds for the population it names, which excludes `tasks.md`; the
`source/counters/linux_pmu/provider.cpp:193` anchor the report raised belongs
to `T076` at `specs/007-counters-and-timers/tasks.md:364`, inside the separate
702-occurrence population, and `specs/007-counters-and-timers/citations.md:59`
already records its correction. The Phase 22 preamble at
`specs/007-counters-and-timers/tasks.md:1180` names
`.specify/memory/constitution.md:568-570` and asserts that range still holds the
machine-local `CMakeUserPresets.json` sentence, which it does, at `:569-570`;
the false correction sits in `citations.md:103`, which is T263. The three
drifted anchors the report attributes to uncovered sites, `T185`,
`T178`, and the Phase 20 preamble, are recorded at `citations.md:96`, `:94`,
and `:100`. The `T072` directive population is gone, and
`specs/007-counters-and-timers/citations.md:57-58` records that. The
`T189` pointers are wrong by three lines and two lines, and
`specs/007-counters-and-timers/citations.md:97` records that. The Phase 22
preamble at `:1177-1179` correctly reports the out-of-range
`source/counters/linux_pmu/fast_read.cpp:309-444` as a `T139` citation in a file
of 331 lines, and `citations.md:80` records it. The owed sentence at
`specs/007-counters-and-timers/spec.md:321` is genuinely absent and
`citations.md:20-24` records the debt, which is T264.

### CRITICAL: a Principle XI.1 violation the shipped prose gate structurally cannot report

- [X] T261 Bring `specs/007-counters-and-timers/tasks.md:407` into Principle
  XI.1 compliance and record the journal's 58 em-dash lines and the gate's two
  blind spots in `specs/007-counters-and-timers/citations.md`, where the line
  at `:407` reads `... one sampling point — \`scope\`'s own buffer ...` and
  carries the code point U+2014 between two clauses of the `T116` task, a
  violation of the binding Principle XI.1 at
  `.specify/memory/constitution.md:397-403`, which the line's own
  2026-09-25 authorship places after the 2026-09-10 amendment the clause's
  scope paragraph at `:405-409` says binds output generated after it. No
  machine check reports it, for two independent reasons, both re-verified
  here: `tools/prose/prose_gate.py:876-883` returns before any rule matcher
  runs when the unit carries an inline code span, a URL, a path-like token, a
  shell command, or a blockquote, and `INLINE_CODE_RE.search` and `path_like`
  both match line 407; and the line is outside the CI range, because the first
  commit touching `specs/007-counters-and-timers/`, `c6d9c19`, is not an
  ancestor of the merge base `65beada`, so
  `python3 tools/prose/prose_gate.py --check prose --mode tree --paths
  specs/007-counters-and-timers docs/pages/counters-overhead.md` exits 0 at 0
  findings over the line. The same rule carries
  `specs/007-counters-and-timers/sg_counters.md:925` and `:936`, which the
  `INDENTED_CODE_RE` precedence at `tools/prose/prose_gate.py:874-875` drops
  because a four-or-more-space markdown continuation line matches it, and 54
  further journal lines the code-span, URL, path-like, and blockquote
  precedence drops, for 58 in all. Principle XI.1's own scope paragraph at
  `.specify/memory/constitution.md:408-409` names the remedy, a tree-wide sweep
  as a formatting-only change under Principle V, scheduled on its own, so the
  change that fixes the `tasks.md` line may not share a commit with any content
  change. The 58 journal lines stay as written, because the journal is a dated
  closed record, and `citations.md` is the place the constitution permits for
  recording them. No dated preamble, no closed task line, no checkbox above
  this line, and no requirement number or position moves (CRITICAL,
  Constitution XI.1, Constitution XI.6, Constitution V: Style and Formatting,
  Constitution X.1, `contradicts`)

### HIGH: `T091` is closed with neither branch landed, and nine live sites still assert the shape the code refuses

- [X] T262 Reconcile the design record with the shipped shape of the plan's fold
  program, where `T091` at `specs/007-counters-and-timers/tasks.md:379` is
  checked `- [X]` and offers two branches, "Store a per-composite fold program
  of slot references in `plan_impl`" or "amend `T017`'s 'fold program per
  composite (column references with algebraic exponents and ops)'", and neither
  landed. The code states the opposite of the first branch at
  `source/counters/detail/core.hpp:92-99`, whose comment reads "no
  per-composite program is stored here (T091)" and justifies it by citing
  `FR-022` itself, while the second branch left `T017` at
  `specs/007-counters-and-timers/tasks.md:66` carrying its original clause
  verbatim, the only two mentions of `T017` in the file being that line and
  `T091`'s reference to it. The cost `T091` named also remains: the fold
  re-resolves leaf to slot through the string-keyed `by_address` map at
  `source/counters/fold.cpp:43` and `:162`, which the decision at
  `core.hpp:95-99` accepts. Eight live sites and one requirement still assert
  the program exists: `FR-022` at
  `specs/007-counters-and-timers/spec.md:250` names "fold sequence" among the
  four things the flat read plan produces, the Key Entities entry at `:298`
  names it again, `specs/007-counters-and-timers/data-model.md:65` reads
  "compiled to a fold program by the plan" and `:85` lists "fold program |
  per composite" as a plan field, `specs/007-counters-and-timers/research.md:41`
  records it in the `R-005` decision, `specs/007-counters-and-timers/plan.md:111`
  names "fold programs" in the `plan.cpp` duty and `:234` draws the class
  diagram edge `plan ..> expression : fold programs`, and
  `specs/007-counters-and-timers/contracts/measurement-contract.md:47` and
  `:110` both state it. The read path itself complies with `FR-022`'s normative
  clause: `sample_row` at `source/counters/plan.cpp:33-43` reaches
  `group.thunk` over the column cursor and touches no expression node and no
  name lookup, and the spine is a flat index-addressed node array at
  `include/speedgun-ng/counters_measurement.hpp:102-110`, so the substance of a
  fold sequence exists and the plan does not own it. `FR-021`'s requirements
  hold, since a fold still accepts any expression over the plan's slots at any
  time. Take `T091`'s second branch and amend the requirement and the eight
  sites to the shape the code has, recording the decision, the competing
  reading, and the reason `core.hpp:92-99` gives at the site, which is what
  Constitution X.1 requires; a change to the code instead is a design change to
  a requirement Principle IX makes binding and takes a DCR under Principle
  III, so it may not be taken inside this task. `T091` and `T017` keep their
  bytes as dated records. Record the journal site
  `specs/007-counters-and-timers/sg_counters.md:252`, which reads "leaf slots +
  fold sequence", in the same `citations.md` pass, and record the `T032` and
  `T098` drift T266 names there too (HIGH, FR-022, FR-021, Constitution IX,
  Constitution III: Design Change Request, Constitution X.1, `contradicts`)

### MEDIUM: the anchor-correction record states a correction the tree contradicts

- [X] T263 Correct the one row of `specs/007-counters-and-timers/citations.md`
  whose "Line now holding the claim" holds other text, which is the row at
  `:103`, reading "| Phase 22 preamble | 1180 |
  `.specify/memory/constitution.md:568-570` | `:571-573`, the machine-local
  `CMakeUserPresets.json` sentence | Constitution IX | this pass |".
  `.specify/memory/constitution.md:571` reads "- **Licensing:** BSD 3-Clause.
  All contributed code is compatible, with no", `:572` reads "  additional
  license burden.", and `:573` is blank, so the range the row names as the new
  landing holds the Licensing bullet and one empty line. The sentence is at
  `:569-570`, "  `CMakeUserPresets.json` is machine-local and must NEVER be
  checked into", "  source control.", which is inside the range `:568-570` the
  Phase 22 preamble at `specs/007-counters-and-timers/tasks.md:1179-1181`
  already names and asserts still holds it, so the preamble needs no correction
  and the row invented one. This is the only one of the 49 anchor rows the
  record carries that fails the third drift criterion it states at
  `specs/007-counters-and-timers/citations.md:40`, "the cited line holds text
  other than the claim the sentence makes"; the first two criteria pass over
  every row, checked here line by line. Replace the row's landing with the
  sentence's own lines and state that the Phase 22 preamble's range already
  holds the claim, so a reader of the record is not sent three lines past the
  text. Every other row, every dated preamble, every closed task line, and
  every requirement number and position keeps its bytes (MEDIUM,
  Constitution IX, Constitution X.4, `contradicts`)

### MEDIUM: the precedence sentence `T259` asked for is still owed

- [X] T264 Write the one precedence sentence `T259` asks for into the Assumptions
  paragraph at `specs/007-counters-and-timers/spec.md:321`, beside the existing
  sentence that names the journal the authoritative design record for intent,
  stating that where the journal's resolved scope statement names a boundary, a
  gate, a mechanism, or a manifest that a later requirement in this spec
  withdrew, the requirement in `spec.md` governs, and that the five sentences
  the rule settles are the ones `specs/007-counters-and-timers/citations.md`
  lists under its journal table. `T259`'s body at
  `specs/007-counters-and-timers/tasks.md:2541` names that paragraph and that
  sentence as where the record belongs, and the task is checked `- [X]`;
  `specs/007-counters-and-timers/spec.md:321` reads "The closed journal
  `sg_counters.md` (2026-09-25) is the authoritative design record; where this
  spec compresses it, the journal's resolved scope statement and decision
  entries govern intent." and carries no precedence clause. A search of the
  file for "resolved scope statement names" returns no line, and the only two
  occurrences of "governs" are the Clarification answers at `:30` and `:32`.
  The debt is recorded at `specs/007-counters-and-timers/citations.md:20-24`,
  which states the rule in prose and that the sentence is owed. A reader who
  follows the journal header at `specs/007-counters-and-timers/sg_counters.md:7-12`
  needs the rule in the spec itself, because the journal carries 58 lines the
  gate cannot read and its resolved scope statement names five boundaries later
  requirements withdrew. No requirement number, position, or text moves, and no
  other artifact changes (MEDIUM, T259, Constitution IV, Constitution IX,
  `partial`)

### MEDIUM: the record's anchor population has no stated counting rule, and its two figures are whole-file figures

- [X] T265 State the counting rule the anchor totals in this feature's records
  depend on, then restate the two figures at
  `specs/007-counters-and-timers/citations.md:28-31` under it. The Method and
  population paragraph reads "The population is every anchor the Phase 1
  through Phase 33 record in `specs/007-counters-and-timers/tasks.md` places,
  which is the 724 explicit `path:line` and `path:NN-MM` tokens the file
  carries, plus the 449 bare `:NN` continuations", and it states the unit as
  "tokens" where 724 is occurrences and 476 is distinct. Under the rule this
  preamble states, the population the paragraph defines is
  `tasks.md` lines 1 through 2422 and carries 702 occurrences and 467 distinct
  tokens plus 420 bare `:NN` continuations; 724, 476, and 449 are the figures
  for lines 1 through 2578, the file as it stands, which include the 22
  occurrences and 29 continuations the Phase 34 section appends. The three
  totals a reader compares, 740 and 762 at
  `specs/007-counters-and-timers/tasks.md:2530-2531` and `:2534-2535` and 724
  with 449 in the record, are all correct under a rule and irreconcilable
  without one, and a reader has no way to tell which population any of them
  names. State the rule, name the file set, the line range, the pattern, and
  whether the unit is an occurrence or a distinct token, then restate the two
  figures and correct the word "tokens". The rule and the figures this
  preamble carries are the ones to state. No total in any dated preamble
  changes, and no closed task line moves (MEDIUM, Constitution X.4, T253, T260,
  `partial`)

### LOW: a closed task line names the group shape the code records as rejected

- [X] T266 Record in `specs/007-counters-and-timers/citations.md` the drift in
  `T032` at `specs/007-counters-and-timers/tasks.md:109`, whose task text asks
  for "per-instance provider group reads (US3 scenario 5)" while
  `source/counters/plan.cpp:470-477` records the opposite decision with its
  reason, "One read group per provider, carrying every leaf that provider owns,
  so a fan-out over many objects is one `open` and one sampling action (FR-047)",
  and names `T098` as the task that chose it. `T098` at
  `specs/007-counters-and-timers/tasks.md:389` offered "Split plan read groups
  per provider instance ... or amend the task" and was closed `- [X]`, and the
  second branch left `T032`'s clause standing. `US3` scenario 5 holds on the
  shipped shape: `test/source/counters_objects_test.cpp:410-413` asserts the
  per-core instruction deltas sum to the shared total, so the drift is confined
  to the closed task text, and the code records the decision at its own site as
  Constitution X.1 requires. Add one row naming the task, the line in
  `tasks.md`, the clause as written, the decision that governs it, and the test
  that shows the acceptance scenario holds. `T032` and `T098` keep their bytes
  (LOW, T032, T098, US3 scenario 5, FR-047, Constitution X.1, `partial`)

## Phase 36: Convergence

Appended by `/speckit.converge` after an audit of the branch tip at `f1d3023`
and of the residue the twenty-four waves before it left. Nothing above this
line changed.

Audit evidence, all produced by this pass from the repository root on Linux.
`cmake --preset=dev` and `cmake --build --preset=dev` exit 0 over an
incremental tree in which every target reported `Built target`, and
`ctest --preset=dev` exits 0 with 100.0 percent of 37 tests passed, 0 failed,
0 skipped, in 43.88 s. `ctest --test-dir build -N` exits 0 and reports
`Total Tests: 37`. `python3 tools/prose/prose_gate.py --check all` exits 0 and
`cmake -P cmake/prose-lint.cmake` reproduces that verdict,
`prose-lint: 129 sources, 10737 units examined, 0 findings, 1 skipped`, the
one skipped source being `hwloc.md`, which the gate names unreadable.
`ctest --test-dir build -R prose_gate_fixtures` exits 0 with 1 of 1.
`cmake --build build/dev -t format-check` exits 0.
`cmake --build build/dev -t dbc-gate` exits 0, reporting
`doc-gate: 135 interfaces, 0 gaps` and `pair-gate: 135 interfaces, 0 gaps`.
`cmake -P cmake/spell.cmake` exits 0, and
`python3 tools/pmu_events/update_pmu_events.py --check` exits 0.
`bash tools/dbc/coverage_gate.sh build/coverage/coverage.info` exits 0 over
21 source files at lines 100.0 percent (1955 of 1955), branches 100.0 percent
(705 of 705), and functions 98.0 percent (289 of 295) on an axis the gate does
not score. `cmake --preset=ci-ubuntu` exits 0, and `cmake --build build`, the
release build Principle IX requires once per feature, exits 0 over an
incremental 26-line log holding 0 lines matching the token `warning` and 0
matching `error:`. No forced recompile ran, so this pass states no
static-analysis diagnostic count. `readelf -d` on
`build/dev/example/counters_standalone_example` and on
`build/dev/example/counters_giraffe_example` names `libstdc++.so.6`,
`libgcc_s.so.1`, and `libc.so.6` in each, with zero `speedgun-ng` entries.
`bash test/counters_header_purity.sh` exits 0 at
`counters_header_purity: clean`.

Three prose-gate figures measured at named heads, each reported with the
command that produced it, and a fourth command reported by its exit code:
`python3 tools/prose/prose_gate.py --check all --head
f1d3023` exits 0 at 129 sources and 10737 units, the same pair the
range-less `--check all` reports on this tree; `--head 1827d76` exits 0 at 128
sources and 10250 units; `--head 30f6361` exits 0 at 126 sources and 9919
units; and `python3 tools/prose/prose_gate.py --check prose --mode tree
--paths specs/007-counters-and-timers docs/pages/counters-overhead.md` exits 0
and no unit total. The merge base with `origin/master` is `65beada`.

Counting rule for every anchor total named in this preamble, so a reader can
reproduce it, which is the rule `specs/007-counters-and-timers/citations.md:32-41`
states: the unit is one occurrence of the regular expression
`(?:[\w./-]+/)*[\w.-]+\.[A-Za-z0-9]+:\d+(-\d+)?` matched over a named line
range of a named file, a distinct token is the deduplicated form of that unit,
and a bare continuation is one occurrence of `(?<![\w./-]):\d+\b`, counted the
same way. Applied to `specs/007-counters-and-timers/tasks.md` lines 1 through
2422, the population the Phase 1 through Phase 33 record occupies, the count
is 702 occurrences, 467 distinct tokens, and 420 bare continuations. Applied
to lines 1 through 2578, the file as it stood when the Phase 34 section was
appended, the count is 724 occurrences, 476 distinct tokens, and 449 bare
continuations. Applied to lines 1 through 2905, the file as it stands, the
count is 776 occurrences, 518 distinct tokens, and 472 bare continuations.
Applied to the nine live artifacts over their whole length, which are
`spec.md`, `plan.md`, `research.md`, `data-model.md`, `quickstart.md`,
`sg_counters.md`, and the three files under `contracts/`, the count is 40
occurrences, distributed `spec.md` 28, `quickstart.md` 4, `plan.md` 3,
`research.md` 3, and `contracts/system-contract.md` 2, with none in
`data-model.md`, `sg_counters.md`, `contracts/provider-contract.md`, or
`contracts/measurement-contract.md`.

Coverage of the check: 127 requirement keys, counted as 50 functional
requirements numbering `FR-001` through `FR-050` with no gap, 10 success
criteria numbering `SC-001` through `SC-010` with no gap, 49 user-story
acceptance scenarios counted as US1 7, US2 7, US3 6, US4 6, US5 4, US6 7, US7 6,
US8 6, and 18 edge cases at `specs/007-counters-and-timers/spec.md:195-212`;
45 design keys, counted as 11 data-model entities `E-01` through `E-11` and 19
contract clauses `C-MEA-1` through `C-MEA-7`, `C-PRO-1` through `C-PRO-6`, and
`C-SYS-1` through `C-SYS-6`; and 11 constitution principles with X.1 through
X.4 and XI.1 through XI.6 read one by one. Re-measured here: the 9 public
headers `include/speedgun-ng/counters*.hpp` matches, the 11 translation units
`find source/counters -name '*.cpp'` returns beside the 2 provider-private
headers under `source/counters/detail/`, the 12 `add_executable(counters_)`
calls `test/CMakeLists.txt` carries, the `LCOV_EXCL` count at 301 tokens in the
feature scope and 305 across `source/` plus `include/`, no line matching
`TODO` or `FIXME` over the feature scope, the single `NOLINT` directive at
`include/speedgun-ng/counters_measurement.hpp:634` carrying its reason in the
same comment block at `:630-633`, no `std::atomic` in the push path, no
platform macro in the 9 public headers, and
`.github/workflows/ci.yml:160` running the table check on every change. Every
`file:line` anchor the nine live artifacts place resolves to an existing file
and a line inside it, and every single-line anchor names a non-blank line. The
single code point U+2014 left in this file stands at
`specs/007-counters-and-timers/tasks.md:2731` inside the quoted defective
line `T261` cites as evidence, which the exemption at
`.specify/memory/constitution.md:472-474` covers, and the 58 journal lines
`citations.md:180-186` lists still read 58 under
`grep -c`; neither is a finding.

Seven findings: 4 `contradicts` and 3 `partial`; 0 CRITICAL, 2 HIGH, 3 MEDIUM,
2 LOW. No finding is `missing`, and no `unrequested` addition was found. One
finding sits in executable code and is reproduced outside the repository. The
other six sit in the feature's own record.

Every finding below was re-derived in this session from the files it cites.
The Phase 12 through Phase 35 preambles were read for task identifiers, phase
grouping, and the file paths each task names, and no closure claim in them was
accepted as evidence. The five commits at the tip were read in full: `88bbd2c`
removes the `target_kind::machine` enumerator and touches no other file, and a
search for `target_kind::machine` over `source/`, `include/`, `test/`, and
`example/` returns no line; `01f905b` restates ten artifact sentences and
touches no executable code; `30f6361` restates seven more and touches no
executable code; `1827d76` adds `specs/007-counters-and-timers/citations.md`
and the Phase 34 section and touches no source, header, test, example, tool,
or build file; `f1d3023` fixes the `T116` line and adds the Phase 35 section
beside the restatements, and touches no source, header, test, example, tool,
or build file. The residue the implement and commit passes reported for the
last cycle was assessed on its own evidence, and T269 and T270 record the two
items that survive it.

### HIGH: the push sampling confinement check is one thread id and the last leaf decides it

- [X] T267 Give `detail::push_window` a per-cell owner, or make `push_provider::open`
  refuse a leaf set whose counters carry more than one owner, so the tier-3
  sampling-side violation `FR-035` requires is detected for every counter in
  the plan, and add a trap mode that drives the violation. Today
  `push_window::owner` is one `std::thread::id` at
  `source/counters/push_provider.cpp:36`, the guard at `:40-43` compares
  `std::this_thread::get_id()` against that one value, and
  `source/counters/push_provider.cpp:112` assigns `window->owner =
  match->owner;` inside the per-address loop, so the last matched address
  decides the verdict for all of them. `FR-035` at
  `specs/007-counters-and-timers/spec.md:266` states that `add()` or sampling
  from a thread other than the owning thread MUST be a tier-3 violation, and
  `source/counters/fold.cpp:210-211` already refuses a push decrement at fold
  time, so the fold layer and the window layer disagree about how much the
  confinement is checked. Reproduced in this session by a program compiled
  outside the repository against `build/dev/libspeedgun-ng.a` and this tree's
  headers: one `push_provider` with counter `a` declared on the main thread
  and counter `b` declared on a second thread that joins, one plan over
  `machine/b + machine/a`, and two `sample()` calls on the main thread. With
  the expression written `ca + cb` the run aborts with exit 134 on
  `[precondition] push counters are sampled on the thread that created them
  (FR-035) (predicate: std::this_thread::get_id() == owner) at
  /home/archerc/code/speedgun-ng/source/counters/push_provider.cpp:40`; with
  the same two counters and the expression written `cb + ca` the run exits 0,
  prints `value=10 ratio=1 scaled=0`, and reads counter `b` from a thread
  that does not own it. No test reaches the guard's violating branch: the
  `push-cross-thread` mode at `test/source/counters_trap_fixture.cpp:93-97`
  calls `handle.add(1)` on a foreign thread, which trips the handle guard at
  `include/speedgun-ng/counters_measurement.hpp:333-336` and never opens a
  window, so the plan's own check that the Test Plan row at
  `specs/007-counters-and-timers/plan.md:442` names has no covering test. The
  fix must keep `add()` a plain non-atomic increment and the sample-time read
  a plain load, must keep the check semantic-gated so a release build carries
  none, and must state at the site why a per-cell owner array costs nothing on
  the sampling path (HIGH, FR-035, US4 scenario 2, US7 scenario 5, Constitution
  II, VI, plan: Test Plan threading row, `partial`)

### HIGH: the tip body records a whole-range figure for a command that scans the head

- [X] T268 Anchor the prose-gate figure the body of commit `f1d3023` records in its
  last evidence paragraph, which reads that the gate over the whole repository
  `exits 0 at 128 sources and 10250 units, 0 findings, 1 skipped` beside the
  command `python3 tools/prose/prose_gate.py --check all`, where that command
  carries no `--head` and this pass measured it reporting 129 sources and
  10737 units at exit 0 on the tree `f1d3023` leaves, while the recorded pair
  reproduces only at `python3 tools/prose/prose_gate.py --check all --head
  1827d76`, so the sentence states a verdict for a range the named command does
  not scan. The sentence must name `1827d76` as the commit whose range
  reproduces the figure or state the exit code alone, the anchoring `T199`
  applied to the body of `2c45407`, `T200` applied to the Phase 21 preamble,
  `T202` applied to the body of `5e2181e`, and `T222` applied to the body of
  `6121416`; the rewrite is the pre-merge rewrite the Immutability clause of
  Pull Request Quality permits for a message, and the message must keep its
  template, its `Refs:` and `Approved-by:` footers, and the 50-character title
  `tools/prose/prose_rules.yaml:11` sets. The same body's paragraph on `T265`
  makes the companion claim, which T270 records (HIGH, Constitution X.4, XI.6,
  T199, T222, T265, `contradicts`)

### MEDIUM: two preambles describe the range form as reading committed content

- [X] T269 Correct the two sentences that describe a range-mode prose-gate figure as
  describing committed text, at
  `specs/007-counters-and-timers/tasks.md:2432-2433`, which reads that the
  figure `describes the committed range the gate scans, from the merge base
  with origin/master to 30f6361`, and at
  `specs/007-counters-and-timers/tasks.md:2589-2591`, which reads that the two
  commands `read the committed range from the merge base with origin/master,
  65beada, to 1827d76`. `tools/prose/prose_gate.py:936` calls `read_source` for
  every candidate in both modes, `read_source` at `:842-860` reads
  `repo / path` from disk, and `collect_candidates` at `:587-604` supplies
  only the path list and the per-line authorship filter that
  `git diff -U1` produces, so a range-mode run evaluates working-tree text
  against a range-derived filter and a second sentence at each site contrasts
  the range form with the `--mode tree` form as though only the latter read the
  working tree. The two sentences must state that the candidate set and the
  line filter come from the range while the examined text comes from the
  working tree, and no prose-gate total in any dated preamble may be described
  as reproducible from a commit (MEDIUM, Constitution IV, X.4, T206, T269, T268,
  `contradicts`)

### MEDIUM: the counting rule and the figures that claim to follow it disagree

- [X] T270 Correct the three figures `specs/007-counters-and-timers/citations.md`
  carries beside the counting rule it states at `:32-41`, where `:46` reads
  `163 distinct tokens` for `tasks.md` lines 1 through 2422, `:55` reads `168
  distinct tokens` for lines 1 through 2578, and `:67-69` reads that the
  `476` the Phase 35 preamble carries at
  `specs/007-counters-and-timers/tasks.md:2644` `does not reproduce under the
  rule stated here, which yields 168`. Applying the rule the record states to
  the populations the record names, this pass measured 467 distinct tokens for
  lines 1 through 2422 and 476 for lines 1 through 2578, so the record's own
  two figures reproduce at neither value and the sentence that reports the
  Phase 35 figure as irreproducible inverts the result. The occurrence totals
  702, 724, 420, and 449 in the same paragraph reproduce exactly. The
  distribution at `:60-63` is stale on the same ground: it reads 38 occurrences
  over the nine live artifacts with `research.md` at 1, and this pass measured
  40 with `research.md` at 3, the two extra anchors being
  `source/counters/detail/core.hpp:92-99` and
  `source/counters/plan.cpp:593`, which `f1d3023` added to
  `specs/007-counters-and-timers/research.md:33` and `:37`. Each figure must
  be the one the stated rule yields for the population the record names, the
  rule and its pattern stay as written, and no dated preamble and no closed
  task line moves (MEDIUM, Constitution X.4, T265, T268, `contradicts`)

### MEDIUM: the accepted cost of the fold's address lookup is recorded nowhere

- [X] T271 Record the cost `T091` accepted at its decision site, or state at that site
  that the cost is accepted without a written record, since
  `source/counters/detail/core.hpp:90-99` records why `plan_impl` holds no
  per-composite fold program and names the competing readings, and says
  nothing about the lookup the fold still performs, while
  `specs/007-counters-and-timers/tasks.md:2777-2778` states that the cost `T091`
  named remains and that the decision at `core.hpp:95-99` accepts it, and
  `specs/007-counters-and-timers/citations.md:145-150` states that the decision,
  the competing reading, and the reason are recorded at
  `source/counters/detail/core.hpp:92-99`. The cost is
  `source/counters/fold.cpp:43`, a `std::map<std::string, std::size_t>` lookup
  by leaf address, evaluated once per leaf occurrence per spine node visit, and
  `source/counters/fold.cpp:237-238` calls `fold_core` once per recorded
  column, so a series fold over `N` committed points repeats every lookup `N`
  times. The fold runs off the measurement path by design, and
  `specs/007-counters-and-timers/plan.md:31` designates `recorder::sample()`
  and push `add()` as the critical paths, so the Constitution VII critical-path
  clause at `.specify/memory/constitution.md:203-205` does not reach it; the
  Principle X.1 clause at `:305-316` requires the cost of a chosen approach to
  be surfaced, and no artifact does. The record must name the lookup, its
  frequency over a pair fold, and the ceiling that makes it acceptable, and no
  code change and no requirement change is in scope (MEDIUM, Constitution X.1,
  IV, VII, FR-021, FR-022, T091, T262, `partial`)

### LOW: the functions axis carries six uncovered public-header functions and no gate scores it

- [X] T272 Name the six functions the functions axis leaves uncovered and the reason each
  stays uncovered, or move each onto an executable line a test reaches, where
  `bash tools/dbc/coverage_gate.sh build/coverage/coverage.info` exits 0 at
  functions 98.0 percent (289 of 295) while `tools/dbc/coverage_gate.sh:25-27`
  greps only the `lines.......: 100.0%` line and `:35-37` only the
  `branches.......: 100.0%` line, so no gate scores the axis, and
  `lcov --branch-coverage --list` places the misses in the two public headers
  at `include/speedgun-ng/counters_measurement.hpp`, 96.6 percent of 88
  functions, and `include/speedgun-ng/counters_provider.hpp`, 80.0 percent of
  15, every other measured file reading 100 percent. The six are compiler-
  generated and defaulted special members, among them
  `include/speedgun-ng/counters_provider.hpp:272` and `:273` and
  `include/speedgun-ng/counters_measurement.hpp:619`, so each must either name
  the members and the exclusion that covers it or reach one, and the
  Complexity Tracking row at `specs/007-counters-and-timers/plan.md:457` must
  list them, since that row enumerates every site the P2 exclusion leaves and
  names none in `include/speedgun-ng/` beyond
  `include/speedgun-ng/counters_provider.hpp:97`. The three hard gates of
  Constitution VI at `.specify/memory/constitution.md:190-192` name lines,
  branches, and DBC, all of which hold at 100.0 percent, and no threshold may
  move (LOW, Constitution VI, plan: Complexity Tracking, `partial`)

### LOW: the narrowed tree-mode figure the Phase 35 preamble records no longer reproduces

- [X] T273 Restate the narrowed tree-mode figure the Phase 35 preamble records at
  `specs/007-counters-and-timers/tasks.md:2594-2597`, which reads that the
  narrowed run `exits 0 at 13 sources, 4865 units examined, 0 findings, 0
  skipped` and that the figure describes the working-tree read of those
  thirteen files at that tip, where this pass measured
  `python3 tools/prose/prose_gate.py --check prose --mode tree --paths
  specs/007-counters-and-timers docs/pages/counters-overhead.md` reporting 13
  sources and 5278 units at exit 0, a difference of 413 units that
  `f1d3023` added to the files the form reads. The sentence must name the
  commit whose tree the figure was measured against, the anchoring `T215`
  applied to the coverage-trace freshness claim and `T268` to the tip body, and
  every other `--mode tree` total keeps its stated reading with no number
  (LOW, Constitution X.4, T206, `contradicts`)

## Phase 37: Convergence

Appended by `/speckit.converge` after an audit of the branch tip at `9c5dfa5`
and of the residue the twenty-five waves before it left. Nothing above this
line changed.

Audit evidence, all produced by this pass from the repository root on Linux.
`cmake --preset=dev` and `cmake --build --preset=dev` exit 0.
`ctest --preset=dev` exits 0 with 100.0 percent of 37 tests passed, 0 failed,
0 skipped, 43.64 s. `ctest --test-dir build -N` exits 0 and reports
`Total Tests: 37`. `python3 tools/prose/prose_gate.py --check all` and
`cmake -P cmake/prose-lint.cmake` both exit 0 on the same verdict,
`prose-lint: 132 sources, 11315 units examined, 0 findings, 1 skipped`, the
one skipped source being `hwloc.md`. `ctest --test-dir build -R
prose_gate_fixtures` exits 0 with 1 of 1. `cmake --build build/dev -t
format-check` exits 0. `cmake --build build/dev -t dbc-gate` exits 0,
reporting `doc-gate: 135 interfaces, 0 gaps` and `pair-gate: 135
interfaces, 0 gaps`. `cmake -P cmake/spell.cmake` exits 0, and `python3
tools/pmu_events/update_pmu_events.py --check` exits 0. `bash
tools/dbc/coverage_gate.sh build/coverage/coverage.info` exits 0 over 21
source files at lines 100.0 percent (1955 of 1955), branches 100.0 percent
(705 of 705), and functions 98.0 percent (289 of 295) on an axis the gate
does not score. `cmake --preset=ci-ubuntu` exits 0 and `cmake --build
build`, the release build Principle IX requires once per feature, exits 0
over an incremental tree, so this pass states no static-analysis diagnostic
count. `bash test/counters_header_purity.sh` exits 0 at
`counters_header_purity: clean`. `readelf -d` on
`build/dev/example/counters_standalone_example` and on
`build/dev/example/counters_giraffe_example` names `libstdc++.so.6`,
`libgcc_s.so.1`, and `libc.so.6` in each, with zero `speedgun-ng` entries.
`./build/dev/test/counters_pmu_test` exits 0 reporting `pmu catalog: 581
table-selected entries beyond kernel aliases` beside `pmu availability: 358
countable, 0 permission_blocked, 261 not_encodable, 356 fast_rdpmc`.
`./build/dev/test/counters_trap_fixture push-mixed-owner` exits 134 on the
precondition at `source/counters/push_provider.cpp:120`.

Counting rule for every anchor total named in this preamble, the rule
`specs/007-counters-and-timers/citations.md:32-41` states: the unit is one
occurrence of the regular expression
`(?:[\w./-]+/)*[\w.-]+\.[A-Za-z0-9]+:\d+(-\d+)?` matched over a named line
range of a named file, a distinct token is the deduplicated form of that
unit, and a bare continuation is one occurrence of `(?<![\w./-]):\d+\b`,
counted the same way. Under that rule, applied to
`specs/007-counters-and-timers/tasks.md` lines 1 through 3192, the file as
it stood before this section was appended, the count is 815 occurrences, 548
distinct tokens, and 485 bare continuations. Under the same rule, applied to the nine live artifacts over
their whole length, `spec.md`, `plan.md`, `research.md`, `data-model.md`,
`quickstart.md`, `sg_counters.md`, and the three files under `contracts/`,
the count is 44 occurrences, distributed `spec.md` 28, `plan.md` 7,
`quickstart.md` 4, `research.md` 3, and `contracts/system-contract.md` 2,
with none in `data-model.md`, `sg_counters.md`,
`contracts/provider-contract.md`, or `contracts/measurement-contract.md`.

Coverage of the check: 127 requirement keys, counted as 50 functional
requirements numbering `FR-001` through `FR-050` with no gap, 10 success
criteria numbering `SC-001` through `SC-010` with no gap, 49 user-story
acceptance scenarios counted as US1 7, US2 7, US3 6, US4 6, US5 4, US6 7,
US7 6, US8 6, and 18 edge cases at `specs/007-counters-and-timers/spec.md:195-212`;
45 design keys, counted as 15 research decisions `R-001` through `R-015`, 11
data-model entities `E-01` through `E-11`, and 19 contract clauses
`C-MEA-1` through `C-MEA-7`, `C-PRO-1` through `C-PRO-6`, and `C-SYS-1`
through `C-SYS-6`; and 11 constitution principles with X.1 through X.4 and
XI.1 through XI.6 read one by one. Re-measured here: the 9 headers
`include/speedgun-ng/counters*.hpp` matches, the 11 translation units
`find source/counters -name '*.cpp'` returns, the 12
`add_executable(counters_)` calls `test/CMakeLists.txt` carries, and the
`LCOV_EXCL` count at 301 tokens in the feature scope. Every finding below
was re-derived in this session from the files it cites, and no closure claim
in the Phase 12 through Phase 36 preambles was accepted as evidence.

Two findings: 2 `contradicts`; 1 HIGH, 1 LOW; none is `missing`, `partial`,
or `unrequested`, and neither is a constitution MUST violation. Both sit in
the feature's own record, and the code the earlier waves converged is
untouched by both. `T267`'s mixed-owner fix is landed and its trap fires,
`T271`'s accepted fold-lookup cost is recorded at
`source/counters/detail/core.hpp:100-114`, `T272`'s six functions are named
and explained at `specs/007-counters-and-timers/plan.md:459`, and the four
anchors that row names resolve where it says, at
`include/speedgun-ng/counters_measurement.hpp:619` and
`include/speedgun-ng/counters_provider.hpp:110`, `:281`, and `:367`. `T234`'s
group scratch is sized in the constructor at
`source/counters/linux_pmu/group_io.cpp:191-196` and the sampling-path growth
arm is gone, `T075`'s fast window reads the pair from the leader's page at
`source/counters/linux_pmu/group_io.cpp:341`, and `T145`'s staged duplicate
path lookup and `T096`'s duplicate alias refusal both stand at
`source/counters/system.cpp:308` and `:344`.

Three residue items the previous passes reported were assessed on their own
evidence and none becomes a task. The body of commit `6121416` is not on
this branch: `git merge-base --is-ancestor 6121416 HEAD` exits non-zero
while `git cat-file -t 6121416` still reports a commit, and
`specs/007-counters-and-timers/citations.md:266-273` already records the
unanchored figure that body carries, so the branch holds no commit to amend.
Semantic gating is the established design for this facility: the
mixed-owner check at `source/counters/push_provider.cpp:120` is an
`SG_REQUIRE`, and so are the `add()` guard at
`include/speedgun-ng/counters_measurement.hpp:334`, the window guard at
`source/counters/push_provider.cpp:40`, and the plan binding at
`source/counters/plan.cpp:61`; FR-035 at
`specs/007-counters-and-timers/spec.md:266` classifies cross-thread push
misuse as tier-3, which FR-046 at `spec.md:283` terminates in dev/CI, and
Principle II at `.specify/memory/constitution.md:112-118` requires a
semantic-gated check to emit no code in release, its one always-on exception
being the `SG_REQUIRE_ALWAYS` bounds at `source/counters/plan.cpp:366` that
FR-027 mandates. The functions axis needs no scoring: Constitution VI at
`.specify/memory/constitution.md:190-192` names line, branch, and DBC as its
three hard gates, `tools/dbc/coverage_gate.sh:25-27` and `:35-37` read the
line row and the branch row alone, and
`specs/007-counters-and-timers/plan.md:459` already states that the set of
six is trace-dependent and that every entry in it is a defaulted or
compiler-emitted special member.

### HIGH: the anchor record's distinct-token axis is inverted against the counting rule it states

- [X] T274 Restate the distinct-token axis of
  `specs/007-counters-and-timers/citations.md` to the figures the record's
  own counting rule yields, and correct the paragraph that declares the
  preambles' figures irreproducible. The rule at
  `specs/007-counters-and-timers/citations.md:32-41` states the unit as one
  occurrence of `(?:[\w./-]+/)*[\w.-]+\.[A-Za-z0-9]+:\d+(-\d+)?` with a
  distinct token its deduplicated form. Applying that exact pattern to the
  exact populations the record names yields 467 distinct tokens for
  `specs/007-counters-and-timers/tasks.md` lines 1 through 2422, 476 for
  lines 1 through 2578, and 518 for lines 1 through 2905, which are the
  figures the Phase 34, Phase 35, and Phase 36 preambles carry at
  `specs/007-counters-and-timers/tasks.md:2872`, `:2642`, `:2644`, and
  `:2964`. The record instead carries 163 at
  `specs/007-counters-and-timers/citations.md:46`, 168 at `:55`, a Phase 34
  contribution of 5 distinct tokens at `:57` where the same rule yields 9,
  and the paragraph at `:74-86` asserts that the preambles' 467, 476, and 518
  "do not reproduce under the rule stated above" and are each "superseded by
  the rule's own yields of 163, 168, and 183", which the rule named in the
  same paragraph does not produce; the paragraph dismisses `T270` at `:83-85`
  as stating "the reverse of that result", while the task text of `T270` at
  `specs/007-counters-and-timers/tasks.md:3113-3114` states the correct
  result and `T270` is checked `- [X]`, so the remediation ran opposite to
  the correction the task specifies. The occurrence totals 702, 724, and 776
  and the bare-continuation totals 420, 449, and 472 the same paragraph
  carries reproduce exactly and keep their values. Restate
  `specs/007-counters-and-timers/citations.md:46`, `:55`, and `:57` to 467,
  476, and 9, correct `:74-86` so the preambles' figures are recorded as
  reproducing under the stated rule, drop the dismissal of `T270`, keep the
  counting rule and its pattern as written, and move no dated preamble and no
  closed task line (HIGH, Constitution X.4, Constitution IV, T265, T270,
  `contradicts`)

### LOW: the newest preamble's narrowed tree-mode figure names no head and does not reproduce

- [X] T275 Restate the narrowed `--mode tree` figure the Phase 36 preamble
  carries at `specs/007-counters-and-timers/tasks.md:2942-2950`, which opens
  `Prose-gate figures measured at named heads, each reported with the command
  that produced it`, names three heads (`f1d3023`, `1827d76`, `30f6361`),
  and then gives a fourth figure, `13 sources and 5278 units`, from
  `python3 tools/prose/prose_gate.py --check prose --mode tree --paths
  specs/007-counters-and-timers docs/pages/counters-overhead.md`, which names
  no head and does not reproduce: the same command on the tree `9c5dfa5`
  leaves exits 0 at `13 sources, 5598 units examined, 0 findings, 0 skipped`,
  and `tools/prose/prose_gate.py:587-604`, `:842-860`, and `:936` establish
  that the form takes its candidate set and its per-line authorship filter
  from the range while `read_source` reads the working tree, so no commit
  reproduces its total. The sentence must state the exit code alone, or name
  the commit whose tree the figure was measured against, the anchoring `T206`
  applied to six earlier preambles, `T215` to the coverage-trace freshness
  claim, `T269` to the two range-form sentences, and `T273` to the Phase 35
  narrowed figure. This pass leaves the Phase 36 preamble byte-for-byte as
  written, so the edit belongs to the implement pass that takes the task, and
  no other figure in the file moves (LOW, Constitution X.4, T206, T215, T269,
  T273, `contradicts`)

## Phase 38: Convergence

Appended by `/speckit.converge` after an audit of the branch tip at `b82fb7e`
and of the residue the twenty-six waves before it left. Nothing above this
line changed.

Audit evidence, all produced by this pass from the repository root on Linux.
`cmake --preset=dev` and `cmake --build --preset=dev` exit 0 over an
incremental tree in which every target reported `Built target`.
`ctest --preset=dev` exits 0 with 100.0 percent of 37 tests passed, 0 failed,
0 skipped, 43.80 s. `ctest --test-dir build -N` exits 0 and reports
`Total Tests: 37`, of which `-R counters` reports 16. `python3
tools/prose/prose_gate.py --check all` exits 0, and `cmake -P
cmake/prose-lint.cmake` exits 0 on the same verdict, `prose-lint: 133
sources, 11585 units examined, 0 findings, 1 skipped`, the one skipped
source being `hwloc.md`. `ctest --test-dir build -R prose_gate_fixtures`
exits 0 with 1 of 1. `cmake --build build/dev -t format-check` exits 0.
`cmake --build build/dev -t dbc-gate` exits 0, reporting `doc-gate: 135
interfaces, 0 gaps` and `pair-gate: 135 interfaces, 0 gaps`. `cmake -P
cmake/spell.cmake` exits 0, and `python3 tools/pmu_events/update_pmu_events.py
--check` exits 0. `bash tools/dbc/coverage_gate.sh
build/coverage/coverage.info` exits 0 over 21 source files at lines 100.0
percent (1955 of 1955), branches 100.0 percent (705 of 705), and functions
98.0 percent (289 of 295) on an axis the gate does not score. `cmake
--preset=ci-ubuntu` exits 0, and `cmake --build build`, the release build
Principle IX requires once per feature, exits 0 over a fully incremental
tree whose log holds 0 lines matching the token `warning` and 0 matching
`error:`, so this pass states no static-analysis diagnostic count. `bash
test/counters_header_purity.sh` exits 0 at `counters_header_purity: clean`.
`readelf -d` on `build/dev/example/counters_standalone_example` and on
`build/dev/example/counters_giraffe_example` names `libstdc++.so.6`,
`libgcc_s.so.1`, and `libc.so.6` in each, with zero `speedgun-ng` entries,
and both executables exit 0. `./build/dev/test/counters_pmu_test` exits 0
reporting `pmu catalog: 581 table-selected entries beyond kernel aliases`
beside `pmu availability: 358 countable, 0 permission_blocked, 261
not_encodable, 356 fast_rdpmc` and `perf_event_paranoid = 1`, the level
`docs/pages/counters-overhead.md:310-322` records for this host.
`./build/dev/test/counters_trap_fixture push-mixed-owner` exits 134 on the
precondition at `source/counters/push_provider.cpp:120`.

The coverage tracefile `build/coverage/coverage.info` is timestamped
2026-09-28 10:21, the newest in-scope source mtime is
`source/counters/detail/core.hpp` at 10:15, and the only file in the
feature scope newer than the tracefile is
`test/source/counters_trap_fixture.cpp` at 10:25, which the tracefile does
not carry: its 21 `SF:` entries name 7 headers under
`include/speedgun-ng/` and 14 files under `source/`, and none under `test/`
or `example/`. The trace therefore describes the tree for every file the
gate measures, and the functions axis holds at 98.0 percent. Run on the
same tracefile, `lcov --branch-coverage --list` places the six uncovered
entries at `counters_measurement.hpp` on 96.6 percent of 88 functions and
at `counters_provider.hpp` on 80.0 percent of 15, the two figures
`specs/007-counters-and-timers/plan.md:459` states.

Counting rule for every anchor total in this preamble, the rule
`specs/007-counters-and-timers/citations.md:32-41` states, applied with
CPython 3.14.7 `re.finditer` over whole matches `m.group(0)`: applied to
`specs/007-counters-and-timers/tasks.md` lines 1 through 2422 the count is
702 occurrences, 467 distinct tokens, and 420 bare continuations; lines 1
through 2578 give 724, 476, and 449; lines 1 through 2905 give 776, 518,
and 472; lines 1 through 3192 give 815, 548, and 485; and the nine live
artifacts over their whole length give 44 occurrences, distributed
`spec.md` 28, `plan.md` 7, `quickstart.md` 4, `research.md` 3, and
 `contracts/system-contract.md` 2, with none in `data-model.md`,
 `sg_counters.md`, `contracts/provider-contract.md`, or
 `contracts/measurement-contract.md`; those nine files are the set
 `specs/007-counters-and-timers/citations.md:57-63` names, and
 `citations.md` itself carries 101 occurrences of the path form.

Coverage of the check: 127 requirement keys, counted as 50 functional
requirements numbering `FR-001` through `FR-050` with no gap, 10 success
criteria numbering `SC-001` through `SC-010` with no gap, 49 user-story
acceptance scenarios counted as US1 7, US2 7, US3 6, US4 6, US5 4, US6 7,
US7 6, and US8 6, and 18 edge cases at
`specs/007-counters-and-timers/spec.md:195-212`; 45 design keys, counted as
15 research decisions `R-001` through `R-015`, 11 data-model entities
`E-01` through `E-11`, and 19 contract clauses `C-MEA-1` through
`C-MEA-7`, `C-PRO-1` through `C-PRO-6`, and `C-SYS-1` through `C-SYS-6`;
and 11 constitution principles with X.1 through X.4 and XI.1 through XI.6
read one by one. Re-measured here: the 9 headers
`include/speedgun-ng/counters*.hpp` matches, the 11 translation units
`find source/counters -name '*.cpp'` returns beside the 2 provider-private
headers under `source/counters/detail/`, the 12 `add_executable(counters_)`
calls `test/CMakeLists.txt` carries, the `LCOV_EXCL` count at 301 tokens in
the feature scope, 305 across `source/` plus `include/`, and 4 in
`include/speedgun-ng/dbc.hpp`, no line matching `TODO` or `FIXME` over the
feature scope, the single `NOLINT` directive at
`include/speedgun-ng/counters_measurement.hpp:634` carrying its reason in
the same comment block at `:630-633`, no `std::atomic` in
`source/counters/push_provider.cpp` or in the push handle, and no platform
macro in the 9 public headers.

Every anchor the nine live artifacts place was resolved and read: each one
lands inside the file it names, and each single-line anchor names a
non-blank line. Every explicit `path:line` anchor in the nine live artifacts
and in `tasks.md` was then resolved mechanically, and 13 distinct explicit
anchors in `tasks.md` fail the first two drift criteria the record states
at `specs/007-counters-and-timers/citations.md:91-92`. Six of the thirteen
sit in a row the record already carries, at
`specs/007-counters-and-timers/citations.md:109`, `:112`, `:132`, `:136`,
`:141`, and `:157`; one at
`specs/007-counters-and-timers/tasks.md:988` names a file under
`specs/002-prose-commit-lint` and lies outside this feature's scope; one at
`specs/007-counters-and-timers/tasks.md:2238` is quoted inside a finding as
the anchor that finding reports; and five are absent from the record in
every row. The short form `plan.md:147-164` at
`specs/007-counters-and-timers/tasks.md:1312` and `:1828` fails the second
criterion as well and joins the five. The nine commits at the tip were read
in full with `git show`: `88bbd2c` removes the `target_kind::machine`
enumerator and touches one file, and a search for `target_kind::machine`
over `source/`, `include/`, `test/`, and `example/` returns no line;
`01f905b`, `30f6361`, `1827d76`, `f1d3023`, `9c5dfa5`, and `b82fb7e` touch
no executable code; `31363e8` adds the mixed-owner refusal at
`source/counters/push_provider.cpp:120`, the precondition it states at
`include/speedgun-ng/counters_push.hpp:88-90`, and the trap mode that
drives it; and `b60b361` adds the accepted cost of the fold's address
lookup at `source/counters/detail/core.hpp:100-114`. No closure claim in
the Phase 12 through Phase 37 preambles was accepted as evidence.

Three findings: 3 `partial`; 0 CRITICAL, 0 HIGH, 2 MEDIUM, 1 LOW. No
finding is `missing`, `contradicts`, or `unrequested`, and none is a
constitution MUST violation. All three sit in the feature's own record, and
the code the earlier waves converged is untouched by every one of them.
`T267`'s mixed-owner refusal is landed and its trap fires,
`source/counters/plan.cpp:61-62` carries the per-thread binding its proof
rests on, `T271`'s accepted fold-lookup cost is recorded at
`source/counters/detail/core.hpp:100-114`, `T272`'s six functions are named
and explained at `specs/007-counters-and-timers/plan.md:459` with the two
`lcov` figures above confirming them, `T274`'s two figures now stand on the
lines the record's table names, and `T091`'s settlement holds at
`source/counters/plan.cpp:593`, the call site its task text never cited.

The residue the last implement pass reported was assessed on its own
evidence, and two of the four items do not survive it. The false clause is
gone: a search for the token `existing tracked file` over the tree's
Markdown returns no line, and
`specs/007-counters-and-timers/citations.md:46-50` carries the attribution
rule the correction installed, with no second instance of the withdrawn
claim anywhere in the tree. The coverage tracefile does describe the tree
and the functions axis holds, on the evidence in the paragraph above.
`T274`'s stale citation and the engine question both survive; they are
T278 and T277 below.

### MEDIUM: five `plan.md` anchors the record does not carry, created by one inserted line

- [X] T276 Record the drifted `plan.md` anchors of `T195`, `T231`, `T236`,
  `T255`, `T256`, and the Phase 23 preamble, in
  `specs/007-counters-and-timers/citations.md`, where `1827d76` added one
  line at `specs/007-counters-and-timers/plan.md:74` and every anchor below
  it shifted down by one, and where five distinct anchors now name a blank
  line. `T195` at
  `specs/007-counters-and-timers/tasks.md:1066` names
  `specs/007-counters-and-timers/plan.md:298` for the Files-and-duties row
  and `:325` for the sentence naming the nine headers, where `:298` and
  `:325` are blank, the row stands at `:301` and reads
  `| \`include/speedgun-ng/counters*.hpp\` (9 files) |`, and the sentence
  stands at `:328`; the Phase 23 preamble at
  `specs/007-counters-and-timers/tasks.md:1297-1299` names the same two.
  `T231` at `:1828` and the Phase 23 preamble at `:1312` name
  `plan.md:147-164`, whose opening line is blank, the block heading stands
  at `:148`, and the eleven counters test sources sit at `:149-159`.
  `T236` at `:1968` names
  `specs/007-counters-and-timers/plan.md:282` for the scope-misuse
  sentence, which is blank and the sentence stands at `:283`, and `T255` at
  `:2355` quotes the same number in reporting what `T236` restated. `T256`
  at `:2365` names `specs/007-counters-and-timers/plan.md:323` for the
  Key-properties sentence, which is blank and the sentence stands at `:324`.
  `git show 01f905b:specs/007-counters-and-timers/plan.md` places the
  scope-misuse sentence at `:282`, the Key-properties sentence at `:323`,
  and the test-source heading at `:147`, so the shift is the one line
  `1827d76` added, and the pre-shift readings are confirmed for each. The
  record's drift criteria at
  `specs/007-counters-and-timers/citations.md:91-92` name the blank-line
  case, and the record's anchor table at `:107-158` carries rows for the
  other drifted populations, naming `T072`, `T076`, `T091`, `T100`, `T120`,
  `T126`, `T129`, `T131`, `T139`, `T142`, `T145`, `T147`, `T152`, `T156`,
  `T166`, `T170`, `T171`, `T178`, `T179`, `T185`, `T189`, `T223`, and the
  Phase 20, Phase 21, and Phase 22 preambles, with no row for any of the six
  texts this task names; the table's own re-anchor section at `:275-282`
  states that it covers the texts citing the record, so the gap is inside the
  record's stated scope. Each row names the task, the line in `tasks.md`,
  the anchor as written, and the line now holding the claim, the rows land
  at the end of the table, every dated preamble and every closed task line
  keeps its bytes under the Immutability clause of Pull Request Quality,
  and no figure in any dated preamble moves (MEDIUM, Constitution IV,
  Constitution X.4, T260, T265, T253, `partial`)

### MEDIUM: the counting rule names a pattern and no engine, and three engines disagree on it

- [X] T277 State the engine the counting rule at
  `specs/007-counters-and-timers/citations.md:32-41` requires, or restate
  the rule so that one engine determines the figure, where the paragraph
  states its purpose as being that `a reader can reproduce any of them` and
  names the unit as one occurrence of
  `(?:[\w./-]+/)*[\w.-]+\.[A-Za-z0-9]+:\d+(-\d+)?` with a distinct token its
  deduplicated form, naming no engine and no whole-match basis. Measured on
  `specs/007-counters-and-timers/tasks.md` lines 1 through 2422 in this
  session, the same rule over the same population yields three figures:
  GNU grep 3.12 `grep -oE` returns 0 matches and prints the warnings `? at
  start of expression` and `stray \ before d`, because the pattern's `(?:`
  is outside POSIX ERE; CPython 3.14.7 `re.findall` returns 163 distinct
  values, because the pattern holds exactly one capturing group `(-\d+)?`
  and `findall` returns that group; and CPython 3.14.7 `re.finditer` with
  `m.group(0)` returns 467, which is the figure
  `specs/007-counters-and-timers/citations.md:46` and the Phase 34, Phase
  35, and Phase 36 preambles carry. `grep -oP` on the same pattern returns
  467 as well, so the second construct the rule states,
  `(?<![\w./-]):\d+\b` for a bare continuation, is outside POSIX ERE too.
  The record already explains what the discarded figures counted, at
  `specs/007-counters-and-timers/citations.md:81-84`, and the engine facts
  live only in the body of commit `b82fb7e`, a landed message the
  Immutability clause of Pull Request Quality makes immutable, so no text
  outside the record carries them for a reader who consults the record. The
  paragraph must name the engine and the whole-match basis beside the
  pattern it already states, the eight figures it and the table at
  `specs/007-counters-and-timers/citations.md:46`, `:55`, `:56`, and
  `:75-86` carry keep their values, and no dated preamble and no closed
  task line moves (MEDIUM, Constitution IV, Constitution X.4, T265, T270,
  T274, `partial`)

### LOW: `T274`'s own citation is one line past the figure it names

- [X] T278 Add the row the record's re-anchor table lacks for `T274`, whose
  task text at `specs/007-counters-and-timers/tasks.md:3308-3330` cites
  `specs/007-counters-and-timers/citations.md:46` and `:55` for the two
  figures the rule yields and `:57` for a third, where `:46` and `:55` land
  on `467` and `476` as the sentence says and `:57` is blank of the figure,
  the Phase 34 contribution of 9 distinct tokens sitting at `:56` after
  `b82fb7e` rewrote the sentence, and where the table at
  `specs/007-counters-and-timers/citations.md:289-290` already carries a row
  for each of the two that still land, reading `:46` and `:55` unchanged,
  with no row for the third. The table's rows are keyed by the anchor as
  written and `:57-58` at `:291` belongs to a different text, so the new
  row must name the task and the anchor together. The row lands at the end
  of that table, `T274` keeps its bytes, and the record's own counting rule
  and every figure it carries keep their values (LOW, Constitution X.4,
  T274, T260, `partial`)

## Phase 39: Convergence

Appended by `/speckit.converge` after an audit of the branch tip at `9da43ac`
and of the residue the twenty-seven waves before it left. Nothing above this
line changed.

Audit evidence, all produced by this pass from the repository root on Linux.
`cmake --preset=dev`, `cmake --build --preset=dev`, and `ctest --preset=dev`
exit 0, the last with 100.0 percent of 37 tests passed, 0 failed, 0 skipped,
43.84 s. `ctest --test-dir build -N` exits 0 and reports `Total Tests: 37`, of
which `-R counters` reports 16. `python3 tools/prose/prose_gate.py --check
all` and `cmake -P cmake/prose-lint.cmake` both exit 0 on the same verdict,
`prose-lint: 134 sources, 11867 units examined, 0 findings, 1 skipped`, the one
skipped source being `hwloc.md`. `ctest --test-dir build -R prose_gate_fixtures`
exits 0 with 1 of 1. `cmake --build build/dev -t format-check` exits 0.
`cmake --build build/dev -t dbc-gate` exits 0, reporting `doc-gate: 135
interfaces, 0 gaps` and `pair-gate: 135 interfaces, 0 gaps`. `cmake -P
cmake/spell.cmake` exits 0, and `python3 tools/pmu_events/update_pmu_events.py
--check` exits 0. `bash tools/dbc/coverage_gate.sh build/coverage/coverage.info`
exits 0 over 21 source files at lines 100.0 percent (1955 of 1955), branches
100.0 percent (705 of 705), and functions 98.0 percent (289 of 295) on an axis
the gate does not score. `cmake --preset=ci-ubuntu` exits 0 and `cmake --build
build`, the release build Principle IX requires once per a feature, exits 0
over a fully incremental tree carrying 0 compile actions and 0 lines matching
`error:`; the 2 lines matching `warning` are CPack duplicate-include messages
raised at configure by the vendored `external/zlib` and
`external/hdrhistogram_c`, so this pass states no static-analysis diagnostic
count. `bash test/counters_header_purity.sh` exits 0 at `counters_header_purity:
clean` and `bash test/counters_push_atomic_scan.sh` exits 0 at
`counters_push_atomic_scan: clean`. `readelf -d` on
`build/dev/example/counters_standalone_example` and on
`build/dev/example/counters_giraffe_example` names `libstdc++.so.6`,
`libgcc_s.so.1`, and `libc.so.6` in each, with 0 `speedgun-ng` entries, and
both executables exit 0. `./build/dev/test/counters_trap_fixture
push-mixed-owner` exits 134. `./build/dev/test/counters_pmu_test` exits 0
reporting `pmu catalog: 581 table-selected entries beyond kernel aliases`
beside `pmu availability: 358 countable, 0 permission_blocked, 261
not_encodable, 356 fast_rdpmc`, with `perf_event_paranoid = 1`.

The coverage tracefile `build/coverage/coverage.info` is timestamped
2026-09-28 10:21:54, the newest in-scope source mtime is
`source/counters/detail/core.hpp` at 10:15:57, and no file under
`source/counters` or matching `include/speedgun-ng/counters*.hpp` is newer
than the tracefile, so the trace describes every file the gate measures. Run on
that tracefile, `lcov --branch-coverage --list` places the six uncovered
functions at `include/speedgun-ng/counters_measurement.hpp` on 96.6 percent of
88 and at `include/speedgun-ng/counters_provider.hpp` on 80.0 percent of 15, the
two figures `specs/007-counters-and-timers/plan.md:459` states, and the six are
the three defaulted `expression` default constructors plus the three deleting
destructors the compiler emits for the defaulted virtual destructors at
`include/speedgun-ng/counters_measurement.hpp:619` and
`include/speedgun-ng/counters_provider.hpp:110`, `:281`, and `:367`. All four
anchors resolve where the row says they do.

Counting rule for every anchor total in this preamble, the rule
`specs/007-counters-and-timers/citations.md:32-41` states, applied with
CPython 3.14.7 `re.finditer` over whole matches `m.group(0)`: applied to
`specs/007-counters-and-timers/tasks.md` lines 1 through 3599 the count is 880
occurrences, 579 distinct tokens, and 540 bare continuations; lines 1 through
3362 give 844, 565, and 501; lines 1 through 3192 give 815, 548, and 485, and
lines 1 through 2422 give 702, 467, and 420, so the figures the Phase 37 and
Phase 38 preambles carry reproduce exactly. The nine live artifacts over their
whole length give 44 occurrences, distributed `spec.md` 28, `plan.md` 7,
`quickstart.md` 4, `research.md` 3, and `contracts/system-contract.md` 2, with
none in `data-model.md`, `sg_counters.md`, `contracts/provider-contract.md`, or
`contracts/measurement-contract.md`; those nine files are the set
`specs/007-counters-and-timers/citations.md:57-63` names.
`specs/007-counters-and-timers/citations.md` over its whole length gives 114
occurrences of the path form and 101 distinct tokens.

Coverage of the check: 127 requirement keys, counted as 50 functional
requirements numbering `FR-001` through `FR-050` with no gap, 10 success
criteria numbering `SC-001` through `SC-010` with no gap, 49 user-story
acceptance scenarios counted as US1 7, US2 7, US3 6, US4 6, US5 4, US6 7, US7 6,
and US8 6, and 18 edge cases at
`specs/007-counters-and-timers/spec.md:195-212`; 45 design keys, counted as 15
research decisions `R-001` through `R-015`, 11 data-model entities `E-01`
through `E-11`, and 19 contract clauses `C-MEA-1` through `C-MEA-7`, `C-PRO-1`
through `C-PRO-6`, and `C-SYS-1` through `C-SYS-6`; and 11 constitution
principles with X.1 through X.4 and XI.1 through XI.6 read one by one.
Re-measured here: the 9 headers `include/speedgun-ng/counters*.hpp` matches,
the 11 translation units `find source/counters -name '*.cpp'` returns beside
the 2 provider-private headers under `source/counters/detail/` and the 5 under
`source/counters/linux_pmu/`, the 12 `add_executable(counters_)` calls
`test/CMakeLists.txt` carries, the `LCOV_EXCL` count at 301 tokens in the
feature scope, 305 across `source/` plus `include/`, and 4 in
`include/speedgun-ng/dbc.hpp`, no line matching `TODO` or `FIXME` over the
feature scope, the single `NOLINT` directive at
`include/speedgun-ng/counters_measurement.hpp:634` carrying its reason in the
same comment block at `:630-633`, and no `std::atomic` in
`source/counters/push_provider.cpp` or in the push handle. Every anchor the
nine live artifacts place was resolved and read: the 24 checked this pass land
inside the file they name on the text the sentence describes. No closure claim
in the Phase 12 through Phase 38 preambles was accepted as evidence.

Six findings: 1 `contradicts`, 5 `partial`; 0 CRITICAL, 0 HIGH, 1 MEDIUM, 5 LOW.
No finding is `missing` or `unrequested`, and none is a constitution MUST
violation. Every one sits in the feature's own record or in a live artifact
beside it, and the code the earlier waves converged is untouched by all six.
`88bbd2c` removed `target_kind::machine` and a search for that name over
`source/`, `include/`, `test/`, and `example/` returns no line.
`31363e8` carries the mixed-owner refusal at
`source/counters/push_provider.cpp:120` with the precondition it states at
`include/speedgun-ng/counters_push.hpp:88-90`, and its trap exits 134. `b60b361`
carries the accepted fold-lookup cost at
`source/counters/detail/core.hpp:100-114`, naming the `by_address.at` lookup at
`source/counters/fold.cpp:43` that resolves. `T234`'s group scratch is sized in
the constructor at `source/counters/linux_pmu/group_io.cpp:191-196` and the
sampling-path growth arm is gone. `T091`'s settlement holds at
`source/counters/plan.cpp:593`, the call site its task text never cited.
`source/counters/plan.cpp:469` stores the one bound target, `:501` hands that
value to every provider's `open()`, and `:61-62` binds each sampling action to
the thread that compiled the plan, so FR-024 and FR-031 hold as
`specs/007-counters-and-timers/spec.md:34` states.

The residue the last implement pass reported was assessed on its own evidence.
Three of the five items survive as findings below: the Phase 38 preamble's
`citations.md` figure is T280, `T195`'s two anchors are T281, and `T277`'s
engine figure is T282. Two do not. The rule now states its engine, naming
CPython 3.14.7 `re.finditer` with whole matches at
`specs/007-counters-and-timers/citations.md:35-36`, and the occurrence and
distinct senses are carried explicitly at `:37-38`, so the record reports both
without ambiguity. The functions axis needs no scoring and no further record:
Constitution VI at `.specify/memory/constitution.md:190-192` names line, branch,
and DBC as its three hard gates, `tools/dbc/coverage_gate.sh:25-27` and `:35-37`
read the line row and the branch row alone, and
`specs/007-counters-and-timers/plan.md:459` already names the six and explains
that every entry in it is a defaulted or compiler-emitted special member. Two
findings below were not on that list: the link manifest is T279 and the P2
population is T283.

### MEDIUM: the link manifest is enumerated from one tool and measured from the other

- [X] T279 Record the measured link manifest in
  `specs/007-counters-and-timers/citations.md`, and correct the enumerated set
  at `specs/007-counters-and-timers/spec.md:31` and at
  `specs/007-counters-and-timers/quickstart.md:25`, where the T120
  clarification and section 2 each state that `ldd` and `readelf -d` name the
  same four runtime libraries, `libstdc++`, `libm`, `libgcc_s`, and `libc`, and
  the SC-001 row at `specs/007-counters-and-timers/quickstart.md:137` reports
  the measured figure `readelf -d ... | grep -c NEEDED` at 4. The built
  artifact contradicts all three: `readelf -d
  build/dev/example/counters_standalone_example` carries 3 `NEEDED` entries,
  `libstdc++.so.6`, `libgcc_s.so.1`, and `libc.so.6`, and names no `libm`
  entry, while `ldd` on the same binary names 5 including `libm.so.6`, so the
  two tools name different sets and the recorded count of 4 reproduces under
  no command. The record's one link-manifest row, at
  `specs/007-counters-and-timers/citations.md:200`, states the correct three
  for `readelf -d` and covers only the journal sentence at
  `specs/007-counters-and-timers/sg_counters.md:1049`, so no row covers these
  three sites. `FR-049` at `specs/007-counters-and-timers/spec.md:286` and
  `SC-001` at `:308` keep their wording: the example exits 0, the manifest
  names the platform C and C++ runtime only, and no `speedgun-ng` entry and no
  third-party entry appears. Correct the two live sentences in place, record
  the measured set and count for the dated SC-001 row beside it, and move no
  requirement, gate, or threshold (MEDIUM, Constitution X.4, Constitution IV,
  FR-049, SC-001, T120, T256, `contradicts`)

### LOW: four figures the tree moved under, and one the record names and leaves open

- [X] T280 Add the row the record lacks for the Phase 38 preamble's
  `citations.md` figure at
  `specs/007-counters-and-timers/tasks.md:3429-3430`, which states that
  `specs/007-counters-and-timers/citations.md` itself carries 101 occurrences
  of the path form. Under the rule at
  `specs/007-counters-and-timers/citations.md:32-41` applied with CPython 3.14.7
  `re.finditer` over whole matches `m.group(0)`, that file carries 114
  occurrences and 101 distinct tokens over its whole length at this head, and
  101 occurrences over 90 distinct at the parent `b82fb7e`, so 101 was the
  occurrence figure at the commit that authored the sentence and the same
  commit's own edit to the record added 13 further occurrences. The preamble is
  a dated record under the Immutability clause of Pull Request Quality, so the
  correction belongs in the record, which carries no row for it. The row names
  the task and the figure together, states both senses so 101 is not read as
  the occurrence total again, the counting rule at `:32-41` and every figure
  the record carries keep their values, and no dated preamble moves (LOW,
  Constitution X.4, T265, T277, `partial`)

- [X] T281 Record in `specs/007-counters-and-timers/citations.md` that the two
  `T195` anchors were wrong before the insert `T276` attributed them to, at
  the rows `specs/007-counters-and-timers/citations.md:159-160`, which give
  `plan.md:298` and `plan.md:325` as blank at this head with the landings at
  `:301` and `:328`, both named by `T276`. At `01f905b`, the parent of
  `1827d76` which added one line at `plan.md:74`,
  `specs/007-counters-and-timers/plan.md:298` held the table header
  `| Path | Duty | Requirements |` and the Files-and-duties row stood at
  `:300`, and `:325` held the heading `### Public API surface added` with the
  nine-headers sentence at `:327`, so each cited anchor was already two lines
  off its claim and the insert accounts for the third. `T276`'s own task text
  at `specs/007-counters-and-timers/tasks.md:3530-3533` confirms the pre-shift
  reading for `T236`, `T256`, and `T231` alone while its closing sentence
  covers all six, and the drift criterion that applies changed with the insert,
  criterion 2 at
  `specs/007-counters-and-timers/citations.md:92` at this head and criterion 3
  at `:93` before it. Both rows keep their landing and their `Named by` value,
  each gains the pre-insert reading and the head it was read at, `T276` and
  every dated preamble keep their bytes, and no landing moves (LOW,
  Constitution X.4, T253, T260, T276, `partial`)

- [X] T282 Record in `specs/007-counters-and-timers/citations.md` the
  occurrence total the `T277` task text attributes to `grep -oP`, at
  `specs/007-counters-and-timers/tasks.md:3568-3570`, which states that
  `grep -oP` on the same pattern returns 467 as well. Over
  `specs/007-counters-and-timers/tasks.md` lines 1 through 2422 with GNU grep
  3.12, `grep -oP` returns 702 occurrences and reaches 467 only after
  `sort -u`, while CPython 3.14.7 `re.finditer` over whole matches returns the
  same 702 occurrences and 467 distinct; `grep -oE` returns 0 with the two
  warnings the task names, and `re.findall` returns 163 because the pattern
  holds one capturing group, so the figure the sentence gives is the distinct
  total presented as the occurrence total. The rule at
  `specs/007-counters-and-timers/citations.md:32-41` already names its engine
  and its whole-match basis at `:35-36` and carries both senses at `:37-38`,
  so it needs no correction; the record needs the row stating what each engine
  returns, so a reader consulting the closed task line is not left with 467 as
  an occurrence count. The task line and every dated preamble keep their bytes
  (LOW, Constitution X.4, T265, T270, T277, `partial`)

- [X] T283 Correct the coverage-exclusion population the plan states twice in
  one row, at `specs/007-counters-and-timers/plan.md:457`, which reads `What
  remains is 296 tokens in three categories` and, later in the same row, `The
  token count moved from 296 to 304`, so the two sentences disagree with each
  other and neither matches the tree. The feature scope carries 301
  `LCOV_EXCL` tokens over 301 marker lines, which the command
  `specs/007-counters-and-timers/quickstart.md:169` names reproduces, and the
  count moved from 304 to 301 at `b270503`, so the last movement is unrecorded.
  The row is the registered P2 justification under Constitution I, and its
  figure governs the population the exception covers. Restate the leading
  figure to 301, record the 304 to 301 movement and the commit that made it,
  leave the three categories, the per-site reasons, and the withdrawn
  kernel-gate reason as written, and record beside it the corrected total for
  the dated `quickstart.md:169` row, whose own command reproduces 301 (LOW,
  Constitution I, P2, Constitution X.4, T066, T234, T272, `partial`)

- [X] T284 State the whole-repository prose-gate verdict that
  `specs/007-counters-and-timers/citations.md:263-266` carries by exit code and
  leaves open, naming `python3 tools/prose/prose_gate.py --check prose --mode
  tree` as the form that reads the whole repository. That form exits 1 at
  `prose-lint: 183 sources, 18818 units examined, 108 findings, 0 skipped`, and
  the record states no verdict for it. Every one of the 108 falls outside the
  feature scope: 89 in `specs/001-dbc-facility/`, 9 in `test/`, 5 in
  `tools/dbc/`, 3 in `docs/pages/dbc-overhead.md`, and 2 in
  `include/speedgun-ng/`. The branch touches two of those files,
  `specs/001-dbc-facility/tasks.md` and `test/source/dbc_test.cpp`, carrying 24
  of the 108, and the green range form establishes that no finding falls on a
  line the branch's diff added or modified. The constitution's own gate is the
  range form at `.specify/memory/constitution.md:245-248`, so nothing is
  weakened and no threshold moved; state the tree form's exit code, its figure,
  and the fact that every finding is pre-existing and outside the feature, so a
  reader following the record's pointer is not left to infer a zero the command
  does not return. No gate changes, no marker is added, and no file outside
  `specs/007-counters-and-timers/citations.md` moves (LOW, Constitution VIII,
  Constitution X.4, T206, T268, `partial`)

## Phase 40: Convergence

Appended by `/speckit.converge` after an audit of the branch tip at `8a62b69`
and of the residue the twenty-eight waves before it left. Nothing above this
line changed.

Audit evidence, all produced by this pass from the repository root on Linux.
`python3 tools/prose/prose_gate.py --check all` exits 0, and
`cmake -P cmake/prose-lint.cmake` exits 0 on the same verdict,
`prose-lint: 135 sources, 12313 units examined, 0 findings, 1 skipped`, the
one skipped source being `hwloc.md`. Both forms take their candidate file set
and their per-line authorship filter from the range `collect_candidates` at
`tools/prose/prose_gate.py:587` runs as `git diff -U1` at `:597` over the
merge base with `origin/master` and the head, defaulting to `HEAD`, and
`read_source` at `:842` reads the examined text from the working tree in both
modes, as the call at `:936` shows. The working tree is clean and equals
`8a62b69`, so at this head the total reproduces at
`python3 tools/prose/prose_gate.py --check all --head 8a62b69`, which reports
the same 135 sources and 12313 units; the same command at `b60b361`,
`31363e8`, `9c5dfa5`, `b82fb7e` and `9da43ac` reports 131 and 10906, 130 and
10849, 132 and 11315, 133 and 11585, and 134 and 11867, so the figure moves
with the range. `python3 tools/prose/prose_gate.py --check prose --mode tree`
exits 1 at
`prose-lint: 183 sources, 19152 units examined, 108 findings, 0 skipped`,
distributed 89 in `specs/001-dbc-facility/`, 9 in `test/`, 5 in `tools/dbc/`,
3 in `docs/pages/dbc-overhead.md` and 2 in `include/speedgun-ng/`, every one
outside the feature scope; the branch touches two of the files carrying them,
`specs/001-dbc-facility/tasks.md` with 22 and `test/source/dbc_test.cpp` with
2. Read narrowed to this feature,
`python3 tools/prose/prose_gate.py --check prose --mode tree --paths
specs/007-counters-and-timers docs/pages/counters-overhead.md` exits 0 at
`13 sources, 6357 units examined, 0 findings, 0 skipped`.

`ctest --test-dir build -N` exits 0 and reports `Total Tests: 37`, of which
`ctest --test-dir build -N -R counters` reports 16. `cmake --preset=dev`
exits 0, and `cmake --build --preset=dev` exits 0 over a fully incremental
tree whose 26-line log reports `Built target` 26 times. `ctest --preset=dev`
exits 0 with 100.0 percent of 37 tests passed, 0 failed, 0 skipped, in 43.89 s.
`ctest --test-dir build -R prose_gate_fixtures` exits 0 with 1 of 1.
`cmake --build build/dev -t format-check` exits 0.
`cmake --build build/dev -t dbc-gate` exits 0, reporting
`doc-gate: 135 interfaces, 0 gaps` and `pair-gate: 135 interfaces, 0 gaps`.
`cmake -P cmake/spell.cmake` exits 0, and
`python3 tools/pmu_events/update_pmu_events.py --check` exits 0.
`bash tools/dbc/coverage_gate.sh build/coverage/coverage.info` exits 0 over
21 source files at lines 100.0 percent (1955 of 1955), branches 100.0 percent
(705 of 705), and functions 98.0 percent (289 of 295) on an axis no gate
scores. The tracefile is timestamped 2026-09-28 10:21:54,
`find source include test example tools/pmu_events -newer
build/coverage/coverage.info` names `test/source/counters_trap_fixture.cpp`
alone, and the tracefile's 21 `SF:` entries name no path under `test/` or
`example/`, so the trace describes every file the gate measures.
`cmake --preset=ci-ubuntu` exits 0, and `cmake --build build`, the release
build Principle IX requires once per feature, exits 0 over a fully incremental
26-line log holding 0 lines matching the token `warning` and 0 matching
`error:`. No forced recompile ran, so this pass states no static-analysis
diagnostic count. `bash test/counters_header_purity.sh` exits 0 at
`counters_header_purity: clean`, and `bash test/counters_push_atomic_scan.sh`
exits 0 at `counters_push_atomic_scan: clean`. `readelf -d` on
`build/dev/example/counters_standalone_example` and on
`build/dev/example/counters_giraffe_example` names `libstdc++.so.6`,
`libgcc_s.so.1` and `libc.so.6` in each, with 0 `speedgun-ng` entries, and
both executables exit 0; `ldd` on the standalone example names those three
beside `libm.so.6`, `linux-vdso.so.1` and the loader.
`./build/dev/test/counters_pmu_test` exits 0 reporting
`pmu catalog: 581 table-selected entries beyond kernel aliases` beside
`pmu availability: 358 countable, 0 permission_blocked, 261 not_encodable,
356 fast_rdpmc` with `perf_event_paranoid = 1`.
`./build/dev/test/counters_trap_fixture push-mixed-owner` exits 134 on the
precondition at `source/counters/push_provider.cpp:120`. The feature scope
carries 301 `LCOV_EXCL` tokens.

Counting rule for every anchor total in this preamble, the rule
`specs/007-counters-and-timers/citations.md:32-41` states, applied with
CPython 3.14.7 `re.finditer` over whole matches `m.group(0)`: applied to
`specs/007-counters-and-timers/tasks.md` lines 1 through 2422 the count is 702
occurrences, 467 distinct tokens, and 420 bare continuations; lines 1 through
2578 give 724, 476, and 449; lines 1 through 2905 give 776, 518, and 472; lines
1 through 3192 give 815, 548, and 485; lines 1 through 3362 give 844, 565, and
501; lines 1 through 3599 give 880, 579, and 540, so the figures the Phase 37,
Phase 38, and Phase 39 preambles carry reproduce exactly; lines 1 through 3852
give 921, 594, and 557 before this section appends its own. The nine live
artifacts over their whole length give 44 occurrences, distributed `spec.md` 28,
`plan.md` 7, `quickstart.md` 4, `research.md` 3, and
`contracts/system-contract.md` 2, with none in `data-model.md`,
`sg_counters.md`, `contracts/provider-contract.md`, or
`contracts/measurement-contract.md`. `specs/007-counters-and-timers/citations.md`
over its whole length gives 129 occurrences and 112 distinct tokens, the pair
its own section records.

Coverage of the check: 127 requirement keys, counted as 50 functional
requirements numbering `FR-001` through `FR-050` with no gap, 10 success
criteria numbering `SC-001` through `SC-010` with no gap, 49 user-story
acceptance scenarios counted as US1 7, US2 7, US3 6, US4 6, US5 4, US6 7, US7 6,
and US8 6, and 18 edge cases at
`specs/007-counters-and-timers/spec.md:195-212`; 45 design keys, counted as 15
research decisions `R-001` through `R-015`, 11 data-model entities `E-01`
through `E-11`, and 19 contract clauses `C-MEA-1` through `C-MEA-7`,
`C-PRO-1` through `C-PRO-6`, and `C-SYS-1` through `C-SYS-6`; and 11
constitution principles with X.1 through X.4 and XI.1 through XI.6 read one by
one. Re-measured here: the 9 headers `include/speedgun-ng/counters*.hpp`
matches, the 11 translation units `find source/counters -name '*.cpp'`
returns beside the 2 headers under `source/counters/detail/` and the 5 under
`source/counters/linux_pmu/`, the 12 `add_executable(counters_` calls
`test/CMakeLists.txt` carries, no line matching `TODO` or `FIXME` over the
feature scope, the single `NOLINT` directive at
`include/speedgun-ng/counters_measurement.hpp:634` carrying its reason in the
same comment block at `:630-633`, and no `std::atomic` in
`source/counters/push_provider.cpp` or in the push handle. Every one of the 44
`file:line` anchors the nine live artifacts place was resolved and read
against the claim its sentence makes, and each lands inside the file it names
on non-blank text. The 19 anchors the code scope places were resolved under
the same pattern: eleven carry the short form `plan.cpp:NNN` inside
`source/counters/plan.cpp` and `source/counters/fold.cpp` and each lands on
the text its comment names, and the remaining one, `spec.md:305` at
`test/source/counters_fake_test.cpp:92`, names a blank line, which is T290
below. The eleven commits at the tip were read in full with `git show`:
`88bbd2c` removes the `target_kind::machine` enumerator and touches one file,
and a search for that name over `source/`, `include/`, `test/`, and `example/`
returns no line; `01f905b`, `30f6361`, `1827d76`, `f1d3023`, `9c5dfa5`,
`b82fb7e`, `9da43ac`, and `8a62b69` touch no executable code; `31363e8` adds
the mixed-owner refusal at `source/counters/push_provider.cpp:120` with the
precondition it states at `include/speedgun-ng/counters_push.hpp:88-90` and
the trap mode that drives it; and `b60b361` adds the accepted fold-lookup cost
at `source/counters/detail/core.hpp:100-114`. The history of every file a
finding cites was read to date the commit that moved it. No closure claim in
the Phase 12 through Phase 39 preambles was accepted as evidence.

Six findings: 3 `contradicts` and 3 `partial`; 1 HIGH, 2 MEDIUM, and 3 LOW.
None is `missing` or `unrequested`, and none is a constitution MUST violation.
One finding names a code shape two artifacts report as present; one names an
acceptance clause the fold does not meet; one names a contract guard with no
covering test; one names a gate-coverage property the record states
incompletely; two name drifted pointers in the record's table and in one
source comment. The code the earlier waves converged is untouched by every one
of them. `T267`'s mixed-owner refusal is landed and its trap exits 134,
`T271`'s accepted fold-lookup cost is recorded at
`source/counters/detail/core.hpp:100-114`, `T272`'s six functions are named and
explained at `specs/007-counters-and-timers/plan.md:459`, `T283`'s
coverage-exclusion figure reads 301 and reproduces, and `T284`'s
whole-repository distribution reproduces figure for figure above.

The four residue items the previous passes reported were assessed on their own
evidence, and three do not survive. The link manifest, a claim that has now
taken four waves, survives in no live site: `specs/007-counters-and-timers/spec.md:31`
and `specs/007-counters-and-timers/quickstart.md:25` each name the measured
three, `specs/007-counters-and-timers/plan.md:33`, `:324`, and `:437`,
`specs/007-counters-and-timers/contracts/measurement-contract.md:126`,
`specs/007-counters-and-timers/contracts/provider-contract.md:56`, and
`.github/workflows/ci.yml:148-152` state the demonstration without a count, and
the only sites still enumerating four libraries are the closed task lines and
dated preambles `specs/007-counters-and-timers/citations.md:376-379` already
names, together with `T279`'s own task text, which is the subject of the
finding that text reports. The red whole-tree gate beside a green range gate
obliges nothing: the constitution's own gate is the range form at
`.specify/memory/constitution.md:245-248`, every one of the 108 findings is
pre-existing and outside the feature scope, and `T284` recorded the state with
the distribution and the exit code, so the obligation `T284` left open is
settled. The functions axis needs no scoring: Constitution VI at
`.specify/memory/constitution.md:190-192` names line, branch, and DBC as its
three hard gates, Principle VIII at `:258-260` makes a gate-set change a
constitution amendment, `tools/dbc/coverage_gate.sh:25-27` and `:35-37` read
the line row and the branch row alone, and the six uncovered entries are the
defaulted `expression` constructor at
`include/speedgun-ng/counters_measurement.hpp:619` and the deleting
destructors the compiler emits for the defaulted virtual destructors at
`include/speedgun-ng/counters_provider.hpp:110`, `:281`, and `:367`, which
`specs/007-counters-and-timers/plan.md:459` names and explains. The fourth
residue item survives, and it is T288 below.

### HIGH: the capability gate `T077` hoisted ahead of the instruction is gone, and two artifacts report it as present

- [X] T285 Record that `T077` at
  `specs/007-counters-and-timers/tasks.md:365` is closed with a requirement the
  code no longer carries, and correct the two rows that repeat the claim. The
  task text reads `Hoist the capability gate ahead of the instruction: test
  user->cap_user_rdpmc in fast_context_read at
  source/counters/linux_pmu/fast_read.cpp before the _rdpmc at :283, so the
  protocol order stated at :40-42 and
  source/counters/detail/pmu.hpp:165-166 actually holds`, and the row at
  `specs/007-counters-and-timers/citations.md:115` gives the landing as
  `:281, the _rdpmc the hoisted capability gate now precedes`. The shipped
  `fast_context_read` reads the capability bit at
  `source/counters/linux_pmu/fast_read.cpp:272`, issues the instruction at
  `:281` under an index test alone at `:278`, and applies the capability gate
  as the first check of `fast_decode`, which is called at `:284` and returns
  `not_allowed` at `:86-88` after the instruction has run.
  `git show 2889608 -- source/counters/linux_pmu/fast_read.cpp` is the commit
  that removed the hoist: its diff deletes, at the `1ff4128` revision, the
  block reading `Capability gate ahead of the instruction (FR-040, R-011):
  the published protocol tests the capability before it takes the read, so a
  caller the kernel grants no read capability never pays for the instruction`
  together with `if ((user->cap_user_rdpmc & 1U) == 0) { return
  fast_read_verdict::not_allowed; }`, and adds the comment at `:262-266` that
  `fast_decode` applies the gates. `FR-040` at
  `specs/007-counters-and-timers/spec.md:271` names capability gating among the
  protocol steps, and the comment at `source/counters/linux_pmu/fast_read.cpp:82-85`
  states the decode order the code does follow, so the two artifacts above are
  the ones that assert the removed shape. The second `T077` row, at
  `specs/007-counters-and-timers/citations.md:116`, gives
  `source/counters/linux_pmu/fast_read.cpp:41-43` and
  `source/counters/detail/pmu.hpp:166-168` as the landing, and those ranges
  hold the `fast_index_valid` comment and the `pmu_device` comment, so that
  landing names no protocol-order text either. Record which of the two
  orderings governs: the decode gate as the settled one, which restates the
  task's requirement to the shape the code has, or the instruction gate, which
  reinstates the hoist. Correct both rows either way, `T077` and every dated
  preamble keep their bytes, and the record's counting rule and every figure it
  carries keep their values (HIGH, FR-040, R-011, T052, T077, T137, T139,
  T253, `contradicts`)

### MEDIUM: US6 scenario 6 requires a scaled value, and the fold discloses a ratio

- [X] T286 Settle the third clause of US6 scenario 6 at
  `specs/007-counters-and-timers/spec.md:150`, which reads `and the value is
  the scaled estimate the kernel computed`, against the shipped fold and
  against the closed task text at
  `specs/007-counters-and-timers/tasks.md:159` that carries the same clause as
  `ratio below 1, scaled set, value is the kernel scaled estimate`. A leaf node
  returns the raw modular delta as `static_cast<double>(delta)` at
  `source/counters/fold.cpp:45-46`, no scaling appears on the path, the
  disclosure the fold returns is the product of the constituent ratios at
  `:170-177`, and `test/source/counters_pmu_test.cpp:644-650` states the
  consequence and calls the fold right, so no assertion anywhere asks for a
  scaled value. `FR-019` at `specs/007-counters-and-timers/spec.md:244` and
  the clarification at `:25` make the disclosure a triple of value, ratio, and
  scaled flag, and neither scales the value; the product form also makes a
  scaled value meaningless for the scenario's own composite, because a sum of
  64 oversubscribed members at a kernel fraction near 0.4 discloses a ratio
  near zero, as this pass measured on this host, where
  `./build/dev/test/counters_pmu_test` printed
  `running_ratio 0.000000 with scaled 1` beside a granted fraction of 0.384213.
  A kernel group read publishes `time_enabled` and `time_running` and leaves
  the scaling to its caller, so no read yields a scaled estimate to report. The
  resolution is a requirement decision, which Principle IX makes binding and
  Principle III takes a DCR for, so this pass records the gap and names the two
  readings: amend the scenario to state that the fold discloses the fraction and
  the flag and leaves scaling to the caller, or scale the value by the
  disclosed ratio, which then contradicts `FR-019`'s product-of-ratios
  disclosure for every composite. No code, gate, threshold, and no requirement
  number or position moves until the decision is recorded (MEDIUM, FR-019,
  FR-041, US6 scenario 6, T043, T134, Constitution III: Design Change Request,
  Constitution IX, `contradicts`)

### MEDIUM: the `FR-035` sampling-side guard `T267` named has no covering trap mode

- [X] T287 Add a trap mode that drives the tier-3 sampling-side violation
  `FR-035` requires, or record at the guard why the construction refusal
  `T267` added leaves the site without a covering test. `T267` at
  `specs/007-counters-and-timers/tasks.md:3024-3058` reported that no test
  reached the guard's violating branch and asked for a trap mode that drives
  the violation; the fix that landed took the task's second branch and added
  the refusal at `source/counters/push_provider.cpp:120` with the trap mode
  `push-mixed-owner` at `test/source/counters_trap_fixture.cpp:120-155`, while
  the guard the report named, the window owner compare at
  `source/counters/push_provider.cpp:40-43`, is the only site that detects a
  single-owner leaf sampled from a thread other than its owner, because
  `sample_point` binds the plan to the compiling thread at
  `source/counters/plan.cpp:61-62` and a leaf set carrying one owner passes the
  refusal. The nine modes the fixture registers are `metric-before-finish`,
  `fold-range`, `fold-out-of-extent`, `push-cross-thread`, `push-decrement`,
  `push-mixed-owner`, `recorder-cross-thread`, `scope-cross-thread`, and
  `overrun`; `push-cross-thread` calls `add` on a foreign thread and trips the
  handle guard at `include/speedgun-ng/counters_measurement.hpp:335`, and
  `recorder-cross-thread` and `scope-cross-thread` trip the plan binding, so
  none reaches `:40`. Reproduced in this session by a program compiled outside
  the repository against `build/dev/libspeedgun-ng.a` and this tree's headers:
  one `push_provider` with a counter declared on the main thread and a second
  declared on a joined worker that adds 10, a leaf set naming the worker's
  counter, a plan compiled and sampled on the main thread, and two `sample()`
  calls; the run aborts with exit 134 on `[precondition] push counters are
  sampled on the thread that created them (FR-035) (predicate:
  std::this_thread::get_id() == owner) at
  /home/archerc/code/speedgun-ng/source/counters/push_provider.cpp:40`, which
  is the violation no mode reaches. The new mode must keep the add a plain
  non-atomic increment and the sample read a plain load, must stay
  semantic-gated, and must leave the plan binding and the mixed-owner refusal
  as they are (MEDIUM, FR-035, FR-046, US4 scenario 2, US7 scenario 5, T267,
  Constitution VI, plan: Test Plan threading row, `partial`)

### MEDIUM: the range form's per-line default is recorded nowhere, and the record names one condition

- [X] T288 Record in `specs/007-counters-and-timers/citations.md` that the
  range form examines a line only when the range's authorship map holds an
  entry for it, so a new line the committed range has no entry for is examined
  by `--mode tree` alone, and correct the sentence that names one condition
  where the code has two. `tools/prose/prose_gate.py:959` reads `status =
  authorship.get(path, {}).get(lineno, "grandfathered")` and `:960-961` skips a
  `grandfathered` line, while `collect_candidates` at `:587` builds that map
  from the `git diff -U1` at `:597` and returns `authorship = None` only in
  tree mode, where `:592` lists the candidates with `git ls-files`, and
  `read_source` at `:842` reads the examined text from disk in both modes, as
  the call at `:936` shows. The record's paragraph at
  `specs/007-counters-and-timers/citations.md:233-243` names `a file predating
  the range's left edge` as what leaves a line unexamined, which covers a
  file outside the candidate set and leaves the per-line default unnamed, and
  the token `grandfathered` appears in no artifact of this feature. A committed
  line the branch's own diff added always has an entry, so the range form's
  green verdict still covers the branch's added prose; the unexamined
  population is an uncommitted insertion inside a ranged file, whose number
  the map holds nothing for, and a pre-existing line of a file the range
  touches. The Phase 34 preamble at
  `specs/007-counters-and-timers/tasks.md:2432-2435` reads `the text it
  examines from the working tree, so an uncommitted line inside a ranged file
  is examined, and no commit reproduces the total`, whose clause overstates for
  the insertion case that paragraph's own account leaves out; that preamble
  keeps its bytes under the Immutability clause of Pull Request Quality, so
  the correction belongs in the record. The record must name both conditions,
  state that a range figure reproduces at a head only when the working tree
  equals that head, and keep the counting rule and every figure it carries
  (MEDIUM, Constitution X.4, Constitution XI.6, T206, T269, T275, T284,
  `contradicts`)

### LOW: four rows of the anchor table name the wrong line in `tasks.md`

- [X] T289 Correct the `Line in tasks.md` column of the four rows at
  `specs/007-counters-and-timers/citations.md:115`, `:116`, `:126`, and
  `:127`, which read `364` for `T077` and `391` for `T101` where those task
  lines stand at `specs/007-counters-and-timers/tasks.md:365` and `:392`, so
  those numbers name `T076` and `T100` and a reader who follows the column
  reaches the adjacent task. The other task-keyed rows of the table carry
  their own line, resolved in this pass under the same read: `T072` 357,
  `T076` 364, `T078` 366, `T079` 367, `T080` 368, `T091` 379, `T129` 423,
  `T145` 493, `T166` 581, `T171` 589, `T195` 1066, `T231` 1828, `T236` 1968,
  `T274` 3308, and the `T255` and `T256` rows name lines inside those task
  bodies at `:2355` and `:2365`, which is the sense the column takes there and
  needs no change. The two `T101` rows' own landings reproduce, at
  `source/counters/system.cpp:163` and `source/counters/clock_provider.cpp:232`,
  and `T101`'s requirement is met, since
  `test/source/counters_clock_push_test.cpp:240` and `:247` read
  `catalog_entry::scaled`; the two `T077` rows' landings are the subject of
  T285 above. No row's landing, no dated preamble, and no closed task line
  moves, and the counting rule and every figure the record carries keep their
  values (LOW, Constitution X.4, T253, T260, T278, T285, `partial`)

### LOW: a live test comment names a blank line for the record `SC-008` derives

- [X] T290 Correct the anchor at `test/source/counters_fake_test.cpp:92`, which
  reads `the shape is derived from the one example in the tree, spec.md:305's
  IPC = 1.31 <- instructions 12.3e9 / cycles 9.4e9, ratio 0.98`, where
  `specs/007-counters-and-timers/spec.md:305` is blank and the `SC-008`
  sentence carrying that example stands at `:315`, and where the record's drift
  criteria at `specs/007-counters-and-timers/citations.md:91-93` cover the
  blank-line case while the record's own population at `:43-50` is the Phase 1
  through Phase 33 record in `tasks.md`, which a source comment sits outside. A
  source comment is live text, so the correction belongs at the comment and
  stays out of the record, and the eleven short-form `plan.cpp:NNN` anchors the
  code scope places inside `source/counters/plan.cpp` and
  `source/counters/fold.cpp` all land on the text their comments name, so this
  is the one anchor in the code scope that fails the second criterion. The
  expected string the helper builds at `:96-113` and the assertion at
  `:480-482` keep their bytes, and the test keeps passing (LOW, Constitution
  IV, SC-008, T084, T253, `partial`)

## Phase 41: Convergence

Appended by `/speckit.converge` after an audit of the branch tip at `4fd1189`
and of the residue the twenty-nine waves before it left. Nothing above this
line changed.

Audit evidence, all produced by this pass from the repository root on Linux,
over a working tree `git status --porcelain` reports clean and `4fd1189`
carries. `cmake --preset=dev` and `cmake --build --preset=dev` exit 0 over a
fully incremental tree whose log reports `Built target` and no compile
action. `ctest --preset=dev` exits 0 with 100.0 percent of 37 tests passed,
0 failed, 0 skipped, in 43.74 s. `ctest --test-dir build -N` exits 0 and
reports `Total Tests: 37`, of which `ctest --test-dir build -N -R counters`
reports 16. `python3 tools/prose/prose_gate.py --check all` and `cmake -P
cmake/prose-lint.cmake` both exit 0 on the same verdict, `prose-lint: 137
sources, 12878 units examined, 0 findings, 1 skipped`, the one skipped source
being `hwloc.md`, which the gate names unreadable. The same command with
`--head 4fd1189` reports the same pair, and this preamble records no figure
for the range holding it: `collect_candidates` at
`tools/prose/prose_gate.py:587` takes the candidate file set and the per-line
authorship filter from the `git diff -U1` at `:597` over the merge base with
`origin/master` and the head, while `read_source` at `:842` reads the examined
text from the working tree in both modes, as the call at `:936` shows, so the
total moves with every edit to any tracked file. `python3
tools/prose/prose_gate.py --check prose --mode tree` exits 1 at `prose-lint:
183 sources, 19582 units examined, 108 findings, 0 skipped`, and every finding
falls outside the feature scope: 89 under `specs/`, 3 in
`docs/pages/dbc-overhead.md`, 3 in `tools/dbc/overhead.cpp`, 2 each in
`test/compile-fail/run.sh`, `test/dbc-gate-fixture/fixture_clean.hpp`,
`test/dbc-gate-fixture/fixture_exempt.hpp` and `test/source/dbc_test.cpp`, and
1 each in `include/speedgun-ng/dbc.hpp`, `include/speedgun-ng/speedgun-ng.hpp`,
`test/source/dbc_trap_checked_test.cpp`, `tools/dbc/asm_smoke.sh` and
`tools/dbc/overhead.sh`. Read narrowed to this feature, `python3
tools/prose/prose_gate.py --check prose --mode tree --paths
specs/007-counters-and-timers docs/pages/counters-overhead.md` exits 0 at
`13 sources, 6782 units examined, 0 findings, 0 skipped`.
`ctest --test-dir build -R prose_gate_fixtures` exits 0 with 1 of 1 in
1.65 s. `cmake --build build/dev -t format-check` exits 0. `cmake --build
build/dev -t dbc-gate` exits 0, reporting `doc-gate: 135 interfaces, 0 gaps`
and `pair-gate: 135 interfaces, 0 gaps`. `cmake -P cmake/spell.cmake` exits
0, and `python3 tools/pmu_events/update_pmu_events.py --check` exits 0. `bash
tools/dbc/coverage_gate.sh build/coverage/coverage.info` exits 0 at
`lines.......: 100.0% (1955 of 1955 lines)`,
`branches....: 100.0% (705 of 705 branches)` and
`functions...: 98.0% (289 of 295 functions)` on an axis the gate does not
score. `cmake --preset=ci-ubuntu` exits 0 with `CMAKE_BUILD_TYPE:STRING=Release`
and `CMAKE_CXX_FLAGS_RELEASE:STRING=-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3 -O3
-DNDEBUG` in the cache, and `cmake --build build`, the release build Principle
IX requires once per feature, exits 0 over a 26-line fully incremental log
holding 0 lines matching the token `warning` and 0 matching `error:`. `readelf
-d` on `build/dev/example/counters_standalone_example` and on
`build/dev/example/counters_giraffe_example` names `libstdc++.so.6`,
`libgcc_s.so.1` and `libc.so.6` in each, with 0 `speedgun-ng` entries, and
`ldd` on the standalone example names those three beside `libm.so.6`,
`linux-vdso.so.1` and the loader, so the two tools name different sets.
`./build/dev/test/counters_pmu_test` exits 0 reporting `pmu catalog: 581
table-selected entries beyond kernel aliases` beside `perf_event_paranoid =
1; hardware event probe granted` and `pmu availability: 358 countable, 0
permission_blocked, 261 not_encodable, 356 fast_rdpmc`.
`./build/dev/test/counters_trap_fixture` exits 134 for
`push-mixed-owner`, `push-foreign-sample`, `push-cross-thread`, `overrun` and
`metric-before-finish`, and the first names the precondition at
`source/counters/push_provider.cpp:120`. A separate `clang-tidy` 22.1.8 run
over the eleven translation units `find source/counters -name '*.cpp'`
returns, with the flags `build/dev/compile_commands.json` records, reports 0
lines carrying `warning:` and 0 carrying `error:`.

Counting rule for every anchor total in this preamble, the rule
`specs/007-counters-and-timers/citations.md:32-41` states, applied with
CPython 3.14.7 `re.finditer` over whole matches `m.group(0)`, the unit
`(?:[\w./-]+/)*[\w.-]+\.[A-Za-z0-9]+:\d+(-\d+)?` and the bare-continuation
unit `(?<![\w./-]):\d+\b`: applied to
`specs/007-counters-and-timers/tasks.md` lines 1 through 2422 the count is 702
occurrences, 467 distinct tokens, and 420 bare continuations; lines 1 through
2578 give 724, 476, and 449; lines 1 through 2905 give 776, 518, and 472;
lines 1 through 3192 give 815, 548, and 485; lines 1 through 3362 give 844,
565, and 501; lines 1 through 3599 give 880, 579, and 540; lines 1 through
3852 give 921, 594, and 557, so the figures the Phase 37, Phase 38, and Phase
39 preambles carry reproduce exactly. The nine live artifacts over their whole
length give 44 occurrences, distributed `spec.md` 28, `plan.md` 7,
`quickstart.md` 4, `research.md` 3, and `contracts/system-contract.md` 2, with
none in `data-model.md`, `sg_counters.md`, `contracts/provider-contract.md`, or
`contracts/measurement-contract.md`.
`specs/007-counters-and-timers/citations.md` over its whole length gives 144
occurrences and 126 distinct tokens, the pair its own section records.

Coverage of the check: 127 requirement keys, counted as 50 functional
requirements numbering `FR-001` through `FR-050` with no gap, 10 success
criteria numbering `SC-001` through `SC-010` with no gap, 49 user-story
acceptance scenarios counted as US1 7, US2 7, US3 6, US4 6, US5 4, US6 7, US7 6,
and US8 6, and 18 edge cases at
`specs/007-counters-and-timers/spec.md:195-212`; 45 design keys, counted as
15 research decisions `R-001` through `R-015`, 11 data-model entities
`E-01` through `E-11`, and 19 contract clauses `C-MEA-1` through `C-MEA-7`,
`C-PRO-1` through `C-PRO-6`, and `C-SYS-1` through `C-SYS-6`; and 11
constitution principles with X.1 through X.4 and XI.1 through XI.6 read one by
one. Re-measured here: the 9 headers `include/speedgun-ng/counters*.hpp`
matches, the 11 translation units `find source/counters -name '*.cpp'`
returns beside the 2 provider-private headers under `source/counters/detail/`
and the 5 `source/counters/linux_pmu/` translation units, the 12
`add_executable(counters_` calls `test/CMakeLists.txt` carries, the
`LCOV_EXCL` count at 301 tokens in the feature scope, no line matching `TODO`
or `FIXME` over the feature scope, and `.github/workflows/ci.yml:159-160`
running the table check in the test job.

Every finding below was re-derived in this session from the files it cites.
The Phase 12 through Phase 40 preambles were read for task identifiers, phase
grouping, and the file paths each task names, and no closure claim in them was
accepted as evidence. The thirteen commits at the tip were read with `git
show`: `88bbd2c` removes the `target_kind::machine` enumerator and touches one
file, and a search for that name over `source/`, `include/`, `test/`, and
`example/` returns no line; `01f905b`, `30f6361`, `f1d3023`, `9c5dfa5`,
`b82fb7e`, `9da43ac`, `8a62b69` and `4fd1189` touch no executable code;
`31363e8` adds the mixed-owner refusal at
`source/counters/push_provider.cpp:120` with the precondition it states at
`include/speedgun-ng/counters_push.hpp:88-90` and the `push-mixed-owner` trap;
`b60b361` adds the accepted fold-lookup cost at
`source/counters/detail/core.hpp:100-114`; and `9aeb595` adds the
`push-foreign-sample` trap mode and its `kModes` row.

The productive class of the last four cycles, the false closure, is not
exhausted. Thirty closed tasks whose text makes a checkable claim were read
against the code, and every one holds. `T003`'s format reach stands at
`cmake/lint.cmake:12-15` with `file(GLOB_RECURSE)` at `:26` covering
`source/counters/**` and `include/speedgun-ng/counters*.hpp`. `T081`'s
per-column check stands at `source/counters/plan.cpp:42` in `sample_row`.
`T085` computes the raw-view ratio at `source/counters/fold.cpp:277` from
`leaf_ratio`. `T100` and `T102` stand as registered gates at
`test/CMakeLists.txt:165-167` and `:174-176`. `T104`'s bracket is recursive at
`CMakeLists.txt:660`. `T108` replaces the eight `operator new` forms at
`test/source/counters_noalloc_test.cpp:120-160` and the delete forms from
`:179`. `T111` recognizes the unit token before dereferencing it at
`include/speedgun-ng/counters_system.hpp:124-126`. `T113` owns the handles
with `std::make_unique` at `source/counters/system.cpp:260`, `:262`, and
`:335`. `T116`'s single sampling point stands at
`source/counters/plan.cpp:54-65`, shared by the scope, the hard-stop
recorder, and the ring. `T118` carries both trivially-copyable assertions at
`include/speedgun-ng/counters_measurement.hpp:559-560`. `T123`'s
regeneration timestamp stands at `docs/pages/counters-overhead.md:13`. `T142`
scales the composite at `include/speedgun-ng/counters_measurement.hpp:141-148`.
`T145` and `T096` stand at `source/counters/system.cpp:308` and `:344`. `T150`
registers the push provider in `test/source/counters_pmu_test.cpp`. `T152`'s
`leaf_sign` returns the exponent at `source/counters/fold.cpp:85-99`. `T166`
and `T175` stand, all three zero-leaf guards reading `leaves.empty()` at
`source/counters/plan.cpp:89`, `:417`, and `:536`. `T234` sizes the group
scratch in the constructor at
`source/counters/linux_pmu/group_io.cpp:191-196`. `T075` reads the fast
window's pair from the leader's page at
`source/counters/linux_pmu/group_io.cpp:341`. `T285`, `T288`, `T289`, and
`T290` stand: `specs/007-counters-and-timers/citations.md:115-116` name the
decode gate, `:524-562` name both conditions that leave a line unexamined,
`:115`, `:116`, `:126`, and `:127` read 365, 365, 392, and 392, and
`test/source/counters_fake_test.cpp:92` names `spec.md:315`.

Four findings: 4 `partial`; 0 CRITICAL, 1 HIGH, 1 MEDIUM, 2 LOW. No finding
is `missing`, `contradicts`, or `unrequested`, and none is a constitution MUST
violation. The four residue items the previous pass reported were assessed on
their own evidence. The claim that Principle VIII's static-analysis gate is
developer-local does not survive: the `test` job at
`.github/workflows/ci.yml:115-118` configures `ci-ubuntu`, which inherits the
`clang-tidy` and `cppcheck` presets at `CMakePresets.json:167` and `:33-45`,
setting `CMAKE_CXX_CLANG_TIDY` and `CMAKE_CXX_CPPCHECK`, which
`build/dev/CMakeFiles/speedgun-ng_speedgun-ng.dir/build.make` passes to
`cmake -E __run_co_compile --tidy= --cppcheck=` on every compile, and the
counters sources, the test sources, and the example sources each go through it.
The eleven jobs are `lint`, `coverage`, `sanitize`, `test`, `test-rocky`,
`shared-audit`, `downstream-consumer`, `consumer-release`, `dbc-gate`,
`prose-lint`, and `docs`. What survives from that item is narrower and is
emitted below. The other three items survive as the first, the third, and the
fourth findings.

### HIGH: the trap checker reports an abort for any non-zero exit, so a mode with no fixture body passes

- [X] T291 Distinguish a contract abort from any other non-zero exit in the
  `expect_abort` branch of `run_mode` at
  `test/source/counters_trap_checked_test.cpp:83-103`, so a mode registered in
  `kModes` at `:52-61` without a fixture body cannot report `counters trap mode
  '<name>' aborted before the marker` and return 0. Today the branch accepts any
  status but 0 at `:84` and any absence of the marker at `:92`, and the
  fixture's unrecognised-mode path prints `fixture: unknown mode` and returns 2
  at `test/source/counters_trap_fixture.cpp:270-274`, so a mode with no body
  satisfies both. Reproduced in this session by running the built
  `./build/dev/test/counters_trap_checked_test` against a fixture written
  outside the repository that recognizes no mode at all: the checker printed
  `aborted before the marker` for all ten modes and exited 0 with
  `counters_trap_checked_test PASS: every misuse mode behaved as the configured
  contract semantic demands`. The check is the evidence for `FR-046`'s tier-3
  list, for `FR-027`'s always-enforced bound, and for the release proof the
  `consumer-release` job runs at `.github/workflows/ci.yml:422-428`, and a mode
  added without a body passes in every checked configuration. Two readings: the
  abort branch must also require the captured output to carry the facility's
  abort diagnostic, which every real abort prints and which `.github/workflows/ci.yml:420`
  already matches on for the `ignore` build, or the fixture must refuse an
  unregistered mode with a status the checker rejects and the mode list must
  come from the fixture. Either must keep the release proof at
  `.github/workflows/ci.yml:400-420` and the survival branch at
  `test/source/counters_trap_checked_test.cpp:105-124` unchanged in what they
  assert, and the red must be observable in a contracts `ignore` tree, which is
  where the mode is read (HIGH, FR-027, FR-035, FR-046, Constitution VI,
  `partial`)

### MEDIUM: the static-analysis gate runs in CI and reports, and a finding does not fail the job

- [X] T292 Settle whether Principle VIII's static-analysis clause requires the
  CI job to fail on a finding, and carry the decision into the configuration or
  the clause. `.specify/memory/constitution.md:240-242` requires that
  `clang-tidy` and `cppcheck` report no new findings, and `:227` reads `Every
  change passes all of the following; each is hard.`. Both tools run in CI:
  `.github/workflows/ci.yml:114` installs them, `:115-116` configures
  `ci-ubuntu`, and `CMakePresets.json:167` inherits the `cppcheck` preset at
  `:33-38` and the `clang-tidy` preset at `:40-45`, setting
  `CMAKE_CXX_CPPCHECK=cppcheck;--inline-suppr` and `CMAKE_CXX_CLANG_TIDY`, which
  `build/dev/CMakeFiles/speedgun-ng_speedgun-ng.dir/build.make` hands to
  `cmake -E __run_co_compile` on every compile. Neither fails the build on a
  finding: `cppcheck` carries no `--error-exitcode`, `.clang-tidy:15` carries
  `WarningsAsErrors: ''`, and a scratch project configured outside the
  repository with the same `CMAKE_CXX_CLANG_TIDY` form and this repository's
  `.clang-tidy` copied in exited 0 with three `warning:` lines from a
  deliberately offending translation unit. The counters sources are clean at
  this head, which a separate `clang-tidy` 22.1.8 run over the eleven
  translation units `find source/counters -name '*.cpp'` returns measured at 0
  `warning:` and 0 `error:` lines, so the report is clean and the clause's
  figure holds. The decision belongs to the clause, which is a governance
  artifact, so the remedy is a spec-level one and not a CI edit: either the
  configuration must make a finding fail the `test` job, which changes the
  pinned pin under Principle I and takes a P2 with its written justification, or
  the clause must state that the gate is a report and name the step a reader
  runs. No analyzer call may be suppressed, no warning class may be demoted in
  `.clang-tidy` or in the presets, and no existing gate may be dropped (MEDIUM,
  Constitution I, P2, Constitution VIII, Constitution IV, Constitution X.1,
  `partial`)

### LOW: two gated trap modes are absent from the hand-maintained CI list

- [X] T293 Add `push-mixed-owner` and `push-foreign-sample` to the mode list
  the `consumer-release` job enumerates by hand, or derive that list from one
  source. The `Run counters_trap_fixture under ignore` step at
  `.github/workflows/ci.yml:359-420` invokes eight modes, at `:364`, `:366`,
  `:368`, `:370`, `:372`, `:374`, `:377`, and `:379`, and asserts their exit
  codes and markers at `:400-420`; the checker's `kModes` at
  `test/source/counters_trap_checked_test.cpp:52-61` carries ten, the two
  absent ones being the modes `T287` and `9aeb595` added. The step asserts
  nothing about them, so a regression in either guard is invisible to it; the
  CTest at `.github/workflows/ci.yml:427-428` runs the checker over the full
  list and covers both, which is why the gap is duplication and not coverage.
  `T287` set the precedent for leaving the list alone, so the record states no
  decision, and a new mode will reproduce the gap. Either branch must keep the
  eight existing assertions and must name, beside the list, that the CTest at
  `:427-428` is the complete check, so a reader does not read the shell step as
  the enumeration of record (LOW, FR-046, FR-027, Constitution IV, T287,
  `partial`)

### LOW: four split code spans the gate's precedence row cannot reach

- [X] T294 Close the four split code spans in
  `specs/007-counters-and-timers/citations.md`, counted with Python over the
  file's lines: an odd backtick count stands at `:254` paired with `:255`, at
  `:258` paired with `:259`, at `:260` paired with `:261`, and at `:337` paired
  with `:340`, eight lines carrying four spans, where each opening backtick
  closes on a later line, so a reader sees literal backticks in the rendered
  page. The blind spot is the one the record itself documents: `T261` records
  that `tools/prose/prose_gate.py:876-883` returns before any rule matcher runs
  on a unit carrying an inline code span, and the record states the class at
  `specs/007-counters-and-timers/citations.md:225-235` and `:233-243`, which
  names the em-dash code point the same precedence row drops and names nothing
  about a span that crosses a line. Every one of the eight lines is
  pre-existing text in a dated record and binds under the Principle XI.1 scope
  paragraph at `.specify/memory/constitution.md:405-409`, so each must be closed
  inside the paragraph that carries it, and no figure the record states may
  move, since the closing of a span changes the byte length of its lines and the
  counting rule at `:32-41` reads occurrences over line ranges (LOW,
  Constitution IV, Constitution XI.1, Constitution XI.6, T261, `partial`)

## Phase 42: Convergence

Appended by `/speckit.converge` after an audit of the branch tip at `9d83823`
and of the residue the thirty waves before it left. Nothing above this line
changed.

Audit evidence, all produced by this pass from the repository root on Linux,
over a working tree `git status --porcelain` reported clean and `9d83823`
carries.

Prose, template, and spelling. `python3 tools/prose/prose_gate.py --check all`
exits 0, and `cmake -P cmake/prose-lint.cmake` exits 0 on the same verdict,
`prose-lint: 141 sources, 13362 units examined, 0 findings, 1 skipped`, the one
skipped source being `hwloc.md`, which the gate names unreadable. `cmake -P
cmake/spell.cmake` exits 0. `python3 tools/prose/prose_gate.py --check prose
--mode tree` exits 1 at `prose-lint: 184 sources, 19910 units examined, 108
findings, 0 skipped`, distributed 89 under `specs/001-dbc-facility/`, 9 in
`test/`, 5 in `tools/dbc/`, 3 in `docs/pages/dbc-overhead.md`, and 2 in
`include/speedgun-ng/`, every one outside the feature scope. The constitution's
own gate is the range form, which is green, so no finding falls on a line the
branch's diff added or modified.

Build and test. `ctest --test-dir build -N` exits 0 and reports `Total Tests:
38`; `build/dev` reports 38. `cmake --preset=dev` exits 0, and
`cmake --build --preset=dev` exits 0 in 0.09 s over a fully incremental tree
whose log reports `Built target` and no compile action. `ctest --preset=dev`
exits 0 with 100.0 percent of 38 tests passed, 0 failed, 0 skipped, in 43.96 s.
`ctest --test-dir build -R prose_gate_fixtures` exits 0 with 1 of 1 in 1.97 s,
and `ctest --test-dir build -R counters_trap` exits 0 with 2 of 2 in 0.69 s.
`cmake --build build/dev -t format-check` exits 0. `cmake --build build/dev -t
dbc-gate` exits 0, reporting `doc-gate: 135 interfaces, 0 gaps` and
`pair-gate: 135 interfaces, 0 gaps`. `python3
tools/pmu_events/update_pmu_events.py --check` exits 0 and prints no line.
`bash tools/dbc/coverage_gate.sh build/coverage/coverage.info` exits 0 over 21
source files at `lines.......: 100.0% (1955 of 1955 lines)`,
`branches....: 100.0% (705 of 705 branches)`, and `functions...: 98.0% (289 of
295 functions)` on an axis the gate does not score. `bash
test/counters_header_purity.sh` exits 0 at `counters_header_purity: clean`, and
`bash test/counters_push_atomic_scan.sh` exits 0 at
`counters_push_atomic_scan: clean`. `readelf -d` on
`build/dev/example/counters_standalone_example` and on
`build/dev/example/counters_giraffe_example` names `libstdc++.so.6`,
`libgcc_s.so.1`, and `libc.so.6` in each, with 0 `speedgun-ng` entries, and
both executables exit 0. `./build/dev/test/counters_trap_fixture
push-mixed-owner` exits 134. `./build/dev/test/counters_pmu_test` exits 0
reporting `pmu catalog: 581 table-selected entries beyond kernel aliases` beside
`perf_event_paranoid = 1; hardware event probe granted` and `pmu availability:
358 countable, 0 permission_blocked, 261 not_encodable, 356 fast_rdpmc`.

Release configuration. `cmake --preset=ci-ubuntu` exits 0 with
`CMAKE_BUILD_TYPE:STRING=Release` and
`CMAKE_CXX_CLANG_TIDY:UNINITIALIZED=clang-tidy;--header-filter=^/home/archerc/code/speedgun-ng/;--exclude-header-filter=^/home/archerc/code/speedgun-ng/external/`
in the cache, and `cmake --build build`, the release build Principle IX
requires once per feature, exits 0 in 0.32 s over a 26-target fully incremental
log carrying 0 compile actions. No compile ran, so this pass states no
static-analysis diagnostic total for the release tree, and that is the fourth
finding.

Static analysis, measured per translation unit. `run-clang-tidy` 22.1.8 over the
eleven translation units `find source/counters -name '*.cpp'` returns, selected
from `build/dev/compile_commands.json`, with the repository `.clang-tidy` and
`-source-filter='.*/source/counters/.*'`, exits 0 and reports 734 lines carrying
`warning:` and 0 carrying `error:`. `clang-tidy` 22.1.8 on
`source/counters/plan.cpp` with the `ci-ubuntu` flags
`build/CMakeFiles/speedgun-ng_speedgun-ng.dir/flags.make` records exits 0 with
92 lines carrying `warning:`; the same invocation under
`--warnings-as-errors='*'` exits 1 with the same 92 carrying `error:`.
`build/dev/compile_commands.json` holds 92 entries. `cppcheck --inline-suppr` on
`source/counters/plan.cpp` exits 0 with findings printed and no
`--error-exitcode`. The 2.10.0 Sync Impact Report at
`.specify/memory/constitution.md:10-14` records the 92, the `--warnings-as-errors`
result, and the 92-entry database, so every figure the report states that this
pass re-measured reproduces. The report's whole-database figures at `:16-20` were
not re-measured here.

Counting rule for every anchor total in this preamble, the rule
`specs/007-counters-and-timers/citations.md:32-41` states, applied with
CPython 3.14.7 `re.finditer` over whole matches `m.group(0)`, the unit
`(?:[\w./-]+/)*[\w.-]+\.[A-Za-z0-9]+:\d+(-\d+)?` and the bare-continuation unit
`(?<![\w./-]):\d+\b`: applied to `specs/007-counters-and-timers/tasks.md` lines
1 through 2422 the count is 702 occurrences, 467 distinct tokens, and 420 bare
continuations; lines 1 through 2578 give 724, 476, and 449; lines 1 through
2905 give 776, 518, and 472; lines 1 through 3192 give 815, 548, and 485; lines
1 through 3362 give 844, 565, and 501; lines 1 through 3599 give 880, 579, and
540; lines 1 through 3852 give 921, 594, and 557, so the figures the Phase 37,
Phase 38, and Phase 39 preambles carry reproduce exactly; lines 1 through 4207
give 979, 624, and 595, and lines 1 through 4479 give 1024, 654, and 639, both
before this section appends its own. The nine live artifacts over their whole
length give 44 occurrences, distributed `spec.md` 28, `plan.md` 7,
`quickstart.md` 4, `research.md` 3, and `contracts/system-contract.md` 2, with
none in `data-model.md`, `sg_counters.md`, `contracts/provider-contract.md`, or
`contracts/measurement-contract.md`. `specs/007-counters-and-timers/citations.md`
over its whole length gives 144 occurrences and 126 distinct tokens, the pair
its own section records.

Coverage of the check: 127 requirement keys, counted as 50 functional
requirements numbering `FR-001` through `FR-050` with no gap, 10 success
criteria numbering `SC-001` through `SC-010` with no gap, 49 user-story
acceptance scenarios counted as US1 7, US2 7, US3 6, US4 6, US5 4, US6 7, US7 6,
and US8 6, and 18 edge cases at `specs/007-counters-and-timers/spec.md:195-212`;
45 design keys, counted as 15 research decisions `R-001` through `R-015` at
`specs/007-counters-and-timers/research.md:7` through `:119`, 11 data-model
entities `E-01` through `E-11` at
`specs/007-counters-and-timers/data-model.md:7` through `:137`, and 19 contract
clauses `C-MEA-1` through `C-MEA-7`, `C-PRO-1` through `C-PRO-6`, and `C-SYS-1`
through `C-SYS-6`; and 11 constitution principles with X.1 through X.4 and
XI.1 through XI.6 read one by one at version 2.10.0. Re-measured here: the 9
headers `include/speedgun-ng/counters*.hpp` matches, the 11 translation units
`find source/counters -name '*.cpp'` returns beside the 2 headers under
`source/counters/detail/`, the 13 `add_executable(counters_` calls
`test/CMakeLists.txt` carries, the `LCOV_EXCL` count at 301 tokens over the
feature scope, 0 lines matching `TODO` or `FIXME` over the feature scope, 0
occurrences of the em-dash code point in every feature-scope file, the single
`NOLINT` directive at `include/speedgun-ng/counters_measurement.hpp:634`, and 0
`std::atomic` in `source/counters/push_provider.cpp`. Every code-scope anchor
the Phase 41 preamble lists was re-read and lands where that preamble says: 26
sites covering `cmake/lint.cmake:12-15` and `:26`,
`source/counters/plan.cpp:42`, `:54-65`, `:89`, `:417`, and `:536`,
`source/counters/fold.cpp:85-99` and `:277`, `test/CMakeLists.txt:165-167` and
`:174-176`, `CMakeLists.txt:660`,
`test/source/counters_noalloc_test.cpp:120-122` and `:179`,
`include/speedgun-ng/counters_system.hpp:124-126`,
`source/counters/system.cpp:260-262`, `:308`, `:335`, and `:344`,
`include/speedgun-ng/counters_measurement.hpp:141-148` and `:559-560`,
`docs/pages/counters-overhead.md:13`, and
`source/counters/linux_pmu/group_io.cpp:191-196` and `:341`. Four functional
requirements were re-read against the shipped code on their own evidence:
`FR-028` enforces the power-of-two ring capacity at
`source/counters/plan.cpp:235` and documents it at
`include/speedgun-ng/counters_measurement.hpp:913`, `FR-022` ships the thunk
seam with its documented fallback at
`include/speedgun-ng/counters_provider.hpp:244-251`, `:268`, `:281`, `:307-321`,
and `:324`, `FR-045` runs the table check in the `test` job at
`.github/workflows/ci.yml:160`, and `FR-049` is met by the `readelf -d` result
above. No closure claim in the Phase 12 through Phase 41 preambles was accepted
as evidence.

The sixteen commits at the tip were read with `git show`. `7920853` and
`9d83823` are the two most recent code-bearing commits: `7920853` adds the
report discriminator at `test/source/counters_trap_checked_test.cpp:56` and
`:106-115`, the negative-control fixture
`test/source/counters_trap_noguard_fixture.cpp`, its CTest, and the two CI trap
modes at `.github/workflows/ci.yml:384-387` and `:421-422`, and `9d83823`
reflows four code spans in the record and adds Phase 41. `31363e8` carries the
mixed-owner refusal at `source/counters/push_provider.cpp:120`, `b60b361`
carries the accepted fold-lookup cost at
`source/counters/detail/core.hpp:100-114`, `88bbd2c` removes
`target_kind::machine` and touches one file, `9aeb595` adds the
`push-foreign-sample` mode, `cb5dee5` is the 2.10.0 amendment, and `01f905b`,
`30f6361`, `1827d76`, `f1d3023`, `9c5dfa5`, `b82fb7e`, and `8a62b69` touch no
executable code.

Seven findings: 2 `contradicts` and 5 `partial`; 0 CRITICAL, 2 HIGH, 3 MEDIUM, 2
LOW. No finding is `missing` or `unrequested`, and no constitution MUST
statement is violated, which is why no CRITICAL finding is emitted: the
constitution's MUST statements in force here are IX's artifact and
release-build clauses, V's `format-check`, VIII's build, test, sanitizer,
static-analysis, format, spell, prose, and coverage gates, II's release-artifact
verification, and I's C++23 rule, and each one the pass could run reported its
verdict. The `ci-sanitize` preset and the `consumer-release` job were not run in
this pass, so no verdict is claimed for them.

The productive class, the false closure, produced the first finding. The
library itself is clean: every code anchor re-read lands, the `FR-028`,
`FR-022`, `FR-045`, and `FR-049` checks hold against the shipped code, the
`T291` and `T293` fixes are landed with the negative control at
`test/source/counters_trap_checked_test.cpp:94-115`, and no code commit on this
branch since `31363e8` touches the measurement path.

The four residue items the previous passes reported were assessed on their own
evidence. The duplicated trap-mode lists no longer reproduce: `T293` took the
add branch, so `.github/workflows/ci.yml:369-389` now enumerates all ten modes
that `kModes` at `test/source/counters_trap_checked_test.cpp:64-73` carries,
asserts all ten at `:414-432`, and carries at `:360-363` the comment naming the
CTest as the enumeration of record. The two lists agree mode for mode, so the
duplication is documented and mitigated, and no finding follows. The
split-code-span class is folded into the third finding below, which names the
same precedence row and states the exemption's scope. The `quick_enforce` hole
is the fifth finding. The missing release-configuration diagnostic count is the
seventh.

### HIGH: the recorded static-analysis figure for the feature scope is 0, and the tool reports 734

- [X] T295 Record the measured static-analysis figure for the feature scope in
  `specs/007-counters-and-timers/citations.md`, and correct the two closed
  texts that state 0. `specs/007-counters-and-timers/tasks.md:4270-4273`, the
  Phase 41 preamble, reads `A separate clang-tidy 22.1.8 run over the eleven
  translation units find source/counters -name '*.cpp' returns, with the flags
  build/dev/compile_commands.json records, reports 0 lines carrying warning: and
  0 carrying error:`, and `specs/007-counters-and-timers/tasks.md:4426-4429`,
  `T292`'s own text, carries the same figure and rests a conclusion on it: `so
  the report is clean and the clause's figure holds`. Measured in this pass,
  `run-clang-tidy` 22.1.8 over the eleven translation units selected from
  `build/dev/compile_commands.json` with the repository `.clang-tidy` exits 0 and
  reports 734 lines carrying `warning:` and 0 carrying `error:`, and
  `clang-tidy` 22.1.8 on `source/counters/plan.cpp` with the `ci-ubuntu` flags
  `build/CMakeFiles/speedgun-ng_speedgun-ng.dir/flags.make` records exits 0 with
  92 lines carrying `warning:`, the same 92 under `--warnings-as-errors='*'`
  carrying `error:` and exiting 1. The flag set is the discriminator and it is
  not one: the dev preset's flags give the same 92 on the same file. The
  2.10.0 Sync Impact Report the same finding produced states the 92 at
  `.specify/memory/constitution.md:11-13`, so the branch contradicts its own
  recorded evidence, and the two sibling commit bodies `7920853` and `9d83823`
  each report lines carrying a clang-tidy finding, which agrees with 734 and
  with 92. Two readings: the record carries the measured figure beside the two
  closed texts, which leaves the closure of `T292` standing on an evidence claim
  the tool denies, or the record states that the 0 was a measurement error and
  that the figure the clause needs is a baseline of pre-existing findings, which
  the clause at `.specify/memory/constitution.md:269-274` names as `no new
  findings` without defining a baseline to measure new against. Either way the
  record must state the measured figure, the head it was read at, the two
  invocations that produce it, and the distinction between a finding count and
  a new-finding count; the two closed texts and every dated preamble keep their
  bytes, the clause is a governance artifact the repository owner may overrule,
  and no analyzer call is suppressed and no warning class demoted (HIGH,
  Constitution VIII, Constitution I, Constitution X.4, T291, T292, `contradicts`)

### HIGH: every constitution anchor is stale after 2.10.0, and the record carries no row

- [X] T296 Add a re-anchor table for the constitution to
  `specs/007-counters-and-timers/citations.md`, carrying for each anchor the
  feature places the number written, the line holding the same text now, the
  head the number was read at, and the shift that carries it. Measured in this
  pass with the rule at
  `specs/007-counters-and-timers/citations.md:32-41`, the constitution holds
  645 lines and the feature places 30 anchors into it, 3 in the record and 27 in
  closed task lines and dated preambles, at
  `specs/007-counters-and-timers/tasks.md:358`, `:397`, `:751`, `:861`, `:962`,
  `:976`, `:980`, `:988`, `:1135`, `:1136`, `:1209`, `:2734`, `:2752`, `:2815`,
  `:2997`, `:3147`, `:3174`, `:3295`, `:3724`, `:3846`, `:4009`, `:4013`,
  `:4014`, `:4413`, `:4414`, and `:4475`, and in the record at
  `specs/007-counters-and-timers/citations.md:156`, `:222`, and `:455`. Every
  one of the 30 is stale, every one was correct at `4fd1189`, the head the 2.9.1
  report held, and the shift is 29 lines for an anchor below line 238 of that
  file and 32 lines above it, which `git diff 4fd1189 cb5dee5` gives as three
  hunks at +29, +32, and +33. No anchor was ever wrong, so the table
  distinguishes an anchor a shift moved from one that was never right, and this
  pass measured the second class as empty. The three in the record are the
  actionable ones, and the third is the one a reader follows:
  `specs/007-counters-and-timers/citations.md:455` names
  `.specify/memory/constitution.md:245-248` as the constitution's own prose-lint
  gate and the range form, a claim `T284` and the Phase 40 preamble both lean
  on, and `:245-248` now holds Principle VII's per-platform baselines bullet
  while the gate stands at `:277-280`; `specs/007-counters-and-timers/citations.md:222`
  names `:405-409` for the Principle XI.1 scope paragraph, now `:437-441`; and
  `specs/007-counters-and-timers/citations.md:156` names `:568-570` for the
  machine-local `CMakeUserPresets.json` sentence, now `:600-602`. The 27 in
  closed task lines and dated preambles keep their bytes under the Immutability
  clause of Pull Request Quality, and `T292`'s two anchors at
  `specs/007-counters-and-timers/tasks.md:4413` and `:4414` and `T294`'s one at
  `:4475` are among them, so the record is the only vehicle. The table states
  the shift map so a later amendment is re-derivable arithmetically, names the
  three live rows first, keeps the counting rule at `:32-41` and every figure
  the record carries, and appends at the end of the file so the gate's
  authorship map, which is keyed by line number against `HEAD`, loses nothing
  (HIGH, Constitution IV, Constitution X.4, T253, T260, T263, T276, T284,
  T288, T292, T294, `partial`)

### MEDIUM: a dated preamble gives the tree form the range form's candidate set

- [X] T297 Record in `specs/007-counters-and-timers/citations.md` that the
  sentence at `specs/007-counters-and-timers/tasks.md:3864-3868` is false, and
  that the Phase 40 preamble's use of both forms beside it depends on the false
  half. The sentence reads `Both forms take their candidate file set and their
  per-line authorship filter from the range collect_candidates at
  tools/prose/prose_gate.py:587 runs as git diff -U1 at :597 over the merge base
  with origin/master and the head, defaulting to HEAD`. The code takes the
  candidate set from the range in the range mode alone:
  `tools/prose/prose_gate.py:591-593` returns the `git ls-files` listing and
  `authorship = None` under `if mode == "tree"`, the range path is `:594-600`,
  and `:956-957` sets every line's status to `new` whenever the map is `None`, so
  the tree form holds no per-line filter at all. `read_source` at `:842` does
  read the examined text from the working tree in both modes, as the call at
  `:936` shows, so the sentence's second half holds and its first half does
  not. The same preamble reports a tree-form figure at `:3875-3885` and a
  tree-form run narrowed to this feature, so a reader applying its first half
  concludes the tree form is range-limited, which would drop every file the
  range does not touch. The Phase 41 sentence at
  `specs/007-counters-and-timers/tasks.md:4226-4231` states the same mechanism
  without the `Both forms` quantifier and is scoped to the range form it
  declines to give a figure for, so it stands; the record must say which of the
  two sentences is wrong so a reader does not carry the first into the next
  pass. The record already states the correct behaviour at
  `specs/007-counters-and-timers/citations.md:526-536`, so this adds the row
  naming the preamble and nothing else; the preamble keeps its bytes, the
  counting rule and every figure the record carries keep their values, and no
  gate rule or threshold moves (MEDIUM, Constitution X.4, Constitution XI.6,
  T206, T269, T284, T288, `contradicts`)

### MEDIUM: the code-span exemption is unit-wide, and three Principle XI violations in this feature's live artifacts ride it

- [X] T298 Close the three Principle XI violations the prose gate's precedence
  row cannot reach, and record the row's scope beside the blind-spot section.
  `tools/prose/prose_gate.py:876-883` returns an empty finding list for a unit
  when `INLINE_CODE_RE.search(unit_text)` matches anywhere in it, and the same
  for `URL_RE`, `path_like`, `SHELL_COMMAND_RE`, and `BLOCKQUOTE_RE` at
  `:877-882`, so a banned token outside the code span is dropped whenever its
  unit carries one. `.specify/memory/constitution.md:504-506` scopes the
  exemption to `a banned token inside a verbatim quotation, code span, command,
  file name, or a literal that is itself the subject under discussion`, which
  is a token, and the row is a unit. Three violations in this feature's live
  artifacts sit in that gap, all verified here: `specs/007-counters-and-timers/plan.md:393`
  reads `The probe, not the kernel, was the obstacle` beside two spans on the
  same line, an XI.2 `, not `; `specs/007-counters-and-timers/quickstart.md:140`
  reads `the binary check is reported, not asserted` on a table row carrying
  many spans, an XI.2 `, not `; and
  `specs/007-counters-and-timers/research.md:57` reads `Platforms without a
  usable TSC simply omit the leaf` on a line of spans, an XI.5 `simply`. A
  word-bounded scan with Python over `git ls-files` finds 211 such lines
  repository-wide, 0 em-dashes in any feature-scope file, and 3 of the 211 in
  this feature's live artifacts. The record's blind-spot section at
  `specs/007-counters-and-timers/citations.md:203-243` names the em-dash family
  and the split-span class and names neither the unit-wide scope nor these
  three, so the record addition states the scope, and the three sentences are
  reworded in place to state what the thing is, each fact in its own sentence.
  A narrower gate row belongs to `specs/002-prose-commit-lint` and to a
  constitutional reading of the exemption, and no gate rule, threshold,
  vocabulary, or marker moves here; the three closed task lines that also carry
  a dropped token, `specs/007-counters-and-timers/tasks.md:390`, `:487`, `:497`,
  `:1633`, and `:2723`, keep their bytes, and whether they are swept belongs to
  the tree-wide sweep Principle XI.1 schedules (MEDIUM, Constitution XI.2,
  Constitution XI.5, Constitution XI.6, Constitution IV, T261, T294,
  `contradicts`)

### MEDIUM: the `T291` discrimination is compiled out under `quick_enforce`

- [X] T299 Close the hole `T291` left in the fourth contract semantic, or
  record at the guard why one semantic cannot exercise the check.
  `test/source/counters_trap_checked_test.cpp:56` reads `constexpr bool
  kViolationIsReported = SG_CONTRACTS_SEMANTIC != 3;`, and `:106` gates the
  report test on it, so under `quick_enforce` the `expect_abort` branch keeps
  only `status == 0` at `:98` and `!printed_marker` at `:116`. The fixture's
  unrecognised-mode path prints its own refusal and returns 2 at
  `test/source/counters_trap_fixture.cpp:270-274`, so an entry in `kModes` with
  no fixture body satisfies both and returns 0: the exact false PASS `T291`
  closed, restored for one semantic. The source says so at
  `test/source/counters_trap_checked_test.cpp:54-55` and the commit body of
  `7920853` states it, so the carve-out is known and unrecorded, and the
  negative control that covers the discrimination,
  `counters_trap_checked_rejects_unknown_mode`, runs in the dev preset at
  `SG_CONTRACTS_SEMANTIC == 2`, where `kViolationIsReported` is true. No preset,
  workflow, or test in the tree builds `quick_enforce`: the token appears in
  `cmake/dbc.cmake:13`, `:26`, `:30`, `:39`, and `:46` and nowhere else, so no
  configuration the matrix drives carries the hole today and the exposure is a
  future mode added without a body in a `quick_enforce` tree. Two readings: the
  second reading `T291` named, where the fixture refuses an unregistered mode
  with a status the checker rejects and the mode list comes from the fixture,
  which discriminates in every semantic, or the carve-out stands and the record
  states it with the reason and the configuration that has no coverage. Either
  must keep the `ignore` and `observe` branches and the release proof at
  `.github/workflows/ci.yml:364-432` unchanged in what they assert, and the
  mode count and the `RESOURCE_LOCK` stay as they are (MEDIUM, FR-027, FR-035,
  FR-046, Constitution VI, T267, T287, T291, T293, `partial`)

### LOW: the executable count the last three preambles carry moved at `7920853`

- [X] T300 Record in `specs/007-counters-and-timers/citations.md` that the
  `add_executable(counters_` count of 12 the Phase 39 preamble at
  `specs/007-counters-and-timers/tasks.md:3684`, the Phase 40 preamble at
  `:3957`, and the Phase 41 preamble at `:4307-4308` carry is 13 at this head,
  and name the commit that moved it. `test/CMakeLists.txt` carries 13, the
  thirteenth being `counters_trap_noguard_fixture` at `:203-206`, which
  `7920853` added. Each preamble's figure was correct at the head it was written
  at, so the three keep their bytes under the Immutability clause of Pull
  Request Quality and the correction belongs in the record, which carries no row
  for it. The row names the three sites, the figure as written, the figure
  measured, and `7920853`, states that the movement is a test-side addition and
  changes no library code, and keeps the counting rule and every figure the
  record carries (LOW, Constitution X.4, T291, T292, T293, `partial`)

### LOW: no release-configuration static-analysis total is recorded anywhere

- [X] T301 Record a static-analysis diagnostic total for the release
  configuration, naming the head and the command that produces it, so a later
  pass states a figure and the obligation closes. Principle IX
  requires one release-configuration build per feature and
  `.specify/memory/constitution.md:269-274` now states that the static-analysis
  gate reports, so a count for the configuration the gate drives is the
  measurement the clause's own wording asks a reader to find. This pass's
  `cmake --build build` was a no-op, 26 targets and 0 compile actions, so it
  states no total, as the Phase 39, Phase 40, and Phase 41 preambles each state
  none. The only recorded totals sit in the 2.10.0 Sync Impact Report at
  `.specify/memory/constitution.md:10-20`: 92 for one translation unit, 28821
  over the 92 entries of `build/dev/compile_commands.json`, and 23700 of those
  from `external/`. This pass re-measured the 92 and the 92 entries and
  reproduced both, and did not re-measure the other two. The task is a
  from-clean `ci-ubuntu` build whose log is read for the two tokens, recorded
  with the head it ran at, or a citation to the report's figures with the head
  they were read at; no analyzer call is suppressed, no warning class is
  demoted, and no gate moves (LOW, Constitution VIII, Constitution IX,

## Phase 43: Convergence

Appended by `/speckit.converge` after an audit of the branch tip at `14b8e48`
and of the residue the thirty-one waves before it left. Nothing above this
line changed.

Audit evidence, all produced by this pass from the repository root on Linux,
over a working tree `git status --porcelain` reports clean and `14b8e48`
carries.

Prose, template, and spelling. `python3 tools/prose/prose_gate.py --check all`
exits 0, and `cmake -P cmake/prose-lint.cmake` exits 0 on the same verdict,
`prose-lint: 143 sources, 14237 units examined, 0 findings, 1 skipped`, the
one skipped source being `hwloc.md`, which the gate names unreadable. `cmake -P`
`cmake/spell.cmake` exits 0. In tree mode the gate exits 1 at `prose-lint: 184`
`sources, 20655 units examined, 108 findings, 0 skipped`, spread over 19
files: 89 under `specs/001-dbc-facility/`, 3 in `tools/dbc/overhead.cpp`, 3 in
`docs/pages/dbc-overhead.md`, 2 each in `test/source/dbc_test.cpp`,
`test/dbc-gate-fixture/fixture_exempt.hpp`,
`test/dbc-gate-fixture/fixture_clean.hpp`, and
`test/compile-fail/run.sh`, and 1 each in
`specs/001-dbc-facility/quickstart.md`,
`specs/001-dbc-facility/contracts/api-contracts.md`,
`tools/dbc/overhead.sh`, `tools/dbc/asm_smoke.sh`,
`test/source/dbc_trap_checked_test.cpp`,
`include/speedgun-ng/speedgun-ng.hpp`, and `include/speedgun-ng/dbc.hpp`. The
constitution's own gate is the range form, which is green, and
`git diff 65beada HEAD -- test/compile-fail/run.sh` is empty, so the two
findings on that file are pre-existing and sit outside the branch's diff. Read
narrowed to `specs/007-counters-and-timers` and
`docs/pages/counters-overhead.md` the same tree-mode run exits 0 at
`13 sources, 7779 units examined, 0 findings, 0 skipped`.

Build and test. `ctest --test-dir build -N` exits 0 and reports `Total Tests:`
`38`, of which 17 are counters tests. `cmake --preset=dev` exits 0, and
`cmake --build --preset=dev` exits 0 over a fully incremental 27-line log
carrying 27 `Built target` lines and 0 lines matching `Building` or `Linking`.
`ctest --preset=dev` exits 0 with 100.0 percent of 38 tests passed, 0 failed,
0 skipped, in 44.06 s. Read narrowed to `prose_gate_fixtures` the same form
exits 0 with 1 of 1 in 1.62 s, read narrowed to `counters_trap` it exits 0
with 2 of 2 in 0.66 s, and read narrowed to
`counters_trap_checked_rejects_unknown_mode` with the verbose flag it exits 0
with the checker printing the negative control's diagnostic, naming the
`metric-before-finish` mode and the absence of the contract facility's
violation report, which is the control `7920853` added.
`cmake --build build/dev -t format-check` exits 0, and
`cmake --build build/dev -t dbc-gate` exits 0, reporting
`doc-gate: 135 interfaces, 0 gaps` beside
`pair-gate: 135 interfaces, 0 gaps`. The table check
`python3 tools/pmu_events/update_pmu_events.py --check` exits 0 and prints no
line. The coverage gate
`bash tools/dbc/coverage_gate.sh build/coverage/coverage.info` exits 0 over 21
source files at `lines.......: 100.0% (1955 of 1955 lines)`,
`branches....: 100.0% (705 of 705 branches)`, and
`functions...: 98.0% (289 of 295 functions)` on the axis the gate does not
score. `bash test/counters_header_purity.sh .` exits 0 at
`counters_header_purity: clean`, and `bash test/counters_push_atomic_scan.sh`
`with the repository root as its argument exits 0 at`
`counters_push_atomic_scan: clean`. `readelf -d` on
`build/dev/example/counters_standalone_example` and on
`build/dev/example/counters_giraffe_example` names `libstdc++.so.6`,
`libgcc_s.so.1`, and `libc.so.6` in each, with 0 `speedgun-ng` entries, and
both executables exit 0.

Release configuration. `cmake --preset=ci-ubuntu` exits 0 with
`CMAKE_BUILD_TYPE:STRING=Release` in the cache, beside
`CMAKE_CXX_CLANG_TIDY:UNINITIALIZED=clang-tidy;--header-filter=^/home/archerc/code/speedgun-ng/;--exclude-header-filter=^/home/archerc/code/speedgun-ng/external/`
and `CMAKE_CXX_CPPCHECK:UNINITIALIZED=cppcheck;--inline-suppr`, and
`cmake --build build`, the release build Principle IX requires once per
feature, exits 0 over a fully incremental 27-line log carrying 0 lines
matching `warning` and 0 matching `error:`. No compile ran, so this pass
states no release-configuration diagnostic total, as the Phase 39, Phase 40,
Phase 41, and Phase 42 preambles each state none.

Static analysis, measured per translation unit. `run-clang-tidy` 22.1.8 over
the eleven translation units `find source/counters -name '*.cpp'` returns,
selected from `build/dev/compile_commands.json` with the repository
`.clang-tidy` and the source filter the file names, exits 0 and reports 734
lines carrying `warning:` and 0 carrying `error:`. The per-file decomposition
reproduces `specs/007-counters-and-timers/citations.md:598-622` figure for
figure, and 3 of the 734 name
`external/simdjson/include/simdjson/dom/document-inl.h` at 2 and
`external/simdjson/include/simdjson/padded_string-inl.h` at 1, so 731 name the
feature's own files, and the record's sentence at `citations.md:595-596` names
that split. The exclusion holds under measurement: the exclude header filter
in the form the `ci-ubuntu` cache records, in the shorter form naming
`external` alone, and absent altogether each leave all three, and the analyzer
checks alone report the same three, so the record answers the question the
previous pass left open. `cppcheck --inline-suppr -q --force` over the eleven
translation units exits 0 and prints no finding, and the same launcher form on
`source/counters/plan.cpp` alone exits 0 and prints nothing, which is what
`citations.md:968-976` records against the Phase 42 preamble and this pass
reproduces.

Counting rule for every anchor total in this preamble, the rule
`specs/007-counters-and-timers/citations.md:32-41` states, applied with
CPython 3.14.7 `re.finditer` over whole matches `m.group(0)`, the unit
`(?:[\w./-]+/)*[\w.-]+\.[A-Za-z0-9]+:\d+(-\d+)?` and the bare-continuation unit
`(?<![\w./-]):\d+\b`: applied to `specs/007-counters-and-timers/tasks.md` lines
1 through 2422 the count is 702 occurrences, 467 distinct tokens, and 420 bare
continuations; lines 1 through 2578 give 724, 476, and 449; 1 through 2905 give
776, 518, and 472; 1 through 3192 give 815, 548, and 485; 1 through 3362 give
844, 565, and 501; 1 through 3599 give 880, 579, and 540; 1 through 3852 give
921, 594, and 557, so the figures the Phase 37, Phase 38, and Phase 39
preambles carry reproduce exactly; 1 through 4207 give 979, 624, and 595, and
1 through 4479 give 1024, 654, and 639. The nine live artifacts over their
whole length give 44 occurrences, distributed `spec.md` 28, `plan.md` 7,
`quickstart.md` 4, `research.md` 3, and `contracts/system-contract.md` 2, with
none in `data-model.md`, `sg_counters.md`,
`contracts/provider-contract.md`, or `contracts/measurement-contract.md`.
`citations.md` over its whole length gives 241 occurrences, 202 distinct
tokens, and 345 bare continuations, where the Phase 42 preamble carries 144
and 126 for the head it was read at. The 97 the last implement pass added are
the re-anchor table and the sections beside it, and every one is a citation
the next sweep resolves.

Constitution anchors. The feature places 45 path-form anchors into
`.specify/memory/constitution.md` over `tasks.md` lines 1 through 4479 in
either the full or the short form, on 38 distinct lines, beside the 3 stale
anchors in the record and the 11 stale bare continuations, which is 59 stale
and 6 historical, 65 in all, as `citations.md:698-706` states. The re-anchor
table's 59 rows were resolved line for line against the constitution at
`4fd1189` and the working tree: 58 hold the same text at both ends with the
shift the row's column states, and one does not, which is the fourth finding
below.

Coverage of the check: 127 requirement keys, counted as 50 functional
requirements numbering `FR-001` through `FR-050` with no gap, 10 success
criteria numbering `SC-001` through `SC-010` with no gap, 49 user-story
acceptance scenarios counted as US1 7, US2 7, US3 6, US4 6, US5 4, US6 7, US7 6,
and US8 6, and 18 edge cases at `specs/007-counters-and-timers/spec.md:195-212`;
45 design keys, counted as 15 research decisions `R-001` through `R-015` at
`specs/007-counters-and-timers/research.md:7` through `:119`, 11 data-model
entities `E-01` through `E-11` at
`specs/007-counters-and-timers/data-model.md:7` through `:137`, and 19 contract
clauses `C-MEA-1` through `C-MEA-7`, `C-PRO-1` through `C-PRO-6`, and
`C-SYS-1` through `C-SYS-6`; and 11 constitution principles with X.1 through
X.4 and XI.1 through XI.6 read one by one at version 2.10.0. Re-measured here:
the 9 headers `include/speedgun-ng/counters*.hpp` matches, the 11 translation
units `find source/counters -name '*.cpp'` returns beside the 2 headers under
`source/counters/detail/` and the 5 under `source/counters/linux_pmu/`, the 13
`add_executable(counters_` calls `test/CMakeLists.txt` carries, the
`LCOV_EXCL` count at 301 tokens over the feature scope, 0 lines matching
`TODO` or `FIXME` over that scope, 0 occurrences of the em-dash code point
over the code scope and 65 over the feature's artifacts, the single `NOLINT`
directive at `include/speedgun-ng/counters_measurement.hpp:634` with its
reason at `:630-633`, 0 `std::atomic` in
`source/counters/push_provider.cpp`, and no `check(true, ...)` in the counters
tests. Four functional requirements were re-read against the shipped code on
their own evidence: `FR-028` enforces the power-of-two ring capacity at
`source/counters/plan.cpp:235` and documents it at
`include/speedgun-ng/counters_measurement.hpp:913`, `FR-022` ships the thunk
seam with its documented fallback at
`include/speedgun-ng/counters_provider.hpp:244-251`, `FR-045` runs the table
check in the `test` job, and `FR-049` is met by the `readelf -d` result
above. No closure claim in the Phase 12 through Phase 42 preambles was
accepted as evidence.

The eighteen commits at the tip were read with `git show`. `7920853` adds the
report discriminator at
`test/source/counters_trap_checked_test.cpp:56` and `:106-115` with the
negative control at `test/source/counters_trap_noguard_fixture.cpp`,
`9aeb595` adds the `push-foreign-sample` mode, `31363e8` carries the
mixed-owner refusal at `source/counters/push_provider.cpp:120`, `b60b361`
carries the accepted fold-lookup cost at
`source/counters/detail/core.hpp:100-114`, `08b3dfc` and `cb5dee5` carry the
`T299` closure and the 2.10.0 amendment, and `88bbd2c`, `01f905b`, `30f6361`,
`1827d76`, `f1d3023`, `9c5dfa5`, `b82fb7e`, `4fd1189`, `8a62b69`, `9d83823`,
and `14b8e48` touch no measurement-path code.

Four findings: 1 `contradicts` and 3 `partial`; 1 HIGH, 1 MEDIUM, 2 LOW. No
finding is `missing` or `unrequested`, and no constitution MUST statement is
violated, which is why no CRITICAL finding is emitted: the MUST statements in
force here are I's C++23 rule, II's release-artifact verification, V's
`format-check`, VI's three coverage gates, VIII's build, test, static-analysis,
format, spell, prose, and coverage gates, and IX's artifact and release-build
clauses, and each one this pass could run reported its verdict. The
`ci-sanitize` preset and the `consumer-release` job were not run, so no verdict
is claimed for them.

The library is clean. Every code anchor this pass re-read lands, the `T267`
mixed-owner refusal stands at `source/counters/push_provider.cpp:120` with its
per-window owner at `:36` and `:123` and its per-cell read at `:44-46`, the
`T291` and `T299` discriminations are landed with the negative control, and
no code commit on this branch since `31363e8` touches the measurement path.

The four residue items were assessed on their own evidence. The analyzer scope
filter question is answered and reproduced: the record's per-file table at
`citations.md:598-622` decomposes 734 into 731 inside the feature and 3 in two
vendored inline headers, and the exclusion holds under every filter form
measured, so no task follows. The clause's `no new findings` figure names no
baseline, and `citations.md:660-676` states the gap and leaves the remedy to
the repository owner under Principle IX, so the decision stays open and no task
closes it. The Phase 42 preamble's `cppcheck` claim is already named at
`citations.md:970-975` and the citation lands. The count and em-dash claims
are the first and third findings below, and the three live-artifact violations
the record names are the second.

### HIGH: the record's exemption-scope total does not reproduce

- [X] T302 Correct the measured total the code-span exemption's blind-spot
  section carries, and the per-file breakdown beside it.
  `specs/007-counters-and-timers/citations.md:863-876` states its method in
  full and reports 129 lines repository-wide that carry a token the row
  drops, naming 68 in `tasks.md`, 13 in `sg_counters.md`, 10 in
  `specs/001-dbc-facility/research.md`, 7 in
  `specs/001-dbc-facility/spec.md`, 6 in
  `specs/001-dbc-facility/plan.md`, 4 in `CMakeLists.txt`, 3 in
  `specs/001-dbc-facility/tasks.md`, 2 in
  `specs/007-counters-and-timers/plan.md`, and 2 in
  `tools/dbc/asm_smoke.sh`, and states the total corrects the 211 the `T298`
  task text carries. Measured in this pass with the gate's own source
  classification, the gate's own unit extraction, the five triggers
  `tools/prose/prose_gate.py:876-883` names, the code spans, URLs, and path
  tokens masked before the token search, and the gate's full rule set, the
  dropped population is 279 lines over 25 files: 68 in `tasks.md`, 61 in
  `sg_counters.md`, 43 in `specs/001-dbc-facility/plan.md`, 33 in
  `specs/001-dbc-facility/tasks.md`, 33 in
  `specs/001-dbc-facility/research.md`, 12 in
  `specs/001-dbc-facility/spec.md`, 5 in
  `.specify/scripts/bash/common.sh`, 4 in
  `specs/001-dbc-facility/contracts/api-contracts.md`, 3 in
  `specs/001-dbc-facility/data-model.md`, 2 in
  `specs/007-counters-and-timers/plan.md`, and 1 each in
  `include/speedgun-ng/dbc.hpp`,
  `specs/001-dbc-facility/quickstart.md`,
  `specs/002-prose-commit-lint/contracts/ci-job.md`,
  `specs/002-prose-commit-lint/contracts/rule-data.md`,
  `specs/004-vendor-simdjson/tasks.md`,
  `specs/005-vendor-hdrhistogram/tasks.md`,
  `specs/007-counters-and-timers/spec.md`, `test/CMakeLists.txt`,
  `test/compile-fail/run.sh`,
  `test/source/counters_trap_checked_test.cpp`,
  `test/source/dbc_test.cpp`, `tools/dbc/asm_smoke.sh`,
  `tools/dbc/coverage_gate.sh`, `tools/dbc/dependency_scan.sh`, and
  `tools/dbc/overhead.cpp`. Two of the nine named figures reproduce, at 68 and
  at 2, and seven do not. The paragraph is arithmetically inconsistent as
  written, naming 115 of its 129 and spreading the remaining 14 over 21
  further files. No subset of the gate's seven rule identifiers reproduces
  129 under the method the record states. Every file in the population is
  unchanged since `9d83823`, so the figure is no head artifact. The
  correction must state the measured total, the method that produces it, the
  head it was read at, and the split between the feature's own lines and the
  rest. The `T298` task text keeps its bytes under the Immutability clause of
  Pull Request Quality. A narrower gate row belongs to
  `specs/002-prose-commit-lint` and to a constitutional reading of the
  exemption's scope, and no gate rule, threshold, vocabulary, or marker moves
  here (HIGH, Constitution X.4, Constitution XI.6, T261, T294, T298,
  `contradicts`)

### MEDIUM: the three live-artifact violations the record names have no task

- [X] T303 Close the three Principle XI violations in this feature's live
  artifacts that the exemption-scope section names and that no task
  authorizes. `specs/007-counters-and-timers/citations.md:875-878` names
  `specs/007-counters-and-timers/plan.md:347`,
  `specs/007-counters-and-timers/plan.md:457`, and
  `specs/007-counters-and-timers/spec.md:33`, and states they stand because
  no task authorizes closing them. All three reproduce in this pass.
  `plan.md:347` reads that the fast window, the fast branch of the open, and
  the catalog's disclosure are all measured, and the sentence contrasts that
  with exclusion, an XI.2 ` rather than ` on a line carrying code spans.
  `plan.md:457` carries five
  constructions on one line: three ` rather than `, at the sentence deleting
  a `find` overload with no callers, at the sentence naming the category-3
  blocks as compiler output, and at the sentence saying the reachability
  claims that failed the audit are gone; and two ` instead of `, at the
  sentence naming the `to_ecma` slice that captured a POSIX class name with
  its colons, and at the sentence saying the exclusion rests on
  measurement. `spec.md:33`
  reads that the console line is unchanged and that it records the regex
  where the matched text would go, an XI.2 ` rather than ` in the `T122`
  clarification. The
  `T298` text authorized exactly three lines and named three others, and all
  three of those are closed at `citations.md:856-862`, so these three stand
  outside that authorization. Each sentence must state what the thing is,
  with each fact in its own sentence, and no line count in any of the three
  files may move, so the anchor totals the counting rule reads over line
  ranges keep their values (MEDIUM, Constitution XI.2, Constitution XI.6,
  Constitution X.4, T298, `missing`)

### LOW: the em-dash figure in the last two records names no scope

- [X] T304 Name the scope the em-dash figure in the last two records covers,
  and carry both measured readings beside it.
  `specs/007-counters-and-timers/tasks.md:4593` reads that 0 occurrences of
  the em-dash code point stand in every feature-scope file, and the same
  phrase appears in the `T298` task text at
  `specs/007-counters-and-timers/tasks.md:4790`. Measured in this pass over
  `git ls-files`, the code scope, which is the 9
  `include/speedgun-ng/counters*.hpp` headers, `source/counters/**`,
  `test/source/counters_*.cpp`, `test/compile-fail/`,
  `test/pmu-events-gate-fixture/`, `example/`, `tools/pmu_events/`,
  `docs/pages/counters-overhead.md`, `cmake/`, `CMakeLists.txt`,
  `.github/workflows/ci.yml`, and `test/counters_*.sh`, carries 0
  occurrences of U+2014, and the feature's artifacts carry 65, of which 64
  sit in `specs/007-counters-and-timers/sg_counters.md` across the 58 lines
  `citations.md:206-215` enumerates, a figure that reproduces, and 1 sits at
  `specs/007-counters-and-timers/tasks.md:2731`, inside a closed task line.
  The record enumerates the journal's debt at `citations.md:203-215` and names
  neither reading of the phrase, so a reader who takes it to cover the
  artifacts reads 0 where 65 stand. The record must give both figures, the
  scope each covers, and the head. The preamble and every closed task line
  keep their bytes, the closed journal keeps its bytes under the Principle
  XI.1 scope paragraph at `.specify/memory/constitution.md:437-441`, and no
  gate rule, threshold, or vocabulary moves (LOW, Constitution X.4,
  Constitution XI.1, T261, T283, `partial`)

### LOW: one re-anchor row's landing spans the line the amendment replaced

- [X] T305 Correct the landing column of the one re-anchor row whose range
  crosses the line the 2.10.0 amendment replaced.
  `specs/007-counters-and-timers/citations.md:774` gives `tasks.md:4413`,
  `T292`, the anchor as written at `:240-242`, the landing at `:269-271`, and
  the shift 29. Measured against the constitution at `4fd1189` and the
  working tree, old `:240` and old `:241` hold the same text as new `:269` and
  new `:270` exactly, and old `:242` reads a line that ends the clause's
  citation while new `:271` reads that same text followed by the sentences
  the amendment added, because the amendment replaced the clause that range
  spans. The record's own paragraph at `citations.md:691-694` carves old line
  242 out of the +29 band, so the table and the paragraph disagree for one row
  of 59. The other 58 rows were resolved line for line in this pass and hold
  the same text at both ends with the shift their column states, and the 48
  path-form anchors and 17 bare continuations the section counts reproduce.
  The row must give the landing the replaced clause now occupies at
  `citations.md:693` beside the +29 figures the other two lines of the range
  take, or split the range at the band boundary the paragraph states. No
  other row, no closed task line, no dated preamble, and no figure the record
  carries moves (LOW, Constitution X.4, Constitution IV, T253, T260, T292,
  T296, `partial`)

## Phase 44: Convergence

Appended by `/speckit.converge` after an audit of the branch tip at `21fcb3a`
and of the residue the thirty-two waves before it left. Nothing above this
line changed.

Audit evidence, all produced by this pass from the repository root on Linux,
over a working tree `git status --porcelain` reports clean and `21fcb3a`
carries. Every figure below is one this pass measured; no figure is carried
forward from an earlier preamble.

Prose, template, and spelling. `python3 tools/prose/prose_gate.py --check all`
exits 0, and `cmake -P cmake/prose-lint.cmake` exits 0 on the same verdict,
`prose-lint: 144 sources, 14620 units examined, 0 findings, 1 skipped`, the
one skipped source being `hwloc.md`, which the gate names unreadable.
`cmake -P cmake/spell.cmake` exits 0. In tree mode
`python3 tools/prose/prose_gate.py --check all --mode tree` exits 1 at
`prose-lint: 259 sources, 24113 units examined, 108 findings, 0 skipped`,
spread over 18 files: 89 under `specs/001-dbc-facility/`, 9 in `test/`, 5 in
`tools/dbc/`, 3 in `docs/pages/dbc-overhead.md`, and 2 in
`include/speedgun-ng/`, which is the spread
`specs/007-counters-and-timers/citations.md:448-452` records. Read narrowed
to `specs/007-counters-and-timers` and `docs/pages/counters-overhead.md`,
`--check all --paths` exits 0 at
`prose-lint: 88 sources, 10913 units examined, 0 findings, 0 skipped`, and
`--check prose --mode tree --paths` on the same two paths exits 0 at
`13 sources, 8439 units examined, 0 findings, 0 skipped`. The gate's own
whole-repository form `--check prose --mode tree` exits 1 at
`prose-lint: 184 sources, 21315 units examined, 108 findings, 0 skipped`.

Build and test. `cmake --preset=dev` exits 0 and `cmake --build --preset=dev`
exits 0 over a fully incremental 27-line log carrying 27 `Built target` lines
and 0 lines matching `Building` or `Linking`. `ctest --preset=dev` exits 0
with `100% tests passed out of 38`, 0 failed, 0 skipped, in 43.90 s.
`ctest --test-dir build -N` exits 0 at `Total Tests: 38`, of which 17 match
`-R counters`, and `ctest --test-dir build/dev -R counters` exits 0 at
`100% tests passed out of 17` in 3.22 s while `-R counters_pmu` exits 0 at
`100% tests passed out of 1` in 0.08 s.
`cmake --build build/dev -t format-check` exits 0, and
`cmake --build build/dev -t dbc-gate` exits 0, reporting
`doc-gate: 135 interfaces, 0 gaps` beside
`pair-gate: 135 interfaces, 0 gaps`. The table check
`python3 tools/pmu_events/update_pmu_events.py --check` exits 0 and prints no
line. The coverage gate `bash tools/dbc/coverage_gate.sh
build/coverage/coverage.info` exits 0 over `source files: 21` at
`lines.......: 100.0% (1955 of 1955 lines)`,
`branches....: 100.0% (705 of 705 branches)`, and
`functions...: 98.0% (289 of 295 functions)` on the axis no gate scores; the
capture is timestamped 2026-09-28 13:44 and the last commit touching
`include/` or `source/counters/` is `31363e8` at 2026-09-28 10:40, so no
library line moved after the capture. `bash test/counters_header_purity.sh .`
exits 0 at `counters_header_purity: clean`, and `bash
test/counters_push_atomic_scan.sh .` exits 0 at
`counters_push_atomic_scan: clean`. `readelf -d` on
`build/dev/example/counters_standalone_example` and on
`build/dev/example/counters_giraffe_example` names `libstdc++.so.6`,
`libgcc_s.so.1`, and `libc.so.6` in each, with 0 `speedgun-ng` entries, and
both executables exit 0.

Release configuration. `cmake --preset=ci-ubuntu` exits 0 with
`CMAKE_BUILD_TYPE:STRING=Release` in the cache, beside
`CMAKE_CXX_CLANG_TIDY:UNINITIALIZED=clang-tidy;--header-filter=^/home/archerc/code/speedgun-ng/;--exclude-header-filter=^/home/archerc/code/speedgun-ng/external/`
and `CMAKE_CXX_CPPCHECK:UNINITIALIZED=cppcheck;--inline-suppr`, and
`cmake --build build` exits 0 over a fully incremental 27-line log carrying 0
lines matching `Building` or `Linking`, 0 matching `warning`, and 0 matching
`error:`. No compile ran, so this pass states no release-configuration
diagnostic total, as the Phase 39 through Phase 43 preambles each state none.

Static analysis, measured per translation unit. `run-clang-tidy` over the
eleven translation units `find source/counters -name '*.cpp'` returns,
selected from `build/dev/compile_commands.json` with the repository
`.clang-tidy` and the source filter the record names at
`specs/007-counters-and-timers/citations.md:586-591`, exits 0 and reports 734
lines carrying `warning:` and 0 carrying `error:`. The per-file
decomposition reproduces `specs/007-counters-and-timers/citations.md:600-624`
figure for figure, and 3 of the 734 name `external/`, so 731 name the
feature's own files. On `source/counters/plan.cpp` alone the same launcher
form exits 0 with 92 `warning:` lines and 0 `error:` lines, reporting
`20811 warnings generated`, the figure
`specs/007-counters-and-timers/citations.md:637` records, decomposing
`include/speedgun-ng/counters_measurement.hpp` 40,
`source/counters/plan.cpp` 35, `include/speedgun-ng/counters_provider.hpp` 9,
`include/speedgun-ng/counters_system.hpp` 4, `source/counters/detail/core.hpp`
3, and `include/speedgun-ng/dbc.hpp` 1, with
`readability-identifier-length` the largest class at 43 of the 92.
`cppcheck --inline-suppr -q --force` over the same eleven translation units
exits 0 and prints no line, and the same launcher form on
`source/counters/plan.cpp` alone exits 0 and prints no line, which is what
`specs/007-counters-and-timers/citations.md:970-972` records against the
Phase 42 preamble.

Counting rule for every anchor total in this preamble, the rule
`specs/007-counters-and-timers/citations.md:32-41` states, applied with
CPython 3.14.7 `re.finditer` over whole matches `m.group(0)`, the unit
`(?:[\w./-]+/)*[\w.-]+\.[A-Za-z0-9]+:\d+(-\d+)?` and the bare-continuation
unit `(?<![\w./-]):\d+\b`: applied to
`specs/007-counters-and-timers/tasks.md` lines 1 through 2422 the count is
702 occurrences, 467 distinct tokens, and 420 bare continuations; 1 through
2578 give 724, 476, and 449; 1 through 2905 give 776, 518, and 472; 1 through
3192 give 815, 548, and 485; 1 through 3362 give 844, 565, and 501; 1 through
3599 give 880, 579, and 540; 1 through 3852 give 921, 594, and 557; 1 through
4207 give 979, 624, and 595; 1 through 4479 give 1024, 654, and 639, so the
figures the Phase 37 through Phase 43 preambles carry reproduce exactly; 1
through 5201 give 1123, 712, and 735. The nine live artifacts over their
whole length give 44 occurrences, distributed `spec.md` 28, `plan.md` 7,
`quickstart.md` 4, `research.md` 3, and `contracts/system-contract.md` 2,
with none in `data-model.md`, `sg_counters.md`,
`contracts/provider-contract.md`, or `contracts/measurement-contract.md`.
`citations.md` over its whole length gives 246 occurrences, 206 distinct
tokens, and 349 bare continuations, against the 241, 202, and 345 the Phase
42 preamble records at `14b8e48`; the same three figures at `9d83823` are 144,
126, and 184, so the record grew by 102 occurrences across the last two
cycles.

Constitution anchors. The feature places 53 path-form anchors into
`.specify/memory/constitution.md` over `tasks.md` lines 1 through 5201 and
`speckit.converge`-measured at 45 in `tasks.md` lines 1 through 4479, 8 in
lines 4480 through 5201, and 8 in the record. Of the 8 in the record, 3 carry
a row in the re-anchor table at
`specs/007-counters-and-timers/citations.md:725-780` and 5 do not, and each of
the 5 lands on the text its sentence describes: `citations.md:576` names
`:10-20` for the 2.10.0 Sync Impact Report, `citations.md:660` and
`citations.md:912` name `:269-274` for the static-analysis clause,
`citations.md:682` names `:24-28` for the report's shift notice, and
`citations.md:844` names `:504-506` for the exemption. Every row of the
re-anchor table was resolved line for line against the constitution at
`4fd1189` and at `21fcb3a`: 47 hold the same text at both ends with the shift
their column states, and the `tasks.md:4413` row holds the corrected split
`T305` wrote, with old `:240-241` equal to new `:269-270` and old `:242` a
prefix of new `:271`. The 11 bare-continuation rows at
`specs/007-counters-and-timers/citations.md:783-796` resolve with 0
mismatches.

Coverage of the check: 127 requirement keys, counted as 50 functional
requirements numbering `FR-001` through `FR-050` with no gap, 10 success
criteria numbering `SC-001` through `SC-010` with no gap, 49 user-story
acceptance scenarios counted as US1 7, US2 7, US3 6, US4 6, US5 4, US6 7, US7 6,
and US8 6, and 18 edge cases at `specs/007-counters-and-timers/spec.md:195-212`;
45 design keys, counted as 15 research decisions `R-001` through `R-015` at
`specs/007-counters-and-timers/research.md:7` through `:125`, 11 data-model
entities `E-01` through `E-11` at
`specs/007-counters-and-timers/data-model.md:7` through `:137`, and 19 contract
clauses `C-MEA-1` through `C-MEA-7`, `C-PRO-1` through `C-PRO-6`, and
`C-SYS-1` through `C-SYS-6`; and 11 constitution principles with X.1 through
X.4 and XI.1 through XI.6 read one by one at version 2.10.0. Re-measured
here: the 9 headers `include/speedgun-ng/counters*.hpp` matches, the 11
translation units `find source/counters -name '*.cpp'` returns beside the 2
headers under `source/counters/detail/` and the 5 under
`source/counters/linux_pmu/`, the 13 `add_executable(counters_` calls
`test/CMakeLists.txt` carries, the 34 top-level `add_test` calls it carries,
the `LCOV_EXCL` count at 301 tokens over 11 files and 301 marker lines, 0
lines matching `TODO` or `FIXME` over the feature scope, 0 occurrences of the
em-dash code point over the 81 tracked code-scope files and 65 over the
feature's 12 Markdown artifacts, the single `NOLINT` directive at
`include/speedgun-ng/counters_measurement.hpp:634` with its reason at
`:630-633`, 0 `std::atomic` in `source/counters/push_provider.cpp`, and no
`check(true, ...)` in the counters tests. The gate's blind spot, the five
carriers at `tools/prose/prose_gate.py:876-883` measured with the carriers
masked before the token search, was reproduced at `14b8e48` as 279 lines over
25 files with 132 inside the feature's own artifacts, and at `21fcb3a` as 276
over 23, and the per-file split at `14b8e48` reproduces the nine named
figures at `specs/007-counters-and-timers/citations.md:867-875` exactly, with
1 in each of 15 further files. Over the live artifacts the masked scan finds
0 hits in `spec.md`, `plan.md`, `citations.md`, `quickstart.md`,
`data-model.md`, `sg_counters.md`, the three contracts,
`checklists/requirements.md`, and `docs/pages/counters-overhead.md`, and 93
in `tasks.md` and 70 in the closed journal, which is the debt the Principle
XI.1 scope paragraph grandfathers. Five functional requirements were re-read
against the shipped code on their own evidence: `FR-028` enforces the
power-of-two ring capacity at `source/counters/plan.cpp:233-236`, `FR-022`
ships the thunk seam with its documented fallback at
`include/speedgun-ng/counters_provider.hpp:324-335`, `FR-045` runs the table
check in the `test` job at `.github/workflows/ci.yml:160`, `FR-049` is met by
the `readelf -d` result above, and `FR-010` holds with the only platform term
in the core headers the mandated enumerator `read_mode::fast_rdpmc` at
`include/speedgun-ng/counters_core.hpp:96`, which
`test/counters_header_purity.sh:22-36` exempts for the reason it states. No
closure claim in the Phase 12 through Phase 43 preambles was accepted as
evidence.

The library is clean. Every code anchor this pass re-read lands, the `T267`
mixed-owner refusal stands at `source/counters/push_provider.cpp:120`, and no
commit on this branch since `31363e8` at 2026-09-28 10:40 touches the
measurement path, so the four findings below are all in the record or in a
live artifact, none in the library.

The measured `SC-004` reading is recorded here because three artifacts state
it and no pass re-measured it. `./build/dev/test/counters_overhead` on this
host prints `clock, syscall (vDSO) min 70.0 ns median 70.0 ns max 110.0 ns`,
`pmu group, fast_rdpmc min 180.0 ns median 190.0 ns max 220.0 ns`,
`pmu single, fast_rdpmc min 130.0 ns median 130.0 ns max 150.0 ns`, and
`pmu group, fold only 542.6 ns per first-to-last fold`, then
`fast median 130.0 ns against syscall median 70.0 ns` and
`order check: the fast-mode median is above the syscall-mode median`, and
exits 0. `./build/test/counters_overhead`, the release binary, prints
`40.0/40.0/50.0`, `70.0/70.0/80.0`, `50.0/60.0/60.0`, `34.3 ns per
first-to-last fold`, the same order line, and exits 0.
`source/counters/plan.cpp:252` carries `constexpr int kCalibrationSamples =
257`, so the `257 timed actions` `specs/007-counters-and-timers/spec.md:311`
names is the shipped constant. The four figures are measurements; no gate
verdict rests on them, and this preamble asserts no release-configuration
performance claim beyond them.

Six findings: 2 `contradicts` and 4 `partial`; 3 MEDIUM and 3 LOW. No finding
is `missing` or `unrequested`, and no constitution MUST statement is
violated, which is why no CRITICAL finding is emitted: the MUST statements in
force here are I's C++23 rule, II's release-artifact verification, V's
`format-check`, VI's three coverage gates, VIII's build, test, static-analysis,
format, spell, prose, and coverage gates, and IX's artifact and release-build
clauses, and each one this pass could run reported its verdict. The
`ci-sanitize` preset and the `consumer-release` job were not run, so no verdict
is claimed for them.

### MEDIUM: the record's working-tree anchor total names no head and no longer reproduces

- [X] T306 Restate the reading of this record's own path-form anchor total, which
  `specs/007-counters-and-timers/citations.md:394-395` states as `raised the
  totals to 144 occurrences and 126 distinct tokens, which is the reading the
  working tree yields`. Measured in this pass with the rule at `:32-41` under
  CPython 3.14.7 `re.finditer` over whole matches `m.group(0)`, the unit
  `(?:[\w./-]+/)*[\w.-]+\.[A-Za-z0-9]+:\d+(-\d+)?`, over the file's whole
  length, the working tree yields 246 occurrences and 206 distinct tokens, at
  `21fcb3a`, and the same three figures at `14b8e48` are 241, 202, and 345
  bare, and at `9d83823` are 144, 126, and 184 bare, so the 144 and 126 the
  sentence asserts are the `9d83823` reading and the clause `which is the
  reading the working tree yields` names no head. The sentence must state the
  head each figure was read at, the current reading, and the rule that makes
  the total move, and no gate rule, threshold, vocabulary, or marker moves
  (MEDIUM, Constitution X.4, T265, `contradicts`)

- [X] T307 Restate the constitution-anchor population the same section counts,
  which `specs/007-counters-and-timers/citations.md:699-701` gives as `the
  feature places 48 path-form anchors into the constitution: 45 in
  specs/007-counters-and-timers/tasks.md lines 1 through 4479 and 3 in this
  record`, and `:709` closes with `The 65 anchors divide into 59 stale and 6
  historical`. Measured in this pass with the same rule over `git ls-files`,
  `tasks.md` lines 1 through 4479 carry 45 constitution path-form anchors, the
  45 reproduces, the 3 in the record is 8, at
  `specs/007-counters-and-timers/citations.md:156`, `:222`, `:455`, `:576`,
  `:660`, `:682`, `:844`, and `:912`, and `tasks.md` lines 4480 through 5201
  carry 8 more, at `:4551`, `:4682`, `:4689`, `:4721`, `:4778`, `:4857`,
  `:4863`, and `:5176`, so the feature places 61 anchors and the 48 and the 65
  are the `9d83823` reading. Five of the eight in the record were added at
  `14b8e48` and carry no row in the re-anchor table at `:725-780`, and all
  five land on the text their sentence names, so the class of an anchor that
  was never right stays empty and only the totals and the table move. The
  paragraph must give the measured totals, the head, and a row per record
  anchor, and no closed task line, no dated preamble, and no constitution line
  moves (MEDIUM, Constitution X.4, Constitution X.2, T253, T260, T292, T296,
  T305, `contradicts`)

### MEDIUM: the success-criteria verdict row states suite counts the tree contradicts

- [X] T308 Correct the two suite counts the SC-002 row carries at
  `specs/007-counters-and-timers/quickstart.md:138`, which reads `the suite
  is 37 tests and all 37 pass, none skipped` and reports `ctest --test-dir
  build/dev -R counters` at `16 passed, 0 skipped, exit 0`. Measured in this
  pass, `ctest --test-dir build -N` exits 0 and reports `Total Tests: 38` and
  17 tests matching `-R counters`, `ctest --preset=dev` exits 0 at `100% tests
  passed out of 38` with 0 skipped, `ctest --test-dir build/dev -R counters`
  exits 0 at `100% tests passed out of 17` in 3.22 s, and
  `ctest --test-dir build/dev -R counters_pmu` exits 0 at `100% tests passed
  out of 1`. The 38th is the negative control commit `7920853` registered at
  `test/CMakeLists.txt:237-240`, and the record's own movement table at
  `specs/007-counters-and-timers/citations.md:903-907` tracks the
  `add_executable(counters_` count 12 to 13 at that commit and carries no row
  for the ctest total or the counters subset, and no closed task authorizes
  the correction. The two counts must match the tree the row reports on, the
  PASS verdict stands on the 38 of 38 this pass measured, and no other SC
  row's verdict, evidence filename, or `permission_blocked` finding may change
  (MEDIUM, Constitution X.4, plan: Test Plan, quickstart section 12 SC-002,
  `contradicts`)

### LOW: the record attributes three progress lines to a form that prints none

- [X] T309 Correct the `cppcheck` output claim at
  `specs/007-counters-and-timers/citations.md:970-972`, which reads that the
  same launcher form on `source/counters/plan.cpp` alone `exits 0 and prints
  three progress lines and no finding`, where the launcher form the record
  quotes at `:967` is
  `cppcheck --inline-suppr -q --force $(find source/counters -name '*.cpp')
  -I include`. Measured in this pass, that exact form on
  `source/counters/plan.cpp` exits 0 and prints 0 lines on stdout and 0 on
  stderr, and the same command with `-q` dropped prints exactly three, the
  progress lines `Checking source/counters/plan.cpp ...` and its two
  macro-definition lines, so the three lines belong to a form the record does
  not quote. The sentence must name the form each reading comes from; the
  `0 findings` reading and the whole-scope `0 lines` result both stand as
  measured, the Phase 42 preamble at
  `specs/007-counters-and-timers/tasks.md:4548-4550` keeps its bytes, and no
  analyzer invocation, suppression, or configuration value moves (LOW,
  Constitution X.4, Constitution VIII, T302, `partial`)

### LOW: the record names seven public headers where the plan names nine

- [X] T310 Correct the population label the static-analysis sections carry, which
  `specs/007-counters-and-timers/citations.md:594-595` reads `the seven public
  counters headers contribute 320` and `:941` reads `feature scope:
  source/counters/ and the seven public counters headers` beside `1467`.
  Measured in this pass, `include/speedgun-ng/counters*.hpp` matches 9
  headers, `counters.hpp` and `include/speedgun-ng/counters_core.hpp` carry
  0 `warning:` lines and the other 7 carry 320 between them, being
  `counters_measurement.hpp` 196, `counters_provider.hpp` 99,
  `counters_system.hpp` 16, `counters_push.hpp` 3, and
  `counters_pmu.hpp`, `counters_fake.hpp`, and `counters_clock.hpp` 2 each,
  so the 320 reproduces and the label does not: the nine are the public set
  `specs/007-counters-and-timers/plan.md:46` and `:328` both name, and seven
  are the ones that carry a diagnostic. Each sentence must say which set it
  counts and that the two silent headers are inside the nine, the 1467 stays
  the figure its own pass measured, and no recorded diagnostic total, gate
  rule, or header moves (LOW, Constitution X.4, plan: Public API surface
  added, `partial`)

### LOW: the record's own sections name populations this pass measured differently

- [X] T311 Bring the three population figures the record's own prose carries into
  line with the readings this pass measured, and state the head each was read
  at. First, `specs/007-counters-and-timers/citations.md:867-868` reads that
  the dropped population `is 279 lines repository-wide over 25 files` and
  `:885-886` closes with `The tree this pass leaves carries 276 over 23
  files`; measured in this pass with the gate's own `classify_source`,
  `build_units`, the five carriers at `tools/prose/prose_gate.py:876-883`, the
  carriers masked before the token search, and the gate's full rule set, the
  population at `14b8e48` is 279 over 25 with 132 inside the feature's own
  artifacts and 147 outside, and at `21fcb3a` it is 276 over 23 with 129
  inside and 147 outside, so both figures reproduce and the sentence names
  the head `14b8e48` while the closing sentence names none. Second,
  `specs/007-counters-and-timers/citations.md:930` reads that the release
  log `carries 2953 lines with warning: and 1 line with error:`, a figure
  this pass did not measure because the release build was fully incremental
  and no compile ran, so the sentence stands on its own head and the
  predecessor preambles state no total, and the record must keep that
  distinction. Third, `specs/007-counters-and-timers/citations.md:208-215`
  enumerates the 58 journal lines holding U+2014, which this pass
  reproduces exactly, 58 lines with 40 carrying a code span, 23 a path-like
  token, and 8 a four-space continuation at the lines named, so that
  enumeration needs no change and is named here as the one population of the
  three that reproduces whole. The two live sentences must name their heads;
  the dated preamble, the closed journal, the closed task lines, the 279, the
  276, and the 2953 keep their bytes, and no gate rule, threshold,
  vocabulary, or marker moves (LOW, Constitution X.4, Constitution XI.6,
  T261, T283, T302, `partial`)

## Phase 45: Convergence

Appended by `/speckit.converge` after an audit of the branch tip at `b945af8`
and of the residue the thirty-three waves before it left. Nothing above this
line changed.

Audit evidence, all produced by this pass from the repository root on Linux,
over a working tree `git status --porcelain` reports clean at `b945af8`.
Every figure below is one this pass measured; no figure is carried forward
from an earlier preamble.

Prose, template, and spelling. `python3 tools/prose/prose_gate.py --check all`
exits 0, and `cmake -P cmake/prose-lint.cmake` exits 0 on the same verdict,
both at `0 findings, 1 skipped`, the one skipped source being `hwloc.md`,
which the gate names unreadable. In tree mode
`python3 tools/prose/prose_gate.py --check all --mode tree` exits 1 at
`108 findings, 0 skipped`, and
`python3 tools/prose/prose_gate.py --check prose --mode tree` exits 1 at
`108 findings, 0 skipped`. Read narrowed to
`specs/007-counters-and-timers` and `docs/pages/counters-overhead.md`,
`--check all --paths` exits 0 at `0 findings, 0 skipped`, and
`--check prose --mode tree --paths` on the same two paths exits 0 at
`0 findings, 0 skipped`. `cmake -P cmake/spell.cmake` exits 0.

The source and unit totals of those five gate forms are omitted here on
purpose. `specs/007-counters-and-timers/tasks.md` is a source the gate
examines in every one of the five, so each total rises by the units this
section adds the moment this section lands, and a total stated in a preamble
is short by the next pass for exactly that reason. The Phase 43 and Phase 44
preambles each stated five such totals, and `b945af8` measured both sets
short, the three tree-mode forms by 325 units each and the two range-mode
forms by 1 each. The findings totals carry no such dependence and each
reproduces at `108`, over the 18 files and in the spread
`specs/007-counters-and-timers/citations.md:448-452` records: 89 under
`specs/001-dbc-facility/`, 9 in `test/` as 4 in `test/dbc-gate-fixture/`,
3 in `test/source/`, and 2 in `test/compile-fail/`, 5 in `tools/dbc/`, 3 in
`docs/pages/dbc-overhead.md`, and 2 in `include/speedgun-ng/`. The branch
touches two of those 18 files, `specs/001-dbc-facility/tasks.md` carrying 22
of the 108 and `test/source/dbc_test.cpp` carrying 2, 24 between them. The
range-form verdict rests on exit 0 at 0 findings, and no line this section
adds carries a finding.

Build and test. `cmake --preset=dev` exits 0 and `cmake --build --preset=dev`
exits 0, and `ctest --preset=dev` exits 0 with `100% tests passed out of 38`,
0 failed, 0 skipped, in 43.69 s. `ctest --test-dir build -N` exits 0 at
`Total Tests: 38`, of which 17 match `-R counters`, and
`ctest --test-dir build/dev -R counters` exits 0 at
`100% tests passed out of 17` in 3.18 s while `-R counters_pmu` exits 0
at `100% tests passed out of 1`.
`cmake --build build/dev -t format-check` exits 0, and
`cmake --build build/dev -t dbc-gate` exits 0, reporting
`doc-gate: 135 interfaces, 0 gaps` beside
`pair-gate: 135 interfaces, 0 gaps`. The table check
`python3 tools/pmu_events/update_pmu_events.py --check` exits 0 and prints no
line. The coverage gate
`bash tools/dbc/coverage_gate.sh build/coverage/coverage.info` exits 0
over `source files: 21` at
`lines.......: 100.0% (1955 of 1955 lines)`,
`branches....: 100.0% (705 of 705 branches)`, and
`functions...: 98.0% (289 of 295 functions)` on the axis no gate scores. The
capture is timestamped 2026-09-28 13:44:24 and the last commit touching
`include/` or `source/counters/` is `b60b361` at 2026-09-28 10:43, so no
library line moved after the capture.
`bash test/counters_header_purity.sh .` exits 0 at
`counters_header_purity: clean`, and
`bash test/counters_push_atomic_scan.sh .` exits 0 at
`counters_push_atomic_scan: clean`.

Release configuration. `cmake --preset=ci-ubuntu` exits 0 with
`CMAKE_BUILD_TYPE:STRING=Release` in the cache, and `cmake --build build`
exits 0 over a fully incremental 27-line log carrying 0 lines matching
`Building` or `Linking`, 0 matching `warning:`, and 0 matching `error:`. No
compile ran, so this pass states no release-configuration diagnostic total,
as the Phase 39 through Phase 44 preambles each state none.

Static analysis, measured per translation unit. `run-clang-tidy` over the
eleven translation units `find source/counters -name '*.cpp'` returns,
selected from `build/dev/compile_commands.json` with the repository
`.clang-tidy`, exits 0 and reports 734 lines carrying `warning:` and 0
carrying `error:`. The per-file decomposition reproduces the figure
`specs/007-counters-and-timers/citations.md:590-598` states figure for
figure: 358 over the eleven translation units, 42 over the two
provider-private headers at `source/counters/detail/core.hpp` and
`source/counters/detail/pmu.hpp`, 320 over seven of the nine public
`counters` headers, 11 over `include/speedgun-ng/dbc.hpp`, and 3 over two
vendored simdjson inline headers, with `counters.hpp` and
`include/speedgun-ng/counters_core.hpp` carrying 0 between them and
`include/speedgun-ng/counters*.hpp` matching 9 headers. Three of the 734
name `external/`, so 731 name the feature's own files.
`cppcheck --inline-suppr -q --force` over the same eleven translation units
exits 0
and prints no line; the same launcher form on `source/counters/plan.cpp`
alone exits 0 and prints 0 lines on stdout and 0 on stderr, and the same
command with `-q` dropped prints 3, being
`Checking source/counters/plan.cpp ...` and its two macro-definition lines,
so every figure `b945af8` wrote into
`specs/007-counters-and-timers/citations.md:972-976` reproduces.

Counting rule for every anchor total in this preamble, the rule
`specs/007-counters-and-timers/citations.md:32-41` states, applied with
CPython 3.14.7 `re.finditer` over whole matches `m.group(0)`, the unit
`(?:[\w./-]+/)*[\w.-]+\.[A-Za-z0-9]+:\d+(-\d+)?` and the bare-continuation
unit `(?<![\w./-]):\d+\b`. `specs/007-counters-and-timers/citations.md` over
its whole length gives 144 occurrences, 126 distinct tokens, and 184 bare
continuations at `9d83823`, 241, 202, and 345 at `14b8e48`, 246, 206, and 349
at `21fcb3a`, and 249, 208, and 351 at `b945af8`, so the figures
`b945af8` wrote at
`specs/007-counters-and-timers/citations.md:394-397` reproduce at the head
that sentence names and the head this pass audits is 3 occurrences, 2
distinct, and 2 bare beyond the highest of them. Over
`specs/007-counters-and-timers/tasks.md` lines 1 through 2422 the count is
702 occurrences, 467 distinct tokens, and 420 bare continuations; 1 through
2578 give 724, 476, and 449; 1 through 2905 give 776, 518, and 472; 1 through
3192 give 815, 548, and 485; 1 through 3362 give 844, 565, and 501; 1 through
3599 give 880, 579, and 540; 1 through 3852 give 921, 594, and 557; 1 through
4207 give 979, 624, and 595; 1 through 4479 give 1024, 654, and 639; and 1
through 5201 give 1123, 712, and 735, so every figure the Phase 37 through
Phase 44 preambles carry reproduces exactly. The nine live artifacts over
their whole length give 44 occurrences, distributed `spec.md` 28, `plan.md`
7, `quickstart.md` 4, `research.md` 3, and
`contracts/system-contract.md` 2, with none in `data-model.md`,
`sg_counters.md`, `contracts/provider-contract.md`, or
`contracts/measurement-contract.md`, and that reading is the same at
`21fcb3a` and at `b945af8`. The Phase 44 preamble's own tasks.md totals stop
at line 5201 for the same reason this section omits its gate totals: the
Phase 44 section it names is below that line and would move a total cut at
5544.

Constitution anchors. Counted with the same rule, the feature places 53
path-form anchors into `.specify/memory/constitution.md` over
`specs/007-counters-and-timers/tasks.md` lines 1 through 5201, being 45 in
lines 1 through 4479 and 8 in lines 4480 through 5201, and 8 in
`specs/007-counters-and-timers/citations.md`, so the 61 and the 78 that
`b945af8` wrote at `specs/007-counters-and-timers/citations.md:700-711`
reproduce at the head that paragraph names. The 8 in the record stand at
`specs/007-counters-and-timers/citations.md:156`, `:222`, `:455`, `:576`,
`:661`, `:683`, `:844`, and `:912` at `b945af8`. The four the 2.10.0
amendment moved still resolve:
`specs/007-counters-and-timers/citations.md:156` names `:568-570`, where the
machine-local `CMakeUserPresets.json` sentence
stands, `:222` names `:405-409` for the Principle XI.1 scope paragraph,
`:455` names `:245-248` where Principle VII's per-platform baselines bullet
stands, and the five at shift 0 name `:10-20`, `:269-274`, `:24-28`, and
`:504-506`, each of which holds the same text at `9d83823`, at `14b8e48`, at
`21fcb3a`, and at `b945af8`, measured line for line at all four heads.

Coverage of the check: 127 requirement keys, counted as 50 functional
requirements numbering `FR-001` through `FR-050` with no gap, 10 success
criteria numbering `SC-001` through `SC-010` with no gap, 49 user-story
acceptance scenarios counted as US1 7, US2 7, US3 6, US4 6, US5 4, US6 7,
US7 6, and US8 6, and 18 edge cases at
`specs/007-counters-and-timers/spec.md:195-212`; 45 design keys, counted as
15 research decisions, 11 data-model entities, and 19 contract clauses; and
11 constitution principles with X.1 through X.4 and XI.1 through XI.6 read
one by one at version 2.10.0. Re-measured here: the 9 headers
`include/speedgun-ng/counters*.hpp` matches, the 11 translation units
`find source/counters -name '*.cpp'` returns beside the 2 headers under
`source/counters/detail/` and the 5 under `source/counters/linux_pmu/`, the
13 `add_executable(counters_` calls `test/CMakeLists.txt` carries, the 34
top-level `add_test` calls it carries, the `LCOV_EXCL` count at 301 tokens
over the 11 feature files and 301 marker lines, with the 4 further tokens in
`include/speedgun-ng/dbc.hpp` outside the feature scope, 0 lines matching
`TODO` or `FIXME` over the feature scope, 0 occurrences of the em-dash code
point over the feature's code files and 65 over its Markdown artifacts, the
single `NOLINT` directive at
`include/speedgun-ng/counters_measurement.hpp:634` with its reason at
`:630-633`, 0 `std::atomic` in `source/counters/push_provider.cpp`, and no
`check(true, ...)` in the counters tests. Every code anchor this pass
re-read lands: `FR-028` enforces the power-of-two ring capacity at
`source/counters/plan.cpp:233-236`, `FR-022` ships the thunk seam with its
documented fallback at
`include/speedgun-ng/counters_provider.hpp:324-335`, `FR-024` binds one
target at `source/counters/plan.cpp:469` and opens every window with it at
`:501`, `FR-034` calibrates at provider construction,
`source/counters/plan.cpp:252` carries
`constexpr int kCalibrationSamples = 257`, `FR-010` holds with the
mandated enumerator
`read_mode::fast_rdpmc` at `include/speedgun-ng/counters_core.hpp:96`, and
the `T267` mixed-owner refusal stands at
`source/counters/push_provider.cpp:120`. No closure claim in the Phase 12
through Phase 44 preambles was accepted as evidence.

The library is clean and no finding below sits in it. Every commit touching
`include/` or `source/counters/` on this branch after the last doc pass that
re-verified anchors was measured: `b60b361` at 2026-09-28 10:43, whose
hunk header is `@@ -96,6 +96,22 @@` over
`source/counters/detail/core.hpp`, 16 comment lines and no code, and
`31363e8` at 2026-09-28 10:40 before it. One insertion at old line 96 moved
every line of that header from 97 onward by 16, and the four live-artifact
anchors that name such a line were measured one by one against it: all four
hold the text their sentences describe at `b60b361^` and none holds it now.
The commit after `01f905b`, which rewrote the ten claims the code
contradicted at 2026-09-28 07:49, and the only one after it is `b60b361`, so
one insertion accounts for every drift found.

Four findings: 2 `contradicts` and 2 `partial`; 2 MEDIUM and 2 LOW. No
finding is `missing` or `unrequested`, and no constitution MUST statement is
violated, which is why no CRITICAL finding is emitted: the MUST statements in
force here are I's C++23 rule, II's release-artifact verification, V's
`format-check`, VI's three coverage gates, VIII's build, test,
static-analysis, format, spell, prose, and coverage gates, and IX's artifact
and release-build clauses, and each one this pass could run reported its
verdict. The `ci-sanitize` preset and the `consumer-release` job were not
run, so no verdict is claimed for them.

### MEDIUM: one library insertion moved four anchors no pass re-read

- [X] T312 Re-anchor the four live-artifact citations that commit `b60b361`
  shifted, whose hunk header is `@@ -96,6 +96,22 @@` over
  `source/counters/detail/core.hpp` and which inserted 16 comment lines at
  old line 96 and no code, moving every line of that header from 97 onward by
  16. Measured in this pass, `specs/007-counters-and-timers/spec.md:34`
  names `source/counters/detail/core.hpp:104` for the one `bound_target`
  `plan_impl` holds, and that declaration stands at `:120`;
  `specs/007-counters-and-timers/citations.md:130` gives the same header's
  `:98` corrected to `:105`, where `bound_thread` is declared, and that
  declaration stands at `:121`;
  `specs/007-counters-and-timers/citations.md:158` gives
  `source/counters/detail/core.hpp:76-109` with `plan_impl` spanning
  `:76-116`, and `struct plan_impl` stands at `:76` with its closing brace at
  `:132`; and `docs/pages/counters-overhead.md:46` names
  `source/counters/detail/core.hpp:170` for the tree lookup that
  `returns nullptr when the path is absent`, and
  `[[nodiscard]] auto find(std::string_view path) -> tree_node*` stands at
  `:186`. Each of the four was read at `b60b361^` and held the text its
  sentence names, so the drift came from the insertion and not from a
  misreading. The sites that name `source/counters/detail/core.hpp:92-99`
  need no change, because the `T091` comment those sites quote stands at
  `:92-98` and stays inside the cited range, and
  `specs/007-counters-and-timers/citations.md:110` names
  `include/speedgun-ng/counters_core.hpp`, which `b60b361` does not touch.
  Each of the four must carry its corrected line and the head that line was
  read at, the correction lands in
  `specs/007-counters-and-timers/citations.md`, which
  `specs/007-counters-and-timers/plan.md:73` names as the vehicle for
  corrected anchors, the three closed task lines and the dated preambles that
  repeat the four keep their bytes, and no code line, header, comment,
  declaration, gate, rule, threshold, or vocabulary moves (MEDIUM,
  Constitution X.4, Constitution X.1, FR-024, FR-031, `contradicts`)

### MEDIUM: the record's anchor total names no reading at the reader's head

- [X] T313 Add the current reading to the sentence that gives the record's own
  anchor totals, which `specs/007-counters-and-timers/citations.md:394-397`
  closes by stating that the occurrence total moves with the file's length.
  The closed task text `T306` at
  `specs/007-counters-and-timers/tasks.md:5428-5430` required that sentence to
  state the head each figure was read at, the current reading, and the rule
  that makes the total move; `b945af8` delivered the head per figure and the
  rule and left the current reading out. Measured in this pass with the rule
  at `specs/007-counters-and-timers/citations.md:32-41` under CPython 3.14.7
  `re.finditer` over whole matches `m.group(0)`, the unit
  `(?:[\w./-]+/)*[\w.-]+\.[A-Za-z0-9]+:\d+(-\d+)?`, and the
  bare-continuation unit `(?<![\w./-]):\d+\b`, the working tree at `b945af8`
  yields 249 occurrences, 208 distinct tokens, and 351 bare continuations
  over this file's whole length, so the highest figure the sentence carries
  is 3 occurrences and 2 distinct short of the tree a reader is holding. The
  current reading is writable and holds, because a bare integer matches
  neither the path-form unit nor the bare-continuation unit, so recording it
  adds zero to the population it reports and this file's total does not move
  under the correction; that is the decision this task settles, in place of
  the alternative of declaring the figure inherently unstable and dropping
  the requirement, which would leave the record with no figure at the
  reader's head, the defect `T306` was raised against. The sentence must
  carry the 249, 208, and 351 with the head `b945af8` beside them, the
  144 and 126 at `9d83823` and the 246 and 206 at `21fcb3a` keep their
  values with their heads, and no closed task line, no dated preamble, no
  gate rule, threshold, vocabulary, or marker moves (MEDIUM,
  Constitution X.4, T306, `partial`)

### LOW: a dated preamble names a library commit behind the last one

- [X] T314 Record in `specs/007-counters-and-timers/citations.md` that the
  Phase 44 preamble's library-freshness premise is wrong, which
  `specs/007-counters-and-timers/tasks.md:5251-5253` and
  `specs/007-counters-and-timers/tasks.md:5383-5386` both state as the last
  commit touching `include/` or `source/counters/` being `31363e8` at
  2026-09-28 10:40. Measured in this pass,
  `git log -1 --date=format='%Y-%m-%d %H:%M' --format='%h %ad'` over
  `-- include source/counters` returns
  `b60b361 2026-09-28 10:43`, and `git show b60b361 --name-only` returns
  `source/counters/detail/core.hpp` alone over a hunk header of
  `@@ -96,6 +96,22 @@`, 16 insertions and no deletions, and
  `git merge-base --is-ancestor b60b361 21fcb3a` reports the commit is an
  ancestor of the head the Phase 44 preamble audits, so the premise was wrong
  at that head and was never right. The conclusion the two sentences draw
  holds, because the coverage capture is timestamped 2026-09-28 13:44:24
  and `b60b361` precedes it, so no library line moved after the capture, and
  that is why `T312` above is the first task to name the drift the insertion
  caused. A dated preamble keeps its bytes, so the correction is one
  sentence in `specs/007-counters-and-timers/citations.md` beside the
  coverage-exclusion population at `:416-436`, naming the true last commit,
  its head, its file, and the reason the conclusion stands; the Phase 44
  preamble keeps its bytes, no coverage figure moves, and no gate rule,
  threshold, vocabulary, or marker moves (LOW, Constitution X.4, `partial`)

### LOW: the re-anchor table names two of its own sites one line above them

- [X] T315 Move the two self-referential site labels the re-anchor table
  gives its five shift-0 rows, which
  `specs/007-counters-and-timers/citations.md:727` and `:728` read as
  `citations.md:660` and `citations.md:682`. Measured in this pass, the
  `.specify/memory/constitution.md:269-274` anchor stands at
  `specs/007-counters-and-timers/citations.md:661` and the
  `.specify/memory/constitution.md:24-28` anchor stands at `:683`, so each
  label sits one line above the anchor it names, and
  `git show b60b361 -- specs/007-counters-and-timers/citations.md` carries a
  hunk of 13 lines beside 14 at `@@ -590,13 +590,14 @@`, which is the one
  line above both that `b945af8` added. The third column of both rows, the
  line holding the same text at `9d83823`, is true as written, because
  `.specify/memory/constitution.md:10-20`, `:24-28`, `:269-274`, and
  `:504-506` were read line for line at `9d83823`, at `14b8e48`, at
  `21fcb3a`, and at `b945af8` and hold identical text at all four, so only
  the two site labels move, from 660 to 661 and from 682 to 683, beside the
  5 later-anchor rows the paragraph above the table already enumerates as 8
  in the record. The two labels must name the lines their anchors stand at,
  the 61 and the 78 and the 59, 6, and 13 split keep their values, and no
  closed task line, no dated preamble, no anchor row's shift, and no
  constitution line moves (LOW, Constitution X.4, Constitution X.2, T307,
  `contradicts`)

## Phase 46: Convergence

Audited at `ea6e48b` with the working tree clean. Every hard gate exits 0.
`python3 tools/prose/prose_gate.py --check all` reports 146 sources, 15469
units, 0 findings, 1 skipped. The same gate in tree mode reports 108 findings
over 18 files, distributed 89 in `specs/001-dbc-facility/`, 9 in `test/`, 5 in
`tools/dbc/`, 3 in `docs/pages/dbc-overhead.md`, and 2 in
`include/speedgun-ng/`, of which this branch touches 2, being
`specs/001-dbc-facility/tasks.md` and `test/source/dbc_test.cpp`; that form
collects its files with `git ls-files` and reads them from the working tree,
so it carries no source or unit count, and its findings count is the figure
that holds. Scoped to `specs/007-counters-and-timers` and
`docs/pages/counters-overhead.md`, the range form reports 90 sources, 11762
units, and 0 findings, and the tree form exits 0 with no finding.
`cmake -P cmake/prose-lint.cmake` and `cmake -P cmake/spell.cmake` exit 0.
`cmake --preset=dev` and `cmake --build --preset=dev` exit 0.
`ctest --preset=dev` reports 100.0 percent with 38 of 38 tests passed, and
the counters subset reports 17 of 17. The `format-check`, `dbc-gate`, PMU,
header-purity, and push-atomic targets exit 0, `dbc-gate` reporting 135
interfaces and 0 gaps on the documentation matrix and on the pairing matrix.
The coverage gate exits 0
over 21 files at 1955 of 1955 lines, 705 of 705 branches, and 289 of 295
functions. `run-clang-tidy` over the 11 counters translation units reports
734 `warning:` lines and 0 `error:` lines, and the decomposition reproduces:
358 over the 11 translation units, 42 over the detail headers, 320 over 7 of
the 9 public counters headers, 11 over `include/speedgun-ng/dbc.hpp`, and 3
external. `cppcheck --inline-suppr -q --force` over the same 11 reports
nothing and exits 0. `cmake --preset=ci-ubuntu` and `cmake --build build`
exit 0, the release build fully incremental at 27 lines with 0 compiles.

What this pass read, and at which head. The four library anchors
`b60b361` moved are re-derived here from the parent, not from the earlier
pass. `git show b60b361 --format='' -U0 -- source/counters/detail/core.hpp`
reports the hunk header `@@ -98,0 +99,16 @@ struct plan_impl` and
`git show b60b361 --format='' -U3` over the same file reports
`@@ -96,6 +96,22 @@ struct plan_impl`, so the 16 inserted lines are 99 through
114 and 96 in the wider header is leading context. Read against
`b60b361^:source/counters/detail/core.hpp`, lines 96, 97, and 98 are
byte-identical at this head, 99 is the first line that differs, old 99 holds
`std::map<std::string, std::size_t> by_address;` and stands at 115, and
`old[99:]` equals `new[115:]`. Old lines 92 through 98 and head lines 92
through 98 are equal, so the `T091` comment did not move, `target
bound_target;` stands at `:120`, and
`std::thread::id bound_thread = std::this_thread::get_id();` stands at `:121`.
`11bc422`, `01f905b`, `31363e8`, and `b60b361^` all carry `bound_target` at
104 and `bound_thread` at 105.

The constitution is byte-identical between `b945af8` and `ea6e48b`, so the
head the earlier pass audited is this one. Counted over the whole file,
`:568-570` carries the commit-message clause, `:600-602` carries the
machine-local `CMakeUserPresets.json` sentence, `:405-409` carries the
`- Standard conversions:` list, `:437-441` carries the Principle XI.1 scope
paragraph, and `:245-248` carries the Principle VII per-platform baselines
bullet, and `:10-20`, `:24-28`, `:269-274`, and `:504-506` hold identical text
at `9d83823`, `14b8e48`, `21fcb3a`, `b945af8`, and `ea6e48b` line for line.
The re-anchor table at
`specs/007-counters-and-timers/citations.md:723-730` carries 3 rows at shift
32 over its 8 rows for this record, being `:156`, `:222`, and `:455`, and 5
at shift 0.

Counting rule for every anchor total in this preamble, the rule
`specs/007-counters-and-timers/citations.md:32-41` states, applied with
CPython 3.14.7 `re.finditer` over whole matches `m.group(0)`, the unit
`(?:[\w./-]+/)*[\w.-]+\.[A-Za-z0-9]+:\d+(-\d+)?` and the bare-continuation
unit `(?<![\w./-]):\d+\b`. The section below adds 21 occurrences, 19 distinct
tokens, and 34 bare continuations, so this file over its whole length gives
1224 occurrences, 766 distinct tokens, and 835 bare continuations at this
head. The Phase 44 and Phase 45 figures over
`specs/007-counters-and-timers/tasks.md` lines 1 through 2422, 2578, 2905,
3192, 3362, 3599, 3852, 4207, 4479, and 5201 reproduce, and so does the
constitution population at
`specs/007-counters-and-timers/citations.md:700-711`.

### MEDIUM: a dated preamble asserts a verification that does not hold

- [X] T316 Record in `specs/007-counters-and-timers/citations.md` that the
  Phase 45 preamble's constitution-anchor paragraph, which
  `specs/007-counters-and-timers/tasks.md:5682-5690` carries, overstates what
  it verified. The paragraph reads `The four the 2.10.0 amendment moved still
  resolve` and then names three, and
  `specs/007-counters-and-timers/citations.md:723-730` carries 3 rows at shift
  32 over this record's 8 rows, being `:156`, `:222`, and `:455`, beside 5 at
  shift 0, so the count is 3 and the sentence names all 3. Measured in this
  pass, with the constitution byte-identical between `b945af8` and `ea6e48b`,
  `.specify/memory/constitution.md:568-570` carries the commit-message clause
  `linear sequence, no gratuitical merge commits on the base` and the
  machine-local `CMakeUserPresets.json` sentence stands at `:600-602`, and
  `.specify/memory/constitution.md:405-409` carries the `- Standard
  conversions:` list and the Principle XI.1 scope paragraph stands at
  `:437-441`, so 2 of the 3 named anchors do not hold the text the paragraph
  attributes to them, while `.specify/memory/constitution.md:245-248` does
  carry the Principle VII per-platform baselines bullet and the third anchor
  resolves. The two corrections the same record's own rows `:723` and `:724`
  already carry stand, so the paragraph contradicts the record it cites. A
  dated preamble keeps its bytes, so the correction is one paragraph in
  `specs/007-counters-and-timers/citations.md` beside the re-anchor table,
  naming the true count of 3, the two anchors that do not resolve with the
  lines that hold their text, and the fact that the third does; no
  constitution line, no re-anchor row, and no shift value moves (MEDIUM,
  Constitution X.4, T315, `contradicts`)

### MEDIUM: the record states the insertion point of the commit that moved four anchors

- [X] T317 Correct the mechanism sentence at
  `specs/007-counters-and-timers/citations.md:1042-1043`, which reads `The
  commit inserted 16 comment lines at old line 96 and no code, so every line
  of that header from 97 onward moved by 16`. Measured in this pass,
  `git show b60b361 --format='' -U0 -- source/counters/detail/core.hpp`
  reports `@@ -98,0 +99,16 @@ struct plan_impl`, so the inserted lines are 99
  through 114 and the 96 in the default-context header
  `@@ -96,6 +96,22 @@` is leading context, not the insertion point. Read
  against `b60b361^:source/counters/detail/core.hpp`, lines 96, 97, and 98 are
  byte-identical at this head, 99 is the first line that differs, and
  `old[99:]` equals `new[115:]`, so the band that moved begins at old 99 and
  old 97 and 98 did not move. The sentence is also inconsistent with the same
  record's row `specs/007-counters-and-timers/citations.md:158`, which holds
  that the `T091` comment stands at `:92-98`, because a band beginning at old
  97 would have carried lines 97 and 98 of that comment. The four figures the
  sentence supports are correct and stay: `target bound_target;` stands at
  `:120`, `std::thread::id bound_thread = std::this_thread::get_id();` at
  `:121`, `[[nodiscard]] auto find(std::string_view path) -> tree_node*` at
  `:186`, and `plan_impl` spans `:76-132` at this head and spanned `:76-116`
  at `b60b361^`. The sentence must name old 99 as the first line that moved
  and must say that old lines 96 through 98 did not move; the two rows at
  `specs/007-counters-and-timers/citations.md:1050-1051` keep their sites,
  their lines, and their heads, and no closed task line, no dated preamble,
  and no other recorded figure moves (MEDIUM, Constitution X.4, T312,
  `contradicts`)

### MEDIUM: one site the same commit moved has no row

- [X] T318 Add the missing re-anchor row for
  `specs/007-counters-and-timers/tasks.md:495`, which is `T147` and names
  `source/counters/detail/core.hpp:105` as the line where the plan carries one
  `bound_target`. The section at
  `specs/007-counters-and-timers/citations.md:1042-1046` claims the
  enumeration is complete over the sites the insertion moved, naming four, and
  the table at `:1050-1051` gives a `Line in tasks.md` column that enumerates
  every site for the anchors `core.hpp:104` and `core.hpp:170`, being 1035,
  1113, 1805, and 2326 and 769 and 813, but the anchor `T147` names is
  neither of those two. Measured in this pass, `:105` is inside the band the
  insertion moved and never held the member the sentence names:
  `11bc422`, `01f905b`, `31363e8`, and `b60b361^` all carry
  `target bound_target;` at 104 and `std::thread::id bound_thread =
  std::this_thread::get_id();` at 105, and at this head
  `source/counters/detail/core.hpp:105` carries a comment line while
  `target bound_target;` stands at `:120`. No row in
  `specs/007-counters-and-timers/citations.md:106-658` names line 495 for
  this header, because the row at `:136` covers `T147`'s
  `specs/007-counters-and-timers/spec.md:249` anchor and nothing else, and
  the commit body of `b60b361` states that the re-anchor is owed to the next
  convergence pass. The row must name `tasks.md:495`, the anchor as written
  `core.hpp:105`, the line that holds the claim `:120` read at `ea6e48b`, the
  fact that `:105` held `bound_thread` at every head this branch can reach,
  and that the sentence's site is one the enumeration at `:1043-1046` left
  out, so the completeness claim there becomes true; the closed task line keeps
  its bytes, the two existing rows keep their values, and no code line,
  header, comment, or declaration moves (MEDIUM, Constitution X.4, FR-024,
  T312, `partial`)

## Phase 47: Convergence

Appended by `/speckit.converge` after an audit of the branch tip at `a2e0f67`
and of the residue the thirty-four waves before it left. Nothing above this
line changed.

Audit evidence, all produced by this pass from the repository root on Linux,
over a working tree `git status --porcelain` reports clean at `a2e0f67`.
Every figure below is one this pass measured; no figure is carried forward
from an earlier preamble.

Prose, template, and spelling. `python3 tools/prose/prose_gate.py --check all`
exits 0, and `cmake -P cmake/prose-lint.cmake` exits 0 on the same verdict,
both at `0 findings, 1 skipped`, the one skipped source being `hwloc.md`,
which the gate names unreadable. In tree mode `python3 tools/prose/prose_gate.py
--check all --mode tree` exits 1 at `108 findings, 0 skipped`, spread over 18
files in the distribution `specs/007-counters-and-timers/citations.md:448-452`
records, 89 under `specs/001-dbc-facility/`, 9 in `test/`, 5 in `tools/dbc/`,
3 in `docs/pages/dbc-overhead.md`, and 2 in `include/speedgun-ng/`, of which
this branch touches 2, being `specs/001-dbc-facility/tasks.md` at 22 and
`test/source/dbc_test.cpp` at 2. That form collects its files with
`git ls-files` and reads them from the working tree, so it carries no source
or unit count, and its findings count is the figure that holds. Read narrowed
to `specs/007-counters-and-timers` and `docs/pages/counters-overhead.md`,
`--check all --paths` exits 0 at `0 findings, 0 skipped`, and
`--check prose --mode tree --paths` on the same two paths exits 0 at
`0 findings, 0 skipped`. `cmake -P cmake/spell.cmake` exits 0.

Build and test. `cmake --preset=dev` exits 0 and `cmake --build --preset=dev`
exits 0 over a 45-line log carrying 27 `Built target` lines and 0 lines
matching `Building` or `Linking`. `ctest --preset=dev` exits 0 with
`100% tests passed out of 38`, 0 failed, 0 skipped, in 43.82 s.
`ctest --test-dir build -N` exits 0 at `Total Tests: 38`, of which 17 match
`-R counters`, and `ctest --test-dir build/dev -R counters` exits 0 at
`100% tests passed out of 17` in 3.27 s while `-R counters_pmu` exits 0 at
`100% tests passed out of 1` in 0.08 s. `test/CMakeLists.txt` carries 38
`add_test` registrations, 13 `add_executable(counters_` calls, and the glob
`include/speedgun-ng/counters*.hpp` matches 9 headers.
`cmake --build build/dev -t format-check` exits 0, and
`cmake --build build/dev -t dbc-gate` exits 0, reporting
`doc-gate: 135 interfaces, 0 gaps` beside
`pair-gate: 135 interfaces, 0 gaps`. The table check
`python3 tools/pmu_events/update_pmu_events.py --check` exits 0 and prints no
line. The coverage gate `bash tools/dbc/coverage_gate.sh
build/coverage/coverage.info` exits 0 over `source files: 21` at
`lines.......: 100.0% (1955 of 1955 lines)`,
`branches....: 100.0% (705 of 705 branches)`, and
`functions...: 98.0% (289 of 295 functions)` on the axis no gate scores. The
capture is timestamped 2026-09-28 13:44:24, and the last commit touching
`include` or `source/counters` is `b60b361` at 2026-09-28 10:43, so no library
line moved after the capture. `bash test/counters_header_purity.sh .` exits 0 at
`counters_header_purity: clean`, and `bash
test/counters_push_atomic_scan.sh .` exits 0 at
`counters_push_atomic_scan: clean`. `readelf -d
build/dev/example/counters_standalone_example` names 3 `NEEDED` entries,
`libstdc++.so.6`, `libgcc_s.so.1`, and `libc.so.6`, and no `libm` entry,
`ldd` on the same binary prints 6 lines, and the example exits 0, which
reproduces `specs/007-counters-and-timers/citations.md:353-369` figure for
figure.

Release configuration. `cmake --preset=ci-ubuntu` exits 0 and
`cmake --build build` exits 0 over a 43-line log carrying 27 `Built target`
lines, 0 lines matching `Building` or `Linking`, 0 matching `warning`, and 0
matching `error:`. No compile ran, so this pass states no
release-configuration diagnostic total, as the Phase 39 through Phase 46
preambles each state none.

Static analysis, measured per translation unit. `run-clang-tidy` over the
eleven translation units `find source/counters -name '*.cpp'` returns,
selected from `build/dev/compile_commands.json` with the repository
`.clang-tidy` and the source filter the record names at
`specs/007-counters-and-timers/citations.md:586-591`, exits 0 and reports 734
lines carrying `warning:` and 0 carrying `error:`. The per-file
decomposition reproduces the table
`specs/007-counters-and-timers/citations.md:602-626` carries figure for
figure, `counters_measurement.hpp` 196, `counters_provider.hpp` 99,
`table_parse.cpp` 64, `group_io.cpp` 54, `fold.cpp` 42, `system.cpp` 37,
`plan.cpp` 35, `provider.cpp` 33, `clock_provider.cpp` 31, `pmu.hpp` 30,
`fake_provider.cpp` 20, `encode.cpp` 18, `fast_read.cpp` 17,
`counters_system.hpp` 16, `core.hpp` 12, `dbc.hpp` 11, `push_provider.cpp` 7,
`counters_push.hpp` 3, and 2 each for `counters_pmu.hpp`,
`counters_fake.hpp`, and `counters_clock.hpp`, beside 2 and 1 over two
vendored simdjson inline headers, so 3 of the 734 name `external/` and 731 name
the feature's own files. `cppcheck --inline-suppr -q --force` over those same
eleven translation units, with `-I include` and the paths `find source/counters
-name '*.cpp' | sort` returns, exits 0 and prints no line.

What this pass read, and at which head. This pass re-derives the `core.hpp`
band `b60b361` moved from the parent. No earlier pass's reading is carried
forward. `git show b60b361
--format='' -U0 -- source/counters/detail/core.hpp` reports the hunk header
`@@ -98,0 +99,16 @@ struct plan_impl` and `git show b60b361 --format='' -U3`
over the same file reports `@@ -96,6 +96,22 @@ struct plan_impl`, so the 16
inserted lines are 99 through 114 and the 96 in the wider header is leading
context. Read against `b60b361^:source/counters/detail/core.hpp`, which is
192 lines, and the working tree, which is 208, `old[1:98]` equals `new[1:98]`,
99 is the first line that differs, and `old[99:]` equals `new[115:]`. Old line
99 holds `std::map<std::string, std::size_t> by_address;` and stands at 115, so
`target bound_target;` stands at `:120` where old 104 held it,
`std::thread::id bound_thread = std::this_thread::get_id();` stands at `:121`
where old 105 held it, `[[nodiscard]] auto find(std::string_view path) ->
tree_node*` stands at `:186` where old 170 held it, and `plan_impl` spans
`:76-132` where it spanned `:76-116`.

Every `core.hpp` anchor across the live artifacts was enumerated independently
and checked against that band, and the set is complete. 50 sites carry a
`core.hpp` anchor, and the 47 whose high number reaches 99 fall into four
classes with no residue: 9 name `core.hpp:100-114`, the cost comment
`b60b361` itself inserted, and each was written after that commit, `b82fb7e`
at 2026-09-28 11:18 being the earliest, so none of them drifted; 16 name a
line the insertion moved and carry one of the 4 rows
`specs/007-counters-and-timers/citations.md:158`, `:1070`, `:1071`, and
`:1072`; 13 name `core.hpp:92-99`, `:95-99`, or `:90-99`, whose cited text
stands at or below 98 and is therefore outside the band; and 9 are the Phase
45 and Phase 46 preamble re-derivations, which are dated and keep their
bytes. No `core.hpp` site is unrecorded and no finding in this section names
one.

The wider history was walked, not sampled. `git log --numstat` over `include`
and `source/counters` returns 29 commits, from `6b50e99` at 2026-09-06 14:09
to `b60b361` at 2026-09-28 10:43, and every one carries an insertion or a
deletion; `b60b361` is the last, and the three commits after it, `b945af8`,
`ea6e48b`, and `a2e0f67`, touch no library file. Against that history the
whole anchor population of the live artifacts was scanned mechanically under
the record's own three criteria at
`specs/007-counters-and-timers/citations.md:88-93`, of which the first two are
mechanical and complete. 1540 anchor occurrences were read; 10 name a path the
resolver does not match, 9 lie beyond the last line of the file they name, and
1530 land in range, of which 1468 land on non-blank text and 62 name a range
that is entirely blank. Of the 9 out of range, 6 are the `fast_read.cpp:346`
and `fast_read.cpp:309-444` sites the record already carries at
`specs/007-counters-and-timers/citations.md:132-133` and `:155`, and the other
3 are the `research.md:413` and `research.md:248` pairs at
`specs/007-counters-and-timers/tasks.md:988`, which name
`specs/002-prose-commit-lint`'s files, as `T192`'s own text states, and not
this feature's 129-line `research.md`. Of the 62 blank ranges, 19 are the
record's own `Anchor as written` column, which reports the figure as written
and is correct that it is blank; 15 inside the record's stated population, the
Phase 1 through Phase 33 record at `specs/007-counters-and-timers/tasks.md`
lines 1 through 2422, name a site a row already carries; 6 more in that
population are an anchor quoted inside a finding that states the drift and its
landing, at `tasks.md:2238`, `:2297` through `:2301`, and `:2380`; and the
remaining 8, over 4 sites, are what the findings below name. Criterion 3, the
line holding text other than the claim, is the one no mechanical pass
settles. No commit on this branch has moved a line since the last pass that
read it under that criterion, so no site outside the 4 is a candidate.

The constitution is byte-identical at `9d83823`, `b945af8`, `ea6e48b`, and
`a2e0f67` and in the working tree, `sha256` prefix `d0d9a6a0b5f551f1` at each.
Counted over the whole file, `:568-570` carries the commit-message clause,
`:600-602` carries the machine-local `CMakeUserPresets.json` sentence,
`:405-409` carries the `- Standard conversions:` list, `:437-441` carries the
Principle XI.1 scope paragraph, and `:245-248` carries the Principle VII
per-platform baselines bullet, so the 3 rows at shift 32 in the re-anchor
table at `specs/007-counters-and-timers/citations.md:721-730` and the 5 at
shift 0 stand, and every claim `a2e0f67` wrote at `:777-793` and `:1059-1072`
reproduces. The 61 path-form anchors the feature places into the constitution
reproduce too, 45 in `specs/007-counters-and-timers/tasks.md` lines 1 through
4479, 8 in lines 4480 through 5201, and 8 in the record.

Counting rule for every anchor total in this preamble, the rule
`specs/007-counters-and-timers/citations.md:32-41` states, applied with
CPython 3.14.7 `re.finditer` over whole matches `m.group(0)`, the unit
`(?:[\w./-]+/)*[\w.-]+\.[A-Za-z0-9]+:\d+(-\d+)?` and the bare-continuation
unit `(?<![\w./-]):\d+\b`. The section below adds 48 occurrences, 43
distinct tokens, and 45 bare continuations, so this file over its whole
length gives 1272 occurrences, 809 distinct tokens, and 880
bare continuations at this head, against 1224, 766, and 835 over the file's
6025 lines before this section landed. The Phase 44, Phase 45, and Phase 46
figures over `specs/007-counters-and-timers/tasks.md` lines 1 through 2422,
2578, 2905, 3192, 3362, 3599, 3852, 4207, 4479, and 5201 reproduce, at 702,
724, 776, 815, 844, 880, 921, 979, 1024, and 1123 occurrences, and the nine
live artifacts over their whole length still give 44, distributed `spec.md` 28,
`plan.md` 7, `quickstart.md` 4, `research.md` 3, and
`contracts/system-contract.md` 2.

Coverage of the check: 127 requirement keys, counted as 50 functional
requirements numbering `FR-001` through `FR-050` with no gap, 10 success
criteria numbering `SC-001` through `SC-010` with no gap, 49 user-story
acceptance scenarios, and 18 edge cases at
`specs/007-counters-and-timers/spec.md:195-212`; 45 design keys, counted as
15 research decisions `R-001` through `R-015`, 11 data-model entities `E-01`
through `E-11`, and 19 contract clauses; and 11 constitution principles with
X.1 through X.4 and XI.1 through XI.6 read one by one at version 2.10.0.
Re-measured here: the 9 public headers, the 11 translation units beside the 2
headers under `source/counters/detail/`, the 38 `add_test` and 13
`add_executable(counters_` calls, the `LCOV_EXCL` population at 301 tokens
over 11 files and 301 marker lines, and the 12 tracked files
`git ls-files specs/007-counters-and-timers` returns. No closure claim in the
Phase 12 through Phase 46 preambles was accepted as evidence.

The `E-##` keys were measured, not assumed. `tasks.md` carries 37 `E-##`
occurrences: the `E-01` through `E-11` range notation on 14 lines, which is 28
of them and is the coverage sentence every preamble from Phase 12 onward
carries, and 9 individual keys over 7 entities, `E-02` at `T030`, `E-03` at
`T007`, `E-05` at `T016`, `E-07` at `T017`, `E-08` at `T024`, `E-09` at `T007`
and `T068`, and `E-10` at `T007` and `T020`. `E-01`, `E-04`, `E-06`, and `E-11`
carry no individual key. That is acceptable as it stands and this pass emits
no finding for it, on three measured grounds. First, no principle in
`.specify/memory/constitution.md` requires entity keying: Principle IX at
`:308-310` requires `tasks.md` to decompose the implementation into
independently verifiable tasks each paired with its covering unit tests, and
nothing in the file names a data-model entity. Second, all four are realized
and covered: `E-01` at `source/counters/system.cpp` and
`include/speedgun-ng/counters_system.hpp` under `FR-009`, `E-04` at
`specs/007-counters-and-timers/contracts/provider-contract.md` under
`FR-011` and `FR-012`, `E-06` in the point and delta arithmetic `FR-013` and
`FR-014` bind, and `E-11` at `external/pmu-events/RECORD` behind `FR-043`
through `FR-045`. Third, the record already carries the entity keys where
cross-reference is the point: `specs/007-counters-and-timers/plan.md:67` and
`:288`, and the three contracts between them name `E-01`, `E-03`, `E-04`,
`E-05`, `E-10`, and `E-11`. What would turn this into a finding is a MUST that
imposed a per-entity task obligation, and there is none.

Three findings: 3 `partial`; 2 MEDIUM and 1 LOW. None is `missing`,
`contradicts`, or `unrequested`, and no constitution MUST statement is
violated, which is why no CRITICAL finding is emitted: the MUST statements in
force here are I's C++23 rule, II's release-artifact verification, V's
`format-check`, VI's three coverage gates, VIII's build, test, static-analysis,
format, spell, prose, and coverage gates, and IX's artifact and release-build
clauses, and each one this pass could run reported its verdict. All three sit
in the record or in a live artifact, and none in the library.

### MEDIUM: the record's own blank-line enumeration names one of the four sites that carry the drift

- [X] T319 Add the three missing per-site rows to the corrected-anchor table at
  `specs/007-counters-and-timers/citations.md:107-166` for the three
  `tasks.md` sites that carry the nine-public-headers count against the blank
  `specs/007-counters-and-timers/plan.md:298` and `plan.md:325` and that no row
  names. Measured in this pass, the sentence at
  `specs/007-counters-and-timers/tasks.md:1165` in the Phase 22 preamble, at
  `:1364` in the Phase 24 preamble, and at `:1428` in the Phase 25 preamble each
  read `the count \`plan.md:35\`, \`plan.md:298\`, and \`plan.md:325\` record`, or
  the same claim over three lines, and
  `specs/007-counters-and-timers/plan.md:298` and `:325` are both blank while
  the Files-and-duties row naming the nine headers stands at `:301` as
  \`| \`include/speedgun-ng/counters*.hpp\` (9 files) |\` and the
  nine-headers sentence stands at `:328`; `plan.md:35` resolves and reads
  `**Scale/Scope**: 9 new public headers`. The record already carries this drift
  twice, at the row `specs/007-counters-and-timers/citations.md:161` for the
  Phase 23 preamble at `tasks.md:1298-1299` and at `:159-160` for `T195` at
  `tasks.md:1066`, so the table names 2 of the 4 sites that make the same claim
  and the 2 it omits are the same shape as the 2 it carries. A mechanical pass
  over the record's own population, `tasks.md` lines 1 through 2422, under its
  own blank-line criterion at `:92` returns exactly these 3 sites besides the
  ones already carried, so the completeness claim at `:95-96` holds once the
  rows land. Each row must name its `Line in tasks.md` value, the anchor as
  written, the blank line it lands on, the two landings `:301` and `:328`, the
  head each figure was read at, and the 9-header glob reading; the closed task
  lines and the dated preambles keep their bytes, and no `plan.md` line, no
  record row already carried, and no gate moves (MEDIUM, Constitution X.4,
  plan: Files and their duties, T276, T281, `partial`)

### MEDIUM: the record resolves two of the four quickstart figures the tree contradicts

- [X] T320 Add a section to `specs/007-counters-and-timers/citations.md`
  resolving the two suite counts the Section 13 gate-pass rows carry at
  `specs/007-counters-and-timers/quickstart.md:173` and `:174`, which each read
  \`PASS: 35 of 35, \`counters_overhead\` probe-skipped\` for
  \`ctest --preset=ci-sanitize\` and \`ctest --preset=dev\`, and which no row
  anywhere in the record resolves. Measured in this pass,
  \`test/CMakeLists.txt\` carries 38 \`add_test\` registrations, \`ctest
  --test-dir build -N\` exits 0 at \`Total Tests: 38\`, and \`ctest --preset=dev\`
  exits 0 at \`100% tests passed out of 38\` with 0 failed and 0 skipped in
  43.82 s, so the suite is 38 and \`counters_overhead\` is no longer
  probe-skipped, which is the same reading \`quickstart.md:138\` already carries
  after \`T191\` and \`T308\` corrected it in place twice, 35 to 37 to 38. The
  record already resolves this file's other two figures the tree contradicts,
  the \`readelf -d ... | grep -c NEEDED\` 4 at \`quickstart.md:137\` against the
  measured 3 at \`specs/007-counters-and-timers/citations.md:363-373\`, and the
  296 marker lines at \`quickstart.md:169\` against the measured 301 at
  \`:416-436\`, and each of those sections states the convention this one
  follows: the dated row records its own pass accurately, the row keeps its
  bytes, and the current figure is the one the tree yields. The new section
  must carry both rows, the 38 and 38 of 38 readings, the fact that
  \`counters_overhead\` passes rather than skips at this head, the head each
  figure was read at, and the \`LCOV_EXCL\` count of 301 tokens over 11 files
  that \`plan.md:457\` records, so the four \`quickstart.md\` figures the tree
  contradicts are resolved in one place; no \`quickstart.md\` line, no closed
  task line, no dated preamble, and no gate moves (MEDIUM, Constitution X.4,
  plan: Test Plan, quickstart section 13, T191, T308, `partial`)

### LOW: a closed task names two anchors for the P2 exclusion that land on nothing

- [X] T321 Add a per-site row to
  `specs/007-counters-and-timers/citations.md:107-166` for
  `specs/007-counters-and-timers/tasks.md:403`, which is `T112` and reads
  \`Register the P2 coverage exclusion that \`plan.md:335\` promises in the
  Complexity Tracking table at \`plan.md:363-366\`, and which no row names.
  Measured in this pass, \`specs/007-counters-and-timers/plan.md:335\` is a blank
  line between the Test Plan paragraph at `:334` and the \`### Execution mode\`
  heading at `:336\`, and \`plan.md:363-366\` holds the fast-read protocol
  prose, being \`cap_user_rdpmc\` bit of \`capabilities\` for the permission\`
  through \`pmc_width\`. The Complexity Tracking table stands at \`plan.md:454-458\`,
  its header at \`:455\`, the P2 x86-intrinsics row at \`:456\`, and the P2
  coverage-exclusion row \`T066\` settled at \`:457\`, which the record already
  points at from \`specs/007-counters-and-timers/citations.md:429-432\`, and the
  record already resolves the identical \`plan.md:365\` claim at \`:119\` for
  \`T079\` at \`tasks.md:367\`. The exclusion \`T112\` asked for is registered, so
  the residue is the pointer alone, which is why this finding is LOW and not
  higher; the row must name \`tasks.md:403\`, the anchor as written, the blank
  line \`:335\` and the prose \`:363-366\`, the landings \`:456\` and \`:457\`, and
  the head each was read at, so a reader auditing the coverage exclusions does
  not follow a closed task to a blank line; the closed task line keeps its
  bytes, the rows the table already carries keep their values, and no
  \`plan.md\` line and no marker moves (LOW, Constitution X.4,
  plan: Complexity Tracking, T066, T079, T112, `partial`)

## Phase 48: Convergence

Appended by `/speckit.converge` after an audit of the branch tip at `1a25e51`
and of the residue the thirty-five waves before it left. Nothing above this
line changed.

Audit evidence, all produced by this pass from the repository root on Linux,
over a working tree `git status --porcelain` reports clean at `1a25e51`.
Every figure below is one this pass measured; no figure is carried forward
from an earlier preamble.

Prose, template, and spelling. `python3 tools/prose/prose_gate.py --check all`
exits 0 at `148 sources, 16162 units examined, 0 findings, 1 skipped`, and
`cmake -P cmake/prose-lint.cmake` exits 0 on the same verdict. In tree mode
`python3 tools/prose/prose_gate.py --check all --mode tree` exits 1 at
`263 sources, 25331 units examined, 108 findings, 0 skipped`, spread over the
18 files in the distribution `specs/007-counters-and-timers/citations.md:448-452`
records, 89 under `specs/001-dbc-facility/`, 9 in `test/`, 5 in `tools/dbc/`,
3 in `docs/pages/dbc-overhead.md`, and 2 in `include/speedgun-ng/`, of which
this branch touches 2. That form collects its files with `git ls-files` and
reads them from the working tree, so it carries no figure that survives this
section landing, and its findings count is the figure that holds. Read
narrowed to `specs/007-counters-and-timers` and
`docs/pages/counters-overhead.md`, `--check all --paths` exits 0 at
`92 sources, 12455 units examined, 0 findings, 0 skipped`, and
`--check prose --mode tree --paths` on the same two paths exits 0 at
`13 sources, 9257 units examined, 0 findings, 0 skipped`.
`cmake -P cmake/spell.cmake` exits 0.

Build and test. `cmake --preset=dev` exits 0, `cmake --build --preset=dev`
exits 0, and `ctest --preset=dev` exits 0 with
`100% tests passed out of 38`, 0 failed, 0 skipped, in 43.98 s.
`ctest --test-dir build -N` exits 0 at `Total Tests: 38`, of which 17 match
`-R counters` and 1 matches `-R counters_pmu`, and
`ctest --test-dir build/dev -R counters` exits 0 at 17 of 17.
`ctest --test-dir build/dev -R counters_overhead` exits 0 at 1 of 1, passed
rather than skipped at this head. `cmake --build build/dev -t format-check`
exits 0 and `-t dbc-gate` exits 0 at `doc-gate: 135 interfaces, 0 gaps` and
`pair-gate: 135 interfaces, 0 gaps`. `python3 tools/pmu_events/update_pmu_events.py
--check` exits 0 with no output. `bash test/counters_header_purity.sh .` and
`bash test/counters_push_atomic_scan.sh .` exit 0, each printing its `clean`
line.

Static analysis and coverage. `run-clang-tidy -p build/dev` over the 11
translation units exits 0 at 734 `warning:` lines and 0 `error:` lines, and
`cppcheck --inline-suppr -q --force -I include` over the same 11 units exits 0
with no output. `bash tools/dbc/coverage_gate.sh
build/coverage/coverage.info` exits 0 at `source files: 21`,
`lines 100.0% (1955 of 1955)`, `branches 100.0% (705 of 705)`, and
`functions 98.0% (289 of 295)`.

Release build. `cmake --preset=ci-ubuntu` exits 0 and `cmake --build build`
exits 0 over a 43-line log carrying 27 `Built target` lines, 0 lines matching
`Building` or `Linking`, 0 matching `warning`, and 0 matching `error:`; the 2
lines carrying `Warning` are the capitalized CPack configure lines.
`readelf -d build/dev/example/counters_standalone_example` reports 3 `NEEDED`
entries, being `libstdc++`, `libgcc_s`, and `libc`, and the example exits 0,
so the standalone target links no benchmark library.

Anchor population. Counted under the record's own rule
`specs/007-counters-and-timers/citations.md:32-41`, applied with CPython 3.14.7
`re.findall` over whole matches `m.group(0)`, the unit
`(?:[\w./-]+/)*[\w.-]+\.[A-Za-z0-9]+:\d+(-\d+)?` and the bare-continuation unit
`(?<![\w./-]):\d+\b`. The Phase 44, Phase 45, and Phase 46 figures over
`specs/007-counters-and-timers/tasks.md` lines 1 through 2422, 2578, 2905,
3192, 3362, 3599, 3852, 4207, 4479, and 5201 reproduce, at 702, 724, 776, 815,
844, 880, 921, 979, 1024, and 1123 occurrences, and the nine live artifacts
over their whole length still give 44, distributed
`specs/007-counters-and-timers/spec.md` 28, `plan.md` 7, `quickstart.md` 4,
`research.md` 3, and `contracts/system-contract.md` 2. The 4 rows and the one
section `1a25e51` added at `specs/007-counters-and-timers/citations.md:167-170`
and `:1078-1112` reproduce every figure they state, and the rows and section
`a2e0f67` added at `:781-797` and `:1063-1076` reproduce theirs. What neither
commit records is that `1a25e51` moved the record's own lines above 167, which
the findings below measure.

The library. `git log --numstat` over `include` and `source/counters` returns
30 commits, from `6b50e99` at 2026-09-06 14:09 to `b60b361` at 2026-09-28
10:43, and every one carries an insertion or a deletion. `b60b361` is the last
commit to touch a library file, and the four commits after it, `b945af8`,
`ea6e48b`, `a2e0f67`, and `1a25e51`, touch none.
`source/counters/detail/core.hpp` stands at 208 lines with the 16 comment lines
`b60b361` inserted at `:99-114`, `bound_target` at `:120`, `bound_thread` at
`:121`, `find` at `:186`, and `plan_impl` spanning `:76-132`, and no commit on
this branch has moved a line of it since.

Findings: five. One HIGH, three MEDIUM, one LOW, and one question the audit
settled without a task. The HIGH finding is the tip commit's own movement of the
record, the three MEDIUM findings are a total the counting rule does not yield,
an enumeration whose classes do not partition its population, and a pair of gate
totals the Phase 45 preamble forbids, and the LOW finding is a commit count one
short.

### HIGH: the tip commit moved the record's own lines and no row records the shift

- [X] T323 Add a section to `specs/007-counters-and-timers/citations.md`
  recording the line movement commit `1a25e51` caused, and correct the target
  column of the re-anchor table at
  `specs/007-counters-and-timers/citations.md:296-317` and the Site column of
  the table at `:725-779`, both of which name lines that commit moved. Measured
  in this pass, `1a25e51` inserted 4 lines at `citations.md:167` and 35 at
  `:1077` and deleted none, `git diff -U0 a2e0f67 1a25e51` reporting the hunks
  `@@ -166,0 +167,4 @@` and `@@ -1072,0 +1077,35 @@`, and the first 166 lines
  of the record are byte-identical at the two heads, so every record line at or
  above 167 stands 4 higher now. Under the counting rule the 12 feature
  Markdown files carry 143 path-form `citations.md` anchors at `a2e0f67`
  numbering, of which 16 name a line whose text moved, 13 restored exactly by a
  uniform `+4` and 3 landing on a blank line. In the first table the 5 target
  values `:170-175`, `:205-211`, `:274-281`, `:283-290`, and `:297-298` stand
  at `:174-179`, `:209-215`, `:278-285`, `:287-294`, and `:301-302`, and the
  second table's Site column carries 7 of its 8 `citations.md` values at
  `:222`, `:455`, `:576`, `:661`, `:683`, `:844`, and `:912`, every one 4
  short, against the one unchanged value at `:156`. The reference at `:1057`
  names the coverage-exclusion section as `:416-436` where it stands at
  `:420-440`, and the Phase 47 preamble at
  `specs/007-counters-and-timers/tasks.md:6180` names the second table as
  `:721-730` where it stands at `:725-779`; both are dated text and keep their
  bytes, so the correction belongs in the record. The new section must name the
  commit, the two hunks, the uniform shift, each of the 16 sites by file and
  line, and the head each was read at, so the next pass measures one movement
  and does not rediscover it; the closed task lines and the dated preambles
  keep their bytes, the rows the tables already carry keep their anchors, and
  no gate moves (HIGH, Constitution X.4, Constitution X.2, T315, T318, T320,
  T321, `partial`)

### MEDIUM: the distinct-token total the Phase 47 preamble states is not the one the rule yields

- [X] T322 Add a paragraph to `specs/007-counters-and-timers/citations.md`
  beside the counting rule at `:32-41` that gives the current whole-file
  reading, so a reader auditing the anchor count finds the figures that hold
  where the Phase 47 preamble at
  `specs/007-counters-and-timers/tasks.md:6191-6194` gives one that does not.
  Measured in this pass under that rule, the whole file at `1a25e51` gives 1272
  occurrences, 790 distinct tokens, and 880 bare continuations, and at
  `a2e0f67`, over the 6025 lines then in the file, 1224, 766, and 835. The
  occurrences and bare totals reproduce at 1272 and 880; the distinct total does
  not, the correct value being 790 against the stated 809. The cause is the
  section total that sentence builds on, which reads 48 occurrences, 43 distinct
  tokens, and 45 bare continuations: the 48 and the 45 are the section's own
  figures and hold, while the 43 counts the section's distinct tokens in
  isolation, of which 19 already appear over the 6025 lines above, so the
  file's distinct total rises by 24 and not by 43. The figure 809 appears at
  `tasks.md:6193` and at no other line of the record or of this file; the other
  matches for those three digits are the 8092 units an earlier preamble names.
  The new paragraph must give the three whole-file figures, the three over the
  prefix, the 24 the distinct total rises by, the rule the reading used, and the
  head, and it must say that the dated preamble keeps its bytes; no
  `citations.md` line the rule states moves, no closed task line moves, and no
  gate moves (MEDIUM, Constitution X.4, Constitution X.2, counting rule at
  `citations.md:32-41`, `contradicts`)

### MEDIUM: the `core.hpp` enumeration at the Phase 47 preamble does not partition its own population

- [X] T324 Add a paragraph to `specs/007-counters-and-timers/citations.md`
  giving the `source/counters/detail/core.hpp` anchor population as this pass
  measured it, replacing the enumeration the Phase 47 preamble at
  `specs/007-counters-and-timers/tasks.md:6131-6143` states, and record that
  the enumeration's four classes overlap; they do not partition. Measured in this
  pass at `a2e0f67` over the 12 feature Markdown files, with the token unit the
  counting rule states and `counters_core.hpp` excluded because the band the
  paragraph reasons about is `source/counters/detail/core.hpp`, the population
  is 49 occurrences over 46 sites, of which 46 occurrences over 43 sites have a
  high number reaching 99. The preamble states 50 sites and 47 reaching 99; the
  two class figures it states as 16 and 13 measure 23 occurrences over 20 sites
  for the anchors naming a line the insertion moved and 14 occurrences over 14
  sites for the anchors naming `:92-99`, `:95-99`, or `:90-99`; the 2 class
  figures that hold are the 9 naming `core.hpp:100-114` and the 9 Phase 45 and
  Phase 46 preamble re-derivations, which are the 9 occurrences over
  `tasks.md:5546-6026`. The residue is 3 occurrences over 3 sites naming
  `core.hpp:98`, at `specs/007-counters-and-timers/citations.md:130` and
  `specs/007-counters-and-timers/tasks.md:374` and `:423`, which no class names,
  and the four classes cannot sum to the population because the 9 re-derivations
  are a subset of the moved-line and sub-99 classes. The new paragraph must give
  the 49, the 46, the 43, the 23, the 20, the
  14, the 9, the 9, and the 3, the token unit and the exclusion, and the head,
  and it must say the enumeration's own conclusion stands, that every
  `core.hpp` anchor in the live artifacts was read against the band and none
  drifted; the dated preamble keeps its bytes, so the correction is additive,
  and no `citations.md` line and no gate moves (MEDIUM, Constitution X.4,
  Constitution X.2, T312, T313, `contradicts`)

### MEDIUM: the Phase 46 preamble states the two gate totals the Phase 45 preamble forbids

- [X] T325 Add a paragraph to `specs/007-counters-and-timers/citations.md`
  carrying the whole-repository prose-gate reading at `1a25e51` and stating why
  it is the only such total this file records, so the two figures the Phase 46
  preamble at `specs/007-counters-and-timers/tasks.md:5870` and `:5879` gives
  are not the last word on them. Measured in this pass,
  `python3 tools/prose/prose_gate.py --check all` exits 0 at
  `148 sources, 16162 units examined, 0 findings, 1 skipped`, and the same
  command read with `--check all --paths specs/007-counters-and-timers
  docs/pages/counters-overhead.md` exits 0 at
  `92 sources, 12455 units examined, 0 findings, 0 skipped`. The Phase 46
  preamble states 146 and 15469, and 90 and 11762, and the Phase 45 preamble at
  `tasks.md:5570-5577` gives its reason for stating neither: this file is a
  source the gate examines in every one of those forms, so each total rises by
  the units the appending section adds the moment the section lands, which is
  what carried the Phase 43 and Phase 44 totals 325 units short in tree mode and
  1 short in range mode. The findings totals hold at 108 in tree mode and 0 in
  the narrowed forms, and the 18-file distribution holds as
  `specs/007-counters-and-timers/citations.md:448-452` records it. The new
  paragraph must give the two findings figures, the two source and unit figures
  as a reading of this head, the reason the source and unit figures cannot
  outlive an append, the head, and the pointer to the Phase 45 passage; the two
  dated preambles keep their bytes, and no gate moves (MEDIUM, Constitution
  X.4, Constitution X.2, `contradicts`)

### LOW: the library commit count the Phase 47 preamble states is one short

- [X] T326 Correct the commit count the Phase 47 preamble at
  `specs/007-counters-and-timers/tasks.md:6146` states, and record the reading
  in `specs/007-counters-and-timers/citations.md` so the figure outlives the
  preamble. Measured in this pass, `git log --numstat` restricted to `include`
  and `source/counters` returns 30 commits, from `6b50e99` at 2026-09-06 14:09
  to `b60b361` at 2026-09-28 10:43, every one carrying at least one insertion
  or one deletion, of which `b60b361` is the last; the preamble states 29 and
  names both ends of the range correctly, so the residue is the count alone,
  which is
  why this finding is LOW. The same passage states that the three commits after
  `b60b361`, `b945af8`, `ea6e48b`, and `a2e0f67`, touch no library file, which
  holds, and `1a25e51` is a fourth that touches none. The correction must give
  the 30, the command that yields it, the two endpoints with their timestamps,
  the fact that every commit carries an insertion or a deletion, and the head,
  so a reader auditing the library history counts the same 30; the dated
  preamble keeps its bytes, and no `citations.md` line and no gate moves (LOW,
  Constitution X.4, `contradicts`)

### Settled without a task: the `E-` entity keys are a convention, not a gap

The audit asked whether `specs/007-counters-and-timers/data-model.md` names 11
entities that no task keys, and it does not find a gap. The file defines `E-01`
through `E-11` at lines 7, 26, 39, 51, 60, 70, 78, 96, 115, 125, and 137, and
Constitution IX at `.specify/memory/constitution.md:308-310` requires the
implementation to be decomposed into independently verifiable tasks each paired
with its covering unit tests, which it does not ask for one row per entity. The
four entities no task names individually are realized and covered: `E-01` by
`source/counters/system.cpp` under FR-009, `E-04` by
`specs/007-counters-and-timers/contracts/provider-contract.md` under FR-011 and
FR-012, `E-06` by the point and delta types under FR-013 and FR-014, and `E-11`
by `external/pmu-events/RECORD` under FR-043 through FR-045. Every task that
needs a cross-reference already carries its key, at
`specs/007-counters-and-timers/plan.md:67` and `:288` and in the three contract
files, and the 37 `E-` occurrences over the file at `a2e0f67` are 11 range
notations and 9 individual keys over the other 7 entities, so the convention is
uniform where it applies. Nothing here needs a task, and this pass records that
so a later audit does not re-derive it.

## Phase 49: Convergence

Appended by `/speckit.converge` after an audit of the branch tip at `1b89b06`
and of the residue the five waves before it left. Nothing above this line
changed.

Audit evidence, all produced by this pass from the repository root on Linux,
over a working tree `git status --porcelain` reports clean at `1b89b06`.
Every figure below is one this pass measured; no figure is carried forward from
an earlier preamble. The counting rule is the one
`specs/007-counters-and-timers/citations.md:32-41` states: CPython 3.14.7
`re.finditer` over whole matches `m.group(0)`, the unit
`(?:[\w./-]+/)*[\w.-]+\.[A-Za-z0-9]+:\d+(-\d+)?`, and the bare-continuation
unit `(?<![\w./-]):\d+\b`. No source total and no unit total is stated below
for a file the gate examines, because this file is one of them and the figure
dies the moment this section lands; findings totals and exit codes are carried
instead, for the reason the Phase 45 preamble at
`specs/007-counters-and-timers/tasks.md:5570-5577` gives.

Prose, template, and spelling. `python3 tools/prose/prose_gate.py --check all`
exits 0 with 0 findings and 1 skipped, the skip naming `hwloc.md` as
unreadable, and `cmake -P cmake/prose-lint.cmake` exits 0 on the same verdict.
In tree mode `python3 tools/prose/prose_gate.py --check all --mode tree` exits
1 with 108 findings over 18 files, distributed `specs/001-dbc-facility/` 89,
`test/` 9, `tools/dbc/` 5, `docs/pages/dbc-overhead.md` 3, and
`include/speedgun-ng/` 2, and that findings total and that distribution are
the figures that hold. Read narrowed to `specs/007-counters-and-timers` and
`docs/pages/counters-overhead.md`, `--check all --paths` exits 0 with 0
findings and 0 skipped, and `--check prose --mode tree --paths` on the same
two paths exits 0 with 0 findings and 0 skipped. `cmake -P cmake/spell.cmake`
exits 0, reporting `Used config files:` and `1: .codespellrc`.

Build and test. `cmake --preset=dev` exits 0, `cmake --build --preset=dev`
exits 0, and `ctest --preset=dev` exits 0 at `100% tests passed out of 38`, 0
failed, 0 skipped, in 43.96 s. `ctest --test-dir build -N` exits 0 at
`Total Tests: 38`, and `ctest --test-dir build/dev -R counters` exits 0 at 17
of 17. `cmake --build build/dev -t format-check` exits 0 and
`-t dbc-gate` exits 0 at `doc-gate: 135 interfaces, 0 gaps` and
`pair-gate: 135 interfaces, 0 gaps`. `python3
tools/pmu_events/update_pmu_events.py --check` exits 0 with no output.
`bash test/counters_header_purity.sh .` exits 0 at
`counters_header_purity: clean` and `bash
test/counters_push_atomic_scan.sh .` exits 0 at
`counters_push_atomic_scan: clean`.

Static analysis, coverage, and release. `run-clang-tidy -p build/dev` over the
11 translation units `find source/counters -name '*.cpp'` returns exits 0 at
734 `warning:` and 0 `error:`, and `cppcheck --inline-suppr -q --force -I
include` over the same 11 units exits 0 with no output. `bash
tools/dbc/coverage_gate.sh build/coverage/coverage.info` exits 0 at
`source files: 21`, `lines.......: 100.0% (1955 of 1955 lines)`,
`functions...: 98.0% (289 of 295 functions)`, and
`branches....: 100.0% (705 of 705 branches)`. `cmake --preset=ci-ubuntu` exits
0 and `cmake --build build` exits 0 over a 27-line log carrying 27 `Built
target` lines, 0 matching `warning`, 0 matching `Warning`, and 0 matching
`error:`.

The library. `git log --format='%h' -- include source/counters` returns 30
commits, from `6b50e99` at 2026-09-06 14:09 to `b60b361` at 2026-09-28 10:43,
and the 5 commits after `b60b361` touch no library file, so the library has
stood clean since `T267` closed. The count 30 holds. The command the record
names for it at `specs/007-counters-and-timers/citations.md:1149` does not
return it, and T329 carries that.

Anchor populations, re-derived from git history in this pass. Counted
at `a2e0f67` over the 12 feature Markdown files with the token unit the rule
states and `counters_core.hpp` excluded, the `core.hpp` anchor population is
49 occurrences over 46 sites, of which 46 occurrences over 43 sites have a
high number reaching 99, which reproduces
`specs/007-counters-and-timers/citations.md:1122-1123` exactly. The four
classes the Phase 47 preamble names at
`specs/007-counters-and-timers/tasks.md:6131-6143` measure 23 occurrences over
20 sites for the moved-line class, 14 over 14 for the `core.hpp:92-99`,
`:95-99`, and `:90-99` class, 9 over 9 for the literal `core.hpp:100-114`
class, and 9 over 8 for the Phase 45 and Phase 46 preamble re-derivations
between `specs/007-counters-and-timers/tasks.md:5546` and `:6026`, and the
residue is 3 occurrences over 3 sites naming `core.hpp:98`. Every figure
`T324` wrote reproduces. The whole-file reading of
`specs/007-counters-and-timers/tasks.md` at `1a25e51` gives 1272 occurrences,
790 distinct tokens, and 880 bare continuations, and at `a2e0f67` over the
6025 lines then in the file it gives 1224, 766, and 835, and the 308-line
section `1a25e51` appended carries 48 occurrences, 43 distinct tokens read in
isolation, and 45 bare continuations, of which 19 distinct tokens already
appear over the 6025 lines above, so the file's distinct total rises by 24.
Every figure `T322` wrote reproduces. The `T323` section's own counts also
reproduce at `a2e0f67`: 143 `citations.md` path-form anchors over 139 sites
across the 12 feature files, 20 of them in the record itself, 16 of the 20
naming a line the `1a25e51` movement carried, and the 4 that do not writing
`citations.md:156` twice, `citations.md:130`, and `citations.md:158`.

The record cites its own line numbers, and the tip commit moved them. Under
the rule above, `specs/007-counters-and-timers/citations.md` holds 331
path-form occurrences, of which 56 name the record itself, over 37 sites, plus
433 bare continuations, and across the 12 feature Markdown files 201
path-form occurrences name the record, over 181 sites, of which 27 land on a
blank or out-of-range record line right now. The command that measures the
tip commit's movement is

```
git diff -U0 1a25e51 1b89b06 -- specs/007-counters-and-timers/citations.md
```

and it reports four insertions and three in-place replacements: 18 lines after old line 42, 22 after old line 465, and
43 after old line 1077, all of them above old line 1111, and 75 after old line
1111, which was the last line of the file, so that hunk moves nothing an
anchor names. Every record line from 43 through 465 therefore stands 18
higher, every line from 466 through 1077 stands 40 higher, and every line from
1078 through 1111 stands 83 higher. The tip commit re-anchored two tables in
place and added 158 lines over the four insertions, and the 8 Site values it
wrote into the constitution table are correct now, read against the lines they
name: `citations.md:174` holds `.specify/memory/constitution.md:568-570`,
`:244` holds `:405-409`, `:477` holds `:245-248`, `:620` holds `:10-20`,
`:705` holds `:269-274`, `:727` holds `:24-28`, `:905` holds `:504-506`, and
`:973` holds `:269-274`. What that same commit missed is recorded below.

Findings: four. Two HIGH, two MEDIUM, and one of the two MEDIUM is a
convention question the audit settled into a task for the repository owner
rather than a defect.

### HIGH: ten live site pointers in the record name the wrong line

- [X] T327 Correct the ten live site pointers in
  `specs/007-counters-and-timers/citations.md` that resolve to the wrong
  record line, so a reader following the record lands on the material the
  sentence describes. Measured in this pass, the tip commit re-anchored the
  Site column of the constitution table at `:765-819` in place, changing 8
  values from `citations.md:156`, `:222`, `:455`, `:576`, `:661`, `:683`,
  `:844`, and `:912` to `citations.md:174`, `:244`, `:477`, `:620`, `:705`,
  `:727`, `:905`, and `:973`, and it left the prose that names those rows
  untouched. The 10 sites are `:762` writing `citations.md:455` where the row
  correcting `:245-248` stands at `:769` under the Site value
  `citations.md:477`; `:826` writing `citations.md:156`, `citations.md:222`,
  and `citations.md:455` where the 3 rows at shift 32 stand at `:767`,
  `:768`, and `:769` under the Site values `citations.md:174`,
  `citations.md:244`, and `citations.md:477`; `:832` writing
  `citations.md:723` and `:834` writing `citations.md:724` where the rows
  carrying the landings `:600-602` and `:437-441` stand at `:767` and `:768`,
  and where `citations.md:723` is a blank line and `citations.md:724` is a
  section heading; `:1101` writing `citations.md:416-436` where the
  coverage-exclusion section stands at `:438-458`, its heading at `:438` and
  its last table row at `:458`; `:1264` writing `citations.md:1061` where the
  reference that sentence discusses stands at `:1101`; `:1265` writing
  `:420-440` where that same section stands at `:438-458`; and `:1267` writing
  `:725-779` where the constitution table stands at `:765-819`, its header at
  `:765` and its last row at `:819`. Six of the 10 sites the tip commit
  invalidated, and they are 8 of the record's 56 self-anchor occurrences, the
  other 2 sites resolving to no row at any head this pass measured. All 10
  corrections are equal-length in-place edits, so none adds a line to the
  record and none moves a line any other anchor names. The dated task lines
  and the dated preambles keep their bytes, the 8 Site values and the 5 target
  values the tables now carry are correct and stay, and no gate moves (HIGH,
  Constitution X.4, Constitution X.2, T312, T313, T315, T318, T320, T321,
  `partial`)

### HIGH: 119 dated sites carry a record anchor the tip commit moved, and no row records the shift

- [X] T328 Add a section to
  `specs/007-counters-and-timers/citations.md` recording the line movement
  commit `1b89b06` caused, so the 119 dated sites it moved have a record, the
  way `T323` gave the 16 that `1a25e51` moved one. Measured in this pass,
  `1b89b06` inserted 18 lines after `citations.md:42`, 22 after `:465`, and 43
  after `:1077`, and replaced in place the 2 lines at old `:311`, the 3 at old
  `:315`, and the 8 at old `:727`, `git diff -U0 1a25e51 1b89b06 --
  specs/007-counters-and-timers/citations.md` reporting those hunks, so the
  record grew from 1111 lines to 1269. Under the counting rule, 119
  occurrences over 118 lines of the dated text in
  `specs/007-counters-and-timers/tasks.md` name a record line in a moved
  band, 71 occurrences over 71 lines in the 43 through 465 band that stands 18
  higher, 48 occurrences over 47 lines in the 466 through 1077 band that
  stands 40 higher, and none in the 1078 through 1111 band, and every one of
  the 119 was correct at `1a25e51`. The drifted text includes the criterion
  block the Phase 47 preamble names as
  `specs/007-counters-and-timers/citations.md:88-93` at
  `specs/007-counters-and-timers/tasks.md:6152`, which stands at `:106-111`
  now, the constitution table that preamble names as `:721-730` at
  `specs/007-counters-and-timers/tasks.md:6181`, which stands at `:765-819`
  now, the re-anchor table the Phase 48 preamble names as `:296-317` at
  `specs/007-counters-and-timers/tasks.md:6433`, which stands at `:314-335`
  now, and the 18-file distribution the Phase 48 preamble names as
  `specs/007-counters-and-timers/citations.md:448-452` at
  `specs/007-counters-and-timers/tasks.md:6535`, which stands at `:466-470`
  now. The new section must name the commit, the four insertions and the
  three in-place replacements, the shift map, the 118 sites by file and by
  line, and the head each was read at, so the next pass measures one movement
  so the next pass measures one movement once. The dated task lines and the
  dated preambles
  keep their bytes, the values the tables already carry are correct and stay,
  and no gate moves (HIGH, Constitution X.4, Constitution X.2, T312, T313,
  T315, T318, T320, T321, T323, `missing`)

### MEDIUM: the command the record names for the library commit count does not return it

- [X] T329 Correct the command at
  `specs/007-counters-and-timers/citations.md:1149`, which the `T326`
  paragraph presents as the reproducible route to the library commit count
  and which returns a different number, so a reader auditing the library
  history with the command the record names reads the wrong figure. Measured
  in this pass, `git log --numstat --format='%h' -- include source/counters |
  sort -u | wc -l` returns 168, because `--numstat` makes the output carry
  one insertion, deletion, and path line per changed file beside the hashes
  and `sort -u` deduplicates those lines too, so the count is a count of
  output lines. The commit count is 30, and
  `git log --format='%h' -- include source/counters | wc -l`, `git log --oneline -- include source/counters |
  wc -l`, `git rev-list --count HEAD -- include source/counters`, and `git
  log --numstat --format='%h' -- include source/counters | grep -cE
  '^[0-9a-f]{7,}$'` each return it. The count 30, the first commit `6b50e99`
  at 2026-09-06 14:09, the last `b60b361` at 2026-09-28 10:43, the fact that
  every one of the 30 carries an insertion or a deletion, and the statement
  that the 5 commits after `b60b361` touch no library file all hold, so the
  residue is the command alone. The correction must put a command that
  returns 30 in place of the one that returned 168, must give the 30, the
  four commands that return it, the two endpoints with their timestamps, and
  the head, so a reader auditing the library history counts the same 30. The
  dated preamble keeps its bytes, and no gate moves (MEDIUM, Constitution X.4,
  T326, `contradicts`)

### MEDIUM: the record addresses itself by line number, and the owner decides whether that stops

- [ ] T330 Decide and record the addressing convention for
  `specs/007-counters-and-timers/citations.md`, because a record that cites
  its own line numbers has no fixed point while it is edited in place, and
  the three routes below differ in what they cost and what they break. This
  pass measured the mechanism. A correction that adds lines above a cited line
  moves that line, and the tip commit added 158 lines over four insertions,
  so the record's own tables were stale the moment that commit landed; `T327`
  then corrects 10 of the resulting site pointers and adds no line, and the
  same commit's line-neutral edit of 8 table cells had already invalidated 4
  further site pointers in the record's prose. That is the sharper form of
  the loop: the driver is the record addressing its own tables by line number,
  and the line count is one sufficient cause among several. Route one drops
  the `citations.md:NNN` Site-column values and cite by section
  heading; it costs rewriting 201 self-anchor occurrences over 181 sites
  across the 12 feature files, and it breaks the re-anchor table at
  `specs/007-counters-and-timers/citations.md:314-335` and the constitution
  table at `:765-819`, whose first column then holds no machine-checkable
  landing, and it leaves the 118 dated sites T328 measures with no correction
  target. Route two freezes the record at its 1269 lines and puts every future
  correction in a new dated file beside it; it costs one new file and it
  breaks nothing, because the 201 self-anchors freeze with the record and each
  becomes permanently historical, which is the exemption the drift criteria
  at `specs/007-counters-and-timers/citations.md:106-111` already grant to
  text describing a past revision's numbering. Route three pins the record's
  line count and appends only; it costs one rule, and it forbids the in-place
  table correction that `T323` performed and that the tip commit performed, so
  the 8 Site values the tip commit wrote become impossible and the 16 sites
  `T323` found stay uncorrected. The choice is a governance decision under
  Constitution IX and belongs to the repository owner, and this pass applies
  none of the three. The recommendation is route two, with the sibling file
  citing by section heading, never by line number, because route two
  alone reaches a fixed point at once and the heading citation removes the
  driver this pass measured (MEDIUM, Constitution IX, Constitution X.1, T323,
  `unrequested`)
