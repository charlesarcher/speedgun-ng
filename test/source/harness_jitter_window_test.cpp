// ============================================================================
// T007: the report surfaces of one run with the probe in play.
//
// The console block prints after the benchmark tables, one column per
// probe cpu, the sixteen stats fields in the E-03 order (FR-023, R-08);
// the disclosure lines print in the end-of-report region the noise
// warning owns (FR-036, D-8, D-12). The JSON scenarios pin the run-level
// `jitter` object in the E-07 key order after `speedgun` and before
// `benchmarks`, the per-entry nullable key after `noiseWarning` (E-08),
// the round trip (FR-029, SC-001), the raw gap list (FR-028, E-10), and
// the reader error codes on the new keys (FR-029, contracts/report-json.md).
// The JSON scenarios stand red against the writer and the reader that
// T013 and T014 land, so they run after the green ones and the first red
// assertion names the scope the wave still misses. Frameworkless
// check()/fail() convention.
// ============================================================================

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <sched.h>
#include <unistd.h>

#include "detail/internal.hpp"
#include "detail/jitter_probe.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_fake.hpp"
#include "speedgun-ng/counters_system.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS JITTER WINDOW TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// A trivial benchmark: the probe spans the batch, so the run needs one
// registered function and the shortest minimum time the option scanner
// admits.
auto bmQuiet(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

// The scripted provider: the leaves the runner resolves its plans
// through, plus the machine/tsc leaf the probe opens, declared in the
// ReadMode the probe's catalog check reads (R-02). The tsc series steps
// by one tick, so no read stands a gap above the risen threshold and
// every field of the fold lands at the empty kept set's zeros.
auto scriptedProvider() -> sg::counters::FakeProvider&
{
  static auto* provider = []
  {
    auto owned = std::make_unique<sg::counters::FakeProvider>();
    auto& ref = *owned;
    ref.addCounter("machine", "monotonic", "nanoseconds", "wall clock");
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
    check(registered.has_value(), "the scripted provider registers (R-02)");
    return &ref;
  }();
  return *provider;
}

// The jump the gate body scripts into the shared tsc series' tail: one
// read stands a kept gap above the risen threshold, and every read
// after it jumps again until the run ends, because the window close
// must read the same tail the kept records ride (R-02, FR-001).
inline constexpr std::uint64_t kSeriesJump = 1000000;

// The jump the gate body scripts, mutable so the comparison scenarios of
// US4 script two different kept-gap magnitudes; every other scenario
// keeps the constexpr default (R-02). The function-local static keeps
// the mutable state out of namespace scope.
auto seriesJump() -> std::uint64_t&
{
  static std::uint64_t value = kSeriesJump;
  return value;
}

// The gate benchmark: the body scripts the jump into the shared series
// inside its own timed window and leaves the jumped tail in place, so
// the window close reads the tail the kept records ride and stands
// above every kept stamp by construction (R-02, FR-039). A one-tick
// restore before the close resets the shared cursor the close edge
// reads. When no probe read lands between the restore and the close
// read, the close forks from the reset cursor and stands below every
// kept stamp. The scenarios that follow reset the series before their
// first run. The two waits stand the jump after the probe's
// measurement pass and count the alongside reads the jump's tail
// carries: the runner's own reads inside one attempt are the chunk
// samples the chunk plan bounds, so the margins below stand above
// them, and the probe's reads are certain.
auto bmGate(sg::State& state) -> void
{
  auto& provider = scriptedProvider();
  // The pass wait: the measurement pass's reads finish before the
  // jump, so the pass measures the one-tick series and the effective
  // threshold stands at the configured value (E-05, FR-007, R-05). The
  // margin stands above the pass and the calibration's read counts
  // together.
  const auto base = provider.readActions();
  std::uint64_t spins = 0;
  while (provider.readActions() < base + 20000) {
    // The wait yields, the way the probe test's waits do, so the pinned
    // probe thread progresses under concurrent load; the bound turns a
    // stalled series into a reported failure; the wait cannot hang
    // (FR-039).
    std::this_thread::yield();
    if (++spins > 1000000000ULL) {
      fail("the probe's measurement pass never finished (FR-039)");
    }
  }
  // The jump wait: the pass is over, so every read in the margin rides
  // the scripted tail and keeps its gap inside this body's window
  // (FR-039, R-04).
  provider.setPoints("machine", "tsc", {}, seriesJump());
  const auto jumped = provider.readActions();
  spins = 0;
  while (provider.readActions() < jumped + 20000) {
    std::this_thread::yield();
    if (++spins > 1000000000ULL) {
      fail("the scripted jump never reached the probe's reads (FR-039)");
    }
  }
  // The tail stays jumped past the window close: the close edge reads
  // the same tail, so the close stamp stands above every kept stamp
  // this window holds (FR-039, R-04).
  for (auto _ : state) {
  }
}

// The cpu indexes the scenarios pin probe threads to, taken from the
// process affinity mask, each below the online count (FR-040).
auto pickCpus(std::size_t count) -> std::vector<std::uint32_t>
{
  cpu_set_t mask {};
  CPU_ZERO(&mask);
  if (sched_getaffinity(0, sizeof(mask), &mask) != 0) {
    fail("the process affinity mask is unreadable (FR-040)");
  }
  const int online = CPU_COUNT(&mask);
  check(online > 0, "the affinity mask holds cpus (FR-040)");
  std::vector<std::uint32_t> cpus;
  for (std::size_t index = 0; index < CPU_SETSIZE && cpus.size() < count;
       ++index)
  {
    if (CPU_ISSET(index, &mask)) {
      check(index < static_cast<std::size_t>(online),
            "a picked cpu stands below the online count (FR-040)");
      cpus.push_back(static_cast<std::uint32_t>(index));
    }
  }
  check(cpus.size() == count,
        "the mask holds the cpus the scenario asks for (FR-040)");
  return cpus;
}

// One speedgunMain run with stdout captured, the way the gap test
// captures: the report lands in a scratch file beside the redirected
// descriptor, and the status must follow contracts/cli.md (FR-036).
auto captureRun(const std::vector<std::string>& arguments,
                const int expected = 0) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_jitter_window_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  static int runIndex = 0;
  const std::string path =
      "harness_jitter_window_out_" + std::to_string(++runIndex) + ".txt";
  const int saved = dup(STDOUT_FILENO);
  if (saved < 0 || std::freopen(path.c_str(), "w", stdout) == nullptr) {
    fail("cannot redirect the report");
  }
  const int status =
      sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
  std::fflush(stdout);
  if (dup2(saved, STDOUT_FILENO) < 0) {
    fail("cannot restore stdout");
  }
  close(saved);
  if (status != expected) {
    std::fprintf(
        stderr, "speedgunMain returned %d, expected %d\n", status, expected);
    fail("the exit status does not follow contracts/cli.md (FR-036)");
  }

  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  std::remove(path.c_str());
  return buffer.str();
}

