# The benchmark harness surface

This page states the registration and timing surface of the benchmark
harness: what each call does, the name it produces, and the precondition
it carries. Each section closes with the test that covers it. Every
suite named here runs on the counters fake provider, so no test needs a
PMU.

## Argument families

The five registration macros `SG_BENCHMARK`,
`SG_BENCHMARK_CAPTURE`, `SG_BENCHMARK_TEMPLATE`,
`SG_BENCHMARK_REGISTER_F` and `SG_BENCHMARK_TEMPLATE_INSTANTIATE_F`
each declare one handle initialized by the registration, so the
family calls chain at the site and the site ends with `;`:
`SG_BENCHMARK(fn).range(8, 1024);`. A site with no chain reads
`SG_BENCHMARK(fn);`.

The ten family calls stand on `BenchmarkHandle` and each returns the
handle for chaining: `arg`, the two `args` overloads over an
`std::initializer_list` and an `std::vector`, `range`,
`rangeMultiplier`, `ranges`, `denseRange`, `argsProduct`, `apply`,
`argName`, and `argNames`. Every family argument is a
`std::int64_t`. `argName` sets the label list to the one label and
`argNames` sets it to its whole list; a later label call replaces the
list that stood before. One `args` call appends one argument list, so
`args({8, 64})` states one instance of arity two while `arg(8).arg(64)`
states two instances of arity one. `range` and `ranges` grow by powers
of the range multiplier with both bounds included, the default
multiplier is `kDefaultRangeMultiplier` (8), and `rangeMultiplier`
settles the multiplier for the later range calls of that family.
`denseRange` steps from the low bound to the high bound inclusive.
`argsProduct` takes the product of its lists with the odometer
advancing the first list fastest. The free builders `sg::createRange`
and `sg::createDenseRange` return the `std::vector<std::int64_t>` lists
a family call accepts. A multiplier below 2, a low bound above a high
bound, a `denseRange` step below 1, and an arity or label count unequal
to the family arity are `SG_REQUIRE` precondition violations. A family
above `kMaxFamilySize` (100 instances) draws one warning on the
standard error stream, keeps its instances, and the run continues.
Expansion completes at the start of `speedgunMain`, before the filter
and the first run. The example family `bmArgs` in
`example/benchmark_example.cpp` carries the `argName` labels `width` and
`depth`.

Each registration site yields one instance per argument, and the instance
set is exactly the arguments that call named, whether they come from a list,
a range, a step, a product, an apply, a builder or a chained site. An
argument list over the bound warns once naming the bound and keeps the
instances that fit. A multiplier, range, step, argument name, apply function
or builder that breaks a precondition ends the run with that precondition in
the message.

## Instance names

An instance name starts with the family name and adds one `/`-joined
segment per argument. A segment reads `label:value` where `argName` or
`argNames` set a label for that position, and an empty label leaves the
segment without one. A family with no family call keeps the family name
alone, with no `/`. The `--filter` option and list mode match
instance names, so one full instance name selects one instance and the
`--catalog` path prints none of them. Two instances with one name are a
recoverable error found at expansion: one line on the standard error
stream names both instance names, the earlier instance stays, the later
instance does not run, and the exit status stays as the runner fixes it.

An instance name is what a filter matches, and one full name selects exactly
one instance. A repeated instance name is reported, the first registration
is kept, the later one dropped, and the exit status is unchanged. The
catalog walk covers the same set without printing rows.

## Argument access

`State::range(std::size_t index)` returns the argument of the running
instance at that position and `State::rangeCount()` returns the
argument count. An index at or above `rangeCount()` is an `SG_REQUIRE`
precondition violation. A family with no family call has one instance
with zero arguments, so `rangeCount()` reports 0 and `range(0)`
violates. A read allocates nothing: the instance owns the storage and
the state holds a `std::span<const std::int64_t>` fixed at expansion.
The timed loop keeps its shape, and the loop-shape gate
`test/loop_shape.sh` stays green.

`range(i)` reads the argument of the instance that runs and `rangeCount()`
the number of arguments it was given. Reading an argument allocates nothing.
An index below the first argument or past the last one breaks the
precondition of the accessor and ends the run.

## Captured arguments

`SG_BENCHMARK_CAPTURE(fn, captureName, ...)` registers `fn` once with
the captured values bound into the callable, under the instance name
`fn/captureName`. The capture name is one name segment, and the
instance takes part in the filter, list mode, and duplicate handling of
the instance names above like any other instance. The registration yields
the handle, so a
site chains family calls: `SG_BENCHMARK_CAPTURE(fn, pair, 2).arg(4);`
registers `fn/pair/4`.

A capture name groups the values one run records while the instance keeps
its own name beside it. The same function may be registered under two
capture names, and a chained site carries its capture name into the instance
name and its argument into the state. A capture name that collides with a
family instance name is settled at the duplicate tier, where the earlier
registration stays.

## Template registrations

`SG_BENCHMARK_TEMPLATE(fn, ...)` registers a function template
instantiated over one or more type arguments, under the instance name
`fn<` followed by the type list stringified exactly as written at the
macro site, followed by `>`. There is no demangling: the spacing at the
macro site is the spacing in the name. The registration yields the
handle, so a site chains family calls:
`SG_BENCHMARK_TEMPLATE(fn, int).arg(8);` registers `fn<int>/8`. The
example `SG_BENCHMARK_TEMPLATE(bmFill, std::uint64_t)` in
`example/benchmark_example.cpp` is an instance named
`bmFill<std::uint64_t>`.

Type arguments appear in the instance name in the order the macro site wrote
them, keeping the spacing of that site through a comma and through a nested
template. A chained site names each instantiation, and the instantiation the
run reaches is the one the type list names.

## Fixtures

`sg::Fixture` is the fixture base class: a virtual destructor and the
virtual `setUp(State&)` and `tearDown(State&)` with empty defaults. The
seven macros below attach a method of a derived class:
`SG_BENCHMARK_F`, `SG_BENCHMARK_DEFINE_F` paired with
`SG_BENCHMARK_REGISTER_F`, `SG_BENCHMARK_TEMPLATE_F`,
`SG_BENCHMARK_TEMPLATE_DEFINE_F`, `SG_BENCHMARK_TEMPLATE_METHOD_F`
paired with `SG_BENCHMARK_TEMPLATE_INSTANTIATE_F`. A fixture instance
is named `FixtureClass/Method`, and a template fixture instance is
named `BaseClass<types>/Method`. The pair runs in the untimed region of
every run, the warm-up, calibration, and measured runs included, and
the runner builds one fixture object per run and destroys it after
`tearDown`. The state the pair receives is the callback state of the
setup and teardown callbacks section:
`range(index)`, `rangeCount()` and `iterations()` are legal in it,
while `begin()`, `skipWithError` and `skipWithMessage` are `SG_REQUIRE`
precondition violations. `SG_BENCHMARK_REGISTER_F` and
`SG_BENCHMARK_TEMPLATE_INSTANTIATE_F` yield the handle, so a fixture
family chains its family calls at the site:
`SG_BENCHMARK_REGISTER_F(F, m).denseRange(1, 3);` expands the fixture
record like any family record, one instance per argument list.
`TableFixture/bmTableTouch`, registered with `SG_BENCHMARK_F`, stands in
`example/benchmark_example.cpp`.

A fixture instance name is the fixture and the method. One fixture object is
built and destroyed for each run, and the setup and teardown pair runs once
per run, including a run of a fixed-count, repeated or warm-up instance.
Untimed work in a setup callback does not reach the reported time, which is
the delta of the timed leaf alone. A method reads the argument and iteration
counts from the state of its own run, and `begin()` or `skipWithError()`
called inside the pair aborts the run. A template fixture yields one
instance per instantiation, a split pair spans the two halves, a chained
site expands into one instance per argument each with its own pair, and a
`DISABLED_` method is excluded like any other disabled name.

## Setup and teardown callbacks

`BenchmarkHandle::setup` and `BenchmarkHandle::teardown` attach one
callback per slot, and the last attachment wins the slot. The pair runs
once around each run, the warm-up, calibration, and measured runs
included, in the untimed region, and the order in one run is the setup
callback, the fixture `setUp`, the callable, the fixture `tearDown`,
the teardown callback. The state a callback receives carries the
instance arguments and the iteration count: `range(index)`,
`rangeCount()` and `iterations()` are legal there, while `begin()`,
`skipWithError` and `skipWithMessage` are `SG_REQUIRE` precondition
violations. `end()` stays legal because it is static and touches no
state. The harness runs one thread today; the once-per-thread-group rule
belongs to a future threads specification.

The setup and teardown callbacks run once per run for a plain instance, a
repeated instance and a fixture. A callback reads the arguments and the
counts of the running instance from its state. An attachment whose body
never runs stays at zero. The wrapping order is fixed: the fixture pair sits
outside the callbacks of the instance it wraps. A callback that calls
`begin()`, `skipWithError()` or `skipWithMessage()` on a state it does not
own aborts the process.

## The `DISABLED_` prefix

The gate is a prefix test on the expanded instance name, held at
selection time in `source/harness/cli.cpp`. An instance whose name
starts with `DISABLED_` registers, expands, never runs, stays out of
the filter match, and stays out of list mode. A fixture method named
`DISABLED_x` runs, because that instance name starts with the fixture
class. A `DISABLED_` family as the only match of a filter leaves the
executable reporting no match, running nothing, and exiting zero.

A family name with the `DISABLED_` prefix never runs and never appears in
the listed set. The prefix is read only where a name begins, so a fixture
method and a name that merely contains it are unaffected. A filter that
matches nothing reports the no match and exits with a zero status.

