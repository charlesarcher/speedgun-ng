// ============================================================================
// TDD test for the `--group-layout` row of specs/039-context-groups (T010,
// T012; FR-011, FR-015, FR-026, FR-035, FR-037, D-5).
//
// The row is one name, one internal value and one path. The scan recognizes
// the word, the settle arm carries the path through the resolved option set,
// the help answer prints the line the row builds, and the row carries no
// environment source, the shape `--compare-rules` keeps (FR-011). The value
// 299 stands in the free block above the noise tracer's 297, and the block
// comment of `detail/options.hpp` names the split that leaves it there.
//
// Nothing here opens the named file. The scan, the settle arm and the gather
// keep their rule of reading no file of their own (D-05), so each case hands
// the row a path that stands uncreated and asserts what the row answers. The
// file lines reach the gather as text, the way the entry point hands them.
// No scenario reads `/sys` or the host topology, starts a thread or reads a
// clock (FR-036, R-5).
//
// Check map:
//   theWordNamesTheRowAndSettlesItsPath
//                                the bare word answers the needs-a-value line
//                                a required row gives, the word with a path
//                                scans clean, and the resolved option set
//                                carries that path (FR-011, FR-008)
//   theRowCarriesItsRecord       the row of the live table carries the value
//                                299, a required argument and the owner this
//                                feature names (FR-011, FR-004)
//   theHelpAnswerCarriesTheRowLine
//                                `--help` prints the line the live row
//                                builds, in the table's own words (FR-011,
//                                FR-015, FR-018)
//   theRowCarriesNoEnvironmentSource
//                                the row flags no environment source, and a
//                                variable that carries the name settles and
//                                discloses nothing (FR-011, FR-026)
//   theWordsHoldThePathOverTheFile
//                                the word of the command line keeps its
//                                path over a file line that names one, and
//                                the dropped line discloses nothing
//                                (FR-037, FR-038)
//   theLastFileLineHoldsThePath  two lines of one file settle the path of the
//                                last of them, disclosed once (FR-035,
//                                FR-040)
// ============================================================================

#include <cstdio>
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <unistd.h>

#include "detail/internal.hpp"
#include "detail/options.hpp"
#include "speedgun-ng/benchmark.hpp"

namespace
{

using Scan = std::expected<sg::detail::CommandLineOptions, std::string>;
using Gathered = std::expected<sg::detail::ResolvedOptions, std::string>;

constexpr int kGroupLayoutValue = 299;
constexpr std::string_view kGroupName = "group-layout";
constexpr std::string_view kGroupEnvironment = "SPEEDGUN_GROUP_LAYOUT";

// The paths no case creates. The row carries a path and opens nothing, so an
// absent file answers the same at this seam as one that stands (D-05).
const std::string kWordPath = "/tmp/harness_group_cli_word.yaml";
const std::string kFileFirst = "/tmp/harness_group_cli_first.yaml";
const std::string kFileSecond = "/tmp/harness_group_cli_second.yaml";
const std::string kFilePath = "/tmp/harness_group_cli_layouts.conf";

auto fail(const std::string& what) -> void
{
  std::fprintf(stderr, "SPEEDGUN-NG TEST FAIL: %s\n", what.c_str());
  std::exit(1);
}

auto check(const bool cond, const std::string& what) -> void
{
  if (!cond) {
    fail(what);
  }
}

auto checkText(const std::string& actual,
               const std::string& expected,
               const std::string& what) -> void
{
  if (actual == expected) {
    return;
  }
  fail(what + "\n  expected: " + expected + "\n  actual:   " + actual);
}

auto readText(const std::filesystem::path& path) -> std::string
{
  std::ifstream file(path);
  std::ostringstream text;
  text << file.rdbuf();
  return text.str();
}

// One scan of one command line through the seam the scan owns. The words
// stand in this process's memory alone, so the scan reads no environment and
// opens no file (FR-012, D-05).
auto scanWords(const std::vector<std::string>& words) -> Scan
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_group_cli_test"));
  for (const std::string& word : words) {
    argv.push_back(const_cast<char*>(word.c_str()));
  }
  return sg::detail::scanCommandLine(static_cast<int>(argv.size()),
                                     argv.data());
}

// The gather of one scan over the sources a case hands as data (D-04, D-05).
auto gather(const sg::detail::CommandLineOptions& scan,
            const std::vector<sg::detail::EnvironmentRow>& environment,
            const std::optional<sg::detail::OptionFileText>& optionFile)
    -> Gathered
{
  return sg::detail::gatherOptionSources(scan, environment, optionFile);
}

