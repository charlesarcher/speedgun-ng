# Data Model: JSON Reports

**Feature**: `specs/023-json-reports/spec.md` | **Date**: 2026-10-20
**Anchors**: read at `c7f1769`

The document mirrors the result types of
`include/speedgun-ng/benchmark.hpp`. Every entity below names the C++
value it carries, the JSON keys it writes, the order of those keys, and
the invariants the writer and the reader both hold.

## E-01: `ReportData`, the value the writer consumes and the loader answers

```cpp
// include/speedgun-ng/benchmark.hpp, public (harness SOVERSION 8)
struct ReportData
{
  std::vector<BenchmarkResult> results;
  std::vector<RowRaw> rowRaw;                    // parallel to results
  std::vector<std::vector<RowRaw>> contextRaw;   // parallel to results,
                                                 // then to result.contexts
};
```

**Fields**: `results` holds one entry per instance in run order, the same
order the console prints them. `rowRaw[i]` carries the chunk columns of
the rows of `results[i]` when the raw setting asked for chunks, and is
empty when it did not. `contextRaw[i][j]` carries the chunk columns of
context `j` of a threaded result.

**Invariants**:
- `rowRaw.size() == results.size()` and `contextRaw.size() ==
  results.size()` always, so a reader indexes by result without a search.
- A single-context result has an empty `contextRaw[i]`; a threaded result
  has one entry per entry of `results[i].contexts`
  (`benchmark.hpp:488`), in context order.
- The loader answers a `ReportData` whose raw entries are empty when the
  file carries no `raw` key (FR-022).

**Lifetime**: the `Runner` fills it during the batch, `speedgunMain` hands
it to the writer once, and `loadReport` answers a fresh one.

## E-02: `RowRaw`, the chunk columns of one row or one context

```cpp
struct RowRaw
{
  std::vector<std::vector<double>> columns;  // [column][chunk]
};
```

**Fields**: one entry per reported column, in the column order of the row
it belongs to: the primary time first, then the time column the primary is
not, then each metric column, then each user counter column. Each inner
vector holds that column's chunk values in chunk order.

**Invariants**:
- `columns[c].size() == row.chunkCount` for every `c` when the run
  captured chunks (`benchmark.hpp:401`).
- A column the run could not fold carries an empty inner vector, the same
  shape `ContextFolds::chunkValues` gives it at
  `source/harness/runner.cpp:1474`.
- The values are the per-iteration values after the chunk divide
  (`divideChunkValues` at `runner.cpp:167`), the same numbers N1 ranked
  for the percentile lines. No further scaling.

**Memory**: `chunkCount * 8` bytes per column. A run with the 1% rule of
021 keeps the chunk count at or below the recorder capacity of the leaf,
so the retention adds no order of magnitude to a run the harness already
holds.

## E-03: The document header

| Key | Kind | Source |
|---|---|---|
| `schemaVersion` | string | the literal `"1.0"` |
| `speedgun` | object | E-03a |

`speedgun` holds `version` (the `SG_PROJECT_VERSION` definition) and
`buildType` (the `SG_BUILD_TYPE` definition), both strings
(`CMakeLists.txt:203-208`).

**Invariants**: `schemaVersion` is the first key of the document. The
major part is what the loader compares (E-12). No key names a date, a
time, a host or a pid (FR-005, FR-021).

## E-04: The benchmark entry, one per result

Key order, mirroring `BenchmarkResult` (`benchmark.hpp:440-502`):

| Key | Kind | Source field |
|---|---|---|
| `name` | string | the family name |
| `suite` | string | the registration suite, empty when none |
| `case` | string | the argument-family case label, empty when none |
| `outcome` | string | E-06 spelling |
| `reason` | string or null | `reason` at `:452` |
| `hideRepetitionRows` | bool | `:473` |
| `timeUnit` | string | `timeUnit` at `:480`, E-06 spelling |
| `primaryIsCpu` | bool | `:498` |
| `pauseSamplesFolded` | unsigned | `:483` |
| `percentilePoints` | array of number | `:492` |
| `contexts` | array of E-09 | `:488` |
| `noiseWarning` | object E-10 or null | the console predicate, D-11 |
| `rows` | array of E-05 | the row list |
| `metricLabels` | array of string | the metric column labels |
| `counterLabels` | array of string | the user counter labels |
| `counterStats` | array of E-08 | `counterStats` at `:463` |
| `statisticValues` | array of E-11 | `:468` |

