# Analysis round 3: specs/038-report-view-generators

Analyzed 2026-10-29 against the tree at branch
`038-report-view-generators`. Method: the constitution first, then the
five artifacts (spec.md, plan.md, tasks.md,
contracts/views-api.md, data-model.md) with rounds 1 and 2 standing
recorded. Every remediation of rounds 1 and 2 was re-verified against
the checked-out tree, then one final consistency pass ran over the
coverage of FR-001 to FR-022, SC-001 to SC-007 and the user stories by
tasks, the module letters and wave numbers, the contracts against the
FRs, the TDD ordering, the named gates, and the commit-title sections
of tasks.md Notes against the `sections` list of
`tools/prose/prose_rules.yaml`.

## Remediation check (rounds 1 and 2)

Round 1's ten findings stand remediated in the artifacts: the hosting
decision names `speedgun-ng_harness` with the source-list seam and the
link lines of T001 and T016 read `speedgun-ng::harness` (F1); T020
carries the `SECTIONS`, `SECTION_OWNERS`, `NAME_SCAN_HEADERS` and
`PAGE_LINE_CAP` moves and the marked snippet its own `views.hpp`
include (F2); T019 and M-I state the `example_run` shape and the
document route (F3); the escaper stands as M-C in wave 2 (F4, but see
finding F1 below for the contracts file); T002 writes the version and
`SOVERSION` assertions into `views_test` and names the test (F5); the
physical view names `test/report_view_cli.sh` (F6); one feature-root
`quickstart.md` stands in the documentation tree (F7); T021 lands
`results/run_views.sh` beside the records (F8); the audit point cites
`options.hpp:93` carrying the value `297` (F9); FR-019, SC-005 and
T023 name the DBC leg (F10). Round 2's F1 stands remediated: the
commit plan's sections are `Harness:`, `CMake:`, `Docs:` and `Meta:`,
every one in the `sections` list of `tools/prose/prose_rules.yaml`
(read this round, lines 19 to 29).

## Findings

### F1. MINOR: the contracts file still names the escaper's home as M-F, the letter the round-1 F4 remediation retired

- Artifact: contracts/views-api.md (A-6).

One side, contracts/views-api.md A-6:

> A-6: A document-derived string stands escaped where the format can
> read it as markup (D-8, the escaper of M-F).

The other side, plan.md: the escaper is module M-C
(`source/views/escape.hpp`, wave 2); M-F is the text view
(`source/views/text_view.cpp`), whose description names no escaping
and whose users of the escaper are M-D and M-E (the SVG label text)
and M-G (the page's text). Round 1's F4 moved the escaper out of the
then-page module and round 2 verified the plan and tasks; the
contracts file kept the pre-remediation letter. An implementer
following A-6 to M-F finds the text view; the escaper the
contracts pin stands elsewhere.

Remediation: A-6 reads "the escaper of M-C". One token.

### F2. MINOR: the plan's metric-unit-label risk cites a data-model decision the data-model does not carry, and names the wrong home for the metric method

- Artifact: plan.md (Risks), data-model.md.

One side, plan.md Risks:

> The metric column's unit label: `MetricValue` carries the value
> and the method; the label text the figures print needs the plan's
> data-model decision, and the tests pin it (M-B, data-model.md).

The other side: `MetricValue` (`include/speedgun-ng/benchmark.hpp:238`)
carries `value`, `runningRatio`, `scaled`, `availability` and `pass` —
no method; the settled method stands at `BenchmarkResult::metricMethod`
(`benchmark.hpp:684`). And data-model.md decides no label text: the
column-resolution table's Unit cell for a metric label reads "as
stored" (how the values print, FR-006, A-5), and `ViewSeries::unit` is
"the label the values print beside" with no stated content for a
metric column. B-1's `\post` ("`unit` names the unit the values print
in") therefore leaves the metric case to the implementer's guess:
empty, or the metric label. The plan's own risk names data-model.md as
the decider, so the pointer dangles.

Remediation: data-model.md states the metric column's `unit` (empty:
the metric column prints as stored and names no unit; or the label,
one ruling), and the risk line names `BenchmarkResult::metricMethod`
instead of `MetricValue` for the method.

## Checks that passed

- Round remediations: all ten of round 1 and the one of round 2 stand
  in the artifacts (the list above).
