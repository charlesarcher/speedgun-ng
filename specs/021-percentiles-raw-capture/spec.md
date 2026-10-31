# Feature Specification: Percentiles and Raw Capture

**Feature Branch**: `021-percentiles-raw-capture`

**Created**: 2026-10-19

**Status**: Draft

**Input**: User description, opening quoted, the full text held by the
specify invocation record:

"Percentiles and raw capture, item N1 (old H6) of the roadmap
`/workspace/speedgun-ng-harness-roadmap.md`. Short name:
percentiles-raw-capture. Target machine: gary. Lane: harness (N1, then
N3, N5, N7). It runs in parallel with N2 on corsairone and N4 on thrawn.
The harness keeps the raw counter points of each benchmark run and
reports percentiles. Today the report gives mean, median and standard
deviation over repetitions only. After N1, a user can see the tail:
p50, p90, p99 and p99.9 of the time per iteration, and of each user
counter. The raw points are the input of N3 JSON reports. [...]"

## Clarifications

### Session 2026-10-19

- Q: Where do the percentile values print in the console report? → A:
  on each run row, one row per requested percentile per reported
  column; the `AGGREGATE` row keeps today's statistics and gains none
  (FR-010).
- Q: Does the chunked capture replace the single sampling window a run
  takes today, or stand beside it? → A: it replaces it, so a run keeps
  one cadence and the value of the whole run folds over the same point
  column the percentiles read (FR-001, FR-003).
- Q: How many raw points does one chunk boundary leave per column, and
  what bounds a column? → A: one point shared by the two chunks it
  separates, so N chunks leave N+1 points, and one constant of the kind
  of `kMaxPauseSamplesPerLeaf` bounds the column while calibration
  raises the chunk size to stay inside it (FR-002, FR-006).
- Q: When does a percentile report as not available? → A: when that
  column holds no chunk value, or when the chunk count falls below the
  minimum the percentile names: a percentile p needs at least
  `ceil(100 / (100 - p))` chunk values, so p50 needs 2, p90 needs 10,
  p99 needs 100 and p99.9 needs 1000. No value is extrapolated
  (FR-013).
- Q: Does the observer cost get a new report field? → A: no, the row's
  existing `overhead floor (ns)=` field carries it and now holds the
  median cost of one chunk window, so no new field enters the row
  (FR-007, FR-017).

## Run-time binding

This spec binds no spec number, commit, or source line beyond the base
it records here.

- **Spec number**: 021, the next free number under `specs/` at branch
  creation. Two other features start the same day on other machines and
  take their own next free number, so a collision is resolved at rebase
  by the later branch renumbering and recording the change here.
- **Branch base**: `ab32400` (2026-10-19), the head of `master` at
  branch creation: version 0.10.0, harness `SOVERSION` 6, `speedgun-ng`
  archive `SOVERSION` 2.
- **Names**: every symbol named below is an anchor read at the audit
  point. A plan re-reads each one on the base it runs against and
  records any drift.

## Decisions

The owner accepted these nine recommendations on 2026-10-19 as part of the
prompt this spec comes from. They are governance for this feature. Each
is recorded again at the FR that carries it.

- **D-1 Chunk cadence**: one sample point per chunk, with the chunk
  size from calibration so the cost of one sample point stays at most
  1% of the chunk's own time. The report discloses the chunk size
  (FR-005, FR-007).
- **D-2 Storage**: one reservation per context, minted before the timed
  window at the chunk plan's point capacity, which the bound of
  `kMaxChunkPointsPerLeaf` holds at or below 32'768 points per column;
  calibration raises the chunk size so the bound holds and no raw point
  is dropped to fit; no allocation happens inside the timed window
  (FR-006, FR-008). This replaces the fixed-size-blocks wording of the
  prompt: a reservation under a bound needs no growth step, and a growth
  step is an allocation inside the window the criterion forbids.
- **D-3 Percentile method**: exact nearest-rank over the sorted chunk
  values after the run. The vendored HdrHistogram stays out of the
  result path; it can arrive with N8 (FR-011, PC-3).
- **D-4 Default percentiles**: p50, p90, p99, p99.9. The list option
  takes a comma list of values in the open interval (0, 100); anything
  else exits with status 2 (FR-012, FR-021, FR-022).
