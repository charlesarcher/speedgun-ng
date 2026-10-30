// specs/029 FR-080 to FR-082, PD-16, PD-17: the eight hunt options, their
// value ranges, their refusals, the glob matcher and the conflict check. The
// scan of `cli.cpp` holds the table rows, the eight named constants and one
// call per row; every parse body lives here (D-22). The `@file` reader these
// routes share stands in `hunt_selection.cpp` (specs/033 R-09).

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <ios>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "speedgun-ng/hunt.hpp"

#include "detail/hunt_selection.hpp"
#include "detail/internal.hpp"
#include "detail/options.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_system.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg::detail
{

namespace
{

// FR-034, FR-081: a value outside the range its option states is a usage error
// that reports one line naming the option, what it expected and the text it
// read, and leaves with status 2 before anything runs. The form is the
// percentile-list refusal of `cli.cpp:73-81`.
[[noreturn]] auto rejectHuntValue(const char* option,
                                  const char* expected,
                                  const char* text) -> void
{
  std::fprintf(stderr, "%s: %s, got '%s'\n", option, expected, text);
  std::exit(2);
}

// D-3, FR-032, specs/040 FR-010: `score` is the rank key N14b brings, so the
// refusal for that word names the work that owns it beside the words this
// option takes. Any other word is a malformed value and keeps the refusal
// above.
[[noreturn]] auto rejectHuntRankReserved(const char* text) -> void
{
  std::fprintf(stderr,
               "--hunt-rank: '%s' is reserved for N14b; "
               "--hunt-rank takes rate or cv\n",
               text);
  std::exit(2);
}

// The refusals that name a selection carry their whole
// line, because the line names the value in the spelling the option carried it
// (FR-081). Every hunt refusal stands on stderr; the `hunt:` lines the run
// prints stand on stdout (D-20).
[[noreturn]] auto rejectHuntSelection(const std::string& line) -> void
{
  std::fprintf(stderr, "%s\n", line.c_str());
  std::exit(2);
}

// One whole number of the closed range its option states. The read is unsigned
// with the range test beside it: a hunt count has no negative spelling, and a
// wrapped one would seat a pass no host can hold.
// A whole number read from the whole of the text. The answer says whether the
// text is a number at all, so the caller that tests a range can quote the text
// the option carried, whole.
auto scanHuntNumber(const char* text, std::uint64_t& value) -> bool
{
  errno = 0;
  char* end = nullptr;
  const unsigned long long raw = std::strtoull(text, &end, 10);
  // A count is spelled with digits alone: `strtoull` would otherwise wrap a
  // leading minus into a huge unsigned value that the high bound has to
  // catch by accident, and it skips leading whitespace the option line
  // never states.
  if (errno != 0 || end == text || *end != '\0'
      || !std::isdigit(static_cast<unsigned char>(*text)))
  {
    return false;
  }
  value = static_cast<std::uint64_t>(raw);
  return true;
}

// Whether one text is spelled as the `e` clause states it: decimal digits with
// at most one decimal point and at least one digit
// (contracts/budget-grammar.md, FR-004). The shared seconds reader is a C
// number reader, which also takes a sign, a decimal exponent and a hexadecimal
// float, three spellings the clause grammar holds no part of; the test stands
// beside the digits-alone rule that `scanHuntNumber` keeps for the whole-count
// clauses (T050).
[[nodiscard]] auto isDecimalSeconds(const std::string& text) -> bool
{
  bool holdsDigit = false;
  bool holdsPoint = false;
  for (const char symbol : text) {
    if (symbol == '.') {
      if (holdsPoint) {
        return false;
      }
      holdsPoint = true;
      continue;
    }
    if (!std::isdigit(static_cast<unsigned char>(symbol))) {
      return false;
    }
    holdsDigit = true;
  }
  return holdsDigit;
}

auto readHuntCount(const char* text,
                   const char* option,
                   const char* expected,
                   const std::uint64_t low,
                   const std::uint64_t high) -> std::uint64_t
{
  std::uint64_t value = 0;
  if (!scanHuntNumber(text, value) || value < low || value > high) {
    rejectHuntValue(option, expected, text);
  }
  return value;
}

// The percent threshold of `--hunt-noise`: a finite number above zero, so a
// threshold of `inf` refuses: no rule of the run may stand above every
// event (D-8).
auto readHuntPercent(const char* text) -> double
{
  errno = 0;
  char* end = nullptr;
  const double value = std::strtod(text, &end);
  if (errno != 0 || end == text || *end != '\0' || !std::isfinite(value)
      || value <= 0.0)
  {
    rejectHuntValue("--hunt-noise", "expected a percentage above 0", text);
  }
  return value;
}

// One refusal for the hunt beside an option it cannot share a run with.
[[noreturn]] auto rejectHuntCombination(const char* other) -> void
{
  rejectHuntSelection(std::string("--hunt cannot combine with ") + other);
}

// The raw event code one element carries, read the way this route reads an
// element: the shape test itself is the shared `detail::hasRawCodeShape`
// (D-11, spec PC-6); what stands here is the cut, which takes the leaf of the
// entry and drops the trailing glob characters so a pattern's leaf is still
// read as a code.
[[nodiscard]] auto rawCodeOf(const std::string_view entry) -> std::string_view
{
  std::string_view leaf = detail::leafPartOf(entry);
  while (!leaf.empty() && (leaf.back() == '*' || leaf.back() == '?')) {
    leaf.remove_suffix(1);
  }
  return detail::hasRawCodeShape(leaf) ? leaf : std::string_view {};
}

// The catalog addresses of the host: the machine object's own entries first,
// then the walk of `detail::catalogWalkOrder`, which is the listing
// `printObject` of `catalog.cpp` prints - the address has the one spelling
// `Object::path() + "/" + entry.name`, in the order the walk reaches the
// devices and the order each device reports its counters (spec PC-1).
auto collectCatalog(const counters::Object& object,
                    std::vector<std::string>& addresses) -> void
{
  const auto addEntries = [&addresses](const counters::Object& node)
  {
    const std::string prefix = std::string(node.path()) + "/";
    for (const auto& entry : node.counters()) {
      addresses.push_back(prefix + std::string(entry.name));
    }
  };
  addEntries(object);
  for (const auto* walked : detail::catalogWalkOrder(object)) {
    addEntries(*walked);
  }
}

// True when one object of the walk or one of its children publishes `leaf` in
// the countable state. The anchor of D-13 is the address the rate divides by,
// so a leaf the host refuses is no anchor for a norm the caller stated.
[[nodiscard]] auto holdsCountableAnchor(const counters::Object& object,
                                        const std::string_view leaf) -> bool
{
  for (const auto& entry : object.counters()) {
    if (std::string_view {entry.name} == leaf
        && entry.avail == counters::Availability::COUNTABLE)
    {
      return true;
    }
  }
  for (const auto* child : object.children()) {
    if (holdsCountableAnchor(*child, leaf)) {
      return true;
    }
  }
  return false;
}

// The selection text with each address kept the first time it stands, the
// deduplication of FR-006: the same address through a list and a file is one
// column, and the joined text of a `@file` states it once.
[[nodiscard]] auto dedupAddresses(std::vector<std::string> addresses)
    -> std::vector<std::string>
{
  std::vector<std::string> unique;
  for (const auto& address : addresses) {
    const bool seen =
        std::find(unique.begin(), unique.end(), address) != unique.end();
    if (!seen) {
      unique.push_back(address);
    }
  }
  return unique;
}

auto joinAddresses(const std::vector<std::string>& addresses) -> std::string
{
  std::string text;
  for (const auto& address : addresses) {
    if (!text.empty()) {
      text += ",";
    }
    text += address;
  }
  return text;
}

// The record one hunt option writes into, created when the option stands
// ahead of `--hunt` and so finds no record yet. The parked record carries no
// selection: that empty text is the mark of a scan which has not read `--hunt`,
// and the expansion below settles or discards the record once the scan is
// over, so the accepted run follows from the values and never from their
// position in argv (D-22).
[[nodiscard]] auto huntValues(RunOptions& options) -> hunt::HuntOptions&
{
  if (!options.hunt.has_value()) {
    options.hunt = hunt::HuntOptions {};
    options.hunt->selection.clear();
  }
  return *options.hunt;
}

// True when the scan read `--hunt` beside the values it collected. A record
// with no selection is a hunt option that never met a `--hunt`, and such a run
// carries no hunt (contracts/hunt-cli.md, spec Assumptions).
[[nodiscard]] auto huntRequested(const RunOptions& options) -> bool
{
  return options.hunt.has_value() && !options.hunt->selection.empty();
}

}  // namespace

// specs/029 D-22, PD-17: the eight parse calls, the post-scan expansion and the
// conflict check stand here so the scan of `cli.cpp` holds table rows and calls
// alone. `hunt.hpp` states the settled record; the ranges and the refusal texts
// are contracts/hunt-cli.md.
//
// Every option writes the record whether or not `--hunt` has stood yet, so the
// order of the options on the command line decides nothing: an option ahead of
// `--hunt` parks the record with an empty selection and the expansion settles
// it once the scan is over. A value outside its range still refuses at the
// option that read it, before the record means anything.

// \pre `text` is the value getopt_long read, or null when `--hunt` carried no
//      value at all, which is the attached-form-only reading of an optional
//      argument row.
// \post the hunt field holds the selection text, with no value standing `all`
//       (D-1) and `none` standing the run with no hunt: the field is empty, so
//       discovery and planning never see it (FR-001). The shape of the text is
//       the expansion's decision, because a `@path` must be read where the
//       providers stand registered.
// \post an attached value with nothing in it is the empty address line and
//       leaves with the line an empty address line of a list leaves, because
//       an empty selection is the mark of a record the scan parked.
auto parseHuntSelection(const char* text, RunOptions& options) -> void
{
  if (text != nullptr && *text == '\0') {
    rejectHuntSelection("--hunt: '' is not a catalog address");
  }
  const std::string_view value =
      text != nullptr ? std::string_view {text} : std::string_view {"all"};
  if (value == "none") {
    options.hunt.reset();
    return;
  }
  huntValues(options).selection = std::string(value);
}

// \pre `text` is the value getopt_long read for a required-argument row.
// \post the norm is the word named, and a word outside the three leaves with
//       the one refusal line (D-3, D-6).
auto parseHuntNorm(const char* text, RunOptions& options) -> void
{
  SG_ASSERT(text != nullptr,
            "the option row is a required-argument row, so getopt_long answers "
            "a value for it (FR-082)");
  const std::string_view word {text};
  hunt::HuntNorm norm = hunt::HuntNorm::INSTR;
  if (word == "instr") {
    norm = hunt::HuntNorm::INSTR;
  } else if (word == "cycle") {
    norm = hunt::HuntNorm::CYCLE;
  } else if (word == "iter") {
    norm = hunt::HuntNorm::ITER;
  } else {
    rejectHuntValue("--hunt-norm", "expected instr, cycle or iter", text);
  }
  huntValues(options).norm = norm;
}

// \pre `text` is the value getopt_long read for a required-argument row.
// \post the rank is the word named; `sensitivity` and `delta` are the keys
//       specs/040 adds (FR-010), `score` is the key N14b brings and leaves
//       with the refusal naming N14b, and a word outside the five leaves with
//       the refusal naming the accepted words (D-3, FR-032, FR-081).
auto parseHuntRank(const char* text, RunOptions& options) -> void
{
  SG_ASSERT(text != nullptr,
            "the option row is a required-argument row, so getopt_long answers "
            "a value for it (FR-082)");
  const std::string_view word {text};
  hunt::HuntRank rank = hunt::HuntRank::RATE;
  if (word == "rate") {
    rank = hunt::HuntRank::RATE;
  } else if (word == "cv") {
    rank = hunt::HuntRank::CV;
  } else if (word == "sensitivity") {
    rank = hunt::HuntRank::SENSITIVITY;
  } else if (word == "delta") {
    rank = hunt::HuntRank::DELTA;
  } else if (word == "score") {
    rejectHuntRankReserved(text);
  } else {
    rejectHuntValue("--hunt-rank", "expected rate or cv", text);
  }
  huntValues(options).rank = rank;
}

// \pre `text` is the value getopt_long read for a required-argument row.
// \post the ranked table size is the count named, in 1 to 10000 (D-7).
auto parseHuntTop(const char* text, RunOptions& options) -> void
{
  SG_ASSERT(text != nullptr,
            "the option row is a required-argument row, so getopt_long answers "
            "a value for it (FR-082)");
  const std::uint64_t top =
      readHuntCount(text, "--hunt-top", "expected 1 to 10000", 1, 10'000);
  huntValues(options).top = static_cast<std::size_t>(top);
}

// \pre `text` is the value getopt_long read for a required-argument row.
// \post the noise threshold is the percent named, above zero (D-8).
auto parseHuntNoise(const char* text, RunOptions& options) -> void
{
  SG_ASSERT(text != nullptr,
            "the option row is a required-argument row, so getopt_long answers "
            "a value for it (FR-082)");
  const double noise = readHuntPercent(text);
  huntValues(options).noise = noise;
}

// \pre `text` is the value getopt_long read for a required-argument row.
// \post the repeats of the hunt stages are the count named, in 1 to 1000
//       (D-2); `--repetitions` keeps its own meaning for the benchmark rows.
auto parseHuntRepeats(const char* text, RunOptions& options) -> void
{
  SG_ASSERT(text != nullptr,
            "the option row is a required-argument row, so getopt_long answers "
            "a value for it (FR-082)");
  const std::uint64_t repeats =
      readHuntCount(text, "--hunt-repeats", "expected 1 to 1000", 1, 1'000);
  huntValues(options).repeats = static_cast<std::size_t>(repeats);
}

// The clause words `--hunt-budget=` answers: the two-stage pair spec 029
// shipped, the sample budget 033 adds, and the multiplex cap US3 joins. One
// walk of the value reads the clauses, each kind at most once and in any order
// (contracts/budget-grammar.md, FR-004, R-01).
constexpr std::string_view kBudgetShape = "expected <n>s, <n>p, <n>e or <n>g";

// \pre `text` is the value getopt_long read for a required-argument row.
// \post the two-stage budget holds the count the `s` or `p` clause named, above
//       zero, in the unit its suffix spelled, and stands unset when the value
//       carries neither clause; the sample budget holds the whole nanoseconds
//       of the `e` clause; the multiplex cap holds the whole count the `g`
//       clause named, in 1 to 10000 (FR-001, FR-003, FR-004, FR-012). A value
//       that carries one
//       `s` or `p` clause settles exactly as spec 029 shipped it, and the last
//       value of the scan stands as the value of the run (R-01).
// \post every refusal quotes the whole value the option carried: an empty
//       clause, a kind spelled twice, a clause whose suffix is missing or
//       unknown, a count that is no number, an `e` word below one nanosecond or
//       above the seconds bound, a `g` count outside 1 to 10000, and a value
//       that opens or ends with a blank each leave one line naming the value
//       (FR-081, contracts/budget-grammar.md).
auto parseHuntBudget(const char* text, RunOptions& options) -> void
{
  SG_ASSERT(text != nullptr,
            "the option row is a required-argument row, so getopt_long answers "
            "a value for it (FR-082)");
  const std::string_view value {text};
  hunt::HuntBudget budget;
  bool twoStage = false;
  std::optional<std::uint64_t> sample;
  std::optional<std::size_t> cap;
  std::size_t seen = 0;
  std::size_t at = 0;
  while (true) {
    const std::size_t comma = value.find(',', at);
    const std::string_view clause = value.substr(
        at, (comma == std::string_view::npos ? value.size() : comma) - at);
    if (clause.empty()) {
      rejectHuntValue("--hunt-budget", kBudgetShape.data(), text);
    }
    // The kind bit stands for the clause letter, so a kind spelled twice is one
    // refusal and a new kind joins the switch below with one arm
    // (contracts/budget-grammar.md).
    const char letter = clause.back();
    const std::size_t kind = letter >= 'a' && letter <= 'z'
        ? std::size_t {1} << static_cast<unsigned>(letter - 'a')
        : 0;
    if (kind == 0 || (seen & kind) != 0) {
      rejectHuntValue("--hunt-budget", kBudgetShape.data(), text);
    }
    seen |= kind;
    const std::string body(clause.substr(0, clause.size() - 1));
    switch (letter) {
      case 's':
      case 'p': {
        std::uint64_t amount = 0;
        if (detail::opensWithBlank(body.c_str())) {
          // A blank stands where the digits belong, which is the shape refusal:
          // the value names no count of any unit (contracts/budget-grammar.md).
          rejectHuntValue("--hunt-budget", kBudgetShape.data(), text);
        }
        if (!scanHuntNumber(body.c_str(), amount) || amount == 0) {
          // The refusal quotes the whole value the option carried, suffix and
          // all, so the line names what the caller typed (FR-081). A count that
          // is no number and a count of zero share the standing sentence, as
          // they did before the clause loop (R-01).
          rejectHuntValue("--hunt-budget", "expected a count above 0", text);
        }
        budget.kind = letter == 's' ? hunt::HuntBudget::Kind::SECONDS
                                    : hunt::HuntBudget::Kind::PASSES;
        budget.amount = amount;
        twoStage = true;
      } break;
      case 'g': {
        // The cap counts events, so it names its range the way the hunt table
        // option names its own: whole digits, 1 to 10000 (FR-003).
        std::uint64_t count = 0;
        if (!scanHuntNumber(body.c_str(), count) || count < 1 || count > 10'000)
        {
          rejectHuntValue("--hunt-budget", "expected 1 to 10000", text);
        }
        cap = static_cast<std::size_t>(count);
      } break;
      case 'e': {
        // The shared seconds rule: finite, above zero, a fraction admitted,
        // nothing below one nanosecond and nothing above the bound the run
        // control carries (FR-004). The shape test keeps the decimal spelling
        // the clause grammar states, so the reader's C number readings stand
        // outside the clause (T050).
        const auto seconds = detail::parseSeconds(
            body.c_str(), "--hunt-budget", detail::kZeroRefused);
        if (!isDecimalSeconds(body) || !seconds.has_value()) {
          // The refusal quotes the whole value the option carried, so the line
          // takes the words of the shared rule and prints them beside that
          // value (contracts/budget-grammar.md).
          const std::string expected =
              detail::secondsExpectedWords(detail::kZeroRefused);
          rejectHuntValue("--hunt-budget", expected.c_str(), text);
        }
        sample = static_cast<std::uint64_t>(*seconds);
      } break;
      default:
        rejectHuntValue("--hunt-budget", kBudgetShape.data(), text);
    }
    if (comma == std::string_view::npos) {
      break;
    }
    at = comma + 1;
  }
  auto& settled = huntValues(options);
  settled.budget = twoStage ? std::optional<hunt::HuntBudget> {budget}
                            : std::optional<hunt::HuntBudget> {};
  settled.sampleBudgetNs = sample;
  settled.multiplexCap = cap;
}

// \pre `text` is the value getopt_long read for a required-argument row.
// \post the seed is the whole number named, the shuffle seed of FR-014; zero is
//       the run with no seed, so the catalog order stands.
auto parseHuntSeed(const char* text, RunOptions& options) -> void
{
  SG_ASSERT(text != nullptr,
            "the option row is a required-argument row, so getopt_long answers "
            "a value for it (FR-082)");
  std::uint64_t seed = 0;
  if (!scanHuntNumber(text, seed)) {
    rejectHuntValue("--hunt-seed", "expected a whole number", text);
  }
  huntValues(options).seed = seed;
}

// \pre `text` is the value getopt_long read for a required-argument row.
// \post the settled path names the file the run writes its promoted set to; an
//       empty value leaves with the one empty-path line of the path rows
//       (FR-012). The guard that asks for a hunt beside the path stands in
//       `validateHuntCombination`, where the whole line has settled.
auto parseHuntEmitSet(const char* text, RunOptions& options) -> void
{
  SG_ASSERT(text != nullptr,
            "the option row is a required-argument row, so getopt_long answers "
            "a value for it (FR-010)");
  if (text[0] == '\0') {
    rejectHuntSelection(emptyPathFault("--hunt-emit-set"));
  }
  options.huntEmitSet = text;
}

// \pre `text` is the value getopt_long read for a required-argument row.
// \post the settled path names the file the run writes the promoted events'
//       comparison rules to; an empty value leaves with the one empty-path
//       line of the path rows (FR-022). The guards that ask for a hunt beside
//       the path and for a file the system opens stand in
//       `validateHuntCombination`, where the whole line has settled.
auto parseHuntEmitRules(const char* text, RunOptions& options) -> void
{
  SG_ASSERT(text != nullptr,
            "the option row is a required-argument row, so getopt_long answers "
            "a value for it (FR-082)");
  if (text[0] == '\0') {
    rejectHuntSelection(emptyPathFault("--hunt-emit-rules"));
  }
  huntValues(options).emitRules = text;
}

// \pre `text` is the value getopt_long read for a required-argument row.
// \post the settled path names the document the delta rank reads its baseline
//       rates from; an empty value leaves with the one empty-path line of the
//       path rows (FR-023). The guards that ask for a hunt beside the path and
//       for a document the loader takes stand in `validateHuntCombination`,
//       where the whole line has settled.
auto parseHuntBaseline(const char* text, RunOptions& options) -> void
{
  SG_ASSERT(text != nullptr,
            "the option row is a required-argument row, so getopt_long answers "
            "a value for it (FR-082)");
  if (text[0] == '\0') {
    rejectHuntSelection(emptyPathFault("--hunt-baseline"));
  }
  huntValues(options).baseline = text;
}

// \pre the clock and PMU providers stand registered, so the catalog answers
//      (FR-006); the option scan has ended, so `options` carries the hunt
//      values the scan read, whether `--hunt` named them or parked them.
// \post the selection holds the addresses the text selects: `all` keeps its
//       word and selects the events of every core device (D-1, FR-004), and
//       every other form holds the joined address lines it resolved to, so the
//       run counts the addresses the option settled on and a list that carries
//       a glob character reaches the plan resolved (FR-006).
// \post an unknown address, a pattern that matches nothing, an empty address
//       line, a raw event code, an unopenable `@path` and a `@path` that holds
//       no address each leave with one line naming the value and status 2
//       (FR-081); the shape rules run first, so the same text gives the same
//       refusal on a host that counts nothing (D-11).
// \post a selection reaching a device that publishes a programmable-counter
//       limit below three leaves with one line naming the device and its
//       count before any planning states a pass (spec PC-2, FR-081).
// \post a norm the caller stated whose anchor the host counts nowhere leaves
//       with the one line naming that norm and status 2 (D-13, FR-081); an
//       unset norm keeps the fallback to iterations.
// \post the scan is over, so a parked record of a hunt option with no `--hunt`
//       beside it leaves the field empty: the values parsed, validated and
//       stood, and the run carries no hunt (contracts/hunt-cli.md).
auto expandHuntSelection(RunOptions& options) -> void
{
  if (!huntRequested(options)) {
    options.hunt.reset();
    return;
  }
  std::string& selection = options.hunt->selection;
  const bool whole = selection == detail::kAllSelection;

  std::vector<std::string> elements;
  bool fromFile = false;
  if (!whole) {
    for (const auto& entry : splitElements(selection)) {
      if (!entry.starts_with('@')) {
        elements.push_back(entry);
        continue;
      }
      // One level of `@path`: the lines of a file are addresses; the file
      // itself names none. The shared reader answers the elements and the one
      // refusal line, and this route prefixes the line with its own option
      // word (FR-006, FR-014, R-09).
      fromFile = true;
      const auto lines = readAddressFileLines(entry.substr(1), entry);
      if (!lines.has_value()) {
        rejectHuntSelection("--hunt: " + lines.error());
      }
      for (const auto& address : *lines) {
        elements.push_back(address);
      }
    }
    if (elements.empty()) {
      // The two refusals are chosen here, each on a line of its own. A
      // coverage trace counts a statement at the line where it begins, so an
      // arm folded into the concatenation as its middle operand leaves the
      // trace with no statement to count on either side of the choice
      // (Principle VI). The refusal itself stays one line, as FR-081 demands.
      const char* tail = "' is not a catalog address";
      if (fromFile) {
        tail = "' holds no catalog address";
      }
      rejectHuntSelection("--hunt: '" + selection + tail);
    }
  }

  for (const auto& element : elements) {
    const std::string_view raw = rawCodeOf(element);
    if (!raw.empty()) {
      // The tail of the line stands on a statement line of its own, for the
      // same trace reason as the refusal above.
      std::string message = "--hunt: '" + std::string(raw);
      message += "' is a raw event code; --hunt takes catalog addresses";
      rejectHuntSelection(message);
    }
  }

  const auto machine = counters::System::local().object("machine");
  SG_ASSERT(machine.has_value(),
            "the counters system opens the host provider, and that provider "
            "publishes the machine object (FR-002)");
  std::vector<std::string> catalog;
  collectCatalog(*machine, catalog);
  hunt::EventCatalog events;

  std::vector<std::string> selected;
  if (whole) {
    // FR-004, D-1: the whole selection is answered by the expansion the run
    // itself uses, so the rule of which addresses `all` holds, the two anchor
    // leaves of a device riding its passes as denominators and standing out of
    // the candidate list (I-1), is stated once, in `appendAll` of `hunt.cpp`.
    // The set validated here is the set the plan seats.
    const auto expanded = events.expand(detail::kAllSelection);
    SG_ASSERT(expanded.has_value(),
              "the machine object that walked for the catalog answers the "
              "expansion of the whole selection too, so no host refuses the "
              "word 'all' at this call (FR-004)");
    selected.assign(events.addresses().begin(), events.addresses().end());
  }
  for (const auto& element : elements) {
    if (element.find_first_of("*?") == std::string::npos) {
      const bool listed =
          std::find(catalog.begin(), catalog.end(), element) != catalog.end();
      if (!listed) {
        rejectHuntSelection("--hunt: '" + element
                            + "' is not a catalog address");
      }
      selected.push_back(element);
      continue;
    }
    const std::size_t first = selected.size();
    for (const auto& address : catalog) {
      if (detail::globMatches(element, address)) {
        selected.push_back(address);
      }
    }
    if (selected.size() == first) {
      rejectHuntSelection("--hunt: pattern '" + element
                          + "' matches no catalog address");
    }
  }

  // D-13, FR-081: the caller stated which denominator the rates divide by, so
  // a host that counts no anchor of that name is a usage error. The unset norm
  // keeps the fallback to iterations, which is why the check reads whether the
  // option stood.
  if (options.hunt->norm.has_value()
      && *options.hunt->norm != hunt::HuntNorm::ITER)
  {
    const bool cycle = *options.hunt->norm == hunt::HuntNorm::CYCLE;
    const std::string word = cycle ? "cycle" : "instr";
    if (!holdsCountableAnchor(*machine,
                              cycle ? detail::kCyclesLeaf : detail::kInstrLeaf))
    {
      rejectHuntSelection("--hunt-norm: '" + word
                          + "' has no countable anchor on this host");
    }
  }

  // specs/029 spec PC-2, FR-011, FR-081 (T164): `detail::admitsHuntEvent` is
  // the device-limit rule and `detail::deviceLimitCause` the cause it states,
  // so the option route refuses the device below three slots with the same
  // bytes the plan route answers, before any planning states a pass. A device
  // that publishes no limit stands open (D-3). An anchor leaf needs no slot of
  // its own, so only event addresses of the selection raise the refusal.
  for (const auto& address : selected) {
    const std::string_view leaf = detail::leafPartOf(address);
    if (leaf == detail::kCyclesLeaf || leaf == detail::kInstrLeaf) {
      continue;
    }
    const std::string device = std::string(detail::devicePartOf(address));
    const auto object = counters::System::local().object(device);
    // A selected address is catalog-listed, and its device part is a path
    // `collectCatalog` walked into that catalog, so the object stands on every
    // entry of this loop; the walk's own postcondition carries that fact
    // (spec PC-1).
    SG_ASSERT(object.has_value(),
              "every device part of a catalog-listed address names an "
              "object the catalog walk just enumerated (spec PC-1)");
    if (!object->programmableCounters().has_value()) {
      continue;
    }
    const std::size_t slots = *object->programmableCounters();
    if (!detail::admitsHuntEvent(slots)) {
      rejectHuntSelection("hunt: " + detail::deviceLimitCause(device, slots));
    }
  }

  // Every form but `all` hands the run the addresses it resolved to, the
  // spelling `EventCatalog::expand` seats without a second reading of the
  // glob characters (FR-006). `all` keeps its word, because that word is the
  // one expansion the run reads for it (FR-004).
  if (!whole) {
    selection = joinAddresses(dedupAddresses(std::move(selected)));
  }
}

// \pre the option scan has ended, so `options` holds every value it read;
//      a record the scan parked is not a hunt. `groupLayout` stands as the
//      winning source of the `--group-layout` row spelled it, empty when no
//      source named the row.
// \post a hunt together with a counter list, a metric list, a compare
//       baseline, a placement preset or a settled group layout leaves with
//       one line naming both options and status 2, the same refusal for
//       either order on the command line (D-16, FR-081; specs/039 FR-019,
//       D-2); a settled emit path beside no hunt, `--hunt=none` among the
//       ways to carry none, leaves with one line naming the value and
//       status 2 (specs/033 FR-012, R-08); a settled rules path or
//       baseline path beside no hunt, a delta rank beside no baseline, a
//       rules file the system refuses to open and a baseline document the
//       loader refuses, each leave with one line naming the value and
//       status 2 (specs/040 FR-005, FR-011, FR-022, FR-023, B-8); a run
//       that carries no hunt, or a hunt beside `--list`, `--catalog` or
//       `--topology`, is not a refusal.
auto validateHuntCombination(const RunOptions& options,
                             const std::string& groupLayout) -> void
{
  // specs/033-hunter-budgets FR-012, R-08: the promoted set is a hunt's output,
  // so a path with no hunt to fill it is read ahead of the unset-hunt return
  // below: `--hunt=none` and no `--hunt` at all both leave nothing to emit.
  if (!options.huntEmitSet.empty() && !huntRequested(options)) {
    rejectHuntSelection("--hunt-emit-set: '" + options.huntEmitSet
                        + "' needs --hunt");
  }
  // specs/040 FR-022, FR-023: the rules file and the baseline document are a
  // hunt's output and input, so each path with no hunt beside it refuses the
  // same way the promoted-set path above does.
  const std::string baselinePath =
      options.hunt.has_value() ? options.hunt->baseline : std::string {};
  const std::string rulesPath =
      options.hunt.has_value() ? options.hunt->emitRules : std::string {};
  if (!rulesPath.empty() && !huntRequested(options)) {
    rejectHuntSelection("--hunt-emit-rules: '" + rulesPath + "' needs --hunt");
  }
  if (!baselinePath.empty() && !huntRequested(options)) {
    rejectHuntSelection("--hunt-baseline: '" + baselinePath + "' needs --hunt");
  }
  if (!huntRequested(options)) {
    return;
  }
  if (!options.counterAddresses.empty()) {
    rejectHuntCombination("--counter");
  }
  if (!options.metricNames.empty()) {
    rejectHuntCombination("--metric");
  }
  if (!options.compareBaseline.empty()) {
    rejectHuntCombination("--compare");
  }
  // The placement preset plans the cpus of the run's threaded contexts and a
  // hunt plans its counter passes, so one run carries one plan (specs/025
  // FR-015, D-16). The option is the whole conflict, so any preset value
  // reaches this refusal and the value stays unread here.
  if (options.placement.has_value()) {
    rejectHuntCombination("--placement");
  }

  // FR-011, the clarify Q2 ruling: the delta rank reads a baseline, so a run
  // that names it beside none refuses one line at status 2, before the
  // entries run (B-8).
  SG_REQUIRE(options.hunt.has_value(),
             "the unset-hunt return above answers every hunt-less option "
             "set, so the rank check reads a settled hunt");
  const bool deltaRank = options.hunt->rank == hunt::HuntRank::DELTA;
  if (deltaRank && baselinePath.empty()) {
    rejectHuntSelection("--hunt-rank: 'delta' needs --hunt-baseline");
  }

  // FR-022, B-8: the rules path refuses at settlement when the file system
  // answers the open with a failure, so a run that cannot write its rules
  // stops before the entries run. The probe opens for appending, so a file
  // that stands keeps its bytes.
  if (!rulesPath.empty()) {
    std::ofstream probe(rulesPath, std::ios::app);
    if (!probe.is_open()) {
      rejectHuntSelection("--hunt-emit-rules: '" + rulesPath
                          + "' cannot be opened");
    }
  }

  // FR-005, R-1: the baseline reaches the run through `loadReport` alone, so
  // a document the loader refuses stands named with the loader's own fault at
  // settlement, before the entries run (B-8, D-6).
  if (!baselinePath.empty()) {
    const auto loaded = loadReport(baselinePath);
    if (!loaded.has_value()) {
      rejectHuntSelection("--hunt-baseline: '" + baselinePath
                          + "' was rejected: " + loaded.error());
    }
  }

  // The layout file names the run's partition and a hunt plans the counter
  // passes of that run, so one run carries one of the two (specs/039 FR-019,
  // D-2). The settled path is the whole conflict, and the file stays unread
  // here: the fit walk reads it.
  if (!groupLayout.empty()) {
    rejectHuntCombination("--group-layout");
  }
}

// specs/033-hunter-budgets FR-011, FR-012, R-07, contracts/counter-file.md:
// the file the run hands the next run. Each hunted entry contributes
// `toCounterSet(topN)` and the union keeps the first occurrence of an address,
// so the lines stand in entry order and, within an entry, in rank order. The
// failure lines are the report file's own: the path, then the reason the file
// system gave, on one line each.
auto writeHuntEmitSet(const std::string& path,
                      std::span<const BenchmarkResult> results,
                      std::size_t topN) -> bool
{
  std::vector<std::string> addresses;
  for (const auto& result : results) {
    if (!result.hunt.has_value()) {
      continue;
    }
    for (const auto& address : result.hunt->toCounterSet(topN)) {
      if (std::ranges::find(addresses, address) == addresses.end()) {
        addresses.push_back(address);
      }
    }
  }

  std::ofstream stream(path, std::ios::binary | std::ios::trunc);
  if (!stream.is_open()) {
    std::fprintf(stderr,
                 "error: the counter set file '%s' cannot be opened: %s\n",
                 quotedText(path).c_str(),
                 quotedText(std::strerror(errno)).c_str());
    return false;
  }
  for (const auto& address : addresses) {
    stream << address << '\n';
  }
  stream.close();
  if (stream.fail()) {
    std::fprintf(stderr,
                 "error: the counter set file '%s' could not be written: %s\n",
                 quotedText(path).c_str(),
                 quotedText(std::strerror(errno)).c_str());
    return false;
  }
  return true;
}

// specs/040-hunter-sensitivity FR-021, FR-022, B-10, D-7: the promoted
// events' comparison rules, the file the `--compare-rules` option reads. The
// union shape of the counter set writer above: each hunted entry contributes
// `toCompareRules(topN, noisePercent)` and the union keeps the first line of
// an address, so the lines stand in entry order and, within an entry, in
// rank order, with the figures the batch stamping pass left.
auto writeHuntEmitRules(const std::string& path,
                        std::span<const BenchmarkResult> results,
                        std::size_t topN,
                        double noisePercent) -> bool
{
  std::vector<std::string> lines;
  for (const auto& result : results) {
    if (!result.hunt.has_value()) {
      continue;
    }
    for (const auto& line : result.hunt->toCompareRules(topN, noisePercent)) {
      if (std::ranges::find(lines, line) == lines.end()) {
        lines.push_back(line);
      }
    }
  }

  std::ofstream stream(path, std::ios::binary | std::ios::trunc);
  if (!stream.is_open()) {
    std::fprintf(stderr,
                 "error: the rules file '%s' cannot be opened: %s\n",
                 quotedText(path).c_str(),
                 quotedText(std::strerror(errno)).c_str());
    return false;
  }
  for (const auto& line : lines) {
    stream << line << '\n';
  }
  stream.close();
  if (stream.fail()) {
    std::fprintf(stderr,
                 "error: the rules file '%s' could not be written: %s\n",
                 quotedText(path).c_str(),
                 quotedText(std::strerror(errno)).c_str());
    return false;
  }
  return true;
}

}  // namespace sg::detail
