# Feature Specification: Fixed Work Quanta and Fixed Time Quanta Probes

**Feature Branch**: `043-fwq-ftq`

**Created**: 2026-11-01

**Status**: Draft

**Input**: User description, opening quoted:

> FWQ and FTQ probes, item N13c of the harness roadmap (section 3 row
> N13c, section 10.3 "N13c", section 10.5 D-13.9). Short name: fwq-ftq.
> N13c adds two more probe methods to the same probe, a second probe
> framework stays out. FWQ repeats one fixed unit of work, a
> calibrated loop through `sg::doNotOptimize`, and records the time of
> each unit. FTQ counts the work units done in each fixed quantum,
> 100 us by default. A low count shows noise. Results per probe cpu
> carry the series statistics, the percentiles from the N1
> nearest-rank code, and the dominant periods of the series from a
> plain DFT, with no new library.

## Spec identity and run-time binding

- Spec number: `043`. `specs/` on master `d454bcff` holds `001`
  through `040` (read 2026-11-01). `gh pr list --state all` names no
  open pull request and no branch for `041`, `042` or `043`; the
  newest merged pull request is `68` for the CI record check and the
  pre-push step, merged 2026-11-01 (read 2026-11-01).
  `git ls-remote --heads origin` names no ref carrying `041`, `042` or
  `043` (read 2026-11-01). The lane prompt reserves `041` for N6-P on
  gary and `042` for N10 on corsairone, so this lane takes `043` and
  records the check.
- Branch: `043-fwq-ftq`, cut from master `d454bcff` (2026-11-01) on
  2026-11-01. An early read of this clone stood one merge behind at
  `3123ce66` (2026-10-31); `git fetch` moved `origin/master` to
  `d454bcff`, the base the lane prompt names, and the branch moved
  with it. Every line number in this spec is read at `d454bcff`, and
  each symbol is named beside its line so a moved line stays findable.
  The move shifted three cites: the shape-fold block, the two
  `example_drift.py` caps, and the option-row count.
- Version: master stands at `0.28.0` (`CMakeLists.txt:45`), harness
  `SOVERSION 19` (`CMakeLists.txt:375`), archive `SOVERSION 5`
  (`CMakeLists.txt:110`). The lane takes the next free minor,
  `0.29.0`. N6-P plans `0.29.0` and N10 plans the next, and neither
  has merged, so the lane that merges first keeps the lowest free
  minor and a later lane rebases and takes the next (FR-005, D-8).
- Harness `SOVERSION`: the method field joins the public
  `JitterProbeOptions` struct (`include/speedgun-ng/benchmark.hpp:2248`)
  and the series record joins the public surface beside
  `JitterStats` (`include/speedgun-ng/benchmark.hpp:512`), so the
  exported layouts move: the harness `SOVERSION` rises from `19` to
  `20` at rebase time. The archive `SOVERSION` stays at the `5` that
  specs/044 landed, because no public counters header changes in this
  lane. The commit that moves the numbers
  records each check beside them, the way the `CMakeLists.txt`
  comments record the ABI list of each lane that moved one (FR-005,
  D-8).
- Report schema: master stands at `1.11`
  (`source/harness/json_report.cpp:43`, `kSchemaVersion`). The series
  record, the calibration record and the period record are new keys
  under the standing root `jitter` object
  (`docs/schema/report-v1.json:45-155`), so the schema minor takes the
  next free value, `1.12`, at rebase time. The `jitter` object stands
  closed (`docs/schema/report-v1.json:65`), so the new keys join its
  `x-order` and `properties` and stay out of its `required` list: a
  run with the default method writes the standing keys and nothing
  else (FR-006, D-7).
- Option values: no new option row. The method rides the standing
  `--jitter-probe` grammar as new keys, so the row stays at value
  `260` inside the `260` to `269` block the N13a lane recorded
  (`source/harness/detail/options.hpp:77`), and the same header
  records `297` to `319` free. The lane takes no value at all, so that
  free block stays clear for a later lane. The name and
  value uniqueness test
  (`test/source/harness_cli_table_test.cpp`) stays green with no new
  row (FR-001, D-1).

## Roadmap position

- Item N13c of the harness roadmap, section 3, with section 10.3 row
  N13c and section 10.5 D-13.9. The roadmap file stands outside this
  repository, so this spec carries the item's content and the
  decisions it fixes.
- N13a (spec `027`, merged) owns the probe: one SPIN context per
  probe cpu reads the TSC through `sg::counters` and records each gap
  above a threshold. N13b (spec `034`, merged) owns the pre-check
  window. N13e (spec `036`, merged) owns the kernel tracers. N13c
  adds two probe methods to the same probe context, the same cpu set,
  the same modes and the same overlap rules.
- Roadmap section 10.6 stays in force in full: no sleep and no clock
  read in a test, one injectable time source per probe, 100% line and
  branch coverage, a bad option exits 2, docs and a runnable example,
  no fixed cpu number in a test.
- The roadmap rulings in force for this item carry the labels AC-c1
  (fake-time tests script the work and time series and check the
  statistics and the period detection) and AC-c2 (the work-unit
  calibration runs before the window and is disclosed).

## Lane identity

- The acceptance host is the lane host: an AMD Ryzen 9 7950X3D, 16
  cores, 32 threads, one socket, two L3 groups, one per CCD. The
  two-CCD shape is the acceptance axis: FWQ and FTQ each run on one
  idle cpu per L3 group, and each recorded figure names the L3 group
  it came from. The host runs kernel `7.3.1-1-cachyos` with
  `CONFIG_HZ=1000`, so the timer tick period stands at 1 ms and the
  spectrum check has a known period to find.
- A second host of a different type (a Threadripper 9960X, Zen 5)
  stands optional: the same runs there add one evidence record and
  gate nothing.
- No acceptance is gated on GitHub runner CI. Every acceptance figure
  is measured on the owner's machines, and the pull-request body
  records the counts, the host and the kernel. The runners stand too
  noisy for noise figures.
- The suite itself stays host-agnostic: the time source and the cpu
  set come from injected seams, the cpus come from
  `sched_getaffinity`, and the suite passes under `taskset -c 0,1`
  and on a 2-cpu host (FR-024, FR-025).

## Audit point

- Repository: `master` at `d454bcff` (project version `0.28.0`,
  report schema `1.11`, harness `SOVERSION 19`, archive `SOVERSION
  4`), read 2026-11-01 on the lane host, and every cite below reads
  there.
- The `--jitter-probe` value grammar stands in
  `source/harness/jitter_probe.cpp:848-900`: the keys `threshold`,
  `gaps`, `mode`, `precheck` and `strict`, each read once, a bad key
  or value answering one quoted line with exit status 2. The row
  stands in the standing option table.
- The probe reads time only through `sg::counters`
  (`ClockProvider`), and `installTestReader`
  (`source/harness/jitter_probe.cpp:690`) is the test seam. The
  measured loop period stands at `loopPeriodNs`
  (`source/harness/jitter_probe.cpp:253`) and feeds the default
  threshold rule.
- `JitterStats` (`include/speedgun-ng/benchmark.hpp:512-530`) carries
  the sixteen sysjitter fields: `gapCount`, `gapsPerSecond`, `minNs`,
  `medianNs`, `meanNs`, `p90Ns`, `p99Ns`, `p999Ns`, `p9999Ns`,
  `maxNs`, `totalNs`, `totalPercent`, `runtimeNs`, `thresholdNs`,
  `loopPeriodNs`, `droppedGaps`. `JitterEntryFold`
  (`include/speedgun-ng/benchmark.hpp:645`) pairs one cpu with its
  stats.
- The percentiles use the N1 nearest-rank code, with the point set
  `p50`, `p90`, `p99`, `p99.9`, `p99.99` and the standing minimum
  sample counts. The series keeps its order and the code reads it in
  place: no copy of the buffer.
- `sg::doNotOptimize` stands in `include/speedgun-ng/barrier.hpp:59`
  and `:78`.
- The root `jitter` object stands at `docs/schema/report-v1.json:45`
  to `:155`: seven keys in `x-order` and `required` (`mode`,
  `thresholdNs`, `loopPeriodNs`, `cpus`, `stats`, `droppedGaps`,
  `disclosure`), `"additionalProperties": false` at `:65`, and a
  `stats` array whose item carries 18 `x-order` names with `gaps`
  optional (`:78-148`). `loadReport` reads the object back
  (`source/harness/json_load.cpp`).
- The console block prints the probe figures after the benchmark
  tables: one block for the run, opened by a cpu list, then one line
  per stats field with one column per probe cpu, then the disclosure
  lines (`docs/pages/harness.md:1798-1820`).