- **D-5 Default output**: off, so the current console report text does
  not change (FR-023).
- **D-6 Merged threaded percentiles**: the union of the chunk values of
  all contexts, sorted; each context also gets its own row (FR-014).
- **D-7 Raw points stay in memory** for N3; this feature writes no file
  (FR-009).
- **D-8 Short runs**: a run with fewer chunks than a rank needs reports
  that percentile as not available, with no extrapolation (FR-013).
- **D-9 Version and merge order**: the first of the three parallel
  features to merge takes 0.11.0, a later one rebases and takes the
  next free minor; the harness `SOVERSION` moves 6 to 7, because the
  public surface of the harness archive grows (FR-030, FR-031); N1
  merges before N4, because both change the runner and the report
  (FR-035).

## Audit point

Read on 2026-10-19 at `ab32400` on Gary, the machine this feature
develops on: one socket, one memory node, 16 cores and 32 hardware
threads, two L3 groups. No claim in this spec depends on that shape;
the threading tests run on the fake provider.

## Preconditions

### PC-1: H5 execution contexts merged. PASS.

`ab32400` is the H5 head, and `specs/020-execution-contexts-core/`
carries its closed artifacts. Each context opens its own thread-target
counter plan, and no fold crosses contexts: `source/harness/runner.cpp`
states it beside the per-context fold ("no fold crosses contexts"), and
`specs/020-execution-contexts-core/spec.md` PC-1 records the plan rule.
This feature inherits that rule (FR-004).

### PC-2: the run loop, calibration, and the pause reservation. PASS.

- The run loop and the per-attempt recorder: `Runner::run` in
  `source/harness/runner.cpp`; the pause room guards at the
  `kMaxPauseSamplesPerLeaf` tests in that file.
- The bound itself: `kMaxPauseSamplesPerLeaf` (32'768 raw points per
  leaf column) in `source/harness/detail/calibration.hpp`, beside
  `kIterationCap` and `kRunBound`.
- The recorder API: `RecorderHandle` in
  `include/speedgun-ng/counters_measurement.hpp`, whose raw point
  column, point identity, and fold disclosure are the E-10 surface.
- The fold: `foldCore` in `source/counters/fold.cpp`, and the pause
  netting helper `nettedFold` in `source/harness/runner.cpp`.
- Today's cadence: one sampling window per run. The main page states
  the count is "the sample pair taken at the window ends", and the
  overhead floor is the median cost of one sampling window
  (`README.md`, "Reading the report").
- Today's aggregate statistics: `mean=`, `median=`, `sd=`, `cv=`,
  `min=`, `max=`, `n=` in `printResult` in `source/harness/report.cpp`.
  No percentile row exists, and
  `test/source/harness_statistics_test.cpp` asserts the percentile form
  stays out of the aggregate row.

### PC-3: the vendored HdrHistogram. PASS, and the harness uses none of it.

`external/hdrhistogram_c` is pinned at 0.11.10, and
`source/hdrhistogram/hdrhistogram_gate.cpp` carries the version
tripwire and the link proof. That translation unit holds zero runtime
lines, so the histogram is a vendored dependency with no harness use
today. D-3 keeps it out of the result path.

### PC-4: the docs budget. PASS, and the page has no headroom.

`README.md` stands at 638 lines, and `PAGE_LINE_CAP` in
`tools/docs/example_drift.py` is 638. `SECTION_LINE_CAP` is 25, with
two recorded exceptions in `SECTION_LINE_CAPS`. A new section therefore
cannot fit without a recorded cap move (FR-029, budget note).

## Roadmap position

N1 of the harness roadmap, the item the roadmap numbered H6. It opens
the gary lane: N1, then N3 JSON reports, then N5 comparison, then N7,
the path to the CI gate. The raw points this feature keeps are the
input N3 reports.

Two features run in parallel on other machines: N2 metric formulas on
corsairone and N4 topology placement on thrawn. The three are
orthogonal by design.

## Parallel work and the version rule

- **Shared files**: the project `VERSION` in `CMakeLists.txt`, the two
  `SOVERSION` lines, `README.md`, the quick-reference table,
  `PAGE_LINE_CAP` in `tools/docs/example_drift.py`,
  `example/CMakeLists.txt`, the `example_run` list, and the `test/`
  target lists. `master` at `ab32400` carries no changelog file.
