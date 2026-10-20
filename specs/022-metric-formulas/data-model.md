# Data Model: Metric Formulas

**Feature**: `022-metric-formulas` | **Plan**: [plan.md](./plan.md)

**Audit point**: `ab32400b86321f98f13b9c66d4488e15831b47f6`

Two groups follow. The internal model lives in the counters library and
reaches no public header. The public model is the one view and one refusal a
caller can hold.

## Internal model

### `MetricRow` (the row as the table states it)

| Field | Type | Source | Note |
| --- | --- | --- | --- |
| `name` | text | `MetricName` | The name a caller asks for |
| `expression` | text | `MetricExpr` | The formula text, verbatim |
| `group` | text | `MetricGroup` | Printed in the catalog, verbatim |
| `unit` | text | `ScaleUnit` | Printed verbatim, factor applied separately |
| `public` | flag | `Unit` and `PerfQuery` | A row hidden upstream stays listed with its reason |

One `MetricRow` exists per expression row of the selected directory,
5958 of them across the 46 directories and 279 in `skylakex`.

### `MetricEntry` (a row that made it into the image)

| Field | Type | Note |
| --- | --- | --- |
| `core` | `detail::ExprCore` | The resolved spine, leaves included |
| `factor` | `double` | From the conversion table, 1.0 when the row carries no scale text |
| `name`, `group`, `unit` | text | Copied from the row |
| `availability` | `Availability` | Decided at load, one of the six existing values |

`core` uses the node kinds `ExprCore` defines plus the two this feature
appends: leaf, add, subtract, divide, scale, minimum, maximum. A conditional,
a helper call and a literal constant exist during the load passes and are
gone before publication (D-07), so `foldPairsCore`, `metricCore` and
`compileCore` gain the two new arms and nothing further.

### `MetricRefusal` (a row that did not make it)

| Field | Type | Note |
| --- | --- | --- |
| `name` | text | Empty when the row carried no name |
| `kind` | `MetricRefusal` | One of the nine values below |
| `reason` | text | One line, names the construct, constant, scale or cycle |

| Kind | Raised by | Rule |
| --- | --- | --- |
| `MALFORMED` | D-01 | Unbalanced parenthesis, trailing or doubled operator, empty text |
| `UNSUPPORTED` | D-01 | A construct outside the closed grammar, with the text that met it |
| `CYCLE` | D-04 | Every member of a reference cycle, with the names in walk order |
| `UNKNOWN_SCALE` | D-05 | A `ScaleUnit` spelling the conversion table lacks |
| `UNKNOWN_CONSTANT` | D-06 | A `#` constant with no reader for this host |
| `NESTED` | D-03, D-07 | A reference or a test that names a refused metric, with the inner refusal named |
| `DIMENSION` | FR-010 | Terms that cannot compose into one expression under the typed rules, naming the operands |
| `UNRESOLVED` | D-03 | A reference to a name no row of the directory carries |
| `COLLISION` | FR-011 | A second row carrying a name the first row already took |

`MetricRefusal` is internal and reaches the catalog text. The public lookup
reports a miss with its own recoverable error, on the object-lookup pattern,
so the enumeration stays out of the public surface.

### `ScaleRule` and `ConstantFact` (the two data tables)

| Table | Field | Note |
| --- | --- | --- |
| `ScaleRule` | `spelling`, `factor`, `unit` | One entry per distinct `ScaleUnit` spelling of the tree |
| `ConstantFact` | `name`, `reader` | One entry per `#` constant with a reader, four at first |

`ConstantFact` readers at first: online processor count, core count, package
count, SMT state. Four constants of the pinned tree carry no reader and raise
`UNKNOWN_CONSTANT`: `#has_pmem`, `#num_dies` and `#core_wide` name a host
property this library publishes no object for, and `#SYSTEM_TSC_FREQ` names a
rate, which the time-stamp-counter entry publishes as a count asserting no
rate at all (D-06; specs/008-timestamp-counter FR-011).

### `SelectionKey` (the mapping-file key)

| Field | Source | Note |
| --- | --- | --- |
| `vendor` | CPUID | `GenuineIntel` or `AuthenticAMD` |
| `family` | CPUID | Decimal, as the mapping file spells it |
| `model` | CPUID | Hex, as the mapping file spells it |
| `stepping` | CPUID | New at this feature, hex, the field the split needs |

`PmuIdent` in `source/counters/detail/pmu.hpp` carries all four;
`mapfileKey` renders them in the order the file uses.

## Public model

### `MetricView`

| Field | Type | Note |
| --- | --- | --- |
| `expression` | `Expression<D>` | Foldable by a scope or a recorder, same path a composed metric takes |
| `group` | text | The table's group text |
| `unit` | text | The table's scale-and-unit text, verbatim |

