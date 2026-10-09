// ============================================================================
// TDD test for the harness capture path (T012; US1).
//
// A scripted FakeProvider owns the machine time leaves, so every number the
// harness prints is a scripted delta: the monotonic leaf steps 4000 ns per
// sampling action and the thread_cpu leaf steps 2000 ns. The assertions read
// the captured console report, which is the surface the story is told on.
// Hand-computed expectations, frameworkless check()/fail() convention.
//
// Covers the per-iteration division of FR-022, the undivided dimensionless
// window of the same requirement, the disclosure columns of FR-024, the gap
// state of FR-025, the overhead floor of FR-026 and SC-014, the
// two-sampling-actions-per-run count of SC-005, and the no-allocation rule of
// FR-017 inside the timed loop.
// ============================================================================

#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <new>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"

std::uint64_t allocationCount = 0;
bool countAllocations = false;

auto operator new(std::size_t size) -> void*
{
  if (countAllocations) {
    ++allocationCount;
  }
  void* const block = std::malloc(size);
  if (block == nullptr) {
    throw std::bad_alloc();
  }
  return block;
}

// GCC's warning pass attributes every allocation in a translation unit
// to the default `operator new` and never consults the replacement
// declared above, so it reports `-Wmismatched-new-delete` at each `free`
// below. The pairing is correct: these allocations hand out `malloc`
// memory and the deletes return it with `free`. The suppression covers
// exactly that false positive, the way test/source/counters_noalloc_test.cpp
// does (constitution I, X.2).
#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic ignored "-Wmismatched-new-delete"
#endif

auto operator delete(void* block) noexcept -> void
{
  std::free(block);
}

auto operator delete(void* block, std::size_t) noexcept -> void
{
  std::free(block);
}

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS CAPTURE TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

auto sameDouble(const double lhs, const double rhs) -> bool
{
  return std::bit_cast<std::uint64_t>(lhs) == std::bit_cast<std::uint64_t>(rhs);
}

constexpr std::uint64_t kMonotonicStep = 4000;
constexpr std::uint64_t kNumeratorStep = 300;
constexpr std::uint64_t kDenominatorStep = 150;
constexpr std::uint64_t kTscStep = 700;

sg::counters::FakeProvider* fake = nullptr;

auto bmScripted(sg::State& state) -> void
{
  const std::uint64_t baseline = allocationCount;
  for (auto _ : state) {
    if (allocationCount != baseline) {
      fail("the timed loop allocated (FR-017)");
    }
  }
}

