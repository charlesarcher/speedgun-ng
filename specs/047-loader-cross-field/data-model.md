# Data Model: Loader Cross-Field Checks for the Pass Fields

**Feature**: `047-loader-cross-field` | **Spec**: `specs/047-loader-cross-field/spec.md`

Every entity the three checks read, its fields, the place each field comes from,
the rule that binds it, and the forms a check must leave loading. `FR-nnn`,
`D-n` and `SC-nnn` cite `spec.md`. The check sequence, the four refusal lines
and the gate commands stand in `plan.md`, and the checks in full stand in
`contracts/loader-pass-pairs.md`. The house shape comes from
`specs/042-topdown/data-model.md`.

The change stores nothing. It adds no carrier, no member, no key, no state and
no public type: every field below already stands in `BenchmarkResult`, and the
reader answers a refusal on its way past the entry. This file states no state
transition.

## Evidence

Code facts carry a `path:line` opened on branch `047-loader-cross-field` at
`e63f7a87` (`git rev-parse HEAD`), the base `b1dd7670` plus two spec commits.
Table facts carry the command that produced them. Paths stand short here:
`benchmark.hpp` is `include/speedgun-ng/benchmark.hpp`, `json_load.cpp` and
`json_report.cpp` stand in `source/harness/`, and `report-v1.json` is
`docs/schema/report-v1.json`.

| Fact | Figure | Command |
| --- | --- | --- |
| Project version | `0.30.0` | `grep -n 'VERSION 0' CMakeLists.txt` (`:59`) |
| Schema version | `1.13` | `grep -n kSchemaVersion source/harness/json_report.cpp` (`:47`) |
| The two archive numbers | harness 20, archive 5 | `grep -n 'SOVERSION' CMakeLists.txt` (`:407`, `:134`) |
| The pass keys, required | the four entry keys at `report-v1.json:517-520`, the tag `pass` at `:662` | `python3` walk of that file |
| The method words, the outcome words | `AUTO`, `PASSES`, `MULTIPLEX`; `MEASURED`, `SKIPPED`, `FAILED` | `json_report.hpp:126-131`, `report-v1.json:527-530` |
| The spread warning limit | 5.0 percent of the median pass time | `json_report.hpp:192` |
| Load test cases today | 27 functions, called from `main` | walk of `harness_json_load_test.cpp:2207-2234` |
| Branch records the three checks add | 34 lines and 42 records, every one covered | the host coverage capture at the head |

The survey of `build/logs/notes/clarify-compat.md` read every committed document
for a refused pair: 40 files, 57 entries, one refused pair at
`test/data/gate_schema_20.json:66-68`. The version gate refuses that file ahead
of the entry read (`json_load.cpp:3129-3135`), so no check reaches it.

## E-01 The report entry

One element of the root `benchmarks` array. `loadReport` reads it in `readEntry`
(`json_load.cpp:2445`) and names it `/benchmarks/<index>` (`:3230-3235`). The
eight fields, with the writer, the reader, the schema entry and the rule that
binds each; two carry no check of this feature:

| Field | Carrier | Writer | Reader | Schema | Bound by |
| --- | --- | --- | --- | --- | --- |
| `name` | `name` (`benchmark.hpp:876`) | `json_report.cpp:692` | `:2480`, `:2495` | `:476`, `:524` | no check reads it |
| `outcome` | `outcome` (`:884`) | `:695-698` | `:2483-2484`, `:2498` | `:479`, `:527-530` | the rule 1 guard |
| `metricMethod` | `metricMethod` (`:931`) | `:849-852` | `:2726-2734` | `:494`, `:517`, `:913-916` | rule 1 (FR-003) |
| `passCount` | `passCount` (`:936`) | `:853` | `:2735-2742` | `:495`, `:518`, `:917` | the partner of all three |
| `passSpread` | `passSpread` (`:942`) | `:854` | `:2744-2751` | `:496`, `:519`, `:918` | rule 2 (FR-005) |
| `spreadWarning` | no carrier | `:855` | `:2752-2763` | `:497`, `:520`, `:919` | the standing check |
| `rows` | `rows` (`:885`) | `:790-806` | `:2636-2656` | `:489`, `:512`, `:575-577` | the walk of rule 3 |
| `hunt` | `hunt` (`:970`) | `:859-862` | `:2770-2777` | `:498`, `:920` | the rule 1 guard |

The `Reader` and `Schema` columns continue the file each row opens, and `Schema`
names the entry `x-order` line and the property line. Four facts stand below:

- `spreadWarning` has no carrier. The writer answers it from `spreadWarned`
  (`source/harness/detail/json_report.hpp:202`), the reader recomputes the same
  predicate, and the read compares the two (`json_load.cpp:2758`).
- The four pass keys stand in the entry `required` list (`:517-520`), so every
  written document carries all four, and the pair rule of FR-002 governs a
  hand-edited document that lacks one. A key that stands absent leaves its member
  at the default of a run that collected no pass (`json_load.cpp:2726`, `:2735`,
  `:2744`), the reading `aDocumentPredatingThePassFieldsLoads` measures.
- The `hunt` object carries its own `method` and `pass_count` keys
  (`report-v1.json:923-924`, written at `json_report.cpp:579-584`). That pair
  stands inside the hunt record, so no check of this feature reads it.

## E-02 The metric value and its pass tag

| Field | Carrier | Writer | Reader | Schema | Bound by |
| --- | --- | --- | --- | --- | --- |
| `pass` | `MetricValue::pass` (`benchmark.hpp:251`) | `json_report.cpp:323` | `json_load.cpp:387-395` | `:654`, `:662`, `:681` | rule 3 (FR-006) |
| the row that carries it | `ResultRow::metrics` (`:407`) | `:477-485` | `:2640-2656` | `:588`, `:610`, `:644` | the walk of rule 3 |

The tag is a 0-based index of the pass that measured the column
(`source/harness/detail/pass_plan.hpp:47-49`). The merge is its only writer
(`pass_plan.cpp:322`), and the merge runs above one pass
(`source/harness/runner.cpp:3303`). A one-pass run leaves the field null, and a
null tag answers no check (D-17). A context block carries percentile columns and
raw columns alone (`json_load.cpp:1658-1689`, `:296-360`), so rule 3 walks
`result.rows` alone (FR-006).

## E-03 The pair

| Pair | The two keys | Guard, in words | Check |
| --- | --- | --- | --- |
| one | `metricMethod`, `passCount` | both keys stand, `MEASURED`, no `hunt` key | rule 1 (FR-003) |
| two | `passSpread`, `passCount` | the count stands, and the spread stands non-null | rule 2 (FR-005) |
| three | one column's `pass`, `passCount` | the count stands, and the tag stands non-null | rule 3 (FR-006) |

Presence per pair (D-17). Rule 1 counts a key present when it stands in the
entry. Rules 2 and 3 count a key present when it stands and holds a non-null
value, the reading the loader takes at `json_load.cpp:2744-2745` and `:388-389`.
A value of the wrong kind keeps the standing kind refusal ahead of every pair
check (`:135-144`, `:158-171`, `:196-211`), and a method word outside the three
names keeps the standing enum refusal (`:2728-2732`).

## E-04 The refusal

| Part | Shape | Source |
| --- | --- | --- |
| the line | `<path>: <words>` | `json_load.cpp:57-58`, `:2759-2761` |
| the path and the words | the non-count field leads, and the words name the partner | D-15 |
| the carrier | the load error of `loadReport`, one `std::string` | `:2451` |
| the reader of it | the compare mode, at status 2 | `compare.cpp:1876-1891` |

One refusal stands per document: the first fault the reader reaches stops the
read (`json_load.cpp:120-133`, measured by `firstFailingPartStopsTheFold` at
`harness_json_load_test.cpp:706-739`). The four lines stand in
`contracts/loader-pass-pairs.md`.

## E-05 The accepted form

An accepted form is a document a writer can make, the harness writer included,
and a check must leave it loading.

| Form | The fields it carries | Where it stands |
| --- | --- | --- |
| the one-pass run | `AUTO`, 0, null, false, a null tag | `pass_plan.cpp:128-130` |
| the multiplexed run | `MULTIPLEX`, 1, null, false, a null tag | `json_fixture.hpp:96-122` |
| the split run | `PASSES`, 2 or more, a number, the measuring tag | `harness_json_report_test.cpp:372-375` |
| the stopped-loop entry | the settled word, 0, null, false, a null tag | `runner.cpp:2884`, `:2889`, `:2923`, `:2928` |
| the hand-built stopped entry | `SKIPPED`, `PASSES`, 2, 3.5, tag 1 | `harness_json_load_test.cpp:181-218` |
| the hunt entry | the forced word, the published count, a `hunt` key | `cli.cpp:1788-1789` |
| the schema 1.0 document | no pass key at all | the twelve gate fixtures in `test/data` |

