# Feature Specification: Harness Timing Modes

**Feature Branch**: `017-harness-timing-modes`

**Created**: 2026-10-10

**Status**: Draft

**Input**: User description, opening quoted, the full text held by the
specify invocation record:

<!-- prose-lint: allow reason="XI.5 verbatim quotation of the request, marked as quoted" -->
"Harness timing modes, spec H3 of the speedgun harness roadmap. Spec
H1, benchmark-harness-core, shipped a single-thread runner. It reports
real time per iteration from one sample pair per run. Spec H2 added
argument families and fixtures. This spec completes Google Benchmark
feature F-4: CPU time, process CPU time, real time, manual time, pause
and resume, and time units. Every time value comes from the counters
library as a fold over raw point pairs. The spec needs no hardware:
every test runs on the fake provider. [...]"

## Audit point

| Field | Value |
| --- | --- |
| Audit point | `adc3b924fe8e09b871554a2445580731c487257c` |
| Audit-point date | 2026-10-10 10:01:06 -0500 |
| Default branch at run time | `master`, tip `adc3b92`, equal to the `origin/master` tip |
| Working tree at run time | clean at `adc3b92` |
| Feature number at run time | 017, the next free number under `specs/` |
| Short name at run time | `harness-timing-modes` |
| Project version at the audit point | 0.7.0 (`CMakeLists.txt:7`) |
| Harness `SOVERSION` at the audit point | 3 (`CMakeLists.txt:219`) |
| Counters `SOVERSION` at the audit point | 2 (`CMakeLists.txt:47`) |
| Package compatibility at the audit point | `SameMinorVersion` (`cmake/install-rules.cmake:39`) |
| Constitution version at the audit point | 2.18.0, lineage table and footer equal (`.specify/memory/constitution.md:1125,1149`) |

The audit read the tree of `adc3b92`, the `master` commit that merged
pull request 33. That pull request is the H2 merge. Every citation
below was read at `adc3b92`.

## Preconditions

Each precondition was checked in the code and the cited artifacts at
the audit point. The result and its evidence follow.

### PC-0: H1 is merged with every task complete; H2 is merged too. PASS

- Pull request 32, "Harness: Add the benchmark harness core", is
  `MERGED` into `master` (merged 2026-10-09T19:26:31Z). Its feature
  directory `specs/015-benchmark-harness-core/` carries `spec.md`,
  `plan.md`, `tasks.md`, `research.md`, `data-model.md`, `contracts/`,
  and `checklists/`. Its `tasks.md` carries 77 closed task boxes and
  zero open boxes.
- Pull request 33, "Harness: Add families, fixtures and callbacks", is
  `MERGED` into `master` at `adc3b92` (merged 2026-10-10T15:29:18Z).
  `specs/016-harness-registration-and-fixtures/tasks.md` carries 64
  closed task boxes and zero open boxes. H2 is merged.
- The H1 and H2 names this spec builds on were confirmed at the audit
  point, with file and line:
  - `registerBenchmark(std::function<void(State&)> fn, std::string_view
    name) -> BenchmarkHandle`
    (`include/speedgun-ng/benchmark.hpp:946`) and
    `registerFixtureBenchmark` (`:966`).
  - `BenchmarkHandle` (`benchmark.hpp:594`), with the setters
    `minTime` (`:622`), `warmupTime` (`:632`), `repetitions` (`:641`),
    `iterations` (`:650`), `addMetric` (`:666`), the family calls
    (`:686-832`), and `setup` and `teardown` (`:855,872`).
  - `State` (`benchmark.hpp:161`), with `iterations()` (`:258`),
    `begin()` (`:279`), `end()` (`:303`), `skipWithError` (`:318`),
    `skipWithMessage` (`:343`), `range` (`:387`), and `rangeCount`
    (`:404`).
  - The registration macros `SG_BENCHMARK` (`:1014`),
    `SG_BENCHMARK_CAPTURE` (`:1053`), `SG_BENCHMARK_TEMPLATE`
    (`:1094`), `SG_BENCHMARK_F` (`:1157`), `SG_BENCHMARK_DEFINE_F`
    (`:1176`), `SG_BENCHMARK_REGISTER_F` (`:1199`),
    `SG_BENCHMARK_TEMPLATE_F` (`:1220`),
    `SG_BENCHMARK_TEMPLATE_DEFINE_F` (`:1249`),
    `SG_BENCHMARK_TEMPLATE_METHOD_F` (`:1317`), and
    `SG_BENCHMARK_TEMPLATE_INSTANTIATE_F` (`:1345`).
  - `speedgunMain(int argc, char** argv) -> int` (`benchmark.hpp:988`).
  - The family record `RegistryEntry`
    (`source/harness/detail/internal.hpp:44-65`) and the parsed
    command line `RunOptions` (`internal.hpp:129-139`).

