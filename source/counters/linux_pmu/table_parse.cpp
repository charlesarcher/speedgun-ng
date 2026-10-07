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
#  include <array>
#  include <cctype>
#  include <charconv>
#  include <cstdint>
#  include <cstdio>
#  include <filesystem>
#  include <fstream>
#  include <map>
#  include <mutex>
#  include <ranges>
#  include <regex>
#  include <sstream>
#  include <string>
#  include <string_view>
#  include <utility>
#  include <vector>

#  include <cpuid.h>
#  include <simdjson.h>

#  include "../detail/pmu.hpp"
#  include "embedded_tables.hpp"

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
    // LCOV_EXCL_BR_START : coverage exclusion (T066): a CPU that does not
    // answer CPUID leaf 0, or answers it with a highest basic leaf below 1.
    // Every x86 CPU that runs this code answers leaf 0, and its EAX is the
    // highest basic leaf number, which is at least 1 on any CPU that also
    // answers leaf 1.
    if (__get_cpuid(0, &eax, &ebx, &ecx, &edx)  // LCOV_EXCL_BR_LINE
        && eax >= 1)
    {  // LCOV_EXCL_BR_LINE
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
        }  // LCOV_EXCL_LINE
      }  // LCOV_EXCL_LINE
    }  // LCOV_EXCL_BR_LINE
    // LCOV_EXCL_BR_STOP
    // LCOV_EXCL_BR_START : coverage exclusion (T066): a CPU whose highest
    // basic leaf is 0, which predates every CPU this library targets.
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {  // LCOV_EXCL_BR_LINE
      const auto base_family = static_cast<int>((eax >> 8) & 0xFU);
      const auto ext_family = static_cast<int>((eax >> 20) & 0xFFU);
      const auto base_model = static_cast<int>((eax >> 4) & 0xFU);
      const auto ext_model = static_cast<int>((eax >> 16) & 0xFU);
      id.family = base_family + ext_family;
      // The mapfile's hex-model spelling (kernel jevents convention).
      id.model = base_model | (ext_model << 4);
    }  // LCOV_EXCL_BR_LINE
    // LCOV_EXCL_BR_STOP
    return id;
#  else
    return pmu_ident {};
#  endif
  }();
  return ident;
}

