# Feature Specification: Report View Generators

**Feature Branch**: `038-report-view-generators`

**Created**: 2026-10-29

**Status**: Draft

**Input**: User description, opening quoted:

> Give the run's numbers their views, item N8 of the roadmap (section
> 3, origin ruling H15): a renderer reads the standing JSON report
> and produces the histogram, the plot, the HTML page and the text
> views of the figures the run recorded. The report is the single
> source of truth; the views add no figure the run did not measure.

The request fixes the rulings R-1 to R-5 of the lane prompt; they
stand in Decisions as D-1 to D-5. The further decisions cover the
shapes the prompt leaves open: the sample set of the histogram, the
series of the plot, the page shape, the driver grammar and the
evidence record shape.

## Spec identity and run-time binding

- Spec number: `038`. `specs/` on master `1049cce8` holds `001`
  through `036` with `033` merged (read 2026-10-29). `031` is
  reserved for N0b host validation, `037` for N11 execution graphs,
  whose lane prompt names chilly's lane. `git ls-remote --heads`
  names no ref carrying `038` or `report-view` (read 2026-10-29).
  The `gh pr list` check did not run: `gh` carries no token in this
  session, so the pull-request list stands on the directory and
  remote-branch checks and on the lane prompt's own record; the
  prompt's open pull request (`033-hunter-budgets`) merged before
  branch time, and the branch cut from the merge.
- Branch: `038-report-view-generators`, cut from master `1049cce8`
  on 2026-10-29. Master stands on that hash at branch time, so no
  moved-base record applies.
- Version: master stands at `0.24.0` (`CMakeLists.txt:26`), harness
  `SOVERSION 15` (`CMakeLists.txt:320`), archive `SOVERSION 4`
  (`CMakeLists.txt:91`). The lane takes the next free minor,
  `0.25.0`. The view functions join the exported library surface
  behind a public header, so the harness `SOVERSION` rises from `15`
  to `16`; the archive `SOVERSION` stays `4`, and the commit that
  moves the numbers records the check beside them (FR-002, D-2).
- Report schema: master stands at `1.8`
  (`source/harness/json_report.cpp:39`, `kSchemaVersion`). The views
  read the document and add no key, so the schema minor stays `1.8`;
  any move stands recorded beside the version (R-4, FR-003).
- Option values: the harness CLI stands unchanged. The renderer's
  arguments are the driver's own, so no option row joins the
  standing table and the value block `298` to `319` stays free
  (`source/harness/detail/options.hpp:81` to `:107` name the
  standing constants; `kOsNoiseOptionValue` stands at `297`) (R-2,
  D-9).

## Roadmap position

- Item N8 of the harness roadmap, section 3, origin ruling H15. The
  figures the views render stand recorded: the statistics of
  specs/019, the percentile points of specs/021, the metric columns
  of specs/022, the jitter, pre-check and hunt records beside them.
  The evidence lanes stand merged: specs/035 (the placement runs)
  and specs/036 (the tracer runs). Roadmap section 10.6 stays in
  force in full.

## Lane identity

- The item stands hardware-neutral. The acceptance axis is the
  recorded reports of the merged lanes: the placement runs of
  specs/035 and the tracer runs of specs/036 stand recorded on
  thrawn, and the views render from reports the standing reader
  loads. The suite itself stays host-agnostic: the tests render from
  documents the fixtures build, and the suite passes under
  `taskset -c 0,1` and on a 2-cpu host (FR-018).
- The lane host records the evidence: the acceptance runs render the
  reports of specs/035 and specs/036 on thrawn, one record per run
  under the results directory, the way specs/036 recorded the tracer
  evidence (D-5, FR-021).

## Audit point

- Repository: `master` at `1049cce8` (project version `0.24.0`,
  report schema `1.8`, harness `SOVERSION 15`, archive
  `SOVERSION 4`), read 2026-10-29 on the lane host (thrawn).
- The report reader stands public at
  `include/speedgun-ng/benchmark.hpp:2878` (`loadReport`, answering
  `std::expected<ReportData, std::string>`); the loaded value's
  record types stand public in the same header: `ReportData`
  (`:792`) with `results`, `rowRaw`, `contextRaw` and the run-level
  `jitter`, `precheck` and `hunt` records beside them,
  `BenchmarkResult` (`:627`) with its rows, `ResultRow` (`:383`)
  carrying the statistics of specs/019 (`timeStats`,
  `metricStats`), the percentiles of specs/021 (`percentiles`), the
  metric columns of specs/022 (`metrics` beside `metricLabels`), and
  `RowKind` (`:90`) naming `REPETITION` and `AGGREGATE`.
