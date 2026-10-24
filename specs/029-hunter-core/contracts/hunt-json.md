# Contract: the two JSON hunt objects

**Feature**: specs/029-hunter-core | **Files**: `source/harness/json_report.cpp`,
`source/harness/json_load.cpp` | **Audit point**: `bd10d76`
**Spec**: FR-050 to FR-053, D-17, hard rules 7, 8 and 11

## What this contract covers

One new top-level object named `hunt` and one new per-entry object named `hunt`.
Both are written by the existing `simdjson` writer and read by the existing
reader, so no new parser, no new writer and no new dependency appears
(Additional Constraints, hard rule 8). The `hunt` keys join the `x-order` and the
`properties` lists of the schema and stay out of both `required` lists, which is
the exception D-17 records; the rule and its consequence are in
[report-schema-key.md](report-schema-key.md).

## Where the keys go

| Position | At `bd10d76` | After this feature |
|---|---|---|
| root | `schemaVersion`, `speedgun`, `benchmarks` (`json_report.cpp:583-598`) | those three, then `hunt` |
| entry | `...`, `metricMethod`, `passCount`, `passSpread`, `spreadWarning` (`json_report.cpp:500-510`) | those four, then `hunt` |
| `schemaVersion` value | `1.1` (`json_report.cpp:31`) | `1.4` |

Key order stays first-appearance: both `hunt` keys are appended, and the bytes a
run wrote before stay in place. A run with no hunt writes no `hunt` key at
either level, so its document differs from the master capture in the
`schemaVersion` value alone (FR-052, AC-6, SC-006).

## The top-level object

```json
"hunt": {
  "selection": "all",
  "norm": "instr",
  "rank": "rate",
  "top": 20,
  "noisePercent": 10.0,
  "repeats": 3,
  "seed": 0,
  "budget": "120s",
  "passes": 14,
  "method": "PASSES",
  "events": ["cpu/instructions_retired_any", "cpu/l2_lines_out.any_core"],
  "stages": [
    { "first": 0, "count": 42, "repeats": 1 },
    { "first": 0, "count": 42, "repeats": 3 }
  ],
  "estimate_ns": 1260000,
  "planned_events": 42,
  "ran_events": 40,
  "stage_two_events": 42
}
```

| Key | Type | Absent when |
|---|---|---|
| `selection` | string | never, once `hunt` is present |
| `norm`, `rank` | string, one of `instr`, `cycle`, `iter` and `rate`, `cv` | never |
| `top`, `repeats`, `passes`, `planned_events`, `ran_events`, `stage_two_events` | integer | never |
| `noisePercent` | number | never |
| `seed` | integer | never |
| `budget` | string such as `120s`, or null | a run with no budget writes null |
| `method` | string, the settled `MetricMethod` word in the spelling `metricMethod` already uses (`json_report.cpp:586`, `kMetricMethodWords`) | never |
| `events` | array of strings | never; every planned address in plan order (D-17, FR-050); the example shows two of 42 for space |
| `stages` | array of closed objects `first`, `count`, `repeats` | never; one object per stage that ran: its first index into `events`, its event count, its repeats; the exact membership of a stage-2 subset stays visible in the rank column of `entries` |
| `estimate_ns` | integer, or null | a plan with no calibration estimate writes null |

## The per-entry object

```json
"hunt": {
  "method": "PASSES",
  "pass_count": 14,
  "top": 20,
  "retried_passes": 1,
  "entries": [
    {
      "address": "cpu/instructions_retired_any",
      "rate": 1042.3,
      "cv": 1.2,
      "noisy": false,
      "multiplexed": false,
      "rank": 1
    },
    {
      "address": "cpu/mem_load_stalled_on_l2",
      "rate": 2.11,
      "cv": 24.8,
      "noisy": true,
      "multiplexed": false,
      "rank": null
    }
  ],
  "skipped": [
    { "address": "cpu/l1_dc_misses", "kind": "permission-blocked" },
    { "address": "armv8_pmuv3/cycle", "kind": "not-encodable" }
  ]
}
```

| Key | Type | Note |
|---|---|---|
| `method` | string | the word of the settled method, the same spelling as `metricMethod` |
| `pass_count` | integer | the pass count of the run, the `passCount` of `BenchmarkResult` re-stated for the hunt plan that ran |
| `top` | integer | the `--hunt-top` value the run used |
| `retried_passes` | integer | passes that fell back to one event per pass (D-18) |
| `entries` | array | one object per planned column, including a column only stage 1 measured, whose record is built from its stage 1 samples; in the printed order: the ranked ones first, then the excluded ones in address order |
| `entries[].address` | string | the catalog address, `Object::path()` plus `/` plus the entry name (spec PC-1) |
| `entries[].rate` | number | the median normalized rate, in the unit `norm` states |
| `entries[].cv` | number | the coefficient of variation as a percent: one hundred times the `coefficientOfVariation` of `statisticsOf`; the printed `cv=` column shows the ratio. `0.0` for a single sample (PD-14) |
| `entries[].noisy` | boolean | true when `cv` is above the threshold, read on the unrounded value |
| `entries[].multiplexed` | boolean | true when the plan settled `MULTIPLEX` (D-19) |
| `entries[].rank` | integer, or null | null for a noisy event, a single-sample event and an event outside the top N (D-7) |
| `skipped` | array | one object per event whose open was refused or whose window disclosed a non-`COUNTABLE` availability; an event that measured and gapped stands here and in `entries` (D-10) |
| `skipped[].address` | string | the address the run dropped |
| `skipped[].kind` | string | `permission-blocked`, `not-encodable`, `absent`, `scope-refused`, `gap`, the console spelling of `SkippedKind`. The record holds the address and the kind: the stage an event stood in belongs to the plan, and `hunt.stages` states the ranges one run measured (T123) |