// The path the resolved option set carries.
auto pathOf(const sg::detail::ResolvedOptions& resolved) -> std::string
{
  return resolved.groupLayout;
}

// The uses that name the layout row.
auto groupUses(const sg::detail::ResolvedOptions& resolved) -> std::size_t
{
  std::size_t found = 0;
  for (const sg::detail::OptionUse& use : resolved.uses) {
    if (use.row->name == kGroupName) {
      ++found;
    }
  }
  return found;
}

// The row of the live table, or no row when the table holds none (FR-033).
auto groupRow() -> const sg::detail::OptionRow*
{
  return sg::detail::rowNamed(kGroupName);
}

// The printed line of the help answer that names `name`, with the standard
// output captured to a file beside this process number. The entry point
// prints the answer and stops at status 0, so no instance runs and no
// topology reads (FR-017). An empty answer means no printed line names it.
auto helpAnswerLine(std::string_view name) -> std::string
{
  const std::filesystem::path outPath = std::filesystem::temp_directory_path()
      / ("harness_group_cli_" + std::to_string(static_cast<long>(getpid()))
         + ".out");
  const int savedOut = dup(STDOUT_FILENO);
  if (savedOut < 0 || std::freopen(outPath.c_str(), "w", stdout) == nullptr) {
    fail("the standard output of the entry point can be redirected");
    return {};
  }
  char program[] = "harness_group_cli_test";
  char word[] = "--help";
  char* argv[] = {program, word, nullptr};
  const int status = sg::speedgunMain(2, argv);
  std::fflush(stdout);
  if (dup2(savedOut, STDOUT_FILENO) < 0) {
    fail("the standard output of the entry point can be restored");
  }
  close(savedOut);
  const std::string printed = readText(outPath);
  std::filesystem::remove(outPath);
  check(
      status == 0,
      "the help answer stops at status 0 (FR-017): " + std::to_string(status));
  const std::string prefix = "  --" + std::string(name);
  std::string current;
  std::string found;
  for (const char letter : printed) {
    if (letter != '\n') {
      current += letter;
      continue;
    }
    if (found.empty() && current.starts_with(prefix)) {
      found = current;
    }
    current.clear();
  }
  return found;
}

// The word that names the row settles its path, and the bare word answers
// the needs-a-value line a required row gives (FR-011, FR-008).
auto theWordNamesTheRowAndSettlesItsPath() -> void
{
  const Scan bare = scanWords({"--group-layout"});
  check(!bare.has_value(),
        "the bare word names a row that needs a value (FR-008)");
  if (!bare.has_value()) {
    checkText(
        bare.error(),
        "option '--group-layout' needs a value",
        "the bare word answers the needs-a-value line of the row " "(FR-008)");
  }

  const Scan given = scanWords({"--group-layout=" + kWordPath});
  check(given.has_value(),
        "the scan recognizes --group-layout (FR-011): " + given.error_or({}));
  if (!given.has_value()) {
    return;
  }
  checkText(
      pathOf(given->settled),
      kWordPath,
      "the resolved option set carries the path the word named " "(FR-011)");
}

// The row of the table carries the value, the argument kind and the owner
// this feature names (FR-011, FR-004).
auto theRowCarriesItsRecord() -> void
{
  const sg::detail::OptionRow* const row = groupRow();
  check(row != nullptr,
        "the option table carries the group-layout row (FR-011)");
  if (row == nullptr) {
    return;
  }
  check(row->value == kGroupLayoutValue,
        "the row answers the value 299 of the free block (FR-011): "
            + std::to_string(row->value));
  check(sg::detail::kGroupLayoutOptionValue == kGroupLayoutValue,
        "the constant of the row spells 299 (FR-011): "
            + std::to_string(sg::detail::kGroupLayoutOptionValue));
  check(row->argument == sg::detail::OptionArgument::REQUIRED,
        "the row takes a required argument (FR-011)");
  checkText(row->owner,
            "specs/039",
            "the row names the requirement that owns it (FR-011)");
}

// The help answer prints one line per row out of the row's own words, so the
// live row is the expectation (FR-011, FR-015, FR-018).
auto theHelpAnswerCarriesTheRowLine() -> void
{
  const sg::detail::OptionRow* const row = groupRow();
  check(row != nullptr,
        "the option table carries the group-layout row (FR-011)");
  if (row == nullptr) {
    return;
  }
  checkText(
      helpAnswerLine(kGroupName),
      "  --" + std::string(kGroupName) + " <value>  " + row->help,
      "the help answer carries the line the row builds (FR-011, " "FR-015)");
}

