// ============================================================================
// The counter-limit decision tables (specs/024-multipass-collection FR-005,
// FR-032; research D-01, D-04; data-model E-02).
//
// Every decoder of `source/counters/linux_pmu/counter_limits.hpp` is a pure
// function of register words, watchdog text or device names, so each rule
// row of research D-01 is a case here, driven with words no host has to
// provide. No case executes CPUID, opens a device or reads the watchdog
// file: the probes hold no decision, and the decisions all live in these
// tables, which is what keeps the seam at full branch coverage on hosts
// that reproduce none of the refusals in hardware (FR-032).
//
// Each row says which rule of D-01 it pins. The vendor table spells every
// word arm of `vendorOf` in both directions, the Intel and AMD tables hold
// the core answers and the extended-leaf gate, the watchdog and fixed
// cycles tables hold the adjustment's inputs, the `usableCounters` table
// holds the adjustment rows in first-match order including the zero that
// publishes no value, and the hybrid and device tables hold the names and
// the leaf 0x23 gate.
//
// The decode lives behind a private header, reached through the relative
// include the way `counters_metric_lookup_test.cpp` reaches
// `linux_pmu/metric_tables.hpp`. Frameworkless check()/fail() convention.
// ============================================================================

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "../source/counters/linux_pmu/counter_limits.hpp"

namespace
{

using sg::counters::detail::CounterLimitRegisters;
using sg::counters::detail::CounterLimitVendor;

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "FAIL: %s\n", what);
  std::exit(EXIT_FAILURE);
}

auto check(const bool condition, const char* what) -> void
{
  if (!condition) {
    fail(what);
  }
}

// One ASCII word of a CPUID register, little-endian as the register holds
// it: `wordOf("Genu")` is leaf 0 `EBX` on an Intel host.
constexpr auto wordOf(std::string_view text) noexcept -> std::uint32_t
{
  std::uint32_t word = 0U;
  for (std::size_t index = 0U; index < text.size(); ++index) {
    word |= std::uint32_t {static_cast<unsigned char>(text[index])}
        << (8U * index);
  }
  return word;
}

// The three vendor words of a name, in the order leaf 0 answers them:
// `EBX`, `EDX`, `ECX`. A name of fewer than 12 characters leaves the rest
// of its last word zero: the padded Shanghai identity of zhaoxin.c fills
// all twelve, the one-space refusal row fills ten.
constexpr auto wordsOf(std::string_view name) noexcept
    -> std::array<std::uint32_t, 3>
{
  return {wordOf(name.substr(0U, 4U)),
          wordOf(name.substr(4U, 4U)),
          wordOf(name.substr(8U, 4U))};
}

constexpr auto intelRegisters() noexcept -> CounterLimitRegisters
{
  CounterLimitRegisters registers {};
  registers.vendorEbx = wordOf("Genu");
  registers.vendorEdx = wordOf("ineI");
  registers.vendorEcx = wordOf("ntel");
  // The leaf 0AH answer measured on corsairone: version 4, four general
  // purpose counters, three fixed counters (research D-01).
  registers.perfMonEax = 0x07300404U;
  registers.perfMonEdx = 0x00000603U;
  return registers;
}

// ----------------------------------------------------------------------------
// The leaf-0AH core decode, arm by arm.
// ----------------------------------------------------------------------------

struct IntelRow
{
  std::uint32_t eax = 0;
  std::optional<std::size_t> expected;
  const char* why = nullptr;
};

auto describe(const IntelRow& row,
              const std::optional<std::size_t>& answer) -> void
{
  std::fprintf(
      stderr,
      "  eax=0x%08X: expected has=%d value=%zu, answered has=%d value=%zu\n",
      row.eax,
      int(row.expected.has_value()),
      row.expected.value_or(0),
      int(answer.has_value()),
      answer.value_or(0));
}

auto decodeTableScenario() -> void
{
  static constexpr IntelRow kRows[] = {
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
    CounterLimitRegisters registers {};
    registers.perfMonEax = row.eax;
    const auto answer = sg::counters::detail::intelCoreLimit(registers);
    if (answer != row.expected) {
      describe(row, answer);
    }
    check(answer == row.expected, row.why);
  }
}

// ----------------------------------------------------------------------------
// The vendor table: every word arm of `vendorOf`, matched and refused.
// ----------------------------------------------------------------------------

struct VendorRow
{
  std::array<std::uint32_t, 3> words {};
  CounterLimitVendor expected = CounterLimitVendor::OTHER;
  const char* why = nullptr;
};

