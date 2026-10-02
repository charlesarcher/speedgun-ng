# Implementation Plan: Linux as the Supported Platform

**Branch**: `010-linux-only` | **Date**: 2026-10-02 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/010-linux-only/spec.md`

**Artifacts**: [research.md](research.md) · [contracts/platform-policy.md](contracts/platform-policy.md) · [quickstart.md](quickstart.md) · [checklists/requirements.md](checklists/requirements.md)

## Summary

Narrow the project's declared support to Linux in every surface that makes
the claim. Every claim in this plan is measured, and each measurement names
its command. Linux is the sole platform the project builds, tests,
and gates; macOS and Windows are unsupported today and a future specification
adds them.

The CI matrix already reflects that. All eleven jobs run on `ubuntu-26.04`,
one of them in a Rocky Linux container, and a token sweep of `.github/`
returns zero platform hits (research.md R-007). What still claims otherwise
is the governance document, the preset set, two prose documents, and three
runtime configure-time diagnostics in the vendored autotools module.

Nothing in the code changes. Every `if(WIN32)`, `if(APPLE)`, `if(MSVC)`, and
`if(UNIX)` block stays byte-identical, and so does every `_WIN32`
preprocessor branch. The file-level strings that *are* claims, a comment
naming a package manager and three `FATAL_ERROR` diagnostics, change; the
branches selecting platform behaviour around them do not
(research.md R-003). No abstraction is introduced for a port that may never
land, which is the generality Principle X.2 forbids.

The five merged vendor specs each state that macOS must keep building for
developers. They stay as dated records and are superseded by name from the
constitution, which is what retires `specs/009-vendor-quill` T038 without
editing a merged feature.

## Technical Context

**Language/Version**: No C++ change. C++23 (`CMAKE_CXX_EXTENSIONS=OFF`)
governs what the project must keep building, and this feature builds it.

**Primary Dependencies**: No dependency change. CMake ≥ 3.20 preset-driven
configuration, and the vendored autotools module's diagnostics are the only
build-system text edited.

**Storage**: none; no runtime data, no schema, no persistent state

**Testing**: CTest, unchanged. No new test entry, because every requirement
here is verified by reading a named file or by diffing against the base
commit, which is what [quickstart.md](quickstart.md) records. A new CTest
entry would need a target, and there is no target.

**Target Platform**: Linux, on the distribution families the CI jobs already
cover: `ubuntu-26.04` for the main jobs and `rockylinux:10` for `test-rocky`.
macOS and Windows are out of scope.

**Project Type**: C++ library with a CMake build, changing its governance and
documentation

**Performance Goals**: none. No code compiles differently after this change,
and the shipped artifact is byte-identical.

**Constraints**: the tracked diff touches exactly five files and nothing else
(SC-006); no platform branch moves (FR-016); no gate, threshold, warning
class, analyzer invocation, job, or dependency changes (FR-019); no merged file
under `specs/` changes (FR-017)

**Scale/Scope**: one governance amendment, seven preset deletions, four
diagnostic and comment lines in one module, two prose documents, one
machine-local preset fix. Five tracked files, 0 lines of C++.

## Constitution Check

*GATE: evaluated before Phase 0 research and re-evaluated after Phase 1
design.*

### Pre-research evaluation

| Principle | Gate | Verdict |
|-----------|------|---------|
| I | Standard-first | PASS. No dependency, no standard, and no library change. The change removes build-configuration surfaces, which is deletion, the preferred direction. |
| II | Design By Contract | PASS. No public API, so no contract annotation changes. `dbc-gate` is untouched and stays green. |
| III | R-DCUT | PASS. Research recorded as R-001 through R-008 with decisions, rationale, and rejected alternatives. Three decisions corrected the specification, and each correction is recorded in place of a quiet edit. |
| IV | Documentation | PASS. The constitution gains a Sync Impact Report and four clause edits, `README.md` and `AGENTS.md` gain or lose platform sentences, and the module's diagnostics become accurate. No content-free comment is added, and one comment is deliberately preserved (research.md R-005). |
| V | Style and formatting | PASS. No C++ file changes, so `format-check` is unaffected. The two prose documents are outside the formatter's glob. |
| VI | Test-Backed Code and Coverage | PASS. No code changes, so the coverage gate's 100 percent line and branch requirements are unaffected and stay green. Every requirement is verified by a command in [quickstart.md](quickstart.md). |
| VII | Performance Discipline | PASS. No code path changes and no measurement moves. The shipped artifact is byte-identical. |
| VIII | CI Quality Gates | PASS with justification. This principle is the reason the feature needs an amendment at all: its hard gate list enumerates three platforms and the project can test one. The gate set's content is unchanged; only the platform enumeration narrows, which is the amendment, and the amendment carries the written rationale Governance requires. The `test-rocky` job is a second Linux distribution and stays. |
| IX | Spec-driven development | PASS. spec, plan, tasks, and then the amendment, in that order. Nothing bypasses the workflow, which is why this feature has a specification at all. |
| X | Anti-slop discipline | PASS. No abstraction is introduced for a port that may never land. The change is deletion: seven presets, two prose claims, three diagnostic strings. The one addition, a Sync Impact Report, is required by the amendment pattern the constitution already uses. |
| XI | Prose standards | PASS, verified mechanically. Every artifact in this feature passes `prose-lint` at 0 findings. The gate caught XI.2 findings during Phase 0 and Phase 1. The token ` instead of ` is banned beside ` rather than `, which cost two rewrites before it was noticed. |

**Gate weakening requested**: yes, and this is the amendment. Narrowing
VIII's hard gate list from three platforms to one withdraws an obligation
from a NON-NEGOTIABLE principle, so it is a MAJOR amendment at 2.11.0 and
never a quiet edit. Governance ranks the gate set as governance and requires a
PR with written rationale and a version bump; research.md R-006 records that
classification and the reasoning behind it. The gate list's content is
identical, the amendment is the record, and the 2.7.0 and 2.8.0 reports stay
in the file so the history of the suspension and the deferral survives.

### Post-design re-evaluation

| Principle | Change after design | Verdict |
|-----------|--------------------|---------|
| VIII | S2 replaces the sentence reinstating the Windows gate when an upstream port lands, because under the new policy a future specification adds a platform and landing an upstream port is not what restores one | PASS, amendment statement S2 |
| IX | S5 narrows the per-feature release-build clause to `ci-ubuntu`. The obligation of one release-configuration build per feature is unchanged; the enumeration of which preset shrinks to the one that exists. | PASS, amendment statement S5 |
| X | The design phase found the file scope wrong. The first specification limited the tracked diff to four files and declared `cmake/ImportAutotoolsSubmodule.cmake` untouchable, which left three runtime diagnostics asserting macOS support. Correcting it took the footprint to five files and split FR-016 into branches, which stay, and strings, which change. | PASS, correction recorded in research.md R-003 |
| XI | The audit pattern widened from four tokens to thirteen after the sweep showed the narrow set caught none of the three diagnostics. A gate that passes while `brew install autoconf` still ships reports clean on a false negative. | PASS, correction recorded in research.md R-004 |

No violation requires the Complexity Tracking table. The two deviations from
the obvious approach, keeping the platform branches and keeping the merged
specs, are both forced by the owner's stated design goal and are recorded in
[contracts/platform-policy.md](contracts/platform-policy.md) sections C2 and
C3.

## Project Structure

### Documentation (this feature)

```text
specs/010-linux-only/
├── spec.md                          # /speckit.specify output, corrected by Phase 0 research
├── plan.md                          # This file (/speckit.plan output)
├── research.md                      # Phase 0 output
├── quickstart.md                    # Phase 1 output
├── contracts/
│   └── platform-policy.md           # Phase 1 output: the amendment's required statements
├── checklists/
│   └── requirements.md              # /speckit.specify output, re-validated after the corrections
└── tasks.md                         # Phase 2 output (/speckit.tasks; not created here)
```

### Source Code (repository root)

```text
.specify/memory/constitution.md   # amendment 2.11.0: Sync Impact Report, four clause
                                  # edits, lineage row, version and Last Amended
