// CPUID ident, mapfile directory selection, and lazy simdjson event-
// table parsing for the Linux PMU provider (specs/007-counters-and-
// timers, US6, R-010, FR-038). This TU is the sole includer of
// <simdjson.h> under source/counters/ (FR-013 privacy boundary);
// nothing here leaks JSON types through the seam. Off-Linux the file
// compiles to nothing (FR-042).

#ifdef __linux__

// P2 diagnostic suppression, written justification (constitution I, VIII):
// GCC 16 reports `-Wmaybe-uninitialized` inside libstdc++'s own
// `std::regex` NFA builder (`std_function.h:395` and `:252`, reached from
// `regex_automaton.h:_M_insert_subexpr_begin`) at -O2 and above. The
// reported objects are libstdc++'s, the diagnostic names no local
// variable, and the same translation unit compiles clean at -Og. This
// suppression covers exactly the mapfile pattern match, the only
// `std::regex` use in the library, and the file-scope form is required
// because GCC attributes the diagnostic to the system header location.
// The call site never receives the diagnostic. Replacing the matcher
// with a bespoke regular-expression engine would trade a compiler
// false positive for a silent mis-selection of the architecture event
// table, which is a far worse failure; the gate stays on for every
// other diagnostic and for every other translation unit.
#  if defined(__GNUC__) && !defined(__clang__)
#    pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#  endif

#  include <algorithm>
#  include <cctype>
#  include <charconv>
#  include <cstdint>
#  include <cstdio>
#  include <filesystem>
#  include <fstream>
#  include <map>
#  include <mutex>
#  include <regex>
#  include <string>
#  include <string_view>
#  include <utility>
#  include <vector>

#  include <cpuid.h>
#  include <simdjson.h>

#  include "../detail/pmu.hpp"

namespace sg::counters::detail
{

auto pmu_ident_current() -> pmu_ident
{
  // Cached once per process (FR-038): the ident never changes.
  static const pmu_ident ident = []
  {
#  if defined(__x86_64__) || defined(__i386__)
    pmu_ident id;
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    if (__get_cpuid(0, &eax, &ebx, &ecx, &edx) && eax >= 1) {
      // The vendor string lives in EBX:EDX:ECX of leaf 0. Leaf 1
      // overwrites those registers, so the string is captured before
      // the family and model are read.
      const unsigned int words[3] = {ebx, edx, ecx};
      id.vendor.resize(12);
      for (int w = 0; w < 3; ++w) {
        for (int b = 0; b < 4; ++b) {
          const auto index =
              static_cast<std::size_t>(w) * 4U + static_cast<std::size_t>(b);
          id.vendor[index] = static_cast<char>((words[w] >> (8 * b)) & 0xFFU);
        }
      }
    }
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
      const auto base_family = static_cast<int>((eax >> 8) & 0xFU);
      const auto ext_family = static_cast<int>((eax >> 20) & 0xFFU);
      const auto base_model = static_cast<int>((eax >> 4) & 0xFU);
      const auto ext_model = static_cast<int>((eax >> 16) & 0xFU);
      id.family = base_family + ext_family;
      // The mapfile's hex-model spelling (kernel jevents convention).
      id.model = base_model | (ext_model << 4);
    }
    return id;
#  else
    return pmu_ident {};
#  endif
  }();
  return ident;
}

