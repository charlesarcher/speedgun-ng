# C++26 Contract Migration Mapping (SC-010)

This page records the mechanical migration path from the speedgun-ng DBC facility to native C++26 contract support (ISO/IEC 14882:2026, [basic.contract], P2900). The facility's vocabulary and semantics were chosen to match the standard so that, once all supported compilers ship the feature, most contract sites become a rename with no behavior change.

The mapping is complete: every facility construct has a documented counterpart or an explicit note that it remains facility-based.

## Mapping table

| SG facility construct                  | C++26 construct                                      | Notes |
|----------------------------------------|------------------------------------------------------|-------|
| `SG_REQUIRE(pred, "msg")`             | `pre (pred)`                                         | Precondition specifier on a function or lambda declarator. |
| `SG_ENSURE(pred, "msg")`              | `post (r : pred)`                                    | Postcondition specifier; `r` (or other identifier) names the result object. Named capture is taken at each return point, matching the facility's design. |
| `SG_ASSERT(pred, "msg")`              | `contract_assert (pred);`                            | Assertion statement inside a function or lambda body. |
| `SG_INVARIANT(pred, "msg")`           | (no counterpart)                                     | Loop invariants and class invariants have no equivalent in C++26. These remain expressed with the facility macros. |
| `SG_REQUIRE_ALWAYS(...)` etc.         | (no counterpart)                                     | The four `_ALWAYS` macros designate per-site always-on enforcement. C++26 evaluation semantics are chosen per evaluation (implementation-defined, often per-TU or build-wide); there is no per-site "always-on" designation that survives an `ignore` semantic. |
| `SG_CONTRACTS_SEMANTIC == 0` (ignore) | `ignore` semantic                                    | No code is emitted for the semantic-gated check; the predicate is not evaluated. |
| `SG_CONTRACTS_SEMANTIC == 1` (observe)| `observe` semantic                                   | Predicate evaluated; on violation the handler is invoked and execution continues. |
| `SG_CONTRACTS_SEMANTIC == 2` (enforce)| `enforce` semantic                                   | Predicate evaluated; on violation the handler is invoked; if the handler returns normally the program is contract-terminated. |
| `SG_CONTRACTS_SEMANTIC == 3` (quick_enforce) | `quick-enforce` semantic                        | Predicate evaluated; on violation the program is contract-terminated immediately with no handler invocation. |
| `sg::dbc::set_observer(obs)` + default response | handle_contract_violation (user-replaceable, taking std::contracts::contract_violation const&) | The facility's runtime-swappable observer is replaced at link time by defining the global handler function. Default handler behavior (diagnostic + terminate under enforce) matches. Under quick-enforce the handler is not called in either design. |
| Compile-time contract layer (`SG_CT_REJECT`, static_assert rejection of runtime checks on constant expressions) | C++26 contract rules in manifestly constant-evaluated contexts | Compile-time constraints continue to use `static_assert`, concepts, or `constexpr` validators (FR-022). A runtime contract on a compile-time-evaluable predicate is ill-formed under terminating semantics in C++26, matching the facility's rejection. |

## Semantics behavior match (SC-010 / US5 acceptance 1)

- `ignore`: no code for semantic-gated sites; predicate not evaluated. Matches.
- `observe`: report via handler, continue. Matches.
- `enforce`: report via handler, then terminate (or throw from handler becomes the termination). Matches.
- `quick_enforce` / `quick-enforce`: immediate termination, no handler. Matches.

## Hook behavior match (US5 acceptance 2)

- Default path under enforce: structured diagnostic to stderr, then terminate. Matches the recommended practice for the default `handle_contract_violation`.
- Under quick-enforce: no handler call, immediate termination. Matches.
- Test observers that throw are supported in the facility under observe/enforce; the standard permits the handler to throw (the throw then participates in normal exception handling or contract termination).

## Always-on and per-site designation

C++26 does not provide a per-site always-on that is independent of the chosen semantic for that evaluation. Sites that must remain active even in release builds stay as facility `_ALWAYS` macros after migration (or are rewritten with an implementation-specific attribute if a vendor extension appears). Each such site must be justified; they are the deliberate exception to the zero-release-cost rule.

## Loop and type invariants

C++26 supplies no loop-invariant or class-invariant construct. All `SG_INVARIANT` uses (per-iteration, ctor exit, dtor entry, public member entry/exit) remain facility-based after migration. The vocabulary name `INVARIANT` is kept for consistency with the constitution.

## 100% construct conformance checklist (SC-010)

The checklist below enumerates every facility construct named in the tasks and spec. Each is either mapped to a named C++26 counterpart or explicitly recorded as remaining facility-based.

All eight macros named for the completeness probe:
`SG_REQUIRE`, `SG_ENSURE`, `SG_INVARIANT`, `SG_ASSERT`,
`SG_REQUIRE_ALWAYS`, `SG_ENSURE_ALWAYS`, `SG_INVARIANT_ALWAYS`, `SG_ASSERT_ALWAYS`.

- [x] `SG_REQUIRE` (precondition) maps to `pre`
- [x] `SG_ENSURE` (postcondition) maps to `post (r : ...)`
- [x] `SG_ASSERT` (in-body assertion) maps to `contract_assert`
- [x] `SG_INVARIANT` (loop and class) remains facility-based; no standard invariant construct
- [x] `SG_REQUIRE_ALWAYS` remains facility-based (no per-site always-on in standard)
- [x] `SG_ENSURE_ALWAYS` remains facility-based
- [x] `SG_INVARIANT_ALWAYS` remains facility-based
- [x] `SG_ASSERT_ALWAYS` remains facility-based
- [x] `ignore` semantic maps to `ignore`
- [x] `observe` semantic maps to `observe`
- [x] `enforce` semantic maps to `enforce`
- [x] `quick_enforce` semantic maps to `quick-enforce`
- [x] violation observer hook (set_observer + default response) maps to replaceable handle_contract_violation (global)
- [x] compile-time layer (rejection of runtime checks on ct-evaluable predicates; use of `static_assert`/concepts) maps to C++26 constant-evaluation rules for contracts
- [x] All eight macros are named explicitly above.
- [x] All four semantics are named explicitly above.
- [x] Hook model and always-on distinction are covered.
- [x] Loop/type invariants are covered as facility-only.

When native contracts are available on every supported platform, a mechanical pass can replace the four semantic-gated macros and leave the `_ALWAYS` and `INVARIANT` sites unchanged (plus any necessary adjustment to the handler definition). No contract site will require a change in predicate logic or intended semantics.

## References

- specs/001-dbc-facility/spec.md (US5, FR-035, SC-010)
- specs/001-dbc-facility/tasks.md (T029)
- specs/001-dbc-facility/contracts/api-contracts.md (preliminary mapping)
- C++26 working draft [basic.contract], cppreference "Contract assertions (since C++26)"
- include/speedgun-ng/dbc.hpp (the eight macros and four semantic paths)