- The N5 rule-name grammar reads `jitter:<cpu|any>:<field>` and
  covers the standing field names of the stats record.
- The acceptance-script precedent stands at
  `tools/noise/jitter_probe_acceptance.sh`: a script, never a CI
  gate, writing one record per run under the feature's `results/`
  directory (`specs/027-jitter-probe/results`).
- The README stands at 1004 lines against `PAGE_LINE_CAP = 1004`
  (`tools/docs/example_drift.py:240`): zero headroom. The
  `### Jitter probe` section runs `README.md:718-740`, 23 lines
  against `SECTION_LINE_CAP = 25` (`tools/docs/example_drift.py:241`).
- The docs page section `## Jitter probe` runs
  `docs/pages/harness.md:1673-1909`, 237 lines, and the page carries
  no line cap.
- The docs-example registration stands at
  `example/CMakeLists.txt:40-48` (`add_docs_example`) with
  `add_docs_example(jitter_probe)` at `:78`, and the shape fold
  stands at `test/CMakeLists.txt:1914-1921`
  (`report_shape_jitter_sample`).

## Standing facts this lane reads

- The `tsc` method stands unchanged: a run with no `method` key
  behaves as spec 027 defines, writes the standing JSON keys, and
  prints the standing console lines.
- The gap-buffer rules of spec 027 bind the new series buffers: one
  pre-sized buffer per probe cpu, no allocation and no I/O inside the
  measured loop, and a full buffer counts each lost record and
  discloses the count.
- The default buffer size and the `gaps=<n>` key stand shared by all
  three methods: 65,536 entries by default, in the range 1 to
  16,777,216 (D-6).
- The pre-check window of spec 034 and the tracers of spec 036 stand
  unchanged: `mode=precheck` and `mode=both` work for every method,
  and a tracer runs beside any method (D-9).
- The time-source rule of the constitution binds every bound source
  file: time comes from `sg::counters`, and no `<chrono>` include, no
  `clock_gettime` and no `rdtsc` join (Principle VII).
- The schema checker `sg::detail::checkReport`
  (`source/harness/json_load.cpp:2613`) validates a report against
  `docs/schema/report-v1.json`, and the closed `jitter` object
  refuses a key the schema does not list
  (`source/harness/json_load.cpp:2532-2543`).
- The prose gate (`tools/prose/prose_gate.py`), the drift gate
  (`tools/docs/example_drift.py`), the coverage gate
  (`tools/dbc/coverage_gate.sh`, branch allowance 21) and the
  time-source gate (`test/time_source_gate.sh`) bind every line this
  lane writes.
## Clarifications

### Session 2026-11-01

- Q: Does one probe run carry one method, or does it carry the `tsc`
  gaps beside FWQ or FTQ on the same cpu? → A: One method per run. A
  user who wants both runs the probe twice. Two methods on one cpu
  change each other: each adds work to the loop the other measures
  (FR-001, D-1).
- Q: Which keys carry the method, the quantum and the work count? →
  A: `method=tsc|fwq|ftq`, `quantum=<us>` for FTQ with default 100 us
  and range 10 to 100,000 us, and `work=<n>` for FWQ with the default
  from calibration. A key that belongs to the other method is an
  invalid value (FR-001, FR-002, FR-003, D-1).
- Q: What does the calibration aim at, and how does it search? → A:
  One FWQ unit near 1 us, and one FTQ unit near one thousandth of the
  quantum. The search doubles the count until the unit time passes
  the target, then narrows by binary search, and every read uses the
  probe's own time source. The run discloses the count and the
  measured unit time (FR-010, FR-011, D-2).
- Q: What does the spectrum print, and how large a series does it
  read? → A: The top five periods with their relative power, and only
  a period whose power stands above three times the median bin. The
  transform reads at most 65,536 points (FR-018, FR-019, D-5).
- Q: How do the new methods meet the N13b pre-check and the N13e
  tracers? → A: `mode=precheck` works for every method. The strict
  refusal uses the method's own noise fraction against a `noise=`
  threshold in percent, 1 percent by default. The tracer is unchanged
  and runs beside any method (FR-004, FR-021, D-9).
- Q: What buffer size does a series use? → A: The standing default and
  the standing `gaps=<n>` key, shared by all three methods: 65,536
  entries (FR-014, FR-016, D-6).
- Q: Does a `tsc` run gain any new key in the JSON document? → A: No.
  The method, calibration, series and period keys stand absent for
  `tsc`, so the document matches master apart from the
  `schemaVersion` value and the version figures the report carries
  (FR-006, FR-020, D-7).

Round 2, from the clarification review of 2026-11-01:

- Q: Does the value keep the standing cpu-list head, given a value such
  as `method=fwq` carries none? → A: The head stays first and
  mandatory: the parser reads token 0 as the head, and a head token
  that carries `=` answers the standing cpu-token line. Every value in
  this spec carries the head, for example
  `--jitter-probe=idle,method=fwq` (FR-002, FR-003).
- Q: Do the new figures split into each benchmark's timed windows, the
  way the `tsc` gaps split? → A: Both levels resolve. The root record
  carries the method and the run-level series, and each benchmark
  entry's `jitter` object gains the same `series` array, folded over
  that benchmark's timed windows on the carrier
  `BenchmarkResult::jitterFolds`, the way the gap fold of spec 027
  reads the one gap buffer. No second buffer joins, because the fold
  reads the series buffer the probe cpu already owns (FR-020, FR-021).
- Q: Does the default buffer cover a 10 s acceptance window? → A: No,
  and the acceptance script says so: it passes `gaps=131072` for FTQ
  and `gaps=10485760` for FWQ, and each record names the kept span. A
  default run of 65,536 entries keeps 65.5 ms of FWQ and 6.55 s of
  FTQ and discloses the rest as drops (FR-014, FR-016, FR-027).
- Q: What is the denominator of each noise fraction? → A: FWQ divides
  the sum of the kept unit times, minus the sample count times the
  minimum, by that sum. FTQ divides the sample count times the maximum
  count, minus the sum of the counts, by the sample count times the
  maximum count. Both stand in the range 0 to 100, and a maximum count
  of 0 answers 100 (FR-009, FR-021).
- Q: Does the calibration run once per run, or once per probe cpu, and
  how large a search stands allowed? → A: Once per probe cpu, on that
  cpu, before its window. The doubling phase starts at 1 and runs at
  most 25 trials, counts 1 through 16,777,216, the bisection runs at
  most 24 steps, and each trial reads the time source twice around the
  body: at most 98 reads. The search ends at the trial cap, at the
  count cap, or when the bisection interval holds one count, so a
  scripted clock that never passes the target stands bounded. The
  prefix length follows the scripted deltas and stands at or below 98
  reads, so a test that scripts a converging clock knows its own
  length. The JSON
  carries the calibration inside each probe cpu's series item, and the
  calibration span stays out of the window's runtime (FR-010, FR-011,
  FR-019).
- Q: Where do the quantum edges stand, and does a trailing open quantum
  enter the count series? → A: Edge k stands at the window's first
  stamp plus k times the quantum, and only a closed quantum records a
  count, so the trailing open quantum stands dropped. The rule keeps
  the minimum of a clean run equal to the median and the maximum
  (FR-015, FR-016).
- Q: Which figures fill the standing pre-check lines for FWQ and FTQ,
  and in which phase does the noise-based refusal fire? → A: The line
  shape stays and the pair stands the method's own: the noise fraction
  and the maximum unit time for FWQ, the noise fraction and the minimum
  count for FTQ. The warning rule and the strict refusal read that pair
  against `noise=`, the pinning-candidate line ranks by the noise
  fraction ascending with ties in cpu order, and the refusal fires in
  the pre-check phase with one line and exit status 2. In
  `mode=alongside` the `strict` word keeps its standing `tsc` meaning,
  and a `noise=` value with no `strict`, or with `method=tsc`, stands
  accepted and disclosed as unused (FR-004, FR-021).
- Q: What fixed step does the FWQ resample use, and how does a sample
  land in a bin? → A: The step stands at the median unit time of the
  kept FWQ series, disclosed beside the period list. Each bin covers
  one step of cumulative unit time and holds the mean unit time inside
  it, and an empty bin takes the previous sample. With a 1 us median
  and the 65,536 cap the transform reads 65.5 ms, so a 1 ms period
  lands at bin 65, that is 1.008 ms, within one bin (FR-017, FR-019).