auto describe(const VendorRow& row, const CounterLimitVendor answer) -> void
{
  std::fprintf(stderr,
               "  ebx=0x%08X edx=0x%08X ecx=0x%08X: answered vendor %d\n",
               row.words[0],
               row.words[1],
               row.words[2],
               int(answer));
}

auto vendorTableScenario() -> void
{
  static constexpr VendorRow kRows[] = {
      {wordsOf("GenuineIntel"),
       CounterLimitVendor::INTEL,
       "GenuineIntel answers INTEL (D-01)"},
      {wordsOf("CentaurHauls"),
       CounterLimitVendor::ZHAOXIN,
       "CentaurHauls answers ZHAOXIN (D-01)"},
      {wordsOf("  Shanghai  "),
       CounterLimitVendor::ZHAOXIN,
       "Shanghai, space padded, answers ZHAOXIN (D-01)"},
      {wordsOf("AuthenticAMD"),
       CounterLimitVendor::AMD,
       "AuthenticAMD answers AMD (D-01)"},
      {wordsOf("HygonGenuine"),
       CounterLimitVendor::AMD,
       "HygonGenuine answers AMD (D-01)"},
      {{},
       CounterLimitVendor::OTHER,
       "the zeroed words of an unanswered leaf 0 answer OTHER (D-04)"},
      {{wordOf("Genu"), wordOf("xxxx"), wordOf("ntel")},
       CounterLimitVendor::OTHER,
       "Intel first word with a refused second word answers OTHER"},
      {{wordOf("Genu"), wordOf("ineI"), wordOf("xxxx")},
       CounterLimitVendor::OTHER,
       "Intel first two words with a refused third word answer OTHER"},
      {{wordOf("Cent"), wordOf("xxxx"), wordOf("auls")},
       CounterLimitVendor::OTHER,
       "Centaur first word with a refused second word answers OTHER"},
      {{wordOf("Cent"), wordOf("aurH"), wordOf("xxxx")},
       CounterLimitVendor::OTHER,
       "Centaur first two words with a refused third word answer OTHER"},
      {{wordOf("  Sh"), wordOf("xxxx"), wordOf("xxxx")},
       CounterLimitVendor::OTHER,
       "Shanghai first word with a refused second word answers OTHER"},
      {wordsOf(" Shanghai "),
       CounterLimitVendor::OTHER,
       "One space of padding each side answers OTHER (D-01)"},
      {{wordOf("  Sh"), wordOf("angh"), wordOf("xxxx")},
       CounterLimitVendor::OTHER,
       "Shanghai first two words with a refused third word answer OTHER"},
      {{wordOf("Auth"), wordOf("xxxx"), wordOf("cAMD")},
       CounterLimitVendor::OTHER,
       "AMD first word with a refused second word answers OTHER"},
      {{wordOf("Auth"), wordOf("enti"), wordOf("xxxx")},
       CounterLimitVendor::OTHER,
       "AMD first two words with a refused third word answer OTHER"},
      {{wordOf("Hygo"), wordOf("xxxx"), wordOf("uine")},
       CounterLimitVendor::OTHER,
       "Hygon first word with a refused second word answers OTHER"},
      {{wordOf("Hygo"), wordOf("nGen"), wordOf("xxxx")},
       CounterLimitVendor::OTHER,
       "Hygon first two words with a refused third word answer OTHER"},
  };

  for (const auto& row : kRows) {
    CounterLimitRegisters registers {};
    registers.vendorEbx = row.words[0];
    registers.vendorEdx = row.words[1];
    registers.vendorEcx = row.words[2];
    const auto answer = sg::counters::detail::vendorOf(registers);
    if (answer != row.expected) {
      describe(row, answer);
    }
    check(answer == row.expected, row.why);
  }
}

// ----------------------------------------------------------------------------
// The AMD rule: the extended-leaf gate, PerfMonV2's nibble, and the two
// fallbacks.
// ----------------------------------------------------------------------------

struct AmdRow
{
  std::uint32_t maxExtended = 0;
  std::uint32_t extendedEcx = 0;
  std::uint32_t perfEax = 0;
  std::uint32_t perfEbx = 0;
  std::size_t expected = 0;
  const char* why = nullptr;
};

auto describe(const AmdRow& row,
              const std::optional<std::size_t>& answer) -> void
{
  std::fprintf(stderr,
               "  maxExt=0x%08X ecx=0x%08X eax=0x%08X ebx=0x%08X: expected "
               "%zu, answered has=%d value=%zu\n",
               row.maxExtended,
               row.extendedEcx,
               row.perfEax,
               row.perfEbx,
               row.expected,
               int(answer.has_value()),
               answer.value_or(0));
}

