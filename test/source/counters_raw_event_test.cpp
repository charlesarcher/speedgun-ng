// ============================================================================
// Raw event-code counter addresses through the one counters core lookup
// (specs/051-raw-events T004; US1, US2; FR-001 to FR-008, FR-011, FR-015).
//
// A leaf spelled `r` then hex digits names one perf config word on the
// device the address names. Every scenario drives that address through the
// public counters API and the kernel seam, so the counters layer is the
// only place the raw shape can be recognized. The scripted sysfs tree and
// the scripted kernel follow `counters_pmu_provider_test.cpp`, and every
// address names a device of that scripted tree. The scripted kernel grants
// or refuses an open by the config word its attribute carries, so a
// scenario reads the opens of the word it addressed.
//
// The unit is red at runtime until the raw arm lands: the lookups below
// ask for the behavior the feature spec states, and the base tree answers
// them with the catalog route. Each scenario stands its own scripted tree
// and its own `System`, so no scenario reads another scenario's settled
// answer. No scenario waits on a timer, reads a clock, names a cpu the
// scripted tree does not hold, or needs a kernel figure the script does not
// supply (FR-011).
//
// Frameworkless check() convention: every failed check prints one line
// naming the behavior it asked for, and `main` answers nonzero when any
// line printed.
// ============================================================================

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <linux/perf_event.h>
#include <sys/mman.h>
#include <unistd.h>

#include "../source/counters/detail/pmu.hpp"
#include "../source/counters/linux_pmu/kernel_calls.hpp"
#include "../source/counters/linux_pmu/metric_formula.hpp"
#include "../source/counters/linux_pmu/pmu_ident.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_measurement.hpp"
#include "speedgun-ng/counters_pmu.hpp"
#include "speedgun-ng/counters_provider.hpp"
#include "speedgun-ng/counters_system.hpp"

namespace
{

auto failures = int {0};

auto check(const bool ok, const std::string_view sentence) -> void
{
  if (!ok) {
    std::printf("RAW EVENT TEST FAIL: %s\n", std::string(sentence).c_str());
    ++failures;
  }
}

using sg::counters::Availability;
using sg::counters::CatalogEntry;
using sg::counters::compile;
using sg::counters::Dim;
using sg::counters::Expression;
using sg::counters::Object;
using sg::counters::PmuProvider;
using sg::counters::ReadMode;
using sg::counters::Scope;
using sg::counters::System;
using sg::counters::detail::CpuidWords;
using sg::counters::detail::installKernelCalls;
using sg::counters::detail::KernelCalls;
using sg::counters::detail::metricDefinition;
using sg::counters::detail::MetricHost;
using sg::counters::detail::parseMetricRows;
using sg::counters::detail::PmuTableEntry;
using sg::counters::detail::resolveLeafCore;

using Events = Dim<0, 1>;
using Seconds = Dim<1, 0>;

// The scripted core device answers PMU type 4, the number the host's own
// core device publishes and the number the scripted pin of SC-001 holds.
constexpr std::uint32_t kCoreType = 4U;
// The scripted window read: one value and one enabled/running pair per
// sampling action, each one step larger than the last, so a two-point
// window folds a positive delta of exactly this step.
constexpr std::uint64_t kReadStep = 1000U;
constexpr std::uint64_t kPairStart = 5000U;
// The seventeen-digit leaf of FR-005, and the two words a truncated parse
// would answer for it.
constexpr std::string_view kWideLeaf = "r1234567890abcdef1";
constexpr std::uint64_t kWideHighSixteen = 0x1234567890abcdefULL;
constexpr std::uint64_t kWideLowSixteen = 0x234567890abcdef1ULL;

// ---------------------------------------------------------------------------
// The scripted kernel and the scripted sysfs tree
//
// The fixture pair of `counters_pmu_provider_test.cpp`, held here in the
// shape this unit needs: the open records the whole attribute it was
// handed, so a scenario can read the type, the config word and the
// settings of one open, and refuses the open of one config word with one
// errno. The refusal keys on the word, so a scenario holds its raw
// address refused while the catalog aliases of the same device stay
// countable.
// ---------------------------------------------------------------------------

std::filesystem::path gDevicesRoot;
std::filesystem::path gParanoidPath;
// The scripted descriptors start above the standard streams: the probe
// closes the descriptor a granted open answers with, and a descriptor the
// kernel would never answer with keeps the streams whole.
int gNextFd = 100;
bool gRefuseByWord = false;
std::uint64_t gRefuseWord = 0;
int gErrno = EACCES;
// Whether the refusal of a word answers the cpu-targeted open alone. The
// probe pair runs one open per target kind, so a scenario can hold the two
// verdicts apart, the shape the standing merge of the two kinds reads
// (FR-021, FR-022).
bool gRefuseCpuOpenOnly = false;
// The identity no pinned mapfile row matches, so the scripted tree carries
// the kernel's own aliases and nothing a vendored table would add.
CpuidWords gCpuidWords {};

// Every open the scripted kernel saw, in call order: the type and the
// config word for the counts of a scenario, and the whole attribute for
// the settings a window scenario reads.
struct Opened
{
  std::uint32_t type = 0;
  std::uint64_t config = 0;
  perf_event_attr attr {};
};

std::vector<Opened> gOpens;
std::uint64_t gReadValue = 0;
std::uint64_t gReadPair = kPairStart;
KernelCalls gScripted {};

/// The words no pinned mapfile row matches, so no vendored directory
/// enters the scripted tree on any host.
auto noRowWords() -> CpuidWords
{
  return {.leaf0Eax = 0x15U,
          .leaf0Ebx = 0x75536F4EU,  // "Nosu"
          .leaf0Edx = 0x6E655663U,  // "cVen"
          .leaf0Ecx = 0x58726F64U,  // "dorX"
          .leaf1Answered = true,
          .leaf1Eax = 0x30650U};  // family 6, model 5
}

/// Whether the scripted kernel answers this open with the tuned refusal: the
/// open carries the tuned config word and belongs to the target kind the
/// tuned refusal answers. The two probe kinds tell apart by the cpu the seam
/// carries, the pair `leaderPid` of `detail/pmu.hpp` answers: `-1` for the
/// per-task kind and the pinned cpu for the cpu kind.
auto refusalAnswersOpen(const perf_event_attr& attr, const int cpu) -> bool
{
  if (!gRefuseByWord || attr.config != gRefuseWord) {
    return false;
  }
  return !gRefuseCpuOpenOnly || cpu >= 0;
}

/// The scripted set: the open records the attribute and answers a fresh
/// descriptor, the open of the tuned word answers the tuned errno at the
/// tuned target kind, the group read answers the header and one value per
/// member, and the two path functions answer the scripted trees.
auto scriptedCalls() -> KernelCalls
{
  KernelCalls calls {};
  calls.openEvent = [](void* attr, int, int cpu, int, unsigned long) -> long
  {
    const auto* typed = static_cast<const perf_event_attr*>(attr);
    gOpens.push_back(
        Opened {.type = typed->type, .config = typed->config, .attr = *typed});
    if (refusalAnswersOpen(*typed, cpu)) {
      errno = gErrno;
      return -1;
    }
    const int fd = gNextFd++;
    return fd;
  };
  // No scripted open answers a page, so every entry settles on the syscall
  // read mode and the group window path.
  calls.mapPage = [](void*, std::size_t, int, int, int, long long) -> void*
  { return MAP_FAILED; };
  calls.unmapPage = [](void*, std::size_t) -> int { return 0; };
  calls.readGroup = [](int, void* buffer, std::size_t count) -> long
  {
    const auto words = static_cast<std::uint64_t*>(buffer);
    const auto room = count / sizeof(std::uint64_t);
    if (room <= 3U) {
      return -1;
    }
    const auto members = room - 3U;
    words[0] = members;
    words[1] = gReadPair;
    words[2] = gReadPair;
    for (std::size_t member = 0; member < members; ++member) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      words[3U + member] = gReadValue;
    }
    gReadValue += kReadStep;
    gReadPair += kReadStep;
    return static_cast<long>(count);
  };
  calls.ioctlEvent = [](int, unsigned long, unsigned long) -> int { return 0; };
  calls.closeFd = [](int) -> int { return 0; };
  calls.devicesRoot = []() -> std::filesystem::path { return gDevicesRoot; };
  calls.paranoidPath = []() -> std::filesystem::path { return gParanoidPath; };
  calls.cpuidWords = []() -> CpuidWords { return gCpuidWords; };
  return calls;
}

