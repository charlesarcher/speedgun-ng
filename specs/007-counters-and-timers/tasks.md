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
- [X] T004 Create the six public header skeletons with doxygen pre/post/invariant contract blocks (dbc-gate pairs documentation with enforcement; bodies may be stubs until their story lands): `include/speedgun-ng/counters.hpp`, `counters_core.hpp`, `counters_provider.hpp`, `counters_system.hpp`, `counters_measurement.hpp`, `counters_fake.hpp` (R-001, Constitution II)

**Checkpoint**: `cmake --preset=dev && cmake --build --preset=dev` green with empty wiring; dbc-gate sees the headers.

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core vocabulary every user story rides on: dimensions, units, catalog/availability/metric-result shapes, provider contract, error type.

**CRITICAL**: No user story work can begin until this phase is complete.

- [X] T005 Implement `template <int T, int C> struct dim` with constexpr exponent arithmetic helpers (identical-tag check, quotient subtraction) in `include/speedgun-ng/counters_core.hpp` (R-003, FR-015/016)
- [X] T006 Implement the `unit` enumeration and the closed `unit -> std::expected<dim, error>` switch in `include/speedgun-ng/counters_core.hpp`: recognized units map (`seconds` to `time^1`, count units to `events^1` per contracts/measurement-contract.md); an unrecognized unit is a resolution error naming the unit, never a guess (FR-017, US1 scenario 6)
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
- [X] T025 [US2] Extend `source/counters/plan.cpp`: arena geometry - one `uint64` array of `capacity` slots per leaf column, column-major SoA, allocated at plan finalization (untimed region); independent buffers per recorder sharing the compiled layout (FR-029, R-005); `hard_stop` bounds check as `SG_REQUIRE_ALWAYS` (present in every configuration including release) beside the write index; ring: power-of-two capacity validated at construction as a tier-2 `std::expected` error, `idx & (cap - 1)` branchless update, one-shot wrapped promotion, dropped count incremented per overwrite (FR-027/028, R-006)
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

- [X] T043 [P] [US6] Write `test/source/counters_pmu_test.cpp`: merge semantics with kernel-wins conflicts and every entry described (scenario 1); CPUID vendor/family/model selects the matching architecture directory, parsed once lazily (scenario 2); an event whose fields exist in neither the vendored table nor kernel format reports `not_encodable`, no partial encoding (scenario 3); at paranoid 2 unprivileged: hardware entries `permission_blocked`, clocks and push `countable`, suite green (scenario 4, SC-002); a group of resolved events samples in syscall mode: one group read per PMU leader delivers members + enabled/running in one action (scenario 5); a multiplexed group: ratio below 1, scaled set, value is the kernel scaled estimate (scenario 6, developer-privileged evidence); plan construction over mismatched targets/clock ids fails recoverably before any hardware read (scenario 7); register in `test/CMakeLists.txt`

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

- [ ] T051 [P] [US7] Write `test/source/counters_overhead.cpp` as a CTest-registered benchmark executable (`SKIP_RETURN_CODE 2` for probe-gated fast sections; skip reason printed): clock-only plan `sample()` stated nanosecond distribution (min/median/max); fast-mode plan tens-of-cycles regime vs the same plan forced `syscall` microsecond regime published side by side (scenario 6, SC-004); sample-path cost tracks the read sequence with fold cost measured separately off the path (SC-010); achieved modes disclosed per catalog entry, fast where the probe passes / `syscall` where it fails with the reason (scenario 1); register in `test/CMakeLists.txt`

### Implementation for User Story 7

- [X] T052 [US7] Implement `source/counters/linux_pmu/fast_read.cpp`: per-thread `perf_event_open` context, single-page read-only `mmap`; read descriptor implementing the mapped-page protocol in order - seqcount `lock` snapshot + retry, `rmb` fences, `cap_user_rdpmc` capability gate, one-based `index` validity with the stated fallback path when not allowed, `_rdpmc(index - 1)`, kernel offset adjustment, `(val + offset) & 0xFFFFFFFFFFFF` counter-width mask, per-thread same-thread context binding (FR-040, R-011, US7 scenario 2); P2 `reinterpret_cast` at the kernel ABI boundary with written justification at the site + attribution comment citing `jevents/rdpmc.{c,h}` from andikleen/pmu-tools (no file copied) (plan Complexity Tracking)
- [ ] T053 [US7] Implement per-leaf mode assignment at plan compile in `source/counters/plan.cpp`: probe mechanism availability, the `perf_user_access` sysctl on affected Intel parts, version, pinning/index validity - achieved mode recorded per leaf and disclosed in the catalog; support probed, never assumed (FR-023, R-011, US7 scenario 1)
- [ ] T054 [US7] Implement per-plan overhead calibration in `source/counters/plan.cpp`: after finalization, repeatedly run the plan's `sample()` over an empty workload, store min/median/max ns on the plan, expose `sample_overhead_ns_min/median/max()`; fold windows state their endpoints' sample cost (FR-032, R-014)
- [ ] T070 [P] [US7] Write the cross-thread trap test extending `test/source/counters_trap_fixture.cpp` and its checker: recorder and plan used from a non-binding thread abort in the dev/CI build (TDD: RED before T055 implements the binding check); register in `test/CMakeLists.txt` (FR-031, FR-047; C-MEA-6; plan Test Plan threading row)
- [X] T055 [US7] Cross-thread misuse contract (test T070 first): recorder/plan/push use from a non-binding thread is a tier-3 `SG_REQUIRE` beside the existing bounds check (cached `thread::id` compare); fast contexts bind same-thread (FR-031/040, US7 scenarios 4-5; R-015); fast-mode off-CPU staleness stays disclosed through the enabled/running ratio leaves in every mode (FR-041, US7 scenario 4)
- [ ] T056 [US7] Publish `docs/pages/counters-overhead.md` (following the `docs/pages/dbc-overhead.md` precedent) with the reference-host distributions: fast and syscall regimes side by side, min/median/max (Principle VII, SC-004)

