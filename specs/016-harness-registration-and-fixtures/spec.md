# Feature Specification: Harness Registration and Fixtures

**Feature Branch**: `016-harness-registration-and-fixtures`

**Created**: 2026-10-09

**Status**: Draft

**Input**: User description, opening quoted, the full text held by the
specify invocation record:

<!-- prose-lint: allow reason="XI.5 verbatim quotation of the request, marked as quoted" -->
"Harness registration and fixtures, spec H2 of the speedgun harness
roadmap. Spec H1, benchmark-harness-core, shipped a single-thread
runner with one registration form: a function under one name. This
spec completes the registration and fixture surface of Google Benchmark
features F-1 and F-2. It covers argument families, instance names,
argument capture, templated benchmarks, fixtures, setup and teardown
callbacks, the DISABLED_ prefix, and suite and case naming. The spec
needs no hardware: every test runs on the fake provider. [...] This
spec extends that surface. It replaces no H1 signature without a
recorded version effect."

## Audit point

| Field | Value |
| --- | --- |
| Audit point | `30f31188167a8215dd636b786128a6a4a1ffe0c2` |
| Audit-point date | 2026-10-09 14:21:22 -0500 |
| Default branch at run time | `master`, tip `30f3118` |
| Working tree at run time | `015-benchmark-harness-core` at `30f3118`, equal to the `origin/master` tip |
| Feature number at run time | 016, the next free number under `specs/` |
| Short name at run time | `harness-registration-and-fixtures` |
| Project version at the audit point | 0.6.0 |
| `SOVERSION` at the audit point | 2 |
| Package compatibility at the audit point | `SameMinorVersion` |
| Constitution version at the audit point | 2.18.0, lineage table and footer equal |

The audit read the tree of `30f3118`, the `master` commit that merged
pull request 32. That pull request is the H1 merge named in the
request. Every citation below was read at `30f3118`.

## Preconditions

Each precondition was checked in the code and the cited artifacts at
the audit point. The result and its evidence follow.

### PC-0: H1 is merged with every task complete. PASS

- `specs/015-benchmark-harness-core/` exists with `spec.md`, `plan.md`,
  `tasks.md`, `research.md`, `data-model.md`, `contracts/`, and
  `checklists/`.
- Its `tasks.md` carries 77 closed task boxes and zero open boxes.
- Pull request 32, "Harness: Add the benchmark harness core", is
  `MERGED` into `master` at `30f3118` (merged 2026-10-09T19:26:31Z),
  and `30f3118` is the `origin/master` tip.

### PC-1: the H1 registration surface. PASS

Every H1 name in the request was confirmed at the audit point. The
surface, with file and line:

- `registerBenchmark(std::function<void(State&)> fn, std::string_view
  name) -> BenchmarkHandle`
  (`include/speedgun-ng/benchmark.hpp:556`).
- `BenchmarkHandle` (`benchmark.hpp:450`), an exported class holding one
  registry-entry pointer, with the setters `minTime` (`:478`),
  `warmupTime` (`:488`), `repetitions` (`:497`), `iterations` (`:506`),
  and the templated `addMetric` (`:522`).
- `State` (`benchmark.hpp:140`), an exported class, with `iterations()`
  (`:237`), `begin()` (`:256`), `end()` (`:277`), `skipWithError`
  (`:291`), `skipWithMessage` (`:312`), and the loop cursor `Cursor`
  (`:149`).
- The `SG_BENCHMARK(fn)` macro (`benchmark.hpp:582`), registering `fn`
  at namespace scope before `main` under the name of `fn`.
- `speedgunMain(int argc, char** argv) -> int` (`benchmark.hpp:576`).
- The registry entry `RegistryEntry`
  (`source/harness/detail/internal.hpp:31`), a non-public record with
  `name`, `callable`, the run-control optionals, `metrics`, and
  `runStarted`; `addRegistryEntry` (`:53`) reports a duplicate name.
- The filter and list mode: `RunOptions::filter` and
  `RunOptions::listMode` (`internal.hpp:65-66`), the `--filter` and
  `--list` options of `specs/015-benchmark-harness-core/contracts/cli.md:11-12`.

This spec extends that surface. It replaces no H1 signature. The
version effect of the extension is recorded under PC-4 and FR-026.

### PC-2: the timed window. PASS

