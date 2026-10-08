# Feature Specification: Benchmark Harness Core

**Feature Branch**: `015-benchmark-harness-core`

**Created**: 2026-10-08

**Status**: Draft

**Input**: User description, opening quoted, the full text held by the
specify invocation record:

<!-- prose-lint: allow reason="XI.5 verbatim quotation of the request, marked as quoted" -->
"Benchmark harness core, the first spec of the speedgun harness
roadmap (spec H1). Spec 007 shipped the counters library as standalone
classes and deferred the benchmark harness and its registration API to
a future spec (007 Assumptions, deferred list). Spec 007 FR-050 bound
those classes to stay embeddable in fixed-iteration, per-thread
benchmark loops. The README describes the project as a C++23
benchmarking framework in the spirit of Google Benchmark, and no part
of the tree runs a benchmark yet. [...] This spec delivers the first
usable vertical slice. A user writes a benchmark, runs it
single-threaded through the speedgun command line, and reads ns per
iteration and a PMU metric with full disclosure on the console.
Principle VI states that a wrong benchmark is worse than no benchmark,
and every requirement below serves that rule."

## Clarifications

### Session 2026-10-08

- Q: When a benchmark fixes the iteration count and a minimum warm-up time is also set, does the harness still run warm-up? → A: Warm-up still runs. It starts at the fixed count and grows by the calibration rule against the minimum warm-up time, within the run bound. The measured phase then runs the fixed count.
- Q: How should the harness treat a SIGINT that arrives while a benchmark is running? → A: The current run completes. The harness then reports the benchmark as skipped with an interrupt reason, releases its resources, starts no later run, and exits nonzero. The timed loop reads no interrupt flag.
- Q: What exit status does the executable return when the filter matches no benchmark, and in list mode? → A: Both exit zero; nonzero stays reserved for a failed benchmark, a failed catalog listing, or an interrupt.
- Q: How should the command line handle an invalid numeric value, such as a zero, negative, or unparseable iteration count or time? → A: Report a recoverable error at the 007 FR-046 tier, run nothing, and exit nonzero. A zero minimum warm-up time stays valid, as the FR-007 default requires.

## Audit point

| Field | Value |
| --- | --- |
| Audit point | `d6bcbb54ff1103843426d9ddc2ccb2d5a8a1babe` |
| Audit-point date | 2026-10-08 14:00:44 -0500 |
| Default branch at run time | `master`, tip `d6bcbb5` |
| Working tree at run time | `master` at `d6bcbb5` |
| Feature number at run time | 015, the next free number under `specs/` |
| Short name at run time | `benchmark-harness-core` |
| Project version at the audit point | 0.5.0 |
| `SOVERSION` at the audit point | 2 |
| Package compatibility at the audit point | `SameMinorVersion` |
| Constitution version at the audit point | 2.17.0, lineage table and footer equal |

The audit read the tree of `d6bcbb5`, the `master` commit that merged
pull request 31. The five counters changes named in the request sit on
`master`. They are 012, 013 with its 0.4.1 patch, the 0.4.1 version,
014, and the post-merge rename repair of pull request 30. Pull request
31 adds six follow-up commits. Three are the constitution footer
correction, the recorder member rule labels, and the dbc macro-local
renames. Three are the Windows clock local renames, the test marker
local rename, and the map citation fix. Every citation below was read
at `d6bcbb5`.

The request binds no spec number, commit, version, or source line.
Each value in this table was resolved at run time, as instructed.

## Preconditions

Each precondition was checked in the code and the cited artifacts at
the audit point. The result and its evidence follow.

### PC-0: the naming law is in force. PASS

- `specs/014-identifier-naming-camelcase/` is merged, and the
  post-merge repair is the `master` tip.
- Constitution Principle V.1 states the naming rules N-1 through N-12.
  V.2 states the closed exception list.
- Principle VIII carries the name-check gate:
  `readability-identifier-naming` reports zero findings, and
  `WarningsAsErrors` in `.clang-tidy` (line 21) names that check, so a
  finding fails the build.

This feature starts on the renamed tree under that law.

### PC-1: the counters corrections are on the default branch with every task complete. PASS

- `specs/012-counters-defect-resolution/` is merged. Its `tasks.md`
  checkpoint records: "every task in this artifact is closed."
- `specs/013-counters-defect-followup/` is merged. Its `plan.md`
  records the 0.4.1 patch (lines 152-153 and the version table row at
  line 417).
- The 014 rename and its repair are merged. The version stands at
  0.5.0 with `SOVERSION` 2, so the 0.4.1 patch and the later minor
  both landed.

### PC-2: the sample path writes raw points alone. PASS

- `RecorderHandle::sample()`
  (`include/speedgun-ng/counters_measurement.hpp:504`) appends one
  cumulative point per managed leaf into a preallocated column. Its
  contract states zero allocation and zero lock (007 FR-011, FR-026).
- `PointSink::put` (`include/speedgun-ng/counters_provider.hpp:214`)
  receives one `std::uint64_t` alone.
- Expression math runs at fold time, never at sample time (007
  FR-021). The capture model of this spec depends on this fact, and
  the fact holds.

### PC-3: availability is one countability enumeration with separate refusal kinds. PASS

- `Availability` (`include/speedgun-ng/counters_core.hpp:87`) holds
  `COUNTABLE`, `PERMISSION_BLOCKED`, `NOT_ENCODABLE`, `ABSENT`,
  `SCOPE_REFUSED`, and `GAP`. The scope refusal and the encoding
  refusal carry separate values (012 FR-021, I-03).
- `TargetMask` (`:107`) is a fixed-size `std::uint32_t` beside it, with
  `kTargetThreadBit` (`:110`) and `kTargetCpuBit` (`:113`).

### PC-4: a gap in a window reaches a fold through a managed disclosure column. PASS

- `PointSink::put` keeps its one-integer signature
  (`counters_provider.hpp:214`), and `PointSink::putDisclosure`
  (`:240`) writes the disclosure column (012 clarify decision, I-04,
  I-05).
- A fold result carries the state in `MetricResult::availability`
  (`counters_core.hpp:285`), and a raw view carries it in
  `PointsView::availability` (`counters_measurement.hpp:426`), with
  `Availability::GAP` for a gap.

### PC-5: the per-plan overhead floor excludes the bracketing clock, and a CI test covers the calibration. PASS

- `calibrate()` (`source/counters/plan.cpp:264`) measures the bracket
  of two clock reads with no sampling action between them and
  subtracts it from every cost: "The published floor excludes it"
  (012 FR-025, I-09). The subtraction clamps at zero, so the published
  duration is never negative.