- **Ownership**: this feature edits no source file of N2 or N4. An edit
  outside the feature that touches another file stops and records the
  reason in the plan first (FR-035).
- **Conflict rule**: rebase onto the new `master`; no merge. Keep the
  lines of both features in every shared list, raise `PAGE_LINE_CAP`
  again by the size of this feature's own section, then run the full
  gate set and push once with `--force-with-lease`, showing
  `git ls-remote origin <branch>` after the push.
- **Version rule**: D-9. The first feature to merge takes 0.11.0; a
  later one rebases and takes the next free minor. The harness
  `SOVERSION` moves 6 to 7 for the grown public surface of the harness
  archive, and the archive `SOVERSION` stays 2 (FR-031).

## User Scenarios & Testing *(mandatory)*

### User Story 1 - See the tail of a run (Priority: P1)

A developer measuring a lock-free queue cares about the worst iteration
as much as the average. They turn percentile output on, run the family,
and read p50, p90, p99 and p99.9 of the time per iteration beside the
row they already read. The tail is where a pause, a page fault, or a
cache miss shows up, and the mean hides it.

**Why this priority**: this is the whole reason the item exists. The
current report answers "how fast on average" and answers nothing about
the tail, and Principle VII asks for distributions with the long tail
investigated.

**Independent Test**: run one docs example with percentile output on and
read the printed rows. The rows exist, carry the four default
percentiles, and each value equals the nearest-rank value computed from
that run's raw points by an independent oracle in the test.

**Acceptance Scenarios**:

1. **Given** a benchmark family and percentile output off, **When** the
   run completes, **Then** the console report matches the report at
   `ab32400` for the same scripted inputs.
2. **Given** the same family with percentile output on, **When** the
   run completes, **Then** the report gives one value for each of p50,
   p90, p99 and p99.9 of the time per iteration.
3. **Given** a scripted run whose chunk values the test chose, **When**
   the report prints a percentile, **Then** the printed value equals the
   nearest-rank value of that sorted set.
4. **Given** percentile output on, **When** the report prints the row,
   **Then** the chunk size in iterations is disclosed.

---

### User Story 2 - Read the tail of every column (Priority: P1)

The same developer attached a counter metric and a user counter to the
family. With percentile output on, each reported column gets its own
percentile set: the time columns and each counter column, in the unit
that column already prints in.

**Why this priority**: a tail on a cache-miss counter is the diagnosis
the time tail only hints at, and the roadmap goal names "each user
counter" beside the time.

**Independent Test**: run a docs example that attaches one counter
metric and one user counter with percentile output on, and check that
every reported column carries the requested percentiles in its own
unit.

**Acceptance Scenarios**:

1. **Given** a family with two counter columns and percentile output
   on, **When** the run completes, **Then** each counter column carries
   the requested percentiles.
2. **Given** a column the run cannot count, **When** the report prints
   its percentiles, **Then** each of them reports as not available.

---

### User Story 3 - Read the tail of a threaded run (Priority: P2)

A run on two contexts merges into one row today. With percentile output
on, the merged row carries the percentiles of the union of both
contexts' chunk values, and each context carries its own row, so a
reader can see that one context took the tail and the other did not.

**Why this priority**: threaded runs are already a first-class report
shape (H5), and a merged tail can hide a per-context problem. It waits
behind P1 because the single-context case delivers the value on its
own.

**Independent Test**: run a two-context scripted family on the fake
provider with percentile output on, and check the merged set against
the nearest-rank value over the union of the two contexts' values and
each context's set against its own values.

**Acceptance Scenarios**:

1. **Given** a threaded run of two contexts with percentile output on,
   **When** the report prints the merged row, **Then** it carries the
   percentiles of the union of both contexts' chunk values.
2. **Given** the same run, **When** the report prints the context rows,
   **Then** each context carries the percentiles of its own chunk
   values.
3. **Given** the same run, **When** the values are folded, **Then** no
   fold reads the points of another context.

---

### User Story 4 - Choose the percentiles you want (Priority: P2)

A developer chasing a p99.9 regression asks for one number:
`--percentiles=99.9`. A developer comparing two builds asks for a
wider set. A mistyped value stops the run before any benchmark
executes.

**Why this priority**: the default set covers the common case, so the
value is in control and in the failure behavior.