## Suite and case naming

`BenchmarkResult` carries `suite` and `caseName` beside `name`.
The suite is the family name up to its first `/`, and a fixture
instance takes its fixture class as the suite. The case is the instance
name with the leading suite and its `/` removed, and an instance name
equal to its suite keeps that name as its case. The pair is unique per
instance. After expansion the instance list groups stably by suite in
first-appearance order, and rows within one suite keep registration
order. The console row keeps one name column carrying the full instance
name, with the split readable on the result value. A
later JSON report reuses the two fields.

The suite and case fields of a result come from the family name and the
instance name of the run, and instances of one suite are reported together.
Instance names stay unique across the list, and the console prints a single
name column.

## Two time columns

Every measured row prints two time columns, `real time/iter (ns)=` and
`cpu time/iter (ns)=`. Each value is a fold over the raw point pair of
that run, and the counters library supplies the fold. A pause whose pair
the recorder has no room for closes through a running fold over the same
leaf, and the column reads the same value in both paths. The CPU column
follows `machine/thread_cpu` until `measureProcessCpuTime` moves it.
The row keeps its other fields around the pair:

```text
<name>  iterations=<N>  real time/iter (ns)=<v>  cpu time/iter (ns)=<v>
        overhead floor (ns)=<v>  endpoint samples=<S>
```

`ResultRow` carries the pair as `timePerIterationNs` and
`cpuTimePerIterationNs`, each one nanosecond per iteration, and carries
`endpointSamples` as the count the row prints. A run that pauses
nothing takes one sample pair, so that count reads 2 samples and the
timed loop keeps its shape: the loop-shape gate
`test/loop_shape.sh` stays green.

One primary time per benchmark drives warm-up qualification,
calibration growth, and the aggregate time statistics, and the CPU
selection is that primary time by default. The aggregate line names it:
`primary time/iter (ns) mean=`. A repetition whose primary fold carried
a gap disclosure stays out of those statistics, and
`ResultRow::timeAvailability` carries the disclosure.

Each time column is the folded delta of its own leaf divided by the
iteration count, so a scripted delta reports exactly the time it names. A
core metric keeps its own fold beside the two time columns. The aggregate
line is built from the selection the run asked for. A column that run did
not print has no place in it.

## The process CPU leaf

`BenchmarkHandle::measureProcessCpuTime()` moves the CPU column of one
family to the `machine/process_cpu` leaf and returns the handle, so the
call chains at the registration site:
`SG_BENCHMARK(bmProcessCpu).measureProcessCpuTime();`. The selection
stands for every instance of that family, and it combines with
`useRealTime` and with `useManualTime`, so one row can carry process CPU
time beside real time. The call carries the guard
the other setters carry: a handle that names no registered entry, or a
family whose run started, is an `SG_REQUIRE` precondition violation. A
provider that publishes no `machine/process_cpu` leaves that benchmark
failed with the resolution line the runner already prints for a missing
leaf. In `example/benchmark_example.cpp`, `bmProcessCpu` carries the
call.

The CPU column reads the process CPU leaf and the real-time column the
monotonic leaf; both may be asked for at once. Where the process CPU leaf
cannot be resolved, the benchmark is reported as failed with a line naming
the resolution that was sought.

## Real time as the primary time

`BenchmarkHandle::useRealTime()` makes real time the primary time of
one family and returns the handle. From that call on, the qualification
test, the growth rule, and the aggregate time statistics read the real
column, and the CPU column keeps the CPU fold it carries. The selection
excludes `useManualTime`: the second of the two calls is the
`SG_REQUIRE` precondition violation, in either order. `bmRealTime` in
`example/benchmark_example.cpp` carries the call.

The time a run treats as primary decides where its growth rule stops, so one
scripted sequence reaches a different iteration count under each selection.
The aggregate line is read against the column that selection prints, which
keeps the primary time the time the report shows.

## Manual time

`BenchmarkHandle::useManualTime()` selects manual time for one family,
and `State::setIterationTime(std::int64_t nanoseconds)` hands the state
one iteration time per iteration. The primary time of a run becomes the
sum of the accepted values: the harness records it by summation alone
and reads no clock for it, so the time-source gate stays clean. The
real column shows that sum divided by the iteration count, and the CPU
column keeps its own fold. The fivefold real-time stop leaves a manual
run alone, so such a run stops at its target or at the iteration cap.
A manual benchmark that accepts nothing reports 0.000 ns in the real
column and qualifies at the cap. `setIterationTime` in a callback state
is a precondition violation, because that state owns no run to
accumulate for. `bmManual` in `example/benchmark_example.cpp` pairs the
selection with one `setIterationTime` call per iteration.

A manual time is accepted against the growth rule of the run, and a sum that
reaches the cap with nothing accepted in it is reported. The reading is the
same whether the manual report arrives before or after the timed loop, so
the order of the two carries no meaning.

## Pause and resume

`State::pauseTiming()` stops the timed window from that point and
`State::resumeTiming()` restarts it, and each call takes one sample of
every leaf of the run plan. The reported time of a run is the window
fold minus the fold of each closed interval:

```text
reported = fold(base, base + samples - 1)
           - the fold of each closed interval
```

The subtraction reaches every time column and every attached metric
fold, so a paused stretch leaves the metric columns too.
`ScopedPauseTiming(State&)` is the scoped form: the constructor pauses,
the destructor resumes, and the destructor skips the resume when the
run was skipped.

The calls stand inside the timed loop, where the run's sampling window is
open, and the loop keeps its shape. The preconditions bound one open
pause per run. `pauseTiming` needs a run state whose sampling window
stands open and no pause already open, and `resumeTiming` needs one open
pause. A run carries no fixed allowance of closed intervals: a loop may
close a pair in every iteration. A pause still open at loop exit is an
`SG_ASSERT` breach in `closeWindow`, and the skip and exception path
closes an open pair in `topUpWindow` before the exit sample, so every
fold reads a complete pair. A pause in a callback state is a precondition
violation, the callback-state rule extended to the pause calls.

Each call costs one sampling action, and the row discloses the cost:
`endpoint samples=<S>` prints the count that run took, so a run with
one paused interval prints `endpoint samples=4`. The room for those
points follows the loop, and no fixed allowance bounds it. Before an
attempt starts, the recorder reserves the two points of the window plus,
from a family's second attempt, two points for each closed pair its
previous attempt observed per iteration, capped at 32768 raw points per
leaf column. An attempt over the cap reserves no pause room: each of its
pairs closes through a running fold at the commit, the columns of the row
read the same values as the recorded points produce, and the row ends
with `pause=folded` so the fold is disclosed. A first attempt of a family
has no history to size the room by, so its pairs fold and its row carries
the token too.
`bmPaused` in `example/benchmark_example.cpp` holds a
`ScopedPauseTiming` region inside the loop, and a dry run of
`build/example/benchmark_example` prints `endpoint samples=4` for it.

A pause removes the time it names: every time column and every attached
metric of the run is the window fold minus the pause fold, to the
nanosecond. The guard reads the same interval as the explicit calls around
it. A loop that pauses in every iteration still completes. A run that pauses
nothing reserves no pause room, and a pause that is opened twice, closed
twice or closed without an opening breaks the precondition at that call.

## Time units

`TimeUnit` names the scale and the label of a printed time value:
`NANOSECONDS`, `MICROSECONDS`, `MILLISECONDS`, and `SECONDS`.
`BenchmarkHandle::unit(TimeUnit timeUnit)` sets the unit of one family
and returns the handle:
`SG_BENCHMARK(bmMillis).unit(sg::TimeUnit::MILLISECONDS);`.
`sg::setDefaultTimeUnit(TimeUnit)` sets the process default that a
benchmark calling no `unit` resolves to when its run starts, and
`sg::getDefaultTimeUnit()` reads it back. The default starts at
`TimeUnit::NANOSECONDS`.

A stored value stays nanoseconds, so the C++ value keeps one unit, and
the report applies the divisor at print time: `ns` 1 ns, `us` 1000 ns,
`ms` 1000000 ns, `s` 1000000000 ns. The scaling reaches both time
columns, the overhead floor, and the aggregate line, so one line
carries one scale, and the metric columns keep their form. The unit
resolves when a run starts: the `unit` call of the family wins, then the
default the entry point snapshots at its own start, then the process
default at the moment the run starts. A
`setDefaultTimeUnit` call in `main` before the entry point therefore
reaches a benchmark the macros registered during static initialization,
and a `unit` call overrides all three.
`bmMillis` in `example/benchmark_example.cpp` sets the millisecond unit,
and a dry run of `build/example/benchmark_example` prints its row under
the `ms` label. A command-line unit option belongs to a future
command-line specification.

One unit choice scales every field of a line the same way: both time
columns, the overhead floor and the aggregate line. A run that never calls
the setter keeps the default unit, and the scaling holds for a runner driven
without the entry point, and through it.

## Longer forms of the same calls

The main page quotes the shortest form that shows each call. These are the
same calls in the surrounding code a reader writes, with the families that
carry more than one argument and the helper a custom statistic needs.

<!-- sg-example: example/docs/registration.cpp#registration-table -->
```cpp
auto bmTable(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

const auto kTable = sg::registerBenchmark(&bmTable, "Table/cells");
```

