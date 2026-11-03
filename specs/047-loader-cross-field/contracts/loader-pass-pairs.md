# Contract: the loader rule for the three pass-field pairs

**Feature**: specs/047-loader-cross-field | **File**: `source/harness/json_load.cpp`,
`readEntry` | **Audit point**: `e63f7a87`
**Spec**: FR-002 to FR-009, FR-011 to FR-013, D-4, D-5, D-9, D-14 to D-17, D-19

## What this contract covers

Three cross-field checks of one report entry: their guards, their refusal lines,
the order they run in, the scope of each, and the documents each must leave
loading. The reader, its refusal shapes and the schema key sets stand in specs/023
and `docs/pages/harness.md`. The law stands in specs/024 FR-038 as this feature
amends it (FR-001, D-8).

## The pair rule

A check of a pair runs only when both keys of that pair stand present (FR-002).
Rule 1 counts a key present when it stands in the entry. Rules 2 and 3 count a
key present when it stands and holds a non-null value, the reading the loader
takes at `json_load.cpp:2744-2745` and `:388-389`. A pair with one key absent
answers no check, so a document written before specs/024 loads as it does today
and no key becomes required by the back door (FR-004). A value of the wrong kind
keeps the standing kind refusal ahead of every pair check (`:135-144`,
`:158-171`, `:196-211`), and a method word outside the three names keeps the
standing enum refusal (`:2728-2732`).

## The refusal order

The checks append after the record reads, at the end of `readEntry` (`:2791`), so
the standing refusals keep their place (D-16). Inside one entry the reader runs:

```text
rows                                    # :2636-2656, unchanged
pass block                              # :2726-2764, unchanged
spreadWarning check                     # :2752-2763, unchanged (FR-009)
hunt record, top-down record            # :2770-2791, unchanged
rule 1  method beside count             # new
rule 2  spread beside count             # new
rule 3  pass tag beside count, per row  # new
```

The first fault stops the read (D-9), the standing stop of the fold
(`:120-133`, measured by `firstFailingPartStopsTheFold` at
`harness_json_load_test.cpp:706-739`). A document that carries a tag fault and a
method fault answers the tag refusal.

## The four refusal lines

The leading path names the pair's non-count field, and the words name the partner
by its full path (D-15). The shape is the standing one: the path, a colon, the
words (`json_load.cpp:57-58`, `:2759-2761`). No new error type joins the reader
(FR-007).

```text
/benchmarks/0/metricMethod: the method 'AUTO' disagrees with the pass count at /benchmarks/0/passCount
/benchmarks/0/passSpread: a pass spread needs a pass count of 2 or more at /benchmarks/0/passCount
/benchmarks/0/rows/0/metrics/0/pass: a pass tag needs a pass count of 2 or more at /benchmarks/0/passCount
/benchmarks/0/rows/0/metrics/0/pass: the pass tag 2 stands at or above the pass count 2 at /benchmarks/0/passCount
```

The method word of the first line comes from `detail::enumWord` over
`detail::kMetricMethodWords` (`json_report.hpp:126-130`). The comparison quotes
the line and answers status 2 on the path it has today (`compare.cpp:1876-1891`,
FR-008).

## Check 1: the method beside the count

| Part | Reading |
| --- | --- |
| Inputs | `metricMethod` (`:2726-2734`), `passCount` (`:2735-2742`), `outcome` (`:2483-2484`), `hunt` (`:2770-2777`) |
| Guard | both keys stand, the outcome reads `MEASURED`, and no `hunt` key stands (D-14) |
| Refusal | the method line, on `AUTO` beside a count other than 0, `MULTIPLEX` other than 1, `PASSES` below 1 (FR-003) |
| Accepted forms | `AUTO` with 0, `MULTIPLEX` with 1, `PASSES` with 1 or more, a stopped entry, a hunt entry, one key absent |
| Tests | `thePassMethodRefusesItsCount`, `aStoppedEntryKeepsItsMethodAndCount`, `aHuntEntryKeepsItsMethodAndCount` |

The waiver of the guard answers two writer forms the rule as worded refuses. The
runner publishes a count of 0 beside a settled `MULTIPLEX` or `PASSES` on four
stop paths (`source/harness/runner.cpp:2884`, `:2889`, `:2923`, `:2928`) and in
the calibration of pass 0 (`:3162`, `:3168`), and the hunt path names the forced
method beside the count the collection published (`cli.cpp:1788-1789`). Spec 024
FR-038 states the waiver with its reason (FR-001).