auto bmGapped(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmCounted(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto captureRun(const std::vector<std::string>& arguments) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_capture_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  const char* const path = "harness_capture_out.txt";
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

auto checkHas(const std::string& line,
              const std::string& needle,
              const char* what) -> void
{
  if (line.find(needle) == std::string::npos) {
    std::fprintf(stderr, "no '%s' in '%s'\n", needle.c_str(), line.c_str());
    fail(what);
  }
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

auto lineOf(const std::string& report, const std::string& name) -> std::string
{
  std::istringstream stream(report);
  std::string line;
  while (std::getline(stream, line)) {
    if (line.rfind(name, 0) == 0) {
      return line;
    }
  }
  return {};
}

auto fieldOf(const std::string& line, const std::string& key) -> double
{
  const std::size_t at = line.find(key);
  if (at == std::string::npos) {
    std::fprintf(stderr, "no field '%s' in '%s'\n", key.c_str(), line.c_str());
    fail("the report carries no such field");
  }
  return std::strtod(line.c_str() + at + key.size(), nullptr);
}

auto scriptedProvider() -> void
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  auto& provider = *owned;
  fake = &provider;

  provider.addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  provider.addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  provider.setPoints("machine", "monotonic", {0}, kMonotonicStep);
  provider.setPoints("machine", "thread_cpu", {0}, 2000);
  // FR-039: the tsc leaf is a tick count that publishes no rate.
  provider.addCounter(
      "machine",
      "tsc",
      "none",
      "raw time-stamp counter ticks; a count asserting no " "rate",
      sg::counters::Availability::COUNTABLE,
      sg::counters::ReadMode::FAST_TSC);
  provider.setPoints("machine", "tsc", {0}, kTscStep);

  provider.addObject("fake", "fake", "scripted metric object");
  provider.addCounter("fake",
                      "enabled",
                      "nanoseconds",
                      "enabled time",
                      sg::counters::Availability::COUNTABLE,
                      sg::counters::ReadMode::SYSCALL,
                      true);
  provider.addCounter("fake",
                      "running",
                      "nanoseconds",
                      "running time",
                      sg::counters::Availability::COUNTABLE,
                      sg::counters::ReadMode::SYSCALL,
                      true);
  provider.setPoints("fake", "enabled", {0}, 200);
  provider.setPoints("fake", "running", {0}, 100);
  provider.addCounter("fake", "num", "none", "numerator");
  provider.addCounter("fake", "den", "none", "denominator");
  provider.setPoints("fake", "num", {0}, kNumeratorStep);
  provider.setPoints("fake", "den", {0}, kDenominatorStep);

  provider.addObject("gapobj", "gapobj", "scripted gap object");
  provider.addCounter("gapobj", "gnum", "none", "gapped numerator");
  provider.addCounter("gapobj", "gden", "none", "gap-free denominator");
  provider.setPoints("gapobj", "gnum", {0}, kNumeratorStep);
  provider.setPoints("gapobj", "gden", {0}, kDenominatorStep);
  std::vector<std::size_t> actions;
  // The plan's own sampling-cost calibration runs 64 warm-up plus 257
  // measured actions before the benchmark's first endpoint, so the
  // scripted gap range has to reach past it.
  for (std::size_t action = 1; action <= 1024; ++action) {
    actions.push_back(action);
  }
  provider.setGapActions("gapobj", "gnum", std::move(actions));

  const auto registered =
      sg::counters::System::local().registerProvider(std::move(owned));
  check(registered.has_value(), "the fake provider registers");
}

}  // namespace

SG_BENCHMARK(bmScripted)
SG_BENCHMARK(bmGapped)
SG_BENCHMARK(bmCounted)

auto main() -> int
{
  scriptedProvider();

  auto ratioHandle = sg::registerBenchmark(&bmScripted, "bmRatio");
  check(ratioHandle.name() == "bmRatio", "the handle names its entry");
  {
    auto& system = sg::counters::System::local();
    const auto object = system.object("fake");
    check(object.has_value(), "the scripted object resolves");
    const auto enabled = object->counter<sg::counters::Dim<1, 0>>("enabled");
    const auto running = object->counter<sg::counters::Dim<1, 0>>("running");
    const auto numerator = object->counter<sg::counters::Dim<0, 1>>("num");
    const auto denominator = object->counter<sg::counters::Dim<0, 1>>("den");
    check(enabled.has_value() && running.has_value() && numerator.has_value()
              && denominator.has_value(),
          "the scripted leaves resolve");
    // A pair-carrying leaf quoted alone keeps its own running ratio, and
    // the pairless quotient keeps the dimensionless form of FR-022. The
    // pair's two halves both enter the plan, which is what lets the fold
    // see the elapsed enabled time.
    ratioHandle.addMetric(
        sg::counters::Expression<sg::counters::Dim<1, 0>>(*enabled), "enabled");
    ratioHandle.addMetric(
        sg::counters::Expression<sg::counters::Dim<1, 0>>(*running), "running");
    ratioHandle.addMetric(*numerator / *denominator, "num/den");
  }

  auto gapHandle = sg::registerBenchmark(&bmGapped, "bmGap");
  {
    auto& system = sg::counters::System::local();
    const auto object = system.object("gapobj");
    const auto numerator = object->counter<sg::counters::Dim<0, 1>>("gnum");
    const auto denominator = object->counter<sg::counters::Dim<0, 1>>("gden");
    check(numerator.has_value() && denominator.has_value(),
          "the gapped leaves resolve");
    gapHandle.addMetric(*numerator / *denominator, "gnum/gden");
  }

  countAllocations = true;
  const std::string single =
      captureRun({"--filter", "^bmRatio$", "--iterations=8"});
  countAllocations = false;

  const std::string scriptedLine = lineOf(single, "bmRatio");
  check(!scriptedLine.empty(), "the report carries the scripted row");
  // The plan calibrates the sampling action against the library's own
  // reference clock (source/counters/plan.cpp), so the floor is a
  // wall-clock figure. It is not a scripted delta. What the harness
  // owes is one plan-level floor, published on every row of the plan.
  check(fieldOf(scriptedLine, "overhead floor (ns)=") > 0.0,
        "the row carries the plan's overhead floor (FR-026)");
  check(sameDouble(fieldOf(scriptedLine, "time/iter (ns)="),
                   static_cast<double>(kMonotonicStep) / 8.0),
        "time per iteration is the scripted delta over N (SC-001)");
  // FR-022: a dimensionless window keeps its value undivided; scripted
  // deltas 300 and 150 fold to 2.0 at any iteration count, and a pairless
  // source carries no ratio.
  check(sameDouble(fieldOf(scriptedLine, "num/den="), 2.0),
        "the dimensionless window stays undivided (FR-022)");
  const std::string quotient =
      scriptedLine.substr(scriptedLine.find("num/den="));
  checkHas(quotient, "ratio=1.000", "a pairless source reports ratio 1");
  checkHas(quotient, "scaled=0", "a pairless source clears the scaled flag");
  // FR-024: the pair-carrying leaf reports its running ratio and the
  // scaled flag beside the per-iteration value (SC-004).
  check(sameDouble(fieldOf(scriptedLine, "enabled="), 200.0 / 8.0),
        "the time metric divides by the iteration count (FR-022)");
  const std::string pair = scriptedLine.substr(scriptedLine.find("enabled="));
  checkHas(pair,
           "ratio=0.500",
           "the running ratio stands beside the value (SC-004)");
  checkHas(pair,
           "scaled=1",
           "the pair-carrying source sets the scaled flag (FR-024)");
  checkHas(scriptedLine, "gap=0", "a gap-free window reports no gap (FR-024)");

  const std::string gapLine =
      lineOf(captureRun({"--filter", "^bmGap$", "--iterations=4"}), "bmGap");
  checkHas(gapLine,
           "gap=1",
           "a window whose endpoint measured nothing reports the gap (FR-025)");

  // FR-039: the scripted tsc leaf reports as a count, and the reported
  // time still comes from machine/monotonic alone.
  const std::string tscLine = lineOf(captureRun({"--filter",
                                                 "^bmCounted$",
                                                 "--iterations=4",
                                                 "--counter",
                                                 "machine/tsc"}),
                                     "bmCounted");
  check(sameDouble(fieldOf(tscLine, "machine/tsc="),
                   static_cast<double>(kTscStep) / 4.0),
        "the tsc leaf reports its ticks as a count per iteration (FR-039)");
  check(sameDouble(fieldOf(tscLine, "time/iter (ns)="),
                   static_cast<double>(kMonotonicStep) / 4.0),
        "no time is derived from the tsc leaf (FR-039)");
  checkHas(tscLine.substr(tscLine.find("machine/tsc=")),
           "scaled=0",
           "the count carries no published rate (FR-039)");

  const std::string doubled = captureRun(
      {"--filter", "^bmCounted$", "--iterations=1", "--repetitions=2"});
  const std::vector<std::string> counted = linesOf(doubled, "bmCounted");
  // Two repetition rows plus the aggregate row FR-027 adds once the
  // repetition count exceeds one.
  check(counted.size() == 3,
        "two repetitions print two rows and an aggregate (FR-035)");
  check(sameDouble(fieldOf(counted[0], "overhead floor (ns)="),
                   fieldOf(counted[1], "overhead floor (ns)=")),
        "one plan publishes one floor (SC-014)");

  // SC-005 at its stated scale: each call compiles its own plan and so
  // pays the plan's sampling-cost calibration once, which the difference
  // of two calls cancels. Over 10,000 runs the difference between a
  // 10,000-run call and a 9,999-run call is the two endpoint actions of
  // the one extra run, and the timed loop of every one of those runs
  // allocates nothing (bmScripted's body is the allocation probe).
  countAllocations = true;
  const std::uint64_t beforeScripted = fake->readActions();
  captureRun(
      {"--filter", "^bmScripted$", "--iterations=1", "--repetitions=10000"});
  const std::uint64_t scriptedActions = fake->readActions() - beforeScripted;
  const std::uint64_t beforeNine = fake->readActions();
  captureRun(
      {"--filter", "^bmCounted$", "--iterations=1", "--repetitions=9999"});
  const std::uint64_t nineThousandActions = fake->readActions() - beforeNine;
  const std::uint64_t beforeTen = fake->readActions();
  captureRun(
      {"--filter", "^bmCounted$", "--iterations=1", "--repetitions=10000"});
  const std::uint64_t tenThousandActions = fake->readActions() - beforeTen;
  countAllocations = false;
  check(scriptedActions == tenThousandActions,
        "10,000 runs sample two actions each and the loop adds none (SC-005)");
  check(tenThousandActions - nineThousandActions == 2,
        "one measured run is two sampling actions over 10,000 runs (SC-005)");

  std::puts("harness_capture_test: ok");
  return 0;
}
