# Implementation Plan: Report View Generators

**Branch**: `038-report-view-generators` | **Date**: 2026-10-29 |
**Spec**: [spec.md](./spec.md)

**Input**: Feature specification from
`/specs/038-report-view-generators/spec.md`

## Summary

The views give the run's numbers their four shapes: the histogram of
one column's distribution over one benchmark, the plot of one column's
series over its repetition rows, the static HTML page that carries
both with the run's identity, and the text view the terminal reads.
The library renders from the value `loadReport` answers; one thin
driver under `tools/` names the report, the benchmark, the metric and
the view. The report stands unchanged at schema `1.8`; the formats are
hand-rolled; no dependency joins.

## Technical Context

**Language/Version**: C++23, `CMAKE_CXX_EXTENSIONS=OFF`, CMake >= 3.20
preset-driven.

**Primary Dependencies**: the standing set only: the `speedgun-ng`
library and its vendored readers. The views add none (R-3, D-3).

**Storage**: none; the views read the loaded document value and answer
strings.

**Testing**: CTest over `test/` executables, gcov/lcov coverage, the
sanitizer and tsan presets, the docs gates (`example_drift`,
`snippet_compile`, `example_run`, `report_shape`), `prose-lint`.

**Target Platform**: Linux on GCC and Clang; the suite stays
host-agnostic and passes under `taskset -c 0,1` and on a 2-cpu host
(FR-018).

**Performance Goals**: none on the timed path: the views read no
clock, measure nothing, and touch no baseline (VII).

**Constraints**: the harness CLI byte-identical (FR-001, FR-017); the
report schema at `1.8` (FR-003); the option value block `298` to `319`
free; 100% line and branch coverage with no exclusions (FR-019).

**Scale/Scope**: one public header, five implementation files and
one shared escaper header, one driver, one docs example, one
evidence directory.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1
design.*

- **I (standard-first)**: the new code is C++23 under the pinned
  Core Guidelines baseline; clang-tidy and cppcheck run on every
  compile the release presets drive.
- **II (DBC)**: every public interface carries `\pre`/`\post`; the
  implementation enforces them through the project contract facility
  (FR-020). The views abort nothing: the error forms carry the
  disclosures (D-10).
- **III (R-DCUT)**: this plan is the design artifact; tasks.md
  carries the decomposition; TDD mode stands recorded below and in
  tasks.md.
- **IV (docs)**: the public header joins the `docs` target; the main
  page gains its section with an example region (FR-022, IV.1).
- **V (style)**: naming follows V.1 (`renderHistogram`,
  `viewSeries`, `ViewSeries`), formatting through `.clang-format`.
- **VI (tests)**: every module lands with its covering tests; the
  gates hold at 100% line and branch (FR-019).
- **VII (performance)**: the views stand off the timed path; no
  baseline moves; no clock or sleep joins test or code (FR-018).
- **VIII (gates)**: the change passes the whole gate list; the
  version commit records the `SOVERSION` check (FR-002).
- **IX (SDD)**: spec, plan, tasks, implementation; all four
  artifacts land.
- **X (anti-slop)**: the module list ships the shapes the user
  stories name; the one shared resolver below is justified by
  FR-015's classification rule, and the Complexity Tracking section
  carries the justification.
- **XI (prose)**: every generated line clears `prose-lint`; the
  code spans stand on one line.

## Project Structure

### Documentation (this feature)

```text
specs/038-report-view-generators/
├── spec.md              # the specification (done)
├── clarifications-round1.md   # clarify round 1 (done)
├── clarifications-round2.md   # clarify round 2 (done)
├── plan.md              # this file
├── data-model.md        # the view request and answer types
├── contracts/
│   └── views-api.md     # the public function contracts
├── quickstart.md        # the evidence index linking each record
├── tasks.md             # the task list (Phase 2 output)
└── results/             # the evidence records (FR-021)
    ├── reports/         # the report documents the records rendered
    ├── placement.md     # the specs/035 placement-report record
    └── tracers.md       # the specs/036 tracer-report record
```

### Source Code (repository root)

```text
include/speedgun-ng/views.hpp   # the public surface (M-A)
source/views/
├── view_series.cpp             # the resolver (M-B)
├── histogram.cpp               # the SVG histogram (M-D)
├── plot.cpp                    # the SVG plot (M-E)
├── text_view.cpp               # the terminal text (M-F)
├── page.cpp                    # the HTML page (M-G)
└── escape.hpp                  # the shared escaper (M-C)
tools/report_view.cpp           # the thin driver (M-H)
example/docs/report_views.cpp   # the docs example (M-I)
example/docs/report_views.expected
test/source/views_test.cpp      # the library tests (M-B to M-G)
test/report_view_cli.sh         # the driver's CLI tests (M-H)
test/data/report_views/         # the fixture documents the tests load
```

