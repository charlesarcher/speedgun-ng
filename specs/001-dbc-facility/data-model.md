# Data Model: Design By Contract Facility

**Feature**: 001-dbc-facility | **Date**: 2026-09-06

The facility is a compile-time / runtime enforcement layer, not a data store.
Its "data model" is the set of value types and registries that the enforcement
machinery and the coverage gate operate on. Entities map 1:1 to `spec.md` →
Key Entities.

## Entities

### Contract
A declared obligation at a contract site.

| Field | Type | Notes |
|---|---|---|
| `kind` | `enum { precondition, postcondition, invariant, assertion }` | The four kinds (REQUIRE / ENSURE / INVARIANT / assert) |
| `subject` | `enum { function, loop, class, statement }` | What the contract is attached to |
| `predicate` | pure C++20 expression | The expression whose truth is required |
| `message` | string-literal constant | Diagnostic text; never a runtime expression (FR-020) |
| `location` | `(file, line)` | Source location of the site |

**Validation**: `predicate` must be pure and evaluated exactly once per check
(FR-019); `message` must be a string literal (FR-020).

### Contract site
The specific source location where a contract is declared and — for non-exempt,
non-compile-time contracts — enforced. The unit of the DBC coverage metric.

| Field | Type | Notes |
|---|---|---|
| `contract` | `Contract` | The obligation declared here |
| `enforcement_call` | registered function identifier | The enforcement function this site calls (FR-025) |
| `exempt` | `bool` | True if the enclosing declaration is on the closed exemption list (FR-029) |
| `compile_time` | `bool` | True if the constraint is expressed at compile time (FR-022/FR-024) |
| `always_on` | `bool` | True if the site is always-on (FR-036): present in every configuration, not stripped by the semantic switch, excluded from the zero-release-code guarantee |

**Validation**: every non-exempt, non-compile-time site calls exactly one
registered enforcement function (FR-025); a site with `compile_time == true`
must not have a runtime enforcement call (FR-023); a site with
`always_on == true` must use the `_ALWAYS` macro family and is verified
present in the consumer-release artifact (SC-002 exception, research R-011).

### Violation record
The structured data delivered to the violation response when a predicate fails.

| Field | Type | Notes |
|---|---|---|
| `kind` | `Contract.kind` | precondition / postcondition / invariant / assertion |
| `file` | `char const*` | `__FILE__` of the site |
| `line` | `unsigned` | `__LINE__` of the site |
| `message` | `char const*` | The contract's message (string literal) |
| `predicate_text` | `char const*` | The predicate expression, for diagnostics |

**Invariants**: carries the contract's stable identity — kind, file, line,
message (FR-021).

### Evaluation semantic
One of the four build-time modes, selected per build (not per site).

| Value | Behavior |
|---|---|
| `ignore` | No contract code emitted, no runtime checks (FR-012) |
| `observe` | Report the violation, then continue — test/diagnostic only, never the default (FR-015) |
| `enforce` | Report the violation, then terminate — project default (FR-014/FR-016) |
| `quick_enforce` | Terminate immediately, without invoking the hook (FR-013) |

**Validation**: exactly one semantic is active per build, selected by the
dedicated build switch, independent of `NDEBUG` and optimization (FR-011/FR-017).

### Macro registry
The machine-readable mapping (YAML) from contract macros to
(enforcement function, contract kind). The single source of truth for what
counts as a contract site.

| Field | Type | Notes |
|---|---|---|
| `macro` | `string` | e.g. `SG_REQUIRE`, `SG_ENSURE`, `SG_INVARIANT`, `SG_ASSERT` (and the `SG_*_ALWAYS` variants) |
| `enforcement_function` | identifier | e.g. `sg::dbc::check_precondition` |
| `kind` | `Contract.kind` | precondition / postcondition / invariant / assertion |
| `always_on` | `bool` | `true` for the `SG_*_ALWAYS` variants (same enforcement function and kind as the base macro); lets the registry, the gates, and the consumer-release job distinguish always-on sites (FR-036, SC-002 exception) |

**Consumers**: the documentation gate, the pairing check, and feature 002's AST
gate (FR-026).

### DBC matrix
The per-interface pairing of documented contract kinds with enforced contract
kinds, produced by the gate and published as a CI artifact.

| Field | Type | Notes |
|---|---|---|
| `interface` | qualified name | The public function or class |
| `documented_kinds` | `set<Contract.kind>` | Kinds present in the doxygen block |
| `enforced_kinds` | `set<Contract.kind>` | Kinds present as enforcement calls |
| `status` | `enum { ok, missing-doc, missing-enforcement, drift }` | Gate verdict |

**Validation**: the gate fails on any interface with `status != ok`
(FR-027/FR-028/FR-031).

### Exemption
A documented category of declaration excluded from documentation/enforcement
requirements (closed list).

| Value | Notes |
|---|---|
| private / protected member | Not public interface |
| lambda / local function | Not an interface |
| defaulted / deleted function | No body to enforce |
| friend declaration | Not an owned interface |
| constexpr-only interface | Constrained at compile time (FR-024) |

**Validation**: the list is closed; changes require a spec change or, after the
first tagged release, a DCR (FR-029).

## Relationships

```text
Contract site ──declares──▶ Contract
      │
      └──calls──▶ enforcement function (from Macro registry)

Violation record ◀──derived from── Contract.identity (on failure)

DBC matrix ──aggregates per interface──▶ (documented_kinds, enforced_kinds)
      │
      └──judged against──▶ Exemption list

Evaluation semantic ──governs all sites in a build (not stored per site)
```

- A **Contract site** declares exactly one **Contract** and calls exactly one
  registered **enforcement function** (from the **Macro registry**).
- A **Violation record** is derived from the failing **Contract**'s identity.
- A **DBC matrix** row aggregates one interface's documented and enforced kinds
  and is judged against the **Exemption** list.
- The **Evaluation semantic** governs the behavior of all sites in a build; it
  is not stored per site.
