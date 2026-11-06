// ============================================================================
// T002 (specs/048-hwlat-probe): the detector engine. The setup writes the
// mask, the `per-cpu` mode, the two knob values, the threshold, the engine
// word and the trace switch in that order, saves the six values it
// overwrites, and restores them in the reverse order on every path the
// process controls (FR-018, FR-019, FR-020, FR-021, FR-022, FR-023). Wave
// W2 writes the rows; this unit stands open with the frameworkless
// check()/fail() convention, the way harness_noise_test carries it.
//
// T021 to T024 (wave W2): the seven writes in the standing order, the six
// reads ahead of the first one, the restore in the reverse of the write
// order, the three figures folded from the buffer text of each tracer cpu,
// and the paths that answer the not-available shape with the run whole.
// Every row stands over a fake tracefs tree under the file-root seam (FR-001,
// FR-017). The order rows read one inotify watch over that tree: the kernel
// queues one event per `read` and one per `write`, so a row names the order
// the session worked in with no sleep and no clock.
// ============================================================================

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <sched.h>
#include <signal.h>
#include <sys/inotify.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../../source/detail/proc_root.hpp"
#include "detail/jitter_probe.hpp"
#include "detail/noise_tracer.hpp"
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

// The check with the text a red row prints under its own line: one literal
// per message, so no row breaks a message across two statements.
auto checkWith(const bool cond,
               const char* what,
               const std::string& detail) -> void
{
  check(cond, what);
  if (!cond) {
    std::printf("        %s\n", detail.c_str());
  }
}

// The cpu indexes the rows name, each one inside the process affinity mask
// (specs/036 FR-008): the value parser validates every cpu of a `cpus=`
// list against that mask, so no row names a cpu the host lacks.
auto pickCpus(std::size_t count) -> std::vector<std::uint32_t>
{
  cpu_set_t mask {};
  CPU_ZERO(&mask);
  if (sched_getaffinity(0, sizeof(mask), &mask) != 0) {
    fail("the process affinity mask is unreadable (FR-008)");
    return {};
  }
  std::vector<std::uint32_t> cpus;
  for (std::size_t index = 0;
       index < static_cast<std::size_t>(CPU_SETSIZE) && cpus.size() < count;
       ++index)
  {
    if (CPU_ISSET(static_cast<unsigned int>(index), &mask) != 0) {
      cpus.push_back(static_cast<std::uint32_t>(index));
    }
  }
  if (cpus.size() < count) {
    fail("the affinity mask holds too few cpus for the row (FR-008)");
  }
  return cpus;
}

// The whole text of one file.
auto readStream(const std::filesystem::path& path) -> std::string
{
  std::ifstream file(path);
  std::ostringstream text;
  text << file.rdbuf();
  return text.str();
}

// One line of one failure message: the names a row read, in order.
auto joined(const std::vector<std::string>& names) -> std::string
{
  std::string text;
  for (const auto& name : names) {
    if (!text.empty()) {
      text += "; ";
    }
    text += name;
  }
  return text;
}

// The seam's root stands restored when the row ends, the way the standing
// install and clear seams stand cleared by the test. One fresh root per
// instance, so no row reads the files another row wrote.
class TracingRootGuard
{
public:
  TracingRootGuard()
  {
    static int sequence = 0;
    m_root = std::filesystem::path("harness_hwlat_tracer_root_")
        / ("pid" + std::to_string(::getpid()) + "-"
           + std::to_string(sequence++));
    std::filesystem::create_directories(m_root);
    sg::detail::installTestProcRoot(m_root);
  }

  ~TracingRootGuard()
  {
    sg::detail::installTestProcRoot(std::nullopt);
    std::error_code removeFailed;
    std::filesystem::remove_all(m_root, removeFailed);
  }

  TracingRootGuard(const TracingRootGuard&) = delete;
  auto operator=(const TracingRootGuard&) -> TracingRootGuard& = delete;

  auto root() const -> const std::filesystem::path& { return m_root; }

private:
  std::filesystem::path m_root;
};

// One fake file below the tracing root, the text written whole.
auto writeTracingFile(const std::filesystem::path& root,
                      const std::string& relative,
                      const std::string& text) -> void
{
  const auto path = root / std::filesystem::path(relative);
  std::filesystem::create_directories(path.parent_path());
  std::ofstream file(path);
  check(static_cast<bool>(file), "the fake tracing file opens (T021)");
  file << text;
}

// The standing tree: the six files the saved set covers, each with the text
// the kernel holds before the run, and `tracing_on` at `0`. The mode file
// carries the read form `none [round-robin] per-cpu` the kernel prints
// (`kernel/trace/trace_hwlat.c:629` to `:642`), and the two knobs and the
// threshold carry the kernel's own defaults (spec A-3 cites them).
auto installDetectorTree(const std::filesystem::path& root,
                         const std::string& maskText = "ffffffff") -> void
{
  writeTracingFile(root, "sys/kernel/tracing/current_tracer", "nop\n");
  writeTracingFile(root, "sys/kernel/tracing/tracing_cpumask", maskText + "\n");
  writeTracingFile(root,
                   "sys/kernel/tracing/hwlat_detector/mode",
                   "none [round-robin] per-cpu\n");
  writeTracingFile(root, "sys/kernel/tracing/hwlat_detector/width", "500000\n");
  writeTracingFile(
      root, "sys/kernel/tracing/hwlat_detector/window", "1000000\n");
  writeTracingFile(root, "sys/kernel/tracing/tracing_thresh", "10\n");
  writeTracingFile(root, "sys/kernel/tracing/tracing_on", "0\n");
}

// One denied control path of the fake tree: the standing file replaced by a
// directory, so the open fails for every user, the root of a CI container
// included (FR-017).
auto denyTracingPath(const std::filesystem::path& root,
                     const std::string& relative) -> bool
{
  const auto path = root / std::filesystem::path(relative);
  return std::filesystem::remove(path)
      && std::filesystem::create_directory(path);
}

