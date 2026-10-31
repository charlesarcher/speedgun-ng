# Feature Specification: Multi-pass Collection

**Feature Branch**: `024-multipass-collection`

**Created**: 2026-10-20

**Status**: Ready

**Input**: User description, opening quoted. The request of 2026-10-20 is
the owner's pasted prompt for roadmap item N6, formerly H13. Its
hard-rules, version-rule and rebase paragraphs restate constitution VIII
and IX plus the three-lane rules, so they are summarized where they bind
this feature and not copied.

> Multi-pass collection, item N6 (old H13) of the roadmap. Short name:
> multipass-collection. Target machine: corsairone (Intel, one socket).
> Lane: counters (N2 done, N6 now, then N10). It runs in parallel with
> N3 JSON reports (gary) and N4 topology placement (thrawn).
>
> A user can ask for more metrics than the hardware counters can measure
> at one time. Today a plan that is larger than the counter limit falls
> back to kernel multiplexing, and `MetricResult::runningRatio` discloses
> the fraction. After N6, the harness can split the events into passes,
> run the benchmark one time for each pass, merge the passes, and
> disclose the method and the variance between passes. N10 (top-down
> tree) needs this.
>
> Capabilities: C-1 pass planner, C-2 pass runs, C-3 merge, C-4
> disclosure, C-5 options, C-6 docs.
>
> Recommendations (accepted): a deterministic first-fit split in request
> order with metric atomicity; a metric wider than the limit stays
> multiplexed in its own pass; the planner reads the counter limit from
> the host through `sg::counters`, the fake provider takes the limit as a
> test parameter; the fixed time leaves ride every pass and count once;
> calibration runs once and every pass reuses its iteration count;
> metrics merge from the pass that measured them, the time columns and
> percentiles come from pass 0; the report shows the method, the pass
> count and the relative spread of the time per iteration between passes,
> with a one-line warning above 5 percent; the option
> `--metric-method=auto|passes|multiplex` defaults to auto and a bad
> value prints one line and exits with status 2; a one-pass run keeps
> today's console report; the split and the merge run outside the timed
> window; threads keep one pass order and passes do not overlap; unit
> tests run on the fake provider with a set limit; a real-PMU script on
> corsairone compares both methods and stands outside CI.
>
> Out of scope: the top-down tree (N10), placement (N4), new metric
> formulas. The JSON output of the pass data is in scope as FR-038.

## Decisions

The owner accepted these as recommendations for this prompt on 2026-10-20.
They are recorded here per X.1 so that no implementer reopens them, and
each one names the reading it settles.

- **D-1 The split is a deterministic first fit.** The planner walks the
  metrics in the order the caller requested them and places each metric
  in the first pass with room for all of its events. The events of one
  metric always share one pass, because an event pair split across two
  windows has no common interval to fold over. The same metric list on
  the same limit produces the same passes every time.
- **D-2 A metric wider than the limit keeps its own pass.** A metric
  whose event count exceeds the limit cannot be split under D-1, so it
  takes a pass alone that oversubscribes the PMU on purpose. The kernel
  rotates it, and the existing `runningRatio` disclosure (PC-2) reports
  the fraction. The multi-pass machinery adds no second ratio mechanism.
- **D-3 The counter limit becomes a published fact of the counters
  library.** PC-3 measured that master reads no counter limit anywhere
  and that the kernel grants an oversize group at open and rotates it
  later, so a split-on-open-failure probe observes nothing: the open
  never fails on an oversize plan. The planner therefore needs the limit
  as data. The counters library publishes the programmable-counter limit
  of an event source it can answer for, and an event source it cannot
  answer for publishes no limit. The fake provider takes the limit as a
  test parameter, so every split decision is testable without a PMU.
- **D-4 The fixed leaves are the two time leaves of the run.** PC-4
  measured the spine every harness run compiles: `machine/monotonic` for
  the real-time column and `machine/thread_cpu` (or `process_cpu` under
  the family switch) for the CPU-time column. Both come from the machine
  clock provider, so they consume no programmable counter. They ride
  every pass, and the split counts them in no pass. Master attaches no
  fixed cycles leaf: a cycles event enters a plan only through a metric
  or a `--counter` address, and such an event counts in the pass that
  carries it.
- **D-5 The merge keeps one row per (pass 0) shape.** Each metric column
  takes its samples from the pass that measured it, and the time
  columns, the overhead floor, the endpoint samples and the percentile
  columns come from the reference pass, which is pass 0. The result
  stays one `BenchmarkResult` with the same column order, so a consumer
  of today's exported types reads the same row shape.
- **D-6 The variance disclosure is a relative spread with a warning.**
  The spread is (max minus min) divided by the median of the time per
  iteration across passes, computed from the merged reference time per
  iteration of each pass. The median of an even count is the lower of
  the two central values. A spread above 5 percent prints a one-line
  warning. The disclosure reports; it never refuses the run. A run of one
  pass carries no spread, because one sample has no spread.
- **D-7 The method is one option with three values.** `auto` uses one
  pass when every event fits and passes when they do not. `passes`
  forces the planner path, which yields one pass when one pass already
  fits. `multiplex` keeps the single oversize plan and the kernel
  rotation that master performs today. The command line carries
  `--metric-method`, the handle carries the same three values, and the
  command line wins over the handle, on the precedent of
  `--percentiles` (PC-4). A bad value prints one line and exits with
  status 2.
