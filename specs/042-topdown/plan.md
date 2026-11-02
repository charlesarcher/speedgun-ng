# Implementation Plan: Vendor-Neutral Top-Down Analysis

**Branch**: `042-topdown` | **Date**: 2026-11-01 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `specs/042-topdown/spec.md`

## Summary

The counters library selects the vendor event table for a CPU identity, groups
the table's top-down rows into a tree by level token and parent group, and
answers the two host facts those rows reach. The harness adds
`--topdown[=<level>]`, collects the tree's nodes inside the pass plan the run
already builds, prints one block per benchmark, writes one report key under a
raised schema minor, and accepts a `topdown:` regression rule.

Three defects block a live value on the acceptance host and ride in this
change as their own commits, each with a test that fails at the base hash: the
fast-read bound that refuses the kernel's fixed-counter index, the SMT state
that no reader answers, and a PMU provider rebuilt on every call into the
harness entry point.

Nothing in this plan evaluates a threshold. The library's formula parser
refuses all 102 threshold texts of the selected table, so the text rides to the
console unchanged and no part marks a node.

## Technical Context

**Language/Version**: C++23 (`cxx_std_23` at `CMakeLists.txt:127` and `:380`)

**Primary Dependencies**: none added. The vendor tables are the vendored tree
under `external/pmu-events`, embedded at build time by
`cmake/embed_pmu_blob.cmake`. The JSON writer, the report reader and the rule
comparator are the harness's own code.

**Storage**: files read at build time only. No run-time read of the vendored
tree (D-12, PC-12).

**Testing**: frameworkless C++ programs with the `check()` and `fail()`
convention, one executable per file, driven by CTest, plus the Python gate in
`tools/docs/example_drift.py`, the prose gate in `cmake/prose-lint.cmake`, the
DBC coverage gate in `tools/dbc/coverage_gate.sh`, and the tier-label audit in
`test/tier_labels.sh`.

**Target Platform**: Linux on x86-64, the two release presets `ci-ubuntu`
(GCC and Clang, clang-tidy, cppcheck) and `ci-rocky`. macOS and Windows stay
unsupported.

**Project Type**: two shared libraries with one CLI front end. The counters
archive answers facts about the host; the harness turns a run into a result, a
console block and a report.

**Performance Goals**: a run without the option pays nothing: the selection and
the tree builder sit behind the option, and no new pass enters the plan. A run
with the option adds nodes to the pass set the planner already sizes, and the
selection reads the embedded table once per benchmark.

**Constraints**: 100 percent line and branch coverage with the branch allowance
held at 21 (`tools/dbc/coverage_gate.sh:102`) and no new exclusion marker; the
README page cap of 1004 lines
(`tools/docs/example_drift.py:240`); a bad option value answers one quoted line
and exit status 2; no CI file changes; no change to `counter_limits.cpp` or any
limit code (FR-022).

**Scale/Scope**: one new public header, one new counters translation unit,
twelve changed harness or counters files, two changed CMake files, one schema,
two documentation pages, two example programs including the new memory-bound
case of FR-058, and five new test executables.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Status at plan time | Evidence in this plan |
| --- | --- | --- |
| I, standard-first P0 to P3 | pass | Vendor spellings come from the table file that governs (FR-004). The level counts state the file's answer, 4 and 8, and the request's 8 and 16 match no directory. |
| II, Design By Contract | pass | Every new public type carries `\pre`, `\post` and `\invariant` in its header comment, and the contracts directory states each one. |
| III, R-DCUT | pass | Each new unit names the requirement it proves. The three carried repairs each start from a test that fails at `d454bcff`. |
| IV, documentation | pass | The README section pays for its lines with named trims, the guide page carries the detail, and the example is a run gate (`FR-044` to `FR-046`). |
| V, style | pass | The change adds no new lint suppression. clang-tidy and cppcheck run in `ci-ubuntu`. |
| VI, coverage | pass | Five new executables join the coverage tree, the allowance stays 21, and no exclusion marker is added. |
| VII, performance discipline | pass | The option is inert when absent. The tree joins the existing pass plan and adds no pass. |
| VIII, hard CI gates | pass | The plan adds no CI file, keeps the matrix commands, and lists the gate order in `quickstart.md`. |
| IX, spec-driven | pass | This plan follows `spec.md` and names the decision for every design point. |
| X, anti-slop | pass | No fallback stands where a refusal is honest: no width is invented for `#core_wide`, no threshold is evaluated, and no node is invented for a level the table lacks. |
| XI, prose | pass | `cmake/prose-lint.cmake` reports 0 findings over this directory. |

