#ifndef SG_HARNESS_DETAIL_OPTIONS_HPP
#define SG_HARNESS_DETAIL_OPTIONS_HPP

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "internal.hpp"
#include "speedgun-ng/benchmark.hpp"

/**
 * @file options.hpp
 * @brief The option surface of specs/028-full-command-line: the one
 * option table, the scan of a command line, the gather over the three
 * sources, and the readers of the option values. Nothing here is public
 * API; the option surface the operator sees is the command line itself
 * (FR-076, D-24).
 */

namespace sg::detail
{

/**
 * @brief The value kind of one option row (E-01, FR-001).
 *
 * `OPTIONAL` stands for `--interleave` alone: that option takes a seed
 * when one stands attached to it and leaves a following word a separate
 * argument (spec D-17, FR-052).
 */
enum class OptionArgument
{
  NONE,
  REQUIRED,
  OPTIONAL,
};

/**
 * @brief Where a settled value came from (E-04, FR-037).
 *
 * The order of the three enumerators is the precedence the gather
 * applies (spec D-1).
 */
enum class OptionSource
{
  COMMAND_LINE,
  OPTION_FILE,
  ENVIRONMENT,
};

/**
 * @brief What the walk of the scan stopped at (E-06, FR-017, spec D-08).
 *
 * `NONE` means the walk met neither print option nor usage fault, so the
 * entry point goes on to run the batch.
 */
enum class StopKind
{
  NONE,
  HELP,
  VERSION,
};

// The internal value each option answers from the scan. A long option
// with no letter of its own takes a number of 256 or more, so it cannot
// collide with a short option, and no option a later feature adds can
// reach a switch arm by claiming a letter. The four carried values keep
// the options master gives them, the seven names this feature brings take
// 290 to 296, and 297 to 319 stay free (FR-001, FR-004, FR-005, D-24).
inline constexpr int kMetricMethodOption = 256;
inline constexpr int kCompareOptionValue = 257;
inline constexpr int kCompareRuleOptionValue = 258;
inline constexpr int kCompareStrictOptionValue = 259;
inline constexpr int kOptionsOptionValue = 290;
inline constexpr int kEnvOptionValue = 291;
inline constexpr int kThreadsOptionValue = 292;
inline constexpr int kInterleaveOptionValue = 293;
inline constexpr int kCompareRulesOptionValue = 294;
inline constexpr int kHelpOptionValue = 295;
inline constexpr int kVersionOptionValue = 296;

/**
 * @brief One row of the one option table (E-01, FR-001).
 *
 * `name` is the long spelling without the leading dashes, a string
 * literal so the pointer stays valid for the whole run and the `getopt`
 * scanner can read it. `value` is unique across the table (FR-004,
 * SC-001). `fromEnvironment` is set for exactly the twelve rows FR-020
 * names, and the environment name of such a row is `SPEEDGUN_` plus the
 * name upper-cased with `-` changed to `_`, computed from the row
 * (FR-019, D-09, D-27). `help` is the text `--help` prints and the guide
 * reference copies (FR-015, FR-018, FR-069), and `owner` names the
 * requirement that owns the row.
 */
struct OptionRow
{
  const char* name;
  int value;
  OptionArgument argument;
  bool fromEnvironment;
  const char* help;
  const char* owner;
};

/// @brief One `SPEEDGUN_*` name and the raw text it holds, as data the
/// gather can read (E-02, FR-019, FR-012).
struct EnvironmentRow
{
  std::string name;
  std::string value;
};

/// @brief One `--options` file as the entry point read it: the path the
/// option named and the whole text (E-03, FR-029, D-05).
struct OptionFileText
{
  std::string path;
  std::string text;
};

/**
 * @brief The disclosure entry of one option a source other than the
 * command line settled (E-05, FR-039, FR-040).
 *
 * `row` points into the table: the row's `name` and its position carry
 * the option text and the order of the disclosure. `value` is the text
 * as the source spelled it, and a repeatable joins its items with `;`.
 */
struct OptionUse
{
  const OptionRow* row;
  OptionSource source;
  std::string value;
};

/// @brief The interleaving state settled across the sources (E-10,
/// FR-051, FR-058). The bare option and `SPEEDGUN_INTERLEAVE=0` both
/// mean given with seed 0.
struct InterleaveSeed
{
  bool given = false;
  std::uint64_t seed = 0;
};

/// @brief What the scan of one command line answers: the options the
/// words named, the seed `--interleave` settled, the uses it recorded in
/// scan order, and the stop the walk met (E-06, FR-010, FR-017, FR-051).
struct CommandLineOptions
{
  RunOptions options;
  std::vector<OptionUse> uses;
  InterleaveSeed interleave;
  StopKind stop = StopKind::NONE;
};

/// @brief What the gather answers: every option at the value of the
/// highest source that named it, the disclosed uses in table order, the
/// seed, and the rules of all four places in spec D-23 order (E-07,
/// FR-037, FR-038, FR-039, FR-063).
struct ResolvedOptions
{
  RunOptions options;
  std::vector<OptionUse> uses;
  InterleaveSeed interleave;
  std::vector<CompareRule> rules;
  // The file a `compare-rules` line of the option file names, empty when it
  // names none. Its rules join after that file's own lines, the last
  // position of the order spec D-23 states (FR-063).
  std::string optionFileRulesPath;
  // The `--json-raw` row has no unset word: its master default is the same
  // value the word `none` answers, so the marker is what tells a source
  // that settled the row from one that never named it (FR-037, FR-050).
  bool rawContentSettled = false;
};

/// @brief The one option table, in the order
/// `specs/028-full-command-line/contracts/option-table.md` fixes it
/// (E-01, FR-001, FR-002, D-01, D-02).
[[nodiscard]] auto optionRows() -> std::span<const OptionRow>;

/// @brief The row of the table that carries `name`, or null when the table
/// holds none: the option file, the help printer and the disclosure tests
/// share this one lookup, so the table is walked in one place (FR-033,
/// FR-017).
[[nodiscard]] auto rowNamed(std::string_view name) -> const OptionRow*;

/**
 * @brief Scan one command line into the options its words name
 * (FR-006, FR-009 to FR-012, D-03, D-06).
 *
 * The scan sets `opterr` to 0 and takes the option string `":"`, hands
 * the `getopt_long` array built from the rows, and answers a usage fault
 * as one text the entry point prints at status 2. It calls no `std::exit`,
 * answers no status and prints nothing (FR-006, FR-010, FR-011).
 */
/// \pre `argc` stands at zero or above and `argv` carries that many words
/// plus the terminating null, the shape the process hands `main`.
/// \post A scan that walked the whole line names no stop, so only the two
/// words that ask for an answer end the run before it gathers options.
[[nodiscard]] auto scanCommandLine(int argc, char** argv)
    -> std::expected<CommandLineOptions, std::string>;

/**
 * @brief Print the help answer of `--help`: one line per table row in
 * table order, each built from that row's `name`, its argument kind and
 * its `help` text, so a row added to the table appears here with no
 * other edit (FR-015, FR-018).
 *
 * The entry point calls this when the scan answers `StopKind::HELP`; the
 * printer writes on standard output, calls no `std::exit` and answers no
 * status.
 */
auto printHelp() -> void;

/**
 * @brief Print the version answer of `--version`: one line naming the
 * project and the version the build defines (FR-016).
 *
 * The entry point calls this when the scan answers `StopKind::VERSION`;
 * the printer writes on standard output, calls no `std::exit` and
 * answers no status.
 */
auto printVersion() -> void;

/// @brief The `SPEEDGUN_*` names one environment table holds, read once by
/// the entry point and handed to the gather as data (E-02, FR-019, FR-026,
/// D-04). The table is the caller's: a null table holds no entry, so it
/// answers no row (FR-012).
[[nodiscard]] auto environmentRows(char* const* table)
    -> std::vector<EnvironmentRow>;

/// @brief The row one `NAME=TEXT` entry of the process environment carries,
/// or no row for an entry that holds no `=` or that the `SPEEDGUN_` prefix
/// does not cover (FR-021, FR-026).
[[nodiscard]] auto environmentRowOf(std::string_view entry)
    -> std::optional<EnvironmentRow>;

/// @brief The reader of one rules file, handed to the gather by the entry
/// point so the gather opens no file of its own: the path in, the rules of
/// that file out, or the one fault line that file answers (FR-061, FR-064,
/// FR-065, D-05).
using RuleFileReader = std::expected<std::vector<CompareRule>, std::string> (*)(
    const std::string& path);

/**
 * @brief Settle every option across the three sources and answer the
 * disclosed uses (FR-020 to FR-028, FR-029 to FR-036, FR-037 to FR-041,
 * D-04, D-10).
 *
 * The gather reads no process environment and opens no file: the option
 * file arrives as text, and a file the system refused is answered by the
 * entry point before the gather runs (FR-012, D-05). A source that names a
 * rules file names it as a path and `ruleFiles` is the reader that turns
 * that path into rules, so the order spec D-23 states settles here while
 * the read stays with the entry point (FR-061, FR-063, D-05).
 */
/// \pre `scan` is the answer of a scan that walked the whole line, so its
/// stop stands at `NONE`: a word that stops the run never reaches here.
/// \post The uses it discloses stand in table order, the order the console
/// line and the report state them in (FR-038, FR-039).
[[nodiscard]] auto gatherOptionSources(
    const CommandLineOptions& scan,
    const std::vector<EnvironmentRow>& environment,
    const std::optional<OptionFileText>& optionFile,
    RuleFileReader ruleFiles = nullptr)
    -> std::expected<ResolvedOptions, std::string>;

/// @brief The thread counts of one `--threads=` list: a comma-separated
/// sequence of counts and `min:max` ranges, the ranges expanded by
/// `buildThreadCounts` (E-11, FR-045, FR-046, D-14). The same reader
/// answers the `SPEEDGUN_THREADS` items.
/// \pre `text` is one value word of one source: the gather splits the
/// environment on `;` and the file on newlines before it hands a word here.
/// \post A refusal names the text the source spelled, the one line the
/// operator reads at status 2 (SC-003).
[[nodiscard]] auto readThreadList(std::string_view text)
    -> std::expected<std::vector<int>, std::string>;

/// @brief The seed of one `--interleave=` value or of
/// `SPEEDGUN_INTERLEAVE`: unsigned decimal digits alone, consumed whole,
/// at or below the largest 64-bit unsigned value (E-10, FR-051, FR-054).
/// \pre `text` is one value word of one source, so it carries no `;` and no
/// newline. The bare option carries no word at all and reaches no reader.
/// \post A refusal names the text the source spelled, the one line the
/// operator reads at status 2 (FR-054, SC-003).
[[nodiscard]] auto readSeed(std::string_view text)
    -> std::expected<std::uint64_t, std::string>;

/**
 * @brief The rules of one rules file: one rule per line in the
 * `--compare-rule` grammar of specs/026, a grammar that does not change
 * here (FR-061, D-13).
 *
 * A line whose first non-space character is `#` is a comment and a blank
 * line yields no rule, yet both hold a number, so the fault of a refused
 * line names the raw line an editor shows (FR-062, FR-064). The file holds
 * rules alone, so a line that names another file reaches the rule parser
 * and the parser refuses it (FR-061, PC-4). A file the system refuses
 * answers the option's own line, naming the path and the reason the open
 * answered (FR-065).
 */
[[nodiscard]] auto readCompareRulesFile(const std::string& path)
    -> std::expected<std::vector<CompareRule>, std::string>;

/// @brief The walk order of an interleaved run: a permutation of
/// `0 .. count - 1` that the same `count` and `seed` answer under every
/// standard library (FR-053, D-15, spec D-12).
/// \pre `count` stands above zero: it is the size of a selection the run is
/// about to walk, and an empty selection answers before the walk settles.
/// \post The answer holds each index of `0 .. count - 1` exactly once, so
/// every selected instance is walked once (FR-053, SC-005).
[[nodiscard]] auto shuffledOrder(std::size_t count, std::uint64_t seed)
    -> std::vector<std::size_t>;

}  // namespace sg::detail

#endif  // SG_HARNESS_DETAIL_OPTIONS_HPP