/// Clears the record the scenarios read between their steps and stands the
/// scripted set under the scenario's own `ScenarioScript`.
auto standScript() -> void
{
  gNextFd = 100;
  gRefuseByWord = false;
  gRefuseWord = 0;
  gErrno = EACCES;
  gRefuseCpuOpenOnly = false;
  gCpuidWords = noRowWords();
  gOpens.clear();
  gReadValue = 0;
  gReadPair = kPairStart;
  gScripted = scriptedCalls();
  installKernelCalls(&gScripted);
}

/// The scripted calls and the cleared record one scenario runs over: the
/// scripted set stands for the body of the case that holds this, and the
/// real set stands again after that case's own objects are destroyed, so a
/// window closes through the script that opened it.
struct ScenarioScript
{
  ScenarioScript() { standScript(); }

  ScenarioScript(const ScenarioScript&) = delete;
  auto operator=(const ScenarioScript&) -> ScenarioScript& = delete;
  ScenarioScript(ScenarioScript&&) = delete;
  auto operator=(ScenarioScript&&) -> ScenarioScript& = delete;

  ~ScenarioScript() { installKernelCalls(nullptr); }
};

int gRootSerial = 0;

/// A fresh fixture root, so a scenario owns the tree it walks.
auto freshRoot() -> std::filesystem::path
{
  const auto root = std::filesystem::temp_directory_path()
      / ("raw-event-devices-" + std::to_string(::getpid()) + "-"
         + std::to_string(gRootSerial++));
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root);
  return root;
}

/// One device directory of the fixture tree: a `type` file, a `format`
/// directory with one file per named spec, and an `events` directory with
/// one file per named alias. A `cpumask` file marks the device scoped.
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

/// The core device of the scripted tree: PMU type 4, one `config` field,
/// and the two aliases this unit reads a catalog row and its near-miss
/// candidates from. The raw words the scenarios address stay apart from
/// those aliases, so a count of opens for one word names one address.
auto writeCoreDevice(const std::filesystem::path& root) -> void
{
  writeDevice(
      root,
      "cpu",
      "4",
      {{"config", "config:0-47"}},
      {{"cache-misses", "config=0x2d"}, {"cache-references", "config=0x31"}});
}

/// Writes the paranoid control file the scripted provider reads.
auto writeParanoid(const std::string& text) -> void
{
  gParanoidPath = std::filesystem::temp_directory_path()
      / ("raw-event-paranoid-" + std::to_string(::getpid()));
  std::ofstream file(gParanoidPath);
  file << text;
}

/// Opens one `System` of the scenario's own over the real provider: the
/// provider constructor walks the scripted tree, so the scenario writes
/// that tree and stands the scripted install before this call.
auto openSystem() -> std::unique_ptr<System>
{
  auto system = std::make_unique<System>();
  const auto registered =
      system->registerProvider(std::make_unique<PmuProvider>());
  check(registered.has_value(), "the scripted provider registers its catalog");
  return system;
}

/// The scripted object one scenario addresses.
auto objectOf(System& system,
              const std::string_view path) -> std::optional<Object>
{
  const auto located = system.object(path);
  if (!located.has_value()) {
    check(false, "the scripted device resolves to a catalog object (FR-002)");
    return std::nullopt;
  }
  return *located;
}