namespace
{

// The vendored mapfile patterns use POSIX classes ([[:xdigit:]]);
// std::regex is ECMAScript, so translate every class name.
auto to_ecma(std::string_view pattern) -> std::string
{
  constexpr std::string_view class_name[] = {"alnum",
                                             "alpha",
                                             "blank",
                                             "cntrl",
                                             "digit",
                                             "graph",
                                             "lower",
                                             "print",
                                             "punct",
                                             "space",
                                             "upper",
                                             "word",
                                             "xdigit"};
  constexpr std::string_view replacement[] = {"0-9A-Za-z",
                                              "A-Za-z",
                                              " \\t",
                                              "\\x00-\\x1f\\x7f",
                                              "0-9",
                                              "!-~",
                                              "a-z",
                                              " -~",
                                              "!-/:-@[-`{-~",
                                              " \\t\\n\\v\\f\\r",
                                              "A-Z",
                                              "0-9A-Za-z_",
                                              "0-9A-Fa-f"};
  std::string out;
  out.reserve(pattern.size());
  std::size_t i = 0;
  while (i < pattern.size()) {
    bool replaced = false;
    if (pattern.compare(i, 2, "[[") == 0) {
      // The class ends at the first `]]` at or after the brackets; a
      // one-bracket span is not a class, because `[[]` is a class holding
      // `[`, so the search starts one past the opening bracket.
      const std::size_t close = pattern.find("]]", i + 1);
      if (close != std::string::npos) {
        // POSIX wraps the class name in colons, so `[[:xdigit:]]` spans
        // `:xdigit:` between the brackets.
        const std::string_view body = pattern.substr(i + 2, close - (i + 2));
        const std::string_view name = body.starts_with(':') && body.size() > 2
            ? body.substr(1, body.size() - 2)
            : std::string_view {};
        for (int k = 0; k < 13; ++k) {
          if (name == class_name[k]) {
            // The table holds the class body, so the brackets that
            // delimit it are written here.
            out += '[';
            out += replacement[k];
            out += ']';
            i = close + 2;
            replaced = true;
            break;
          }
        }
      }
    }
    if (!replaced) {
      out.push_back(pattern[i]);
      ++i;
    }
  }
  return out;
}

auto to_lower(std::string text) -> std::string
{
  std::transform(text.begin(),
                 text.end(),
                 text.begin(),
                 [](unsigned char c)
                 { return static_cast<char>(std::tolower(c)); });
  return text;
}

// Parse a scalar as a config value: hex string ("0x3c"), decimal
// string, or integer. False for anything else (expressions, free
// text), so non-semantic keys are skipped when their parse fails.
auto parse_scalar(simdjson::dom::element value, std::uint64_t& out) -> bool
{
  std::uint64_t number = 0;
  if (value.get_uint64().get(number) == simdjson::SUCCESS) {
    out = number;
    return true;
  }
  std::int64_t signed_number = 0;
  if (value.get_int64().get(signed_number) == simdjson::SUCCESS) {
    if (signed_number < 0) {
      return false;
    }
    out = static_cast<std::uint64_t>(signed_number);
    return true;
  }
  std::string_view text;
  if (value.get_string().get(text) != simdjson::SUCCESS) {
    return false;
  }
  const bool hex = text.starts_with("0x") || text.starts_with("0X");
  const std::string_view digits = hex ? text.substr(2) : text;
  if (digits.empty()) {
    return false;
  }
  std::uint64_t parsed = 0;
  const auto [end, error] = std::from_chars(
      digits.data(), digits.data() + digits.size(), parsed, hex ? 16 : 10);
  return error == std::errc {} && end == digits.data() + digits.size();
}

void add_entry(std::vector<pmu_table_entry>& table,
               std::string_view name,
               simdjson::dom::object attributes)
{
  // Metric definitions are catalog data for a future release,
  // never countables (R-010, US6).
  if (attributes["MetricExpr"].error() == simdjson::SUCCESS
      || attributes["MetricName"].error() == simdjson::SUCCESS)
  {
    return;
  }
  pmu_table_entry entry;
  entry.name = std::string(name);
  for (auto [key, value] : attributes) {
    std::uint64_t number = 0;
    if (key == "EventCode") {
      if (parse_scalar(value, number)) {
        entry.fields.emplace_back("event", number);
      }
    } else if (key == "UMask" || key == "Umask") {
      if (parse_scalar(value, number)) {
        entry.fields.emplace_back("umask", number);
      }
    } else if (key == "UMASK_EXT") {
      if (parse_scalar(value, number)) {
        entry.fields.emplace_back("umask_ext", number);
      }
    } else if (key == "EventName" || key == "Description"
               || key == "PublicDescription" || key == "BriefDescription"
               || key == "Unit")
    {
      // Descriptive keys carry no config semantic.
    } else if (parse_scalar(value, number)) {
      // Any other integer-valued key names a format field
      // directly (e.g. "CounterMask", "Edge").
      entry.fields.emplace_back(key, number);
    }
  }
  // Description precedence; AMD tables ship only "BriefDescription".
  for (const auto candidate :
       {
           "Description",
           "PublicDescription",
           "BriefDescription",
       })
  {
    simdjson::dom::element found;
    std::string_view text;
    if (attributes[candidate].get(found) == simdjson::SUCCESS
        && found.get_string().get(text) == simdjson::SUCCESS)
    {
      entry.description = std::string(text);
      break;
    }
  }
  simdjson::dom::element unit;
  std::string_view unit_text;
  if (attributes["Unit"].get(unit) == simdjson::SUCCESS
      && unit.get_string().get(unit_text) == simdjson::SUCCESS)
  {
    entry.unit = std::string(unit_text);
  }
  table.push_back(std::move(entry));
}

void parse_json_file(const std::filesystem::path& path,
                     std::vector<pmu_table_entry>& table)
{
  // A file that fails to parse is skipped silently; total failure
  // surfaces as an empty table (seam contract). No exceptions: every
  // simdjson error becomes a skip, the padded read included.
  auto loaded = simdjson::padded_string::load(path.string());
  if (loaded.error() != simdjson::SUCCESS) {
    return;
  }

  simdjson::dom::parser dom;
  simdjson::dom::element root;
  if (dom.parse(loaded.value_unsafe()).get(root) != simdjson::SUCCESS) {
    return;
  }
  // Kernel tables appear in two shapes: an array of objects each
  // with an "EventName" key, or a top-level object mapping event
  // name to attributes.
  simdjson::dom::array items;
  if (root.get_array().get(items) == simdjson::SUCCESS) {
    for (auto item : items) {
      simdjson::dom::object attributes;
      simdjson::dom::element name;
      std::string_view name_text;
      if (item.get_object().get(attributes) == simdjson::SUCCESS
          && attributes["EventName"].get(name) == simdjson::SUCCESS
          && name.get_string().get(name_text) == simdjson::SUCCESS)
      {
        add_entry(table, name_text, attributes);
      }
    }
    return;
  }
  simdjson::dom::object pairs;
  if (root.get_object().get(pairs) == simdjson::SUCCESS) {
    for (auto [key, value] : pairs) {
      simdjson::dom::object attributes;
      if (value.get_object().get(attributes) == simdjson::SUCCESS) {
        add_entry(table, key, attributes);
      }
    }
  }
}

}  // namespace