- 012 FR-026 and SC-009 bind a registered test to the calibration, and
  012 FR-027 holds the coverage-exclusion count down.

### PC-6: destroying a plan, or failing to open one, releases every descriptor and mapping it acquired. PASS

- `Plan::~Plan` (`source/counters/plan.cpp:185`) releases the compiled
  layout and the provider windows it owns.
- 012 FR-013, FR-014, and FR-015 state the three cases (I-08), and
  012 SC-004 counts real descriptors and mappings across 10,000
  open-and-destroy cycles. Failure isolation depends on this, and the
  fact holds.

### PC-7: recorder reuse across repetitions stays with the harness. PASS

Both facts confirmed at the audit point:

- `Plan::recorder(capacity)` (`counters_measurement.hpp:925`) allocates
  the arena from the plan at minting, and the contract states that the
  plan outlives its recorders. The overload
  `Plan::recorder(capacity, Ring)` sits at `:936`.
- `RecorderHandle` has no reset. The header names none.
- 012 FR-009 and the clarify decision at
  `specs/012-counters-defect-resolution/spec.md:134-137` assign reuse
  to the harness specification and hold 012 to no recorder change.

D-2 rests on these facts. They hold, so D-2 stands unchanged.

### PC-8: the tsc leaf requires one thread to take both window endpoints. PASS

- `include/speedgun-ng/counters_clock.hpp:53-59` states the
  precondition on the caller and names the harness as the caller that
  meets it (012 FR-031, I-10). The leaf gained no fence.
- This spec runs one thread, so the rule holds by construction.

### PC-9: each object publishes its catalog through `Object::counters()`. PASS

- `Object::counters()` (`include/speedgun-ng/counters_system.hpp:94`)
  returns `std::vector<CatalogEntry>`.
- `CatalogEntry` (`counters_core.hpp:246`) carries `name`,
  `description`, `unit`, the availability state in `avail`, the read
  mode in `mode`, and the target mask in `targets`. The record also
  carries `frequencyHz` and `scaled`, two fields beyond the request's
  enumeration. The listing requirement (FR-037) prints the fields the
  request names; the plan decides whether the two extras join the
  line.

### PC-10: the rename left no old spelling on the harness path. PASS, with three recorded hits

The scan of owned code at the audit point found exactly the three gaps
the request predicted, and no others:

- `sgCtReject` in the always-on contract macros of
  `include/speedgun-ng/dbc.hpp:419-503`, eight sites. N-8 gives a
  named constant the `k` prefix; the spelling lacks it.
- `reached_after` at `test/source/dbc_test.cpp:491` and `:498`.
- The call `noexcept_violator()` at `test/source/dbc_test.cpp:493`, in
  the non-Unix arm. The function that arm calls is `noexceptViolator`
  at `:476`, so the arm names a function that does not exist. Linux is
  the supported platform, and the arm does not compile there.

None lies on the harness path, so none blocks this feature. Each hit
is a dependency on a separate fix (X.3), recorded under Dependencies.
The harness copies no old spelling and adds no new one.

Name-check question: the name check does not report `sgCtReject` in a
harness translation unit. Commit `4a3e0fe` on master records the
mechanism:
clang-tidy sees the tokens of a macro body without resolving the
identifiers in it. The tree at the audit point passes the Principle
VIII name gate with `sgCtReject` present, and the harness expands
those macros. FR-048 carries the fact.

The rename map resolves older spellings. It sits at
`specs/014-identifier-naming-camelcase/rename-map.md` and
`docs/pages/identifier-rename-map.md`. Both exist at the audit point.

### Name confirmation

Every library name the request uses was confirmed at the audit point
in the post-rename spelling: `RecorderHandle::sample`, `PointSink::put`,
`PointSink::putDisclosure`, `MetricResult::availability`,
`PointsView::availability`, `Plan::recorder`, `Object::counters`,
`CatalogEntry`, `Availability`, `TargetMask`, `kTargetThreadBit`,
`kTargetCpuBit`, `Plan::sampleOverheadNsMedian`, `ClockProvider`, and
the fake provider in `include/speedgun-ng/counters_fake.hpp`.

## Decisions

The maintainer settled these choices before specify ran. Each is
recorded as a decision with its reason.

### D-1: executable shape

Each suite links the library target that provides `speedgunMain` and
becomes one speedgun executable. The harness loads no suite as a
shared object. The library ships as a static archive (007 FR-049).
Reason: one link line, no loader path, and the archive already carries
the counters surface.

### D-2: recorder sizing

The harness mints one recorder per benchmark, and the calibration
bound of D-3 sizes it before the first run. The counters library gains
no recorder reset, so this decision changes no counters signature and
no `SOVERSION`. The chunked-capture spec of the roadmap owns recorder
reuse (012 FR-009 and its clarify decision).

### D-3: run-control defaults and calibration

The harness takes the Google Benchmark defaults and growth rule. The
revision read: `google/benchmark`, branch `main`, commit
`e662de9aab8e705ecf4fa4bd41a207e5a0acfd0c`, dated 2026-10-08
15:52:18Z. At that revision the sources state these values:

- the minimum time is 0.5 s (`kDefaultMinTimeStr` in
  `include/benchmark/benchmark_api.h`);
- the minimum warm-up time is 0 s, and the repetition count is 1
  (`benchmark_min_warmup_time` and `benchmark_repetitions` in
  `src/benchmark.cc`);
- calibration starts at 1 iteration;
- a run qualifies on any of five conditions
  (`ShouldReportIterationResults` in `src/benchmark_runner.cc`): the
  benchmark skipped; the run is a dry run; the iteration count reached
  the cap; the decision time reached the minimum time; the real time
  reached 5 times the minimum time;
- the decision time is the thread CPU time of the run, unless the
  benchmark selects real time or manual time;
- a run that does not qualify sets a growth factor
  (`PredictNumItersNeeded`): 1.4 times the minimum time divided by the
  decision time, the divisor floored at 1 ns; a run whose decision
  time is 10 percent of the minimum time or less takes a factor of 10;
  the next count is the rounded product, or the old count plus 1
  iteration where that is larger; the iteration cap of 10^12
  iterations limits it (`kMaxIterations`);
- warm-up uses the same rule against the minimum warm-up time, and it
  starts from the fixed count where the benchmark sets one
  (`RunWarmUp`); the measured phase discards the warm-up count and
  starts again from its own start count;
- only the first repetition calibrates, and each later repetition
  reuses its count (`DoOneRepetition`);
- an explicit iteration count skips calibration; a dry run takes 1
  iteration, 1 repetition, and no warm-up.

