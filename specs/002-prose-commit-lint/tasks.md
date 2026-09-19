---
description: "Task list for feature 002-prose-commit-lint"
---

# Tasks: Prose and Commit-Message Lint Gate

**Input**: Design documents from `specs/002-prose-commit-lint/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, quickstart.md, contracts/ (all present)

**Tests**: Included. The spec mandates them: FR-023 requires fixtures covering every rule family in both directions, run inside the gate, and the plan fixes a red-first land order (the `dbc_gate_fixtures` block at `test/CMakeLists.txt:89-99` and its convention note at `test/CMakeLists.txt:88`). The fixtures are the tests.

**Organization**: Tasks are grouped by user story so each story is independently implementable and testable after the shared foundation.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1 = refuse violating prose (P1), US2 = refuse malformed commits (P2), US3 = local parity (P3)
- Paths are repository-relative

## Path Conventions

- Gate and rule data: `tools/prose/`
- CMake entry points: `cmake/`
- Fixture package: `test/prose-gate-fixture/`
- CI: `.github/workflows/ci.yml`
- No `src/` or `include/` work: the library is untouched (plan, Additional Constraints)

---

## Phase 1: Setup (red-first fixtures, harness, registration)

**Purpose**: Land the failing tests before any gate code, following the red-first convention recorded at `test/CMakeLists.txt:88` and plan Summary.

- [ ] T001 [P] Write `test/prose-gate-fixture/fixture_rules.yaml`: rule subset per quickstart.md's assertion table (seven prose ids, commit thresholds, sections, vague titles, non-imperative shapes, marker literal `prose-lint: allow`)
- [ ] T002 [P] Write `test/prose-gate-fixture/prose/violations.md`: one reported case per rule family XI.1 through XI.5, one ASCII double-hyphen case, one contrastive-framing case per `contracts/rule-data.md`
- [ ] T003 [P] Write `test/prose-gate-fixture/prose/silent.md`: one silent case per family: en-dash numeric range, inline code span, fenced block, blockquote line quoting a banned token, URL, file path, shell command line, the longer-word cases `candidate` and `adjust`, a marked quotation with a reason, and one CRLF line ending whose trailing `\r` is stripped before matching
- [ ] T004 [P] Write `test/prose-gate-fixture/prose/comments.c`: comment cases for `//` and `/* */`, one reported violation comment and one silent comment (license-header shape, URL comment)
- [ ] T005 [P] Write `test/prose-gate-fixture/prose/comments.sh`: `#` comment cases, one reported and one silent
- [ ] T006 [P] Write `test/prose-gate-fixture/prose/comments.cmake`: `#` comment cases, one reported and one silent
- [ ] T007 Write `test/prose-gate-fixture/commits/cases.yaml`: a case list where each case names its expectation and the harness holds one assert per case: conforming commit; `CM.TITLE-FORMAT` (missing section colon); `CM.TITLE-LENGTH` (51-character title); `CM.SECTION-UNKNOWN` (`Kubernetes: fix crash`); `CM.NON-IMPERATIVE` (`Added feature`); trailing period in title (`CM.TITLE-FORMAT`); `CM.VAGUE-TITLE` (`wip`); `CM.BODY-REQUIRED` (40 changed lines, no body); body-less at 3 changed lines passing; `CM.BODY-WRAP` (body line over 72 columns); marked body line over 72 columns passing once the marker substring is stripped; `CM.FOOTER-APPROVAL` (missing `Approved-by:`); `XI1.EMDASH` in body; merge commit with title and `Approved-by:` and no body passing (R-09); rename-only commit counted at 0 changed lines by `git show --numstat -M`; commit touching a binary file whose `-\t-\t` counts as 0; empty-range case exiting 0
- [ ] T008 Write `test/prose-gate-fixture/run_prose_gate_fixtures.py`: unittest harness skeleton mirroring `test/dbc-gate-fixture/run_gate_fixtures.py`; copies the prose fixtures and builds the commit fixtures in a throwaway git repo under `/tmp`; exits with "gate script not present (TDD red)" while `tools/prose/prose_gate.py` is missing
- [ ] T009 Register `add_test(NAME prose_gate_fixtures ...)` in `test/CMakeLists.txt` mirroring the `dbc_gate_fixtures` block at `test/CMakeLists.txt:89-99` with the red-first comment of `test/CMakeLists.txt:88`; verify red by `cmake --preset=dev -B build/dev && cmake --build build/dev && ctest -R prose_gate_fixtures --output-on-failure`, expect failure naming the missing gate script. Red-phase commits from this task stay local until Phase 2 turns them green, per the landing note