## Design

### Decision index

| Ruling / question | Decision |
| --- | --- |
| R-1 the reader stands alone | D-1: `loadReport` and the record types are the whole input surface |
| R-2 library first | D-2: one public header, implementation under `source/views/`, the driver thin; harness `SOVERSION` rises to `16` |
| R-3 no new dependency | D-3: SVG, HTML and text hand-rolled; the histogram and the plot stand SVG figures the page embeds inline |
| R-4 the report stands unchanged | D-4: schema `1.8`, no key added, the `not available` disclosure for every absent record |
| R-5 evidence before close | D-5: one record per run, the report documents beside them, a script not a gate |
| Clarify 1/Q1 unavailable rows | out of the sample set and the series; the count names the rows a value stands for (D-6, D-7) |
| Clarify 1/Q2 exit classification | the driver pre-checks the request against the loaded document; every view error discloses at status 0 (FR-015, FR-016) |
| Clarify 1/Q3 counter columns | outside the selector; a counter label refuses; out of scope (FR-006) |
| Clarify 1/Q4 the page's disclosed figure | the disclosure text in place, the value form (FR-012) |
| Clarify 1/Q5 the time unit | the time columns scale by `timeUnit`, the unit named; metrics print as stored (FR-006) |
| Clarify 2/Q1 the text view's composition | per part, the page's rule (FR-013) |
| Clarify 2/Q2 the plot over an empty series | the disclosure naming the column and the zero count (FR-010) |
| Clarify 2/Q3 the histogram's format | SVG, one bar per bin beside its range and count label (FR-007, D-3) |

### Modules and dependency order

The feature splits into ten modules; each carries its public
surface, its contracts and its test plan, and each lands apart. The
order below stands the landing order; the brace marks the parallel
group.

```text
1. M-A  the public header, the CMake seams and the version move
2. { M-B  the series resolver
     M-C  the shared escaper }     # the two land in parallel
3. { M-D  the SVG histogram
     M-E  the SVG plot
     M-F  the text view }          # the three land in parallel
4. M-G  the HTML page
5. M-H  the driver
6. M-I  the docs and the example
7. M-J  the evidence records
```

- **M-A the public header, the CMake seams and the version move**
  (`include/speedgun-ng/views.hpp`, `CMakeLists.txt`). Public
  surface: `ViewSeries`, `viewSeries`, `renderHistogram`,
  `renderPlot`, `renderPage`, `renderText`, all `sg::`, all
  declared with doxygen contracts. The seams: the
  `speedgun-ng_harness` target's source list gains the
  `source/views/` files beside the `source/harness/` glob, the
  test executable `views_test` registers in `test/CMakeLists.txt`
  linking `speedgun-ng::harness`, and the version moves to
  `0.25.0` with harness `SOVERSION 16` and archive `SOVERSION 4`
  (FR-001, FR-002); the version assertions land in `views_test`
  beside the surface tests. Nothing else builds before M-A lands.
- **M-B the series resolver** (`source/views/view_series.cpp`).
  Public surface: `viewSeries` answering the column's recorded
  values, scaled by `timeUnit` for the time columns with the unit
  named, stored for a metric column. The resolver filters the rows
  whose value stands unavailable (the clarify 1/Q1 rule) and
  errors only on a name the document does not carry: the benchmark,
  or a column outside `time`, `cpu` and the metric labels (the
  counter-label refusal of clarify 1/Q3). Contracts:
  [contracts/views-api.md](./contracts/views-api.md) B-1. Test
  plan: the resolution rows of `views_test` over fixture documents
  (wave 2): the three column kinds, the unit scale and label, the
  unavailable-row filter, the benchmark-missing error, the
  counter-label error, the empty-document error. Depends on M-A.
- **M-C the shared escaper** (`source/views/escape.hpp`). Public
  surface: none (detail): the escape helper the SVG and the HTML
  writers share. Contracts: A-6 of views-api.md. Test plan: the
  escape rows of `views_test` (wave 2): the markup characters
  stand escaped, the plain text stands byte-identical. Depends on
  M-A alone, so it lands beside M-B.