`RankedEvent::samples` and `repeated` stay out of the document: D-17 names the
six entry keys, and the console derives the `unrepeated` mark from `samples`.

## Writer rules

- The objects go through the existing `Builder` with the RAII helpers
  `JsonObjectWriter` and `JsonArrayWriter` (`json_report.cpp:43-78`), reached
  through the `put`, `putOrNull` and `enumWord` helpers the 023 and 024 keys
  already use. No raw string assembly appears.
- `writeHunt(builder, const RunHunt&)` is called from `writeReport` after the
  `benchmarks` array; `writeEntryHunt(builder, const EntryHunt&)` is the last
  thing `writeEntry` does. Both stand inside the same `SG_ENSURE` fold the
  writer already holds, so a failed buffer is handled the way 023 handles it and
  adds no new branch.
- A `std::nullopt` estimate or budget writes `null` through `putOrNull`, the
  path `passSpread` already takes (`json_report.cpp:589`).
- `writeReport` takes `const ReportData&` (`json_report.cpp:726`), so the run
  data reaches the writer with no new parameter: `ReportData::hunt` carries it
  (PD-10).
- No `LCOV_EXCL` marker joins either file (FR-091).

## Reader rules

- `readEntry` (`json_load.cpp:1012`) reads the entry object with
  `optionalField(value, "hunt")` (`json_load.cpp:107`): an absent key leaves
  `BenchmarkResult::hunt` empty and the rest of the entry reads as it does
  today, and a present key keeps its own kind and its own refusal. `loadReport`
  (`json_load.cpp:1561`) does the same for the top-level key before it reads the
  `benchmarks` array.
- Inside a present hunt object every key is required, so a key that stands
  missing is a refusal naming the pointer, the way the 024 record reads do
  through `settled` and `missing` (`json_load.cpp:81-118`).
- Every value read goes through the `.get(out)` error-code form, and one test
  per refused code exists (hard rule 7): a number where a string stands, a
  string where a number stands, a boolean where a number stands, an unknown
  word for `method` or `norm` or `rank` or `kind`, an integer too wide for the
  field, an array where an object stands, and a missing key of a present object.
- The schema-major gate is untouched: `detail::schemaMajor` inside
  `loadReport` refuses a text that states no major or a major other than 1
  before any hunt key is read (`json_load.cpp:1585-1594`).
- Unknown keys stay tolerated by the reader (spec PC-5), and the schema checker
  reports them for the closed hunt objects.

## Forward and backward reading

| Document | Reader | Result |
|---|---|---|
| written by master, schema `1.1`, no hunt keys | this build | loads; both hunt members stand empty |
| written by this build without `--hunt`, schema `1.4` | master | loads: the root is open and the entry object carries no key master has never seen, because a run with no hunt writes no `hunt` key |
| written by this build with `--hunt`, schema `1.4` | master | loads and ignores both hunt objects (spec PC-5); `checkReport` of this build reports the same document as conforming, which is why the keys join `properties` |
| written by this build with `--hunt`, schema `1.4` | this build | round-trips: every field of both records reads back equal to what the run produced (FR-051, AC-6) |

## Verification

| Behaviour | Test |
|---|---|
| A hunt run's document holds both objects with every key of the tables above, in the `x-order` of the schema | `test/source/harness_hunt_json_test.cpp` |
| Round trip through `loadReport`, field by field, including a rank that is null and a budget that is null | `harness_hunt_json_test.cpp` |
| One case per refused reader code, naming the pointer | `harness_hunt_json_test.cpp` |
| A two-stage document: `stages` holds two objects, `events` holds the planned addresses, and `entries` covers a stage-1-only address through its stage-1-built record; the round trip reads all of it back equal | `harness_hunt_json_test.cpp` |
| A document with no hunt keys loads with both members empty | `harness_hunt_json_test.cpp`, existing `harness_json_load_test.cpp` passes unchanged |
| Schema conformance of a hunt document through `SG_SCHEMA_PATH` | `harness_hunt_json_test.cpp`, `harness_json_schema_test.cpp` |
| A run with no `--hunt` writes bytes identical to the master capture apart from `schemaVersion`, prints the same console, and exits the same | `harness_hunt_json_test.cpp`, `harness_hunt_report_test.cpp`, existing `harness_json_report_test.cpp` passes unchanged |
| The ranked values equal the printed table | `harness_hunt_report_test.cpp` against `harness_hunt_json_test.cpp` fixtures |

Every case above runs against the scripted provider, so the tests need no PMU
access, and none of them sleeps or reads a clock (FR-092).
