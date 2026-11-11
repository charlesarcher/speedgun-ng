// ============================================================================
// The noise tracer value parser and the engine session
// (specs/036-osnoise-timerlat-tracers T007, T011, T012;
// specs/048-hwlat-probe T029, T030, T031).
//
// One translation unit owns the tracer: the value grammar of
// contracts/cli-options.md, the standing cpu-list reader of the jitter
// probe, the refusal shape the option module quotes, and the session
// lifecycle and the figure read over the tracefs files the file-root
// seam resolves (R-01 to R-05, R-07). The detector engine joins the two
// standing engines on that lifecycle: the six saved texts, the seven
// writes in the order FR-019 names, the three figures off the buffer
// text, and the restore that walks the saved set in reverse for all
// three engines (FR-018 to FR-022, FR-027, D-8, D-9, D-15).
// ============================================================================

#include <array>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "detail/noise_tracer.hpp"

#include "../detail/proc_root.hpp"
#include "detail/jitter_probe.hpp"
#include "detail/options.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg::detail
{

// The value grammar of contracts/cli-options.md (FR-002, FR-003, FR-018,
// D-4, R-06): the head names the engine word osnoise, timerlat or hwlat,
// then at most
// one optional cpus key follows, whose items the standing cpu-list reader
// reads whole, and the key stands once in the value. Every rejection
// names the token it read, quoted the way the option module quotes a
// value (FR-003).
auto parseOsNoiseValue(const std::string& value,
                       TracerEngine& engine,
                       std::vector<std::uint32_t>& cpus,
                       std::string& message) -> bool
{
  engine = TracerEngine::NONE;
  cpus.clear();
  const auto headEnd = value.find(',');
  const std::string head =
      headEnd == std::string::npos ? value : value.substr(0, headEnd);
  if (head == "osnoise") {
    engine = TracerEngine::OSNOISE;
  } else if (head == "timerlat") {
    engine = TracerEngine::TIMERLAT;
  } else if (head == "hwlat") {
    engine = TracerEngine::HWLAT;
  } else {
    // An empty word refuses the same way: the value names an engine, so
    // the refusal reads the empty token (FR-002, FR-018, R-06).
    message = "the engine '" + quotedText(head)
        + "' is not osnoise, timerlat or hwlat";
    return false;
  }
  if (headEnd == std::string::npos) {
    return true;
  }

  // The key stands once in a value (FR-002): every item after the head
  // extends the one cpus list, a token naming the key a second time
  // stands the standing repeated-key refusal, and a bare item before
  // the key names no list the value asked for.
  std::set<std::string> seenKeys;
  std::size_t position = headEnd + 1;
  while (position <= value.size()) {
    const auto comma = value.find(',', position);
    const std::string token = value.substr(
        position,
        comma == std::string::npos ? std::string::npos : comma - position);
    const bool last = comma == std::string::npos;
    position = last ? value.size() + 1 : comma + 1;

    const auto equals = token.find('=');
    if (equals != std::string::npos) {
      const std::string key = token.substr(0, equals);
      if (key != "cpus") {
        message =
            "the key '" + quotedText(key) + "' is not one the value names";
        return false;
      }
      if (!seenKeys.insert(key).second) {
        message = "the key '" + quotedText(key) + "' stands twice in the value";
        return false;
      }
      // The key's item stands in the same token, so an empty item
      // reaches the cpu-list reader and its refusal reads the empty
      // token (FR-003).
      const std::string item = token.substr(equals + 1);
      if (!parseProbeCpuToken(item, cpus, message)) {
        return false;
      }
      continue;
    }
    if (seenKeys.empty()) {
      message =
          "the token '" + quotedText(token) + "' stands outside the cpus key";
      return false;
    }
    if (!parseProbeCpuToken(token, cpus, message)) {
      return false;
    }
  }

  // A cpu that stands twice in one list is an invalid value (FR-002,
  // FR-003); the whole value stands named beside the index that repeated,
  // the standing shape (FR-038).
  std::set<std::uint32_t> seenCpus;
  for (const auto cpu : cpus) {
    if (!seenCpus.insert(cpu).second) {
      message = "the cpu '" + std::to_string(cpu)
          + "' stands more than once in '" + quotedText(value) + "'";
      return false;
    }
  }
  return true;
}

namespace
{

// The first line of one file text, trimmed of the blanks the line
// carries at either end (R-04): the form the control files carry.
auto trimmedWordOf(const std::string& text) -> std::string
{
  const auto stop = text.find('\n');
  const auto line = stop == std::string::npos ? text : text.substr(0, stop);
  const auto begin = line.find_first_not_of(" \t\r");
  if (begin == std::string::npos) {
    return {};
  }
  const auto end = line.find_last_not_of(" \t\r");
  return line.substr(begin, end - begin + 1);
}

// One file under the tracing root, read whole (R-01): a file that
// stands missing or unreadable answers nothing, and one the read
// cannot carry to the end answers nothing beside it (FR-016).
auto readTracerFile(const std::string& relative) -> std::optional<std::string>
{
  std::ifstream file(procRootPath(relative));
  if (!file) {
    return std::nullopt;
  }
  std::string text;
  std::string line;
  while (std::getline(file, line)) {
    text += line;
    text += '\n';
  }
  if (!file.eof()) {
    return std::nullopt;
  }
  return text;
}

// One file under the tracing root, written whole (R-01): a write that
// does not stand through answers false (FR-016).
auto writeTracerFile(const std::string& relative,
                     const std::string& text) -> bool
{
  std::ofstream file(procRootPath(relative));
  if (!file) {
    return false;
  }
  file << text;
  file.close();
  return file.good();
}

// The mask text of the tracer cpus at `width` hex digits per word,
// comma-separated, the leftmost word naming the highest cpus, bit i of
// word w naming cpu w * width * 4 + i (R-02, R-05): the width the
// standing text carried, and the word count the cpus need.
auto renderCpuMask(const std::vector<std::uint32_t>& cpus,
                   std::size_t width) -> std::string
{
  const std::size_t bitsPerWord = width * 4;
  std::size_t wordCount = 1;
  for (const auto cpu : cpus) {
    const std::size_t needed = cpu / bitsPerWord + 1;
    if (needed > wordCount) {
      wordCount = needed;
    }
  }
  std::string text;
  for (std::size_t at = 0; at < wordCount; ++at) {
    const std::size_t word = wordCount - 1 - at;
    std::uint64_t bits = 0;
    for (const auto cpu : cpus) {
      if (cpu / bitsPerWord == word) {
        bits |= std::uint64_t {1} << (cpu % bitsPerWord);
      }
    }
    std::ostringstream digits;
    digits << std::hex << std::setw(static_cast<int>(width))
           << std::setfill('0') << bits;
    if (!text.empty()) {
      text += ',';
    }
    text += digits.str();
  }
  return text;
}

// The word width the standing mask text carries: the digits of the
// first comma-separated word (R-05). A text that names no word answers
// nothing.
auto maskWidthOf(const std::string& text) -> std::optional<std::size_t>
{
  const auto stop = text.find(',');
  const auto word = stop == std::string::npos ? text : text.substr(0, stop);
  const auto begin = word.find_first_not_of(" \t\r\n");
  if (begin == std::string::npos) {
    return std::nullopt;
  }
  const auto end = word.find_last_not_of(" \t\r\n");
  return word.substr(begin, end - begin + 1).size();
}

// The word the mode file's write handler compares (P-16): the kernel
// prints the mode list with the standing mode inside brackets, the form
// `none [round-robin] per-cpu`, so the bracketed word stands the value to
// save and to write back. A text that carries no brackets stands its one
// word (FR-019).
auto modeWordOf(const std::string& text) -> std::string
{
  const auto line = trimmedWordOf(text);
  const auto open = line.find('[');
  const auto close = line.find(']');
  if (open == std::string::npos || close == std::string::npos || close < open) {
    return line;
  }
  return line.substr(open + 1, close - open - 1);
}

// The mode the detector run writes: the per-cpu mode samples every tracer
// cpu, the mode the lane rules (D-9, P-14).
constexpr const char* kHwlatModeWord = "per-cpu";

// One write of the setup: the file with the value the setup saves ahead of
// it, and the text the setup writes (FR-019, D-8, D-9).
struct SetupStep
{
  TracerSetting saved;
  std::string text;
};

// The saved set written back in the reverse of the write order (FR-021,
// P-8, D-8): each file whose write failed joins `failed` in the order the
// writes landed. The setup's undo path passes only the files it wrote and
// reads no answer: the not-available shape discloses the tracer's absence
// whole, so a failed undo adds no line (FR-021, D-15, P-9).
auto writeSavedBack(const std::vector<TracerSetting>& saved,
                    std::vector<TracerSetting>& failed) -> void
{
  for (auto at = saved.size(); at > 0; --at) {
    const auto& pair = saved.at(at - 1);
    if (!writeTracerFile(pair.path, pair.value + "\n")) {
      failed.push_back(pair);
    }
  }
}

// One unsigned figure off one token; a token that parses as no number
// answers nothing (R-07).
auto countOfToken(std::string_view token) -> std::optional<std::uint64_t>
{
  std::uint64_t figure = 0;
  const auto* begin = token.data();
  const auto* end = begin + token.size();
  const auto parsed = std::from_chars(begin, end, figure);
  if (parsed.ec != std::errc {} || parsed.ptr != end) {
    return std::nullopt;
  }
  return figure;
}

// The blank-separated tokens of one buffer line (FR-027): a blank line
// answers no token, and no token stands empty.
auto tokensOf(std::string_view line) -> std::vector<std::string_view>
{
  std::vector<std::string_view> tokens;
  for (std::size_t at = 0; at < line.size();) {
    const auto begin = line.find_first_not_of(" \t\r", at);
    if (begin == std::string::npos) {
      break;
    }
    const auto end = line.find_first_of(" \t\r", begin);
    tokens.push_back(end == std::string::npos
                         ? line.substr(begin)
                         : line.substr(begin, end - begin));
    at = end == std::string::npos ? line.size() : end + 1;
  }
  return tokens;
}

// Whether the token ahead of position `first` ends with the timestamp colon
// both engines print ahead of their figures (FR-027). The caller stands a
// token ahead of `first`.
auto standsAnchored(const std::vector<std::string_view>& tokens,
                    std::size_t first) -> bool
{
  return tokens.at(first - 1).back() == ':';
}

// Whether one token answers digits, one `.` and digits (FR-027): the shape
// of the percent figure of an osnoise period line.
auto decimalOf(std::string_view token) -> bool
{
  const auto dot = token.find('.');
  if (dot == std::string_view::npos) {
    return false;
  }
  return countOfToken(token.substr(0, dot)).has_value()
      && countOfToken(token.substr(dot + 1)).has_value();
}

// The tail widths of the two line shapes: the nine figures of an osnoise
// period line, and the six tokens of a timerlat activation line (FR-027).
constexpr std::size_t kPeriodColumns = 9;
constexpr std::size_t kActivationColumns = 6;

// The counter columns stand the last five of the nine period figures,
// positional in the order the line prints them (FR-027).
constexpr std::size_t kCounterColumns = 5;
constexpr std::size_t kShapeColumns = kPeriodColumns - kCounterColumns;

// Whether the tokens from `first` answer the four shape figures of a
// period line ahead of the counter columns: digits, digits,
// digits-dot-digits and digits, behind a token that ends with the
// timestamp colon (FR-027).
auto periodShaped(const std::vector<std::string_view>& tokens,
                  std::size_t first) -> bool
{
  if (!standsAnchored(tokens, first)) {
    return false;
  }
  if (!countOfToken(tokens.at(first)).has_value()) {
    return false;
  }
  if (!countOfToken(tokens.at(first + 1)).has_value()) {
    return false;
  }
  if (!decimalOf(tokens.at(first + 2))) {
    return false;
  }
  return countOfToken(tokens.at(first + 3)).has_value();
}

// The five counter figures of one period line's tail, in print order:
// hardware, nmi, irq, softirq and thread. A tail token that answers no
// figure answers nothing for the whole line (R-07).
auto countersOfTail(const std::vector<std::string_view>& tokens,
                    std::size_t first)
    -> std::optional<std::array<std::uint64_t, kCounterColumns>>
{
  std::array<std::uint64_t, kCounterColumns> counters {};
  for (std::size_t column = kShapeColumns; column < kPeriodColumns; ++column) {
    const auto figure = countOfToken(tokens.at(first + column));
    if (!figure.has_value()) {
      return std::nullopt;
    }
    counters.at(column - kShapeColumns) = figure.value();
  }
  return counters;
}

// The context word of one timerlat activation line's tail: the six tail
// tokens answer `#` and digits, `context`, `irq` or `thread`,
// `timer_latency`, digits and `ns` with nothing following (FR-027). A tail
// beside that shape answers nothing (R-07).
auto activationContextOf(const std::vector<std::string_view>& tokens,
                         std::size_t first) -> std::optional<std::string_view>
{
  const auto& sequence = tokens.at(first);
  if (sequence.front() != '#') {
    return std::nullopt;
  }
  if (!countOfToken(sequence.substr(1)).has_value()) {
    return std::nullopt;
  }
  if (tokens.at(first + 1) != "context") {
    return std::nullopt;
  }
  const auto& context = tokens.at(first + 2);
  if (context != "irq" && context != "thread") {
    return std::nullopt;
  }
  if (tokens.at(first + 3) != "timer_latency") {
    return std::nullopt;
  }
  if (!countOfToken(tokens.at(first + 4)).has_value()) {
    return std::nullopt;
  }
  if (tokens.back() != "ns") {
    return std::nullopt;
  }
  return context;
}

// The figures the period lines of one cpu's buffer carry (FR-027, D-9,
// D-11): a line stands one period line when it carries nine tokens behind a
// token that ends with the timestamp colon, those nine answering digits,
// digits, digits-dot-digits and six groups of digits, and a line whose first
// character stands `#` stands a header line whatever its tail. The last five
// of the nine stand the interference counters, positional in the order the
// line prints them, and they sum into the hardware, nmi, irq, softirq and
// thread figures, so a counter column that reads 0 on every parsed line
// stands a filled 0. The first four stand the shape and answer no figure of
// their own. A line beside that shape stands skipped, the standing rule that
// no figure stands a guess (R-07). A buffer that carries no parsed line,
// stands missing, or stands unreadable answers all five figures absent.
auto osnoiseFiguresOfCpu(std::uint32_t cpu,
                         const std::optional<std::string>& text)
    -> PrecheckTracerCpuRecord
{
  PrecheckTracerCpuRecord record;
  record.cpu = cpu;
  if (!text.has_value()) {
    return record;
  }
  std::uint64_t hardwareSum = 0;
  std::uint64_t nmiSum = 0;
  std::uint64_t irqSum = 0;
  std::uint64_t softirqSum = 0;
  std::uint64_t threadSum = 0;
  std::uint64_t periods = 0;
  std::istringstream stream(*text);
  std::string line;
  while (std::getline(stream, line)) {
    if (!line.empty() && line.front() == '#') {
      continue;
    }
    const auto tokens = tokensOf(line);
    if (tokens.size() < kPeriodColumns + 1) {
      continue;
    }
    const std::size_t first = tokens.size() - kPeriodColumns;
    if (!periodShaped(tokens, first)) {
      continue;
    }
    const auto counters = countersOfTail(tokens, first);
    if (!counters.has_value()) {
      continue;
    }
    hardwareSum += counters.value().at(0);
    nmiSum += counters.value().at(1);
    irqSum += counters.value().at(2);
    softirqSum += counters.value().at(3);
    threadSum += counters.value().at(4);
    ++periods;
  }
  const bool any = periods > 0;
  if (any) {
    record.hardware = hardwareSum;
    record.nmi = nmiSum;
    record.irq = irqSum;
    record.softirq = softirqSum;
    record.thread = threadSum;
  }
  SG_ENSURE(any == (record.hardware.has_value() && record.nmi.has_value()
                    && record.irq.has_value() && record.softirq.has_value()
                    && record.thread.has_value()),
            "osnoiseFiguresOfCpu answers five figures beside a period line "
            "and none beside no line");
  return record;
}

// The figures the activation lines of one cpu's buffer carry (FR-027, D-10):
// a line stands one activation line when it carries six tokens behind a
// token that ends with the timestamp colon, those six answering `#` and
// digits, `context`, a context word, `timer_latency`, digits and `ns` with
// nothing following `ns`, and a line whose first character stands `#` stands
// a header line whatever its tail. The `irq` word counts the irq figure and
// the `thread` word counts the thread figure, the two counts standing
// independent: no line looks for its pair. The engine states nothing for the
// other three sources, so those figures stand absent whatever the buffer
// holds (D-10). A line beside that shape stands skipped, the standing rule
// that no figure stands a guess (R-07). A buffer that carries no parsed
// line, stands missing, or stands unreadable answers the two counts absent.
// Whether one timerlat record answers exactly the two counts beside a
// parsed activation line and nothing beside none, the three figures the
// engine states nothing for standing absent (FR-028, D-10). The call site
// stands inside `SG_ENSURE`, so the contracts=ignore build leaves the helper
// unused (specs/052 FR-002).
[[maybe_unused]] auto activationAnswered(const PrecheckTracerCpuRecord& record,
                                         bool any) -> bool
{
  const std::size_t present = static_cast<std::size_t>(record.irq.has_value())
      + static_cast<std::size_t>(record.thread.has_value())
      + static_cast<std::size_t>(record.nmi.has_value())
      + static_cast<std::size_t>(record.softirq.has_value())
      + static_cast<std::size_t>(record.hardware.has_value());
  return present == (any ? 2U : 0U);
}

auto timerlatFiguresOfCpu(std::uint32_t cpu,
                          const std::optional<std::string>& text)
    -> PrecheckTracerCpuRecord
{
  PrecheckTracerCpuRecord record;
  record.cpu = cpu;
  if (!text.has_value()) {
    return record;
  }
  std::uint64_t irqs = 0;
  std::uint64_t threads = 0;
  std::uint64_t counted = 0;
  std::istringstream stream(*text);
  std::string line;
  while (std::getline(stream, line)) {
    if (!line.empty() && line.front() == '#') {
      continue;
    }
    const auto tokens = tokensOf(line);
    if (tokens.size() < kActivationColumns + 1) {
      continue;
    }
    const std::size_t first = tokens.size() - kActivationColumns;
    if (!standsAnchored(tokens, first)) {
      continue;
    }
    const auto context = activationContextOf(tokens, first);
    if (!context.has_value()) {
      continue;
    }
    if (context.value() == "irq") {
      ++irqs;
    } else {
      ++threads;
    }
    ++counted;
  }
  const bool any = counted > 0;
  if (any) {
    record.irq = irqs;
    record.thread = threads;
  }
  SG_ENSURE(activationAnswered(record, any),
            "timerlatFiguresOfCpu answers two figures beside an activation "
            "line and keeps the three the engine states nothing for absent");
  return record;
}

// The marker `kernel/trace/trace_output.c:1531` prints ahead of the two
// microsecond figures of one detected gap (A-6, P-10).
constexpr std::string_view kGapMarker = "inner/outer(us):";

// The field that names this cpu's count for the gap one line carries.
// `nmi-total` names a different count, so the sum reads this field alone
// (P-10).
constexpr std::string_view kNmiField = "nmi-count:";

// The wider of the two microsecond figures of one line, scaled to no
// unit: the text after the marker stands `<inner>/<outer>` ahead of the
// fields that follow (A-6). A tail that answers no pair answers nothing.
auto widerGapOf(const std::string& tail) -> std::optional<std::uint64_t>
{
  const auto slash = tail.find('/');
  if (slash == std::string::npos) {
    return std::nullopt;
  }
  const auto space = tail.find_first_of(" \t", slash + 1);
  const auto inner = countOfToken(trimmedWordOf(tail.substr(0, slash)));
  const auto outer = countOfToken(trimmedWordOf(tail.substr(
      slash + 1,
      space == std::string::npos ? std::string::npos : space - slash - 1)));
  if (!inner.has_value() || !outer.has_value()) {
    return std::nullopt;
  }
  return inner.value() > outer.value() ? inner : outer;
}

// The `nmi-count` figure of one line, absent where the line carries no
// such field (P-10).
auto nmiOf(const std::string& line) -> std::optional<std::uint64_t>
{
  const auto at = line.find(kNmiField);
  if (at == std::string::npos) {
    return std::nullopt;
  }
  const auto begin = at + kNmiField.size();
  const auto end = line.find_first_of(" \t", begin);
  return countOfToken(line.substr(
      begin, end == std::string::npos ? std::string::npos : end - begin));
}

// The figures the buffer text of one cpu carries (FR-018, FR-027, A-6,
// P-10): a line stands one detected gap when `#` fills column 1 and a
// digit fills column 2, the shape `kernel/trace/trace_output.c:1531`
// prints, so each header legend line stands outside the count. The widest
// line of the cpu stands `maxGapNs` in nanoseconds, the count of lines
// stands `gapCount`, and the sum of the `nmi-count` fields stands
// `nmiCount`, 0 where lines stand and none carries one. A line whose two
// figures answer no pair counts as no gap, the standing rule that no
// figure stands a guess (R-07). A text that counts no gap, stands
// missing, or stands unreadable answers all three figures absent.
auto gapFiguresOfCpu(std::uint32_t cpu, const std::optional<std::string>& text)
    -> PrecheckTracerCpuRecord
{
  PrecheckTracerCpuRecord record;
  record.cpu = cpu;
  if (!text.has_value()) {
    return record;
  }
  std::uint64_t widest = 0;
  std::uint64_t gaps = 0;
  std::uint64_t nmi = 0;
  std::istringstream stream(*text);
  std::string line;
  while (std::getline(stream, line)) {
    if (line.size() < 2 || line.front() != '#'
        || std::isdigit(static_cast<unsigned char>(line.at(1))) == 0)
    {
      continue;
    }
    const auto marker = line.find(kGapMarker);
    if (marker == std::string::npos) {
      continue;
    }
    const auto wider = widerGapOf(line.substr(marker + kGapMarker.size()));
    if (!wider.has_value()) {
      continue;
    }
    ++gaps;
    if (wider.value() > widest) {
      widest = wider.value();
    }
    const auto counted = nmiOf(line);
    if (counted.has_value()) {
      nmi += counted.value();
    }
  }
  if (gaps == 0) {
    return record;
  }
  record.gapCount = gaps;
  record.maxGapNs = widest * 1000ULL;
  record.nmiCount = nmi;
  SG_ENSURE(record.gapCount.has_value() && record.maxGapNs.has_value()
                && record.nmiCount.has_value(),
            "gapFiguresOfCpu answers three figures beside a counted gap");
  return record;
}

}  // namespace

// The word the engine writes into `current_tracer` (R-02): see the
// declaration's contract in detail/noise_tracer.hpp.
auto engineWordOf(TracerEngine engine) -> std::string
{
  SG_REQUIRE(engine != TracerEngine::NONE,
             "engineWordOf reaches an engine that names no tracer");
  if (engine == TracerEngine::OSNOISE) {
    return "osnoise";
  }
  if (engine == TracerEngine::TIMERLAT) {
    return "timerlat";
  }
  // The requirement above stands NONE out, so the detector word stands the
  // only answer left (FR-018, D-2).
  return "hwlat";
}

auto tracerStandsAsCurrent(const TracerEngine engine) -> bool
{
  // specs/048 FR-023, P-15: the standing engines write the engine word, so a
  // run that asked for one of them names no standing tracer, whatever the file
  // happens to hold after a failed setup wrote its saved word back.
  if (engine != TracerEngine::HWLAT) {
    return false;
  }
  const auto text = readTracerFile("sys/kernel/tracing/current_tracer");
  if (!text.has_value()) {
    return false;
  }
  return trimmedWordOf(*text) == engineWordOf(engine);
}

auto noiseTracerSetup(TracerEngine engine,
                      const std::vector<std::uint32_t>& cpus)
    -> std::optional<NoiseTracerSession>
{
  SG_REQUIRE(engine != TracerEngine::NONE,
             "noiseTracerSetup reaches an engine that names no tracer");
  SG_REQUIRE(!cpus.empty(), "noiseTracerSetup reaches an empty cpu list");
  // The word stands read first, because the word decides whether the run
  // writes at all (FR-019, R-04, P-15).
  const auto word = readTracerFile("sys/kernel/tracing/current_tracer");
  if (!word.has_value()) {
    return std::nullopt;
  }
  const std::string engineWord = engineWordOf(engine);
  if (engine == TracerEngine::HWLAT && trimmedWordOf(*word) == engineWord) {
    // The kernel answers success without initializing a tracer that
    // already stands, so it clears no buffer and the live detector's lines
    // stand outside this run: the run writes nothing and answers the shape
    // FR-023 names (FR-019, FR-023, P-15).
    return std::nullopt;
  }
  const auto mask = readTracerFile("sys/kernel/tracing/tracing_cpumask");
  if (!mask.has_value()) {
    return std::nullopt;
  }
  const auto maskWidth = maskWidthOf(*mask);
  if (!maskWidth.has_value()) {
    return std::nullopt;
  }
  // The detector's four further reads stand ahead of the first write with
  // the two above, the six reads FR-019 names (FR-019, D-8).
  std::optional<std::string> mode;
  std::optional<std::string> knobWidth;
  std::optional<std::string> knobWindow;
  std::optional<std::string> knobThreshold;
  if (engine == TracerEngine::HWLAT) {
    mode = readTracerFile("sys/kernel/tracing/hwlat_detector/mode");
    knobWidth = readTracerFile("sys/kernel/tracing/hwlat_detector/width");
    knobWindow = readTracerFile("sys/kernel/tracing/hwlat_detector/window");
    knobThreshold = readTracerFile("sys/kernel/tracing/tracing_thresh");
    if (!mode.has_value() || !knobWidth.has_value() || !knobWindow.has_value()
        || !knobThreshold.has_value())
    {
      return std::nullopt;
    }
  }
  // The writes, each with the value the setup saves ahead of it, in the
  // order FR-019 names for the detector and the order R-04 names for the
  // two standing engines (D-9, D-8, P-14).
  const SetupStep maskStep {
      {"sys/kernel/tracing/tracing_cpumask", trimmedWordOf(*mask)},
      renderCpuMask(cpus, *maskWidth)};
  const SetupStep wordStep {
      {"sys/kernel/tracing/current_tracer", trimmedWordOf(*word)}, engineWord};
  std::vector<SetupStep> steps;
  if (engine == TracerEngine::HWLAT) {
    steps = {
        maskStep,
        {{"sys/kernel/tracing/hwlat_detector/mode", modeWordOf(*mode)},
         kHwlatModeWord},
        {{"sys/kernel/tracing/hwlat_detector/width", trimmedWordOf(*knobWidth)},
         std::to_string(kHwlatWidthUs)},
        {{"sys/kernel/tracing/hwlat_detector/window",
          trimmedWordOf(*knobWindow)},
         std::to_string(kHwlatWindowUs)},
        {{"sys/kernel/tracing/tracing_thresh", trimmedWordOf(*knobThreshold)},
         std::to_string(kHwlatThresholdUs * 1000ULL)},
        wordStep};
  } else {
    steps = {wordStep, maskStep};
  }
  std::vector<TracerSetting> saved;
  std::vector<TracerSetting> undone;
  for (const auto& step : steps) {
    if (!writeTracerFile(step.saved.path, step.text + "\n")) {
      writeSavedBack(saved, undone);
      return std::nullopt;
    }
    saved.push_back(step.saved);
  }
  // S-5: the word `tracing_on` held before the start write, so the restore
  // can put it back. The read stands after the saved set's writes, outside
  // the six reads FR-019 orders ahead of the first write, and before the one
  // write of this file. A file that answers nothing reads as the stop word.
  const std::string standingOn = trimmedWordOf(
      readTracerFile("sys/kernel/tracing/tracing_on").value_or("0\n"));
  if (!writeTracerFile("sys/kernel/tracing/tracing_on", "1\n")) {
    writeSavedBack(saved, undone);
    return std::nullopt;
  }
  NoiseTracerSession session;
  session.previousTracingOn = standingOn;
  session.engine = engine;
  session.cpus = cpus;
  session.previousTracer = trimmedWordOf(*word);
  session.previousCpuMask = trimmedWordOf(*mask);
  session.saved = std::move(saved);
  SG_ENSURE(session.engine == engine && session.cpus == cpus
                && !session.previousCpuMask.empty()
                && session.saved.size() == steps.size(),
            "noiseTracerSetup answers a session that carries the engine, "
            "the cpus, the standing mask text and one saved pair per write");
  return session;
}

auto noiseTracerFigures(const NoiseTracerSession& session)
    -> std::vector<PrecheckTracerCpuRecord>
{
  SG_REQUIRE(session.engine != TracerEngine::NONE,
             "noiseTracerFigures reaches a session that names no tracer");
  std::vector<PrecheckTracerCpuRecord> figures;
  figures.reserve(session.cpus.size());
  for (const auto cpu : session.cpus) {
    // All three engines read the per-cpu buffer text the kernel carries,
    // one read per cpu for the run (FR-027, D-9). The read keeps no buffer
    // of its own: it folds the text into the entry and answers (FR-018).
    const auto text = readTracerFile("sys/kernel/tracing/per_cpu/cpu"
                                     + std::to_string(cpu) + "/trace");
    if (session.engine == TracerEngine::HWLAT) {
      // The detector's three figures come off the buffer text the detector
      // wrote (FR-018, FR-027, A-6).
      figures.push_back(gapFiguresOfCpu(cpu, text));
      continue;
    }
    if (session.engine == TracerEngine::OSNOISE) {
      figures.push_back(osnoiseFiguresOfCpu(cpu, text));
      continue;
    }
    figures.push_back(timerlatFiguresOfCpu(cpu, text));
  }
  SG_ENSURE(figures.size() == session.cpus.size(),
            "noiseTracerFigures answers one entry per session cpu");
  return figures;
}

auto noiseTracerRestore(const NoiseTracerSession& session)
    -> std::vector<TracerSetting>
{
  SG_REQUIRE(session.engine != TracerEngine::NONE,
             "noiseTracerRestore reaches a session that names no tracer");
  // The stop stands written ahead of the saved set, so the engine stops
  // before any file it reads stands back (FR-021, P-8, SC-007). The answer
  // names no failure of this write: no saved value backs it (D-8).
  writeTracerFile("sys/kernel/tracing/tracing_on", "0\n");
  std::vector<TracerSetting> failed;
  writeSavedBack(session.saved, failed);
  // S-5: a host that traced before the run traces after it. The stop above
  // stays first, so the engine never runs over a half-restored set; the word
  // the setup read goes back only once every saved file stands back. A failed
  // write-back keeps the stop: a word that stayed `hwlat` must not run again.
  if (failed.empty() && session.previousTracingOn != "0") {
    writeTracerFile("sys/kernel/tracing/tracing_on",
                    session.previousTracingOn + "\n");
  }
  SG_ENSURE(failed.size() <= session.saved.size(),
            "noiseTracerRestore answers no more failures than the saved set "
            "holds files");
  return failed;
}

}  // namespace sg::detail