- Q: Which JSON keys do the new records carry, and where do they sit in
  the closed `jitter` object? → A: Two root keys, `method` in the set
  `TSC`, `FWQ`, `FTQ` and a `series` array with one closed item per
  probe cpu. The item carries `cpu`, the five calibration keys, the 12
  series figures, the two spectrum keys and a nested closed `periods`
  array of `periodNs` and `powerRatio`, so no name stands twice in one
  closed object. The console spells the method in lower case and the
  JSON in upper case, the standing split between the two (FR-006,
  FR-019, FR-020).
- Q: A closed object keeps seven root keys and 17 keys per `stats` item
  required: what does a FWQ run write for the gap figures? → A: The
  standing list stays. A FWQ or FTQ run writes `mode`, `cpus` and
  `disclosure` as today, `thresholdNs` and `loopPeriodNs` as the
  standing default rule measured them, `droppedGaps` 0, and one `stats`
  item per probe cpu with the 16 gap fields at 0. The new figures ride
  the new keys, so the standing reader and the byte-identity check of
  SC-004 keep one shape (FR-006, FR-020).
- Q: Does a bare rule name such as `jitter:any:noisePercent` resolve,
  given a bare name reads the per-benchmark record? → A: It resolves:
  each benchmark entry's `jitter` object gains the same `series` array,
  folded over that benchmark's timed windows on the carrier
  `BenchmarkResult::jitterFolds`, with no `method` and no `periods`.
  The fold reads the one series buffer, the way the gap fold of spec
  027 reads the one gap buffer, so no second buffer joins (FR-020,
  FR-021).
- Q: Which public carrier holds the method, the resample step and the
  point count when `loadReport` reads them back? → A: `JitterEntryFold`
  gains the series record and its period list, and `ReportData` gains
  one run record with the method word, the resample step and the point
  count riding the per-cpu series item, the way spec 037 moved the
  harness `SOVERSION` when
  `ReportData` gained its graph member. The ABI check of FR-005 names
  all four layout moves (FR-005, FR-020).
- Q: Where do the calibration and the period list sit: root level, or
  per probe cpu? → A: Per probe cpu. The search runs on the cpu that
  will run the loop, because the lane host's two L3 groups run at
  different clocks, so one shared count would miss the 1 us target on
  one group. The calibration keys ride inside that cpu's series item,
  and the period list rides there too, because the transform reads one
  cpu's series (FR-010, FR-011, FR-019, FR-020).
- Q: How many names does the rule-name table gain, and which contract
  records them? → A: 16 names, so `kJitterFields` grows from 16 to 32
  and the fault text reads `names none of the thirty-two jitter
  fields`. `runtimeNs` stays the gap figure, and the series figure
  takes the new name `seriesRuntimeNs`, so no name stands twice in the
  table and the lookup answers one entry. The rule-names contract of
  spec 027 gains the names, the count word and the `any` reading in the
  same commit as the table (FR-021, FR-030).
- Q: Which cpu does `any` name for a count figure, where a low count is
  the noise? → A: The uniform maximum stays, so `any` names the best
  cpu for a count. The contract records the reading, and a noise rule
  over an FTQ run names `noisePercent` with `higher` (FR-021).
- Q: What console line shape carries five periods per cpu in a block
  that prints one line per field with one column per cpu? → A: The
  standing shape stays, and each field prints once: `method`,
  `workCount`, `targetNs` and `calibrationSpanNs`, then the 16 figures
  of FR-021, which carry `unitTimeNs`, `quantumNs`, `resampleStepNs` and
  `spectrumPoints`, then `period1Ns` through `period5Ns` and
  `period1Power` through `period5Power`: 30 lines, each with one column
  per probe cpu, and a rank a cpu does not carry prints 0. The example's
  `.expected` file records the lines for the shape fold (FR-019,
  FR-029).
- Q: How many time-source reads does one FWQ unit take, and does the
  standing 1,024-read measurement pass run for the new methods? → A: One
  read per unit, the unit time standing at the delta between two
  consecutive reads, the shape the standing gap loop runs. The
  measurement pass runs for every method, because it sets
  `loopPeriodNs` and the default threshold rule, so a scripted FWQ
  series carries the calibration prefix, then 1,024 pass stamps, then
  one stamp per unit, then one tail stamp (FR-012, FR-015, FR-024).
- Q: Which series length does User Story 1 scenario 2 script? → A:
  10,000 units with one slow unit. The least sample count for the 99.99
  point stands at 10,000, so at 1,000 samples the rank lands on the slow
  unit and the stated assertion fails. At 10,000 samples the rank lands
  on the equal value, the maximum names the slow unit, and the noise
  fraction reads 0.09 (FR-008, SC-001).
- Q: What is one bin in the period tolerance, and over which bins does
  the median stand? → A: One bin is one frequency bin, `1 / (N x Ts)`,
  so the tolerance of a planted period `P` stands at `P^2 / (N x Ts)`:
  10 us at a 100 us quantum, 1,000 samples and a 1 ms period, so the
  test asserts the range 990,000 to 1,010,000 ns. The median stands over
  bins 1 through N/2, the DC bin out, so a constant series answers an
  empty list (FR-018, SC-002).
- Q: Which transform arms does the case list leave uncovered? → A: Three
  join it: a 70,000-sample series, where the transform reads the last
  65,536 points and discloses 4,464 skipped points; a six-tone series,
  where the list carries five entries and the sixth period stands out;
  and the FWQ resample path, reached by a run with a dip every 1,000
  units at a 1 us unit time (FR-018, FR-025, FR-032).
- Q: What wall-clock budget does the cap case carry? → A: The transform
  costs `N x N/2` multiply-adds and builds each bin's sine and cosine by
  recurrence, with no library call in the inner loop. Measured on the
  lane host on 2026-11-01: 3.3 s at `-O2` and 14.6 s at `-O0` with
  `--coverage` for one cap call, against the standing 300 s test timeout,
  so the suite carries one cap case and keeps the tone cases at or below
  4,096 points (FR-018, FR-025).
- Q: Which spans does the byte-identity check mask, and where does the
  master document come from? → A: An inline fixture of master's bytes,
  the shape `test/source/harness_noise_report_test.cpp:456-462` carries.
  The check masks `schemaVersion`, the version figures, and the
  host-measured probe fields `loopPeriodNs`, `thresholdNs`, `runtimeNs`,
  `gapCount`, `gapsPerSecond`, `totalNs`, `totalPercent` and
  `droppedGaps`, and each mask checks that the span holds a number, the
  way `maskHostFloor` does (FR-026, SC-004).
- Q: What shape does the acceptance record carry, and which commands
  make the window and the sibling load? → A: One markdown file per
  script run, one section per run: two methods, two L3 groups and two
  load states, eight sections, each naming the command line, the exit
  status and the captured output. The window comes from a named
  benchmark run with `--min-time=10` and `--repetitions=1`, so the
  window spans 10 s, and the load comes from a busy loop pinned to the
  SMT sibling thread of the probe cpu. `CONFIG_HZ` comes from `/proc/config.gz`, then
  `/boot/config-$(uname -r)`, and the record reads `unknown` when both
  stand absent (FR-027, FR-028, SC-008).

## Decisions

Decisions the request fixes for this feature, recorded as
requirements:

- **D-1: one probe, three methods (Q-1, Q-2).** `method=tsc` is the
  default and stands exactly as spec 027 defines. `fwq` and `ftq`
  run in the same probe context, with the same cpu set, the same
  modes and the same overlap rules. The keys ride the standing
  `--jitter-probe` value, each key standing once, and a key of the
  other method refuses one line with exit status 2 (FR-001, FR-002,
  FR-003).
- **D-2: the calibration runs before the window and is disclosed
  (AC-c2, Q-3).** The run finds the work count that puts one unit near
  its target time with the probe's own time source, before the
  measured window starts. The console and the JSON disclose the count
  and the measured unit time (FR-010, FR-011, FR-019).
- **D-3: FWQ times one fixed unit (roadmap 10.3).** One unit is one
  fixed count of a calibrated loop body closed on
  `sg::doNotOptimize`. The probe records the time of each unit into
  the pre-sized buffer under the standing gap-buffer rules (FR-012,
  FR-014, D-6).
- **D-4: FTQ counts units per fixed quantum (roadmap 10.3).** The
  quantum edges come from the time source, 100 us by default. The
  probe records the count of whole units finished in each quantum into
  the pre-sized buffer under the same rules (FR-015, FR-016, D-6).
- **D-5: the spectrum is a plain DFT (D-13.9, Q-4).** No new library
  joins. The transform reads the FTQ count series, or the FWQ time
  series resampled to a fixed step, and reports the top five periods
  with relative power above three times the median bin. The
  transform is a pure function, the direct sum is acceptable, and the
  point count stands capped at 65,536 (FR-017, FR-018, FR-019).
