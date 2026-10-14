# Contract: The complexity fit and the aggregate-only modes

`BenchmarkHandle` (`include/speedgun-ng/benchmark.hpp`) gains the
fit selection, the mode calls, and `State` gains the problem size
(FR-016 to FR-023).

## The fit calls (FR-019, FR-020)

```cpp
// State member
auto setComplexityN(std::int64_t problemSize) -> void;

// BenchmarkHandle members, each returning the handle
enum class BigO : std::uint8_t
{ NONE, ONE, N, N_SQUARED, N_CUBED, LOG_N, N_LOG_N, AUTOMATIC };
using BigOFun = std::function<double(double n)>;

auto complexity(BigO bigO) -> BenchmarkHandle&;
auto complexity(BigOFun function) -> BenchmarkHandle&;
```

| Rule | Requirement |
| --- | --- |
| Each call shall stand behind the standing before-the-first-run setter precondition. | FR-046; the standing `SG_REQUIRE` on the handle setters in `source/harness/registry.cpp` |
| The curve of `ONE`, `N`, `N_SQUARED`, `N_CUBED`, `LOG_N`, and `N_LOG_N` shall match the cited class list. | FR-020; `BigO` in `statistics.h` |
| `AUTOMATIC` shall try each curve and keep the least RMS, with the constant curve as the fallback. | FR-021; the `MinimalLeastSq` auto path in `src/complexity.cc` |

## The fit computation (FR-021, FR-022, FR-023)

| Rule | Requirement |
| --- | --- |
| One fit per family over its instances, per fitted quantity: the primary time, each named user counter, each attached metric. | FR-022, clarification 2026-10-14 |
| The coefficient shall be the least-squares solution of the absolute residuals, the closed form `k = sum(t * f(N)) / sum(f(N)^2)`. | FR-021; `LeastSquaresComplexity` in `src/complexity.cc` |
| The RMS shall be `sqrt(sum((t - k * f(N))^2) / n) / mean(t)` times 100, printed unscaled by the unit multiplier. | FR-022 |
| The instance N shall be the `setComplexityN` value when set, else the instance's range value. | FR-019, research D-08 |
| A family with fewer than two instances shall state the named reason in place of rows. | FR-023 |
| An instance without a positive N shall state the named reason in place of rows. | FR-023; the `ComputeBigO` guards in `src/complexity.cc` |
| A quantity missing from any instance shall state the named reason for that quantity's rows. | spec Edge Cases |

## The fit rows (FR-022)

```text
<family>  BIG_O  <quantity> <curve> (<coefficient>)
<family>  RMS    <quantity> <rms-percentage>%
```

| Rule | Requirement |
| --- | --- |
| The time row shall carry its coefficient in the row unit; a counter or metric row shall carry its coefficient in that quantity's value. | FR-022 |
| The rows shall print after the family's aggregate row, one pair per quantity, in the order: time, counters in open order, metrics in attach order. | FR-022, research D-09 |
| The rows shall travel as one synthetic fit result per family through the same report pipeline. | research D-09 |

## The mode calls (FR-016, FR-017, FR-018)

```cpp
// BenchmarkHandle members, each returning the handle
auto reportAggregatesOnly() -> BenchmarkHandle&;
auto displayAggregatesOnly() -> BenchmarkHandle&;
```

| Rule | Requirement |
| --- | --- |
| `reportAggregatesOnly` shall restrict every report output of that family to the aggregate rows; while the console is the only output it reaches the console. | FR-016 |
| `displayAggregatesOnly` shall restrict the console display alone, leaving every other output channel the mode is defined against untouched. | FR-017 |
| While the console is the only channel, both modes shall print the same trimmed console. | FR-017 |
| Neither mode shall hide a row that has no aggregate: a single-run instance prints its one row under either mode. | FR-018 |
| Neither mode shall gain a command-line form in this feature. | FR-018, Q-5 |

## Consumers

`test/source/harness_complexity_test.cpp` asserts the fit against
scripted curves; `test/source/harness_aggregates_only_test.cpp` asserts
the mode matrix; `test/example_run.sh` covers the docs examples (SC-04,
SC-05).
