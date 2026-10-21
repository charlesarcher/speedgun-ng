// ============================================================================
// T008: the option grammar of contracts/cli-options.md, driven directly.
//
// Every value's rejection stands one line on stderr with the
// --jitter-probe prefix and exit status 2, before any benchmark runs
// (FR-008, FR-009, FR-011, FR-014, FR-038, R-06); the standing
// getopt_long paths keep their shapes (contracts/cli-options.md); and
// the option-value uniqueness test scans the parser's own table through
// the source path the registration hands the binary (FR-011, R-06), the
// way the schema tests read their schema path. The mode-machine
// scenario reads the JSON document, so it stands red against the writer
// T013 lands, and runs after the green scenarios. Frameworkless
// check()/fail() convention.
// ============================================================================

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <set>
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

#ifndef SG_CLI_SOURCE
#error "the registration passes the CLI source path the scan reads (R-06)"
#endif

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS JITTER CLI TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// A trivial benchmark: an accepted value proceeds into the batch, so
// the acceptance scenarios need one registered function.
auto bmQuiet(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

// The scripted provider: the leaves the runner resolves its plans
// through, plus the machine/tsc leaf the probe's availability check
// reads (R-02, Q-1). The tsc series steps by one tick, so the probe
// records no gap and the batch stays quiet.
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

// The cpu index the scenarios pin probe threads to, taken from the
// process affinity mask, below the online count (FR-040).
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

// One speedgunMain run with stdout captured and the status checked
// (FR-036); the acceptance scenarios read the console text.
auto captureRun(const std::vector<std::string>& arguments,
                const int expected = 0) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_jitter_cli_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  static int runIndex = 0;
  const std::string path =
      "harness_jitter_cli_out_" + std::to_string(++runIndex) + ".txt";
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

// One speedgunMain run with stderr captured: the option's rejections
// print their one line there (FR-038, R-06).
auto captureError(const std::vector<std::string>& arguments,
                  const int expected) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_jitter_cli_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  static int runIndex = 0;
  const std::string path =
      "harness_jitter_cli_err_" + std::to_string(++runIndex) + ".txt";
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

// The one-line rule: a rejection prints exactly one line on stderr and
// names the option (FR-011, FR-038, R-06).
auto checkOneLine(const std::string& note, const char* what) -> void
{
  check(note.rfind("--jitter-probe: ", 0) == 0, what);
  check(std::count(note.begin(), note.end(), '\n') == 1, what);
}

auto readDocument(const std::string& path) -> std::string
{
  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

}  // namespace

SG_BENCHMARK(bmQuiet);

auto main() -> int
{
  scriptedProvider();
  const auto cpus = pickCpus(1);
  const std::string cpu = std::to_string(cpus.at(0));
  const std::vector<std::string> quickRun {
      "--filter=bmQuiet", "--repetitions=1", "--iterations=2"};

  // FR-011: the cpu list, `idle`, and every key parse once, each key
  // standing once in the value; the accepted value proceeds into the
  // batch.
  {
    auto arguments = quickRun;
    arguments.push_back("--jitter-probe=" + cpu + ",threshold=500,"
                        "mode=alongside,precheck=50,gaps=4,strict");
    const auto report = captureRun(arguments);
    check(report.find("bmQuiet") != std::string::npos,
          "the accepted value runs the batch (FR-011)");
  }
  {
    auto arguments = quickRun;
    arguments.push_back("--jitter-probe=idle");
    const auto report = captureRun(arguments);
    check(report.find("bmQuiet") != std::string::npos,
          "the idle value parses and the batch runs (FR-011)");
  }

  // FR-011: a repeated key stands rejected on one line and status 2,
  // before any benchmark runs.
  {
    auto arguments = quickRun;
    arguments.push_back("--jitter-probe=" + cpu + ",threshold=1,threshold=2");
    const auto note = captureError(arguments, 2);
    checkOneLine(note, "a repeated key stands rejected (FR-011)");
  }

  // FR-008, FR-038: threshold=0, a negative value, a non-number each
  // stand one line and status 2.
  for (const auto* value : {"0", "-5", "abc"}) {
    auto arguments = quickRun;
    arguments.push_back(std::string("--jitter-probe=") + cpu
                        + ",threshold=" + value);
    const auto note = captureError(arguments, 2);
    checkOneLine(note, "a bad threshold stands rejected (FR-008, FR-038)");
  }

  // FR-009: gaps=0, a value above 16777216, a non-number.
  for (const auto* value : {"0", "16777217", "abc"}) {
    auto arguments = quickRun;
    arguments.push_back(std::string("--jitter-probe=") + cpu
                        + ",gaps=" + value);
    const auto note = captureError(arguments, 2);
    checkOneLine(note, "a bad gaps value stands rejected (FR-009)");
  }

  // FR-014, FR-038: a mode word outside the three, and precheck=0 or a
  // non-number.
  {
    auto arguments = quickRun;
    arguments.push_back("--jitter-probe=" + cpu + ",mode=waltz");
    checkOneLine(captureError(arguments, 2),
                 "a mode word outside the three stands rejected (FR-014)");
  }
  for (const auto* value : {"0", "abc"}) {
    auto arguments = quickRun;
    arguments.push_back(std::string("--jitter-probe=") + cpu
                        + ",precheck=" + value);
    checkOneLine(captureError(arguments, 2),
                 "a bad precheck value stands rejected (FR-014, FR-038)");
  }

  // contracts/cli-options.md: `--jitter-probe` with no argument keeps
  // the standing getopt_long missing-argument path, and an unknown flag
  // the standing line; both exit status 2 and no benchmark runs.
  {
    const auto note = captureError({"--jitter-probe"}, 2);
    check(!note.empty(),
          "the missing argument keeps the standing getopt path "
          "(contracts/cli-options.md)");
  }
  {
    const auto note = captureError({"--jitter-probe-nonsense"}, 2);
    check(note.find("unknown option") != std::string::npos,
          "an unknown flag keeps the standing line "
          "(contracts/cli-options.md)");
  }

  // FR-011, R-06: every value in the option table is unique. The scan
  // reads the parser's own table through the source path the
  // registration hands the binary, the way the schema tests read their
  // schema path.
  {
    std::ifstream source(SG_CLI_SOURCE);
    check(source.is_open(), "the CLI source stands readable (R-06)");
    std::stringstream buffer;
    buffer << source.rdbuf();
    const auto text = buffer.str();
    const auto tableStart = text.find("option longOptions[]");
    check(tableStart != std::string::npos,
          "the option table stands in the source (R-06)");
    const auto tableEnd = text.find("\n  };", tableStart);
    check(tableEnd != std::string::npos,
          "the option table closes (R-06)");

    std::set<std::string> values;
    auto cursor = text.find('{', tableStart);
    while (cursor != std::string::npos && cursor < tableEnd) {
      const auto entryEnd = text.find('}', cursor);
      if (entryEnd == std::string::npos || entryEnd > tableEnd) {
        break;
      }
      const auto entry = text.substr(cursor, entryEnd - cursor);
      const auto nameStart = entry.find('"');
      const auto nameEnd = entry.find('"', nameStart + 1);
      if (nameStart != std::string::npos && nameEnd != std::string::npos) {
        const auto name = entry.substr(nameStart, nameEnd - nameStart);
        if (name.empty()) {
          // The terminator's entry: {"nullptr", 0, nullptr, 0}.
          break;
        }
        const auto valueStart = entry.rfind(", ");
        check(valueStart != std::string::npos,
              "the entry carries its value (R-06)");
        const auto value = entry.substr(valueStart + 2);
        const auto inserted = values.insert(value);
        check(inserted.second,
              "every value in the option table is unique (FR-011, R-06)");
      }
      cursor = text.find('{', entryEnd + 1);
    }
    check(values.size() > 15,
          "the scan read the whole table (FR-011, R-06)");
  }

  // FR-013, SC-003: a run with no --jitter-probe prints the console
  // report with the fields and the shape master prints.
  {
    const auto report = captureRun(quickRun);
    check(report.find("bmQuiet") != std::string::npos,
          "the probe-free run prints the benchmark (FR-013, SC-003)");
    check(report.find("jitter probe:") == std::string::npos
              && report.find("jitter: ") == std::string::npos,
          "the probe-free run prints no probe block and no disclosure "
          "(FR-013, SC-003)");
  }

  // FR-012, D-1: the mode machine. No mode= value stands ALONGSIDE.
  // Red against the JSON writer T013 lands: the document carries no
  // jitter scope, so the mode word stands missing.
  {
    const std::string path = "harness_jitter_cli_mode.json";
    auto arguments = quickRun;
    arguments.push_back("--json=" + path);
    arguments.push_back("--jitter-probe=" + cpu);
    captureRun(arguments);
    const auto document = readDocument(path);
    std::remove(path.c_str());
    check(document.find("\"mode\":\"ALONGSIDE\"") != std::string::npos,
          "no mode= value stands alongside (FR-012, D-1)");
  }

  std::puts("harness_jitter_cli_test: ok");
  return 0;
}