CMakePresets.json                 # seven presets deleted, 15 configure survive
README.md                         # platform statement added; Homebrew line dropped
AGENTS.md                         # Linux presets only; CI matrix line narrowed
cmake/ImportAutotoolsSubmodule.cmake
                                  # 3 diagnostic strings + 1 header comment, no branch
CMakeUserPresets.json             # machine-local, gitignored, zero tracked diff:
                                  # drop any preset inheriting a deleted one (FR-018a)
```

Not touched, deliberately: `CMakeLists.txt` and every other file under
`cmake/` carry per-platform seams and stay byte-identical; `.github/workflows/`
needs no change because the matrix is already Linux-only; `.codespellrc` keeps
its macOS comment because it explains a spelling exemption
(research.md R-005); every file under `specs/003`, `004`, `005`, `006`, and
`009` is a dated record; every C++ source, header, and test is unchanged;
`docs/` gains nothing because `Doxyfile.in` reads `README.md`, so the README
edit reaches the published documentation by itself.

## Complexity Tracking

> **Fill ONLY if Constitution Check has violations that must be justified**

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| VIII's hard gate list is narrowed from three platforms to one | The project builds, tests, and gates Linux only. Leaving macOS and Windows in the gate list makes the project's highest authority claim a coverage it does not have, and that claim is what a reader trusts when deciding whether to file an issue against this platform. | Leaving the gate list alone: the list is where the obligation lives, so a policy decision that stops at the prose leaves governance asserting something untrue. Governance ranks the gate set as governance and requires an amendment, so the alternative is a governance document that contradicts the tree, and not a smaller diff. |
| `cmake/ImportAutotoolsSubmodule.cmake` changes, after the first specification forbade it | Three `FATAL_ERROR` diagnostics tell a developer that Linux and macOS are supported and that `brew install` is the fix for a missing toolchain. A configure-time abort is the last place to leave a false claim, because it is read when the build has already failed. | Leaving the module alone on the reading that preservation outranks accuracy: the branches stay either way, and the strings are separate literals from the branches. Leaving them would leave FR-015 unsatisfied by the one file that most needs to satisfy it. |
| Seven presets deleted while the per-platform branches they selected stay | The presets are a CI claim, and a reader who lists presets concludes the platform is supported. The branches are a seam, and a future port consumes them. The two are different surfaces doing different work, so they get different fates. | Deleting the branches too: that is the foreclosing the owner explicitly ruled out, and it is the one change a later specification cannot undo. Keeping the presets: the gate-shaped surface this feature exists to remove, contradicting the constitution the same change amends. |