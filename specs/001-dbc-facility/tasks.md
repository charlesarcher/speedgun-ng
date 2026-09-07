# Tasks: Design By Contract (DBC) Facility

**Input**: Design documents from `specs/001-dbc-facility/`

**Prerequisites**: plan.md (required), spec.md (required for user stories), research.md, data-model.md, contracts/

**Tests**: MANDATORY for this feature — constitution Principle III records **TDD** as the
execution mode (plan.md → Test Plan). Red tests are written and observed to fail before the
enforcement machinery exists.

**Organization**: Tasks are grouped by user story to enable independent implementation and
testing of each story. Granularity rule (post `/speckit.analyze`, 2026-09-06): one task = one
concern in one file (or one CI job / one script), each task independently verifiable; red→green
pairs are adjacent; no task may depend on a later-phase file for its own harness.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

## Path Conventions

- Public header: `include/speedgun-ng/`
- Implementation: `source/`
- Tests: `test/source/` (registered in `test/CMakeLists.txt`); gate fixtures `test/dbc-gate-fixture/`; negative-compile TUs `test/compile-fail/`
- Gate tooling: `tools/dbc/` + `cmake/dbc-gate.cmake`
- Build: `cmake/dbc.cmake` + `CMakePresets.json` + `CMakeLists.txt`
- CI: `.github/workflows/ci.yml`

**Design decisions these tasks implement** (plan.md + research R-013, confirmed at the plan
gate): statement-form `SG_*` macros (predicate first, `[[unlikely]]` on the failing branch);
**header-inline `noinline` `detail::dispatch` is PRIMARY** (standalone, emitted-on-use, absent
from `ignore` builds — SC-002); per-semantic dispatch qualifiers (`[[noreturn]]` in
`enforce`/`quick_enforce`, `noexcept` only in `quick_enforce`, neither in `observe`; always-on
sites force the default response regardless of the global semantic); single numeric compile
definition `SG_CONTRACTS_SEMANTIC` (`0`=ignore, `1`=observe, `2`=enforce, `3`=quick_enforce);
four per-kind `sg::dbc::check_*` public functions forwarding to one internal kind-tagged
`detail::dispatch`; feature-detected cold/unreachability shims confined to the cold dispatch
(hot path is pure C++20); out-of-line `source/dbc/dbc.cpp` dispatch is the documented
**fallback only** (engaged only if the assembly smoke test rejects the primary).

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Build-system scaffolding that all later phases consume.

- [ ] T001 [P] Create directories at the repository root: `tools/dbc/` (registry + gate/verification scripts), `source/dbc/` (fallback dispatch + measurement artifacts), `test/dbc-gate-fixture/` (US4 fixtures), `test/compile-fail/` (negative-compile TUs + harness). **Verification**: all four directories exist.
- [ ] T002 Create `cmake/dbc.cmake`: option `speedgun-ng_CONTRACTS` (values `ignore|observe|enforce|quick_enforce`, default `enforce`) mapped to a single **numeric** compile definition `SG_CONTRACTS_SEMANTIC` (0/1/2/3) applied PUBLIC to the library and all tests; wire `include(cmake/dbc.cmake)` into `CMakeLists.txt`; keep the dev-mode preset default at `enforce` in `CMakePresets.json` (FR-011/FR-016/FR-017; option name follows the `speedgun-ng_*` family per research R-012). **Verification**: configure + build a trivial TU echoing `SG_CONTRACTS_SEMANTIC` for each of the four option values.

**Checkpoint**: Build configures with the semantic switch; `SG_CONTRACTS_SEMANTIC` is visible
from library and test translation units.

---

## Phase 2: Foundational (Blocking Prerequisites — TDD: red, then green)

**Purpose**: The contract machinery itself. No user story can be implemented until this phase is
complete.

**⚠️ CRITICAL**: No user story work can begin until Phase 2 is complete.

### Red (tests written FIRST, observed to fail — all fail with the same root cause: `dbc.hpp` does not exist yet)

