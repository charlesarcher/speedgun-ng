# Contract: the `topdown:` rule prefix and the node it compares

**Feature**: specs/042-topdown | **File**: `source/harness/compare.cpp` | **Audit point**: `65c48be9`
**Spec**: FR-037 to FR-039, FR-050, D-14

## What this contract covers

One value-name prefix in the rule grammar, the node value it reads, and every
fault a caller can read. The call, the field order and the three prefixes of
today stand in `specs/026-regression-gate/contracts/rule-parse.md`, which this
contract extends.

## The grammar line it extends

```text
<value>   ::= time | real_time | cpu_time | p <point>
            | counter : <label> | metric : <label> | stat : <label>
            | topdown : <node>
            | jitter : [ run : ] <jitter-cpu> : <jitter-field>
<node>    ::= a node name with its leading "topdown." dropped, dots included
```

`<label>` ends at the first unescaped colon or at sign
(`scanName`, `source/harness/compare.cpp:183-202`), so one split names the node
and leaves the direction and threshold text whole (FR-037). A dot is an
ordinary character inside the label, and `\:` `\@` `\,` `\\` stay the only escapes
(`kEscapedCharacters`, `:63`). `topdown:backend_bound:higher:abs=5` and
`topdown:intel.tma_l1_bound:lower:10%` read; no extra quoting is needed
(SC-13).

The threshold field is unchanged, and its grammar is strict: measured today,
`time@median:lower:rel=10` answers `--compare-rule: the threshold of the rule
'time@median:lower:rel=10' is not readable: 'rel=10' is neither a percent, nor
the abs= marker, nor the two in that order with one comma between` at status 2
(`readThreshold`, `:207-234`). A top-down rule therefore reads
`topdown:backend_bound:lower:5%` or `topdown:intel.tma_l1_bound:lower:10%`. No
artifact of this feature spells a threshold `rel=10`, and the comparator would
refuse such a text (FR-039).

## The prefix list and the value it reads

| Fact | Place today | Change |
|---|---|---|
| prefix list | `kLabelPrefixes`, `:55-56`, three entries | gains `"topdown:"`, four entries |
| the split | `labelPrefixOf`, `:97-105` | none: it walks the array |
| one value, no aggregate | `statisticsValue = prefix != "stat:"`, `:400` | gains `&& prefix != "topdown:"` |
| the `median` default | `:446-448`, under `else if (statisticsValue)` | none |
| the node read | the `metric:` arm of the resolver, `:611-627`, and the `stat:` fall-through, `:629-633` | one `topdown:` arm between them: re-join the `topdown.` head, select the node of `result.topdown` by neutral name, answer its value and no statistics record |
| a node with no value | the guard `index >= result.metricLabels.size()` answers `{}` at `:619-620` | the same: a gapped node answers no value |
| availability | `metricAvailabilityOf`, `:1431-1445`; any prefix but `metric:` answers `COUNTABLE` at `:1436` | none: a gapped node is a missing value, never an uncountable one |

A node carries one value per entry, the way a custom statistic does, so a
written `@median` is a fault and no aggregate lands by default (FR-018 as
`rule-parse.md` states it).

## The faults a caller reads

Both paths print one line on standard error and answer status 2, with no table.
A parse fault answers where the option is read: the command line puts
`--compare-rule: ` in front of the parser's text
(`source/harness/options.cpp:1792`) and a rule file line puts `<file>:<line>: `
in front (`:1301-1302`). A comparison fault prints through the compare mode,
which writes `error: ` and answers status 2 (`compare.cpp:1818-1819`).
Measured today, before the prefix exists:

```text
--compare-rule: the value name of the rule 'topdown:backend_bound:lower:5%' is not readable: 'topdown' is not a value the report carries
```

After the change that text is gone and the five lines below are the whole set.

| Case | The one line |
|---|---|
| `topdown:` names nothing | `--compare-rule: the value name of the rule 'topdown:' is not readable: the 'topdown:' prefix names nothing` |
| `topdown:backend_bound@median:lower:5%` | `--compare-rule: the aggregate of the rule '...' is not readable: this value carries one value and takes no aggregate` |
| a node the contender lacks (FR-038) | `error: the rule 'topdown:nope:higher:abs=5' finds no topdown:nope for bmSum in the contender report` |
| a contender with no top-down object (FR-039, D-14) | `error: the rule 'topdown:backend_bound:higher:abs=5' finds no topdown:backend_bound for bmSum in the contender report` |
| a baseline written without the option | the same text naming the baseline report |

The first two come from `ruleFault` (`:80-88`), which names the field of the
fault and the whole rule text; a parse consults no report, so an unknown node
name is not a parse fault. The last three come from `missingValueFault`
(`:1416-1424`), which names the rule, the value, the benchmark and the report,
and the caller owns the status. A report with no top-down record reads as the
plain missing value: one reading is true, so one fault stands, unlike the jitter
faults (`:1472-1483`), where a cpu is a thing a report can hold or lack beside
a name.

## What moves beside the code

`docs/schema/compare-v1.json` holds each rule as one string (`:80`), so neither
the comparison schema nor its version moves. `specs/026-regression-gate` gains
the fourth prefix and the three fault lines in
`contracts/compare-call.md`, the file `missingValueFault` names at `:1414`.

## Verification

| Fact | Test |
|---|---|
| `topdown:` with nothing after it, and a written aggregate, each answer their line | `test/source/harness_compare_test.cpp`, `theGrammarFaultsEveryUnreadableCorner` (`:2468`) |
| Both SC-13 rules parse, with the node name whole and the threshold read | `theGrammarAcceptsTheRemainingShapes` (`:2531`) |
| A node the matched benchmark lacks faults with the three names | model: `aMissingValueFaultsNamingRuleBenchmarkAndReport` (`:1247`) |
| A node carried with no value counts as a missing value | model: `anUncountableMetricCountsAsAMissingValue` (`:1274`) |
| One line, status 2, no table, nothing run | `test/source/harness_compare_cli_test.cpp`, the `oneFaultLine` helper (`:770-788`) |