- H1 FR-005 (`specs/015-benchmark-harness-core/spec.md:603`) places
  setup and teardown in the untimed region and requires the state to
  report the iteration count of the current run. H1 FR-017
  (`spec.md:651`) samples once at the entry of the timed loop and once
  at its exit. The loop carries exactly two raw points per run and no
  other counter work.
- The H1 merge moved both samples into `State`. H1 takes the entry
  sample in `State::begin()` through `openWindow()`
  (`benchmark.hpp:256-266, 354`). The exit sample comes from the path
  that leaves the loop: `Cursor::operator!=` into `closeWindow()` into
  `takeExitSample()` (`benchmark.hpp:173-179, 365-381`). The recorder
  handle is a `State` member (`:404`).
- The loop-shape gate `test/loop_shape.sh` reads the shipped loop back
  from the generated code. It fails on a per-iteration check or flag
  load (IF-04; 015 FR-004, FR-005, R-06). Fixture setup and teardown
  and the callbacks of this spec rest on this rule, and the rule holds.

### PC-3: the constitution. PASS, with one recorded observation

- The constitution is 2.18.0 (footer `line 1149`, lineage row
  `line 1125`).
- Principle V.1 states the naming rules N-1 through N-12
  (`.specify/memory/constitution.md:533-580`). N-3 requires the `SG_`
  prefix on every macro (`:549`).
- Observation required by the request: the N-6 text (`:555-556`) lists
  the namespace tree as `sg`, `sg::counters`, and
  `sg::counters::detail`. It does not list the `sg::detail` namespace
  that H1 uses (`benchmark.hpp:30-33`, `internal.hpp:57`). The rule
  itself, `lower_case`, binds `sg::detail` and every namespace this
  spec may add; the enumeration in the text is descriptive and lags
  H1. No new namespace outside that tree is needed here.
- V.2 states the closed naming-exception list (`:583-634`). Nothing
  this spec adds needs an entry: every new name takes the project
  spelling.
- Principle VIII carries the hard gates (`:657-699`), including the
  name-check gate, the prose gate, the coverage gates, and the
  time-source gate item (`:691`). The counters-only rule sits in
  Additional Constraints (`:1068-1092`) with its forbidden time
  sources and its single named exception.

### PC-4: the layout of `State` and `BenchmarkHandle`. PASS

- `State` (`benchmark.hpp:140`) and `BenchmarkHandle` (`:450`) are
  exported classes (`SPEEDGUN_NG_EXPORT`) with inline members. The
  data members of `State` are at `:400-408`; `BenchmarkHandle` holds
  one pointer (`:542`).
- Argument storage in `State` changes its layout. `BenchmarkHandle`
  gains methods only and keeps its one-pointer layout.
- The `SOVERSION` value is hand-kept. The rule is stated in full at
  `CMakeLists.txt:41-47` for the counters archive. The same rule
  applies to the harness archive at `CMakeLists.txt:211-218`. A
  comment there records the H1 decision: "the harness adds a surface
  and changes no existing signature, so the ABI number stays 2 (D-5,
  FR-046)". This spec changes that condition: `State` gains storage.
  The rule applies, and FR-026 carries the effect.

### PC-5: the fake provider. PASS

- `FakeProvider` (`include/speedgun-ng/counters_fake.hpp:102`) scripts
  any leaf through `FakeScript` (`:43`): an explicit cumulative point
  sequence per leaf, with gap actions (`:86, 226`). It scripts
  `machine/monotonic` and `machine/thread_cpu` like any other leaf.
- The H1 harness tests already run on it.
  `test/source/harness_calibration_test.cpp` scripts `machine/thread_cpu`
  so run deltas are exact (lines 7 and 252), and
  `test/source/harness_capture_test.cpp` takes time from
  `machine/monotonic` alone (line 362). H1 FR-042
  (`specs/015-benchmark-harness-core/spec.md:807`) binds the fake
  provider as the test substitution. Every test of this spec runs on
  it, and no test needs a PMU.

## Roadmap position

This is spec H2 of the speedgun harness roadmap. H1 shipped the runner
and one registration form. This spec completes the registration and
fixture surface of Google Benchmark features F-1 and F-2. It covers
argument families, instance names, argument access, capture,
templates, fixtures, setup and teardown callbacks, the `DISABLED_`
prefix, and suite and case naming. The spec needs no hardware; every
test runs on the fake provider. The roadmap document sits outside this
repository and is absent from this machine at the audit point. The
scope boundary below comes from the request. The roadmap spec names
are the boundary.

