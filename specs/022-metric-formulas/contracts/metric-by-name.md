# Contract: Metric by Name

**Capability**: C-1 formula parse, C-2 metric by name | **Plan**: D-01 to D-10

## Surface

```cpp
namespace sg::counters {

struct MetricView
{
  Expression<D> expression;  // foldable by a scope or a recorder
  std::string_view group;    // MetricGroup text, verbatim
  std::string_view unit;     // ScaleUnit text, verbatim
};

[[nodiscard]] SPEEDGUN_NG_EXPORT auto
metricByName(std::string_view name)
  -> Result<MetricView, LookupError>;

}  // namespace sg::counters
```

The declaration sketch fixes shape and meaning. The dimension type of the
expression and the exact `Result` and `LookupError` spellings follow the
conventions the object lookup in `include/speedgun-ng/counters_core.hpp`
already sets, and the header must compile standalone under the purity scan.

## Guarantees

- The name match is exact and case-sensitive against the `MetricName` of the
  selected directory (FR-015).
- The returned expression folds to the same value a hand-composed expression
  over the same events gives, given the same counts, and the `ScaleUnit`
  factor is inside it exactly once (FR-018, D-05, D-09).
- `group` and `unit` carry the table's text without change (FR-015).
- `MetricResult` keeps its four members and `Availability` keeps its six
  values (FR-018, D-09).
- The call reads no counter, parses no text and performs no allocation
  beyond the view it returns: the first call for a directory triggers that
  directory's load and every later call reuses it (FR-019).
- A metric that loaded and cannot be counted folds to a result whose
  availability says so, with the reason recorded at load (FR-020).

## Refusals

| Condition | Result | Rule |
| --- | --- | --- |
| No directory selected for this host | recoverable error saying the host carries no metric table | FR-017 |
| Name absent | one-line message, near-miss names in the diagnostics | FR-016 |
| Name carried by two rows | the first row answers, the second is a load refusal naming the duplicate | FR-011 |
| Name refused at load | the load refusal kind and its recorded reason | FR-020, D-01, D-04, D-05, D-06, FR-010 |

No refusal is an exception and no refusal leaves a partially applied metric
behind. The error message is one line, so the harness can print it verbatim.

## Parse guarantees (C-1)

- The grammar is the closed list of D-01 and covers FR-003: numeric
  literals in decimal, hexadecimal and exponent form, dotted event names,
  the `:` privilege suffix, the `unit@modifier@` spelling with its backslash
  escapes, the four operators, parentheses and unary minus.
- The five functions FR-004 names parse: `min`, `max`, `d_ratio`,
  `has_event` and `source_count`, each with two arguments in every row of
  the tree, and `min` and `max` of two runtime values reach the fold as the
  appended node kinds of D-09.
- Conditionals take the FR-005 forms: `<` and `>` comparisons, and a test
  that asks `has_event` or `source_count`.
- A literal constant resolves from a published host fact, otherwise the row
  refuses and names the constant (FR-006).
- Reference resolution covers the 2973 cross-referencing rows; a cycle
  refuses every member with the names in walk order and leaves the metrics
  outside the cycle available (FR-008, FR-009).
- Terms that cannot compose under the existing typed rules refuse at load
  with the operands named (FR-010), and a duplicated name keeps the first
  row and refuses the second (FR-011).
- A `has_event` test settles from what the host can count, and the selected
  branch discloses its own availability (FR-012).
- A metric name never encodes as an event, so the existing refusal that
  keeps metric rows out of the countable catalog stays in force (FR-013).
- The parser adds no runtime dependency to the project (FR-014).
- A row that refuses is recorded with its kind and reason, and the rest of
  the directory loads (FR-007).

## Checks

`test/source/counters_metric_formula_test.cpp` drives every accepted
construct from table text and every refusal from a crafted input, then sweeps
every vendored directory and asserts that entries plus refusals equal the row
count (FR-038). `test/source/counters_metric_lookup_test.cpp` covers
`cpi`, a `100%` row, `tma_info_thread_ipc` through `tma_info_thread_clks`,
the near-miss message, the cycle fixture, the duplicated-name pair and both
branches of a `#SMT_on` conditional under the fake provider (FR-009, FR-011,
FR-015, FR-016, FR-018, FR-020, FR-039).
