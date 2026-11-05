// ============================================================================
// The one pass that seats a device's counter limit (specs/041-counter-probe,
// T040; FR-022, FR-030 to FR-034, FR-040a, FR-042 to FR-044, D-6, D-8).
//
// Three finders can answer a device, and only one of them opens anything in
// the kernel. The decode reads registers the host already answered, the
// device and event rules read a name, and the group search opens copies of an
// event. The order below is the cost order: the free finders first, and the
// search only for a device the registers are silent about (FR-030, FR-031).
// The search then runs once per device for the life of the process, because a
// device's capacity does not move while the process runs (FR-022, D-6).
// ============================================================================

#ifdef __linux__

#  include <algorithm>
#  include <cstddef>
#  include <cstdint>
#  include <map>
#  include <optional>
#  include <string>
#  include <string_view>

#  include "../detail/limit_fill.hpp"

#  include "../detail/group_probe.hpp"
#  include "../detail/limit_source_record.hpp"
#  include "../detail/pmu.hpp"
#  include "counter_limits.hpp"
#  include "speedgun-ng/dbc.hpp"

namespace sg::counters::detail
{

namespace
{

// The searches this process has settled, keyed by device name. A second
// enumerate of the same device reads the answer the first one paid for and
// opens nothing, which is the difference between one search per catalog open
// and one per process (FR-022, SC-005). A search that stopped with no count
// settles too: a device that cannot answer holds no bound, and asking it
// again costs the same opens for the same nothing. No lock stands here,
// because this pass is the only writer and it runs under the registration
// lock the system holds.
[[nodiscard]] auto searchedLimit(const PmuDevice& device,
                                 std::string_view eventName)
    -> std::optional<std::size_t>
{
  static std::map<std::string, std::optional<std::size_t>> settled;

  const std::string key {device.path};
  const auto known = settled.find(key);
  if (known != settled.end()) {
    return known->second;
  }

  // The words the device publishes for that event, opened on the device's own
  // type number: the pair that asks the device named. A generic open would ask
  // the device serving the calling thread, and on a hybrid host those are two
  // devices with two answers (FR-033, D-9). A device listing no such entry, or
  // listing one
  // the encoder refused, has nothing to search with and pays no open (FR-034).
  const auto entry =
      std::ranges::find(device.entries, eventName, &PmuEntry::name);
  if (entry == device.entries.end() || entry->words.empty()) {
    return std::nullopt;
  }

  const auto answer = probeGroupLimit(GroupProbeOptions {
      .deviceName = device.path,
      .pmuType = static_cast<std::uint32_t>(device.type),
      .eventConfig = static_cast<std::uint64_t>(entry->words.front().second),
  });
  return settled.emplace(key, answer.count).first->second;
}

}  // namespace

auto fillDeviceLimit(const PmuDevice& device,
                     const CounterLimitVendor vendor,
                     const int watchdogValue,
                     const bool hasFixedCycles,
                     const std::optional<std::size_t> decoded) -> LimitFill
{
  LimitFill fill;

  if (decoded.has_value()) {
    // The decode answered, so the watchdog deduction runs on its figure and
    // no open follows (FR-030, FR-044). A deduction that leaves no counter
    // publishes nothing, and the search stays unasked: the registers were not
    // silent, they were spent.
    fill.count = usableCounters(decoded, vendor, watchdogValue, hasFixedCycles);
    fill.source =
        fill.count.has_value() ? LimitSource::DECODE : LimitSource::NONE;
  } else if (isProbeCoreDevice(device.path)) {
    // The registers are silent about a core device. The event rule names the
    // event to search with, and a device it names nothing for is asked nothing
    // (FR-031, FR-034). The figure the search answered reaches the seed as the
    // kernel answered it: k copies ran whole at k and not at k + 1, so the
    // counter the watchdog holds is already missing from that answer, and a
    // second deduction would take one counter twice (D-8, FR-042).
    if (const auto eventName = probeEventNameForDevice(device.path, vendor)) {
      if (const auto count = searchedLimit(device, *eventName);
          count.has_value())
      {
        fill.count = count;
        fill.source = LimitSource::PROBE;
      }
    }
  }

  // The record answers the pass that ran, on every arm (FR-040a, FR-043).
  // A pass that seats no figure records NONE, which erases any record an
  // earlier pass left on that path: the reader's answer belongs to the state
  // the catalog holds now, and a stale finder name beside an absent figure is
  // the one answer the pair must never give.
  recordLimitSource(device.path, fill.source);
  SG_ENSURE(fill.count.has_value() == (fill.source != LimitSource::NONE),
            "a published limit names its finder");

  return fill;
}

}  // namespace sg::counters::detail

#endif  // __linux__
