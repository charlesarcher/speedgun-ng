// ============================================================================
// TDD test for the one option table of specs/028-full-command-line (T004,
// T016, T018, T054, T059; FR-001, FR-003, FR-004, FR-005, FR-015 to
// FR-018, FR-061, FR-069, FR-070, FR-074, FR-079, SC-001, SC-008,
// contracts/option-table.md, contracts/help-and-version.md,
// contracts/page-and-examples.md).
//
// The contract fixes 35 rows in one order. The 20 rows master carries keep
// their spelling, their argument kind and their internal value, the seven
// new names take the constants 290 to 296 that spec D-24 reserves, and the
// eight hunt names of specs/029-hunter-core take the block 270 to 279 that
// lane reserves. The expected rows stand here as literals, the master name
// to value mapping of FR-013 among them.
//
// The seam the contract names, `detail::optionRows()`, is declared at T006
// and written at T007, so this file reads the rows out of the source that
// holds the table: the row text is the only carrier of the internal value,
// and FR-075 keeps that text open to a reader, which is how
// `tools/docs/example_drift.py` reaches the same rows. The table file is
// found by argument, by the working directory or by this file's own path,
// and the candidate with the most rows wins, so the move of the table from
// `cli.cpp` to `options.cpp` at T007 needs no change here.
//
// The help and version cases of T018 drive the entry point itself: each
// command line runs through `sg::speedgunMain` with both output streams
// captured to files, and the expected help text is built from the live
// table through `detail::optionRows()`, the seam the contract names as
// this file's oracle, so the printed lines and the table cannot drift.
// The version expectation reads the `VERSION` of the `project(` block of
// the root `CMakeLists.txt`, the value the build hands the harness as
// `SG_PROJECT_VERSION` for its console line. The printing lands at T019,
// so the four cases stand red until that task lands.
//
// The two cases of T054 pin the rules option into the two surfaces a
// reader learns the options from: the recorded help answer and the
// option reference of `docs/pages/harness.md`. The page is read from the
// source directory handed to this test as an argument, the way the
// project version is read. The sweep of T059 extends that one-row
// comparison to every row of both surfaces in both directions, and it
// counts the rows each direction walks and the rows that carry an
// environment name (FR-069, FR-070, FR-074, SC-008).
//
// Check map:
//   theTableAnswersARowList        one source file yields rows
//   theTableHoldsThirtyFiveRows    the row count is the contract's 35
//                                  (FR-001, FR-004, SC-001)
//   theNamesStandInTheContractOrder
//                                  the names in table order equal the
//                                  contract list (FR-001, FR-003)
//   eachRowCarriesItsArgumentKind  required, optional or none per row
//                                  (FR-001)
//   eachRowCarriesItsValue         the letter or the constant per row
//                                  (FR-001, FR-003, FR-079)
//   theBaseOptionsKeepTheirMasterValues
//                                  the 20 rows master carries keep the
//                                  letter or the constant of FR-013,
//                                  spelled out as a table of their own
//                                  (FR-003, FR-079, SC-001)
//   theSevenNewNamesTakeTheReservedConstants
//                                  `options`, `env`, `threads`,
//                                  `interleave`, `compare-rules`, `help`
//                                  and `version` at 290 to 296 (FR-004)
//   noNameAndNoValueRepeat         uniqueness over the whole table
//                                  (FR-004, SC-001)
//   theHuntNamesTakeTheReservedConstants
//                                  the eight hunt rows at 270 to 277,
//                                  the numbers
//                                  contracts/hunt-cli.md states them with
//                                  (specs/029 FR-083, D-22)
//   theFixtureTwinReportsItsPlantedDefects
//                                  the two rules above report the planted
//                                  duplicate name and the planted value
//                                  clash of the fixture twin, so a table
//                                  that deserves red gets it
//                                  (specs/029 FR-083, T020)
//   theValueBlocksStayUniqueAndDisjoint
//                                  the 20 master rows answer inside the
//                                  letters and 256 to 259, the 7 new
//                                  rows inside 290 to 296, the two
//                                  blocks share no number, and no name
//                                  or value repeats across them
//                                  (FR-004, FR-079)
//   theCarriedConstantsNameTheirOptions
//                                  256 to 259 name `metric-method`,
//                                  `compare`, `compare-rule` and
//                                  `compare-strict` (FR-003, SC-001)
//   theScanCarriesNoShortLetter    the option string of the scan holds no
//                                  letter (FR-005)
//   theHelpAnswerPrintsOneLinePerRow
//                                  `--help` prints one line per table row
//                                  in table order, each the two leading
//                                  spaces, the long name with its leading
//                                  dashes, the argument part its kind
//                                  implies and the row's help text
//                                  (FR-015, FR-018, SC-008)
//   theVersionAnswerPrintsTheProjectVersion
//                                  `--version` prints one line naming the
//                                  project and the version the build
//                                  defines (FR-016, SC-008)
//   helpAndVersionAnswerStatusZero
//                                  both answers stop at status 0 and
//                                  print no report row (FR-017, SC-008)
//   theWalkAnswersTheFirstEventItMeets
//                                  the first of `--help`, `--version` or a
//                                  usage fault that the left-to-right walk
//                                  meets wins, over the three orderings
//                                  the contract gives (FR-017, SC-008)
//   theRulesOptionShowsInTheHelpAnswer
//                                  the recorded help answer built from
//                                  `detail::optionRows()` carries the line
//                                  of the `compare-rules` row (FR-061,
//                                  FR-015, SC-008)
//   theRulesOptionShowsOnTheGuidePage
//                                  the option reference of the guide page
//                                  carries a row for `compare-rules` with
//                                  the help text of the table, and names
//                                  no environment variable beside a row
//                                  the table flags for none (FR-061,
//                                  FR-069, FR-074)
//   thePageAndTheTableAgreeInBothDirections
//                                  every table row has a page row of the
//                                  same argument kind, environment name
//                                  and help text, every page row names a
//                                  table row, and both directions walk 27
//                                  rows of which 12 carry an environment
//                                  name (FR-069, FR-070, FR-074, SC-008)
//   aTwoCountThreadListRunsAtBothCounts
//                                  `--threads=1,2` lists the bare family
//                                  and the `/threads:2` instance, and the
//                                  run answers one report row per count
//                                  (FR-044, FR-048, SC-004)
// ============================================================================

#include <array>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <unistd.h>

#include "detail/cli_table_fixture.hpp"
#include "detail/options.hpp"
#include "speedgun-ng/benchmark.hpp"

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

auto report(const char* what,
            const std::string& expected,
            const std::string& actual) -> void
{
  ++failures;
  std::printf("FAILED: %s\n  expected: %s\n  actual:   %s\n",
              what,
              expected.c_str(),
              actual.c_str());
}

// One row of the contract table, in the order the contract fixes it.
// `argument` is 'r' where the option takes a value, 'o' where the value
// is optional and 'n' where the option takes none. `value` is the code the
// scan switches on: a letter for the sixteen master rows that carry one, a
// number of 256 or above for the rest.
struct ExpectedRow
{
  std::string_view name;
  char argument;
  int value;
};

constexpr char kRequired = 'r';
constexpr char kOptional = 'o';
constexpr char kNoArgument = 'n';