// The mask text of one cpu set at `width` hex digits per word,
// comma-separated, the leftmost word naming the highest cpus, bit i of word
// w naming cpu w * width * 4 + i (R-02, R-05): the shape the mask write
// renders the tracer cpus in, at the width the standing text carried.
auto maskTextOf(const std::vector<std::uint32_t>& cpus,
                std::size_t width) -> std::string
{
  const std::size_t bitsPerWord = width * 4;
  std::size_t wordCount = 1;
  for (const auto cpu : cpus) {
    const std::size_t needed = cpu / bitsPerWord + 1;
    if (needed > wordCount) {
      wordCount = needed;
    }
  }
  std::string text;
  for (std::size_t at = 0; at < wordCount; ++at) {
    const std::size_t word = wordCount - 1 - at;
    std::uint64_t bits = 0;
    for (const auto cpu : cpus) {
      if (cpu / bitsPerWord == word) {
        bits |= std::uint64_t {1} << (cpu % bitsPerWord);
      }
    }
    std::ostringstream digits;
    digits << std::hex << std::setw(static_cast<int>(width))
           << std::setfill('0') << bits;
    if (!text.empty()) {
      text += ',';
    }
    text += digits.str();
  }
  return text;
}

// One read or one write of one file below the tracing root, in the order the
// kernel queued the event.
struct TracingStep
{
  std::string path;
  bool written = false;
};

// The order the session reads and writes the fake tree: one inotify
// descriptor watches the tracing directory and the detector's own directory,
// and the kernel queues one `IN_ACCESS` per read and one `IN_MODIFY` per
// write, so a row reads the order off one queue with no sleep and no clock
// (T021, T022). The queue drains when the row asks, so a row that drains
// after the setup and again after the restore reads the two calls apart.
class TracingEventLog
{
public:
  explicit TracingEventLog(const std::filesystem::path& root)
  {
    m_fd = ::inotify_init1(IN_NONBLOCK);
    if (m_fd < 0) {
      fail("the event watch stands unusable (T021)");
      return;
    }
    const auto tracing = root / "sys/kernel/tracing";
    m_tracingWatch = addWatch(tracing);
    m_detectorWatch = addWatch(tracing / "hwlat_detector");
  }

  TracingEventLog(const TracingEventLog&) = delete;
  auto operator=(const TracingEventLog&) -> TracingEventLog& = delete;

  ~TracingEventLog()
  {
    if (m_fd >= 0) {
      ::close(m_fd);
    }
  }

  auto steps() -> std::vector<TracingStep>
  {
    std::vector<TracingStep> seen;
    if (m_fd < 0) {
      return seen;
    }
    char buffer[8192] {};
    for (;;) {
      const auto filled = ::read(m_fd, buffer, sizeof(buffer));
      if (filled <= 0) {
        break;
      }
      append(buffer, static_cast<std::size_t>(filled), seen);
    }
    return seen;
  }

private:
  static constexpr std::uint32_t kMask = IN_ACCESS | IN_MODIFY;

  auto addWatch(const std::filesystem::path& path) -> int
  {
    const auto watch = ::inotify_add_watch(m_fd, path.c_str(), kMask);
    if (watch < 0) {
      fail("the event watch stands unset (T021)");
    }
    return watch;
  }

  auto prefixOf(const int watch) const -> std::string
  {
    if (watch == m_detectorWatch) {
      return "sys/kernel/tracing/hwlat_detector/";
    }
    return "sys/kernel/tracing/";
  }

  auto append(const char* buffer,
              std::size_t filled,
              std::vector<TracingStep>& seen) const -> void
  {
    std::size_t offset = 0;
    while (offset + sizeof(inotify_event) <= filled) {
      const auto* event =
          reinterpret_cast<const inotify_event*>(buffer + offset);
      offset += sizeof(inotify_event) + event->len;
      if (event->len == 0U) {
        continue;
      }
      TracingStep step {prefixOf(event->wd) + std::string(event->name),
                        (event->mask & IN_MODIFY) != 0U};
      // One write reaches the queue once per `write`, and one read once per
      // `read`, so a repeat of one file's own kind names no second step.
      if (!seen.empty() && seen.back().path == step.path
          && seen.back().written == step.written)
      {
        continue;
      }
      seen.push_back(step);
    }
  }

  int m_fd = -1;
  int m_tracingWatch = -1;
  int m_detectorWatch = -1;
};

// The written paths, in the order the writes landed, one entry per file.
auto writtenOrder(const std::vector<TracingStep>& steps)
    -> std::vector<std::string>
{
  std::vector<std::string> order;
  for (const auto& step : steps) {
    if (step.written && (order.empty() || order.back() != step.path)) {
      order.push_back(step.path);
    }
  }
  return order;
}

// The paths read ahead of the first write, first read first, one entry per
// file.
auto readAheadOfFirstWrite(const std::vector<TracingStep>& steps)
    -> std::vector<std::string>
{
  std::vector<std::string> order;
  for (const auto& step : steps) {
    if (step.written) {
      break;
    }
    if (std::find(order.begin(), order.end(), step.path) == order.end()) {
      order.push_back(step.path);
    }
  }
  return order;
}

// One saved set against the set the row expects: the same pairs in the same
// order.
auto sameSaved(const std::vector<sg::detail::TracerSetting>& saved,
               const std::vector<sg::detail::TracerSetting>& wanted) -> bool
{
  if (saved.size() != wanted.size()) {
    return false;
  }
  for (std::size_t at = 0; at < wanted.size(); ++at) {
    if (saved.at(at).path != wanted.at(at).path
        || saved.at(at).value != wanted.at(at).value)
    {
      return false;
    }
  }
  return true;
}

