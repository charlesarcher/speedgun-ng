// ============================================================================
// TDD test for the order `--interleave` runs the selected instances in
// (T005, T044 to T050; spec 028, FR-051 to FR-060, SC-003, SC-005, SC-012,
//  contracts/threads-and-interleave.md).
//
// The order comes from research D-15: a fill of `0 .. count - 1`, a walk of
// `i` from `count - 1` down to 1, and a swap of the entry at `i` with the
// entry at `engine() % (i + 1)` from a `std::mt19937_64` seeded with the
// option seed. The engine stream is what the standard fixes, so one seed
// answers one order under every standard library, and the expected
// permutations can stand here as literals. Both literals were measured
// under `g++` 16.2.1 and `clang++` 23.1.3 with a scratch copy of the walk.
//
// The header `detail/options.hpp` declares both seams this file drives.
// `detail::shuffledOrder` is defined in `source/harness/options.cpp` by
// T044, and `detail::scanCommandLine` carries the `interleave` arm and the
// seed forms since T046, so every case here links and runs.
//
// What the scan seam can show about a seed: the scan result carries the
// seed nowhere a reader discloses, because a command-line `--interleave`
// produces no disclosure use (data model E-05) and the number travels in
// the gather result (E-10). So the bare form stands here as the walk
// accepting the word and claiming no value, and the separated form as the
// word standing unclaimed behind `optind` when the walk ends. The seed
// number itself is asserted where the gather settles it.
//
// The held-row cases of T047 and the interrupt case of T049 drive a third
// seam, `sg::speedgunMain`, in process with both output streams captured
// to files, the way `harness_cli_table_test` drives it. The suite they run
// is registered in `main` in the order the cases pin as the registration
// order. A row of the console answers as `<name> <kind>`, the 24-field
// name the console pads and the kind that follows it, so a failing case
// reports the whole print order it saw. The interrupt case takes the rows
// that ran from `detail::shuffledOrder` itself, the seam the walk of T048
// uses, so the expectation travels with the pinned permutation instead of
// restating it. No case waits on a timer and none reads a clock: the
// interrupt arrives from `std::raise(SIGINT)` inside a body, which names
// the instant (research D-17, FR-081).
//
// T048 wrote the walk and the held-row print, so every case above runs
// green in this tree. The shape case T050 adds reads the entry-point
// source, the way `harness_cli_table_test` reads the option table: by
// argument, then by this file's own path, then by the working directory.
//
// Check map:
//   thePinnedPermutations        count 12 answers the two literal orders
//                                at seeds 7 and 8, and each seed answers
//                                its order over two calls (FR-053, SC-005)
//   everyCountAnswersAPermutation
//                                the answer holds `0 .. count - 1` once
//                                each, over four counts (FR-053)
//   theTwoSeedsAnswerDifferentOrders
//                                seed 7 and seed 8 move the instances
//                                apart (SC-005)
//   theBareFormIsGivenWithSeedZero
//                                `--interleave` alone is accepted, claims
//                                no value and leaves no word unclaimed
//                                (FR-051, FR-052)
//   theSeparatedNumberStaysASeparateArgument
//                                `--interleave 7` is accepted and `7`
//                                stands unclaimed after the walk (FR-052)
//   theNonNumericSeedsAnswerTheSeedFault
//                                `-1`, `0x10`, `7x` and `true` each answer
//                                the seed line naming the text it refused
//                                (FR-054, SC-003)
//   theOverLimitDigitStringAnswersTheSeedFault
//                                the digits of 2^64 answer that same line
//                                (FR-054, SC-003)
//   theHeldRowsPrintInRegistrationOrder
//                                a shuffled run prints one row per held
//                                instance, in registration order, and no
//                                row twice (FR-055, FR-056)
//   theFitLineFollowsTheLastHeldRowOfItsFamily
//                                the two fit rows of the complexity family
//                                stand after its last held row and before
//                                the next family (FR-056)
//   thePassesOfOneInstanceStayTogether
//                                three repetitions of an instance print as
//                                four adjacent rows, never spread over the
//                                run (FR-055)
//   theMasterOrderStandsWithoutTheOption
//                                the same command line without the option
//                                prints the same rows in the same order
//                                (FR-059)
//   theInterruptedShuffledRunAnswersTheRowsThatRan
//                                a body that raises `SIGINT` stops the
//                                walk, the rows that ran print in
//                                registration order and the status is the
//                                master interrupt status (FR-057, FR-081)
//   theLoopShapeStandsOverTheWalk
//                                the run line stands inside the walk loop,
//                                the gate's range of `test/cli_shape.sh`
//                                reads the interrupt flag once, the print
//                                pass behind the loop reaches no flag, and
//                                the run-wide postcondition compares the
//                                status with the two run-wide flags
//                                (FR-060, FR-083, SC-012)
// ============================================================================