auto readDocument(const std::string& path) -> std::string
{
  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// One speedgunMain run with stderr captured: the comparison run's usage
// errors print their one line there (FR-032, FR-038), the way the option
// rejections of the cli test do.
auto captureError(const std::vector<std::string>& arguments,
                  const int expected) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_jitter_window_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  static int runIndex = 0;
  const std::string path =
      "harness_jitter_window_err_" + std::to_string(++runIndex) + ".txt";
  const int saved = dup(STDERR_FILENO);
  if (saved < 0 || std::freopen(path.c_str(), "w", stderr) == nullptr) {
    fail("cannot redirect the failure note");
  }
  const int status =
      sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
  std::fflush(stderr);
  if (dup2(saved, STDERR_FILENO) < 0) {
    fail("cannot restore stderr");
  }
  close(saved);
  if (status != expected) {
    std::fprintf(
        stderr, "speedgunMain returned %d, expected %d\n", status, expected);
    fail("the exit status does not follow contracts/cli.md (FR-036)");
  }

  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  std::remove(path.c_str());
  return buffer.str();
}

// The document with the digits of one integral key's value replaced, the
// first occurrence of the key at or after `from`: the comparison
// scenarios script one report's value the way the reader scenarios
// script a broken document, and the stats integers stand as bare digits
// (E-03).
auto withIntegerReplaced(const std::string& document,
                         std::size_t from,
                         const char* key,
                         std::uint64_t value) -> std::string
{
  const auto at = document.find(std::string("\"") + key + "\":", from);
  check(at != std::string::npos, "the document carries the key");
  const auto digitsEnd =
      document.find_first_not_of("0123456789", at + std::strlen(key) + 3);
  check(digitsEnd != std::string::npos, "the key carries a digit run");
  std::string replaced = document.substr(0, at + std::strlen(key) + 3);
  replaced += std::to_string(value);
  replaced += document.substr(digitsEnd);
  return replaced;
}

// The sixteen stats fields in the E-03 order, the canonical enumeration
// the console block's lines and the JSON writer's keys follow.
constexpr std::array<std::string_view, 16> kStatFields {"gapCount",
                                                        "gapsPerSecond",
                                                        "minNs",
                                                        "medianNs",
                                                        "meanNs",
                                                        "p90Ns",
                                                        "p99Ns",
                                                        "p999Ns",
                                                        "p9999Ns",
                                                        "maxNs",
                                                        "totalNs",
                                                        "totalPercent",
                                                        "runtimeNs",
                                                        "thresholdNs",
                                                        "loopPeriodNs",
                                                        "droppedGaps"};

auto splitLines(const std::string& text) -> std::vector<std::string>
{
  std::istringstream stream(text);
  std::vector<std::string> lines;
  std::string line;
  while (std::getline(stream, line)) {
    lines.push_back(line);
  }
  return lines;
}

// The unsigned value one integral JSON key carries, read as the digit
// run that follows the key's colon from `from` on: the writer writes
// the stats integers as bare digits (E-03).
auto integerValueAfter(const std::string& document,
                       std::size_t from,
                       const char* key) -> std::uint64_t
{
  const auto at = document.find(std::string("\"") + key + "\":", from);
  check(at != std::string::npos, "the document carries the key");
  auto cursor = at + std::strlen(key) + 3;
  std::uint64_t value = 0;
  while (std::isdigit(static_cast<unsigned char>(document.at(cursor))) != 0) {
    value = value * 10 + static_cast<std::uint64_t>(document.at(cursor) - '0');
    ++cursor;
  }
  return value;
}

// The per-thread stamp state of the seam reader below: each probe
// thread carries its own series, so the two cpus' reads interleave
// without sharing a cursor (R-02, FR-039). The probe threads are new
// threads of the run, so the state starts fresh for every scenario.
struct GapThreadState
{
  std::uint64_t calls = 0;
  std::uint64_t stamp = 0;
};

