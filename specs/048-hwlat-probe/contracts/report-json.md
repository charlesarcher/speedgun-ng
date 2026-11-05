# Contract: the `hwlat` keys of the report

Phase 1 output. The delta over three standing contracts:
`specs/027-jitter-probe/contracts/report-json.md` for the `jitter`
object and its `stats` and `gaps` items,
`specs/034-host-noise-precheck/contracts/report-json.md` for
`precheck.cpus[].stats`, and
`specs/036-osnoise-timerlat-tracers/contracts/report-json.md` for the
`tracer` object. The schema minor moves to `1.15`. The top-level
`x-order` stands unchanged, because no top-level key joins (spec
FR-004, FR-029, D-6, D-13). The standing contracts' key tables stand
unchanged apart from the rows this file names, and each amendment joins
its standing file in the commit that lands the writer (spec FR-029).

## Schema version

| Rule | Detail | Requirement |
| --- | --- | --- |
| The stamp moves to `1.15` | `kSchemaVersion` stands `"1.14"` at `source/harness/json_report.cpp:49`, and the writer stamps the new value | spec D-5, FR-004 |
| The top level stands still | `docs/schema/report-v1.json:7` keeps its eight-name `x-order`, because every new key joins an object the document already carries | spec FR-029 |
| Each touched object moves in the same commit | the six `x-order` lists below move with the writer, the habit specs/036 FR-006 states | spec FR-029 |

## Key order

Six `x-order` lists move. Each line below states the list after the
move, with the standing line's file and line beside it.

| Object and line | `x-order` after the move |
| --- | --- |
| `jitter`, `docs/schema/report-v1.json:47` | `["mode", "thresholdNs", "loopPeriodNs", "method", "series", "cpus", "stats", "droppedGaps", "disclosure", "limits"]` |
| `jitter.stats[]`, `:170` | `["cpu", "gapCount", "gapsPerSecond", "minNs", "medianNs", "meanNs", "p90Ns", "p99Ns", "p999Ns", "p9999Ns", "maxNs", "totalNs", "totalPercent", "runtimeNs", "thresholdNs", "loopPeriodNs", "droppedGaps", "smiDelta", "nmiDelta", "smiGapCount", "gaps"]` |
| `gaps[]`, `:232` | `["stamp", "gapNs", "smi"]` |
| `precheck.cpus[].stats`, `:268` | the sixteen standing names, then `"smiDelta", "nmiDelta", "smiGapCount"` |
| `precheck.tracer`, `:367` | `["engine", "windowUs", "widthUs", "thresholdUs", "cpus"]` |
| `precheck.tracer.cpus[]`, `:376` | `["cpu", "nmi", "irq", "softirq", "thread", "hardware", "maxGapNs", "gapCount", "nmiCount"]` |

The `gaps` name keeps its place at the end of the `stats` item, because
the writer writes the array last at
`source/harness/json_report.cpp:354`. The three new `stats` names take
positions 18, 19 and 20 of that item, and `gaps` moves from 18 to 21.
The three `tracer` settings take positions 2, 3 and 4, beside the
engine word, and `cpus` moves from 2 to 5.

## Optionality

| Rule | Detail | Requirement |
| --- | --- | --- |
| No new name joins a `required` list | the six lists at `docs/schema/report-v1.json` lines 47, 170, 232, 268, 367 and 376 keep their standing names, and each new key joins `properties` only | specs/036 FR-014, spec FR-029 |
| Every object stays closed | `additionalProperties` stays `false` on each touched object, so a key the schema lists nowhere refuses with the pointer of the item that carries it | specs/036 FR-006 |
| The writer's order is the schema's order | the checker compares the key set, the key order and the kinds of every written document, at `source/harness/json_load.cpp:2996` | spec FR-029 |

## The `jitter` object keys

| Key | Kind | Written when | `null` rule | Requirement |
| --- | --- | --- | --- | --- |
| `method` | string, one of `FWQ`, `FTQ`, `HWLAT`, `TSC` | the run carried a probe whose method stands apart from `TSC`, the standing gate at `source/harness/json_report.cpp:1183` | never `null` | spec FR-004, FR-029 |
| `limits` | string | the run carried `method=hwlat`, beside `disclosure` | never `null` | spec FR-031, P-13 |

