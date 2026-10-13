// The capability snippets the root page quotes, one region per capability
// the other example files do not carry. The file builds as a target so a
// pasted block cannot rot: each region holds a complete declaration plus
// its own registration, and no region is a fragment.
//
// The entry point never enters the runner, so no family below reaches a
// report or the `run-examples` target: see the comment beside the target in
// example/CMakeLists.txt.

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

SG_BENCHMARK(bmLoop);

// sg-example:end run-loop

// sg-example:begin registration
auto bmNamed(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

SG_BENCHMARK(bmNamed);

auto bmSweep(sg::State& state, int cells) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(cells);
  }
}

SG_BENCHMARK_CAPTURE(bmSweep, wide, 8);

template<class Value>
auto bmTyped(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(Value {1});
  }
}

SG_BENCHMARK_TEMPLATE(bmTyped, std::uint64_t);

const auto kTyped =
    sg::registerBenchmark(&bmTyped<std::uint64_t>, "Table/typed");

// sg-example:end registration

// sg-example:begin disabled-name
auto bmQuiet(sg::State&) -> void {}

const auto kQuiet = sg::registerBenchmark(&bmQuiet, "DISABLED_bmQuiet");

// sg-example:end disabled-name

// sg-example:begin ranges
auto bmCells(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.range(0) + state.range(1));
  }
}

SG_BENCHMARK(bmCells)
    .argNames({"width", "depth"})
    .args({8, 64})
    .ranges({{8, 8}, {16, 32}})
    .rangeMultiplier(2);

// sg-example:end ranges

// sg-example:begin ranges-family
auto bmSquares(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.range(0) * state.range(0));
  }
}

SG_BENCHMARK(bmSquares).argName("cells").arg(32).range(128, 512).denseRange(
    1024, 2048, 512);

// sg-example:end ranges-family

// sg-example:begin fixture
class TableFixture : public sg::Fixture
{
public:
  auto setUp(sg::State&) -> void override { m_values.assign(64, 1); }

  auto tearDown(sg::State&) -> void override { m_values.clear(); }

protected:
  std::vector<std::uint64_t> m_values;
};

SG_BENCHMARK_F(TableFixture, bmTableSum)

(sg::State& state)
{
  std::uint64_t total = 0;
  for (auto index : state) {
    total += m_values[static_cast<std::size_t>(index) % m_values.size()];
  }
  sg::doNotOptimize(total);
}

SG_BENCHMARK_DEFINE_F(TableFixture, bmTableTouch)

(sg::State& state)
{
  for (auto _ : state) {
    sg::doNotOptimize(m_values.front());
  }
}

SG_BENCHMARK_REGISTER_F(TableFixture, bmTableTouch)
    .setup([](sg::State& state) { sg::doNotOptimize(state.rangeCount()); })
    .teardown([](sg::State&) {});

// sg-example:end fixture

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

// sg-example:begin run-control
auto bmFixed(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

SG_BENCHMARK(bmFixed)
    .minTime(200'000'000)  // 200 ms, in nanoseconds
    .warmupTime(50'000'000)
    .iterations(1000)
    .repetitions(5);

// sg-example:end run-control

// sg-example:begin timing-modes
auto bmIo(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

SG_BENCHMARK(bmIo);

const auto kIoReal = sg::registerBenchmark(&bmIo, "Disk/real").useRealTime();

const auto kIoProcess =
    sg::registerBenchmark(&bmIo, "Disk/process").measureProcessCpuTime();

auto bmHand(sg::State& state) -> void
{
  for (auto _ : state) {
    state.setIterationTime(1500);
  }
}

SG_BENCHMARK(bmHand).useManualTime();

// sg-example:end timing-modes

// sg-example:begin paused
auto bmPaused(sg::State& state) -> void
{
  std::vector<std::uint64_t> scratch(64, 1);
  for (auto index : state) {
    if (index == 0) {
      state.pauseTiming();
      scratch.assign(128, 2);
      state.resumeTiming();
    }
    if (index == 1) {
      const sg::ScopedPauseTiming guard {state};
      scratch.shrink_to_fit();
    }
    sg::doNotOptimize(scratch.front());
  }
}

SG_BENCHMARK(bmPaused).iterations(4);

// sg-example:end paused

// sg-example:begin time-unit
auto bmMillis(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

const auto kMillis = sg::registerBenchmark(&bmMillis, "Table/millis")
                         .unit(sg::TimeUnit::MILLISECONDS);

auto installTimeUnit() -> void
{
  sg::setDefaultTimeUnit(sg::TimeUnit::MICROSECONDS);
}

// sg-example:end time-unit

auto bmTick(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

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

SG_BENCHMARK(bmGuard).arg(0);

// sg-example:end skips

auto main() -> int
{
  installTimeUnit();
  auto handle = sg::registerBenchmark(&bmTick, "Table/tick");
  attachMetric(handle);
  return 0;
}
