# Research: Fixed Work Quanta and Fixed Time Quanta Probes

Phase 0 output of `/speckit.plan` for `specs/043-fwq-ftq/spec.md`. Each
entry names the reading it fixes, the code fact that decides it, and the
alternative that loses. Every cite reads the working tree at the audit
point `d454bcff` (2026-11-01), the base of branch `043-fwq-ftq`. The
spec cites read the same tree. The lane host is the AMD Ryzen 9 7950X3D
of the spec's Lane identity, 16 cores on 32 threads, two L3 groups,
kernel `7.3.1-1-cachyos` with `CONFIG_HZ=1000`.

## R-1: how many time-source reads one FWQ unit takes

- **Decision**: One read per unit. The unit time stands at the delta
  between two consecutive reads. A scripted FWQ series carries the
  calibration prefix, then 1,024 pass stamps, then one stamp per unit,
  then one tail stamp. A 1,000-unit series reads 2,025 stamps behind the
  prefix, and the first unit time answers the delta between read 1,025
  and read 1,026 (FR-012).
- **Rationale**: The standing gap loop reads the stamp once per pass and
  takes the delta from the stamp one read behind
  (`source/harness/jitter_probe.cpp:321-328`). The measurement pass
  takes a fixed 1,024 reads (`kPassReads` at
  `source/harness/jitter_probe.cpp:50`, loop `:245-252`) and writes
  `loopPeriodNs` from its least delta (`:253`). The default threshold
  rule rises from that figure (`:254-257`), so the pass carries a value
  for every method.
- **Alternatives considered**: Two reads per unit, one before the body
  and one after. It doubles every scripted series, and it doubles the
  read count inside the measured body, whose standing rule names one
  reader call per pass (`:313-317`). Dropping the pass loses too:
  `loopPeriodNs` then carries no value in the standing block, and the
  scripted prefix length becomes method-dependent.

## R-2: what bounds the calibration search

- **Decision**: The doubling phase starts at count 1 and runs at most 25
  trials. The bisection runs at most 24 steps. Each trial reads the time
  source twice around the body, so the search stands at 98 reads at the
  cap. The search ends at the trial cap, at the count cap 16,777,216, or
  when the bisection interval holds one count, and the run keeps the
  closest count it measured (FR-010).
- **Rationale**: A scripted leaf steps its tail by one fixed delta once
  its point list stands exhausted (`include/speedgun-ng/counters_fake.hpp:36-41`).
  A trial of n units measures delta divided by n, so the measured unit
  time falls as the count doubles and a 1 us target never passes. The
  count cap stands the standing buffer bound (`kMaxGapCount` at
  `include/speedgun-ng/benchmark.hpp:2194`). FR-010 fixes the trial and
  read counts (`specs/043-fwq-ftq/spec.md:897`).
- **Alternatives considered**: A search that stops only when the target
  passes. The probe thread then spins to the entry's 300 s ceiling
  (`test/CMakeLists.txt:2126`), and the scripted prefix has no length a
  test can align a dip behind. A fixed count table loses too: the
  disclosed count then answers a table, so the rule that keeps the
  closest measured count has no measured set to read.

## R-3: what stands in each noise-fraction denominator

- **Decision**: FWQ divides the sum of the kept unit times, minus the
  sample count times the minimum unit time, by that sum, times 100. FTQ
  divides the sample count times the maximum count, minus the sum of the
  counts, by the same product, times 100. A maximum count of 0 answers
  100. Both figures stand in the range 0 to 100 percent (FR-009).
- **Rationale**: The strict rule compares the figure against `noise=`, a
  key whose range stands 0 to 100 percent (`specs/043-fwq-ftq/spec.md:816`),
  so the figure must land inside it. The scripted series then answer
  0.09 percent for 10,000 units with one unit at ten times the unit time
  (SC-001, `:1172`), and 0.05 percent for 1,000 quanta with one quantum
  at half the best count.