constexpr std::array<ExpectedRow, 35> kContract = {{
    {"filter", kRequired, 'f'},
    {"list", kNoArgument, 'l'},
    {"repetitions", kRequired, 'r'},
    {"min-time", kRequired, 'm'},
    {"iterations", kRequired, 'i'},
    {"warmup-time", kRequired, 'w'},
    {"dry-run", kNoArgument, 'd'},
    {"counter", kRequired, 'c'},
    {"metric", kRequired, 'M'},
    {"catalog", kNoArgument, 'C'},
    {"percentiles", kRequired, 'p'},
    {"no-percentiles", kNoArgument, 'P'},
    {"placement", kRequired, 'A'},
    {"topology", kNoArgument, 'T'},
    {"json", kRequired, 'j'},
    {"json-raw", kRequired, 'J'},
    {"compare", kRequired, 257},
    {"compare-rule", kRequired, 258},
    {"compare-strict", kNoArgument, 259},
    {"metric-method", kRequired, 256},
    {"options", kRequired, 290},
    {"env", kRequired, 291},
    {"threads", kRequired, 292},
    {"interleave", kOptional, 293},
    {"compare-rules", kRequired, 294},
    {"help", kNoArgument, 295},
    {"version", kNoArgument, 296},
    {"hunt", kOptional, 270},
    {"hunt-norm", kRequired, 271},
    {"hunt-rank", kRequired, 272},
    {"hunt-top", kRequired, 273},
    {"hunt-noise", kRequired, 274},
    {"hunt-repeats", kRequired, 275},
    {"hunt-budget", kRequired, 276},
    {"hunt-seed", kRequired, 277},
}};

// The rows master carries today; the seven rows after them are the ones
// this feature adds, and the eight hunt rows of specs/029-hunter-core close
// the table.
constexpr std::size_t kMasterRows = 20;
constexpr std::size_t kRowsBeforeHunt = 27;

// The bounds of the three value blocks: the constants the harness carried
// before this feature, the constants specs/029-hunter-core reserves for the
// eight hunt names, and the constants spec D-24 reserves for the seven new
// names with 297 to 319 left free for the lanes that follow (FR-004,
// FR-079; specs/029 FR-083, D-22).
constexpr int kCarriedLow = 256;
constexpr int kCarriedHigh = 259;
constexpr int kHuntLow = 270;
constexpr int kHuntHigh = 279;
constexpr int kReservedLow = 290;
constexpr int kReservedHigh = 296;

// The eight hunt rows and the numbers
// `specs/029-hunter-core/contracts/hunt-cli.md` assigns them; the rule binds
// the number, whichever constant spells it (specs/029 FR-083, D-22).
constexpr std::array<std::pair<std::string_view, int>, 8> kHuntValues = {{
    {"hunt", 270},
    {"hunt-norm", 271},
    {"hunt-rank", 272},
    {"hunt-top", 273},
    {"hunt-noise", 274},
    {"hunt-repeats", 275},
    {"hunt-budget", 276},
    {"hunt-seed", 277},
}};

// The hunt rows of the fixture twin: the eight of the contract plus the row
// whose name is the planted duplicate.
constexpr std::size_t kFixtureHuntRows = 9;

// The master name to value mapping of FR-013: the sixteen letters and the
// four carried constants of those 20 rows, spelled out here as a table of
// their own so a change to one of them answers a named case (FR-003,
// FR-079, SC-001).
constexpr std::array<std::pair<std::string_view, int>, kMasterRows>
    kBaseValues = {{
        {"filter", 'f'},         {"list", 'l'},
        {"repetitions", 'r'},    {"min-time", 'm'},
        {"iterations", 'i'},     {"warmup-time", 'w'},
        {"dry-run", 'd'},        {"counter", 'c'},
        {"metric", 'M'},         {"catalog", 'C'},
        {"percentiles", 'p'},    {"no-percentiles", 'P'},
        {"placement", 'A'},      {"topology", 'T'},
        {"json", 'j'},           {"json-raw", 'J'},
        {"compare", 257},        {"compare-rule", 258},
        {"compare-strict", 259}, {"metric-method", 256},
    }};

// One row read out of the source text.
struct Row
{
  std::string name;
  char argument;
  int value;
};

using Constants = std::vector<std::pair<std::string, int>>;

auto readText(const std::filesystem::path& path) -> std::string
{
  std::ifstream file(path);
  std::ostringstream text;
  text << file.rdbuf();
  return text.str();
}

// The directory that holds the table source, reached from the argument,
// the working directory or this file's own path, so the test runs before
// CMake hands it a path.
auto tableDirectory(const char* argument)
    -> std::optional<std::filesystem::path>
{
  const std::array<const char*, 4> prefixes = {
      "source/harness/", "harness/", "source/", ""};
  std::vector<std::filesystem::path> starts;
  if (argument != nullptr) {
    starts.emplace_back(argument);
  }
  starts.push_back(std::filesystem::current_path());
  starts.emplace_back(__FILE__);
  for (auto start : starts) {
    for (int hop = 0; hop <= 8 && !start.empty(); ++hop) {
      for (const char* prefix : prefixes) {
        for (const char* file : {"options.cpp", "cli.cpp"}) {
          if (std::filesystem::exists(start / prefix / file)) {
            return start / prefix;
          }
        }
      }
      start = start.parent_path();
    }
  }
  return std::nullopt;
}

// The named option values of the sources: each `constexpr int` line, so a
// row that spells its value as a constant resolves to the number the scan
// switches on.
auto collectConstants(std::string_view text, Constants& found) -> void
{
  constexpr std::string_view kLead = "constexpr int ";
  for (std::size_t at = text.find(kLead); at != std::string_view::npos;
       at = text.find(kLead, at + kLead.size()))
  {
    std::size_t cursor = at + kLead.size();
    const std::size_t nameStart = cursor;
    while (cursor < text.size()
           && (std::isalnum(static_cast<unsigned char>(text[cursor]))
               || text[cursor] == '_'))
    {
      ++cursor;
    }
    const std::string name(text.substr(nameStart, cursor - nameStart));
    while (cursor < text.size() && (text[cursor] == ' ' || text[cursor] == '='))
    {
      ++cursor;
    }
    const std::size_t numberStart = cursor;
    while (cursor < text.size()
           && std::isdigit(static_cast<unsigned char>(text[cursor])))
    {
      ++cursor;
    }
    if (!name.empty() && numberStart < cursor) {
      found.emplace_back(name,
                         std::stoi(std::string(
                             text.substr(numberStart, cursor - numberStart))));
    }
  }
}

// The argument kind of one row, read from the spelling the row uses:
// `required_argument` and its like in a `getopt` row, the enumerator name
// in an `OptionRow`.
auto argumentKindOf(std::string_view entry) -> char
{
  if (entry.find("required_argument") != std::string_view::npos
      || entry.find("REQUIRED") != std::string_view::npos)
  {
    return kRequired;
  }
  if (entry.find("optional_argument") != std::string_view::npos
      || entry.find("OPTIONAL") != std::string_view::npos)
  {
    return kOptional;
  }
  if (entry.find("no_argument") != std::string_view::npos
      || entry.find("NONE") != std::string_view::npos)
  {
    return kNoArgument;
  }
  return '?';
}

