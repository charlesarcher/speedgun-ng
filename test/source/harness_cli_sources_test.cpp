// ============================================================================
// TDD test for the usage faults of the command-line scan of
// specs/028-full-command-line (T010; FR-007 to FR-012, SC-003,
// contracts/option-table.md).
//
// The scan answers one fault text and prints nothing; the entry point turns
// that text into one line on standard error at status 2 (FR-011). This file
// compares the text the scan answers byte for byte and leaves the status to
// `harness_cli_test` and `base_cli_matrix`. Each text carries no newline,
// which is the shape of one line at this seam.
//
// The four texts the contract fixes:
//
//   unknown name      `unknown option '<text>'; --help lists the options`
//   name near a row   `unknown option '<text>', nearest '--<name>'`
//   ambiguous prefix  `ambiguous option '<text>': <candidate>, <candidate>`
//   option, no value  `option '--<name>' needs a value`
//
// The scan seam `detail::scanCommandLine` stands declared by T006 in
// `detail/options.hpp` and defined in `source/harness/options.cpp` by T011.
// The declaration below carries the seam until that header exists, and the
// target stays out of `test/CMakeLists.txt` until the task that lands the
// scan registers it, so the link error naming that seam is the red state of
// this task. `CommandLineOptions` is the scan result of E-06 and arrives in
// the same header. The placeholder here keeps the declaration well formed,
// and the branch above takes the real definition over as soon as the header
// exists, so one program never holds two definitions of the name.
//
// Every command line is an `argv` array built here, and every gather is
// handed its environment rows as data, so no gather case reads the process
// environment (FR-012). The two cases that need a variable of their own,
// the entry point and the reader of the process names, reach it through a
// child forked here and re-run with an explicit `envp` that carries the
// row, so this process never holds one of a test's own (D-04, FR-081).
//
// Check map:
//   theUnknownOptionNamesTheText
//                                `--zzzz` answers the unknown line naming
//                                the typed text, with no suggestion because
//                                no row name sits within two edits of it
//                                (FR-007, SC-003)
//   theNearestNameRidesTheUnknownLine
//                                `--filtr` answers the unknown line that
//                                ends with the nearest row name (FR-007)
//   theAmbiguousPrefixNamesBothCandidates
//                                each of the five prefixes that start more
//                                than one row name answers one line naming
//                                every candidate in table order (FR-009,
//                                FR-013)
//   theMissingValueNamesTheOption
//                                `--filter` with no value answers the
//                                needs-a-value line (FR-008)
//   theShortOptionAnswersTheUnknownLine
//                                `-x` answers the unknown line naming `-x`,
//                                the option string of the scan carrying no
//                                short letter (FR-005, FR-007)
//
// The environment cases of T022, T025 and T027 build `EnvironmentRow`
// vectors and hand them to `detail::gatherOptionSources`, the seam
// `contracts/environment.md` names, so no test writes the process
// environment and none reads it (FR-012, FR-081). The gather answers one
// fault text and no status; the entry point prints that text at status 2,
// which stays the reading of `harness_cli_test` and `base_cli_matrix`.
//
// Check map (the environment source):
//   theFlaggedRowsAnswerTheirEnvironmentName
//                                each of the twelve rows FR-020 marks
//                                settles from `SPEEDGUN_` plus the long
//                                name upper-cased with `-` changed to `_`,
//                                the name computed from the live row
//                                (FR-019, FR-020)
//   theUnflaggedRowsCarryNoEnvironmentName
//                                each of the fifteen rows FR-021 clears
//                                carries no flag, and its variable settles
//                                nothing (FR-021)
//   aRepeatableSplitsOnItsItems  one `;`-separated value answers one entry
//                                per item (FR-024)
//   anItemReachesTheCommandLineRule
//                                a refused item of a split value answers the
//                                reader's own line behind the variable name
//                                (FR-024, FR-027)
//   theFourFlagWordsAndTheirRefusal
//                                `1`, `0`, `true` and `false` answer the
//                                flag, and any other text answers the
//                                refusal line (FR-025). The row is the
//                                negating flag, so the word `1` turns
//                                percentiles off.
//   anUnknownVariableChangesNothing
//                                a `SPEEDGUN_*` name that matches no
//                                flagged row answers no line and settles
//                                nothing (FR-026)
//   aRefusedValueNamesTheVariableAndTheReaderLine
//                                a value the option's own reader refuses
//                                opens with the variable and carries the
//                                line the command line answers (FR-027)
//   theInterleaveVariableTurnsTheOptionOn
//                                `SPEEDGUN_INTERLEAVE` gives the option with
//                                the seed its reader answers, and `=0` means
//                                given with seed 0 (FR-022)
//   theEnvironmentPercentilePairStandsTogether
//                                the two percentile variables together
//                                answer the pair line even when the command
//                                line overrides one half (FR-023)
//   aCommandLineHalfLeavesThePairApart
//                                one half in the environment and the other
//                                on the command line answer no pair line,
//                                and the command line keeps the option
//                                (FR-023, FR-037)
//
// The option file cases of T032 build `OptionFileText` values here and hand
// them to `detail::gatherOptionSources`, the seam
// `contracts/option-file.md` names. The path of such a value stands nowhere,
// because the gather splits the text and opens no file (FR-012, D-05). Each
// fault of the file is one line the entry point prints at status 2, so this
// file compares the text and leaves the status to the entry-point case and
// to `base_cli_matrix`.
//
// Check map (the option file source):
//   anOptionFileLineSetsItsOption  one option per line in long spelling
//                                  without the leading dashes sets its
//                                  option, a comment and a blank line set
//                                  nothing, and the two uses come back in
//                                  table order (FR-029, FR-030, FR-039,
//                                  FR-040)
//   aRefusedNameNamesItsLine       `options`, `env`, `help` and `version`
//                                  answer the file, the line number and the
//                                  refused name (FR-031)
//   anUnknownNameNamesItsLine      a name with no row answers the file, the
//                                  number and the name, with no suggestion,
//                                  and the number counts the comment and the
//                                  blank lines before it (FR-030, FR-033)
//   aSecondOptionsNamesTheOption   a second `--options` answers one line
//                                  naming the option alone, with no path
//                                  repeated (FR-032)
//   aRepeatableAccumulatesInOrder  two lines of one repeatable keep both
//                                  items in line order and disclose them
//                                  joined with `;` (FR-035, FR-040)
//   aFlagLineReachesTheFlagReader  a flag line written `name=value` reaches
//                                  the flag reader of FR-025, whose refusal
//                                  stands behind the file and the number
//                                  (FR-025, FR-035)
//   aFileValueReachesTheReader     a value the reader refuses carries the
//                                  command line's own line behind the file
//                                  and the number (FR-035, FR-027)
//   anUnreadableFileAnswersOneLine the file the system cannot read answers
//                                  one line at status 2 at the entry point,
//                                  where the read stands (FR-034, SC-003,
//                                  SC-013)
//
// The precedence cases of T034 hand all three sources to one call of the
// gather, the shape SC-002 states, and read the value that takes effect
// together with the use the disclosure carries. The fourth source, the
// master default, is what stands when none of the three names the option,
// so its leg asks that the settled field hold nothing.
//
// Check map (the precedence of the four sources):
//   theThreeSourcesSettleInOrder one option named in the command line, the
//                                option file and the environment with three
//                                different values settles to the command
//                                line, then to the file with the words
//                                away, then to the environment with both
//                                away, and to the master default with
//                                neither, and the one disclosure entry
//                                names the source that won (FR-037, FR-039,
//                                FR-040, SC-002)
//   aHigherSourceReplacesTheWholeList
//                                a repeatable a higher source names replaces
//                                the whole list of the lower one and
//                                appends nothing to it, and the disclosed
//                                value carries the winner's items alone
//                                (FR-038)
//   theRuleListReplacesAcrossSources
//                                the comparison rule list is the one list
//                                that does not replace: the command line's
//                                rules stand first and the file's lines
//                                follow them (FR-038, FR-061, FR-063,
//                                spec D-23)
//   aFileLineThatStartsWithADashNamesItsText
//                                a line whose first character is `-` answers
//                                the file, the number and the text, and the
//                                table is never asked for that name (FR-036,
//                                SC-003)
//   aRequiredArgumentLineWithoutAnEqualsNamesTheOption
//                                a line of a required-argument row that
//                                carries no `=` answers the file, the number
//                                and the command line's own needs-a-value
//                                line, while a flag line written without `=`
//                                is the option standing (FR-029, FR-036)
//   aCommandLineOnlyRunDisclosesNothing
//                                a run that names its options on the command
//                                line alone, a file that names no option and
//                                an environment row that matches no flagged
//                                row all answer no use and no seed, which is
//                                the run that prints no disclosure line and
//                                writes no `run` object (FR-041)
// Check map (the closing pass of the coverage gate):
//   theEnvironmentSwitchLeavesTheProcessNamesUnread
//                                `off` is the only word `--env` takes, and the
//                                switched run answers what the same words
//                                answer with the variable absent (FR-028,
//                                SC-005)
//   theThreadListFaultStandsAtTheScan
//                                every list the reader refuses is the scan's
//                                own refusal, and an accepted list reaches the
//                                counts the reader answers (FR-044, FR-046)
//   aWordOfDashesAloneNamesNoOption
//                                `---` carries no row name, so no row is its
//                                candidate and the unknown line stands (FR-007,
//                                FR-009)
//   aLowerSourceYieldsToTheCommandLine
//                                each of the twelve flagged rows a command line
//                                settled is read again by neither the
//                                environment nor the file, and discloses no use
//                                of its own (FR-037, FR-040)
//   aRefusedValueOfEveryKindNamesItsSource
//                                a value its reader refuses is one fault for
//                                every kind of reader, with the variable or the
//                                file and number ahead of the reader's line
//                                (FR-027, FR-035, SC-003)
//   theLastLineOfASourceMayCarryNoBreak
//                                a last line that ends at its text settles its
//                                option, in the option file and in the rules
//                                file alike (FR-030, FR-061)
//   aBareSeedLineStandsForTheSeedZero
//                                a file line naming the seed alone turns the
//                                option on with the seed 0 and discloses that
//                                text (FR-029, FR-051, E-10)
//   everyRowOfTheTableSettlesItsRow
//                                each word a file line can carry settles
//                                from its own line and discloses that
//                                word, and a word its reader refuses
//                                answers one line naming the file and
//                                the number (FR-033, FR-035, FR-040,
//                                FR-050)
//   aGoodLineOfALowerSourceYieldsAndDrops
//                                a good line under a command-line word
//                                is read and dropped: the higher value
//                                stands and the drop discloses nothing
//                                (FR-035, FR-037, FR-040)
//   aShadowedBadValueStillNamesItsSource
//                                a value a higher source shadows is
//                                still read, so the bad word of a file
//                                line or a variable answers its own
//                                line (FR-027, FR-035, FR-037)
//   theEnvironmentWalkReadsTheTableItIsGiven
//                                a null table and an empty one answer no
//                                row, and a handed table answers the one
//                                entry the prefix covers (FR-012,
//                                FR-021, FR-026)
//   aBadLetterOfAShortGroupAfterALongOption
//                                the letter a short group is refused on
//                                is named by the group's own line behind
//                                a long option as it is alone (FR-005,
//                                FR-007)
//   aRulesLineBeforeItsRulesFileOfOneSourceJoins
//                                the rules of the file one source names join
//                                after the rules that source spelled, when
//                                the file line stands first and when it
//                                stands last (FR-038, FR-063)
//   anEmptyRulesFilePathNamesNoSource
//                                a rules line whose path stands empty opens
//                                no file and settles no source, so a later
//                                line of the same file settles the list
//                                (FR-041, FR-063)
//   aLowerRulesPathIsReadUnderACommandLineRule
//                                the rules path of a lower source is opened
//                                before its rules drop, so its own fault
//                                stands (FR-038, FR-061)
//   anEmptyCommandLineRulesFileSettlesTheList
//                                a rules file of no rule named by the words
//                                settles the empty list, and a later source
//                                adds no rule to it (FR-038, FR-063)
//   aRulesFileOfOneSourceTurnsTheGateFail
//                                the rule of the file an option file names
//                                is a rule of the run, so it decides the
//                                gate answer at the entry point (FR-063,
//                                FR-066)
//   aRulesFileTheReaderCannotOpenFaultsTheGather
//                                the reader the gather is handed answers its
//                                own line for a path it cannot open (FR-062,
//                                D-05)
//   aHeldValueOfALowerSourceNamesItsSource
//                                the bad filter, preset and metric word that
//                                the settle keeps as text answers one line
//                                behind its own source at status 2, settled
//                                or dropped, and so does a rules path it
//                                holds (FR-027, FR-033, FR-038)
//   anEmptyFilterWordSettlesTheRowItNames
//                                an empty filter word of the words or of a
//                                file line settles the row, so a lower
//                                source leaves the empty pattern standing
//                                (FR-035, FR-037, FR-050)
//   aLowerPercentileListIsCountedAlone
//                                the cap of sixteen points counts the points
//                                of the source that settles, so two lists
//                                that stand on their own settle to the
//                                higher source and a list of seventeen
//                                points answers behind its own source
//                                (FR-022, FR-038)
//   theBadValueOfEverySourceSharesTheWordsLine
//                                one bad value of each value row given to the
//                                words, the file and the variable answers one
//                                line, and the line of a lower source is the
//                                line of the words behind that source's own
//                                prefix (FR-024, FR-027, FR-031, FR-046)
//   theBareFlagOfEverySourceSettlesItsRow
//                                the six rows that carry no value settle from
//                                the bare word and the bare line, and a text
//                                that is not a flag answers behind the source
//                                that holds it (FR-025, FR-029, FR-033)
//   theFourRefusedRowsAnswerTheirOwnLines
//                                the two rows that would re-root a file's own
//                                sources and the two that print keep the
//                                lines of the scan, and a print row stops
//                                the walk where it stands (FR-015 to FR-017,
//                                FR-028, FR-031, FR-032)
//   theEnvironmentRowOfOneProcessName
//                                one process name answers a row only when it
//                                carries the prefix and an `=`, and the rows of
//                                the process carry the name it holds (FR-018,
//                                FR-019)
//   everyRefusedRowLineFaultsTheEntry
//                                the refused word of each of those nine
//                                rows answers the file and its number at
//                                status 2 at the entry point, with no
//                                benchmark row (FR-035, FR-050, SC-003)
//   aWordOfTwoValuesFaultsTheEntry
//                                a value word of two values from any of
//                                the three sources answers one line at
//                                status 2, never a contract failure
//                                (FR-024, FR-027, FR-035, FR-046,
//                                SC-003)
//   aSourceLineWithANulByteIsRefused
//                                a NUL byte of an option file line or a
//                                rules file line is refused behind that
//                                file and its number at status 2
//                                (FR-033, FR-061, FR-065, SC-003)
//   aRefusedThreadRowFaultsTheEntry
//                                the list that SPEEDGUN_THREADS or an option
//                                file line refuses answers one line naming its
//                                source at status 2 at the entry point, with
//                                no benchmark row (FR-027, FR-035, FR-044,
//                                SC-003)
//   aRefusedSeedRowFaultsTheEntry
//                                the seed that SPEEDGUN_INTERLEAVE or an
//                                option file line refuses answers one line
//                                naming its source at status 2 at the entry
//                                point, with no benchmark row (FR-027,
//                                FR-035, FR-054, SC-003)
// ============================================================================

