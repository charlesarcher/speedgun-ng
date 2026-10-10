# The benchmark harness registration surface (016 C-1 to C-10)

Feature 016 completes the registration and fixture surface of the
benchmark harness on top of the feature-015 core. Each section below
states the surface of one capability and names the test that covers it.
Every suite named here runs on the counters fake provider, so no test
needs a PMU.

## C-1: Argument families

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
`std::int64_t`. One `args` call appends one argument list, so
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
and the first run.

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

## C-2: Instance names

An instance name starts with the family name and adds one `/`-joined
segment per argument. A segment reads `label:value` where `argName` or
`argNames` set a label for that position, and an empty label leaves the
segment without one. A family with no family call keeps the family name
alone, with no `/`. The H1 `--filter` option and list mode match
instance names, so one full instance name selects one instance and the
`--catalog` path prints none of them. Two instances with one name are a
recoverable error found at expansion: one line on the standard error
stream names both instance names, the earlier instance stays, the later
instance does not run, and the exit status stays as the harness core
fixes it.

Coverage: `test/source/harness_instance_name_test.cpp`. The
`nameFormScenario` rows assert the name forms, `filterScenario` asserts
that one full instance name runs exactly one instance,
`listAndCatalogScenario` asserts the listed set and the silent
`--catalog` path, and `duplicateScenario` with its `duplicate` re-exec
mode asserts the duplicate line, the kept instance, the dropped
instance, and the unchanged status.

## C-3: Argument access

`State::range(std::size_t index)` returns the argument of the running
instance at that position and `State::rangeCount()` returns the
argument count. An index at or above `rangeCount()` is an `SG_REQUIRE`
precondition violation. A family with no family call has one instance
with zero arguments, so `rangeCount()` reports 0 and `range(0)`
violates. A read allocates nothing: the instance owns the storage and
the state holds a `std::span<const std::int64_t>` fixed at expansion.
The timed loop keeps the feature-015 shape, and the loop-shape gate
`test/loop_shape.sh` stays green.

Coverage: `test/source/harness_argument_test.cpp`. `twoArgumentFamily`
reads each instance's pair, `argumentCountReported` reads the counts,
`rangeReadAllocatesNothing` counts allocations through a replacing
`operator new`, and `readZeroArgumentFamily` and `readPastFamilyArity`
observe the precondition violations through the `range-zero-args` and
`range-past-arity` re-exec modes.

## C-4: Capture

`SG_BENCHMARK_CAPTURE(fn, captureName, ...)` registers `fn` once with
the captured values bound into the callable, under the instance name
`fn/captureName`. The capture name is one name segment, and the
instance takes part in the filter, list mode, and duplicate handling of
C-2 like any other instance. The registration yields the handle, so a
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

## C-5: Templates

`SG_BENCHMARK_TEMPLATE(fn, ...)` registers a function template
instantiated over one or more type arguments, under the instance name
`fn<` followed by the type list stringified exactly as written at the
macro site, followed by `>`. There is no demangling: the spacing at the
macro site is the spacing in the name. The registration yields the
handle, so a site chains family calls:
`SG_BENCHMARK_TEMPLATE(fn, int).arg(8);` registers `fn<int>/8`.

Coverage: `test/source/harness_template_test.cpp`.
`singleTypeArgumentScenario` asserts `sortOf<int>`,
`twoTypeArgumentsScenario` asserts `sortOf<int, double>` with the comma
spacing of the macro site, `qualifiedTypeArgumentScenario` asserts
`pairOf<std::pair<int, int>>`, `chainedSiteScenario` registers
`chainOf<int>/8` through a chained site and reads `range(0)`, and
`instantiationRunsScenario` runs each of the three instances and checks
that the reached instantiation carries the named types.

## C-6: Fixtures

`sg::Fixture` is the fixture base class: a virtual destructor and the
virtual `setUp(State&)` and `tearDown(State&)` with empty defaults. The
seven macros of FR-016 attach a method of a derived class:
`SG_BENCHMARK_F`, `SG_BENCHMARK_DEFINE_F` paired with
`SG_BENCHMARK_REGISTER_F`, `SG_BENCHMARK_TEMPLATE_F`,
`SG_BENCHMARK_TEMPLATE_DEFINE_F`, `SG_BENCHMARK_TEMPLATE_METHOD_F`
paired with `SG_BENCHMARK_TEMPLATE_INSTANTIATE_F`. A fixture instance
is named `FixtureClass/Method`, and a template fixture instance is
named `BaseClass<types>/Method`. The pair runs in the untimed region of
every run, the warm-up, calibration, and measured runs included, and
the runner builds one fixture object per run and destroys it after
`tearDown`. The state the pair receives is the callback state of C-7:
`range(index)`, `rangeCount()` and `iterations()` are legal in it,
while `begin()`, `skipWithError` and `skipWithMessage` are `SG_REQUIRE`
precondition violations. `SG_BENCHMARK_REGISTER_F` and
`SG_BENCHMARK_TEMPLATE_INSTANTIATE_F` yield the handle, so a fixture
family chains its family calls at the site:
`SG_BENCHMARK_REGISTER_F(F, m).denseRange(1, 3);` expands the fixture
record like any family record, one instance per argument list.

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
fixture exception of C-8.

## C-7: Setup and teardown callbacks

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
state. This feature runs one thread; the once-per-thread-group rule
belongs to the threads spec of the roadmap.

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

## C-8: The `DISABLED_` prefix

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

## C-9: Suite and case naming

`BenchmarkResult` carries `suite` and `caseName` beside the H1 `name`.
The suite is the family name up to its first `/`, and a fixture
instance takes its fixture class as the suite. The case is the instance
name with the leading suite and its `/` removed, and an instance name
equal to its suite keeps that name as its case. The pair is unique per
instance. After expansion the instance list groups stably by suite in
first-appearance order, and rows within one suite keep registration
order. The console row keeps the H1 shape: one name column carrying the
full instance name, with the split readable on the result value. A
later JSON report reuses the two fields.

Coverage: `test/source/harness_instance_name_test.cpp`.
`suiteCaseScenario` runs the three derivation rows of
`contracts/result-fields.md` through `Runner::run` and checks the two
fields, `pairUniquenessScenario` checks uniqueness across the instance
list, `suiteOrderScenario` walks the grouped order of the instance
list, and `consoleRowScenario` checks the single name column.

## C-10: Examples and documentation

FR-027 asks for one documentation section per capability, and this page
carries them. The example suite carries the new surface:
`example/benchmark_example.cpp` registers the argument family `bmArgs`
with the `argName` labels `width` and `depth`, the fixture method
`TableFixture/bmTableTouch` through `SG_BENCHMARK_F`, and the templated
benchmark `SG_BENCHMARK_TEMPLATE(bmFill, std::uint64_t)`.