auto amdTableScenario() -> void
{
  constexpr std::uint32_t kPerfCtrExtCore = 1U << 23U;
  static constexpr std::uint32_t kHasLeaf = 0x80000022U;
  static constexpr AmdRow kRows[] = {
      {kHasLeaf,
       0U,
       0x00000001U,
       0x00000006U,
       6U,
       "PerfMonV2 answers EBX[3:0] (D-01)"},
      {kHasLeaf,
       0U,
       0x00000001U,
       0x0000001AU,
       0xAU,
       "the nibble mask drops the high bits of EBX (D-01)"},
      {kHasLeaf,
       kPerfCtrExtCore,
       0x00000000U,
       0x00000000U,
       6U,
       "PerfCtrExtCore without PerfMonV2 answers 6 (D-01)"},
      {kHasLeaf,
       0U,
       0x00000000U,
       0x00000000U,
       4U,
       "the pre-extension core PMU answers 4 (D-01)"},
      {0x8000001EU,
       0U,
       0x00000001U,
       0x00000006U,
       4U,
       "a max extended leaf below 0x80000022 blocks the leaf (D-01)"},
      {0x8000001EU,
       kPerfCtrExtCore,
       0x00000001U,
       0x00000006U,
       6U,
       "the blocked leaf words leave PerfCtrExtCore's 6 (D-01)"},
  };

  for (const auto& row : kRows) {
    CounterLimitRegisters registers {};
    registers.maxExtended = row.maxExtended;
    registers.extendedEcx = row.extendedEcx;
    registers.amdPerfEax = row.perfEax;
    registers.amdPerfEbx = row.perfEbx;
    const auto answer = sg::counters::detail::amdCoreLimit(registers);
    if (answer != row.expected) {
      describe(row, answer);
    }
    check(answer == row.expected, row.why);
  }
}

// ----------------------------------------------------------------------------
// The fixed cycles answer: the version floor, the fixed-counter floor and
// the EDX nibble mask.
// ----------------------------------------------------------------------------

struct FixedRow
{
  std::uint32_t eax = 0;
  std::uint32_t edx = 0;
  bool expected = false;
  const char* why = nullptr;
};

auto fixedCyclesTableScenario() -> void
{
  static constexpr FixedRow kRows[] = {
      {0x07300404U,
       0x00000603U,
       true,
       "version 4 with three fixed counters have fixed cycles (D-01)"},
      {0x07300402U,
       0x00000002U,
       true,
       "version 2 with two architectural fixed counters has one (D-01)"},
      {0x07300401U,
       0x00000603U,
       false,
       "version 1 has no fixed cycles counter (D-01)"},
      {0x07300404U,
       0x00000001U,
       false,
       "one fixed counter is below the floor (D-01)"},
      {0x07300404U, 0x00000020U, false, "the mask reads EDX[4:0] alone (D-01)"},
  };

  for (const auto& row : kRows) {
    CounterLimitRegisters registers {};
    registers.perfMonEax = row.eax;
    registers.perfMonEdx = row.edx;
    const auto answer = sg::counters::detail::fixedCycles(registers);
    if (answer != row.expected) {
      std::fprintf(stderr,
                   "  eax=0x%08X edx=0x%08X: answered %d\n",
                   row.eax,
                   row.edx,
                   int(answer));
    }
    check(answer == row.expected, row.why);
  }
}

// ----------------------------------------------------------------------------
// The watchdog text: the parsed number, and -1 for text with no number.
// ----------------------------------------------------------------------------

struct WatchdogRow
{
  std::string_view text;
  int expected = -1;
  const char* why = nullptr;
};

auto watchdogTableScenario() -> void
{
  static constexpr WatchdogRow kRows[] = {
      {"1", 1, "the enabled watchdog answers 1 (D-01's watchdog table)"},
      {"0", 0, "the released watchdog answers 0 (D-01's watchdog table)"},
      {"2", 2, "a number that is not 1 is read as its own value (D-01)"},
      {"", -1, "the empty text of a failed read answers -1 (D-01)"},
      {"on", -1, "text with no number answers -1 (D-01)"},
  };

  for (const auto& row : kRows) {
    const auto answer = sg::counters::detail::watchdogValueFrom(row.text);
    if (answer != row.expected) {
      std::fprintf(stderr,
                   "  text=\"%.*s\": answered %d\n",
                   int(row.text.size()),
                   row.text.data(),
                   answer);
    }
    check(answer == row.expected, row.why);
  }
}