// One braced row of the table. The first quoted text is the name, the
// first single quoted character is a letter value, a bare word can name a
// constant and a bare number is a value spelled as a literal. Quoted text
// is read ahead of the words, so a help sentence cannot reach the value.
// The terminator row carries no name and answers nothing.
auto parseRow(std::string_view entry,
              const Constants& constants) -> std::optional<Row>
{
  Row row;
  row.argument = argumentKindOf(entry);
  std::optional<int> letter;
  std::optional<int> named;
  std::optional<int> number;
  for (std::size_t at = 0; at < entry.size(); ++at) {
    const char current = entry[at];
    if (current == '"') {
      std::string text;
      ++at;
      while (at < entry.size() && entry[at] != '"') {
        if (entry[at] == '\\' && at + 1 < entry.size()) {
          ++at;
        }
        text += entry[at];
        ++at;
      }
      if (row.name.empty()) {
        row.name = std::move(text);
      }
      continue;
    }
    if (current == '\'' && at + 2 < entry.size() && entry[at + 2] == '\'') {
      letter = static_cast<unsigned char>(entry[at + 1]);
      at += 2;
      continue;
    }
    if (std::isalpha(static_cast<unsigned char>(current)) || current == '_') {
      const std::size_t start = at;
      while (at < entry.size()
             && (std::isalnum(static_cast<unsigned char>(entry[at]))
                 || entry[at] == '_'))
      {
        ++at;
      }
      const std::string word(entry.substr(start, at - start));
      for (const auto& constant : constants) {
        if (constant.first == word) {
          named = constant.second;
        }
      }
      --at;
      continue;
    }
    if (std::isdigit(static_cast<unsigned char>(current))) {
      const std::size_t start = at;
      while (at < entry.size()
             && std::isdigit(static_cast<unsigned char>(entry[at])))
      {
        ++at;
      }
      if (!number) {
        number = std::stoi(std::string(entry.substr(start, at - start)));
      }
      --at;
      continue;
    }
  }
  if (row.name.empty()) {
    return std::nullopt;
  }
  if (letter) {
    row.value = *letter;
  } else if (named) {
    row.value = *named;
  } else if (number) {
    row.value = *number;
  } else {
    return std::nullopt;
  }
  return row;
}

// The closing brace of the row that opens at `open`, with quoted text
// skipped so a brace inside a help sentence cannot close the row early.
auto closingBrace(std::string_view text, std::size_t open) -> std::size_t
{
  int depth = 0;
  for (std::size_t at = open; at < text.size(); ++at) {
    const char current = text[at];
    if (current == '"') {
      ++at;
      while (at < text.size() && text[at] != '"') {
        if (text[at] == '\\') {
          ++at;
        }
        ++at;
      }
      continue;
    }
    if (current == '\'') {
      at += 2;
      continue;
    }
    if (current == '{') {
      ++depth;
    }
    if (current == '}' && --depth == 0) {
      return at;
    }
  }
  return std::string_view::npos;
}

// The rows of the table in source order, from the row that names `filter`
// to the terminator that carries no name.
auto parseRows(std::string_view text,
               const Constants& constants) -> std::vector<Row>
{
  std::vector<Row> rows;
  const std::size_t anchor = text.find("\"filter\"");
  if (anchor == std::string_view::npos) {
    return rows;
  }
  std::size_t at = text.rfind('{', anchor);
  while (at != std::string_view::npos && rows.size() < 64) {
    const std::size_t end = closingBrace(text, at);
    if (end == std::string_view::npos) {
      break;
    }
    const auto row = parseRow(text.substr(at + 1, end - at - 1), constants);
    if (!row) {
      break;
    }
    rows.push_back(*row);
    const std::size_t next = text.find('{', end + 1);
    // The rows stand side by side in one array, so a closing brace
    // between two of them ends the array: the walk stops there and reads
    // no braced block of other code as a row (FR-079).
    if (next == std::string_view::npos || text.find('}', end + 1) < next) {
      break;
    }
    at = next;
  }
  return rows;
}

// The option string the scan hands `getopt_long`, the third argument of
// the call, quotes included. An empty answer means the call takes that
// string from a name this reader cannot resolve.
auto scanOptionString(std::string_view text) -> std::string
{
  const std::size_t call = text.find("getopt_long(");
  if (call == std::string_view::npos) {
    return {};
  }
  std::string argument;
  std::size_t depth = 1;
  std::size_t index = 0;
  for (std::size_t at = call + 12; at < text.size() && depth > 0; ++at) {
    const char current = text[at];
    if (current == '"') {
      argument += current;
      while (at + 1 < text.size() && text[at + 1] != '"') {
        ++at;
        if (text[at] == '\\') {
          ++at;
        }
        argument += text[at];
      }
      if (at + 1 < text.size()) {
        argument += '"';
        ++at;
      }
      continue;
    }
    if (current == '(') {
      ++depth;
    }
    if (current == ')') {
      --depth;
      if (depth == 0) {
        break;
      }
    }
    if (current == ',' && depth == 1) {
      if (index == 2) {
        return argument;
      }
      ++index;
      argument.clear();
      continue;
    }
    argument += current;
  }
  return index == 2 ? argument : std::string {};
}

auto rowNamed(const std::vector<Row>& rows, std::string_view name) -> const Row*
{
  for (const auto& row : rows) {
    if (row.name == name) {
      return &row;
    }
  }
  return nullptr;
}

auto rowWithValue(const std::vector<Row>& rows, const int value) -> const Row*
{
  for (const auto& row : rows) {
    if (row.value == value) {
      return &row;
    }
  }
  return nullptr;
}

auto kindWord(const char kind) -> std::string
{
  switch (kind) {
    case kRequired:
      return "required";
    case kOptional:
      return "optional";
    case kNoArgument:
      return "none";
    default:
      return "unreadable";
  }
}

// The value as the contract writes it: a quoted letter, or the number.
auto valueWord(const int value) -> std::string
{
  if (value >= 256) {
    return std::to_string(value);
  }
  return "'" + std::string(1, static_cast<char>(value)) + "'";
}

auto joinedNames(const std::vector<Row>& rows) -> std::string
{
  std::string text;
  for (const auto& row : rows) {
    if (!text.empty()) {
      text += ' ';
    }
    text += row.name;
  }
  return text;
}

auto contractNames() -> std::string
{
  std::string text;
  for (const auto& expected : kContract) {
    if (!text.empty()) {
      text += ' ';
    }
    text += expected.name;
  }
  return text;
}

auto theTableAnswersARowList(const std::vector<Row>& rows) -> void
{
  check(!rows.empty(), "the table source answers a row list (FR-079)");
}

auto theTableHoldsThirtyFiveRows(const std::vector<Row>& rows) -> void
{
  if (rows.size() == kContract.size()) {
    return;
  }
  report("the table holds the 35 rows of the contract (FR-001, FR-004, SC-001)",
         "35 rows",
         std::to_string(rows.size()) + " rows");
}