## Check 2: the spread beside the count

| Part | Reading |
| --- | --- |
| Inputs | `passSpread` (`:2744-2751`) and `passCount` (`:2735-2742`) |
| Guard | the count key stands, and the spread key stands with a non-null value (D-17) |
| Refusal | the spread line, when the count stands below 2 (FR-005) |
| Accepted forms | a null spread beside any count, a spread beside 2 or more, one key absent |
| Tests | `thePassSpreadRefusesItsCount` (FR-005) |

The spread needs no waiver. `passSpread` answers nothing below two pass times
(`source/harness/pass_plan.cpp:404-408`), so no written document carries a
non-null spread beside a count below 2 (FR-010, FR-011).

## Check 3: the pass tag beside the count

| Part | Reading |
| --- | --- |
| Inputs | `passCount` (`:2735-2742`) and the `pass` key of every metric column of every row read (`:387-395`, `:2636-2656`) |
| Guard | the count key stands, and the tag stands with a non-null value (D-17) |
| Refusal | the tag count line below 2, and the tag index line when the 0-based tag stands at or above the count (FR-006) |
| Accepted forms | a null tag beside any count, a tag below the count, the count key absent |
| Tests | `thePassTagRefusesItsCount` (FR-006) |

The check runs over every row of the entry that carries the count, and over no
context block (FR-006). A tag of a row of a later entry names that entry in its
path, and the check reads that entry's own count. The tag is a 0-based pass index
(`source/harness/detail/pass_plan.hpp:47-49`).

## Scope

| Reach | Reading |
| --- | --- |
| Per entry | the checks run inside `readEntry`, once per element of `benchmarks` (`:3230-3235`) |
| Every row | rule 3 walks every row in document order, and the loaded indices equal the document indices (`:2640-2656`, `:1589-1598`) |
| No context block | rule 3 reads `result.rows` alone, for a context block carries percentile and raw columns (`:1658-1689`, `:296-360`) |
| Every minor of major 1 | a check runs on every document that carries its pair, and the reader gates the major alone (`:3129-3135`, D-6) |
| The standing check | the `spreadWarning` check keeps its rule, text and place, and reads an absent spread as no spread (FR-009) |

## Out of scope

| Item | Reason |
| --- | --- |
| The writer | it writes every key of every pair already (`json_report.cpp:849-855`, `:323`), so no rule refuses a written document (D-10, FR-013) |
| The schema file | it states key sets, order and kinds (`report-v1.json:517-520`, `:654-662`), and the checker language carries no cross-field rule |
| The schema checker | `checkReport` is a separate reader (`:3055-3097`), and `loadReport` runs no schema check (`:3101-3247`) |
| The comparison rules | a refused document answers status 2 and one quoted line today (`compare.cpp:1876-1891`), and the grammar gains nothing |
| The public API, the two `SOVERSION` numbers, the schema version, every option | unchanged (D-2, the Scope of `spec.md`) |

## Verification

| Fact | Test |
| --- | --- |
| Each refusal answers its own line | `thePassMethodRefusesItsCount`, `thePassSpreadRefusesItsCount`, `thePassTagRefusesItsCount` |
| One line names its own path and the partner's path | `thePassPairRefusalNamesItsPartnerPath` (FR-007) |
| A pair with one key absent loads | `aPassPairWithOneKeyAbsentLoads` (FR-002, FR-004) |
| The order answers the tag refusal first | `thePassPairChecksRunInReadOrder`, beside `:2752-2763` (FR-009) |
| A stopped entry and a hunt entry keep their pair | `aStoppedEntryKeepsItsMethodAndCount`, `aHuntEntryKeepsItsMethodAndCount` (FR-011) |
| Every written pass form loads | `everyWrittenPassFormLoads`, `aDocumentPredatingThePassFieldsLoads` (FR-010, FR-012) |
| A pair fault answers status 2, one quoted line | `aPassFieldDisagreementAnswersOneQuotedLine`, over `oneFaultLine` at `:782-800` (FR-008) |
| The version refusal keeps its answer | `aForeignSchemaMajorAnswersTheLoadFault` at `:875-884` (FR-011) |

The first row proves FR-003, FR-005 and FR-006. Every case stands in
`test/source/harness_json_load_test.cpp` except the compare pairing, which
writes its document of schema major 1 at run time (D-21). Nine new functions join
the load test, which runs 27 today (`:2205-2238`), and one joins the compare
test; no CTest entry changes (`test/CMakeLists.txt:775-783`).
