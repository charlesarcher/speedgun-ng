// ============================================================================
// The `--topdown` option: the two forms the contract states, the level each
// settles, the refusals, and the environment name that does not exist
// (specs/042-topdown T025; US1; FR-019, FR-031, FR-050, D-5;
// contracts/topdown-option.md).
//
// Two seams hold every expectation. `detail::scanCommandLine` answers the
// level a command line settles, the seam the contract names for the accepted
// forms, and it leaves the words it claimed nothing for behind `optind`.
// `sg::speedgunMain` answers the refusals: the row's arm reports one line and
// leaves with the usage status, so each refusal row runs in a re-exec of this
// binary, the pattern of `test/source/harness_hunt_cli_test.cpp`, and the wait
// status carries whichever exit route the library took. The child prints
// `BENCHMARK_NO_RUN` when the entry point reached no benchmark function, so a
// refusal answers with one line, that marker, and nothing else.
//
// The gather answers the environment row. The table flags this row for no
// environment source, the scan reads no process environment, and the gather
// reads the rows the entry point hands it (FR-012, D-05), so a
// `SPEEDGUN_TOPDOWN` name settles nothing here and changes nothing there.
//
// No case sleeps, reads a clock for its verdict, or retries: a scan answers in
// this process, and a refusal child answers at its own exit.
//
// Check map:
//   theBareWordAsksLevelOne        `--topdown` settles level 1 and claims no
//                                  second word (FR-019)
//   anAttachedLevelAsksThatLevel   `--topdown=<n>` settles level `<n>` at each
//                                  of the six levels (FR-019)
//   aSpaceSeparateWordIsNoLevel    `--topdown 2` settles level 1 and leaves
//                                  `2` an unclaimed positional (D-5)
//   aBadLevelAnswersOneLine        `0`, `7`, `x` and the empty text each
//                                  answer one quoted line at status 2 with no
//                                  benchmark reached (FR-050, D-5)
//   aFileLineRefusesTheRow         a file line that names the row answers the
//                                  origin-prefixed refusal at status 2, in the
//                                  bare and in the attached spelling (FR-031)
//   noEnvironmentNameExists        no `SPEEDGUN_TOPDOWN` reaches the run: the
//                                  gather settles no level for it, and the
//                                  entry point runs as if it stood unset
//                                  (FR-019)
// ============================================================================

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <getopt.h>
#include <sys/wait.h>
#include <unistd.h>

#include "detail/options.hpp"
#include "speedgun-ng/benchmark.hpp"

