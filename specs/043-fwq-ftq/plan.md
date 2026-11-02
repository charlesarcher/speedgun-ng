# Implementation Plan: Fixed Work Quanta and Fixed Time Quanta Probes

**Branch**: `043-fwq-ftq` | **Date**: 2026-11-01 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `specs/043-fwq-ftq/spec.md`

## Summary

The standing jitter probe gains two methods. `method=fwq` repeats one
calibrated unit of work and records the time of each unit. `method=ftq`
counts the whole units finished in each fixed quantum, 100 us by default.
One run carries one method, and `method=tsc` stands as spec 027 defines
it. Each probe cpu calibrates its own work count on its own cpu before
its own window, because the lane host has two L3 groups at different
clocks. The run discloses the count, the measured unit time, the target
and the span before the window starts. Each probe cpu reports 16 series
figures from one pre-sized buffer under the standing gap-buffer rules. A
plain DFT over the series names the five strongest periods, and the
transform reads at most 65,536 points. The console gains 30 lines, one
per field, one column per probe cpu. The JSON gains `method` and `series`
under the standing root `jitter` object, and `loadReport` reads both
back. The version moves to `0.29.0` (`CMakeLists.txt:45`), the harness
`SOVERSION` moves from `19` to `20` (`CMakeLists.txt:375`), the archive
`SOVERSION` stays `5` (`CMakeLists.txt:110`), and the report schema moves
from `1.11` to `1.12` (`source/harness/json_report.cpp:43`).

## Technical Context

**Language/Version**: C++23 (`cxx_std_23` on every target; the
constitution pins the language)

**Primary Dependencies**: the standing probe unit and its seams: the test
reader seam (`installTestReader`, `source/harness/jitter_probe.cpp:690`),
the measurement pass and the loop period (`:243`, `loopPeriodNs` at
`:253`), the nearest-rank percentile code (`percentileAt`,
`source/harness/percentiles.cpp:104`), the fold carrier
(`BenchmarkResult::jitterFolds`, `include/speedgun-ng/benchmark.hpp:733`),
the console printers (`printJitterReport`, `source/harness/report.cpp:726`;
`printPrecheckReport`, `:1172`), the report writer
(`source/harness/json_report.cpp`), the reader and the schema checker
(`readJitterCpu`, `source/harness/json_load.cpp:460`; `readJitterRecord`,
`:1243`; `checkReport`, `:2613`), the rule-name table (`kJitterFields`,
`source/harness/compare.cpp:251`), and the schema
(`docs/schema/report-v1.json`). The transform uses the standard library,
so no dependency joins (spec D-5).

**Storage**: N/A: no file and no database of the feature's own. One
pre-sized series buffer per probe cpu holds the samples, sized from the
standing `gaps=<n>` key. The one document the feature writes is the
report file `--json` names.

**Testing**: the repository's own harness (one executable per area plus
`add_test`): three new executables `harness_fwq_ftq_test`,
`harness_spectrum_test` and `harness_jitter_bytes_test`; extended
`harness_jitter_window_test`, `harness_precheck_test`,
`harness_jitter_cli_test`, `harness_json_load_test`,
`harness_json_schema_test`, `harness_jitter_probe_alloc_test` and
`harness_cli_table_test`. Each series stands scripted through
`installTestReader` and the fake provider, so no test sleeps and no test
reads a clock. The probe cpus come from `sched_getaffinity`. The suite shall pass
under `taskset -c 0,1` and on a 2-cpu host (spec FR-024).

**Target Platform**: Linux only (`ci-ubuntu`, `ci-rocky`). The daily loop
drives `ci-rocky`; once per feature, `ci-ubuntu` builds with clang-tidy
and cppcheck wired in. The acceptance axis stands on the lane host: an
AMD Ryzen 9 7950X3D, 16 cores, 32 threads, one socket, two L3 groups,
kernel `7.3.1-1-cachyos` with `CONFIG_HZ=1000`, so the 1 ms tick gives the
spectrum a known period (spec D-10). No acceptance is gated on runner CI.

**Project Type**: library feature: the public surface grows inside the
standing types, one detail header and one source file join, and the
option table gains no row. No new target, no new library.

**Performance Goals**: one time-source read per unit, the unit time
standing at the delta between two consecutive reads. The unit loop and the
quantum loop allocate nothing, read no file, take no lock and read no
clock outside `sg::counters`. The calibration runs before the window, at
most 98 reads per probe cpu. The transform runs after the window: 65,536
points cost 2,147,483,648 complex multiply-adds, measured on the lane host
on 2026-11-01 at 3.3 s with `-O2` and 14.6 s with `-O0` and `--coverage`.
No benchmark baseline is claimed, and the tree names no baseline
mechanism, so none is promised.

**Constraints**: every time value flows through `sg::counters`, so the
time-source gate stays green; 100% line and branch coverage with the
branch allowance at 21 (`tools/dbc/coverage_gate.sh:102`) and no new
exclusion; no sleep, no clock read and no fixed cpu number in a test; one
quoted line and exit status 2 for every bad value; no lambda inside an
`SG_*` macro; `[[maybe_unused]]` on contract-only helpers; simdjson stands
the only JSON reader; the docs gates bind the README section, the guide
page and the example; the prose gate binds every generated line; the lane
adds no option row, no option value, no dependency and no CI change, and
moves no cap, threshold, preset, job, exclusion or allowance; the
registration rows a gate requires stand as required work (spec FR-033,
FR-034).

**Scale/Scope**: one new detail header (`source/harness/detail/spectrum.hpp`)
and one new source (`source/harness/spectrum.cpp`); the public types grow
inside `include/speedgun-ng/benchmark.hpp`; the probe body, the parser
arms and the two folds grow in `source/harness/jitter_probe.cpp`; the
entry point, the console printer, the writer, the reader and the rule
table extend; the schema, the two docs pages, one example, the drift-gate
lists, three new test executables, seven extended test files, the
acceptance script and the six standing contracts move.

**Execution mode**: TDD (Principle III). Every test stands written and
observed failing before the code that turns it green exists; the Test Plan
records the waves, the files each wave touches and the gate that closes
it.

## Constitution Check

*GATE: Must pass before Phase 0 research.*

| Principle | Gate | Status |
| --- | --- | --- |
| I. Standard-first | C++23 surface. The transform uses `std::span`, `std::vector` and the standard math header for one bin's sine and cosine; no compiler extension and no new dependency joins | PASS |
| II. Design by contract | The new enum, the four option fields, the series record, the period record and the run record carry doxygen `\pre`, `\post` and `\invariant`; the two kernel functions and the two folds carry them; the parser enforces the three ranges at the command-line boundary (FR-003); the loader carries `SG_REQUIRE` and `SG_ENSURE` at its seams | PASS |
| III. R-DCUT | FR-001 to FR-034 map to the decisions R-01 to R-24 below and to `tasks.md` (landing with `/speckit.tasks`). Both views stand below, and the Test Plan records TDD | PASS |
| IV. Documentation | The README section `Fixed work and time quanta` runs 18 lines inside the 25-line cap, with four quick-reference rows (IV.1, spec FR-023); its marked block quotes the `fwq-run` region of `example/docs/fwq_ftq.cpp`; the guide page carries the keys, the 30 series lines and the recorded run; the example and its `.expected` fold (spec FR-029) | PASS |
| V. Style | clang-format 18.1.8, clang-tidy and cppcheck clean at the feature head; every new name follows V.1 and the naming rules N-1 to N-12 (R-20) | PASS |
| VI. Coverage | 100% line and branch on the new paths; the 172 new branch positions each stand reached by a named test, so the branch shortfall stands 0 rows; the allowance stays 21 (`tools/dbc/coverage_gate.sh:102`); no exclusion marker joins (spec FR-032) | PASS |
| VII. Performance | The calibration runs before the window and the transform runs after it; the unit loop, the quantum loop and the transform kernel stand the designated critical paths; the loops keep the standing no-allocation, no-I/O, no-syscall and no-lock rules, and the kernel keeps its per-bin recurrence with no library call in the inner loop; time comes from `sg::counters` alone | PASS |
| VIII. CI gates | Every hard gate runs at the feature head: format-check, spell-check, tidy, cppcheck, sanitizers, tsan, coverage, name-check, prose, time-source, DBC, the schema shape check over every written document, and the docs gates; the Gate commands table names each command | PASS |
| IX. Spec-driven | Spec, plan, research, data model, contracts, quickstart and results stand in `specs/043-fwq-ftq/`; `tasks.md` lands with `/speckit.tasks` before code | PASS |
| X. Anti-slop | The methods ride the standing probe; the fold reads the one series buffer; the transform is a pure function with no library; the public surface grows by one enum, four fields, two records and one run record; no second buffer, no second seam and no second framework join (spec Out of scope) | PASS |
| XI. Prose | The gate walks `git ls-files`, so at this point it clears the tracked tree only; these artifacts stand untracked, and the branch's first commit, the one that lands `specs/043-fwq-ftq/`, closes the row, the way specs/025 and specs/027 recorded | PASS for the tracked tree, pending for these artifacts |

**Gate weakening requested**: no. **Gate strengthening requested**: no.
No budget figure moves: the README section lands at 18 lines inside the
standing 25-line section cap (`tools/docs/example_drift.py:241`), the page
lands at or below `PAGE_LINE_CAP = 1004` once the
two sections quote their own regions
(`tools/docs/example_drift.py:240`), the schema minor moves to `1.12` on
the standing key-growth rule, and the coverage branch allowance stays 21.

No gate fails, so **Complexity Tracking stays empty**.

## Project Structure

### Documentation (this feature)

```text
specs/043-fwq-ftq/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output (/speckit.plan command)
├── data-model.md        # Phase 1 output (/speckit.plan command)
├── quickstart.md        # Phase 1 output (/speckit.plan command)
├── contracts/           # Phase 1 output (/speckit.plan command)
│   ├── cli-options.md   # The four keys, the head rule, the error surface
│   ├── public-api.md    # The option fields, the series record, the folds
│   ├── report-json.md   # The method key, the series item, the periods item
│   ├── rule-names.md    # The 16 names, the count word, the any reading
│   ├── console-report.md# The 30 lines, the calibration lines, the pair
│   └── report-shape.md  # The fold rules the 30 lines need
├── results/             # The acceptance records of spec FR-027; the
│                        #   tasks phase fills them and the quickstart
│                        #   links them
├── _notes/              # The design, test-plan and docs notes this plan
│                        #   reads, with the clarify record
└── tasks.md             # Phase 2 output (/speckit.tasks - NOT created here)
```

