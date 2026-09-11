<!--
Sync Impact Report (2.4.1, PATCH): token compression. Every rule,
threshold, identifier, gate, and banned-word list survives; wording
tightened, rationale prose dropped, repeated rules given one canonical
home with cross-references. Amendment history: `git log --oneline --
.specify/memory/constitution.md`.

Open deferrals, binding until a spec lands them:
- Commit-template lint (commit-msg hook / CI) absent; authors
  self-verify (Pull Request Quality).
- Principle XI machine check absent; review enforces the XI.6
  grep-checkable subset. Both land together as one gate, specced in
  specs/002-prose-commit-lint.
- Principle VII baseline infrastructure absent; VII mandates it, a
  future spec delivers it.
- DCR and P2 exception label conventions: project policy, tracked in
  the issue tracker.
-->

# speedgun-ng Constitution

## Core Principles

### I. Standard-First Coding (NON-NEGOTIABLE)

Code conforms to one pinned standard, tool-enforced, the calibration
baseline for review.

- Default rule set: C++ Core Guidelines
  (isocpp.github.io/CppCoreGuidelines). Active revision pinned in the
  analysis configuration (`.clang-tidy`, presets) so tooling and review
  share one baseline; changing the pin is a governance action.
- Rule priority, highest first:
  1. **P0:** critical-path performance techniques. Invoked lightly, only
     on code designated critical-path in the spec or design, always
     documented. P0 overrides another principle only through a recorded
     P2 exception.
  2. **P1:** project conventions: this constitution, established
     repository patterns, and, contributing to an external project, that
     project's own conventions.
  3. **P2:** documented exceptions to the P3 baseline, each requiring
     written justification in the feature spec or design change request.
     None registered.
  4. **P3:** default rules: the pinned Core Guidelines, enforced by
     clang-tidy and cppcheck through the project presets.
- Code that is not C++23, or that needs a compiler extension, MUST NOT be
  merged (Additional Constraints).

### II. Design By Contract (NON-NEGOTIABLE)

DBC applies to all code, from the first commit: contracts catch bugs
early, communicate intent, narrow the testing surface.

- Every interface (every public function and method declaration) documents
  preconditions, postconditions, invariants with doxygen `\pre`, `\post`,
  `\invariant`.
- Every implementation enforces the same contracts at runtime through the
  project's contract facility (a macro or equivalent).
- Primitives: **REQUIRE** (precondition), **ENSURE** (postcondition),
  **INVARIANT**; they apply to functions, loops, types.
- Contracts are NOT an error-handling mechanism. A violation aborts
  loudly, like a fuse, never caught, logged-and-continued, or softened.
- Contract checks MUST NOT emit code in release builds: zero cost on
  critical paths. That covers semantic-gated checks, selected by the
  `ignore` / `observe` / `enforce` / `quick_enforce` switch. A contract
  MAY be designated always-on: enforced in every configuration including
  release, a deliberate, sparing exception for invariants that must hold
  in release binaries. The macro registry and release-artifact
  verification MUST distinguish always-on from semantic-gated sites.
- The header documents the contract, the source enforces it, and the
  facility MUST keep each contract stated in one place. A comment copy
  beside an enforced copy is forbidden drift.
- Missing or unenforced contracts fail CI: doxygen warnings for missing
  `\pre`/`\post`/`\invariant` reach CI; 100% DBC coverage (every interface
  carries and enforces its contracts) is a hard gate (VI).
- The standard's original formulation, 100% LOC plus DBC in place of branch
  coverage, is superseded: all three gates hold at once (VI). Contracts
  narrow the surface those gates cover, which is what makes the combination
  achievable.

### III. R-DCUT Design Process

Every non-trivial feature follows Requirements, Design, Code, Unit Test.
Spec Kit (IX) is the mandatory vehicle; each stage produces its artifact in
the canonical location given there.

- **Requirements:** EARS form, "when <trigger>, the <component> shall
  <response>", with the standard variations for ubiquitous, state-driven,
  optional behavior, plus user stories. Every story testable and traceable
  to at least one requirement. Depth scales with product maturity;
  analysis paralysis MUST be avoided.