This spec extends that surface. It replaces no H1 or H2 signature. The
version effect of the extension is recorded under PC-4 and FR-022.

### PC-1: the clock leaves. PASS

The shipped clock provider (`ClockProvider`,
`include/speedgun-ng/counters_clock.hpp:73`) seeds the `machine`
object with five leaves. The address table is
`source/counters/clock_provider.cpp:35-39`; the catalog seeds are
`:247-299`; the leaf guarantees are the class comment
(`counters_clock.hpp:27-59`).

| Leaf | Clock source | Unit and dimension | Read mode |
| --- | --- | --- | --- |
| `machine/monotonic` | `CLOCK_MONOTONIC` through the vDSO (`clock_provider.cpp:68`) | nanoseconds, time (`Dim<1,0>`) | syscall (`:250`) |
| `machine/thread_cpu` | `CLOCK_THREAD_CPUTIME_ID` (`:112`) | nanoseconds, time | syscall (`:257`) |
| `machine/process_cpu` | `CLOCK_PROCESS_CPUTIME_ID` (`:141`) | nanoseconds, time | syscall (`:264`) |
| `machine/tsc` | `__rdtsc`, published on x86 only (`:176`) | raw ticks, unit `none`, no rate (`:280`) | fast tick |
| `machine/monotonic_raw` | `CLOCK_MONOTONIC_RAW` (`:99`) | nanoseconds, time | syscall (`:295`) |

The list covers real time (`monotonic`), thread CPU time
(`thread_cpu`), and process CPU time (`process_cpu`). The runner
already resolves `machine/monotonic` and `machine/thread_cpu` as time
counters (`runner.cpp:174-190`).

### PC-2: the sampling window. PASS

- The entry sample stands in `State::begin()`, which calls
  `openWindow()`, which calls `recorder->sample()`
  (`benchmark.hpp:279-296,426-435`). The exit sample stands on the
  path that leaves the loop: `Cursor::operator!=` calls
  `closeWindow()`, which calls `takeExitSample()`
  (`benchmark.hpp:191-205,437-455`). `topUpWindow()`
  (`benchmark.hpp:456-461`) leaves exactly one pair per run on the
  skip and exception paths (`runner.cpp:371-379`).
- One run adds one sample pair. Each sample takes one raw point per
  leaf of the run plan. The plan holds the real-time core, the
  decision core, and one core per metric column
  (`runner.cpp:274-279`). A benchmark with no attached metric adds
  two raw points per sample and four per run.
- The recorder capacity is `2 * (kRunBound + kRunBound +
  repetitions)` points: the warm-up bound, the calibration bound, and
  the measured repetitions, one pair per run (`runner.cpp:307-308`);
  `kRunBound` is 96 (`source/harness/detail/calibration.hpp:20`). The
  rule holds no allowance for pause pairs today. FR-014 extends it.

### PC-3: the report and the calibration clock. PASS

- The report row prints one time column, `time/iter (ns)`, and it
  shows real time per iteration (`source/harness/report.cpp:93-94`;
  the value comes from the fold of `record.realNs`,
  `runner.cpp:107-108`). The row also prints `overhead floor (ns)`
  and the fixed text `endpoint samples=2` (`report.cpp:94`). The
  aggregate row summarizes that one column (`report.cpp:63-66`;
  `runner.cpp:504`).
- Warm-up and calibration decide on `decisionNs`, the fold of
  `machine/thread_cpu` (`runner.cpp:179-190,402`). `qualifies()`
  tests `decisionNs` against the target, the iteration cap, and
  `realNs` against five times the target
  (`calibration.hpp:67-71`). `nextIterationCount` grows from
  `decisionNs` (`calibration.hpp:36-52,130`).

### PC-4: the constitution. PASS

- The constitution is 2.18.0 (footer `constitution.md:1149`, lineage
  row `:1125`).
- The counters-only rule stands in Additional Constraints
  (`:1054`, rule `:1068-1092`): every time and counter value outside
  the counters library comes from `sg::counters`; the forbidden time
  sources are listed; the single named exception is
  `tools/dbc/overhead.cpp`.
- The time-source gate is a hard gate of Principle VIII (`:690-691`),
  enforced by `test/time_source_gate.sh`.
- Principle V.1 states the naming rules N-1 through N-12
  (`:533-580`). N-3 requires the `SG_` prefix on every macro (`:549`).
- The hand-kept `SOVERSION` rule is stated in full at
  `CMakeLists.txt:41-47` for the counters archive and applied to the
  harness archive at `CMakeLists.txt:211-219`.