The six standing contracts the spec names stand amended by the deltas
above, each in the commit that lands the code it records (spec FR-030):
the keys and the head rule join
`specs/027-jitter-probe/contracts/cli-options.md`; the option fields and
the series record join `specs/027-jitter-probe/contracts/public-api.md`;
the new keys join `specs/027-jitter-probe/contracts/report-json.md`; the
16 names, the count word `thirty-two` and the `any` reading join
`specs/027-jitter-probe/contracts/rule-names.md`; the series lines and
the pre-check pair join
`specs/034-host-noise-precheck/contracts/console-report.md`; the fold rows
join `specs/034-host-noise-precheck/contracts/report-shape.md`. This
directory carries the deltas, so the standing files stand untouched until
that commit.

### Source (this feature)

```text
include/speedgun-ng/benchmark.hpp         # + JitterMethod, the bounds, the
                                          #   four option fields, JitterPeriodRecord,
                                          #   JitterSeriesStats, JitterRunRecord,
                                          #   JitterEntryFold::series,
                                          #   PrecheckCpuRecord::series,
                                          #   ReportData::jitterRun
CMakeLists.txt                            # + version 0.29.0; harness SOVERSION 20;
                                          #   archive SOVERSION stays 5
source/harness/detail/jitter_probe.hpp    # + SeriesRecord, the series fields of
                                          #   ProbeCpuState, the two seriesStatsOf
source/harness/detail/spectrum.hpp        # new: resampleUnitTimes, dftPeriods,
                                          #   the cap, the top count, the factor
source/harness/spectrum.cpp               # new: the kernel definitions
source/harness/jitter_probe.cpp           # + keepSeriesOrDrop, calibrateUnit, the
                                          #   method arms, the two folds, the four
                                          #   key arms, the cross-method check
source/harness/cli.cpp                    # + the calibration disclosure, the noise
                                          #   refusal, the two folds, the record build
source/harness/report.cpp                 # + the 30 series lines, the method pair
                                          #   in the warning line and the ranking
source/harness/json_report.cpp            # + the series writer, the two root keys,
                                          #   the per-entry array; kSchemaVersion 1.12
source/harness/detail/json_report.hpp     # + kJitterMethodWords, the series field,
                                          #   the method field, the postconditions
source/harness/json_load.cpp              # + readJitterSeriesCpu, the two keys,
                                          #   the run record, the entry series
source/harness/compare.cpp                # + the 16 names, the count word, the
                                          #   fold argument at two call sites
docs/schema/report-v1.json                # + method and series in the jitter
                                          #   x-order, properties, the two closed items
docs/pages/harness.md                     # + the keys, the 30 field rows, the
                                          #   names rows, the recorded run, the minor
README.md                                 # + the 18-line section, four rows, the trim
example/docs/fwq_ftq.cpp                  # new: one FWQ family, one FTQ family
example/docs/fwq_ftq.expected             # new: the recorded run
example/docs/jitter_probe.cpp             # the region splits into two units
example/CMakeLists.txt                    # + add_docs_example(fwq_ftq)
tools/docs/example_drift.py               # + the section row, the owner row,
                                          #   the cap ledger comment
tools/noise/fwq_ftq_acceptance.sh         # new: the eight sections of FR-027,
                                          #   a script and never a gate
test/source/harness_fwq_ftq_test.cpp      # new: the series, the calibration,
                                          #   the drop rule, the resample path
test/source/harness_spectrum_test.cpp     # new: the kernel as a pure function
test/source/harness_jitter_bytes_test.cpp # new: the byte identity of a tsc run
test/source/harness_jitter_window_test.cpp # + the block, the record, the names
test/source/harness_precheck_test.cpp     # + the pair, the ranking, the refusal
test/source/harness_jitter_cli_test.cpp   # + the grammar rows, the 16 refusals
test/source/harness_json_load_test.cpp    # + the read-back rows
test/source/harness_json_schema_test.cpp  # + the schema rows
test/source/harness_jitter_probe_alloc_test.cpp # + the two armed spans
test/source/harness_cli_table_test.cpp    # the version rows move
test/CMakeLists.txt                       # + the three executables, the shape fold
specs/043-fwq-ftq/results/                # + the records of spec FR-027
```

**Structure Decision**: the feature extends the probe spec 027 owns. The
two new files carry the spectrum kernel, the one place the transform
lives. The methods grow inside the standing probe item, so the phases,
the cpu set, the overlap rule and the tracer of spec 036 stay untouched
(spec FR-004, FR-031). No new target, no second seam, no second buffer
and no second framework join.

## Design

### Decision index

- **R-01 One method per run (spec FR-001, D-1).** The method rides the
  standing `--jitter-probe` value as the key `method`, so no option row
  and no option value join. The row at `source/harness/options.cpp:654-659`
  stays, `kJitterProbeOptionValue` stays 260
  (`source/harness/detail/options.hpp:87`), and the lane block at
  `source/harness/detail/cli_options.hpp:22` stays open at 260 (FR-034).
- **R-02 The head stays first (spec FR-002).** The parser reads token 0
  as the cpu-list head, and a head token that carries `=` answers the
  standing cpu-token line (`source/harness/jitter_probe.cpp:735-737`).
  The four keys ride the standing grammar, each read once, in arms beside
  the `mode` arm (`:866-877`).
- **R-03 The cross-method pairing (spec FR-003).** A `quantum` key with
  `method=fwq`, a `work` key with `method=ftq`, and a key that stands
  twice each answer one quoted line at exit status 2. The check stands
  beside the standing cpu-duplicate check (`:910-917`).
- **R-04 The calibration runs on the probe cpu before its window (spec
  FR-010, FR-011, D-2).** `calibrateUnit` (new, file-local in
  `source/harness/jitter_probe.cpp`) runs inside `ProbeItem::operator()`
  (`:197`), after the buffer first touch (`:209`) and before the
  1,024-read measurement pass (`:243`, `kPassReads` at `:50`). It doubles
  a count from 1 in at most 25 trials, then bisects in at most 24 steps,
  and each trial reads the time source twice around a body closed on
  `sg::doNotOptimize` (`include/speedgun-ng/barrier.hpp:59`, `:78`). The
  count cap stands at 16,777,216, the standing `kMaxGapCount`
  (`include/speedgun-ng/benchmark.hpp:2194`). The target stands at 1,000
  ns for FWQ and at one thousandth of the quantum for FTQ.
- **R-05 The calibration stays out of the window (spec FR-010, FR-011).**
  The latch line (`source/harness/jitter_probe.cpp:306`) stands behind
  the search, so `JitterProbe::start()` (`:388`) returns after every cpu
  has calibrated. The window first stamp is the read behind the latch
  (`:311`), the same line the gap loop reads. The disclosure prints at
  `source/harness/cli.cpp:1129`, ahead of the batch's first run, and
  rides each cpu's series item.
- **R-06 FWQ times one unit with one read (spec FR-012, D-3).** The FWQ
  arm replaces the gap loop of the alongside phase
  (`source/harness/jitter_probe.cpp:318-329`) and of the pre-check phase
  (`:266-298`). One pass reads the time source once and takes the unit
  time as the delta between two consecutive reads, the shape the gap loop
  runs (`:321-327`).
- **R-07 FTQ counts whole units per quantum (spec FR-015, FR-016, D-4).**
  Edge k stands at the window's first stamp plus k times the quantum. The
  pass compares the stamp with the next edge; at the edge it stores the
  count, resets it, and advances the edge by one quantum. A trailing open
  quantum records no count, so the minimum of a clean run equals the
  median and the maximum.
- **R-08 One buffer shape for every method (spec FR-014, FR-016, FR-022,
  D-6).** `keepSeriesOrDrop` (new, beside `keepOrDrop` at
  `source/harness/jitter_probe.cpp:106-120`) writes into the pre-sized
  buffer and counts each lost record. The default and the range stand
  shared: 65,536 entries by default, in the range 1 to 16,777,216.
- **R-09 The statistics fold (spec FR-007, FR-008).**
  `seriesStatsOf(const ProbeCpuState&)` (new, beside `gapStatsOf` at
  `source/harness/jitter_probe.cpp:523`) copies the kept values into a
  sorted `std::vector<double>`, the shape `:527-535` uses, and reads the
  five points through `percentileField` (`:346`), which calls
  `percentileAt` (`source/harness/percentiles.cpp:104`). The standing
  minimum sample count binds each point, and the code copies no buffer.
- **R-10 The two noise fractions (spec FR-009).** FWQ divides the sum of
  the kept unit times, minus the sample count times the minimum, by that
  sum, times 100. FTQ divides the sample count times the maximum count,
  minus the sum of the counts, by the sample count times the maximum
  count, times 100. A maximum count of 0 answers 100.
- **R-11 The per-benchmark fold (spec FR-020).**
  `seriesStatsOf(const ProbeCpuState&, std::span<const TimedWindow>)`
  (new, beside the window fold at `source/harness/jitter_probe.cpp:614`)
  reads the same one series buffer and keeps the values whose stamp falls
  in a half-open window, the rule at `:636-643`. The split loop
  (`source/harness/cli.cpp:1802-1826`) fills `BenchmarkResult::jitterFolds`
  (`include/speedgun-ng/benchmark.hpp:733`), and the record rides
  `JitterEntryFold::series`. No second buffer joins
  The series element carries a value and no stamp, so the window fold
  recovers the stamp. `ProbeCpuState::seriesStartNs` stands at the stamp
  of the first kept sample, written at the first series store. A FWQ stamp
  is that start plus the running sum of the unit times. A FTQ stamp is
  that start plus the sample rank plus one, times the quantum. The
  half-open test reads the recovered stamp, and no second buffer joins.
- **R-12 The resample (spec FR-017).** `resampleUnitTimes` (new,
  `source/harness/detail/spectrum.hpp`) turns the FWQ unit times into the
  transform input. The step stands at the median unit time of the kept
  series. Each bin covers one step of cumulative unit time and holds the
  mean unit time inside it, and an empty bin takes the previous sample.
  FTQ skips the resample, because its counts stand one per quantum.
- **R-13 The transform is a plain DFT (spec FR-018, D-5).** `dftPeriods`
  (new, `source/harness/detail/spectrum.hpp`) is a pure function over a
  sample span. It reads at most 65,536 points, builds each bin's sine and
  cosine by recurrence, keeps a bin whose power stands above three times
  the median of bins 1 through N/2, and answers at most five periods. It
  runs after the window, outside every measured loop. The section "The
  spectrum kernel" states the kernel. The kernel stands a designated
  critical path (Principle VII), because the cap budget binds it: one cap
  call measured 3.3 s at `-O2` and 14.6 s under `--coverage` on the lane
  host for 65,536 points, against the 300 s test timeout.
