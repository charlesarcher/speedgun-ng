// ============================================================================
// The Linux PMU provider through the public surface, over a scripted
// sysfs tree and a scripted kernel (G-19, G-21; Phase D).
//
// The provider's enumeration walks two kernel-published file trees and
// probes every entry with one `perf_event_open` per kind, so a runner
// whose kernel refuses the syscall reaches none of the arms the
// catalog reports. The seam stands the trees and the answers in: the
// devices root and the paranoid file are fixture files the test
// writes, and the scripted kernel grants or refuses the probe, the
// mapped page, and the releases the provider drives. Each scenario
// clears the install it stands in.
//
// Frameworkless check()/fail() convention.
// ============================================================================

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "speedgun-ng/counters_pmu.hpp"

#include <linux/perf_event.h>
#include <sys/mman.h>
#include <unistd.h>

#include "../source/counters/linux_pmu/kernel_calls.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_provider.hpp"

namespace
{

auto failures = int {0};

auto check(const bool ok, const char* sentence) -> void
{
  if (!ok) {
    std::printf("PMU PROVIDER TEST FAIL: %s\n", sentence);
    ++failures;
  }
}

using sg::counters::Availability;
using sg::counters::LeafSet;
using sg::counters::ObjectSeed;
using sg::counters::ObjectSink;
using sg::counters::PmuProvider;
using sg::counters::ReadMode;
using sg::counters::Target;
using sg::counters::detail::installKernelCalls;
using sg::counters::detail::KernelCalls;

/// The catalog the provider seeds, one object per call.
class RecordingSink final : public ObjectSink
{
public:
  struct Entry
  {
    std::string name;
    std::string description;
    Availability avail = Availability::COUNTABLE;
    ReadMode mode = ReadMode::SYSCALL;
    bool hasRatioPair = false;
  };

  void addObject(const ObjectSeed& seed) override
  {
    paths.emplace_back(seed.path);
    descriptions.emplace_back(seed.description);
    limits.push_back(seed.programmableCounters);
    entries.emplace_back();
    for (const auto& entry : seed.entries) {
      entries.back().push_back(
          Entry {.name = std::string(entry.name),
                 .description = std::string(entry.description),
                 .avail = entry.avail,
                 .mode = entry.mode,
                 .hasRatioPair = entry.hasRatioPair});
    }
  }

