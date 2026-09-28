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
- [X] T077 Hoist the capability gate ahead of the instruction: test `user->cap_user_rdpmc` in `fast_context_read` at `source/counters/linux_pmu/fast_read.cpp` before the `_rdpmc` at `:283`, so the protocol order stated at `:40-42` and `source/counters/detail/pmu.hpp:165-166` actually holds (HIGH, FR-040, `contradicts`)
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
figure describes the committed range the gate scans, from the merge base with
`origin/master` to `30f6361`. The `--mode tree` form collects its files with
`git ls-files` and reads them from the working tree, so no commit reproduces its
total, which is the reading the Phase 21 preamble states at
`specs/007-counters-and-timers/tasks.md:1112-1119`; this preamble records no
`--mode tree` total. `ctest --test-dir build -N` exits 0 and reports
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
read the committed range from the merge base with `origin/master`, `65beada`,
to `1827d76`, and the one skipped source is `hwloc.md`, which the gate names
unreadable. The `--mode tree` form collects its files with `git ls-files` and
reads them from the working tree, so no commit reproduces its total. Read
narrowed to this feature, `python3 tools/prose/prose_gate.py --check prose
--mode tree --paths specs/007-counters-and-timers docs/pages/counters-overhead.md`
exits 0 at `13 sources, 4865 units examined, 0 findings, 0 skipped`, and that
figure describes the working-tree read of those thirteen files at this tip.
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