<!-- sg-example: example/docs/user_counters.cpp#user-counters -->
```cpp
auto bmCopyCells(sg::State& state) -> void
{
  const auto cells = static_cast<std::size_t>(state.range(0));
  std::vector<std::uint64_t> sink(cells, 0);
  for (auto _ : state) {
    for (std::size_t cell = 0; cell < cells; ++cell) {
      sink[cell] = cell + 1;
    }
    state.setCounter("cells", {static_cast<double>(cells)});
    sg::doNotOptimize(sink);
  }
  // The byte setter takes the total work of the whole run (FR-009).
  const auto bytes = static_cast<std::int64_t>(state.iterations())
      * static_cast<std::int64_t>(cells) * 16;
  state.setBytesProcessed(bytes);
}

SG_BENCHMARK(bmCopyCells).argsProduct({{64, 1024}}).repetitions(3);
```

<!-- sg-example: example/docs/throughput.cpp#throughput -->
```cpp
auto bmEncode(sg::State& state) -> void
{
  const auto items = static_cast<std::size_t>(state.range(0));
  std::vector<std::uint8_t> buffer(items);
  for (auto _ : state) {
    for (std::size_t item = 0; item < items; ++item) {
      buffer[item] = static_cast<std::uint8_t>(item * 7);
    }
    sg::doNotOptimize(buffer);
  }
  const auto total = static_cast<std::int64_t>(state.iterations())
      * static_cast<std::int64_t>(items);
  state.setItemsProcessed(total);
  state.setBytesProcessed(total);
}

SG_BENCHMARK(bmEncode).argNames({"items"}).argsProduct({{1024, 65536}});
```

The fixture macros and the two other registration spellings need their own
method each, so they sit here beside the class they extend.

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

## Reading a report row

A run prints a version line, then one row per instance and repetition, and a
final `AGGREGATE` row when the benchmark asks for repetitions.

<!-- sg-example: example/docs/reading_the_report.cpp#reading-the-report -->
```cpp
auto spreadOf(const std::vector<double>& times, double& spread) -> bool
{
  if (times.empty()) {
    return false;
  }
  double lo = times.front();
  double hi = lo;
  for (const double time : times) {
    lo = std::min(lo, time);
    hi = std::max(hi, time);
  }
  spread = lo > 0.0 ? (hi - lo) / lo * 100.0 : 0.0;
  return true;
}

auto bmRows(sg::State& state) -> void
{
  const auto cells = state.range(0);
  for (auto _ : state) {
    for (auto cell = std::int64_t {0}; cell < cells; ++cell) {
      sg::doNotOptimize(cell);
    }
  }
  state.setBytesProcessed(cells * 8);
  state.setLabel("filled");
}

SG_BENCHMARK(bmRows)
    .argsProduct({{8, 16, 32}})
    .repetitions(3)
    .complexity(sg::BigO::AUTOMATIC)
    .addStatistic("spread", sg::StatUnit::PERCENTAGE, &spreadOf);
```

The registration asks for a counter column, a label, one custom statistic and
the complexity fit, so the rows carry those fields. `bytes_per_second=` is the
byte rate the byte setter reports, `label=` holds the text `setLabel` gave the
instance, `spread=` is the custom statistic, and the fit prints a `BIG_O`
estimate with an `RMS` residual for the primary time and for each counter.

<!-- sg-sample-output: example/docs/reading_the_report.expected#report -->
```text
speedgun-ng 0.9.0 build=Release
bmRows/8                 iterations=   6044798  real time/iter (ns)=    2.242  cpu time/iter (ns)=    2.236  overhead floor (ns)=  681.000  endpoint samples=2  bytes_per_second= 4734.871  label=filled
bmRows/8                 iterations=   6044798  real time/iter (ns)=    2.250  cpu time/iter (ns)=    2.243  overhead floor (ns)=  681.000  endpoint samples=2  bytes_per_second= 4720.075  label=filled
bmRows/8                 iterations=   6044798  real time/iter (ns)=    2.250  cpu time/iter (ns)=    2.247  overhead floor (ns)=  681.000  endpoint samples=2  bytes_per_second= 4712.806  label=filled
bmRows/8                  AGGREGATE  primary time/iter (ns) mean=    2.242 median=    2.243 sd=    0.005 cv=  0.0024 min=    2.236 max=    2.247 n=3  spread=    0.468  label=filled
bmRows/8                             bytes_per_second mean= 4722.584 median= 4720.075 sd=   11.245 cv=  0.0024 min= 4712.806 max= 4734.871 n=3
bmRows/16                iterations=   3256469  real time/iter (ns)=    4.136  cpu time/iter (ns)=    4.130  overhead floor (ns)=  684.000  endpoint samples=2  bytes_per_second= 9516.840  label=filled
bmRows/16                iterations=   3256469  real time/iter (ns)=    4.071  cpu time/iter (ns)=    4.066  overhead floor (ns)=  684.000  endpoint samples=2  bytes_per_second= 9666.322  label=filled
bmRows/16                iterations=   3256469  real time/iter (ns)=    4.091  cpu time/iter (ns)=    4.085  overhead floor (ns)=  684.000  endpoint samples=2  bytes_per_second= 9621.425  label=filled
bmRows/16                 AGGREGATE  primary time/iter (ns) mean=    4.094 median=    4.085 sd=    0.033 cv=  0.0080 min=    4.066 max=    4.130 n=3  spread=    1.571  label=filled
bmRows/16                            bytes_per_second mean= 9601.529 median= 9621.425 sd=   76.701 cv=  0.0080 min= 9516.840 max= 9666.322 n=3
bmRows/32                iterations=   1836085  real time/iter (ns)=    7.716  cpu time/iter (ns)=    7.644  overhead floor (ns)=  680.000  endpoint samples=2  bytes_per_second=18238.993  label=filled
bmRows/32                iterations=   1836085  real time/iter (ns)=    7.589  cpu time/iter (ns)=    7.581  overhead floor (ns)=  680.000  endpoint samples=2  bytes_per_second=18391.439  label=filled
bmRows/32                iterations=   1836085  real time/iter (ns)=    7.620  cpu time/iter (ns)=    7.612  overhead floor (ns)=  680.000  endpoint samples=2  bytes_per_second=18317.167  label=filled
bmRows/32                 AGGREGATE  primary time/iter (ns) mean=    7.612 median=    7.612 sd=    0.032 cv=  0.0042 min=    7.581 max=    7.644 n=3  spread=    0.836  label=filled
bmRows/32                            bytes_per_second mean=18315.866 median=18317.167 sd=   76.231 cv=  0.0042 min=18238.993 max=18391.439 n=3
bmRows                        BIG_O  time/iter N (    0.243)
bmRows                          RMS  time/iter     4.932%
bmRows                        BIG_O  bytes_per_second N (  578.507)
bmRows                          RMS  bytes_per_second     2.167%
bmFill/8                 iterations=  31127816  real time/iter (ns)=    0.445  cpu time/iter (ns)=    0.445  overhead floor (ns)=  683.000  endpoint samples=2  bytes_per_second= 4622.274  label=filled
bmFill/8/threads:2       iterations=  62255632  real time/iter (ns)=    0.447  cpu time/iter (ns)=    0.894  overhead floor (ns)=  683.000  endpoint samples=4  bytes_per_second= 4618.207  label=filled
  contexts=2  ctx0=SPIN/NO_PIN  ctx1=SPIN/NO_PIN
```

A threaded row ends the run above. Its name carries the `/threads:2` segment,
and the line under it names the wait mode and the placement of each context the
row merged.

## The command line of an example

One entry point reads the command line, and any example binary takes the same
options. `--list` prints the registered instances, `--filter` selects them by
name, `--dry-run` reports one iteration per instance against the counter you
name, and `--catalog` lists the metrics the host offers.

<!-- sg-example: example/docs/command_line.sh#options -->
```sh
./build/example/docs_command_line --list
./build/example/docs_command_line --filter='bmWrite' --min-time=0.2
./build/example/docs_command_line --counter=machine/monotonic --dry-run
./build/example/docs_command_line --catalog
printf '%s\n' 'filter=bmWrite' 'min-time=0.2' > "$dir"/run.options
./build/example/docs_command_line --options="$dir"/run.options
SPEEDGUN_FILTER=bmWrite SPEEDGUN_MIN_TIME=0.2 ./build/example/docs_command_line
./build/example/docs_command_line --threads=1,4
./build/example/docs_command_line --threads=2:16
./build/example/docs_command_line --interleave=7
printf '%s\n' 'time@median:lower:5%' > "$dir"/rules.txt
./build/example/docs_command_line \
  --compare=test/data/gate_ok_base.json,test/data/gate_ok_contender.json \
  --compare-rules="$dir"/rules.txt
```

`--metric` names one metric the host's table defines, in the shape
`--metric=cpi`, and the column it adds holds the metric the library composes:
one column per name, in the order the names are given. A name the table
carries nothing for stops the run before any measurement with status 2, on one
line naming the near-miss names; a metric the host carries but cannot count
prints its refusal kind in place of the number, and the run continues. The
`--catalog` listing marks a metric line with `mode=metric` and prints it under
the object its first leaf reaches, after that object's counter lines.

`--counter` names one catalog counter address, in the shape
`--counter=machine/monotonic`, and adds one column per address in the order
the addresses are given. An address this host does not find, a typo or a valid
path whose device the host lacks, stops the run before any measurement with
status 2, on one line naming the source that held the address, the address,
and what the host found there. The listing and the catalog answer before the
addresses are read, so `--counter=/no/such/leaf --list` still prints them. A
caller that hands addresses to the runner keeps the absent column the runner
answers for a leaf it cannot resolve.

The aggregate row of the quick-start benchmark carries the built-in statistics
for one benchmark: mean, median, `sd=`, `cv=`, min, max and the repetition
count.

