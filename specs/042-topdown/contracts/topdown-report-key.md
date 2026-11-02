# Contract: the `topdown` entry key, its order and its stamp

**Feature**: specs/042-topdown | **Files**: `docs/schema/report-v1.json` with
`source/harness/json_report.cpp:kSchemaVersion` | **Audit point**: `65c48be9`
**Spec**: FR-032 to FR-036, FR-047, D-12, D-17

## What this contract covers

One key of the benchmark entry object, the order it stands in, the key lists it
stays out of, and the version stamp that moves with it.

## The order the schema states

| Place | At `65c48be9` | After this feature |
|---|---|---|
| entry `x-order` (`:383-407`) | 23 names, `"hunt"` last (`:406`) | 24 names, `"topdown"` right after `"hunt"` |
| entry `required` (`:408-427`) | 19 names | unchanged |
| entry `additionalProperties` (`:429`) | `false` | `false` |
| entry `properties` (`:430-903`) | the `"hunt"` object closes it (`:827-902`) | a `"topdown"` object after `"hunt"` |
| root `x-order` (`:7`) | 8 names ending `"hunt"` | unchanged: the record stands per entry |
| `kSchemaVersion` (`json_report.cpp:43`) | `"1.11"`, appended at `:992` | `"1.12"` |

The checker enforces the order. `checkNode` answers
`<pointer>/"<key>": the key stands out of the order the schema states`
(`source/harness/json_load.cpp:2545-2556`) and, inside a closed object,
`<pointer>/"<key>": the schema lists no key of that name` (`:2534-2542`). A
writer that emits `topdown` before `hunt` fails
`test/source/harness_json_schema_test.cpp`, which runs the writer's own document
through `checkReport` (`json_load.cpp:2613`, called at `:57`).

## The object

```json
"topdown": {
  "type": "object",
  "x-order": ["supported", "reason", "reference", "directory",
              "files", "asked_level", "level", "nodes", "refused"],
  "required": ["supported", "reason", "reference", "directory",
               "files", "asked_level", "level", "nodes", "refused"],
  "additionalProperties": false,
  "properties": {
    "supported":   { "type": "boolean" },
    "reason":      { "type": "string" },
    "reference":   { "type": "string" },
    "directory":   { "type": "string" },
    "files":       { "type": "array", "items": { "type": "object",
                     "x-order": ["name", "manifest_hash"],
                     "required": ["name", "manifest_hash"],
                     "additionalProperties": false } },
    "asked_level": { "type": "integer" },
    "level":       { "type": "integer" },
    "nodes":       { "type": "array", "items": { "type": "object",
                     "x-order": ["name", "vendor_name", "parent", "level",
                     "value", "availability", "threshold"],
                     "required": ["name", "vendor_name", "parent", "level",
                     "value", "availability", "threshold"],
                     "additionalProperties": false } },
    "refused":     { "type": "array", "items": { "type": "object",
                     "x-order": ["name", "kind", "reason"],
                     "required": ["name", "kind", "reason"],
                     "additionalProperties": false } }
  }
}
```

Six rulings the fragment carries:

- Inner names follow the entry `hunt` object, which spells `pass_count` in
  snake case (`:829-844`); the entry's own outer keys stay camelCase.
- `files` holds one record per contributing file: FR-015 asks the hash of each
  file whose rows reached the tree, so the one `manifestHash` field the harness
  plan lists for `TopdownRecord` is short of the requirement (D-12).
- `nodes[].availability` reuses the six words of the counter columns
  (`:577-586`, `kAvailabilityWords` at
  `source/harness/detail/json_report.hpp:57-64`); a gap node
  carries `"value": null` and no number (FR-030).
- `nodes[].threshold` is the table text, and the report states no verdict on it
  (FR-017, FR-032).
- An unsupported host writes `"supported": false` with the reason and empty
  arrays (FR-033, D-14).
- Each item object closes itself with `additionalProperties: false`, the way the
  hunt item object does (`:890-899`). An open item object is read loosely: the
  reader answers for keys the schema lists and passes over the rest
  (`json_load.cpp:2536-2543`), so a misspelled key would load as nothing and a
  `null` value would carry no type.

## What stays out of `required`

The key `topdown` joins no `required` list. Three facts make it so, and each is
a gate on its own:

