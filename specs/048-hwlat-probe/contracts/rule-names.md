# Contract: the thirty-five jitter names and the three `hwlat` names

Phase 1 output. The delta over `specs/027-jitter-probe/contracts/rule-names.md`:
the jitter field table grows from 32 rows to 35, the count word of its
refusal line moves to `thirty-five`, and the three detector figures
stand behind a new head word `hwlat` with a table of their own (spec
FR-028, plan P-11). The four standing grammar forms, the prefix
semantics, the `any` reading and the status-2 stop on a probe-free
report stand unchanged. The standing file carries the amendment in the
commit that lands the table (spec FR-039).

## The name grammar

```text
<value>   ::= ... | jitter : [ run : ] <jitter-cpu> : <jitter-field>
            | hwlat : <hwlat-cpu> : <hwlat-figure>
<jitter-cpu>   ::= any | digits
<jitter-field> ::= one of the thirty-five field names of the jitter
            table, in camelCase
<hwlat-cpu>    ::= any | digits
<hwlat-figure> ::= maxGapNs | gapCount | nmiCount
```

The four standing forms of the pattern carry the jitter names and keep
their shape (`specs/027-jitter-probe/contracts/rule-names.md:10` to
`:13`). The new head takes the same suffix as the standing heads:

```text
hwlat:<cpu>:<figure>:<direction>:<mode>=<bound>
hwlat:any:<figure>:<direction>:<mode>=<bound>
```

| Part | Meaning | Requirement |
| --- | --- | --- |
| `hwlat` | the head word of the detector figures, the fourth value name of the report beside `time`, the percentile points and `jitter` | spec FR-028 |
| `<cpu>` | a tracer cpu index the report's `precheck.tracer` object names | spec FR-028 |
| `any` | the largest figure of the tracer cpus for that name, the uniform maximum the standing `any` rule carries | specs/027 FR-030, D-11 |
| `<figure>` | one of the three names `maxGapNs`, `gapCount` and `nmiCount` | spec FR-028 |
| `<direction>:<mode>=<bound>` | the standing comparison suffix the N5 rule grammar carries | specs/027 FR-033, D-11 |

The head stands as a value name of its own, the way `jitter` does at
`source/harness/compare.cpp:467`. The head word joins no entry of
`kLabelPrefixes` at `source/harness/compare.cpp:55`, which holds the
four colon prefixes `counter:`, `metric:`, `stat:` and `topdown:`.

The new branch joins the value-name dispatch of `readRule` at
`source/harness/compare.cpp:443` to `:493`, beside the `jitter` branch at
`:467` to `:476`. The three figures read a table of their own,
`kHwlatFields`, beside `kJitterFields` at `:262` (plan P-11). A head word
the dispatch names nowhere answers the standing line at `:489` to `:492`,
which for the new word reads `'hwlat' is not a value the report carries`.

## The field row

Line 20 of the standing contract becomes, in the standing columns
`| Part | Meaning | Requirement |`:

| `<field>` | one of the 35 field names the jitter table carries: the sixteen of `JitterStats`, then the sixteen of the series fold, then the three of the attribution, in camelCase | FR-030, specs/048 FR-028 |

## The three new jitter names

The three names join `kJitterFields` at
`source/harness/compare.cpp:262` after the sixteen series names, at
indices 32, 33 and 34. Each reads the carrier the standing names read,
the `JitterEntryFold` the bare prefix and the `run:` prefix select.

| Name | Value the rule reads | Requirement |
| --- | --- | --- |
| `smiDelta` | the change of the SMI count across the window the fold covers | specs/048 FR-025, FR-028 |
| `nmiDelta` | the change of the NMI count across the same window | specs/048 FR-025, FR-028 |
| `smiGapCount` | the count of kept gaps the run marked SMI | specs/048 FR-025, FR-028 |

`hwlat:` names reach the detector record alone, and the `jitter:` names
reach the probe record alone: no name reads both (spec D-6).

## The refusal lines

The parse fault reaches `ruleFault` at
`source/harness/compare.cpp:80`, which wraps the detail in the standing
frame. Each line below stands assembled, with the values one run
inserts.

