// ============================================================================
// TDD test for the complexity fit (T027; US7, FR-019 to FR-023, SC-04).
//
// `BenchmarkHandle::complexity` asks for one least-squares fit per family over
// its instances, and the fit reports one pair of rows per fitted quantity: the
// primary time, each named user counter, and each attached metric (FR-022).
// The coefficient is the least-squares solution k = sum(t * f(N)) /
// sum(f(N)^2), and the RMS is the root-mean of the residuals t - k * f(N)
// divided by the mean of the values, times 100, printed unscaled by the unit
// (FR-021, FR-022).
//
// Every family here runs on manual time with one iteration, so the primary
// value of an instance is exactly what its body reports and the fit has an
// expected figure with no tolerance band:
//
//   bmQuad     y = 12 N^2 ns      ->  N^2, k = 12.000, rms 0.000
//            counter cells 4000 N^2 (the base never divides the value)
//                                 ->  N^2, k = 4000.000, rms 0.000
//            metric work = 200     ->  1,   k =  200.000, rms 0.000
//   bmCubed  the same y, fixed N^3 ->  N^3, k =  1.524, rms 17.487
//   bmFlat   the same y, fixed 1   ->  1,   k =  336.000, rms 92.582
//   bmNlgN   y = 5 N lgN ns        ->  NlgN, k =  5.000, rms 0.000
//   bmNoisy  y = 10, 40, 90, fixed N
//                                 ->  N,   k = 10.714, rms 15.509
//   bmUser   y = 12 N^2, f = N^2/2 ->  f(N), k = 24.000, rms 0.000
//   bmSingle one instance          ->  the reason naming the two-instance floor
//   bmBadN   an instance with N=0  ->  the reason naming the positive bound
//   bmHalf   a counter some instances never open
//                                    ->  the reason for that quantity's pair
//   bmSkip   one instance skipped    ->  the reason naming the measured floor
//
// `AUTOMATIC` tries each curve and keeps the least RMS with the constant curve
// as the fallback (FR-021); a fixed curve stands whatever its RMS (FR-020); a
// user curve is honored as the curve of the fit (FR-020). Frameworkless
// check()/fail() convention, one capture for the whole matrix.
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

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"

namespace
{

// The planted per-iteration time of the quadratic families, in nanoseconds per
// square of the instance size.
constexpr std::int64_t kTimeCoefficientNs = 12;
// The planted per-iteration counter value, in units of the square of the
// instance size, so the row holds 4000 N^2 and the base leaves it as it stands.
constexpr std::int64_t kCellCoefficient = 4000;
// The scripted metric leaf steps by this much per sampling action, and one
// action belongs to each iteration, so its per-iteration value is constant.
constexpr std::uint64_t kWorkStep = 200;
// The coefficient and RMS the fixed N-cubed curve earns on quadratic data,
// computed by hand from the closed form the harness states: over N = 2, 4, 8
// the times are 48, 192, 768, so sum(t * N^3) = 405888 and sum(N^6) = 266304,
// and the residuals of k = 405888 / 266304 root-mean to 17.487 % of the mean
// time 336.
constexpr double kCubedCoefficient = 405888.0 / 266304.0;
constexpr double kCubedRmsPercent = 17.486774983;
// The coefficient and RMS the constant curve earns on the same data: k is the
// mean time 336, and the residuals -288, -144 and 432 root-mean to 311.0755,
// which is 92.582 % of that mean.
constexpr double kFlatCoefficient = 336.0;
constexpr double kFlatRmsPercent = 92.582009977;
// The planted times of the noisy family at N = 2, 4, 8. Over them
// sum(t * N) = 20 + 160 + 720 = 900 and sum(N^2) = 84, so the fixed N curve
// earns k = 900 / 84 = 75 / 7 with residuals -80/7, -20/7 and 30/7, whose
// root-mean is 7.2375 ns of the mean time 140/3, that is 15.509 %.
constexpr std::int64_t kNoisyTimeSmall = 10;
constexpr std::int64_t kNoisyTimeMiddle = 40;
constexpr std::int64_t kNoisyTimeLarge = 90;
constexpr double kNoisyCoefficient = 75.0 / 7.0;
constexpr double kNoisyRmsPercent = 15.508861381;
// The planted coefficient of the linearithmic family, y = 5 N lgN.
constexpr double kLinearithmicCoefficient = 5.0;

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS COMPLEXITY TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

auto closeTo(const double value, const double expected) -> bool
{
  const double scale = std::max(1.0, std::abs(expected));
  return std::abs(value - expected) <= 0.001 * scale;
}

// Registers the machine leaves the sampling plan compiles, plus the leaf the
// fitted metric reads.
auto scriptProvider() -> void
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  owned->addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  owned->addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  owned->addObject("fit", "fit", "the family the fit reads");
  owned->addCounter("fit", "work", "none", "a count leaf");
  owned->setPoints("machine", "monotonic", {0}, 1000);
  owned->setPoints("machine", "thread_cpu", {0}, 800);
  owned->setPoints("fit", "work", {}, kWorkStep);
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(owned));
  check(registered.has_value(), "the fake provider registers");
}