### PC-5: the fake provider scripts every PC-1 leaf. PASS

- `FakeProvider` (`include/speedgun-ng/counters_fake.hpp:102`)
  declares any leaf through `addCounter` (`:167-173`) and scripts it
  with an explicit cumulative point sequence through `setPoints`
  (`:190-194`). The script binds one `(object, leaf)` pair, so
  `machine/monotonic`, `machine/thread_cpu`, `machine/process_cpu`,
  `machine/tsc`, and `machine/monotonic_raw` are scripted like any
  other leaf.
- The harness tests already script `machine/thread_cpu` so run deltas
  are exact (`test/source/harness_calibration_test.cpp:7`,
  `test/source/harness_argument_test.cpp:111-113`). H1 FR-042
  (`specs/015-benchmark-harness-core/spec.md:807`) binds the fake
  provider as the test substitution. Every test of this spec runs on
  it, and no test needs a PMU.

## Roadmap position

This is spec H3 of the speedgun harness roadmap. H1 shipped the
single-thread runner with one real-time column; H2 added argument
families and fixtures. This spec completes Google Benchmark feature
F-4: CPU time, process CPU time, real time, manual time, pause and
resume, and time units. The spec needs no hardware; every test runs
on the fake provider. The roadmap document sits outside this
repository and is absent from this machine at the audit point. The
scope boundary below comes from the request. The roadmap spec names
are the boundary.

## Google Benchmark reference read

Google Benchmark is a reference read, nothing more. No vendored copy
enters this spec, and the harness links nothing from it. The revision
read is `google/benchmark` `main` at
`e662de9aab8e705ecf4fa4bd41a207e5a0acfd0c` (2026-10-08 15:52:18Z),
the same revision H1 and H2 read (015 `spec.md:1111`; 016 reference
read section). Each semantic this spec takes, with the file and the
function of that revision:

| Semantic taken | Source at the revision |
| --- | --- |
| `MeasureProcessCPUTime` sets the process CPU choice; it combines with `UseRealTime` and `UseManualTime` | `src/benchmark_register.cc` (`Benchmark::MeasureProcessCPUTime` `:429-433`); the choice reaches the timer at `src/benchmark_runner.cc` (`RunInThread` `:148-151`) and `src/thread_timer.h` (`ReadCpuTimerOfChoice` `:64-67`) |
| `UseRealTime` makes real time the decision time; it conflicts with `UseManualTime` | `benchmark_register.cc` (`:435-439`); `benchmark_runner.cc` (`DoNIterations` decision `:363-367`) |
| `UseManualTime` makes the accepted manual sum the decision time and the reported real column | `benchmark_register.cc` (`:441-445`); `benchmark_runner.cc` (`BuildReport` `:110-114`, decision `:363-367`) |
| `SetIterationTime(seconds)` adds one user value per call to the manual accumulator | `include/benchmark/state.h` (`:84`); `src/benchmark.cc` (`State::SetIterationTime` `:294-296`); `thread_timer.h` (`SetIterationTime` `:41`) |
| `PauseTiming` and `ResumeTiming` are legal only inside the timed loop; they stop and start the timer, and the reported time is the sum of the running segments | `state.h` (`:60-72`, the `in_timed_section` assertion); `benchmark.cc` (`PauseTimingImpl` `:239-255`, `ResumeTimingImpl` `:257-262`); `thread_timer.h` (`StartTimer` `:23-27`, `StopTimer` `:30-38`) |
| `ScopedPauseTiming` pauses for its lifetime; the destructor resumes unless the run was skipped | `state.h` (`:282-296`) |
| Calibration defaults to CPU time; real or manual time replaces it when selected | `benchmark_runner.cc` (`:363-367`) |
| A phase stops at the target on the decision time, at the iteration cap, or at five times the target in real time; the fivefold stop exempts manual time | `benchmark_runner.cc` (`ShouldReportIterationResults` `:403-411`) |
| Time units: four units; a per-benchmark `Unit` call; a library-set default captured at registration; the report scales the value and prints the unit label | `include/benchmark/types.h` (`TimeUnit` `:46`); `include/benchmark/benchmark_api.h` (`Unit` `:100`, `GetDefaultTimeUnit` and `SetDefaultTimeUnit` `:74-76`); `benchmark_register.cc` (default capture `:225-226`, `Unit` `:256-259`); `benchmark.cc` (`:663-668`); `src/reporter.cc` (`GetAdjustedRealTime` `:122`, `GetAdjustedCPUTime` `:130`); `src/console_reporter.cc` (`:157-173`) |
| The console row prints a real-time column and a CPU-time column, each under the unit label | `console_reporter.cc` (`:157-173`) |

