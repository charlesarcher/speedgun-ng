// ============================================================================
// Synthetic-fixture coverage for the Linux PMU provider's pure seam
// functions (T066, plan.md Test Plan; FR-037, FR-040, R-010, R-011).
//
// The plan requires the mapped-page protocol logic and the config
// encoder to be exercised with injected page and index inputs, so CI
// covers them without privileges. The kernel glue that feeds them
// (perf_event_open, the mapping, the instruction itself) runs for real
// on any host the kernel lets open a per-process user event, which
// includes the CI matrix; the arms no host can reach carry a recorded
// coverage exclusion with written justification at their own site
// (Principle VI, VIII).
//
// This is the one test in the repository that reaches a private seam
// header. Every other test drives the public surface, and that is the
// right default; a pure function behind a private header is otherwise
// reachable only from a host that owns the hardware, so the plan names
// this fixture as the way to cover it in CI.
//
// Frameworkless check()/fail() convention.
// ============================================================================

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <linux/perf_event.h>

#include "../source/counters/detail/pmu.hpp"

#ifndef SG_SEAM_TABLE_DIR
#  error "SG_SEAM_TABLE_DIR must name the fixture root (test/CMakeLists.txt)"
#endif

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "COUNTERS LINUX PMU SEAM TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

using sg::counters::availability;
using sg::counters::leaf_set;
using sg::counters::point_sink;
using sg::counters::read_mode;
using sg::counters::target;
using sg::counters::detail::alias_description;
using sg::counters::detail::fast_read_verdict;
using sg::counters::detail::format_range;
using sg::counters::detail::kRnpmcCounterWidth;
using sg::counters::detail::parse_attr;
using sg::counters::detail::parse_format_field;
using sg::counters::detail::pmu_compose_config;
using sg::counters::detail::pmu_device;
using sg::counters::detail::pmu_entry;
using sg::counters::detail::pmu_ident;
using sg::counters::detail::pmu_select_directory;
using sg::counters::detail::pmu_state;
using sg::counters::detail::pmu_table_entry;
using sg::counters::detail::table_description;
using sg::counters::detail::to_ecma;
using sg::counters::detail::to_hex;

// A synthetic perf page's published fields, as the protocol reads them
// (FR-040, R-011, T066). The page itself is a kernel mapping, so the
// fixture carries the fields the protocol gates on and nothing else.
struct synthetic_page
{
  std::uint32_t index = 0;
  std::uint16_t pmc_width = 0;
};

// The gates a synthetic page reaches, in the order the protocol applies
// them: the one-based index the instruction takes, the counter width the
// decode masks to, and the seqlock comparison that closes the window.
auto page_gate_scenario() -> void
{
  using sg::counters::detail::fast_counter_width;
  using sg::counters::detail::fast_index_valid;
  using sg::counters::detail::fast_pair_stable;
  using sg::counters::detail::kRnpmcMaxIndex;

  // One-based index validity over the page index alone: the kernel
  // publishes the index in the event page and the caller reads
  // `rdpmc(index - 1)`, so no second page and no slot id take part.
  check(fast_index_valid(1), "index 1 names the first counter");
  check(fast_index_valid(kRnpmcMaxIndex),
        "the highest operand bound is a valid index");
  check(
      !fast_index_valid(0),
      "index 0 reports no usable counter and falls back to the group " "read");
  check(!fast_index_valid(kRnpmcMaxIndex + 1),
        "an index past the operand bound is refused");

  // The width the decode masks to: the published width, or the fallback
  // when the page publishes none. A host whose counters are wider or
  // narrower than the fallback is read at the width it publishes.
  check(fast_counter_width(0) == kRnpmcCounterWidth,
        "a page publishing no width is read at the fallback width");
  check(fast_counter_width(32) == 32,
        "a published width is the width the decode masks to");
  check(fast_counter_width(64) == 64,
        "a full-width published width is taken as published");

  // The seqlock comparison, both directions.
  check(fast_pair_stable(11, 11), "an unmoved sequence is stable");
  check(!fast_pair_stable(11, 12), "a moved sequence is not stable");

  // The three gates compose with the decode over one synthetic page, so
  // the whole protocol is exercised without a mapping.
  const synthetic_page page {.index = 3, .pmc_width = 0};
  std::uint64_t value = 0;
  check(fast_index_valid(page.index)
            && sg::counters::detail::fast_decode(
                   11,
                   11,
                   page.index,
                   1,
                   0xabc,
                   0,
                   fast_counter_width(page.pmc_width),
                   value)
                == fast_read_verdict::ok
            && value == 0xabc,
        "a page with a valid index decodes to the raw read");
}

// The probe's verdict over the three fields the mapped event page
// publishes, every arm, so the catalog's refusal sentence is covered
// without a host the kernel refuses (FR-023, R-011).
auto probe_verdict_scenario() -> void
{
  using sg::counters::detail::fast_probe_allows;

  std::string refusal = "unset";
  check(fast_probe_allows(true, 1, refusal) && refusal.empty(),
        "a page granting the capability with a usable index allows the "
        "mapped-page read and clears the refusal");
  check(!fast_probe_allows(false, 1, refusal)
            && refusal.find("cap_user_rdpmc") != std::string::npos,
        "a page publishing no capability refuses and names the bit it "
        "looked for");
  check(!fast_probe_allows(true, 0, refusal)
            && refusal.find("counter index 0") != std::string::npos,
        "a page indexing no counter refuses and names the index");
  check(!fast_probe_allows(false, 0, refusal)
            && refusal.find("cap_user_rdpmc") != std::string::npos,
        "the capability gate is the first one the protocol states");
}

using field_map =
    std::vector<std::pair<std::string, std::vector<format_range>>>;