- **R-14 The console gains 30 lines (spec FR-013, FR-019, D-7).**
  `printJitterReport` (`source/harness/report.cpp:726-813`) keeps the 16
  standing field lines (`:771-796`) and the disclosure lines (`:802-812`),
  and adds 30 lines after `:796`. The 30 open with the four calibration
  lines `method`, `workCount`, `targetNs` and `calibrationSpanNs`, then
  the 16 figures of FR-021, then the ten period lines. Each field prints
  once, with one column per probe cpu in the cpu order the block already
  walks (`:738-743`). A
  rank a cpu carries no period prints 0. The console spells the method
  `tsc`, `fwq`, `ftq`, and the JSON spells it `TSC`, `FWQ`, `FTQ`.
- **R-15 The pre-check pair and the noise refusal (spec FR-004, FR-021,
  D-9).** `printPrecheckReport` (`source/harness/report.cpp:1172`) keeps
  its line shape and swaps the pair the warning line reads (`:1305-1310`)
  and the ranking reads (`:1311-1321`) for the method pair: the noise
  fraction and the maximum unit time for FWQ, the noise fraction and the
  minimum count for FTQ. The figure's key is `noisePercent`. The pinning
  line ranks by the noise fraction ascending, ties in cpu order. The refusal fires in the pre-check phase,
  beside the standing strict refusal (`source/harness/cli.cpp:1155-1176`),
  with one line and exit status 2. In `mode=alongside` the `strict` word
  keeps its standing meaning, and a `noise=` value with no `strict`, or
  with `method=tsc`, stands accepted and disclosed as unused.
- **R-16 The writer and the schema (spec FR-006, D-7).**
  `writeJitterSeriesCpu` (new, beside `writeJitterCpu` at
  `source/harness/json_report.cpp:315`) writes one closed series item of
  21 keys. The root keys join after `loopPeriodNs` (`:1017`) inside the
  standing `jitter` object (`:1010-1036`). The per-entry object gains the
  `series` array beside the standing `stats` array (`:630-644`), with no
  `method` key and no `periods` key. The schema gains the two keys in the
  `jitter` object's `x-order` (`docs/schema/report-v1.json:47-55`) and
  `properties` (`:72`); `required` (`:56-64`) and `additionalProperties`
  (`:65`) stay.
- **R-17 The reader and the carriers (spec FR-020).** `readJitterSeriesCpu`
  (new, beside `readJitterCpu` at `source/harness/json_load.cpp:460`)
  reads and kind-checks each key in the written order, the shape
  `:494-511` uses. `readJitterRecord` (`:1243`) reads the two keys
  between the `loopPeriodNs` read (`:1265-1271`) and the `cpus` read
  (`:1272-1282`). The root arm (`:2695-2701`) fills `ReportData::jitterRun`,
  and the per-entry arm (`:2178-2207`) fills the entry series.
- **R-18 The rule-name table (spec FR-021).** `kJitterFields`
  (`source/harness/compare.cpp:251-292`) grows from 16 to 32 entries, the
  value reader takes the fold (`:241-245`), and the fault text
  (`:366-369`) reads `names none of the thirty-two jitter fields`. The
  bare `jitter:<cpu|any>:<field>` form reads the per-benchmark series,
  and `jitter:run:<cpu|any>:<field>` reads the run record. The `any`
  reading stays the uniform maximum, so a noise rule over an FTQ run
  names `noisePercent` with `higher`.
- **R-19 The byte identity (spec FR-026, FR-031, SC-004).** A run with no
  `method` key writes neither new key, so its bytes match master's apart
  from the `schemaVersion` value (`source/harness/json_report.cpp:43`,
  `:992`) and the version figures the report carries. The check runs over
  an inline fixture of master's bytes, the shape
  `test/source/harness_noise_report_test.cpp:456-461` carries, and each
  mask checks that its span holds a number, the way `maskHostFloor` does.
- **R-20 The naming (V.1).** `JitterMethod`, `JitterSeriesStats`,
  `JitterPeriodRecord` and `JitterRunRecord` take PascalCase;
  `calibrateUnit`, `keepSeriesOrDrop`, `seriesStatsOf`, `resampleUnitTimes`
  and `dftPeriods` take camelBack; `kSpectrumCap`, `kDefaultQuantumUs` and
  `kMaxWorkCount` take the `k` prefix; the new files take lower_case
  (`spectrum.hpp`, `spectrum.cpp`, `fwq_ftq.cpp`). The public spellings
  stand `JitterMethod`, `method`, `quantumUs`, `workCount`,
  `noisePercent` and `JitterSeriesStats`. The option and the public field
  carry microseconds as `quantumUs`, and the JSON key carries nanoseconds
  as `quantumNs`.
- **R-21 The evidence records (spec FR-027, FR-028, D-10).**
  `tools/noise/fwq_ftq_acceptance.sh` records one markdown file per script
  run, with eight sections: two methods, two L3 groups, two load states.
  The window comes from a named benchmark run with `--min-time=10` and
  `--repetitions=1`, so one 10 s window stands per run, and the script
  passes `gaps=10485760` for FWQ and `gaps=131072` for FTQ. Each record
  shall show a drop count of 0. The script is a script, and no CI gate
  stands on it.
- **R-22 The coverage plan (spec FR-032).** The new code adds 172 branch
  positions, and each stands reached by a named test in the Coverage plan
  below, so the branch shortfall stands 0 rows. The allowance stays 21
  (`tools/dbc/coverage_gate.sh:102`), the 21 accepted ledger rows at
  `:61-79` stay as they are, and no exclusion marker joins.
- **R-23 The README budget (spec FR-023, IV.1).** The page runs 1004
  lines against `PAGE_LINE_CAP = 1004` (`tools/docs/example_drift.py:240`),
  so the lane pays inside the three jitter sections it owns: `Jitter
  probe` (`README.md:718-740`), `Host noise pre-check` (`:741-763`) and
  `Noise tracer` (`:764-786`). Prose lines inside those three sections
  give back 35 lines, and the new section with its four rows lands 22.
  The section quotes regions of the new example
  `example/docs/fwq_ftq.cpp`, and the standing region of
  `example/docs/jitter_probe.cpp:9-25` splits into two complete top-level
  units, so every quoted
  region stands one complete top-level unit and `snippet_compile`
  passes. The page lands at or below 1004 lines, the cap staying 1004.
- **R-24 The example and the fold (spec FR-029).**
  `example/docs/fwq_ftq.cpp` carries one FWQ family and one FTQ family,
  because one run carries one method. The line `add_docs_example(fwq_ftq)`
  joins `example/CMakeLists.txt:78`, and the function stands at `:40-48`.
  The fold entry `report_shape_fwq_ftq_sample` joins after
  `report_shape_jitter_sample` (`test/CMakeLists.txt:1905-1922`).

### Modules and dependency order

The feature splits into nine modules; each carries its public surface,
its contracts and its test plan, and each lands apart behind the seams
the standing code owns. The order below stands the landing order; the
brace marks the parallel group.

```text
1. M-A  the public surface and the numbers
2. { M-B  the value grammar
     M-E  the spectrum kernel }       # M-B needs M-A; M-E needs no module
3. M-C  the probe body                # needs M-A, M-B
4. M-D  the series fold               # needs M-C
5. { M-F  the console
     M-G  the record }                # M-F needs M-C, M-D, M-E; M-G needs M-A, M-D, M-E
6. M-H  the docs and the example
7. M-I  the evidence records
```

- **M-A the public surface and the numbers**
  (`include/speedgun-ng/benchmark.hpp`, `CMakeLists.txt`). Public
  surface: `JitterMethod`, the bounds, the four fields of
  `JitterProbeOptions`, `JitterPeriodRecord`, `JitterSeriesStats`,
  `JitterRunRecord`, `JitterEntryFold::series`,
  `PrecheckCpuRecord::series`, `ReportData::jitterRun`. Contracts: the
  doxygen paragraphs of the new members and types, and the bounds the
  parser enforces at the command-line boundary (FR-003): `quantumUs`
  inside 10 to 100,000, `workCount` at 0 or inside 1 to 16,777,216,
  `noisePercent` inside 0 to 100. Test plan: the version rows
  of `harness_cli_table_test:1382-1393` move to `0.29.0`, `SOVERSION 20`
  and `SOVERSION 5` (wave 1). Nothing else builds before M-A lands.
- **M-B the value grammar** (`source/harness/jitter_probe.cpp`). Public
  surface: none beyond M-A. Contracts: the grammar, the ranges and the
  error surface of [contracts/cli-options.md](./contracts/cli-options.md).
  Test plan: the grammar rows and the 16 refusal rows of
  `harness_jitter_cli_test` (wave 1). Depends on M-A.
- **M-E the spectrum kernel** (`source/harness/detail/spectrum.hpp`,
  `source/harness/spectrum.cpp`). Public surface: none (`sg::detail`):
  `resampleUnitTimes`, `dftPeriods`, `SpectrumPeriod`, `SpectrumResult`,
  `kSpectrumCap`, `kSpectrumTop`, `kSpectrumMedianFactor`. Contracts: the
  two functions carry `\pre` and `\post` on the span, the step, the cap
  and the answer. Test plan: the tone, constant, one-sample, cap,
  truncation, six-tone and resample rows of `harness_spectrum_test`
  (wave 2). Needs no module, so it lands beside M-B.
- **M-C the probe body** (`source/harness/detail/jitter_probe.hpp`,
  `source/harness/jitter_probe.cpp`). Public surface: none (detail):
  `SeriesRecord`, the series fields of `ProbeCpuState`, `calibrateUnit`,
  `keepSeriesOrDrop`. Contracts: `SeriesRecord` carries the 8-byte
  `static_assert` after `GapRecord` (`:35-41`), the series fields join
  `ProbeCpuState` before the `alignas` stop word (`:73`), and
  `calibrateUnit` carries the caps of R-04. Test plan: the calibration,
  series, drop and resample rows of `harness_fwq_ftq_test` (wave 3), and
  the two armed spans of `harness_jitter_probe_alloc_test`
  (`test/CMakeLists.txt:890-900`). Depends on M-A and M-B.
