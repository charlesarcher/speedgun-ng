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
#include <regex>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "detail/options.hpp"

#include <getopt.h>

#include "detail/compare.hpp"
#include "detail/hunt_selection.hpp"
#include "detail/noise_tracer.hpp"
#include "speedgun-ng/dbc.hpp"

/**
 * @file options.cpp
 * @brief The one option table of specs/028-full-command-line and the scan
 * that walks it: the 37 rows `detail::optionRows()` answers in the order
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

// The declaration and the contract of the quoting stand in
// detail/options.hpp (FR-024, FR-027, FR-035, SC-003).
auto quotedText(const std::string_view text) -> std::string
{
  static constexpr std::string_view kHexDigits = "0123456789abcdef";
  std::string quoted;
  quoted.reserve(text.size());
  for (const char letter : text) {
    const auto byte = static_cast<unsigned char>(letter);
    if (letter == '\\') {
      quoted += "\\\\";
    } else if (letter == '\n') {
      quoted += "\\n";
    } else if (letter == '\r') {
      quoted += "\\r";
    } else if (letter == '\0') {
      quoted += "\\0";
    } else if (byte < 0x20 || byte == 0x7f) {
      // A tab, an escape and the rest of that class move the cursor or
      // erase a line, so the byte reaches the record as its own number
      // (FR-027, SC-003).
      quoted += "\\x";
      quoted.push_back(kHexDigits[byte >> 4]);
      quoted.push_back(kHexDigits[byte & 0x0f]);
    } else {
      quoted.push_back(letter);
    }
  }
  return quoted;
}

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
  if (errno != 0 || end == text || *end != '\0' || value <= 0
      || opensWithBlank(text))
  {
    return std::unexpected(std::string(option)
                           + ": expected a positive integer, got '"
                           + quotedText(text) + "'");
  }
  return static_cast<std::uint64_t>(value);
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
// through here, so the three words are written once (FR-034, FR-035, FR-050).
auto parseRawContent(const std::string_view text)
    -> std::expected<RawContent, std::string>
{
  if (text == "chunks") {
    return RawContent::CHUNKS;
  }
  if (text == "gaps") {
    return RawContent::GAPS;
  }
  if (text == "none") {
    return RawContent::NONE;
  }
  return std::unexpected(
      "--json-raw: 'chunks', 'gaps' and 'none' are the values the option "
      "names");
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

// The tail of the line a row that names a path answers when the word for
// The tail of the line an environment variable answers when its value stands
// empty, which no reader of the table settles (FR-025, FR-027).
constexpr std::string_view kEmptyValueTail = "the value stands empty";

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
    return "unknown option '" + quotedText(word) + "'; --help lists the "
           "options";
  }
  return "unknown option '" + quotedText(word) + "', nearest '--"
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
  return "ambiguous option '" + quotedText(word) + "': " + candidates;
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
 * @brief The one option table: the 37 rows in the order the contract
 * fixes them, each with its name, internal value, argument kind,
 * environment flag, help text and owning requirement (FR-001, FR-002,
 * FR-003, FR-004, FR-079, SC-001).
 *
 * The 20 rows the master carries keep their spelling and their internal
 * value, the four constants 256 to 259 among them (FR-003, PC-1). The
 * seven names specs/028 brings take the constants 290 to 296 that
 * spec D-24 reserves (FR-004). The letters of the master rows are switch
 * codes and never short options: the scan's option string carries none
 * (FR-005). Exactly the 12 rows FR-020 names carry the environment flag,
 * and the name of such a row's variable follows from the row (FR-019).
 * The noise tracer's row takes the constant 297 (specs/036 FR-001).
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
       "Selects the chunk columns or the kept gaps the document carries",
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
      {"jitter-probe",
       kJitterProbeOptionValue,
       OptionArgument::REQUIRED,
       false,
       "Runs the jitter probe over the batch by that value",
       "specs/027-jitter-probe"},
      {"os-noise",
       kOsNoiseOptionValue,
       OptionArgument::REQUIRED,
       false,
       "Runs the named noise tracer across the pre-check window",
       "specs/036-osnoise-timerlat-tracers"},
      // specs/029-hunter-core FR-080 to FR-082, D-22: the hunt family, nine
      // rows on the ids the constants block of `detail/options.hpp` reserves.
      // No hunt row carries an environment name, so the gather refuses each of
      // them from a file or the environment and the command line owns the
      // family. `--hunt` is the table's first optional_argument row, and
      // `getopt_long` reads an optional argument only in the attached form, so
      // `--hunt=all` selects and `--hunt all` selects nothing and leaves `all`
      // positional.
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
       "Sets the hunt budget: <n>s, <n>p, <n>e or <n>g clauses",
       "specs/029"},
      {"hunt-seed",
       kHuntSeedOption,
       OptionArgument::REQUIRED,
       false,
       "Sets the seed the hunt repeats draw from",
       "specs/029"},
      // specs/033-hunter-budgets FR-010, R-13: the hunt lane's ninth row, on
      // the free constant 280 the hunt block comment names. The command line
      // owns it, the way it owns every row of the family.
      {"hunt-emit-set",
       kHuntEmitSetOption,
       OptionArgument::REQUIRED,
       false,
       "Writes the ranked hunt events to that file",
       "specs/033"},
  });
  return kRows;
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

// The row of the table that carries this option value. The values of the rows
// are distinct, so one value names at most one row, and every caller holds a
// value the table itself carries (FR-002, FR-006).
auto rowOfValue(const int value) -> const OptionRow*
{
  const auto found = std::ranges::find_if(optionRows(),
                                          [value](const OptionRow& row)
                                          { return row.value == value; });
  // The value came out of the option table, so the search stops on a row.
  SG_ENSURE(found != optionRows().end(),
            "a value the option table carries names one row");
  // optionRows() is static constexpr storage, so this address is stable.
  // cppcheck-suppress returnDanglingLifetime
  return &*found;
}

// Whether the source in hand settled this row already. The two percentile
// rows write one flag, so a word or a line of the sibling row is a prior
// settle of the state this row writes: the last word of one source wins the
// flag and the points, while a higher source keeps its word over a lower one
// (FR-025, FR-035, FR-038).
auto settledInSource(const std::vector<const OptionRow*>& seen,
                     const OptionRow& row) -> bool
{
  if (std::find(seen.begin(), seen.end(), &row) != seen.end()) {
    return true;
  }
  if (row.value != 'p' && row.value != 'P') {
    return false;
  }
  const OptionRow* const sibling = rowOfValue(row.value == 'p' ? 'P' : 'p');
  return std::find(seen.begin(), seen.end(), sibling) != seen.end();
}

// Settle one row of one source through the reader of its value kind. The
// definition stands below, where the readers of every kind stand, and the
// scan calls it for every word that names a row so that one row answers one
// fault text in every source (FR-019, FR-024, FR-046, FR-054).
auto applySourceValue(const OptionRow& row,
                      const std::string& text,
                      const std::string& origin,
                      const PriorSettle prior,
                      const OptionSource source,
                      ResolvedOptions& resolved)
    -> std::expected<bool, std::string>;

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
  RunOptions& options = scan.settled.options;
  const std::vector<option>& entries = getoptOptionArray();

  int choice = 0;
  bool optionsSeen = false;
  opterr = 0;
  optind = 0;
  // The rows the words have settled already: a word that names one of them
  // again is a repeat within one source, where a repeatable accumulates and a
  // scalar keeps its last word (FR-035, FR-037).
  std::vector<const OptionRow*> settled;
  while ((choice = getopt_long(argc, argv, ":", entries.data(), nullptr)) != -1)
  {
    switch (choice) {
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
      case kOptionsOptionValue:
        // FR-032, spec D-9: one option file per run. The second word
        // answers one line naming the option alone, with no path repeated.
        // The path reaches `RunOptions` (E-03) and the entry point owns the
        // read, so the gather keeps opening nothing (D-05).
        if (*optarg == '\0') {
          return std::unexpected(emptyPathFault("--options"));
        }
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
      case kJitterProbeOptionValue:
        // specs/027 FR-011, R-06: the text stands raw here; the entry
        // point parses it once before the expansion and before any run,
        // and an empty value stands stored so its refusal reads the
        // empty token (FR-011, R-06).
        options.jitterProbeText = optarg;
        break;
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
      case kHuntEmitSetOption:
        parseHuntEmitSet(optarg, options);
        break;
      case ':':
        return std::unexpected(missingValueLine(optopt));
      case '?': {
        // The word getopt_long could not place stands in this arm: the
        // missing-value marker has its arm above and every value the table
        // carries settles below, so the value that reaches here names no row
        // by construction, and the scanner reports only that the word is
        // bad. A single dash letter arrives through `optopt`, and the option
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
      default: {
        // Every other word names a row of the table, and that row settles
        // through the one function that settles it for the environment and
        // the option file, so one reader answers one fault text for one bad
        // value in every source (FR-019, FR-024, FR-046, FR-054). The origin
        // stands empty, since the line of the words names the option itself
        // (FR-027, FR-033).
        const OptionRow* const row = rowOfValue(choice);
        // A row that takes no value, and the one row whose value may stand
        // absent, stand bare on the word the way they do on a file line: the
        // flag word is the flag standing, and an absent optional value is
        // the seed 0 the bare option carries (FR-025, E-10).
        const std::string text = optarg != nullptr
            ? std::string(optarg)
            : std::string(row->argument == OptionArgument::OPTIONAL ? "0"
                                                                    : "1");
        const auto again = settledInSource(settled, *row) ? kSameSourceSettled
                                                          : kNoPriorSettle;
        const auto applied = applySourceValue(*row,
                                              text,
                                              std::string {},
                                              again,
                                              OptionSource::COMMAND_LINE,
                                              scan.settled);
        if (!applied.has_value()) {
          return std::unexpected(applied.error());
        }
        settled.push_back(row);
        break;
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
    const std::string quoted = quotedText(text);
    const std::string line = "--interleave: the seed '" + quoted
        + "' is not an unsigned decimal value";
    SG_ENSURE(line.find(quoted) != std::string::npos,
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
    return std::unexpected("the file '" + quotedText(path)
                           + "' cannot be opened: "
                           + quotedText(std::strerror(errno)));
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
    return std::unexpected("the file '" + quotedText(path)
                           + "' cannot be read: "
                           + quotedText(std::strerror(errno)));
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
      + quotedText(line) + "'";
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
          nulLine(quotedText(path) + ":" + std::to_string(number), line));
    }
    const std::size_t last = line.find_last_not_of(kBlanks);
    const auto rule = parseCompareRule(line.substr(first, last - first + 1));
    if (!rule.has_value()) {
      // The file, the number of the raw line, and the parser's own text for
      // that line, joined in that order (FR-064).
      return std::unexpected(quotedText(path) + ":" + std::to_string(number)
                             + ": " + rule.error());
    }
    rules.push_back(std::move(*rule));
  }
  return rules;
}

auto compileFilterPattern(const std::string& pattern)
    -> std::expected<std::optional<std::regex>, std::string>
{
  if (pattern.empty()) {
    return std::optional<std::regex> {};
  }
  try {
    return std::optional<std::regex>(pattern);
  } catch (const std::regex_error& error) {
    return std::unexpected("invalid regular expression '" + quotedText(pattern)
                           + "': " + quotedText(error.what()));
  }
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

// The values one source's text holds. The `;` list of FR-024 belongs to the
// environment alone, which names a row once: a file line and a word of the
// command line hand the whole of their text to the reader of the row, the
// rule FR-035 gives the file, so the same value faults the same way from
// either (FR-024, FR-035).
auto sourceItems(const std::string& text,
                 const OptionSource source) -> std::vector<std::string_view>
{
  if (source != OptionSource::ENVIRONMENT) {
    return {std::string_view(text)};
  }
  return splitItems(text);
}

// specs/033-hunter-budgets FR-014, R-09, contracts/counter-file.md: the
// addresses one `--counter` item names. An item that spells `@path` is one list
// file, read one level deep through `detail::readAddressFileLines`, the reader
// the hunt selection reads with; every other item is the address it spells. The
// refusals of the file form name the value the option carried: the file no host
// opens, the comma that left an address empty, and the file that holds no
// address. An address the host cannot resolve is no refusal of this arm, the
// way it is none when the command line spells it: the entry point reads it the
// standing way and answers the standing line.
[[nodiscard]] auto counterItemAddresses(std::string_view item,
                                        const std::string& option)
    -> std::expected<std::vector<std::string>, std::string>
{
  if (!item.starts_with('@')) {
    return std::vector<std::string> {std::string(item)};
  }
  const auto lines = readAddressFileLines(std::string(item.substr(1)), item);
  if (!lines.has_value()) {
    return std::unexpected(option + ": " + lines.error());
  }
  for (const auto& address : *lines) {
    if (address.empty()) {
      return std::unexpected(option + ": '' is not a counter address");
    }
  }
  if (lines->empty()) {
    return std::unexpected(option + ": '" + std::string(item)
                           + "' holds no counter address");
  }
  return lines;
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

}  // namespace

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
// (FR-027, FR-035). The words of the command line carry no origin: their line
// names the option the reader named, so it stands without a prefix (FR-046,
// FR-054). `source` says which source holds the text, which is the
// mark the two rules rows compare against the source that settled the list
// (FR-063).
//
// The `no-percentiles` row is the negating flag, so the word is the flag's
// truth and `1` turns percentiles off, the answer the scan's `case 'P'`
// gives for the same option (FR-025).
auto applySourceValue(const OptionRow& row,
                      const std::string& text,
                      const std::string& origin,
                      const PriorSettle prior,
                      const OptionSource source,
                      ResolvedOptions& resolved)
    -> std::expected<bool, std::string>
{
  RunOptions& options = resolved.options;
  const std::string option = std::string("--") + row.name;
  const auto refuse = [&](const std::string& line)
  { return std::unexpected(origin.empty() ? line : origin + ": " + line); };
  switch (row.value) {
    case 'f': {
      // FR-024, FR-050, FR-051: the pattern is compiled where the row
      // settles, so the expression a lower source holds is read whether or
      // not it wins, and the refusal of an expression the regex library
      // refuses names the source that holds it (FR-027, FR-033, FR-038). The
      // row is settled by the word that named it and not by the text it
      // holds: the empty pattern selects every benchmark, so a line whose
      // word stands empty settles the row (FR-037).
      const auto pattern = compileFilterPattern(text);
      if (!pattern.has_value()) {
        return refuse(option + ": " + pattern.error());
      }
      if (resolved.filterSettled && prior != kSameSourceSettled) {
        return false;
      }
      resolved.filterSettled = true;
      resolved.filterPattern = *pattern;
      options.filter = text;
      return true;
    }
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
    case 'c': {
      // FR-038, FR-087: the host resolves an address, so every address every
      // source named is kept for the walk of the entry point, settled or
      // dropped, behind the origin of the source that held it. Each item is an
      // address or the `@path` of one list file (FR-014), and no cap binds the
      // number of addresses either form names.
      std::vector<std::string> addresses;
      for (const std::string_view item : sourceItems(text, source)) {
        const auto read = counterItemAddresses(item, option);
        if (!read.has_value()) {
          return refuse(read.error());
        }
        addresses.insert(addresses.end(), read->begin(), read->end());
      }
      for (const auto& address : addresses) {
        resolved.counterAddresses.push_back(CounterAddress {origin, address});
      }
      if (!options.counterAddresses.empty() && prior != kSameSourceSettled) {
        return false;
      }
      options.counterAddresses.insert(
          options.counterAddresses.end(), addresses.begin(), addresses.end());
      return true;
    }
    case 'M': {
      const std::vector<std::string_view> items = sourceItems(text, source);
      // FR-028, FR-033, FR-038: the lookup of a metric name reads the host,
      // so the entry point walks the names and not the gather. Every name
      // every source named is kept for that walk, settled or dropped, behind
      // the origin of the source that held it, so a name that drops still
      // states its own failure. The words keep their names in the settled
      // list, and the gather seeds that walk with the empty origin its own
      // line carries (FR-037).
      for (const std::string_view item : items) {
        resolved.metricNames.push_back(MetricName {origin, std::string(item)});
      }
      if (!options.metricNames.empty() && prior != kSameSourceSettled) {
        return false;
      }
      for (const std::string_view item : items) {
        options.metricNames.emplace_back(item);
      }
      return true;
    }
    case 'p': {
      // The display flag is what marks this row settled: the scan's
      // `--percentiles` arm and this arm both set it when they fill the
      // points, so the points carry no fact the flag does not (FR-025).
      // The cap of sixteen points counts the points of the source that
      // settles the row: a list a higher source drops is never joined to the
      // points that source holds, so two lists that each stand within the cap
      // cannot fault together, while the points of one source keep
      // accumulating over its own earlier lines (FR-022, FR-035, FR-038).
      std::vector<double> points = prior == kSameSourceSettled
          ? options.percentilePoints
          : std::vector<double> {};
      for (const std::string_view item : sourceItems(text, source)) {
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
    case 'A': {
      // FR-046, FR-048: the preset text is parsed where the row settles, so
      // the name a lower source holds is read whether or not it wins, and the
      // refusal of a name that is not a preset names the source that holds it
      // (FR-027, FR-033, FR-038). No run can meet a name the settle let stand
      // because the settle read it (FR-046).
      const auto preset = parsePreset(text);
      if (!preset.has_value()) {
        return refuse(option + ": '" + quotedText(text)
                      + "' is not one of the six preset names");
      }
      if (options.placement.has_value() && prior != kSameSourceSettled) {
        return false;
      }
      options.placement = text;
      // The preset stands kept beside the text it came from, so the entry
      // point reads a preset the row already parsed (FR-046, FR-048).
      resolved.preset = preset;
      return true;
    }
    case kThreadsOptionValue: {
      // A count list the command line settled keeps its word, which is the
      // replacement FR-038 states for a repeatable a higher source named.
      // Within one source the items append in order, the way the counter
      // addresses do (FR-035).
      // The word of the environment carries the `;` items of FR-024, each one
      // the comma-separated list the command line reads and that a file line
      // hands over whole; the settled counts stand in `RunOptions` (E-11)
      // until the loop over `registry()` writes them into every family
      // record ahead of the expansion (FR-024, FR-035, FR-044, FR-045,
      // FR-050, D-14).
      std::vector<int> counts;
      for (const std::string_view item : sourceItems(text, source)) {
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
        return refuse(emptyPathFault(option));
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
        return refuse("--metric-method '" + quotedText(text)
                      + "' is not auto, passes or multiplex");
      }
      if (options.metricMethod.has_value() && prior != kSameSourceSettled) {
        return false;
      }
      options.metricMethod = method;
      return true;
    }
    case kCompareRuleOptionValue: {
      // FR-063, spec D-23: the highest source that named a rule settles the
      // whole list. The text is parsed first, so a rule of a lower source
      // answers its own fault line before it drops (FR-024, FR-038); what is
      // settled is then held until its own source is applied in full, which
      // is what keeps the rules one source spells before the rules of the
      // file that source names (FR-035, recommendation 8).
      const auto rule = parseCompareRule(text);
      if (!rule.has_value()) {
        // The refusal of a rule names the row that carried it, the way the
        // word of the command line does, so the row answers one text in every
        // source (FR-049, FR-051).
        return refuse(option + ": " + rule.error());
      }
      if (resolved.rulesSource.has_value() && *resolved.rulesSource != source) {
        return false;
      }
      resolved.rulesSource = source;
      resolved.stagedRules.push_back(std::move(*rule));
      return true;
    }
    case kCompareRulesOptionValue:
      // FR-063: the rules of the file one source names join after the rules
      // that source spells, so the path is held until the gather has applied
      // that source in full. The row is scalar within one source: every path
      // a source names is held for the read and the last of them names the
      // rules that would join, so the fault of an earlier path reaches the
      // operator before a lower source drops (FR-038, FR-061, FR-062). An
      // empty word names no file at all, so the row answers the empty-path
      // line of the source that holds it (FR-033, FR-035, FR-063).
      if (text.empty()) {
        return refuse(emptyPathFault(option));
      }
      if (resolved.rulesSource.has_value() && *resolved.rulesSource != source) {
        resolved.stagedRulesPaths.push_back(text);
        return false;
      }
      resolved.rulesSource = source;
      resolved.stagedRulesPaths.push_back(text);
      return true;
    case kJitterProbeOptionValue: {
      // The value parses where it settles, so a file line's refusal
      // names the path and the line every file fault names, and a lower
      // source's line is still read before it drops (FR-024, FR-027,
      // FR-038, R-06).
      JitterProbeOptions parsed;
      std::string message;
      if (!parseJitterProbeValue(text, parsed, message)) {
        return refuse(option + ": " + message);
      }
      // The command line's own text is the only source that fills
      // `jitterProbeText`, and a row that settles this value clears it
      // below, so a text standing held here names a command-line value:
      // the line sets nothing and discloses nothing, and no second test
      // is needed to say so (FR-037, FR-038).
      if (options.jitterProbeText.has_value()) {
        return false;
      }
      options.jitterProbe = std::move(parsed);
      options.jitterProbeText.reset();
      return true;
    }
    case kOsNoiseOptionValue: {
      // specs/036 FR-002, FR-003, R-06: the value parses where it
      // settles, so a file line's refusal names the path and the line
      // every file fault names, and a lower source's line is still read
      // before it drops (FR-024, FR-027, FR-038, R-06).
      auto engine = TracerEngine::NONE;
      auto cpus = std::vector<std::uint32_t> {};
      std::string message;
      if (!parseOsNoiseValue(text, engine, cpus, message)) {
        return refuse(option + ": " + message);
      }
      // The scan routes the command line's word through this arm, so an
      // engine standing settled here names a higher source's word, and
      // the line sets nothing and discloses nothing (FR-037, FR-038).
      // Within one source the last line's word stands, the scalar rule
      // the file shares with the command line (FR-035).
      if (options.noiseTracer.has_value() && prior != kSameSourceSettled) {
        return false;
      }
      options.noiseTracer = engine;
      options.noiseTracerCpus = std::move(cpus);
      return true;
    }
  }
}

namespace
{

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

// Join what the source applied most recently named for the rules of the run:
// first the rules that source spelled, then the rules of the last file it
// named, whichever of the two lines stood first (FR-063, spec D-23,
// recommendation 8). A source below the one that settled the list is read for
// its faults and then dropped, so every file its paths name is opened and the
// reader's own line stands while the rules it holds never join (FR-038,
// FR-061, FR-062). Either way the held text is cleared, so the next source
// starts from nothing of its own.
// The rules of one named file appended to the settled list. The read
// belongs to the reader the entry point hands, so the gather opens no file
// of its own, and the fault the reader answers is printed as it stands
// (FR-061, FR-064, FR-065, D-05).
auto commitRules(ResolvedOptions& resolved,
                 const OptionSource source,
                 RuleFileReader ruleFiles) -> std::expected<void, std::string>
{
  const bool higher =
      resolved.rulesSource.has_value() && *resolved.rulesSource != source;
  if (!higher) {
    std::move(resolved.stagedRules.begin(),
              resolved.stagedRules.end(),
              std::back_inserter(resolved.rules));
  }
  resolved.stagedRules.clear();
  std::optional<std::vector<CompareRule>> fileRules;
  for (const std::string& path : resolved.stagedRulesPaths) {
    SG_ASSERT(ruleFiles != nullptr,
              "a source that names a rules file hands its reader along with "
              "the path (D-05)");
    auto rules = ruleFiles(path);
    if (!rules.has_value()) {
      return std::unexpected(rules.error());
    }
    fileRules = std::move(*rules);
  }
  if (fileRules.has_value() && !higher) {
    std::move(fileRules->begin(),
              fileRules->end(),
              std::back_inserter(resolved.rules));
  }
  resolved.stagedRulesPaths.clear();
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
    // the environment names its variable (FR-031, FR-033, FR-035). The path
    // is quoted at this one place, so the origin a dropped name carries
    // reads as text wherever it is later printed (FR-027, FR-061).
    const std::string where =
        quotedText(file.path) + ":" + std::to_string(number);
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
      return std::unexpected(where + ": unknown option '" + quotedText(name)
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
        settledInSource(settled, *row) ? kSameSourceSettled : kNoPriorSettle;
    const auto applied = applySourceValue(
        *row, value, where, again, OptionSource::OPTION_FILE, resolved);
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
 * The command line stands first: the scan settled its rows through the same
 * seam every lower source goes through, so the gather starts from that state,
 * and a row the words settled keeps that value and answers no use (FR-037,
 * FR-038). The file lines follow, then the `SPEEDGUN_*` rows,
 * so the same precedence rule settles every source below the words. The
 * disclosed uses follow the table order of FR-039 and FR-040, whichever
 * source settled them and in whatever order the file lines stood.
 *
 * The option file arrives as the path and the whole text the entry point
 * read: the gather splits the text and opens no file, and a file the
 * system refused is answered at the entry point before the gather runs
 * (FR-012, FR-029, FR-034, D-05). The highest source that names rules sets
 * the whole list of FR-063, and the master default of FR-041 stays for every
 * option no source names.
 */