Semantics deliberately not taken: threads and per-thread CPU
aggregation belong to the threads spec; user counters and statistics
to the counters spec; the `--benchmark_time_unit` command-line option
to the full command-line spec; complexity, memory, and profiler hooks
to no roadmap spec. The pause arithmetic differs by design: the cited
revision accumulates running segments inside a timer object, while
this harness folds raw point pairs and subtracts each paused
interval, because the counters-only rule binds every value to a fold
(Q-4). The cited revision's `double` seconds become integer
nanoseconds here, as they are throughout the harness.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Read real time and CPU time per iteration (Priority: P1)

A user runs a benchmark and reads two time columns in every row:
real time per iteration and CPU time per iteration. The CPU column
defaults to the thread CPU time of the sampling thread. Both values
come from the counters library as folds over the run's raw point
pair. This is the MVP slice: one column already prints, and the
second column is the base every later capability switches, pauses,
or scales.

**Why this priority**: Every other capability of this spec names a
time column or the clock behind one. Without the pair, the process
CPU switch, the real-time switch, and the manual column have nothing
to change.

**Independent Test**: Register one benchmark on the fake provider
with scripted `machine/monotonic`, `machine/thread_cpu`, and
`machine/process_cpu` sequences, run the suite, and read both time
columns. Each column equals its scripted delta divided by the
iteration count.

**Acceptance Scenarios**:

1. **Given** a benchmark with scripted real and thread CPU
   sequences, **When** the suite runs, **Then** the row prints real
   time per iteration and CPU time per iteration, each exact to the
   scripted deltas.
2. **Given** the same run with no pause call, **When** the harness
   samples, **Then** the run adds one sample pair, as H1 left it, and
   the row prints the sample count of the run.
3. **Given** a benchmark with an attached metric, **When** the row
   prints, **Then** the metric keeps its H1 fold and the two time
   columns stand beside it.

---

### User Story 2 - Switch the CPU leaf and the primary time (Priority: P2)

A user marks one benchmark to report process CPU time instead of
thread CPU time, as `MeasureProcessCPUTime` does. A user marks
another benchmark to calibrate on real time and to report real time
as its primary time, as `UseRealTime` does. The primary time is the
time that drives warm-up qualification, calibration growth, and the
aggregate time statistics; it defaults to the CPU time selection.

**Why this priority**: These are the two selection calls of F-4 that
change which clock answers which column. They depend on US1's two
columns and add no new run mechanics.

**Independent Test**: Register one benchmark with the process CPU
call and one with the real-time call, each on scripted leaves, and
run. Check the CPU column source and the clock the growth rule
reads.

**Acceptance Scenarios**:

1. **Given** a benchmark with the process CPU call and a scripted
   `machine/process_cpu` sequence, **When** the row prints, **Then**
   the CPU column shows the process CPU fold and not the thread CPU
   fold.
2. **Given** a benchmark with the real-time call, **When** the
   harness calibrates, **Then** the qualification test and the growth
   rule read the real-time fold, and the aggregate time statistics
   summarize the real-time column.
3. **Given** a benchmark with the process CPU call and the real-time
   call together, **When** the suite runs, **Then** the run is legal:
   the CPU column shows process CPU time and the primary time is
   real time, as the cited revision combines them.

---

### User Story 3 - Report a manually measured time (Priority: P2)

A user marks one benchmark as manual-time, as `UseManualTime` does,
and the benchmark function hands the state one iteration time per
iteration, as `SetIterationTime` does. The reported time is the sum
of the accepted times. Manual time is the one value the user
supplies; the harness records it by summation and reads no clock for
it.

**Why this priority**: Manual time serves benchmarks that carry their
own instrument, such as an in-tool profiler counter. It depends on
US1's columns and US2's primary-time rule.

**Independent Test**: Register a manual-time benchmark whose function
calls the accept method once per iteration with fixed values, run on
the fake provider, and check the reported column and the calibration
clock against the scripted sum.

**Acceptance Scenarios**:

1. **Given** a manual-time benchmark that accepts a fixed
   nanosecond value per iteration, **When** the row prints, **Then**
   the real-time column shows the accepted sum divided by the
   iteration count.
2. **Given** the same benchmark, **When** the harness calibrates,
   **Then** the qualification test and the growth rule read the
   accepted sum, and the fivefold real-time stop does not apply.
3. **Given** a manual-time benchmark that never accepts a time,
   **When** the suite runs, **Then** the sum is zero, the run
   qualifies at the iteration cap, and the row prints zero.

---

### User Story 4 - Pause and resume the clock (Priority: P2)

A user calls `pauseTiming` inside the timed loop to take the setup
work out of the reported time, and `resumeTiming` to bring the clock
back. A scoped guard pauses for its lifetime. Each pause and each
resume takes one sample. The reported time is the window fold minus
the fold of each paused interval.