// The body of the quadratic families: the manual time is exactly the planted
// coefficient times the square of the instance size (FR-019).
auto bmQuadratic(sg::State& state) -> void
{
  const double size = static_cast<double>(state.range(0));
  for (auto index : state) {
    state.setIterationTime(
        static_cast<std::int64_t>(kTimeCoefficientNs * size * size));
    sg::doNotOptimize(index);
  }
}

// The same manual time, with the counter the quadratic family opens and the one
// the partial family opens only on its first instance.
auto bmCells(sg::State& state) -> void
{
  const double size = static_cast<double>(state.range(0));
  for (auto index : state) {
    state.setIterationTime(
        static_cast<std::int64_t>(kTimeCoefficientNs * size * size));
    state.setCounter("cells",
                     {static_cast<double>(kCellCoefficient) * size * size});
    sg::doNotOptimize(index);
  }
}

// The body of the linearithmic family: the manual time is exactly the planted
// coefficient times N lg N, so the fit has a known answer (FR-019, FR-021).
auto bmLinearithmic(sg::State& state) -> void
{
  const double size = static_cast<double>(state.range(0));
  for (auto index : state) {
    state.setIterationTime(static_cast<std::int64_t>(kLinearithmicCoefficient
                                                     * size * std::log2(size)));
    sg::doNotOptimize(index);
  }
}

// The planted time of the noisy family at one instance size.
auto noisyTime(const std::int64_t argument) -> std::int64_t
{
  if (argument <= 2) {
    return kNoisyTimeSmall;
  }
  if (argument <= 4) {
    return kNoisyTimeMiddle;
  }
  return kNoisyTimeLarge;
}

// The body of the noisy family: three planted times no curve reproduces, so the
// coefficient and the RMS of the fixed curve are the hand-computed pair.
auto bmNoisy(sg::State& state) -> void
{
  const std::int64_t planted = noisyTime(state.range(0));
  for (auto index : state) {
    state.setIterationTime(planted);
    sg::doNotOptimize(index);
  }
}

auto bmHalfCells(sg::State& state) -> void
{
  const auto argument = state.range(0);
  const double size = static_cast<double>(argument);
  for (auto index : state) {
    state.setIterationTime(
        static_cast<std::int64_t>(kTimeCoefficientNs * size * size));
    if (argument == 2) {
      state.setCounter("half", {8.0});
    }
    sg::doNotOptimize(index);
  }
}

// The body of the family that states its own problem size: the instance
// argument is half of N, so the same planted time reads as 3 N^2 against the
// stated size and as 12 N^2 against the range value (FR-019).
auto bmSized(sg::State& state) -> void
{
  const double size = static_cast<double>(state.range(0));
  for (auto index : state) {
    state.setComplexityN(static_cast<std::int64_t>(2.0 * size));
    state.setIterationTime(
        static_cast<std::int64_t>(kTimeCoefficientNs * size * size));
    sg::doNotOptimize(index);
  }
}

