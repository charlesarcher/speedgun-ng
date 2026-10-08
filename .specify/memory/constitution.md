<!--
Sync Impact Report (2.17.0, MINOR): V.2 gains the tag-object entry, names
the macros the build writes, and states the shape the kernel-name entry
reaches.

`.clang-tidy` exempted `hardStop` and `ring` through
`ConstexprVariableIgnoredRegexp` while V.2 held no entry naming them,
and V.2 says no exception exists outside its list. N-11 describes the
shape, so the list now carries the entry the enforcement cites.

The macro entry named `SPEEDGUN_NG_EXPORT` alone, while the export-header
generator writes a family and `cmake/variables.cmake` writes
`SPEEDGUN_NG_SUPPRESS_C4251`. The configuration exempted every
`SPEEDGUN_NG_*` name, a wider set than the entry described. The entry
now names the family, and the configuration is anchored to it. Include
guards carry the `SG_` prefix, and the five test fixtures that did not
carry it now do.

The kernel-name entry named the fields without naming the shape they
occupy, so the configuration exempted a parameter and a local constant
besides the member. The entry now states that the exemption reaches the
member spelling a mirror copies, and the configuration is anchored to
the field names.

The amendment adds one entry and completes two. It removes no entry and
weakens none. The version moves MINOR under Governance: an entry added
to an existing list.

The insertion shifts every line below line 1. A citation naming a line
number in this file needs re-anchoring; search for the named token.
-->

<!--
Sync Impact Report (2.16.0, MINOR): Principle V.1 gains the FR-012
obligation sentence, and N-1 names concepts and type traits.

`specs/014-identifier-naming-camelcase` T017 required the 2.14.0
amendment to state that a later spec, plan, local naming override, or
suppression cannot create a deviation, and that a suppression names
the exception entry it applies. V.1 carried no such sentence. FR-012
is a standing obligation of every later change, so it joins the law.

The spec's N-1 enumerates concepts and type traits among the types.
The 2.14.0 text omitted both shapes from the enumeration, so the
constitution covered fewer shapes than the rule it restates. The
clang-tidy 23.1.1 dump carries no `ConceptCase` key, so a concept
stays PascalCase under N-1 by review, and
`specs/014-identifier-naming-camelcase/contracts/naming-check.md`
records that silence. The enumeration now matches the spec.

The amendment adds one obligation and completes one enumeration. It
removes no obligation and weakens none. V.2 and the VIII gate list
stay as 2.14.0 wrote them. The version moves MINOR under Governance:
an obligation added inside an existing principle.

The insertion shifts every line below line 1. A citation naming a line
number in this file needs re-anchoring; search for the named token.
-->

<!--
Sync Impact Report (2.15.0, MINOR): Principle V.1 gains N-12, the
public data member rule. `.clang-tidy` has enforced the spelling since
the 2.14.0 amendment, through `PublicMemberCase: camelBack`, yet V.1
held no matching rule. V.1 names that shape a defect, so the rule
joins the law here and the key cites it. `specs/014-identifier-naming-camelcase`
Task T037 records the finding.

The amendment adds one rule to V.1. It removes no obligation and
weakens none. V.2 and the VIII gate list stay as 2.14.0 wrote them.
The version moves MINOR under Governance: expanded guidance, one
rule added.

The insertion shifts every line below line 1. A citation naming a line
number in this file needs re-anchoring; search for the named token.
-->

<!--
Sync Impact Report (2.14.0, MINOR): Principle V gains V.1 Identifier
Naming and V.2 Naming exceptions, the one spelling rule every C++
identifier the project owns follows, which
`specs/014-identifier-naming-camelcase` requires.

The addition binds every change. It removes no existing obligation and
weakens none: `.clang-tidy` already carried a partial naming map, and
this amendment states the rule that map implements.

V.1 states eleven rules and the constant spelling, and V.2 states the
closed exception list. Together they are the whole naming law. The
amendment adds one gate item to VIII: `readability-identifier-naming`
reports zero findings on owned code. The check is already in the
wildcard set, so it reports findings at the pre-rename heads; the
closing commit names it in `WarningsAsErrors`, and a finding then
fails the build.

The clause that governs this change is V.1's own: a rule that exists
only in `.clang-tidy` is a defect. This amendment is that clause's
route, and it lands in the same change as the configuration it governs.

Two closed specifications state a naming convention this amendment
supersedes: `specs/012-counters-defect-resolution` and the successor
log that cites it. Both stay unedited on the terms the 2.11.0 report set
for superseded merged artifacts. Each stated the convention that was true
when its spec shipped.

The Open deferrals block is left unaltered, and no entry in it changes.
Its wording reads "binding until a spec lands them", and every entry
records an obligation that no specification has delivered. The
obligation this amendment adds is delivered by the specification in the
first paragraph.

