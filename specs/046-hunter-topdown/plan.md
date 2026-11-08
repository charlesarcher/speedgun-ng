# Implementation Plan: Hunter Top-Down Area

**Branch**: `046-hunter-topdown` | **Date**: 2026-11-03 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/046-hunter-topdown/spec.md`

**Mode**: Test-driven. FR-029 puts each new unit in the same commit as the code it
covers, and each unit reads red at `b1dd7670` before that code lands.

## Summary

Hunter mode ranks catalog addresses. This feature names the top-down area of each
ranked event: the deepest node of the running host's `N10` tree whose formula uses
the event, else the vendored list file that holds the event, else `unmapped`. One
`area=<path>` cell joins the ranked row, one `topdownArea` key joins each event of
the report document, one read-only counters accessor publishes the vendor grouping,
and the same run prints its level-1 top-down block after the ranked rows. No counter
path, no formula grammar and no new option appear (D-1, D-2).

## Technical Context

**Language/Version**: C++23, the standard the tree pins (`CMakeLists.txt`)

**Primary Dependencies**: the counters library through `include/speedgun-ng/counters_topdown.hpp`,
the vendored `external/pmu-events` tables, `hwloc` for the host topology, and the
`N10` selection path of specs/042

**Storage**: the report document (`docs/schema/report-v1.json`), two committed
console and document fixtures under `test/data/`, and the docs pages

**Testing**: frameworkless units with their own `main` and a scenario table, driven
by `ctest` tiers, plus the gate scripts `tools/ci/prepush.sh`,
`tools/docs/example_drift.py`, `tools/prose/prose_gate.py` and the two DBC gates

**Target Platform**: Linux only. `ci-rocky` (GCC 16.2.1) and `ci-ubuntu` (clang with
clang-tidy and cppcheck) are the checked-in release presets. macOS and Windows stay
unsupported.

**Project Type**: library plus a benchmark harness binary, with a generated docs page
set

**Performance Goals**: the area adds no measurement and no input/output. The second
`topdownTree` ask of a process answers in 0.57 ms from the read the run already pays
for (AP-12), and no hunt rank, budget or emitted byte moves (FR-010).

**Constraints**: 100 percent line coverage with the branch shortfall at 21 or lower
(`tools/dbc/coverage_gate.sh:102`), no new exclusion marker, no clock read or sleep in
a new unit, no invented cpu device, and no `perf_event_open` from a new unit
(FR-030, FR-031).

**Scale/Scope**: two new harness files, one new counters accessor in one existing
file, one public header pair, six harness source files touched, one schema, one
example, two docs pages, and four version bindings (FR-028).

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Gate (Principle VIII) | Reading for this change | Status |
|---|---|---|
| Preset builds, Linux, GCC and Clang | `ci-rocky` and `ci-ubuntu` build this tree at the base with no finding | design passes, measured at base |
| All tests pass | the base answers three failures, all in files the 045 lane owns (AP-1), so SC-005 is a base-to-head name diff | design passes |
| ASan/UBSan clean (`ci-sanitize`) | the mapper reads three in-memory records and opens nothing, so no new path reaches a sanitizer fault | design passes |
| TSan clean (`ci-tsan`) | the companion pass runs on the run's own thread through the runner that `N10` already drives | design passes, measured by `ci-sanitize` and the `tsan` tier of `tools/ci/prepush.sh --full` |
| clang-tidy and cppcheck | CI fails a finding that names a check in `WarningsAsErrors`, and the constitution also asks for no new finding of any check, so the new pair is written to the baseline: designated initializers, no magic number in a comparison, trailing commas, no varargs, no raw pointer arithmetic | design passes, delta measured |
| `readability-identifier-naming` zero | the base answers 0 of that check, and the new names follow the pinned cases | design passes |
| `format-check` and `spell-check` | `cmake -P cmake/lint.cmake` and `cmake -P cmake/spell.cmake` answer 0 at the base | design passes |
| Prose (XI) | the base tree answers 0 findings over 966 sources, and the branch point range is a void run, so every prose claim names its form (AP-2b) | design passes |
| Time-source clean | the mapper reads no clock, so `test/time_source_gate.sh` stays green | design passes |
| Page-drift clean | the new example block rides `docs/pages/harness.md`, where no cap reaches it, and `README.md` gains one quick-reference row at 1001 of 1004 lines; `SECTION_LINE_CAPS` stands unchanged (FR-025, FR-026) | design passes, measured in a copy of the tree |
| Coverage (VI) | the mapper is pure, so every arm reaches a unit case, and the two new files carry no exclusion marker | design passes |
| Docs shape gate | `tools/docs/report_shape.py` gains no fold rule. FR-027 rules that no recorded region holds a real level-1 block, because that block carries a table path, a kernel reference and digests whose count the table owns. The example registers its own recorded region as its machine (FR-025, FR-027) | design passes, no tool work in scope |
| Performance (VII) | one extra tree ask costs 0.57 ms and one short level-1 pass joins the run beside the stages (AP-12, D-9) | design passes |
| Gate configuration | no cap, allowance, omit pattern or rule file moves | design passes |

Design By Contract (II) and R-DCUT (III) hold by shape: the mapper is a pure function
over three inputs, its preconditions and postconditions appear in
`contracts/area-mapper.md`, and the grouping accessor is a read of build-time data.
Anti-slop (X) holds: the mapper adds no abstraction above the tree it reads, and the
one new public function replaces no existing path. Principle XI holds in every line
this plan writes.

**One scope amendment, an addition the request implies.** The request allowed read-only use of
`include/speedgun-ng/counters_topdown.hpp`, and D-16 adds one public read-only
accessor there. The cost is one declaration, one definition, and archive SOVERSION 5
to 6. The refused alternative was a second vendored-json reader inside the harness,
which would parse the same vendor data a second time. The Scope section of the spec
carries the amendment, and the pull request body names it.

## Project Structure

### Documentation (this feature)

```text
specs/046-hunter-topdown/
├── plan.md                    # this file
├── research.md                # Phase 0: decisions D-1 to D-17 with their evidence
├── data-model.md              # Phase 1: records, values, index, document shape
├── quickstart.md              # Phase 1: clone to a screen that shows the cell
├── contracts/
│   ├── area-mapper.md         # the pure function and its unit cases
│   ├── counters-grouping.md   # the accessor, its doc form, its arms
│   ├── console-cell.md        # the cell, the one-block rule, the bytes that stand
│   └── document-key.md        # the key, the reader rule, the specs/047 rebase point
├── checklists/requirements.md # requirements quality pass
└── tasks.md                   # Phase 2 (/speckit.tasks)
```

### Source Code (repository root)

```text
include/speedgun-ng/
├── hunt.hpp                        # RankedEvent gains topdownArea after delta
└── counters_topdown.hpp            # TopdownEventGroup and topdownEventGroups