- The report document and its schema stand at
  `docs/schema/report-v1.json` (the top-level `x-order` at `:7`).
- The results-evidence precedent stands at
  `specs/036-osnoise-timerlat-tracers/results/`: one record per run
  naming the commands, the exit codes and the artifacts, the
  quickstart linking each record.
- The recorded reports the evidence renders stand on the lane host:
  the tracer-run documents of specs/036 (`/tmp/osnoise.json` and
  `/tmp/timerlat.json`, schema `1.7`) stand in the shell's scratch
  space, and the placement-run documents of specs/035 stand recorded
  as commands alone. The lane re-runs the commands the specs/035
  records name and records the fresh documents beside its own
  records (D-5, FR-021).

## Standing facts this lane reads

- `loadReport` answers `std::expected<ReportData, std::string>`; the
  error form names the JSON pointer of the fault and what the reader
  expected there (the postconditions at
  `include/speedgun-ng/benchmark.hpp:2878`).
- A benchmark result carries its metric columns in `metricLabels`,
  one `MetricValue` per label per row (`ResultRow::metrics`); the
  primary time column prints in the unit `timeUnit` names.
- The row kinds the document records: `REPETITION` per repetition,
  `AGGREGATE` per benchmark; a multipass run records the repetition
  rows of its passes in pass order (specs/024).
- The disclosure rule the console and the document share: a figure
  the record does not carry prints `not available` (the tracer lines
  of specs/036, the pre-check lines of specs/034).
- The option master records the harness value blocks at
  `source/harness/detail/options.hpp`; the block `298` to `319`
  stands free.

## Clarifications

### Session 2026-10-29

- Q: Does a repetition row whose value stands unavailable join the
  histogram's sample set and the plot's series, and what does the
  sample count name? → A: The row stands out of the sample set and
  the series; the figure carries the measured values alone, and the
  sample count names the rows a value stands for (D-6, D-7). A gap
  marker or a zero count would carry a shape the run did not
  measure, and one unavailable value hides no measured repetition.
- Q: How does the driver choose between the refusal of FR-015 and
  the disclosure of FR-016 when both ride one error form? → A: The
  driver checks the request against the loaded document (the
  benchmark name, the column selector, the view word) before it
  calls the view function; a failed check refuses with status 2,
  and every error the view function answers is a disclosure with
  status 0 (FR-015, FR-016). The library keeps the one error shape
  D-10 names, and the classification stands testable as two driver
  cases.
- Q: Does the column selector resolve a user counter column? → A:
  No: the selector ends at `time`, `cpu` and the metric labels; a
  counter label refuses one line, and a later spec extends the
  selector (FR-006, Out of scope). No user story, fixture or
  evidence run names a counter column, so the lane ships the
  shapes the stories name.
- Q: What does the page carry when one embedded figure answers the
  disclosure? → A: The disclosure text stands in the figure's
  place, and the page answers the value form (FR-012). The page
  composes the two figures and the identity, so each part renders
  by the rule the part follows alone; a one-repetition column
  still renders its plot in the page view.
- Q: What unit do the time figures print? → A: The time columns
  print scaled by the `timeUnit` the benchmark records, the unit
  named beside the values; a metric column prints as stored
  (FR-006). The figures then agree with the report's printed
  columns a reader cross-reads.

## Decisions

- **D-1 (R-1)**: The reader stands alone. `loadReport` and the
  record types are the whole input surface; the renderer carries no
  second JSON reader and no second schema.
- **D-2 (R-2)**: Library first. The view functions live in the
  `speedgun-ng` library behind one public header,
  `include/speedgun-ng/views.hpp`; the implementation lives under
  `source/views/`. The driver stands at `tools/report_view.cpp` and
  carries no logic the tests cannot reach through the library. The
  exported library surface grows, so the harness `SOVERSION` rises
  to `16` and the commit records the check (FR-002).
- **D-3 (R-3)**: The formats stand hand-rolled. The SVG, the HTML
  and the text views are written from the standing records; no
  plotting or HTML library joins the dependency set.
- **D-4 (R-4)**: The report stands unchanged. The schema minor stays
  `1.8`; the views render the figures the run recorded and invent
  none; a record the document does not carry answers the standing
  `not available` disclosure.
