// ============================================================================
// The Linux PMU windows through the scripted kernel (G-19, G-21; Phase D).
//
// The two windows the provider opens, the syscall-mode group window and
// the mapped-page fast window, run their reads, their refusals, and
// their releases over kernel answers the test stands in through the
// seam (`installKernelCalls`), so every arm the coverage gate measures
// is reached on every host, a runner whose `perf_event_open` is
// refused included. The synthetic catalog the windows resolve against
// follows the fixture `counters_linux_pmu_seam_test.cpp` builds; each
// scenario clears the install it stands in before the next one.
//
// Frameworkless check()/fail() convention.
// ============================================================================

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <linux/perf_event.h>

#include "../source/counters/detail/pmu.hpp"
#include "../source/counters/linux_pmu/kernel_calls.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_provider.hpp"

namespace
{

auto failures = int {0};

auto check(const bool ok, const char* sentence) -> void
{
  if (!ok) {
    std::printf("PMU WINDOW TEST FAIL: %s\n", sentence);
    ++failures;
  }
}

using sg::counters::Availability;
using sg::counters::LeafSet;
using sg::counters::PointSink;
using sg::counters::ReadMode;
using sg::counters::Target;
using sg::counters::detail::installKernelCalls;
using sg::counters::detail::PmuDevice;
using sg::counters::detail::PmuEntry;
using sg::counters::detail::pmuOpenFastWindow;
using sg::counters::detail::pmuOpenWindow;
using sg::counters::detail::PmuState;

// The scripted kernel's fixture state. The seam's fields are plain
// function references, so nothing can capture, and the state travels
// through these file-scope holders the wrappers read.
int gNextFd = 0;
std::vector<int> gOpenedFds;
std::vector<int> gClosedFds;
std::vector<std::pair<unsigned long, unsigned long>> gIoctls;
std::vector<std::size_t> gReadSizes;
int gIoctlAnswer = 0;
long (*gReadAnswer)(void* buffer, std::size_t count) = nullptr;
void* gReadBuffer = nullptr;
// The page each open's descriptor maps: the fast window reads one page
// per member, and the descriptor the scripted open answers is the key.
std::map<int, void*> gPages;

/// The scripted set: the open answers fresh descriptors and records
/// them, the map answers the page the scenario registered under the
/// descriptor, the close and the unmap answer success and record, and
/// the read and the ioctl answer what the scenario tuned.
auto scriptedCalls() -> sg::counters::detail::KernelCalls
{
  sg::counters::detail::KernelCalls calls {};
  calls.openEvent = [](void*, int, int, int, unsigned long) -> long
  {
    const int fd = gNextFd++;
    gOpenedFds.push_back(fd);
    return fd;
  };
  calls.mapPage = [](void*, std::size_t, int, int, int fd, long long) -> void*
  {
    const auto found = gPages.find(fd);
    return found != gPages.end() ? found->second : nullptr;
  };
  calls.unmapPage = [](void*, std::size_t) -> int { return 0; };
  calls.readGroup = [](int, void* buffer, std::size_t count) -> long
  {
    gReadSizes.push_back(count);
    gReadBuffer = buffer;
    return gReadAnswer(buffer, count);
  };
  calls.ioctlEvent = [](int, unsigned long request, unsigned long arg) -> int
  {
    gIoctls.emplace_back(request, arg);
    return gIoctlAnswer;
  };
  calls.closeFd = [](int fd) -> int
  {
    gClosedFds.push_back(fd);
    return 0;
  };
  calls.devicesRoot = []() -> std::filesystem::path { return {}; };
  calls.paranoidPath = []() -> std::filesystem::path { return {}; };
  return calls;
}

/// Clears the record the scenarios read between their steps, and hands
/// the real set back so no scenario reads another's scripted answers.
auto resetFixture() -> void
{
  gNextFd = 0;
  gOpenedFds.clear();
  gClosedFds.clear();
  gIoctls.clear();
  gReadSizes.clear();
  gIoctlAnswer = 0;
  gReadAnswer = nullptr;
  gReadBuffer = nullptr;
  gPages.clear();
  installKernelCalls(nullptr);
}

/// One synthetic device the window scenarios resolve against: two
/// countable syscall-mode members, the time pair, and two fast-mode
/// members beside them (FR-021, FR-023).
auto syntheticState() -> PmuState
{
  PmuState state;
  PmuDevice device;
  device.path = "cpu";
  device.type = PERF_TYPE_HARDWARE;
  device.hasTimePair = true;
  const auto entry = [](const std::string& name,
                        const std::vector<std::pair<int, std::uint64_t>>& words,
                        const ReadMode mode)
  {
    PmuEntry one;
    one.name = name;
    one.description = "synthetic " + name;
    one.words = words;
    one.avail = Availability::COUNTABLE;
    one.mode = mode;
    return one;
  };
  device.entries.push_back(
      entry("work", {{0, PERF_COUNT_HW_INSTRUCTIONS}}, ReadMode::SYSCALL));
  device.entries.push_back(
      entry("cycle", {{0, PERF_COUNT_HW_CPU_CYCLES}}, ReadMode::SYSCALL));
  device.entries.push_back(
      entry("fast", {{0, PERF_COUNT_HW_INSTRUCTIONS}}, ReadMode::FAST_RDPMC));
  device.entries.push_back(
      entry("fast2", {{0, PERF_COUNT_HW_CPU_CYCLES}}, ReadMode::FAST_RDPMC));
  for (const char* pair : {"enabled", "running"}) {
    PmuEntry one;
    one.name = pair;
    one.description = "synthetic time pair";
    one.isTimePair = true;
    one.avail = Availability::COUNTABLE;
    one.mode = ReadMode::SYSCALL;
    device.entries.push_back(std::move(one));
  }
  state.devices.push_back(std::move(device));
  return state;
}

auto leafSetOf(std::vector<std::string> addresses) -> LeafSet
{
  return LeafSet {.addresses = std::move(addresses)};
}

auto disclosingLeafSetOf(std::vector<std::string> addresses,
                         std::size_t disclosureColumn) -> LeafSet
{
  return LeafSet {.addresses = std::move(addresses),
                  .disclosureColumn = disclosureColumn};
}

/// The group-read writer the scenarios stand in: the header the kernel
/// reports, the member count first, then the enabled and running pair,
/// then one value per member. The answer's byte count is the payload
/// the writer filled.
auto writeGroupPayload(const std::size_t nr,
                       const std::uint64_t enabled,
                       const std::uint64_t running,
                       const std::vector<std::uint64_t>& values) -> long
{
  auto* words = static_cast<std::uint64_t*>(gReadBuffer);
  words[0] = nr;
  words[1] = enabled;
  words[2] = running;
  for (std::size_t member = 0; member < values.size(); ++member) {
    words[3 + member] = values[member];
  }
  return static_cast<long>((3 + values.size()) * sizeof(std::uint64_t));
}

/// The group window: one granted open per member, the read delivering
/// the header and the values the kernel reports, and the disclosure
/// column naming the action (FR-026, FR-041, FR-047, FR-013).
auto groupWindowReadScenario() -> void
{
  const PmuState state = syntheticState();
  const Target where {};
  auto calls = scriptedCalls();
  resetFixture();
  gReadAnswer = [](void*, std::size_t) -> long
  { return writeGroupPayload(1, 900, 300, {42}); };
  installKernelCalls(&calls);

  auto window = pmuOpenWindow(
      state,
      disclosingLeafSetOf({"cpu/work", "cpu/enabled", "cpu/running"}, 3),
      where);
  check(window != nullptr, "a granted group open returns a window");
  if (window != nullptr) {
    check(gOpenedFds.size() == 1, "one member open, the leader");
    std::uint64_t cells[8] = {};
    PointSink sink {cells, 3, 4, 2, 0};
    window->readPoints(sink);
    sink.checkAction();
    check(cells[0] == 42,
          "the member's point is the value the read carried (FR-026)");
    check(cells[2] == 900 && cells[4] == 300,
          "the enabled and running points are the leader's pair (FR-041)");
    check(cells[6] == static_cast<std::uint64_t>(Availability::COUNTABLE),
          "the disclosure column names a countable action");

    // The thunk the compiled plan holds reads the same group through
    // the scripted kernel (FR-022).
    const auto thunk = window->resolveThunk();
    std::uint64_t slotCells[8] = {};
    PointSink slotSink {slotCells, 3, 4, 2, 0};
    thunk(*window, slotSink);
    slotSink.checkAction();
    check(slotCells[0] == 42, "the direct-call slot reads the same group");
  }

  // The window destructor releases every descriptor the open collected,
  // through the seam the test scripted (FR-013).
  window.reset();
  check(gClosedFds.size() == 1, "the release closes the one member open");
  check(gClosedFds.front() == gOpenedFds.front(),
        "the release closes the descriptor the open answered");
  resetFixture();
}

/// The group window over a read the kernel refuses and a read that
/// answers below the header size: the action reports no count, the
/// zeros stand beside a gap disclosure, and a count is never
/// fabricated (FR-006).
auto groupWindowGappedScenario() -> void
{
  const PmuState state = syntheticState();
  const Target where {};
  auto calls = scriptedCalls();

  // The refused read: the scripted kernel answers -1 with an errno,
  // the way a group read the kernel cannot serve is refused (FR-006).
  gReadAnswer = [](void*, std::size_t) -> long
  {
    errno = EACCES;
    return -1;
  };
  installKernelCalls(&calls);
  auto refused = pmuOpenWindow(
      state,
      disclosingLeafSetOf({"cpu/work", "cpu/enabled", "cpu/running"}, 3),
      where);
  check(refused != nullptr, "a granted open builds the window");
  if (refused != nullptr) {
    std::uint64_t cells[8] = {};
    PointSink sink {cells, 3, 4, 2, 0};
    refused->readPoints(sink);
    sink.checkAction();
    check(cells[0] == 0 && cells[2] == 0 && cells[4] == 0,
          "a refused group read reports the zeros (FR-006)");
    check(cells[6] == static_cast<std::uint64_t>(Availability::GAP),
          "the disclosure column names the gap beside the zeros");
  }
  refused.reset();

  // The short read: the kernel answers below the three header words,
  // and the action is marked the same way (FR-006).
  gReadAnswer = [](void*, std::size_t) -> long
  { return 2 * static_cast<long>(sizeof(std::uint64_t)); };
  auto shortRead = pmuOpenWindow(
      state,
      disclosingLeafSetOf({"cpu/work", "cpu/enabled", "cpu/running"}, 3),
      where);
  check(shortRead != nullptr, "a granted open builds the window");
  if (shortRead != nullptr) {
    std::uint64_t cells[8] = {};
    PointSink sink {cells, 3, 4, 2, 0};
    shortRead->readPoints(sink);
    sink.checkAction();
    check(cells[6] == static_cast<std::uint64_t>(Availability::GAP),
          "a short group read is marked beside the zeros the same way");
  }
  shortRead.reset();
  check(gClosedFds.size() == 2,
        "the two windows released their members on destruction");
  resetFixture();
}

/// The group window over a read whose header reports fewer members
/// than the group carries: the members past the count stand at zero,
/// and no count is fabricated for a member the kernel did not report
/// (FR-026).
auto groupWindowShortHeaderScenario() -> void
{
  const PmuState state = syntheticState();
  const Target where {};
  auto calls = scriptedCalls();
  resetFixture();
  gReadAnswer = [](void*, std::size_t) -> long
  { return writeGroupPayload(1, 700, 700, {5, 9}); };
  installKernelCalls(&calls);
  auto window =
      pmuOpenWindow(state, leafSetOf({"cpu/work", "cpu/cycle"}), where);
  check(window != nullptr, "a two-member granted open returns a window");
  if (window != nullptr) {
    check(gOpenedFds.size() == 2, "two member opens, the leader and one");
    std::uint64_t cells[4] = {};
    PointSink sink {cells, 2, 2, 2, 0};
    window->readPoints(sink);
    sink.checkAction();
    check(cells[0] == 5,
          "the member the header reported carries its value (FR-026)");
    check(cells[1] == 0, "the member past the reported count stands at zero");
  }
  window.reset();
  check(gClosedFds.size() == 2, "both members released on destruction");
  resetFixture();
}

#if defined(__x86_64__) || defined(__i386__)

/// The fast window: the mapped-page read per member, the leader's page
/// disclosing the pair, and a member whose page publishes no
/// `cap_user_rdpmc` bit reporting no count beside the gap (FR-040,
/// FR-041, FR-013).
auto fastWindowReadScenario() -> void
{
  const PmuState state = syntheticState();
  const Target where {};
  auto calls = scriptedCalls();
  resetFixture();
  // Two pages: the leader's, stable and granting the read, and one
  // member page that publishes no capability bit. The pages are plain
  // storage the mapped-page read fields alone (FR-040).
  static perf_event_mmap_page leaderPage;
  static perf_event_mmap_page refusedPage;
  // The leader's page: a stable sequence, the read granted, and the
  // time pair the disclosure takes. The page indexes nothing, so the
  // read withholds the instruction, which is the arm a fixture page
  // can stand in for (FR-040).
  leaderPage = perf_event_mmap_page {};
  leaderPage.lock = 2;  // an even sequence, so the pair stands stable
  leaderPage.cap_user_rdpmc = 1;
  leaderPage.index = 0;
  leaderPage.time_enabled = 900;
  leaderPage.time_running = 300;
  // The second member's page grants no read at all.
  refusedPage = perf_event_mmap_page {};
  refusedPage.cap_user_rdpmc = 0;
  gPages[0] = &leaderPage;
  gPages[1] = &refusedPage;
  installKernelCalls(&calls);

  auto window = pmuOpenFastWindow(
      state,
      disclosingLeafSetOf(
          {"cpu/fast", "cpu/fast2", "cpu/enabled", "cpu/running"}, 4),
      where);
  check(window != nullptr, "a granted fast open returns a window");
  if (window != nullptr) {
    check(gOpenedFds.size() == 2, "two member opens, the leader and one");
    std::uint64_t cells[10] = {};
    PointSink sink {cells, 4, 5, 2, 0};
    window->readPoints(sink);
    sink.checkAction();
    check(cells[2] == 0,
          "a member whose page grants no read reports no count (FR-040)");
    check(cells[4] == 900 && cells[6] == 300,
          "the pair is the leader page's own two counts (FR-041)");
    check(cells[8] == static_cast<std::uint64_t>(Availability::GAP),
          "the disclosure column names the gap the refused member marked");
  }
  window.reset();
  check(gClosedFds.size() == 2, "both member contexts released");
  check(gIoctls.empty(), "the fast read runs no ioctl");
  resetFixture();
}

#endif  // x86

}  // namespace

auto main() -> int
{
  groupWindowReadScenario();
  groupWindowGappedScenario();
  groupWindowShortHeaderScenario();
#if defined(__x86_64__) || defined(__i386__)
  fastWindowReadScenario();
#endif

  if (failures != 0) {
    return 1;
  }
  std::printf(
      "counters_pmu_window_test PASS: windows over the scripted " "kernel\n");
  return 0;
}
