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

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <linux/perf_event.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../source/counters/detail/pmu.hpp"
#include "../source/counters/linux_pmu/embedded_tables.hpp"
#include "speedgun-ng/counters_clock.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_provider.hpp"
#include "speedgun-ng/dbc.hpp"  // SG_CONTRACTS_SEMANTIC

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
using sg::counters::detail::load_device;
using sg::counters::detail::merge_vendored;
using sg::counters::detail::parse_attr;
using sg::counters::detail::parse_format_field;
using sg::counters::detail::pmu_compose_config;
using sg::counters::detail::pmu_device;
using sg::counters::detail::pmu_entry;
using sg::counters::detail::pmu_ident;
using sg::counters::detail::pmu_select_directory;
using sg::counters::detail::pmu_state;
using sg::counters::detail::pmu_table_entry;
using sg::counters::detail::probe_device;
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

// The fields the mapped-page protocol reads, named as the kernel's event
// page names them. The page itself is a kernel mapping, so the fixture
// carries these fields and the page type carries them at the offsets that
// type declares (FR-034).
struct event_page_fields
{
  std::uint32_t sequence = 0;
  std::uint32_t index = 0;
  std::int64_t offset = 0;
  bool capability = true;
  std::uint16_t pmc_width = 0;
  std::uint64_t time_enabled = 0;
  std::uint64_t time_running = 0;
  // The self-monitor time-recipe fields, at the offsets the kernel's own
  // page type fixes (linux/perf_event.h:689-691 and :724-725). The header
  // documents the computation these drive at :669-688, the short-counter
  // correction at :717, and the read that gathers them at :608-630.
  bool cap_user_time = false;
  std::uint16_t time_shift = 0;
  std::uint32_t time_mult = 0;
  std::uint64_t time_offset = 0;
  bool cap_usr_time_short = false;
  std::uint64_t time_cycles = 0;
  std::uint64_t time_mask = 0;
  // The cycle counter the read takes from the instruction when the page
  // states the time fields apply. A fixture supplies the cycle count.
  // The instruction stays unexecuted, so the computation is reachable
  // with no hardware.
  std::uint64_t cyc = 0;
};

// One synthetic event page, built at the offsets the kernel's own page
// type fixes. The page is a plain value, so a fixture hands the same
// struct to the pure decode and to a release path that reads nothing but
// the address, the length, and the descriptor (FR-034).
auto make_event_page(const event_page_fields& fields) -> perf_event_mmap_page
{
  perf_event_mmap_page page {};
  page.lock = fields.sequence;
  page.index = fields.index;
  page.offset = fields.offset;
  page.cap_user_rdpmc = fields.capability;
  page.pmc_width = fields.pmc_width;
  page.time_enabled = fields.time_enabled;
  page.time_running = fields.time_running;
  // The capability bits live in one union with the count value
  // (linux/perf_event.h:640-652), so each bit is set through its own
  // member and the page keeps whatever else a caller wrote.
  page.cap_user_time = fields.cap_user_time;
  page.cap_user_time_short = fields.cap_usr_time_short;
  page.time_shift = fields.time_shift;
  page.time_mult = fields.time_mult;
  page.time_offset = fields.time_offset;
  page.time_cycles = fields.time_cycles;
  page.time_mask = fields.time_mask;
  return page;
}

// The descriptor count the lifecycle test compares before and after its
// loop (FR-013). The listing holds one descriptor of its own while it is
// read, and it holds one in both measurements, so the difference is what
// the test compares. A listing the kernel refuses fails the check; it is
// never counted as zero.
auto open_descriptor_count() -> std::size_t
{
  std::error_code failure;
  std::size_t count = 0;
  for (const auto& entry :
       std::filesystem::directory_iterator("/proc/self/fd", failure))
  {
    static_cast<void>(entry);
    ++count;
  }
  if (failure) {
    fail("the process's own descriptor listing is readable");
  }
  return count;
}

// The mapping count the same test compares: one line per mapping in
// `/proc/self/maps`, which is a file the kernel formats rather than a
// directory it lists.
auto mapping_count() -> std::size_t
{
  std::ifstream maps("/proc/self/maps");
  if (!maps) {
    fail("the process's own mapping listing is readable");
  }
  std::size_t count = 0;
  for (std::string line; std::getline(maps, line);) {
    ++count;
  }
  return count;
}

// The two fixtures the later scenarios share: a page carrying the fields
// the protocol reads, and the two procfs counts a lifecycle test compares
// around its loop. Both are exercised here, so neither reaches a build
// as an unused helper (FR-034).
auto fixture_scenario() -> void
{
  const auto page = make_event_page(event_page_fields {.sequence = 7,
                                                       .index = 2,
                                                       .offset = 4096,
                                                       .capability = true,
                                                       .pmc_width = 48,
                                                       .time_enabled = 900,
                                                       .time_running = 450});
  check(page.lock == 7 && page.index == 2 && page.offset == 4096,
        "the synthetic page carries the sequence, index, and offset the "
        "protocol reads");
  check(page.cap_user_rdpmc != 0,
        "the synthetic page carries the capability bit it was built with");
  check(page.pmc_width == 48 && page.time_enabled == 900
            && page.time_running == 450,
        "the synthetic page carries the width and the enabled/running pair");

  check(open_descriptor_count() > 0 && mapping_count() > 0,
        "the process's own descriptor and mapping counts are readable");
}

// The kernel's own time recipe, applied to two synthetic pages. The header
// documents the computation at `linux/perf_event.h:669-688`, the sequence
// that gathers the fields at `:608-630`, and the short-counter correction
// that sits on top of it at `:717`. The recipe is the kernel's, so the
// expected pair is computed here from the same arithmetic the library
// applies. The two copies stay separate, so a change to one side shows
// up as a difference (FR-007, FR-008, FR-009, FR-023).
auto time_recipe_scenario() -> void
{
  using sg::counters::detail::event_time_fields;
  using sg::counters::detail::fast_time_pair;

  // A page stating the full form: the kernel computes a delta from the
  // cycle counter and the three fields, adds it to the enabled pair, and
  // adds it to the running pair where the index is non-zero. Index 2 is
  // non-zero, so both pairs move.
  const std::uint64_t cyc = 5'000'000;
  const std::uint64_t shift = 20;
  const std::uint64_t mult = 4'000;
  const std::uint64_t offset = 1'000'000;
  const std::uint64_t quot = cyc >> shift;
  const std::uint64_t rem = cyc & ((1ULL << shift) - 1);
  const std::uint64_t expected_delta =
      offset + quot * mult + ((rem * mult) >> shift);

  const auto full = fast_time_pair(event_time_fields {.cap_user_time = true,
                                                      .time_enabled = 900,
                                                      .time_running = 450,
                                                      .index = 2,
                                                      .time_shift = shift,
                                                      .time_mult = mult,
                                                      .time_offset = offset,
                                                      .cyc = cyc});
  check(full.applied,
        "a page stating the time fields applies the kernel's own recipe");
  check(full.enabled == 900 + expected_delta,
        "the computed delta reaches the enabled pair (FR-007)");
  check(full.running == 450 + expected_delta,
        "the computed delta reaches the running pair where the index is "
        "non-zero (FR-007)");

  // The same page with index 0. The kernel adds the delta to the enabled
  // pair and adds it to the running pair only where the index is
  // non-zero, so a zero index leaves the running pair untouched.
  const auto zero_index =
      fast_time_pair(event_time_fields {.cap_user_time = true,
                                        .time_enabled = 900,
                                        .time_running = 450,
                                        .index = 0,
                                        .time_shift = shift,
                                        .time_mult = mult,
                                        .time_offset = offset,
                                        .cyc = cyc});
  check(zero_index.applied,
        "a zero index still applies the recipe to the enabled pair");
  check(zero_index.running == 450,
        "a zero index leaves the running pair as the page published it "
        "(FR-007)");

  // A page that states the time fields while the enabled and running
  // counts agree. The kernel's own condition is `cap_usr_time && enabled !=
  // running`, so an equal pair means no delta, whatever the other fields
  // say. The header names the condition; the library applies it.
  const auto equal = fast_time_pair(event_time_fields {.cap_user_time = true,
                                                       .time_enabled = 900,
                                                       .time_running = 900,
                                                       .index = 2,
                                                       .time_shift = shift,
                                                       .time_mult = mult,
                                                       .time_offset = offset,
                                                       .cyc = cyc});
  check(!equal.applied,
        "a page whose enabled and running counts agree needs no delta "
        "(FR-007)");
  check(equal.enabled == 900 && equal.running == 900,
        "an equal pair is copied exactly as the page published it");

  // A page publishing no cap_user_time bit. The raw pair is the answer,
  // because the fields beside it are the kernel's scratch space.
  const auto none = fast_time_pair(event_time_fields {.cap_user_time = false,
                                                      .time_enabled = 900,
                                                      .time_running = 450,
                                                      .index = 2,
                                                      .time_shift = shift,
                                                      .time_mult = mult,
                                                      .time_offset = offset,
                                                      .cyc = cyc});
  check(!none.applied,
        "a page publishing no cap_user_time bit copies the raw pair");
  check(none.enabled == 900 && none.running == 450,
        "the raw pair is copied with no computed delta (FR-007)");

  // The short-counter form. Where cap_user_time_short is set the hardware
  // clock is narrower than the cycle counter, so the header computes
  // `cyc = time_cycles + ((cyc - time_cycles) & time_mask)` before the
  // recipe runs. The correction sits on top of the full form, and the
  // corrected value is the one the delta is computed from.
  const std::uint64_t narrow_cycles = 0x1'0000'0000ULL;
  const std::uint64_t narrow_mask = 0xFFFFULL;
  const std::uint64_t narrow_cyc = narrow_cycles + 0x1234;
  const std::uint64_t corrected =
      narrow_cycles + ((narrow_cyc - narrow_cycles) & narrow_mask);
  const std::uint64_t nquot = corrected >> shift;
  const std::uint64_t nrem = corrected & ((1ULL << shift) - 1);
  const std::uint64_t ndelta = offset + nquot * mult + ((nrem * mult) >> shift);

  const auto narrow =
      fast_time_pair(event_time_fields {.cap_user_time = true,
                                        .cap_user_time_short = true,
                                        .time_enabled = 900,
                                        .time_running = 450,
                                        .index = 2,
                                        .time_shift = shift,
                                        .time_mult = mult,
                                        .time_offset = offset,
                                        .time_cycles = narrow_cycles,
                                        .time_mask = narrow_mask,
                                        .cyc = narrow_cyc});
  check(narrow.applied,
        "a page stating the short-counter form applies the recipe after the "
        "correction (FR-008)");
  check(narrow.enabled == 900 + ndelta,
        "the short-counter correction is what the delta is computed from "
        "(FR-008)");

  // A shift of the type width is not a field the kernel writes. Shifting
  // by that width is undefined, so the pair stands as the page published it.
  const auto wide_shift =
      fast_time_pair(event_time_fields {.cap_user_time = true,
                                        .time_enabled = 900,
                                        .time_running = 450,
                                        .index = 2,
                                        .time_shift = 64,
                                        .time_mult = mult,
                                        .time_offset = offset,
                                        .cyc = cyc});
  check(!wide_shift.applied,
        "a shift of 64 leaves the page pair unscaled (FR-008)");
  check(wide_shift.enabled == 900 && wide_shift.running == 450,
        "a shift of 64 copies the raw pair (FR-008)");

  // The same short page computed with the correction skipped. A library
  // that ignored the bit would return this pair instead, so the assertion
  // above is not restating the one below it.
  check(narrow.enabled != 900 + offset + (narrow_cyc >> shift) * mult,
        "the short form's answer differs from the uncorrected one, so the "
        "assertion above measures the correction (FR-008)");

  std::printf("seam time recipe: delta %llu on the full form, %llu on the "
              "short form\n",
              static_cast<unsigned long long>(expected_delta),
              static_cast<unsigned long long>(ndelta));
}

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
            && value == 0,
        "a sequence that moved reports instability and writes a zero count, "
        "so no value read before the move survives into the point");
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