**Independent Test**: drive the command line with a scripted suite: a
valid list, a list with repeats, and one invalid value each of 0, 100,
a negative, a non-numeric, and an empty element.

**Acceptance Scenarios**:

1. **Given** `--percentiles=99.9`, **When** the run completes, **Then**
   the report carries p99.9 alone.
2. **Given** a list with a repeated value, **When** the report prints,
   **Then** each distinct value prints once, in ascending order.
3. **Given** `--percentiles=100`, **When** the program starts, **Then**
   it prints one line and exits with status 2, and no benchmark runs.

---

### User Story 5 - See what the capture costs (Priority: P2)

Sampling more often costs something. The report shows the measured cost
of one sample point beside the percentiles, so a reader can tell how
much of a chunk's time the capture itself pays, and can push the chunk
size until that share is acceptable.

**Why this priority**: a benchmarking framework that adds unmeasured
cost to a measurement is a defect, and the roadmap names this as the
risk of the item.

**Independent Test**: read the disclosed cost from a scripted run and
check it against the calibration bound of 1% of the chunk time.

**Acceptance Scenarios**:

1. **Given** percentile output on, **When** the report prints the
   percentiles, **Then** the cost of one sample point is disclosed
   beside them.
2. **Given** the calibrated chunk size, **When** the disclosed cost is
   compared with the chunk's own measured time, **Then** the cost stays
   at most 1%.

---

### User Story 6 - Read the page, copy the block, run the example (Priority: P3)

A reader opens the main page, reads the Percentiles section, copies the
block over the canonical includes, builds it, and gets the rows the
section's result sentence names.

**Why this priority**: Principle IV.1 makes the main page an obligation
of the feature, and the drift and run gates enforce it.

**Independent Test**: `python3 tools/docs/example_drift.py .` reports no
finding, and the `example_run` gate runs the program that carries the
region and prints the rows.

**Acceptance Scenarios**:

1. **Given** the merged page, **When** the drift gate runs, **Then** the
   Percentiles section, its region, and its quick-reference rows agree.
2. **Given** the docs example, **When** `example_run` runs it, **Then**
   every row its result sentence names prints.

---

### Edge Cases

- A run with fewer chunks than a rank needs: that percentile reports as
  not available, and no value is extrapolated (FR-013).
- A run of one chunk holds one chunk value, below the minimum p50
  names, so every percentile of that column reports as not available
  (FR-013).
- A column the run cannot count, or a column whose fold is disclosed as
  unavailable: every percentile of that column reports as not available
  (FR-016).
- A chunk that holds a paused interval: the paused time comes out of
  that chunk's value, following the pause netting rule of the run
  (FR-015).
- A manual-time family: no primary fold exists, so the primary time
  percentiles report as not available while the counter columns report
  where the recorder holds chunk points (Assumptions).
- A skipped or disabled instance: no run, so no percentile row.
- A percentile list with a repeated value, an unsorted list, or a value
  with more decimals than the column prints: one row per distinct value
  in ascending order (FR-024).
- A percentile value of 0, of 100, a negative, a non-numeric, an empty
  element, or a list over the option's stated bound: one line on
  standard error and exit status 2 before any benchmark runs
  (FR-022).
- A run whose chunk count would exceed the raw point bound: calibration
  raises the chunk size until the count fits, and the disclosure states
  the chunk size that was used (FR-006).
- A threaded run where one context produces no chunk value: the merged
  set holds the values that exist, and the empty context reports its own
  percentiles as not available.
- Percentile output on with `--dry-run`: no run happens, so no
  percentile row prints.

## Requirements *(mandatory)*

### Raw capture (C-1)

- **FR-001**: The timed path of a run shall leave one raw point per
  column at each chunk boundary of that run, where a column is the
  primary time, the CPU time, and each user counter of the run. That
  cadence replaces the single sampling window a run takes today, and
  the value of the whole run folds over the same point column the
  percentiles read (PC-2).
- **FR-002**: The value of one chunk shall be the difference of that
  column's two boundary points, and the report shall divide the primary
  time chunk value by the chunk's iteration count before printing it.
  Two consecutive chunks share the point between them, so a run of N
  chunks leaves N+1 points in a column.
- **FR-003**: The fold that turns raw points into a reported value
  shall happen after the timed window closes.
