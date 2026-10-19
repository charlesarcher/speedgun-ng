# Contract: Catalog Metric Lines

**Capability**: C-3 catalog listing | **Plan**: D-11 | **Owner**: `source/harness/catalog.cpp`

## Surface

The `--catalog` path keeps its shape. `printCatalog` walks the object tree
and `printObject` prints one line per counter under an object path with the
`description`, `unit`, `mode`, `avail` and `refusal` fields. Metric lines
follow the counter lines of the same object path, carrying the same field
set in the same column order.

## Guarantees

- One line per metric row of the selected table, so the listing accounts for
  the whole directory (FR-021).
- Counter lines keep their current shape, their order and their field set
  (FR-022). A reader of the existing output sees nothing reordered.
- A metric that is not available stays listed with the availability value and
  the reason recorded at load (FR-023). That covers an event the host cannot
  encode, an event the host lacks, a reference cycle, a construct outside the
  grammar, a `ScaleUnit` spelling the conversion table lacks, and a literal
  constant with no reader.
- A host that selects no table prints no metric line and reports no error
  from the listing itself (FR-024). Asking for a metric by name in that state
  is the error path the lookup contract owns (FR-017).
- Group text and unit text appear exactly as the table spells them, so a
  reader comparing the listing against the JSON sees the same string.
- Order is table order inside the metric block: the parse and resolution
  passes reorder nothing (D-02, D-03).

## Line shape

```text
<path>.<MetricName>  description=<group and the table description>
                     unit=<ScaleUnit text>  mode=metric
                     avail=<availability or ->  refusal=<one-line reason or ->
```

`mode=metric` is the field that separates a metric line from a counter line,
so a script that parses the listing can tell them apart without a name list.
A counter line never gains that value.

## Refusal reasons a reader sees

| Reason text carries | Raised by |
| --- | --- |
| the construct name and its offset | D-01 |
| the names of the cycle in walk order | D-04 |
| the `ScaleUnit` spelling | D-05 |
| the constant name | D-06 |
| the inner refusal of a reference or a test | D-03, D-07 |
| the operands that cannot compose | FR-010 |
| the duplicate name and the row that kept it | FR-011 |

One line each, no wrapping and no second line, so the listing stays parseable
by the harness tests that count lines.

## Checks

`test/source/harness_catalog_test.cpp` asserts the counter lines keep their
shape while metric lines join, that every row of a fixture directory appears
once, that a refused row appears with its reason, and that a host with no
selected table prints no metric line (FR-021 to FR-024). Line-count fixtures
that exist today are updated in the same commit that adds the lines, so no
commit in the feature carries a red gate.
