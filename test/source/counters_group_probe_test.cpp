// ============================================================================
// The group probe over a scripted kernel (specs/041-counter-probe, T010;
// FR-010 to FR-021, F-1 to F-5).
//
// Every kernel answer the search reads stands in this file: the grant, the
// refusal with each errno, the size that runs short, and the read that
// answers no pair. No scenario reaches the host kernel, so the refusing arms
// carry their counts on a runner that refuses every counter open
// (FR-011, FR-073, PC-1). The scripted answers follow the reading F-1 to F-5
// recorded on the Zen 5 host: every open answers a descriptor, and the group
// stops running whole above the count the device schedules.
//
// The seam's fields are plain function references, so nothing captures: the
// scripted state travels through the file-scope holders below, and
// `resetFixture()` clears them and the install between scenarios.
//
// Frameworkless check()/fail() convention.
// ============================================================================

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <vector>

#include <linux/perf_event.h>
#include <sys/mman.h>

#include "../source/counters/detail/group_probe.hpp"
#include "../source/counters/linux_pmu/kernel_calls.hpp"

namespace
{

auto failures = int {0};

auto check(const bool ok, const char* sentence) -> void
{
  if (!ok) {
    std::printf("GROUP PROBE TEST FAIL: %s\n", sentence);
    ++failures;
  }
}

using sg::counters::detail::GroupProbeOptions;
using sg::counters::detail::GroupProbeStatus;
using sg::counters::detail::installKernelCalls;
using sg::counters::detail::kernelCalls;
using sg::counters::detail::kGroupProbeMaxSize;
using sg::counters::detail::probeGroupLimit;

// What one recorded open carries. The search passes the attribute through
// the seam as an untyped pointer, so the scripted kernel reads it back in
// the shape the UAPI declares (FR-013).
struct OpenRecord
{
  std::uint64_t type = 0;
  std::uint64_t config = 0;
  int pid = 0;
  int cpu = 0;
  int groupFd = 0;
  int fd = 0;
  unsigned long flags = 0;
  unsigned long long excludeKernel = 0;
  unsigned long long excludeHv = 0;
  unsigned long long disabled = 0;
  unsigned long long pinned = 0;
  unsigned long long readFormat = 0;
};

// The scripted kernel's state.
auto gOpens = std::vector<OpenRecord> {};
auto gRefuseAt = int {-1};  ///< the open ordinal that refuses; -1 none
auto gRefuseErrno = int {EACCES};
auto gMaxFit = std::size_t {6};  ///< sizes at or below this run whole
auto gWriteWords = std::size_t {3};  ///< the words the scripted read writes
auto gEnabledWord = std::uint64_t {4000};  ///< the `time_enabled` word the read
                                           ///< answers; zero started nothing
auto gOpened = int {0};
auto gClosed = int {0};
auto gReads = int {0};
auto gNextFd = int {100};
auto gGroupSize = std::size_t {0};  ///< members of the group last joined

// The device every scenario searches: a type number no host publishes, so a
// search that leaked a real open would answer a refusal this file cannot
// script. The event is the branch-instructions word D-9 names.
constexpr std::uint32_t kScriptedType = 41;
constexpr std::uint64_t kBranchInstructions = 0x0005;

auto scriptedOpen(
    void* attr, int pid, int cpu, int groupFd, unsigned long flags) -> long
{
  const auto* posted = static_cast<const perf_event_attr*>(attr);
  gOpens.push_back(OpenRecord {
      .type = posted->type,
      .config = posted->config,
      .pid = pid,
      .cpu = cpu,
      .groupFd = groupFd,
      .fd = 0,
      .flags = flags,
      .excludeKernel = posted->exclude_kernel,
      .excludeHv = posted->exclude_hv,
      .disabled = posted->disabled,
      .pinned = posted->pinned,
      .readFormat = posted->read_format,
  });
  auto& record = gOpens.back();
  gGroupSize = groupFd < 0 ? 1 : gGroupSize + 1;
  if (gRefuseAt >= 0 && static_cast<int>(gOpens.size() - 1) == gRefuseAt) {
    errno = gRefuseErrno;
    return -1;
  }
  record.fd = gNextFd;
  ++gOpened;
  return gNextFd++;
}

// The read of a group leader: the leader's count, the enabled window, and the
// running window. The pair follows the size of the group the last open joined,
// which is how F-1's reading answers the search (F-3).
auto scriptedRead(int, void* buf, std::size_t count) -> long
{
  ++gReads;
  const bool whole = gGroupSize <= gMaxFit;
  const std::uint64_t enabled = gEnabledWord;
  const std::uint64_t words[3] = {17, enabled, whole ? enabled : 1000};
  auto* posted = static_cast<std::uint64_t*>(buf);
  const std::size_t written = std::min(gWriteWords, std::size_t {3});
  for (std::size_t index = 0; index < written; ++index) {
    posted[index] = words[index];
  }
  const auto bytes = written * sizeof(std::uint64_t);
  // Both sides are a byte count, and the cast names the one signed type the
  // seam's answer settles in.
  // NOLINTNEXTLINE(modernize-use-integer-sign-comparison)
  return static_cast<long>(std::min(bytes, count));
}

auto scriptedClose(int) -> int
{
  ++gClosed;
  return 0;
}

// The scripted set every scenario stands: the three calls the search issues
// answer above, and the rest refuse without touching the host, the way
// `counters_linux_pmu_seam_test.cpp` fills all nine fields (FR-011).
auto scriptedCalls() -> sg::counters::detail::KernelCalls
{
  auto calls = sg::counters::detail::KernelCalls {};
  calls.openEvent = &scriptedOpen;
  calls.mapPage = [](void*, std::size_t, int, int, int, long long) -> void*
  { return MAP_FAILED; };
  calls.unmapPage = [](void*, std::size_t) -> int { return 0; };
  calls.readGroup = &scriptedRead;
  calls.ioctlEvent = [](int, unsigned long, unsigned long) -> int { return 0; };
  calls.closeFd = &scriptedClose;
  calls.devicesRoot = []() -> std::filesystem::path { return {}; };
  calls.paranoidPath = []() -> std::filesystem::path { return {}; };
  calls.cpuidWords = []() -> sg::counters::detail::CpuidWords { return {}; };
  return calls;
}

// Clears the record and the install, so no scenario reads another's answers.
auto resetFixture() -> void
{
  gOpens.clear();
  gRefuseAt = -1;
  gRefuseErrno = EACCES;
  gMaxFit = 6;
  gWriteWords = 3;
  gEnabledWord = 4000;
  gOpened = 0;
  gClosed = 0;
  gReads = 0;
  gNextFd = 100;
  gGroupSize = 0;
  installKernelCalls(nullptr);
}

auto options() -> GroupProbeOptions
{
  return GroupProbeOptions {
      .deviceName = "cpu",
      .pmuType = kScriptedType,
      .eventConfig = kBranchInstructions,
  };
}

// The opens a search issues to reach size `size`, which is 1 + 2 + ... +
// size: every copy of every size is one open (FR-012).
auto opensThrough(std::size_t size) -> int
{
  return static_cast<int>(size * (size + 1) / 2);
}

// The release claim FR-019 states for every path: the search closed exactly
// what it opened, on the refusals included.
auto checkReleased(const char* path) -> void
{
  if (gOpened != gClosed) {
    std::fprintf(
        stderr, "%s path: opened %d, closed %d\n", path, gOpened, gClosed);
  }
  check(gOpened == gClosed, path);
}

auto runsShortAtSevenAnswersSixScenario() -> void
{
  auto calls = scriptedCalls();
  resetFixture();
  installKernelCalls(&calls);
  gMaxFit = 6;
  const auto answer = probeGroupLimit(options());
  check(answer.count.has_value() && *answer.count == 6,
        "the search answers 6, the largest size that ran whole (FR-015, F-1)");
  check(answer.status == GroupProbeStatus::RUNS_SHORT,
        "the size that ran short names the stop (FR-015)");
  check(gOpened == opensThrough(7),
        "seven sizes opened 28 copies, which is F-5's open count (F-5)");
  check(gReads == 7, "one read settled each of the seven sizes (FR-014)");
  checkReleased("runs-short");
  resetFixture();
}

auto refusedOpenAnswersTheSizeBeforeScenario() -> void
{
  auto calls = scriptedCalls();
  resetFixture();
  installKernelCalls(&calls);
  gMaxFit = kGroupProbeMaxSize;
  gRefuseAt = opensThrough(6);  // the first open of size 7
  gRefuseErrno = EINVAL;
  const auto answer = probeGroupLimit(options());
  check(answer.count.has_value() && *answer.count == 6,
        "a refused open at 7 answers 6, the size that fitted (FR-015)");
  check(answer.status == GroupProbeStatus::OPEN_REFUSED,
        "a refused open names the stop (FR-015)");
  check(gOpened == opensThrough(6),
        "the refused size opened no descriptor of its own (FR-012)");
  checkReleased("open-refused");
  resetFixture();
}

auto permissionRefusalAnswersNothingScenario() -> void
{
  struct Row
  {
    int err;
    bool refuses;
    const char* why;
  };

  static constexpr Row kRows[] = {
      {EACCES, true, "EACCES is the gate refusal FR-018 names"},
      {EPERM, true, "EPERM is the gate refusal FR-018 names"},
      {EINVAL, false, "EINVAL is an encoding refusal, not a gate refusal"},
  };
  for (const auto& row : kRows) {
    auto calls = scriptedCalls();
    resetFixture();
    installKernelCalls(&calls);
    gRefuseAt = 0;
    gRefuseErrno = row.err;
    const auto answer = probeGroupLimit(options());
    check(!answer.count.has_value(), row.why);
    check(row.refuses ? answer.status == GroupProbeStatus::PERMISSION_REFUSED
                      : answer.status == GroupProbeStatus::OPEN_REFUSED,
          row.why);
    check(gOpened == 0, "a refusal at size 1 opened no descriptor (FR-012)");
    checkReleased(row.why);
    resetFixture();
  }
}

auto shortReadAnswersNothingScenario() -> void
{
  auto calls = scriptedCalls();
  resetFixture();
  installKernelCalls(&calls);
  gWriteWords = 1;  // the read answers the count alone, so no pair stands
  const auto answer = probeGroupLimit(options());
  check(!answer.count.has_value(),
        "a read with no pair settles no size, so nothing is known (FR-014)");
  check(answer.status == GroupProbeStatus::READ_SHORT,
        "a short read names the stop (FR-015)");
  check(gReads == 1, "the search stopped at the first size it could not read");
  checkReleased("read-short");
  resetFixture();
}

// The read answers all three words and both windows of the pair stand at zero,
// which is a group the kernel never started. FR-014 settles a size on a
// `time_enabled` past zero beside an equal pair, so the pair of zeroes answers
// a size that did not run whole and the search stops at the first size, where
// no bound stands below it (FR-014, FR-017).
auto zeroPairAnswersNoBoundScenario() -> void
{
  auto calls = scriptedCalls();
  resetFixture();
  installKernelCalls(&calls);
  gMaxFit = kGroupProbeMaxSize;  // every size answers a pair of equal words
  gEnabledWord = 0;  // and both words of that pair stand at zero
  const auto answer = probeGroupLimit(options());
  check(!answer.count.has_value(),
        "a pair of zeroes is a group the kernel never started, so the search "
        "holds no bound (FR-014, FR-017)");
  check(answer.status == GroupProbeStatus::RUNS_SHORT,
        "the equal pair of zeroes answers a size that did not run whole "
        "(FR-014)");
  check(gOpened == 1 && gReads == 1,
        "the search settled size 1 on its one read and tried no larger size "
        "(FR-015)");
  checkReleased("zero-pair");
  resetFixture();
}

auto sizeOneShortRunAnswersNothingScenario() -> void
{
  auto calls = scriptedCalls();
  resetFixture();
  installKernelCalls(&calls);
  gMaxFit = 0;
  const auto answer = probeGroupLimit(options());
  check(!answer.count.has_value(),
        "a size 1 that did not run leaves no bound below it (FR-017)");
  check(answer.status == GroupProbeStatus::RUNS_SHORT,
        "the short run of size 1 names the stop (FR-015)");
  check(gOpened == 1 && gReads == 1,
        "the search stopped at size 1 and tried no larger size (FR-015)");
  checkReleased("size-one-short");
  resetFixture();
}

auto everySizeFittingAnswersNothingScenario() -> void
{
  auto calls = scriptedCalls();
  resetFixture();
  installKernelCalls(&calls);
  gMaxFit = kGroupProbeMaxSize;
  const auto answer = probeGroupLimit(options());
  check(!answer.count.has_value(),
        "a search that never found a bound answers no value (FR-016)");
  check(answer.status == GroupProbeStatus::CAP_REACHED,
        "the cap names the stop, and no run reports a limit of 32 (FR-016)");
  check(gOpened == opensThrough(kGroupProbeMaxSize),
        "the search tried 1 to 32 and no larger size (FR-016, D-4)");
  checkReleased("cap-reached");
  resetFixture();
}

auto groupFdNamesTheLeaderScenario() -> void
{
  auto calls = scriptedCalls();
  resetFixture();
  installKernelCalls(&calls);
  gMaxFit = 2;  // sizes 1 and 2 fit, size 3 stops the search
  const auto answer = probeGroupLimit(options());
  check(answer.count.has_value() && *answer.count == 2,
        "the scripted shape reaches the size 3 stop (FR-015)");
  const auto last = gOpens.size() - 3;  // size 3's three copies
  check(gOpens[last].groupFd == -1,
        "the first copy of a size leads the group (FR-012)");
  check(gOpens[last + 1].groupFd == gOpens[last].fd
            && gOpens[last + 2].groupFd == gOpens[last].fd,
        "every later copy names that leader as its group_fd (FR-012)");
  checkReleased("group-fd");
  resetFixture();
}

auto attributeCarriesTheMeasuredShapeScenario() -> void
{
  auto calls = scriptedCalls();
  resetFixture();
  installKernelCalls(&calls);
  gMaxFit = 2;
  (void)probeGroupLimit(options());
  check(!gOpens.empty(), "the scripted kernel recorded the search's opens");
  for (const auto& record : gOpens) {
    check(record.type == kScriptedType && record.config == kBranchInstructions,
          "the copy carries the device's type and the event the rule named "
          "(FR-010, D-9)");
    check(record.excludeKernel == 1 && record.excludeHv == 1
              && record.disabled == 1 && record.pinned == 0,
          "the attribute excludes the kernel, starts the group disabled, and "
          "clears pinned (FR-013)");
    check(record.pid == 0 && record.cpu == -1,
          "the copy binds the calling thread on any cpu, the shape F-1 "
          "measured (FR-013)");
    check(record.readFormat
              == (PERF_FORMAT_TOTAL_TIME_ENABLED
                  | PERF_FORMAT_TOTAL_TIME_RUNNING),
          "the read format asks for the pair F-3 settles a size with (FR-014)");
  }
  checkReleased("attribute-shape");
  resetFixture();
}

}  // namespace

auto main() -> int
{
  runsShortAtSevenAnswersSixScenario();
  refusedOpenAnswersTheSizeBeforeScenario();
  permissionRefusalAnswersNothingScenario();
  shortReadAnswersNothingScenario();
  zeroPairAnswersNoBoundScenario();
  sizeOneShortRunAnswersNothingScenario();
  everySizeFittingAnswersNothingScenario();
  groupFdNamesTheLeaderScenario();
  attributeCarriesTheMeasuredShapeScenario();
  if (failures != 0) {
    return 1;
  }
  std::printf("counters_group_probe_test PASS: every scripted kernel answer "
              "settles the size the spec states\n");
  return EXIT_SUCCESS;
}
