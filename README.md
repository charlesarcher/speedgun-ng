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

<!-- sg-example: example/quickstart.sh#configure -->
```sh
cmake -S . -B build -D CMAKE_BUILD_TYPE=Release
```

<!-- sg-example: example/quickstart.sh#build -->
```sh
cmake --build build -j
```

<!-- sg-example: example/quickstart.sh#install -->
```sh
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

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"

namespace
{

constexpr std::uint64_t kCells = 1 << 12;

auto bmSum(sg::State& state) -> void
{
  std::vector<std::uint64_t> cells(kCells, 1);
  std::uint64_t total = 0;
  for (auto _ : state) {
    for (const std::uint64_t cell : cells) {
      total += cell;
    }
    sg::doNotOptimize(total);
  }
}

}  // namespace

SG_BENCHMARK(bmSum);

auto main(int argc, char** argv) -> int
{
  return sg::speedgunMain(argc, argv);
}
```

Configure, build, and run it:

<!-- sg-example: example/quickstart.sh#consumer-configure -->
```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH="$HOME/.local"
```

<!-- sg-example: example/quickstart.sh#consumer-build -->
```sh
cmake --build build
```

<!-- sg-example: example/quickstart.sh#run -->
```sh
./build/first_benchmark
```

Its output on one host's run, with the numbers of that machine:

<!-- sg-sample-output: example/first_benchmark.expected#report -->
```text
speedgun-ng 0.8.0 build=Release
bmSum                    iterations=   1069106  real time/iter (ns)=  675.230  cpu time/iter (ns)=  674.173  overhead floor (ns)=  688.000  endpoint samples=2
```

## How you do it

### The run loop

A benchmark is a function over `State`: the loop body runs once per iteration, and the harness settles how many iterations the run needs.
<!-- sg-example: example/docs_snippets.cpp#run-loop -->
```cpp
auto bmLoop(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
  sg::clobberMemory();
}

SG_BENCHMARK(bmLoop);
```

`doNotOptimize` keeps a value live across the barrier and `clobberMemory` pushes cached values back to memory; `speedgun-ng/barrier.hpp` declares both, and it stands beside `speedgun-ng/benchmark.hpp` in every snippet below.

### Registration

`SG_BENCHMARK`, `SG_BENCHMARK_CAPTURE`, and `SG_BENCHMARK_TEMPLATE` register a plain function, a function with a captured argument, and one instantiation of a function template; `registerBenchmark` is the runtime form, which names the family itself and returns the `BenchmarkHandle` the family calls chain onto.
<!-- sg-example: example/docs_snippets.cpp#registration -->
```cpp
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
```
<!-- sg-example: example/docs_snippets.cpp#disabled-name -->
```cpp
auto bmQuiet(sg::State&) -> void {}

const auto kQuiet = sg::registerBenchmark(&bmQuiet, "DISABLED_bmQuiet");
```