// The family that runs two repetitions per instance, so the fit reads the
// aggregate row of the instance: its counter and its metric both reach the fit
// through the mean of a statistic set (FR-022).
auto bmRepeating(sg::State& state) -> void
{
  const double size = static_cast<double>(state.range(0));
  for (auto index : state) {
    state.setIterationTime(
        static_cast<std::int64_t>(kTimeCoefficientNs * size * size));
    state.setCounter("packed",
                     {static_cast<double>(kCellCoefficient) * size * size});
    sg::doNotOptimize(index);
  }
}

// A counter whose two instances cancel: the values average zero and the fit
// cannot reproduce them, so the error of the fit has no mean to divide by.
auto bmZeroSum(sg::State& state) -> void
{
  const double size = static_cast<double>(state.range(0));
  const double delta = size < 3.0 ? 1000.0 : -1000.0;
  for (auto index : state) {
    state.setIterationTime(kTimeCoefficientNs);
    state.setCounter("delta", {delta});
    sg::doNotOptimize(index);
  }
}

// A counter that is zero at every size: the fit reproduces the values exactly,
// so the same zero mean earns a zero error.
auto bmZeroValues(sg::State& state) -> void
{
  for (auto index : state) {
    state.setIterationTime(kTimeCoefficientNs);
    state.setCounter("quiet", {0.0});
    sg::doNotOptimize(index);
  }
}

// A family whose instances carry no argument and state no size: the size of
// such an instance is zero, the value the fit reads when neither the body nor
// the range names one (FR-019).
auto bmSizeless(sg::State& state) -> void
{
  for (auto index : state) {
    state.setIterationTime(kTimeCoefficientNs);
    sg::doNotOptimize(index);
  }
}

// A body that speaks no problem size and takes real time, so the fit of the
// family reads the row's own real time per iteration (FR-022).
auto bmRealFit(sg::State& state) -> void
{
  for (auto index : state) {
    sg::doNotOptimize(index);
  }
}

// The body of the family that skips one instance: the instance that measures
// plants the quadratic time, the other leaves the loop before it plants
// anything, so the family holds one measured value (FR-019, FR-022).
auto bmSkipped(sg::State& state) -> void
{
  if (state.range(0) == 4) {
    state.skipWithMessage("the scripted gap");
    return;
  }
  const double size = static_cast<double>(state.range(0));
  for (auto index : state) {
    state.setIterationTime(
        static_cast<std::int64_t>(kTimeCoefficientNs * size * size));
    sg::doNotOptimize(index);
  }
}

auto captureRun(const std::vector<std::string>& arguments,
                int& status) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_complexity_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  const char* const path = "harness_complexity_out.txt";
  const int saved = dup(STDOUT_FILENO);
  if (saved < 0 || std::freopen(path, "w", stdout) == nullptr) {
    fail("cannot redirect the report");
  }
  status = sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
  std::fflush(stdout);
  if (dup2(saved, STDOUT_FILENO) < 0) {
    fail("cannot restore stdout");
  }
  close(saved);

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

// The one fit row of `name` that carries `kind` and the quantity, empty when no
// such row printed (FR-022).
auto fitRowOf(const std::vector<std::string>& lines,
              const std::string& kind,
              const std::string& quantity) -> std::string
{
  for (const auto& line : lines) {
    if (line.contains(kind) && line.contains(quantity)) {
      return line;
    }
  }
  return {};
}

// The curve token of a BigO row: the word between the quantity and the
// coefficient in parentheses.
auto curveOf(const std::string& row, const std::string& quantity) -> std::string
{
  const std::size_t at = row.find(quantity);
  if (at == std::string::npos) {
    return {};
  }
  std::istringstream stream(row.substr(at + quantity.size()));
  std::string curve;
  std::string rest;
  stream >> curve >> rest;
  return curve;
}

// The coefficient of a BigO row, the number inside the closing pair of
// parentheses: the curve token of a user function holds parentheses of its own,
// so the coefficient is read from the last pair of the line.
auto coefficientOf(const std::string& row) -> double
{
  const std::size_t close = row.rfind(')');
  const std::size_t open =
      close == std::string::npos ? close : row.rfind('(', close);
  if (open == std::string::npos || close == std::string::npos) {
    fail("a BigO row carries no coefficient");
  }
  return std::stod(row.substr(open + 1, close - open - 1));
}

