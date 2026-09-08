// ============================================================================
// SC-008 checked-build overhead harness (T018).
//
// Two near-identical hot loops timed with std::chrono::steady_clock:
//   uncontracted — payload only
//   contracted   — the same payload plus a satisfied SG_REQUIRE per iteration
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

// Measurement harness, not a library TU: short names in the timed
// loops and parseable stdio are the point of the file.
// NOLINTBEGIN(readability-identifier-length,bugprone-easily-swappable-parameters,cppcoreguidelines-avoid-magic-numbers,cppcoreguidelines-pro-type-vararg,hicpp-vararg,cert-err33-c,google-runtime-int,cppcoreguidelines-pro-bounds-pointer-arithmetic,readability-use-std-min-max,readability-math-missing-parentheses,modernize-use-ranges,boost-use-ranges)

namespace
{

using clock = std::chrono::steady_clock;

constexpr int k_default_n = 100000000;
constexpr int k_default_trials = 51;
// A trial shorter than this is below a credible clock sample (tiny N).
constexpr std::int64_t k_min_trial_ns = 1000000;

SG_OH_NOINLINE auto uncontracted_loop(int const n, int const x) -> std::int64_t
{
  std::int64_t acc = 0;
  volatile int vx = x;
  for (int i = 0; i < n; ++i) {
    int const cur = vx;
    acc += cur;
  }
  return acc;
}

SG_OH_NOINLINE auto contracted_loop(int const n, int const x) -> std::int64_t
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

auto now_ns(clock::time_point const t0, clock::time_point const t1)
    -> std::int64_t
{
  return std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
}

auto time_uncontracted(int const n, int const x, std::int64_t& sink)
    -> std::int64_t
{
  auto const t0 = clock::now();
  sink += uncontracted_loop(n, x);
  auto const t1 = clock::now();
  return now_ns(t0, t1);
}

auto time_contracted(int const n, int const x, std::int64_t& sink)
    -> std::int64_t
{
  auto const t0 = clock::now();
  sink += contracted_loop(n, x);
  auto const t1 = clock::now();
  return now_ns(t0, t1);
}

auto nearest_rank(std::vector<double> const& sorted, int const percent)
    -> double
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

auto print_csv(char const* const key, std::vector<double> const& values) -> void
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

auto print_dist(char const* const prefix, std::vector<double> sorted) -> void
{
  std::sort(sorted.begin(), sorted.end());
  std::printf("%s_min=%.6f\n", prefix, sorted.front());
  std::printf("%s_max=%.6f\n", prefix, sorted.back());
  std::printf("%s_n50=%.6f\n", prefix, nearest_rank(sorted, 50));
  std::printf("%s_n99=%.6f\n", prefix, nearest_rank(sorted, 99));
}

auto parse_positive_int(char const* const text, int const fallback) -> int
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
  int const n = parse_positive_int(argc > 1 ? argv[1] : nullptr, k_default_n);
  int const trials =
      parse_positive_int(argc > 2 ? argv[2] : nullptr, k_default_trials);
  if (n < 1 || trials < 1) {
    std::fprintf(stderr, "overhead: usage: overhead [N] [trials]\n");
    return 2;
  }

  volatile int seed = 1;
  int const x = seed;

  std::int64_t sink = 0;
  // Full-N warmup so timed trials are not dominated by cold I-cache.
  sink += uncontracted_loop(n, x);
  sink += contracted_loop(n, x);
  sink += uncontracted_loop(n, x);
  sink += contracted_loop(n, x);

  std::vector<double> uncontracted_ns_per_iter;
  std::vector<double> contracted_ns_per_iter;
  std::vector<double> overhead_ns_per_iter;
  uncontracted_ns_per_iter.reserve(static_cast<std::size_t>(trials));
  contracted_ns_per_iter.reserve(static_cast<std::size_t>(trials));
  overhead_ns_per_iter.reserve(static_cast<std::size_t>(trials));

