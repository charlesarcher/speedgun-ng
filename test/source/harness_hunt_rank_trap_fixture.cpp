// ============================================================================
// The ranker's invariant trap fixture (specs/032 FR-048, review B 20): one
// address whose every window carries no anchor, so no rate stands, beside no
// refusal kind. The invariant of `rankEvents` aborts the process over it
// (specs/029 AC-10, the corsairone defect record). This executable is NOT a
// test; it is a driven fixture the way `counters_trap_fixture.cpp` is, and
// `harness_hunt_rank_trap_test.cpp` asserts the abort.
// ============================================================================

#include <cstdint>
#include <cstdio>
#include <utility>
#include <vector>

#include "detail/hunt_fixture.hpp"
#include "speedgun-ng/hunt.hpp"

namespace
{

// One window of the trap address: an event value the rank reads, and neither
// anchor, so `rateOf` gives no rate and the column stands unrated.
auto window(const std::uint64_t eventValue,
            const std::uint64_t iterations) -> sg::hunt::HuntSample
{
  auto row = sg::hunt::HuntSample {};
  row.pass = 0;
  row.stage = 1;
  row.eventValue = eventValue;
  row.anchorInstr = std::nullopt;
  row.anchorCycle = std::nullopt;
  row.iterations = iterations;
  return row;
}

}  // namespace

auto main() -> int
{
  sg::hunt::EventSamples collected;
  collected.address =
      sg::test::huntAddress(sg::test::kHuntCpu, sg::test::huntEvent(0));
  collected.samples.push_back(window(1, 1));
  const sg::hunt::HuntOptions options;
  const auto entry =
      sg::hunt::rankEvents({std::move(collected)}, options, std::nullopt);
  static_cast<void>(entry);
  std::printf("hunt-rank-trap-survived\n");
  return 0;
}