// The RMS percentage of an RMS row, the number before the percent sign.
auto rmsOf(const std::string& row) -> double
{
  const std::size_t sign = row.find('%');
  if (sign == std::string::npos) {
    fail("an RMS row carries no percent sign");
  }
  const std::size_t space = row.rfind(' ', sign);
  return std::stod(row.substr(space + 1, sign - space - 1));
}

// FR-019, FR-021, FR-022: the automatic fit names the planted class, its
// coefficient and its RMS for the primary time, for the named counter and for
// the attached metric, each in the order time, counters, metrics.
auto checkAutomatic(const std::vector<std::string>& lines) -> void
{
  const std::string timeRow = fitRowOf(lines, "BIG_O", "time/iter");
  check(!timeRow.empty(), "the primary time of the family is fitted (FR-022)");
  check(curveOf(timeRow, "time/iter") == "N^2",
        "the automatic fit keeps the least-RMS curve (FR-021)");
  check(
      closeTo(coefficientOf(timeRow), static_cast<double>(kTimeCoefficientNs)),
      "the least-squares closed form gives the planted coefficient (FR-021)");
  const std::string timeRms = fitRowOf(lines, "RMS", "time/iter");
  check(!timeRms.empty() && rmsOf(timeRms) <= 0.0005,
        "an exact planted curve earns a zero RMS (FR-022)");

  const std::string cellRow = fitRowOf(lines, "BIG_O", "cells");
  check(!cellRow.empty(), "a named user counter is a fitted quantity (FR-022)");
  check(curveOf(cellRow, "cells") == "N^2",
        "the counter pair names the class of the counter values (FR-022)");
  check(closeTo(coefficientOf(cellRow), static_cast<double>(kCellCoefficient)),
        "a counter coefficient stands in that quantity's value (FR-022)");
  check(rmsOf(fitRowOf(lines, "RMS", "cells")) <= 0.0005,
        "the counter RMS stands unscaled by the unit (FR-022)");

  const std::string workRow = fitRowOf(lines, "BIG_O", "work");
  check(!workRow.empty(), "an attached metric is a fitted quantity (FR-022)");
  check(curveOf(workRow, "work") == "1",
        "a constant quantity falls to the constant curve (FR-021)");
  check(closeTo(coefficientOf(workRow), static_cast<double>(kWorkStep)),
        "the constant curve carries the constant value (FR-021)");
}

// FR-021, FR-022: an exact linearithmic planting names NlgN, carries its
// planted factor and earns no residual.
auto checkLinearithmic(const std::vector<std::string>& lines) -> void
{
  const std::string row = fitRowOf(lines, "BIG_O", "time/iter");
  check(!row.empty(), "the linearithmic family is fitted (FR-019)");
  check(curveOf(row, "time/iter") == "NlgN",
        "the automatic fit keeps the linearithmic curve (FR-021)");
  check(closeTo(coefficientOf(row), kLinearithmicCoefficient),
        "the closed form gives the factor of an exact planting (FR-021)");
  check(rmsOf(fitRowOf(lines, "RMS", "time/iter")) <= 0.0005,
        "an exact linearithmic planting earns a zero RMS (FR-022)");
}

// FR-020, FR-021, FR-022: the hand-computed noisy vector over the times 10,
// 40 and 90, where k = 900 / 84 and the RMS is 15.509 % of the mean.
auto checkNoisy(const std::vector<std::string>& lines) -> void
{
  const std::string row = fitRowOf(lines, "BIG_O", "time/iter");
  check(!row.empty(), "the noisy family is fitted (FR-019)");
  check(curveOf(row, "time/iter") == "N",
        "a family fixed to N keeps that curve on noisy data (FR-020)");
  check(closeTo(coefficientOf(row), kNoisyCoefficient),
        "the hand-computed sums give the coefficient (FR-021)");
  check(closeTo(rmsOf(fitRowOf(lines, "RMS", "time/iter")), kNoisyRmsPercent),
        "the hand-computed residuals give the RMS (FR-022)");
}