<!-- sg-example: example/first_benchmark.expected#aggregate -->
```text
bmSum                     AGGREGATE  primary time/iter (ns) mean=  649.108 median=  649.499 sd=    0.942 cv=  0.0015 min=  648.034 max=  649.792 n=3
```

`speedgunMain` accepts the long options below, in the order its option table
carries them. Only the command line sets an option that names no environment
variable.

| Option | Argument | Environment | Help text |
| --- | --- | --- | --- |
| `filter` | required | `SPEEDGUN_FILTER` | Selects the instances whose names match |
| `list` | none |  | Prints the registry and stops |
| `repetitions` | required | `SPEEDGUN_REPETITIONS` | Sets how many times one instance runs |
| `min-time` | required | `SPEEDGUN_MIN_TIME` | Sets the least measuring seconds of one instance |
| `iterations` | required | `SPEEDGUN_ITERATIONS` | Fixes the iteration count and skips calibration |
| `warmup-time` | required | `SPEEDGUN_WARMUP_TIME` | Sets the untimed warm-up seconds |
| `dry-run` | none |  | Runs one iteration and prints the row shape |
| `counter` | required | `SPEEDGUN_COUNTER` | Adds one catalog counter to the report columns |
| `metric` | required | `SPEEDGUN_METRIC` | Adds one metric of the host's table to the report columns |
| `catalog` | none |  | Prints the counter catalog and stops |
| `percentiles` | required | `SPEEDGUN_PERCENTILES` | Sets the percentile points one run reports |
| `no-percentiles` | none | `SPEEDGUN_NO_PERCENTILES` | Turns the percentile lines off |
| `placement` | required | `SPEEDGUN_PLACEMENT` | Selects the preset that plans the run's cpus |
| `topology` | none |  | Prints the machine layout listing and stops |
| `json` | required |  | Writes one JSON report document for the run |
| `json-raw` | required |  | Selects the chunk columns or the kept gaps the document carries |
| `compare` | required |  | Compares two report documents; the mode runs no benchmark |
| `compare-rule` | required |  | Adds one comparison rule, repeatable in order |
| `compare-strict` | none |  | Makes every noisy row fail the verdict |
| `metric-method` | required |  | Sets the metric collection method to auto, passes or multiplex |
| `options` | required |  | Reads one option per line from a file |
| `env` | required |  | Takes off so the process environment sets nothing |
| `threads` | required | `SPEEDGUN_THREADS` | Sets the thread counts of every family |
| `interleave` | optional | `SPEEDGUN_INTERLEAVE` | Runs the selected instances in a seeded random order |
| `compare-rules` | required |  | Reads one comparison rule per line from a file |
| `help` | none |  | Prints one line per option and stops |
| `version` | none |  | Prints the project version and stops |
| `jitter-probe` | required |  | Runs the jitter probe over the batch by that value |
| `hunt` | optional |  | Hunts the selected catalog events and ranks them |
| `hunt-norm` | required |  | Sets the hunt score normaliser: instr, cycle or iter |
| `hunt-rank` | required |  | Sets the hunt ranking: rate or cv |
| `hunt-top` | required |  | Sets how many ranked events the hunt prints |
| `hunt-noise` | required |  | Sets the hunt noise threshold as a percent CV |
| `hunt-repeats` | required |  | Sets how many repeats one hunt event takes |
| `hunt-budget` | required |  | Sets the hunt budget in seconds or passes |
| `hunt-seed` | required |  | Sets the seed the hunt repeats draw from |

The highest source that sets an option wins: the command line, then the option
file, then the environment, then the built-in default. A repeatable option from
a higher source, the comparison rules included, replaces the whole list of a
lower one.

## User counters

`State::setCounter(name, counter)` reports a named number for the row the
loop is running. The `Counter` it takes carries a `double` value, a
`CounterFlag` set and a `CounterBase`. `CounterFlag::RATE` divides the
value by the primary time of the run in seconds,
`CounterFlag::ITERATION_INVARIANT` multiplies it by the iteration count,
`CounterFlag::AVG_ITERATIONS` divides by that count,
`CounterFlag::AVG_THREADS` divides by the thread count of the run, and
`CounterFlag::INVERT` takes the reciprocal last, after every other flag,
of a value that differs from zero: the reciprocal of zero is reported as
zero, because a rate of nothing has no reciprocal to print. Flags
combine through `operator|` on the set, so `RATE | AVG_THREADS` reads as
the average per-thread rate; a combined set is how this library spells
the single combined flags a Google Benchmark port names
`kAvgThreadsRate`, `kIsIterationInvariantRate` and `kAvgIterationsRate`.
Each divisor applies on its own, so a run whose primary time is zero
skips the rate divisor alone and the iteration flags still apply.
`CounterBase::K1000` and `CounterBase::K1024` choose the suffix, `k` or
`Ki`, that a human-readable rendering of the column carries. The
console prints raw numbers, so the base never divides the value: a
counter that holds 64 prints 64 under either base, and
`CounterFlag::NONE` prints the value as it stands.

One benchmark carries at most `kMaxUserCounters` names, sixteen of them,
and a name holds at most `kMaxCounterNameBytes` minus one bytes, so 31
characters. Both bounds are `SG_REQUIRE` preconditions of `setCounter`:
a seventeenth name, or a name of 32 bytes, ends the run with the bound
named in the message. A name a run opens keeps the flags and base of that
call, and a later call for the same name carrying a different pair
breaches the same precondition.

The counter columns print after the two time columns, in the order the run
opened the names, and the aggregate row carries one statistic line for
each counter beside the built-in ones. `example/docs/user_counters.cpp`
runs one family with a counter and a byte rate, and its aggregate lines
print under the built-in ones.

Every bound on a counter name and on the number of counters is a
precondition of the call that breaches it, so a breach ends the run with the
bound in the message. A name keeps the flags and the base of the call that
opened it, the column carries the value the body set after the flags are
applied, and the aggregate line carries one statistic for each counter.

## Throughput

`State::setBytesProcessed(bytes)` and `State::setItemsProcessed(items)`
report the work one run did and open a rate column for it. Each call takes
the total work of the whole run, so a body that works a known amount per
iteration passes that amount times `state.iterations()` after the loop.
Each call sets
a counter of its own name carrying `CounterFlag::RATE`, so the value
divides by the primary time of the run in seconds. The byte column opens
with `CounterBase::K1024` and the item column with `CounterBase::K1000`:
the base carries the suffix a human-readable rendering would show, and
the console column prints the rate as a raw number. Both are counters of
the row like any other: they print in the same fields and carry the same
aggregate line.

Each rate column is the total the body reported divided by the primary time
of the run, and a run that reports no work prints neither column.

## Labels

`State::setLabel(label)` attaches a word to the row the loop is running.
A repetition keeps one label, the last call of that repetition wins, and
the report prints it as the trailing field, `label=<text>`, after the
counter columns. A label holding a newline would break the one line per
row shape of the report, so the newline is a `SG_REQUIRE` precondition of
the call. The aggregate row carries the label every repetition ended with,
and prints no label where the repetitions ended apart.

The label is a trailing field on every row of the family and on its
aggregate. The label the last repetition set is the one the row prints. A
label carrying a newline breaks the precondition of the call that sets it.

## Custom statistics

`BenchmarkHandle::addStatistic(name, unit, function)` adds one named
number to the aggregate row of the family. The `StatisticFunc` takes the
primary time per iteration of the clean repetitions, the same list the
built-in statistics summarize, and writes its result through a `double`
reference; a callback that returns false leaves its value out of the row.
`StatUnit::TIME` prints the value in the unit of the row,
`StatUnit::PERCENTAGE` prints it unscaled by the unit. The value prints
among the built-in fields in the order the calls came in. The names
`mean`, `median`, `sd`, `cv`, `min`, `max` and `n` belong to the
built-in statistics, and a call naming one of them breaches the
precondition of `addStatistic`.

A custom statistic appears on the aggregate row, one value per callback, in
the unit the callback names. A callback that returns false leaves its value
out of the row. A name a built-in field already uses breaks the precondition
of the call that registers it.

## Aggregate-only reporting

`BenchmarkHandle::reportAggregatesOnly()` restricts the report of one
family to its aggregate rows: the repetition rows stay measured and stored
and never print. `BenchmarkHandle::displayAggregatesOnly()` trims the
console display of one family the same way while the stored result keeps
every repetition row, the difference a second output channel reads back. A
family with no aggregate row keeps its rows, so neither mode empties a run
of output.

Each aggregates-only mode changes what the console prints for the same
family, and the rows it stores match the rows it displays. A family with
nothing to aggregate keeps its row.

## Complexity fit

`BenchmarkHandle::complexity(BigO)` fits the family against the problem
size of its instances. `BigO::ONE`, `BigO::N`, `BigO::N_SQUARED`,
`BigO::N_CUBED`, `BigO::LOG_N` and `BigO::N_LOG_N` each try one curve,
and `BigO::AUTOMATIC` tries them and keeps the closest.
`complexity(BigOFun)` takes a curve of your own: a `double(double)` of
the size. `State::setComplexityN(problemSize)` sets the size a run
reports, which the fit reads in place of the argument the instance
carries.

The fit runs over the instances of the family and reports one pair of rows
per fitted quantity: the primary time as `time/iter`, then each named
user counter and each attached metric. The first row of the pair reads
`BIG_O` with the curve and its coefficient, the second reads `RMS` with
the fit error as a percentage. A quantity a member never set gets a
`SKIPPED` row naming the quantity and the reason, and a family with
fewer than two instances, or an instance without a positive size, states
the reason in place of the rows.