- **D-6: one buffer shape for every method (Q-6).** The series buffer
  is pre-sized from the standing `gaps=<n>` key, 65,536 entries by
  default, in the range 1 to 16,777,216. A full buffer counts each
  lost record, discloses the count, and the run keeps going (FR-014,
  FR-016, FR-022).
- **D-7: the record rides the standing jitter object (Q-2).** The
  console prints the new lines in the standing per-cpu block after the
  benchmark tables. The JSON carries the method, the calibration, the
  series statistics and the periods as new keys under the standing
  root `jitter` object, and `loadReport` reads them back. The keys
  stand absent for `tsc`, so a default run matches master apart from
  the `schemaVersion` value and the version figures the report
  carries (FR-019, FR-020, FR-021, D-8).
- **D-8: the ABI and the schema move once (roadmap 10.6).** The
  method field joins the public options struct and the series record
  joins the public surface, so the harness `SOVERSION` rises from `19`
  to `20`, the archive `SOVERSION` stays `5`, the version moves to
  `0.29.0`, and the schema minor moves to `1.12`. One commit carries
  the numbers and records each check beside them (FR-005, FR-006).
- **D-9: the standing phases and the tracers stay (Q-5).** `mode=
  alongside`, `mode=precheck` and `mode=both` work for every method,
  and the standing overlap refusal binds the alongside phase as it
  does today. The strict refusal reads the method's own noise fraction
  against `noise=<percent>`, 1 percent by default. The tracer of spec
  036 is unchanged (FR-004, FR-021).
- **D-10: the evidence is local (G-20).** The acceptance runs stand
  recorded under the feature's `results/` directory before the lane
  closes: FWQ and FTQ on one idle cpu per L3 group on the lane host,
  10 s each, with and without a load on a sibling cpu, each record
  naming the host, the kernel, `CONFIG_HZ`, the commands, the exit
  codes, the calibration figures and the detected periods. A script
  records it, and no CI gate stands on it (FR-026, FR-027).

## Out of scope

- A second probe framework, a second probe context, and a second
  timing window: the methods join the probe spec 027 owns.
- Two methods in one run, on one cpu or on two cpus of one run: one
  method per run, and a user who wants both runs twice (D-1).
- A new library for the spectrum, a windowed transform, a filter, a
  chirp transform, and an autocorrelation: the lane ships a plain DFT
  over a capped series (D-5).
- A standalone FWQ or FTQ window with no benchmark batch: the methods
  ride the standing probe lifecycle.
- Any change to the `tsc` gap rules, the pre-check window of spec 034
  and the tracer of spec 036. The standing warning rule, the
  pinning-candidate line and the host-settings lines keep their `tsc`
  figures: the FWQ and FTQ pair of FR-021 rides beside them.
- A new option row and a new option value: the keys ride the standing
  `--jitter-probe` value (D-1).
- A CI change: this lane needs none, and no gate configuration moves.
- A README line-cap raise: the lane trims its own section and keeps
  the page at or below 1004 lines (FR-023).
- A noise figure asserted about a real host in any test: the suite
  scripts its series, and the acceptance records carry the host
  figures (FR-025).
## User Scenarios & Testing *(mandatory)*

### User Story 1 - A fixed work quanta run shows each slow unit (Priority: P1)

A performance engineer wants to see the noise a host inserts into a
fixed unit of work. The run repeats one calibrated unit, records the
time of each unit, and prints the series statistics per probe cpu: the
minimum, the median, the mean, `p90` through `p99.99`, the maximum,
and the noise fraction. One slow unit shows as a tail above the
minimum, and the noise fraction states how much of the window the
slow units took.

**Why this priority**: the fixed work quanta series is the method the
roadmap item names first, and it is the method whose figures the
spectrum reads. A run that carries FWQ alone already tells the reader
more about noise than the `tsc` gap count alone.

**Independent Test**: with the injected time source and a scripted
work series, a run with `--jitter-probe=idle,method=fwq` prints the
calibration line and the per-cpu series block, and the statistics
match the scripted series. A clean series, one slow unit, and a full
buffer each have their own scenario.

**Acceptance Scenarios**:

1. **Given** a scripted series of 1,000 equal units, **When** the run
   reads it, **Then** the minimum, the median, the mean and the
   maximum stand equal, the percentiles stand equal to them, and the
   noise fraction stands 0.
2. **Given** a scripted series of 10,000 units with one unit at ten
   times the unit time, **When** the run reads it, **Then** the maximum
   names that unit, `p99.99` and `p99.9` stand at the equal value, and
   the noise fraction stands 0.09.
3. **Given** a series longer than the buffer, **When** the run fills
   the buffer, **Then** the run counts each lost record, discloses the
   count, keeps the window running, and the statistics read the kept
   records only.
4. **Given** a run with `method=fwq` on the lane host, **When** the
   calibration runs, **Then** the console and the JSON state the work
   count and the measured unit time before the window starts (AC-c2).

---

### User Story 2 - A fixed time quanta run shows each low count (Priority: P1)

A performance engineer wants to see how much work the host lets the
probe finish inside each fixed quantum. The run counts whole units per
quantum, 100 us by default, and prints the count statistics per probe
cpu: the minimum, the median, the mean, `p90` through `p99.99`, the
maximum, and the noise fraction against the best quantum. A quantum
with a low count shows the noise directly.

**Why this priority**: FTQ is the second method of the roadmap item,
and its count series is the natural input of the spectrum: one sample
per fixed quantum, so the period of a periodic noise source lands on
the frequency axis with no resampling.

**Independent Test**: with the injected time source and a scripted
quantum series, a run with `--jitter-probe=idle,method=ftq,quantum=1000`
prints the quantum, the calibration and the per-cpu count block, and
the statistics match the scripted counts.

**Acceptance Scenarios**:

1. **Given** a scripted series where every quantum finishes the same
   count, **When** the run reads it, **Then** the minimum, the median,
   the mean and the maximum stand equal and the noise fraction stands
   0.
2. **Given** a scripted series of 1,000 quanta where one quantum
   finishes half the best count, **When** the run reads it, **Then**
   the minimum names that quantum and the noise fraction stands 0.05.
3. **Given** a value with `quantum=10`, **When** the run binds the
   value, **Then** the quantum stands at 10 us and the calibration
   aims at one unit near 10 ns of target unit time.
4. **Given** a series longer than the buffer, **When** the run fills
   the buffer, **Then** the run counts each lost record and discloses
   the count.

---

### User Story 3 - The spectrum names the dominant periods (Priority: P2)

A performance engineer wants the period of a periodic noise source,
not only its size. The run reads a plain DFT over the series and
prints the top five periods with their relative power, keeping a
period whose power stands above three times the median bin. A 1 ms
timer tick shows as a 1 ms period.

**Why this priority**: the periods turn a series into a diagnosis. The
story rides the series the first two stories already produce, so it
lands after them.

**Independent Test**: the transform is a pure function, so a test
scripts a series with one planted period and reads the period list
back; the probe test scripts a series with a planted 1 ms dip and
checks the block prints that period.

**Acceptance Scenarios**:

1. **Given** a pure tone of a known period, **When** the transform
   reads it, **Then** the top period names the planted period within
   one bin and carries the largest power.
2. **Given** two tones of known periods, **When** the transform reads
   them, **Then** both periods appear in the list, the stronger first.
3. **Given** a constant series, **When** the transform reads it,
   **Then** the list stands empty and no period prints.
4. **Given** a scripted series with a dip every 1 ms, **When** a probe
   run reads it, **Then** the block prints a 1 ms period among the
   top five (AC-c1).

---

### User Story 4 - The report carries the method, the calibration and the periods (Priority: P2)

A user of stored reports wants the new figures in the JSON document
and back through the reader. The document carries the method, the
calibration, the series statistics and the periods under the standing
root `jitter` object. A run with the default method writes the
standing keys and nothing else.

**Why this priority**: the compare path, the regression gate and the
stored reports read the document, so the record is what makes the
figures usable after a run.

**Independent Test**: a scripted run writes the document, the schema
checker passes it against `docs/schema/report-v1.json`, and
`loadReport` reads every new key back.

**Acceptance Scenarios**:

1. **Given** a run with `idle,method=fwq`, **When** the report is written,
   **Then** the `jitter` object carries the method, the calibration,
   the series statistics and the periods, and `loadReport` reads them
   back.
2. **Given** a run with no `method` key, **When** the report is
   written, **Then** the document matches master apart from the
   `schemaVersion` value and the version figures the report carries.
