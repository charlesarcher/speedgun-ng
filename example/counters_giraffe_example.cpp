// ============================================================================
// The giraffe example (T040; US5, SC-003): an out-of-tree provider the
// library has never seen, attached through public headers only.
//
// `menagerie/giraffe-2` counts honks; a scope measures
// `honks / monotonic` to a honk rate, both leaves read from one shared
// sampling action (FR-047). Public headers plus the standard library:
// no `source/` include, no internal access (FR-011, FR-012).
// ============================================================================

#include <cstdint>
#include <cstdio>
#include <memory>

#include "speedgun-ng/counters.hpp"

namespace
{

using sg::counters::availability;
using sg::counters::catalog_seed;
using sg::counters::clock_provider;
using sg::counters::compile;
using sg::counters::dim;
using sg::counters::expression;
using sg::counters::leaf_set;
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

// Each sampling action adds this many honks to the giraffe's running
// total; the provider reports cumulative points like every other leaf
// kind (C-PRO-2).
constexpr std::uint64_t kHonksPerAction = 3;

// The giraffe's counting window: yields the cumulative honks within
// each sampling action, exactly once per action (FR-011).
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

// The giraffe provider: one object it owns, described and countable
// (C-PRO-1). The system never saw this provider before registration.
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
        .entries = {catalog_seed {
            .name = "honks",
            .description = "honks emitted",
            .unit = "ops",
            .avail = availability::countable,
        }},
    });
  }

  // The system asks this provider only about leaves it enumerated;
  // anything else is not ours to sample.
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

}  // namespace

auto main() -> int
{
  // Attach the provider, then the clock for the time base.
  auto giraffe = std::make_unique<giraffe_provider>();
  if (!system::local().register_provider(std::move(giraffe)).has_value()) {
    std::fprintf(stderr, "giraffe: provider registration failed\n");
    return 1;
  }
  if (!system::local()
           .register_provider(std::make_unique<clock_provider>())
           .has_value())
  {
    std::fprintf(stderr, "giraffe: clock registration failed\n");
    return 1;
  }

  // Compose the honk rate: honks per nanosecond of monotonic time,
  // both resolved from the catalog through their public handles.
  const auto menagerie = system::local().object("menagerie/giraffe-2");
  const auto animal = system::local().object("machine");
  if (!menagerie.has_value() || !animal.has_value()) {
    std::fprintf(stderr, "giraffe: object resolution failed\n");
    return 1;
  }
  const expression<events> honks {menagerie->counter<events>("honks").value()};
  const expression<time_dim> mono {
      animal->counter<time_dim>("monotonic").value()};
  const auto rate = honks / mono;
  const auto compiled = compile(system::local(), rate);
  if (!compiled.has_value()) {
    std::fprintf(stderr, "giraffe: plan compile failed\n");
    return 1;
  }

  // One shared sampling action per window endpoint measures the whole
  // composite (FR-047); the fold reports value plus disclosure.
  scope window {*compiled};
  window.start();
  volatile double spin = 0.0;
  for (int i = 0; i < 10'000'000; ++i) {
    spin += 1.0;
  }
  static_cast<void>(spin);
  window.finish();
  const auto result = window.metric(rate);

  std::printf(
      "giraffe-2 honk rate: %.3e honks/ns (running ratio %.6f," " scaled %s)\n",
      result.value,
      result.running_ratio,
      result.scaled ? "yes" : "no");
  return 0;
}