/// How many opens carried this config word.
auto openCountForWord(const std::uint64_t word) -> std::size_t
{
  std::size_t count = 0;
  for (const auto& one : gOpens) {
    if (one.config == word) {
      ++count;
    }
  }
  return count;
}

/// True when one open carried this device type with this config word.
auto openedWith(const std::uint32_t type, const std::uint64_t word) -> bool
{
  for (const auto& one : gOpens) {
    if (one.type == type && one.config == word) {
      return true;
    }
  }
  return false;
}

/// The attribute the window of one leaf opens with: the typed route
/// resolves the leaf, the plan opens its window, and the scripted kernel
/// recorded the attribute of that one open.
auto windowAttrOf(const System& system,
                  const Object& object,
                  const std::string& leaf) -> std::optional<perf_event_attr>
{
  const auto counter = object.counter<Events>(leaf);
  if (!counter.has_value()) {
    check(false,
          "the typed route resolves the leaf whose window attribute is read "
          "(FR-003, FR-007)");
    return std::nullopt;
  }
  const Expression<Events> expression(*counter);
  gOpens.clear();
  const auto plan = compile(system, expression);
  if (!plan.has_value()) {
    check(false, "the plan opens a window over the leaf (FR-015)");
    return std::nullopt;
  }
  if (gOpens.size() != 1U) {
    check(false, "one leaf opens one window attribute (FR-003)");
    return std::nullopt;
  }
  return gOpens.front().attr;
}

/// The exclusion and group-read words of an attribute, as the catalog
/// settings of a leaf: every bit `fillAttr` states for a leaf of the
/// device, and the enabled/running pair the group read discloses.
auto settingsOf(const perf_event_attr& attr) -> std::string
{
  std::string text;
  text += "type=" + std::to_string(attr.type);
  text += " size=" + std::to_string(attr.size);
  text += " read_format=" + std::to_string(attr.read_format);
  text += " disabled=" + std::to_string(attr.disabled);
  text += " inherit=" + std::to_string(attr.inherit);
  text += " inherit_stat=" + std::to_string(attr.inherit_stat);
  text += " enable_on_exec=" + std::to_string(attr.enable_on_exec);
  text += " task=" + std::to_string(attr.task);
  text += " mmap=" + std::to_string(attr.mmap);
  text += " mmap_data=" + std::to_string(attr.mmap_data);
  text += " comm=" + std::to_string(attr.comm);
  text += " exclude_user=" + std::to_string(attr.exclude_user);
  text += " exclude_kernel=" + std::to_string(attr.exclude_kernel);
  text += " exclude_hv=" + std::to_string(attr.exclude_hv);
  text += " exclude_idle=" + std::to_string(attr.exclude_idle);
  text += " exclude_host=" + std::to_string(attr.exclude_host);
  text += " exclude_guest=" + std::to_string(attr.exclude_guest);
  return text;
}

/// The catalog names one object holds, in catalog order.
auto namesOf(const std::vector<CatalogEntry>& entries)
    -> std::vector<std::string>
{
  std::vector<std::string> names;
  names.reserve(entries.size());
  for (const auto& one : entries) {
    names.emplace_back(one.name);
  }
  return names;
}

/// The near-miss names the core lookup offers for one misspelled leaf.
auto nearMissesFor(const Object& object,
                   const std::string_view leaf) -> std::vector<std::string>
{
  const auto settled = resolveLeafCore(object, leaf);
  return settled.has_value() ? std::vector<std::string> {}
                             : settled.error().suggestions;
}

// One scenario of this unit: a case function named by the plan table.
using Case = auto (*)() -> void;

/// Names the scenario ahead of its checks, so a failing line reads as the
/// scenario that printed it (T006 records those names).
auto runCase(const std::string_view name, const Case body) -> void
{
  std::printf("case: %s\n", std::string(name).c_str());
  body();
}

}  // namespace

// ---------------------------------------------------------------------------
// The twelve scenarios of the counters plan table, and the scenarios the
// dark branch outcomes of the coverage trace add beside them
// ---------------------------------------------------------------------------

// FR-001, SC-001 scripted pin: the raw address reaches the device's own PMU
// type, and the hex text of the leaf is the config word.
auto rawAcceptRecordsTypeAndConfigWord() -> void
{
  const ScenarioScript script {};
  const auto root = freshRoot();
  writeCoreDevice(root);
  gDevicesRoot = root;
  writeParanoid("2\n");
  const auto system = openSystem();
  const auto object = objectOf(*system, "cpu");
  if (object.has_value()) {
    const auto leaf = resolveLeafCore(*object, "rc0");
    check(leaf.has_value(),
          "the raw address `cpu/rc0` resolves through the core lookup "
          "(FR-001)");
    if (leaf.has_value()) {
      check(leaf->address == "cpu/rc0",
            "the resolved leaf carries the canonical address of the raw "
            "address (FR-001)");
      check(leaf->avail == Availability::COUNTABLE,
            "the scripted probe settles the raw address countable (FR-004)");
    }
    check(openedWith(kCoreType, 0xC0U),
          "the scripted kernel saw an attribute of type 4 and config 0xc0 "
          "(SC-001)");
  }
}