**Checkpoint**: The performance promise is measured and published; every host reports either its regimes or its skip reason.

## Phase 10: User Story 8 - Keep the vendored event tables current (Priority: P3)

**Goal**: First-class upgrade utility `tools/pmu_events/update_pmu_events.py` with `--to <ref>` fetch/strip/validate/replace/record/bump/digest and `--check` drift verification (no network), wired into CI with synthetic fixtures.

**Independent Test**: `python3 tools/pmu_events/update_pmu_events.py --check` exits 0 clean / 1 naming the drifted file; `ctest -R pmu_events_check` passes the synthetic fixture trees in both directions (quickstart 11).

### Tests for User Story 8 (TDD - fixtures FIRST, verify RED)

- [X] T057 [P] [US8] Create `test/pmu-events-gate-fixture/`: tiny synthetic table trees - one clean (check passes), one with a corrupted file hash (check exits 1 naming the file), one with a gate/RECORD disagreement (exits 1); wire both directions into CTest as `pmu_events_check` fixtures following the `prose_gate_fixtures` pattern in `test/CMakeLists.txt` (FR-045, US8 scenario 6; the `tools/prose/` precedent)

### Implementation for User Story 8

- [X] T058 [US8] Implement `tools/pmu_events/update_pmu_events.py --check`: verify every tree file's sha256 against `RECORD`, verify the gate constant (root `CMakeLists.txt` bracket) against the recorded ref, exit 1 naming the file on any drift, zero network access ever (FR-045, US8 scenarios 1-2/5, R-013)
- [X] T059 [US8] Implement `--to <kernel-ref>` mode in `tools/pmu_events/update_pmu_events.py`: fetch the `tools/perf/pmu-events` path archive from kernel.org cgit (documented GitHub mirror fallback), strip to the in-scope x86 table directories plus `mapfile.csv` recording the explicit exclusion list, validate every file parses (json) and passes schema sanity (`EventName`/`EventCode` present, mapfile regexes compile), replace byte-exact via a staging directory (replace-last: a failed validation leaves the tree untouched), rewrite `RECORD` (ref, date, URL, per-file sha256, exclusions), bump the gate constant (single bump point), print the per-architecture old-to-new event-count digest (FR-044, US8 scenarios 3-4, R-013)
- [X] T060 [US8] Add the CI step to `.github/workflows/ci.yml`: run `update_pmu_events.py --check` (no network) in the existing test job; counters tests ride every existing test job unchanged; no gate weakened (Principle VIII, FR-045, US8 scenario 5)
- [X] T061 [US8] Add the `pmu-events` re-pinning section to `README.md` beside the existing vendored-dep sections (what the gate is, how to re-pin via `--to`, that `--check` runs in CI) (FR-043/044 documentation, plan Project Structure)

**Checkpoint**: Table currency is a first-class maintenance operation; drift is a red build without network.

## Phase 11: Polish & Cross-Cutting Concerns

**Purpose**: Whole-feature gates and documentation close-out (Principle VIII; quickstart 13).

- [ ] T062 [P] Run `cmake --build build/dev -t dbc-gate` clean: every new public interface in `include/speedgun-ng/counters*.hpp` pairs doxygen contracts with runtime enforcement
- [ ] T063 [P] Run `cmake -P cmake/prose-lint.cmake` (prose-lint) clean over all added prose: specs, README section, docs page (Principle XI)
- [ ] T064 [P] Run `cmake --build build/dev -t format-check` clean; formatting-only fixes in a separate commit (Principle V)
- [ ] T065 [P] `cmake --preset=ci-sanitize && cmake --build --preset=ci-sanitize && ctest --preset=ci-sanitize` clean: group-fd lifetimes and the arena under ASan/UBSan (plan "Determinism and regression")
- [ ] T066 [P] Coverage preset: 100% line/branch/DBC gates hold for the `include/speedgun-ng/counters*` and `source/counters/` additions; `SG_*` macro lines excluded via the existing `--omit-lines` mechanism; the `SG_REQUIRE_ALWAYS` bounds abort branch covered by the out-of-process trap pair (Principle VI)
- [ ] T067 Verify `consumer-release` job semantics: the release artifact carries contract code only at the spec-mandated always-on `hard_stop` bounds site (dbc facility `SG_*_ALWAYS` registry distinguishes it) (plan gate-set note, FR-027)
- [ ] T068 Document the cadence idiom in `include/speedgun-ng/counters_measurement.hpp` header docs: chunked sampling every K iterations, capacity `N/K + 1`, `fold_pairs` for the per-interval series, the K=1 observer-effect cost stated numerically from the plan calibration; plus the 2^53 exactness note (FR-032/048, E-09; Principle IV)
- [ ] T069 Run quickstart.md sections 1-13 end to end as final validation; record the verdicts against the SC-001..SC-010 index in the PR description (Constitution III/IX)

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

