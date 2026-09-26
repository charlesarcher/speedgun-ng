# Implementation Plan: Standalone Counters Library

**Branch**: `007-counters-and-timers` | **Date**: 2026-09-25 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/007-counters-and-timers/spec.md`, grounded in the closed design journal [sg_counters.md](sg_counters.md) (A1–A29, resolved scope statement).

**Artifacts**: [research.md](research.md) · [data-model.md](data-model.md) · [quickstart.md](quickstart.md) · [contracts/system-contract.md](contracts/system-contract.md) · [contracts/provider-contract.md](contracts/provider-contract.md) · [contracts/measurement-contract.md](contracts/measurement-contract.md)

## Summary

007 ships the measurement primitives layer: a standalone counters library where the system is a tree of named countable objects, every object owns a catalog of named described counters, counters compose under a compile-time `time^T x events^C` dimension algebra, a recorder's `sample()` appends one column of cumulative `uint64` points on the hot path, and composites fold recorded point columns into metrics later, with ratio disclosure and per-leaf provenance structurally attached ([data-model.md](data-model.md)). One concept covers timers and counters alike: a timer is a counter whose events are clock ticks (A8). The organizing principle is the provider contract (A22): four shipped providers (clock, push, fake, linux_pmu) plus an out-of-tree giraffe example prove that everything below the contract (kernel clocks, hot-path increments, vendored-table PMU reads, scripted test data) is an interchangeable implementation detail, and the core vocabulary carries no platform terms (FR-010).

The engineering spine is the hard construction/read split (A19, FR-021, FR-026): name resolution, dimension checking, group layout, mode probing, arena allocation, and fold-program compilation all happen once in the untimed region; `sample()` is one flat pass over compiled read descriptors (`noexcept`, zero allocation, zero lock), landing in the tens-of-cycles regime where the platform permits (`fast_tsc`, `fast_rdpmc` via the mapped-page protocol) with `syscall` (one group read per PMU leader, vDSO clocks) as the fallback, and `push_load` in-instruction by construction. Folds run whenever the caller asks, over recorded columns only, and a scope (`start`/`finish`/`metric`) is exactly a two-point recorder (FR-030). Overflow is a compile-time policy type (`hard_stop` default with always-enforced bounds; `ring` opt-in, branchless, drop-accounted) chosen through one constexpr-tag factory with CTAD, so call sites carry no templates (FR-025..FR-029).

The linux_pmu provider consumes a new vendored data tree: `external/pmu-events`, a byte-exact path snapshot of the kernel's `tools/perf/pmu-events` x86 tables plus `mapfile.csv`, under a configure-time version gate in the house tripwire pattern, with `tools/pmu_events/update_pmu_events.py` as the first-class upgrade and `--check` utility (FR-043..FR-045, R-012, R-013). Table semantics (JSON event codes, descriptions, units) compose with the running kernel's sysfs `format/` bit layouts; availability is probed per entry, so CI at `perf_event_paranoid=2` stays green on fake, clock, and push alone while hardware entries report `permission_blocked` (FR-039, SC-002). 007 is standalone in the binding sense (FR-049, FR-050): public headers plus std compile, run, and fold a metric; zero benchmarking-framework code appears anywhere in the feature.

## Technical Context

**Language/Version**: C++23 (`CMAKE_CXX_STANDARD=23` in presets, `cxx_std_23` PUBLIC on the target, `CMAKE_CXX_EXTENSIONS=OFF`). CMake >= 3.20.

**Primary Dependencies**: zero new runtime dependencies (constitution Additional Constraints). The vendored simdjson (specs/004, private, statically absorbed) parses the PMU event-table JSON inside `source/counters/linux_pmu/` only. New vendored data (not code): `external/pmu-events` (R-012). hwloc, HdrHistogram_c, yaml-cpp, zlib stay untouched by 007.

**Storage**: N/A at runtime (fixed-capacity point arenas, all construction-time). One committed data tree: `external/pmu-events` plus its `RECORD` provenance manifest.

**Testing**: the repo's frameworkless convention: hand-rolled `check()`/`fail()` executables in `test/source/`, plain `add_test` registration in `test/CMakeLists.txt`; cross-process trap-fixture pairs for abort semantics; `test/compile-fail/` for negative compilation; python gate fixtures for the table tool. No test framework may be added (the dependency-scan test forbids it).

**Target Platform**: Linux (GCC/Clang) is the full feature (PMU provider, fast modes). macOS (AppleClang, developer-local per the constitution deferral) and Windows (MSVC suspension in force per 2.7.0) carry the reduced catalog (clocks, push, fake) behind the identical interface, zero API difference (FR-042).

**Project Type**: C++ library feature: the first substantive public surface of `speedgun-ng` beyond the DBC facility, delivered through the existing single library target (`speedgun-ng_speedgun-ng`, alias `speedgun-ng::speedgun-ng`), plus one vendored data tree, one python tool, examples, and CI wiring.

**Performance Goals**: designated critical paths: `recorder::sample()` (tens-of-cycles regime for a fast-mode plan where the probe passes; `noexcept`, zero allocation, zero lock) and push `add()` (plain non-atomic increment). Budgets published as min/median/max distributions per platform (VII): fast and syscall regimes side by side, clock-only ns distribution stated (SC-004, SC-010, `docs/pages/counters-overhead.md`; the `docs/pages/dbc-overhead.md` precedent).

**Constraints**: read path free of expression tree, dynamic dispatch, name lookup (FR-022); bounds check `SG_REQUIRE_ALWAYS` under `hard_stop` (memory safety never semantic-gated); per-thread plans/recorders with binding checked in dev/CI; dimensions erased before the buffer; catalog immutable after open; standalone example link manifest names this library alone (FR-049).

**Scale/Scope**: 6 new public headers (`include/speedgun-ng/counters*.hpp`), `source/counters/` (~8 TUs plus provider-private internals), 4 built-in providers, ~11 new test executables plus compile-fail cases and fixtures, 2 example targets, 1 python tool with fixtures, `external/pmu-events` data tree plus gate bracket, CI step, README re-pinning section, one docs overhead page. Infrastructure: root `CMakeLists.txt`, `test/CMakeLists.txt`, `cmake/lint.cmake` (glob reach into `source/counters/`), `.github/workflows/ci.yml`.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-checked after Phase 1 design: still passing.*

| Principle | Status | Notes |
|---|---|---|
| I. Standard-First Coding | PASS | C++23, extensions off. P0 invoked (documented, designated in spec FR-026/FR-035 and [contracts/measurement-contract.md](contracts/measurement-contract.md)): `sample()` and push `add()` carry the P0 techniques (register-resident push load, branchless ring mask, dispatch-free flat read loop). Two P2 exceptions anticipated and recorded here for written justification at the site: x86 intrinsics (`__rdtsc`, `_rdpmc`) and the `reinterpret_cast` of the mmap'd perf user-access page, both confined inside provider implementations where the probe plus kernel ABI make them sound (FR-034, FR-040; R-007, R-011). |
| II. Design By Contract | PASS | Every new public interface carries doxygen `\pre`/`\post`/`\invariant` plus runtime enforcement (dbc-gate covers `include/speedgun-ng/` automatically). Tier mapping per FR-046 (R-002): dimensions and result shape in types; `std::expected<T, error>` for resolution/registration/construction-input failures; `SG_REQUIRE` for misuse sequences, `SG_REQUIRE_ALWAYS` for the `hard_stop` bounds. Contracts state each rule once (header doc pairs one enforcement site). |
| III. R-DCUT | PASS | spec → plan (this artifact set, logical and physical views, test plan) → tasks → code+tests. TDD mode recorded in the Test Plan. |
| IV. Documentation | PASS | Full doxygen on the six public headers (docs target picks them up); the unit-to-dimension switch, the 2^53 exactness note, the cadence idiom, and the fold-endpoint-cost statement are documented behavior per spec, per Principle IV. The overhead page and README re-pinning section complete the written surface. |
| V. Style and Formatting | PASS | `.clang-format` governs new files; `cmake/lint.cmake`'s glob gains reach into `source/counters/` (verify at implement; the hwloc/simdjson gate subdirs prove the pattern the glob needs to match). Formatting-only changes commit separately. |
| VI. Test-Backed Code | PASS | Tests ship with every component (Test Plan table covers US1–US8, all FR tiers, SC-001..SC-010). 100% line/branch/DBC gates apply to the new `include/`+`source/` code; contract-macro lines stay excluded by the existing `--omit-lines` mechanism; the fake provider makes fold/provenance exactness testable with zero tolerance (SC-006). Measurement correctness (wrap, drop accounting, ratio products, endpoint cost) is the safety property of this feature and carries the fixtures. |
| VII. Performance Discipline | PASS | Critical paths designated and documented (Constraints). The feature ships the measurement artifacts: per-plan overhead calibration (FR-032), probe-gated fast/syscall regime benchmark with distributions published side by side (`docs/pages/counters-overhead.md`, following the `docs/pages/dbc-overhead.md` precedent). The open deferral (baseline infrastructure absent) stays open: this plan delivers baselines as published data; regression-gate wiring lands with the baseline spec. |
| VIII. CI Quality Gates | PASS: extended; no gate weakened | Existing gates untouched; one additive CI step (`update_pmu_events.py --check`, no network, FR-045). Suite green unprivileged at paranoid 2 (SC-002); sanitizer and coverage presets unchanged; `consumer-release` semantics preserved (the always-on bounds check is deliberate, spec-mandated, and release-artifact-verified as an exception to semantic-gating). Windows MSVC: suspension in force (2.7.0), and the reduced-catalog design keeps the port unprecluded; macOS: developer-local evidence per the 2.8.0 deferral. |
| IX. Spec-Driven Development | PASS | This artifact set under `specs/007-counters-and-timers/`; `tasks.md` via `/speckit.tasks` next. Public API work: no bypass possible or attempted. |
| X. Anti-Slop | PASS | The provider abstraction answers to the spec's organizing principle (A22, FR-011/012): four implementations plus an out-of-tree proof exist. No knobs beyond spec-named ones: one factory, two policies, no `recorder_opts` struct until a second knob appears (A27). The scope class is sugar over the recorder (FR-030): one mechanism, two spellings. `MetricExpr` rows ship as dead catalog data for 008 (A14); no string-grammar machinery here. |
| XI. Discourse and Prose | PASS | Generated prose in this directory follows XI; prose-lint covers `specs/` over the PR range. |

**Gate-set note (Principle VIII)**: nothing weakens. The single build-configuration addition is the vendored-data gate bracket (configure-time tripwire, yaml-cpp precedent) and the CI check step. The one deliberate always-on contract code in release (`hard_stop` bounds) is spec-mandated (FR-027) and lands in the dbc facility's `SG_*_ALWAYS` registry, so the release-artifact verification distinguishes it correctly.

## Project Structure

### Documentation (this feature)

```text
specs/007-counters-and-timers/
├── plan.md                        # This file (/speckit.plan)
├── spec.md                        # Feature spec (clarifications session 2026-09-25)
├── sg_counters.md                 # Closed design journal (authoritative record)
├── research.md                    # Phase 0 output (R-001 through R-015)
├── data-model.md                  # Phase 1 output (E-01 through E-11)
├── quickstart.md                  # Phase 1 output (validation runs, SC mapping)
├── contracts/
│   ├── system-contract.md         # Phase 1: system, objects, catalog, resolution
│   ├── provider-contract.md       # Phase 1: provider obligations, point yield, read modes
│   └── measurement-contract.md    # Phase 1: algebra, plan, recorder, folds, scope
└── tasks.md                       # Phase 2 output (/speckit.tasks, NOT created here)
```

### Source Code (repository root)

```text
include/speedgun-ng/
├── counters.hpp                   # NEW umbrella: the documented single include (R-001)
├── counters_core.hpp              # NEW dim<T,C>, unit + closed mapping, availability,
│                                  #   read_mode, catalog_entry, metric_result, error
├── counters_provider.hpp          # NEW provider concept + registration base,
│                                  #   window_reader, point/metadata shapes (FR-011)
├── counters_system.hpp            # NEW system, object, selection, resolution (FR-001..009)
├── counters_measurement.hpp       # NEW counter<D>/expression<D>, compile<plan>,
│                                  #   recorder factory + handle, folds, scope sugar,
│                                  #   push counter handle (inline add(), FR-035)
└── counters_fake.hpp              # NEW fake provider control surface (FR-036)