auto theNamesStandInTheContractOrder(const std::vector<Row>& rows) -> void
{
  std::size_t shared = 0;
  while (shared < rows.size() && shared < kContract.size()
         && rows[shared].name == kContract[shared].name)
  {
    ++shared;
  }
  if (shared == kContract.size()) {
    return;
  }
  report("the names stand in the contract order (FR-001, FR-003)",
         contractNames(),
         joinedNames(rows));
}

auto eachRowCarriesItsArgumentKind(const std::vector<Row>& rows) -> void
{
  for (const auto& expected : kContract) {
    const std::string label = "the row '" + std::string(expected.name)
        + "' carries the kind '" + kindWord(expected.argument) + "' (FR-001)";
    const Row* found = rowNamed(rows, expected.name);
    if (found == nullptr) {
      report(label.c_str(), kindWord(expected.argument), "no such row");
      continue;
    }
    if (found->argument != expected.argument) {
      report(label.c_str(),
             kindWord(expected.argument),
             kindWord(found->argument));
    }
  }
}

auto eachRowCarriesItsValue(const std::vector<Row>& rows) -> void
{
  for (const auto& expected : kContract) {
    const std::string label = "the row '" + std::string(expected.name)
        + "' carries the value " + valueWord(expected.value)
        + " (FR-001, FR-003, FR-079)";
    const Row* found = rowNamed(rows, expected.name);
    if (found == nullptr) {
      report(label.c_str(), valueWord(expected.value), "no such row");
      continue;
    }
    if (found->value != expected.value) {
      report(label.c_str(), valueWord(expected.value), valueWord(found->value));
    }
  }
}

auto theBaseOptionsKeepTheirMasterValues(const std::vector<Row>& rows) -> void
{
  for (const auto& [name, value] : kBaseValues) {
    const std::string label = "the master row '" + std::string(name)
        + "' keeps the value " + valueWord(value) + " (FR-003, FR-079, SC-001)";
    const Row* found = rowNamed(rows, name);
    if (found == nullptr) {
      report(label.c_str(), valueWord(value), "no such row");
      continue;
    }
    if (found->value != value) {
      report(label.c_str(), valueWord(value), valueWord(found->value));
    }
  }
}

auto theSevenNewNamesTakeTheReservedConstants(const std::vector<Row>& rows)
    -> void
{
  for (std::size_t index = kMasterRows; index < kRowsBeforeHunt; ++index) {
    const auto& expected = kContract[index];
    const std::string label = "the new row '" + std::string(expected.name)
        + "' takes the constant " + std::to_string(expected.value)
        + " (FR-004)";
    const Row* found = rowNamed(rows, expected.name);
    if (found == nullptr) {
      report(label.c_str(), std::to_string(expected.value), "no such row");
      continue;
    }
    if (found->value < 290 || found->value > 319) {
      report(label.c_str(),
             "a value inside 290 to 319",
             std::to_string(found->value));
    }
  }
}

auto noNameAndNoValueRepeat(const std::vector<Row>& rows) -> void
{
  for (std::size_t first = 0; first < rows.size(); ++first) {
    for (std::size_t second = first + 1; second < rows.size(); ++second) {
      if (rows[first].name == rows[second].name) {
        report("no two rows share a name (FR-004, SC-001)",
               "one row named '" + rows[first].name + "'",
               "two rows named '" + rows[first].name + "'");
      }
      if (rows[first].value == rows[second].value) {
        report("no two rows share a value (FR-004, SC-001)",
               "one row with the value " + valueWord(rows[first].value),
               "two rows with the value " + valueWord(rows[first].value));
      }
    }
  }
}

// The first name two rows share, or null when every name stands alone.
auto repeatedName(const std::vector<Row>& rows) -> const std::string*
{
  for (std::size_t first = 0; first < rows.size(); ++first) {
    for (std::size_t second = first + 1; second < rows.size(); ++second) {
      if (rows[first].name == rows[second].name) {
        return &rows[first].name;
      }
    }
  }
  return nullptr;
}

// The first row whose value another row already carries, or null when
// every value stands alone.
auto repeatedValue(const std::vector<Row>& rows) -> const Row*
{
  for (std::size_t first = 0; first < rows.size(); ++first) {
    for (std::size_t second = first + 1; second < rows.size(); ++second) {
      if (rows[first].value == rows[second].value) {
        return &rows[first];
      }
    }
  }
  return nullptr;
}

