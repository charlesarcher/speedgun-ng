// ============================================================================
// The two counter sources the `hwlat` method reads
// (specs/048-hwlat-probe T007, T008, T009): the vendor gate of
// `proc/cpuinfo`, the register node `dev/cpu/<cpu>/msr` with its
// system-management-interrupt counter, and the `NMI` count of one cpu in
// the `proc/interrupts` text (FR-007, FR-009, FR-010, FR-011, D-3, D-4,
// P-1, P-2). This unit owns the only kernel interface reads of the
// feature. No path here loads a module, changes a module parameter, or
// opens a node for write (FR-008, D-3).
// ============================================================================

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "detail/latency_counters.hpp"

#include "speedgun-ng/dbc.hpp"

namespace sg::detail
{

namespace
{

// The one vendor text whose part documents the counter (FR-010, D-4, P-4).
// The gate names no other vendor and no second counter, so a part that
// states none gets no substitute (D-4, FR-008).
[[maybe_unused]] constexpr std::string_view kSmiVendorId = "GenuineIntel";

}  // namespace

auto smiSourceSupported() -> bool
{
  // T008 lands the gate: the `vendor_id` text the seam answers decides
  // the answer, and an unreadable file decides unsupported (FR-010, P-4).
  return false;
}

auto openMsrNode(const std::uint32_t cpu) -> std::optional<int>
{
  // T007 lands the pair: one read-only `::open` of the node of `cpu`
  // under the file root (FR-007, D-3, P-2).
  static_cast<void>(cpu);
  return std::nullopt;
}

auto closeMsrNode(const int descriptor) -> void
{
  SG_REQUIRE(descriptor >= 0, "closeMsrNode reaches a descriptor that is none");
  // T007 lands the second half of the pair: one `::close` call (P-2).
}

auto smiCountOf(const int descriptor) -> std::optional<std::uint64_t>
{
  SG_REQUIRE(descriptor >= 0, "smiCountOf reaches a descriptor that is none");
  // T007 lands the read: one `::pread` of the counter width at the
  // counter address (FR-007, D-3, P-2).
  return std::nullopt;
}

auto nmiCountOfText(const std::string& text,
                    const std::uint32_t cpu) -> std::optional<std::uint64_t>
{
  SG_REQUIRE(text.find('\0') == std::string::npos,
             "nmiCountOfText reaches a text that is not one whole text");
  static_cast<void>(cpu);
  // T008 lands the parse: the `NMI` row, the column of `cpu`, one figure
  // settled by std::from_chars (FR-009, P-4).
  return std::nullopt;
}

}  // namespace sg::detail