source/counters/                   # NEW implementation dir (root CMakeLists owns
│   │                              #   every addition via target_sources PRIVATE)
├── system.cpp                     # object tree, registration, resolution, selection,
│   │                              #   did-you-mean diagnostics
├── clock_provider.cpp             # CLOCK_MONOTONIC / _THREAD_CPUTIME_ID /
│   │                              #   _PROCESS_CPUTIME_ID leaves; tsc leaf: sysfs
│   │                              #   tsc_khz + CPUID calibration, scaled flag (R-007)
├── push_provider.cpp              # confinement bookkeeping; counter creation
├── fake_provider.cpp              # scripted deterministic point sequences (R-009)
├── plan.cpp                       # compile(): slots, group layout + target/clock
│   │                              #   validation, mode assignment, arena geometry,
│   │                              #   fold programs, overhead calibration (FR-032)
├── fold.cpp                       # modular deltas, ratio products, drop-aware fold,
│   │                              #   series fold helpers (template entry points inline)
├── detail/                        # NEW provider-private headers (never installed)
└── linux_pmu/                     # NEW Linux-only provider (FR-037..FR-040)
    ├── provider.cpp               # enumerate: vendored tables x sysfs merge, kernel
    │                              #   wins; availability probe (FR-037, FR-039)
    ├── table_parse.cpp            # sole vendored-table JSON reader via private simdjson
    │                              #   (specs/004 seam); lazy, one CPUID-matched dir
    ├── encode.cpp                 # JSON semantic codes composed with sysfs format/
    │                              #   bit layouts; missing field -> not_encodable
    ├── group_io.cpp               # syscall mode: group fds, one read(PERF_FORMAT_GROUP)
    │                              #   per PMU leader per action; enabled/running leaves
    └── fast_read.cpp              # mapped-page protocol (R-011): mmap page access
                                   #   (P2: reinterpret_cast at the ABI boundary,
                                   #   justification written at site), seqcount retry,
                                   #   cap/index gates, offset, 48-bit mask, attribution

