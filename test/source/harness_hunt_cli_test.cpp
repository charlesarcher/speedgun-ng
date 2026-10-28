// ============================================================================
// The hunt command-line refusals and accepted forms
// (specs/029-hunter-core T018; US1; FR-001, FR-006, FR-060 to FR-068, FR-080
// to FR-083; AC-5, SC-05; research D-1, D-16, D-19, D-22;
// contracts/hunt-cli.md).
//
// The seam is `sg::speedgunMain`, the entry point
// `test/source/harness_cli_test.cpp` drives. Every scenario runs in a re-exec
// of this binary, because each scripts the host of `detail/hunt_fixture.hpp`
// through a `FakeProvider` of its own and the counters system takes these
// object paths once per process, so no case reaches a PMU, a clock, a sleep or
// a CPU count (FR-092).
//
// A refusal row is checked through a child, the pattern of
// `test/source/harness_cli_test.cpp:820`, `:1186` and `:1437`: the refusal path
// may return the status or call `std::exit`, and a child answers both shapes
// with the same wait status. The child prints `BENCHMARK_NO_RUN` when
// `speedgunMain` reached no benchmark function, so the exact bytes of the child
// answer are `one refusal line + the marker`: exactly one stderr line naming
// the value, and no run. An accepted form is checked in process, where the
// entry point returns its status and the captured stdout carries the hunt lines
// and the tables.
//
// The scenario list below is what CTest runs, so a scenario absent from it
// never runs, and `argv[1]` selects one entry of it. Frameworkless
// check()/fail() convention, the one `harness_cli_test.cpp` carries.
// ============================================================================

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "detail/hunt_fixture.hpp"
#include "detail/hunt_selection.hpp"
#include "detail/internal.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"
#include "speedgun-ng/counters_fake.hpp"
#include "speedgun-ng/counters_system.hpp"
#include "speedgun-ng/hunt.hpp"

namespace
{

// The scripted hosts the child modes and the scenarios name: the limit of
// `kHuntRows` the case needs. `narrow` is the device that publishes below the
// three slots a hunt pass needs.
constexpr const char* kScriptFull = "full";
// The full host beside the machine/tsc leaf the jitter probe reads, the
// host the hunt-and-probe scenario runs (D-08, FR-035).
constexpr const char* kScriptHuntProbe = "hunt-probe";
constexpr const char* kScriptNarrow = "narrow";
constexpr const char* kScriptNoLimit = "nolimit";
// `nocycles` and `noinstr` refuse one anchor leaf on every core device, the
// host D-13 refuses an explicit norm on.
constexpr const char* kScriptNoCycles = "nocycles";
constexpr const char* kScriptNoInstr = "noinstr";
// `seatrefuse` opens nothing at all: the scripted group ceiling of zero leaves
// refuses every window the run asks for, the host the corsairone defect record
// met at its first seat.
constexpr const char* kScriptSeatRefuse = "seatrefuse";
// `narrowgiraffe` is the `narrow` host beside the out-of-tree device of the
// giraffe provider of specs/007: a path two levels deep, so its address carries
// two slashes and the device the limit list keys by is not the text before the
// first one.
constexpr const char* kScriptNarrowGiraffe = "narrowgiraffe";
constexpr std::string_view kGiraffeParent {"menagerie"};
constexpr std::string_view kGiraffeDevice {"menagerie/giraffe-2"};
constexpr std::string_view kGiraffeLeaf {"honks"};
constexpr std::size_t kFullLimit = 6;
constexpr std::size_t kNarrowLimit = 2;

// The scratch files of this binary: the printed-report capture and the `@file`
// selections the contract's rows name. Every name stands in the system
// temporary directory under this process number, so a run from the checkout
// root leaves no file behind and a second copy of this binary shares no file.
// The path is absolute because a refusal child reads the `@file` whose name
// its re-exec carries.
auto capturePath(const char* kind) -> std::filesystem::path
{
  return std::filesystem::temp_directory_path()
      / ("harness_hunt_cli_" + std::to_string(static_cast<long>(getpid()))
         + kind);
}

auto scratchPath() -> std::filesystem::path
{
  return capturePath("_out.txt");
}

auto documentPath() -> std::filesystem::path
{
  return capturePath("_report.json");
}

auto blankFile() -> std::filesystem::path
{
  return capturePath("_blank.txt");
}

auto rawFile() -> std::filesystem::path
{
  return capturePath("_raw.txt");
}

auto listFile() -> std::filesystem::path
{
  return capturePath("_list.txt");
}

auto dupFile() -> std::filesystem::path
{
  return capturePath("_dup.txt");
}

// The two files of the file-selection refusals below: one whose every line is
// a comment, one that carries an address beside a raw code in the upper-case
// hex spelling no other row quotes.
auto parkedFile() -> std::filesystem::path
{
  return capturePath("_parked.txt");
}

auto upperRawFile() -> std::filesystem::path
{
  return capturePath("_upper_raw.txt");
}

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS HUNT CLI TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool condition, const char* what) -> void
{
  if (!condition) {
    fail(what);
  }
}

auto address(const std::size_t index) -> std::string
{
  return sg::test::huntAddress(sg::test::kHuntCpu, sg::test::huntEvent(index));
}

auto writeFile(const std::filesystem::path& path,
               const std::string& text) -> void
{
  std::ofstream file(path, std::ios::trunc);
  check(static_cast<bool>(file), "the scenario could write its @file fixture");
  file << text;
  file.close();
  check(static_cast<bool>(file), "the @file fixture holds its bytes");
}

// Lay down every `@file` a row of the contract names. The blank file carries
// comments and blank lines only, the raw file one address and one raw code, the
// list file the comma and newline spellings with their comments.
auto writeSelectionFiles() -> void
{
  writeFile(blankFile(), "# a comment\n\n   \n# another comment\n");
  writeFile(rawFile(), address(0) + "\nr4a03\n");
  writeFile(listFile(),
            "# one address per line\n" + address(0) + "\n\n" + address(1) + ","
                + address(2) + "\n");
  writeFile(dupFile(),
            "# the same address the list carries\n" + address(0) + "\n");
}

// ---------------------------------------------------------------------------
// The capture and child seams of `harness_cli_test.cpp`
// ---------------------------------------------------------------------------

// Refuse one anchor leaf on every core device that owns one: the leaf stays in
// the catalog with its disclosure, the shape D-13 describes.
auto refuseAnchor(sg::counters::FakeProvider& host,
                  const std::string_view leaf) -> void
{
  for (const auto device : sg::test::huntDevices()) {
    if (device == sg::test::kHuntUncore) {
      continue;
    }
    host.addCounter(device,
                    leaf,
                    "none",
                    "a refused anchor",
                    sg::counters::Availability::PERMISSION_BLOCKED);
  }
}

auto registerHost(const std::string& script) -> void
{
  auto provider = std::make_unique<sg::counters::FakeProvider>();
  if (script == kScriptNoLimit) {
    sg::test::huntHost(*provider, std::nullopt);
  } else if (script == kScriptNarrow) {
    sg::test::huntHost(*provider, kNarrowLimit);
  } else if (script == kScriptNoCycles) {
    refuseAnchor(sg::test::huntHost(*provider, kFullLimit),
                 sg::test::kHuntCycle);
  } else if (script == kScriptNoInstr) {
    refuseAnchor(sg::test::huntHost(*provider, kFullLimit),
                 sg::test::kHuntInstr);
  } else if (script == kScriptHuntProbe) {
    // The three leaves the probe reads: the wall clock and the thread
    // clock the disclosure reads beside the tsc leaf the loop spins on,
    // scripted the way the jitter fixtures script them (D-08, FR-035).
    sg::test::huntHost(*provider, kFullLimit)
        .addCounter("machine", "monotonic", "nanoseconds", "fixture wall clock")
        .setPoints("machine", "monotonic", {0}, 4000)
        .addCounter(
            "machine", "thread_cpu", "nanoseconds", "fixture thread cpu")
        .setPoints("machine", "thread_cpu", {0}, 2000)
        .addCounter("machine",
                    "tsc",
                    "none",
                    "fixture tick",
                    sg::counters::Availability::COUNTABLE,
                    sg::counters::ReadMode::FAST_TSC)
        .setPoints("machine", "tsc", {}, 0, 1);
  } else if (script == kScriptSeatRefuse) {
    sg::test::huntHost(*provider, kFullLimit)
        .setGroupLeafLimit(sg::test::kHuntCpu, 0);
  } else if (script == kScriptNarrowGiraffe) {
    // The parent object has to stand for the nested device to reach a catalog
    // walk at all: `Object::children()` nests one level per `/`.
    sg::test::huntHost(*provider, kFullLimit)
        .addObject(kGiraffeParent, "menagerie", "hunt fixture menagerie")
        .addObject(kGiraffeDevice, "menagerie", "hunt fixture giraffe")
        .setProgrammableCounters(kGiraffeDevice, kNarrowLimit)
        .addCounter(kGiraffeDevice, kGiraffeLeaf, "none", "fixture honks")
        .setPoints(kGiraffeDevice, kGiraffeLeaf, {0}, sg::test::huntValue(0));
  } else {
    sg::test::huntHost(*provider, kFullLimit);
  }
  const auto settled =
      sg::counters::System::local().registerProvider(std::move(provider));
  check(settled.has_value(), "the hunt fixture registers on the local system");
}

std::atomic<int> huntRuns {0};

auto bmHuntCli(sg::State& state) -> void
{
  ++huntRuns;
  for (auto _ : state) {
  }
}

auto registerCase(const char* name = "bmHuntCli") -> sg::BenchmarkHandle
{
  auto handle = sg::registerBenchmark(&bmHuntCli, name);
  handle.iterations(1);
  return handle;
}

std::string selfPath;

auto captureRun(const std::vector<std::string>& arguments,
                const int expected = 0) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_hunt_cli_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  const int saved = dup(STDOUT_FILENO);
  if (saved < 0 || std::freopen(scratchPath().c_str(), "w", stdout) == nullptr)
  {
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
    fail("the exit status does not follow contracts/hunt-cli.md (FR-081)");
  }

  std::ifstream file(scratchPath());
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

auto subprocessOutput(const std::string& command, std::string& output) -> int
{
  FILE* const pipe = popen(command.c_str(), "r");
  if (pipe == nullptr) {
    fail("the usage subprocess starts");
  }
  char buffer[256];
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    output += buffer;
  }
  return pclose(pipe);
}

// Re-exec this binary in one of its usage modes with the arguments given, and
// report the child's merged output; the answer is the wait status. Every
// argument stands quoted, so a word with nothing in it survives the shell.
auto childRun(const std::string& self,
              const std::string& mode,
              const std::vector<std::string>& arguments,
              std::string& output,
              const std::string& environment = {}) -> int
{
  std::string command = environment + "\"" + self + "\" " + mode;
  for (const auto& argument : arguments) {
    command += " \"" + argument + "\"";
  }
  return subprocessOutput(command + " 2>&1", output);
}

// One refusal row of contracts/hunt-cli.md: the scripted host, the command
// line, and the one line the contract states for it.
struct RefusalRow
{
  const char* script;
  const std::vector<std::string> arguments;
  std::string line;
  // The environment the child inherits, spelled as the shell reads an
  // assignment: `NAME=value ` before the binary. The child receives the
  // variable through its environment array, so no test calls `setenv`
  // (FR-056). The default stands empty, so the rows that inherit the
  // parent's environment name no member.
  std::string environment = {};
};

auto rawCodeLine(const std::string& code = "r4a03") -> std::string
{
  return "--hunt: '" + code
      + "' is a raw event code; --hunt takes catalog addresses";
}

// The one line a selection of files leaves with when their lines hold no
// address. The whole selection stands quoted, so two file names ride one pair
// of quotes.
auto holdsNoAddressLine(const std::string& selection) -> std::string
{
  return "--hunt: '" + selection + "' holds no catalog address";
}

// The one line a reserved rank key leaves with, naming the word the option
// carried, the work that owns it and the words the option takes
// (contracts/hunt-cli.md, D-3, FR-032).
auto reservedRankLine(const char* word) -> std::string
{
  return std::string {"--hunt-rank: '"} + word
      + "' is reserved for N14b; --hunt-rank takes rate or cv";
}

