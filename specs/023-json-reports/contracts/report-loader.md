# Contract: The Report Loader

## The public call

```cpp
// include/speedgun-ng/benchmark.hpp
/**
 * @brief Read a report document written by this harness.
 *
 * \pre `path` is not empty: an empty path is a contract violation, the
 *      rule the spec's contract-boundary constraint states. A path the
 *      file system refuses is an input error the call answers.
 * \post When the call answers a value, that value holds one entry per
 *       benchmark of the document, in document order, with the raw chunk
 *       columns of the document in its `rowRaw` and `contextRaw` fields.
 * \post When the call answers an error, the string names the JSON
 *       pointer of the fault and what the reader expected there.
 * \post The call throws nothing to the caller and aborts nothing.
 */
[[nodiscard]] SPEEDGUN_NG_EXPORT auto loadReport(const std::string& path)
    -> std::expected<ReportData, std::string>;
```

The call is public API of the harness archive, so the harness
`SOVERSION` moves 7 to 8 (D-16). `ReportData` and `RowRaw` are public
types of the same header, because the call answers them.

## What a loaded value answers

The loader answers the same result types the run produced, so a loaded
value written again by `writeReport` gives the same bytes the file already
held, provided the raw setting matches what the file carries (FR-026).
The rule the tests hold:

- `writeReport(p, loadReport(p).value())` over a file the writer produced
  with `--json-raw=chunks` gives a byte-identical file.
- The same holds for a file written with the raw setting at `none`.
- A field the document omits that the format allows to be absent (`raw`)
  answers the empty value the field documents.

## Version rule

`schemaVersion` splits at the first `.`. The loader reads major `1`.

| Document | Answer |
|---|---|
| major `1`, any minor | read, unknown keys ignored |
| major `2` or higher | error naming the version read and `1.x` as what it reads |
| missing or non-string `schemaVersion` | error naming the pointer `/schemaVersion` |

A newer minor is accepted because the format only adds keys inside a minor
(FR-010), and an unknown key is skipped and left unread. The reader
walks the fields it knows by name, so a document whose keys stand in
another order loads to the same value (FR-009).

## Error forms the tests cover

Each form answers an error whose string names the JSON pointer:

| Input | Named fault |
|---|---|
| a path that does not exist | the path |
| an empty file | the document root |
| a truncated document | the pointer where the parse stopped |
| a document that is not an object | `/` |
| `benchmarks` not an array | `/benchmarks` |
| a missing required key | the pointer of the missing key |
| a number where a string belongs | the pointer of the field |
| an enum spelling outside the closed list | the pointer of the field |
| a `raw` column whose length disagrees with `chunkCount` | the pointer of the column |
| a `null` where a number is required | the pointer of the field |

No error form leaves a partial value to the caller, and no error form
calls `std::exit`: the caller decides.

## The null rule

A cell `{"available": false, "value": null}` loads to a `PercentileValue`
with `available` false and no value, the same shape the run answered
(D-12). A `null` in a field the format marks nullable (`reason`, `label`,
`counterDisclosure`, `fit`, `noiseWarning`, `timeStats`, an entry of
`metricStats`, `cpuTimePerIterationNs`) loads to the empty optional or the
default the field documents.

## What the loader never does

- It never repairs a document: no default substituted for a missing
  required key, no re-ranked percentile, no recomputed statistic.
- It never reads a second file, and it never reads the schema file: the
  schema is the gate's oracle, and the loader reads no file but its input.
- It never writes anything.
