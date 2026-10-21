// ============================================================================
// The CPUID and watchdog reads of the programmable-counter limit and their
// pure decodes (specs/024-multipass-collection FR-005, FR-032; research
// D-01, D-04).
//
// The probes hold no decision and the decodes hold no instruction, which is
// what keeps the hardware-dependent half branch-free and the rule half
// drivable from a table (research D-04, FR-032). See the header for the
// contract each piece keeps.
// ============================================================================

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>

#include "counter_limits.hpp"

#if defined(__x86_64__) || defined(__i386__)
#  include <cpuid.h>
#endif

namespace sg::counters::detail
{
namespace
{

/// CPUID leaf 0AH, the architectural performance-monitoring discovery leaf.
constexpr std::uint32_t kProgrammableLeaf = 0x0AU;

/// CPUID leaf 0x23, the unified performance-monitoring leaf: a host that
/// answers it is a hybrid-capable layout whose core figure would need one
/// value per core type (research D-01's hybrid rule).
constexpr std::uint32_t kUnifiedLeaf = 0x23U;

/// CPUID leaf 0x80000000, whose `EAX` is the highest extended leaf.
constexpr std::uint32_t kMaxExtendedLeaf = 0x80000000U;

/// CPUID leaf 0x80000001, whose `ECX` carries `PerfCtrExtCore`.
constexpr std::uint32_t kExtendedFeaturesLeaf = 0x80000001U;

/// CPUID leaf 0x80000022, AMD's extended performance-monitoring leaf.
constexpr std::uint32_t kAmdPerfMonLeaf = 0x80000022U;

/// The width of one `EAX` field of leaf 0AH: the version sits in `EAX[7:0]`
/// and the counter count in `EAX[15:8]` (research D-01).
constexpr std::uint32_t kFieldMask = 0xFFU;

/// The shift from `EAX[7:0]` to `EAX[15:8]`: the count of general-purpose
/// counters that support programmable events (research D-01).
constexpr unsigned int kCountShift = 8U;

/// The shift from `EAX[7:0]` to the version's comparator floor: a fixed
/// cycles counter exists from version 2 (research D-01).
constexpr std::uint32_t kFixedCyclesVersion = 2U;

/// `EDX[4:0]` of leaf 0AH: the number of architectural fixed counters;
/// at least 2 means the fixed cycles counter is among them.
constexpr std::uint32_t kFixedCountMask = 0x1FU;

/// The fixed-counter floor that names a fixed cycles counter alongside the
/// fixed instructions counter (research D-01).
constexpr std::uint32_t kFixedCountFloor = 2U;

/// `0x80000022 EAX` bit 0: `PerfMonV2`, AMD's core-counter count in
/// `EBX[3:0]` (research D-01's AMD rule).
constexpr std::uint32_t kPerfMonV2Bit = 1U;

/// `0x80000001 ECX` bit 23: `PerfCtrExtCore`, the extended core counters
/// that answer 6 before `PerfMonV2` existed (research D-01's AMD rule).
constexpr std::uint32_t kPerfCtrExtCoreBit = 1U << 23U;

/// `EBX[3:0]` of leaf 0x80000022: the core PMU counter count under
/// `PerfMonV2` (research D-01's AMD rule).
constexpr std::uint32_t kLowNibbleMask = 0xFU;

/// The core PMU count of a `PerfCtrExtCore` part without `PerfMonV2`.
constexpr std::size_t kExtendedCoreCount = 6U;

/// The core PMU count of a pre-extension AMD part.
constexpr std::size_t kLegacyCoreCount = 4U;

/// The 12 vendor characters of one name as the three leaf 0 words, in the
/// order `EBX`, `EDX`, `ECX`. Compile-time only: `consteval` keeps the fold
/// out of the runtime binary.
[[nodiscard]] consteval auto vendorWords(std::string_view name) noexcept
    -> std::array<std::uint32_t, 3>
{
  std::array<std::uint32_t, 3> words {};
  for (std::size_t index = 0U; index < name.size(); ++index) {
    words[index / 4U] |= std::uint32_t {static_cast<unsigned char>(name[index])}
        << (8U * (index % 4U));
  }
  return words;
}

constexpr auto kIntelWords = vendorWords("GenuineIntel");
constexpr auto kCentaurWords = vendorWords("CentaurHauls");
constexpr auto kShanghaiWords = vendorWords("Shanghai");
constexpr auto kAmdWords = vendorWords("AuthenticAMD");
constexpr auto kHygonWords = vendorWords("HygonGenuine");

/// Whether the three vendor words spell the named vendor's string.
[[nodiscard]] auto spellsWords(
    const CounterLimitRegisters& registers,
    const std::array<std::uint32_t, 3>& words) noexcept -> bool
{
  return registers.vendorEbx == words[0] && registers.vendorEdx == words[1]
      && registers.vendorEcx == words[2];
}

/// The count with the watchdog's counter taken away; the subtraction that
/// reaches zero answers no value (research D-01, FR-005). The caller has
/// already settled that its argument holds a nonzero count.
[[nodiscard]] auto publishMinusOne(const std::size_t hardware) noexcept
    -> std::optional<std::size_t>
{
  if (hardware <= 1U) {
    return std::nullopt;
  }
  return hardware - 1U;
}

}  // namespace

auto readCounterLimitRegisters() noexcept -> CounterLimitRegisters
{
  CounterLimitRegisters registers {};
#if defined(__x86_64__) || defined(__i386__)
  // Straight-line by decision. `__get_cpuid_count` answers false without
  // executing the leaf when it exceeds the answered maximum, so a fresh
  // zero-init before each call makes an absent leaf read as zeros and the
  // probe keeps no guard whose other side no CI host can reach (research
  // D-04, FR-032). The sub-leaf operand is zero except for leaf 0x23.
  {
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    (void)__get_cpuid_count(0x00U, 0U, &eax, &ebx, &ecx, &edx);
    registers.maxLeaf = eax;
    registers.vendorEbx = ebx;
    registers.vendorEdx = edx;
    registers.vendorEcx = ecx;
  }
  {
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    (void)__get_cpuid_count(kProgrammableLeaf, 0U, &eax, &ebx, &ecx, &edx);
    registers.perfMonEax = eax;
    registers.perfMonEdx = edx;
  }
  {
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    (void)__get_cpuid_count(kUnifiedLeaf, 0U, &eax, &ebx, &ecx, &edx);
    registers.unified0Eax = eax;
  }
  {
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    (void)__get_cpuid_count(kUnifiedLeaf, 1U, &eax, &ebx, &ecx, &edx);
    registers.unified1Eax = eax;
  }
  {
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    (void)__get_cpuid_count(kMaxExtendedLeaf, 0U, &eax, &ebx, &ecx, &edx);
    registers.maxExtended = eax;
  }
  {
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    (void)__get_cpuid_count(kExtendedFeaturesLeaf, 0U, &eax, &ebx, &ecx, &edx);
    registers.extendedEcx = ecx;
  }
  {
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    (void)__get_cpuid_count(kAmdPerfMonLeaf, 0U, &eax, &ebx, &ecx, &edx);
    registers.amdPerfEax = eax;
    registers.amdPerfEbx = ebx;
  }
#endif
  return registers;
}

auto vendorOf(const CounterLimitRegisters& registers) noexcept
    -> CounterLimitVendor
{
  if (spellsWords(registers, kIntelWords)) {
    return CounterLimitVendor::INTEL;
  }
  if (spellsWords(registers, kCentaurWords)
      || spellsWords(registers, kShanghaiWords))
  {
    return CounterLimitVendor::ZHAOXIN;
  }
  if (spellsWords(registers, kAmdWords) || spellsWords(registers, kHygonWords))
  {
    return CounterLimitVendor::AMD;
  }
  return CounterLimitVendor::OTHER;
}

auto intelCoreLimit(const CounterLimitRegisters& registers) noexcept
    -> std::optional<std::size_t>
{
  if ((registers.perfMonEax & kFieldMask) == 0U) {
    return std::nullopt;
  }
  return static_cast<std::size_t>((registers.perfMonEax >> kCountShift)
                                  & kFieldMask);
}

auto amdCoreLimit(const CounterLimitRegisters& registers) noexcept
    -> std::optional<std::size_t>
{
  // The extended leaf answers garbage when the host does not carry it:
  // `__get_cpuid_count` answered false and the words stayed zero, and an
  // older AMD part's `0x80000022` read is the maximum leaf's data, so the
  // gate on `maxExtended` decides before the bits do (research D-01).
  if (registers.maxExtended >= kAmdPerfMonLeaf
      && (registers.amdPerfEax & kPerfMonV2Bit) != 0U)
  {
    return static_cast<std::size_t>(registers.amdPerfEbx & kLowNibbleMask);
  }
  if ((registers.extendedEcx & kPerfCtrExtCoreBit) != 0U) {
    return kExtendedCoreCount;
  }
  return kLegacyCoreCount;
}

auto fixedCycles(const CounterLimitRegisters& registers) noexcept -> bool
{
  return (registers.perfMonEax & kFieldMask) >= kFixedCyclesVersion
      && (registers.perfMonEdx & kFixedCountMask) >= kFixedCountFloor;
}

auto watchdogValueFrom(std::string_view text) noexcept -> int
{
  int value = 0;
  const auto parsed =
      std::from_chars(text.data(), text.data() + text.size(), value);
  if (parsed.ec != std::errc {}) {
    return -1;
  }
  return value;
}

auto readWatchdogValue() noexcept -> int
{
  std::ifstream file("/proc/sys/kernel/nmi_watchdog");
  std::string text;
  // A failed open needs no branch: the line stays empty and the parse
  // answers -1, the watchdog value of a host the read could not reach.
  std::getline(file, text);
  return watchdogValueFrom(text);
}

auto isHybrid(std::span<const std::string_view> deviceNames) -> bool
{
  return std::ranges::any_of(deviceNames,
                             [](const std::string_view name) noexcept {
                               return name == "cpu_core" || name == "cpu_atom";
                             });
}

auto limitForDevice(const std::string_view deviceName,
                    const CounterLimitVendor vendor,
                    const bool hybrid,
                    const CounterLimitRegisters& registers)
    -> std::optional<std::size_t>
{
  const bool coreDevice = deviceName == "cpu" || deviceName == "cpu_core"
      || deviceName == "cpu_atom";
  if (!coreDevice) {
    return std::nullopt;
  }
  // Leaf 0x23 answers only on hybrid-capable layouts, and the version
  // word of an answered sub-leaf is never zero, so the two words' OR
  // answers "this host answers leaf 0x23" with no decision of its own.
  const bool leafAnswers =
      (registers.unified0Eax | registers.unified1Eax) != 0U;
  if (hybrid && leafAnswers) {
    return std::nullopt;
  }
  switch (vendor) {
    case CounterLimitVendor::INTEL:
    case CounterLimitVendor::ZHAOXIN:
      return intelCoreLimit(registers);
    case CounterLimitVendor::AMD:
      return amdCoreLimit(registers);
    case CounterLimitVendor::OTHER:
      break;
  }
  return std::nullopt;
}

auto usableCounters(const std::optional<std::size_t> hardware,
                    const CounterLimitVendor vendor,
                    const int watchdogValue,
                    const bool hasFixedCycles) noexcept
    -> std::optional<std::size_t>
{
  // A zero hardware answer is no value entering the table, and a zero
  // result is no value leaving it (research D-01, FR-005).
  if (hardware.value_or(0U) == 0U) {
    return std::nullopt;
  }
  if (watchdogValue != 1) {
    return hardware;
  }
  switch (vendor) {
    case CounterLimitVendor::INTEL:
    case CounterLimitVendor::ZHAOXIN:
      // With a fixed cycles counter the watchdog's cycles event holds that
      // counter; without one the watchdog occupies a programmable counter.
      return hasFixedCycles ? hardware : publishMinusOne(*hardware);
    case CounterLimitVendor::AMD:
    case CounterLimitVendor::OTHER:
      // AMD has no fixed cycles counter and its watchdog pins a cycles
      // event; an unknown vendor keeps the conservative subtraction.
      return publishMinusOne(*hardware);
  }
  return std::nullopt;
}

}  // namespace sg::counters::detail