// One row, checked in a child: status 2, exactly one line naming the value, and
// no benchmark function reached (FR-081, AC-5, SC-05).
auto checkRefusal(const RefusalRow& row) -> void
{
  std::vector<std::string> arguments {row.script};
  for (const auto& argument : row.arguments) {
    arguments.push_back(argument);
  }
  std::string output;
  const int status =
      childRun(selfPath, "hunt", arguments, output, row.environment);
  const std::string expected = row.line + "\nBENCHMARK_NO_RUN\n";
  if (output != expected) {
    std::fprintf(stderr,
                 "%s exited %d with (%zu vs %zu):\n%s\n",
                 row.arguments.front().c_str(),
                 WEXITSTATUS(status),
                 output.size(),
                 expected.size(),
                 output.c_str());
  }
  check(WEXITSTATUS(status) == 2,
        "a hunt refusal exits with the usage status (FR-081, AC-5)");
  check(output == expected,
        "the refusal prints exactly one line naming the value and reaches no "
        "benchmark (FR-081, SC-05)");
}

auto checkRefusals(const std::vector<RefusalRow>& rows) -> void
{
  for (const auto& row : rows) {
    checkRefusal(row);
  }
}

// The hunt lines of a captured run, in the spelling contracts/hunt-cli.md
// states.
auto lineHolding(const std::string& text,
                 const std::string& needle) -> std::string
{
  std::istringstream stream(text);
  std::string line;
  while (std::getline(stream, line)) {
    if (line.find(needle) != std::string::npos) {
      return line;
    }
  }
  return {};
}

auto huntLines(const std::string& text,
               const std::string& needle) -> std::vector<std::string>
{
  std::vector<std::string> found;
  std::istringstream stream(text);
  std::string line;
  while (std::getline(stream, line)) {
    if (line.rfind("hunt: ", 0) == 0 && line.find(needle) != std::string::npos)
    {
      found.push_back(line);
    }
  }
  return found;
}

auto countOf(const std::string& text, const std::string& needle) -> std::size_t
{
  std::size_t count = 0;
  for (std::size_t at = text.find(needle); at != std::string::npos;
       at = text.find(needle, at + needle.size()))
  {
    ++count;
  }
  return count;
}

auto planLine(const std::string& text) -> std::string
{
  return lineHolding(text, "hunt: plan events=");
}