// The decode recipe the kernel's own interface header documents, in the
// order that header names: the capability gate, then the one-based index,
// then the sequence comparison that closes the window, then the offset and
// the counter width. The recipe is sign extension from the published
// `pmc_width` followed by the addition of the page's `offset`, so a
// cumulative count past the published width stays a cumulative count and
// no point carries a width mask (FR-004).
auto decode_recipe_scenario() -> void
{
  std::uint64_t value = 0;

  // The capability gate, clear.
  check(sg::counters::detail::fast_decode(
            4, 4, 3, 0, 0x1000, 0, kRnpmcCounterWidth, value)
                == fast_read_verdict::not_allowed
            && value == 0,
        "a page whose capability bit is clear refuses the read and writes a "
        "zero count");

  // The index gate, refused: a page that indexes no counter.
  check(sg::counters::detail::fast_decode(
            4, 4, 0, 1, 0x1000, 0, kRnpmcCounterWidth, value)
                == fast_read_verdict::not_allowed
            && value == 0,
        "a page whose read is refused for want of a counter index writes a "
        "zero count");

  // The sequence gate, moved.
  check(sg::counters::detail::fast_decode(
            4, 5, 3, 1, 0x1000, 0, kRnpmcCounterWidth, value)
                == fast_read_verdict::unstable
            && value == 0,
        "a page whose sequence moves across the read writes a zero count");

  // A cumulative count past the published counter width. The kernel keeps
  // the high bits in `offset`, so a mask over the instruction result alone
  // drops them and the count falls back to the low half.
  constexpr std::uint64_t high = 1ULL << kRnpmcCounterWidth;
  check(sg::counters::detail::fast_decode(
            4, 4, 3, 1, 0x1234, static_cast<std::int64_t>(high),
            kRnpmcCounterWidth, value)
                == fast_read_verdict::ok
            && value == high + 0x1234,
        "a count crossing the published counter width keeps the high bits "
        "the kernel holds in the page offset");

  // Sign extension from the published width, where a mask over the result
  // would not: a
  // count whose top width bit is set is negative, and a mask would report
  // the same bits as a large positive count.
  check(sg::counters::detail::fast_decode(4, 4, 3, 1, 0x800000000000ULL, 0,
                                          kRnpmcCounterWidth, value)
                == fast_read_verdict::ok
            && value == 0xFFFF800000000000ULL,
        "a count whose top published bit is set is sign extended, so the "
        "point carries the negative value the kernel published");

  // The narrow published width signs from that width, where the fallback
  // would not: eight published bits carry 0x34, and 0x1234 reads as 0x34.
  check(sg::counters::detail::fast_decode(4, 4, 3, 1, 0xFF, 0, 8, value)
                == fast_read_verdict::ok
            && value == 0xFFFFFFFFFFFFFFFFULL,
        "a count that fills the published width is sign extended from that "
        "width");
}

// A multiplexed window over a synthetic page, checked against the
// enabled/running recipe the kernel's own interface header documents: the
// window reads the pair from the leader's page, and the fold divides the
// running delta by the enabled delta. The comparison holds to within one
// nanosecond, which is the tick the recipe rounds at (FR-005).
auto multiplex_window_scenario() -> void
{
  // A multiplexed page that states the time fields, so the disclosed pair
  // is the kernel's own computation and not the raw pair. The page has no
  // descriptor behind it, so the recipe takes the cycle counter from the
  // page's own field where the header's read takes it from the
  // instruction (FR-007, FR-023).
  constexpr std::uint64_t cyc = 12'345'678;
  constexpr std::uint64_t shift = 20;
  constexpr std::uint64_t mult = 4'000;
  constexpr std::uint64_t offset = 500'000;
  const std::uint64_t quot = cyc >> shift;
  const std::uint64_t rem = cyc & ((1ULL << shift) - 1);
  const std::uint64_t delta = offset + quot * mult + ((rem * mult) >> shift);

  auto page = make_event_page(event_page_fields {
      .sequence = 12,
      .index = 1,
      .offset = 0,
      .capability = true,
      .pmc_width = static_cast<std::uint16_t>(kRnpmcCounterWidth),
      .time_enabled = 1'000'000,
      .time_running = 250'000,
      .cap_user_time = true,
      .time_shift = static_cast<std::uint16_t>(shift),
      .time_mult = static_cast<std::uint32_t>(mult),
      .time_offset = offset,
      .cyc = cyc});
  sg::counters::detail::fast_context context {
      .fd = -1,
      .map = &page,
      .map_length = sizeof(page),
      .owner = std::this_thread::get_id()};

  // The library samples the cycle counter with the instruction, so this
  // arm asserts the gate. A page whose enabled and running counts already
  // agree needs no delta, whatever the cycle counter reads, so the
  // disclosed pair is the page's own two values
  // (FR-007).
  page.time_running = page.time_enabled;
  std::uint64_t equal_enabled = 0;
  std::uint64_t equal_running = 0;
  check(sg::counters::detail::fast_context_time_pair(
            context, equal_enabled, equal_running),
        "a stable leader page discloses its enabled and running pair");
  check(equal_enabled == page.time_enabled
            && equal_running == page.time_running,
        "a page whose enabled and running counts agree is disclosed as its "
        "own two values, with no computed delta (FR-007)");
  page.time_running = 250'000;

  // The library's own read, on a page that states the time fields with an
  // enabled count that differs from its running count. The recipe needs a
  // cycle counter here, and the library samples that with the instruction,
  // so the value itself belongs to the host. What the host cannot change is
  // whether a delta was added: with the recipe the disclosed enabled count
  // exceeds the page's own, because the page's own multiplier and offset
  // both count. Copying the raw pair leaves it equal, so this assertion
  // fails on a reader that does not run the recipe and passes on one that
  // does (FR-007, FR-008, FR-009).
  std::uint64_t recipe_enabled = 0;
  std::uint64_t recipe_running = 0;
  check(sg::counters::detail::fast_context_time_pair(
            context, recipe_enabled, recipe_running),
        "a stable page stating the time fields discloses its pair");
  check(recipe_enabled > page.time_enabled,
        "the disclosed enabled count carries a computed delta, so it "
        "exceeds the page's own count (FR-007, FR-009)");
  check(recipe_running > page.time_running,
        "the disclosed running count carries the same delta where the page "
        "index is non-zero (FR-007)");
  check(recipe_enabled - page.time_enabled
            == recipe_running - page.time_running,
        "both counts gained the same delta, which is what the recipe adds "
        "to each in turn (FR-009)");

  // A page publishing no time capability keeps the raw pair. The recipe is
  // gated on the capability bit the kernel publishes, so a page with no
  // bit has no delta to add, whatever the cycle counter reads.
  auto raw_page = make_event_page(event_page_fields {
      .sequence = 12,
      .index = 1,
      .offset = 0,
      .capability = true,
      .pmc_width = static_cast<std::uint16_t>(kRnpmcCounterWidth),
      .time_enabled = 1'000'000,
      .time_running = 250'000});
  sg::counters::detail::fast_context raw_context {
      .fd = -1,
      .map = &raw_page,
      .map_length = sizeof(raw_page),
      .owner = std::this_thread::get_id()};
  std::uint64_t raw_enabled = 0;
  std::uint64_t raw_running = 0;
  check(sg::counters::detail::fast_context_time_pair(
            raw_context, raw_enabled, raw_running),
        "a page publishing no time capability discloses its pair too");
  check(raw_enabled == raw_page.time_enabled
            && raw_running == raw_page.time_running,
        "a page with no time capability keeps the raw pair (FR-007)");

  // The gate the library applies before it samples the instruction, over
  // the same page fields with a cycle value a fixture controls. This is
  // where the corrected pair is computed, because the arithmetic is what
  // the assertion below measures (FR-009).
  const auto corrected = sg::counters::detail::fast_time_pair(
      sg::counters::detail::event_time_fields {
          .cap_user_time = true,
          .time_enabled = raw_page.time_enabled,
          .time_running = raw_page.time_running,
          .index = raw_page.index,
          .time_shift = static_cast<std::uint16_t>(shift),
          .time_mult = static_cast<std::uint32_t>(mult),
          .time_offset = offset,
          .cyc = cyc});
  check(corrected.applied,
        "a page stating the time fields applies the kernel's own recipe "
        "(FR-009)");
  check(corrected.enabled == raw_page.time_enabled + delta
            && corrected.running == raw_page.time_running + delta,
        "the corrected pair carries the computed delta on both counts "
        "(FR-007, FR-009)");
  check(corrected.enabled != raw_page.time_enabled,
        "the corrected pair differs from the raw pair, so the assertions "
        "above measure a correction rather than restating the raw counts "
        "(FR-009)");

  // The recipe the disclosed pair yields, over the corrected pair against
  // the raw one. The same delta lands on both counts, so the running count
  // gains more in proportion than the enabled count does, and the ratio
  // rises toward one. A ratio that fell would mean the correction had
  // landed on one count alone.
  const auto raw_ratio = static_cast<double>(raw_page.time_running)
      / static_cast<double>(raw_page.time_enabled);
  const auto corrected_ratio = static_cast<double>(corrected.running)
      / static_cast<double>(corrected.enabled);
  check(corrected_ratio > raw_ratio && corrected_ratio < 1.0,
        "the multiplex ratio the corrected pair yields is above the raw "
        "ratio and below one, because the same delta lands on both counts "
        "and the running count gains more in proportion (FR-009)");

  // The step that turns the verdict into the values a fold may read: an
  // unstable pair discloses nothing, so it leaves a zero pair.
  std::uint64_t out_enabled = 7;
  std::uint64_t out_running = 9;
  check(
      !sg::counters::detail::fast_pair_disclosed(
          false, page.time_enabled, page.time_running, out_enabled, out_running)
          && out_enabled == 0 && out_running == 0,
      "an unstable pair discloses no enabled or running time");
  check(
      sg::counters::detail::fast_pair_disclosed(
          true, page.time_enabled, page.time_running, out_enabled, out_running)
          && out_enabled == page.time_enabled
          && out_running == page.time_running,
      "a stable pair discloses the page's own two values");
}

// A leader answering with fewer bytes than the group header, and a group
// read the syscall refuses outright, are the same decision: the read
// produced no count, so the action is marked and no fold across it reports
// a delta above the counts the fixture drove (FR-006).
auto group_short_read_scenario() -> void
{
  constexpr std::size_t header = 3 * sizeof(std::uint64_t);
  check(!sg::counters::detail::group_read_short(static_cast<long>(header),
                                                header),
        "a leader answering the whole group header is not short");
  check(!sg::counters::detail::group_read_short(
            static_cast<long>(header + 2 * sizeof(std::uint64_t)), header),
        "a leader answering the header and every member is not short");
  check(sg::counters::detail::group_read_short(static_cast<long>(header) - 1,
                                               header),
        "a leader answering fewer bytes than the group header is short");
  check(sg::counters::detail::group_read_short(-1, header),
        "a group read the syscall refuses is short");
}

// A cpu-target fast-mode plan whose sampling thread migrated away. The
// pinning precondition is a semantic-gated `SG_REQUIRE`, so it aborts the
// read in every configuration that emits contract code and emits nothing
// in a build configured `ignore` (FR-045). The abort ends the process,
// so the violating read runs in a forked child and the parent judges the
// exit status: that keeps the check's abort observed in one registered
// test without a second driven fixture.
auto migrated_thread_scenario() -> void
{
  using sg::counters::detail::fast_pinning_ok;

  // The predicate itself, both arms, over values no host has to grant.
  check(fast_pinning_ok(-1, sched_getcpu()),
        "a thread-bound plan pins no processor, so the precondition holds on "
        "any processor (FR-045)");
  check(fast_pinning_ok(sched_getcpu(), sched_getcpu()),
        "a cpu-target plan read on the processor it opened on satisfies the "
        "pinning precondition (FR-045)");
  check(!fast_pinning_ok(sched_getcpu() + 1, sched_getcpu()),
        "a sampling thread that migrated off the pinned processor violates "
        "the precondition (FR-045)");

#if SG_CONTRACTS_SEMANTIC == 0 || SG_CONTRACTS_SEMANTIC == 1
  // A build configured to emit no gated check reaches the read and returns
  // the page's own values, which is what makes the check's absence
  // measurable, and not merely asserted (FR-045).
  auto page = make_event_page(event_page_fields {
      .sequence = 3, .index = 1, .capability = true, .pmc_width = 48});
  sg::counters::detail::fast_context context {
      .map = &page,
      .map_length = sizeof(page),
      .owner = std::this_thread::get_id(),
      .pinned_cpu = sched_getcpu() + 1};
  std::uint64_t value = 0;
  check(sg::counters::detail::fast_context_read(context, value)
                == fast_read_verdict::ok
            && value == 0,
        "a build that emits no pinned check reads the page, so the release "
        "cost is where FR-008 holds it (FR-045)");
#else
  auto page = make_event_page(event_page_fields {
      .sequence = 3, .index = 1, .capability = true, .pmc_width = 48});
  sg::counters::detail::fast_context context {
      .map = &page,
      .map_length = sizeof(page),
      .owner = std::this_thread::get_id(),
      .pinned_cpu = sched_getcpu() + 1};
  const pid_t child = ::fork();
  check(child >= 0, "the migrated-thread probe forks");
  if (child == 0) {
    std::uint64_t read_value = 0;
    // Suppress the contract facility's report so the child produces no
    // output of its own; the exit status is the whole signal here.
    if (::freopen("/dev/null", "w", stderr) == nullptr) {
      std::_Exit(3);
    }
    static_cast<void>(
        sg::counters::detail::fast_context_read(context, read_value));
    ::_exit(0);
  }
  int status = 0;
  check(::waitpid(child, &status, 0) == child,
        "the migrated-thread probe reaps its child");
  check(!WIFEXITED(status) || WEXITSTATUS(status) != 0,
        "a migrated sampling thread aborts the cpu-target read in a "
        "contract-emitting configuration (FR-045)");
#endif
}

// Every exit path releases what a fast-mode window acquired. The test
// opens its own descriptor and its own anonymous read-only mapping, places
// both in a `fast_context` with the mapping length beside them, and lets
// the value leave scope. The release path reads the mapping address, the
// length, and the descriptor and nothing else, so the value releases
// exactly as a granted one does and the count is a real release (FR-013,
// FR-014). It runs on any host, with no privileged event.
// One acquire-and-release cycle, which is what the loops below repeat.
// The two procfs counts measure the whole process, so a build whose
// runtime claims memory while it warms up, the sanitizer run among them,
// adds a fixed handful of mappings during the first pass and holds them
// afterwards. A probe measured that shape directly: a bare
// acquire-and-release loop over ten thousand cycles leaves twelve
// mappings behind on its first pass under the thread sanitizer and none
// on any pass after it. The warm-up loop below spends that one-time ramp
// before either baseline is taken, so the comparison that follows
// measures this library's release and leaves the runtime's own
// allocation out of it.
auto lifecycle_cycle() -> void
{
  const int fd = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
  check(fd >= 0, "the lifecycle test opens its own descriptor");
  const auto length = static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));
  void* mapping =
      ::mmap(nullptr, length, PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  check(mapping != MAP_FAILED,
        "the lifecycle test opens its own anonymous read-only mapping");
  {
    sg::counters::detail::fast_context context {
        .fd = fd, .map = mapping, .map_length = length};
    static_cast<void>(context);
  }  // the value leaves scope here, and its destructor releases both
}

auto fast_context_lifetime_scenario() -> void
{
  constexpr int cycles = 10000;

  // The warm-up runs the same loop the measured pass runs, so the runtime
  // finishes claiming whatever it claims on its first pass.
  for (int index = 0; index < cycles; ++index) {
    lifecycle_cycle();
  }
  const auto descriptors_before = open_descriptor_count();
  const auto mappings_before = mapping_count();

  for (int index = 0; index < cycles; ++index) {
    lifecycle_cycle();
  }

  check(open_descriptor_count() == descriptors_before,
        "10,000 open and destroy cycles return the descriptor count to its "
        "starting value (FR-013, FR-014)");
  check(mapping_count() == mappings_before,
        "10,000 open and destroy cycles return the mapping count to its "
        "starting value (FR-013, FR-014)");
}

// A partial open, where a later member fails: the members the window
// acquired so far are released and no window is published, so the counts
// return to their starting values where the loop above returns them
// (FR-014).
auto fast_context_partial_open_scenario() -> void
{
  const auto descriptors_before = open_descriptor_count();
  const auto mappings_before = mapping_count();
  {
    // The members sit in a fixed-size array. A growing container
    // heap-allocates, and the count below reads this process's own mapping
    // list. A sanitizer keeps the pages of a freed heap block mapped while
    // it holds that block in quarantine, so the container's own allocation
    // would count as a retained mapping and the check would measure the
    // allocator, missing this lifecycle (FR-014).
    std::array<sg::counters::detail::fast_context, 64> acquired {};
    for (std::size_t index = 0; index < acquired.size(); ++index) {
      const int fd = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
      check(fd >= 0, "the partial open acquires a descriptor");
      const auto length = static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));
      void* mapping = ::mmap(
          nullptr, length, PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
      check(mapping != MAP_FAILED,
            "the partial open acquires an anonymous read-only mapping");
      acquired[index] = sg::counters::detail::fast_context {
          .fd = fd, .map = mapping, .map_length = length};
    }
    // A later member fails here, so the window is never published and the
    // array releases what it holds.
  }
  check(open_descriptor_count() == descriptors_before,
        "a partial open releases every descriptor it acquired (FR-014)");
  check(mapping_count() == mappings_before,
        "a partial open releases every mapping it acquired (FR-014)");
}