// FR-003: the window of a raw leaf carries the exclusion, group-read and
// time-pair settings a catalog row of the same device carries.
auto rawAttrMatchesCatalogRowAttr() -> void
{
  const ScenarioScript script {};
  const auto root = freshRoot();
  writeCoreDevice(root);
  gDevicesRoot = root;
  writeParanoid("2\n");
  const auto system = openSystem();
  const auto object = objectOf(*system, "cpu");
  if (object.has_value()) {
    const auto raw = windowAttrOf(*system, *object, "r412");
    const auto row = windowAttrOf(*system, *object, "cache-misses");
    if (raw.has_value() && row.has_value()) {
      check(raw->config == 0x412U,
            "the raw attribute carries the hex text as its config word "
            "(FR-001, FR-003)");
      check(raw->type == row->type,
            "the raw attribute names the PMU type of the device (FR-003)");
      check(settingsOf(*raw) == settingsOf(*row),
            "the exclusion, group-read and time-pair words of the raw "
            "attribute equal the catalog row's (FR-003)");
      constexpr std::uint64_t kPairWords =
          PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
      check((raw->read_format & PERF_FORMAT_GROUP) != 0ULL,
            "the raw attribute asks for the group read a catalog row asks "
            "for (FR-003)");
      check((raw->read_format & kPairWords) == kPairWords,
            "the raw attribute discloses the enabled and running pair "
            "(FR-003)");
    }
  }
}

// One row of the errno arm: the leaf, the word its hex text spells, the
// errno the scripted open answers, and the state the one errno table gives.
struct ErrnoRow
{
  const char* leaf;
  std::uint64_t word;
  int error;
  Availability settled;
};

// FR-004, SC-002: each refused open settles the state its errno names, and
// the refused address still answers a settled leaf.
auto rawProbeErrnoArms() -> void
{
  constexpr ErrnoRow kRows[] = {
      {"r1e", 0x1EU, EINVAL, Availability::NOT_ENCODABLE},
      {"r2f", 0x2FU, EOPNOTSUPP, Availability::NOT_ENCODABLE},
      {"r3a", 0x3AU, ENOENT, Availability::NOT_ENCODABLE},
      {"r4b", 0x4BU, EACCES, Availability::PERMISSION_BLOCKED},
  };
  const ScenarioScript script {};
  const auto root = freshRoot();
  writeCoreDevice(root);
  gDevicesRoot = root;
  writeParanoid("2\n");
  const auto system = openSystem();
  const auto object = objectOf(*system, "cpu");
  if (object.has_value()) {
    for (const auto& row : kRows) {
      gRefuseByWord = true;
      gRefuseWord = row.word;
      gErrno = row.error;
      const auto leaf = resolveLeafCore(*object, row.leaf);
      const std::string named = std::string {"the raw address "} + row.leaf
          + " answers a settled leaf (FR-004)";
      check(leaf.has_value(), named);
      check(leaf.has_value() && leaf->avail == row.settled,
            std::string {"the refused open of "} + row.leaf
                + " settles the state of the one errno table (FR-004)");
      gRefuseByWord = false;
    }
  }
}

// FR-004 edge: a permission refusal on a device the kernel scopes by
// cpumask settles on the scope's own refusal.
auto rawProbeScopeRelabel() -> void
{
  const ScenarioScript script {};
  const auto root = freshRoot();
  writeCoreDevice(root);
  writeDevice(root,
              "scoped",
              "9",
              {{"config", "config:0-47"}},
              {{"alias", "config=0x1"}},
              true);
  gDevicesRoot = root;
  writeParanoid("2\n");
  const auto system = openSystem();
  const auto object = objectOf(*system, "scoped");
  if (object.has_value()) {
    gRefuseByWord = true;
    gRefuseWord = 0x500U;
    gErrno = EACCES;
    const auto leaf = resolveLeafCore(*object, "r500");
    check(leaf.has_value(),
          "a raw address on a cpumask device answers a settled leaf (FR-004)");
    if (leaf.has_value()) {
      check(leaf->avail == Availability::SCOPE_REFUSED,
            "the standing scope rule relabels a permission refusal on a "
            "cpumask device (FR-004 edge)");
    }
    check(openCountForWord(0x500U) == 1U,
          "the device scope refuses the per-task kind, so one probe runs for "
          "the address (FR-004 edge)");
    gRefuseByWord = false;
  }
}

// FR-002: the device part of a raw address must name a published device,
// and the refusal costs no probe.
auto absentDeviceRefusesWithoutProbe() -> void
{
  const ScenarioScript script {};
  const auto root = freshRoot();
  writeCoreDevice(root);
  gDevicesRoot = root;
  writeParanoid("2\n");
  const auto system = openSystem();
  // The address of the scenario is `nosuch/r4000`: the device part names a
  // device the scripted tree does not hold, and the lookup never runs.
  const auto located = system->object("nosuch");
  check(!located.has_value(),
        "a device the scripted tree does not hold answers a refusal (FR-002)");
  if (!located.has_value()) {
    check(located.error().message.find("no object at path 'nosuch'")
              != std::string::npos,
          "a raw address on an absent device answers the standing no-object "
          "wording (FR-002)");
  }
  check(openCountForWord(0x4000U) == 0U,
        "the absent device answers ahead of any probe for its raw leaf "
        "(FR-002, FR-005)");
}

// FR-005 library side: a seventeen-digit leaf stays raw-shaped, stays
// refused, and keeps its whole hex text.
auto wideLeafNeverProbes() -> void
{
  const ScenarioScript script {};
  const auto root = freshRoot();
  writeCoreDevice(root);
  gDevicesRoot = root;
  writeParanoid("2\n");
  const auto system = openSystem();
  const auto object = objectOf(*system, "cpu");
  if (object.has_value()) {
    const auto leaf = resolveLeafCore(*object, kWideLeaf);
    check(!leaf.has_value(),
          "a seventeen-digit raw leaf stays refused (FR-005)");
    if (!leaf.has_value()) {
      const auto& message = leaf.error().message;
      check(message.find("has no counter named") == std::string::npos,
            "the wide leaf answers at the leaf-match stage, ahead of the "
            "catalog-name route (FR-005)");
    }
    check(openCountForWord(kWideHighSixteen) == 0U
              && openCountForWord(kWideLowSixteen) == 0U,
          "a seventeen-digit leaf reaches no probe and its hex text reaches "
          "no truncation (FR-005, D-2)");
  }
}