- [ ] T003 [P] Write the red core test suite in `test/source/dbc_test.cpp` and register the `dbc_test` executable in `test/CMakeLists.txt` (pattern: `add_executable` + `target_link_libraries(... speedgun-ng::speedgun-ng)` + `add_test`, matching the existing `speedgun-ng_test`). Assertion groups: (a) observer-based violation tests for each primitive — precondition (FR-002), postcondition over named result capture (FR-003/004), loop invariant at iteration entry (FR-005), class invariant at ctor exit / dtor entry / public member entry + non-const exit, const members entry-only (FR-006), `SG_ASSERT` in-body (FR-007); (b) every observed `ViolationRecord` carries kind, file, line, message, predicate text (FR-021); (c) exactly-once predicate evaluation via a side-effect counter (FR-019; counter == 0 under `ignore`); (d) semantic behavior — `observe` reports and continues (FR-015), `quick_enforce` terminates without invoking the hook (FR-013, forked child), `enforce` terminates (FR-014, forked child). **Observed failure**: compile error (no `dbc.hpp`).
- [ ] T004 [P] Write the red trap fixture `test/source/dbc_trap_fixture.cpp` and register the `dbc_trap_fixture` executable in `test/CMakeLists.txt` (**no direct `add_test`** — it aborts in checked builds): `main()` reaches a semantic-gated site that would abort in a checked build (`SG_REQUIRE(false, ...)`), prints the marker `gated-site-passed` to stdout, then reaches an always-on site (`SG_REQUIRE_ALWAYS(false, ...)`) which must abort. **Observed failure**: compile error.
- [ ] T005 Write the red **checked-build trap verification** ctest: a fork-based wrapper test (e.g. `test/source/dbc_trap_checked_test.cpp`, registered as `add_test(NAME dbc_trap_checked ...)`) that spawns `dbc_trap_fixture` in the default dev (`enforce`) configuration and asserts: non-zero exit (abort) **and** no `gated-site-passed` marker on stdout — proving the fixture "provably traps in a checked build" (SC-002/FR-018 half (a), the half the consumer-release job does not cover). **Observed failure**: the executable does not build yet.

### Green (implementation makes the red tests pass — sequential, one concern per task, all in `include/speedgun-ng/dbc.hpp`)

- [ ] T006 Implement the value types and observer storage in `include/speedgun-ng/dbc.hpp`: `sg::dbc::Kind` (precondition/postcondition/invariant/assertion), `sg::dbc::ViolationRecord` (kind, file, line, message, predicateText per data-model.md), `sg::dbc::violation_observer` (callable type), `sg::dbc::set_observer` (function-local static storage, ODR-shared across TUs), and a **minimal** default response (structured stderr diagnostic + `std::abort()`). **Verification**: `dbc_test` assertion groups (a)/(b) now pass — records are delivered with full identity; `dbc_test` (a)–(d) compiles.
- [ ] T007 Implement `sg::dbc::detail::dispatch(Kind, msg, file, line, pred)` in `include/speedgun-ng/dbc.hpp`: **header-inline + noinline** (feature-detected `SG_COLD`/`SG_NOINLINE`/`SG_UNREACHABLE` shims via `__has_attribute`/`__has_builtin`, degrading to nothing; hot path uses only standard C++20 — R-013); branches on `SG_CONTRACTS_SEMANTIC` for the violation response (observe: report+return; enforce: hook+terminate; quick_enforce: trap placeholder to be completed in T021); **per-semantic qualifiers** (R-013 C5): `[[noreturn]]` in `enforce`/`quick_enforce`, `noexcept` only in `quick_enforce`, neither in `observe`; **always-on sites force the default response regardless of the global semantic** (FR-036); `LCOV_EXCL` markers on the cold-dispatch blocks (FR-032). **Verification**: `dbc_test` compiles; the `observe` group passes; no EH machinery visible at call sites in a quick `-O2` objdump spot-check (full proof is T017).
- [ ] T008 Implement the four per-kind public functions in `include/speedgun-ng/dbc.hpp`: `sg::dbc::check_precondition` / `check_postcondition` / `check_invariant` / `check_assertion(msg, file, line, pred)` — each the **single registered enforcement function for its kind** (FR-025), each type-checking the message argument as a string literal (FR-020), each forwarding to `detail::dispatch` with its `Kind`. **Verification**: `dbc_test` groups (a)–(c) pass under `enforce` and `observe`.
- [ ] T009 Implement the eight macros in `include/speedgun-ng/dbc.hpp`: `SG_REQUIRE` / `SG_ENSURE` / `SG_INVARIANT` / `SG_ASSERT` in statement form `do { if (!(pred)) [[unlikely]] { check_*(msg, __FILE__, __LINE__, #pred); } } while (false)` (predicate first — FR-038/FR-040) with `#if SG_CONTRACTS_SEMANTIC == 0` elision to `((void)0)` (FR-012/FR-018/FR-037); plus the always-on family `SG_REQUIRE_ALWAYS` / `SG_ENSURE_ALWAYS` / `SG_INVARIANT_ALWAYS` / `SG_ASSERT_ALWAYS` with the identical body **outside** the `#if` (FR-036). **Verification**: `dbc_test` (d) passes (observe continues; `quick_enforce`/`enforce` terminate in forked children); the `ignore` build of `dbc_test` shows the exactly-once counter == 0 and compiles with the macros elided.
- [ ] T010 Implement the capture/placement mechanics in `include/speedgun-ng/dbc.hpp`: `SG_ENSURE` named result capture taken once at each return point, predicate evaluated against the capture (no implicit `old()`; pre-call references are explicit named captures taken at function entry — FR-003/FR-004, Edge Case: multiple return points); `SG_INVARIANT` placement for loops (each iteration entry, FR-005) and classes (ctor exit, dtor entry, public member entry, non-const exit; const members entry-only, FR-006, Edge Case: const member functions). **Verification**: ALL of T003/T004/T005 red tests now pass (`ctest` green for `dbc_test` and `dbc_trap_checked` under `enforce`).

