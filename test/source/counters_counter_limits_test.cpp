// ============================================================================
// The leaf-0AH decode decision table (specs/024 T009; FR-005, FR-032,
// research D-01, D-04).
//
// `programmableCountersFromCpuid` is a pure function of three register
// values, so the table drives every arm of its decision with registers no
// host has to provide: a maximum leaf below 0AH, a maximum leaf of 0, version
// 0, count 0, the measured `EAX=0x07300404` that answers 4 (version 4, count
// 4 on `corsairone`), and an `EBX` event vector that changes nothing.
//
// `maxLeaf` decides first: a CPUID above the maximum leaf returns the maximum
// leaf's registers, whose version nibble is vendor-string text, so the raw
// registers mean nothing until the leaf vouches for them (research D-04). The
// refusal arms matter as much as the count: a published zero limit would
// empty every pass, so version 0 and count 0 answer nothing (data-model
// E-02). No case here executes CPUID, opens a device, or needs a privilege,
// which is what keeps the decode at full branch coverage on a host that
// reproduces none of the refusals in hardware (FR-032).
//
// The decode lives in `source/counters/linux_pmu/counter_limits.hpp`, reached
// through the relative include of a private header the way
// `counters_metric_lookup_test.cpp` reaches `linux_pmu/metric_tables.hpp`.
// Frameworkless check()/fail() convention.
// ============================================================================

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <optional>

#include "../source/counters/linux_pmu/counter_limits.hpp"

namespace
{

[[noreturn]] auto fail(const char* what) -> void
{
  std::fprintf(stderr, "COUNTERS COUNTER LIMITS TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool condition, const char* what) -> void
{
  if (!condition) {
    fail(what);
  }
}

// One row: three synthetic registers and the answer the decode owes them.
// `ebx` is the event vector; the decode ignores it by decision, and the rows
// carry a nonzero vector so the omission stays a decision (research D-04).
struct Row
{
  std::uint32_t maxLeaf;
  std::uint32_t eax;
  std::uint32_t ebx;
  std::optional<std::size_t> expected;
  const char* why;
};

// The registers of a row whose answer disagrees, so a red table says which
// nibble of which register the decode read wrong.
auto describe(const Row& row, const std::optional<std::size_t>& answer) -> void
{
  std::fprintf(stderr,
               "  maxLeaf=0x%08X eax=0x%08X ebx=0x%08X: expected has=%d "
               "value=%zu, answered has=%d value=%zu\n",
               row.maxLeaf,
               row.eax,
               row.ebx,
               int(row.expected.has_value()),
               row.expected.value_or(0),
               int(answer.has_value()),
               answer.value_or(0));
}

// The scenario: the decision table of FR-005, arm by arm.
auto decodeTableScenario() -> void
{
  static constexpr Row kRows[] = {
      {0x09U,
       0x07300404U,
       0x00000000U,
       std::nullopt,
       "a maximum leaf below 0AH answers nothing (FR-005, D-04)"},
      {0x00U,
       0x07300404U,
       0x00000000U,
       std::nullopt,
       "a maximum leaf of 0 answers nothing (FR-005, D-04)"},
      {0x0AU,
       0x07300400U,
       0x00000000U,
       std::nullopt,
       "version 0 answers nothing (FR-005, D-01)"},
      {0x0AU,
       0x07300004U,
       0x00000000U,
       std::nullopt,
       "count 0 answers nothing (FR-005, D-01)"},
      {0x0AU,
       0x07300404U,
       0x00000000U,
       std::size_t {4},
       "EAX=0x07300404 answers 4 (FR-005, D-01)"},
      {0x0BU,
       0x07300404U,
       0x00000000U,
       std::size_t {4},
       "a maximum leaf above 0AH answers too (FR-005, D-04)"},
      {0x0AU,
       0x07300804U,
       0x00000000U,
       std::size_t {8},
       "the answer is EAX[15:8], not the version nibble (D-01)"},
      {0x0AU,
       0x07300404U,
       0x00000FBBU,
       std::size_t {4},
       "a nonzero EBX vector leaves the answer unchanged (D-04)"},
      {0x0AU,
       0x07300400U,
       0xFFFFFFFFU,
       std::nullopt,
       "an all-ones EBX vector changes nothing (D-04)"},
  };

  for (const auto& row : kRows) {
    const auto answer = sg::counters::detail::programmableCountersFromCpuid(
        row.maxLeaf, row.eax, row.ebx);
    if (answer != row.expected) {
      describe(row, answer);
    }
    check(answer == row.expected, row.why);
  }
}

}  // namespace

auto main() -> int
{
  decodeTableScenario();
  std::printf("counters_counter_limits_test: ok\n");
  return 0;
}