- **FR-004**: Each execution context of a threaded run shall hold its
  own raw points, and no fold shall read the points of another context
  (PC-1).
- **FR-005**: The chunk size shall come from calibration, which shall
  pick a chunk whose sample cost stays at most 1% of that chunk's own
  measured time (D-1).
- **FR-006**: One constant, of the kind `kMaxPauseSamplesPerLeaf` is,
  shall bound the raw point count of one column. Calibration shall keep
  the chunk count inside that bound by raising the chunk size, and no
  raw point shall be dropped to fit.
- **FR-007**: The report shall disclose the chunk size, in iterations
  per chunk, of a run that captures.
- **FR-008**: The raw point storage of one context shall be reserved
  before the timed window opens, at a capacity bounded by
  `kMaxChunkPointsPerLeaf` points per column; no allocation shall happen
  inside the timed window (D-2, FR-006).
- **FR-009**: The chunk values of a finished run shall stay readable in
  the process after the run, and this feature shall write no raw point
  to a file (D-7).

### Percentiles (C-2)

- **FR-010**: When percentile output is on, the report shall give one
  value for each requested percentile of each reported column of the
  run, on the row of that run and of each repetition. The `AGGREGATE`
  row keeps the statistics it prints today and gains no percentile row.
- **FR-011**: A reported percentile shall be the exact nearest-rank
  value of the sorted chunk values of that column (D-3).
- **FR-012**: The default percentile list shall be p50, p90, p99 and
  p99.9 (D-4).
- **FR-013**: A percentile shall report as not available when that
  column holds no chunk value, or when the chunk count falls below the
  minimum the percentile names: a percentile p needs at least
  `ceil(100 / (100 - p))` chunk values, so p50 needs 2, p90 needs 10,
  p99 needs 100 and p99.9 needs 1000. No value shall be extrapolated
  (D-8).
- **FR-014**: A threaded run shall report the percentiles of each
  context and the percentiles of the merged run, where the merged set is
  the union of the chunk values of every context (D-6).
- **FR-015**: A chunk that holds a paused interval shall have the paused
  time removed from that chunk's value, following the pause netting
  rule of the run (PC-2).
- **FR-016**: A column the run cannot count shall report every
  percentile of that column as not available.

### Observer cost (C-3)

- **FR-017**: The measured cost of one sample action shall print in the
  `overhead floor (ns)=` field the row carries today, that field holding
  the median cost the plan calibrates for one sample; no new field shall
  enter the row.
- **FR-018**: That cost shall come from the counters library, like every
  other time and counter value of the row.

### Options (C-4)

- **FR-019**: A benchmark handle call shall turn percentile output on or
  off for one family.
- **FR-020**: A command-line option shall turn percentile output on or
  off for every family of the run, and a command-line setting shall win
  over a family setting, as `--repetitions` does today.
- **FR-021**: A command-line option shall set the percentile list of the
  run as a comma-separated list of values in the open interval (0, 100)
  (D-4).
- **FR-022**: An invalid percentile value shall print one line on
  standard error and exit with status 2 before any benchmark runs, on
  the usage-error path the existing options already take (PC-2).
- **FR-023**: Percentile output shall be off by default. With the output
  off, no chunk disclosure line and no percentile line shall print, and
  the run row shall keep every field it prints today (D-5). The
  `endpoint samples=` field counts the points the cadence leaves, so its
  value moves with the cadence whatever the output setting stands at.
- **FR-024**: A percentile list with repeated values shall print one row
  per distinct value, in ascending order.

### Main-page documentation (C-5)

- **FR-025**: The main page shall gain a "Percentiles" section of at
  most 25 lines: one purpose sentence, one marked snippet, one result
  sentence (Principle IV.1).
- **FR-026**: The snippet shall come from a runnable example under
  `example/docs/` that CI builds through a `run_docs_` target and runs
  through an `example_run` entry.
- **FR-027**: The quick-reference table shall gain the rows of the new
  handle call and the new options.
- **FR-028**: `docs/pages/harness.md` shall carry the detail: the chunk
  rule, the rank rule, the merged-threaded rule, and the not-available
  rule.
- **FR-029**: `PAGE_LINE_CAP` shall rise by the measured size of the new
  section and its rows, through the recorded decision of the budget note
  (PC-4).

