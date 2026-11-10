# Implementation Plan: Names and User Paths Out of Old Records (T-NAMES)

**Branch**: `049-names-old-records` | **Date**: 2026-11-10 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `specs/049-names-old-records/spec.md`

## Summary

Rewrite 193 record lines in 71 files under `specs/` so the agreed name
grep reads clean, then prove the rewrite with the grep, the gate reads,
and identifier-preservation checks. Two token families carry all matches:
a home-path prefix (160 lines) and a repository login inside recorded
GitHub URLs, endpoints, and slugs (33 lines). One throwaway rewriter
script applies the spec's uniform rules; per-lane workers rewrite
disjoint file groups; the orchestrator verifies, commits, squashes,
pushes, and opens one pull request.

## Technical Context

**Language/Version**: Markdown, reStructuredText, and plain-text records.
The rewriting tool is a throwaway Python 3 script kept outside the
repository tree.

**Primary Dependencies**: `git grep`, Python 3 standard library,
`codespell` with `.codespellrc`, `tools/docs/example_drift.py`,
`tools/prose/prose_gate.py`.

**Storage**: tracked text files under `specs/` only; `docs/` carries no
match at base and stays unchanged.

**Testing**: the grep reads themselves. The red state is the 193-line
base count; the green state is zero lines in `specs` and `docs` and
exactly the 30 kept lines tree-wide. `git diff` inspection proves
identifier preservation.

**Target Platform**: Linux. This host reports kernel
`7.3.3-1-cachyos`, GCC 16.2.1 20260810, Clang 23.1.4, AMD vendor
`AuthenticAMD`, family 26 (0x1A), model 68 (0x44), Ryzen 9 9950X3D.

**Project Type**: repository records only. No build target changes.

**Performance Goals**: none. One grep over 71 files is instant.

**Constraints**: no source, header, test, example, CMake, CI, or gate
file changes (FR-006); the tracked `.specify/feature.json` pointer is
the one exception; no history rewrite (FR-008, G-27); marker count 84
stands (FR-007); version, schema, and SOVERSION numbers do not move.

**Scale/Scope**: 193 lines, 71 files, 18 lanes; seven work groups sized
by line count (research.md table).

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- I, II, VI, VII: no C++ change, no coverage or performance surface
  moves. The marker read of FR-007 reports a figure and changes
  nothing.
- III: requirements are EARS-shaped in the spec; this plan carries the
  design and the test plan. TDD mode applies: each group's grep is
  observed red at base and green after the rewrite, recorded per group.
- IV.1: no main-page impact. `README.md`, `example/`, and `docs/pages`
  regions stand; the drift read proves it.
- V: the format gate reads C++ patterns only; no formatted file is
  touched.
- VIII: the lane runs the spell, drift, and prose reads (FR-009), the
  commit-lint read on every commit, the marker count (FR-007), and one
  `ci-rocky` release build with the full `ctest` suite at the lane's
  final tree (task T017), so the build and suite reading stands.
- IX: all four artifacts exist (spec, plan, tasks, and the rewritten
  records as the delivered change).
- X: every changed line traces to FR-002 or FR-003 (X.3). The step and
  check plan below is the X.4 form. No speculative tooling: the
  rewriter lives in `/tmp` and enters no tracked path.
- XI: every rewritten line is checked by the prose gate over the range;
  grandfathering per XI.1 leaves untouched lines alone.

Gate verdict: PASS, no violations to justify.

## Design

### Logical view: token model and decision table

One record line carries zero or more tokens. The rewrite maps each token
to one replacement. No other edit enters a line, except the two
sentence-level repairs the uniform run-link row produces in specs/014,
named in T009.

| Token shape | Replacement | Spec basis |
|---|---|---|
| `/home/<user>/` followed by a path | `$HOME/` | D-1 |
| bare `/home/<user>` with no trailing path component | `$HOME` | D-1 |
| `[<id>](https://github.com/<owner>/speedgun-ng/actions/runs/<id>)` | `run <id>`, uniformly, the link text dropped | D-2 |
| bare Actions URL | `run <id>` | D-2 |
| pull-request link | link text kept as plain text, `#<n>` surviving | D-2 |
| bare pull-request URL | `PR #<n>` | D-2 |
| `gh api`/`gh run` endpoint path naming the login | same path with `<owner>` | D-3 |
| bare `<login>/<repo>` slug | `<owner>/speedgun-ng` | D-3 |
| endpoint path quoted without a verb | same path with `<owner>` | D-3 |
| quoted workflow expression naming the owner | `<owner>` inside the quotes | D-3 |
| personal email address | role word | D-5; none in scope |