The insertion shifts every line below line 1. A citation naming a line
number in this file needs re-anchoring; the `runner` example token named
in `specs/002-prose-commit-lint/contracts/rule-data.md`,
`specs/002-prose-commit-lint/data-model.md`,
`specs/002-prose-commit-lint/research.md`, and
`tools/prose/prose_rules.yaml` moves again. Search this file for the
token; the number those four files name was already stale and the
correct anchor is the token's present position.

<!--
Sync Impact Report (2.13.0, MINOR): Principle VIII's hard gate list
gains a thread-sanitizer item, which FR-012 and SC-003 of
`specs/012-counters-defect-resolution` require.

The addition binds every change. It removes no existing obligation and
weakens none.

The gate list's last item governs this change.
It forbids weakening a gate configuration silently. It states that
"changing the gate set itself requires a constitution amendment".
The clause is unqualified and governs an addition and a removal alike.
This amendment is the route the clause names.

The gate names a `ci-tsan` preset and a `tsan` CI job, and neither
exists in the tree yet. They arrive with tasks T020 and T024 of
`specs/012-counters-defect-resolution/tasks.md`. The amendment and those
tasks land in the same change. A gate item naming a preset and a job
that do not exist is a gate no change can pass.

The 2.11.0 report states "all eleven jobs". That sentence records the
count at that amendment and stands as that record. This amendment adds
one job, and the 2.11.0 text stays unedited because an amendment report
is a record of a measurement.

Two closed specifications count CI jobs. `specs/010-linux-only/spec.md`
and `specs/007-counters-and-timers/tasks.md` name a total that this
amendment moves by one. Both stay unedited on the terms the 2.11.0
report set for superseded merged artifacts. Each count was true when its
spec shipped and stays a record of what was decided then.

The Open deferrals block is left unaltered, and no entry in it changes.
Its wording reads "binding until a spec lands them", and every entry
records an obligation that no specification has delivered. The
obligation this amendment adds is delivered by the specification in
the first paragraph.

The insertion shifts every line below line 1 and below the new gate
item. A citation naming a line number in this file needs re-anchoring.
The affected citations are the `runner` example token named in
`specs/002-prose-commit-lint/contracts/rule-data.md`,
`specs/002-prose-commit-lint/data-model.md`,
`specs/002-prose-commit-lint/research.md`, and
`tools/prose/prose_rules.yaml`. Two of them cite ranges:
`specs/002-prose-commit-lint/contracts/rule-data.md` cites "constitution
lines 395-397" for the XI.6 machine-string list, and
`specs/002-prose-commit-lint/research.md` cites
`constitution.md:387-476`. The `runner` token stands in the Pull Request
Quality section. The `:515` those four files name was already stale
before this amendment, so the correct anchor is the token's present
position. Search this file for the token; the number moves with every
edit to the reports above it.

Enforcement is by review, at lint parity. The 2.10.0 report places a
static-analysis finding there. The 2.12.0 report places a discourse
violation there. A race the thread sanitizer reports is a defect at lint
parity, and the author clears it before merge.

No tool changes. `specs/002-prose-commit-lint` enumerates Principle XI
prose rule identifiers, and a gate item in Principle VIII adds no
rule identifier, no vocabulary, no threshold, and no path.
`Constitution` is already an accepted commit-title section, so this
amendment's commit title passes the commit-message gate unchanged.

MINOR because one hard gate item was added inside an existing principle.
No principle is added, removed, renamed, or renumbered. No gate,
threshold, warning class, analyzer invocation, runner, or dependency is
removed or weakened; the gate set grows by one item. Amendment history
lives in the git log of this file.
-->
<!--
Sync Impact Report (2.12.0, MINOR): XI gains XI.7, Simplified Technical
English. Generated prose in this repository must reach about 80 percent
compliance with ASD-STE100.

The rule binds the channels XI already binds: interactive replies, code
comments, commit messages, specs, plans, tasks, documentation, figure
labels, and slide text. It adds an obligation and removes none.

The two rule sets overlap and they conflict in places. ASD-STE100
prefers `instead of` and a few other constructions that XI.2 and XI.5
ban, and XI.5 bans weak requirement words where the standard prefers
them. XI.1 through XI.6 therefore govern, and XI.7 applies where those
subsections are silent. The amendment weakens no existing rule and
creates no exception to one.

Scope matches the grandfathering clause in XI.1. The rule binds prose
written after this amendment. Earlier text keeps its violations, and a
change brings the lines it touches into compliance. No earlier artifact
needs a rewrite of substance.