- **D-5 (R-5)**: The evidence lands before the lane closes. The
  rendered views stand in the results directory, each record naming
  the report it rendered and the views it produced; the lane records
  the report documents beside the records so a reader can re-render
  every figure. A script stands behind the runs; no CI gate carries
  them.
- **D-6**: The histogram's sample set: the per-repetition values of
  one column, the distribution the `Statistics` of specs/019
  summarizes. A row whose value stands unavailable (an `Availability`
  other than `COUNTABLE`) stands out of the sample set: the figure
  carries the measured values alone, and the sample count names the
  rows a value stands for. Fewer than two recorded values names no
  shape, so the histogram answers `not available` with the sample
  count named.
- **D-7**: The plot's series: one column's values across the
  benchmark's repetition rows in document order; a multipass run's
  rows stand the passes in order. The `AGGREGATE` row stands out of
  the series: it summarizes the rows, it is no sample. A row whose
  value stands unavailable stands out of the series, the rule D-6
  states for the sample set.
- **D-8**: The page shape: one static HTML document, no script, the
  views inline, the run's identity beside them, the text escaped by
  hand.
- **D-9**: The driver grammar:
  `report_view <report> --benchmark=<name> --metric=<column> --view=histogram|plot|page|text`,
  the view on standard output, the status 0; one-line refusals on
  standard error, the status 2; `--help` prints one line per
  argument.
- **D-10**: The answer shape: each view function answers
  `std::expected<std::string, std::string>`; the value carries the
  view bytes, the error names the input the document does not carry;
  the functions read no file, write no file, read no clock, throw
  nothing to the caller and abort nothing.

## Out of scope

- Raster or PDF output, and a plotting, charting or HTML library.
- A report writer, a schema key, or a reader key: the document
  stands byte-shaped as master writes it.
- Harness CLI options: the renderer's arguments are the driver's
  own.
- Views over two reports, comparison figures, and interactive
  views: one report in, one view out.
- User counter columns as a view column: the selector ends at the
  time columns and the metric labels; a later spec extends it.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - The histogram renders the recorded distribution (Priority: P1)

A user who ran a benchmark with several repetitions loads the JSON
report with `loadReport` and asks the library for the histogram of
one column over one benchmark. The answer carries one bin block per
bin over the recorded value range, each block naming its range and
its count, and no figure the document does not record.

**Why this priority**: the histogram is the distribution view the
statistics of specs/019 summarize; without it the report's numbers
stand unviewed.

**Independent Test**: with a fixture-built document, the histogram
renders; with a fixture whose benchmark stands with fewer than two
values, the answer is the disclosure naming the sample count.

**Acceptance Scenarios**:

1. **Given** a fixture-built document with repetition rows carrying
   the primary time and one metric, **When** the library renders the
   histogram of each column, **Then** each answer carries bins over
   the recorded values and no figure the document does not record.
2. **Given** a fixture whose benchmark stands with one repetition
   row, **When** the library renders the histogram, **Then** the
   answer names `not available` and the sample count.

### User Story 2 - The plot renders the recorded series (Priority: P1)

A user asks the library for the plot of one column over one
benchmark. The answer carries one SVG polyline with labeled axes,
one point per repetition row in document order; a multipass run's
rows stand the passes in order.

**Why this priority**: the series over the repetitions is the figure
the run measures repetition by repetition; the histogram summarizes
it, the plot shows its order.

**Independent Test**: with a fixture-built document with repetition
rows and with a multipass fixture, the plot renders one point per
recorded row; with a fixture whose benchmark records no repetition
row, the plot answers the disclosure.

**Acceptance Scenarios**:

1. **Given** a fixture-built document with repetition rows,
   **When** the library renders the plot of one column, **Then** the
   answer carries one point per repetition row in document order and
   the axes carry labels naming the row and the column.
2. **Given** a multipass fixture, **When** the library renders the
   plot, **Then** the series stands in pass order.
3. **Given** a fixture whose benchmark carries the aggregate row
   alone, **When** the library renders the plot, **Then** the answer
   names `not available`.

### User Story 3 - The page carries the views with the run's identity (Priority: P1)

A user asks the library for the page of one column over one
benchmark. The answer carries one static HTML document: the run's
identity as the document records it, the histogram and the plot
inline, the text escaped, and no script.

**Why this priority**: the page is the deliverable a browser reads;
the figures and the identity stand in one artifact.

