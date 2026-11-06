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

// The five source words of the pinned count-file format, and the
// record figure each one feeds (R-03).
struct CountSource
{
  const char* word;
  std::optional<std::uint64_t> PrecheckTracerCpuRecord::*figure;
};

constexpr std::array<CountSource, 5> kCountSources {{
    {"nmi", &PrecheckTracerCpuRecord::nmi},
    {"irq", &PrecheckTracerCpuRecord::irq},
    {"softirq", &PrecheckTracerCpuRecord::softirq},
    {"thread", &PrecheckTracerCpuRecord::thread},
    {"hw", &PrecheckTracerCpuRecord::hardware},
}};

// One unsigned figure off one token; a token that parses as no number
// answers nothing (R-07).
auto countOfToken(const std::string& token) -> std::optional<std::uint64_t>
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

// The figures the count file of one cpu carries (R-07): the five
// figures of the pinned format, `hw` mapping to `hardware`; a file
// that states nothing stands every figure absent, and one malformed
// line stands the whole file unparsable, every figure absent.
auto figuresOfCpu(std::uint32_t cpu, const std::optional<std::string>& text)
    -> PrecheckTracerCpuRecord
{
  PrecheckTracerCpuRecord record;
  record.cpu = cpu;
  if (!text.has_value()) {
    return record;
  }
  std::array<bool, kCountSources.size()> seen {};
  std::array<std::optional<std::uint64_t>, kCountSources.size()> counts;
  std::istringstream stream(*text);
  std::string line;
  while (std::getline(stream, line)) {
    if (trimmedWordOf(line).empty()) {
      continue;
    }
    const auto colon = line.find(':');
    if (colon == std::string::npos) {
      return record;
    }
    const auto word = trimmedWordOf(line.substr(0, colon));
    const auto count = countOfToken(trimmedWordOf(line.substr(colon + 1)));
    bool matched = false;
    for (std::size_t at = 0; at < kCountSources.size(); ++at) {
      if (word == kCountSources[at].word) {
        if (seen[at] || !count.has_value()) {
          return record;
        }
        seen[at] = true;
        counts.at(at) = count;
        matched = true;
        break;
      }
    }
    if (!matched) {
      return record;
    }
  }
  for (std::size_t at = 0; at < kCountSources.size(); ++at) {
    record.*kCountSources[at].figure = counts.at(at);
  }
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
  return text.has_value() && trimmedWordOf(*text) == engineWordOf(engine);
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
  if (!writeTracerFile("sys/kernel/tracing/tracing_on", "1\n")) {
    writeSavedBack(saved, undone);
    return std::nullopt;
  }
  NoiseTracerSession session;
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
    if (session.engine == TracerEngine::HWLAT) {
      // The detector names no count file, so the three figures come off
      // the buffer text the detector wrote (FR-018, FR-027, A-6). The read
      // keeps no buffer of the run's own: it folds the text into the entry
      // and answers (FR-018).
      figures.push_back(
          gapFiguresOfCpu(cpu,
                          readTracerFile("sys/kernel/tracing/per_cpu/cpu"
                                         + std::to_string(cpu) + "/trace")));
      continue;
    }
    figures.push_back(
        figuresOfCpu(cpu,
                     readTracerFile("sys/kernel/tracing/osnoise/cpu"
                                    + std::to_string(cpu))));
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
  SG_ENSURE(failed.size() <= session.saved.size(),
            "noiseTracerRestore answers no more failures than the saved set "
            "holds files");
  return failed;
}

}  // namespace sg::detail
