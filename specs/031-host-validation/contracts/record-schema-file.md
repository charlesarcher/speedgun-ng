# Contract: the pinned record schema

Phase 1 output. `docs/schema/host-record-v1.json` is the guard the
reader and the writer share (spec FR-019 to FR-022). It follows
the standing schema rules of
`specs/023-json-reports/contracts/schema-file.md`: the file is plain
JSON, it names the document kind in its `title` the way the standing
files name theirs, it states the version, and the checker reads a fixed
set of keywords.

## What the file states

| Part | Content |
| --- | --- |
| document kind | `host-record`, the label the file carries in its `title` and the reader quotes in the line for a broken record; plain text, read by no keyword (FR-019) |
| version | `1.0`, the format version `recordVersion` must carry; the record names that version, the file names the kind (FR-019) |
| key sets | the four root keys and the keys of each nested object |
| `x-order` | `["recordVersion", "cpu", "machine", "evidence"]`, and the order inside each object |
| `required` | one array per object naming every key of that object, over the six objects that carry keys: root, `cpu`, `machine`, `machine.l3[]`, `machine.nodes[]` and `evidence[]` (FR-020, FR-021) |
| kinds | `string`, `integer`, `array`, one kind per key, with `cpu.stepping` and `machine.nodes` the two nullable keys, each stated as a `type` list naming its kind and `null` (FR-013, FR-020) |
| closed objects | every object states that no key beyond its set stands |
| array items | the row shapes of `machine.l3`, `machine.nodes` and `evidence` |

Every key stands present in a closed object, so an unanswered value
stands `null`: the node block reads `null` exactly where the machine
memory reads unavailable, and the stepping reads `null` where the host
answers no leaf 1 (FR-013, FR-020).

An absent key reaches a report through no other keyword. `checkReport`
looks for a missing key in its `required` loop alone
(`source/harness/json_load.cpp:2275-2279`), so an object that states no
`required` array takes a document missing any key of it, and each of the
six states its own list (FR-020, FR-021).

## What the checker reads

`sg::detail::checkReport(schemaPath, document)`
(`source/harness/detail/json_report.hpp:322`, implemented at
`json_load.cpp:2362`) reads `type`, `required`, `x-order`,
`additionalProperties`, `enum`, `properties` and `items`, and root
duplicate keys (`source/harness/json_load.cpp:2115-2205`, `:2378`,
`:2397`). It reads no other keyword
(`specs/023-json-reports/contracts/schema-file.md`), so the schema states
nothing the reader cannot enforce. The standing files carry `$schema`,
`$id`, `title` and `description` beside those keywords and no kind key,
and this file follows them: the document kind is a label, so no
comparison reads it (FR-019, FR-020, FR-022).

`minItems` belongs beside that set, because it bounds nothing here: the
checker reads no array length, so a file that states `minItems` on a row
array states it for whoever reads the file, and the guard clause stays the
key set, the key order and the value kinds a record carries (FR-020,
FR-022).

The same function already guards a second document: the compare document
against `docs/schema/compare-v1.json`
(`test/source/harness_compare_cli_test.cpp:969`). This feature adds no
second checker (FR-022, R-03).

## What the guard test asserts

`test/source/harness_host_record_schema_test.cpp` reads the file through
the `SG_SCHEMA_PATH` compile definition, the pattern
`harness_json_report_test` sets in `test/CMakeLists.txt`, and asserts:

1. the key set and `x-order` of the schema equal the writer's call order,
   read from a document the writer produced, and the file names the kind
   `host-record` and the version `1.0` (FR-019, FR-021);
2. a planted document missing one key fails, run once per object, because
   each of the six objects states its own `required` array (FR-021);
3. a planted document with one added key fails, because the object stands
   closed (FR-020);
4. a planted document whose two root keys swap places fails (FR-021);
5. a planted document with one value retyped fails (FR-021);
6. a planted document with one root key twice fails (FR-021);
7. a document the writer produced passes, and re-reading its bytes
   yields the same values (FR-021, FR-023).

## Versioning

A change to the key set, the order, a kind or a nullability moves the
schema version and the writer's `recordVersion` in the same change, and
a record that carries the older version reads stale (FR-019, FR-025).
`docs/schema/report-v1.json` stands unchanged: the record is a separate
document with its own schema file (FR-040).