- Coverage: every FR-001 to FR-022 carries covering tasks (FR-001
  T001/T002/T016/T024, FR-002 T002, FR-003 T024, FR-004/005
  T001/T004/T005, FR-006 T003/T004/T005, FR-007/008 T008/T009,
  FR-009/010 T010/T011, FR-011/012 T014/T015, FR-013 T012/T013,
  FR-014 to FR-017 T016 to T018, FR-018 T003/T023, FR-019 T023,
  FR-020 T005/T024, FR-021 T021/T022, FR-022 T019/T020); every
  SC-001 to SC-007 stands covered (SC-002 T021/T022, SC-003
  T017/T018, SC-004 T018/T024, SC-005 T023, SC-006 T023, SC-007
  T019/T020/T023); every user story stands covered by its module's
  tasks, the acceptance scenarios included (the one-repetition and
  aggregate-alone disclosures through T008/T010/T012, the markup
  label through the T003 fixture and T006/T014, the absent identity
  field through T014, the driver's five rows through T017).
- Consistency: the module letters M-A to M-J and the wave numbers
  agree between plan.md's module list and tasks.md's phases and
  dependency section; the function names (`ViewSeries`,
  `viewSeries`, `renderHistogram`, `renderPlot`, `renderPage`,
  `renderText`), the driver grammar, the refusal and disclosure
  table, and the decision references D-1 to D-10 and the clarify
  rulings agree across the five artifacts; the plan's module count
  (ten) matches its list.
- Contracts vs FRs: A-1 to A-6 and B-1 to B-6 cite and match the FRs
  they name; table C agrees with FR-015 and FR-016 and the clarify
  1/Q2 classification; B-1's empty-series value form, B-3's
  empty-series error form and clarify 2/Q2 agree; B-4 and B-5 carry
  the per-part composition rule of clarify 2/Q1 and FR-012/FR-013.
- TDD ordering: every wave stands its red task before the
  implementation task that turns it green (T001 before T002; T004
  before T005; T006 before T007; T008/T010/T012 before
  T009/T011/T013; T014 before T015; T017 before T018).
- Gates: wave 8 and T023 name the coverage read at 100.0 lines,
  branches and DBC, the `ci-sanitize` and `ci-tsan` presets, the
  `ci-ubuntu` analyzer build, the prose gate and the `taskset -c 0,1`
  run; the presets and `harness_cli_table_test` stand in the tree.
- Commit titles: the sections tasks.md Notes names (`Harness`,
  `CMake`, `Docs`, `Meta`) all stand in the `sections` list of
  `tools/prose/prose_rules.yaml`.
- Grounding: every cited line and token re-checked against the tree:
  `CMakeLists.txt:26` version `0.24.0`, `:91` archive `SOVERSION 4`,
  `:234` the harness source `file(GLOB_RECURSE ...)`, `:320` harness
  `SOVERSION 15`; `json_report.cpp:39` schema `1.8`;
  `options.hpp` constants `:81` to `:107` with `kOsNoiseOptionValue`
  at `:93` carrying `297`, the block `298` to `319` free;
  `benchmark.hpp` `RowKind` `:90`, `ResultRow` `:383` (with
  `timePerIterationNs`, `cpuTimePerIterationNs`,
  `timeAvailability`, `metrics`, `timeStats`, `metricStats`,
  `percentiles`), `BenchmarkResult` `:627` (with `rows`,
  `metricLabels`, `timeUnit`), `ReportData` `:792` (with `results`,
  `rowRaw`, `contextRaw`, `schemaVersion`, `speedgunVersion`), and
  `loadReport` exported at `:2878`; `Availability` carries
  `COUNTABLE` beside its gap states (A-4's rule reads on the enum);
  the drift gate's `SECTIONS` at `:32`, `SECTION_OWNERS` at `:68`,
  `NAME_SCAN_HEADERS` at `:504`, `PAGE_LINE_CAP` `912` at `:214`;
  the `example_run` gate's `--list`, `--min-time=0.01` and row
  checks; `add_docs_example` and the `.expected` precedent; the
  `snippet-compile` prelude (`<cstdint>`, `<vector>`, `barrier.hpp`,
  `benchmark.hpp`); `test/time_source_gate.sh` standing registered.

## Verdict

Rounds 1 and 2 stand remediated; the tree grounds the artifacts. Two
MINOR findings stand: one stale module letter in the contracts file
that the round-1 escaper move left behind, and one dangling pointer in
the plan's risk to a data-model decision that data-model.md does not
carry. No blocker, no major, no gate at risk.
