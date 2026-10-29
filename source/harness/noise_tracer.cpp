// ============================================================================
// The noise tracer value parser and the engine session
// (specs/036-osnoise-timerlat-tracers T007, T011, T012).
//
// One translation unit owns the tracer: the value grammar of
// contracts/cli-options.md, the standing cpu-list reader of the jitter
// probe, the refusal shape the option module quotes, and the session
// lifecycle and the figure read over the tracefs files the file-root
// seam resolves (R-01 to R-05, R-07).
// ============================================================================

#include <array>
#include <charconv>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "detail/noise_tracer.hpp"

#include "detail/jitter_probe.hpp"
#include "detail/options.hpp"
#include "detail/proc_root.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg::detail
{

// The value grammar of contracts/cli-options.md (FR-002, FR-003, D-4,
// R-06): the head names the engine word osnoise or timerlat, then at most
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
  } else {
    // An empty word refuses the same way: the value names an engine, so
    // the refusal reads the empty token (FR-002, R-06).
    message =
        "the engine '" + quotedText(head) + "' is not osnoise or timerlat";
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

}  // namespace

// The word the engine writes into `current_tracer` (R-02): see the
// declaration's contract in detail/noise_tracer.hpp.
auto engineWordOf(TracerEngine engine) -> std::string
{
  SG_REQUIRE(engine != TracerEngine::NONE,
             "engineWordOf reaches an engine that names no tracer");
  return engine == TracerEngine::OSNOISE ? "osnoise" : "timerlat";
}

auto noiseTracerSetup(TracerEngine engine,
                      const std::vector<std::uint32_t>& cpus)
    -> std::optional<NoiseTracerSession>
{
  SG_REQUIRE(engine != TracerEngine::NONE,
             "noiseTracerSetup reaches an engine that names no tracer");
  SG_REQUIRE(!cpus.empty(), "noiseTracerSetup reaches an empty cpu list");
  const auto word = readTracerFile("sys/kernel/tracing/current_tracer");
  if (!word.has_value()) {
    return std::nullopt;
  }
  const auto mask = readTracerFile("sys/kernel/tracing/tracing_cpumask");
  if (!mask.has_value()) {
    return std::nullopt;
  }
  const auto width = maskWidthOf(*mask);
  if (!width.has_value()) {
    return std::nullopt;
  }
  if (!writeTracerFile("sys/kernel/tracing/current_tracer",
                       engineWordOf(engine) + "\n"))
  {
    return std::nullopt;
  }
  if (!writeTracerFile("sys/kernel/tracing/tracing_cpumask",
                       renderCpuMask(cpus, *width) + "\n"))
  {
    return std::nullopt;
  }
  if (!writeTracerFile("sys/kernel/tracing/tracing_on", "1\n")) {
    return std::nullopt;
  }
  NoiseTracerSession session;
  session.engine = engine;
  session.cpus = cpus;
  session.previousTracer = trimmedWordOf(*word);
  session.previousCpuMask = trimmedWordOf(*mask);
  SG_ENSURE(session.engine == engine && session.cpus == cpus
                && !session.previousCpuMask.empty(),
            "noiseTracerSetup answers a session that carries the engine, "
            "the cpus and the standing mask text");
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
    figures.push_back(
        figuresOfCpu(cpu,
                     readTracerFile("sys/kernel/tracing/osnoise/cpu"
                                    + std::to_string(cpu))));
  }
  SG_ENSURE(figures.size() == session.cpus.size(),
            "noiseTracerFigures answers one entry per session cpu");
  return figures;
}

auto noiseTracerRestore(const NoiseTracerSession& session) -> void
{
  SG_REQUIRE(session.engine != TracerEngine::NONE,
             "noiseTracerRestore reaches a session that names no tracer");
  writeTracerFile("sys/kernel/tracing/tracing_on", "0\n");
  writeTracerFile("sys/kernel/tracing/current_tracer",
                  session.previousTracer + "\n");
  writeTracerFile("sys/kernel/tracing/tracing_cpumask",
                  session.previousCpuMask + "\n");
}

}  // namespace sg::detail