// ----------------------------------------------------------------------------
// The hybrid answer: the device list, with and without the hybrid names.
// ----------------------------------------------------------------------------

struct HybridRow
{
  std::array<std::string_view, 3> names {};
  std::size_t count = 0;
  bool expected = false;
  const char* why = nullptr;
};

auto hybridTableScenario() -> void
{
  static constexpr HybridRow kRows[] = {
      {{}, 0U, false, "the empty device list is no hybrid host (D-01)"},
      {{"cpu"}, 1U, false, "the plain core device is no hybrid host (D-01)"},
      {{"cpu", "uncore_imc_0"},
       2U,
       false,
       "an uncore name is no hybrid name (D-01)"},
      {{"cpu", "cpu_core", "cpu_atom"},
       3U,
       true,
       "the hybrid core names mark a hybrid host (D-01)"},
      {{"cpu_atom"},
       1U,
       true,
       "the atom name alone marks a hybrid host (D-01)"},
  };

  for (const auto& row : kRows) {
    const std::span<const std::string_view> names(row.names.data(), row.count);
    const auto answer = sg::counters::detail::isHybrid(names);
    if (answer != row.expected) {
      std::fprintf(stderr,
                   "  first name \"%.*s\": answered %d\n",
                   row.count == 0U ? 0 : int(row.names[0].size()),
                   row.count == 0U ? "" : row.names[0].data(),
                   int(answer));
    }
    check(answer == row.expected, row.why);
  }
}

// ----------------------------------------------------------------------------
// The adjustment table, in first-match order: the watchdog row, the vendor
// rows, and the zero that publishes no value.
// ----------------------------------------------------------------------------

struct UsableRow
{
  std::optional<std::size_t> hardware;
  CounterLimitVendor vendor = CounterLimitVendor::OTHER;
  int watchdog = 0;
  bool hasFixedCycles = false;
  std::optional<std::size_t> expected;
  const char* why = nullptr;
};

auto describe(const UsableRow& row,
              const std::optional<std::size_t>& answer) -> void
{
  std::fprintf(stderr,
               "  hardware has=%d value=%zu vendor=%d watchdog=%d fixed=%d: "
               "expected has=%d value=%zu, answered has=%d value=%zu\n",
               int(row.hardware.has_value()),
               row.hardware.value_or(0),
               int(row.vendor),
               row.watchdog,
               int(row.hasFixedCycles),
               int(row.expected.has_value()),
               row.expected.value_or(0),
               int(answer.has_value()),
               answer.value_or(0));
}

auto usableTableScenario() -> void
{
  static constexpr std::size_t kFour = 4U;
  static constexpr std::size_t kOne = 1U;
  static constexpr std::size_t kThree = 3U;
  static constexpr UsableRow kRows[] = {
      {kFour,
       CounterLimitVendor::INTEL,
       0,
       true,
       kFour,
       "a watchdog value of 0 leaves the hardware answer (row 1)"},
      {kFour,
       CounterLimitVendor::INTEL,
       -1,
       true,
       kFour,
       "the failed read of -1 is not 1 either (D-01 adjustment row 1)"},
      {kFour,
       CounterLimitVendor::INTEL,
       1,
       true,
       kFour,
       "Intel with fixed cycles keeps the hardware answer (row 2)"},
      {kFour,
       CounterLimitVendor::ZHAOXIN,
       1,
       true,
       kFour,
       "Zhaoxin rides the Intel row (D-01 adjustment row 2)"},
      {kFour,
       CounterLimitVendor::INTEL,
       1,
       false,
       kThree,
       "Intel without fixed cycles gives one away (row 3)"},
      {kFour,
       CounterLimitVendor::ZHAOXIN,
       1,
       false,
       kThree,
       "Zhaoxin gives one away too (D-01 adjustment row 3)"},
      {kFour,
       CounterLimitVendor::AMD,
       1,
       false,
       kThree,
       "AMD gives one away (D-01 adjustment row 4)"},
      {kFour,
       CounterLimitVendor::OTHER,
       1,
       false,
       kThree,
       "an unknown vendor gives one away (D-01 adjustment row 5)"},
      {std::nullopt,
       CounterLimitVendor::INTEL,
       1,
       true,
       std::nullopt,
       "no hardware answer stays no value (D-01 adjustment table)"},
      {kOne,
       CounterLimitVendor::AMD,
       1,
       false,
       std::nullopt,
       "a subtraction to zero answers no value (D-01, FR-005)"},
      {std::size_t {0},
       CounterLimitVendor::OTHER,
       0,
       true,
       std::nullopt,
       "a zero hardware answer is no value entering the table (FR-005)"},
      {kFour,
       CounterLimitVendor::OTHER,
       0,
       true,
       kFour,
       "the watchdog row wins before the vendor row (first match)"},
  };

  for (const auto& row : kRows) {
    const auto answer = sg::counters::detail::usableCounters(
        row.hardware, row.vendor, row.watchdog, row.hasFixedCycles);
    if (answer != row.expected) {
      describe(row, answer);
    }
    check(answer == row.expected, row.why);
  }
}

