# speedgun-ng

<p align="center">
  <img src="docs/images/sg.jpg" alt="speedgun-ng" width="280">
</p>

A benchmark framework in the lineage of Google Benchmark, written in
C++23 for Linux, the one supported platform. A benchmark is a function
over a `State` object: you write the loop, register the function, and
read a report of time per iteration.

## What speedgun does

- Draws every time value and every counter value from the counters library of this project.
- Attaches hardware counter metrics to a benchmark, printed beside the timing columns of the same run.
- Guards its public API with design-by-contract checks at the boundaries.

## Quick start

Linux, a C++23 compiler, and CMake 3.20 or newer. Configure, build, and install the library:
<!-- sg-example: example/quickstart.sh#library -->
```sh
cmake -S . -B build -D CMAKE_BUILD_TYPE=Release
cmake --build build -j
cmake --install build --prefix "$HOME/.local"
```

The consumer project's `CMakeLists.txt`, whole: the package and the two targets a benchmark links:
<!-- sg-example: example/quickstart/CMakeLists.txt#package -->
```cmake
cmake_minimum_required(VERSION 3.20)

project(speedgun-ngQuickStart CXX)

find_package(speedgun-ng REQUIRED)

add_executable(first_benchmark first_benchmark.cpp)
target_link_libraries(first_benchmark
                      PRIVATE speedgun-ng::speedgun-ng speedgun-ng::harness)
```

The first benchmark program, whole:
<!-- sg-example: example/first_benchmark.cpp#program -->
```cpp
#include <cstdint>
#include <vector>

#include "speedgun-ng/barrier.hpp"  // sg::doNotOptimize is declared here
#include "speedgun-ng/benchmark.hpp"

auto bmSum(sg::State& state) -> void
{
  std::vector<std::uint64_t> cells(1 << 12, 1);
  std::uint64_t total = 0;
  for (auto _ : state) {
    for (const std::uint64_t cell : cells) {
      total += cell;
    }
    sg::doNotOptimize(total);
  }
}

SG_BENCHMARK(bmSum);

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
```

Configure, build, and run it:
<!-- sg-example: example/quickstart.sh#consumer -->
```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH="$HOME/.local"
cmake --build build
./build/first_benchmark
```

Its output on one host's run, with the numbers of that machine:
<!-- sg-sample-output: example/first_benchmark.expected#report -->
```text
speedgun-ng 0.9.0 build=Release
bmSum                    iterations=   1066261  real time/iter (ns)=  657.575  cpu time/iter (ns)=  656.663  overhead floor (ns)=  684.000  endpoint samples=2
```