// The value block of the 20 rows master carries: the letters the scan
// switches on, kept as internal switch codes, and the constants 256 to
// 259 those rows already hold (FR-003, FR-004, FR-005).
auto inMasterBlock(const int value) -> bool
{
  return (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z')
      || (value >= kCarriedLow && value <= kCarriedHigh);
}

// The value block of the 7 rows this feature brings: the constants 290
// to 296 that spec D-24 reserves (FR-004).
auto inNewBlock(const int value) -> bool
{
  return value >= kReservedLow && value <= kReservedHigh;
}

// The value block of the 8 hunt rows: the constants 270 to 279 that
// specs/029-hunter-core reserves, 278 and 279 unused this release
// (specs/029 FR-083, D-22).
auto inHuntBlock(const int value) -> bool
{
  return value >= kHuntLow && value <= kHuntHigh;
}

auto theValueBlocksStayUniqueAndDisjoint(const std::vector<Row>& rows) -> void
{
  const std::string* sharedName = repeatedName(rows);
  if (sharedName != nullptr) {
    report("no name repeats across the three blocks (FR-004, FR-079)",
           "one row named '" + *sharedName + "'",
           "two rows named '" + *sharedName + "'");
  }
  const Row* sharedValue = repeatedValue(rows);
  if (sharedValue != nullptr) {
    report("no value repeats across the three blocks (FR-004, FR-079)",
           "one row with the value " + valueWord(sharedValue->value),
           "two rows with the value " + valueWord(sharedValue->value));
  }

  bool sawMaster = false;
  bool sawNew = false;
  bool sawHunt = false;
  int highestMaster = 0;
  int lowestNew = 0;
  int lowestHunt = 0;
  int highestHunt = 0;
  for (std::size_t index = 0; index < rows.size(); ++index) {
    const Row& row = rows[index];
    const bool master = index < kMasterRows;
    const bool hunt = index >= kRowsBeforeHunt;
    const bool inside = hunt
        ? inHuntBlock(row.value)
        : (master ? inMasterBlock(row.value) : inNewBlock(row.value));
    if (!inside) {
      const std::string label = std::string(hunt         ? "the hunt row '"
                                                : master ? "the master row '"
                                                         : "the new row '")
          + row.name + "' answers inside its block (FR-004, FR-079)";
      report(label.c_str(),
             hunt         ? "a constant from 270 to 279"
                 : master ? "a letter or a constant from 256 to 259"
                          : "a constant from 290 to 296",
             valueWord(row.value));
    }
    if (hunt) {
      if (!sawHunt || row.value < lowestHunt) {
        lowestHunt = row.value;
      }
      if (!sawHunt || row.value > highestHunt) {
        highestHunt = row.value;
      }
      sawHunt = true;
    } else if (master) {
      if (!sawMaster || row.value > highestMaster) {
        highestMaster = row.value;
      }
      sawMaster = true;
    } else {
      if (!sawNew || row.value < lowestNew) {
        lowestNew = row.value;
      }
      sawNew = true;
    }
  }
  if (sawMaster && sawHunt && highestMaster >= lowestHunt) {
    report("the master block and the hunt block share no number (FR-004, "
           "FR-079, specs/029 D-22)",
           "the highest master value below the lowest hunt value",
           std::to_string(highestMaster) + " against "
               + std::to_string(lowestHunt));
  }
  if (sawHunt && sawNew && highestHunt >= lowestNew) {
    report(
        "the hunt block and the new block share no number (FR-004, FR-079)",
        "the highest hunt value below the lowest new value",
        std::to_string(highestHunt) + " against " + std::to_string(lowestNew));
  }
  if (sawMaster && sawNew && highestMaster >= lowestNew) {
    report("the two value blocks share no number (FR-004, FR-079)",
           "the highest master value below the lowest new value",
           std::to_string(highestMaster) + " against "
               + std::to_string(lowestNew));
  }
}

// The eight hunt rows of the table, each at the number the contract of
// specs/029-hunter-core states (FR-083, SC-05, D-22).
auto theHuntNamesTakeTheReservedConstants(const std::vector<Row>& rows) -> void
{
  for (const std::pair<std::string_view, int>& entry : kHuntValues) {
    const Row* row = rowNamed(rows, entry.first);
    if (row == nullptr) {
      report("the table carries a row for every hunt option (FR-083)",
             std::string(entry.first) + " at " + std::to_string(entry.second),
             "no row named " + std::string(entry.first));
      continue;
    }
    if (row->value != entry.second) {
      report("every hunt option answers the number its contract states "
             "(FR-083, D-22)",
             std::string(entry.first) + " at " + std::to_string(entry.second),
             std::string(entry.first) + " at " + std::to_string(row->value));
    }
  }
}

// The rows of the fixture twin of T020, in the shape the two rules above
// read them: the terminator row whose name is null stands out of the list,
// as it does in a parsed table.
auto fixtureRows() -> std::vector<Row>
{
  std::vector<Row> rows;
  for (const option& entry : sg::test::kCliFixtureTable) {
    if (entry.name == nullptr) {
      continue;
    }
    rows.push_back(Row {std::string(entry.name), kRequired, entry.val});
  }
  return rows;
}

// The negative control of the two rules above: a gate that has only ever
// seen a correct table proves nothing, so the twin carries one planted
// duplicate name and one planted value clash and each rule must report what
// was planted in it (specs/029 FR-083, T020, SC-05).
auto theFixtureTwinReportsItsPlantedDefects() -> void
{
  std::size_t huntRows = 0;
  for (const Row& row : fixtureRows()) {
    if (row.name.starts_with("hunt")) {
      ++huntRows;
    }
  }
  check(huntRows == kFixtureHuntRows,
        "the twin holds the eight hunt rows and the duplicated one (T020)");

  const std::string* name = repeatedName(fixtureRows());
  if (name == nullptr || *name != sg::test::kCliFixturePlantedName) {
    report("the name rule reports the planted duplicate of the twin (T020)",
           std::string(sg::test::kCliFixturePlantedName),
           name == nullptr ? "no repeated name" : *name);
  }

  const Row* clash = repeatedValue(fixtureRows());
  if (clash == nullptr || clash->value != sg::test::kCliFixturePlantedValue) {
    report(
        "the value rule reports the planted clash of the twin (T020)",
        std::to_string(sg::test::kCliFixturePlantedValue),
        clash == nullptr ? "no repeated value" : std::to_string(clash->value));
  }
}

auto theCarriedConstantsNameTheirOptions(const std::vector<Row>& rows) -> void
{
  const std::array<std::pair<int, std::string_view>, 4> carried = {{
      {256, "metric-method"},
      {257, "compare"},
      {258, "compare-rule"},
      {259, "compare-strict"},
  }};
  for (const auto& [value, name] : carried) {
    const std::string label = "the value " + std::to_string(value) + " names '"
        + std::string(name) + "' (FR-003, SC-001)";
    const Row* found = rowWithValue(rows, value);
    if (found == nullptr) {
      report(label.c_str(), std::string(name), "no row carries that value");
      continue;
    }
    if (found->name != name) {
      report(label.c_str(), std::string(name), found->name);
    }
  }
}

auto theScanCarriesNoShortLetter(const std::string& optionText) -> void
{
  std::string letters = optionText;
  if (letters.size() >= 2 && letters.front() == '"' && letters.back() == '"') {
    letters = letters.substr(1, letters.size() - 2);
  }
  for (const char current : letters) {
    if (std::isalpha(static_cast<unsigned char>(current))) {
      report("the option string of the scan carries no short letter (FR-005)",
             "an option string with no letter",
             "the option string '" + letters + "'");
      return;
    }
  }
}

// One run of the entry point with both output streams captured: the
// printed text and the answered status are read together, and a fault
// line on standard error stays out of this file's own verdict output.
// The command lines these cases give answer a status from the entry
// point, and the scan calls no `std::exit` on them (FR-010, FR-011). The
// capture files stand in the system temporary directory under this
// process number, so a run from the repository root leaves no file
// behind and two runs never share one.
struct Run
{
  int status = 0;
  std::string out;
  std::string err;
};

auto capturePath(const char* kind) -> std::filesystem::path
{
  return std::filesystem::temp_directory_path()
      / ("harness_cli_table_" + std::to_string(static_cast<long>(getpid()))
         + kind);
}

auto runEntry(std::initializer_list<const char*> words) -> Run
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_cli_table_test"));
  for (const char* word : words) {
    argv.push_back(const_cast<char*>(word));
  }

  const std::filesystem::path outPath = capturePath(".out");
  const std::filesystem::path errPath = capturePath(".err");
  const int savedOut = dup(STDOUT_FILENO);
  const int savedErr = dup(STDERR_FILENO);
  if (savedOut < 0 || savedErr < 0
      || std::freopen(outPath.c_str(), "w", stdout) == nullptr
      || std::freopen(errPath.c_str(), "w", stderr) == nullptr)
  {
    fail("the two output streams of the entry point can be redirected");
    return {};
  }

  Run run;
  run.status = sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
  std::fflush(stdout);
  std::fflush(stderr);
  if (dup2(savedOut, STDOUT_FILENO) < 0 || dup2(savedErr, STDERR_FILENO) < 0) {
    fail("the two output streams of the entry point can be restored");
  }
  close(savedOut);
  close(savedErr);
  run.out = readText(outPath);
  run.err = readText(errPath);
  std::filesystem::remove(outPath);
  std::filesystem::remove(errPath);
  return run;
}

// The lines one capture printed. The newline that closes the last line
// leaves no line of its own, so a trailing blank line shows as one empty
// entry (FR-015).
auto linesOf(const std::string& text) -> std::vector<std::string>
{
  std::vector<std::string> lines;
  std::string line;
  for (const char current : text) {
    if (current == '\n') {
      lines.push_back(line);
      line.clear();
      continue;
    }
    line += current;
  }
  if (!line.empty()) {
    lines.push_back(line);
  }
  return lines;
}