external/pmu-events/               # NEW vendored data tree: byte-exact path snapshot of
│   │                              #   kernel tools/perf/pmu-events x86 dirs + mapfile.csv
│   └── RECORD                     # NEW provenance: ref, date, URL, per-file sha256,
                                   #   exclusions (FR-044); verified by --check

CMakeLists.txt                     # MODIFY: target_sources for source/counters/**;
                                   #   pmu-events configure-time gate bracket (gate
                                   #   constant vs RECORD; yaml-cpp precedent, R-012)

tools/pmu_events/
└── update_pmu_events.py           # NEW: --to <ref> fetch/strip/validate/replace/
                                   #   RECORD/gate-bump/digest; --check drift exit 1
                                   #   (FR-044/045, R-013)

example/
├── counters_standalone_example.cpp # NEW: public headers + std only; composes and
│                                   #   folds a metric over clock+fake (SC-001)
└── counters_giraffe_example.cpp    # NEW: out-of-tree provider ~100 lines (US5, SC-003)

test/source/                       # NEW executables (frameworkless check/fail):
├── counters_core_test.cpp          # dims, unit switch, error shape
├── counters_fake_test.cpp          # US1 spine: resolution, folds exact, provenance,
│                                   #   repeated .metric() with frozen reads (SC-006/008)
├── counters_recorder_test.cpp      # capacity/ring/wrap/drop/fold_pairs/range tiers
├── counters_objects_test.cpp       # US3: tree, alias, selection, fan-out, duplicates
├── counters_clock_push_test.cpp    # US4: tolerance bands, exact push totals, modes
├── counters_provider_ext_test.cpp  # US5 in-suite conformance: mini provider, no
│                                   #   internals (FR-012)
├── counters_pmu_test.cpp           # US6: paranoid-2 permission_blocked vs
│                                   #   countable; merge + encodability states
├── counters_trap_fixture.cpp       # NEW aborting fixture (registered indirectly)
├── counters_trap_checked_test.cpp  # NEW tier-3 traps incl. hard_stop overrun in a
│                                   #   release-configured checker (FR-027)
├── counters_noalloc_test.cpp       # SC-005: counting operator new/delete around
│                                   #   sample()
└── counters_overhead.cpp           # SC-004/SC-010 benchmark; probe-gated sections,
                                    #   CTest SKIP_RETURN_CODE, distributions printed

