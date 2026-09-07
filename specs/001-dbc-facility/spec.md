# Feature Specification: Design By Contract (DBC) Facility

**Feature Branch**: `001-dbc-facility`

**Created**: 2026-09-06

**Status**: Draft

**Input**: User description: "Design By Contract (DBC) facility — a header-only, zero-dependency, in-repo C++20 contract facility that documents and enforces REQUIRE/ENSURE/INVARIANT contracts (plus in-body assertions) on the public interface, per constitution Principle II. Violations abort loudly (fuse semantics) via a swappable fault handler emitting a structured diagnostic. Confirmed decisions: own thin macro facility (no Boost, zero external dependencies); semantics and build modes mirror C++26 contracts (ignore/observe/enforce/quick_enforce, replaceable violation hook) for a future mechanical migration; contracts on functions (pre/post), loops (per-iteration invariants), and types (class invariants at ctor exit, dtor entry, public function entry/exit); postconditions reference a named capture of the return value (no implicit old()); zero contract code in release via a dedicated build switch (not NDEBUG); checker-friendly design (every macro expands to a call of one registered enforcement function; macro→kind registry as machine-readable data); predicate hygiene (pure, exactly-once, string-literal message, stable identity); mandatory compile-time contract layer (static_assert/concepts/constexpr); Phase 0 DBC coverage gate ships with this feature (doxygen doc-presence gate + doc↔enforcement pairing gate in CI). Out of scope: custom clang-tidy AST coverage gate (feature 002), dynamic execution/vacuity coverage reporting, native C++26 compiler integration, function-contract subcontracting across inheritance."

## Clarifications

### Session 2026-09-06

- **Q: How should CI prove release builds contain zero contract code (SC-002)?** → **A:** A dedicated consumer-release CI job (Release configuration, developer mode off, `ignore` semantic) that builds the artifact exactly as a consumer would, symbol-inspects it for the absence of contract-machinery code, and runs a trap fixture that aborts in a checked build but must exit silently.
- **Q: Should 001 also conform the pre-existing public interface (`exported_class`) to 100% DBC before the gate lands?** → **A:** Yes — conform it in 001 (documented `\pre`/`\post` plus matching runtime enforcement) so the Phase 0 gate is green from the landing commit; no allowlist.
- **Q: What should the in-body assertion primitive be named (FR-001)?** → **A:** `assert` (macro `SG_ASSERT`), mapping 1:1 to C++26 `contract_assert`.
- **Q: Where should the Phase 0 DBC gate run in CI?** → **A:** A new `dbc-gate` job on every `pull_request` and `push` (ubuntu-only), separate from the master-only docs deploy job.
- **Q (plan review 2026-09-06): What performance properties must the contract call site have, and may some contracts stay on in release?** → **A:** The facility is extremely performance-sensitive and intended for pervasive use. Baseline requirements captured: (1) the public contract header is standalone with minimal dependencies; (2) the call site is a lightweight statement-form macro that evaluates the predicate first and evaluates no diagnostic argument on a satisfied check; (3) no exception machinery at the call site — the violation dispatch is an out-of-line cold `[[noreturn]]` function (any observer throw happens inside it); (4) the satisfied path is `[[likely]]`, the failing path `[[unlikely]]`; (5) a small set of contracts may be designated always-on, enforced in every configuration including `ignore`/release, compiler-eliminable-proof by observable side effects, and excluded from the zero-release-code guarantee. This list is the current baseline of a continuing call-site performance family; the design must keep the satisfied-check hot-path cost negligible as further requirements in the family are added.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Express and enforce contracts on the public interface (Priority: P1)

As a speedgun-ng developer, when I write or review a public function, loop, or class, I want to declare its preconditions, postconditions, and invariants so that intent is explicit, misuse is caught at the earliest possible moment, and the test surface is narrowed to what the contracts do not already rule out.

**Why this priority**: This is the constitution's non-negotiable (Principle II): every interface documents its contracts and every implementation enforces them, from the first commit. Nothing else in this feature is useful without the contract primitives working end to end.