// FR-004, FR-008: one address is probed once, and a later resolve reads the
// settled answer in place of a second probe.
auto probeRunsOncePerAddress() -> void
{
  const ScenarioScript script {};
  const auto root = freshRoot();
  writeCoreDevice(root);
  gDevicesRoot = root;
  writeParanoid("2\n");
  const auto system = openSystem();
  const auto object = objectOf(*system, "cpu");
  if (object.has_value()) {
    const auto first = resolveLeafCore(*object, "r5d3");
    check(first.has_value(),
          "the raw address resolves before its probe count is read (FR-004)");
    const std::size_t paid = openCountForWord(0x5d3U);
    check(paid == 2U,
          "one address pays one probe pair over the two target kinds (FR-004)");
    static_cast<void>(resolveLeafCore(*object, "r5d3"));
    static_cast<void>(resolveLeafCore(*object, "r5d3"));
    check(openCountForWord(0x5d3U) == paid,
          "a repeated resolve of one address opens nothing further (FR-008)");
  }
}

// FR-007: the typed library route takes a raw leaf, and its unit is the
// closed token `none`, hence the events^1 dimension.
auto typedRouteUnitNoneEventsOne() -> void
{
  const ScenarioScript script {};
  const auto root = freshRoot();
  writeCoreDevice(root);
  gDevicesRoot = root;
  writeParanoid("2\n");
  const auto system = openSystem();
  const auto object = objectOf(*system, "cpu");
  if (object.has_value()) {
    const auto events = object->counter<Events>("rc0");
    check(events.has_value(),
          "the typed route resolves a raw address under events^1 (FR-007)");
    if (events.has_value()) {
      check(events->unitToken() == "none",
            "the raw leaf carries the unit token `none` (FR-007)");
      check(events->avail() == Availability::COUNTABLE,
            "the typed route carries the probed state of the raw leaf "
            "(FR-004)");
    }
    const auto wrong = object->counter<Seconds>("rc0");
    check(!wrong.has_value(),
          "a raw address under another dimension answers a refusal (FR-007)");
    if (!wrong.has_value()) {
      check(wrong.error().message.find(
                "has unit 'none' (dimension time^0 x events^1)")
                != std::string::npos,
            "the wrong dimension answers the standing dimension line (FR-007)");
    }
  }
}

// A synthetic event-table row whose formula names a raw-shaped event.
auto rawFormulaRows() -> std::vector<PmuTableEntry>
{
  return {PmuTableEntry {.name = "raw_rate",
                         .description = "a synthetic row naming a raw event",
                         .unit = {},
                         .fields = {},
                         .metricName = "raw_rate",
                         .metricExpr = "r8f",
                         .scaleUnit = {},
                         .group = {},
                         .threshold = {},
                         .fileName = {}}};
}

// FR-007: a metric row whose event name is raw-shaped resolves through the
// same core lookup the counter address uses.
auto formulaResolvesRawEventName() -> void
{
  const ScenarioScript script {};
  const auto root = freshRoot();
  writeCoreDevice(root);
  gDevicesRoot = root;
  writeParanoid("2\n");
  const auto system = openSystem();
  const auto object = objectOf(*system, "cpu");
  if (object.has_value()) {
    const auto rows = rawFormulaRows();
    const auto parsed = parseMetricRows(rows);
    check(parsed.rows.size() == 1U,
          "the synthetic row reaches the parsed metric table (FR-007)");
    const std::vector<PmuTableEntry> events;
    MetricHost host;
    host.system = system.get();
    const auto shown = metricDefinition(parsed, events, host, "raw_rate");
    if (!shown.has_value()) {
      const std::string asked =
          "a metric row whose event name is raw-shaped resolves (FR-007): ";
      check(false, asked + shown.error().message);
    } else {
      check(shown->published,
            "the raw formula row publishes over the scripted host (FR-007)");
      check(shown->core.leaves.size() == 1U,
            "the raw formula spine carries one leaf (FR-007)");
      if (!shown->core.leaves.empty()) {
        check(shown->core.leaves[0].address == "cpu/r8f",
              "the formula leaf is the raw address (FR-007)");
      }
      check(shown->time == 0 && shown->events == 1,
            "the raw formula carries the events^1 dimension (FR-007)");
    }
  }
}

// FR-011, FR-015: the plan takes the raw leaf through the standing gate,
// opens it once, and the fold reads the value the scripted kernel answered.
auto scriptedPlanCompilesAndMeasuresRawLeaf() -> void
{
  const ScenarioScript script {};
  const auto root = freshRoot();
  writeCoreDevice(root);
  gDevicesRoot = root;
  writeParanoid("2\n");
  const auto system = openSystem();
  const auto object = objectOf(*system, "cpu");
  if (object.has_value()) {
    const auto counter = object->counter<Events>("r2b0");
    check(counter.has_value(),
          "the typed route resolves the raw leaf the plan compiles (FR-007)");
    if (counter.has_value()) {
      const Expression<Events> expression(*counter);
      gOpens.clear();
      const auto plan = compile(*system, expression);
      check(plan.has_value(),
            "compileCore takes the raw leaf through the standing gate "
            "(FR-015)");
      if (plan.has_value()) {
        check(openCountForWord(0x2b0U) == 1U,
              "the plan opens the raw leaf once (FR-015)");
        Scope scope(*plan);
        scope.start();
        scope.finish();
        const auto folded = scope.metric(expression);
        check(folded.availability == Availability::COUNTABLE,
              "the folded window discloses the countable state (FR-011)");
        check(folded.value > 0.0,
              "the fold answers the value the scripted kernel read for the "
              "raw leaf (FR-011)");
      }
    }
  }
}

