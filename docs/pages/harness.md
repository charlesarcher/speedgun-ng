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

Coverage: `test/source/harness_family_test.cpp`. The `FamilyCase` table
asserts the instance set of every call through list mode, with rows
`bmArg`, `bmArgsList`, `bmArgsVector`, `bmRange`, `bmHalf`, `bmLater`,
`bmRanges`, `bmDense`, `bmStepped`, `bmProduct`, `bmApplied`,
`bmLabelled`, `bmNamed`, `bmBuilt`, `bmBuiltScaled`, `bmBuiltDense`,
and the chained site `bmChain`.
The `oversize` re-exec mode asserts the single warning naming the bound
and the unchanged exit status, and the re-exec modes `bad-multiplier`,
`bad-range`, `bad-step`, `bad-argnames`, `bad-apply`, and `bad-builder`
assert each precondition.

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

Coverage: `test/source/harness_instance_name_test.cpp`. The
`nameFormScenario` rows assert the name forms, `filterScenario` asserts
that one full instance name runs exactly one instance,
`listAndCatalogScenario` asserts the listed set and the silent
`--catalog` path, and `duplicateScenario` with its `duplicate` re-exec
mode asserts the duplicate line, the kept instance, the dropped
instance, and the unchanged status.

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

Coverage: `test/source/harness_argument_test.cpp`. `twoArgumentFamily`
reads each instance's pair, `argumentCountReported` reads the counts,
`rangeReadAllocatesNothing` counts allocations through a replacing
`operator new`, and `readZeroArgumentFamily` and `readPastFamilyArity`
observe the precondition violations through the `range-zero-args` and
`range-past-arity` re-exec modes.

## Captured arguments

`SG_BENCHMARK_CAPTURE(fn, captureName, ...)` registers `fn` once with
the captured values bound into the callable, under the instance name
`fn/captureName`. The capture name is one name segment, and the
instance takes part in the filter, list mode, and duplicate handling of
the instance names above like any other instance. The registration yields
the handle, so a
site chains family calls: `SG_BENCHMARK_CAPTURE(fn, pair, 2).arg(4);`
registers `fn/pair/4`.

Coverage: `test/source/harness_capture_macro_test.cpp`.
`captureNameScenario` lists `addTwo/pair` and no further name under it,
`capturedValuesScenario` runs the instance and checks the sum of the
captured values, `twoCapturesScenario` registers `fn` twice under
`captureA` and `captureB`, `chainedCaptureScenario` registers
`bmCapChain/pair/4` through a chained site and reads the captured
value and the argument from the state, and `duplicateScenario` shows a
capture name clashing with a family instance at the duplicate tier.

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

Coverage: `test/source/harness_template_test.cpp`.
`singleTypeArgumentScenario` asserts `sortOf<int>`,
`twoTypeArgumentsScenario` asserts `sortOf<int, double>` with the comma
spacing of the macro site, `qualifiedTypeArgumentScenario` asserts
`pairOf<std::pair<int, int>>`, `chainedSiteScenario` registers
`chainOf<int>/8` through a chained site and reads `range(0)`, and
`instantiationRunsScenario` runs each of the three instances and checks
that the reached instantiation carries the named types.

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

Coverage: `test/source/harness_fixture_test.cpp`. `instanceNameScenario`
asserts `QueueFixture/push` in list mode, `pairPerRunScenario` counts
the pair against the runs of a fixed-count, repeated, and warm-up
instance, `untimedScenario` checks the scripted `machine/monotonic`
delta against the reported time, `oneObjectPerRunScenario` counts
fixture constructions and destructions, `laterRegistrationScenario`
covers the define-then-register pair, `templateScenario` covers
`TypedFixture<int>/run`, `templatePairScenario` covers the two split
template pairs, `callbackStateScenario` reads the argument and iteration
counts from the state of the pair and re-execs the binary to observe
`begin()` and `skipWithError()` aborting inside the pair,
`chainedFixtureScenario` expands a chained `SG_BENCHMARK_REGISTER_F`
site into three instances each with its own pair, and
`disabledMethodScenario` covers the
`DISABLED_` exception of a fixture method.

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

Coverage: `test/source/harness_callback_test.cpp`. `pairPerRunScenario`
and `repetitionsScenario` count the pair against every run,
`untimedScenario` burns work in a setup callback and checks the
scripted time, `argumentReadScenario` reads `range(0)` of the running
instance, `stateReadScenario` reads `rangeCount()` and `iterations()`,
`lastAttachmentScenario` shows the first attachment staying at zero,
`orderScenario` prints the event log of the wrapping order for a plain
instance and a fixture instance, and `guardScenario` observes
`begin()`, `skipWithError` and `skipWithMessage` aborting in a callback
state as `SIGABRT` in a forked child.