- **D-8 The disclosure printer is a new function.** The pass lines print
  from a function of their own. `printContextDisclosure` in
  `source/harness/report.cpp` stays untouched, because N4 edits that
  function on its own lane (the request's file-overlap note).
- **D-9 A one-pass run changes no byte.** With a metric set that fits
  one pass, the console report of an `auto` run equals what master
  prints for the same run today. The pass machinery shows itself only
  when more than one pass ran or the caller forced a method.
- **D-10 The planner and the merge live outside the timed window.** The
  split runs before the run starts. Each pass opens its plan before its
  first timed window and closes it after its last, following the
  existing per-context open of PC-4. No timed window contains a split,
  a merge, a report line or an allocation of pass state.
- **D-11 An unreadable limit keeps today's behavior.** On a host whose
  event source publishes no limit (D-3), the `auto` method and a forced
  `passes` method both keep the single-plan kernel-multiplexed run and
  the report discloses the `multiplex` method. The feature never turns a
  host fact's absence into a measurement failure. (Clarified
  2026-10-20.)
- **D-12 The docs section is a new capability section.** The main page
  gains one section, "Multi-pass collection", from a new owner file under
  `example/docs/` with its own `run_docs_` target and run-gate entry. The
  budget note records that the page sits at its cap, so the added lines
  need a recorded `PAGE_LINE_CAP` raise.

## Audit point

| Field | Value |
| --- | --- |
| Audit point | `7a75e95e19ff6198efe0a3db6705f856f4339933` |
| Audit-point date | 2026-10-20 10:49:02 -0500 (commit date) |
| Default branch at run time | `master`, tip `7a75e95`, equal to the `origin/master` tip |
| Working tree at run time | clean at that commit (`git status --porcelain` printed nothing) |
| Open pull requests at run time | none (`gh pr list` printed an empty list) |
| Development host | `corsairone`, Intel i9-9920X, CPUID family 6, model 85 (`0x55`), stepping 4 |
| Project version | 0.12.0 (`VERSION` in `CMakeLists.txt`) |
| Counters archive `SOVERSION` | 2 (`speedgun-ng_speedgun-ng` target property) |
| Harness `SOVERSION` | 7 (`speedgun-ng_harness` target property) |

The request binds no spec number beyond a first choice, and it binds no
commit and no source line. The number resolved at run time: `specs/` on
master holds 001 to 022, no pull request is open, and 023 stands reserved
for N3 (JSON reports) by the owner's instruction, so this feature is 024.
The branch base is the audit point itself: 7a75e95, 2026-10-20, the state
the request names (N1 and N2 merged).

## Preconditions

Each check read the audit-point tree on corsairone.

### PC-1: the metric API of N2. PASS.

- `include/speedgun-ng/counters_metrics.hpp` declares `MetricRefusal`
  (line 33), `MetricView` with `expression`, `group` and `unit`
  (line 63), `metricByName(std::string_view) ->
  std::expected<MetricView, Error>` (line 81), `MetricListing`
  (line 91) and `catalogMetrics()` (line 113). The names the request
  cites are the names on master.
- `source/counters/linux_pmu/metric_formula.cpp` holds the loader and the
  `metricByName` body (line 1653); `metric_formula.hpp` and
  `metric_tables.hpp` sit beside it in `source/counters/linux_pmu/`.

### PC-2: the fold and result types. PASS.

- `MetricResult` in `include/speedgun-ng/counters_core.hpp` holds
  `value`, `runningRatio`, `availability` and `scaled`. Its document
  states that `runningRatio` is "the multiplex disclosure over the
  window", which is the field N6's disclosure must keep working for the
  metric that stays multiplexed (D-2).
- `Availability` in the same header (line 87) holds `COUNTABLE`,
  `PERMISSION_BLOCKED`, `NOT_ENCODABLE`, `ABSENT`, `SCOPE_REFUSED` and
  `GAP`.
- `detail::foldPairs` and `detail::metricCore` live in
  `include/speedgun-ng/counters_measurement.hpp`; the folds land in
  `source/counters/fold.cpp`, which sets `runningRatio` from the group
  disclosure (lines 310 and 323).

### PC-3: group scheduling, the counter limit, and the oversize plan. PASS, with one measured gap this feature closes.

- `ReadGroup` lives in `source/counters/detail/core.hpp` (line 60) and
  the plan holds one per provider (`plan.cpp`, lines 500 to 560): the
  group carries every leaf that provider owns, one disclosure column
  follows each group's own leaves, and `provider->open` runs while the
  plan compiles (`plan.cpp`, around line 585). A provider that declines
  a window answers with the error "provider cannot open a window for its
  leaves (FR-011)".
- `source/counters/linux_pmu/group_io.cpp` sets
  `PERF_FORMAT_GROUP | PERF_FORMAT_TOTAL_TIME_ENABLED |
  PERF_FORMAT_TOTAL_TIME_RUNNING` on every member (line 222), opens
  members against the device's group leader (lines 596 to 615), and a
  `perf_event_open` refusal returns `nullptr` (line 603). The fold
  computes the enabled/running ratio from that pair.
- How the plan learns the counter limit today: it does not. No file in
  `source/` or `include/speedgun-ng/` reads or publishes a programmable
  counter count. An oversize group is not a refusal: the kernel grants
  the open and rotates the group across its counters afterwards.
  `docs/pages/counters-overhead.md` records the measurement on this host
  (64 countable core-PMU events opened at once returned ratios 0.428174
  and 0.543263 of the enabled time), and `test/source/counters_pmu_test.cpp`
  carries the multiplex scenario (lines 824 to 946) that checks the
  disclosed ratio.
- How a leaf that does not fit is reported today: nothing reports a leaf
  that does not fit. The plan opens whole, every leaf counts, and the
  shared shortfall appears as the ratio disclosure of every fold of that
  group. That behavior is the `multiplex` method of D-7, and N6 keeps it
  reachable.
- `test/source/counters_fake_test.cpp` drives the published
  `sg::counters::FakeProvider` with hand-computed counts, including the
  composite multiplex ratio product. The fake provider carries no limit
  parameter yet, which is the parameter D-3 adds.

### PC-4: the harness metric path and the spine. PASS.

