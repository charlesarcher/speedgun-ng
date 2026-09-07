# Research: Design By Contract Facility

**Feature**: 001-dbc-facility | **Date**: 2026-09-06
**Status**: All NEEDS CLARIFICATION resolved (0 remaining).

## Phase 0 Findings

Research was completed upstream (four parallel decision-grade research packets,
2026-09-06) and consolidated into a decision brief. This file records the
decisions the spec and plan rest on, their rationale, and the alternatives that
were rejected. Each finding is tagged for traceability into the plan.

### R-001: Mechanism = own thin macro facility (no vendored library)
- **Decision**: Implement the contract facility as a small in-repo macro layer
  over a single enforcement function per kind. No Boost, no external library,
  zero new runtime dependencies.
- **Rationale**: The constitution mandates zero external runtime dependencies
  and C++20 without extensions. The strongest production evidence in C/C++
  (libstdc++ / MSVC-STL / libc++ hardening, Zephyr, FreeRTOS, pico-sdk, HPX) is
  exactly this macro pattern. A macro facility keeps the `ignore` release-strip
  a true `#if` elision and keeps contract sites mechanically detectable for the
  coverage gate.
- **Alternatives considered**:
  - *Boost.Contract* — rejected: pulls ~15 Boost header components, a global
    mutex in checked builds (perturbs benchmark measurements), ignores `NDEBUG`
    (release-strip trap), and carries an open sanitizer issue. Violates the
    zero-dependency mandate.
  - *Lib.Contract* — rejected: no `result`/`old` support, release-strip leaves
    RAII scaffolding compiled (optimizer-dependent, not `#if`-guaranteed),
    single maintainer.
  - *C++26 native contracts* — deferred: as of 2026-09 only GCC 16 ships it and
    the standard has no loop or class invariant constructs. Chosen as the
    forward migration target (FR-035), not the current mechanism.

### R-002: Semantics and build modes mirror C++26
- **Decision**: Four build-time evaluation semantics named exactly
  `ignore` / `observe` / `enforce` / `quick_enforce`, plus a single replaceable
  global violation hook. `enforce` is the dev/CI default; `ignore` is the
  release configuration.
- **Rationale**: 1:1 naming with the standard (P2900R14) makes a future
  migration a mechanical rename. This is the ecosystem convergence point (HPX,
  OpenImageIO, and TrenchBroom all use these names).
- **Alternatives considered**: `NDEBUG`-based switching (rejected — couples the
  contract switch to the optimization flag, the canonical anti-pattern); a
  per-site runtime toggle (rejected — FR-017 fixes a single build-time switch).

### R-003: Zero release cost via a dedicated switch
- **Decision**: Contract evaluation is controlled by a dedicated build switch,
  independent of `NDEBUG` and of the optimization level. `ignore` emits no
  contract code (`#if` elision).
- **Rationale**: FR-017/FR-018. A benchmarking framework must not ship checks
  into the measured binary.
- **Verification**: A dedicated consumer-release CI job (R-006) proves the
  artifact is contract-free (SC-002).

### R-004: Contract coverage gate, phased
- **Decision**: 001 ships the facility plus a Phase 0 DBC coverage gate
  (documentation-presence gate over Doxygen XML + a doc-to-enforcement pairing
  check). The AST-level hard gate is deferred to feature 002.
- **Rationale**: The constitution makes 100% DBC coverage a hard gate, but no
  off-the-shelf tool measures doc↔code pairing. A phased approach keeps CI
  honest from the landing commit (Phase 0, coarse but real) while the
  higher-fidelity AST gate (feature 002) is built.
- **Alternatives considered**: AST-only gate from day one (rejected — blocks
  001 on a multi-week tool build); no gate (rejected — an unenforceable hard
  gate).