// The argument part of one help line: ` <value>` for a required
// argument, ` [<value>]` for an optional one, nothing for none.
auto argumentPart(const sg::detail::OptionArgument argument) -> std::string
{
  if (argument == sg::detail::OptionArgument::REQUIRED) {
    return " <value>";
  }
  if (argument == sg::detail::OptionArgument::OPTIONAL) {
    return " [<value>]";
  }
  return {};
}

// The help line of one table row in the shape the contract fixes: two
// leading spaces, the long name with its leading dashes, the argument
// part, two spaces, the row's help text (FR-015, FR-018).
auto helpLine(const sg::detail::OptionRow& row) -> std::string
{
  return "  --" + std::string(row.name) + argumentPart(row.argument) + "  "
      + row.help;
}

// The project version the build defines: the `VERSION` of the `project(`
// block of the root `CMakeLists.txt`, the same text the build hands the
// harness as `SG_PROJECT_VERSION` for the console line. The walk starts
// at the table directory and climbs, so the version is read the way the
// table itself is read.
auto projectVersion(const std::filesystem::path& start) -> std::string
{
  std::filesystem::path directory = std::filesystem::absolute(start);
  for (int hop = 0; hop <= 8 && !directory.empty(); ++hop) {
    const std::string text = readText(directory / "CMakeLists.txt");
    if (text.find("project(") != std::string::npos) {
      std::istringstream lines(text);
      std::string line;
      while (std::getline(lines, line)) {
        const std::size_t word = line.find_first_not_of(" \t");
        if (word == std::string::npos || line.compare(word, 8, "VERSION ") != 0)
        {
          continue;
        }
        std::istringstream value(line.substr(word + 8));
        std::string version;
        if ((value >> version) && !version.empty()
            && std::isdigit(static_cast<unsigned char>(version[0])))
        {
          return version;
        }
      }
    }
    directory = directory.parent_path();
  }
  return {};
}

// FR-015, FR-018, SC-008: one line per table row, in table order, each
// built from the live table.
auto theHelpAnswerPrintsOneLinePerRow() -> void
{
  const Run run = runEntry({"--help"});
  const std::vector<std::string> printed = linesOf(run.out);
  const auto rows = sg::detail::optionRows();
  if (printed.size() != rows.size()) {
    report("the help answer prints one line per table row (FR-015, SC-008)",
           std::to_string(rows.size()) + " lines",
           std::to_string(printed.size()) + " lines");
    return;
  }
  for (std::size_t index = 0; index < rows.size(); ++index) {
    const std::string expected = helpLine(rows[index]);
    if (printed[index] != expected) {
      report("each help line carries the name, the argument part and the "
             "help text of its row (FR-015, FR-018)",
             expected,
             printed[index]);
      return;
    }
  }
}

// FR-016, SC-008: one line naming the project and the version the build
// defines.
auto theVersionAnswerPrintsTheProjectVersion(const std::string& version) -> void
{
  const Run run = runEntry({"--version"});
  const std::vector<std::string> printed = linesOf(run.out);
  if (printed.size() != 1) {
    report("the version answer prints one line (FR-016, SC-008)",
           "1 line",
           std::to_string(printed.size()) + " lines");
    return;
  }
  const std::string expected = "speedgun-ng " + version;
  if (printed[0] != expected) {
    report("the version line names the project and the version the build "
           "defines (FR-016, SC-008)",
           expected,
           printed[0]);
  }
}

// FR-017, SC-008: both answers stop before any benchmark runs and answer
// status 0. A report row carries `iterations=`, so its absence is the
// reading that no run happened.
auto helpAndVersionAnswerStatusZero() -> void
{
  const Run help = runEntry({"--help"});
  const Run version = runEntry({"--version"});
  check(help.status == 0, "the help answer stops at status 0 (FR-017, SC-008)");
  check(version.status == 0,
        "the version answer stops at status 0 (FR-017, SC-008)");
  check(help.out.find("iterations=") == std::string::npos,
        "the help answer runs no benchmark (FR-017)");
  check(version.out.find("iterations=") == std::string::npos,
        "the version answer runs no benchmark (FR-017)");
}

// FR-017, SC-008: the walk answers the first of the two print options or
// a usage fault that its left-to-right walk meets, over the three
// orderings the contract gives.
auto theWalkAnswersTheFirstEventItMeets() -> void
{
  const Run helpFirst = runEntry({"--help", "--nonsense=1"});
  check(helpFirst.status == 0,
        "the help word ahead of a bad word answers status 0 (FR-017)");
  check(
      helpFirst.out.find("  --filter") != std::string::npos,
      "the help word ahead of a bad word prints the option lines " "(FR-017)");

  const Run faultFirst = runEntry({"--nonsense=1", "--help"});
  check(faultFirst.status == 2,
        "a usage fault ahead of the help word answers status 2 (FR-017)");
  check(faultFirst.out.find("  --filter") == std::string::npos,
        "the fault ahead of the help word prints no option lines (FR-017)");
  check(!faultFirst.err.empty(),
        "the fault ahead of the help word prints its own line (FR-017)");

  const Run both = runEntry({"--help", "--version"});
  check(both.status == 0,
        "the help word ahead of the version word answers status 0 (FR-017)");
  check(both.out.find("speedgun-ng ") == std::string::npos,
        "the help word ahead of the version word prints the help alone "
        "(FR-017)");
}

// The row of the live table that carries `name`, or null when no row
// does. The live table is this file's oracle, so a case that pins one
// option reads its expectation from there, and the walk itself belongs to
// the library's one lookup (FR-033).
auto liveRow(const std::string_view name) -> const sg::detail::OptionRow*
{
  return sg::detail::rowNamed(name);
}

// The first printed line that opens with `prefix`, or an empty answer
// when the printed text holds none.
auto lineWith(const std::vector<std::string>& lines,
              const std::string& prefix) -> std::string
{
  for (const std::string& line : lines) {
    if (line.compare(0, prefix.size(), prefix) == 0) {
      return line;
    }
  }
  return {};
}

// The guide page that carries the option reference, reached by climbing
// from the table directory the way the project version is read. An empty
// answer means no page stands on the way up.
auto guidePageText(const std::filesystem::path& start) -> std::string
{
  std::filesystem::path directory = std::filesystem::absolute(start);
  for (int hop = 0; hop <= 8 && !directory.empty(); ++hop) {
    const std::filesystem::path page =
        directory / "docs" / "pages" / "harness.md";
    if (std::filesystem::exists(page)) {
      return readText(page);
    }
    directory = directory.parent_path();
  }
  return {};
}

// The cells of one markdown table row, each stripped of the spaces and
// the backticks that set an option name. A line that opens with no
// vertical bar is prose and answers nothing.
auto cellsOf(const std::string& line) -> std::vector<std::string>
{
  std::vector<std::string> cells;
  if (line.empty() || line.front() != '|') {
    return cells;
  }
  std::string cell;
  for (const char current : line) {
    if (current == '|') {
      cells.push_back(cell);
      cell.clear();
      continue;
    }
    if (current != ' ' && current != '`') {
      cell += current;
    }
  }
  cells.push_back(cell);
  return cells;
}