### Surface, version, and discipline

- **FR-030**: The feature takes the next free minor version: 0.11.0 when
  it merges first, the next free minor after a rebase otherwise (D-9).
- **FR-031**: The harness `SOVERSION` moves from 6 to 7, because the
  public surface of the harness archive grows: a handle call and the
  fields of a result row. The constitution's Refactoring and Evolution
  rule keeps `SOVERSION` consistent with a public API change, and the
  same rule moved that `SOVERSION` from 5 to 6 for H5. The `speedgun-ng`
  archive `SOVERSION` stays 2, because the counters library gains
  nothing.
- **FR-032**: New code ships with tests at 100% line and 100% branch
  coverage, with no `LCOV_EXCL` marker, waiver, residual row, or open
  row. Where a gap stays, the change stops and reports the cause with
  `gcov -b` evidence.
- **FR-033**: Tests shall use the fake provider and barriers. No test
  shall sleep, and no test shall read a wall clock.
- **FR-034**: The time-source gate shall stay clean over every file this
  feature adds or edits.
- **FR-035**: This feature shall edit no source file owned by N2 or N4.
  A shared-list edit shall keep the lines of both features after a
  rebase, and an edit outside that boundary shall stop and record its
  reason in the plan first.

## Key Entities *(include if feature involves data)*

- **Raw point**: one reading of one column at one chunk boundary of one
  run on one context. It is the unit of capture and the input of both
  the fold and the percentile.
- **Chunk**: a group of consecutive iterations of one run on one
  context, bounded by two raw points. Its size in iterations is a
  calibration outcome and is disclosed.
- **Chunk value**: the difference of a column's two boundary points,
  the sample one percentile is computed over. The primary time chunk
  value is divided by the chunk's iteration count before it prints.
- **Raw point column**: the ordered raw points of one column of one
  context of one run, bounded in count (FR-006), reserved before the
  timed window (FR-008), and shared at each interior boundary by the
  two chunks that meet there.
- **Percentile set**: the ordered list of percentages a user asked for,
  defaulting to p50, p90, p99 and p99.9, each in the open interval
  (0, 100) and each carrying the minimum chunk count FR-013 names.
- **Observer cost**: the measured cost of one sample action, printed in
  the row's existing overhead floor field so the reader can see what
  the capture pays.
- **Percentile option**: the family handle call and the two command-line
  options that select the output and the list, with the command line
  winning.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: With percentile output on, a run prints one value per
  requested percentile per reported column, and every printed value
  equals the nearest-rank value an independent oracle computes from that
  run's chunk values.
- **SC-002**: With percentile output off, no percentile line and no
  chunk disclosure line prints, and each run row keeps every field it
  carries today; the `endpoint samples=` value of that row counts the
  points the chunked cadence leaves.
- **SC-003**: At the calibrated chunk size, the disclosed cost of one
  sample point stays at most 1% of that chunk's measured time, on the
  fake provider test and on a real run on Gary.
- **SC-004**: A test that counts allocations across a timed window
  reports zero allocations for a capturing run.
- **SC-005**: For a two-context scripted run, the merged percentile
  equals the nearest-rank value over the union of both contexts' chunk
  values, and each context's percentile equals the nearest-rank value
  over its own values.
- **SC-006**: Each invalid percentile form (0, 100, a negative, a
  non-numeric, an empty element) exits with status 2 after one line on
  standard error, with no benchmark run.
- **SC-007**: A run with fewer chunks than a rank needs reports that
  percentile as not available, and the report carries no extrapolated
  value.
- **SC-008**: The coverage gates report 100% line and 100% branch over
  the new code with no exclusion marker.
- **SC-009**: The main page's Percentiles section prints the rows its
  result sentence names, through `example_run`, and
  `python3 tools/docs/example_drift.py .` reports no finding.
- **SC-010**: The chunk value columns of a finished run stay readable in
  the process at the end of that run, which is the input N3 consumes.
- **SC-011**: The full gate set passes on the branch: the `ci-ubuntu`
  and `ci-rocky` builds, `ctest`, `ci-sanitize`, `ci-tsan`, clang-tidy
  and cppcheck with no new finding, `format-check`, `spell-check`,
  `prose-lint`, the time-source gate, the page-drift gates, and the
  coverage gates.