namespace
{

int failures = 0;

// The name this binary was started under, which the child re-execs.
std::string selfPath;

auto report(const std::string& what,
            const std::string& expected,
            const std::string& actual) -> void
{
  ++failures;
  std::printf("FAILED: %s\n  expected: %s\n  actual:   %s\n",
              what.c_str(),
              expected.c_str(),
              actual.c_str());
}

auto check(const bool condition, const char* what) -> void
{
  if (!condition) {
    ++failures;
    std::printf("FAILED: %s\n", what);
  }
}

// The one line of a refused level, quoted the way the hunt range reader quotes
// a value it refused (`source/harness/hunt_cli.cpp:44`).
auto refusalLine(const std::string& text) -> std::string
{
  return "--topdown: expected 1 to 6, got '" + text + "'";
}

auto levelWord(const std::optional<std::uint64_t>& level) -> std::string
{
  return level.has_value() ? std::to_string(*level) : "no level";
}

// ---------------------------------------------------------------------------
// The scan seam: one command line, the fault or the level it settled, and the
// words the walk left unclaimed (the shape `harness_cli_interleave_test.cpp`
// reads).
// ---------------------------------------------------------------------------

const char* const kAccepted = "the scan answered no fault";

struct Outcome
{
  std::string fault;
  std::optional<std::uint64_t> level;
  std::vector<std::string> unclaimed;
  sg::detail::CommandLineOptions settled {};
};

auto scanOf(std::initializer_list<const char*> words) -> Outcome
{
  // The words outlive the walk: the scanner reports a rejected word by the
  // pointer it holds, so the text has to stand while the answer is read.
  const std::vector<std::string> owned(words.begin(), words.end());
  std::vector<char*> argv {const_cast<char*>("harness_topdown_option_test")};
  for (const auto& word : owned) {
    argv.push_back(const_cast<char*>(word.c_str()));
  }
  const auto answer =
      sg::detail::scanCommandLine(static_cast<int>(argv.size()), argv.data());
  Outcome outcome {};
  if (!answer.has_value()) {
    outcome.fault = answer.error();
    return outcome;
  }
  outcome.fault = kAccepted;
  outcome.level = answer->settled.options.topdownLevel;
  outcome.settled = *answer;
  // The walk claimed every word it read, so the words behind `optind` are the
  // ones no row took (D-5).
  for (int index = optind; index < static_cast<int>(argv.size()); ++index) {
    outcome.unclaimed.push_back(argv[static_cast<std::size_t>(index)]);
  }
  return outcome;
}

auto theBareWordAsksLevelOne() -> void
{
  const Outcome outcome = scanOf({"--topdown"});
  if (outcome.fault != kAccepted) {
    report("the bare word is accepted (FR-019)", kAccepted, outcome.fault);
    return;
  }
  if (outcome.level != 1ULL) {
    report("the bare word settles level 1 (FR-019)",
           "1",
           levelWord(outcome.level));
  }
  if (!outcome.unclaimed.empty()) {
    report("the bare word claims no further word (FR-019)",
           "no unclaimed word",
           outcome.unclaimed.front());
  }
}

auto anAttachedLevelAsksThatLevel() -> void
{
  for (std::uint64_t level = 1; level <= 6; ++level) {
    const std::string word = "--topdown=" + std::to_string(level);
    const Outcome outcome = scanOf({word.c_str()});
    const std::string what =
        "the attached form '" + word + "' settles that level (FR-019)";
    if (outcome.fault != kAccepted) {
      report(what, kAccepted, outcome.fault);
      continue;
    }
    if (outcome.level != level) {
      report(what, std::to_string(level), levelWord(outcome.level));
    }
  }
}

auto aSpaceSeparateWordIsNoLevel() -> void
{
  const Outcome outcome = scanOf({"--topdown", "2"});
  if (outcome.fault != kAccepted) {
    report(
        "a separate word is accepted at all (D-5)", kAccepted, outcome.fault);
    return;
  }
  if (outcome.level != 1ULL) {
    report("a separate word leaves the level at 1 (D-5)",
           "1",
           levelWord(outcome.level));
  }
  if (outcome.unclaimed.size() != 1 || outcome.unclaimed.front() != "2") {
    report("the separate word stands unclaimed after the walk (D-5)",
           "2",
           outcome.unclaimed.empty() ? "no unclaimed word"
                                     : outcome.unclaimed.front());
  }
}

// ---------------------------------------------------------------------------
// The entry point seam: one re-exec, its merged answer, and the marker that
// says whether a benchmark function ran.
// ---------------------------------------------------------------------------

int benchmarkRuns = 0;

auto bmTopdownOption(sg::State& state) -> void
{
  ++benchmarkRuns;
  for (auto _ : state) {
  }
}

// The marker stands beside the refusal line on every exit route, the way it
// stands for the hunt refusals.
void printRunMarker()
{
  std::fprintf(
      stderr, "%s\n", benchmarkRuns > 0 ? "BENCHMARK_RAN" : "BENCHMARK_NO_RUN");
}

auto scratchPath(const char* kind) -> std::filesystem::path
{
  return std::filesystem::temp_directory_path()
      / ("harness_topdown_option_" + std::to_string(static_cast<long>(getpid()))
         + kind);
}

auto writeFile(const std::filesystem::path& path,
               const std::string& text) -> void
{
  std::ofstream file(path, std::ios::trunc);
  file << text;
  file.close();
  check(static_cast<bool>(file), "the scenario could write its option file");
}

auto subprocessOutput(const std::string& command, std::string& output) -> int
{
  FILE* const pipe = popen(command.c_str(), "r");
  if (pipe == nullptr) {
    ++failures;
    std::printf("FAILED: the usage subprocess starts\n");
    return -1;
  }
  char buffer[256];
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    output += buffer;
  }
  return pclose(pipe);
}

// Re-exec this binary as a run of the entry point. Every argument stands
// quoted, so a word with nothing in it survives the shell.
auto childRun(const std::vector<std::string>& arguments,
              std::string& output,
              const std::string& environment = {}) -> int
{
  std::string command = environment + "\"" + selfPath + "\" run";
  for (const auto& argument : arguments) {
    command += " \"" + argument + "\"";
  }
  return subprocessOutput(command + " 2>&1", output);
}