Each curve is fitted by the least-squares closed form
`k = sum(t * f(N)) / sum(f(N)^2)` over the series of the family, where `t`
is the value of one instance and `f(N)` the shape of the curve at its
size. The error row is the root-mean residual `t - k * f(N)` over the
mean of the values, as a percentage. `AUTOMATIC` fits every curve and
keeps the one with the least error, with the constant curve standing
when none earns less. A family with too few points to fit states the
reason and prints no coefficient.

## Execution context lifecycle

An `ExecutionContext` is one thread the library runs, the inbox that thread
reads, and the policy it was created under. A constructor starts that thread:
the default one under `BLOCK` with no pin, the other under the policy you pass.
`policy()` reports that policy for the life of the context.

Three calls close a context, and they close it in this order. `requestStop()`
stands the one stop request that every `StopToken` of the context reads, and it
returns without waiting. `drain()` waits until the inbox stands empty and the
loop stands idle. `join()` ends the loop and joins the thread, and no work item
of the context runs after that call returns. The destructor asks for a context
that `join()` has returned for.

The enqueue of a `dispatch()` and the stop request race, and the enqueue
settles it. A dispatch that finds the inbox open appends its node, and the loop
runs that node during `drain()`. A dispatch that finds the stop request standing
refuses its node and completes it `STOPPED` in that call itself. The loop never
takes such a node, so its item never runs. The three calls in order
run each accepted node once.

`drain()` with no stop request standing is a flush. It waits the same way, the
accepted nodes have run when it returns, and the loop keeps running, so a later
`dispatch()` runs its node. Work that runs until the stop request stands reads
`stopRequested()` on the token of its `WorkContext` and returns once that token
reads true.

Two calls share one precondition: the calling thread is not the thread of that
context's loop. `drain()` from that thread would wait on the work that thread
must finish, and `join()` from it would wait on itself.

The caller owns its node. `WorkNode` lives in caller storage, and the caller
keeps it alive until `state()` reads `DONE` or `STOPPED`. A refused node reaches
`STOPPED` in the dispatch that refused it, and the loop completes a node it
accepted, so one of the two settles the state.

Work runs on the thread of the context it was dispatched to. The dispatches of
one thread run in the order that thread made them, and the inbox decides
between two threads. Work dispatched from inside a running item runs after that
item returns, on the same context.

Three calls close a context, the destructor asks for the joined state, and each
accepted node runs once. A node a stopped context refused reads `STOPPED`
without running. A context you start yourself outlives the benchmark run,
because `speedgunMain` returns while its work runs, and these three calls
end it.

## What a threaded row merges

An instance run on N contexts runs its body on each context and publishes one
merged row for the N runs. The table states the rule behind each summed field
of that row.

| Row field | The rule for a merged row |
| --- | --- |
| Instance name | The family name, the argument suffixes, then `/threads:N` |
| `iterations=` | The summed per-context counts |
| `real time/iter (ns)=` | The summed real time, or the summed manual time under manual timing |
| `cpu time/iter (ns)=` | The summed cpu time |
| A counter column | The counter values of the contexts, summed |
| A counter read with `AVG_THREADS` | That sum divided by the context count |
| A counter read with `RATE` | That sum divided by the summed decision seconds, divided again by the context count |

The flag pass reads the real context count, so the two dividing flags scale by
the number of contexts that ran. The decision seconds the `RATE` divisor reads
are the cpu sum by default, the manual sum under manual timing, and the real sum
under real-time timing. A merged row prints its times per iteration, the way a
single-context row prints its own.

`BenchmarkResult::contexts` carries one `ContextDisclosure` per context, and the
report prints those disclosures under the merged row. The first line states
`contexts=N`, and one field per context states its wait mode and its placement.

A merged row sums the times, the iterations and the counters of its contexts.
The flag pass then reads the real context count, and the disclosure line under
the row names the policy of each context that contributed.

## Topology placement

`--placement` plans the cpus of every threaded run of the process from
the machine layout the harness reads once, at the first need, through
the vendored hwloc. Five presets name the plans:

| Preset | Places |
| --- | --- |
| `none` | no plan and no pin, the behavior of every run before this option |
| `core` | one thread per core, in core order |
| `l3` | one cpu per L3 group, the group's lowest cpu |
| `smt` | the sibling threads of one core, in core order |
| `compact` | every thread of each L3 group, in group order |

A handle states its family's preset with `placement("l3")`, and the
handle wins over the command line. The process option is parsed once,
before the first run.

The fit rule: a preset has as many places as its walk produces cpus.
A run that asks more contexts than the preset has places stops the
batch before the first run with one line naming the preset, the place
count and the requested count, and the process exits 2. A layout the
harness cannot read stops a preset run the same way, whatever the
context count.

The report of a run with a preset carries a topology line below the
version line, `topology: packages=1 l3-groups=4 cores=24 threads=48`,
and each context of a threaded run discloses its planned pair,
`ctx0=SPIN/0@0/applied`: the cpu it ran on, `@`, and the ordinal of
its L3 group. A pair longer than the field meets the standing cut
mark.

`--topology` prints the layout the plans come from and exits: the
counts line above, the process mask the layout was built from, and
one line per L3 group naming that group's cpus inside the mask in the
range spelling `0-5,24-29`. An unreadable layout prints one line and
exits 2.

## Percentiles

A family that reports percentiles divides one run into chunks of iterations,
times each chunk, and reads percentile points off the chunk times.
`showPercentiles(true)` turns the lines on for one family. `--percentiles=50,90`
turns them on for every family of the run and sets the points; `--no-percentiles`
turns them off. The default points are p50, p90, p99 and p99.9.

The run takes one sample at the end of every chunk and one at the end of the
run. The `chunks` line under the row discloses the plan: `chunks=` states the
count of chunks and `iterations/chunk=` the size one chunk holds. One percentile
line prints for each reported column: the primary time, then the time column the
primary is not, then each metric. Each line states one value per point, in the
unit that column prints in.

<!-- sg-sample-output: example/docs/percentiles.expected#report -->
```text
speedgun-ng 0.11.0 build=Release
bmRoll                   iterations=       300  real time/iter (ns)=22376.753  cpu time/iter (ns)=22347.760  overhead floor (ns)=  110.000  endpoint samples=301
bmRoll                       chunks  chunks=300  iterations/chunk=1
bmRoll                   percentile  primary time/iter (ns)  p50=22010.000  p90=23240.000  p99=23730.000  p99.9=not available
bmRoll                   percentile  real time/iter (ns)  p50=22010.000  p90=23240.000  p99=24150.000  p99.9=not available
```

### The chunk rule

A chunk holds the least count of iterations whose time reaches 100 times the
cost of one sample action, which keeps a sample under 1% of the window it
bounds. The first attempt of a fixed-count run has no measured iteration time
to calibrate from, so its chunks hold one iteration each and the next attempt
of the instance calibrates from the time the first measured. A pause interval
inside a chunk nets out of that chunk's time and of no other chunk's.

### The rank rule

A point reads the nearest rank of the sorted chunk times: the value at position
`ceil(p * n / 100)`, counting from one, over the `n` chunk times the run holds.
The point needs a least count of chunks before it answers at all, and that count
is `ceil(100 / (100 - p))`.

| Point | Least chunk count that answers |
| --- | --- |
| p50 | 2 |
| p90 | 10 |
| p99 | 100 |
| p99.9 | 1000 |

### The merged and per-context rule

The merged row of a threaded run reads its points over the union of every
context's chunk times, sorted together. The disclosure line of each context
reads that context's own values alone, so a slow context stays visible in its
own line and inside the union.

### The not-available rule

A point whose least count the run does not reach prints `not available`. The
harness never interpolates between the values it holds. A column whose fold
disclosed a gap answers nothing for every point, because its values do not
cover the whole run.
## Hardware counter metrics

`addMetric` attaches a counter expression to a family, behind the availability
check the counter lookup returns.

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

The row carries the metric label with `ratio=`, `scaled=`, and `gap=` fields,
and on a host where the leaf is unavailable the row prints the refusal kind in
place of the number and the run continues. These walks are the snippets that
add a header to the pair: `speedgun-ng/counters.hpp` and
`speedgun-ng/counters_metrics.hpp`.

`metricByName` hands back a metric the host's table defines, and `compile`
turns its expression into a counting plan the window folds:

```cpp
#include <cstdio>
#include <string>
#include <string_view>

#include "speedgun-ng/counters.hpp"
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

The print carries the table's group and unit text beside the folded value.
`--metric` puts the same metric in a report column, one column per name, in
the order the names are given.

## JSON reports

`--json=path` writes one JSON document for the run beside the console report.
The console keeps the bytes it always printed; the file rides beside it.
`--json-raw=chunks` asks for the chunk columns of every measured row, which is
what lets a run whose percentile lines stay off still carry them.

<!-- sg-example: example/docs/json_reports.cpp#json-reports -->
```cpp
auto bmDice(sg::State& state) -> void
{
  std::uint64_t dice = 0;
  for (auto _ : state) {
    for (std::uint64_t i = 0; i < 3'000; ++i) {
      dice = (dice + i) * 1'099'511'628'211ULL;
    }
    sg::doNotOptimize(dice);
  }
}