The `HWLAT` word joins the enum list at
`docs/schema/report-v1.json:77`, which stands `["FWQ", "FTQ", "TSC"]`
today. The checker tests membership only
(`source/harness/json_load.cpp:3008` to `:3013`), so the position inside
an enum list carries no meaning. `limits` carries the one sentence the
console states, and a run of any other method writes neither key, so its
bytes stand where master wrote them.

## The `stats` item keys

One table serves the three carriers that share
`writeJitterCpu` at `source/harness/json_report.cpp:332`: the run-level
`jitter.stats[]` at `:1208`, the per-benchmark `jitter.stats[]` at
`:768`, and `precheck.cpus[].stats` at `:1242`.

| Key | Kind | Written when | `null` rule | Requirement |
| --- | --- | --- | --- | --- |
| `smiDelta` | integer or `null` | the run carried `method=hwlat` | `null` where the SMI count stands unavailable | spec FR-025 |
| `nmiDelta` | integer or `null` | the run carried `method=hwlat` | `null` where the NMI count stands unavailable | spec FR-025 |
| `smiGapCount` | integer | the run carried `method=hwlat` | never `null`; 0 answers no marked gap | spec FR-025 |

A run of `tsc`, `fwq` or `ftq` writes none of the three in any of the
three carriers (spec FR-025). The per-benchmark fold covers one
benchmark's windows and holds no window edge, so its two delta figures
stand `null` for a `hwlat` run and its count figure stands 0. The spec's
key table names two carriers; the third joins it in the same commit,
because one writer fills all three.

## The `gaps[]` item key

| Key | Kind | Written when | `null` rule | Requirement |
| --- | --- | --- | --- | --- |
| `smi` | string or `null`, one of `SMI`, `NOT_SMI` | the run carried `method=hwlat` under the standing `--json-raw=gaps` gate at `source/harness/json_report.cpp:354` | `null` where the SMI count stands unavailable | spec FR-026 |

`loadReport` validates the kind and keeps no record of it, the standing
shape of the two names it reads beside it at
`source/harness/json_load.cpp:516` to `:542`. A value of another kind
refuses, and an absent key refuses, the way `stamp` and `gapNs` do.

## The `tracer` object keys

| Key | Kind | Written when | `null` rule | Requirement |
| --- | --- | --- | --- | --- |
| `windowUs` | integer | the engine stands `hwlat` | never `null` | spec FR-027 |
| `widthUs` | integer | the engine stands `hwlat` | never `null` | spec FR-027 |
| `thresholdUs` | integer | the engine stands `hwlat` | never `null` | spec FR-027 |

The three names carry the values FR-020 names in microseconds: 10000,
5000 and 1000 at A-3. The `engine` key takes the third word, `hwlat`,
and its enum list at `docs/schema/report-v1.json:371` moves from two
names to three.

## A `cpus` entry

| Key | Kind | Written when | `null` rule | Requirement |
| --- | --- | --- | --- | --- |
| `maxGapNs` | integer or `null` | the engine stands `hwlat` | `null` where the buffer of that cpu holds no detector line | spec FR-027 |
| `gapCount` | integer or `null` | the engine stands `hwlat` | `null` where the buffer of that cpu holds no detector line | spec FR-027 |
| `nmiCount` | integer or `null` | the engine stands `hwlat` | `null` where the buffer of that cpu holds no detector line | spec FR-027 |

Under the `per-cpu` mode the run writes (spec D-9, P-14), each tracer
cpu carries its own detector thread, so a figure of 0 answers a cpu the
detector watched and found nothing on. `null` stands for a buffer that
states nothing, the file form of the console's `not available`.

## The enum word sets

The document's method words, the table at
`source/harness/detail/json_report.hpp:89`:

```text
TSC, FWQ, FTQ, HWLAT
```

The document's engine words, the table at
`source/harness/detail/json_report.hpp:103`:

```text
NONE, osnoise, timerlat, hwlat
```

The first word of each table answers the enumerator no document names:
`TSC` writes no `method` key, and `NONE` writes no `tracer` object. Each
table closes with a `static_assert` on its enum, at `:94` and `:108`, so
a new enumerator without a word fails the build. The console spells the
method in lower case and the document in upper case, the standing rule
of specs/043 FR-019. The engine words stand lower case in both.

## Rules

- A run that names neither new word writes the bytes master writes,
  apart from the `schemaVersion` value and the version figures, the
  standing byte test of SC-006 (spec FR-029).