### R-005: Checker-friendly macro design
- **Decision**: Every contract macro expands to a call of exactly one
  registered enforcement function (one per kind). The macro→kind registry ships
  as machine-readable data (YAML) consumed by the Phase 0 gate and by feature
  002's AST gate.
- **Rationale**: FR-025/FR-026. This single constraint is what buys ~90–95%
  pairing fidelity without a full AST tool.

### R-006: CI verification of the zero-release-cost guarantee
- **Decision**: A dedicated consumer-release CI job (Release, developer mode
  off, `ignore`) symbol-inspects the artifact and runs a trap fixture that must
  exit silently.
- **Rationale**: SC-002. All existing CI jobs build with contracts on; without
  this job the "release is contract-free" guarantee would be asserted but never
  verified.
- **Alternatives considered**: Flipping the existing 3-OS matrix to
  consumer-style (rejected — macOS/Windows release runs would stop exercising
  contracts); no CI verification (rejected — the guarantee must be gated, not
  documented).

### R-007: The in-body assertion is named `assert` (SG_ASSERT)
- **Decision**: The 4th contract primitive is `assert`, macro `SG_ASSERT`.
- **Rationale**: FR-001/FR-035. Maps 1:1 to C++26 `contract_assert`; the
  ecosystem has already converged on this name.
- **Alternatives considered**: `check` / `verify` (rejected — no direct
  standard counterpart, weaker migration mapping).

### R-008: The Phase 0 gate runs in a dedicated `dbc-gate` CI job
- **Decision**: A new ubuntu-only `dbc-gate` job runs on every `pull_request`
  and `push` (Doxygen XML + pairing check), separate from the master-only docs
  deploy job.
- **Rationale**: FR-027/FR-028/FR-031 + SC-006. The gate must enforce per-PR;
  the existing docs job only runs on master pushes.
- **Alternatives considered**: Folding into the docs job (rejected —
  master-only); folding into the 3-OS test matrix (rejected — Doxygen on three
  OSes × shared/static for no added value, since the gate is
  platform-independent).

### R-009: The pre-existing interface is conformed in 001
- **Decision**: `exported_class` (the only pre-existing public class) is brought
  to 100% DBC conformance (documented `\pre`/`\post` + matching runtime
  enforcement) within 001, before the gate lands.
- **Rationale**: FR-029 scopes the gate to all public headers from day one.
  Un-conformed pre-existing code would make CI red on the very PR that ships the
  gate; an allowlist would be a forbidden gate-weakening.
- **Alternatives considered**: An allowlist for pre-existing symbols (rejected —
  the constitution forbids silent gate weakening); deferring conformance to a
  follow-up (rejected — leaves the gate red at the landing commit).

### R-010: Call-site performance design (performance-sensitive, pervasive use)
- **Decision**: The contract call site is designed as a **statement-form macro** (`do { if (!(pred)) [[unlikely]] { check_kind(msg, __FILE__, __LINE__, #pred); } } while (false)`), not a function call:
  - *Predicate first*: C++ evaluates function arguments before the call, so a call-form macro would evaluate `msg`/`file`/`line` even on the satisfied path. The `if`-guard form evaluates **only the predicate** first; diagnostic arguments are inside the branch and evaluated **only on failure** (FR-038).
  - *Exactly-once*: the predicate appears exactly once in the condition (FR-019).
  - *`[[likely]]`/`[[unlikely]]`*: the failing branch is annotated `[[unlikely]]` so the hot path is laid out contiguously and the cold dispatch is placed out of line (FR-040).
  - *No EH at the call site*: the check dispatch is a separate cold function, declared `[[noreturn]]` in terminating semantics, and **not inlined** into the call site (header-only `inline` + a portable noinline shim — `__attribute__((noinline))` on GCC/Clang, `__declspec(noinline)` on MSVC — or a genuinely out-of-line library function). Any throw (a test observer) occurs **inside** the cold function, so the hot path carries no inlined landing pads or unwind continuation; the failure edge is terminal (`[[noreturn]]`), so the call site needs no landing pad (FR-039).
  - *Standalone header*: `dbc.hpp` does not depend on the library's export header or other project headers; in `ignore` mode semantic-gated macros expand to nothing (predicate not evaluated, zero code) (FR-037).