## Constraints

- **Counters-only.** Every time and counter value comes from
  `sg::counters`, and `test/time_source_gate.sh` stays clean (Additional
  Constraints of the constitution).
- **No allocation in the timed window.** The raw arrays are reserved
  before the window opens (FR-008).
- **No fold crosses contexts.** One context folds its own points
  (FR-004, PC-1).
- **Contracts at the boundary.** An invalid percentile on a handle call
  is a contract violation; an invalid command-line value is a usage
  error that prints one line and exits with status 2.
- **Coverage.** 100% line and 100% branch, no `LCOV_EXCL` under
  `source/harness/` (FR-032).
- **Test discipline.** The fake provider and barriers; no sleep, no
  clock (FR-033). `ctest -j 2` stays the runner. The coverage preset
  keeps `-fprofile-update=atomic` and stays free of
  `-D_GLIBCXX_ASSERTIONS`.
- **Docs budget.** `SECTION_LINE_CAP` stays 25; the page cap moves by
  the measured amount through a recorded decision (FR-029).
- **Prose and commits.** Principle XI and the commit lint bind every
  word and every commit; commit messages carry blank lines between body
  paragraphs and one footer block with `Approved-by:` then `Refs:`.
- **Rebase discipline.** Rebase, push with `--force-with-lease`, and
  show `git ls-remote origin <branch>` after each push.

## Scope

In: FR-001 to FR-035. The chunked capture, the percentile rows for the
time and the counter columns, the per-context and merged threaded
percentiles, the observer cost disclosure, the family call and the two
command-line options, the main-page section with its runnable example,
the quick-reference rows, the `docs/pages/harness.md` detail, the tests,
and the version effect.

Out, each owned by a later roadmap item:

- JSON and other machine-readable reports (N3);
- comparison against a baseline (N5);
- histograms, plots, and any use of the vendored HdrHistogram (N8);
- metric formulas (N2, corsairone);
- topology placement and presets (N4, thrawn);
- NUMA memory-node placement (N12);
- any file output of raw points; this feature keeps them in memory
  (FR-009).

## Assumptions

- The roadmap file the prompt names sits outside this repository and is
  absent from this machine, so no claim here rests on it. The prompt
  text is the authority for scope.
- A percentile is a property of one run: the `AGGREGATE` row keeps
  `mean`, `median`, `sd`, `cv`, `min`, `max` and `n` as it prints them
  today and gains no percentile row. `test/source/harness_statistics_test.cpp`
  already asserts the percentile form stays out of that row.
- A manual-time family has no primary fold, so its primary time
  percentiles report as not available while its counter columns report
  where the recorder holds chunk points.
- The command line wins over the family setting, matching
  `--repetitions` and `--min-time`; the plan confirms that precedence
  against the base it runs on.
- A percentile prints in the unit of the column it describes, the same
  scale the row already prints that column in.
- The chunk boundary points reuse the recorder's raw point column and
  the existing fold machinery; the plan names the exact calls, and the
  bound of FR-006 is a constant of the same kind as
  `kMaxPauseSamplesPerLeaf`.
- Threading tests run on the fake provider, so no test needs hardware,
  a PMU, or privileges.
- Percentile output is a console report feature in this item; a later
  item decides how the same values reach a machine-readable report.

## Budget note

`README.md` stands at 638 lines of a 638 cap (PC-4), so the Percentiles
section and its quick-reference rows cannot fit as they stand. The cap
moves by the measured amount when the section lands, the smallest
fitting amount, recorded here and in the gate's own comment, the way
`specs/020-execution-contexts-core` recorded its move from 570 to 638.
`SECTION_LINE_CAP` stays 25, prose is trimmed first, no snippet is
dropped, and no earlier spec is edited. If a rebase brings another
feature's section in first, this feature's cap move is measured against
the page as the rebase leaves it.

## Dependencies

- The counters library: `RecorderHandle` with its raw point column and
  point identity, `foldCore`, and `FakeProvider`
  (`include/speedgun-ng/counters_measurement.hpp`,
  `source/counters/fold.cpp`, `include/speedgun-ng/counters_fake.hpp`).
- H1's run loop and calibration: `Runner::run` in
  `source/harness/runner.cpp` and `source/harness/detail/calibration.hpp`
  (`kIterationCap`, `kRunBound`, `kMaxPauseSamplesPerLeaf`).
