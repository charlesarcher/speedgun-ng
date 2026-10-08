// ============================================================================
// SC-008 checked-build overhead harness (T018).
//
// Two near-identical hot loops timed with std::chrono::steady_clock:
//   uncontracted: payload only
//   contracted  : the same payload plus a satisfied SG_REQUIRE per iteration
//
// The predicate is a runtime value (loaded from volatile each iteration) so
// the check cannot be constant-folded. This translation unit is a documented
// measurement, not a CI gate; it is compiled -O2 by tools/dbc/overhead.sh.
// ============================================================================

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "speedgun-ng/dbc.hpp"

#if defined(__GNUC__)
#  define SG_OH_NOINLINE __attribute__((noinline))
#else
#  define SG_OH_NOINLINE
#endif

// Measurement harness. The file is outside the library: short names in the timed
// loops and parseable standard I/O are the point of the file.
// NOLINTBEGIN(readability-identifier-length,bugprone-easily-swappable-parameters,cppcoreguidelines-avoid-magic-numbers,cppcoreguidelines-pro-type-vararg,hicpp-vararg,cert-err33-c,google-runtime-int,cppcoreguidelines-pro-bounds-pointer-arithmetic,readability-use-std-min-max,readability-math-missing-parentheses,modernize-use-ranges,boost-use-ranges)

namespace
{

using Clock = std::chrono::steady_clock;

constexpr int kDefaultN = 100000000;
constexpr int kDefaultTrials = 51;
// A trial shorter than this is below a credible Clock sample (tiny N).
constexpr std::int64_t kMinTrialNs = 1000000;

SG_OH_NOINLINE auto uncontractedLoop(int const n, int const x) -> std::int64_t
{
  std::int64_t acc = 0;
  volatile int vx = x;
  for (int i = 0; i < n; ++i) {
    int const cur = vx;
    acc += cur;
  }
  return acc;
}

SG_OH_NOINLINE auto contractedLoop(int const n, int const x) -> std::int64_t
{
  std::int64_t acc = 0;
  volatile int vx = x;
  for (int i = 0; i < n; ++i) {
    int const cur = vx;
    SG_REQUIRE(cur > 0, "runtime predicate: cur > 0");
    acc += cur;
  }
  return acc;
}

auto nowNs(Clock::time_point const t0,
            Clock::time_point const t1) -> std::int64_t
{
  return std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
}

auto timeUncontracted(int const n,
                       int const x,
                       std::int64_t& sink) -> std::int64_t
{
  auto const t0 = Clock::now();
  sink += uncontractedLoop(n, x);
  auto const t1 = Clock::now();
  return nowNs(t0, t1);
}

auto timeContracted(int const n,
                     int const x,
                     std::int64_t& sink) -> std::int64_t
{
  auto const t0 = Clock::now();
  sink += contractedLoop(n, x);
  auto const t1 = Clock::now();
  return nowNs(t0, t1);
}

auto nearestRank(std::vector<double> const& sorted,
                  int const percent) -> double
{
  auto const n = sorted.size();
  if (n == 0U) {
    return 0.0;
  }
  auto rank = (static_cast<std::size_t>(percent) * n + 99U) / 100U;
  if (rank < 1U) {
    rank = 1U;
  }
  if (rank > n) {
    rank = n;
  }
  return sorted[rank - 1U];
}

auto printCsv(char const* const key, std::vector<double> const& values) -> void
{
  std::fputs(key, stdout);
  std::fputc('=', stdout);
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0U) {
      std::fputc(',', stdout);
    }
    std::printf("%.6f", values[i]);
  }
  std::fputc('\n', stdout);
}

auto printDist(char const* const prefix, std::vector<double> sorted) -> void
{
  std::sort(sorted.begin(), sorted.end());
  std::printf("%s_min=%.6f\n", prefix, sorted.front());
  std::printf("%s_max=%.6f\n", prefix, sorted.back());
  std::printf("%s_n50=%.6f\n", prefix, nearestRank(sorted, 50));
  std::printf("%s_n99=%.6f\n", prefix, nearestRank(sorted, 99));
}

auto parsePositiveInt(char const* const text, int const fallback) -> int
{
  if (text == nullptr || text[0] == '\0') {
    return fallback;
  }
  char* end = nullptr;
  long const value = std::strtol(text, &end, 10);
  if (end == text || *end != '\0' || value <= 0 || value > 2000000000L) {
    return -1;
  }
  return static_cast<int>(value);
}

}  // namespace