Run bound. A non-qualifying run has a decision time below the
minimum time, so its factor exceeds 1.4 before rounding, or it is 10.
With rounding, the realized factor never falls below 4/3 once the
count reaches 3. From 1 iteration, two growth steps reach 3, and 93
more pass the 10^12 cap: 3 times (4/3)^93 exceeds 10^12. Each phase
therefore holds at most 96 runs: the start run and 95 growth steps.
A benchmark runs at most 96 warm-up runs, 96 calibration runs in the
first repetition, and one measured run in each later repetition. The
recorder capacity of D-2 is 2 × (96 + 96 + R) sampling actions for a
repetition count R. FR-019 fixes it before the first run.

Every time the rule reads comes from the counters library (FR-038).

### D-4: optimization barriers

`doNotOptimize` is one empty extended-assembly statement per overload,
in the form `DoNotOptimize` uses in `include/benchmark/utils.h` at the
revision read: the statement names the value as an input and output
operand with a register-or-memory constraint and a memory clobber. The
harness follows the overloads Google Benchmark keeps and omits the
const reference overload that Google Benchmark marks deprecated. The
plan records the P2 justification under Principle I, on the precedent
of the `__asm__` statement in the 011 marker
(`specs/011-nanosecond-counter-ssc-mark/plan.md`, Constitution Check).
`clobberMemory` follows `ClobberMemory` at the revision read: the
standard fence `std::atomic_signal_fence(std::memory_order_acq_rel)`,
which needs no extension.

### D-5: version

The feature releases 0.6.0, the next minor version above the 0.5.0
audit-point value, and keeps `SOVERSION` at its audit-point value of
2. The harness adds types and functions and changes no existing
signature. A change to an existing public signature or record layout
raises `SOVERSION` by one, under the hand-kept rule stated in
`CMakeLists.txt` (lines 41-47). The package config keeps
`SameMinorVersion` (`cmake/install-rules.cmake:39`).

### D-6: counters-only rule in the constitution

This feature amends `.specify/memory/constitution.md`, and the rule
binds every later spec.

- Placement: a new entry in Additional Constraints, beside the
  Library-first entry. That entry sits in Additional Constraints at
  the audit point.
- Rule text: every time and counter value that speedgun code outside
  the counters library measures or reports comes from the
  `sg::counters` library. A plan, a recorder, a fold, or a value a
  plan publishes supplies it.
- Scope: the rule binds every C++ source and header under `source/`,
  `include/`, `example/`, `test/`, and `tools/`. Code under `external/`
  and the counters library sit outside it. The counters library is
  `source/counters/`, the `include/speedgun-ng/counters*.hpp` headers,
  and the `counters_` tests and gate scripts under `test/`.
- Banned list: the entry names the direct time sources that code
  outside the counters library shall not call. The list is the one in
  FR-040. It covers the `std::chrono` clocks, `std::clock`,
  `std::time`, `timespec_get`, `clock_gettime`, `clock_getres`,
  `gettimeofday`, `time`, `times`, and `getrusage`. It also covers the
  `rdtsc` and `rdtscp` instructions, their intrinsics, and the headers
  that declare them.
- Missing capability: a need the counters library does not meet
  becomes a counters-library change. No code outside the library
  bypasses it.
- Exceptions: an exception needs a constitutional amendment. This
  amendment names one. `tools/dbc/overhead.cpp` keeps the `<chrono>`
  include and `std::chrono::steady_clock`. It measures the contract
  overhead against an independent, well-known reference clock on
  purpose. The exception covers those two terms in that file alone. A
  spec, a plan, a local override, or a suppression comment creates no
  exception.
- Gate: the Principle VIII gate list gains a hard CI gate. The
  time-source gate of SC-013 scans the code the rule binds and allows
  the named exception alone. A hit fails the build.
- Procedure: the amendment follows the Governance section at the audit
  point. It records the rationale and bumps the version. It writes a
  new Sync Impact Report at the head of the file, adds a lineage row,
  and sets the Last Amended date.
- Version: the lineage table ends at 2.17.0 at the audit point, and
  the version footer equals it. The amendment takes the next MINOR
  above the table at the audit point, 2.18.0 from 2.17.0, for a new
  constraint and a new gate under Governance. The amendment sets the
  footer to the new version.

## Roadmap position

This is spec H1 of the speedgun harness roadmap. The harness is its
own runner. It reaches the full Google Benchmark feature set over
several specs, and it measures metrics with the counters library. A
metric is the fold of counter points between two recorded points: time
in ns, instructions, cycles, and instructions per cycle. The roadmap
document sits outside this repository and is absent from this machine
at the audit point; the scope boundary below is transcribed from the
request, and the roadmap spec names are the boundary.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Write a benchmark, run it, read the numbers (Priority: P1)

A user writes a benchmark function, registers it under a name, links
the suite against the library, and runs the executable with no
options. The harness calibrates the iteration count, runs the timed
loop, and prints one row with real time per iteration in ns. The user
attaches a PMU metric, instructions per cycle for example, and the
row carries that value with its running ratio, its scaled flag, and
its gap state beside it. Every row also carries the plan's median
sampling overhead floor in ns.

**Why this priority**: this is the vertical slice. Nothing else in the
feature has value until a user can produce and read one correct
number.

**Independent Test**: compile the example suite, run it with no
options, and read the report. The run delivers ns per iteration and a
PMU metric with full disclosure on its own.

**Acceptance Scenarios**:

1. **Given** a registered benchmark, **When** the executable runs with
   no options, **Then** the report carries a row with real time per
   iteration in ns, measured through the counters library.
2. **Given** the same run with a metric attached, **When** the host
   publishes countable leaves for it, **Then** the row carries the
   metric value with its running ratio, scaled flag, and gap state.
3. **Given** any measured row, **When** the user reads it, **Then**
   the row carries the plan's median sampling overhead in ns and
   states that each run window includes the cost of its two endpoint
   samples.

---

### User Story 2 - Control the run from the command line (Priority: P2)

A user filters benchmarks by a regular expression, lists the matching
names without running them, sets repetitions, sets a minimum time in
seconds or an explicit iteration count, sets a minimum warm-up time,
asks for a dry run, and attaches catalog leaves by address to every
selected benchmark.

**Why this priority**: the slice in US1 works but is rigid. Run
control turns it into a tool a user aims at one benchmark among many.

**Independent Test**: run one executable holding several benchmarks
with each option, and check the selected set, the run counts, and the
discarded warm-up and calibration results.

**Acceptance Scenarios**:

1. **Given** three registered benchmarks, **When** the user passes a
   filter that matches two, **Then** exactly those two run.