- `include/speedgun-ng/benchmark.hpp` holds `MetricSeed` with `core`,
  `label` and `perIteration` (line 1431), the templated
  `BenchmarkHandle::addMetric` (line 1703), `addMetricCore`
  (line 2101), `MetricValue` with `value`, `runningRatio`, `scaled` and
  `availability` (line 211), and `ResultRow::metrics` (line 374).
- `source/harness/runner.cpp` builds the spine every run compiles:
  `machine/monotonic` as the clock leaf (line 1119) and
  `machine/thread_cpu` or `process_cpu` per the family switch
  (lines 1128 and 1135), then the metric columns, a named metric
  resolving through `sg::counters::metricByName` and
  `sg::counters::catalogMetrics` (lines 1220 to 1237), then
  `compileCore(sys, Target{}, cores)` with the real-time and CPU-time
  cores pushed first (lines 1238 to 1253). In a threaded run each
  context compiles the same spine on its own thread (lines 1721 to
  1732).
- `source/harness/cli.cpp` carries the `--metric` option (line 301,
  short `M`), collects names (line 339), and refuses a name the host
  carries nothing for through `metricNameFailure` with a one-line
  message and exit status 2 (lines 211 to 246). The percentiles pair
  `--percentiles`/`--no-percentiles` (lines 303 to 305, handling around
  line 340) records the precedent that the command line wins over the
  family setting.
- `source/harness/report.cpp` prints each metric column as
  `label= value= ratio= scaled= gap=` with a `refusal=` token for a
  refused metric (lines 489 to 505) inside `printResult` (line 373).
  `printContextDisclosure` (line 173), `printNoiseWarning` (line 227)
  and `printPercentileLine` (line 299) are the printer functions the
  request's N4 boundary names: N6 adds a printer beside them and edits
  none of them.

### PC-5: the existing coverage exclusions in `provider.cpp`. PASS, and this feature adds none.

- `source/counters/linux_pmu/provider.cpp` carries the recorded
  exclusions at lines 92, 124, 142, 161, 191 to 201, 232 to 304, 312 to
  325, 345, 364 to 404, 504 to 538, 612 to 703, 742 to 838 and 902.
  Every marker names its task (T056, T066, T140) and its ground: a
  kernel-grant arm a refused-open runner cannot reach, a kernel-refusal
  arm a granted host cannot reach, or a gcov block gcov attributes to a
  brace or an epilogue. The paired seam-test fixtures are named in the
  comments (for example `fastDecode` and `open_fast_window`).
- Those markers stand for N6 for the same reason they were recorded: the
  two-sided kernel conditions they mark stay unobservable in-process.
  The pass planner needs no kernel-grant arm of its own, because D-3
  makes the limit a value the fake provider can carry, so N6 adds no new
  marker anywhere.

## Roadmap position

The roadmap file lives outside this repository in the owner's notes and
is absent from this machine, so the request of 2026-10-20 is the
authority for the numbering below.

- This feature is N6 on the counters lane. N2 (metric formulas,
  specs/022) merged before it; N10 (the top-down metric tree) follows it
  on the same lane and consumes its pass machinery.
- N3 (JSON reports) and N4 (topology placement) run on two other
  machines at the same time. The shared-file list of Constraints
  settles the overlap with each.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Measure more metrics than the hardware can count at once (Priority: P1)

A developer asks one benchmark for a metric set larger than the
processor's programmable counters. On today's master every value arrives
scaled down: the kernel rotates the oversize group, and every metric of
the run discloses a running ratio below 1 (0.428 and 0.543 were recorded
on the development host). After this feature the same command splits
the metrics into passes, runs the benchmark once per pass, and reports
each metric with the full-fidelity value of the pass that measured it.

**Why this priority**: this is the feature. Every other story controls
it, reports it or teaches it.

**Independent Test**: on the fake provider with a set counter limit,
attach a metric set whose event total exceeds the limit, run the
harness, and check that every metric column carries a value from a pass
that ran at ratio 1.0, that the time columns come from the first pass,
and that no metric was dropped. The story is complete with no real PMU,
no command-line option and no documentation.

**Acceptance Scenarios**:

1. **Given** a fake host whose limit is 4 and four metrics of two events
   each, **When** the run attaches them, **Then** the planner assigns
   two metrics to pass 0 and two to pass 1 in request order, the run
   executes both passes with the same iteration count, and the merged
   row carries four metric values, each from its own pass.
2. **Given** the same fake host and one metric of 5 events, **When** the
   run attaches it, **Then** that metric takes a pass of its own, that
   pass reports a running ratio below 1 from the fake's rotation, and
   the other metrics keep ratio 1.0.
3. **Given** a metric list that fits the limit, **When** the run uses
   the default method, **Then** exactly one pass runs and the console
   report is the bytes master prints for the same run.
4. **Given** the fake host with limit 4 and the metric list of scenario
   1, **When** the planner runs twice on the same list, **Then** the two
   pass assignments are identical.

---

### User Story 2 - Choose the collection method (Priority: P2)

A developer compares pass collection against multiplexing, or pins one
method for a scripted comparison. The benchmark handle carries the
method, and the command line carries the same three values; the command
line wins. A bad value stops the process before any measurement runs.

**Why this priority**: the comparison between the two methods is the
evidence the feature works, and the option is how the owner reaches it
on any benchmark without editing code.

**Independent Test**: run one registered benchmark three ways
(`auto`, `passes`, `multiplex`), check the disclosed method token and
pass count of each report, and check that a bad value prints one line
and exits with status 2 without running any benchmark work.

**Acceptance Scenarios**:

1. **Given** a metric set that needs two passes, **When** the run uses
   `--metric-method=multiplex`, **Then** one pass runs, the kernel
   rotation applies, and every metric column discloses its ratio as
   today.
2. **Given** a metric set that fits one pass, **When** the run uses
   `--metric-method=passes`, **Then** one pass runs and the report
   discloses the passes method with a pass count of 1.
