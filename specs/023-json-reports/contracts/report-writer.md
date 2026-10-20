# Contract: The Report Writer

## The internal call

```cpp
// source/harness/detail/json_report.hpp
/**
 * @brief Write one JSON report document for one batch.
 *
 * \pre `data` is the value the runner answered for the batch that just
 *      finished, and `path` is not empty.
 * \post When the call answers true, the file at `path` holds one JSON
 *       document that `checkReport` accepts against
 *       `docs/schema/report-v1.json`, and that document carries every
 *       entry of `data.results` in the order it holds them.
 * \post When the call answers false, no file exists at `path`, the
 *       document it did not write left nothing behind, and the console
 *       output of the run is unchanged.
 * \post The call reads no clock, samples no counter and touches no
 *       recorder.
 */
[[nodiscard]] auto writeReport(const std::string& path,
                               const ReportData& data) -> bool;
```

The call runs once per process, after the last console row of the batch
(D-09). It is internal to the harness archive: no public header names it.

## Key order

The writer appends keys in the order `data-model.md` lists them, and that
order is the order `docs/schema/report-v1.json` declares them. No key is
written conditionally except `raw`, which stands absent when the run
retained no chunk columns. The schema checker reads the same order, so a
key appended out of turn fails the gate. A general validator that ignores
order would let it pass.

## Numbers

Every number goes through the vendored builder's number path
(`json_string_builder-inl.h:749`), which answers the shortest decimal that
reads back to the same `double`. The writer adds no formatting of its own:
no fixed precision, no unit scaling beyond the nanosecond values the
result already holds, and no rounding.

Every reported double is finite by the postconditions of the row builders
that produced it, so the writer carries no non-finite guard and no test
covers one. A guard no test reaches is a branch the coverage gate reads as
uncovered (the rule 021 records in its implementation notes).

Integers write as JSON integers, not as decimals: `iterations`,
`chunkCount`, `chunkIterations`, `endpointSamples`, `pauseSamplesFolded`,
`samples`, `index`, `runnableCpus`.

## Strings

Every string goes through the builder's escaping path. The writer copies
the label text, the reason text and the placement text whole, with no
column cut: the console's cut at `report.cpp:189` is a print concern, and
the loader must answer what the run held.

## The file write

The writer opens `path` in binary write mode, writes the builder's buffer
once, and closes. It answers false when the open fails, when the write
fails, or when the close reports an error. It creates no directory, so a
path whose parent is missing is a false answer with the path named on
standard error in one line:

```text
speedgun-ng: cannot write report to <path>
```

No partial file stands on a false answer: the writer builds the whole
document in memory first, so the only failure that can leave bytes behind
is a disk that fills mid-write, and the writer removes the file in that
case before answering false.

## What the writer never does

- It never writes a date, a time, a host name, a pid or anything else a
  clock or a system query answers (FR-005, FR-021).
- It never changes a byte of console output, at any setting of the new
  options (FR-003).
- It never runs inside a timed window, and it allocates nothing while one
  is open (FR-032).
- It never re-runs a benchmark to fill a field.