// The sysfs format spelling: a field name, a colon, then comma
// separated low-high bit ranges (FR-037).
auto format_scenario() -> void
{
  std::vector<format_range> ranges;
  check(parse_format_field("config:0-7,32-35", ranges) && ranges.size() == 2
            && ranges[0].config_word == 0 && ranges[0].low == 0
            && ranges[0].high == 7 && ranges[1].low == 32
            && ranges[1].high == 35,
        "a two-range config field parses both ranges in order");
  check(parse_format_field("config1:16-31", ranges) && ranges.size() == 1
            && ranges[0].config_word == 1 && ranges[0].low == 16
            && ranges[0].high == 31,
        "the trailing digits of a field name select its config word");
  check(parse_format_field("config2:0", ranges) && ranges.size() == 1
            && ranges[0].config_word == 2 && ranges[0].low == 0
            && ranges[0].high == 0,
        "a single-bit range parses");
  check(parse_format_field("inv:23", ranges) && ranges.size() == 1
            && ranges[0].config_word == 0 && ranges[0].low == 23
            && ranges[0].high == 23,
        "a field name without digits names config word 0");

  check(!parse_format_field("", ranges), "an empty file parses to nothing");
  check(!parse_format_field("noseparator", ranges),
        "a file with no colon describes no config bits");
  check(!parse_format_field("config:", ranges),
        "a field with no ranges parses to nothing");
  check(!parse_format_field("config:,", ranges),
        "a trailing separator parses to nothing");
  check(!parse_format_field("config:x-7", ranges),
        "a non-numeric low bound is rejected");
  check(!parse_format_field("config:7-1", ranges),
        "an inverted range is rejected");
  check(!parse_format_field("config:0-123", ranges),
        "a bound wider than two digits is rejected");
  check(!parse_format_field("config:-7", ranges),
        "a field with no low bound parses to nothing");
  check(!parse_format_field("config:7-", ranges),
        "a field with no high bound parses to nothing");
  // The config word a field name selects skips a leading run of
  // lowercase letters, so a name with none, or with a non-digit after
  // them, names word 0 (FR-037).
  check(parse_format_field("_x:0-3", ranges) && ranges.size() == 1
            && ranges[0].config_word == 0 && ranges[0].high == 3,
        "a field name with no lowercase prefix names config word 0");
  check(parse_format_field("Config:0-3", ranges) && ranges.size() == 1
            && ranges[0].config_word == 0 && ranges[0].high == 3,
        "an uppercase leading letter ends the prefix and names word 0");
  check(parse_format_field("config1x:0-3", ranges) && ranges.size() == 1
            && ranges[0].config_word == 0,
        "a non-digit inside the index falls back to config word 0");
  check(parse_format_field("config:0-3", ranges) && ranges.size() == 1
            && ranges[0].config_word == 0,
        "a field name with no index at all names config word 0");
  check(parse_format_field("c:0-3", ranges) && ranges.size() == 1
            && ranges[0].config_word == 0,
        "a one-letter field name consumes its whole name as the prefix");
}

// The value split across the ranges a field publishes, and the two
// refusals that keep a partial encoding from being composed (FR-037).
auto compose_scenario() -> void
{
  const field_map cpu_format {{"event", {{0, 0, 7}, {0, 32, 35}}},
                              {"umask", {{0, 8, 15}}}};
  std::vector<std::pair<int, std::uint64_t>> words;

  check(pmu_compose_config({{"event", 0xc0}}, cpu_format, words)
            && words.size() == 1 && words[0].first == 0
            && words[0].second == 0xc0,
        "a value fitting the first range lands in the low config bits");
  check(pmu_compose_config({{"event", 0x120}}, cpu_format, words)
            && words.size() == 1 && words[0].second == 0x100000020ULL,
        "a value wider than one range splits across the ranges in order");
  check(
      pmu_compose_config({{"event", 0xc0}, {"umask", 0x03}}, cpu_format, words)
          && words.size() == 1 && words[0].second == 0x3c0,
      "two fields sharing a config word merge into one value");
  check(pmu_compose_config({{"umask", 0xff}}, cpu_format, words)
            && words[0].second == 0xff00,
        "a field with no low range places its bits where the kernel reads "
        "them");
  check(pmu_compose_config({}, cpu_format, words) && words.empty(),
        "an entry with no semantic fields composes to no config");

  // The kernel publishes no layout for this field, so the whole
  // composition is refused and nothing is half-written.
  words.push_back({0, 0xdeadbeef});
  check(!pmu_compose_config({{"nosuchfield", 1}}, cpu_format, words),
        "a field the kernel does not publish refuses the composition");
  check(words.size() == 1 && words[0].second == 0xdeadbeef,
        "a refused composition leaves the caller's words untouched");
  check(!pmu_compose_config({{"event", 0xffffff}}, cpu_format, words),
        "a value wider than the published ranges span is refused");
  check(pmu_compose_config(
            {{"event", 1}}, field_map {{"event", {{0, 0, 63}}}}, words)
            && words.size() == 1 && words[0].second == 1,
        "a range spanning the full config word accepts the whole value");
  check(!pmu_compose_config({{"event", 2}},
                            field_map {{"event", {{0, 0, 63}, {0, 64, 64}}}},
                            words),
        "a value that overruns a full-word range is refused");
  check(!pmu_compose_config({{"event", 1}}, field_map {{"event", {}}}, words),
        "a field with an empty range list is refused");
  // A field split across two config words places each range in the word
  // the kernel published it in (FR-037).
  const field_map split {{"event", {{0, 0, 7}, {1, 32, 35}}}};
  check(pmu_compose_config({{"event", 0x120}}, split, words)
            && words.size() == 2 && words[0].first == 0
            && words[0].second == 0x20 && words[1].first == 1
            && words[1].second == 0x100000000ULL,
        "a field split across two config words lands one range in each");
}