This amendment shifts every line below its insertion points, so a
citation that names a line number in this file needs re-anchoring, on the
terms the 2.9.0 and 2.10.0 reports set. The affected citations are the
`runner` example token named in `tools/prose/prose_rules.yaml`,
`specs/002-prose-commit-lint/contracts/rule-data.md`,
`specs/002-prose-commit-lint/data-model.md`, and
`specs/002-prose-commit-lint/research.md`, plus the ledger in
`specs/007-counters-and-timers/citations.md`. The `:515` those four
files name was already stale before this amendment, so the correct anchor
is the token's present position. Search this file for the token; the
number moves with every edit to the reports above it.

Enforcement is by review, at lint parity, on the terms XI.6 sets for
every other violation in this principle. Most XI.7 rules are a judgment
about a sentence, so no mechanical check covers them. The wordy
connectives that a matcher can bound are the exception: they sit in XI.5's
filler list, `prose-lint` reports them from `tools/prose/prose_rules.yaml`
over added and modified lines, and the fixtures in `specs/002` assert each
one in both directions. This amendment adds no rule identifier, so the
canonical-id coverage probe is unchanged.

MINOR because guidance expanded materially inside an existing principle.
No principle is added, removed, renamed, or renumbered. No gate,
threshold, warning class, analyzer invocation, job, runner, or dependency
is added, removed, or weakened, and the gate set is unchanged.
Amendment history lives in the git log of this file.
-->
<!--
Sync Impact Report (2.11.0, MAJOR): Principle VIII's hard gate list and the
Additional Constraints platform definition name Linux alone, and IX's
per-feature release-build clause names the `ci-ubuntu` preset only. Linux is
the platform the project builds, tests, and gates; macOS and Windows are
unsupported and a future specification adds them.

The obligation this removes is measured. At the 2.10.0 gate set every change
had to build on developer and CI presets for Linux, macOS, and Windows. The
matrix has never run a macOS or Windows runner: all eleven jobs run on
`ubuntu-26.04`, one of them in a Rocky Linux container, and amendment 2.8.0
already recorded the absent macOS runner as a deferral. The gate list
therefore named two platforms no job has ever tested, which is a weaker claim
than the project makes everywhere else.

This report supersedes the effect of two earlier reports, and both stay in
this file with their rows in the lineage table. Amendment 2.7.0 suspended the
Windows MSVC preset-build gate for the vendored autotools lifetime, and
amendment 2.8.0 recorded the macOS gate as developer-local until a runner
specification lands. Both described a platform the project no longer claims.
Their rows stay because the record that those gates were once suspended, and
why, is worth keeping.

Closing the Open deferrals entry for the macOS runner is what retires
`specs/009-vendor-quill` T038, which asked for a macOS build result. T038
closes by reference through this amendment and needs no edit to any merged
file, so that feature's task file keeps it marked open and a reader there
learns of the supersession from here.

Five merged vendor specs stated, each as a Fixed decision, that Linux is the
enforced gate, that macOS must keep building for developers, and that Windows
stays possible by design: `specs/003-vendor-hwloc`,
`specs/004-vendor-simdjson`, `specs/005-vendor-hdrhistogram`,
`specs/006-vendor-yaml-cpp`, and `specs/009-vendor-quill`. Those statements
were true when each spec shipped and stay true as records of what was decided
then. This amendment supersedes them prospectively and none of the five is
edited.

The on-ramp for a future port, which is what makes this policy reversible by
reading: re-add the platform preset, configure, build, and add a runner. The
presets this amendment's scope removed from `CMakePresets.json` are
`flags-appleclang`, `flags-msvc`, `ci-darwin`, `ci-win64`, `ci-macos`,
`ci-windows`, and `ci-multi-config`, the last of which existed only to shorten
Xcode and Visual Studio builds. Every per-platform block in the tree is
untouched by this amendment and by the change that carries it: the `if(WIN32)`,
`if(APPLE)`, `if(MSVC)`, and `if(UNIX)` branches in `CMakeLists.txt` and
`cmake/`, every `_WIN32` preprocessor branch, and the Windows on-ramp abort in
`cmake/ImportAutotoolsSubmodule.cmake` that names the upstream
`contrib/windows-cmake/` wrapper. A port consumes those blocks as they stand.
No sentence here claims a port is scheduled, funded, or planned.

MAJOR because narrowing a NON-NEGOTIABLE principle's gate list withdraws an
obligation, which Governance classifies as an incompatible principle removal
and not as expanded guidance. No principle is added, removed, renamed, or
renumbered. No gate, threshold, warning class, analyzer invocation, job, or
dependency is added, removed, or weakened; the gate set's content is identical
and only its platform enumeration narrows. Amendment history lives in the git
log of this file.
-->