auto gatherOptionSources(const CommandLineOptions& scan,
                         const std::vector<EnvironmentRow>& environment,
                         const std::optional<OptionFileText>& optionFile,
                         const RuleFileReader ruleFiles)
    -> std::expected<ResolvedOptions, std::string>
{
  SG_REQUIRE(scan.stop == StopKind::NONE,
             "the gather settles a line the scan walked to its end");
  // The words are the highest source, and the scan settled each of them
  // through the one function that settles a row, so the gather takes that
  // state over as it stands and asks the sources below it: the marks of the
  // two rows that settle through a mark, the seed of FR-051, the metric names
  // with the empty origin their own line carries, and the compiled pattern of
  // the filter (FR-028, FR-033, FR-037, FR-038, FR-050, FR-051).
  //
  // The rules the words named stand staged, since the scan opens no file: the
  // file a word named joins after them here, where the reader the entry point
  // hands stands, and no lower source adds to the list the words settled
  // (FR-061, FR-063, D-05, spec D-23).
  ResolvedOptions resolved = scan.settled;
  const auto wordsJoined =
      commitRules(resolved, OptionSource::COMMAND_LINE, ruleFiles);
  if (!wordsJoined.has_value()) {
    return std::unexpected(wordsJoined.error());
  }

  // The file stands between the words and the environment (FR-037, D-1).
  if (optionFile.has_value()) {
    const auto read = applyOptionFile(*optionFile, resolved);
    if (!read.has_value()) {
      return std::unexpected(read.error());
    }
    // The last position of the order this round reads for the file: its own
    // rules first, then the rules of the one file its lines name (FR-063).
    const auto joined =
        commitRules(resolved, OptionSource::OPTION_FILE, ruleFiles);
    if (!joined.has_value()) {
      return std::unexpected(joined.error());
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

  for (const OptionRow& row : optionRows()) {
    if (!row.fromEnvironment) {
      continue;
    }
    const std::string* const value =
        environmentValue(environment, environmentName(row));
    if (value == nullptr) {
      // FR-026: the operator set no variable for this row, so the gather
      // asks nothing of it and the row keeps the value the higher sources
      // left, if any.
      continue;
    }
    if (value->empty()) {
      // FR-025, FR-027: the variable names its row and hands it nothing, so
      // the gather answers here ahead of the reader, and the line names the
      // variable the operator set. A word of the command line and a line of
      // a file keep the answer of their own row (FR-035, SC-003).
      return std::unexpected(environmentName(row) + ": --"
                             + std::string(row.name) + ": "
                             + std::string(kEmptyValueTail));
    }
    const std::string& text = *value;
    const auto settled = applySourceValue(row,
                                          text,
                                          environmentName(row),
                                          kNoPriorSettle,
                                          OptionSource::ENVIRONMENT,
                                          resolved);
    if (!settled.has_value()) {
      return std::unexpected(settled.error());
    }
    if (*settled) {
      resolved.uses.push_back(
          OptionUse {&row, OptionSource::ENVIRONMENT, text});
    }
  }

  // The two rules rows carry no environment name (FR-026), so the environment
  // names no rule and the join above, made for the option file, is the last
  // one the gather makes (FR-063).

  // FR-024, FR-050, FR-051: the pattern of the words is compiled where their
  // own row settles, in the settle function, so no lower source's fault can
  // hide it and the gather compiles nothing here (FR-037, FR-038).

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
