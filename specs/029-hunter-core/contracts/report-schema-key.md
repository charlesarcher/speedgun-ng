# Contract: the schema keys and the 1.4 reservation

**Feature**: specs/029-hunter-core | **File**: `docs/schema/report-v1.json` with
`source/harness/json_report.cpp:kSchemaVersion` | **Audit point**: `bd10d76`
**Spec**: FR-053, D-17, hard rule 11

## What this contract covers

The document rule for the two hunt keys: where they join the key lists, why
neither joins a `required` list, what the minor bump costs, and how the rebase
with the two other lanes settles the number.

## The rule as written

`hunt` joins the `x-order` list and the `properties` list of both the root
object and the per-entry object, and joins neither `required` list.

| Node | At `bd10d76` | After this feature |
|---|---|---|
| root `x-order` | `schemaVersion`, `speedgun`, `benchmarks` (`report-v1.json:7`) | those three, then `hunt` |
| root `required` | the same three (`report-v1.json:8`) | unchanged |
| root `additionalProperties` | `true` (`report-v1.json:9`) | unchanged: the root stays open |
| entry `x-order` | 21 keys ending `metricMethod`, `passCount`, `passSpread`, `spreadWarning` (`report-v1.json:28-50`) | those 21, then `hunt` as the 22nd |
| entry `required` | 19 keys: every entry key except `reason` and `noiseWarning` (`report-v1.json:51-71`) | unchanged |
| entry `additionalProperties` | `false` (`report-v1.json:72`) | unchanged: the entry stays closed |
| `schemaVersion` written | `1.1` (`json_report.cpp:31`) | `1.4` |

## Why `properties` must carry the keys

The per-entry object is closed. `checkNode` (`json_load.cpp:1060`, reached
through `checkReport` at `json_load.cpp:1156` and declared at
`detail/json_report.hpp:219`) compares the key set, the key order and the value
kinds of a written document against the schema, and a key that stands in a
closed node's document and not in its `properties` is a finding. The hunt
documents the harness writes therefore fail the schema check unless the schema
lists the keys: `harness_json_report_test.cpp:278,382` and
`harness_compare_cli_test.cpp:636,704` run `checkReport` over written documents,
so an unlisted key reddens those tests on the first hunt document written.

`x-order` is normative, the description of the schema file states, so a listed
key that stands at no position in `x-order` is a finding of the same kind. Both
`hunt` keys are appended at the end of their node's order, which keeps every
byte order a document had before.

## Why `required` must not carry them

A required key of a closed node that stands absent is a finding, and the reader
cannot rescue it: `optionalField` (`json_load.cpp:103`) answers a key that is
not there, and the schema check has already reported the absence. Every entry
key written by master stands in the entry's `required` list except `reason` and
`noiseWarning`, so the pattern of a key that a document may omit exists and is
the pattern D-17 puts `hunt` in.

The consequence is the one the spec records: a document written before this
feature carries no `hunt` key per entry, and a `required` entry key would make
that document invalid against the schema this build reads. Keeping `hunt` out of
both `required` lists is what keeps a pre-1.4 document valid while the reader
still refuses a hunt object that stands present with a key missing, because
inside a present `hunt` object every key is required by the hunt node's own
`required` list ([hunt-json.md](hunt-json.md)).

## The two hunt nodes

```text
"hunt": {
  "type": "object",
  "x-order": ["selection", "norm", "rank", "top", "noisePercent", "repeats",
              "seed", "budget", "passes", "method", "events", "stages",
              "estimate_ns", "planned_events", "ran_events",
              "stage_two_events"],
  "required": [the same sixteen],
  "additionalProperties": false,
  "properties": { one entry per name above, `events` an array of strings and
    `stages` an array of a closed node with x-order
    ["first", "count", "repeats"] }
}

"hunt": {
  "type": "object",
  "x-order": ["method", "pass_count", "top", "retried_passes", "entries",
              "skipped"],
  "required": [the same six],
  "additionalProperties": false,
  "properties": {
    "entries": array of a closed node with x-order
               ["address", "rate", "cv", "noisy", "multiplexed", "rank"],
    "skipped": array of a closed node with x-order ["address", "kind", "stage"]
  }
}
```

Both nodes are closed and both list every key as required, because a hunt object
the harness wrote always carries all of them: an absent value inside one is
written as `null`, which is the shape `passSpread` already uses
(`json_report.cpp:508`). The null-accepting kinds are the top-level `budget`
and `estimate_ns`, `entries[].rank` and `skipped[].stage`, each written as a
one-of pair of its kind
and `null` in the form the existing optional entries of the schema use.

## The 1.4 reservation and its rebase

`1.4` is the reserved minor (D-17, hard rule 11). Lanes specs/027 and specs/028
reserve the earlier minors; the landing rule is the one hard rule 11 states and
this feature's own precedent at 1.1 shows:

- read `json_report.cpp:kSchemaVersion` and `docs/schema/report-v1.json` on
  master at rebase time,
- take the next free minor, keeping both files on the same value,
- carry the keys of the lanes that merged before this one in the same document,
- record the value and its reason in the merge notes.

## What the bump costs

Four files name the version `1.1` today and move with the bump. None of them is
a gate: each is a test or a page that states the number the writer stamps.

| File | What it pins |
|---|---|
| `test/source/harness_json_load_test.cpp:113` | the current-version document the loader test builds, `withVersion("\"1.1\"")` |
| `test/source/harness_json_load_test.cpp:529` | the `schemaVersion` kind refusal row, which plants `"\"schemaVersion\":1"` beside the current text |
| `test/source/harness_compare_cli_test.cpp:516` | the comparison console line, which expects the words `schema 1.1` beside `speedgun 0.16.0` |
| `docs/pages/harness.md:1067` | the sentence that states the version a run writes |

The committed comparison baselines keep their own versions on purpose:
`harness_compare_cli_test.cpp:492` and `harness_compare_test.cpp:1191-1202` pin
that a `1.0` baseline compares against a `1.1` run and that a minor difference
does not stop a comparison. The bump moves neither of those facts: the minor
position stays a value the comparison reads and reports and never refuses on.

## Verification

| Behaviour | Test |
|---|---|
| A hunt document conforms to the edited schema at both nodes | `test/source/harness_hunt_json_test.cpp` through `SG_SCHEMA_PATH`, the pattern `harness_json_schema_test.cpp:21-22,57` states |
| A hunt key out of order is a finding naming the pointer | `harness_hunt_json_test.cpp`, following the planted-order cases of `harness_json_schema_test.cpp:102` |
| A key outside a closed hunt node is a finding | `harness_hunt_json_test.cpp` |
| A pre-1.4 document, hunt keys absent, stays valid and loads | `harness_json_load_test.cpp` passes unchanged, plus the pre-024 document case of `harness_hunt_json_test.cpp` |
| A present hunt object with a missing key is refused by the reader | `harness_hunt_json_test.cpp` |
| The written `schemaVersion` equals the version the page states | the four rows of the table above move with the bump, and `harness_json_report_test.cpp:278,382` keeps checking conformance over written documents |

The schema file is data, so no coverage line attaches to it; the branch that
reports a finding for an unlisted key is covered by the existing
`harness_json_schema_test.cpp` cases and by the two hunt cases above, and no
exclusion marker joins any file (FR-091).