<!--
Prior report (2.10.0, MINOR): Principle VIII's static-analysis
clause states that the gate reports and names where a reader collects
the report, and the report records the measurement behind that
statement. Both analyzers already ran in the `test` job: the job
installs them, configures `ci-ubuntu`, and every compile of every
translation unit hands `CMAKE_CXX_CLANG_TIDY` and `CMAKE_CXX_CPPCHECK`
to the compile launcher, with no `--error-exitcode` in the cppcheck
launcher and `WarningsAsErrors: ''` in `.clang-tidy`. Measured on
2026-09-28 at this repository's head, one translation unit,
`source/counters/plan.cpp`, exits 0 with 92 `warning:` lines under the
configured launcher form and exits 1 on the same 92 once
`--warnings-as-errors=*` is added; the same holds for cppcheck, which
exits 0 on its findings and exits 1 under `--error-exitcode=1`. The
competing reading would make a finding fail the `test` job, and it was
measured as unviable at this head: `run-clang-tidy` over the 92
translation units of `build/dev/compile_commands.json` reports 28821
`warning:` lines, 23700 of them from the vendored trees under
`external/`, so the build would fail on its first translation unit
unless the vendored trees left the launcher, and no analyzer call is
suppressed and no warning class demoted to get there. The clause's
obligation is unchanged, no analyzer invocation, warning class, gate,
job, runner, or dependency changes, and the gate set is unchanged. This
report shifts every line in the file, so a citation into the
constitution, including the Phase 41 citations in
`specs/007-counters-and-timers/tasks.md`, needs re-anchoring by the
next convergence pass. Amendment history lives in the git log of this
file.
-->

Prior report (2.9.1, PATCH): Principle IX's per-feature release build
clause names the preset for the platform it runs on: `ci-ubuntu` plus
`cmake --build build` on Linux, `ci-macos` on macOS, and `ci-windows` on
Windows where the gate is reinstated. 2.9.0's MUST named no platform while
its parenthetical named one, so a macOS developer's prescribed command
configured the `Unix Makefiles` generator with the flag set inherited
through `ci-linux` and carried a `CMAKE_BUILD_TYPE` the `Xcode` generator
of `ci-macos` ignores, and AGENTS.md carried the same single-preset
wording four lines above the sentence naming macOS and Windows. The
obligation is unchanged, one release-configuration build per feature on
its own platform, and the Linux command is the one commit a6d26bf measured,
so this is a clarification of a defective clause and not an expansion of
the obligation. No gate, job, runner, or dependency was added; Principle
VIII's hard gate list is unchanged; the macOS runner stays under the Open
deferral with `specs/004` T019 and `specs/005` T024 open under it, and the
Windows gate stays under the 2.7.0 suspension. This report shifts every
line in the file, so a citation into the constitution, including the
Phase 18 citations in `specs/007-counters-and-timers/tasks.md`, needs
re-anchoring by the next convergence pass.
-->

Prior report (2.9.0, MINOR): Principle IX's per-task verification
clause gains a release-configuration build, once per feature, naming the
`ci-ubuntu` preset. The `dev` preset remains the per-task loop and stays
cheap. The clause binds because an unoptimized build cannot report a
finding an optimizer's analysis produces: a `-Werror=null-dereference` at
`source/counters/plan.cpp` went unreported through three convergence
waves, and the project's own release configuration did not compile for
three cycles. No principle, threshold, identifier, gate, or banned-word
list changes. Amendment history lives in the git log of this file.

Prior report (2.8.0, MINOR): the Open deferrals block gains the
macOS runner entry. Principle VIII's macOS preset-build clause is
enforced developer-local until a runner spec lands; the deferral
records the existing, spec-documented practice and binds nothing new.
No principle, rule, threshold, identifier, gate, or banned-word list
changes. Amendment history lives in the git log of this file.

Prior report (2.7.0, MINOR): VIII's Windows (MSVC) preset
build gate is suspended for the lifetime of the vendored autotools
ingestion of specs/003-vendor-hwloc. The ingestion module aborts
Windows configuration by design, naming the upstream
`contrib/windows-cmake/` on-ramp; platform blocks keep that port
additive, and landing it reinstates the gate. Additional
Constraints carries the pointer. No other principle, rule,
threshold, identifier, gate, or banned-word list changes; amendment
history lives in the git log of this file.

Prior report (2.6.0, MINOR): XI.5 gains a banned-jargon entry,
the token `smoke test`, with the downstream consumer test as its
canonical replacement. Guidance expanded; no principle redefined.
The prose-lint rule data XI5.FILLER gains the token in the same
change per the Editing contract.

Open deferrals, binding until a spec lands them:
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

#### V.1 Identifier Naming

