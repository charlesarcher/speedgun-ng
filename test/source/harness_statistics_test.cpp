// ============================================================================
// TDD test for the repetition aggregates (T032; US5, FR-025, FR-027,
// FR-028, FR-035, SC-003).
//
// The five repetition rows of the report are the fixture: the aggregates are
// recomputed here from the printed rows with the sample form of R-05, the
// n-1 denominator, and compared to the aggregate row. The gap case scripts
// one run whose window measures nothing, and the aggregate of the affected
// quantity must drop that sample (FR-025). Frameworkless check()/fail()
// convention.
// ============================================================================

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"

namespace
{

sg::counters::FakeProvider* fakeLeaf = nullptr;

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS STATISTICS TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

auto bmWork(sg::State& state) -> void
{
  double accumulator = 1.0;
  for (auto _ : state) {
    for (int index = 0; index < 50; ++index) {
      accumulator += accumulator * 1.000000001;
    }
    if (accumulator < 0.0) {
      std::exit(1);
    }
  }
}

auto captureRun(const std::vector<std::string>& arguments) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_statistics_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  const char* const path = "harness_statistics_out.txt";
  const int saved = dup(STDOUT_FILENO);
  if (saved < 0 || std::freopen(path, "w", stdout) == nullptr) {
    fail("cannot redirect the report");
  }
  const int status =
      sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
  std::fflush(stdout);
  if (dup2(saved, STDOUT_FILENO) < 0) {
    fail("cannot restore stdout");
  }
  close(saved);
  if (status != 0) {
    std::fprintf(stderr, "speedgunMain returned %d\n", status);
    fail("the run did not exit zero");
  }

  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

auto linesOf(const std::string& report,
             const std::string& name) -> std::vector<std::string>
{
  std::vector<std::string> found;
  std::istringstream stream(report);
  std::string line;
  while (std::getline(stream, line)) {
    if (line.rfind(name, 0) == 0) {
      found.push_back(line);
    }
  }
  return found;
}

auto doubleFieldOf(const std::string& line, const std::string& key) -> double
{
  const std::size_t at = line.find(key);
  if (at == std::string::npos) {
    std::fprintf(stderr, "no field '%s' in '%s'\n", key.c_str(), line.c_str());
    fail("the report carries no such field");
  }
  return std::strtod(line.c_str() + at + key.size(), nullptr);
}

auto countFieldOf(const std::string& line, const std::string& key) -> long long
{
  const std::size_t at = line.find(key);
  if (at == std::string::npos) {
    std::fprintf(stderr, "no field '%s' in '%s'\n", key.c_str(), line.c_str());
    fail("the report carries no such field");
  }
  return std::strtoll(line.c_str() + at + key.size(), nullptr, 10);
}

auto closeTo(const double actual, const double expected) -> bool
{
  return std::abs(actual - expected) <= 1e-3 + 1e-9 * std::abs(expected);
}

auto meanOf(const std::vector<double>& values) -> double
{
  double total = 0.0;
  for (const double value : values) {
    total += value;
  }
  return total / static_cast<double>(values.size());
}

auto medianOf(std::vector<double> values) -> double
{
  std::sort(values.begin(), values.end());
  const std::size_t middle = values.size() / 2;
  if (values.size() % 2 != 0) {
    return values[middle];
  }
  return (values[middle - 1] + values[middle]) / 2.0;
}

auto sampleStdDevOf(const std::vector<double>& values,
                    const double mean) -> double
{
  double sumOfSquares = 0.0;
  for (const double value : values) {
    sumOfSquares += (value - mean) * (value - mean);
  }
  return std::sqrt(sumOfSquares / static_cast<double>(values.size() - 1));
}

auto scriptedProvider() -> void
{
  auto provider = std::make_unique<sg::counters::FakeProvider>();
  auto* const raw = provider.get();
  provider->addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  provider->setPoints("machine", "monotonic", {0}, 0, 7);
  provider->addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  provider->setPoints("machine", "thread_cpu", {0}, 2000);
  provider->addObject("gapobj", "gapobj", "the gapped object");
  provider->addCounter("gapobj", "gnum", "none", "a leaf with a scripted gap");
  provider->setPoints("gapobj", "gnum", {0}, 600);
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(provider));
  if (!registered.has_value()) {
    std::fprintf(stderr, "register: %s\n", registered.error().message.c_str());
    fail("the fake provider registers");
  }
  fakeLeaf = raw;
}

}  // namespace