Re-check after Phase 1 design: passed. The contracts add no gate, and the
data model adds no exported type beyond the two carriers the version plan
prices.

## Project Structure

### Documentation (this feature)

```text
specs/042-topdown/
├── spec.md                    # Requirements FR-001 to FR-058, SC-01 to SC-13
├── plan.md                    # This file
├── research.md                # Decision, rationale and alternatives per point
├── data-model.md              # Carriers, fields, validation, ownership
├── quickstart.md              # Build, run, acceptance and gate order
├── contracts/
│   ├── topdown-selection.md   # The public selection surface
│   ├── topdown-option.md      # The option, its values and refusals
│   ├── topdown-report-key.md  # The report key, schema order and version
│   ├── topdown-rule.md        # The `topdown:` rule prefix
│   └── topdown-page.md        # The README section and example pair
├── checklists/
│   └── requirements.md        # Requirements quality checklist, passed
└── tasks.md                   # Phase 2 output, not written by this file
```

### Source Code (repository root)

```text
include/speedgun-ng/
├── counters_topdown.hpp           # new: identity, row, source, node, tree
├── counters_metrics.hpp           # threshold on the two public carriers
└── benchmark.hpp                  # the record member on BenchmarkResult

source/counters/
├── detail/pmu.hpp                 # mark constant, entry fields, identity
└── linux_pmu/
    ├── topdown_select.cpp         # new: selection, tree, refusals, identity
    ├── table_parse.cpp            # threshold text and owning file per entry
    ├── metric_formula.{hpp,cpp}   # threshold carrier, the #core_wide branch
    ├── metric_tables.hpp          # core-wide flag, SMT sibling fallback
    ├── embedded_tables.hpp        # file hashes and the reference string
    └── fast_read.cpp              # the fixed-counter bound (FR-053)

source/harness/
├── detail/{options.hpp,internal.hpp}  # option value, RunOptions, helpers
├── options.cpp                    # the row, the scan arm, the level reader
├── runner.cpp                     # node columns, one plan, the strip step
├── report.cpp                     # the block printer and its hook
├── json_report.cpp                # the key and the raised minor
├── json_load.cpp                  # the reader
├── compare.cpp                    # the `topdown:` prefix
└── cli.cpp                        # the unsupported line, one provider (FR-055)

cmake/{embed_pmu_blob.cmake,EmbedPmuEvents.cmake}   # hash and reference in
CMakeLists.txt                                     # version, two SOVERSIONs
docs/schema/report-v1.json                         # x-order and one property
docs/pages/harness.md                              # the guide section
README.md                                          # one section, 24 lines
example/docs/topdown.cpp                           # the documented example
example/                                           # one memory-bound benchmark
tools/docs/example_drift.py                        # the new header's name scan

test/
├── CMakeLists.txt                 # five blocks in the standing six-line shape
├── tier_labels.sh                 # the skip rows the new counters tests need
├── source/detail/topdown_fixture.hpp              # new: synthetic tables
├── source/counters_topdown_select_test.cpp        # new
├── source/counters_topdown_source_test.cpp        # new
├── source/harness_topdown_option_test.cpp         # new
├── source/harness_topdown_run_test.cpp            # new
├── source/harness_topdown_report_test.cpp         # new: block, key, schema
└── source/counters_linux_pmu_seam_test.cpp        # the bound's seam checks
```

**Structure Decision**: selection, the tree and the two host facts belong to the
counters library, because that library already owns the table parse, the
formula resolution and the host facts. The option, the pass plan, the console
block, the report key and the rule prefix belong to the harness, because those
are the run spine and the report. The two halves meet at one new public header,
`include/speedgun-ng/counters_topdown.hpp`, which the harness includes directly
the way `source/harness/catalog.cpp` includes the metric header today.

**Deliberate boundaries**: no edit to `source/counters/linux_pmu/provider.cpp`,
`counter_limits.cpp`, any limit code, or any file under `.github/`
(FR-022, PC-10, D-16). The one exception inside those paths is named by FR-053,
FR-054 and FR-055, which carry the three repairs, and each repair sits in its
own commit with its test first.

## Complexity Tracking

No violations. The change adds no library, no abstraction layer, no new
dependency and no configuration knob beyond the one option the request names.
The one place a simpler shape was available and refused is recorded in
`research.md`: a bare presence flag for the option, which the standing
option-table contract refuses.