// The decode half of the mapped-page protocol, over injected values
// (FR-040, R-011): the capability gate runs first, then the one-based
// index gate, then the sequence comparison closes the window, and the
// kernel offset and counter width are applied last.
auto decode_scenario() -> void
{
  std::uint64_t value = 0;
  check(sg::counters::detail::fast_decode(
            7, 7, 1, 1, 0x1234, 0, kRnpmcCounterWidth, value)
                == fast_read_verdict::ok
            && value == 0x1234,
        "a stable sequence with the capability granted decodes the read");
  check(
      sg::counters::detail::fast_decode(
          7, 7, 1, 0, 0x1234, 0, kRnpmcCounterWidth, value)
          == fast_read_verdict::not_allowed,
      "a page with no read capability refuses before the index is " "consulte"
                                                                    "d");
  check(sg::counters::detail::fast_decode(7, 7, 0, 1, 0x1234, 0,
                                          kRnpmcCounterWidth, value)
            == fast_read_verdict::not_allowed,
        "a page indexing no counter refuses the read, so the instruction is "
        "never issued for it");
  value = 0xdeadbeef;
  check(sg::counters::detail::fast_decode(
            7, 9, 1, 1, 0x1234, 0, kRnpmcCounterWidth, value)
                == fast_read_verdict::unstable
            && value == 0xdeadbeef,
        "a sequence that moved reports instability and writes no value");
  check(sg::counters::detail::fast_decode(
            7, 7, 1, 1, 0x10, -16, kRnpmcCounterWidth, value)
                == fast_read_verdict::ok
            && value == 0,
        "a negative kernel offset subtracts from the raw read");
  check(sg::counters::detail::fast_decode(
            7, 7, 1, 1, 0x10, 16, kRnpmcCounterWidth, value)
                == fast_read_verdict::ok
            && value == 0x20,
        "a positive kernel offset adds to the raw read");
  check(sg::counters::detail::fast_decode(7, 7, 1, 1, 0x1234, 0, 8, value)
                == fast_read_verdict::ok
            && value == 0x34,
        "the value is masked to the counter width the kernel publishes");
}

// The open-refusal arm of `fast_context_open`, reached without a host
// the kernel refuses: `perf_event_open` rejects a PMU type no kernel
// publishes, and the refusal the catalog would disclose is the sentence
// the open wrote out (FR-023, R-011).
auto context_open_refusal_scenario() -> void
{
  std::string refusal;
  const target where {};
  auto context =
      sg::counters::detail::fast_context_open(999999, 0, where, &refusal);
  check(!context, "an event type no kernel publishes opens no context");
  check(refusal.find("perf_event_open") != std::string::npos,
        "the refusal names the syscall the kernel refused");
  // The refusal is optional, because a window open names no catalog fact:
  // the window reports the refusal to its caller by refusing, and only the
  // probe has a catalog sentence to fill.
  check(sg::counters::detail::fast_context_open(999999, 0, where) == nullptr,
        "the same refusal is reported by refusing when no sentence is asked "
        "for");
  // Closing a context that owns nothing touches nothing, so a window that
  // refuses mid-open leaves no descriptor and no mapping behind.
  sg::counters::detail::fast_context empty;
  sg::counters::detail::fast_context_close(empty);
  check(empty.fd == -1 && empty.map == nullptr,
        "closing a context that owns nothing is a no-op (FR-040)");
}

// The mapping file selects the architecture directory for a CPU
// identification, first matching row wins, and a row whose pattern names
// no match selects nothing (FR-038).
auto mapfile_scenario() -> void
{
  const auto identified = sg::counters::detail::pmu_ident_current();
  std::printf("seam ident: vendor='%s' family=%d model=%d\n",
              identified.vendor.c_str(),
              identified.family,
              identified.model);
  const auto directory = pmu_select_directory(identified);
  std::printf("seam selected directory: '%s'\n", directory.c_str());
  if (directory.empty()) {
    std::printf("seam: the mapping file matched no row for this CPU; the "
                "vendored table is unavailable, which is a catalog fact "
                "(FR-038)\n");
    return;
  }
  check(directory.starts_with("arch/x86/") && directory.ends_with("/"),
        "a matched row names an architecture directory under the table root");

  // The directory and the identification are cached once per process, so
  // the second selection of each is a cache hit (FR-038).
  check(pmu_select_directory(identified) == directory,
        "selecting the same identification twice yields the same directory");
  const auto& table = sg::counters::detail::pmu_load_table(directory);
  check(&sg::counters::detail::pmu_load_table(directory) == &table,
        "loading the same directory twice returns the same parsed table");
  std::printf("seam table: %zu entries\n", table.size());
  check(!table.empty(), "the selected directory parsed to at least one entry");
  std::size_t described = 0;
  for (const auto& entry : table) {
    if (!entry.name.empty()) {
      ++described;
    }
  }
  check(described == table.size(), "every parsed table row carries a name");
  // A directory that does not exist yields an empty table. A missing
  // architecture degrades to a reduced catalog; it is a catalog fact.
  const std::vector<pmu_table_entry> absent =
      sg::counters::detail::pmu_load_table("arch/x86/no-such-directory/");
  check(absent.empty(),
        "an absent architecture directory yields an empty " "table (FR-038)");

  const pmu_ident unknown {
      .vendor = "NoSuchVendorXXXXX", .family = 999, .model = 999};
  check(pmu_select_directory(unknown).empty(),
        "an identification no row matches selects no directory (FR-038)");
}