- **Rationale**: the facility is intended for pervasive use, so the satisfied-check cost must stay a predicate evaluation plus a predicted branch. This is the current baseline of a **continuing call-site performance family** (per plan review, 2026-09-06); the design keeps further requirements in the family cheap to add.
- **Verification**: SC-008 overhead distribution (checked vs uncontracted), an assembly smoke test (Linux) confirming the hot path of a satisfied check has no EH continuation machinery, and a standalone-compile test (dbc.hpp compiles in a TU with no project includes).
- **Alternatives considered**: call-form macros with pre-evaluated args (rejected — violates FR-038); `assert`-style with no stringified predicate (rejected — loses the predicate text in the record, FR-021); inlined dispatch (rejected — EH spread at the call site, FR-039).

### R-011: Always-on DBC (deliberate exception to zero-release-cost)
- **Decision**: A contract may be designated **always-on**: enforced in every configuration (including `ignore`/release), not stripped by the semantic switch, and compiler-eliminable-proof because the check has observable side effects on violation (diagnostic + termination). Always-on uses a distinct macro family (e.g. `SG_REQUIRE_ALWAYS` / `SG_ENSURE_ALWAYS` / `SG_INVARIANT_ALWAYS` / `SG_ASSERT_ALWAYS`) so the registry, the gates, and the consumer-release job can distinguish always-on sites from semantic-gated ones.
- **Rationale**: some critical invariants must hold even in release binaries. The zero-release-code guarantee (FR-018/SC-002) applies to **semantic-gated** contracts; the consumer-release trap fixture distinguishes an always-on site (must still fire) from a semantic-gated site (must be silent).
- **✅ Constitution tension (RESOLVED 2026-09-06, plan gate)**: constitution Principle II previously stated, without exception, that "contract checks MUST NOT emit any code in release builds". Always-on contracts (FR-036) were in direct tension with that sentence. Resolved at the plan gate by the user (maintainer): the always-on carve-out is applied **directly to the Principle II text** at the user's direction — the user explicitly declined the formal-amendment framing for now ("don't call it an amendment, we're not going there yet; just make the change"). The constitution text now carries the carve-out (applied wording below); the formal amendment ceremony (v2.2.1 → 2.3.0 MINOR version bump + Sync Impact Report) is **deferred** at the user's request and may be recorded later as a housekeeping follow-up. Rationale of record: this spec (FR-036/FR-018/SC-002) + research R-011/R-013.
- **Applied change (constitution Principle II, 2026-09-06 — user-directed, no formal version bump).** The Principle II bullet "Contract checks MUST NOT emit any code in release builds: zero performance cost on critical paths." was replaced with:
  > Contract checks MUST NOT emit any code in release builds: zero performance cost on critical paths. This applies to semantic-gated contract checks — those selected by the `ignore` / `observe` / `enforce` / `quick_enforce` evaluation switch. A contract MAY be explicitly designated always-on; always-on contracts are present and enforced in every build configuration, including release, and are the deliberate, sparing exception reserved for critical invariants that must hold even in release binaries. The macro registry and release-artifact verification MUST distinguish always-on sites from semantic-gated ones. (Rationale of record: spec 001-dbc-facility FR-036/FR-018/SC-002; research R-011/R-013.)
  The accompanying formal ceremony (version 2.2.1 → 2.3.0 MINOR bump + Sync
  Impact Report) is **deferred** at the user's request ("not going there
  yet"); if formalized later, the Sync Impact Report would record: modified
  section Principle II (always-on carve-out added to the release-code
  bullet); added/removed sections: none.
