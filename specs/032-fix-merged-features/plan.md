# Implementation Plan: Fix Merged Features

**Branch**: `032-fix-merged-features` | **Date**: 2026-10-27 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `specs/032-fix-merged-features/spec.md`

## Summary

The branch closes every open item of the three pull requests that merged
unfinished: PR #46 (spec 027, the jitter probe), PR #49 (spec 028, the
command-line follow-up) and PR #47 (spec 029, the hunter core). It has
three kinds of work. The check items (group B) confirm against master the
fixes earlier lanes already landed and record the file:line evidence. The
defect items (groups C to E) fix what master still carries: one
precedence for the probe value, the affinity-mask fallback of
`onlineCpuSet()`, the compare-table quoting, one top-level `x-order` key
in the report schema, the re-recorded hunt samples, the coverage
keep-set review, and the PR 47 review items. The process items (group F)
bind the commit footer, the prose gates, the push shape and the one
pull-request summary comment. The end state is one branch whose every CI
job passes, with coverage at 100 percent for lines and for branches.

## Technical Context

**Language/Version**: C++23 (`cxx_std_23` on every target; the
constitution pins the language at 2.3.0 of its lineage). CMake 3.20+.

**Primary Dependencies**: no dependency is added. The change touches the
existing harness surface: `source/harness/` (`cli.cpp`, `compare.cpp`,
`topology.cpp`, `execution_context.cpp`, `jitter_probe.cpp`,
`hunt*.cpp`, `options.cpp`, `json_report.cpp`), the public header
`include/speedgun-ng/hunt.hpp` (the one candidate edit, a new
`SkippedKind` value), the vendored trees under `external/` and
`tools/` as they stand, and the coverage machinery
(`cmake/coverage.cmake`, `tools/dbc/coverage_gate.sh`) inside the
boundaries the spec fixes (FR-025, FR-026, FR-031, FR-059).

**Storage**: N/A: no file and no database. The one document the feature
edits is `docs/schema/report-v1.json` (one `x-order` key after FR-017)
and the recorded outputs `docs/pages/harness.md:1509` and
`example/docs/hunt_rank.expected:2` re-record after FR-018.

**Testing**: the repository's own harness (`speedgun-ng_test` pattern,
one executable per area plus `add_test`), driven by `ctest`. The
coverage gate runs `lcov` over the trace and holds the 100 percent line
and branch bar with the allowance of 19 (`tools/dbc/coverage_gate.sh:60`)
and the five `--omit-lines` patterns (`cmake/coverage.cmake:92` to
`:98`). Tests pass under `taskset -c 0,1` and on a real 2-cpu host
(SC-004). No test calls `setenv` or `clearenv` (FR-056); a child
receives a variable through its environment array.

**Target Platform**: Linux only (`ci-ubuntu`, `ci-rocky`). The daily
loop drives `ci-rocky` (`cmake --preset=ci-rocky`, `cmake --build
build`, `ctest --test-dir build --output-on-failure`); once per feature
`ci-ubuntu` builds with clang-tidy and cppcheck wired in. The coverage
job's compiler is the compiler FR-031 names, and the zero list is
re-taken from it (D-13).

**Project Type**: library and command line: the C++ archive surface plus
the harness entry point. No new target, no new option row (FR-003).

**Performance Goals**: no behavior and no measured figure changes
beyond the defect fixes. The hunt runs one pass (D-10, FR-037). No
benchmark regression is claimed, so no number is promised here
(Principle VII).

**Constraints**: the spec's Constraints section binds the change in
full: 100 percent coverage with no exclusion marker and no waiver (the
D-12 keep-set and the D-13 option are the standing exceptions); no code
shaped for coverage (D-14); no sleep, no wall-clock read, no made-up cpu
in a test; exit 2 with one quoted line for every bad value from every
source; no lambda inside an `SG_*` macro (FR-057); `[[maybe_unused]]` on
contract-only helpers (FR-040); a `SOVERSION` rises only for a public
ABI change and the archive `SOVERSION` stays `4` (FR-002); the JSON
libraries the repo imports are the only ones a change uses; rebase only,
push once with `--force-with-lease` (FR-060); the footer of FR-053 on
each commit; `prose-lint` green on each commit (FR-063).

**Scale/Scope**: 16 check items (FR-005 to FR-016), 13 master-defect
items (FR-017 to FR-029), 4 compare and counters items (FR-030 to
FR-033), 17 hunt and probe items (FR-034 to FR-052), 13 process items
(FR-053 to FR-065). Every fix ships with its test, written first and
seen failing, in the same commit (FR-055).