#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <expected>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#if __has_include("detail/options.hpp")
#  include "detail/options.hpp"
#else
namespace sg::detail
{
// The scan result of E-06 as a placeholder, so the declaration below is
// well formed: this test reads the fault text and never the accepted value.
// T006 writes the real definition in `detail/options.hpp`.
struct CommandLineOptions
{
};

[[nodiscard]] auto scanCommandLine(int argc, char** argv)
    -> std::expected<CommandLineOptions, std::string>;
}  // namespace sg::detail
#endif

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

// One command line through the scan, in the shape a shell hands to `main`.
auto scanOf(std::initializer_list<const char*> words)
    -> std::expected<sg::detail::CommandLineOptions, std::string>
{
  const std::vector<std::string> owned(words.begin(), words.end());
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("speedgun"));
  for (const auto& word : owned) {
    argv.push_back(const_cast<char*>(word.c_str()));
  }
  return sg::detail::scanCommandLine(static_cast<int>(argv.size()),
                                     argv.data());
}

// The same command line read for its fault text alone: the answer is the
// fault text, or the sentence that says the scan accepted the words, so a
// mismatch reports which of the two happened.
auto faultOf(std::initializer_list<const char*> words) -> std::string
{
  const auto answer = scanOf(words);
  return answer.has_value() ? "the scan answered no fault" : answer.error();
}

// One row of the contract table: the command line, the text the scan must
// answer byte for byte, and the check that the answer holds one line.
auto expectFault(std::initializer_list<const char*> words,
                 const std::string& expected,
                 const char* what) -> void
{
  const std::string actual = faultOf(words);
  if (actual != expected) {
    report(what, expected, actual);
    return;
  }
  check(actual.find('\n') == std::string::npos,
        "that fault is one line (FR-007 to FR-009)");
}

auto theUnknownOptionNamesTheText() -> void
{
  expectFault({"--zzzz"},
              "unknown option '--zzzz'; --help lists the options",
              "an unknown name answers the unknown line naming the text "
              "(FR-007, SC-003)");
}

auto theNearestNameRidesTheUnknownLine() -> void
{
  expectFault(
      {"--filtr"},
      "unknown option '--filtr', nearest '--filter'",
      "a name within two edits of a row answers the nearest line " "(FR-007)");
}

// One abbreviation of the table and the one line the scan answers for it,
// both captured from the built binary.
struct AbbreviationCase
{
  const char* text;
  const char* answer;
};

// The check each row below carries, standing behind the prefix it pins.
constexpr char kAmbiguousFault[] =
    "a prefix of several rows names every candidate in table "
    "order (FR-009, FR-013): ";

// Every prefix of the 27-row table that starts more than one row name, each
// answer captured byte for byte from `docs_command_line`, which prints the
// line on standard error and answers status 2.
constexpr AbbreviationCase kAmbiguous[] = {
    {"--i",
     "ambiguous option '--i': iterations, " "interleave"},
    {"--t",
     "ambiguous option '--t': topology, " "threads"},
    {"--compare-r",
     "ambiguous option '--compare-r': compare-rule, " "compare-rules"},
    {"--compare-ru",
     "ambiguous option '--compare-ru': compare-rule, " "compare-rules"},
    {"--compare-rul",
     "ambiguous option '--compare-rul': compare-rule, " "compare-rules"},
};

auto theAmbiguousPrefixNamesBothCandidates() -> void
{
  for (const AbbreviationCase& row : kAmbiguous) {
    const std::string what = std::string(kAmbiguousFault) + row.text;
    expectFault({row.text}, row.answer, what.c_str());
  }
}

auto theMissingValueNamesTheOption() -> void
{
  expectFault({"--filter"},
              "option '--filter' needs a value",
              "an option given with no value answers the needs-a-value line "
              "(FR-008)");
}

auto theShortOptionAnswersTheUnknownLine() -> void
{
  expectFault({"-x"},
              "unknown option '-x'; --help lists the options",
              "a single dash letter answers the unknown line (FR-005, FR-007)");
}

// ===========================================================================
// The environment source of `contracts/environment.md`. The rows reach the
// gather as data, so no test writes the process environment and none reads
// it (FR-012, FR-081).
// ===========================================================================

// One long name and a value its own reader accepts, so the variable of that
// name settles the option when the row carries the environment flag and
// changes nothing when it does not.
struct NameCase
{
  const char* name;
  const char* value;
};

// The twelve rows FR-020 marks with the environment flag, in the order the
// requirement names them.
constexpr NameCase kFlagged[] = {
    {"filter", "one"},
    {"repetitions", "3"},
    {"min-time", "0.5"},
    {"iterations", "7"},
    {"warmup-time", "2"},
    {"counter", "cycles"},
    {"metric", "ipc"},
    {"percentiles", "90"},
    {"no-percentiles", "true"},
    {"placement", "core"},
    {"threads", "2"},
    {"interleave", "7"},
};

// The fifteen rows FR-021 clears: the rows that carry no environment
// name.
constexpr NameCase kUnflagged[] = {
    {"list", "1"},
    {"catalog", "1"},
    {"topology", "1"},
    {"dry-run", "1"},
    {"json", "report.json"},
    {"json-raw", "none"},
    {"compare", "base.json,run.json"},
    {"compare-rule", "time@median:lower:5%"},
    {"compare-strict", "1"},
    {"compare-rules", "rules.txt"},
    {"metric-method", "auto"},
    {"options", "options.txt"},
    {"env", "off"},
    {"help", "1"},
    {"version", "1"},
};

// The variable FR-019 computes from a row: `SPEEDGUN_` plus the long name
// upper-cased with every `-` changed to `_`.
auto variableFor(const std::string_view name) -> std::string
{
  std::string variable = "SPEEDGUN_";
  for (const char letter : name) {
    variable.push_back(letter == '-'
                           ? '_'
                           : static_cast<char>(std::toupper(
                                 static_cast<unsigned char>(letter))));
  }
  return variable;
}

// The row of the live table that carries this long name, or null when the
// table holds none. Every expectation below is built from the row, so a row
// that moves moves the expectation with it. The lookup stands in one helper
// of the library, which is the one place the table is walked (FR-017,
// FR-033).
auto liveRow(const std::string_view name) -> const sg::detail::OptionRow*
{
  return sg::detail::rowNamed(name);
}

// One call of the gather: the command line words, the environment rows, and
// no option file. The answer holds the settled options or the one fault
// text, never both.
struct Gathered
{
  std::optional<sg::detail::ResolvedOptions> settled;
  std::string fault;
};

// The path every option-file expectation names. No file of this name exists
// anywhere the test runs, which is what proves the gather opens nothing.
constexpr const char* kOptionFile = "options-file.txt";

// The gather over the sources one call hands it as data: the command line
// words, the environment rows and the text of the option file, each left
// out of the call when that source stands absent (E-02, E-03, E-06, E-07).
auto gatherSources(std::initializer_list<const char*> words,
                   std::vector<sg::detail::EnvironmentRow> environment,
                   const std::optional<std::string>& fileText,
                   sg::detail::RuleFileReader ruleFiles = nullptr) -> Gathered
{
  Gathered gathered;
  const auto scan = scanOf(words);
  if (!scan.has_value()) {
    gathered.fault = "the scan refused the command line: " + scan.error();
    return gathered;
  }
  std::optional<sg::detail::OptionFileText> file;
  if (fileText.has_value()) {
    file = sg::detail::OptionFileText {kOptionFile, *fileText};
  }
  const auto answer =
      sg::detail::gatherOptionSources(*scan, environment, file, ruleFiles);
  if (answer.has_value()) {
    gathered.settled = *answer;
  } else {
    gathered.fault = answer.error();
  }
  return gathered;
}

auto gather(std::initializer_list<const char*> words,
            std::vector<sg::detail::EnvironmentRow> environment) -> Gathered
{
  return gatherSources(words, std::move(environment), std::nullopt);
}

// The one use the gather answers for an option a source other than the
// command line settled (E-05): it names the row, the source and the text.
auto expectUse(const sg::detail::ResolvedOptions& settled,
               const NameCase& wanted,
               const std::string& variable) -> void
{
  const std::string label = "the variable " + variable + " settles the row "
      + wanted.name + " (FR-019, FR-020)";
  if (settled.uses.size() != 1) {
    report(label.c_str(),
           "one use naming the row",
           std::to_string(settled.uses.size()) + " uses");
    return;
  }
  const sg::detail::OptionUse& use = settled.uses.front();
  check(use.row != nullptr && std::string_view(use.row->name) == wanted.name,
        (label + ": the row the use names").c_str());
  check(use.source == sg::detail::OptionSource::ENVIRONMENT,
        (label + ": the source the use names").c_str());
  check(use.value == wanted.value,
        (label + ": the text the use carries").c_str());
}

// Two point lists hold the same points in the same order.
auto sameValues(const std::vector<double>& values,
                std::initializer_list<double> wanted) -> bool
{
  return values == std::vector<double>(wanted);
}

auto theFlaggedRowsAnswerTheirEnvironmentName() -> void
{
  for (const NameCase& wanted : kFlagged) {
    const std::string variable = variableFor(wanted.name);
    const sg::detail::OptionRow* row = liveRow(wanted.name);
    if (row == nullptr) {
      fail((std::string("the table carries no row named ") + wanted.name
            + " (FR-020)")
               .c_str());
      continue;
    }
    check(row->fromEnvironment,
          (std::string("the row ") + wanted.name
           + " carries the environment flag (FR-020)")
              .c_str());
    const Gathered gathered = gather({}, {{variable, wanted.value}});
    if (!gathered.settled.has_value()) {
      report(
          ("the variable " + variable + " reaches the gather (FR-019)").c_str(),
          "the gather answers no fault",
          gathered.fault);
      continue;
    }
    expectUse(*gathered.settled, wanted, variable);
  }
}

auto theUnflaggedRowsCarryNoEnvironmentName() -> void
{
  for (const NameCase& wanted : kUnflagged) {
    const std::string variable = variableFor(wanted.name);
    const sg::detail::OptionRow* row = liveRow(wanted.name);
    if (row == nullptr) {
      fail((std::string("the table carries no row named ") + wanted.name
            + " (FR-021)")
               .c_str());
      continue;
    }
    check(!row->fromEnvironment,
          (std::string("the row ") + wanted.name
           + " carries no environment flag (FR-021)")
              .c_str());
    const Gathered gathered =
        gather({}, {{variable, wanted.value}, {"SPEEDGUN_FILTER", "one"}});
    if (!gathered.settled.has_value()) {
      report(("the variable " + variable + " is refused for " + wanted.name
              + " (FR-021)")
                 .c_str(),
             "the gather answers no fault",
             gathered.fault);
      continue;
    }
    // A flagged row beside the unflagged one covers both halves: the
    // anchor settles, and the unflagged variable adds no use beside it.
    const std::string label =
        "the variable " + variable + " settles nothing (FR-021)";
    const auto& uses = gathered.settled->uses;
    if (uses.size() != 1 || uses.front().row == nullptr
        || std::string_view(uses.front().row->name) != "filter")
    {
      report(label.c_str(),
             "one use, that of the anchor SPEEDGUN_FILTER",
             std::to_string(uses.size()) + " uses");
      continue;
    }
    check(gathered.settled->options.filter == "one",
          (label + ": the anchor still settles (FR-019)").c_str());
  }
}

auto aRepeatableSplitsOnItsItems() -> void
{
  const Gathered counters =
      gather({}, {{"SPEEDGUN_COUNTER", "cycles;instructions"}});
  check(counters.settled.has_value()
            && counters.settled->options.counterAddresses
                == std::vector<std::string> {"cycles", "instructions"},
        "SPEEDGUN_COUNTER answers one counter per item (FR-024)");
  const Gathered metrics =
      gather({}, {{"SPEEDGUN_METRIC", "ipc;branch-misses"}});
  check(metrics.settled.has_value()
            && metrics.settled->options.metricNames
                == std::vector<std::string> {"ipc", "branch-misses"},
        "SPEEDGUN_METRIC answers one metric per item (FR-024)");
  const Gathered points = gather({}, {{"SPEEDGUN_PERCENTILES", "50;90"}});
  check(
      points.settled.has_value()
          && sameValues(points.settled->options.percentilePoints, {50.0, 90.0}),
      "SPEEDGUN_PERCENTILES answers one point per item (FR-024)");
}

auto anItemReachesTheCommandLineRule() -> void
{
  // The second item of the list reaches the reader the command line uses,
  // so the refusal is that reader's own line with the variable ahead of it.
  const Gathered gathered = gather({}, {{"SPEEDGUN_THREADS", "2;x"}});
  const auto refused = sg::detail::readThreadList("x");
  const std::string wanted = "SPEEDGUN_THREADS: " + refused.error();
  if (gathered.fault != wanted) {
    report("one refused item answers the reader's line naming that item "
           "(FR-024, FR-027)",
           wanted,
           gathered.fault);
  }
}

auto theFourFlagWordsAndTheirRefusal() -> void
{
  // The four words and the percentile state each answers. The word is the
  // truth of the negating flag, so `1` and `true` turn percentiles off, the
  // same answer the scan's `--no-percentiles` arm gives (FR-025).
  struct FlagWord
  {
    const char* text;
    bool percentiles;
  };

  for (const FlagWord& word : {FlagWord {"1", false},
                               FlagWord {"0", true},
                               FlagWord {"true", false},
                               FlagWord {"false", true}})
  {
    const Gathered gathered =
        gather({}, {{"SPEEDGUN_NO_PERCENTILES", word.text}});
    check(gathered.settled.has_value()
              && gathered.settled->options.showPercentiles.has_value()
              && *gathered.settled->options.showPercentiles == word.percentiles,
          ("SPEEDGUN_NO_PERCENTILES=" + std::string(word.text)
           + " leaves percentiles " + (word.percentiles ? "on" : "off")
           + " (FR-025)")
              .c_str());
  }
  const Gathered gathered = gather({}, {{"SPEEDGUN_NO_PERCENTILES", "yes"}});
  if (gathered.fault
      != "SPEEDGUN_NO_PERCENTILES: expected 1, 0, true or false, got 'yes'")
  {
    report("a flag value outside the four words answers the refusal line "
           "(FR-025, SC-003)",
           "SPEEDGUN_NO_PERCENTILES: expected 1, 0, true or false, got 'yes'",
           gathered.fault);
  }
}

auto anUnknownVariableChangesNothing() -> void
{
  for (const char* const name :
       {"SPEEDGUN_NOT_AN_OPTION", "SPEEDGUN_FILTERS", "SPEEDGUN_JSON_PATH"})
  {
    const Gathered gathered =
        gather({}, {{name, "1"}, {"SPEEDGUN_FILTER", "one"}});
    if (!gathered.settled.has_value()) {
      report((std::string(name) + " answers no line (FR-026)").c_str(),
             "the gather answers no fault",
             gathered.fault);
      continue;
    }
    // The unknown name is skipped: the flagged row beside it
    // still settles, and the unknown one adds no use of its own.
    const auto& uses = gathered.settled->uses;
    if (uses.size() != 1 || uses.front().row == nullptr
        || std::string_view(uses.front().row->name) != "filter")
    {
      report((std::string(name) + " settles nothing (FR-026)").c_str(),
             "one use, that of the anchor SPEEDGUN_FILTER",
             std::to_string(uses.size()) + " uses");
      continue;
    }
    check(gathered.settled->options.filter == "one",
          (std::string(name) + " leaves the flagged row settling (FR-026)")
              .c_str());
  }
}

