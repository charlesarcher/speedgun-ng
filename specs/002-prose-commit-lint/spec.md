# Feature Specification: Prose and Commit-Message Lint Gate

**Feature Branch**: `002-prose-commit-lint`

**Created**: 2026-09-10

**Status**: Draft

**Input**: User description: "Automated prose and commit-message lint gate. Principle XI.6 of the constitution promises a machine check for discourse and prose rules (em-dash ban, contrastive framing, truth vouchers, meta-editorializing, filler and marketing vocabulary, with a quotation and code-span exemption), and the Sync Impact Report also defers the commit-template lint (title format `<Section>: <Imperative>` at 50 characters or fewer, why-body, `Approved-by:` footer, no vague messages). Deliver one CI lint target covering both, so the two recorded deferrals close together and Principle VIII gains the gate. The check must run on Linux CI and be runnable interactively with the same presets, must not require new hard runtime dependencies (the project keeps zero external runtime dependencies), and must exempt code spans, verbatim quotations, and literals under discussion."

## Clarifications

### Session 2026-09-10

- **Q: One gate or two?** → **A:** One CI job named `prose-lint` (the existing `lint` job belongs to clang-tidy and cppcheck) running two checks, prose and commit, over one shared machine-readable rule vocabulary. Both deferrals close in the same landing change.
- **Q: Does the grandfathering clause in Principle XI.1 change what the gate reports?** → **A:** Yes. In pull-request mode the prose check reports violations on lines the change adds or modifies. Whole-file mode exists for local use and for the planned tree-wide sweep. Grandfathered text in the tree stays silent per pull request.
- **Q: What surfaces does the prose check read?** → **A:** Tracked Markdown (the README, `AGENTS.md`, `.specify/memory/`, `specs/`, `docs/`) plus comments inside tracked source files: `//` and `/* */` in C and C++, `#` in CMake and shell. Principle XI covers code comments and carries no exemption for them, so a documents-only gate would leave half the surface unchecked. Generated and vendored paths are excluded by an explicit list.
- **Q: How is a deliberate counter-example expressed, given that Principle XI's own section quotes banned tokens?** → **A:** Auto-exemption for fenced code blocks, inline code spans, URLs, file paths, and shell command lines, plus a per-line exemption marker carrying a mandatory reason. The marker is the P2 exception site required by Principles I and XI.6.
- **Q: Who checks commit messages, and over what range?** → **A:** Every commit in the pull-request head range, and every commit newly pushed to the default branch, which covers a squash result separately from the commits it collapses.
- **Q: The template makes the why-body optional for `genuinely trivial` changes. How does a machine judge triviality?** → **A:** Mechanically and conservatively: a body may be absent only when the commit changes at most a configured number of lines (default 5). The alternative, an explicit skip marker, would add a footer to the template and needs a constitution amendment; it is left out. Recorded in Assumptions as an accepted approximation.
- **Q: Imperative mood is a grammatical property. How is it checked?** → **A:** A narrow rejection list of the observed non-imperative shapes (`Added`, `Adds`, `Fixing`, `Fixes`, `Update of`, `was added`, and similar) plus a capitalization rule. Full grammatical detection is out of scope. Recorded in Assumptions.
- **Q: What happens when the rule data and the constitution disagree?** → **A:** The constitution is normative. The disagreement is a defect, and a fix updates the constitution and the data file in one change (constitution §Governance).

## User Scenarios & Testing *(mandatory)*

### User Story 1 - A violating pull request is refused before merge (Priority: P1)

As a maintainer, when a pull request adds generated prose that breaks a discourse rule, I want the change refused automatically at the same standing as a static-analysis finding, so that the rules in Principle XI hold without depending on a reviewer noticing.

**Why this priority**: This is the promise Principle XI.6 records and the constitution currently carries as a deferred item. Until the gate runs, the rules bind reviewer discipline alone, the weakest enforcement available and the first thing an agent-generated pull request slips past.

**Independent Test**: Can be fully tested by staging a pull request that adds one line carrying a banned pattern from each rule family alongside a fixture line that must stay silent, running the gate, and observing a non-zero verdict naming the file, the line, and the rule identifier for each violation and nothing else. Delivers the enforcement value on its own.

**Acceptance Scenarios**:

1. **Given** a pull request that adds a Markdown line containing an em-dash (U+2014) in prose, **when** the gate runs, **then** it fails and reports the file, line, rule identifier, and the offending token.
2. **Given** a pull request that adds a line containing an en-dash inside a numeric range such as `P0–P3`, **when** the gate runs, **then** it reports nothing for that line.
3. **Given** a pull request that adds a banned vocabulary word inside an inline code span or inside a fenced block, **when** the gate runs, **then** it reports nothing for that line.
4. **Given** a pull request that adds a line where a banned word is a substring of a longer identifier such as `candidate`, **when** the gate runs, **then** it reports nothing for that line.
5. **Given** a pull request that adds a verbatim quotation carrying a banned token, marked with the exemption marker and a reason, **when** the gate runs, **then** it reports nothing for that line.
6. **Given** the same quotation marked with the exemption marker and an empty reason, **when** the gate runs, **then** it fails and reports the marker as the problem.
7. **Given** a pull request whose added lines violate no rule, **when** the gate runs, **then** it exits zero and reports the number of files examined.
8. **Given** a violation sitting on a line the pull request did not add or modify, **when** the gate runs in pull-request mode, **then** it reports nothing for that line.

---

### User Story 2 - A malformed commit message is refused (Priority: P2)

As a maintainer, when a commit in a pull request breaks the message template in the constitution (Pull Request Quality), I want the gate to refuse it, so that history stays uniform, bisectable, and readable in `git log` without an editor pass after the fact.

**Why this priority**: The commit-template deferral is the second half of the same missing gate. It ranks below the prose check because a prose violation lands inside the shipped artifact while a malformed message costs review time, and because the commit check reads a bounded, well-structured object with a smaller blast radius.

**Independent Test**: Can be fully tested against a fixture range of commits covering each template rule, one conforming and at least one non-conforming per rule, and observing a verdict that names the commit, the rule, and the offending text. Delivers commit-message conformance independently of the prose check.

**Acceptance Scenarios**:

1. **Given** a commit titled `Docs: Clarify runner shutdown` with a why-body, a blank line between title and body, and an `Approved-by:` footer, **when** the commit check runs, **then** it passes.
2. **Given** a commit whose title is 51 characters long, **when** the commit check runs, **then** it fails and names the title-length rule.
3. **Given** a commit titled `runner: fix crash` where the section token is absent from the configured list, **when** the commit check runs, **then** it fails and names the section rule.
4. **Given** a commit titled `Docs: Added the runner section.`, **when** the commit check runs, **then** it fails and names the non-imperative-shape rule and the trailing-period rule.
5. **Given** a commit with a title and no body and 40 changed lines, **when** the commit check runs, **then** it fails and names the missing-body rule.
6. **Given** a commit with a title and no body and 3 changed lines, **when** the commit check runs, **then** it passes the body rule.
7. **Given** a commit with no `Approved-by:` footer, **when** the commit check runs, **then** it fails and names the footer rule.
8. **Given** a commit titled `wip`, **when** the commit check runs, **then** it fails and names the vague-title rule.
9. **Given** a commit whose body contains an em-dash in prose, **when** the commit check runs, **then** it fails and names the prose rule that covers it.
10. **Given** a range of commits where two of five are non-conforming, **when** the commit check runs, **then** it fails, reports both offending commits by short hash, and reports the three conforming commits as passing.

---

### User Story 3 - The same verdict locally and in CI (Priority: P3)

