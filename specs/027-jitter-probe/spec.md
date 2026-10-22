# Feature Specification: Jitter Probe

**Feature Branch**: `027-jitter-probe`

**Created**: 2026-10-21

**Status**: Draft

**Input**: User description, opening quoted, the full text held by the
specify invocation record:

"Built-in jitter probe, item N13a of the roadmap
`/workspace/speedgun-ng-harness-roadmap.md` (section 10). Short name:
jitter-probe. Target machine: thrawn (Threadripper 9960X, Zen 5, 24
cores, 48 threads, one socket, several CCDs with one L3 each, 128 GB).
Lane: placement and noise (N4 done, N13a now, then N13b, N12, N13e). It
runs in parallel with the N5 fixes (gary, PR #45) and the N6 fixes
(corsairone, PR #44). Goal: every benchmark or test binary built with
speedgun can deploy a jitter-probe execution context. It is a built-in
harness capability. A spinning context, pinned through the N4 placement
on chosen cpus, reads the time-stamp counter in a tight loop and records
each gap above a threshold (the sysjitter method). The harness gives the
result for each probe cpu in the console report and the JSON report,
splits the gaps into each benchmark's timed windows, and gives N5 rule
names, so CI can fail a run on noise. [...] The full text carries the
run-time binding, the version rule, the parallel-work and rebase rules,
the hard rules, preconditions PC-1 to PC-7, capabilities C-1 to C-9,
recommendations D-13.1 to D-13.9 with design items R-13.1 to R-13.5,
the console and JSON shapes, the unit-test and example rules, acceptance
criteria AC-1 to AC-8, and the scope."

## Run-time binding

- **Spec number**: 027, the next free number under `specs/` on master at
  the audit point. Master carries `specs/` up to 023 plus
  `specs/025-topology-placement`. The number 024 lives on
  `origin/024-multipass-collection` (PR #44, the N6 fixes) and the number
  026 lives on `origin/026-regression-gate` (PR #45, the N5 fixes), so
  both stand reserved by their open pull requests. The `gh` CLI is
  present on this machine and unauthenticated, so `gh pr list` answers
  with an auth prompt and no list; the open-PR state above comes from
  the two remote branches, which stand at the tips the pull requests
  review. 027 is free, and this feature records why it takes it.
- **Branch base**: `aec8fcef6a3664ec311890930b65ea20541c0fa7`, the tip
  of `master` at run time (2026-10-21), equal to the tip of
  `origin/master` after `git fetch origin --prune`. Master carries
  version 0.14.0, harness `SOVERSION` 8, archive `SOVERSION` 2
  (`CMakeLists.txt` lines 7, 247, 47). The feature rebases onto the
  master tip current at its rebase time and records that commit in the
  audit point row of the rebase note.
- **Version reservation**: PR #45 (N5) merges first with 0.15.0 and
  harness `SOVERSION` 9; PR #44 (N6) merges second with 0.16.0 and
  harness `SOVERSION` 10, archive `SOVERSION` 3. N13a reserves 0.17.0.
  When both land before this feature rebases, the harness `SOVERSION`
  rises to 11. Each rebase re-reads the `VERSION` and `SOVERSION` lines
  of `CMakeLists.txt` on master, takes the next free minor version, and
  raises each `SOVERSION` by one from master, and the constitution rule
  governs (Decisions D-16). The rebase landed on `bd10d76` (0.16.0,
  harness 10, archive 3) and records the ABI check the lane owner
  asks for: the public surface gains `runJitterProbe` and
  `BenchmarkHandle::jitterProbe`, so the harness `SOVERSION` rises to
  11 and the archive, which carries the harness objects, rises to 4;
  with the public surface unchanged the versions would have stood
  still at master's.
- **Roadmap file**: the roadmap path
  `/workspace/speedgun-ng-harness-roadmap.md` held no file on this
  machine at run time. The prompt carries the decisions D-13.1 to
  D-13.9 and the design items R-13.1 to R-13.5 in full, and this spec
  records them under Decisions; the `sysjitter` field order comes from
  the capability list C-1 of the prompt (Assumptions).
- **Merged lane state**: N4 (topology placement) is merged as
  `specs/025-topology-placement`. N3 (JSON reports) is merged as
  `specs/023-json-reports`. N5 and N6 stand open as the pull requests
  named above.

## Clarifications

### Session 2026-10-21

- **Q-1 The provider's availability answer**: Clarified 2026-10-21:
  master publishes no runtime invariant-TSC detection. The `tsc` catalog
  leaf is omitted at build time on builds without the time-stamp
  instruction (`#ifdef SG_COUNTERS_X86`,
  `source/counters/clock_provider.cpp:271-293`); the class comment at
  `include/speedgun-ng/counters_clock.hpp:32-33` records the omission as
  a catalog fact with no API difference. No `Availability` enumerator
  describes the absence (`include/speedgun-ng/counters_core.hpp:87-95`),
  and `ClockProvider::open` returns a null reader for a `machine/tsc`
  request on such a build (`clock_provider.cpp:317-321`), a recoverable
  error. The probe performs the availability check itself: it checks the
  provider catalog for the `machine/tsc` leaf, and when the leaf is
  missing or the open returns null, it discloses "not available" for the
  probe and the run continues with status 0; with `,strict` on the
  option value the process exits with status 2. Recorded at FR-037,
  FR-004, D-10 and PC-3. Clarified 2026-10-22: the null-reader
  disjunct went, with the proof the K-8 review wave records. The open
  refuses a `machine/tsc` request only on a build without the
  time-stamp instruction (`kTscAvailable`,
  `source/counters/clock_provider.cpp`), and `enumerate` publishes the
  leaf under the same macro, so the catalog that publishes the leaf is
  the catalog whose open answers a reader: one build fact governs
  both, and the check asks the catalog alone.
- **Q-2 The allocation counter AC-5 rides on**: Clarified 2026-10-21:
  master carries no shared allocation counter in the harness library.
  Each harness test binary carries its own file-local replaceable
  `operator new`/`operator delete` counting hooks and diffs the count
  around the region under test
  (`test/source/harness_capture_test.cpp:33-44`,
  `test/source/counters_noalloc_test.cpp:29-49`; four more listed in
  `.omo/waves/027-clarify/q-resolve.md`). The probe-loop test writes its
  own counting pair in that pattern and proves the probe loop takes no
  allocation. Recorded at FR-041 and SC-005.

## Decisions

Charles accepted these recommendations with the prompt on 2026-10-21.
They are governance for this feature; each lands in the requirement
named beside it.

- **D-1 Modes (D-13.3, R-13.1).** `alongside` is the default mode when
  `--jitter-probe` is given. With no `--jitter-probe` option, there is
  no probe, and the console report and the JSON of the run carry the
  same fields and shape as master's (FR-013). `precheck` runs the probe
  only before the first benchmark. `both` runs both (FR-012 to FR-014).
- **D-2 Threshold (D-13.1).** The default threshold is 200 ns, or 10
  times the measured minimum loop period when that is larger. The probe
  measures the loop period before its window and discloses the loop
  period and the threshold. `threshold=<ns>` sets the value; a value of
  0, a negative value, or a value that is not a number exits with
  status 2 (FR-007, FR-008). Corrected at the PR #46 review: an
  explicit `threshold=200` sets exactly 200 ns - the default rule
  applies only when the option names no threshold value, and the
  library type carries the absence (`JitterProbeOptions::thresholdNs`
  is `std::optional<std::uint64_t>`) so no in-range value can stand in
  for it (FR-009, R-05).
- **D-3 Overlap (D-13.2, R-13.2).** In `alongside` mode, a probe cpu
  that is a planned cpu of a benchmark context, or an SMT sibling of
  one, is a usage error: one line naming the cpu and the benchmark, and
  exit status 2. In `precheck` mode, the probe may use the benchmark
  cpus, because it stops before they start. In `both`, the alongside
  phase carries the usage error and the pre-check phase carries the
  allowance (FR-018).
- **D-4 The `idle` preset (R-13.2).** One probe cpu for each L3 group
  the benchmark plans use, taken from the cpus no plan uses and that
  are not SMT siblings of a planned cpu, the lowest such cpu in each
  group. A group with no free cpu is skipped and disclosed. When no cpu
  is free at all, `idle` is a usage error with status 2 (FR-016,
  FR-017).
- **D-5 Unpinned benchmarks (R-13.2).** When a benchmark carries no
  placement, the harness removes the probe cpus from the benchmark's
  affinity mask for the run and discloses the change. A benchmark that
  carries a preset with a single context is unpinned (the runner pins
  only benchmarks with more than one context), so the removal covers it
  too. When the mask empties, the process exits with status 2
  (FR-020).
- **D-6 Gap buffer (D-13.4, R-13.3).** The buffer is pre-sized to
  65,536 gaps for each probe cpu, 16 bytes each, allocated and first
  touched on the probe cpu before the probe window. `gaps=<n>` sets the
  size in the range 1 to 16,777,216; a value outside the range exits
  with status 2. When the buffer fills, the probe counts gaps, their
  total, and their max, drops the time stamps, and discloses
  `droppedGaps`. The window split and the percentiles use only the kept
  gaps, and the report says so (FR-009, FR-010).
- **D-7 The loop (R-13.3).** No allocation, no syscall, and no I/O in
  the probe loop. The loop reads the TSC, compares, writes one gap
  record when a gap lands, and reads one stop flag on its own cache
  line. The probe starts before the first benchmark and stops after the
  last, outside every timed window. The fold and the percentiles run
  after the probe stops (FR-005, FR-006).
- **D-8 Overhead disclosure (R-13.3).** The report states that a probe
  was running and on which cpus. The acceptance script measures the
  benchmark time with and without the probe and records it in the spec
  results. N13a changes no benchmark measurement (FR-025, FR-041).
- **D-9 Same-core ban (R-13.4).** Probing a cpu a benchmark context is
  planned onto is refused in `alongside` mode (D-3). The noise on the
  measured cpus comes later from N13b (interrupt and context-switch
  deltas) (FR-018).
- **D-10 Time source (R-13.5).** The TSC through the `ClockProvider`
  and the existing calibration. A host without an invariant TSC, or
  without the TSC provider, discloses "not available" for the probe and
  continues with status 0; with `--jitter-probe=...,strict` the process
  exits with status 2. Master publishes no runtime invariant-TSC
  detection: the `tsc` catalog leaf is omitted at build time on builds
  without the time-stamp instruction, and `ClockProvider::open` refuses
  a `machine/tsc` request on exactly such a build. The probe checks the
  catalog for the leaf, and the catalog's answer carries the open's
  (FR-037, Clarifications Q-1).
- **D-11 Rule names (D-13.7).** `jitter:<cpu>:<field>` and
  `jitter:any:<field>`, the worst cpu for that field, with the field
  names of FR-002 in camelCase; for example
  `--compare-rule=jitter:any:p99Ns:lower:abs=5000`. A `jitter:` rule on
  a benchmark names the per-benchmark values by default;
  `jitter:run:` names the whole-run values. A `jitter:` rule on a
  report with no probe exits with status 2 (FR-030 to FR-032).
- **D-12 Privilege (D-13.5).** N13a needs no root and changes no
  kernel setting. The probe discloses when cpusets limit the probe
  cpus (FR-036).
- **D-13 Pre-check scope (D-13.6).** `precheck` runs the probe on all
  chosen cpus at the same time for 200 ms; `precheck=<ms>` sets the
  duration. The full pre-check with warnings and attribution is N13b
  (FR-014).
- **D-14 One spec (D-13.8).** N13a is one spec. N13b to N13e receive
  their own specs later (Scope).
- **D-15 FWQ and FTQ out (D-13.9).** The FWQ/FTQ probes and their
  spectrum are N13c, outside this feature (Scope).
- **D-16 Version and merge order.** Charles set the merge order on
  2026-10-21: PR #45 lands first, PR #44 second, and N13a reserves
  0.17.0. Each rebase re-reads `CMakeLists.txt` on master and takes the
  next free minor version; each `SOVERSION` rises by one from master.
  A change in the order takes the next free value, and the rebase note
  records why (Run-time binding). The review of PR #46 corrects the
  rule's application: a `SOVERSION` rises only when that archive's
  public ABI changes, and the check stands recorded here. The harness
  ABI changes - `JitterStats`, `JitterEntryFold`, `JitterProbeMode`,
  `JitterProbeOptions` and `runJitterProbe` join the surface,
  `BenchmarkResult` gains `jitterFolds`, `ReportData` gains
  `runJitterStats`, `RawContent` gains `GAPS`, and `BenchmarkHandle`
  gains the `jitterProbe` member (every name read in
  `include/speedgun-ng/benchmark.hpp`) - so the harness `SOVERSION`
  rises to 11. The archive's only counters change is the fake
  provider's lock scope, which names no public symbol, so the archive
  `SOVERSION` stays at master's 3. The audit table's rise rows stand
  as the plan-time record; this paragraph carries the applied check.

## Audit point

| Field | Value |
| --- | --- |
| Audit point | `aec8fcef6a3664ec311890930b65ea20541c0fa7` |
| Audit-point date | 2026-10-21 (commit date 2026-10-20) |
| Default branch at run time | `master`, tip `aec8fce`, equal to the `origin/master` tip after `git fetch origin --prune` |
| Working tree at run time | no tracked-file changes; 28 untracked scratch files from the specs/025 acceptance runs, `cli-placement-*.txt` pairs; they carry no code this feature reads |
| Feature number at run time | 027, the next free number under `specs/`; 024 and 026 stand reserved by the open pull requests #44 and #45 (Run-time binding) |
| Short name at run time | `jitter-probe` |
| Git branch at run time | none created: `.specify/extensions.yml` is absent, so this command created the directory on `master` and the orchestrator owns every commit |
| Project version at the audit point | 0.14.0 (the `project()` `VERSION` in `CMakeLists.txt` line 7) |
| Harness `SOVERSION` at the audit point | 8 (`CMakeLists.txt` line 247); rises to 9 with PR #45 and to 10 with PR #44; this feature takes 11 from that master, 0.17.0 with it (D-16) |
| Archive `SOVERSION` at the audit point | 2 (`CMakeLists.txt` line 47); rises to 3 with PR #44 (D-16) |
| Main page at the audit point | `README.md` at 713 lines, the drift gate's `PAGE_LINE_CAP` 713 (`tools/docs/example_drift.py`), equal; the new section moves the cap through a recorded decision (Budget note) |
| Constitution version at the audit point | 2.20.0, lineage row and footer equal |

Every citation below was read at that content.

## Preconditions

### PC-1: execution contexts. PASS.

`include/speedgun-ng/execution_context.hpp` carries `WaitMode` with
`SPIN`, `YIELD`, and `BLOCK` (lines 91 to 95), `kNoPin` (line 100),
`PlacementSlot` with `cpuIndex` and `noPin()` (lines 116 to 125),
`ContextPolicy` (line 150), `PlacementFn` (line 184), and
`setPlacementFunction` (line 201). `source/harness/execution_context.cpp`
carries the pin path `pinToPlacementSlot` (line 106), the `NO_PIN`
account text (line 79), and the placement registry `placementSlot()`
(line 127). The harness starts and stops a context outside the timed
window through this surface, and the probe pins its spinning contexts
through the same slot and function (FR-021).

### PC-2: the N4 plan and layout. PASS.

`source/harness/placement_preset.cpp` carries the preset table and the
resolver: `Preset` (line 38), `presetText` (line 75), and
`resolvePreset(const Preset, ..., const int contexts) -> PlacementPlan`
(line 83). `source/harness/topology.cpp` carries the hwloc layout with
the L3 groups, the cores, and the SMT siblings. The test seams for a
synthetic layout live in `source/harness/detail/internal.hpp`, the
harness-internal install calls specs/025 Q-5 records, and no public
header carries them. The probe reads the plan and the layout and
changes no preset rule (FR-021).

### PC-3: the TSC read and the fake provider. PASS.

`include/speedgun-ng/counters_clock.hpp` carries `ClockProvider`
(line 62), a provider whose catalog documents the `tsc` read mode and
the `machine/tsc` leaf (lines 29 to 53). The clock calibration runs in
the counters plan on first request (`source/counters/plan.cpp:264`,
`calibrate`; `source/counters/detail/core.hpp:140` holds the state).
The fake provider for scripted time is the installed header
`include/speedgun-ng/counters_fake.hpp`, and
`test/source/counters_fake_test.cpp` covers it. The TSC read shape
carries its own gate, `test/counters_tsc_read_shape.sh`. The `tsc`
catalog leaf is omitted on builds without the time-stamp instruction,
and the provider publishes no availability status for the absence; the
probe takes its availability answer from the catalog and the null
reader (Clarifications Q-1). The probe
reads time only through this surface (FR-004).

### PC-4: percentiles. PASS.

`source/harness/percentiles.cpp` carries `percentileAt` (line 104) with
the nearest-rank rule (line 115), and the report columns fill through
it (line 155). The probe's p90, p99, p99.9, p99.99, median, and fold
reuse this rule (FR-002).

### PC-5: the timed windows. PASS.

`source/harness/runner.cpp` owns the repetition loop and the chunk
capture: the chunk values one repetition holds (lines 152 to 158), and
the per-run capture of chunk values behind the percentile record (lines
252 to 273). Each repetition and each chunk opens and closes a timed
window with a start and a stop, so the probe can split gaps into
windows by time stamp. On `origin/024-multipass-collection`, N6 adds
the multi-pass loop with `BenchmarkResult::metricMethod`
(`benchmark.hpp:528`), `passCount` (line 533), and `passSpread`
(line 539), and each pass carries its own timed windows; the split
shall cover all of them (FR-022).

### PC-6: NoiseWarning. PASS.

`source/harness/detail/json_report.hpp` carries `NoiseWarning` at line
137, with the account `printNoiseWarning` reaches it through at line
133 and the two-spinning-contexts condition at line 148;
`source/harness/report.cpp:193` carries `printNoiseWarning`, and line
516 calls it. N13a adds to the disclosure block and replaces nothing
(FR-025).

### PC-7: the N5 value-name parser. PASS, on the PR branch.

`origin/026-regression-gate` carries
`parseCompareRule(std::string_view)` as public API
(`include/speedgun-ng/benchmark.hpp:2500`) with the implementation in
`source/harness/compare.cpp:722`, the option `--compare-rule` with the
short letter `Q` (`source/harness/cli.cpp:315`), and
`compare-strict` with the short letter `S` (line 316). N13a adds the
`jitter:` value names through this parser after PR #45 merges, and the
branch waits for N5 and rebases (FR-033).

## Roadmap position

The lane is placement and noise. N4 (topology placement) is merged as
specs/025. N13a, this feature, adds the built-in jitter probe. N13b
adds the host pre-check with warnings and `/proc` attribution. N12 adds
NUMA memory-node placement. N13e adds the osnoise and timerlat probes.
N13c adds the FWQ/FTQ probes and their spectrum, and N13d adds the
hardware latency probe and the SMI counts; both lanes run after the
features the roadmap orders before them.

## Parallel work: two open pull requests and the shared lists

- PR #45 (N5, `origin/026-regression-gate`, spec 026) adds the
  comparison gate: `compare.cpp`, `parseCompareRule`,
  `compareReports(..., const CompareOptions&)`, the options `Q` and
  `S`. N13a adds the `jitter:` value names to the N5 parser after N5
  merges. When N13a is ready before N5 merges, the branch waits for N5
  and rebases (FR-033).
- PR #44 (N6, `origin/024-multipass-collection`, spec 024) adds the
  multi-pass runner loop, `BenchmarkResult::metricMethod`,
  `passCount`, `passSpread`, and new JSON fields. A multi-pass run
  gives each pass its own timed windows, and the probe splits its gaps
  into all of them (FR-022). N6 also takes the short letter `S`; the
  collision resolves on the N6 side, and N13a takes no letter.
- The short option values of master are `f l r m i w d c M C p P A T j
  J` (`source/harness/cli.cpp:294` to `:312`). N13a takes no letter;
  its long options use distinct values of 256 or more, held as named
  constants, and a test asserts every value in the option table is
  unique (FR-011).
- The shared lists this feature touches: the `VERSION` and `SOVERSION`
  lines of `CMakeLists.txt`, `README.md`, the quick-reference table,
  `PAGE_LINE_CAP` and `SECTIONS` in `tools/docs/example_drift.py`,
  `example/CMakeLists.txt` and the example run list, the
  `test/CMakeLists.txt` lists, the option table in
  `source/harness/cli.cpp`, `include/speedgun-ng/benchmark.hpp`, and
  `docs/schema/report-v1.json`.
- On rebase conflicts: rebase onto the new master (no merge commit),
  keep the lines of all features in each shared list, take the next
  free minor version, set `PAGE_LINE_CAP` to the measured length
  through a recorded decision, run the full gate set, and push one time
  with `--force-with-lease`.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Deploy a probe alongside a run (Priority: P1)

An engineer runs a benchmark suite on thrawn and suspects the host adds
noise the benchmark numbers absorb. The engineer adds
`--jitter-probe=idle` to the standard command line. The harness pins
one spinning context per L3 group the plans leave free, runs the probe
from before the first benchmark to after the last, and prints a probe
block after the benchmark tables: one column per probe cpu, the fields
of the sysjitter order. The JSON report of the run carries the same
result as a `jitter` object, and a reader loads the report back with
equal values.

**Why this priority**: the probe exists to answer one question, how much
noise the host adds during a run, and this story is that answer.

**Independent Test**: on a host with 4 or more cpus, one benchmark run
with `--jitter-probe=<cpu>` prints the probe block and writes the
`jitter` JSON object; `loadReport` reads the file back with equal
values.

**Acceptance Scenarios**:

1. **Given** a host with 4 or more cpus, **When** a run carries
   `--jitter-probe=<cpu>`, **Then** the console report prints the probe
   block after the benchmark tables and the JSON report carries the
   `jitter` object.
2. **Given** the JSON file of that run, **When** `loadReport` reads it,
   **Then** every `jitter` field reads back equal to the value the run
   wrote.
3. **Given** a run with no `--jitter-probe` option, **When** the run
   finishes, **Then** the console report and the JSON carry the same
   fields and shape as master's (D-1).

### User Story 2 - Place the probe safely (Priority: P2)

An engineer asks for the `idle` preset and the harness picks one probe
cpu per L3 group from the cpus the plans leave free. When the engineer
instead names a cpu a benchmark context is planned onto, or an SMT
sibling of one, the process refuses with one line naming the cpu and
the benchmark. The engineer never sees a probe share a core with a
measured context in `alongside` mode.

**Why this priority**: a probe that shares a core with the measured
work corrupts both the measurement and the probe; the safety rules are
what make the numbers trustworthy.

**Independent Test**: with a synthetic layout from the N4 seams, the
`idle` preset resolves one cpu per group from the free cpus; an
explicit overlap in `alongside` mode exits with status 2 and one line.

**Acceptance Scenarios**:

1. **Given** a plan that uses cpus of two L3 groups, **When** the run
   carries `--jitter-probe=idle`, **Then** each group contributes the
   lowest free cpu that no plan uses and no SMT sibling of a planned
   cpu names.
2. **Given** a group with no free cpu, **When** the probe resolves,
   **Then** the probe skips that group and the report discloses it.
3. **Given** no free cpu at all, **When** the run carries
   `--jitter-probe=idle`, **Then** the process prints one line and
   exits with status 2.
4. **Given** a probe cpu that is a planned cpu or an SMT sibling of one
   in `alongside` mode, **When** the run starts, **Then** one line
   names the cpu and the benchmark and the process exits with status 2.
5. **Given** the same overlap in `precheck` mode, **When** the run
   starts, **Then** the probe runs on that cpu, because it stops
   before the benchmark starts (D-3).

### User Story 3 - Attribute noise to each benchmark (Priority: P2)

An engineer runs a suite where one benchmark is noisy and the rest are
clean. The harness splits the recorded gaps of each probe cpu into each
benchmark's timed windows by time stamp, so each benchmark carries its
own `JitterStats` for each probe cpu, and the console report prints one
line per benchmark naming the worst cpu. A multi-pass run splits the
gaps into every pass's windows.

**Why this priority**: whole-run numbers hide which benchmark absorbed
the noise; per-benchmark attribution is the finding an engineer acts on.

**Independent Test**: with a scripted time series from the fake
provider, a two-benchmark run where the gaps land inside one
benchmark's window yields stats for that benchmark only.

**Acceptance Scenarios**:

1. **Given** gaps that land inside one benchmark's timed window,
   **When** the run reports, **Then** that benchmark's `jitter` entry
   carries the stats and the other benchmark's entry carries none.
2. **Given** a multi-pass run (N6), **When** the run reports, **Then**
   each pass's windows hold the gaps that landed inside them.
3. **Given** a benchmark with gaps above the max the buffer kept,
   **When** the report prints, **Then** the benchmark line reads
   `jitter: worst cpu <n> gaps=<k> max=<ns> total=<percent>`.

### User Story 4 - Fail a run on noise from CI (Priority: P3)

After N5 merges, an engineer adds
`--compare-rule=jitter:any:p99Ns:lower:abs=5000` to a comparison run.
When the p99 of the whole-run probe moves more than 5000 ns, the
comparison fails with status 1. A `jitter:` rule against a report that
carries no probe is a usage error with status 2.

**Why this priority**: the gate is the reason the numbers reach CI, and
it stands on the parser N5 ships.

**Independent Test**: two reports where one carries probe stats and a
rule names a field, compared through the N5 parser; a probe-free
report with a `jitter:` rule exits with status 2.

**Acceptance Scenarios**:

1. **Given** two reports whose whole-run `p99Ns` differ by more than
   the rule's bound, **When** the comparison runs, **Then** it fails
   with status 1.
2. **Given** a report with no probe and a `jitter:` rule, **When** the
   comparison parses the rule, **Then** the process prints one line and
   exits with status 2.
3. **Given** a per-benchmark rule with no `run` prefix, **When** the
   comparison evaluates it, **Then** the rule reads that benchmark's
   per-benchmark values (D-11).

### User Story 5 - Learn and verify from the docs (Priority: P3)

A reader opens the README, reads the "Jitter probe" section, copies the
snippet's example, and runs it; the example runs one benchmark
with `--jitter-probe=idle,mode=precheck` through the library, and CI
runs it on every push. The detail page `docs/pages/harness.md` carries
the full option table and the field reference.

**Why this priority**: the docs gate (IV.1) binds the feature, and the
example is the runnable proof the page teaches something that runs.

**Independent Test**: the drift gate compares the page block with the
example region; `example_run` executes the example and fails on a row
that never prints.

**Acceptance Scenarios**:

1. **Given** the README section, **When** the drift gate runs, **Then**
   the block equals the example region it quotes and the section stays
   within 25 lines.
2. **Given** the example target, **When** `example_run` runs it,
   **Then** the probe block prints and the `.expected` file checks the
   fields that carry no host dependence: the block header, the field
   names, and the cpu count rule.

### Edge Cases

- The gap buffer fills: the probe keeps counting gaps, their total,
  and their max, drops the time stamps, sets `droppedGaps`, and the
  report states that the split and the percentiles use only the kept
  gaps (D-6).
- The host carries no invariant TSC or no TSC provider: the report
  discloses "not available" for the probe, the run continues with
  status 0; with `--jitter-probe=...,strict` the process exits with
  status 2 (D-10).
- Cpusets limit the probe cpus: the report discloses the limit (D-12).
- A benchmark carries no placement: the harness removes the probe cpus
  from the benchmark's affinity mask and discloses the change; an empty
  mask exits with status 2 (D-5).
- `gaps=0` or a value above 16,777,216: one line, status 2 (FR-009).
- `threshold=0`, a negative value, or a non-number: one line, status 2
  (FR-008).
- An unknown mode, an unknown preset name, a bad cpu list: one line,
  status 2 (FR-040).
- A two-cpu host and a four-cpu CI runner: every test passes on both
  and none skips (FR-040).

## Requirements *(mandatory)*

### Probe API (C-1)

- **FR-001**: The harness shall provide the jitter probe as a built-in
  capability of every benchmark and test binary that links the harness:
  one spinning execution context for each probe cpu, pinned through the
  N4 placement, reads the time-stamp counter in a tight loop and
  records each gap above the threshold (the sysjitter method). No
  separate tool shall carry the capability.
- **FR-002**: The probe shall produce, for each probe cpu, one
  `JitterStats` result holding the sixteen fields data-model.md E-03
  carries, in the order that table is the canonical enumeration of.
  The percentiles shall follow the nearest-rank rule of
  `source/harness/percentiles.cpp` (PC-4).
- **FR-003**: The probe's public entry points shall take one options
  struct, `JitterProbeOptions`, and no bool parameter.
- **FR-004**: The probe shall read time only through `sg::counters`:
  the `ClockProvider` TSC read mode and the calibration the counters
  plan runs (PC-3). The probe takes its availability answer from the
  provider catalog (FR-037, Clarifications Q-1). The time-source gate
  (`test/time_source_gate.sh`) shall stay green.
- **FR-005**: The probe loop shall perform no allocation, no syscall,
  and no I/O. It shall read the TSC, compare, write one gap record when
  a gap lands, and read one stop flag that stands on its own cache
  line (D-7).
- **FR-006**: The probe shall start before the first benchmark of the
  run and shall stop after the last, outside every timed window of
  every benchmark (D-7).
- **FR-007**: The default threshold shall be 200 ns, or 10 times the
  minimum loop period the probe measures before its window, when that
  value is larger. The probe shall measure the loop period before its
  window and shall disclose the loop period and the threshold (D-2).
- **FR-008**: `threshold=<ns>` shall set the threshold. A value of 0, a
  negative value, or a value that is not a number shall print one line
  and exit with status 2; the negative value is a rule of the option
  value's grammar, where the string carries the sign, because the field
  carries `std::optional<std::uint64_t>`, an unsigned value, and the
  absence stands apart from every in-range value (D-2).
- **FR-009**: The gap buffer shall be pre-sized to 65,536 gaps for each
  probe cpu, 16 bytes each, allocated and first touched on the probe
  cpu before the probe window. `gaps=<n>` shall set the size, in the
  range 1 to 16,777,216; a value outside the range shall print one line
  and exit with status 2 (D-6).
- **FR-010**: When the buffer of a probe cpu fills, the probe shall
  keep counting the gaps, their total, and their max, shall drop the
  time stamps, and shall disclose `droppedGaps`. The window split and
  the percentiles shall use only the kept gaps, and the report shall
  state it (D-6).

### Harness integration and modes (C-2)

- **FR-011**: The standard main shall carry the option
  `--jitter-probe=<cpu-list|idle>[,threshold=<ns>][,mode=alongside|precheck|both][,precheck=<ms>][,gaps=<n>][,strict]`.
  The option shall take no short letter; its long-option values shall
  be distinct and 256 or more, held as named constants, and one test
  shall assert every value in the option table is unique.
- **FR-012**: `alongside` shall be the default mode when the option is
  given with no `mode=` value (D-1).
- **FR-013**: With no `--jitter-probe` option, there shall be no probe,
  and the console report and the JSON of the run shall carry the same
  fields and shape as master's: the `report_shape` fixture test holds
  the console shape, a digit that differs folding equal and a renamed
  or missing column failing, the schema validation holds the JSON to
  the key set, key order and value kinds master's document carries,
  and the by-hand master-worktree diff of quickstart.md records the
  manual check (D-1).
- **FR-014**: `precheck` shall run the probe only before the first
  benchmark, on all chosen cpus at the same time, for 200 ms;
  `precheck=<ms>` shall set the duration. `both` shall run the probe
  alongside and as a pre-check (D-1, D-13).
- **FR-015**: The library shall expose the same capability: a
  suite-level call that runs the probe for the suite's run, and a
  `BenchmarkHandle::jitterProbe(...)` fluent form beside the handle's
  existing setters (`include/speedgun-ng/benchmark.hpp:1620` and
  below), both taking `JitterProbeOptions` (FR-003).

### Placement (C-3)

- **FR-016**: The `idle` preset shall give one probe cpu for each L3
  group the benchmark plans use, taken from the cpus no plan uses and
  that are not SMT siblings of a planned cpu, the lowest such cpu in
  each group (D-4).
- **FR-017**: A group with no free cpu shall be skipped and disclosed.
  When no cpu is free at all, `idle` shall print one line and exit with
  status 2 (D-4).
- **FR-018**: In `alongside` mode and in the alongside phase of
  `both`, a probe cpu that is a planned cpu of a benchmark context, or
  an SMT sibling of one, shall print one line naming the cpu and the
  benchmark and shall exit with status 2 (D-3, D-9). In `precheck` mode
  and in the pre-check phase of `both`, the probe may use the benchmark
  cpus, because it stops before they start (D-3).
- **FR-019**: When a benchmark carries no placement, the harness shall
  remove the probe cpus from the benchmark's affinity mask for the run
  and shall disclose the change; when that leaves no cpu, the process
  shall exit with status 2 (D-5).
- **FR-020**: The probe shall read the N4 placement plan and the
  layout, and shall change no preset rule. Tests shall take synthetic
  layouts from the N4 seams in `source/harness/detail/internal.hpp`
  (PC-2).

### Window split (C-4)

- **FR-021**: The harness shall split the recorded gaps of each probe
  cpu into each benchmark's timed windows by time stamp, so each
  benchmark carries its own `JitterStats` for each probe cpu (PC-5).
- **FR-022**: A multi-pass run shall give each pass its own timed
  windows, and the split shall cover all of them (PC-5, PR #44).

### Console report (C-5)

- **FR-023**: The console report shall print one probe block after the
  benchmark tables, one column for each probe cpu, listing the
  `JitterStats` fields in the order of FR-002.
- **FR-024**: Each benchmark shall print one line:
  `jitter: worst cpu <n> gaps=<k> max=<ns> total=<percent>`, naming the
  worst probe cpu for that benchmark's windows.
- **FR-025**: The report shall disclose that a probe was running and on
  which cpus, the loop period and the threshold, the `droppedGaps`
  condition, the skipped groups, the affinity-mask change, and the
  statement that the split and the percentiles use only the kept gaps.
  The disclosure record of data-model.md E-09 is the canonical
  enumeration of this content, and the `not available` answer of
  FR-037 and the cpuset limit of FR-036 join the same record. The
  disclosure extends the block `NoiseWarning` and
  `printNoiseWarning` own today and replaces nothing (PC-6, D-8).

### JSON report (C-6)

- **FR-026**: The run-level JSON shall carry a `jitter` object holding
  the mode, `thresholdNs`, `loopPeriodNs`, the cpus, the stats of each
  probe cpu, `droppedGaps`, and the disclosure text.
- **FR-027**: Each benchmark entry shall carry a `jitter` object
  holding the stats of each probe cpu over that benchmark's windows
  (FR-021).
- **FR-028**: No gap list shall appear in the JSON unless
  `--json-raw=gaps` is given; `gaps` shall join the values `chunks` and
  `none` the N3 option carries today
  (`source/harness/cli.cpp:309`, `:383`).
- **FR-029**: `docs/schema/report-v1.json` and `loadReport` shall carry
  the new fields, and round-trip tests shall read the written report
  back with equal values. The writer shall use
  `builder::string_builder`, the reader shall use the DOM error-code
  form, and one test shall cover each error code (Constraints).

### Comparison rule names (C-7)

- **FR-030**: The value names `jitter:<cpu>:<field>` and
  `jitter:any:<field>`, the worst cpu for that field, shall join the N5
  value-name parser, with the field names of FR-002 in camelCase
  (D-11).
- **FR-031**: A `jitter:` rule on a benchmark shall name that
  benchmark's per-benchmark values by default; the prefix
  `jitter:run:` shall name the whole-run values (D-11).
- **FR-032**: A `jitter:` rule evaluated on a report with no probe
  shall print one line and exit with status 2 (D-11).
- **FR-033**: This feature shall add the names through the N5 parser
  after PR #45 merges, and shall copy no parser code; when N13a is
  ready before N5 merges, the branch waits for N5 and rebases.

### Documentation (C-8)

- **FR-034**: The README shall carry a section "Jitter probe" of at
  most 25 lines, with its quick-reference rows, and shall add no entry
  to `SECTION_LINE_CAPS` (the default cap 25 applies). The detail page
  `docs/pages/harness.md` shall carry the full option table and the
  field reference.
- **FR-035**: `example/docs/jitter_probe.cpp` shall run one benchmark
  with `--jitter-probe=idle,mode=precheck` through the library, built
  as a `run_docs_` target with an entry in the example
  run list and an `.expected` file that checks only the fields with no
  host dependence: the block header, the field names, and the cpu count
  rule.

### Privilege, availability, and errors

- **FR-036**: The probe shall need no root privilege and shall change
  no kernel setting. The report shall disclose when cpusets limit the
  probe cpus (D-12).
- **FR-037**: The probe shall perform the availability check itself:
  it shall check the provider catalog for the `machine/tsc` leaf, and
  when the leaf is missing, the probe shall disclose "not available"
  for the probe and shall continue the run with status 0. With
  `--jitter-probe=...,strict` on the option value, the process shall
  exit with status 2. Master publishes no runtime invariant-TSC
  detection; the catalog carries the fact of a build without the
  time-stamp instruction, and no `Availability` enumerator describes
  the absence. The publication and the open's refusal answer to one
  build fact (`kTscAvailable`, `source/counters/clock_provider.cpp`),
  so the catalog's answer carries the open's, and the check asks the
  catalog alone (D-10, PC-3, Clarifications Q-1).
- **FR-038**: An invalid command-line option or value of this feature
  shall print one line and exit with status 2: a bad cpu, an unknown
  preset, an overlap in the alongside phase, a bad threshold, a bad
  mode, a bad `gaps` value, a bad `precheck` value, no free cpu for
  `idle`, and a `jitter:` rule on a probe-free report.

### Test seams (VI)

- **FR-039**: Tests shall script the time series and the gaps through
  the fake provider (`include/speedgun-ng/counters_fake.hpp`) or a
  probe seam, with barriers for start and stop, so every branch of the
  gap detection, the buffer-full path, the threshold, the window split,
  the `idle` preset, the overlap check, the unpinned-mask change, the
  modes, the option parse, the JSON write and read, and the rule names
  runs with no real noise, no sleep, and no clock.
- **FR-040**: No test shall hold a fixed cpu number or a host-size
  assumption. Cpus shall come from `sched_getaffinity`, each cpu shall
  stand below the online count, the tests shall pass on a 4-cpu CI
  runner and on a 2-cpu host, and no test shall skip. The host check in
  `source/harness/execution_context.cpp` shall not weaken.
- **FR-041**: The probe-loop test shall write its own file-local
  replaceable `operator new`/`operator delete` counting hooks, in the
  pattern of the harness tests
  (`test/source/harness_capture_test.cpp:33-44`), and shall prove the
  probe loop takes no allocation by diffing the count around the
  region under test. The harness library carries no shared allocation
  counter (AC-5, Clarifications Q-2).

### Acceptance runs (C-9)

- **FR-042**: `tools/noise/jitter_probe_thrawn.sh`, a script and not a
  CI gate, shall run on thrawn: N4-V, each N4 preset with `--threads`
  at 24 and at 48, printing the planned cpus and the L3 group of each;
  N13-V, `--jitter-probe=idle` with one probe per L3 group, the
  overlap error on a planned cpu, and the benchmark time with and
  without the probe (D-8).
- **FR-043**: The acceptance results shall be recorded under
  `specs/027-jitter-probe/results/` with the date, the kernel, the
  BIOS settings that are visible, and the `isolcpus` and `nohz_full`
  state.

## Key Entities *(include if feature involves data)*

- **JitterProbe**: the built-in probe. One spinning execution context
  per probe cpu, pinned through `PlacementSlot` and `PlacementFn`
  (PC-1), reading the TSC through `ClockProvider` (PC-3), recording
  gaps above the threshold into a pre-sized buffer (D-6), started
  before the first benchmark and stopped after the last (D-7).
- **JitterProbeOptions**: the options struct of every public entry
  point (FR-003): the cpu list or `idle`, the threshold, the mode, the
  gap buffer size, the pre-check duration, the strict flag.
- **JitterStats**: the result of one probe cpu, the sixteen fields of
  data-model.md E-03 in the E-03 order.
- **Gap record**: one kept gap, 16 bytes: the time stamp of the read
  and the gap in ns, dropped when the buffer fills (D-6).
- **Run `jitter` object and benchmark `jitter` object**: the JSON
  shapes of FR-026 and FR-027, carried by the schema and read back by
  `loadReport` (FR-029).

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001** (AC-1): on a host with 4 or more cpus,
  `--jitter-probe=<cpu>` prints the probe block and writes the
  `jitter` JSON object, and `loadReport` reads the file back with
  equal values.
- **SC-002** (AC-2): a bad cpu, an unknown preset, an overlap in the
  alongside phase, a bad threshold, a bad mode, a bad `gaps` value,
  a bad `precheck` value, or no free cpu for `idle` prints one line
  and exits with status 2.
- **SC-003** (AC-3): a run without `--jitter-probe` gives the same
  console report as master, and its JSON bytes are the same apart from
  the `schemaVersion` value.
- **SC-004** (AC-4): an N5 rule on `jitter:any:maxNs` fails a
  comparison with status 1, and a `jitter:` rule on a report with no
  probe exits with status 2.
- **SC-005** (AC-5): the probe loop performs no allocation and no I/O,
  proven by the test of FR-041, which writes its own counting
  operator-new pair in the pattern of the harness tests.
- **SC-006** (AC-6): 100 percent line and branch coverage with no
  exclusion, and the dbc-gate, consumer-release, test-rocky, tsan, and
  sanitize jobs pass.
- **SC-007** (AC-7): the README section stays at 25 lines or fewer, the
  example runs in CI, and the drift gate passes.
- **SC-008** (AC-8): the thrawn acceptance results of FR-042 and
  FR-043 are recorded under `specs/027-jitter-probe/results/`.

## Constraints

- **Coverage**: 100 percent line and branch coverage. No new
  `LCOV_EXCL` marker, waiver, residual row, "PARTIAL" row, or "OPEN"
  row. No code shape exists to move coverage counts. Coverage uses
  `lcov --filter brace`; no code shape removes brace counts. When a
  gap stays, the spec results record the cause with `gcov -b` evidence.
- **Tests**: no sleep and no clock in tests. The probe loop takes an
  injectable time source; tests script the time series and the gaps
  through the fake provider or a probe seam, and use barriers for
  start and stop (FR-039).
- **Counters-only**: the probe reads time only through `sg::counters`
  (the TSC `ClockProvider`, `counters_clock.hpp`). The time-source gate
  stays green (FR-004, Additional Constraints).
- **Host shapes**: no fixed cpu numbers and no host-size assumptions
  in tests; the host check in `execution_context.cpp` does not weaken
  (FR-040).
- **Errors**: an invalid command-line option or value prints one line
  and exits with status 2 (FR-038).
- **Docs**: the README section is at most 25 lines and adds no entry to
  `SECTION_LINE_CAPS`; the snippet comes from an example CI builds and
  runs; the snippet region stays small, with no include lines in the
  region where the snippet gate permits (FR-034, FR-035, IV.1).
- **DBC**: each `\pre` carries an `SG_REQUIRE`, each `\post` carries an
  `SG_ENSURE`, and each `SG_ASSERT` carries its doc line, in the form
  `tools/dbc/dbc_pair_gate.py` reads. The gate runs before each push. A
  helper or local that only `SG_ENSURE`, `SG_REQUIRE`, or `SG_ASSERT`
  reads is `[[maybe_unused]]`, so the Release build (`-Werror`,
  consumer-release) passes.
- **JSON**: simdjson v4.6.11 in `external/simdjson`. Write with
  `builder::string_builder`; read with the DOM error-code form, one
  test per error code; no throwing `.value()` getters; no loop over a
  temporary `simdjson_result`; read files with `stream.read`, no
  `istreambuf_iterator`. No in-house parser or emitter, and no new
  library (FR-029, Additional Constraints).
- **API**: a new public call takes an options struct, no bool
  (FR-003).
- **CI**: the coverage preset keeps what master has
  (`-fprofile-update=atomic`, no `-D_GLIBCXX_ASSERTIONS`, and
  `-Wl,-u,__gcov_dump` when PR #44 has merged). Tests run with
  `ctest -j 2`. No other CI change without approval.
- **Commits**: blank lines between body paragraphs, and one footer
  block (`Approved-by: OmO` first, then `Refs:`) with no blank line
  inside. The commit never records approval of Charles Archer in any
  spelling. The prose rules of Principle XI bind every commit message,
  and no word is split or changed to pass the spell-check or the prose
  gate.
- **CI logs**: after each push, every job's log is read, including
  dbc-gate, test-rocky (GCC 14, `-Werror`), consumer-release, tsan,
  and sanitize; the feature's own failures are found and fixed, and no
  failure is handed to the owner.
- **Rebase and push**: rebase, no merge; push with
  `--force-with-lease`; after each push, show
  `git ls-remote origin <branch>`.
- **Ledger**: the task-list count and the coverage counts of the CI run
  of the last pushed commit are recorded.

## Scope

In scope: the probe capability, the modes, the `idle` preset and the
overlap check, the window split, the console and JSON reports, the N5
rule names, the docs and the example, the acceptance script and its
recorded results.

Out of scope: the host pre-check with warnings and `/proc` attribution
(N13b), the FWQ/FTQ probes and their spectrum (N13c, D-15), the
hardware latency probe and the SMI counts (N13d), the osnoise and
timerlat probes (N13e), the NUMA memory-node placement (N12), and any
kernel setting change (D-12). The noise attribution on the measured
cpus is N13b (D-9).

## Assumptions

- The `sysjitter` field order is the order of the capability list C-1
  of the prompt: `gapCount`, `gapsPerSecond`, `minNs`, `medianNs`,
  `meanNs`, `p90Ns`, `p99Ns`, `p999Ns`, `p9999Ns`, `maxNs`, `totalNs`,
  `totalPercent`, `runtimeNs`, `thresholdNs`, `loopPeriodNs`,
  `droppedGaps`. The roadmap file was absent from
  `/workspace/` at run time (Run-time binding), and no token
  `sysjitter` exists in the tree, so the order stands recorded here and
  the rebase note re-checks it against the roadmap when the file is
  reachable.
- Each harness test binary carries its own file-local replaceable
  `operator new`/`operator delete` counting hooks and diffs the count
  around the region under test; master carries no shared allocation
  counter in the harness library. The probe-loop test writes its own
  pair in that pattern (FR-041, Clarifications Q-2).
- The `.expected` file of the example checks only fields with no host
  dependence, matching the pattern the other docs examples with
  host-dependent output follow; the plan cites the pattern it follows.
- The `BenchmarkHandle` fluent form fits the registry because the
  handle already carries fluent setters for run shape and timing
  (`benchmark.hpp:1648` to `:1726`); the plan confirms the seam and
  records it (FR-015).

## Budget note

`README.md` stands at 713 lines against `PAGE_LINE_CAP` 713 at the
audit point, equal. The "Jitter probe" section of at most 25 lines and
its quick-reference rows move the cap by the measured figure through a
recorded decision at the rebase, the pattern of the specs/023 and
specs/025 moves the drift-gate comments record. The section adds no
entry to `SECTION_LINE_CAPS`; the default cap 25 applies
(`tools/docs/example_drift.py:133`). The detail page
`docs/pages/harness.md` (1093 lines at the audit point) carries the
option table and the field reference.

## Dependencies

No new library. The feature builds on the vendored hwloc (N4 layout),
the vendored simdjson v4.6.11 (JSON), and the counters library (TSC
read, calibration, fake provider). The `jitter:` rule names depend on
the N5 parser, which PR #45 carries (FR-033).

## Citations

Read at the audit point `aec8fce` unless a row names a branch.

- `include/speedgun-ng/execution_context.hpp:91-95` (`WaitMode`:
  `SPIN`, `YIELD`, `BLOCK`), `:100` (`kNoPin`), `:116-125`
  (`PlacementSlot`), `:150` (`ContextPolicy`), `:184` (`PlacementFn`),
  `:201` (`setPlacementFunction`)
- `source/harness/execution_context.cpp:79` (`NO_PIN` text), `:106-127`
  (`pinToPlacementSlot`, the placement registry)
- `source/harness/placement_preset.cpp:38,75,83` (the preset table,
  `presetText`, `resolvePreset` returning `PlacementPlan`)
- `source/harness/topology.cpp` (the N4 hwloc layout)
- `source/harness/detail/internal.hpp` (the N4 test seams; `:454`
  names the `--json-raw` chunk content)
- `include/speedgun-ng/counters_clock.hpp:29-53` (the `tsc` read mode
  and the `machine/tsc` leaf), `:62` (`ClockProvider`)
- `include/speedgun-ng/counters_fake.hpp` and
  `test/source/counters_fake_test.cpp` (the fake provider)
- `source/counters/plan.cpp:264` (`calibrate`),
  `source/counters/detail/core.hpp:140` (the calibration state)
- `source/harness/percentiles.cpp:104,115,155` (`percentileAt`, the
  nearest rank, the report columns)
- `source/harness/runner.cpp:152-158,252-273` (the chunk values and the
  per-run capture behind the timed windows)
- `source/harness/detail/json_report.hpp:133,137,148`
  (`NoiseWarning` and its account),
  `source/harness/report.cpp:193,516` (`printNoiseWarning`)
- `source/harness/cli.cpp:294-312` (the option table of master),
  `:309,383` (`--json-raw` with the values `chunks` and `none`)
- `origin/026-regression-gate`: `include/speedgun-ng/benchmark.hpp:2500`
  (`parseCompareRule`), `source/harness/compare.cpp:722`,
  `source/harness/cli.cpp:315-316` (`Q`, `S`)
- `origin/024-multipass-collection`:
  `include/speedgun-ng/benchmark.hpp:528-547` (`metricMethod`,
  `passCount`, `passSpread`), `source/harness/cli.cpp:331` (`S`)
- `tools/docs/example_drift.py:30` (`SECTIONS`), `:133`
  (`SECTION_LINE_CAP` 25), `:138` (`SECTION_LINE_CAPS`),
  `PAGE_LINE_CAP` 713
- `docs/schema/report-v1.json`, `source/harness/json_load.cpp`,
  `source/harness/json_report.cpp` (the N3 report surface)
- `include/speedgun-ng/benchmark.hpp:1620-1726` (`BenchmarkHandle` and
  its fluent setters)

## Open questions

None block this spec. The plan carries one forward check: the
`sysjitter` field order re-checks against the roadmap file when that
file is reachable (Assumptions).

## Constitution Checklist

- Main-page impact check (IV.1): this feature changes user-visible
  behavior, and the README section "Jitter probe", the example region
  in `example/docs/jitter_probe.cpp`, and the quick-reference rows land
  in the same change (FR-034, FR-035).
