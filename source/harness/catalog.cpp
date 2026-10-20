#include <array>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

#include "detail/internal.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_metrics.hpp"
#include "speedgun-ng/counters_system.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg::detail
{

namespace
{

// The names sit in a table indexed by the enumerator, so the listing
// holds no branch for a mode the enumeration excludes.
constexpr std::array<const char*, 4> kReadModeNames {
    "fast-tsc",
    "fast-rdpmc",
    "syscall",
    "push-load",
};
static_assert(kReadModeNames.size()
              == std::to_underlying(sg::counters::ReadMode::PUSH_LOAD) + 1);

auto readModeName(const sg::counters::ReadMode mode) -> const char*
{
  // The static_assert above ties the table size to the last enumerator of
  // the closed enumeration, so the index stays inside the table.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
  return kReadModeNames[std::to_underlying(mode)];
}

}  // namespace

// Metric lines follow the counter lines of the same object path, in table
// order, and `mode=metric` is the field that separates them (FR-021, FR-022,
// D-11).
auto printObject(const sg::counters::Object& object,
                 const std::vector<sg::counters::MetricListing>& metrics)
    -> void
{
  const std::string path = std::string(object.path());
  for (const auto& entry : object.counters()) {
    const bool countable = entry.avail == sg::counters::Availability::COUNTABLE;
    std::printf(
        "catalog: %s/%s description=%s unit=%s mode=%s avail=%s " "refusal=%"
                                                                  "s\n",
        path.c_str(),
        std::string(entry.name).c_str(),
        std::string(entry.description).c_str(),
        std::string(sg::counters::unitName(entry.unit)).c_str(),
        readModeName(entry.mode),
        availabilityName(entry.avail),
        countable ? "-" : availabilityName(entry.avail));
  }
  for (const auto& row : metrics) {
    if (row.objectPath != path) {
      continue;
    }
    // The two refusal fields settle outside the argument list: a
    // published row states neither, and the strings outlive the call.
    const std::string refusalText = row.published
        ? std::string("-")
        : std::string(sg::counters::metricRefusalName(row.refusal));
    const std::string reasonText =
        row.published ? std::string("-") : std::string(row.reason);
    std::printf("catalog: %s/%s group=%s description=%s unit=%s "
                "mode=metric avail=%s refusal=%s reason=%s\n",
                path.c_str(),
                std::string(row.name).c_str(),
                std::string(row.group).c_str(),
                std::string(row.description).c_str(),
                std::string(row.unit).c_str(),
                availabilityName(row.avail),
                refusalText.c_str(),
                reasonText.c_str());
  }
  for (const auto* child : object.children()) {
    printObject(*child, metrics);
  }
}

auto printCatalog() -> void
{
  const auto machine = sg::counters::System::local().object("machine");
  SG_ASSERT(machine.has_value(),
            "the counters system opens the host provider, and that provider "
            "publishes the machine object (FR-002)");
  // An empty listing is a host that selected no event table, which prints
  // no metric line and reports no error (FR-024).
  printObject(*machine, sg::counters::catalogMetrics());
}

}  // namespace sg::detail
