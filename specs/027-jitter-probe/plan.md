# Implementation Plan: Jitter Probe

**Branch**: `027-jitter-probe` | **Date**: 2026-10-21 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `specs/027-jitter-probe/spec.md`

## Summary

The harness gains a built-in jitter probe: one spinning execution context per probe cpu, pinned through the N4 placement seams, that reads the time-stamp counter through the `sg::counters` clock provider and records each gap above a threshold, the sysjitter method. The probe spans the whole batch, outside every timed window, splits its kept gaps into each benchmark's timed windows by time stamp, and reports per-cpu `JitterStats` in a console block after the benchmark tables, in a per-benchmark attribution line, and in a `jitter` object the JSON document carries at the root and beside every entry. `--jitter-probe` selects the cpus, the threshold, the mode (`alongside`, `precheck`, `both`), the gap buffer size and the strict flag; the `idle` preset picks one free cpu per L3 group the plans touch, and an overlap with a planned cpu in `alongside` mode is a status-2 usage error. The `jitter:` value names join the N5 comparison parser after PR #45 merges, so CI fails a run on noise. The probe unit is harness-internal; the public surface is one options struct, the suite-level call, the `BenchmarkHandle::jitterProbe(...)` fluent form, and `JitterStats`.

## Technical Context

**Language/Version**: C++23 (`cxx_std_23` on every target; the constitution pins the language at 2.3.0 of its lineage)

