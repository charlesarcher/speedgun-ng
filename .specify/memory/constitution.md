<!--
Sync Impact Report (constitution amendment)
===========================================
Version change: 2.3.0 -> 2.4.0 (MINOR: two new principles, X and XI,
covering anti-slop code discipline and discourse and prose standards,
written for C++23 plus Design By Contract, with machine-checkable
enforcement)

Modified sections:
  VIII. CI Quality Gates -> prose defects recorded at lint parity; the
  machine check for Principle XI is listed as deferred enforcement
  alongside the existing commit-template gate.

Added sections:
  X. Anti-Slop Code Discipline (X.1 surface assumptions, X.2 simplicity
  first, X.3 surgical changes, X.4 verifiable success)
  XI. Discourse and Prose Standards, Anti-Slop (XI.1..XI.6)

Removed sections: none.

Rationale: this project is written by an AI army under the README's own
description, so the characteristic failure modes of generated work are
first-order risks: silent interpretation choices, speculative
generality, drive-by diffs, completion claims without evidence, and
LLM prose tics. Principle II already forbids catching a violated
contract and Principle IV already bans content-free comments and
TODOs; the new principles close the remaining gaps (speculative
abstraction, unsound cast suppression used to silence diagnostics,
unverifiable success claims, and discourse habits). Enforcement parity
with lint is what makes them rules and not preferences.

Compatibility: X and XI add no requirement that contradicts I through
IX. Where they touch an existing principle they cross-reference it and
never restate it, keeping one canonical home per rule. Scope follows the
Principle IX scope rule: bug fixes and trivial changes may bypass; work
touching public API, behavior, or build configuration cannot.

Deferred / follow-up:
  - Machine enforcement of the commit template (commit-msg hook / CI
    commit lint) does not exist yet; authors MUST self-verify, and
    tooling enforcement will be delivered through a future spec.
  - Machine enforcement of Principle XI (prose lint over tracked Markdown,
    comments, and commit subjects, using the grep-checkable patterns
    listed in XI.6) does not exist yet; reviewers enforce it by hand
    until a future spec delivers the gate. It lands as one target shared
    with the commit-lint deferral above.
  - Grandfathered prose: text written before XI.1 contains banned
    tokens, including em-dashes in existing Principle headings. A change
    brings the lines it touches into compliance; a tree-wide sweep is a
    formatting-only change under Principle V and is scheduled separately.
  - DCRs and P2 exception justifications are tracked in the issue
    tracker; the exact label/convention is project policy, not
    governance.
  - Performance baseline infrastructure (per-platform baselines,
    min/max/n50/n99 reporting harness) does not exist yet; Principle
    VII mandates it and it will be delivered through a future spec.

History:
  2.4.0  2026-09-10  Anti-slop principles X and XI (code discipline +
                     discourse standards); prose enforcement listed as
                     deferred
  2.3.0  2026-09-09  Pin language to C++23; CMake >= 3.20
  2.2.1  2026-09-06  Consolidate docs into README.md; update
                     runtime-guidance cross-references
  2.2.0  2026-09-06  Commit message + linear-history standard
                     (MPICH-derived, Pull Request Quality)
  2.1.0  2026-09-06  SDD <-> R-DCUT artifact mapping, UML design
                      mandate, TDD execution mode
  2.0.0  2026-09-06  Wholesale redefinition from the software
                     engineering standards (DBC, R-DCUT, coverage
                     gates, performance discipline, hard CI gates)
  1.0.0  2026-09-06  Initial ratification from repository conventions
-->

# speedgun-ng Constitution

## Core Principles

### I. Standard-First Coding (NON-NEGOTIABLE)

All code conforms to a single, pinned coding standard; conformance is
enforced by tooling and is the calibration baseline for code review.

- The default rule set is the C++ Core Guidelines
  (isocpp.github.io/CppCoreGuidelines). The active revision is pinned in
  the repository's analysis configuration (`.clang-tidy` / presets) so
  that tooling and human review agree on one baseline; changing the pin
  is a governance action (see Governance).