auto aRefusedValueNamesTheVariableAndTheReaderLine() -> void
{
  // The expectation is the line the same text answers on the command line,
  // with the variable ahead of it, so the reader's own words are never
  // pinned here (FR-027, D-11).
  for (const NameCase& wanted :
       {NameCase {"min-time", "banana"}, NameCase {"repetitions", "-3"}})
  {
    const std::string variable = variableFor(wanted.name);
    const Gathered gathered = gather({}, {{variable, wanted.value}});
    const std::string word =
        "--" + std::string(wanted.name) + "=" + wanted.value;
    const std::string expected = variable + ": " + faultOf({word.c_str()});
    if (gathered.fault != expected) {
      const std::string what = "a refused " + variable
                               + " value names the variable and carries the "
                                 "reader's line (FR-027, SC-003)";
      report(what.c_str(), expected, gathered.fault);
    }
  }
}

auto theInterleaveVariableTurnsTheOptionOn() -> void
{
  const Gathered seeded = gather({}, {{"SPEEDGUN_INTERLEAVE", "7"}});
  const auto seed = sg::detail::readSeed("7");
  check(seeded.settled.has_value() && seeded.settled->interleave.given,
        "SPEEDGUN_INTERLEAVE turns the option on (FR-022)");
  check(seeded.settled.has_value()
            && seeded.settled->interleave.seed == seed.value(),
        "SPEEDGUN_INTERLEAVE carries the seed its reader answers (FR-022)");
  const Gathered zero = gather({}, {{"SPEEDGUN_INTERLEAVE", "0"}});
  check(zero.settled.has_value() && zero.settled->interleave.given
            && zero.settled->interleave.seed == 0,
        "SPEEDGUN_INTERLEAVE=0 means given with seed 0 (FR-022)");
}

auto theEnvironmentPercentilePairStandsTogether() -> void
{
  const std::vector<sg::detail::EnvironmentRow> pair = {
      {"SPEEDGUN_PERCENTILES", "50"},
      {"SPEEDGUN_NO_PERCENTILES", "1"},
  };
  // The pair is read over the environment rows alone and ahead of the
  // precedence rule, so a command line that overrides one half leaves the
  // fault standing (FR-023, spec D-5, spec D-27).
  for (const bool overridden : {false, true}) {
    const Gathered gathered = gather(
        overridden ? std::initializer_list<const char*> {"--percentiles=25"}
                   : std::initializer_list<const char*> {},
        pair);
    if (gathered.fault
        != "SPEEDGUN_PERCENTILES and SPEEDGUN_NO_PERCENTILES cannot both "
           "stand")
    {
      report(overridden
                 ? "the environment pair stands beside a command-line half "
                   "(FR-023)"
                 : "the environment pair answers one line (FR-023, SC-003)",
             "SPEEDGUN_PERCENTILES and SPEEDGUN_NO_PERCENTILES cannot both "
             "stand",
             gathered.fault);
    }
  }
}

auto aCommandLineHalfLeavesThePairApart() -> void
{
  const Gathered gathered =
      gather({"--percentiles=25"}, {{"SPEEDGUN_NO_PERCENTILES", "1"}});
  if (!gathered.settled.has_value()) {
    report("one half in the environment and the other on the command line "
           "answer no pair line (FR-023)",
           "the gather answers no fault",
           gathered.fault);
    return;
  }
  check(gathered.settled->options.showPercentiles.has_value()
            && *gathered.settled->options.showPercentiles,
        "the command-line half keeps the percentile option (FR-037)");
  check(sameValues(gathered.settled->options.percentilePoints, {25.0}),
        "the command-line half carries its points (FR-037)");
}

// ===========================================================================
// The option file source of `contracts/option-file.md`. The gather takes the
// file as the path and the whole text the entry point read (E-03, D-05), so
// these cases build the text here and name a path that stands nowhere: the
// gather splits the text and opens no file (FR-012). The unreadable file is
// the one fault of the six the gather cannot answer, because the read stands
// at the entry point, so that case drives `sg::speedgunMain` in process the
// way the table test drives it.
// ===========================================================================

// One call of the gather with an option file: the command line words, no
// environment row, and the text the entry point would have read.
auto gatherFile(std::initializer_list<const char*> words,
                const std::string& text) -> Gathered
{
  return gatherSources(words, {}, text);
}

// The text one option file answers, or the sentence that says the gather
// settled, so a mismatch reports which of the two happened.
auto fileFaultOf(const std::string& text) -> std::string
{
  const Gathered gathered = gatherFile({}, text);
  return gathered.settled.has_value() ? "the gather answered no fault"
                                      : gathered.fault;
}

// One row of the contract's fault table: the text the gather must answer
// byte for byte, and the check that the answer holds one line (FR-031,
// FR-033, SC-003).
auto expectFileFault(const std::string& text,
                     const std::string& expected,
                     const char* what) -> void
{
  const std::string actual = fileFaultOf(text);
  if (actual != expected) {
    report(what, expected, actual);
    return;
  }
  check(actual.find('\n') == std::string::npos,
        "that option file fault is one line (FR-031, FR-033)");
}

// One run of the entry point with both output streams captured to files, so
// the printed text and the answered status are read together and a fault
// line stays out of this file's own verdict output. The capture files stand
// in the system temporary directory under this process number, so two runs
// never share one.
struct Run
{
  int status = 0;
  std::string out;
  std::string err;
};

auto capturePath(const char* kind) -> std::filesystem::path
{
  return std::filesystem::temp_directory_path()
      / ("harness_cli_sources_" + std::to_string(static_cast<long>(getpid()))
         + kind);
}

auto readCapture(const std::filesystem::path& path) -> std::string
{
  std::ifstream stream(path);
  std::ostringstream text;
  text << stream.rdbuf();
  return text.str();
}

