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

// The number of times one whole line stands in one captured report.
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

// The whole line one captured report carries behind one prefix, an empty
// answer when no line carries it.
auto lineStarting(const std::string& text,
                  const std::string& start) -> std::string
{
  const auto found = text.find(start);
  if (found == std::string::npos) {
    return {};
  }
  return text.substr(found, text.find('\n', found) - found);
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
// file-size limit stands at the byte count of the mask write the row
// FR-017: the denial stands a directory in the place of the on-off file, so the
// open of that write fails for every user, the root of a CI container included.
// The path bounds no other write of the process: a file-size limit bounds every
// write in it, the writes of the unit itself among them. The on-off file is the
// one control the setup writes and never reads, so the read set stays whole and
// the refusal lands on a write. Four facts stand observable: the log names no
// file beyond the six the setup writes, the last write names the mask, the mask
// file stands back at its standing text, which the render differs from, and the
// denied file stands unwritten. The queue merges one file's consecutive writes,
// so the write of a restored file and its restore read as one step.
auto theSetupWhoseDeniedWriteFailsRestoresWhatItWrote() -> void
{
  const auto cpus = pickCpus(1);
  if (cpus.empty()) {
    return;
  }
  const std::string render = maskTextOf(cpus, 1);
  TracingRootGuard guard;
  installDetectorTree(guard.root(), "e");
  const std::string denied = "sys/kernel/tracing/tracing_on";
  check(denyTracingPath(guard.root(), denied),
        "the on-off path stands a directory (T022, FR-017)");
  TracingEventLog log(guard.root());
  const auto session =
      sg::detail::noiseTracerSetup(sg::TracerEngine::HWLAT, cpus);
  const auto written = writtenOrder(log.steps());
  const std::vector<std::string> touched {
      "sys/kernel/tracing/tracing_cpumask",
      "sys/kernel/tracing/hwlat_detector/mode",
      "sys/kernel/tracing/hwlat_detector/width",
      "sys/kernel/tracing/hwlat_detector/window",
      "sys/kernel/tracing/tracing_thresh",
      "sys/kernel/tracing/current_tracer"};
  bool bounded = true;
  for (const auto& path : written) {
    bounded = bounded
        && std::find(touched.begin(), touched.end(), path) != touched.end();
  }
  check(!session.has_value(),
        "the failed setup answers the empty session (T022, FR-021, D-15)");
  checkWith(bounded,
            "the setup writes no file beyond the writes it made (T022, FR-021, "
            "D-15)",
            joined(written));
  checkWith(!written.empty() && written.back() == touched.front(),
            "the last write restores the mask the setup wrote (T022, FR-021, "
            "P-9, D-15)",
            joined(written));
  check(render != "e",
        "the render differs from the standing mask text, so the restore of the "
        "mask stands observable (T022, P-9)");
  check(
      readStream(guard.root() / "sys/kernel/tracing/tracing_cpumask") == "e\n",
      "the mask file stands back at its standing text (T022, FR-021, D-15, "
      "P-9)");
  check(
      readStream(guard.root() / "sys/kernel/tracing/current_tracer") == "nop\n",
      "the word file stands back at its standing text (T022, FR-021, FR-024, "
      "P-9)");
  check(std::find(written.begin(), written.end(), denied) == written.end(),
        "the denied file stands unwritten (T022, FR-021, D-8, FR-017)");
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

// The pre-check span the whole-run rows name: `precheck=1`, one
// millisecond, in the nanosecond base the reader stamps (FR-011). The
// interrupted row's script takes the same span, so the option string and the
// script cannot drift apart.
constexpr std::uint64_t kPrecheckSpanNs = 1'000'000;

// The samples the interrupted row holds for that span.
constexpr std::size_t kWindowSamples = 1'000;

// The one command line of the whole-run rows: the pre-check mode, the tracer
// word, both on one named cpu, and the document the row reads. The probe
// names its cpu: the `idle` preset resolves over the batch's plans, and these
// rows pin no benchmark, so the preset would plan no cpu at all.
auto runArguments(std::uint32_t cpu,
                  const std::string& tag,
                  const std::string& probeTail = "") -> std::vector<std::string>
{
  const std::string list = std::to_string(cpu);
  return {"--filter=bmQuiet",
          "--repetitions=1",
          "--iterations=2",
          "--jitter-probe=" + list + ",mode=precheck,precheck="
              + std::to_string(kPrecheckSpanNs / 1'000'000)
              + ",threshold=1000000" + probeTail,
          "--os-noise=hwlat,cpus=" + list,
          "--json=harness_hwlat_" + tag + ".json"};
}

// The report of one in-process run: the status the entry point answered, and
// the bytes the report printed (T024, FR-023).
struct RunReport
{
  int status = 0;
  std::string text;
};

// One speedgunMain run in this process, its report captured to a file the row
// removes and read back: a row reads the status, and a row that reads the block
// the console printed reads the text. The suite's own output stays the row's
// own lines.
auto captureRunReport(const std::vector<std::string>& arguments) -> RunReport
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
  RunReport report;
  report.status = sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
  std::fflush(stdout);
  if (::dup2(saved, STDOUT_FILENO) < 0) {
    fail("stdout stands unrestored (T024)");
  }
  ::close(saved);
  report.text = readStream(std::filesystem::path(path));
  std::remove(path.c_str());
  return report;
}

// The status of one run, the shape the standing rows read.
auto captureRun(const std::vector<std::string>& arguments) -> int
{
  return captureRunReport(arguments).status;
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
  const auto status = captureRun(runArguments(cpus.at(0), "missing"));
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
  const auto status = captureRun(runArguments(cpus.at(0), "denied"));
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

// T024, FR-020, FR-027, D-12: a whole detector run names the three settings it
// wrote to the detector in both surfaces. The console block states them on one
// line below the engine word, and the document carries the three keys inside
// `precheck.tracer`, the values the setup wrote (A-3). The run's record carries
// them, so neither surface reads a constant of its own.
auto theWholeDetectorRunNamesItsThreeSettings() -> void
{
  const auto cpus = pickCpus(1);
  if (cpus.empty()) {
    return;
  }
  const auto lineOf = [](const std::string& text,
                         const std::string& start) -> std::string
  {
    const auto found = text.find(start);
    if (found == std::string::npos) {
      return "no line begins " + start;
    }
    return text.substr(found, text.find('\n', found) - found);
  };
  const auto figure = [](const std::optional<std::uint64_t>& value) {
    return value.has_value() ? std::to_string(*value) : std::string("absent");
  };
  TracingRootGuard guard;
  installDetectorTree(guard.root());
  const std::string json = "harness_hwlat_settings.json";
  const auto report = captureRunReport(runArguments(cpus.at(0), "settings"));
  check(report.status == 0,
        "the whole detector run holds status 0 (T024, FR-023)");
  checkWith(lineOf(report.text, "tracer: windowUs=")
                == "tracer: windowUs=10000 widthUs=5000 thresholdUs=1000",
            "the block states the three settings below the engine word "
            "(FR-020, FR-027, FR-023, D-12)",
            lineOf(report.text, "tracer: windowUs="));
  const auto loaded = sg::loadReport(json);
  checkWith(loaded.has_value(),
            "the run writes the document (T024, FR-023)",
            loaded.has_value() ? "written" : loaded.error());
  if (!loaded.has_value() || !loaded->precheck.has_value()
      || !loaded->precheck->tracer.has_value())
  {
    std::remove(json.c_str());
    return;
  }
  const auto& tracer = *loaded->precheck->tracer;
  check(tracer.engine == "hwlat",
        "the document names the detector engine (FR-024, FR-027)");
  checkWith(tracer.windowUs.has_value() && *tracer.windowUs == 10'000
                && tracer.widthUs.has_value() && *tracer.widthUs == 5'000
                && tracer.thresholdUs.has_value()
                && *tracer.thresholdUs == 1'000,
            "the document carries the three settings inside the tracer object "
            "(FR-027, FR-020, A-3)",
            "windowUs=" + figure(tracer.windowUs) + " widthUs="
                + figure(tracer.widthUs)
                + " thresholdUs=" + figure(tracer.thresholdUs));
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
  const auto status = captureRun(runArguments(cpus.at(0), "window"));
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

// The stamps of one window script: `samples` reads across `spanNs`, one
// step apart. The reader owns this list, and the pre-check walk runs to its
// span, so the list carries the window's length (FR-011).
auto windowScript(std::uint64_t spanNs,
                  std::size_t samples) -> std::vector<std::uint64_t>
{
  std::vector<std::uint64_t> script;
  script.reserve(samples);
  const std::uint64_t step = spanNs / samples;
  for (std::size_t at = 0; at < samples; ++at) {
    script.push_back(at * step);
  }
  return script;
}

// The reader of the interrupted row: the raise stands at the middle of the
// script the reader carries, so the raise stands inside the window, because
// the script owns the window's length. Past the script the walk keeps the
// script's own step, so the stamps keep rising and the window reaches its
// span. The raise reaches the whole process, the way
// `harness_jitter_cli_test` raises it inside a body, and it lands while the
// session stands live, ahead of the restore call.
class InterruptedReader final : public sg::counters::WindowReader
{
public:
  explicit InterruptedReader(std::vector<std::uint64_t> script)
      : m_script(std::move(script))
      , m_raiseAt(m_script.size() / 2)
      , m_step(m_script.size() > 1 ? m_script.at(1) - m_script.at(0) : 1)
  {
  }

  auto readPoints(sg::counters::PointSink& sink) noexcept -> void override
  {
    const auto index = m_reads.fetch_add(1);
    if (index == m_raiseAt) {
      std::raise(SIGINT);
    }
    const std::uint64_t stamp = index < m_script.size()
        ? m_script.at(index)
        : m_script.back() + (index - m_script.size() + 1) * m_step;
    sink.put(stamp);
  }

private:
  std::vector<std::uint64_t> m_script;
  std::size_t m_raiseAt = 0;
  std::uint64_t m_step = 1;
  std::atomic<std::uint64_t> m_reads {0};
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
    sg::detail::installTestReader(std::make_unique<InterruptedReader>(
        windowScript(kPrecheckSpanNs, kWindowSamples)));
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
      runArguments(cpus.at(0), "interrupted"), "interrupted");
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

// Whether the scheduled read of the reader below found the file and stood the
// denial in it. The run's session takes the reader and the session dies with
// the run, so the flag stands outside both for the row to read, the way the
// probe suite carries the flag of its scripted writes (T022, FR-017, FR-022).
std::atomic<bool> denialPlanted {false};

// The reader of the failed-restore row: one scheduled read replaces one saved
// file with a directory, so the write the restore owes that file stands denied
// while every setup write stood through. The reader owns the moment, so the
// denial stands planted inside the window ahead of the restore call, with no
// sleep and no clock read. Past the scheduled read the stamps keep the step of
// the window script, so the window reaches its span (T022, FR-021, FR-022,
// D-12, D-15).
class DeniedRestoreReader final : public sg::counters::WindowReader
{
public:
  DeniedRestoreReader(std::filesystem::path denied, std::size_t readAt)
      : m_denied(std::move(denied))
      , m_readAt(readAt)
      , m_step(kPrecheckSpanNs / kWindowSamples)
  {
  }

  auto readPoints(sg::counters::PointSink& sink) noexcept -> void override
  {
    const auto index = m_reads.fetch_add(1);
    if (index == m_readAt) {
      std::error_code removed;
      std::error_code made;
      const bool taken = std::filesystem::remove(m_denied, removed) && !removed;
      const bool stood =
          std::filesystem::create_directory(m_denied, made) && !made;
      denialPlanted.store(taken && stood, std::memory_order_relaxed);
    }
    sink.put(index * m_step);
  }

private:
  std::filesystem::path m_denied;
  std::size_t m_readAt = 0;
  std::uint64_t m_step = 1;
  std::atomic<std::uint64_t> m_reads {0};
};

// T022, FR-003, FR-021, FR-022, FR-023, D-12, D-15: the setup stands whole, the
// window runs, and one restore write stands denied. The console names that file
// and the value it saved on one line, and the run keeps the status its path
// carried, because the standing rule holds the tracer out of the run's status.
// The control run schedules the same denial past the window's last read, so the
// same tree with the same reader prints no restore line and leaves every saved
// file standing restored.
auto aDeniedRestoreWriteKeepsTheStatusItsPathCarried() -> void
{
  const auto cpus = pickCpus(1);
  if (cpus.empty()) {
    return;
  }
  constexpr const char* kWindowFile =
      "sys/kernel/tracing/hwlat_detector/window";
  constexpr std::size_t kMidWindow = kWindowSamples / 2;
  {
    TracingRootGuard guard;
    installDetectorTree(guard.root());
    denialPlanted.store(false, std::memory_order_relaxed);
    sg::detail::installTestReader(std::make_unique<DeniedRestoreReader>(
        guard.root() / kWindowFile, kMidWindow));
    const auto report =
        captureRunReport(runArguments(cpus.at(0), "restore-denied"));
    const bool planted = denialPlanted.load(std::memory_order_relaxed);
    sg::detail::installTestReader(nullptr);
    const std::string json = "harness_hwlat_restore-denied.json";
    check(planted,
          "the scheduled read stood its denial over one saved file (T022, "
          "FR-022)");
    check(report.status == 0,
          "the run whose restore write stood denied keeps the status its path "
          "carried (T022, FR-003, FR-022, D-12)");
    check(countLine(report.text, "tracer: restore failed:") == 1,
          "the run states the failed write once (T022, FR-022)");
    checkWith(lineStarting(report.text, "tracer: restore failed:")
                  == "tracer: restore failed: sys/kernel/tracing/hwlat_detector/"
                     "window = 1000000",
              "the line names the file and the value it saved (T022, FR-022, "
              "US2)",
              lineStarting(report.text, "tracer: restore failed:"));
    theUserSpaceStandsWhole(json);
    check(std::filesystem::is_directory(guard.root() / kWindowFile),
          "the denied file stands the denial the run met, unwritten (T022, "
          "FR-022)");
    check(readStream(guard.root() / "sys/kernel/tracing/current_tracer")
              == "nop\n",
          "the undo still returns the standing word (T022, FR-021, D-15)");
    check(readStream(guard.root() / "sys/kernel/tracing/tracing_thresh")
              == "10\n",
          "the undo still returns the saved threshold (T022, FR-021, D-15)");
    std::remove(json.c_str());
  }
  {
    TracingRootGuard guard;
    installDetectorTree(guard.root());
    denialPlanted.store(false, std::memory_order_relaxed);
    sg::detail::installTestReader(std::make_unique<DeniedRestoreReader>(
        guard.root() / kWindowFile, kWindowSamples * 64));
    const auto report =
        captureRunReport(runArguments(cpus.at(0), "restore-whole"));
    const bool planted = denialPlanted.load(std::memory_order_relaxed);
    sg::detail::installTestReader(nullptr);
    const std::string json = "harness_hwlat_restore-whole.json";
    check(!planted,
          "the control read never came, so no denial stood (T022, FR-022)");
    check(report.status == 0,
          "the control run holds the same status (T022, FR-023)");
    check(report.text.find("tracer: restore failed:") == std::string::npos,
          "the whole restore prints no failed-write line (T022, FR-022)");
    check(
        readStream(guard.root() / kWindowFile) == "1000000\n",
        "the whole restore writes the saved window back (T022, FR-021, D-15)");
    std::remove(json.c_str());
  }
}

// ---- T059, T061, T089: the record the entry point builds, and the tracer
// that already stands (module M10)

// The vendor text the counter gate reads: one key and one word, the pair the
// gate answers supported on (FR-010, D-4).
constexpr const char* kIntelCpuinfo = "vendor_id\t: GenuineIntel\n";

// The register node of one cpu, the file spec D-3 names. The node answers one
// 8-byte count at the address of the system-management-interrupt counter, so a
// node of this length stands readable (FR-007, P-2).
constexpr std::size_t kMsrNodeBytes = 64;
constexpr std::size_t kSmiCountAddress = 0x34;

// One register node below the seam's root, with one count at the counter
// address, and the vendor text the gate reads ahead of it. The seam's writer
// takes any path below the root, so the two counter files ride it too.
auto installMsrNode(const std::filesystem::path& root,
                    std::uint32_t cpu,
                    std::uint64_t count) -> void
{
  writeTracingFile(root, "proc/cpuinfo", kIntelCpuinfo);
  const auto path =
      root / std::filesystem::path("dev/cpu/" + std::to_string(cpu) + "/msr");
  std::filesystem::create_directories(path.parent_path());
  std::ofstream file(path, std::ios::binary);
  check(static_cast<bool>(file), "the fake register node opens (T059)");
  std::string bytes(kMsrNodeBytes, '\0');
  for (std::size_t at = 0; at < 8U; ++at) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    bytes[kSmiCountAddress + at] =
        static_cast<char>((count >> (8U * at)) & 0xFFULL);
  }
  file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

// The `NMI` row of `proc/interrupts`: one column per cpu up to the named one,
// the count of that cpu standing the value given (FR-009).
auto nmiRowText(std::uint32_t cpu, std::uint64_t value) -> std::string
{
  std::ostringstream row;
  row << "NMI:";
  for (std::uint32_t column = 0; column <= cpu; ++column) {
    row << '\t' << (column == cpu ? value : 0ULL);
  }
  row << '\n';
  return row.str();
}

// The value behind the first `smi` key of one document, the blanks between the
// key and its value dropped. The reader keeps no record of the mark, so a row
// that reads a mark reads the document's own bytes (FR-026, FR-029).
auto smiMarkOf(const std::string& path) -> std::string
{
  const std::string text = readStream(std::filesystem::path(path));
  const auto key = text.find("\"smi\"");
  if (key == std::string::npos) {
    return "no smi key";
  }
  std::size_t at = key + 5U;
  while (at < text.size() && (text[at] == ':' || text[at] == ' ')) {
    ++at;
  }
  const auto end = text.find_first_of(",}", at);
  if (end == std::string::npos) {
    return text.substr(at);
  }
  return text.substr(at, end - at);
}

// The step of the scripted stamps: the loop period the run reads, and the one
// jump that stands the gap the row reads. The measurement pass reads
// `kPassReads` of them ahead of the pre-check window, so the jump stands behind
// that pass and inside the window.
constexpr std::uint64_t kStampStepNs = 1'000;
constexpr std::uint64_t kStampJumpNs = 2'000'000;
constexpr std::size_t kPassReads = 1'024;
constexpr std::size_t kJumpIndex = kPassReads + 16;
constexpr std::size_t kStampCount = kJumpIndex + 16;

// The stamps of the two raw-gap rows: one step apart, with one jump at
// `kJumpIndex`. The step keeps the window's span short, and the jump stands the
// run's one kept gap.
class GapScriptedReader final : public sg::counters::WindowReader
{
public:
  GapScriptedReader()
      : m_stamps(makeStamps())
  {
  }

  auto readPoints(sg::counters::PointSink& sink) noexcept -> void override
  {
    const auto index = m_reads.fetch_add(1);
    const std::uint64_t stamp = index < m_stamps.size()
        ? m_stamps.at(index)
        : m_stamps.back() + (index - m_stamps.size() + 1) * kStampStepNs;
    sink.put(stamp);
  }

private:
  static auto makeStamps() -> std::vector<std::uint64_t>
  {
    std::vector<std::uint64_t> stamps;
    stamps.reserve(kStampCount);
    std::uint64_t stamp = 0;
    for (std::size_t at = 0; at < kStampCount; ++at) {
      stamps.push_back(stamp);
      stamp += at == kJumpIndex ? kStampJumpNs : kStampStepNs;
    }
    return stamps;
  }

  std::vector<std::uint64_t> m_stamps;
  std::atomic<std::uint64_t> m_reads {0};
};

// The command line of the two raw-gap rows: the detector method, the pre-check
// window the scripted stamps span, the document, and the raw gaps the document
// carries. No tracer stands on the line: the mark belongs to the probe alone.
auto gapArguments(std::uint32_t cpu,
                  const std::string& tag) -> std::vector<std::string>
{
  const std::string list = std::to_string(cpu);
  return {"--filter=bmQuiet",
          "--repetitions=1",
          "--iterations=2",
          "--jitter-probe=" + list + ",mode=precheck,precheck="
              + std::to_string(kPrecheckSpanNs / 1'000'000)
              + ",threshold=1000000,method=hwlat",
          "--json=harness_hwlat_" + tag + ".json",
          "--json-raw=gaps"};
}

// T059, FR-015, FR-026, D-13, P-6: the register node answers, so the window
// holds two reads of the counter and the change prefix rides the kept gaps. The
// scripted stamps hold one jump above the threshold, so the run keeps one gap.
// The count stands unchanged across the window, so that gap's mark reads
// `NOT_SMI`.
auto theAnsweredSourceMarksItsKeptGap() -> void
{
  const auto cpus = pickCpus(1);
  if (cpus.empty()) {
    return;
  }
  TracingRootGuard guard;
  installMsrNode(guard.root(), cpus.at(0), 7U);
  sg::detail::installTestReader(std::make_unique<GapScriptedReader>());
  const std::string json = "harness_hwlat_marked.json";
  const auto report = captureRunReport(gapArguments(cpus.at(0), "marked"));
  check(report.status == 0,
        "the run of an answered source holds status 0 (T059, FR-015)");
  const auto mark = smiMarkOf(json);
  checkWith(mark == "\"NOT_SMI\"",
            "the kept gap of an answered source carries its mark (T059, "
            "FR-015, " "FR-026, P-3)",
            "mark=" + mark);
  std::remove(json.c_str());
}

// T059, FR-015, P-6: no node stands under the root, so the source answers
// nothing and the change prefix stays empty. The gap the same stamps keep
// marks nothing, the answer the writer states for a cpu whose source stood
// absent.
auto theAbsentSourceMarksNoKeptGap() -> void
{
  const auto cpus = pickCpus(1);
  if (cpus.empty()) {
    return;
  }
  TracingRootGuard guard;
  sg::detail::installTestReader(std::make_unique<GapScriptedReader>());
  const std::string json = "harness_hwlat_unmarked.json";
  const auto report = captureRunReport(gapArguments(cpus.at(0), "unmarked"));
  check(report.status == 0,
        "the run of an absent source holds status 0 (T059, FR-015)");
  const auto mark = smiMarkOf(json);
  checkWith(
      mark == "null",
      "the kept gap of an absent source marks nothing (T059, FR-015, P-6)",
      "mark=" + mark);
  std::remove(json.c_str());
}

// T061, FR-004, FR-027, D-6: one run names the detector method and the detector
// engine. The tracer buffer states its three figures, the probe reads its own
// counters, and the document keeps each figure on the keys of its own surface.
auto theTwoSurfacesRecordTheirOwnFigures() -> void
{
  const auto cpus = pickCpus(1);
  if (cpus.empty()) {
    return;
  }
  const auto figure = [](const std::optional<std::uint64_t>& value) {
    return value.has_value() ? std::to_string(*value) : std::string("absent");
  };
  TracingRootGuard guard;
  installDetectorTree(guard.root());
  writeTracingFile(guard.root(),
                   bufferPath(cpus.at(0)),
                   std::string(kLegend) + detectionLine(0, 12345, 12000, 7));
  writeTracingFile(guard.root(), "proc/interrupts", nmiRowText(cpus.at(0), 3U));
  const std::string json = "harness_hwlat_surfaces.json";
  const auto report =
      captureRunReport(runArguments(cpus.at(0), "surfaces", ",method=hwlat"));
  check(report.status == 0,
        "the run of both surfaces holds status 0 (T061, FR-004)");
  const auto loaded = sg::loadReport(json);
  if (!loaded.has_value() || !loaded->precheck.has_value()
      || !loaded->precheck->tracer.has_value()
      || !loaded->runJitterStats.has_value())
  {
    check(false,
          ("the run of both surfaces writes both records (T061, FR-027): "
           + (loaded.has_value() ? "a record stands absent" : loaded.error()))
              .c_str());
    std::remove(json.c_str());
    return;
  }
  const auto& tracerCpus = loaded->precheck->tracer->cpus;
  checkWith(tracerCpus.size() == 1,
            "the tracer record names its own cpu (T061, FR-027)",
            "entries=" + std::to_string(tracerCpus.size()));
  if (tracerCpus.size() != 1) {
    std::remove(json.c_str());
    return;
  }
  const auto& tracer = tracerCpus.at(0);
  checkWith(tracer.maxGapNs.has_value() && *tracer.maxGapNs == 12'345'000ULL
                && tracer.gapCount.has_value() && *tracer.gapCount == 1
                && tracer.nmiCount.has_value() && *tracer.nmiCount == 7,
            "the tracer record carries the three figures of its buffer "
            "(T061, FR-027)",
            "maxGapNs=" + figure(tracer.maxGapNs)
            + " gapCount=" + figure(tracer.gapCount)
            + " nmiCount=" + figure(tracer.nmiCount));
  const auto& folds = *loaded->runJitterStats;
  const auto probe =
      std::ranges::find(folds, cpus.at(0), &sg::JitterEntryFold::cpu);
  checkWith(probe != folds.end(),
            "the probe record names its own cpu (T061, FR-025)",
            "entries=" + std::to_string(folds.size()));
  if (probe == folds.end()) {
    std::remove(json.c_str());
    return;
  }
  const auto& stats = probe->stats;
  checkWith(!stats.smiDelta.has_value(),
            "the probe names the source its run could not read (T061, FR-025)",
            "smiDelta=" + figure(stats.smiDelta));
  checkWith(stats.nmiDelta.has_value() && *stats.nmiDelta == 0
                && stats.smiGapCount == 0,
            "the probe states its own two figures, apart from the tracer's "
            "(T061, FR-004, FR-025)",
            "nmiDelta=" + figure(stats.nmiDelta)
                + " smiGapCount=" + std::to_string(stats.smiGapCount));
  checkWith(
      stats.maxNs != 12'345'000ULL,
      "no figure of the tracer enters the probe's fold (T061, FR-004, D-6)",
      "maxNs=" + std::to_string(stats.maxNs));
  std::remove(json.c_str());
}

// T089, FR-019, FR-023, P-15: `current_tracer` already reads `hwlat`, so the
// kernel answers success without initializing the tracer. The session writes no
// file, the block names the standing tracer ahead of the standing
// not-available line, the user-space figures stand whole and the run holds
// status 0.
auto theStandingTracerWritesNoFile() -> void
{
  const auto cpus = pickCpus(1);
  if (cpus.empty()) {
    return;
  }
  const auto lineOf = [](const std::string& text,
                         const std::string& start) -> std::string
  {
    const auto found = text.find(start);
    if (found == std::string::npos) {
      return "no line begins " + start;
    }
    return text.substr(found, text.find('\n', found) - found);
  };
  TracingRootGuard guard;
  installDetectorTree(guard.root());
  writeTracingFile(
      guard.root(), "sys/kernel/tracing/current_tracer", "hwlat\n");
  const std::string json = "harness_hwlat_standing.json";
  const auto report = captureRunReport(runArguments(cpus.at(0), "standing"));
  check(report.status == 0,
        "the run whose tracer already stands holds status 0 (T089, FR-023)");
  const auto unchanged =
      [&](const std::string& relative, const std::string& text)
  {
    check(readStream(guard.root() / std::filesystem::path(relative)) == text,
          ("the standing tracer leaves " + relative
           + " as the tree held it (T089, FR-019)")
              .c_str());
  };
  unchanged("sys/kernel/tracing/current_tracer", "hwlat\n");
  unchanged("sys/kernel/tracing/tracing_cpumask", "ffffffff\n");
  unchanged("sys/kernel/tracing/hwlat_detector/mode",
            "none [round-robin] per-cpu\n");
  unchanged("sys/kernel/tracing/hwlat_detector/width", "500000\n");
  unchanged("sys/kernel/tracing/hwlat_detector/window", "1000000\n");
  unchanged("sys/kernel/tracing/tracing_thresh", "10\n");
  unchanged("sys/kernel/tracing/tracing_on", "0\n");
  const auto standing =
      lineOf(report.text, "tracer: hwlat already stands as the current tracer");
  checkWith(
      standing == "tracer: hwlat already stands as the current tracer",
      "the block names the tracer that already stands (T089, FR-023, P-15)",
      standing);
  const auto unavailable = report.text.find("tracer: not available");
  checkWith(report.text.find("tracer: hwlat already stands") < unavailable
                && unavailable != std::string::npos,
            "the standing line reads ahead of the not-available line (T089, "
            "FR-023, P-15)",
            "standing=" + standing + " unavailable="
                + (unavailable == std::string::npos ? "absent" : "present"));
  theUserSpaceStandsWhole(json);
  std::remove(json.c_str());
}

// ---- T079: the lines the coverage contract reads (FR-038, SC-002)

// The mode file of the standing tree carries the chosen word inside
// brackets, the list the kernel prints. A host that prints the words without
// brackets leaves one word on the line, and that word stands the value the
// setup saves whole (FR-019, P-16). The two shapes stand in one row: each ends
// with the file back at the same text, and only the read of the pair, or of the
// whole line, answers it.
auto aModeTextWithNoBracketsSavesItsWholeWord() -> void
{
  const auto cpus = pickCpus(1);
  if (cpus.empty()) {
    return;
  }
  constexpr const char* kModePath = "sys/kernel/tracing/hwlat_detector/mode";
  // The kernel's own list, the chosen word inside the brackets. The text the
  // read would answer for the other shape stands a whole line longer.
  {
    TracingRootGuard guard;
    installDetectorTree(guard.root());
    const auto session =
        sg::detail::noiseTracerSetup(sg::TracerEngine::HWLAT, cpus);
    check(session.has_value(),
          "the setup of the bracketed mode text answers a session (T079, "
          "FR-019)");
    if (session.has_value()) {
      check(readStream(guard.root() / kModePath) == "per-cpu\n",
            "the setup writes the mode the run chose (T079, FR-019, D-9)");
      sg::detail::noiseTracerRestore(*session);
      check(readStream(guard.root() / kModePath) == "round-robin\n",
            "the bracketed text saves the word the brackets hold (T079, "
            "FR-019, P-16)");
    }
  }
  // One word on the line, blanks beside it and no bracket anywhere. The read
  // saves the word trimmed, so the restore writes one line and no blank.
  {
    TracingRootGuard guard;
    installDetectorTree(guard.root());
    writeTracingFile(guard.root(), kModePath, "  round-robin  \n");
    const auto session =
        sg::detail::noiseTracerSetup(sg::TracerEngine::HWLAT, cpus);
    check(session.has_value(),
          "the setup of the unbracketed mode text answers a session (T079, "
          "FR-019)");
    if (!session.has_value()) {
      return;
    }
    check(readStream(guard.root() / kModePath) == "per-cpu\n",
          "the setup writes the mode the run chose beside the unbracketed "
          "text (T079, FR-019, D-9)");
    sg::detail::noiseTracerRestore(*session);
    check(readStream(guard.root() / kModePath) == "round-robin\n",
          "the unbracketed text saves its whole word trimmed (T079, FR-019, "
          "P-16)");
  }
}

// The three shapes the buffer walk reads as no gap, and the one line it reads
// as one: a line whose marker names no slash, a line whose pair holds one side
// that reads no count, a line that carries no marker at all, and the standing
// detection line (FR-018, FR-027, A-6, P-10).
constexpr const char* kNoSlashLine =
    "#1 inner/outer(us): 999 ts:0000000000000000 count:1\n";
constexpr const char* kNoCountLine =
    "#2 inner/outer(us): wide/5 ts:0000000000000000 count:1\n";
constexpr const char* kNoMarkerLine = "#7 no marker of a gap here\n";

// T079, FR-018, FR-027, A-6, P-10: the walk of one buffer text skips the three
// shapes and folds the line that names a pair. The same three shapes alone
// answer no figure at all, so the three cannot clear the row between
// themselves.
auto aBufferLineThatNamesNoPairCountsAsNoGap() -> void
{
  const auto cpus = pickCpus(1);
  if (cpus.empty()) {
    return;
  }
  TracingRootGuard guard;
  installDetectorTree(guard.root());
  writeTracingFile(guard.root(),
                   bufferPath(cpus.at(0)),
                   std::string(kLegend) + kNoSlashLine + kNoCountLine
                       + kNoMarkerLine + detectionLine(9, 1234, 5678, 2));
  const auto session =
      sg::detail::noiseTracerSetup(sg::TracerEngine::HWLAT, cpus);
  check(session.has_value(),
        "the setup of the four-shape buffer answers a session (T079, FR-019)");
  if (!session.has_value()) {
    return;
  }
  const auto counted = sg::detail::noiseTracerFigures(*session);
  checkWith(counted.size() == 1 && counted.at(0).gapCount.has_value()
                && *counted.at(0).gapCount == 1
                && counted.at(0).maxGapNs.has_value()
                && *counted.at(0).maxGapNs == 5678000ULL
                && counted.at(0).nmiCount.has_value()
                && *counted.at(0).nmiCount == 2,
            "the line that names a pair stands the three figures, and the "
            "three shapes beside it stand skipped (T079, FR-018, FR-027, A-6, "
            "P-10)",
            "entries=" + std::to_string(counted.size()));
  writeTracingFile(
      guard.root(),
      bufferPath(cpus.at(0)),
      std::string(kLegend) + kNoSlashLine + kNoCountLine + kNoMarkerLine);
  const auto skipped = sg::detail::noiseTracerFigures(*session);
  checkWith(skipped.size() == 1 && !skipped.at(0).gapCount.has_value()
                && !skipped.at(0).maxGapNs.has_value()
                && !skipped.at(0).nmiCount.has_value(),
            "the three shapes alone count no gap, so the cpu answers the "
            "three figures absent (T079, FR-018, A-6, P-10)",
            "entries=" + std::to_string(skipped.size()));
  sg::detail::noiseTracerRestore(*session);
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
  theSetupWhoseDeniedWriteFailsRestoresWhatItWrote();
  aFailedRestoreWriteNamesItsFileAndItsValue();
  theFiguresFoldTheLinesOfEachCpuBuffer();
  theColumnRuleSkipsTheLegendLines();
  theNmiCountSumsItsFieldAcrossLines();
  aBufferWithNoLineAnswersTheThreeFiguresAbsent();
  theMissingTracefsRunStandsWhole();
  theDeniedKnobRunStandsWhole();
  theWholeDetectorRunNamesItsThreeSettings();
  theInWindowFailureStillRestoresTheTree();
  theInterruptedChildRestoresTheTree();
  aDeniedRestoreWriteKeepsTheStatusItsPathCarried();
  theStandingTracerWritesNoFile();
  theTwoSurfacesRecordTheirOwnFigures();
  theAnsweredSourceMarksItsKeptGap();
  theAbsentSourceMarksNoKeptGap();
  aModeTextWithNoBracketsSavesItsWholeWord();
  aBufferLineThatNamesNoPairCountsAsNoGap();
  std::printf(failures == 0 ? "ok\n" : "%d checks failed\n", failures);
  return failures == 0 ? 0 : 1;
}
