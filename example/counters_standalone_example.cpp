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

#include "speedgun-ng/simulation.hpp"

namespace
{

using sg::counters::ClockProvider;
using sg::counters::compile;
using sg::counters::Dim;
using sg::counters::FakeProvider;
using sg::counters::MetricResult;
using sg::counters::System;

using Events = Dim<0, 1>;
using TimeDim = Dim<1, 0>;

constexpr std::size_t kIterations = 60;

}  // namespace

auto main() -> int
{
  auto clock = std::make_unique<ClockProvider>();
  if (!System::local().registerProvider(std::move(clock)).has_value()) {
    std::fprintf(stderr, "standalone: clock provider registration failed\n");
    return 1;
  }

  // Fake sources with per-action deltas: 1000 instructions and 400
  // cycles per sampling action, so every interval folds to IPC 2.5.
  auto fake = std::make_unique<FakeProvider>();
  fake->addObject("package-0/core-0", "core-0", "core", "first core");
  fake->addCounter(
      "package-0/core-0", "instructions", "ops", "instructions retired");
  fake->addCounter("package-0/core-0", "cycles", "ops", "core cycles");
  fake->setPoints("package-0/core-0", "instructions", {}, 1000);
  fake->setPoints("package-0/core-0", "cycles", {}, 400);
  if (!System::local().registerProvider(std::move(fake)).has_value()) {
    std::fprintf(stderr, "standalone: provider registration failed\n");
    return 1;
  }

  const auto core = System::local().object("package-0/core-0");
  if (!core.has_value()) {
    std::fprintf(stderr, "standalone: object resolution failed\n");
    return 1;
  }
  const auto instructions = core->counter<Events>("instructions").value();
  const auto cycles = core->counter<Events>("cycles").value();
  const auto ipc = instructions / cycles;

  // The clock leaf the rate normalizes by. One sampling action reads it
  // beside the two fake leaves (FR-047), so the recorder capacity is
  // unchanged by the second metric.
  const auto machine = System::local().object("machine");
  if (!machine.has_value()) {
    std::fprintf(stderr, "standalone: machine resolution failed\n");
    return 1;
  }
  const auto monotonic = machine->counter<TimeDim>("monotonic").value();
  const auto rate = instructions / monotonic;

  const auto compiled = compile(System::local(), ipc, rate);
  if (!compiled.has_value()) {
    std::fprintf(stderr, "standalone: plan compile failed\n");
    return 1;
  }

  // The marker opens an instruction trace at the region boundary. It sits
  // outside the timed window because it occupies two instructions and
  // would perturb the measurement it sits inside (specs/011 FR-019). Run
  // this binary under `sde64 -start_ssc_mark FACE:repeat -- <binary>` and
  // collection begins at the loop below.
  sg::simulationStart();

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
  const std::vector<MetricResult> intervals = ipc.foldPairs(recorder.view());
  const std::vector<MetricResult> rates = rate.foldPairs(recorder.view());
  for (std::size_t iteration = 0; iteration < intervals.size(); ++iteration) {
    const auto& folded = intervals[iteration];
    const auto& foldedRate = rates[iteration];
    std::printf("iteration %2zu: ipc %.6f (running ratio %.6f, scaled %s)\n",
                iteration + 1,
                folded.value,
                folded.runningRatio,
                folded.scaled ? "yes" : "no");
    std::printf("iteration %2zu: instructions per ns %.6f "
                "(running ratio %.6f, scaled %s)\n",
                iteration + 1,
                foldedRate.value,
                foldedRate.runningRatio,
                foldedRate.scaled ? "yes" : "no");
  }
  return 0;
}