- Rules apply in the following priority order (highest first):
  1. **P0 — Critical-path performance.** Techniques that optimize
     identified critical paths. Invoked lightly, only on code explicitly
     designated critical-path in the spec or design, and always
     documented. P0 never silently overrides the other principles: when
     it does, the override is recorded as a P2 exception.
  2. **P1 — Project conventions.** This constitution, established
     repository patterns, and — when contributing to an external project
     — that project's own conventions.
  3. **P2 — Documented exceptions.** Deviations from the P3 baseline.
     No exception exists without a written justification in the feature
     spec or design change request. No exceptions are currently
     registered.
  4. **P3 — Default rules.** The pinned C++ Core Guidelines, enforced by
     clang-tidy and cppcheck via the project presets.
- Code that is not C++23, or that requires a compiler extension, MUST NOT
  be merged (see Additional Constraints).

### II. Design By Contract (NON-NEGOTIABLE)

Design by contract is used for all code, from the first commit. It
complements the coding standard: contracts catch bugs early and often,
communicate intent, and narrow the testing surface.

- Every interface — every public function and method declaration —
  documents its preconditions, postconditions, and invariants via
  doxygen `\pre`, `\post`, and `\invariant`.
- Every implementation enforces the same contracts at runtime through
  the project's contract facility (a macro or equivalent mechanism).
- The contract primitives are **REQUIRE** (precondition: "is needed"),
  **ENSURE** (postcondition: "is provided / guaranteed"), and
  **INVARIANT** ("does not change"); they apply to functions, loops, and
  types.
- Contracts are NOT an error-handling mechanism. A contract violation
  aborts loudly, like a fuse; it is never caught, logged-and-continued,
  or otherwise softened.
- Contract checks MUST NOT emit any code in release builds: zero
  performance cost on critical paths. This applies to semantic-gated
  contract checks — those selected by the `ignore` / `observe` /
  `enforce` / `quick_enforce` evaluation switch. A contract MAY be
  explicitly designated always-on; always-on contracts are present and
  enforced in every build configuration, including release, and are the
  deliberate, sparing exception reserved for critical invariants that
  must hold even in release binaries. The macro registry and
  release-artifact verification MUST distinguish always-on sites from
  semantic-gated ones.
- The header documents the contract; the source enforces it. The contract
  facility MUST keep the contract stated in one place (single source of
  truth); documenting it in comments and enforcing a second copy is
  forbidden drift.
- Missing or unenforced contracts fail CI: doxygen warnings for missing
  `\pre`/`\post`/`\invariant` are reported to CI, and 100% DBC coverage
  (every interface carries and enforces its contracts) is a hard gate.
- 100% DBC coverage is a hard gate alongside the 100% line and branch
  coverage gates of Principle VI. (The standard's original formulation
  — 100% LOC plus 100% DBC as a substitute for branch coverage — is
  superseded: this project requires all three.) Contracts narrow the
  testing surface those gates must cover, which is what makes the
  combination achievable.

### III. R-DCUT Design Process

Every non-trivial feature is developed through the R-DCUT procedure:
Requirements → Design → Code → Unit Test. The Spec Kit workflow
(Principle IX) is the mandatory vehicle for R-DCUT, and every stage
produces its artifact in the canonical location given there.

- **Requirements.** Written in a rigorous, unambiguous EARS form —
  "when <trigger>, the <component> shall <response>", with the standard
  EARS variations for ubiquitous, state-driven, and optional behavior —
  accompanied by user stories. Every user story is testable and
  traceable to at least one requirement. Depth scales with product
  maturity; analysis paralysis is a failure mode and MUST be avoided.
- **Design.** UML-driven, and includes a test plan. The design MUST
  contain both views:
  - *Logical view* — what the feature is and how it behaves: component,
    class, sequence, and state diagrams where useful, with interfaces
    designed with their contracts (Principle II) before implementation.
  - *Physical view* — where the feature lives: module/namespace/file
    layout, build targets and link relationships, and the public API
    surface it adds.
- **Code.** C++23 (Principle I and all other principles), in
  `include/speedgun-ng/` for the public interface and `source/` for the
  implementation.
- **Unit Test.** In `test/`, registered with CTest, following Principle
  VI. Tests accompany the component through the process; they are not
  added afterwards.
- **TDD mode.** Test-first development is a permitted execution mode of
  R-DCUT: the unit tests are written and observed to fail (red) before
  the implementation they cover (green), then the pair is refactored.
  A test that passes before the implementation exists does not
  satisfy the requirement. When TDD is used for a feature, `plan.md`
  MUST record it.
- **Design Change Request (DCR).** Once a codebase is stable — defined as
  a tagged release — any design change to it requires a DCR
  (issue or spec, recorded before implementation starts). DCRs exist to
  prevent developer work being thrown away: no PR against a stable
  design is started without an approved DCR.

