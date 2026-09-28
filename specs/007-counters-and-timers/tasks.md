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
- [X] T008 Implement the provider seam in `include/speedgun-ng/counters_provider.hpp`: C++20 `provider` concept, `provider_iface` registration base (`enumerate(object_sink&) const`, `open(leaf_set, target)` returning a `window_reader`), `window_reader::read_points(point_sink&) noexcept`, per-leaf point + unit + metadata yield shapes; virtual calls confined to setup (FR-011, R-004, contracts/provider-contract.md)
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
- [X] T019 [US1] Implement scope sugar in `include/speedgun-ng/counters_measurement.hpp` (+ `source/counters/plan.cpp` as needed): `scope::start()`/`finish()`/`metric(expr)` over a two-point buffer living in the scope object, semantics identical to the recorder (FR-030); misuse sequences (`metric` before `finish`, `finish` without `start`, double `start`, use-after-finish, registering a composite into a started scope) are tier-3 `SG_REQUIRE` violations (FR-046, spec edge cases)
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
- [X] T036 [US4] Add the `tsc` leaf in `source/counters/clock_provider.cpp`: `__rdtsc` (or `rdtscp` variant per spectre posture), frequency calibrated at system-open from `tsc_khz` sysfs cross-checked against CPUID frequency-invariance data, achieved mode `fast_tsc` plus calibration provenance and the scaled-TSC flag as catalog fields, platforms without a usable TSC omit the leaf (catalog fact, zero API difference); P2 intrinsic justification written at the site (FR-034, R-007; plan Complexity Tracking)
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
- [X] T041 [P] [US5] Write `example/counters_standalone_example.cpp`: public headers + std only, composes an `instructions/cycles`-shape metric over fake+clock sources and drives it in a fixed-iteration per-thread loop with recorder capacity computed from the known iteration count and fold results feeding per-iteration counter inputs (FR-050), runs, prints the folded metric line with ratio and scaled fields, exit 0; add to `example/CMakeLists.txt`; verify the link manifest (`ldd`/`readelf -d`) names `speedgun-ng` alone and grep finds zero third-party includes (SC-001; quickstart 2)
- [X] T042 [US5] Stabilize the provider surface so T039-T041 compile against installed public headers with zero internal access; confirm the giraffe source includes no `source/` header; run `ctest --preset=dev -R counters_provider_ext` and both example targets GREEN; `git status --porcelain source include` clean after the example builds (SC-003)

**Checkpoint**: The provider abstraction is proven from outside the library; the standalone claim is verified with a link manifest.

## Phase 8: User Story 6 - Count hardware events on Linux from a rich catalog (Priority: P3)

**Goal**: `linux_pmu` provider: vendored per-CPU event tables merged with kernel-discovered aliases (kernel wins), CPUID table selection, encoding composed with sysfs `format/` bit layouts, availability probed per entry (`permission_blocked` at paranoid 2), one group read per PMU leader per action, enabled/running as ordinary leaves.

**Independent Test**: `ctest --preset=dev -R counters_pmu` green unprivileged at `perf_event_paranoid=2` (hardware entries `permission_blocked`, clocks/push `countable`); privileged-host developer evidence: named Intel/AMD events described, a group (instructions+cycles) IPC equals the raw-column quotient (quickstart 10).

### Tests for User Story 6 (TDD - write FIRST, verify RED)

- [X] T043 [P] [US6] Write `test/source/counters_pmu_test.cpp`: merge semantics with kernel-wins conflicts and every entry described (scenario 1); CPUID vendor/family/model selects the matching architecture directory, parsed once lazily (scenario 2); an event whose fields exist in neither the vendored table nor kernel format reports `not_encodable`, no partial encoding (scenario 3); at paranoid 2 unprivileged: hardware entries `permission_blocked`, clocks and push `countable`, suite green (scenario 4, SC-002); a group of resolved events samples in syscall mode: one group read per PMU leader delivers members + enabled/running in one action (scenario 5); a multiplexed group: ratio below 1, scaled set, value is the kernel scaled estimate (scenario 6, developer-privileged evidence); compiling over a leaf the catalog reports as not `countable`, or over a target the kernel refuses to open, fails recoverably with the member at fault named, before any hardware read; a plan binds one target, so a target or clock-id mismatch across group members has no spelling (scenario 7, FR-024 single-plan-target design); register in `test/CMakeLists.txt`