namespace
{

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
  if (value.get_uint64().get(out) == simdjson::SUCCESS) {
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
  if (error != std::errc {} || end != digits.data() + digits.size()) {
    return false;
  }
  out = parsed;
  return true;
}

// The sampling and metadata keys. One names a sampling period, the others
// name a scope label or an upstream annotation, and none of them names a
// kernel format, so none carries an encoding obligation (FR-016). They are
// dropped here, before the row reaches the encoder, because recording them
// as fields would demand a format the device does not publish and turn
// every Intel row into `not_encodable`. The register index sits beside them
// and is read ahead of this loop instead: the index names the format its
// register value encodes into, and the encoder sees that one field, never
// the index itself (FR-010, D-05).
auto carries_no_obligation(const std::string_view key) noexcept -> bool
{
  constexpr std::array<std::string_view, 6> no_obligation {
      "SampleAfterValue",
      "MSRIndex",
      "PEBS",
      "Data_LA",
      "PerPkg",
      "Experimental",
  };
  return std::ranges::find(no_obligation, key) != std::end(no_obligation);
}

// The constraint and deprecation keys. They name a condition on when a
// counter is meaningful. Neither names a bit in an encoding register, so
// they carry
// no encoding obligation either. The kernel publishes no format for either
// name, and a row reaching the encoder with either one resolves to nothing
// and publishes `not_encodable` for a field no format reads (FR-012,
// FR-013, D-06).
auto carries_no_constraint(const std::string_view key) noexcept -> bool
{
  constexpr std::array<std::string_view, 2> no_constraint {
      "Counter",
      "Deprecated",
  };
  return std::ranges::find(no_constraint, key) != std::end(no_constraint);
}

// The register value key. The value is not a field of its own either: the
// index names the format, and the pair is recorded under that one name so
// the encoder sees a single field it can resolve (FR-010, D-05).
auto is_register_value(const std::string_view key) noexcept -> bool
{
  return key == "MSRValue";
}

// The kernel's own register-index map, read from its table generator
// `tools/perf/pmu-events/jevents.py` at kernel commit
// eaab2eb09dc2f86f41e8fa55243c31a274978233, lines 248 to 257. The key is
// the register number the table names and the value is the event field the
// kernel encodes the value into. The kernel takes the first index of a
// comma-separated pair, so a pair resolves through this map exactly as a
// single index does (FR-010, D-05).
// The case labels are the kernel's register numbers. Naming each one
// would hide the map the kernel publishes.
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
auto register_format(const std::uint64_t index) noexcept -> std::string_view
{
  switch (index) {
    case 0x3f6:
      return "ldlat";
    case 0x3f7:
      return "frontend";
    case 0x1a6:
    case 0x1a7:
    case 0x3e0:
    case 0x3e1:
    case 0x3e2:
    case 0x3e3:
      return "offcore_rsp";
    default:
      return {};
  }
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

// The register value and index a row carries, if any. Both are string
// values in every table the pinned tree ships, and the index may be a
// comma-separated pair. The pair resolves through its first index, the
// same rule the kernel's generator applies (FR-010, D-05).
struct register_filter
{
  std::uint64_t value = 0;
  std::string_view format;
  bool present = false;
  // A non-zero register value whose index names no format. The row must
  // not encode as the base event with the filter dropped (FR-010).
  bool unnamed = false;
};

// A field name no kernel format directory publishes. Composition refuses
// a row that carries it, which is how the catalog publishes
// `not_encodable` (FR-010).
constexpr std::string_view kUnnamedRegister {"unnamed_register"};

// The register number one index text names. The text may carry a
// comma-separated pair, and the kernel's generator reads the first index of
// that pair, so the first index is what this parses.
auto first_index_of(const std::string_view text,
                    std::uint64_t& out) noexcept -> bool
{
  const auto comma = text.find(',');
  const std::string_view first =
      comma == std::string_view::npos ? text : text.substr(0, comma);
  // The tree spells an index with surrounding spaces in a pair, and the
  // scalar parse rejects a space, so the text is trimmed by hand.
  // NOLINTNEXTLINE(llvm-qualified-auto, readability-qualified-auto)
  const auto begin =
      std::ranges::find_if(first,
                           [](const char character) -> bool
                           { return character != ' ' && character != '\t'; });
  // NOLINTNEXTLINE(llvm-qualified-auto, readability-qualified-auto)
  const auto end =
      std::ranges::find_if(std::ranges::reverse_view(first),
                           [](const char character) -> bool
                           { return character != ' ' && character != '\t'; })
          .base();
  if (begin >= end) {
    return false;
  }
  const std::string_view trimmed(begin, end);
  const bool hex = trimmed.starts_with("0x") || trimmed.starts_with("0X");
  std::string_view digits = trimmed;
  if (hex) {
    digits = trimmed.substr(2);
  }
  if (digits.empty()) {
    return false;
  }
  std::uint64_t parsed = 0;
  const int base = hex ? 16 : 10;
  // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  const auto [stop, error] = std::from_chars(
      digits.data(), digits.data() + digits.size(), parsed, base);
  // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  if (error != std::errc {} || stop != digits.data() + digits.size()) {
    return false;
  }
  out = parsed;
  return true;
}

// NOLINTNEXTLINE(misc-include-cleaner)
auto register_filter_of(simdjson::dom::object attributes) -> register_filter
{
  register_filter out;
  simdjson::dom::element value_element;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
  if (attributes["MSRValue"].get(value_element) != simdjson::SUCCESS) {
    return out;
  }
  if (!parse_scalar(value_element, out.value)) {
    return out;
  }
  simdjson::dom::element index_element;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
  if (attributes["MSRIndex"].get(index_element) != simdjson::SUCCESS) {
    // A value with no index names no format. A zero value encodes no
    // filter, so only a non-zero value refuses the row (FR-010).
    out.unnamed = out.value != 0;
    return out;
  }
  std::string_view index_text;
  std::uint64_t index = 0;
  // A blank index, an index that does not parse, and an index that is not
  // a string name no format. A non-zero value then refuses the row, the
  // same refusal a missing index takes (FR-010).
  if (index_element.get_string().get(index_text) != simdjson::SUCCESS
      || !first_index_of(index_text, index))
  {
    out.unnamed = out.value != 0;
    return out;
  }
  out.format = register_format(index);
  out.present = true;
  out.unnamed = out.value != 0 && out.format.empty();
  return out;
}