// The row carries no environment source: the gather never asks for a
// variable of its name, so such a variable settles nothing and discloses
// nothing (FR-011, FR-026).
auto theRowCarriesNoEnvironmentSource() -> void
{
  const sg::detail::OptionRow* const row = groupRow();
  check(row != nullptr,
        "the option table carries the group-layout row (FR-011)");
  if (row == nullptr) {
    return;
  }
  check(!row->fromEnvironment, "the row flags no environment source (FR-011)");

  const Scan empty = scanWords({});
  check(empty.has_value(),
        "a line with no word scans clean (FR-012): " + empty.error_or({}));
  if (!empty.has_value()) {
    return;
  }
  const Gathered gathered = gather(
      *empty, {{std::string(kGroupEnvironment), kWordPath}}, std::nullopt);
  check(gathered.has_value(),
        "the gather accepts a variable no flagged row claims (FR-026): "
            + gathered.error_or({}));
  if (!gathered.has_value()) {
    return;
  }
  checkText(pathOf(*gathered),
            {},
            "the variable of the row's name settles no path (FR-011)");
  check(groupUses(*gathered) == 0,
        "the variable of the row's name discloses no use (FR-039)");
}

// The words stand above the file: the path the command line named keeps its
// place, and the file's line sets nothing and discloses nothing (FR-037,
// FR-038).
auto theWordsHoldThePathOverTheFile() -> void
{
  const Scan given = scanWords({"--group-layout=" + kWordPath});
  check(given.has_value(),
        "the scan recognizes --group-layout (FR-011): " + given.error_or({}));
  if (!given.has_value()) {
    return;
  }
  const sg::detail::OptionFileText file {kFilePath,
                                         "group-layout=" + kFileFirst + "\n"};
  const Gathered gathered = gather(*given, {}, file);
  check(gathered.has_value(),
        "the gather reads a file line of the layout row: "
            + gathered.error_or({}));
  if (!gathered.has_value()) {
    return;
  }
  checkText(pathOf(*gathered),
            kWordPath,
            "the word of the command line holds the path over the file line "
            "(FR-037)");
  check(groupUses(*gathered) == 0,
        "the dropped file line discloses no use (FR-038)");
}

// Two lines of one file are one source: the last line's path stands, and the
// row discloses the text that took effect (FR-035, FR-040).
auto theLastFileLineHoldsThePath() -> void
{
  const Scan empty = scanWords({});
  check(empty.has_value(),
        "a line with no word scans clean (FR-012): " + empty.error_or({}));
  if (!empty.has_value()) {
    return;
  }
  const sg::detail::OptionFileText file {
      kFilePath,
      "group-layout=" + kFileFirst + "\ngroup-layout=" + kFileSecond + "\n"};
  const Gathered gathered = gather(*empty, {}, file);
  check(gathered.has_value(),
        "the gather reads two file lines of the layout row: "
            + gathered.error_or({}));
  if (!gathered.has_value()) {
    return;
  }
  checkText(pathOf(*gathered),
            kFileSecond,
            "the last line of one file settles the path (FR-035)");
  check(groupUses(*gathered) == 1,
        "the row discloses one use for the settled path (FR-040): "
            + std::to_string(groupUses(*gathered)));
}

auto run() -> int
{
  theWordNamesTheRowAndSettlesItsPath();
  theRowCarriesItsRecord();
  theHelpAnswerCarriesTheRowLine();
  theRowCarriesNoEnvironmentSource();
  theWordsHoldThePathOverTheFile();
  theLastFileLineHoldsThePath();
  std::printf("harness_group_cli_test PASS\n");
  return 0;
}

}  // namespace

auto main() -> int
{
  try {
    return run();
  } catch (...) {
    std::fprintf(stderr, "SPEEDGUN-NG TEST FAIL: unhandled exception\n");
    return 1;
  }
}

// The split the row's own reach fixes. The six cases above answer as far as
// T012 carries the row: nothing reads the file the word names until the fit
// walk of T013 stands. The refusals T010 names arrive with the code that
// answers them, each written and seen red beside its implementation: a path
// no host opens, the pairing beside a placement preset and beside `--hunt`
// (FR-019, D-2), the batch a slot count stops (FR-020), and the one fault
// line that names the path (FR-021). Those cases land in the T013 and T014
// commit on the instruction of the orchestrating session, which fixes where
// each refusal is born.
