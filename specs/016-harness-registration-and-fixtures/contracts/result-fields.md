# Contract: result fields, suite grouping, and version effect

The result value `sg::BenchmarkResult`
(`include/speedgun-ng/benchmark.hpp:116`) gains the suite and case
pair. That value is where the split becomes readable; the console row
keeps the H1 shape (Q-6).

## The result value (FR-021)

```cpp
struct BenchmarkResult {
  std::string name;        // the full instance name (H1, benchmark.hpp:118)
  std::string suite;       // new (R-07)
  std::string caseName;    // new (R-07); `case` is a C++ keyword (N-12)
  RunOutcome outcome;
  std::vector<ResultRow> rows;
  std::string reason;
  std::vector<std::string> metricLabels;
};
```

| Rule | Requirement |
| --- | --- |
| The suite and the case of its instance shall be fields of `BenchmarkResult`; the pair spells `suite` and `caseName` (R-07). | FR-021 |
| The suite shall be the family name up to its first `/`, and the fixture class name for a fixture instance (R-08, Q-1). | FR-021 |
| The case shall be the instance name with the leading suite and its `/` removed (R-08). | FR-021 |
| An instance name equal to its suite keeps that name as its case (R-08). | FR-021 |
| The pair shall be unique for each instance, following the unique instance name of FR-012. | FR-021 |
| A later JSON report of the roadmap shall reuse these two fields. | FR-021 |
| `ResultRow` (`benchmark.hpp:101`) carries neither field; it is one repetition or aggregate. | FR-021 |

## Report order (R-12)

| Rule | Requirement |
| --- | --- |
| After expansion the instance list shall group stably by suite in first-appearance order, and the runner shall walk that order. | FR-021 |
| Rows within one suite shall keep registration order. | FR-021 |
| The console row shall keep the H1 single name column carrying the full instance name (`specs/015-benchmark-harness-core/contracts/cli.md`). | FR-021 |

## Derivation examples (R-08)

```text
family name       instance name           suite          case
fib/bits          fib/bits/1/8/64         fib            bits/1/8/64
QueueFixture      QueueFixture/push/8     QueueFixture   push/8
plain             plain                   plain          plain
```

| Rule | Requirement |
| --- | --- |
| The `plain` row shows the equal-name rule: the instance name equals its suite, so it stays its case (R-08). | FR-021 |
| A test observes the suite order and the pair through the two fields of the result value (SC-007). | FR-021 |

## Layout change and `SOVERSION` (FR-026)

| Artifact | At audit point | After H2 | Reason |
| --- | --- | --- | --- |
| `project(speedgun-ng VERSION)` | 0.6.0 | 0.7.0 | FR-026, the next minor |
| `speedgun-ng` archive `SOVERSION` | 2 | 2 | the counters surface changes nothing (R-16) |
| `speedgun-ng_harness` archive `SOVERSION` | 2 | 3 | `State` gains argument storage and `BenchmarkResult` gains two fields (FR-026) |
| constitution | 2.18.0 | 2.18.0 | no amendment (R-16) |

| Rule | Requirement |
| --- | --- |
| `State` gains `std::span<const std::int64_t> m_arguments`, so its layout changes (R-04). | FR-026 |
| `BenchmarkResult` gains two fields, so its layout changes (R-07). | FR-026 |
| The hand-kept `SOVERSION` rule of `CMakeLists.txt:41-47` binds the harness archive at `CMakeLists.txt:211-218`; the number shall rise from 2 to 3 (R-16). | FR-026 |
| The counters archive keeps `SOVERSION` 2. | FR-026 |