- An `osnoise` document loses no byte and no name: the six new `tracer`
  names ride the `hwlat` engine alone (plan P-7).
- A `tsc`, `fwq` or `ftq` document loses no byte and no name, because
  the three `stats` keys ride the `hwlat` method alone (spec FR-025).
- `loadReport` reads each new figure through the standing
  `optionalField` path at `source/harness/json_load.cpp:86`, the way the
  `tracer` key itself reads at `:1198` and the `method` word at `:1457`.
  A document that omits a key loads as the absent figure (spec A-7).
- The standing readers keep their required keys: `readJitterCpu` reads
  its sixteen names as required at `source/harness/json_load.cpp:495` to
  `:512`, and `readPrecheckTracer` reads `engine` and `cpus` as required
  at `:1089` and `:1100`. A new name joins none of those calls, so a
  document written before this feature loads with no complaint (spec
  FR-029).
- A nullable figure reads through the standing `readNullableCount` path
  at `source/harness/json_load.cpp:723`, which answers an absent figure
  for a `null` and refuses a value of another kind.
- `loadReport` answers the tracer record in `ReportData::precheck` at
  `include/speedgun-ng/benchmark.hpp:1223`, the carrier the comparison
  reads for the detector names.
- The schema checker fails a document that carries a duplicate key, a
  key a closed object does not list, or an order its `x-order` list does
  not state, the standing rule of the two contracts this file deltas.

## Standing contract lines this delta amends

| File and line | Standing text | New text |
| --- | --- | --- |
| `specs/027-jitter-probe/contracts/report-json.md:39` | the `method` kind lists `FWQ`, `FTQ`, `TSC` | the `method` kind lists `FWQ`, `FTQ`, `HWLAT`, `TSC` |
| `specs/027-jitter-probe/contracts/report-json.md:42` | `cpu` and the sixteen fields of `JitterStats` in the E-03 order | `cpu` and the nineteen names of `JitterStats` in the E-03 order, the last three written for the `hwlat` method alone |
| `specs/027-jitter-probe/contracts/report-json.md:93` | each gap item carries the stamp and the gap, the 16-byte gap record's two fields | each gap item carries the stamp, the gap, and the `smi` mark of the `hwlat` method |
| `specs/027-jitter-probe/contracts/report-json.md:103` | `cpu` plus the sixteen fields in the E-03 order | `cpu` plus the nineteen names in the E-03 order, beside the `smi` name of the `gaps` item |
| `specs/034-host-noise-precheck/contracts/report-json.md:37` | the sixteen standing stats fields of the console probe block | the nineteen standing stats fields of the console probe block |
| `specs/036-osnoise-timerlat-tracers/contracts/report-json.md:34` | the `engine` kind lists `osnoise` or `timerlat` | the `engine` kind lists `osnoise`, `timerlat` or `hwlat` |
| `specs/036-osnoise-timerlat-tracers/contracts/report-json.md:35` | the `cpus` row, the second and last row of the `tracer` object table | three rows join above it: `windowUs`, `widthUs`, `thresholdUs`, each an integer the `hwlat` run wrote |
| `specs/036-osnoise-timerlat-tracers/contracts/report-json.md:46` | the `hardware` row, the last of the five source figures | three rows join below it: `maxGapNs`, `gapCount`, `nmiCount`, each integer or `null` |
| `specs/036-osnoise-timerlat-tracers/contracts/report-json.md:17` | the `precheck` order of six names, ending at `tracer` | unchanged: the new names join inside `tracer`, so the `precheck` order stands still |

## Consumers

- `test/source/harness_json_report_test.cpp`: the eleven keys, their kinds
  and their order, and the no-key rule for every other method and engine
  (spec FR-025 to FR-027).
- `test/source/harness_json_schema_test.cpp`: the six moved `x-order`
  lists, the two moved enum lists, and the closed objects (spec FR-029).
- `test/source/harness_jitter_bytes_test.cpp`: the byte identity of a
  run that names neither new word (spec FR-029, SC-006).
- `test/source/harness_json_load_test.cpp`: the round trip of the six
  figures, the kind validation of `smi`, and a document written before
  this feature (spec FR-026, FR-029, A-7).
- The three standing contracts above carry the rows in the same commit
  as the writer (spec FR-029).
