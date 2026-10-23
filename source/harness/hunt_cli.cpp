// specs/029 FR-080 to FR-082, PD-16, PD-17: the eight hunt options, their
// value ranges, their refusals, the `@file` reader, the glob matcher and the
// conflict check. The scan of `cli.cpp` holds the table rows, the eight named
// constants and one call per row; every parse body lives here (D-22).

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "speedgun-ng/hunt.hpp"

#include "detail/internal.hpp"
#include "speedgun-ng/counters_system.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg::detail
{

namespace
{

// The two anchor leaves of a device: the denominators every hunt pass rides
// beside its event (D-13, I-1). `hunt.cpp` names the same pair beside the walk
// that seats them, and the rule of which addresses `all` selects stands there.
constexpr std::string_view kCyclesLeaf {"cpu-cycles"};
constexpr std::string_view kInstrLeaf {"instructions"};

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

// D-3, FR-032: `sensitivity`, `delta` and `score` are the rank keys N14b
// brings, so the refusal for one of those three words names the work that owns
// it beside the words this option takes. Any other word is a malformed value
// and keeps the refusal above.
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
      || !(value > 0.0))
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

// The lines a `@path` carries, read where the providers stand registered. The
// file the host refuses is a refusal naming the path the option spelled
// (FR-006, FR-081).
auto readSelectionFile(const std::string& path) -> std::string
{
  std::ifstream file(path);
  if (!file) {
    rejectHuntSelection("--hunt: '@" + path + "' cannot be opened");
  }
  std::string text;
  std::string line;
  while (std::getline(file, line)) {
    text += line;
    text += "\n";
  }
  return text;
}

// The whitespace that stands around an address, an element and the `@path` of a
// file, trimmed by hand the way `rejectPercentileList`'s parser reads its
// spaces, the trim landing before the number is read.
[[nodiscard]] auto trimText(const std::string_view text) -> std::string_view
{
  std::size_t first = 0;
  std::size_t last = text.size();
  const auto blank = [](const char c)
  { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
  while (first < last && blank(text[first])) {
    ++first;
  }
  while (last > first && blank(text[last - 1])) {
    --last;
  }
  return text.substr(first, last - first);
}

// The elements of a selection text: one line at a newline, each line cut at the
// first `#` the way FR-006 reads a `@file`, then one element at a comma. A line
// a comment or whitespace leaves empty is ignored (FR-006); a comma that leaves
// nothing beside it is the empty address line, which stands in the list as an
// empty element for the resolution below to refuse.
[[nodiscard]] auto splitElements(const std::string& text)
    -> std::vector<std::string>
{
  std::vector<std::string> elements;
  const std::string_view whole {text};
  std::size_t at = 0;
  while (at < whole.size()) {
    const std::size_t found = whole.find('\n', at);
    const std::size_t lineEnd =
        found == std::string_view::npos ? whole.size() : found;
    std::string_view line = whole.substr(at, lineEnd - at);
    at = lineEnd + 1;
    const std::size_t comment = line.find('#');
    if (comment != std::string_view::npos) {
      line = line.substr(0, comment);
    }
    std::size_t piece = 0;
    while (piece <= line.size()) {
      const std::size_t comma = line.find(',', piece);
      const std::size_t pieceEnd =
          comma == std::string_view::npos ? line.size() : comma;
      const std::string_view element =
          trimText(line.substr(piece, pieceEnd - piece));
      if (!element.empty() || comma != std::string_view::npos) {
        elements.push_back(std::string(element));
      }
      if (comma == std::string_view::npos) {
        break;
      }
      piece = pieceEnd + 1;
    }
  }
  return elements;
}

// The raw event code of D-11: the perf spelling, a lower-case `r` and one or
// more hex digits. The leaf of an entry carries it, so the shape refuses inside
// an address, inside a list, inside a pattern (whose trailing glob characters
// stand outside the shape) and on a line of a `@file` alike. The answer is the
// code and nothing when the entry carries none, and the check runs before the
// catalog is consulted, so `r4a03` is refused on a host whose kernel would
// encode it (spec PC-6).
[[nodiscard]] auto rawCodeOf(const std::string_view entry) -> std::string_view
{
  const std::size_t slash = entry.find_last_of('/');
  std::string_view leaf =
      slash == std::string_view::npos ? entry : entry.substr(slash + 1);
  while (!leaf.empty() && (leaf.back() == '*' || leaf.back() == '?')) {
    leaf.remove_suffix(1);
  }
  if (leaf.size() < 2 || leaf.front() != 'r') {
    return {};
  }
  for (std::size_t at = 1; at < leaf.size(); ++at) {
    const char digit = leaf[at];
    const bool hex = (digit >= '0' && digit <= '9')
        || (digit >= 'a' && digit <= 'f') || (digit >= 'A' && digit <= 'F');
    if (!hex) {
      return {};
    }
  }
  return leaf;
}

// The case-sensitive match of one catalog address against one pattern: `*`
// matches any run of characters including none, `?` matches one character, and
// every other character matches itself. The last star carries the backtracking,
// so one pass over the address decides it.
[[nodiscard]] auto matchesGlob(const std::string_view address,
                               const std::string_view pattern) -> bool
{
  std::size_t at = 0;
  std::size_t here = 0;
  std::size_t star = std::string_view::npos;
  std::size_t resume = 0;
  while (at < address.size()) {
    if (here < pattern.size()
        && (pattern[here] == '?' || pattern[here] == address[at]))
    {
      ++at;
      ++here;
    } else if (here < pattern.size() && pattern[here] == '*') {
      star = here;
      ++here;
      resume = at;
    } else if (star != std::string_view::npos) {
      here = star + 1;
      at = ++resume;
    } else {
      return false;
    }
  }
  while (here < pattern.size() && pattern[here] == '*') {
    ++here;
  }
  return here == pattern.size();
}

// The catalog addresses of the host, the walk `catalog.cpp:83-92` performs: the
// address has the one spelling `Object::path() + "/" + entry.name`, in the
// order the walk reaches the devices and the order each device reports its
// counters (spec PC-1).
auto collectCatalog(const counters::Object& object,
                    std::vector<std::string>& addresses) -> void
{
  const std::string prefix = std::string(object.path()) + "/";
  for (const auto& entry : object.counters()) {
    addresses.push_back(prefix + std::string(entry.name));
  }
  for (const auto* child : object.children()) {
    collectCatalog(*child, addresses);
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
// \post the rank is the word named; `sensitivity`, `delta` and `score` are the
//       keys N14b brings and leave with the refusal naming N14b, and a word
//       outside the five leaves with the refusal naming the accepted words
//       (D-3, FR-032, FR-081).
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
  } else if (word == "sensitivity" || word == "delta" || word == "score") {
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

// \pre `text` is the value getopt_long read for a required-argument row.
// \post the budget is the count named in the unit the suffix spelled, above
//       zero; a text with no suffix names no unit and is refused (FR-012).
auto parseHuntBudget(const char* text, RunOptions& options) -> void
{
  SG_ASSERT(text != nullptr,
            "the option row is a required-argument row, so getopt_long answers "
            "a value for it (FR-082)");
  const std::string_view value {text};
  hunt::HuntBudget budget;
  if (value.ends_with("s")) {
    budget.kind = hunt::HuntBudget::Kind::SECONDS;
  } else if (value.ends_with("p")) {
    budget.kind = hunt::HuntBudget::Kind::PASSES;
  } else {
    rejectHuntValue("--hunt-budget", "expected <n>s or <n>p", text);
  }
  const std::string count(value.substr(0, value.size() - 1));
  std::uint64_t amount = 0;
  if (!scanHuntNumber(count.c_str(), amount) || amount == 0) {
    // The refusal quotes the whole value the option carried, suffix and
    // all, so the line names what the caller typed (FR-081).
    rejectHuntValue("--hunt-budget", "expected a count above 0", text);
  }
  budget.amount = amount;
  huntValues(options).budget = budget;
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
  const bool whole = selection == "all";

  std::vector<std::string> elements;
  bool fromFile = false;
  if (!whole) {
    for (const auto& entry : splitElements(selection)) {
      if (!entry.starts_with('@')) {
        elements.push_back(entry);
        continue;
      }
      // One level of `@path`: the lines of a file are addresses; the file
      // itself names none.
      fromFile = true;
      for (const auto& address :
           splitElements(readSelectionFile(entry.substr(1))))
      {
        elements.push_back(address);
      }
    }
    if (elements.empty()) {
      rejectHuntSelection("--hunt: '" + selection
                          + (fromFile ? "' holds no catalog address"
                                      : "' is not a catalog address"));
    }
  }

  for (const auto& element : elements) {
    const std::string_view raw = rawCodeOf(element);
    if (!raw.empty()) {
      rejectHuntSelection("--hunt: '" + std::string(raw)
                          + "' is a raw event code; --hunt takes catalog "
                            "addresses");
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
    const auto expanded = events.expand("all");
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
      if (matchesGlob(address, element)) {
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
    if (!holdsCountableAnchor(*machine, cycle ? kCyclesLeaf : kInstrLeaf)) {
      rejectHuntSelection("--hunt-norm: '" + word
                          + "' has no countable anchor on this host");
    }
  }

  // specs/029 spec PC-2, FR-081: a device that publishes fewer than three
  // programmable counters cannot carry a hunt pass beside its two anchors,
  // so the run is refused before any planning states a pass. A device that
  // publishes no limit stands open (D-3). An anchor leaf needs no slot of
  // its own, so only event addresses of the selection raise the refusal.
  for (const auto& address : selected) {
    const std::size_t slash = address.find('/');
    const std::string_view leaf = std::string_view(address).substr(slash + 1);
    if (leaf == kCyclesLeaf || leaf == kInstrLeaf) {
      continue;
    }
    const std::string device = address.substr(0, slash);
    const auto object = counters::System::local().object(device);
    if (!object.has_value() || !object->programmableCounters().has_value()) {
      continue;
    }
    const std::size_t slots = *object->programmableCounters();
    if (slots < 3) {
      rejectHuntSelection("hunt: device '" + device + "' publishes "
                          + std::to_string(slots)
                          + " slots and a hunt pass needs 3");
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
//      a record the scan parked is not a hunt.
// \post a hunt together with a counter list, a metric list or a compare
//       baseline leaves with one line naming both options and status 2, the
//       same refusal for either order on the command line (D-16, FR-081); a run
//       that carries no hunt, or a hunt beside `--list`, `--catalog` or
//       `--topology`, is not a refusal.
auto validateHuntCombination(const RunOptions& options) -> void
{
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
}

}  // namespace sg::detail