### Implementation for User Story 6

- [X] T044 [US6] Create the vendored data tree `external/pmu-events/` (byte-exact path snapshot of kernel `tools/perf/pmu-events` x86 architecture table directories plus `mapfile.csv` at the pinned revision) with the `RECORD` provenance manifest (ref, date, URL, per-file sha256, exclusion list), and add the configure-time gate bracket to `CMakeLists.txt` (recorded gate constant vs `RECORD` ref; configuration fails on drift; yaml-cpp precedent, no git metadata consulted) (FR-043, R-012)
- [X] T045 [US6] Implement `source/counters/linux_pmu/table_parse.cpp`: read CPUID, match `mapfile.csv` regex (first match wins) to the architecture directory, parse that directory's JSON once lazily with vendored simdjson (private seam: these TUs are the sole simdjson includers under `source/counters/`, preserving the 004 privacy-audit boundary) (FR-037/038, R-010)
- [X] T046 [US6] Implement `source/counters/linux_pmu/encode.cpp`: compose JSON semantic `EventCode`/`UMask` with the running kernel's sysfs `format/<field>` bit positions; a required format field missing from the kernel marks the entry `not_encodable`, encoding never half-attempted (FR-037, R-010)
- [X] T047 [US6] Implement `source/counters/linux_pmu/provider.cpp`: enumerate `/sys/bus/event_source/devices/*` (type ids, `format/`, `events/` aliases), merge with vendored entries (kernel-discovered aliases win conflicts), probe availability per entry by `perf_event_open` test-open plus `perf_event_paranoid` read, report `countable`/`permission_blocked`/`not_encodable`/`absent`; Linux-only compile guard, absent cleanly on other platforms behind the identical interface with the reduced catalog (FR-037/039/042, R-010)
- [X] T048 [US6] Implement `source/counters/linux_pmu/group_io.cpp`: syscall-mode window reader - group fds, one `read(PERF_FORMAT_GROUP)` per PMU leader per sampling action delivering all member values plus `time_enabled`/`time_running` into scratch; enabled and running are ordinary cumulative leaves appended to every group's leaf set (FR-026 syscall mode, FR-041, R-010)
- [X] T049 [US6] Extend `source/counters/plan.cpp` group layout: validate shared target and clock identity across group members at construction; mismatch is a recoverable construction error, never a read-time surprise (FR-024, US6 scenario 7); bind thread/cpu targeting at plan open, plan is a per-thread object, multiple plans over one system first-class (FR-031)
- [X] T050 [US6] Run `ctest --preset=dev -R counters_pmu` GREEN on an unprivileged host at paranoid 2 (quickstart 10 verdict; privileged evidence recorded in the PR per the developer-machine protocol)

**Checkpoint**: Real hardware events on Linux from the rich catalog; the suite stays green at paranoid 2 via `permission_blocked` states.

## Phase 9: User Story 7 - Near-single-instruction reads where the platform permits (Priority: P3)

**Goal**: Per-leaf read mode from probe (`fast_tsc`, `fast_rdpmc`, `syscall`, `push_load`), the mapped-page rdpmc protocol, per-plan overhead calibration, and the published fast-vs-syscall regime benchmark.

**Independent Test**: `ctest --preset=dev -R counters_overhead` - on a fast-capable probe-passing host: fast plan tens-of-cycles vs the same plan forced `syscall` in microseconds, distributions min/median/max published side by side; other hosts skip with the failure named (quickstart 12).

### Tests for User Story 7 (TDD - write FIRST, verify RED)