- FR-035 asks a run without the option to write the document the same run wrote
  before this feature, version stamp aside.
- A key added to a closed object's `required` list breaks every stored document:
  `checkNode` answers `<where>: the key '<name>' stands missing`
  (`json_load.cpp:2526-2529`, text at `:60-63`).
- The reader lane is `optionalField` (`json_load.cpp:85-93`), the lane `hunt`
  takes at `:2342-2349` and the pass-disclosure keys before it (comment at
  `:79-84`).

Inside the object every key is required, the way the entry `hunt` object lists
all six of its keys (`:837-844`): the writer always writes the whole record, so
a document that carries the key and lacks a field is corrupt and reads as one.
The reader answers `<where>: expected an object, found <kind>` for a key of the
wrong kind (`json_load.cpp:52-58`), and it never refuses a key it does not
know: only the schema checker does.

## The stamp and the eight sites

Six sites assert or embed the current minor and move to `1.12`. Two plant a
later minor as their subject and move to `1.13` (FR-036, D-17).

| Site | Case | Moves to |
|---|---|---|
| `test/source/harness_json_report_test.cpp:546` | `theDocumentStampsTheMinorThisFeatureRequires` (`:543`) | 1.12 |
| `test/source/harness_json_schema_test.cpp:186` | `entryHuntKeysPairAtStampedMinor` (`:158`) | 1.12 |
| `test/source/harness_json_load_test.cpp:829` | `kMutations[0]` (`:828`), read by `brokenDocumentsRefuseWithPointers` (`:1080`) | 1.12 |
| `test/source/harness_hunt_json_test.cpp:1537` | `schemaConformance` (`:1534`) | 1.12 |
| `test/source/harness_noise_report_test.cpp:426` | `theWrittenTracerDocumentPassesTheSchema` (`:415`) | 1.12 |
| `test/source/harness_noise_report_test.cpp:457` | file-scope `kDocumentFixture` (`:456-461`), compared by `theTracerLessDocumentMatchesTheFixtureBytes` (`:463`) | 1.12 |
| **`test/source/harness_json_load_test.cpp:107`** | `versionRuleGatesTheReader` (`:92`) | **1.13** |
| **`test/source/harness_json_load_test.cpp:1199`, `:1202`** | `theHeaderFieldsAndTheStoredWarningsLoad` (`:1172`) | **1.13** |

Both plants move silently. `:107` stamps a document `1.12` and checks "a later
minor of the same major loads"; once the writer stamps `1.12` that plant holds
the current version and the case passes while proving nothing. `:1199` rewrites
a written document's stamp to `1.12` and checks the reader returns it (`:1202`).
Both must name `1.13` to hold the gate they claim.

Seven further lines carry the old minor in prose and move with the stamp, so the
tree states one minor: `harness_hunt_json_test.cpp:1538`,
`harness_json_report_test.cpp:547`, `harness_json_schema_test.cpp:157` and
`:187`, and `harness_noise_report_test.cpp:413`, `:427` and `:455`.

The same change carries the three numbers FR-047 names: `VERSION 0.28.0` to
`0.29.0` (`CMakeLists.txt:45`), harness `SOVERSION 19` to `20` (`:375`, chain at
`:369-374`) and the counters archive `SOVERSION 4` to `5` (`:110`) for the new
public header. One case pins all three: `theVersionBindingsReadTheMovedNumbers`
(`test/source/harness_cli_table_test.cpp:1382-1393`).

## Verification

| Behaviour | Test |
|---|---|
| The written document passes the schema with the new key, and the key stands after `hunt` | `test/source/harness_json_schema_test.cpp`: the `checkReport` call at `:57` runs the order gate, while `entryHuntKeysPairAtStampedMinor` (`:158`) reads the hunt order and the stamp |
| A `topdown` key written before `hunt`, and an unlisted inner key, are each found | the planted-finding rows of `harness_json_schema_test.cpp` (`:111`) |
| Every field of a written record reads back equal, and an absent key reads as nothing | `test/source/harness_topdown_report_test.cpp`, `theRecordRoundTrips` and `aDocumentWithoutTheKeyLoads` (FR-034, FR-035) |
| A run without the option writes no `topdown` key at all, and the document stamps `1.12` | `aRunWithoutTheOptionWritesNoKey` in the same file, with `harness_json_report_test.cpp:546` (FR-035) |