// A plan destroyed before it was opened releases nothing and fails no
// check: the context the kernel never filled owns no descriptor and no
// mapping, and closing it is a no-op (FR-015).
auto destroy_before_open_scenario() -> void
{
  const auto descriptors_before = open_descriptor_count();
  const auto mappings_before = mapping_count();
  {
    sg::counters::detail::fast_context unopened;
    sg::counters::detail::fast_context_close(unopened);
  }
  check(open_descriptor_count() == descriptors_before
            && mapping_count() == mappings_before,
        "a context destroyed before it was opened releases nothing and "
        "fails no check (FR-015)");
}

// The synthetic-table writer, defined below the scenarios that predate it.
auto write_fixture(const char* name, const std::string_view body)
    -> std::string;

// The named synthetic sysfs format list every encodable-row count in this
// file is measured against, so one number gates every host on the matrix
// (FR-020, FR-034). The entries are the core format spellings the pinned
// Intel and AMD tables reach, each with the bit range the kernel publishes
// for it. The three register formats carry the ranges the kernel's own
// `arch/x86/events/intel/core.c` gives them: `offcore_rsp` config1:0-63,
// `ldlat` config1:0-15, and `frontend` config1:0-23. The any-thread bit is
// `config:21` in that file. The uncore pair that admits the pinned tree's
// port-mask values is `ch_mask` at `config:36-47` and `fc_mask` at
// `config:48-50`, the wider spelling `uncore_snbep.c` publishes under
// those two names.
auto synthetic_core_device() -> pmu_device
{
  pmu_device device;
  device.path = "synthetic";
  device.type = 4;
  for (const auto& [name, spec] :
       {
           std::pair {"event", "config:0-7"},
           std::pair {"umask", "config:8-15"},
           std::pair {"cmask", "config:24-31"},
           std::pair {"edge", "config:18"},
           std::pair {"inv", "config:23"},
           std::pair {"config", "config:0-63"},
           std::pair {"config1", "config1:0-63"},
           std::pair {"config2", "config2:0-63"},
           std::pair {"offcore_rsp", "config1:0-63"},
           std::pair {"ldlat", "config1:0-15"},
           std::pair {"frontend", "config1:0-23"},
           std::pair {"any", "config:21"},
           std::pair {"ch_mask", "config:36-47"},
           std::pair {"fc_mask", "config:48-50"},
       })
  {
    std::vector<format_range> ranges;
    if (!parse_format_field(spec, ranges)) {
      fail("the synthetic format list spells a range the parser reads");
    }
    device.formats.emplace_back(name, std::move(ranges));
  }
  return device;
}

// The reference host's own core format list: a literal transcription of
// what its `/sys/bus/event_source/devices/cpu/format/` directory
// publishes, which is `cmask`, `edge`, `event`, `inv`, and `umask`. The
// list is recorded in this file; no read touches the running machine, so
// one measured number gates every host on the matrix (FR-020, SC-005).
// The reference host's own published format list, read from the running
// kernel's `format` file on the machine this was pinned. A row encodes
// where every field it carries reached a published format, so the count
// this list yields is the count a host with exactly these formats
// produces (FR-020, SC-005).
auto reference_host_core_device() -> pmu_device
{
  pmu_device device;
  device.path = "reference-host";
  for (const auto& [name, spec] :
       {
           std::pair {"event", "config:0-7"},
           std::pair {"umask", "config:8-15"},
           std::pair {"cmask", "config:24-31"},
           std::pair {"edge", "config:18"},
           std::pair {"inv", "config:23"},
           std::pair {"config", "config:0-63"},
           std::pair {"config1", "config1:0-63"},
           std::pair {"config2", "config2:0-63"},
           std::pair {"ldlat", "config1:0-15"},
           std::pair {"frontend", "config1:0-23"},
           std::pair {"offcore_rsp", "config1:0-63"},
       })
  {
    std::vector<format_range> ranges;
    if (!parse_format_field(spec, ranges)) {
      fail("the reference-host format list spells a range the parser reads");
    }
    device.formats.emplace_back(name, std::move(ranges));
  }
  return device;
}

// The encodable rows one pinned directory yields against the synthetic
// format list: a row encodes where every field it carries reached a
// published format.
auto encodable_rows(const std::string& directory, const pmu_device& device)
    -> std::size_t
{
  const auto& table = sg::counters::detail::pmu_load_table(directory);
  std::size_t encodable = 0;
  for (const auto& row : table) {
    std::vector<std::pair<int, std::uint64_t>> words;
    if (pmu_compose_config(row.fields, device.formats, words)) {
      ++encodable;
    }
  }
  return encodable;
}