**Invariants**: `contexts` is empty for a single-context run.
`percentilePoints` is the list the run reported, ascending and duplicate
free (021 FR-024). `noiseWarning` is null when the console printed no
warning and carries E-10 when it printed one.

## E-05: The row entry, one per row of a benchmark

Key order, mirroring `ResultRow` (`benchmark.hpp:351-404`):

| Key | Kind | Source field |
|---|---|---|
| `kind` | string | E-06 spelling |
| `label` | string or null | `:386` |
| `iterations` | unsigned | the iteration count of the row |
| `timePerIterationNs` | number | the primary time |
| `cpuTimePerIterationNs` | number or null | the CPU fold, null for a manual run |
| `endpointSamples` | unsigned | `:364` |
| `pauseSamplesFolded` | unsigned | `:368` |
| `timeAvailability` | string | `:372`, E-06 spelling |
| `metrics` | array of E-07 | `metrics` at `:374` |
| `counters` | array of number | the user counter columns |
| `counterDisclosure` | string or null | `:381` |
| `fit` | object E-11a or null | `fit` at `:390` |
| `overheadFloorNs` | number | `:391` |
| `percentiles` | array of E-06 cell | `:398` |
| `contextPercentiles` | array of array of E-06 cell, or absent | the per-context percentile blocks of the row, one block per context, the row's slice of `contextPercentiles` at `benchmark.hpp:503` |
| `contextRaw` | array of `{"columns": [[...]]}`, or absent | the chunk columns of each context of the row, the row's slice of `contextRaw` in E-02 |
| `chunkCount` | unsigned | `:401` |
| `chunkIterations` | unsigned | `:402` |
| `timeStats` | object E-08 or null | `:403` |
| `metricStats` | array of E-08 or null | `:404` |
| `raw` | `{"columns": [[...]]}` or absent | the retained chunk columns |

**Invariants**: `percentiles[c]` answers column `c` in the same order as
`metrics` and the time columns, the order `printRowPercentiles`
(`report.cpp:351`) prints. `raw` is absent when the raw setting asked for
nothing, present with `{"columns": [[...]]}` when it asked for chunks.
`chunkCount` is 0 when the run captured no chunks, and then `raw` is
absent even if the setting asked for chunks. The complexity fit entries
FR-002 names are entries of `rows` that carry the fit kinds of `RowKind`
(`benchmark.hpp:87`), not separate entries of the `benchmarks` array.

## E-06: The scalar spellings

A percentile cell is `{"available": bool, "value": number|null}`
(`PercentileValue` at `benchmark.hpp:337`, D-12).

Enum spellings follow the C++ enumerators (D-06):

| Type | Values | Anchor |
|---|---|---|
| `RunOutcome` | `MEASURED`, `SKIPPED`, `FAILED` | `benchmark.hpp:76` |
| `RowKind` | `REPETITION`, `AGGREGATE`, and the fit kinds the header names | `benchmark.hpp:87` |
| `Availability` | `COUNTABLE`, `PERMISSION_BLOCKED`, `NOT_ENCODABLE`, `ABSENT`, `SCOPE_REFUSED`, `GAP` | `counters_core.hpp:87` |
| `WaitMode` | `SPIN`, `YIELD`, `BLOCK` | `execution_context.hpp:91` |
| `TimeUnit` | `NANOSECONDS`, `MICROSECONDS`, `MILLISECONDS`, `SECONDS` | `benchmark.hpp:65` |
| `StatUnit` | `TIME`, `PERCENTAGE` | `benchmark.hpp:248` |

**Invariants**: a name table is a closed list with a `static_assert` on
the last enumerator, the shape `report.cpp:35` and `:138` already use. A
value outside the closed enumeration writes `unknown`, and the loader
refuses it as a wrong-kind field.