- **M-D the series fold** (`source/harness/jitter_probe.cpp`,
  `source/harness/cli.cpp`). Public surface: none beyond M-A. Contracts:
  the two `seriesStatsOf` declarations after `gapStatsOf`
  (`source/harness/detail/jitter_probe.hpp:121`, `:136`), and the
  half-open window rule of R-11. Test plan: the fold rows of
  `harness_jitter_window_test` and the read-count rows of
  `harness_fwq_ftq_test` (wave 4). Depends on M-C.
- **M-F the console** (`source/harness/report.cpp`,
  `source/harness/cli.cpp`). Public surface: none. Contracts:
  [contracts/console-report.md](./contracts/console-report.md): the 30
  lines, the calibration lines, the method pair and the ranking. Test
  plan: the block rows of `harness_jitter_window_test` and the pair,
  ranking and refusal rows of `harness_precheck_test` (waves 4 and 5).
  Depends on M-C, M-D and M-E.
- **M-G the record** (`source/harness/json_report.cpp`,
  `source/harness/detail/json_report.hpp`, `source/harness/json_load.cpp`,
  `source/harness/compare.cpp`, `docs/schema/report-v1.json`). Public
  surface: none beyond M-A. Contracts:
  [contracts/report-json.md](./contracts/report-json.md) and
  [contracts/rule-names.md](./contracts/rule-names.md). Test plan:
  `harness_jitter_bytes_test`, the read-back rows of
  `harness_json_load_test`, the schema rows of `harness_json_schema_test`
  and the rule-name rows of `harness_jitter_window_test` (wave 4).
  Depends on M-A, M-D and M-E.
- **M-H the docs and the example** (`README.md`, `docs/pages/harness.md`,
  `example/docs/fwq_ftq.cpp`, `example/docs/fwq_ftq.expected`,
  `example/docs/jitter_probe.cpp`, `example/CMakeLists.txt`,
  `tools/docs/example_drift.py`, `test/CMakeLists.txt`). Contracts: spec
  FR-023, FR-029 and IV.1; the quick-reference rows carry the new names,
  and the drift gate binds them against the header. Test plan:
  `example_drift`, `snippet_compile`, `example_run`,
  `report_shape_fwq_ftq_sample`, the prose gate (wave 6). Depends on
  every module above.
- **M-I the evidence records** (`tools/noise/fwq_ftq_acceptance.sh`,
  `specs/043-fwq-ftq/results/`, `specs/043-fwq-ftq/quickstart.md`).
  Contracts: spec FR-027, FR-028 and R-21. Test plan: none in CI, because
  the spec stands no gate on the records; the check stands the
  quickstart's links and the record's own shape (wave 7). Depends on the
  built binary and the lane host.

### Risks

- **The measured loop gains an allocation, a syscall, a lock, or a clock
  read outside `sg::counters`.** The unit loop and the quantum loop run
  in the same body the gap loop runs
  (`source/harness/jitter_probe.cpp:318-329`), while the resample and the
  transform allocate. Check: `harness_jitter_probe_alloc_test`
  (`test/CMakeLists.txt:890-900`) replaces global `operator new` and
  proves zero allocations inside the loop, and
  `bash test/time_source_gate.sh .` names one `file:line` per forbidden
  clock term in every bound source and header.
- **A `tsc` run document moves.** The closed `jitter` object
  (`docs/schema/report-v1.json:65`) refuses a key it does not list, and
  the checker names a key out of the `x-order`
  (`source/harness/json_load.cpp:2545-2557`), so a key written for `tsc`,
  a key out of order, or a `required` list that moves breaks the
  byte-identity contract of SC-004. Check: the byte-identity test over an
  inline fixture of master's bytes, the shape
  `test/source/harness_noise_report_test.cpp:456-461` carries, and
  `harness_json_schema_test` (`test/CMakeLists.txt:691-699`) over every
  written document.
- **The main page passes 1004 lines.** `README.md` measures 1004 lines
  against `PAGE_LINE_CAP = 1004` (`tools/docs/example_drift.py:240`),
  zero headroom, and the new section plus its rows add 22 lines (FR-023).
  Check: `example_drift` (`test/CMakeLists.txt:1800-1805`), which reports
  the page length only above the cap at `tools/docs/example_drift.py:656-657`,
  and each
  section length at `:667-673` against `SECTION_LINE_CAP = 25` (`:241`).
  The trim stands inside the three jitter sections as a region
  split, and the page lands at or below 1004 lines.
- **The scripted calibration prefix stands longer than a test states.** A
  scripted clock that never passes the target runs the full search, so
  the prefix reaches 98 reads. Check: every builder takes the prefix
  length as a parameter, the two known cases stand 2 reads for a delta at
  or above the target and 98 reads for a delta below it, and each test
  asserts its own read count.
- **The cap case runs past the CTest ceiling.** One cap call costs
  2,147,483,648 complex multiply-adds, measured at 3.3 s with `-O2` and
  14.6 s with `-O0` with `--coverage` on the lane host on 2026-11-01,
  against the standing 300 s ceiling for an entry that names no `TIMEOUT`
  (`test/CMakeLists.txt:2123-2128`). Check: the suite carries one cap
  case, and the tone cases stay at or below 4,096 points.
- **The example's recorded run carries host-dependent figures.** The
  shape fold masks every digit run, every whitespace run, the per-cpu
  value lists and the per-cpu line multiplicity
  (`tools/docs/report_shape.py:131-155`, `:186-197`, `:200-247`), so a
  digit difference passes and a renamed, missing or reordered line fails,
  the pattern `example/docs/jitter_probe.expected` already follows.

### Logical view: the probe item, three methods

```text
run starts (the batch probe carries one method, one cpu set, one mode)
  └─ entry point, the option gather
       ├─ parse the value: head, threshold, gaps, mode, precheck, strict,
       │  method, quantum, work, noise                     [R-02, R-03]
       ├─ a bad key, a bad range, a cross-method key, a repeated key
       │  each answer one quoted line and exit 2, ahead of every run
  └─ JitterProbe::start()                       (jitter_probe.cpp:388)
       └─ one ProbeItem per probe cpu, one ProbeCpuState each
            ├─ first touch of the series buffer              (:209)
            ├─ calibrateUnit(count, target)      [on this cpu, before the
            │  │                                window]  [R-04, R-05]
            │  ├─ double from 1, at most 25 trials
            │  ├─ bisect, at most 24 steps
            │  ├─ two reads per trial, at most 98 reads
            │  └─ keep the count closest to the target
            ├─ the 1,024-read measurement pass               (:243)
            ├─ the latch line stands behind the calibration  (:306)
            ├─ the window first stamp                        (:311)
            ├─ the method arm of the loop
            │    ├─ tsc:  the standing gap loop, unchanged
            │    ├─ fwq:  one read per unit, the delta is the unit time
            │    └─ ftq:  one read per unit, the count closes at each edge
            └─ keepSeriesOrDrop counts every lost record    [R-08]
  └─ start() returns: every cpu has calibrated and closed its window
       ├─ the calibration lines print, ahead of the batch's first run
       │                                          (cli.cpp:1129)
       ├─ the noise gate fires in the pre-check phase, on a noise
       │  fraction at or above `noise=`, one line and exit 2   [R-15]
       ├─ seriesStatsOf(state) fills the run-level record   [R-09, R-10]
       ├─ seriesStatsOf(state, windows) fills each benchmark fold
       │                                          (cli.cpp:1802-1826)
       └─ resampleUnitTimes, then dftPeriods, after the window  [R-12]
report
  └─ console: the 16 standing lines, then the 30 series lines, one
     column per probe cpu, then the disclosure lines          [R-14]
  └─ JSON: method, series, and the per-entry series array     [R-16]
  └─ reader: loadReport fills JitterEntryFold::series and
     ReportData::jitterRun                                    [R-17]
  └─ rules: jitter:<cpu|any>:<field> reads the fold, and
     jitter:run:<cpu|any>:<field> reads the run record        [R-18]
```

### The spectrum kernel

```cpp
// source/harness/detail/spectrum.hpp
namespace sg::detail
{

inline constexpr std::uint64_t kSpectrumCap = 65536;
inline constexpr std::size_t kSpectrumTop = 5;
inline constexpr double kSpectrumMedianFactor = 3.0;

struct SpectrumPeriod
{
  std::uint64_t periodNs = 0;
  double powerRatio = 0.0;
};

struct SpectrumResult
{
  std::uint64_t points = 0;
  std::uint64_t skippedPoints = 0;
  std::uint64_t stepNs = 0;
  std::vector<SpectrumPeriod> periods;
};

/// \pre `unitTimes` holds the kept FWQ unit times in record order,
///     `stepNs` stands at or above 1, `cap` stands at or below
///     `kSpectrumCap`
/// \post one sample per whole step of cumulative unit time, an empty
///        bin taking the previous sample (FR-017)
[[nodiscard]] auto resampleUnitTimes(std::span<const std::uint64_t> unitTimes,
                                     std::uint64_t stepNs, std::uint64_t cap)
    -> std::vector<double>;

/// \pre `samples` holds at least one sample, `stepNs` stands at or
///     above 1
/// \post the answer reads the last `min(samples.size(), kSpectrumCap)`
///        points, names the skipped count, and carries at most
///        `kSpectrumTop` periods ordered by power, strongest first,
///        ties in period order (FR-018)
[[nodiscard]] auto dftPeriods(std::span<const double> samples,
                              std::uint64_t stepNs) -> SpectrumResult;

}  // namespace sg::detail
```

- The bin recurrence. For bin k, `theta` stands at `2 * pi * k / N`. The
  kernel computes `c1` and `s1` once per bin, before the inner loop. The
  inner loop keeps `cn` at `1.0` and `sn` at `0.0` and updates them by
  `cn' = cn * c1 - sn * s1` and `sn' = sn * c1 + cn * s1`, the angle-sum
  rule. The accumulators run `re += x[n] * cn` and `im -= x[n] * sn`, and
  the bin power stands at `re * re + im * im`. No library call stands in
  the inner loop (FR-018).
- The truncation. `N` stands at `min(samples.size(), kSpectrumCap)`, and
  the read starts at `samples.size() - N`, so a long series reads as its
  last 65,536 points and `skippedPoints` rides the record. A
  70,000-sample series reads 65,536 points and discloses 4,464 skipped
  points (FR-018, FR-025).
- The median filter. The median stands over bins 1 through `N / 2`, the
  DC bin out. A bin survives when its power stands above `3.0 * median`.
  A constant series leaves every such bin at zero power, so the strict
  comparison keeps nothing and the list stands empty. `N = 1` leaves no
  bin and answers an empty list (FR-018).