As a contributor, when I run one command in my own checkout, I want the identical verdict CI will produce, so that I never spend a review cycle on what my own machine could have told me (constitution, Pull Request Quality: it is not the reviewer's job to find what the tools find).

**Why this priority**: Local parity turns the gate into a tool the contributor uses alongside the gate the repository enforces. It ranks third because it multiplies the value the two checks already provide on top of enforcement they already deliver.

**Independent Test**: Can be fully tested by running the single command against a violating checkout and a clean checkout and comparing the two verdicts with the CI artifacts from the same inputs. Delivers contributor-facing parity on its own.

**Acceptance Scenarios**:

1. **Given** a violating working tree, **when** I run the single local command, **then** it exits non-zero and prints file, line, rule identifier, and the offending token for every violation.
2. **Given** a clean working tree, **when** I run the single local command, **then** it exits zero.
3. **Given** the same input, **when** I compare the local verdict with the CI job artifact, **then** the two verdicts list the identical set of violations.
4. **Given** the whole repository, **when** I run the command in whole-file mode, **then** it completes within the bound stated in SC-003.

---

### Edge Cases

- A banned token that is part of a longer word (`candid` inside `candidate`, `just` inside `adjust`) must stay unreported: word-boundary matching is required.
- The em-dash ban and the en-dash allowance meet in one line, for example `Ports 0–3 carry — in the example — traffic`: the range is legal and the two connectors are violations.
- A URL or file path containing a hyphen sequence that looks like `--` must stay unreported.
- A shell command line such as `cmake --build --preset=dev` inside prose is exempt: it is a command, and commands are out of scope for the prose rules.
- The rule text of Principle XI quotes its own banned vocabulary; that quotation must carry the exemption marker, else the constitution fails its own gate.
- A comment in a source file that is a URL, a license header, or a generated block must stay unreported.
- A commit message that quotes a violating line from a document, for example a revert body citing the original text, needs the same exemption mechanism available to it. Otherwise the marker rule and the commit rule contradict each other.
- An empty commit range, from a pull request with no commits or a push that moves no refs, reports success.
- A file that is invalid UTF-8, or that carries a byte-order mark, is handled without a crash and reported as skipped with a reason.
- A merge commit, which the constitution forbids on the base branch, can still appear inside a contributor branch range: it is checked as any other commit except for the body rule.
- A line longer than the report column budget is still reported with enough context to locate it.
- A banned token on the deleted side of a diff stays unreported: pull-request mode looks at added and modified lines.
- Unicode look-alikes of the em-dash (the double hyphen U+2010, the minus sign U+2212, and similar) need an explicit decision per token. The ban names U+2014 plus ASCII `--` and `---`; every other character is out of scope until the rule data adds it.

## Requirements *(mandatory)*

### Functional Requirements

**Rule vocabulary and scope**

- **FR-001** (ubiquitous): The gate shall enforce the six rule families of Principle XI: the em-dash ban (XI.1), the contrastive-framing ban (XI.2), the truth-voucher ban (XI.3), the meta-editorializing ban (XI.4), the filler ban (XI.5), and the marketing-vocabulary ban (XI.5), with the exact token and pattern lists drawn from machine-readable rule data.
- **FR-002** (ubiquitous): The gate shall treat the en-dash (U+2013) as legal wherever it separates numeric or enumerated values, and shall treat it as out of scope otherwise.
- **FR-003** (ubiquitous): Vocabulary rules shall match whole words only, so a banned token inside a longer word stays unreported.
- **FR-004** (ubiquitous): The gate shall exempt fenced code blocks, inline code spans, indented code, URLs, file paths, and shell command lines from every prose rule.
- **FR-005** (event-driven): When a line carries the documented exemption marker, the gate shall suppress prose-rule reports for that line and shall require a non-empty reason within the marker, reporting the marker itself when the reason is absent.
- **FR-006** (ubiquitous): The prose check shall read tracked Markdown in the repository root, in `.specify/memory/`, in `specs/`, and in `docs/`, together with comments in tracked C, C++, CMake, and shell sources, and it shall exclude generated and vendored paths named in an explicit exclusions list.
- **FR-007** (event-driven): When the gate runs against a pull request, the prose check shall report violations only on lines that pull request adds or modifies, which implements the scope clause of Principle XI.1.
- **FR-008** (optional): Where a caller selects whole-file mode, the prose check shall report every violation in the named files, which serves local use and the scheduled tree-wide sweep.

**Commit-message template**

- **FR-009** (ubiquitous): For every commit in the checked range, the title shall have the form `<Section>: <Imperative description>`, with a single space after the colon, no trailing period, and a total length of 50 characters or fewer.
- **FR-010** (ubiquitous): The section token shall be present in the configured section list, and that list shall be editable as data so a new section token is a one-line change carried in the pull request that first uses it.
- **FR-011** (unwanted-behavior): If a title begins with a non-imperative shape named in the rule data, or contains a passive shape named in the rule data, the commit shall be rejected.
- **FR-012** (unwanted-behavior): If a title matches a member of the vague-message list in the rule data, the commit shall be rejected.
- **FR-013** (ubiquitous): A body shall follow the title after one blank line, wrapped at 72 columns or fewer, except where the commit changes no more than the configured number of lines (default 5), the mechanical stand-in for the template's `genuinely trivial` exemption.
- **FR-014** (ubiquitous): Every commit shall carry an `Approved-by:` footer.
- **FR-015** (ubiquitous): The prose rules shall apply to commit titles and bodies with the exemptions of FR-004 and FR-005 available to a quoted line.
- **FR-016** (ubiquitous): The commit check shall cover every commit in the head range of a pull request and every commit newly pushed to the default branch, so a squash-merged landing commit is checked separately from the commits it collapses.

**Reporting and integration**

- **FR-017** (ubiquitous): Every report shall name the source (a file with a line number, or a commit with a short hash), the stable rule identifier, the offending token, and the rule family, in a form a reviewer can act on and a machine can parse.
- **FR-018** (ubiquitous): The gate shall exit zero when it finds nothing and non-zero otherwise, so any caller can use it as a binary check (constitution Principle X.4).
- **FR-019** (ubiquitous): One command shall run both checks locally from a clean checkout, using tooling already present in the developer environment, and that command shall be documented in the README alongside the existing quality gates.
- **FR-020** (ubiquitous): The gate shall run as a Linux CI job on every pull request and on every push to the default branch, following the existing job conventions so the CI artifact is re-creable interactively from the same commands (constitution Principle VIII).
- **FR-021** (ubiquitous): The gate shall add no external runtime dependency to the library and no dependency to the project beyond tooling the CI runners already install for the existing checks (constitution, Additional Constraints: Dependencies).
- **FR-022** (ubiquitous): Landing this feature shall amend the constitution: the two deferred items in the Sync Impact Report resolve, Principle XI.6 names the delivered check in place of the deferral, and Principle VIII's gate bullet names it, so the tree carries no stale deferral after the merge.
- **FR-023** (ubiquitous): The checker's own fixtures shall cover every rule family in both directions, a case that must be reported and a case that must stay silent, and those fixtures shall run inside the gate itself.

### Key Entities

- **Rule**: One banned pattern or one exemption, identified by a stable identifier, carrying its family (the Principle XI subsection it implements), its match expression, its scope, and the constitution reference a reviewer reads to understand it.
- **Rule Data File**: The machine-readable set of rules plus the section list, the vague-title list, the exclusions list, and the numeric thresholds (title length, body-wrap width, trivial-change size). The single editable source for all four.
- **Exemption Marker**: A per-line annotation carrying a mandatory reason, serving as the P2 exception site for a quotation or a literal under discussion.
- **Checked Unit**: A line of prose in scope, or a commit message in range, together with its authorship status (new, modified, or grandfathered).
- **Verdict**: The ordered set of findings plus the counts of units examined, reported per source, with the exit status FR-018 derives from.

## Success Criteria

### Measurable Outcomes

- **SC-001**: Every pull request that adds a prose violation on an added or modified line is refused by the gate before merge, demonstrated by a fixture set covering all six rule families with a reported case and a silent case each.
- **SC-002**: The gate reports zero findings on grandfathered text and zero false positives across a fixture corpus of legal constructs covering numeric ranges, inline code spans, fenced blocks, URLs, file paths, shell commands, longer-word substrings, and marked quotations.
- **SC-003**: One local command produces the CI verdict on the whole repository in under 10 seconds on a developer machine, and the local and CI verdicts on identical input list the identical findings.
- **SC-004**: Every commit in a checked range conforms to the message template, measured over a fixture range carrying deliberate violations in at least five distinct template rules, each caught and named.
- **SC-005**: After the landing merge, the constitution states zero open deferrals for prose checking or commit-message checking, and the gate appears in the Principle VIII gate list.
- **SC-006**: The library still links no external runtime dependency and the CI runners install nothing beyond what the existing jobs already install.

## Assumptions

- The CI runners already provide the interpreter and libraries the existing `tools/dbc` check scripts use, so the gate needs no new dependency. Which interpreter and which libraries are a plan decision.
- Diff-scoped checking in pull-request mode is the correct first deployment, matching Principle XI.1. The tree-wide em-dash sweep stays a separate, formatting-only change under Principle V, after which whole-file mode can become the CI default. That sweep is out of scope here.
- Triviality of a body-less commit is approximated by a changed-line threshold (default 5). An explicit skip footer would extend the commit template and require a constitution amendment, so this feature rejects it.
- Imperative mood is checked by rejecting the observed non-imperative shapes. Full grammatical detection is out of scope, and the rule data is where a further shape is added when one appears.
- Interactive chat replies, which Principle XI also governs, produce no artifact the gate can read. Enforcement there stays with reviewer discipline, and is out of scope.
- Editor and pre-commit integrations are out of scope. FR-019 covers the documented single command, and any hook wrapping it is a later convenience.
- Historical commit messages are immutable (constitution: history after merge is immutable), so the commit check applies to ranges moving forward.
- The constitution stays the normative text and the rule data stays its mechanical projection. On disagreement the constitution wins and the fix lands in both in one change (constitution §Governance).
- Spell-checking already exists as `spell-check` and stays separate. This gate covers the discourse rules and the commit template.