3. **Given** any benchmark, **When** the run carries
   `--metric-method=fast`, **Then** the harness prints one line naming
   the bad value and exits with status 2 before measurement starts.
4. **Given** a family whose handle forces `multiplex`, **When** the
   command line asks for `passes`, **Then** the run collects by passes.

---

### User Story 3 - Read the method, the pass count and the spread in the report (Priority: P2)

A reader of the console report needs to know how a row was measured.
The report states the method, the number of passes and the relative
spread of the time per iteration between passes, and it warns when the
spread crosses 5 percent. When more than one pass ran, each metric
column carries the pass that measured it.

**Why this priority**: an undisclosed multi-pass row is a measurement a
reviewer cannot trust, and Principle VII requires the disclosure of
anything a row's number depends on.

**Independent Test**: on the fake provider, drive two fixtures, one with
equal per-pass times and one whose pass times differ by more than 5
percent, and check the disclosure lines and the warning line of each.

**Acceptance Scenarios**:

1. **Given** a two-pass run, **When** the report prints, **Then** one
   line names the passes method, the pass count of 2 and the spread,
   and each metric column carries a pass tag of 0 or 1.
2. **Given** pass times per iteration of 100 ns, 108 ns and 112 ns
   through a median of 108 ns, **When** the report prints, **Then** the
   spread reads 11.1 percent and the warning line follows.
3. **Given** a one-pass run, **When** the report prints under the
   default method, **Then** no pass line appears at all (D-9).
4. **Given** a `multiplex` run, **When** the report prints, **Then** the
   method line names `multiplex`, the per-metric ratio fields keep their
   current spelling, and no spread appears, because one plan ran.

---

### User Story 4 - Read the page and copy a working block (Priority: P3)

A reader of the main page learns that a metric set wider than the
hardware exists as a supported case. The new section shows the option in
a runnable block, the quick reference carries its rows, and the detail
lives in the harness page.

**Why this priority**: constitution IV.1 makes the page part of the
feature, and the drift and run gates keep the claim true.

**Independent Test**: check out the tree, copy the block over the
canonical includes, build it, run it, and read the printed row; the run
gate does the same in CI.

**Acceptance Scenarios**:

1. **Given** the new section, **When** the drift gate compares it with
   its example region, **Then** the two match and the run gate runs the
   example's own binary.
2. **Given** the quick-reference and option tables, **When** the drift
   gate checks them, **Then** every new row resolves and the option
   table carries `--metric-method`.
3. **Given** the main page after the section lands, **When** the gate
   checks the page budget, **Then** the recorded `PAGE_LINE_CAP` covers
   the measured page.

### Edge Cases

- A metric list with no metrics at all (a plain run): no planner work,
  no pass machinery, no disclosure line. The run is the plain run of
  master.
- One metric whose events fit: one pass, and the default method prints
  exactly what master prints today.
- The same metric attached in code and named on the command line: the
  existing one-column rule (PC-4) keeps one column, and that column
  belongs to one pass.
- An unavailable metric (refused at load or uncountable on the host):
  the existing refusal path keeps it out of every pass, and the column
  prints its refusal as today.
- A metric set that needs one pass per metric on a limit of 1 (the fake
  provider can set it): every pass holds one metric, the pass count
  equals the metric count, and the spread disclosure works unchanged.
- A pass whose time is zero per iteration (a dry-run style fixture): the
  spread formula divides by the median; a median of zero publishes the
  spread as `not available` with no warning and no failure.
- A threaded run of two contexts and two passes: both contexts run pass
  0 then pass 1 in that order, no pass window of either context is open
  while a later pass window of any context is open, and the merged row
  is one row set as in a single-context run.
- A pause-and-resume benchmark collected by passes: the pause netting
  rule of the primary time applies inside each pass, and the merged row
  reads its time from the reference pass.
- Percentile columns under two passes: the chunk times of the reference
  pass feed them; metric percentiles come from each metric's own pass.
- A host whose event source publishes no limit: `auto` and a forced
  `passes` both keep the single kernel-multiplexed plan and the
  disclosure names `multiplex` (D-11).
- A forced method on a run that carries no metric columns: no
  disclosure line prints and the report stays the plain run of master.
- A run that carries user-counter columns beside metrics: every PMU
  column takes part in the split, the metrics place first in request
  order, and each column carries its values from the pass that measured
  it.
- `--metric-method` given twice with different values: the last value
  wins, matching the scanner's behavior for repeated value options.
- A bad value on the handle path: the selector takes the closed
  enumeration, so a value outside the three is unrepresentable there and
  no error path exists for it; the exit-2 rule belongs to the command
  line alone. `contracts/metric-method-selector.md` states the
  unreachability.
- A metric set spanning two providers (the CPU tree plus another event
  source with no published limit): the split applies the limit of the
  sources that publish one, and a source without a limit rides every
  pass it needs while its folds keep the ratio disclosure of D-2.
- A pass loop left early on a stop, the interrupt flag or a body skip,
  once the method has settled: the result publishes the method it
  settled to and, for a stop in pass k, a `passCount` of k + 1
  counting the passes whose folds stand. Pass k stands in that
  count once it has folded a row; a pass that folded none stays out of
  it, and a stop before pass 0 opens publishes the `passCount` of 0
  that says the mechanism did not run. No `passSpread` appears: the
  spread is the figure of a complete set of passes. No value carries a
  pass tag: the tag names the measuring pass of a merged row, and a
  stopped collection merged nothing (FR-017, FR-019).

## Requirements *(mandatory)*

### Pass planner (C-1)

- **FR-001**: When a run attaches metrics and the selected method allows
  splitting, the harness shall partition the events of the run's PMU
  columns (metric columns and user-counter columns) into one or more
  passes so that the PMU events of one pass number at most the
  programmable-counter limit of the event source they open against. (Q5,
  2026-10-20.)
