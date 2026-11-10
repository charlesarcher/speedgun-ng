# Feature Specification: Names and User Paths Out of Old Records (T-NAMES)

**Feature Branch**: `049-names-old-records`

**Created**: 2026-11-10

**Status**: Draft

**Input**: User description: "Remove person names and user paths from old
records per G-17 / G-27 history rules. Short name: names-old-records."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - A maintainer searches for names and finds none (Priority: P1)

A maintainer runs the agreed name grep over `specs/` and `docs/` at the
branch head. The grep prints only kept lines. The 193 matching lines that
stand at base `1f1f9b20` carry a user home path or a repository login in
record text from eighteen older feature lanes, tallied in the
Assumptions section. The lane rewrites each such line in place so the
search comes back clean.

**Why this priority**: the owner ruling G-17 and the specs/045 close-out
made the clean grep the standing record rule (F1-006, F1-007 of
`specs/045-merge-fixes`). Ticket T-NAMES (owner ruling, 2026-11-04) is
this feature, and the grep result is its definition of done.

**Independent Test**: run
`git grep -nE 'arche[r]c|char[l]es|@gmai[l]' specs docs`
at the branch head. The output lists no line outside the kept list. The
kept list is empty, so the grep prints nothing.

**Acceptance Scenarios**:

1. **Given** the squashed branch head, **When** the maintainer runs the
   agreed grep over `specs/` and `docs/`, **Then** the grep exits and
   prints no line.
2. **Given** the same head, **When** the maintainer runs the same grep
   over the whole tree, **Then** the only printed lines sit in the kept
   out-of-scope files named in FR-005.

---

### User Story 2 - A reader follows an old record without a personal path (Priority: P2)

A reader opens an old tasks or citations record. A recorded command that
once spelled a home path now spells `$HOME/` or a path relative to the
build tree. The path no longer names one user. Commands that run from
the checkout read the same on any machine. A recorded path into a scratch
tree on the old lane host keeps its shape under the same spelling; the
cleanup takes the personal name out of it.

**Why this priority**: the records are the evidence trail for every
earlier lane. A personal path makes a record unrunnable on the next
machine and puts a user name back into generated text.

**Independent Test**: pick any rewritten line, for example a
clang-tidy header-filter record under
`specs/007-counters-and-timers/`. The line names `$HOME/` or a relative
path, keeps its surrounding evidence (file, line number, figure), and
spells the same command shape.

**Acceptance Scenarios**:

1. **Given** a record line that named `/home/<user>/code/speedgun-ng/`,
   **When** the lane rewrites it, **Then** the line names
   `$HOME/code/speedgun-ng/` or a path relative to the repository root
   or build tree, with the rest of the line unchanged.
2. **Given** a record of a cached CMake entry that named a home path
   inside a filter pattern, **When** the lane rewrites it, **Then** the
   pattern keeps its shape with the `$HOME` spelling.

---

### User Story 3 - Evidence identifiers survive every rewrite (Priority: P2)

A maintainer re-derives a figure from an old record: a run id, a pull
request number, a commit sha, a job name. Every rewrite preserves those
identifiers. A GitHub Actions run that once carried a hyperlink with the
repository login becomes the bare form this lane already uses:
`run <id>`. A `gh` command keeps its endpoint with
the login replaced by the `<owner>` placeholder.

**Why this priority**: the rewrites must not damage the trail they
clean. A lost run id makes a record unverifiable, which is the same
defect class as a planted figure.

**Independent Test**: diff the branch against base. Every changed line
keeps its run ids, PR numbers, and shas. The diff renames no identifier.

**Acceptance Scenarios**:

1. **Given** a line with a run link whose URL names the repository
   owner, **When** the lane rewrites it, **Then** the line states the
   run id as plain text and keeps the PR number when the record names
   one.
2. **Given** a `gh api` record of the form
   `gh api repos/<owner>/speedgun-ng/...`, **When** the lane rewrites
   it, **Then** the endpoint stays intact under an `<owner>`
   placeholder.

---

### Edge Cases

- A line contains the login inside a URL path that the reader cannot
  retype: the bare run id keeps it findable through the Actions list.
- A rewritten line would become a self-matching grep line: the bracketed
  classes of the agreed grep keep such a line clean, the shape specs/045
  F1-006 uses.
- A home path sits inside a quoted cache value or a filter regular
  expression: the replacement keeps the quoting and the pattern anchors.
- A kept file under FR-005 changes on master between the lane start and
  the final head: the lane still edits nothing outside `specs/` and
  `docs/`.
- Older text carries XI violations on its own lines: the lane rewrites
  only the lines the cleanup needs, per XI.1 grandfathering.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: When the agreed name grep
  `git grep -nE 'arche[r]c|char[l]es|@gmai[l]' specs docs` runs at the
  branch head, the tool shall print only kept lines, and the kept list
  shall be the empty list named in D-6.
- **FR-002**: The lane shall replace every home-path occurrence of the
  form `/home/<user>/` in in-scope text with a `$HOME/` path or a path
  relative to the repository root or build tree, preserving the
  remainder of the path and the evidence on the line.