// FR-015: the raw-entry table feeds the lookup, the plan and the window,
// and the published listings answer as they did before any raw resolve.
auto listingsNeverReadRawTable() -> void
{
  const ScenarioScript script {};
  const auto root = freshRoot();
  writeCoreDevice(root);
  gDevicesRoot = root;
  writeParanoid("2\n");
  const auto system = openSystem();
  const auto object = objectOf(*system, "cpu");
  if (object.has_value()) {
    const auto before = object->counters();
    const auto seeded = namesOf(before);
    // The misspelled alias drives the near-miss route of FR-008, and the
    // misspelling is the input the route answers.
    const auto nearBefore = nearMissesFor(*object, "cache-mise");
    const auto raw = resolveLeafCore(*object, "r77b");
    check(raw.has_value(),
          "a raw resolve runs before these listing reads (FR-015)");
    const auto after = object->counters();
    check(after.size() == before.size(),
          "a raw resolve adds no leaf to the seeded count (FR-015)");
    check(namesOf(after) == seeded,
          "the catalog shape answers as it did before the raw resolve "
          "(FR-015)");
    const auto nearAfter = nearMissesFor(*object, "cache-mise");
    check(nearAfter == nearBefore,
          "the near-miss candidates answer as they did before the raw "
          "resolve (FR-015)");
    check(!nearAfter.empty(),
          "a name that is not raw-shaped keeps the catalog route (FR-007)");
  }
}

// FR-008 and the identity edge: the address string is the identity, so one
// spelling twice is one leaf and two spellings are two leaves.
auto duplicateSpellsDistinctAddresses() -> void
{
  const ScenarioScript script {};
  const auto root = freshRoot();
  writeCoreDevice(root);
  gDevicesRoot = root;
  writeParanoid("2\n");
  const auto system = openSystem();
  const auto object = objectOf(*system, "cpu");
  if (object.has_value()) {
    const auto first = resolveLeafCore(*object, "r43");
    const auto again = resolveLeafCore(*object, "r43");
    const auto padded = resolveLeafCore(*object, "r0043");
    check(first.has_value() && again.has_value() && padded.has_value(),
          "each raw spelling resolves through the core lookup (FR-001)");
    if (first.has_value() && again.has_value()) {
      check(first->address == again->address,
            "one address written twice names one address (FR-008)");
    }
    if (first.has_value() && padded.has_value()) {
      check(first->address != padded->address,
            "the padded and the short spelling name two addresses (FR-008)");
    }
    const auto one = object->counter<Events>("r43");
    const auto twice = object->counter<Events>("r43");
    const auto other = object->counter<Events>("r0043");
    if (one.has_value() && twice.has_value() && other.has_value()) {
      const auto repeated =
          Expression<Events>(*one) + Expression<Events>(*twice);
      check(repeated.core.leaves.size() == 1U,
            "one address written twice holds one leaf of the spine (FR-008)");
      const auto distinct =
          Expression<Events>(*one) + Expression<Events>(*other);
      check(distinct.core.leaves.size() == 2U,
            "the two spellings hold two leaves of the spine (FR-008)");
    }
  }
}

// FR-004, FR-021, FR-022: on a cpumask device the standing scope rule
// relabels a permission refusal as its own refusal, and an encoding refusal
// keeps the state the one errno table names for it. The granted word is the
// control: the device counts it on the one kind its scope admits.
auto rawProbeScopeEncodingArms() -> void
{
  constexpr ErrnoRow kRows[] = {
      {"r81e", 0x81EU, EINVAL, Availability::NOT_ENCODABLE},
      {"r82f", 0x82FU, EOPNOTSUPP, Availability::NOT_ENCODABLE},
      {"r83a", 0x83AU, ENOENT, Availability::NOT_ENCODABLE},
  };
  const ScenarioScript script {};
  const auto root = freshRoot();
  writeCoreDevice(root);
  writeDevice(root,
              "scopeenc",
              "11",
              {{"config", "config:0-47"}},
              {{"alias", "config=0x3"}},
              true);
  gDevicesRoot = root;
  writeParanoid("2\n");
  const auto system = openSystem();
  const auto object = objectOf(*system, "scopeenc");
  if (object.has_value()) {
    for (const auto& row : kRows) {
      gRefuseByWord = true;
      gRefuseWord = row.word;
      gErrno = row.error;
      const auto leaf = resolveLeafCore(*object, row.leaf);
      check(leaf.has_value(),
            std::string {"the raw address "} + row.leaf
                + " answers a settled leaf on a cpumask device (FR-004)");
      check(leaf.has_value() && leaf->avail == row.settled,
            std::string {"the encoding refusal of "} + row.leaf
                + " keeps the state of the one errno table, which the scope "
                  "relabels for a permission refusal alone (FR-004, FR-021)");
      gRefuseByWord = false;
    }
    const auto counted = resolveLeafCore(*object, "r8c4");
    check(counted.has_value() && counted->avail == Availability::COUNTABLE,
          "the raw word the scripted kernel grants on a cpumask device "
          "settles countable (FR-004, FR-021)");
    check(openCountForWord(0x8c4U) == 1U,
          "the device scope admits the cpu kind alone, so the granted word "
          "pays one probe (FR-004, FR-021)");
  }
}

// FR-003, FR-021: the read mode of a raw leaf is the one device-wide verdict
// of its device. A scripted device whose only seeded event the seed probe
// refused holds no countable entry to read that verdict from, so the settled
// raw leaf answers the syscall fallback of the read mode.
auto rawModeAnswersSyscallWithoutCountableSeed() -> void
{
  const ScenarioScript script {};
  const auto root = freshRoot();
  writeCoreDevice(root);
  writeDevice(root,
              "noseed",
              "8",
              {{"config", "config:0-47"}},
              {{"onlyevent", "config=0x21"}});
  gDevicesRoot = root;
  writeParanoid("2\n");
  // The refusal stands over the load of the scripted tree, so the provider
  // seeds the device with its one event already refused.
  gRefuseByWord = true;
  gRefuseWord = 0x21U;
  gErrno = EACCES;
  const auto system = openSystem();
  gRefuseByWord = false;
  const auto object = objectOf(*system, "noseed");
  if (object.has_value()) {
    const auto seeded = resolveLeafCore(*object, "onlyevent");
    check(seeded.has_value()
              && seeded->avail == Availability::PERMISSION_BLOCKED,
          "the seed probe refused the scripted device's only event, so the "
          "device holds no countable entry (FR-021)");
    const auto leaf = resolveLeafCore(*object, "r8d5");
    check(leaf.has_value() && leaf->avail == Availability::COUNTABLE,
          "the scripted kernel counts the raw word on the device holding no "
          "countable seeded event (FR-004)");
    check(leaf.has_value() && leaf->mode == ReadMode::SYSCALL,
          "the raw leaf of a device holding no countable entry answers the "
          "syscall read mode (FR-003)");
  }
}