### IV. Documentation and Self-Documenting Code

- Code is self-documenting: descriptive variable and function names, no
  magic values (use named constants), and type-erased values
  (`void*`, raw integers, etc.) carry strong names plus strong contracts
  (Principle II) constraining their values.
- Content-free comments MUST NOT be added. Inline comments are limited
  and, where used, applied consistently so reviewers know where to look.
- Complicated inline logic is split into a well-documented routine with
  pre- and post-conditions rather than annotated in place.
- TODOs and FIXMEs MUST NOT appear in code. Work is either done or
  tracked in the issue tracker; TODOs in source generate reviewer chaos.
- All interfaces are documented with doxygen: descriptive text plus
  `\pre`/`\post`/`\invariant`. Implementations of documented interfaces
  need not be documented again.
- Public API is documented (the `docs` target, Doxygen + m.css). Where a
  project component has man pages, they are hand-written.

### V. Style and Formatting

- All code is formatted with the project's `.clang-format`; the
  `format-check` target MUST pass, and CI enforces style.
- Formatting-only changes are committed separately from content changes.
  Mixed reformat + content changes confuse review and make merges harder
  than they need to be; they MUST NOT be combined.
- Tooling integration is expected: `format-fix` for local fixing,
  clang-format in editors.

### VI. Test-Backed Code and Coverage (NON-NEGOTIABLE)

- New code ships with tests in the same change; every bug fix ships with
  a test that reproduces the bug before it is fixed.
- Coverage is measured with gcov/lcov via the `coverage` preset
  (`ENABLE_COVERAGE=ON`). The following are hard CI gates:
  - 100% line (LOC) coverage,
  - 100% branch coverage,
  - 100% DBC coverage (Principle II).
- The contract facility's own internal check machinery is excluded from
  coverage measurement (gcov exclusion) so the gates measure
  application code, not the fuse box.
- Tests MUST be deterministic and fast enough to run in every CI job.
- For a benchmarking framework, measurement correctness is a safety
  property: warm-up handling, statistical treatment, and methodology
  MUST be covered by tests or documented in the spec. Wrong benchmarks
  are worse than no benchmarks.

### VII. Performance Discipline

Benchmarking frameworks earn trust through trustworthy numbers; the
same rigor applies to how this project performs.

- Code identified as critical path MUST have performance test cases and
  per-platform baseline metrics. CI runs these metrics and fails the
  build on regression beyond the platform baseline.
- Critical-path goals: minimize branches and memory touches (the two
  biggest performance killers). Techniques: copy important small data to
  the stack so side-effecting methods do not regenerate loads; dispatch
  to optimized variants (late binding / virtual dispatch) instead of
  branching; do not over-inline — icache pressure is real.
- P0 performance techniques (Principle I) apply only to designated
  critical paths and are documented; they are the exception, not the
  default.
- Performance results are reported as distributions (min / max / n50 /
  n99 or equivalent), not single points. Long tails are investigated,
  not ignored: outliers are not "outliers" until they are proved to be.
- Baselines are per-platform (architecture, microarchitecture,
  platform thresholds file). Comparisons are produced against the
  previous build, against releases, and long-term (to detect metric
  creep). Efficiency data (instruction/memory analysis, cold-cache
  performance counters) is gathered for important critical paths where
  the tooling exists.
- Performance measurement requires noise-free execution (exclusive
  resources, pinned cores where relevant); measurements taken on
  contended systems MUST NOT update baselines.

### VIII. CI Quality Gates (NON-NEGOTIABLE)

Every change passes all of the following gates in CI; each is hard.

- Builds succeed for the developer and CI presets: Linux (GCC/Clang),
  macOS (AppleClang), Windows (MSVC). Build tooling is decoupled from
  the operating system target, and CI artifacts are re-creable
  interactively with the same presets (traceability).
- All tests pass (`ctest`).
- Sanitizer-clean: ASan/UBSan (`ci-sanitize`) report no errors.
- Static-analysis-clean: clang-tidy and cppcheck (per
  `CMakePresets.json` presets) report no new findings; the pinned C++
  Core Guidelines baseline (Principle I) is enforced by the same
  configuration.