// The saved set in one text, so a red row names the pairs it read.
auto describeSaved(const std::vector<sg::detail::TracerSetting>& saved)
    -> std::string
{
  std::string text;
  for (const auto& pair : saved) {
    if (!text.empty()) {
      text += "; ";
    }
    text += pair.path + "=" + pair.value;
  }
  return text;
}

// The run's own file-size limit stands below one write's text: the kernel
// refuses that write for every user, the root of a CI container included
// (FR-017), so the denial needs no permission change. The standing limit and
// the standing signal disposition stand restored when the guard ends.
class SetupWriteLimit
{
public:
  explicit SetupWriteLimit(rlim_t limit)
  {
    m_savedSignal = ::signal(SIGXFSZ, SIG_IGN);
    if (m_savedSignal == SIG_ERR) {
      fail("the file-size signal stands unrestorable (T022)");
      return;
    }
    if (::getrlimit(RLIMIT_FSIZE, &m_saved) != 0) {
      fail("the file-size limit stands unreadable (T022)");
      return;
    }
    rlimit limited = m_saved;
    limited.rlim_cur = limit;
    if (::setrlimit(RLIMIT_FSIZE, &limited) != 0) {
      fail("the file-size limit stands unset (T022)");
      return;
    }
    m_lowered = true;
  }

  SetupWriteLimit(const SetupWriteLimit&) = delete;
  auto operator=(const SetupWriteLimit&) -> SetupWriteLimit& = delete;

  ~SetupWriteLimit()
  {
    if (m_lowered) {
      ::setrlimit(RLIMIT_FSIZE, &m_saved);
    }
    if (m_savedSignal != SIG_ERR) {
      ::signal(SIGXFSZ, m_savedSignal);
    }
  }

private:
  rlimit m_saved {};
  void (*m_savedSignal)(int) = SIG_DFL;
  bool m_lowered = false;
};

// The path of one tracer cpu's buffer text, the file
// `kernel/trace/trace.c:7672` creates (FR-018, A-6, P-10).
auto bufferPath(std::uint32_t cpu) -> std::string
{
  return "sys/kernel/tracing/per_cpu/cpu" + std::to_string(cpu) + "/trace";
}

// The legend lines a buffer header carries. Each starts with `#` and a
// space, or with `#` alone, so none carries a digit in column 2 (A-6).
const char* const kLegend = "# tracer: hwlat\n"
                            "#\n"
                            "# entries-in-buffer/entries-written: 3/4   #P:8\n"
                            "#\n"
                            "#                    _------=> CPU#\n"
                            "#                   / _-----=> irqs-off\n";

// One detection line in the shape `kernel/trace/trace_output.c:1531`
// prints: `#` and the sequence number, the two microsecond figures, the
// time stamp, the count, and the `nmi-count` field only when that count
// stands above zero (P-10).
auto detectionLine(std::uint32_t sequence,
                   std::uint64_t inner,
                   std::uint64_t outer,
                   std::uint64_t nmi) -> std::string
{
  std::ostringstream line;
  line << '#' << std::left << std::setw(5) << sequence << std::right
       << " inner/outer(us): " << std::setw(4) << inner << '/' << std::left
       << std::setw(5) << outer << std::right << " ts:0000000000000000 count:1";
  if (nmi > 0) {
    line << " nmi-count:" << nmi;
  }
  line << '\n';
  return line.str();
}

// ---- T021: the setup order (FR-018 to FR-020, D-9, D-10, P-14, A-3)

// T021, FR-019, FR-021, D-8, E-07, P-16: the setup reads the standing text
// of each of the six saved files ahead of its first write, `current_tracer`
// first, and the session carries the value of each one. The saved value of
// `hwlat_detector/mode` holds the bracketed word alone, because the mode
// file's write handler compares one stripped word. `tracing_on` stands
// outside the set, so the setup reads no text there.
auto theSetupReadsTheSixAheadOfItsFirstWrite() -> void
{
  const auto cpus = pickCpus(1);
  if (cpus.empty()) {
    return;
  }
  TracingRootGuard guard;
  installDetectorTree(guard.root());
  TracingEventLog log(guard.root());
  const auto session =
      sg::detail::noiseTracerSetup(sg::TracerEngine::HWLAT, cpus);
  const auto steps = log.steps();
  const auto reads = readAheadOfFirstWrite(steps);
  std::vector<std::string> sorted = reads;
  std::sort(sorted.begin(), sorted.end());
  const std::vector<std::string> wanted {
      "sys/kernel/tracing/current_tracer",
      "sys/kernel/tracing/hwlat_detector/mode",
      "sys/kernel/tracing/hwlat_detector/width",
      "sys/kernel/tracing/hwlat_detector/window",
      "sys/kernel/tracing/tracing_cpumask",
      "sys/kernel/tracing/tracing_thresh"};
  checkWith(sorted == wanted,
            "the six saved files stand read ahead of the first write (T021, "
            "FR-019, D-8)",
            joined(reads));
  checkWith(
      !reads.empty() && reads.front() == "sys/kernel/tracing/current_tracer",
      "the word file stands read first (T021, FR-019)",
      joined(reads));
  check(std::find(reads.begin(), reads.end(), "sys/kernel/tracing/tracing_on")
            == reads.end(),
        "the setup reads no on-off text (T021, D-8, E-07)");
  check(session.has_value(), "the setup answers the session (T021, FR-019)");
  if (!session.has_value()) {
    return;
  }
  const std::vector<sg::detail::TracerSetting> wantedSaved {
      {"sys/kernel/tracing/tracing_cpumask", "ffffffff"},
      {"sys/kernel/tracing/hwlat_detector/mode", "round-robin"},
      {"sys/kernel/tracing/hwlat_detector/width", "500000"},
      {"sys/kernel/tracing/hwlat_detector/window", "1000000"},
      {"sys/kernel/tracing/tracing_thresh", "10"},
      {"sys/kernel/tracing/current_tracer", "nop"}};
  checkWith(sameSaved(session->saved, wantedSaved),
            "the saved set holds the six standing values in write order, the "
            "mode word alone (T021, FR-019, D-8, P-16)",
            describeSaved(session->saved));
  sg::detail::noiseTracerRestore(*session);
}

