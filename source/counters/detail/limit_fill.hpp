#ifndef SG_COUNTERS_DETAIL_LIMIT_FILL_HPP
#define SG_COUNTERS_DETAIL_LIMIT_FILL_HPP

// Layer: internal detail, module M4 of specs/041-counter-probe.
// SPDX-License-Identifier: BSD-3-Clause

/// \file
/// \brief The one pass that decides a device's published counter limit and
/// the finder that answered it.
///
/// The provider decodes a device's limit from the host's own registers, and a
/// host whose registers answer nothing used to publish nothing at all
/// (FR-031). The group search of `group_probe.hpp` can answer that device, and
/// the two finders must never both be asked: a device the decode answers pays
/// no open (FR-030), and a figure the search answered takes no second
/// deduction for the NMI watchdog, because the search already saw the counter
/// the watchdog holds (D-8, FR-042).
///
/// The decision takes the decoded figure as an argument. It reads no register
/// of its own. That is the whole reason it stands apart from
/// `provider.cpp`: the arms below are a table, and a table whose rows depend
/// on the CPU of the machine running the test has rows no CI host can reach.
/// `provider.cpp` reads the registers and hands the words over.
///
/// The pass records the finder beside the figure through
/// `limit_source_record.hpp`, which is how `objectLimitSource` and
/// `Object::programmableCounters()` agree on every device (FR-040a, FR-043).

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include "speedgun-ng/counters_system.hpp"

namespace sg::counters::detail
{

struct PmuDevice;
enum class CounterLimitVendor : std::uint8_t;

/// The figure one device publishes, beside the finder that answered it.
struct LimitFill
{
  std::optional<std::size_t> count {};  ///< the figure the seed publishes
  LimitSource source {LimitSource::NONE};  ///< the finder that answered it
};

/// Decide one device's published limit, and record the finder.
///
/// The steps stand in this order, and the order is the requirement: the
/// decode first, the device rule and the event rule before any open, the
/// search once per device per process, and the record last (FR-022, FR-030 to
/// FR-034, FR-040a, FR-043, D-6, D-8).
///
/// \param device The device the pass seats, with its own listed events.
/// \param vendor The host's vendor answer, which picks the event rule.
/// \param watchdogValue The host's NMI watchdog answer in ticks.
/// \param hasFixedCycles True when the host's decode counts a fixed cycle
///        counter among the programmable set.
/// \param decoded The decode's figure for this device, empty where the decode
///        is silent about it.
/// \pre `device.path` names the object the caller seats, and no requirement
///      falls on the other fields: a device listing no events is a legal
///      argument and answers no probed figure.
/// \post The answer carries a count exactly where it names a finder that is
///       not `NONE`, and `objectLimitSource(device.path)` answers that same
///       finder (FR-040a).
/// \post A decoded figure keeps the watchdog deduction of `usableCounters`
///       and pays no open (FR-030, FR-044).
/// \post A searched figure reaches the answer as the search answered it, with
///       no deduction (FR-042, D-8).
/// \post A device the pass seats with no figure records `NONE`, which erases
///       any record an earlier pass left on that path, so the reader answers
///       `NONE` beside the seed's absent figure (FR-040a).
[[nodiscard]] auto fillDeviceLimit(const PmuDevice& device,
                                   CounterLimitVendor vendor,
                                   int watchdogValue,
                                   bool hasFixedCycles,
                                   std::optional<std::size_t> decoded)
    -> LimitFill;

}  // namespace sg::counters::detail

#endif  // SG_COUNTERS_DETAIL_LIMIT_FILL_HPP