2. **Given** the same filter, **When** the user adds list mode,
   **Then** the executable prints the two names and runs no benchmark
   function.
3. **Given** a benchmark with no fixed count, **When** the user passes
   an explicit iteration count, **Then** calibration is skipped and
   the run uses that count.
4. **Given** any selection, **When** the user asks for a dry run,
   **Then** each benchmark executes one iteration and one repetition,
   with no warm-up.
5. **Given** a benchmark that sets its own minimum time, **When** the
   command line also sets one, **Then** the benchmark's value wins.

---

### User Story 3 - Find a counter address in the catalog listing (Priority: P2)

A user runs a built-in option of any speedgun executable. The listing
prints every counter the running host publishes: the object path, the
counter name, the description, the unit, the read mode, and the
availability state, with the refusal kind for a leaf that cannot
count. The executable then exits without running a benchmark. The
user copies an address from the listing into the counter option of
US2.

**Why this priority**: the PMU half of US1 is unusable until the user
can name a leaf the host can count.

**Independent Test**: run the listing option against a fake-provider
system with a known leaf set, and against the reference host, and
check every field on every line.

**Acceptance Scenarios**:

1. **Given** a host with core PMU leaves, **When** the user runs the
   listing option, **Then** each line carries the six fields and the
   refusal kind where the leaf cannot count.
2. **Given** the listing option, **When** it runs, **Then** no
   benchmark function executes and the exit status is zero.

---

### User Story 4 - Skips and failures keep the run alive (Priority: P2)

A benchmark skips with an error or with a message, and the report
states the reason and prints no statistics. A benchmark function
throws, and the harness marks that benchmark failed with the exception
text, releases every plan, recorder, descriptor, and mapping the
benchmark held, and continues with the next benchmark.

**Why this priority**: Principle VI. A harness that leaks a failed
benchmark or hides its reason hands the user a wrong or missing
number.

**Independent Test**: run a suite whose benchmark throws on every run,
and check the descriptor and mapping counts of the process before and
after, then check that the next benchmark still runs and reports.

**Acceptance Scenarios**:

1. **Given** a benchmark that skips with a message, **When** the run
   finishes, **Then** the report states the reason and prints no
   statistics for it.
2. **Given** a benchmark that throws, **When** the run finishes,
   **Then** the report marks it failed with the exception text, the
   process holds no leftover descriptor or mapping from it, and the
   next benchmark runs.

---

### User Story 5 - Statistics over repetitions (Priority: P2)

A user sets repetitions greater than one. Each repetition produces one
result row. The harness then prints aggregate rows with the mean,
median, standard deviation, coefficient of variation, min, and max for
time and for each metric.

**Why this priority**: one run of one benchmark is an anecdote. The
distribution form of Principle VII is what makes the number
trustworthy.

**Independent Test**: feed a fixture with a known set of repetition
results and match every aggregate exactly.

**Acceptance Scenarios**:

1. **Given** five repetitions, **When** the report prints, **Then**
   five repetition rows and the aggregate rows appear in fixed
   columns.
2. **Given** a known result set, **When** the aggregates print,
   **Then** mean, median, standard deviation, coefficient of
   variation, min, and max each equal the fixture value.

---

### User Story 6 - Barriers keep the measured work (Priority: P3)

A user calls `doNotOptimize` on the value a benchmark computes, and
the compiler keeps the work. A user calls `clobberMemory` where the
work is a store. The documentation states when a barrier is justified.

**Why this priority**: the slice works without barriers for
side-effecting code, and measuring pure computation needs
them.

**Independent Test**: an instrumented or compile-observed test shows
the measured work survives optimization with the barrier and falls to
a constant without it.

**Acceptance Scenarios**:

1. **Given** a benchmark that discards a computed value, **When** it
   calls `doNotOptimize` on the value, **Then** the `-O2` disassembly
   of GCC and Clang keeps it.
2. **Given** the same benchmark without the barrier, **When** GCC and
   Clang compile it at `-O2`, **Then** neither disassembly holds the
   computation.

---

### Edge Cases

- A registration repeats a name: the harness reports a recoverable
  error at the tier 007 FR-046 gives a registration duplicate, keeps
  the first registration, and runs the rest.
- A filter matches nothing: the executable says so, runs nothing, and
  exits zero.
- A leaf a metric needs cannot count: the metric is marked unavailable
  with its refusal kind, and the rest of the benchmark runs (PC-3).
- A run window carries a gap: the value reports as unavailable, and
  the statistics exclude the run (PC-4).
- A host has no countable `cpu/instructions` leaf: the example suite
  still reports ns per iteration, reports the metric unavailable with
  its refusal kind, and exits zero.
- A benchmark function throws on every run: each run releases its
  resources, and the process descriptor and mapping counts return to
  their start values (PC-6).
- A benchmark skips: the reason prints, and no statistics print.
- SIGINT arrives mid-run: the current run completes, and no later run
  starts. The benchmark reports as skipped with an interrupt reason,
  no resource is left held, and the exit status is nonzero.
- A benchmark fixes N: calibration is skipped entirely. A set minimum
  warm-up time still runs warm-up, which starts at N and grows by the
  FR-010 rule within the FR-016 bound. The harness discards every
  warm-up result.
- A dry run: one iteration, one repetition, no warm-up.
- An option carries an invalid numeric value: the executable reports a
  recoverable error, runs no benchmark, and exits nonzero.
- A host whose clock costs more than the sampling action: the overhead
  floor clamps to 0 ns, and the published duration stays non-negative.
- The `machine/tsc` leaf is absent on non-x86 builds: the listing
  shows the catalog fact with no API difference.
- The `machine/tsc` leaf carries no published rate: the harness
  reports it as a count and derives no time from it.
- A metric needs a leaf the catalog lacks: the harness reports the
  resolution failure as a recoverable error, at the 007 FR-046 tier.

## Requirements *(mandatory)*

### Registration

- **FR-001**: The harness shall register a benchmark function under a
  unique name. A macro shall register a function at namespace scope
  before `main` begins. A runtime function shall register any callable
  from any code that runs before the run starts.
- **FR-002**: When a registration repeats a registered name, the
  harness shall report a recoverable error at the tier 007 FR-046
  gives a registration duplicate, shall keep the first registration,
  and shall run the rest.
- **FR-003**: Registration shall return a handle that sets the
  run-control options of FR-007 through FR-015 and the metric options
  of FR-021.

### State and timed loop

- **FR-004**: The harness shall call the benchmark function with a
  state object. The function shall run its setup, run the timed loop
  as `for (auto _ : state)`, and run its teardown.