// T021, FR-018 to FR-020, D-9, D-10, P-14, A-3: the setup writes the seven
// files in the standing order, the mask in the width the standing mask
// carried, the mode with the word `per-cpu`, the two knobs with the two
// constants with the width ahead of the window, the threshold in
// nanoseconds, then the engine word, then the start.
auto theSetupWritesTheSevenInOrder() -> void
{
  const auto cpus = pickCpus(2);
  if (cpus.size() < 2) {
    return;
  }
  TracingRootGuard guard;
  installDetectorTree(guard.root());
  TracingEventLog log(guard.root());
  const auto session =
      sg::detail::noiseTracerSetup(sg::TracerEngine::HWLAT, cpus);
  const auto written = writtenOrder(log.steps());
  const std::vector<std::string> wanted {
      "sys/kernel/tracing/tracing_cpumask",
      "sys/kernel/tracing/hwlat_detector/mode",
      "sys/kernel/tracing/hwlat_detector/width",
      "sys/kernel/tracing/hwlat_detector/window",
      "sys/kernel/tracing/tracing_thresh",
      "sys/kernel/tracing/current_tracer",
      "sys/kernel/tracing/tracing_on"};
  checkWith(written == wanted,
            "the seven writes stand in the standing order (T021, FR-019, D-9, "
            "P-14)",
            joined(written));
  check(session.has_value(), "the setup answers the session (T021, FR-019)");
  if (!session.has_value()) {
    return;
  }
  const auto at = [&](const char* name)
  { return readStream(guard.root() / name); };
  check(at("sys/kernel/tracing/tracing_cpumask") == maskTextOf(cpus, 8) + "\n",
        "the mask write carries the tracer cpus in the standing width "
        "(T021, FR-019, D-10)");
  check(at("sys/kernel/tracing/hwlat_detector/mode") == "per-cpu\n",
        "the mode write carries the word per-cpu (T021, FR-019, D-9, P-14)");
  check(at("sys/kernel/tracing/hwlat_detector/width") == "5000\n",
        "the width write carries the 5000 us constant (T021, FR-020, A-3)");
  check(at("sys/kernel/tracing/hwlat_detector/window") == "10000\n",
        "the window write carries the 10000 us constant (T021, FR-020, A-3)");
  check(at("sys/kernel/tracing/tracing_thresh") == "1000000\n",
        "the threshold write carries the 1000 us constant in nanoseconds "
        "(T021, FR-020, A-3)");
  check(at("sys/kernel/tracing/current_tracer") == "hwlat\n",
        "the engine word stands written (T021, FR-018, FR-019)");
  check(at("sys/kernel/tracing/tracing_on") == "1\n",
        "the start write stands last (T021, FR-019)");
  sg::detail::noiseTracerRestore(*session);
}

// T021, FR-019, E-07: the mask render takes its width from the standing
// mask text, so a standing word of two digits renders a two-digit mask.
auto theMaskWriteTakesTheWidthTheStandingMaskCarried() -> void
{
  const auto cpus = pickCpus(1);
  if (cpus.empty()) {
    return;
  }
  TracingRootGuard guard;
  installDetectorTree(guard.root(), "ff");
  const auto session =
      sg::detail::noiseTracerSetup(sg::TracerEngine::HWLAT, cpus);
  check(session.has_value(), "the setup answers the session (T021, FR-019)");
  if (!session.has_value()) {
    return;
  }
  check(readStream(guard.root() / "sys/kernel/tracing/tracing_cpumask")
            == maskTextOf(cpus, 2) + "\n",
        "the mask renders in the width the standing text carried (T021, "
        "FR-019, E-07)");
  sg::detail::noiseTracerRestore(*session);
}

// ---- T022: the restore order (FR-003, FR-021, FR-022, D-8, D-12, D-15,
// P-8, P-9, SC-007)

// T022, FR-021, D-8, P-8, SC-007: the restore writes `tracing_on` at `0`
// first, then each saved value in the reverse of the write order, so the
// engine word goes back before the threshold and the kernel's own reset runs
// ahead of the host threshold. Each saved file then holds the value the run
// saved, and the mode file holds the one word the run saved.
auto theRestoreWritesTheStopThenTheSavedSetInReverse() -> void
{
  const auto cpus = pickCpus(2);
  if (cpus.size() < 2) {
    return;
  }
  TracingRootGuard guard;
  installDetectorTree(guard.root());
  TracingEventLog log(guard.root());
  const auto session =
      sg::detail::noiseTracerSetup(sg::TracerEngine::HWLAT, cpus);
  check(session.has_value(), "the setup answers the session (T022, FR-019)");
  if (!session.has_value()) {
    return;
  }
  log.steps();
  sg::detail::noiseTracerRestore(*session);
  const auto written = writtenOrder(log.steps());
  const std::vector<std::string> wanted {
      "sys/kernel/tracing/tracing_on",
      "sys/kernel/tracing/current_tracer",
      "sys/kernel/tracing/tracing_thresh",
      "sys/kernel/tracing/hwlat_detector/window",
      "sys/kernel/tracing/hwlat_detector/width",
      "sys/kernel/tracing/hwlat_detector/mode",
      "sys/kernel/tracing/tracing_cpumask"};
  checkWith(written == wanted,
            "the restore writes the stop, then the saved set in reverse "
            "(T022, FR-021, P-8)",
            joined(written));
  const auto at = [&](const char* name)
  { return readStream(guard.root() / name); };
  check(at("sys/kernel/tracing/tracing_on") == "0\n",
        "the restore writes the on-off file at 0 (T022, FR-021, SC-007)");
  check(at("sys/kernel/tracing/current_tracer") == "nop\n",
        "the word file stands back at its standing text (T022, FR-021)");
  check(at("sys/kernel/tracing/tracing_cpumask") == "ffffffff\n",
        "the mask file stands back at its standing text (T022, FR-021)");
  check(at("sys/kernel/tracing/tracing_thresh") == "10\n",
        "the threshold file stands back at the host value (T022, FR-021, D-8)");
  check(at("sys/kernel/tracing/hwlat_detector/width") == "500000\n",
        "the width file stands back at its standing text (T022, FR-021)");
  check(at("sys/kernel/tracing/hwlat_detector/window") == "1000000\n",
        "the window file stands back at its standing text (T022, FR-021)");
  check(at("sys/kernel/tracing/hwlat_detector/mode") == "round-robin\n",
        "the mode file stands back at the one saved word (T022, P-16)");
}

