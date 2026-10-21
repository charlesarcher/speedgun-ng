#ifndef SG_TEST_DETAIL_PASS_FIXTURE_HPP
#define SG_TEST_DETAIL_PASS_FIXTURE_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "speedgun-ng/counters_fake.hpp"
#include "speedgun-ng/counters_system.hpp"

// ============================================================================
// The scripted fake the multi-pass tests share (specs/024-multipass-collection
// T008; FR-006, FR-033, SC-09; data-model E-02, E-03, E-06).
//
// The shapes are the ones the pass tasks drive, and nothing else:
//
//   * the `cpu` object with a programmable-counter limit the test sets (E-02),
//   * four metric columns of two PMU events each, so a limit of 4 puts the
//     first two pairs in pass 0 and the last two in pass 1 (E-03 rule 3),
//   * one column of five events, over that limit, which stands alone in its
//     own pass (E-03 rule 4),
//   * a second scripted source with a limit of its own beside a source with no
//     scripted limit, whose leaves are costed against nothing and ride every
//     pass whose columns need them (E-03 rule 5, the two-provider edge),
//   * the clock object's two fixed time leaves, which belong to a source that
//     publishes no limit and consume no programmable counter (E-03 rule 6),
//   * the per-pass reference time sets of the spread-fixture table of data
//     model E-06, the one table every spread test cites.
//
// Every scripted fact is laid down before the provider registers, and the
// provider registers before the first resolution opens the tree: a fact
// scripted after the open is silently skipped and the script would test
// nothing, the trap `registerMetricHost` names for the same reason.
//
// The scripted leaves answer only their own names, so a host that publishes a
// real PMU catalog answers none of them. The cpu path is the host device's own
// name on purpose: the collision makes the harness's own registration of the
// real pmu provider refuse for the run (source/harness/cli.cpp:289 discards
// that registration), so a pass test never reads a real counter (FR-032).
// ============================================================================

