# Contract: The Schema File and Its Checker

## The file

`docs/schema/report-v1.json` is a JSON Schema 2020-12 document, checked
into the repository and installed with the docs. It declares:

- `$schema`, `$id` and `title` at the root, and `additionalProperties`
  left open so a newer minor can add keys inside a major (FR-010).
- `schemaVersion` as the first required key, a string with the pattern
  `^1\\.[0-9]+$`.
- one `$defs` entry per entity of `data-model.md`: `benchmark`, `row`,
  `metricValue`, `statistics`, `context`, `noiseWarning`, `percentileCell`,
  `fitRow`, `statisticValue`, `rawColumns`.
- `required` lists that hold every key the format always writes, and
  `null` in a `type` list for every nullable field.
- `enum` lists that hold exactly the spellings of `data-model.md` E-06.

The key order inside each `properties` object is the order the writer
appends, and the file is written by hand in that order.

## The checker

No validator library enters the repository (FR-008, D-05). The harness
carries the oracle:

```cpp
// source/harness/detail/json_report.hpp
/**
 * @brief Check a written report against the schema document.
 *
 * \pre `schemaPath` names the schema file and `document` holds one JSON
 *      document.
 * \post The call answers void when every object of the document holds
 *       exactly the keys its schema entry declares, in the declared
 *       order, with a value of the declared kind.
 * \post The call answers an error naming the JSON pointer of the first
 *       disagreement when it finds one.
 */
[[nodiscard]] auto checkReport(std::string_view schemaPath,
                               std::string_view document)
    -> std::expected<void, std::string>;
```

The checker walks the schema's `properties` in declaration order and
compares, for each object of the document: the key set, the key order, and
the value kind against `type`. It reads `enum` lists for the fields that
carry one, and it treats a `null` member of a `type` list as a permitted
`null`. It reads no other keyword, and the schema file stays free to carry
the keywords it carries.

`test/source/harness_json_schema_test.cpp` drives it over a document the
writer produced, and over planted documents with one disagreement each: a
missing key, an extra key inside a `required` object, a key out of order,
a value of the wrong kind, and an enum spelling outside its list. The test
target gains the harness include directory, the shape
`test/CMakeLists.txt:504` and `:522` already use, so the test reaches the
checker without reaching the vendored headers.

## Why the checker is harness code

The checker parses JSON, and `specs/004-vendor-simdjson` FR-013 keeps
the vendored headers inside `source/` translation units. A test
translation unit cannot include them, so the oracle lives in the harness
unit that may, and the test drives it. The same rule keeps the loader in
the harness unit.

## Privacy

The schema file is data. It adds no header, no link and no symbol. The
two vendored proofs stay green over the feature:

- `simdjson_purity_scan` (`test/CMakeLists.txt:88`)
- `simdjson_nm_proof` (`test/CMakeLists.txt:89`)

The harness link that brings the headers in is
`PRIVATE $<BUILD_INTERFACE:simdjson::simdjson>`, so the install tree, the
package files and the dynamic symbol table of the harness archive stay
free of simdjson (FR-048, and specs/004 FR-014, FR-015 and FR-016).

## Version bump rule

The schema file is named `report-v1.json` and its `schemaVersion` is
`1.0`. A key added inside major 1 moves the minor to `1.1` and leaves the
file name. A renamed, removed or retyped key is a major change: it takes a
new file, `report-v2.json`, and the loader of D-08 refuses major 2 with an
error that names the version it reads.