**Why this priority**: Pause and resume take unmeasured work out of
the reported time of a benchmark that cannot move that work out of
the loop. They depend on US1's window and change every time column
and every attached metric fold.

**Independent Test**: Register a benchmark that pauses and resumes
once inside the loop, with scripted leaves, and run. The reported
time equals the window fold minus the scripted pause fold, exact to
the nanosecond, and the attached metric fold shows the same
subtraction.

**Acceptance Scenarios**:

1. **Given** a benchmark that pauses and resumes once, **When** the
   row prints, **Then** each time column equals the window fold minus
   the pause interval fold.
2. **Given** the same benchmark with an attached metric, **When** the
   row prints, **Then** the metric value excludes the paused interval
   too, because each pause and resume samples every leaf of the plan.
3. **Given** a scoped guard that pauses for its lifetime, **When**
   the guarded block ends, **Then** the clock resumes and the result
   matches the explicit pair.
4. **Given** a benchmark that pauses and never resumes, **When** the
   loop exits, **Then** the harness reports a precondition violation.
5. **Given** a run with one pause interval, **When** the row prints,
   **Then** the printed sample count names the four endpoint samples
   of that run.

---

### User Story 5 - Scale the printed time to a unit (Priority: P2)

A user sets the time unit of one benchmark: nanoseconds,
microseconds, milliseconds, or seconds. A library call sets the
default unit for every benchmark that sets none. Each printed time
value carries the scaled value and its unit label.

**Why this priority**: Units change only the printed scale of values
US1 to US4 already produce; they make slow benchmarks readable.

**Independent Test**: Register benchmarks with each unit call and one
with none, set the default, and compare the printed value and label
against the scaled scripted delta.

**Acceptance Scenarios**:

1. **Given** a benchmark with the microsecond unit and a scripted
   delta, **When** the row prints, **Then** each time column shows
   the delta scaled to microseconds and labeled `us`.
2. **Given** a library call that sets the default unit to
   milliseconds, **When** a benchmark with no unit call runs,
   **Then** its time columns print scaled to milliseconds and labeled
   `ms`.
3. **Given** the default set after a benchmark registered, **When**
   that benchmark runs, **Then** it keeps the default captured at its
   registration, as the cited revision captures it.

---

### User Story 6 - Examples and documentation (Priority: P3)

The example suite gains one benchmark for each of the capabilities
C-2 through C-6. The harness documentation page gains one section
for each capability C-1 through C-7.

**Why this priority**: Examples and documentation follow the code and
give the roadmap its demonstration surface for F-4.

**Independent Test**: Build and run the example suite; check that one
benchmark shows each capability. Read the documentation page; check
one section per capability.

**Acceptance Scenarios**:

1. **Given** the example suite, **When** it is built and run,
   **Then** it shows one benchmark for the process CPU switch, one
   for the real-time switch, one for manual time, one for pause and
   resume, and one for a time unit.
2. **Given** `docs/pages/harness.md`, **When** it is read, **Then**
   it holds one section for each capability C-1 through C-7.

### Edge Cases

- A pause still open at loop exit, a `resumeTiming` with no open
  pause, and a second `pauseTiming` while already paused are each a
  precondition violation (Q-3). The cited revision's timer check
  fails on the same double stop (`thread_timer.h:31`).
- `pauseTiming` or `resumeTiming` in a callback state is a
  precondition violation: that state opens no sampling window (H2
  FR-018, R-05).
- A manual-time benchmark that accepts no time reports zero and
  qualifies at the iteration cap.
- Selecting real time and manual time on one benchmark is a
  precondition violation, as the cited revision checks
  (`benchmark_register.cc:435-445`). The process CPU selection stays
  legal with either.
- A pause interval whose endpoint pair carries a gap disclosure
  follows the H1 gap rule for the affected fold: the value stays out
  of the aggregate statistics.
- A time unit set after registration does not change a benchmark
  registered earlier; the default is captured at registration.
- A `DISABLED_` family keeps H2 behavior under every mode: it
  registers and never runs.
- A dry run keeps its H1 shape: one iteration, one repetition, no
  warm-up. Pause and resume stay legal inside the loop.
- A run whose pause count exceeds the per-run allowance of the
  capacity rule is a precondition violation, named before the run
  overruns the recorder (FR-014).
- The process CPU leaf resolves on the fake provider like any leaf;
  a host that publishes no `machine/process_cpu` is a failed
  resolution for that benchmark, as the thread CPU miss fails today
  (`runner.cpp:181-186`).

## Requirements *(mandatory)*

### Time columns

