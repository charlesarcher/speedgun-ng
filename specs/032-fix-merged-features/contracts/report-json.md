# Contract: the report schema top-level key order

Phase 1 output. The contract of `specs/023-json-reports` holds as it
is; this file records the one delta FR-017 lands.

## Top-level key order

`docs/schema/report-v1.json` carries exactly one top-level `x-order`
key:

```json
"x-order": ["schemaVersion", "speedgun", "run", "jitter", "benchmarks", "hunt"]
```

The writer order of `source/harness/json_report.cpp` matches it:
`jitter` stands at `:590` before `benchmarks`, and `hunt` stands at
`:676` after it (measured on master `500704b`).

## Rules

- A document that carries a duplicate top-level key fails the schema
  test (FR-017).
- The schema version stands at `1.5` (`kSchemaVersion`,
  `json_report.cpp:36`) and the mutation anchor reads it once
  (FR-006); no `1.4` figure remains in the test.
- No other key moves and no field changes in this feature.
