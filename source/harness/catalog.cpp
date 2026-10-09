#include <array>
#include <cstdio>
#include <utility>

#include "detail/internal.hpp"
#include "speedgun-ng/counters_core.hpp"
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
  return kReadModeNames[std::to_underlying(mode)];
}

void printObject(const sg::counters::Object& object)
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
  for (const auto* child : object.children()) {
    printObject(*child);
  }
}

}  // namespace

auto printCatalog() -> bool
{
  const auto machine = sg::counters::System::local().object("machine");
  SG_ASSERT(machine.has_value(),
            "the counters system opens the host provider, and that provider "
            "publishes the machine object (FR-002)");
  printObject(*machine);
  return true;
}

}  // namespace sg::detail
