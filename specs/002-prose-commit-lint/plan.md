# Implementation Plan: Prose and Commit-Message Lint Gate

**Branch**: `002-prose-commit-lint` | **Date**: 2026-09-11 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `specs/002-prose-commit-lint/spec.md`

## Summary

One Python 3.12 gate script, `tools/prose/prose_gate.py`, enforcing the
six discourse rule families of constitution Principle XI over tracked
Markdown and over comments in C, C++, CMake, and shell sources, plus the
commit-message template of Pull Request Quality over a git commit range.
One machine-readable rule file, `tools/prose/prose_rules.yaml`, holds the
vocabulary, the section-token list, the vague-title list, the exclusions,
and the numeric thresholds, so the constitution stays the normative text
and the file is its mechanical projection. A `cmake -P` entry point and a
CMake target wrapper give one command that produces the CI verdict
locally. A new Linux CI job `prose-lint` runs both checks and the gate's
own fixtures, which closes the two deferrals recorded in the Sync Impact
Report: the machine check Principle XI.6 promises and the commit-template
lint.

Land order is red-first, following the convention at
`test/CMakeLists.txt:88-89`: the fixture harness lands before the gate, so
the first commit fails for the right reason.

## Technical Context

**Language/Version**: Python 3.12, the distribution interpreter on the
`ubuntu-26.04` runners and the floor for local runs (decision D1). The
`prose-lint` job does not run `actions/setup-python`: that step belongs to
the `lint` job (`ci.yml:19-20`) and installs a standalone CPython without
PyYAML. No C++ is added, so the C++23
pin, the pinned Core Guidelines, and `CMAKE_CXX_EXTENSIONS=OFF` are
untouched.

**Primary Dependencies**: Python standard library, plus PyYAML for the
rule file. PyYAML reaches runners through apt `python3-yaml`
(`ci.yml:48`, `81`, `110`, `156`, `247`) and dnf `python3-pyyaml`
(`ci.yml:133`), never pip. The git CLI supplies history, diff, and
`--numstat`. Nothing else, which is what satisfies FR-021 and SC-006.
Evidence and rejected alternatives: [research.md](./research.md) R-01 and
R-02.

**Storage**: The rule file `tools/prose/prose_rules.yaml` for reading, and
fixture files under `test/prose-gate-fixture/` for reading. The gate writes
nothing outside the temporary directory its commit fixtures use. No state,
no cache, no database.

**Testing**: `test/prose-gate-fixture/run_prose_gate_fixtures.py`,
registered through `add_test(NAME prose_gate_fixtures ...)` in
`test/CMakeLists.txt`, mirroring `test/CMakeLists.txt:90-99`, and run in CI
through `ctest -R prose_gate_fixtures --output-on-failure
--no-tests=error`. The `unittest` module covers the extractor's internal
units inside the same harness. Rationale at R-13.

**Target Platform**: Linux, `runs-on: ubuntu-26.04`, the label every
existing job uses. Locally any machine with Python 3.12 or newer and
git: 3.12 is both the floor and the CI interpreter (decision D1).

**Project Type**: Repository tooling for a C++23 library. One gate script,
one rule file, two CMake modules, one CI job, one fixture package, one
README paragraph.

**Performance Goals**: Whole-tree scan under 10 s per SC-003. Measured on
this checkout: the 60 in-scope files of the canonical count below,
scanned line by line against the full banned vocabulary in **0.062 s**
(R-04 and its amendment). The `prose-lint` CI job target
stays well under two minutes including the full-history fetch.

**Constraints**: No new dependency (FR-021, SC-006). Exit codes 0 for
clean, 1 for findings, 2 for usage or an unusable environment, matching
`tools/dbc/dbc_gate_common.py:10-14`. An unresolvable range exits 2, and is
never treated as an empty range that passes, which is the
`cmake/coverage.cmake:6-13` doctrine that a missing prerequisite never
becomes a skip. Pull-request mode reports added and modified lines only,
which turns the grandfathering clause of Principle XI.1 into behavior
(FR-007).

**Scale/Scope**: 60 in-scope files, 15 Markdown and 45 source, measured
2026-09-16 by the canonical command:

```sh
git ls-files -- '*.md' '*.c' '*.cc' '*.cpp' '*.h' '*.hpp' '*.cmake' '*.sh' '*CMakeLists.txt' \
  | grep -vE '^(\.opencode/|\.specify/scripts/|\.specify/templates/|build/|docs/images/|test/prose-gate-fixture/)' | wc -l
```

The command is the source of truth as the tree moves. Six prose
families covered by seven prose rules, plus eight commit rules. 225
em-dash occurrences across 12 files are pre-existing and stay silent
in range mode; whole-file mode is opt-in until the Principle V sweep
lands (R-05, R-07).

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-checked after Phase 1 design.*