3. **Given** a document with the new keys, **When** the schema checker
   reads it, **Then** the check passes with no finding.
4. **Given** a rule name `jitter:any:noisePercent`, **When** the
   regression gate reads a report that carries it, **Then** the rule
   resolves the field the same way the standing field names resolve.
5. **Given** the lane's build, **When** a reader checks
   `CMakeLists.txt` and the report, **Then** the version stands
   `0.29.0`, the harness `SOVERSION` stands `20`, the archive
   `SOVERSION` stands `5`, and the commit that moved them names the
   four layout moves.

---

### User Story 5 - A bad value refuses before anything runs (Priority: P2)

A user who mistypes a value wants one clear line and a fast exit. Each
bad key, each bad value and each key of the other method refuse one
quoted line on standard error with exit status 2, before the first run
starts.

**Why this priority**: the refusal shape is the standing contract of
the probe option, and the new keys extend it. A silent default would
hide a typo in a noise figure.

**Independent Test**: each row of the error table runs the option
parser with the injected value and checks the line and the status.

**Acceptance Scenarios**:

1. **Given** `method=x`, **When** the run parses the value, **Then**
   one quoted line names the method and the run exits 2.
2. **Given** `quantum=0` and `quantum=x`, **When** the run parses each
   value, **Then** each prints one quoted line and exits 2.
3. **Given** `work=0`, **When** the run parses the value, **Then** one
   quoted line names the work count and the run exits 2.
4. **Given** `idle,method=fwq,quantum=1000`, **When** the run parses the
   value, **Then** one quoted line names the key and the run exits 2.
5. **Given** `idle,method=ftq,work=1000`, **When** the run parses the
   value, **Then** one quoted line names the key and the run exits 2.
6. **Given** a bad value on an option file line, **When** the run
   parses the file, **Then** the line names the path, the line and the
   token, and the run exits 2.

---

### User Story 6 - The docs teach the two methods (Priority: P3)

A reader of the main page and of the harness page wants to know the
keys, the calibration disclosure, the series fields and the period
list. One runnable example shows a fixed work quanta run and a fixed
time quanta run.

**Why this priority**: the docs gates bind every feature, and the
reader's path is the last mile. The example also carries the shape
fold, which checks the printed block against the run's own output.

**Independent Test**: the example builds, runs, and its `.expected`
file folds against the binary's own run; the drift gate, the snippet
compile gate and the prose gate read no finding.

**Acceptance Scenarios**:

1. **Given** the main page, **When** the drift gate reads it,
   **Then** the new section stands at or below 25 lines, the page
   stands at or below 1004 lines, and every block matches its region.
2. **Given** the harness page, **When** a reader follows the jitter
   section, **Then** the section carries the keys, the calibration
   lines, the series fields and the period lines.
3. **Given** the example, **When** it runs, **Then** it prints the
   calibration line and the series block, and the shape fold passes.

---

### User Story 7 - The acceptance records stand per L3 group (Priority: P3)

The owner wants the local acceptance figures recorded on the lane
host: FWQ and FTQ on one idle cpu per L3 group, 10 s each, with and
without a load on a sibling cpu, and the periods the DFT found.

**Why this priority**: the figures are the evidence the pull-request
body carries, and the roadmap's acceptance axis is the two-CCD shape.
The suite gates nothing on them.

**Independent Test**: the results directory carries one record per
run, each naming the host, the kernel, `CONFIG_HZ`, the L3 group, the
commands, the exit codes, the calibration figures and the detected
periods.

**Acceptance Scenarios**:

1. **Given** the lane host, **When** the acceptance script runs,
   **Then** one record per method per L3 group stands recorded, with
   and without the sibling load.
2. **Given** a record, **When** a reader checks it, **Then** the
   kernel, `CONFIG_HZ`, the calibration count, the measured unit time
   and the detected periods stand named.
3. **Given** the suite, **When** it runs on the lane host and under
   `taskset -c 0,1`, **Then** every test passes and no test asserts a
   noise figure of the real host.

### Edge Cases

- A series with one sample: the statistics read that sample, the
  percentiles read it, and the period list stands empty. The scripted
  clock answers the calibration prefix, then the 1,024 pass stamps, then
  one unit stamp, then the tail stamp, so "one sample" counts the units
  recorded in the series. The clock answers more reads than that.
- A series at the cap of 65,536 points: the transform reads every
  point, and a longer series is read as its last 65,536 points with
  the count of skipped points disclosed.
- A constant series: the transform finds no period above three times
  the median bin, and the period list prints as empty.
- A quantum shorter than one calibrated unit: the count stands 0 for
  every quantum, the noise fraction stands 100, and the run discloses
  the calibration beside it.
- A calibration that finds no count near its target: the run keeps the
  closest count it measured, discloses the count and the unit time,
  and the window runs.
- A buffer that fills: each lost record counts, the count discloses,
  and the statistics read the kept records only.
- A `work=1000` run: the count reads 1000, no search runs, the
  calibration span reads 0, and the window runs at that count.
- A `work=` count with `method=ftq`, and a `quantum=` value with
  `method=fwq`: each refuses one line with exit status 2.
- A `noise=` value outside the range 0 to 100, a `noise=` value that is
  not a number, a `work=` value that is not a number, and a `work=`
  count above 16,777,216: each prints one quoted line and the run exits
  2.
- A key that stands twice in one value, for example
  `idle,method=fwq,work=1000,work=2000`: one quoted line names the key
  and the run exits 2.
- A scripted FTQ series of Q quanta at U units each carries the
  calibration prefix, then Q x U unit stamps, then the tail stamp, then
  the counts the closed quanta record.
- A `mode=precheck` run with `method=ftq`: the pre-check window runs
  the FTQ probe, and the standing pre-check lines keep their shape.
- A strict run with `method=fwq` in `mode=precheck` or `mode=both`: the
  refusal reads the FWQ noise fraction against the `noise=` threshold
  after the pre-check window. In `mode=alongside` no noise gate fires,
  the figures stand disclosed, and `strict` keeps its standing meaning.
  A `tsc` run keeps the standing gap rule.
- A host with one L3 group, and a host restricted to two cpus: the
  suite passes, and the acceptance records name the groups the host
  states.
- A probe cpu that stands busy: the figures grow, no test asserts a
  bound on them, and the acceptance record names the load.
- A resampled FWQ series: the step stands at the median unit time,
  each bin holds the mean unit time inside it, an empty bin takes the
  previous sample, and the step stands disclosed beside the period
  list.
- A window that ends inside a quantum: the open quantum records no
  count, and the statistics and the spectrum read the closed quanta.
- A pre-check window with `method=fwq` or `method=ftq`: the standing
  pre-check line shape stays, and the pair it carries stands the
  method's own: the noise fraction and the maximum unit time for FWQ,
  the noise fraction and the minimum count for FTQ. The pinning-
  candidate line ranks by the noise fraction ascending, ties in cpu
  order.
- A calibration whose target stands unreachable at the count bound:
  the run keeps the closest count it measured and discloses it.
## Requirements *(mandatory)*

### The surface, the grammar, the version and the schema (A)

- **FR-001** (D-1): The `--jitter-probe` value SHALL read one method
  through `method=tsc|fwq|ftq`, and one run SHALL carry one method.
  `tsc` SHALL stand as spec 027 defines it, and a value with no
  `method` key SHALL behave as it does today.
- **FR-002** (D-1): The value grammar SHALL carry `quantum=<us>` for
  FTQ, `work=<n>` for FWQ, and `noise=<percent>` for the strict
  refusal, beside the standing keys `threshold`, `gaps`, `mode`,
  `precheck` and `strict`. Each key SHALL stand once in one value, a
  second occurrence SHALL be an invalid value, `quantum` SHALL take
  the default 100 us and the range 10 to 100,000 us, `work` SHALL take
  the range 1 to 16,777,216, and `noise` SHALL take the default 1 and
  the range 0 to 100. Each of the three SHALL take a whole number, on
  the standing number grammar of the value. The value SHALL keep the
  standing head: the first
  token names a cpu list or the word `idle`, and every value this spec
  shows carries the head.
- **FR-003**: A bad value SHALL refuse one quoted line on standard
  error with exit status 2 before anything runs: a method word outside
  the three, a `quantum` value outside the range, a `quantum` value
  that is not a number, a `work` count of 0, a `work` count above
  16,777,216, a `work` value that is not a number, a `noise` value
  outside the range, a `noise` value that is not a number, a fractional
  `quantum`, `work` or `noise` value, a `quantum`
  key with `method=fwq`, a `work` key with `method=ftq`, and a repeated
  key each name their fault. A bad value
  from an option file line SHALL name the path, the line and the
  token, the standing file-fault shape.