- **Alternatives considered**: The standing `totalPercent` denominator,
  the covered span (`source/harness/jitter_probe.cpp:575-576`). The sum
  of the unit deltas differs from the kept records' stamp span
  (`:560-571`), so a scripted series states no exact figure. Dividing
  the FTQ figure by the maximum count alone loses too: the figure
  reaches 100 times the sample count, leaves the range, and refuses
  every strict run.

## R-4: where the quantum edges stand and where the open quantum goes

- **Decision**: Edge k stands at the window's first stamp plus k times
  the quantum, read from the time source. Only a closed quantum records
  a count, and the trailing open quantum stands dropped. The quantum
  stands at the `quantum=` value, 100 us by default, in the range 10 to
  100,000 us (FR-015, FR-002).
- **Rationale**: The window's first stamp is the read behind the latch
  line (`source/harness/jitter_probe.cpp:306-311`). The standing span
  test compares a stamp against a start plus a fixed span (`:279`,
  `:287`), the shape one absolute edge list repeats.
- **Alternatives considered**: Recording the trailing partial quantum.
  The last quantum of every clean run then stands at the minimum, so
  User Story 2 scenario 1 fails with its equal minimum, median and
  maximum. Re-anchoring each quantum at the last unit's finish loses
  too: the sample step then carries the count's own drift, and a planted
  1 ms dip lands off-bin.

## R-5: what step the FWQ resample takes and how a sample lands in a bin

- **Decision**: The step stands at the median unit time of the kept FWQ
  series, in nanoseconds, disclosed beside the period list. Bin k covers
  one step of cumulative unit time and holds the mean unit time inside
  it, and an empty bin takes the previous sample (FR-017).
- **Rationale**: The median of the kept set is the standing 50.0 point
  of the nearest-rank code (`source/harness/jitter_probe.cpp:550`,
  `source/harness/percentiles.cpp:104-126`), the figure the series fold
  already answers. With a 1 us step the 65,536-point cap reads 65.5 ms,
  so a 1 ms period lands at bin 65, that is 1.008 ms, within one bin.
- **Alternatives considered**: The calibration's measured unit time as
  the step. The calibration runs ahead of the window, and the window
  opens behind the latch line (`source/harness/jitter_probe.cpp:306-311`),
  so a clock change during the window scales the whole axis and moves
  the planted period off-bin. Nearest-sample resampling loses too: one
  slow unit lands in one bin, and the three-times-median-bin test
  reports a spike with no period.

## R-6: which DFT form, its cost, and its recurrence

- **Decision**: The direct sum over N samples and N/2 bins, costing N
  times N/2 complex multiply-adds, four real each. Each bin's sine and
  cosine stand built by recurrence, and no library call stands in the
  inner loop. The transform reads at most 65,536 points, allocates one
  span of N bins before its loop and nothing inside it, and runs after
  the window (FR-017, FR-018).
- **Rationale**: Measured on the lane host on 2026-11-01: 3.3 s at
  `-O2` and 14.6 s at `-O0` with `--coverage` for one cap call, against
  the 300 s ceiling every entry with no own value takes
  (`test/CMakeLists.txt:2126`). The same call with a libm `cos` and
  `sin` per sample reads 47.8 s at `-O0` with `--coverage`
  (`specs/043-fwq-ftq/spec.md:399-405`).
- **Alternatives considered**: A libm call per sample in the inner loop.
  The sanitizer builds run two to three times over 47.8 s and crowd the
  entry's whole budget. A windowed transform, a filter, a chirp
  transform and an autocorrelation lose on the standing rule that the
  lane adds no library (Out of scope, D-5).

## R-7: how the top-five list and the median-bin filter read