// The mapping file's patterns use POSIX character classes and std::regex
// is ECMAScript, so every class name is translated before the row is
// compiled (FR-038, T066). The pinned mapfile spells one class,
// `[[:xdigit:]]`, and libstdc++ accepts it untranslated, so a fixture is
// the only way the translation itself is measured.
auto ecma_scenario() -> void
{
  check(to_ecma("[[:xdigit:]]") == "[0-9A-Fa-f]",
        "the hexadecimal class translates to its ECMAScript spelling");
  check(to_ecma("[[:digit:]]+") == "[0-9]+",
        "a class carrying a quantifier translates once");
  check(to_ecma("authenticamd-26-([12467][[:xdigit:]]|[[:xdigit:]])")
            == "authenticamd-26-([12467][0-9A-Fa-f]|[0-9A-Fa-f])",
        "every class of one mapfile row translates");
  check(to_ecma("genuineintel-6-8f") == "genuineintel-6-8f",
        "a pattern with no bracket sequence is unchanged");
  check(to_ecma("[[") == "[[",
        "an unterminated bracket sequence stays literal");
  check(to_ecma("[[xdigit]]") == "[[xdigit]]",
        "a bracket sequence with no POSIX colons stays literal");
  check(to_ecma("[[:nonesuch:]]") == "[[:nonesuch:]]",
        "a class name no table lists stays literal");
  check(to_ecma("[[::]]") == "[[::]]",
        "a bracket sequence carrying no class name stays literal");
}

// The mapping-file row guards over a synthetic mapping file: the header
// is skipped, a row that does not carry the three leading columns is
// skipped, the first matching row wins, and a row whose pattern does not
// compile throws (FR-038, T066). The pinned mapfile carries four columns
// in every row, so only a fixture reaches the refusals.
auto mapfile_row_scenario() -> void
{
  const pmu_ident ident {.vendor = "GenuineIntel", .family = 6, .model = 0x8f};
  std::istringstream rows("Family-model,Version,Filename,EventType\n"
                          "a-row-with-no-column\n"
                          "GenuineIntel-6-8F,v1\n"
                          "GenuineIntel-6-8F,v1,\n"
                          "GenuineIntel-6-8F,v1,,core\n"
                          "GenuineIntel-6-AB,v1,synthetic,core\n"
                          "GenuineIntel-6-8F,v1,synthetic,core\n"
                          "GenuineIntel-6-8F,v1,later,core\n");
  check(pmu_select_directory(rows, ident) == "arch/x86/synthetic/",
        "the first row matching the identification names the directory");

  std::istringstream unmatched("Family-model,Version,Filename,EventType\n"
                               "GenuineIntel-6-AB,v1,synthetic,core\n");
  check(pmu_select_directory(unmatched, ident).empty(),
        "a mapping file whose rows name no match selects no directory");

  std::istringstream corrupt("Family-model,Version,Filename,EventType\n"
                             "GenuineIntel-6-[,v1,synthetic,core\n");
  bool threw = false;
  try {
    static_cast<void>(pmu_select_directory(corrupt, ident));
  } catch (const std::regex_error&) {
    threw = true;
  }
  check(threw, "a row whose pattern does not compile throws std::regex_error");
}

// Writes one synthetic event-table file into the fixture root and
// parses it through the seam. Every tree is generated here, so the
// fixture is not a second source of truth beside `external/pmu-events/`
// (the `pmu-events-gate-fixture` precedent, T057).
auto write_fixture(const char* name, const std::string_view body) -> std::string
{
  const std::filesystem::path root(SG_SEAM_TABLE_DIR);
  std::error_code code;
  std::filesystem::create_directories(root, code);
  const auto path = root / name;
  std::ofstream file(path, std::ios::binary | std::ios::trunc);
  file << body;
  file.close();
  if (!file) {
    fail("the synthetic table fixture is writable");
  }
  return path.string();
}

// The field list of one decoded event_attr text or one parsed entry,
// for the equality assertions below.
auto fields_of(const std::vector<std::pair<std::string, std::uint64_t>>& fields)
    -> std::string
{
  std::string out;
  for (const auto& [name, value] : fields) {
    if (!out.empty()) {
      out += ',';
    }
    out += name;
    out += '=';
    out += std::to_string(value);
  }
  return out;
}

auto fields_of(const pmu_table_entry& entry) -> std::string
{
  return fields_of(entry.fields);
}

auto find_row(const std::vector<pmu_table_entry>& table,
              const std::string& name) -> const pmu_table_entry*
{
  for (const auto& entry : table) {
    if (entry.name == name) {
      return &entry;
    }
  }
  return nullptr;
}

// One vendored table row for the description assertions below. The
// seam type declares four members and the build treats a partial
// initializer as an error, so the row is built here.
auto table_row(std::string name,
               std::string description,
               std::string unit = "none",
               std::vector<std::pair<std::string, std::uint64_t>> fields = {})
    -> pmu_table_entry
{
  return pmu_table_entry {.name = std::move(name),
                          .description = std::move(description),
                          .unit = std::move(unit),
                          .fields = std::move(fields)};
}