  std::int64_t uncontracted_sum_ns = 0;
  std::int64_t contracted_sum_ns = 0;
  std::int64_t uncontracted_min_ns = 0;
  bool first = true;
  bool any_zero = false;

  for (int trial = 0; trial < trials; ++trial) {
    // ABBA pairing cancels first-of-pair warmup bias.
    std::int64_t u_ns = 0;
    std::int64_t c_ns = 0;
    if (trial % 2 == 0) {
      auto const u1 = time_uncontracted(n, x, sink);
      auto const c1 = time_contracted(n, x, sink);
      auto const c2 = time_contracted(n, x, sink);
      auto const u2 = time_uncontracted(n, x, sink);
      u_ns = u1 + u2;
      c_ns = c1 + c2;
    } else {
      auto const c1 = time_contracted(n, x, sink);
      auto const u1 = time_uncontracted(n, x, sink);
      auto const u2 = time_uncontracted(n, x, sink);
      auto const c2 = time_contracted(n, x, sink);
      u_ns = u1 + u2;
      c_ns = c1 + c2;
    }
    uncontracted_sum_ns += u_ns;
    contracted_sum_ns += c_ns;
    if (first || u_ns < uncontracted_min_ns) {
      uncontracted_min_ns = u_ns;
    }
    first = false;
    if (u_ns <= 0) {
      any_zero = true;
    }
    // Each trial now covers 2N iterations (the ABBA pair).
    double const inv_n = 1.0 / (2.0 * static_cast<double>(n));
    double const u = static_cast<double>(u_ns) * inv_n;
    double const c = static_cast<double>(c_ns) * inv_n;
    uncontracted_ns_per_iter.push_back(u);
    contracted_ns_per_iter.push_back(c);
    overhead_ns_per_iter.push_back(c - u);
  }

  volatile std::int64_t live = sink;
  static_cast<void>(live);

  std::vector<double> uncontracted_sorted = uncontracted_ns_per_iter;
  std::vector<double> contracted_sorted = contracted_ns_per_iter;
  std::sort(uncontracted_sorted.begin(), uncontracted_sorted.end());
  std::sort(contracted_sorted.begin(), contracted_sorted.end());
  double const uncontracted_n50 = nearest_rank(uncontracted_sorted, 50);
  double const contracted_n50 = nearest_rank(contracted_sorted, 50);
  // Predicted-taken branch cost is often inside clock noise; treat
  // contracted as >= baseline if it is within 10% of uncontracted n50.
  double const baseline_floor = uncontracted_n50 * 0.9;

  char const* reason = "ok";
  int valid = 1;
  if (any_zero || uncontracted_sum_ns <= 0) {
    reason = "uncontracted baseline is zero";
    valid = 0;
  } else if (uncontracted_min_ns < k_min_trial_ns) {
    reason = "uncontracted trial below 1ms credibility floor";
    valid = 0;
  } else if (contracted_sum_ns < 0 || contracted_n50 < baseline_floor) {
    reason = "contracted loop faster than uncontracted baseline";
    valid = 0;
  }

  std::printf("n=%d\n", n);
  std::printf("trials=%d\n", trials);
  std::printf("uncontracted_sum_ns=%lld\n",
              static_cast<long long>(uncontracted_sum_ns));
  std::printf("contracted_sum_ns=%lld\n",
              static_cast<long long>(contracted_sum_ns));
  print_dist("uncontracted_ns_per_iter", uncontracted_ns_per_iter);
  print_dist("contracted_ns_per_iter", contracted_ns_per_iter);
  print_dist("overhead_ns_per_iter", overhead_ns_per_iter);
  print_csv("raw_uncontracted_ns_per_iter", uncontracted_ns_per_iter);
  print_csv("raw_contracted_ns_per_iter", contracted_ns_per_iter);
  print_csv("raw_overhead_ns_per_iter", overhead_ns_per_iter);
  std::printf("valid=%d\n", valid);
  std::printf("reason=%s\n", reason);

  return valid == 1 ? 0 : 1;
}

// NOLINTEND
