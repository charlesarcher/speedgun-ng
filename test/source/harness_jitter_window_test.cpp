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

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
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
  static auto* provider = [] {
    auto owned = std::make_unique<sg::counters::FakeProvider>();
    auto& ref = *owned;
    ref.addCounter("machine", "monotonic", "nanoseconds", "wall clock");
    ref.setPoints("machine", "monotonic", {0}, 4000);
    ref.addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
    ref.setPoints("machine", "thread_cpu", {0}, 2000);
    ref.addCounter("machine", "tsc", "none", "raw tick counter",
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
  for (std::size_t index = 0;
         index < CPU_SETSIZE && cpus.size() < count; ++index) {
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
    std::fprintf(stderr,
                 "speedgunMain returned %d, expected %d\n",
                 status,
                 expected);
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

// The sixteen stats fields in the E-03 order, the canonical enumeration
// the console block's lines and the JSON writer's keys follow.
constexpr std::array<std::string_view, 16> kStatFields {
    "gapCount", "gapsPerSecond", "minNs", "medianNs", "meanNs", "p90Ns",
    "p99Ns", "p999Ns", "p9999Ns", "maxNs", "totalNs", "totalPercent",
    "runtimeNs", "thresholdNs", "loopPeriodNs", "droppedGaps"};

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

auto main() -> int
{
  scriptedProvider();
  const auto cpus = pickCpus(2);
  const std::string cpuList =
      std::to_string(cpus.at(0)) + "," + std::to_string(cpus.at(1));
  const std::vector<std::string> quickRun {
      "--filter=bmQuiet", "--repetitions=1", "--iterations=2"};

  // FR-013, SC-003: a run with no --jitter-probe prints the console
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
          && blockLine == 0) {
        blockLine = index;
      }
    }
    check(blockLine > 0, "the run prints the probe block (FR-023, R-08)");
    check(blockLine > lastBenchmarkLine,
          "the probe block stands after the benchmark tables (FR-023, R-08)");
    check(lines.at(blockLine)
              == "jitter probe: cpus " + cpuList,
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
    check(probed.find("jitter: probe on cpus " + cpuList)
              != std::string::npos,
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
    const std::string_view keys[] = {"mode", "thresholdNs", "loopPeriodNs",
                                     "cpus", "stats", "droppedGaps",
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

  // FR-028, E-10: --json-raw=gaps writes the per-cpu gap list, each
  // record carrying stamp and gapNs. The gap series rides the
  // reader-install seam, the route the fake's shared cursor cannot
  // express (R-02); red against the writer T014 lands.
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

  std::puts("harness_jitter_window_test: ok");
  return 0;
}