#include <algorithm>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#include <getopt.h>
#include <unistd.h>

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

// The permutation as one space separated line, the shape the contract
// table prints, so a failing literal reports the order it saw.
auto render(const std::vector<std::size_t>& order) -> std::string
{
  std::string text;
  for (const auto value : order) {
    if (!text.empty()) {
      text += ' ';
    }
    text += std::to_string(value);
  }
  return text;
}

// One row of the pinned table: the answer must read the literal, and a
// second call with the same arguments must read the same order.
auto expectOrder(const std::size_t count,
                 const std::uint64_t seed,
                 const char* expected,
                 const char* what) -> void
{
  const auto first = sg::detail::shuffledOrder(count, seed);
  if (render(first) != expected) {
    ++failures;
    std::printf("FAILED: %s\n  expected: %s\n  actual:   %s\n",
                what,
                expected,
                render(first).c_str());
  }
  const auto second = sg::detail::shuffledOrder(count, seed);
  check(render(second) == render(first),
        "the same count and seed answer the same order twice (FR-053)");
}

auto isPermutationOf(const std::vector<std::size_t>& order,
                     const std::size_t count) -> bool
{
  if (order.size() != count) {
    return false;
  }
  std::vector<std::size_t> sorted(order);
  std::sort(sorted.begin(), sorted.end());
  for (std::size_t i = 0; i < count; ++i) {
    if (sorted[i] != i) {
      return false;
    }
  }
  return true;
}

// The sentence that says the scan accepted the words, the same one
// `harness_cli_sources_test` answers, so a mismatch reports which of the
// two happened.
const char* const kAccepted = "the scan answered no fault";

// One command line through the scan, in the shape a shell hands to `main`,
// answered as the two facts this seam can show: the fault text and the
// words the walk left unclaimed. `getopt` gathers the words it claims for
// nothing behind `optind` when the walk ends, which is where a following
// word stands when the row's argument kind is optional.
struct Outcome
{
  std::string fault;
  std::vector<std::string> unclaimed;
};

auto scanOf(std::initializer_list<const char*> words) -> Outcome
{
  const std::vector<std::string> owned(words.begin(), words.end());
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("speedgun"));
  for (const auto& word : owned) {
    argv.push_back(const_cast<char*>(word.c_str()));
  }
  const auto answer =
      sg::detail::scanCommandLine(static_cast<int>(argv.size()), argv.data());
  Outcome outcome {answer.has_value() ? kAccepted : answer.error(), {}};
  for (int index = optind; index < static_cast<int>(argv.size()); ++index) {
    outcome.unclaimed.push_back(argv[static_cast<std::size_t>(index)]);
  }
  return outcome;
}

// A text comparison that reports both sides, the shape the pinned order
// case already prints.
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

// The unclaimed words as one space separated line, so a failing case
// reports the words it saw.
auto joinWords(const std::vector<std::string>& words) -> std::string
{
  std::string text;
  for (const auto& word : words) {
    if (!text.empty()) {
      text += ' ';
    }
    text += word;
  }
  return text;
}

// The two rows of the pinned table of contracts/threads-and-interleave.md.
auto thePinnedPermutations() -> void
{
  expectOrder(12,
              7ULL,
              "4 0 2 6 10 9 1 5 11 8 7 3",
              "count 12 at seed 7 answers the pinned order (FR-053, SC-005)");
  expectOrder(12,
              8ULL,
              "7 0 6 10 8 2 11 5 9 4 3 1",
              "count 12 at seed 8 answers the pinned order (FR-053, SC-005)");
}