- **FR-002**: The partition shall be a first fit in the order the caller
  requested the metrics: each metric joins the first pass with room for
  all of its events, and a metric with no such pass opens a pass of its
  own. The same metric list, method and limit shall produce the same
  passes on every run.
- **FR-003**: The events of one metric shall always share one pass. No
  requirement, option or error path shall fold one metric's events
  across passes.
- **FR-004**: When one metric carries more events than the limit, the
  planner shall give it a pass of its own, that pass shall run
  oversubscribed under the kernel's rotation, and the metric's column
  shall disclose its running ratio with the existing fields.
- **FR-005**: The counters library shall publish, beside the catalog
  facts it already publishes, the number of programmable counters one
  context can hold at open time on an event source it can answer for,
  for each vendor and per device, as the research D-01 table gives. An
  event source it cannot answer for shall publish no limit, and the
  planner shall treat a run that needs only such sources as the `auto`
  fallback of D-11.
- **FR-006**: The fake provider shall take the counter limit as a test
  parameter, and the split, merge and disclosure logic shall read the
  limit only through the provider-facing facts, so every branch of the
  planner runs on the fake provider without a PMU.
- **FR-007**: The pass plan shall carry the run's fixed time leaves on
  every pass (PC-4): the two machine clock leaves ride every spine and
  count against no pass, because they consume no programmable counter.
  A PMU cycles event a metric names counts in the pass that carries it.
- **FR-008**: The planner shall run once per run, before the first
  timed window of the first pass, and it shall read no counter.

### Pass runs (C-2)

- **FR-009**: The harness shall execute the benchmark once per pass, and
  each pass shall run every repetition the family and run control
  specify.
- **FR-010**: The calibration shall run once, before the first pass.
  Every pass shall use the iteration count that calibration produced, so
  the passes measure the same amount of work.
- **FR-011**: Every pass shall run on the same execution contexts and
  placement as the other passes of the run. The run chooses contexts
  once; a pass shall not re-place a context.
- **FR-012**: Each pass shall open its plan before its first timed
  window and close it after its last timed window. No timed window shall
  contain planner work, merge work, report output or an allocation of
  pass state (D-10).
- **FR-013**: In a threaded run, every context shall execute the same
  pass order, and no pass window shall stay open in any context when a
  later pass window opens anywhere in the run.

### Merge (C-3)

- **FR-014**: The harness shall merge the passes into one result: each
  metric column shall carry the values of the pass that measured it, in
  the column order the run established before the split. A user-counter
  column shall carry its values the same way.
- **FR-015**: The time columns, the overhead floor, the endpoint
  samples, the CPU-time column and the primary time's percentile columns
  of the merged result shall come from the reference pass, which is
  pass 0. A metric's percentile columns shall come from the pass that
  measured that metric.
- **FR-016**: The merge shall run after the last pass closes, outside
  every timed window, and it shall publish no value the passes did not
  measure.

### Disclosure (C-4)

- **FR-017**: When more than one pass ran, the console report shall
  print one disclosure line that names the method (`passes`), the pass
  count, and the relative spread of the time per iteration across
  passes. The spread is (max minus min) divided by the median, expressed
  as a percentage, computed over the merged reference time per iteration
  of each pass (Q3, 2026-10-20). The median of an even count is the
  lower of the two central values; no value is averaged. The merged
  result shall also carry the
  settled method, the pass count and the spread as published disclosure
  fields, which the JSON document carries through FR-038 (Q4,
  2026-10-20).
- **FR-018**: When the spread exceeds 5 percent, the report shall print
  one warning line beside it. The warning discloses; it shall not
  change the run's outcome or exit status.
- **FR-019**: When more than one pass ran, each metric column shall
  carry the index of the pass that measured it in the report's existing
  metric-field spelling.
- **FR-020**: When the method is `multiplex`, the report shall name the
  method on the same disclosure line, the existing per-metric `ratio=`,
  `scaled=` and `gap=` fields shall keep their current spelling, and no
  spread shall appear.
- **FR-021**: A run that executed one pass under the default method
  shall print the console report master prints for the same run today,
  field for field (D-9), except that a host publishing no limit shows
  the FR-024 `multiplex` disclosure. The disclosure line appears when
  the caller forced a method even with one pass, and it appears only
  when the run carries at least one metric column (Q2, 2026-10-20).
- **FR-022**: The pass disclosure shall print from a new printer
  function. `printContextDisclosure` shall stay byte-identical, because
  the placement feature edits it on its own lane.

### Options (C-5)

- **FR-023**: The harness shall offer the metric collection method as
  three values, `auto`, `passes` and `multiplex`, through a benchmark
  handle selector and through the command line option
  `--metric-method=VALUE`. The default shall be `auto`.
- **FR-024**: `auto` shall collect in one pass when every PMU event of
  the run's PMU columns fits the limit and the limit is published, and
  by passes when they do not. On a host with no published limit `auto`
  and a forced `passes` shall both keep the single kernel-multiplexed
  plan and the disclosure shall name `multiplex` (D-11, Q1, 2026-10-20).
  `passes` shall force the planner path wherever a limit is published.
  `multiplex` shall keep the single oversize plan that master performs
  today.
- **FR-025**: When the command line and the handle disagree, the command
  line shall win, on the precedent of `--percentiles`. A `--list` or
  `--catalog` run ends before any method settles: the word carried by
  `--metric-method` is still checked per FR-026, and no method settles
  and no disclosure line prints on such a run.
- **FR-026**: When the command line carries a value outside the three,
  the harness shall print one line naming the value and exit with status
  2 before any measurement starts, matching the existing scanner
  behavior.

### Documentation (C-6)

- **FR-027**: The main page shall carry a section "Multi-pass
  collection" of at most `SECTION_LINE_CAP` lines, with one purpose
  sentence, one marked snippet and one result sentence. The snippet
  shall come from a new owner file under `example/docs/` that CI builds
  through its `docs_` target and runs through its own `run_docs_` target
  and the run gate.