## Google Benchmark reference read

Google Benchmark is a reference read, nothing more. No vendored copy
enters this spec, and the harness links nothing from it. The revision
read is `google/benchmark` `main` at
`e662de9aab8e705ecf4fa4bd41a207e5a0acfd0c` (2026-10-08 15:52:18Z),
the same revision H1 read (`015 spec.md:1111`). Each semantic this
spec takes, with the file and function of that revision:

| Semantic taken | Source at the revision |
| --- | --- |
| Family calls `Arg`, `Args`, `Range`, `RangeMultiplier`, `Ranges`, `DenseRange`, `ArgsProduct`, `Apply`, `ArgName`, `ArgNames` | `src/benchmark_register.cc` (`Benchmark::Arg` `:250`, `::Range` `:262`, `::Ranges` `:273`, `::ArgsProduct` `:287`, `::ArgName` `:315`, `::ArgNames` `:321`, `::DenseRange` `:327`, `::Args` `:336`, `::Apply` `:342`, `::RangeMultiplier` `:372`); declarations `include/benchmark/benchmark_api.h` (`Benchmark` `:94-144`) |
| Free list builders `CreateRange`, `CreateDenseRange` | `src/benchmark_register.cc` (`CreateRange` `:544`, `CreateDenseRange` `:550`); declarations `benchmark_api.h:282,285` |
| Range growth: bounds inclusive, powers of the multiplier strictly between, default multiplier 8, negatives mirrored | `src/benchmark_register.h` (`AddRange` `:61`); `src/benchmark_register.cc` (`kRangeMultiplier` `:61`) |
| Range preconditions: multiplier above 1, low not above high, arity consistent | `benchmark_register.cc` (`RangeMultiplier` `:373`, `DenseRange` `:329`, `CreateDenseRange` `:551`, `AddRange` checks through `benchmark_register.h:64-65`, `ArgsCnt` `:509`) |
| Instance name: family name, then one `/`-joined segment per argument. A segment reads `label:value` where an argument name is set | `src/benchmark_api_internal.cc` (`BenchmarkInstance` constructor `:34-51`); `src/benchmark_name.cc` (`BenchmarkName::str` `:56`) |
| Capture name: `func/captureName` | `include/benchmark/registration.h` (`BENCHMARK_CAPTURE` `:69-75`) |
| Template name: `fn<`, the type arguments as written, then `>`. The variadic form yields `fn<int, double>` | `registration.h` (`BENCHMARK_TEMPLATE` `:101-107`) |
| Fixture instance name: `BaseClass/Method`, template fixture `BaseClass<types>/Method` | `registration.h` (`BENCHMARK_PRIVATE_DECLARE_F` `:121-130`, `BENCHMARK_TEMPLATE_PRIVATE_DECLARE_F` `:154-163`) |
| Fixture `SetUp`/`TearDown` run inside `Run`, around the benchmark method | `benchmark_api.h` (`Fixture` `:264-279`) |
| Setup and teardown callbacks wrap each run: warm-up, calibration, and measured runs included. They receive a state carrying the instance arguments | `src/benchmark_runner.cc` (`RunWarmUp` `:440-442`, `DoOneRepetition` `:513-515`); `src/benchmark_api_internal.cc` (`BenchmarkInstance::Setup` `:103-109`, `::Teardown` `:111-117`) |
| `DISABLED_` prefix: registered, never run, excluded from filter matching and from the listed set | `benchmark_register.cc` (`kDisabledPrefix` `:67`, `FindBenchmarks` `:181`); `src/benchmark.cc` (`RunSpecifiedBenchmarks` list path `:650-651`) |
| Family-size warning above 100 instances | `benchmark_register.cc` (`kMaxFamilySize` `:65`, warning `:163-165`) |
| Argument access: `range(pos)` returns the instance argument, `range_size()` the count | `include/benchmark/state.h` (`range` `:123`, `range_size` `:151`) |

Semantics deliberately not taken: the name segments for `min_time`,
`iterations`, `repeats`, time type, and threads
(`benchmark_api_internal.cc:53-89`) belong to later roadmap specs.
The thread families (`Threads`, `ThreadRange`, `ThreadPerCpu`) belong
to the threads spec. This spec omits the deprecated `range_x` and
`range_y` accessors (`state.h:128-132`). It also omits the
`BENCHMARK_TEMPLATE1`, `BENCHMARK_TEMPLATE2`, and template capture
macros; the variadic forms cover them.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Sweep one function over a family of arguments (Priority: P1)