| Builder | The line as printed |
| --- | --- |
| `source/harness/compare.cpp:427` to `:431`, with the count word the 35-row table moves | `the value name of the rule 'jitter:any:smiDeltaX:lower:abs=5' is not readable: 'smiDeltaX' names none of the thirty-five jitter fields` |
| the new `kHwlatFields` lookup beside `:423` to `:431`, in the same frame | `the value name of the rule 'hwlat:any:gapCounts:lower:abs=5' is not readable: 'gapCounts' names none of the three hwlat figures` |

The standing frame reads `the value name of the rule '<rule text>' is
not readable: <detail>`, and the rule text stands quoted by `quotedText`
while the offending token stands inside literal quotes (spec FR-028).
Each fault prints one line and exits with status 2, before any
comparison runs.

## A name with no figure

| Rule | Requirement |
| --- | --- |
| A fold that carries no value for one of the three jitter names reads 0, the standing rule of a series field on a fold that carries no series at `source/harness/compare.cpp:251` to `:255` | specs/048 FR-028, specs/043 FR-032 |
| A tracer cpu whose figure stands `null` reads 0 under the same rule, and the run gains no new status. The table reader answers a `double` at `source/harness/compare.cpp:244`, so the absent check stands ahead of that read | specs/048 FR-028, plan M8 |
| A `hwlat:` name on a report that carries no tracer record answers the standing absence shape: one line naming the rule and the report, and exit status 2, the shape of `source/harness/compare.cpp:1563` to `:1576` | specs/027 FR-032, specs/048 FR-028 |
| A named cpu the tracer record does not carry answers that same line, the standing rule of a named probe cpu the jitter object does not carry | specs/027 FR-032 |

The figures ride `ReportData::precheck` at
`include/speedgun-ng/benchmark.hpp:1223`, the first pre-check figure the
comparison reads; `source/harness/compare.cpp` reads no pre-check record
today (spec FR-028).

## The standing rows this delta changes none of

- The four grammar forms of lines 10 to 13 keep their reading, and the
  standing suffix keeps its reading (specs/026, specs/027 FR-033).
- The standing 32 names keep their indices: the three new names join the
  table at the end, so no standing name moves and no index shifts (spec
  FR-028).
- `jitter:run:` keeps its reading, and a bare `jitter:` name keeps
  reading the per-benchmark folds (specs/027 FR-031).
- The status-2 stop on a probe-free report stands as specs/027 states
  it, one line and exit status 2 (specs/027 FR-032).
- The comparison gate gains no option: the names ride `--compare-rule`
  (specs/027 FR-033).

## The amendment the 027 contract needs

| File and line | Standing text | New text |
| --- | --- | --- |
| `specs/027-jitter-probe/contracts/rule-names.md:20` | one of the 32 field names the jitter table carries: the sixteen of `JitterStats`, then the sixteen of the series fold | one of the 35 field names the jitter table carries, the three attribution names last |
| `specs/027-jitter-probe/contracts/rule-names.md:31` | `names none of the thirty-two jitter fields` | `names none of the thirty-five jitter fields` |
| `specs/027-jitter-probe/contracts/rule-names.md:26` to `:31` | the sentence that lists the names and the count | the same sentence with `smiDelta`, `nmiDelta` and `smiGapCount` named |
| `specs/026-regression-gate/contracts/rule-parse.md:35` | the `<value>` production ends with its `jitter` alternative | one further alternative joins below it, for the `hwlat` head |
| `specs/026-regression-gate/contracts/rule-parse.md:39` | the `<jitter-field>` production names the sixteen field names of the stats record | the production names the thirty-five field names of the jitter table |
| `specs/026-regression-gate/contracts/rule-parse.md:104` | the example row refuses an unknown field as "none of the sixteen jitter fields" | the same row reads "none of the thirty-five jitter fields", the words the moved table prints |

## Consumers

- `test/source/harness_compare_test.cpp`: the 35 jitter names resolve,
  the three `hwlat` names resolve, and a standing name keeps its index
  (spec FR-028).
- `test/source/harness_compare_cli_test.cpp`: both refusal lines, the
  one-line count and exit status 2, and the standing refusals word for
  word over the grown table (spec FR-028, plan R-7).
- `test/source/harness_jitter_window_test.cpp`: the probe report the
  comparison reads, with the three attribution figures filled (spec
  FR-028).
- `specs/027-jitter-probe/contracts/rule-names.md` and
  `specs/026-regression-gate/contracts/rule-parse.md` carry the rows
  above in the same commit as the table (spec FR-028, FR-039).