- **Decision**: At most five periods, strongest first, each with its
  period in nanoseconds and its power relative to the strongest bin. A
  period stands kept only above three times the median of bins 1
  through N/2, the DC bin out. One bin means one frequency bin,
  `1 / (N x Ts)`, so the tolerance of a planted period P stands at P
  squared divided by N times the step. That is 10 us at a 100 us
  quantum, 1,000 samples and a 1 ms period, and the test asserts the
  range 990,000 to 1,010,000 ns (FR-018, SC-002).
- **Rationale**: The DC bin of a series with a non-zero mean carries the
  largest power, so a median taken over bin 0 as well rises with the
  series offset. The median of bins 1 through N/2 of a constant series
  answers 0, the threshold answers 0, and the list stands empty, the
  stated edge case.
- **Alternatives considered**: The median over bins 0 through N/2. The
  filter's threshold moves with the mean of the series. A list of five
  with no filter loses too: a constant series then prints five periods.

## R-8: how large the series buffer stands and what a full buffer does

- **Decision**: One pre-sized buffer per probe cpu, sized from the
  standing `gaps=<n>` key: 65,536 entries by default, in the range 1 to
  16,777,216, one 8-byte entry each. A full buffer counts each lost
  record, discloses the count, and the window keeps running. The
  statistics and the spectrum read the kept samples only (FR-014,
  FR-022).
- **Rationale**: The standing drop rule keeps a record below
  `buffer.size()` and moves the count, the total and the maximum of the
  lost records to the per-cpu counters at the bound (`keepOrDrop` at
  `source/harness/jitter_probe.cpp:106-120`). The buffer allocates and
  first touches on the probe cpu after the pin and before the first read
  (`:205-209`). The range stands at `:856-865` and the default at
  `include/speedgun-ng/benchmark.hpp:2192`.
- **Alternatives considered**: Growing the buffer inside the loop. The
  measured body allocates nothing, reads no file and takes no lock
  (`:313-317`). Raising the default loses too: 1,048,576 entries cost
  8 MB per probe cpu in every tsc run as well, against FR-031.

## R-9: how each benchmark entry gets its series

- **Decision**: The per-benchmark series folds over that benchmark's
  timed windows on the standing carrier `BenchmarkResult::jitterFolds`
  (`include/speedgun-ng/benchmark.hpp:733`). The entry's `jitter`
  object gains the same `series` array, with no `method` key and no
  `periods` key, and `null` stands where the windows kept no sample. No
  schema edit joins there (FR-020).
- **Rationale**: The gap fold already reads the one buffer over the
  windows' half-open intervals (`source/harness/jitter_probe.cpp:636-640`)
  and pushes one fold per cpu after the join
  (`source/harness/cli.cpp:1819-1825`). The per-entry key stands open in
  the schema, typed `["object", "null"]` with no key list
  (`docs/schema/report-v1.json:479-481`). A bare rule name reads the
  entry's folds (`source/harness/compare.cpp:1465`).
- **Alternatives considered**: A second buffer per cpu per benchmark. It
  costs 512 KB each at 65,536 entries of 8 bytes, and the measured loop
  allocates nothing. A run-level record alone loses too: the bare name
  `jitter:any:noisePercent` of User Story 4 scenario 4 then reads
  nothing, and a rule over a report with no value for it is a status-2
  fault (`source/harness/compare.cpp:1479-1483`).

## R-10: where the new keys sit in the closed jitter object

- **Decision**: Two root keys after `loopPeriodNs`: `method`, a string
  in the closed set `TSC`, `FWQ`, `FTQ`, and `series`, an array with one
  closed item per probe cpu ascending. The item carries `cpu`, the five
  calibration keys `workCount`, `unitTimeNs`, `targetNs`, `quantumNs`
  and `calibrationSpanNs`, the 12 series figures, the two spectrum keys
  `resampleStepNs` and `spectrumPoints`, and a nested closed `periods`
  array of `periodNs` and `powerRatio`, 21 keys in all. The standing
  `required` lists stay, so a FWQ run writes the 16 gap fields at 0
  (FR-006).