- `format-check` and `spell-check` pass.
- Generated prose satisfies Principle XI. A discourse violation is a
  review defect at lint parity; the automated prose check is pending
  (Sync Impact Report, deferred items) and review carries it meanwhile.
- Coverage gates of Principle VI pass (100% LOC, 100% branch, 100%
  DBC).
- DBC completeness is checked (Principle II).
- Critical-path performance metrics are within baseline (Principle VII)
  once baselines exist.
- Code is designed for debug and tracing: debug builds expose full
  diagnostics, and critical-path code leaves a traceable footprint
  (no untraceable silent paths).
- Build and test results are recorded so regressions over time are
  visible; a pass/fail verdict is produced per change.
- Weakening a gate configuration silently is forbidden. Any exception is
  a P2 (Principle I) with written justification, and changing the gate
  set itself requires a constitution amendment.

### IX. Spec-Driven Development (NON-NEGOTIABLE)

The project is developed spec-driven with Spec Kit. For all non-trivial
work, the workflow is executed before code is written. It is the
execution vehicle for R-DCUT (Principle III), with this artifact
mapping:

| R-DCUT stage | Spec Kit step | Artifact and canonical location |
| ------------ | ------------- | ------------------------------- |
| Requirements | `/speckit.specify` | `spec.md` in `specs/NNN-feature-name/` |
| Design | `/speckit.plan` | `plan.md` in `specs/NNN-feature-name/` |
| Tasks | `/speckit.tasks` | `tasks.md` in `specs/NNN-feature-name/` |
| Code + Unit Test | `/speckit.implement` | `include/` + `source/` (code), `test/` (unit tests) |

- `spec.md` MUST contain the feature's EARS-formulated requirements and
  user stories (Principle III), together with scope, constraints, and
  acceptance criteria.
- `plan.md` MUST contain the UML-driven design — logical view and
  physical view — together with the test plan (Principle III).
- `tasks.md` MUST decompose the implementation into independently
  verifiable tasks, pairing each code task with the unit tests that
  cover it. In TDD mode the covering test tasks precede the code tasks
  they gate.
- `/speckit.implement` produces C++ code in its canonical locations and
  unit tests in `test/`, registered with CTest; every task is verified
  (build + `ctest --preset=dev`) before the next.
- A feature that goes through the workflow MUST produce all four
  artifacts; a missing artifact is a failed feature.
- Specs live under `specs/NNN-feature-name/`; the feature directory is
  the single source of truth for what and why, while code is the source
  of truth for how.
- `/speckit.clarify` is used for ambiguous specs and `/speckit.analyze`
  for cross-artifact consistency where valuable.
- **Scope:** bug fixes and trivial changes (typo, format, build fix)
  may bypass the full workflow; anything touching public API, behavior,
  or build configuration must not.

### X. Anti-Slop Code Discipline (NON-NEGOTIABLE)

Behavioral guardrails for every implementer, human or AI. They bias
toward caution over speed: minimal diffs, traceable intent, and rare
rework. Scope follows Principle IX: bug fixes and trivial changes call
for judgment; everything else obeys all four rules.

#### X.1 Think before coding: surface assumptions

- Assumptions are stated in writing before implementation: the spec, the
  plan, the commit body, or a comment adjacent to the affected lines.
- When a requirement admits several readings, each candidate is recorded
  and the chosen one justified. Silent selection is PROHIBITED.
- When a simpler approach exists than the one requested, the alternative
  is surfaced in writing before proceeding.
- When ambiguity blocks correctness (style disagreements do not count),
  the implementer names it and resolves it from an authoritative source:
  the governing spec, this constitution, or the established code
  pattern. The user is asked only when those three cannot settle it.

Rationale: hidden assumptions are the largest single source of rework,
and generated code hides them well. Written assumptions are auditable at
review and cheap to revisit.

#### X.2 Simplicity first: no speculative generality

- Implementations contain the minimum code satisfying the stated
  requirement and its covering tests.
- The following are PROHIBITED unless the spec asks for them or another
  principle requires them:
  - features beyond what the requirement specifies;
  - abstractions, indirection, or configurability serving one caller;
  - extension hooks, injection seams, template parameters, or
    customization points for use cases that do not exist yet;
  - overloads, specializations, or branches for inputs the call site
    excludes;
  - defensive handling of conditions a contract already rules out:
    Principle II makes a violated precondition abort, so a `try`/`catch`,
    an error code, or a fallback path around it is dead weight at best
    and a suppressed fuse at worst.
