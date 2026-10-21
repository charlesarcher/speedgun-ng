# Contract: the jitter objects in the report

Phase 1 output. What the JSON document and its schema gain, the
`--json-raw=gaps` condition, and the round trip through `loadReport`
(FR-013, FR-025, FR-026 to FR-029, D-8, R-09).

## Key style

The new keys take camelCase, the style every key the document carries
today takes (`docs/schema/report-v1.json`); the schema checker compares
key set, key order and value kinds (`source/harness/json_report.hpp:186`
area), so the schema rows and the writer move in the same commit (R-09).
The document stays version 1: the reader accepts every 1.x, and no
version bump rides the addition (R-09).

## The run-level `jitter` object

The key joins the root `x-order` after `speedgun` and before
`benchmarks`, in `properties` and out of `required`, so a run with no
probe writes no key and the document carries the same fields and shape
as master's (FR-013, R-09). The writer writes the key exactly when the
run carried a probe (FR-026). A run that requested the probe and found the TSC unavailable carried a
probe in this sense: the writer writes the key with `mode` as
requested, the configured `thresholdNs`, `loopPeriodNs` 0, the empty
`cpus` and `stats`, `droppedGaps` 0, and the unavailability disclosure
E-07 states (FR-037).

| Key | Kind | Content | Requirement |
| --- | --- | --- | --- |
| `mode` | string, one of `ALONGSIDE`, `PRECHECK`, `BOTH` | the mode the run used, the words the `JitterProbeMode` enumerators name | FR-026, R-10 |
| `thresholdNs` | integer | the effective threshold in ns | FR-007, FR-026 |
| `loopPeriodNs` | integer | the minimum loop period the probe measured | FR-007, FR-026 |
| `cpus` | array of integer | the probe cpu indexes, ascending | FR-026 |
| `stats` | array of objects | one object per probe cpu: `cpu` and the sixteen fields of `JitterStats` in the E-03 order | FR-002, FR-026 |
| `droppedGaps` | integer | the gaps the buffers dropped, summed over the probe cpus | FR-010, D-6, FR-026 |
| `disclosure` | string | the disclosure text of the run, the record data-model.md E-09 canonically carries | FR-025, D-8, FR-026 |

The `disclosure` text carries the facts FR-025 lists; the wording stays
unpinned, and the schema checks the key's presence and kind.

## The per-benchmark `jitter` entry

The key joins the entry `x-order` directly after `noiseWarning`, in
`properties` and out of `required`, the optional-key pattern
`contextPercentiles` and `raw` use (R-09). The kind is
`["object", "null"]`, the shape `noiseWarning` carries.

| State of the benchmark's windows | Value | Requirement |
| --- | --- | --- |
| The run carried a probe and kept gaps landed inside the windows | an object holding `stats`, one object per probe cpu over those windows: `cpu` and the sixteen fields in the E-03 order | FR-021, FR-027 |
| The run carried a probe and no kept gap landed inside the windows | `null` | FR-027, User Story 3 |
| The run carried no probe | the key stands absent | FR-013 |

A multi-pass run gives each pass its own timed windows, and the split
covers all of them, so one benchmark's `stats` fold the gaps of every
pass (FR-022, PC-5).

## The `--json-raw=gaps` condition

| Rule | Requirement |
| --- | --- |
| No gap list appears in the document unless `--json-raw=gaps` is given. | FR-028 |
| Under `--json-raw=gaps`, each per-cpu stats object gains `gaps`: an array of the kept records, each `{"stamp": integer, "gapNs": integer}`, the 16-byte gap record's two fields (D-6). | FR-028, D-6 |
| The list holds only the kept gaps; the gaps the buffer dropped carry no stamps the split can place, and `droppedGaps` counts them (D-6). | FR-010 |

## Schema deltas (`docs/schema/report-v1.json`)

| Delta | Detail | Requirement |
| --- | --- | --- |
| Root `x-order` | `jitter` between `speedgun` and `benchmarks`; in `properties`, out of `required` | FR-026, R-09 |
| Root `jitter` schema | `x-order` `mode`, `thresholdNs`, `loopPeriodNs`, `cpus`, `stats`, `droppedGaps`, `disclosure`; a closed three-word enum on `mode`; `additionalProperties: false` | FR-026 |
| Entry `x-order` | `jitter` after `noiseWarning`; in `properties`, out of `required`; kind `["object", "null"]` | FR-027, R-09 |
| Per-cpu stats object schema | `cpu` plus the sixteen fields in the E-03 order, integer or real per the `JitterStats` kinds; `additionalProperties: false`; the `gaps` array allowed only under `--json-raw=gaps` | FR-002, FR-028 |

## The `loadReport` round trip

```text
loadReport(const std::string& path) -> std::expected<ReportData, std::string>
  \pre `path` is not empty (the standing `SG_REQUIRE`,
       `source/harness/json_load.cpp:1104`)
  \post a document the run wrote reads back with every `jitter` field
        equal to the value the run wrote (FR-029, SC-001)
  \post the jitter keys of a probe-free document stand absent, and the
        read `ReportData` carries no jitter stats (FR-013)
  \post an absent `jitter` key reads as no stats, the optional-key
        pattern of `contextPercentiles` and `raw`
        (`source/harness/json_load.cpp:90`)
```

Error surface: the reader answers `std::unexpected` with a message in
the shape `<path-part>: <simdjson error_message(code)>`, the DOM
error-code form, and no throwing getter (spec Constraints, FR-029,
`source/harness/json_load.cpp:93`). One test per error code: the
standing rule of FR-029 extends to the new keys, so the round-trip suite
carries one test per simdjson error code the reader answers on the
jitter keys; each test plants the wrong shape the code answers and
asserts the message names the field path. The codes include
`NO_SUCH_FIELD` for the absent key, `INCORRECT_TYPE` for a wrong kind,
and the parse code for a file that holds no JSON the parser reads
(`source/harness/json_load.cpp:1093`).

The writer extends the `JsonObjectWriter` scopes of `writeReport` with
`builder::string_builder`, and the reader extends `readEntry`/`readRow`
with the same optional-key pattern the standing keys use (R-09, spec
Constraints).

## Console parity

The JSON carries the values the console probe block prints, one column
per probe cpu in the E-03 field order (FR-023, FR-026). The
per-benchmark attribution line names the worst cpu the per-benchmark
`stats` carry, and its grammar is the one code span:

`jitter: worst cpu <n> gaps=<k> max=<ns> total=<percent>` (FR-024,
FR-027).

## Consumers

- `test/source/harness_jitter_window_test.cpp`: the JSON round trip with
  equal values, one test per reader error code, the `--json-raw=gaps`
  list, and the null entry on a benchmark whose windows held no kept gap
  (FR-026 to FR-029).
- `report_shape` keeps every registered comparison folding equal on
every probe-free binary (FR-013).