A user registers one benchmark function and states its arguments as a
family. The family forms are single arguments, an argument vector, a
geometric range, and a dense range. Two more forms are a product of
lists and a saved list built with the free range helpers. Each
instance of the family runs, sees its own argument values through the
state, and reports under a name that carries those values. This is
the MVP slice: without families the harness runs one function once,
and every benchmark suite in the roadmap's examples needs the sweep.

**Why this priority**: F-1 is the registration surface every later
capability names instances against. Capture, templates, and fixtures
all produce instances that this story's naming and argument access
already define.

**Independent Test**: Register one function with each family call on
the fake provider, and run the suite. Check that the instance set, the
per-instance argument reads, and the reported names match the table of
the cited revision.

**Acceptance Scenarios**:

1. **Given** a function registered with `args({8, 64})` and
   `args({8, 1024})`, **When** the suite runs, **Then** two instances
   run. Each instance reads its own pair through `range(index)`, and
   the names carry one segment per argument.
2. **Given** a function registered with `range(8, 1024)` at the
   default multiplier, **When** the suite runs, **Then** the instance
   set is the geometric set. That set holds the bounds and the powers
   of 8 strictly between, as the cited revision defines.
3. **Given** a function registered with `denseRange(1, 4)`, **When**
   the suite runs, **Then** four instances run with arguments 1, 2, 3,
   4.
4. **Given** a function registered with `argsProduct` over two lists,
   **When** the suite runs, **Then** the instance set is the product
   in the order of the cited revision.
5. **Given** a family with a fixed argument count, **When** a later
   family call states another count, **Then** the FR-006 check reports
   a violation. The violation names the arity.

---

### User Story 2 - Name, filter, and list the instances (Priority: P2)

A user names argument positions with `argName` and reads the generated
instance names. The H1 filter and list mode select instance names, so
the user can run or list exactly one instance of a family. Two
instances that would carry one name are a recoverable registration
error, and the first registration stays.

**Why this priority**: Naming is what makes a family addressable from
the command line; it depends on US1 and adds no new run mechanics.

**Independent Test**: Register families with and without argument
labels, and run list mode and filters. Compare the names and the
selected set against the cited revision.

**Acceptance Scenarios**:

1. **Given** a family with `argName("size")` and one argument,
   **When** list mode prints the names, **Then** each argument segment
   reads `label:value`.
2. **Given** a filter that matches one instance name exactly,
   **When** the suite runs, **Then** exactly that instance runs.
3. **Given** two registrations whose expanded instance names are
   equal, **When** the suite starts, **Then** the harness reports a
   recoverable error at the H1 duplicate tier. The earlier instance
   stays. The later instance does not run, and every other instance
   runs.

---

### User Story 3 - Capture arguments and instantiate templates (Priority: P2)

A user registers a function with extra captured parameters under a
capture name, as `BENCHMARK_CAPTURE` does. The user also registers a
function template instantiated over one or more type arguments, as
`BENCHMARK_TEMPLATE` does. The capture name and the type list become
name segments.

**Why this priority**: These are the other two registration forms of
F-1; they reuse US1's instance machinery and add only name forms.

**Independent Test**: Register one capture and one template
instantiation, run, and compare the instance names and the captured
values against the cited revision.

**Acceptance Scenarios**:

1. **Given** `SG_BENCHMARK_CAPTURE(addTwo, pair, 2, 2)`, **When** the
   suite runs, **Then** one instance runs named `addTwo/pair` and the
   captured values reach the function.
2. **Given** `SG_BENCHMARK_TEMPLATE(sortOf, int, double)`, **When**
   the suite runs, **Then** one instance runs named
   `sortOf<int, double>`.

---

### User Story 4 - Fixtures and suite grouping (Priority: P2)

A user derives a class from the fixture base, overrides `setUp` and
`tearDown`, and defines benchmark methods with the fixture macros. The
fixture pair runs around each run in the untimed region, so its work
never enters the reported time. Each instance carries a suite name and
a case name. A fixture instance takes the fixture class as its suite.
The report prints suites in the order of their first registered
instance, and rows within one suite keep registration order.