## The `DISABLED_` prefix

The gate is a prefix test on the expanded instance name, held at
selection time in `source/harness/cli.cpp`. An instance whose name
starts with `DISABLED_` registers, expands, never runs, stays out of
the filter match, and stays out of list mode. A fixture method named
`DISABLED_x` runs, because that instance name starts with the fixture
class. A `DISABLED_` family as the only match of a filter leaves the
executable reporting no match, running nothing, and exiting zero.

Coverage: `test/source/harness_disabled_test.cpp`.
`disabledNeverRunsScenario` keeps the function of `DISABLED_slow` at
zero while its sibling runs, `listExcludesScenario` keeps the prefix
out of the listed set, `fixtureScenario` and `midNameScenario` show
where the prefix does not reach, and `filterNoMatchScenario` and
`onlyDisabledScenario` assert the no-match report and the zero exit.
The fixture exception is covered a second time by
`disabledMethodScenario` in `test/source/harness_fixture_test.cpp`.

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

Coverage: `test/source/harness_instance_name_test.cpp`.
`suiteCaseScenario` runs the three derivation rows of
`contracts/result-fields.md` through `Runner::run` and checks the two
fields, `pairUniquenessScenario` checks uniqueness across the instance
list, `suiteOrderScenario` walks the grouped order of the instance
list, and `consoleRowScenario` checks the single name column.

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

Coverage: `test/source/harness_time_columns_test.cpp`. `columnsScenario`
reads each column against its scripted delta over the iteration count,
`metricScenario` keeps the core metric fold beside the two
columns, and `aggregateScenario` compares the aggregate line against
the CPU selection.

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

Coverage: `test/source/harness_cpu_leaf_test.cpp`. `processCpuScenario`
reads the scripted `machine/process_cpu` delta in the CPU column,
`combinedScenario` runs the switch together with `useRealTime`, and
`missingProcessCpuScenario` with its `missingProcessCpu` re-exec mode
asserts the failed benchmark and its resolution line.

## Real time as the primary time

`BenchmarkHandle::useRealTime()` makes real time the primary time of
one family and returns the handle. From that call on, the qualification
test, the growth rule, and the aggregate time statistics read the real
column, and the CPU column keeps the CPU fold it carries. The selection
excludes `useManualTime`: the second of the two calls is the
`SG_REQUIRE` precondition violation, in either order. `bmRealTime` in
`example/benchmark_example.cpp` carries the call.

Coverage: `test/source/harness_real_time_test.cpp`. `scenarioRealWalk`
and `scenarioCpuWalk` run one scripted growth sequence under each
selection and compare the iteration counts the walk reaches, and
`scenarioAggregate` checks the aggregate line against the real column.
The three stand behind the `real-walk`, `cpu-walk`, and `aggregate`
re-exec modes.

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

Coverage: `test/source/harness_manual_time_test.cpp`. `scenarioReport`
checks the accepted sum over N in the real column, `scenarioGrowth`
reads that sum through the growth rule, `scenarioFivefold` shows the
exemption, `scenarioEmpty` reports the empty sum at the cap, and
`scenarioRule` with the `real-then-manual` and `manual-then-real`
re-exec modes asserts each order of the conflicting pair.

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

Coverage: `test/source/harness_pause_test.cpp`. `scenarioExact` checks
every time column and an attached metric against the window fold minus
the scripted pause fold to the nanosecond, `scenarioScoped` reports the
same value through the guard, `scenarioEveryIteration` closes a guard in
every iteration and reads the netted columns of the folded path,
`scenarioPauseHeavy` grows a two-pair loop past the cap and completes
with the reservation at or below the ceiling, `scenarioNonPauseReserve`
shows a run that pauses nothing reserving no pause room, and
`scenarioRawAfterHistory` compares a folded repetition against the raw
repetition that follows it. The `doublePause`, `resumeFirst`,
`openAtExit`, and `callbackPause` re-exec modes assert each precondition
breach.

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

Coverage: `test/source/harness_time_unit_test.cpp`. The `unitCall`
re-exec mode checks the two columns, the overhead floor, and the
aggregate line under the `ms` label through `checkScaledRow` and
`checkMillisecondAggregate`, the `default` mode scales a benchmark that
sets no unit, the `captured` mode moves the unit of a benchmark the
macros registered before any mode could set a default, and the
`testHook` mode resolves the unit through the runner that a test drives
without the entry point.