  std::vector<std::string> paths;
  std::vector<std::string> descriptions;
  std::vector<std::optional<std::size_t>> limits;
  std::vector<std::vector<Entry>> entries;
};

// The scripted kernel's fixture state: the fixture roots, the answers
// the probe and the page read see, and the record of the calls the
// provider drives.
std::filesystem::path gDevicesRoot;
std::filesystem::path gParanoidPath;
// The scripted descriptors start above the standard streams: the probe
// closes the descriptor a granted open answers with, and a descriptor
// the kernel would never answer with keeps the streams whole.
int gNextFd = 100;
// The open ordinal the scripted kernel refuses; -1 refuses none. The
// refusal carries the errno the scenario tuned, so the catalog reports
// the state the real refusal carries.
int gRefuseAt = -1;
int gErrno = EACCES;
// True when the scripted kernel refuses every cpu-targeted open while
// it grants the per-task ones, the matrix the paranoid level of 2
// produces (FR-021, FR-034).
bool gRefuseCpuTarget = false;
std::map<int, void*> gPages;
std::vector<int> gOpenedFds;
std::vector<int> gClosedFds;

/// The scripted set the provider runs through: the two path functions
/// answer the fixture trees, the open grants fresh descriptors or
/// refuses on the ordinal the scenario tuned, the map answers the page
/// the scenario registered under the descriptor, and the rest answer
/// success without touching the host.
auto scriptedCalls() -> KernelCalls
{
  KernelCalls calls {};
  calls.openEvent = [](void*, int, int cpu, int, unsigned long) -> long
  {
    if (gRefuseCpuTarget && cpu >= 0) {
      errno = EACCES;
      return -1;
    }
    if (gRefuseAt >= 0 && static_cast<int>(gOpenedFds.size()) == gRefuseAt) {
      errno = gErrno;
      return -1;
    }
    const int fd = gNextFd++;
    gOpenedFds.push_back(fd);
    return fd;
  };
  calls.mapPage = [](void*, std::size_t, int, int, int fd, long long) -> void*
  {
    const auto found = gPages.find(fd);
    // A descriptor no scenario registered a page under answers the
    // mapping refusal, the way a kernel that maps nothing answers.
    return found != gPages.end() ? found->second : MAP_FAILED;
  };
  calls.unmapPage = [](void*, std::size_t) -> int { return 0; };
  calls.readGroup = [](int, void*, std::size_t) -> long { return -1; };
  calls.ioctlEvent = [](int, unsigned long, unsigned long) -> int { return 0; };
  calls.closeFd = [](int fd) -> int
  {
    gClosedFds.push_back(fd);
    return 0;
  };
  calls.devicesRoot = []() -> std::filesystem::path { return gDevicesRoot; };
  calls.paranoidPath = []() -> std::filesystem::path { return gParanoidPath; };
  return calls;
}

/// Clears the record the scenarios read between their steps, and hands
/// the real set back so no scenario reads another's scripted answers.
auto resetFixture() -> void
{
  gNextFd = 100;
  gRefuseAt = -1;
  gErrno = EACCES;
  gRefuseCpuTarget = false;
  gPages.clear();
  gOpenedFds.clear();
  gClosedFds.clear();
  installKernelCalls(nullptr);
}

/// One device directory of the fixture tree: a `type` file, a `format`
/// directory with one file per named spec, and an `events` directory
/// with one file per named alias. A name may carry a dot, which is how
/// a scenario models the per-alias attribute files sysfs publishes
/// beside an event.
auto writeDevice(
    const std::filesystem::path& root,
    const std::string& name,
    const std::string& type,
    const std::vector<std::pair<std::string, std::string>>& formats,
    const std::vector<std::pair<std::string, std::string>>& events,
    const bool withCpumask = false) -> void
{
  const auto dir = root / name;
  std::filesystem::create_directories(dir / "format");
  std::filesystem::create_directories(dir / "events");
  if (!type.empty()) {
    std::ofstream file(dir / "type");
    file << type;
  }
  for (const auto& [field, spec] : formats) {
    std::ofstream file(dir / "format" / field);
    file << spec;
  }
  for (const auto& [alias, text] : events) {
    std::ofstream file(dir / "events" / alias);
    file << text;
  }
  if (withCpumask) {
    std::ofstream file(dir / "cpumask");
    file << "0";
  }
}

/// Writes the paranoid file the scenario tunes, or leaves it absent,
/// which is the unreadable case the provider reports.
auto writeParanoid(const std::string& text) -> void
{
  const std::filesystem::path path = std::filesystem::temp_directory_path()
      / ("pmu-paranoid-" + std::to_string(::getpid()));
  if (text.empty()) {
    std::filesystem::remove(path);
  } else {
    std::ofstream file(path);
    file << text;
  }
  gParanoidPath = path;
}

int gRootSerial = 0;

/// A fresh fixture root, so a scenario owns the tree it walks.
auto freshRoot() -> std::filesystem::path
{
  const auto root = std::filesystem::temp_directory_path()
      / ("pmu-devices-" + std::to_string(::getpid()) + "-"
         + std::to_string(gRootSerial++));
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root);
  return root;
}

/// The core device every scenario's tree carries: a usable type, one
/// config field, and two aliases beside it.
auto writeCoreDevice(const std::filesystem::path& root,
                     const std::vector<std::pair<std::string, std::string>>&
                         extraEvents = {}) -> void
{
  auto events = std::vector<std::pair<std::string, std::string>> {
      {"cache-misses", "config=0x2d"}, {"cache-references", "config=0x31"}};
  for (const auto& one : extraEvents) {
    events.push_back(one);
  }
  writeDevice(root, "cpu", "4", {{"config", "config:0-47"}}, events);
}



/// Runs one provider over the fixture tree and answers the catalog it
/// seeded.
auto enumerate() -> RecordingSink
{
  RecordingSink sink;
  PmuProvider provider;
  provider.enumerate(sink);
  return sink;
}

}  // namespace

// ---- P1: the paranoid read ------------------------------------------------

/// The level the provider reports comes from the file the seam names:
/// a readable file names its value, and a file the read cannot parse
/// names the unreadable level (FR-039).
auto paranoidReadScenario() -> void
{
  auto calls = scriptedCalls();
  resetFixture();
  const auto root = freshRoot();
  writeCoreDevice(root);
  gDevicesRoot = root;
  writeParanoid("2\n");
  installKernelCalls(&calls);
  const auto granted = enumerate();
  check(granted.descriptions.size() == 1, "the fixture tree seeds one object");
  check(granted.descriptions.front().find("perf_event_paranoid 2")
            != std::string::npos,
        "the level note names the value the paranoid file carries " "(FR-039)");
  resetFixture();

  writeParanoid("not a number");
  installKernelCalls(&calls);
  const auto unreadable = enumerate();
  check(unreadable.descriptions.size() == 1,
        "the fixture tree seeds one object");
  check(
      unreadable.descriptions.front().find("an unreadable perf_event_paranoid")
          != std::string::npos,
      "a file the read cannot parse names the unreadable level " "(FR-039)");
  resetFixture();
}

auto main() -> int
{
  paranoidReadScenario();
  if (failures != 0) {
    return 1;
  }
  std::printf(
      "counters_pmu_provider_test PASS: provider over the scripted " "kernel"
                                                                     "\n");
  return 0;
}