**Why this priority**: Fixtures are the F-2 half of the surface; they
depend on US1's instance model and drive the report's grouping.

**Independent Test**: Register a fixture with scripted counting in
`setUp` and `tearDown`, and run on the fake provider. Check the call
counts, the scripted clock deltas, and the grouped report.

**Acceptance Scenarios**:

1. **Given** a fixture instance, **When** the harness runs it,
   **Then** `setUp` runs before the timed loop and `tearDown` after
   it, once per run. The scripted clock deltas show their work adds
   nothing to the reported time.
2. **Given** two fixture instances of one fixture class, **When** the
   report prints, **Then** both rows carry the fixture class as their
   suite. The rows group under it in registration order.
3. **Given** `SG_BENCHMARK_DEFINE_F` defining a method and
   `SG_BENCHMARK_REGISTER_F` registering it later, **When** the suite
   runs, **Then** the instance runs named `FixtureClass/Method`.

---

### User Story 5 - Setup and teardown callbacks (Priority: P2)

A user attaches setup and teardown callbacks to a registration handle.
Each callback runs around each run, in the untimed region, at the
points the cited revision uses, and receives a state carrying the
instance arguments.

**Why this priority**: The callback pair is the last registration
option of F-1; it depends on US1 and touches no report shape.

**Independent Test**: Attach counting callbacks, run on the fake
provider with warm-up and repetitions, and check the call counts and
the scripted clock deltas.

**Acceptance Scenarios**:

1. **Given** a handle with `setup` and `teardown` callbacks, **When**
   the harness runs the instance, **Then** the callbacks run once
   around each run. Warm-up, calibration, and measured runs each get
   the pair, and the reported time carries no callback work.
2. **Given** a setup callback reading `range(0)`, **When** it runs for
   an instance with arguments, **Then** it reads that instance's
   argument.

---

### User Story 6 - Disabled benchmarks, examples, and documentation (Priority: P3)

A family name with the `DISABLED_` prefix registers and never runs;
the filter and list mode treat it as the cited revision does. The
example suite gains one argument family, one fixture, and one
templated benchmark. A new harness documentation page holds a section
for each capability.

**Why this priority**: The prefix is a run gate over surface US1 to
US5 already build; examples and documentation follow the code.

**Independent Test**: Register a `DISABLED_` family, run list mode,
and run a matching filter. Check that no function runs and that the
listing and filter follow C-8.

**Acceptance Scenarios**:

1. **Given** a family registered as `DISABLED_slow`, **When** the
   suite runs with no filter, **Then** no function of that family
   runs.
2. **Given** the same family, **When** list mode runs, **Then** the
   family is not listed, and a filter naming it matches nothing.
3. **Given** the example suite, **When** it is built and run,
   **Then** it shows one argument family, one fixture, and one
   templated benchmark.

### Edge Cases

- A family expands to more than 100 instances: the harness warns with
  the bound of the cited revision and runs anyway.
- A `range` low bound above its high bound, a `rangeMultiplier` below
  2, or a `denseRange` step of zero is a precondition violation. The
  values come from code.
- An `argName` or `argNames` list whose length differs from the family
  arity: a precondition violation, as the cited revision checks
  (`benchmark_register.cc:315-324`). An empty label leaves its segment
  without a label.
- A family with no family call at all has one instance with zero
  arguments, as H1 leaves it. `rangeCount()` reports zero and
  `range(0)` is a precondition violation.
- A `range(index)` index at or above the argument count: a
  precondition violation.
- A capture name or template instantiation that collides with an
  existing instance name: the H1 duplicate tier, first stays.
- A `DISABLED_` family is the only match of a filter: the executable
  says so, runs nothing, and exits zero, as H1 states (015
  `spec.md:550`, FR-036).
- A fixture instance and a plain family instance with equal names: one
  name, one duplicate error, first stays.
- A fixture method named `DISABLED_x` runs. The cited revision tests
  the prefix on the full instance name, and a fixture name starts with
  its class (`benchmark_register.cc:181`).
- A setup or teardown callback attached twice: the last attachment
  wins, one callback stored per slot.

## Requirements *(mandatory)*

### Argument families

- **FR-001**: The handle shall offer the family calls `arg`, `args`,
  `range`, `rangeMultiplier`, `ranges`, `denseRange`, `argsProduct`,
  `apply`, `argName`, and `argNames` in lowerCamelCase. Each call
  shall expand with the semantics of the cited revision.