namespace sg::test
{

// The core counting source, the object whose limit the test sets.
inline constexpr std::string_view kPassCpuObject {"cpu"};

// The second scripted source, whose published limit is its own: the
// two-provider shape of E-02's last validation rule. No host publishes it.
inline constexpr std::string_view kPassSecondObject {"cpu_second"};

// A source scripted no limit at all. Its leaves are costed against nothing and
// ride every pass whose columns need them (E-03 rule 5, FR-005's edge).
inline constexpr std::string_view kPassRideObject {"uncore_pass_77"};

// The clock object and its two fixed time leaves, which every pass rides.
inline constexpr std::string_view kPassClockObject {"machine"};
inline constexpr std::string_view kPassRealTimeLeaf {"monotonic"};
inline constexpr std::string_view kPassCpuTimeLeaf {"thread_cpu"};

// The limits the pass tasks drive: one per pass at the narrowest, the count
// the development host publishes (CPUID leaf 0AH reads version 4, count 4), and
// a count that holds all four pairs at once.
inline constexpr std::size_t kNarrowPassLimit = 1;
inline constexpr std::size_t kHostPassLimit = 4;
inline constexpr std::size_t kWidePassLimit = 8;

// The second source's own limit, deliberately different from the cpu object's,
// so a two-provider run reads two answers.
inline constexpr std::size_t kSecondSourceLimit = 2;

// Column k's scripted leaves step by (k + 1) times this per sampling action, so
// the fold of one iteration names the column and how many leaves it carries:
// the pair of index 0 folds 2000, the pair of index 3 folds 8000, the
// five-event column of index 4 folds 25000.
inline constexpr std::uint64_t kPassEventStep = 1000;

// The step the clock leaves carry until a spread test re-scripts them with the
// row of `passReferenceTimes` it runs; `setPoints` rewinds the leaf, so this
// default stands outside that script.
inline constexpr std::uint64_t kPassTimeStep = 100;

// One metric column of the fixture: the label the report prints, the source its
// leaves live on, and the leaf names in the order the planner costs them.
struct PassColumn
{
  std::string label;
  std::string_view object;
  std::vector<std::string_view> events;
};

// The four columns of two PMU events each and the one column of five, in
// request order and all on `kPassCpuObject` (E-03 rule 1). At
// `kHostPassLimit` the four pairs fill two passes of two columns each and the
// five-event column stands alone.
[[nodiscard]] inline auto passColumns() -> std::vector<PassColumn>
{
  return {
      PassColumn {"pair_a", kPassCpuObject, {"pass_a0", "pass_a1"}},
      PassColumn {"pair_b", kPassCpuObject, {"pass_b0", "pass_b1"}},
      PassColumn {"pair_c", kPassCpuObject, {"pass_c0", "pass_c1"}},
      PassColumn {"pair_d", kPassCpuObject, {"pass_d0", "pass_d1"}},
      PassColumn {"wide",
                  kPassCpuObject,
                  {"pass_w0", "pass_w1", "pass_w2", "pass_w3", "pass_w4"}},
  };
}

// The columns of the two other sources: one pair costed against
// `kSecondSourceLimit`, one pair costed against nothing. A test that wants a
// column straddling two sources composes one out of these names and
// `passColumns()`.
[[nodiscard]] inline auto passRideColumns() -> std::vector<PassColumn>
{
  return {
      PassColumn {"second", kPassSecondObject, {"pass_s0", "pass_s1"}},
      PassColumn {"ride", kPassRideObject, {"pass_r0", "pass_r1"}},
  };
}

// Lay the whole fixture script on a caller-owned provider: the three sources,
// every leaf of the columns above, and the two clock leaves. `kPassCpuObject`
// publishes `cpuLimit` and `kPassSecondObject` publishes `kSecondSourceLimit`;
// `kPassRideObject` and the clock object are scripted no limit on purpose,
// because the shape under test is the source that cannot answer (FR-005).
//
// The limit setter is the contract surface of
// specs/024-multipass-collection/contracts/programmable-counters-fact.md, which
// T015 lands; like every other scripted fact it has to precede the open, and
// the `\pre limit > 0` of that contract holds because both callers pass a
// positive constant.
inline auto scriptPassFixture(sg::counters::FakeProvider& provider,
                              const std::size_t cpuLimit = kHostPassLimit)
    -> void
{
  provider.addObject(kPassCpuObject, "pmu", "core source of the pass fixture");
  provider.addObject(
      kPassSecondObject, "pmu", "second source of the pass fixture");
  provider.addObject(kPassRideObject, "pmu", "unlimit source of the fixture");
  provider.setProgrammableCounters(kPassCpuObject, cpuLimit);
  provider.setProgrammableCounters(kPassSecondObject, kSecondSourceLimit);
  provider.addCounter(
      kPassClockObject, kPassRealTimeLeaf, "nanoseconds", "fixture clock");
  provider.addCounter(
      kPassClockObject, kPassCpuTimeLeaf, "nanoseconds", "fixture cpu clock");
  provider.setPoints(kPassClockObject, kPassRealTimeLeaf, {0}, kPassTimeStep);
  provider.setPoints(kPassClockObject, kPassCpuTimeLeaf, {0}, kPassTimeStep);

  auto columns = passColumns();
  const auto ride = passRideColumns();
  columns.insert(columns.end(), ride.begin(), ride.end());
  for (std::size_t index = 0; index < columns.size(); ++index) {
    const std::uint64_t step = (index + 1) * kPassEventStep;
    for (const auto& event : columns[index].events) {
      provider.addCounter(columns[index].object, event, "none", "fixture leaf");
      provider.setPoints(columns[index].object, event, {0}, step);
    }
  }
}

// Register the fixture with the process's own counters system, which is how
// every harness test of this suite opens its tree. A scenario that needs a
// limit other than `cpuLimit` runs in a re-exec of its binary the way
// `harness_threads_test.cpp` drives its scenarios, because the local system
// takes these object paths once per process; the provider this function builds
// is not reachable afterwards, so a scenario that needs to re-script a leaf
// owns its provider and calls `scriptPassFixture` on it directly.
//
// The answer is false when something already published one of these paths; the
// caller names that reason and skips, as every consumer of `registerMetricHost`
// does.
[[nodiscard]] inline auto registerPassFixture(
    const std::size_t cpuLimit = kHostPassLimit) -> bool
{
  static const bool kAccepted = [&cpuLimit]
  {
    auto provider = std::make_unique<sg::counters::FakeProvider>();
    scriptPassFixture(*provider, cpuLimit);
    const auto settled =
        sg::counters::System::local().registerProvider(std::move(provider));
    return settled.has_value();
  }();
  return kAccepted;
}

// The rows of the spread-fixture table of data-model.md E-06, the one table
// every spread test and document cites. Each row is one pass's merged reference
// time per iteration, in nanoseconds. At `--iterations=1` on one context a
// window folds one step of the monotonic leaf, so the row's figure is that
// leaf's scripted step for that pass.
//
//   shape         pass times       spread           warning
//   IN_BAND       100, 102, 104    3.9 percent      no
//   BOUNDARY      100, 100, 105    exactly 5.0      no, the predicate is strict
//   WARNING       100, 104, 108    7.7 percent      yes
//   TWO_PASS      100, 108         8.0 percent      yes, even-count median
//   STEEP         100, 108, 112    11.1 percent     yes
//   EQUAL         100, 100, 100    0.0 percent      no
//   ZERO_MEDIAN   0, 0, 0          no value         no, prints `spread=not
//                                                   available`
enum class PassTimeShape : std::uint8_t
{
  IN_BAND,
  BOUNDARY,
  WARNING,
  TWO_PASS,
  STEEP,
  EQUAL,
  ZERO_MEDIAN,
};

// The per-pass reference times of one row, in pass order. The spread and the
// warning are what the tests compute from these and never values mirrored here:
// a mirrored answer would keep passing while the function under it is wrong.
[[nodiscard]] inline auto passReferenceTimes(const PassTimeShape shape)
    -> std::vector<std::uint64_t>
{
  if (shape == PassTimeShape::IN_BAND) {
    return {100, 102, 104};
  }
  if (shape == PassTimeShape::BOUNDARY) {
    return {100, 100, 105};
  }
  if (shape == PassTimeShape::WARNING) {
    return {100, 104, 108};
  }
  if (shape == PassTimeShape::TWO_PASS) {
    return {100, 108};
  }
  if (shape == PassTimeShape::STEEP) {
    return {100, 108, 112};
  }
  if (shape == PassTimeShape::EQUAL) {
    return {100, 100, 100};
  }
  return {0, 0, 0};
}

// The one-pass per-iteration provider read count at this feature's audit point,
// the one number the timed-path tasks compare against (SC-09). It is read from
// the no-allocation check of `test/source/harness_capture_test.cpp:423-440` at
// 7a75e95, not derived: with the allocation-counting `operator new` of that
// file live, the difference between a 10,000-run call and a 9,999-run call at
// `--iterations=1` is two `readActions` on the scripted provider ("one measured
// run is two sampling actions over 10,000 runs"), and a 10,000-run call costs
// exactly the same per run, so the timed loop of a one-pass run adds none. The
// fake counts one action per `readPoints` call whatever the leaf count
// (`source/counters/fake_provider.cpp:98`), so two endpoint samples per window
// is two reads per iteration for a metric run of any width.
inline constexpr std::uint64_t kAuditPointReadsPerIteration = 2;

}  // namespace sg::test

#endif  // SG_TEST_DETAIL_PASS_FIXTURE_HPP