// The row index of the first fit row of one quantity, the largest value when it
// printed no row at all.
auto rowIndexOf(const std::vector<std::string>& lines,
                const std::string& kind,
                const std::string& quantity) -> std::size_t
{
  for (std::size_t index = 0; index < lines.size(); ++index) {
    if (lines[index].contains(kind) && lines[index].contains(quantity)) {
      return index;
    }
  }
  return lines.size();
}

// FR-022, research D-09: the pairs print in the order time, counters, metrics.
auto checkOrder(const std::vector<std::string>& lines) -> void
{
  const std::size_t time = rowIndexOf(lines, "BIG_O", "time/iter");
  const std::size_t cells = rowIndexOf(lines, "BIG_O", "cells");
  const std::size_t work = rowIndexOf(lines, "BIG_O", "work");
  check(time < cells && cells < work,
        "the fit pairs print in the order time, counters, metrics (FR-022)");
}

// FR-019: the instance N is the value setComplexityN stated when it stands, and
// the range value only when it does not.
auto checkStatedSize(const std::vector<std::string>& lines) -> void
{
  const std::string row = fitRowOf(lines, "BIG_O", "time/iter");
  check(!row.empty(), "the family that states its size is fitted (FR-019)");
  check(curveOf(row, "time/iter") == "N^2",
        "the stated size carries the class of the planted time (FR-019)");
  check(closeTo(coefficientOf(row), 3.0),
        "the fit reads the stated size, not the range value (FR-019)");
}

// FR-020: a family fixed to one curve prints that curve whatever its RMS.
auto checkFixedCurve(const std::vector<std::string>& lines) -> void
{
  const std::string row = fitRowOf(lines, "BIG_O", "time/iter");
  check(!row.empty(), "the fixed family is fitted (FR-020)");
  check(curveOf(row, "time/iter") == "N^3",
        "a family fixed to a curve keeps it (FR-020)");
  check(closeTo(coefficientOf(row), kCubedCoefficient),
        "the closed form gives the coefficient of the fixed curve (FR-021)");
  check(closeTo(rmsOf(fitRowOf(lines, "RMS", "time/iter")), kCubedRmsPercent),
        "the RMS is the root-mean relative error times 100 (FR-022)");
}

// FR-020: a family fixed to the constant curve prints it over data of another
// class, with the error that choice earns.
auto checkFixedConstant(const std::vector<std::string>& lines) -> void
{
  const std::string row = fitRowOf(lines, "BIG_O", "time/iter");
  check(!row.empty(),
        "the family fixed to the constant curve is fitted (FR-020)");
  check(curveOf(row, "time/iter") == "1",
        "a family fixed to the constant curve keeps it (FR-020)");
  check(closeTo(coefficientOf(row), kFlatCoefficient),
        "the constant curve carries the closed-form coefficient (FR-021)");
  check(closeTo(rmsOf(fitRowOf(lines, "RMS", "time/iter")), kFlatRmsPercent),
        "the RMS of the fixed curve stands whatever its size (FR-020)");
}

// FR-020: a user-supplied curve function is the curve of the fit.
auto checkUserCurve(const std::vector<std::string>& lines) -> void
{
  const std::string row = fitRowOf(lines, "BIG_O", "time/iter");
  check(!row.empty(), "the user-curve family is fitted (FR-020)");
  check(curveOf(row, "time/iter") == "f(N)",
        "a user curve is honored as the curve of the fit (FR-020)");
  check(closeTo(coefficientOf(row), 24.0),
        "the closed form carries the coefficient of the user curve (FR-021)");
  check(rmsOf(fitRowOf(lines, "RMS", "time/iter")) <= 0.0005,
        "the planted user curve fits the planted data exactly (FR-022)");
}

// A family of the planted quadratic with the curve the scenario asks for, over
// the instance sizes the scenario passes. One `arg` call stands one instance,
// so three calls give the family the three instances a fit reads (FR-019).
auto registerQuadratic(const char* name,
                       const sg::BigO bigO,
                       const std::vector<std::int64_t>& sizes)
    -> sg::BenchmarkHandle
{
  auto handle = sg::registerBenchmark(&bmQuadratic, name);
  check(handle.name() == name, "the family registers");
  handle.useManualTime();
  for (const std::int64_t size : sizes) {
    handle.arg(size);
  }
  handle.complexity(bigO);
  return handle;
}

