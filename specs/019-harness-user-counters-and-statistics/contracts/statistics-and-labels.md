# Contract: Labels and custom statistics

`State` gains `setLabel`, and `BenchmarkHandle`
(`include/speedgun-ng/benchmark.hpp:826`) gains `addStatistic`. Both
write family or run state consumed by the report (FR-011 to FR-015).

## The label call (FR-011, FR-012)

```cpp
// State member
auto setLabel(std::string_view label) -> void;
```

| Rule | Requirement |
| --- | --- |
| The row carrying a non-empty label shall print ` label=<text>` after the name column, verbatim. | FR-011, clarification 2026-10-14 |
| A row with no label shall print no label field, and the name column shall keep its H1 bytes. | FR-011 |
| The aggregate row shall carry the label only when every repetition's label is equal, the cited shared-label rule. | FR-012; `src/statistics.cc:178-186` |
| A label containing a newline character is a precondition violation at the call. | spec Edge Cases |
| The last write before the row builds wins within one repetition. | research D-04 |

## The statistic call (FR-013 to FR-015)

```cpp
// BenchmarkHandle member, returning the handle for chaining
enum class StatUnit : std::uint8_t { TIME, PERCENTAGE };
using StatisticFunc =
    std::function<bool(const std::vector<double>& values, double& result)>;

auto addStatistic(std::string_view name,
                  StatUnit unit,
                  const StatisticFunc& function) -> BenchmarkHandle&;
```

| Rule | Requirement |
| --- | --- |
| The statistic shall compute over the clean repetition values of the primary time, the same list the built-in `Statistics` consumes. | FR-013 |
| The printed fields of the aggregate shall carry the statistic among `mean=`, `median=`, `sd=`, `cv=`, `min=`, `max=`, `n=` with its name. | FR-013 |
| A `TIME` statistic shall print in the row's unit; a `PERCENTAGE` statistic shall print unscaled by the unit multiplier. | FR-014 |
| A function returning false shall print no value for that statistic while the run still reports. | FR-015 |
| A name equal to `mean`, `median`, `sd`, `cv`, `min`, or `max` is a precondition violation at the call. | spec Edge Cases |
| The call shall stand behind the `m_entry != nullptr && !m_entry->runStarted` precondition the existing setters use. | FR-046; `source/harness/registry.cpp:75-79` |

## Consumers

`test/source/harness_label_test.cpp` and
`test/source/harness_custom_stats_test.cpp` assert each rule; the
re-exec pattern holds the newline abort and the built-in-name abort
(SC-03).