// T022, FR-003, FR-021, D-15, P-9: a setup that fails after one write
// restores the file it wrote before it answers the empty session. The run's
// file-size limit stands at four bytes: the mask render of one cpu at the
// one-digit width stands within it, and the mode write of eight bytes stands
// beyond it, so the first write lands and the second one fails.
auto theSetupThatFailsAfterOneWriteRestoresWhatItWrote() -> void
{
  const auto cpus = pickCpus(1);
  if (cpus.empty()) {
    return;
  }
  const std::string render = maskTextOf(cpus, 1);
  if (render.size() + 1 > 4) {
    // The row bounds the mask write, so the row needs that write to land
    // under the limit: a cpu at or above 16 needs a wider render.
    std::printf("skipped: the mask render holds %zu bytes (T022)\n",
                render.size() + 1);
    return;
  }
  TracingRootGuard guard;
  installDetectorTree(guard.root(), "1");
  TracingEventLog log(guard.root());
  const SetupWriteLimit limit(4);
  const auto session =
      sg::detail::noiseTracerSetup(sg::TracerEngine::HWLAT, cpus);
  const auto written = writtenOrder(log.steps());
  const std::vector<std::string> wanted {"sys/kernel/tracing/tracing_cpumask",
                                         "sys/kernel/tracing/tracing_cpumask"};
  check(!session.has_value(),
        "the failed setup answers the empty session (T022, FR-021, D-15)");
  checkWith(written == wanted,
            "the setup writes the mask once, then restores it (T022, FR-021, "
            "P-9, D-15)",
            joined(written));
  check(
      readStream(guard.root() / "sys/kernel/tracing/tracing_cpumask") == "1\n",
      "the mask file stands back at its standing text (T022, FR-021, D-15)");
  check(
      readStream(guard.root() / "sys/kernel/tracing/current_tracer") == "nop\n",
      "the word file stands unwritten (T022, FR-021, FR-024)");
  check(readStream(guard.root() / "sys/kernel/tracing/tracing_on") == "0\n",
        "the on-off file stands unwritten (T022, FR-021, D-8)");
}

// T022, FR-022, D-12: a restore write that fails answers one pair naming the
// file and the saved value, and the restore stays a disclosure ahead of the
// status the run holds (FR-003).
auto aFailedRestoreWriteNamesItsFileAndItsValue() -> void
{
  const auto cpus = pickCpus(1);
  if (cpus.empty()) {
    return;
  }
  TracingRootGuard guard;
  installDetectorTree(guard.root());
  const auto session =
      sg::detail::noiseTracerSetup(sg::TracerEngine::HWLAT, cpus);
  check(session.has_value(), "the setup answers the session (T022, FR-019)");
  if (!session.has_value()) {
    return;
  }
  check(
      denyTracingPath(guard.root(), "sys/kernel/tracing/hwlat_detector/window"),
      "the window path stands a directory (T022, FR-017)");
  const auto failed = sg::detail::noiseTracerRestore(*session);
  checkWith(failed.size() == 1,
            "the restore answers one failed write (T022, FR-022, D-12)",
            describeSaved(failed));
  if (failed.size() != 1) {
    return;
  }
  checkWith(failed.front().path
                == "sys/kernel/tracing/hwlat_detector/window"
            && failed.front().value == "1000000",
            "the failed pair names the file and its saved value (T022, "
            "FR-022, D-12)",
            describeSaved(failed));
}

// ---- T023: the three figures (FR-018, FR-027, A-6, P-10)

// T023, FR-018, FR-027, A-6, P-10: one entry per tracer cpu in session
// order, `maxGapNs` the larger of the two microsecond figures times 1000,
// `gapCount` the lines of that cpu, and `nmiCount` the sum of the
// `nmi-count` fields.
auto theFiguresFoldTheLinesOfEachCpuBuffer() -> void
{
  const auto cpus = pickCpus(2);
  if (cpus.size() < 2) {
    return;
  }
  TracingRootGuard guard;
  installDetectorTree(guard.root());
  writeTracingFile(guard.root(),
                   bufferPath(cpus.at(0)),
                   std::string(kLegend) + detectionLine(0, 1234, 5678, 2)
                       + detectionLine(1, 999, 1000, 0));
  writeTracingFile(guard.root(),
                   bufferPath(cpus.at(1)),
                   std::string(kLegend) + detectionLine(0, 12000, 11000, 5));
  const std::vector<std::uint32_t> sessionCpus {cpus.at(1), cpus.at(0)};
  const auto session =
      sg::detail::noiseTracerSetup(sg::TracerEngine::HWLAT, sessionCpus);
  check(session.has_value(), "the setup answers the session (T023, FR-019)");
  if (!session.has_value()) {
    return;
  }
  const auto figures = sg::detail::noiseTracerFigures(*session);
  check(figures.size() == 2,
        "the read answers one entry per tracer cpu (T023, FR-027)");
  if (figures.size() != 2) {
    sg::detail::noiseTracerRestore(*session);
    return;
  }
  check(figures.at(0).cpu == cpus.at(1) && figures.at(1).cpu == cpus.at(0),
        "the entries stand in the session order (T023, FR-027)");
  check(figures.at(0).maxGapNs == 12000000 && figures.at(0).gapCount == 1
            && figures.at(0).nmiCount == 5,
        "the first cpu folds its one line (T023, FR-027, A-6, P-10)");
  check(figures.at(1).maxGapNs == 5678000
          && figures.at(1).gapCount == 2 && figures.at(1).nmiCount == 2,
        "the second cpu folds the larger of its two lines (T023, FR-027, "
        "A-6, P-10)");
  sg::detail::noiseTracerRestore(*session);
}