SG_BENCHMARK(bmDice).iterations(120).showPercentiles(true);
```

<!-- sg-sample-output: example/docs/json_reports.expected#report -->
```text
speedgun-ng 0.14.0 build=Release
bmDice                   iterations=       120  real time/iter (ns)= 2188.825  cpu time/iter (ns)= 2188.833  overhead floor (ns)=   90.000  endpoint samples=121
bmDice                       chunks  chunks=120  iterations/chunk=1
bmDice                   percentile  primary time/iter (ns)  p50= 2190.000  p90= 2200.000  p99= 2220.000  p99.9=not available
bmDice                   percentile  real time/iter (ns)  p50= 2190.000  p90= 2190.000  p99= 2220.000  p99.9=not available
```

### The shape of the document

The root object carries `schemaVersion`, `speedgun` and `benchmarks`. The
`speedgun` object carries the library version and the build type of the build
that ran. `benchmarks` holds one entry per benchmark, in registry order, and
each entry holds its name, suite, case, outcome, reason, time unit, percentile
points, contexts, noise warning, rows, labels and statistics, beside the pass
disclosure `metricMethod`, `passCount`, `passSpread` and `spreadWarning`. One
row per repetition carries its iteration count, its two time columns, its metric
and counter columns, its statistics, its percentile cells, its fit and its raw
chunk columns. Each metric column of a row carries `pass`, the index of the pass
that measured the value. `docs/schema/report-v1.json` states the whole shape.

### The version rule

`schemaVersion` names the shape of the document. A document whose major stands
at 1 loads, whatever its minor says; a later major is refused, and the refusal
names the version the document carries. A run writes `1.1`, the shape that
added the pass fields; a `1.0` document, which carries none of them, loads
through the same rule.

### The null rule

A percentile the chunk count cannot answer stands as `null`, the file form of
the `not available` the console prints. `reason`, `label`, `noiseWarning`,
`counterDisclosure` and `fit` stand `null` where the run carries none, and a
statistic whose sample set stands empty answers `null` for every quantity.
`passSpread` and each metric column's `pass` stand `null` where fewer than two
passes measured the run.

### The raw column rule

`raw.columns` holds one array per reported quantity, in the order the percentile
lines read them: the primary time, the time column the primary is not, then each
metric. Each array holds `chunkCount` values in the order the run closed the
chunks. A percentile line reads its own sorted order over those values. The key
stays out of the row when the run captured nothing.

### The loader rule

`loadReport(path)` returns the run's own values, or an error naming the JSON
pointer of the field it could not read. The loader checks the fields it reads;
it does not validate the document against the schema file, which stands as the
gate's oracle.

### The no-date rule

No key of the document names a date, a time, a host or a process id. The file is
a function of the values the run measured, so two runs of one build write the
same bytes and a diff of two runs shows only what changed in the run.

## Regression gate

Two report documents go in and one verdict comes out. `--compare` names the
baseline document and the contender document, split at the first comma, and the
mode loads both through `loadReport`; it runs no benchmark and reads no clock.
`--compare-rule` adds one rule and repeats in the order given, and with no rule
the gate applies one: the median primary time at five percent relative.
`--compare-strict` makes a `NOISY` row fail the run while the word that row
prints stays `NOISY`. `--filter` scopes the pair the way it scopes a run.

```sh
./build/example/first_benchmark \
  --compare=test/data/gate_ok_base.json,test/data/gate_ok_contender.json
./build/example/first_benchmark \
  --compare=test/data/gate_fail_base.json,test/data/gate_fail_contender.json
```

The table prints one line per input naming its path, its speedgun version and
its schema version, then a header and one row per benchmark under one rule: the
baseline value, the contender value, the change in percent, the change in the
unit the baseline report declares, and the state word. The first run above
answers status 0, the second answers 1 with its row `REGRESSED`.

### The rule grammar

```text
<rule>      ::= <value> [ @ <aggregate> ] : <direction> : <threshold> [ : <filter> ]
<value>     ::= time | real_time | cpu_time | p <point>
              | counter : <label> | metric : <label> | stat : <label>
<aggregate> ::= median | mean | min | max
<direction> ::= lower | higher
<threshold> ::= <percent>% | abs = <number> | <percent>% , abs = <number>
<filter>    ::= any text, the rest of the rule text
```

The fields run in that order. `time` reads the primary column, `real_time` and
`cpu_time` read one column by name, `p<point>` reads one percentile cell, and
the three prefixed names read the column or the statistic their label names. A
label ends at the first unescaped colon, and `\:` , `\,` , `\@` and `\\` stand inside
it. The `@` suffix names one aggregate and defaults to `median` for a name the
report holds as a statistics record; a written suffix on a percentile cell or on
a statistic is a fault. The direction word is required. A threshold carries a
percent, an `abs=` number, or the two in that order, the number standing in the
stored unit of the value. The filter is a regular expression over the full
instance name, matched the way the batch filter option matches.

`parseCompareRule` is the public call behind that parse, and `compareReports`
takes the rules with two loaded reports and one `CompareOptions` value, so a
program can drive the comparison the options drive. The settings struct carries
`strict`, the same setting `--compare-strict` answers, and a later setting
joins it with the call's signature unchanged.

### The state words

An entry is one benchmark under one rule, and it carries one of seven words.

| Word | The row that prints it |
| --- | --- |
| `OK` | the change sits inside the rule's threshold |
| `REGRESSED` | the change moves against the direction past the threshold |
| `IMPROVED` | the change moves with the direction past the threshold |
| `NOISY` | the row cannot carry a verdict |
| `ADDED` | the name stands in the contender alone |
| `REMOVED` | the name stands in the baseline alone |
| `SKIPPED` | the entry holds no readable row |

`lower` means a smaller value is better, so the change a `lower` rule watches is
the positive one, and a rule with both thresholds written regresses a row only
when both are exceeded. A `NOISY` row prints `reason=` naming every condition
that fired: a noise warning the report carries, a coefficient of variation past
the relative threshold, a sample standard deviation past the absolute threshold,
or a metric running ratio below one. That row fails the run only under
`--compare-strict`. `ADDED`, `REMOVED` and `SKIPPED` print `n/a` in their value
cells and never fail the run.

### The fault lines

A fault answers status 2, prints one line, and prints no table.

```text
--compare: expected '<baseline>,<contender>', got 'test/data/gate_ok_base.json'
--compare-rule: the threshold of the rule 'time@median:lower:5' is not readable: '5' is neither a percent, nor the abs= marker, nor the two in that order with one comma between
error: the compare document 'test/data/gate_schema_20.json' was rejected: the file 'test/data/gate_schema_20.json' carries schema version '2.0', which this build cannot read: the reader knows version 1
error: the rule 'time@median:lower:5%:nothing-matches' selects no benchmark: its filter 'nothing-matches' matches neither report
error: the rule 'time@median:lower:5%' cannot measure bmGate: the baseline of time is zero
```

The scan faults name the option and the text it read. A document the reader
refuses prints the reader's own line naming the file and the reason, and a
document the file system refuses names the path. A pair that shares no name
after `--filter` prints one line and answers 0.

### The comparison document

`--json=` writes the comparison as one document while the mode runs, and the
write happens before anything prints; the same option writes a run's own report
document when the mode stands off. The root carries `schemaVersion`, `speedgun`,
`inputs`, `rules`, `entries` and `verdict`, and one entry object stands per row
with `name`, `rule`, `baseline`, `contender`, `changePercent`, `changeAbsolute`,
`state` and `reason`. `docs/schema/compare-v1.json` describes the shape. Values
stand in the stored unit with no printed scaling, and no key names a date, a
host or a process id.

The pairs under `test/data/` are committed report documents, so the runs quoted
above read the same values every time, and two runs of one command write the
same bytes:

```sh
./build/example/first_benchmark \
  --compare=test/data/gate_ok_base.json,test/data/gate_ok_contender.json \
  --json=compare-a.json
./build/example/first_benchmark \
  --compare=test/data/gate_ok_base.json,test/data/gate_ok_contender.json \
  --json=compare-b.json
cmp compare-a.json compare-b.json
```
## Multi-pass collection

A metric set that needs more PMU events than the processor counts at once is
a supported case. A pass is one collection round. It opens the events of the
columns assigned to it, runs every repetition, and closes with its own
readings. The harness splits the set into passes, runs the benchmark once per
pass, and merges the passes into the usual one row per instance. The split and
the merge sit outside every timed window.

Two published facts drive the split: `Object::programmableCounters` states
how many programmable counters one context can hold open on one event
source, and a loaded metric states how many events its column needs. The
planner reads both before the first timed window and opens no counter.

`--metric-method=auto`, `--metric-method=passes` and
`--metric-method=multiplex` choose the method, and `metricMethod` on the
handle carries the same three values. `auto` is the default. The command line
wins over the handle, and a repeated option keeps its last word. A word
outside the three prints one line and ends the run with status 2.

`example/docs/metric_passes.cpp` makes that request from the registration
side, on the handle the over-limit set was attached to:

<!-- sg-example: example/docs/metric_passes.cpp#metric-method -->
```cpp
#include "speedgun-ng/benchmark.hpp"