One spelling rule governs every C++ identifier the project owns. It is
enforced by `readability-identifier-naming`, and `.clang-tidy` maps each
key below to the rule it implements. A rule that exists only in
`.clang-tidy` is a defect; this section is the rule and the configuration
is its enforcement.

- **N-1 Types take PascalCase.** `fake_provider` becomes `FakeProvider`.
  One acronym spells as one word, so `pmu_table_entry` becomes
  `PmuTableEntry` and never `PMUTableEntry`. The rule covers classes,
  structs, unions, enums, type aliases, typedefs, concepts, and type
  traits.
- **N-2 Functions take camelBack.** The rule holds for a free function
  and for a member function alike. `register_provider` becomes
  `registerProvider`; `select_directory` becomes `selectDirectory`.
- **N-3 Macros take `UPPER_SNAKE_CASE` under the `SG_` prefix.** A macro
  the project defines spells `SG_` first, as `SG_ENSURE` and
  `SG_NOEXCEPT` do.
- **N-4 A scoped enumerator takes `UPPER_SNAKE_CASE`**, with no prefix.
- **N-5 Variables take camelBack.** A parameter and a local take the same
  spelling as any other variable.
- **N-6 A namespace takes `lower_case`.** The namespace tree is
  `sg`, `sg::counters`, and `sg::counters::detail`.
- **N-7 A template parameter takes PascalCase.** `D` in
  `template<class D>` already complies; `dimension` becomes `Dimension`.
- **N-8 The constant spelling: a named constant takes PascalCase under the
  `k` prefix.** `hard_stop_width` becomes `kHardStopWidth`. The prefix is
  `k` for a namespace-scope, class-scope, or `static` constant alike.
- **N-9 File names stay `lower_case` with underscores.** No identifier
  option governs a file name, and none is added. A header is
  `counters_measurement.hpp`, and renaming one is out of scope.
- **N-10 A private or protected data member keeps the `m_` prefix** and
  takes camelBack for the remainder: `m_column_count` becomes
  `m_columnCount`.
- **N-11 A tag type drops its `_t` suffix and takes PascalCase.** The tag
  object beside the type takes lowerCamelCase. `hard_stop_t` becomes
  `HardStop`, `ring_t` becomes `Ring`, `hard_stop` becomes `hardStop`, and
  `ring` stays `ring`. A tag object is an exception to N-8 and appears
  in `ConstexprVariableIgnoredRegexp`.
- **N-12 A public data member of an aggregate takes camelBack, prefix
  free.** `frequency_hz` becomes `frequencyHz`. The `m_` prefix marks a
  private or protected member under N-10, and it stays off a public
  member. `.clang-tidy` enforces the rule through `PublicMemberCase`.

Every later change obeys these rules. A deviation requires a
constitutional amendment through Governance. A spec, a plan, a local
naming override, or a suppression comment creates no deviation. A
suppression names the V.2 exception entry it applies.

#### V.2 Naming exceptions

A name that the language, the standard library, a vendored library, or
the platform looks up by spelling keeps that spelling. No exception
exists outside this list, and each suppression names the entry it cites.

- Standard container and range protocol: `begin`, `end`, `cbegin`, `cend`,
  `rbegin`, `rend`, `size`, `empty`, `data`, `swap`.
- Standard member types: `value_type`, `size_type`, `difference_type`,
  `reference`, `const_reference`, `pointer`, `iterator`,
  `const_iterator`, `iterator_category`, `iterator_concept`,
  `element_type`.
- Specializations in `std`: `std::hash`, `std::formatter`,
  `std::tuple_size`, `std::tuple_element`, and their members.
- Structured binding protocol: `get`.
- Operator functions, user-defined literal suffixes, and the names of
  standard functions the code calls.
- Names a vendored library or the platform defines: hwloc, simdjson,
  HdrHistogram_c, yaml-cpp, zlib, quill, and Linux and POSIX names. The
  fields of `perf_event_mmap_page` and `perf_event_attr` are kernel
  names, for example `cap_user_rdpmc`, `time_mult`, and `pmc_width`. The
  exemption reaches the member spelling a mirror of such a struct
  copies; a parameter and a local take the project spelling under N-5.
- The macros the build writes from the target name: `SPEEDGUN_NG_EXPORT`
  and the generator siblings `SPEEDGUN_NG_NO_EXPORT`,
  `SPEEDGUN_NG_DEPRECATED`, `SPEEDGUN_NG_DEPRECATED_EXPORT`,
  `SPEEDGUN_NG_DEPRECATED_NO_EXPORT`, `SPEEDGUN_NG_NO_DEPRECATED`,
  `SPEEDGUN_NG_STATIC_DEFINE`, `SPEEDGUN_NG_LIBRARY_DEFINE`, and
  `SPEEDGUN_NG_EXPORT_H`, plus `SPEEDGUN_NG_SUPPRESS_C4251` from
  `cmake/variables.cmake`. Each carries no `SG_` prefix. Include guards
  carry the `SG_` prefix like any other macro.