- The top-five cut. The surviving bins sort by power descending, ties by
  period ascending, and the kernel takes the first five. The period of
  bin k stands at `(N * stepNs) / k` in nanoseconds, and `powerRatio`
  divides the bin power by the strongest bin power, so the head stands
  1.0 (FR-019).
- The cost and the memory. The kernel runs `N * N / 2` complex
  multiply-adds, and allocates one `std::vector<double>` of `N` bin
  powers before the loop and nothing inside it: 524,288 bytes at the cap.
  The resample allocates at most `cap` samples, and the period list holds
  at most five entries (FR-017, FR-018).
- The target list needs no edit: `CMakeLists.txt:256-260` globs
  `source/harness/*.cpp` with `CONFIGURE_DEPENDS`.

### Physical view

| File | Added, at the line read at `d454bcff` |
| --- | --- |
| `include/speedgun-ng/benchmark.hpp` | `JitterMethod` ahead of the record block, after `JitterStats` (`:512-530`): the run record names the enum, so the enum precedes `ReportData` (`:962`); the bounds after `kDefaultPrecheckMs` (`:2195`); `JitterPeriodRecord`, `JitterSeriesStats` and `JitterRunRecord` after `JitterStats` (`:512-530`); `series` in `JitterEntryFold` after `stats` (`:648`); `series` in `PrecheckCpuRecord` after `stats` (`:576`); `jitterRun` in `ReportData` after `graph` (`:1001`); four fields in `JitterProbeOptions` after `tracerCpus` (`:2258`) |
| `source/harness/detail/jitter_probe.hpp` | `SeriesRecord` with its 8-byte `static_assert` after `GapRecord` (`:35-41`); the series fields in `ProbeCpuState` before the `alignas` stop word (`:73`); two `seriesStatsOf` declarations after `gapStatsOf` (`:121`, `:136`) |
| `source/harness/detail/spectrum.hpp` | new: the two kernel declarations, `SpectrumPeriod`, `SpectrumResult`, `kSpectrumCap`, `kSpectrumTop`, `kSpectrumMedianFactor` |
| `source/harness/spectrum.cpp` | new: the kernel definitions; the target list needs no edit (`CMakeLists.txt:256-260`) |
| `source/harness/jitter_probe.cpp` | `keepSeriesOrDrop` beside `keepOrDrop` (`:106-120`); `calibrateUnit` before `ProbeItem` (`:160`); the calibration call after the buffer first touch (`:209`); the method arms around the pre-check loop (`:266-298`) and the alongside loop (`:318-329`); the two `seriesStatsOf` definitions after `gapStatsOf` (`:688`); four key arms after the `mode` arm (`:866-877`); the pairing check beside the cpu-duplicate check (`:910-917`) |
| `source/harness/cli.cpp` | the calibration disclosure after `jitterProbe->start()` (`:1129`); the noise refusal beside the standing strict refusal (`:1155-1176`); the series fold in the pre-check record loop (`:1214-1223`); the series fold in the split loop (`:1819-1825`); the method and the series items in `buildJitterReportRecord` (`:402-469`); the method argument at the two `printJitterReport` calls (`:1881`, `:1883`) |
| `source/harness/report.cpp` | 30 lines in `printJitterReport` after `droppedGaps` (`:795-796`); the method pair in the warning line (`:1305-1310`) and in the ranking (`:1311-1321`) of `printPrecheckReport` |
| `source/harness/json_report.cpp` | `kSchemaVersion` (`:43`) and its history comment (`:30-42`); `writeJitterSeriesCpu` after `writeJitterCpu` (`:347`); the two root keys after `loopPeriodNs` (`:1017`); the per-entry `series` array (`:630-644`) |
| `source/harness/detail/json_report.hpp` | `kJitterMethodWords` after `kJitterModeWords` (`:77-83`); `series` in `JitterReportCpu` (`:232-237`); `method` in `JitterReportRecord` (`:249-259`); the writer postconditions (`:290-312`) |
| `source/harness/json_load.cpp` | `readJitterSeriesCpu` after `readJitterCpu` (`:544`); the two keys in `readJitterRecord` (`:1265-1282`); `data.jitterRun` at the root arm (`:2700`); the entry series at the per-entry arm (`:2193-2206`) |
| `source/harness/compare.cpp` | the value reader of `JitterField` takes the fold (`:241-245`); `kJitterFields` grows 16 to 32 (`:251-292`); the fault text (`:366-369`); the two call sites (`:1477`, `:1489`) |
| `docs/schema/report-v1.json` | `method` and `series` in the `jitter` `x-order` after `loopPeriodNs` (`:47-55`); the two properties after `loopPeriodNs` (`:72`); the closed series item and the closed `periods` item; `required` (`:56-64`) and `additionalProperties` (`:65`) stay |
| `README.md` | the new section after `### Noise tracer` (`:764-786`); four quick-reference rows after `PrecheckRecord::tracer` (`:938`); the `--jitter-probe` row (`:966`) gains the keys; the region split that keeps the page at or below 1004 lines |
| `docs/pages/harness.md` | the value rows after `strict` (`:1832`); the 30 series field rows after `droppedGaps` (`:1853`); the names rows after `JitterEntryFold` (`:1866`); the block sample (`:1798-1820`); the version sentence (`:1399`) reads `1.12`; the field rows stand prose the console block governs, and no gate reads the table |
| `example/docs/fwq_ftq.cpp`, `example/docs/fwq_ftq.expected` | new: one FWQ run and one FTQ run; the `fwq-run` region and the `ftq-run` region each carry one complete top-level unit, and the `report` region carries the recorded run; the README section quotes the two run regions |
| `example/docs/jitter_probe.cpp` | the standing region (`:9-25`) splits into `probe-unit` and `probe-main`, each a complete top-level unit, the drift gate barring a marker nested inside an open region; `Jitter probe` quotes `probe-unit`, `Host noise pre-check` and `Noise tracer` quote `probe-main`, and the guide page quotes `probe-main`; the example's output bytes stay unchanged |
| `example/CMakeLists.txt` | `add_docs_example(fwq_ftq)` after `:78`, the function at `:40-48` |
| `test/CMakeLists.txt` | `harness_fwq_ftq_test` and `harness_spectrum_test` after the `harness_jitter_preset_test` block (`:973-984`), the shape of `:874-880`; `harness_jitter_bytes_test` with the schema define, the shape of `:958-968`; the shape fold after `report_shape_jitter_sample` (`:1905-1922`) |
| `test/source/harness_fwq_ftq_test.cpp` | new: the scripted series, the calibration, the noise fractions, the drop rule, the resample path |
| `test/source/harness_spectrum_test.cpp` | new: the kernel as a pure function, the frameworkless shape of `harness_json_schema_test.cpp:16`, `:301-312` |
| `test/source/harness_jitter_bytes_test.cpp` | new: the byte identity of a run with no `method` key, and its standing console lines |
| `test/source/harness_jitter_window_test.cpp` | the block, record and rule-name rows, in the shape of the field-line walk at `:655-662` and the name rows at `:1180-1441` |
| `test/source/harness_precheck_test.cpp` | the pair, ranking and refusal rows over `capturePrecheck` (`:234-252`), the exec'd child shape at `:278-341` |
| `test/source/harness_jitter_cli_test.cpp` | the grammar rows and the 16 refusal rows in the block at `:2660-2825` |
| `test/source/harness_json_load_test.cpp` | the read-back rows, the kind rows and the closed-item rows |
| `test/source/harness_json_schema_test.cpp` | the stamped-minor row and the planted-disagreement rows, the shape at `:41-117`, `:158-189` |
| `test/source/harness_jitter_probe_alloc_test.cpp` | two armed spans: the FWQ unit loop and the FTQ quantum loop, the shape at `:212-266` |
| `test/source/harness_cli_table_test.cpp` | the version rows at `:1382-1393` move to `0.29.0`, `SOVERSION 20`, `SOVERSION 5` |
| `tools/noise/fwq_ftq_acceptance.sh` | new: the eight sections of FR-027, a script and never a gate, the shape of `tools/noise/jitter_probe_acceptance.sh` |
| `tools/docs/example_drift.py` | the new title in `SECTIONS` (`:33-68`), the owner row in `SECTION_OWNERS` (`:73-108`), the cap ledger comment (`:178-183`); both caps stay |
| `specs/043-fwq-ftq/` | `plan.md`, `research.md`, `data-model.md`, `contracts/`, `quickstart.md`, `tasks.md`, `results/` |
| the six standing contracts of FR-030 | `specs/027-jitter-probe/contracts/{cli-options,public-api,report-json,rule-names}.md`, `specs/034-host-noise-precheck/contracts/{console-report,report-shape}.md` |

No option row joins: the row at `source/harness/options.cpp:654-659`
stays, `kJitterProbeOptionValue` stays 260
(`source/harness/detail/options.hpp:87`), and the lane block at
`source/harness/detail/cli_options.hpp:22` stays open at 260 (FR-034).

### Public API surface added

| Name | Shape | Contract |
| --- | --- | --- |
| `JitterMethod` | closed enum: `TSC`, `FWQ`, `FTQ` | the method one run measures; `TSC` stands as spec 027 defines it (spec FR-001, D-1) |
| the bounds | `kDefaultQuantumUs` 100, `kMinQuantumUs` 10, `kMaxQuantumUs` 100000, `kMaxWorkCount` 16777216, `kTargetUnitTimeNs` 1000, `kCalibrationTrials` 25, `kCalibrationSteps` 24, `kDefaultNoisePercent` 1, each `std::uint32_t` and `kTargetUnitTimeNs` `std::uint64_t`; the noise maximum stands file-local at the parser as `kMaxNoisePercent` 100 | the ranges the parser enforces and the calibration reads (spec FR-002, FR-010) |
| `JitterProbeOptions::method` | `JitterMethod`, default `TSC` | the field carries the struct's doxygen contract; the enum closes the set at the type level (spec FR-001, FR-005) |
| `JitterProbeOptions::quantumUs` | `std::uint32_t`, default 100 | the FTQ quantum in microseconds, inside 10 to 100,000; the JSON key carries nanoseconds as `quantumNs` (spec FR-002, FR-015) |
| `JitterProbeOptions::workCount` | `std::uint32_t`, default 0 | 0 names the calibration of FR-010; a value stands inside 1 to 16,777,216 (spec FR-002, FR-010) |
| `JitterProbeOptions::noisePercent` | `std::uint32_t`, default 1 | inside 0 to 100, a whole number on the value grammar; the strict refusal reads it in the pre-check phase (spec FR-002, FR-021) |
| `JitterSeriesStats` | 19 figures plus the period list | the per-cpu record of one method's series; it joins `JitterEntryFold` beside `JitterStats`, so the run-level read and the per-benchmark read share one type (spec FR-007, FR-019, FR-020) |
| `JitterPeriodRecord` | `periodNs`, `powerRatio` | one reported period, nested inside the cpu's series item (spec FR-019) |
| `JitterRunRecord` | `method` | the run-level record `loadReport` fills; the resample step and the point count ride the per-cpu series item (spec FR-020, FR-006) |
| `JitterEntryFold::series` | `std::optional<JitterSeriesStats>` | the per-benchmark read over that benchmark's timed windows; empty when the windows kept no sample (spec FR-020) |
| `PrecheckCpuRecord::series` | `std::optional<JitterSeriesStats>` | the series of one pre-check window, so the pair of R-15 stands readable (spec FR-004, FR-021) |
| `ReportData::jitterRun` | `std::optional<JitterRunRecord>` | stands when the run wrote a method; a `tsc` run leaves it empty (spec FR-020, FR-031) |