- **Alternatives considered**: piggybacking always-on on the `enforce` semantic (rejected — `ignore` builds would silently drop the critical invariant); a per-build CMake allowlist of always-on symbols (rejected — invisible at the call site, breaks single source of truth); no always-on support (rejected — explicitly requested at plan review).

### R-012: Naming — `SG_` macro prefix and `sg` namespace (Core Guidelines-verified)
- **Decision**:
  - Macros: `SG_REQUIRE` / `SG_ENSURE` / `SG_INVARIANT` / `SG_ASSERT` (+ `SG_*_ALWAYS`) — the `SG_` prefix replaces the earlier `SGN_` draft.
  - Namespace: `sg` (lowercase), e.g. `sg::dbc::check_precondition` — replaces the earlier `sgn::dbc` draft.
  - CMake option: `speedgun-ng_CONTRACTS` — follows the repo's existing `speedgun-ng_*` option family (P1 project convention outranks P3 per constitution Principle I).
- **Rationale (verified verbatim against isocpp/CppCoreGuidelines@master, NL section, 2026-09-06)**:
  - **NL.9**: "Use `ALL_CAPS` for macro names only" → `SG_*` macros compliant.
  - **NL.8** ("Use a consistent naming style"): the ISO-standard example is "use lower case only and digits, separate words with underscores" (`int`, `vector`, `my_map`) — the base identifier style, including namespaces (`std`).
  - **NL.10**: "Prefer `underscore_style` names" — "the original C and C++ style and used in the C++ Standard Library".
  - Stroustrup/PPP variant (NL.8/NL.10 example): "ISO Standard, but with upper case used for your own types and concepts" — uppercase is reserved for types/concepts; namespaces stay lowercase.
  - The Core Guidelines are the pinned P3 baseline (constitution Principle I), so `sg` (lowercase) is the constitution-aligned choice; `Sg` would deviate from the pinned standard's recommended style.
- **Alternatives considered**: `Sg` (PascalCase namespace — rejected: deviates from NL.8/NL.10 recommended style); `SG` as a namespace (rejected: ALL_CAPS is reserved for macros per NL.9, and `SG::` reads as an acronym macro); `sgn` (rejected at plan review in favor of the shorter `SG_`/`sg` per user decision).