**Independent Test**: with a fixture-built document, the page
renders; the identity fields name the document's values; a fixture
whose identity field stands absent renders `not available` in its
place.

**Acceptance Scenarios**:

1. **Given** a fixture-built document, **When** the library renders
   the page, **Then** the answer carries the `speedgun` version, the
   `schemaVersion`, the benchmark name and the column, with the
   histogram and the plot inline.
2. **Given** a fixture whose document carries a metric label with a
   markup character, **When** the library renders the page,
   **Then** the label stands escaped in the answer.

### User Story 4 - The text views render the same figures for the terminal (Priority: P2)

A user at a terminal asks for the text view of one column over one
benchmark. The answer carries the histogram as one line per bin with
a proportional bar and the plot as one line per recorded value in
row order.

**Why this priority**: the terminal is where the run happened; the
text views name the same figures the graphics name, so one rendering
stands behind both.

**Independent Test**: with the fixtures of the two stories above,
the text view renders and names the same bins and values the
graphic views render.

**Acceptance Scenarios**:

1. **Given** a fixture-built document, **When** the library renders
   the text view, **Then** the histogram lines name the bin ranges
   and counts the graphic histogram renders, and the plot lines name
   the values in row order.

### User Story 5 - The driver drives the library from the command line (Priority: P2)

A user runs `report_view` with the report file, the benchmark, the
metric and the view. The driver prints the view on standard output
and exits 0; a malformed document, a name the document does not
carry, and a bad argument each refuse one line on standard error
with exit status 2 before any view output.

**Why this priority**: the driver is the entry point the lane prompt
names; the library stands whole without it, and the refusals keep a
script's pipeline loud.

**Independent Test**: with a fixture document written to a file, the
driver prints each view and exits 0; with a malformed file and with
each refusal case, one line and exit status 2.

**Acceptance Scenarios**:

1. **Given** a fixture document and
   `--view=histogram --benchmark=<name> --metric=time`, **When** the
   driver runs, **Then** the view stands on standard output and the
   exit status is 0.
2. **Given** a file the reader refuses, **When** the driver runs,
   **Then** one line names the reader's fault and the exit status is
   2.
3. **Given** `--benchmark=absent`, **When** the driver runs, **Then**
   one line names the benchmark and the exit status is 2.
4. **Given** `--view=sound`, **When** the driver runs, **Then** one
   line names the view word and the exit status is 2.
5. **Given** `--help`, **When** the driver runs, **Then** one line
   per argument stands on standard output and the exit status is 0.

### User Story 6 - The disclosure names the absent record (Priority: P2)

A user asks for a view whose record the run left absent: a benchmark
hidden its repetition rows, a run captured no metric the request
names, the page's identity field stands absent. The view answers
`not available` naming the input, the driver prints the disclosure
on standard output, and the exit status is 0.

**Why this priority**: the standing disclosure rule keeps a view
from inventing a figure; the record the document does not carry
names itself.

**Independent Test**: with fixtures whose records stand absent, each
view answers the disclosure naming its input, and the driver exits 0.

**Acceptance Scenarios**:

1. **Given** a fixture whose benchmark carries the aggregate row
   alone, **When** the library renders the histogram, **Then** the
   answer names `not available` and the sample count.
2. **Given** a fixture whose document carries no `speedgun` header,
   **When** the library renders the page, **Then** the identity
   field renders `not available`.

### User Story 7 - The evidence stands recorded and the docs teach the views (Priority: P3)

The lane records the acceptance runs on thrawn: one record per run
under the results directory naming the commands, the exit codes, the
report rendered and the views produced, the report documents beside
the records, the quickstart linking every record. The main page
gains its section teaching the views, with a working example, and
the doxygen documents the public header.

**Why this priority**: the evidence is what a later lane reads, and
the docs gates bind every feature; both stand after the views work.

**Independent Test**: the results directory carries one record per
run with the report documents beside it and the quickstart links
every record; the example builds, runs and prints; the drift gates
read no finding.

**Acceptance Scenarios**:

1. **Given** the acceptance runs execute on thrawn, **When** the
   lane closes, **Then** each record names the commands, the exit
   codes, the report and the views, and the quickstart links every
   record.
2. **Given** the main-page section, **When** the drift gates run,
   **Then** the section stands equal to the region it quotes and the
   rows name the public header.

### Edge Cases

- A benchmark with one repetition row: the histogram answers
  `not available` with the sample count named; the plot renders the
  single point.