- A tag object beside its tag type: `hardStop` and `ring`, the exception
  N-11 names, exempt through `ConstexprVariableIgnoredRegexp`.
- `main` in each executable, and every identifier inside `external/`.

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

- Builds succeed for developer and CI presets on Linux (GCC/Clang). Build
  tooling is decoupled from the OS target; CI artifacts are re-creable
  interactively with the same presets. Linux is the supported platform:
  macOS and Windows are unsupported, and a future specification adds a
  platform. An upstream port landing does not reinstate this gate. The
  per-platform blocks that a port consumes are untouched, including the
  ingestion module's Windows abort naming the upstream
  `contrib/windows-cmake/` on-ramp (specs/003-vendor-hwloc R-014,
  cmake/ImportAutotoolsSubmodule.cmake), so landing a port is additive.
- All tests pass (`ctest`).
- Sanitizer-clean: ASan/UBSan (`ci-sanitize`) report no errors.
- Thread-sanitizer-clean: a `ci-tsan` preset and a `tsan` CI job build and
  run the suite under the thread sanitizer and report no race. The preset
  stays apart from `ci-sanitize`, because the compilers reject the pairing.
- Static-analysis-clean: clang-tidy and cppcheck (per `CMakePresets.json`)
  report no new findings, against the pinned Core Guidelines baseline (I)
  from the same configuration. Both launchers reach every compile the
  `ci-ubuntu` preset drives, so the report lands in the `test` job's build
  log and in the build a contributor runs locally. The gate reports, and a
  finding is a defect at lint parity: the author clears it before merge.
- Name-check-clean: `readability-identifier-naming` reports zero
  findings on the project tree, and `WarningsAsErrors` in `.clang-tidy`
  makes a finding fail the build (V.1).
- `format-check` and `spell-check` pass.
- Generated prose satisfies XI: a discourse violation is a defect at lint
  parity. The `prose-lint` job enforces Principle XI and the commit template
  in CI over the pull-request range, and
  `cmake -P cmake/prose-lint.cmake` reproduces the verdict locally
  (specs/002-prose-commit-lint).
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
  `ctest --preset=dev`) before the next. That loop builds unoptimized, so it
  cannot report a finding an optimizer's analysis produces; each feature MUST
  therefore also be built once in its release preset
  (`cmake --preset=ci-ubuntu`, then `cmake --build build`) before its tasks
  are called done. A task
  closed only against the unoptimized build stays open.
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
- Local style wins on indentation, comment conventions, and file
  organization in the edited file, matched even against a different
  preference. Naming follows the naming rules of this constitution, and
  a local spelling yields to those rules. Adopting another convention is
  a dedicated formatting-only change (V).
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
device, and never structure a claim as "does this, not that". State what the <!-- prose-lint: allow reason="XI.2 self-quotation of the banned claim shape" -->
thing IS.
- Wrong: "the runner is a scheduler, not a thread pool" <!-- prose-lint: allow reason="XI.2 self-quotation of the Wrong example" -->
- Right: "the runner dispatches benchmark executions onto a fixed pool of
  worker threads"

Where a distinction is technically load-bearing, each fact gets its own
sentence, both stated on their own terms.

#### XI.3 Never vouch for truthfulness

Banned: "honest", "honestly", "to be honest", "candid", "candidly", "frankly", <!-- prose-lint: allow reason="XI.3 self-quotation of the banned voucher list" -->
"transparent", "transparently", "genuinely", "straight answer", and any <!-- prose-lint: allow reason="XI.3 self-quotation of the banned voucher list" -->
phrasing certifying the truthfulness of a statement. Vouching for one statement
implies the others lack it. Every statement here is grounded in evidence (X.4)
or labeled an estimate with its uncertainty; none needs a marker.

#### XI.4 No meta-editorializing

Do not narrate the authoring process, the reading process, or the framing
inside the artifact. Banned patterns: "in this section we", "this document <!-- prose-lint: allow reason="XI.4 self-quotation of banned meta-editorializing patterns" -->
will cover", "my approach to this file", "what this would take", "how we read <!-- prose-lint: allow reason="XI.4 self-quotation of banned meta-editorializing patterns" -->
your input", "as an AI", "I notice that", "let me walk you through". State the <!-- prose-lint: allow reason="XI.4 self-quotation of banned meta-editorializing patterns" -->
content. The artifact is the content, and it does not describe itself.

#### XI.5 No filler, hedge, or marketing vocabulary