- **FR-005**: Setup and teardown shall run in the untimed region (007
  FR-050). The state shall report the iteration count of the current
  run.

### Run control

- **FR-006**: A run shall be one pass of the timed loop over N
  iterations.
- **FR-007**: The run-control defaults shall be: minimum time 0.5 s;
  minimum warm-up time 0 s; repetitions 1; calibration start 1
  iteration. Each default carries its unit (D-3).
- **FR-008**: A run shall qualify when any of five conditions holds:
  the benchmark skipped; the run is a dry run; the iteration count
  reached the cap of 10^12 iterations; the decision time reached the
  minimum time; the real time reached 5 times the minimum time.
- **FR-009**: The decision time of a run shall be the thread CPU time
  of that run. This spec offers no selection of real time or manual
  time; the D-3 clause about selection binds when the timing-modes
  spec of the roadmap lands.
- **FR-010**: A run that does not qualify shall set the next iteration
  count by the growth rule of D-3: the factor is 1.4 times the minimum
  time divided by the decision time, with a divisor floor of 1 ns; the
  factor is 10 when the decision time is 10 percent of the minimum
  time or less; the next count is the rounded product, or the old
  count plus 1 iteration where that is larger; the cap limits it.
- **FR-011**: Warm-up shall use the same rule against the minimum
  warm-up time. The measured phase shall discard the warm-up count and
  start again from its own start count. The harness shall discard
  every warm-up result. A set minimum warm-up time shall still produce
  warm-up for a benchmark that fixes N. That warm-up shall start at N
  and grow by the FR-010 rule within the FR-016 bound. The measured
  phase shall then run N iterations.
- **FR-012**: Only the first repetition shall calibrate. Each later
  repetition shall reuse the count the first repetition settled.
- **FR-013**: When a benchmark fixes N directly, the harness shall
  skip calibration and run N iterations. The skip reaches calibration
  alone; the FR-011 warm-up rule still applies.
- **FR-014**: A dry run shall execute 1 iteration, 1 repetition, and
  no warm-up.
- **FR-015**: A command-line value shall apply where the benchmark
  sets none. A value the benchmark sets shall win over the command
  line.
- **FR-016**: Each warm-up phase and each calibration phase shall stay
  within the bound derived in D-3: at most 96 runs from 1 iteration to
  the cap.

### Raw capture and post-processing

- **FR-017**: The harness shall sample once at the entry of the timed
  loop and once at its exit. Each run shall add exactly two raw points
  to the recorder. The timed loop shall do no other counter work.
- **FR-018**: All metric math shall run after the run, through a fold
  over the two points of each run (007 FR-018, FR-021). Each
  calibration and warm-up decision shall read its run through a fold
  over that run's two points.
- **FR-019**: The harness shall fix the recorder capacity from the
  run bound of FR-016 before the first run, and shall mint the
  recorder in the untimed region (007 FR-050, D-2). One recorder
  serves one benchmark.

### Metrics

- **FR-020**: Every benchmark shall report real time per iteration,
  measured on the `machine/monotonic` leaf through the counters
  library.
- **FR-021**: A benchmark shall attach counter expressions built with
  the 007 C++ arithmetic, for example instructions / cycles. A
  command-line option shall attach catalog leaves by address, for
  example `cpu/instructions`, to every selected benchmark.
- **FR-022**: A metric of dimension events^1 or time^1 shall report
  per iteration: its window value divided by N. A metric of any other
  dimension shall keep its window value. Examples are a dimensionless
  ratio, such as instructions per cycle, and an events-per-time rate
  (007 design journal, section "Harness seam notes").
- **FR-023**: Before the run, the harness shall read the availability
  of every leaf each metric needs (PC-3). A leaf that cannot count
  shall mark the metric unavailable with its refusal kind, and the
  rest of the benchmark shall run.

### Disclosure and overhead floor

- **FR-024**: Every reported metric value shall carry its running
  ratio, its scaled flag, and its gap state (007 FR-019, PC-4).
- **FR-025**: A run with a gap shall report the value as unavailable,
  and shall add no sample to the statistics.
- **FR-026**: Every result row shall carry the plan's median sampling
  overhead in nanoseconds (007 FR-032, PC-5). The row shall state that
  each run window includes the cost of its two endpoint samples.

### Statistics

- **FR-027**: For one repetition the result shall be the fold of that
  repetition's measured run. Over the repetitions the harness shall
  report mean, median, standard deviation, coefficient of variation,
  min, and max for time and for each metric. This is the min, median,
  and max distribution form of Principle VII.
- **FR-028**: The percentile form of Principle VII is out of this
  spec; the chunked-capture spec of the roadmap owns it, and this spec
  records the limit.

### Optimization barriers

- **FR-029**: The harness shall provide `doNotOptimize` and
  `clobberMemory` with the semantics of the Google Benchmark barriers
  at the revision read, in the form D-4 states.
- **FR-030**: The documentation shall state the Principle X.2 rule: a
  barrier stands only where the compiler would otherwise eliminate the
  measured work.

### Skips and failures

- **FR-031**: The state shall provide a skip with an error and a skip
  with a message. A skipped benchmark shall report its reason and no
  statistics.
- **FR-032**: When the benchmark function throws, the harness shall
  mark that benchmark failed with the exception text, shall release
  every plan, recorder, descriptor, and mapping the benchmark held,
  and shall continue the run (PC-6; the 007 design journal lists
  unwind-clean construction resources as a harness acceptance item,
  section "Harness seam notes"). When SIGINT arrives, the handler
  shall set one flag. The harness shall read the flag after each run.
  The current run shall complete, and the timed loop shall read no
  flag. On a set flag, the harness shall report the current benchmark
  as skipped with an interrupt reason. It shall start no later run,
  release the same resources, and return a nonzero exit status.

### Command line and console report

- **FR-033**: The library shall ship an entry function, `speedgunMain`,
  that parses the command line with `getopt_long`. A suite shall link
  the CMake target that provides it and become one speedgun executable
  (D-1). The target name follows the existing target naming in
  `CMakeLists.txt`; target names are outside the C++ identifier set,
  and the naming rules do not reach them.
- **FR-034**: The command-line options of this spec shall be: a name
  filter by regular expression; a list mode that prints the matching
  benchmark names and runs nothing; a repetition count; a minimum time
  in seconds; an explicit iteration count; a minimum warm-up time; a
  dry run of one iteration and one repetition; the counter
  leaves of FR-021; and the catalog listing of FR-037. An invalid
  option value shall be a recoverable error at the 007 FR-046 tier: the
  executable shall report it, run no benchmark, and exit nonzero.
  Invalid values are an unparseable or negative count or time, and a
  zero repetition count, iteration count, or minimum time. A zero
  minimum warm-up time is valid, as the default of FR-007 requires.
