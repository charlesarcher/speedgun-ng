// The one pass that seats a device's counter limit, and the finder it names
// (specs/041-counter-probe, T041; FR-022, FR-030 to FR-034, FR-040a, FR-042 to
// FR-044, D-6, D-8).
//
// The pass takes the decode's figure as an argument, so every row of its table
// reaches a case on any host: a machine whose registers answer nothing about a
// device is a state no CI runner can be asked to produce, and a row no runner
// can reach is a row no one has tested. The search's own arms stand behind the
// scripted kernel, which answers the grant at a size, the short run past it,
// and the refusal the unit names (FR-011, FR-073).
//
// One device name carries one search for the life of the process (FR-022), so
// the scenarios below name a different device wherever a search is asked for:
// a second scenario on the same name would read the first one's settled answer
// in place of the answer it scripts.

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <linux/perf_event.h>
#include <sys/mman.h>
#include <unistd.h>

#include "../source/counters/detail/limit_fill.hpp"
#include "../source/counters/detail/limit_source_record.hpp"
#include "../source/counters/detail/pmu.hpp"
#include "../source/counters/linux_pmu/counter_limits.hpp"
#include "../source/counters/linux_pmu/kernel_calls.hpp"
#include "speedgun-ng/counters_system.hpp"

namespace
{

using sg::counters::LimitSource;
using sg::counters::objectLimitSource;
using sg::counters::detail::clearLimitSources;
using sg::counters::detail::CounterLimitVendor;
using sg::counters::detail::fillDeviceLimit;
using sg::counters::detail::installKernelCalls;
using sg::counters::detail::KernelCalls;
using sg::counters::detail::PmuDevice;
using sg::counters::detail::PmuEntry;

auto failures = int {0};

auto check(const bool ok, const char* sentence) -> void
{
  if (!ok) {
    std::fprintf(stderr, "  FAIL: %s\n", sentence);
    ++failures;
  }
}

// ---------------------------------------------------------------------------
// The scripted kernel
//
// `gProbeFitMax` is the largest group size the scripted kernel runs whole: at
// that size and below the pair answers `time_running == time_enabled`, above
// it the running word falls short, the shape the measured host showed at 7
// copies (F-1). Zero grants no window at all: the read itself refuses, which is
// the arm a host with no counter answers.
// ---------------------------------------------------------------------------

int gNextFd = 100;
int gProbeFitMax = 0;
int gGroupMemberOpens = 0;
int gOpenCount = 0;
std::map<int, int> gGroupSizes;
std::vector<int> gOpenedFds;
std::vector<int> gClosedFds;
std::vector<std::uint32_t> gOpenTypes;
std::vector<std::uint64_t> gOpenConfigs;

auto scriptedCalls() -> KernelCalls
{
  KernelCalls calls {};
  calls.openEvent = [](void* attr, int, int, int groupFd, unsigned long) -> long
  {
    const auto* typed = static_cast<const perf_event_attr*>(attr);
    gOpenTypes.push_back(typed->type);
    gOpenConfigs.push_back(typed->config);
    ++gOpenCount;
    const int fd = gNextFd++;
    gOpenedFds.push_back(fd);
    if (groupFd < 0) {
      gGroupSizes[fd] = 1;
    } else {
      ++gGroupSizes[groupFd];
      ++gGroupMemberOpens;
    }
    return fd;
  };
  calls.mapPage = [](void*, std::size_t, int, int, int, long long) -> void*
  { return MAP_FAILED; };
  calls.unmapPage = [](void*, std::size_t) -> int { return 0; };
  calls.readGroup = [](int fd, void* buf, std::size_t count) -> long
  {
    if (gProbeFitMax == 0 || count < 3U * sizeof(std::uint64_t)) {
      return -1;
    }
    const auto words = static_cast<std::uint64_t*>(buf);
    const auto size = gGroupSizes[fd];
    words[0] = 7U;
    words[1] = 1000U;
    words[2] = size <= gProbeFitMax ? 1000U : 500U;
    return static_cast<long>(3U * sizeof(std::uint64_t));
  };
  calls.ioctlEvent = [](int, unsigned long, unsigned long) -> int { return 0; };
  calls.closeFd = [](int fd) -> int
  {
    gClosedFds.push_back(fd);
    return 0;
  };
  calls.devicesRoot = []() -> std::filesystem::path { return {}; };
  calls.paranoidPath = []() -> std::filesystem::path { return {}; };
  calls.cpuidWords = []() -> sg::counters::detail::CpuidWords { return {}; };
  return calls;
}

auto gScripted = KernelCalls {};

auto beginScenario() -> void
{
  gNextFd = 100;
  gProbeFitMax = 0;
  gGroupMemberOpens = 0;
  gOpenCount = 0;
  gGroupSizes.clear();
  gOpenedFds.clear();
  gClosedFds.clear();
  gOpenTypes.clear();
  gOpenConfigs.clear();
  gScripted = scriptedCalls();
  clearLimitSources();
  installKernelCalls(&gScripted);
}

auto endScenario() -> void
{
  installKernelCalls(nullptr);
  clearLimitSources();
}

// One core device of the scripted tree: its own type number, and the event the
// rule names listed with the words the device itself publishes.
auto coreDevice(const std::string_view name,
                const int type,
                const bool listsEvent) -> PmuDevice
{
  auto device = PmuDevice {};
  device.path = std::string {name};
  device.type = type;
  device.description = "scripted device of the fill table";
  if (listsEvent) {
    device.entries.push_back(PmuEntry {
        .name = "branch-instructions",
        .description = "scripted alias",
        .words = {{0, 0xC2U}},
    });
  }
  return device;
}

auto openedWith(const std::uint32_t type, const std::uint64_t config) -> bool
{
  for (std::size_t i = 0; i < gOpenConfigs.size(); ++i) {
    if (gOpenTypes[i] == type && gOpenConfigs[i] == config) {
      return true;
    }
  }
  return false;
}

// ---------------------------------------------------------------------------
// The decode rows
// ---------------------------------------------------------------------------

// The decode answered, so its figure keeps the watchdog deduction and the
// search stays unasked (FR-030, FR-044).
auto decodedRowsKeepTheDeduction() -> void
{
  beginScenario();
  const auto device = coreDevice("cpu", 41, true);
  const auto untouched = fillDeviceLimit(device,
                                         CounterLimitVendor::AMD,
                                         0,
                                         false,
                                         std::optional<std::size_t> {8U});
  check(untouched.count.value_or(0U) == 8U,
        "a decoded figure with no watchdog holds at its figure (FR-044)");
  check(untouched.source == LimitSource::DECODE,
        "a decoded figure names the decode (FR-043)");
  check(objectLimitSource("cpu") == LimitSource::DECODE,
        "the reader names the decode beside the figure (FR-040a)");

  const auto watched = fillDeviceLimit(device,
                                       CounterLimitVendor::AMD,
                                       1,
                                       false,
                                       std::optional<std::size_t> {8U});
  check(watched.count.value_or(0U) == 7U,
        "a watchdog of 1 costs a decoded figure one counter (FR-044)");
  check(watched.source == LimitSource::DECODE,
        "the deducted figure still names the decode (FR-043)");

  const auto spent = fillDeviceLimit(device,
                                     CounterLimitVendor::AMD,
                                     1,
                                     false,
                                     std::optional<std::size_t> {1U});
  check(!spent.count.has_value() && spent.source == LimitSource::NONE,
        "a deduction that leaves no counter publishes nothing (FR-005)");
  check(objectLimitSource("cpu") == LimitSource::NONE,
        "no record stands beside an absent figure (FR-040a)");
  check(gOpenCount == 0, "a device the decode answers pays no open (FR-030)");
  endScenario();
}

// ---------------------------------------------------------------------------
// The search rows
// ---------------------------------------------------------------------------

// The registers are silent, the kernel runs 6 copies whole and 7 short, and the
// figure that reaches the seed is the largest size that ran (FR-031, FR-015).
auto silentDecodeSearchesTheDevice() -> void
{
  beginScenario();
  gProbeFitMax = 6;
  const auto device = coreDevice("cpu_core", 41, true);
  const auto fill =
      fillDeviceLimit(device, CounterLimitVendor::AMD, 0, false, std::nullopt);
  check(fill.count.value_or(0U) == 6U,
        "a silent decode publishes the figure the search answered (FR-031)");
  check(fill.source == LimitSource::PROBE,
        "a searched figure names the search (FR-043)");
  check(objectLimitSource("cpu_core") == LimitSource::PROBE,
        "the reader names the search beside the figure (FR-040a)");
  check(openedWith(41U, 0xC2U),
        "the search opens the device's own type with the device's own words "
        "(FR-033, D-9)");
  check(gGroupMemberOpens > 0, "a search opens group members (FR-012)");
  check(gOpenedFds.size() == gClosedFds.size(),
        "every descriptor the search opened is closed (FR-019)");

  // The second pass reads the answer the first one paid for: a device's
  // capacity holds while the process runs, so a second catalog open costs no
  // open at all (FR-022, SC-005).
  const int paidBefore = gOpenCount;
  gOpenCount = 0;
  const auto again =
      fillDeviceLimit(device, CounterLimitVendor::AMD, 0, false, std::nullopt);
  check(again.count.value_or(0U) == 6U,
        "the settled answer survives the second pass (FR-022)");
  check(gOpenCount == 0,
        "a second enumerate of one device opens nothing (FR-022, SC-005)");
  check(paidBefore > 0, "the first pass paid the one search (FR-022)");
  endScenario();
}

// The counter the watchdog holds is missing from the kernel's answer already,
// so a searched figure takes no second deduction (D-8, FR-042).
auto searchedFigureTakesNoDeduction() -> void
{
  beginScenario();
  gProbeFitMax = 6;
  const auto device = coreDevice("cpu", 41, true);
  const auto fill =
      fillDeviceLimit(device, CounterLimitVendor::AMD, 1, false, std::nullopt);
  check(fill.count.value_or(0U) == 6U,
        "a watchdog of 1 costs a searched figure nothing (FR-042, D-8)");
  check(fill.source == LimitSource::PROBE,
        "the undeducted figure names the search (FR-043)");
  endScenario();
}

// A search that reads no pair holds no bound, and the refusal settles for the
// process: the device is asked once, and a kernel that would answer later is
// not asked again (FR-014, FR-018, FR-022).
auto refusedSearchPublishesNothingAndSettles() -> void
{
  beginScenario();
  const auto device = coreDevice("cpu_atom", 41, true);
  const auto refused =
      fillDeviceLimit(device, CounterLimitVendor::AMD, 0, false, std::nullopt);
  check(!refused.count.has_value() && refused.source == LimitSource::NONE,
        "a search that reads no pair publishes nothing (FR-014, FR-018)");
  check(objectLimitSource("cpu_atom") == LimitSource::NONE,
        "no record stands beside a refused search (FR-040a)");
  check(gOpenedFds.size() == gClosedFds.size(),
        "a refused search closes what it opened (FR-019)");

  const int paidBefore = gOpenCount;
  gOpenCount = 0;
  gProbeFitMax = 6;
  const auto again =
      fillDeviceLimit(device, CounterLimitVendor::AMD, 0, false, std::nullopt);
  check(!again.count.has_value(),
        "a settled refusal answers no figure (FR-022)");
  check(gOpenCount == 0, "a settled refusal costs no second search (FR-022)");
  check(paidBefore > 0, "the first pass paid the refusal once (FR-022)");
  endScenario();
}

// ---------------------------------------------------------------------------
// The rows that pay nothing
// ---------------------------------------------------------------------------

// The device lists the event the rule names, and the listing carries no words
// for it: the encoder answered nothing for that event, so the search holds no
// config to open with. The device answers no figure, names no finder, records
// nothing and opens nothing (FR-034, FR-040a). The refusal returns ahead of the
// settled table, so it leaves no answer behind and the name stays free for the
// row that pays a search (FR-022).
auto listedEventWithNoWordsPaysNothing() -> void
{
  beginScenario();
  gProbeFitMax = 6;  // a search that reached the kernel would answer a figure
  auto device = coreDevice("cpu_core", 41, true);
  device.entries.front().words.clear();
  const auto fill =
      fillDeviceLimit(device, CounterLimitVendor::AMD, 0, false, std::nullopt);
  check(!fill.count.has_value() && fill.source == LimitSource::NONE,
        "an entry listed with no words answers no figure and names none "
        "(FR-034)");
  check(objectLimitSource("cpu_core") == LimitSource::NONE,
        "no record stands beside an entry listed with no words (FR-040a)");
  check(gOpenCount == 0,
        "an entry listed with no words opens nothing (FR-034)");
  endScenario();
}

// An uncore device is outside the rule, and a device the event rule names
// nothing for is asked nothing: both publish nothing and open nothing
// (FR-032, FR-034).
auto guardedDevicesPayNothing() -> void
{
  beginScenario();
  gProbeFitMax = 6;
  const auto uncore = coreDevice("uncore_imc_0", 17, true);
  const auto uncoreFill =
      fillDeviceLimit(uncore, CounterLimitVendor::AMD, 0, false, std::nullopt);
  check(!uncoreFill.count.has_value() && uncoreFill.source == LimitSource::NONE,
        "a device outside the rule publishes nothing (FR-032)");
  check(gOpenCount == 0, "a device outside the rule opens nothing (FR-032)");

  const auto arm = coreDevice("armv8_core0", 9, true);
  const auto armFill =
      fillDeviceLimit(arm, CounterLimitVendor::OTHER, 0, false, std::nullopt);
  check(!armFill.count.has_value() && armFill.source == LimitSource::NONE,
        "a device the event rule names nothing for publishes nothing (FR-034)");
  check(gOpenCount == 0, "a device with no named event opens nothing (FR-034)");

  const auto bare = coreDevice("cpu", 41, false);
  const auto bareFill =
      fillDeviceLimit(bare, CounterLimitVendor::AMD, 0, false, std::nullopt);
  check(!bareFill.count.has_value(),
        "a core device listing no such event publishes nothing (FR-034)");
  check(gOpenCount == 0,
        "a core device listing no such event opens nothing (FR-034)");
  check(objectLimitSource("uncore_imc_0") == LimitSource::NONE
            && objectLimitSource("armv8_core0") == LimitSource::NONE
            && objectLimitSource("cpu") == LimitSource::NONE,
        "no record stands beside any of these three (FR-040a)");
  endScenario();
}

}  // namespace

auto main() -> int
{
  // The order is the device names': one name carries one search for the life
  // of the process (FR-022), so the row that asks for no search at all runs
  // first on `cpu`, and the rows that settle a search come after it. The row
  // that lists an event with no words stands ahead of the row that searches
  // `cpu_core`, because a settled name answers in place of the listing.
  guardedDevicesPayNothing();
  listedEventWithNoWordsPaysNothing();
  decodedRowsKeepTheDeduction();
  silentDecodeSearchesTheDevice();
  searchedFigureTakesNoDeduction();
  refusedSearchPublishesNothingAndSettles();

  if (failures != 0) {
    std::fprintf(stderr, "counters_limit_fill_test: %d FAILURES\n", failures);
    return EXIT_FAILURE;
  }
  std::printf("counters_limit_fill_test: PASS\n");
  return EXIT_SUCCESS;
}
