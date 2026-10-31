# Contract: The Comparison Call

## The public call

```cpp
// include/speedgun-ng/benchmark.hpp
/**
 * @brief The settings one comparison applies (specs/026 FR-045, FR-064).
 */
struct CompareOptions
{
  bool strict = false;
};

/**
 * @brief Compare two loaded report values under a list of rules.
 *
 * \pre `baseline` and `contender` are values `loadReport` answered, or
 *      values built to the same shape: `noiseWarnings`, when non-empty,
 *      holds one entry per entry of `results`
 * \post When the call answers a value, its `entries` hold one entry per
 *       benchmark per rule and `passed` states the verdict of FR-044
 *       and FR-045
 * \post When the call answers an error, the text names the fault and
 *       stands non-empty, and no partial entries reach the caller
 *       (FR-003)
 */
[[nodiscard]] SPEEDGUN_NG_EXPORT auto
compareReports(const ReportData& baseline, const ReportData& contender,
               const std::vector<CompareRule>& rules,
               const CompareOptions& options)
    -> std::expected<CompareResult, std::string>;
```

An empty `rules` list applies the default rule
`time@median:lower:5%` (spec D-5, FR-033). `options.strict` moves the
verdict only: a `NOISY` row fails the run when it stands and passes when
it does not, and the state word never changes for it (FR-044, FR-045).
The settings stand in `CompareOptions`, so a later setting grows the call
with the exported signature unchanged (the owner, 2026-10-21).

## Validation before the comparison

The call walks every rule over its scope before comparing anything
(FR-035). The scope of a rule is the matched benchmark names its
filter selects; an empty filter selects all matched names. The first
fault stops the call (FR-003). The faults, in the order the walk
reaches them:

| Fault | Named in the text |
|---|---|
| the two `schemaVersion` majors differ, or one text reads no major (FR-051, D-13) | both version texts |
| a rule filter matches no benchmark in either report (FR-039) | the rule |
| the value is absent from one report: a missing column, a missing statistic name, a percentile point one report lacks, a cell that answers nothing, an aggregate named on a column with no statistics record in a repeated run (FR-036, FR-037, FR-038, spec D-18) | the rule, the benchmark, the report |
| a metric whose `availability` is not `COUNTABLE` in either report (FR-037) | the rule, the benchmark, the report |
| a zero baseline magnitude under a relative threshold (FR-031) | the rule, the benchmark |
| a rule whose whole filtered scope holds only results with no readable row (FR-040) | the rule |

A benchmark whose rows hold no readable row answers `SKIPPED` inside a
scope that also holds readable results (spec D-20); it never faults by
itself.

## The value and the verdict

The value resolution, the row selection and the units stand in
[data-model.md](./data-model.md) E-08 and E-09. The comparison reads
the aggregate from the statistics record the report carries for the
value (FR-016) and answers the single row's value for every aggregate
of a single-row run (FR-017). The noise guard marks `NOISY` before the
verdict words answer (D-11).

## The result values

`CompareResult`, `CompareEntry` and `CompareState` are public types of
the same header, because the call answers them; their fields and
invariants stand in [data-model.md](./data-model.md) E-02, E-03 and
E-04. The call is public API of the harness archive, so the harness
`SOVERSION` moves 8 to 9 (D-17).

## What the call never does

- It never reads a file: the documents arrive as loaded values
  (FR-006).
- It never runs a benchmark, reads a clock or samples a sensor
  (FR-007).
- It never repairs a value: no recomputed statistic, no re-ranked
  percentile, no substituted default.
- It never prints: the command line owns the table and the fault line.