Each row names the place that proves the form. The scripted run of the load test
registers one metric column on an object that publishes no limit, so the planner
settles `MULTIPLEX` (`source/harness/pass_plan.cpp:131-135`). The split shapes
are written and read back at `harness_json_report_test.cpp:383-423`, the stopped
entry answers the in-loop stops at `runner.cpp:3162` and `:3168`, and the hunt
entry's document stands in `harness_hunt_cli_test.cpp:1277-1302`.

## E-06 The stopped-loop document

A report whose entry names the `SKIPPED` outcome and a reason that names why the
loop stopped (`report-v1.json:527-530`). A signal answers `interrupted by the
user (SIGINT)` (`source/harness/runner.cpp:90`, `:94-98`), and a body skip
answers its own text (`:2882-2883`).

Rule 1 does not run on it (D-14). The runner settles the method before the
warm-up and publishes a count of 0 beside `MULTIPLEX` or `PASSES` on four stop
paths (`:2884`, `:2889`, `:2923`, `:2928`), and the calibration of pass 0 inside
the pass loop answers the same pair while no fold stands (`:3162`, `:3168`).
Rules 2 and 3 need no waiver: a stopped collection publishes no spread and no tag
(`:1720-1722`), the merge is the writer of both and runs above one pass
(`source/harness/pass_plan.cpp:322`, `source/harness/runner.cpp:3303`), and
`passSpread` answers nothing below two pass times (`pass_plan.cpp:404-408`).

## E-07 The schema 1.0 document

A report whose `schemaVersion` reads `1.0` and that carries no pass key, no hunt
key and no topdown key (`json_load.cpp:3125-3135`, `docs/pages/harness.md:1395-1403`).
The reader gates the major alone, so 1.0, 1.12 and 1.13 answer the same three
rules (D-6), and each pass member stays at the default of a run that collected
no pass (`json_load.cpp:2720-2742`).

## Validation rules

| Rule | Guard | Refusal | The writer form that keeps the guard false |
| --- | --- | --- | --- |
| 1 (FR-003) | both keys stand, `MEASURED`, and no `hunt` key stands | the method line | a stopped entry names `SKIPPED`, and a hunt entry carries a `hunt` key |
| 2 (FR-005) | the count stands, and the spread stands non-null | the spread line | a stopped collection publishes no spread |
| 3 (FR-006) | the count stands, and one column's tag stands non-null | the two tag lines | the merge writes no tag below two passes |
| the pair rule (FR-002) | one key of the pair stands absent | no refusal, and the document loads | a schema 1.0 document carries no pass key |
| the standing check (FR-009) | the `spreadWarning` key stands | the standing disagreement line | the writer answers the key from the reader's own predicate |

Each refusal cell names the line of `contracts/loader-pass-pairs.md` that leads
with the pair's non-count field. The last column keeps a guard false on every
document the harness makes (`pass_plan.cpp:322`, `:404-408`).

The contract `contracts/loader-pass-pairs.md` carries the four refusal lines in
full, and this file quotes none of them.

The order inside one entry (D-16): the rows, the standing `spreadWarning` check,
rule 1, rule 2, rule 3 over the rows in order. The first fault stops the read
(D-9), so a document with a tag fault and a method fault answers the method
line, and one with a tag fault and a spread fault answers the spread line.

## Invariants

| # | Invariant | Held by |
| --- | --- | --- |
| I-1 | A check runs only when both keys of its pair stand present (D-17) | FR-002 |
| I-2 | No key of a pair joins a `required` list, so no stored document breaks | FR-004, `report-v1.json:517-520` |
| I-3 | The writer's own document answers no check, stopped loops included | FR-010, FR-011, D-10 |
| I-4 | The standing `spreadWarning` check keeps its rule, text and place | FR-009 |
| I-5 | The change stores nothing: no member, key, schema change, type, option | FR-013, D-10 |
| I-6 | Every refusal travels the standing load error path, with no new type | FR-007, D-5 |
| I-7 | Rule 3 reads every row of the entry that carries the count, and no context block | FR-006 |

## Carrier changes

None. No header changes, so no `SOVERSION` moves: the harness number stays 20 and
the archive stays 5 (`CMakeLists.txt:49-58`, D-2). The schema stays 1.13, and
`report-v1.json` lists every key of every pair already (`:517-520`, `:654-662`).
The version moves 0.30.0 to 0.30.1 at `CMakeLists.txt:59`, and the two sites that
pin it move with it (FR-015).