- **M-D the SVG histogram** (`source/views/histogram.cpp`). Public
  surface: `renderHistogram`. The bins cover the recorded value
  range with the last bin closed on the maximum, the bars stand
  proportional to the counts, each bar carries its range and count
  label, and fewer than two values answers the disclosure naming
  the sample count (FR-007, FR-008, clarify 2/Q3). Contracts:
  views-api.md B-2. Test plan: the histogram rows of `views_test`
  (wave 3): the bin count argument, the range coverage, the
  proportional bars, the label text, the one-value disclosure, the
  all-unavailable disclosure. Depends on M-B and M-C.
- **M-E the SVG plot** (`source/views/plot.cpp`). Public surface:
  `renderPlot`. The series stands one point per recorded value in
  document order, the axes carry labels naming the row and the
  column, and an empty series answers the disclosure naming the
  column and the zero count (FR-009, FR-010, clarify 2/Q2).
  Contracts: views-api.md B-3. Test plan: the plot rows of
  `views_test` (wave 3): the point count and order, the axis
  labels, the single-point case, the empty-series disclosure, the
  multipass order. Depends on M-B and M-C.
- **M-F the text view** (`source/views/text_view.cpp`). Public
  surface: `renderText`. The histogram part renders one line per
  bin with a proportional character bar, the plot part one line
  per recorded value in row order, each part composing by the rule
  FR-012 states (clarify 2/Q1). Contracts: views-api.md B-4. Test
  plan: the text rows of `views_test` (wave 3): the bin lines, the
  series lines, the one-repetition composition, the disclosure
  lines. Depends on M-B and M-C.
- **M-G the HTML page** (`source/views/page.cpp`). Public surface:
  `renderPage`. The page carries the identity the document records
  (the `speedgun` version, the `schemaVersion`, the benchmark, the
  column), the two SVG figures inline, the text escaped through
  M-C's helper, and no script (FR-011, FR-012, D-8). Contracts:
  views-api.md B-5. Test plan: the page rows of `views_test`
  (wave 4): the identity fields, the absent-field disclosure, the
  embedded figures standing the standalone bytes, the escaping of
  a markup label, the no-script shape. Depends on M-D and M-E.
- **M-H the driver** (`tools/report_view.cpp`, `tools/CMakeLists.txt`).
  Public surface: none (a program). The driver parses the four
  arguments, loads the document, pre-checks the request against
  it, calls the view function, and answers the exit mapping of
  FR-014 to FR-017: view bytes on stdout at status 0, the
  disclosure on stdout at status 0, one refusal line on stderr at
  status 2, `--help` one line per argument at status 0. Contracts:
  views-api.md B-6. Test plan: the driver rows of
  `test/report_view_cli.sh` (wave 5): every view word over a
  fixture document, every refusal of FR-015, the disclosure exit,
  the help lines, and the harness CLI byte-identity rows of the
  standing option-table test staying untouched (SC-004). Depends
  on M-B through M-G.
- **M-I the docs and the example** (`README.md`,
  `example/docs/report_views.cpp`,
  `example/docs/report_views.expected`,
  `tools/docs/example_drift.py`). The example registers one
  benchmark the `example_run` gate drives (`--list` names it, the
  run carries `--min-time=0.01`, the rows print), runs it with the
  JSON write, loads the written document, renders the text view,
  and prints the lines; the `.expected` file records the output
  the page quotes. The main page gains its section: one purpose
  sentence, one marked snippet from the example region with its
  own `views.hpp` include, one result sentence, the
  quick-reference rows naming the public header (FR-022, IV.1).
  The drift gate's fixed tables move with the section: the section
  joins `SECTIONS` and `SECTION_OWNERS`, `views.hpp` joins
  `NAME_SCAN_HEADERS`, and the `PAGE_LINE_CAP` move stands
  measured and recorded beside it. Test plan: `example_drift`,
  `snippet_compile`, `example_run`, `report_shape`, the prose gate
  (wave 6). Depends on every module above.
- **M-J the evidence records**
  (`specs/038-report-view-generators/results/`). One record per
  run on thrawn: the commands, the exit codes, the report rendered
  and the views produced, the report documents under
  `results/reports/`, the script that drove the runs standing
  beside the records, the quickstart linking every record (FR-021,
  D-5). Test plan: none in CI; the check stands the quickstart's
  links and the records' own shape (wave 7). Depends on the built
  driver and the lane host.

### Risks