- H4's pause netting and counter column pass: `nettedFold` and
  `appendCounterColumns` in `source/harness/runner.cpp`.
- H5's execution contexts and per-context fold (PC-1).
- H4's console report: `printResult` in `source/harness/report.cpp`.
- The command-line scanner: `source/harness/cli.cpp` with its
  usage-error exit path.
- D1's main-page structure and drift gate: `README.md`,
  `tools/docs/example_drift.py` (`SECTIONS`, `PAGE_LINE_CAP`,
  `SECTION_LINE_CAP`, `SECTION_LINE_CAPS`), and the example run
  machinery (`add_docs_example`, the `run_docs_` targets, and
  `run-examples` in `example/CMakeLists.txt`).
- The constitution gates of PC-3 and VIII: the counters-only scan, the
  naming rules, Principle II, IV.1, the coverage gates, `ci-sanitize`,
  `ci-tsan`, clang-tidy and cppcheck, and the prose and commit lint.

## Citations

Symbol and section anchors, each read at the audit point `ab32400`.

| Artifact | Location |
| --- | --- |
| `RecorderHandle`, raw point column, point identity, fold disclosure | `include/speedgun-ng/counters_measurement.hpp` |
| `foldCore`, pair folds, availability | `source/counters/fold.cpp` |
| `Runner::run`, per-attempt recorder, pause room guards | `source/harness/runner.cpp` |
| `nettedFold`, `appendCounterColumns` | `source/harness/runner.cpp`, declared in `source/harness/detail/internal.hpp` |
| `kIterationCap`, `kRunBound`, `kMaxPauseSamplesPerLeaf` | `source/harness/detail/calibration.hpp` |
| Per-context fold and the no-cross-thread rule | `source/harness/runner.cpp`, `specs/020-execution-contexts-core/spec.md` PC-1 |
| Console row and aggregate statistics | `printResult`, `source/harness/report.cpp` |
| Option scanner and usage-error exit | `source/harness/cli.cpp` |
| Percentile form kept out of the aggregate row | `test/source/harness_statistics_test.cpp` |
| Allocation-counting precedent | `test/source/counters_noalloc_test.cpp` |
| Vendored histogram and its gate | `external/hdrhistogram_c`, `source/hdrhistogram/hdrhistogram_gate.cpp` |
| Docs example machinery | `add_docs_example`, the `run_docs_` targets, `run-examples` in `example/CMakeLists.txt` |
| Drift gate caps and section list | `tools/docs/example_drift.py` |
| Main page structure | the "How you do it", "Quick reference", and "Planned work" sections of `README.md` |
| Harness guide | `docs/pages/harness.md` |
| Gate scripts | `test/time_source_gate.sh`, `test/loop_shape.sh`, `test/barrier_shape.sh`, `test/cli_shape.sh` |
| Gate registration and CI jobs | `test/CMakeLists.txt`; the `lint`, `coverage`, `sanitize`, `tsan`, `dbc-gate`, and `prose-lint` jobs of `.github/workflows/ci.yml` |
| Version and `SOVERSION` | the `project()` version and the target properties in `CMakeLists.txt` |
| Constitution IV.1, VIII gates, II, V.1, counters-only, footer | `.specify/memory/constitution.md` |
| H5 artifacts | `specs/020-execution-contexts-core/spec.md` and its `tasks.md` |

## Open questions

The prompt carried its open items as recommendations, and the owner
accepted each on 2026-10-19, so the decisions stand at D-1 to D-9 and
again at the FR that carries them. The clarification session of
2026-10-19 resolved the two points that had waited for the plan, and
its record sits under Clarifications. No open item remains.

Two code-level names still wait for the plan, and neither is an open
question of behavior: the constant that carries the FR-006 bound, read
from the recorder's own capacity, and the spelling of the FR-019 handle
call, which selects the output and carries no percentile list.

## Constitution Checklist

- Main-page impact check (IV.1): YES. The feature changes user-visible
  behavior, so FR-025 to FR-029 carry the Percentiles section, its
  `example/docs/` region with its `run_docs_` target and `example_run`
  entry, its quick-reference rows, and the `docs/pages/harness.md`
  detail in the same change, inside the budget the budget note records.