// The postcondition of the contract: the answer is a permutation of
// `0 .. count - 1`. The counts span one instance, a pair, the pinned
// width and a suite above the ten instances SC-005 asks for.
auto everyCountAnswersAPermutation() -> void
{
  for (const std::size_t count : {1ULL, 2ULL, 12ULL, 40ULL}) {
    const auto order = sg::detail::shuffledOrder(count, 7ULL);
    check(isPermutationOf(order, count),
          "the answer holds each index once at that count (FR-053)");
  }
}

// SC-005: a fixed suite under seed 7 and under seed 8 runs in different
// orders.
auto theTwoSeedsAnswerDifferentOrders() -> void
{
  const auto seven = sg::detail::shuffledOrder(12, 7ULL);
  const auto eight = sg::detail::shuffledOrder(12, 8ULL);
  check(render(seven) != render(eight),
        "seed 7 and seed 8 answer different orders (SC-005)");
}

// FR-051: the bare form is given, and it carries no seed text, so the seed
// is 0. At this seam that is the row standing with an optional argument:
// the walk accepts the word, claims no value from it and leaves nothing
// behind. A row that needed a value would answer
// `option '--interleave' needs a value` here.
auto theBareFormIsGivenWithSeedZero() -> void
{
  const Outcome outcome = scanOf({"--interleave"});
  if (outcome.fault != kAccepted) {
    report("the bare form is accepted and claims no value (FR-051, FR-052)",
           kAccepted,
           outcome.fault);
    return;
  }
  check(outcome.unclaimed.empty(),
        "the bare form leaves no word unclaimed (FR-052)");
}

// FR-052: the seed binds only in the attached form. The row's argument
// kind is optional, so `--interleave 7` leaves `7` an unclaimed positional
// the way a stray word stands today, and the seed stays 0.
auto theSeparatedNumberStaysASeparateArgument() -> void
{
  const Outcome outcome = scanOf({"--interleave", "7"});
  if (outcome.fault != kAccepted) {
    report("the separated number is accepted at all (FR-052)",
           kAccepted,
           outcome.fault);
    return;
  }
  if (joinWords(outcome.unclaimed) != "7") {
    report("the separated number stands unclaimed after the walk (FR-052)",
           "7",
           joinWords(outcome.unclaimed));
  }
}

// One row of the seed fault table: the attached text, the one line the scan
// must answer byte for byte, and the check that the answer holds one line.
// The entry point turns that text into one line on standard error at status
// 2 (FR-011, SC-003); the scan itself answers no status.
auto expectSeedFault(const std::string& text, const char* what) -> void
{
  const std::string word = "--interleave=" + text;
  const std::string expected =
      "--interleave: the seed '" + text + "' is not an unsigned decimal value";
  const Outcome outcome = scanOf({word.c_str()});
  if (outcome.fault != expected) {
    report(what, expected, outcome.fault);
    return;
  }
  check(outcome.fault.find('\n') == std::string::npos,
        "that fault is one line (FR-054, SC-003)");
}

// FR-054: a seed text is unsigned decimal digits alone, consumed whole.
// The four texts that hold no such number each answer the line naming the
// option and the text it refused.
auto theNonNumericSeedsAnswerTheSeedFault() -> void
{
  expectSeedFault("-1", "a signed seed answers the seed line (FR-054, SC-003)");
  expectSeedFault("0x10",
                  "a hexadecimal seed answers the seed line (FR-054, SC-003)");
  expectSeedFault(
      "7x",
      "digits with a trailing letter answer the seed line " "(FR-054, SC-003)");
  expectSeedFault("true", "a flag word answers the seed line (FR-054, SC-003)");
}

// The limit itself: 2^64 is one past the largest value a seed holds, and
// the text is digits alone, so the range rule is what refuses it.
auto theOverLimitDigitStringAnswersTheSeedFault() -> void
{
  expectSeedFault("18446744073709551616",
                  "a digit string past the 64-bit limit answers the seed "
                  "line (FR-054, SC-003)");
}

