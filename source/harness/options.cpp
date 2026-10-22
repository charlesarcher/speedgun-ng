#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <expected>
#include <fstream>
#include <iterator>
#include <limits>
#include <numeric>
#include <optional>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "detail/options.hpp"

#include <getopt.h>

#include "detail/compare.hpp"
#include "speedgun-ng/dbc.hpp"

/**
 * @file options.cpp
 * @brief The one option table of specs/028-full-command-line and the scan
 * that walks it: the 27 rows `detail::optionRows()` answers in the order
 * `specs/028-full-command-line/contracts/option-table.md` fixes, the
 * `getopt_long` array built from those rows, the walk of one command
 * line that `detail::scanCommandLine` answers, and the two answers that
 * stop that walk, printed by `detail::printHelp` and
 * `detail::printVersion` (FR-001, FR-002, FR-003, FR-005, FR-010,
 * FR-015, FR-016, FR-079).
 *
 * One reader serves each value kind, so an option carries no parse of its
 * own and the environment and the option file reach the same text under
 * the same rule as the command line (FR-024, FR-025, FR-027, D-11).
 *
 * The row is the only carrier of an option's internal value: the scan
 * switches on what the row names, and a reader reaches the same rows from
 * this source text (FR-075, FR-079). `speedgunMain` calls the scan and
 * holds no option list of its own, so the table here is the one list an
 * option is recognized from (FR-002).
 */