**Independent Test**: Can be fully tested by writing a small fixture interface with a documented precondition, postcondition, loop invariant, and class invariant, building with contracts enabled, and observing (a) normal execution when contracts hold and (b) a loud, well-identified termination when each contract is violated. Delivers the core DBC value: intent documented once, enforced at runtime.

**Acceptance Scenarios**:

1. **Given** a public function with a documented precondition, **when** the precondition is violated in a contract-enabled build, **then** the program terminates with a diagnostic that identifies the contract kind (precondition), file, line, and the contract's message.
2. **Given** a public function with a documented postcondition over its return value, **when** the function returns a value that violates the postcondition, **then** the program terminates with a diagnostic identifying the postcondition at the return point.
3. **Given** a function whose contracts all hold, **when** it is called in a contract-enabled build, **then** it executes normally with no diagnostic output and no observable error state.
4. **Given** a class with a documented invariant, **when** a new object is constructed and the invariant does not hold at constructor exit, **then** the program terminates with a diagnostic identifying the class invariant.
5. **Given** a class with a documented invariant, **when** a public member function is entered or (for non-const functions) exited and the invariant does not hold, **then** the program terminates with a diagnostic.
6. **Given** a loop with a documented invariant, **when** the invariant does not hold at an iteration entry, **then** the program terminates with a diagnostic.
7. **Given** a constraint that is evaluable at compile time for a given instantiation (a type property or a template-parameter range or size relation), **when** the code is compiled in any configuration, **then** the constraint is verified at compile time with a clear diagnostic, and no runtime contract check exists for it.

---

### User Story 2 - Zero contract cost in release builds (Priority: P1)

As a maintainer and as a consumer of speedgun-ng, when the library is built for release, I want the produced binary to contain no contract-check code at all, so that benchmark measurements are never perturbed by the safety machinery and the release binary stays lean.

**Why this priority**: The constitution (Principle II) makes zero release cost non-negotiable, and a benchmarking framework's credibility rests on unmeasurable-noise-free binaries. A DBC facility that silently ships checks into release would corrupt the product's core purpose.

**Independent Test**: Can be fully tested by a measurable fixture: a contract site that provably traps in a contract-enabled (checked) build must run silently in a release build; additionally the release artifact is inspected for the absence of contract-machinery code/symbols. Delivers the zero-cost guarantee on its own.

**Acceptance Scenarios**:

1. **Given** the release configuration (contract semantic: ignore), **when** a contract site that would abort in a checked build is reached at runtime, **then** it executes normally and no diagnostic is produced.
2. **Given** the release configuration, **when** the library is built, **then** the resulting artifact contains no contract-check code (verified by the fixture and by symbol/code inspection of the artifact).
3. **Given** any optimization level and any state of the standard assertion-control macro, **when** the dedicated contract switch is set to ignore, **then** contract code is excluded regardless — the contract switch is fully independent of optimization and of the standard assertion macro, and vice versa.

---

### User Story 3 - Observe violations deterministically in tests (Priority: P2)

As a test author, when I need to assert that a contract is actually enforced (not merely documented), I want to install a substitute violation observer and inspect the structured violation record without killing the test process, so that every contract is proven to fail loudly when it should fail — the fail side of 100% DBC coverage.

**Why this priority**: A contract that is documented but never proven to fire provides no protection and would silently hollow out the coverage gate. Observability is what makes "every contract has an intentional-violation test" (the pass-side AND fail-side requirement) deterministic and automatable. It depends on Story 1 (working contracts) and enables the test strategy the coverage gate assumes.

**Independent Test**: Can be fully tested by installing a test observer, triggering a known violation, and asserting the observed record carries the correct kind, file, line, message, and predicate. Delivers the verification capability independent of the CI gate.

**Acceptance Scenarios**:

1. **Given** a test with a substitute violation observer installed, **when** a contract is violated in an observing configuration, **then** the observer receives a structured record containing the contract kind, file, line, message, and predicate text.
2. **Given** a test in a terminating configuration, **when** a contract is violated, **then** the process terminates via the default response (structured diagnostic plus termination) and the diagnostic names the kind, file, line, and message.
3. **Given** the default response, **when** a violation occurs, **then** the response does not throw an exception and is not catchable by application code — termination is the outcome.

