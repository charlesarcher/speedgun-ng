// ============================================================================
// T060, T008, T009: the spectrum kernel of specs/043-fwq-ftq, driven as the
// pure function over a sample span the plan states (US3, module M-E). The
// transform names the strongest periods of a scripted span, keeps a bin
// whose power stands above three times the median of bins 1 through N/2,
// reads at most 65,536 points and names the skipped count (FR-018, SC-002,
// D-5, R-13). The resample turns a scripted FWQ unit-time series into the
// transform input at the median unit time (FR-017, R-12). The kernel reads
// no probe, no seam, no provider and no clock, so every row scripts a span
// and reads the period list back (FR-025). The tone cases stand at or below
// 4,096 points and the suite carries one cap case, the budget R-6 measures
// at the cap. Frameworkless check()/fail() convention, the way
// harness_json_schema_test.cpp carries it.
// ============================================================================

#include <cstdio>

#include "detail/json_fixture.hpp"

using sg::test::check;

auto main() -> int
{
  std::printf(sg::test::failures == 0 ? "ok\n" : "%d checks failed\n",
              sg::test::failures);
  return sg::test::failures == 0 ? 0 : 1;
}