**Checkpoint**: Foundation ready — the four primitives enforce end to end (TDD green); the
trap fixture traps under checked builds; user story implementation can now begin.

---

## Phase 3: User Story 1 - Express and enforce contracts on the public interface (Priority: P1) 🎯 MVP

**Goal**: Intent documented once, enforced at runtime — including the compile-time-first layer
and conformance of the pre-existing `exported_class`, proving the facility on real code.

**Independent Test**: A fixture interface with a documented precondition, postcondition, loop
invariant, and class invariant builds with contracts enabled; normal execution when contracts
hold; loud, well-identified termination when each is violated; compile-time-evaluable
constraints are `static_assert`s with no runtime check (spec.md US1 acceptance 1–7).

### Tests for User Story 1 (TDD — written first, observed to fail)

- [ ] T011 [P] [US1] Create the negative-compile harness: `test/compile-fail/run.sh` (compiles each sibling TU in `test/compile-fail/` with the project warning set and asserts the expected compile **failure** with a diagnostic-match string per TU; TUs in a `positive/` subdirectory must compile **cleanly**) + a self-test TU (a known-ill-formed TU proving the harness itself catches failures) + an `add_test(NAME dbc_compile_fail COMMAND run.sh)` entry in `test/CMakeLists.txt`. **Verification**: harness self-test passes (it catches the known-ill-formed TU); the suite is trivially green until T012 adds TUs — the meaningful red is T012's.
- [ ] T012 [US1] Add the red compile-time-layer tests: negative TUs `test/compile-fail/negative_runtime_only_constraint.cpp` (a compile-time-evaluable constraint expressed only as a runtime `SG_*` check — must FAIL to compile, FR-023) and `test/compile-fail/negative_nonliteral_message.cpp` (a non-string-literal message expression — must FAIL to compile, FR-020); positive TUs `test/compile-fail/positive/static_assert_constraint.cpp` (a compile-time-evaluable constraint as `static_assert`/concept, active in every configuration at zero runtime cost, FR-022) and `test/compile-fail/positive/constexpr_only_interface.cpp` (a constexpr-only interface constrained at compile time and exempt from runtime enforcement, FR-024). **Observed failure**: the negative TUs compile when they must not (FR-023 rejection not yet implemented) — TDD red.
- [ ] T013 [US1] TDD green — implement the compile-time contract layer in `include/speedgun-ng/dbc.hpp`: the documented `static_assert`/concept/constexpr-validator idiom for constraints evaluable at compile time, and the FR-023 rejection mechanism (a runtime `SG_*` on a compile-time-evaluable constraint is a compile error with a clear diagnostic). **Verification**: T011's harness green (negatives fail with the expected diagnostics; positives compile clean).
- [ ] T014 [P] [US1] Conform the pre-existing interface (R-009/FR-029): add doxygen `\pre`/`\post` to `exported_class` in `include/speedgun-ng/speedgun-ng.hpp` exactly per `contracts/api-contracts.md` (ctor: `\pre` none, `\post` `name()` returns the project name; `name() const`: `\pre` object in a valid state (class invariant), `\post` returns a non-owning pointer to the stored string); enforce them in `source/speedgun-ng.cpp` via `SG_REQUIRE`/`SG_ENSURE` plus the class `SG_INVARIANT` (ctor exit, dtor entry, member entry/exit); add pass-side AND fail-side contract tests for `exported_class` in `test/source/speedgun-ng_test.cpp` (SC-004). **Verification**: `speedgun-ng_test` green; `exported_class` appears in the DBC matrix as fully paired (verified again by the gate in Phase 6).