// FR-022: a quantity whose values average zero and which the fit cannot
// reproduce reports an infinite error, because the relative root-mean-square
// has no mean left to divide the residual by.
auto checkZeroMeanUnreproduced(const std::vector<std::string>& lines) -> void
{
  const std::string row = fitRowOf(lines, "RMS", "delta");
  check(!row.empty(),
        "a quantity whose values average zero earns an RMS row (FR-022)");
  check(std::isinf(rmsOf(row)),
        "a zero mean the fit misses reports an infinite error (FR-022)");
}

// FR-022: a quantity whose values average zero and which the fit reproduces
// reports a zero error: the residual is nothing, so the fit stands.
auto checkZeroMeanReproduced(const std::vector<std::string>& lines) -> void
{
  const std::string row = fitRowOf(lines, "RMS", "quiet");
  check(!row.empty(),
        "a quantity that is zero at every size earns an RMS row (FR-022)");
  check(closeTo(rmsOf(row), 0.0),
        "a zero mean the fit reproduces reports a zero error (FR-022)");
  check(closeTo(coefficientOf(fitRowOf(lines, "BIG_O", "quiet")), 0.0),
        "the fit of values that are all zero carries a zero coefficient");
}

// FR-022: the quantity of a family that runs several repetitions is the mean of
// its statistic set, so the planted curve still names the class and carries the
// planted coefficient through the aggregate row.
auto checkAggregateQuantities(const std::vector<std::string>& lines) -> void
{
  const std::string counterRow = fitRowOf(lines, "BIG_O", "packed");
  check(!counterRow.empty(),
        "a counter of a repeated family is a fitted quantity (FR-022)");
  check(curveOf(counterRow, "packed") == "N^2",
        "the aggregate mean of a counter names the same class as its rows");
  check(
      closeTo(coefficientOf(counterRow), static_cast<double>(kCellCoefficient)),
      "the aggregate mean carries the planted coefficient (FR-022)");
  const std::string metricRow = fitRowOf(lines, "BIG_O", "flux");
  check(!metricRow.empty(),
        "a metric of a repeated family is a fitted quantity (FR-022)");
  check(curveOf(metricRow, "flux") == "1",
        "the aggregate mean of a constant metric keeps the constant curve");
}

}  // namespace