- Suppression of diagnostics is never silent. A `const_cast`,
  `reinterpret_cast`, a `std::any`/`std::variant` down-cast, a C-style
  cast, or a `(void)parameter` discard exists only where a contract makes
  it sound; each is a P2 exception (Principle I) with the justification
  written at the site, and each `NOLINT` carries its reason in the same
  comment.
- A `DoNotOptimize`-style barrier appears only where the compiler would
  otherwise eliminate the measured work. A barrier with nothing to defeat
  is noise, and noise in a benchmarking framework is a defect.
- A function roughly four times the length a senior engineer writes for
  the same problem is rewritten before merge.

Rationale: speculative generality is a maintenance liability that must be
re-justified at every API change, and it survives review precisely because
it looks like foresight. Banned here, it stops being a default.

#### X.3 Surgical changes: touch only what you must

- Every changed line traces to the requirement, task, or defect this
  change addresses. Diffs carrying drive-by refactors, adjacent comment
  polishing, or unrelated cleanup are rejected at review.
- Local style wins. Naming, indentation, comment conventions, and file
  organization in the edited file are matched even when the implementer
  prefers otherwise; adopting a different convention is a dedicated
  formatting-only change (Principle V).
- Working code is not refactored because another shape looks cleaner.
- Orphans created by this change are removed in this change: unused
  includes, variables, functions, template instantiations, and dead
  branches. Pre-existing dead code is filed as an issue and removed in
  its own change.
- Code you did not write is not "fixed" inside a change about something
  else (Principle V and Pull Request Quality).

Rationale: surgical diffs keep review tractable, `git bisect` reliable,
and rollback safe, which is what the linear-history rule depends on.

#### X.4 Goal-driven execution: define verifiable success

- Every non-trivial task is converted into a verifiable goal before
  implementation. "Make it work" is a non-goal.
- Multi-step work is recorded as a step and check plan in the plan, the
  tasks artifact, or the commit body:

  ```
  1. <step> -> verify: <observable check>
  2. <step> -> verify: <observable check>
  ```

- Checks are observable and binary: a test passes or fails, a command
  exits zero or non-zero, a diff is empty, output matches a fixture.
  Subjective checks ("looks right", "should be fine") are PROHIBITED.
- Standard conversions:
  - "Add validation" becomes "list the invalid inputs, write the tests,
    make them pass".
  - "Fix the bug" becomes "write the failing test, confirm it fails
    against the unfixed baseline, fix, confirm it passes" (Principle VI
    already requires the reproducing test).
  - "Refactor X" becomes "record the passing test set, refactor, confirm
    the identical set passes, add no new tests".
  - "Speed up the hot path" becomes "record the baseline distribution,
    change, confirm the improvement exceeds noise on the same platform"
    (Principle VII).
- Completion is reported with evidence: the command run, the exit code,
  the test name, or the output excerpt. A completion claim without
  recorded evidence is non-compliant.

Rationale: an AI army produces confident prose for free and verified work
only under obligation. Evidence is the difference between the two, and it
is the only thing Principle VIII's gates can consume.

### XI. Discourse and Prose Standards (Anti-Slop) (NON-NEGOTIABLE)

These rules govern every word generated in this repository, in every
channel: interactive replies, code comments, commit messages, specs,
plans, tasks, documentation, figure labels, and slide text. A reply typed
into a conversation is generated output and is held to the same standard
as a shipped document. Common LLM writing habits damage technical
discourse, so they are removed by rule. Ratified by the maintainer,
2026-09-10. This principle is the canonical home: other guides reference
it and never restate it.

#### XI.1 No em-dashes

Use a colon, a semicolon, a comma, or parentheses. The rule applies to
every generated sentence, including bullets and figure text. The en-dash
(U+2013) is permitted for numeric ranges alone (`P0–P3`, `C++11–C++23`);
it carries a precise meaning unrelated to the em-dash-as-connector habit.
ASCII `--` and `---` are forbidden in prose everywhere, including
Markdown source that a converter might rewrite.

Scope and grandfathering: the rule binds output generated after this
amendment. Text written earlier carries violations, this document's own
Principle headings among them, and those are pre-existing rather than
non-compliant. A change brings the lines it touches into compliance; a
tree-wide sweep is a formatting-only change under Principle V, scheduled
on its own.

#### XI.2 No contrastive framing

