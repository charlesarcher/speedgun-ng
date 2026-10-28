# Contract: the report JSON with the node plans

Amendment of `specs/025-topology-placement/contracts/report-json.md`
and the standing schema `docs/schema/report-v1.json` (specs/035).

## Schema

No key is added and no key moves. The context object carries the
standing `placement` string
(`docs/schema/report-v1.json:350`), and a bound context's value grows
the node tail:

```json
"placement": "cpu@group@node"
```

| Rule | Requirement |
| --- | --- |
| The schema minor stays `1.6`; the top-level `x-order` stands unchanged. | FR-004 |
| `loadReport` reads the field unchanged: the value is a string, and the reader carries no node-specific rule. | FR-002 |
| A run with no node preset writes the same bytes as master apart from the version figures the report carries. | FR-002, D-5 |

## Round trip

The writer emits the field with the node tail where the plan bound
one; the reader loads the string as it stands; a written and reloaded
document compare equal, the way the standing round-trip tests read
(`harness_json_report_test.cpp`, `harness_json_load_test.cpp`).
