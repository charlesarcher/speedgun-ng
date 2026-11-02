# Contract: the `method` and `series` keys of the root `jitter` object

Phase 1 output. The delta over the standing `jitter` object contract of
`specs/027-jitter-probe/contracts/report-json.md`: the closed root
`jitter` object gains two keys, `method` and `series`, and the schema
minor moves to `1.12` (spec FR-006, FR-020, D-7, D-8). The standing key
table, the `--json-raw=gaps` condition and every other row of the
standing contract stand unchanged. The standing file is amended in the
commit that lands the writer (spec FR-030).

## Key style

One sentence joins `## Key style` at line 7 to 15:

```text
The document stays version 1, and the minor moves to `1.12` for the
two keys. The schema file name stands, and the writer value stands at
`source/harness/json_report.cpp:43` (specs/043 FR-006,
specs/023-json-reports/contracts/schema-file.md:83-87).
```

The writer at `source/harness/json_report.cpp:43` reads `1.11` at the
audit point, and `docs/pages/harness.md:1399` states the minor rule.

## Key rows

Two rows join the key table after line 38, in the standing columns
`| Key | Kind | Content | Requirement |`:

| Key | Kind | Content | Requirement |
| --- | --- | --- | --- |
| `method` | string, one of `TSC`, `FWQ`, `FTQ` | the method the run used, the words the `JitterMethod` enumerators name | specs/043 FR-006, FR-019 |
| `series` | array of closed objects | one item per probe cpu, ascending, 21 keys: `cpu`, the five calibration keys, the 12 series figures, `resampleStepNs`, `spectrumPoints`, and a closed `periods` array | specs/043 FR-006, FR-011, FR-019, FR-020 |

One sentence joins after line 41:

```text
The two keys stand after `loopPeriodNs` in the object's `x-order` and
in `properties`, and out of `required`, so a `tsc` run writes the
standing keys and nothing else (specs/043 FR-006, D-7).
```

## The `series` item

The item's `x-order` list, 21 keys, closed:

```json
["cpu", "workCount", "unitTimeNs", "targetNs", "quantumNs", "calibrationSpanNs", "sampleCount", "minValue", "medianValue", "meanValue", "p90Value", "p99Value", "p999Value", "p9999Value", "maxValue", "noisePercent", "seriesRuntimeNs", "droppedSamples", "resampleStepNs", "spectrumPoints", "periods"]
```

The nested `periods` item carries two keys, closed:

```json
["periodNs", "powerRatio"]
```

- The five calibration keys stand inside the item, the record spec
  FR-011 discloses per probe cpu.
- `quantumNs` carries nanoseconds, and the option and the public field
  carry microseconds (spec FR-002, FR-015, FR-019).
- `seriesRuntimeNs` names the window span, and `runtimeNs` keeps its
  gap meaning in the standing `stats` item
  (`include/speedgun-ng/benchmark.hpp:526`), so no name stands twice in
  the rule table (spec FR-021).
- The standing `required` lists stay as they are: a `FWQ` or `FTQ` run
  writes `mode`, `cpus` and `disclosure` as today, `thresholdNs` and
  `loopPeriodNs` as the standing default rule measured them,
  `droppedGaps` 0, and one `stats` item per probe cpu with the 16 gap
  fields at 0 (spec FR-006).
- A `tsc` run writes neither key, so its document matches master apart
  from the `schemaVersion` value and the version figures the report
  carries (spec FR-006, SC-004).

## The per-benchmark row

One row joins the per-benchmark table after line 54, in the standing
columns `| State of the benchmark's windows | Value | Requirement |`:

| State of the benchmark's windows | Value | Requirement |
| --- | --- | --- |
| The run carried a probe and the windows kept a series sample | the object gains the same `series` array, folded over that benchmark's timed windows on `BenchmarkResult::jitterFolds`, with no `method` key and no `periods` key | specs/043 FR-020 |

The per-benchmark `jitter` key keeps its open shape, so no schema edit
joins there (spec FR-006). `null` stands where the windows kept no
sample, the standing kind of the key (specs/027 FR-027).

## Schema delta rows

Three rows join the schema delta table after line 75, in the standing
columns `| Delta | Detail | Requirement |`:

| Delta | Detail | Requirement |
| --- | --- | --- |
| Root `jitter` `x-order` | `method` and `series` after `loopPeriodNs`; in `properties`, out of `required`; the standing seven keys keep their `required` place | specs/043 FR-006 |
| `series` item schema | 21 keys, `additionalProperties: false`, its own `x-order` and `required`, the shape the standing `stats` item carries at `docs/schema/report-v1.json:78-117` | specs/043 FR-006 |
| `periods` item schema | `periodNs` integer, `powerRatio` real, closed, its own `x-order` and `required` | specs/043 FR-006 |

The document root's `x-order` list stands unchanged (spec FR-006). The
closed object refuses a key the schema does not list, the standing
checker path at `source/harness/json_load.cpp:2532-2543`.

## The `loadReport` line

One line joins the `loadReport` block after line 86, the block that
runs 79 to 90:

```text
\post a `series` item reads back with every calibration figure, every
      series figure, and the period list (specs/043 FR-020)
```

## Gate

- `harness_json_schema_test` reads the two keys against
  `docs/schema/report-v1.json` at the stamped minor (spec FR-006).
- `harness_jitter_window_test` reads the round trip through
  `loadReport` with equal values, and the per-benchmark fold (spec
  FR-020).
- `harness_jitter_bytes_test` reads a `tsc` document against an inline
  fixture of master's bytes, the shape
  `test/source/harness_noise_report_test.cpp:456-461` carries (spec
  FR-026, SC-004).

## Consumers

- `test/source/harness_jitter_window_test.cpp`: the written keys, the
  round trip, and the null per-benchmark entry (spec FR-006, FR-020).
- `test/source/harness_jitter_bytes_test.cpp`: the byte identity of a
  run with no `method` key (spec FR-026).
- `specs/027-jitter-probe/contracts/report-json.md` gains these rows
  in the same commit as the writer (spec FR-030).