Filler and hedge drops: "it's worth noting", "importantly", "notably", <!-- prose-lint: allow reason="XI.5 self-quotation of the filler and hedge list" -->
"essentially", "basically", "simply", "just", "very", "actually", "in fact", <!-- prose-lint: allow reason="XI.5 self-quotation of the filler and hedge list" -->
"of course", "needless to say", "in order to". <!-- prose-lint: allow reason="XI.5 self-quotation of the filler and hedge list" -->

The wordy connectives XI.7 names join that list, because they add nothing: <!-- prose-lint: allow reason="XI.5 self-quotation of the connective list XI.7 names" -->
"in addition to", "as well as", "and so on", "in the same way", <!-- prose-lint: allow reason="XI.5 self-quotation of the connective list XI.7 names" -->
"due to the fact that", "as a result of", "similarly". <!-- prose-lint: allow reason="XI.5 self-quotation of the connective list XI.7 names" -->

Marketing vocabulary is banned from technical claims: "seamlessly", <!-- prose-lint: allow reason="XI.5 self-quotation of the marketing vocabulary list" -->
"cutting-edge", "leverages", "world-class", "best-in-class", <!-- prose-lint: allow reason="XI.5 self-quotation of the marketing vocabulary list" -->
"industry-leading", "robust", "blazing-fast", "elegant", "powerful". A <!-- prose-lint: allow reason="XI.5 self-quotation of the marketing vocabulary list" -->
performance claim carries a number, a platform, a distribution (VII). An
interface claim carries a contract (II).

Banned jargon: `smoke test`. Name a check by what it does: the
downstream consumer test configures, builds, and runs an installed
package. Owner directive 2026-09-20; the token joins the prose-lint
filler vocabulary.

Weak requirement language stays banned in EARS statements per III: "should",
"may", "might", "approximately".

#### XI.6 Enforcement

- A violation in generated text is a defect at lint parity: reviewers reject
  it, the author fixes it before merge, as with a clang-tidy finding.
- Machine checks cover the grep-checkable subset: the em-dash code point, `--`
  or `---` in prose, `, not `, ` rather than `, ` instead of `, the XI.3
  voucher list, the XI.5 filler list, the XI.5 marketing list. The delivered
  check is `prose-lint` (specs/002-prose-commit-lint): one gate script, one
  rule-data file, a CI job, and fixtures asserting every family in both
  directions over added and modified lines. Rule data mirrors this
  principle, and renaming a rule identifier is a breaking change. The
  exemption constructs, precedence, and marker grammar are specified in
  specs/002-prose-commit-lint/contracts/rule-data.md.
- Exemptions: a banned token inside a verbatim quotation, code span, command,
  file name, or a literal that is itself the subject under discussion. Mark
  the quotation as quoted.
- XI binds generated output. It leaves a contributor's personal voice in prose
  they author by hand untouched, and never weakens IV or V.

#### XI.7 Simplified Technical English

Generated prose must reach about 80 percent compliance with ASD-STE100,
Simplified Technical English. That standard is the reference. This
subsection names the rules that carry the most weight here, and it
settles how the two rule sets meet.

- Keep a sentence short: 20 words for a step in a procedure, 25 words for
  a description.
- Put one topic in a sentence. Split a sentence that carries two actions.
- Use the active voice. Name the actor that acts.
- Use the present tense.
- Use a verb in place of a noun form: `compile`, not `compilation`.
- Order a sentence as subject, verb, object.
- Use one term for one concept in every document. Do not rotate synonyms.
- Use `shall` for an obligation. Use `should` for advice and `may` for
  permission where the surrounding text allows those words; XI.5 bans them
  inside a requirement statement.
- Avoid the wordy connectives in XI.5's filler list. `prose-lint` reports
  each one on an added or modified line.
- Avoid `etc.`, `so that`, `otherwise`, and `and/or`. A reviewer reports
  these four. The gate cannot match them, for two measured reasons. A
  word boundary does not follow the period in `etc.`. And `and/or` has the
  shape of a file path, so the path exemption in XI.6 removes the line
  before any rule runs.
- Avoid `if` at the start of a sentence. Put the condition first, then the
  result.
- Avoid `to be`, and avoid `be` used as a noun.
- Use `if` for a condition. Keep `when` for a time.
- Prefer one word where one word says it: `more than`, not `greater than`.
- Use `usually`, `often`, `seldom`, and `never` in place of a vague
  quantity.
- Give a number its unit: `20 ns`, not `20`.
- Define a term before the text uses it.
- Keep every code span on one line, so a wrapped command cannot read as
  prose.