**Primary Dependencies**: the vendored hwloc 2.14.0 at `external/hwloc`, read through the N4 layout that `MachineLayout` and the `installTestLayout` seam of `source/harness/detail/internal.hpp` carry; the vendored simdjson v4.6.11 at `external/simdjson` for the JSON write and read; the counters library: the `ClockProvider` `machine/tsc` leaf and its calibration (`include/speedgun-ng/counters_clock.hpp`), the fake provider (`include/speedgun-ng/counters_fake.hpp`); the N4 placement seams (`resolvePreset`, `Preset`, `PlacementPlan` in `source/harness/placement_preset.cpp`); `getopt_long` inside `source/harness/cli.cpp`; the N5 value-name parser (`parseCompareRule`, PR #45) for the `jitter:` rule names.

**Storage**: N/A: no file and no database. The gap buffers live in process memory for the life of the probe. The one document the feature writes is the report file `--json` names, beside the recorded acceptance results under `specs/027-jitter-probe/results/`.

**Testing**: the repository's own harness (`speedgun-ng_test` pattern: one executable per area plus `add_test`), the `FakeProvider` scripting a `machine/tsc` leaf at `ReadMode::FAST_TSC`, the `installTestLayout` seam for synthetic layouts, the probe reader-install seam (R-02), and per-file counting `operator new` pairs (Q-2). Tests run with `ctest -j 2`.

**Target Platform**: Linux only (`ci-ubuntu`, `ci-rocky`). The daily loop drives `ci-rocky`; once per feature, `ci-ubuntu` builds with clang-tidy and cppcheck wired in. thrawn (4 L3 groups, 24 cores, 48 threads, one node) is the reference host. Every test passes on a 4-cpu CI runner and on a 2-cpu host: cpus come from `sched_getaffinity` and each stands below the online count (FR-040).

**Project Type**: library feature: a C++ archive surface plus a command line of the harness entry point. No new target.

**Performance Goals**: the probe starts before the first benchmark and stops after the last, outside every timed window (FR-006). The probe loop performs no allocation, no syscall and no I/O (FR-005). The loop period and the threshold are measured before the window and disclosed (FR-007). No benchmark regression is claimed in this plan, so no number is promised here; the acceptance script measures the benchmark time with and without the probe and records the figure under `specs/027-jitter-probe/results/` (FR-042, D-8, Principle VII).

**Constraints**: the probe reads time only through `sg::counters` (FR-004; the time-source gate stays green); 100 percent line and branch coverage on new paths with no exclusion marker; no sleep and no clock in tests; one line and exit status 2 for every bad option value of this feature (FR-038); the README section stays within the default section cap 25, and `PAGE_LINE_CAP` rises by the measured amount at the rebase (FR-034, R-14); hwloc stays private to `source/`; the option takes no short letter, and its long-option values stand at 256 or above (FR-011).

**Scale/Scope**: one new harness translation unit beside its detail header (`source/harness/jitter_probe.cpp`, `source/harness/detail/jitter_probe.hpp`), one public header extended, one pure resolver added beside the preset resolver, four new test executables, one docs example, one schema extension, one acceptance script, and the shared lists the spec's Parallel work section names.

**Execution mode**: TDD (Principle III). Every test task is written and observed failing before the code task that turns it green exists; the Test Plan below records the order and the gates each wave runs.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Gate | Status |
| --- | --- | --- |
| I. Standard-first | C++23 standard surface. The designated critical path is the probe loop (D-7): its P0 techniques are the fixed-stride record write into the pre-sized buffer, the single stop-flag read on its own cache line, and the spin wait through the shipped pause-hint shape; each stands documented at R-03 and R-04. The TSC read rides the counters provider, so no intrinsic header and no compiler extension join | PASS |
| II. Design by contract | Every new public interface documents its contract in the `\pre`/`\post`/`\invariant` shape and enforces it with the contract facility: the options struct, the suite-level call, the handle call, and `JitterStats` carry the contracts of the Public API surface table; the internal holder, resolver, overlap check, split and fold carry `SG_REQUIRE`/`SG_ENSURE` at their seams | PASS |
| III. R-DCUT | Requirements FR-001 to FR-043 map to the design decisions R-01 to R-15 below and to `tasks.md`. Execution mode: TDD, recorded above and in the Test Plan | PASS |
| IV. Documentation | README section "Jitter probe" with its quick-reference rows (IV.1 main-page impact: the spec carries the checklist line), `docs/pages/harness.md` option table and field reference, one runnable `example/docs/jitter_probe.cpp` with its `.expected` file | PASS |
| V. Style | clang-format, clang-tidy, cppcheck clean at the feature head; the naming rules of V.1 hold at every new name: PascalCase types, camelBack functions, `k`-prefixed constants, lower_case files (R-15) | PASS |
| VI. Coverage | 100 percent line and branch on new paths, no exclusion marker; every branch of the gap detection, the buffer-full path, the threshold rule, the split, the `idle` preset, the overlap check, the mask change, the modes, the option parse, the JSON write and read, and the rule names runs under scripted time (FR-039) | PASS |
| VII. Performance | No probe work stands inside a timed window: the loop starts before the first benchmark and stops after the last, the fold and the percentiles run after the stop, and the disclosure reads no clock. The loop minimizes branches and memory touches (P0, I) | PASS |
| VIII. CI gates | Every hard gate runs at the feature head: format-check, spell-check, tidy, cppcheck, sanitizers, tsan, coverage, docs drift (drift, snippet compile, example run, report shape), prose, DBC, time-source, name-check | PASS |
| IX. Spec-driven | spec, plan and research stand in `specs/027-jitter-probe/`; `tasks.md` lands with `/speckit.tasks` before code | PASS |
| X. Anti-slop | One probe unit, one pure resolver, one mode enum over three closed values; the pin rides `PlacementSlot` and `PlacementFn`, so no second pin path exists; the reader-install seam installs a counters reader, so no second time source exists; the rule names wait for the N5 parser and copy none of it (R-12). The one new seam carries the need R-02 records: the fake's action-count model cannot express a series a barrier releases | PASS |
| XI. Prose | The gate walks `git ls-files`, so at this point it clears the tracked tree only, while these artifacts stand untracked and await the run that lands with the branch's first commit, the way specs/025 recorded | PASS for the tracked tree, pending for these artifacts |

**Gate weakening requested**: no. **Gate strengthening requested**: no. The one budget figure that moves is `PAGE_LINE_CAP` in `tools/docs/example_drift.py`, raised at the rebase by the measured size of the new section and its rows through the recorded-decision comment the specs/023 and specs/025 moves precede (R-14, the spec's Budget note); no constitution text changes and no gate loses a check.

No gate fails, so **Complexity Tracking stays empty** at the gate: the new public types are the feature's own surface (FR-003, FR-015), and no new target and no new dependency join.

## Project Structure

### Documentation (this feature)

```text
specs/027-jitter-probe/
├── spec.md              # FR-001 to FR-043, D-1 to D-16, Clarifications Q-1, Q-2,
│                        #      Constraints, SC-001 to SC-008
├── plan.md              # this file
├── research.md          # R-01 to R-15 with rationale and rejected alternatives
├── checklists/
│   └── requirements.md
├── results/             # FR-043: the thrawn acceptance records
└── tasks.md             # Phase 2 output of /speckit.tasks
```

The main-page surface this feature ships (IV.1):

- `README.md`, section "Jitter probe": at most 25 lines, with its quick-reference rows, and no entry added to `SECTION_LINE_CAPS` (FR-034).
- `docs/pages/harness.md`: the full option table and the field reference of `JitterStats` (FR-034).
- `example/docs/jitter_probe.cpp`: one benchmark run with `--jitter-probe=idle,mode=precheck` through the library, built as the `docs_jitter_probe` target with its `run_docs_jitter_probe` runner and its `.expected` file (FR-035, R-14).
- `docs/schema/report-v1.json`: the run-level and per-entry `jitter` objects, key order normative (FR-029, R-09).
- `tools/docs/example_drift.py`: the `SECTIONS` and `SECTION_OWNERS` entries, and the measured `PAGE_LINE_CAP` move at the rebase (R-14).

### Source (this feature)

```text
include/speedgun-ng/benchmark.hpp        # JitterProbeMode, JitterProbeOptions, JitterStats,
                                         #      runJitterProbe, BenchmarkHandle::jitterProbe
source/harness/
├── jitter_probe.cpp                     # NEW: the probe unit: the holder of the probe contexts,
│                                        #      the availability check, the loop work item, the fold
├── detail/jitter_probe.hpp              # NEW: the holder, the gap record, the reader-install seam,
│                                        #      the split and the fold; nothing here is public API
├── detail/internal.hpp                  # RunOptions fields, the seam declarations beside
│                                        #      installTestLayout
├── placement_preset.cpp                 # resolveIdleCpus and the overlap check, beside resolvePreset
├── cli.cpp                              # --jitter-probe in the option table (kOptJitterProbe),
│                                        #      the parse once before expansion, the batch start and stop
├── runner.cpp                           # TSC stamps of every timed window beside the chunk boundaries
├── report.cpp                           # probe block after the tables, the per-benchmark line,
│                                        #      disclosure lines in the noise-warning region
├── json_report.cpp                      # the root jitter object, the per-entry jitter object
└── json_load.cpp                        # the optional jitter keys in readEntry/readRow

test/
├── source/harness_jitter_probe_test.cpp  # NEW: the loop branches, the buffer-full path, the
                                         #      threshold rule, the no-allocation proof
├── source/harness_jitter_window_test.cpp # NEW: the split, the multi-pass split, the
                                         #      attribution, the JSON round trip
├── source/harness_jitter_preset_test.cpp # NEW: the idle resolver, the overlap check, the mask change
├── source/harness_jitter_cli_test.cpp    # NEW: the option grammar, the status-2 cases, the
                                         #      option-value uniqueness test
└── CMakeLists.txt                        # four add_executable/add_test pairs

example/
├── docs/jitter_probe.cpp                 # the README snippet's program (listed above)
└── CMakeLists.txt                        # add_docs_example(jitter_probe)

tools/noise/jitter_probe_thrawn.sh        # NEW: the acceptance script of FR-042
```

**Structure decision** (R-01): the probe is one harness-internal translation unit beside a private header in `source/harness/detail/`, the shape the topology read and the preset resolver stand behind `detail/internal.hpp`. The spinning contexts ride the batch state `speedgunMain` already owns, so the probe spans the whole batch with no change to the runner's per-instance lifecycle, and the pin rides the one placement path feature 020 and specs/025 ship. The public surface lands in `include/speedgun-ng/benchmark.hpp` beside `BenchmarkHandle`: one options struct, the suite-level call, the fluent form, and `JitterStats` (FR-015, R-15).

**Files outside the feature directory this change edits**, each with the reason the spec's Parallel work rule asks for: `CMakeLists.txt` (the `VERSION` and `SOVERSION` lines, D-16), `include/speedgun-ng/benchmark.hpp` (the public surface this feature adds, FR-015), `specs/015-benchmark-harness-core/contracts/cli.md` (the option table is the CLI contract feature 015 owns, and the new option joins it in the same commit as the parser), `specs/026-regression-gate/contracts/` (the `jitter:` value names join the grammar the N5 parser contract owns, in the same commit as the names, after PR #45 merges; FR-033), `test/CMakeLists.txt` (the four new test executables, each carrying the `source/harness` include directory the way `harness_gap_test` does), `example/CMakeLists.txt` (the new docs example and its runner), `README.md` and `docs/pages/harness.md` (Principle IV), `docs/schema/report-v1.json` (the jitter objects, R-09), and `tools/docs/example_drift.py` (the section registration and the measured page cap). No source file of the two open lanes' features is edited.

## Design

### Decision index

| Id | Decision | Requirement |
| --- | --- | --- |
| R-01 | The batch owns the probe contexts: `speedgunMain` builds one holder beside the `ReportData` batch state, starts it before the first `runner.run(*entry)`, and stops, drains and joins it after the selected loop ends; each probe context constructs as `ExecutionContext(ContextPolicy{WaitMode::SPIN, PlacementSlot::cpu(n)})`, so the pin rides the default placement function every benchmark context uses; the probe unit is `source/harness/jitter_probe.cpp` beside a private header in `source/harness/detail/` | FR-001, FR-006, FR-015, FR-021 |
| R-02 | The time seam is the provider catalog: the probe opens a reader over the `machine/tsc` leaf through the same `System::local()` the runner resolves its leaves through; tests script the gap series with a `FakeProvider` that declares `machine/tsc` at `ReadMode::FAST_TSC` and hands the window a scripted cumulative sequence with `setPoints`; a reader-install seam in `source/harness/detail/` substitutes the reader the probe opens where a barrier must release the series, and it installs a counters `WindowReader`, never a second time source | FR-004, FR-039, FR-041, Q-1 |
| R-03 | One kept gap is one 16-byte record: the stamp and the gap in ns. Each probe cpu owns one buffer pre-sized to 65,536 records (`kDefaultGapCount`), settable through `gaps=<n>` in the range 1 to 16,777,216, allocated and first touched on the probe cpu inside the probe thread's work item before the window opens; a full buffer keeps counting the gaps, their total and their max, drops the stamps, and sets `droppedGaps`; the fold and the split read only the kept records | FR-005, FR-009, FR-010, D-6 |
| R-04 | The window split reads one time base: the runner samples `machine/tsc` on the benchmark thread at the open and the close of every timed window, beside the chunk boundaries the recorder already tracks, so each repetition, each chunk and each pass of a multi-pass run carries its own `[start, end)` window; a kept gap folds into every window that contains its stamp; the handoff is join-ordered, with the stop flag one `std::atomic<bool>` in an `alignas` block of its own cache line, and the runner reads the buffers only after the probe contexts are joined | FR-005, FR-006, FR-021, FR-022 |
| R-05 | `JitterStats` carries the sixteen fields of data-model.md E-03 in the E-03 order; the percentile fields compute through `percentileAt` with its nearest-rank rule over the sorted kept gaps, the rule the report columns fill through; `runtimeNs` is the span the loop measured from its start to the read of the stop flag, through the same reader; `loopPeriodNs` is the minimum loop period of the fixed measurement pass data-model.md E-05 specifies, 1,024 loop reads on the same probe cpu before the window opens, which sets the effective threshold when 10 times that period stands above the configured value | FR-002, FR-007, PC-4 |
| R-06 | `--jitter-probe` enters the option table as a long-only entry whose value stands at 256 or above, held as the named constant `kOptJitterProbe`; the value parses once in `speedgunMain` before the expansion and before any run, the point where the `--placement` parse stands today; every rejection is one line and exit status 2; one test asserts every value in the option table is unique | FR-008, FR-011, FR-038 |
| R-07 | The `idle` resolution is one pure function beside `resolvePreset`: given the layout and the set of cpus the selected instances' plans use, it returns the lowest free cpu of each L3 group the plans touch; it skips and discloses a group with no free cpu, and it stops with status 2 when no cpu is free at all; the overlap check in `alongside` mode compares each probe cpu against the planned cpus and their SMT siblings; for a benchmark that carries no placement, the harness narrows the process affinity mask with `sched_setaffinity(0, ...)` for that instance's run, discloses the change, restores the mask after the instance ends, and stops with status 2 when the intersection empties; tests take synthetic layouts through `installTestLayout` | FR-016, FR-017, FR-018, FR-019, FR-020, D-3, D-4, D-5 |
| R-08 | The console probe block prints after the benchmark tables, one column per probe cpu, listing the `JitterStats` fields in the E-03 order; the per-benchmark line `jitter: worst cpu <n> gaps=<k> max=<ns> total=<percent>` prints under that benchmark's rows inside `printResult`; the run-level disclosure (the record data-model.md E-09 canonically carries) prints as new lines in the end-of-report region `printNoiseWarning` owns, and replaces nothing: the noise-warning condition and its three fields keep their bytes | FR-013, FR-023, FR-024, FR-025, FR-036, PC-6, D-12 |
| R-09 | The document gains a run-level `jitter` object at the root, after `speedgun` and before `benchmarks`, and each benchmark entry gains a nullable `jitter` object after `noiseWarning`; the writer extends the `JsonObjectWriter` scopes of `writeReport`, the reader extends `readEntry`/`readRow` with the same optional-key pattern `contextPercentiles` and `raw` use, and the schema rows move in the same commit; `--json-raw` gains the value `gaps` beside `chunks` and `none`, and only that value writes the gap list | FR-026, FR-027, FR-028, FR-029 |
| R-10 | The mode is one enum (`ALONGSIDE`, `PRECHECK`, `BOTH`) inside `JitterProbeOptions`; the run executes at most two phases over one probe cpu set and one buffer set per cpu: the pre-check phase (all chosen cpus spin at the same time for 200 ms, `precheck=<ms>` setting the duration) runs before the first benchmark when the mode is `PRECHECK` or `BOTH`, and the alongside phase spans the batch when the mode is `ALONGSIDE` or `BOTH`; the overlap check binds the alongside phase only | FR-012, FR-014, D-1, D-3, D-13 |
| R-11 | At probe start, before any benchmark runs, the probe checks the provider catalog for the `machine/tsc` leaf and opens a reader over it; a missing leaf or a null reader makes the probe disclose `not available` and the run continue with status 0; only `,strict` on the option value fails the process with status 2; no new `Availability` enumerator and no CPUID check stand beside the catalog fact the build already publishes | FR-037, D-10, Q-1 |
| R-12 | The value names `jitter:<cpu>:<field>` and `jitter:any:<field>`, with the `jitter:run:` prefix naming the whole-run values, join `parseCompareRule` after PR #45 merges; the field names are the E-03 names in camelCase; the branch waits for N5 and rebases, and copies no parser code; a `jitter:` rule evaluated on a report with no probe is the parser's usage error, one line and status 2 | FR-030, FR-031, FR-032, FR-033, D-11 |
| R-13 | Four test executables follow the `test/source/harness_*_test.cpp` convention and join `test/CMakeLists.txt` with the standing `add_executable`/`add_test` pair: `harness_jitter_probe_test.cpp` (the loop branches, the buffer-full path, the threshold rule with the 10x floor, the no-allocation proof), `harness_jitter_window_test.cpp` (the split, the multi-pass split, the attribution, the kept-gaps-only rule, the JSON round trip), `harness_jitter_preset_test.cpp` (the `idle` resolver, the overlap check, the mask change, over synthetic layouts installed through `installTestLayout`), and `harness_jitter_cli_test.cpp` (the option grammar, every status-2 case, the option-value uniqueness test, the mode machine) | FR-039, FR-040, FR-041, SC-005, Q-2 |
| R-14 | The README section runs at most 25 lines with its quick-reference rows and adds no entry to `SECTION_LINE_CAPS`, the default cap 25 applying; the example builds through `add_docs_example` as the `docs_jitter_probe` target with its `run_docs_jitter_probe` runner, and its `.expected` file checks only the fields with no host dependence: the block header, the field names, and the cpu count rule; `docs/pages/harness.md` carries the full option table and the field reference; at the rebase, `PAGE_LINE_CAP` moves to the measured length of the page through the recorded-decision comment | FR-034, FR-035, SC-007 |
| R-15 | New types take PascalCase (`JitterProbeOptions`, `JitterStats`, `JitterProbeMode`, `GapRecord`), new functions take camelBack (`resolveIdleCpus`, `runJitterProbe`), new constants take the `k` prefix (`kDefaultGapCount`, `kMinGapCount`, `kMaxGapCount`, `kDefaultThresholdNs`, `kDefaultPrecheckMs`, `kOptJitterProbe`), and new files take lower_case (`jitter_probe.cpp`, `jitter_probe.hpp`); the public entry points take one `const JitterProbeOptions&` and no bool, and the strict flag is one field of the struct | FR-003, FR-015 |

### Risks

- **The probe loop purity (R-03, R-04)**. An allocation, a syscall or an I/O call could reach the loop through a helper it calls, and the measurement the feature exists to take would carry the noise of the probe itself. Mitigation: the loop body holds four moves, one reader call, one comparison, one record store into the pre-sized buffer, one stop-flag read; the no-allocation proof of FR-041 diffs a counting `operator new` pair around the region under test; the time-source gate holds clock calls out of the tree; the record store carries no capacity check beyond the fill test.
- **The window-split handoff (R-04)**. The runner thread could read a probe buffer while the loop writes it, or a gap record could tear across the join edge, and a gap would land in the wrong window. Mitigation: one producer and one consumer over each buffer; the join of the probe contexts is the happens-before edge the runner reads the buffers behind; the stop flag stands in its own cache line; the split compares stamps taken from one leaf in one time base, so no conversion stands between a gap stamp and a window edge.
- **The buffer-full path (D-6, FR-010)**. The report could understate the noise when the buffer fills, because the dropped records carry no stamps the split can place. Mitigation: the probe keeps counting the gaps, their total and their max while it drops the stamps, sets `droppedGaps`, and the disclosure states that the split and the percentiles use only the kept gaps; the buffer-full branch stands under test through the scripted series (R-02, R-13).
- **The option-parse error surface (FR-038, R-06)**. A bad value could print after the first benchmarks ran, and a CI run would lose measured time to a typo. Mitigation: the parse runs once in `speedgunMain` before the expansion and before any run, the pattern the `--placement` parse and the `--json-raw` rejection established; every rejection path of this feature names one test in `harness_jitter_cli_test.cpp`; the option-value uniqueness test holds the table against future edits.
- **Parallel lanes (FR-033, D-16)**. N5 and N6 stood open as PR #45 and PR #44 at plan time; both are merged into master (`9976768`, `bd10d76`) as the rebase records, and the `jitter:` names ride the merged N5 parser. The shared lists this feature touches (`CMakeLists.txt`, the option table, the public header, the schema, the docs caps, the example run list) are the lists both carried. Mitigation: the spec's rebase rule governs: rebase onto the new master, keep the lines of all features in each shared list, take the next free minor version, set `PAGE_LINE_CAP` to the measured length, run the full gate set, and push one time with `--force-with-lease`.

### Logical view: the probe lifecycle

```text
speedgunMain
  ├─ parse --jitter-probe once, before expansion           (FR-011, R-06)
  ├─ availability: catalog leaf + open() over machine/tsc  (FR-037, R-11)
  │     missing leaf or null reader ─▶ "not available", status 0
  │                                        (strict on the value: status 2)
  ├─ start: one context per probe cpu, before the first run      (R-01)
  │     work item: measure loop period ─▶ effective threshold   (FR-007, R-05)
  │     loop: read TSC ─▶ compare ─▶ store one 16-byte record   (FR-005, R-03)
  │            read the stop flag, own cache line, once per pass (R-04)
  ├─ ... the batch runs ...
  ├─ stop: write the flag, drain, join, after the last run       (FR-006)
  └─ fold + percentiles over the kept gaps, after the stop       (D-7, R-05)
```

The buffer is allocated and first touched inside the probe thread's work item, after the pin at thread start and before the loop takes its first read, so the first-touch faults land on the probe cpu and outside every measured window (FR-009, R-03).

### Logical view: one run with the probe alongside

```text
mode ∈ {ALONGSIDE, PRECHECK, BOTH}     # one enum, default ALONGSIDE (D-1, R-10)
  PRECHECK or BOTH ─▶ pre-check phase: all chosen cpus spin at the same
                        time for 200 ms (precheck=<ms> sets it), before
                        the first benchmark; overlap with planned cpus
                        stands allowed here (D-3, D-13)
  ALONGSIDE or BOTH ─▶ alongside phase: the probe spans the batch (R-01);
                        the overlap check binds here: a probe cpu that is
                        a planned cpu or an SMT sibling of one is a
                        status-2 stop naming the cpu and the benchmark
                        (D-3, D-9, R-07)
  idle preset ─▶ one lowest free cpu per L3 group the plans touch;
                 a group with no free cpu is skipped and disclosed;
                 no free cpu at all is a status-2 stop (D-4, R-07)
  unpinned benchmark ─▶ the process mask drops the probe cpus for that
                 instance's run, disclosed, restored after it ends;
                 an empty intersection is a status-2 stop (D-5, R-07)
```

### Logical view: the window split

```text
gap stamps (TSC counts)        window edges (TSC counts, the same leaf)  (R-04)
  one 16-byte record per kept    [start, end) per repetition, per chunk,
  gap, in stamp order            and per pass of a multi-pass run (PC-5)
                 │
                 ▼
  for each kept record: fold into every window with start <= stamp < end
                 │
                 ▼
  per benchmark, per probe cpu: JitterStats over that benchmark's windows
                 │
                 ▼
  the per-benchmark console line, and the per-entry jitter object
  (FR-021, FR-022, FR-024, FR-027)
```

A buffer that filled keeps its counts, its total and its max, and the split and the percentiles read only the kept gaps; the report says so (FR-010, R-03).

### Logical view: the report flow

```text
benchmark tables (the same shape with no probe, FR-013)
  └─ per-benchmark line under the rows, inside printResult  (FR-024, R-08)
probe block after the benchmark tables                       (FR-023, R-08)
  └─ one column per probe cpu, the E-03 field order
disclosure lines in the noise-warning region                 (FR-025, R-08)
  └─ probe running and on which cpus, the loop period, the threshold,
     droppedGaps, skipped groups, the mask change, kept-gaps-only
JSON document                                                (R-09)
  ├─ root jitter object after speedgun, before benchmarks    (FR-026)
  ├─ per-entry nullable jitter object after noiseWarning     (FR-027)
  └─ gap list only under --json-raw=gaps                     (FR-028)
loadReport reads the written file back with equal values     (FR-029)
```

### Physical view

- `jitter_probe.cpp` is the one harness TU that owns the probe: the holder, the availability check, the loop work item, the fold. Its private header `source/harness/detail/jitter_probe.hpp` declares the holder and the seams; nothing there is public API.
- The harness-internal seam stands in `source/harness/detail/internal.hpp` beside `installTestLayout`: the reader-install seam of R-02, the `RunOptions` field the parse fills, and the window-stamp access the split reads.
- The public surface stands in `include/speedgun-ng/benchmark.hpp`: `JitterProbeMode`, `JitterProbeOptions`, `JitterStats`, the suite-level call, and `BenchmarkHandle::jitterProbe(...)` beside the fluent setters (FR-015, R-15).
- The call graph: `cli.cpp` parses the option and starts and stops the holder; `runner.cpp` stamps the window edges; `placement_preset.cpp` resolves `idle` and answers the overlap check; `report.cpp` prints the block, the line and the disclosure; `json_report.cpp` and `json_load.cpp` write and read the document; the compare gate rides the N5 parser after PR #45 merges (R-12).

### Public API surface added

| Name | Kind | Contract |
| --- | --- | --- |
| `JitterProbeMode` | enum | `ALONGSIDE`, `PRECHECK`, `BOTH`; `ALONGSIDE` is the default when the option names no `mode=` value (D-1, R-10) |
| `JitterProbeOptions` | struct | the cpu list or `idle`, `thresholdNs`, the mode, the gap count, the pre-check duration, and the strict flag, each field with a default member initializer; no entry point of this feature takes a bool (FR-003, R-15) |
| `runJitterProbe(const JitterProbeOptions&)` | suite-level call | \pre the run has not started for any benchmark (E-02). \post the suite's next run starts the probe before its first selected instance and stops, drains and joins it after the last, over these options, the way the command-line option does; the command-line value wins when both stand (FR-015, R-01) |
| `BenchmarkHandle::jitterProbe(const JitterProbeOptions&)` | handle option | \pre the handle names a registered entry and the run has not started for this benchmark (E-02). \post the entry carries the options, the returned handle is this handle, and the run of that family starts the probe before its first instance and stops it after the last (FR-015) |
| `JitterStats` | struct | the sixteen fields of data-model.md E-03 in the E-03 order; \invariant the percentile fields follow the nearest-rank rule of `source/harness/percentiles.cpp` over the kept gaps (PC-4, R-05) |

No new public header, no new target: the names join `include/speedgun-ng/benchmark.hpp` beside `BenchmarkHandle` (FR-015, R-01).

### Version and `SOVERSION`

- `VERSION` takes the reserved minor at merge: `0.17.0`, PR #45 having taken `0.15.0` and PR #44 `0.16.0` (D-16).
- Harness `SOVERSION` takes 11 from the post-PR master: 8 at the audit point (`CMakeLists.txt:247`), 9 with PR #45, 10 with PR #44, 11 with this feature (D-16).
- Archive `SOVERSION`: 3 with PR #44, and the same rule takes it to 4 at this rebase (D-16: each `SOVERSION` rises by one from master). The design moves no exported layout: the new types stand free, and the one member function joins the existing exported class without a data member, a virtual, or an exported struct change, the shape specs/025 recorded as a no-move.
- Each rebase re-reads the `VERSION` and the `SOVERSION` lines of `CMakeLists.txt` on master, takes the next free minor version, and raises each `SOVERSION` by one from master; the rebase note records the values and, when the merge order changed, why (D-16).

## Test Plan

| Executable | Covers | Method |
| --- | --- | --- |
| `test/source/harness_jitter_probe_test.cpp` | FR-002, FR-005, FR-007, FR-008, FR-009, FR-010, FR-014, FR-041, SC-005 | the scripted series through the fake provider and the reader-install seam: every branch of the gap detection, the threshold rule with the 10x loop-period floor, the buffer-full path with `droppedGaps`, the kept-records fold, the pre-check phase execution FR-014 names, and the no-allocation proof. Start and stop synchronize on barriers |
| `test/source/harness_jitter_window_test.cpp` | FR-021, FR-022, FR-024, FR-026, FR-027, FR-029 | the split over a two-benchmark run where the gaps land inside one benchmark's window; the multi-pass windows of PR #44; the per-benchmark attribution line; the console block; the JSON round trip: `writeReport` writes, `loadReport` reads back with equal values, one test per reader error code |
| `test/source/harness_jitter_preset_test.cpp` | FR-016, FR-017, FR-018, FR-019, FR-020 | the `idle` resolver and the overlap check over synthetic layouts installed through `installTestLayout`; the unpinned-mask change and its empty intersection; the skipped group and the no-free-cpu stop |
| `test/source/harness_jitter_cli_test.cpp` | FR-008, FR-009, FR-011, FR-012, FR-013, FR-038, FR-040 | the option grammar driven directly: every status-2 case (a bad cpu, an unknown preset, an overlap in `alongside`, a bad threshold, a bad mode, a bad `gaps` value, a bad `precheck` value, no free cpu for `idle`), the mode machine, the probe-free report shape, and the option-value uniqueness test over the whole table |
| the N5 comparison surface | FR-030, FR-031, FR-032, SC-004 | the rule names through `parseCompareRule` after PR #45 merges: the worst-cpu field names, the `jitter:run:` prefix, the status-2 stop on a probe-free report, and the status-1 comparison failure on a bound the probe moves past |
| `report_shape` and `example_run` | FR-013, FR-035, SC-007 | the registered comparisons fold equal on every probe-free binary; `docs_jitter_probe` builds, runs, and its `.expected` file checks the block header, the field names, and the cpu count rule |
| `tools/noise/jitter_probe_thrawn.sh` | FR-042, FR-043, SC-008 | the thrawn acceptance runs, recorded under `specs/027-jitter-probe/results/`; kept out of CI because the fit limits a user meets are the machine's |

**TDD order** (Principle III): the preset test lands first, the pure resolver needing no probe; then the probe test, the loop and its seams; then the window test, the split over a stopped probe; then the cli test, the option table over a whole parse. Each code task is gated by the red run of the test task that covers it, observed failing against the unfixed baseline before the code that turns it green exists.

**Test rules** (FR-039, FR-040, the Constraints):

- No sleep and no clock in any test: the time series is scripted, start and stop ride barriers, and every branch runs on every host.
- No fixed cpu number and no host-size assumption: cpus come from `sched_getaffinity`, each below the online count; the suite passes on a 4-cpu CI runner and on a 2-cpu host, and no test skips. The host check in `source/harness/execution_context.cpp` does not weaken.
- The no-allocation proof follows the pattern the harness tests carry (Q-2): the file writes its own file-local replaceable `operator new`/`operator delete` counting hooks, in the shape of `test/source/harness_capture_test.cpp:33-44`, and diffs the count around the region under test; a negative test plants one allocation and sees the count move, so a proof that inspects nothing cannot pass.

**Gates each wave runs** (Principle VIII): `ctest -j 2`; the coverage build with the 100 percent line and branch rule; `test/time_source_gate.sh` (the counters-only rule, FR-004); the docs drift set (`example_drift`, `snippet_compile`, `example_run`, `report_shape`); `tools/dbc/dbc_pair_gate.py`; the prose gate; and, once per feature, the `ci-ubuntu` release build with clang-tidy and cppcheck wired in, beside the `ci-sanitize` and `ci-tsan` builds.

Coverage note: every new branch of `jitter_probe.cpp`, `placement_preset.cpp`, `cli.cpp`, `report.cpp`, `json_report.cpp` and `json_load.cpp` must stay reachable from a test. The scripted series drives the gap detection and the buffer-full branches; the synthetic layouts drive the idle, overlap and mask branches; the option grammar drives the status-2 branches; and the fake provider's `machine/tsc` leaf drives the availability branches on every host, the case the real catalog cannot drive on a build that carries the leaf.

## Constitution Check

*(Post-design pass: does the design as written honor I to XI?)*

| Principle | Verdict and note |
| --- | --- |
| I | PASS. The standard C++23 surface carries the feature. The designated critical path is the probe loop (D-7), and its P0 techniques stand recorded at R-03 and R-04: the fixed-stride record write, the single-flag read on its own cache line, and the spin wait through the shipped pause-hint shape. No intrinsic header and no compiler extension join: the TSC read rides the counters provider. |
| II | PASS. The five public names carry `\pre`/`\post`/`\invariant` contracts in the table above, and the internal holder, resolver, overlap check, split and fold carry `SG_REQUIRE`/`SG_ENSURE` pairs at their seams; the dbc-gate runs before each push. |
| III | PASS. FR-001 to FR-043 map to R-01 to R-15, every task of `tasks.md` cites a requirement id, a success criterion or the constitution clause it serves, and each test task names the red run it observes. |
| IV | PASS. The README section, the detail page, the quick-reference rows and the runnable example ship in the same lane, held by the drift gate and by `example_run` (IV.1). |
| V | PASS. The naming rules hold at every new name (R-15), and clang-format, clang-tidy and cppcheck stay clean at the feature head with no suppression. |
| VI | PASS. 100 percent line and branch coverage on new paths with no exclusion marker; the scripted series, the synthetic layouts and the option grammar drive every branch, on every host, with no skip. |
| VII | PASS. The probe starts before the first benchmark and stops after the last; the fold and the percentiles run after the stop; the disclosure reads no clock; the loop minimizes branches and memory touches; the acceptance script, and no claim in this plan, carries the overhead number (FR-042, D-8). |
| VIII | PASS. Every hard gate runs at the feature head; no gate loses a check; the one budget figure, `PAGE_LINE_CAP`, moves by the measured amount through the recorded decision the spec's Budget note names. |
| IX | PASS. spec, plan and research stand; `tasks.md` lands with `/speckit.tasks` before the first code task; the four artifacts of the workflow stand complete. |
| X | PASS. One probe unit, one pure resolver, one mode enum over a closed value set; the pin rides the shipped placement path, the time rides the counters surface, the rule names ride the N5 parser; the one new seam carries the justification R-02 records. |
| XI | PASS for the tracked tree, pending for these artifacts: the gate walks `git ls-files`, and these artifacts stand untracked at this point; the branch's prose gate run covers them at the feature head. |

The design adds no target and no dependency; the probe touches no measured window; the JSON move rides version 1 of the document with the schema updated in the same commit (R-09); the parser integration waits for PR #45 and copies nothing (R-12, FR-033).

## Complexity Tracking

No violation to justify: the new public types are the feature's own surface (FR-003, FR-015), no new target joins, and no new dependency joins.