## How you do it
### The run loop
A benchmark is a function over `State`: the loop body runs once per iteration, and the harness settles how many iterations the run needs.
<!-- sg-example: example/docs/run_loop.cpp#run-loop -->
```cpp
auto bmLoop(sg::State& state) -> void
{
  std::uint64_t total = 0;
  for (auto _ : state) {
    total += 1;
  }
  sg::doNotOptimize(total);
  sg::clobberMemory();
}

SG_BENCHMARK(bmLoop);
```
`doNotOptimize` keeps a value live across the barrier and `clobberMemory` pushes cached values back to memory; `speedgun-ng/barrier.hpp` declares both beside `speedgun-ng/benchmark.hpp`, the pair every snippet below includes.
### Registration
`SG_BENCHMARK` registers a plain function, and its row carries the function name.
<!-- sg-example: example/docs/registration.cpp#registration-macro -->
```cpp
auto bmCells(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

SG_BENCHMARK(bmCells);
```
`SG_BENCHMARK_CAPTURE` binds the arguments at registration, and the row names the capture label, `bmAdd/pair`.
<!-- sg-example: example/docs/registration.cpp#capture -->
```cpp
auto bmAdd(sg::State& state, int left, int right) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(left + right);
  }
}

SG_BENCHMARK_CAPTURE(bmAdd, pair, 2, 3);
```
`SG_BENCHMARK_TEMPLATE` registers one instantiation, and the row names it, `bmWidth<std::uint32_t>`.
<!-- sg-example: example/docs/registration.cpp#template -->
```cpp
template<typename Type>
auto bmWidth(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

SG_BENCHMARK_TEMPLATE(bmWidth, std::uint32_t);
```
### Argument families and ranges
`arg`, `args`, `range`, `ranges`, `denseRange`, and `argsProduct` build the instance list, `argName` and `argNames` label the positions, and the body reads a position with `state.range(i)`.
<!-- sg-example: example/docs/argument_families.cpp#ranges -->
```cpp
auto bmSquares(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.range(0) * state.range(0));
  }
}

SG_BENCHMARK(bmSquares)
    .argNames({"cells"})
    .arg(32)
    .args({96})
    .argsProduct({{4, 8}})
    .rangeMultiplier(2)
    .range(64, 512)
    .ranges({{4096, 8192}})
    .denseRange(1024, 2048, 512);
```
`argsProduct` takes the product of the lists it carries, `rangeMultiplier(k)` settles the growth factor for the `range` and `ranges` calls that follow it on that family, and every instance prints as its own row, named by its labels. See [argument families](docs/pages/harness.md#argument-families) and [argument access](docs/pages/harness.md#argument-access).
### Fixtures
A `Fixture` subclass holds the state its methods measure, and `setUp` and `tearDown` run around the timed loop.
<!-- sg-example: example/docs/fixtures.cpp#fixture -->
```cpp
class TableFixture : public sg::Fixture
{
public:
  auto setUp(sg::State& state) -> void override
  {
    m_values.assign(3, 1);
    sg::doNotOptimize(&state);
  }

  auto tearDown(sg::State& state) -> void override
  {
    m_values.clear();
    sg::doNotOptimize(&state);
  }

protected:
  std::vector<std::uint64_t> m_values {1, 2, 3};
};
```
`SG_BENCHMARK_F` defines and registers in one step; `SG_BENCHMARK_DEFINE_F` only defines and `SG_BENCHMARK_REGISTER_F` registers it, printing `TableFixture/bmTableRead` and `TableFixture/bmTableSize/1`.
<!-- sg-requires: example/docs/fixtures.cpp#fixture -->
<!-- sg-example: example/docs/fixtures.cpp#fixture-methods -->
```cpp
SG_BENCHMARK_F(TableFixture, bmTableRead)(sg::State& state)
{
  for (auto _ : state) {
    sg::doNotOptimize(m_values.front());
  }
}

SG_BENCHMARK_DEFINE_F(TableFixture, bmTableSize)(sg::State& state)
{
  for (auto _ : state) {
    sg::doNotOptimize(m_values.size());
  }
}

SG_BENCHMARK_REGISTER_F(TableFixture, bmTableSize).argsProduct({{1}});
```
### Run control
`minTime`, `warmupTime`, `iterations`, and `repetitions` bound the measuring window and repeat it.
<!-- sg-example: example/docs/run_control.cpp#run-control -->
```cpp
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
```
`minTime` and `warmupTime` take nanoseconds in the handle chain, while `--min-time` and `--warmup-time` take seconds on the command line, and each repetition prints a row with an aggregate row at the end of more than one.
### Timing modes
CPU time is the primary time of a run by default, `useRealTime` makes real time the primary time, and `useManualTime` lets the body report each iteration's own time.
<!-- sg-example: example/docs/timing_modes.cpp#timing-modes -->
```cpp
auto bmIo(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

SG_BENCHMARK(bmIo).useRealTime().measureProcessCpuTime();

auto bmHand(sg::State& state) -> void
{
  for (auto _ : state) {
    state.setIterationTime(1500);
  }
}

SG_BENCHMARK(bmHand).useManualTime();
```
The primary time is the column the qualification test, the growth rule, and the aggregate statistics read: the CPU column by default, the real column under `useRealTime`, and the sum of the values the body reports through `setIterationTime` under `useManualTime`, which states that time in nanoseconds. `measureProcessCpuTime` leaves the primary time alone and selects the process CPU leaf for the CPU column. See [two time columns](docs/pages/harness.md#two-time-columns), [real time as the primary time](docs/pages/harness.md#real-time-as-the-primary-time), [manual time](docs/pages/harness.md#manual-time), and [the process CPU leaf](docs/pages/harness.md#the-process-cpu-leaf).
### Pause and resume
`pauseTiming` and `resumeTiming` take work out of the timed window, and `ScopedPauseTiming` is the guard that pairs them.
<!-- sg-example: example/docs/pause_and_resume.cpp#paused -->
```cpp
auto bmPaused(sg::State& state) -> void
{
  std::vector<std::uint64_t> scratch(64, 1);
  for (auto _ : state) {
    state.pauseTiming();
    scratch.assign(128, 2);
    state.resumeTiming();
    {
      const sg::ScopedPauseTiming guard {state};
      scratch.shrink_to_fit();
    }
    sg::doNotOptimize(scratch.front());
  }
}

SG_BENCHMARK(bmPaused).iterations(3);
```
A closed pause is subtracted from every time column of the row. The calls stand inside the loop, where the run's sampling window is open, one pause may stand open at a time, and each pause costs one more sample pair. The room for those samples follows the loop: the recorder reserves the pause room the family's previous attempt used, and a pair it cannot hold closes through a running fold that leaves the columns unchanged, with `pause=folded` beside the row. See [pause and resume](docs/pages/harness.md#pause-and-resume).
### The time unit
`unit` sets the unit of one family, and `setDefaultTimeUnit` sets the unit for the families that call no `unit`.
<!-- sg-example: example/docs/time_unit.cpp#time-unit -->
```cpp
auto bmMillis(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

const auto kMillis = sg::registerBenchmark(&bmMillis, "Table/millis")
                         .unit(sg::TimeUnit::MILLISECONDS);

auto configureUnits() -> void
{
  sg::setDefaultTimeUnit(sg::TimeUnit::MICROSECONDS);
}
```
Call `setDefaultTimeUnit` in `main` before `speedgunMain`, and `TimeUnit` names the four scales a column can print in. See [time units](docs/pages/harness.md#time-units).
### Hardware counter metrics
`addMetric` attaches a counter expression to a family, behind the availability check the counter lookup returns.
<!-- sg-example: example/docs/counter_metrics.cpp#metrics -->
```cpp
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
```
The row carries the metric label with `ratio=`, `scaled=`, and `gap=` fields, and on a host where the leaf is unavailable the row prints the refusal kind in place of the number and the run continues. This block and the lookup below are the two snippets that add a header to the pair: `speedgun-ng/counters.hpp` and `speedgun-ng/counters_metrics.hpp`.

<!-- sg-requires: example/docs/counter_metrics.cpp#metrics -->
<!-- sg-example: example/docs/counter_metrics.cpp#metric-lookup -->
```cpp
#include <cstdio>
#include <string>
#include <string_view>

#include "speedgun-ng/counters_metrics.hpp"

auto printMetric(std::string_view name) -> void
{
  const auto metric = sg::counters::metricByName(name);
  if (!metric.has_value()) {
    std::printf(
        "%s: %s\n", std::string(name).c_str(), metric.error().message.c_str());
    return;
  }
  const auto plan =
      sg::counters::compile(sg::counters::System::local(), metric->expression);
  if (!plan.has_value()) {
    std::printf(
        "%s: %s\n", std::string(name).c_str(), plan.error().message.c_str());
    return;
  }
  sg::counters::Scope window {*plan};
  window.start();
  window.finish();
  std::printf("%s: %g group=%s unit=%s\n",
              std::string(name).c_str(),
              window.metric(metric->expression).value,
              std::string(metric->group).c_str(),
              std::string(metric->unit).c_str());
}
```

`metricByName` hands back a metric the host's table defines, so the reader names `cpi` rather than composing it, and the print carries the table's group and unit text beside the folded value. `--metric` puts the same metric in a report column, one column per name, in the order the names are given.
### Skips
`skipWithMessage` ends a run early when the work cannot be done, and `skipWithError` ends it as a failure.
<!-- sg-example: example/docs/skips.cpp#skips -->
```cpp
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

SG_BENCHMARK(bmGuard).denseRange(0, 4, 4);
```
The report prints `SKIPPED: <reason>` for the first kind and `FAILED: <reason>` for the second, and `DISABLED_` at the front of an instance name keeps it registered while the run, the `--list` output, and every `--filter` match skip it. See [the `DISABLED_` prefix](docs/pages/harness.md#the-disabled_-prefix).
### The command line
One entry point, `speedgunMain`, reads the command line, and the options are long options alone.
<!-- sg-example: example/docs/command_line.cpp#command-line-read -->
```cpp
auto bmRead(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

SG_BENCHMARK(bmRead).argNames({"cells"}).arg(256);
```
`--min-time` and `--warmup-time` take seconds, and a usage error reports one line and exits with status 2.
### Reading the report
A run prints a version line, then one row per instance and repetition.

<!-- sg-example: example/docs/reading_the_report.cpp#report-fields -->
```cpp
auto bmFill(sg::State& state) -> void
{
  const auto cells = state.range(0);
  std::vector<std::uint64_t> row(static_cast<std::size_t>(cells), 0);
  for (auto _ : state) {
    row.front() = static_cast<std::uint64_t>(cells);
    sg::doNotOptimize(row);
  }
  state.setBytesProcessed(cells * 8);
  state.setLabel("filled");
}

SG_BENCHMARK(bmFill).argsProduct({{8}}).threadRange(1, 2);
```
The row carries `bytes_per_second=` from the byte setter and the `label=` field the label setter gave it, and [Reading a report row](docs/pages/harness.md#reading-a-report-row) shows the full run with the `spread=` statistic and the `BIG_O` and `RMS` pair of the fit.
A row gives `iterations=`, `real time/iter (ns)=`, `cpu time/iter (ns)=`, `overhead floor (ns)=`, and `endpoint samples=`. The floor is the median cost of one sampling window for the run, so a fast loop prints a floor above its own per-iteration time, and the count is the sample pair taken at the window ends, plus one point for
each chunk when the run reports percentiles. With repetitions, a final `AGGREGATE` row gives mean, median, `sd=`, `cv=`, min, max, and `n=`. A threaded result prints its merged row, `bmFill/8/threads:2`, then the disclosure of the contexts it merged: `contexts=2  ctx0=SPIN/NO_PIN  ctx1=SPIN/NO_PIN`.

### User counters
<!-- sg-example: example/docs/user_counters.cpp#counter-column -->
```cpp
auto bmCount(sg::State& state) -> void
{
  const auto cells = static_cast<std::size_t>(state.range(0));
  std::uint64_t sink = 0;
  for (auto _ : state) {
    sink += cells;
    state.setCounter("cells", {static_cast<double>(cells)});
    sg::doNotOptimize(sink);
  }
}

SG_BENCHMARK(bmCount).argsProduct({{64}}).repetitions(3);
```
A counter is a named number the loop reports beside the times, and its flags say how the value scales. A run of `bmCopyCells` prints `cells=` and the `bytes_per_second=` rate beside the two time columns, and its `AGGREGATE` row carries one statistic line for each counter, so `cells mean=` and `bytes_per_second mean=` stand under the built-in ones. Names are capped at `kMaxUserCounters`, sixteen for one benchmark, and a name must be shorter than `kMaxCounterNameBytes`, so 31 characters; both bounds are `SG_REQUIRE` preconditions on `State::setCounter`, and a breach ends the run with the bound in its message. See [user counters](docs/pages/harness.md#user-counters).

### Throughput
<!-- sg-example: example/docs/throughput.cpp#throughput-call -->
```cpp
auto bmScale(sg::State& state) -> void
{
  std::int64_t work = 0;
  for (auto _ : state) {
    work += static_cast<std::int64_t>(state.iterations());
    sg::doNotOptimize(work);
  }
  state.setItemsProcessed(work);
  state.setBytesProcessed(work * 4);
}

SG_BENCHMARK(bmScale).argsProduct({{1}}).repetitions(3);
```
`setBytesProcessed` and `setItemsProcessed` open the two rate counters the harness derives from the time it measured. The row gains `items_per_second=` and `bytes_per_second=` columns, each one the total the call passed divided by the primary time of the run, so a larger argument shows the throughput the loop held. Both calls take the total work of the whole run: a body that works a known amount per iteration passes that amount times `state.iterations()` after the loop, as the example does. See [throughput](docs/pages/harness.md#throughput).

### Labels
A label is a trailing word a run prints after its numbers, so a row says which variant it measured.
<!-- sg-example: example/docs/labels.cpp#labels -->
```cpp
auto bmScanBlocks(sg::State& state) -> void
{
  const auto cells = static_cast<std::size_t>(state.range(0));
  std::vector<std::uint64_t> block(cells, 3);
  for (auto _ : state) {
    std::uint64_t total = 0;
    for (const std::uint64_t cell : block) {
      total += cell;
    }
    sg::doNotOptimize(total);
  }
  state.setLabel("summed");
}

SG_BENCHMARK(bmScanBlocks).argNames({"cells"}).range(256, 4096).repetitions(3);
```
`label=summed` stands last on every row of the family, after the counters, and the `AGGREGATE` row carries the label the repetitions agreed on. See [labels](docs/pages/harness.md#labels).

### Custom statistics
A custom statistic turns the repetition times of one instance into one more named number on its aggregate row.
<!-- sg-example: example/docs/custom_statistics.cpp#custom-statistics -->
```cpp
auto spreadOf(const std::vector<double>& times, double& spread) -> bool
{
  if (times.empty()) {
    return false;
  }
  double lo = times[0];
  double hi = times[0];
  for (const double value : times) {
    lo = value < lo ? value : lo;
    hi = value > hi ? value : hi;
  }
  if (lo <= 0.0) {
    return false;
  }
  spread = (hi - lo) / lo * 100.0;
  return true;
}
```
`BenchmarkHandle::addStatistic("spread", sg::StatUnit::PERCENTAGE, &spreadOf)` attaches the callback to the family. `spread=    0.110` then prints beside the built-in statistics with the percentage unit the registration declared, and the names `mean`, `median`, `sd`, `cv`, `min`, `max` and `n` stay reserved: a callback registered under one of them breaches its precondition at the call. See [custom statistics](docs/pages/harness.md#custom-statistics).

### Aggregate-only reporting
`reportAggregatesOnly` keeps the repetition rows out of the report and leaves the aggregate.
<!-- sg-example: example/docs/aggregates_only.cpp#aggregates-only -->
```cpp
auto bmCountWords(sg::State& state) -> void
{
  const std::string line =
      "speedgun-ng reports one row per repetition and one aggregate";
  for (auto _ : state) {
    std::size_t spaces = 0;
    for (const char letter : line) {
      spaces += letter == ' ' ? 1 : 0;
    }
    sg::doNotOptimize(spaces);
  }
}

SG_BENCHMARK(bmCountWords).repetitions(5).reportAggregatesOnly();
```
The family prints its single `AGGREGATE` row for the five repetitions and nothing above it; `displayAggregatesOnly` applies the same rule to every family from the command line. See [aggregate-only reporting](docs/pages/harness.md#aggregate-only-reporting).

### Complexity fit
A family fitted over its complexity sizes reports the curve each of its quantities follows.
<!-- sg-example: example/docs/complexity_fit.cpp#complexity-fit -->
```cpp
auto bmSwapPasses(sg::State& state) -> void
{
  const auto cells = static_cast<std::size_t>(state.range(0));
  std::vector<std::uint64_t> values(cells, 1);
  for (auto _ : state) {
    for (std::size_t row = 0; row < cells; ++row) {
      for (std::size_t col = 0; col + 1 < cells; ++col) {
        std::swap(values[col], values[col + 1]);
      }
    }
    sg::doNotOptimize(values);
  }
}

SG_BENCHMARK(bmSwapPasses)
    .argsProduct({{8, 16, 32}})
    .complexity(sg::BigO::AUTOMATIC);
```
The family ends with a `BIG_O` row naming the curve and its coefficient, an `RMS` row with the fit error as a percentage, one pair for every quantity the family carries, and a `SKIPPED` row naming a quantity a member never set together with its reason. See [complexity fit](docs/pages/harness.md#complexity-fit).

### Execution contexts
A context is a thread the library runs, with an inbox of typed work and a policy fixing its wait mode and placement.
<!-- sg-example: example/docs/execution_contexts.cpp#noise-load -->
```cpp
const auto kNoiseLoad = [](sg::WorkContext& work) noexcept
{
  while (!work.stopToken().stopRequested()) {
  }
};

auto main(int argc, char** argv) -> int
{
  const sg::ContextPolicy spin {.waitMode = sg::WaitMode::SPIN};
  sg::ExecutionContext load {spin};
  sg::WorkNode<decltype(kNoiseLoad)> node {kNoiseLoad};
  sg::dispatch(load, node);
  const int status = sg::speedgunMain(argc, argv);
  load.requestStop();
  load.drain();
  load.join();
  return status;
}
```
The `bmQuiet` row prints while the load spins on its own context; `requestStop`, `drain`, and `join` close the context after the run. See [execution context lifecycle](docs/pages/harness.md#execution-context-lifecycle).

### Threads
`threads(4)` runs one benchmark body on four contexts at once and prints one merged row for them.
<!-- sg-example: example/docs/threads.cpp#threads -->
```cpp
auto bmSharded(sg::State& state) -> void
{
  const int index = state.threadIndex();
  for (auto _ : state) {
    sg::doNotOptimize(index + state.threads());
  }
}

SG_BENCHMARK(bmSharded).threads(4);
```
`state.threadIndex()` numbers the context running the loop, and `state.threads()` gives the count the instance asked for. See [what a threaded row merges](docs/pages/harness.md#what-a-threaded-row-merges).

### Percentiles
<!-- sg-example: example/docs/percentiles.cpp#percentiles -->
```cpp
auto bmRoll(sg::State& state) -> void
{
  std::uint64_t roll = 0;
  for (auto _ : state) {
    for (std::uint64_t i = 0; i < 30'000; ++i) {
      roll = (roll + i) * 1'099'511'628'211ULL;
    }
    sg::doNotOptimize(roll);
  }
}

SG_BENCHMARK(bmRoll).iterations(300).showPercentiles(true);
```
The run divides its iterations into chunks, times each chunk, and reads the nearest rank of the chunk times, so `showPercentiles(true)` reports how one run's iterations varied. `--percentiles=50,90` sets the points of the run and `--no-percentiles` turns the lines off; a point the chunk count cannot answer prints `not available`. See [percentiles](docs/pages/harness.md#percentiles).

## Quick reference

| Name | Owner | What it does | Teaches |
| --- | --- | --- | --- |
| `SG_BENCHMARK` | macro | Registers one function as one family | [registration](#registration) |
| `SG_BENCHMARK_CAPTURE` | macro | Registers a function with its captured arguments | [registration](#registration) |
| `SG_BENCHMARK_TEMPLATE` | macro | Registers one instantiation of a function template | [registration](#registration) |
| `SG_BENCHMARK_F` | macro | Defines and registers a fixture method | [fixtures](#fixtures) |
| `SG_BENCHMARK_DEFINE_F` | macro | Defines a fixture method to register later | [fixtures](#fixtures) |
| `SG_BENCHMARK_REGISTER_F` | macro | Registers a defined fixture method and its callbacks | [fixtures](#fixtures) |
| `State` | type | The object a benchmark loop iterates over | [the-run-loop](#the-run-loop) |
| `BenchmarkHandle` | type | The handle a registration returns, chaining one family's calls | [registration](#registration) |
| `Fixture` | type | Builds state before the loop and releases it after | [fixtures](#fixtures) |
| `ScopedPauseTiming` | type | Takes a scope out of the timed window | [pause-and-resume](#pause-and-resume) |
| `TimeUnit` | type | The four scales a time column prints in | [the-time-unit](#the-time-unit) |
| `registerBenchmark` | free function | Registers a family under a name you give | [registration](#registration) |
| `setDefaultTimeUnit` | free function | Sets the unit for families that call no `unit` | [the-time-unit](#the-time-unit) |
| `speedgunMain` | free function | Runs the registry against the command line | [the-command-line](#the-command-line) |
| `doNotOptimize` | free function | Keeps a value live across the barrier | [the-run-loop](#the-run-loop) |
| `clobberMemory` | free function | Pushes cached values back to memory | [the-run-loop](#the-run-loop) |
| `arg` | BenchmarkHandle | Appends one argument value to the instance list | [argument-families-and-ranges](#argument-families-and-ranges) |
| `args` | BenchmarkHandle | Appends a list of argument values | [argument-families-and-ranges](#argument-families-and-ranges) |
| `range` | BenchmarkHandle | Appends every value of a multiplied sweep | [argument-families-and-ranges](#argument-families-and-ranges) |
| `ranges` | BenchmarkHandle | Appends one position per bound pair | [argument-families-and-ranges](#argument-families-and-ranges) |
| `rangeMultiplier` | BenchmarkHandle | Settles the growth factor of later `range` calls | [argument-families-and-ranges](#argument-families-and-ranges) |
| `denseRange` | BenchmarkHandle | Appends every value of a stepped sweep | [argument-families-and-ranges](#argument-families-and-ranges) |
| `argsProduct` | BenchmarkHandle | Appends one instance per combination of lists | [argument-families-and-ranges](#argument-families-and-ranges) |
| `argName` | BenchmarkHandle | Labels one argument position | [argument-families-and-ranges](#argument-families-and-ranges) |
| `argNames` | BenchmarkHandle | Labels the argument positions in order | [argument-families-and-ranges](#argument-families-and-ranges) |
| `minTime` | BenchmarkHandle | Sets the least measuring time of one instance, in nanoseconds | [run-control](#run-control) |
| `warmupTime` | BenchmarkHandle | Sets the untimed warm-up window in nanoseconds | [run-control](#run-control) |
| `iterations` | BenchmarkHandle | Fixes the iteration count of one instance | [run-control](#run-control) |
| `repetitions` | BenchmarkHandle | Sets how many runs one instance makes | [run-control](#run-control) |
| `setup` | BenchmarkHandle | Runs a callback before the timed loop of each run | [fixtures](#fixtures) |
| `teardown` | BenchmarkHandle | Runs a callback after the timed loop of each run | [fixtures](#fixtures) |
| `useRealTime` | BenchmarkHandle | Makes real time the primary time of the family | [timing-modes](#timing-modes) |
| `useManualTime` | BenchmarkHandle | Makes the reported times the primary time of the family | [timing-modes](#timing-modes) |
| `measureProcessCpuTime` | BenchmarkHandle | Selects the process CPU leaf for the CPU column | [timing-modes](#timing-modes) |
| `unit` | BenchmarkHandle | Sets the unit one family prints its times in | [the-time-unit](#the-time-unit) |
| `addMetric` | BenchmarkHandle | Attaches a counter expression to the family | [hardware-counter-metrics](#hardware-counter-metrics) |
| `metricByName` | free function | Resolves one metric of the host's table by name | [hardware-counter-metrics](#hardware-counter-metrics) |
| `State::range` | State | Reads one argument position of the instance | [argument-families-and-ranges](#argument-families-and-ranges) |
| `State::setIterationTime` | State | Reports one iteration's time in nanoseconds | [timing-modes](#timing-modes) |
| `State::pauseTiming` | State | Opens the pause that excludes the work below it | [pause-and-resume](#pause-and-resume) |
| `State::resumeTiming` | State | Closes the pause and records the interval | [pause-and-resume](#pause-and-resume) |
| `State::skipWithMessage` | State | Ends the run as skipped, with a reason | [skips](#skips) |
| `State::skipWithError` | State | Ends the run as failed, with a reason | [skips](#skips) |
| `setCounter` | State | Reports a named number beside the times | [user counters](#user-counters) |
| `setBytesProcessed` | State | Opens the `bytes_per_second` rate counter | [user counters](#user-counters) |
| `setItemsProcessed` | State | Opens the `items_per_second` rate counter | [user counters](#user-counters) |
| `setLabel` | State | Prints a trailing word after the row | [labels](#labels) |
| `setComplexityN` | State | Sets the size the complexity fit scales against | [complexity fit](#complexity-fit) |
| `Counter` | type | The named number one call reports | [user counters](#user-counters) |
| `CounterFlag` | type | How a counter value scales into its column | [user counters](#user-counters) |
| `CounterBase` | type | The `k` or `Ki` suffix a rendered counter carries | [user counters](#user-counters) |
| `StatUnit` | type | The unit a custom statistic prints in | [custom statistics](#custom-statistics) |
| `BigO` | type | The curves a complexity fit searches | [complexity fit](#complexity-fit) |
| `BigOFun` | type | The curve shape a fit of your own supplies | [complexity fit](#complexity-fit) |
| `addStatistic` | BenchmarkHandle | Adds a named aggregate statistic to the report | [custom statistics](#custom-statistics) |
| `reportAggregatesOnly` | BenchmarkHandle | Keeps the repetition rows of this family out | [aggregate only reporting](#aggregate-only-reporting) |
| `displayAggregatesOnly` | BenchmarkHandle | Trims the console of this family to its aggregate rows | [aggregate only reporting](#aggregate-only-reporting) |
| `complexity` | BenchmarkHandle | Chooses the curve the fit searches | [complexity fit](#complexity-fit) |
| `ExecutionContext` | type | One thread, its inbox and the policy it runs under | [execution contexts](#execution-contexts) |
| `ContextPolicy` | type | The wait mode and placement one context starts under | [execution contexts](#execution-contexts) |
| `WaitMode` | type | The three ways a waiting context gives up its cpu | [execution contexts](#execution-contexts) |
| `PlacementSlot` | type | The cpu one context asks to be pinned to | [execution contexts](#execution-contexts) |
| `kNoPin` | constant | The slot value that asks for no pin | [execution contexts](#execution-contexts) |
| `ContextId` | type | The identity one context reports | [execution contexts](#execution-contexts) |
| `currentContext` | free function | Names the context running the calling code | [execution contexts](#execution-contexts) |
| `setPlacementFunction` | free function | Installs the function that places a pinned context | [execution contexts](#execution-contexts) |
| `PlacementResult` | type | The outcome a placement function reports | [execution contexts](#execution-contexts) |
| `PlacementFn` | type | The callback the library calls to place a context | [execution contexts](#execution-contexts) |
| `WorkItem` | concept | The callable shape a context accepts as work | [execution contexts](#execution-contexts) |
| `WorkNode` | type | Caller storage for one dispatched work item | [execution contexts](#execution-contexts) |
| `WorkNodeState` | type | The state a dispatched node reports | [execution contexts](#execution-contexts) |
| `dispatch` | free function | Offers one node to a context and returns | [execution contexts](#execution-contexts) |
| `WorkContext` | type | The running work item's view of its context | [execution contexts](#execution-contexts) |
| `StopToken` | type | The read side of the stop request of a context | [execution contexts](#execution-contexts) |
| `ContextBarrier` | type | The point several contexts wait at together | [execution contexts](#execution-contexts) |
| `threads` | BenchmarkHandle | Runs every instance of the family on a context count | [threads](#threads) |
| `threadRange` | BenchmarkHandle | Appends one instance per doubling of a thread sweep | [threads](#threads) |
| `denseThreadRange` | BenchmarkHandle | Appends one instance per step of a thread sweep | [threads](#threads) |
| `threadRunner` | BenchmarkHandle | Attaches the dispatcher of this family's threaded runs | [threads](#threads) |
| `ThreadRun` | type | The view of one threaded attempt its dispatcher receives | [threads](#threads) |
| `ThreadRunnerFn` | type | The callback shape a threaded dispatcher takes | [threads](#threads) |
| `State::threadIndex` | State | Numbers the context running the loop | [threads](#threads) |
| `State::threads` | State | Gives the context count the instance was run with | [threads](#threads) |
| `ContextDisclosure` | type | The wait mode and placement one context disclosed | [reading the report](#reading-the-report) |
| `BenchmarkResult::contexts` | BenchmarkResult | The disclosures one run recorded for its contexts | [reading the report](#reading-the-report) |

| Option | What it does |
| --- | --- |
| `--filter` | Selects the instances whose names match |
| `--list` | Prints the registry and stops |
| `--repetitions` | Sets how many times one instance runs |
| `--min-time` | Sets the least measuring seconds of one instance |
| `--iterations` | Fixes the iteration count and skips calibration |
| `--warmup-time` | Sets the untimed warm-up seconds |
| `--percentiles` | Sets the percentile points one run reports |
| `--no-percentiles` | Turns the percentile lines off |
| `--dry-run` | Runs one iteration and prints the row shape |
| `--counter` | Adds one catalog counter to the report columns |
| `--metric` | Adds one metric of the host's table to the report columns |
| `--catalog` | Prints the counter catalog and stops |

## Planned work

- Structured machine-readable output
- Histogram output

The build page covers the platform matrix, the vendored dependencies, and the quality gates: [Building speedgun-ng](docs/pages/building.md). The name marks this library as a rethink of Speedgun, a tool its author built at Akuna: it shares no code with that project, and everything here came from the author and a local AI army.
