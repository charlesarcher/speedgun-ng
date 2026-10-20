# Contract: The pass disclosure lines

**Feature**: 024-multipass-collection | **Data-model entities**: E-06, E-07,
E-08 | **Research**: D-08, D-10, D-11 | **Implements**: FR-017, FR-018,
FR-019, FR-020, FR-021, FR-022

The console output that tells a reader how a metric-bearing run collected its
events. The printed lines are golden-testable: `test/source/
harness_pass_report_test.cpp` diffs them against a fixture, in the pattern of
`example/docs/percentiles.expected`.

## The passes line

One line, printed by the new `printPassDisclosure` beside the existing
disclosure call sites (`source/harness/report.cpp:537,558`), shaped by the
existing tagged-line format `printf("%-24s %10s  ...")` of the percentile line
(`report.cpp:306`) and the chunks line (`report.cpp:358-362`). The tag is
`passes`.

```text
<name>                       passes  method=<word>  count=<n>  spread=<p>%
<name>                       passes  method=<word>  count=<n>
```

| Field | Rule |
|---|---|
| `<name>` | the instance name, the same `%-24s` field every row line uses |
| `method=` | the settled method token: `passes` or `multiplex` (E-01) |
| `count=` | the pass count run (E-07 `passCount`) |
| `spread=` | present exactly when the run had more than one pass; the value is E-06 to one decimal, printed once at print time; a run whose spread cannot be computed prints the token `spread=not available`, following the percentile field precedent (`report.cpp:314`) |

Print rule (E-07, research D-10): the line appears when the run carries at least one
metric column and one of: the settled method is `MULTIPLEX`; the pass count
exceeds one; the caller forced the method (the flag or the selector named
one). An `auto` run that settles to `PASSES` with one pass prints no line:
its report is master's bytes (FR-021, D-9, SC-03).

## The warning line

```console
warning: pass spread 11.1% exceeds 5%
```

Printed directly after the passes line when the spread value exists and
exceeds 5.0 percent (FR-018; research D-08's predicate on the unrounded value;
the rule rests on spec D-6). The
warning refuses nothing and changes nothing: the run's rows, outcome and exit
status are the run's regardless (FR-018; US3 scenario 3).

## The pass tag on metric fields

The existing metric field spelling (`report.cpp:489-504`: `label=%9.3f
ratio=%.3f scaled=%d gap=%d`) gains one conditional token:

```text
  <label>=<value> ratio=<r> scaled=<s> gap=<g> pass=<k>
```

`pass=` carries the measuring pass index (E-08 `MetricValue::pass`) and
appears exactly when `passCount > 1`. A one-pass run prints today's four
tokens and nothing more (FR-019, FR-021).

## Golden fixture (fixture values, fake provider, limit 4)

```text
bmRoll                   100x  real time/iter (ns)=112000.000  cpu time/iter (ns)=110000.000  overhead floor (ns)=  120.000  endpoint samples=101  cpi=0.900 ratio=1.000 scaled=0 gap=0 pass=0  bw=0.800 ratio=1.000 scaled=0 gap=0 pass=1
bmRoll                       passes  method=passes  count=2  spread=11.1%
bmRoll                       warning  ...
```

(the warning line as spelled above; the `11.1` comes from the fixture's pass
reference times 100/108/112: (112-100)/108*100 rounded at print; SC-05).
The fixture rows print the two-pass shape; the one-pass and multiplex shapes
appear as the rules state: no `pass=` tokens and no line for the `auto` fit,
`method=multiplex  count=1` and no spread field for a multiplex settling
(FR-020, US3 s4).

## Byte-identity rule

For a run that settles to `PASSES` with one pass with no forcing, the whole
console report equals the master capture for the same run, field by field
(FR-021, D-9; the fixture is captured from master at the audit
point).
`printContextDisclosure` prints what it prints today for every run,
threaded ones included (spec D-8, N4): the pass lines are separate output from a
separate function.

## Scope boundary

The disclosure names no limit, no provider path and no column-to-pass table:
it prints the method, the count, the spread and the per-value tag. A
`--list` or `--catalog` run prints neither line: the run ends before any
method settles (FR-025). The
programmatic halves of the same facts are the `BenchmarkResult` fields
`metricMethod`, `passCount` and `passSpread` (E-07); their JSON names join the
schema when the report lane lands it (FR-017; spec N3).
