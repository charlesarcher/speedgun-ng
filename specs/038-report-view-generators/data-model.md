# Data model: the report view generators (specs/038)

The views stand on the record types the document schema names; this
file records the shapes the feature adds and the fields it reads.

## The types the feature adds

- **`sg::ViewSeries`**: the resolved series one view renders.
  Public data members (N-12): `values`, the recorded values of the
  column in document order with the unavailable rows filtered out
  and the time columns scaled per A-5 of
  [contracts/views-api.md](./contracts/views-api.md); `unit`, the
  label the values print beside.
- **The view request**: the benchmark name and the column selector
  as `std::string_view`, plus the view's own argument (`binCount`
  for the histogram, the page and the text view). No new request
  type: the arguments stay plain, and the resolution stands in
  `viewSeries`.
- **The view answer**: `std::expected<std::string, std::string>`;
  the value form carries the view bytes, the error form the
  disclosure. No new answer type.

## The standing types the views read

- `sg::ReportData` (`include/speedgun-ng/benchmark.hpp:792`):
  `results` carries one `BenchmarkResult` per benchmark in document
  order; `schemaVersion` and `speedgunVersion` carry the identity
  the page prints.
- `sg::BenchmarkResult` (`:627`): `name` matches the request's
  benchmark; `metricLabels` names the metric columns; `timeUnit`
  scales the two time columns; `rows` carries the rows in document
  order.
- `sg::ResultRow` (`:383`): `kind` of `RowKind::REPETITION` joins
  the sample set and the series, `RowKind::AGGREGATE` stands out
  (D-7); `timePerIterationNs` and `cpuTimePerIterationNs` are the
  two time columns; `metrics` carries one `MetricValue` per label
  in `metricLabels` order; `timeAvailability` gates the time
  columns and `MetricValue::availability` the metric column (A-4).
- The run-level records (`jitter`, `precheck`, `hunt`) stand beside
  the benchmarks; this feature's views read none of them: no user
  story names a figure from those records, and the lane prompt's
  figures stand the benchmark columns. A later spec extends the
  figure set (X.2).

## The column resolution

| Selector | Column | Source per row | Unit |
| --- | --- | --- | --- |
| `time` | primary time | `timePerIterationNs` | `timeUnit` scaled |
| `cpu` | CPU time | `cpuTimePerIterationNs` | `timeUnit` scaled |
| a metric label | metric | the matching `metrics` slot | as stored; `unit` stands empty |

A metric column prints as stored and names no unit, so
`ViewSeries::unit` stands empty for a metric label; only the time
columns carry a unit label, the scale of FR-006. The settled method
stands at `BenchmarkResult::metricMethod`; the figures name no
method.

A selector matching none of the three refuses: a counter label
stands outside the selector this spec fixes (clarify 1/Q3).

## The formats

- **SVG** (the histogram, the plot): hand-rolled elements; the
  histogram one bar per bin beside its range and count label; the
  plot one polyline with labeled axes; document-derived text
  escaped (A-6).
- **HTML** (the page): one static document, UTF-8, no script; the
  identity fields, the two SVG figures inline, the disclosure
  tokens in place.
- **Text** (the text view): one line per bin with a proportional
  character bar; one line per recorded value in row order; the
  disclosure line in a disclosed part's place.
