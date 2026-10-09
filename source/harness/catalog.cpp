#include <cstdio>

#include "detail/internal.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_system.hpp"

namespace sg::detail
{

namespace
{

auto readModeName(const sg::counters::ReadMode mode) -> const char*
{
  switch (mode) {
    case sg::counters::ReadMode::FAST_TSC:
      return "fast-tsc";
    case sg::counters::ReadMode::FAST_RDPMC:
      return "fast-rdpmc";
    case sg::counters::ReadMode::SYSCALL:
      return "syscall";
    case sg::counters::ReadMode::PUSH_LOAD:
      return "push-load";
  }
  return "unknown";
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
  if (!machine.has_value()) {
    std::fprintf(stderr,
                 "the catalog publishes no machine object: %s\n",
                 machine.error().message.c_str());
    return false;
  }
  printObject(*machine);
  return true;
}

}  // namespace sg::detail
