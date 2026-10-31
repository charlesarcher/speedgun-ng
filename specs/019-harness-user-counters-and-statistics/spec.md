# Feature Specification: Harness User Counters and Statistics

**Feature Branch**: `019-harness-user-counters-and-statistics`

**Created**: 2026-10-14

**Status**: Draft

**Input**: User description, opening quoted, the full text held by the
specify invocation record:

<!-- prose-lint: allow reason="XI.5 verbatim quotation of the request, marked as quoted" -->
"Harness user counters and statistics, spec H4 of the speedgun harness
roadmap. D1 runs before this spec and sets the main-page format. Spec
H1 shipped a single-thread runner with attached counter metrics and
repetition aggregates. Specs H2 and H3 added families, fixtures, and
timing modes. This spec completes Google Benchmark feature F-5 and the
F-6 part that needs no raw-data capture. It adds user counters with
flags, bytes and items processed, and labels. It also adds custom
statistics, the aggregate-only modes, and complexity fitting. The spec
needs no hardware: every test runs on the fake provider. [...]"

## Clarifications

### Session 2026-10-14

- Q: Should the complexity fit cover only the family's primary time,
  or also each named user counter? → A: the fit covers the primary
  time, each named user counter, and each attached metric; every
  fitted quantity gets its BigO row and RMS row (Google Benchmark
  parity).
- Q: Where in a report row does the benchmark's label get printed?
  → A: a dedicated trailing ` label=<text>` field, printed only when
  the row carries a label; the instance-name field stays unchanged.
- Q: What is the maximum number of named user counters one benchmark
  can carry? → A: sixteen; opening a seventeenth name fails the
  benchmark with a reason that states the cap.

## Audit point

| Field | Value |
| --- | --- |
| Audit point | `a7776b608c14a58730b702882c005b8287d21573` |
| Audit-point date | 2026-10-13 19:38:09 -0500 |
| Default branch at run time | `master`, tip `a7776b6`, equal to the `origin/master` tip |
| Working tree at run time | the `018-docs-main-page` checkout at `c78fc6a`; `git diff origin/master HEAD` is empty, so the read saw the audit-point content byte for byte |
| Feature number at run time | 019, the next free number under `specs/` |
| Short name at run time | `harness-user-counters-and-statistics` |
| Project version at the audit point | 0.8.0 (the `project()` VERSION declaration in `CMakeLists.txt`) |
| Harness `SOVERSION` at the audit point | 4 (the harness target `SOVERSION` in `CMakeLists.txt`) |
| Counters `SOVERSION` at the audit point | 2 (the counters target `SOVERSION` in `CMakeLists.txt`) |
| Package compatibility at the audit point | `SameMinorVersion` (the `COMPATIBILITY` clause in `cmake/install-rules.cmake`) |
| Constitution version at the audit point | 2.19.0, lineage row and footer equal (the amendment record and the footer of `.specify/memory/constitution.md`) |

The audit point is the `master` commit that merged D1: the request
names the D1 merge as `a7776b6`, and the fetched `origin/master` tip is
that commit. Every citation below was read at that content. The local
`master` reference is stale at `8655201` (2026-10-12); the audit read
the fetched `origin/master`, not the stale reference.

## Preconditions

Each precondition was checked in the code and the cited artifacts at
the audit point. The result and its evidence follow.

### PC-0: H1 has merged with every task complete. PASS. H2 and H3 have merged too.

- `specs/015-benchmark-harness-core/` (H1), `specs/016-harness-registration-and-fixtures/`
  (H2), `specs/017-harness-timing-modes/` (H3), and
  `specs/018-docs-main-page/` (D1) stand in the audit-point tree with
  `spec.md`, `plan.md`, `tasks.md`, and `checklists/`. Each `tasks.md`
  carries zero open task boxes: the ledgers closed at their merges.
  H1 merged as pull request 32 and H2 as pull request 33, with merge
  dates, on 017's own audit record
  (the PC-0 precondition of `specs/017-harness-timing-modes/spec.md`).
  H3 merged after
  that audit; D1 merged last, and its merge is the audit point of
  this spec.
- The harness names this request uses were confirmed at the audit
  point, with file and symbol:
  - `TimeUnit`, `RunOutcome`, `RowKind`, `MetricValue`,
    `Statistics`, `ResultRow`, and `BenchmarkResult`
    (`include/speedgun-ng/benchmark.hpp`).
  - `State` (same header), with `setIterationTime` and
    `pauseTiming`.
  - The `timeUnit` field of `BenchmarkResult` (same header),
    with its doxygen block.
  - `BenchmarkHandle` (same header), with `minTime`,
    `warmupTime`, `repetitions`, `iterations`,
    `measureProcessCpuTime`, `useRealTime`, `useManualTime`,
    `unit`, `addMetric`, `setup`, and `teardown`.
  - `setDefaultTimeUnit`, `getDefaultTimeUnit`, and
    `speedgunMain` (same header).
  - The registry record `RegistryEntry`
    (`source/harness/detail/internal.hpp`).

This spec extends that surface. It replaces no shipped signature.

### PC-1: the result model. PASS.

- Row type: `ResultRow` (`include/speedgun-ng/benchmark.hpp`)
  carries `kind` (a `RowKind` of `REPETITION` or `AGGREGATE`),
  `iterations`, `timePerIterationNs` (the real column),
  `cpuTimePerIterationNs` (the CPU column), `endpointSamples`,
  `timeAvailability`, `metrics`, `overheadFloorNs`, `timeStats`, and
  `metricStats`.
- Statistics fields: the `Statistics` record (same header) carries
  `samples`, `mean`, `median`, `sampleStdDev`,
  `coefficientOfVariation`, `min`, and `max`.
- Metric value with its disclosure fields: the `MetricValue` record
  (same header) carries `value`, `runningRatio`, `scaled`, and
  `availability` (an availability kind plus a reason string).
- `BenchmarkResult` (same header) carries `name`, `suite`,
  `caseName`, `outcome`, `rows`, `reason`, `metricLabels`, and
  `timeUnit`.
- Report printing: `printResult` (`source/harness/report.cpp`)
  prints a repetition row as the instance name, `iterations=`, the two
  time columns under the row unit, `overhead floor (<unit>)=`, and
  `endpoint samples=`, then the metric fields
  `ratio=`, `scaled=` (a `scaled` flag), and `gap=` on the same line,
  with the refusal kind printed in place of a missing number. The
  aggregate row prints as the instance
  name, `AGGREGATE`, `primary time/iter (<unit>)`, then
  `mean= median= sd= cv= min= max= n=`,
  followed by one statistic line per attached metric. A failed or
  skipped run prints `FAILED: <reason>` or `SKIPPED: <reason>`
  through the same function.

### PC-2: the primary time. PASS.

- Which column is primary per mode: `Runner::run`
  (`source/harness/runner.cpp`) sets the decision value
  of a run to the manual sum when manual time stands,
  to the real fold under `useRealTime`, and to the CPU fold
  otherwise (the assignment of `record.decisionNs`, stated with its
  rule comment in the same function).
- Calibration reads the same value: `qualifies` stops at
  `record.decisionNs >= targetNs`
  (`source/harness/detail/calibration.hpp`), and
  `nextIterationCount` scales from `record->decisionNs` (same
  header, with `growUntilQualified`).
- The aggregate reads the primary column: the aggregate assembly in
  `Runner::run` collects
  `row.timePerIterationNs` for manual and real rows and
  `row.cpuTimePerIterationNs` otherwise, skipping every repetition
  whose primary fold carried a disclosure. The disclosure of a row
  is anchored to the primary core: the CPU core by default, the real
  core under the real selection, and no fold for a manual run (the
  primary-core selection in the same function).

### PC-3: the pause limit is gone. FAIL. Q-7 applies.

