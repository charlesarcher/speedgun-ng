#ifndef SG_TEST_DETAIL_JSON_FIXTURE_HPP
#define SG_TEST_DETAIL_JSON_FIXTURE_HPP

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "detail/internal.hpp"
#include "detail/json_report.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"
#include "speedgun-ng/counters_fake.hpp"
#include "speedgun-ng/hunt.hpp"

// The scripted run and the field-by-field comparison the three report
// tests share (specs/023 T016, T017, T022, T023). The scripted window is
// the one of `harness_percentile_test.cpp`: the calibration prefix the
// plan's sampling-cost read takes, then four chunk deltas whose time,
// cpu and metric columns answer different values, so a column that
// travels the document under another name fails a comparison.
namespace sg::test
{

inline int failures = 0;

auto fail(const char* what) -> void
{
  ++failures;
  std::printf("FAILED: %s\n", what);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// A reported value travels the document as a shortest round-trip decimal,
// so the loaded value answers the run's value; the tolerance is the house
// convention, since -Werror=float-equal rules out `==` on a double.
auto sameValue(const double actual, const double expected) -> bool
{
  return std::abs(actual - expected) <= 1e-9 + 1e-9 * std::abs(expected);
}

constexpr std::uint64_t kCalibrationReads = 321;
constexpr std::uint64_t kCalibrationStep = 1'000;
constexpr std::uint64_t kWindowIterations = 4;

constexpr std::array<std::uint64_t, 4> kTimeDeltas {
    1'000'000, 4'000'000, 2'000'000, 3'000'000};
constexpr std::array<std::uint64_t, 4> kCpuDeltas {
    10'000'000, 40'000'000, 20'000'000, 30'000'000};
constexpr std::array<std::uint64_t, 4> kMetricDeltas {10, 40, 20, 30};

auto windowPoints(const std::array<std::uint64_t, 4>& deltas)
    -> std::vector<std::uint64_t>
{
  std::vector<std::uint64_t> points;
  points.reserve(kCalibrationReads + kWindowIterations + 1);
  for (std::uint64_t read = 0; read < kCalibrationReads; ++read) {
    points.push_back(read * kCalibrationStep);
  }
  std::uint64_t running = (kCalibrationReads - 1) * kCalibrationStep;
  points.push_back(running);
  for (const std::uint64_t delta : deltas) {
    running += delta;
    points.push_back(running);
  }
  return points;
}

auto bmJsonWindow(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

// The scripted run of one option set: the result the run handed back,
// beside the chunk columns it retained for that result's rows.
struct ScriptedRun
{
  sg::BenchmarkResult result;
  std::vector<std::vector<sg::RowRaw>> rowRaw;
  std::vector<std::vector<std::vector<sg::RowRaw>>> contextRaw;
};

auto scriptedRun(const sg::detail::RunOptions& options) -> ScriptedRun
{
  static const bool kScripted = []
  {
    auto owned = std::make_unique<sg::counters::FakeProvider>();
    auto& provider = *owned;
    provider.addObject("scripted", "scripted", "scripted metric object");
    provider.addCounter("scripted", "work", "none", "scripted work");
    provider.setPoints("scripted", "work", windowPoints(kMetricDeltas));
    provider.addCounter("machine", "monotonic", "nanoseconds", "monotonic");
    provider.addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
    provider.setPoints("machine", "monotonic", windowPoints(kTimeDeltas));
    provider.setPoints("machine", "thread_cpu", windowPoints(kCpuDeltas));
    static_cast<void>(
        sg::counters::System::local().registerProvider(std::move(owned)));
    return true;
  }();
  static_cast<void>(kScripted);
  static const sg::BenchmarkHandle kHandle = []
  {
    auto registered = sg::registerBenchmark(&bmJsonWindow, "bmJsonWindow");
    const auto object = sg::counters::System::local().object("scripted");
    const auto work = object->counter<sg::counters::Dim<0, 1>>("work");
    registered.addMetric(
        sg::counters::Expression<sg::counters::Dim<0, 1>>(*work), "work");
    return registered;
  }();
  static_cast<void>(kHandle);
  static const bool kExpanded = []
  {
    sg::detail::expandRegistry();
    return true;
  }();
  static_cast<void>(kExpanded);

  sg::detail::Runner runner(options);
  ScriptedRun run;
  for (auto& instance : sg::detail::instances()) {
    if (instance.name != "bmJsonWindow") {
      continue;
    }
    run.result = runner.run(instance);
    run.rowRaw.push_back(runner.rowRaw().back());
    run.contextRaw.push_back(runner.contextRaw().back());
  }
  return run;
}

// The whole file as one string, or the empty string when the file stands
// unopenable; the tests read back what the writer wrote.
auto readText(const std::string& path) -> std::string
{
  std::ifstream stream(path, std::ios::binary);
  std::string text;
  char chunk[1024];
  do {
    stream.read(chunk, std::streamsize(sizeof chunk));
    text.append(chunk, std::size_t(stream.gcount()));
  } while (stream.gcount() == std::streamsize(sizeof chunk));
  return text;
}

auto writeText(const std::string& path, const std::string& text) -> void
{
  std::ofstream stream(path, std::ios::binary);
  stream << text;
}

auto sameStatistics(const sg::Statistics& actual,
                    const sg::Statistics& want) -> bool
{
  return actual.samples == want.samples && sameValue(actual.mean, want.mean)
      && sameValue(actual.median, want.median)
      && sameValue(actual.sampleStdDev, want.sampleStdDev)
      && sameValue(actual.coefficientOfVariation, want.coefficientOfVariation)
      && sameValue(actual.min, want.min) && sameValue(actual.max, want.max);
}

auto sameCell(const sg::PercentileValue& actual,
              const sg::PercentileValue& want) -> bool
{
  return actual.available == want.available
      && (actual.available ? sameValue(actual.value, want.value) : true);
}

auto sameColumns(const std::vector<std::vector<sg::PercentileValue>>& actual,
                 const std::vector<std::vector<sg::PercentileValue>>& want)
    -> bool
{
  if (actual.size() != want.size()) {
    return false;
  }
  for (std::size_t column = 0; column < actual.size(); ++column) {
    if (actual[column].size() != want[column].size()) {
      return false;
    }
    for (std::size_t point = 0; point < actual[column].size(); ++point) {
      if (!sameCell(actual[column][point], want[column][point])) {
        return false;
      }
    }
  }
  return true;
}

auto sameMetric(const sg::MetricValue& actual,
                const sg::MetricValue& want) -> bool
{
  return sameValue(actual.value, want.value)
      && sameValue(actual.runningRatio, want.runningRatio)
      && actual.scaled == want.scaled
      && actual.availability == want.availability && actual.pass == want.pass;
}

// The spread of the passes, empty where fewer than two ran (specs/024
// FR-017).
auto sameSpread(const std::optional<double>& actual,
                const std::optional<double>& want) -> bool
{
  return actual.has_value() == want.has_value()
      && (!actual.has_value() || sameValue(*actual, *want));
}

auto sameFit(const sg::FitRow& actual, const sg::FitRow& want) -> bool
{
  return actual.quantity == want.quantity && actual.curve == want.curve
      && sameValue(actual.coefficient, want.coefficient)
      && sameValue(actual.rmsPercent, want.rmsPercent)
      && actual.scalesWithUnit == want.scalesWithUnit
      && actual.reason == want.reason;
}

auto sameRow(const sg::ResultRow& actual, const sg::ResultRow& want) -> bool
{
  return actual.kind == want.kind && actual.label == want.label
      && actual.iterations == want.iterations
      && sameValue(actual.timePerIterationNs, want.timePerIterationNs)
      && sameValue(actual.cpuTimePerIterationNs, want.cpuTimePerIterationNs)
      && actual.endpointSamples == want.endpointSamples
      && actual.pauseSamplesFolded == want.pauseSamplesFolded
      && actual.timeAvailability == want.timeAvailability
      && actual.metrics.size() == want.metrics.size()
      && actual.counters.size() == want.counters.size()
      && actual.counterDisclosure == want.counterDisclosure
      && sameFit(actual.fit, want.fit)
      && sameValue(actual.overheadFloorNs, want.overheadFloorNs)
      && sameColumns(actual.percentiles, want.percentiles)
      && actual.chunkCount == want.chunkCount
      && actual.chunkIterations == want.chunkIterations
      && sameStatistics(actual.timeStats, want.timeStats)
      && actual.metricStats.size() == want.metricStats.size()
      && (actual.metrics.size() != want.metrics.size()
          || std::equal(actual.metrics.begin(),
                        actual.metrics.end(),
                        want.metrics.begin(),
                        sameMetric))
      && (actual.counters.size() != want.counters.size()
          || std::equal(actual.counters.begin(),
                        actual.counters.end(),
                        want.counters.begin(),
                        sameValue))
      && (actual.metricStats.size() != want.metricStats.size()
          || std::equal(actual.metricStats.begin(),
                        actual.metricStats.end(),
                        want.metricStats.begin(),
                        sameStatistics));
}

auto sameRowRaw(const std::vector<sg::RowRaw>& actual,
                const std::vector<sg::RowRaw>& want) -> bool
{
  if (actual.size() != want.size()) {
    return false;
  }
  for (std::size_t row = 0; row < actual.size(); ++row) {
    if (actual[row].columns.size() != want[row].columns.size()) {
      return false;
    }
    for (std::size_t column = 0; column < actual[row].columns.size(); ++column)
    {
      if (actual[row].columns[column].size()
          != want[row].columns[column].size())
      {
        return false;
      }
      for (std::size_t chunk = 0; chunk < actual[row].columns[column].size();
           ++chunk)
      {
        if (!sameValue(actual[row].columns[column][chunk],
                       want[row].columns[column][chunk]))
        {
          return false;
        }
      }
    }
  }
  return true;
}

auto sameRowRaw(const std::vector<std::vector<sg::RowRaw>>& actual,
                const std::vector<std::vector<sg::RowRaw>>& want) -> bool
{
  if (actual.size() != want.size()) {
    return false;
  }
  for (std::size_t row = 0; row < actual.size(); ++row) {
    if (!sameRowRaw(actual[row], want[row])) {
      return false;
    }
  }
  return true;
}

// The hunt records of specs/029 (data-model.md E-06, E-08; FR-051) travel the
// document as the six keys of the closed entry node and the sixteen of the
// closed run node, so both members compare field by field beside the rest of
// the result. `RankedEvent::samples` and `RankedEvent::repeated` stand outside
// the comparison: D-17 keeps the sample count and the repeated mark out of the
// document, so the reader leaves both at the default an unmarked rank carries
// and no round trip restores them.
auto sameRankedEvent(const sg::hunt::RankedEvent& actual,
                     const sg::hunt::RankedEvent& want) -> bool
{
  return actual.address == want.address && sameValue(actual.rate, want.rate)
      && sameValue(actual.cv, want.cv) && actual.noisy == want.noisy
      && actual.multiplexed == want.multiplexed && actual.rank == want.rank;
}

auto sameSkippedEvent(const sg::hunt::SkippedEvent& actual,
                      const sg::hunt::SkippedEvent& want) -> bool
{
  return actual.address == want.address && actual.kind == want.kind;
}

auto sameEntryHunt(const sg::hunt::EntryHunt& actual,
                   const sg::hunt::EntryHunt& want) -> bool
{
  if (actual.method != want.method || actual.passCount != want.passCount
      || actual.top != want.top || actual.retriedPasses != want.retriedPasses
      || actual.entries.size() != want.entries.size()
      || actual.skipped.size() != want.skipped.size())
  {
    return false;
  }
  return std::equal(actual.entries.begin(),
                    actual.entries.end(),
                    want.entries.begin(),
                    sameRankedEvent)
      && std::equal(actual.skipped.begin(),
                    actual.skipped.end(),
                    want.skipped.begin(),
                    sameSkippedEvent);
}

// The per-entry hunt of one result. Two empty members agree: that is the
// reading of a run that carried no hunt (PD-10).
auto sameEntryHunt(const std::optional<sg::hunt::EntryHunt>& actual,
                   const std::optional<sg::hunt::EntryHunt>& want) -> bool
{
  return actual.has_value() == want.has_value()
      && (!actual.has_value() || sameEntryHunt(*actual, *want));
}

auto sameStageRange(const sg::hunt::HuntStageRange& actual,
                    const sg::hunt::HuntStageRange& want) -> bool
{
  return actual.first == want.first && actual.count == want.count
      && actual.repeats == want.repeats;
}

auto sameBudget(const std::optional<sg::hunt::HuntBudget>& actual,
                const std::optional<sg::hunt::HuntBudget>& want) -> bool
{
  return actual.has_value() == want.has_value()
      && (!actual.has_value()
          || (actual->kind == want->kind && actual->amount == want->amount));
}

// The run-level hunt beside the benchmark array: the settled options, the
// shape of the plan the run ran, and nothing else (PD-10).
// `HuntOptions::selection` stands outside the comparison: the closed node
// carries one selection text and the reader hangs it on `RunHunt::selection`,
// so the CLI text of the options is empty in anything that came back from a
// document.
auto sameRunHunt(const std::optional<sg::hunt::RunHunt>& actual,
                 const std::optional<sg::hunt::RunHunt>& want) -> bool
{
  if (actual.has_value() != want.has_value()) {
    return false;
  }
  if (!actual.has_value()) {
    return true;
  }
  const auto& got = actual->options;
  const auto& asked = want->options;
  const bool options = got.norm == asked.norm && got.rank == asked.rank
      && got.top == asked.top && sameValue(got.noise, asked.noise)
      && got.repeats == asked.repeats && got.seed == asked.seed
      && sameBudget(got.budget, asked.budget);
  return options && actual->selection == want->selection
      && actual->passes == want->passes && actual->method == want->method
      && actual->estimateNs == want->estimateNs
      && actual->plannedAddresses == want->plannedAddresses
      && actual->ranEvents == want->ranEvents
      && actual->stageTwoEvents == want->stageTwoEvents
      && actual->stages.size() == want->stages.size()
      && std::equal(actual->stages.begin(),
                    actual->stages.end(),
                    want->stages.begin(),
                    sameStageRange);
}

// One result against the run that wrote it, field by field (FR-022,
// SC-010).
auto sameResult(const sg::BenchmarkResult& actual,
                const sg::BenchmarkResult& want) -> bool
{
  if (actual.name != want.name || actual.suite != want.suite
      || actual.caseName != want.caseName || actual.outcome != want.outcome
      || actual.reason != want.reason
      || actual.hideRepetitionRows != want.hideRepetitionRows
      || actual.timeUnit != want.timeUnit
      || actual.primaryIsCpu != want.primaryIsCpu
      || actual.pauseSamplesFolded != want.pauseSamplesFolded
      || actual.metricMethod != want.metricMethod
      || actual.passCount != want.passCount
      || !sameSpread(actual.passSpread, want.passSpread)
      || actual.percentilePoints.size() != want.percentilePoints.size()
      || actual.metricLabels != want.metricLabels
      || actual.counterLabels != want.counterLabels
      || actual.rows.size() != want.rows.size()
      || actual.counterStats.size() != want.counterStats.size()
      || actual.statisticValues.size() != want.statisticValues.size()
      || actual.contexts.size() != want.contexts.size()
      || actual.contextPercentiles.size() != want.contextPercentiles.size()
      || !sameEntryHunt(actual.hunt, want.hunt))
  {
    return false;
  }
  for (std::size_t point = 0; point < actual.percentilePoints.size(); ++point) {
    if (!sameValue(actual.percentilePoints[point],
                   want.percentilePoints[point]))
    {
      return false;
    }
  }
  for (std::size_t row = 0; row < actual.rows.size(); ++row) {
    if (!sameRow(actual.rows[row], want.rows[row])) {
      return false;
    }
  }
  for (std::size_t stat = 0; stat < actual.counterStats.size(); ++stat) {
    if (!sameStatistics(actual.counterStats[stat], want.counterStats[stat])) {
      return false;
    }
  }
  for (std::size_t value = 0; value < actual.statisticValues.size(); ++value) {
    const auto& got = actual.statisticValues[value];
    const auto& asked = want.statisticValues[value];
    if (got.name != asked.name || got.unit != asked.unit
        || !sameValue(got.value, asked.value))
    {
      return false;
    }
  }
  for (std::size_t context = 0; context < actual.contexts.size(); ++context) {
    const auto& got = actual.contexts[context];
    const auto& asked = want.contexts[context];
    if (got.waitMode != asked.waitMode || got.placement != asked.placement
        || got.applied != asked.applied)
    {
      return false;
    }
  }
  for (std::size_t block = 0; block < actual.contextPercentiles.size(); ++block)
  {
    if (!sameColumns(actual.contextPercentiles[block],
                     want.contextPercentiles[block]))
    {
      return false;
    }
  }
  return true;
}

// One pre-check record against the record the run built, field by
// field (specs/034 FR-024, SC-004). The figures carry the absent
// optionals beside the counts, and the softirq kinds carry their own
// order.
auto samePrecheckRecord(const sg::PrecheckRecord& got,
                        const sg::PrecheckRecord& asked) -> bool
{
  const auto sameAttribution = [](const sg::PrecheckAttribution& actual,
                                  const sg::PrecheckAttribution& want)
  {
    return actual.interrupts == want.interrupts
        && actual.switches == want.switches
        && actual.schedules == want.schedules
        && actual.softirqs.size() == want.softirqs.size()
        && std::equal(actual.softirqs.begin(),
                      actual.softirqs.end(),
                      want.softirqs.begin(),
                      [](const auto& left, const auto& right) {
                        return left.first == right.first
                            && left.second == right.second;
                      });
  };
  const auto sameCpu = [&sameAttribution](const sg::PrecheckCpuRecord& actual,
                                          const sg::PrecheckCpuRecord& want)
  {
    return actual.cpu == want.cpu && actual.planned == want.planned
        && actual.stats.gapCount == want.stats.gapCount
        && sameValue(actual.stats.gapsPerSecond, want.stats.gapsPerSecond)
        && actual.stats.minNs == want.stats.minNs
        && actual.stats.medianNs == want.stats.medianNs
        && sameValue(actual.stats.meanNs, want.stats.meanNs)
        && actual.stats.p90Ns == want.stats.p90Ns
        && actual.stats.p99Ns == want.stats.p99Ns
        && actual.stats.p999Ns == want.stats.p999Ns
        && actual.stats.p9999Ns == want.stats.p9999Ns
        && actual.stats.maxNs == want.stats.maxNs
        && actual.stats.totalNs == want.stats.totalNs
        && sameValue(actual.stats.totalPercent, want.stats.totalPercent)
        && actual.stats.runtimeNs == want.stats.runtimeNs
        && actual.stats.thresholdNs == want.stats.thresholdNs
        && actual.stats.loopPeriodNs == want.stats.loopPeriodNs
        && actual.stats.droppedGaps == want.stats.droppedGaps
        && sameAttribution(actual.attribution, want.attribution);
  };
  const auto sameHost = [](const sg::PrecheckHostSettings& actual,
                           const sg::PrecheckHostSettings& want)
  {
    return actual.isolcpus == want.isolcpus && actual.nohzFull == want.nohzFull
        && actual.rcuNocbs == want.rcuNocbs
        && actual.governors.size() == want.governors.size()
        && std::equal(actual.governors.begin(),
                      actual.governors.end(),
                      want.governors.begin())
        && actual.irqAffinity.size() == want.irqAffinity.size()
        && std::equal(actual.irqAffinity.begin(),
                      actual.irqAffinity.end(),
                      want.irqAffinity.begin());
  };
  return got.serial == asked.serial && got.windowMs == asked.windowMs
      && got.cpus.size() == asked.cpus.size()
      && std::equal(
             got.cpus.begin(), got.cpus.end(), asked.cpus.begin(), sameCpu)
      && sameHost(got.host, asked.host)
      && got.skippedGroups == asked.skippedGroups;
}

auto sameReport(const sg::ReportData& actual,
                const sg::ReportData& want) -> bool
{
  const bool precheck = actual.precheck.has_value() == want.precheck.has_value()
      && (!actual.precheck.has_value()
          || samePrecheckRecord(*actual.precheck, *want.precheck));
  if (actual.results.size() != want.results.size()
      || actual.rowRaw.size() != want.rowRaw.size()
      || actual.contextRaw.size() != want.contextRaw.size()
      || !sameRunHunt(actual.hunt, want.hunt) || !precheck)
  {
    return false;
  }
  for (std::size_t entry = 0; entry < actual.results.size(); ++entry) {
    if (!sameResult(actual.results[entry], want.results[entry])
        || !sameRowRaw(actual.rowRaw[entry], want.rowRaw[entry])
        || !sameRowRaw(actual.contextRaw[entry], want.contextRaw[entry]))
    {
      return false;
    }
  }
  return true;
}

}  // namespace sg::test

#endif  // SG_TEST_DETAIL_JSON_FIXTURE_HPP
