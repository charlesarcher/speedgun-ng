#include <cstdio>

#include "detail/internal.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_core.hpp"

namespace sg::detail
{

auto availabilityName(const sg::counters::Availability availability) -> const
    char*
{
  switch (availability) {
    case sg::counters::Availability::COUNTABLE:
      return "countable";
    case sg::counters::Availability::PERMISSION_BLOCKED:
      return "permission-blocked";
    case sg::counters::Availability::NOT_ENCODABLE:
      return "not-encodable";
    case sg::counters::Availability::ABSENT:
      return "absent";
    case sg::counters::Availability::SCOPE_REFUSED:
      return "scope-refused";
    case sg::counters::Availability::GAP:
      return "gap";
  }
  return "unknown";
}

auto printContext() -> void
{
  // R-08: the catalog publishes no host or cpu model string at this
  // head, so those fields stay out of the line (FR-035).
  std::printf("speedgun-ng %s build=%s\n", SG_PROJECT_VERSION, SG_BUILD_TYPE);
}

auto printResult(const BenchmarkResult& result) -> void
{
  if (result.outcome == RunOutcome::FAILED) {
    std::printf(
        "%-24s FAILED: %s\n", result.name.c_str(), result.reason.c_str());
    return;
  }
  if (result.outcome == RunOutcome::SKIPPED) {
    // FR-031: the reason prints, and no statistics print.
    std::printf(
        "%-24s SKIPPED: %s\n", result.name.c_str(), result.reason.c_str());
    return;
  }

  for (const auto& row : result.rows) {
    if (row.kind == RowKind::AGGREGATE) {
      std::printf("%-24s %10s  time/iter (ns) mean=%9.3f median=%9.3f "
                  "sd=%9.3f cv=%8.4f min=%9.3f max=%9.3f n=%llu\n",
                  result.name.c_str(),
                  "AGGREGATE",
                  row.timeStats.mean,
                  row.timeStats.median,
                  row.timeStats.sampleStdDev,
                  row.timeStats.coefficientOfVariation,
                  row.timeStats.min,
                  row.timeStats.max,
                  static_cast<unsigned long long>(row.timeStats.samples));
      for (std::size_t column = 0; column < row.metricStats.size(); ++column) {
        const auto& stats = row.metricStats[column];
        const char* label = column < result.metricLabels.size()
            ? result.metricLabels[column].c_str()
            : "metric";
        std::printf("%-24s %10s  %s mean=%9.3f median=%9.3f sd=%9.3f "
                    "cv=%8.4f min=%9.3f max=%9.3f n=%llu\n",
                    result.name.c_str(),
                    "",
                    label,
                    stats.mean,
                    stats.median,
                    stats.sampleStdDev,
                    stats.coefficientOfVariation,
                    stats.min,
                    stats.max,
                    static_cast<unsigned long long>(stats.samples));
      }
      continue;
    }

    std::printf("%-24s iterations=%10llu  time/iter (ns)=%9.3f  "
                "overhead floor (ns)=%9.3f  endpoint samples=2",
                result.name.c_str(),
                static_cast<unsigned long long>(row.iterations),
                row.timePerIterationNs,
                row.overheadFloorNs);
    for (std::size_t column = 0; column < row.metrics.size(); ++column) {
      const auto& metric = row.metrics[column];
      const char* label = column < result.metricLabels.size()
          ? result.metricLabels[column].c_str()
          : "metric";
      std::printf(
          "  %s=%9.3f ratio=%.3f scaled=%d gap=%d",
          label,
          metric.value,
          metric.runningRatio,
          metric.scaled ? 1 : 0,
          metric.availability == sg::counters::Availability::GAP ? 1 : 0);
      if (metric.availability != sg::counters::Availability::COUNTABLE
          && metric.availability != sg::counters::Availability::GAP)
      {
        std::printf(" refusal=%s", availabilityName(metric.availability));
      }
    }
    std::printf("\n");
  }
}

}  // namespace sg::detail