- **FR-001**: Each measured row shall report real time per iteration
  and CPU time per iteration. Both values shall be folds over the
  run's raw point pair, supplied by the counters library. The CPU
  column shall default to the `machine/thread_cpu` leaf.
- **FR-002**: One primary time per benchmark shall drive warm-up
  qualification, calibration growth, and the aggregate time
  statistics. The primary time shall default to the CPU time
  selection. (Q-1 confirmed default.)
- **FR-003**: A run with no pause shall keep the H1 timed loop and
  shall add exactly one sample pair per run. The loop-shape gate of
  H1 shall stay green.

### Process CPU time

- **FR-004**: A handle call shall switch the CPU column of one
  benchmark to the `machine/process_cpu` leaf, as
  `MeasureProcessCPUTime` does. The switch shall combine with the
  real-time and manual-time selections, as the cited revision
  combines them.

### Real-time calibration

- **FR-005**: A handle call shall make real time the primary time of
  one benchmark: real time shall drive warm-up qualification,
  calibration growth, and the aggregate time statistics, as
  `UseRealTime` does.

### Manual time

- **FR-006**: A handle call shall select manual time for one
  benchmark, as `UseManualTime` does. The primary time shall then be
  the sum of the iteration times the state accepted during the run.
  The fivefold real-time stop shall not apply to a manual-time
  benchmark, as the cited revision exempts it
  (`benchmark_runner.cc:403-411`).
- **FR-007**: The state shall accept one iteration time per iteration
  through a call that takes a nanosecond count, as `SetIterationTime`
  does. The harness shall record the sum of the accepted values by
  summation alone and shall read no clock for it. When manual time is
  selected, the real-time column shall show that sum divided by the
  iteration count.
- **FR-008**: Selecting both real time and manual time on one
  benchmark shall be a precondition violation. The process CPU
  selection shall stay legal with either selection.

### Pause and resume

- **FR-009**: The state shall offer `pauseTiming` and `resumeTiming`.
  Each call shall be legal only inside the timed loop of a run state.
  Each call shall take exactly one sample of every leaf of the run
  plan; the cost of one sampling action per call stands disclosed
  beside each affected row (FR-013).
- **FR-010**: The reported time of a run shall be the window fold
  minus the fold of each paused interval. The subtraction shall apply
  to every time column and to every attached metric fold, so attached
  metrics exclude paused work too. (Q-4 confirmed.)
- **FR-011**: A `pauseTiming` call while already paused, a
  `resumeTiming` call with no open pause, and a pause still open at
  loop exit shall each be a precondition violation. (Q-3 confirmed.)
- **FR-012**: A scoped guard type shall pause at construction and
  resume at destruction, for its lifetime, as `ScopedPauseTiming`
  does. The destructor shall skip the resume when the run was
  skipped.
- **FR-013**: Each row shall print the number of endpoint samples its
  run took, replacing today's fixed text, so the sampling cost of
  each pause and each resume stands beside the affected row.
- **FR-014**: The recorder shall hold every raw point a run adds, and
  the capacity rule shall cover the pause pairs a run adds. A run
  whose pause count exceeds the per-run allowance of the capacity
  rule shall be a precondition violation.

### Time units

- **FR-015**: A handle call shall set the time unit of one benchmark:
  nanoseconds, microseconds, milliseconds, or seconds. Each printed
  time value of that benchmark shall carry the value scaled to that
  unit and the unit label.
- **FR-016**: A library call shall set the default time unit for
  every benchmark that sets none. The default shall start at
  nanoseconds, shall be captured at registration, and a later call
  shall not change a registered benchmark. The command-line option
  stays with the full command-line spec. (Q-5 confirmed.)

### Examples and documentation

- **FR-017**: `example/benchmark_example.cpp` shall gain one
  benchmark for each of C-2 through C-6. `docs/pages/harness.md`
  shall gain one section for each capability C-1 through C-7.

### Surface, version, and discipline

- **FR-018**: This spec shall extend the H1 and H2 surface and
  replace no existing signature. The report row layout changes: two
  time columns and a real sample count replace the single time column
  and the fixed sample text. `ResultRow` and `BenchmarkResult` gain
  time fields, and `State` gains pause bookkeeping and the manual
  accumulator.
- **FR-019**: Every new identifier shall follow V.1 rules N-1 through
  N-12, and every new macro shall take the `SG_` prefix. The name
  check shall report zero findings.
- **FR-020**: Every new interface shall document `\pre`, `\post`, and
  `\invariant`, and the source shall enforce each contract
  (Principle II).
- **FR-021**: Every time and counter value shall come from the
  counters library; the time-source gate shall stay clean. The
  accepted manual iteration time is the one value the user supplies,
  and the harness records it by summation only (FR-007).