auto main() -> int
{
  scriptProvider();

  auto cells = sg::registerBenchmark(&bmCells, "bmQuad");
  check(cells.name() == "bmQuad", "the quadratic family registers");
  cells.useManualTime();
  cells.arg(2).arg(4).arg(8).complexity(sg::BigO::AUTOMATIC);
  const auto object = sg::counters::System::local().object("fit");
  check(object.has_value(), "the metric object resolves");
  const auto work = object->counter<sg::counters::Dim<0, 1>>("work");
  check(work.has_value(), "the metric leaf resolves");
  cells.addMetric(sg::counters::Expression<sg::counters::Dim<0, 1>>(*work),
                  "work");

  registerQuadratic("bmCubed", sg::BigO::N_CUBED, {2, 4, 8});
  registerQuadratic("bmFlat", sg::BigO::ONE, {2, 4, 8});
  auto nlg = sg::registerBenchmark(&bmLinearithmic, "bmNlgN");
  check(nlg.name() == "bmNlgN", "the linearithmic family registers");
  nlg.useManualTime().arg(2).arg(4).arg(8).complexity(sg::BigO::AUTOMATIC);
  auto noisy = sg::registerBenchmark(&bmNoisy, "bmNoisy");
  check(noisy.name() == "bmNoisy", "the noisy family registers");
  noisy.useManualTime().arg(2).arg(4).arg(8).complexity(sg::BigO::N);
  auto sized = sg::registerBenchmark(&bmSized, "bmSized");
  check(sized.name() == "bmSized", "the sized family registers");
  // Two repetitions give this family an aggregate row, so the fit reads the
  // summary of the instance in place of a single row (FR-022).
  sized.useManualTime().arg(1).arg(2).arg(4).repetitions(2).complexity(
      sg::BigO::AUTOMATIC);
  auto user = sg::registerBenchmark(&bmQuadratic, "bmUser");
  check(user.name() == "bmUser", "the user-curve family registers");
  user.useManualTime().arg(2).arg(4).arg(8).complexity([](const double n)
                                                       { return n * n * 0.5; });

  registerQuadratic("bmSingle", sg::BigO::AUTOMATIC, {4});
  auto badN = sg::registerBenchmark(&bmQuadratic, "bmBadN");
  check(badN.name() == "bmBadN", "the non-positive family registers");
  badN.useManualTime().arg(0).arg(4).complexity(sg::BigO::AUTOMATIC);

  auto zeroSum = sg::registerBenchmark(&bmZeroSum, "bmZeroSum");
  check(zeroSum.name() == "bmZeroSum", "the canceling family registers");
  zeroSum.useManualTime().arg(2).arg(4).complexity(sg::BigO::AUTOMATIC);
  auto repeating = sg::registerBenchmark(&bmRepeating, "bmRepeating");
  check(repeating.name() == "bmRepeating", "the repeated family registers");
  repeating.useManualTime().arg(2).arg(4).arg(8).repetitions(2).complexity(
      sg::BigO::AUTOMATIC);
  const auto repeatedObject = sg::counters::System::local().object("fit");
  check(repeatedObject.has_value(), "the metric object resolves again");
  const auto flux = repeatedObject->counter<sg::counters::Dim<0, 1>>("work");
  check(flux.has_value(), "the metric leaf resolves again");
  repeating.addMetric(sg::counters::Expression<sg::counters::Dim<0, 1>>(*flux),
                      "flux");
  auto zeroValues = sg::registerBenchmark(&bmZeroValues, "bmZeroValues");
  check(zeroValues.name() == "bmZeroValues", "the all-zero family registers");
  zeroValues.useManualTime().arg(2).arg(4).complexity(sg::BigO::AUTOMATIC);

  auto zeroCurve = sg::registerBenchmark(&bmQuadratic, "bmZeroCurve");
  check(zeroCurve.name() == "bmZeroCurve", "the zero-curve family registers");
  // A curve that is zero at every size leaves the least squares no denominator
  // to divide by, so the family earns no coefficient (FR-021, FR-022).
  zeroCurve.useManualTime().arg(2).arg(4).complexity([](const double)
                                                     { return 0.0; });

  auto sizeless = sg::registerBenchmark(&bmSizeless, "bmSizeless");
  check(sizeless.name() == "bmSizeless", "the argument-less family registers");
  // No `arg` call stands one instance that carries no argument, so its size is
  // the zero the fit reads from an instance that names no size (FR-019).
  sizeless.useManualTime().complexity(sg::BigO::N);

  auto realFit = sg::registerBenchmark(&bmRealFit, "bmRealFit");
  check(realFit.name() == "bmRealFit", "the real-time fit family registers");
  // One repetition leaves the instance no aggregate row, so the fit reads the
  // real time per iteration of the row itself (FR-022).
  realFit.useRealTime().arg(2).arg(4).complexity(sg::BigO::N);

  auto half = sg::registerBenchmark(&bmHalfCells, "bmHalf");
  check(half.name() == "bmHalf", "the partial family registers");
  // The instance that opens the counter stands last, so the name joins the
  // family's quantity list after an instance that carries no value of it.
  half.useManualTime().arg(4).arg(2).complexity(sg::BigO::AUTOMATIC);

  auto skipped = sg::registerBenchmark(&bmSkipped, "bmSkip");
  check(skipped.name() == "bmSkip", "the family with a skip registers");
  // The skipped instance reaches the report with a result and the family asks
  // for a fit, so its values stand out of the samples the fit reads and one
  // measured value is left below the floor (FR-022, FR-023).
  skipped.useManualTime().arg(2).arg(4).complexity(sg::BigO::N);

  int status = 0;
  const std::string report = captureRun(
      {"--filter", "^bm", "--iterations=1", "--repetitions=1"}, status);
  if (status != 0) {
    std::fprintf(stderr, "speedgunMain returned %d\n", status);
    fail("a family with a fit still reports (FR-015, FR-023)");
  }

  checkAutomatic(linesOf(report, "bmQuad"));
  checkOrder(linesOf(report, "bmQuad"));
  checkFixedCurve(linesOf(report, "bmCubed"));
  checkFixedConstant(linesOf(report, "bmFlat"));
  checkStatedSize(linesOf(report, "bmSized"));
  checkUserCurve(linesOf(report, "bmUser"));
  checkLinearithmic(linesOf(report, "bmNlgN"));
  checkNoisy(linesOf(report, "bmNoisy"));
  checkZeroMeanUnreproduced(linesOf(report, "bmZeroSum"));
  checkZeroMeanReproduced(linesOf(report, "bmZeroValues"));
  checkAggregateQuantities(linesOf(report, "bmRepeating"));

  const std::vector<std::string> singleLines = linesOf(report, "bmSingle");
  check(fitRowOf(singleLines, "BIG_O", "time/iter").empty(),
        "a family of one instance holds no fit row (FR-023)");
  check(fitRowOf(singleLines, "SKIPPED", "bmSingle").contains("two"),
        "a family of one instance states the reason naming the floor of two "
        "instances (FR-023)");

  const std::vector<std::string> badLines = linesOf(report, "bmBadN");
  check(fitRowOf(badLines, "BIG_O", "time/iter").empty(),
        "an instance without a positive N holds no fit row (FR-023)");
  check(fitRowOf(badLines, "SKIPPED", "bmBadN").contains("positive"),
        "the family states the reason naming the positive bound (FR-023)");

  const std::vector<std::string> skipLines = linesOf(report, "bmSkip");
  check(fitRowOf(skipLines, "BIG_O", "time/iter").empty(),
        "a family whose instance skipped holds no fit row (FR-022, FR-023)");
  check(fitRowOf(skipLines, "SKIPPED", "holds no fit").contains("measured"),
        "the family states the reason naming the measured values it lacks "
        "(FR-023)");

  const std::vector<std::string> halfLines = linesOf(report, "bmHalf");
  check(!fitRowOf(halfLines, "BIG_O", "time/iter").empty(),
        "the quantity every instance carries still fits (FR-022)");
  check(fitRowOf(halfLines, "BIG_O", "half").empty()
            && fitRowOf(halfLines, "RMS", "half").empty(),
        "a quantity missing from an instance holds no pair (spec Edge Cases)");
  check(fitRowOf(halfLines, "SKIPPED", "half").contains("every instance"),
        "the missing quantity states its reason (spec Edge Cases)");

  const std::vector<std::string> zeroCurveLines =
      linesOf(report, "bmZeroCurve");
  const std::string zeroCurveRow =
      fitRowOf(zeroCurveLines, "BIG_O", "time/iter");
  check(!zeroCurveRow.empty(),
        "a family whose curve is zero at every size still prints its quantity "
        "(FR-021)");
  check(closeTo(coefficientOf(zeroCurveRow), 0.0),
        "a curve with no denominator leaves no coefficient (FR-021, FR-022)");
  check(std::isinf(rmsOf(fitRowOf(zeroCurveLines, "RMS", "time/iter"))),
        "a curve that reproduces nothing reports an infinite error (FR-022)");

  const std::vector<std::string> sizelessLines = linesOf(report, "bmSizeless");
  check(fitRowOf(sizelessLines, "BIG_O", "time/iter").empty(),
        "an instance that carries no size holds no fit row (FR-019, FR-023)");
  check(!sizelessLines.empty() && sizelessLines[0].contains("iterations="),
        "the argument-less instance still prints its measured row (FR-019)");

  const std::vector<std::string> realFitLines = linesOf(report, "bmRealFit");
  const std::string realFitRow = fitRowOf(realFitLines, "BIG_O", "time/iter");
  check(!realFitRow.empty(),
        "a family without an aggregate row fits the row time it printed "
        "(FR-022)");
  check(std::isfinite(rmsOf(fitRowOf(realFitLines, "RMS", "time/iter"))),
        "the real-time row of one repetition carries a finite error (FR-022)");

  std::puts("harness_complexity_test: ok");
  return 0;
}