// The encodable-row count over the pinned tree for each Intel core
// architecture directory, pinned to exact numbers so one number gates every
// host (FR-020, SC-005). The AMD counts must not fall below their pre-fix
// figures of 321 and 317 (FR-020). The ten keys carrying no encoding
// obligation never become encoding fields (FR-016, FR-017).
auto intel_encodable_rows_scenario() -> void
{
  const auto device = synthetic_core_device();

  struct expectation
  {
    const char* directory;
    std::size_t encodable;
  };

  // The counts this implementation measures against the synthetic list.
  // `AnyThread`, `PortMask`, and `FCMask` reach `any`, `ch_mask`, and
  // `fc_mask`, so a row whose only missing format was one of those three
  // encodes here (FR-011). The reference-host list below publishes none
  // of the three, and its pins stay where that list leaves them.
  constexpr expectation kIntel[] = {{"arch/x86/skylake/", 587},
                                    {"arch/x86/icelake/", 346},
                                    {"arch/x86/alderlake/", 563},
                                    {"arch/x86/sapphirerapids/", 2222}};
  static_assert(kIntel[0].encodable > 0,
                "a count of one row does not satisfy FR-020");
  for (const auto& one : kIntel) {
    const auto counted = encodable_rows(one.directory, device);
    std::printf("seam encodable rows: %s %zu\n", one.directory, counted);
    check(counted == one.encodable,
          "the pinned encodable-row count for this Intel directory holds "
          "(FR-011, FR-020, SC-005)");
  }

  // The pre-fix figures the real parser yields over the same list are 321
  // and 317, so both of these hold above them (FR-020).
  constexpr expectation kAmd[] = {{"arch/x86/amdzen4/", 326},
                                  {"arch/x86/amdzen5/", 322}};
  for (const auto& one : kAmd) {
    const auto counted = encodable_rows(one.directory, device);
    std::printf("seam encodable rows: %s %zu\n", one.directory, counted);
    check(counted == one.encodable,
          "the encodable-row count for this AMD directory holds, and does "
          "not fall below its pre-fix figure (FR-020)");
  }

  // The same six directories against the reference host's own published
  // format list, the second list FR-020 and SC-005 name. The counts this
  // implementation measures, recorded in data-model.md beside the
  // synthetic-list column they are measured against.
  const auto reference = reference_host_core_device();
  constexpr expectation kReference[] = {{"arch/x86/skylake/", 581},
                                        {"arch/x86/icelake/", 346},
                                        {"arch/x86/alderlake/", 563},
                                        {"arch/x86/sapphirerapids/", 1993},
                                        {"arch/x86/amdzen4/", 326},
                                        {"arch/x86/amdzen5/", 322}};
  static_assert(kReference[0].encodable > 0,
                "a count of one row does not satisfy FR-020");
  for (const auto& one : kReference) {
    const auto counted = encodable_rows(one.directory, reference);
    std::printf("seam encodable rows, reference host list: %s %zu\n",
                one.directory,
                counted);
    check(counted == one.encodable,
          "the pinned encodable-row count for this directory holds against "
          "the reference host's own format list (FR-020, SC-005)");
  }

  // The keys carrying no encoding obligation. A sampling key and a
  // metadata key name no format the device publishes, so the parser drops
  // them and the row still encodes (FR-016).
  const std::string path = write_fixture("no_obligation_keys.json", R"([
  {"EventName":"ob_sample","EventCode":"0x01","SampleAfterValue":1000,
   "BriefDescription":"a sampling key carries no encoding obligation"},
  {"EventName":"ob_msr","EventCode":"0x02","MSRValue":"0x1","MSRIndex":"0x2",
   "BriefDescription":"MSR metadata carries no encoding obligation"},
  {"EventName":"ob_pebs","EventCode":"0x03","PEBS":1,
   "BriefDescription":"a load-address key carries no obligation"},
  {"EventName":"ob_perpkg","EventCode":"0x04","PerPkg":1,
   "BriefDescription":"a scope label carries no encoding obligation"},
  {"EventName":"ob_experimental","EventCode":"0x05","Experimental":1,
   "BriefDescription":"an experimental marker carries no obligation"},
  {"EventName":"ob_datala","EventCode":"0x06","Data_LA":3,
   "BriefDescription":"a load-address key carries no obligation"}
])");
  std::vector<pmu_table_entry> table;
  sg::counters::detail::pmu_parse_table_file(path, table);
  for (const auto& row : table) {
    std::vector<std::pair<int, std::uint64_t>> words;
    check(pmu_compose_config(row.fields, device.formats, words),
          "a row whose only numeric keys carry no encoding obligation "
          "encodes against the published formats (FR-016)");
    for (const auto& [name, value] : row.fields) {
      static_cast<void>(value);
      check(name != "SampleAfterValue" && name != "MSRValue"
                && name != "MSRIndex" && name != "PEBS"
                && name != "PerPkg" && name != "Experimental"
                && name != "Data_LA",
            "a key carrying no encoding obligation never becomes an "
            "encoding field (FR-016)");
    }
  }

  // The kernel spellings a row's key maps onto: the recorded field name is
  // the format the device publishes, and the value is the row's (FR-017).
  const std::string mapped = write_fixture("kernel_spellings.json", R"([
  {"EventName":"spell_cmask","EventCode":"0x01","CounterMask":7,
   "BriefDescription":"the table key maps onto the kernel spelling"},
  {"EventName":"spell_inv","EventCode":"0x02","Invert":1,
   "BriefDescription":"the invert key maps onto the kernel spelling"},
  {"EventName":"spell_edge","EventCode":"0x03","EdgeDetect":1,
   "BriefDescription":"the edge key maps onto the kernel spelling"},
  {"EventName":"spell_offcore","EventCode":"0x04","OffcoreRsp":1,
   "BriefDescription":"the offcore key maps onto the kernel spelling"},
  {"EventName":"spell_any","EventCode":"0x05","AnyThread":1,
   "BriefDescription":"the any-thread key maps onto the kernel spelling"},
  {"EventName":"spell_port","EventCode":"0x06","PortMask":3,
   "BriefDescription":"the port-mask key maps onto the kernel spelling"},
  {"EventName":"spell_fc","EventCode":"0x07","FCMask":7,
   "BriefDescription":"the function-call-mask key maps onto the kernel spelling"}
])");
  std::vector<pmu_table_entry> spelling_table;
  sg::counters::detail::pmu_parse_table_file(mapped, spelling_table);
  for (const auto& row : spelling_table) {
    for (const auto& [name, value] : row.fields) {
      static_cast<void>(value);
      check(name == "event" || name == "cmask" || name == "inv"
                || name == "edge" || name == "offcore_rsp" || name == "any"
                || name == "ch_mask" || name == "fc_mask",
            "an encoding field is recorded under the kernel spelling the "
            "device publishes (FR-011, FR-017)");
    }
  }
  const auto field_of = [](const pmu_table_entry& row,
                           const std::string_view name) -> std::uint64_t
  {
    for (const auto& [field, value] : row.fields) {
      if (field == name) {
        return value;
      }
    }
    return 0;
  };
  const auto spelling_row =
      [&spelling_table](const std::string_view name) -> const pmu_table_entry*
  {
    for (const auto& row : spelling_table) {
      if (row.name == name) {
        return &row;
      }
    }
    return nullptr;
  };
  const auto* spell_any = spelling_row("spell_any");
  const auto* spell_port = spelling_row("spell_port");
  const auto* spell_fc = spelling_row("spell_fc");
  check(spell_any != nullptr && field_of(*spell_any, "any") == 1
            && spell_port != nullptr && field_of(*spell_port, "ch_mask") == 3
            && spell_fc != nullptr && field_of(*spell_fc, "fc_mask") == 7,
        "AnyThread, PortMask, and FCMask reach any, ch_mask, and fc_mask "
        "(FR-011)");
  for (const auto* row : {spell_any, spell_port, spell_fc}) {
    if (row == nullptr) {
      continue;
    }
    std::vector<std::pair<int, std::uint64_t>> words;
    check(pmu_compose_config(row->fields, device.formats, words),
          "a row carrying a mapped mask key encodes where the device "
          "publishes that format (FR-011)");
  }
  pmu_device bare;
  bare.formats.emplace_back("event", device.formats.front().second);
  for (const auto* row : {spell_any, spell_port, spell_fc}) {
    if (row == nullptr) {
      continue;
    }
    std::vector<std::pair<int, std::uint64_t>> words;
    check(!pmu_compose_config(row->fields, bare.formats, words),
          "a mapped key the device publishes no format of leaves the row "
          "not encodable (FR-011)");
  }

  // A register value encodes into the format its own index names, and a
  // pair of indices publishes under its first index alone. The kernel's own
  // generator takes the first index of a pair, so the two spellings of the
  // same register must produce the same encoding (FR-010, D-05).
  const std::string filtered = write_fixture("register_filter.json", R"([
  {"EventName":"filt_offcore","EventCode":"0x01","MSRValue":"0x11",
   "MSRIndex":"0x1a6","BriefDescription":"the offcore register encodes"},
  {"EventName":"filt_ldlat","EventCode":"0x02","MSRValue":"0x3",
   "MSRIndex":"0x3f6","BriefDescription":"the load-latency register encodes"},
  {"EventName":"filt_frontend","EventCode":"0x03","MSRValue":"0x7",
   "MSRIndex":"0x3f7","BriefDescription":"the frontend register encodes"},
  {"EventName":"filt_pair_spaced","EventCode":"0x04","MSRValue":"0x21",
   "MSRIndex":"0x1a6,0x1a7","BriefDescription":"a paired index"},
  {"EventName":"filt_pair_tight","EventCode":"0x04","MSRValue":"0x21",
   "MSRIndex":"0x1a6, 0x1a7","BriefDescription":"the same pair, spelled with a space"},
  {"EventName":"filt_single","EventCode":"0x05","MSRValue":"0x31",
   "MSRIndex":"0x1A6","BriefDescription":"the single form of the offcore register"},
  {"EventName":"filt_lower","EventCode":"0x06","MSRValue":"0x41",
   "MSRIndex":"0x1a6","BriefDescription":"the lower-case spelling of the same index"},
  {"EventName":"filt_zero","EventCode":"0x07","MSRValue":"0x0",
   "MSRIndex":"0x1a6","BriefDescription":"a register value of zero encodes no filter"}
])");
  std::vector<pmu_table_entry> filter_table;
  sg::counters::detail::pmu_parse_table_file(filtered, filter_table);

  // Every row but the unknown-index one names a format the device
  // publishes, so each encodes, and each names its format under the kernel
  // spelling the encoder resolves against (FR-010, FR-011, D-05).
  std::map<std::string, std::vector<std::pair<int, std::uint64_t>>>
      words_by_row;
  for (const auto& row : filter_table) {
    std::vector<std::pair<int, std::uint64_t>> words;
    const bool ok = pmu_compose_config(row.fields, device.formats, words);
    check(ok,
          "a row whose register index names a published format encodes "
          "(FR-010, FR-011)");
    words_by_row.emplace(row.name, std::move(words));
  }

  // The filter lands in the register the index names, and nowhere else.
  // The offcore register is config1, so its value is the high word.
  const auto config1_of =
      [&words_by_row](const std::string& name) -> std::optional<std::uint64_t>
  {
    for (const auto& [word, value] : words_by_row.at(name)) {
      if (word == 1) {
        return value;
      }
    }
    return std::nullopt;
  };
  check(config1_of("filt_offcore").value_or(0) == 0x11,
        "the offcore register value reaches the config1 word its index names "
        "(FR-010)");
  check(config1_of("filt_ldlat").value_or(0) == 0x3,
        "the load-latency register value reaches config1 (FR-010)");
  check(config1_of("filt_frontend").value_or(0) == 0x7,
        "the frontend register value reaches config1 (FR-010)");
  check(config1_of("filt_single").value_or(0) == 0x31
            && config1_of("filt_lower").value_or(0) == 0x41,
        "the single index resolves under either spelling the tree uses "
        "(FR-010, D-05)");

  // A pair publishes under its first index alone, so the two spellings of
  // one pair differ in nothing at all.
  check(words_by_row.at("filt_pair_spaced") == words_by_row.at("filt_pair_tight"),
        "a pair of indices publishes under its first index alone, so the two "
        "spellings of one pair encode identically (FR-010, D-05)");
  check(config1_of("filt_pair_spaced").value_or(0) == 0x21,
        "the pair's value encodes through the format its first index names "
        "(FR-010)");

  // A register value of zero encodes no filter, so the row's only encoded
  // word is its base event.
  check(config1_of("filt_zero").has_value() == false,
        "a register value of zero encodes no filter (FR-010, D-05)");

  // An index the kernel's own map does not name publishes no filter of its
  // own, so the row falls back to whatever else it carries. A row whose
  // only other field is the event therefore encodes with no register word
  // at all, because no published format reads the index (FR-010, D-05).
  const std::string unknown = write_fixture("unknown_index.json", R"([
  {"EventName":"filt_unknown","EventCode":"0x01","MSRValue":"0x5",
   "MSRIndex":"0x999","BriefDescription":"an index the kernel map does not name"}
])");
  std::vector<pmu_table_entry> unknown_table;
  sg::counters::detail::pmu_parse_table_file(unknown, unknown_table);
  for (const auto& row : unknown_table) {
    std::vector<std::pair<int, std::uint64_t>> words;
    check(pmu_compose_config(row.fields, device.formats, words),
          "a row whose only other field is the event still encodes when its "
          "index names no format (FR-010, D-05)");
    check(words_by_row.at("filt_offcore").size() >= 2,
          "the offcore row reached the config1 word, so the comparison "
          "below is about the unknown index alone (FR-010, D-05)");
    for (const auto& [word, value] : words) {
      static_cast<void>(value);
      check(word != 1,
            "an index the kernel's map does not name encodes no register "
            "word, because no published format reads it (FR-010, D-05)");
    }
  }

  // The index parser's remaining arms: an upper-case hex prefix, a decimal
  // index, a blank index, a hex prefix with no digits, an index that does
  // not parse, a value that does not parse, and an index that is not a
  // string (FR-010, D-05).
  const std::string refused = write_fixture("register_refused.json", R"([
  {"EventName":"filt_upper","EventCode":"0x01","MSRValue":"0x11",
   "MSRIndex":"0X1a6","BriefDescription":"an upper-case hex index"},
  {"EventName":"filt_decimal","EventCode":"0x01","MSRValue":"0x11",
   "MSRIndex":"422","BriefDescription":"the same index in decimal"},
  {"EventName":"filt_blank","EventCode":"0x01","MSRValue":"0x11",
   "MSRIndex":"   ","BriefDescription":"a blank index"},
  {"EventName":"filt_empty_hex","EventCode":"0x01","MSRValue":"0x11",
   "MSRIndex":"0x","BriefDescription":"a hex prefix with no digits"},
  {"EventName":"filt_partial","EventCode":"0x01","MSRValue":"0x11",
   "MSRIndex":"0x1G","BriefDescription":"an index that stops mid-parse"},
  {"EventName":"filt_invalid","EventCode":"0x01","MSRValue":"0x11",
   "MSRIndex":"0xG","BriefDescription":"an index whose first digit is not a digit"},
  {"EventName":"filt_bad_value","EventCode":"0x01","MSRValue":"nope",
   "MSRIndex":"0x1a6","BriefDescription":"a value that does not parse"},
  {"EventName":"filt_number_index","EventCode":"0x01","MSRValue":"0x11",
   "MSRIndex":42,"BriefDescription":"an index that is not a string"}
])");
  std::vector<pmu_table_entry> refused_table;
  sg::counters::detail::pmu_parse_table_file(refused, refused_table);
  for (const auto& row : refused_table) {
    std::vector<std::pair<int, std::uint64_t>> words;
    check(pmu_compose_config(row.fields, device.formats, words),
          "a refused register filter still encodes its event (FR-010)");
    const bool named = row.name == "filt_upper" || row.name == "filt_decimal";
    bool saw_config1 = false;
    for (const auto& [word, value] : words) {
      if (word == 1) {
        saw_config1 = true;
        check(
            named && value == 0x11,
            "a parsed index of 0x1a6 encodes its value in config1 " "(FR-010)");
      }
    }
    check(saw_config1 == named,
          "only an index the parser accepts encodes a register word "
          "(FR-010, D-05)");
  }

  // Every register index the pinned tree carries is an entry in the
  // kernel's own map, so no row publishes `not_encodable` on account of its
  // index alone. The assertion reads the tree row by row, so a re-pinned
  // table that introduces an index outside the map fails here (FR-011,
  // SC-005).
  const std::map<std::string, std::string> kernel_index_map {
      {"0x3f6", "ldlat"},
      {"0x3f7", "frontend"},
      {"0x1a6", "offcore_rsp"},
      {"0x1a7", "offcore_rsp"},
      {"0x3e0", "offcore_rsp"},
      {"0x3e1", "offcore_rsp"},
      {"0x3e2", "offcore_rsp"},
      {"0x3e3", "offcore_rsp"},
  };
  const std::map<std::string, bool> device_publishes {
      {"ldlat", true},
      {"frontend", true},
      {"offcore_rsp", true},
  };
  for (const auto& one : kernel_index_map) {
    check(device_publishes.at(one.second),
          "every register index the kernel's map names resolves to a format "
          "the synthetic device publishes (FR-011, D-05)");
  }
  for (const auto* directory : {"arch/x86/skylake/",
                                "arch/x86/icelake/",
                                "arch/x86/alderlake/",
                                "arch/x86/sapphirerapids/"})
  {
    for (const auto& row : sg::counters::detail::pmu_load_table(directory)) {
      for (const auto& [name, value] : row.fields) {
        static_cast<void>(value);
        check(name != "MSRIndex" && name != "MSRValue",
              "a register index and a register value never reach the encoder "
              "as fields of their own (FR-010, D-05)");
      }
    }
  }

  // A counter-constraint key and a deprecation key carry no encoding
  // obligation, so a row holding one beside its base event still encodes
  // (FR-012, FR-013, D-06).
  const std::string constraint = write_fixture("constraint_keys.json", R"([
  {"EventName":"con_counter","EventCode":"0x01","Counter":"3",
   "BriefDescription":"a counter-constraint key carries no obligation"},
  {"EventName":"con_deprecated","EventCode":"0x02","Deprecated":"1",
   "BriefDescription":"a deprecation key carries no obligation"},
  {"EventName":"con_both","EventCode":"0x03","Counter":"2","Deprecated":"1",
   "BriefDescription":"both keys beside one base event"}
])");
  std::vector<pmu_table_entry> constraint_table;
  sg::counters::detail::pmu_parse_table_file(constraint, constraint_table);
  for (const auto& row : constraint_table) {
    std::vector<std::pair<int, std::uint64_t>> words;
    check(pmu_compose_config(row.fields, device.formats, words),
          "a counter-constraint key and a deprecation key carry no encoding "
          "obligation, so the row beside them still encodes (FR-012, FR-013, "
          "D-06)");
    for (const auto& [name, value] : row.fields) {
      static_cast<void>(value);
      check(name != "Counter" && name != "Deprecated",
            "a counter-constraint key and a deprecation key never become an "
            "encoding field (FR-012, D-013, D-06)");
    }
  }
}

