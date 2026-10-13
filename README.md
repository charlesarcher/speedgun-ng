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
speedgun-ng 0.8.0 build=Release
bmSum                    iterations=   1066261  real time/iter (ns)=  657.575  cpu time/iter (ns)=  656.663  overhead floor (ns)=  684.000  endpoint samples=2
```

## How you do it
### The run loop

A benchmark is a function over `State`: the loop body runs once per iteration, and the harness settles how many iterations the run needs.
<!-- sg-example: example/docs_snippets.cpp#run-loop -->
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

`SG_BENCHMARK` registers a plain function, `SG_BENCHMARK_CAPTURE` registers one with a captured argument, and `registerBenchmark` is the runtime form, naming the family itself and returning the `BenchmarkHandle` the family calls chain onto; `SG_BENCHMARK_TEMPLATE` registers one instantiation of a function template, shown in [template registrations](docs/pages/harness.md#template-registrations).
<!-- sg-example: example/docs_snippets.cpp#registration -->
```cpp
auto bmCells(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

SG_BENCHMARK(bmCells);

auto bmTable(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

const auto kTable = sg::registerBenchmark(&bmTable, "Table/cells");
```
A name splits at its first `/` into suite and case, so `Table/cells` is the case `cells` of the suite `Table`, and the runtime form hands back the handle the family calls chain onto. See [captured arguments](docs/pages/harness.md#captured-arguments) and [suite and case naming](docs/pages/harness.md#suite-and-case-naming).
### Argument families and ranges

`arg`, `args`, `range`, `ranges`, `denseRange`, and `argsProduct` build the instance list, `argName` and `argNames` label the positions, and the body reads a position with `state.range(i)`.
<!-- sg-example: example/docs_snippets.cpp#ranges -->
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

A `Fixture` subclass holds the state its methods measure, and three macros turn its methods into benchmark families.
<!-- sg-example: example/docs_snippets.cpp#fixture -->
```cpp
class TableFixture : public sg::Fixture
{
protected:
  std::vector<std::uint64_t> m_values {1, 2, 3};
};

SG_BENCHMARK_F(TableFixture, bmTableRead)(sg::State& state)
{
  for (auto _ : state) {
    sg::doNotOptimize(m_values.front());
  }
}

SG_BENCHMARK_DEFINE_F(TableFixture, bmTableSize)(sg::State&) {}

SG_BENCHMARK_REGISTER_F(TableFixture, bmTableSize)
    .setup([](sg::State&) {})
    .teardown([](sg::State&) {});
```
`SG_BENCHMARK_F` defines a method and registers it in one step; `SG_BENCHMARK_DEFINE_F` defines one to register later, and `SG_BENCHMARK_REGISTER_F` registers it and carries the `setup` and `teardown` callbacks, which the runner runs in the untimed region of each run, with the fixture's own `setUp` and `tearDown` between those callbacks and the loop. `bmTableSize` keeps an empty body here to hold the section down, and a body that never enters the loop prints `FAILED: the benchmark never entered the timed loop`. A fixture instance is named `FixtureClass/Method`, so the fixture class is its suite. See [fixtures](docs/pages/harness.md#fixtures) and [setup and teardown callbacks](docs/pages/harness.md#setup-and-teardown-callbacks).
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
<!-- sg-example: example/docs_snippets.cpp#paused -->
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
A closed pause is subtracted from every time column of the row. The calls stand inside the loop, where the run's sampling window is open, one pause may stand open at a time, a run carries at most eight closed pauses, and each pause costs one more sample pair. See [pause and resume](docs/pages/harness.md#pause-and-resume).
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

auto configureUnits() -> void
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
The report prints `SKIPPED: <reason>` for the first kind and `FAILED: <reason>` for the second, and `DISABLED_` at the front of an instance name keeps it registered while the run, the `--list` output, and every `--filter` match skip it. See [the `DISABLED_` prefix](docs/pages/harness.md#the-disabled_-prefix).
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
bmSum                     AGGREGATE  primary time/iter (ns) mean=  649.108 median=  649.499 sd=    0.942 cv=  0.0015 min=  648.034 max=  649.792 n=3
```
A row gives `iterations=`, `real time/iter (ns)=`, `cpu time/iter (ns)=`, `overhead floor (ns)=`, and `endpoint samples=`. The floor is the median cost of one sampling window for the run, so a fast loop prints a floor above its own per-iteration time, and the count is the sample pair taken at the window ends. With repetitions, a final `AGGREGATE` row gives mean, median, `sd=`, `cv=`, min, max, and `n=`.

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

The build page covers the platform matrix, the vendored dependencies, and the quality gates: [Building speedgun-ng](docs/pages/building.md). The name marks this library as a rethink of Speedgun, a tool its author built at Akuna: it shares no code with that project, and everything here came from the author and a local AI army.