Never use "X, not Y", "X rather than Y", or "X instead of Y" as a
rhetorical device, and never structure a claim as "does this, not that".
State what the thing IS.
- Wrong: "the runner is a scheduler, not a thread pool"
- Right: "the runner dispatches benchmark executions onto a fixed pool of
  worker threads"

Where a distinction is technically load-bearing, each fact gets its own
sentence and both are stated on their own terms.

#### XI.3 Never vouch for truthfulness

Banned: "honest", "honestly", "to be honest", "candid", "candidly",
"frankly", "transparent", "transparently", "genuinely", "straight
answer", and any phrasing that certifies the truthfulness of a statement.
Vouching for one statement implies the others lack it. Every statement
here is grounded in evidence (X.4) or is labeled as an estimate with its
uncertainty; none needs a marker.

#### XI.4 No meta-editorializing

Do not narrate the authoring process, the reading process, or the framing
inside the artifact. Banned patterns: "in this section we", "this
document will cover", "my approach to this file", "what this would take",
"how we read your input", "as an AI", "I notice that", "let me walk you
through". State the content. The artifact is the content; it does not
describe itself.

#### XI.5 No filler, hedge, or marketing vocabulary

Filler and hedge drops: "it's worth noting", "importantly", "notably",
"essentially", "basically", "simply", "just", "very", "actually", "in
fact", "of course", "needless to say", "in order to".

Marketing vocabulary is banned from technical claims: "seamlessly",
"cutting-edge", "leverages", "world-class", "best-in-class",
"industry-leading", "robust", "blazing-fast", "elegant", "powerful". A
performance claim carries a number, a platform, and a distribution
(Principle VII). An interface claim carries a contract (Principle II).

Weak requirement language stays banned in EARS statements per Principle
III: "should", "may", "might", "approximately".

#### XI.6 Enforcement

- A violation of this principle in generated text is a defect at lint
  parity: reviewers reject it and the author fixes it before merge, the
  same way a clang-tidy finding is handled.
- Machine checks cover the grep-checkable subset: the em-dash code point,
  `--` or `---` in prose, `, not `, ` rather than `, ` instead of `, the
  XI.3 voucher list, the XI.5 filler list, and the XI.5 marketing list.
  Wiring is deferred to a future spec together with the commit-template
  lint (see the Sync Impact Report); until it lands, review enforces it.
- Exemptions: a banned token inside a verbatim quotation, a code span, a
  command, a file name, or a literal that is itself the subject under
  discussion. Mark the quotation as quoted.
- Principle XI binds generated output. It does not rewrite a contributor's
  personal voice in prose they author by hand, and it never weakens
  Principles IV or V.

## Refactoring and Evolution

- Refactoring is at the heart of a healthy codebase: the code you write
  will eventually be refactored by someone. Write code that makes that
  cheap.
- Refactoring for its own sake is forbidden — shipping work takes
  priority.
- Refactor with confidence: the coverage, DBC, and performance test
  suites (Principles VI and VII) are the safety net that makes
  refactoring possible.
- Complexity metrics (cyclomatic complexity, path length) are used to
  find refactoring targets, integrated with CI where available.
- Public API changes are breaking changes: version them deliberately,
  document them in the spec or DCR, and keep `VERSION`/`SOVERSION`
  consistent with the change.

## Pull Request Quality

- PRs are presentable and reviewable. The author runs the full local
  toolchain before requesting review: format, tests, sanitizers, static
  analysis, DBC checks. It is not the reviewer's job to find what the
  tools find.
- Work-in-progress PRs are marked `[WIP]` and do not sit abandoned;
  final PRs carry a complete description and a link to the tracking
  issue.