---

## Phase 2: Foundational (rule data and gate kernel)

**Purpose**: The rule file and the load/CLI kernel both checks need. Blocking for every story.

**⚠️ CRITICAL**: No story work begins until this phase is complete.

- [ ] T010 Write `tools/prose/prose_rules.yaml` per `contracts/rule-data.md`: version 1; seven prose rules `XI1.EMDASH`, `XI1.DOUBLE-HYHEN`, `XI2.CONTRASTIVE`, `XI3.VOUCHER`, `XI4.META-EDITORIALIZING`, `XI5.FILLER`, `XI5.MARKETING` with vocabularies copied verbatim from constitution Principle XI.1 through XI.5 (multi-word entries included); `sections` seeded with the nine tokens `CMake`, `CI`, `Constitution`, `Docs`, `Meta`, `dbc`, `deploy`, `runner`, `test` (decision D4, contract sample); `vague_titles` and `non_imperative_shapes` per contract; `auto_exempts` the seven-entry set including `blockquote-line` (decision D2); `marker: "prose-lint: allow"`; `thresholds` 50/72/5; `exclusions` the six seeded prefixes of the contract, `test/prose-gate-fixture/` among them (see T031 for the recorded rationale)
- [ ] T031 Record the exclusions pairing: `test/prose-gate-fixture/` sits in the contract's seeded `exclusions` under the checker-input category FR-006 defines; the corpus carries deliberate violations, is checker input, and its comment files fall inside FR-006's tracked C/CMake/shell scope. The contract entry and this task are the pair; no other artifact repeats the rationale
- [ ] T011 Write `tools/prose/prose_gate.py` kernel: shebang and `main(argv) -> int` and `die()` per the `tools/dbc/` convention; argparse surface per `contracts/cli.md` (`--check`, `--mode`, `--base`, `--head`, `--paths`, `--rules`, `--version`); usage error for `--mode tree` with `--check commit`, and for `--paths` with `--check commit` (exit 2); rule loader with every load-time validation of `contracts/rule-data.md` (parse, version equal to 1, id uniqueness, canonical id coverage of the seven prose rule ids, pattern compile, wildcard prohibition, vocabulary probe pairs, scope validity, threshold bounds), each failure exit 2 naming the field; checks return an empty verdict for now
- [ ] T012 Extend `test/prose-gate-fixture/run_prose_gate_fixtures.py` with loader asserts: wildcard pattern exits 2 naming the field; probe-failing vocabulary rule exits 2; dropped canonical id exits 2, dropping `XI5.FILLER` while `XI5.MARKETING` stays included; out-of-bounds threshold exits 2; a valid file loads and the loader's id set equals the seven canonical prose ids of the `contracts/rule-data.md` namespace table; verify GREEN for the loader subset with `ctest -R prose_gate_fixtures`

**Checkpoint**: Kernel runs, exits 2 on broken rule data, and the loader fixtures are green. Story phases can begin.

---

## Phase 3: User Story 1 - A violating pull request is refused before merge (Priority: P1) 🎯 MVP

**Goal**: The prose check reports every family on added or modified lines, stays silent on grandfathered text and every auto-exempt construct, and names file, line, rule id, and token (FR-001 through FR-008, FR-017, FR-018).

**Independent Test**: Per spec US1: a staged fixture range with one violation per family plus silent cases; the verdict names each violation with file, line, rule id, and nothing else; a clean range exits 0 printing the units-examined count.

### Implementation for User Story 1

All US1 tasks edit `tools/prose/prose_gate.py`; they are sequential by file.