// The request stands for every instance of the family; a command line word
// outranks it (FR-025).
auto requestPasses(sg::BenchmarkHandle& handle) -> sg::BenchmarkHandle&
{
  return handle.metricMethod(sg::MetricMethod::PASSES);
}
```

A run that measured more than one pass prints one disclosure line under its
rows. A caller that forced a method sees the same line for one pass:

```text
<name>                      passes  method=passes  count=2  spread=11.1%
warning: pass spread 11.1% exceeds 5%
```

`method=` names the method the run settled to, `count=` the passes it
measured, and `spread=` the relative spread of the time per iteration across
those passes. A spread above 5% brings the warning line with it, and the
warning changes no value and no exit status. When more than one pass ran,
each metric column gains a `pass=<index>` token after its `gap=` field,
naming the pass that measured the value. A one-pass run of the default
method prints no line, and its report names `AUTO` with a count of zero, a
null spread and a null `pass` on every metric column.

### The split rule

The planner takes the PMU columns of the run in request order: the metric
columns in the order `addMetric` attached them, then the user-counter
columns in the order `--counter` opened them. Each column joins the first
pass that still holds all of its events within every source limit, and a
column with no such pass opens a pass of its own.

The events of one column stay in one pass, because a pair split across two
windows has no common interval to fold. A column wider than the limit takes
a pass of its own and runs oversubscribed: the kernel rotates its events over
the counters, and the `ratio=` field discloses the fraction of the enabled
time the value covers. The two machine clock leaves
of the spine (`machine/monotonic` and the CPU leaf the family selected) ride
every pass and count against no limit, because they consume no programmable
counter. The same columns, method and limits give the same passes on every
run.

One calibration runs before the first pass, and every pass takes the
iteration count it produced, so the passes measure the same amount of work.
Every pass runs the repetitions the family and the run control ask for, on
the same execution contexts as the other passes. In a threaded run every
context finishes one pass before any context opens the next.

### The merge rule

The merge publishes one result of the usual shape: one row per instance and
repetition, in the column order the run fixed before the split. Each metric
column and each user-counter column carries the values of the pass that
measured it. The time columns, the overhead floor, the endpoint samples, the
CPU-time column and the percentile columns of the primary time come from
pass 0, the reference pass. The percentile columns of a metric come from the
pass that measured that metric. A value whose measuring pass folded a gap
keeps the gap disclosure, and a refused metric keeps its refusal kind. The
merge moves the values the passes folded, and it publishes no value a pass
left unmeasured.

### The pass spread

The spread is the maximum minus the minimum, divided by the median of the
reference time per iteration of the passes, as a percentage. The primary
time column of each pass's reference row supplies that pass's figure. The
median of an even count is the lower of the two central values, and the
formula takes no average. One pass carries no spread. A median of zero
answers `spread=not available`. Pass times of 100 ns, 108 ns and 112 ns
through a median of 108 ns give a spread of 11.1%, and the warning line
follows.

The merged result carries the same three values as published fields,
`BenchmarkResult::metricMethod`, `passCount` and `passSpread`, so a consumer
reads what the line prints.

### A host that publishes no limit

An event source the counters library cannot answer for publishes no limit.
Such a host keeps the single oversize plan and the kernel rotation of it
under `auto` and under a forced `passes` alike, and the disclosure line names
`method=multiplex`. No spread appears, because one plan measured the run,
and every column keeps its `ratio=`, `scaled=` and `gap=` fields. A missing
limit stays a missing fact: the run completes and discloses the method it
used.

A run whose metric set is wider than the counters measures the set in rounds
and still reports one row per instance: each metric value comes from a pass
that counted it inside the limit, the times come from the first pass, and the
line under the row states the method, the pass count and the spread between
the passes. `--metric-method` pins one method for a scripted comparison of
the two. The split, the merge and the disclosure run in
`test/source/harness_pass_plan_test.cpp`,
`test/source/harness_pass_collect_test.cpp` and
`test/source/harness_pass_report_test.cpp`, on the fake provider with a set
counter limit.
## Jitter probe
The `--jitter-probe` option and the `runJitterProbe` call state the probe options of the run: a spinning context per touched L3 group counts the gaps the host inserts above a threshold while the batch runs, and the report prints one block of the folds after the benchmark tables.
<!-- sg-example: example/docs/jitter_probe.cpp#jitter-probe -->
```cpp
auto bmMeasured(sg::State& state) -> void
{
  for (auto _ : state) {
    sg::doNotOptimize(state.iterations());
  }
}

SG_BENCHMARK(bmMeasured).threads(2).placement("compact");

auto main(int argc, char** argv) -> int
{
  sg::runJitterProbe({.mode = sg::JitterProbeMode::PRECHECK});
  return sg::speedgunMain(argc, argv);
}
```

<!-- sg-sample-output: example/docs/jitter_probe.expected#report -->
```text
speedgun-ng 0.17.0 build=Release
topology: packages=1 l3-groups=4 cores=24 threads=48
bmMeasured/threads:2     iterations=  26039366  real time/iter (ns)=    0.529  cpu time/iter (ns)=    0.528  overhead floor (ns)=  219.000  endpoint samples=4
  contexts=2  ctx0=SPIN/0@0/applied  ctx1=SPIN/24@0/applied