- **Rationale**: The root `jitter` object stands closed
  (`docs/schema/report-v1.json:65`), the checker refuses a key a closed
  object does not list (`source/harness/json_load.cpp:2538-2542`) and
  refuses a key out of the stated order (`:2545-2557`), so the writer's
  key order (`source/harness/json_report.cpp:1010-1035`) and the schema
  `x-order` (`docs/schema/report-v1.json:47-55`) move together. Each
  nested object copies the closed shape of the standing `stats` item
  (`docs/schema/report-v1.json:117`).
- **Alternatives considered**: Relaxing the `required` list to four
  keys. The standing reader reads the seven keys in order
  (`source/harness/json_load.cpp:1249-1314`), and a second document
  shape stands in front of the byte-identity check of SC-004.
  Root-level calibration and periods lose too: the search runs on the
  cpu that runs the loop, and the transform reads one cpu's series.

## R-11: how far the rule-name table grows and what the fault text says

- **Decision**: `kJitterFields` grows from 16 entries to 32, adding
  `sampleCount`, `minValue`, `medianValue`, `meanValue`, `p90Value`,
  `p99Value`, `p999Value`, `p9999Value`, `maxValue`, `noisePercent`,
  `seriesRuntimeNs`, `droppedSamples`, `unitTimeNs`, `quantumNs`,
  `resampleStepNs` and `spectrumPoints`. The fault text reads `names
  none of the thirty-two jitter fields`. `runtimeNs` keeps the gap
  figure and the series figure takes `seriesRuntimeNs`, so one name
  answers one entry (FR-021).
- **Rationale**: The table stands as `constexpr std::array<JitterField, 16>`
  (`source/harness/compare.cpp:251`) and the fault text names the count
  (`:366-369`), so a grown table prints a false count. The reader takes
  `const JitterStats&` (`:244`) and the value read passes `fold.stats`
  (`:1477`, `:1489`), so the 16 new names need the fold, the carrier the
  series record joins.
- **Alternatives considered**: A second table for the new names. The
  parse and the value read share one table today (`:362-370`), and two
  tables is the divergence FR-033 exists to prevent. A method-aware
  `any` loses too: the per-benchmark path carries no method word, and
  the uniform maximum is the standing rule (`:1485-1491`).

## R-12: what the byte-identity test compares and which spans it masks

- **Decision**: An inline fixture of master's bytes, compared after
  masks over `schemaVersion`, the version figures, the host-measured
  probe fields `loopPeriodNs`, `thresholdNs`, `runtimeNs`, `gapCount`,
  `gapsPerSecond`, `totalNs`, `totalPercent`, `droppedGaps`, and the
  host-calibrated overhead floor. Each mask checks that the span it
  replaces holds a number. The same test compares the standing console
  lines byte for byte (FR-026, SC-004).
- **Rationale**: The precedent carries master's bytes inline
  (`test/source/harness_noise_report_test.cpp:456-461`) and masks one
  span (`maskSpeedgun`, `:440-451`). `maskHostFloor`
  (`test/source/harness_pass_report_test.cpp:267-281`) checks that both
  bounding marks stand present and that the hidden span holds a `.`
  (`:272-279`), so a mask cannot pass over a missing field.
- **Alternatives considered**: Two runs of the same build compared to
  each other. It proves determinism and answers nothing about master's
  bytes. A key-sequence comparison loses too: SC-004 states byte
  identity of every standing console line, and a key-sequence check
  reads no value byte.

## R-13: what shape the acceptance record carries and what drives the window and the load

- **Decision**: One markdown file per script run, one section per run,
  eight sections: two methods, two L3 groups, two load states. Each
  section names the command line, the exit status and the captured
  output. The window comes from a named benchmark run with
  `--min-time=10` and `--repetitions=1`. The load comes from a busy
  loop pinned to the SMT sibling thread of the probe cpu, recorded with
  its command line and its cpu id. `CONFIG_HZ` comes from
  `/proc/config.gz`, then `/boot/config-$(uname -r)`, and the record
  reads `unknown` when both stand absent. The script passes
  `gaps=131072` for FTQ and `gaps=10485760` for FWQ, and each record
  names the kept span in milliseconds (FR-027).