auto runEntry(std::initializer_list<std::string> words) -> Run
{
  std::vector<std::string> owned(words.begin(), words.end());
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_cli_sources_test"));
  for (std::string& word : owned) {
    argv.push_back(word.data());
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
  run.out = readCapture(outPath);
  run.err = readCapture(errPath);
  std::filesystem::remove(outPath);
  std::filesystem::remove(errPath);
  return run;
}

// The word that turns a child of this binary into one of the two helpers
// below, so a child answers that helper and answers nothing else; the check
// list of this file runs only in the parent.
constexpr const char* kChildWord = "--sources-test-child";

// One child of this binary, execved with the child word, then `words`, and
// with the names of this process plus the environment rows of `rows`. The
// answer is the exit code of the child, or -1 when it could not be run or
// waited for at all (D-04).
auto runChild(std::vector<std::string> words,
              std::initializer_list<std::string> rows) -> int
{
  std::vector<std::string> names;
  for (char** entry = environ; entry != nullptr && *entry != nullptr; ++entry) {
    names.emplace_back(*entry);
  }
  for (const std::string& row : rows) {
    names.push_back(row);
  }

  std::vector<char*> argv {const_cast<char*>("harness_cli_sources_test"),
                           const_cast<char*>(kChildWord)};
  for (std::string& word : words) {
    argv.push_back(word.data());
  }
  argv.push_back(nullptr);

  std::vector<char*> envp;
  for (std::string& name : names) {
    envp.push_back(name.data());
  }
  envp.push_back(nullptr);

  std::fflush(stdout);
  const pid_t child = fork();
  if (child < 0) {
    fail("a child of this binary can be forked");
    return -1;
  }
  if (child == 0) {
    execve("/proc/self/exe", argv.data(), envp.data());
    _exit(127);
  }
  int status = 0;
  if (waitpid(child, &status, 0) != child) {
    fail("the forked child can be waited for");
    return -1;
  }
  return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

// One run of the entry point in a child whose environment carries `rows`: the
// child answers the status and writes its two streams to files this process
// reads back, so the row reaches the harness through the child's environment
// and this process never carries it (FR-012, FR-027, FR-081).
auto runEntryWithEnv(std::initializer_list<std::string> words,
                     std::initializer_list<std::string> rows) -> Run
{
  const std::filesystem::path outPath = capturePath(".child.out");
  const std::filesystem::path errPath = capturePath(".child.err");
  std::vector<std::string> childWords {
      "entry", outPath.string(), errPath.string()};
  for (const std::string& word : words) {
    childWords.push_back(word);
  }

  Run run;
  run.status = runChild(std::move(childWords), rows);
  run.out = readCapture(outPath);
  run.err = readCapture(errPath);
  std::filesystem::remove(outPath);
  std::filesystem::remove(errPath);
  return run;
}

// What a child of this binary does in place of the checks: run the entry point
// over the words it carries, or answer whether the rows of its own environment
// carry one name and value. The answer of each is the exit code, and a word
// that names no helper answers 125.
auto runChildMode(int argc, char** argv) -> std::optional<int>
{
  if (argc < 2 || std::string_view(argv[1]) != kChildWord) {
    return std::nullopt;
  }
  if (argc >= 5 && std::string_view(argv[2]) == "entry") {
    if (std::freopen(argv[3], "w", stdout) == nullptr
        || std::freopen(argv[4], "w", stderr) == nullptr)
    {
      return 126;
    }
    const int status = sg::speedgunMain(argc - 4, argv + 4);
    std::fflush(stdout);
    std::fflush(stderr);
    return status;
  }
  if (argc == 5 && std::string_view(argv[2]) == "row") {
    for (const sg::detail::EnvironmentRow& row :
         sg::detail::environmentRows(environ))
    {
      if (row.name == argv[3] && row.value == argv[4]) {
        return 0;
      }
    }
    return 1;
  }
  return 125;
}

// The answer shape SC-003 pins for every source fault: status 2, exactly one
// line across the two streams, that one line carrying each of `needles`, and
// no benchmark row on standard output, neither the column header line nor an
// `iterations=` summary. The compare-mode tests call this shape
// `oneFaultLine`; it stands once above the entry-point cases that read it
// (FR-027, FR-035, SC-003).
auto oneFaultLine(const Run& run,
                  const char* what,
                  std::initializer_list<std::string_view> needles) -> void
{
  check(run.status == 2,
        (std::string(what) + " answers status 2 (SC-003)").c_str());
  const std::string both = run.out + "\n" + run.err;
  std::vector<std::string_view> textLines;
  for (std::size_t at = 0; at < both.size();) {
    const std::size_t breakAt = both.find('\n', at);
    const std::size_t end =
        breakAt == std::string::npos ? both.size() : breakAt;
    if (end != at) {
      textLines.push_back(std::string_view(both).substr(at, end - at));
    }
    at = end + 1;
  }
  if (textLines.size() != 1) {
    report(what,
           "exactly one line across both streams",
           std::to_string(textLines.size()) + " lines: " + both);
    return;
  }
  for (const std::string_view needle : needles) {
    if (textLines.front().find(needle) == std::string_view::npos) {
      report(what,
             "the one line carries '" + std::string(needle) + "'",
             std::string(textLines.front()));
    }
  }
  check(run.out.find("benchmark") == std::string::npos
            && run.out.find("iterations=") == std::string::npos,
        (std::string(what) + " runs no benchmark (SC-003)").c_str());
}

auto anOptionFileLineSetsItsOption() -> void
{
  // One option per line in long spelling without the leading dashes, as
  // `name=value`, beside a comment and two blank lines that set nothing
  // (FR-029, FR-030). The two option lines stand in the reverse of table
  // order, so the two uses also carry the disclosure order of FR-039.
  const Gathered gathered =
      gatherFile({}, "# the run's filter\n\n\t\nfilter=one\niterations=7\n");
  if (!gathered.settled.has_value()) {
    report("a file line in long spelling sets its option (FR-029, FR-035)",
           "the gather answers no fault",
           gathered.fault);
    return;
  }
  const sg::detail::ResolvedOptions& settled = *gathered.settled;
  check(settled.options.filter == "one",
        "the filter line sets the filter (FR-029, FR-035)");
  check(settled.options.fixedIterations.has_value()
            && *settled.options.fixedIterations == 7,
        "the iterations line sets the count (FR-029, FR-035)");
  if (settled.uses.size() != 2) {
    report("one file line answers one disclosed use (FR-039)",
           "two uses",
           std::to_string(settled.uses.size()) + " uses");
    return;
  }
  check(
      settled.uses.front().row != nullptr
          && std::string_view(settled.uses.front().row->name) == "filter"
          && settled.uses.back().row != nullptr
          && std::string_view(settled.uses.back().row->name) == "iterations",
      "the two uses stand in table order, whatever order the lines stood "
      "(FR-039, FR-040)");
  for (const sg::detail::OptionUse& use : settled.uses) {
    check(use.source == sg::detail::OptionSource::OPTION_FILE,
          "the use names the option file as its source (FR-039)");
  }
  check(settled.uses.front().value == "one" && settled.uses.back().value == "7",
        "each use carries the text of its own line (FR-040)");
}

auto aRefusedNameNamesItsLine() -> void
{
  // The file cannot re-root its own sources and cannot print, so the four
  // names are refused by name ahead of any value rule (FR-031, D-12).
  for (const char* const name : {"options", "env", "help", "version"}) {
    const std::string expected = std::string(kOptionFile) + ":2: the option '"
        + name + "' cannot stand in an option file";
    expectFileFault(std::string("filter=one\n") + name + "=1\n",
                    expected,
                    "a refused name answers the file, the number and the name "
                    "(FR-031, SC-003)");
  }
}

auto anUnknownNameNamesItsLine() -> void
{
  // The table is the only place an option is recognized, and the number
  // counts the comment and the two blank lines that stand before the fault,
  // so the line names 5 and carries no suggestion (FR-030, FR-033, D-12).
  expectFileFault("# comment\n\n \nfilter=one\npercentil=90\n",
                  std::string(kOptionFile) + ":5: unknown option 'percentil'",
                  "a name with no row answers the file, the number and the "
                  "name (FR-033, SC-003)");
}

auto aSecondOptionsNamesTheOption() -> void
{
  // One option file per run: the second word answers one line naming the
  // option, and the first file's path is not repeated (FR-032, D-9).
  const std::string actual =
      faultOf({"--options=first-options.txt", "--options=second-options.txt"});
  if (actual != "--options cannot stand twice") {
    report("a second --options answers one line naming the option (FR-032)",
           "--options cannot stand twice",
           actual);
    return;
  }
  check(actual.find("first-options.txt") == std::string::npos
            && actual.find("second-options.txt") == std::string::npos,
        "the line names the option alone, with no path repeated (FR-032)");
}

auto aRepeatableAccumulatesInOrder() -> void
{
  // Two lines of one repeatable keep both items, in the order the lines
  // stand, and the disclosure joins them with `;` (FR-035, FR-040, D-6).
  const Gathered counters =
      gatherFile({}, "counter=cycles\ncounter=instructions\n");
  check(counters.settled.has_value()
            && counters.settled->options.counterAddresses
                == std::vector<std::string> {"cycles", "instructions"},
        "two counter lines answer one counter per line, in line order "
        "(FR-035)");
  const Gathered points = gatherFile({}, "percentiles=50\npercentiles=90\n");
  check(points.settled.has_value()
            && sameValues(points.settled->options.percentilePoints,
                          {50.0, 90.0}),
        "two percentiles lines answer one point per line, in line order "
        "(FR-035)");
  if (counters.settled.has_value() && counters.settled->uses.size() == 1) {
    check(counters.settled->uses.front().value == "cycles;instructions",
          "the disclosed repeatable joins its items with ; (FR-040)");
  } else {
    report("the two counter lines answer one disclosed use (FR-039)",
           "one use",
           counters.settled.has_value()
               ? std::to_string(counters.settled->uses.size()) + " uses"
               : counters.fault);
  }
}

auto aFlagLineReachesTheFlagReader() -> void
{
  // A flag line written `name=value` reaches the flag reader of FR-025, and
  // a word outside the four answers that reader's own line behind the file
  // and the number (FR-025, FR-035). The row is the negating flag, so the
  // word `1` turns percentiles off as the environment word does.
  const Gathered gathered = gatherFile({}, "no-percentiles=1\n");
  check(gathered.settled.has_value()
            && gathered.settled->options.showPercentiles.has_value()
            && !*gathered.settled->options.showPercentiles,
        "the no-percentiles line turns percentiles off (FR-025, FR-035)");
  expectFileFault("no-percentiles=yes\n",
                  std::string(kOptionFile)
                      + ":1: expected 1, 0, true or false, got 'yes'",
                  "a flag line outside the four words answers the reader's "
                  "line behind the file and the number (FR-025, FR-035)");
}

auto aFileValueReachesTheReader() -> void
{
  // The value of a line goes through the reader the command line uses for
  // that kind, so the refusal is the command line's own line behind the file
  // and the number. The reader's words are never pinned here (FR-035, D-11).
  for (const NameCase& wanted :
       {NameCase {"min-time", "banana"}, NameCase {"repetitions", "-3"}})
  {
    const std::string word =
        "--" + std::string(wanted.name) + "=" + wanted.value;
    const std::string expected =
        std::string(kOptionFile) + ":1: " + faultOf({word.c_str()});
    const std::string actual =
        fileFaultOf(std::string(wanted.name) + "=" + wanted.value + "\n");
    if (actual != expected) {
      report("a refused file value carries the reader's line behind the file "
             "and the number (FR-035, FR-027)",
             expected,
             actual);
    }
  }
}

auto anUnreadableFileAnswersOneLine() -> void
{
  // The read stands at the entry point, which hands the gather the path and
  // the text (D-05), so this is the one fault of the six the gather cannot
  // answer. The line is the reader's own text behind the option (FR-034).
  const std::string path = "/speedgun-absent-directory/options-file.txt";
  const Run run = runEntry({"--options=" + path});
  const std::string wanted = "--options: the file '" + path
      + "' cannot be opened: " + std::strerror(ENOENT);
  if (run.err != wanted + "\n") {
    report("a file the system cannot read answers one line naming the path "
           "(FR-034, SC-003)",
           wanted,
           run.err);
  }
  check(run.status == 2,
        "the unreadable option file answers status 2 (FR-034, SC-013)");
  check(run.out.find("iterations=") == std::string::npos,
        "the unreadable option file runs no benchmark (SC-003)");
}

// ===========================================================================
// The precedence of the four sources of spec D-1: the command line, the
// option file, the environment and the master default (T034; FR-036 to
// FR-038, FR-041, SC-002). Every source reaches one call of the gather as
// the same data the two sections above hand it one at a time, which is the
// shape SC-002 states. The master default is not a source the test hands
// over: it is what stands in the settled answer when none of the three
// names the option, so its leg reads the field the option settles and asks
// that it hold nothing.
// ===========================================================================

// The settled answer of a gather, or null after one failure line naming the
// leg, so each precedence leg reads the answer it expects.
auto settledOf(const Gathered& gathered,
               const char* what) -> const sg::detail::ResolvedOptions*
{
  if (!gathered.settled.has_value()) {
    report((std::string(what) + " settles no fault (FR-037)").c_str(),
           "the gather answers no fault",
           gathered.fault);
    return nullptr;
  }
  return &*gathered.settled;
}

// The value `--min-time=` answers for this word through the scan, so a
// precedence expectation carries the reader's own unit and pins no number
// (FR-037, D-11).
auto minTimeOf(const std::string& word) -> std::optional<std::int64_t>
{
  const auto scan = scanOf({word.c_str()});
  return scan.has_value() ? scan->settled.options.minTimeNs
                          : std::optional<std::int64_t> {};
}

// The rule texts of the settled list, joined with `;`, so an order
// expectation reads as one line (E-07, E-12).
auto ruleTextsOf(const sg::detail::ResolvedOptions& settled) -> std::string
{
  std::string texts;
  for (const sg::CompareRule& rule : settled.rules) {
    if (!texts.empty()) {
      texts.push_back(';');
    }
    texts += rule.text;
  }
  return texts;
}

auto theThreeSourcesSettleInOrder() -> void
{
  // One option, three sources, three different values. The value that takes
  // effect is the command line's; with the words away it is the file's; with
  // both away it is the environment's; with neither it is the master default
  // (FR-037, spec D-1, SC-002). Each expectation is the value the scan
  // answers for the same word, so no unit is pinned here.
  const std::vector<sg::detail::EnvironmentRow> environment = {
      {"SPEEDGUN_MIN_TIME", "3"}};
  const std::string fileText = "min-time=2\n";

  const Gathered allRun =
      gatherSources({"--min-time=1"}, environment, fileText);
  if (const auto* all = settledOf(
          allRun,
          "the command line, the file and the environment over " "min-time"))
  {
    check(all->options.minTimeNs == minTimeOf("--min-time=1"),
          "the command line settles min-time over the file and the "
          "environment (FR-037, SC-002)");
    check(all->uses.empty(),
          "a value the command line settled answers no use (FR-039, FR-041)");
  }
  const Gathered fileRun = gatherSources({}, environment, fileText);
  if (const auto* file =
          settledOf(fileRun, "the file and the environment over min-time"))
  {
    check(file->options.minTimeNs == minTimeOf("--min-time=2"),
          "with the words away the option file settles min-time (FR-037, "
          "SC-002)");
    check(file->uses.size() == 1
              && file->uses.front().source
                     == sg::detail::OptionSource::OPTION_FILE
              && file->uses.front().value == "2",
          "the disclosure carries the file's text, not the environment's "
          "(FR-039, FR-040)");
  }
  const Gathered environmentRun = gatherSources({}, environment, std::nullopt);
  if (const auto* environmentOnly =
          settledOf(environmentRun, "the environment alone over min-time"))
  {
    check(environmentOnly->options.minTimeNs == minTimeOf("--min-time=3"),
          "with the words and the file away the environment settles min-time "
          "(FR-037, SC-002)");
    check(environmentOnly->uses.size() == 1
              && environmentOnly->uses.front().source
                  == sg::detail::OptionSource::ENVIRONMENT
              && environmentOnly->uses.front().value == "3",
          "the disclosure carries the environment's text (FR-039, FR-040)");
  }
  const Gathered noneRun = gatherSources({}, {}, std::nullopt);
  if (const auto* none = settledOf(noneRun, "no source at all")) {
    check(!none->options.minTimeNs.has_value(),
          "an option no source names stands at the master default (FR-037)");
    check(none->uses.empty(),
          "the master default answers no use (FR-039, FR-041)");
  }
}

auto aHigherSourceReplacesTheWholeList() -> void
{
  // FR-038: a repeatable set by a higher source replaces the whole list of a
  // lower one and appends nothing to it. The command line names two
  // counters, the file one and the environment two, and the run carries
  // exactly the command line's two.
  const std::vector<sg::detail::EnvironmentRow> environment = {
      {"SPEEDGUN_COUNTER", "faults;branches"}};
  const std::string fileText = "counter=pages\n";

  const Gathered allRun = gatherSources(
      {"--counter=cycles", "--counter=instructions"}, environment, fileText);
  // The counter walk reads the host, so it reads every address every source
  // named: the two of the words behind no origin, the two of the variable
  // behind its name, and the one of the file line behind its path and number,
  // although the settled list carries the words alone (FR-038, FR-087).
  if (const auto* kept =
          settledOf(allRun, "the three sources over one repeatable"))
  {
    const auto originOf =
        [](const sg::detail::ResolvedOptions& state, const std::string& address)
    {
      const auto found =
          std::ranges::find(state.counterAddresses,
                            address,
                            &sg::detail::CounterAddress::address);
      return found == state.counterAddresses.end() ? std::string("<absent>")
                                                   : found->origin;
    };
    check(kept->counterAddresses.size() == 5
              && originOf(*kept, "cycles").empty()
              && originOf(*kept, "instructions").empty()
              && originOf(*kept, "faults") == "SPEEDGUN_COUNTER"
              && originOf(*kept, "branches") == "SPEEDGUN_COUNTER"
              && originOf(*kept, "pages") == "options-file.txt:1",
          "every counter address keeps the origin of its source (FR-087)");
  }
  if (const auto* all =
          settledOf(allRun, "the three sources over one repeatable"))
  {
    check(all->options.counterAddresses
              == std::vector<std::string> {"cycles", "instructions"},
          "the command line's counters replace the file's and the "
          "environment's whole lists (FR-038, SC-002)");
    check(all->uses.empty(),
          "a repeatable the command line settled answers no use (FR-039)");
  }
  const Gathered fileRun = gatherSources({}, environment, fileText);
  if (const auto* file = settledOf(
          fileRun, "the file and the environment over one repeatable"))
  {
    check(file->options.counterAddresses == std::vector<std::string> {"pages"},
          "the file's one item replaces the environment's whole list and "
          "appends nothing (FR-038)");
    check(
        file->uses.size() == 1 && file->uses.front().value == "pages",
        "the disclosed repeatable carries the winner's items alone " "(FR-"
                                                                     "040)");
  }
  // The same rule over the thread counts, whose items the reader expands
  // (E-11, FR-044): the file's list cannot lengthen the words' list.
  const Gathered threadRun =
      gatherSources({"--threads=2,4"}, {}, "threads=8\n");
  if (const auto* threads = settledOf(
          threadRun, "the command line and the file over the thread list"))
  {
    check(
        threads->options.threadCounts == std::vector<int> {2, 4},
        "the command line's counts replace the file's list (FR-038, " "FR-"
                                                                      "044)");
  }
}

auto theRuleListReplacesAcrossSources() -> void
{
  // FR-038, FR-063: the comparison rule list is settled by the highest
  // source that names a rule, the way every other option is. A rule of a
  // lower source is read, so its own fault still reaches the operator, and
  // then drops: the list does not combine across the sources. The two
  // positions a file of its own names (the `--compare-rules` file and
  // the file a `compare-rules` line names) stay out of this seam, which opens
  // no file (FR-012, FR-037).
  const Gathered bothRun =
      gatherSources({"--compare-rule=time@median:lower:5%"},
                    {},
                    "compare-rule=time@max:higher:10%\n");
  if (const auto* both = settledOf(
          bothRun, "the command line and the file over one rule list"))
  {
    check(ruleTextsOf(*both) == "time@median:lower:5%",
          "the command line's rule settles the list alone (FR-038, FR-063)");
  }
  // Within one source the scalar rule of FR-035 stands: the last line of one
  // file settles the list, so a repeat of one source keeps its last word.
  const Gathered repeatRun = gatherSources(
      {},
      {},
      "compare-rule=time@max:higher:10%\n" "compare-rule=time@min:lower:1%\n");
  if (const auto* repeat = settledOf(repeatRun, "one file over two rule lines"))
  {
    check(
        ruleTextsOf(*repeat) == "time@max:higher:10%;time@min:lower:1%",
        "one file keeps the order of the rules it spells (FR-035, " "FR-063)");
  }
  // Two words of one source stand in the order that source spelled them, so
  // the replacement is a precedence between sources and never a limit on the
  // number of rules one source may name (FR-035, FR-063, recommendation 8).
  const Gathered pairRun = gatherSources({"--compare-rule=time@median:lower:5%",
                                          "--compare-rule=time@max:higher:10%"},
                                         {},
                                         std::nullopt);
  if (const auto* pair =
          settledOf(pairRun, "the command line over two rule words"))
  {
    check(ruleTextsOf(*pair)
              == "time@median:lower:5%;time@max:higher:10%",
          "two command-line rules stand in the order they were spelled "
          "(FR-035, FR-063)");
  }
  // A rules file that the option file names carries rules of its own, and a
  // command-line rule settles the list against them: the named file joins no
  // rule below a higher source (FR-038, FR-063).
  const std::filesystem::path rulesPath = capturePath(".namedrules");
  {
    std::ofstream stream(rulesPath);
    stream << "time@max:higher:10%\n";
  }
  const Gathered namedRun =
      gatherSources({"--compare-rule=time@median:lower:5%"},
                    {},
                    "compare-rules=" + rulesPath.string() + "\n",
                    &sg::detail::readCompareRulesFile);
  if (const auto* named =
          settledOf(namedRun, "a named rules file under a command-line rule"))
  {
    check(ruleTextsOf(*named) == "time@median:lower:5%",
          "the command line's rule settles the list against a named rules "
          "file (FR-038, FR-063)");
  }
  std::filesystem::remove(rulesPath);
  // The same file settles the list when nothing stands above it, so the drop
  // is a precedence and not a refusal to read the named file (FR-061, D-05).
  const std::filesystem::path ownPath = capturePath(".ownrules");
  {
    std::ofstream stream(ownPath);
    stream << "time@max:higher:10%\n";
  }
  const Gathered ownRun =
      gatherSources({},
                    {},
                    "compare-rules=" + ownPath.string() + "\n",
                    &sg::detail::readCompareRulesFile);
  if (const auto* own = settledOf(ownRun, "a named rules file alone")) {
    check(ruleTextsOf(*own) == "time@max:higher:10%",
          "the file's named rules settle the list when nothing stands above "
          "them (FR-061, FR-063)");
  }
  std::filesystem::remove(ownPath);
  // One source alone still answers a list, so the replacement is not what
  // makes the list stand (FR-061, FR-063).
  const Gathered wordsRun =
      gatherSources({"--compare-rule=time@median:lower:5%"}, {}, std::nullopt);
  if (const auto* words =
          settledOf(wordsRun, "the command line alone over one rule list"))
  {
    check(
        ruleTextsOf(*words) == "time@median:lower:5%",
        "the command line's rule reaches the answer of the gather " "(FR-063)");
  }
  const Gathered fileRun =
      gatherSources({}, {}, "compare-rule=time@max:higher:10%\n");
  if (const auto* file =
          settledOf(fileRun, "the option file alone over one rule list"))
  {
    check(ruleTextsOf(*file) == "time@max:higher:10%",
          "an option file rule reaches the answer of the gather (FR-061, "
          "FR-063)");
  }
}

auto aFileLineThatStartsWithADashNamesItsText() -> void
{
  // A line whose first non-space character is `-` is the command-line
  // spelling pasted into the file. The fault names the file, the number and
  // the text, and the table is never asked for a name of that shape; the
  // line before it holds a number, so the fault names 2 (FR-036, D-12).
  expectFileFault("filter=one\n--filter=two\n",
                  std::string(kOptionFile)
                      + ":2: expected an option name without --, got "
                        "'--filter=two'",
                  "a line that starts with a dash answers the file, the "
                  "number and the text (FR-036, SC-003)");
}

auto aRequiredArgumentLineWithoutAnEqualsNamesTheOption() -> void
{
  // A line of a required-argument row that carries no `=` answers the
  // command line's own needs-a-value line behind the file and the number, so
  // the reader's words are never pinned here (FR-036, FR-008, D-11).
  for (const char* const name : {"filter", "min-time"}) {
    const std::string word = "--" + std::string(name);
    expectFileFault(std::string(name) + "\n",
                    std::string(kOptionFile) + ":1: " + faultOf({word.c_str()}),
                    "a required-argument line with no value answers the "
                    "file, the number and the option (FR-036, SC-003)");
  }
  // The other half of the same rule: a flag row carries no `=` and is not a
  // fault, the bare name being the option standing as it is on the command
  // line (FR-029, FR-036).
  const Gathered gathered = gatherFile({}, "no-percentiles\n");
  check(gathered.settled.has_value()
            && gathered.settled->options.showPercentiles.has_value()
            && !*gathered.settled->options.showPercentiles,
        "a flag line written without = turns the flag on (FR-029, FR-036)");
}

auto aCommandLineOnlyRunDisclosesNothing() -> void
{
  // FR-041: with no option taken from a file or the environment and no seed
  // given, the gather answers no use at all, which is the run that prints no
  // disclosure line and writes no `run` object (FR-039, FR-040).
  const Gathered wordsRun = gatherSources({"--filter=one",
                                           "--counter=cycles",
                                           "--min-time=0.5",
                                           "--percentiles=90"},
                                          {},
                                          std::nullopt);
  if (const auto* words =
          settledOf(wordsRun, "a command line that names four options"))
  {
    check(words->uses.empty(),
          "a run with no file and no environment answers no use (FR-041)");
    check(!words->interleave.given,
          "and no seed stands given (FR-041, FR-051)");
  }
  // A file that stands present and names no option is the same run, and so
  // is an environment row that matches no flagged row (FR-026, FR-030,
  // FR-041).
  const Gathered emptyRun = gatherSources({}, {}, "# nothing here\n\n");
  if (const auto* empty =
          settledOf(emptyRun, "a file of one comment and one blank line"))
  {
    check(empty->uses.empty(),
          "a file that names no option answers no use (FR-030, FR-041)");
  }
  const Gathered unknownRun =
      gatherSources({}, {{"SPEEDGUN_NOT_AN_OPTION", "1"}}, std::nullopt);
  if (const auto* unknown = settledOf(
          unknownRun, "an environment row that matches no flagged row"))
  {
    check(unknown->uses.empty(),
          "an ignored variable answers no use (FR-026, FR-041)");
  }
}

auto theEnvironmentSwitchLeavesTheProcessNamesUnread() -> void
{
  // FR-028, D-04: `off` is the only word the option takes. The proof that the
  // switched run reads no name is a variable that would refuse the run: read,
  // it answers the gather's line at status 2; switched off, the run answers
  // what the same words answer with the variable absent (SC-005).
  const auto off = scanOf({"--env=off"});
  check(off.has_value() && off->settled.options.environmentOff,
        "--env=off settles the switch the entry point reads (FR-028)");
  expectFault({"--env=on"},
              "--env: expected 'off', got 'on'",
              "a word beside off answers the line naming it as typed (FR-028, "
              "SC-003)");

  const Run poisoned = runEntryWithEnv({}, {"SPEEDGUN_ITERATIONS=nonsense"});
  const Run switched =
      runEntryWithEnv({"--env=off"}, {"SPEEDGUN_ITERATIONS=nonsense"});
  const Run plain = runEntryWithEnv({}, {});

  check(poisoned.status == 2,
        "a variable whose value its reader refuses is a fault at the entry "
        "point (FR-027, SC-003)");
  check(poisoned.err.find("SPEEDGUN_ITERATIONS: ") != std::string::npos,
        "that fault names the variable that carries the refused text (FR-027)");
  check(switched.err.find("SPEEDGUN_ITERATIONS") == std::string::npos,
        "--env=off answers no fault line over the same variable (FR-028)");
  check(switched.status == plain.status && switched.out == plain.out,
        "--env=off answers what the same words answer with the variable absent "
        "(FR-028, SC-005)");
}

auto theThreadListFaultStandsAtTheScan() -> void
{
  // The scan owns no reading of the list: it hands the text to
  // `detail::readThreadList` and answers that reader's line, so the expectation
  // is the reader's own answer and no text is pinned here (FR-044, FR-046,
  // D-11).
  for (const char* const text :
       {"nonsense", "12x", "0", "a:2", "0:2", "4:2", "2:1", "1:1,3"})
  {
    const std::string word = std::string("--threads=") + text;
    const auto counts = sg::detail::readThreadList(text);
    const auto answer = scanOf({word.c_str()});
    const std::string what = "the list '" + std::string(text)
                           + "' reaches the reader through the scan "
                             "(FR-044, FR-046)";
    if (counts.has_value() != answer.has_value()) {
      report(what.c_str(),
             counts.has_value() ? "the scan accepts the list"
                                : "the scan refuses the list",
             answer.has_value() ? "the scan accepted the list"
                                : "the scan refused the list");
      continue;
    }
    if (!counts.has_value()) {
      expectFault({word.c_str()}, counts.error(), what.c_str());
      continue;
    }
    check(answer->settled.options.threadCounts == *counts,
          (what + ": the counts the reader answers").c_str());
  }
}

auto aWordOfDashesAloneNamesNoOption() -> void
{
  // A word with no letters carries no row name, so no row of the table is a
  // candidate for it and the unknown line stands (FR-007, FR-009).
  expectFault(
      {"---"},
      "unknown option '---'; --help lists the options",
      "a word of dashes alone answers the unknown line (FR-007, " "FR-009)");
}

auto aLowerSourceYieldsToTheCommandLine() -> void
{
  // FR-037: the command line stands above both lower sources, so a row it
  // settled is neither read again nor disclosed by them. The empty disclosure
  // list is the proof: had a lower source settled the row, it would name a use
  // of its own (FR-040).
  for (const NameCase& wanted : kFlagged) {
    const std::string variable = variableFor(wanted.name);
    // A row that takes no value stands bare on the command line the way the
    // same row stands bare in the file (FR-029, FR-036).
    const sg::detail::OptionRow* const row = liveRow(wanted.name);
    const std::string word =
        row != nullptr && row->argument == sg::detail::OptionArgument::NONE
        ? std::string("--") + wanted.name
        : std::string("--") + wanted.name + '=' + wanted.value;
    const Gathered fromEnvironment =
        gather({word.c_str()}, {{variable, wanted.value}});
    if (const auto* settled = settledOf(
            fromEnvironment,
            ("the environment beside --" + std::string(wanted.name)).c_str()))
    {
      const std::string label =
          "the variable " + variable + " yields to the words (FR-037)";
      check(settled->uses.empty(), label.c_str());
    }
    const Gathered fromFile = gatherFile(
        {word.c_str()}, std::string(wanted.name) + '=' + wanted.value + '\n');
    if (const auto* settled = settledOf(
            fromFile,
            ("the option file beside --" + std::string(wanted.name)).c_str()))
    {
      const std::string label = std::string(wanted.name)
          + " of the file yields to the words (FR-037)";
      check(settled->uses.empty(), label.c_str());
    }
  }
}

auto aRefusedValueOfEveryKindNamesItsSource() -> void
{
  // The rule of FR-027 stands for every reader a source can reach, the two the
  // first pass names and the rest alike, and the file carries the rule beside
  // the rule list (FR-027, FR-035, SC-003). The expectation is the line the
  // same text answers on the command line, so no reader's words are pinned here
  // (D-11).
  for (const NameCase& wanted : {NameCase {"iterations", "nonsense"},
                                 NameCase {"warmup-time", "banana"},
                                 NameCase {"percentiles", "banana"},
                                 NameCase {"threads", "nonsense"},
                                 NameCase {"interleave", "banana"}})
  {
    const std::string variable = variableFor(wanted.name);
    const std::string word =
        std::string("--") + wanted.name + '=' + wanted.value;
    const Gathered gathered = gather({}, {{variable, wanted.value}});
    const std::string expected = variable + ": " + faultOf({word.c_str()});
    if (gathered.fault != expected) {
      const std::string what = "a refused " + variable
                             + " value names the variable and carries the "
                               "reader's line (FR-027, SC-003)";
      report(what.c_str(), expected, gathered.fault);
    }
  }
  // The rule list of a file the parser refuses names the file and the line
  // before the reader's line, which is the line the words answer with the
  // option in front of it: the row settles in one function, so its text is
  // one text in every source (FR-035, FR-049, FR-061).
  expectFileFault("compare-rule=not-a-rule\n",
                  std::string(kOptionFile) + ":1: "
                      + faultOf({"--compare-rule=not-a-rule"}),
                  "a rule of the file its parser refuses names the file, the "
                  "line and the parser's line (FR-035, FR-061, SC-003)");
}

auto theLastLineOfASourceMayCarryNoBreak() -> void
{
  // FR-030, D-12: a last line may end at its text, and the rules file a
  // `compare-rules` line names follows the same rule (FR-061).
  const Gathered noBreak = gatherFile({}, "filter=last");
  if (const auto* settled =
          settledOf(noBreak, "a file whose last line carries no break"))
  {
    check(settled->options.filter == "last",
          "the last line of the file settles its option without a break "
          "(FR-030)");
    check(settled->uses.size() == 1 && settled->uses.front().value == "last",
          "that line discloses the text it carries (FR-040)");
  }

  const std::filesystem::path path = capturePath(".rules");
  {
    std::ofstream stream(path);
    stream << "time@median:lower:5%\ntime@max:higher:2%";
  }
  const auto rules = sg::detail::readCompareRulesFile(path.string());
  check(rules.has_value() && rules->size() == 2,
        "the last line of a rules file reaches the reader without a break "
        "(FR-061)");
  std::filesystem::remove(path);
}

auto aBareSeedLineStandsForTheSeedZero() -> void
{
  // E-10: a line that names a row whose value may stand absent settles the
  // value that bare option settles on the command line, which for the seed is
  // zero (FR-029, FR-036, FR-051).
  const Gathered bare = gatherFile({}, "interleave\n");
  if (const auto* settled =
          settledOf(bare, "a file line naming the seed with no value"))
  {
    check(settled->interleave.given && settled->interleave.seed == 0,
          "a bare seed line turns the option on with the seed zero (FR-029, "
          "E-10)");
    check(settled->uses.size() == 1 && settled->uses.front().value == "0",
          "that line discloses the value the bare name settled (FR-040)");
  }
}

// The nine rows an option file can name and the gather settled through no
// arm: each carries one good word and one word its reader refuses, so the
// settle function of the row is read from both sides (FR-033, FR-035, FR-040,
// FR-050, FR-052).
struct RowSettleCase
{
  const char* good;
  const char* bad;
  const char* word;
};

constexpr RowSettleCase kRowSettle[] = {
    {"list=1", "list=2", "1"},
    {"dry-run=true", "dry-run=x", "true"},
    {"catalog=0", "catalog=q", "0"},
    {"topology=true", "topology=0.5", "true"},
    {"compare-strict=1", "compare-strict=nope", "1"},
    {"json=report.json", "json=", "report.json"},
    {"json-raw=chunks", "json-raw=bogus", "chunks"},
    {"json-raw=none", "json-raw=", "none"},
    {"metric-method=passes", "metric-method=bogus", "passes"},
    {"compare=base.json,run.json", "compare=nocomma", "base.json,run.json"},
    {"compare=left.json,right.json", "compare=left,", "left.json,right.json"},
};

auto everyRowOfTheTableSettlesItsRow() -> void
{
  // The option file recognizes every row of the table and the settle seam
  // carries an arm for every one of them, so no line of a file is read and
  // dropped unseen (FR-033, FR-040). The two names that would re-root the
  // file's own sources and the two that print are refused where they stand,
  // which the case of the four refused names reads (FR-031).
  for (const RowSettleCase& row : kRowSettle) {
    const Gathered good = gatherFile({}, std::string(row.good) + "\n");
    if (!good.settled.has_value()) {
      report("the good word of one table row settles its row (FR-033)",
             "no fault",
             good.fault);
      continue;
    }
    check(good.settled->uses.size() == 1
              && good.settled->uses.front().value == row.word,
          "the settled row discloses the word of its own line (FR-039, "
          "FR-040)");
    const std::string fault = fileFaultOf(std::string(row.bad) + "\n");
    check(fault.starts_with(std::string(kOptionFile) + ":1: ")
              && fault.find('\n') == std::string::npos,
          "the refused word of the same row answers one line naming the file "
          "and the number (FR-031, FR-035, SC-003)");
  }
}

// The nine rows the file can name, each with a word the command line gives
// above it. The two values differ, so the check can tell which one settled
// (FR-037).
struct RowYieldCase
{
  const char* word;
  const char* line;
};

constexpr RowYieldCase kRowYield[] = {
    {"--list", "list=0"},
    {"--dry-run", "dry-run=false"},
    {"--catalog", "catalog=0"},
    {"--topology", "topology=false"},
    {"--compare-strict", "compare-strict=0"},
    {"--json=top.json", "json=below.json"},
    {"--json-raw=none", "json-raw=chunks"},
    {"--metric-method=auto", "metric-method=passes"},
    {"--compare=top.json,below.json", "compare=below.json,below.json"},
};

auto aGoodLineOfALowerSourceYieldsAndDrops() -> void
{
  // FR-024, FR-037, FR-040: a value the reader accepts, handed while a higher
  // source holds the row, is read and then dropped: the higher value stands
  // and the drop discloses nothing. The nine rows the file can name each
  // carry the pair, so no row of the settle function keeps its drop line
  // unseen (FR-035).
  for (const RowYieldCase& row : kRowYield) {
    const Gathered gathered =
        gatherFile({row.word}, std::string(row.line) + '\n');
    if (!gathered.settled.has_value()) {
      report("a good file line under a command-line word settles (FR-037)",
             "no fault",
             gathered.fault);
      continue;
    }
    const std::string label = std::string(row.line) + " yields to " + row.word
        + " and discloses " + "nothing (FR-037, FR-040)";
    check(gathered.settled->uses.empty(), label.c_str());
  }

  // The four rows whose settled value the answer carries by name: the word
  // of the higher source is the value the run reads.
  const Gathered path = gatherFile({"--json=top.json"}, "json=below.json\n");
  if (path.settled.has_value()) {
    check(path.settled->options.jsonPath == "top.json",
          "the path of the words stands over the path of the file (FR-037)");
  }
  const Gathered raw = gatherFile({"--json-raw=none"}, "json-raw=chunks\n");
  if (raw.settled.has_value()) {
    check(raw.settled->options.rawContent == sg::RawContent::NONE,
          "the content of the words stands over the content of the file "
          "(FR-037, FR-050)");
  }
  const Gathered method =
      gatherFile({"--metric-method=auto"}, "metric-method=passes\n");
  if (method.settled.has_value()) {
    check(method.settled->options.metricMethod
              == sg::MetricMethod::AUTO,
          "the method of the words stands over the method of the file "
          "(FR-037)");
  }
  const Gathered pair = gatherFile({"--compare=top.json,below.json"},
                                   "compare=below.json,under.json\n");
  if (pair.settled.has_value()) {
    check(pair.settled->options.compareBaseline == "top.json"
              && pair.settled->options.compareContender == "below.json",
          "the pair of the words stands over the pair of the file (FR-037, "
          "FR-052)");
  }
}

auto aShadowedBadValueStillNamesItsSource() -> void
{
  // FR-024, FR-037: a value a higher source shadows is still handed to its
  // reader, so a word an operator spelled wrongly answers its own line behind
  // the source that carries it, and the higher
  // source's value stands. Each shape runs in a child at the entry point and
  // exits 2 (FR-027, FR-035, FR-037, SC-003).
  const std::string refusal = faultOf({"--min-time=abc"});
  oneFaultLine(
      runEntryWithEnv({"--min-time=1"}, {"SPEEDGUN_MIN_TIME=abc"}),
      "a bad variable under a command-line value (FR-027, FR-037, " "SC-003)",
      {"SPEEDGUN_MIN_TIME: ", refusal});

  const std::filesystem::path goodPath = capturePath(".goodshadow");
  const std::filesystem::path badPath = capturePath(".badshadow");
  {
    std::ofstream stream(goodPath);
    stream << "min-time=1\n";
  }
  {
    std::ofstream stream(badPath);
    stream << "min-time=abc\n";
  }
  oneFaultLine(
      runEntryWithEnv({"--min-time=1", "--options=" + badPath.string()}, {}),
      "a bad file line under a command-line value (FR-035, FR-037, SC-003)",
      {badPath.string() + ":1: ", refusal});
  oneFaultLine(
      runEntryWithEnv({"--options=" + goodPath.string()},
                      {"SPEEDGUN_MIN_TIME=abc"}),
      "a bad variable under a file value (FR-027, FR-035, FR-037, SC-003)",
      {"SPEEDGUN_MIN_TIME: ", refusal});
  std::filesystem::remove(goodPath);
  std::filesystem::remove(badPath);

  const Gathered counts =
      gatherSources({"--threads=2"},
                    {sg::detail::EnvironmentRow {"SPEEDGUN_THREADS", "x"}},
                    std::nullopt);
  check(counts.fault.starts_with("SPEEDGUN_THREADS: --threads:"),
        "a refused count list behind settled counts still answers its line "
        "(FR-044, FR-046)");

  const Gathered ruleRun = gatherSources(
      {"--compare-rule=time@median:lower:5%"}, {}, "compare-rule=nonsense\n");
  check(ruleRun.fault.starts_with("options-file.txt:1: ")
            && ruleRun.fault.find("rule") != std::string::npos,
        "a refused rule of a lower source answers its own line (FR-035, "
        "FR-063)");
}

auto aHeldValueOfALowerSourceNamesItsSource() -> void
{
  // Three rows are kept as text by the settle and read only for the winner:
  // the batch filter, the process preset and the user metrics. A bad word of
  // a lower source therefore settles nothing and exits 0, and the line of the
  // one that does answer names the option it comes from, with the source
  // that holds it standing behind that name. Each of the three is read where it
  // settles now, so each answers one line behind its own source at status 2,
  // whether or not a higher source settled the row (FR-027, FR-033, FR-038,
  // FR-046, FR-051).
  const Run barePreset = runEntryWithEnv({}, {"SPEEDGUN_PLACEMENT=bogus"});
  oneFaultLine(barePreset,
               "the refused preset of SPEEDGUN_PLACEMENT (FR-027, FR-046)",
               {"SPEEDGUN_PLACEMENT: ", "--placement"});
  const Run heldPreset =
      runEntryWithEnv({"--placement=compact"}, {"SPEEDGUN_PLACEMENT=bogus"});
  oneFaultLine(heldPreset,
               "the refused preset SPEEDGUN_PLACEMENT holds under a preset of "
               "the words (FR-038, FR-046)",
               {"SPEEDGUN_PLACEMENT: ", "--placement"});

  const Run bareFilter = runEntryWithEnv({}, {"SPEEDGUN_FILTER=("});
  oneFaultLine(bareFilter,
               "the refused pattern of SPEEDGUN_FILTER (FR-027, FR-051)",
               {"SPEEDGUN_FILTER: ", "--filter"});
  const Run heldFilter =
      runEntryWithEnv({"--filter=^bm"}, {"SPEEDGUN_FILTER=("});
  oneFaultLine(heldFilter,
               "the refused pattern SPEEDGUN_FILTER holds under a pattern of "
               "the words (FR-038, FR-051)",
               {"SPEEDGUN_FILTER: ", "--filter"});

  // The metric row is read at the entry point, where a benchmark stands to
  // be run: its two cases of the lower source, of the environment and of a
  // file line, are in the command line test beside the metric cases of the
  // words (FR-027, FR-028).

  // The same two words of a file line name the file and the number of the
  // line that holds them, as every refusal of a file line does (FR-033,
  // FR-035, FR-065).
  for (const NameCase& wanted :
       {NameCase {"placement", "bogus"}, NameCase {"filter", "("}})
  {
    const std::filesystem::path path = capturePath(".held");
    {
      std::ofstream stream(path);
      stream << wanted.name << '=' << wanted.value << "\n";
    }
    const Run held = runEntry({"--options=" + path.string()});
    const std::string what =
        std::string("a file line whose ") + wanted.name
        + " the settle keeps as text names its file and its number "
          "(FR-033, FR-035)";
    oneFaultLine(held,
                 what.c_str(),
                 {path.string() + ":1: ", std::string("--") + wanted.name});
    std::filesystem::remove(path);
  }

  // The path of a rules file a lower source names is opened before its rules
  // drop, so its own refusal stands (FR-038, FR-061, FR-063).
  const std::filesystem::path file = capturePath(".heldrules");
  {
    std::ofstream stream(file);
    stream << "compare-rules=/speedgun-absent-directory/more.rules\n";
  }
  const Run heldRules = runEntry(
      {"--compare-rule=time@median:lower:5%", "--options=" + file.string()});
  oneFaultLine(heldRules,
               "the rules path a lower source names is opened before it drops "
               "(FR-038, FR-061)",
               {"/speedgun-absent-directory/more.rules"});
  std::filesystem::remove(file);
}

auto anEmptyFilterWordSettlesTheRowItNames() -> void
{
  // `--filter=` names the row with an empty pattern, and an empty pattern is
  // the pattern that selects every benchmark, so the row is settled and no
  // lower source replaces it. The mark of the row cannot be the emptiness of
  // the text, so the settle keeps a mark of its own (FR-024, FR-035, FR-037,
  // FR-050).
  const Gathered fromWords =
      gatherSources({"--filter="}, {{"SPEEDGUN_FILTER", "^bm"}}, std::nullopt);
  if (!fromWords.settled.has_value()) {
    fail("the gather settles an empty filter word of the words");
  } else {
    check(fromWords.settled->options.filter.empty(),
          "the empty filter word of the words settles the row the variable "
          "would name (FR-037, FR-050)");
  }

  const Gathered fromFile =
      gatherSources({}, {{"SPEEDGUN_FILTER", "^bm"}}, "filter=\n");
  if (!fromFile.settled.has_value()) {
    fail("the gather settles an empty filter line of a file");
  } else {
    check(fromFile.settled->options.filter.empty(),
          "the empty filter line of one file settles the row the variable "
          "would name (FR-035, FR-037)");
  }

  // The expression of the words is read where their own row settles, in the
  // one seam a file line and a variable use, so the refusal names the option
  // and not a source path (FR-024, FR-033, FR-051).
  const std::string badWords = faultOf({"--filter=("});
  check(badWords.starts_with("--filter: invalid regular expression '(': "),
        "the refusal of the expression of the words names the option it "
        "holds (FR-051, FR-033)");
}

auto aLowerPercentileListIsCountedAlone() -> void
{
  // FR-022, FR-038: the cap of sixteen points counts the points of the source
  // that settles the row. A list that drops is never joined to the points a
  // higher source holds, so two lists that each stand on their own settle to
  // the higher source, and only a list that passes the cap within its own
  // source answers a refusal.
  const Gathered held =
      gather({"--percentiles=1,2,3,4,5,6,7,8,9,10"},
             {{"SPEEDGUN_PERCENTILES", "11,12,13,14,15,16,17"}});
  if (!held.settled.has_value()) {
    fail("a percentile list of the environment under a list of the words "
         "settles the row (FR-022, FR-038)");
  } else {
    const std::vector<double> expected {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    check(held.settled->options.percentilePoints == expected,
          "the points of the words settle the row (FR-037)");
    check(held.settled->options.showPercentiles.value_or(false),
          "the row that settled turns the percentiles on (FR-020)");
  }

  const Gathered over =
      gather({"--percentiles=1,2,3,4,5,6,7,8,9,10"},
             {{"SPEEDGUN_PERCENTILES",
               "11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27"}});
  if (over.settled.has_value()) {
    fail(
        "a percentile list of seventeen points is read and refused " "(FR-"
                                                                     "022)");
  } else {
    check(over.fault.starts_with("SPEEDGUN_PERCENTILES: --percentiles: "),
          "the refusal of the list that drops names the variable that holds "
          "it (FR-033, FR-038)");
  }
}

// One value row of the table given one bad text through every source that may
// name it. The three refusals must be one line each, and the line of a lower
// source must be the line of the words with that source's own prefix in front
// of it: the scan and the gather read a row through one reader, so no row can
// drift into two fault texts (FR-024, FR-027, FR-031, FR-046, FR-054, SC-003).
struct SettleCase
{
  const char* name;  // the long name of the row, no dashes
  const char* bad;  // the text no reader of that kind takes
  const char* variable;  // SPEEDGUN_* name, empty when the row has none
};

// The rows that answer a fault when the text of their value is refused. The
// two rows that name a catalog object take any text and answer where the
// object is looked up, and the rules row answers where its file is read, so
// none of the three carries a bad value here (FR-028, FR-061, FR-064).
constexpr SettleCase kSettleCases[] = {
    {"filter", "[", "SPEEDGUN_FILTER"},
    {"repetitions", "x", "SPEEDGUN_REPETITIONS"},
    {"min-time", "x", "SPEEDGUN_MIN_TIME"},
    {"iterations", "x", "SPEEDGUN_ITERATIONS"},
    {"warmup-time", "x", "SPEEDGUN_WARMUP_TIME"},
    {"percentiles", "150", "SPEEDGUN_PERCENTILES"},
    {"placement", "nosuchpreset", "SPEEDGUN_PLACEMENT"},
    {"threads", "x", "SPEEDGUN_THREADS"},
    {"interleave", "x", "SPEEDGUN_INTERLEAVE"},
    {"json", "", ""},
    {"json-raw", "nosuch", ""},
    {"compare", "nocomma", ""},
    {"compare-rule", "garbage", ""},
    {"metric-method", "x", ""},
};

auto theBadValueOfEverySourceSharesTheWordsLine() -> void
{
  for (const SettleCase& row : kSettleCases) {
    const std::string name {row.name};
    const std::string word = "--" + name + "=" + row.bad;
    const std::string words = faultOf({word.c_str()});
    if (words == "the scan answered no fault") {
      const std::string label =
          "the bad value of " + word + " is refused by the scan (FR-046)";
      report(label.c_str(), "one fault line", words);
      continue;
    }
    check(words.find('\n') == std::string::npos,
          "the fault of the words is one line (SC-003)");

    const std::string wanted = std::string(kOptionFile) + ":1: " + words;
    const std::string file = fileFaultOf(name + "=" + row.bad + "\n");
    if (file != wanted) {
      const std::string label = "the file line of " + name
          + " is the line of the words behind the path and its number "
            "(FR-027, FR-031)";
      report(label.c_str(), wanted, file);
    }

    if (row.variable[0] == '\0') {
      continue;
    }
    const Gathered under = gather({}, {{row.variable, row.bad}});
    if (under.settled.has_value()) {
      const std::string label =
          "the variable of " + name + " is refused (FR-027, FR-046)";
      report(label.c_str(), "one fault line", "the gather answered no fault");
      continue;
    }
    const std::string wantedEnv = std::string(row.variable) + ": " + words;
    if (under.fault != wantedEnv) {
      const std::string label = "the variable line of " + name
          + " is the line of the words behind the variable name "
            "(FR-027, FR-033)";
      report(label.c_str(), wantedEnv, under.fault);
    }
  }
}

// The six rows that carry no value on the command line. The bare word is the
// flag standing, and so is the bare line of a file or a variable; the text a
// lower source holds is read as a flag, so the refusal of a text that is not
// one stands behind that source's own prefix and nowhere else (FR-025,
// FR-029, FR-033).
constexpr const char* kFlagRows[] = {
    "list",
    "dry-run",
    "catalog",
    "topology",
    "compare-strict",
    "no-percentiles",
};

auto theBareFlagOfEverySourceSettlesItsRow() -> void
{
  for (const char* const name : kFlagRows) {
    const std::string word = std::string("--") + name;
    const std::string label = "the bare word " + word + " settles its row";
    check(faultOf({word.c_str()}) == "the scan answered no fault",
          (label + " (FR-025)").c_str());

    const std::string wanted =
        std::string(kOptionFile) + ":1: expected 1, 0, true or false, got 'x'";
    const std::string text = fileFaultOf(std::string(name) + "=x\n");
    if (text != wanted) {
      const std::string bad =
          "the flag text of " + word + " answers its own line (FR-025)";
      report(bad.c_str(), wanted, text);
    }

    const std::string bare = fileFaultOf(std::string(name) + "\n");
    if (bare != "the gather answered no fault") {
      const std::string missing =
          "the bare line of " + word + " settles its row (FR-029)";
      report(missing.c_str(), "the gather answered no fault", bare);
    }
  }

  const Gathered under = gather({}, {{"SPEEDGUN_NO_PERCENTILES", "x"}});
  const std::string wanted =
      "SPEEDGUN_NO_PERCENTILES: expected 1, 0, true or false, got 'x'";
  if (under.settled.has_value()) {
    report("the flag text of a variable answers its own line (FR-025)",
           wanted,
           "the gather answered no fault");
  } else if (under.fault != wanted) {
    report("the flag text of a variable answers its own line (FR-025)",
           wanted,
           under.fault);
  }
}

// The four rows the words settle and no lower source may name: the two that
// would re-root the file's own sources and the two that print. Their lines
// belong to the scan alone, and a print row ends the walk where it stands, so
// no word behind it is read (FR-015 to FR-017, FR-028, FR-031, FR-032,
// spec D-9).
auto theFourRefusedRowsAnswerTheirOwnLines() -> void
{
  expectFault({"--options=a", "--options=b"},
              "--options cannot stand twice",
              "the second option file answers the twice line (FR-032)");
  expectFault({"--env=on"},
              "--env: expected 'off', got 'on'",
              "a word other than off answers the env line (FR-028)");
  check(faultOf({"--help"}) == "the scan answered no fault",
        "the help row answers no fault (FR-015)");
  check(faultOf({"--version"}) == "the scan answered no fault",
        "the version row answers no fault (FR-016)");
  check(faultOf({"--help", "--zzzz"}) == "the scan answered no fault",
        "the help row stops before a word behind it (FR-017)");
  expectFault({"--zzzz", "--help"},
              "unknown option '--zzzz'; --help lists the options",
              "an unknown word before the help row still faults (FR-017)");

  for (const char* const name : {"options", "env", "help", "version"}) {
    const std::string wanted = std::string(kOptionFile) + ":1: the option '"
        + name + "' cannot stand in an option file";
    const std::string label =
        std::string("a refused row of a file answers the refusal line for ")
        + name + " (FR-031)";
    expectFileFault(std::string(name) + "=a\n", wanted, label.c_str());
  }
}

auto theEnvironmentWalkReadsTheTableItIsGiven() -> void
{
  // FR-012, FR-026: the entry point owns the read of the process table and
  // hands it along, so the walk reads the table it is given and nothing
  // else. A null table holds no entry and answers no row, the way an empty
  // one does, and a table of three entries answers the one row the
  // `SPEEDGUN_` prefix covers.
  check(sg::detail::environmentRows(nullptr).empty(),
        "a null table answers no row (FR-012)");
  char* const empty[] = {nullptr};
  check(sg::detail::environmentRows(empty).empty(),
        "an empty table answers no row (FR-012)");
  char first[] = "SPEEDGUN_FILTER=one";
  char second[] = "OTHER_NAME=two";
  char third[] = "NO_EQUALS_SIGN";
  char* const table[] = {first, second, third, nullptr};
  const std::vector<sg::detail::EnvironmentRow> rows =
      sg::detail::environmentRows(table);
  check(rows.size() == 1 && rows.front().name == "SPEEDGUN_FILTER"
            && rows.front().value == "one",
        "the walk answers the one entry the prefix covers (FR-021, FR-026)");
}

auto aBadLetterOfAShortGroupAfterALongOption() -> void
{
  // FR-007, FR-005: the letter a short group is refused on belongs to the
  // group, and the walk stands inside a group it stopped within, so the line
  // names that letter as it does when the group stands alone. The word
  // behind the walk belongs to the long option and names no letter, so the
  // base line would be the wrong answer here (FR-009).
  expectFault({"--filter=a", "-xy"},
              "unknown option '-x'; --help lists the options",
              "a bad letter of a short group behind a long option names that "
              "letter (FR-007, SC-003)");
  expectFault({"-xy"},
              "unknown option '-x'; --help lists the options",
              "the same letter of a group alone names the same line "
              "(FR-007, FR-005)");
}

// One row spelled twice by one source, each line carrying its own word.
struct RepeatCase
{
  const char* name;
  const char* first;
  const char* later;
  bool joins;
};

// The rows whose arm settles a value, each with two words the reader of its
// kind accepts.
constexpr RepeatCase kRepeats[] = {
    {"filter", "one", "two", false},
    {"repetitions", "3", "5", false},
    {"min-time", "0.5", "1.5", false},
    {"iterations", "7", "9", false},
    {"warmup-time", "2", "3", false},
    {"metric", "ipc", "cpi", true},
    {"percentiles", "90", "99", true},
    {"no-percentiles", "true", "false", false},
    {"placement", "core", "l3", false},
    {"threads", "2", "4", true},
    {"interleave", "7", "9", false},
    {"list", "0", "1", false},
    {"dry-run", "false", "true", false},
    {"catalog", "1", "0", false},
    {"topology", "true", "false", false},
    {"compare-strict", "0", "1", false},
    {"json", "one.json", "two.json", false},
    {"json-raw", "chunks", "none", false},
    {"metric-method", "passes", "auto", false},
    {"compare", "one.json,two.json", "three.json,four.json", false},
};

auto oneWordTwiceKeepsTheLastWordOfOneSource() -> void
{
  // The edge case "An option file that holds the same option twice": the rule
  // of the command line applies inside one source, so a scalar keeps the last
  // word and a repeatable accumulates in line order (FR-035, FR-038). The
  // disclosure names such a row once and carries the value that took effect,
  // a repeatable joining its items with `;` (FR-040, E-05).
  for (const RepeatCase& wanted : kRepeats) {
    const std::string label = std::string("the row ") + wanted.name
        + " spelled twice by one file " + "(FR-035, FR-040)";
    const Gathered twice =
        gatherFile({},
                   std::string(wanted.name) + "=" + wanted.first + "\n"
                       + wanted.name + "=" + wanted.later + "\n");
    if (!twice.settled.has_value()) {
      report(label.c_str(), "the gather settles the pair", twice.fault);
      continue;
    }
    const std::string joined(wanted.first);
    const std::string expected =
        wanted.joins ? joined + ";" + wanted.later : std::string(wanted.later);
    std::size_t named = 0;
    std::string text;
    for (const sg::detail::OptionUse& use : twice.settled->uses) {
      if (use.row != nullptr && std::string_view(use.row->name) == wanted.name)
      {
        ++named;
        text = use.value;
      }
    }
    if (named != 1) {
      report(label.c_str(),
             "one use naming the row",
             std::to_string(named) + " uses");
      continue;
    }
    check(text == expected, (label + ": the value that took effect").c_str());
  }

  const Gathered fixed = gatherFile({}, "iterations=7\niterations=9\n");
  if (fixed.settled.has_value()) {
    check(*fixed.settled->options.fixedIterations == 9,
          "the later line of one file fixes the iteration count (FR-035)");
  } else {
    fail("the gather settles a file that fixes the count twice");
  }

  const Gathered listed = gatherFile({}, "threads=2\nthreads=4\n");
  if (listed.settled.has_value()) {
    const std::vector<int> expected {2, 4};
    check(listed.settled->options.threadCounts == expected,
          "both counts of one file stand in line order (FR-035)");
  } else {
    fail("the gather settles a file that names two counts");
  }

  const Gathered word = gather({"--iterations=7", "--iterations=9"}, {});
  if (word.settled.has_value()) {
    check(*word.settled->options.fixedIterations == 9,
          "the later word of one command line fixes the count (FR-038)");
  } else {
    fail("the gather settles a command line that fixes the count twice");
  }

  const Gathered paths =
      gatherFile({}, "json=one.json\njson=two.json\njson-raw=chunks\n");
  if (paths.settled.has_value()) {
    check(paths.settled->options.jsonPath == "two.json"
              && paths.settled->options.rawContent == sg::RawContent::CHUNKS,
          "the later path of one file stands and the raw word of the next "
          "line settles its own row (FR-035, FR-050)");
  } else {
    fail("the gather settles a file that names a path twice and a raw word");
  }

  const Gathered modes = gatherFile({},
                                    "list=0\nlist=1\ndry-run=false\n"
                                    "metric-method=passes\nmetric-method=auto\n");
  if (modes.settled.has_value()) {
    check(modes.settled->options.listMode && !modes.settled->options.dryRun
              && modes.settled->options.metricMethod == sg::MetricMethod::AUTO,
          "the later word of one file settles each row of the group (FR-035)");
  } else {
    fail("the gather settles a file that spells three rows twice");
  }
}

// The two percentile rows write one flag, so one source settles that flag
// once and the last word of the source wins the flag and the points, the way
// a scalar keeps its last word (FR-025, FR-035, FR-038). The refusal of the
// pair stands across sources alone, where the environment names both halves
// (FR-023).
auto thePercentilePairKeepsTheLastWordOfOneSource() -> void
{
  struct Order
  {
    const char* first;
    const char* later;
    bool percentiles;
  };
  const Order orders[] {
      {"--percentiles=50", "--no-percentiles", false},
      {"--no-percentiles", "--percentiles=50", true},
  };
  for (const Order& order : orders) {
    const std::string label = std::string("the words ") + order.first + " then "
        + order.later + " leave percentiles "
        + (order.percentiles ? "on" : "off") + " (FR-025, FR-035)";
    const auto scan = scanOf({order.first, order.later});
    if (!scan.has_value()) {
      report(label.c_str(), "the scan accepts the pair", scan.error());
      continue;
    }
    const sg::detail::RunOptions& words = scan->settled.options;
    if (!words.showPercentiles.has_value()) {
      report(label.c_str(), "a settled percentile flag", "no flag");
      continue;
    }
    check(*words.showPercentiles == order.percentiles, label.c_str());
    if (order.percentiles) {
      check(sameValues(words.percentilePoints, {50.0}),
            (label + ": the points of the word that settled").c_str());
    }
    const Run run =
        runEntryWithEnv({order.first, order.later, "--dry-run"}, {});
    const std::string statusLabel =
        std::string("the pair ") + order.first + " " + order.later
        + " keeps status 0 (FR-025, FR-035)";
    check(run.status == 0, statusLabel.c_str());
  }

  struct FileOrder
  {
    const char* text;
    bool percentiles;
  };
  const FileOrder fileOrders[] {
      {"percentiles=50\nno-percentiles\n", false},
      {"no-percentiles\npercentiles=50\n", true},
  };
  for (const FileOrder& order : fileOrders) {
    const std::string label =
        std::string("the two lines of one file leave percentiles ")
        + (order.percentiles ? "on" : "off") + " (FR-025, FR-035)";
    const Gathered gathered = gatherFile({}, order.text);
    if (!gathered.settled.has_value()) {
      report(label.c_str(), "the gather settles the pair", gathered.fault);
      continue;
    }
    const sg::detail::RunOptions& lines = gathered.settled->options;
    if (!lines.showPercentiles.has_value()) {
      report(label.c_str(), "a settled percentile flag", "no flag");
      continue;
    }
    check(*lines.showPercentiles == order.percentiles, label.c_str());
    if (order.percentiles) {
      check(sameValues(lines.percentilePoints, {50.0}),
            (label + ": the points of the line that settled").c_str());
    }
  }
}

// The one line of the rules-file reader the gather reads with: a path this
// helper does not know answers a file of no rule, and each name a case uses
// answers one rule of its own text, so the order of a settled list is read
// from the rules themselves.
auto oneRuleFile(const std::string& path)
    -> std::expected<std::vector<sg::CompareRule>, std::string>
{
  std::string_view text;
  if (path == "second.rules") {
    text = "time@median:lower:5%";
  } else if (path == "more.rules") {
    text = "time@median:lower:1%";
  }
  if (text.empty()) {
    return std::vector<sg::CompareRule> {};
  }
  const auto rule = sg::parseCompareRule(text);
  if (!rule.has_value()) {
    return std::unexpected(rule.error());
  }
  return std::vector<sg::CompareRule> {*rule};
}

auto theLaterRulesFileOfOneSourceStands() -> void
{
  // A second `compare-rules` line of one source keeps its own path, the
  // scalar rule of FR-035, so the rules of the later file are the rules of
  // the run and the disclosure names the row once with that path (FR-063).
  const Gathered twice = gatherSources(
      {},
      {},
      "compare-rules=first.rules\n" "compare-rules=second.rules\n",
      &oneRuleFile);
  if (!twice.settled.has_value()) {
    fail("the gather settles a file that names two rules files");
    return;
  }
  check(twice.settled->rules.size() == 1,
        "the rules of the later file are the rules of the run (FR-063)");
  std::size_t named = 0;
  std::string text;
  for (const sg::detail::OptionUse& use : twice.settled->uses) {
    if (use.row != nullptr
        && std::string_view(use.row->name) == "compare-rules")
    {
      ++named;
      text = use.value;
    }
  }
  check(named == 1 && text == "second.rules",
        "the row is named once, with the path that took effect (FR-040, "
        "FR-063)");

  // The same source naming a rules file, then a rule, then a second rules
  // file: the first path settles before any rule of the source stands, and
  // the later line replaces it the way a scalar of one source replaces its
  // earlier line, so the file joins after the rule the source spells
  // (FR-035, FR-038, FR-063).
  const Gathered paired = gatherSources({},
                                        {},
                                        "compare-rules=first.rules\n"
                                        "compare-rule=time@median:lower:5%\n"
                                        "compare-rules=second.rules\n",
                                        &oneRuleFile);
  if (!paired.settled.has_value()) {
    fail("the gather settles a file that names a rules file twice across a "
         "rule");
    return;
  }
  check(paired.settled->rules.size() == 2,
        "the rule of the source stands before the rules of the later file "
        "(FR-038, FR-063)");
}

auto aRulesLineBeforeItsRulesFileOfOneSourceJoins() -> void
{
  // FR-063, spec D-23, recommendation 8: the rules a source spells stand
  // first and the rules of the file that source names join after them,
  // whichever of the two lines came first, so a `compare-rules` line that
  // follows a `compare-rule` line of one file is read and joins (FR-035,
  // FR-038).
  const Gathered joined = gatherSources(
      {},
      {},
      "compare-rule=time@median:lower:5%\n" "compare-rules=more.rules\n",
      &oneRuleFile);
  if (!joined.settled.has_value()) {
    fail("the gather settles a rule line that a rules-file line follows");
    return;
  }
  std::string seen;
  for (const sg::CompareRule& rule : joined.settled->rules) {
    seen += seen.empty() ? rule.text : ";" + rule.text;
  }
  check(seen == "time@median:lower:5%;time@median:lower:1%",
        "the rule the source spelled stands before the rule of its file "
        "(FR-038, FR-063)");
}

auto anEmptyRulesFilePathNamesNoSource() -> void
{
  // A `compare-rules` line whose path stands empty opens no file and names no
  // rule, so it settles no source and a later line of the same file still
  // settles the list (FR-041, FR-063).
  const Gathered settled =
      gatherSources({},
                    {},
                    "compare-rules=\n" "compare-rule=time@median:lower:5%\n",
                    &oneRuleFile);
  if (!settled.settled.has_value()) {
    fail("the gather settles a rules line whose path stands empty");
    return;
  }
  check(settled.settled->rules.size() == 1,
        "the rule of the later line stands past an empty rules path "
        "(FR-041, FR-063)");
}

auto aLowerRulesPathIsReadUnderACommandLineRule() -> void
{
  // The path a lower source names is held for the read, so the fault of a
  // file that cannot be opened stands even though the words settled the list
  // ahead of it (FR-038, FR-061, FR-063).
  const std::string path = "/speedgun-absent-directory/more.rules";
  const auto refused = sg::detail::readCompareRulesFile(path);
  if (refused.has_value()) {
    fail("the rules reader refuses a path that stands absent");
    return;
  }
  const Gathered gathered =
      gatherSources({"--compare-rule=time@median:lower:5%"},
                    {},
                    "compare-rules=" + path + "\n",
                    &sg::detail::readCompareRulesFile);
  if (gathered.fault != refused.error()) {
    report("the gather reads the rules path a lower source names (FR-038, "
           "FR-061)",
           refused.error(),
           gathered.fault);
  }
}

auto anEmptyCommandLineRulesFileSettlesTheList() -> void
{
  // The words name the list through a file that holds no rule, so the list of
  // the run stands empty and a rule line of the option file adds nothing to it
  // (FR-038, FR-063, spec D-23).
  const Gathered gathered = gatherSources({"--compare-rules=empty.rules"},
                                          {},
                                          "compare-rule=time@median:lower:5%\n",
                                          &oneRuleFile);
  if (!gathered.settled.has_value()) {
    fail("the gather settles a rules file of no rule named by the words");
    return;
  }
  check(gathered.settled->rules.empty(),
        "the empty rules file the words name settles an empty list (FR-063)");
}

auto aRulesFileOfOneSourceTurnsTheGateFail(const std::string& data) -> void
{
  // The rule of the file a source names is a rule of the run, so the gate
  // answer of a pair is the answer of that rule. The contender of the two
  // fixtures stands 3% above the baseline: the 5% rule of the first line
  // passes it and the 1% rule of the named file fails it, so the run takes
  // the failing-gate status of FR-066 (FR-063).
  const std::filesystem::path rules = capturePath(".rules");
  {
    std::ofstream stream(rules);
    stream << "time@median:lower:1%\n";
  }
  const std::filesystem::path file = capturePath(".gateopts");
  {
    std::ofstream stream(file);
    stream << "compare-rule=time@median:lower:5%\n"
           << "compare-rules=" << rules.string() << "\n";
  }
  const Run run = runEntryWithEnv({"--compare=" + data + "/gate_ok_base.json,"
                                       + data + "/gate_ok_contender.json",
                                   "--options=" + file.string()},
                                  {});
  check(run.status == 1,
        "the rule of the file one source names turns the gate to fail "
        "(FR-063, FR-066)");
  std::filesystem::remove(rules);
  std::filesystem::remove(file);
}

auto aRulesFileTheReaderCannotOpenFaultsTheGather() -> void
{
  // The gather opens nothing: the reader it is handed answers for the path,
  // and the gather carries that line unchanged (FR-062, D-05).
  const std::string path = "/speedgun-absent-directory/rules.txt";
  const auto refused = sg::detail::readCompareRulesFile(path);
  if (refused.has_value()) {
    fail("the rules reader refuses a path that stands absent");
    return;
  }
  const Gathered gathered = gatherSources({},
                                          {},
                                          "compare-rules=" + path + "\n",
                                          &sg::detail::readCompareRulesFile);
  if (gathered.fault != refused.error()) {
    report("the gather carries the rules reader's line for a path it cannot "
           "open (FR-062, SC-003)",
           refused.error(),
           gathered.fault);
  }
}

auto theEnvironmentRowOfOneProcessName() -> void
{
  // The entry point turns the process names into rows through one reader per
  // entry, so the shape rules stand there: the prefix, an `=`, and nothing
  // else (FR-018, FR-019).
  const auto ours = sg::detail::environmentRowOf("SPEEDGUN_FILTER=one");
  check(ours.has_value() && ours->name == "SPEEDGUN_FILTER"
            && ours->value == "one",
        "a name of the prefix with a value answers the row of both (FR-019)");
  check(!sg::detail::environmentRowOf("SPEEDGUN_FILTER").has_value(),
        "a name of the prefix with no `=` answers no row (FR-018)");
  check(!sg::detail::environmentRowOf("PATH=/bin").has_value(),
        "a name outside the prefix answers no row (FR-018)");

  check(runChild({"row", "SPEEDGUN_FILTER", "row"}, {"SPEEDGUN_FILTER=row"})
            == 0,
        "the rows of the process carry the name it holds and its value "
        "(FR-018, FR-019)");
}

auto aWordOfTwoValuesFaultsTheEntry() -> void
{
  // FR-024, FR-046: a value word that carries two values is a fault of the
  // source that holds it, never the failure of a contract. The semicolon and
  // the break are the two shapes a scripted source can hold. Each answers one
  // line at status 2 while no benchmark starts: the semicolon of the count
  // list and of the seed from the words and from the environment, the break of
  // the count list from the environment, and the semicolon of the seed from an
  // option file line. The `;` list of FR-024 belongs to the environment alone,
  // so a file line of a list row answers the line of the words too; those four
  // rows stand in the command line test, where a benchmark stands to run
  // (FR-027, FR-035, FR-054, SC-003).
  const std::string counts = faultOf({"--threads=1;2"});
  const std::string seed = faultOf({"--interleave=7;8"});
  oneFaultLine(
      runEntryWithEnv({"--threads=1;2"}, {}),
      "a semicolon of the command line's count list (FR-046, " "SC-003)",
      {counts});
  oneFaultLine(runEntryWithEnv({"--interleave=7;8"}, {}),
               "a semicolon of the command line's seed (FR-054, SC-003)",
               {seed});
  oneFaultLine(runEntryWithEnv({}, {"SPEEDGUN_INTERLEAVE=7;8"}),
               "a semicolon of the seed variable (FR-027, FR-054, SC-003)",
               {"SPEEDGUN_INTERLEAVE: ", seed});
  oneFaultLine(runEntryWithEnv({}, {"SPEEDGUN_THREADS=1\n2"}),
               "a break of the count list variable (FR-027, FR-046, SC-003)",
               {"SPEEDGUN_THREADS: ", faultOf({"--threads=1\n2"})});

  // The fault names the break with its own words and doubles a backslash, so
  // the named break cannot be mistaken for the letters of its own name
  // (FR-024, SC-003).
  const Run doubled = runEntryWithEnv({"--threads=1\\2"}, {});
  const std::string echoed = doubled.out + doubled.err;
  oneFaultLine(doubled,
               "a backslash of the count list (FR-024, FR-046, SC-003)",
               {faultOf({"--threads=1\\2"})});
  check(echoed.find("got '1\\\\2'") != std::string::npos,
        "the refusal doubles the backslash it names (FR-024)");

  // SC-003: every refusal that names the text a source spelled puts a break of
  // that text through the same quoting, whatever the row and whatever the
  // source. Each input below carries a break inside the value, or inside the
  // option word itself, and each answers exactly one line at status 2. The
  // metric and counter rows are refused by the walk of a run, so they stand in
  // the command line test, where a benchmark stands to run (FR-024, FR-027,
  // FR-035, FR-054).
  oneFaultLine(runEntry({"--interleave=7\n8"}),
               "a break of the seed word (FR-054, SC-003)",
               {"--interleave", "7\\n8"});
  oneFaultLine(runEntryWithEnv({}, {"SPEEDGUN_INTERLEAVE=7\n8"}),
               "a break of the seed variable (FR-027, FR-054, SC-003)",
               {"SPEEDGUN_INTERLEAVE: ", "--interleave", "7\\n8"});
  oneFaultLine(runEntryWithEnv({}, {"SPEEDGUN_PLACEMENT=a\nb"}),
               "a break of the placement variable (FR-027, FR-046, SC-003)",
               {"SPEEDGUN_PLACEMENT: ", "--placement", "a\\nb"});
  oneFaultLine(runEntry({"--metric-method=a\nb"}),
               "a break of the metric-method word (FR-033, SC-003)",
               {"--metric-method", "a\\nb"});
  oneFaultLine(runEntry({"--filter=(\n"}),
               "a break of the filter expression (FR-051, SC-003)",
               {"--filter"});
  oneFaultLine(runEntry({"--options=a\nb"}),
               "a break of the option-file path (FR-031, SC-003)",
               {"a\\nb"});
  oneFaultLine(runEntry({"--compare-rules=a\nb"}),
               "a break of the rules-file path (FR-065, SC-003)",
               {"--compare-rules", "a\\nb"});
  oneFaultLine(runEntry({"--compare-rule=a\nb"}),
               "a break of a rule word (FR-063, SC-003)",
               {"a\\nb"});
  oneFaultLine(runEntry({std::string("--fo\no")}),
               "a break of an unknown option word (FR-009, SC-003)",
               {"unknown option", "--fo\\no"});

  // FR-024, SC-003: a carriage return lets a terminal print the second half of
  // a value over the first, and a NUL byte ends the C string a reader is
  // handed, so the quoting names both with their own words.
  oneFaultLine(runEntry({"--interleave=7\r8"}),
               "a carriage return of the seed word (FR-054, SC-003)",
               {"--interleave", "7\\r8"});
  const std::string returnKey = fileFaultOf("filter\r=^bm\n");
  check(returnKey.find("unknown option 'filter\\r'") != std::string::npos,
        "the refusal of an unknown key names the return it carries (FR-033, "
        "SC-003)");

  const std::filesystem::path path = capturePath(".twoopts");
  {
    std::ofstream stream(path);
    stream << "interleave=7;8\n";
  }
  oneFaultLine(runEntryWithEnv({"--options=" + path.string()}, {}),
               "a semicolon of the seed line of an option file (FR-035, "
               "FR-054, SC-003)",
               {path.string() + ":1: ", seed});
  std::filesystem::remove(path);
}

auto aSourceLineWithANulByteIsRefused() -> void
{
  // FR-033, FR-061: a NUL byte ends the text a reader reads, so a line that
  // carries one is refused before any reader takes it, the way a name the
  // table carries is refused. The refusal names the file, the number of the
  // line and the line as it stands, and the run does not start (FR-035,
  // FR-065, SC-003). The two readers answer first in this process, then the
  // entry point answers in a child.
  // The text is built with its length spelled out: a NUL byte ends the text
  // of a literal handed to a string without one, so the byte would vanish
  // before the gather ever saw it.
  const std::string nulOptionLine {"percentiles=50\0x\n", 17};
  const std::string optionFault = fileFaultOf(nulOptionLine);
  check(optionFault.starts_with(
            "options-file.txt:1: expected a value without a NUL byte"),
        "a NUL byte of an option file line is refused by the gather (FR-033, "
        "FR-035)");
  check(optionFault.find("got 'percentiles=50\\0x'") != std::string::npos,
        "the refusal names the NUL byte with its own words (FR-033, SC-003)");
  const std::filesystem::path directPath = capturePath(".nuldirect");
  {
    std::ofstream stream(directPath, std::ios::binary);
    stream.write("time@median:lower:5%\0x\n", 23);
  }
  const auto direct = sg::detail::readCompareRulesFile(directPath.string());
  check(!direct.has_value()
            && direct.error().find(
                   "expected a value without a NUL byte")
                   != std::string::npos,
        "a NUL byte of a rules file line is refused by the reader (FR-061, "
        "FR-065)");
  std::filesystem::remove(directPath);

  const std::filesystem::path optionsPath = capturePath(".nulopts");
  {
    std::ofstream stream(optionsPath, std::ios::binary);
    stream.write("percentiles=50\0x\n", 17);
  }
  oneFaultLine(
      runEntryWithEnv({"--options=" + optionsPath.string()}, {}),
      "a NUL byte of an option file line (FR-033, FR-035, SC-003)",
      {optionsPath.string() + ":1: expected a value without a NUL byte"});
  std::filesystem::remove(optionsPath);

  const std::filesystem::path rulesPath = capturePath(".nulrules");
  {
    std::ofstream stream(rulesPath, std::ios::binary);
    stream.write("time@median:lower:5%\0x\n", 23);
  }
  oneFaultLine(
      runEntryWithEnv({"--compare-rules=" + rulesPath.string(),
                       "--compare=base.json,run.json"},
                      {}),
      "a NUL byte of a rules file line (FR-061, FR-065, SC-003)",
      {rulesPath.string() + ":1: expected a value without a NUL byte"});
  std::filesystem::remove(rulesPath);
}

// The refused word of each of those nine rows, read at the entry point: the
// file line answers one line naming that file and its number, at status 2,
// with no benchmark row (FR-035, FR-050, SC-003).
auto everyRefusedRowLineFaultsTheEntry() -> void
{
  for (const RowSettleCase& row : kRowSettle) {
    const std::filesystem::path path = capturePath(".badrow");
    {
      std::ofstream stream(path);
      stream << row.bad << "\n";
    }
    char what[160];
    std::snprintf(what,
                  sizeof what,
                  "the refused word of the %s line of a file at the entry "
                  "point (FR-035, SC-003)",
                  row.good);
    oneFaultLine(runEntryWithEnv({"--options=" + path.string()}, {}),
                 what,
                 {path.string() + ":1: "});
    std::filesystem::remove(path);
  }
}

auto aRefusedThreadRowFaultsTheEntry() -> void
{
  // SC-003 stands at the entry point for the two lower sources as it does
  // for the command line: the list a reader refuses answers one line behind
  // the variable name, or behind the file and the line number, at status 2,
  // and no benchmark starts. The expectation is the scan's own refusal of
  // the same words, so no reader's words are pinned here (FR-027, FR-035,
  // FR-044, FR-046, D-11).
  const std::string refusal = faultOf({"--threads=nonsense"});
  const Run fromEnvironment =
      runEntryWithEnv({}, {"SPEEDGUN_THREADS=nonsense"});
  oneFaultLine(fromEnvironment,
               "the refused list of SPEEDGUN_THREADS (FR-027, FR-044, SC-003)",
               {"SPEEDGUN_THREADS: ", refusal});

  const std::filesystem::path path = capturePath(".opts");
  {
    std::ofstream stream(path);
    stream << "threads=nonsense\n";
  }
  const Run fromFile = runEntry({"--options=" + path.string()});
  oneFaultLine(
      fromFile,
      "the refused list of an option file line (FR-035, FR-044, " "SC-003)",
      {path.string() + ":1: ", refusal});
  std::filesystem::remove(path);
}

auto aRefusedSeedRowFaultsTheEntry() -> void
{
  // The seed reaches a run through the same two sources, and its refusal
  // answers the same shape at status 2 (FR-027, FR-035, FR-051, FR-054,
  // SC-003). The flagged variable of the seed row is SPEEDGUN_INTERLEAVE,
  // which FR-019 computes from the row name, and the bare word `seed` names
  // no row of the table, so the refused text stands behind the
  // interleave names. The expectation is the scan's own refusal again
  // (D-11).
  const std::string refusal = faultOf({"--interleave=nonsense"});
  const Run fromEnvironment =
      runEntryWithEnv({}, {"SPEEDGUN_INTERLEAVE=nonsense"});
  oneFaultLine(
      fromEnvironment,
      "the refused seed of SPEEDGUN_INTERLEAVE (FR-027, FR-054, SC-003)",
      {"SPEEDGUN_INTERLEAVE: ", refusal});

  const std::filesystem::path path = capturePath(".opts");
  {
    std::ofstream stream(path);
    stream << "interleave=nonsense\n";
  }
  const Run fromFile = runEntry({"--options=" + path.string()});
  oneFaultLine(
      fromFile,
      "the refused seed of an option file line (FR-035, FR-054, " "SC-003)",
      {path.string() + ":1: ", refusal});
  std::filesystem::remove(path);
}

}  // namespace

auto main(int argc, char** argv) -> int
{
  if (const auto child = runChildMode(argc, argv)) {
    return *child;
  }
  // The two gate fixtures of the rules-file case stand under the directory
  // the test registration names (FR-063, FR-066).
  const std::string data = argc > 1 ? argv[1] : std::string();
  theUnknownOptionNamesTheText();
  theNearestNameRidesTheUnknownLine();
  theAmbiguousPrefixNamesBothCandidates();
  theMissingValueNamesTheOption();
  theShortOptionAnswersTheUnknownLine();
  theFlaggedRowsAnswerTheirEnvironmentName();
  theUnflaggedRowsCarryNoEnvironmentName();
  aRepeatableSplitsOnItsItems();
  anItemReachesTheCommandLineRule();
  theFourFlagWordsAndTheirRefusal();
  anUnknownVariableChangesNothing();
  aRefusedValueNamesTheVariableAndTheReaderLine();
  theInterleaveVariableTurnsTheOptionOn();
  theEnvironmentPercentilePairStandsTogether();
  aCommandLineHalfLeavesThePairApart();
  anOptionFileLineSetsItsOption();
  aRefusedNameNamesItsLine();
  anUnknownNameNamesItsLine();
  aSecondOptionsNamesTheOption();
  aRepeatableAccumulatesInOrder();
  aFlagLineReachesTheFlagReader();
  aFileValueReachesTheReader();
  anUnreadableFileAnswersOneLine();
  theThreeSourcesSettleInOrder();
  aHigherSourceReplacesTheWholeList();
  theRuleListReplacesAcrossSources();
  aFileLineThatStartsWithADashNamesItsText();
  aRequiredArgumentLineWithoutAnEqualsNamesTheOption();
  aCommandLineOnlyRunDisclosesNothing();
  theEnvironmentSwitchLeavesTheProcessNamesUnread();
  theThreadListFaultStandsAtTheScan();
  aWordOfDashesAloneNamesNoOption();
  aLowerSourceYieldsToTheCommandLine();
  aRefusedValueOfEveryKindNamesItsSource();
  theLastLineOfASourceMayCarryNoBreak();
  aBareSeedLineStandsForTheSeedZero();
  everyRowOfTheTableSettlesItsRow();
  aGoodLineOfALowerSourceYieldsAndDrops();
  aShadowedBadValueStillNamesItsSource();
  aHeldValueOfALowerSourceNamesItsSource();
  anEmptyFilterWordSettlesTheRowItNames();
  aLowerPercentileListIsCountedAlone();
  theBadValueOfEverySourceSharesTheWordsLine();
  theBareFlagOfEverySourceSettlesItsRow();
  theFourRefusedRowsAnswerTheirOwnLines();
  theEnvironmentWalkReadsTheTableItIsGiven();
  aBadLetterOfAShortGroupAfterALongOption();
  everyRefusedRowLineFaultsTheEntry();
  aWordOfTwoValuesFaultsTheEntry();
  aSourceLineWithANulByteIsRefused();
  oneWordTwiceKeepsTheLastWordOfOneSource();
  thePercentilePairKeepsTheLastWordOfOneSource();
  theLaterRulesFileOfOneSourceStands();
  aRulesLineBeforeItsRulesFileOfOneSourceJoins();
  anEmptyRulesFilePathNamesNoSource();
  aLowerRulesPathIsReadUnderACommandLineRule();
  anEmptyCommandLineRulesFileSettlesTheList();
  aRulesFileOfOneSourceTurnsTheGateFail(data);
  aRulesFileTheReaderCannotOpenFaultsTheGather();
  theEnvironmentRowOfOneProcessName();
  aRefusedThreadRowFaultsTheEntry();
  aRefusedSeedRowFaultsTheEntry();
  std::printf(failures == 0 ? "ok\n" : "%d checks failed\n", failures);
  return failures == 0 ? 0 : 1;
}