- **FR-002**: The free functions `createRange` and `createDenseRange`
  shall build argument lists with the semantics of the cited
  revision. A family call shall accept a list they build.
- **FR-003**: Each family call shall expand into the instance set of
  the cited revision. Expansion, naming, and every allocation shall
  complete at the start of `speedgunMain`, before the filter and the
  first run. No expansion work shall happen during a run.
- **FR-004**: `range` and `ranges` shall grow by powers of the range
  multiplier, default 8, with both bounds included. Negatives follow
  the cited revision. `rangeMultiplier` shall set the multiplier for
  later range calls of that family.
- **FR-005**: `denseRange` shall step from the low bound to the high
  bound inclusive, default step 1.
- **FR-006**: Invalid family arguments shall be precondition
  violations enforced in source. They are a multiplier below 2, a low
  bound above a high bound, and a `denseRange` step below 1. An arity
  or label count unequal to the family arity is also a violation. The
  step check goes beyond the cited revision. Under the ignore
  semantic these checks emit no code, and a `denseRange` step of zero
  then never ends. (Q-2 adopted.)
- **FR-007**: A family that expands to more than 100 instances shall
  draw one warning with the bound of the cited revision. Its instances
  shall stay. (Q-3 adopted.)

### Argument access

- **FR-008**: The state shall report the argument count of the current
  instance and each argument by index through `range(index)`. A read
  shall allocate nothing.
- **FR-009**: The timed loop shall stay as H1 left it: no added work,
  no allocation, no lock, no contract check. The loop-shape gate of
  H1 shall stay green.

### Instance names

- **FR-010**: Each instance name shall start with the family name and
  add one `/`-joined segment per argument. A segment carries the
  `argName` label as `label:` where one is set. The form follows the
  cited revision.
- **FR-011**: The filter and list mode of H1 shall match instance
  names.
- **FR-012**: Two instances with one name shall be a recoverable
  error at the H1 duplicate tier, found at expansion. The instance of
  the earlier registration shall stay. The later instance shall not
  run, and every other instance shall run. The cited revision has no
  such check.

### Capture and templates

- **FR-013**: A macro shall register one function under a family name
  with captured arguments, as `BENCHMARK_CAPTURE` does. The capture
  name shall become a name segment in the form of the cited revision.
- **FR-014**: Macros shall register a function template instantiated
  over one or more type arguments, as `BENCHMARK_TEMPLATE` does. The
  type list shall enter the instance name in the stringified form of
  the cited revision.

### Fixtures

- **FR-015**: A fixture base class `sg::Fixture` shall carry virtual
  `setUp(State&)` and `tearDown(State&)` with empty defaults.
- **FR-016**: The fixture macros of F-2 shall exist under the `SG_`
  prefix: `SG_BENCHMARK_F`, `SG_BENCHMARK_DEFINE_F`,
  `SG_BENCHMARK_REGISTER_F`, `SG_BENCHMARK_TEMPLATE_F`,
  `SG_BENCHMARK_TEMPLATE_DEFINE_F`, `SG_BENCHMARK_TEMPLATE_METHOD_F`,
  and `SG_BENCHMARK_TEMPLATE_INSTANTIATE_F`. Each shall carry the
  definition, registration, and naming behavior of the cited
  revision.
- **FR-017**: Fixture `setUp` and `tearDown` shall run in the untimed
  region of each run, resting on the H1 FR-005 and FR-017 rule.

### Setup and teardown callbacks

- **FR-018**: The handle shall accept setup and teardown callbacks.
  Each shall run once around each run, the warm-up, calibration, and
  measured runs included, in the untimed region. The state handed to a
  callback shall carry the instance arguments.
- **FR-019**: This spec runs one thread. The once-per-thread-group
  rule stays with the threads spec of the roadmap.

### Disabled benchmarks

- **FR-020**: A family name with the `DISABLED_` prefix shall register
  and never run. The filter shall not match it and list mode shall not
  print it, as the cited revision treats it.

### Suite and case naming

- **FR-021**: Each instance shall carry a suite name and a case name.
  A fixture instance shall take the fixture class as its suite. The
  report shall print suites in the order of their first registered
  instance. Rows within one suite shall keep registration order. The
  pair of suite and case shall be unique for each instance. The JSON
  report of the roadmap shall reuse these two fields. (Q-1 adopted:
  the family name up to its first `/` is the suite.)

