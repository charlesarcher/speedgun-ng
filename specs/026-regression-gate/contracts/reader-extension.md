# Contract: The Reader Extension

## What changes and what does not

The load call `loadReport` (`include/speedgun-ng/benchmark.hpp:2330`)
keeps its signature, its version gate
(`source/harness/json_load.cpp:1117` to `:1128`) and its open-object
key rule (`:1000` to `:1030`). It gains three fields of `ReportData`
to fill (FR-070, D-04, E-07):

| Field | Source in the document |
| --- | --- |
| `schemaVersion` | the `schemaVersion` text the version gate already reads |
| `speedgunVersion` | the `version` text under the `speedgun` header object; empty when the document omits it |
| `noiseWarnings` | one entry per benchmark entry: the record the reader already parses at `source/harness/json_load.cpp:729` to `:743`, now stored; nothing when the entry carried no `noiseWarning` |

The report writer keeps its current behavior: it stamps its own version
texts (`source/harness/json_report.cpp:28`, `:530`) and recomputes the
warning from the result's contexts (`:400`, `:485`), so the writer
reads none of the new fields and 023's byte-identical round trip
stands. The run path leaves the fields empty.

## The record type moves

`NoiseWarning` moves from `source/harness/detail/json_report.hpp:137`
to the public header with its fields unchanged (FR-070, D-05). The
predicate `noiseWarningOf` (`source/harness/detail/json_report.hpp:154`)
stays internal and answers `std::optional<sg::NoiseWarning>`.

## Error forms

The extension reads with the DOM error-code form `x.get(out)`, never a
throwing getter, never a loop over a temporary `simdjson_result`
(FR-067). Each error code the extension can answer carries its own
test: a non-object `speedgun` header, a non-string `version`, and the
noise record's existing field checks each get a fixture case in
`harness_json_load_test`. A document that omits the `speedgun` object
entirely answers an empty `speedgunVersion` and no fault: the reader
stays lenient on a header text it only prints.

## What the extension never does

- It never rejects a document for the new fields: the version gate and
  the required-key behavior of 023 stand.
- It never rewrites a value: no unit conversion, no recomputed
  statistic, no substituted default.
- It never grows the report format: the writer emits the same bytes as
  at the base, so the pass fields N6 will add stay N6's work and this
  reader keeps accepting unknown keys inside open objects (FR-073,
  PC-4).
