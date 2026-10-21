#include <array>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <utility>

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// One measured benchmark with the single row the comparison reads, whose
// primary time per iteration is `cpuTimeNs` (FR-032).
auto measured(const char* name, const double cpuTimeNs) -> sg::BenchmarkResult
{
  auto result = sg::BenchmarkResult {};
  result.name = name;
  auto row = sg::ResultRow {};
  row.iterations = 1'000;
  row.timePerIterationNs = cpuTimeNs;
  row.cpuTimePerIterationNs = cpuTimeNs;
  result.rows.push_back(row);
  return result;
}

// One benchmark that skipped in both runs, so no row stands for the rule to
// read and the entry answers SKIPPED (FR-011).
auto skipped(const char* name) -> sg::BenchmarkResult
{
  auto result = sg::BenchmarkResult {};
  result.name = name;
  result.outcome = sg::RunOutcome::SKIPPED;
  result.reason = "the fixture refused its argument";
  return result;
}

// The two documents a compare run would read, written here in code so the
// printed lines stand the same on every machine (D-16). The version they
// carry is the one the writer stamps, whose major the comparison pairs
// (specs/026 D-13).
auto baselineReport() -> sg::ReportData
{
  auto report = sg::ReportData {};
  report.schemaVersion = "1.1";
  report.results = {measured("bmGate/8", 100.0),
                    measured("bmGate/16", 200.0),
                    measured("bmGate/32", 400.0),
                    skipped("bmGate/64"),
                    measured("bmGate/128", 800.0),
                    measured("bmGate/256", 1'600.0)};
  return report;
}

auto contenderReport() -> sg::ReportData
{
  auto report = sg::ReportData {};
  report.schemaVersion = "1.1";
  report.results = {measured("bmGate/8", 100.0),
                    measured("bmGate/16", 230.0),
                    skipped("bmGate/64"),
                    measured("bmGate/128", 640.0),
                    measured("bmGate/256", 1'620.0),
                    measured("bmGate/512", 3'200.0)};
  // The contender ran on a busier machine, and its document carries the
  // warning of that run beside the result it names (FR-070).
  report.noiseWarnings = {std::nullopt,
                          std::nullopt,
                          std::nullopt,
                          std::nullopt,
                          sg::NoiseWarning {.spinning = 2, .runnableCpus = 3},
                          std::nullopt};
  return report;
}

auto bmGate(sg::State& state) -> void
{
  const auto cells = state.range(0);
  std::uint64_t sum = 0;
  for (auto _ : state) {
    for (auto cell = std::int64_t {0}; cell < cells; ++cell) {
      sum += static_cast<std::uint64_t>(cell);
    }
    sg::doNotOptimize(sum);
  }
}

SG_BENCHMARK(bmGate).argsProduct({{8, 16}});

// The comparison the two documents answer, printed one line per entry with
// the word of its state, indexed by the enumerator the way the comparison
// table itself is indexed (E-04). The block stands on its own, so a reader
// can paste it into the program the quick start builds.
// sg-example:begin regression-gate
auto printComparison(const sg::ReportData& baseline,
                     const sg::ReportData& contender) -> int
{
  constexpr std::array kStateWords {
      "OK", "REGRESSED", "IMPROVED", "NOISY", "ADDED", "REMOVED", "SKIPPED"};
  const auto rule = sg::parseCompareRule("time@median:lower:5%");
  const auto answer = sg::compareReports(baseline, contender, {*rule}, {});
  if (!answer.has_value()) {
    std::printf("compare: %s\n", answer.error().c_str());
    return 2;
  }
  for (const sg::CompareEntry& entry : answer->entries) {
    const auto word = kStateWords[std::to_underlying(entry.state)];
    std::printf("%-12s %s\n", entry.name.c_str(), word);
  }
  std::printf("compare: the verdict is %s\n", answer->passed ? "pass" : "fail");
  return 0;
}

// sg-example:end regression-gate

auto main(int argc, char** argv) -> int
{
  printComparison(baselineReport(), contenderReport());
  return sg::speedgunMain(argc, argv);
}
