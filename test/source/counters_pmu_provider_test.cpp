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
#include <cerrno>
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
#include "../source/counters/linux_pmu/pmu_ident.hpp"
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
using sg::counters::detail::CpuidWords;
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

    auto operator==(const Entry&) const -> bool = default;
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
// The CPUID words the scripted identity answers: the words a pinned
// mapfile row matches stand in for the host's own identity, so the
// vendored merge reads the fixture's identity on every host (FR-001,
// FR-013).
CpuidWords gCpuidWords {};
// True when the scripted kernel refuses every cpu-targeted open while
// it grants the per-task ones, the matrix the paranoid level of 2
// produces (FR-021, FR-034).
bool gRefuseCpuTarget = false;
std::map<int, void*> gPages;
std::vector<int> gOpenedFds;
std::vector<int> gClosedFds;
// The config word of every attribute the scripted kernel saw, in call
// order: a scenario that composes an event reads here to learn the words
// the probe carried (FR-004).
std::vector<std::uint64_t> gOpenConfigs;

/// The words a pinned mapfile row matches: leaf 0 spells the vendor and
/// leaf 1 names family 26, model 8, the amdzen5 row the vendored table
/// pins (FR-001).
auto vendorRowWords() -> CpuidWords
{
  return {.leaf0Eax = 0x15U,
          .leaf0Ebx = 0x68747541U,  // "Auth"
          .leaf0Edx = 0x69746E65U,  // "enti"
          .leaf0Ecx = 0x444D4163U,  // "cAMD"
          .leaf1Answered = true,
          .leaf1Eax = 0x1000A80U};  // family 26, model 8
}

/// The words no pinned row matches: the vendor string the leaf 0 words
/// spell carries no mapfile row, so the selection finds no directory on
/// any host (FR-001).
auto noRowWords() -> CpuidWords
{
  return {.leaf0Eax = 0x15U,
          .leaf0Ebx = 0x75536F4EU,  // "Nosu"
          .leaf0Edx = 0x6E655663U,  // "cVen"
          .leaf0Ecx = 0x58726F64U,  // "dorX"
          .leaf1Answered = true,
          .leaf1Eax = 0x30650U};  // family 6, model 5
}

/// The words a pinned Intel row matches: leaf 0 spells GenuineIntel and
/// leaf 1 names family 6, model 106 (`0x6A`), the identity the row
/// `GenuineIntel-6-6[AC]` answers with `arch/x86/icelakex/`. That table
/// carries the fixed-counter rows the composition step reads (FR-004).
auto fixedCounterRowWords() -> CpuidWords
{
  return {.leaf0Eax = 0x1FU,
          .leaf0Ebx = 0x756E6547U,  // "Genu"
          .leaf0Edx = 0x49656E69U,  // "ineI"
          .leaf0Ecx = 0x6C65746EU,  // "ntel"
          .leaf1Answered = true,
          .leaf1Eax = 0x606A0U};  // family 6, model 0x6A
}