// The kernel format a table key names. The kernel's generator maps each
// of these keys at jevents.py:389-404. The field is recorded under the
// kernel spelling, so the encoder resolves the row's key against a format
// the device publishes (FR-011, D-06).
auto kernel_spelling(const std::string_view key) noexcept -> std::string_view
{
  if (key == "CounterMask") {
    return "cmask";
  }
  if (key == "Invert") {
    return "inv";
  }
  if (key == "EdgeDetect") {
    return "edge";
  }
  if (key == "OffcoreRsp") {
    return "offcore_rsp";
  }
  if (key == "AnyThread") {
    return "any";
  }
  if (key == "PortMask") {
    return "ch_mask";
  }
  if (key == "FCMask") {
    return "fc_mask";
  }
  if (key == "RdWrMask") {
    return "rdwrmask";
  }
  if (key == "EnAllCores") {
    return "enallcores";
  }
  if (key == "EnAllSlices") {
    return "enallslices";
  }
  if (key == "SliceId") {
    return "sliceid";
  }
  if (key == "ThreadMask") {
    return "threadmask";
  }
  return {};
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
  // The row's own register filter, resolved through the kernel's index map
  // before the fields are read, because the value and the index are string
  // keys and neither encodes as a field of its own (FR-010, D-05).
  const auto filter = register_filter_of(attributes);
  for (auto [key, value] : attributes) {
    std::uint64_t number = 0;
    if (key == "EventCode") {
      // A pair such as "0xB7, 0xBB" names two event codes. The kernel's
      // generator takes the first code, and `first_index_of` is that rule.
      std::string_view text;
      if (value.get_string().get(text) == simdjson::SUCCESS) {
        if (first_index_of(text, number)) {
          entry.fields.emplace_back("event", number);
        }
      } else if (parse_scalar(value, number)) {
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
    } else if (carries_no_obligation(key)) {
      // Dropped above the encoder: it names no published format. The
      // register value sits beside it and is read ahead of this loop, so
      // the row never carries either one as a field (FR-010, D-05).
    } else if (carries_no_constraint(key)) {
      // A condition on when the counter is meaningful. Neither key names a
      // bit in an encoding register (FR-012, FR-013, D-06).
    } else if (is_register_value(key)) {
      // Read ahead of this loop and recorded under the format its index
      // names, never under its own name (FR-010, D-05).
    } else if (const auto kernel = kernel_spelling(key);
               !kernel.empty() && parse_scalar(value, number))
    {
      entry.fields.emplace_back(std::string(kernel), number);
    } else if (parse_scalar(value, number)) {
      // Any other integer-valued key is recorded under its own name, and
      // the encoder admits it only where the device publishes a format of
      // that name. A row needing a field the device does not publish
      // resolves to nothing and publishes `not_encodable` (FR-018).
      entry.fields.emplace_back(key, number);
    }
  }
  // The register filter reaches the encoder as a field under the format
  // its index names. A value of zero encodes no filter. A non-zero value
  // whose index names no format carries a field no device publishes, so
  // composition refuses the row and it publishes `not_encodable` (FR-010,
  // FR-011, D-05).
  if (filter.value != 0) {
    const std::string_view field_name =
        filter.format.empty() ? kUnnamedRegister : filter.format;
    entry.fields.emplace_back(std::string(field_name), filter.value);
  }
  // A register index and a register value never survive as fields of
  // their own: the value is read ahead of the loop and recorded under the
  // format its index names, and both keys are dropped before a field is
  // built (FR-010, D-05). The seam fixture asserts the recorded names.
  // Description precedence; AMD tables ship only "BriefDescription".
  for (const auto candidate : {
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

// The walk both loaders share: padded JSON bytes in, entries out. A
// document that fails to parse contributes nothing and total failure
// surfaces as an empty table (seam contract). No exceptions: every
// simdjson error becomes a skip.
// The check reports both that <simdjson.h> goes unused and that no header
// provides simdjson::padded_string. That header is the one providing the
// type, so the pair cannot both be satisfied, and the include stays because
// FR-013 makes this the only unit under source/counters/ that may include it.
// NOLINTNEXTLINE(misc-include-cleaner)
void parse_padded(const simdjson::padded_string& loaded,
                  std::vector<pmu_table_entry>& table)
{
  simdjson::dom::parser dom;
  simdjson::dom::element root;
  if (dom.parse(loaded).get(root) != simdjson::SUCCESS) {
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

void parse_json_file(const std::filesystem::path& path,
                     std::vector<pmu_table_entry>& table)
{
  const auto loaded = simdjson::padded_string::load(path.string());
  if (loaded.error() != simdjson::SUCCESS) {
    return;
  }
  parse_padded(loaded.value_unsafe(), table);
}

// The vendored tables reach the parser as bytes the library already owns
// (FR-036). The padded copy is what simdjson requires and what the file
// loader gets from the file system, so the walk cannot tell them apart.
void parse_json_bytes(std::string_view bytes,
                      std::vector<pmu_table_entry>& table)
{
  parse_padded(simdjson::padded_string {bytes}, table);
}

}  // namespace

// The vendored mapfile patterns use POSIX classes ([[:xdigit:]]);
// std::regex is ECMAScript, so translate every class name. libstdc++
// accepts the POSIX spelling in its ECMAScript grammar, so the mapfile
// rows match either way on this toolchain; libc++ and MSVC do not, and
// an untranslated class is a row that silently stops matching.
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
      // one-bracket span is not a class (`[[]` is a class holding
      // `[`), so the search starts one past the opening bracket.
      const std::size_t close = pattern.find("]]", i + 1);
      if (close != std::string::npos) {
        // POSIX wraps the class name in colons, so `[[:xdigit:]]` spans
        // `:xdigit:` between the brackets. A bare name sits inside the
        // brackets without colons.
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
  // LCOV_EXCL_LINE : the epilogue block of a by-value return; the `return`
  // above carries the call count.
}  // LCOV_EXCL_LINE

// The mapping-file key for `id`: "<vendor>-<family>-<MODEL>", with the
// family decimal and the model uppercase hex (kernel jevents
// convention).
auto mapfile_key(const pmu_ident& id) -> std::string
{
  char hex[8];
  std::snprintf(hex, sizeof(hex), "%X", static_cast<unsigned>(id.model));
  return id.vendor + '-' + std::to_string(id.family) + '-' + hex;
}

auto pmu_select_directory(std::istream& mapfile,
                          const pmu_ident& id) -> std::string
{
  // Format finding (the vendored file is truth): the columns are
  // "Family-model,Version,Filename,EventType"; the first is a
  // POSIX-class regex matched against the mapping-file key, and the
  // Filename column is a directory name relative to the mapfile. The
  // first matching row wins (FR-038).
  const std::string key = mapfile_key(id);
  std::string selected;
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
    // A row whose pattern does not compile throws `std::regex_error`:
    // the mapfile is regex-validated when it is re-pinned
    // (`tools/pmu_events/update_pmu_events.py`), so a row that fails to
    // compile here is corrupt data, and naming the corrupt row beats
    // selecting a table the mapfile does not name for this CPU.
    const std::regex pattern(to_lower(to_ecma(text.substr(0, first))),
                             std::regex::icase | std::regex::optimize);
    if (std::regex_match(key, pattern)) {
      selected = "arch/x86/";
      selected += filename;
      selected += '/';
      break;
    }
  }
  return selected;
}

auto pmu_select_directory(const pmu_ident& id) -> std::string
{
  static std::mutex cache_mutex;
  static std::map<std::string, std::string> cache;
  // The mapfile is compiled into the library as arch/x86's single file,
  // so selecting a directory reads no path (FR-036).
  const std::string key = mapfile_key(id);

  const std::scoped_lock lock(cache_mutex);
  if (const auto cached = cache.find(key); cached != cache.end()) {
    return cached->second;
  }

  std::string selected;
  // LCOV_EXCL_BR_START : coverage exclusion (T056): the arm where the
  // registry holds no arch/x86. The generator embeds that directory
  // unconditionally, naming the mapfile as its pattern, and fails the
  // build outright when the pattern matches nothing, so the entry is
  // present in every archive this library links (FR-036).
  if (const embedded_dir* dir = embedded_find_dir("arch/x86")) {
    std::istringstream mapfile {
        std::string {embedded_file_bytes(*dir, "mapfile.csv")}};
    selected = pmu_select_directory(mapfile, id);
  }
  // LCOV_EXCL_BR_STOP
  cache.emplace(key, selected);
  return selected;
}

auto pmu_load_table(const std::string& directory)
    -> const std::vector<pmu_table_entry>&
{
  // Once-per-directory lazy cache (FR-038).
  static std::mutex cache_mutex;
  static std::map<std::string, std::vector<pmu_table_entry>> cache;

  const std::scoped_lock lock(cache_mutex);
  if (const auto cached = cache.find(directory); cached != cache.end()) {
    return cached->second;
  }

  std::vector<pmu_table_entry> table;
  // A directory arrives with a trailing separator and the registry names
  // it without one (FR-036).
  std::string_view key {directory};
  if (key.ends_with('/')) {
    key.remove_suffix(1);
  }
  // A directory the library holds no bytes for parses nothing, which is
  // the seam contract: no files, empty table.
  if (const embedded_dir* dir = embedded_find_dir(key)) {
    for (std::size_t i = 0; i < dir->file_count; ++i) {
      // The registry's index is a generated C array and its count is the
      // bound on this loop, so the subscript is in range by construction.
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      parse_json_bytes(embedded_file_bytes(*dir, dir->files[i].name), table);
    }
  }
  return cache.emplace(directory, std::move(table)).first->second;
}

void pmu_parse_table_file(const std::string_view path,
                          std::vector<pmu_table_entry>& out)
{
  parse_json_file(std::filesystem::path(std::string(path)), out);
}

}  // namespace sg::counters::detail

#endif  // __linux__
