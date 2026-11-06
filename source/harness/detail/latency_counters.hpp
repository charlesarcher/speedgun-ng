#ifndef SG_HARNESS_LATENCY_COUNTERS_HPP
#define SG_HARNESS_LATENCY_COUNTERS_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace sg::detail
{

/**
 * @file latency_counters.hpp
 * @brief The two counter sources behind the `hwlat` method
 * (specs/048-hwlat-probe P-1): the system-management-interrupt counter
 * behind the `msr` node of one cpu, and the non-maskable-interrupt count
 * of one cpu in the `NMI` row of `proc/interrupts`. The vendor gate
 * stands ahead of the counter, because a part has to document it
 * (FR-010, D-4). No other unit opens a kernel interface directly (P-1).
 * Every path resolves under the file-root seam (FR-001, D-3). Nothing
 * here is public API.
 */

/// @brief The address of the system-management-interrupt counter inside
/// the register node, the one address spec D-3 names (FR-007).
inline constexpr std::uint64_t kSmiCountAddress = 0x34;

/// @brief The width of one counter figure: the node answers the count as
/// one little-endian unsigned value of this many bytes, and a read that
/// answers fewer bytes stands unparsable (FR-007, FR-011).
inline constexpr std::size_t kSmiCountBytes = 8;

/// @brief The vendor gate of the counter (FR-010, D-4, P-4): the
/// `vendor_id` line of `proc/cpuinfo`, resolved through `procRootPath`,
/// decides the answer, so a test plants the text it needs. One vendor
/// text documents the counter, and the gate names no substitute counter
/// for any other part (D-4).
///
/// \pre none
/// \post a true answer stands a `vendor_id` line that reads the vendor
///       text the counter stands documented behind, and a false answer
///       stands any other text and an unreadable file
[[nodiscard]] auto smiSourceSupported() -> bool;

/// @brief The open of the register node of one cpu (FR-007, D-3, P-2):
/// `dev/cpu/<cpu>/msr` through `procRootPath`, one `::open` call with
/// read rights and no write rights anywhere (D-3). The probe holds the
/// descriptor across one window, so the gap-path read costs one `::pread`
/// and no open (P-2, FR-012). A node that stands absent and a node the
/// process holds no rights on each answer the empty optional, and the
/// source stands not available (FR-011). A directory in the place of the
/// node answers a descriptor whose read answers nothing (FR-011).
///
/// \pre none
/// \post a held answer stands an open read-only descriptor of the node
///       of `cpu`, and the empty optional stands a node the open refused
[[nodiscard]] auto openMsrNode(std::uint32_t cpu) -> std::optional<int>;

/// @brief The close of a descriptor `openMsrNode` answered (P-2): the
/// second half of the pair, so no path reaches `::close` on a descriptor
/// the unit did not open. The result of the call answers nothing: the run
/// never fails on a counter source (FR-011).
///
/// \pre `descriptor` stands a descriptor `openMsrNode` answered
auto closeMsrNode(int descriptor) -> void;

/// @brief The counter read of one open node (FR-007, FR-011, D-3, P-2):
/// one `::pread` of `kSmiCountBytes` bytes at `kSmiCountAddress` into a
/// stack buffer, read as one little-endian unsigned value. The call
/// writes nowhere and allocates nothing, so the gap path keeps its
/// standing cost (FR-012). A read that fails and a read that answers
/// fewer bytes than the width each answer the empty optional (FR-011).
///
/// \pre `descriptor` stands a descriptor `openMsrNode` answered, open
///      read-only
/// \post a held answer stands the count the node answered at the counter
///       address, and the empty optional stands a read that failed or
///       answered a short count
[[nodiscard]] auto smiCountOf(int descriptor) -> std::optional<std::uint64_t>;

/// @brief The non-maskable-interrupt count of one cpu out of the text of
/// `proc/interrupts` (FR-009, FR-011, D-4, P-4): the figure of the `NMI`
/// row in the column that names `cpu`, the columns counted from zero the
/// way the standing `interrupts` reader counts them. The figure stands
/// named apart from the standing `interrupts` figure of
/// `precheckReadSnapshot`, which keeps its sum over every labeled row
/// (FR-009). A text that holds no `NMI` row, a row that names fewer
/// columns than the cpu asks for, and a figure that parses as no number
/// each answer the empty optional (FR-011).
///
/// \pre `text` stands one whole text
/// \post a held answer stands one figure of the `NMI` row of the column
///       of `cpu`
[[nodiscard]] auto nmiCountOfText(const std::string& text, std::uint32_t cpu)
    -> std::optional<std::uint64_t>;

}  // namespace sg::detail

#endif  // SG_HARNESS_LATENCY_COUNTERS_HPP