**Checkpoint**: MVP — User Story 1 fully functional and testable independently (Principle II
satisfied for the public interface, including the pre-existing class).

---

## Phase 4: User Story 2 - Zero contract cost in release builds (Priority: P1)

**Goal**: The release artifact provably contains zero semantic-gated contract code; always-on
sites are the deliberate, sparing exception; the satisfied-check hot path is proven to be
predicate + one predicted branch with no EH machinery at the call site.

**Independent Test**: A contract site that provably traps in a checked build runs silently in the
`ignore` (release) build; symbol inspection of the release artifact shows no semantic-gated
contract-machinery symbols while the always-on site still fires (SC-002; spec.md US2
acceptance 1–3).

### Tests and implementation for User Story 2 (all [P] — distinct files, depend only on Phase 2 green)

- [ ] T015 [P] [US2] Standalone-compile test: `test/source/dbc_standalone.cpp` — a TU that includes `<speedgun-ng/dbc.hpp>` with **no project include paths beyond the public `include/` directory** (in particular no generated-export-header include) that compiles, links, and runs a trivial always-on check (FR-037); register `dbc_standalone_test` in `test/CMakeLists.txt` with include dirs restricted to the public `include/` only. **Verification**: `dbc_standalone_test` green.
- [ ] T016 [P] [US2] Add the dedicated **consumer-release** CI job to `.github/workflows/ci.yml` (SC-002, research R-006). One job, four explicit assertions: (1) Release configuration, developer mode **off**, `speedgun-ng_CONTRACTS=ignore`, build the library exactly as a consumer would; (2) symbol-inspect the artifact (`nm`) for the **absence** of semantic-gated `sg::dbc::check_*` machinery; (3) run `dbc_trap_fixture` and assert the `gated-site-passed` marker on stdout **and** an abort (non-zero exit) with the always-on diagnostic on stderr — gated site elided, always-on site fires (FR-036 both directions); (4) run `dbc_test` under `ignore` asserting an always-on violation fires identically to checked builds while a semantic-gated violation is silent (FR-036/FR-012). **Verification**: the job is well-formed (YAML lint) and green on a feature branch.
- [ ] T017 [P] [US2] Implement `tools/dbc/asm_smoke.sh` (Linux; GCC and Clang, `-O2`): compile a hot function containing a satisfied `SG_REQUIRE` with a **runtime** predicate and assert from the assembly that the satisfied path is predicate + one predicted branch with no `call` / landing-pad / `_Unwind` / `.cfi_personality` references attributable to the check, and that all EH machinery is confined to the dispatch (FR-039; R-013 empirical baseline; SC-008 evidence step — Linux evidence, not a hard cross-platform gate per plan Test Plan). **Verification**: script exits 0 on the reference toolchains; register it as `add_test(NAME dbc_asm_smoke ...)` (Linux-only, skipped on other platforms).
- [ ] T018 [P] [US2] Implement the SC-008 overhead measurement: `tools/dbc/overhead.cpp` (a hot-loop TU with a satisfied `SG_REQUIRE` vs an identical uncontracted loop) + `tools/dbc/overhead.sh` (compile `-O2`, run both, report the distribution min/max/n50/n99 into a committed report file `docs/pages/dbc-overhead.md`); documented measurement, not a per-PR assertion (Principle VII). **Verification**: `overhead.sh` exits 0 and the report file is produced with the distribution.
- [ ] T019 [P] [US2] Implement the FR-017 independence matrix: `tools/dbc/semantics_matrix.sh` — build/run `dbc_test` across `NDEBUG` on/off × `-O0`/`-O2` and assert contract evaluation state is unchanged by `NDEBUG`/optimization and that the dedicated switch never changes standard `assert` behavior (Edge Case: standard assertion macro interaction). **Verification**: script exits 0 for all 4 combinations; register as `add_test(NAME dbc_semantics_matrix ...)` (Linux-only).