- **FR-028**: The quick-reference table shall carry a row for each
  public name a benchmark author looks up: the `metricMethod` handle
  selector and the `Object::programmableCounters` limit fact, and the
  option table shall carry the `--metric-method` row, so the drift
  gate's option check passes. The `MetricMethod` enumerators and the
  published result fields are covered by those rows and by the section
  prose.
- **FR-029**: The detail a reader needs beyond the page shall live in
  `docs/pages/harness.md`: the split rule, the merge rule, the spread
  formula and the fallback of D-11.
- **FR-030**: The page's added lines shall move `PAGE_LINE_CAP` through
  a recorded raise that states the measured figure, on the precedent the
  file's own comments record, and the drift gate shall stay green.

### Version, tests, and discipline

- **FR-031**: This feature shall ship as the next free minor version at
  merge time: 0.13.0 when nothing merges first. It shall raise the
  counters archive `SOVERSION` from its master value by one when it
  changes the counters exported layout (the limit fact and the fake
  provider parameter are such changes), and the harness `SOVERSION` by
  one when it changes the harness exported layout (the handle selector
  and any exported disclosure field are such changes). Each raise names
  the layout that moved, on the 0.x rule stated in `CMakeLists.txt` and
  the precedent of the harness raise to 7.
- **FR-032**: New code shall hold 100% line and 100% branch coverage
  with no new exclusion marker, no waiver, no partial row and no open
  row. A remaining gap shall stop the work and be reported with `gcov
  -b` evidence. The markers recorded in PC-5 stand unchanged, because
  the kernel-grant and kernel-refusal arms they mark stay unobservable
  in-process.
- **FR-033**: Every split, merge, disclosure and warning branch shall
  run in a unit test on the fake provider with a set limit. No test
  shall sleep, read a clock, or require a real PMU; synchronization shall
  use the fake provider and the existing barriers.
- **FR-034**: A threaded test shall show two contexts and two passes
  keeping the pass order of FR-013.
- **FR-035**: A script on corsairone shall collect one metric set that
  needs passes, print the passes report and the multiplex report of the
  same set, and be recorded with the run that shows it. It is not a CI
  gate, and no CI job shall depend on its host.
- **FR-036**: Every time and counter value of the feature shall come
  from `sg::counters`, so the time-source gate stays green. The feature
  shall add no library and no vendored tree.
- **FR-037**: The coverage preset shall keep `-fprofile-update=atomic`
  and stay without `-D_GLIBCXX_ASSERTIONS`; the suite shall keep passing
  under `ctest -j 2`.
- **FR-038**: The JSON document shall carry the pass disclosure. Each
  benchmark entry gains `metricMethod`, `passCount`, `passSpread` and
  `spreadWarning` after the fields it already carried, and each metric
  column of a row gains `pass`, the index of the pass that measured the
  value, `null` where one pass measured the run. The schema file shall
  state the five fields, `loadReport` shall read them and refuse a
  document whose `spreadWarning` disagrees with the spread beside it, and
  a round-trip test shall cover one pass, two passes, a spread above the
  warning limit and `multiplex`, with an error path for each field. A
  one-pass run keeps the bytes the format wrote before, with the new keys
  added. A document written before the five fields shall load as it
  stands: `loadReport` shall read a key that stands missing as the
  default its member holds, so the comparison of specs/026 (FR-051)
  reaches a baseline stored before this feature.

## Clarifications

### Session 2026-10-20

- Q: On a host whose event source publishes no counter limit, what must
  a run forced to `passes` do? → A: It keeps the single
  kernel-multiplexed plan and the disclosure names `multiplex`, the
  D-11 fallback extended to a forced method.
- Q: Does a forced method print its disclosure line on a run that
  carries no metric columns? → A: No. The pass machinery discloses only
  when the run carries at least one metric column.
- Q: Which numbers feed the pass spread when a threaded run merges
  several contexts? → A: The merged reference time per iteration of
  each pass, the same numbers the report shows.
- Q: Must the settled method, pass count and spread be readable from
  the result object and not only the console? → A: Yes. They are
  published disclosure fields of the merged result, and FR-038 writes
  them into the JSON document.
- Q: Do user-counter columns from `--counter` take part in the pass
  split? → A: Yes. Every PMU column of the run participates, and the
  metrics place first in request order.

## Key Entities

- **Pass**: one collection round. It carries a set of metric columns and
  the plan that opens their events, runs the benchmark's repetitions,
  and closes with its own recorder readings.
- **Pass plan**: the plan a pass compiles: the two fixed time leaves plus
  the metric columns assigned to that pass, on the existing spine path.
- **Metric collection method**: `auto`, `passes` or `multiplex`. One run
  settles to exactly one concrete method before its first window.
- **Programmable counter limit**: the number of PMU events one context
  can hold open on one event source at once. A published fact per source
  (D-3), a parameter on the fake provider (FR-006), and absent where the
  source cannot answer.
- **Reference pass**: pass 0, the pass whose time columns and primary
  percentiles the merged result carries.
- **Pass spread**: (max minus min) divided by the median of the time per
  iteration across passes, as a percentage. Absent for a one-pass run
  and for a zero median.
- **Pass disclosure**: the report line naming the method, the pass count
  and the spread, plus the warning line above 5 percent. The same three
  values stand as published fields of the merged result.
- **Pass tag**: the pass index a metric column prints when more than one
  pass ran.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-01**: On the fake provider with limit 4 and the four-metric set of
  US1 scenario 1, all four metric columns report a value with running
  ratio 1.0, while the same set under `multiplex` reports ratios below
  1. On corsairone the real-PMU record of FR-035 shows the same shape,
  where master reported ratios 0.428174 and 0.543263 for the recorded
  oversize open.