- **Rationale**: The standing script appends the command line, the exit
  status and the captured output of each run to one dated record
  (`tools/noise/jitter_probe_acceptance.sh:35-52`, record path `:29`,
  results root `:25`), and it is a script that CI never invokes
  (`:4-5`). No script in the tree reads `CONFIG_HZ` today. The two
  options stand in the standing table (`source/harness/options.cpp:504`
  and `:510`).
- **Alternatives considered**: One file per run, eight files. The
  standing results directory carries one dated record per script run,
  and the quickstart link rule of SC-008 then names eight paths per
  host. A load on a core of the same L3 group loses too: the host runs
  16 cores on 32 threads, so sibling names the SMT thread, and a
  same-group load is an axis the spec states nowhere.

## R-14: which transform arms the coverage plan needs

- **Decision**: Three rows join the FR-025 case list: a 70,000-sample
  series, where the transform reads the last 65,536 points and
  discloses 4,464 skipped points; a six-tone series, where the list
  carries five entries, strongest first, and the sixth period stands
  out; and the FWQ resample path, reached by a run with a dip every
  1,000 units at a 1 us unit time. One cap case stands in the suite,
  and the tone cases stay at or below 4,096 points (FR-025).
- **Rationale**: The gate reads 100.0% lines
  (`tools/dbc/coverage_gate.sh:45-48`) and passes a branch shortfall at
  or below `allowance=21` (`:102`, `:114-118`). No `LCOV_EXCL` marker
  stands in `source/harness/`, so every arm of the transform needs a
  named case. The transform is a pure function over a sample span, so
  one call reaches each arm.
- **Alternatives considered**: Reaching the arms through probe-run
  scenarios alone. Each run costs a scripted series of the full series
  length through the seam, and the pure function answers the same arm
  in one call. Leaving the arms to the acceptance records loses too:
  the acceptance gates nothing (FR-028), and the coverage gate reads the
  suite's trace.

## Open readings

`_notes/plan-research.md` records two open flags, and the calibration
bound stands stated two ways across the notes. Each reading below closes
one flag and fixes the shared spelling.

- The acceptance window runs one repetition: `--min-time=10` with
  `--repetitions=1`, the value FR-027 states
  (`specs/043-fwq-ftq/spec.md:1072`) and the round-2 clarification
  repeats (`:419`). Each record shows one 10 s window per run, and each
  record shows a drop count of 0.
- The console gains 30 new lines, the count FR-019 states
  (`specs/043-fwq-ftq/spec.md:972`): four calibration lines, the 16
  figures of FR-021, and ten period lines. The example's `.expected`
  file records the list for the shape fold (FR-029).
- The calibration runs at most 25 doubling trials and 24 bisection
  steps, two reads per trial, so at most 98 reads (FR-010,
  `specs/043-fwq-ftq/spec.md:897`). R-2 takes these numbers, and the
  count cap stands 16,777,216.
- The closed `series` item carries 21 keys, the list FR-006 states
  (`specs/043-fwq-ftq/spec.md:853`).
- The calibration record carries 5 keys, the list FR-011 states
  (`:904`) and the Key Entities row repeats (`:1146-1149`).
- One series buffer element stays 8 bytes: one `std::uint64_t` per
  sample, sized from `gaps=<n>` (FR-014, E-26 of data-model.md).
- `noise=` follows the standing integer grammar of `parseProbeNumber`
  (`source/harness/jitter_probe.cpp:698-719`), so `noise=0.5` refuses
  one line with exit status 2 and the public field stands
  `std::uint32_t` (FR-002, FR-003).