test/compile-fail/                 # MODIFY: dimension-violation cases (bytes+monotonic,
                                   #   mismatched tags) into the existing harness
test/pmu-events-gate-fixture/      # NEW synthetic table trees: check pass + drift fail
test/CMakeLists.txt                # MODIFY: register every executable above + the
                                   #   fixture scripts (plain add_test convention)
cmake/lint.cmake                   # MODIFY (verify): format glob reaches source/counters/
.github/workflows/ci.yml           # MODIFY: pmu-events --check step (no network); the
                                   #   counters tests ride every existing test job
docs/pages/counters-overhead.md    # NEW: published distributions (lands with measurements)
README.md                          # MODIFY: pmu-events re-pinning section beside the
                                   #   existing vendored-dep sections
```

**Structure Decision**: the existing single-library layout stands unchanged. Public surface is flat `include/speedgun-ng/counters*.hpp` per house convention (R-001); implementation lives in `source/counters/` attached through the root CMakeLists `target_sources` pattern the vendored gate dirs already establish; the vendored input is data under `external/` (outside the compiled-tree conventions, gated at configure time); the tool follows `tools/prose/` precedent; examples join `example/` under `add_example()`; tests follow the flat `test/source/*_test.cpp` plus trap-pair plus compile-fail conventions exactly. The umbrella `speedgun-ng.hpp` stays untouched: 007 advertises `counters.hpp` as the standalone entry point (FR-049).

---

## Design: Logical View

*What the feature is and how it behaves: one ontology (points, deltas, folds), one seam (providers), two phases (compile, sample; fold later).*

### Class diagram (public surface)

```mermaid
classDiagram
  direction LR
  class provider_iface { <<registration base>>
    +enumerate(object_sink&) const
    +open(leaf_set, target) window_reader }
  class window_reader {
    +read_points(point_sink&) noexcept }
  class system {
    +local() system&
    +register_provider(p) expected~void,error~
    +object(path) expected~object&,error~
    +objects(kind, filters) object_range }
  class object {
    +path() string_view canonical
    +alias() optional~string_view~
    +kind() string_view
    +counters() counter_catalog
    +counter(name) expected~resolved_leaf,error~ }
  class catalog_entry {
    +name, description
    +unit to dim
    +availability
    +read_mode achieved }
  class "counter~D~" as counter { <<resolved leaf>> }
  class "expression~D~" as expression {
    +operator+ - / scalar
    +fold(rec,i,j) metric_result
    +fold_pairs(rec) series
    +raw(obj,leaf) points_view }
  class plan {
    +recorder(cap) handle~hard_stop~
    +recorder(cap, ring) handle~ring~
    +overhead min/median/max }
  class "recorder_handle~P~" as recorder {
    +sample() noexcept }
  class metric_result {
    +value, running_ratio, scaled }
  class scope {
    +start() +finish() +metric(expr) }
  system "1" *-- "*" object : tree
  object "1" *-- "*" catalog_entry
  catalog_entry ..> counter : resolution
  expression o-- counter : leaves
  plan ..> expression : fold programs
  recorder --> plan : arena buffer
  scope ..> recorder : two-point sugar
  expression ..> recorder : fold
  provider_iface <|.. clock_provider
  provider_iface <|.. push_provider
  provider_iface <|.. fake_provider
  provider_iface <|.. linux_pmu_provider
  provider_iface ..> window_reader : open
```

### Sequence: one measurement window (syscall-mode PMU + push + clock)

```mermaid
sequenceDiagram
    participant U as user code (thread T)
    participant R as recorder_handle
    participant RD as compiled read descriptors
    participant G as PMU group leader (one per PMU)
    participant C as vDSO clock / push cell

    Note over U,G: setup region: compile(), recorder(cap), binding to T
    U->>R: sample()
    R->>RD: flat loop, no dispatch, no lookup
    RD->>G: read(PERF_FORMAT_GROUP): all member points + enabled + running
    RD->>C: clock_gettime points; plain load of push cell
    Note over RD: one column appended: FR-047 invariant holds
    U->>U: work
    U->>R: sample()
    U->>U: later, any number of times, off the measurement path:
    U->>U: expr.fold(rec, 0, 1) -> value + ratio product + scaled flag
    U->>U: expr.raw("package-1/core-3","instructions") -> provenance
```

### State: recorder lifecycle (per policy)

```mermaid
stateDiagram-v2
    [*] --> empty : factory allocates columns (untimed region)
    empty --> filling : sample()
    filling --> full : head reaches capacity
    full --> ABORT : hard_stop sample past capacity (SG_REQUIRE_ALWAYS, every config)
    full --> wrapped : ring sample (idx &= cap-1; dropped++)
    wrapped --> wrapped : further samples, branchless
    note right of wrapped : folds over wrapped consult dropped (FR-028)
```

The system's own transition closes provider registration at open and freezes the catalog (FR-009); plan binding to a thread or cpu happens at open and never moves (FR-031). Scope misuse sequences (`metric` before `finish`, double `start`, use-after-finish) are terminal contract violations in dev/CI (FR-046), the A6 tier assignment applied to the two-point-recorder sugar.

### Cross-cutting correctness promises

1. One window, one truth: per-column `SG_INVARIANT` that all leaf values came from one sampling action under one binding (FR-047); cross-metric agreement is structural (spec promise 1).
2. No number without provenance: `metric_result` fields cannot be omitted; composites expose raw columns with full E-10 records (FR-019, FR-020; spec promise 2).
3. Availability under privilege pressure: availability probing at paranoid 2 reports `permission_blocked` and the suite stays green (FR-039, SC-002).

---

## Design: Physical View

*Where it lives: files, targets, link relationships, and the public API surface added.*

### Files and their duties

| Path | Duty | Requirements |
|---|---|---|
| `include/speedgun-ng/counters*.hpp` (6 files) | Entire public surface; doxygen contracts; inline hot-path pieces (push `add()`, `sample()` core loop) | FR-001..FR-035, FR-046..FR-050; R-001 |
| `source/counters/{system,plan,fold}.cpp` | Tree/resolution, plan compile + arena + calibration, fold kernels | FR-008, FR-018..FR-024, FR-032 |
| `source/counters/{clock,push,fake}_provider.cpp` | Three built-in providers | FR-033..FR-036; R-007..R-009 |
| `source/counters/linux_pmu/` (5 TUs) | The rich Linux backend: table merge, encode, probe, group I/O, fast read | FR-037..FR-041; R-010, R-011 |
| `external/pmu-events/` + `RECORD` | Vendored event-table data + provenance | FR-037, FR-038, FR-043, FR-044; R-012 |
| `CMakeLists.txt` | target_sources; configure-time data gate bracket | FR-043; R-012 |
| `tools/pmu_events/update_pmu_events.py` | Upgrade + `--check` | FR-044, FR-045; R-013 |
| `example/counters_{standalone,giraffe}_example.cpp` | SC-001 standalone proof in the FR-050 fixed-iteration loop shape; SC-003 provider extensibility proof | FR-012, FR-049, FR-050 |
| `test/source/counters_*.cpp`, `test/compile-fail/`, `test/pmu-events-gate-fixture/` | Full test plan coverage (next section) | SC-001..SC-010 |
| `.github/workflows/ci.yml`, `README.md`, `docs/pages/counters-overhead.md`, `cmake/lint.cmake` | Gate wiring, re-pinning doc, published budgets, format reach | FR-045, FR-032; VII, V |

### Build targets and link relationships

```mermaid
graph LR
    L[speedgun-ng_speedgun-ng] -- target_sources PRIVATE --> C[source/counters/**]
    C -- PRIVATE BUILD_INTERFACE --> SJD[simdjson::simdjson<br/>(linux_pmu table parse only,<br/>specs/004 privacy preserved)]
    PD[external/pmu-events data] -- configure-time gate --> L
    X1[example: standalone] -- PUBLIC include + link<br/>speedgun-ng::speedgun-ng --> L
    X2[example: giraffe] -- public headers only --> L
    T[tests: counters_*] -- speedgun-ng::speedgun-ng --> L
```

Key properties, each traced: the public link surface stays `speedgun-ng::speedgun-ng` alone (FR-049; the standalone example's manifest is the evidence); simdjson remains private to `source/counters/linux_pmu/table_parse.cpp`, preserving the 004 privacy audits unchanged; the vendored tree is data (no target, no link edge), gated at configure; `counters.hpp` does not enter `speedgun-ng.hpp`, keeping 007's standalone claim visible in the include graph; export uses the existing `generate_export_header` machinery (`SPEEDGUN_NG_EXPORT` on non-template public classes).

### Public API surface added

Namespace `sg::counters`: `system`, `object`, `catalog_entry`, `unit`, `availability`, `read_mode`, `error`, `dim<T,C>`, `counter<D>`, `expression<D>`, `metric_result`, `plan`, `recorder` factory + `recorder_handle<P>`, `hard_stop`/`ring` tags, `scope`, `push counter` handle, `fake_provider`, `provider_iface` + `provider` concept, `compile()`. This is the entire contract surface of the feature ([contracts/](contracts/)); nothing else becomes public.

---

## Test Plan

*Principle III/VI: tests accompany the component. Verification is binary throughout (X.4). Quickstart sections 1–13 are the runnable form.*

### Execution mode: TDD (recorded per Principle III)

TDD applies at every seam that a test can fail first: dimension compile-fail cases (written against the pre-implementation headers: red), fake-provider fold exactness fixtures including the crafted `2^64` wrap and drop accounting (hand-computed expected values committed with the tests, red before the fold kernels), recorder policy traps (red before the arena), and the giraffe/mini-provider conformance tests (red before the provider base stabilizes). The table-tool fixtures (`--check` pass/drift) precede the tool. Probe-gated benchmark sections arrive with their skip paths (skip-before-measure, measure-where-capable).

### Coverage strategy

- All new public code sits inside the coverage extraction globs (`include/speedgun-ng/*`, `source/*`); the 100% line/branch/DBC gates apply unchanged. `source/counters/detail/` internals are extracted and measured like any source file.
- Contract-macro lines drop via the existing `--omit-lines` filter; the `SG_REQUIRE_ALWAYS` bounds site is exercised by the release-configured trap checker (its abort branch covered by the out-of-process fixture pair).
- Linux-only provider code compiles (and is measured) on Linux CI; on other platforms it is excluded from the build, so the reduced-catalog path carries the macOS/Windows developer evidence.
- Fast-mode hardware reads are probe-gated: hosts failing the probe skip with the reason named (CTest `SKIP_RETURN_CODE`), the skip reports, and the suite stays green everywhere (SC-002).
- Coverage of probe-gated and privileged code: the mapped-page protocol logic (seqcount retry, capability/index gates, offset, width mask) and the fold ratio arithmetic are implemented as pure functions over injected page and index inputs, so CI covers them with synthetic-page fixtures without privileges. The residual kernel glue (`mmap`, `perf_event_open`, `rdpmc` execution) runs on the fast-capable, probe-passing developer host; its CI-unreachable lines carry a recorded P2 coverage exclusion with written justification (VI, VIII: the exception is recorded, nothing weakens).
- The frameworkless convention holds: the dependency-scan test keeps zero external test deps.

### Scenario and check mapping

| US / FR / SC | Check (test / quickstart §) | Pass condition |
|---|---|---|
| US1 / FR-005/008/015-021 / SC-006, SC-008 | `counters_core_test`, `counters_fake_test`, `counters_compile_fail` (§3, §4) | Resolution carries name/desc/unit/avail; near-miss diagnostics; `a/b` tag arithmetic; `bytes+monotonic` fails compile; folds equal hand-computed quotients incl. ratio fields; ten `.metric()` calls identical with provider reads frozen; unknown unit names the unit; raw view complete |
| US2 / FR-013/018/025-029 / SC-006, SC-010 | `counters_recorder_test`, traps (§3, §5) | Construction allocates all columns; hard_stop overrun aborts incl. release config; ring mask, wrapped+dropped recorded, non-power-of-two fails construction recoverably; crafted wrap delta exact; `fold_pairs` per-interval series; bad ranges trap (the push-decrement trap lands with US4) |
| US3 / FR-001-004, FR-008 / SC-007 | `counters_objects_test` (§9) | Enumeration fields complete; alias and canonical resolve to one object; selection exact; cross-object fold from one action; fan-out IPC reconciles per core against shared total; duplicate path/name recoverable |
| US4 / FR-033-035 / SC-002 | `counters_clock_push_test` (§7) | Three clocks positive, mutual tolerance on CPU-bound work; `add(1000)` folds exactly 1000 with plain non-atomic ops; fold-time push decrement traps (US4 scenario 6); `bytes/monotonic` rate + disclosure; catalog lists modes; tsc provenance/scaled flag on calibrated hosts |
| US5 / FR-012, FR-049 / SC-001, SC-003 | giraffe example + `counters_provider_ext_test` (§2, §6) | Out-of-tree provider registers through public headers; `honks/monotonic` folds from one action; example exit 0; link manifest this library alone; core tree unmodified |
| US6 / FR-037-042 / SC-002 | `counters_pmu_test` (§10) | Merge with kernel-wins conflicts; every entry described; unencodable marked; paranoid 2: hardware `permission_blocked`, clocks/push `countable`, suite green; privileged-host evidence (developer): group read delivers members + enabled/running; multiplexed ratio < 1 with scaled flag |
| US7 / FR-023/026/031/040 / SC-004 | `counters_overhead` (§12) | Achieved modes disclosed per entry; fast-mode read follows the full protocol sequence; fast plan tens-of-cycles vs syscall microseconds published side by side, distributions min/median/max; skip names the failing probe |
| US8 / FR-043-045 / SC-009 | `pmu_events_check` fixtures + CI step (§11) | Clean tree exit 0; drifted hash names file exit 1; gate/RECORD disagreement exit 1; fixtures both directions green; drifted fixture red; check performs zero network calls |
| Contracts / FR-010 | header purity scan (ctest): platform terms absent from `include/speedgun-ng/counters*` | exit 0; violations named |
| Threading / FR-031, FR-035, FR-047 | trap pair: cross-thread plan/recorder/push use | aborts in dev/CI build; release carries no check (semantic-gated) |
| FR-025/FR-027 memory safety | `counters_trap_checked` release-configured checker | overrun aborts with contracts at `ignore` semantics |
| Gates / VIII | full matrix: dev, ci-sanitize, coverage, dbc-gate, prose-lint, consumer-release | all green; dbc-gate pairs every new interface; prose-lint clean; consumer-release carries only the mandated always-on site |

### Determinism and regression

Every assertion is a value equality, a command exit, a grep verdict, or an exit-code-gated skip (X.4). The fake provider removes all timing from the correctness spine: no sleeps, no tolerance bands outside the documented clock-vs-clock comparison (tolerance bands come from calibration data; no fixed wall constants). Sanitizer runs exercise group-fd lifetimes and the arena under ASan/UBSan. The pmu-events check makes vendored drift a red build without network. Published budgets land in the docs page per platform; regression wiring awaits the baseline-infrastructure spec (recorded deferral, no weakening).

## Complexity Tracking

> **No gate weakened; two P2 exceptions anticipated** (written justification lands at each site per X.2, previewed here and in R-007/R-011). The provider abstraction, the two-tier phase model, and the policy types are all spec-mandated structures with multiple live implementations, with the spec-mandated justification.

| Violation | Why Needed | Simpler Alternative Rejected Because |
|---|---|---|
| P2: x86 intrinsics (`__rdtsc`, `_rdpmc`) and `reinterpret_cast` of the mmap'd perf user-access page, inside provider implementations only | FR-034, FR-040 mandate the exact hardware mechanisms; the kernel ABI for the page is a fixed published struct, and the probe (capability bits, version, sysctl) establishes soundness before any read (R-007, R-011) | Any portable abstraction over these reads either forfeits the tens-of-cycles budget (defeating SC-004) or invents a second mechanism the kernel does not provide |
| P2 anticipated: none beyond the above; if the toolchain check (R-002) finds `std::expected` unavailable, a minimal in-house expected in a detail header joins the registry with that justification | Tier-2 errors must carry typed suggestion lists (FR-008) | Error codes alone cannot carry diagnostics; exceptions have no place in this repo's release semantics |