- **FR-004** (D-9): The modes SHALL work for every method:
  `mode=alongside`, `mode=precheck` and `mode=both` SHALL run the
  selected method in the standing phases, over the standing cpu set,
  under the standing overlap refusal. The tracer of spec 036 SHALL run
  beside any method with no change.
- **FR-005** (D-8): The version SHALL stand at `0.29.0` with the
  harness `SOVERSION` at `20` and the archive `SOVERSION` at `5`, the
  values free above master at branch time. A rebase SHALL take the next
  free value of each and record the check. The commit that moves the
  numbers SHALL record the version check, the ABI check and the schema
  check beside them. The ABI check SHALL name
  the four layout moves: the new fields of `JitterProbeOptions`, the
  series record of `JitterEntryFold`, the series record of
  `PrecheckCpuRecord`, and the run record of `ReportData`.
- **FR-006** (D-7, D-8): The report schema minor SHALL stand at `1.12`
  above master at branch time, and a rebase SHALL take the next free
  minor. The root `jitter` object SHALL gain two keys after
  `loopPeriodNs`: `method`, a string in the closed set `TSC`, `FWQ`,
  `FTQ`, and `series`, an array with one closed item per probe cpu
  ascending. The item SHALL carry `cpu`, the calibration keys
  `workCount`, `unitTimeNs`, `targetNs`, `quantumNs` and
  `calibrationSpanNs`, the 12 series figures `sampleCount`, `minValue`,
  `medianValue`, `meanValue`, `p90Value`, `p99Value`, `p999Value`,
  `p9999Value`, `maxValue`, `noisePercent`, `seriesRuntimeNs` and
  `droppedSamples`, the spectrum keys `resampleStepNs` and
  `spectrumPoints`, and a nested closed `periods` array whose item
  carries `periodNs` and `powerRatio`. Each nested object SHALL stand
  closed with its own `x-order`, `required` and
  `"additionalProperties": false`, the shape the standing `stats` item
  carries. The standing `required` lists SHALL stay as they are, so a
  `FWQ` or `FTQ` run writes `mode`, `cpus` and `disclosure` as today,
  `thresholdNs` and `loopPeriodNs` as the standing default rule
  measured them, `droppedGaps` 0, and one `stats` item per probe cpu
  with the 16 gap fields at 0. The per-benchmark `jitter` key SHALL
  keep its open shape, so no schema edit joins there. The document
  root's `x-order` SHALL stay unchanged, and the `jitter` object's
  `x-order` SHALL gain the new names.

### The series statistics (B)

- **FR-007**: Each probe cpu SHALL report the statistics of its
  series: the sample count, the minimum, the median, the mean, `p90`,
  `p99`, `p99.9`, `p99.99`, the maximum, the runtime of the window,
  and the count of dropped samples.
- **FR-008**: The percentiles SHALL use the N1 nearest-rank code of
  spec 021 on the series in place, with the standing minimum sample
  count for each point. The code SHALL copy no buffer, and the
  statistics SHALL read the kept samples only.
- **FR-009**: The noise fraction SHALL stand per probe cpu, in the
  range 0 to 100. For FWQ it SHALL be the sum of the kept unit times,
  minus the sample count times the minimum unit time, divided by that
  sum, times 100. For FTQ it SHALL be the sample count times the
  maximum count, minus the sum of the counts, divided by the sample
  count times the maximum count, times 100. A maximum count of 0 SHALL
  answer 100.

### The calibration (C)

- **FR-010** (D-2, AC-c2): Before its window, on its own probe cpu,
  the run SHALL find the work count that puts one unit near its target:
  1 us for FWQ, and one thousandth of the quantum for FTQ. The search
  SHALL double the count from 1 in at most 25 trials, counts 1 through
  16,777,216, then narrow by bisection in at most 24 steps, each trial
  reading the probe's own time source twice around the body. The search
  SHALL end at the trial cap, at the count cap 16,777,216, or when the
  bisection interval holds one count, so a scripted clock that never
  passes the target stands bounded at 98 reads. A `work=` count the value names SHALL stand as the unit count, and
  the search SHALL not run for that cpu. The run SHALL keep the closest
  count it measured, and the calibration span SHALL stay out of the
  window's runtime.
- **FR-011** (D-2, AC-c2): The run SHALL disclose the calibration of
  each probe cpu before that cpu's window starts, on the console and in
  the JSON: the work count, the measured unit time, the target, the
  calibration span, and the quantum for FTQ. For a run whose count the value names, the calibration span reads 0,
  and the measured unit time reads the first unit of that cpu's window.
  The JSON SHALL carry them inside that cpu's `series` item (FR-006,
  FR-010).

### FWQ: the fixed work quanta series (D)

- **FR-012** (D-3): The probe SHALL repeat one fixed unit of work, the
  calibrated loop body closed on `sg::doNotOptimize`, and SHALL record
  the time of each unit in nanoseconds into the series buffer. One unit
  SHALL read the time source once, the unit time standing at the delta
  between two consecutive reads. The standing 1,024-read measurement
  pass SHALL run for every method, because it sets `loopPeriodNs` and
  the default threshold rule, so a scripted FWQ series carries the
  calibration prefix, then 1,024 pass stamps, then one stamp per unit,
  then one tail stamp.
- **FR-013**: The FWQ block SHALL report the series statistics of
  FR-007 in nanoseconds, the noise fraction of FR-009, and the unit
  time the calibration measured.
- **FR-014** (D-6): The series buffer SHALL stand pre-sized from the
  standing `gaps=<n>` key, 65,536 entries by default, in the range 1
  to 16,777,216. The measured loop SHALL allocate nothing, SHALL read
  no file, and SHALL take no lock. A full buffer SHALL count each lost
  record and the run SHALL disclose the count.

### FTQ: the fixed time quanta series (E)

- **FR-015** (D-4): The probe SHALL count the whole units finished in
  each fixed quantum, and SHALL record each count into the series
  buffer. One unit SHALL read the time source once, as FR-012 states,
  and the read SHALL compare against the next edge. Edge k SHALL stand
  at the window's first stamp plus k times the quantum, read from the
  time source. Only a closed quantum SHALL
  record a count, and a trailing open quantum SHALL stand dropped. The
  quantum SHALL stand at the `quantum=` value, 100 us by default.
- **FR-016** (D-4, D-6): The FTQ block SHALL report the count
  statistics of FR-007, the noise fraction of FR-009, the quantum and
  the unit time. The buffer rules of FR-014 SHALL bind the count
  series.

### The spectrum (F)

- **FR-017** (D-5): The run SHALL read a plain DFT over the series:
  the FTQ count series, or the FWQ time series resampled to a fixed
  step. The step SHALL stand at the median unit time of the kept FWQ
  series in nanoseconds, and the resample SHALL average each bin of
  one step of cumulative unit time, an empty bin taking the previous
  sample. The transform SHALL be a pure function over a sample span,
  SHALL allocate one span of N bins before its loop and nothing inside
  it, and SHALL add no library.
- **FR-018** (D-5, Q-4): The spectrum SHALL report at most five
  periods, each with its period in nanoseconds and its power relative
  to the strongest bin, and SHALL keep only a period whose power stands
  above three times the median of bins 1 through N/2, the DC bin out.
  The list SHALL stand sorted by power, strongest first, ties in period
  order.
  One bin SHALL mean one frequency bin of the transform, `1 / (N x Ts)`,
  so the tolerance of a planted period `P` stands at `P^2 / (N x Ts)`.
  The transform SHALL read at most 65,536 points, and a longer series
  SHALL be read as its last 65,536 points with the count of skipped
  points disclosed. The transform SHALL cost `N x N/2` complex multiply-adds, four real each,
  SHALL build each bin's sine and cosine by recurrence with no library
  call in the inner loop, and SHALL stay inside the standing 300 s test
  timeout at the cap.
- **FR-019** (D-2, D-5, D-7): The console SHALL print the new figures
  in the standing jitter block, in the standing field-per-line shape
  with one column per probe cpu in the standing cpu order, each field
  printing once: `method`, `workCount`, `targetNs` and
  `calibrationSpanNs`, then the 16 figures of FR-021, then `period1Ns`
  through `period5Ns` and `period1Power` through `period5Power`, 30 new
  lines. The console SHALL spell the method in lower case, `tsc`, `fwq`
  and `ftq`, the words the value takes, and the JSON SHALL spell it
  `TSC`, `FWQ` and `FTQ`. A rank a cpu does not carry SHALL print 0. The
  calibration lines SHALL print before the window starts, and the block
  SHALL keep the standing lines in place.

