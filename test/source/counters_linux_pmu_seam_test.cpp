// ============================================================================
// Synthetic-fixture coverage for the Linux PMU provider's pure seam
// functions (T066, plan.md Test Plan; FR-037, FR-040, R-010, R-011).
//
// The plan requires the mapped-page protocol logic and the config
// encoder to be exercised with injected page and index inputs, so CI
// covers them without privileges. The kernel glue that feeds them
// (mmap, perf_event_open, the instruction itself) runs on a
// fast-capable, probe-passing developer host; its unreachable lines
// carry a recorded coverage exclusion with written justification
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
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../source/counters/detail/pmu.hpp"

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

using sg::counters::detail::fast_read_verdict;
using sg::counters::detail::format_range;
using sg::counters::detail::kRnpmcCounterWidth;
using sg::counters::detail::parse_format_field;
using sg::counters::detail::pmu_compose_config;
using sg::counters::detail::pmu_ident;
using sg::counters::detail::pmu_select_directory;
using sg::counters::detail::pmu_table_entry;

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
}

// The decode half of the mapped-page protocol, over injected values
// (FR-040, R-011): the capability gate runs before the read, the
// sequence comparison closes it, and the kernel offset and counter width
// are applied last.
auto decode_scenario() -> void
{
  std::uint64_t value = 0;
  check(sg::counters::detail::fast_decode(
            7, 7, 1, 0x1234, 0, kRnpmcCounterWidth, value)
                == fast_read_verdict::ok
            && value == 0x1234,
        "a stable sequence with the capability granted decodes the read");
  check(sg::counters::detail::fast_decode(7, 7, 0, 0x1234, 0,
                                          kRnpmcCounterWidth, value)
            == fast_read_verdict::not_allowed,
        "a page with no read capability refuses before the sequence is "
        "consulted");
  value = 0xdeadbeef;
  check(sg::counters::detail::fast_decode(
            7, 9, 1, 0x1234, 0, kRnpmcCounterWidth, value)
                == fast_read_verdict::unstable
            && value == 0xdeadbeef,
        "a sequence that moved reports instability and writes no value");
  check(sg::counters::detail::fast_decode(
            7, 7, 1, 0x10, -16, kRnpmcCounterWidth, value)
                == fast_read_verdict::ok
            && value == 0,
        "a negative kernel offset subtracts from the raw read");
  check(sg::counters::detail::fast_decode(
            7, 7, 1, 0x10, 16, kRnpmcCounterWidth, value)
                == fast_read_verdict::ok
            && value == 0x20,
        "a positive kernel offset adds to the raw read");
  check(sg::counters::detail::fast_decode(7, 7, 1, 0x1234, 0, 8, value)
                == fast_read_verdict::ok
            && value == 0x34,
        "the value is masked to the counter width the kernel publishes");
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

  const auto& table = sg::counters::detail::pmu_load_table(directory);
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

}  // namespace

auto main() -> int
{
  format_scenario();
  compose_scenario();
  decode_scenario();
  mapfile_scenario();
  std::printf("counters_linux_pmu_seam_test PASS: encoder and protocol\n");
  return 0;
}