**Checkpoint**: User Stories 1 AND 2 independently functional — the facility enforces in checked
builds and is provably absent in release.

---

## Phase 5: User Story 3 - Observe violations deterministically in tests (Priority: P2)

**Goal**: Every contract is provable to fail loudly: the observer API delivers the full record,
the default response is uncatchable, and the single global hook is the only interception point.

**Independent Test**: Install a test observer, trigger a known violation, and assert the observed
record carries the correct kind, file, line, message, and predicate (spec.md US3 acceptance 1–3).

### Tests for User Story 3 (TDD — written first, observed to fail)

- [ ] T020 [US3] Add red observer/response tests to `test/source/dbc_test.cpp`: (a) default response — a forked child under `enforce` violating a contract terminates (abort) with a structured stderr diagnostic naming kind, file, line, message, and is **not catchable** (a `try`/`catch` around the site does not intercept, FR-008/FR-010); (b) single global hook — a second `set_observer` replaces the first (FR-010); (c) observer-throw propagation — under `enforce`, a test observer throwing a unique exception type terminates the process **via that exception** (not a generic `std::terminate` conversion), verifying the dispatch is not `noexcept` in `enforce` (FR-009/FR-014, R-013 C5); (d) a violation inside a `noexcept` function terminates via the response with no exception escaping (Edge Case: noexcept context); (e) re-entry guard — a violation triggered from within the default response (via the installed observer calling a contract-violating function) does not recurse (Edge Case: violation inside the response). **Observed failure**: group (e) — the re-entry guard (implemented in T021) — is the TDD red (L1 note: groups (a)–(d) may already pass against T007–T008's machinery; record (e)'s failure as the red and note the state of (a)–(d) at write time).

### Implementation for User Story 3

- [ ] T021 [US3] TDD green — complete the violation response in `include/speedgun-ng/dbc.hpp`: `default_response` formatting (`[kind] message (predicate: pred) at file:line` to stderr, flush, then `std::abort()`; best-effort emission; **non-recursive re-entry guard** — a thread-local in-response flag suppresses recursive handling; best-effort thread-safe diagnostic per Edge Case: handler re-entrancy across threads) and the `SG_TRAP` primitive for `quick_enforce` (guarded `__builtin_trap` / MSVC `__debugbreak`, no hook, no diagnostic, FR-013) replacing the T007 placeholder. **Verification**: T020 group (e) now passes; all T020 groups green; the forked-child diagnostics match exactly.

**Checkpoint**: All three P1/P2 runtime stories independently functional — contracts are
enforced, release is provably clean, and every contract's fail side is deterministically
observable.

---

## Phase 6: User Story 4 - CI fails on undocumented or unenforced contracts (Priority: P2)

**Goal**: The Phase 0 DBC coverage gate: documentation presence + doc-to-enforcement pairing,
data-driven from the macro registry, failing CI on any gap, with a per-interface DBC matrix
artifact.

**Independent Test**: Seed the gate's input with (a) a public function missing a `\pre` or
`\post` section and (b) a documented contract with no matching enforcement, and observe CI fail
on each with a diagnostic naming the offending interface (spec.md US4 acceptance 1–5; SC-006).

### Tests for User Story 4 (TDD — written first, observed to fail)