- A column whose recorded values all stand equal: the histogram
  renders one bin covering the value.
- A benchmark name or metric label the document does not carry: the
  driver refuses one line, exit status 2.
- A document the reader refuses: one line naming the reader's fault,
  exit status 2, before any view output.
- An identity field the document does not carry: the page renders
  `not available` in its place.
- A metric label carrying a markup or escape character: the page and
  the SVG escape it.
- A column name that names a user counter: one refusal line, exit
  status 2, the counter columns standing outside the selector this
  spec fixes.
- A document with zero benchmarks: the driver refuses one line,
  exit status 2.

## Requirements *(mandatory)*

### The library surface (A)

- **FR-001**: The library SHALL expose the view functions behind one
  public header, `include/speedgun-ng/views.hpp`; the
  implementation SHALL live under `source/views/`, and the harness
  CLI SHALL stand byte-identical to master (R-1, R-2, D-2).
- **FR-002**: The project version SHALL stand at `0.25.0` with the
  harness `SOVERSION` at `16` and the archive `SOVERSION` at `4`,
  and the commit that moves the numbers SHALL record the ABI check
  (D-2).
- **FR-003**: The report schema SHALL stay at `1.8`; the views SHALL
  add no key to the document and the reader SHALL carry no new key
  (R-4, D-4).
- **FR-004**: Each view function SHALL take the loaded report value
  `loadReport` answered, the benchmark name, the column selector and
  the view's own arguments; no view function SHALL read a file,
  write a file, read a clock, throw to the caller or abort (R-1,
  D-10).
- **FR-005**: Each view function SHALL answer
  `std::expected<std::string, std::string>`; the value carries the
  view bytes, the error names the input the document does not carry
  or the record the run left absent, the standing `not available`
  shape (D-10).
- **FR-006**: The column selector SHALL read `time` for the primary
  time column, `cpu` for the CPU-time column, and a metric label of
  the benchmark for a metric column; the views SHALL render no
  figure outside the columns the document records (R-4). A user
  counter label SHALL stand outside the selector this spec fixes:
  the driver refuses it one line (FR-015), and a later spec extends
  the selector. The time columns SHALL print scaled by the
  `timeUnit` the benchmark records, the unit named beside the
  values; a metric column SHALL print as stored.

### The histogram (B)

- **FR-007**: The histogram SHALL render one column's distribution
  over one benchmark from the repetition rows the document records;
  the bins SHALL cover the recorded value range, the bin count SHALL
  be the function's argument with the driver passing `20`, and the
  bars SHALL stand proportional to the counts (D-6).
- **FR-008**: A benchmark with fewer than two recorded values for
  the column SHALL answer the `not available` error naming the
  sample count; the histogram SHALL invent no sample (R-4, D-6).

### The plot (C)

- **FR-009**: The plot SHALL render one column's values across the
  benchmark's repetition rows in document order as one SVG polyline
  with labeled axes; a multipass run's rows stand the passes in
  order (D-7).
- **FR-010**: The plot SHALL answer `not available` when the
  benchmark records no repetition row; a single recorded value
  renders one point (D-7).

### The page (D)

- **FR-011**: The page SHALL render one static HTML document
  carrying the run's identity as the document records it (the
  `speedgun` version, the `schemaVersion`, the benchmark name, the
  column) and the histogram and the plot views of the request, the
  text escaped, and no script (D-8).
- **FR-012**: An identity field the document does not carry SHALL
  render `not available`; a figure the page embeds that answers the
  disclosure SHALL render the disclosure text in the figure's place;
  the page SHALL invent no field and SHALL answer the value form
  (R-4, FR-005).

### The text views (E)

- **FR-013**: The text view SHALL render the same figures the
  graphic views render as terminal text: the histogram as one line
  per bin naming the range and the count with a proportional bar,
  and the plot as one line per recorded value in row order (D-6,
  D-7).

### The driver (F)

- **FR-014**: The driver SHALL stand at `tools/report_view.cpp` as
  one thin program naming its arguments, the report file, the
  benchmark, the metric and the view; it SHALL print the view on
  standard output and exit 0 (R-2, D-9).
- **FR-015**: The driver SHALL refuse one line on standard error
  with exit status 2 before any view output: a document `loadReport`
  refuses, the reader's error text; a name the document does not
  carry, the benchmark, the column or the view word; a missing or
  repeated required argument (the standing refusal shape). The
  driver SHALL check the request against the loaded document (the
  benchmark name, the column selector, the view word) before it
  calls the view function, so a failed check refuses and every
  error a view function answers rides FR-016's disclosure shape
  (D-10).