### R-013: Call-site micro-optimization synthesis (ranked techniques, empirical proof, divergences reconciled)
- **Decision**: The call-site design in the plan (statement-form macro, predicate-first, `[[unlikely]]` on the failing branch, header-inline `noinline` + `[[noreturn]]` dispatch as **primary**, out-of-line `source/dbc/dbc.cpp` dispatch as the **documented fallback**) is **confirmed and hardened** by a researched + empirically-verified technique inventory. No primary/fallback inversion. Two divergences raised by the external research are **resolved in favor of the approved spec**, and the research's own premise errors are corrected (below). The satisfying-path cost is proven to be **exactly one predicate evaluation plus one predicted branch**, with **no EH machinery at the call site**.
- **Ranked adopt/skip** (consolidated from the external call-site research, each verified against 2026-09-06 sources; the adopted set is the plan's design):

  | # | Technique | Verdict | Where it lands |
  |---|---|---|---|
  | 1 | Out-of-line / non-inlined cold `[[noreturn]]` dispatch | **ADOPT (foundation)** | `check_*` dispatch (header-inline `noinline` primary; TU out-of-line fallback) |
  | 2 | `[[unlikely]]` on the failing branch (C++20) | **ADOPT (sole hot-path hint)** | the `if (!(pred)) [[unlikely]]` statement |
  | 3 | `__attribute__((cold))` + `noinline` on dispatch | **ADOPT (guarded)** | feature-detected `SG_COLD`/`SG_NOINLINE` shim inside the dispatch TU |
  | 4 | `__builtin_expect` | **SKIP** | redundant with `[[unlikely]]`; non-portable |
  | 5 | `__builtin_unreachable()` / MSVC `__assume(0)` inside dispatch | **ADOPT (internal hygiene)** | guarded `SG_UNREACHABLE()` after `std::abort()` in the terminating dispatch |
  | 6 | `#pred` stringizing (zero hot-path cost) | **ADOPT (free)** | the `#pred` argument, evaluated only inside the failing branch |
  | 7 | `__FILE_NAME__` / file-static string dedup | **ADOPT (optional, size-only)** | optional `SG_THIS_FILE` shim; zero hot-path impact either way |
  | 8 | `do { if (…) } while (0)` + `[[unlikely]]` (over ternary) | **ADOPT** | the statement-form macro shape |
  | 9 | Preprocessor 4-semantics + `static_assert`/`if constexpr` elision | **ADOPT** | mirrors the libc++ ignore/observe/quick_enforce/enforce model; `ignore` → `((void)0)` (see premise correction C1) |
  | 10 | Trap vs `std::abort` termination primitive | **ADOPT: `std::abort()` default; trap opt-in for `quick_enforce`** | the default response (FR-008) |
  | 11 | `noinline` as the EH-safety mechanism | **ADOPT** | **not** an LTO net — this repo configures **no LTO**, so `noinline` exists to keep the dispatch frame (and its EH table) out of the caller |
  | 12 | `__builtin_assume` / `__assume` / `[[assume]]` | **SKIP** | unportable (Clang-only / MSVC-only / C++23), pessimizes layout, redundant with `[[noreturn]]`, `-Wassume` risk |
  | 13 | `-fno-exceptions` interaction | **no action** | the Rank-1 design already carries no EH at the call site |
  | 14 | Macro hygiene (`do-while(0)`, parens, `(void)0`, no `##__VA_ARGS__`, no C-cast) | **ADOPT (free)** | `SG_*` macro bodies |

- **Empirical verification (this repo's toolchain: GCC 16.2.1 + Clang 22.1.8, `-std=c++20 -O2`, project warning set)** — the design was compiled and the assembly inspected, not assumed:
  - **Satisfied path** of an external-linkage `hot(int x){ SG_REQUIRE(x>0,…); return x*2; }` is exactly `testl %edi,%edi` (predicate) + `jle .Lcold` (one predicted branch) + `leal` (result) + `ret`. **Zero `call`, zero diagnostic-argument evaluation** (the file/pred/msg/kind `leaq`s sit in the relocated cold block, `.p2align 4,,10` / `.p2align 3`). This is the FR-038/FR-040 guarantee, proven.
  - **No EH at the call site (FR-039, proven):** the caller's frame (`.cfi_startproc`…`.cfi_endproc`) carries **no `.cfi_personality` and no `.cfi_lsda`** — i.e. **no landing pad**. The EH machinery (personality reference + LSDA + `.section .gcc_except_table`) lives **entirely inside the dispatch function**, for **both** the terminating variant (default response → `abort`) and the returning variant (`observe`). The cold dispatch is where the only throw-capable edge (a test observer, FR-009) is quarantined, exactly as the plan states.
  - **Header-inline `noinline` primary achieves identical call-site cleanliness:** a `check_precondition` defined `inline SG_NOINLINE [[noreturn]]` in the header keeps the caller frame free of `.cfi_personality`/`.cfi_lsda` and is **not** inlined (emits its own comdat body). So the primary (header-only) design meets FR-039 without requiring the out-of-line TU — the TU dispatch remains the fallback for the assembly-smoke-test gate (SC-008), not the default.
  - **Extensions are feature-detected and compile under strict `-std=c++20`:** the guarded `SG_COLD`/`SG_NOINLINE`/`SG_UNREACHABLE` shim (`__has_attribute`/`__has_builtin`, degrading to nothing) compiles **clean** under `-std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wold-style-cast` on both GCC 16 and Clang 22. The **hot path itself uses only standard C++20** (`[[unlikely]]`, `do-while(0)`, `#pred`). The intrinsics are confined to the cold dispatch and are **not** a `CMAKE_CXX_EXTENSIONS=ON` requirement (they are per-compiler builtins/attributes detected at preprocessing time, not language extensions that change `-std`). **Governance:** this is a P0 critical-path optimization (Principle I.1), explicitly designated critical-path in the spec (FR-037…FR-040) and documented here — the constitution's "no compiler extension" clause (Additional Constraints) is not violated because the emitted standard-conformance is unchanged and the intrinsics degrade to nothing where unavailable.
  - **`ignore`-mode warning behavior (FR-034):** pure elision `((void)0)` is **silent** on predicate type-checking and emits no code; the research's Qt idiom `static_cast<void>(false && (pred))` **does** parse/type-check the predicate (it caught an injected `r > "str"` type error) **and** suppresses `-Wunused-variable`. Because the project's strict set uses **`-Wunused` but not `-Werror`**, the only cost of spec-literal pure elision is a *warning* (not a build failure) in the rare case where a local is used **only** inside a semantic-gated predicate that is elided in `ignore`. This is accepted, is consistent with the universal `#define NDEBUG` + `<cassert>` convention (predicate not evaluated in the disabled build), and is mitigated because the project's default dev/CI semantic is `enforce` (FR-016), so predicates are type-checked in the primary development configuration and at every `SG_*_ALWAYS` site.
- **Premise corrections to the external research report** (its Appendix A and a couple of Rank claims did not match this project's approved spec or the toolchain facts):
  - **C1 — `ignore` semantics (material):** the report's Appendix A attributes "ignore still evaluates the predicate" to speedgun-ng. **The approved spec is the opposite.** FR-012/FR-018/FR-037 and SC-002 are unambiguous: in `ignore`, a *semantic-gated* macro expands to **nothing** — predicate **not evaluated** and **no code emitted**, proven by a symbol-absence check on the release artifact. The report's Rank-9/14 "Qt warning-hygiene trick" (`static_cast<void>(false && (pred))`) **evaluates (parses/type-checks) the predicate** and is therefore **rejected** for this project as a FR-037 violation. The plan keeps spec-literal `((void)0)`. (This is the single place the research's higher-ranked technique is deliberately *not* adopted, and it is the constitution-correct call: the approved spec outranks the research ranking.)
  - **C2 — `[[cold]]`/`[[hot]]`:** these are **not** a standard C++ attribute at any version. `__attribute__((cold))`/`((hot))` is the only form (GCC/Clang); MSVC has no equivalent. Adopted only as a guarded `SG_COLD` shim (Rank 3), never as `[[cold]]`.
  - **C3 — assume/unreachable availability:** `__builtin_assume` is **Clang-only** (GCC has none; MSVC has `__assume`); MSVC has **no** `__unreachable` (use `__assume(false)` / `__fastfail`). Hence Rank 12 is **SKIP** and Rank 5 is a guarded `SG_UNREACHABLE()` (GCC/Clang `__builtin_unreachable`, MSVC `__assume(0)`), used only *inside* the terminating dispatch after `std::abort()`.
  - **C4 — LTO:** the report framed `noinline` partly as an "LTO net." This repo configures **no LTO** (verified across `CMakePresets.json`/`cmake/`), so `noinline` is adopted for its **EH-isolation** effect only (Rank 11), not for LTO.
  - **C5 — `noexcept`/`[[noreturn]]` placement (refines FR-039):** the dispatch is `[[noreturn]]` **and** `noexcept` **only in the terminating semantics** where the default response is `std::abort()` (which never returns and never throws). In `observe` — where a test observer **may throw** (FR-009) and that throw "propagates as the termination mechanism" (FR-014) — the dispatch must **not** be `noexcept`, or the observer's throw would be converted to `std::terminate` and would not propagate. The `[[noreturn]]` on the terminating path is a **layout hint** for the hot path (the check does not fall through); a test observer that throws is a test-only scenario and the throw occurs inside the cold frame, never at the call site (unchanged FR-039 property). This is a *refinement* of FR-039's "the dispatch is `[[noreturn]]` in terminating semantics," adding the `noexcept`-only-in-terminating qualifier; it is recorded for the plan and flagged at the plan gate (it does not change the approved spec's observable behavior).
- **Divergence reconciliation (the two design forks the external research surfaced, both resolved for the approved spec):**
  - **D1 — per-kind `check_*` vs a single kind-tagged `dbc_dispatch`:** the plan keeps **four per-kind** `sg::dbc::check_precondition/postcondition/invariant/assertion` functions as the **single registered enforcement function per kind** (FR-025) — the macro expands to a call of **exactly one** of them, which is what makes sites mechanically detectable and keeps `tools/dbc/macros.yaml` (FR-026) and both gate halves (FR-027/028) working. The research's single kind-tagged `dbc_dispatch(kind, …)` is **not** a public divergence: it is realized as the **internal** `sg::dbc::detail::dispatch(Kind, msg, file, line, pred)` that the four `check_*` functions forward to. The kind remains part of the stable site identity (FR-021). So: per-kind public surface (spec), kind-tagged internal sink (research), no conflict.
  - **D2 — header-inline `noinline` vs genuinely out-of-line TU dispatch:** **header-inline `noinline` remains primary** (FR-037 default form "at most standard headers," and SC-002's symbol-absence proof — an emitted-on-use inline dispatch is *absent* from a TU that has no live `check_*` call, which an always-compiled `source/dbc/dbc.cpp` dispatch would **not** be). The out-of-line TU dispatch (behind its own dedicated generated export header, FR-037 form b) is the **documented fallback**, engaged only if the header-inline shape shows EH artifacts in the Linux assembly smoke test (SC-008). The empirical proof above (header-inline caller frame is EH-clean) shows the primary is expected to pass the smoke test.
- **Verification (plan / test-plan hooks):** satisfied-path shape and no-EH-at-call-site → the Linux assembly smoke test (FR-039/SC-008, `tools/dbc/asm_smoke.sh`) and the measured overhead distribution (SC-008); standalone-header + zero-`ignore`-code → the standalone-compile test and the consumer-release symbol-absence job (FR-037/SC-002); `SG_*_ALWAYS` presence in `ignore` → the trap fixture (FR-036/SC-002). All four are already in the plan's test-case mapping; R-013 adds the empirical evidence that the *primary* design is expected to satisfy them.
- **Alternatives considered**: (a) inverting primary/fallback to out-of-line TU dispatch (the external research's literal framing) — **rejected**: breaks SC-002's symbol-absence proof (an always-compiled dispatch TU emits `check_*` symbols even in `ignore`) and needs an export header in the default case (FR-037 form b); header-inline satisfies both spec and measured cleanliness. (b) the Qt `false && (pred)` `ignore` idiom (Rank 9/14) — **rejected**: parses/type-checks the predicate, violating FR-037 "predicate not evaluated"; the project's `-Wunused`-not-`-Werror` set makes spec-literal elision's only cost a non-fatal, rare warning. (c) adopting `__builtin_assume`/`[[assume]]` (Rank 12) — **rejected**: unportable, pessimizes, redundant with `[[noreturn]]`. (d) `__builtin_expect` on the hot path (Rank 4) — **rejected**: redundant with `[[unlikely]]` and non-portable.

## Resolved Clarifications (none open)

The spec's `/speckit.clarify` (2026-09-06) resolved the four underspecified
areas (see `spec.md` → `## Clarifications`): the consumer-release CI job
(R-006), existing-code conformance in 001 (R-009), the `assert` primitive name
(R-007), and the `dbc-gate` CI job (R-008). No NEEDS CLARIFICATION markers
remain.