- Bug-fix PRs describe both the bug and how it is fixed.
- Every pushed commit MUST follow the commit message template and the
  commit-quality rules below.
  - **Template:**

    ```
    <Section>: <one-line imperative description>

    <body: the why - motivation, context, non-obvious consequences;
    wrapped at 72 columns; omitted only for genuinely trivial changes>

    Approved-by: <reviewer(s)>

    Fixes #123
    Refs: specs/NNN-name
    ```

  - **Title.** Format is `<Section>: <One Line Description>`, where
    Section names the area of the codebase (e.g. `CMake`, `Docs`,
    `runner`, `dbc`). Imperative mood: "Add X", "Fix Y" - never
    "Added X" / "Adds X" / "X was added". Keep the whole title to 50
    characters or fewer.
  - **Body.** A blank line separates the title from the body. The body
    states the *why*, is complete but concise, wraps at 72 columns, and
    uses correct punctuation and capitalization. Omit the body only for
    genuinely trivial changes.
  - **One idea per commit.** A commit is one logical change, sized so it
    can be rolled back on its own (if features A and B ship together and
    B must be reverted, A must not be lost). Each commit compiles and
    passes its tests so history is bisectable with `git bisect`.
  - **Separate concerns.** Refactoring (e.g. changing API call sites)
    goes in its own commit, separate from behavior changes. Do not
    reformat or "fix" code you did not write in the same commit as your
    change (keep reformatting separate) so authorship of each line stays
    clear.
  - **Issue references.** When a commit fixes an issue, use a GitHub
    keyword so the issue is linked (and auto-closed on merge): `Fixes
    #123`, `Resolves #123`, or `See #123` (or the full
    `<owner>/<repo>#123`). Reference the governing spec as `Refs:
    specs/NNN-<name>` where one exists.
  - **Approval footer.** Record `Approved-by: <reviewer(s)>` in the
    landing commit so the review history is preserved in `git log`.
  - **Linear history.** The base branch stays linear: rebase a PR onto
    the base branch (never merge the base into the feature branch),
    squash the PR's commits into logical blocks that each carry the
    template, and land them as a linear sequence - no gratuitous merge
    commits on the base branch. Vague messages ("fix stuff", "wip",
    "updates") MUST NOT be pushed; unfinished work stays in a `[WIP]`
    PR, not in pushed history.
  - **Immutability.** Before a PR is merged, its commits may be freely
    rewritten (squash/rebase) into logical blocks; the template applies
    to the result. History is more important than content: write for the
    future maintainer. Once merged, history is immutable and is
    corrected only by new commits.
- The main branch is not pushed to directly. All changes merge through
  reviewed PRs, and only designated maintainers merge.
- Reviewers review in a timely manner and give feedback rather than
  fixing the contributor's bugs; the contributor addresses the feedback.
- New code in a PR is covered by tests in the same PR.

## Additional Constraints

- **Language**: C++23 only (`CMAKE_CXX_EXTENSIONS=OFF`). The CI matrix —
  Linux (GCC/Clang), macOS (AppleClang), Windows (MSVC) — defines the
  supported platforms; new code must not break any of them.
- **Warnings and hardening**: the strict warning sets in
  `CMakePresets.json` (`-Wall -Wextra -Wpedantic -Wconversion -Wshadow
  -Wold-style-cast` family; `/W4 /permissive-` on MSVC) are preserved,
  as are the security-hardening flags (stack protector, control-flow
  protection, fortified Release builds, hardened linker flags).
- **Library-first**: functionality lives in the `speedgun-ng` library.
  Public API is exposed through `include/speedgun-ng/` only;
  implementation lives in `source/`. Symbol visibility is hidden by
  default via the generated export header.
- **Dependencies**: no new hard runtime dependencies without documented
  justification in the feature spec. The project currently has zero
  external runtime dependencies; keep it that way.
- **Build system**: CMake ≥ 3.20, preset-driven configuration.
  `CMakeUserPresets.json` is machine-local and must NEVER be checked
  into source control.
- **Licensing**: BSD 3-Clause. All contributed code is compatible and
  carries no additional license burden.

## Governance

This constitution supersedes all other development practices,
guidelines, and conventions of the project. Where a spec, plan, task, or
tooling configuration conflicts with this document, the constitution
wins.

- **Compliance**: code review and `/speckit.analyze` verify compliance
  with the principles above. Complexity, deviations, and P0
  invocations must be justified in the feature spec or DCR.
- **Standard baseline**: the pinned C++ Core Guidelines revision and
  the gate set (Principle VIII) are part of governance. Updating the
  pin or the gates is done by PR with written rationale and a version
  bump of this constitution.
- **Amendments**: any contributor may propose an amendment. Amendments
  must be documented in this file with a rationale, must bump the
  version (MAJOR for incompatible principle removals or redefinitions,
  MINOR for new principles or materially expanded guidance, PATCH for
  wording), and must update the `Last Amended` date below.
- **Runtime guidance**: see `README.md` for build/test and contribution
  instructions.

**Version**: 2.4.0 | **Ratified**: 2026-09-06 | **Last Amended**: 2026-09-10
