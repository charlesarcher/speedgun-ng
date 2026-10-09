#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <expected>
#include <string>
#include <vector>

#include "detail/calibration.hpp"
#include "detail/internal.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_measurement.hpp"
#include "speedgun-ng/counters_system.hpp"
#include "speedgun-ng/dbc.hpp"

namespace
{

// FR-007 and FR-011 carry the defaults; FR-010 and FR-016 bound the
// growth, and detail/calibration.hpp owns that rule.
constexpr std::int64_t kDefaultMinTimeNs = 500'000'000;
constexpr std::int64_t kDefaultWarmupNs = 0;
constexpr std::uint64_t kDefaultRepetitions = 1;

// A metric column of one run: the label, the FR-022 per-iteration
// decision, the erased spine, and the availability the catalog
// disclosed before the run (FR-023).
struct MetricColumn
{
  std::string label;
  bool perIteration = false;
  bool unavailable = false;
  sg::counters::Availability availability =
      sg::counters::Availability::COUNTABLE;
  sg::counters::detail::ExprCore core;
};

auto splitAddress(std::string_view address)
    -> std::pair<std::string_view, std::string_view>
{
  const std::size_t slash = address.find_last_of('/');
  if (slash == std::string_view::npos) {
    return {address, {}};
  }
  return {address.substr(0, slash), address.substr(slash + 1)};
}

auto statisticsOf(std::vector<double> values) -> sg::Statistics
{
  sg::Statistics stats;
  if (values.empty()) {
    return stats;
  }

  std::sort(values.begin(), values.end());
  stats.samples = static_cast<std::uint64_t>(values.size());
  stats.min = values.front();
  stats.max = values.back();

  double sum = 0.0;
  for (const double value : values) {
    sum += value;
  }
  stats.mean = sum / static_cast<double>(values.size());

  const std::size_t n = values.size();
  stats.median =
      (n % 2 == 1) ? values[n / 2] : (values[n / 2 - 1] + values[n / 2]) / 2.0;

  double squared = 0.0;
  for (const double value : values) {
    squared += (value - stats.mean) * (value - stats.mean);
  }
  stats.sampleStdDev =
      n > 1 ? std::sqrt(squared / static_cast<double>(n - 1)) : 0.0;
  stats.coefficientOfVariation = std::fpclassify(stats.mean) != FP_ZERO
      ? stats.sampleStdDev / stats.mean
      : 0.0;
  return stats;
}

}  // namespace

