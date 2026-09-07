<!--
Sync Impact Report (constitution amendment)
===========================================
Version change: 2.2.0 -> 2.2.1 (PATCH: non-semantic refinement - the
runtime-guidance cross-references now point at the consolidated
README.md, which replaces the former standalone BUILDING.md,
CONTRIBUTING.md, and HACKING.md)

Modified sections:
  Governance -> "Runtime guidance" reference updated to README.md
  (build/test/contribution instructions now live there)

Added sections: none.
Removed sections: none.

Deferred / follow-up:
  - Machine enforcement of the commit template (commit-msg hook / CI
    commit lint) does not exist yet; authors MUST self-verify, and
    tooling enforcement will be delivered through a future spec.
  - DCRs and P2 exception justifications are tracked in the issue
    tracker; the exact label/convention is project policy, not
    governance.
  - Performance baseline infrastructure (per-platform baselines,
    min/max/n50/n99 reporting harness) does not exist yet; Principle
    VII mandates it and it will be delivered through a future spec.

History:
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
- Code that is not C++20, or that requires a compiler extension, MUST NOT
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
- **Code.** C++20 (Principle I and all other principles), in
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

- **Language**: C++20 only (`CMAKE_CXX_EXTENSIONS=OFF`). The CI matrix —
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
- **Build system**: CMake ≥ 3.14, preset-driven configuration.
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

**Version**: 2.2.1 | **Ratified**: 2026-09-06 | **Last Amended**: 2026-09-06