| Principle | Status | Basis |
| --- | --- | --- |
| I Standard-First Coding | Pass | No C++ added, so the pinned Core Guidelines baseline and the clang-tidy and cppcheck presets are untouched. No new P2 exception is registered. Adopting JSON rule data would have been an unregistered deviation from the repo's convention, so the plan follows the convention instead (R-02). |
| II Design By Contract | Pass with interpretation | The doxygen and contract-facility requirements bind the C++ library, and the DBC gate reads doxygen XML against `tools/dbc/macros.yaml`. The gate adds no C++ interface. The Python follows the existing `tools/dbc/` pattern: argparse preconditions, explicit usage failure through `die()`, type hints. Recorded as an interpretation at R-14, matching the five existing Python gate scripts, none of which carries doxygen markup. |
| III R-DCUT | Pass | Requirements in `spec.md`, design here with the logical view in `data-model.md` and the physical view below, test plan in `quickstart.md` and R-13. TDD is used, and Principle III requires it to be recorded here. |
| IV Documentation | Pass | Docstrings on the gate's functions, banned-token-free comments, no TODO or FIXME, descriptive names, and a threshold in data with the constitution subsection named beside it. |
| V Style and Formatting | Pass | New `.py`, `.cmake`, and `.yaml` files fall outside `format-check`, whose `PATTERNS` covers `source/`, `include/`, `test/`, and `example/` C++ only (`cmake/lint.cmake:10-16`), and stay inside the global `spell-check`. No formatting-only change is mixed in: the Principle V sweep of the 225 grandfathered em-dashes is explicitly excluded from this feature. |
| VI Test-Backed Code and Coverage | Pass | The feature ships fixtures in the same change and runs them in CI. The 100 percent line, branch, and DBC gates measure the C++ library through gcov and the DBC registry, and adding no C++ leaves them where they are. |
| VII Performance Discipline | Pass | No critical-path code. The measured budget, 0.062 s whole tree against a 10 s bound, and the CI job cost are recorded as measurements. |
| VIII CI Quality Gates | Pass, with a required companion change | The job is additive, follows existing job conventions, and is re-creable interactively from the same command. Changing the gate set requires a constitution amendment, which FR-022 mandates in the same landing change; the plan carries it as a task inside this feature. |
| IX Spec-Driven Development | Pass | This plan is the Design stage artifact in the canonical location. `tasks.md` and the implementation follow, and all four artifacts are required. |
| X Anti-Slop Code Discipline | Pass | X.1: assumptions and every decision are written down with evidence in `research.md`. X.2: no plugin architecture, no abstraction for hypothetical languages, no `--format=json` for an absent consumer, one script and one data file. X.3: surgical, five new paths plus five existing files edited, the constitution amendment among them. X.4: every claim below is a command, an exit code, or a measured number. |
| XI Discourse and Prose Standards | Pass | These artifacts quote banned tokens only inside code spans, use no em-dash, and carry the exemption decision for the constitution's own self-quoting lines (R-06), which is the gap that would otherwise let the governing document fail its own gate. |
| Additional Constraints | Pass | The library is untouched, so zero external runtime dependencies holds (SC-006). The CMake script modules follow the `cmake_minimum_required(VERSION 3.14)` and `default()` style of `cmake/spell.cmake:1-7`. `CMakeUserPresets.json` is untouched and BSD 3-Clause is unaffected. |
| Pull Request Quality | Pass | Commit-mode rules mirror the template: `<Section>: <imperative>` at 50 characters or fewer, a 72-column body, an `Approved-by:` footer, and `Fixes #n` and `Refs: specs/NNN-name` recognized. The landing change carries them. |

**Gate verdict**: no violations, so Complexity Tracking stays empty.

**Required companion change inside this feature (FR-022, SC-005)**: amend
the constitution to version 2.5.0, MINOR under Governance because guidance
is materially expanded while no principle is redefined. It resolves the two
Sync Impact Report deferrals, names the delivered check in Principle XI.6
in place of the deferral sentence, adds the gate to Principle VIII's gate
list, and adds the version lineage row. After that merge no open prose or
commit-lint deferral remains anywhere in the tree.

## Project Structure

### Documentation (this feature)

```text
specs/002-prose-commit-lint/
├── spec.md                            # Requirements, merged in PR #8
├── plan.md                            # This file
├── research.md                        # Phase 0: R-01 through R-14, evidence
├── data-model.md                      # Phase 1: logical view, entities, rule-data schema
├── quickstart.md                      # Phase 1: run and validation guide
├── checklists/
│   └── requirements.md                # Requirements-quality review, reviewer-owned
└── contracts/
    ├── cli.md                         # argv, modes, exit codes, report format
    ├── rule-data.md                   # prose_rules.yaml schema and validation
    └── ci-job.md                      # prose-lint job contract, inputs, annotations
```

### Source Code (repository root)

