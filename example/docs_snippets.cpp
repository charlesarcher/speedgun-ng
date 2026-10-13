// The capability snippets the root page quotes, one region per capability
// the other example files do not carry. The file builds as a target so a
// pasted block cannot rot: each region holds a complete declaration plus
// its own registration, and no region is a fragment.

#include <cstdint>
#include <vector>

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

// sg-example:begin run-loop
auto bmLoop(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
  sg::clobberMemory();
}

[[maybe_unused]] const auto kLoopRegistrar =
    sg::registerBenchmark(&bmLoop, "DISABLED_bmLoop");

// sg-example:end run-loop

// sg-example:begin registration
auto bmNamed(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

SG_BENCHMARK(bmNamed);

[[maybe_unused]] const auto kNamedRegistrar =
    sg::registerBenchmark(&bmNamed, "DISABLED_Suite/bmNamed").arg(8);

// sg-example:end registration

// sg-example:begin capture
auto bmCapture(sg::State& state, int cells) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(cells);
  }
}

SG_BENCHMARK_CAPTURE(bmCapture, wide, 8);

// sg-example:end capture

// sg-example:begin template-bench
template<class Value>
auto bmFill(sg::State& state) -> void
{
  std::vector<Value> values(64, Value {1});
  for (auto _ : state) {
    sg::doNotOptimize(values);
  }
}

SG_BENCHMARK_TEMPLATE(bmFill, std::uint64_t);

// sg-example:end template-bench

// sg-example:begin fixture
class TableFixture : public sg::Fixture
{
public:
  auto setUp(sg::State&) -> void override { m_values.assign(64, 1); }

  auto tearDown(sg::State&) -> void override { m_values.clear(); }

protected:
  std::vector<std::uint64_t> m_values;
};
// sg-example:end fixture

SG_BENCHMARK_DEFINE_F(TableFixture, bmTableTouch)

(sg::State& state)
{
  for (auto _ : state) {
    sg::doNotOptimize(m_values.back());
  }
}

// sg-example:begin fixture-callbacks
SG_BENCHMARK_REGISTER_F(TableFixture, bmTableTouch)
    .setup([](sg::State& state) { sg::doNotOptimize(state.rangeCount()); })
    .teardown([](sg::State&) {});

// sg-example:end fixture-callbacks

// sg-example:begin template-fixture
template<class Value>
class ColumnFixture : public sg::Fixture
{
protected:
  std::vector<Value> m_values;
};

SG_BENCHMARK_TEMPLATE_METHOD_F(ColumnFixture, bmColumnFill)

(sg::State& state)
{
  for (auto _ : state) {
    this->m_values.assign(32, 1);
    sg::doNotOptimize(this->m_values);
  }
}

SG_BENCHMARK_TEMPLATE_INSTANTIATE_F(ColumnFixture, bmColumnFill, std::uint64_t);

// sg-example:end template-fixture

// sg-example:begin ranges
auto bmRanges(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.range(0) + state.range(1));
  }
}

[[maybe_unused]] const auto kRangesRegistrar =
    sg::registerBenchmark(&bmRanges, "DISABLED_bmRanges")
        .argNames({"width", "depth"})
        .rangeMultiplier(2)
        .ranges({{8, 8}, {16, 32}});

// sg-example:end ranges

// sg-example:begin ranges-family
auto bmOneRange(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.range(0));
  }
}

auto bmTwoRanges(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.range(0) * state.range(1));
  }
}

[[maybe_unused]] const auto kArgRegistrar =
    sg::registerBenchmark(&bmOneRange, "DISABLED_bmOne")
        .arg(4)
        .argName("cells")
        .range(8, 64)
        .denseRange(64, 128, 16);

[[maybe_unused]] const auto kProductRegistrar =
    sg::registerBenchmark(&bmTwoRanges, "DISABLED_bmProduct")
        .argNames({"rows", "cols"})
        .argsProduct({{2, 4}, {8, 16, 32}});

// sg-example:end ranges-family

// sg-example:begin run-control
auto bmFixed(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

[[maybe_unused]] const auto kFixedRegistrar =
    sg::registerBenchmark(&bmFixed, "DISABLED_bmFixed")
        .minTime(200'000'000)
        .warmupTime(50'000'000)
        .repetitions(5)
        .iterations(1000);

// sg-example:end run-control

// sg-example:begin timing-modes
auto bmHand(sg::State& state) -> void
{
  for (auto _ : state) {
    state.setIterationTime(1500);
  }
}

[[maybe_unused]] const auto kHandRegistrar =
    sg::registerBenchmark(&bmHand, "DISABLED_bmHand").useManualTime();

// sg-example:end timing-modes

// sg-example:begin timing-cpu
auto bmCpu(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

[[maybe_unused]] const auto kCpuRegistrar =
    sg::registerBenchmark(&bmCpu, "DISABLED_bmCpu").measureProcessCpuTime();

// sg-example:end timing-cpu

// sg-example:begin paused
auto bmPaused(sg::State& state) -> void
{
  std::vector<std::uint64_t> scratch(64, 1);
  for (auto _ : state) {
    {
      const sg::ScopedPauseTiming guard {state};
      scratch.assign(64, 1);
    }
    sg::doNotOptimize(scratch.front());
  }
}

[[maybe_unused]] const auto kPausedRegistrar =
    sg::registerBenchmark(&bmPaused, "DISABLED_bmPaused");

// sg-example:end paused

// sg-example:begin time-unit
auto bmMillis(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

[[maybe_unused]] const auto kMillisRegistrar =
    sg::registerBenchmark(&bmMillis, "DISABLED_bmMillis")
        .unit(sg::TimeUnit::MILLISECONDS);

// sg-example:end time-unit

auto installTimeUnit() -> void
{
  sg::setDefaultTimeUnit(sg::TimeUnit::MICROSECONDS);
}

// sg-example:begin skips
auto bmGuard(sg::State& state) -> void
{
  if (state.range(0) == 0) {
    state.skipWithMessage("no cells configured");
    return;
  }
  for (auto _ : state) {
    sg::doNotOptimize(state.range(0));
  }
}

[[maybe_unused]] const auto kGuardRegistrar =
    sg::registerBenchmark(&bmGuard, "DISABLED_bmGuard").arg(0);

// sg-example:end skips

// sg-example:begin skip-error
auto bmProbe(sg::State& state) -> void
{
  state.skipWithError("the counter leaf is absent");
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

[[maybe_unused]] const auto kProbeRegistrar =
    sg::registerBenchmark(&bmProbe, "DISABLED_bmProbe");
// sg-example:end skip-error

// sg-example:begin metrics
#include "speedgun-ng/counters.hpp"

auto attachMetric(sg::BenchmarkHandle& handle) -> void
{
  const auto cpu = sg::counters::System::local().object("cpu");
  if (!cpu.has_value()) {
    return;
  }
  const auto cycles = cpu->counter<sg::counters::Dim<0, 1>>("cpu-cycles");
  const auto retired = cpu->counter<sg::counters::Dim<0, 1>>("instructions");
  if (cycles.has_value() && retired.has_value()) {
    handle.addMetric(*retired / *cycles, "instructions/cycle");
  }
}

// sg-example:end metrics

auto bmTick(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

auto main() -> int
{
  installTimeUnit();
  auto handle = sg::registerBenchmark(&bmTick, "DISABLED_bmTick");
  attachMetric(handle);
  return 0;
}