// The sysfs event_attr decoder, the hexadecimal event code, and the two
// catalog descriptions: every catalog entry describes itself, from the
// kernel's own text or from the vendored table's (FR-017, FR-037, T066).
// The kernel publishes the accepting texts and this host decodes all of
// them, so what a fixture adds is every refusal below.
auto attr_text_scenario() -> void
{
  check(fields_of(parse_attr("event=0xc0,umask=0x01")) == "event=192,umask=1",
        "a two-field event_attr text decodes in order");
  check(fields_of(parse_attr("event=0xc0,,umask=0x01")) == "event=192,umask=1",
        "an empty piece between two fields decodes to no field");
  check(fields_of(parse_attr("event=0Xc0")) == "event=192",
        "an uppercase hexadecimal prefix decodes like a lowercase one");
  check(fields_of(parse_attr("event=0xC0")) == "event=192",
        "an uppercase hexadecimal digit decodes like a lowercase one");
  check(fields_of(parse_attr("event=192")) == "event=192",
        "a decimal value decodes in base ten");
  check(fields_of(parse_attr("")).empty(), "an empty text decodes to nothing");
  check(fields_of(parse_attr("nodelimiter")).empty(),
        "a piece with no '=' names no field");
  check(fields_of(parse_attr("=0x01")).empty(),
        "a piece with no field name names no field");
  check(fields_of(parse_attr("event=")).empty(),
        "an empty value names no field");
  check(fields_of(parse_attr("event=0x")).empty(),
        "a hexadecimal prefix with no digits names no field");
  check(fields_of(parse_attr("event=0xzz")).empty(),
        "a non-hexadecimal digit names no field");
  check(fields_of(parse_attr("event=0xc0z")).empty(),
        "a value whose digits stop being hexadecimal names no field");
  check(fields_of(parse_attr("event=0x00")).empty(),
        "a value decoding to zero names no field");

  // The event code as the catalog spells it, with no leading zero nibble.
  check(to_hex(0) == "0x0", "the event code zero reads as 0x0");
  check(to_hex(1) == "0x1", "a one-nibble event code keeps its one digit");
  check(to_hex(0x3c) == "0x3c", "an event code reads as its own digits");
  check(to_hex(0xc0) == "0xc0",
        "an event code with a zero low nibble keeps the high digit");
  check(to_hex(0xabc) == "0xabc",
        "a wide event code drops its leading zero nibbles");
  check(to_hex(0xffffffffffffffffULL) == "0xffffffffffffffff",
        "the widest event code reads all sixteen digits");

  // A kernel alias describes itself with the text the kernel publishes,
  // and names itself when the kernel publishes none (FR-037).
  check(alias_description("cpu/cycles", "event=0x3c")
            == "kernel event configuration: event=0x3c",
        "an alias repeats the event_attr text the kernel publishes");
  check(alias_description("cpu/cycles", "")
            == "kernel event 'cpu/cycles'; the kernel publishes no "
               "configuration text for this alias",
        "an alias the kernel publishes no text for names itself instead");

  // A vendored row uses the table's prose, names its event code when the
  // table carries no prose, and rides the Unit scope label (FR-017).
  check(table_description(table_row("ev", "prose")) == "prose",
        "a row carrying prose describes itself with that prose");
  check(table_description(table_row("ev", ""))
            == "hardware event 'ev'; the vendored kernel table carries no "
               "description",
        "a row carrying no prose names itself and the missing description");
  check(table_description(table_row("ev", "", "none", {{"event", 0xc0}}))
            == "hardware event 'ev'; the vendored kernel table carries no "
               "description (event code 0xc0)",
        "a row carrying no prose names its event code");
  check(table_description(table_row("ev", "", "none", {{"umask", 1}}))
            == "hardware event 'ev'; the vendored kernel table carries no "
               "description",
        "a row whose only field is no event code names no event code");
  check(table_description(table_row("ev", "prose", "DFPMC"))
            == "prose [table scope: DFPMC]",
        "a table scope label rides along after the row's own prose");
  check(table_description(table_row("ev", "prose", "none")) == "prose",
        "the default unit names no table scope");
}