- **SC-02**: A fixture run of the same metric list and limit twice
  produces identical pass assignments (D-1), and the pass count equals
  the first-fit minimum on the fixture limits 1, 4 and 8.
- **SC-03**: A fitted one-pass run under the default method (the limit
  published and the events fitting) prints a report byte-identical to
  the fixture captured from master for the same run.
- **SC-04**: Every command-line failure this feature adds prints one
  line and exits with status 2 in a scripted check, and no benchmark
  output precedes the line.
- **SC-05**: The fixture whose pass times per iteration are 100 ns,
  108 ns and 112 ns prints a spread of 11.1 percent and the warning
  line; the fixture inside 5 percent prints the spread with no warning;
  a one-pass default-method run prints no disclosure line.
- **SC-06**: The merged report carries one line naming method, pass
  count and spread for every multi-pass run, every metric column of
  such a run carries its pass tag, and the merged result object carries
  the same three values as published fields.
- **SC-07**: The new code holds 100% line and 100% branch coverage with
  no new exclusion marker, and the whole gate set of Principle VIII is
  green on the pushed state, with the task-list and coverage counts read
  from the CI run of the last pushed commit.
- **SC-08**: The docs example builds and runs in CI, the drift gate is
  green with the recorded `PAGE_LINE_CAP` raise, and the quick-reference
  rows resolve.
- **SC-09**: The timed path gains nothing: the per-iteration provider
  read count of the existing no-allocation and overhead checks stays at
  the value measured before this feature on a one-pass run.
- **SC-10**: A threaded fixture of two contexts and two passes shows
  both contexts completing pass 0 before either opens pass 1, with no
  sleep and no clock in the test.

## Constraints

- Constitution I, II, VI, VII, VIII, X and XI bind in full. 100% line,
  branch and DBC coverage, no exclusion marker, no partial or open row.
- The counters-only rule of Additional Constraints binds the harness
  side: the split, the merge and the disclosure read time and counts
  only through plans, recorders and folds of `sg::counters`. The limit
  fact lives inside the counters library.
- Zero external runtime dependencies stays true.
- `ctest -j 2`, and the coverage preset keeps `-fprofile-update=atomic`
  and stays without `-D_GLIBCXX_ASSERTIONS`.
- Rebase onto `master`, never merge, and push with `--force-with-lease`.
  After each push, show `git ls-remote origin <branch>`, and read the CI
  logs of the pushed commit and fix its failures in this lane.
- The shared files of the three-lane contract: the project `VERSION` and
  the two `SOVERSION` lines in `CMakeLists.txt`, `README.md`, the
  quick-reference and option rows, `PAGE_LINE_CAP` in
  `tools/docs/example_drift.py`, `example/CMakeLists.txt` and the
  example run list, and `test/CMakeLists.txt`. On a rebase: keep both
  features' lines, take the next free minor, raise `PAGE_LINE_CAP` again
  by this feature's own section, run the full gate set, and push once
  with `--force-with-lease`.
- The JSON schema and its round-trip test belong to N3: if a feature
  adds a field to `ResultRow`, `MetricValue` or `BenchmarkResult`, the
  feature that merges second adds it to the JSON output and the
  round-trip test. This feature merges second over N3, so FR-038 carries
  that work here. The schema gains fields, so `schemaVersion` moves from
  `1.0` to `1.1` under specs/023 D-4 and FR-010, the rule that a minor
  adds fields and a major is what breaks a loader. A `1.0` document
  stands outside the new keys and loads through the version rule, and
  the schema file keeps its name, `docs/schema/report-v1.json`, because
  the major stands at 1.
- Commit messages follow the constitution template: blank lines between
  body paragraphs, one footer block with `Approved-by:` then `Refs:`, an
  `Approved-by:` line only for a decision the owner approved, and
  `Refs: specs/024-multipass-collection`.

## Scope

In: FR-001 to FR-038. The planner and its first-fit split, the
per-source limit fact inside counters, the fake provider's limit
parameter, the per-pass runs with one calibration, the merge, the
console disclosure with its warning, the method option on handle and
command line, the threaded pass order, the pass fields of the JSON
document with their schema and round-trip test, the docs section with its
runnable example, and the version and `SOVERSION` effects.

Out, each with its owner:

- The top-down metric tree and its levels: N10.
- Topology placement and any edit of `ContextDisclosure` or
  `printContextDisclosure`: N4.
- New metric formulas, new vendored tables and new metric names: none.
- Modeling cross-process counter contention (another perf session on the
  same CPU): the kernel still rotates, and the ratio field discloses it,
  as today.
- Splitting one metric's events across passes: D-1 forbids it.
- Rejecting a run because its spread is large: D-6 discloses.
- A per-source limit on an event source the library cannot answer for:
  D-11 defines the fallback, and any future answer is a counters change
  of its own.

## Assumptions

- The request of 2026-10-20 is the owner's pasted text and is the
  authority for scope, numbering and the accepted recommendations. The
  roadmap file it names sits outside this repository and is absent from
  this machine.
- corsairone is the development and real-PMU host, as the request
  states. This session ran on it and read family 6, model 85, stepping 4
  from `/proc/cpuinfo`; it has one socket.
- The kernel keeps granting oversize groups and rotating them, which is
  the behavior the tree already measures and documents (PC-3). The
  `multiplex` method relies on it.
- The limit the planner needs is the programmable general-purpose count
  per core, for each vendor, as the research D-01 table gives; where
  the provider reads it is a plan decision, and this spec states only
  that the fact is published, fakeable and per source.
- Pass 0 is the reference pass for every run; nothing in the request
  selects a different reference.
- Metric event totals come from the loaded metric spines, which the
  planner reads without opening anything (FR-008).
- Percentile columns already exist (specs/021); "the percentiles come
  from pass 0" means the reference pass supplies the chunk times of the
  primary column, and each metric column keeps its own pass's chunks.