// The suite the held-row and interrupt cases run, and the registration
// order `main` establishes for it: the raising body stands between the
// plain families and the complexity family, so the fit of that family
// prints in the middle of the print order and not at its end.
constexpr const char* kIlAlpha = "bmIlAlpha";
constexpr const char* kIlBeta = "bmIlBeta";
constexpr const char* kIlRaises = "bmIlRaises";
constexpr const char* kIlQuad = "bmIlQuad";
constexpr const char* kIlGamma = "bmIlGamma";

// The three filters: the held rows without a raising body, the three plain
// families alone, and the four instances of the interrupt case.
constexpr const char* kHeldFilter = "^bmIl(Alpha|Beta|Gamma|Quad)";
constexpr const char* kPlainFilter = "^bmIl(Alpha|Beta|Gamma)";
constexpr const char* kInterruptFilter = "^bmIl(Alpha|Beta|Gamma|Raises)";

// The seed of the interrupt case: at four selected instances it walks
// `3 0 2 1`, so the raising body sits third in the walk. The second
// instance of the registration order never runs, the last one runs first,
// and the rows that ran print in an order that is neither the walk order
// nor a prefix of it, which is what separates a held-row print from a run
// that never shuffled at all.
constexpr std::uint64_t kInterruptSeed = 8ULL;

// The master interrupt status of contracts/cli.md, read at
// `source/harness/cli.cpp` where the flag sets the status.
constexpr int kInterruptStatus = 1;

