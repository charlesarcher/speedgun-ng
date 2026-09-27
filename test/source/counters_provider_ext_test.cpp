// ============================================================================
// TDD test for out-of-tree providers (T039; US5 scenarios 1-4; FR-011,
// FR-012, FR-047).
//
// A mini provider defined in this translation unit against public
// headers only: it registers `menagerie/giraffe-2` with a `honks`
// counter and a described-but-unavailable `sleeps` entry, appears in
// enumeration with descriptions and availability states, measures
// through a scope, folds `honks / monotonic` to a honk rate from one
// shared sampling action, and branches the unavailable entry on
// catalog state alone. Hand-computed expectations, frameworkless
// check()/fail() convention.
// ============================================================================

#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string_view>
#include <vector>

#include "speedgun-ng/counters.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "COUNTERS PROVIDER EXT TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

auto same_double(const double lhs, const double rhs) -> bool
{
  return std::bit_cast<std::uint64_t>(lhs) == std::bit_cast<std::uint64_t>(rhs);
}

using sg::counters::availability;
using sg::counters::catalog_entry;
using sg::counters::catalog_seed;
using sg::counters::clock_provider;
using sg::counters::compile;
using sg::counters::dim;
using sg::counters::expression;
using sg::counters::leaf_set;
using sg::counters::object;
using sg::counters::object_seed;
using sg::counters::object_sink;
using sg::counters::point_sink;
using sg::counters::provider_iface;
using sg::counters::scope;
using sg::counters::system;
using sg::counters::target;
using sg::counters::window_reader;

using events = dim<0, 1>;
using time_dim = dim<1, 0>;

constexpr std::uint64_t kHonksPerAction = 7;

// The out-of-tree reader: honks accumulate between sampling actions.
class honk_window final : public window_reader
{
public:
  void read_points(point_sink& sink) noexcept override
  {
    m_total += kHonksPerAction;
    sink.put(m_total);
  }

private:
  std::uint64_t m_total = 0;
};

// The out-of-tree provider: one object, one countable counter, one
// described-but-unavailable entry.
class giraffe_provider final : public provider_iface
{
public:
  void enumerate(object_sink& sink) const override
  {
    sink.add_object(object_seed {
        .kind = "animal",
        .path = "menagerie/giraffe-2",
        .alias = {},
        .description = "the famous giraffe",
        .entries =
            {
                catalog_seed {
                    .name = "honks",
                    .description = "honks emitted",
                    .unit = "ops",
                },
                catalog_seed {
                    .name = "sleeps",
                    .description = "sleeps counted (giraffes barely sleep)",
                    .unit = "ops",
                    .avail = availability::absent,
                },
            },
    });
  }

  std::unique_ptr<window_reader> open(const leaf_set& leaves,
                                      const target& /*where*/) override
  {
    for (const auto& address : leaves.addresses) {
      if (address != "menagerie/giraffe-2/honks") {
        return nullptr;
      }
    }
    return std::make_unique<honk_window>();
  }
};

auto giraffe() -> object
{
  const auto found = system::local().object("menagerie/giraffe-2");
  if (!found.has_value()) {
    fail("giraffe object resolves after registration");
  }
  return *found;
}

// Scenario 1: the provider appears in enumeration with its kind,
// descriptions, and availability states (FR-012).
auto enumeration_scenario() -> void
{
  const auto& animal = giraffe();
  check(animal.kind() == "animal", "provider kind is reported");
  check(!animal.description().empty(), "provider description is reported");

  const auto entries = animal.counters();
  const catalog_entry* honks = nullptr;
  const catalog_entry* sleeps = nullptr;
  for (const auto& entry : entries) {
    if (entry.name == "honks") {
      honks = &entry;
    }
    if (entry.name == "sleeps") {
      sleeps = &entry;
    }
  }
  check(honks != nullptr && honks->avail == availability::countable
            && !honks->description.empty(),
        "honks is countable with a description (scenario 1)");
  check(sleeps != nullptr && sleeps->avail == availability::absent
            && !sleeps->description.empty(),
        "sleeps is described but not countable (scenario 1)");
}

// Scenario 2: the counter measures through a scope: start and finish
// are two actions, so the window holds exactly one action's worth of
// honks.
auto scope_scenario() -> void
{
  const expression<events> honks {*giraffe().counter<events>("honks")};
  const expression<time_dim> mono {
      *system::local().object("machine")->counter<time_dim>("monotonic")};
  const auto compiled = compile(system::local(), honks, mono);
  check(compiled.has_value(), "cross-provider plan compiles");
  scope window {*compiled};
  window.start();
  volatile double spin = 0.0;
  for (int i = 0; i < 2'000'000; ++i) {
    spin += 1.0;
  }
  static_cast<void>(spin);
  window.finish();
  const auto honked = window.metric(honks);
  check(same_double(honked.value, static_cast<double>(kHonksPerAction)),
        "one window, one action gap: seven honks (scenario 2)");
}

// Scenario 3: honks / monotonic folds to a honk rate; the clock leaf
// and the honk leaf are read within the same sampling actions
// (FR-047).
auto rate_scenario() -> void
{
  const expression<events> honks {*giraffe().counter<events>("honks")};
  const expression<time_dim> mono {
      *system::local().object("machine")->counter<time_dim>("monotonic")};
  const auto rate = honks / mono;
  const auto compiled = compile(system::local(), rate, mono);
  check(compiled.has_value(), "rate plan compiles");
  scope window {*compiled};
  window.start();
  window.finish();
  const auto honks_per_ns = window.metric(rate);
  const auto mono_ns = window.metric(mono);
  check(honks_per_ns.value > 0.0, "honk rate is positive (scenario 3)");
  check(
      std::fabs(honks_per_ns.value * mono_ns.value - kHonksPerAction) <= 7.0e-6,
      "rate x window reconstitutes the honks (scenario 3)");
  check(same_double(honks_per_ns.running_ratio, 1.0),
        "cross-provider composite carries standard disclosure");
}

// Scenario 4: an unavailable entry is branched on catalog state
// alone: no resolution is attempted for it.
auto availability_branch_scenario() -> void
{
  std::size_t countable = 0;
  std::size_t skipped = 0;
  for (const auto& entry : giraffe().counters()) {
    if (entry.avail == availability::countable) {
      check(giraffe().counter<events>(entry.name).has_value(),
            "countable entry resolves");
      ++countable;
    } else {
      ++skipped;
    }
  }
  check(countable == 1 && skipped == 1,
        "catalog state alone drives the branch (scenario 4)");
}

}  // namespace

auto main() -> int
{
  auto giraffe = std::make_unique<giraffe_provider>();
  if (!system::local().register_provider(std::move(giraffe)).has_value()) {
    fail("out-of-tree provider registers");
  }
  if (!system::local()
           .register_provider(std::make_unique<clock_provider>())
           .has_value())
  {
    fail("clock provider registers");
  }

  enumeration_scenario();
  scope_scenario();
  rate_scenario();
  availability_branch_scenario();

  std::printf(
      "counters_provider_ext_test PASS: giraffe counts from out of tree\n");
  return 0;
}
