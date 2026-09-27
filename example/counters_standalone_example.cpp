// ============================================================================
// The standalone example (T041, T121; FR-050, SC-001): public headers plus
// the standard library, nothing else. An `instructions / cycles`-shape
// metric and a clock-normalized instruction rate are driven in a
// fixed-iteration loop over a fake source and a clock source; the
// recorder capacity is computed from the known iteration count, and the
// folded per-interval results feed the per-iteration lines below. The
// link manifest names the platform C and C++ runtime alone, with no
// `speedgun-ng` entry and no third-party entry (quickstart 2).
// ============================================================================

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>

#include "speedgun-ng/counters.hpp"

namespace
{

using sg::counters::clock_provider;
using sg::counters::compile;
using sg::counters::dim;
using sg::counters::fake_provider;
using sg::counters::metric_result;
using sg::counters::system;

using events = dim<0, 1>;
using time_dim = dim<1, 0>;

constexpr std::size_t kIterations = 60;

}  // namespace

auto main() -> int
{
  auto clock = std::make_unique<clock_provider>();
  if (!system::local().register_provider(std::move(clock)).has_value()) {
    std::fprintf(stderr, "standalone: clock provider registration failed\n");
    return 1;
  }

  // Fake sources with per-action deltas: 1000 instructions and 400
  // cycles per sampling action, so every interval folds to IPC 2.5.
  auto fake = std::make_unique<fake_provider>();
  fake->add_object("package-0/core-0", "core-0", "core", "first core");
  fake->add_counter(
      "package-0/core-0", "instructions", "ops", "instructions retired");
  fake->add_counter("package-0/core-0", "cycles", "ops", "core cycles");
  fake->set_points("package-0/core-0", "instructions", {}, 1000);
  fake->set_points("package-0/core-0", "cycles", {}, 400);
  if (!system::local().register_provider(std::move(fake)).has_value()) {
    std::fprintf(stderr, "standalone: provider registration failed\n");
    return 1;
  }

  const auto core = system::local().object("package-0/core-0");
  if (!core.has_value()) {
    std::fprintf(stderr, "standalone: object resolution failed\n");
    return 1;
  }
  const auto instructions = core->counter<events>("instructions").value();
  const auto cycles = core->counter<events>("cycles").value();
  const auto ipc = instructions / cycles;

  // The clock leaf the rate normalizes by. One sampling action reads it
  // beside the two fake leaves (FR-047), so the recorder capacity is
  // unchanged by the second metric.
  const auto machine = system::local().object("machine");
  if (!machine.has_value()) {
    std::fprintf(stderr, "standalone: machine resolution failed\n");
    return 1;
  }
  const auto monotonic = machine->counter<time_dim>("monotonic").value();
  const auto rate = instructions / monotonic;

  const auto compiled = compile(system::local(), ipc, rate);
  if (!compiled.has_value()) {
    std::fprintf(stderr, "standalone: plan compile failed\n");
    return 1;
  }

  // The window holds one point per iteration plus the initial point;
  // the capacity is computed from that known count (FR-050).
  auto recorder = compiled->recorder(kIterations + 1);
  recorder.sample();
  for (std::size_t iteration = 0; iteration < kIterations; ++iteration) {
    volatile double work = 0.0;
    for (int i = 0; i < 1000; ++i) {
      work += 1.0;
    }
    static_cast<void>(work);
    recorder.sample();
  }

  // Fold results feed the per-iteration lines: each interval folds
  // independently from the shared columns, value plus disclosure.
  const std::vector<metric_result> intervals = ipc.fold_pairs(recorder.view());
  const std::vector<metric_result> rates = rate.fold_pairs(recorder.view());
  for (std::size_t iteration = 0; iteration < intervals.size(); ++iteration) {
    const auto& folded = intervals[iteration];
    const auto& folded_rate = rates[iteration];
    std::printf("iteration %2zu: ipc %.6f (running ratio %.6f, scaled %s)\n",
                iteration + 1,
                folded.value,
                folded.running_ratio,
                folded.scaled ? "yes" : "no");
    std::printf("iteration %2zu: instructions per ns %.6f "
                "(running ratio %.6f, scaled %s)\n",
                iteration + 1,
                folded_rate.value,
                folded_rate.running_ratio,
                folded_rate.scaled ? "yes" : "no");
  }
  return 0;
}