---

### User Story 4 - CI fails on undocumented or unenforced contracts (Priority: P2)

As a maintainer, when CI evaluates any change, I want the build to fail if a public interface lacks documented contracts or lacks the runtime enforcement that matches its documentation, so that 100% DBC coverage (documented AND enforced, with no drift) is preserved from this feature forward instead of eroding silently.

**Why this priority**: Without the gate, Principle II's hard coverage requirement is unenforceable and the facility rots into "documents say one thing, code enforces another". This is the Phase 0 gate: an honest, real CI failure mechanism now, with a stricter AST-level hard gate (feature 002) replacing its weakest half later. It depends on Stories 1–2 (a facility whose sites are mechanically detectable).

**Independent Test**: Can be fully tested by seeding the tree with (a) a public function whose documentation is missing a precondition or postcondition section and (b) a documented contract with no corresponding enforcement in the definition, and observing CI fail on each with a diagnostic naming the offending interface. Delivers the enforceability guarantee.

**Acceptance Scenarios**:

1. **Given** a public function whose documentation block lacks a precondition section or a postcondition section, **when** the documentation gate runs in CI, **then** the build fails with a diagnostic naming the function and the missing section.
2. **Given** a public function whose documentation declares a contract kind for which no corresponding enforcement exists in its definition, **when** the pairing check runs in CI, **then** the build fails with a diagnostic naming the function, the kind, and the drift (documented-not-enforced or enforced-not-documented).
3. **Given** a fully documented and enforced public interface (including an explicit "none" marker for a genuinely empty contract set), **when** both gate halves run, **then** they pass.
4. **Given** an exempted declaration (private/protected member, lambda or local function, defaulted or deleted function, friend declaration, or a constexpr-only interface constrained at compile time), **when** the gates run, **then** it is not required to carry documentation or enforcement.
5. **Given** any gate run, **when** it completes, **then** a per-interface matrix (documented kinds vs enforced kinds) is produced as a CI artifact.

---

### User Story 5 - Migrate mechanically when language contracts land (Priority: P3)

As a future maintainer, when native language contract support becomes available on all supported compiler families, I want this facility's contract vocabulary and build-mode semantics to map one-to-one onto the standard's constructs (precondition/postcondition/in-body assertion; ignore/observe/enforce/quick_enforce; replaceable violation hook), so that migration is a mechanical rename rather than a redesign, and loop and type invariants (absent from the language) keep working unchanged.

**Why this priority**: Real value only when the compiler support arrives, but the cost of getting the naming and semantics right now is near zero, while retrofitting them later would touch every contract site in the codebase. Lowest priority because it constrains design choices rather than delivering current functionality.

**Independent Test**: Can be tested by a conformance review: the facility's documented build modes, semantic behaviors, and hook contract are checked against the standard's four semantics and handler model (a documented mapping table), and the macro vocabulary is checked against the standard's construct names. Delivers the migration guarantee.

**Acceptance Scenarios**:

1. **Given** the facility's four build-time semantics, **when** they are compared against the standard's ignore/observe/enforce/quick_enforce, **then** each name corresponds to exactly one standard semantic with matching behavior (ignore: no code; observe: report and continue; enforce: report and terminate; quick_enforce: terminate immediately without reporting).
2. **Given** the facility's violation-response hook, **when** compared against the standard's replaceable violation handler, **then** the behaviors match (default handler reports and terminates under enforce; immediate termination without handler under quick_enforce).
3. **Given** a documented migration mapping, **when** native support exists on all supported compilers, **then** each facility construct has a named standard counterpart, and loop/type invariant usage is documented as remaining facility-based (the language provides no invariant constructs).

---

### Edge Cases

