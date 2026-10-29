# Analysis round 2: specs/038-report-view-generators

Analyzed 2026-10-29 against the tree at branch
`038-report-view-generators`. Method: the constitution first, then the
four artifacts with `contracts/views-api.md` and `data-model.md`, then
the grounding files (`CMakeLists.txt`, `test/CMakeLists.txt`,
`example/CMakeLists.txt`, `tools/docs/example_drift.py`,
`test/example_run.sh`, `test/snippet-compile/run.sh`,
`include/speedgun-ng/benchmark.hpp`, `tools/prose/prose_rules.yaml`,
`tools/prose/prose_gate.py`). Round 1's ten findings stand verified
remediated; every quoted token below was checked against the
checked-out tree.

## Remediation check (round 1)

- F1 stands fixed: plan.md's physical view and M-A name the
  `speedgun-ng_harness` target and the source-list seam; T001 and T016
  link `speedgun-ng::harness`; the `SOVERSION` record matches the tree
  (archive `4` at `CMakeLists.txt:91`, harness `15` at `:320`).
- F2 stands fixed: T020 names the `SECTIONS`, `SECTION_OWNERS`,
  `NAME_SCAN_HEADERS` and `PAGE_LINE_CAP` moves in
  `tools/docs/example_drift.py`, and M-I gives the marked snippet its
  own `views.hpp` include, the route the fixed prelude of
  `test/snippet-compile/run.sh` (`<cstdint>`, `<vector>`,
  `barrier.hpp`, `benchmark.hpp`) needs.
- F3 stands fixed: T019 and M-I state the example shape
  `test/example_run.sh` drives (`--list` names the benchmark, the run
  carries `--min-time=0.01`, the rows print) and name the document
  route: the run writes its JSON document, then `loadReport`.
- F4 stands fixed: the escaper stands as module M-C in wave 2 beside
  M-B; the page is M-G alone, and the histogram, the plot and the text
  view name it as their escaper source.
- F5 stands fixed: T002 writes the version and `SOVERSION` assertions
  into `views_test` and names that test in its verify.
- F6 stands fixed: the plan's physical view names
  `test/report_view_cli.sh`, the file T017 creates.
- F7 stands fixed: the plan's documentation tree carries one
  `quickstart.md` at the feature root.
- F8 stands fixed: T021 lands `results/run_views.sh` beside the
  records.
- F9 stands fixed: the audit point cites `options.hpp:93` for the
  constant and writes "carrying the value `297`"; the tree confirms the
  constants run `:81` to `:107` and `kOsNoiseOptionValue` stands at
  `:93`.
- F10 stands fixed: FR-019, SC-005 and T023 name the DBC leg beside
  line and branch.

## Findings

### F1. MAJOR: the commit plan titles the views commits with a section token the commit gate does not accept

- Artifact: tasks.md (Notes).

One side, tasks.md Notes:

> the titles following the template (`Views: ...`, `CMake: ...`,
> `Docs: ...`, `Meta: ...`), the `Refs:` footer naming
> `specs/038-report-view-generators`

The other side, `tools/prose/prose_rules.yaml`, the `sections` list the
commit gate reads:

> sections:
>   - CMake
>   - CI
>   - Constitution
>   - Counters
>   - Docs
>   - Harness
>   - Meta
>   - dbc
>   - deploy
>   - runner
>   - test

`tools/prose/prose_gate.py` fails a title whose section token stands
outside that list (`CM.SECTION-UNKNOWN`, "section token is one of the
configured sections", the membership check at `:1133`), and the gate
runs over the pull-request range (constitution VIII). No commit in the
branch's recent history carries `Views` (the section tokens of the last
80 titles: `Docs`, `Harness`, `test`, `Meta`). A lane that titles its
library commits as the plan says fails the commit-message gate on its
first views commit, and the failure repeats per phase.

