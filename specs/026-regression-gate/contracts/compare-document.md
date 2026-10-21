# Contract: The Comparison Document and Its Schema

## The document

With the compare mode on, `--json=<path>` writes one comparison
document (FR-057); the mode writes no report document. The writer is
internal
to the harness, in `source/harness/compare.cpp`, and emits through the
vendored builder's `string_builder`, the report writer's tool
(023 D-03).

```cpp
// source/harness/detail/compare.hpp
/**
 * @brief Write the comparison document of one finished comparison.
 *
 * \pre `path` is not empty and `result` is the value the comparison
 *      answered for the two reports and rules named here
 * \post true when the file holds the whole document, false when the
 *       file system refused it; the call reports the refusal on stderr
 *       and throws nothing
 */
[[nodiscard]] auto writeCompareReport(const std::string& path,
                                      const CompareResult& result,
                                      const ReportData& baseline,
                                      const ReportData& contender,
                                      const std::vector<CompareRule>& rules,
                                      const std::string& baselinePath,
                                      const std::string& contenderPath) -> bool;
```

A write failure answers status 2 from the compare mode, the form the
report write already answers
(`specs/015-benchmark-harness-core/contracts/cli.md:74`), and the mode
prints one line naming the path (FR-074).

## Key order and content

The document's shape stands in [data-model.md](./data-model.md) E-10.
Key order is normative and stands in `x-order` in the schema file, the
report schema's convention (`docs/schema/report-v1.json:7`). Top-level
keys: `schemaVersion`, `speedgun`, `inputs`, `rules`, `entries`,
`verdict`. Entry keys: `name`, `rule`, `baseline`, `contender`,
`changePercent`, `changeAbsolute`, `state`, `reason`. An empty optional
writes `null`, the report format's unanswered form (023 D-12). Values
stand in the stored unit, with no printed scaling (spec D-22).

The document carries no date, no host and no pid, so two runs of one
command write the same bytes (SC-009).

## The schema file

`docs/schema/compare-v1.json` is a JSON Schema document in the report
schema's conventions: `x-order` beside `required` at every object,
definitions inlined, no `$ref` (`docs/schema/report-v1.json:1` to
`:10`). Its `schemaVersion` const is `1.0`; a future incompatible
document is `compare-v2.json` beside it, the rule the report schema
carries.

## The checker

The existing checker `checkReport`
(`source/harness/json_load.cpp:1067`, declared at
`source/harness/detail/json_report.hpp:188`) reads any schema file in
these conventions and compares the key set, the key order and the value
kinds of a document against it. The comparison document test calls it
with the new schema path and the written bytes; no second checker
enters the tree (D-14). The checker stays harness-internal: it reaches
no installed header and no exported symbol (004 FR-013 to FR-017).

## What the document never carries

- No per-iteration values and no chunk columns: the document holds the
  comparison the call answered, and
  `docs/schema/compare-v1.json` describes that document whole
  (FR-057).
- No recomputed statistic and no substituted default.
- No second document: one file per `--json=` path, the report writer's
  one-write shape.