No other public name joins: the series reaches the reader through the
console block, the JSON record and the fold alone. `BenchmarkResult`
gains no field, because the series rides the fold it already carries
(`include/speedgun-ng/benchmark.hpp:733`).

### Version and `SOVERSION`

The version moves to `0.29.0` (the next free minor at branch time,
`CMakeLists.txt:45`). Four public layouts move: the new fields of
`JitterProbeOptions` (`include/speedgun-ng/benchmark.hpp:2248`), the
series record of `JitterEntryFold` (`:645`), the series record of
`PrecheckCpuRecord` (`:576`) and the run record of `ReportData` (`:962`).
The version comment block names the four types. That is a public-ABI
change, so the harness
`SOVERSION` rises from `19` to `20` (`CMakeLists.txt:375`); the archive
`SOVERSION` stays `5`, the number specs/044 landed (`CMakeLists.txt:110`), because no public counters
header moves. The schema minor moves to `1.12`
(`source/harness/json_report.cpp:43`), because the series record, the
method key, the series array and its nested period list join the closed
`jitter` object.
One commit shall carry the four numbers and record the version check,
the ABI check with the four layout moves named, and the schema check
beside them (spec FR-005, FR-006, D-8, SC-009), the way the comment blocks at
`CMakeLists.txt:14-44`, `:104-109` and `:352-374` record every lane that
moved one. A rebase shall take the next free value of each and record the
check, because N6-P plans `0.29.0` and N10 plans the next and neither has
merged.

## Test Plan

TDD waves; each wave's tests shall stand written and observed failing
before the code that turns them green. Each wave names the files it
touches and the gate that closes it.

1. **Wave 1, the surface, the numbers and the grammar (FR-001 to FR-005,
   FR-034, SC-005, SC-009).** Files:
   `include/speedgun-ng/benchmark.hpp`, `CMakeLists.txt`,
   `source/harness/jitter_probe.cpp`, `test/source/harness_jitter_cli_test.cpp`,
   `test/source/harness_cli_table_test.cpp`, `test/CMakeLists.txt`. The
   wave proves: the version rows read `0.29.0`, `SOVERSION 20` and
   `SOVERSION 5`; the three method words stand accepted with the head
   first; a head token that carries `=` answers the standing cpu-token
   line; each of the 16 refusal rows prints one quoted line at status 2;
   the value-block rows stay green with no new row. Gate: `ctest` on
   `harness_jitter_cli_test` and `harness_cli_table_test`.
2. **Wave 2, the spectrum kernel (FR-017, FR-018, FR-025, SC-002).**
   Files: `source/harness/detail/spectrum.hpp`, `source/harness/spectrum.cpp`,
   `test/source/harness_spectrum_test.cpp`, `test/CMakeLists.txt`. The
   wave proves: a pure tone names its period within one bin; two tones
   rank strongest first; a constant series and a one-sample series answer
   an empty list; the cap reads every point; a 70,000-sample series reads
   the last 65,536 and discloses 4,464 skipped points; six tones keep five
   entries; the median stands over bins 1 through N/2; a bin below three
   times the median stands dropped; the three resample arms. Gate:
   `ctest` on `harness_spectrum_test`.
3. **Wave 3, the probe body (FR-007 to FR-016, FR-022, SC-001).** Files:
   `source/harness/detail/jitter_probe.hpp`, `source/harness/jitter_probe.cpp`,
   `test/source/harness_fwq_ftq_test.cpp`,
   `test/source/harness_jitter_probe_alloc_test.cpp`, `test/CMakeLists.txt`.
   The wave proves: the five calibration stops, one calibration per probe
   cpu, the calibration out of the window runtime; a clean FWQ series
   answers equal figures; one slow unit moves the maximum and the noise
   fraction; the FTQ edges stand from the first stamp; one planted 1 ms
   dip names its period; a trailing open quantum records nothing; a
   maximum count of 0 answers 100; a full
   buffer counts drops; the read counts match the scripted layout; the two
   armed spans record zero allocations. Gate: `ctest` on
   `harness_fwq_ftq_test` and `harness_jitter_probe_alloc_test`, and
   `bash test/time_source_gate.sh .`.
4. **Wave 4, the folds and the record (FR-006, FR-008, FR-019 to FR-021,
   FR-026, FR-031, SC-004).** Files: `source/harness/jitter_probe.cpp`,
   `source/harness/cli.cpp`, `source/harness/json_report.cpp`,
   `source/harness/detail/json_report.hpp`, `source/harness/json_load.cpp`,
   `source/harness/compare.cpp`, `docs/schema/report-v1.json`,
   `test/source/harness_jitter_bytes_test.cpp`,
   `test/source/harness_jitter_window_test.cpp`,
   `test/source/harness_json_load_test.cpp`,
   `test/source/harness_json_schema_test.cpp`, `test/CMakeLists.txt`. The
   wave proves: the per-entry series folds the windows and stands null
   without samples; the round trip reads every key back; a closed item
   refuses an unknown key; the schema pairs at the stamped minor; the 16
   new names resolve and the fault text names thirty-two fields; the run
   name reads the run record; `any` answers the largest count; a `tsc` run
   matches the fixture bytes. Gate: `ctest` on `harness_jitter_bytes_test`,
   `harness_json_load_test`, `harness_json_schema_test` and
   `harness_jitter_window_test`, and `report_shape`.
5. **Wave 5, the console and the pre-check (FR-004, FR-013, FR-019,
   FR-021).** Files: `source/harness/report.cpp`, `source/harness/cli.cpp`,
   `test/source/harness_precheck_test.cpp`,
   `test/source/harness_jitter_window_test.cpp`. The wave proves: the
   block prints the 30 new lines, one column per probe cpu, each field
   once; a rank a cpu omits prints 0; the calibration lines stand before
   the window; the JSON method word stands upper case; the FWQ pair
   carries the noise fraction and the maximum unit time; the FTQ pair
   carries the noise fraction and the minimum count; the pinning rank sorts
   by noise fraction; the refusal stops at status 2; the alongside
   `strict` word keeps its meaning; an unused `noise=` value stands
   disclosed. Gate: `ctest` on `harness_precheck_test` and
   `harness_jitter_window_test` under `taskset -c 0,1`.
6. **Wave 6, the docs and the example (FR-023, FR-029, SC-007).** Files:
   `README.md`, `docs/pages/harness.md`, `example/docs/fwq_ftq.cpp`,
   `example/docs/fwq_ftq.expected`, `example/docs/jitter_probe.cpp`,
   `example/CMakeLists.txt`, `tools/docs/example_drift.py`,
   `test/CMakeLists.txt`. The wave proves: the section runs 18 lines and
   the four quick-reference rows name fields the header declares; the page
   lands at or below 1004 lines; the split regions match their
   markers; the
   guide page carries the keys, the 30 field rows and the recorded run;
   the example builds, runs and folds. Gate: `example_drift`,
   `snippet_compile`, `example_run`, `report_shape_fwq_ftq_sample`,
   `report_shape_jitter_sample`, and the prose gate, which T040 runs
   over the two pages.
7. **Wave 7, the gates, the contracts and the evidence (FR-024, FR-027,
   FR-028, FR-030 to FR-033, SC-006, SC-008).** Files:
   `tools/noise/fwq_ftq_acceptance.sh`, `specs/043-fwq-ftq/results/`,
   `specs/043-fwq-ftq/quickstart.md`, the six standing contracts of
   FR-030. The wave proves: the suite passes under `taskset -c 0,1` and on
   a 2-cpu host; coverage reads 100.0% lines and 100.0% branches with a
   branch shortfall of 0 rows and the allowance unchanged at 21;
   sanitizers and tsan stand clean; the six contract
   deltas stand joined in the commits that land the code; the acceptance
   record carries eight sections, each with a drop count of 0. Gate: the
   full CI matrix, the coverage preset, `taskset -c 0,1 ctest`, and
   `bash tools/noise/fwq_ftq_acceptance.sh`.

### FR coverage map