auto bmIlAlpha(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmIlBeta(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmIlRaises(sg::State& state) -> void
{
  for (auto _ : state) {
  }
  (void)std::raise(SIGINT);
}

auto bmIlQuad(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmIlGamma(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

// One run of the entry point with both output streams captured, the shape
// `harness_cli_table_test` drives it in: the printed text and the answered
// status are read together, and the console text of a run stays out of
// this file's own verdict output. The capture files stand in the system
// temporary directory under this process number.
struct Run
{
  int status = 0;
  std::string out;
  std::string err;
};

auto capturePath(const char* kind) -> std::filesystem::path
{
  return std::filesystem::temp_directory_path()
      / ("harness_cli_interleave_" + std::to_string(static_cast<long>(getpid()))
         + kind);
}

auto readText(const std::filesystem::path& path) -> std::string
{
  std::ifstream file(path);
  std::ostringstream text;
  text << file.rdbuf();
  return text.str();
}

auto runEntry(std::initializer_list<const char*> words) -> Run
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_cli_interleave_test"));
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

// The lines one capture printed.
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

auto trimRight(const std::string& text) -> std::string
{
  std::size_t end = text.size();
  while (end > 0 && text[end - 1] == ' ') {
    --end;
  }
  return text.substr(0, end);
}

// The print order of one capture as one line of `<name> <kind>` entries:
// the 24-field name the console pads, and the kind of the row it prints.
// A line that carries no suite name, the version line and the disclosure
// line among them, holds no row and answers nothing.
auto rowTokens(const std::string& text) -> std::vector<std::string>
{
  constexpr std::size_t kNameField = 24;
  std::vector<std::string> tokens;
  for (const auto& line : linesOf(text)) {
    if (line.size() <= kNameField) {
      continue;
    }
    const std::string name = trimRight(line.substr(0, kNameField));
    if (name.rfind("bmIl", 0) != 0) {
      continue;
    }
    const std::string rest = line.substr(kNameField);
    const char* kind = nullptr;
    if (rest.rfind(" iterations=", 0) == 0) {
      kind = "run";
    } else if (rest.find("AGGREGATE") != std::string::npos) {
      kind = "aggregate";
    } else if (rest.find("BIG_O") != std::string::npos) {
      kind = "big_o";
    } else if (rest.find("RMS") != std::string::npos) {
      kind = "rms";
    } else if (rest.find("SKIPPED:") != std::string::npos) {
      kind = "skipped";
    }
    if (kind != nullptr) {
      tokens.push_back(name + " " + kind);
    }
  }
  return tokens;
}

// The measured rows of a run, the aggregate and fit rows of a family left
// out, so the first case states the held rows alone.
auto runRowsOf(const std::vector<std::string>& tokens)
    -> std::vector<std::string>
{
  std::vector<std::string> rows;
  for (const auto& token : tokens) {
    if (token.ends_with(" run")) {
      rows.push_back(token);
    }
  }
  return rows;
}

// The print order as one line, so a failing case reports the whole order
// it saw against the whole order it expected.
auto renderRows(const std::vector<std::string>& tokens) -> std::string
{
  std::string text;
  for (const auto& token : tokens) {
    if (!text.empty()) {
      text += "; ";
    }
    text += token;
  }
  return text;
}

// The print order of the held set: five instances, the two argument
// instances of the complexity family between the plain ones.
const std::string kHeldRows =
    "bmIlAlpha run; bmIlBeta run; bmIlQuad/2 run; bmIlQuad/4 run; "
    "bmIlGamma run";

// The same five rows with the fit of the complexity family after the last
// row of that family and before the row of the next one.
const std::string kHeldRowsWithFit =
    "bmIlAlpha run; bmIlBeta run; bmIlQuad/2 run; bmIlQuad/4 run; "
    "bmIlQuad big_o; bmIlQuad rms; bmIlGamma run";

// The print order of three plain families under three repetitions: four
// adjacent rows per instance, three passes and the aggregate of that
// instance, and no row of another instance between them.
const std::string kRepetitionRows =
    "bmIlAlpha run; bmIlAlpha run; bmIlAlpha run; bmIlAlpha aggregate; "
    "bmIlBeta run; bmIlBeta run; bmIlBeta run; bmIlBeta aggregate; "
    "bmIlGamma run; bmIlGamma run; bmIlGamma run; bmIlGamma aggregate";

// How many times `needle` stands inside `line`, at every offset it opens.
auto countOf(const std::string& line, const std::string& needle) -> std::size_t
{
  std::size_t count = 0;
  for (auto at = line.find(needle); at != std::string::npos;
       at = line.find(needle, at + 1))
  {
    ++count;
  }
  return count;
}

// The entry-point source the shape case reads, found by argument, then by
// this file's own path, then by the working directory, the three ways the
// table test of T004 finds its table file. An empty answer means no
// candidate is a readable file.
auto entryPointF(const std::string& root) -> std::filesystem::path
{
  const std::vector<std::filesystem::path> candidates = {
      root.empty() ? std::filesystem::path {}
                   : std::filesystem::path(root) / "source/harness/cli.cpp",
      std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()
          / "source/harness/cli.cpp",
      std::filesystem::path("source/harness/cli.cpp")};
  for (const auto& path : candidates) {
    auto readable = std::error_code {};
    if (!path.empty() && std::filesystem::is_regular_file(path, readable)) {
      return path;
    }
  }
  return {};
}

// FR-055, FR-056: the loop of a shuffled run holds its rows and prints
// nothing; the cold pass after it prints every held row in registration
// order. One row per instance, in that order, is what says both halves:
// a row printed inside the loop would stand out of that order or twice.
auto theHeldRowsPrintInRegistrationOrder() -> void
{
  const Run run =
      runEntry({"--filter", kHeldFilter, "--iterations=1", "--interleave=7"});
  const auto rows = runRowsOf(rowTokens(run.out));
  if (renderRows(rows) != kHeldRows) {
    report("a shuffled run prints every held row in registration order "
           "(FR-055, FR-056)",
           kHeldRows,
           renderRows(rows));
  }
}

// FR-056: a family's fit prints after the last held row of that family.
// The complexity family stands between two plain ones in the registration
// order, so its two fit rows have a place to be and an order to be wrong
// in.
auto theFitLineFollowsTheLastHeldRowOfItsFamily() -> void
{
  const Run run =
      runEntry({"--filter", kHeldFilter, "--iterations=1", "--interleave=7"});
  const auto tokens = rowTokens(run.out);
  if (renderRows(tokens) != kHeldRowsWithFit) {
    report("a family's fit line follows the last held row of that family "
           "(FR-056)",
           kHeldRowsWithFit,
           renderRows(tokens));
  }
}

// FR-055: the walk moves whole instances, so the passes and the
// repetitions of one instance stay together however the instances are
// ordered.
auto thePassesOfOneInstanceStayTogether() -> void
{
  const Run run = runEntry({"--filter",
                            kPlainFilter,
                            "--iterations=1",
                            "--repetitions=3",
                            "--interleave=7"});
  const auto tokens = rowTokens(run.out);
  if (renderRows(tokens) != kRepetitionRows) {
    report(
        "the passes and repetitions of one instance stay together " "(FR-055)",
        kRepetitionRows,
        renderRows(tokens));
  }
}

// FR-059: with the option absent the master print order stands. The same
// command line without the option answers the same rows in the same order.
auto theMasterOrderStandsWithoutTheOption() -> void
{
  const Run run =
      runEntry({"--filter", kPlainFilter, "--iterations=1", "--repetitions=3"});
  const auto tokens = rowTokens(run.out);
  if (renderRows(tokens) != kRepetitionRows) {
    report(
        "the master print order stands when the option is absent " "(FR-059)",
        kRepetitionRows,
        renderRows(tokens));
  }
}

// FR-057, FR-081: the interrupt arrives from a body that raises `SIGINT`,
// so the case names the instant with no timer wait and no clock read. The
// walk order comes from the seam the walk itself uses, so the rows that
// ran are the prefix of that order up to the raising instance, and the
// print order is the registration order of those rows. The instance the
// walk never reaches holds no row.
auto theInterruptedShuffledRunAnswersTheRowsThatRan() -> void
{
  const std::vector<std::string> selected {
      kIlAlpha, kIlBeta, kIlRaises, kIlGamma};
  const auto order = sg::detail::shuffledOrder(selected.size(), kInterruptSeed);
  std::vector<std::string> ran;
  for (const auto index : order) {
    if (index >= selected.size()) {
      fail("the walk answers an index of a selected instance (FR-053)");
      return;
    }
    ran.push_back(selected[index]);
    if (selected[index] == kIlRaises) {
      break;
    }
  }
  std::vector<std::string> expected;
  for (const auto& name : selected) {
    if (std::find(ran.begin(), ran.end(), name) == ran.end()) {
      continue;
    }
    expected.push_back(name + (name == kIlRaises ? " skipped" : " run"));
  }

  const std::string seedWord = "--interleave=" + std::to_string(kInterruptSeed);
  const Run run = runEntry(
      {"--filter", kInterruptFilter, "--iterations=1", seedWord.c_str()});
  const auto tokens = rowTokens(run.out);
  if (renderRows(tokens) != renderRows(expected)) {
    report("an interrupted shuffled run prints the rows that ran in "
           "registration order (FR-057)",
           renderRows(expected),
           renderRows(tokens));
  }
  if (run.status != kInterruptStatus) {
    report("an interrupted shuffled run answers the master interrupt status "
           "(FR-057)",
           std::to_string(kInterruptStatus),
           std::to_string(run.status));
  }
}

// T050, FR-060, FR-083, SC-012: the shape of the run loop now that the run
// walks a settled order. `test/cli_shape.sh` reads the loop from the line
// that runs a benchmark to the line that closes it and asks for one reading
// of the interrupt flag inside that range. The walk reaches the instances in
// its own order and prints the held rows behind the loop, so the reading
// that decides the break stays where the flag is raised, inside the loop,
// and the print pass behind it reaches no flag at all. The case reads the
// same range out of the same source, so a move of the reading answers red
// here as it does in the gate.
auto theLoopShapeStandsOverTheWalk(const std::string& root) -> void
{
  const auto path = entryPointF(root);
  if (path.empty()) {
    fail("the entry-point source of the shape case is readable");
    return;
  }
  const std::vector<std::string> lines = linesOf(readText(path));
  const auto missing = lines.size();
  auto runLine = missing;
  auto walkLine = missing;
  auto closeLine = missing;
  for (std::size_t index = 0; index < lines.size(); ++index) {
    if (runLine == missing
        && lines.at(index).find("const auto result = runner.run(*entry);")
            != std::string::npos)
    {
      runLine = index;
    }
    if (walkLine == missing
        && lines.at(index).find(": walk)") != std::string::npos)
    {
      walkLine = index;
    }
    if (runLine != missing && closeLine == missing && index > runLine
        && lines.at(index) == "  }")
    {
      closeLine = index;
    }
  }
  check(runLine < missing,
        "the line that runs a benchmark stands in the entry point (FR-060)");
  check(walkLine < missing && walkLine < runLine,
        "the loop walks an order settled before it, the walk of FR-051 and "
        "FR-055 (FR-060)");
  check(closeLine < missing,
        "the line that closes the loop, the end of the gate's range, stands "
        "after the run line (FR-060)");
  if (closeLine == missing) {
    return;
  }

  auto reads = std::size_t {0};
  auto localReadings = std::size_t {0};
  for (std::size_t index = runLine; index < closeLine; ++index) {
    reads += countOf(lines.at(index), "interruptFlag().load()");
    localReadings += countOf(lines.at(index), "bool interrupted =");
  }
  check(reads == 1,
        "the loop range reads the interrupt flag once per pass (FR-036, "
        "FR-060)");
  check(localReadings == 1,
        "the loop holds one local reading of the flag (FR-060)");

  auto readsBehindTheLoop = std::size_t {0};
  for (std::size_t index = closeLine; index < lines.size(); ++index) {
    readsBehindTheLoop += countOf(lines.at(index), "interruptFlag().load()");
  }
  check(readsBehindTheLoop == 0,
        "the held-row print pass reaches no interrupt flag (FR-056, FR-060)");

  auto comparesTheRunWideFlags = false;
  for (std::size_t index = closeLine; index + 1 < lines.size(); ++index) {
    if (lines.at(index).find("SG_ENSURE((status != 0)") != std::string::npos) {
      const std::string pair = lines.at(index) + lines.at(index + 1);
      comparesTheRunWideFlags = pair.find("anyFailed") != std::string::npos
          && pair.find("anyInterrupted") != std::string::npos
          && pair.find("interruptFlag") == std::string::npos;
    }
  }
  check(comparesTheRunWideFlags,
        "the run-wide postcondition compares the status with the two "
        "run-wide flags and reaches no flag itself (FR-060)");
}

}  // namespace

auto main(const int argc, char** argv) -> int
{
  // The handles keep the registrations alive for the whole run, and the
  // order of these calls is the registration order the cases pin.
  [[maybe_unused]] const auto alpha =
      sg::registerBenchmark(&bmIlAlpha, kIlAlpha);
  [[maybe_unused]] const auto beta = sg::registerBenchmark(&bmIlBeta, kIlBeta);
  [[maybe_unused]] const auto raises =
      sg::registerBenchmark(&bmIlRaises, kIlRaises);
  auto quad = sg::registerBenchmark(&bmIlQuad, kIlQuad);
  quad.arg(2).arg(4).complexity(sg::BigO::N);
  [[maybe_unused]] const auto gamma =
      sg::registerBenchmark(&bmIlGamma, kIlGamma);

  thePinnedPermutations();
  everyCountAnswersAPermutation();
  theTwoSeedsAnswerDifferentOrders();
  theBareFormIsGivenWithSeedZero();
  theSeparatedNumberStaysASeparateArgument();
  theNonNumericSeedsAnswerTheSeedFault();
  theOverLimitDigitStringAnswersTheSeedFault();
  theHeldRowsPrintInRegistrationOrder();
  theFitLineFollowsTheLastHeldRowOfItsFamily();
  thePassesOfOneInstanceStayTogether();
  theMasterOrderStandsWithoutTheOption();
  theInterruptedShuffledRunAnswersTheRowsThatRan();
  theLoopShapeStandsOverTheWalk(argc > 1 ? argv[1] : std::string {});
  std::printf(failures == 0 ? "ok\n" : "%d checks failed\n", failures);
  return failures == 0 ? 0 : 1;
}
