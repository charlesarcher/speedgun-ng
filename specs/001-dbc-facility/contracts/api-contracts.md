# Public API Contracts: Design By Contract Facility

**Feature**: 001-dbc-facility | **Date**: 2026-09-06

The facility is part of the `speedgun-ng` library and is exposed through
`include/speedgun-ng/` only. This file documents the public API surface and the
contract each symbol carries. The `SG_` macro prefix keeps the constitution
vocabulary (REQUIRE / ENSURE / INVARIANT / assert) collision-free with
`<cassert>` and third-party macros; the prefix is a physical-view detail, the
vocabulary is fixed by constitution Principle II.

## Contract macros (the public vocabulary)

The macros are **statement-form** (not function calls) so that the predicate
is evaluated first and no diagnostic argument is evaluated on the satisfied
path (FR-038). Each expands to a call of exactly one registered enforcement
function (FR-025):

| Macro | Kind | Checked-build expansion (schematic) | Documented as |
|---|---|---|---|
| `SG_REQUIRE(expr, msg)` | precondition | `do { if (!(expr)) [[unlikely]] { sg::dbc::check_precondition(msg, __FILE__, __LINE__, #expr); } } while (false)` | `\pre` |
| `SG_ENSURE(expr, msg)` | postcondition | same shape, `check_postcondition` | `\post` |
| `SG_INVARIANT(expr, msg)` | invariant | same shape, `check_invariant` | `\invariant` |
| `SG_ASSERT(expr, msg)` | assertion | same shape, `check_assertion` | in-body |
| `SG_*_ALWAYS(expr, msg)` | (same kind as the base macro) | identical shape, **compiled outside the semantic `#if`** — active in every configuration (FR-036) | `\pre`/`\post`/`\invariant` + "always-on" note |

**Contract on every macro**:
- In an `ignore` build, a *semantic-gated* macro expands to `((void)0)` — the
  predicate is not even evaluated, zero code (FR-012/FR-018/FR-037).
- In a checked build the predicate is evaluated exactly once (FR-019); on
  failure the macro invokes the cold, non-inlined, `[[noreturn]]` (terminating
  semantics) enforcement function, which delivers a **Violation record** to the
  violation response (FR-002/003/006/007, FR-039/FR-040).
- `msg` must be a string literal (FR-020); `expr` must be pure (FR-019).
- The always-on family is present in every configuration, including `ignore`
  / release (FR-036), and is the deliberate exception to the zero-release-code
  guarantee; use sparingly and document each site.

## Standalone header (FR-037)

`dbc.hpp` is self-contained: it depends on **no project headers** (no library
export header). Primary design: standard headers only, with the dispatch as
`inline` + noinline functions and observer state in a function-local static
(shared across TUs by ODR). Documented fallback: out-of-line dispatch in
`source/dbc/dbc.cpp` behind a dedicated generated export header, if the
noinline-shim shape fails the assembly smoke test.

## Violation response API

| Symbol | Contract |
|---|---|
| `sg::dbc::violation_observer` (type) | A callable receiving a **Violation record** by const reference. |
| `sg::dbc::set_observer(obs)` | Installs the substitute observer (FR-009). In a terminating semantic the observer is called before termination; a test observer may record and throw. In `quick_enforce` the observer is **not** called (FR-013). |
| default response | Emits a structured diagnostic (kind, file, line, message, predicate) to standard error and terminates; does not throw and is not catchable by application code (FR-008/FR-010). |

**Invariants**: the response is a single global, replaceable hook
(FR-010); the response path does not re-enter the handler recursively (Edge
Case: violation inside the response); emission is best-effort and the outcome
remains termination.

## Build configuration

| Switch | Values | Effect |
|---|---|---|
| `speedgun-ng_CONTRACTS` (CMake) | `ignore` \| `observe` \| `enforce` \| `quick_enforce` | Selects the **Evaluation semantic** (FR-011). Independent of `NDEBUG` and of the optimization level (FR-017). |

- Developer / CI presets: `enforce` (FR-016).
- Release / consumer configuration: `ignore` (FR-016).
- The dedicated consumer-release CI job proves `ignore` yields a contract-free
  artifact (SC-002, research R-006).

## Machine-readable registry

`tools/dbc/macros.yaml` (the **Macro registry**, FR-026) lists each macro →
(enforcement function, kind). It is the single source of truth consumed by the
Phase 0 gate and feature 002's AST gate. No gate hard-codes a macro list.

## Conformed pre-existing interface

`exported_class` (the only pre-existing public class) is conformed to 100% DBC
in 001 (research R-009):

| Member | `\pre` | `\post` |
|---|---|---|
| `exported_class()` | none | `name()` returns the project name |
| `name() const` | object is in a valid state (class invariant) | returns a non-owning pointer to the stored string |

## Migration mapping (FR-035)

| Facility | C++26 counterpart |
|---|---|
| `SG_REQUIRE` | `pre (...)` |
| `SG_ENSURE` | `post (r : ...)` (named result capture) |
| `SG_ASSERT` | `contract_assert (...)` |
| `SG_INVARIANT` (loop / type) | — (remains facility-based; the standard has no invariant construct) |
| `SG_*_ALWAYS` (FR-036) | — (no standard counterpart; the standard's semantics are per-TU and cannot express a per-site always-on designation) |
| `ignore` / `observe` / `enforce` / `quick_enforce` | same-named evaluation semantics |
| `set_observer` / default response | replaceable `handle_contract_violation` hook |