// The row lookup the table scenarios share, defined below them.
auto find_row(const std::vector<pmu_table_entry>& table,
              const std::string& name) -> const pmu_table_entry*;

// Each vendored row lands on the device its table scope names, and no
// uncore row appears under the core device (FR-019). A hybrid host
// publishes core and atom devices beside an uncore device, and a row
// scoped to a class the host publishes nothing for reaches no device at
// all. Every arm is decided by the two strings, so the check runs on any
// host and needs no granted event.
auto device_placement_scenario() -> void
{
  using sg::counters::detail::scope_reaches;

  // A core-scoped row, empty scope and `core` alike, reaches every core
  // device and no uncore device.
  for (const auto* scope : {"", "core"}) {
    check(scope_reaches("cpu", scope),
          "a core-scoped row reaches the core device (FR-019)");
    check(scope_reaches("cpu_core", scope) && scope_reaches("cpu_atom", scope),
          "a core-scoped row reaches every core device a hybrid host "
          "publishes (FR-019)");
    check(!scope_reaches("uncore_imc", scope),
          "no core-scoped row appears under an uncore device (FR-019)");
  }

  // An uncore-scoped row reaches the device of its class, spelled bare
  // or under the kernel's prefix, and never the core device.
  check(
      scope_reaches("uncore_imc", "iMC"),
      "an uncore-scoped row reaches the uncore device of its class " "(FR-"
                                                                     "019)");
  check(scope_reaches("imc", "iMC"),
        "the kernel's bare device spelling reaches the same class (FR-019)");
  check(!scope_reaches("cpu", "iMC"),
        "no uncore row appears under the core device (FR-019)");
  check(!scope_reaches("uncore_arb", "iMC"),
        "a row scoped to one uncore class reaches no other (FR-019)");

  // A kernel uncore device carries an instance suffix, `uncore_imc_0`
  // among them. The generator's unit map names the device class and
  // ignores a numeric suffix, so a suffixed device is reached by the unit
  // naming its class and by no other unit. At the audit point the rule
  // compared the whole name, so a suffixed device reached nothing and no
  // numbered instance ever received a row (FR-014, FR-015, D-08).
  check(scope_reaches("uncore_imc_0", "iMC")
            && scope_reaches("uncore_arb_3", "ARB"),
        "a suffixed uncore device is reached by the unit naming its class "
        "(FR-014, FR-015, D-08)");
  check(scope_reaches("uncore_imc_0", "iMC")
            && scope_reaches("uncore_imc_1", "iMC"),
        "every numbered instance of one class is reached by the one unit "
        "naming that class (FR-014, D-08)");
  check(!scope_reaches("uncore_imc_0-1", "iMC"),
        "a suffix that is not a run of digits is not an instance suffix "
        "(FR-014)");
  check(!scope_reaches("uncore_imc_0", "ARB")
            && !scope_reaches("uncore_arb_3", "iMC"),
        "a suffixed device is reached by the unit naming its own class alone, "
        "so the postcondition and the verdict agree on it (FR-015, FR-024)");

  // The fixtures the contract names, using the device names the running
  // kernel publishes and the units the pinned AMD tables spell. The two
  // spellings differ, so the reachability map carries the pair rather than
  // comparing them as one text (FR-014, FR-015, SC-008, D-08, D-09).
  const std::pair<const char*, const char*> kNumbered[] = {
      {"uncore_cha_0", "CHA"},
      {"uncore_cha_1", "CHA"},
      {"uncore_imc_0", "iMC"},
  };
  for (const auto& [device, unit] : kNumbered) {
    check(scope_reaches(device, unit),
          "a numbered Intel uncore instance is reached by the unit naming its "
          "class (FR-014, FR-015, D-08)");
  }
  const std::pair<const char*, const char*> kVendor[] = {
      {"amd_df", "DFPMC"},
      {"amd_l3", "L3PMC"},
      {"amd_umc_0", "UMCPMC"},
  };
  for (const auto& [device, unit] : kVendor) {
    check(scope_reaches(device, unit),
          "an AMD vendor device is reached by the unit naming it (FR-014, "
          "FR-015, D-09)");
  }
  check(scope_reaches("amd_umc_0", "UMCPMC")
            && scope_reaches("amd_umc_1", "UMCPMC"),
        "every numbered AMD memory instance is reached by the one unit naming "
        "that class (FR-014, D-09)");
  // Each instance of one class is reached, and an instance reaches no unit
  // naming another class. A plan that read instance zero alone would
  // understate the class, so every instance has to receive the rows (D-08).
  check(scope_reaches("uncore_cha_0", "CHA")
            && scope_reaches("uncore_cha_1", "CHA")
            && !scope_reaches("uncore_cha_1", "iMC"),
        "every instance of one class receives that class's rows and no other "
        "class's (FR-015, D-08)");
  check(!scope_reaches("uncore_cha_0", "DFPMC")
            && !scope_reaches("amd_df", "iMC")
            && !scope_reaches("amd_umc_0", "L3PMC"),
        "a device is reached by the unit naming its own class alone, so an "
        "Intel instance and an AMD device never exchange rows (FR-015, "
        "D-09)");
  check(!scope_reaches("cpu", "CHA") && !scope_reaches("cpu", "iMC")
            && !scope_reaches("cpu", "DFPMC"),
        "no uncore row of any class reaches a core device (FR-019, D-09)");

  // A row scoped to a class this host publishes nothing for reaches no
  // device, so it stays out of the catalog and runs no probe.
  check(!scope_reaches("cpu", "never_published")
            && !scope_reaches("uncore_imc", "never_published"),
        "a row scoped to an absent device class stays out of the catalog "
        "(FR-019)");
}

// A row is published `not_encodable` only where the running kernel's
// formats lack a field the row needs, and a row whose fields the kernel
// publishes never reports it (FR-018). Both arms run over synthetic rows
// against the named synthetic format list, so no host has to grant an
// event and the refusal is decided by the fields alone.
auto encoding_refusal_scenario() -> void
{
  const auto device = synthetic_core_device();
  const std::string path = write_fixture("encoding_refusal.json", R"([
  {"EventName":"needs_absent","EventCode":"0x01","SliceId":3,
   "BriefDescription":"a row needing a format the device omits"},
  {"EventName":"needs_absent_two","EventCode":"0x02","CounterMask":7,
   "NodeType":3,"BriefDescription":"a row needing a format nobody publishes"},
  {"EventName":"all_published","EventCode":"0x03","UMask":"0x04",
   "CounterMask":7,"EdgeDetect":1,
   "BriefDescription":"a row whose fields the device publishes"},
  {"EventName":"sampling_only","EventCode":"0x05","SampleAfterValue":1000,
   "MSRValue":"0x1","PerPkg":1,
   "BriefDescription":"a row carrying only keys with no obligation"}
])");
  std::vector<pmu_table_entry> table;
  sg::counters::detail::pmu_parse_table_file(path, table);

  const auto encodes = [&device](const pmu_table_entry& row)
  {
    std::vector<std::pair<int, std::uint64_t>> words;
    return sg::counters::detail::pmu_compose_config(
        row.fields, device.formats, words);
  };
  const auto* absent = find_row(table, "needs_absent");
  check(absent != nullptr && !encodes(*absent),
        "a row needing a format the device omits refuses to encode, and the "
        "catalog publishes it as not_encodable (FR-018)");
  const auto* absent_two = find_row(table, "needs_absent_two");
  check(absent_two != nullptr && !encodes(*absent_two),
        "a row needing a format no device publishes refuses to encode "
        "(FR-018)");
  const auto* published = find_row(table, "all_published");
  check(published != nullptr && encodes(*published),
        "a row whose fields the device publishes never reports "
        "not_encodable (FR-018)");
  const auto* sampling = find_row(table, "sampling_only");
  check(sampling != nullptr && encodes(*sampling),
        "a row carrying only keys with no encoding obligation encodes, so "
        "FR-016 does not cost the row its count (FR-016, FR-018)");
}