- **Multiple return points**: a postcondition must be checked against the value actually returned at each return point; the named result capture is taken once per exit, and the predicate is evaluated exactly once per exit.
- **Violation inside the response**: if the default diagnostic output fails or a substitute observer misbehaves, the program still terminates; the response path does not re-enter the handler recursively.
- **noexcept context**: a violation inside a `noexcept` function terminates the program via the response; no exception escapes from a contract site.
- **Templates**: a contract documented on the primary template declaration is enforced from the template body and therefore applies to every instantiation; an instantiation that violates a contract aborts identically.
- **Virtual overrides**: each overriding implementation enforces the documented contract itself; derived classes do not inherit enforcement from a base implementation (subcontracting across inheritance is out of scope).
- **Const member functions**: class-invariant checks run at entry of const public member functions (exit checks apply to non-const public member functions), since a const function is not permitted to leave the object in a new, possibly invalid state.
- **Genuinely empty contract sets**: a public function with no meaningful precondition (or postcondition) documents "none" explicitly; the pairing gate must not demand enforcement for an explicitly empty contract, and must fail for an *implicitly* empty one.
- **Checked-build overhead on measured code**: contract checks in the default (enforce) build do execute inside measured regions during development; the added cost of the checked build must be quantified and documented (as a distribution) so that measured numbers are interpreted correctly, and the release (ignore) build must be provably zero-cost.
- **Handler re-entrancy across threads**: two threads violating simultaneously must not corrupt the diagnostic or deadlock; diagnostic emission is best-effort and the outcome remains termination.
- **Standard assertion macro interaction**: enabling/disabling the standard assertion macro (`NDEBUG`) must never change contract evaluation state, and the dedicated contract switch must never change standard assertion behavior.
- **Always-on check in release**: an always-on contract (FR-036) fires in a release (`ignore`) build exactly as in a checked build — it is the deliberate, sparing exception to zero-release-cost. The consumer-release trap fixture distinguishes an always-on site (must fire) from a semantic-gated site (must be silent).
- **Call-site cost on the hot path**: a satisfied contract check contributes only a predicate evaluation and a predicted-taken branch on the hot path (FR-038/FR-039/FR-040); because the facility is intended for pervasive use, the checked-build overhead of a satisfied check must remain negligible and be measurable (SC-008).

## Requirements *(mandatory)*

### Functional Requirements

**Contract primitives**

- **FR-001** (ubiquitous): The facility shall provide the contract primitives REQUIRE (precondition), ENSURE (postcondition), INVARIANT (invariant), and an in-body assertion named `assert` (macro `SG_ASSERT`), applicable to functions, loops, and types, with exactly this vocabulary (per constitution Principle II); the `assert` name maps 1:1 to C++26 `contract_assert` so the future migration (FR-035) is a mechanical rename.
- **FR-002** (event-driven): When a REQUIRE predicate of a public function evaluates to false in a contract-enabled build, the facility shall invoke the violation response with a record identifying the kind (precondition), file, line, message, and predicate text.
- **FR-003** (event-driven): When a function returns and its ENSURE predicate, evaluated against the named capture of the returned value, evaluates to false, the facility shall invoke the violation response with a postcondition record.
- **FR-004** (ubiquitous): Postconditions shall reference the return value only through a named capture taken at each return point; there shall be no implicit reference to prior state; any reference to pre-call state shall be an explicit named capture taken at function entry.
- **FR-005** (state-driven): While a loop carrying a documented INVARIANT is executing, the facility shall check the invariant at each iteration entry.
- **FR-006** (state-driven): While an object of a class carrying a documented INVARIANT exists, the facility shall check the invariant at constructor exit, at destructor entry, at entry of every public member function, and at exit of every non-const public member function.
- **FR-007** (event-driven): When an in-body assertion predicate evaluates to false in a contract-enabled build, the facility shall invoke the violation response with an assertion record.

**Violation response**

- **FR-008** (ubiquitous): The default violation response shall emit a structured diagnostic (kind, file, line, message, predicate) to standard error and then terminate the program; it shall not throw an exception and shall not be catchable by application code.
- **FR-009** (optional): Where a substitute violation observer is installed, the facility shall deliver the complete violation record to it before the default termination occurs; a substitute observer used in a test context shall be permitted to record the violation and to throw an observable exception.
- **FR-010** (ubiquitous): The violation response shall be a single global, replaceable hook; application code shall have no other means to intercept, catch, or continue past a violation in a terminating semantic.

**Evaluation semantics and build modes**