auto main(int argc, char** argv) -> int
{
  int const n = parsePositiveInt(argc > 1 ? argv[1] : nullptr, kDefaultN);
  int const trials =
      parsePositiveInt(argc > 2 ? argv[2] : nullptr, kDefaultTrials);
  if (n < 1 || trials < 1) {
    std::fprintf(stderr, "overhead: usage: overhead [N] [trials]\n");
    return 2;
  }

  volatile int seed = 1;
  int const x = seed;

  std::int64_t sink = 0;
  // Full-N warm-up so timed trials are not dominated by cold I-cache.
  sink += uncontractedLoop(n, x);
  sink += contractedLoop(n, x);
  sink += uncontractedLoop(n, x);
  sink += contractedLoop(n, x);

  std::vector<double> uncontractedNsPerIter;
  std::vector<double> contractedNsPerIter;
  std::vector<double> overheadNsPerIter;
  uncontractedNsPerIter.reserve(static_cast<std::size_t>(trials));
  contractedNsPerIter.reserve(static_cast<std::size_t>(trials));
  overheadNsPerIter.reserve(static_cast<std::size_t>(trials));

  std::int64_t uncontractedSumNs = 0;
  std::int64_t contractedSumNs = 0;
  std::int64_t uncontractedMinNs = 0;
  bool first = true;
  bool anyZero = false;

  for (int trial = 0; trial < trials; ++trial) {
    // ABBA pairing cancels first-of-pair warm-up bias.
    std::int64_t uNs = 0;
    std::int64_t cNs = 0;
    if (trial % 2 == 0) {
      auto const u1 = timeUncontracted(n, x, sink);
      auto const c1 = timeContracted(n, x, sink);
      auto const c2 = timeContracted(n, x, sink);
      auto const u2 = timeUncontracted(n, x, sink);
      uNs = u1 + u2;
      cNs = c1 + c2;
    } else {
      auto const c1 = timeContracted(n, x, sink);
      auto const u1 = timeUncontracted(n, x, sink);
      auto const u2 = timeUncontracted(n, x, sink);
      auto const c2 = timeContracted(n, x, sink);
      uNs = u1 + u2;
      cNs = c1 + c2;
    }
    uncontractedSumNs += uNs;
    contractedSumNs += cNs;
    if (first || uNs < uncontractedMinNs) {
      uncontractedMinNs = uNs;
    }
    first = false;
    if (uNs <= 0) {
      anyZero = true;
    }
    // Each trial now covers 2N iterations (the ABBA pair).
    double const invN = 1.0 / (2.0 * static_cast<double>(n));
    double const u = static_cast<double>(uNs) * invN;
    double const c = static_cast<double>(cNs) * invN;
    uncontractedNsPerIter.push_back(u);
    contractedNsPerIter.push_back(c);
    overheadNsPerIter.push_back(c - u);
  }

  volatile std::int64_t live = sink;
  static_cast<void>(live);

  std::vector<double> uncontractedSorted = uncontractedNsPerIter;
  std::vector<double> contractedSorted = contractedNsPerIter;
  std::sort(uncontractedSorted.begin(), uncontractedSorted.end());
  std::sort(contractedSorted.begin(), contractedSorted.end());
  double const uncontractedN50 = nearestRank(uncontractedSorted, 50);
  double const contractedN50 = nearestRank(contractedSorted, 50);
  // Predicted-taken branch cost is often inside Clock noise; treat
  // contracted as >= baseline if it is within 10% of uncontracted n50.
  double const baselineFloor = uncontractedN50 * 0.9;

  char const* reason = "ok";
  int valid = 1;
  if (anyZero || uncontractedSumNs <= 0) {
    reason = "uncontracted baseline is zero";
    valid = 0;
  } else if (uncontractedMinNs < kMinTrialNs) {
    reason = "uncontracted trial below 1ms credibility floor";
    valid = 0;
  } else if (contractedSumNs < 0 || contractedN50 < baselineFloor) {
    reason = "contracted loop faster than uncontracted baseline";
    valid = 0;
  }

  std::printf("n=%d\n", n);
  std::printf("trials=%d\n", trials);
  std::printf("uncontracted_sum_ns=%lld\n",
              static_cast<long long>(uncontractedSumNs));
  std::printf("contracted_sum_ns=%lld\n",
              static_cast<long long>(contractedSumNs));
  printDist("uncontracted_ns_per_iter", uncontractedNsPerIter);
  printDist("contracted_ns_per_iter", contractedNsPerIter);
  printDist("overhead_ns_per_iter", overheadNsPerIter);
  printCsv("raw_uncontracted_ns_per_iter", uncontractedNsPerIter);
  printCsv("raw_contracted_ns_per_iter", contractedNsPerIter);
  printCsv("raw_overhead_ns_per_iter", overheadNsPerIter);
  std::printf("valid=%d\n", valid);
  std::printf("reason=%s\n", reason);

  return valid == 1 ? 0 : 1;
}

// NOLINTEND