- **Design:** UML-driven, with a test plan, containing both views.
  - *Logical:* what the feature is and how it behaves; component, class,
    sequence, state diagrams where useful; interfaces designed with their
    contracts (II) before implementation.
  - *Physical:* where it lives; module, namespace, file layout, build
    targets and link relationships, public API surface added.
- **Code:** C++23 (I and the rest), public interface in
  `include/speedgun-ng/`, implementation in `source/`.
- **Unit Test:** in `test/`, registered with CTest, following VI. Tests
  accompany the component through the process, arriving with it.
- **TDD mode:** a permitted execution mode of R-DCUT: tests written and
  observed failing (red) before the implementation that turns them green,
  then the pair refactored. A test passing before its implementation exists
  does not satisfy the requirement. Where a feature uses TDD, `plan.md`
  MUST record it.
- **Design Change Request (DCR):** for a codebase stable enough to be
  tagged, any design change requires a DCR, an issue or spec recorded before
  implementation starts. No PR against a stable design begins without an
  approved DCR; that is what keeps work from being thrown away.

### IV. Documentation and Self-Documenting Code

- Code is self-documenting: descriptive names, no magic values (named
  constants), and type-erased values (`void*`, raw integers) carrying strong
  names plus strong contracts (II) constraining them.
- Content-free comments MUST NOT be added. Inline comments are limited and
  applied consistently, so reviewers know where to look.
- Complicated inline logic becomes a well-documented routine with pre- and
  postconditions, in place of annotation.
- TODOs and FIXMEs MUST NOT appear in code: work is done or tracked in the
  issue tracker.
- Every interface gets doxygen: descriptive text plus
  `\pre`/`\post`/`\invariant`. Implementations of documented interfaces need
  no second documentation.
- Public API is documented (the `docs` target, Doxygen plus m.css). Man
  pages, where a component has them, are hand-written.

### V. Style and Formatting

- All code is formatted with `.clang-format`; `format-check` MUST pass, CI
  enforces style. `format-fix` locally and clang-format in editors are
  expected.
- Formatting-only changes are committed separately from content changes.
  They MUST NOT be combined (also X.3, Pull Request Quality).

### VI. Test-Backed Code and Coverage (NON-NEGOTIABLE)

- New code ships with tests in the same change; every bug fix ships with a
  test reproducing the bug before the fix.
- Coverage measured with gcov/lcov via the `coverage` preset
  (`ENABLE_COVERAGE=ON`). Hard CI gates: 100% line, 100% branch, 100% DBC
  (II).
- The contract facility's internal check machinery is excluded from coverage
  measurement (gcov exclusion), so the gates measure application code.
- Tests MUST be deterministic and fast enough to run in every CI job.
- Measurement correctness is a safety property in a benchmarking framework:
  warm-up handling, statistical treatment, methodology MUST be covered by
  tests or documented in the spec. Wrong benchmarks are worse than no
  benchmarks.

### VII. Performance Discipline

- Critical-path code MUST have performance test cases and per-platform
  baseline metrics; CI runs them and fails the build on regression beyond the
  platform baseline.
- Critical-path goals: minimize branches and memory touches. Techniques:
  copy important small data to the stack so side-effecting methods do not
  regenerate loads; dispatch to optimized variants (late binding, virtual
  dispatch) in place of branching; avoid over-inlining, icache pressure is
  real.
- P0 techniques (I) apply to designated critical paths only, documented;
  they are the exception.
- Results are reported as distributions (min / max / n50 / n99 or
  equivalent). Long tails are investigated: an outlier is not an outlier
  until proved one.
- Baselines are per-platform (architecture, microarchitecture, platform
  thresholds file). Comparisons run against the previous build, against
  releases, and long-term, catching metric creep. Efficiency data
  (instruction and memory analysis, cold-cache performance counters) is
  gathered for important critical paths where the tooling exists.
- Measurement requires noise-free execution (exclusive resources, pinned
  cores where relevant). Measurements from contended systems MUST NOT update
  baselines.

### VIII. CI Quality Gates (NON-NEGOTIABLE)

Every change passes all of the following; each is hard.

- Builds succeed for developer and CI presets on Linux (GCC/Clang), macOS
  (AppleClang), Windows (MSVC). Build tooling is decoupled from the OS
  target; CI artifacts are re-creable interactively with the same presets.