// The array shape the pinned tree carries, plus every scalar spelling the
// parser accepts and the rows it must skip (FR-038, R-010, T066). The
// pinned tree spells every semantic value as a string and never names a
// unit outside its metric files, so the numeric, negative, unparsable,
// metric, unit, and description-precedence rows are reachable only from
// a fixture.
auto table_array_scenario() -> void
{
  const std::string path = write_fixture("array_shapes.json", R"([
  {"EventName":"ev_hex","EventCode":"0x3c","UMask":"0x01",
   "BriefDescription":"hex string values"},
  {"EventName":"ev_upper_hex","EventCode":"0X3C",
   "BriefDescription":"uppercase hex prefix"},
  {"EventName":"ev_dec","EventCode":"61","UMask":"2",
   "BriefDescription":"decimal string values"},
  {"EventName":"ev_num","EventCode":60,"UMask":1,
   "Description":"numeric values"},
  {"EventName":"ev_neg","EventCode":-1,"UMask":-2,
   "BriefDescription":"a negative scalar carries no config"},
  {"EventName":"ev_unparsable","EventCode":"0xzz","UMask":"",
   "BriefDescription":"an unparsable scalar carries no config"},
  {"EventName":"ev_partial","EventCode":"0x3cg","UMask":"0x1g",
   "BriefDescription":"a partially parsed scalar carries no config"},
  {"EventName":"ev_free_key","EventCode":"0x01","UMask":"0x02",
   "UMASK_EXT":128,"CounterMask":3,"Edge":1,"Unit":"DFPMC",
   "BriefDescription":"extended mask and free numeric keys"},
  {"EventName":"ev_umask_lower","EventCode":"0x01","Umask":"0x04",
   "BriefDescription":"the lowercase mask spelling"},
  {"EventName":"ev_public","EventCode":"0x01","Description":"full prose",
   "PublicDescription":"public prose","BriefDescription":"brief prose",
   "Unit":"iMC"},
  {"EventName":"ev_brief_only","EventCode":"0x01",
   "PublicDescription":"public prose","BriefDescription":"brief prose"},
  {"EventName":"ev_no_prose","EventCode":"0x01"},
  {"EventName":"ev_desc_number","EventCode":"0x01","Description":7,
   "BriefDescription":"brief prose"},
  {"EventName":"ev_unit_number","EventCode":"0x01","Unit":7,
   "BriefDescription":"scope label, non-string"},
  {"EventName":"ev_unit_missing","EventCode":"0x01","Unit":null,
   "BriefDescription":"scope label, null"},
  {"EventName":"ev_brief_number","EventCode":"0x01","BriefDescription":true},
  {"EventName":"ev_free_bool","EventCode":"0x01","CounterMask":true,
   "BriefDescription":"a boolean free key carries no config"},
  {"EventName":"ev_free_null","EventCode":"0x01","Edge":null,
   "BriefDescription":"a null free key carries no config"},
  {"EventName":"ev_ext_bad","EventCode":"0x01","UMASK_EXT":true,
   "BriefDescription":"a non-scalar extended mask carries no config"},
  {"EventName":7,"EventCode":"0x01","BriefDescription":"a numeric event name"},
  {"EventName":true,"EventCode":"0x01",
   "BriefDescription":"a boolean event name"},
  {"EventName":"metric_expr","EventCode":"0x01","MetricName":"m",
   "MetricExpr":"a+b","BriefDescription":"a metric row"},
  {"EventName":"metric_name","EventCode":"0x01","MetricName":"m2",
   "BriefDescription":"a metric row"},
  "not an object",
  {"NoEventName":1}
])");

  std::vector<pmu_table_entry> table;
  sg::counters::detail::pmu_parse_table_file(path, table);

  const auto* hex = find_row(table, "ev_hex");
  check(hex != nullptr && fields_of(*hex) == "event=60,umask=1"
            && hex->description == "hex string values",
        "hex-string event code and mask parse to their config values");
  const auto* upper = find_row(table, "ev_upper_hex");
  check(upper != nullptr && fields_of(*upper) == "event=60",
        "an uppercase hex prefix parses like a lowercase one");
  const auto* dec = find_row(table, "ev_dec");
  check(dec != nullptr && fields_of(*dec) == "event=61,umask=2",
        "decimal-string values parse in base ten");
  const auto* numeric = find_row(table, "ev_num");
  check(numeric != nullptr && fields_of(*numeric) == "event=60,umask=1",
        "a JSON number parses to the same config value as its hex spelling");
  const auto* negative = find_row(table, "ev_neg");
  check(negative != nullptr && negative->fields.empty(),
        "a negative JSON number carries no config semantic");
  const auto* unparsable = find_row(table, "ev_unparsable");
  check(unparsable != nullptr && unparsable->fields.empty(),
        "an unparsable scalar string carries no config semantic");
  const auto* partial = find_row(table, "ev_partial");
  check(partial != nullptr && partial->fields.empty(),
        "a scalar that parses only part of its digits carries no config");
  const auto* free_key = find_row(table, "ev_free_key");
  check(free_key != nullptr
            && fields_of(*free_key)
                == "event=1,umask=2,umask_ext=128,CounterMask=3,Edge=1"
            && free_key->unit == "DFPMC",
        "the extended mask, free numeric keys, and the scope label parse");
  const auto* lower = find_row(table, "ev_umask_lower");
  check(lower != nullptr && fields_of(*lower) == "event=1,umask=4",
        "the lowercase mask spelling maps to the umask field");
  const auto* full = find_row(table, "ev_public");
  check(full != nullptr && full->description == "full prose"
            && full->unit == "iMC",
        "the description precedence starts at Description");
  const auto* brief = find_row(table, "ev_brief_only");
  check(brief != nullptr && brief->description == "public prose",
        "the description precedence falls through to PublicDescription");
  const auto* bare = find_row(table, "ev_no_prose");
  check(bare != nullptr && bare->description.empty() && bare->unit == "none",
        "a row with no description and no unit keeps the table defaults");
  const auto* desc_number = find_row(table, "ev_desc_number");
  check(desc_number != nullptr && desc_number->description == "brief prose",
        "a non-string description candidate falls through to the next");
  const auto* unit_number = find_row(table, "ev_unit_number");
  check(unit_number != nullptr && unit_number->unit == "none",
        "a non-string unit leaves the default unit in place");
  const auto* unit_missing = find_row(table, "ev_unit_missing");
  check(unit_missing != nullptr && unit_missing->unit == "none",
        "a null unit leaves the default unit in place");
  const auto* brief_number = find_row(table, "ev_brief_number");
  check(brief_number != nullptr && brief_number->description.empty(),
        "a row whose every description candidate is non-string has none");
  check(find_row(table, "metric_expr") == nullptr
            && find_row(table, "metric_name") == nullptr,
        "a metric-definition row is catalog data, never a countable");
  check(table.size() == 19,
        "only the rows carrying an event name enter the table");

  // A file the parser cannot read adds nothing, and says so by leaving
  // the caller's table untouched.
  const std::size_t before = table.size();
  sg::counters::detail::pmu_parse_table_file(
      write_fixture("not_json.txt", "{ this is not json"), table);
  check(table.size() == before,
        "a file that does not parse contributes no entry");
  sg::counters::detail::pmu_parse_table_file(write_fixture("scalar.json", "42"),
                                             table);
  check(table.size() == before,
        "a file that is neither an array nor an object contributes no entry");
  std::vector<pmu_table_entry> absent;
  sg::counters::detail::pmu_parse_table_file(
      (std::filesystem::path(SG_SEAM_TABLE_DIR) / "no-such-file.json").string(),
      absent);
  check(absent.empty(), "a file that cannot be read contributes no entry");
}

// The object shape: event name to attributes, the other spelling the
// kernel tables use (FR-038, R-010, T066).
auto table_object_scenario() -> void
{
  const std::string path = write_fixture("object_shapes.json", R"({
  "obj_hex": {"EventCode":"0x01","BriefDescription":"object shape, hex"},
  "obj_num": {"EventCode":7,"Unit":"uncore_imc","Description":"number"},
  "obj_metric": {"EventCode":"0x02","MetricName":"m"},
  "obj_text": "not an object"
})");

  std::vector<pmu_table_entry> table;
  sg::counters::detail::pmu_parse_table_file(path, table);
  check(table.size() == 2, "an object-shaped file yields its countable rows");
  const auto* hex = find_row(table, "obj_hex");
  check(hex != nullptr && fields_of(*hex) == "event=1"
            && hex->description == "object shape, hex",
        "an object-shaped row parses like an array-shaped one");
  const auto* num = find_row(table, "obj_num");
  check(num != nullptr && fields_of(*num) == "event=7"
            && num->unit == "uncore_imc" && num->description == "number",
        "an object-shaped row carries its unit and description");
}