- [X] T051 [P] [US7] Write `test/source/counters_overhead.cpp` as a CTest-registered benchmark executable (`SKIP_RETURN_CODE 2` for probe-gated fast sections; skip reason printed): clock-only plan `sample()` stated nanosecond distribution (min/median/max); fast-mode plan tens-of-cycles regime vs the same plan forced `syscall` microsecond regime published side by side (scenario 6, SC-004); sample-path cost tracks the read sequence with fold cost measured separately off the path (SC-010); achieved modes disclosed per catalog entry, fast where the probe passes / `syscall` where it fails with the reason (scenario 1); register in `test/CMakeLists.txt`

### Implementation for User Story 7

- [X] T052 [US7] Implement `source/counters/linux_pmu/fast_read.cpp`: per-thread `perf_event_open` context, single-page read-only `mmap`; read descriptor implementing the mapped-page protocol in order - seqcount `lock` snapshot + retry, `rmb` fences, `cap_user_rdpmc` capability gate, one-based `index` validity with the stated fallback path when not allowed, `_rdpmc(index - 1)`, kernel offset adjustment, `(val + offset) & 0xFFFFFFFFFFFF` counter-width mask, per-thread same-thread context binding (FR-040, R-011, US7 scenario 2); P2 `reinterpret_cast` at the kernel ABI boundary with written justification at the site + attribution comment citing `jevents/rdpmc.{c,h}` from andikleen/pmu-tools (no file copied) (plan Complexity Tracking)
- [X] T053 [US7] Implement per-leaf mode assignment at plan compile in `source/counters/plan.cpp`: probe mechanism availability, the `perf_user_access` sysctl on affected Intel parts, version, pinning/index validity - achieved mode recorded per leaf and disclosed in the catalog; support probed, never assumed (FR-023, R-011, US7 scenario 1)
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
- [X] T073 Split the contrastive sentence in `source/counters/linux_pmu/table_parse.cpp:18-19` so no unit carries the `X rather than Y` shape, and drop the filler token `actually` from the comment at `include/speedgun-ng/counters_measurement.hpp:147`; both are reviewer-parity defects under `constitution.md:424-425` rather than mechanical prose-lint findings, because `prose_rules.yaml:94` matches a leading space the stripped comment unit does not carry and `prose_gate.py:876-883` exempts a whole unit containing a code span (CRITICAL, Constitution XI.2 and XI.5, `partial`)

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
- [X] T106 Add a `ci-sanitize` build preset and a `ci-sanitize` test preset to the committed `CMakePresets.json` so T065's command and `quickstart.md:114` can run; the file currently declares no `buildPresets` and no `testPresets`, and the `dev` presets must stay in the machine-local `CMakeUserPresets.json` (`constitution.md:531-533`) (MEDIUM, T065, `partial`)
- [X] T107 Read the staging tree when replacing in `tools/pmu_events/update_pmu_events.py:325-336`, where the staging directory is written and never used because the tree is rewritten from the in-memory dict; or drop the staging directory and record that the replace is last-writer-wins from memory (MEDIUM, T059, `partial`)
- [X] T108 Replace the full set of allocation functions in `test/source/counters_noalloc_test.cpp:42-76`, which overrides only the plain `operator new(std::size_t)` and leaves `new[]`, the nothrow form and the aligned forms uncounted, so an array-form allocation on the sample path escapes the count (MEDIUM, SC-005, `partial`)
- [X] T109 Delete or repair `test/source/counters_recorder_test.cpp:107`, which asserts that a full `hard_stop` recorder never wraps, a fact `hard_stop_sample_core` at `source/counters/plan.cpp:318-330` can never violate because it never assigns the flag (MEDIUM, T022, `partial`)
- [X] T110 Add a fake leaf carrying an enabled/running pair and assert the composite ratio product at `source/counters/fold.cpp:130-131`, which no test can reach today because no fake leaf sets `has_ratio_pair` (`source/counters/fake_provider.cpp:133-139`) (MEDIUM, FR-019, `partial`)
- [X] T111 Move the unrecognized-unit failure from `register_provider` to resolution as US1 scenario 6 frames it, or amend the spec; today `source/counters/system.cpp:174-177` rejects at registration, leaving the resolution-side switch at `include/speedgun-ng/counters_system.hpp:119-122` unreachable and `object::counters()` degrading an unmappable unit to `unit::none` at `source/counters/system.cpp:373` (MEDIUM, US1 scenario 6, `partial`)
- [X] T112 Register the P2 coverage exclusion that `plan.md:335` promises in the Complexity Tracking table at `plan.md:363-366`, with its written justification, or drop the claim; no `LCOV_EXCL` marker exists in the counters scope (MEDIUM, plan.md:335, `missing`)
- [X] T113 Own the opaque handles with `std::unique_ptr`: `source/counters/system.cpp:260` wraps `new` where `std::make_unique` applies, and `source/counters/system.cpp:124` leaves the destructor defaulted while nothing deletes `m_impl` declared at `include/speedgun-ng/counters_system.hpp:249`, so one heap block per process is never reclaimed (MEDIUM, Constitution I, `partial`)
- [X] T114 Record the `Counters` section token added to `tools/prose/prose_rules.yaml:23` inside the 007 commit range in `specs/002-prose-commit-lint`, which owns that artifact, or revert it; the gate change at `tools/dbc/dbc_pair_gate.py` is already covered by commit `3c67647` with its `Approved-by` footer (MEDIUM, Constitution IX, `unrequested`)
- [X] T115 Remove the Windows behaviour promise from the `pmu_provider` class doc at `include/speedgun-ng/counters_pmu.hpp:31-33`, which claims a syscall-mode group read per leader, where `:34-36` and `source/counters/linux_pmu/provider.cpp:40-49` state the provider seeds no objects off Linux and `spec.md:312` defers Windows and macOS hardware-PMU providers (MEDIUM, FR-042, `contradicts`)
- [X] T116 Record an explicit decision on the three parallel implementations of one sampling point — `scope`'s own buffer and state at `source/counters/plan.cpp:270-276`, separate from `hard_stop_sample_core` at `:318-330` and `ring_sample_core` at `:332-345` — or fold them onto one implementation; FR-030's one-semantics claim currently rests on tests rather than shared code (MEDIUM, Constitution X.2, `unrequested`)
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