// T023, FR-018, FR-027, A-6, P-10: a line counts only when `#` stands at
// column 1 and a digit at column 2, so each header legend line stands
// skipped and the count names the one detection line.
auto theColumnRuleSkipsTheLegendLines() -> void
{
  const auto cpus = pickCpus(1);
  if (cpus.empty()) {
    return;
  }
  TracingRootGuard guard;
  installDetectorTree(guard.root());
  writeTracingFile(guard.root(),
                   bufferPath(cpus.at(0)),
                   std::string(kLegend)
                       + "#  cmd     pid     |||||| time  |   caller\n"
                       + detectionLine(0, 250, 300, 0));
  const auto session =
      sg::detail::noiseTracerSetup(sg::TracerEngine::HWLAT, cpus);
  check(session.has_value(), "the setup answers the session (T023, FR-019)");
  if (!session.has_value()) {
    return;
  }
  const auto figures = sg::detail::noiseTracerFigures(*session);
  check(figures.size() == 1,
        "the read answers one entry per tracer cpu (T023, FR-027)");
  if (figures.size() != 1) {
    sg::detail::noiseTracerRestore(*session);
    return;
  }
  check(figures.at(0).gapCount == 1 && figures.at(0).maxGapNs == 300000
          && figures.at(0).nmiCount == 0,
        "the legend lines stand skipped and the one line counts (T023, "
        "FR-027, A-6, P-10)");
  sg::detail::noiseTracerRestore(*session);
}

// T023, FR-027, A-6, P-10: `nmiCount` sums the `nmi-count` fields of the
// cpu, the field standing behind a `nmi-total` field as it stands alone, and
// answers 0 where lines stand and none carries one.
auto theNmiCountSumsItsFieldAcrossLines() -> void
{
  const auto cpus = pickCpus(1);
  if (cpus.empty()) {
    return;
  }
  TracingRootGuard guard;
  installDetectorTree(guard.root());
  writeTracingFile(guard.root(), bufferPath(cpus.at(0)),
                   detectionLine(0, 400, 700, 2)
                   + "#0     inner/outer(us):  800/600  "
                     "ts:0000000000000000 count:1 nmi-total:99 nmi-count:3\n"
                   + detectionLine(2, 100, 200, 0));
  const auto session =
      sg::detail::noiseTracerSetup(sg::TracerEngine::HWLAT, cpus);
  check(session.has_value(), "the setup answers the session (T023, FR-019)");
  if (!session.has_value()) {
    return;
  }
  const auto figures = sg::detail::noiseTracerFigures(*session);
  check(figures.size() == 1,
        "the read answers one entry per tracer cpu (T023, FR-027)");
  if (figures.size() != 1) {
    sg::detail::noiseTracerRestore(*session);
    return;
  }
  check(figures.at(0).gapCount == 3 && figures.at(0).nmiCount == 5
          && figures.at(0).maxGapNs == 800000,
        "the sum names the field of each line, the total field aside (T023, "
        "FR-027, A-6, P-10)");
  sg::detail::noiseTracerRestore(*session);
}

// T023, FR-027, A-6: a text with no detection line answers all three
// figures absent, the same answer a cpu with no buffer text answers.
auto aBufferWithNoLineAnswersTheThreeFiguresAbsent() -> void
{
  const auto cpus = pickCpus(2);
  if (cpus.size() < 2) {
    return;
  }
  TracingRootGuard guard;
  installDetectorTree(guard.root());
  writeTracingFile(guard.root(), bufferPath(cpus.at(0)), kLegend);
  const std::vector<std::uint32_t> sessionCpus {cpus.at(0), cpus.at(1)};
  const auto session =
      sg::detail::noiseTracerSetup(sg::TracerEngine::HWLAT, sessionCpus);
  check(session.has_value(), "the setup answers the session (T023, FR-019)");
  if (!session.has_value()) {
    return;
  }
  const auto figures = sg::detail::noiseTracerFigures(*session);
  check(figures.size() == 2,
        "the read answers one entry per tracer cpu (T023, FR-027)");
  if (figures.size() != 2) {
    sg::detail::noiseTracerRestore(*session);
    return;
  }
  for (const auto& entry : figures) {
    check(!entry.maxGapNs.has_value() && !entry.gapCount.has_value()
              && !entry.nmiCount.has_value(),
          "the cpu with no detection line answers every figure absent (T023, "
          "FR-027, A-6)");
  }
  sg::detail::noiseTracerRestore(*session);
}

// ---- T024: the paths the run holds whole (FR-003, FR-021, FR-023, D-15,
// plan R-5, plan R-6)

// The scripted provider: the time source the whole-run rows read through,
// the shape `harness_noise_test` scripts. The tsc series steps one tick per
// read, so every window keeps no gap and the run stands quiet.
auto scriptedProvider() -> sg::counters::FakeProvider&
{
  static auto* provider = []
  {
    auto owned = std::make_unique<sg::counters::FakeProvider>();
    auto& ref = *owned;
    ref.addCounter("machine", "monotonic", "nanoseconds", "host time");
    ref.setPoints("machine", "monotonic", {0}, 4000);
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
    check(registered.has_value(), "the scripted provider registers (T024)");
    return &ref;
  }();
  return *provider;
}