A name splits at its first `/` into suite and case, so `bmSweep/wide` is the case `wide` of the suite `bmSweep` and `Table/typed` is the case `typed` of the suite `Table`; `DISABLED_` at the front of the expanded instance name keeps that instance registered, out of every `--filter` match, out of `--list`, and out of the run. See [captured arguments](docs/pages/harness.md#captured-arguments), [template registrations](docs/pages/harness.md#template-registrations), [the `DISABLED_` prefix](docs/pages/harness.md#the-disabled_-prefix), and [suite and case naming](docs/pages/harness.md#suite-and-case-naming).

### Argument families and ranges

`arg`, `args`, `range`, `ranges`, `denseRange`, and `argsProduct` build the instance list, `argName` and `argNames` label the positions, and the body reads a position with `state.range(i)`.
<!-- sg-example: example/docs_snippets.cpp#ranges -->
```cpp
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
```
<!-- sg-example: example/docs_snippets.cpp#ranges-family -->
```cpp
auto bmSquares(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.range(0) * state.range(0));
  }
}

SG_BENCHMARK(bmSquares).argName("cells").arg(32).range(128, 512).denseRange(
    1024, 2048, 512);
```

`rangeMultiplier(k)` scales every bound of every range of the family by `k`, so one call widens the whole sweep, and each instance prints as its own row, named by its labels. See [argument families](docs/pages/harness.md#argument-families) and [argument access](docs/pages/harness.md#argument-access).

### Fixtures

A `Fixture` subclass builds what its method measures, with `setUp` and `tearDown` either side of the loop.
<!-- sg-example: example/docs_snippets.cpp#fixture -->
```cpp
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
```

`SG_BENCHMARK_F` defines the method and registers it in one step, `SG_BENCHMARK_DEFINE_F` defines one to register later, and `SG_BENCHMARK_REGISTER_F` registers a defined method and carries the `setup` and `teardown` callbacks that run outside the loop. A fixture instance is named `FixtureClass/Method`, so the fixture class is its suite, and the template forms `SG_BENCHMARK_TEMPLATE_F`, `SG_BENCHMARK_TEMPLATE_DEFINE_F`, and `SG_BENCHMARK_TEMPLATE_METHOD_F` with `SG_BENCHMARK_TEMPLATE_INSTANTIATE_F` name an instance `FixtureClass<types>/Method`. See [fixtures](docs/pages/harness.md#fixtures) and [setup and teardown callbacks](docs/pages/harness.md#setup-and-teardown-callbacks).

### Run control

`minTime`, `warmupTime`, `iterations`, and `repetitions` bound the measuring window and repeat it.
<!-- sg-example: example/docs_snippets.cpp#run-control -->
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
<!-- sg-example: example/docs_snippets.cpp#timing-modes -->
```cpp
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
```

The primary time is the column the qualification test, the growth rule, and the aggregate statistics read: the CPU column by default, the real column under `useRealTime`, and the sum of the values the body reports through `setIterationTime` under `useManualTime`, which states that time in nanoseconds. `measureProcessCpuTime` leaves the primary time alone and selects the process CPU leaf for the CPU column. See [two time columns](docs/pages/harness.md#two-time-columns), [real time as the primary time](docs/pages/harness.md#real-time-as-the-primary-time), [manual time](docs/pages/harness.md#manual-time), and [the process CPU leaf](docs/pages/harness.md#the-process-cpu-leaf).

### Pause and resume

`pauseTiming` and `resumeTiming` take work out of the timed window, and `ScopedPauseTiming` is the guard that pairs them.
<!-- sg-example: example/docs_snippets.cpp#paused -->
```cpp
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
```

A closed pause is subtracted from every time column of the row, one pause may stand open at a time, and each pause costs one more sample pair. See [pause and resume](docs/pages/harness.md#pause-and-resume).

### The time unit

`unit` sets the unit of one family, and `setDefaultTimeUnit` sets the unit for the families that call no `unit`.
<!-- sg-example: example/docs_snippets.cpp#time-unit -->
```cpp
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
```

Call `setDefaultTimeUnit` in `main` before `speedgunMain`, and `TimeUnit` names the four scales a column can print in. See [time units](docs/pages/harness.md#time-units).

### Hardware counter metrics

`addMetric` attaches a counter expression to a family, behind the availability check the counter lookup returns.
<!-- sg-example: example/docs_snippets.cpp#metrics -->
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

The row carries the metric label with `ratio=`, `scaled=`, and `gap=` fields, and on a host where the leaf is unavailable the row prints the refusal kind in place of the number and the run continues. This block is the one snippet that adds a header to the pair: `speedgun-ng/counters.hpp`.

### Skips

`skipWithMessage` ends a run early when the work cannot be done, and `skipWithError` ends it as a failure.
<!-- sg-example: example/docs_snippets.cpp#skips -->
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

SG_BENCHMARK(bmGuard).arg(0);
```

The report prints `SKIPPED: <reason>` for the first kind and `FAILED: <reason>` for the second.

### The command line

One entry point, `speedgunMain`, reads the command line, and the options are long options alone.
<!-- sg-example: example/quickstart.sh#options -->
```sh
./build/first_benchmark --list
./build/first_benchmark --filter='bmSum' --min-time=0.2
./build/first_benchmark --counter=machine/monotonic --dry-run
./build/first_benchmark --catalog
```

`--min-time` and `--warmup-time` take seconds, and a usage error reports one line and exits with status 2.

### Reading the report

A run prints a version line, then one row per instance and repetition.
<!-- sg-example: example/first_benchmark.expected#aggregate -->
```text
bmSum                     AGGREGATE  primary time/iter (ns) mean=  656.219 median=  650.596 sd=   10.314 cv=  0.0157 min=  649.938 max=  668.122 n=3
```

A row gives `iterations=`, `real time/iter (ns)=`, `cpu time/iter (ns)=`, `overhead floor (ns)=`, and `endpoint samples=`. The floor is the median cost of one sampling window, measured for the run and printed beside the per-iteration columns, and the count is the sample pair taken at the window ends. With repetitions, a final `AGGREGATE` row gives mean, median, `sd=`, `cv=`, min, max, and `n=`.

## Quick reference

The API names this page teaches, with the object that owns each call:

| Name | Owner | What it does | Teaches |
| --- | --- | --- | --- |
| `SG_BENCHMARK` | macro | Registers one function as one family | [registration](#registration) |
| `SG_BENCHMARK_CAPTURE` | macro | Registers a function with its captured arguments | [registration](#registration) |
| `SG_BENCHMARK_TEMPLATE` | macro | Registers one instantiation of a function template | [registration](#registration) |
| `SG_BENCHMARK_F` | macro | Defines and registers a fixture method | [fixtures](#fixtures) |
| `SG_BENCHMARK_DEFINE_F` | macro | Defines a fixture method to register later | [fixtures](#fixtures) |
| `SG_BENCHMARK_REGISTER_F` | macro | Registers a defined fixture method and its callbacks | [fixtures](#fixtures) |
| `SG_BENCHMARK_TEMPLATE_F` | macro | Defines and registers a template fixture method | [fixtures](#fixtures) |
| `SG_BENCHMARK_TEMPLATE_DEFINE_F` | macro | Defines a template fixture method to register later | [fixtures](#fixtures) |
| `SG_BENCHMARK_TEMPLATE_METHOD_F` | macro | Defines one method for every instantiation | [fixtures](#fixtures) |
| `SG_BENCHMARK_TEMPLATE_INSTANTIATE_F` | macro | Registers one fixture instantiation | [fixtures](#fixtures) |
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
| `rangeMultiplier` | BenchmarkHandle | Scales every bound of every range of the family | [argument-families-and-ranges](#argument-families-and-ranges) |
| `denseRange` | BenchmarkHandle | Appends every value of a stepped sweep | [argument-families-and-ranges](#argument-families-and-ranges) |
| `argsProduct` | BenchmarkHandle | Appends one instance per combination of lists | [argument-families-and-ranges](#argument-families-and-ranges) |
| `argName` | BenchmarkHandle | Labels one argument position | [argument-families-and-ranges](#argument-families-and-ranges) |
| `argNames` | BenchmarkHandle | Labels the argument positions in order | [argument-families-and-ranges](#argument-families-and-ranges) |
| `minTime` | BenchmarkHandle | Sets the least measuring time of one instance, in nanoseconds | [run-control](#run-control) |
| `warmupTime` | BenchmarkHandle | Sets the untimed warm-up window in nanoseconds | [run-control](#run-control) |
| `iterations` | BenchmarkHandle | Fixes the iteration count of one instance | [run-control](#run-control) |
| `repetitions` | BenchmarkHandle | Runs the instance again that many times | [run-control](#run-control) |
| `setup` | BenchmarkHandle | Runs a callback before the loop of each repetition | [fixtures](#fixtures) |
| `teardown` | BenchmarkHandle | Runs a callback after the loop of each repetition | [fixtures](#fixtures) |
| `useRealTime` | BenchmarkHandle | Makes real time the primary time of the family | [timing-modes](#timing-modes) |
| `useManualTime` | BenchmarkHandle | Makes the reported times the primary time of the family | [timing-modes](#timing-modes) |
| `measureProcessCpuTime` | BenchmarkHandle | Selects the process CPU leaf for the CPU column | [timing-modes](#timing-modes) |
| `unit` | BenchmarkHandle | Sets the unit one family prints its times in | [the-time-unit](#the-time-unit) |
| `addMetric` | BenchmarkHandle | Attaches a counter expression to the family | [hardware-counter-metrics](#hardware-counter-metrics) |
| `State::range` | State | Reads one argument position of the instance | [argument-families-and-ranges](#argument-families-and-ranges) |
| `State::setIterationTime` | State | Reports one iteration's time in nanoseconds | [timing-modes](#timing-modes) |
| `State::pauseTiming` | State | Opens the pause that excludes the work below it | [pause-and-resume](#pause-and-resume) |
| `State::resumeTiming` | State | Closes the pause and records the interval | [pause-and-resume](#pause-and-resume) |
| `State::skipWithMessage` | State | Ends the run as skipped, with a reason | [skips](#skips) |
| `State::skipWithError` | State | Ends the run as failed, with a reason | [skips](#skips) |

| Option | What it does |
| --- | --- |
| `--filter` | Selects the instances whose names match |
| `--list` | Prints the registry and stops |
| `--repetitions` | Sets how many times one instance runs |
| `--min-time` | Sets the least measuring seconds of one instance |
| `--iterations` | Fixes the iteration count and skips calibration |
| `--warmup-time` | Sets the untimed warm-up seconds |
| `--dry-run` | Runs one iteration and prints the row shape |
| `--counter` | Adds one catalog counter to the report columns |
| `--catalog` | Prints the counter catalog and stops |

## Planned work

- Counters on the state
- Bytes and items throughput
- Labels
- Custom statistics
- Aggregate-only report output
- Complexity fitting
- Structured machine-readable output
- Histogram output

The build page covers the platform matrix, the vendored dependencies, and the quality gates: [Building speedgun-ng](docs/pages/building.md).

The name marks this library as a rethink of Speedgun, a tool its author built at Akuna: it shares no code with that project, and everything here came from the author and a local AI army.