### Surface, version, and discipline

- **FR-022**: This spec shall extend the H1 surface and replace no H1
  signature. `registerBenchmark`, `SG_BENCHMARK`, `speedgunMain`, the
  H1 setters, and the H1 `State` methods shall keep their signatures.
- **FR-023**: Every new identifier shall follow V.1 rules N-1 through
  N-12, and every new macro shall take the `SG_` prefix. The name
  check shall report zero findings.
- **FR-024**: Every new interface shall document `\pre`, `\post`, and
  `\invariant`, and the source shall enforce each contract
  (Principle II).
- **FR-025**: Every time and counter value shall come from the
  counters library; the time-source gate shall stay clean.
- **FR-026**: The feature shall release version 0.7.0, the next minor
  above 0.6.0. `State` gains argument storage, so its layout changes.
  Under the hand-kept rule of `CMakeLists.txt:41-47`, the harness
  `SOVERSION` shall rise from 2 to 3. The counters archive keeps
  `SOVERSION` 2. The plan's version table shall record both. (Q-4
  adopted.)
- **FR-027**: `example/benchmark_example.cpp` shall gain one argument
  family, one fixture, and one templated benchmark. A new page,
  `docs/pages/harness.md`, shall hold one section for each capability
  C-1 through C-10.
- **FR-028**: The feature shall add no compiler extension and no
  runtime dependency.
- **FR-029**: The plan shall record TDD mode. Every test of this spec
  shall run on the fake provider, stay deterministic, and need no
  PMU.
- **FR-030**: All spec, plan, task, and code prose shall obey
  Principle XI, including XI.7, with zero prose gate findings.

## Key Entities *(include if feature involves data)*

- **Family record**: the H1 registry entry extended with the argument
  lists, the argument-name labels, and the range multiplier of one
  family. It also holds the setup and teardown callbacks. It exists
  before the first run and expands into instances.
- **Instance**: one point of a family: the argument vector, the
  instance name, the suite name, and the case name. It is what the
  filter selects, what the runner runs, and what the report prints.
- **State argument view**: the read-only view of the current
  instance's arguments carried by `State`, fixed at expansion and
  unchanged through a run.
- **Fixture**: a user class derived from `sg::Fixture`, contributing
  `setUp` and `tearDown` around each run and its class name as the
  suite of its instances.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Each family call of FR-001 and FR-002 expands to the
  instance set of the cited revision. A table test covers every call.
- **SC-002**: `range(index)` returns the instance argument in every
  run of that instance, and an instrumented read allocates nothing.
- **SC-003**: Instance names match the cited revision for each family
  form, capture, and template case. The filter selects exactly the
  matching instances.
- **SC-004**: A duplicate instance name is a recoverable error, and
  the first registration stays.
- **SC-005**: Fixture `setUp` and `tearDown` and the setup and
  teardown callbacks run once around each run. Scripted clock deltas
  show that their work adds nothing to the reported time.
- **SC-006**: A `DISABLED_` family runs no function, and list mode and
  the filter follow FR-020.
- **SC-007**: The report prints suites in the order of their first
  registered instance, and rows within one suite keep registration
  order.
- **SC-008**: Every hard gate of Principle VIII passes at the feature
  head, the time-source gate and the loop-shape gate included.
- **SC-009**: The version and `SOVERSION` follow FR-026, and the
  plan's version table records both.

## Scope

In: FR-001 through FR-030, a test for each capability C-1 through
C-10, and the version effect of FR-026.

Out, each owned by a later roadmap spec:

- threads and thread families, and timing modes;
- user counters, statistics, and chunked capture;
- JSON reports and the full command line;
- topology and execution contexts.

## Assumptions

- The recommendations of Q-1 through Q-4 stand as the working
  defaults, and the open questions below carry them to clarify.
- The argument-count method on the state is named `rangeCount()`; the
  request names `range(index)` and leaves the count unnamed, and
  `range_size` is not a V.2 exception.
- The family-size warning is checked where expansion completes: at
  expansion, at the start of `speedgunMain`. The cited revision checks
  it while resolving the filter, and H2 expands earlier.
- The case name of an instance is its instance name with the leading
  suite and its `/` removed. An instance name equal to its suite is
  also its case name. Each instance thus carries a unique pair of
  suite and case.