- The evidence runs need a report document with several repetitions:
  the specs/035 records name the placement commands, and the lane
  re-runs them with the report flag; a command the example refuses
  goes to the pull request as a question (D-5, the audit point).
- The recorded tracer documents stand at schema `1.7` on the lane
  host's scratch space: the reader loads the minors the merged
  lanes recorded (the Assumptions), and each record names the
  schema its document carries.
- The metric column's unit label: the data-model rules it empty,
  the metric column printing as stored and naming no unit (FR-006,
  A-5), and the tests pin it (M-B, data-model.md). The settled
  method stands at `BenchmarkResult::metricMethod`; the figures
  name no method.
- The SVG label text meets document-derived strings (benchmark
  names, labels): the escaper is shared and tested once (M-C), and
  the histogram and the plot call it (X.2: one escaper, no second).

### Logical view: the render path

```text
loadReport(path) ──> ReportData
                        │
viewSeries(report, benchmark, column)
   │  resolves the benchmark, the column kind,
   │  filters unavailable values, scales the time columns
   ▼
ViewSeries { values, unit }
   │
renderHistogram(report, benchmark, column, binCount) ── SVG text
renderPlot(report, benchmark, column)                ── SVG text
renderText(report, benchmark, column, binCount)      ── terminal text
renderPage(report, benchmark, column, binCount)
   │  identity + renderHistogram + renderPlot inline,
   │  each renderer resolving through viewSeries
   ▼
HTML text

driver: parse args → loadReport (error: refuse, 2)
        → viewSeries (error: refuse, 2)
        → render* (error: disclosure, 0) → stdout (0)
```

### Physical view

- The public surface stands in `include/speedgun-ng/views.hpp`,
  namespace `sg`, exported through the standing export header.
- The implementation stands under `source/views/` and compiles into
  the `speedgun-ng_harness` target, the reader's home: the target's
  source list gains the `source/views/` files beside the
  `source/harness/` glob (`CMakeLists.txt:234`). The driver stands
  at `tools/report_view.cpp` as its own executable target linking
  `speedgun-ng::harness`.
- The tests stand at `test/source/views_test.cpp` and
  `test/report_view_cli.sh`, registered with CTest; the fixture
  documents stand under `test/data/report_views/`.

### Public API surface added

- `sg::ViewSeries`: the resolved series, `values` and `unit`.
- `sg::viewSeries(const ReportData&, std::string_view benchmark,
  std::string_view column)`.
- `sg::renderHistogram(const ReportData&, std::string_view
  benchmark, std::string_view column, std::size_t binCount)`.
- `sg::renderPlot(const ReportData&, std::string_view benchmark,
  std::string_view column)`.
- `sg::renderPage(const ReportData&, std::string_view benchmark,
  std::string_view column, std::size_t binCount)`.
- `sg::renderText(const ReportData&, std::string_view benchmark,
  std::string_view column, std::size_t binCount)`.
- Every function answers `std::expected<std::string, std::string>`
  (`viewSeries`: `std::expected<ViewSeries, std::string>`), throws
  nothing, aborts nothing, reads no file, writes no file, reads no
  clock (FR-004, D-10).

### Version and `SOVERSION`

- Version: `0.24.0` → `0.25.0`, the next free minor on the master
  line (`CMakeLists.txt:26`).
- Harness `SOVERSION`: `15` → `16`: the view functions join the
  exported library surface behind the public header (FR-002, D-2).
- Archive `SOVERSION`: stays `4` (`CMakeLists.txt:91`).
- Report schema: stays `1.8` (`source/harness/json_report.cpp:39`);
  no key moves (FR-003, D-4).
- Option values: no row joins; the block `298` to `319` stays free
  (`source/harness/detail/options.hpp`).
- The commit that moves the numbers records the ABI check beside
  them (FR-002).

## Test Plan

**Execution mode**: TDD (constitution III). The covering test task
of each wave stands written and observed failing before the
implementation task that turns it green; plan.md records the mode
here, and tasks.md carries it per wave.

The tests render from documents the fixtures build: small JSON
documents under `test/data/report_views/` the tests load with
`loadReport`, plus documents a fixture builder assembles in memory
through the record types. No test sleeps, reads a clock, or runs a
timing measurement: the renderer carries no timing figure (FR-018).
The suite passes under `taskset -c 0,1` and on a 2-cpu host.

### Waves

- **Wave 1 (M-A)**: the header compiles, the functions link, the
  version stands at `0.25.0` with `SOVERSION 16` / archive `4`.
  Verify: `ctest --preset=dev -R views_test`, plus the version
  print test.