- [ ] T022 [P] [US4] Create `tools/dbc/macros.yaml` (the **Macro registry**, FR-026): the machine-readable mapping of every macro → (enforcement function, kind) — `SG_REQUIRE`/`SG_ENSURE`/`SG_INVARIANT`/`SG_ASSERT` plus the four `SG_*_ALWAYS` variants (same enforcement function and kind, `always_on: true` flag; see data-model.md Macro registry) — the single source of truth consumed by both gate halves and feature 002's AST gate; no gate hard-codes a macro list. **Verification**: YAML parses; every macro in `dbc.hpp` has exactly one registry entry and vice versa (checked by the gate harness itself).
- [ ] T023 [P] [US4] Create the committed gate-fixture tree `test/dbc-gate-fixture/` (self-contained headers, **not** part of the library): `fixture_missing_docs.hpp` (a public function lacking a `\pre` or `\post` section), `fixture_drift.hpp` (a documented contract kind with no enforcement), `fixture_enforced_not_documented.hpp` (an enforcement with no documentation), `fixture_exempt.hpp` (exempted declarations — private/protected member, defaulted/deleted, friend, constexpr-only — plus an explicit `none` marker for a genuinely empty contract set, FR-029/FR-030), and `fixture_clean.hpp` (a fully documented-and-enforced interface, including `none` where genuinely empty). **Verification**: the tree compiles standalone (each fixture is syntactically valid C++).
- [ ] T024 [US4] Write the red gate effectiveness harness — **self-contained, no dependency on the Phase-6 cmake target** (H3 fix): `test/dbc-gate-fixture/run_gate_fixtures.py` (invoked directly, e.g. `python3 run_gate_fixtures.py --gate <script> --registry tools/dbc/macros.yaml <fixture-dir>` per fixture) that runs both gate halves against the `test/dbc-gate-fixture/` tree and asserts: `fixture_missing_docs` → doc-gate failure naming the interface + missing section; `fixture_drift` and `fixture_enforced_not_documented` → pairing failure naming interface + kind + drift direction; `fixture_exempt` and `fixture_clean` → pass; every run emits the per-interface DBC matrix artifact (SC-006, FR-031, US4 acceptance 1–5) — plus a matching `add_test(NAME dbc_gate_fixtures ...)` entry in `test/CMakeLists.txt` that invokes the harness once both gate scripts exist. **Observed failure**: the gate scripts do not exist — TDD red.

### Implementation for User Story 4

- [ ] T025 [P] [US4] TDD green — implement `tools/dbc/dbc_doc_gate.py` (documentation-presence half, FR-027/FR-029/FR-030/FR-031): parse the Doxygen XML build of the target headers (CLI: `--xml <dir> --registry <macros.yaml> --out <matrix>`), scope = all public functions and classes in the public headers with the **closed** exemption list (FR-029); fail on a missing precondition/postcondition section (or class invariant section for invariant-bearing classes) with a diagnostic naming the interface + missing section (FR-027); honor the explicit `none` marker (explicitly empty passes, implicitly empty fails, FR-030); emit the documented-kinds half of the per-interface DBC matrix (FR-031). **Verification**: `run_gate_fixtures.py` doc-gate assertions pass.
- [ ] T026 [P] [US4] TDD green — implement `tools/dbc/dbc_pair_gate.py` (pairing half, FR-028/FR-030/FR-031): detect enforcement mechanically from the macro registry (the `SG_*` → enforcement-function/kind mapping, FR-025/FR-026 — no source-text heuristics; CLI: `--src <header-dir> --registry <macros.yaml> --out <matrix>`); compare documented kinds vs enforced kinds in **both directions** (documented-not-enforced and enforced-not-documented, FR-028) with diagnostics naming the interface + kind + drift; honor the explicit `none` marker (FR-030); emit the enforced-kinds half of the DBC matrix (FR-031). **Verification**: `run_gate_fixtures.py` green in full (SC-006).
- [ ] T027 [US4] Create `cmake/dbc-gate.cmake`: a developer-mode `dbc-gate` target that (a) runs a **minimal standalone Doxygen invocation** (XML-only; `GENERATE_XML = YES` is already in `docs/Doxyfile.in` — do **not** reuse the m.css `docs` target, which FetchContents m.css from the network) over `include/speedgun-ng/`, (b) runs both gate scripts with `tools/dbc/macros.yaml`, (c) publishes the per-interface DBC matrix as a CI artifact, and (d) fails the build/CI on any gap (FR-027/FR-028/FR-031; R-008). **Verification**: `dbc-gate` green on the current tree (including the conformed `exported_class` from T014).
- [ ] T028 [US4] Add the `dbc-gate` CI job to `.github/workflows/ci.yml` (per-PR, R-008): run the `dbc-gate` target and assert green on the current tree; additionally run the fixture-based effectiveness proof (SC-006) so the gate's own failure modes are CI-gated. **Verification**: job green on a feature branch; a deliberately seeded drift (temporary commit) fails the job with the expected diagnostic.

**Checkpoint**: The 100% DBC coverage mandate is now CI-enforced: undocumented, unenforced, or
drifted contracts fail the build; the DBC matrix artifact is produced every run.

---

## Phase 7: User Story 5 - Migrate mechanically when language contracts land (Priority: P3)

**Goal**: The facility's vocabulary and semantics map one-to-one onto C++26 so migration is a
mechanical rename; loop and type invariants remain facility-based.