auto readAll(const std::string& path) -> std::string
{
  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// The command line every accepted-form case runs, kept short so the scripted
// host counts its own passes, calibration a fixed figure behind them.
auto withFilter(const std::vector<std::string>& arguments)
    -> std::vector<std::string>
{
  std::vector<std::string> full {"--filter", "^bmHuntCli$", "--iterations=1"};
  for (const auto& argument : arguments) {
    full.push_back(argument);
  }
  return full;
}

// ---------------------------------------------------------------------------
// One case per refusal row of contracts/hunt-cli.md
// ---------------------------------------------------------------------------

auto scenarioSelectionRefusals() -> void
{
  writeSelectionFiles();
  const std::string raw = rawCodeLine();
  checkRefusals({{"full",
                  {"--hunt=cpu/nope"},
                  "--hunt: 'cpu/nope' is not a catalog address"},
                 {"full",
                  {"--hunt=no/such*"},
                  "--hunt: pattern 'no/such*' matches no catalog address"},
                 {"full",
                  {"--hunt=@hunt_cli_missing.txt"},
                  "--hunt: '@hunt_cli_missing.txt' cannot be opened"},
                 {"full",
                  {"--hunt=@" + blankFile().string()},
                  holdsNoAddressLine("@" + blankFile().string())},
                 {"full", {"--hunt=r4a03"}, raw},
                 {"full", {"--hunt=cpu/r4a03"}, raw},
                 {"full", {"--hunt=" + address(0) + ",r4a03"}, raw},
                 {"full", {"--hunt=cpu/r4a03*"}, raw},
                 // The bare form with its trailing glob: the glob strips
                 // before the shape read, so the raw code stands named
                 // without it (FR-047).
                 {"full", {"--hunt=r4*"}, rawCodeLine("r4")},
                 {"full", {"--hunt=cpu/r4A03"}, rawCodeLine("r4A03")},
                 {"full",
                  {"--hunt=cpu/r4g03"},
                  "--hunt: 'cpu/r4g03' is not a catalog address"},
                 {"full", {"--hunt=@" + std::string(rawFile())}, raw},
                 {"full",
                  {"--hunt=\t\r cpu/cpu-cycles \r\t,cpu/nope"},
                  "--hunt: 'cpu/nope' is not a catalog address"},
                 {"full",
                  {"--hunt=cpu/cpu-cycles,,"},
                  "--hunt: '' is not a catalog address"},
                 {"full", {"--hunt= "}, "--hunt: ' ' is not a catalog address"},
                 {"full", {"--hunt=r4A"}, rawCodeLine("r4A")}});
  std::printf("selection-refusals: ok\n");
}

// An attached value with nothing in it is one thing and a bare `--hunt` is
// another: the bare form parks the record for the scan to settle, while
// `--hunt=` carried a value, and that value is the empty text, which is no
// catalog address. The shape rule answers it where the option read it, so the
// same empty text refuses on a host that counts nothing (D-11, FR-006, FR-081).
auto scenarioEmptyAttachedValue() -> void
{
  checkRefusals({{"full", {"--hunt="}, "--hunt: '' is not a catalog address"}});
  std::printf("empty-attached-value: ok\n");
}

auto scenarioOptionValueRefusals() -> void
{
  checkRefusals(
      {{"full",
        {"--hunt-norm=ipc"},
        "--hunt-norm: expected instr, cycle or iter, got 'ipc'"},
       {"full", {"--hunt-rank=sensitivity"}, reservedRankLine("sensitivity")},
       {"full", {"--hunt-rank=delta"}, reservedRankLine("delta")},
       {"full", {"--hunt-rank=score"}, reservedRankLine("score")},
       {"full",
        {"--hunt-rank=rate2"},
        "--hunt-rank: expected rate or cv, got 'rate2'"},
       {"full", {"--hunt-top=0"}, "--hunt-top: expected 1 to 10000, got '0'"},
       {"full",
        {"--hunt-top=10001"},
        "--hunt-top: expected 1 to 10000, got '10001'"},
       {"full",
        {"--hunt-noise=0"},
        "--hunt-noise: expected a percentage above 0, got '0'"},
       {"full",
        {"--hunt-noise=-1"},
        "--hunt-noise: expected a percentage above 0, got '-1'"},
       {"full",
        {"--hunt-repeats=0"},
        "--hunt-repeats: expected 1 to 1000, got '0'"},
       {"full",
        {"--hunt-repeats=1001"},
        "--hunt-repeats: expected 1 to 1000, got '1001'"},
       {"full",
        {"--hunt-budget=120"},
        "--hunt-budget: expected <n>s, <n>p, <n>e or <n>g, got '120'"},
       {"full",
        {"--hunt-budget=abc"},
        "--hunt-budget: expected <n>s, <n>p, <n>e or <n>g, got 'abc'"},
       {"full",
        {"--hunt-budget=0s"},
        "--hunt-budget: expected a count above 0, got '0s'"},
       {"full",
        {"--hunt-budget=0p"},
        "--hunt-budget: expected a count above 0, got '0p'"},
       {"full",
        {"--hunt-budget=-5p"},
        "--hunt-budget: expected a count above 0, got '-5p'"},
       {"full",
        {"--hunt-seed=abc"},
        "--hunt-seed: expected a whole number, got 'abc'"},
       {"full",
        {"--hunt-top=99999999999999999999999"},
        "--hunt-top: expected 1 to 10000, got '99999999999999999999999'"},
       {"full",
        {"--hunt-top=12x"},
        "--hunt-top: expected 1 to 10000, got '12x'"},
       {"full",
        {"--hunt-top=abc"},
        "--hunt-top: expected 1 to 10000, got 'abc'"},
       {"full",
        {"--hunt-noise=1e400"},
        "--hunt-noise: expected a percentage above 0, got '1e400'"},
       {"full",
        {"--hunt-noise=abc"},
        "--hunt-noise: expected a percentage above 0, got 'abc'"},
       {"full",
        {"--hunt-noise=5x"},
        "--hunt-noise: expected a percentage above 0, got '5x'"},
       {"full",
        {"--hunt-noise=inf"},
        "--hunt-noise: expected a percentage above 0, got 'inf'"},
       {"full",
        {"--hunt-noise=nan"},
        "--hunt-noise: expected a percentage above 0, got 'nan'"}});
  std::printf("option-value-refusals: ok\n");
}

// The clause grammar of `--hunt-budget=` (specs/033-hunter-budgets T015, T022,
// FR-003, FR-004, R-01; contracts/budget-grammar.md). Each row stands for a row
// of that contract: a kind spelled twice, an empty clause, a suffix that names
// no kind, a value that opens or ends with a blank, an `e` word below one
// nanosecond, an `e` word above the seconds bound and a `g` count outside the
// range each answer one line that quotes the whole value and end the run at
// status 2.
auto scenarioBudgetClauseRefusals() -> void
{
  // The three sentences the grammar answers, in the standing family: the
  // option, what it expected, and the whole value it read.
  constexpr const char* kShape =
      "--hunt-budget: expected <n>s, <n>p, <n>e or <n>g";
  constexpr const char* kSeconds =
      "--hunt-budget: expected a finite positive number of seconds at most "
      "9223372036";
  constexpr const char* kCount = "--hunt-budget: expected 1 to 10000";
  const auto line = [](const char* expected,
                       const std::string& value) -> std::string
  { return std::string(expected) + ", got '" + value + "'"; };

  checkRefusals({
      {"full", {"--hunt-budget=0.5e,2e"}, line(kShape, "0.5e,2e")},
      {"full", {"--hunt-budget=8p,2p"}, line(kShape, "8p,2p")},
      {"full", {"--hunt-budget=32g,16g"}, line(kShape, "32g,16g")},
      {"full", {"--hunt-budget=120s,,1e"}, line(kShape, "120s,,1e")},
      {"full", {"--hunt-budget=120x"}, line(kShape, "120x")},
      {"full", {"--hunt-budget= 120s"}, line(kShape, " 120s")},
      {"full", {"--hunt-budget=120s "}, line(kShape, "120s ")},
      {"full", {"--hunt-budget=0g"}, line(kCount, "0g")},
      {"full", {"--hunt-budget=10001g"}, line(kCount, "10001g")},
      {"full", {"--hunt-budget=1.5g"}, line(kCount, "1.5g")},
      {"full", {"--hunt-budget=xg"}, line(kCount, "xg")},
      {"full", {"--hunt-budget=xe"}, line(kSeconds, "xe")},
      {"full", {"--hunt-budget=0e"}, line(kSeconds, "0e")},
      {"full",
       {"--hunt-budget=0.0000000001e"},
       line(kSeconds, "0.0000000001e")},
      {"full", {"--hunt-budget=9223372037e"}, line(kSeconds, "9223372037e")},
  });
  std::printf("budget-clause-refusals: ok\n");
}

auto scenarioBudgetClauseAccepted() -> void
{
  registerHost(kScriptFull);
  registerCase();

  // The accepted rows read the plan line, which carries each clause in the
  // canonical text rebuilt from the settled figure: the standing budget first,
  // then `sample=`, then `mux=`, in that order (FR-012, FR-023, N-06).
  struct Form
  {
    std::string_view value;
    std::string_view segments;
  };

  for (const Form form :
       {Form {"120s", "budget=120s"},
        Form {"8p", "budget=8p"},
        Form {"0.5e", "budget=none sample=0.5e"},
        Form {"1e", "budget=none sample=1e"},
        Form {"1e,120s", "budget=120s sample=1e"},
        Form {"120s,1e", "budget=120s sample=1e"},
        Form {"9223372036e", "budget=none sample=9223372036e"},
        Form {"32g", "budget=none mux=32g"},
        Form {"1g", "budget=none mux=1g"},
        Form {"10000g", "budget=none mux=10000g"},
        Form {"32g,120s", "budget=120s mux=32g"},
        Form {"120s,0.5e,32g", "budget=120s sample=0.5e mux=32g"}})
  {
    const std::string run = captureRun(
        withFilter({"--hunt=all", "--hunt-budget=" + std::string(form.value)}));
    check(run.find("hunt: plan events=") != std::string::npos,
          "an accepted budget clause value reaches the plan line (FR-004)");
    check(run.find(std::string(form.segments)) != std::string::npos,
          "the plan line carries the clause texts of the value, in order and "
          "in their canonical spellings (FR-012, FR-023)");
  }
  std::printf("budget-clause-accepted: ok\n");
}

auto scenarioLimitRefusal() -> void
{
  // The binding rule: the device whose published limit stands below three is
  // refused before any planning, so no plan line reaches the output and the
  // marker bounds what the child printed (FR-081, spec PC-2).
  checkRefusal(
      {"narrow",
       {"--hunt=" + address(0)},
       "hunt: device 'cpu' publishes 2 slots and a hunt pass needs 3"});
  checkRefusal(
      {"narrow",
       {"--hunt=all"},
       "hunt: device 'cpu' publishes 2 slots and a hunt pass needs 3"});
  std::printf("limit-refusal: ok\n");
}

// The slot rule of spec PC-2 asks of event addresses only: an anchor leaf needs
// no slot of its own (D-3). So `--hunt=cpu/cpu-cycles` of the two-slot host
// stands open at the gate that refuses the event addresses of the same host,
// the benchmark runs, and what the run answers is the planner's own limit
// refusal: the device-limit line, the same bytes the option route prints, so
// one device parts over no wording between the two routes (T164).
auto scenarioAnchorLeafNeedsNoSlot() -> void
{
  const std::string anchor =
      sg::test::huntAddress(sg::test::kHuntCpu, sg::test::kHuntCycle);
  std::string output;
  const int status =
      childRun(selfPath, "hunt", {kScriptNarrow, "--hunt=" + anchor}, output);
  check(WIFEXITED(status) && WEXITSTATUS(status) == 1,
        "an anchor-only selection passes the slot gate and answers the failure "
        "status of the refused plan, not the usage status (FR-081, spec PC-2)");
  check(output.find("BENCHMARK_RAN") != std::string::npos,
        "the run of an anchor-only selection reaches the benchmark");
  const std::string line =
      lineHolding(output, "device 'cpu' publishes 2 slots");
  check(line == "hunt: device 'cpu' publishes 2 slots and a hunt pass needs 3",
        "the refused plan prints the device-limit line whole, the bytes "
        "contracts/hunt-cli.md pins, and the status above is what states that "
        "the option-route gate stood open for an anchor leaf (D-3, T164)");
  check(countOf(output, "slots and a hunt pass needs 3") == 1,
        "the run states its one refusal line");

  const std::string instr =
      sg::test::huntAddress(sg::test::kHuntCpu, sg::test::kHuntInstr);
  std::string instrOutput;
  const int instrStatus = childRun(
      selfPath, "hunt", {kScriptNarrow, "--hunt=" + instr}, instrOutput);
  check(WIFEXITED(instrStatus) && WEXITSTATUS(instrStatus) == 1,
        "the instructions anchor passes the slot gate as its sibling does "
        "and answers the refused plan's status (FR-081, spec PC-2)");
  check(lineHolding(instrOutput, "device 'cpu' publishes 2 slots") ==
            "hunt: device 'cpu' publishes 2 slots and a hunt pass needs 3",
        "the refused plan of the instructions-anchor selection prints the "
        "same device-limit line whole (D-3, T164)");
  std::printf("anchor-leaf-needs-no-slot: ok\n");
}

// The nested device, the same rule one path level deeper: the limit list keys a
// device by the text before the *last* `/` (E-01, `detail::devicePartOf`), so
// `menagerie/giraffe-2/honks` is checked against `menagerie/giraffe-2` and its
// two slots refuse the pass. A cut at the first `/` looks the limit up under
// `menagerie`, finds no such object, and lets the device stand open.
auto scenarioNestedDeviceLimitRefusal() -> void
{
  checkRefusal(
      {kScriptNarrowGiraffe,
       {"--hunt=" + sg::test::huntAddress(kGiraffeDevice, kGiraffeLeaf)},
       "hunt: device '" + std::string(kGiraffeDevice)
           + "' publishes 2 slots and a hunt pass needs 3"});
  std::printf("nested-device-limit-refusal: ok\n");
}

// The command line on the host that opens nothing: the half the collect suite
// disclaims in its header, the status itself. The corsairone defect record
// captured exit 134 on the wider selections, and this route aborted the same
// way for a different reason - a status set over no failure flag tripped the
// CLI's own postcondition. What is pinned here is what the shell reads: one
// line naming the refusal, and the failure status of a failed outcome
// (FR-081, spec PC-9, AC-10).
auto scenarioSeatRefusalExitsOneWithReason() -> void
{
  std::string output;
  const int status = childRun(
      selfPath, "hunt", {kScriptSeatRefuse, "--hunt=" + address(0)}, output);
  check(WIFEXITED(status),
        "a hunt whose every open refuses ends of its own accord, not on a "
        "signal (AC-10)");
  check(WEXITSTATUS(status) == 1,
        "the refused hunt answers the failure status, not the usage status "
        "(FR-081, spec PC-9)");
  // What the child owes is read by content and by count, never as whole bytes
  // and never as a line order between the two streams. `--hunt` runs the
  // benchmark first and hunts the outcome after (`cli.cpp:557-569`), so the
  // refused hunt still prints that run's row, and the row's
  // `overhead floor (ns)=` field is the median the plan measured on this host:
  // equal invocations differ in those bytes. And stderr reaches this pipe
  // unbuffered while stdout leaves it block-buffered at exit, so which stream
  // speaks first is no part of the contract either.
  check(countOf(output, "hunt: ") == 1,
        "the refused hunt prints its one error line and no hunt block beside "
        "it (FR-081, FR-052)");
  const std::string line = lineHolding(output, "hunt: ");
  check(line.rfind("hunt: ", 0) == 0 && line.size() > 6,
        "the refused hunt stands its one error line on stderr (FR-081)");
  check(line.find("does not compile") != std::string::npos,
        "the refusal names the seat that failed, not a bare status (PD-09)");
  check(line.find("cannot open a window") != std::string::npos,
        "the refusal carries the cause the counter layer answered, the words "
        "`plan.cpp:599` states and not ones this file wrote over them (D-18)");
  check(output.find("BENCHMARK_RAN") != std::string::npos
            && output.find("BENCHMARK_NO_RUN") == std::string::npos,
        "the refusal belongs to the hunt route only: the run of the benchmark "
        "itself reached its body and stands beside the refusal line");
  std::printf("seat-refusal-status: ok\n");
}

auto scenarioNormAnchorRefusals() -> void
{
  // D-13: a norm the caller stated refuses when the host counts no anchor of
  // that name (contracts/hunt-cli.md). The accepted word of the same option is
  // pinned by "repeated-option-keeps-last", where both anchors count: a run of
  // its own cannot ride the two hosts below, because a pass seats the anchors
  // of its device and the plan compiler refuses a leaf the catalog marks
  // countable nowhere (FR-023).
  checkRefusals(
      {{kScriptNoCycles,
        {"--hunt=" + address(0), "--hunt-norm=cycle"},
        "--hunt-norm: 'cycle' has no countable anchor on this host"},
       {kScriptNoInstr,
        {"--hunt=" + address(0), "--hunt-norm=instr"},
        "--hunt-norm: 'instr' has no countable anchor on this host"}});
  std::printf("norm-anchor-refusals: ok\n");
}

auto scenarioConflictRefusals() -> void
{
  const std::vector<std::pair<std::string, std::string>> pairs {
      {"--counter=cpu/instructions", "--hunt cannot combine with --counter"},
      {"--metric=ipc", "--hunt cannot combine with --metric"},
      {"--compare=nosuch-base.json,nosuch-contender.json",
       "--hunt cannot combine with --compare"},
  };
  std::vector<RefusalRow> rows;
  for (const auto& [argument, line] : pairs) {
    // The check runs after the scan, so the order of the two options on the
    // command line is not part of the refusal (contracts/hunt-cli.md).
    rows.push_back({"full", {"--hunt", argument}, line});
    rows.push_back({"full", {argument, "--hunt"}, line});
  }
  checkRefusals(rows);
  std::printf("conflict-refusals: ok\n");
}

// specs/025 FR-016 governs the scope option of a run, and this tree spells
// that option `--placement`. A hunt beside it leaves with the refusal of the
// family above, for either order on the command line.
auto scenarioPlacementConflictRefusal() -> void
{
  checkRefusals({{"full",
                  {"--hunt", "--placement=core"},
                  "--hunt cannot combine with --placement"},
                 {"full",
                  {"--placement=core", "--hunt"},
                  "--hunt cannot combine with --placement"}});
  std::printf("placement-conflict-refusal: ok\n");
}

// FR-046: the hunt runs beside the run-shape options: the threaded arm
// runs two contexts, the interleaved arm holds the walk's own order,
// and both print the rank lines the hunt carried alone.
auto scenarioHuntWithThreadsAndInterleave() -> void
{
  registerHost(kScriptFull);
  registerCase();
  // The threaded arm carries no end anchor: a thread suffix rides the
  // instance name the filter reads, so the anchor the exact spelling
  // needs would match nothing (FR-046).
  const std::vector<std::string> threaded {"--filter",
                                           "^bmHuntCli",
                                           "--iterations=1",
                                           "--hunt=" + address(0),
                                           "--threads=2"};
  const auto threadedRun = captureRun(threaded);
  check(!huntLines(threadedRun, "hunt: rank ").empty(),
        "the threaded hunt prints its rank lines (FR-046)");
  const std::vector<std::string> interleaved {"--filter",
                                              "^bmHuntCli",
                                              "--iterations=1",
                                              "--hunt=" + address(0),
                                              "--interleave"};
  const auto interleavedRun = captureRun(interleaved);
  check(!huntLines(interleavedRun, "hunt: rank ").empty(),
        "the interleaved hunt prints its rank lines (FR-046)");
  std::printf("hunt-with-threads-and-interleave: ok\n");
}

// D-08, FR-035: the hunt and the probe run together. The child carries
// the leaf the probe reads, the run completes, and the report carries
// both the hunt's records and the block the probe recorded, beside the
// benchmark the walk ran.
auto scenarioHuntWithProbe() -> void
{
  cpu_set_t mask {};
  CPU_ZERO(&mask);
  check(sched_getaffinity(0, sizeof(mask), &mask) == 0,
        "the parent reads the mask it hands the child (FR-056)");
  std::uint32_t probeCpu = 0;
  for (std::size_t index = 0; index < static_cast<std::size_t>(CPU_SETSIZE);
       ++index)
  {
    if (CPU_ISSET(index, &mask) != 0) {
      probeCpu = static_cast<std::uint32_t>(index);
      break;
    }
  }
  std::string output;
  const int status = childRun(selfPath,
                              "hunt",
                              {kScriptHuntProbe,
                               "--hunt=" + address(0),
                               "--jitter-probe=" + std::to_string(probeCpu),
                               "--filter=bmHuntCli"},
                              output);
  check(WEXITSTATUS(status) == 0,
        "the hunt beside the probe finishes the run (D-08, FR-035)");
  check(output.find("hunt: ") != std::string::npos,
        "the report carries the hunt's records (D-08, FR-035)");
  check(output.find("jitter") != std::string::npos,
        "the report carries the block the probe recorded (D-08, FR-035)");
  check(output.find("BENCHMARK_RAN") != std::string::npos,
        "the walk ran the benchmark beside the probe (FR-035)");
  std::printf("hunt-with-probe: ok\n");
}

// D-07, FR-034, FR-045: the hunt refuses an ambient value from every
// source. The environment carries `SPEEDGUN_COUNTER`, `SPEEDGUN_METRIC`
// or `SPEEDGUN_PLACEMENT`, the option file carries the row of the same
// option, and the run stops on the first it reads with one line naming
// the source, at status 2, before any benchmark runs. The child
// receives the variable through its environment array, and the option
// file stands written before the child runs and removed after, so no
// test calls `setenv` (FR-056).
auto scenarioAmbientCombinationRefusals() -> void
{
  registerHost(kScriptFull);
  checkRefusals({
      {"full",
       {"--hunt", "--filter=bmHuntCli"},
       "--hunt cannot combine with --counter",
       "SPEEDGUN_COUNTER=cycles "},
      {"full",
       {"--hunt", "--filter=bmHuntCli"},
       "--hunt cannot combine with --metric",
       "SPEEDGUN_METRIC=ipc "},
      {"full",
       {"--hunt", "--filter=bmHuntCli"},
       "--hunt cannot combine with --placement",
       "SPEEDGUN_PLACEMENT=core "},
  });
  const std::string path = "harness_hunt_cli_ambient_options.txt";
  writeFile(path, "counter=cycles\n");
  checkRefusals({{"full",
                  {"--options=" + path, "--hunt", "--filter=bmHuntCli"},
                  "--hunt cannot combine with --counter"}});
  std::error_code ignored;
  std::filesystem::remove(path, ignored);
  std::printf("ambient-combination-refusals: ok\n");
}

// ---------------------------------------------------------------------------
// The accepted forms (FR-001, FR-006, D-1)
// ---------------------------------------------------------------------------

auto scenarioAcceptedSelectionForms() -> void
{
  registerHost(kScriptFull);
  writeSelectionFiles();
  registerCase();

  const auto comma =
      captureRun(withFilter({"--hunt=" + address(0) + "," + address(1)}));
  check(countOf(comma, "hunt: plan events=2 ") == 1,
        "a comma list selects the two addresses it names (FR-001)");

  const auto newline =
      captureRun(withFilter({"--hunt=" + address(0) + "\n" + address(1)}));
  check(countOf(newline, "hunt: plan events=2 ") == 1,
        "a newline list selects the two addresses it names (FR-001)");

  const auto glob = captureRun(withFilter({"--hunt=cpu/hunt00?"}));
  check(!planLine(glob).empty(), "one glob pattern plans (FR-001)");

  const auto file =
      captureRun(withFilter({"--hunt=@" + std::string(listFile())}));
  check(countOf(file, "hunt: plan events=3 ") == 1,
        "a @file with comments and blank lines selects its three addresses "
        "(FR-006)");

  const auto dup = captureRun(
      withFilter({"--hunt=@" + std::string(dupFile()) + "," + address(0)}));
  check(countOf(dup, "hunt: plan events=1 ") == 1,
        "the same address across a list and a file keeps one column (FR-006)");
  check(countOf(dup, address(0)) == 1,
        "one column means one row naming that address once (FR-006)");

  const auto rated =
      captureRun(withFilter({"--hunt=" + address(0), "--hunt-rank=rate"}));
  check(!planLine(rated).empty(),
        "a run whose rank option spells the word the default answers plans "
        "the same hunt (FR-032)");
  std::printf("accepted-forms: ok\n");
}

auto scenarioBareHuntSelectsTheCatalog() -> void
{
  registerHost(kScriptFull);
  registerCase();

  const auto run = captureRun(withFilter({"--hunt", "--dry-run"}));
  const std::string plan = planLine(run);
  check(plan.find("events=" + std::to_string(sg::test::kHuntEvents))
            != std::string::npos,
        "a bare --hunt selects the whole core catalog (D-1)");
  check(
      plan.find(sg::test::huntAddress(
          sg::test::kHuntUncore, sg::test::huntEvent(sg::test::kHuntEvents)))
          == std::string::npos,
      "the core catalog the bare option selects holds no uncore address (D-1)");
  std::printf("bare-hunt-whole-catalog: ok\n");
}

auto scenarioListCarriesAGlob() -> void
{
  registerHost(kScriptFull);
  registerCase();

  // One element of a list may carry glob characters beside the commas around
  // it. The parser settles the whole text, so the run sees ten addresses and
  // no pattern to resolve (FR-006).
  const auto run =
      captureRun(withFilter({"--hunt=" + address(0) + ",cpu/hunt00?"}));
  check(countOf(run, "hunt: plan events=10 ") == 1,
        "a list whose element carries a glob character selects the ten "
        "addresses it names, the one the list repeats standing once (FR-006)");
  check(huntLines(run, "hunt: rank ").size() == 10,
        "the resolved selection gives one column per address the text "
        "selected (FR-006)");
  std::printf("list-carries-glob: ok\n");
}

// The two star arms of `globMatches` of `detail/hunt_selection.hpp`: the arm
// that records a star, and the arm that backtracks onto the recorded star once
// a later pattern character mismatches. A pattern of literal text and `?`
// settles on one pass and never opens a star, so the star has to stand in
// front of a leaf character the run reaches by lengthening the star's own run.
// `cpu/*9` is that text: the star opens after `cpu/`, the `9` mismatches the
// first leaf character of every address at once, and each backtrack hands it
// one more character until it stands on the last one (FR-007).
auto scenarioGlobStarArms() -> void
{
  registerHost(kScriptFull);
  registerCase();

  const auto run = captureRun(withFilter({"--hunt=cpu/*9"}));
  check(countOf(run, "hunt: plan events=20 ") == 1,
        "a pattern whose star backtracks selects the twenty scripted events "
        "whose leaf ends in 9 (FR-007)");
  check(huntLines(run, "hunt: rank ").size() == 20,
        "the selection the backtracked star resolved gives one column per "
        "address it reached (FR-007)");
  check(!huntLines(run, address(199)).empty(),
        "the event at the end of the scripted numbering stands in a column of "
        "that selection");
  check(huntLines(run, address(20)).empty(),
        "the event whose leaf ends in 0 stands outside that selection");
  std::printf("glob-star-arms: ok\n");
}

// The refusals a selection read out of a files leaves with, and the refusal of
// a pattern whose star backtracks onto every address and satisfies none. Each
// line names the value in the spelling the option carried, so a selection of
// two files quotes both names inside one pair of quotes (FR-006, FR-007,
// FR-081).
auto scenarioSelectionRefusalShapes() -> void
{
  // The parked-and-blank row below names both files, so the blank fixture
  // stands here too: the child reads a path this process wrote, and the name
  // it lays down carries this process number.
  writeSelectionFiles();
  writeFile(parkedFile(), "# cpu/hunt000 stays parked here\n\n   \n");
  writeFile(upperRawFile(), address(3) + "\n# between the two\nr1F\n");
  checkRefusals({{"full",
                  {"--hunt=cpu/*z"},
                  "--hunt: pattern 'cpu/*z' matches no catalog address"},
                 {"full",
                  {"--hunt=@" + std::string(parkedFile())},
                  holdsNoAddressLine("@" + parkedFile().string())},
                 {"full",
                  {"--hunt=@" + std::string(parkedFile()) + ",@"
                   + std::string(blankFile())},
                  holdsNoAddressLine("@" + parkedFile().string() + ",@"
                                     + blankFile().string())},
                 {"full",
                  {"--hunt=@" + std::string(upperRawFile())},
                  rawCodeLine("r1F")}});
  std::printf("selection-refusal-shapes: ok\n");
}

auto scenarioAllSelectionCount() -> void
{
  registerHost(kScriptFull);
  registerCase();

  // FR-004, I-1: the option parser and the plan answer `all` from one
  // expansion, so the pinned count holds on both sides of the seam.
  sg::hunt::EventCatalog catalog;
  const auto expanded = catalog.expand("all");
  check(expanded.has_value(),
        "the whole selection expands on the scripted host (FR-004)");
  check(catalog.addresses().size() == sg::test::kHuntEvents,
        "--hunt=all yields one event per catalog event of the core devices "
        "(FR-004)");
  const bool anchorsOut = std::none_of(
      catalog.addresses().begin(),
      catalog.addresses().end(),
      [](const std::string& address)
      {
        return address.find(sg::test::kHuntCycle) != std::string::npos
            || address.find(sg::test::kHuntInstr) != std::string::npos;
      });
  check(anchorsOut,
        "no anchor leaf stands among them: the two anchors of a device ride "
        "its passes as denominators (I-1)");

  const auto run = captureRun(withFilter({"--hunt", "--dry-run"}));
  check(countOf(run,
                "hunt: plan events="
                    + std::to_string(catalog.addresses().size()) + " ")
                == 1,
        "the count the option parser validates for --hunt=all is the count the "
        "plan seats, so the two cannot drift (FR-004)");
  std::printf("all-selection-count: ok\n");
}

auto scenarioHuntNoneBehavesLikeNoHunt() -> void
{
  registerHost(kScriptFull);
  registerCase();

  const std::string none = captureRun(withFilter({"--hunt=none"}));
  const std::string plain = captureRun(withFilter({}));
  // The scripted floor samples stand consumed after the first capture, so the
  // second run measures a different floor; masking that one field keeps the
  // byte equality over everything the two runs print for a reason.
  auto stable = [](std::string text)
  {
    const auto at = text.find("overhead floor (ns)=");
    if (at != std::string::npos) {
      text.replace(at + 20, 9, std::string(9, '?'));
    }
    return text;
  };
  check(stable(none) == stable(plain),
        "--hunt=none behaves exactly like a run with no --hunt (spec "
        "Assumptions, FR-001)");
  check(none.find("hunt: ") == std::string::npos,
        "the run with no hunt prints no hunt line at all (FR-052, I-8)");
  std::remove(documentPath().c_str());
  captureRun(
      withFilter({"--hunt=none", "--json=" + std::string(documentPath())}));
  check(readAll(documentPath()).find("\"hunt\"") == std::string::npos,
        "the run with no hunt writes no hunt key (FR-052, I-8)");
  std::printf("hunt-none-like-no-hunt: ok\n");
}

auto scenarioMultiplexCombination() -> void
{
  registerHost(kScriptFull);
  registerCase();
  std::remove(documentPath().c_str());

  const auto run =
      captureRun(withFilter({"--hunt=" + address(0) + "," + address(1),
                             "--metric-method=multiplex",
                             "--json=" + std::string(documentPath())}));
  check(planLine(run).find("MULTIPLEX") != std::string::npos,
        "--hunt with --metric-method=multiplex prints the method word in the "
        "plan line (D-16, D-19)");
  const auto rows = huntLines(run, "hunt: rank ");
  check(!rows.empty(), "the multiplexed hunt prints its rows (D-19)");
  const bool everyRow =
      std::all_of(rows.begin(),
                  rows.end(),
                  [](const std::string& line)
                  { return line.find("multiplexed") != std::string::npos; });
  check(everyRow, "every row of the multiplexed hunt carries the word (D-19)");
  const std::string document = readAll(documentPath());
  check(document.find("MULTIPLEX") != std::string::npos,
        "the document writes the method value MULTIPLEX (D-19)");
  std::printf("multiplex-combination: ok\n");
}

auto scenarioDryRunCombination() -> void
{
  registerHost(kScriptFull);
  registerCase();

  const std::string run = captureRun(
      withFilter({"--hunt=" + address(0) + "," + address(1), "--dry-run"}));
  check(!planLine(run).empty(), "--dry-run with --hunt prints the plan line");
  const auto rows = huntLines(run, "hunt: rank ");
  check(rows.size() == 2, "--dry-run with --hunt prints one row per column");
  check(countOf(run, "iterations=         1") >= 1,
        "every pass of the dry run runs one iteration (contracts/hunt-cli.md)");
  const bool unrepeated =
      std::all_of(rows.begin(),
                  rows.end(),
                  [](const std::string& line)
                  {
                    return line.find("cv=0.00") != std::string::npos
                        && line.find("mark=unrepeated") != std::string::npos;
                  });
  check(unrepeated,
        "every dry-run row is one sample, a cv of 0.0 and unrepeated, which "
        "carries no rank (I-5)");
  std::printf("dry-run-one-row-per-column: ok\n");
}

auto scenarioModesPrintAndStop() -> void
{
  registerHost(kScriptFull);
  registerCase();

  for (const char* mode : {"--list", "--catalog", "--topology"}) {
    const std::string run = captureRun(withFilter({"--hunt", mode}));
    check(run.find("hunt: ") == std::string::npos,
          "--hunt with a print-and-stop mode prints no hunt line (contracts/"
          "hunt-cli.md)");
    check(run.find("iterations=") == std::string::npos,
          "the mode that prints and stops runs no benchmark (contracts/"
          "hunt-cli.md)");
  }
  std::printf("modes-print-and-stop: ok\n");
}

auto scenarioFilterAppliesToTheHunt() -> void
{
  registerHost(kScriptFull);
  registerCase();
  registerCase("bmHuntOther");

  const std::string run =
      captureRun({"--filter", "^bmHuntCli$", "--iterations=1", "--hunt"});
  check(!planLine(run).empty(),
        "--hunt with --filter plans (contracts/" "hunt-cli.md)");
  check(run.find("bmHuntOther") == std::string::npos,
        "the hunt applies to the instances the filter selected and to no other "
        "instance");
  check(countOf(run, "hunt: plan summary") == 1,
        "one filtered instance means one hunt summary (D-16)");
  std::printf("filter-applies: ok\n");
}

auto scenarioRepeatedOptionKeepsLast() -> void
{
  registerHost(kScriptFull);
  registerCase();

  const std::string run =
      captureRun(withFilter({"--hunt=" + address(0), "--hunt=" + address(1)}));
  check(
      countOf(run, address(1)) == 1,
      "a hunt option given twice keeps the last value (contracts/hunt-cli.md)");
  check(run.find(address(0)) == std::string::npos,
        "the first value of a repeated hunt option is gone (contracts/"
        "hunt-cli.md)");

  const std::string norm = captureRun(withFilter(
      {"--hunt=" + address(0), "--hunt-norm=iter", "--hunt-norm=cycle"}));
  check(norm.find("iter=") == std::string::npos
            && norm.find("cycle=") != std::string::npos,
        "a repeated --hunt-norm keeps its last word, the way every other "
        "option in the table behaves");
  std::printf("repeated-option-keeps-last: ok\n");
}

auto scenarioNoLimitHostPlanLine() -> void
{
  registerHost(kScriptNoLimit);
  registerCase();

  const std::string run = captureRun(withFilter({"--hunt=" + address(0)}));
  check(planLine(run).find("MULTIPLEX") != std::string::npos,
        "a device that publishes no limit settles MULTIPLEX on the plan line "
        "too (D-19, spec PC-2)");
  std::printf("no-limit-host: ok\n");
}

// ---------------------------------------------------------------------------
// The position of a hunt option beside `--hunt` (FR-080 to FR-082, D-22)
// ---------------------------------------------------------------------------

// The document one hunt run wrote, which is where every settled option value
// stands once beside the plan it decided.
auto huntDocument(const std::vector<std::string>& arguments) -> std::string
{
  std::remove(documentPath().c_str());
  std::vector<std::string> full = withFilter(arguments);
  full.push_back("--json=" + std::string(documentPath()));
  captureRun(full);
  const std::string text = readAll(documentPath());
  check(text.find("\"hunt\"") != std::string::npos,
        "a hunt run writes the hunt key (FR-050)");
  return text;
}

auto scenarioOptionOrderSettles() -> void
{
  registerHost(kScriptFull);
  registerCase();

  // One value for every hunt option that carries one, in the spelling the
  // option takes. This scenario moves the whole list around `--hunt`, so a
  // value that settles in one order and not the other names the option that
  // lost it.
  const std::vector<std::string> kOptionValues {"--hunt-norm=iter",
                                                "--hunt-rank=cv",
                                                "--hunt-top=3",
                                                "--hunt-noise=25",
                                                "--hunt-repeats=5",
                                                "--hunt-budget=8p",
                                                "--hunt-seed=7"};

  // The document spelling of each value above, in the same order. The settled
  // `hunt` object states the values the run planned with (E-08), and no needle
  // here stands in the document of defaults a discarded value leaves behind.
  const std::vector<std::string> kSettledWords {"\"norm\":\"iter\"",
                                                "\"rank\":\"cv\"",
                                                "\"top\":3",
                                                "\"noisePercent\":25",
                                                "\"repeats\":5",
                                                "\"budget\":\"8p\"",
                                                "\"seed\":7"};

  std::vector<std::string> ahead {kOptionValues.begin(), kOptionValues.end()};
  ahead.push_back("--hunt=" + address(0));
  const std::string parked = huntDocument(ahead);

  std::vector<std::string> behind {"--hunt=" + address(0)};
  behind.insert(behind.end(), kOptionValues.begin(), kOptionValues.end());
  const std::string named = huntDocument(behind);

  check(kOptionValues.size() == kSettledWords.size(),
        "one settled word per hunt option");
  for (std::size_t index = 0; index < kSettledWords.size(); ++index) {
    const std::string word {kSettledWords[index]};
    const bool aheadOfHunt = countOf(parked, word) > 0;
    const bool afterHunt = countOf(named, word) > 0;
    if (!aheadOfHunt || !afterHunt) {
      std::fprintf(stderr,
                   "%s settled ahead=%d behind=%d\n",
                   kOptionValues[index].c_str(),
                   aheadOfHunt,
                   afterHunt);
    }
    check(aheadOfHunt, "an option ahead of --hunt settles (FR-080)");
    check(afterHunt, "the same option behind --hunt settles (FR-080)");
  }
  std::printf("option-order-settles: ok\n");
}

auto scenarioOptionWithoutHunt() -> void
{
  registerHost(kScriptFull);
  registerCase();

  // A hunt option with no `--hunt` keeps the hunt off (contracts/hunt-cli.md):
  // the value parses and stands nowhere the run reads, so the conflict check
  // sees a run with no hunt beside its `--counter`.
  const std::string run = captureRun(
      withFilter({"--hunt-budget=8p", "--counter=cpu/instructions"}));
  check(run.find("hunt: ") == std::string::npos,
        "a hunt option with no --hunt runs no hunt (FR-001)");

  std::remove(documentPath().c_str());
  captureRun(withFilter({"--hunt-norm=iter",
                         "--hunt-seed=7",
                         "--json=" + std::string(documentPath())}));
  check(readAll(documentPath()).find("\"hunt\"") == std::string::npos,
        "a hunt option with no --hunt writes no hunt key (FR-052)");
  std::printf("option-without-hunt: ok\n");
}

auto scenarioOptionOrderRefusals() -> void
{
  // A value outside its range refuses at the option that read it, whether
  // `--hunt` stood before it or after it: the scan defers no range check, and
  // the refusal line names the value in the spelling the option carried.
  checkRefusals({{"full",
                  {"--hunt-budget=0p", "--hunt=all"},
                  "--hunt-budget: expected a count above 0, got '0p'"},
                 {"full",
                  {"--hunt-top=0", "--hunt"},
                  "--hunt-top: expected 1 to 10000, got '0'"},
                 {"full",
                  {"--hunt=cpu/nope", "--hunt-norm=iter"},
                  "--hunt: 'cpu/nope' is not a catalog address"}});
  std::printf("option-order-refusals: ok\n");
}

// ---------------------------------------------------------------------------
// Where the plan line stands beside the rows it bought (T074, US2 scenario 1,
// FR-012, FR-013)
// ---------------------------------------------------------------------------

// The line a needle first stands on, counted from the first line of the
// report, or `npos` when no line holds it. Where the report puts a line is
// what the two cases below weigh, and a line position states it in the bytes
// of the report itself, where a reader can see it.
auto lineIndex(const std::string& text,
               const std::string& needle) -> std::size_t
{
  const std::size_t hit = text.find(needle);
  if (hit == std::string::npos) {
    return std::string::npos;
  }
  std::size_t index = 0;
  for (std::size_t at = 0; at < hit; ++at) {
    if (text[at] == '\n') {
      ++index;
    }
  }
  return index;
}

// The lines of one capture, split at newlines, the shape the ordering checks
// walk.
auto splitLines(const std::string& text) -> std::vector<std::string>
{
  auto lines = std::vector<std::string> {};
  auto stream = std::istringstream(text);
  for (auto line = std::string {}; std::getline(stream, line);) {
    lines.push_back(line);
  }
  return lines;
}

// The line the `wanted`th occurrence of a line prefix first stands on, the
// count of lines when the capture holds fewer. The row of an instance starts
// with its name padded to the column width, so the name with two spaces
// reaches the row alone.
auto lineStartIndex(const std::vector<std::string>& lines,
                    const std::string& needle,
                    std::size_t wanted) -> std::size_t
{
  for (std::size_t index = 0; index < lines.size(); ++index) {
    if (lines[index].starts_with(needle) && wanted-- == 0) {
      return index;
    }
  }
  return lines.size();
}

// The lines that rank a hunt stand behind the rows they measured, but the plan
// line says what the run set out to measure, so it stands above the first row:
// the reader holds the budget before the figures it explains (FR-012).
auto scenarioPlanLineStandsAboveTheRows() -> void
{
  registerHost(kScriptFull);
  registerCase();

  const std::string run = captureRun(withFilter({"--hunt=all"}));
  const std::size_t plan = lineIndex(run, "hunt: plan events=");
  const std::size_t row = lineIndex(run, "iterations=");
  check(countOf(run, "hunt: plan events=") == 1,
        "the run prints the plan line once (FR-012)");
  check(lineHolding(run, "iterations=").rfind("bmHuntCli", 0) == 0,
        "the line weighed as the first row is the row of this benchmark");
  check(plan < row, "the plan line stands above the first result row (FR-012)");
  check(plan + 1 == row,
        "no line of the report stands between the plan line and the row");
  check(row < lineIndex(run, "hunt: rank "),
        "the lines that rank still stand behind the rows (FR-040)");
  check(lineIndex(run, "hunt: plan budget cut") == std::string::npos,
        "a plan with no budget prints no cut line (FR-013)");
  std::printf("plan-line-above-rows: ok\n");
}

// A budget too small to hold stage 2 drops it whole and says so on the line
// directly under the plan line, which stands above the rows as well (E-07).
auto scenarioPlanLineUnderABudgetCut() -> void
{
  registerHost(kScriptFull);
  registerCase();

  const std::string run =
      captureRun(withFilter({"--hunt=all", "--hunt-budget=1p"}));
  const std::size_t plan = lineIndex(run, "hunt: plan events=");
  const std::size_t cut = lineIndex(run, "hunt: plan budget cut");
  const std::size_t row = lineIndex(run, "iterations=");
  check(cut != std::string::npos,
        "a budget that cannot hold stage 2 prints the cut line (FR-013)");
  check(lineHolding(run, "iterations=").rfind("bmHuntCli", 0) == 0,
        "the line weighed as the first row is the row of this benchmark");
  check(cut == plan + 1,
        "the cut line stands directly under the plan line (FR-013)");
  check(cut < row,
        "the plan line and the cut line under it stand above the first result "
        "row (FR-012, US2 scenario 1)");
  std::printf("plan-line-budget-cut: ok\n");
}

// The bytes one print of the console writes, captured while stdout points at
// the scratch file - the redirection seam of `captureRun` around a call that
// needs no run.
template<typename Print>
[[nodiscard]] auto capturePrint(const Print& print) -> std::string
{
  const int saved = dup(STDOUT_FILENO);
  if (saved < 0 || std::freopen(scratchPath().c_str(), "w", stdout) == nullptr)
  {
    fail("cannot redirect the plan line");
  }
  print();
  std::fflush(stdout);
  if (dup2(saved, STDOUT_FILENO) < 0) {
    fail("cannot restore stdout");
  }
  close(saved);
  std::ifstream file(scratchPath());
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// The reason of the cut line is the value the planner recorded for the drop
// (`RunHunt::cutReason`, the `StageSplit::cutReason` of the split that dropped
// the stage), and not a word the console chooses for the shape it sees
// (FR-013, data-model E-07). The case builds the record, and no run does: the
// case weighs which source the line reads, and it carries a reason
// no shape of a record could answer, so a console that derived the word from
// the pair of a budget and one stage prints something else. A run with no
// budget prints no cut line, which `plan-line-above-rows` weighs on real
// bytes.
auto scenarioCutLinePrintsTheRecordedReason() -> void
{
  const auto record = sg::hunt::RunHunt {
      .selection = "all",
      .options =
          sg::hunt::HuntOptions {
              .selection = "all",
              .norm = std::nullopt,
              .rank = sg::hunt::HuntRank::RATE,
              .top = 20,
              .noise = 10.0,
              .repeats = 3,
              .seed = 0,
              .budget =
                  sg::hunt::HuntBudget {
                      .kind = sg::hunt::HuntBudget::Kind::PASSES,
                      .amount = 1,
                  },
          },
      .passes = 1,
      .method = std::nullopt,
      .estimateNs = std::nullopt,
      .plannedAddresses = {},
      .stages = {sg::hunt::HuntStageRange {
          .first = 0, .count = 4, .repeats = 1}},
      .ranEvents = 4,
      .stageTwoEvents = 0,
      .cutReason = "planner-recorded",
      .sample = {},
      .muxGroups = {},
  };

  const std::string lines =
      capturePrint([&record] { sg::detail::printHuntPlan(record); });
  check(countOf(lines, "hunt: plan budget cut") == 1,
        "a budgeted record that holds one stage prints the cut line once "
        "(FR-013)");
  check(lineHolding(lines, "hunt: plan budget cut")
            .find("reason=planner-recorded")
        != std::string::npos,
        "the cut line prints the reason the record carries, not a word the "
        "console chooses for the shape it sees (FR-013, E-07)");
  check(lineHolding(lines, "hunt: plan events=").find(" budget=1p")
            != std::string::npos,
        "the plan line above the cut line still spells the budget it was "
        "given (FR-012)");
  std::printf("cut-line-recorded-reason: ok\n");
}

// ---------------------------------------------------------------------------
// The estimate the plan line weighs the passes against (T119, FR-012;
// T120, FR-040)
// ---------------------------------------------------------------------------

// The JSON number that stands behind one key, scanned the way
// `holdOutOverheadFloor` of `harness_json_report_test.cpp` scans it, or the
// empty text when the document holds no such key.
auto numberAfter(const std::string& text,
                 const std::string_view key) -> std::string
{
  const auto at = text.find(key);
  if (at == std::string::npos) {
    return {};
  }
  const auto start = at + key.size();
  auto stop = start;
  while (stop < text.size()
         && ((text[stop] >= '0' && text[stop] <= '9') || text[stop] == '.'
             || text[stop] == '-' || text[stop] == '+' || text[stop] == 'e'
             || text[stop] == 'E'))
  {
    ++stop;
  }
  return text.substr(start, stop - start);
}

// The estimate of the plan line is the nanoseconds of one pass times the
// passes and the repeats the plan runs (`hunt.hpp`), and the hunt times that
// pass at its own entry (T119, FR-012). The pass figure behind the estimate
// and the overhead floor the row publishes are two readings of one host, and
// ThreadSanitizer answered a pass reading below a floor reading, so no order
// between them belongs to this code and this case claims none. What the case
// pins is what the planner owes whatever the host answers: the plan line
// carries an estimate, and that estimate is one figure of a pass weighed
// against the passes of the plan, so it divides by them whole. The signature
// of the T119 defect, an estimate that is a copy of the floor, is weighed
// where the figure is handed to the planner and no clock runs:
// `estimate-figures-the-passes` of `harness_hunt_collect_test.cpp`. A hunt
// that measures no pass leaves the estimate reading `not available` (FR-012).
auto scenarioPlanWeighsTheMeasuredPass() -> void
{
  registerHost(kScriptFull);
  registerCase();

  std::remove(documentPath().c_str());
  const std::string run =
      captureRun(withFilter({"--hunt=" + address(0),
                             "--hunt-repeats=1",
                             "--json=" + std::string(documentPath())}));
  const std::string plan = planLine(run);
  const std::string floor =
      numberAfter(readAll(documentPath()), "\"overheadFloorNs\":");
  check(!plan.empty(), "the hunt of one address prints its plan line");
  check(!floor.empty(), "the document carries the floor of the finished row");

  const auto passes = std::stoull(numberAfter(plan, "passes="));
  const auto estimate = std::stoull(numberAfter(plan, "estimate="));
  check(estimate > 0,
        "the plan line carries the figure of the pass the hunt measured (T119, "
        "FR-012)");
  check(estimate % passes == 0,
        "the estimate weighs the passes of the plan against one figure of a "
        "pass (FR-012)");
  std::remove(documentPath().c_str());
  std::printf("measured-pass-estimate: ok\n");
}

// ---------------------------------------------------------------------------
// What one hunt run prints whole, on one context and on several (T074, T114,
// FR-012, FR-025, FR-022)
// ---------------------------------------------------------------------------

// A hunt run on one context answers with the plan line, the rows, and the
// lines that rank: the context tail names a threaded result only, so a
// single-context run keeps the bytes it printed before the contexts existed
// (FR-025, FR-022). The case below is the guard against moving the hunt block
// behind that single-context return, which would leave the plan line standing
// alone with nothing that ranks below it.
auto scenarioSingleContextHuntPrintsTheHuntWhole() -> void
{
  registerHost(kScriptFull);
  registerCase();

  const std::string run = captureRun(withFilter({"--hunt=all"}));
  const std::size_t plan = lineIndex(run, "hunt: plan events=");
  const std::size_t row = lineIndex(run, "iterations=");
  const std::size_t rank = lineIndex(run, "hunt: rank ");
  check(plan < row, "the plan line stands above the first result row (FR-012)");
  check(rank != std::string::npos,
        "a single-context hunt run prints the lines that rank (FR-040)");
  check(countOf(run, "hunt: ") >= 2,
        "a single-context hunt run prints the whole hunt block, not the plan "
        "line alone (FR-040, FR-041)");
  check(row < rank, "the hunt lines stand below the rows they rank (FR-040)");
  check(lineIndex(run, "contexts=") == std::string::npos,
        "a single-context hunt run prints no context tail (FR-025)");
  std::printf("single-context-hunt: ok\n");
}

// The threaded case of the scenario below, and the filter that picks it out of
// the command line `withFilter` builds (the last `--filter` settles).
constexpr const char* kThreadedCase = "bmHuntCliThreads";
constexpr const char* kThreadedFilter = "bmHuntCliThreads";

// The one run where a context tail does print is a threaded hunt: the plan
// line, the lines that rank and the tail naming the contexts all stand in the
// same report, above and below the rows in that order (FR-012, FR-025).
auto scenarioThreadedHuntPrintsTheContextTail() -> void
{
  registerHost(kScriptFull);
  registerCase(kThreadedCase).threads(2);

  const std::string run = captureRun(withFilter({"--filter",
                                                 kThreadedFilter,
                                                 "--hunt=all",
                                                 "--hunt-rank=cv",
                                                 "--hunt-top=2"}));
  const std::size_t plan = lineIndex(run, "hunt: plan events=");
  const std::size_t row = lineIndex(run, "iterations=");
  const std::size_t tail = lineIndex(run, "contexts=");
  check(plan != std::string::npos,
        "a threaded hunt run prints the plan line (FR-012)");
  check(row != std::string::npos, "a threaded hunt run prints its rows");
  check(countOf(run, "hunt: ") >= 2,
        "a threaded hunt run prints the whole hunt block (FR-040, FR-041)");
  check(tail != std::string::npos,
        "a threaded hunt run prints the context tail (FR-025)");
  check(plan < row, "the plan line stands above the first result row (FR-012)");
  check(row < tail, "the context tail stands below the rows it names (FR-025)");
  std::printf("threaded-hunt-context-tail: ok\n");
}

// ---------------------------------------------------------------------------
// The scenario list CTest runs
// ---------------------------------------------------------------------------

struct Scenario
{
  std::string_view name;
  void (*body)();
};

// The scenarios of this file: what CTest runs with no argument, and what
// `argv[1]` selects from (T018, US1).
// ---------------------------------------------------------------------------
// One filter, two instances, the record of each hunt riding its own result
// (T126, FR-040, FR-050, PD-10)
// ---------------------------------------------------------------------------

// The filter selects two instances and `--hunt` hunts both. Each instance
// prints its own plan line above its own row and its own lines that rank
// below it, and the document carries the two entry nodes beside one
// run-level node: every hunt block reads the record its own hunt answered,
// the fact a process-wide record could only hold as long as the loop kept
// printing in the order it hunted (T126).
auto scenarioFilterSelectsTwoHuntedInstances() -> void
{
  registerHost(kScriptFull);
  registerCase("bmHuntCliPair");
  registerCase("bmHuntCliPairTwo");

  const std::string run = captureRun({"--filter",
                                      "bmHuntCliPair",
                                      "--iterations=1",
                                      "--hunt=" + address(0) + "," + address(1),
                                      "--json=" + std::string(documentPath())});
  const auto lines = splitLines(run);
  const std::size_t firstPlan = lineStartIndex(lines, "hunt: plan events=", 0);
  const std::size_t firstRow = lineStartIndex(lines, "bmHuntCliPair  ", 0);
  const std::size_t firstRank = lineStartIndex(lines, "hunt: rank 1 ", 0);
  const std::size_t secondPlan = lineStartIndex(lines, "hunt: plan events=", 1);
  const std::size_t secondRow = lineStartIndex(lines, "bmHuntCliPairTwo  ", 0);
  const std::size_t secondRank = lineStartIndex(lines, "hunt: rank 1 ", 1);
  check(firstPlan < firstRow && firstRow < firstRank
            && firstRank < secondPlan && secondPlan < secondRow
            && secondRow < secondRank,
        "each instance's plan line stands above its own row and its ranks "
        "below, so no block reads the record another hunt answered (FR-040, "
        "T126)");
  check(countOf(run, "hunt: plan summary events=") == 2,
        "both hunt blocks close with their own summary (FR-040)");
  const std::string document = readAll(documentPath());
  check(countOf(document, "\"hunt\":{") == 3,
        "the document carries one hunt node per hunted result and one "
        "run-level node for the plan both instances hunted under (FR-050, "
        "PD-10)");
  std::printf("filter-selects-two-hunted: ok\n");
}

// ---------------------------------------------------------------------------
// The shared `@`-file reader (specs/033-hunter-budgets T002; FR-014, R-09,
// data-model N-07). Both routes read one grammar through
// `detail::readAddressFileLines`: the hunt selection of the rows above, and the
// `--counter` `@`-form of the round trip below. The scenario calls the reader
// directly, so spec 029's separator rule stands pinned at the one place it now
// lives, and the refusal rows above keep pinning the sentence the hunt route
// prints over it.
// ---------------------------------------------------------------------------

auto readerFile(const char* kind,
                const std::string& text) -> std::filesystem::path
{
  const auto path = capturePath(kind);
  writeFile(path, text);
  return path;
}

// The elements of one answer, spelled out for the failure line.
auto elementText(const std::vector<std::string>& elements) -> std::string
{
  std::string text;
  for (const auto& element : elements) {
    if (!text.empty()) {
      text += "|";
    }
    text += element.empty() ? "<empty>" : element;
  }
  return text;
}

auto scenarioSharedReaderRules() -> void
{
  // Spec 029's separator rule, stated once for both routes: each line cuts at
  // the first `#`, a blank or whitespace-only line answers nothing, one line of
  // two comma-separated addresses answers two elements, and a comma beside
  // nothing answers the empty element each caller refuses in its own sentence.
  const auto rules =
      readerFile("_rules.txt",
                 "# a header\n" + address(0) + "\n\n   \n" + address(1) + ","
                     + address(2) + " # tail\n," + address(3) + "\n");
  const auto lines = sg::detail::readAddressFileLines(rules, "@rules.txt");
  check(lines.has_value(), "the shared reader opens the file it is handed");
  const std::string expected = address(0) + "|" + address(1) + "|" + address(2)
      + "|<empty>|" + address(3);
  if (elementText(*lines) != expected) {
    std::fprintf(stderr,
                 "reader answered (%zu): %s\nexpected: %s\n",
                 lines->size(),
                 elementText(*lines).c_str(),
                 expected.c_str());
  }
  check(elementText(*lines) == expected,
        "the reader answers the file's address elements under the separator "
        "rule (FR-006, FR-014)");

  // One level stands: a line that names another `@path` is an element, and the
  // file behind it stays unread.
  const auto inner = readerFile("_nested_inner.txt", address(9) + "\n");
  const auto outer =
      readerFile("_nested_outer.txt", "@" + inner.string() + "\n");
  const auto nested =
      sg::detail::readAddressFileLines(outer, "@" + outer.string());
  check(nested.has_value(), "the shared reader opens the outer file");
  check(nested->size() == 1 && (*nested)[0] == "@" + inner.string(),
        "one level stands: a nested `@path` answers its own text, never the "
        "file behind it (029 D-14, FR-014)");

  // The reader's own refusal names the path in the spelling the caller gave it,
  // in one line; each route prefixes its own option word.
  const auto missing = sg::detail::readAddressFileLines(
      "/no/such/counter-set.txt", "@/no/such/counter-set.txt");
  check(!missing.has_value(),
        "an unopenable file leaves the reader no element list (FR-012)");
  if (missing.has_value()) {
    return;
  }
  if (missing.error() != "'@/no/such/counter-set.txt' cannot be opened") {
    std::fprintf(stderr, "the refusal reads: %s\n", missing.error().c_str());
  }
  check(missing.error() == "'@/no/such/counter-set.txt' cannot be opened",
        "the reader refuses in one line naming the path it could not open "
        "(FR-014, N-07)");

  // A file that holds nothing answers the empty list, the shape the emit row
  // writes and both routes refuse in their own sentence.
  const auto blank = readerFile("_empty.txt", "");
  const auto none = sg::detail::readAddressFileLines(blank, "@empty.txt");
  check(none.has_value() && none->empty(),
        "a file that holds no line answers no element (FR-011, FR-014)");

  std::printf("shared-reader-rules: ok\n");
}

// ---------------------------------------------------------------------------
// The emitted counter set (specs/033-hunter-budgets T006; FR-010 to FR-012,
// SC-004, R-07, R-08; contracts/counter-file.md). The file the run writes is
// the round trip's half: one catalog address per line, best first, no header.
// The ranked order is read back off the run's own rank lines, so the file is
// compared against what the same run ranked and no fixture constant is
// repeated.
// ---------------------------------------------------------------------------

auto emitPath() -> std::filesystem::path
{
  return capturePath("_emit.txt");
}

// The addresses of the run's ranked lines, in the order the report printed
// them. An unranked line (`hunt: rank -`) names no culprit and stands skipped.
auto rankedAddresses(const std::string& text) -> std::vector<std::string>
{
  std::vector<std::string> addresses;
  for (const auto& line : huntLines(text, "rank ")) {
    std::istringstream words(line);
    std::string option;
    std::string key;
    std::string rank;
    std::string address;
    if (words >> option >> key >> rank >> address && rank != "-") {
      addresses.push_back(address);
    }
  }
  return addresses;
}

// The lines of the emitted file, the trailing newline of the last line left
// out of the answer.
auto fileLines(const std::filesystem::path& path) -> std::vector<std::string>
{
  std::vector<std::string> lines;
  const std::string text = readAll(path);
  std::istringstream stream(text);
  std::string line;
  while (std::getline(stream, line)) {
    lines.push_back(line);
  }
  return lines;
}

auto joined(const std::vector<std::string>& lines) -> std::string
{
  std::string text;
  for (const auto& line : lines) {
    if (!text.empty()) {
      text += "|";
    }
    text += line;
  }
  return text;
}

auto scenarioEmitSetHoldsTheRankedSet() -> void
{
  registerHost(kScriptFull);
  registerCase();
  const std::string selection =
      address(0) + "," + address(1) + "," + address(2) + "," + address(3);
  const auto run = captureRun({"--iterations=1",
                               "--hunt=" + selection,
                               "--hunt-top=3",
                               "--hunt-emit-set=" + emitPath().string()});
  // The ranking itself stops at `--hunt-top`: the fourth event leaves the table
  // `beyond-top`, so the ranked list and the emitted list carry the same three
  // addresses and the cap of the call is the one T005 pins at the record.
  const auto ranked = rankedAddresses(run);
  check(ranked.size() == 3,
        "the scripted hunt ranks its top three and leaves the fourth beyond "
        "the top");
  const std::vector<std::string> wanted(ranked.begin(), ranked.end());
  const auto written = fileLines(emitPath());
  if (joined(written) != joined(wanted)) {
    std::fprintf(stderr,
                 "the file holds (%zu): %s\nthe top three rank: %s\n",
                 written.size(),
                 joined(written).c_str(),
                 joined(wanted).c_str());
  }
  check(joined(written) == joined(wanted),
        "the emitted file holds the top `--hunt-top` addresses, one per line, "
        "best first and no header (FR-011, SC-004)");
  std::printf("emit-set-holds-the-ranked-set: ok\n");
}

auto scenarioEmitSetUnionsTwoEntries() -> void
{
  registerHost(kScriptFull);
  registerCase("bmHuntCliPair");
  registerCase("bmHuntCliPairTwo");
  const std::string selection = address(0) + "," + address(1);
  const auto run = captureRun({"--filter",
                               "bmHuntCliPair",
                               "--iterations=1",
                               "--hunt=" + selection,
                               "--hunt-top=2",
                               "--hunt-emit-set=" + emitPath().string()});
  check(countOf(run, "hunt: plan events=") == 2,
        "the filter hunts both entries, so two entries emit");
  const auto written = fileLines(emitPath());
  // The scripted host ranks the two entries alike, so the union's work here is
  // the deduplication: two entries that each emit two addresses write two lines
  // and no address twice.
  check(written.size() == 2,
        "two entries that rank alike write each address once (FR-011, R-07)");
  check(countOf(joined(written), address(0)) == 1
            && countOf(joined(written), address(1)) == 1,
        "the union holds both addresses of the ranked pair (FR-011)");
  std::printf("emit-set-unions-two-entries: ok\n");
}

auto scenarioEmitSetStandsEmptyWithoutRanks() -> void
{
  registerHost(kScriptFull);
  registerCase();
  // One repeat leaves every event unrepeated, and an unrepeated event carries
  // no rank (hunt.hpp, I-5): the run ranks nothing, so the file stands written
  // and empty, and the run keeps the exit status of the hunt that ran.
  const auto run = captureRun({"--iterations=1",
                               "--hunt=" + address(0) + "," + address(1),
                               "--hunt-repeats=1",
                               "--hunt-emit-set=" + emitPath().string()});
  check(countOf(run, "mark=unrepeated") == 2,
        "the scripted run leaves every event unranked");
  check(std::filesystem::exists(emitPath()),
        "a hunt that ranks nothing still writes the file it named "
        "(FR-011, SC-004)");
  check(fileLines(emitPath()).empty(),
        "the file of a hunt that ranks nothing holds no line (FR-011)");
  std::printf("emit-set-stands-empty-without-ranks: ok\n");
}

auto scenarioEmitSetRefusals() -> void
{
  const std::string wanted =
      "--hunt-emit-set: '" + emitPath().string() + "' needs --hunt";
  checkRefusals({
      {"full", {"--hunt-emit-set=" + emitPath().string()}, wanted},
      {"full",
       {"--hunt=none", "--hunt-emit-set=" + emitPath().string()},
       wanted},
      {"full",
       {"--hunt=" + address(0), "--hunt-emit-set="},
       "--hunt-emit-set: the path the option names stands empty"},
  });
  std::printf("emit-set-refusals: ok\n");
}

auto scenarioEmitSetRefusesAnUnwritableDestination() -> void
{
  registerHost(kScriptFull);
  registerCase();
  const std::string destination = "/no/such/directory/culprits.txt";
  std::string output;
  const int status = childRun(selfPath,
                              "hunt",
                              {"full",
                               "--iterations=1",
                               "--hunt=" + address(0),
                               "--hunt-emit-set=" + destination},
                              output);
  check(WEXITSTATUS(status) == 2,
        "an emit the file system refused leaves the usage status (FR-012)");
  const std::string heading =
      "error: the counter set file '" + destination + "' cannot be opened: ";
  const std::string line = lineHolding(output, heading);
  if (line.rfind(heading, 0) != 0 || line.size() == heading.size()) {
    std::fprintf(stderr, "the failed write reads: %s\n", output.c_str());
  }
  check(line.rfind(heading, 0) == 0 && line.size() > heading.size(),
        "the failed write names the path and the reason it got, on one line, "
        "the shape the report file names (FR-012)");
  check(countOf(output, "BENCHMARK_RAN") == 1,
        "the benchmark ran, because the failure stands after the batch "
        "(FR-012)");
  std::printf("emit-set-refuses-an-unwritable-destination: ok\n");
}

// ---------------------------------------------------------------------------
// The `@`-file form of `--counter` (specs/033-hunter-budgets T007; FR-014,
// R-09; contracts/counter-file.md). The addresses the file carries reach the
// same gather the command line feeds, so the columns, the resolution and the
// refusals of a typed address stand unchanged behind a file name.
// ---------------------------------------------------------------------------

auto counterSetPath() -> std::filesystem::path
{
  return capturePath("_set.txt");
}

// The counter columns of the run's row, counted by the label each column
// prints.
auto columnCount(const std::string& text) -> std::size_t
{
  std::size_t found = 0;
  for (std::size_t index = 0; index < sg::test::kHuntEvents; ++index) {
    found += countOf(text, address(index) + "=");
  }
  return found;
}

auto scenarioCounterFileFormReadsAddresses() -> void
{
  registerHost(kScriptFull);
  registerCase();

  // Twenty addresses, one per line, and one line of two comma-separated
  // addresses: twenty-one columns. A comment and a blank line answer nothing,
  // the separator rule of spec 029 the shared reader carries.
  std::string text = "# the set a hunt emitted\n";
  for (std::size_t index = 0; index < 20; ++index) {
    text += address(index) + "\n";
  }
  text += "\n   \n" + address(20) + "," + address(21) + " # tail\n";
  writeFile(counterSetPath(), text);

  const auto run =
      captureRun({"--iterations=1", "--counter=@" + counterSetPath().string()});
  check(columnCount(run) == 22,
        "the file's address elements answer one counter column each, with no "
        "count cap on the file form (FR-014)");
  check(countOf(run, address(21) + "=") == 1,
        "the two addresses of one comma-separated line both reach a column "
        "(FR-014, R-09)");
  check(countOf(run, "no such leaf") == 0,
        "no address of the file leaves a column the host cannot resolve "
        "(FR-014)");
  std::printf("counter-file-form-reads-addresses: ok\n");
}

auto scenarioCounterFileKeepsTheStandingResolution() -> void
{
  // An address the host has no leaf for keeps the standing `--counter` shape
  // whether the file or the command line spelled it: the entry point resolves
  // both lists the same way, so the file form changes nothing about resolution.
  const std::string unknown = "cpu/nope";
  writeFile(counterSetPath(), address(0) + "\n" + unknown + "\n");
  std::string typed;
  const int typedStatus =
      childRun(selfPath, "hunt", {"full", "--counter=" + unknown}, typed);
  std::string read;
  const int readStatus = childRun(
      selfPath,
      "hunt",
      {"full", "--iterations=1", "--counter=@" + counterSetPath().string()},
      read);
  check(WEXITSTATUS(typedStatus) == 2 && WEXITSTATUS(readStatus) == 2,
        "the unknown address leaves the usage status from either spelling "
        "(FR-014)");
  check(lineHolding(read, unknown) == lineHolding(typed, unknown),
        "the file's unknown address answers the line the typed spelling "
        "answers (FR-014, R-09)");
  std::printf("counter-file-keeps-the-standing-resolution: ok\n");
}

auto scenarioCounterFileRefusals() -> void
{
  const std::string emptyAddress = capturePath("_empty_address.txt").string();
  writeFile(emptyAddress, address(0) + ",\n," + address(1) + "\n");
  const std::string comments = capturePath("_comments.txt").string();
  writeFile(comments, "# nothing but a comment\n\n");
  checkRefusals({
      {"full",
       {"--counter=@/no/such/culprits.txt"},
       "--counter: '@/no/such/culprits.txt' cannot be opened"},
      {"full",
       {"--counter=@" + emptyAddress},
       "--counter: '' is not a counter address"},
      {"full",
       {"--counter=@" + comments},
       "--counter: '@" + comments + "' holds no counter address"},
  });
  std::printf("counter-file-refusals: ok\n");
}

auto scenarioCounterFileFromEnvironmentAndOptionFile() -> void
{
  registerHost(kScriptFull);
  registerCase();

  // The environment splits its `;` items first, so one variable reads one file
  // and keeps a literal address beside it, each item carrying the grammar the
  // command line carries (FR-024, FR-014).
  writeFile(counterSetPath(), address(0) + "\n");
  std::string output;
  const int status = childRun(selfPath,
                              "hunt",
                              {"full", "--iterations=1"},
                              output,
                              "SPEEDGUN_COUNTER=\"@" + counterSetPath().string()
                                  + ";" + address(1) + "\" ");
  check(WEXITSTATUS(status) == 0,
        "the environment settles the counter row the way the command line "
        "does (FR-024, FR-014)");
  check(countOf(output, address(0) + "=") == 1
            && countOf(output, address(1) + "=") == 1,
        "one item reads the file and the other keeps its literal address "
        "(FR-014, FR-024)");

  // The option file source reads the same grammar, and its fault names the
  // file and the line the way every option-file fault names them (FR-027,
  // FR-031, FR-035).
  const auto counterOptions = capturePath("_counter_options.txt");
  writeFile(counterOptions, "counter=@" + counterSetPath().string() + "\n");
  std::string read;
  const int readStatus = childRun(
      selfPath,
      "hunt",
      {"full", "--iterations=1", "--options=" + counterOptions.string()},
      read);
  check(WEXITSTATUS(readStatus) == 0
            && countOf(read, address(0) + "=") == 1,
        "an option file line reads the `@`-form the command line reads "
        "(FR-014, FR-035)");

  const auto missing = capturePath("_missing_options.txt");
  writeFile(missing, "counter=@/no/such/culprits.txt\n");
  checkRefusals({
      {"full",
       {"--options=" + missing.string()},
       missing.string()
           + ":1: --counter: '@/no/such/culprits.txt' cannot be opened"},
  });
  std::printf("counter-file-from-environment-and-option-file: ok\n");
}

// ---------------------------------------------------------------------------
// The round trip (specs/033-hunter-budgets T012; FR-015, SC-001, roadmap
// criterion 12.7-6). One scripted run hunts and emits, the second reads the
// emitted file back as its counter list. No clock, no sleep, no PMU hardware.
// ---------------------------------------------------------------------------

auto scenarioEmitSetRoundTrip() -> void
{
  registerHost(kScriptFull);
  registerCase();
  std::string selection;
  for (std::size_t index = 0; index < 6; ++index) {
    if (index != 0) {
      selection += ",";
    }
    selection += address(index);
  }
  const auto run = captureRun({"--iterations=1",
                               "--hunt=" + selection,
                               "--hunt-top=6",
                               "--hunt-emit-set=" + counterSetPath().string()});
  const auto emitted = fileLines(counterSetPath());
  check(emitted.size() == 6,
        "the hunt wrote every ranked address it was asked to emit (FR-011)");
  check(countOf(run, "no such leaf") == 0,
        "the hunt itself resolved every address (FR-011)");

  const auto readBack =
      captureRun({"--iterations=1", "--counter=@" + counterSetPath().string()});
  check(countOf(readBack, address(5) + "=") == 1,
        "the second run reads the emitted file back as its counter list "
        "(FR-015, criterion 12.7-6)");
  check(columnCount(readBack) == emitted.size(),
        "the re-read run reports one counter column per emitted address "
        "(FR-015, SC-001)");
  check(countOf(readBack, "no such leaf") == 0,
        "no emitted column stands refused on the host that ranked it "
        "(FR-015, criterion 12.7-6)");
  std::printf("emit-set-round-trip: ok\n");
}

[[nodiscard]] auto scenarios() -> const std::vector<Scenario>&
{
  static const std::vector<Scenario> kList {
      Scenario {"selection-refusals", &scenarioSelectionRefusals},
      Scenario {"empty-attached-value", &scenarioEmptyAttachedValue},
      Scenario {"option-value-refusals", &scenarioOptionValueRefusals},
      Scenario {"budget-clause-refusals", &scenarioBudgetClauseRefusals},
      Scenario {"budget-clause-accepted", &scenarioBudgetClauseAccepted},
      Scenario {"limit-refusal", &scenarioLimitRefusal},
      Scenario {"anchor-leaf-needs-no-slot", &scenarioAnchorLeafNeedsNoSlot},
      Scenario {"nested-device-limit-refusal",
                &scenarioNestedDeviceLimitRefusal},
      Scenario {"seat-refusal-status", &scenarioSeatRefusalExitsOneWithReason},
      Scenario {"norm-anchor-refusals", &scenarioNormAnchorRefusals},
      Scenario {"conflict-refusals", &scenarioConflictRefusals},
      Scenario {"accepted-forms", &scenarioAcceptedSelectionForms},
      Scenario {"bare-hunt-whole-catalog", &scenarioBareHuntSelectsTheCatalog},
      Scenario {"all-selection-count", &scenarioAllSelectionCount},
      Scenario {"list-carries-glob", &scenarioListCarriesAGlob},
      Scenario {"glob-star-arms", &scenarioGlobStarArms},
      Scenario {"selection-refusal-shapes", &scenarioSelectionRefusalShapes},
      Scenario {"hunt-none-like-no-hunt", &scenarioHuntNoneBehavesLikeNoHunt},
      Scenario {"multiplex-combination", &scenarioMultiplexCombination},
      Scenario {"dry-run-one-row-per-column", &scenarioDryRunCombination},
      Scenario {"modes-print-and-stop", &scenarioModesPrintAndStop},
      Scenario {"filter-applies", &scenarioFilterAppliesToTheHunt},
      Scenario {"repeated-option-keeps-last", &scenarioRepeatedOptionKeepsLast},
      Scenario {"no-limit-host", &scenarioNoLimitHostPlanLine},
      Scenario {"plan-line-above-rows", &scenarioPlanLineStandsAboveTheRows},
      Scenario {"plan-line-budget-cut", &scenarioPlanLineUnderABudgetCut},
      Scenario {"cut-line-recorded-reason",
                &scenarioCutLinePrintsTheRecordedReason},
      Scenario {"measured-pass-estimate", &scenarioPlanWeighsTheMeasuredPass},
      Scenario {"single-context-hunt",
                &scenarioSingleContextHuntPrintsTheHuntWhole},
      Scenario {"threaded-hunt-context-tail",
                &scenarioThreadedHuntPrintsTheContextTail},
      Scenario {"filter-selects-two-hunted",
                &scenarioFilterSelectsTwoHuntedInstances},
      Scenario {"shared-reader-rules", &scenarioSharedReaderRules},
      Scenario {"emit-set-holds-the-ranked-set",
                &scenarioEmitSetHoldsTheRankedSet},
      Scenario {"emit-set-unions-two-entries",
                &scenarioEmitSetUnionsTwoEntries},
      Scenario {"emit-set-stands-empty-without-ranks",
                &scenarioEmitSetStandsEmptyWithoutRanks},
      Scenario {"emit-set-refusals", &scenarioEmitSetRefusals},
      Scenario {"emit-set-refuses-an-unwritable-destination",
                &scenarioEmitSetRefusesAnUnwritableDestination},
      Scenario {"counter-file-form-reads-addresses",
                &scenarioCounterFileFormReadsAddresses},
      Scenario {"counter-file-keeps-the-standing-resolution",
                &scenarioCounterFileKeepsTheStandingResolution},
      Scenario {"counter-file-refusals", &scenarioCounterFileRefusals},
      Scenario {"counter-file-from-environment-and-option-file",
                &scenarioCounterFileFromEnvironmentAndOptionFile},
      Scenario {"emit-set-round-trip", &scenarioEmitSetRoundTrip},
      Scenario {"option-order-settles", &scenarioOptionOrderSettles},
      Scenario {"option-without-hunt", &scenarioOptionWithoutHunt},
      Scenario {"option-order-refusals", &scenarioOptionOrderRefusals},
      Scenario {"placement-conflict-refusal",
                &scenarioPlacementConflictRefusal},
      Scenario {"ambient-combination-refusals",
                &scenarioAmbientCombinationRefusals},
      Scenario {"hunt-with-probe", &scenarioHuntWithProbe},
      Scenario {"hunt-with-threads-and-interleave",
                &scenarioHuntWithThreadsAndInterleave},
  };
  return kList;
}

// The marker line saying whether any benchmark function ran, printed at
// process exit so it stands beside the refusal line on every exit route.
void printRunMarker()
{
  std::fprintf(
      stderr, "%s\n", huntRuns > 0 ? "BENCHMARK_RAN" : "BENCHMARK_NO_RUN");
}

// The child the refusal rows run: the scripted host, the benchmark, the
// command line the parent passed, and the marker that says whether any
// benchmark function ran.
auto runChild(const std::string& script,
              const std::vector<std::string>& arguments,
              char** argv) -> int
{
  registerHost(script);
  writeSelectionFiles();
  registerCase();
  // The marker rides `std::atexit`, so it stands beside the refusal line
  // whether the run answers the refusal by returning it or by exiting with
  // the usage status (FR-081).
  std::atexit(&printRunMarker);
  std::vector<char*> inner {argv[0]};
  for (const auto& argument : arguments) {
    inner.push_back(const_cast<char*>(argument.c_str()));
  }
  return sg::speedgunMain(static_cast<int>(inner.size()), inner.data());
}

}  // namespace

auto main(const int argc, char** argv) -> int
{
  const std::string self =
      argc > 0 && argv[0] != nullptr ? argv[0] : "harness_hunt_cli_test";
  const std::string mode = argc > 1 && argv[1] != nullptr ? argv[1] : "";
  selfPath = self;

  if (mode == "hunt" && argc > 2 && argv[2] != nullptr) {
    const std::vector<std::string> arguments(argv + 3, argv + argc);
    return runChild(argv[2], arguments, argv);
  }

  const auto& list = scenarios();
  // One name selects one scenario through `argv[1]`, so a name that stands
  // twice would hide the scenario it repeats.
  for (std::size_t first = 0; first < list.size(); ++first) {
    check(list[first].body != nullptr, "every scenario carries a body");
    for (std::size_t second = first + 1; second < list.size(); ++second) {
      check(list[first].name != list[second].name,
            "no two scenarios of this file carry the same name");
    }
  }
  for (const auto& scenario : list) {
    if (scenario.name == mode) {
      scenario.body();
      return 0;
    }
  }
  if (!mode.empty()) {
    fail("the scenario name is not one of this file's list");
  }
  // Each scenario scripts a provider of its own and the counters system takes
  // these object paths once per process, so the walk re-executes this binary
  // once per entry, the pattern of `harness_pass_collect_test.cpp`.
  for (const auto& scenario : list) {
    std::string output;
    const int status = childRun(self, std::string(scenario.name), {}, output);
    if (output.find(": ok") == std::string::npos) {
      // Name the scenario, the wait status and the path the walk re-execed:
      // a child the walk could not start (a relink of this binary between two
      // scenarios answers 127 with "No such file or directory") must read as
      // that, and never as a scenario whose checks disagree.
      std::fprintf(stderr,
                   "%s: status %d from \"%s\":\n%s\n",
                   std::string(scenario.name).c_str(),
                   WEXITSTATUS(status),
                   self.c_str(),
                   output.c_str());
    }
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "the re-executed scenario passes");
  }
  std::printf("harness_hunt_cli_test: ok\n");
  return 0;
}