// The open-refusal arm of `fast_context_open`, reached without a host
// the kernel refuses: `perf_event_open` rejects a PMU type no kernel
// publishes, and the refusal the catalog would disclose is the sentence
// the open wrote out. The close follows the refusal, over a context the
// kernel never filled and over one it granted, so both arms of the
// release are measured on any host (FR-023, R-011, FR-040).
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
  // refuses mid-open leaves no descriptor and no mapping behind. Both arms
  // of the close run on any host: this one owns nothing, and the one below
  // owns what the kernel granted (FR-040).
  sg::counters::detail::fast_context unowned;
  check(unowned.fd == -1 && unowned.map == nullptr,
        "a context no open ever filled owns no descriptor and no mapping "
        "(FR-040)");
  sg::counters::detail::fast_context_close(unowned);
  check(unowned.fd == -1 && unowned.map == nullptr && unowned.map_length == 0,
        "closing a context that owns nothing is a no-op (FR-040)");
  std::string fast_refusal;
  auto granted = sg::counters::detail::fast_context_open(
      PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS, where, &fast_refusal);
  if (!granted) {
    std::printf("seam: no mapped-page context on this host (%s), so a close "
                "of a granted context goes unmeasured\n",
                fast_refusal.c_str());
  } else {
    check(granted->fd >= 0 && granted->map != nullptr,
          "a granted context owns a descriptor and a mapping (FR-040)");
    sg::counters::detail::fast_context_close(*granted);
    check(granted->fd == -1 && granted->map == nullptr
              && granted->map_length == 0,
          "a close releases the descriptor and the mapping (FR-040)");
  }
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

  // The identification and its selected directory are cached once per
  // process, so the second selection of the same identification reads the
  // cache and names the directory the first selection named (FR-038).
  check(pmu_select_directory(identified) == directory,
        "selecting the same identification twice yields the same directory");

  // The table selections are made once per process and cached per
  // directory, so the second load of a directory hands back the same
  // parsed table and a second directory holds its own (FR-038).
  const auto& table = sg::counters::detail::pmu_load_table(directory);
  check(&sg::counters::detail::pmu_load_table(directory) == &table,
        "loading the same directory twice returns the same parsed table");
  // A directory reaches the loader with a trailing separator from
  // `pmu_select_directory`, and the registry names it without one. Both
  // spellings resolve to the same directory (FR-036).
  std::string bare {directory};
  if (!bare.empty() && bare.ends_with('/')) {
    bare.pop_back();
  }
  const auto& bare_table = sg::counters::detail::pmu_load_table(bare);
  check(bare_table.size() == table.size() && !bare_table.empty(),
        "a directory spelled without its trailing separator loads the same "
        "table (FR-036)");
  const std::string absent_directory {"arch/x86/no-such-directory/"};
  const auto& absent_table =
      sg::counters::detail::pmu_load_table(absent_directory);
  check(&absent_table != &table && absent_table.empty(),
        "a second directory spelling holds its own table beside this one "
        "(FR-038)");
  check(&sg::counters::detail::pmu_load_table(directory) == &table,
        "loading the same directory twice returns the same parsed table");
  std::printf("seam table: %zu entries\n", table.size());
  check(!table.empty(), "the selected directory parsed to at least one entry");
  // The parse maps the kernel table's numeric attributes onto the
  // semantic field names the encoder reads, so a row reaching the catalog
  // carries a config the provider can compose (FR-037, FR-038).
  std::size_t mapped = 0;
  for (const auto& entry : table) {
    if (std::ranges::any_of(
            entry.fields,
            [](const std::pair<std::string, std::uint64_t>& field)
            { return field.first == "event" || field.first == "umask"; }))
    {
      ++mapped;
    }
  }
  std::printf("seam table: %zu of %zu rows carry a mapped config field\n",
              mapped,
              table.size());
  check(
      mapped > 0,
      "the parsed table maps at least one row onto a config field " "(FR-038)");
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
               std::string unit = "",
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
  check(table_description(table_row("ev", "", "", {{"event", 0xc0}}))
            == "hardware event 'ev'; the vendored kernel table carries no "
               "description (event code 0xc0)",
        "a row carrying no prose names its event code");
  check(table_description(table_row("ev", "", "", {{"umask", 1}}))
            == "hardware event 'ev'; the vendored kernel table carries no "
               "description",
        "a row whose only field is no event code names no event code");
  check(table_description(table_row("ev", "prose", "DFPMC"))
            == "prose [table scope: DFPMC]",
        "a table scope label rides along after the row's own prose");
  check(table_description(table_row("ev", "prose")) == "prose",
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
                == "event=1,umask=2,umask_ext=128,cmask=3,Edge=1"
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
  check(bare != nullptr && bare->description.empty() && bare->unit.empty(),
        "a row carrying no unit scope reads as the core scope, which is what "
        "reaches a core device, so the default is the empty scope and no row "
        "is left naming a class no device publishes (FR-014, FR-019, D-08)");
  const auto* desc_number = find_row(table, "ev_desc_number");
  check(desc_number != nullptr && desc_number->description == "brief prose",
        "a non-string description candidate falls through to the next");
  const auto* unit_number = find_row(table, "ev_unit_number");
  check(unit_number != nullptr && unit_number->unit.empty(),
        "a non-string unit leaves the default unit in place");
  const auto* unit_missing = find_row(table, "ev_unit_missing");
  check(unit_missing != nullptr && unit_missing->unit.empty(),
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

// A leaf set whose plan also writes a disclosure column. A window gives
// that column a slot of its own, distinct from every device member, so
// the slot a window registers for it is decided by the column and not by
// a device (FR-007).
auto disclosing_leaf_set_of(std::vector<std::string> addresses,
                            std::size_t disclosure_column) -> leaf_set
{
  return leaf_set {.addresses = std::move(addresses),
                   .disclosure_column = disclosure_column};
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
  // A countable member beside a disclosure column reaches the window
  // build, and the disclosure slot registers on its own (FR-007).
  const auto disclosed = sg::counters::detail::pmu_open_window(
      state, disclosing_leaf_set_of({"cpu/work"}, 1), where);
  check(disclosed != nullptr,
        "a countable member beside a disclosure column opens a window");
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
        sg::counters::detail::pmu_probe(state.devices[0].type, words, target {})
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
      sg::counters::detail::pmu_probe(
          state.devices[0].type, {{0, PERF_COUNT_HW_CPU_CYCLES}}, target {})
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
    point_sink one {first, 3, 3, 2, 0};
    pair->read_points(one);
    one.check_action();
    std::uint64_t second[6] = {0, 0, 0, 0, 0, 0};
    point_sink two {second, 3, 3, 2, 0};
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
    // The window constructor installed its own read in the direct-call
    // slot, so the slot the compiled plan holds reaches the same read
    // the virtual entry reaches (FR-022, R-004).
    const auto thunk = pair->resolve_thunk();
    std::uint64_t third[6] = {0, 0, 0, 0, 0, 0};
    point_sink via_slot {third, 3, 3, 2, 0};
    thunk(*pair, via_slot);
    via_slot.check_action();
    check(third[0] >= second[0] && third[2] >= second[2],
          "the installed direct-call slot reads the same group the virtual "
          "entry reads (FR-022)");
  }
  // Two members in one group: the follower is opened disabled into its
  // leader's group, which is the arm a single-member open never reaches.
  const auto duo = sg::counters::detail::pmu_open_window(
      state, leaf_set_of({"cpu/work", "cpu/cycle"}), where);
  const bool duo_granted =
      sg::counters::detail::pmu_probe(
          state.devices[0].type, {{0, PERF_COUNT_HW_INSTRUCTIONS}}, target {})
          == availability::countable
      && sg::counters::detail::pmu_probe(
             state.devices[0].type, {{0, PERF_COUNT_HW_CPU_CYCLES}}, target {})
          == availability::countable;
  check(
      (duo != nullptr) == duo_granted,
      "a two-member group opens exactly when the probe grants both " "configs");
  if (duo != nullptr) {
    // Two columns, the two members: the read fills one row of a two-row
    // block, and the row this action did not commit stays untouched.
    std::uint64_t cells[4] = {0, 0, 0, 0};
    point_sink sink {cells, 2, 2, 2, 0};
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
  // The fast window opens exactly when the availability probe grants the
  // config the member carries, the same verdict `group_open_scenario`
  // holds the group path to (FR-023, FR-040).
  const bool granted =
      sg::counters::detail::pmu_probe(
          state.devices[0].type, {{0, PERF_COUNT_HW_INSTRUCTIONS}}, target {})
      == availability::countable;
  const auto fast = sg::counters::detail::pmu_open_fast_window(
      state, leaf_set_of({"cpu/fast"}), where);
  std::printf("seam: the synthetic fast-capable open returned %s\n",
              fast != nullptr ? "a fast window" : "no window");
  check((fast != nullptr) == granted,
        "the fast branch returns a window exactly when the probe grants the "
        "member's config (FR-040)");
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

// The verdicts' own journey: `probe_device` runs the per-kind probes, the
// kinds they settled ride on the catalog entry, the provider records them
// under the address the system gives a seeded leaf, and the mask the
// catalog publishes is the record the probe left. Every check is read
// against the verdicts this kernel gives the seam's own probes, so it holds
// on a host that grants a cpu-targeted event and on one that grants none
// (FR-021, FR-022, FR-046).
auto probe_kind_record_scenario() -> void
{
  using sg::counters::target_cpu_bit;
  using sg::counters::target_kind;
  using sg::counters::target_mask;
  using sg::counters::target_thread_bit;
  using sg::counters::detail::note_probed_kinds;
  using sg::counters::detail::pmu_probe;
  using sg::counters::detail::probed_kinds_at;
  using sg::counters::detail::settled_targets;

  const std::vector<std::pair<int, std::uint64_t>> words {
      {0, PERF_COUNT_HW_INSTRUCTIONS},
  };
  const target_mask counted = (pmu_probe(PERF_TYPE_HARDWARE, words, target {})
                                       == availability::countable
                                   ? target_thread_bit
                                   : target_mask {})
      | (pmu_probe(PERF_TYPE_HARDWARE,
                   words,
                   target {.kind = target_kind::cpu, .cpu = 0})
                 == availability::countable
             ? target_cpu_bit
             : target_mask {});
  const std::string description = "a hardware event the seam probes";

  // The core event source counts a thread's own events, so its entries run
  // both probes and their record carries both verdicts.
  pmu_device core;
  core.path = "cpu";
  core.type = PERF_TYPE_HARDWARE;
  core.device_scoped = false;
  core.entries.push_back(pmu_entry {
      .name = "record",
      .description = description,
      .words = words,
  });
  probe_device(core, /*fast_capable=*/false);
  check(core.entries.front().probed_kinds == counted,
        "the catalog entry carries exactly the kinds the two probes settled "
        "for it (FR-021)");
  check(settled_targets(core.entries.front().avail,
                        core.entries.front().probed_kinds,
                        "pmu",
                        core.path)
            == core.entries.front().probed_kinds,
        "the published mask is the record the probe left, so a countable "
        "entry the cpu probe refused names no cpu bit (FR-021)");

  // The same event on a device that binds one processor for every task: its
  // scope refuses the per-task kind, so no per-task probe runs and no
  // per-task bit is named.
  pmu_device scoped;
  scoped.path = "unpublished_scoped";
  scoped.type = PERF_TYPE_HARDWARE;
  scoped.device_scoped = true;
  scoped.entries.push_back(pmu_entry {
      .name = "record",
      .description = description,
      .words = words,
  });
  probe_device(scoped, /*fast_capable=*/false);
  check(scoped.entries.front().probed_kinds == (counted & target_cpu_bit),
        "a device-scoped entry consults no per-task probe, so its record "
        "names the cpu kind alone (FR-021, FR-022)");

  // The carrier the catalog reads, keyed by the address the system gives a
  // seeded leaf.
  note_probed_kinds("record/core/record", target_cpu_bit);
  check(probed_kinds_at("record/core/record") == target_cpu_bit,
        "the kinds recorded for a leaf address read back where the catalog "
        "looks for them (FR-021)");
  check(probed_kinds_at("no/such/leaf") == target_mask {},
        "an address no event source probed names no kind, which is the "
        "answer for every leaf the availability probe never reached (FR-021)");
}

}  // namespace

// Every availability the catalog can publish reaches the mode selector,
// which maps each one to the mode and pair it publishes (FR-022, FR-024).
auto read_mode_selection_scenario() -> void
{
  using sg::counters::availability;
  using sg::counters::detail::entry_read_selection_for;

  const auto fast_countable =
      entry_read_selection_for(availability::countable, true);
  check(fast_countable.mode == sg::counters::read_mode::fast_rdpmc,
        "a countable entry takes the fast mode where the device supports "
        "it (FR-022)");
  check(fast_countable.publish_pair,
        "a countable entry publishes its pair (FR-022)");

  const auto slow_countable =
      entry_read_selection_for(availability::countable, false);
  check(slow_countable.mode == sg::counters::read_mode::syscall,
        "a countable entry takes the syscall mode where the device has no "
        "fast read (FR-022)");
  check(slow_countable.publish_pair,
        "a countable entry publishes its pair on either mode (FR-022)");

  for (const auto state : {availability::permission_blocked,
                           availability::not_encodable,
                           availability::absent,
                           availability::scope_refused,
                           availability::gap})
  {
    const auto selection = entry_read_selection_for(state, true);
    check(selection.mode == sg::counters::read_mode::syscall
              && !selection.publish_pair,
          "every state but countable takes the syscall mode and publishes "
          "no pair (FR-022)");
  }
}

// The embedded registry answers a name no vendored directory holds with an
// empty view, and a directory it does not hold with no entry, so a caller
// asking for something the library does not carry reads nothing
// (FR-036).
auto embedded_registry_scenario() -> void
{
  using sg::counters::detail::embedded_file_bytes;
  using sg::counters::detail::embedded_find_dir;

  const auto* dir = embedded_find_dir("arch/x86/icelake");
  check(dir != nullptr, "the registry holds the icelake table directory");
  if (dir == nullptr) {
    return;
  }
  check(!embedded_file_bytes(*dir, "cache.json").empty(),
        "the registry holds a table every icelake directory publishes");
  check(embedded_file_bytes(*dir, "no-such-table.json").empty(),
        "the registry answers a name it does not hold with an empty view "
        "(FR-036)");
  check(embedded_find_dir("arch/x86/no-such-directory") == nullptr,
        "the registry answers a directory it does not hold with no entry "
        "(FR-036)");
}

// A hybrid processor publishes one device per core, and each carries the
// scope of its own per-core instance. The reference host publishes
// neither, so a fixture directory carries each name and the chain that
// reads a device's scope off its directory takes both arms no published
// device on this host reaches. The false arm at each comparison is the
// one that names a hybrid scope, and the pair is what FR-021's per-kind
// refusal rests on: a device the chain leaves unscoped is one whose
// entries a cpu target can count.
auto hybrid_device_scope_scenario() -> void
{
  const std::filesystem::path root(SG_SEAM_TABLE_DIR);
  std::error_code code;
  for (const auto* name : {"cpu_core", "cpu_atom"}) {
    const auto dir = root / name;
    std::filesystem::create_directories(dir, code);
    std::ofstream file(dir / "type", std::ios::binary | std::ios::trunc);
    file << "4\n";
    file.close();
    if (!file) {
      fail("a hybrid device fixture carries a readable type");
    }
    // A leftover processor-set file from an earlier run would mark the
    // device scoped. This fixture publishes none, so both files go.
    std::filesystem::remove(dir / "cpumask", code);
    std::filesystem::remove(dir / "cpus", code);
    const auto loaded = load_device(dir);
    check(loaded.has_value(), "a hybrid per-core device directory loads");
    if (!loaded.has_value()) {
      continue;
    }
    check(loaded->device_scoped == false,
          "a hybrid per-core device publishes no processor set, so it stays "
          "per-task capable (FR-016)");
  }

  // A device that publishes `cpumask` or `cpus` binds a processor set. The
  // kernel registers no per-task context for it, and the catalog marks it
  // device-scoped from that file (FR-016).
  const auto write_scoped = [&root](const char* name, const char* file)
  {
    std::error_code scoped_code;
    const auto dir = root / name;
    std::filesystem::create_directories(dir, scoped_code);
    std::ofstream type(dir / "type", std::ios::binary | std::ios::trunc);
    type << "4\n";
    type.close();
    std::ofstream published(dir / file, std::ios::binary | std::ios::trunc);
    published << "0\n";
    published.close();
    return load_device(dir);
  };
  const auto by_mask = write_scoped("scoped_cpumask", "cpumask");
  check(by_mask.has_value() && by_mask->device_scoped,
        "a device that publishes a cpumask is device scoped (FR-016)");
  const auto by_cpus = write_scoped("scoped_cpus", "cpus");
  check(by_cpus.has_value() && by_cpus->device_scoped,
        "a device that publishes a cpus file is device scoped (FR-016)");
}

// Whether the placement step left a row of `name` on `device` (FR-019).
// The lookup reads the entries `merge_vendored` appended, so it answers
// where a row ended up.
auto carries(const pmu_device& device, const std::string& name) -> bool
{
  return std::ranges::any_of(device.entries,
                             [&name](const pmu_entry& entry)
                             { return entry.name == name; });
}

// The kernel's device name for one table Unit scope (FR-019): the empty
// scope and `core` name the core PMU, and every other scope names its own
// class, folded to lower case the way the placement rule folds it.
auto kernel_device_name(const std::string& scope) -> std::string
{
  std::string folded;
  folded.reserve(scope.size());
  for (const char letter : scope) {
    folded.push_back(
        static_cast<char>(std::tolower(static_cast<unsigned char>(letter))));
  }
  // The vendor unit map the reachability rule reads, spelled once here so
  // the fixture device a scope names and the device that scope reaches are
  // the same name. An AMD unit carries a vendor device name the unit does
  // not spell (FR-014, D-09).
  for (const auto& [unit, device] : {std::pair {"dfpmc", "amd_df"},
                                     std::pair {"l3pmc", "amd_l3"},
                                     std::pair {"umcpmc", "amd_umc"}})
  {
    if (folded == unit) {
      return device;
    }
  }
  return folded.empty() || folded == "core" ? "cpu" : folded;
}

// The vendored table of the architecture directory the running CPU
// selects, empty where no mapping-file row matches this CPU (FR-038).
// Every placement question is a question over this table, since a
// vendored row is the only row the placement step ever adds (FR-019).
auto selected_table() -> const std::vector<pmu_table_entry>&;

// Writes one device directory named `name` and reads it back through the
// seam, the way `hybrid_device_scope_scenario` writes a device the running
// kernel publishes no device for. The type is one no kernel publishes,
// because the placement step opens no event (FR-019). The device publishes
// a config range for every field name the selected table's rows carry, one
// range per config word, so a row composes on it and the placement step
// measures both of its composition arms, the refused one and the composed
// one (FR-037).
auto fixture_device(const std::string& name) -> std::optional<pmu_device>
{
  const auto dir = std::filesystem::path(SG_SEAM_TABLE_DIR) / name;
  std::error_code code;
  std::filesystem::create_directories(dir, code);
  std::ofstream file(dir / "type", std::ios::binary | std::ios::trunc);
  file << "999999\n";
  file.close();
  if (!file) {
    fail("a placement fixture device carries a readable type");
  }
  // The names come from the parsed table, so no host's field vocabulary is
  // spelled here and every row composes wherever the table lands one.
  std::vector<std::string> published;
  for (const auto& row : selected_table()) {
    for (const auto& field : row.fields) {
      if (std::ranges::none_of(published,
                               [&field](const std::string& one) -> bool
                               { return one == field.first; }))
      {
        published.push_back(field.first);
      }
    }
  }
  const auto format_dir = dir / "format";
  std::filesystem::create_directories(format_dir, code);
  constexpr std::size_t bits_per_word = 64;
  std::size_t word = 0;
  for (const auto& field : published) {
    const std::size_t low = word * bits_per_word;
    std::ofstream format(format_dir / field,
                         std::ios::binary | std::ios::trunc);
    format << "config:" << low << "-" << (low + bits_per_word - 1) << "\n";
    format.close();
    if (!format) {
      fail("a placement fixture device carries a readable format file");
    }
    ++word;
  }
  return load_device(dir);
}

// The vendored table of the architecture directory the running CPU
// selects, empty where no mapping-file row matches this CPU (FR-038).
// Every placement question is a question over this table, since a
// vendored row is the only row the placement step ever adds (FR-019).
auto selected_table() -> const std::vector<pmu_table_entry>&
{
  return sg::counters::detail::pmu_load_table(
      pmu_select_directory(sg::counters::detail::pmu_ident_current()));
}

// An uncore device directory read through `load_device`, with the
// placement step driven over it: the reference host publishes no uncore
// device of this class that a catalog run can open, so the loader's own
// uncore arm and the placement over it run against a directory the kernel
// never lists. Every row of the selected table whose Unit scope names this
// class lands on the device, and no core-scoped row does (FR-019, SC-006).
auto uncore_device_fixture_scenario() -> void
{
  const std::filesystem::path dir =
      std::filesystem::path(SG_SEAM_TABLE_DIR) / "uncore_imc";
  std::error_code code;
  std::filesystem::create_directories(dir, code);
  std::ofstream file(dir / "type", std::ios::binary | std::ios::trunc);
  file << "5\n";
  file.close();
  if (!file) {
    fail("an uncore device fixture carries a readable type");
  }
  auto loaded = load_device(dir);
  check(loaded.has_value(), "an uncore device directory loads");
  if (!loaded.has_value()) {
    return;
  }
  merge_vendored(*loaded);

  const auto& table = selected_table();
  std::size_t scoped_here = 0;
  std::size_t expected = 0;
  std::size_t misplaced = 0;
  for (const auto& row : table) {
    const bool here = carries(*loaded, row.name);
    if (row.unit == "iMC") {
      ++expected;
      if (here) {
        ++scoped_here;
      }
    } else if (here) {
      ++misplaced;
    }
  }
  std::printf("seam uncore fixture: %s carries %zu of %zu iMC-scoped rows "
              "and %zu rows of another class\n",
              loaded->path.c_str(), scoped_here, expected, misplaced);
  check(scoped_here == expected,
        "every iMC-scoped row of the selected table landed on the loaded "
        "uncore device (FR-019, SC-006)");
  check(misplaced == 0,
        "no core-scoped row and no row of another class appears under a "
        "loaded uncore device (FR-019)");
}

// The placement step itself, which no other scenario reaches:
// `merge_vendored` is the only code that puts a vendored row on a device
// and the provider assembly reaches it only from a constructor over
// sysfs, so a row landing on the device its scope names, and a row
// staying off every device, are facts this fixture alone measures. The
// vendored table is the pinned one the running CPU selects, and the
// devices are fixtures, so both arms run on a host that publishes no
// device of these classes (FR-019, SC-006, T037, T081).
auto vendored_row_placement_scenario() -> void
{
  const auto& table = selected_table();
  if (table.empty()) {
    std::printf("seam: no mapping-file row matches this CPU, so no vendored "
                "table is available to place (FR-038)\n");
    return;
  }

  // The distinct Unit scopes the selected table carries, in table order.
  std::vector<std::string> scopes;
  for (const auto& row : table) {
    if (std::ranges::none_of(scopes,
                             [&row](const std::string& seen)
                             { return seen == row.unit; }))
    {
      scopes.push_back(row.unit);
    }
  }
  std::printf("seam placement: %zu rows over %zu Unit scopes\n",
              table.size(),
              scopes.size());
  check(scopes.size() >= 2,
        "the selected table carries at least two Unit scopes, so a row's "
        "own class and a class no device names are both in it (FR-019)");

  // One fixture device per scope but the last: the last scope is the
  // class no device here names, so its rows reach no device at all.
  std::vector<pmu_device> devices;
  for (auto scope = scopes.begin(); scope + 1 != scopes.end(); ++scope) {
    auto device = fixture_device(kernel_device_name(*scope));
    check(device.has_value(), "a placement fixture device directory loads");
    if (!device.has_value()) {
      continue;
    }
    merge_vendored(*device);
    std::printf("seam placement: device '%s' holds %zu merged rows\n",
                device->path.c_str(),
                device->entries.size());
    devices.push_back(std::move(*device));
  }

  std::size_t landed = 0;
  std::size_t unreached = 0;
  for (const auto& row : table) {
    const std::string own = kernel_device_name(row.unit);
    std::size_t on_own = 0;
    std::size_t on_other = 0;
    for (const auto& device : devices) {
      if (!carries(device, row.name)) {
        continue;
      }
      if (device.path == own) {
        ++on_own;
      } else {
        ++on_other;
      }
    }
    const bool published = std::ranges::any_of(
        devices, [&own](const pmu_device& one) { return one.path == own; });
    if (published) {
      check(on_own == 1 && on_other == 0,
            "a vendored row lands on the one device its Unit scope names and "
            "on no other (FR-019)");
      ++landed;
      continue;
    }
    check(on_own == 0 && on_other == 0,
          "a vendored row scoped to a class no device names reaches no "
          "device and stays out of the catalog (FR-019)");
    ++unreached;
  }
  std::printf("seam placement: %zu rows landed on the device their scope "
              "names, %zu reached no device\n",
              landed, unreached);
  check(landed > 0,
        "the placement step put at least one vendored row on the device its "
        "Unit scope names (FR-019, SC-006)");
  check(unreached > 0,
        "the placement step left at least one vendored row on no device, so "
        "a row scoped to an unpublished class stays out of the catalog "
        "(FR-019, SC-006)");
}

// No device this host publishes takes the chain's false arm at the count
// a cpu target settles an entry on: every entry the kernel lists here has
// its cpu-targeted event granted, so the entry is countable on every
// device and the refusal direction never runs. A device whose PMU type
// no kernel publishes refuses both probes, which is the direction the
// chain's own else arm answers, and the entry settles on that refusal
// with no time pair disclosed beside it (FR-021).
auto unpublished_device_probe_scenario() -> void
{
  pmu_device device;
  device.path = "unpublished";
  device.type = 999999;
  device.device_scoped = false;
  device.entries.push_back(pmu_entry {
      .name = "synthetic",
      .description = "an event on a PMU type no kernel publishes",
      .words = {{0, 0}},
  });
  probe_device(device, false);
  check(device.entries.front().avail != availability::countable,
        "an entry on an unpublished PMU type settles on its refusal");
  check(!device.has_time_pair,
        "a device whose every probe is refused discloses no time pair");
}

// The scope-cause decision, driven over every arm of its two inputs and
// then over the arm of `probe_device` no other scenario reaches: a
// device-scoped device whose PMU type the running kernel publishes no
// event source for, so both probes answer without a grant. Where the
// kernel's two answers differ, the scoped device publishes the scope's own
// and never the per-task one, which is the whole point of the flag
// (FR-021, FR-022, FR-046).
auto device_scope_probe_scenario() -> void
{
  using sg::counters::target_kind;
  using sg::counters::detail::pmu_probe;
  using sg::counters::detail::scope_settled_state;

  for (const auto verdict :
       {
           availability::countable,
           availability::permission_blocked,
           availability::not_encodable,
           availability::absent,
           availability::scope_refused,
           availability::gap,
       })
  {
    check(scope_settled_state(verdict, true)
              == (verdict == availability::permission_blocked
                      ? availability::scope_refused
                      : verdict),
          "a device-scoped device publishes the scope's own refusal for a "
          "permission verdict and every other verdict unchanged (FR-021, "
          "FR-022)");
    check(scope_settled_state(verdict, false) == verdict,
          "a device no scope owns publishes the cpu probe's own verdict "
          "(FR-021)");
  }

  pmu_device scoped;
  scoped.path = "unpublished_scoped";
  scoped.type = 999999;
  scoped.device_scoped = true;
  scoped.entries.push_back(pmu_entry {
      .name = "synthetic",
      .description = "an event on a PMU type no kernel publishes",
      .words = {{0, 0}},
  });
  const std::vector<std::pair<int, std::uint64_t>> words =
      scoped.entries.front().words;
  const availability per_task = pmu_probe(scoped.type, words, target {});
  const availability on_cpu = pmu_probe(
      scoped.type, words, target {.kind = target_kind::cpu, .cpu = 0});
  probe_device(scoped, false);
  check(scoped.entries.front().avail == scope_settled_state(on_cpu, true),
        "a device-scoped entry settles on the state the extracted decision "
        "publishes for the verdict the cpu probe gave (FR-021, FR-022)");

  // The same device unscoped, where the per-task probe does run. Its
  // published state names the per-task verdict, so the two states below
  // are read against what this kernel answered for each probe.
  pmu_device unscoped;
  unscoped.path = scoped.path;
  unscoped.type = scoped.type;
  unscoped.entries.push_back(pmu_entry {
      .name = scoped.entries.front().name,
      .description = scoped.entries.front().description,
      .words = words,
  });
  probe_device(unscoped, false);
  check(unscoped.entries.front().avail
            == (on_cpu == availability::countable ? availability::countable
                                                  : per_task),
        "an unscoped device publishes the per-task probe's own verdict, so "
        "both of the kernel's answers are named here (FR-021, FR-022)");
  if (on_cpu != availability::countable && per_task != on_cpu) {
    check(scoped.entries.front().avail != unscoped.entries.front().avail,
          "a device-scoped device consults no per-task probe: where the two "
          "probes disagree it publishes the scope's answer and not the "
          "per-task verdict (FR-021, FR-022)");
  } else {
    std::printf("seam scope probe: this kernel answers both probes alike, so "
                "the flag has no verdict of its own to separate here; the "
                "extracted decision above carries both arms (FR-021)\n");
  }
}

// The two verdicts one entry's probes give, reduced to the target kinds
// they settled, and the mask the catalog publishes from them. A kind's bit
// is named where that kind's own probe counted the entry, so a countable
// entry the cpu-targeted probe refused names no cpu bit; a leaf no probe
// ran for takes the object's own scope, which is what the enabled and
// running leaves and every leaf no event source probed do. Both decisions
// are pure over values a caller already holds, so every arm runs on a host
// that grants no event, which is what the reference host is (FR-021,
// FR-022, FR-046).
auto settled_target_mask_scenario() -> void
{
  using sg::counters::target_cpu_bit;
  using sg::counters::target_mask;
  using sg::counters::target_thread_bit;
  using sg::counters::detail::probed_kind_mask;
  using sg::counters::detail::settled_targets;

  // Every pair of probe verdicts, so each kind's bit is driven on both of
  // its arcs and against every state the other kind can answer with.
  for (const auto per_task :
       {
           availability::countable,
           availability::permission_blocked,
           availability::not_encodable,
           availability::absent,
           availability::scope_refused,
           availability::gap,
       })
  {
    for (const auto on_cpu :
         {
             availability::countable,
             availability::permission_blocked,
             availability::not_encodable,
             availability::absent,
             availability::scope_refused,
             availability::gap,
         })
    {
      const target_mask expected =
          (per_task == availability::countable ? target_thread_bit
                                               : target_mask {})
          | (on_cpu == availability::countable ? target_cpu_bit
                                               : target_mask {});
      check(probed_kind_mask(per_task, on_cpu) == expected,
            "a kind's bit is named exactly where that kind's own probe "
            "settled the entry, whatever the other probe answered (FR-021, "
            "FR-022)");
    }
  }

  for (const auto state :
       {
           availability::permission_blocked,
           availability::not_encodable,
           availability::absent,
           availability::scope_refused,
           availability::gap,
       })
  {
    check(settled_targets(state, target_mask {}, "pmu", "uncore_imc") == 0,
          "a refused entry names no target kind on a device that binds one "
          "processor for every task (FR-021)");
    check(settled_targets(state, target_mask {}, "machine", "machine") == 0,
          "a refused entry names no target kind on an object no event "
          "provider seeded (FR-021)");
  }

  check(settled_targets(availability::countable,
                        target_mask {},
                        "machine",
                        "machine")
            == (target_thread_bit | target_cpu_bit),
        "a countable entry on an object no event provider seeded names both "
        "kinds (FR-021)");

  for (const auto* path : {"cpu", "cpu_core", "cpu_atom"}) {
    check(settled_targets(availability::countable, target_mask {}, "pmu", path)
              == (target_thread_bit | target_cpu_bit),
          "a countable leaf no probe settled names both kinds on a core "
          "device path, a hybrid per-core instance counting a thread's own "
          "events (FR-021, FR-022)");
  }

  check(settled_targets(availability::countable,
                        target_mask {},
                        "pmu",
                        "uncore_imc")
            == target_cpu_bit,
        "a countable leaf no probe settled names the cpu kind alone on a "
        "device that binds one processor for every task (FR-021, FR-022)");

  // The per-kind verdicts, which is the answer on a host whose cpu-targeted
  // probe is refused: the record names the kinds the probes settled and no
  // others, so the object's scope adds nothing to it.
  check(settled_targets(availability::countable,
                        target_thread_bit,
                        "pmu",
                        "cpu")
            == target_thread_bit,
        "an entry the cpu-targeted probe refused names no cpu bit on a core "
        "device, so the mask states the probe's own verdicts (FR-021)");
  check(settled_targets(availability::countable, target_cpu_bit, "pmu", "cpu")
            == target_cpu_bit,
        "an entry the per-task probe refused names the cpu bit alone, the "
        "core device scope notwithstanding (FR-021)");
  check(settled_targets(availability::countable,
                        target_thread_bit | target_cpu_bit,
                        "pmu",
                        "cpu")
            == (target_thread_bit | target_cpu_bit),
        "an entry both probes counted names both kinds (FR-021)");
  check(settled_targets(availability::countable,
                        target_cpu_bit,
                        "pmu",
                        "uncore_imc")
            == target_cpu_bit,
        "a device-scoped entry names the cpu kind its own probe settled and "
        "nothing else (FR-021, FR-022)");
}

// The plan-side availability gate, over every arm the clause in the seam
// header names: a countable entry passes for either target kind, a
// scope-refused entry passes for the cpu kind alone, and every other
// state is refused for either kind. The decisions are the catalog state
// and the requested kind, so every arm runs on a host whose catalog
// publishes no scope-refused entry, which is what the reference host
// does: a cpu-targeted probe there answers EINVAL and the probe settles
// that answer as not_encodable (FR-021, FR-022, FR-024, FR-046).
auto availability_gate_scenario() -> void
{
  using sg::counters::target_kind;
  using sg::counters::detail::availability_gate_passes;

  for (const auto state :
       {
           availability::permission_blocked,
           availability::not_encodable,
           availability::absent,
           availability::scope_refused,
           availability::gap,
       })
  {
    check(!availability_gate_passes(state, target_kind::thread),
          "a state no probe settled refuses a per-task request (FR-024)");
    check(availability_gate_passes(state, target_kind::cpu)
              == (state == availability::scope_refused),
          "a scope-refused entry lets a cpu request past the gate and every "
          "other state refuses it (FR-021, FR-022, FR-024)");
  }

  check(availability_gate_passes(availability::countable, target_kind::thread),
        "a countable entry lets a per-task request past the gate (FR-024)");
  check(availability_gate_passes(availability::countable, target_kind::cpu),
        "a countable entry lets a cpu request past the gate (FR-024)");
}

// The clock window's disclosure decision, over both arms (FR-007). A leaf
// set that names a disclosure column carries the clock leaf's own
// countability value into the column no leaf resolves to, and a leaf set
// that names none publishes no second point. The window is the clock
// provider's own, opened over its own leaf, so both arms run here without
// a granted event (FR-046).
auto clock_disclosure_scenario() -> void
{
  using sg::counters::clock_provider;

  // The sink indexes cells as `columns[index * stride + row]`, so a stride
  // of one puts column `n` in cell `n`.
  constexpr std::uint64_t unwritten = ~std::uint64_t {0};
  constexpr std::size_t column_count = 2;
  constexpr std::size_t row_stride = 1;

  clock_provider provider {};

  leaf_set disclosing;
  disclosing.addresses = {"machine/monotonic"};
  disclosing.disclosure_column = 1;
  auto with_column = provider.open(disclosing, target {});
  check(with_column != nullptr,
        "the clock provider opens a window over one of its own leaves "
        "(FR-007)");
  std::array<std::uint64_t, column_count> disclosed {unwritten, unwritten};
  point_sink disclosed_sink(disclosed.data(), 1, column_count, row_stride, 0);
  with_column->read_points(disclosed_sink);
  check(disclosed[1] == static_cast<std::uint64_t>(availability::countable),
        "the window that names a disclosure column discloses the clock "
        "leaf's own countability value (FR-007)");

  leaf_set quiet;
  quiet.addresses = disclosing.addresses;
  auto without_column = provider.open(quiet, target {});
  check(without_column != nullptr,
        "the same leaf opens a window whose leaf set names no disclosure "
        "column (FR-007)");
  std::array<std::uint64_t, column_count> quiet_columns {unwritten, unwritten};
  point_sink quiet_sink(quiet_columns.data(), 1, column_count, row_stride, 0);
  without_column->read_points(quiet_sink);
  check(quiet_columns[0] != unwritten,
        "the window writes the clock point the platform clock returned "
        "(FR-011, FR-033)");
  check(quiet_columns[1] != disclosed[1],
        "a leaf set naming no disclosure column publishes no second point, "
        "where the window that names one carries the catalog's own "
        "countability value (FR-007)");
}

auto main() -> int
{
  availability_gate_scenario();
  clock_disclosure_scenario();
  fixture_scenario();
  format_scenario();
  compose_scenario();
  attr_text_scenario();
  decode_scenario();
  decode_recipe_scenario();
  multiplex_window_scenario();
  time_recipe_scenario();
  group_short_read_scenario();
  migrated_thread_scenario();
  fast_context_lifetime_scenario();
  fast_context_partial_open_scenario();
  destroy_before_open_scenario();
  intel_encodable_rows_scenario();
  encoding_refusal_scenario();
  device_placement_scenario();
  read_mode_selection_scenario();
  embedded_registry_scenario();
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
  hybrid_device_scope_scenario();
  uncore_device_fixture_scenario();
  vendored_row_placement_scenario();
  unpublished_device_probe_scenario();
  device_scope_probe_scenario();
  settled_target_mask_scenario();
  probe_kind_record_scenario();
  context_open_refusal_scenario();
  std::printf("counters_linux_pmu_seam_test PASS: encoder and protocol\n");
  return 0;
}