- **FR-035**: The console report shall print one row per repetition
  and one row per aggregate in fixed columns. Context lines above the
  rows shall give the host, the cpu, the library version, and the
  build type. A measured value in a context line shall come from the
  counters library (FR-038). A value the library does not publish
  stays out of the line.
- **FR-036**: The executable shall return a nonzero exit status when a
  benchmark fails, when the catalog listing fails, or when SIGINT
  interrupts a run. A filter that matches no benchmark and a list
  mode run shall exit zero. The harness shall expose the results as a
  C++ value, and the console report shall format that value.

### Catalog listing

- **FR-037**: Every speedgun executable shall have a built-in option
  that lists every counter the running host publishes and then exits
  without running a benchmark. Each line shall give the object path,
  the counter name, the description, the unit, the read mode, and the
  availability state, with the refusal kind for a leaf that cannot
  count (PC-3, PC-9). A user finds the address for the counter option
  of FR-021 in this listing. Metrics join the listing in the
  metric-formulas spec of the roadmap.

### Counters as the only time source

- **FR-038**: Every time and counter value the harness measures or
  reports shall come from the counters library: a plan, a recorder, a
  fold, or a value the plan publishes, for example
  `sampleOverheadNsMedian()`. The rule covers the real time per
  iteration, the thread CPU time that calibration reads, every
  calibration, minimum-time, and warm-up decision, the overhead floor,
  and every PMU metric. It also binds the CPU time and process CPU
  time of the timing-modes spec of the roadmap.
- **FR-039**: The clock leaves at the audit point:
  `machine/monotonic`, `machine/thread_cpu`, `machine/process_cpu`,
  and `machine/monotonic_raw` on every supported build;
  `machine/tsc` on x86 builds alone. Real time uses
  `machine/monotonic`. Thread CPU time uses `machine/thread_cpu`.
  Process CPU time uses `machine/process_cpu`. The tsc leaf is a tick
  count with no published rate, so the harness reports it as a count
  and derives no time from it.
- **FR-040**: No code the D-6 rule binds shall call a time source
  outside the counters library. The banned set is the constitutional
  list of D-6:
  the `std::chrono` clocks, `std::clock`, `std::time`, and
  `timespec_get`; `clock_gettime`, `clock_getres`, `gettimeofday`,
  `time`, `times`, and `getrusage`; the `rdtsc` and `rdtscp`
  instructions and their intrinsics; and the headers that declare
  them. This requirement is the gate form of FR-038. The gate scans
  every C++ source and header the D-6 scope names. It allows the
  `<chrono>` include and `std::chrono::steady_clock` in
  `tools/dbc/overhead.cpp` alone, the one D-6 exception. Any other
  banned term in that file is a hit. The counters library keeps its
  own clock reads.
- **FR-041**: A harness need the counters library does not meet shall
  become a counters change inside this spec, as D-6 requires. Examples
  are a cheaper clock read for calibration and a clock the catalog
  lacks. The change carries its own requirement, test, and version
  effect under D-5. The harness shall never bypass the library.
- **FR-042**: A test executable shall be able to register a fake
  provider that publishes the clock addresses in place of the clock
  provider, so scripted time drives every harness decision. The fake
  provider surface exists at the audit point in
  `include/speedgun-ng/counters_fake.hpp`.

### Constitution amendment

- **FR-043**: The feature shall add the D-6 entry to Additional
  Constraints beside the Library-first entry, stating the rule, the
  D-6 scope sentence, the counters-library definition, the full
  banned list, the named exception, the counters-change route, and
  the amendment-only exception route.
- **FR-044**: The Principle VIII gate list shall gain the time-source
  gate as a hard gate.
- **FR-045**: The amendment shall follow the Governance section at the
  audit point: rationale recorded, version bumped to 2.18.0, a new
  Sync Impact Report at the head of the file, a lineage row, the Last
  Amended date set, and the version footer set to the new version.
  The amendment leaves V.2 and `.clang-tidy` unchanged.

### Version

- **FR-046**: The feature shall release version 0.6.0 and keep
  `SOVERSION` at 2. The package config keeps `SameMinorVersion`. A
  counters change under FR-041 that alters an existing public
  signature or record layout raises `SOVERSION` by one.

### Naming

- **FR-047**: Every new identifier shall follow the rules N-1 through
  N-12 of Principle V.1. The harness shall copy no old spelling and
  add no new one. The exception list of V.2 is the only exception
  list, and a deviation needs a constitutional amendment.
- **FR-048**: The name check shall report zero
  `readability-identifier-naming` findings on harness translation
  units. The check does not resolve identifiers inside macro bodies,
  so the `sgCtReject` gap recorded under PC-10 raises no finding in a
  harness translation unit that expands the contract macros.

### Repository constraints

- **FR-049**: The harness shall live in the `speedgun-ng` library with
  its public interface under `include/speedgun-ng/` (Additional
  Constraints, Library-first). The counters headers shall gain no
  harness term, which keeps 007 FR-049 true for the counters surface.
- **FR-050**: No public header shall gain a platform term
  (`test/counters_header_purity.sh`). `getopt_long` shall appear in
  the implementation alone.
- **FR-051**: The harness shall add no new runtime dependency
  (Additional Constraints, Dependencies), and it shall use no vendored
  library in this spec. The leak audits of specs 003 to 009 shall keep
  passing.
- **FR-052**: The timed loop shall allocate nothing, take no lock, and
  sample only at its entry and its exit. The harness shall add no work
  to `RecorderHandle::sample()` (007 FR-026).
- **FR-053**: Every interface shall document `\pre`, `\post`, and
  `\invariant`, and the source shall enforce each contract (Principle
  II, dbc-gate).
- **FR-054**: Tests shall run unprivileged at `perf_event_paranoid` 2
  on the CI matrix. A test that asserts an exact value shall use the
  fake provider and stay deterministic (Principle VI).
- **FR-055**: The `__asm__` statement of D-4 is the one compiler
  extension this spec adds. The plan shall record its P2 justification
  under Principle I, on the 011 precedent. The plan shall record TDD
  mode (Principle III).

### Scope

In: FR-001 through FR-055, a test for each capability, the time-source
gate of SC-013, a downstream consumer test that builds a suite against
the installed package, a documentation page, and an example suite
under `example/` that reports ns per iteration and instructions per
cycle.

Out, each owned by a later roadmap spec:

- argument families, fixtures, templated benchmarks, setup and
  teardown callbacks, and suite naming
  (`harness-registration-and-fixtures`);