- [ ] T013 [US1] Implement discovery and extraction in `tools/prose/prose_gate.py`: `git ls-files`, exclusions filter, Markdown roots per FR-006 (repo-root README, `AGENTS.md`, `.specify/memory/`, `specs/`, `docs/`), comment extraction for C, C++, CMake, shell, symlinks `stat`-ed and skipped with a printed reason so a linked duplicate cannot double-count findings, and skip-with-reason for invalid UTF-8 or a byte-order mark (spec edge case at spec.md:111)
- [ ] T014 [US1] Implement auto-exempts and the marker in `tools/prose/prose_gate.py`: the seven constructs of `auto_exempts`, fenced block, inline code span, indented code, URL, file path, shell command line, and blockquote line (FR-004); the first match of the `contracts/rule-data.md` "Exemption precedence" total order wins the line; marker detection per the contract's "Marker grammar", substring match, HTML comment wrapper accepted, non-empty reason required, malformed or empty reason raises `MARKER.NO-REASON` on non-auto-exempt lines only (FR-005); no block form
- [ ] T015 [US1] Implement the matcher in `tools/prose/prose_gate.py`: codepoint U+2014 with en-dash U+2013 legal between numeric or alphanumeric endpoints (FR-002); the pinned regex `(?<!-)-{2,3}(?!-)` for `XI1.DOUBLE-HYHEN` per `contracts/rule-data.md`; contrastive regex with the three alternations of the contract; vocabulary compiled to case-insensitive whole-word alternation, verified silent on `candidate` and `adjust` (FR-003, spec.md:103); a trailing `\r` of a CRLF line ending is stripped before matching and column counting
- [ ] T016 [US1] Implement authorship in `tools/prose/prose_gate.py`: range mode parses `git diff -U1 --no-color --no-renames BASE...HEAD`, `+` lines become `new`, the emitted ±1 context lines become `modified`, all else `grandfathered`; deleted-side lines never reported; tree mode marks every line `new` (FR-007, FR-008, data-model authorship table, R-08 amendment)
- [ ] T017 [US1] Implement the verdict in `tools/prose/prose_gate.py`: stderr finding lines in the `contracts/cli.md` format, ordered by location then rule id; stdout summary line with sources, units, findings, skipped counts; exit 0 or 1 per FR-018; `::error file=,line=` annotations only when `GITHUB_ACTIONS=true` (FR-017)
- [ ] T018 [US1] Extend `test/prose-gate-fixture/run_prose_gate_fixtures.py`: assert the reported case per family fires with the file, line, rule id, and token named in the report line, and every silent case stays silent, across Markdown, C, shell, and CMake comment files, marked quotation, and empty-reason marker; assert the authorship fixture where a `modified` line one below an added line fires while a line two below stays grandfathered (R-08 amendment); assert clean tree exits 0 with a nonzero units count (US1 scenarios 1 through 8); verify `ctest -R prose_gate_fixtures` GREEN

**Checkpoint**: US1 fully functional: `python3 tools/prose/prose_gate.py --check prose` reproduces fixture verdicts.

---

## Phase 4: User Story 2 - A malformed commit message is refused (Priority: P2)

**Goal**: Every commit in a resolved range is checked against the eight `CM.*` rules and the prose rules over title and body; verdicts name commit, rule, and text (FR-009 through FR-016).

**Independent Test**: Per spec US2: run `--check commit` against the throwaway repo's fixture range; two of five non-conforming commits are named by short hash and rule, three pass, and each template rule fires on its case.

### Implementation for User Story 2

All US2 tasks edit `tools/prose/prose_gate.py`; sequential by file.

