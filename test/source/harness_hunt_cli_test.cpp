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
// `test/source/harness_cli_test.cpp:800`, `:1166` and `:1417`: the refusal path
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
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "detail/hunt_fixture.hpp"
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
constexpr const char* kScriptNarrow = "narrow";
constexpr const char* kScriptNoLimit = "nolimit";
constexpr std::size_t kFullLimit = 6;
constexpr std::size_t kNarrowLimit = 2;

// The scratch files of this binary: the printed-report capture and the `@file`
// selections the contract's rows name.
constexpr const char* kScratchPath = "harness_hunt_cli_out.txt";
constexpr const char* kDocumentPath = "harness_hunt_cli_report.json";
constexpr const char* kBlankFile = "hunt_cli_blank.txt";
constexpr const char* kRawFile = "hunt_cli_raw.txt";
constexpr const char* kListFile = "hunt_cli_list.txt";
constexpr const char* kDupFile = "hunt_cli_dup.txt";

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

auto writeFile(const char* path, const std::string& text) -> void
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
  writeFile(kBlankFile, "# a comment\n\n   \n# another comment\n");
  writeFile(kRawFile, address(0) + "\nr4a03\n");
  writeFile(kListFile,
            "# one address per line\n" + address(0) + "\n\n" + address(1) + ","
                + address(2) + "\n");
  writeFile(kDupFile,
            "# the same address the list carries\n" + address(0) + "\n");
}

// ---------------------------------------------------------------------------
// The capture and child seams of `harness_cli_test.cpp`
// ---------------------------------------------------------------------------

auto registerHost(const std::string& script) -> void
{
  auto provider = std::make_unique<sg::counters::FakeProvider>();
  if (script == kScriptNoLimit) {
    sg::test::huntHost(*provider, std::nullopt);
  } else if (script == kScriptNarrow) {
    sg::test::huntHost(*provider, kNarrowLimit);
  } else {
    sg::test::huntHost(*provider, kFullLimit);
  }
  const auto settled =
      sg::counters::System::local().registerProvider(std::move(provider));
  check(settled.has_value(), "the hunt fixture registers on the local system");
}

int huntRuns = 0;

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
  if (saved < 0 || std::freopen(kScratchPath, "w", stdout) == nullptr) {
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

  std::ifstream file(kScratchPath);
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
              std::string& output) -> int
{
  std::string command = "\"" + self + "\" " + mode;
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
};

auto rawCodeLine() -> std::string
{
  return "--hunt: 'r4a03' is a raw event code; --hunt takes catalog addresses";
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
  const int status = childRun(selfPath, "hunt", arguments, output);
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
                  {"--hunt=@hunt_cli_blank.txt"},
                  "--hunt: '@hunt_cli_blank.txt' holds no catalog address"},
                 {"full", {"--hunt=r4a03"}, raw},
                 {"full", {"--hunt=cpu/r4a03"}, raw},
                 {"full", {"--hunt=" + address(0) + ",r4a03"}, raw},
                 {"full", {"--hunt=cpu/r4a03*"}, raw},
                 {"full", {"--hunt=@" + std::string(kRawFile)}, raw}});
  std::printf("selection-refusals: ok\n");
}

auto scenarioOptionValueRefusals() -> void
{
  checkRefusals(
      {{"full",
        {"--hunt-norm=ipc"},
        "--hunt-norm: expected instr, cycle or iter, got 'ipc'"},
       {"full",
        {"--hunt-rank=sensitivity"},
        "--hunt-rank: expected rate or cv, got 'sensitivity'"},
       {"full",
        {"--hunt-rank=delta"},
        "--hunt-rank: expected rate or cv, got 'delta'"},
       {"full",
        {"--hunt-rank=score"},
        "--hunt-rank: expected rate or cv, got 'score'"},
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
        "--hunt-budget: expected <n>s or <n>p, got '120'"},
       {"full",
        {"--hunt-budget=abc"},
        "--hunt-budget: expected <n>s or <n>p, got 'abc'"},
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
        "--hunt-seed: expected a whole number, got 'abc'"}});
  std::printf("option-value-refusals: ok\n");
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
      captureRun(withFilter({"--hunt=@" + std::string(kListFile)}));
  check(countOf(file, "hunt: plan events=3 ") == 1,
        "a @file with comments and blank lines selects its three addresses "
        "(FR-006)");

  const auto dup = captureRun(
      withFilter({"--hunt=@" + std::string(kDupFile) + "," + address(0)}));
  check(countOf(dup, "hunt: plan events=1 ") == 1,
        "the same address across a list and a file keeps one column (FR-006)");
  check(countOf(dup, address(0)) == 1,
        "one column means one row naming that address once (FR-006)");
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
  std::remove(kDocumentPath);
  captureRun(
      withFilter({"--hunt=none", "--json=" + std::string(kDocumentPath)}));
  check(readAll(kDocumentPath).find("\"hunt\"") == std::string::npos,
        "the run with no hunt writes no hunt key (FR-052, I-8)");
  std::printf("hunt-none-like-no-hunt: ok\n");
}

auto scenarioMultiplexCombination() -> void
{
  registerHost(kScriptFull);
  registerCase();
  std::remove(kDocumentPath);

  const auto run =
      captureRun(withFilter({"--hunt=" + address(0) + "," + address(1),
                             "--metric-method=multiplex",
                             "--json=" + std::string(kDocumentPath)}));
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
  const std::string document = readAll(kDocumentPath);
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
// The scenario list CTest runs
// ---------------------------------------------------------------------------

struct Scenario
{
  std::string_view name;
  void (*body)();
};

// The scenarios of this file: what CTest runs with no argument, and what
// `argv[1]` selects from (T018, US1).
[[nodiscard]] auto scenarios() -> const std::vector<Scenario>&
{
  static const std::vector<Scenario> kList {
      Scenario {"selection-refusals", &scenarioSelectionRefusals},
      Scenario {"option-value-refusals", &scenarioOptionValueRefusals},
      Scenario {"limit-refusal", &scenarioLimitRefusal},
      Scenario {"conflict-refusals", &scenarioConflictRefusals},
      Scenario {"accepted-forms", &scenarioAcceptedSelectionForms},
      Scenario {"bare-hunt-whole-catalog", &scenarioBareHuntSelectsTheCatalog},
      Scenario {"hunt-none-like-no-hunt", &scenarioHuntNoneBehavesLikeNoHunt},
      Scenario {"multiplex-combination", &scenarioMultiplexCombination},
      Scenario {"dry-run-one-row-per-column", &scenarioDryRunCombination},
      Scenario {"modes-print-and-stop", &scenarioModesPrintAndStop},
      Scenario {"filter-applies", &scenarioFilterAppliesToTheHunt},
      Scenario {"repeated-option-keeps-last", &scenarioRepeatedOptionKeepsLast},
      Scenario {"no-limit-host", &scenarioNoLimitHostPlanLine},
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
      std::fprintf(stderr, "%s\n", output.c_str());
    }
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "the re-executed scenario passes");
  }
  std::printf("harness_hunt_cli_test: ok\n");
  return 0;
}