- **Wave 2 (M-B, M-C)**: the resolver and escaper rows. Verify:
  `ctest --preset=dev -R views_test`, red first, then green.
- **Wave 3 (M-D, M-E, M-F in parallel)**: the three renderers'
  rows, one test binary, disjoint scenario blocks per module.
  Verify: `ctest --preset=dev -R views_test`.
- **Wave 4 (M-G)**: the page rows. Verify:
  `ctest --preset=dev -R views_test`.
- **Wave 5 (M-H)**: the driver rows through the CLI script test.
  Verify: `ctest --preset=dev -R report_view_cli_test`.
- **Wave 6 (M-I)**: the docs gates. Verify:
  `ctest --preset=dev -R 'example_drift|snippet_compile|example_run|report_shape'`
  and `cmake -P cmake/prose-lint.cmake`.
- **Wave 7 (M-J)**: the evidence records on thrawn. Verify: the
  records' exit codes stand 0 and the quickstart links resolve.
- **Wave 8 (convergence)**: the full suite, the coverage gate, the
  sanitizer and tsan presets, the release build (`ci-ubuntu` once
  for the analyzer findings), the prose gate over the range.
  Verify: `ctest --test-dir build --output-on-failure`, the
  coverage read at 100.0/100.0, `ci-sanitize`, `ci-tsan`,
  `cmake --preset=ci-ubuntu && cmake --build build`.

### FR coverage map

| FR | Wave | Test |
| --- | --- | --- |
| FR-001 | 1, 8 | the header builds; the byte-identity rows of the option-table test stay untouched |
| FR-002 | 1 | the version assertions of `views_test` read `0.25.0`; the commit records the check |
| FR-003 | 8 | the schema round-trip tests stay green; no key moves |
| FR-004 | 2-4 | the resolver and renderer rows: no file, no clock, no throw (the contracts) |
| FR-005 | 2-4 | the error-form rows: the disclosure text names the input |
| FR-006 | 2 | the three column kinds, the unit scale, the counter-label refusal |
| FR-007 | 3 | the histogram rows: bins, coverage, proportion, labels, SVG bytes |
| FR-008 | 3 | the one-value and all-unavailable disclosure rows |
| FR-009 | 3 | the plot rows: points, order, labels, single point |
| FR-010 | 3 | the empty-series disclosure row |
| FR-011 | 4 | the identity, the embedded figures, the no-script shape |
| FR-012 | 4 | the absent-field and in-place disclosure rows |
| FR-013 | 3 | the text rows: bin lines, series lines, per-part composition |
| FR-014 | 5 | the driver rows: four views on stdout, status 0 |
| FR-015 | 5 | the refusal rows: reader fault, absent names, bad view word, missing or repeated argument |
| FR-016 | 5 | the disclosure-exit rows |
| FR-017 | 5 | the help rows; the harness CLI byte-identity holds |
| FR-018 | 2-5, 8 | no clock or sleep in the tests; the taskset run |
| FR-019 | 8 | the coverage read at 100.0/100.0, no `LCOV_EXCL`, no new `--ignore-errors` |
| FR-020 | 2-4 | the doxygen gate over the public header; the contract enforcement rows |
| FR-021 | 7 | the records name commands, exit codes, report and views; the quickstart links |
| FR-022 | 6 | `example_drift`, `snippet_compile`, `example_run`, `report_shape` |

## Constitution Check (re-check after design)

The design holds the gate list: library-first (D-2), no dependency
(D-3), the report unchanged (D-4), the harness CLI byte-identical
(FR-001, FR-017), the tests deterministic and fixture-fed (FR-018),
the coverage gates named (FR-019), the contracts carried (FR-020),
the docs gates named (FR-022), the version record named (FR-002).
One shared resolver stands justified below.

## Complexity Tracking

- The `viewSeries` resolver is one public function serving four
  renderers and the driver's pre-check: five callers, so the
  indirection earns itself (X.2). The alternative, each renderer
  resolving internally, would either duplicate the
  unavailable-row filter four times or move the driver's
  classification rule into the renderers, the duplication the
  clarify 1/Q2 ruling avoids.
- The escaper is one shared helper for the SVG and the HTML
  writers; a per-writer escaper would carry two spellings of one
  fact (X.2).
- No other abstraction, configurability or extension point joins:
  the view words are four, the formats are three, and the page
  composes two figures the spec names.