// FR-004, FR-021, FR-022: the two probe kinds answer apart. The per-task
// probe counts the address while the cpu-targeted probe refuses it, and the
// grant settles the address countable on the kind the kernel counted.
auto rawCountableWhenCpuKindRefuses() -> void
{
  const ScenarioScript script {};
  const auto root = freshRoot();
  writeCoreDevice(root);
  gDevicesRoot = root;
  writeParanoid("2\n");
  const auto system = openSystem();
  const auto object = objectOf(*system, "cpu");
  if (object.has_value()) {
    gRefuseByWord = true;
    gRefuseWord = 0x9a5U;
    gErrno = EACCES;
    gRefuseCpuOpenOnly = true;
    const auto leaf = resolveLeafCore(*object, "r9a5");
    gRefuseByWord = false;
    gRefuseCpuOpenOnly = false;
    check(leaf.has_value(),
          "the raw address the per-task probe counted answers a settled leaf "
          "(FR-004)");
    check(leaf.has_value() && leaf->avail == Availability::COUNTABLE,
          "the per-task grant settles the address countable over the refusal "
          "of the cpu kind (FR-004, FR-021, FR-022)");
    check(openCountForWord(0x9a5U) == 2U,
          "the address pays its probe pair: the grant of one kind and the "
          "refusal of the other (FR-004)");
  }
}

// FR-001, FR-006, FR-008: an upper-case hex run is raw-shaped, stands in the
// settled leaf as it was typed, and reaches the config word its lower-case
// spelling reaches.
auto upperCaseHexLeafSharesItsLowerCaseWord() -> void
{
  const ScenarioScript script {};
  const auto root = freshRoot();
  writeCoreDevice(root);
  gDevicesRoot = root;
  writeParanoid("2\n");
  const auto system = openSystem();
  const auto object = objectOf(*system, "cpu");
  if (object.has_value()) {
    const auto typed = resolveLeafCore(*object, "rAB7");
    const auto lowered = resolveLeafCore(*object, "rab7");
    check(typed.has_value(),
          "a raw leaf spelling its hex run in upper case resolves through the "
          "core lookup (FR-001)");
    check(lowered.has_value(),
          "the lower-case spelling of one run resolves too (FR-001)");
    if (typed.has_value() && lowered.has_value()) {
      check(typed->name == "rAB7" && typed->address == "cpu/rAB7",
            "the upper-case spelling stands in the settled leaf as typed "
            "(FR-001, FR-006)");
      check(typed->address != lowered->address,
            "the two spellings name two addresses (FR-008)");
      check(typed->avail == Availability::COUNTABLE
                && lowered->avail == Availability::COUNTABLE,
            "each spelling settles countable on the scripted kernel (FR-004)");
    }
    check(openedWith(kCoreType, 0xab7U),
          "the scripted kernel saw the config word the upper-case run spells, "
          "the word the lower-case spelling spells (SC-001)");
    check(openCountForWord(0xab7U) == 4U,
          "two addresses over one config word pay one probe pair each "
          "(FR-004, FR-008)");
  }
}

// FR-002, R-06: a raw address at a path the PMU provider holds no device at
// answers the standing no-object wording and costs no probe, both where the
// settled device state holds no device for that path and where no provider
// published a state at all.
auto machinePathRefusesRawWithoutProbe() -> void
{
  const ScenarioScript script {};
  const auto root = freshRoot();
  writeCoreDevice(root);
  gDevicesRoot = root;
  writeParanoid("2\n");
  // The first step holds a system of no provider at all, so the settled
  // device state the synthesis reads stands nowhere.
  {
    System bare {};
    const auto bareRoot = objectOf(bare, "machine");
    if (bareRoot.has_value()) {
      const auto leaf = resolveLeafCore(*bareRoot, "rb1c");
      check(!leaf.has_value(),
            "a raw address where no settled device state stands answers a "
            "refusal (FR-002)");
      if (!leaf.has_value()) {
        check(leaf.error().message.find("no object at path 'machine'")
                  != std::string::npos,
              "the refusal of that path answers the standing no-object "
              "wording (FR-002)");
      }
    }
  }
  // The second step holds the PMU provider, so a settled device state stands
  // and the machine root of the tree names no device of that state.
  const auto system = openSystem();
  const auto pmuRoot = objectOf(*system, "machine");
  if (pmuRoot.has_value()) {
    const auto leaf = resolveLeafCore(*pmuRoot, "rb2d");
    check(!leaf.has_value(),
          "a raw address at a path the settled device state holds no device "
          "for answers a refusal (FR-002)");
    if (!leaf.has_value()) {
      check(leaf.error().message.find("no object at path 'machine'")
                != std::string::npos,
            "the settled state answers that path with the same no-object "
            "wording (FR-002)");
    }
  }
  check(openCountForWord(0xb1cU) == 0U && openCountForWord(0xb2dU) == 0U,
        "a raw address at a path holding no PMU device reaches no probe "
        "(FR-002, FR-005)");
}