// One refusal row: the usage status, the one line, the marker that no
// benchmark ran, and no other byte.
auto checkRefusal(const std::vector<std::string>& arguments,
                  const std::string& line,
                  const std::string& what) -> void
{
  std::string output;
  const int status = childRun(arguments, output);
  const std::string expected = line + "\nBENCHMARK_NO_RUN\n";
  if (WEXITSTATUS(status) != 2) {
    report(what, "status 2", std::to_string(WEXITSTATUS(status)));
  }
  if (output != expected) {
    report(what, expected, output);
  }
}

auto aBadLevelAnswersOneLine() -> void
{
  const std::vector<std::string> texts {"0", "7", "x", ""};
  for (const std::string& text : texts) {
    const std::string what = "the value '" + text
        + "' answers one line at status 2 with no benchmark reached "
          "(FR-050, D-5)";
    checkRefusal(
        {"--topdown=" + text, "--iterations=1"}, refusalLine(text), what);
  }
}

auto aFileLineRefusesTheRow() -> void
{
  const auto path = scratchPath("_file.txt");
  const std::vector<std::string> lines {"topdown", "topdown=2"};
  for (const std::string& line : lines) {
    writeFile(path, line + "\n");
    const std::string expected =
        path.string() + ":1: the option 'topdown' cannot stand in an option "
                        "file";
    const std::string what = "the option file line '" + line
        + "' answers the refusal behind its origin at status 2 (FR-031)";
    checkRefusal(
        {"--options=" + path.string(), "--iterations=1"}, expected, what);
  }
  std::filesystem::remove(path);
}

auto noEnvironmentNameExists() -> void
{
  const auto row = sg::detail::environmentRowOf("SPEEDGUN_TOPDOWN=4");
  check(row.has_value(),
        "the environment reader reads a SPEEDGUN_ name of any shape (FR-021)");
  const Outcome scan = scanOf({"--iterations=1"});
  if (!row.has_value() || scan.fault != kAccepted) {
    report("the two reads the environment case needs both answer",
           kAccepted,
           scan.fault);
    return;
  }
  const auto settled =
      sg::detail::gatherOptionSources(scan.settled, {*row}, std::nullopt);
  if (!settled.has_value()) {
    report("a SPEEDGUN_TOPDOWN name answers no fault (FR-019)",
           "the gather settled the sources",
           settled.error());
    return;
  }
  if (settled->options.topdownLevel.has_value()) {
    report("no SPEEDGUN_TOPDOWN name settles a level (FR-019)",
           "no level",
           levelWord(settled->options.topdownLevel));
  }
  std::string output;
  const int status =
      childRun({"--iterations=1"}, output, "SPEEDGUN_TOPDOWN=4 ");
  if (WEXITSTATUS(status) != 0) {
    report("a SPEEDGUN_TOPDOWN name leaves the run's status alone (FR-019)",
           "status 0",
           std::to_string(WEXITSTATUS(status)));
  }
  if (output.find("topdown") != std::string::npos) {
    report("a SPEEDGUN_TOPDOWN name prints nothing about the option (FR-019)",
           "no line names the option",
           output);
  }
  check(output.find("BENCHMARK_RAN") != std::string::npos,
        "the run under SPEEDGUN_TOPDOWN reaches its benchmark (FR-019)");
}

// The child every entry-point row re-executes: the one benchmark, the marker,
// and the entry point itself.
auto runChild(const std::vector<std::string>& arguments, char** argv) -> int
{
  auto handle = sg::registerBenchmark(&bmTopdownOption, "bmTopdownOption");
  handle.iterations(1);
  std::atexit(&printRunMarker);
  std::vector<char*> inner {argv[0]};
  for (const auto& argument : arguments) {
    inner.push_back(const_cast<char*>(argument.c_str()));
  }
  return sg::speedgunMain(static_cast<int>(inner.size()), inner.data());
}

}  // namespace

auto main(int argc, char** argv) -> int
{
  selfPath =
      argc > 0 && argv[0] != nullptr ? argv[0] : "harness_topdown_option_test";
  const std::string mode = argc > 1 && argv[1] != nullptr ? argv[1] : "";
  if (mode == "run") {
    return runChild(std::vector<std::string>(argv + 2, argv + argc), argv);
  }

  theBareWordAsksLevelOne();
  anAttachedLevelAsksThatLevel();
  aSpaceSeparateWordIsNoLevel();
  aBadLevelAnswersOneLine();
  aFileLineRefusesTheRow();
  noEnvironmentNameExists();

  if (failures != 0) {
    std::printf("%d checks failed\n", failures);
    return 1;
  }
  std::printf("harness_topdown_option_test: ok\n");
  return 0;
}