// The page row that carries one option: the markdown row whose first
// named cell spells the option, with or without its leading dashes. The
// name cell is the matcher, so a row of a longer option that opens with
// these letters cannot answer for it.
auto pageRowOf(const std::vector<std::string>& lines,
               const std::string_view name) -> std::optional<std::string>
{
  for (const std::string& line : lines) {
    std::string spelled;
    for (const std::string& cell : cellsOf(line)) {
      if (!cell.empty()) {
        spelled = cell;
        break;
      }
    }
    while (spelled.size() >= 3 && spelled[0] == '-' && spelled[1] == '-') {
      spelled.erase(0, 2);
    }
    if (std::string_view(spelled) == name) {
      return line;
    }
  }
  return std::nullopt;
}

// The argument kind as the page spells it: `required`, `optional` or
// `none`, the three words the option table contract uses for its
// `argument` column (FR-001, FR-069).
auto argumentWord(const sg::detail::OptionArgument argument) -> std::string
{
  if (argument == sg::detail::OptionArgument::REQUIRED) {
    return "required";
  }
  if (argument == sg::detail::OptionArgument::OPTIONAL) {
    return "optional";
  }
  return "none";
}

// The environment name of a flagged row, computed from the row: `SPEEDGUN_`
// plus the long name upper-cased with its dashes turned into underscores
// (FR-019, D-09). The gather keeps its own reader inside an anonymous
// namespace in `options.cpp`, so this file applies the same rule to the
// same row and holds no list of names.
auto environmentOf(const sg::detail::OptionRow& row) -> std::string
{
  std::string name = "SPEEDGUN_";
  for (const char letter : std::string_view(row.name)) {
    name.push_back(letter == '-' ? '_'
                                 : static_cast<char>(std::toupper(
                                       static_cast<unsigned char>(letter))));
  }
  return name;
}

// The text of a table row as a page cell holds it. `cellsOf` reads a
// markdown cell without its spaces, so the table's own text is compared
// with its spaces dropped the same way.
auto cellText(const std::string_view text) -> std::string
{
  std::string cell;
  for (const char current : text) {
    if (current != ' ') {
      cell += current;
    }
  }
  return cell;
}

// The rows of the option reference of the page: the run of markdown rows
// under the header that names the four columns Option, Argument,
// Environment and Help text. The header anchors the sweep, so a table
// elsewhere on the page stays out of it (FR-070). A row answers its four
// cells through `cellsOf`, and the delimiter row of the table answers
// nothing.
auto pageOptionRows(const std::vector<std::string>& lines)
    -> std::vector<std::vector<std::string>>
{
  std::size_t header = 0;
  for (; header < lines.size(); ++header) {
    const std::vector<std::string> cells = cellsOf(lines[header]);
    if (cells.size() == 6 && cells[1] == "Option" && cells[2] == "Argument"
        && cells[3] == "Environment" && cells[4] == "Helptext")
    {
      break;
    }
  }
  std::vector<std::vector<std::string>> rows;
  for (std::size_t at = header + 1; at < lines.size(); ++at) {
    const std::vector<std::string> cells = cellsOf(lines[at]);
    if (cells.size() != 6 || cells[1].empty()) {
      break;
    }
    if (cells[1] != "---") {
      rows.push_back(cells);
    }
  }
  return rows;
}

// The 12 rows FR-020 names as the ones the environment can set.
constexpr int kEnvironmentRows = 12;

// FR-061, FR-015, SC-008: the rules option stands among the options the
// help answer lists, so the line the recorded answer prints for it is
// the line the live table builds.
auto theRulesOptionShowsInTheHelpAnswer() -> void
{
  const sg::detail::OptionRow* row = liveRow("compare-rules");
  if (row == nullptr) {
    fail("the live table carries the compare-rules row (FR-061)");
    return;
  }
  const Run run = runEntry({"--help"});
  const std::string printed = lineWith(linesOf(run.out), "  --compare-rules");
  const std::string expected = helpLine(*row);
  if (printed != expected) {
    report("the help answer carries the compare-rules line built from the "
           "table (FR-061, FR-015, SC-008)",
           expected,
           printed.empty() ? "no printed line names the option" : printed);
  }
}

// FR-061, FR-069, FR-074: the guide reference carries the same option
// with the help text the table carries, and it names no environment
// variable beside a row the table flags for none.
auto theRulesOptionShowsOnTheGuidePage(const std::filesystem::path& start)
    -> void
{
  const sg::detail::OptionRow* row = liveRow("compare-rules");
  if (row == nullptr) {
    fail("the live table carries the compare-rules row (FR-061)");
    return;
  }
  const std::string page = guidePageText(start);
  check(!page.empty(),
        "the guide page of the option reference can be read (FR-069)");
  const std::optional<std::string> found = pageRowOf(linesOf(page), row->name);
  if (!found) {
    report("the guide reference carries a row for the compare-rules option "
           "(FR-061, FR-074)",
           "a page row whose first named cell spells compare-rules",
           "no page row names it");
    return;
  }
  check(found->find(row->help) != std::string::npos,
        "the guide row of compare-rules carries the help text of the table "
        "(FR-069)");
  check(found->find("SPEEDGUN_") == std::string::npos,
        "the guide row of an option the table flags for no environment "
        "variable names none (FR-074)");
}

// FR-069, FR-070, FR-074, SC-008: the option reference of the guide page
// and the one option table agree in both directions. Every row of the
// table has a row on the page of the same name, carrying the same
// argument kind, the same environment name where the row carries one and
// the same help text, and every row of the page names a row of the
// table. A row missing from the page and an option missing from the table
// are both findings. Both directions walk 27 rows, and 12 of them carry
// an environment name.
auto thePageAndTheTableAgreeInBothDirections(const std::filesystem::path& start)
    -> void
{
  const std::string page = guidePageText(start);
  check(!page.empty(),
        "the guide page of the option reference can be read (FR-069)");
  const std::vector<std::vector<std::string>> printed =
      pageOptionRows(linesOf(page));
  const auto rows = sg::detail::optionRows();

  if (printed.size() != rows.size()) {
    report("the page carries one row per row of the table (FR-069, FR-070, "
           "SC-008)",
           std::to_string(rows.size()) + " page rows",
           std::to_string(printed.size()) + " page rows");
  }

  int flaggedInTable = 0;
  for (const sg::detail::OptionRow& row : rows) {
    if (row.fromEnvironment) {
      ++flaggedInTable;
    }
    const std::vector<std::string>* found = nullptr;
    for (const std::vector<std::string>& cells : printed) {
      if (cells[1] == row.name) {
        found = &cells;
        break;
      }
    }
    const std::string head =
        "the page row of the option '" + std::string(row.name) + "' ";
    if (found == nullptr) {
      report((head + "stands on the page (FR-069, FR-070)").c_str(),
             "a page row named " + std::string(row.name),
             "no page row names it");
      continue;
    }
    const std::string kind = argumentWord(row.argument);
    if ((*found)[2] != kind) {
      report((head + "carries the argument kind of the table (FR-069)").c_str(),
             kind,
             (*found)[2]);
    }
    const std::string environment =
        row.fromEnvironment ? environmentOf(row) : std::string();
    if ((*found)[3] != environment) {
      report(
          (head + "carries the environment name of the table (FR-074)").c_str(),
          environment.empty() ? "no environment name" : environment,
          (*found)[3].empty() ? "no environment name" : (*found)[3]);
    }
    const std::string help = cellText(row.help);
    if ((*found)[4] != help) {
      report((head + "carries the help text of the table (FR-069)").c_str(),
             help,
             (*found)[4]);
    }
  }

  int flaggedOnPage = 0;
  for (const std::vector<std::string>& cells : printed) {
    if (!cells[3].empty()) {
      ++flaggedOnPage;
    }
    if (liveRow(cells[1]) == nullptr) {
      const std::string label = "every page row names a row of the table "
                                "(FR-070, SC-008): the page row '"
          + cells[1] + "'";
      report(label.c_str(),
             "a table row named " + cells[1],
             "no table row names it");
    }
  }

  if (flaggedInTable != kEnvironmentRows) {
    report("the table flags 12 rows for the environment (FR-020, FR-074)",
           "12 flagged rows",
           std::to_string(flaggedInTable) + " flagged rows");
  }
  if (flaggedOnPage != kEnvironmentRows) {
    report("the page names an environment variable beside 12 rows (FR-074, "
           "SC-008)",
           "12 named rows",
           std::to_string(flaggedOnPage) + " named rows");
  }
}

}  // namespace

