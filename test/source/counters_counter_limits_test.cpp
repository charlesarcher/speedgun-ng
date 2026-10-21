// ============================================================================
// The leaf-0AH core decode decision table (specs/024 T009; FR-005, FR-032,
// research D-01, D-04).
//
// `intelCoreLimit` is a pure function of the leaf 0AH words, so the table
// drives every arm of its decision with registers no host has to provide:
// the zeroed words of a leaf `__get_cpuid_count` left unanswered, version
// 0, count 0, and the measured `EAX=0x07300404` that answers 4 (version 4,
// count 4 on `corsairone`).
//
// The refusal arms matter as much as the count: a published zero limit
// would empty every pass, and `usableCounters` publishes a zero core
// answer as no value (data-model E-02). The probe zero-initializes every
// word before its call and an unanswered leaf leaves its registers
// untouched, so the decode reads version 0 on a host without the leaf and
// needs no maximum-leaf guard of its own (research D-04). No case here
// executes CPUID, opens a device, or needs a privilege, which is what
// keeps the decode at full branch coverage on a host that reproduces none
// of the refusals in hardware (FR-032).
//
// The decode lives in `source/counters/linux_pmu/counter_limits.hpp`,
// reached through the relative include of a private header the way
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

// One row: one synthetic leaf 0AH `EAX` and the answer the decode owes it.
// The words stand in for what the probe stores after its calls, answered
// leaves included (research D-04).
struct Row
{
  std::uint32_t eax = 0;
  std::optional<std::size_t> expected;
  const char* why = nullptr;
};

// The registers of a row whose answer disagrees, so a red table says which
// nibble of which register the decode read wrong.
auto describe(const Row& row, const std::optional<std::size_t>& answer) -> void
{
  std::fprintf(
      stderr,
      "  eax=0x%08X: expected has=%d value=%zu, answered has=%d " "value=%zu\n",
      row.eax,
      int(row.expected.has_value()),
      row.expected.value_or(0),
      int(answer.has_value()),
      answer.value_or(0));
}

// The scenario: the decision table of FR-005, arm by arm.
auto decodeTableScenario() -> void
{
  static constexpr Row kRows[] = {
      {0x00000000U,
       std::nullopt,
       "the zeroed words of an unanswered leaf answer nothing (D-04)"},
      {0x07300400U, std::nullopt, "version 0 answers nothing (FR-005, D-01)"},
      {0x07300004U,
       std::size_t {0},
       "count 0 answers zero, which publishes as no value (D-01, E-02)"},
      {0x07300404U, std::size_t {4}, "EAX=0x07300404 answers 4 (FR-005, D-01)"},
      {0x07300804U, std::size_t {8}, "the answer reads EAX[15:8] (D-01)"},
  };

  for (const auto& row : kRows) {
    sg::counters::detail::CounterLimitRegisters registers {};
    registers.perfMonEax = row.eax;
    const auto answer = sg::counters::detail::intelCoreLimit(registers);
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
