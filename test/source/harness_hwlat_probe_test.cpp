// ============================================================================
// T012, T013, T014, T015 (specs/048-hwlat-probe): the probe path of the
// `hwlat` method. One read of the SMI counter stands beside each kept gap,
// the change of the span that ends at a gap marks that gap, and the two
// window deltas and the marked count fold onto that cpu's figures (FR-012 to
// FR-017, D-7, P-3, P-5). The gap series scripts through the reader-install
// seam (R-02), and the counter and the interrupt text stand in fake files
// under the file-root seam, so a row holds on a host that holds no counter
// (FR-001, FR-032). The scripted writes ride the reader's own calls, so a
// rise stands between two planned reads and beside no race (FR-039).
// Frameworkless check()/fail() convention, the way harness_noise_test
// carries it.
// ============================================================================

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <sched.h>
#include <unistd.h>

#include "../../source/detail/proc_root.hpp"
#include "detail/jitter_probe.hpp"
#include "detail/latency_counters.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_fake.hpp"
#include "speedgun-ng/counters_system.hpp"

namespace
{

int failures = 0;

auto fail(const char* what) -> void
{
  ++failures;
  std::printf("FAILED: %s\n", what);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// The width of one counter figure and the address of the counter inside the
// node, both from the unit that reads them (FR-007).
constexpr std::uint64_t kNodeAddress = sg::detail::kSmiCountAddress;

// The scripted series: every read steps by `kStep`, and a scripted gap pass
// adds `kJump` (FR-039). The explicit threshold stands above the in-pass
// step and below the jump, so a gap lands on a scripted pass and on no
// other (FR-007).
constexpr std::uint64_t kStep = 1000;
constexpr std::uint64_t kJump = 200000;

// The width of one scripted gap: the jump rides on the step of the same
// pass, so the kept record carries the sum of both (FR-001, FR-039).
constexpr std::uint64_t kGapWidth = kStep + kJump;
constexpr std::uint64_t kThresholdNs = 20000;

// The passes of one run, counted in reader calls. The measurement pass
// covers the first 1,024 calls (E-05), the read behind the latch opens the
// run span, and the loop's first pass stands one call later (FR-006).
constexpr std::uint64_t kPassCalls = sg::detail::kPassReads;
constexpr std::uint64_t kSpanOpeningCall = kPassCalls + 1;
constexpr std::uint64_t kFirstPassCall = kSpanOpeningCall + 1;
constexpr std::uint64_t kGapOneCall = kFirstPassCall + 4;
constexpr std::uint64_t kGapTwoCall = kFirstPassCall + 14;
constexpr std::uint64_t kGapThreeCall = kFirstPassCall + 24;

// The pre-check window of the `both` scenarios: one millisecond of scripted
// stamps steps in 1,000 calls, and the scripted gap at the window's last
// call ends it (FR-014). The jump carries the window span 200 calls early,
// so the window's last call, and the run span's opening call with it, stand
// where the arithmetic puts them (FR-016).
constexpr std::uint32_t kPrecheckMs = 1;
constexpr std::uint64_t kPhaseCalls =
    static_cast<std::uint64_t>(kPrecheckMs) * 1000000ULL / kStep;
constexpr std::uint64_t kJumpCalls = kJump / kStep;
constexpr std::uint64_t kPhaseGapCall = kPassCalls + kPhaseCalls - kJumpCalls;
constexpr std::uint64_t kBothOpeningCall = kPhaseGapCall + 1;
constexpr std::uint64_t kBothFirstPassCall = kBothOpeningCall + 1;
constexpr std::uint64_t kBothGapCall = kBothFirstPassCall + 4;

// The records one run pre-sizes: the gap ring and the change ring carry this
// many entries, and the drop rows fill a shorter pair (FR-009, FR-012).
constexpr std::uint32_t kGapCount = 8;
constexpr std::uint32_t kShortGapCount = 2;

// The counter figures one scenario plants and rewrites (FR-007, FR-012).
constexpr std::uint64_t kOpeningCount = 7;
constexpr std::uint64_t kRiseCount = 8;
constexpr std::uint64_t kSecondRiseCount = 10;
constexpr std::uint64_t kClosingCount = 12;
constexpr std::uint64_t kFlatCount = 9;
constexpr std::uint64_t kFallingCount = 5;
constexpr std::uint64_t kNmiOpening = 100;
constexpr std::uint64_t kNmiRise = 104;

// The vendor texts of the gate: the one that documents the counter, and one
// that does not (FR-010, D-4).
constexpr const char* kIntelText =
    "processor\t: 0\nvendor_id\t: GenuineIntel\nmodel name\t: x\n";
constexpr const char* kOtherVendorText =
    "processor\t: 0\nvendor_id\t: AuthenticAMD\nmodel name\t: y\n";

// The `proc/interrupts` text of a kernel that names no `NMI` row: the labeled
// row of another kind stands, and the detector figure stands absent (FR-011).
constexpr const char* kNoNmiRowText = "  8:  10   IO-APIC  8-edge  timer\n";

// The cpu indexes the scenarios pin probe threads to, taken from the process
// affinity mask, so no cpu number stands fixed in this file (FR-040).
auto pickCpus(std::size_t count) -> std::vector<std::uint32_t>
{
  cpu_set_t mask {};
  CPU_ZERO(&mask);
  if (sched_getaffinity(0, sizeof(mask), &mask) != 0) {
    fail("the process affinity mask is unreadable (FR-040)");
  }
  check(CPU_COUNT(&mask) > 0, "the affinity mask holds cpus (FR-040)");
  std::vector<std::uint32_t> cpus;
  for (std::size_t index = 0; index < CPU_SETSIZE && cpus.size() < count;
       ++index)
  {
    if (CPU_ISSET(index, &mask)) {
      cpus.push_back(static_cast<std::uint32_t>(index));
    }
  }
  check(cpus.size() == count,
        "the mask holds the cpus the scenario asks for (FR-040)");
  return cpus;
}

// The scripted provider: one FakeProvider declaring the machine/tsc leaf at
// ReadMode::FAST_TSC (R-02), registered before the first catalog lookup, so
// the availability gate answers to the scripted leaf and the scenarios drive
// the loop through the reader seam (R-02, R-11). The two time leaves stand
// scripted as well, because the whole-run row reaches the entry point, and the
// batch it runs reads them.
auto scriptedProvider() -> sg::counters::FakeProvider&
{
  static auto* provider = []
  {
    auto owned = std::make_unique<sg::counters::FakeProvider>();
    auto& ref = *owned;
    // specs/045-merge-fixes phase 16 (FR-039): the rate pair's second
    // leaf stands at the tsc leaf's own step, so the measured rate
    // answers one tick per ns and the projection is the identity the
    // scenarios' figures read.
    ref.addCounter("machine", "monotonic", "nanoseconds", "host time");
    ref.setPoints("machine", "monotonic", {0}, 1);
    ref.addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
    ref.setPoints("machine", "thread_cpu", {0}, 2000);
    ref.addCounter("machine",
                   "tsc",
                   "none",
                   "raw tick counter",
                   sg::counters::Availability::COUNTABLE,
                   sg::counters::ReadMode::FAST_TSC);
    ref.setPoints("machine", "tsc", {0}, 1);
    const auto registered =
        sg::counters::System::local().registerProvider(std::move(owned));
    check(registered.has_value(), "the scripted tsc provider registers (R-02)");
    return &ref;
  }();
  return *provider;
}

// The node path of one cpu under the file root, the path spec D-3 names
// (FR-007).
auto nodePath(std::uint32_t cpu) -> std::string
{
  return "dev/cpu/" + std::to_string(cpu) + "/msr";
}

// The file-root seam of one scenario: a fresh tree the probe's reads resolve
// under, installed for the scenario and cleared when it ends, the way the
// noise suite carries its tracing root (FR-001, FR-032).
class HwlatRootGuard
{
public:
  HwlatRootGuard()
  {
    static int sequence = 0;
    m_root = std::filesystem::path("harness_hwlat_probe_root_")
        / ("pid" + std::to_string(::getpid()) + "-"
           + std::to_string(sequence++));
    std::filesystem::remove_all(m_root);
    std::filesystem::create_directories(m_root);
    sg::detail::installTestProcRoot(m_root);
  }

  ~HwlatRootGuard()
  {
    sg::detail::installTestProcRoot(std::nullopt);
    std::error_code removeFailed;
    std::filesystem::remove_all(m_root, removeFailed);
  }

  HwlatRootGuard(const HwlatRootGuard&) = delete;
  auto operator=(const HwlatRootGuard&) -> HwlatRootGuard& = delete;

  auto root() const -> const std::filesystem::path& { return m_root; }

private:
  std::filesystem::path m_root;
};

// One file of the fake tree written whole, with the rights the row names
// (FR-032).
auto plant(const std::filesystem::path& root,
           const std::string& relative,
           const std::string& bytes,
           std::filesystem::perms rights) -> void
{
  const auto path = root / std::filesystem::path(relative);
  std::filesystem::create_directories(path.parent_path());
  std::ofstream file(path, std::ios::binary | std::ios::trunc);
  check(static_cast<bool>(file), "the fake file opens for writing (T012)");
  file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  file.close();
  std::error_code rightsFailed;
  std::filesystem::permissions(
      path, rights, std::filesystem::perm_options::replace, rightsFailed);
  check(!rightsFailed, "the fake file takes the rights the row plants (T012)");
}

// The node image of one counter figure: padding up to the counter address,
// then the figure as the unit reads it, least significant byte first
// (FR-007).
auto nodeImage(std::uint64_t value, std::size_t width) -> std::string
{
  std::string bytes(kNodeAddress, '\0');
  for (std::size_t index = 0; index < width; ++index) {
    bytes.push_back(static_cast<char>((value >> (8U * index)) & 0xffULL));
  }
  return bytes;
}

// The rights of a node the probe may read (FR-007).
constexpr auto kNodeRights = std::filesystem::perms::owner_read
    | std::filesystem::perms::owner_write | std::filesystem::perms::group_read;

// The rights of a node the process may not read, the shape the no-rights
// row plants (FR-011).
constexpr auto kNoRights = std::filesystem::perms::none;

// The `NMI` row of `proc/interrupts`: one row, one figure per column, the
// columns standing in the order the standing reader counts them (FR-009).
auto interruptsText(std::size_t columns, std::uint64_t figure) -> std::string
{
  std::string text = "NMI:";
  for (std::size_t column = 0; column < columns; ++column) {
    text += "  " + std::to_string(figure);
  }
  text += "   NMI       performance-metrics\n";
  return text;
}

// The highest cpu column one scenario's text has to carry (FR-009).
auto columnCount(std::span<const std::uint32_t> cpus) -> std::size_t
{
  std::uint32_t highest = 0;
  for (const auto cpu : cpus) {
    highest = cpu > highest ? cpu : highest;
  }
  return static_cast<std::size_t>(highest) + 1;
}

// One whole node of one cpu, written whole: the shape the closing-edge write
// of a row takes (FR-012).
auto writeNodeWhole(const std::filesystem::path& root,
                    std::uint32_t cpu,
                    std::uint64_t value) -> void
{
  const auto path = root / std::filesystem::path(nodePath(cpu));
  std::ofstream file(path, std::ios::binary);
  const std::string bytes = nodeImage(value, sg::detail::kSmiCountBytes);
  file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  file.close();
  check(static_cast<bool>(file), "the rewritten node stands written (T012)");
}

// One whole `proc/interrupts` carrying the text a row names. The scripted
// path reads the answer itself, so no check stands on the probe thread: a
// failed write reaches the flag beside the reader (FR-009, FR-011).
auto writeInterruptsText(const std::filesystem::path& root,
                         const std::string& text) -> bool
{
  std::ofstream file(root / "proc/interrupts", std::ios::binary);
  file.write(text.data(), static_cast<std::streamsize>(text.size()));
  file.close();
  return static_cast<bool>(file);
}

// One whole `proc/interrupts` of one cpu set, written whole (FR-009).
auto writeInterruptsWhole(const std::filesystem::path& root,
                          std::size_t columns,
                          std::uint64_t figure) -> bool
{
  return writeInterruptsText(root, interruptsText(columns, figure));
}

// The script of the running scenario: the calls whose stamp jumps, the calls
// that rewrite this cpu's node, the calls that rewrite the interrupt text,
// and the tree they stand in. The test thread writes these before the start,
// and the probe threads read them behind the thread's creation, so no write
// races a read (FR-039).
std::vector<std::uint64_t> gapCalls;
std::vector<std::pair<std::uint64_t, std::uint64_t>> nodeWrites;
std::vector<std::pair<std::uint64_t, std::uint64_t>> interruptsWrites;
// The calls that replace the interrupt text whole: the shape that leaves one
// source with no row to read at a later edge (FR-011).
std::vector<std::pair<std::uint64_t, std::string>> interruptsTexts;
// The calls at which one cpu's node figure appears in the tree, with the bytes
// the node then answers. The item opens its node once at its start, so the path
// has to stand for the figure to reach the reads of the window: the list writes
// whole bytes into the standing path, which brings a figure to a node that
// answered none (FR-011, D-13). A write that failed reaches the flag beside the
// reader, so a row reads the premise it planted (FR-032, FR-039).
std::vector<std::pair<std::uint64_t, std::string>> nodeAppears;
std::atomic<bool> scriptCreateFailed {false};
std::filesystem::path scriptRoot;
std::size_t scriptColumns = 1;
std::atomic<bool> scriptWriteFailed {false};

// The reader's call counts: the per-thread stamp series, the ticket one
// thread takes at its first call, and the call count each thread mirrors to
// the slot its ticket names, so a wait reads the calls of a running loop
// with no sleep and no clock (FR-039).
constexpr std::size_t kMaxScriptedCpus = 4;

struct ThreadSeries
{
  std::uint64_t calls = 0;
  std::uint64_t stamp = 0;
  std::uint64_t ticket = 0;
};

thread_local ThreadSeries threadSeries;
std::atomic<std::uint64_t> seriesTicket {0};
std::atomic<std::uint64_t> seriesCalls[kMaxScriptedCpus] {};

// The counter figure of one node, written at the counter address and nowhere
// else, so a scripted rise leaves every other byte of the node alone
// (FR-007, FR-012).
auto writeCounterAt(const std::filesystem::path& path,
                    std::uint64_t value) noexcept -> bool
{
  const int descriptor = ::open(path.c_str(), O_RDWR | O_CLOEXEC);
  if (descriptor < 0) {
    return false;
  }
  std::byte bytes[sg::detail::kSmiCountBytes] {};
  for (std::size_t index = 0; index < sg::detail::kSmiCountBytes; ++index) {
    bytes[index] = static_cast<std::byte>((value >> (8U * index)) & 0xffULL);
  }
  const auto written = ::pwrite(descriptor,
                                bytes,
                                sg::detail::kSmiCountBytes,
                                static_cast<off_t>(kNodeAddress));
  ::close(descriptor);
  return written == static_cast<off_t>(sg::detail::kSmiCountBytes);
}

// The scripted writes of one call, applied before that call's stamp. The
// counter read of a gap pass follows the stamp read of the same pass, so a
// write at the call of that pass stands between the read before the gap and
// the read after it, and beside no race (D-7, FR-039). A node the scenario
// left absent stands written by nobody: the write opens no file, so the
// absent shape of a neighbor cpu holds through every scripted call
// (FR-011).
auto applyScriptWrites(std::uint64_t call) noexcept -> void
{
  const int cpu = sched_getcpu();
  for (const auto& [at, bytes] : nodeAppears) {
    if (at != call || cpu < 0) {
      continue;
    }
    const auto path =
        sg::detail::procRootPath(nodePath(static_cast<std::uint32_t>(cpu)));
    std::error_code made;
    std::filesystem::create_directories(path.parent_path(), made);
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    file.close();
    if (made || !file) {
      scriptCreateFailed.store(true, std::memory_order_relaxed);
    }
  }
  for (const auto& [at, figure] : nodeWrites) {
    if (at != call || cpu < 0) {
      continue;
    }
    const auto path =
        sg::detail::procRootPath(nodePath(static_cast<std::uint32_t>(cpu)));
    if (!std::filesystem::exists(path)) {
      continue;
    }
    if (!writeCounterAt(path, figure)) {
      scriptWriteFailed.store(true, std::memory_order_relaxed);
    }
  }
  for (const auto& [at, figure] : interruptsWrites) {
    if (at != call) {
      continue;
    }
    if (!writeInterruptsWhole(scriptRoot, scriptColumns, figure)) {
      scriptWriteFailed.store(true, std::memory_order_relaxed);
    }
  }
  for (const auto& [at, text] : interruptsTexts) {
    if (at != call) {
      continue;
    }
    if (!writeInterruptsText(scriptRoot, text)) {
      scriptWriteFailed.store(true, std::memory_order_relaxed);
    }
  }
}

// The scripted series reader: one per-thread stamp series that steps by the
// scenario's step, and jumps at the calls the scenario names, so a gap lands
// on a planned pass and on no other (R-02, FR-039).
class SeriesReader final : public sg::counters::WindowReader
{
public:
  auto readPoints(sg::counters::PointSink& sink) noexcept -> void override
  {
    auto& thread = threadSeries;
    if (thread.ticket == 0) {
      thread.ticket = 1 + seriesTicket.fetch_add(1, std::memory_order_relaxed);
    }
    ++thread.calls;
    applyScriptWrites(thread.calls);
    thread.stamp += kStep;
    for (const auto at : gapCalls) {
      if (at == thread.calls) {
        thread.stamp += kJump;
      }
    }
    sink.put(thread.stamp);
    if (thread.ticket <= kMaxScriptedCpus) {
      seriesCalls[thread.ticket - 1].store(thread.calls,
                                           std::memory_order_relaxed);
    }
  }
};

// The script of one scenario, set fresh before each start (FR-039).
auto resetScript(const std::filesystem::path& root, std::size_t columns) -> void
{
  gapCalls.clear();
  nodeWrites.clear();
  interruptsWrites.clear();
  interruptsTexts.clear();
  nodeAppears.clear();
  scriptCreateFailed.store(false, std::memory_order_relaxed);
  scriptRoot = root;
  scriptColumns = columns;
  scriptWriteFailed.store(false, std::memory_order_relaxed);
  seriesTicket.store(0, std::memory_order_relaxed);
  for (auto& slot : seriesCalls) {
    slot.store(0, std::memory_order_relaxed);
  }
}

// The wait for one scripted call count: the bound turns a stalled series
// into a reported failure, so the wait cannot hang (FR-039).
auto waitCalls(std::size_t count, std::uint64_t target) -> void
{
  std::uint64_t spins = 0;
  while (true) {
    std::uint64_t behind = 0;
    for (std::size_t slot = 0; slot < count; ++slot) {
      if (seriesCalls[slot].load(std::memory_order_relaxed) < target) {
        ++behind;
      }
    }
    if (behind == 0) {
      return;
    }
    std::this_thread::yield();
    if (++spins > 1000000000ULL) {
      fail("the scripted series never reached the waited call (FR-039)");
    }
  }
}

// The options of one scripted run: the explicit threshold, the pre-check
// span of the `both` scenarios, and the buffer the rows fill (FR-007,
// FR-009, FR-014).
auto spanOptions(const std::vector<std::uint32_t>& cpus,
                 sg::JitterProbeMode mode,
                 std::uint32_t gapCount) -> sg::JitterProbeOptions
{
  sg::JitterProbeOptions options;
  options.cpus = cpus;
  // D-1: the `hwlat` method carries the two counter reads, so every scenario
  // that names a read drives that method (FR-012, FR-013).
  options.method = sg::JitterMethod::HWLAT;
  options.mode = mode;
  options.thresholdNs = kThresholdNs;
  options.gapCount = gapCount;
  options.precheckMs = kPrecheckMs;
  return options;
}

// One scripted run: the seam reader installed, the probe started, the loop
// run to the waited call, the between hook called while the loop still
// runs, and the states handed to the assertions behind the join edge (R-04).
template<class Between, class Assert>
auto runSpan(const sg::JitterProbeOptions& options,
             std::size_t cpus,
             std::uint64_t waitedCall,
             Between&& between,
             Assert&& assertions) -> void
{
  scriptedProvider();
  sg::detail::installTestReader(std::make_unique<SeriesReader>());
  sg::detail::JitterProbe probe(options);
  probe.start();
  waitCalls(cpus, waitedCall);
  between();
  probe.stopAndJoin();
  sg::detail::installTestReader(nullptr);
  check(!scriptWriteFailed.load(std::memory_order_relaxed),
        "every scripted write reached the file it names (T012)");
  check(probe.states().size() == cpus,
        "the probe started over the scripted leaf (T012)");
  assertions(probe.states());
}

// The marks of the kept gaps, read off the change ring: no element of the
// kept prefix stands above zero (FR-011, FR-015).
auto marksOff(const sg::detail::ProbeCpuState& state) -> bool
{
  for (std::size_t index = 0; index < state.kept; ++index) {
    if (state.changes.at(index) != 0) {
      return false;
    }
  }
  return true;
}

// The standing figures of one scripted run: the kept set, the widths the
// scripted series dictates, the threshold and the period the run measured,
// and the drop count, all of which an unavailable counter source leaves
// untouched (FR-011, FR-015, E-03).
auto standingFigures(const sg::JitterStats& stats,
                     std::uint64_t kept,
                     std::uint64_t dropped) -> bool
{
  return stats.gapCount == kept && stats.minNs == kGapWidth
      && stats.maxNs == kGapWidth && stats.totalNs == kept * kGapWidth
      && stats.thresholdNs == kThresholdNs && stats.loopPeriodNs == kStep
      && stats.droppedGaps == dropped;
}

// The figures one unavailable source leaves: no mark on any kept gap, the
// marked count at 0, and the delta of that source with no value, while every
// standing figure holds (FR-011, FR-015).
auto checkSourceAbsent(const sg::detail::ProbeCpuState& state,
                       const sg::JitterStats& stats,
                       const char* what) -> void
{
  check(marksOff(state), what);
  check(stats.smiGapCount == 0, what);
  check(!stats.smiDelta.has_value(), what);
}

// T012: the mark of one gap answers the change of the SMI counter across the
// span that ends at that gap, and index i of the change ring answers index i
// of the gap ring (FR-012, FR-013, FR-014, D-7, P-3, SC-001, SC-008).
auto theRiseRows() -> void
{
  const auto cpus = pickCpus(1);
  const HwlatRootGuard root;
  plant(root.root(), "proc/cpuinfo", kIntelText, kNodeRights);
  plant(root.root(),
        nodePath(cpus.at(0)),
        nodeImage(kOpeningCount, sg::detail::kSmiCountBytes),
        kNodeRights);
  plant(root.root(),
        "proc/interrupts",
        interruptsText(columnCount(cpus), kNmiOpening),
        kNodeRights);
  resetScript(root.root(), columnCount(cpus));
  gapCalls = {kGapOneCall, kGapTwoCall};
  nodeWrites = {{kGapOneCall, kRiseCount}};
  runSpan(
      spanOptions(cpus, sg::JitterProbeMode::ALONGSIDE, kGapCount),
      1,
      kGapTwoCall,
      [] {},
      [&](const auto& states)
      {
        const auto& state = states.at(0);
        const auto stats = sg::detail::gapStatsOf(state);
        check(state.kept == 2, "the run keeps the two scripted gaps (T012)");
        check(state.changes.size() == kGapCount
                    && state.changes.size() == state.buffer.size(),
                  "the change ring pre-sizes beside the gap ring with one "
                  "capacity (T017, P-3, E-03)");
        check(state.changes.at(0) == 1,
                  "a node that rises by one between the two reads that bound "
                  "a gap marks that gap (FR-012, FR-014, D-7)");
        check(state.changes.at(1) == 0,
                  "a node that stands flat between the two reads that bound a "
                  "gap marks that gap NOT_SMI (FR-013, FR-014, D-7)");
        check(state.edges.smiOpening.has_value()
                    && *state.edges.smiOpening == kOpeningCount,
                  "the first gap's span opens at the read of the window's "
                  "opening edge (FR-013, D-7)");
        check(stats.gapCount == state.kept,
              "the fold counts the kept set both rings carry (FR-012, " "P-3)");
      });
  writeNodeWhole(root.root(), cpus.at(0), kOpeningCount);
  resetScript(root.root(), columnCount(cpus));
  gapCalls = {kGapOneCall, kGapTwoCall, kGapThreeCall};
  nodeWrites = {{kGapOneCall, 11}, {kGapTwoCall, 25}, {kGapThreeCall, 44}};
  runSpan(
      spanOptions(cpus, sg::JitterProbeMode::ALONGSIDE, kGapCount),
      1,
      kGapThreeCall,
      [] {},
      [&](const auto& states)
      {
        const auto& state = states.at(0);
        check(state.kept == 3, "the run keeps the three scripted gaps (T012)");
        check(state.changes.at(0) == 4 && state.changes.at(1) == 14
                    && state.changes.at(2) == 19,
                  "index i of the change ring answers index i of the gap ring "
                  "(FR-012, P-3, SC-008)");
        check(state.buffer.at(0).stamp < state.buffer.at(1).stamp
                    && state.buffer.at(1).stamp < state.buffer.at(2).stamp,
                  "the paired gap stamps stand in the order the loop kept "
                  "them (FR-012, P-3)");
      });
}

// T013: the window of the three per-cpu figures: the pre-check figures answer
// the pre-check window, and the run-level figures answer the probe's own run
// span (FR-016, P-5, E-02).
auto theWindowRows() -> void
{
  const auto cpus = pickCpus(1);
  const HwlatRootGuard root;
  plant(root.root(), "proc/cpuinfo", kIntelText, kNodeRights);
  plant(root.root(),
        nodePath(cpus.at(0)),
        nodeImage(kOpeningCount, sg::detail::kSmiCountBytes),
        kNodeRights);
  plant(root.root(),
        "proc/interrupts",
        interruptsText(columnCount(cpus), kNmiOpening),
        kNodeRights);
  resetScript(root.root(), columnCount(cpus));
  gapCalls = {kPhaseGapCall, kBothGapCall};
  nodeWrites = {{kBothGapCall, kRiseCount}};
  runSpan(
      spanOptions(cpus, sg::JitterProbeMode::BOTH, kGapCount),
      1,
      kBothGapCall,
      [] {},
      [&](const auto& states)
      {
        const auto& state = states.at(0);
        const auto& latch = sg::detail::latchSnapshotOf(state);
        const auto pre = sg::detail::gapStatsOf(latch);
        const auto run = sg::detail::gapStatsOf(state);
        check(state.kept == 2 && latch.gapsKept == 1,
                  "the two windows keep their gaps in one ring, and the "
                  "snapshot copies the pre-check prefix (T017, E-03)");
        check(latch.changes.size() == latch.gapsKept
                    && latch.changes.at(0) == state.changes.at(0),
                  "the latch snapshot carries the change prefix beside the "
                  "gap prefix (T017, FR-013, E-03)");
        check(latch.window.smiOpening.has_value()
                    && latch.window.nmiOpening.has_value(),
                  "the latch snapshot carries the edge reads of the window it "
                  "copied (T017, FR-016, P-5)");
        check(pre.smiDelta.has_value() && *pre.smiDelta == 0,
                  "the pre-check figures answer the change of the two edge "
                  "reads of the pre-check window (FR-016, P-5)");
        check(pre.smiGapCount == 0,
                  "the pre-check fold marks no gap when that window's change "
                  "stands flat (FR-014, FR-016)");
        check(run.smiDelta.has_value() && *run.smiDelta == 1,
                  "the run-level figures answer the change of the probe's own "
                  "run span (FR-016, P-5)");
        check(run.smiGapCount == 1,
                  "the marked count counts the kept gaps whose span carries a "
                  "change above zero (FR-014, FR-016)");
      });
}

// T013: the NMI change stands as a disclosed figure of the same window, and
// marks no gap (FR-009, FR-016, FR-017, D-14).
auto theNmiRows() -> void
{
  const auto cpus = pickCpus(1);
  const HwlatRootGuard root;
  plant(root.root(), "proc/cpuinfo", kIntelText, kNodeRights);
  plant(root.root(),
        nodePath(cpus.at(0)),
        nodeImage(kOpeningCount, sg::detail::kSmiCountBytes),
        kNodeRights);
  plant(root.root(),
        "proc/interrupts",
        interruptsText(columnCount(cpus), kNmiOpening),
        kNodeRights);
  resetScript(root.root(), columnCount(cpus));
  gapCalls = {kGapOneCall, kGapTwoCall};
  nodeWrites = {{kGapOneCall, kRiseCount}};
  interruptsWrites = {{kGapTwoCall, kNmiRise}};
  runSpan(
      spanOptions(cpus, sg::JitterProbeMode::ALONGSIDE, kGapCount),
      1,
      kGapTwoCall,
      [] {},
      [&](const auto& states)
      {
        const auto& state = states.at(0);
        const auto stats = sg::detail::gapStatsOf(state);
        check(stats.nmiDelta.has_value() && *stats.nmiDelta == 4,
                  "the NMI figure answers the change of the two edge reads of "
                  "the same window (FR-009, FR-016, FR-017)");
        check(stats.smiDelta.has_value() && *stats.smiDelta == 1,
                  "the SMI figure answers the change of the two edge reads of "
                  "the same window (FR-016)");
        check(state.changes.at(1) == 0 && stats.smiGapCount == 1,
                  "a rising NMI figure marks no gap, because the mark answers "
                  "the SMI counter alone (FR-014, FR-017, D-14)");
      });
}

// T013: one source loses its row between the two edges of the window. The
// opening edge reads the planted figure, the closing edge reads a text that
// names no `NMI` row, and the delta of that source answers nothing while the
// pair of the counter source stands whole (FR-011, FR-016, FR-017).
auto theNmiRowGoneAtTheClosingEdge() -> void
{
  const auto cpus = pickCpus(1);
  const HwlatRootGuard root;
  plant(root.root(), "proc/cpuinfo", kIntelText, kNodeRights);
  plant(root.root(),
        nodePath(cpus.at(0)),
        nodeImage(kOpeningCount, sg::detail::kSmiCountBytes),
        kNodeRights);
  plant(root.root(),
        "proc/interrupts",
        interruptsText(columnCount(cpus), kNmiOpening),
        kNodeRights);
  resetScript(root.root(), columnCount(cpus));
  gapCalls = {kGapOneCall, kGapTwoCall};
  nodeWrites = {{kGapOneCall, kRiseCount}};
  interruptsTexts = {{kGapTwoCall, kNoNmiRowText}};
  runSpan(
      spanOptions(cpus, sg::JitterProbeMode::ALONGSIDE, kGapCount),
      1,
      kGapTwoCall,
      [] {},
      [&](const auto& states)
      {
        const auto& state = states.at(0);
        const auto stats = sg::detail::gapStatsOf(state);
        check(state.edges.nmiOpening.has_value()
                  && *state.edges.nmiOpening == kNmiOpening
                  && !state.edges.nmiClosing.has_value(),
              "the window carries the opening figure of the source and leaves "
              "its closing edge unread (T013, FR-016)");
        check(!stats.nmiDelta.has_value(),
              "the delta of a source whose closing edge reads no row answers "
              "nothing (T013, FR-011, FR-017)");
        check(state.edges.smiClosing.has_value() && stats.smiDelta.has_value()
                  && *stats.smiDelta == 1,
              "the pair of the source that keeps its row answers its own "
              "change (T013, FR-016)");
      });
}

// The whole text of one file a row captured (T014, FR-011).
auto readWhole(const std::filesystem::path& path) -> std::string
{
  std::ifstream file(path);
  std::ostringstream text;
  text << file.rdbuf();
  return text.str();
}

// The number of times one whole line stands in one captured report: a run names
// an absent source once, whatever the count of its cpus (FR-011, P-12).
auto countLine(const std::string& text, const std::string& line) -> std::size_t
{
  std::size_t hits = 0;
  for (auto at = text.find(line); at != std::string::npos;
       at = text.find(line, at + line.size()))
  {
    ++hits;
  }
  return hits;
}

// The report of one whole run: the status the entry point answered, and the
// bytes the report printed (T014).
struct RunReport
{
  int status = 0;
  std::string text;
};

// The quiet benchmark of the whole-run rows: one empty body, so the pre-check
// window stands the run's only measured span (T014).
auto bmQuiet(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

// One speedgunMain run in this process, its report captured to a file the row
// removes and read back: the row reads the status the entry point answered and
// the lines the disclosure printed, the way the tracer suite carries its
// whole-run rows (T014, FR-011).
auto captureRunReport(const std::vector<std::string>& arguments) -> RunReport
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_hwlat_probe_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }
  static int runIndex = 0;
  const std::string path =
      "harness_hwlat_probe_run_" + std::to_string(++runIndex) + ".txt";
  const int saved = ::dup(STDOUT_FILENO);
  if (saved < 0 || std::freopen(path.c_str(), "w", stdout) == nullptr) {
    fail("the report stands unredirectable (T014)");
  }
  RunReport report;
  report.status = sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
  std::fflush(stdout);
  if (::dup2(saved, STDOUT_FILENO) < 0) {
    fail("stdout stands unrestored (T014)");
  }
  ::close(saved);
  report.text = readWhole(path);
  std::remove(path.c_str());
  return report;
}

// The one command line of the whole run: the quiet benchmark, the probe over
// both scripted cpus with the detector method, the pre-check window the
// scripted stamps span, the standing threshold, and the document the row reads.
// No tracer stands on the line: the row names the probe's own source (T014).
auto runArguments(std::span<const std::uint32_t> cpus,
                  const std::string& tag) -> std::vector<std::string>
{
  std::string list;
  for (const auto cpu : cpus) {
    if (!list.empty()) {
      list.push_back(',');
    }
    list += std::to_string(cpu);
  }
  return {"--filter=bmQuiet",
          "--repetitions=1",
          "--iterations=2",
          "--jitter-probe=" + list + ",method=hwlat,mode=precheck,precheck="
              + std::to_string(kPrecheckMs)
              + ",threshold=" + std::to_string(kThresholdNs),
          "--json=harness_hwlat_probe_" + tag + ".json"};
}

// T014: a node that stands absent leaves the marks off and the delta with no
// value, while its neighbor cpu keeps its own figures (FR-011, FR-015,
// D-14, P-12).
auto theAbsentNodeRows() -> void
{
  const auto cpus = pickCpus(2);
  const HwlatRootGuard root;
  plant(root.root(), "proc/cpuinfo", kIntelText, kNodeRights);
  plant(root.root(),
        nodePath(cpus.at(1)),
        nodeImage(kFlatCount, sg::detail::kSmiCountBytes),
        kNodeRights);
  plant(root.root(),
        "proc/interrupts",
        interruptsText(columnCount(cpus), kNmiOpening),
        kNodeRights);
  resetScript(root.root(), columnCount(cpus));
  gapCalls = {kGapOneCall, kGapTwoCall};
  runSpan(
      spanOptions(cpus, sg::JitterProbeMode::ALONGSIDE, kGapCount),
      2,
      kGapTwoCall,
      [] {},
      [&](const auto& states)
      {
        const auto& first = states.at(0);
        const auto& second = states.at(1);
        const auto absentStats = sg::detail::gapStatsOf(first);
        const auto keptStats = sg::detail::gapStatsOf(second);
        check(sg::detail::smiSourceSupported(),
                  "the vendor gate stands open in the same tree, so the "
                  "absent node stands the cause (FR-010, FR-011)");
        checkSourceAbsent(first,
                              absentStats,
                              "an absent node leaves every gap unmarked, the "
                              "marked count at 0 and the SMI figure with no "
                              "value (FR-011, FR-015)");
        check(standingFigures(absentStats, 2, 0)
                    && standingFigures(keptStats, 2, 0),
                  "an absent node keeps every standing figure of both cpus "
                  "(FR-011, FR-015, E-03)");
        check(keptStats.smiDelta.has_value()
                    && *keptStats.smiDelta == 0
                    && keptStats.nmiDelta.has_value(),
                  "one cpu keeps its own figures while its neighbor answers "
                  "absent (FR-011, FR-016)");
      });
}

// Whether the kernel's rights check binds this process on the planted node. A
// process that holds the discretionary-access override reads a node that
// carries no rights bits, and the continuous integration runner of specs/036
// runs as one (specs/036 T010, FR-032).
auto theRightsBind(std::uint32_t cpu) -> bool
{
  const auto probe = sg::detail::openMsrNode(cpu);
  const bool refused = !probe.has_value();
  if (probe.has_value()) {
    sg::detail::closeMsrNode(*probe);
  }
  return refused;
}

// T014: a node the process holds no rights on stands unavailable to a process
// the kernel checks, and answers its count to a process that holds the
// discretionary-access override. The other source keeps its figure on both
// answers (FR-007, FR-011, FR-015, D-14, specs/036 T010).
auto theNoRightsRows() -> void
{
  const auto cpus = pickCpus(1);
  const HwlatRootGuard root;
  plant(root.root(), "proc/cpuinfo", kIntelText, kNodeRights);
  plant(root.root(),
        nodePath(cpus.at(0)),
        nodeImage(kOpeningCount, sg::detail::kSmiCountBytes),
        kNoRights);
  plant(root.root(),
        "proc/interrupts",
        interruptsText(columnCount(cpus), kNmiOpening),
        kNodeRights);
  resetScript(root.root(), columnCount(cpus));
  const bool rightsRefused = theRightsBind(cpus.at(0));
  gapCalls = {kGapOneCall, kGapTwoCall};
  runSpan(
      spanOptions(cpus, sg::JitterProbeMode::ALONGSIDE, kGapCount),
      1,
      kGapTwoCall,
      [] {},
      [&](const auto& states)
      {
        const auto& state = states.at(0);
        const auto stats = sg::detail::gapStatsOf(state);
        if (rightsRefused) {
          checkSourceAbsent(state,
                            stats,
                            "a node without rights leaves every gap "
                            "unmarked, the marked count at 0 and the SMI "
                            "figure with no value (FR-011, FR-015)");
        } else {
          check(stats.smiDelta.has_value(),
                "a node that carries no rights bits answers its count to a "
                "process the kernel lets read past them, so the source stands "
                "available (FR-007, FR-011, FR-015)");
        }
        check(standingFigures(stats, 2, 0),
                  "a node without rights keeps every standing figure "
                  "(FR-011, FR-015, E-03)");
        check(stats.nmiDelta.has_value(),
              "the other source keeps its figure on the answer the node gave "
              "(FR-009, FR-011)");
      });
}

// T014: a node that answers fewer bytes than the counter width stands
// unavailable (FR-007, FR-011, FR-015).
auto theShortNodeRows() -> void
{
  const auto cpus = pickCpus(1);
  const HwlatRootGuard root;
  plant(root.root(), "proc/cpuinfo", kIntelText, kNodeRights);
  plant(root.root(),
        nodePath(cpus.at(0)),
        nodeImage(kOpeningCount, 0),
        kNodeRights);
  plant(root.root(),
        "proc/interrupts",
        interruptsText(columnCount(cpus), kNmiOpening),
        kNodeRights);
  resetScript(root.root(), columnCount(cpus));
  gapCalls = {kGapOneCall, kGapTwoCall};
  runSpan(
      spanOptions(cpus, sg::JitterProbeMode::ALONGSIDE, kGapCount),
      1,
      kGapTwoCall,
      [] {},
      [&](const auto& states)
      {
        const auto& state = states.at(0);
        const auto stats = sg::detail::gapStatsOf(state);
        checkSourceAbsent(state,
                              stats,
                              "a short read leaves every gap unmarked, the "
                              "marked count at 0 and the SMI figure with no "
                              "value (FR-007, FR-011, FR-015)");
        check(stats.nmiDelta.has_value(),
                  "the interrupt figure stands read while the node answers "
                  "short (FR-009, FR-011)");
      });
}

// T014: a part that states no documented counter stands unavailable, and the
// same tree answers its figures once the line states the documented vendor
// (FR-010, FR-011, FR-015, D-4).
auto theVendorRows() -> void
{
  const auto cpus = pickCpus(1);
  const HwlatRootGuard root;
  plant(root.root(), "proc/cpuinfo", kOtherVendorText, kNodeRights);
  plant(root.root(),
        nodePath(cpus.at(0)),
        nodeImage(kOpeningCount, sg::detail::kSmiCountBytes),
        kNodeRights);
  plant(root.root(),
        "proc/interrupts",
        interruptsText(columnCount(cpus), kNmiOpening),
        kNodeRights);
  resetScript(root.root(), columnCount(cpus));
  gapCalls = {kGapOneCall, kGapTwoCall};
  runSpan(
      spanOptions(cpus, sg::JitterProbeMode::ALONGSIDE, kGapCount),
      1,
      kGapTwoCall,
      [] {},
      [&](const auto& states)
      {
        const auto& state = states.at(0);
        const auto stats = sg::detail::gapStatsOf(state);
        checkSourceAbsent(state,
                              stats,
                              "a part that states no documented counter "
                              "leaves every gap unmarked, the marked count at "
                              "0 and the SMI figure with no value (FR-010, "
                              "FR-011, FR-015)");
        check(standingFigures(stats, 2, 0),
                  "an undocumented part keeps every standing figure of the "
                  "run (FR-010, FR-015)");
      });
  plant(root.root(), "proc/cpuinfo", kIntelText, kNodeRights);
  resetScript(root.root(), columnCount(cpus));
  gapCalls = {kGapOneCall, kGapTwoCall};
  nodeWrites = {{kGapOneCall, kRiseCount}};
  runSpan(
      spanOptions(cpus, sg::JitterProbeMode::ALONGSIDE, kGapCount),
      1,
      kGapTwoCall,
      [] {},
      [&](const auto& states)
      {
        const auto& state = states.at(0);
        const auto stats = sg::detail::gapStatsOf(state);
        check(state.changes.at(0) == 1 && stats.smiGapCount == 1
                    && stats.smiDelta.has_value() && *stats.smiDelta == 1,
                  "the same tree marks its gaps once the vendor line states "
                  "the documented text (FR-010, FR-014)");
      });
}

// T014: a `/proc/interrupts` read with no `NMI` row leaves the NMI figure
// with no value, while the SMI source keeps its marks (FR-009, FR-011,
// FR-015, FR-017).
auto theNoNmiRows() -> void
{
  const auto cpus = pickCpus(1);
  const HwlatRootGuard root;
  plant(root.root(), "proc/cpuinfo", kIntelText, kNodeRights);
  plant(root.root(),
        nodePath(cpus.at(0)),
        nodeImage(kOpeningCount, sg::detail::kSmiCountBytes),
        kNodeRights);
  plant(root.root(),
        "proc/interrupts",
        "LOC:  500  500   Local timer interrupts\n",
        kNodeRights);
  resetScript(root.root(), columnCount(cpus));
  gapCalls = {kGapOneCall, kGapTwoCall};
  nodeWrites = {{kGapOneCall, kRiseCount}};
  runSpan(
      spanOptions(cpus, sg::JitterProbeMode::ALONGSIDE, kGapCount),
      1,
      kGapTwoCall,
      [] {},
      [&](const auto& states)
      {
        const auto& state = states.at(0);
        const auto stats = sg::detail::gapStatsOf(state);
        check(!stats.nmiDelta.has_value(),
                  "an interrupt text with no NMI row leaves the NMI figure "
                  "with no value (FR-009, FR-011)");
        check(state.changes.at(0) == 1 && stats.smiGapCount == 1
                    && stats.smiDelta.has_value() && *stats.smiDelta == 1,
                  "the SMI source keeps its marks and its figure while the "
                  "NMI row stands absent (FR-009, FR-015, FR-017)");
      });
}

// T014: one run answers one absent figure per source for every cpu, so the
// standing wording names each source once for the run and the run keeps its
// standing figures (FR-011, FR-015, D-14, P-12).
auto theOncePerSourceRows() -> void
{
  const auto cpus = pickCpus(2);
  const HwlatRootGuard root;
  plant(root.root(), "proc/cpuinfo", kIntelText, kNodeRights);
  plant(root.root(),
        "proc/interrupts",
        "LOC:  500  500   Local timer interrupts\n",
        kNodeRights);
  resetScript(root.root(), columnCount(cpus));
  gapCalls = {kGapOneCall, kGapTwoCall};
  runSpan(
      spanOptions(cpus, sg::JitterProbeMode::ALONGSIDE, kGapCount),
      2,
      kGapTwoCall,
      [] {},
      [&](const auto& states)
      {
        std::size_t silentSources = 0;
        for (const auto& state : states) {
          const auto stats = sg::detail::gapStatsOf(state);
          checkSourceAbsent(state,
                                stats,
                                "an unavailable source leaves every gap of "
                                "the run unmarked and keeps the standing "
                                "figures (FR-011, FR-015)");
          if (!stats.smiDelta.has_value()) {
            ++silentSources;
          }
          check(!stats.nmiDelta.has_value(),
                    "the NMI source answers one absent figure for every cpu "
                    "of the run, so the wording names it once for the "
                    "run (FR-011, FR-016, P-12)");
        }
        check(silentSources == states.size() && states.size() == 2,
                  "the SMI source answers one absent figure for every cpu of "
                  "the run, so the standing wording names it once (FR-011, "
                  "FR-016, P-12)");
        check(standingFigures(sg::detail::gapStatsOf(states.at(0)), 2, 0)
                    && standingFigures(sg::detail::gapStatsOf(states.at(1)),
                                       2,
                                       0),
                  "the two unavailable sources keep the run's standing "
                  "figures (FR-011, FR-015, D-14)");
      });
}

// T014, FR-011, FR-015, FR-016, FR-025, D-14, P-12: one whole run over a tree
// that holds no register node for either probe cpu. The console names the
// management-interrupt source once for the run, the figure line states the
// standing token in both columns, the run keeps the status of a run that lost a
// source, and both folds keep the figures the scripted series dictates: the two
// kept gaps, their widths, the threshold, the measured period and no drop. The
// control run plants both nodes at one flat figure in the same tree, so the
// same run names no absent source and states one count in both columns.
auto theAbsentNodeRunDisclosesItsSourceOnce() -> void
{
  const auto cpus = pickCpus(2);
  scriptedProvider();
  const auto drive = [&](const bool nodesStand,
                         const std::string& tag) -> RunReport
  {
    const HwlatRootGuard root;
    plant(root.root(), "proc/cpuinfo", kIntelText, kNodeRights);
    plant(root.root(),
          "proc/interrupts",
          interruptsText(columnCount(cpus), kNmiOpening),
          kNodeRights);
    if (nodesStand) {
      for (const auto cpu : cpus) {
        plant(root.root(),
              nodePath(cpu),
              nodeImage(kFlatCount, sg::detail::kSmiCountBytes),
              kNodeRights);
      }
    }
    resetScript(root.root(), columnCount(cpus));
    gapCalls = {kGapOneCall, kGapTwoCall};
    sg::detail::installTestReader(std::make_unique<SeriesReader>());
    auto report = captureRunReport(runArguments(cpus, tag));
    sg::detail::installTestReader(nullptr);
    return report;
  };
  const auto widths = "minNs: " + std::to_string(kGapWidth) + " "
      + std::to_string(kGapWidth) + "\n";
  const std::string absentJson = "harness_hwlat_probe_no-node.json";
  const auto absent = drive(false, "no-node");
  check(absent.status == 0,
        "the run over the tree with no register node keeps the status of a run "
        "that lost a source (T014, FR-011)");
  check(countLine(absent.text, "jitter: SMI count not available\n") == 1,
        "the run names its absent source once, over the two cpus it probed "
        "(T014, FR-011, FR-016, P-12)");
  check(countLine(absent.text, "jitter: NMI count not available\n") == 0,
        "the planted interrupt text answers, so the disclosure names the one "
        "source that stands absent (T014, FR-016)");
  check(countLine(absent.text,
                   "smiDelta: not available not available\n")
            == 1,
        "the figure line states the standing token in one column per cpu "
        "(T014, FR-016, FR-025)");
  check(countLine(absent.text, "gapCount: 2 2\n") == 1
            && countLine(absent.text, widths) == 1
            && countLine(absent.text, "smiGapCount: 0 0\n") == 1,
        "the console keeps the gap count, the widths and the marked count of "
        "both cpus whole (T014, FR-015, E-03)");
  const auto absentDoc = sg::loadReport(absentJson);
  if (!absentDoc.has_value() || !absentDoc->runJitterStats.has_value()) {
    const std::string why = "the run folds each probe cpu (T014, FR-025): "
        + (absentDoc.has_value() ? "a record stands absent"
                                 : absentDoc.error());
    check(false, why.c_str());
  } else {
    const auto& folds = *absentDoc->runJitterStats;
    check(folds.size() == cpus.size(),
          "the run folds one entry per probe cpu (T014, FR-025)");
    for (const auto& entry : folds) {
      check(standingFigures(entry.stats, 2, 0),
            "the absent node keeps every standing figure of its cpu (T014, "
            "FR-015, E-03)");
      check(!entry.stats.smiDelta.has_value()
                && entry.stats.smiGapCount == 0,
            "the fold of the absent node carries no delta and no marked gap "
            "(T014, FR-011, FR-015)");
      check(entry.stats.nmiDelta.has_value()
                && *entry.stats.nmiDelta == 0,
            "the planted interrupt text answers its delta in the fold (T014, "
            "FR-016)");
    }
  }
  std::remove(absentJson.c_str());
  const std::string wholeJson = "harness_hwlat_probe_both-nodes.json";
  const auto whole = drive(true, "both-nodes");
  check(whole.status == 0,
        "the control run keeps the same status (T014, FR-016)");
  check(countLine(whole.text, "jitter: SMI count not available\n") == 0,
        "the planted node answers its count, so the run names no absent source "
        "(T014, FR-016, FR-032)");
  check(countLine(whole.text, "smiDelta: 0 0\n") == 1
            && countLine(whole.text, widths) == 1,
        "the control run states one count in both columns beside the same "
        "figures (T014, FR-016)");
  const auto wholeDoc = sg::loadReport(wholeJson);
  if (wholeDoc.has_value() && wholeDoc->runJitterStats.has_value()) {
    for (const auto& entry : *wholeDoc->runJitterStats) {
      check(standingFigures(entry.stats, 2, 0)
                && entry.stats.smiDelta.has_value()
                && *entry.stats.smiDelta == 0,
            "the control fold keeps its figures beside its counter (T014, "
            "FR-016)");
    }
  } else {
    check(false, "the control run folds each probe cpu (T014, FR-025)");
  }
  std::remove(wholeJson.c_str());
}

// T015: a full buffer drops record and change in one step, raises the one
// drop counter, and the marked count counts the kept gaps only (FR-012,
// FR-013, E-03).
auto theBufferRows() -> void
{
  const auto cpus = pickCpus(1);
  const HwlatRootGuard root;
  plant(root.root(), "proc/cpuinfo", kIntelText, kNodeRights);
  plant(root.root(),
        nodePath(cpus.at(0)),
        nodeImage(kOpeningCount, sg::detail::kSmiCountBytes),
        kNodeRights);
  plant(root.root(),
        "proc/interrupts",
        interruptsText(columnCount(cpus), kNmiOpening),
        kNodeRights);
  resetScript(root.root(), columnCount(cpus));
  gapCalls = {kGapOneCall, kGapTwoCall, kGapThreeCall};
  nodeWrites = {
      {kGapOneCall, kRiseCount}, {kGapTwoCall, 9}, {kGapThreeCall, 10}};
  runSpan(
      spanOptions(cpus, sg::JitterProbeMode::ALONGSIDE, kShortGapCount),
      1,
      kGapThreeCall,
      [] {},
      [&](const auto& states)
      {
        const auto& state = states.at(0);
        const auto stats = sg::detail::gapStatsOf(state);
        check(state.kept == kShortGapCount
                    && state.droppedGaps == 1 && stats.gapCount == 2
                    && stats.droppedGaps == state.droppedGaps,
                  "a full buffer drops the record and the change in one step "
                  "and raises the one drop counter (FR-012, E-03)");
        check(state.changes.at(0) == 1 && state.changes.at(1) == 1
                    && state.changes.size() == state.buffer.size(),
                  "the two rings keep one prefix, so a dropped gap stands in "
                  "neither of them (FR-012, E-03)");
        check(stats.smiGapCount == 2,
                  "the marked count counts the kept gaps only (FR-013, "
                  "FR-014, P-3)");
      });
}

// T015: a gap at the last instant of the window carries the change of the
// span that ends at it, while the window delta keeps the change of the two
// edge reads, and the gap the read behind the latch opens stands unmarked
// (FR-012, FR-013, FR-014, D-7, E-03).
auto theEdgeRows() -> void
{
  const auto cpus = pickCpus(1);
  const HwlatRootGuard root;
  plant(root.root(), "proc/cpuinfo", kIntelText, kNodeRights);
  plant(root.root(),
        nodePath(cpus.at(0)),
        nodeImage(kOpeningCount, sg::detail::kSmiCountBytes),
        kNodeRights);
  plant(root.root(),
        "proc/interrupts",
        interruptsText(columnCount(cpus), kNmiOpening),
        kNodeRights);
  resetScript(root.root(), columnCount(cpus));
  gapCalls = {kGapOneCall, kGapTwoCall};
  nodeWrites = {{kGapOneCall, kRiseCount}, {kGapTwoCall, kSecondRiseCount}};
  runSpan(
      spanOptions(cpus, sg::JitterProbeMode::ALONGSIDE, kGapCount),
      1,
      kGapTwoCall,
      [&] { writeNodeWhole(root.root(), cpus.at(0), kClosingCount); },
      [&](const auto& states)
      {
        const auto& state = states.at(0);
        const auto stats = sg::detail::gapStatsOf(state);
        check(state.changes.at(1) == 2,
                  "the gap at the last instant carries the change of the span "
                  "that ends at it (FR-013, D-7)");
        check(stats.smiDelta.has_value() && *stats.smiDelta == 5,
                  "the window delta keeps the change of the two edge reads, "
                  "past the last gap's read (FR-013, FR-016, D-7, E-03)");
        check(state.changes.at(0) + state.changes.at(1) == 3
                    && stats.smiGapCount == 2,
                  "the change prefix sums the spans of the kept gaps up to the "
                  "last kept gap (FR-012, D-7, E-03)");
      });
  writeNodeWhole(root.root(), cpus.at(0), kOpeningCount);
  resetScript(root.root(), columnCount(cpus));
  gapCalls = {kFirstPassCall};
  runSpan(
      spanOptions(cpus, sg::JitterProbeMode::ALONGSIDE, kGapCount),
      1,
      kFirstPassCall,
      [] {},
      [&](const auto& states)
      {
        const auto& state = states.at(0);
        const auto stats = sg::detail::gapStatsOf(state);
        check(state.kept == 1 && state.changes.at(0) == 0
                    && stats.smiGapCount == 0,
                  "the gap the probe's own read opens answers NOT_SMI, "
                  "because its span holds no change (FR-013, FR-014, D-7)");
        check(stats.smiDelta.has_value() && *stats.smiDelta == 0,
                  "a run whose two edge reads answer one figure reports a "
                  "window delta of zero (FR-016, E-02)");
      });
}

// T019: a run of a method that carries no counter read reads neither source:
// the marks stand off, the marked count answers 0, and the two changes answer
// nothing, while every standing figure holds (D-1, FR-012, FR-025, E-02).
auto theStandingMethodRows() -> void
{
  const auto cpus = pickCpus(1);
  const HwlatRootGuard root;
  plant(root.root(), "proc/cpuinfo", kIntelText, kNodeRights);
  plant(root.root(),
        nodePath(cpus.at(0)),
        nodeImage(kOpeningCount, sg::detail::kSmiCountBytes),
        kNodeRights);
  plant(root.root(),
        "proc/interrupts",
        interruptsText(columnCount(cpus), kNmiOpening),
        kNodeRights);
  resetScript(root.root(), columnCount(cpus));
  gapCalls = {kGapOneCall, kGapTwoCall};
  nodeWrites = {{kGapOneCall, kRiseCount}, {kGapTwoCall, kSecondRiseCount}};
  auto options = spanOptions(cpus, sg::JitterProbeMode::ALONGSIDE, kGapCount);
  options.method = sg::JitterMethod::TSC;
  runSpan(
      options,
      1,
      kGapTwoCall,
      [] {},
      [&](const auto& states)
      {
        const auto& state = states.at(0);
        const auto stats = sg::detail::gapStatsOf(state);
        check(state.kept == 2,
              "the standing method keeps its gaps, so the row reads a run that "
              "kept gaps and read no counter (D-1)");
        checkSourceAbsent(
            state,
            stats,
            "a method that carries no counter read leaves every gap unmarked "
            "and the marked count at 0 (D-1, FR-012, FR-025)");
        check(!stats.nmiDelta.has_value(),
              "a method that carries no counter read leaves the interrupt "
              "change with no value (D-1, FR-009, FR-025)");
        check(standingFigures(stats, 2, 0),
              "a method that carries no counter read keeps every standing "
              "figure of the run (D-1, A-4, E-03)");
      });
}

// T019: a node that answers no figure at the window's opening edge leaves the
// window's first gap unmarked, and the next gap's span is measured from the
// first read that answered (FR-011, FR-013, D-7, FR-016).
auto theLateNodeRows() -> void
{
  const auto cpus = pickCpus(1);
  const HwlatRootGuard root;
  plant(root.root(), "proc/cpuinfo", kIntelText, kNodeRights);
  plant(root.root(),
        nodePath(cpus.at(0)),
        nodeImage(kOpeningCount, 0),
        kNodeRights);
  plant(root.root(),
        "proc/interrupts",
        interruptsText(columnCount(cpus), kNmiOpening),
        kNodeRights);
  resetScript(root.root(), columnCount(cpus));
  gapCalls = {kGapOneCall, kGapTwoCall};
  nodeWrites = {{kGapOneCall, kRiseCount}, {kGapTwoCall, kSecondRiseCount}};
  runSpan(
      spanOptions(cpus, sg::JitterProbeMode::ALONGSIDE, kGapCount),
      1,
      kGapTwoCall,
      [] {},
      [&](const auto& states)
      {
        const auto& state = states.at(0);
        const auto stats = sg::detail::gapStatsOf(state);
        check(state.kept == 2 && state.changes.at(0) == 0
                  && state.changes.at(1) == 2,
              "a node that answers nothing at the opening edge leaves the "
              "first gap unmarked, and the next gap's span answers the two "
              "reads that did answer (FR-011, FR-013, D-7, FR-014)");
        check(stats.smiGapCount == 0,
              "the window whose two edge reads answer one read marks no gap of "
              "the run, so the count answers 0 beside a ring that carries a "
              "rise (FR-015, FR-016, D-13, P-3)");
        check(!stats.smiDelta.has_value(),
              "an opening edge that answered nothing leaves the window change "
              "with no value, past the reads that answered (FR-011, FR-016)");
        check(stats.nmiDelta.has_value(),
              "the source that answers from its first read keeps its change "
              "while the node answers short (FR-009, FR-011)");
      });
  plant(root.root(),
        nodePath(cpus.at(0)),
        nodeImage(kOpeningCount, sg::detail::kSmiCountBytes),
        kNodeRights);
  resetScript(root.root(), columnCount(cpus));
  gapCalls = {kGapOneCall, kGapTwoCall};
  nodeWrites = {{kGapOneCall, kRiseCount}, {kGapTwoCall, kSecondRiseCount}};
  runSpan(
      spanOptions(cpus, sg::JitterProbeMode::ALONGSIDE, kGapCount),
      1,
      kGapTwoCall,
      [] {},
      [&](const auto& states)
      {
        const auto& state = states.at(0);
        const auto stats = sg::detail::gapStatsOf(state);
        check(state.changes.at(0) == 1 && state.changes.at(1) == 2
                  && stats.smiGapCount == 2 && stats.smiDelta.has_value()
                  && *stats.smiDelta == 3,
              "a node that answers at the opening edge marks the window's "
              "first gap too, and the window change answers the edge pair "
              "(FR-013, FR-014, FR-016)");
      });
}

// T014, FR-011, FR-015, FR-016, FR-025, P-3, D-13: a register node that stands
// out of reach of the window's opening read and answers from the first gap read
// onward. The item opens its node once at its start, so the row plants a node
// that opens and answers no figure, and the scripted list brings the figure to
// that node at the call of the run's first gap. The pair of edge reads then
// answers one read: the window change carries no value, and FR-015 leaves no
// gap of that run marked, so the marked count answers 0 while the change ring
// carries the rise the scripted write planted and every standing figure holds.
// The document leaves the mark of every gap empty beside that count, so no
// figure claims a mark its carriers leave empty. The control plants the whole
// node from the start behind the same scripted writes and reads the marked
// count above zero, so the row cannot pass on a run that kept no gap.
auto theNodeThatAnswersFromTheFirstGapOnward() -> void
{
  const auto cpus = pickCpus(1);
  const auto marksOf = [](const sg::detail::ProbeCpuState& state)
  {
    std::size_t marks = 0;
    for (std::size_t index = 0; index < state.kept; ++index) {
      marks += state.changes.at(index) > 0 ? std::size_t {1} : std::size_t {0};
    }
    return marks;
  };
  const auto drive = [&](const bool nodeStands, const auto& assertions)
  {
    const HwlatRootGuard root;
    plant(root.root(), "proc/cpuinfo", kIntelText, kNodeRights);
    plant(root.root(),
          nodePath(cpus.at(0)),
          nodeImage(kOpeningCount,
                    nodeStands ? sg::detail::kSmiCountBytes : std::size_t {0}),
          kNodeRights);
    plant(root.root(),
          "proc/interrupts",
          interruptsText(columnCount(cpus), kNmiOpening),
          kNodeRights);
    resetScript(root.root(), columnCount(cpus));
    gapCalls = {kGapOneCall, kGapTwoCall};
    nodeWrites = {{kGapTwoCall, kRiseCount}};
    if (!nodeStands) {
      nodeAppears = {
          {kGapOneCall, nodeImage(kOpeningCount, sg::detail::kSmiCountBytes)}};
    }
    runSpan(
        spanOptions(cpus, sg::JitterProbeMode::ALONGSIDE, kGapCount),
        1,
        kGapTwoCall,
        [] {},
        assertions);
    check(!scriptCreateFailed.load(std::memory_order_relaxed)
              && !scriptWriteFailed.load(std::memory_order_relaxed),
          "the scripted node writes reached the node they name (FR-011, "
          "FR-032, FR-039)");
  };
  drive(
      false,
      [&](const auto& states)
      {
        const auto& state = states.at(0);
        const auto stats = sg::detail::gapStatsOf(state);
        check(state.kept == 2,
              "the run over the node that answers behind its first gap keeps "
              "its two scripted gaps (FR-011, FR-025)");
        check(!state.edges.smiOpening.has_value()
                  && state.edges.smiClosing.has_value(),
              "the node answers nothing at the window's opening read and "
              "answers at its closing read (FR-011, D-13)");
        check(!stats.smiDelta.has_value(),
              "the edge pair that answers one read leaves the window change "
              "with no value (FR-011, FR-016)");
        check(marksOf(state) > 0 && stats.smiGapCount == 0,
              "the marked count answers 0 while the change ring carries a gap "
              "above zero, because no gap of that run carries a mark (FR-015, "
              "FR-016, D-13, P-3)");
        check(standingFigures(stats, 2, 0),
              "the node that answers one edge read keeps every standing figure "
              "of the run (FR-011, FR-025, E-03)");
      });
  drive(
      true,
      [&](const auto& states)
      {
        const auto& state = states.at(0);
        const auto stats = sg::detail::gapStatsOf(state);
        check(state.edges.smiOpening.has_value()
                  && state.edges.smiClosing.has_value(),
              "the control node answers both edge reads of the same window "
              "(FR-011, FR-016)");
        check(stats.smiDelta.has_value() && *stats.smiDelta == 1,
              "the control window answers the change of its two edge reads "
              "(FR-016, FR-025)");
        check(marksOf(state) > 0 && stats.smiGapCount == marksOf(state),
              "the control count counts the gaps that carry the mark, so the "
              "row above cannot pass on a run that kept no gap (FR-015, FR-016, "
              "P-3)");
        check(
            standingFigures(stats, 2, 0),
            "the control run keeps the same standing figures (FR-011, FR-025)");
      });
}

// T019: a counter that stands lower than the read before it marks no gap, and
// the window change answers the floor the figure carries: zero (FR-013,
// FR-014, FR-016, E-02).
auto theFallingNodeRows() -> void
{
  const auto cpus = pickCpus(1);
  const HwlatRootGuard root;
  plant(root.root(), "proc/cpuinfo", kIntelText, kNodeRights);
  plant(root.root(),
        nodePath(cpus.at(0)),
        nodeImage(kOpeningCount, sg::detail::kSmiCountBytes),
        kNodeRights);
  plant(root.root(),
        "proc/interrupts",
        interruptsText(columnCount(cpus), kNmiOpening),
        kNodeRights);
  resetScript(root.root(), columnCount(cpus));
  gapCalls = {kGapOneCall, kGapTwoCall};
  nodeWrites = {{kGapOneCall, kRiseCount}, {kGapTwoCall, kFallingCount}};
  runSpan(
      spanOptions(cpus, sg::JitterProbeMode::ALONGSIDE, kGapCount),
      1,
      kGapTwoCall,
      [] {},
      [&](const auto& states)
      {
        const auto& state = states.at(0);
        const auto stats = sg::detail::gapStatsOf(state);
        check(state.changes.at(0) == 1 && state.changes.at(1) == 0
                  && stats.smiGapCount == 1,
              "a counter that stands lower than the read before it marks its "
              "gap NOT_SMI, the way a flat span does (FR-013, FR-014)");
        check(stats.smiDelta.has_value() && *stats.smiDelta == 0,
              "the window change answers zero when the closing read stands "
              "below the opening read, the floor the figure carries (FR-016, "
              "E-02)");
        check(standingFigures(stats, 2, 0),
              "a falling counter leaves every standing figure of the run "
              "(FR-016, E-03)");
      });
  plant(root.root(),
        nodePath(cpus.at(0)),
        nodeImage(kOpeningCount, sg::detail::kSmiCountBytes),
        kNodeRights);
  resetScript(root.root(), columnCount(cpus));
  gapCalls = {kGapOneCall, kGapTwoCall};
  nodeWrites = {{kGapOneCall, kRiseCount}, {kGapTwoCall, kClosingCount}};
  runSpan(
      spanOptions(cpus, sg::JitterProbeMode::ALONGSIDE, kGapCount),
      1,
      kGapTwoCall,
      [] {},
      [&](const auto& states)
      {
        const auto& state = states.at(0);
        const auto stats = sg::detail::gapStatsOf(state);
        check(state.changes.at(0) == 1 && state.changes.at(1) == 4
                  && stats.smiGapCount == 2 && stats.smiDelta.has_value()
                  && *stats.smiDelta == 5,
              "a counter that rises between the reads marks both gaps, and the "
              "window change answers the difference of the edge pair (FR-013, "
              "FR-014, FR-016)");
      });
}

// T019: the per-entry fold answers the count of the gaps its own windows place
// and answers no window change, and the counts of the items add to the
// run-level count (FR-013, FR-016, FR-025, P-3, E-06).
auto theItemFoldRows() -> void
{
  const auto cpus = pickCpus(1);
  const HwlatRootGuard root;
  plant(root.root(), "proc/cpuinfo", kIntelText, kNodeRights);
  plant(root.root(),
        nodePath(cpus.at(0)),
        nodeImage(kOpeningCount, sg::detail::kSmiCountBytes),
        kNodeRights);
  plant(root.root(),
        "proc/interrupts",
        interruptsText(columnCount(cpus), kNmiOpening),
        kNodeRights);
  resetScript(root.root(), columnCount(cpus));
  gapCalls = {kGapOneCall, kGapTwoCall};
  nodeWrites = {{kGapOneCall, kRiseCount}};
  runSpan(
      spanOptions(cpus, sg::JitterProbeMode::ALONGSIDE, kGapCount),
      1,
      kGapTwoCall,
      [] {},
      [&](const auto& states)
      {
        const auto& state = states.at(0);
        const auto run = sg::detail::gapStatsOf(state);
        // One window per kept gap, opened on that gap's own stamp, the reading
        // E-06 gives the split (E-06, R-04).
        const auto itemOf = [&](const std::size_t index)
        {
          const auto stamp = state.buffer.at(index).stamp;
          const std::vector<sg::detail::TimedWindow> windows {
              {.start = stamp,
               .end = stamp + 1,
               .entry = 0,
               .repetition = 0,
               .chunk = 0,
               .pass = 0},
          };
          return sg::detail::gapStatsOf(state, windows);
        };
        const auto first = itemOf(0);
        const auto second = itemOf(1);
        check(first.gapCount == 1 && second.gapCount == 1
                  && run.gapCount == 2,
              "each item folds the one gap its own window places (E-06, E-08, "
              "FR-025)");
        check(first.smiGapCount == 1 && second.smiGapCount == 0
                  && first.smiGapCount + second.smiGapCount == run.smiGapCount,
              "the per-entry count folds the changes of that item's own kept "
              "gaps, and the items' counts add to the run-level figure "
              "(FR-014, FR-025, P-3)");
        check(!first.smiDelta.has_value() && !first.nmiDelta.has_value()
                  && run.smiDelta.has_value() && run.nmiDelta.has_value(),
              "the per-entry record answers no window change, because that fold "
              "holds no window edge, and the run-level record carries both "
              "(FR-016, FR-025, P-5)");
      });
}

// T045, FR-002, FR-019, D-1: the console spelling of the detector method. The
// name table carries a fourth word, and the detector method reads it.
auto theDetectorNameRow() -> void
{
  const std::string_view detector =
      sg::detail::jitterMethodName(sg::JitterMethod::HWLAT);
  check(detector == "hwlat",
        "the detector method reads the fourth word of the name table "
        "(FR-002, FR-019, D-1)");
}

}  // namespace

SG_BENCHMARK(bmQuiet);

auto main() -> int
{
  theRiseRows();
  theWindowRows();
  theNmiRows();
  theNmiRowGoneAtTheClosingEdge();
  theAbsentNodeRows();
  theNoRightsRows();
  theShortNodeRows();
  theVendorRows();
  theNoNmiRows();
  theOncePerSourceRows();
  theAbsentNodeRunDisclosesItsSourceOnce();
  theBufferRows();
  theEdgeRows();
  theStandingMethodRows();
  theLateNodeRows();
  theNodeThatAnswersFromTheFirstGapOnward();
  theFallingNodeRows();
  theItemFoldRows();
  theDetectorNameRow();
  std::printf(failures == 0 ? "ok\n" : "%d checks failed\n", failures);
  return failures == 0 ? 0 : 1;
}
