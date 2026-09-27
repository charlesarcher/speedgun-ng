// Config-word composition for the Linux PMU provider
// (specs/007-counters-and-timers, US6, T046, FR-037, R-010). The
// vendored table carries semantic field values; the running kernel
// carries the bit layout of every field it accepts. This translation
// unit joins the two and refuses to compose a partial encoding. Off
// Linux the file compiles to nothing (FR-042).

#ifdef __linux__

#  include <algorithm>
#  include <cstdint>
#  include <limits>
#  include <string_view>
#  include <utility>
#  include <vector>

#  include "../detail/pmu.hpp"

namespace sg::counters::detail
{
namespace
{

// The config word a field name selects: "config" is word 0, "config1"
// is word 1, and so on. A name without a trailing index is word 0.
auto word_of(const std::string_view name) -> int
{
  int word = 0;
  std::size_t index = 0;
  // Skip the alphabetic prefix: "config", "config1", "config2".
  while (index < name.size() && name[index] >= 'a' && name[index] <= 'z') {
    ++index;
  }
  for (; index < name.size(); ++index) {
    if (name[index] < '0' || name[index] > '9') {
      return 0;
    }
    word = word * 10 + static_cast<int>(name[index] - '0');
  }
  return word;
}

}  // namespace

auto parse_format_field(const std::string_view spec,
                        std::vector<format_range>& out) -> bool
{
  // Sysfs spells a format file "<field>:<low>-<high>,<low>-<high>".
  // Anything without that separator describes no config bits.
  const std::size_t colon = spec.find(':');
  if (colon == std::string_view::npos) {
    out.clear();
    return false;
  }
  const int word = word_of(spec.substr(0, colon));
  // Parsed into a local and moved out on success, so the caller's vector
  // ends up holding exactly this file's ranges: a second parse into the
  // same vector replaces the first result, and a failed parse leaves
  // nothing behind.
  std::vector<format_range> parsed;
  std::size_t cursor = colon + 1;
  std::size_t found = 0;
  while (cursor <= spec.size()) {
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
    const bool low_ok = read(piece.substr(0, dash), low);
    const bool high_ok = dash == std::string_view::npos
        ? (high = low, true)
        : read(piece.substr(dash + 1), high);
    if (!low_ok || !high_ok || high < low) {
      out.clear();
      return false;
    }
    parsed.push_back(
        format_range {.config_word = word, .low = low, .high = high});
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

auto pmu_compose_config(
    const std::vector<std::pair<std::string, std::uint64_t>>& fields,
    const std::vector<std::pair<std::string, std::vector<format_range>>>&
        formats,
    std::vector<std::pair<int, std::uint64_t>>& words) -> bool
{
  // A field the running kernel does not publish, or a value too wide
  // for its bit range, leaves the entry not_encodable: the composition
  // is refused whole, never half-attempted (FR-037).
  std::vector<std::pair<int, std::uint64_t>> composed;
  for (const auto& [name, value] : fields) {
    const std::vector<format_range>* ranges = nullptr;
    for (const auto& [format_name, layout] : formats) {
      if (format_name == name) {
        ranges = &layout;
        break;
      }
    }
    if (ranges == nullptr || ranges->empty()) {
      return false;
    }
    // A field the kernel publishes as several bit ranges splits its
    // value across them in order: the first range takes the low bits,
    // the next the bits above, and so on. "event: config:0-7,32-35"
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
        if (word.first == range.config_word) {
          word.second |= placed;
          merged = true;
          break;
        }
      }
      if (!merged) {
        composed.emplace_back(range.config_word, placed);
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

}  // namespace sg::counters::detail

#endif  // __linux__