thread_local GapThreadState gapThreadState;

// The per-thread stamp series the raw-gap scenario runs through the
// reader-install seam (R-02): the fake's one shared cursor cannot
// express a per-thread jump, because the run drives both probe threads'
// reads through it interleaved and the test cannot step between the
// phases. Each thread's series steps 1 through the measurement pass's
// 1,024 reads and jumps once at its 1,025th read, the first read of the
// pre-check phase, whose loop runs to its scripted span before any stop
// can matter, so each cpu keeps exactly one record under any load
// (FR-028, E-10, FR-014).
class GapSeriesReader final : public sg::counters::WindowReader
{
public:
  auto readPoints(sg::counters::PointSink& sink) noexcept -> void override
  {
    auto& thread = gapThreadState;
    ++thread.calls;
    thread.stamp += thread.calls == 1025 ? 1000000 : 1;
    sink.put(thread.stamp);
  }
};

}  // namespace

SG_BENCHMARK(bmQuiet);
SG_BENCHMARK(bmGate);

auto main() -> int
{
  scriptedProvider();
  const auto cpus = pickCpus(2);
  const std::string cpuList =
      std::to_string(cpus.at(0)) + "," + std::to_string(cpus.at(1));
  const std::vector<std::string> quickRun {
      "--filter=bmQuiet", "--repetitions=1", "--iterations=2"};

  // FR-013, SC-003: a run with no probe option prints the console
  // report with the fields and the shape master prints, and the JSON
  // writer of the same run carries no jitter key.
  {
    const auto report =
        captureRun({"--filter=bmQuiet", "--repetitions=1", "--iterations=2"});
    check(report.find("jitter probe:") == std::string::npos
              && report.find("jitter: ") == std::string::npos,
          "a probe-free run prints no probe block and no disclosure "
          "(FR-013, SC-003)");
    check(report.find("bmQuiet") != std::string::npos,
          "the probe-free run prints the benchmark (SC-003)");
  }

  // FR-026, SC-003, D-1: the probe-free document's key sequence equals
  // what master prints: the schema version, the run facts, the entries,
  // and no jitter key anywhere.
  {
    const std::string path = "harness_jitter_window_free.json";
    captureRun({"--filter=bmQuiet",
                "--repetitions=1",
                "--iterations=2",
                "--json=" + path});
    const auto document = readDocument(path);
    std::remove(path.c_str());
    const auto schema = document.find("\"schemaVersion\"");
    const auto speedgun = document.find("\"speedgun\"");
    const auto benchmarks = document.find("\"benchmarks\"");
    check(schema != std::string::npos && speedgun != std::string::npos
              && benchmarks != std::string::npos
              && schema < speedgun && speedgun < benchmarks,
          "the probe-free document keeps the master key sequence "
          "(FR-026, D-1)");
    check(document.find("\"jitter\"") == std::string::npos,
          "the probe-free document carries no jitter key (FR-013, E-07)");
  }

  // FR-023, R-08: the console report of a run with the probe prints the
  // probe block after the benchmark tables, one column per probe cpu,
  // the sixteen stats fields in the E-03 order; the disclosure lines
  // follow in the end-of-report region (FR-036, D-8).
  {
    auto arguments = quickRun;
    arguments.push_back("--jitter-probe=" + cpuList);
    const auto probed = captureRun(arguments);
    const auto lines = splitLines(probed);

    std::size_t blockLine = 0;
    std::size_t lastBenchmarkLine = 0;
    for (std::size_t index = 0; index < lines.size(); ++index) {
      if (lines.at(index).rfind("bmQuiet", 0) == 0) {
        lastBenchmarkLine = index;
      }
      if (lines.at(index).rfind("jitter probe: cpus ", 0) == 0
          && blockLine == 0)
      {
        blockLine = index;
      }
    }
    check(blockLine > 0, "the run prints the probe block (FR-023, R-08)");
    check(blockLine > lastBenchmarkLine,
          "the probe block stands after the benchmark tables (FR-023, R-08)");
    check(lines.at(blockLine) == "jitter probe: cpus " + cpuList,
          "the probe block names one column per probe cpu (FR-023, R-08)");
    for (std::size_t field = 0; field < kStatFields.size(); ++field) {
      const auto& line = lines.at(blockLine + 1 + field);
      check(line.rfind(kStatFields.at(field), 0) == 0,
            "the block lists the stats fields in the E-03 order (R-08)");
      check(line.find_first_not_of(" \t", kStatFields.at(field).size())
                != std::string::npos,
            "each stats line carries one value per probe cpu (FR-023)");
    }
    check(lines.at(blockLine + 14).find("200") != std::string::npos,
          "the threshold field carries the risen default (FR-007, R-08)");
    check(lines.at(blockLine + 15).find(" 1 ") != std::string::npos,
          "the loop-period field carries the pass's minimum (E-05, R-05)");

    // FR-036, D-8, D-12: the disclosure lines stand in the
    // end-of-report region: the availability and the cpus, the measured
    // points, the per-cpu counts, and the kept-gaps-only rule.
    check(probed.find("jitter: probe on cpus " + cpuList) != std::string::npos,
          "the disclosure names the probe and the cpus (FR-036, E-09)");
    check(probed.find("jitter: loop period 1 ns, threshold 200 ns")
              != std::string::npos,
          "the disclosure carries the measured points (FR-007, E-09)");
    check(probed.find("jitter: split and percentiles use the kept gaps only")
              != std::string::npos,
          "the disclosure carries the kept-gaps-only rule (E-09)");
  }

  // FR-026, E-07, R-09: the JSON writer carries the run-level jitter
  // object in the E-07 key order after `speedgun` and before
  // `benchmarks`, and each benchmark entry the nullable per-entry key
  // after `noiseWarning` (E-08), standing null before the split ships.
  // Red against the writer T013 lands: the scope the wave still misses.
  {
    const std::string path = "harness_jitter_window_run.json";
    auto arguments = quickRun;
    arguments.push_back("--json=" + path);
    arguments.push_back("--jitter-probe=" + cpuList);
    captureRun(arguments);
    const auto document = readDocument(path);
    std::remove(path.c_str());
    const auto speedgun = document.find("\"speedgun\"");
    const auto jitter = document.find("\"jitter\"");
    const auto benchmarks = document.find("\"benchmarks\"");
    check(jitter != std::string::npos && jitter > speedgun
              && jitter < benchmarks,
          "the run-level jitter object stands after speedgun and before "
          "benchmarks (FR-026, E-07)");
    const std::string_view keys[] = {"mode",
                                     "thresholdNs",
                                     "loopPeriodNs",
                                     "cpus",
                                     "stats",
                                     "droppedGaps",
                                     "disclosure"};
    auto cursor = jitter;
    for (const auto key : keys) {
      cursor = document.find("\"" + std::string(key) + "\"", cursor);
      check(cursor != std::string::npos,
            "the jitter object carries its keys in the E-07 order (E-07)");
    }
    const auto noiseWarning = document.find("\"noiseWarning\"");
    check(noiseWarning != std::string::npos,
          "the entry carries its noise warning (FR-027)");
    const auto entryJitter = document.find("\"jitter\"", noiseWarning);
    check(entryJitter != std::string::npos,
          "the per-entry key stands after noiseWarning (FR-027, E-08)");
  }

  // FR-029, SC-001: the reader reads the written file back with every
  // jitter field equal. Red against the reader T014 lands: the fields
  // the wave still misses.
  {
    const std::string path = "harness_jitter_window_roundtrip.json";
    auto arguments = quickRun;
    arguments.push_back("--json=" + path);
    arguments.push_back("--jitter-probe=" + cpuList);
    captureRun(arguments);
    const auto loaded = sg::loadReport(path);
    std::remove(path.c_str());
    check(loaded.has_value(),
          "the document of a probed run loads (FR-029, SC-001)");
  }

  // FR-028, E-10: the gaps value of the raw JSON option writes the
  // per-cpu gap list, each record carrying stamp and gapNs. The gap
  // series rides the reader-install seam, the route the fake's shared
  // cursor cannot express (R-02); red against the writer T014 lands.
  {
    const std::string path = "harness_jitter_window_gaps.json";
    auto arguments = quickRun;
    arguments.push_back("--json=" + path);
    arguments.push_back("--json-raw=gaps");
    // The pre-check mode makes the kept record load-independent: the
    // pre-check loop runs to its scripted span with no stop check, and
    // `precheck=1` with the series' one-tick step reaches that span at
    // the jump read, so the phase is one read that every cpu keeps
    // (FR-014, E-05).
    arguments.push_back("--jitter-probe=" + cpuList
                        + ",mode=precheck,precheck=1");
    sg::detail::installTestReader(std::make_unique<GapSeriesReader>());
    captureRun(arguments);
    sg::detail::installTestReader(nullptr);
    const auto document = readDocument(path);
    std::remove(path.c_str());
    check(document.find("\"gaps\"") != std::string::npos
              && document.find("\"stamp\"") != std::string::npos
              && document.find("\"gapNs\"") != std::string::npos,
          "the raw gap list carries its records (FR-028, E-10)");
  }

  // FR-029, contracts/report-json.md: one test per reader error code on
  // the new keys. The documents carry a jitter scope the reader of
  // master ignores, so both stand red against the reader T014 lands.
  {
    const std::string path = "harness_jitter_window_free_reader.json";
    captureRun({"--filter=bmQuiet",
                "--repetitions=1",
                "--iterations=2",
                "--json=" + path});
    auto document = readDocument(path);
    std::remove(path.c_str());
    const auto benchmarks = document.find(",\"benchmarks\"");
    check(benchmarks != std::string::npos,
          "the document carries its entries key (FR-026)");

    // NO_SUCH_FIELD: a jitter scope whose `mode` key stands missing.
    {
      auto broken = document;
      broken.insert(benchmarks, ",\"jitter\":{\"thresholdNs\":200}");
      const std::string brokenPath = "harness_jitter_window_nofield.json";
      std::ofstream out(brokenPath);
      out << broken;
      out.close();
      const auto loaded = sg::loadReport(brokenPath);
      std::remove(brokenPath.c_str());
      check(!loaded.has_value()
                && loaded.error().find("jitter") != std::string::npos
                && loaded.error().find("stands missing")
                       != std::string::npos,
            "a jitter scope with a missing key names the field path "
            "(FR-029, contracts/report-json.md)");
    }

    // INCORRECT_TYPE: a jitter scope whose `mode` key carries a number.
    {
      auto broken = document;
      broken.insert(benchmarks,
                    ",\"jitter\":{\"mode\":42,\"thresholdNs\":200}");
      const std::string brokenPath = "harness_jitter_window_wrongtype.json";
      std::ofstream out(brokenPath);
      out << broken;
      out.close();
      const auto loaded = sg::loadReport(brokenPath);
      std::remove(brokenPath.c_str());
      check(!loaded.has_value()
                && loaded.error().find("jitter") != std::string::npos
                && loaded.error().find("expected ") != std::string::npos,
            "a jitter scope with a wrong kind names the kind it expected "
            "(FR-029, contracts/report-json.md)");
    }
  }

  // US3 scenario 1, FR-021, FR-024, FR-027, E-08, R-04: the split
  // places the kept gaps of one probe cpu into the entry whose windows
  // contain their stamps. The gate benchmark scripts a jump into the
  // shared tsc series inside its own timed window, so the probe cpu's
  // next read stands a kept gap above the risen threshold, and the
  // window close reads the same jumped tail the kept records ride, so
  // the close stamp stands above every kept record's stamp (R-02,
  // R-04). Observe red before T023 to T025 exist: the
  // per-entry key stands null and no attribution line prints.
  {
    auto& provider = scriptedProvider();
    provider.setPoints("machine", "tsc", {}, 1);
    const std::string path = "harness_jitter_window_split.json";
    const auto console =
        captureRun({"--repetitions=1",
                    "--iterations=2",
                    "--json=" + path,
                    "--jitter-probe=" + std::to_string(cpus.at(0))});
    const auto document = readDocument(path);
    std::remove(path.c_str());

    const auto gateName = document.find("\"bmGate\"");
    const auto quietName = document.find("\"bmQuiet\"");
    check(gateName != std::string::npos && quietName != std::string::npos,
          "the document carries both entries (FR-021)");
    check(document.find("\"jitter\":{\"stats\"", gateName) != std::string::npos,
          "the gate entry's jitter object carries the stats (FR-021, E-08)");
    check(document.find("\"jitter\":null", quietName) != std::string::npos,
          "the entry whose windows held no kept gap stands null "
          "(FR-027, E-08)");
    check(document.find("\"cpu\":" + std::to_string(cpus.at(0)), gateName)
              != std::string::npos,
          "the stats name the probe cpu (E-08)");
    check(integerValueAfter(document, gateName, "maxNs") == kSeriesJump,
          "the stats carry the scripted gap (E-08, R-04)");

    const auto lines = splitLines(console);
    std::size_t gateLast = 0;
    std::size_t blockLine = 0;
    std::size_t lineAt = 0;
    for (std::size_t index = 0; index < lines.size(); ++index) {
      if (lines.at(index).rfind("bmGate", 0) == 0) {
        gateLast = index;
      }
      if (lines.at(index).rfind("jitter: worst cpu ", 0) == 0 && lineAt == 0) {
        lineAt = index;
      }
      if (lines.at(index).rfind("jitter probe: cpus ", 0) == 0
          && blockLine == 0)
      {
        blockLine = index;
      }
    }
    check(lineAt != 0 && gateLast != 0 && blockLine != 0,
          "the run prints the attribution line (FR-024, R-08)");
    check(lineAt > gateLast && lineAt < blockLine,
          "the attribution line prints under that benchmark's rows "
          "(FR-024, R-08)");
    check(lines.at(lineAt).find("gaps=") != std::string::npos
              && lines.at(lineAt).find("max=") != std::string::npos
              && lines.at(lineAt).find("total=") != std::string::npos,
          "the line carries the grammar of the one code span (FR-024)");
    check(lines.at(lineAt).find("worst cpu " + std::to_string(cpus.at(0)))
              != std::string::npos,
          "the line names the worst probe cpu (FR-024, R-08)");
  }

  // US3 scenario 2, FR-022, PC-5: each pass the runner carries gives its
  // own timed windows and the split covers all of them. The gate's three
  // repetitions each keep their jump inside their own window, and the
  // entry's fold counts every repetition's kept gaps; a split over one
  // window alone would carry one repetition's records and stand red
  // against the count below.
  {
    auto& provider = scriptedProvider();
    provider.setPoints("machine", "tsc", {}, 1);
    const std::string path = "harness_jitter_window_passes.json";
    captureRun({"--filter=bmGate",
                "--repetitions=3",
                "--iterations=2",
                "--json=" + path,
                "--jitter-probe=" + std::to_string(cpus.at(0))});
    const auto document = readDocument(path);
    std::remove(path.c_str());
    const auto gateName = document.find("\"bmGate\"");
    check(gateName != std::string::npos,
          "the document carries the gate entry (FR-022)");
    check(document.find("\"jitter\":{\"stats\"", gateName) != std::string::npos,
          "the fold covers the windows of every repetition (FR-022, PC-5)");
    check(integerValueAfter(document, gateName, "gapCount") >= 3,
          "each repetition's windows placed their kept gaps (FR-022, PC-5)");
    check(integerValueAfter(document, gateName, "maxNs") == kSeriesJump,
          "the kept gaps carry the scripted jump (E-04, R-04)");
  }

  // FR-010, D-6: a buffer that filled attributes only the kept records.
  // The one-slot buffer keeps the jump's first record and drops the
  // reads after it, the entry's stats carry the kept record alone, the
  // run-level record counts the dropped gaps, and the report states the
  // rule.
  {
    auto& provider = scriptedProvider();
    provider.setPoints("machine", "tsc", {}, 1);
    const std::string path = "harness_jitter_window_kept.json";
    const auto console = captureRun(
        {"--filter=bmGate",
         "--repetitions=1",
         "--iterations=2",
         "--json=" + path,
         "--jitter-probe=" + std::to_string(cpus.at(0)) + ",gaps=1"});
    const auto document = readDocument(path);
    std::remove(path.c_str());
    const auto gateName = document.find("\"bmGate\"");
    check(gateName != std::string::npos,
          "the document carries the gate entry (FR-010)");
    check(document.find("\"jitter\":{\"stats\"", gateName) != std::string::npos,
          "the entry carries the kept record's stats (FR-010, D-6)");
    check(integerValueAfter(document, gateName, "gapCount") == 1,
          "the stats attribute the kept record alone (FR-010, D-6)");
    const auto benchmarks = document.find(",\"benchmarks\"");
    check(benchmarks != std::string::npos,
          "the document carries its entries key (FR-026)");
    check(integerValueAfter(document, 0, "droppedGaps") >= 1,
          "the run-level record counts the dropped gaps (FR-010, E-07)");
    check(console.find("jitter: split and percentiles use the kept gaps only")
              != std::string::npos,
          "the report states the kept-gaps-only rule (FR-010, D-6)");
  }

  // E-06, R-04, FR-021: the fold of one probe cpu over one benchmark's
  // windows places each kept record by its stamp: a record inside a
  // window folds once into the entry's stats even when two of its
  // windows contain it, a record outside every window stands absent
  // from the stats, and the covered span the percent and rate fields
  // divide is the windows' total. The fold rides the seam header, so
  // the red this scenario predicts against T024 is the undefined
  // reference the split's definition answers (Principle III).
  {
    sg::detail::ProbeCpuState state;
    state.cpu = cpus.at(0);
    state.buffer.resize(4);
    state.buffer.at(0) = {.stamp = 10, .gapNs = 5};
    state.buffer.at(1) = {.stamp = 12, .gapNs = 9};
    state.buffer.at(2) = {.stamp = 30, .gapNs = 7};
    state.buffer.at(3) = {.stamp = 100, .gapNs = 11};
    state.kept = 4;
    state.thresholdNs = 200;
    state.loopPeriodNs = 1;
    const std::vector<sg::detail::TimedWindow> windows {
        {.start = 0,
         .end = 20,
         .entry = 0,
         .repetition = 0,
         .chunk = 0,
         .pass = 0},
        {.start = 10,
         .end = 25,
         .entry = 0,
         .repetition = 0,
         .chunk = 0,
         .pass = 0},
        {.start = 25,
         .end = 40,
         .entry = 0,
         .repetition = 1,
         .chunk = 0,
         .pass = 0},
    };
    const auto fold = sg::detail::gapStatsOf(state, windows);
    check(fold.gapCount == 3,
          "the fold counts each placed record once (E-06, FR-021)");
    check(fold.minNs == 5 && fold.maxNs == 9 && fold.totalNs == 21,
          "the fold carries the placed records' fields (E-03, E-06)");
    check(fold.thresholdNs == 200 && fold.loopPeriodNs == 1,
          "the fold carries the run's measured points (E-03)");
    check(fold.runtimeNs == 50,
          "the covered span is the windows' total (E-03, R-05)");
    check(std::fabs(fold.totalPercent - 42.0) <= 1e-9,
          "the percent divides by the windows' total (E-03, R-05)");

    const std::vector<sg::detail::TimedWindow> elsewhere {
        {.start = 1000,
         .end = 2000,
         .entry = 0,
         .repetition = 0,
         .chunk = 0,
         .pass = 0},
    };
    const auto absent = sg::detail::gapStatsOf(state, elsewhere);
    check(absent.gapCount == 0 && absent.totalNs == 0
              && absent.maxNs == 0,
          "a record outside every window stands absent from the stats "
          "(E-06, FR-021)");
  }

  // US4 scenario 1, FR-030, FR-031, D-11, contracts/rule-names.md: the
  // parser accepts the four jitter value names with every one of the
  // sixteen field names of FR-002 in camelCase and the standing
  // comparison suffix the N5 grammar carries. Observe red before T027
  // exists: the parser carries no `jitter:` names.
  {
    const std::string cpu = std::to_string(cpus.at(0));
    const std::string heads[] {"jitter:" + cpu + ":",
                               "jitter:any:",
                               "jitter:run:" + cpu + ":",
                               "jitter:run:any:"};
    for (const auto& head : heads) {
      for (const auto field : kStatFields) {
        const std::string text = head + std::string(field) + ":lower:abs=5000";
        const auto rule = sg::parseCompareRule(text);
        check(rule.has_value(),
              "the parser accepts the jitter value name (FR-030, D-11)");
        check(
            rule->text == text && rule->valueName == head + std::string(field),
            "the rule keeps the text and the value name it read (D-08)");
        check(rule->absoluteThreshold.has_value()
                  && std::fabs(*rule->absoluteThreshold - 5000.0) <= 1e-9,
              "the standing suffix's absolute bound lands in the rule "
              "(FR-033)");
      }
    }
    const auto percent = sg::parseCompareRule("jitter:any:p99Ns:higher:5%");
    check(percent.has_value() && percent->relativeThreshold.has_value()
              && percent->direction == sg::Direction::HIGHER_IS_BETTER,
          "the percent suffix and the direction word read on a jitter "
          "name (FR-033)");
    const auto filtered =
        sg::parseCompareRule("jitter:run:any:p99Ns:lower:abs=1:bm");
    check(filtered.has_value() && filtered->filter == "bm",
          "the filter field reads on a jitter name (FR-024)");
    // A field name outside the sixteen, a cpu that is neither an index
    // nor `any`, a cpu past the index range, and an aggregate suffix on
    // a jitter name are faults: the value carries one value (FR-038,
    // spec D-21).
    check(!sg::parseCompareRule("jitter:any").has_value(),
          "a jitter name with no field is a fault (FR-030)");
    check(!sg::parseCompareRule("jitter:any:madeUpNs:lower:abs=1").has_value(),
          "a field name outside the sixteen is a fault (FR-038)");
    check(!sg::parseCompareRule("jitter:x:p99Ns:lower:abs=1").has_value(),
          "a cpu that is neither an index nor any is a fault (FR-038)");
    check(!sg::parseCompareRule("jitter:4294967296:p99Ns:lower:abs=1")
               .has_value(),
          "a cpu past the index range is a fault (FR-038)");
    check(
        !sg::parseCompareRule("jitter:any:p99Ns@mean:lower:abs=1").has_value(),
        "an aggregate suffix on a jitter name is a fault (spec D-21)");
  }

  // US4 scenario 2, FR-038: the field fault is the usage error of the
  // option scan: one line and exit status 2, before any report is read.
  {
    const auto note =
        captureError({"--compare=a.json,b.json",
                      "--compare-rule=jitter:any:madeUpNs:lower:abs=5000"},
                     2);
    check(
        note.rfind("--compare-rule: ", 0) == 0
            && std::count(note.begin(), note.end(), '\n') == 1,
        "the field fault prints one line and exits with status 2 " "(FR-038)");
  }

  // US4 scenarios 3 to 7, FR-030 to FR-032, SC-004: the comparison runs
  // over documents the probe writes. The gate benchmark scripts its
  // jump through `seriesJump`, so the two runs' kept gaps differ by more
  // than the example rule's bound of D-11.
  {
    const std::string cpu = std::to_string(cpus.at(0));
    const std::string otherCpu = std::to_string(cpus.at(1));
    const std::string probe = "--jitter-probe=" + cpu;

    auto& provider = scriptedProvider();
    provider.setPoints("machine", "tsc", {}, 1);
    seriesJump() = kSeriesJump;
    const std::string basePath = "harness_jitter_rule_base.json";
    captureRun({"--filter=bmGate",
                "--repetitions=1",
                "--iterations=2",
                "--json=" + basePath,
                probe});
    provider.setPoints("machine", "tsc", {}, 1);
    seriesJump() = 2 * kSeriesJump;
    const std::string bigPath = "harness_jitter_rule_big.json";
    captureRun({"--filter=bmGate",
                "--repetitions=1",
                "--iterations=2",
                "--json=" + bigPath,
                probe});
    seriesJump() = kSeriesJump;

    const auto baseDocument = readDocument(basePath);
    const auto gateName = baseDocument.find("\"bmGate\"");
    check(gateName != std::string::npos,
          "the document carries the gate entry (FR-030)");
    check(integerValueAfter(baseDocument, gateName, "p99Ns") == kSeriesJump,
          "the entry stats carry the scripted jump as their p99 (E-03)");
    check(integerValueAfter(baseDocument, 0, "p99Ns") == kSeriesJump,
          "the run-level stats carry the scripted jump as their p99 " "(E-07)");

    // US4 scenario 3, FR-030, SC-004: the bare rules read the
    // per-benchmark values, and a bound the contender's p99 moves past
    // fails the comparison with status 1.
    const auto regressed =
        captureRun({"--compare=" + basePath + "," + bigPath,
                    "--compare-rule=jitter:" + cpu + ":p99Ns:lower:abs=5000"},
                   1);
    check(regressed.find("REGRESSED") != std::string::npos,
          "the named-cpu bare rule reads the per-benchmark values and the "
          "moved bound fails with status 1 (SC-004, FR-030)");
    const auto regressedAny =
        captureRun({"--compare=" + basePath + "," + bigPath,
                    "--compare-rule=jitter:any:p99Ns:lower:abs=5000"},
                   1);
    check(regressedAny.find("REGRESSED") != std::string::npos,
          "the worst-cpu bare rule reads the per-benchmark values and the "
          "moved bound fails with status 1 (SC-004, D-11)");
    // The same rule over two equal documents reads a delta of zero and
    // passes: the state follows the values the documents carry.
    const auto passed =
        captureRun({"--compare=" + basePath + "," + basePath,
                    "--compare-rule=jitter:any:p99Ns:lower:abs=5000"},
                   0);
    check(passed.find("REGRESSED") == std::string::npos,
          "a bound the values clear passes the comparison (SC-004)");

    // US4 scenario 4, FR-031, SC-004: the `jitter:run:` prefix reads the
    // whole-run values, and the same moved bound fails with status 1.
    const auto regressedRun = captureRun(
        {"--compare=" + basePath + "," + bigPath,
         "--compare-rule=jitter:run:" + cpu + ":p99Ns:lower:abs=5000"},
        1);
    check(regressedRun.find("REGRESSED") != std::string::npos,
          "the run-scoped rule reads the whole-run values and the moved "
          "bound fails with status 1 (SC-004, FR-031)");
    const auto regressedRunAny =
        captureRun({"--compare=" + basePath + "," + bigPath,
                    "--compare-rule=jitter:run:any:p99Ns:lower:abs=5000"},
                   1);
    check(regressedRunAny.find("REGRESSED") != std::string::npos,
          "the run-scoped worst-cpu rule reads the whole-run values and "
          "the moved bound fails with status 1 (SC-004, FR-031)");

    // US4 scenario 5, FR-031, D-11: the bare rule reads the entry's own
    // stats and the `jitter:run:` prefix reads the run-level object:
    // editing one scope of the contender moves only the rule that reads
    // it.
    const std::string entryEditedPath = "harness_jitter_rule_entry.json";
    {
      const std::string edited =
          withIntegerReplaced(baseDocument, gateName, "p99Ns", 2 * kSeriesJump);
      std::ofstream out(entryEditedPath);
      out << edited;
      out.close();
    }
    const auto entryMoved =
        captureRun({"--compare=" + basePath + "," + entryEditedPath,
                    "--compare-rule=jitter:any:p99Ns:lower:abs=5000"},
                   1);
    check(entryMoved.find("REGRESSED") != std::string::npos,
          "the bare rule reads the entry's own stats (FR-031, D-11)");
    const auto entryUnmoved =
        captureRun({"--compare=" + basePath + "," + entryEditedPath,
                    "--compare-rule=jitter:run:any:p99Ns:lower:abs=5000"},
                   0);
    check(entryUnmoved.find("REGRESSED") == std::string::npos,
          "the run-scoped rule reads the run-level object, which the "
          "entry edit leaves equal (FR-031, D-11)");
    const std::string rootEditedPath = "harness_jitter_rule_root.json";
    {
      const std::string edited =
          withIntegerReplaced(baseDocument, 0, "p99Ns", 2 * kSeriesJump);
      std::ofstream out(rootEditedPath);
      out << edited;
      out.close();
    }
    const auto rootMoved =
        captureRun({"--compare=" + basePath + "," + rootEditedPath,
                    "--compare-rule=jitter:run:any:p99Ns:lower:abs=5000"},
                   1);
    check(rootMoved.find("REGRESSED") != std::string::npos,
          "the run-scoped rule reads the run-level object (FR-031, D-11)");
    const auto rootUnmoved =
        captureRun({"--compare=" + basePath + "," + rootEditedPath,
                    "--compare-rule=jitter:any:p99Ns:lower:abs=5000"},
                   0);
    check(rootUnmoved.find("REGRESSED") == std::string::npos,
          "the bare rule reads the entry's own stats, which the "
          "run-level edit leaves equal (FR-031, D-11)");
    std::remove(entryEditedPath.c_str());
    std::remove(rootEditedPath.c_str());

    // US4 scenario 6, FR-032, FR-038: a named cpu the report's jitter
    // object does not carry is the same usage error: one line and exit
    // status 2, in both scopes. The probe ran on one cpu, so the mask's
    // other cpu stands uncarried.
    {
      const auto note = captureError(
          {"--compare=" + basePath + "," + basePath,
           "--compare-rule=jitter:" + otherCpu + ":p99Ns:lower:abs=5000"},
          2);
      check(note.rfind("error: ", 0) == 0
                && std::count(note.begin(), note.end(), '\n') == 1,
            "the uncarried cpu of a bare rule is the usage error, one "
            "line and status 2 (FR-032, FR-038)");
      const auto runNote = captureError(
          {"--compare=" + basePath + "," + basePath,
           "--compare-rule=jitter:run:" + otherCpu + ":p99Ns:lower:abs=5000"},
          2);
      check(runNote.rfind("error: ", 0) == 0
                && std::count(runNote.begin(), runNote.end(), '\n') == 1,
            "the uncarried cpu of a run-scoped rule is the usage error, "
            "one line and status 2 (FR-032, FR-038)");
    }

    // US4 scenario 7, FR-032, SC-004: a `jitter:` rule on a report with
    // no probe prints one line and exits with status 2, in both scopes.
    {
      const std::string freeA = "harness_jitter_rule_free_a.json";
      captureRun({"--filter=bmQuiet",
                  "--repetitions=1",
                  "--iterations=2",
                  "--json=" + freeA});
      const std::string freeB = "harness_jitter_rule_free_b.json";
      captureRun({"--filter=bmQuiet",
                  "--repetitions=1",
                  "--iterations=2",
                  "--json=" + freeB});
      const auto note =
          captureError({"--compare=" + freeA + "," + freeB,
                        "--compare-rule=jitter:any:p99Ns:lower:abs=5000"},
                       2);
      check(note.rfind("error: ", 0) == 0
                && std::count(note.begin(), note.end(), '\n') == 1,
            "a bare jitter rule on a probe-free report is the usage "
            "error, one line and status 2 (FR-032, SC-004)");
      const auto runNote =
          captureError({"--compare=" + freeA + "," + freeB,
                        "--compare-rule=jitter:run:any:p99Ns:lower:abs=5000"},
                       2);
      check(runNote.rfind("error: ", 0) == 0
                && std::count(runNote.begin(), runNote.end(), '\n') == 1,
            "a run-scoped jitter rule on a probe-free report is the "
            "usage error, one line and status 2 (FR-032, SC-004)");
      std::remove(freeA.c_str());
      std::remove(freeB.c_str());
    }

    std::remove(basePath.c_str());
    std::remove(bigPath.c_str());
  }

  std::puts("harness_jitter_window_test: ok");
  return 0;
}