jitter probe: cpus 0,24
gapCount: 54 65
gapsPerSecond: 272.44 327.94
minNs: 420 252
medianNs: 5670 4662
meanNs: 8494.89 5272.62
p90Ns: 10542 9534
p99Ns: 93072 27048
p999Ns: 93072 27048
p9999Ns: 93072 27048
maxNs: 93072 27048
totalNs: 458724 342720
totalPercent: 0.23 0.17
runtimeNs: 198208038 198206316
thresholdNs: 420 200
loopPeriodNs: 42 1
droppedGaps: 0 0
jitter: probe on cpus 0,24
jitter: loop period 42 ns, threshold 420 ns
jitter: cpu 0 kept 54 dropped 0
jitter: cpu 24 kept 65 dropped 0
jitter: split and percentiles use the kept gaps only
```

The value takes a head, a cpu list or the word `idle`, and the keys follow, each once:

| Part | Meaning | Default |
| --- | --- | --- |
| `<cpu-list\|idle>` | One or more cpu indexes or closed `a-b` ranges, comma separated, or the word `idle` | None: the head names the probe cpus; the word `idle` resolves one probe cpu per L3 group the plans touch, or the planned cpus in `precheck` mode |
| `threshold=<ns>` | The gap threshold in ns, at or above 1; a value stands as given | When the option names no threshold value: 200, or 10 times the measured loop period when that stands larger |
| `mode=` | `alongside`, `precheck`, or `both` | `alongside` |
| `precheck=<ms>` | The pre-check duration in ms, at or above 1 | 200 |
| `gaps=<n>` | The gap buffer size per probe cpu, 1 to 16,777,216 | 65,536 |
| `strict` | An unavailable time-stamp counter stops the run | Absent: the run discloses it and continues |

The block prints one field line per stats field, in this order, one value per probe cpu:

| Field | Unit | Rule |
| --- | --- | --- |
| `gapCount` | count | The kept gaps of the covered windows |
| `gapsPerSecond` | 1/s | `gapCount` over the seconds of the covered windows' span |
| `minNs` | ns | The smallest kept gap |
| `medianNs` | ns | The nearest-rank median over the sorted kept gaps |
| `meanNs` | ns | The mean of the kept gaps |
| `p90Ns` | ns | The nearest rank of the sorted kept gaps |
| `p99Ns` | ns | The nearest rank of the sorted kept gaps |
| `p999Ns` | ns | The nearest rank of the sorted kept gaps |
| `p9999Ns` | ns | The nearest rank of the sorted kept gaps |
| `maxNs` | ns | The largest kept gap |
| `totalNs` | ns | The sum of the kept gaps |
| `totalPercent` | percent | `totalNs` over the covered duration |
| `runtimeNs` | ns | The span from the loop start to the read of the stop flag |
| `thresholdNs` | ns | The effective threshold the run used |
| `loopPeriodNs` | ns | The minimum loop period the measurement pass measured |
| `droppedGaps` | count | The gaps a full buffer dropped; zero when the buffer never filled |

The block ends with the disclosure lines: the probe cpus, the loop period and the threshold the run used, one `kept` and `dropped` line per cpu, and the kept-gaps-only rule the split reads.

## Hunter mode

Hunter mode puts a wide catalog event set on one benchmark and ranks the events
by how far each one stands out of the run. `--hunt` turns it on and selects the
events, and seven `--hunt-*` options shape the measurement. The hunt drives the
pass loop described above, so it opens no second counter path. Its lines print
on stdout under the `hunt:` prefix, after the benchmark tables of the run. A
value outside the range its option states prints one line naming it on stderr
and ends the run with status 2.

A run with no `--hunt` prints no `hunt:` line and writes no `hunt` key. Its
console bytes and its document are the bytes they were before hunter mode
existed.

<!-- sg-sample-output: example/docs/hunt_rank.expected#report -->
```text
speedgun-ng 0.19.0 build=Release
bmHunt/8                 iterations= 262294045  real time/iter (ns)=    2.758  cpu time/iter (ns)=    2.753  overhead floor (ns)= 1144.000  endpoint samples=2
bmHunt/64                iterations=  32787145  real time/iter (ns)=   21.379  cpu time/iter (ns)=   21.350  overhead floor (ns)=  680.000  endpoint samples=2
hunt: rank 1 cpu/hunt_branches instr=2000.000 cv=0.00
hunt: rank 2 cpu/hunt_loads instr=1000.000 cv=0.00
hunt: rank - cpu/hunt_rtls instr=0.000 cv=0.00 mark=unrepeated
hunt: rank - cpu/hunt_stalls instr=3000.000 cv=33.33 mark=noisy
hunt: rank - cpu/hunt_stores instr=0.000 cv=0.00 mark=beyond-top
hunt: skipped cpu/hunt_blocked permission-blocked
hunt: skipped cpu/hunt_noenc not-encodable
```

### Selecting events

`--hunt` reads its value in the attached form: `--hunt=all`. A space between
the option and the word leaves `all` as a positional argument and selects
nothing.

| Form | What it selects |
| --- | --- |
| `--hunt` | every catalog entry of every core device |
| `--hunt=none` | nothing, and the run is the run with no `--hunt` |
| `--hunt=cpu/instructions_retired_any,cpu/l2_lines_out.any_core` | the listed addresses, in the order of the text |
| `--hunt=uncore/*` | one glob over the catalog addresses, reaching any device |
| `--hunt=@events.txt` | the addresses a file holds |

A glob matches `*` to any run of characters including none, `?` to one
character, and every other character to itself. The match is case-sensitive.
Addresses separate at a comma and at a newline, in the option value and in a
`@file` alike, and blank lines and `#` comments of a file are ignored. One
address that arrives twice, through a list and a file included, is one column.

A raw event code such as `r4a03` is refused wherever it stands: in a list, in a
pattern, or on a line of a file. The shape check runs before the catalog is
read, so one text gives the same refusal on a host that counts nothing. A
device that publishes fewer than three programmable counters cannot carry a
hunt pass beside its two anchors, and the run is refused before any pass is
planned.

### The eight options

| Option | Default | Accepted value |
| --- | --- | --- |
| `--hunt` | off | `all`, `none`, an address list, one glob, or `@path` |
| `--hunt-norm` | `instr` | `instr`, `cycle`, `iter` |
| `--hunt-rank` | `rate` | `rate`, `cv` |
| `--hunt-top` | `20` | 1 to 10000 ranked events |
| `--hunt-noise` | `10` | a percentage above 0 |
| `--hunt-repeats` | `3` | 1 to 1000 hunt repetitions |
| `--hunt-budget` | none | `<n>s` seconds or `<n>p` passes, `<n>` above 0 |
| `--hunt-seed` | `0` | any whole number a 64-bit unsigned holds |

`--hunt-norm` unset ranks by instructions. `--hunt-seed` at `0` keeps the
catalog order, and any other value shuffles the planned order the same way on
every host. `--hunt-repeats` sets the repeats of the hunt stages, and
`--repetitions` keeps its own meaning for the repetitions of the report rows. A
hunt option given twice keeps its last value, and a hunt option beside no
`--hunt` leaves the hunt off. Beside `--list`, `--catalog` or `--topology` the
hunt forms no plan, because those modes print and stop.

### The refusal lines

Each line below is the whole of what the run prints on stderr, with the quoted
text being the value the option carried. The run then ends with status 2.

```text
--hunt: 'cpu/nope' is not a catalog address
--hunt: pattern 'no/such*' matches no catalog address
--hunt: '@missing.txt' cannot be opened
--hunt: '@blank.txt' holds no catalog address
--hunt: 'r4a03' is a raw event code; --hunt takes catalog addresses
--hunt-norm: expected instr, cycle or iter, got 'ipc'
--hunt-rank: expected rate or cv, got 'sensitivity'
--hunt-top: expected 1 to 10000, got '0'
--hunt-noise: expected a percentage above 0, got '0'
--hunt-repeats: expected 1 to 1000, got '1001'
--hunt-budget: expected <n>s or <n>p, got '120'
--hunt-budget: expected a count above 0, got '0p'
--hunt-seed: expected a whole number, got 'abc'
hunt: device 'cpu' publishes 2 slots and a hunt pass needs 3
--hunt cannot combine with --counter
--hunt cannot combine with --metric
--hunt cannot combine with --compare
```

### The hunt lines

The plan line prints once above the first result row, the cut line directly
under it, and the lines that rank print after the benchmark tables.

```text
hunt: plan events=42 passes=14 method=PASSES estimate=1260000ns budget=120s
hunt: rank 1 cpu/instructions_retired_any instr=1042.300 cv=1.20
hunt: rank 2 cpu/l2_lines_out.any_core instr=6.910 cv=2.05 multiplexed
hunt: rank - cpu/mem_load_stalled_on_l2 instr=2.110 cv=24.80 mark=noisy
hunt: rank - cpu/page_walker_walk_completed instr=0.310 cv=0.00 mark=unrepeated
hunt: skipped cpu/l1_dc_misses permission-blocked stage=2
hunt: skipped armv8_pmuv3/cycle not-encodable stage=2
hunt: plan summary events=40/42 passes=14 spend=1184000ns budget=120s
```

`events=` counts the planned columns and `passes=` the passes of the plan.
`method=` names the settled collection method, and `estimate=` carries the
plan's own figure in nanoseconds. That figure is calibration data, so a run
that reaches the plan before any calibration answers prints
`estimate=not available`. The rank lines carry the rate under the label of the
norm in force, `instr=`, `cycle=` or `iter=`. A skipped line names the kind of
the refusal in the words the `--catalog` listing prints, and adds `stage=` when
a stage lost the event. The summary denominator counts the planned events plus
the refusals that kept an event out of the plan, and its numerator counts the
events that produced at least one usable sample.

A host that refuses every event prints the plan line, the skipped list and the
summary with `events=0/N`, and the run ends with status 0. A group open that
refuses mid-run retries its events one at a time; an event that still refuses
joins the skipped list, and the run still ends with status 0. Beside
`--dry-run` every pass runs one iteration, so each column reaches the table
with one sample, a `cv` of `0.00` and `mark=unrepeated`.

### The norm and the rank

`--hunt-norm=instr` divides an event count by the instructions of the same
window, `cycle` by the cpu-cycles of the same window, and `iter` by the
iteration count of the attempt. A rate reads per thousand of its denominator,
and per iteration under `iter`. A window that disclosed a gap contributes no
rate, and so does a window whose denominator read zero.

Each anchor rides every pass of its own device, so a device that lacks one of
the two anchors leaves the whole plan to measure under `iter`. The plan line
then carries `normalized=iter`, and the rate column reads `iter=`. An explicit
word whose anchor discovery refused for the hunt is a refusal, and it ends the
run with status 1:

```text
hunt: the hunt norm cycle names the anchor 'cpu/cpu-cycles', which discovery refused for the hunt of 'cpu/l2_lines_out.any_core'
```

`rate` is the median of the per-window rates and `cv` is the coefficient of
variation of those rates, as a percentage. `--hunt-rank=rate` orders the table
by `rate` descending and `--hunt-rank=cv` by `cv` ascending, with the catalog
address as the tie break. At most `--hunt-top` rows carry a rank, numbered from
1 with no gap. Every other row prints `rank -` and a `mark=`: `noisy` for a
`cv` above the `--hunt-noise` percent, `unrepeated` for an event with one
sample, `beyond-top` for a rankable event the size of the table left out. One
sample carries no deviation, so it is never `noisy`.

### The budget

With no `--hunt-budget` one stage measures every planned event at
`--hunt-repeats` repetitions. A budget splits the run into two: a first stage
over every planned event at one repetition, then a second stage over the first
`3 x --hunt-top` planned events, or over the whole plan if it holds fewer, at
`--hunt-repeats`. A budgeted run measures everything once and the top set
again. The budget prints in the unit it was given, `budget=120s` or `budget=8p`,
on the plan line and on the summary, and the summary adds the time the hunt
spent.

### The stated collection method

The plan settles its method from the published counter limits: `PASSES` if a
device it needs publishes one, `MULTIPLEX` if no needed device publishes one.
`--metric-method` states a method outright, and the stated word outranks that
settlement. The plan hands that word to the pass loop, the plan line prints
the stated word in `method=`, every rank row carries the word `multiplexed` if
the settled method is `MULTIPLEX`, and both `method` keys of the document carry
the stated word. `--hunt` with `--metric-method=multiplex`
therefore measures every event in one oversubscribed window and says so in all
three places.

### The two hunt objects

Every benchmark entry of a hunt run carries a `hunt` object as its last key,
with `method`, `pass_count`, `top`, `retried_passes`, `entries` and `skipped`.
`method` is the settled method, `pass_count` counts the passes the run measured
with the passes a retry appended included, `top` is the `--hunt-top` value, and
`retried_passes` counts the passes whose group open refused. Every item of
`entries` carries `address`, `rate`, `cv`, `noisy`, `multiplexed` and `rank`,
the ranked events first in rank order and the excluded ones after them in
address order, each `rank` of an excluded event standing `null`. Every item of
`skipped` carries `address`, `kind` and `stage`, and `stage` stands `null`
unless a stage lost that event. A run with no hunt writes no `hunt` key in any
entry.

The document closes with one run-level `hunt` object after the `benchmarks`
array, holding `selection`, `norm`, `rank`, `top`, `noisePercent`, `repeats`,
`seed`, `budget`, `passes`, `method`, `events`, `stages`, `estimate_ns`,
`planned_events`, `ran_events` and `stage_two_events`. `selection` repeats the text the option carried, and a `@file` selection
arrives here as the deduplicated address lines it read. `events` holds every planned address in plan order,
and `stages` holds one item per stage that ran, each with `first`, `count` and
`repeats`. `budget` stands `null` with no budget and `estimate_ns` stands
`null` with no calibration figure. `ran_events` counts the events that produced
a usable sample and `stage_two_events` the events the second stage re-measured.

The words these keys take are the words the options take: `method` is `AUTO`,
`PASSES` or `MULTIPLEX`, `norm` is `instr`, `cycle` or `iter`, the ranking key
`rank` is `rate` or `cv`, and a `kind` is `permission-blocked`,
`not-encodable`, `absent`, `scope-refused` or `gap`.

### The library surface

`speedgun-ng/hunt.hpp` carries the surface the options drive.
`runHunt(benchmark, options, calibrationNsPerPass)` plans, measures and ranks
one hunt for a registered benchmark. `planHunt(events, limits, anchors, options,
calibrationNsPerPass)` answers the plan over the events, the per-device limits
and the anchors, and `rankEvents(collected, options)` ranks samples the caller
already holds. None of the three opens a window: every time and count figure in
their records comes from an argument. `EventCatalog` expands a selection text
against the catalog and splits it into the events to plan and the events
discovery refused.