The view holds no value. A value appears when the caller folds the
expression, and that fold produces an ordinary `MetricResult` with
`value`, `runningRatio`, `availability` and `scaled`. The factor is already
inside the expression, so the fold applies it once.

## Relationships

```text
directory image  1 --- *  MetricRow
MetricRow        1 --- 1  MetricEntry  xor  MetricRefusal
MetricEntry      1 --- 1  ScaleRule     (absent means factor 1.0)
MetricEntry      * --- 1  ConstantFact  (through a folded literal)
MetricEntry      * --- *  MetricEntry   (a reference, resolved away before publication)
name index       1 --- 1  directory     (exact, case-sensitive)
```

## Invariants

1. Every row of the selected directory is either one `MetricEntry` or one
   `MetricRefusal`, and the two together equal the row count. The catalog
   prints both, so the listing is checkable against the table.
2. A published `MetricEntry` has no unresolved reference, no conditional, no
   helper call and no literal constant left in its spine. Those forms exist
   only during the load passes.
3. A reference cycle leaves no published entry among its members. Every
   member is a `CYCLE` refusal.
4. The scale factor is applied exactly once, inside the spine, at load. A
   fold multiplies by nothing further.
5. Two rows of one directory never share a published name. The first row
   publishes and the second is a `COLLISION` refusal naming the duplicate
   (FR-011), so a lookup reaches the row the table states first.
6. Matching a name is exact and case-sensitive. No edit distance enters a
   successful lookup; near-miss names appear only in a refusal's diagnostics.
7. A `MetricView` is valid while its directory image lives, and it holds no
   counter handle, so it outlives a scope and cannot outlive the library.
8. No parsed form exists for a refused row, and no refusal exists for a row
   that published. The two sets are disjoint by construction.
9. `MetricResult` and `Availability` keep their member lists. The stepping
   field is private, so no member of either public type changes.

## Lifecycle of a row

```text
table text
  |  split, fields read
MetricRow
  |  D-01 parse
parsed spine with name placeholders   or  refusal MALFORMED / UNSUPPORTED
  |  D-03 resolve, D-04 cycle walk
resolved spine                        or  refusal UNRESOLVED / CYCLE / NESTED
  |  D-06 constants, D-07 conditionals and helpers
leaf-only spine                       or  refusal UNKNOWN_CONSTANT / NESTED
  |  D-05 scale factor, FR-010 dimension check
MetricEntry                           or  refusal UNKNOWN_SCALE / DIMENSION
  |  name index, catalog, lookup
published to callers
```

Each arrow is one load pass, runs once per directory, and never runs again
for that directory. A row leaves the pipeline at the first refusal and never
re-enters.

## Recorded real-host run (T060, FR-039, FR-017)

Run on `corsairone`, 2026-10-19: CPUID family 6, model 85 (`0x55`),
stepping 4, the identity D-7 selects `arch/x86/skylakex/` for. The binary is
`build/example/docs_command_line`, a Release build of this branch, and the
listing is its own `--catalog` output. No unit test reads these numbers; the
row counts they pin are pinned in the test suite by the sweep of T056, which
resolves the same rows against the scripted host.

| Observation | Figure |
| --- | --- |
| Metric lines the listing prints | 279 |
| Metric rows the spec pins for `skylakex` | 279 |
| Rows published (`refusal=-`) | 128 |
| Rows refused | 151 |
| Refusals naming `UNKNOWN_CONSTANT` | 76 |
| Refusals naming `UNRESOLVED` | 71 |
| Refusals naming `UNSUPPORTED` | 3 |
| Refusals naming `DIMENSION` | 1 |
| Rows the catalog marks `not-encodable` | 12 |

Each refusal line carries the reason beside its kind, so the 76 constant rows
name the constant they need (`#has_pmem`, `#num_dies`, `#core_wide`,
`#SYSTEM_TSC_FREQ`) exactly as D-06 records, and the 71 unresolved lines name the leaf and the scope that
reaches no object on this host.

The column path was exercised with `--metric=cpi --metric=l2_mpi` and with
`--metric=cpi --counter=cpu/cpu_clks --counter=cpu/instructions`: the metric
columns stand in the report in the order named, keyed by the name as typed,
and an unknown name stops the run with status 2 before any measured row. The
values themselves are zero on this host at run time, because
`/proc/sys/kernel/perf_event_paranoid` reads 2: the kernel then refuses CPU
counting to user space, the leaves of the spine report absent, a counter
column prints `refusal=absent` with `gap=1`, and a ratio of two absent leaves
folds to `-nan`. That is the environment, not the metric path: the same
columns carry values on a host where the knob permits counting, and the value
equality of FR-029 is gated by `harness_cli_test` on the scripted host.