- **FR-016**: A view that answers the `not available` disclosure
  SHALL print the disclosure on standard output and exit 0 (D-9).
- **FR-017**: `--help` SHALL print one line per argument row on
  standard output with exit status 0, and the harness CLI SHALL
  stand byte-identical to master (D-9, FR-001).

### The seams, determinism and gates (G)

- **FR-018**: The tests SHALL render from documents the fixtures
  build, the suite SHALL pass under `taskset -c 0,1` and on a 2-cpu
  host, no test SHALL sleep or read a clock, and the renderer SHALL
  carry no timing figure (the acceptance shape).
- **FR-019**: Coverage SHALL stand at 100% line and 100% branch with
  no `LCOV_EXCL` marker and no new `--ignore-errors` entry (VI).
- **FR-020**: Every public interface SHALL carry its doxygen
  contracts (`\pre`, `\post`), the implementation SHALL enforce
  them through the project contract facility, and the public header
  SHALL stand documented in the `docs` target (II, IV).

### The evidence and the docs (H)

- **FR-021**: The acceptance runs SHALL render the reports of
  specs/035 and specs/036 on thrawn, one record per run under
  `specs/038-report-view-generators/results/`, each record naming the
  commands, the exit codes, the report it rendered and the views it
  produced; the lane SHALL record the report documents beside the
  records, and the quickstart SHALL link every record. A script
  stands behind the runs; no CI gate carries them (R-5, D-5).
- **FR-022**: The main page SHALL gain its section teaching the
  views: one purpose sentence, one marked snippet from an
  `example/` region a target builds and runs, one result sentence,
  and the quick-reference rows naming the public header; the page
  and the region SHALL move in the same change (IV.1).

## Key Entities *(include if feature involves data)*

- **ReportData**: the loaded document value `loadReport` answers;
  the whole input surface, read once, carried by reference.
- **View request**: the benchmark name, the column selector and the
  view's own arguments (the histogram's bin count), the names the
  driver's arguments carry.
- **View answer**: the view bytes on the value form, the disclosure
  naming the absent input on the error form.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: With fixture-built documents, the histogram, the
  plot, the page and the text views render, each carrying only
  figures the document records; every absent record answers the
  `not available` disclosure naming its input (FR-005 to FR-013).
- **SC-002**: The evidence records stand under the results
  directory for the reports of specs/035 and specs/036, each naming
  the commands, the exit codes, the report and the views, and the
  quickstart links every record (FR-021).
- **SC-003**: Every refusal of FR-015 prints one line and exits 2
  with no view output ahead of it; every disclosure of FR-016 exits
  0 (FR-015, FR-016).
- **SC-004**: The harness CLI stands byte-identical to master: the
  option-table rows and the value blocks stand unchanged, the block
  `298` to `319` free (FR-001, FR-017).
- **SC-005**: The coverage gate reads 100.0% lines and 100.0%
  branches, with no `LCOV_EXCL` marker and no new `--ignore-errors`
  entry (FR-019).
- **SC-006**: The suite passes under the sanitizer and tsan gates
  the matrix carries, and under `taskset -c 0,1` (FR-018).
- **SC-007**: The docs gates stand green: the example builds and
  runs, the snippet compiles, the drift gates read no finding, and
  the prose gate reads no finding (FR-022).

## Assumptions

- The reader loads the schema minors the merged lanes recorded
  (`1.5` through `1.8`); the views render whatever the reader
  answers, and each evidence record names the schema the document
  carries.
- The SVG stands hand-rolled text: the views write the elements, the
  attributes and the labels themselves; no raster backend joins
  (D-3).
- The driver prints the disclosure of an absent record with exit
  status 0; a name the document does not carry is a request fault
  and refuses with status 2 (D-9, FR-015, FR-016).

## Constitution Checklist

- Main-page impact check (IV.1): the views join the public API and
  the driver joins the tools a user runs, so the page gains its
  section and the example region moves in the same change (FR-022).
- Coverage (VI): 100% line and branch coverage, no exclusions
  (FR-019).
- Performance (VII): the views stand off the timed path: they read
  no clock and touch no measurement; no baseline moves.
- Prose (XI): every generated line clears the prose gate; the code
  spans stand on one line.