- A callback's state is a harness-built state carrying the instance
  arguments, mirroring the temporary state the cited revision hands
  the callbacks. It opens no sampling window.
- The template name form stringifies the type arguments exactly as
  written at the macro site, as the cited revision does; no
  demangling.
- The roadmap spec names in the Out list are the scope boundary. A
  later rename of a roadmap spec does not change this boundary.

## Dependencies

- The H1 harness surface at the audit point: the registry, the
  runner, the report, the filter and list mode, and the timed window
  of PC-2.
- The counters fake provider (`include/speedgun-ng/counters_fake.hpp`)
  as the test substitution of H1 FR-042.
- The constitution 2.18.0 gates: name check, prose, sanitizers,
  coverage, time source, and the loop-shape gate of H1.
- Google Benchmark is a reference read, nothing more. No vendored copy
  enters this spec, and the harness links nothing from it.

## Citations

File and line citations, read at the audit point `30f3118`.

| Artifact | Location |
| --- | --- |
| `registerBenchmark` | `include/speedgun-ng/benchmark.hpp:556` |
| `BenchmarkHandle` and setters | `benchmark.hpp:450,478,488,497,506,522` |
| `State`, cursor, window | `benchmark.hpp:140,149,173-179,237,256-266,277,354,365-381,400-408` |
| `SG_BENCHMARK` | `benchmark.hpp:582` |
| `speedgunMain` | `benchmark.hpp:576` |
| `sg::detail` use by H1 | `benchmark.hpp:30-33`, `source/harness/detail/internal.hpp:57` |
| `RegistryEntry`, `addRegistryEntry` | `source/harness/detail/internal.hpp:31-42,53` |
| `RunOptions::filter`, `listMode` | `internal.hpp:65-66` |
| `--filter`, `--list` contract | `specs/015-benchmark-harness-core/contracts/cli.md:11-12` |
| H1 FR-002 duplicate tier | `specs/015-benchmark-harness-core/spec.md:590` |
| H1 FR-005, FR-017 window | `spec.md:603,651` |
| H1 FR-042 fake substitution | `spec.md:807` |
| H1 SC-013 time gate | `spec.md:993` |
| H1 Google Benchmark revision | `spec.md:1111` |
| Version, `SOVERSION` rule | `CMakeLists.txt:7,41-47` |
| Harness archive `SOVERSION` | `CMakeLists.txt:211-218` |
| Package compatibility | `cmake/install-rules.cmake:39` |
| Existing documentation pages | `docs/pages/` (no harness page at the audit point) |
| Constitution V.1, N-6, V.2 | `.specify/memory/constitution.md:533-580,555-556,583-634` |
| Constitution VIII gates, time-source item | `constitution.md:657-699,691` |
| Counters-only rule | `constitution.md:1068-1092` |
| Constitution version footer | `constitution.md:1149` |
| `FakeProvider`, `FakeScript` | `include/speedgun-ng/counters_fake.hpp:102,43` |
| Fake scripted harness tests | `test/source/harness_calibration_test.cpp:7,252`, `test/source/harness_capture_test.cpp:362` |
| Loop-shape gate | `test/loop_shape.sh` (IF-04; 015 FR-004, FR-005, R-06) |
| Time-source gate | `test/time_source_gate.sh` |
| Google Benchmark revision | `google/benchmark` `main` at `e662de9aab8e705ecf4fa4bd41a207e5a0acfd0c` (2026-10-08 15:52:18Z) |
| GB semantics table | the table of the reference read section above |

## Open questions

The request carries four questions to clarify. Each has a
recommendation, the spec adopts the recommendation as the working
default, and clarify confirms or replaces it.

- Q-1 Suite and case source: Google Benchmark has no suite field.
  Recommendation: the family name up to its first `/` is the suite,
  and the fixture class is the suite for a fixture instance. Adopted
  in FR-021.
- Q-2 Invalid family arguments, for example a multiplier below 2 or a
  low bound above the high bound: Google Benchmark aborts.
  Recommendation: a `SG_REQUIRE` precondition, since the values come
  from code. Adopted in FR-006.
- Q-3 Large families: Google Benchmark warns above 100 instances in
  one family. Recommendation: keep the warning with the same bound.
  Adopted in FR-007.
- Q-4 Version: Recommendation: the next minor version, and `SOVERSION`
  up by one when `State` or `BenchmarkHandle` changes layout. Adopted
  in FR-026.