- **FR-022**: The feature shall release version 0.8.0, the next minor
  above 0.7.0. `State` gains pause bookkeeping and the manual
  accumulator, and the result value gains the time fields, so both
  layouts change. Under the hand-kept rule of `CMakeLists.txt:41-47`,
  the harness `SOVERSION` shall rise from 3 to 4. The counters
  archive keeps `SOVERSION` 2. The plan's version table shall record
  both. (Q-6 confirmed.)
- **FR-023**: No `LCOV_EXCL` marker shall enter `source/harness/`.
  Coverage shall stay at 100% line and branch.
- **FR-024**: The plan shall record TDD mode. Every test of this spec
  shall run on the fake provider, stay deterministic, and need no
  PMU.
- **FR-025**: All spec, plan, task, and code prose shall obey
  Principle XI, including XI.7, with zero prose gate findings.
- **FR-026**: The feature shall add no compiler extension and no
  runtime dependency.

## Key Entities *(include if feature involves data)*

- **Timing mode**: the per-family selection this spec adds to the H2
  family record: the CPU leaf choice (thread or process), the primary
  time choice (CPU, real, or manual), and the time unit. It exists
  before the first run, like every other family field.
- **Manual accumulator**: the sum of the iteration times the state
  accepted during one run. It is the primary time of a manual-time
  benchmark and the real column of its rows.
- **Pause interval**: the pair of samples that bounds one paused
  stretch of one run. Its fold is the value subtracted from the
  window fold. A run holds a list of these pairs.
- **Time unit**: the scale and label of the printed time values: a
  library default captured at registration, and a per-benchmark
  override.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Scripted thread CPU, process CPU, and real time leaves
  give exact column values per iteration, for the default selection
  and for each selection call.
- **SC-002**: Each of C-2, C-3, and C-4 changes the primary time and
  the calibration clock as the cited revision does, observed through
  the scripted growth sequence.
- **SC-003**: A scripted pause interval leaves the reported time
  exact to the nanosecond: window fold minus pause fold, in every
  time column and in an attached metric fold.
- **SC-004**: A run with no pause adds the same raw point count as
  H1: one sample pair, and the printed sample count reads two.
- **SC-005**: Each unit of C-6 prints the scaled value and its unit
  label, for the per-benchmark call and for the library default.
- **SC-006**: Every hard gate of Principle VIII passes at the feature
  head, the time-source gate and the loop-shape gate included.
- **SC-007**: The version and `SOVERSION` follow FR-022, and the
  plan's version table records both.

## Scope

In: FR-001 through FR-026, a test for each capability C-1 through
C-7, and the version effect of FR-022. The seven capabilities, with
the requirements that carry each:

| Capability | Requirements |
| --- | --- |
| C-1 time columns | FR-001, FR-002, FR-003 |
| C-2 process CPU time | FR-004 |
| C-3 real-time calibration | FR-005 |
| C-4 manual time | FR-006, FR-007, FR-008 |
| C-5 pause and resume | FR-009, FR-010, FR-011, FR-012, FR-013, FR-014 |
| C-6 time units | FR-015, FR-016 |
| C-7 examples and documentation | FR-017 |

FR-018 through FR-026 bind the whole surface and carry no capability
id.

Out, each owned by a later roadmap spec:

- threads, thread families, and per-thread CPU aggregation;
- user counters and statistics;
- JSON reports and the full command line;
- chunked capture and percentiles.

## Assumptions

- The Q-1 to Q-6 recommendations of the request stand as the adopted
  working defaults; the Open questions section records each, and
  clarify confirms or replaces it.
- The per-run pause allowance inside the capacity rule takes its
  value in the plan. The spec fixes only that the rule covers pause
  pairs and that exceeding the allowance is a precondition violation.
- The scoped guard is a class in the `sg` namespace named by V.1
  rules; no new macro is needed for it.
- The unit labels are `ns`, `us`, `ms`, and `s`, the labels of the
  cited revision's console reporter.
- The report keeps one row per repetition and the H1 aggregate shape;
  the single time column becomes two, and the fixed sample text
  becomes the run's real sample count.
- Manual time is unaffected by pause and resume: the user's accepted
  values are the record, and no fold subtracts from them.
- The time unit scales the printed time columns; the overhead floor
  keeps its nanosecond form unless the plan scales it too.

## Dependencies

- The H1 and H2 harness surface at the audit point: the registry, the
  family record, the runner, the report, the filter and list mode,
  and the timed window of PC-2.
- The counters library: the plan, the recorder, the fold, and the
  availability disclosure (specs 007, 008, 011, 012, 013).
- The counters fake provider (`include/speedgun-ng/counters_fake.hpp`)
  as the test substitution of H1 FR-042.
