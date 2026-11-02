// Config-word composition for the Linux PMU provider
// (specs/007-counters-and-timers, US6, T046, FR-037, R-010). The
// vendored table carries semantic field values; the running kernel
// carries the bit layout of every field it accepts. This translation
// unit joins the two and refuses to compose a partial encoding. Off
// Linux the file compiles to nothing (FR-042).

#ifdef __linux__

#  include <algorithm>
#  include <charconv>
#  include <cstdint>
#  include <limits>
#  include <string>
#  include <string_view>
#  include <utility>
#  include <vector>

#  include "../detail/pmu.hpp"

namespace sg::counters::detail
{
namespace
{

// The config word a field name selects: "config" is word 0, "config1"
// is word 1, and "configN" is word N. A name without a trailing index
// is word 0.
/// Whether the byte carries a lowercase letter, the alphabet the
/// config-word prefixes use.
///
/// \pre none
/// \post True exactly for the bytes 'a' through 'z'.
[[nodiscard]] auto isConfigLetter(const char byte) noexcept -> bool
{
  return byte >= 'a' && byte <= 'z';
}

/// Whether the byte carries a decimal digit.
///
/// \pre none
/// \post True exactly for the bytes '0' through '9'.
[[nodiscard]] auto isConfigDigit(const char byte) noexcept -> bool
{
  return byte >= '0' && byte <= '9';
}

auto wordOf(const std::string_view name) -> int
{
  int word = 0;
  std::size_t index = 0;
  // Skip the alphabetic prefix: "config", "config1", "config2". The
  // stop reasons stand apart, so each edge carries its own count: the
  // prefix ends where the digits begin, or the name ends first.
  while (index < name.size()) {
    if (!isConfigLetter(name[index])) {
      break;
    }
    ++index;
  }
  for (; index < name.size(); ++index) {
    if (!isConfigDigit(name[index])) {
      return 0;
    }
    word = word * 10 + static_cast<int>(name[index] - '0');
  }
  return word;
}

}  // namespace

auto parseFormatField(const std::string_view spec,
                      std::vector<FormatRange>& out) -> bool
{
  // Sysfs spells a format file "<field>:<low>-<high>,<low>-<high>".
  // Anything without that separator describes no config bits.
  const std::size_t colon = spec.find(':');
  if (colon == std::string_view::npos) {
    out.clear();
    return false;
  }
  const int word = wordOf(spec.substr(0, colon));
  // Parsed into a local and moved out on success, so the caller's vector
  // ends up holding exactly this file's ranges: a second parse into the
  // same vector replaces the first result, and a failed parse leaves
  // nothing behind.
  std::vector<FormatRange> parsed;
  std::size_t cursor = colon + 1;
  std::size_t found = 0;
  // The cursor walks one piece past each comma, and a comma index is at
  // most `spec.size() - 1`, so the cursor never stands past the spec on
  // entry; the loop leaves through its breaks only.
  for (;;) {  // LCOV_EXCL_BR_LINE
    const std::size_t comma = spec.find(',', cursor);
    const std::string_view piece =
        spec.substr(cursor,
                    comma == std::string_view::npos ? std::string_view::npos
                                                    : comma - cursor);
    if (piece.empty()) {
      break;
    }
    const std::size_t dash = piece.find('-');
    int low = 0;
    int high = 0;
    auto read = [](const std::string_view digits, int& value) -> bool
    {
      if (digits.empty() || digits.size() > 2) {
        return false;
      }
      value = 0;
      for (const char digit : digits) {
        if (digit < '0' || digit > '9') {
          return false;
        }
        value = value * 10 + static_cast<int>(digit - '0');
      }
      return true;
    };
    const bool lowOk = read(piece.substr(0, dash), low);
    const bool highOk = dash == std::string_view::npos
        ? (high = low, true)
        : read(piece.substr(dash + 1), high);
    if (!lowOk || !highOk || high < low) {
      out.clear();
      return false;
    }
    parsed.push_back(
        FormatRange {.configWord = word, .low = low, .high = high});
    ++found;
    if (comma == std::string_view::npos) {
      break;
    }
    cursor = comma + 1;
  }
  if (found == 0) {
    out.clear();
    return false;
  }
  out = std::move(parsed);
  return true;
}

auto fixedCounterDesignation(const std::string_view text) noexcept -> int
{
  // The vendored spelling is "Fixed counter 0" with a decimal number, and
  // the number is the whole tail: "Fixed counter x" and "Fixed counter 1x"
  // name no counter the table means. A range text such as "0,1,2,3" names
  // the programmable counters and reaches the same answer, because the
  // composition of such a row already reads its own event code (FR-037).
  constexpr std::string_view kPrefix = "Fixed counter ";
  if (!text.starts_with(kPrefix)) {
    return -1;
  }
  const auto tail = text.substr(kPrefix.size());
  int number = -1;
  const auto parsed =
      std::from_chars(tail.data(), tail.data() + tail.size(), number);
  if (parsed.ec != std::errc() || parsed.ptr != tail.data() + tail.size()) {
    return -1;
  }
  return number < 0 ? -1 : number;
}

auto pmuRowTakesFixedAlias(
    const std::vector<std::pair<std::string, std::uint64_t>>& fields,
    const int fixedCounter) noexcept -> bool
{
  // Two facts settle it. The row names one fixed counter, and the row
  // states no event code of its own. A row that states an event already
  // names the bits the composition places, whatever counter it names, so
  // the standing composition stands for it (FR-037, PC-7).
  return fixedCounter >= 0
      && !std::ranges::any_of(
             fields,
             [](const std::pair<std::string, std::uint64_t>& field)
             { return field.first == "event"; });
}

auto pmuFixedCounterAlias(const std::uint32_t number) noexcept
    -> std::string_view
{
  // The three architectural fixed counters, each under the alias name the
  // kernel publishes beside the `format/` directory it composes against
  // (/sys/bus/event_source/devices/cpu/events/). A host publishing none of
  // these names no fixed counter, and the row that asks for one is refused
  // below, so it never composes the degraded umask (PC-7).
  switch (number) {
    case 0:
      return "instructions";
    case 1:
      return "cpu-cycles";
    case 2:
      return "ref-cycles";
    default:
      return {};
  }
}

auto pmuComposeConfig(
    const std::vector<std::pair<std::string, std::uint64_t>>& fields,
    const std::vector<std::pair<std::string, std::vector<FormatRange>>>&
        formats,
    std::vector<std::pair<int, std::uint64_t>>& words) -> bool
{
  // A field the running kernel does not publish, or a value too wide
  // for its bit range, leaves the entry not_encodable: the composition
  // is refused whole, never half-attempted (FR-037).
  std::vector<std::pair<int, std::uint64_t>> composed;
  for (const auto& [name, value] : fields) {
    const std::vector<FormatRange>* ranges = nullptr;
    for (const auto& [formatName, layout] : formats) {
      if (formatName == name) {
        ranges = &layout;
        break;
      }
    }
    if (ranges == nullptr || ranges->empty()) {
      return false;
    }
    // A field the kernel publishes as several bit ranges splits its
    // value across them in order: the first range takes the low bits and
    // each later range takes the bits above the one before it.
    // "event: config:0-7,32-35"
    // therefore places event bits 0-7 in config bits 0-7 and event bits
    // 8-11 in config bits 32-35. A value wider than the ranges span
    // leaves the entry not_encodable (FR-037).
    unsigned shift = 0;
    for (const auto& range : *ranges) {
      if (shift >= 64) {
        return false;
      }
      const auto width = static_cast<std::uint64_t>(range.high - range.low + 1);
      const std::uint64_t capacity = width >= 64
          ? std::numeric_limits<std::uint64_t>::max()
          : (1ULL << width) - 1ULL;
      const std::uint64_t chunk = (value >> shift) & capacity;
      const auto placed = chunk << static_cast<unsigned>(range.low);
      bool merged = false;
      for (auto& word : composed) {
        if (word.first == range.configWord) {
          word.second |= placed;
          merged = true;
          break;
        }
      }
      if (!merged) {
        composed.emplace_back(range.configWord, placed);
      }
      shift += static_cast<unsigned>(width);
    }
    if (shift < 64 && (value >> shift) != 0) {
      return false;
    }
  }
  words = std::move(composed);
  return true;
}

auto pmuComposeFixedCounterRow(
    const std::uint32_t number,
    const std::vector<std::pair<int, std::uint64_t>>& aliasWords,
    const std::vector<std::pair<std::string, std::uint64_t>>& fields,
    const std::vector<std::pair<std::string, std::vector<FormatRange>>>&
        formats,
    std::vector<std::pair<int, std::uint64_t>>& words,
    std::string& refusal) -> bool
{
  // The event comes from the alias, because that text is the encoding this
  // kernel publishes for the counter it names. The row contributes its own
  // fields with the `umask` left out: on a fixed row the umask is the
  // counter's number, one above the number the kernel publishes, and it
  // lands on the umask bits of a programmable event. No bit reaches the
  // fixed counter, which is why the degraded composition counts nothing
  // (PC-7).
  if (aliasWords.empty()) {
    refusal = "this kernel publishes no alias for fixed counter "
        + std::to_string(number)
        + ", so the row names a counter with no encoding on this host";
    return false;
  }
  std::vector<std::pair<std::string, std::uint64_t>> own;
  own.reserve(fields.size());
  for (const auto& field : fields) {
    if (field.first != "umask") {
      own.push_back(field);
    }
  }
  std::vector<std::pair<int, std::uint64_t>> composed;
  if (!pmuComposeConfig(own, formats, composed)) {
    refusal = "the row for fixed counter " + std::to_string(number)
        + " carries a field this device publishes no format for";
    return false;
  }
  // The two halves describe one event, so a bit the alias places and a bit
  // the row places cannot share a position. A row stating an event code of
  // its own would meet that guard, and the caller excludes that row before
  // this function runs: such a row states an event and composes from its
  // own fields.
  auto merged = aliasWords;
  for (const auto& [word, value] : composed) {
    bool placed = false;
    for (auto& target : merged) {
      if (target.first != word) {
        continue;
      }
      if ((target.second & value) != 0) {
        refusal = "the row for fixed counter " + std::to_string(number)
            + " places bits the kernel alias of that counter already places";
        return false;
      }
      target.second |= value;
      placed = true;
      break;
    }
    if (!placed) {
      merged.emplace_back(word, value);
    }
  }
  words = std::move(merged);
  return true;
}

}  // namespace sg::counters::detail

#endif  // __linux__
