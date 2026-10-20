# Contract: The `--metric-method` option

**Feature**: 024-multipass-collection | **Data-model entity**: E-01 |
**Research**: D-09, D-10 | **Implements**: FR-024, FR-025, FR-026

The command-line half of E-01. The family-side half is
[`metric-method-selector.md`](metric-method-selector.md). One gate keeps the
README option row honest: `example_drift` compares the page's option rows with
the table in `source/harness/cli.cpp`.

## Option

```text
--metric-method=auto|passes|multiplex
```

One long option, one required argument, added beside `--percentiles` and
`--no-percentiles` in the option table (`source/harness/cli.cpp:303-304`).
`--help` carries the same line (FR-026).

| Word | Meaning |
|---|---|
| `auto` | the harness chooses: `PASSES` where a limit is published, `MULTIPLEX` where none is (the default for every run whose family and command line name nothing; FR-025) |
| `passes` | the planner path wherever a limit is published; on a host with no limit the run keeps the single kernel-multiplexed plan and the disclosure names `multiplex` (FR-024, US2 edge, spec D-11) |
| `multiplex` | one plan with every PMU event; the kernel rotates them and the `ratio=` field discloses (FR-024) |

## Command line wins

A run carries one method. The command line's word wins the family's
`metricMethod` request for every instance in the run (FR-025). A repeated
flag applies the last occurrence, the behavior every value-carrying flag in
the table already has.

## Refusals

Every refusal prints one line to stderr and exits 2, before expansion and
before the first measurement, the position `metricNameFailure` holds for the
same reason (`source/harness/cli.cpp:215-247,413`).

| Input | Line to stderr | Exit |
|---|---|---|
| `--metric-method mixed` | `--metric-method 'mixed' is not auto, passes or multiplex` | 2 |
| `--metric-method` with an empty word | the same line with the empty word quoted | 2 |
| `--metric-method=passes --metric-method=multiplex` | none: the last word applies | run |

The line names the bad word in quotes and names the three good words in table
order, one line, no suggestions (there is no near-miss machinery for a
three-word grammar). Comparison is exact and case-sensitive: `Auto` is a bad
word.

## Worked example

The over-limit set on a host publishing a 4-counter limit:

```console
$ ./bench --metric-method=passes --metric=instructions --metric=cpi \
    --metric=branch-density --metric=ipc --metric=bw
bmRoll                   100x  real time/iter (ns)=18000.000  cpu time/iter (ns)=17800.000  overhead floor (ns)=  120.000  endpoint samples=101  cpi=0.900 ratio=1.000 scaled=0 gap=0 pass=0  bw=0.800 ratio=0.400 scaled=0 gap=0 pass=1
bmRoll                       passes  method=passes  count=2  spread=0.4%
```

The row shows the field shape for two of the five columns: the `instructions`,
`branch-density` and `ipc` columns print the same
`label= ratio= scaled= gap= pass=` spelling, each carrying the index of the
pass that measured it.

On the same host:

```console
$ ./bench --metric-method=multiplex ...    # one plan, kernel rotation, ratio fields
$ ./bench --metric-method=auto ...         # the planner splits; same plan as passes
$ ./bench --metric-method=auto --metric=cpi  # fits: master's bytes, no pass line
```

Catalog (`--catalog`) and list (`--list`) modes print what they print today:
they run nothing, so no method settles and no line prints (FR-025, US2).

## Scope boundary

The option carries no limit, no pass count and no column selection. The
printed outcome (the `passes` line and the warning line) is
[`pass-disclosure.md`](pass-disclosure.md).