| FR | Covering test (the wave that runs it) |
| --- | --- |
| FR-001 | `scenarioMethodWordAccepted`, `scenarioMethodWordRefused` (`harness_jitter_cli_test`, wave 1); `aTscRunWritesNoNewKey` (wave 4) |
| FR-002 | `scenarioHeadStandsFirst`, `scenarioHeadCarriesEquals`, the range rows of `scenarioQuantumRange`, `scenarioWorkRange`, `scenarioNoiseRange` (wave 1) |
| FR-003 | the 16 refusal rows of `harness_jitter_cli_test` (wave 1) |
| FR-004 | `scenarioModesRunEveryMethod` (wave 1); the phase rows of `harness_precheck_test` (wave 5) |
| FR-005 | `theVersionBindingsReadTheMovedNumbers` (`harness_cli_table_test:1382-1393`, wave 1) |
| FR-006 | `theMethodWordStandsUpperCase`, `theSeriesItemCarriesEveryKey`, `aTscRunWritesNoNewKey`, `theSeriesKeysPairAtTheStampedMinor` (wave 4) |
| FR-007 | `fwqCleanSeriesAnswersEqualFigures`, `ftqCleanCountsAnswerEqualFigures` (wave 3) |
| FR-008 | `seriesPercentilesReadTheKeptSamples`, `oneSampleSeriesReadsThatSample` (wave 3); `harness_percentile_test` runs unchanged |
| FR-009 | `fwqNoiseFractionOfASlowUnit`, `ftqNoiseFractionOfALowQuantum`, `ftqZeroMaximumAnswersOneHundred` (wave 3) |
| FR-010 | the five calibration stops and `calibrationRunsOncePerProbeCpu` (wave 3) |
| FR-011 | `theCalibrationLinesStandBeforeTheWindow` (wave 5); `calibrationStaysOutOfTheWindowRuntime` (wave 3) |
| FR-012 | the read-count checks inside `fwqCleanSeriesAnswersEqualFigures`, `ftqCleanCountsAnswerEqualFigures` (wave 3) |
| FR-013 | `theFwqBlockPrintsTheThirtyNewLines` (wave 5) |
| FR-014 | `fwqBufferFullCountsDrops` (wave 3); the armed FWQ span of `harness_jitter_probe_alloc_test` (wave 3) |
| FR-015 | `ftqEdgesStandFromTheFirstStamp`, `ftqTrailingOpenQuantumRecordsNothing` (wave 3) |
| FR-016 | `theFtqBlockPrintsTheQuantumLines` (wave 5) |
| FR-017 | `theResampleAveragesEachBin`, `anEmptyBinTakesThePreviousSample`, `theResampleStepStandsAtTheMedian` (wave 2) |
| FR-018 | the tone, constant, one-sample, cap, truncation, six-tone, median and threshold rows of `harness_spectrum_test` (wave 2) |
| FR-019 | `theFwqBlockPrintsTheThirtyNewLines`, `theRankACpuOmitsPrintsZero`, `theMethodWordStandsUpperCase` (wave 5) |
| FR-020 | `theSeriesRoundTripsThroughLoadReport`, `thePerEntrySeriesFoldsTheWindows`, `thePerEntrySeriesStandsNullWithoutSamples` (wave 4); the read-back rows of `harness_json_load_test` (wave 4) |
| FR-021 | `theSixteenNewRuleNamesResolve`, `theFaultTextNamesThirtyTwoFields`, `theRunScopedNameReadsTheRunRecord`, `theAnyReadingAnswersTheLargestCount` (wave 4); the refusal rows of `harness_precheck_test` (wave 5) |
| FR-022 | `fwqBufferFullCountsDrops`, `ftqBufferFullCountsDrops` (wave 3); the disclosure rows of `harness_jitter_window_test` (wave 5) |
| FR-023 | `example_drift` over `README.md:718-740` and the two sibling sections at `:741-786` (wave 6) |
| FR-024 | `test/time_source_gate.sh` (wave 3); the suite under `taskset -c 0,1` (wave 7); the `pickCpus` shape at `harness_jitter_probe_test.cpp:100-122` |
| FR-025 | `harness_spectrum_test` (wave 2) and `harness_fwq_ftq_test` (wave 3); no row reads a host figure |
| FR-026 | `aTscRunMatchesTheFixtureBytes`, `eachMaskedSpanHoldsANumber`, `theStandingConsoleLinesMatchTheRecordedBytes` (wave 4) |
| FR-027 | the acceptance record under `specs/043-fwq-ftq/results/` (wave 7); no test |
| FR-028 | the acceptance script and the `quickstart.md` links (wave 7); no CI gate |
| FR-029 | `report_shape_fwq_ftq_sample`, `snippet_compile`, `example_run`, `example_drift` (wave 6) |
| FR-030 | the six contract deltas, read by the tasks phase (wave 7); `harness_cli_table_test` reads the option table (wave 1) |
| FR-031 | `aTscRunWritesNoNewKey`, `aTscRunMatchesTheFixtureBytes` (wave 4); `aTscRunKeepsTheGapRefusal` (wave 5); `harness_noise_test` runs unchanged (wave 7) |
| FR-032 | the coverage preset and `tools/dbc/coverage_gate.sh` (wave 7) |
| FR-033 | `test/time_source_gate.sh`, the prose gate, `dbc-gate`, the `ci-ubuntu` build with clang-tidy and cppcheck (wave 7) |
| FR-034 | `harness_cli_table_test` with no new row and value 260 (wave 1); `test_tier_labels` and the CI file diff (wave 7) |

| SC | Covering test (the wave that runs it) |
| --- | --- |
| SC-001 | `fwqCleanSeriesAnswersEqualFigures`, `fwqOneSlowUnitMovesTheMaximum`, `fwqNoiseFractionOfASlowUnit` (wave 3) |
| SC-002 | `aPureToneNamesItsPeriod`, `aConstantSeriesAnswersAnEmptyList` (wave 2); `ftqDipEveryMillisecondNamesThePeriod` (wave 3) |
| SC-003 | `theCalibrationLinesStandBeforeTheWindow` (wave 5); `theSeriesItemCarriesEveryKey` (wave 4) |
| SC-004 | `aTscRunMatchesTheFixtureBytes`, `theNewKeysStandAbsentFromTheDocument`, `theStandingConsoleLinesMatchTheRecordedBytes` (wave 4) |
| SC-005 | the 16 refusal rows, each with `checkOneLine` and status 2 (wave 1) |
| SC-006 | the coverage preset, `tools/dbc/coverage_gate.sh`, `taskset -c 0,1 ctest` (wave 7) |
| SC-007 | `example_drift`, `snippet_compile`, `report_shape_fwq_ftq_sample`, the prose gate (wave 6) |
| SC-008 | the acceptance records and the `quickstart.md` links (wave 7) |
| SC-009 | `theVersionBindingsReadTheMovedNumbers` (wave 1); the version commit names the four layout moves |

### Coverage plan

gcov counts one condition as two branch positions, so the count column
carries positions. Each row names the arm and the test that reaches it.

| Arm | New positions | Reached by |
| --- | --- | --- |
| transform: an empty span, a one-point span | 2 | `oneSampleAnswersAnEmptyList` |
| transform: the cap compare, the skipped count | 4 | `theCapReadsEveryPoint`, `aLongerSeriesDisclosesTheSkippedCount` |
| transform: the median over bins 1 to N/2, a zero median | 4 | `theMedianStandsOverBinsOneToHalf`, `aConstantSeriesAnswersAnEmptyList` |
| transform: the power compare against three times the median | 2 | `aBinBelowTheThresholdStandsDropped` |
| transform: the top-five fill, the cut at five | 4 | `sixTonesKeepFiveEntries` |
| transform: the recurrence at bin 1, the recurrence at bin k | 2 | `aPureToneNamesItsPeriod` |
| resample: the direct path, the resampled path | 2 | `aPureToneNamesItsPeriod`, `theResampleAveragesEachBin` |
| resample: an empty bin, a filled bin | 2 | `anEmptyBinTakesThePreviousSample` |
| resample: a bin boundary, a bin open | 2 | `theResampleAveragesEachBin` |
| calibration: the target passed, the target missed | 2 | `calibrationStopsAtTheFirstTrial` |
| calibration: the trial cap at 25 trials | 2 | `calibrationStopsAtTheTrialCap` |
| calibration: the count cap at 16,777,216 counts | 2 | `calibrationStopsAtTheCountCap` |
| calibration: the interval holds one count | 2 | `calibrationStopsAtTheOneCountInterval` |
| calibration: the bisection side | 2 | `calibrationStopsAtTheOneCountInterval` |
| calibration: the closest count kept | 2 | `calibrationKeepsTheClosestCount` |
| drop rule, FWQ and FTQ | 4 | `fwqBufferFullCountsDrops`, `ftqBufferFullCountsDrops` |
| FTQ: the edge compare | 2 | `ftqEdgesStandFromTheFirstStamp` |
| FTQ: a closed quantum, a trailing open quantum | 2 | `ftqTrailingOpenQuantumRecordsNothing` |
| parser: the three method words, the refusal | 8 | `scenarioMethodWordAccepted`, `scenarioMethodWordRefused` |
| parser: the quantum range, the number read | 4 | `scenarioQuantumRange`, `scenarioQuantumNotNumber` |
| parser: the work range, the number read | 4 | `scenarioWorkRange`, `scenarioWorkNotNumber` |
| parser: the noise range, the number read | 4 | `scenarioNoiseRange`, `scenarioNoiseNotNumber` |
| parser: the cross-method key, two keys | 4 | `scenarioQuantumWithFwq`, `scenarioWorkWithFtq` |
| parser: a key that stands twice | 2 | `scenarioKeyStandsTwice` |
| parser: a fractional `quantum`, `work` or `noise` value | 6 | `scenarioQuantumFractional`, `scenarioWorkFractional`, `scenarioNoiseFractional` |
| writer: the method key present, absent | 2 | `aTscRunWritesNoNewKey` |
| writer: the series array loop | 2 | `theSeriesItemCarriesEveryKey` |
| writer: an empty period list, a filled one | 2 | `thePeriodsArrayRidesTheCpuItem` |
| writer: a per-entry null, a per-entry object | 2 | `thePerEntrySeriesStandsNullWithoutSamples` |
| reader: the method key present, absent | 2 | `aMissingMethodKeyLeavesTheRunRecordEmpty` |
| reader: the series key present, absent | 2 | `aMissingSeriesKeyLeavesTheFoldsEmpty` |
| reader: 23 keys, each kind check | 46 | `theSeriesRecordReadsBackEveryKey`, `eachNewKeyOfTheWrongKindStandsRefused` |
| reader: the series loop, the period loop | 4 | `thePeriodListReadsBack` |
| console: the 30-line loop | 2 | `theFwqBlockPrintsTheThirtyNewLines` |
| console: a rank present, a rank absent | 2 | `theRankACpuOmitsPrintsZero` |
| console: the three method words | 6 | `theMethodWordStandsUpperCase` |
| pre-check: the FWQ pair, the FTQ pair | 4 | `theFwqPrecheckPairCarriesNoiseAndMaximum`, `theFtqPrecheckPairCarriesNoiseAndMinimum` |
| pre-check: the noise compare against `noise=` | 2 | `theNoiseRefusalStopsAtStatusTwo` |
| pre-check: the pre-check phase, the alongside phase | 2 | `theAlongsideStrictWordKeepsItsMeaning` |
| pre-check: an unused `noise=` value | 2 | `anUnusedNoiseValueStandsDisclosed` |
| fold: the window loop, an empty kept set | 4 | `thePerEntrySeriesFoldsTheWindows` |
| compare: the name scan hits, the name scan misses | 2 | `theSixteenNewRuleNamesResolve`, `theFaultTextNamesThirtyTwoFields` |
| compare: the run-scoped name, the bare name | 2 | `theRunScopedNameReadsTheRunRecord`, `theAnyReadingAnswersTheLargestCount` |
| compare: the fold carries a series, the fold stands empty | 2 | `thePerEntrySeriesFoldsTheWindows`, `thePerEntrySeriesStandsNullWithoutSamples` |
| compare: the `any` maximum over a series, over one cpu | 2 | `theAnyReadingAnswersTheLargestCount` |
| compare: the run record present, absent | 2 | `aMissingMethodKeyLeavesTheRunRecordEmpty` |
| total | 172 | |