- **FR-003**: The lane shall replace every person-identity token (a
  repository login, a system user name, or an email address that names
  a person) in in-scope text with a role word or a placeholder, keeping
  each sentence grammatical and its facts intact.
- **FR-004**: Every rewrite shall preserve run ids, pull request
  numbers, commit shas, file paths that name no user, job names, and
  measured figures.
- **FR-005**: The lane shall leave unchanged the license copyright line
  of `LICENSE`, the prose-gate fixture data under `test/`, the CI
  workflow files, `CMakeLists.txt`, and `CODEOWNERS`. Those carry the
  repository identity tokens the T-NAMES ruling keeps, and they sit
  outside this lane.
- **FR-006**: The lane shall change no file outside `specs/` and
  `docs/`, with one exception: the tracked tooling pointer
  `.specify/feature.json`, which every lane updates when it allocates a
  feature directory and which carries no record text. No source,
  header, test, example, CMake, CI, or gate file is touched.
- **FR-007**: The marker count `git grep -c LCOV_EXCL -- source include`
  shall read the same at the squashed head as at base, and that base
  figure stands at 84 at `1f1f9b20`.
- **FR-008**: The lane shall not rewrite git history. Master history
  stays as it stands (G-27).
- **FR-009**: When the lane's checks run at the tree built at the
  pre-squash head, the squashed commit differing from that tree only
  by lane-record files under the feature directory, the spell
  read `codespell` from the repository root, the drift read
  `python3 tools/docs/example_drift.py .`, and the prose read
  `python3 tools/prose/prose_gate.py --base origin/master` shall each
  exit 0, and one `ci-rocky` release build with the full `ctest` suite
  shall pass, so the records-only lane still carries the Principle VIII
  build and suite reading.
- **FR-010**: Every commit on the lane shall carry author and committer
  `OmO <omo@speedgun-ng.local>`, a title of the form
  `<Section>: <one-line imperative>` at 50 characters or fewer, a body
  wrapped at 72 columns, and a footer paragraph `Refs:
  specs/049-names-old-records` then `Approved-by: OmO`.
- **FR-011**: The lane shall carry its work on the branch
  `049-names-old-records`, push it, rebase it onto the current
  `origin/master` when master moved before the final push, squash it to
  one commit, push the squashed head, and open one pull request to
  `master`; it shall never merge that pull request and shall close no
  issue. When the merge base moved, it shall re-derive every base
  count named in this spec at the new merge base and record the result
  in the lane records.
- **FR-012**: Every line this lane writes, including the artifacts of
  `specs/049-names-old-records/`, shall satisfy FR-001 from the moment
  it is committed.

### Key Entities

- **Record line**: one line of text in a file under `specs/` or
  `docs/`. It carries prose, a recorded command, or a recorded figure.
- **Person-identity token**: a string that names a person: a repository
  login, a system user name, or a personal email address.
- **Kept line**: an in-scope line the ruling exempts, each with its
  reason recorded in this spec. The list is empty at drafting.
- **Out-of-scope match**: a match in an FR-005 kept file, reported by
  the whole-tree read and never edited.
- **Evidence identifier**: a run id, pull request number, commit sha,
  job name, or measured figure that a record needs to stay verifiable.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: The agreed grep over `specs` and `docs` at the squashed
  head prints 0 lines.
- **SC-002**: The agreed grep over the whole tree at the squashed head
  prints lines only in the FR-005 kept files. The base reading is 30
  such lines: 24 under `test/prose-gate-fixture`, 4 in
  `.github/workflows/ci.yml`, 1 in `CMakeLists.txt`, 1 in `CODEOWNERS`,
  and 0 in `LICENSE`.
- **SC-003**: The base count agrees: the agreed grep over `specs` and
  `docs` at `1f1f9b20` counts 193 lines, and every one of those lines is
  rewritten or named on the kept list with a reason.
- **SC-004**: Every FR-009 reading passes at the lane's final tree,
  the release build and full suite reading included, and the marker
  count reads 84.
- **SC-005**: `git diff --name-only origin/master...HEAD` at the
  squashed head lists files under `specs/` and `docs/` only, with the
  one `.specify/feature.json` exception of FR-006, and the pull request
  body carries the head sha, the host kernel, the compiler version, the
  CPU family and model as this host reports them, every FR-009 reading,
  and the FR-007 marker figure.

## Assumptions

- The agreed grep is the bracketed pattern that `specs/045-merge-fixes`
  F1-006 and F1-007 use, widened from one feature directory to `specs`
  and `docs` for this lane.
- A survey at `1f1f9b20` found the in-scope matches in two token families
  only: a user home path (160 lines; 159 carry a `/home/<user>/` prefix,
  and one, in `specs/048-hwlat-probe/plan.md`, carries a bare recorded
  search string) and a repository login inside GitHub Actions URLs,
  `gh` commands, and recorded owner/repo slugs (33 lines). `docs/`
  carries none. Of the home-path lines, 106 name the repository
  checkout, 53 name other home paths on the old lane host (scratch
  trees, caches, sibling trees, and host tools), and one carries the
  bare recorded string.