// A synthetic catalog over the core PMU type, built from the seam's own
// aggregates so the resolve, layout, and open paths are reachable without
// a catalog (FR-024, FR-041, T066). The kernel is the only part a fixture
// cannot supply, so every assertion below holds the seam to the verdict
// the same kernel gives the availability probe, the pattern
// `counters_pmu_test` uses for the catalog (T094).
auto synthetic_state() -> pmu_state
{
  pmu_state state;
  pmu_device device;
  device.path = "cpu";
  device.type = PERF_TYPE_HARDWARE;
  device.has_time_pair = true;
  const auto entry = [](const std::string& name,
                        const std::vector<std::pair<int, std::uint64_t>>& words,
                        const availability state_value)
  {
    pmu_entry one;
    one.name = name;
    one.description = "synthetic " + name;
    one.words = words;
    one.avail = state_value;
    one.mode = read_mode::syscall;
    return one;
  };
  device.entries.push_back(entry(
      "work", {{0, PERF_COUNT_HW_INSTRUCTIONS}}, availability::countable));
  device.entries.push_back(
      entry("cycle", {{0, PERF_COUNT_HW_CPU_CYCLES}}, availability::countable));
  // Config words 1 and 2 reach the fill_attr switch arms a core-PMU type
  // never carries.
  device.entries.push_back(entry("word1", {{1, 1}}, availability::countable));
  device.entries.push_back(entry("word2", {{2, 1}}, availability::countable));
  device.entries.push_back(entry(
      "fast", {{0, PERF_COUNT_HW_INSTRUCTIONS}}, availability::countable));
  device.entries.back().mode = read_mode::fast_rdpmc;
  device.entries.push_back(
      entry("blocked", {{0, 1}}, availability::permission_blocked));
  for (const char* pair : {"enabled", "running"}) {
    pmu_entry one;
    one.name = pair;
    one.description = "synthetic time pair";
    one.is_time_pair = true;
    one.avail = availability::countable;
    one.mode = read_mode::syscall;
    device.entries.push_back(std::move(one));
  }
  state.devices.push_back(std::move(device));
  return state;
}

auto leaf_set_of(std::vector<std::string> addresses) -> leaf_set
{
  return leaf_set {.addresses = std::move(addresses)};
}

// The resolve and layout refusals, each decided before the provider
// touches the kernel (FR-024, FR-041).
auto window_refusal_scenario() -> void
{
  const pmu_state state = synthetic_state();
  const target where {};
  check(sg::counters::detail::pmu_open_window(state, leaf_set_of({}), where)
            == nullptr,
        "an empty leaf set opens no window");
  check(sg::counters::detail::pmu_open_window(
            state, leaf_set_of({"nodelimiter"}), where)
            == nullptr,
        "a leaf address naming no device opens no window");
  check(sg::counters::detail::pmu_open_window(
            state, leaf_set_of({"nosuchdevice/work"}), where)
            == nullptr,
        "a leaf address naming an undiscovered device opens no window");
  check(sg::counters::detail::pmu_open_window(
            state, leaf_set_of({"cpu/nosuchleaf"}), where)
            == nullptr,
        "a leaf name the catalog does not publish opens no window");
  check(sg::counters::detail::pmu_open_window(
            state, leaf_set_of({"cpu/blocked"}), where)
            == nullptr,
        "a leaf the catalog reports permission_blocked opens no window");
  // The time pair is no device member, so a leaf set carrying only the
  // pair needs no group and no leader.
  check(sg::counters::detail::pmu_open_window(
            state, leaf_set_of({"cpu/enabled", "cpu/running"}), where)
            == nullptr,
        "a leaf set carrying only the enabled/running pair opens no group");
  check(sg::counters::detail::pmu_open_fast_window(
            state, leaf_set_of({"cpu/enabled"}), where)
            == nullptr,
        "a fast window over the pair alone holds no member context");
  check(sg::counters::detail::pmu_open_fast_window(
            state, leaf_set_of({"nodelimiter"}), where)
            == nullptr,
        "a fast window over a leaf address naming no device opens nothing");
}