// The whole-run rows' benchmark: one quiet body, so the pre-check window
// stands the run's only measured span.
auto bmQuiet(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

// The one command line of the whole-run rows: the pre-check mode, the tracer
// word with the named tracer cpu, and the document the row reads.
auto runArguments(const std::string& tracerValue,
                  const std::string& tag) -> std::vector<std::string>
{
  return {"--filter=bmQuiet",
          "--repetitions=1",
          "--iterations=2",
          "--jitter-probe=idle,mode=precheck,precheck=1,threshold=1000000",
          tracerValue,
          "--json=harness_hwlat_" + tag + ".json"};
}

// One speedgunMain run in this process, its report captured to a file the
// row removes: the row reads the status, and the suite's own output stays
// the row's own lines.
auto captureRun(const std::vector<std::string>& arguments) -> int
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_hwlat_tracer_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }
  static int runIndex = 0;
  const std::string path =
      "harness_hwlat_run_" + std::to_string(++runIndex) + ".txt";
  const int saved = ::dup(STDOUT_FILENO);
  if (saved < 0 || std::freopen(path.c_str(), "w", stdout) == nullptr) {
    fail("the report stands unredirectable (T024)");
  }
  const int status =
      sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
  std::fflush(stdout);
  if (::dup2(saved, STDOUT_FILENO) < 0) {
    fail("stdout stands unrestored (T024)");
  }
  ::close(saved);
  std::remove(path.c_str());
  return status;
}

// The user-space figures of one run: the pre-check record and its cpu
// entries, the shape spec FR-023 keeps whole.
auto theUserSpaceStandsWhole(const std::string& jsonPath) -> void
{
  const auto loaded = sg::loadReport(jsonPath);
  checkWith(loaded.has_value(),
            "the run writes the document (T024, FR-023)",
            loaded.has_value() ? "written" : loaded.error());
  if (!loaded.has_value()) {
    return;
  }
  check(loaded->precheck.has_value() && !loaded->precheck->cpus.empty(),
        "the user-space figures stand whole (T024, FR-023)");
}

// T024, FR-003, FR-021, FR-023: no tracefs stands at the standing path. The
// run reaches the restore list, writes no file, keeps the user-space figures
// whole and answers status 0.
auto theMissingTracefsRunStandsWhole() -> void
{
  const auto cpus = pickCpus(1);
  if (cpus.empty()) {
    return;
  }
  TracingRootGuard guard;
  const std::string json = "harness_hwlat_missing.json";
  const auto status = captureRun(runArguments(
      "--os-noise=hwlat,cpus=" + std::to_string(cpus.at(0)), "missing"));
  check(status == 0,
        "the run without a tracefs tree stands status 0 (T024, FR-023)");
  theUserSpaceStandsWhole(json);
  check(!std::filesystem::exists(guard.root()
                                 / "sys/kernel/tracing/current_tracer"),
        "the run wrote no file (T024, FR-024)");
  std::remove(json.c_str());
}

// T024, FR-003, FR-023, D-15: one knob stands without rights. The run
// reaches the restore list, the tree stands as it stood, and the run keeps
// status 0 with the user-space figures whole.
auto theDeniedKnobRunStandsWhole() -> void
{
  const auto cpus = pickCpus(1);
  if (cpus.empty()) {
    return;
  }
  TracingRootGuard guard;
  installDetectorTree(guard.root());
  check(denyTracingPath(guard.root(),
                        "sys/kernel/tracing/hwlat_detector/" "width"),
        "the width knob stands a directory (T024, FR-017)");
  const std::string json = "harness_hwlat_denied.json";
  const auto status = captureRun(runArguments(
      "--os-noise=hwlat,cpus=" + std::to_string(cpus.at(0)), "denied"));
  check(status == 0,
        "the run whose knob stands denied holds status 0 (T024, FR-023)");
  theUserSpaceStandsWhole(json);
  check(
      readStream(guard.root() / "sys/kernel/tracing/current_tracer") == "nop\n",
      "the word file stands restored (T024, FR-021, D-15)");
  check(readStream(guard.root() / "sys/kernel/tracing/tracing_cpumask")
            == "ffffffff\n",
        "the mask file stands restored (T024, FR-021, D-15)");
  check(readStream(guard.root() / "sys/kernel/tracing/tracing_on") == "0\n",
        "the on-off file stands down (T024, FR-021, SC-007)");
  std::remove(json.c_str());
}

// T024, FR-021, FR-023, D-15: the step the window owns fails, because the
// buffer path of one tracer cpu stands a directory the run can neither read
// nor write. The restore list still runs, and the run keeps status 0 with
// the user-space figures whole.
auto theInWindowFailureStillRestoresTheTree() -> void
{
  const auto cpus = pickCpus(1);
  if (cpus.empty()) {
    return;
  }
  TracingRootGuard guard;
  installDetectorTree(guard.root());
  check(std::filesystem::create_directories(
            guard.root() / std::filesystem::path(bufferPath(cpus.at(0)))),
        "the buffer path stands a directory (T024, FR-017)");
  const std::string json = "harness_hwlat_window.json";
  const auto status = captureRun(runArguments(
      "--os-noise=hwlat,cpus=" + std::to_string(cpus.at(0)), "window"));
  check(status == 0,
        "the run whose window step fails holds status 0 (T024, FR-021)");
  theUserSpaceStandsWhole(json);
  check(readStream(guard.root() / "sys/kernel/tracing/tracing_on") == "0\n",
        "the restore writes the on-off file at 0 (T024, FR-021, D-15)");
  check(
      readStream(guard.root() / "sys/kernel/tracing/current_tracer") == "nop\n",
      "the restore returns the standing word (T024, FR-021, D-15)");
  check(
      readStream(guard.root() / "sys/kernel/tracing/tracing_thresh") == "10\n",
      "the restore returns the host threshold (T024, FR-021, P-8)");
  std::remove(json.c_str());
}