- [ ] T019 [US2] Implement range resolution in `tools/prose/prose_gate.py`: default base merge-base with `origin/master`, exit 2 when unresolvable (never an empty pass, R-07); `git rev-list BASE..HEAD` two-dot enumeration while the prose diff stays three-dot (R-07 amendment); empty resolved range is success with zero records (spec.md:110); per-commit `git show --numstat -M` changed-lines sum with binary `-\t-\t` counted as 0 and merge commits summing to 0 (R-10 amendment); parents parse with merge-commit flag (R-09)
- [ ] T020 [US2] Implement CommitRecord parsing in `tools/prose/prose_gate.py`: title is line one, body after the first blank line, footers parsed from the trailing paragraph as `Key: value`, short hash at 7 hex for reporting (data-model CommitRecord table)
- [ ] T021 [US2] Implement the eight commit rules in `tools/prose/prose_gate.py`: `CM.TITLE-FORMAT`, `CM.TITLE-LENGTH`, `CM.SECTION-UNKNOWN`, `CM.NON-IMPERATIVE`, `CM.VAGUE-TITLE`, `CM.BODY-REQUIRED` with the trivial-changed-lines exemption and the merge-commit exception, `CM.BODY-WRAP` measured with exemption marker substrings stripped, `CM.FOOTER-APPROVAL` (FR-009 through FR-014; data-model commit rule table)
- [ ] T022 [US2] Apply the prose rules to commit title and body in `tools/prose/prose_gate.py` with scopes `commit-title` and `commit-body`, with auto-exempts and the marker available to quoted lines (FR-015; spec edge case at spec.md:109; example line `commit 1a2b3c4: XI1.EMDASH` from `contracts/cli.md`)
- [ ] T023 [US2] Extend `test/prose-gate-fixture/run_prose_gate_fixtures.py`: drive every `commits/cases.yaml` case in the throwaway repo with one assert per case against its named expectation (T007's enumeration covers all eight `CM.*` rules plus the passing merge, rename, binary, and empty-range cases), including the US2 mixed range with two offenders named by short hash; verify `ctest -R prose_gate_fixtures` GREEN

**Checkpoint**: US1 and US2 both functional and independently testable.

---

## Phase 5: User Story 3 - The same verdict locally and in CI (Priority: P3)

**Goal**: One documented command produces the CI verdict anywhere; entry points converge on identical program arguments (FR-019; US3 scenarios).

**Independent Test**: Per spec US3: run the command on violating and clean checkouts, compare against simulated CI output from identical inputs, verdicts match modulo `::error` lines; whole-tree mode completes inside the SC-003 bound.

### Implementation for User Story 3

- [ ] T024 [US3] Write `cmake/prose-lint.cmake`: script-mode module in the `cmake/spell.cmake:1-7` style with `cmake_minimum_required(VERSION 3.14)` and a `default()` macro; variables `PROSE_MODE`, `PROSE_BASE`, `PROSE_HEAD`, `PROSE_CHECK`, `PROSE_RULES`, `PYTHON_COMMAND`; every `-D` documented to precede `-P` on the invocation, per the `contracts/cli.md` section 2 warning; `execute_process` at `CMAKE_SOURCE_DIR`; `message(FATAL_ERROR ...)` so exit 1 and 2 both surface as the wrapper's exit 8 per `contracts/cli.md` section 2; no FIX flag
- [ ] T025 [US3] Write `cmake/prose-lint-targets.cmake`: `add_custom_target(prose-lint)` re-invoking the script module in range mode, and `add_custom_target(prose-lint-fixtures)` running the harness, neither in `ALL`; add `include(cmake/prose-lint-targets.cmake)` in `cmake/dev-mode.cmake` beside the lint and spell target includes (the include block at `cmake/dev-mode.cmake:19-20`); verify `cmake --build build/dev -t prose-lint` and `-t prose-lint-fixtures`
- [ ] T026 [US3] Extend `test/prose-gate-fixture/run_prose_gate_fixtures.py`: parity assert that a run with and a run without `GITHUB_ACTIONS=true` yield identical finding sets once `::error` lines are removed; wall-clock assert that whole-tree mode on this repo stays under the 10 s SC-003 bound (plan R-04 measured 0.062 s), evaluated only when `GITHUB_ACTIONS` is unset because the bound is a developer-machine bound; verify GREEN
- [ ] T027 [US3] Add the quality-gate paragraph to `README.md`: the single command `cmake -P cmake/prose-lint.cmake`, the equivalent direct and build-target invocations, exit-code meaning in one line, placed beside the existing format-check, spell-check, and coverage guidance (FR-019)

**Checkpoint**: `cmake -P cmake/prose-lint.cmake` is the one command, documented, identical verdict to CI.

---

## Phase 6: Polish and gate closure

**Purpose**: Make the tree pass its own gate, amend the constitution, and add the CI job in the single landing change FR-022 requires.

- [ ] T028 Run `python3 tools/prose/prose_gate.py --check prose --mode tree --paths .specify/memory/constitution.md AGENTS.md` and match the findings against the canonical self-quotation inventory in `contracts/rule-data.md`; add `prose-lint: allow` markers with a quotation reason to the lines the inventory marks as requiring one (plan R-06; spec.md:107), leaving fenced and code-span quotations to the auto-exempt. Acceptance: the same scoped tree command exits 0 (SC-005)
- [ ] T029 Amend `.specify/memory/constitution.md` to version 2.5.0 (MINOR per Governance): resolve both entries in the Open deferrals block (line 8 region); replace the Principle XI.6 deferral sentence (lines 397-399) with the delivered check naming `prose-lint`; update the Principle VIII prose-gate bullet (lines 189-190) from pending to delivered; add the 2.5.0 lineage row and bump the version footer dates (FR-022, SC-005)
- [ ] T030 Add the `prose-lint` job to `.github/workflows/ci.yml` exactly per `contracts/ci-job.md`: `needs: [lint]`, `runs-on: ubuntu-26.04`, checkout `fetch-depth: 0`, apt `python3-yaml`, the `Resolve range` step as the contract's YAML block (`set -euo pipefail`, `$GITHUB_ENV` exports, `git cat-file -e` and `git merge-base --is-ancestor` validation, the `before` fallback to `github.event.commits[0].id^`, exit 2 when that parent is unusable or `BASE` equals `HEAD`, never a vacuous pass), the `cmake -P cmake/prose-lint.cmake` gate step, then a named `Configure` step with `ci-ubuntu`, `-t prose-lint-fixtures || true`, and `ctest -R prose_gate_fixtures --output-on-failure --no-tests=error` (FR-016, FR-020, FR-023; land in the same PR as T029 so no window exists between constitution and CI)
- [ ] T032 Run `quickstart.md` end to end and record evidence: clean range exits 0 with the summary line; a staged violating line exits 1 naming file, line, rule id, and token; a 51-character fixture title exits 1 with `CM.TITLE-LENGTH`; whole-tree mode prints wall time under 10 s; `ctest -R prose_gate_fixtures` passes; `cmake --build --preset=dev` unaffected; `cmake -P cmake/spell.cmake` passes over the new files; `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake` is unaffected; the `contracts/ci-job.md` reproduction block runs clean; the `contracts/cli.md` section 2 vars-land probe prints the resolved variables

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: no dependencies; red-first by design
- **Foundational (Phase 2)**: depends on Phase 1 (fixtures exist to fail); BLOCKS all stories
- **US1 (Phase 3)**: depends on Phase 2
- **US2 (Phase 4)**: depends on Phase 2; shares `prose_gate.py` with US1, so sequence after US1 when one agent works the file
- **US3 (Phase 5)**: depends on Phases 3 and 4 being functionally complete (parity needs both checks)
- **Polish (Phase 6)**: depends on all stories; T030 and T029 land in one PR (FR-022)

### Cross-file parallelism

- T001 through T006 are mutually parallel (distinct fixture files)
- T010, T031, and T011 are parallel (distinct files; T012 needs the code pair, T031 is the register note beside T010)
- Within one file (`prose_gate.py` in T011 and T013 through T017; the harness in T008, T012, T018, T023, T026) tasks are strictly sequential
- T028 and T029 (docs) never touch `tools/`, so they parallelize against code work once T015 is in place and markers can be validated

### Within Each User Story

- Fixtures exist and fail before the code that satisfies them (Phase 1 doctrine holds throughout)
- Extraction and matching before authorship before verdict formatting
- Range resolution before commit rules
- Entry points after the program they wrap

## Parallel Example: Phase 1

```bash
# Launch together (distinct files):
Task: "T001 fixture_rules.yaml"
Task: "T002 prose/violations.md"
Task: "T003 prose/silent.md"
Task: "T004 prose/comments.c"
Task: "T005 prose/comments.sh"
Task: "T006 prose/comments.cmake"
# Then T007, T008, T009 in order (harness needs fixtures, registration needs harness).
```

## Parallel Example: Foundational then stories

```bash
# Launch together (distinct files):
Task: "T010 tools/prose/prose_rules.yaml"
Task: "T011 tools/prose/prose_gate.py kernel"
# After T012 is green, a single agent walks T013 through T017 (one file).
```

---

## Implementation Strategy

### MVP First (User Story 1)

1. Phase 1: fixtures, harness, registration; confirm RED
2. Phase 2: rule data and kernel; loader fixtures GREEN
3. Phase 3: prose check; US1 fixtures GREEN
4. **STOP and VALIDATE**: `python3 tools/prose/prose_gate.py --check prose` against the fixture range; the gate has standalone enforcement value even before Phase 4

### Incremental Delivery

1. Phase 1 + 2 → foundation ready
2. US1 → prose enforcement (MVP)
3. US2 → commit-template enforcement
4. US3 → one command, documented parity
5. Phase 6 → constitution amendment and CI job land together; deferrals closed, SC-005 true

### Notes on this feature's landing

- Everything above is one pull request: the plan red-first order governs commits inside it
- Landing includes marking the `prose-lint` job a required status check in branch protection, the same standing as the eight existing jobs (SC-001)
- The tree-wide em-dash sweep stays out (plan, Principle V formatting-only change elsewhere)
- Exclusion conformance: `test/prose-gate-fixture/` sits among the contract's seeded six exclusion prefixes under the checker-input category FR-006 names; the rationale lives once in `contracts/rule-data.md` and T031 records the pairing

## Notes

- [P] tasks = different files, no dependencies
- Rule ids are stable; renaming one is a breaking change (data-model preamble)
- Exit 2 is never a pass (cli.md exit-code table)
- Identical input means byte-identical findings in CI and locally (parity rule, cli.md section 4)
- Commit after each phase checkpoint; template compliance applies to those commits too