source/harness/
├── hunt_topdown.cpp                # NEW: the mapper, the index, the stamp
├── detail/hunt_topdown.hpp         # NEW: internal declarations, no installed header
├── cli.cpp                         # stamp pass, refusal line, one-block rule
├── report.cpp                      # area cell, companion print, suppressed call
├── json_report.cpp                 # kSchemaVersion, the written key
├── json_load.cpp                   # the key read (specs/047 rebase point)
└── detail/internal.hpp             # RunOptions seam for the planted identity

source/counters/linux_pmu/
└── topdown_select.cpp              # the grouping accessor, one function

test/source/
├── harness_hunt_topdown_test.cpp   # NEW: mapper, index, stamp, arms
├── counters_topdown_select_test.cpp# grouping accessor cases
├── harness_hunt_report_test.cpp    # cell bytes, one-block rule
├── harness_hunt_json_test.cpp      # key round trip, older document, version pins
└── data/hunt_no_hunt_*             # two fixtures that change by zero lines

example/docs/
├── hunt_topdown.cpp                # NEW: the area example region
└── hunt_ranks.cpp                  # unchanged, re-homed block on harness.md

docs/
├── schema/report-v1.json           # 1.14, x-order and properties, not required
└── pages/harness.md                # hunter subsections, the new example block

tools/docs/
├── report_shape.py                 # the fold rule for the level-1 block
└── (fixtures)                      # the planted pair the rule needs

CMakeLists.txt                      # version 0.31.0, SOVERSION 21 and 6
README.md                           # one quick-reference row, no new block
```

**Structure Decision**: the mapper lives in the harness, in a new pair under
`source/harness/`, because the area is a reading of a report record and no other
component reads one. The level-1 companion keeps its own internal option record, so
no line of the process option record moves (FR-035). The vendor grouping comes from the counters library, which
already parses the pinned tables, and reaches the harness through one read-only
function (D-16). The `detail` directory keeps the mapper out of the interface the DBC
gates count, so the count moves by two interfaces and not by more. No new target,
directory or dependency appears.

## Complexity Tracking

No constitution gate needs a justification to pass, so this table stays empty. Three
items a reader may mistake for a violation are recorded above: the D-16 scope
amendment, the docs-page home that avoids a cap change, and the level-1 block that
stands outside every recorded region, which is why the shape tool gains no rule.


---

## Acceptance plan: Intel final acceptance (2026-11-07) - the hunter part

The shared plan, the command table and the risk list stand in
`specs/048-hwlat-probe/plan.md`. This part runs two commands, three times each, on a
tree built at the measured head, and writes one record.

- **Commands**: `./build/example/memory_walk --hunt --hunt-budget 120s` and
  `./build/example/first_benchmark --hunt --hunt-budget 120s`, the pair that
  `results/acceptance-hunt.md` records. No `--timeout`, and no method word in the
  normal runs.
- **What each run leaves**: the console text, the exit code, the `events=` summary
  line, the plan line with its method word, every `area=` cell, the `topdown:` block
  with its `level=1` line and its file lines, and the count of lines the run printed.
- **What the record may say**: the three counts over the printed rows with their
  arithmetic, the distinct cell texts with a count each, the table the block names with
  the digests it prints, and the mapping of each figure to SC-007, SC-010 and AP-9.
- **What it may not say**: that the run read a level no line prints, that a rank order
  holds across runs, that an AMD, Arm or hybrid figure stands for an Intel reading, or
  that a base re-run happened. A base build stands out of scope and the record names it
  as not run.
- **Failure route**: where the normal method fails, the record carries the failure with
  its exit code before any separately labeled alternative.