// The one family the two-count case runs. An empty body keeps the run
// short at either count.
auto bmTwoCounts(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

SG_BENCHMARK(bmTwoCounts);

// The printed lines that name `name`, joined for a failure reading.
auto namingLines(const std::vector<std::string>& lines,
                 const std::string& name) -> std::vector<std::string>
{
  std::vector<std::string> found;
  for (const std::string& line : lines) {
    if (line.find(name) != std::string::npos) {
      found.push_back(line);
    }
  }
  return found;
}

auto joinedLines(const std::vector<std::string>& lines) -> std::string
{
  std::string text;
  for (const std::string& line : lines) {
    text += line;
    text += " | ";
  }
  return text;
}

// FR-044, FR-048, SC-004: a `--threads` list of two counts runs every
// benchmark at both counts. The listing names the bare family and the
// `/threads:2` instance, and the run of one family answers one report row
// per count, the row of the one-count instance carrying no `/threads:`
// segment. `--no-percentiles` keeps the console at one line per instance,
// one filter and one iteration keep the run short, and the filter keeps
// its `^` anchor and drops the `$`, because the instance name carries the
// segment above one count.
auto aTwoCountThreadListRunsAtBothCounts() -> void
{
  const std::string name = "bmTwoCounts";

  const Run listed = runEntry({"--threads=1,2", "--list"});
  const std::vector<std::string> names = linesOf(listed.out);
  check(listed.status == 0,
        "a listing of a two-count thread list answers status 0 "
        "(FR-044, SC-004)");
  const std::vector<std::string> expectedNames = {name, name + "/threads:2"};
  if (names != expectedNames) {
    report("a listing of a two-count thread list names the bare family and "
           "the /threads:2 instance (FR-048, SC-004)",
           name + " | " + name + "/threads:2",
           joinedLines(names));
  }

  const std::string filter = "^" + name;
  const Run run = runEntry({"--threads=1,2",
                            "--iterations=1",
                            "--no-percentiles",
                            "--filter",
                            filter.c_str()});
  check(run.status == 0,
        "a run of a two-count thread list answers status 0 (FR-044, SC-004)");
  const std::vector<std::string> rows = namingLines(linesOf(run.out), name);
  if (rows.size() != 2) {
    report("a run of a two-count thread list prints one report row per count "
           "(FR-044, SC-004)",
           "2 lines naming " + name,
           joinedLines(rows));
    return;
  }
  check(rows[0].find("/threads:") == std::string::npos,
        "the row of the one-count name carries no /threads: segment "
        "(FR-048, SC-004)");
  check(rows[1].find("/threads:2") != std::string::npos,
        "the row of the two-count instance carries the /threads:2 segment "
        "(FR-048, SC-004)");
}

// T063: a row that takes no value and is handed one keeps the line the
// base answers. The scanner refuses the word with `optopt` holding the
// row's own value, so the letter line would print a control byte, and the
// contract fixes no other text for the case (FR-007, SC-003).
auto aFlagGivenAValueKeepsTheBaseLine() -> void
{
  const Run run = runEntry({"--help=1"});
  check(run.status == 2,
        "a flag handed a value answers status 2 (FR-007, SC-003)");
  const std::vector<std::string> printed = linesOf(run.out);
  check(printed.empty(),
        "a flag handed a value prints nothing on standard output (SC-003)");
  const std::vector<std::string> faults = linesOf(run.err);
  const std::string expected =
      "unknown option; see the option table of "
      "specs/015-benchmark-harness-core/contracts/cli.md";
  if (faults.size() != 1 || faults.front() != expected) {
    report("a flag handed a value answers the line the base answers (T063)",
           expected,
           faults.empty() ? "no line on standard error" : faults.front());
  }
}

auto main(int argc, char** argv) -> int
{
  const auto directory = tableDirectory(argc > 1 ? argv[1] : nullptr);
  if (!directory) {
    fail("the table source can be read (FR-079)");
    std::printf("%d checks failed\n", failures);
    return 1;
  }

  const std::string header = readText(*directory / "detail/options.hpp");
  const std::string table = readText(*directory / "options.cpp");
  const std::string scan = readText(*directory / "cli.cpp");

  Constants constants;
  collectConstants(header, constants);
  collectConstants(table, constants);
  collectConstants(scan, constants);

  std::vector<Row> rows = parseRows(table, constants);
  const std::vector<Row> scanRows = parseRows(scan, constants);
  if (scanRows.size() > rows.size()) {
    rows = scanRows;
  }

  std::string optionText = scanOptionString(scan);
  if (optionText.empty()) {
    optionText = scanOptionString(table);
  }

  theTableAnswersARowList(rows);
  theTableHoldsThirtyFiveRows(rows);
  theNamesStandInTheContractOrder(rows);
  eachRowCarriesItsArgumentKind(rows);
  eachRowCarriesItsValue(rows);
  theBaseOptionsKeepTheirMasterValues(rows);
  theSevenNewNamesTakeTheReservedConstants(rows);
  noNameAndNoValueRepeat(rows);
  theValueBlocksStayUniqueAndDisjoint(rows);
  theHuntNamesTakeTheReservedConstants(rows);
  theFixtureTwinReportsItsPlantedDefects();
  theCarriedConstantsNameTheirOptions(rows);
  theScanCarriesNoShortLetter(optionText);

  const std::string version = projectVersion(*directory);
  check(!version.empty(),
        "the project version of the build can be read (FR-016)");

  theHelpAnswerPrintsOneLinePerRow();
  theVersionAnswerPrintsTheProjectVersion(version);
  helpAndVersionAnswerStatusZero();
  theWalkAnswersTheFirstEventItMeets();
  theRulesOptionShowsInTheHelpAnswer();
  theRulesOptionShowsOnTheGuidePage(*directory);
  thePageAndTheTableAgreeInBothDirections(*directory);
  aFlagGivenAValueKeepsTheBaseLine();
  aTwoCountThreadListRunsAtBothCounts();

  std::printf(failures == 0 ? "ok\n" : "%d checks failed\n", failures);
  return failures == 0 ? 0 : 1;
}