**Independent Test**: A conformance review: the documented mapping table covers the four
semantics, the hook model, and every facility construct (spec.md US5 acceptance 1–3; SC-010).

- [ ] T029 [P] [US5] Create the documented migration mapping `docs/pages/dbc-migration.md` (FR-035): `SG_REQUIRE` → `pre`, `SG_ENSURE` → `post (r : …)` (named result capture), `SG_ASSERT` → `contract_assert`, `SG_INVARIANT` (loop/type) → facility-based (no standard counterpart), `SG_*_ALWAYS` → no standard counterpart (per-site designation is inexpressible in the standard's per-TU semantics), the four semantics → same-named standard semantics, `set_observer`/default response → replaceable `handle_contract_violation` hook; plus the conformance review: each semantic name ↔ standard semantic with matching behavior (ignore: no code; observe: report and continue; enforce: report and terminate; quick_enforce: terminate immediately without reporting), and the hook behaviors match (default handler reports and terminates under enforce; immediate termination without handler under quick_enforce) — the mapping table must cover 100% of the facility's constructs (SC-010). **Verification**: a checklist in the doc marks every facility construct (8 macros, 4 semantics, hook model, loop/type invariants) as mapped or explicitly documented as facility-based; `dbc-gate` still green (the new doc file is outside the gate scope but must not break Doxygen).

**Checkpoint**: The migration guarantee is documented and reviewable; no code changes.

---

## Phase 8: Polish & Cross-Cutting Concerns

**Purpose**: Full quality-gate conformance and final validation.

- [ ] T030 [P] Documentation pass: doxygen comments on all new public interfaces in `include/speedgun-ng/dbc.hpp` (with `\pre`/`\post`/`\invariant` where applicable — the facility's own public API is DBC-conformed, SC-001); update `README.md` (developer-mode `dbc-gate` target, the `speedgun-ng_CONTRACTS` option, the consumer-release and dbc-gate CI jobs); link `docs/pages/dbc-migration.md` and `docs/pages/dbc-overhead.md` from the docs index. **Verification**: Doxygen builds `include/speedgun-ng/dbc.hpp` without documentation warnings (the gate's XML build succeeds); README renders with the new targets/options listed; both docs pages are linked.
- [ ] T031 [P] Quality-gate sweep: run `format-check` and `spell-check` over all new/changed files (fix via `format-fix`/`spell-fix`); verify the existing clang-tidy/cppcheck presets cover the new sources with no config changes needed (Principle V/VIII); **explicit FR-033/SC-009 assertion**: scan `CMakeLists.txt` + `cmake/` and assert zero external **runtime** dependencies of the library target (no runtime `find_package`/`FetchContent`/external `target_link_libraries`; developer-mode docs tooling such as m.css is out of scope and noted as such). **Verification**: `format-check` and `spell-check` exit 0; the dependency scan prints its result and exits 0.
- [ ] T032 [P] Additional edge-case tests in `test/source/dbc_test.cpp`: templates — a contract documented on the primary template declaration is enforced from the template body and applies to every instantiation, and a violating instantiation aborts identically (Edge Case: templates); virtual overrides — each overriding implementation enforces the documented contract itself, derived classes do not inherit enforcement (Edge Case: virtual overrides). **Verification**: new tests green and the full `dbc_test` suite remains green.
- [ ] T033 Final validation: run the `quickstart.md` walkthrough end-to-end (sections 1–7); full `cmake --preset=dev` + `ctest --preset=dev` green (SC-005); verify the coverage run shows 100% line/branch with the FR-032 `LCOV_EXCL` exclusions in effect; confirm all CI jobs are green on a feature branch (build matrix, sanitizers, coverage, `dbc-gate`, `consumer-release`). **Verification** (acceptance): quickstart sections 1–7 pass verbatim; `ctest` 100% green; coverage report at 100% line/branch; feature-branch CI fully green.

**Checkpoint**: Feature 001 complete — all quality gates green, quickstart validated.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies — can start immediately
- **Foundational (Phase 2)**: Depends on Setup completion — **BLOCKS all user stories**
- **User Stories (Phases 3–7)**: All depend on Foundational phase completion
  - US1 (P1) and US2 (P1) can proceed in parallel after Foundational
  - US3 (P2) depends only on Foundational (the observer API and dispatch land in T006–T009); may run in parallel with US1/US2
  - US4 (P2) depends on the facility (T008) and the conformed `exported_class` (T014) so the gate is green on the real tree at landing
  - US5 (P3) is documentation-only; depends on the final vocabulary (after US1)
- **Polish (Phase 8)**: Depends on all desired user stories being complete

### User Story Dependencies

- **User Story 1 (P1)**: Can start after Foundational (Phase 2) — no dependencies on other stories
- **User Story 2 (P1)**: Can start after Foundational — the trap fixture (T004) and its checked-build verification (T005) are written in the foundational red phase; may integrate with US1 but is independently testable
- **User Story 3 (P2)**: Can start after Foundational — the observer/hook machinery lands in T006–T009; response completion in T021; independently testable
- **User Story 4 (P2)**: Can start after US1 (registry needs the enforcement-function names from T008; the gate must pass on the conformed tree from T014) — independently testable via the fixtures
- **User Story 5 (P3)**: Can start after US1 (vocabulary is stable) — documentation only

### Within Each User Story

- Red tests MUST be written and observed to fail before implementation (TDD, Principle III)
- One concern per task; each task carries an explicit **Verification** step
- Story complete before moving to the next priority

### Parallel Opportunities

- Phase 1: T001 ∥ T002
- Phase 2 red: T003 ∥ T004 ∥ T005 (three distinct files); green: T006 → T007 → T008 → T009 → T010 (sequential — all edit `dbc.hpp`)
- Phase 3: T011 ∥ T014, then T012 → T013 (T012 depends on T011's harness; T014 is an independent file set)
- Phase 4: T015 ∥ T016 ∥ T017 ∥ T018 ∥ T019 (all distinct files; all depend only on Phase 2 green)
- Phase 6: T022 ∥ T023 → T024 (red, self-contained) → T025 ∥ T026 (distinct scripts) → T027 → T028
- Phase 8: T030 ∥ T031 ∥ T032 → T033 (final validation is sequential)

---

## Parallel Example: User Story 4

```bash
# After US1 (T008/T014 done), launch the gate's inputs together:
Task: "T022 registry in tools/dbc/macros.yaml"
Task: "T023 fixture tree in test/dbc-gate-fixture/"

# Then red → green:
Task: "T024 red gate-effectiveness harness (self-contained, no T027 dependency)"
Task: "T025 doc gate in tools/dbc/dbc_doc_gate.py"  ∥  "T026 pairing gate in tools/dbc/dbc_pair_gate.py"
Task: "T027 cmake/dbc-gate.cmake (minimal Doxygen XML, no m.css)"
Task: "T028 dbc-gate CI job in .github/workflows/ci.yml"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup
2. Complete Phase 2: Foundational (CRITICAL — blocks all stories)
3. Complete Phase 3: User Story 1
4. **STOP and VALIDATE**: test US1 independently (spec.md US1 acceptance 1–7)
5. The facility is usable: contracts documented once, enforced at runtime

### Incremental Delivery

1. Setup + Foundational → foundation ready
2. US1 → test independently → **MVP** (Principle II satisfied for the public interface)
3. US2 → test independently → release is provably contract-free (SC-002)
4. US3 → test independently → every contract's fail side is deterministically observable
5. US4 → test independently → 100% DBC coverage is a hard CI gate
6. US5 → documentation → migration guarantee
7. Polish → all quality gates green

### Parallel Team Strategy

With multiple developers after Foundational:

1. Developer A: US1 (compile-time layer + `exported_class` conformance)
2. Developer B: US2 (consumer-release job, asm smoke, overhead + matrix scripts)
3. Developer C: US3 (observer/response hardening)
4. Then Developer A/B/C converge on US4 (gate), with US5 and Polish following

---

## Notes

- [P] tasks = different files, no dependencies on incomplete tasks
- [Story] label maps task to specific user story for traceability
- Each user story is independently completable and testable
- **Verify red tests fail before implementing** (TDD — constitution Principle III)
- Every task carries an explicit **Verification** step (post `/speckit.analyze` granularity rule)
- Commit after each task or logical group (atomic, bisectable; constitution Pull Request Quality)
- Stop at any checkpoint to validate the story independently
- Avoid: vague tasks, same-file conflicts, cross-story dependencies that break independence
- The out-of-line `source/dbc/dbc.cpp` dispatch fallback (FR-037 form b) is **not** a
  scheduled task — it is engaged only if T017's assembly smoke test rejects the header-inline
  primary (research R-013 D2)
