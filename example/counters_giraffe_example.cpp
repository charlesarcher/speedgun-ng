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

using sg::counters::Availability;
using sg::counters::CatalogSeed;
using sg::counters::ClockProvider;
using sg::counters::compile;
using sg::counters::Dim;
using sg::counters::Expression;
using sg::counters::LeafSet;
using sg::counters::ObjectSeed;
using sg::counters::ObjectSink;
using sg::counters::PointSink;
using sg::counters::ProviderIface;
using sg::counters::Scope;
using sg::counters::System;
using sg::counters::Target;
using sg::counters::WindowReader;

using Events = Dim<0, 1>;
using TimeDim = Dim<1, 0>;

// Each sampling action adds this many honks to the giraffe's running
// total; the provider reports cumulative points like every other leaf
// kind (C-PRO-2).
constexpr std::uint64_t kHonksPerAction = 3;

// The giraffe's counting window: yields the cumulative honks within
// each sampling action, exactly once per action (FR-011).
class HonkWindow final : public WindowReader
{
public:
  void readPoints(PointSink& sink) noexcept override
  {
    m_total += kHonksPerAction;
    sink.put(m_total);
  }

private:
  std::uint64_t m_total = 0;
};

// The giraffe provider: one object it owns, described and countable
// (C-PRO-1). The system never saw this provider before registration.
class GiraffeProvider final : public ProviderIface
{
public:
  void enumerate(ObjectSink& sink) const override
  {
    sink.addObject(ObjectSeed {
        .kind = "animal",
        .path = "menagerie/giraffe-2",
        .alias = {},
        .description = "the famous giraffe",
        .entries = {CatalogSeed {
            .name = "honks",
            .description = "honks emitted",
            .unit = "ops",
            .avail = Availability::COUNTABLE,
        }},
    });
  }

  // The system asks this provider only about leaves it enumerated;
  // anything else is not ours to sample.
  std::unique_ptr<WindowReader> open(const LeafSet& leaves,
                                     const Target& /*where*/) override
  {
    for (const auto& address : leaves.addresses) {
      if (address != "menagerie/giraffe-2/honks") {
        return nullptr;
      }
    }
    return std::make_unique<HonkWindow>();
  }
};

}  // namespace

auto main() -> int
{
  // Attach the provider, then the clock for the time base.
  auto giraffe = std::make_unique<GiraffeProvider>();
  if (!System::local().registerProvider(std::move(giraffe)).has_value()) {
    std::fprintf(stderr, "giraffe: provider registration failed\n");
    return 1;
  }
  if (!System::local()
           .registerProvider(std::make_unique<ClockProvider>())
           .has_value())
  {
    std::fprintf(stderr, "giraffe: clock registration failed\n");
    return 1;
  }

  // Compose the honk rate: honks per nanosecond of monotonic time,
  // both resolved from the catalog through their public handles.
  const auto menagerie = System::local().object("menagerie/giraffe-2");
  const auto animal = System::local().object("machine");
  if (!menagerie.has_value() || !animal.has_value()) {
    std::fprintf(stderr, "giraffe: object resolution failed\n");
    return 1;
  }
  const Expression<Events> honks {menagerie->counter<Events>("honks").value()};
  const Expression<TimeDim> mono {
      animal->counter<TimeDim>("monotonic").value()};
  const auto rate = honks / mono;
  const auto compiled = compile(System::local(), rate);
  if (!compiled.has_value()) {
    std::fprintf(stderr, "giraffe: plan compile failed\n");
    return 1;
  }

  // One shared sampling action per window endpoint measures the whole
  // composite (FR-047); the fold reports value plus disclosure.
  Scope window {*compiled};
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
      result.runningRatio,
      result.scaled ? "yes" : "no");
  return 0;
}