- The per-lane tally at `1f1f9b20` is the work list: 030 with 55 lines,
  039 with 32, 029 with 26, 043 with 16, 050 with 11, 024 with 9, 007,
  014, and 037 with 8 each, 048 with 6, 041 with 5, 031 and 040 with 2
  each, and 032, 034, 035, 036, and 045 with 1 each. Eighteen lanes,
  193 lines.
- Out-of-scope kept files (`LICENSE`, `test/` fixture data, `.github/`,
  `CMakeLists.txt`, `CODEOWNERS`) may carry matches; this lane reports
  them and edits none of them.
- The lane runs on one Linux host; the acceptance facts are local
  readings on that host (G-20), as recorded for every lane so far.
- The squashed head reuses no version, schema, or SOVERSION number: the
  lane changes no code, report layout, or public interface.

## Decisions

The feature description recommends one answer per open question, and
this spec takes each one:

- **D-1 (home paths)**: rewrite each `/home/<user>/` prefix to `$HOME/`.
  Keep the remainder of the path. One uniform spelling applies to every
  line, so no per-line judgement enters the rewrite. Cached CMake
  values and filter patterns keep their quoting and anchors with the
  `$HOME` spelling. The rule is uniform: the 53 base
  lines that name home paths outside the checkout, and the one line
  that carries a bare recorded search string with no trailing path,
  take the same shape; the bare string becomes `$HOME` with no trailing
  slash.
- **D-2 (run links)**: a Markdown link whose URL names an Actions run
  becomes a plain-text `run <id>`, the link text dropped uniformly. A
  Markdown link whose URL names a pull request keeps its link text as
  plain text, and a bare pull-request URL becomes `PR #<n>`. This is
  the spelling the specs/045 records already use.
- **D-3 (`gh` commands and recorded slugs)**: the owner segment of a
  recorded `gh api` or `gh run` endpoint becomes the `<owner>`
  placeholder, and the endpoint keeps every other segment. The same
  placeholder covers a bare recorded `owner/repo` slug, an endpoint
  path quoted without a verb, and a quoted workflow expression that
  names the owner; the kept source file those records quote stands
  unchanged.
- **D-4 (ownership prose)**: a phrase that names the login as repository
  identity becomes "this repository".
- **D-5 (emails)**: a personal email address in scope becomes a role
  word. The survey found none in `specs/` or `docs/`.
- **D-6 (kept list)**: the kept list for FR-001 is empty. Any line the
  lane keeps needs a reason recorded in this spec before the final head,
  and no default exemption exists.
- **D-7 (history)**: no rebase of master, no filter-branch, no amend of
  pushed master commits (G-27). Only branch-local commits are created.
- **D-8 (acceptance)**: local acceptance only (G-20). CI on a GitHub
  runner is information. The PR body carries the local readings and the
  final head sha.
- **D-9 (one PR)**: one branch, one squashed commit, one pull request;
  never merge, and no issue closes.
- **D-10 (matches arriving with a rebase)**: master has gathered
  in-scope matches through its own lanes before (170 to 193 since the
  specs/045 tally). When a rebase under FR-011 brings new in-scope
  matches onto the branch, the lane re-derives the base-state table at
  the new merge base, rewrites the new lines under the same decisions,
  and SC-001 still reads zero at the squashed head. Every group count
  names the merge base it was read at.

## Clarifications

### Session 2026-11-10

- Q: SC-002 capped the kept whole-tree matches at 6; does that agree
  with FR-005? A: No. The tree at base reads 30 kept lines, 24 of them
  prose-gate fixture data under `test/`. SC-002 now states the figure
  with its groups; `LICENSE` reads 0.
- Q: Does the tooling pointer `.specify/feature.json` collide with the
  specs/docs-only file rule? A: It is a tracked pointer every lane
  updates, and it carries no record text. FR-006 and SC-005 name it as
  the single exception.
- Q: Can the rewrite make every recorded command runnable again? A: No.
  53 lines name home paths outside the checkout. D-1 keeps one
  uniform `$HOME/` rule, and US-2 claims runnable commands for
  checkout paths only.
- Q: Do D-2 and D-3 cover all 33 login lines? A: They cover 26. A bare
  `owner/repo` slug, an endpoint path quoted without a verb, and a
  quoted workflow expression naming the owner join D-3's `<owner>`
  placeholder rule.
- Q: When master moves during the lane, where do the acceptance reads
  stand? A: At the rebased squashed tip; every base count is re-derived
  at the new merge base and recorded, the shape specs/045 F1-006 used.

## Constitution Checklist

- Main-page impact check (IV.1): no. The lane edits no behavior, option,
  API, or page region the root `README.md` teaches. `README.md` stands
  unchanged, and the drift read proves the page still matches the code.

## Out of Scope

- `source/`, `include/`, `test/`, `example/`, `cmake/`, `.github/`,
  `tools/`, `CMakeLists.txt`, `CODEOWNERS`, `LICENSE`, and `external/`.
- Git history rewriting of any kind (G-27).
- Version, schema, or SOVERSION changes: this lane moves none.
- Any gate, filter, allowance, or CI change: none.