- All tests pass (`ctest`).
- Sanitizer-clean: ASan/UBSan (`ci-sanitize`) report no errors.
- Static-analysis-clean: clang-tidy and cppcheck (per `CMakePresets.json`)
  report no new findings, against the pinned Core Guidelines baseline (I)
  from the same configuration.
- `format-check` and `spell-check` pass.
- Generated prose satisfies XI: a discourse violation is a review defect at
  lint parity; the automated check is pending (Sync Impact Report) and review
  carries it meanwhile.
- Coverage gates of VI pass: 100% LOC, 100% branch, 100% DBC. DBC
  completeness is checked (II).
- Critical-path performance metrics stay within baseline (VII) once
  baselines exist.
- Code is designed for debug and tracing: debug builds expose full
  diagnostics; critical-path code leaves a traceable footprint, no
  untraceable silent paths.
- Build and test results are recorded so regressions over time are visible,
  with a pass/fail verdict per change.
- Weakening a gate configuration silently is forbidden. Any exception is a
  P2 (I) with written justification; changing the gate set itself requires a
  constitution amendment.

### IX. Spec-Driven Development (NON-NEGOTIABLE)

Development is spec-driven with Spec Kit; for all non-trivial work the
workflow runs before code is written. It executes R-DCUT (III):

- Requirements: `/speckit.specify` → `specs/NNN-feature-name/spec.md`
- Design: `/speckit.plan` → `specs/NNN-feature-name/plan.md`
- Tasks: `/speckit.tasks` → `specs/NNN-feature-name/tasks.md`
- Code and Unit Test: `/speckit.implement` → `include/` + `source/`, `test/`

- `spec.md` MUST contain the feature's EARS requirements and user stories
  (III) with scope, constraints, acceptance criteria.
- `plan.md` MUST contain the UML design, logical and physical views, with the
  test plan (III).
- `tasks.md` MUST decompose the implementation into independently verifiable
  tasks, each paired with its covering unit tests. In TDD mode the covering
  test tasks precede the code tasks they gate.
- `/speckit.implement` produces code in its canonical locations and unit
  tests in `test/`, registered with CTest; every task is verified (build plus
  `ctest --preset=dev`) before the next.
- A feature going through the workflow MUST produce all four artifacts; a
  missing artifact is a failed feature.
- Specs live under `specs/NNN-feature-name/`: the directory is the single
  source of truth for what and why, code for how.
- `/speckit.clarify` serves ambiguous specs, `/speckit.analyze` cross-artifact
  consistency, where valuable.
- **Scope:** bug fixes and trivial changes (typo, format, build fix) may
  bypass the full workflow; anything touching public API, behavior, or build
  configuration must not.

### X. Anti-Slop Code Discipline (NON-NEGOTIABLE)

Behavioral guardrails for every implementer, human or AI, biasing toward
caution over speed: minimal diffs, traceable intent, rare rework. Scope
follows IX: bug fixes and trivial changes call for judgment; everything else
obeys all four rules.

#### X.1 Think before coding: surface assumptions

- State assumptions in writing before implementing: the spec, plan, commit
  body, or a comment adjacent to the affected lines.
- A requirement admitting several readings: record each candidate, justify the
  chosen one. Silent selection is PROHIBITED.
- A simpler approach than the one requested: surface it in writing before
  proceeding.
- Ambiguity that blocks correctness (style disagreements do not count): name
  it, resolve from an authoritative source, the governing spec, this
  constitution, or the established code pattern. Ask the user only when those
  three cannot settle it.

#### X.2 Simplicity first: no speculative generality

- Implement the minimum code satisfying the stated requirement and its
  covering tests.
- PROHIBITED unless the spec asks or another principle requires:
  - features beyond the requirement;
  - abstractions, indirection, configurability serving one caller;
  - extension hooks, injection seams, template parameters, customization
    points for use cases that do not exist yet;
  - overloads, specializations, branches for inputs the call site excludes;
  - defensive handling of conditions a contract already rules out: II makes a
    violated precondition abort, so `try`/`catch`, an error code, or a
    fallback around it is dead weight at best, a suppressed fuse at worst.