### The record and the rule names (G)

- **FR-020** (D-7): The JSON document SHALL carry, under the standing
  root `jitter` object, the method and one `series` item per probe cpu
  with the calibration, the series statistics and the period list, and
  `loadReport` SHALL read each key back. Each benchmark entry's
  standing `jitter` object SHALL gain the same `series` array, folded
  over that benchmark's timed windows on the carrier
  `BenchmarkResult::jitterFolds`, with no `method` key and no
  `periods` key, and `null` SHALL stand where the windows kept no
  sample. `JitterEntryFold` SHALL gain the series record and its
  period list, and `ReportData` SHALL gain one run record with the
  method word, the way spec 037 moved the harness `SOVERSION` when
  `ReportData` gained its graph member. The resample step and the point
  count SHALL ride each cpu's series item. A run with the default method SHALL write the
  standing keys only, so its document matches master apart from the
  `schemaVersion` value and the version figures the report carries.
- **FR-021** (D-9): The N5 rule-name grammar SHALL resolve the new
  field names: the bare `jitter:<cpu|any>:<field>` form SHALL read the
  per-benchmark series, and `jitter:run:<cpu|any>:<field>` SHALL read
  the run-level series. The table `kJitterFields` of
  `source/harness/compare.cpp:241-292` SHALL grow from 16 to 32
  entries, the 16 names `sampleCount`, `minValue`, `medianValue`,
  `meanValue`, `p90Value`, `p99Value`, `p999Value`, `p9999Value`,
  `maxValue`, `noisePercent`, `seriesRuntimeNs`, `droppedSamples`,
  `unitTimeNs`, `quantumNs`, `resampleStepNs` and `spectrumPoints`, and
  the fault text SHALL read `names none of the thirty-two jitter
  fields`. The `any` reading SHALL stay the uniform maximum over every
  field, so a noise rule over an FTQ run SHALL name `noisePercent` with
  `higher`, and the contract SHALL record that reading. The noise-based
  strict refusal SHALL fire in the pre-check phase, after the pre-check
  window ends and before the batch's first run, on a probe cpu whose
  noise fraction stands at or above the `noise=` threshold, with one
  line and exit status 2. In `mode=alongside` the method's window runs
  while the batch records, so a refusal there would land after the
  record: the `strict` word SHALL keep its standing meaning, the noise
  figures SHALL stand disclosed, and no noise gate SHALL fire. A `tsc`
  run SHALL keep the standing gap rule. A `noise=` value with no
  `strict`, and a `noise=` value with `method=tsc`, SHALL stand accepted
  and disclosed as unused, the way `threshold=` stands accepted and
  unused for FWQ and FTQ.
- **FR-022** (D-6): A dropped-sample count SHALL stand disclosed on the
  console and in the JSON for every method, and the statistics and the
  spectrum SHALL read the kept samples only.

### The tests and the seams (H)

- **FR-023** (IV.1): The main page SHALL carry one section of at most
  25 lines that teaches the two methods, their keys and the calibration
  disclosure, with its quick-reference rows. The page stands at 1004
  lines against the cap, so the lane SHALL shorten the quoted regions
  inside the three jitter sections it owns, `Jitter probe`, `Host noise
  pre-check` and `Noise tracer`: a marked region is the floor of a
  section, so a shorter region is the trim. The shared 17-line region
  SHALL split into complete top-level units, each quoted by the sections
  that name it. The lane SHALL keep the page at or below 1004 lines. No other
  section SHALL move, and the cap SHALL stay 1004.
- **FR-024**: No test SHALL sleep and no test SHALL read a clock: the
  series and the time edges come from the injected time source through
  `installTestReader` and the fake provider. No test SHALL name a
  fixed cpu number: the probe cpus SHALL come from
  `sched_getaffinity`, and the suite SHALL pass under `taskset -c 0,1`
  and on a 2-cpu host.
- **FR-025** (AC-c1): The suite SHALL script each series through the
  seam and check the figures: a clean series, one slow unit, a periodic
  dip at a known period, and a full buffer. The suite SHALL test the
  transform as a pure function on a pure tone, on two tones, on a
  constant series, on one sample, and on a series at the cap. The suite
  SHALL also cover the truncation arms: a 70,000-sample series, where
  the transform reads the last 65,536 points and discloses 4,464
  skipped points; a six-tone series, where the list carries five
  entries, strongest first, and the sixth period stands out; and the
  FWQ resample path, reached by a run with a dip every 1,000 units at a
  1 us unit time. One cap case SHALL stand in the suite, and the tone
  cases SHALL stay at or below 4,096 points. No test SHALL assert a
  noise figure of the real host.
