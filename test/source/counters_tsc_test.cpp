// ============================================================================
// TDD test for the raw time-stamp counter accessor
// (specs/008-timestamp-counter FR-001..FR-008; US1, US2).
//
// Covers the corrected publication rule, where the entry ships wherever
// the build executes the instruction and carries a count with no rate
// (FR-001, FR-002), and the accessor that resolves it as a shorter
// spelling of the uniform lookup, returning the identical counter type so
// the two are interchangeable (FR-004). Hand-rolled check()/fail()
// convention; no test framework may be added to this repository.
//
// The first scenario runs before any provider is registered, because
// `system::local()` is a process singleton and the unregistered branch is
// reachable only in a fresh process (FR-007). Reordering the scenarios
// silently loses that branch.
// ============================================================================

#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "speedgun-ng/counters.hpp"

// The entry publishes exactly where the build executes the instruction
// (FR-001), so the presence assertions are guarded on the same condition
// the provider uses.
#if (defined(__x86_64__) || defined(__i386__)) && !defined(_MSC_VER)
#  define SG_TEST_HAS_TSC 1
#else
#  define SG_TEST_HAS_TSC 0
#endif

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "COUNTERS TSC TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

using sg::counters::availability;
using sg::counters::catalog_entry;
using sg::counters::clock_provider;
using sg::counters::compile;
using sg::counters::dim;
using sg::counters::fake_provider;
using sg::counters::read_mode;
using sg::counters::scope;
using sg::counters::system;
using sg::counters::unit;
using sg::counters::unit_name;

// The raw entry's unit is the closed token 007 already assigns it, which
// maps to dim<0,1> (counters_core.hpp:178), so it is a counted source and
// a quotient against another counted source folds to dim<0,0>.
using events = dim<0, 1>;

// Exact double comparison through the bit pattern: these are exactness
// checks, and a tolerance band has no place in them (the build enables
// -Werror=float-equal, so `==` is not available here).
auto same_double(const double lhs, const double rhs) -> bool
{
  return std::bit_cast<std::uint64_t>(lhs) == std::bit_cast<std::uint64_t>(rhs);
}

auto find_entry(const std::vector<catalog_entry>& entries,
                const std::string_view name) -> const catalog_entry*
{
  for (const auto& entry : entries) {
    if (entry.name == name) {
      return &entry;
    }
  }
  return nullptr;
}

auto machine_entries() -> std::vector<catalog_entry>
{
  const auto machine = *system::local().object("machine");
  return machine.counters();
}

// CPU-bound work, so a sampling window over it retires a positive count.
auto burn_cpu() -> void
{
  volatile double work = 0.0;
  for (int i = 0; i < 200000; ++i) {
    work += static_cast<double>(i);
  }
  static_cast<void>(work);
}

// FR-007: with no clock provider registered the tree holds no entry, and the
// accessor names the absent provider and guesses at nothing. This runs first
// because the singleton cannot be rewound.
auto test_absent_provider() -> void
{
  const auto absent = system::local().tsc();
  check(!absent.has_value(),
        "the accessor refuses before any provider is registered (FR-007)");
  check(absent.error().message.find("no clock provider is registered")
            != std::string::npos,
        "the error names the absent provider (FR-007)");
  check(absent.error().suggestions.empty(),
        "the absent-provider error carries no catalog suggestion (FR-008)");
}

// FR-006: the accessor resolves an entry and opens no boundary, so the
// registration that follows it in this same process still succeeds.
auto register_fixture() -> void
{
  auto clock = std::make_unique<clock_provider>();
  check(system::local().register_provider(std::move(clock)).has_value(),
        "the clock provider registers after an accessor call (FR-006)");

  // A counted source to compose the time-stamp counter against, which is
  // what proves interchangeability, and rules out a private spelling.
  auto fake = std::make_unique<fake_provider>();
  fake->add_object("core-0", "core0", "core", "the core under test");
  fake->add_counter("core-0", "instructions", "ops", "instructions retired");
  fake->set_points("core-0", "instructions", {}, 1000);
  check(system::local().register_provider(std::move(fake)).has_value(),
        "the counted source registers (FR-004)");
}