Precedence: XI.1 through XI.6 govern. XI.7 applies where they are
silent. Where the standard prefers a construction that XI.2 or XI.5
bans, the XI rule wins. Full compliance is not required, and the
standard lets a writer depart from it on purpose for a stated audience.
Record such a departure in the artifact when a reader would expect the
rule to hold.

Enforcement: a violation is a defect at lint parity and a reviewer
rejects it before merge, on the terms XI.6 sets. The gate covers the
connectives in XI.5's filler list and nothing else here, because most of
these rules are a judgment about a sentence.

Scope and grandfathering: the rule binds prose written after this
amendment. Earlier text keeps its violations. A change brings the lines
it touches into compliance, the same way XI.1 does.

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

- **Language:** C++23 only (`CMAKE_CXX_EXTENSIONS=OFF`). Linux on GCC and Clang
  is the supported platform, and new code must not break it. macOS and Windows
  are unsupported; a future specification adds one.
- **Warnings and hardening:** the strict warning set in `CMakePresets.json`
  (`-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wold-style-cast`) is
  preserved, as are the security-hardening
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
| 2.17.0 | 2026-10-08 | V.2 gains the tag-object entry `hardStop` and `ring` that `.clang-tidy` already exempted, names the export-header macro family and `SPEEDGUN_NG_SUPPRESS_C4251` in place of the single export macro, and states that the kernel-field entry reaches the member spelling a mirror copies; specs/014 post-merge repair |
| 2.16.0 | 2026-10-08 | V.1 gains the FR-012 obligation: a later spec, plan, local naming override, or suppression creates no deviation, a deviation needs an amendment, and a suppression names the V.2 entry it applies; N-1 adds concepts and type traits to the enumeration; T048 and T050 of specs/014 |
| 2.15.0 | 2026-10-07 | V.1 gains N-12: a public data member of an aggregate takes camelBack, prefix free, the rule `.clang-tidy` `PublicMemberCase` enforces; T037 of specs/014 closes the gap where the rule lived only in the key |
| 2.14.0 | 2026-10-07 | V gains V.1 Identifier Naming and V.2 Naming exceptions: the one spelling rule every owned C++ identifier follows, and the closed list of names that keep a spelling the language, the standard library, a vendor, or the platform requires. FR-001 to FR-021 of specs/014. |
| 2.13.0 | 2026-10-04 | VIII hard gate list gains a thread-sanitizer item: a `ci-tsan` preset and a `tsan` job report no race, FR-012 and SC-003 of specs/012 require them, no gate removed or weakened |
| 2.12.0 | 2026-10-03 | XI.7 Simplified Technical English: generated prose reaches about 80 percent ASD-STE100 compliance, reviewer-enforced, XI.1 to XI.6 govern on conflict, no rule identifier added |
| 2.11.0 | 2026-10-02 | supported platform narrowed to Linux; gate list, platform definition, and release-build clause name it alone; 2.7.0 Windows suspension and 2.8.0 macOS deferral superseded, both reports retained; macOS Open deferral closed, retiring specs/009 T038; five merged vendor specs superseded by name and left unedited |
| 2.10.0 | 2026-09-28 | VIII static-analysis clause states the gate reports and names the step a reader collects the report from |
| 2.9.1 | 2026-09-27 | IX per-feature release build names the preset per platform |
| 2.9.0 | 2026-09-27 | IX per-task verification adds a release-configuration build once per feature; an unoptimized build cannot report an optimizer-backed finding |
| 2.8.0 | 2026-09-25 | macOS gate enforcement recorded as a deferral: developer-local until a runner spec lands |
| 2.7.0 | 2026-09-21 | Windows MSVC preset-build gate suspended for the vendored-autotools lifetime; reinstated when the port lands |
| 2.6.0 | 2026-09-20 | XI.5 banned jargon `smoke test`; downstream consumer test canonical |
| 2.5.0 | 2026-09-18 | prose-lint gate: XI.6 machine check, commit-template lint, deferrals closed |
| 2.4.1 | 2026-09-10 | token compression, no rule changed |
| 2.4.0 | 2026-09-10 | principles X (anti-slop code) and XI (discourse) |
| 2.3.0 | 2026-09-09 | language pinned to C++23, CMake >= 3.20 |
| 2.2.1 | 2026-09-06 | docs consolidated into README |
| 2.2.0 | 2026-09-06 | commit template and linear-history standard |
| 2.1.0 | 2026-09-06 | SDD to R-DCUT mapping, UML mandate, TDD mode |
| 2.0.0 | 2026-09-06 | redefinition on DBC, R-DCUT, coverage, CI gates |
| 1.0.0 | 2026-09-06 | initial ratification from repository conventions |

**Version**: 2.17.0 | **Ratified**: 2026-09-06 | **Last Amended**: 2026-10-08