- CPU time and process CPU time as reported columns, manual time,
  pause and resume, and time units (`harness-timing-modes`).
  Calibration still reads thread CPU time under FR-009;
- user counters and their flags, bytes and items processed, labels,
  custom statistics, and complexity
  (`harness-user-counters-and-statistics`);
- threads and thread families (`harness-threads`);
- chunked capture, percentiles through HdrHistogram_c, and recorder
  reuse beyond this spec's single recorder
  (`harness-chunked-capture-and-percentiles`);
- JSON metrics export and JSON raw-data reports
  (`harness-json-reports`);
- the regression reporter (`harness-regression-reporter`);
- the full command-line option set, environment defaults, and random
  interleaving (`speedgun-cli-and-run-control`);
- hwloc topology, placement presets, and cpu pinning
  (`topology-placement-presets`);
- execution contexts and custom YAML configuration
  (`execution-contexts`);
- metric formulas, metrics in the catalog listing, multi-pass
  collection, and top-down reconstruction (`metric-formulas`,
  `multi-pass-event-collection`, `topdown-reconstruction`).

This feature writes no file. JSON is the decided format for metrics
export and raw-data reports. YAML is the decided format for
application and execution-context configuration alone, and built-in
placement presets make a YAML file the advanced path. Neither format
enters this spec.

## Key Entities *(include if feature involves data)*

- **Benchmark handle**: the registration result. It carries the
  benchmark's name and sets the run-control and metric options.
- **State**: the object the harness passes to the benchmark function.
  It reports the iteration count of the current run and carries the
  two skip actions.
- **Run**: one pass of the timed loop over N iterations. It owns
  exactly two recorded points, the window endpoints.
- **Metric**: a counter expression built with the 007 arithmetic, or a
  catalog leaf attached by address. Its value is the fold of the two
  points of a run.
- **Result row**: one repetition or one aggregate. It carries the time
  value, each metric value with its disclosures, and the overhead
  floor.
- **Benchmark result**: the C++ value the harness exposes. The console
  report formats it and adds nothing to it.
- **Report context**: the host, the cpu, the library version, and the
  build type printed above the rows. A measured field comes from the
  counters library.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A fake-provider benchmark that drives known counts
  reports each per-iteration count equal to the scripted delta divided
  by N. A ratio metric over the same runs reports its window value
  undivided.
- **SC-002**: A fake-provider benchmark with scripted
  `machine/monotonic` and `machine/thread_cpu` deltas walks the D-3
  calibration sequence. The test asserts the iteration count of each
  step and the step that qualifies. A scripted step whose real time
  reaches 5 times the minimum time qualifies, while its thread CPU
  time stays below the minimum time. The recorded points of each
  measured run meet a D-3 stop condition. Warm-up and calibration runs
  add no sample to the statistics. A fixed N skips calibration. Its
  warm-up starts at N and grows within the FR-016 bound.
- **SC-003**: A fixture with a known set of repetition results matches
  mean, median, standard deviation, coefficient of variation, min, and
  max exactly.
- **SC-004**: A fake-provider run with running ratio 0.5 reports ratio
  0.5 and the scaled flag beside its value. A run with a gap reports
  the value as unavailable, and the statistics exclude it.
- **SC-005**: An instrumented test shows exactly two sampling actions
  per run and zero allocations inside the timed loop over 10,000 runs.
- **SC-006**: A benchmark function that throws on each of 1,000 runs
  leaves the process descriptor count and mapping count at their start
  values. The next benchmark runs and reports.
- **SC-007**: The name filter selects exactly the matching benchmarks.
  List mode runs no benchmark function. Dry run executes one iteration
  and one repetition per benchmark.
- **SC-008**: On a host with a countable `cpu/instructions` leaf, the
  example suite reports ns per iteration and instructions per cycle
  with running ratio, scaled flag, and overhead floor. On a host
  without it, the suite reports the metric unavailable with its
  refusal kind and exits zero.
- **SC-009**: On a fake-provider system, the catalog listing prints
  every registered leaf with its description, unit, read mode, and
  availability, and it runs no benchmark function. On the reference
  host, the listing shows the machine clock leaves and the core PMU
  leaves with their states.
- **SC-010**: The downstream consumer test builds and runs a suite
  against the installed package. The vendored-dependency leak audits
  pass.
- **SC-011**: Every hard gate of Principle VIII passes at the feature
  head. The set covers `ctest`, `dbc-gate`, `format-check`,
  `spell-check`, and `prose-lint`. It covers the static-analysis gate
  and the name check, which reports zero
  `readability-identifier-naming` findings. It covers the
  `ci-sanitize` preset, the `ci-tsan` preset with its `tsan` job, and
  the vendored-dependency leak audits. It also covers the shared-build
  audit, the downstream-consumer and consumer-release jobs, and the
  100% line, branch, and DBC coverage gates.
- **SC-012**: The project version is 0.6.0 and `SOVERSION` follows D-5
  at the feature head, and the plan's version table records both. A
  consumer that requests version 0.5 rejects the new package.
- **SC-013**: The time-source gate of the D-6 amendment runs in ctest
  and as a hard CI gate. It scans the code the D-6 scope binds,
  `tools/` included, for every banned source on the constitutional
  list and fails on a hit. A planted `std::chrono::steady_clock::now()`
  call in a harness source fails the gate, and its removal returns the
  gate to a pass. The feature records both pairs. A planted
  `std::chrono::system_clock` in `tools/dbc/overhead.cpp` fails the
  gate, and its removal returns the gate to a pass.
- **SC-014**: Every time the harness reports traces to a counters
  fold. A fake-provider run scripts each clock leaf. The test checks
  three outputs: the reported real time per iteration, each
  calibration and warm-up decision, and the reported overhead floor.
  Each one equals the value the scripted deltas and the plan produce.
  No harness value differs from its counters source.
- **SC-015**: Each counters change that FR-041 adds carries its own
  test in the counters suite, and the counters gates of SC-011 pass
  with it.
- **SC-016**: The constitution at the feature head carries the D-6
  rule. A review against D-6 confirms four facts: the Additional
  Constraints entry states the rule, the D-6 scope sentence, the
  counters-library definition, the full banned list, the named
  exception, the counters-change route, and the amendment-only
  exception route; the
  Principle VIII gate list names the time-source gate as a hard gate;
  the CI workflow runs that gate, and a hit fails the job; the
  version, Sync Impact Report, lineage row, Last Amended date, and a
  version footer equal to the new lineage row follow D-6.