The allowance counts positions the trace holds at taken zero
(`tools/dbc/coverage_gate.sh:102`, `:114-120`). Each of the 172 new
positions stands reached by a named test, so the new code adds no row to
the shortfall. The lane's reading stands lines 100.0%, branches 100.0%,
and a branch shortfall of 0 rows, with the allowance unchanged at 21. The
gate passes on a branch reading of 100.0%, and it prints the shortfall
line only when branches stand below 100.0% (`coverage_gate.sh:104-117`).
The ledger keeps its 21 accepted rows: 19 rows of the 030 round and 2
rows of specs/031 at `source/harness/host_record_check.cpp:266`
(`coverage_gate.sh:61-79`). No `LCOV_EXCL` marker, no `--omit-lines`
entry, no `--ignore-errors` entry and no `--remove` entry shall join, and
no code shall join only to reach a branch (spec FR-032).

### Gate commands

The order runs the cheap tree scans first, then the build, then the tier,
then the instrumented builds.

| Order | Command | A pass looks like |
| --- | --- | --- |
| 1 | `python3 tools/docs/example_drift.py .` | exit 0, no line printed; a page above `PAGE_LINE_CAP = 1004` prints its length (`tools/docs/example_drift.py:656-657`) |
| 2 | `bash test/time_source_gate.sh .` | the probe lines, then `time_source_gate: clean`, exit 0 |
| 3 | `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake` | exit 0, no file named; CI pins clang-format `18.1.8`, and `tools/ci/prepush.sh:186-194` refuses another version |
| 4 | `cmake -P cmake/spell.cmake` | exit 0 |
| 5 | `python3 tools/prose/prose_gate.py --base origin/master` | `prose-lint: <n> sources, <n> units examined, 0 findings, <n> skipped`, exit 0 |
| 6 | `cmake --preset=ci-rocky` | `Configuring done`, `Generating done`, exit 0 |
| 7 | `cmake --build build` | exit 0, no warning line |
| 8 | `ctest --test-dir build --output-on-failure` | `100% tests passed, 0 tests failed out of <N>`; each new entry prints `***Passed` |
| 9 | `ctest --test-dir build -R 'harness_fwq_ftq_test\|harness_spectrum_test\|harness_jitter_bytes_test' --output-on-failure` | three `***Passed` lines, each binary prints its `ok` line |
| 10 | `ctest --test-dir build -R 'harness_cli_table_test\|harness_json_schema_test\|harness_json_load_test\|harness_compare_test' --output-on-failure` | each `***Passed`; the version rows read `0.29.0`, `SOVERSION 20`, `SOVERSION 5` |
| 11 | `ctest --test-dir build -R 'example_drift\|snippet_compile\|example_run\|report_shape_fwq_ftq_sample\|report_shape_jitter_sample\|example_drift_fixtures\|example_run_fixtures\|report_shape_fixtures' --output-on-failure` | each `***Passed`; the fold prints no difference; the three fixture entries prove the scans inspect something |
| 12 | `taskset -c 0,1 ctest --test-dir build --output-on-failure` | the same pass line; the two-cpu rows print the standing `SKIPPED` note at `harness_jitter_window_test.cpp:623-626` and `harness_precheck_test.cpp:443-445` |
| 13 | `cmake --preset=ci-coverage` | exit 0 |
| 14 | `cmake --build build/coverage -j "$(nproc)"` | exit 0 |
| 15 | `ctest --test-dir build/coverage -j 1 --output-on-failure --no-tests=error` | `100% tests passed`; the cap case finishes inside 300 s |
| 16 | `cmake --build build/coverage -t coverage` | `lines......: 100.0%`, `branches...: 100.0%`, exit 0; the shortfall line prints only when branches stand below 100.0% |
| 17 | `cmake --preset=ci-sanitize`, `cmake --build build/sanitize -j "$(nproc)"`, `ctest --test-dir build/sanitize --output-on-failure -j "$(nproc)"` | `100% tests passed`; no `AddressSanitizer`, no `runtime error` line |
| 18 | `CXX=clang++ cmake --preset=ci-tsan`, `cmake --build build/tsan -j "$(nproc)"`, `ctest --test-dir build/tsan --output-on-failure -j "$(nproc)"` | `100% tests passed`; no `WARNING: ThreadSanitizer` line |
| 19 | `cmake --preset=ci-ubuntu`, `cmake --build build` | exit 0; clang-tidy and cppcheck print no new finding |
| 20 | `cmake -S . -B build/dev --preset=ci-ubuntu`, `cmake --build build/dev -t dbc-gate -j "$(nproc)"` | exit 0 |
| 21 | `bash tools/ci/prepush.sh`, `bash tools/ci/prepush.sh --full` | every step prints its ok line, exit 0 |
| 22 | `bash tools/noise/fwq_ftq_acceptance.sh` | one markdown record under `specs/043-fwq-ftq/results/`, eight sections, each with a drop count of 0, exit 0; no CI gate stands on it |

Gate 16 is the coverage reading the plan states. Gate 12 is the FR-024
reading. Gate 1 is the FR-023 reading, and the trim stands inside the
three jitter sections as a region split. The page total lands at or
below 1004 lines, and one task
counts it with `wc -l README.md`, because the drift gate prints a page
length only above the cap. Gates 9 and 10 run as named selections inside
the full `ctest` of T048.

## Constitution Check: design re-check

- **P0**: critical-path techniques apply only to code the design names
  critical-path, and stand documented (constitution.md:484-487, `:718`).
  The unit loop, the quantum loop and the transform kernel stand named
  here. The plan keeps one time read per unit, no allocation, no I/O, no
  syscall and no lock in the two loops. The kernel keeps its per-bin sine
  and cosine recurrence, with no library call in the inner loop, because
  the cap budget binds it: 3.3 s at `-O2` and 14.6 s under `--coverage`
  for 65,536 points on the lane host, against the 300 s test timeout.
- **P1**: project conventions bind the lane: the constitution and the
  established repository patterns (constitution.md:488-490). The plan
  copies the standing probe lifecycle, the parser arms, the writer shape,
  the reader shape and the docs gates, and adds no new convention.
- **P2**: a documented exception to the P3 baseline needs written
  justification, and none stands registered (constitution.md:491-493). The
  plan registers none: no suppression, no gate change, no exclusion, and
  no `const_cast` or down-cast site.
- **P3**: the pinned Core Guidelines run under clang-tidy and cppcheck
  (constitution.md:494-495). The lane builds `ci-ubuntu` once per feature,
  and clears every new finding before merge (gate 19).
- **I**: C++23 only, and no compiler extension merges (constitution.md:496-497).
  The kernel uses `std::span`, `std::vector` and the standard math header,
  so no dependency joins (spec D-5).
- **II**: every public declaration carries `\pre`, `\post` and
  `\invariant`, and the code enforces it (constitution.md:504-508). The
  enum, the four fields, the three records, the two folds and the two
  kernel functions carry them, and the parser enforces the three ranges
  at the command-line boundary (FR-003).
- **III**: R-DCUT asks for EARS requirements, a design with both views, a
  test plan, code and tests in one change (constitution.md:533-551). The
  logical view, the physical view, the public surface and the TDD waves
  stand above, and each wave precedes the code it turns green.
- **IV**: IV.1 binds a feature that changes user-visible behavior
  (constitution.md:581-584). The lane lands one 18-line section, one
  marked snippet that quotes the `fwq-run` region of
  `example/docs/fwq_ftq.cpp`, one result sentence, four quick-reference
  rows, and the example target that builds and runs the region (R-23,
  R-24).
- **V**: clang-format, the naming rules N-1 to N-12, and the pinned
  warning set bind every new name (constitution.md:602-651). R-20 fixes
  the spellings, and the public names match the JSON keys one for one.
- **VI**: 100% lines, 100% branches and 100% DBC stand as hard gates
  (constitution.md:698-699). The Coverage plan names the covering test for
  each of the 172 new branch positions, so the branch shortfall stands 0
  rows, and the allowance stays 21.
- **VII**: time comes from `sg::counters` alone, and a contended host
  updates no baseline (constitution.md:1163-1180, `:728-730`). The lane
  names the unit loop, the quantum loop and the transform kernel as the
  critical paths, and adds the two armed-span allocation tests of
  `harness_jitter_probe_alloc_test`. The baseline clause stays dormant:
  the tree names no baseline mechanism and the CI job set carries no
  performance job, so the VIII clause that runs once baselines exist
  (`constitution.md:794-795`) binds nothing here. The calibration runs
  before the window, the transform runs after it, and the noise figures
  stand recorded on the lane host, never asserted in a test.
- **VIII**: every hard gate runs at the feature head, and a silent gate
  weakening is forbidden (constitution.md:734-803). The Gate commands table
  names all 22 commands, and no gate configuration moves (spec FR-033).
  The registration rows a gate requires stand as required work: the
  drift-gate `SECTIONS` and `SECTION_OWNERS` rows, the ctest entry of each
  new test executable, and the fold rows and golden pairs of the shape
  gate (FR-034).
- **IX**: the workflow produces spec, plan, tasks and code, and a missing
  artifact is a failed feature (constitution.md:810-831). `plan.md` stands
  here, and `tasks.md` lands with `/speckit.tasks` before code. IX names
  `ctest --preset=dev` for the per-task loop (`:822-828`), and the tree
  carries no `dev` configure preset, so every task verify runs the `build`
  tree of `ci-rocky`, the daily loop a fresh clone drives, and each feature
  also builds `ci-ubuntu` once.
- **X**: the minimum code that satisfies the requirement stands written,
  and every changed line traces to a requirement (constitution.md:862-888).
  The lane adds no second buffer, no second seam, no second framework and
  no library, and R-01 to R-24 carry the trace.
- **XI**: every generated line clears the prose gate, and each commit
  follows the template (constitution.md:932-935, `:1003-1076`). Gate 5
  runs the prose gate over the lane range, and this artifact clears the
  same rule at its own commit.

The design keeps every gate green, so **PASS**.

## Complexity Tracking

> Empty: no constitution violation stands, so no justification row
> applies.