// FR-001, FR-002: the entry publishes on this host, which executes the
// instruction and publishes no counter frequency, and it carries a count
// with no rate attached.
auto test_raw_entry() -> void
{
  const auto entries = machine_entries();
  const auto* raw = find_entry(entries, "tsc");
#if SG_TEST_HAS_TSC
  check(raw != nullptr,
        "the entry publishes wherever the build executes the instruction, "
        "whatever frequency the platform publishes (FR-001)");
#else
  check(raw == nullptr,
        "a build without the instruction publishes no entry (FR-001)");
#endif
  if (raw == nullptr) {
    return;
  }
  check(raw->mode == read_mode::fast_tsc,
        "the entry reports the fast single-instruction read mode (FR-001)");
  check(raw->avail == availability::countable,
        "the raw entry is countable (FR-002)");
  check(raw->unit == unit::none,
        "the raw entry carries a count, not a duration (FR-002)");
  check(unit_name(raw->unit) == "none",
        "the unit is the closed token 007 already assigned (FR-002)");
  check(raw->frequency_hz == 0,
        "the raw entry attaches no frequency (FR-002)");
  check(!raw->scaled, "the raw entry attaches no scaled flag (FR-002)");
  check(raw->description.find("raw") != std::string::npos,
        "the description states that the count is raw (FR-002)");
  std::printf("tsc entry: mode fast_tsc, unit none, frequency 0, "
              "description '%s'\n",
              std::string(raw->description).c_str());
}

// FR-004: the accessor and the uniform lookup name the same canonical
// entry, which is what makes the shorter spelling interchangeable.
auto test_accessor_matches_lookup() -> void
{
  const auto direct = system::local().tsc();
  check(direct.has_value(), "the accessor resolves once a provider is "
                            "registered (FR-004)");
  const auto machine = *system::local().object("machine");
  const auto named =
      machine.counter<events>("tsc");  // the uniform spelling
  check(named.has_value(),
        "the uniform lookup resolves the same entry (FR-004)");
  check(direct->name() == named->name(),
        "both spellings name the same counter (FR-004)");
  check(direct->description() == named->description(),
        "both spellings carry the same description (FR-004)");
  check(direct->name() == "tsc", "the counter names itself (FR-004)");
  std::printf("accessor and lookup both name '%s'\n",
              std::string(direct->name()).c_str());
}

// FR-005: the accessor resolves an entry and reads nothing, so a thousand
// calls leave the catalog byte-identical.
auto test_accessor_reads_nothing() -> void
{
  const auto before = machine_entries();
  for (int i = 0; i < 1000; ++i) {
    const auto read = system::local().tsc();
    check(read.has_value(), "the accessor keeps resolving (FR-005)");
  }
  const auto after = machine_entries();
  check(after.size() == before.size(),
        "a thousand accessor calls add no catalog entry (FR-005)");
  bool identical = after.size() == before.size();
  for (std::size_t i = 0; identical && i < after.size(); ++i) {
    identical = after[i].name == before[i].name
                && after[i].description == before[i].description
                && after[i].mode == before[i].mode;
  }
  check(identical,
        "a thousand accessor calls leave every catalog entry unchanged "
        "(FR-005)");
}

// FR-004 at the point that matters: the counter the accessor returns
// composes with a counted source through the existing algebra and folds
// with full disclosure. A bespoke type could not do this.
auto test_counter_composes() -> void
{
  const auto core = *system::local().object("core-0");
  const auto instructions = *core.counter<events>("instructions");
  const auto raw = *system::local().tsc();
  const auto per_tick = instructions / raw;
  const auto compiled = compile(system::local(), per_tick);
  check(compiled.has_value(),
        "a counted source divided by the time-stamp counter compiles "
        "(FR-004)");

  scope window {*compiled};
  window.start();
  burn_cpu();
  window.finish();
  const auto folded = window.metric(per_tick);
  check(folded.value > 0.0,
        "the quotient folds to a positive instructions-per-tick ratio "
        "(FR-004)");
  check(same_double(folded.running_ratio, 1.0),
        "the fold discloses ratio 1.0 for an unscaled counted source "
        "(FR-002)");
  std::printf("instructions per tick: %.6f, running ratio %.6f, scaled %s\n",
              folded.value,
              folded.running_ratio,
              folded.scaled ? "yes" : "no");
}

}  // namespace

auto main() -> int
{
  test_absent_provider();
  register_fixture();
  test_raw_entry();
  test_accessor_matches_lookup();
  test_accessor_reads_nothing();
  test_counter_composes();
  std::printf("counters_tsc_test PASS: raw entry published, accessor "
              "interchangeable\n");
  return 0;
}