## E-07: The metric value

| Key | Kind | Source |
|---|---|---|
| `label` | string | the column label |
| `value` | number | `MetricValue::value` at `:213` |
| `runningRatio` | number | `:214` |
| `scaled` | bool | `:215` |
| `availability` | string | `:217`, E-06 spelling |

**Invariants**: one entry per metric column in `MetricColumn` order, the
order the console prints. A column with no fold carries its availability
and a `value` of 0, the value the row already holds.

## E-08: The statistics block

| Key | Kind | Source |
|---|---|---|
| `samples` | unsigned | `Statistics::samples` at `:232` |
| `mean` | number | `:233` |
| `median` | number | `:234` |
| `sampleStdDev` | number | `:235` |
| `coefficientOfVariation` | number | `:236` |
| `min` | number | `:237` |
| `max` | number | `:238` |

**Invariants**: the block is null for a row that carries no repetition
spread, the same rule the console prints no `time_stats` line for. Time
statistics are in nanoseconds; the loader needs no unit conversion because
the file names the unit once, at the benchmark.

## E-09: The context disclosure

| Key | Kind | Source |
|---|---|---|
| `waitMode` | string | `ContextDisclosure::waitMode` at `:421` |
| `placement` | string | the full placement text, D-11 |
| `applied` | bool | `:421` region |

**Invariants**: the order matches `contexts`. `placement` carries the
whole text the console cuts for its column, so a reader gets what the
loader gets back.

## E-10: The noise warning

| Key | Kind | Source |
|---|---|---|
| `spinning` | unsigned | the count of `SPIN` contexts of the result |
| `runnableCpus` | unsigned | `selectRunnableCpuCount` at `report.cpp:203` |
| `sharesOneCpu` | bool | two spinning contexts name one cpu index |

**Invariants**: the object is present exactly when the console printed the
warning (FR-017). `spinning` and `runnableCpus` are the two counts the
console line names, and `sharesOneCpu` is the first half of the condition
`printNoiseWarning` applies at `report.cpp:229`.

## E-11: The statistic value and the fit row

`statisticValues` entries (`StatisticValue` at `:273`): `name` string,
`unit` string of E-06, `value` number.

`fit` (`FitRow` at `:316`): `quantity` string, `curve` string,
`coefficient` number, `rmsPercent` number, `scalesWithUnit` bool,
`reason` string or null.

**Invariants**: a benchmark with no fit writes `fit` null. A `TIME`
statistic holds nanoseconds, the rule `:244-246` states.

## E-12: The version rule and the error channel

`schemaVersion` splits at the first `.` into a major and a minor part.

**Rules**:
- A major the loader does not know is an error naming the version it read
  and the versions it reads (FR-025).
- A newer minor inside a known major is accepted, and unknown keys are
  ignored on read (FR-009, FR-010).
- A missing `schemaVersion`, a non-string version, a wrong-kind field, a
  missing required key, a chunk column whose size disagrees with
  `chunkCount`, and an unreadable path are each an error whose string
  names the JSON pointer of the fault (FR-023, FR-024).

**Invariants**: no error form aborts, throws across the public call, or
leaves a partial `ReportData` to the caller (D-08).

## Entity graph

```text
ReportData
├── results[]            BenchmarkResult        E-04
│   ├── contexts[]       ContextDisclosure      E-09
│   ├── noiseWarning     NoiseWarning | null    E-10
│   ├── rows[]           ResultRow              E-05
│   │   ├── metrics[]    MetricValue            E-07
│   │   ├── percentiles[] PercentileValue cell  E-06
│   │   ├── timeStats    Statistics | null      E-08
│   │   ├── metricStats[] Statistics | null     E-08
│   │   └── fit          FitRow | null          E-11
│   ├── counterStats[]   E-08 form
│   └── statisticValues[] StatisticValue        E-11
├── rowRaw[]             RowRaw                 E-02
└── contextRaw[][]       RowRaw                 E-02
```