namespace sg
{
namespace detail
{
namespace
{

/**
 * @brief The `getopt` argument kind that answers one row's kind
 * (FR-001, FR-005).
 */
auto getoptArgumentKind(const OptionArgument argument) -> int
{
  if (argument == OptionArgument::REQUIRED) {
    return required_argument;
  }
  if (argument == OptionArgument::OPTIONAL) {
    return optional_argument;
  }
  return no_argument;
}

/**
 * @brief The `getopt_long` array of the scan: one entry per table row in
 * table order, each the row's name, its argument kind, a null flag and
 * the row's internal value, closed by the all-zero entry `getopt`
 * requires (FR-002, FR-005).
 *
 * The array is built once: the rows are fixed at link time and the
 * scanner reads the entries without changing them. The scanner reports no
 * diagnostic of its own: `opterr` stands at 0, so the four usage fault
 * lines of the contract are the only fault text the run answers (FR-006,
 * FR-007 to FR-009).
 */
auto getoptOptionArray() -> const std::vector<option>&
{
  static const std::vector<option> kEntries = []
  {
    const std::span<const OptionRow> rows = optionRows();
    std::vector<option> built;
    built.reserve(rows.size() + 1);
    for (const OptionRow& row : rows) {
      built.push_back(option {
          row.name, getoptArgumentKind(row.argument), nullptr, row.value});
    }
    built.push_back(option {nullptr, 0, nullptr, 0});
    return built;
  }();
  return kEntries;
}

// The largest whole number of seconds whose nanoseconds fit the signed
// count the run control carries.
constexpr std::int64_t kMaxWholeSeconds =
    std::numeric_limits<std::int64_t>::max() / 1'000'000'000;

// FR-034: an invalid value is a usage fault the scan answers, and the
// entry point ends the run without running anything. The signed read is
// deliberate: `strtoull` wraps `-5` to a huge count, so a negative count
// would parse, run, and hang. The signed read turns it into a usage
// fault (T044).
// specs/021 FR-021, FR-022: the point list of `--percentiles=`. One
// element is optional spaces, one or more digits, an optional point with
// digits after it, and optional spaces; it stands in the open interval
// (0, 100). A list holds at least one element and at most
// kMaxPercentilePoints of them. The shape is checked by hand rather than
// left to `strtod`, which reads `.5` and `50.` as numbers and a trailing
// space as the end of the element.
constexpr std::size_t kMaxPercentilePoints = 16;

/**
 * @brief The text of a value as one fault record carries it.
 *
 * A value of the command line or of the environment can hold a line break,
 * and the fault of any source is one line (FR-024, FR-027, FR-035, SC-003),
 * so a break is named with its own words. A backslash stands doubled, so a
 * named break cannot be confused with the two letters of its own name.
 */
auto quotedText(const std::string_view text) -> std::string
{
  std::string quoted;
  quoted.reserve(text.size());
  for (const char letter : text) {
    if (letter == '\\') {
      quoted += "\\\\";
    } else if (letter == '\n') {
      quoted += "\\n";
    } else {
      quoted.push_back(letter);
    }
  }
  return quoted;
}

auto parsePercentileList(const char* text,
                         const char* option,
                         std::vector<double>& points)
    -> std::expected<void, std::string>
{
  const char* at = text;
  bool more = true;
  while (more) {
    while (std::isspace(static_cast<unsigned char>(*at)) != 0) {
      ++at;
    }
    const char* const begin = at;
    while (std::isdigit(static_cast<unsigned char>(*at)) != 0) {
      ++at;
    }
    const bool hasInteger = at != begin;
    bool shaped = true;
    if (*at == '.') {
      ++at;
      const char* const afterPoint = at;
      while (std::isdigit(static_cast<unsigned char>(*at)) != 0) {
        ++at;
      }
      shaped = at != afterPoint;
    }
    while (std::isspace(static_cast<unsigned char>(*at)) != 0) {
      ++at;
    }
    points.push_back(std::strtod(begin, nullptr));
    const double point = points.back();
    if (!shaped || !hasInteger || (*at != '\0' && *at != ',') || point <= 0.0
        || point >= 100.0 || points.size() > kMaxPercentilePoints)
    {
      return std::unexpected(std::string(option)
                             + ": expected comma-separated points in "
                               "(0, 100), got '"
                             + quotedText(text) + "'");
    }
    more = *at == ',';
    if (more) {
      ++at;
    }
  }
  return {};
}

auto parseCount(const char* text,
                const char* option) -> std::expected<std::uint64_t, std::string>
{
  errno = 0;
  char* end = nullptr;
  const long long value = std::strtoll(text, &end, 10);
  if (errno != 0 || end == text || *end != '\0' || value <= 0) {
    return std::unexpected(std::string(option)
                           + ": expected a positive integer, got '"
                           + quotedText(text) + "'");
  }
  return static_cast<std::uint64_t>(value);
}

// The two duration options share one reader of the seconds kind. The
// argument names whether zero stands: FR-034 with the option table of
// contracts/cli.md gives `--min-time` a positive count of seconds, where
// `--warmup-time` admits zero. The words the fault line carries follow the
// same argument, so each option keeps the line the base prints. The argument
// is a two-word enumeration, so a call names its shape and no reader takes
// a second bool (FR-034).
enum class ZeroWord
{
  ADMITTED,
  REFUSED
};
constexpr auto kZeroAdmitted = ZeroWord::ADMITTED;
constexpr auto kZeroRefused = ZeroWord::REFUSED;

auto parseSeconds(const char* text, const char* option, const ZeroWord zeroWord)
    -> std::expected<std::int64_t, std::string>
{
  errno = 0;
  char* end = nullptr;
  const double seconds = std::strtod(text, &end);
  const bool zeroAdmitted = zeroWord == kZeroAdmitted;
  // The sign the option asks for: `--warmup-time` admits zero, `--min-time`
  // asks above it. A reading that is no number at all fails the finite test
  // below.
  const bool signHolds = zeroAdmitted ? seconds >= 0.0 : seconds > 0.0;
  if (errno != 0 || end == text || *end != '\0' || !std::isfinite(seconds)
      || seconds > static_cast<double>(kMaxWholeSeconds) || !signHolds)
  {
    const std::string_view qualifier =
        zeroAdmitted ? "number" : "positive number";
    return std::unexpected(std::string(option) + ": expected a finite "
                           + std::string(qualifier) + " of seconds at most "
                           + std::to_string(kMaxWholeSeconds) + ", got '"
                           + quotedText(text) + "'");
  }
  return static_cast<std::int64_t>(seconds * 1'000'000'000.0);
}

// specs/024 FR-023, FR-026, research D-09: the word of `--metric-method`, read
// against the three words of contracts/metric-method-option.md. The comparison
// is exact and case-sensitive, so `Auto` is a fourth word, and the reader
// answers with nothing for one of those: the entry point owns the report, so
// that a refusal returns the usage status and the entry point keeps the
// process exit.
auto parseMetricMethod(const char* text) -> std::optional<MetricMethod>
{
  const std::string_view word {text};
  if (word == "auto") {
    return MetricMethod::AUTO;
  }
  if (word == "passes") {
    return MetricMethod::PASSES;
  }
  if (word == "multiplex") {
    return MetricMethod::MULTIPLEX;
  }
  return std::nullopt;
}

// The content the `--json-raw` row names, and the line that refuses any
// other word. The scan and the settle function of the row read the word
// through here, so the two words are written once (FR-034, FR-035, FR-050).
auto parseRawContent(const std::string_view text)
    -> std::expected<RawContent, std::string>
{
  if (text == "chunks") {
    return RawContent::CHUNKS;
  }
  if (text == "none") {
    return RawContent::NONE;
  }
  return std::unexpected(
      "--json-raw: 'chunks' and 'none' are the values the " "option names");
}

// The pair of report paths of the `--compare` row, split at the first comma.
// A missing comma and an empty side are refused here, before the mode could
// open a document, and the text the line echoes carries the quoting of the
// refusal helper (FR-048, FR-052, spec D-16).
auto parseComparePair(const std::string_view text)
    -> std::expected<std::pair<std::string, std::string>, std::string>
{
  const std::size_t comma = text.find(',');
  if (comma == std::string_view::npos || comma == 0 || comma + 1 == text.size())
  {
    return std::unexpected("--compare: expected '<baseline>,<contender>', got '"
                           + quotedText(text) + "'");
  }
  return std::pair {std::string(text.substr(0, comma)),
                    std::string(text.substr(comma + 1))};
}

// The one line of the `--json` row whose word stands empty, read the same way
// by the scan and by the settle function of the row (FR-030, FR-035).
constexpr const char* kEmptyPathFault =
    "--json: the path the option names stands empty";

// One whole number of a `--threads` item: `from_chars` reads the digits of
// the item whole, and a text with a stray character, an empty item or a
// number past what an `int` holds names no count (FR-045).
auto readItemCount(const std::string_view item) -> std::optional<int>
{
  int count = 0;
  const auto answer =
      std::from_chars(item.data(), item.data() + item.size(), count);
  if (answer.ec != std::errc() || answer.ptr != item.data() + item.size()) {
    return std::nullopt;
  }
  return count;
}

// The four usage fault lines that
// `specs/028-full-command-line/contracts/option-table.md` fixes, each one
// line the entry point prints at status 2 (FR-007 to FR-009, SC-003). The
// scan builds them from the row list and from the word the scanner
// stopped on, and writes nothing itself.

// The largest edit distance that earns a suggestion of a row name
// (FR-007).
constexpr std::size_t kSuggestionDistance = 2;

// The edit distance of two names: the row name of least distance is the
// one the unknown line suggests, and only when that distance stands at or
// below two (FR-007).
auto editDistance(std::string_view left, std::string_view right) -> std::size_t
{
  std::vector<std::size_t> row(right.size() + 1);
  for (std::size_t column = 0; column < row.size(); ++column) {
    row[column] = column;
  }
  for (std::size_t line = 1; line <= left.size(); ++line) {
    std::size_t corner = row[0];
    row[0] = line;
    for (std::size_t column = 1; column < row.size(); ++column) {
      const std::size_t above = row[column];
      const std::size_t substitution = corner
          + static_cast<std::size_t>(left[line - 1] != right[column - 1]);
      row[column] = std::min({substitution, above + 1, row[column - 1] + 1});
      corner = above;
    }
  }
  return row.back();
}

// The word as the operator typed it, up to an attached value: the value
// holds no part of the name the fault line names (FR-007, FR-009).
auto typedWord(std::string_view word) -> std::string_view
{
  return word.substr(0, word.find('='));
}

// The word that carries the letter a short group was refused on. glibc
// leaves `optind` standing on a group it stopped inside, so the word behind
// it names an earlier option there and the group itself stands one word
// ahead; a letter a long row answers with belongs to no single-dash word,
// so the search answers nothing and the base line stays (FR-007, T063).
auto groupWord(int argc,
               char** argv,
               int optind,
               char letter) -> std::string_view
{
  const std::string head {'-', letter};
  for (const int index : {optind - 1, optind}) {
    if (index > 0 && index < argc) {
      const std::string_view word = typedWord(argv[index]);
      if (word.starts_with(head)) {
        return word;
      }
    }
  }
  return {};
}

// The name a typed word carries: the leading dashes and an attached value
// hold no part of it (FR-007, FR-009).
auto typedName(std::string_view word) -> std::string_view
{
  const std::string_view head = typedWord(word);
  const std::size_t letters = head.find_first_not_of('-');
  if (letters == std::string_view::npos) {
    return {};
  }
  return head.substr(letters);
}

// The line the base answers for a word the scanner refuses and no rule
// above can name: kept so a base command line keeps its text and its
// status 2 (C-1).
auto baseUnknownLine() -> std::string
{
  return "unknown option; see the option table of "
         "specs/015-benchmark-harness-core/contracts/cli.md";
}

// The unknown line: the typed text, and the nearest row name when the
// edit distance to it stands at or below two (FR-007).
auto unknownOptionLine(std::string_view word) -> std::string
{
  const std::string_view name = typedName(word);
  std::string_view nearest;
  std::size_t best = kSuggestionDistance + 1;
  for (const OptionRow& row : optionRows()) {
    const std::size_t distance = editDistance(name, row.name);
    if (distance < best) {
      best = distance;
      nearest = row.name;
    }
  }
  if (best > kSuggestionDistance) {
    return "unknown option '" + std::string(word) + "'; --help lists the "
           "options";
  }
  return "unknown option '" + std::string(word) + "', nearest '--"
      + std::string(nearest) + "'";
}

// The ambiguous line, when the typed name starts two or more row names:
// the candidates in table order, which the scan computes itself because
// the scanner names no list once `opterr` stands at 0 (FR-009, D-25).
auto ambiguousOptionLine(std::string_view word,
                         std::string_view name) -> std::optional<std::string>
{
  if (name.empty()) {
    // A word of dashes alone carries no name, so no row of the table is a
    // candidate for it and the unknown-option line stands (FR-007, FR-009).
    return std::nullopt;
  }
  std::string candidates;
  std::size_t count = 0;
  for (const OptionRow& row : optionRows()) {
    if (!std::string_view(row.name).starts_with(name)) {
      continue;
    }
    if (count++ != 0) {
      candidates += ", ";
    }
    candidates += row.name;
  }
  if (count < 2) {
    return std::nullopt;
  }
  return "ambiguous option '" + std::string(word) + "': " + candidates;
}

// The needs-a-value line names the row the scanner stopped on: the
// scanner reports that row's value through `optopt`, and the row list is
// the only place a name comes from (FR-008).
auto missingValueLine(int value) -> std::string
{
  const std::span<const OptionRow> rows = optionRows();
  const auto found = std::ranges::find(rows, value, &OptionRow::value);
  SG_ASSERT(
      found != rows.end(),
      "the scanner reports a value only for a row it built from the " "table");
  return std::string("option '--") + found->name + "' needs a value";
}

// The argument part of one help line, from the row's own kind: ` <value>`
// for a required value, ` [<value>]` for an optional one, nothing for
// none (FR-015, contracts/help-and-version.md).
auto argumentText(const OptionArgument argument) -> const char*
{
  if (argument == OptionArgument::REQUIRED) {
    return " <value>";
  }
  if (argument == OptionArgument::OPTIONAL) {
    return " [<value>]";
  }
  return "";
}

}  // namespace

/**
 * @brief The one option table: the 27 rows in the order the contract
 * fixes them, each with its name, internal value, argument kind,
 * environment flag, help text and owning requirement (FR-001, FR-002,
 * FR-003, FR-004, FR-079, SC-001).
 *
 * The 20 rows the master carries keep their spelling and their internal
 * value, the four constants 256 to 259 among them (FR-003, PC-1). The
 * seven names this feature brings take the constants 290 to 296 that
 * spec D-24 reserves (FR-004). The letters of the master rows are switch
 * codes and never short options: the scan's option string carries none
 * (FR-005). Exactly the 12 rows FR-020 names carry the environment flag,
 * and the name of such a row's variable follows from the row (FR-019).
 */
auto optionRows() -> std::span<const OptionRow>
{
  static constexpr auto kRows = std::to_array<OptionRow>({
      {"filter",
       'f',
       OptionArgument::REQUIRED,
       true,
       "Selects the instances whose names match",
       "specs/015"},
      {"list",
       'l',
       OptionArgument::NONE,
       false,
       "Prints the registry and stops",
       "specs/015"},
      {"repetitions",
       'r',
       OptionArgument::REQUIRED,
       true,
       "Sets how many times one instance runs",
       "specs/015"},
      {"min-time",
       'm',
       OptionArgument::REQUIRED,
       true,
       "Sets the least measuring seconds of one instance",
       "specs/015"},
      {"iterations",
       'i',
       OptionArgument::REQUIRED,
       true,
       "Fixes the iteration count and skips calibration",
       "specs/015"},
      {"warmup-time",
       'w',
       OptionArgument::REQUIRED,
       true,
       "Sets the untimed warm-up seconds",
       "specs/015"},
      {"dry-run",
       'd',
       OptionArgument::NONE,
       false,
       "Runs one iteration and prints the row shape",
       "specs/015"},
      {"counter",
       'c',
       OptionArgument::REQUIRED,
       true,
       "Adds one catalog counter to the report columns",
       "specs/015"},
      {"metric",
       'M',
       OptionArgument::REQUIRED,
       true,
       "Adds one metric of the host's table to the report columns",
       "specs/015"},
      {"catalog",
       'C',
       OptionArgument::NONE,
       false,
       "Prints the counter catalog and stops",
       "specs/015"},
      {"percentiles",
       'p',
       OptionArgument::REQUIRED,
       true,
       "Sets the percentile points one run reports",
       "specs/015"},
      {"no-percentiles",
       'P',
       OptionArgument::NONE,
       true,
       "Turns the percentile lines off",
       "specs/015"},
      {"placement",
       'A',
       OptionArgument::REQUIRED,
       true,
       "Selects the preset that plans the run's cpus",
       "specs/024"},
      {"topology",
       'T',
       OptionArgument::NONE,
       false,
       "Prints the machine layout listing and stops",
       "specs/024"},
      {"json",
       'j',
       OptionArgument::REQUIRED,
       false,
       "Writes one JSON report document for the run",
       "specs/015"},
      {"json-raw",
       'J',
       OptionArgument::REQUIRED,
       false,
       "Selects the chunk columns the document carries",
       "specs/023"},
      {"compare",
       kCompareOptionValue,
       OptionArgument::REQUIRED,
       false,
       "Compares two report documents; the mode runs no benchmark",
       "specs/026"},
      {"compare-rule",
       kCompareRuleOptionValue,
       OptionArgument::REQUIRED,
       false,
       "Adds one comparison rule, repeatable in order",
       "specs/026"},
      {"compare-strict",
       kCompareStrictOptionValue,
       OptionArgument::NONE,
       false,
       "Makes every noisy row fail the verdict",
       "specs/026"},
      {"metric-method",
       kMetricMethodOption,
       OptionArgument::REQUIRED,
       false,
       "Sets the metric collection method to auto, passes or multiplex",
       "specs/025"},
      {"options",
       kOptionsOptionValue,
       OptionArgument::REQUIRED,
       false,
       "Reads one option per line from a file",
       "FR-029"},
      {"env",
       kEnvOptionValue,
       OptionArgument::REQUIRED,
       false,
       "Takes off so the process environment sets nothing",
       "FR-028"},
      {"threads",
       kThreadsOptionValue,
       OptionArgument::REQUIRED,
       true,
       "Sets the thread counts of every family",
       "FR-044"},
      {"interleave",
       kInterleaveOptionValue,
       OptionArgument::OPTIONAL,
       true,
       "Runs the selected instances in a seeded random order",
       "FR-051"},
      {"compare-rules",
       kCompareRulesOptionValue,
       OptionArgument::REQUIRED,
       false,
       "Reads one comparison rule per line from a file",
       "FR-061"},
      {"help",
       kHelpOptionValue,
       OptionArgument::NONE,
       false,
       "Prints one line per option and stops",
       "FR-015"},
      {"version",
       kVersionOptionValue,
       OptionArgument::NONE,
       false,
       "Prints the project version and stops",
       "FR-016"},
      // specs/029-hunter-core FR-080 to FR-082, D-22: the hunt family. No hunt
      // row carries an environment name, so the gather refuses each of them
      // from a file or the environment and the command line owns the family.
      // `--hunt` is the table's first optional_argument row, and `getopt_long`
      // reads an optional argument only in the attached form, so `--hunt=all`
      // selects and `--hunt all` selects nothing and leaves `all` positional.
      {"hunt",
       kHuntOption,
       OptionArgument::OPTIONAL,
       false,
       "Hunts the selected catalog events and ranks them",
       "specs/029"},
      {"hunt-norm",
       kHuntNormOption,
       OptionArgument::REQUIRED,
       false,
       "Sets the hunt score normaliser: instr, cycle or iter",
       "specs/029"},
      {"hunt-rank",
       kHuntRankOption,
       OptionArgument::REQUIRED,
       false,
       "Sets the hunt ranking: rate or cv",
       "specs/029"},
      {"hunt-top",
       kHuntTopOption,
       OptionArgument::REQUIRED,
       false,
       "Sets how many ranked events the hunt prints",
       "specs/029"},
      {"hunt-noise",
       kHuntNoiseOption,
       OptionArgument::REQUIRED,
       false,
       "Sets the hunt noise threshold as a percent CV",
       "specs/029"},
      {"hunt-repeats",
       kHuntRepeatsOption,
       OptionArgument::REQUIRED,
       false,
       "Sets how many repeats one hunt event takes",
       "specs/029"},
      {"hunt-budget",
       kHuntBudgetOption,
       OptionArgument::REQUIRED,
       false,
       "Sets the hunt budget in seconds or passes",
       "specs/029"},
      {"hunt-seed",
       kHuntSeedOption,
       OptionArgument::REQUIRED,
       false,
       "Sets the seed the hunt repeats draw from",
       "specs/029"},
  });
  return kRows;
}

/**
 * @brief Scan one command line into the options its words name (FR-002,
 * FR-009 to FR-012, D-03, D-06).
 *
 * The walk hands the scanner the array built from the table rows, so a
 * name is recognized from a row and from nothing else (FR-002). The
 * scanner keeps process state and a suite can call the entry point more
 * than once, so the walk restarts `optind` here the way the base scan did
 * and stands `opterr` at 0 with the option string `":"`, so the scanner
 * reports no diagnostic and the scan names every fault itself (FR-006,
 * FR-010).
 *
 * A rejected word answers one of the four usage fault lines that
 * `specs/028-full-command-line/contracts/option-table.md` fixes, and a
 * rejected value answers the line the base prints for it. Each is the
 * fault text the entry point prints at status 2: the scan writes nothing,
 * calls no `std::exit` and answers no status (FR-010, FR-011, SC-013).
 */
auto scanCommandLine(int argc, char** argv)
    -> std::expected<CommandLineOptions, std::string>
{
  SG_REQUIRE(argc >= 0 && argv != nullptr,
             "the scan takes the argc and argv the process hands main");
  CommandLineOptions scan;
  RunOptions& options = scan.options;
  const std::vector<option>& entries = getoptOptionArray();

  int choice = 0;
  bool optionsSeen = false;
  opterr = 0;
  optind = 0;
  while ((choice = getopt_long(argc, argv, ":", entries.data(), nullptr)) != -1)
  {
    switch (choice) {
      case 'f':
        options.filter = optarg;
        break;
      case 'l':
        options.listMode = true;
        break;
      case 'r':
        if (const auto count = parseCount(optarg, "--repetitions")) {
          options.repetitions = *count;
        } else {
          return std::unexpected(count.error());
        }
        break;
      case 'm':
        if (const auto seconds =
                parseSeconds(optarg, "--min-time", kZeroRefused))
        {
          options.minTimeNs = *seconds;
        } else {
          return std::unexpected(seconds.error());
        }
        break;
      case 'i':
        if (const auto count = parseCount(optarg, "--iterations")) {
          options.fixedIterations = *count;
        } else {
          return std::unexpected(count.error());
        }
        break;
      case 'w':
        if (const auto seconds =
                parseSeconds(optarg, "--warmup-time", kZeroAdmitted))
        {
          options.warmupTimeNs = *seconds;
        } else {
          return std::unexpected(seconds.error());
        }
        break;
      case 'd':
        options.dryRun = true;
        break;
      case 'c':
        options.counterAddresses.emplace_back(optarg);
        break;
      case 'M':
        options.metricNames.emplace_back(optarg);
        break;
      case 'C':
        options.catalogMode = true;
        break;
      case 'p': {
        // specs/021 FR-020, FR-021: the command line wins over the family.
        options.showPercentiles = true;
        const auto points = parsePercentileList(
            optarg, "--percentiles", options.percentilePoints);
        if (!points.has_value()) {
          return std::unexpected(points.error());
        }
        break;
      }
      case 'P':
        options.showPercentiles = false;
        break;
      case 'A':
        // specs/025 FR-016: the text stands raw here; the entry point
        // parses it once before the first run.
        options.placement = std::string(optarg);
        break;
      case 'T':
        options.topologyMode = true;
        break;
      case 'j':
        // FR-030: an empty path is a usage error, and it reports before
        // anything runs.
        if (std::string_view(optarg).empty()) {
          return std::unexpected(kEmptyPathFault);
        }
        options.jsonPath = optarg;
        break;
      case 'J': {
        // FR-029: `chunks` is the one content the option names besides
        // `none`; any other word is a usage error (FR-034). The row has no
        // unset word, so the name itself is what the gather reads to tell a
        // word of the command line from the default (FR-037, FR-050).
        const auto content = parseRawContent(optarg);
        if (!content.has_value()) {
          return std::unexpected(content.error());
        }
        options.rawContent = *content;
        scan.rawContentNamed = true;
        break;
      }
      case kMetricMethodOption: {
        // specs/024 FR-025, FR-026: the word is the method the run is asked
        // for and wins over the family's own request the way `--percentiles`
        // wins over `showPercentiles`; the assignment overwrites, so a
        // repeated flag keeps its last word. A word outside the three ends
        // the run here, before the expansion, the listing and the first
        // measurement, so a typo costs no measured time.
        const auto method = parseMetricMethod(optarg);
        if (!method.has_value()) {
          return std::unexpected("--metric-method '" + std::string(optarg)
                                 + "' is not auto, passes or multiplex");
        }
        options.metricMethod = method;
        break;
      }
      case kCompareOptionValue: {
        // specs/026 FR-048: the text is the two report paths split at the
        // first comma. A missing comma or an empty side is a usage fault:
        // one line naming the option and the text read, status 2 (FR-051).
        const auto pair = parseComparePair(optarg);
        if (!pair.has_value()) {
          return std::unexpected(pair.error());
        }
        options.compareBaseline = pair->first;
        options.compareContender = pair->second;
        break;
      }
      case kCompareRuleOptionValue: {
        // specs/026 FR-049: one rule per option, in the order given. The
        // parse call names the field of its fault and the text it read, so
        // the line below adds the option and nothing else (FR-051).
        const auto rule = parseCompareRule(optarg);
        if (!rule.has_value()) {
          return std::unexpected("--compare-rule: " + rule.error());
        }
        options.compareRules.push_back(std::move(*rule));
        break;
      }
      case kCompareStrictOptionValue:
        // specs/026 FR-045: a NOISY row fails the run; the state word
        // stands as printed.
        options.compareStrict = true;
        break;
      case kCompareRulesOptionValue:
        // FR-061, D-24: the word names a file of rules, one per line. The
        // scan keeps the path alone, since the read belongs to the reader
        // the entry point hands the gather (D-05). The row takes no repeat,
        // so a second word keeps its own path, the way a scalar option
        // keeps its last word (FR-035).
        options.compareRulesFile = optarg;
        break;
      case kOptionsOptionValue:
        // FR-032, spec D-9: one option file per run. The second word
        // answers one line naming the option alone, with no path repeated.
        // The path reaches `RunOptions` (E-03) and the entry point owns the
        // read, so the gather keeps opening nothing (D-05).
        if (optionsSeen) {
          return std::unexpected("--options cannot stand twice");
        }
        optionsSeen = true;
        options.optionsFile = optarg;
        break;
      case kEnvOptionValue:
        // FR-028, D-04: `off` is the only word the option takes. It settles
        // the run with no environment source, so the entry point hands the
        // gather no rows at all and the process names stand unread. Any
        // other word is a usage fault naming the option and the word as
        // typed, and the entry point prints it at status 2 (SC-003).
        if (std::string_view(optarg) != "off") {
          return std::unexpected("--env: expected 'off', got '"
                                 + quotedText(optarg) + "'");
        }
        options.environmentOff = true;
        break;
      case kThreadsOptionValue: {
        // specs/028 FR-044, FR-045: the text reaches the reader of the
        // list, and a repeated option appends the way `-c` appends its
        // addresses, so two words state the union of their counts. The
        // refusal names the option and the text as typed, and the entry
        // point prints it at status 2 (FR-046).
        const auto counts = readThreadList(optarg);
        if (!counts.has_value()) {
          return std::unexpected(counts.error());
        }
        options.threadCounts.insert(
            options.threadCounts.end(), counts->begin(), counts->end());
        break;
      }
      case kInterleaveOptionValue: {
        // FR-051, FR-052: the row's argument kind is optional, so the bare
        // word stands given with seed 0 and claims nothing from behind it.
        // The attached text goes to the reader of the seed, whose line names
        // the option and the text it refused (FR-054). The last word of
        // several keeps its seed, the master answer of one command line.
        if (optarg == nullptr) {
          scan.interleave = InterleaveSeed {true, 0};
          break;
        }
        const auto seed = readSeed(optarg);
        if (!seed.has_value()) {
          return std::unexpected(seed.error());
        }
        scan.interleave = InterleaveSeed {true, *seed};
        break;
      }
      case kHelpOptionValue:
        // FR-017: the walk answers the first of the two print options or
        // a usage fault that it meets, so the walk ends here and reads
        // no word that stands behind the one it met. The entry point
        // owns the printing and the status.
        scan.stop = StopKind::HELP;
        return scan;
      case kVersionOptionValue:
        scan.stop = StopKind::VERSION;
        return scan;
      // specs/029-hunter-core FR-080 to FR-082, PD-17: one call per hunt row,
      // each body standing in `hunt_cli.cpp` with its own refusal line.
      case kHuntOption:
        // specs/029 FR-080, D-1: no value means the whole core catalog.
        parseHuntSelection(optarg, options);
        break;
      case kHuntNormOption:
        parseHuntNorm(optarg, options);
        break;
      case kHuntRankOption:
        parseHuntRank(optarg, options);
        break;
      case kHuntTopOption:
        parseHuntTop(optarg, options);
        break;
      case kHuntNoiseOption:
        parseHuntNoise(optarg, options);
        break;
      case kHuntRepeatsOption:
        parseHuntRepeats(optarg, options);
        break;
      case kHuntBudgetOption:
        parseHuntBudget(optarg, options);
        break;
      case kHuntSeedOption:
        parseHuntSeed(optarg, options);
        break;
      case ':':
        return std::unexpected(missingValueLine(optopt));
      default: {
        // The word getopt_long could not place stands in this arm: every
        // row value of the table and the missing-value marker carry an arm
        // above, so the value that reaches here names no arm by
        // construction, and the scanner reports only that the word is bad.
        // A single dash letter arrives through `optopt`, and the option
        // string carries no letter, so every letter names no row (FR-005,
        // FR-007). Only a long name can start a row name, so the candidate
        // list belongs to it alone (FR-006, FR-009).
        if (optopt != 0
            && !groupWord(argc, argv, optind, static_cast<char>(optopt))
                    .empty())
        {
          return std::unexpected(unknownOptionLine(
              std::string(1, '-') + static_cast<char>(optopt)));
        }
        const std::string_view word = typedWord(argv[optind - 1]);
        // A row that takes no value answers '?' with `optopt` holding that
        // row's value, so a word naming a row that the scanner still
        // refused keeps the line the base answers: the contract fixes no
        // text for it, and the status stays 2 (T063).
        const std::string_view name = typedName(word);
        for (const OptionRow& row : optionRows()) {
          if (row.name == name) {
            return std::unexpected(baseUnknownLine());
          }
        }
        if (const auto ambiguous = ambiguousOptionLine(word, name)) {
          return std::unexpected(*ambiguous);
        }
        return std::unexpected(unknownOptionLine(word));
      }
    }
  }

  SG_ENSURE(scan.stop == StopKind::NONE,
            "a scan that walked the whole line names no stop");
  return scan;
}

/**
 * @brief Print the help answer: one line per table row in table order,
 * each built from that row's `name`, its argument kind and its `help`
 * text, so a row added to the table appears here with no other edit
 * (FR-015, FR-018).
 *
 * The entry point calls this when the scan answers `StopKind::HELP`. The
 * printer writes on standard output, calls no `std::exit` and answers no
 * status.
 */
auto printHelp() -> void
{
  for (const OptionRow& row : optionRows()) {
    std::printf(
        "  --%s%s  %s\n", row.name, argumentText(row.argument), row.help);
  }
}

/**
 * @brief Print the version answer: one line naming the project and the
 * version the build hands the harness as `SG_PROJECT_VERSION`, the same
 * text the console report carries (FR-016).
 *
 * The entry point calls this when the scan answers `StopKind::VERSION`.
 * The printer writes on standard output, calls no `std::exit` and
 * answers no status.
 */
auto printVersion() -> void
{
  std::printf("speedgun-ng %s\n", SG_PROJECT_VERSION);
}

// The readers of the value kinds the three sources share (D-11). Each
// answers the value it read, or one line naming the rejected text. The
// four usage lines of FR-007 and FR-008 name a rejected name, and a reader
// answers what stands after a name the table carries, so the two sets of
// lines stay apart (FR-024, FR-025, FR-054).

/**
 * @brief The flag value of one option read from the environment or from
 * an option file: `1`, `0`, `true` and `false`, read exactly as spelled
 * (FR-025, D-11).
 *
 * The refusal names the rejected text. The source names itself ahead of
 * that line: the gather writes `SPEEDGUN_NO_PERCENTILES: ` before it, and
 * an option-file line writes the path and the line number (FR-027,
 * FR-035).
 */
auto readFlag(const std::string_view text) -> std::expected<bool, std::string>
{
  if (text == "1" || text == "true") {
    return true;
  }
  if (text == "0" || text == "false") {
    return false;
  }
  return std::unexpected("expected 1, 0, true or false, got '"
                         + quotedText(text) + "'");
}

/**
 * @brief The counts of one `--threads=` list: a comma-separated sequence
 * of counts and `min:max` ranges, each range expanded by the doubling of
 * `buildThreadCounts` (FR-044, FR-045, FR-050, E-11).
 *
 * The same reader answers the `SPEEDGUN_THREADS` items that the gather
 * splits on `;` (FR-024). One refusal covers the whole list: the line
 * names the option and the text as the operator typed it, and no count of
 * a refused list reaches the run (FR-046, SC-003).
 */
auto readThreadList(const std::string_view text)
    -> std::expected<std::vector<int>, std::string>
{
  const auto refuse = [text]
  {
    const std::string quoted = quotedText(text);
    const std::string line =
        "--threads: expected counts and min:max ranges, got '" + quoted + "'";
    SG_ENSURE(line.find(quoted) != std::string::npos,
              "the refusal names the text the source spelled (SC-003)");
    return std::unexpected(line);
  };
  std::vector<int> counts;
  for (std::size_t at = 0;;) {
    const std::size_t comma = text.find(',', at);
    const std::string_view item = comma == std::string_view::npos
        ? text.substr(at)
        : text.substr(at, comma - at);
    const std::size_t colon = item.find(':');
    if (colon == std::string_view::npos) {
      const auto count = readItemCount(item);
      if (!count.has_value() || *count < 1) {
        return refuse();
      }
      counts.push_back(*count);
    } else {
      const auto low = readItemCount(item.substr(0, colon));
      const auto high = readItemCount(item.substr(colon + 1));
      if (!low.has_value() || !high.has_value() || *low < 1 || *low > *high) {
        return refuse();
      }
      const std::vector<int> range = buildThreadCounts(*low, *high);
      counts.insert(counts.end(), range.begin(), range.end());
    }
    if (comma == std::string_view::npos) {
      break;
    }
    at = comma + 1;
  }
  return counts;
}

/**
 * @brief The seed of `--interleave=<seed>` or of `SPEEDGUN_INTERLEAVE`:
 * unsigned decimal digits alone, consumed whole, with a value at or below
 * the largest 64-bit unsigned integer (FR-051, FR-054, FR-022, E-10).
 *
 * The bare option carries no text and takes seed 0, so it reaches no
 * reader (FR-051). The refusal names the option and the text the source
 * spelled, which is the line both forms print at status 2 (FR-054,
 * SC-003).
 */
auto readSeed(const std::string_view text)
    -> std::expected<std::uint64_t, std::string>
{
  std::uint64_t seed = 0;
  const auto answer =
      std::from_chars(text.data(), text.data() + text.size(), seed);
  if (answer.ec != std::errc() || answer.ptr != text.data() + text.size()) {
    const std::string line = "--interleave: the seed '" + std::string(text)
        + "' is not an unsigned decimal value";
    SG_ENSURE(line.find(text) != std::string::npos,
              "the refusal names the text the source spelled (SC-003)");
    return std::unexpected(line);
  }
  return seed;
}

// FR-053, D-15, spec D-12: the walk order of one interleaved run.
//
// `std::shuffle` cannot answer this contract: it draws through a uniform
// integer distribution of the standard library's own choosing, and the
// standard pins neither that distribution's bounds nor its drawing order,
// so two standard libraries shuffle the same seed differently. The engine
// below is pinned. `mt19937_64` carries its initialization constants, its
// state and its output transformation in the standard itself, and the
// remainder below is the whole of the randomness the walk draws, so every
// standard library answers one order for one `count` and `seed`.
//
// The draw is the Fisher-Yates walk from the last slot to the second, each
// step exchanging the slot with one drawn from the still-unsettled prefix.
// Two rows of the table `contracts/threads-and-interleave.md` pins, read
// here so a change to the engine cannot pass unnoticed:
//
//     count 12, seed 7: 4 0 2 6 10 9 1 5 11 8 7 3
//     count 12, seed 8: 7 0 6 10 8 2 11 5 9 4 3 1
auto shuffledOrder(const std::size_t count,
                   const std::uint64_t seed) -> std::vector<std::size_t>
{
  SG_REQUIRE(count > 0,
             "the walk order belongs to a selection of at least one");
  std::vector<std::size_t> order(count);
  std::iota(order.begin(), order.end(), std::size_t {0});
  std::mt19937_64 engine(seed);
  for (std::size_t index = count; index > 1; --index) {
    // The engine answers 64 bits at a step and the index is a slot of the
    // unsettled prefix, so one draw per step settles one slot.
    const std::size_t pick = static_cast<std::size_t>(engine() % index);
    std::swap(order[index - 1], order[pick]);
  }
  std::vector<std::size_t> walked(count);
  std::iota(walked.begin(), walked.end(), std::size_t {0});
  SG_ENSURE(std::is_permutation(
                order.begin(), order.end(), walked.begin(), walked.end()),
            "the walk order holds each index once (FR-053, SC-005)");
  return order;
}

/**
 * @brief The text of one file, or one fault line naming the path and the
 * reason the system refused (research D-05, FR-029, FR-061).
 *
 * The reader moved out of the anonymous namespace of `json_load.cpp`
 * unchanged, so both fault texts stay byte-identical and the loader
 * tests keep their expectations until its call sites call this one
 * (T031). The chunked `stream.read` loop is deliberate:
 * `istreambuf_iterator` broke an older compiler.
 */
auto readTextFile(const std::string& path)
    -> std::expected<std::string, std::string>
{
  std::ifstream stream(path, std::ios::binary);
  if (!stream.is_open()) {
    return std::unexpected("the file '" + path
                           + "' cannot be opened: " + std::strerror(errno));
  }
  std::string text;
  char chunk[1024];
  do {
    stream.read(chunk, std::streamsize(sizeof chunk));
    text.append(chunk, std::size_t(stream.gcount()));
  } while (stream.gcount() == std::streamsize(sizeof chunk));
  if (stream.bad()) {
    // The file opens and the read itself fails (a directory,
    // /proc/self/mem): the expected stays the contract's answer
    // channel, so the failure is answered here.
    return std::unexpected("the file '" + path
                           + "' cannot be read: " + std::strerror(errno));
  }
  return text;
}

/**
 * @brief The refusal of a source line that carries a NUL byte, naming the
 * source, the line number and the text as it stands (FR-033, FR-061).
 *
 * A NUL byte ends a C string, so a reader handed such a line reads a
 * prefix of it and answers for a value nobody spelled. The line is refused
 * before any reader takes it, the way a name the table carries is refused
 * (FR-035, FR-065, D-12).
 */
auto nulLine(const std::string& where, std::string_view line) -> std::string
{
  return where + ": expected a value without a NUL byte, got '"
      + std::string(line) + "'";
}

/**
 * @brief The rules of one rules file, read outside the gather so the
 * gather opens nothing (FR-061, FR-062, FR-064, FR-065, D-05).
 *
 * The line numbers count from 1 over the raw lines, so a comment and a
 * blank line hold a number the fault can name, the way an option file
 * names them (FR-062, D-13). The grammar of specs/026 is what reads every
 * other line, unchanged (FR-061).
 */
auto readCompareRulesFile(const std::string& path)
    -> std::expected<std::vector<CompareRule>, std::string>
{
  const auto text = readTextFile(path);
  if (!text.has_value()) {
    // The reader's own text names the path and the reason the open
    // answered, and the option names the line (FR-065).
    return std::unexpected("--compare-rules: " + text.error());
  }
  std::vector<CompareRule> rules;
  std::string_view rest {*text};
  std::size_t number = 0;
  while (!rest.empty()) {
    const std::size_t cut = rest.find('\n');
    const std::string_view line =
        cut == std::string_view::npos ? rest : rest.substr(0, cut);
    rest = cut == std::string_view::npos ? std::string_view {}
                                         : rest.substr(cut + 1);
    ++number;
    constexpr std::string_view kBlanks {" \t\r"};
    const std::size_t first = line.find_first_not_of(kBlanks);
    if (first == std::string_view::npos || line[first] == '#') {
      continue;  // FR-062
    }
    if (line.find('\0') != std::string_view::npos) {
      return std::unexpected(
          nulLine(path + ":" + std::to_string(number), line));
    }
    const std::size_t last = line.find_last_not_of(kBlanks);
    const auto rule = parseCompareRule(line.substr(first, last - first + 1));
    if (!rule.has_value()) {
      // The file, the number of the raw line, and the parser's own text for
      // that line, joined in that order (FR-064).
      return std::unexpected(path + ":" + std::to_string(number) + ": "
                             + rule.error());
    }
    rules.push_back(std::move(*rule));
  }
  return rules;
}

namespace
{

// The environment name of a flagged row: `SPEEDGUN_` plus the long name
// upper-cased with every `-` changed to `_`, computed from the row so a
// row that moves carries its variable with it (FR-019, D-09).
auto environmentName(const OptionRow& row) -> std::string
{
  std::string name = "SPEEDGUN_";
  for (const char letter : std::string_view(row.name)) {
    name.push_back(letter == '-' ? '_'
                                 : static_cast<char>(std::toupper(
                                       static_cast<unsigned char>(letter))));
  }
  return name;
}

// The text the source holds under this name, or null when it holds none.
// A `SPEEDGUN_*` name that matches no flagged row is never asked for,
// which is what ignores it (FR-026).
auto environmentValue(const std::vector<EnvironmentRow>& environment,
                      const std::string& name) -> const std::string*
{
  for (const EnvironmentRow& row : environment) {
    if (row.name == name) {
      return &row.value;
    }
  }
  return nullptr;
}

// The items of a repeatable value read from the environment, split on `;`
// (FR-024, spec D-6).
auto splitItems(const std::string& text) -> std::vector<std::string_view>
{
  std::vector<std::string_view> items;
  const std::string_view whole = text;
  for (std::size_t at = 0;;) {
    const std::size_t cut = whole.find(';', at);
    items.push_back(cut == std::string_view::npos ? whole.substr(at)
                                                  : whole.substr(at, cut - at));
    if (cut == std::string_view::npos) {
      break;
    }
    at = cut + 1;
  }
  return items;
}

// The rows whose value accumulates within one source: their arms below
// append the items of every word, where a scalar keeps the last word
// (FR-035, FR-038). The disclosure of an accumulated row joins its items
// with `;` and the disclosure of a scalar names the word that took effect
// (FR-040). The rule list stands with the scalars: the highest source that
// named a rule settles the whole list, and a rule of a lower source is read
// for its fault and then dropped (FR-038, FR-063, D-23).
auto accumulates(const OptionRow& row) -> bool
{
  switch (row.value) {
    case 'c':
    case 'M':
    case 'p':
    case kThreadsOptionValue:
      return true;
    default:
      return false;
  }
}

// The member one flag row turns. A row that carries no value has nothing to
// parse beyond its flag word, so one arm settles every flag row through the
// member it names; a row that carries a value answers no member (FR-025,
// FR-033).
auto flagMember(const int value) -> std::optional<bool RunOptions::*>
{
  switch (value) {
    case 'l':
      return &RunOptions::listMode;
    case 'd':
      return &RunOptions::dryRun;
    case 'C':
      return &RunOptions::catalogMode;
    case 'T':
      return &RunOptions::topologyMode;
    case kCompareStrictOptionValue:
      return &RunOptions::compareStrict;
    default:
      return std::nullopt;
  }
}

// Where the earlier settle of one row came from, when a row carries one:
// the same source on an earlier line, a higher source, or nothing yet. The
// answer picks which of the two rules of FR-037 and FR-038 applies, so the
// seam names the rule it reads (FR-035, FR-038, N7 hard rule 8).
enum class PriorSettle
{
  /// No source has settled the row yet, or nothing above this source did.
  NO_SETTLE,
  /// This same source settled the row on an earlier line, so the rule of one
  /// source stands: a repeatable accumulates and a scalar keeps its word.
  SAME_SOURCE,
};

constexpr auto kNoPriorSettle = PriorSettle::NO_SETTLE;
constexpr auto kSameSourceSettled = PriorSettle::SAME_SOURCE;

// Settle one row from the text one source holds for it, through the reader
// the command line uses for that value kind, so one rule covers the
// environment and the option file (FR-024, FR-025, FR-027, FR-035, D-11).
// Every text is handed to its reader first, so a value that a higher source
// shadows still answers its own fault line at status 2 (FR-024, FR-027,
// SC-003). The answer then says whether the value
// took effect: a row a higher source already settled keeps its word, which
// is the precedence of FR-037 read at this seam, and `prior` says this
// same source settled the row on an earlier line, where the command
// line's own rule applies instead: a repeatable accumulates and a scalar
// keeps the last word (FR-035, FR-038).
// `origin` names the source ahead of the reader's own refusal line: the
// variable for the environment, the path and the line number for a file
// (FR-027, FR-035).
//
// The `no-percentiles` row is the negating flag, so the word is the flag's
// truth and `1` turns percentiles off, the answer the scan's `case 'P'`
// gives for the same option (FR-025).
auto applySourceValue(const OptionRow& row,
                      const std::string& text,
                      const std::string& origin,
                      const PriorSettle prior,
                      ResolvedOptions& resolved)
    -> std::expected<bool, std::string>
{
  RunOptions& options = resolved.options;
  const std::string option = std::string("--") + row.name;
  const auto refuse = [&](const std::string& line)
  { return std::unexpected(origin + ": " + line); };
  switch (row.value) {
    case 'f':
      if (!options.filter.empty() && prior != kSameSourceSettled) {
        return false;
      }
      options.filter = text;
      return true;
    default: {
      // The five rows that carry no value turn one member of `RunOptions`,
      // and the word is the flag's truth, the way `--no-percentiles` reads
      // its own word (FR-025, FR-033). The four rows that carry a value yet
      // cannot stand in a file - the two that would re-root the file's own
      // sources and the two that print - are refused here ahead of any value
      // rule, the single place that rule is read (FR-031, spec D-9).
      const auto member = flagMember(row.value);
      if (!member.has_value()) {
        return refuse("the option '" + std::string(row.name)
                      + "' cannot stand in an option file");
      }
      const auto flag = readFlag(text);
      if (!flag.has_value()) {
        return refuse(flag.error());
      }
      if (options.*member.value() && prior != kSameSourceSettled) {
        return false;
      }
      options.*member.value() = *flag;
      return true;
    }
    case 'r': {
      const auto count = parseCount(text.c_str(), option.c_str());
      if (!count.has_value()) {
        return refuse(count.error());
      }
      if (options.repetitions.has_value() && prior != kSameSourceSettled) {
        return false;
      }
      options.repetitions = *count;
      return true;
    }
    case 'm': {
      const auto seconds =
          parseSeconds(text.c_str(), option.c_str(), kZeroRefused);
      if (!seconds.has_value()) {
        return refuse(seconds.error());
      }
      if (options.minTimeNs.has_value() && prior != kSameSourceSettled) {
        return false;
      }
      options.minTimeNs = *seconds;
      return true;
    }
    case 'i': {
      const auto count = parseCount(text.c_str(), option.c_str());
      if (!count.has_value()) {
        return refuse(count.error());
      }
      if (options.fixedIterations.has_value() && prior != kSameSourceSettled) {
        return false;
      }
      options.fixedIterations = *count;
      return true;
    }
    case 'w': {
      const auto seconds =
          parseSeconds(text.c_str(), option.c_str(), kZeroAdmitted);
      if (!seconds.has_value()) {
        return refuse(seconds.error());
      }
      if (options.warmupTimeNs.has_value() && prior != kSameSourceSettled) {
        return false;
      }
      options.warmupTimeNs = *seconds;
      return true;
    }
    case 'c':
      if (!options.counterAddresses.empty() && prior != kSameSourceSettled) {
        return false;
      }
      for (const std::string_view item : splitItems(text)) {
        options.counterAddresses.emplace_back(item);
      }
      return true;
    case 'M':
      if (!options.metricNames.empty() && prior != kSameSourceSettled) {
        return false;
      }
      for (const std::string_view item : splitItems(text)) {
        options.metricNames.emplace_back(item);
      }
      return true;
    case 'p': {
      // The display flag is what marks this row settled: the scan's
      // `--percentiles` arm and this arm both set it when they fill the
      // points, so the points carry no fact the flag does not (FR-025).
      std::vector<double> points = options.percentilePoints;
      for (const std::string_view item : splitItems(text)) {
        const std::string itemText(item);
        const auto read =
            parsePercentileList(itemText.c_str(), option.c_str(), points);
        if (!read.has_value()) {
          return refuse(read.error());
        }
      }
      if (prior != kSameSourceSettled && options.showPercentiles.has_value()) {
        return false;
      }
      options.percentilePoints = points;
      options.showPercentiles = true;
      return true;
    }
    case 'P': {
      const auto flag = readFlag(text);
      if (!flag.has_value()) {
        return refuse(flag.error());
      }
      if (options.showPercentiles.has_value() && prior != kSameSourceSettled) {
        return false;
      }
      // The row is the negating flag: the word is the flag's truth, and the
      // flag standing turns percentiles off exactly as the scan's
      // `case 'P'` does, so `SPEEDGUN_NO_PERCENTILES=1` answers
      // `showPercentiles = false` and `=0` answers true (FR-025).
      options.showPercentiles = !*flag;
      return true;
    }
    case 'A':
      if (options.placement.has_value() && prior != kSameSourceSettled) {
        return false;
      }
      options.placement = text;
      return true;
    case kThreadsOptionValue: {
      // A count list the command line settled keeps its word, which is the
      // replacement FR-038 states for a repeatable a higher source named.
      // Within one source the items append in order, the way the counter
      // addresses do (FR-035).
      // Each `;` item carries the comma-separated list the command line
      // reads, and the settled counts stand in `RunOptions` (E-11) until
      // the loop over `registry()` writes them into every family
      // record ahead of the expansion (FR-044, FR-045, FR-050, D-14).
      std::vector<int> counts;
      for (const std::string_view item : splitItems(text)) {
        const auto words = readThreadList(item);
        if (!words.has_value()) {
          return refuse(words.error());
        }
        counts.insert(counts.end(), words->begin(), words->end());
      }
      if (!options.threadCounts.empty() && prior != kSameSourceSettled) {
        return false;
      }
      options.threadCounts.insert(
          options.threadCounts.end(), counts.begin(), counts.end());
      return true;
    }
    case kInterleaveOptionValue: {
      const auto seed = readSeed(text);
      if (!seed.has_value()) {
        return refuse(seed.error());
      }
      if (resolved.interleave.given && prior != kSameSourceSettled) {
        return false;
      }
      // `SPEEDGUN_INTERLEAVE=0` is given with seed 0: the reader answers
      // the number and the option stands on (FR-022, E-10).
      resolved.interleave = InterleaveSeed {true, *seed};
      return true;
    }
    case 'j':
      if (text.empty()) {
        return refuse(kEmptyPathFault);
      }
      if (!options.jsonPath.empty() && prior != kSameSourceSettled) {
        return false;
      }
      options.jsonPath = text;
      return true;
    case 'J': {
      // The word is read against the two words the contract names, through
      // the one reader of the row (FR-049, FR-050).
      const auto content = parseRawContent(text);
      if (!content.has_value()) {
        return refuse(content.error());
      }
      if (resolved.rawContentSettled && prior != kSameSourceSettled) {
        return false;
      }
      options.rawContent = *content;
      resolved.rawContentSettled = true;
      return true;
    }
    case kCompareOptionValue: {
      // The pair of paths splits at the first comma through the one reader
      // of the row, and an empty side is refused before the mode could open
      // a document (FR-052, spec D-16).
      const auto pair = parseComparePair(text);
      if (!pair.has_value()) {
        return refuse(pair.error());
      }
      if (!options.compareBaseline.empty() && prior != kSameSourceSettled) {
        return false;
      }
      options.compareBaseline = pair->first;
      options.compareContender = pair->second;
      return true;
    }
    case kMetricMethodOption: {
      const auto method = parseMetricMethod(text.c_str());
      if (!method.has_value()) {
        return refuse("--metric-method '" + text
                      + "' is not auto, passes or multiplex");
      }
      if (options.metricMethod.has_value() && prior != kSameSourceSettled) {
        return false;
      }
      options.metricMethod = method;
      return true;
    }
    case kCompareRuleOptionValue: {
      // FR-063, spec D-23 as this round reads it: the rule list does not
      // combine across sources, so the highest source that named a rule
      // settles the whole list. A rule of a lower source is still read, so
      // its own fault reaches the operator, then drops (FR-024, FR-038).
      // Within one source the order recommendation 8 keeps holds: every rule
      // that source spells stands, in the order it spelled them (FR-035).
      const auto rule = parseCompareRule(text);
      if (!rule.has_value()) {
        return refuse(rule.error());
      }
      if (!resolved.rules.empty() && prior != kSameSourceSettled) {
        return false;
      }
      if (prior == kSameSourceSettled) {
        resolved.rules.push_back(std::move(*rule));
      } else {
        resolved.rules = {std::move(*rule)};
      }
      return true;
    }
    case kCompareRulesOptionValue:
      // FR-063: the file this source names joins after the rules this
      // source spells, so the path is kept here and the gather reads it
      // once every source has been applied. A source below one that named a
      // rule names no file either: its path drops the way a rule of that
      // source drops (FR-038, FR-063). A second line of one source keeps its
      // own path, the scalar rule of FR-035.
      if (!resolved.rules.empty() && prior != kSameSourceSettled) {
        return false;
      }
      resolved.optionFileRulesPath = text;
      return true;
  }
}

// The row of the table that carries this long name, or null when the table
// holds none: the table is the only place an option is recognized, so a
// name with no row is a fault and never a silent word (FR-033).
// The line without the spaces that stand around it, a carrying carriage
// return included, so a line of spaces and tabs is blank and a name starts
// at its first non-space character (FR-030, D-12).
auto trimmed(const std::string_view line) -> std::string_view
{
  constexpr std::string_view kBlanks {" \t\r"};
  const std::size_t begin = line.find_first_not_of(kBlanks);
  if (begin == std::string_view::npos) {
    return {};
  }
  return line.substr(begin, line.find_last_not_of(kBlanks) - begin + 1);
}

// The rules of one named file appended to the settled list. The read
// belongs to the reader the entry point hands, so the gather opens no file
// of its own, and the fault the reader answers is printed as it stands
// (FR-061, FR-064, FR-065, D-05).
auto appendRuleFile(ResolvedOptions& resolved,
                    const std::string& path,
                    RuleFileReader ruleFiles)
    -> std::expected<void, std::string>
{
  SG_ASSERT(ruleFiles != nullptr,
            "a source that names a rules file hands its reader along with "
            "the path (D-05)");
  const auto rules = ruleFiles(path);
  if (!rules.has_value()) {
    return std::unexpected(rules.error());
  }
  std::move(rules->begin(), rules->end(), std::back_inserter(resolved.rules));
  return {};
}

/**
 * @brief Read the text of the `--options` file as data: one option per line
 * in long spelling without the leading dashes, as `name` or `name=value`,
 * each value reaching the reader the command line uses for that kind
 * (FR-025, FR-029, FR-030, FR-031, FR-033, FR-035, spec D-12).
 *
 * The text arrives from the entry point, which owns the read; the gather
 * splits it and opens no file (FR-012, D-05). Line numbers count from 1
 * over the raw lines, so a comment and a blank line hold a number and a
 * fault names the line the reader sees in an editor (D-12). A repeatable
 * accumulates within one file in line order and a scalar keeps the last
 * line's word, which is the rule of the command line the file shares
 * (FR-035).
 */
auto applyOptionFile(const OptionFileText& file, ResolvedOptions& resolved)
    -> std::expected<void, std::string>
{
  std::vector<const OptionRow*> settled;
  // The uses this file can join are the ones it pushed itself, so the join
  // search starts where they start (E-05, FR-040).
  const auto fileUses = static_cast<std::ptrdiff_t>(resolved.uses.size());
  std::string_view rest {file.text};
  std::size_t number = 0;
  while (!rest.empty()) {
    const std::size_t cut = rest.find('\n');
    const std::string_view line =
        trimmed(cut == std::string_view::npos ? rest : rest.substr(0, cut));
    rest = cut == std::string_view::npos ? std::string_view {}
                                         : rest.substr(cut + 1);
    ++number;
    if (line.empty() || line.front() == '#') {
      continue;  // FR-030
    }
    const std::size_t equals = line.find('=');
    const std::string_view name =
        equals == std::string_view::npos ? line : line.substr(0, equals);
    // Every fault of the file names the path and the line number, the way
    // the environment names its variable (FR-031, FR-033, FR-035).
    const std::string where = file.path + ":" + std::to_string(number);
    if (line.find('\0') != std::string_view::npos) {
      return std::unexpected(nulLine(where, line));
    }
    if (line.front() == '-') {
      // The command-line spelling pasted into the file. The table is never
      // asked for a name of that shape, and the fault prints the whole line
      // as the text it read (FR-036, D-12).
      return std::unexpected(where
                             + ": expected an option name without --, got '"
                             + quotedText(line) + "'");
    }
    const OptionRow* const row = rowNamed(name);
    if (row == nullptr) {
      return std::unexpected(where + ": unknown option '" + std::string(name)
                             + "'");
    }
    // A line that carries no `=` carries no value either, and what that
    // means depends on the row's own argument kind (FR-036, FR-008, D-11).
    std::string value = equals == std::string_view::npos
        ? std::string {}
        : std::string(line.substr(equals + 1));
    if (equals == std::string_view::npos) {
      if (row->argument == OptionArgument::REQUIRED) {
        // The reader's own words stand behind the file and the number, so
        // the file invents no text of its own (FR-036, FR-008).
        return std::unexpected(where + ": " + missingValueLine(row->value));
      }
      // A row that takes no value, and a row whose value may stand absent,
      // both stand bare on the line the way they do on the command line: the
      // flag word is the flag standing, and an absent optional value is the
      // seed 0 the bare option carries (FR-029, E-10).
      value = row->argument == OptionArgument::OPTIONAL ? "0" : "1";
    }
    const auto again =
        std::find(settled.begin(), settled.end(), row) != settled.end()
        ? kSameSourceSettled
        : kNoPriorSettle;
    const auto applied = applySourceValue(*row, value, where, again, resolved);
    if (!applied.has_value()) {
      return std::unexpected(applied.error());
    }
    if (!*applied) {
      // A higher source settled the row, so the line sets nothing and
      // discloses nothing (FR-037, FR-038).
      continue;
    }
    settled.push_back(row);
    // One use per row: a repeatable joins the text of every line that settled
    // it with `;`, and a scalar carries the line that took effect, which is the
    // last of them (E-05, FR-035, FR-040).
    const auto prior =
        std::find_if(resolved.uses.begin() + fileUses,
                     resolved.uses.end(),
                     [&](const OptionUse& use) { return use.row == row; });
    if (prior != resolved.uses.end()) {
      if (accumulates(*row)) {
        prior->value += ";" + value;
      } else {
        prior->value = value;
      }
    } else {
      resolved.uses.push_back(
          OptionUse {row, OptionSource::OPTION_FILE, value});
    }
  }
  return {};
}

}  // namespace

// The row of the table that carries this long name, or null when the table
// holds none: the table is the only place an option is recognized, so a name
// with no row is a fault and never a silent word (FR-033). The option file,
// the help printer and the disclosure tests all read the table through this
// one lookup, so the table is walked in one place (FR-033, FR-017).
auto rowNamed(const std::string_view name) -> const OptionRow*
{
  for (const OptionRow& row : optionRows()) {
    if (name == std::string_view(row.name)) {
      // optionRows() is static constexpr storage, so this address is stable.
      // cppcheck-suppress returnDanglingLifetime
      return &row;
    }
  }
  return nullptr;
}

/**
 * @brief Settle every option across the sources the entry point hands as
 * data: the words the scan read, the `SPEEDGUN_*` rows, and the text of
 * the file `--options` named (FR-019 to FR-028, FR-037 to FR-040, D-04,
 * D-10).
 *
 * The command line stands first: the scan already wrote its values into
 * the answer, and a row the words settled keeps that value and answers no
 * use (FR-037, FR-038). The file lines follow, then the `SPEEDGUN_*` rows,
 * so the same precedence rule settles every source below the words. The
 * disclosed uses follow the table order of FR-039 and FR-040, whichever
 * source settled them and in whatever order the file lines stood.
 *
 * The option file arrives as the path and the whole text the entry point
 * read: the gather splits the text and opens no file, and a file the
 * system refused is answered at the entry point before the gather runs
 * (FR-012, FR-029, FR-034, D-05). The rule list of FR-063 combines across
 * the sources in the order spec D-23 states, and the master default of
 * FR-041 stays for every option no source names.
 */
auto gatherOptionSources(const CommandLineOptions& scan,
                         const std::vector<EnvironmentRow>& environment,
                         const std::optional<OptionFileText>& optionFile,
                         const RuleFileReader ruleFiles)
    -> std::expected<ResolvedOptions, std::string>
{
  SG_REQUIRE(scan.stop == StopKind::NONE,
             "the gather settles a line the scan walked to its end");
  ResolvedOptions resolved;
  resolved.options = scan.options;
  // The `--json-raw` row has no unset word, so the name the scan carries is
  // what marks the row settled before any lower source is asked (FR-037,
  // FR-050).
  resolved.rawContentSettled = scan.rawContentNamed;
  // FR-051: the seed of the command line is the highest source, so it
  // settles before any lower source is asked, and the rule that a seed of
  // another source answers nothing is what keeps a file or a variable from
  // replacing it (FR-037).
  resolved.interleave = scan.interleave;

  // FR-063, spec D-23 as this round reads it: the highest source that names
  // a rule settles the whole list, so the list starts here with the command
  // line's own rules and the file its word named follows them in line order
  // (FR-038). No lower source adds to it.
  resolved.rules = scan.options.compareRules;
  if (!scan.options.compareRulesFile.empty()) {
    const auto read =
        appendRuleFile(resolved, scan.options.compareRulesFile, ruleFiles);
    if (!read.has_value()) {
      return std::unexpected(read.error());
    }
  }

  // The file stands between the words and the environment (FR-037, D-1).
  if (optionFile.has_value()) {
    const auto read = applyOptionFile(*optionFile, resolved);
    if (!read.has_value()) {
      return std::unexpected(read.error());
    }
  }

  // The last position of the order this round reads: the file that a
  // `compare-rules` line of the option file names joins after that file's
  // own rules, and only when no higher source named a rule at all (FR-063).
  if (!resolved.optionFileRulesPath.empty()) {
    const auto read =
        appendRuleFile(resolved, resolved.optionFileRulesPath, ruleFiles);
    if (!read.has_value()) {
      return std::unexpected(read.error());
    }
  }

  // The flagged rows whose variable the source holds, in table order
  // (E-07). A name that matches no flagged row is never asked for here,
  // which is the ignoring of FR-026.
  std::vector<const OptionRow*> present;
  for (const OptionRow& row : optionRows()) {
    if (row.fromEnvironment
        && environmentValue(environment, environmentName(row)) != nullptr)
    {
      present.push_back(&row);
    }
  }

  // FR-023, spec D-5: the pair is read over the environment rows alone
  // and ahead of the precedence rule, so a command line that overrides
  // one half leaves the fault standing. The command-line pair keeps the
  // master answer, where the last word wins and the status stays 0.
  const auto holdsName = [&present](const char* const name)
  {
    // optionRows() is static constexpr storage, so these pointers are stable.
    // cppcheck-suppress danglingTemporaryLifetime
    return std::any_of(present.begin(),
                       present.end(),
                       [name](const OptionRow* const row)
                       { return name == std::string_view(row->name); });
  };
  if (holdsName("percentiles") && holdsName("no-percentiles")) {
    return std::unexpected(
        "SPEEDGUN_PERCENTILES and SPEEDGUN_NO_PERCENTILES cannot both stand");
  }

  for (const OptionRow* const row : present) {
    const std::string& text =
        *environmentValue(environment, environmentName(*row));
    const auto settled = applySourceValue(
        *row, text, environmentName(*row), kNoPriorSettle, resolved);
    if (!settled.has_value()) {
      return std::unexpected(settled.error());
    }
    if (*settled) {
      resolved.uses.push_back(OptionUse {row, OptionSource::ENVIRONMENT, text});
    }
  }

  // FR-039, FR-040: the disclosed uses stand in table order. The file
  // answers them in line order, so the whole list is put in order here.
  // The rows stand in one static array, so the order of their addresses is
  // the order of the table and one comparator serves the sort and the
  // postcondition.
  const auto tableOrder = [](const OptionUse& left, const OptionUse& right)
  { return left.row < right.row; };
  std::stable_sort(resolved.uses.begin(), resolved.uses.end(), tableOrder);
  SG_ENSURE(
      std::is_sorted(resolved.uses.begin(), resolved.uses.end(), tableOrder),
      "the disclosed uses stand in table order (FR-038, FR-039)");
  return resolved;
}

}  // namespace detail
}  // namespace sg
