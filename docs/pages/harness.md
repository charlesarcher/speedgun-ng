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