/// The scripted set the provider runs through: the two path functions
/// answer the fixture trees, the open grants fresh descriptors or
/// refuses on the ordinal the scenario tuned, the map answers the page
/// the scenario registered under the descriptor, and the rest answer
/// success without touching the host.
auto scriptedCalls() -> KernelCalls
{
  KernelCalls calls {};
  calls.openEvent = [](void* attr, int, int cpu, int, unsigned long) -> long
  {
    gOpenConfigs.push_back(static_cast<const perf_event_attr*>(attr)->config);
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
  calls.cpuidWords = []() -> CpuidWords { return gCpuidWords; };
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
  gOpenConfigs.clear();
  // The default identity matches no pinned row, so a scenario that
  // names no identity reads the no-row arm on every host (FR-001).
  gCpuidWords = noRowWords();
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

// ---- P2: device discovery -------------------------------------------------

/// The fixture alias's entry inside the object the provider seeded, or
/// no pointer when the object carries none: the vendor table merges its
/// own rows beside the fixture aliases, so the assertions name the
/// fixture entries and read the pair apart from them.
auto findEntry(const RecordingSink& sink,
               const std::string& object,
               const std::string& name) -> const RecordingSink::Entry*
{
  for (std::size_t index = 0; index < sink.paths.size(); ++index) {
    if (sink.paths[index] != object) {
      continue;
    }
    for (const auto& entry : sink.entries[index]) {
      if (entry.name == name) {
        return &entry;
      }
    }
  }
  return nullptr;
}

/// The devices the provider walks: the loadable ones seed the catalog,
/// the device with no usable `type` file is skipped, the device with a
/// negative type is skipped, and the device with nothing to describe
/// is dropped (FR-039).
auto deviceDiscoveryScenario() -> void
{
  auto calls = scriptedCalls();
  resetFixture();
  const auto root = freshRoot();
  writeCoreDevice(root);
  writeDevice(root, "empty-type", "", {}, {});
  writeDevice(root, "negative-type", "-1", {}, {});
  writeDevice(root,
              "bad-format",
              "5",
              {{"junk", "garbage"}},
              {{"alias", "config=0x1"}});
  writeDevice(root, "unknown-field", "6", {}, {{"alias", "event=0x1"}});
  writeDevice(root, "no-entries", "7", {}, {});
  writeDevice(
      root,
      "with-attr",
      "8",
      {{"config", "config:0-47"}},
      {{"cache-misses", "config=0x2d"}, {"cache-misses.percpu", "some=thing"}});
  writeDevice(root,
              "scoped",
              "9",
              {{"config", "config:0-47"}},
              {{"alias", "config=0x1"}},
              true);
  gDevicesRoot = root;
  writeParanoid("2\n");
  installKernelCalls(&calls);
  const auto sink = enumerate();
  check(sink.paths.size() == 5,
        "the five loadable devices seed five objects (FR-039)");
  const auto has = [&sink](const std::string& path)
  {
    for (const auto& one : sink.paths) {
      if (one == path) {
        return true;
      }
    }
    return false;
  };
  check(has("cpu") && has("bad-format") && has("unknown-field")
            && has("with-attr") && has("scoped"),
        "the loadable devices seed the catalog");
  check(!has("empty-type") && !has("negative-type"),
        "a device with no usable type file is skipped (FR-039)");
  check(
      !has("no-entries"),
      "a device with nothing countable or describable is dropped " "(FR-039)");
  // The attribute file beside the alias is no event of its own: the
  // with-attr device carries the alias alone (FR-037).
  for (std::size_t index = 0; index < sink.paths.size(); ++index) {
    if (sink.paths[index] != "with-attr") {
      continue;
    }
    check(findEntry(sink, "with-attr", "cache-misses") != nullptr,
          "the alias itself seeds the catalog");
    check(findEntry(sink, "with-attr", "cache-misses.percpu") == nullptr,
          "the attribute file beside the alias names no event (FR-037)");
  }
  resetFixture();
}

// ---- P3: the catalog parse ------------------------------------------------

/// The kernel alias wins a name conflict with the vendored table, and
/// the alias describes itself with the text the kernel publishes
/// (FR-037).
auto kernelAliasWinsScenario() -> void
{
  auto calls = scriptedCalls();
  resetFixture();
  const auto root = freshRoot();
  writeCoreDevice(root);
  gDevicesRoot = root;
  writeParanoid("2\n");
  // The identity a pinned row matches stands in for the host's own: the
  // vendored table merges its core row on every host, so the assertions
  // below hold wherever the test runs (FR-001, FR-013).
  gCpuidWords = vendorRowWords();
  installKernelCalls(&calls);

  // The first enumeration learns one vendored row's name: a cpu entry
  // whose description is the table's own prose.
  const auto first = enumerate();
  std::string vendored;
  std::string vendoredDescription;
  for (const auto& entry : first.entries.front()) {
    if (entry.name == "enabled" || entry.name == "running") {
      continue;
    }
    if (entry.description.find("kernel event configuration: ")
        == std::string::npos)
    {
      vendored = entry.name;
      vendoredDescription = entry.description;
      break;
    }
  }
  check(!vendored.empty(), "the vendored table merges a core row");

  // The second enumeration carries a kernel alias of the same name:
  // the alias wins, the entry count for the name stays at one, and the
  // description is the kernel's own text (FR-037).
  const auto root2 = freshRoot();
  writeCoreDevice(root2, {{vendored, "config=0x2d"}});
  gDevicesRoot = root2;
  const auto second = enumerate();
  std::size_t found = 0;
  std::string won;
  for (const auto& entry : second.entries.front()) {
    if (entry.name == vendored) {
      ++found;
      won = entry.description;
    }
  }
  check(found == 1, "the kernel alias wins the name conflict (FR-037)");
  check(
      won.find("kernel event configuration: ") == 0,
      "the winning entry describes itself with the kernel's text " "(FR-037)");
  resetFixture();
}

/// An identity no pinned row matches merges no vendored row: every
/// entry the catalog carries describes itself with the kernel's own
/// text, and the empty selection merges nothing (FR-001, FR-038).
auto noRowIdentityScenario() -> void
{
  auto calls = scriptedCalls();
  resetFixture();
  const auto root = freshRoot();
  writeCoreDevice(root);
  gDevicesRoot = root;
  writeParanoid("2\n");
  installKernelCalls(&calls);
  const auto sink = enumerate();
  for (const auto& entry : sink.entries.front()) {
    if (entry.name == "enabled" || entry.name == "running") {
      continue;
    }
    check(
        entry.description.find("kernel event") == 0,
        "an identity no row matches merges no vendored row (FR-001, " "FR-"
                                                                      "038)");
  }
  resetFixture();
}

/// The fixed-counter rows of a pinned Intel table reach the merge: the
/// event comes from the alias the device publishes for the counter, and
/// a row naming a counter the device publishes no alias for is refused
/// whole, with the number it names in its sentence (FR-004, FR-037).
auto fixedCounterRowScenario() -> void
{
  auto calls = scriptedCalls();
  resetFixture();
  const auto root = freshRoot();
  // The fixture publishes an alias for two of the counters the table's
  // rows ask for: fixed counter 0 and fixed counter 1. Fixed counter 2
  // has no alias here, and fixed counter 3 names no alias at all.
  writeDevice(
      root,
      "cpu",
      "4",
      {{"config", "config:0-47"}},
      {{"instructions", "config=0x3c"}, {"cpu-cycles", "config=0x300000003c"}});
  gDevicesRoot = root;
  writeParanoid("2\n");
  gCpuidWords = fixedCounterRowWords();
  installKernelCalls(&calls);
  const auto sink = enumerate();

  for (const auto* name : {"INST_RETIRED.ANY",
                           "INST_RETIRED.PREC_DIST",
                           "CPU_CLK_UNHALTED.THREAD"})
  {
    const auto* entry = findEntry(sink, "cpu", name);
    check(entry != nullptr && entry->avail == Availability::COUNTABLE,
          "a fixed row with the alias of its counter composes and counts "
          "(FR-037)");
    check(entry != nullptr
              && entry->description.find("refused") == std::string::npos,
          "the composed row carries no refusal sentence (FR-037)");
  }
  // The alias words reach the probe. The alias entry and the merged row
  // each take one probe per target kind, so the alias config appears
  // more often than the alias entry alone answers for.
  check(std::ranges::count(gOpenConfigs, 0x300000003cULL) >= 4,
        "the merged row is probed with the config the alias publishes "
        "(FR-037)");

  struct Refusal
  {
    std::string name;
    std::string needle;
  };

  for (const auto& one :
       {Refusal {"CPU_CLK_UNHALTED.REF_TSC", "no alias for fixed counter 2"},
        Refusal {"TOPDOWN.SLOTS", "no alias for fixed counter 3"}})
  {
    const auto* entry = findEntry(sink, "cpu", one.name);
    check(entry != nullptr && entry->avail == Availability::NOT_ENCODABLE,
          "a fixed row with no alias of its counter is refused (FR-037)");
    check(entry != nullptr
              && entry->description.find(one.needle) != std::string::npos,
          "the refusal names the fixed counter the row asks for (FR-039)");
  }
  resetFixture();
}

/// The errno matrix over the probe's refusal switch: the three encoding
/// errnos settle the not-encodable state, the permission errno settles
/// the blocked state, and the scripted kernel refuses with each errno
/// in turn, so no real open takes part (FR-002, FR-013).
auto errnoMatrixScenario() -> void
{
  auto calls = scriptedCalls();
  resetFixture();
  const auto root = freshRoot();
  writeCoreDevice(root);
  gDevicesRoot = root;
  writeParanoid("2\n");
  const std::pair<int, Availability> cases[] {
      {EINVAL, Availability::NOT_ENCODABLE},
      {EOPNOTSUPP, Availability::NOT_ENCODABLE},
      {ENOENT, Availability::NOT_ENCODABLE},
      {EACCES, Availability::PERMISSION_BLOCKED}};
  for (const auto& [refusal, expected] : cases) {
    gRefuseAt = 0;
    gErrno = refusal;
    installKernelCalls(&calls);
    const auto sink = enumerate();
    for (const auto& entry : sink.entries.front()) {
      if (entry.name == "enabled" || entry.name == "running") {
        continue;
      }
      check(entry.avail == expected,
            "the probe settles the state the refused open's errno names "
            "(FR-021)");
    }
    resetFixture();
  }
}

/// An alias whose file the kernel publishes empty describes itself
/// with the no-configuration sentence (FR-037).
auto aliasWithoutTextScenario() -> void
{
  auto calls = scriptedCalls();
  resetFixture();
  const auto root = freshRoot();
  writeCoreDevice(root, {{"quiet", ""}});
  gDevicesRoot = root;
  writeParanoid("2\n");
  installKernelCalls(&calls);
  const auto sink = enumerate();
  for (const auto& entry : sink.entries.front()) {
    if (entry.name != "quiet") {
      continue;
    }
    check(entry.description.find("the kernel publishes no configuration")
              != std::string::npos,
          "an alias with no configuration text says so (FR-037)");
  }
  resetFixture();
}

/// The defaulted copy constructor of the receiver: a copied sink carries
/// the state of the sink it came from, accepts its own objects, and
/// leaves that sink whole (FR-004).
auto sinkCopyScenario() -> void
{
  auto calls = scriptedCalls();
  resetFixture();
  const auto root = freshRoot();
  writeCoreDevice(root);
  gDevicesRoot = root;
  writeParanoid("2\n");
  installKernelCalls(&calls);

  RecordingSink kept;
  PmuProvider provider;
  provider.enumerate(kept);
  const auto seeded = kept.paths.size();
  const auto seededEntries = kept.entries;
  check(seeded > 0, "the fixture tree seeds the receiver before the copy");
  RecordingSink copy {kept};
  check(copy.paths == kept.paths && copy.entries == kept.entries,
        "the copy carries the state of the receiver it came from");
  provider.enumerate(copy);
  check(copy.paths.size() == 2 * seeded,
        "the copy accepts its own objects beside the ones it carried");
  check(kept.paths.size() == seeded && kept.entries == seededEntries,
        "the receiver the copy came from keeps the state it had");
  resetFixture();
}

auto main() -> int
{
  paranoidReadScenario();
  deviceDiscoveryScenario();
  kernelAliasWinsScenario();
  noRowIdentityScenario();
  fixedCounterRowScenario();
  errnoMatrixScenario();
  aliasWithoutTextScenario();
  sinkCopyScenario();
  if (failures != 0) {
    return 1;
  }
  std::printf(
      "counters_pmu_provider_test PASS: provider over the scripted " "kernel"
                                                                     "\n");
  return 0;
}