```text
tools/prose/
├── prose_gate.py                      # The gate: prose check, commit check, one entry point
└── prose_rules.yaml                   # Rule data: 6 families, sections, vague titles,
                                       # non-imperative shapes, exclusions, thresholds

cmake/
├── prose-lint.cmake                   # Script mode entry: cmake -P cmake/prose-lint.cmake.
│                                      # Resolves git, maps exit codes like cmake/spell.cmake
└── prose-lint-targets.cmake           # add_custom_target(prose-lint) wrapper, included from
                                       # cmake/dev-mode.cmake beside the lint and spell targets

.github/workflows/
└── ci.yml                             # One job added: prose-lint, needs [lint], fetch-depth 0

test/
├── CMakeLists.txt                     # One add_test added: prose_gate_fixtures
└── prose-gate-fixture/
    ├── run_prose_gate_fixtures.py     # Harness: builds a throwaway commit repo in tmp
    ├── fixture_rules.yaml             # Rule subset the fixtures assert against
    ├── prose/
    │   ├── violations.md              # One reported case per rule family
    │   ├── silent.md                  # One silent case per rule family
    │   ├── comments.c                 // Comment cases for // and /* */
    │   ├── comments.sh                # Comment cases for shell
    │   └── comments.cmake             # Comment cases for CMake
    └── commits/
        └── cases.yaml                 # Conforming commit plus one violation per template rule

README.md                              # One paragraph: the single local command, placed
                                       # beside format-check, spell-check, and coverage
```

**Structure Decision**: The gate lives under `tools/prose/`, sibling to
`tools/dbc/`, because that is where this repository keeps gate scripts and
their data, and because the DBC gates establish every convention the new
script follows: shebang, `main(argv) -> int`, `die()`, exit 0, 1, 2,
findings on stderr, one summary line on stdout. Two CMake modules are
needed, because the repository holds two distinct shapes and each serves a
caller: a script-mode module invoked as `cmake -P`, which is the
`cmake/spell.cmake` and `cmake/lint.cmake` pattern and gives CI and a
build-free local run the same command, and a target wrapper included from
`cmake/dev-mode.cmake:19-20` so `ninja prose-lint` works inside a
configured build directory. The `dbc-gate.cmake` shape, an included module
defining a custom target and nothing else, would deny the CI step the same
`cmake -P` command developers run, which FR-019 and US3 scenario 3 forbid.
`test/prose-gate-fixture/` mirrors `test/dbc-gate-fixture/`, including the
harness name pattern and the `add_test` registration.

**Physical touch list**: the five new paths above, plus five existing files
edited, `.github/workflows/ci.yml`, `test/CMakeLists.txt`, `README.md`,
`cmake/dev-mode.cmake` (one `include` line beside its 19-20 block), and
`.specify/memory/constitution.md` for the FR-022 amendment. Nothing else.
`CMakeLists.txt` needs no edit because `cmake/dev-mode.cmake` is where the
target wrappers are included.

## Phase 1 Artifacts

| Artifact | Contents |
| --- | --- |
| [data-model.md](./data-model.md) | Logical view: Rule, RuleDataFile, ExemptionMarker, CheckedUnit, Finding, Verdict, CommitRecord, with states, validation rules, and the rule-file schema |
| [contracts/cli.md](./contracts/cli.md) | Command line, three modes, exit codes, report and annotation formats, and the CI-to-local parity rule |
| [contracts/rule-data.md](./contracts/rule-data.md) | `prose_rules.yaml` schema, per-field validation, the wildcard prohibition, and the constitution-to-data mapping table |
| [contracts/ci-job.md](./contracts/ci-job.md) | `prose-lint` job definition, checkout depth, event inputs, range resolution, failure behavior |
| [quickstart.md](./quickstart.md) | Prerequisites, local and range runs, fixture run, CI reproduction, adding an exemption, adding a section token |

## Constitution Re-Check After Phase 1

Re-run against the design, with the twelve rows above re-derived:

- **Structure**: no new dependency, no C++, no library surface, and no
  build graph change beyond one included CMake module. Principles I, II,
  VI, and Additional Constraints hold unchanged.
- **VIII**: additive job with `needs: [lint]` and the same conventions, and
  the amendment is inside the change, so the gate set never contains a gate
  the constitution fails to list.
- **IX**: all four artifacts exist after this command, and `tasks.md`
  belongs to the next command.
- **X.2**: the design was searched for speculative parts and none survive.
  No `--format=json`, no rule-class registry, no abstraction layer beyond
  one comment-extraction function covering the four languages FR-006
  names, no block-form exemption toggle, and no cache. The
  file-per-language question is answered with one module, because two
  checks sharing one rule model would be structure without a second
  caller.
- **X.3**: five existing files touched, each for a stated reason, and the
  grandfathered em-dash sweep stays out, so no formatting-only change is
  mixed with content.
- **XI**: the design's own prose holds the rules, and R-06 records the
  constitution's self-quoting lines as exempt in the rule data, which
  removes the only self-contradiction the gate would otherwise create on
  the day it lands.
- **Risk found in Phase 1, with its answer**: whole-file mode is hostile as
  a default today, 225 findings in 12 files nobody is editing. The design
  defaults the local command to range mode against the `origin/master`
  merge-base, keeps whole-file mode opt-in, and never makes it the CI
  default before the sweep. Recorded at R-07 and in the CLI contract.

**Post-design verdict**: gate passes, no Complexity Tracking entry.

## Complexity Tracking

No violations to justify.