- Diagnostic suppression is never silent. `const_cast`, `reinterpret_cast`,
  `std::any`/`std::variant` down-cast, C-style cast, or `(void)parameter`
  discard exist only where a contract makes them sound; each is a P2 (I) with
  the justification written at the site, and each `NOLINT` carries its reason
  in the same comment.
- A `DoNotOptimize`-style barrier appears only where the compiler would
  otherwise eliminate the measured work. A barrier with nothing to defeat is
  noise; noise in a benchmarking framework is a defect.
- A function roughly four times the length a senior engineer writes for the
  same problem is rewritten before merge.

#### X.3 Surgical changes: touch only what you must

- Every changed line traces to the requirement, task, or defect addressed.
  Diffs carrying drive-by refactors, adjacent comment polishing, or unrelated
  cleanup are rejected at review.
- Local style wins: naming, indentation, comment conventions, file
  organization in the edited file are matched even against a different
  preference. Adopting another convention is a dedicated formatting-only
  change (V).
- Working code is not refactored because another shape looks cleaner.
- Orphans this change creates are removed in this change: unused includes,
  variables, functions, template instantiations, dead branches. Pre-existing
  dead code is filed as an issue and removed in its own change.
- Code you did not write is not "fixed" inside a change about something else
  (V, Pull Request Quality).

#### X.4 Goal-driven execution: define verifiable success

- Every non-trivial task becomes a verifiable goal before implementation.
  "Make it work" is a non-goal.
- Multi-step work is recorded as a step and check plan in the plan, the tasks
  artifact, or the commit body:

  ```
  1. <step> -> verify: <observable check>
  2. <step> -> verify: <observable check>
  ```

- Checks are observable and binary: a test passes or fails, a command exits
  zero or non-zero, a diff is empty, output matches a fixture. Subjective
  checks ("looks right", "should be fine") are PROHIBITED.
- Standard conversions:
  - "Add validation" → "list the invalid inputs, write the tests, make them
    pass".
  - "Fix the bug" → "write the failing test, confirm it fails against the
    unfixed baseline, fix, confirm it passes" (VI already requires the
    reproducing test).
  - "Refactor X" → "record the passing test set, refactor, confirm the
    identical set passes, add no new tests".
  - "Speed up the hot path" → "record the baseline distribution, change,
    confirm the improvement exceeds noise on the same platform" (VII).
- Completion is reported with evidence: command run, exit code, test name, or
  output excerpt. A completion claim without recorded evidence is
  non-compliant.

### XI. Discourse and Prose Standards (Anti-Slop) (NON-NEGOTIABLE)

These rules govern every word generated in this repository, in every channel:
interactive replies, code comments, commit messages, specs, plans, tasks,
documentation, figure labels, slide text. A reply typed into a conversation is
generated output, held to the standard of a shipped document. Common LLM
writing habits damage technical discourse, so they are removed by rule.
Ratified by the maintainer, 2026-09-10. Canonical home: other guides reference
XI, they do not restate it.

#### XI.1 No em-dashes

Use a colon, semicolon, comma, or parentheses. Applies to every generated
sentence, bullets and figure text included. The en-dash (U+2013) is permitted
for numeric ranges alone (`P0–P3`, `C++11–C++23`); it carries a precise meaning
unrelated to the em-dash-as-connector habit. ASCII `--` and `---` are forbidden
in prose everywhere, Markdown source a converter might rewrite included.

Scope and grandfathering: the rule binds output generated after this
amendment. Text written earlier carries violations, this document's own
Principle headings among them, and those are pre-existing. A change brings the
lines it touches into compliance; a tree-wide sweep is a formatting-only change
under V, scheduled on its own.

#### XI.2 No contrastive framing

Never use `X, not Y`, `X rather than Y`, or `X instead of Y` as a rhetorical
device, and never structure a claim as "does this, not that". State what the
thing IS.
- Wrong: "the runner is a scheduler, not a thread pool"
- Right: "the runner dispatches benchmark executions onto a fixed pool of
  worker threads"

Where a distinction is technically load-bearing, each fact gets its own
sentence, both stated on their own terms.

#### XI.3 Never vouch for truthfulness