// The reader of the interrupted row: the stamp steps one tick per read, and
// the read past the bound raises the interrupt signal inside the window, the
// way `harness_jitter_cli_test` raises it inside a body. The signal lands
// while the session stands live, ahead of the restore call.
class InterruptedReader final : public sg::counters::WindowReader
{
public:
  auto readPoints(sg::counters::PointSink& sink) noexcept -> void override
  {
    if (m_reads.fetch_add(1) == kInterruptAt && !m_fired.exchange(true)) {
      std::raise(SIGINT);
    }
    sink.put(m_stamp.fetch_add(1));
  }

private:
  static constexpr std::uint64_t kInterruptAt = 200;

  std::atomic<std::uint64_t> m_reads {0};
  std::atomic<std::uint64_t> m_stamp {0};
  std::atomic<bool> m_fired {false};
};

// The exit status of one forked child that runs the detector under the
// interrupt, with both streams captured and cleared: the interrupt stands a
// process-wide fact, so the run stands in a child either way.
auto runInterruptedChild(const std::vector<std::string>& arguments,
                         const std::string& tag) -> int
{
  const std::string outPath = "harness_hwlat_" + tag + "-out.txt";
  const std::string errPath = "harness_hwlat_" + tag + "-err.txt";
  std::fflush(stdout);
  std::fflush(stderr);
  const pid_t pid = fork();
  if (pid < 0) {
    fail("cannot fork the child (T024)");
    return 3;
  }
  if (pid == 0) {
    if (std::freopen(outPath.c_str(), "w", stdout) == nullptr
        || std::freopen(errPath.c_str(), "w", stderr) == nullptr)
    {
      _exit(3);
    }
    sg::detail::installTestReader(std::make_unique<InterruptedReader>());
    std::vector<char*> argv;
    argv.push_back(const_cast<char*>("harness_hwlat_tracer_test"));
    for (const auto& argument : arguments) {
      argv.push_back(const_cast<char*>(argument.c_str()));
    }
    const int status =
        sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
    std::fflush(stdout);
    std::fflush(stderr);
    std::exit(status);
  }
  int waitStatus = 0;
  if (waitpid(pid, &waitStatus, 0) < 0) {
    fail("cannot wait for the child (T024)");
    return 3;
  }
  if (!WIFEXITED(waitStatus)) {
    fail("the child did not exit normally (T024)");
    return 3;
  }
  std::remove(outPath.c_str());
  std::remove(errPath.c_str());
  return WEXITSTATUS(waitStatus);
}

// T024, FR-003, FR-021, D-15: the interrupt arrives inside the window. The
// restore list still runs and leaves every saved file at the value the tree
// held before the run, the user-space figures stand whole, and the tracer
// changes no status: the run holds the standing interrupt status.
auto theInterruptedChildRestoresTheTree() -> void
{
  const auto cpus = pickCpus(1);
  if (cpus.empty()) {
    return;
  }
  TracingRootGuard guard;
  installDetectorTree(guard.root());
  const std::string json = "harness_hwlat_interrupted.json";
  const auto status = runInterruptedChild(
      runArguments("--os-noise=hwlat,cpus=" + std::to_string(cpus.at(0)),
                   "interrupted"),
      "interrupted");
  check(status == 1,
        "the interrupted run holds the standing interrupt status (T024, "
        "FR-003)");
  theUserSpaceStandsWhole(json);
  check(
      readStream(guard.root() / "sys/kernel/tracing/current_tracer") == "nop\n",
      "the restore returns the standing word (T024, FR-021, D-15)");
  check(readStream(guard.root() / "sys/kernel/tracing/tracing_cpumask")
            == "ffffffff\n",
        "the restore returns the standing mask (T024, FR-021, D-15)");
  check(readStream(guard.root() / "sys/kernel/tracing/hwlat_detector/mode")
            == "round-robin\n",
        "the restore returns the saved mode word (T024, FR-021, P-16)");
  check(
      readStream(guard.root() / "sys/kernel/tracing/tracing_thresh") == "10\n",
      "the restore returns the host threshold (T024, FR-021, P-8)");
  check(readStream(guard.root() / "sys/kernel/tracing/tracing_on") == "0\n",
        "the restore writes the on-off file at 0 (T024, FR-021, SC-007)");
  std::remove(json.c_str());
}

}  // namespace

SG_BENCHMARK(bmQuiet);

auto main() -> int
{
  // The scripted provider stands the whole-run rows' time source, installed
  // ahead of the fork so the child inherits the registration with the rest
  // of the image.
  scriptedProvider();
  theSetupReadsTheSixAheadOfItsFirstWrite();
  theSetupWritesTheSevenInOrder();
  theMaskWriteTakesTheWidthTheStandingMaskCarried();
  theRestoreWritesTheStopThenTheSavedSetInReverse();
  theSetupThatFailsAfterOneWriteRestoresWhatItWrote();
  aFailedRestoreWriteNamesItsFileAndItsValue();
  theFiguresFoldTheLinesOfEachCpuBuffer();
  theColumnRuleSkipsTheLegendLines();
  theNmiCountSumsItsFieldAcrossLines();
  aBufferWithNoLineAnswersTheThreeFiguresAbsent();
  theMissingTracefsRunStandsWhole();
  theDeniedKnobRunStandsWhole();
  theInWindowFailureStillRestoresTheTree();
  theInterruptedChildRestoresTheTree();
  std::printf(failures == 0 ? "ok\n" : "%d checks failed\n", failures);
  return failures == 0 ? 0 : 1;
}