// ----------------------------------------------------------------------------
// The device table and the hybrid rule: which device carries which answer.
// ----------------------------------------------------------------------------

struct DeviceRow
{
  std::string_view name;
  CounterLimitVendor vendor = CounterLimitVendor::OTHER;
  bool hybrid = false;
  CounterLimitRegisters registers {};
  std::optional<std::size_t> expected;
  const char* why = nullptr;
};

auto describe(const DeviceRow& row,
              const std::optional<std::size_t>& answer) -> void
{
  std::fprintf(stderr,
               "  device \"%.*s\" vendor=%d hybrid=%d: expected has=%d "
               "value=%zu, answered has=%d value=%zu\n",
               int(row.name.size()),
               row.name.data(),
               int(row.vendor),
               int(row.hybrid),
               int(row.expected.has_value()),
               row.expected.value_or(0),
               int(answer.has_value()),
               answer.value_or(0));
}

auto deviceTableScenario() -> void
{
  static constexpr std::size_t kFour = 4U;
  static constexpr std::size_t kSix = 6U;
  static constexpr DeviceRow kRows[] = {
      {"uncore_imc_0",
       CounterLimitVendor::INTEL,
       false,
       intelRegisters(),
       std::nullopt,
       "a device outside the device table publishes no limit (D-01)"},
      {"cpu",
       CounterLimitVendor::INTEL,
       false,
       intelRegisters(),
       kFour,
       "cpu answers the Intel core answer (D-01 device table)"},
      {"cpu_core",
       CounterLimitVendor::INTEL,
       false,
       intelRegisters(),
       kFour,
       "a hybrid name on a plain host keeps the core answer (D-01)"},
      {"cpu_atom",
       CounterLimitVendor::INTEL,
       true,
       intelRegisters(),
       kFour,
       "a hybrid host with a silent leaf 0x23 keeps the answer (D-01)"},
      {"cpu_core",
       CounterLimitVendor::INTEL,
       true,
       []
       {
         auto registers = intelRegisters();
         registers.unified0Eax = 0x00000001U;
         return registers;
       }(),
       std::nullopt,
       "an answered leaf 0x23 sub-leaf 0 publishes nothing (D-01)"},
      {"cpu",
       CounterLimitVendor::INTEL,
       true,
       []
       {
         auto registers = intelRegisters();
         registers.unified1Eax = 0x00000001U;
         return registers;
       }(),
       std::nullopt,
       "the answered sub-leaf 1 word answers the hybrid rule (D-01)"},
      {"cpu",
       CounterLimitVendor::ZHAOXIN,
       false,
       intelRegisters(),
       kFour,
       "Zhaoxin decodes the same leaf 0AH (D-01)"},
      {"cpu",
       CounterLimitVendor::AMD,
       false,
       []
       {
         CounterLimitRegisters registers {};
         registers.maxExtended = 0x80000022U;
         registers.amdPerfEax = 0x00000001U;
         registers.amdPerfEbx = 0x00000006U;
         return registers;
       }(),
       kSix,
       "cpu answers the AMD extended leaf (D-01 device table)"},
      {"cpu",
       CounterLimitVendor::OTHER,
       false,
       intelRegisters(),
       std::nullopt,
       "an unknown vendor publishes no limit (D-01 device table)"},
  };

  for (const auto& row : kRows) {
    const auto answer = sg::counters::detail::limitForDevice(
        row.name, row.vendor, row.hybrid, row.registers);
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
  vendorTableScenario();
  amdTableScenario();
  fixedCyclesTableScenario();
  watchdogTableScenario();
  hybridTableScenario();
  usableTableScenario();
  deviceTableScenario();
  std::printf(
      "counters_counter_limits_test PASS: every decision table row matches\n");
  return EXIT_SUCCESS;
}
