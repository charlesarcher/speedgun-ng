# Contract: the views API (specs/038)

The public surface `include/speedgun-ng/views.hpp` declares; the
implementation under `source/views/` enforces. One contract stands
stated in one place: the header carries the doxygen, the source
carries the enforcement, and this file names the behavior the tests
pin. The doxygen contracts of the header govern on disagreement
(Constitution II).

## A. The shared rules

- A-1: Every function takes the loaded document value `loadReport`
  answered by const reference, the benchmark name and the column
  selector as `std::string_view`, and the view's own arguments
  (FR-004, D-10). No function reads a file, writes a file, reads a
  clock, throws to the caller or aborts.
- A-2: The value form of the answer carries the view bytes; the
  error form names the input the document does not carry or the
  record the run left absent, the standing `not available` shape
  (FR-005).
- A-3: The column selector reads `time` for the primary time
  column, `cpu` for the CPU-time column, and a metric label of the
  benchmark for a metric column; a user counter label stands
  outside the selector (FR-006).
- A-4: A row whose value stands unavailable (an `Availability`
  other than `COUNTABLE`) stands out of every sample set and
  series; a count names the rows a value stands for (D-6, D-7).
- A-5: The time columns print scaled by the `timeUnit` the
  benchmark records, the unit named beside the values; a metric
  column prints as stored (FR-006).
- A-6: A document-derived string stands escaped where the format
  can read it as markup (D-8, the escaper of M-F).

## B. The per-function contracts

### B-1 `viewSeries(report, benchmark, column)`

- Answers `std::expected<ViewSeries, std::string>`; `ViewSeries`
  carries the recorded values and the unit label.
- \pre `benchmark` and `column` name no empty string (an empty
  name is a contract violation).
- \post on the value form: `values` holds the column's recorded
  values in document order, the unavailable rows filtered out,
  scaled per A-5, and `unit` names the unit the values print in.
- \post on the error form: the benchmark name matches no result,
  or the column stands outside A-3's enumeration; the string names
  the name and the document's recorded set.
- An empty `values` answers on the value form: the column resolved,
  the run recorded no measured value for it (the views disclose,
  B-2 to B-5 decide).

### B-2 `renderHistogram(report, benchmark, column, binCount)`

- Answers the SVG histogram: one bar per bin beside its range and
  count label (FR-007, clarify 2/Q3).
- \pre `binCount` is at least 2 (a smaller count is a contract
  violation).
- \post the bins cover the recorded value range; the last bin
  stands closed on the maximum; the bars stand proportional to the
  counts; the label text names the range and the count.
- \post on the error form: fewer than two recorded values stand
  for the column, the string naming `not available` and the sample
  count (FR-008, D-6).

### B-3 `renderPlot(report, benchmark, column)`

- Answers the SVG plot: one point per recorded value in document
  order, the axes labeled with the row and the column names
  (FR-009, D-7).
- \post a multipass run's rows stand in pass order; a single
  recorded value renders one point.
- \post on the error form: the series stands empty (no repetition
  row, or no row's value stands for the column), the string naming
  the column and the sample count of zero (FR-010, clarify 2/Q2).

### B-4 `renderText(report, benchmark, column, binCount)`

- Answers the terminal text of both figures: the histogram as one
  line per bin naming the range and the count with a proportional
  character bar, the plot as one line per recorded value in row
  order (FR-013).
- \pre `binCount` is at least 2.
- \post the view composes per part: a part whose figure answers
  the disclosure renders the disclosure line in the part's place,
  and the whole answers the value form (clarify 2/Q1).

### B-5 `renderPage(report, benchmark, column, binCount)`

- Answers one static HTML document: the run's identity as the
  document records it (the `speedgun` version, the `schemaVersion`,
  the benchmark, the column), the histogram and the plot inline as
  the two SVG figures, the text escaped, and no script (FR-011,
  D-8).
- \pre `binCount` is at least 2.
- \post an identity field the document does not carry renders
  `not available` in its place; a figure that answers the
  disclosure renders the disclosure text in the figure's place;
  the page answers the value form (FR-012, clarify 1/Q4).
- \post the embedded figure bytes stand equal to the standalone
  bytes (D-3).

### B-6 the driver `report_view`

- Grammar:
  `report_view <report> --benchmark=<name> --metric=<column> --view=histogram|plot|page|text` (D-9).
- The driver loads the document, pre-checks the request against it
  (the benchmark name, the column selector, the view word), and
  calls the view function (FR-015, clarify 1/Q2).
- Exit mapping: the view bytes on standard output at status 0; the
  disclosure on standard output at status 0 (FR-016); one refusal
  line on standard error at status 2, before any view output, for
  a reader fault, a name the document does not carry, a view word
  outside the four, and a missing or repeated required argument;
  `--help` prints one line per argument on standard output at
  status 0 (FR-017).
- The harness CLI stands byte-identical to master (FR-001, FR-017).

## C. The refusal and disclosure table

| Case | Channel | Status |
| --- | --- | --- |
| `loadReport` refuses | stderr, one line, the reader's text | 2 |
| benchmark name matches no result | stderr, one line | 2 |
| column outside the selector (counter included) | stderr, one line | 2 |
| view word outside the four | stderr, one line | 2 |
| missing or repeated required argument | stderr, one line | 2 |
| histogram under two recorded values | stdout, the disclosure | 0 |
| plot over an empty series | stdout, the disclosure | 0 |
| page identity field absent | stdout, the page with the disclosure in place | 0 |
