# Analysis round 4: specs/038-report-view-generators

Analyzed 2026-10-29 against the tree at branch
`038-report-view-generators`. Method: round 3's two remediation points
re-verified first, then one final consistency sweep over the five
artifacts (spec.md, plan.md, tasks.md,
contracts/views-api.md, data-model.md) against the tree: the module
letters and waves, the function signatures and the render path, the
contracts against the FRs, the task coverage, the TDD ordering, and
the grounding of the claims the remediations turned on. Rounds 1-3's
settled decisions, plan-level details, the Clarifications and the lane
rulings stand out of scope.

## Remediation check (round 3)

Round 3's two findings stand remediated in the artifacts:

- F1: the contracts' escaper pointer stands at M-C. A-6 now reads
  "the escaper of M-C"; no other artifact carries the retired M-F
  letter for the escaper (read this round across all five).
- F2: the metric unit-label ruling stands carried and the risk's
  pointer resolves. data-model.md's column-resolution table reads
  "as stored; `unit` stands empty" for a metric label, and the
  paragraph below it rules `ViewSeries::unit` empty for a metric
  column; plan.md's risk line cites the ruling ("the data-model rules
  it empty") and names `BenchmarkResult::metricMethod` for the
  method. The grounding re-checked: `MetricValue`
  (`include/speedgun-ng/benchmark.hpp`) carries `value`,
  `runningRatio`, `scaled`, `availability` and `pass` and no method;
  `metricMethod` stands on `BenchmarkResult`, as the remediated lines
  state.

## Findings

### F1. MINOR: the plan's logical-view diagram shows the three renderers taking the resolved `ViewSeries`, the signatures the normative artifacts retired

- Artifact: plan.md (Logical view: the render path).

The diagram reads:

> renderHistogram(series, binCount) -- SVG text
> renderPlot(series)                -- SVG text
> renderText(series)                -- terminal text

The normative signatures disagree in three places: the plan's own
"Public API surface added" names `renderHistogram(const ReportData&,
benchmark, column, binCount)`, `renderPlot(const ReportData&, ...)`
and `renderText(const ReportData&, ..., binCount)`; contracts
B-2 to B-4 state the same; data-model.md records "the arguments stay
plain, and the resolution stands in `viewSeries`". The design's
renderers take the loaded report and resolve through `viewSeries`
themselves (the Complexity Tracking justifies the resolver on exactly
this: one function serving the four renderers and the driver's
pre-check). The diagram shows the pre-resolution flow instead: one
`viewSeries` pass feeding the renderers the series. The same
diagram's `renderPage(report, benchmark, column, binCount)` line
carries the normative signature, so the diagram disagrees with itself
besides the other artifacts. An implementer sketching from the
diagram writes `renderHistogram(ViewSeries, std::size_t)` and meets
the contracts only at B-2.

Remediation: the three diagram lines read
`renderHistogram(report, benchmark, column, binCount)`,
`renderPlot(report, benchmark, column)`,
`renderText(report, benchmark, column, binCount)` (or the renderers'
lines drop the parameter list and the flow notes the internal
`viewSeries` call, the way the `renderPage` line's composition note
does). No normative artifact moves.

## Checks that passed

- Round remediations: round 3's F1 and F2 stand in the artifacts as
  described above; rounds 1 and 2's remediations spot-re-read green
  (the `speedgun-ng::harness` link lines of T001 and T016, the M-C
  escaper in the plan's module list and risk, the T020 drift-table
  moves, the T002 version and `SOVERSION` assertions, the
  `test/report_view_cli.sh` path, the `results/run_views.sh` path,
  the `options.hpp:93` value `297`, the one feature-root
  `quickstart.md`).
- Signatures and names elsewhere: the six public names
  (`ViewSeries`, `viewSeries`, `renderHistogram`, `renderPlot`,
  `renderPage`, `renderText`) and their parameter shapes agree
  between the plan's API surface, contracts B-1 to B-6, data-model.md
  and the tasks that touch them; F1 above names the one exception.
- Contracts vs FRs: A-1 to A-6 and B-1 to B-6 cite and match the FRs
  they name; the table C rows match FR-015, FR-016 and clarify 1/Q2;
  B-1's empty-values value form composes with B-2's and B-3's
  disclosures and the data-model ruling; the metric `unit`-empty
  ruling stands consistent across A-5, B-1, the data-model table and
  the M-B test plan.
- Coverage and order: every FR-001 to FR-022 and SC-001 to SC-007
  carries covering tasks; every wave stands its red task before the
  green task; the module letters M-A to M-J and the wave numbers
  agree between the plan's module list, the phases and the dependency
  section; the plan's module count (ten) matches its list.
- Gates and commit titles: wave 8 and T023 name the coverage read at
  100.0 lines, branches and DBC, the `ci-sanitize` and `ci-tsan`
  presets, the `ci-ubuntu` analyzer build, the prose gate and the
  `taskset -c 0,1` run; the commit sections tasks.md Notes names
  (`Harness`, `CMake`, `Docs`, `Meta`) all stand in the `sections`
  list of `tools/prose/prose_rules.yaml` (re-read this round).
- Grounding of the remediation points: `MetricValue` carries no
  method; `BenchmarkResult::metricMethod` stands on the result
  record; the prose `sections` list carries the four commit
  sections.

## Verdict

Round 3 stands remediated; the tree grounds the artifacts. One MINOR
finding stands: the plan's logical-view diagram draws the three
renderers taking the resolved series, the flow the normative
signatures retired, while its own `renderPage` line carries the
normative signature. No blocker, no major, no gate at risk.
