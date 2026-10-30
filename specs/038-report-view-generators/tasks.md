# Tasks: Report View Generators

**Input**: Design documents from
`/specs/038-report-view-generators/`

**Prerequisites**: [plan.md](./plan.md) (required), [spec.md](./spec.md)
(required for user stories), [contracts/views-api.md](./contracts/views-api.md),
[data-model.md](./data-model.md)

**Tests**: TDD mode (plan.md, Execution mode): the test task of each
wave stands written and observed failing before the implementation task
that turns it green.

**Organization**: tasks stand grouped by wave behind the module landing
order plan.md fixes (M-A to M-J): the public surface stands first, the
resolver and the escaper land in one parallel block, the three
renderers land in the next, the page, the driver, the docs and the
evidence close. Every task names its exact file path, the requirements
it closes, and the verify command with an exit status.

## Format: `[ID] [P?] [Wave] Description`

- **[P]**: can run in parallel (different files, or one file behind
  disjoint scenario blocks)
- **[Wave]**: the wave this task serves (the plan's wave list)
- Every description names the exact file path, the requirements it
  closes, and the verify command

---

## Phase 1: Setup and Foundational - the public header and the version move (module M-A)

**Purpose**: the surface every story stands on, and the version record
the lane stands on. No other module builds before this phase closes.

- [X] T001 [P] Register the test executable `views_test` in
  `test/CMakeLists.txt` beside the standing harness tests
  (`add_executable`, link `speedgun-ng::harness`, `add_test`), then
  write the public-surface tests in `test/source/views_test.cpp`
  red first (the header missing, so the unit fails to compile):
  `ViewSeries` default-constructs empty, and the declared
  signatures stand type-checked through the header's declares
  (FR-001, FR-004, FR-005). The zero-benchmark error row stands in
  the resolver block T004 carries. Verify:
  `ctest --test-dir build -R views_test --output-on-failure`, red
  on the missing header, green once the header lands (the release
  build configured).
- [X] T002 Move the version and record the ABI check: `VERSION
  0.25.0` in `CMakeLists.txt` with the comment recording it (the
  view functions join the exported harness surface, so the harness
  `SOVERSION` stands `16` and the archive `SOVERSION` stands `4`),
  `SOVERSION 16` beside the harness target's `SOVERSION 15`, the
  `speedgun-ng_harness` source list gaining the `source/views/`
  files beside the `source/harness/` glob, and the version
  assertions in `views_test` reading `0.25.0` and the two
  `SOVERSION` lines (FR-002, D-2). Verify:
  `ctest --test-dir build -R views_test --output-on-failure` green,
  the standing version expectation of `harness_cli_table_test`
  green (it reads the `project(` block, so the move stands
  observed), and the commit message carries the record.

## Phase 2: The resolver and the escaper in parallel (modules M-B and M-C, wave 2)

- [X] T003 [P] Write the fixture documents under
  `test/data/report_views/`: one multi-repetition document with a
  metric column and mixed availability, one single-repetition
  document, one multipass document, one document whose `speedgun`
  header stands absent, one metric label carrying a markup
  character (FR-006, FR-018, the fixture rows of the test plan).
  Verify: the files load through `loadReport` in a temporary check
  run of `views_test`.
- [X] T004 Write the failing resolver tests in
  `test/source/views_test.cpp` (the resolver block): the three
  column kinds resolve the recorded values in document order, the
  time columns scale by `timeUnit` with the unit named, the metric
  column stands stored, the unavailable rows stand filtered with
  the count naming the rows a value stands for, the
  benchmark-missing error, the counter-label error, and the
  empty-series value form (FR-004 to FR-006, B-1, A-3 to A-5 of
  the contract). Verify:
  `ctest --test-dir build -R views_test --output-on-failure` red on
  the block.
- [X] T005 Implement `viewSeries` in
  `source/views/view_series.cpp` with the contracts of B-1:
  resolution over `results`, the availability filter, the unit
  scale, and the error forms naming the name and the recorded set;
  enforce the contracts with the project facility (FR-020). Verify:
  `ctest --test-dir build -R views_test --output-on-failure` green.
- [X] T006 [P] Write the failing escaper tests in
  `test/source/views_test.cpp` (the escape block): the markup
  characters stand escaped, the plain text stands byte-identical,
  and the fixture's markup metric label stands escaped through the
  escaper (A-6, D-8). Verify:
  `ctest --test-dir build -R views_test --output-on-failure` red on
  the block.
- [X] T007 [P] Implement the shared escaper at
  `source/views/escape.hpp` (A-6). Verify:
  `ctest --test-dir build -R views_test --output-on-failure` green
  on the block.

## Phase 3: The three renderers in parallel (modules M-D, M-E, M-F, wave 3)

- [X] T008 [P] Write the failing histogram tests in
  `test/source/views_test.cpp` (the histogram block): the SVG bytes
  carry one bar per bin beside its range and count label, the bins
  cover the recorded value range with the last bin closed on the
  maximum, the bars stand proportional to the counts, the bin
  count argument bounds the bars, the one-value disclosure, the
  all-unavailable disclosure, and the no-throw/no-file contract
  rows (FR-007, FR-008, B-2). Verify:
  `ctest --test-dir build -R views_test --output-on-failure` red on
  the block.
- [X] T009 [P] Implement `renderHistogram` in
  `source/views/histogram.cpp`, the label text through the shared
  escaper (FR-007, FR-008). Verify:
  `ctest --test-dir build -R views_test --output-on-failure` green.
- [X] T010 [P] Write the failing plot tests in
  `test/source/views_test.cpp` (the plot block): one point per
  recorded value in document order, the multipass order, the axis
  labels naming the row and the column, the single-point case, the
  empty-series disclosure naming the column and the zero count
  (FR-009, FR-010, B-3). Verify:
  `ctest --test-dir build -R views_test --output-on-failure` red on
  the block.
- [X] T011 [P] Implement `renderPlot` in `source/views/plot.cpp`,
  the axis label text through the shared escaper (FR-009, FR-010).
  Verify: `ctest --test-dir build -R views_test --output-on-failure`
  green.
- [X] T012 [P] Write the failing text-view tests in
  `test/source/views_test.cpp` (the text block): one line per bin
  with the proportional character bar, one line per recorded value
  in row order, the one-repetition composition with the disclosure
  line in the histogram's place and the plot lines rendering
  (FR-013, B-4). Verify:
  `ctest --test-dir build -R views_test --output-on-failure` red on
  the block.
- [X] T013 [P] Implement `renderText` in
  `source/views/text_view.cpp` (FR-013). Verify:
  `ctest --test-dir build -R views_test --output-on-failure` green.

## Phase 4: The HTML page (module M-G, wave 4)

- [X] T014 Write the failing page tests in
  `test/source/views_test.cpp` (the page block): the identity
  fields name the document's values, the absent-field disclosure,
  the embedded figure bytes standing equal to the standalone
  bytes, the disclosure text in a disclosed figure's place, the
  no-script shape (FR-011, FR-012, B-5). Verify:
  `ctest --test-dir build -R views_test --output-on-failure` red on
  the block.
- [X] T015 Implement `renderPage` in `source/views/page.cpp`
  composing the identity, the two inline figures and the shared
  escaper (FR-011, FR-012). Verify:
  `ctest --test-dir build -R views_test --output-on-failure` green.

## Phase 5: The driver (module M-H, wave 5)

- [X] T016 Add the driver target: `tools/CMakeLists.txt` with the
  `report_view` executable linking `speedgun-ng::harness`, and the
  `add_subdirectory` seam in the top-level `CMakeLists.txt` (D-2,
  FR-014). Verify: `cmake --build build` builds `report_view`.
- [X] T017 [P] Write the failing CLI tests at
  `test/report_view_cli.sh`, the standing gate-script pattern
  (`test/time_source_gate.sh`): registered with CTest, the script
  drives the built binary over the fixture documents and asserts
  every row of the contract's refusal and disclosure table: the
  four views on stdout at status 0, the reader-fault refusal, the
  benchmark-missing refusal, the counter-label refusal, the view
  word refusal, the missing and repeated argument refusals, the
  disclosure exit at status 0, and the `--help` lines (FR-014 to
  FR-017, B-6). Verify:
  `ctest --test-dir build -R report_view_cli_test --output-on-failure`
  red.
- [X] T018 Implement the driver at `tools/report_view.cpp`: the
  argument parse, the load, the request pre-check against the
  loaded document, the view call, the exit mapping, and the help
  lines (FR-014 to FR-017). Verify:
  `ctest --test-dir build -R report_view_cli_test --output-on-failure`
  green, and the harness CLI byte-identity rows of
  `harness_cli_table_test` stand untouched (SC-004).

## Phase 6: The docs teach the views (module M-I, wave 6)

- [X] T019 [P] Write the docs example at
  `example/docs/report_views.cpp`: one registered benchmark the
  `example_run` gate drives (the `--list` flag names it, the run
  carries `--min-time=0.01`, the rows print), the run writing its
  JSON document, the document loaded with `loadReport`, the text
  view rendered, and the lines printed; register it through
  `add_docs_example` and write
  `example/docs/report_views.expected` recording the output (the
  page's result sentence quotes it) (FR-022, IV.1). Verify:
  `ctest --test-dir build -R example_run --output-on-failure` runs
  the binary and the rows print.
- [X] T020 [P] Add the main-page section to `README.md`: one purpose
  sentence, the marked snippet quoting the example region with its
  own `views.hpp` include, one result sentence, and the
  quick-reference rows naming `sg::renderHistogram` to
  `sg::renderText` and `sg::viewSeries`; move the drift gate's
  fixed tables with the section: the section joins `SECTIONS` and
  `SECTION_OWNERS` in `tools/docs/example_drift.py`, `views.hpp`
  joins `NAME_SCAN_HEADERS`, and the `PAGE_LINE_CAP` move stands
  measured and recorded beside it (FR-022, IV.1). Verify:
  `ctest --test-dir build -R 'example_drift|snippet_compile' --output-on-failure`
  green, and `cmake -P cmake/prose-lint.cmake` reads no finding.

## Phase 7: The evidence records (module M-J, wave 7)

- [X] T021 Run the acceptance evidence on thrawn: land the script
  that drove the runs beside the records
  (`specs/038-report-view-generators/results/run_views.sh`), re-run
  the placement commands the specs/035 records name with the
  report flag, render the placement report's views through the
  driver, render the tracer reports' views (`/tmp/osnoise.json`,
  `/tmp/timerlat.json`), and record one document per run under
  `specs/038-report-view-generators/results/`: the commands, the
  exit codes, the report rendered and the views produced, with the
  report documents under `results/reports/` (FR-021, D-5). Verify:
  every recorded command exits 0, every record names the report
  and the views, and the script runs end to end.
- [X] T022 [P] Write `specs/038-report-view-generators/quickstart.md`
  linking every record and the standing feature docs, the way the
  specs/036 quickstart links its records (FR-021). Verify: every
  link resolves on the checked-out tree.

## Phase 8: Polish, gates and convergence

- [X] T023 Run the full suite and the hard gates:
  `ctest --test-dir build --output-on-failure`, the coverage preset
  read at 100.0% lines, 100.0% branches and 100.0% DBC with no
  `LCOV_EXCL` marker and no new `--ignore-errors` entry, the
  `ci-sanitize` and `ci-tsan` presets, the release build
  (`cmake --preset=ci-ubuntu && cmake --build build`) once for the
  analyzer findings, `cmake -P cmake/prose-lint.cmake`, and the
  `taskset -c 0,1` suite run (FR-018, FR-019, SC-005 to SC-007).
  Verify: every gate green, the coverage read recorded in the
  results directory's verdict record.
- [X] T024 Confirm the standing surfaces stand untouched: the
  harness CLI byte-identity, the schema at `1.8` with no key moved
  (`docs/schema/report-v1.json` stands unchanged), the option block
  `298` to `319` free, and the doxygen `docs` target documents the
  new header (FR-001, FR-003, FR-017, FR-020, SC-004). Verify:
  `ctest --test-dir build -R 'harness_cli_table|report_shape|schema'`
  green and the doxygen build warns on nothing new.

## Dependencies & Execution Order

### Phase Dependencies

- Phase 1 (M-A) stands first: the header and the version move stand
  the surface every later task builds on.
- Phase 2 (M-B, M-C) depends on Phase 1 alone; its two modules
  stand in one parallel block.
- Phase 3 (M-D, M-E, M-F) depends on Phase 2; its three modules
  stand in one parallel block.
- Phase 4 (M-G) depends on the histogram and the plot of Phase 3.
- Phase 5 (M-H) depends on Phases 2 through 4: the driver calls
  every view.
- Phase 6 (M-I) depends on Phase 5: the example renders through the
  library the docs name.
- Phase 7 (M-J) depends on the built driver and the lane host.
- Phase 8 closes the feature and stands last.

### Parallel Blocks

- Phase 2: T003/T004 (the fixtures and the resolver red run) and
  T006/T007 (the escaper) stand parallel over disjoint blocks.
- Phase 3: T008/T009 (histogram), T010/T011 (plot), T012/T013
  (text) stand three parallel pairs over disjoint files.
- T019 and T020 stand in parallel over the example and the page;
  T022 stands beside T021's runs.

## Implementation Strategy

### MVP First (User Stories 1 and 2)

Phases 1 and 2 deliver the resolver and the escaper: the library
answers the series the figures render, every later task stands
testable, and the not-available rule and the escape rule stand
enforced once.

### Incremental Delivery

Each phase leaves the suite green: a phase closed is a feature slice
that builds, passes its tests, and stands committed.

## Notes

- The commit plan: one logical block per phase, each commit
  compiling and passing its tests, the titles following the
  template with the standing sections (`Harness: ...` for the
  views and the driver, `CMake: ...`, `Docs: ...`, `Meta: ...`),
  the `Refs:` footer naming `specs/038-report-view-generators`.
- The evidence runs ride the lane host thrawn and stand a script,
  not a CI gate (D-5).
