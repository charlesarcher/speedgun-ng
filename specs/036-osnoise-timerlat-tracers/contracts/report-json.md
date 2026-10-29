# Contract: the `tracer` keys of the `precheck` JSON record

Phase 1 output. The delta over the standing `precheck` record contract
of `specs/034-host-noise-precheck/contracts/report-json.md`: the
optional tracer record joins the `precheck` object, the schema minor
moves to `1.7`, and the top-level `x-order` stands unchanged (spec
FR-006, FR-014, D-3). The standing contract's key tables stand
unchanged; the amendment this delta names joins the standing file in
the commit that lands the code (spec FR-024).

## Key order

The `precheck` object's `x-order` moves in the same commit as the
writer (spec FR-006):

```json
["serial", "windowMs", "cpus", "hostSettings", "skippedGroups", "tracer"]
```

The top-level `x-order` list stands unchanged. The `precheck` object's
`required` list stands unchanged: the `tracer` key joins `properties`
and `x-order` and stands outside `required`, the optionality mechanics
the standing checker reads (`checkNode` of
`source/harness/json_load.cpp`: a key of `properties` stands checked
only when the document carries it, the order check reads only the keys
the document carries, and a closed object refuses a key the
`properties` list does not name; research.md R-11). The per-benchmark
`hunt` object stands the standing precedent (specs/029).

## The `tracer` object

| Key | Kind | Meaning |
| --- | --- | --- |
| `engine` | string, enum `osnoise` or `timerlat` | the engine word the value named |
| `cpus` | array | one entry per tracer cpu, ascending |

## A `cpus` entry

| Key | Kind | Meaning |
| --- | --- | --- |
| `cpu` | integer | the cpu index |
| `nmi` | integer or `null` | the NMI noise count |
| `irq` | integer or `null` | the IRQ noise count |
| `softirq` | integer or `null` | the softirq noise count |
| `thread` | integer or `null` | the thread noise count |
| `hardware` | integer or `null` | the hardware noise count, the source the kernel's files word `hw` |

The figures stand the engine's cumulative totals since the tracer's
start, read once after the measurement ends (spec FR-009). A count of
zero stands a figure like any other; `null` stands only for a source
the engine states no figure for, or a cpu whose files state nothing at
read time, the file form of the console's `not available` (spec
FR-010, the Edge Cases, the standing shape of the `attribution`
object). The record names cpus alone, and the CCD mapping stands in
the acceptance evidence (spec FR-014, D-7).

## Rules

- A run that requests no tracer, and a run whose tracer stands not
  available, write no `tracer` key: the document stands byte-identical
  to master's apart from the `schemaVersion` value and the version
  figures the document carries (spec FR-015, D-3).
- `loadReport` reads the object back through the standing
  `optionalField` path, the way `hunt` reads (spec FR-014); a document
  written before the key stands read as the tracer-less record.
- The compare flows read `schemaVersion` alone for the major check;
  the key stands inside the `precheck` object beside it.
- The schema test fails when a document carries a duplicate key, a key
  the closed object does not list, or an order the `x-order` list does
  not state.