auto pmu_select_directory(const pmu_ident& id) -> std::string
{
#  ifdef SG_PMU_EVENTS_DIR
  static std::mutex cache_mutex;
  static std::map<std::string, std::string> cache;
  // Format finding (the vendored file is truth): the mapfile lives
  // at arch/x86/mapfile.csv; columns are "Family-model,Version,
  // Filename,EventType"; the first is a POSIX-class regex matched
  // against "<vendor>-<family>-<MODEL>" with family decimal and
  // model uppercase hex. First matching row wins (FR-038); the
  // Filename column is a directory name relative to the mapfile.
  char hex[8];
  std::snprintf(hex, sizeof(hex), "%X", static_cast<unsigned>(id.model));
  const std::string key =
      id.vendor + '-' + std::to_string(id.family) + '-' + hex;

  const std::scoped_lock lock(cache_mutex);
  if (const auto cached = cache.find(key); cached != cache.end()) {
    return cached->second;
  }

  std::string selected;
  std::ifstream mapfile(std::string(SG_PMU_EVENTS_DIR)
                        + "/arch/x86/mapfile.csv");
  std::string line;
  bool header = true;
  while (std::getline(mapfile, line)) {
    if (header) {
      header = false;
      continue;
    }
    const std::string_view text = line;
    const std::size_t first = text.find(',');
    if (first == std::string::npos) {
      continue;
    }
    const std::size_t second = text.find(',', first + 1);
    const std::size_t third = text.find(',', second + 1);
    if (second == std::string::npos || third == std::string::npos) {
      continue;
    }
    const std::string_view filename =
        text.substr(second + 1, third - (second + 1));
    if (filename.empty()) {
      continue;
    }
    try {
      const std::regex pattern(to_lower(to_ecma(text.substr(0, first))),
                               std::regex::icase | std::regex::optimize);
      if (std::regex_match(key, pattern)) {
        selected = "arch/x86/";
        selected += filename;
        selected += '/';
        break;
      }
    } catch (const std::regex_error&) {
      // A row whose pattern does not compile cannot match.
    }
  }
  cache.emplace(key, selected);
  return selected;
#  else
  static_cast<void>(id);
  return {};
#  endif
}

auto pmu_load_table(const std::string& directory)
    -> const std::vector<pmu_table_entry>&
{
#  ifdef SG_PMU_EVENTS_DIR
  // Once-per-directory lazy cache (FR-038).
  static std::mutex cache_mutex;
  static std::map<std::string, std::vector<pmu_table_entry>> cache;

  const std::scoped_lock lock(cache_mutex);
  if (const auto cached = cache.find(directory); cached != cache.end()) {
    return cached->second;
  }

  std::vector<pmu_table_entry> table;
  std::vector<std::filesystem::path> files;
  const std::filesystem::path dir =
      std::string(SG_PMU_EVENTS_DIR) + '/' + directory;
  // Missing or unreadable directory: no files, empty table (seam
  // contract); the throwing iterator would be an exception.
  std::error_code code;
  const std::filesystem::directory_iterator end;
  for (auto it = std::filesystem::directory_iterator(
           dir,
           std::filesystem::directory_options::skip_permission_denied,
           code);
       it != end;
       it.increment(code))
  {
    if (it->is_regular_file(code) && it->path().extension() == ".json") {
      files.push_back(it->path());
    }
  }
  std::ranges::sort(files);
  for (const auto& file : files) {
    parse_json_file(file, table);
  }
  return cache.emplace(directory, std::move(table)).first->second;
#  else
  static const std::vector<pmu_table_entry> empty;
  static_cast<void>(directory);
  return empty;
#  endif
}

}  // namespace sg::counters::detail

#endif  // __linux__