- `kMaxPausePerRun` stands in
  `include/speedgun-ng/benchmark.hpp`; `State::pauseTiming`
  breaches with a count at the bound (its `SG_REQUIRE` guard); the
  capacity argument of the `plan->recorder` call in `Runner::run`
  reserves room as `2 * (1 + kMaxPausePerRun) * (kRunBound +
  kRunBound + repetitions)`
  (`source/harness/runner.cpp`).
- Tests assert the bound: a run of `kMaxPausePerRun + 1` pauses must
  breach (`test/source/harness_pause_test.cpp`), and the
  `ninthPause` re-exec mode asserts the breach at the process exit
  (the same file's mode table and its parent-side check).
- A calibrated run that pauses in every iteration therefore cannot
  complete once the iteration count exceeds eight. The request's
  Q-7 decision lands the removal as this spec's own early work:
  FR-024 to FR-026. The page texts that name the limit travel with
  the fix (FR-039, FR-042).

### PC-4: the default time unit resolves at the start of `speedgunMain`. FAIL. Q-7 applies.

- A registration captures the process default at its own moment:
  `addRegistryEntry` (`source/harness/registry.cpp`) copies
  `getDefaultTimeUnit()` into the entry and
  states that a later `setDefaultTimeUnit` reaches no earlier
  registration. The default
  itself is a function-local static of the registration path, and
  `setDefaultTimeUnit` (same file) writes it at call time.
- `SG_BENCHMARK` (`include/speedgun-ng/benchmark.hpp`) expands into a
  namespace-scope initializer that registers during static
  initialization, before `main` runs, so a `setDefaultTimeUnit` call
  in `main` reaches no macro-registered benchmark that sets no unit.
- The main page states the intended behavior anyway: the
  "The time unit" section of README.md
  teaches "Call `setDefaultTimeUnit` in `main` before `speedgunMain`,
  and `TimeUnit` names the four scales a column can print in", and
  the section's opening sentence calls `setDefaultTimeUnit` "the unit
  for the families that call no `unit`"; both read as if a `main`
  call reaches the macro registrations, and today it does not. Q-7
  moves resolution to the start of `speedgunMain` as this spec's own
  early work: FR-027.

### PC-5: the constitution carries the four rules. PASS.

- Version 2.19.0 (the amendment record row and the footer of
  `.specify/memory/constitution.md`).
- Counters-only rule, exact text of its opening: "every time and
  counter value that speedgun code outside the counters library
  measures or reports comes from the `sg::counters` library, and a
  plan, a recorder, a fold, or a value a plan publishes supplies it"
  (the counters-only clause of the constitution's Additional
  Constraints). The rule
  binds `source/`, `include/`, `example/`, `test/`, and `tools/`; it
  lists the forbidden time sources; it names one exception.
- Time-source gate: Principle VIII lists the scan as a hard gate and
  `test/time_source_gate.sh` enforces it (the gate list and
  Additional Constraints of the constitution).
- The constitution's Principle V.1 carries naming rules N-1 to N-12.
- Hand-kept `SOVERSION` rule: "Public API changes are breaking
  changes: version them deliberately, document them in the spec or
  DCR, keep `VERSION`/`SOVERSION` consistent with the change"
  (the hand-kept version rule of the constitution). The 0.x spelling
  of the rule is stated in the version comments above the counters
  and harness target properties in `CMakeLists.txt`.

### PC-6: the fake provider scripts every clock leaf the primary time reads. PASS.

- `FakeProvider` (`include/speedgun-ng/counters_fake.hpp`)
  publishes scripted leaves through `addCounter` and
  `setPoints`.
- The harness tests script each leaf the primary time can read:
  `machine/monotonic` and `machine/thread_cpu`
  (`test/source/harness_pause_test.cpp`), the three machine
  time leaves together
  (`test/source/harness_time_columns_test.cpp`), and
  `machine/process_cpu` (`test/source/harness_cpu_leaf_test.cpp`).
- Every test of this spec runs on the same provider, as H3's tests do.

### PC-7: D1 docs-main-page has merged. PASS.

- D1 is spec `018-docs-main-page`, merged at the audit point itself
  (`a7776b6`, the `origin/master` tip). Constitution 2.19.0 carries
  IV.1, "Main-page documentation per feature"
  (IV.1 of the constitution): every feature that changes
  user-visible behavior adds or updates its main-page section with a
  working example, and IV.1 today defines "working" as compiles.
- The main page has the D1 structure: the "What speedgun does"
  section, the "How you do it" section with twelve
  feature sections, the "Quick reference" section, and
  the "Planned work" section of `README.md`.
- The example gate runs in CI: the lint job runs
  `tools/docs/example_drift.py` directly
  (`.github/workflows/ci.yml`), and the gate stands as the hard
  gate of Principle VIII, through ctest as
  `example_drift`, `example_drift_fixtures`, `snippet_compile`,
  `report_shape`, and `report_shape_fixtures`
  (registered in `test/CMakeLists.txt`).

## Roadmap position

This is spec H4 of the speedgun harness roadmap. H1 (015) shipped the
single-thread runner with attached counter metrics and repetition
aggregates; H2 (016) added families, fixtures, and callbacks; H3 (017)
added timing modes, pause and resume, and time units. D1 (018) set the
main-page format and its drift gate. This spec completes Google
Benchmark feature F-5 (custom counters, custom statistics, report
modes) and the part of F-6 that needs no raw-data capture (complexity
fitting). It gives speedgun the reporting features of Google Benchmark
except JSON output and histograms: JSON export and raw-sample capture
are the next roadmap spec, percentile histograms and reloaded pass or
fail judgments follow it. The spec needs no hardware: every test runs
on the fake provider. The roadmap document sits outside this
repository and is absent from this machine at the audit point; the
scope boundary below comes from the request.

## Google Benchmark reference read

Google Benchmark is a reference read, nothing more. No vendored copy
enters this spec, and the harness links nothing from it. The revision
read is `google/benchmark` `main` at
`e662de9aab8e705ecf4fa4bd41a207e5a0acfd0c` (2026-10-08 15:52:18Z),
the same revision H1, H2, and H3 read (the Google Benchmark
revision row of the 015 spec's Citations). Each
semantic this spec takes, with the file and the function of that
revision, read from the fetched files at the audit date:

| Semantic taken | Source at the revision |
| --- | --- |
| The state holds named user counters, one value each, as a map | `include/benchmark/state.h` (`State::counters`); `include/benchmark/counter.h` (`UserCounters`) |
| Counter flags: rate, average over threads, iteration invariant, average over iterations, and invert; and a base of 1000 or 1024 | `include/benchmark/counter.h` (`Counter::Flags` with its `kInvert` enumerator; `Counter::OneK`; the value, flags, and base fields and constructor) |
| The flags are applied after the run in a fixed order: the rate divides by the run seconds, the thread average divides by the thread count, the iteration-invariant multiplies by the iteration count, the average over iterations divides by it, and the invert is always last | `src/counter.cc` (`internal::Finish(Counter const&, …)` and its per-counter map loop) |
| The finish call passes the run's total seconds divided by the thread count, and the per-thread counters are merged into the shared result before the finish | `src/benchmark_runner.cc` (`BuildFinalResult` counters, the thread division and `internal::Finish`; the `internal::Increment` merge) |
| `SetBytesProcessed` sets a named rate counter over bytes with base 1024; `SetItemsProcessed` sets a named rate counter over items | `include/benchmark/state.h` (`SetBytesProcessed`, `SetItemsProcessed`) |
| `SetLabel` stores one label for the row; the aggregate keeps the label only when every repetition shares it | `include/benchmark/state.h` (the `State::SetLabel` declaration); `src/benchmark.cc` (`State::SetLabel`); `src/benchmark_runner.cc` (the label copy onto the report); `src/statistics.cc` (`ComputeStats` label rule) |
| `ComputeStatistics` adds a named statistic over the repetition values, declared with a time unit or a percentage unit | `include/benchmark/benchmark_api.h` (`ComputeStatistics`); `include/benchmark/statistics.h` (`StatisticUnit`, `StatisticsFunc`, `internal::Statistics`); `src/statistics.cc` (`ComputeStats` and its custom-statistic loop) |
| `ReportAggregatesOnly` restricts every report output to the aggregate rows; `DisplayAggregatesOnly` restricts the display alone | `include/benchmark/benchmark_api.h` (`ReportAggregatesOnly`, `DisplayAggregatesOnly`); `include/benchmark/statistics.h` (`AggregationReportMode`); `src/benchmark_runner.cc` (the mode consumption); `src/benchmark.cc` (`report_one` per output) |
| `SetComplexityN` records the problem size of a run | `include/benchmark/state.h` (`SetComplexityN`); `include/benchmark/statistics.h` (`ComplexityN`) |
| `Complexity` selects a fitted class or a user function; the classes are constant, N, N², N³, log N, N log N, and auto | `include/benchmark/benchmark_api.h` (the `Complexity` overloads); `include/benchmark/statistics.h` (`BigO`, `BigOFunc`) |
| The fit minimizes the sum of squared absolute errors by least squares; auto tries each curve and keeps the least RMS, with the constant as the fallback | `src/complexity.cc` (`LeastSquaresComplexity`; the `MinimalLeastSq` auto path) |
| Per fitted quantity the fit reports two aggregate rows: the class with its coefficient, in that quantity's unit, and the RMS as a percentage, unscaled by the unit multiplier | `src/complexity.cc` (`ComputeBigO`, its BigO row and its RMS row) |
| A family with fewer than two instances gets no fit; an instance with no positive N cannot be fitted | `src/complexity.cc` (`ComputeBigO` guards) |
| The aggregate summarizes each named counter across the successful repetitions; the flags of one name must agree across runs; custom counters are not rescaled into the aggregate | `src/statistics.cc` (`ComputeStats` counter collection and its no-rescale note) |
| The console prints a counter value scaled by its declared base | `src/console_reporter.cc` (the counter print with `oneK`) |

Semantics deliberately not taken: threads, so the thread-average
flags divide by one here until the threads spec lands (Q-3); the
`benchmark_report_aggregates_only` and display command-line flags,
which belong to the full command-line spec (Q-5); the
`kStringifyExponent` flag, which no roadmap feature names; the
performance-counter auto-counters, which ride the perf-counter
collector this harness does not have; JSON and CSV reporters, which
are the next roadmap spec; memory and allocation counters; profiler
hooks; and the `--benchmark_counters_tabular` layout. The divisor of a
rate differs by design where it must: the cited revision divides by
the instance's total seconds on the chosen timing kind, and this
harness divides by the row's primary time, which is the same value on
one thread (FR-004). The cited revision's `double` seconds become
integer nanoseconds here, as they are throughout the harness.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Report user counters with flags (Priority: P1)

A developer measuring a cache routine wants the row to say `hits` and
`misses` beside the times. They set the counter values from inside the
timed loop; the harness prints one column per counter. They mark a
counter as a rate to see events per second, or iteration invariant to
see the value per run in place of the value per iteration, or a ratio to see the
value averaged per iteration, or inverted to see a latency from a
count.

**Why this priority**: the counters map is the core of Google
Benchmark feature F-5 and of every throughput feature below.

**Independent Test**: register a benchmark that sets two named
counters on a scripted provider whose primary time is a fixed
per-iteration value, run it, and read the printed columns against the
values the cited `Finish` order produces for each flag, with the
inverted form last (the cited `Finish` in
`src/counter.cc`).

**Acceptance Scenarios**:

1. **Given** a benchmark that sets `hits` to 4 per iteration, **When**
   it runs with a primary time the script fixes, **Then** the row
   prints `hits` with the flag-neutral value and no clock sample was
   taken for it.
2. **Given** the same counter marked with the rate flag, **When** the
   run finishes, **Then** the column shows the value divided by the
   row's primary time in seconds.
3. **Given** a counter marked iteration invariant and rate, **When**
   the run finishes, **Then** the value is multiplied by the iteration
   count before the same division.
4. **Given** a counter marked inverted, **When** the flags are
   applied, **Then** the reciprocal is taken after every other flag
   has been applied.

---

### User Story 2 - Read throughput as bytes and items per second (Priority: P1)

A developer measuring a parser says "this iteration consumed 4 KiB".
They call the state's bytes-processed call once per iteration with the
byte count. The report shows a `bytes_per_second` column that scales
with the primary time, printed with the 1024 base so the number reads
as GiB/s-sized text. Items processed does the same for element counts.

**Why this priority**: throughput is the single most-used counter in
Google Benchmark's own examples and rides the same machinery as US1.

**Independent Test**: set `n` bytes per iteration on a scripted run
whose primary time is fixed, and check the printed rate equals
`n × iterations ÷ primarySeconds`, scaled by the base the cited
`SetBytesProcessed` chooses (the cited `SetBytesProcessed` in
`include/benchmark/state.h`).

**Acceptance Scenarios**:

1. **Given** a benchmark that declares 1024 bytes per iteration,
   **When** it runs with a scripted primary time, **Then** the report
   shows the rate over that primary time.
2. **Given** a family under `useRealTime`, **When** it reports,
   **Then** the rate divides by the real column of the row.
3. **Given** a manual-time benchmark that reports 2000 ns per
   iteration and 500 items, **When** it runs, **Then** the items rate
   divides by the manual sum.

---

### User Story 3 - Pause in every iteration; set the default unit in `main` (Priority: P1)

A developer measures an allocator and excludes the `malloc` from every
iteration with a pause. Today the eighth pause in one run fails the
benchmark. After this spec the run completes at any pause count the
calibration chose. The same developer sets the process default unit in
`main`; the macro-registered benchmarks they never touched print in
that unit.

**Why this priority**: both behaviors are already promised by the
shipped main page and block the counter examples of this spec,
which pause per iteration and print in the default unit (Q-7).

**Independent Test**: run a calibrated benchmark that pauses in every
iteration on the fake provider and read its completed row; call
`setDefaultTimeUnit` at the top of `main` before `speedgunMain` and
check a `SG_BENCHMARK`-registered benchmark that calls no `unit`
prints in the new unit.

**Acceptance Scenarios**:

1. **Given** a benchmark that pauses and resumes in every loop
   iteration, **When** calibration gives it one hundred iterations,
   **Then** the run completes and the row nets every pause.
2. **Given** a source tree after the fix, **When** it is searched for
   the old per-run pause allowance, **Then** no such constant or guard
   is found.
3. **Given** `main` that calls `setDefaultTimeUnit(MICROSECONDS)`
   before `speedgunMain`, **When** a macro-registered benchmark that
   calls no `unit` runs, **Then** its row prints under the `us` label.
4. **Given** the same process, **When** a benchmark calls
   `unit(SECONDS)`, **Then** its row prints under the `s` label.

---

### User Story 4 - Mark a row with a label (Priority: P2)

A developer compares two runs of one family by marking each with a
short free-text label ("avx2", "scalar"). The report prints the label
in a trailing field of the row so the rows are readable side by side.

**Why this priority**: labels organize the report but add no
measurement; they ride US1's row machinery.

**Independent Test**: set a label during the run of one scripted
benchmark and check the printed row ends with its label in the
trailing field; run repetitions whose labels disagree and check the aggregate
line carries no label (the cited `ComputeStats` label rule in
`src/statistics.cc`).

**Acceptance Scenarios**:

1. **Given** a benchmark whose body sets the label `avx2`, **When**
   it runs, **Then** its row ends with the field ` label=avx2`.
2. **Given** repetitions that set different labels, **When** the
   aggregate prints, **Then** the aggregate shows no label.

---

### User Story 5 - Add custom statistics to the aggregate (Priority: P2)

A developer wants a trimmed mean, or a 95th percentile of repetition
times, beside the built-in mean and standard deviation. They add a
named statistic over the repetition values through the handle and
declare whether it prints as a time or a percentage. The aggregate
prints it among the built-in fields, in its declared unit.

**Why this priority**: the feature completes F-5's statistics surface
but the report is whole without it.

**Independent Test**: add a statistic that returns the range of the
repetition values on a scripted three-repetition run and check the
printed value against the scripted times, in the declared unit; add a
percentage statistic and check it prints with a percent meaning and
without unit scaling.

**Acceptance Scenarios**:

1. **Given** a handle that adds a statistic `range` over the
   repetition values in the time unit, **When** three scripted
   repetitions print, **Then** the aggregate shows `range` with its
   value beside `mean=` and `sd=`.
2. **Given** a statistic whose function refuses its input, **When**
   the aggregate prints, **Then** that statistic prints no value and
   the run still reports.

---

### User Story 6 - Show only the aggregates (Priority: P2)

A developer running twenty repetitions for a summary does not want
two hundred repetition rows. One handle call reports only aggregate
rows everywhere the report goes; a second call trims the console
display while a future file output keeps the per-repetition rows.

**Why this priority**: a report-shape convenience over shipped
repetitions; it gates nothing else.

**Independent Test**: set each mode on a scripted three-repetition
family and check the output it governs: the first mode shows no
repetition rows, the second trims the console while the per-repetition
rows remain in the other output channel the mode is defined against
(today: with only the console alive, both modes trim the console).

**Acceptance Scenarios**:

1. **Given** a family with three repetitions and report-aggregates
   only, **When** it runs, **Then** only the aggregate row prints.
2. **Given** a family with three repetitions and display-aggregates
   only, **When** it runs, **Then** the console shows only the
   aggregate row.
3. **Given** a family with one run, **When** either mode stands,
   **Then** the single row still prints: the mode hides nothing that
   has no aggregate.

---

### User Story 7 - Fit the time against the problem size (Priority: P2)

A developer sweeps N = 8, 16, 32, … and asks what the growth is. The
runner fits the time per iteration against N over the family's
instances by least squares and prints, for every fitted quantity, the
fitted class with its coefficient and the RMS error as a percentage:
the primary time, each named user counter, and each attached metric.
With the class fixed to N·log N by the handle, the fit uses that
curve.

**Why this priority**: the last capability of this spec's F-6 slice;
it consumes families (H2) and needs no counter machinery.

**Independent Test**: script a family whose per-iteration time is a
known quadratic in N and check the reported class, coefficient, and
RMS against the computed fit (the cited `MinimalLeastSq` auto path
in `src/complexity.cc`); check a
user-supplied curve function is honored.

**Acceptance Scenarios**:

1. **Given** a family over N with scripted times of known order and
   its complexity class set to auto, **When** the family reports,
   **Then** each fitted quantity shows its class and coefficient in
   its own unit and its RMS as a percentage: one pair of rows for the
   time, one pair per named counter, one pair per attached metric.
2. **Given** the same family with its class fixed to constant,
   **When** it reports, **Then** the fit uses the constant curve
   whatever the RMS.
3. **Given** a family of one instance with complexity selected,
   **When** it reports, **Then** no fit row prints and the reason
   names the two-instance floor.

---

### User Story 8 - Read the page, copy the block, run the example (Priority: P3)

A reader opens the main page, finds a section for each new feature in
the same shape as the D1 sections (one purpose sentence, one snippet,
one result sentence), copies a block over the canonical includes, and
builds. The block is a marked region of an example that CI builds and
runs, and the run of that example prints the result the section
promises. The reader can run any section's example with one documented
command.

**Why this priority**: the documentation obligation of Principle IV
travels with every capability but delivers value after them.

**Independent Test**: for every main-page section, old and new,
extract its region, run the example that owns it, and check every
registered benchmark in that example prints a result row with no
`FAILED` row; every skipped row matches an expected-skip marker of
the example file; check the drift gate passes over the
page.

**Acceptance Scenarios**:

1. **Given** the six new feature sections and the twelve D1 sections,
   **When** CI builds and runs the example suite, **Then** every
   section's example runs and prints at least one result row per
   registered benchmark.
2. **Given** the fixture section's block as shipped after this spec,
   **When** a reader pastes it into the canonical file and builds and
   runs, **Then** it prints a row and not `FAILED: the benchmark
   never entered the timed loop`.
3. **Given** the registration section after this spec, **When** a
   reader copies its captured-argument or template block, **Then** it
   builds and runs.

---

### Edge Cases

- A manual-time run that accepted no iteration time has a primary time
  of zero. The rate flags then have no defined value: the row prints
  the counter's flag-neutral value and a disclosure naming the zero
  divisor, never an infinity (`nextIterationCount` in
  `source/harness/detail/calibration.hpp` guards the same
  value for growth).
- A counter whose run carried a gap disclosure keeps that disclosure
  on its row column and stays out of the counter's aggregate
  statistic, the same exclusion the time statistics already apply
  (the aggregate assembly in `source/harness/runner.cpp`).
- A counter name collides with an attached metric label: the row
  cannot show two columns under one name, so the benchmark fails with
  a reason naming the collision.
- A body that opens a counter name past the sixteenth: the benchmark
  fails with a reason naming the cap, and no row prints.
- A counter name that changes its flag set or base between the
  repetitions of one run set is a contract breach; the benchmark
  fails with a reason, as the cited revision requires flags to agree
  (the flags check of `ComputeStats` in `src/statistics.cc`).
- A counter set outside the timed loop (a callback state or before the
  loop) adds no sampling action; the loop-shape gate stays green.
- A custom statistic named like a built-in field (`mean`, `median`,
  `sd`, `cv`, `min`, `max`) is rejected with a reason at registration.
- A label containing a newline is a contract breach at the call; any
  other text prints as given in the trailing ` label=` field.
- A complexity family whose instance carries no positive problem size
  after the fit is selected: that benchmark fails with a reason,
  where the cited revision aborts (the `ComputeBigO` guards in
  `src/complexity.cc`).
- A fitted quantity that some instance of the family does not carry:
  the fit rows for that quantity state the named reason instead of
  fitting a partial instance set.
- A fit over instances whose times are all zero or equal: the class
  prints with the computed coefficient and RMS, no special case.
- Report-aggregates-only with a single run prints the single row
  unchanged: there is no repetition set to hide.
- An example whose benchmark prints `FAILED` or skips: the new run
  check fails CI, so a snippet that drifts into a fragment cannot ship
  behind a compiling block.
- The README near its line cap while six sections must join: prose is
  trimmed first, and no snippet is dropped to fit (Q-8).
- A CLI seconds value against a handle nanoseconds value: both
  statements stay true and no nanoseconds CLI value returns
  (FR-043).

## Requirements *(mandatory)*

### User counters

- **FR-001**: The state shall hold named user counters, one value
  each, as the counters map of the state does at the cited revision
  (`State::counters` in `state.h`; `UserCounters` in
  `counter.h`). A benchmark body shall be able to
  set and update a counter by name during its run. A benchmark shall
  carry at most sixteen named counters; opening a seventeenth name
  fails the benchmark with a reason that states the cap (clarified
  2026-10-14).
- **FR-002**: Each user counter shall carry flags for rate, average
  over threads, iteration invariant, average over iterations, and
  invert, and a base of 1000 or 1024, as the cited counter does
  (`Counter::Flags` and `Counter::OneK` in `counter.h`). Combinations
  shall be legal as at the cited
  revision.
- **FR-003**: The runner shall apply the flags after each run, in the
  order of `Finish` in the cited revision: the rate divides by the
  run's primary time in seconds, the thread average divides by the
  thread count, the iteration-invariant multiplies by the iteration
  count, the average over iterations divides by it, and the invert is
  applied last and always (`internal::Finish` in
  `src/counter.cc`).
- **FR-004**: The divisor of the rate flag shall be the primary time
  of the row: the CPU selection by default, the real column under
  `useRealTime`, and the sum of accepted iteration times under
  `useManualTime` (PC-2). The divisor of the thread-average flag
  shall be one while threads are out of scope (Q-3).
- **FR-005**: A user counter value shall be a number the user
  supplies; the harness shall sample no clock for it, and the
  counters-only rule binds its divisor, which is a fold the counters
  library supplies (Q-1). A row whose primary fold carried a
  disclosure shall keep that disclosure on the counter column and
  shall keep that repetition out of the counter's aggregate statistic.
  `CounterFlag::INVERT` shall take the reciprocal of a value that
  differs from zero and leave a zero value at zero. Each flag of a
  combined set shall apply on its own, so a zero primary fold skips the
  rate divisor alone and the iteration flags still bind.
- **FR-006**: The report shall print each user counter as a labeled
  column of the row carrying the flag-applied value, never a value the
  base divides: `CounterBase` selects the `k` or `Ki` suffix a
  human-readable rendering carries, and the console prints raw numbers,
  so a counter that holds 64 prints 64 under either base. The aggregate row
  shall summarize each counter across the clean repetitions with the
  statistic set of `Statistics`, as the cited `ComputeStats` collects
  counters by name (the counter collection of `ComputeStats` in
  `src/statistics.cc`).
- **FR-007**: A counter value supplied outside the timed loop shall
  add no sampling action to the loop, and the loop-shape gate
  `test/loop_shape.sh` shall stay green.
- **FR-008**: The set of counter names, flags, and bases of one
  benchmark shall agree across its repetitions; a disagreement shall
  fail the benchmark with a reason naming the name (`Edge Cases`).

### Bytes and items processed

- **FR-009**: The state shall offer one call for bytes processed and
  one for items processed; each shall set a named rate counter as
  `SetBytesProcessed` and `SetItemsProcessed` do at the cited
  revision, bytes with the 1024 base and items with the 1000 base
  (`SetBytesProcessed` and `SetItemsProcessed` in
  `state.h`).
- **FR-010**: The two calls shall print as rates over the primary
  time of the row under every timing mode, following FR-004's
  divisor.

### Labels

- **FR-011**: The state shall accept one label for its row, as
  `SetLabel` does (the `State::SetLabel` declaration in `state.h`
  and its definition in `src/benchmark.cc`), and
  the report shall print the label as a dedicated trailing
  ` label=<text>` field on the row, printed only when the row carries
  a label, with the instance-name field unchanged (clarified
  2026-10-14).
- **FR-012**: The aggregate row shall carry the label only when every
  repetition shares it, printed in the same trailing field, as
  `ComputeStats` decides (the label rule of `ComputeStats` in
  `src/statistics.cc`).

### Custom statistics

- **FR-013**: A handle call shall add a named statistic computed over
  the repetition values of the primary time, as `ComputeStatistics`
  does (`ComputeStatistics` in `benchmark_api.h`; the
  custom-statistic loop of `ComputeStats` in
  `src/statistics.cc`).
  The built-in aggregates shall keep their H1 form (`mean`, `median`,
  `sd`, `cv`, `min`, `max`, `n`).
- **FR-014**: Each custom statistic shall declare a time unit or a
  percentage unit (`StatisticUnit` in
  `statistics.h`). A time-unit statistic shall
  print in the row's unit; a percentage statistic shall print
  unscaled by the unit.
- **FR-015**: A statistic function that refuses its input shall
  produce no printed value for that statistic, and the run shall
  still report.

### Aggregate-only modes

- **FR-016**: A handle call shall restrict the report output to the
  aggregate rows, as `ReportAggregatesOnly` does
  (`ReportAggregatesOnly` in `benchmark_api.h`). Today the console is the only report
  output, so the restriction reaches it; a later file output inherits
  the mode.
- **FR-017**: A second handle call shall restrict the console display
  alone, as `DisplayAggregatesOnly` does
  (`DisplayAggregatesOnly` in `benchmark_api.h`), and
  shall leave per-repetition rows to every other output channel the
  mode is defined against. While the console is the only channel, the
  two modes print the same trimmed console.
- **FR-018**: Neither mode shall hide a row that has no aggregate: a
  single-run instance prints its one row under either mode. The
  command-line forms wait for the full command-line spec (Q-5).

### Complexity fitting

- **FR-019**: The state shall accept a problem size N for its run, as
  `SetComplexityN` does (`SetComplexityN` in
  `state.h`).
- **FR-020**: A handle call shall select a complexity class from the
  cited curve list, including auto, or a user-supplied curve function,
  as `Complexity` does (the `Complexity` overloads in
  `benchmark_api.h`; `BigO` and `BigOFunc` in
  `statistics.h`).
- **FR-021**: The runner shall fit the coefficient by least squares of
  the absolute error over the family's instances, by the closed form
  `k = sum(t * f(N)) / sum(f(N)^2)`, and shall report the RMS as
  `sqrt(sum((t - k * f(N))^2) / n) / mean(t)` as a percentage. Auto
  shall try each curve of the cited list and keep the least RMS, with
  the constant curve as the fallback (`LeastSquaresComplexity` and the
  `MinimalLeastSq` auto path in `src/complexity.cc`).
- **FR-022**: The fit shall be one per family over its instances and
  shall report, for each fitted quantity, two extra aggregate rows:
  the fitted class with its coefficient in the unit of that quantity,
  and the RMS as a percentage, unscaled by the unit multiplier. The
  fitted quantities are the family's primary time, each named user
  counter, and each attached metric (clarified 2026-10-14;
  `ComputeBigO` in `src/complexity.cc`).
- **FR-023**: A family of fewer than two instances, or an instance
  without a positive N while a fit is selected, shall report no fit
  row and shall state the named reason, in place of the cited
  revision's hard check (the `ComputeBigO` guards in
  `src/complexity.cc`).

### Leftovers the fix precedes the feature (Q-7)

- **FR-024**: The source shall hold no per-run pause allowance: the
  constant `kMaxPausePerRun` and its `SG_REQUIRE` guard in
  `State::pauseTiming` (`include/speedgun-ng/benchmark.hpp`), and
  the bound tests and the `ninthPause` re-exec mode
  (`test/source/harness_pause_test.cpp`)
  shall be removed (PC-3).
- **FR-025**: The recorder's pause room shall be reserved at plan
  build from the family's observed pause history, in place of the
  fixed allowance of the capacity argument of the `plan->recorder`
  call in `Runner::run`; a run whose closed pairs outrun the
  reservation shall net them by folding, so no closed pair is lost
  and any number of closed pause intervals reports correct columns
  with bounded memory (Q-10, FR-051 to FR-054).
- **FR-026**: A calibrated run that pauses in every iteration shall
  complete, and every pause shall net out of every time column and
  metric fold of its row as it does today.
- **FR-027**: The default time unit shall resolve when `speedgunMain`
  starts, so a `setDefaultTimeUnit` call in `main` reaches every
  macro-registered benchmark that sets no unit; a `unit` call shall
  keep overwriting it; a benchmark run outside `speedgunMain` shall
  keep the documented process default (PC-4). The contracts of
  `setDefaultTimeUnit` and of `BenchmarkResult::timeUnit` (the
  captured-at-registration sentence of its doxygen block) shall be
  amended to the new resolution
  point.

### Main-page documentation (D1 rule)

- **FR-028**: The main page shall gain one "How you do it" section,
  in the D1 shape of purpose sentence, marked snippet, and result
  sentence, for each of: user counters and their flags; bytes and
  items processed as throughput; labels; custom statistics; the
  aggregate-only modes; and complexity fitting. Each snippet shall be
  a marked region of an example file CI builds and runs, and the
  drift check shall pass.
- **FR-029**: The quick reference shall gain a row for each new API
  name, and the option table shall stay unchanged, since no new
  command-line option lands (Q-5).
- **FR-030**: The "Reading the report" section shall show the new
  columns and rows: counter columns, the label field, custom
  statistic fields, and the per-quantity fit rows. The planned-work list shall
  drop the six features this spec ships and keep structured
  machine-readable output and histogram output.
- **FR-031**: `docs/pages/harness.md` shall gain the detail for each
  new feature, under feature names, with no spec numbers, in the
  reader voice the rest of the page keeps.

### Runnable examples for every section (C-9)

- **FR-032**: Every main-page section, the twelve from D1 and the six
  this spec adds, shall have a runnable example in the source tree
  under `example/docs/`, named in snake_case for its section
  (research D-12), and the page's snippet shall be a marked region
  of that example, which the drift check keeps equal.
- **FR-033**: Each such example shall have a real `main` that calls
  `speedgunMain`, a `run_docs_` target, and an entry in
  `run-examples`, through a sibling helper `add_docs_example` added
  to `example/CMakeLists.txt` (research D-12). The compile-only
  exclusion, the `add_executable` block that builds `docs_snippets`
  without a run target, retires as its regions move into runnable
  files.
- **FR-034**: A CTest check shall run each section example with the
  `--min-time` option set short (`--min-time=0.01`, seconds on the
  command line), pass only when every registered benchmark of
  the example prints a result row, and fail on any row that reads
  `FAILED`. A `SKIPPED` row shall pass only when the example file
  carries an expected-skip marker naming that instance and that
  message, and a marker whose instance prints no matching `SKIPPED`
  row shall fail the check (research D-15). CI shall run this check.
- **FR-035**: A reader shall be able to build and run any section
  example with one documented command named in the docs.
- **FR-036**: Constitution IV.1 shall be amended at the next minor
  version so a main-page snippet must come from an example CI builds
  and runs, upgrading the rule from its compile-only form (IV.1 of
  the constitution), and
  Principle VIII's example gate shall name the new run check beside
  the compile checks (Principle VIII of the constitution).

### D1 leftovers on the main page (C-8)

- **FR-037**: The fixture section shall show `TableFixture` with its
  own `setUp` and `tearDown` overrides, and every method its snippet
  registers shall carry a body with a timed loop, so a pasted copy
  runs and prints a row; the sentence in the README fixture section
  that excuses the empty body shall be removed.
- **FR-038**: The registration section shall gain a short runnable
  snippet for `SG_BENCHMARK_CAPTURE` and one for
  `SG_BENCHMARK_TEMPLATE`, each a marked region that builds and runs
  when pasted, within the per-section line cap.
- **FR-039**: With FR-024 to FR-026 removing the pause allowance, the
  sentence "a run carries at most eight closed pauses" in the README
  "Pause and resume" section and the `docs/pages/harness.md` "Pause
  and resume" texts that name the constant and its bound, with the
  re-exec mode list naming the ninth-pause scenario, shall be
  rewritten to the shipped rule.
- **FR-040**: The fixture, capture, and template snippets of FR-037
  and FR-038 shall build and run in CI, and the drift check shall
  pass over the page.
- **FR-041**: The "The time unit" texts shall state the FR-027
  resolution rule and stay true at the feature head: the README
  "The time unit" section and the `docs/pages/harness.md` "Time
  units" paragraph that
  describes the captured-at-registration default shall match the
  shipped behavior.
- **FR-042**: The pause texts of FR-039 and the time-unit texts of
  FR-041 shall name no limit or capture point the source no longer
  carries.
- **FR-043**: The handle calls `minTime` and `warmupTime` shall keep
  taking nanoseconds (the two declarations on `BenchmarkHandle` in
  `include/speedgun-ng/benchmark.hpp`) and the command-line
  options `--min-time` and `--warmup-time` shall keep taking seconds
  (the seconds parsers `parseSeconds` and `parsePositiveSeconds` in
  `source/harness/cli.cpp`); both statements shall
  stay true in the docs, and no nanosecond command-line value shall
  be reintroduced.

### Line budget (Q-8)

- **FR-044**: The page cap of the drift gate shall move from 400 to
  550 lines (the `PAGE_LINE_CAP` constant in
  `tools/docs/example_drift.py`). The D1 budget that cap belongs to
  (SC-011 and the Q-4 decision of the 018 spec) is updated by the
  record in this spec; no file under `specs/018-docs-main-page/`
  is edited. The fixed
  feature-section list (the `SECTIONS` constant of
  `example_drift.py`) shall gain the six
  new section titles. The 25-line cap per section
  (the `SECTION_LINE_CAP` constant) shall not move. Prose shall be trimmed
  first; no snippet shall be dropped to fit.

  Amendment (D-A of the PR #37 final pass, 2026-10-15): the main page
  shall teach every registration form in `Registration` and every
  fixture macro in `Fixtures`, each region copied from its example.
  `example_drift.py` shall gain a `SECTION_LINE_CAPS` table that caps
  those two sections and no other, `SECTION_LINE_CAP` shall stay 25 as
  the default for every other section, and the page cap shall move to
  570 lines, the smallest that holds the page: at 570 lines the page
  does not fit under 550, and prose elsewhere gives back less than the
  32 lines the two sections add. The drift gate fixture shall show a
  section at its own override cap passing and one line over failing.
  The macro block of `Fixtures` stands on its own only beside the
  class it registers, so a marked block may name such a companion in
  an `sg-requires` comment; the drift gate shall check that each named
  region exists, `--emit-blocks` shall prepend it to the emitted
  snippet, and the snippet compile gate shall thereby keep compiling
  every emitted block over the canonical prelude alone.

### Surface, version, and discipline

- **FR-045**: Every new identifier shall follow the constitution's
  Principle V.1 rules N-1 to N-12,
  and macros take the `SG_` prefix;
  the name check shall report zero findings.
- **FR-046**: Every new public interface shall document `\pre`,
  `\post`, and `\invariant`, and the source shall enforce each
  contract (Principle II). Handle setters shall carry the existing
  before-the-first-run and live-entry guards as the handle setters
  do (the standing `SG_REQUIRE` on those setters in
  `source/harness/registry.cpp`).
- **FR-047**: No `LCOV_EXCL` marker shall enter `source/harness/`,
  and coverage shall stay 100% line and branch.
- **FR-048**: The version shall move to the next minor, 0.9.0, and
  the hand-kept rule shall decide `SOVERSION`: the harness number
  moves from 4 to 5 while the state, the handle, or the result rows
  change layout, and the counters library stays at 2 (Q-6;
  the hand-kept version rule of the constitution; the version
  comments above the counters and harness target properties in
  `CMakeLists.txt`). The
  plan records both numbers in its version table.
- **FR-049**: The examples and the documentation shall use the new
  calls through the public `speedgun-ng` headers alone; every section
  example shall build and run in the example suite.
- **FR-050**: Every test shall run on the fake provider and stay
  deterministic; the plan records TDD mode. Every time value shall
  come from the counters library and the time-source gate shall stay
  clean. All prose shall obey Principle XI, including XI.7, with zero
  prose-gate findings.
- **FR-051**: From a family's second attempt on, the recorder shall
  reserve per leaf column two points per closed pause pair the
  previous attempt observed per iteration of that family, times the
  attempt's iteration count, plus the endpoint points of each run it
  starts; the pause room of any reservation shall not exceed
  `kMaxPauseSamplesPerLeaf` raw points per leaf column (FR-025,
  Q-10).
- **FR-052**: An attempt that reserves no pause room, a family's
  first attempt included, or whose room cannot cover its closed
  pairs, shall fold each closed pause pair into the row's running
  pause totals, one accumulator per leaf, at the commit of the pair's
  second point, from the same two raw counter points and through the
  counters library alone; the fold is the documented exception to the
  rule that every printed value is a fold over retained raw points,
  and the netted columns shall equal the raw path's (FR-026,
  FR-050).
- **FR-053**: A run whose body closes no pause shall reserve no pause
  room beyond the endpoint points of each run it starts (FR-051).
- **FR-054**: The result shall record whether its run folded, and the
  report shall disclose a folded run with a token beside the row; a
  raw run keeps the exact bytes it prints today; and
  `docs/pages/harness.md` shall state the fold rule beside the
  sentence that every printed value is a fold over the raw point pair
  (FR-052).

## Key Entities *(include if feature involves data)*

- **User Counter**: a named value the benchmark body supplies, one
  value per name per run. Carries the flag set (rate, average over
  threads, iteration invariant, average over iterations, invert), the
  base (1000 or 1024), and the flag-neutral value. Its printed column
  carries the flag-applied value and any disclosure.
- **Throughput Counter**: a preset user counter: bytes processed with
  the 1024 base and items processed with the 1000 base, each fixed to
  the rate flag over the row's primary time.
- **Label**: one short free-text string one row carries, printed in
  the row's trailing ` label=` field, kept by the aggregate only
  while every repetition shares it.
- **Custom Statistic**: a named function over the repetition values
  with a declared unit (time or percentage), printed among the
  built-in aggregate fields.
- **Fit Selection**: a family-level choice of curve: one class of the
  cited list, auto, or a user function.
- **Family Fit**: the result of one least-squares fit over a family's
  instances: the class label, the coefficient, and the RMS
  percentage; printed as one BigO row and one RMS row per fitted
  quantity (the primary time, each named user counter, each attached
  metric).
- **Report Restriction**: the family-level mode that hides
  per-repetition rows, in every output or in the display alone.
- Existing entities this spec extends, unchanged in identity:
  `ResultRow`, `Statistics`, `MetricValue`, `BenchmarkResult`, and the
  registry record (`include/speedgun-ng/benchmark.hpp`;
  `RegistryEntry` in
  `source/harness/detail/internal.hpp`).

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-01**: For a scripted primary time, each flag combination of
  FR-002 and each base prints the value the cited `Finish` order
  produces, verified per combination against the scripted numbers,
  with the invert applied last.
- **SC-02**: Bytes and items print as rates over the primary time
  under all three timing modes, to the exact scripted quotient.
- **SC-03**: Each custom statistic prints beside the built-in
  aggregates in its declared unit; the percentage statistic prints
  unscaled.
- **SC-04**: Each aggregate-only mode hides every per-repetition row
  from the output it governs and hides nothing from the other, and a
  single-run row never disappears.
- **SC-05**: A family with scripted times and counters of known order
  reports that class and its coefficient for the time and for every
  named quantity, each with the RMS percentage; a fixed class or user
  curve is honored; a one-instance family states the named reason
  instead.
- **SC-06**: Every hard gate of Principle VIII passes at the feature
  head, the time-source and loop-shape gates included.
- **SC-07**: The version moves to 0.9.0, the harness `SOVERSION`
  moves to 5, and the plan's version table records both.
- **SC-08**: The main page carries a D1-shaped section and
  quick-reference rows for each feature of FR-001 to FR-023, each
  snippet compiles in CI, and the drift check passes.
- **SC-09**: The fixture, capture, and template snippets run when
  pasted, and no page text names a pause limit or a capture point the
  source no longer carries.
- **SC-10**: Every main-page section, from D1 and from this spec, has
  an example under `example/` that CI builds and runs, and the run
  check shows a result row for every registered benchmark of every
  such example, `FAILED` rows are absent, and every `SKIPPED` row
  matches an expected-skip marker of that example file (D-15).
- **SC-14**: The run check fixtures pin the four outcomes: an
  undeclared skip fails the check, a skip whose instance and message
  match a declaration passes, an expected skip that never prints
  fails, and a `FAILED` row fails (FR-034).
- **SC-11**: A scripted run whose body closes two pause pairs in
  every iteration of a ten-thousand-iteration calibrated run
  completes with every column netted, and its reservation stays at
  or under `kMaxPauseSamplesPerLeaf` points per leaf column (FR-051).
- **SC-12**: A scripted run whose body never pauses reserves exactly
  the endpoint points and no pause room (FR-053).
- **SC-13**: On the scripted provider, a run routed through the fold
  path prints net columns equal to the same run through the raw path
  (FR-052).

## Scope

In: FR-001 to FR-054: the six capabilities, the two H3 leftovers, the
main-page sections, the runnable-example obligation, the D1 leftovers,
the budget update, and the version effect.

Out, each owned by a later roadmap spec:

- threads and the cross-thread aggregation of counter values;
- JSON reports, the raw-data report, and the full command line;
- percentiles over per-chunk samples, the percentile histograms, and
  reloaded pass-or-fail judgments.

The commit-lint findings of the H3 history sit in merged history; the
commit gate of this spec does not cover them (Q-7).

## Assumptions

- The rate divisor is the row's primary time; on one thread that is
  the value the cited revision feeds `Finish` (FR-004). A zero
  primary time prints the flag-neutral value with a disclosure
  (Edge Cases).
- A user counter is a supplied value, as manual time is; the
  counters-only rule binds the divisor alone (Q-1). A rule text that
  forbade supplied values would need a patch amendment first; the
  audit read shows the current rule binds measurement, and supplied
  numbers sit outside it (the counters-only clause of the
  constitution's Additional Constraints).
- Counter storage is fixed at registration and allocates nothing
  inside the timed loop (Q-2); the cap is sixteen named counters per
  benchmark (clarified 2026-10-14, FR-001).
- The thread-average flags divide by one until the threads spec lands
  (Q-3).
- The fit is one per family over its instances, printed as one BigO
  row and one RMS row per fitted quantity (Q-4, clarified
  2026-10-14).
- The aggregate-only modes land as handle calls; command-line options
  wait for the full command-line spec (Q-5).
- The version moves to 0.9.0 with the harness `SOVERSION` at 5, since
  the state, the handle, and the result rows change layout (Q-6).
- The pause-limit removal and the default-unit move land as the first
  tasks of this feature, since PC-3 and PC-4 fail at the audit point
  and Q-7 admits either landing site (FR-024 to FR-027).
- The page cap moves to 550 and the per-section cap stays 25 (Q-8).
  The README stands at 394 lines against the current 400 cap; the
  six new sections plus the C-8 additions do not fit under 400.
- The page cap moves again to 570, and the `Registration` and
  `Fixtures` sections take per-section caps of 38 and 43 lines from
  the `SECTION_LINE_CAPS` table of `example_drift.py`; every other
  section keeps the 25-line default (D-A, recorded 2026-10-15).
- One example file per main-page section under `example/` is the
  working layout; one runnable snippets binary is the fallback if the
  build-time cost is too high (Q-9).
- The counter names of the throughput presets follow the cited
  revision's names `bytes_per_second` and `items_per_second`.
- All tests run on `FakeProvider`; the spec needs no hardware and no
  PMU.

## Dependencies

- The counters library: folds over point pairs, availability
  disclosures, and `FakeProvider`, with its `addCounter` and
  `setPoints` calls
  (`include/speedgun-ng/counters_fake.hpp`).
- H2 family expansion and instance naming, which the fit walks
  (`source/harness/family.cpp`; `RegistryEntry` in
  `source/harness/detail/internal.hpp`).
- H3 primary-time selection, which every rate divisor reads
  (the assignment of `record.decisionNs` in `Runner::run`,
  `source/harness/runner.cpp`).
- D1 main-page structure, drift gate, and IV.1
  (the `SECTIONS`, `PAGE_LINE_CAP`, and `SECTION_LINE_CAP`
  constants of `tools/docs/example_drift.py`;
  IV.1 and Principle VIII of the constitution).
- Google Benchmark at the cited revision is a reference read only;
  nothing links it.

## Citations

Symbol and section anchors, each read at the audit point
`a7776b6`.

| Artifact | Location |
| --- | --- |
| `TimeUnit`, `RowKind`, `MetricValue`, `Statistics`, `ResultRow`, `BenchmarkResult` | `include/speedgun-ng/benchmark.hpp` |
| `State`, `setIterationTime`, `pauseTiming`, its `SG_REQUIRE` guard | `include/speedgun-ng/benchmark.hpp` |
| `kMaxPausePerRun` | `include/speedgun-ng/benchmark.hpp` |
| `BenchmarkHandle` setters | `include/speedgun-ng/benchmark.hpp` |
| `setDefaultTimeUnit`, `getDefaultTimeUnit`, `speedgunMain` | `include/speedgun-ng/benchmark.hpp` |
| Default-unit static and registration capture | `addRegistryEntry`, the default-unit static, and `setDefaultTimeUnit` in `source/harness/registry.cpp` |
| Primary-time decision values | the assignment of `record.decisionNs` in `Runner::run`, `source/harness/runner.cpp` |
| Aggregate primary column and gap skip | the aggregate assembly in `Runner::run`, `source/harness/runner.cpp` |
| Availability anchor | the primary-core selection in `Runner::run`, `source/harness/runner.cpp` |
| Recorder capacity rule | the capacity argument of the `plan->recorder` call in `Runner::run`, `source/harness/runner.cpp`; `kRunBound` in `source/harness/detail/calibration.hpp` |
| Qualification and growth on the decision value | `qualifies`, `nextIterationCount`, and `growUntilQualified` in `source/harness/detail/calibration.hpp` |
| Report row and aggregate format | `printResult` in `source/harness/report.cpp` |
| CLI seconds parsing | `parseSeconds` and `parsePositiveSeconds` in `source/harness/cli.cpp` |
| `RegistryEntry` | `source/harness/detail/internal.hpp` |
| Pause bound tests | the bound scenarios and the `ninthPause` re-exec mode in `test/source/harness_pause_test.cpp` |
| Fake provider and scripted leaves | `FakeProvider`, `addCounter`, and `setPoints` in `include/speedgun-ng/counters_fake.hpp`; the scripted-leaf scenarios in `test/source/harness_pause_test.cpp`, `test/source/harness_time_columns_test.cpp`, and `test/source/harness_cpu_leaf_test.cpp` |
| Gate scripts | `test/loop_shape.sh`, `test/time_source_gate.sh` |
| Drift gate and caps | the `SECTIONS`, `PAGE_LINE_CAP`, and `SECTION_LINE_CAP` constants and the page checks of `tools/docs/example_drift.py` |
| Example run machinery | the `add_example` helper and the `run-examples` target in `example/CMakeLists.txt` |
| Snippet regions and its main | the marked regions and the entry point of `example/docs_snippets.cpp` |
| Main-page structure and texts C-8 edits | the "What speedgun does", "How you do it", "Quick reference", and "Planned work" sections of `README.md`, with the fixture, pause, and time-unit sentences inside them |
| Harness guide sections C-8 edits | the "Pause and resume" and "Time units" sections of `docs/pages/harness.md` |
| CI drift step and ctest gate names | the lint job of `.github/workflows/ci.yml`; the gate registrations in `test/CMakeLists.txt` |
| Version, `SOVERSION` rule | the `project()` version and the counters and harness target properties, with their version comments, in `CMakeLists.txt` |
| Package compatibility | the `COMPATIBILITY` clause in `cmake/install-rules.cmake` |
| Constitution IV.1, VIII gate, counters-only, N-rules, `SOVERSION` rule, lineage, footer | IV.1, Principle VIII, Principle V.1, the Additional Constraints, the amendment record, and the footer of `.specify/memory/constitution.md` |
| D1 budget note | SC-011 and the Q-4 decision of `specs/018-docs-main-page/spec.md` |
| Google Benchmark revision | `google/benchmark` `main` at `e662de9aab8e705ecf4fa4bd41a207e5a0acfd0c` (2026-10-08 15:52:18Z) |
| GB semantics table | the reference-read section above |

## Open questions

The request carries nine questions to clarify. Each has a
recommendation, and the spec adopted the recommendation as the working
default. `/speckit-clarify` may confirm each as written or replace it.

- **Q-1 User values and the counters-only rule**: Recommendation: the
  spec names a user counter as a supplied value, as manual time is.
  The rule binds the rate divisor alone. Adopted at FR-005; the rule
  text at the audit point supports it.
- **Q-2 Counter storage**: Clarified 2026-10-14: a fixed-capacity
  store sized at registration, with no allocation inside the timed
  loop; the cap is sixteen named counters per benchmark. Recorded at
  FR-001 and Assumptions.
- **Q-3 Thread flags before threads**: Recommendation: the
  thread-average flags divide by a thread count of one until the
  threads spec lands. Adopted at FR-004.
- **Q-4 Complexity grouping**: Clarified 2026-10-14: one fit per
  family over its instances, reported per fitted quantity (the
  primary time, each named user counter, each attached metric) as one
  BigO row and one RMS row for each. Recorded at FR-022.
- **Q-5 Aggregate-only option on the command line**: Recommendation:
  the handle calls land now, and the command-line options wait for
  the full command-line spec. Adopted at FR-018 and FR-029.
- **Q-6 Version**: Recommendation: the next minor version, and
  `SOVERSION` up by one when `State`, `BenchmarkHandle`, or the result
  rows change layout. Adopted at FR-048; all three change layout, so
  the harness number moves 4 → 5.
- **Q-7 H3 leftovers**: Decision: fix the pause limit and the default
  time unit before the H4 plan; the fix lands on master first or as
  the first task of H4. PC-3 and PC-4 fail at the audit point, so the
  spec carries the fixes as FR-024 to FR-027 with the docs that must
  travel with them (FR-039, FR-041).
- **Q-8 Line cap**: Recommendation: raise the cap to 550 lines and
  keep the 25-line cap per section, recording the budget update in
  this spec; the 018 text stays as written. Adopted at FR-044. The
  README stands at 394 of 400 lines
  at the audit point; the six sections plus the C-8 additions do not
  fit below 550 with the snippets intact.
  Second record (2026-10-15, D-A adopted): Registration and Fixtures
  teach every form on the main page, and the page lands at 570 lines
  with those sections at 38 and 43 lines. The page cap moves to 570,
  the smallest fitting amount, and the two sections take caps of
  their measured spans through `SECTION_LINE_CAPS`; no other section
  moves off the 25-line default.
- **Q-9 Example layout**: Recommendation: one example file for each
  main-page section, each with its own `main`; a single runnable
  snippets binary is the fallback. Adopted at FR-032 to FR-035 and
  Assumptions.
- **Q-10 Pause sample memory** (raised at plan review as the memory
  question behind FR-025): reserving pause room of
  `2 + 2 × iterations` raw points per leaf is unbounded memory at the
  iteration cap of `kIterationCap`. **Decided by the owner: recommendation accepted
  (2026-10-14).** A family reserves pause-sample capacity only
  where an earlier attempt paused, sized from the pauses per
  iteration that attempt observed times the next attempt's iteration
  count, plus the endpoint points of each run. A fixed per-leaf
  bound, `kMaxPauseSamplesPerLeaf` (32'768 raw points per leaf
  column; one `std::uint64_t` per point, 256 KiB per leaf column at
  the cap), caps every reservation; a family's first attempt and any
  over-cap run fold each closed pair into the row's running totals
  from the same two raw points, and the report discloses the fold.
  Recorded at FR-025 and FR-051 to FR-054, SC-11 to SC-13, and
  research D-10.
- **Q-11 Harness guide internal notes** (raised with the request,
  outside the nine): the "Coverage:" paragraphs of
  `docs/pages/harness.md` name test scenario functions and re-exec
  modes of the suite, which is suite-internal material inside the
  user guide. Recommendation: fold the cleanup into this feature's
  guide work (FR-031, FR-039): rewrite each paragraph as one
  reader-facing sentence, since C-7 and C-8 rewrite that page's
  neighborhoods anyway and Principle XI keeps the page to the
  reader's vocabulary.

## Constitution Checklist

- Main-page impact check (IV.1): YES. The feature changes
  user-visible behavior, so FR-028 to FR-044 update the root
  `README.md` sections, their `example/` regions, the quick-reference
  rows, and `docs/pages/harness.md` in the same feature, and FR-036
  amends IV.1 itself to the build-and-run obligation.