- **SC-017**: A fake-provider benchmark raises `SIGINT` inside its
  function during a measured run. That run completes, and no later run
  starts. The report marks the benchmark skipped with an interrupt
  reason. The process descriptor count and mapping count return to
  their start values, and the exit status is nonzero. A source check
  finds no read of the interrupt flag in the `State` iterator.
- **SC-018**: Each invalid value of FR-034 fails before any benchmark
  runs. The cases are an unparseable value and a negative value for
  each numeric option, and a zero repetition count, iteration count,
  and minimum time. Each case prints a recoverable error, runs no
  benchmark function, and exits nonzero. A zero minimum warm-up time
  runs and exits zero.
- **SC-019**: The exit statuses match FR-036. A filter that matches no
  benchmark and a list-mode run each exit zero and run no benchmark
  function. A failed benchmark, a failed catalog listing, and an
  interrupt each exit nonzero.

## Assumptions

- Execution is single-threaded. Threads and thread families arrive
  with `harness-threads`, and the tsc single-endpoint rule (PC-8)
  holds by construction until then.
- The console is the only report surface. JSON reports arrive with
  `harness-json-reports`.
- The default minimum time, warm-up time, and repetition count are the
  D-3 values, read at the recorded upstream revision. A later upstream
  change does not move them; this spec pins the values it read.
- The reference host for SC-008 and SC-009 is the maintainer host. CI
  runs on GitHub-hosted runners, which promise no CPU vendor and no
  PMU access. CI covers the SC-008 branch its runner reaches.
- Tests run unprivileged at `perf_event_paranoid` 2, as the Constraints
  section states.
- The roadmap spec names in the Out list are the scope boundary. A
  later rename of a roadmap spec does not change this boundary.
- The counters library at the audit point meets every harness need the
  request names. FR-041 is the route for a need it does not meet, and
  no such need was found at the audit point.

## Dependencies

- The counters library as merged after the five changes: 012, 013 with
  its 0.4.1 patch, the 0.4.1 version, 014, and the post-merge rename
  repair. PC-1 records the state.
- The three PC-10 rename gaps (`sgCtReject`, `reached_after`, the
  `noexcept_violator` call) await a separate fix under X.3. None lies
  on the harness path, and this feature touches none of them.
- The D-6 constitution amendment lands inside this feature, per
  Governance.
- Google Benchmark is a reference read, nothing more. No vendored copy
  enters this spec, and the harness links nothing from it.
- The 007 fake provider (`include/speedgun-ng/counters_fake.hpp`)
  carries the test substitution of FR-042.

## Citations

File and line citations, read at the audit point `d6bcbb5`.

| Artifact | Location |
| --- | --- |
| `Availability` enumeration | `include/speedgun-ng/counters_core.hpp:87` |
| `TargetMask`, target bits | `include/speedgun-ng/counters_core.hpp:107,110,113` |
| `CatalogEntry` fields | `include/speedgun-ng/counters_core.hpp:246` |
| `MetricResult::availability` | `include/speedgun-ng/counters_core.hpp:285` |
| `PointsView::availability` | `include/speedgun-ng/counters_measurement.hpp:426` |
| `RecorderHandle::sample` | `include/speedgun-ng/counters_measurement.hpp:504` |
| `Plan::recorder` overloads | `include/speedgun-ng/counters_measurement.hpp:925,936` |
| `Plan::sampleOverheadNs*` | `include/speedgun-ng/counters_measurement.hpp:954,963,973` |
| `PointSink::put`, `putDisclosure` | `include/speedgun-ng/counters_provider.hpp:214,240` |
| `Object::counters` | `include/speedgun-ng/counters_system.hpp:94` |
| Clock leaf guarantees | `include/speedgun-ng/counters_clock.hpp:40-59` |
| Clock leaf addresses, x86 gate | `source/counters/clock_provider.cpp:21,35-39` |
| `Plan::~Plan` | `source/counters/plan.cpp:185` |
| Calibration and bracket subtraction | `source/counters/plan.cpp:264-315` |
| `sgCtReject` sites | `include/speedgun-ng/dbc.hpp:419-503` |
| `reached_after`, `noexcept_violator` | `test/source/dbc_test.cpp:476,491-498` |
| Name gate configuration | `.clang-tidy:21` |
| Version and `SOVERSION` | `CMakeLists.txt:7,41-47` |
| Package compatibility | `cmake/install-rules.cmake:39` |
| Naming law, gate, constraints | `.specify/memory/constitution.md` (V.1, V.2, VIII, Additional Constraints, lineage 2.17.0) |
| 007 FR-011, FR-018, FR-019, FR-021, FR-026, FR-032, FR-046, FR-049, FR-050 | `specs/007-counters-and-timers/spec.md:233,243,244,246,254,260,283,286,287` |
| 007 design journal, harness seam notes | `specs/007-counters-and-timers/sg_counters.md:1195` |
| 012 recorder-reuse decision | `specs/012-counters-defect-resolution/spec.md:134-137` |
| 012 FR-009 | `specs/012-counters-defect-resolution/spec.md:737` |
| 012 FR-013 to FR-015 (I-08) | `specs/012-counters-defect-resolution/spec.md:765-773` |
| 012 FR-021 (I-03) | `specs/012-counters-defect-resolution/spec.md:800-815` |
| 012 FR-025 to FR-027 (I-09) | `specs/012-counters-defect-resolution/spec.md:853-864` |
| 012 FR-028 to FR-031 (I-10) | `specs/012-counters-defect-resolution/spec.md:866-880` |
| 013 0.4.1 patch record | `specs/013-counters-defect-followup/plan.md:152-153,417` |
| 011 `__asm__` precedent | `specs/011-nanosecond-counter-ssc-mark/plan.md:45,92` |
| Rename map pair | `specs/014-identifier-naming-camelcase/rename-map.md`, `docs/pages/identifier-rename-map.md` |
| Header purity check | `test/counters_header_purity.sh` |
| Google Benchmark revision | `google/benchmark` `main` at `e662de9aab8e705ecf4fa4bd41a207e5a0acfd0c` (2026-10-08 15:52:18Z) |
| Upstream defaults | `include/benchmark/benchmark_api.h` (`kDefaultMinTimeStr`); `src/benchmark.cc` (`benchmark_min_warmup_time`, `benchmark_repetitions`) |
| Upstream calibration rule | `src/benchmark_runner.cc` (`kMaxIterations`, `ShouldReportIterationResults`, `PredictNumItersNeeded`, `RunWarmUp`, `DoOneRepetition`) |
| Upstream barriers | `include/benchmark/utils.h` (`DoNotOptimize`, `ClobberMemory`) |

## Open questions

None. Clarify records any question the HEAD checks raise.