Banned: "honest", "honestly", "to be honest", "candid", "candidly", "frankly",
"transparent", "transparently", "genuinely", "straight answer", and any
phrasing certifying the truthfulness of a statement. Vouching for one statement
implies the others lack it. Every statement here is grounded in evidence (X.4)
or labeled an estimate with its uncertainty; none needs a marker.

#### XI.4 No meta-editorializing

Do not narrate the authoring process, the reading process, or the framing
inside the artifact. Banned patterns: "in this section we", "this document
will cover", "my approach to this file", "what this would take", "how we read
your input", "as an AI", "I notice that", "let me walk you through". State the
content. The artifact is the content, and it does not describe itself.

#### XI.5 No filler, hedge, or marketing vocabulary

Filler and hedge drops: "it's worth noting", "importantly", "notably",
"essentially", "basically", "simply", "just", "very", "actually", "in fact",
"of course", "needless to say", "in order to".

Marketing vocabulary is banned from technical claims: "seamlessly",
"cutting-edge", "leverages", "world-class", "best-in-class",
"industry-leading", "robust", "blazing-fast", "elegant", "powerful". A
performance claim carries a number, a platform, a distribution (VII). An
interface claim carries a contract (II).

Weak requirement language stays banned in EARS statements per III: "should",
"may", "might", "approximately".

#### XI.6 Enforcement

- A violation in generated text is a defect at lint parity: reviewers reject
  it, the author fixes it before merge, as with a clang-tidy finding.
- Machine checks cover the grep-checkable subset: the em-dash code point, `--`
  or `---` in prose, `, not `, ` rather than `, ` instead of `, the XI.3
  voucher list, the XI.5 filler list, the XI.5 marketing list. Wiring is
  deferred to a future spec with the commit-template lint, per the
  Sync Impact Report; until it lands, review enforces it.
- Exemptions: a banned token inside a verbatim quotation, code span, command,
  file name, or a literal that is itself the subject under discussion. Mark
  the quotation as quoted.
- XI binds generated output. It leaves a contributor's personal voice in prose
  they author by hand untouched, and never weakens IV or V.

## Refactoring and Evolution

- Refactoring is at the heart of a healthy codebase: the code you write will
  eventually be refactored by someone. Write code that makes that cheap.
- Refactoring for its own sake is forbidden; shipping work takes priority.
- Refactor with confidence: the coverage, DBC, performance suites (VI, VII)
  are the safety net.
- Complexity metrics (cyclomatic complexity, path length) find refactoring
  targets, integrated with CI where available.
- Public API changes are breaking changes: version them deliberately, document
  them in the spec or DCR, keep `VERSION`/`SOVERSION` consistent with the
  change.

## Pull Request Quality

- PRs are presentable and reviewable. The author runs the full local toolchain
  before requesting review: format, tests, sanitizers, static analysis, DBC
  checks. Finding what the tools find is the author's job.
- Work-in-progress PRs are marked `[WIP]` and do not sit abandoned; final PRs
  carry a complete description and a link to the tracking issue.
- Bug-fix PRs describe both the bug and the fix.
- Every pushed commit MUST follow the template and the rules below.
  - **Template:**

    ```
    <Section>: <one-line imperative description>

    <body: the why - motivation, context, non-obvious consequences;
    wrapped at 72 columns; omitted only for genuinely trivial changes>

    Approved-by: <reviewer(s)>

    Fixes #123
    Refs: specs/NNN-name
    ```

  - **Title:** `<Section>: <One Line Description>`, Section naming the area of
    the codebase (`CMake`, `Docs`, `runner`, `dbc`). Imperative mood: "Add X",
    "Fix Y", never "Added X", "Adds X", "X was added". Whole title 50
    characters or fewer.
  - **Body:** a blank line separates title from body. It states the *why*,
    complete but concise, wrapped at 72 columns, with correct punctuation and
    capitalization. Omit it only for trivial changes.
  - **One idea per commit.** One logical change per commit, sized to roll back
    alone (features A and B shipping together: reverting B keeps A). Each
    commit compiles and passes its tests, so history is bisectable with
    `git bisect`.
  - **Separate concerns.** Refactoring (changing API call sites) goes in its
    own commit, apart from behavior changes; the diff-level rule is X.3.
  - **Issue references.** Use a GitHub keyword so the issue links and
    auto-closes on merge: `Fixes #123`, `Resolves #123`, `See #123` (or full
    `<owner>/<repo>#123`). Reference the governing spec as `Refs:
    specs/NNN-<name>` where one exists.
  - **Approval footer.** `Approved-by: <reviewer(s)>` in the landing commit,
    so review history survives in `git log`.
  - **Linear history.** The base branch stays linear: rebase a PR onto the
    base (never merge the base into the feature branch), squash the PR's
    commits into logical blocks each carrying the template, land them as a
    linear sequence, no gratuitous merge commits on the base. Vague messages
    ("fix stuff", "wip", "updates") MUST NOT be pushed; unfinished work stays
    in a `[WIP]` PR.
  - **Immutability.** Before merge a PR's commits may be freely rewritten
    (squash, rebase) into logical blocks; the template applies to the result.
    History outlives content: write for the future maintainer. Once merged,
    history is immutable, corrected only by new commits.