namespace sg::detail
{

auto Runner::run(RegistryEntry& entry) -> BenchmarkResult
{
  entry.runStarted = true;

  BenchmarkResult result;
  result.name = entry.name;

  auto& sys = sg::counters::System::local();
  const std::int64_t minTimeNs =
      entry.minTimeNs.value_or(m_options.minTimeNs.value_or(kDefaultMinTimeNs));
  const std::int64_t warmupNs = entry.warmupTimeNs.value_or(
      m_options.warmupTimeNs.value_or(kDefaultWarmupNs));
  const std::uint64_t repetitions = entry.repetitions.value_or(
      m_options.repetitions.value_or(kDefaultRepetitions));
  const std::optional<std::uint64_t> fixedIterations =
      entry.fixedIterations ? entry.fixedIterations : m_options.fixedIterations;
  const bool dryRun = m_options.dryRun;

  auto fail = [&](const std::string& reason) -> BenchmarkResult
  {
    result.outcome = RunOutcome::FAILED;
    result.reason = reason;
    return result;
  };

  const auto machine = sys.object("machine");
  if (!machine.has_value()) {
    return fail("the system publishes no machine object: "
                + machine.error().message);
  }
  const auto monotonic = machine->counter<sg::counters::Dim<1, 0>>("monotonic");
  if (!monotonic.has_value()) {
    return fail("machine/monotonic does not resolve: "
                + monotonic.error().message);
  }
  const auto threadCpu =
      machine->counter<sg::counters::Dim<1, 0>>("thread_cpu");
  if (!threadCpu.has_value()) {
    return fail("machine/thread_cpu does not resolve: "
                + threadCpu.error().message);
  }

  sg::counters::Expression<sg::counters::Dim<1, 0>> realTime(*monotonic);
  sg::counters::Expression<sg::counters::Dim<1, 0>> decisionTime(*threadCpu);

  std::vector<MetricColumn> columns;
  columns.reserve(entry.metrics.size() + m_options.counterAddresses.size());
  for (const auto& seed : entry.metrics) {
    columns.push_back(MetricColumn {
        .label = seed.label,
        .perIteration = seed.perIteration,
        .unavailable = false,
        .availability = sg::counters::Availability::COUNTABLE,
        .core = seed.core,
    });
  }

  // R-04: an address-attached leaf resolves through the catalog and is
  // instantiated as the pair its unit names, so it reaches the
  // per-iteration rule of FR-022.
  for (const auto& address : m_options.counterAddresses) {
    const auto [objectPath, leafName] = splitAddress(address);
    MetricColumn column {
        .label = address,
        .perIteration = true,
        .unavailable = false,
        .availability = sg::counters::Availability::COUNTABLE,
        .core = {},
    };

    const auto object = sys.object(objectPath);
    if (!object.has_value()) {
      column.unavailable = true;
      column.availability = sg::counters::Availability::ABSENT;
      std::fprintf(
          stderr, "%s: %s\n", address.c_str(), object.error().message.c_str());
      columns.push_back(std::move(column));
      continue;
    }

    const sg::counters::CatalogEntry* found = nullptr;
    // counters() returns the catalog by value, so the vector has to
    // outlive the search: `found` points into it below.
    const auto catalog = object->counters();
    for (const auto& catalogEntry : catalog) {
      if (catalogEntry.name == leafName) {
        found = &catalogEntry;
        break;
      }
    }
    if (found == nullptr) {
      column.unavailable = true;
      column.availability = sg::counters::Availability::ABSENT;
      std::fprintf(stderr,
                   "%s: no such leaf in the catalog (FR-023)\n",
                   address.c_str());
      columns.push_back(std::move(column));
      continue;
    }

    const auto dimension = sg::counters::dimensionOf(found->unit);
    if (!dimension.has_value()) {
      column.unavailable = true;
      column.availability = sg::counters::Availability::ABSENT;
      std::fprintf(stderr,
                   "%s: %s\n",
                   address.c_str(),
                   dimension.error().message.c_str());
      columns.push_back(std::move(column));
      continue;
    }

    if (dimension->time == 1 && dimension->events == 0) {
      const auto leaf = object->counter<sg::counters::Dim<1, 0>>(leafName);
      if (!leaf.has_value()) {
        return fail(leaf.error().message);
      }
      const sg::counters::Expression<sg::counters::Dim<1, 0>> expression(*leaf);
      column.core = expression.core;
    } else {
      const auto leaf = object->counter<sg::counters::Dim<0, 1>>(leafName);
      if (!leaf.has_value()) {
        return fail(leaf.error().message);
      }
      const sg::counters::Expression<sg::counters::Dim<0, 1>> expression(*leaf);
      column.core = expression.core;
    }
    columns.push_back(std::move(column));
  }

  std::vector<const sg::counters::detail::ExprCore*> cores;
  cores.reserve(columns.size() + 2);
  cores.push_back(&realTime.core);
  cores.push_back(&decisionTime.core);
  for (auto& column : columns) {
    result.metricLabels.push_back(column.label);
    if (!column.unavailable) {
      cores.push_back(&column.core);
    }
  }

  auto plan =
      sg::counters::detail::compileCore(sys, sg::counters::Target {}, cores);
  if (!plan.has_value()) {
    return fail("the run plan does not compile: " + plan.error().message);
  }

  // FR-023: the availability of every leaf a metric needs is read
  // before the run, and an uncountable metric stays out of the run
  // while the benchmark itself still runs.
  for (auto& column : columns) {
    if (column.unavailable) {
      continue;
    }
    for (const auto& leaf : column.core.leaves) {
      if (leaf.avail != sg::counters::Availability::COUNTABLE) {
        column.unavailable = true;
        column.availability = leaf.avail;
        break;
      }
    }
  }

  // R-02, FR-019: one hardStop recorder for the whole benchmark,
  // sized for the warm-up bound, the calibration bound, and the
  // measured repetitions.
  auto recorder = plan->recorder(
      static_cast<std::size_t>(2 * (kRunBound + kRunBound + repetitions)));
  const double overheadFloorNs = plan->sampleOverheadNsMedian();

  std::uint64_t nextPair = 0;

  // R-06: one flag read after each run ends the benchmark with this
  // reason, and the process exits nonzero.
  constexpr const char* kInterruptReason = "interrupted by the user (SIGINT)";
  auto interrupted = [&](const BenchmarkResult& current) -> BenchmarkResult
  {
    BenchmarkResult stopped = current;
    stopped.outcome = RunOutcome::SKIPPED;
    stopped.reason = kInterruptReason;
    return stopped;
  };

  auto foldAt = [&](const sg::counters::detail::ExprCore& core,
                    const std::uint64_t pair) -> sg::counters::MetricResult
  {
    return sg::counters::detail::foldCore(
        core,
        recorder.view(),
        static_cast<std::size_t>(2 * pair),
        static_cast<std::size_t>(2 * pair + 1));
  };

  auto sampleRun = [&](const std::uint64_t iterations)
      -> std::expected<RunRecord, std::string>
  {
    RunRecord record;
    record.iterations = iterations;
    record.pair = nextPair;

    State state(iterations, recorder);
    try {
      entry.callable(state);
    } catch (const std::exception& error) {
      state.topUpWindow();
      ++nextPair;
      return std::unexpected(error.what());
    } catch (...) {
      state.topUpWindow();
      ++nextPair;
      return std::unexpected("the benchmark threw an unknown exception");
    }
    state.topUpWindow();
    ++nextPair;

    // FR-017: a measured run owns one complete timed loop. A function
    // that never entered it, entered it twice, or left it early without
    // a skip has no single measured window to report.
    if (state.outcome() != RunOutcome::SKIPPED) {
      if (state.loopsStarted() == 0) {
        return std::unexpected(
            "the benchmark never entered the timed loop (FR-017)");
      }
      if (state.loopsStarted() > 1) {
        return std::unexpected(
            "the benchmark entered the timed loop twice (FR-017)");
      }
      if (!state.loopCompleted()) {
        return std::unexpected(
            "the benchmark left the timed loop without a skip (FR-017)");
      }
    }

    record.realNs = static_cast<std::int64_t>(
        std::llround(foldAt(realTime.core, record.pair).value));
    record.decisionNs = static_cast<std::int64_t>(
        std::llround(foldAt(decisionTime.core, record.pair).value));
    if (state.outcome() == RunOutcome::SKIPPED) {
      record.skipped = true;
      record.reason = state.reason();
    }
    return record;
  };

  auto measuredRow = [&](const RunRecord& record) -> ResultRow
  {
    ResultRow row;
    row.kind = RowKind::REPETITION;
    row.iterations = record.iterations;
    row.timePerIterationNs = static_cast<double>(record.realNs)
        / static_cast<double>(record.iterations);
    row.overheadFloorNs = overheadFloorNs;
    for (const auto& column : columns) {
      MetricValue value;
      if (column.unavailable) {
        value.availability = column.availability;
        row.metrics.push_back(value);
        continue;
      }
      const auto folded = foldAt(column.core, record.pair);
      value.value = folded.value;
      value.runningRatio = folded.runningRatio;
      value.scaled = folded.scaled;
      value.availability = folded.availability;
      if (column.perIteration && record.iterations > 0
          && folded.availability == sg::counters::Availability::COUNTABLE)
      {
        value.value /= static_cast<double>(record.iterations);
      }
      row.metrics.push_back(value);
    }
    return row;
  };

  std::uint64_t settled = fixedIterations.value_or(1);

  // FR-011: warm-up runs grow by the same rule against the warm-up
  // target, and their results are discarded. The measured phase keeps
  // its own start count, so the warm-up count never carries into it.
  if (warmupNs > 0 && !dryRun) {
    const auto warm = growUntilQualified(settled,
                                         warmupNs,
                                         kRunBound,
                                         sampleRun,
                                         []() noexcept -> bool
                                         { return interruptFlag().load(); });
    if (warm.outcome == GrowOutcome::FAILED) {
      return fail(warm.error);
    }
    if (warm.outcome == GrowOutcome::SKIPPED) {
      result.outcome = RunOutcome::SKIPPED;
      result.reason = warm.record.reason;
      return result;
    }
    if (warm.outcome == GrowOutcome::INTERRUPTED) {
      return interrupted(result);
    }
    if (warm.outcome == GrowOutcome::BOUND_EXHAUSTED) {
      return fail("warm-up did not qualify within 96 runs (FR-011)");
    }
  }

  // FR-014: a dry run takes one iteration, one repetition, and no
  // warm-up, and it qualifies at once.
  const std::uint64_t repetitionsOfRun = dryRun ? 1 : repetitions;

  for (std::uint64_t repetition = 0; repetition < repetitionsOfRun;
       ++repetition)
  {
    std::uint64_t iterations = dryRun ? 1 : settled;
    std::expected<RunRecord, std::string> chosen = std::unexpected("");

    if (repetition == 0 && !fixedIterations.has_value() && !dryRun) {
      // FR-014: the first repetition calibrates from one iteration, and
      // the run that qualifies is its measured run.
      const auto grown = growUntilQualified(1,
                                            minTimeNs,
                                            kRunBound,
                                            sampleRun,
                                            []() noexcept -> bool
                                            { return interruptFlag().load(); });
      if (grown.outcome == GrowOutcome::FAILED) {
        return fail(grown.error);
      }
      if (grown.outcome == GrowOutcome::SKIPPED) {
        result.outcome = RunOutcome::SKIPPED;
        result.reason = grown.record.reason;
        return result;
      }
      if (grown.outcome == GrowOutcome::INTERRUPTED) {
        return interrupted(result);
      }
      if (grown.outcome == GrowOutcome::BOUND_EXHAUSTED) {
        return fail("calibration did not qualify within 96 runs (FR-016)");
      }
      chosen = std::move(grown.record);
      iterations = chosen->iterations;
      settled = iterations;
    } else {
      chosen = sampleRun(iterations);
      if (!chosen.has_value()) {
        return fail(chosen.error());
      }
      if (chosen->skipped) {
        result.outcome = RunOutcome::SKIPPED;
        result.reason = chosen->reason;
        return result;
      }
    }

    result.rows.push_back(measuredRow(*chosen));

    if (interruptFlag().load()) {
      result.outcome = RunOutcome::SKIPPED;
      result.reason = kInterruptReason;
      break;
    }
  }

  if (result.outcome == RunOutcome::MEASURED && repetitionsOfRun > 1) {
    ResultRow aggregate;
    aggregate.kind = RowKind::AGGREGATE;

    std::vector<double> times;
    for (const auto& row : result.rows) {
      times.push_back(row.timePerIterationNs);
    }
    aggregate.timeStats = statisticsOf(std::move(times));

    for (std::size_t column = 0; column < columns.size(); ++column) {
      std::vector<double> values;
      for (const auto& row : result.rows) {
        // FR-025: a repetition whose run carried a gap stays out of the
        // statistics of the affected quantity.
        if (row.metrics[column].availability
            == sg::counters::Availability::COUNTABLE)
        {
          values.push_back(row.metrics[column].value);
        }
      }
      aggregate.metricStats.push_back(statisticsOf(std::move(values)));
    }

    result.rows.push_back(aggregate);
  }

  return result;
}

}  // namespace sg::detail