- **FR-011** (ubiquitous): The facility shall provide exactly four build-time evaluation semantics, named to match the standard's vocabulary: `ignore`, `observe`, `enforce`, and `quick_enforce`.
- **FR-012** (state-driven): While the `ignore` semantic is in effect, the facility shall emit no semantic-gated contract-check code and perform no semantic-gated runtime checks. Always-on sites (FR-036) are the deliberate exception and remain active.
- **FR-013** (state-driven): While the `quick_enforce` semantic is in effect, a violation shall terminate the program immediately without invoking the violation response hook.
- **FR-014** (state-driven): While the `enforce` semantic is in effect, a violation shall invoke the violation response hook and the program shall terminate (the default hook terminates; an observer's thrown exception propagates as the termination mechanism).
- **FR-015** (state-driven): While the `observe` semantic is in effect, a violation shall invoke the violation response hook and execution shall continue past the failed predicate; `observe` shall be documented as a diagnostic and test mode, never the project default.
- **FR-016** (ubiquitous): The default evaluation semantic for the project's developer and CI builds shall be `enforce`; the release configuration shall use `ignore`.
- **FR-017** (ubiquitous): The contract evaluation state shall be controlled by a dedicated build switch that is independent of the standard assertion-control macro and of the optimization level; changes to optimization or to the standard assertion macro shall not change contract evaluation, and changes to the contract switch shall not change standard assertion behavior.
- **FR-018** (ubiquitous): In any configuration using the `ignore` semantic, contract sites shall contribute zero executable code to the produced binary, and this shall be verifiable by (a) a fixture that provably traps in a checked build and runs silently in an ignore build, and (b) absence of contract-machinery symbols/code in the release artifact. Always-on contract sites (FR-036) are the deliberate exception: they are present in every configuration by design and are excluded from this zero-code guarantee.

**Predicate hygiene**

- **FR-019** (ubiquitous): Contract predicates shall be pure (no side effects) and the facility's contract sites shall evaluate each predicate exactly once per check.
- **FR-020** (ubiquitous): The message associated with a contract shall be a string-literal constant; runtime-evaluated message expressions shall not be supported.
- **FR-021** (ubiquitous): Every contract site shall have a stable identity composed of its kind, file, line, and message, and every violation record shall carry that identity.

**Compile-time contract layer**

- **FR-022** (unwanted-behavior): If a contract constraint is evaluable at compile time for a given instantiation (a type property, a template-parameter range, or a size relation), then the project shall express that constraint at compile time (concept, `static_assert`, or `constexpr` validator), active in every configuration at zero runtime cost.
- **FR-023** (unwanted-behavior): If a constraint has a compile-time expression, then expressing it only as a runtime contract check shall be a spec violation; the compile-time form is mandatory and the runtime form is forbidden for that constraint.
- **FR-024** (ubiquitous): Constexpr-only interfaces shall be constrained through the compile-time layer and shall be exempt from runtime contract enforcement.

**Checker-friendly design**

- **FR-025** (ubiquitous): Every contract macro shall expand to a call of exactly one registered enforcement function (one enforcement function per contract kind), so that contract sites are mechanically detectable without source-text heuristics.
- **FR-026** (ubiquitous): The project shall ship a machine-readable registry (data file) mapping each contract macro to its enforcement function and contract kind; the documentation gate, the pairing check, and the future AST-level hard gate (feature 002) shall consume this registry instead of hard-coding macro lists.

**Phase 0 DBC coverage gate**

- **FR-027** (event-driven): When the documentation gate runs in CI, it shall fail the build if any in-scope public function's documentation block lacks a precondition section or a postcondition section, or if a class that declares an invariant lacks an invariant section in its documentation, with a diagnostic naming the interface and the missing section.
- **FR-028** (event-driven): When the pairing check runs in CI, it shall fail the build if a documented contract kind on an in-scope public function has no corresponding enforcement of that kind in its definition, or if an enforcement of a contract kind has no corresponding documentation (drift in either direction), with a diagnostic naming the interface and the kind.
- **FR-029** (ubiquitous): The scope of the gates shall be all public functions and classes in the public headers; the closed exemption list shall be: private and protected members, lambdas and local functions, defaulted and deleted functions, friend declarations, and constexpr-only interfaces (FR-024); changes to this list require a spec change or, after the first tagged release, a design change request.
- **FR-030** (optional): Where a public function has a genuinely empty contract set for a kind, its documentation shall state "none" explicitly for that kind, and the pairing check shall not require enforcement for an explicitly empty contract while it shall fail for an implicitly empty one.
- **FR-031** (ubiquitous): The gates shall produce a per-interface DBC matrix (documented kinds vs enforced kinds) as a CI artifact for every run.

**Facility self-conformance**

- **FR-032** (ubiquitous): The facility's own internal check machinery shall be excluded from line and branch coverage measurement so that the coverage gates measure application code, not the fuse box (constitution Principle VI).
- **FR-033** (ubiquitous): The facility shall introduce no new external runtime dependencies; the project shall remain at zero external runtime dependencies.
- **FR-034** (ubiquitous): The facility shall compile without warnings under the project's strict warning settings, as C++20 without extensions, on every supported platform (Linux GCC/Clang, macOS AppleClang, Windows MSVC).
- **FR-035** (optional): Where native language contract support becomes available on all supported compilers, the facility's vocabulary and build-mode semantics shall map one-to-one onto the standard's constructs and semantics (FR-011–FR-015 counterpart constructs; replaceable violation hook), and a documented migration mapping shall be maintained; loop and type invariants shall remain facility-based and be documented as such.

**Call-site performance and always-on enforcement** (added 2026-09-06, plan review)

- **FR-036** (ubiquitous): The facility shall provide an always-on contract designation. A contract marked always-on is enforced in every build configuration (including `ignore`/release), is not stripped by the evaluation-semantic switch, and is guaranteed to be present in the emitted binary: because the always-on check has observable side effects on violation, it cannot be eliminated by the compiler. Always-on contracts are intended for a small number of critical invariants that must hold even in release; their use is deliberate and documented.
- **FR-037** (ubiquitous): The public contract header shall be self-contained with as few dependencies as possible: it shall not depend on the library's export header or on other project headers (at most standard headers, or — if the out-of-line dispatch fallback is chosen — its own dedicated generated export header). In the `ignore` configuration every semantic-gated contract macro shall expand to nothing (its predicate not evaluated and no code emitted); always-on macros (FR-036) remain active by design.
- **FR-038** (ubiquitous): A contract check's call site shall be lightweight and shall evaluate the predicate first, before any diagnostic argument is evaluated. On a satisfied check (the common path) no diagnostic argument shall be evaluated. Because C++ evaluates function arguments before the call, the check shall be structured (via a macro) so the predicate is tested first and only a failing check branches to the violation dispatch.
- **FR-039** (ubiquitous): The common (satisfied) path of a contract check shall contain no exception-handling machinery, and a violation shall not be signaled by a throw at the call site. The violation dispatch (record construction, observer invocation, termination) shall be out-of-line (a separate cold function) so that the call site retains no inlined landing pads or unwind tables. In terminating semantics the dispatch is `[[noreturn]]`; any throw (e.g., a test observer) occurs only inside the out-of-line dispatch.
- **FR-040** (ubiquitous): The satisfied path of a contract check shall be annotated likely and the failing path unlikely, so that the compiler lays out the hot path contiguously and places the cold dispatch out of line.

### Key Entities

- **Contract**: a declared obligation at a contract site; attributes: kind (precondition / postcondition / invariant / assertion), subject (function / loop / class / statement), predicate (the expression whose truth is required), message (string-literal text), location (file, line).
- **Contract site**: the specific source location where a contract is declared and (for non-exempt, non-compile-time contracts) enforced; the unit of the DBC coverage metric. An optional `always_on` designation marks the site as always-on (FR-036): present in every configuration and not stripped by the semantic switch.
- **Violation record**: the structured data delivered to the violation response when a predicate fails; attributes: contract identity (kind, file, line, message), predicate text, subject.
- **Evaluation semantic**: one of the four build-time modes (`ignore`, `observe`, `enforce`, `quick_enforce`) governing how violations are handled and how much code is emitted; selected by the dedicated build switch, not per site.
- **Macro registry**: the machine-readable mapping from contract macros to (enforcement function, contract kind) consumed by the gates; the single source of truth for what counts as a contract site.
- **DBC matrix**: the per-interface pairing of documented contract kinds with enforced contract kinds, produced by the gates and published as a CI artifact.
- **Exemption**: a documented category of declaration excluded from documentation/enforcement requirements (closed list per FR-029).

## Scope and Constraints

### In Scope

- The contract primitives REQUIRE / ENSURE / INVARIANT plus an in-body assertion, applicable to functions, loops, and types.
- The violation response: structured diagnostic, default terminate, replaceable observer hook, and the four build-time evaluation semantics.
- The dedicated contract build switch, independent of optimization and of the standard assertion macro.
- The compile-time contract layer (mandatory for every constraint evaluable at compile time).
- The checker-friendly macro design and the machine-readable macro registry.
- The Phase 0 DBC coverage gate: the documentation-presence gate and the doc-to-enforcement pairing check, wired into CI, with the per-interface DBC matrix artifact.
- The facility's own conformance to the project's quality gates (coverage self-exclusion, zero dependencies, warning-free on all supported platforms).
- Conformance of the pre-existing public interface (`exported_class`) to 100% DBC (documented `\pre`/`\post` plus matching runtime enforcement) so the Phase 0 gate is green from the landing commit (within the FR-029 scope).
- The documented migration mapping toward native language contracts.

### Out of Scope

- The AST-level hard coverage gate (custom static-analysis check with full fidelity) — feature 002, which supersedes the pairing check's coarser half.
- Dynamic contract-execution coverage and vacuity (never-fired contract) reporting — a later phase, report-only.
- Native C++26 compiler contract integration — deferred until all supported compiler families ship it (tracked by FR-035).
- Function-contract subcontracting across inheritance (derived implementations enforcing base function contracts) — a future feature or DCR.
- Static analysis or formal verification of contract satisfaction.
- Changes to the constitution, the C++ standard pin, or the CI gate set.

### Hard Constraints (from the constitution; non-negotiable)

- C++20 without compiler extensions; supported platforms are Linux (GCC/Clang), macOS (AppleClang), Windows (MSVC).
- Zero external runtime dependencies; no new hard runtime dependency without documented justification (this feature introduces none).
- Contracts are fuses, not error handling: the default response terminates; violations are never caught, logged-and-continued, or softened.
- Zero contract code in release builds (`ignore` semantic).
- Single source of truth: the header documents the contract and the implementation enforces the same contract; drift is forbidden and fails the gate.
- 100% DBC coverage is a hard CI gate alongside the 100% line and branch coverage gates.
- The facility lives in the `speedgun-ng` library and is exposed through the public header directory only.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: 100% of in-scope public interfaces in the facility's own headers carry documented contracts and matching runtime enforcement — the DBC matrix reports zero gaps and zero drift on the facility itself.
- **SC-002**: The release artifact provably contains zero semantic-gated contract-check code, verified by a dedicated consumer-release CI job (Release configuration, developer mode off, `ignore` semantic) that builds the artifact exactly as a consumer would, symbol-inspects it for the absence of contract-machinery code, and runs a trap fixture in which a semantic-gated site must exit silently while an always-on site (FR-036) must still fire. Always-on sites are the deliberate, sparing exception to the zero-release-code guarantee.
- **SC-003**: A contract violation in a contract-enabled build terminates the process with a diagnostic that names the kind, file, line, and message; the diagnostic identifies which predicate failed.
- **SC-004**: Every contract site in the facility has at least one passing-path test (contract holds, execution proceeds) and at least one intentional-violation test (contract fails, violation is observed or termination is verified) — 100% pass-side and fail-side contract exercise in the test suite.
- **SC-005**: The complete test suite passes with contracts enabled in the default `enforce` semantic; no test requires disabling contracts to pass.
- **SC-006**: The Phase 0 gate is proven effective: a seeded missing-documentation case and a seeded missing-enforcement case each cause the CI build to fail with a diagnostic naming the offending interface.
- **SC-007**: The facility builds warning-free on all four supported compiler families under the project's strict warning settings, with no compiler extensions.
- **SC-008**: The checked-build overhead of the contract machinery is quantified as a distribution (min / max / n50 / n99 or equivalent) against the same uncontracted code path, and the measurement is documented; the `ignore` build measures zero added cost.
- **SC-009**: The project's external runtime dependency count remains zero after this feature lands.
- **SC-010**: The documented migration mapping covers 100% of the facility's constructs (primitives, the four semantics, the violation hook) with named standard counterparts, so that no contract site requires a semantic change at migration time.

## Assumptions

- **Greenfield facility**: the project currently has no contract code; the facility's macro shape may be designed checker-friendly from the first commit (FR-025/FR-026) rather than retrofitted.
- **Native language contracts are not viable now**: as of 2026-09, no supported compiler family ships standard contract support on all of GCC/Clang/AppleClang/MSVC, and the standard has no loop or class invariant constructs; hence a macro facility now with a forward migration path (FR-035). The C++20-era "contracts" feature never shipped; the relevant standard is the C++26 contract assertions.
- **Vocabulary is fixed by the constitution**: REQUIRE/ENSURE/INVARIANT are the project's contract primitives (Principle II); the build-mode names follow the standard's vocabulary (`ignore`/`observe`/`enforce`/`quick_enforce`) to keep the future migration mechanical.
- **Fuse is the default, not the only, semantic**: the constitution's "violations abort loudly, never caught or softened" is the default (`enforce`) production behavior; the `observe` semantic exists strictly as a documented diagnostic/test mode and is never the project default (FR-015/FR-016).
- **Phase 0 gate tooling is dependency-free**: the documentation gate builds on the project's existing Doxygen documentation target, and the pairing check is a CI script over documentation output and the macro registry; no new external runtime or build-time dependency is introduced. The gate's enforcement-side fidelity is deliberately coarser than feature 002's AST-level gate; it is an interim hard gate, not the final one. It runs in a dedicated `dbc-gate` CI job on every `pull_request` and `push` (ubuntu-only), separate from the master-only docs deploy job, so it enforces per-PR.
- **"None" marker convention**: "none" (or equivalent explicitly-empty marker) is the accepted documentation form for a genuinely empty contract set of a kind (FR-030).
- **Templates**: documentation attaches to the primary template declaration; enforcement in the template body covers all instantiations; explicit specializations that alter the contract require their own documentation and enforcement.
- **No inheritance of enforcement**: each overriding implementation enforces the documented contract itself; function-contract subcontracting across inheritance is deferred (out of scope; a future feature or DCR).
- **Exemption list is closed**: the FR-029 list is the complete exemption set for this feature; expanding it is a spec change (or post-stabilization DCR).
- **Coverage exclusion is pre-arranged**: the coverage tooling (Principle VI) supports excluding the facility's internal check machinery from measurement, as the constitution already mandates.
- **Diagnostic output is best-effort**: the structured diagnostic is emitted to standard error before termination; in extreme failure states (e.g., I/O failure while reporting) termination still occurs.
- **The facility lives in the `speedgun-ng` library**: it is part of the single library's public interface, exposed through `include/speedgun-ng/`, not a separate component or package.
- **Checked-build numbers need interpretation**: development/CI builds run in `enforce`; any benchmark comparison involving a checked build must account for the measured overhead (SC-008); release measurements are unaffected (SC-002).
- **Release-clean proof is CI-gated**: the dedicated consumer-release CI job (developer mode off, `ignore`) verifies SC-002 on every change, so the zero-release-cost guarantee is enforced rather than merely documented (see Clarifications, 2026-09-06).
- **Always-on is the deliberate exception to zero-release-cost**: the zero-release-cost guarantee (FR-018/SC-002) applies to semantic-gated contracts. A small number of critical invariants may be designated always-on (FR-036) and are present in all builds, including release, by design; their use is deliberate, documented, and sparing.
- **The call site is performance-critical by intent**: the facility is meant to be used pervasively, so the satisfied-check hot path is a first-class design constraint (FR-038/FR-039/FR-040). The five captured call-site requirements (2026-09-06 plan review) are the current baseline of a continuing family; further requirements in the family are expected to land as spec deltas, and the design must not make them structurally expensive to add.