## Budget note

Measured at the audit point, before any section is written:

- Page: `README.md` is 666 lines and `PAGE_LINE_CAP` is 666
  (`tools/docs/example_drift.py`, line 124), so the page sits at its cap
  and every added line needs a recorded raise. The new section's lines
  and its quick-reference and option rows move the cap by their measured
  total, recorded in the raise's comment as the precedents of that file
  record.
- Section: `SECTION_LINE_CAP` is 25 (line 125) with two recorded
  exceptions (`Registration` 38, `Fixtures` 43, lines 130 to 133). The
  new section holds 25 lines per FR-027 and needs no exception.
- Sections and owners: the `SECTIONS` list gains "Multi-pass collection"
  and `SECTION_OWNERS` gains its new file under `example/docs/`. The
  list currently ends at "Percentiles".
- Example: one new file under `example/docs/` with its `docs_` target,
  `run_docs_` runner (`example/CMakeLists.txt`, lines 38 to 47) and
  run-gate entry. The golden-`.expected` pattern of
  `percentiles.expected` is available if the plan wants a printed
  disclosure shape.
- Quick reference: three rows, one for the handle selector, one for the
  counters limit fact, and the `--metric-method` option row.
- User guide: `docs/pages/harness.md` (978 lines) holds the detail per
  FR-029; the drift gate's page list already covers it.

## Dependencies

- specs/007-counters-and-timers: providers, plans, recorders, folds and
  the fake provider the planner and the tests drive.
- specs/015-benchmark-harness-core: the run spine, the status-2 usage
  rule, the counters-only rule with its gate.
- specs/019-harness-user-counters-and-statistics: the metric column path
  the passes merge back into.
- specs/020-execution-contexts-core: the per-context plan compile and
  open the pass runs reuse (FR-011, FR-013).
- specs/021-percentiles-raw-capture: the percentile columns the merge
  sources per FR-015, and the command-line-wins precedent of FR-025.
- specs/022-metric-formulas: `metricByName`, `MetricView` and the loaded
  spines whose events the planner counts.
- specs/018-docs-main-page: the drift and run gates FR-027 to FR-030
  clear.

## Citations

- `include/speedgun-ng/counters_metrics.hpp`: `MetricRefusal` (33),
  `MetricView` (63), `metricByName` (81), `MetricListing` (91),
  `catalogMetrics` (113).
- `source/counters/linux_pmu/metric_formula.cpp` (1653),
  `metric_formula.hpp`, `metric_tables.hpp`.
- `include/speedgun-ng/counters_core.hpp`: `Availability` (87),
  `MetricResult` with the `runningRatio` multiplex document.
- `include/speedgun-ng/counters_measurement.hpp`: `foldPairs`,
  `metricCore`, the ratio-bearing fold view.
- `source/counters/fold.cpp` (310, 323): `runningRatio` from the group
  disclosure.
- `source/counters/detail/core.hpp`: `ReadGroup` (60), the group vector
  (133).
- `source/counters/plan.cpp`: one read group per provider (500 to 560),
  the per-group disclosure column, the provider open with the FR-011
  refusal (585 to 610).
- `source/counters/linux_pmu/group_io.cpp`: the read format with
  `PERF_FORMAT_GROUP` and the enabled/running pair (222), the member
  open against the leader with the `fd < 0` refusal (596 to 615),
  `openGroupWindow` (548) and `pmuOpenWindow` (779).
- `source/counters/linux_pmu/provider.cpp`: the recorded coverage
  exclusions listed in PC-5.
- `docs/pages/counters-overhead.md`: the recorded oversize open on this
  host (ratios 0.428174 and 0.543263).
- `test/source/counters_pmu_test.cpp`: the multiplex scenario (824 to
  946). `test/source/counters_fake_test.cpp`: the fake provider's
  hand-computed spine tests.
- `include/speedgun-ng/benchmark.hpp`: `MetricValue` (211),
  `ResultRow::metrics` (374), `MetricSeed` (1431), `addMetric` (1703),
  `addMetricCore` (2101).
- `source/harness/runner.cpp`: the monotonic and CPU leaves
  (1119 to 1135), the named-metric columns (1220 to 1237), the spine
  compile (1238 to 1253), the per-context compile (1721 to 1732).
- `source/harness/cli.cpp`: `metricNameFailure` (211 to 246), the option
  table (293 to 306), the percentiles handling precedent (340 to 352).
- `source/harness/report.cpp`: `printContextDisclosure` (173),
  `printResult` (373), the metric fields with `ratio=` (489 to 505).
- `tools/docs/example_drift.py`: `PAGE_LINE_CAP` (124),
  `SECTION_LINE_CAP` (125), `SECTION_LINE_CAPS` (130), `SECTIONS`,
  `SECTION_OWNERS`.
- `example/CMakeLists.txt`: the `docs_` and `run_docs_` target pattern
  (38 to 47). `README.md`: 666 lines; the option table and quick
  reference at the sections the gate checks.
- `CMakeLists.txt`: `VERSION 0.12.0` (7), the 0.x rule with archive
  `SOVERSION 2` (41 to 47), the harness `SOVERSION 7` with the 020 and
  021 raise notes (214 to 225).
- `.specify/memory/constitution.md` II, VI, VII, VIII, X, XI, Additional
  Constraints (counters-only, dependencies), Refactoring and Evolution
  (version and `SOVERSION`).

## Open questions

None open. The request's open points are settled by D-1 to D-12, and
PC-3 records the one gap in master (no published counter limit) that
D-3 closes.

## Constitution Checklist

- Main-page impact check (IV.1): YES. The feature changes user-visible
  behavior, so FR-027 to FR-030 carry its main-page section, its
  runnable `example/docs/` owner file, its quick-reference and option
  rows, and its `docs/pages/harness.md` detail in the same change,
  inside the budget this spec's Budget note records.