- The constitution 2.18.0 gates: name check, prose, sanitizers,
  coverage, time source, and the loop-shape gate of H1.
- Google Benchmark is a reference read, nothing more. No vendored
  copy enters this spec, and the harness links nothing from it.

## Citations

File and line citations, read at the audit point `adc3b92`.

| Artifact | Location |
| --- | --- |
| `registerBenchmark`, `registerFixtureBenchmark` | `include/speedgun-ng/benchmark.hpp:946,966` |
| `BenchmarkHandle` and setters | `benchmark.hpp:594,622,632,641,650,666,686-832,855,872` |
| `State`, cursor, window, members | `benchmark.hpp:161,191-205,258,279-296,303,318,343,387,404,415,426-435,437-455,456-461,472-486` |
| Registration macros | `benchmark.hpp:1014,1053,1094,1157,1176,1199,1220,1249,1317,1345` |
| `speedgunMain` | `benchmark.hpp:988` |
| `RegistryEntry`, `RunOptions` | `source/harness/detail/internal.hpp:44-65,129-139` |
| `ResultRow`, `BenchmarkResult` | `benchmark.hpp:110-119,130-146` |
| Clock leaf table | `source/counters/clock_provider.cpp:35-39,247-299` |
| Clock leaf readers | `clock_provider.cpp:68,99,112,141,176` |
| Clock leaf guarantees | `include/speedgun-ng/counters_clock.hpp:27-59,73` |
| Runner clock resolution | `source/harness/runner.cpp:174-190,274-279` |
| Recorder capacity rule | `runner.cpp:307-308`; `kRunBound` `source/harness/detail/calibration.hpp:20` |
| Run pair bookkeeping | `runner.cpp:371-379,400-402` |
| Row time value | `runner.cpp:98-108`; aggregate `runner.cpp:504` |
| Report row and aggregate format | `source/harness/report.cpp:63-66,93-94` |
| Qualification and growth | `calibration.hpp:36-52,67-71,130` |
| `FakeProvider`, `addCounter`, `setPoints` | `include/speedgun-ng/counters_fake.hpp:102,167-173,190-194` |
| Fake scripted harness tests | `test/source/harness_calibration_test.cpp:7`, `test/source/harness_argument_test.cpp:111-113` |
| H1 FR-017 window, FR-042 fake, SC-013 time gate | `specs/015-benchmark-harness-core/spec.md:651,807,993` |
| H2 FR-018 callback state | `specs/016-harness-registration-and-fixtures/spec.md` (FR-018) |
| Loop-shape gate | `test/loop_shape.sh` (IF-04; 015 FR-004, FR-005, R-06) |
| Time-source gate | `test/time_source_gate.sh` |
| Version, `SOVERSION` rule | `CMakeLists.txt:7,41-47,211-219` |
| Package compatibility | `cmake/install-rules.cmake:39` |
| Constitution V.1, VIII, counters-only, footer | `.specify/memory/constitution.md:533-580,690-691,1068-1092,1125,1149` |
| Harness documentation page | `docs/pages/harness.md` (sections C-1 to C-10 of H2) |
| Example suite | `example/benchmark_example.cpp` |
| Google Benchmark revision | `google/benchmark` `main` at `e662de9aab8e705ecf4fa4bd41a207e5a0acfd0c` (2026-10-08 15:52:18Z) |
| GB semantics table | the table of the reference read section above |

## Open questions

The request carries six questions to clarify. Each has a
recommendation, the spec adopts the recommendation as the working
default, and clarify confirms or replaces it.

- Q-1 Calibration clock: Google Benchmark calibrates on CPU time
  unless real or manual time is selected. Recommendation: follow that
  rule. Adopted in FR-002, FR-005, and FR-006.
- Q-2 Manual time source: Recommendation: the state accepts a
  duration in nanoseconds, and the documentation shows a measurement
  through the counters API. Adopted in FR-007.
- Q-3 Pause state at loop exit: Recommendation: a pause still open at
  loop exit is a precondition violation. Adopted in FR-011.
- Q-4 Pause sampling: Recommendation: each pause and resume samples
  every leaf of the plan, so attached metrics exclude paused work
  too. Adopted in FR-009 and FR-010.
- Q-5 Default time unit: Recommendation: a library call sets it now;
  the command-line option waits for the full command-line spec.
  Adopted in FR-016.
- Q-6 Version: Recommendation: the next minor version, and `SOVERSION`
  up by one when `State` or `BenchmarkHandle` changes layout. Adopted
  in FR-022: version 0.8.0, harness `SOVERSION` 3 to 4, because
  `State` gains pause bookkeeping and the manual accumulator.