- The main branch is not pushed to directly: all changes merge through
  reviewed PRs, and only designated maintainers merge.
- Reviewers review in a timely manner and give feedback, leaving the
  contributor to address it.
- New code in a PR is covered by tests in the same PR.

## Additional Constraints

- **Language:** C++23 only (`CMAKE_CXX_EXTENSIONS=OFF`). The CI matrix (Linux
  GCC/Clang, macOS AppleClang, Windows MSVC) defines supported platforms; new
  code must not break any.
- **Warnings and hardening:** the strict warning sets in `CMakePresets.json`
  (`-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wold-style-cast` family;
  `/W4 /permissive-` on MSVC) are preserved, as are the security-hardening
  flags (stack protector, control-flow protection, fortified Release builds,
  hardened linker flags).
- **Library-first:** functionality lives in the `speedgun-ng` library. Public
  API is exposed through `include/speedgun-ng/` only, implementation in
  `source/`. Symbol visibility hidden by default via the generated export
  header.
- **Dependencies:** no new hard runtime dependency without documented
  justification in the feature spec. Zero external runtime dependencies today;
  keep it that way.
- **Build system:** CMake ≥ 3.20, preset-driven configuration.
  `CMakeUserPresets.json` is machine-local and must NEVER be checked into
  source control.
- **Licensing:** BSD 3-Clause. All contributed code is compatible, with no
  additional license burden.

## Governance

This constitution supersedes all other development practices, guidelines, and
conventions of the project. Where a spec, plan, task, or tooling configuration
conflicts, the constitution wins.

- **Compliance:** code review and `/speckit.analyze` verify compliance.
  Complexity, deviations, and P0 invocations must be justified in the feature
  spec or DCR.
- **Standard baseline:** the pinned Core Guidelines revision and the gate set
  (VIII) are governance. Updating the pin or gates is done by PR with written
  rationale and a version bump of this constitution.
- **Amendments:** any contributor may propose one. Amendments must be
  documented here with rationale, must bump the version (MAJOR for
  incompatible principle removals or redefinitions, MINOR for new principles
  or materially expanded guidance, PATCH for wording), and must update the
  `Last Amended` date below.
- **Runtime guidance:** `README.md` for build, test, contribution instructions.

**Version lineage** (full rationale per amendment: `git log` for this file):

| Version | Date | Change |
| ------- | ---- | ------ |
| 2.4.1 | 2026-09-10 | token compression, no rule changed |
| 2.4.0 | 2026-09-10 | principles X (anti-slop code) and XI (discourse) |
| 2.3.0 | 2026-09-09 | language pinned to C++23, CMake >= 3.20 |
| 2.2.1 | 2026-09-06 | docs consolidated into README |
| 2.2.0 | 2026-09-06 | commit template and linear-history standard |
| 2.1.0 | 2026-09-06 | SDD to R-DCUT mapping, UML mandate, TDD mode |
| 2.0.0 | 2026-09-06 | redefinition on DBC, R-DCUT, coverage, CI gates |
| 1.0.0 | 2026-09-06 | initial ratification from repository conventions |

**Version**: 2.4.1 | **Ratified**: 2026-09-06 | **Last Amended**: 2026-09-10