**Execution mode**: TDD (Principle III). Each fix's test is written and
observed failing before the fix that turns it green exists; the
check items carry their confirmation evidence instead.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Gate | Status |
| --- | --- | --- |
| I. Standard-first | C++23 standard surface. The fixes touch parse, precedence and fallback code that the standard library alone covers: `std::optional`, `std::uint64_t`, the affinity mask through the existing seam. No intrinsic header and no compiler extension joins; the `loop_shape` item (FR-020) is reproduced and named, and no compiler check joins | PASS |
| II. Design by contract | Each changed function keeps or gains its `\pre`/`\post`/`\invariant` wording, and the contract facility enforces it at the seams the fix touches. The exit-2 lines are the parse surface's standing refusal shape, not new contract failures | PASS |
| III. R-DCUT | Requirements FR-005 to FR-065 map to the design decisions of the spec (D-01 to D-16), the artifacts of this plan, and `tasks.md` when `/speckit.tasks` lands. Execution mode: TDD, recorded above | PASS |
| IV. Documentation | The two re-recorded hunt samples carry their page regions and pass the drift gate in the same commits (the spec's Constitution Checklist). The docs sweep of FR-018 checks every option, key and exit status of the three features against the code. Each behavior change ships with docs and an example (FR-062) | PASS |
| V. Style | clang-format, clang-tidy, cppcheck clean at the branch head: `ci-ubuntu` builds once per feature with the analyzers wired in, and the format check holds (`d7e22c2` stands on master for the CLI table test) | PASS |
| VI. Coverage | 100 percent line and branch with the standing exceptions only: the D-12 keep-set until the approval it names, and the D-13 sides the coverage compiler proves are exception-cleanup sides. No `LCOV_EXCL` marker, no waiver, no new `--ignore-errors` entry, no code shaped for coverage | PASS |
| VII. Performance | The hunt's one-pass rule (D-10) removes a retry, not a measured path. The precedence fix and the quoting fix stand outside every timed window. No figure moves | PASS |
| VIII. CI gates | Every hard gate runs at the branch head: build, test, sanitizers, tsan, analyzers, coverage, spell, prose-lint, dbc-gate, docs drift, downstream consumer. The one CI-file change the feature may make is the narrow `lcov` option D-13 proves, in `cmake/coverage.cmake` alone, after the approval path D-12 names | PASS |
| IX. Spec-driven | The spec stands at `specs/032-fix-merged-features/spec.md`; this plan and `research.md` stand beside it; `tasks.md` lands with `/speckit.tasks` before code. The change touches behavior (exit codes, precedence) and possibly a public header, so the full workflow applies | PASS |
| X. Anti-slop | Each fix is the narrowest change its item names: one precedence rule, one fallback body, one quoting call at the print site, one schema key, one comment rewrite. No fallback, no shim, no compatibility layer joins. D-14 keeps test witnesses from shaping code | PASS |
| XI. Prose | The spec carries no em-dash, no contrastive framing, no filler vocabulary and no vouching word. This plan and the artifacts beside it hold the same shape; `prose-lint` runs on every commit and the artifacts stand untracked until the branch's first commit the way specs/027 recorded | PASS for the tracked tree, pending for these artifacts |

**Gate weakening requested**: no. **Gate strengthening requested**: no.
The two standing exceptions (the D-12 keep-set, the D-13 narrow filter)
come from the request itself and stand until the approval FR-026
records; no constitution text changes and no gate loses a check.

No gate fails, so **Complexity Tracking stays empty** at the gate.

**Post-design re-check** (after Phase 1): the design artifacts add no
entity and no target (`data-model.md`), name no new dependency and no
new interface beyond the standing contracts they record
(`contracts/cli-options.md`, `contracts/report-json.md`,
`contracts/public-abi.md`), and bind the validation path the gates
run (`quickstart.md`). The one CI-file change the design admits is the
narrow `lcov` option D-13 proves, inside the approval path FR-026
records; the coverage compiler naming of research R-10 moves into the
spec at implementation time (FR-031). The re-measurement of research
R-07 folds the three comment matches into FR-021's commit and opens no
new item. Every gate reads PASS after design, and **Complexity
Tracking stays empty**.

## Project Structure

### Documentation (this feature)

```text
specs/032-fix-merged-features/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output (/speckit.plan command)
├── data-model.md        # Phase 1 output (/speckit.plan command)
├── quickstart.md        # Phase 1 output (/speckit.plan command)
├── contracts/           # Phase 1 output (/speckit.plan command)
│   ├── cli-options.md   # the option, precedence and exit-2 surface
│   ├── report-json.md   # the report schema top-level key order
│   └── public-abi.md    # SkippedKind, the SOVERSION and the seam
└── tasks.md             # Phase 2 output (/speckit.tasks command - NOT created by /speckit.plan)
```

### Source Code (repository root)

```text
source/harness/            # the fixes: cli.cpp, compare.cpp, topology.cpp,
│                          # execution_context.cpp, jitter_probe.cpp,
│                          # options.cpp, json_report.cpp, hunt*.cpp
├── detail/                # options.hpp (no row added, FR-003),
│                          # hunt_selection.hpp (coverage of FR-052)
include/speedgun-ng/       # hunt.hpp (SkippedKind, only when an item needs it, D-09)
test/source/               # the test of each fix, written first (FR-055)
cmake/coverage.cmake       # comment rewrite (FR-025); the one narrow
│                          # lcov option D-13 allows
docs/pages/harness.md      # the docs rows and the re-recorded sample
docs/schema/report-v1.json # one top-level x-order key (FR-017)
example/docs/              # the re-recorded sample output (FR-018)
README.md                  # 835 lines held (D-16), sections at 25 lines
```

**Structure Decision**: no new tree and no new target: the feature edits
the files its items name inside the existing layout, and each commit
touches the files its item names and nothing else (the spec's Principle
X.3 note).

## Complexity Tracking

> **Fill ONLY if Constitution Check has violations that must be justified**

The table stays empty: no gate fails and no violation needs a
justification.