Guard: the replacement text contains no token the agreed grep matches,
so every rewritten line satisfies FR-001 by construction. The bracketed
grep classes already satisfy it for lines that quote the grep itself.

### Physical view

Touched: 71 recorded files under 18 `specs/NNN-*` directories, plus
`specs/049-names-old-records/` artifacts and the pointer
`.specify/feature.json`. Untouched: every other path, by FR-006.
`git diff --name-only origin/master...HEAD` is the enforcement read.

### Test plan (TDD)

1. Red at base: the agreed grep prints 193 lines over `specs docs`; the
   per-group table in research.md fixes each group's count.
2. Rewrite one group. Its own grep over its directory prints 0 lines
   (green for the group) and its `git diff` lines show only token
   replacements.
3. Whole-scope green: grep over `specs docs` prints 0 lines; the
   tree-wide read prints the kept lines only, and their count matches
   the kept groups (research.md).
4. Identifier census: two reads of one scan, the alternation
   `\b[0-9a-f]{4,}\b|\b[0-9]{2,}\b` counting each match position once.
   The recorded base multiset runs over the 193 saved bodies as
   written (49 distinct, 146 occurrences). The equality read masks
   markdown links to URLs, bare `https?://` spans, a case-insensitive
   `run <digits>` word, `PR #<digits>`, and `#<digits>` spans on both
   sides (the D-2 rows replace those spans by construction): 39
   distinct, 122 occurrences at base, and the same masked multisets
   after the rewrite, line by line (FR-004).
5. Gates exit 0: spell, drift, prose over the range; marker count 84.

## Step and Check Plan (X.4)

```text
1. Record base state      -> verify: results/base-state.md counts equal
   the spec tally for every group; marker read prints 84
2. Write rewriter         -> verify: script dry-run over one group
   prints the per-file replacement counts and writes nothing
3. Rewrite group G (x7)   -> verify: rewriter dry-run counts equal the
   group occurrence figures (A 55, B 32, C 26, D 16, E 23, F 38, G 8;
   198 occurrences over 193 lines); the agreed grep over the group
   prints 0 lines; git diff lines changed equal the group's base line
   count
4. Identifier census      -> verify: before and after multisets equal
5. Whole-scope greps      -> verify: 0 lines in specs docs; tree read
   equals the 30 kept lines
6. Gate reads (pre-squash)-> verify: codespell, example_drift,
   prose_gate range, marker read each exit 0 with printed figures
7. D-10 rebase            -> verify: after the rebase the agreed grep
   over specs docs prints 0 lines, with rewriter rewrites of any
   arrivals first and a dated addendum to results/base-state.md
8. Head readings          -> verify: ci-rocky build with the full
   ctest suite passes at the pre-squash head; results/acceptance.md
   is committed as an artifact commit naming that head, the host
   figures, and every reading, with the note that the built head
   differs from the squashed tree only by lane-record files under
   the feature directory
9. Squash                 -> verify: the FR-012 grep walks every lane
   commit first; one commit with OmO identity; prose commit-lint exit
   0; the fast reads re-run clean at the squashed head
10. Push and PR           -> verify: ls-remote equals local head, one
    open PR whose body names the squashed sha
```

## Project Structure

### Documentation (this feature)

```text
specs/049-names-old-records/
├── spec.md
├── plan.md              # this file
├── research.md          # base measurements and the group table
├── data-model.md        # token and line entities
├── quickstart.md        # the reads any reviewer repeats
├── checklists/requirements.md
├── tasks.md
└── results/
    ├── base-state.md
    ├── group-log.md
    └── acceptance.md
```

### Source Code (repository root)

No source change. The 71 touched record files sit inside the existing
`specs/` trees.

**Structure Decision**: records stay where they are; the lane adds files
only under its own spec directory.

## Parallelization

Seven work groups (research.md) own disjoint file sets and all seven
run in one wave. Workers edit in the shared worktree without touching
git; the orchestrator commits after each group and runs the group's
green read.

## Complexity Tracking

No violations. No new projects, abstractions, or dependencies.