## Phase 13: Fast-Path Verification (OPEN)

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
Phase 12 recorded; `dbc-gate` 135 interfaces with 0 gaps; `prose-lint` 99 sources and
6693 units with 0 findings and 1 skipped. Coverage of the check: 109 requirement keys
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
findings; `prose_gate.py --check prose --mode tree` reports 109 findings over 182
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
all` reports 101 sources, 7246 units, 0 findings, 1 skipped; the coverage gate run
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
`python3 tools/prose/prose_gate.py --check all` exits 0 over 103 sources and 7441 units
with 1 skipped (`hwloc.md`, absent); `--mode tree` reports 182 sources and 108 findings,
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
`:58-73` and `:120-130`, Principle XI.1 scopes the rule to the lines a change touches,
and the tree carries 108 findings of the same class across 182 sources, so a sweep is a
formatting-only change under V on its own.

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
`.specify/memory/constitution.md:250` and `AGENTS.md:43` name `ctest --preset=dev` as the
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

- [ ] T184 Name a `Release` compile in the per-task verification instruction: `.specify/memory/constitution.md:250` and `AGENTS.md:43` both define a task's whole verification as `ctest --preset=dev`, and that preset is `Debug` (`CMakeUserPresets.json:22`), so the optimizer-backed analyses that found the unchecked nullable dereference that kept `ci-ubuntu` from compiling never run in the loop every task is verified through, and three convergence waves reported green gates while the configuration the `test` job builds had never compiled; the instruction must add the `ci-ubuntu` configure and build the `test` job already runs, and the `dev` preset keeps its place as the fast iteration loop because it carries the complete committed set through `ci-linux` (Constitution IX, VIII, `plan.md:33`, T173, `partial`)