SG_BENCHMARK(bmWork);

auto main() -> int
{
  // The scripted leaf must register before the system opens (FR-009), so
  // it joins the process before the first run.
  scriptedProvider();

  // FR-035, SC-003: five repetitions print five rows plus the aggregate,
  // and every aggregate equals the sample-form value of the rows.
  const std::string report = captureRun(
      {"--filter", "^bmWork$", "--iterations=20", "--repetitions=5"});
  const std::vector<std::string> rows = linesOf(report, "bmWork");
  check(rows.size() == 6,
        "five repetitions print five rows and one aggregate row (FR-035)");

  std::vector<double> times;
  for (std::size_t row = 0; row + 1 < rows.size(); ++row) {
    times.push_back(doubleFieldOf(rows[row], "time/iter (ns)="));
  }
  const std::string& aggregate = rows.back();
  check(aggregate.find("AGGREGATE") != std::string::npos,
        "the last row is the aggregate (FR-035)");

  const double mean = meanOf(times);
  const double stdDev = sampleStdDevOf(times, mean);
  std::vector<double> sorted = times;
  std::sort(sorted.begin(), sorted.end());

  check(closeTo(doubleFieldOf(aggregate, "mean="), mean),
        "the aggregate mean equals the rows (FR-027)");
  check(closeTo(doubleFieldOf(aggregate, "median="), medianOf(times)),
        "the aggregate median equals the rows (FR-027)");
  check(closeTo(doubleFieldOf(aggregate, "sd="), stdDev),
        "the aggregate uses the sample form with the n-1 denominator (R-05)");
  check(closeTo(doubleFieldOf(aggregate, "cv="), stdDev / mean),
        "the coefficient of variation is the standard deviation over the mean "
        "(R-05)");
  check(closeTo(doubleFieldOf(aggregate, "min="), sorted.front())
            && closeTo(doubleFieldOf(aggregate, "max="), sorted.back()),
        "the aggregate min and max equal the rows (FR-027)");
  check(countFieldOf(aggregate, " n=") == 5,
        "the aggregate counts the five measured repetitions (FR-027)");
  check(doubleFieldOf(aggregate, "sd=") > 0.0,
        "the fixture rows vary, so the sample form is exercised (SC-003)");
  check(aggregate.find("p50=") == std::string::npos
            && aggregate.find("percentile") == std::string::npos,
        "the percentile form stays out (FR-028)");

  // FR-025: a repetition whose run carries a gap contributes no sample to
  // the aggregate of the affected quantity, and the time aggregate keeps
  // all five. The gap script matches the window's own action count, and one
  // window holds the plan's calibration points plus ten measured points, so
  // the batch above reveals the calibration length and the next window's
  // second run ends at calibration plus four.
  const std::uint64_t consumed = fakeLeaf->readActions();
  fakeLeaf->setGapActions("gapobj", "gnum", {consumed - 6});
  const std::string gapped = captureRun({"--filter",
                                         "^bmWork$",
                                         "--iterations=2",
                                         "--repetitions=5",
                                         "--counter",
                                         "gapobj/gnum"});
  const std::vector<std::string> gappedRows = linesOf(gapped, "bmWork");
  check(gappedRows.size() == 7,
        "the aggregate rows carry the metric aggregate too (FR-035)");
  const std::string& timeAggregate = gappedRows[5];
  const std::string& metricAggregate = gappedRows[6];
  check(countFieldOf(timeAggregate, " n=") == 5,
        "the time aggregate keeps every measured repetition (FR-027)");
  check(metricAggregate.find("gapobj/gnum") != std::string::npos
            && countFieldOf(metricAggregate, " n=") == 4,
        "the gapped repetition contributes no sample (FR-025)");
  std::size_t gapRows = 0;
  for (const auto& row : gappedRows) {
    if (row.find("gap=1") != std::string::npos) {
      ++gapRows;
    }
  }
  check(gapRows == 1, "one repetition reports the gap (FR-025)");

  std::puts("harness_statistics_test: ok");
  return 0;
}