- **FR-026**: Each refusal row of FR-003 SHALL stand covered by a test
  that checks the one line and exit status 2. A run with no `method` key
  SHALL stand covered by a byte-identity check against an inline fixture
  of master's bytes, the shape `test/source/harness_noise_report_test.cpp
  :456-462` carries. The check SHALL mask `schemaVersion`, the version
  figures, and the host-measured probe fields `loopPeriodNs`,
  `thresholdNs`, `runtimeNs`, `gapCount`, `gapsPerSecond`, `totalNs`,
  `totalPercent` and `droppedGaps`, plus the host-calibrated overhead
  floor the precedent masks, and each mask SHALL check that the span
  holds a number, the way `maskHostFloor` does. The same test SHALL
  check that a `tsc` run's standing console lines match the recorded
  bytes.

### The evidence and the docs (I)

- **FR-027** (D-10, G-20): The results directory SHALL carry the
  acceptance runs on the lane host before the lane closes: FWQ and FTQ
  on one idle cpu per L3 group, 10 s each, with and without a load on
  a sibling cpu. Each record SHALL name the host, the kernel,
  `CONFIG_HZ`, the L3 group, the commands, the exit codes, the
  calibration count, the measured unit time, the noise fraction, the
  `gaps=` value the run passed, the kept span in milliseconds, and the
  detected periods. The script SHALL pass `gaps=131072` for FTQ and
  `gaps=10485760` for FWQ, so a 10 s window keeps every sample. The
  record SHALL stand one markdown file per script run, with one section
  per run: two methods, two L3 groups and two load states, eight
  sections, each naming the command line, the exit status and the
  captured output. The window SHALL come from a named benchmark run
  with `--min-time=10` and `--repetitions=1`, and the load SHALL come
  from a busy loop pinned to the SMT sibling thread of the probe cpu,
  recorded with its command line and its cpu id. Each record SHALL name
  the measured span and SHALL show a drop count of 0. `CONFIG_HZ` SHALL
  come from `/proc/config.gz`, then `/boot/config-$(uname -r)`, and the
  record SHALL read `unknown` when both stand absent.
- **FR-028**: A script SHALL record the acceptance runs, and no CI gate
  SHALL stand on it. The feature's `quickstart.md` SHALL link every
  record, and the pull-request body SHALL record the local counts, the
  host and the kernel.
- **FR-029**: `docs/pages/harness.md` SHALL carry the keys, the
  calibration lines, the series fields and the period lines beside the
  standing jitter section. One runnable example under `example/docs/`
  SHALL carry a fixed work quanta run and a fixed time quanta run,
  with its `.expected` file, and the shape gate SHALL fold it against
  the binary's own run.

### The standing contracts (J)

- **FR-030**: Six standing contracts SHALL move, each named by path:
  `specs/027-jitter-probe/contracts/cli-options.md` gains the `method`,
  `quantum`, `work` and `noise` keys and the head rule;
  `specs/027-jitter-probe/contracts/public-api.md` gains the new option
  fields and the series record; `specs/027-jitter-probe/contracts/
  report-json.md` gains the new keys; `specs/027-jitter-probe/contracts/
  rule-names.md` gains the 16 names, the count word `thirty-two` and
  the `any` reading of a count figure;
  `specs/034-host-noise-precheck/contracts/console-report.md` gains the
  series and period lines and the pre-check pair for FWQ and FTQ;
  `specs/034-host-noise-precheck/contracts/report-shape.md` gains the
  fold rows the new lines need. No other standing rule SHALL move.
- **FR-031**: The standing `tsc` path, the pre-check window of spec
  034 and the tracer of spec 036 SHALL keep their behavior, their
  console lines and their JSON keys.

### The gates (K)

- **FR-032**: Coverage SHALL read 100% lines and 100% branches, and the
  branch allowance SHALL stay 21. No `LCOV_EXCL` marker, no
  `--omit-lines` entry, no `--ignore-errors` entry and no `--remove`
  entry SHALL join, and no code SHALL join only to reach a branch.
- **FR-033**: The change SHALL keep the standing gate shape: no lambda
  inside an `SG_*` macro, contract-only helpers marked
  `[[maybe_unused]]`, simdjson as the only JSON reader, the
  time-source gate clean, the drift gate clean, and the prose gate
  clean on every generated line.
- **FR-034**: The lane SHALL add no option row, no option value, no
  dependency, no CI change and no gate-configuration change. It SHALL
  move no cap, threshold, preset, job, exclusion or allowance. The
  registration rows a gate requires stand as required work: the
  section entry and the owner row of the drift gate, the ctest entry
  of each new test executable, and the fold rows and golden pairs of
  the shape gate.

## Key Entities *(include if feature involves data)*

- `JitterProbeOptions` (`include/speedgun-ng/benchmark.hpp:2248`): the
  public options struct; the method, the quantum, the work count and
  the noise threshold join it, and the join is the public-ABI change
  FR-005 records (D-1, D-8).
- The series record: the per-cpu record of one method's series, the
  JSON keys `sampleCount`, `minValue`, `medianValue`, `meanValue`,
  `p90Value`, `p99Value`, `p999Value`, `p9999Value`, `maxValue`,
  `noisePercent`, `seriesRuntimeNs`, `droppedSamples`, `unitTimeNs`,
  `quantumNs`, `workCount`, `targetNs`, `calibrationSpanNs`,
  `resampleStepNs` and `spectrumPoints`. It joins `JitterEntryFold`
  (`include/speedgun-ng/benchmark.hpp:645`) beside `JitterStats`
  (`include/speedgun-ng/benchmark.hpp:512`), so the run-level read and
  the per-benchmark read share one type (D-7, D-8, FR-020).
- The calibration record: the JSON keys `workCount`, `unitTimeNs`,
  `targetNs`, `quantumNs` and `calibrationSpanNs`, measured on the
  probe cpu before its window and disclosed beside that cpu's series
  (D-2, FR-011).
- The period record: the JSON keys `periodNs` and `powerRatio`, one
  entry per reported period, nested inside the cpu's series item
  (D-5).
- The run record: the method word, a new member of `ReportData`
  (`include/speedgun-ng/benchmark.hpp:984`) (FR-020, FR-005).
- The spectrum kernel: a pure function from a sample span to the
  period list, private to the harness
  (`source/harness/detail/spectrum.hpp`), with the cap of 65,536
  points (D-5, FR-018).
- The series buffer: one pre-sized span per probe cpu, sized from the
  standing `gaps=<n>` key, written inside the measured loop (D-6,
  FR-014).
- The `jitter` JSON object (`docs/schema/report-v1.json:45-155`): the
  standing closed object that gains the method, calibration, series
  and period keys (D-7, FR-006).
- The `--jitter-probe` value: the standing option value that gains the
  `method`, `quantum`, `work` and `noise` keys (D-1, FR-002).

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001** (AC-c1): With an injected time source, a scripted series
  of equal units yields a block whose minimum, median, mean and
  maximum stand equal and whose noise fraction stands 0, and a
  scripted series of 10,000 units with one unit at ten times the unit
  time yields a maximum that names that unit, a `p99.99` at the equal
  value, and a noise fraction of 0.09.
- **SC-002** (AC-c1): A scripted FTQ series at a 100 us quantum with a
  dip every 1 ms over 1,000 samples yields a period inside the range
  990,000 to 1,010,000 ns, and a constant series yields an empty list.
- **SC-003** (AC-c2): Every FWQ and FTQ run discloses the calibration
  count and the measured unit time before the window starts, on the
  console and in the JSON.
- **SC-004**: A run with no `method` key writes a document that
  matches master apart from the `schemaVersion` value and the version
  figures the report carries, and every standing console line stays
  byte-identical.
- **SC-005**: Every refusal row of FR-003 prints one line and exits
  with status 2, and no benchmark output stands ahead of the refusal.
- **SC-006**: The coverage gate reads 100.0% lines and 100.0% branches,
  so the branch shortfall stands at 0 rows, and the recorded allowance
  stays 21. The suite passes under `taskset -c 0,1`.
- **SC-007**: The docs gates stand green: the drift gate, the snippet
  compile gate, the shape fold and the prose gate read no finding, and
  the main page stands at or below 1004 lines.
- **SC-008**: The results directory carries the acceptance records on
  the lane host: two methods, one idle cpu per L3 group, with and
  without a sibling load, each record naming the kernel, `CONFIG_HZ`,
  the calibration figures and the detected periods, and the quickstart
  links every record.
- **SC-009**: The version stands at `0.29.0`, the harness `SOVERSION`
  at `20` and the archive `SOVERSION` at `5`, and the commit that moved
  them records the version check, the ABI check with the four layout
  moves named, and the schema check.

## Constraints

- Language and style: C++23, `.clang-format`, the naming rules N-1 to
  N-12, and the warning set of the presets (Principle V).
- Contracts: every public declaration carries `\pre`, `\post` and
  `\invariant`, and the implementation enforces them with the `SG_*`
  facility (Principle II).
- Time source: time comes from `sg::counters` only; no `<chrono>`, no
  `clock_gettime`, no `gettimeofday` and no `rdtsc` in bound code
  (Principle VII, `test/time_source_gate.sh`).
- Measured loop: no allocation, no I/O, no syscall and no lock inside
  the unit loop and the quantum loop (D-6, FR-014).
- Options: one options struct carries the settings; no bool parameter
  joins the public surface (the standing spec-level rule).
- JSON: simdjson stands the only reader, and the writer is the
  standing report writer (the standing spec-level rule).
- Tests: 100% line and branch coverage, the branch allowance at 21, no
  new exclusion, each test in the same commit as the code it tests
  (Principle VI).
- Prose: ASD-STE100 Simplified Technical English in every generated
  line, the prose gate clean, and the commit template of Principle XI
  in every commit: a title of 50 characters or fewer, a body wrapped at
  72 columns, and the footer block with `Refs: specs/043-fwq-ftq` and
  the approval line as its last lines. The constitution template puts
  the approval line first and names an issue keyword line. The lane
  follows the order every repository commit carries since spec 031,
  and records the departure here. The lane names no issue number.
- Names: no person is named anywhere in the repository, the copyright
  line of `LICENSE` standing as the one exception.
- History: rebase only, no merge commit, one push with
  `--force-with-lease` after the first push, and no merge by this lane.

## Assumptions

- The lane host states its L3 groups through the topology the harness
  already reads, so the acceptance script names one idle cpu per group
  with no new topology code.
- The timer tick period the acceptance looks for comes from the
  kernel's `CONFIG_HZ` value, 1000 on the lane host, so 1 ms stands the
  expected period; a host with another value records the period its
  own `CONFIG_HZ` states.
- One unit near 1 us keeps the FWQ loop period near the standing
  `tsc` probe's loop period, so the calibration cost stays small
  against a 10 s window.
- The DFT cost at the cap stands acceptable: 65,536 points at the
  direct sum run after the window, outside every measured loop
  (D-5).
- The FTQ quantum of 100 us with a unit near one thousandth of it
  keeps the count per quantum near 1000, so a dip shows as a clear
  count drop.
- The standing `gaps=<n>` range covers the acceptance runs at 10 s:
  FWQ at a 1 us unit makes 10,000,000 samples, 80 MB per probe cpu at
  8 bytes each, and FTQ at a 100 us quantum makes 100,000 samples,
  0.8 MB per probe cpu. The acceptance script passes a buffer for the
  whole window, and a default run of 65,536 entries keeps 65.5 ms of
  FWQ and 6.55 s of FTQ and discloses the rest as drops.

## Constitution Checklist

- Main-page impact check (IV.1): the feature changes what the root
  `README.md` teaches, so the page and its `example/` regions move in
  the same change (FR-023, FR-029).
- Contracts (II): the new public fields and the new record carry
  `\pre`, `\post` and `\invariant`, and the code enforces them.
- R-DCUT (III): the EARS requirements above, the design in `plan.md`
  with both views, the code, and the unit tests in the same change.
- Coverage (VI): 100% line and branch coverage, the allowance at 21,
  no exclusion (FR-032).
- Performance (VII): the calibration runs before the window, the
  transform runs after the window, and the measured loop keeps the
  standing no-allocation and no-I/O rules (FR-010, FR-014).
- Gates (VIII): every hard gate runs on the lane, and the local
  acceptance carries the noise figures (FR-032, FR-033, FR-034).
- Prose (XI): every generated line clears the prose gate, and each
  commit follows the template.