// FR-004, R-07: the device-level figures a raw row takes come from an event
// leaf of its node, and the standing partition skips the enabled/running
// control names. A scripted node whose only leaves carry those two names
// states them on no leaf, so the row keeps what the synthesis settled.
auto rawStampSkipsControlLeafNames() -> void
{
  const ScenarioScript script {};
  const auto root = freshRoot();
  writeCoreDevice(root);
  // Both control names stand as scripted aliases over one config word, and
  // the refusal of that word over the tree load settles the device with no
  // countable entry. A countable device takes the provider's own pair of
  // these names on top, and the tree refuses one object holding two leaves
  // of one name (FR-008).
  writeDevice(root,
              "pairctl",
              "12",
              {{"config", "config:0-47"}},
              {{"enabled", "config=0x3e"}, {"running", "config=0x3e"}});
  gDevicesRoot = root;
  writeParanoid("2\n");
  gRefuseByWord = true;
  gRefuseWord = 0x3eU;
  gErrno = EACCES;
  const auto system = openSystem();
  gRefuseByWord = false;
  const auto object = objectOf(*system, "pairctl");
  if (object.has_value()) {
    const auto enabled = resolveLeafCore(*object, "enabled");
    const auto running = resolveLeafCore(*object, "running");
    check(enabled.has_value() && running.has_value(),
          "the scripted control names stand as leaves of the node (FR-008)");
    check(enabled.has_value()
              && enabled->address == "pairctl/enabled"
              && enabled->avail == Availability::PERMISSION_BLOCKED
              && running.has_value() && running->address == "pairctl/running"
              && running->avail == Availability::PERMISSION_BLOCKED,
          "each control name resolves to its own seeded leaf, and the seed "
          "probe refused both (FR-004, FR-008)");
    const auto leaf = resolveLeafCore(*object, "rd17");
    check(leaf.has_value() && leaf->avail == Availability::COUNTABLE,
          "the raw word settles countable on a node whose only leaves carry "
          "control names (FR-004)");
    check(leaf.has_value() && leaf->mode == ReadMode::SYSCALL
              && leaf->scaled == false,
          "the raw row keeps the device-level figures the synthesis settled, "
          "which the partition reaches no event leaf to state (R-07)");
    check(openCountForWord(0xd17U) == 2U,
          "the raw address pays its probe pair over the two kinds (FR-004)");
  }
}

// FR-001, FR-007: a bare `r` carries no digit run, and a run with a non-hex
// tail spells no config word. Both answer the catalog route, and neither
// reaches a probe.
auto unshapedLeafShapesNeverProbe() -> void
{
  const ScenarioScript script {};
  const auto root = freshRoot();
  writeCoreDevice(root);
  gDevicesRoot = root;
  writeParanoid("2\n");
  const auto system = openSystem();
  const auto object = objectOf(*system, "cpu");
  if (object.has_value()) {
    const auto bare = resolveLeafCore(*object, "r");
    check(!bare.has_value(),
          "a bare `r` names no raw code and answers a refusal (FR-001)");
    if (!bare.has_value()) {
      check(bare.error().message.find("has no counter named 'r'")
                != std::string::npos,
            "a bare `r` answers the standing unknown-leaf wording (FR-007, "
            "FR-008)");
    }
    const auto tail = resolveLeafCore(*object, "r4g");
    check(!tail.has_value(),
          "a digit run with a non-hex tail names no raw code and answers a "
          "refusal (FR-001)");
    if (!tail.has_value()) {
      check(tail.error().message.find("has no counter named 'r4g'")
                != std::string::npos,
            "a non-hex tail answers the standing unknown-leaf wording "
            "(FR-007, FR-008)");
    }
    check(openCountForWord(0U) == 0U && openCountForWord(0x4U) == 0U,
          "neither shape reaches a probe, so no config word is formed for it "
          "(FR-001, FR-005)");
  }
}

auto main() -> int
{
  runCase("rawAcceptRecordsTypeAndConfigWord",
          &rawAcceptRecordsTypeAndConfigWord);
  runCase("rawAttrMatchesCatalogRowAttr", &rawAttrMatchesCatalogRowAttr);
  runCase("rawProbeErrnoArms", &rawProbeErrnoArms);
  runCase("rawProbeScopeRelabel", &rawProbeScopeRelabel);
  runCase("absentDeviceRefusesWithoutProbe", &absentDeviceRefusesWithoutProbe);
  runCase("wideLeafNeverProbes", &wideLeafNeverProbes);
  runCase("probeRunsOncePerAddress", &probeRunsOncePerAddress);
  runCase("typedRouteUnitNoneEventsOne", &typedRouteUnitNoneEventsOne);
  runCase("formulaResolvesRawEventName", &formulaResolvesRawEventName);
  runCase("scriptedPlanCompilesAndMeasuresRawLeaf",
          &scriptedPlanCompilesAndMeasuresRawLeaf);
  runCase("listingsNeverReadRawTable", &listingsNeverReadRawTable);
  runCase("duplicateSpellsDistinctAddresses",
          &duplicateSpellsDistinctAddresses);
  runCase("rawProbeScopeEncodingArms", &rawProbeScopeEncodingArms);
  runCase("rawModeAnswersSyscallWithoutCountableSeed",
          &rawModeAnswersSyscallWithoutCountableSeed);
  runCase("rawCountableWhenCpuKindRefuses", &rawCountableWhenCpuKindRefuses);
  runCase("upperCaseHexLeafSharesItsLowerCaseWord",
          &upperCaseHexLeafSharesItsLowerCaseWord);
  runCase("machinePathRefusesRawWithoutProbe",
          &machinePathRefusesRawWithoutProbe);
  runCase("rawStampSkipsControlLeafNames", &rawStampSkipsControlLeafNames);
  runCase("unshapedLeafShapesNeverProbe", &unshapedLeafShapesNeverProbe);
  if (failures != 0) {
    std::printf("counters_raw_event_test: %d FAILED CHECKS\n", failures);
    return 1;
  }
  std::printf("counters_raw_event_test PASS: raw addresses over the scripted "
              "kernel and sysfs trees\n");
  return 0;
}