Remediation: title the library and driver commits with an accepted
section, `Harness:` (the views compile into the `speedgun-ng_harness`
target per D-2), keeping `CMake:`, `Docs:` and `Meta:` where they fit;
or add `Views` to the `sections` list of
`tools/prose/prose_rules.yaml` in the same change, the way amendment
2.6.0 grew `XI5.FILLER` beside the rule it served, and let the prose
gate's fixtures confirm the data move.

## Checks that passed

- Round 1 remediations: all ten stand in the artifacts (the list
  above).
- Coverage: every FR-001 to FR-022, every SC-001 to SC-007 and every
  user story carries covering tasks; the spec's edge cases stand
  covered by the test plan (the one-repetition column through T010 and
  T012, the markup metric label through the T003 fixture and the
  escape and page rows of T006 and T014, the absent identity field
  through T014, the aggregate-alone benchmark through the disclosure
  rows of T008 and T010, the zero-benchmark document through T001's
  error-form row and the driver's benchmark-missing refusal).
- Consistency: the module letters M-A to M-J, the wave numbers, the
  function names (`ViewSeries`, `viewSeries`, `renderHistogram`,
  `renderPlot`, `renderPage`, `renderText`), the driver grammar, the
  refusal and disclosure table, and the decision references D-1 to D-10
  agree across the artifacts; the plan's module count (ten) matches its
  own list.
- Grounding: every cited line and token checks out against the tree:
  `benchmark.hpp` `ReportData` at `:792` (with `results`, `rowRaw`,
  `contextRaw`, `schemaVersion`, `speedgunVersion`), `ResultRow` at
  `:383` (with `kind`, `timePerIterationNs`, `cpuTimePerIterationNs`,
  `timeAvailability`, `metrics`, `timeStats`, `metricStats`,
  `percentiles`), `BenchmarkResult` at `:627` (with `rows`,
  `metricLabels`, `timeUnit`), `RowKind` at `:90`, and `loadReport`
  exported at `:2878`; `CMakeLists.txt:26` version `0.24.0`, `:91`
  archive `SOVERSION 4`, the harness source glob at `:234`, `:320`
  harness `SOVERSION 15`; `json_report.cpp:39` schema `1.8`;
  `options.hpp` constants `:81` to `:107` with `kOsNoiseOptionValue` at
  `:93` carrying `297`, the block `298` to `319` free; the drift gate's
  fixed tables (`SECTIONS` at `:32`, `SECTION_OWNERS` at `:68`,
  `PAGE_LINE_CAP` of `912` at `:214`, `NAME_SCAN_HEADERS` at `:504`);
  the `snippet-compile` prelude; the `example_run` gate's list,
  min-time and row checks; `add_docs_example` and the `.expected`
  precedent in `example/CMakeLists.txt`; the standing `--json` option
  row the example's document route rides; the presets `ci-ubuntu`,
  `ci-sanitize` and `ci-tsan` and the coverage preset exist;
  `harness_cli_table_test` reads the project `VERSION` from the source
  tree, so the version move leaves it green with no edit.
- Constitution: TDD ordering holds per phase (the red task before the
  implementation task that turns it green); the planned identifiers
  pass V.1; the VIII gate list stands represented in wave 8 and T023;
  the DBC, coverage, docs and prose obligations stand named (FR-019,
  FR-020, FR-022); the resolver and the escaper carry their X.2
  justifications.
- Soundness: the dependency order is implementable; the verify commands
  name standing or task-created tests, and the conditional
  registrations (`example_run`, `report_shape`) stand reachable in a
  developer-mode tree, where `BUILD_EXAMPLES` defaults on; the
  contracts A-1 to A-6 and B-1 to B-6 match the FRs they cite, and the
  exit classification of clarify 1/Q2 stays consistent between FR-015,
  FR-016 and the contract's table C.

## Verdict

Round 1's ten findings stand remediated in the artifacts. One major
finding remains: the commit plan's `Views:` section token fails the
commit gate. The remediation is one word in tasks.md, or one token in
`tools/prose/prose_rules.yaml`; no other artifact moves.