// The syscall-mode group open, one leaf at a time so the config a leaf
// carries is the only variable. The seam must agree with the
// availability probe on the same config words, which is what proves the
// group open composes what the probe did.
auto group_open_scenario() -> void
{
  const pmu_state state = synthetic_state();
  const target where {};
  const std::vector<
      std::pair<std::string, std::vector<std::pair<int, std::uint64_t>>>>
      cases {{"cpu/work", {{0, PERF_COUNT_HW_INSTRUCTIONS}}},
             {"cpu/word1", {{1, 1}}},
             {"cpu/word2", {{2, 1}}},
             {"cpu/cycle", {{0, PERF_COUNT_HW_CPU_CYCLES}}}};
  std::size_t granted_count = 0;
  for (const auto& [leaf, words] : cases) {
    const bool granted =
        sg::counters::detail::pmu_probe(state.devices[0].type, words)
        == availability::countable;
    const auto window = sg::counters::detail::pmu_open_window(
        state, leaf_set_of({leaf}), where);
    check((window != nullptr) == granted,
          "a group open succeeds exactly when the availability probe "
          "grants the same config words");
    granted_count += granted ? 1U : 0U;
  }
  // A member beside the leader's enabled leaf: one group read delivers
  // the member and the pair, in the requested order (FR-026, FR-041).
  const auto pair = sg::counters::detail::pmu_open_window(
      state, leaf_set_of({"cpu/cycle", "cpu/enabled", "cpu/running"}), where);
  const bool pair_granted =
      sg::counters::detail::pmu_probe(state.devices[0].type,
                                      {{0, PERF_COUNT_HW_CPU_CYCLES}})
      == availability::countable;
  check(
      (pair != nullptr) == pair_granted,
      "a member beside the enabled leaf agrees with the availability " "probe");
  if (pair != nullptr) {
    // Two reads of the same group, into a two-column, two-row block: the
    // committed row carries the two points, the row this action did not
    // commit stays untouched, and no counter ever decreases across the
    // two reads (FR-013, FR-026, FR-041, FR-047).
    std::uint64_t first[6] = {0, 0, 0, 0, 0, 0};
    point_sink one {first, 3, 2, 0};
    pair->read_points(one);
    one.check_action();
    std::uint64_t second[6] = {0, 0, 0, 0, 0, 0};
    point_sink two {second, 3, 2, 0};
    pair->read_points(two);
    two.check_action();
    check(first[1] == 0 && first[3] == 0 && first[5] == 0
              && second[1] == 0 && second[3] == 0 && second[5] == 0,
          "one group read fills the managed columns of the committed row "
          "only (FR-026, FR-047)");
    check(second[0] >= first[0],
          "the member counter never decreases between two group reads "
          "(FR-013)");
    check(second[2] >= first[2] && second[4] >= first[4],
          "the leader's enabled and running times never decrease between "
          "two group reads (FR-041)");
  }
  // Two members in one group: the follower is opened disabled into its
  // leader's group, which is the arm a single-member open never reaches.
  const auto duo = sg::counters::detail::pmu_open_window(
      state, leaf_set_of({"cpu/work", "cpu/cycle"}), where);
  const bool duo_granted =
      sg::counters::detail::pmu_probe(state.devices[0].type,
                                      {{0, PERF_COUNT_HW_INSTRUCTIONS}})
          == availability::countable
      && sg::counters::detail::pmu_probe(state.devices[0].type,
                                         {{0, PERF_COUNT_HW_CPU_CYCLES}})
          == availability::countable;
  check(
      (duo != nullptr) == duo_granted,
      "a two-member group opens exactly when the probe grants both " "configs");
  if (duo != nullptr) {
    // Two columns, the two members: the read fills one row of a two-row
    // block, and the row this action did not commit stays untouched.
    std::uint64_t cells[4] = {0, 0, 0, 0};
    point_sink sink {cells, 2, 2, 0};
    duo->read_points(sink);
    sink.check_action();
    check(cells[1] == 0 && cells[3] == 0,
          "a two-member group read fills the committed row only (FR-047)");
  }
  std::printf("seam: the synthetic group open granted %zu of %zu configs\n",
              granted_count,
              cases.size());
}

// The fast-read branch of the open, entered with a catalog the fixture
// marks fast-capable. Its contexts need a user counter page this host
// does not grant, so the assertions are the ones the catalog alone
// decides: an entry whose disclosed mode is syscall never takes the
// mapped-page branch (FR-023, C-PRO-4), and a plan spanning two event
// sources takes the group path (FR-040).
auto fast_branch_scenario() -> void
{
  pmu_state state = synthetic_state();
  state.fast_available = true;
  state.fast_refusal = "synthetic fast-capable state";
  const target where {};
  const auto fast = sg::counters::detail::pmu_open_fast_window(
      state, leaf_set_of({"cpu/fast"}), where);
  std::printf("seam: the synthetic fast-capable open returned %s\n",
              fast != nullptr ? "a fast window" : "no window");
  pmu_state twin = state;
  twin.devices.push_back(twin.devices[0]);
  twin.devices.back().path = "cpu-uncore";
  check(sg::counters::detail::pmu_open_fast_window(
            twin, leaf_set_of({"cpu/fast", "cpu-uncore/fast"}), where)
            == nullptr,
        "a leaf set spanning two event sources opens no fast window");
  // The mapped-page read addresses the first config word only, so an entry
  // whose encoding also sets a later word names a different event once that
  // word is dropped. The window refuses it. Counting the wrong event is
  // the one outcome it will not produce, and the caller's group path
  // encodes the whole entry (FR-037, FR-040).
  check(sg::counters::detail::pmu_open_fast_window(
            state, leaf_set_of({"cpu/word1"}), where)
            == nullptr,
        "an entry encoded across more than the first config word opens no "
        "fast window");
  check(sg::counters::detail::pmu_open_fast_window(
            state, leaf_set_of({"cpu/word2"}), where)
            == nullptr,
        "the refusal holds for a third config word as well");

  // The general open takes the mapped-page branch only for a fast-capable
  // catalog whose every member discloses it; one syscall-mode member
  // sends the whole leaf set to the group path (FR-023, FR-040). Both
  // verdicts are the availability probe's, the same way `group_open_
  // scenario` holds the group path to it.
  const auto over_fast = sg::counters::detail::pmu_open_window(
      state, leaf_set_of({"cpu/fast"}), where);
  const auto over_syscall = sg::counters::detail::pmu_open_window(
      state, leaf_set_of({"cpu/work"}), where);
  const bool granted =
      sg::counters::detail::pmu_probe(state.devices[0].type,
                                      {{0, PERF_COUNT_HW_INSTRUCTIONS}})
      == availability::countable;
  check((over_fast != nullptr) == granted
            && (over_syscall != nullptr) == granted,
        "a fast-capable catalog opens over a fast member and over a "
        "syscall-mode member exactly when the probe grants the config");
  // The time pair is no group member, so it neither takes nor refuses the
  // mapped-page branch.
  check(sg::counters::detail::pmu_open_window(
            state, leaf_set_of({"cpu/enabled", "cpu/running"}), where)
            == nullptr,
        "a fast-capable leaf set carrying only the enabled/running pair "
        "opens no window");
}

}  // namespace

auto main() -> int
{
  format_scenario();
  compose_scenario();
  attr_text_scenario();
  decode_scenario();
  page_gate_scenario();
  table_array_scenario();
  table_object_scenario();
  mapfile_scenario();
  ecma_scenario();
  mapfile_row_scenario();
  window_refusal_scenario();
  group_open_scenario();
  fast_branch_scenario();
  probe_verdict_scenario();
  context_open_refusal_scenario();
  std::printf("counters_linux_pmu_seam_test PASS: encoder and protocol\n");
  return 0;
}
