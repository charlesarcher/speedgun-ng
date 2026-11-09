---

description: "Task list for 045-merge-fixes"
---

# Tasks: Fix the counters and FWQ/FTQ gates

**Input**: Design documents from `/specs/045-merge-fixes/`

**Prerequisites**: plan.md (required), spec.md (required for user
stories), research.md with its two annexes, data-model.md,
quickstart.md. No contracts directory exists: the spec records no
interface change (decision D-8).

**Format**: `[ID] [P?] [USx] description`. `[P]` marks a task that touches different files from its
siblings and runs in parallel once the dependencies its text and the
Dependencies section name land. Every description names the exact file paths,
the symbols, the scenario, the assertion and the gate that verifies it. A
covering test stands observed failing before its code lands where a test can
fail first (constitution III and VI). Each commit carries one topic and its
topic's tests, and passes the gates alone: its own build passes and its topic
ctest entries pass; the hard coverage gate binds at the PR head (FR-011,
SC-002), and each commit records its line and branch counts in the PR body.
Commit footers follow the spec's commit rules: part A commits `Refs: G-19 G-21 T-LCOV-3`, part B
commits `Refs: specs/043-fwq-ftq N13c-F`, spec-file commits `Refs: specs/045-merge-fixes`, then `Approved-by: OmO`, as the last two lines with no
blank line between them. No mixed commit arises: files under
`specs/045-merge-fixes/` land only in spec-file commits. Task ids T011 to T026
carry Part A; T101 to T118 carry Part B; references to `T0` numbers above T026
and to `T1` numbers above T118 name the spec 043 task rows, which stay outside
this list; a `T` reference that could name a task of this list always carries
the "spec 043" qualifier when it means a 043 row. Main-page impact check (IV.1):
no page change, per the spec's Constitution Checklist.

## Phase 0: Setup

**Purpose**: the lane stands up on a known base, and the spec
artifacts land first.

- [ ] T001 [P] [US1] Create the branch `045-merge-fixes` from `origin/master` at
`b1dd7670` after the number check the spec binds: `specs/` on master, the remote
branches (`git ls-remote --heads origin`) and the pull request list carry no 045 entry. Record the base sha
and the date in `specs/045-merge-fixes/spec.md` and in the first commit body.
- [ ] T002 [US1] Land the spec artifacts as the first spec-file
  commit: `specs/045-merge-fixes/` with `spec.md`, `plan.md`,
  `research.md`, `research-part-a.md`, `research-part-b.md`,
  `data-model.md`, `quickstart.md`, `checklists/requirements.md` and
  this `tasks.md`. Footer: `Refs: specs/045-merge-fixes`, then
  `Approved-by: OmO`. Gate: `cmake -P cmake/prose-lint.cmake` reads
  the added prose clean.
- [ ] T003 [US1] Capture the baseline at `b1dd7670` for the PR body:
  `git grep -c LCOV_EXCL -- source include` by file (94 total), the
  `ci-rocky` ctest result, the coverage preset's line percent, branch
  shortfall and dark lines by file (group_io.cpp 2, provider.cpp 6,
  44 dark lines and 52 short branch positions of the PR #72 code per
  FR-011), and the tsan job's race report. Record the figures under
  `specs/045-merge-fixes/results/baseline.md` in a spec-file commit.

## Phase 1: Part A (counters, user stories US-1 and US-2, priority P1)

**Purpose**: host-free counters tests and the marker repairs, in
commits A1 to A3.

## Part A tasks

### Commit A1: the seam field and the host-free tests

Covers FR-001, FR-002 and the FR-001/FR-002 test scenarios of
research-part-a, with the FR-013 rework of the seam-driven tests.

- [ ] T011 [P] [US2] In `test/source/counters_kernel_calls_test.cpp`, add the new-field round-trip scenario: install a
scripted `CpuidWords` pair through the seam, assert `kernelCalls().cpuidWords()`
answers the installed pair; clear the install, assert the field answers the same
value the CPUID probe (`readCpuidWords`, `source/counters/linux_pmu/table_parse.cpp:56-75`) answers directly, and assert no value derived
from CPUID (the watchdog pattern, FR-013). Gate: `ctest --test-dir build -R counters_kernel_calls_test --output-on-failure`; the scenario reads red
first, because the field does not exist yet.
- [ ] T012 [US2] In `source/counters/linux_pmu/kernel_calls.hpp`, add the ninth seam field to the `KernelCalls` options
struct at lines 55-79: a function reference named `cpuidWords` that returns
`CpuidWords` (`source/counters/linux_pmu/pmu_ident.hpp:18-27`), the pattern the other eight members follow, documented with
the struct's contract pairs and the non-null postcondition enforced per the DBC
facility (II). In `source/counters/linux_pmu/kernel_calls.cpp`, add the matching real function to the real set at lines
127-136 with a Linux arm that calls `readCpuidWords` and an off-Linux stub,
following the file pattern at lines 28-115; wire it into the install and clear
machinery. Makes the T011 scenario green. Gate: `ctest --test-dir build -R counters_kernel_calls_test --output-on-failure`.
- [ ] T013 [US2] In `source/counters/linux_pmu/table_parse.cpp`, rewrite `pmuIdentCurrent` (lines 113-126): drop the
function-local `static const PmuIdent` cache, call `kernelCalls().cpuidWords()`
on each call, and hand the words to `pmuIdentFromWords` (`table_parse.cpp:77-111`), which stays a
pure function of the words. The counter-limit register probe (`source/counters/linux_pmu/counter_limits.cpp:133-205`) stays real;
the commit body states that reason and the watchdog-pattern rule. Gate: `ctest --test-dir build -R counters_kernel_calls_test --output-on-failure`,
plus `ctest --test-dir build -R counters_pmu_test --output-on-failure` for the
integration entry.
- [ ] T014 [US2] In `test/source/counters_pmu_provider_test.cpp`, script two CPU identities through the new field. First
identity: a vendor and family a pinned mapfile row matches, installed before
`kernelAliasWinsScenario` (lines 393-442), so the assertions at lines 408-420
(`!vendored.empty()`, the merge on a vendored core row) hold on every host. Second identity:
`NoSuchVendorXXXXX`-style words no pinned row matches, so the empty-selection arm at
`source/counters/linux_pmu/provider.cpp:160-162` executes on every host. Gate:
`ctest --test-dir build -R counters_pmu_provider_test --output-on-failure`; the host-dependent part reads red on any host whose CPU matches no vendored
row, which is the red the scenario records first.
- [ ] T015 [US2] In `test/source/counters_linux_pmu_seam_test.cpp`, install scripted words through the new field at
`mapfileScenario` (lines 2206-2286) and at the `selectedTable` helper (lines
3673-3677), so the host-dependent early returns at lines 2215-2220 and 3751-3755
lose their host dependence. Gate:
`ctest --test-dir build -R counters_linux_pmu_seam_test --output-on-failure`
under `taskset -c 0,1` (FR-013).
- [ ] T016 [P] [US2] In `test/source/counters_pmu_provider_test.cpp`, add the errno matrix over the `pmuProbe` switch at
`source/counters/linux_pmu/provider.cpp:726-733`, driven through the existing `gErrno` fixture (lines 110-111): script `EINVAL`,
`EOPNOTSUPP` and `ENOENT`, each scenario asserting the `NOT_ENCODABLE` state; script
`EACCES` asserting the `PERMISSION_BLOCKED` state beside it. No real open takes
part (FR-002, FR-013). Gate: `ctest --test-dir build -R counters_pmu_provider_test --output-on-failure`.
- [ ] T017 [P] [US2] In `test/source/counters_linux_pmu_seam_test.cpp`, finish the FR-013 rework of the host-fact sites:
script `fastBranchScenario`'s availability probe (lines 3196-3199) and `devicePageVerdictScenario`'s granted opens (lines
4225-4227) through `installKernelCalls`, the pattern `scriptedKernelScenario` (lines 4300-4435)
follows; hold `contextOpenRefusalScenario` (line 2160) to the FR-002 rule and
script the refusal where the asserted result is host-shaped; rework the procfs
descriptor and mapping counters at lines 208-238 to fixture state; in
`multiplexWindowScenario` (lines 821-835), keep the recipe assertions at lines
867-883 and drop the real `__rdtsc` call at `fast_read.cpp:399` by driving a
page whose enabled and running counts agree. Gate: `ctest --test-dir build -R counters_linux_pmu_seam_test --output-on-failure`, then the whole suite
under `taskset -c 0,1`.
- [ ] T018 [US1] Verify commit A1 passes the gates alone: `cmake --preset=ci-rocky`, `cmake --build build`, `ctest --test-dir build --output-on-failure`, the
same suite under `taskset -c 0,1`, and `cmake -P cmake/lint.cmake` plus `cmake -P cmake/spell.cmake`. Assert the
standing option-error behavior of FR-014: run one entry whose bad value exits 2
with one quoted line, and record the exit. Record the ctest result and the
commit line and branch counts in the commit body for the PR body.

### Commit A2: the marker repairs and their coverage scenarios

Covers FR-003 and FR-004. Every removal here is driven by a scripted
answer or a host-record ground; the tree gains no new marker
(FR-015).

- [ ] T019 [P] [US4] In `include/speedgun-ng/counters_provider.hpp`, remove the `LCOV_EXCL_LINE` at line 111 on the
defaulted copy constructor of `ObjectSink` (added by commit `65dea585`, a gcc 16 host
record). Keep the default-constructor marker at line 106 and its comment block
at lines 100-105. The commit body states the gcc 16 record and the removal
reason. Gate: the coverage run at the commit head confirms line 111 carries a
count on the acceptance host;
`ctest --test-dir build -R counters_pmu_provider_test --output-on-failure`
passes.
- [ ] T020 [US4] In `source/counters/linux_pmu/group_io.cpp`, replace `LCOV_EXCL_LINE` with `LCOV_EXCL_BR_LINE` on
the two `switch (slot.source)` lines at line 306 and line 432 (commit `387ca19f`
widened both). The no-case arcs stay branch-excluded; both lines join the
line-coverage denominator. FR-003 fallback: when a line sits dark on an
acceptance host, cover it through a scripted seam answer or keep the
`LCOV_EXCL_LINE` in the kept list with its ledger reason. Gate:
`ctest --test-dir build -R counters_pmu_window_test --output-on-failure` (the
scenarios at `test/source/counters_pmu_window_test.cpp:291-293` and `:536-540`
hand each switch every enumerator), then the coverage run at the commit head
confirms both lines carry counts.
- [ ] T021 [US4] In `source/counters/linux_pmu/group_io.cpp`, remove the T140 block at lines 395-404: the `LCOV_EXCL_BR_START`, the BR
lines at 400 and 403, and the `LCOV_EXCL_BR_STOP`. The scripted answer is
`fastWindowReadScenario` (`test/source/counters_pmu_window_test.cpp:508-559`): a member page with `cap_user_rdpmc` clear (lines
530-533) reaches the refused `gapped = true` line 402, and the scenario asserts
the gap beside the zero count (lines 548-553). The not-taken OK side adds one
short branch position to the whole-tree count; the implement phase reports the
number (FR-004). Gate: `ctest --test-dir build -R counters_pmu_window_test --output-on-failure`, then the coverage run at the commit head shows line
402 counted.
- [ ] T022 [US4] In `source/counters/linux_pmu/provider.cpp`, remove the empty-selection markers at lines 157-163:
the `LCOV_EXCL_BR_START`, the BR line at 160, the LINE markers at 161 and 162, and the `LCOV_EXCL_BR_STOP`.
Depends on T014: the scripted no-row identity drives the arm at lines 160-162 on
every host, and `counters_linux_pmu_seam_test.cpp:2282-2285` already asserts the
selection behavior for it. Gate: the coverage run at the commit head shows the
arm counted;
`ctest --test-dir build -R counters_pmu_provider_test --output-on-failure` and
`ctest --test-dir build -R counters_linux_pmu_seam_test --output-on-failure`
pass.
- [ ] T023 [US4] Verify the FR-004 coverage state at the commit A2
  head: build the coverage preset with the gate and confirm
  `group_io.cpp` and `provider.cpp` carry zero dark lines outside kept
  markers, with the branch shortfall recorded. Record the ledger
  verification entry for the group short read
  (`gapped = true` at `group_io.cpp:288`, driven by
  `groupWindowGappedScenario` at
  `test/source/counters_pmu_window_test.cpp:277-329`, which asserts the
  gap disclosure at lines 303-304 and 322-323; no edit). Record the
  T020 fallback decision, whichever way it went. Gate: the coverage
  preset with the gate; the counts go into the commit A2 body.

### Commit A3: the ledger and record updates (spec-file commits)

Covers FR-005. This commit produces the marker count, the kept list
and the recorded Part A counts. Its artifacts sit under
`specs/045-merge-fixes/`, so it lands as spec-file commits with
footer `Refs: specs/045-merge-fixes`, outside the Part A footer
rule.

- [ ] T024 [US4] At the commit A3 head, run
`git grep -c LCOV_EXCL -- source include` and verify the count reads 84: 94 at
base, minus the 10 removed marker lines (one at `counters_provider.hpp:111`, four in `group_io.cpp:395-404`, five in `provider.cpp:157-163`);
the two T020 swaps change no count. Verify the six G-21 markers stay: four in
`include/speedgun-ng/dbc.hpp` (the pairs at lines 106-216 and 238-394), two in
`source/simulation/marker.cpp` (lines 40-43). Gate: the grep command; the
reading satisfies SC-003.
- [ ] T025 [US4] Write the kept-marker ledger with one reason per
  kept marker into `specs/045-merge-fixes/results/marker-ledger.md`:
  the count by file; the counters kept list (`group_io.cpp` lines 306
  and 432 as BR markers: the no-case arc of a switch over a closed
  enumeration; `group_io.cpp` lines 376-390 and 409-428: the
  page-rewrite arms no fixture answer forces; `provider.cpp` lines
  483-517: the past-the-enum arm of a closed switch; `table_parse.cpp`
  lines 712-714: the by-value return epilogue; `table_parse.cpp` lines
  807-817: the registry arm the generator guarantees;   `fast_read.cpp`
  lines 335-356 and 408-410: the instruction arm and the pair-stability
  arm; the two surviving `counters_provider.hpp` markers at lines 100
  to 106: the defaulted default constructor emits no code (the T066
  record, older than PR #71); `dbc.hpp` and `marker.cpp` per G-21),
  one reason each; any
  marker no ruling covers carries "waits for ruling" (D-7); markers
  outside Part A scope keep their recorded T066 reasons. Draft the
  PR-body counts from it. The file lands in a spec-file commit with
  footer `Refs: specs/045-merge-fixes`.
- [ ] T026 [US1] Run the Part A acceptance preparation at the Part A head on the
development host: full `ctest` and `ctest` under `taskset -c 0,1`; the sanitize and tsan
presets over the counters tests; the coverage preset with the gate; the five
tools of SC-005 (`cmake -P cmake/lint.cmake`, `cmake -P cmake/spell.cmake`, `tools/docs/example_drift.py`, `tools/prose/prose_gate.py --base origin/master`, `tools/dbc/dbc_pair_gate.py`); the release build with the analyzers,
`cmake --preset=ci-ubuntu` then `cmake --build build`, expecting zero clang-tidy and cppcheck
findings and the name check green (VIII, and the per-feature release build of
IX); and the time-source gate `test/time_source_gate.sh`. Record the host name, kernel, GCC version,
head sha, line percent, branch shortfall against 21, dark lines by file and the
marker count by file in `specs/045-merge-fixes/results/` and in the commit
bodies. The results files land in a spec-file commit with footer `Refs: specs/045-merge-fixes`.

## Phase 2: Part B (jitter probe, user stories US-3, US-4, US-5)

**Purpose**: the jitter probe repairs and the record fixes, in commits
B1 to B6.

## Part B tasks

### Commit B1: the paced clock and the signed sine delta (FR-006, FR-007)

- [ ] T101 [US3] Add the covering assertion first in
  `test/source/harness_jitter_window_test.cpp`: in the FWQ window
  scenario whose document the gate checks (the `integerValueAfter`
  assertions over `document` and `gateName`, lines 1410 to 1522),
  assert the entry's window holds the samples of the watched entry
  (`sampleCount` above 0) and the document carries a real jitter record
  with `passCount` above 0, the shape PR #73 run 41576309608 broke.
  Run the entry against the standing one-tick clock and record the red.
  Verify: `cmake --build build --target harness_jitter_window_test`
  exits 0 and `ctest --test-dir build -R harness_jitter_window_test`
  reports the entry failing on the new assertions.
- [ ] T102 [US3] Pace the scripted clock in the same file,
  `test/source/harness_jitter_window_test.cpp`: add the named constant
  `kPacedStepNs` (1000, the `kTargetUnitTimeNs` value of
  `include/speedgun-ng/benchmark.hpp:2443`) beside `kSeriesJump` at
  line 117; replace the tail-delta argument 1 with `kPacedStepNs` in
  the six `setPoints("machine", "tsc", ...)` calls at lines 93, 1452,
  1492, 1532, 1781 and 1800; the jump hand-offs set `kSeriesJump` as
  before (lines 336, 1905, 1915, 1924, 2783) and the restore hand-offs
  set `kPacedStepNs` where they set 1; rewrite the scenario comment at
  lines 2118 to 2120 that names the 2 ms unit time to the paced
  cadence, in constitution XI prose. The probe changes nothing: the
  alongside loop drops no sample a real run would keep, and the commit
  body records no probe change. Verify: the T101 assertions turn green
  and `ctest --test-dir build -R harness_jitter_window_test` reports
  every entry passing.
- [ ] T103 [US3] Reshape the roundtrip scenario
  `theSeriesRoundTripsThroughLoadReport` in the same file
  (`test/source/harness_jitter_window_test.cpp`, line 2300 area): drop
  the `DipSeriesReader` install and run the scenario on the paced
  standing clock; keep the assertions reading the document shape and
  the fold presence, so no expected magnitude moves. Verify:
  `ctest --test-dir build -R harness_jitter_window_test` reports every
  entry passing.
- [ ] T104 [P] [US3] Make the sine-arm delta signed and clamped in
  `test/source/harness_fwq_ftq_test.cpp`, `scriptedDelta` lines 232 to
  238: first run the FWQ sine arm under the sanitize preset and record
  the UBSan report of the conversion to `unsigned long` at line 237
  (red observed); then compute the delta as
  `windowStep + std::llround(sineAmplitude * sin(phase))` in
  `std::int64_t` and clamp a negative result at 0; keep the FWQ
  assertions at lines 961 to 970 and the FTQ assertions at lines 1374
  to 1390 relational, so no expected magnitude moves. Verify:
  `cmake --preset=ci-sanitize`, `cmake --build build/sanitize`, then
  `ctest --test-dir build/sanitize -R harness_fwq_ftq_test` reports the
  entry passing with no UBSan finding, and the same holds for the tsan
  build under `ctest --test-dir build/tsan`.

### Commit B2: the latch-time snapshot and the race fix (FR-008)

- [ ] T105 [US3] Add the covering scenario first in
  `test/source/harness_jitter_cli_test.cpp`: a `mode=both` scenario
  that asserts the pre-check record's series figures fold from a
  per-cpu latch-time snapshot, one object per cpu: the record's
  `series` figures (`sampleCount`, the percentiles, `droppedSamples`)
  equal the figures of the prefix the latch stored and stand stable
  while the alongside loop runs on. The red stands as a compile
  failure while the snapshot type and its fold lack in
  `source/harness/detail/jitter_probe.hpp`. Verify:
  `cmake --build build --target harness_jitter_cli_test` exits nonzero
  on the missing snapshot symbols (red observed).
- [ ] T106 [US3] Add the per-cpu snapshot type `JitterLatchSnapshot`
  and its fold to
  `source/harness/detail/jitter_probe.hpp`: one object per probe cpu
  carrying the series prefix copied at the latch line, the kept-at-
  latch count, the dropped-sample count, the five calibration figures
  (`workCount`, `unitTimeNs`, `targetNs`, `quantumNs`,
  `calibrationSpanNs`) and the method; the fold answers the same
  `JitterSeriesStats` the state folds answer; document the type and
  the fold with the `\pre` and `\post` pairs the DBC gate requires,
  enforced per the facility (II).
  Verify: `cmake --build build --target harness_jitter_cli_test`
  compiles.
- [ ] T107 [US3] Build and publish the snapshot in
  `source/harness/jitter_probe.cpp`: the probe thread builds one
  snapshot per cpu at the latch line, after `seriesKeptAtLatch` stands
  stored at line 589, and release-stores one publish flag; the
  alongside loops gain no lock and no atomic, and the probe thread
  keeps writing the series into the `ProbeCpuState` its item owns.
  Verify: `cmake --build build` exits 0 and
  `ctest --test-dir build -R harness_jitter_cli_test` reports the T105
  scenario passing.
- [ ] T108 [US3] Point the mid-run folds at the snapshot in
  `source/harness/cli.cpp`: the method noise gate (lines 1302 to 1325,
  `precheckNoisyCpus` and its `seriesStatsOf` fold at line 1313) and
  the mode=both record site (lines 1379 to 1385,
  `seriesStatsOf(state, state.seriesKeptAtLatch)`) acquire-load the
  publish flag and read the snapshot only; the run-level and
  per-benchmark folds at lines 2020 to 2025 and in
  `source/harness/report.cpp` (lines 873 and 978) stay join-ruled
  behind `stopAndJoin`; for the `tsc` gap-fold site at line 1368
  (`gapStatsOf(state)`), take one of the two shapes research.md FR-008
  names, the snapshot carrying the kept gap prefix and the dropped-gap
  count, or the `tsc` record reading the gap fold behind the join, and
  record the choice in the commit body. Verify:
  `ctest --test-dir build -R harness_jitter_cli_test` and
  `ctest --test-dir build -R harness_jitter_window_test` report every
  entry passing, then `cmake --preset=ci-tsan`,
  `cmake --build build/tsan`, and
  `ctest --test-dir build/tsan -R harness_jitter` report no race.

### Commit B3: the prose fixture runner in one process (FR-009)

- [ ] T109 [US1] Reshape the gate invocation in
  `test/prose-gate-fixture/run_prose_gate_fixtures.py`, and correct
  the stale comment the `prose_gate_fixtures` entry carries in
  `test/CMakeLists.txt` (comments only, the file the Part B scope
  binds): `run_gate`
  (line 493) loads `tools/prose/prose_gate.py` once through
  `importlib` and calls the module entry point in process for every
  case except the timing case `run_timing_case` (lines 1008 to 1062),
  which keeps its subprocess run over the repository root, its
  10-second check, its exit 0-or-1 acceptance and its
  `GITHUB_ACTIONS` skip; capture the entry's output with
  `contextlib.redirect_stdout` and `redirect_stderr`, read the exit
  code from the return value, switch the working directory per case
  with `os.chdir` and restore it after each case; the rule data loads
  per call, because the loader mutants of spec 043 T012 prove the
  load-time validation fires. Verify:
  `ctest --test-dir build -R prose_gate_fixtures` reports the entry
  passing.
- [ ] T110 [US1] Time the runner itself in the same file:
  `main` (line 1282) reads `time.monotonic()` at entry and checks the
  elapsed wall time before every exit; above `TIMING_BOUND_SECONDS`
  (10.0, unchanged at line 84) the runner prints one line that names
  the bound and the measured time and exits nonzero; the internal
  gate-run assert of the timing case stays untouched. F1-004 replaced this
  bound: the runner keeps the `GATE_RUNS` tally and the two work-count
  asserts, and no clock stands in the runner. Verify:
  `ctest --test-dir build -R prose_gate_fixtures` reports the entry
  passing, and a parallel run
  `ctest --test-dir build -j 8 -R prose_gate_fixtures` finishes inside
  the 10-second bound.

### Commit B4: the record fixes (FR-010, FR-012)

The ticks this commit lands record the gate results the B1 to B3
commits prove (FR-010, and the footer rule the spec states).

- [ ] T111 [US5] Tick the spec 043 Phase 10 rows in
  `specs/043-fwq-ftq/tasks.md`: run every Phase 10 gate the lane can
  execute at the Part B head and tick T048, T049, T051, T052, T053,
  T054, T055 and T056 when their verify commands pass; leave T050 open
  with a note line under the row that names the 045 branch allowance of
  21 against the shortfall of 0 its verify text demands; leave T057
  open with a note line naming the owning 043 lane acceptance record;
  leave T061 open with a note line naming the owning 043 lane push; no
  Verify line changes. Verify: `git diff -- specs/043-fwq-ftq/tasks.md`
  shows tick marks and note lines alone, and each ticked row's verify
  commands pass at the head.
- [ ] T112 [P] [US5] Fix the two `CMakeLists.txt` comment sites: the
  version-block comment at lines 45 to 51 and the harness-target
  comment at lines 399 to 407 name PR #70 (T-LCOV) as the change that
  moved the archive SOVERSION to 5; the change touches comments alone
  and no number line moves (`SOVERSION 20` and the archive 5 stand).
  Verify: `git diff -- CMakeLists.txt` shows comment lines alone,
  `cmake -P cmake/spell.cmake` exits 0, and
  `tools/prose/prose_gate.py --base origin/master` exits 0.

### Commit B5: the coverage mapping (FR-011)

- [ ] T113 [US4] Run the coverage build at the Part B head and map the PR #72
dark lines and short branch positions: `cmake --preset=ci-coverage`, `cmake --build build/coverage`, then the coverage ctest entry;
list the dark lines and the short branch positions by file and map each one to
the scripted arm research-part-b.md FR-011 names; build the shortfall budget
beside the mapping: the base figure, plus the short positions the marker
removals add (the not-taken OK side of T021 and the unmarked arms of T022),
minus the positions the new scenarios close, reconciled to 21 or less before the
gate runs; a line no real test can reach stops the work and reports to the
owner, like the public-layout rule. Verify: the mapping names one arm per dark
line, and the list stands recorded for the PR body.
- [ ] T114 [P] [US4] Add the scenarios the mapping demands to
  `test/source/harness_fwq_ftq_test.cpp`: the `ScriptedReader` and the
  `ScriptPlan` table (lines 146 to 380) drive the calibration search,
  the unit loops, the drop counting and the latch prefix with exact
  read counts, one arm per mapped dark line of
  `source/harness/jitter_probe.cpp`. Verify:
  `ctest --test-dir build -R harness_fwq_ftq_test` reports every entry
  passing.
- [ ] T115 [P] [US4] Add the window-fold scenarios the mapping demands
  to `test/source/harness_jitter_window_test.cpp`: the scenario readers
  (`CountingReader`, `DipSeriesReader`, `GapSeriesReader`,
  `FlatSeriesReader`, lines 142 to 179 and 869 to 991) and the
  state-level fold scenarios with the planted series (`fillFwqSeries`,
  `fillFtqSeries`, lines 950 to 976) cover each mapped dark line of
  the window-fold surfaces. Verify:
  `ctest --test-dir build -R harness_jitter_window_test` reports every
  entry passing.
- [ ] T116 [P] [US4] Add the option-range, refusal-row and spectrum scenarios
the mapping demands to `test/source/harness_jitter_cli_test.cpp`, `test/source/harness_spectrum_test.cpp` and
`test/source/harness_jitter_bytes_test.cpp` for the mapped dark lines those
files cover. Verify:
`ctest --test-dir build -R "jitter_cli|spectrum|jitter_bytes"` reports every
entry passing.
- [ ] T117 [US4] Run the coverage gate at the Part B head and record the
figures: lines at 100 percent, the branch shortfall figure of 21 or less against
the budget T113 built, and the dark-line count by file; the numbers go into the
PR body. Verify: the coverage entry of `build/coverage` passes with the gate,
and the recorded shortfall reads 21 or less.

### Commit B6: the spec 045 records

- [ ] T118 [US5] Write the acceptance records under
  `specs/045-merge-fixes/results/`: per host, the full `ctest` result,
  the `ctest` result under `taskset -c 0,1`, the coverage gate figures
  (line percent, branch percent, shortfall against 21, dark lines by
  file), the marker count by file, the sanitize and tsan outcomes, and
  the five tools of SC-005; corsairone runs first at the PR head, the
  thrawn-side runs stand in when the lcov repair fails or the host
  stays unreachable, and the record states the corsairone gap in those
  cases. Verify: the files exist under `specs/045-merge-fixes/results/`,
  the commit touches `specs/045-merge-fixes/` alone, and its footer
  block reads `Refs: specs/045-merge-fixes` and `Approved-by: OmO`.


## Phase 3: Final (cross-cutting)

**Purpose**: the acceptance runs, the push and the pull request
record.

- [ ] T201 [US1] Run the corsairone acceptance at the PR head, per the
  spec's acceptance section: the lcov repair on the host alone, full
  `ctest`, `ctest` under `taskset -c 0,1`, the coverage gate figures,
  the marker count by file, the sanitize and tsan presets, and the
  five tools of SC-005. Record the host name, kernel, GCC version and
  head sha in `specs/045-merge-fixes/results/`. When the repair fails
  or the host stays unreachable, record the thrawn-side counts with
  the corsairone gap statement.
- [ ] T202 [US1] Run the thrawn acceptance at the same PR head with
  the same five items, and complete the results records.
- [ ] T203 [US1] Push and open the record: `git push -u origin 045-merge-fixes`; later pushes use `git push --force-with-lease=045-merge-fixes:<sha>`, and
`git ls-remote origin 045-merge-fixes` must equal `git rev-parse HEAD`. Assemble the PR body: the
commit list with Refs footers, the marker ledger, the coverage figures with the
branch shortfall number, the per-host acceptance counts, the open owner
questions O-1 and O-2. PR title: "Fix the counters and FWQ/FTQ gates". Last two
lines of the body: `Refs: specs/045-merge-fixes G-19 G-21 T-LCOV-3 specs/043-fwq-ftq N13c-F`, then `Approved-by: OmO`. The pull request opens when the owner's `gh`
credentials stand; the branch and its pushes carry the work. Verify SC-008
beside the assembly:
`git diff --stat <base>..HEAD -- .github tools/dbc tools/ci cmake` reads empty,
and the version, schema and SOVERSION lines diff empty.

## Dependencies and completion order

```text
Phase 0 (T001-T003, the spec-file commit)
  └─> Commit A1 (T011-T018) ─> Commit A2 (T019-T023) ─> Commit A3 (T024-T026)
        └─> Commit B1 (T101-T104) ─> Commit B2 (T105-T108) ─> Commit B3 (T109-T110)
              └─> Commit B4 (T111-T112) ─> Commit B5 (T113-T117) ─> Commit B6 (T118)
                    └─> Phase 3 (T201, T202, T203)
```

- Part A stands before Part B: the tsan and coverage gates of B2 and
  B5 read the whole tree, and the marker ledger of A3 feeds the PR
  body B6 assembles.
- Inside commit A1: T011 (test first) before T012 and T013; T014 and
  T016 stand after T012, because both drive the new field; T015 and
  T017 run beside them on other files.
- Inside commit A2: T019 runs in parallel with everything; T022
  depends on T014 (the scripted no-row identity); T020 and T021 stand
  independent; T023 verifies the commit head.
- Inside part B: B2 depends on B1 (the paced clock keeps the window
  scenarios the snapshot scenarios reuse); B5 depends on B2 and B3,
  because the coverage mapping reads their heads; B6 depends on every
  prior commit.
- T201 runs before T202 (corsairone first), and T203 runs last.

## Parallel execution examples

- Commit A1: T011, T015, T016, T017 run in parallel once T012 lands;
  T016 touches only `counters_pmu_provider_test.cpp` and T017 only
  `counters_linux_pmu_seam_test.cpp`.
- Commit A2: T019 (`counters_provider.hpp`) and T020/T021
  (`group_io.cpp` groups) and T022 (`provider.cpp`, after T014) run in
  parallel; T023 closes the commit.
- Commit B1: the window scenarios (T101, then T102 and T103, one
  file, red first) and the sine-arm scenario (T104, a different file)
  run in parallel.
- Commit B3: the runner reshape and its covering fixture scenario
  touch `run_prose_gate_fixtures.py` and its fixture tree and run as
  one unit; T109 and T110 pair test-first.
- Commit B5: T114, T115 and T116 touch different test files and run in
  parallel; T113 runs first, T117 closes.

## Implementation strategy

- MVP scope: commits A1 and B1 turn the two red, host-dependent
  failures the CI jobs report into green, scripted evidence
  (US-1, US-2, US-3). Each later commit adds an independently
  verifiable slice.
- Test-first where a test can fail first: the seam-field round trip,
  the scripted identities, the paced window, the signed sine arm and
  the snapshot fold all name their red observation; the marker
  removals verify through the coverage run that replaces the excluded
  line with a count.
- The gates of Principle VIII run at every commit head; the hard
  coverage gate binds at the PR head, and every commit records its
  line and branch counts for the PR body (FR-011, SC-002).

## Task count

- Setup: 3 tasks (T001-T003). Part A: 16 tasks (T011-T026) over
  commits A1-A3. Part B: 18 tasks (T101-T118) over commits B1-B6.
  Final: 3 tasks (T201-T203). Fixes round 1: 10 tasks (F1-001-F1-010).
  Post-merge fixes: 4 tasks (PM45-001, PM45-002, PM45-003, PM-END). The spec
  046 lane carries PM46-001 on the same branch. Close-rows round: 4 tasks
  (PM45-004 to PM45-007). Total: 58 tasks.

## Phase 4: Fixes round 1 (owner rulings G-22, 2026-11-04)

- [x] F1-001 Rebase the branch on `origin/master` at `561095df` (PR #74, version
      0.30.1). Keep version 0.30.1. Change no version, schema or SOVERSION.
- [x] F1-002 [R1] Make every window of the jitter window fixture run on the
      scripted clock, so the gap-free entry is always gap-free. No host gate.
      Test and fix in one commit. Run the test 100 times on thrawn and 100 times
      under `taskset -c 0,1`; all pass.
- [x] F1-003 [R2] Remove the unreachable arm at `topdown_select.cpp:567`, or fold
      it into the `SG_ENSURE` at `:595`. Name the reason in the commit body.
      Update the matching FR in `spec.md`. No new marker.
- [x] F1-004 [R3 R4] Give the prose timing case a bound that does not depend on
      the host, or move it out of the unit tests. Remove `RUNNER_BOUND_SECONDS`
      and the 30 s entry bound. Raise no bound. Update FR-009 in `spec.md`.
- [x] F1-005 Make line coverage 100% (the one line at 17617 of 17618) with a real
      test, or through F1-003. Give the branch shortfall number; it must be 21 or
      fewer.
- [x] F1-006 Replace each `/home/<user>/` path in the records with `$HOME/` or a
      path relative to the build tree. The check reads this lane only:
      `git grep -nE 'arche[r]c|char[l]es|@gmai[l]' -- specs/045-merge-fixes`
      prints nothing; the bracket classes stop that line from matching itself.
      The 194 matches outside this lane move to ticket T-NAMES (owner ruling,
      2026-11-04): 030-ci-speedup 55, 039-context-groups 32, 029-hunter-core
      26, the prose-gate fixture data under `test/` 24, 043-fwq-ftq 16,
      024-multipass-collection 9, 037-execution-graphs 8,
      014-identifier-naming-camelcase 8, 007-counters-and-timers 8,
      040-hunter-sensitivity 2, 031-host-validation 2, and 1 each in
      036-osnoise-timerlat-tracers, 035-numa-placement,
      034-host-noise-precheck and 032-fix-merged-features. Six further matches
      sit in the CI workflow, `CMakeLists.txt` and `CODEOWNERS`.
- [x] F1-007 Run every gate: `cmake -P cmake/lint.cmake`, `cmake -P
      cmake/spell.cmake`, `python3 tools/docs/example_drift.py .`,
      `python3 tools/prose/prose_gate.py --base origin/master`, the DBC gates,
      full `ctest`, `taskset -c 0,1 ctest`, ASan and UBSan, TSan, `ci-coverage`.
      Give the marker count from `git grep -c LCOV_EXCL -- source include`; it
      must be 84 or fewer.
- [x] F1-008 Write `results/pr-body.md` again: every commit, the head sha, the
      host, the kernel, the GCC version, the counts and the coverage figures
      (G-20). 72 columns or fewer per line, no break inside a word.
- [x] F1-009 Open one pull request to `master` with that body. CI on a GitHub
      runner is not acceptance.
- [ ] F1-010 On corsairone at the new head, run the acceptance again: full
      `ctest`, `taskset -c 0,1 ctest`, the sanitizers, `ci-coverage`. Record the
      host, the kernel and the head sha in `results/` and in the PR body.

Rulings: R1 the fixture premise stands fixed; no host gate. R2 the unreachable
arm goes. R3 and R4 the prose bound goes host-independent and the raised entry
bound goes away.

## Phase 5: Post-merge fixes (owner ruling, 2026-11-05)

**Base**: `858c55d9`, the merged head of pull requests #78 and #79: version
`0.31.0`, schema `1.14`, harness SOVERSION 21, archive SOVERSION 6. The
branch is `fix/post-merge-78-79`, cut from `origin/master`, and it carries
the spec 046 row PM46-001 beside these rows. No version, schema or SOVERSION
change. No CI or gate change.

- [x] PM45-001 [US1] On thrawn at `858c55d9`, run `ci-coverage` and list each
      short branch row by file and line. Add real tests until the shortfall
      reads 21 rows or fewer. Each test lands in its own commit with the code
      it reaches, if any. No code shaped for coverage. No new `LCOV_EXCL`, no
      new `--omit-lines`, no new `--ignore-errors` and no new `--remove`. The
      marker count from `git grep -c LCOV_EXCL -- source include` stays 84 or
      fewer.
- [x] PM45-002 [US1] When a short row stands beyond the reach of any real
      test, do not hide it. List it with its reason in
      `specs/045-merge-fixes/results/post-merge.md` and stop for an owner
      ruling.
- [x] PM45-003 [US1] On thrawn at the pull-request head, run the full `ctest`,
      `taskset -c 0,1 ctest`, the address and undefined-behavior sanitizer
      preset, the thread sanitizer preset and `ci-coverage`. Record the host,
      the kernel, the GCC version, the head sha and the counts in
      `specs/045-merge-fixes/results/post-merge.md`.
- [x] PM-END [US1] Squash the lane into one commit with author OmO, a title of
      50 characters or fewer, and the `Refs:` then `Approved-by: OmO` footer
      (owner ruling 2026-11-04), then measure on that head. Open one pull
      request to `master` whose body gives the host, the kernel, the GCC
      version, the head sha, the counts, the coverage figures and the branch
      shortfall number (G-20). Body lines: 72 columns or fewer.

## Phase 12: Close the rows the record lists (owner ruling, 2026-11-05)

**Base**: `2cfdd47a`, the merged head of pull request #83: version `0.31.0`,
schema `1.14`, harness SOVERSION 21, archive SOVERSION 6. The branch is
`fix/coverage-rows-to-allowance`, cut from `origin/master`. The owner ruling of
2026-11-05 sets the rule for this phase: no allowance moves, and the rows stand
closed by real work. No `LCOV_EXCL` marker, no `--omit-lines`, no
`--ignore-errors` and no `--remove` joins the route, and the marker count from
`git grep -c LCOV_EXCL -- source include` stays 84 or fewer.

- [x] PM45-004 [US1] Make the two readbacks of the top-down selection testable.
      Move the shape readback and the published readback of
      `source/counters/linux_pmu/topdown_select.cpp` into internal functions with
      their own contract, and call those functions from the selection. Drive each
      clause of each readback from `counters_topdown_select_test.cpp` with a
      hand-built tree. The eight short arms at lines 617, 620, 627, 633 and 652
      stand read.
- [x] PM45-005 [US1] Close the five arms of this host through the seams the
      repository already carries. `installTestProcRoot` feeds a sibling list of
      one cpu, and a sibling list of none, to the coverage line of
      `source/harness/cli.cpp:1621` to `:1628`. The `cpuidWords` seam of
      `source/counters/linux_pmu/kernel_calls.hpp` hands an identity of no
      vendored table to the area ask of `source/harness/cli.cpp:2044` to `:2051`.
      `detail::resolvedNodePlan()` and `detail::memoryBinder()` stand a cpu
      outside the plan, and a refused bind, in front of
      `source/harness/execution_context.cpp:464` to `:477`.
- [x] PM45-006 [US1] Close the five rows the record of PM45-002 names reachable:
      two probed families whose last instances interleave, a second probed family
      beside the `machine/tsc` availability refusal, and one document whose first
      field of one record read answers an error.
- [x] PM45-007 [US1] Measure at the head of this branch with `ci-coverage`. The
      gate must read a shortfall of 21 rows or fewer with the allowance of D-14
      unchanged. Record the reading, the rows this phase closed and the reason for
      every row that still stands in
      `specs/045-merge-fixes/results/close-rows.md`.

## Phase 13: Repair the coverage-data fault (owner ruling G-25, 2026-11-07)

**Base**: `origin/master` at `615c0964547e4e9768b8e2f2d79991033a0eb5bd`
(version 0.34.0, schema 1.16, harness SOVERSION 22, archive SOVERSION 8,
markers 84, README 1002 lines against the cap of 1004). The branch is
`fix/coverage-data-repair`, cut from `origin/master` at branch time.
One squashed commit, one pull request, no merge. No version, schema or
SOVERSION change. No new `LCOV_EXCL` marker, `--omit-lines` pattern,
`--remove` pattern, `--filter` entry, allowance or CI change. The
capture list keeps its class and stands reported.

- [x] COV-001 Reproduce the fault from a fresh local instrumented build
      and classify it (FR-016).
  - a. Fresh tree: `rm -rf build/coverage`, then
    `cmake --preset=ci-coverage -DCMAKE_C_COMPILER_LAUNCHER= -DCMAKE_CXX_COMPILER_LAUNCHER=`,
    then `cmake --build build/coverage -j 24`, then
    `ctest --test-dir build/coverage --output-on-failure --no-tests=error -j 24`,
    every read with the host `~/.lcovrc` out of reach.
    Verify: the build exits 0 and the suite log gives the passed
    figure, the failed figure and the `did not run` block.
  - b. Capture with the class tolerated, then the same capture without
    it. Verify: the first exits 0 and prints the end-line records; the
    second exits 1 and writes no trace.
  - c. Extract, `--list` and `--summary` with and without the class.
    Verify: the extract exits 0 both ways on lcov 2.6; the list and the
    summary exit 1 naming a contract function record; each command,
    exit and quoted line stands in the record.
  - d. Read the function records of the three closure sites in the raw
    and the extracted trace. Verify: the record gives the `FNL` and
    `FNA` pair, the hit count and the `DA` line the filter removes for
    each site.
  - e. Classify the three causes. Verify: the record carries one line
    per cause with the command and the exit that shows it; stale build
    data names the `.gcda` stamp mismatch and the configure that clears
    it; the instrumentation shape names the seven end-line records and
    their three files; the source shape names the three predicates and
    the six emptied function records.
- [x] COV-002 Audit the pull request #89 range and remove the
      suppression as FR-017 and FR-019 state (COV-001 first).
  - a. Audit `48e89812` and `c87ede7a` hunk by hunk. Verify: the record
    carries one line per hunk of `git diff 48e89812^..c87ede7a`, the row
    count equals the hunk count that diff prints, each row ends with
    exactly one of `plumbing`, `comment` or `unrelated change`, and the
    count of `unrelated change` rows reads 0.
  - b. Remove `--ignore-errors inconsistent` from the extract step, the
    `--list` step and the `--summary` read of `tools/dbc/coverage_gate.sh`.
    Verify: `grep -rn 'ignore-errors' cmake tools` names the class at
    the capture list only, and `mismatch` and `negative` stand there.
  - c. Rewrite the two rationale comments so each names the repaired
    cause and the cause that stands. Verify: the prose gate in tree mode
    reports 0 findings and names the unit count it examined, and the
    spell gate passes from the repository root. The range form examines
    nothing before a commit exists, so COV-006 runs it after the squash
    with its unit count.
- [x] COV-003 Repair the data cause with semantic regression tests
      (FR-018, after COV-001 and COV-002).
  - a. Name a helper for each of the `stripTopdownColumns` closures at
    `runner.cpp:711`, `:823`, `:834`, `:841` and `:845`, and declare them
    in `source/harness/detail/internal.hpp` with their contracts. Four
    helpers carry the five closures: the nested context-row closure
    stands covered by the context helper.
  - b. Name the helper of `topdownDeepestNode` and declare it in
    `source/harness/detail/hunt_topdown.hpp` with its contract.
  - c. Move each closure body into its helper; keep each conjunction,
    each evaluation order and each message.
  - d. Drive every arm of every helper from a test that calls it
    directly, in `test/source/harness_topdown_report_test.cpp` and
    `test/source/harness_hunt_report_test.cpp`. Those two entries run on
    the acceptance host; `harness_topdown_run_test` stands `Skipped`
    there, so a test placed in it adds no trace. Verify: the two entries
    pass, and the head trace shows each helper's arms taken both ways.
  - e. Quote the violation predicate text before and after in the
    record.
    Verify: `lcov --branch-coverage --config-file /dev/null --list` and
    `--summary` exit 0 with no class on the tree of this commit, and no
    `(inconsistent)` line names a contract function record.
- [x] COV-004 Complete the coverage with legitimate input tests
      (FR-020, FR-021, FR-022; parallel with COV-003).
  - a. Run `harness_jitter_cli_test` to its end outside `ctest` in the
    instrumented tree. Verify: the binary prints
    `harness_jitter_cli_test: ok`, and the record gives the 300 s
    ceiling, the seconds the entry needs, and the two trace readings.
  - b. Close the rows a legitimate input reaches:
    `source/harness/compare.cpp:602`,
    `source/harness/latency_counters.cpp:183`, `:184`, `:193`,
    `source/harness/jitter_probe.cpp:197`, and
    `source/harness/noise_tracer.cpp:486` where the repository carries
    a seam for the tracer file. Each row lands with its test.
  - c. List every row that stays open with its file, line, arms and
    evidence, and keep the two `switch (slot.source)` `BR` markers at
    `source/counters/linux_pmu/group_io.cpp:306` and `:426`.
    Verify: the gate reads `lines.......: 100.0%` and a shortfall of 21
    or fewer, and `git grep -c LCOV_EXCL -- source include` summed over
    its files reads 84 or fewer.
- [x] COV-005 Run the full, pinned, sanitizer and cheap gates and
      record every count (SC-011, SC-012; after COV-003 and COV-004).
  - a. `sched_getaffinity` and the online cpu list, recorded first.
  - b. Full `ctest`; `taskset -c 0,1 ctest`; a four-cpu run on real
    allowed cpus; `ci-sanitize`; `ci-tsan`. Verify: each lane gives its
    passed, failed, `did not run` and timeout figures, and no
    sanitizer or race line stands in any log.
  - c. The `ci-rocky` release developer build, `ci-ubuntu` with the
    analyzers, `format-check`, `spell-check`,
    `prose_gate.py` over the range and the tree, `example_drift.py`,
    `example_run`, `time_source_gate.sh`, the DBC doc and pair gates,
    and the downstream consumer build. Verify: each exits 0 and the
    README stands at or below 1004 lines.
- [x] COV-006 Squash the branch to one commit and measure that exact
      head again (SC-009, SC-013; COV-005 first). Author and committer `OmO`; title at
    50 characters or fewer; body at 72 columns; footer
    `Refs: specs/045-merge-fixes G-25` then `Approved-by: OmO`.
    Verify: the coverage lane, the pinned lane and the sanitizer lanes
    are re-run at the squashed SHA and the record names that SHA.
- [x] COV-007 Push, open one pull request, and record the proof
      (SC-013, SC-014; COV-006 first).
    Verify: `git push -u origin fix/coverage-data-repair` then, for
    every later push, `--force-with-lease` against the expected full
    SHA; `git ls-remote origin refs/heads/fix/coverage-data-repair`
    equals `git rev-parse HEAD`; the body names the measured squashed
    SHA, the counts, the cold-row ledger and the blocked capture item.

- [x] COV-008 Remove the `inconsistent` class from the capture list and
      repair the fixture-macro record behind it (FR-023, SC-015, D-14;
      COV-002 first).
    Verify: `grep -rn 'ignore-errors' cmake tools` names the capture's
    `mismatch,negative` and nothing else; a whole-tree capture at the
    measured head prints no `(inconsistent)` line; the raw trace holds no
    function record that carries two names over two different end lines.
- [x] COV-009 Bring the branch shortfall to at most 21 rows by
      measurement, with no new marker and no widened allowance (FR-024,
      SC-016, D-15; COV-008 first).
    Verify: the gate exits 0 on the extracted trace of the measured head;
    the ledger names every row that stays open with its file, line, arm
    and evidence; the marker count stands at or below 84.
- [x] COV-010 Make `harness_context_barrier_test` answer the same way in
      every run of the thread-sanitizer tree (FR-025, SC-017; COV-005
      first).
    Verify: eight runs of the entry in that tree answer the same way; the
    entry passes in `ci-rocky`, in both pinned lanes and in `ci-sanitize`;
    no ThreadSanitizer line stands in any log.
- [x] COV-011 Record the entry count of every lane and the condition that
      moves it (FR-026, SC-018; COV-005 first).
    Verify: the record carries one count line per lane, names the entries
    a lane does not register, and cites the build condition of each.

### Dependencies and parallel order

- COV-001 stands first: it fixes the base reading every later row
  compares against.
- COV-002 follows COV-001 and precedes COV-003, so the record shows
  the refusal, then the removal, then the repair that answers it.
- COV-003 and COV-004 run in parallel: COV-003 owns
  `source/harness/runner.cpp`, `source/harness/hunt_topdown.cpp`, the
  two internal headers and the two report tests; COV-004 owns the
  input tests of `compare.cpp`, `latency_counters.cpp`,
  `jitter_probe.cpp` and `noise_tracer.cpp` and their test files. No
  file stands in both sets.
- COV-005, COV-006 and COV-007 run alone, in that order.
- COV-008 follows COV-002, and COV-009 follows COV-008: the capture class
  goes only once the shape behind it stands repaired. COV-010 runs in its
  own tree, away from the coverage tree, and COV-011 reads the lane logs
  COV-005 wrote.
- COV-006 and COV-007 run again at the head that carries COV-008 to
  COV-011, so the measured SHA, the pushed ref and the pull request all
  name that head.

### Completion rule for phase 13

A row closes only with its verify reading in
`specs/045-merge-fixes/results/coverage-data-repair.md`: the command,
the exit, the host, the tool versions and the figure. A row that stops
for an owner ruling stays open with the reason, and the pull request
body carries it.


## Phase 14: Prove the barrier wait entry and repair the FWQ sample (owner order, 2026-11-08)

**Base**: `origin/master` at `2d4d8c57975a86db6c77536f7090aba2bdaf6417`
(version 0.34.0, schema 1.16, harness SOVERSION 22, archive SOVERSION 8,
markers 84, README 1002 lines against the cap of 1004). The branch is
`fix/barrier-entry-fwq`. One squashed commit, one pull request, no
merge. No version, schema or SOVERSION change, no public header change,
no new marker, filter, allowance, retry or timeout ceiling.

Two lanes run in parallel, with one exception: the wall-time rows of
BFW-004 and BFW-005 stand taken while no other lane runs, and each row
records the load average beside it (Principle VII). The barrier lane owns
`test/source/harness_context_barrier_test.cpp`,
`source/harness/execution_context.cpp` and
`source/harness/detail/internal.hpp`. The FWQ lane owns
`source/harness/jitter_probe.cpp`,
`source/harness/detail/jitter_probe.hpp` and
`test/source/harness_fwq_ftq_test.cpp`. No file stands in both sets.

- [x] BFW-001 Reproduce the barrier fixture's behavior and separate the
      four things it holds together (FR-027, SC-025).
  - a. Read the affinity mask and the online cpu list, and record both.
    Verify: the record gives 48 online cpus and the mask the shell holds.
  - b. Run the entry as it stands: 20 direct runs in the release tree and
    20 in the thread-sanitizer tree, each scenario named.
    Verify: the record gives the pass count per tree and every failure
    line.
  - c. Write the separation: the handshake (`observer.ready`,
      `watch(observer.go)`), the entry flag (`observer.entered`), the
      status-file read (`readStatus`, `stateCharOf`,
      `selfVoluntarySwitches`) and the barrier's own behavior
      (`ContextBarrier::arriveAndWait`), each named by symbol.
      Verify: the record carries one row per thing with its symbol.
  - d. State the edges a wait-mode claim needs in release and acquire
    terms, and mark each edge held or lacking at the base.
    Verify: the record names the `m_generation` release at
    `source/harness/execution_context.cpp:865` and the three acquire
    reads at `:889`, `:893` and `:895`, and names `observer.entered` as
    the only pre-arrival edge, stored before the call.
  - e. State the run the fixture cannot rule out, in which no wait arm
    stands entered and every assertion passes.
    Verify: the record cites the measured 12-of-20 reading of
    `results/coverage-data-repair.md:621-623` beside the run, and names
    no fixed loop, wall duration, log timing or repeat count as support
    for any ordering claim.

- [x] BFW-002 Stand the wait-arm observation seam (FR-028, SC-019,
      SC-027; BFW-001 first).
  - a. Declare in `source/harness/detail/internal.hpp`, in `sg::detail`:
    a `noexcept` function-pointer observer over `sg::WaitMode`, an
    `installTestWaitArmObserver` taking `std::optional` of it, and the
    reader of the slot. Give each a contract block in the file's style,
    and carry no `SG_*` contract in the note helper itself (D-13).
    Verify: `git diff --name-only` names no file under `include/`, and
    `git grep -n 'installTestWaitArmObserver'` names the header, the
    definition and the test.
  - b. Define the slot in `source/harness/execution_context.cpp` beside
    `ContextBarrier::arriveAndWait` as a function-local static, the
    shape `placementSlot()` gives at `:128-132`, and install through an
    empty-optional clear.
    Verify: the definition stands in the same translation unit as the
    barrier, and the install writes the slot once.
  - c. Call the note once at the head of each of the three arms, inside
    the arm's block, naming that arm: `SPIN`, `YIELD`, `BLOCK`.
    Verify: `git diff` shows three call sites, none inside a loop body,
    and the arrival and release paths at `:851-871` stand unchanged.
  - d. Justify the cost at the site: one load and one branch per waiting
    arrival, nothing per spin iteration.
    Verify: the comment names the cost model of `:873-874`, and
    `test/time_source_gate.sh .` exits 0.

- [x] BFW-003 Gate the observer's arrival and add the regression cases
      (FR-029, SC-019, SC-020; BFW-002 first).
  - a. In `spinWait`, `yieldWait` and the BLOCK half of `insideOutside`:
    install the observer before the participant exists, publish the arm
    into a test atomic with release order, and gate the observer's own
    arrival on `armEntered || departed`.
    Verify: each scenario prints its arm word, and the arm word equals
    the policy's `waitMode` in every run.
  - b. Assert exactly one record per waiting participant, and clear the
    observer at the end of the scenario.
    Verify: the count stands asserted, and the clear runs through the
    empty optional.
  - c. Keep `round` as the no-wait home: the main thread fills the
    round, waits on nothing and publishes no record, and the three
    worker records stand asserted beside it.
    Verify: the scenario counts the records of the round and finds three.
  - d. State at the gate that it opens on the record or on the
    participant's departure, so a removed arm answers with a named
    failure in the same run that trips the barrier's own postcondition.
    Verify: the removed-arm substitution ends its child by `SIGABRT` and
    the scenario line names the missing record.
  - e. Run the four substitutions of FR-029 in a scratch tree, 240 runs
    each in the release tree: a SPIN participant given the BLOCK wait, a
    YIELD participant given the BLOCK wait, a participant that runs on no
    context given a spinning arm, and one arm removed.
    Verify: each substitution fails its scenario in all 240 runs, the
    record carries the patch, the command and the failure line, and the
    unmodified head passes 240 of 240 in the release tree and in the
    thread-sanitizer tree.

- [x] BFW-004 Diagnose the FWQ sample in three forms (FR-030, SC-022).
  - a. Record the time budget of the example: probe cpu set, calibration
    figures, pre-check window, ring capacity, transform points and the
    count of transform calls.
    Verify: the record gives the measured wall per part and cites
    `source/harness/jitter_probe.cpp`, `source/harness/spectrum.cpp` and
    `source/harness/detail/spectrum.hpp` by symbol.
  - b. Run the entry in the full parallel suite form, the pinned form and
    the instrumented form, and hold the child's stderr, the child's
    exit, the grant that ended the run, the cpu set, the calibration
    figures and the concurrency context of each.
    Verify: each form carries its wall time and its exit, and the record
    names which grant stands nearer.
  - c. Run the contention forms: 8, 24 and 48 concurrent copies of the
    example, and the entry beside a full suite of another tree.
    Verify: the record gives the wall per copy and the ratio against the
    idle figure.
  - d. Separate the four cost candidates of `~/scratch/bfw/fwq-research.md`
    section 6 with numbers: the serial pre-check, the calibration's work
    count, the benchmarks' iteration count, and the transform.
    Verify: the ring sweep row set stands recorded, one row per candidate
    names the measurement that rules it in or out, and the record names
    the three alternatives SC-022 lists with the line each prints.
  - e. State the verdict: the mechanism proved, and the failure
    reproduced or not on this host.
    Verify: an unreproduced failure stands written as unresolved with
    the next precise experiment and its expected reading (D-21).

- [x] BFW-005 Fold the transform once per probe cpu (FR-031, SC-021,
      SC-022, SC-028; BFW-004 first, parallel with BFW-002 and BFW-003).
  - a. Keep the transform answer of a probe cpu in a `mutable` member of
    that cpu's state, and read it on the second fold of the same state.
    The two fold sites hold a `const` pointer to that state, so the
    mutability keeps `source/harness/cli.cpp` and
    `source/harness/report.cpp` out of the change.
    Verify: `git diff --name-only` names `source/harness/jitter_probe.cpp`
    and `source/harness/detail/jitter_probe.hpp`, and neither
    `source/harness/cli.cpp` nor `source/harness/report.cpp`.
  - b. State at the site why the answer cannot go stale: the series
    stands written only inside the window, ahead of the latch line, and
    both folds run after the probe stops.
    Verify: the comment names the latch line and the two fold sites by
    symbol.
  - c. Add the regression case in `test/source/harness_fwq_ftq_test.cpp`:
    a scripted window, one fold, a sentinel written into the stored
    answer, a second fold, and the sentinel read back.
    Verify: the scenario fails on a build that recomputes, and passes on
    the fixed build, in both the release and the instrumented tree.
  - d. Measure the example before and after in the release tree and in
    the instrumented tree.
    Verify: the record gives the two wall figures and the transform call
    count per probe cpu before and after.
  - e. Keep the example, the page block and the recorded run unchanged,
    and record the ring-bound option as an owner decision with its
    measured effect.
    Verify: `python3 tools/docs/example_drift.py .` exits 0, the
    `report_shape` entries pass, and the record names the owner item.

- [x] BFW-006 Run every required lane at the squashed head (FR-032,
      SC-021, SC-023, SC-024; BFW-003 and BFW-005 first).
  - a. Squash the branch to one commit, require a clean tracked tree, and
    save `git rev-parse HEAD`.
    Verify: `git status --porcelain` prints nothing tracked, and the
    record names the head as the measured one.
  - b. Fresh release tree: configure, build, whole suite.
    Verify: the passed, failed, did-not-run and timeout figures.
  - c. `taskset -c 0,1` suite, then a four-cpu suite on real allowed
    cpus.
    Verify: each lane gives its four figures, and a refused mask stands
    recorded as blocked.
  - d. `ci-sanitize` and `ci-tsan`.
    Verify: zero `AddressSanitizer`, `runtime error` and
    `WARNING: ThreadSanitizer` lines, and the four figures per lane.
  - e. `ci-coverage` through the `coverage` target and the gate, with the
    host lcovrc out of reach.
    Verify: 100 percent lines, a branch shortfall at or below 21, at or
    below 84 markers, and the cold-row ledger with its row count.
  - f. The stress matrix, the commands the local-acceptance section of
    the spec states: 240 runs of the barrier entry with 8 in flight in
    the release tree and in the thread-sanitizer tree, and 20 `ctest`
    runs of the FWQ entry in the release tree.
    Verify: every failure stands listed, the scenario readings of SC-019
    stand reported apart from these counts, and the four substitution
    counts of SC-020 stand listed with them.
  - g. The cheap gates: format, spell, prose over the range and the tree,
    example drift, example run, time-source, the DBC documentation and
    pairing gates, the analyzer build, the downstream consumer build, and
    the SC-027 symbol reading: a `BUILD_SHARED_LIBS=ON` install under
    `$HOME/scratch` and `nm -D --defined-only` over the installed object.
    Verify: each exits 0 with its reading, no new `sg::detail` name stands
    in `.dynsym`, and the export count stands at 250.

- [x] BFW-007 Push and open one pull request (FR-033, SC-026; BFW-006
      first).
  - a. `git push -u origin fix/barrier-entry-fwq`.
    Verify: `git ls-remote origin refs/heads/fix/barrier-entry-fwq`
    equals `git rev-parse HEAD`.
  - b. Any later push: save the expected full remote SHA, require it to
    equal the last observed value, and push with
    `--force-with-lease=fix/barrier-entry-fwq:<expected-full-sha>`.
    Verify: a failed lease stops the work and stands reported.
  - c. Open one pull request whose body names the measured head, the
    evidence, the cold rows and the push proof.
    Verify: the body names the head, and no tracked file carries that
    SHA. The work merges nothing and closes no issue.
    Verify: the pull request stands open against `master` with the
    measured head as its tip, and the body names that head. The work
    merges nothing and closes no issue.

- [x] BFW-008 Re-check the exit condition of the blocking arm (D-22;
      found during BFW-006).
  - a. Record the violation: three full release suites at this tree, one
    plain and two pinned, each fail the `round` scenario with
    `a round that released without advancing the generation`, and 480
    copies at 48 in flight beside a running suite fail 480 of 480. The
    same protocol at the base answers 0 of 480.
    Verify: the record carries both counts and the protocol that produced
    them.
  - b. Hold the exit condition of the blocking arm the way the two
    spinning arms hold theirs: re-check it, and wait again inside the
    loop.
    Verify: the arm keeps its mode, its place in the chain, its read of no
    clock and its absence of any timeout (specs/020 FR-014, D-03).
  - c. Run the shapes again: 480 at 48 in flight beside a running suite,
    240 at 8 in flight, 240 at 8 in flight pinned to two cpus, 240 in the
    thread-sanitizer tree, and three full release suites.
    Verify: 0 failures in every shape, and 190 of 190 in each suite.

### Dependencies and parallel order for phase 14

- BFW-001 stands first in the barrier lane: it fixes the base reading
  every later barrier row compares against.
- BFW-002 follows BFW-001, and BFW-003 follows BFW-002: the seam stands
  before the fixture can gate on it.
- BFW-004 stands first in the FWQ lane and runs in parallel with the
  barrier lane from the start.
- BFW-005 follows BFW-004 and runs in parallel with BFW-002 and BFW-003.
  No file stands in both lanes.
- BFW-006 and BFW-007 run alone, in that order, after both lanes close.
- A tracked change after BFW-006 produces a new squashed head, and every
  required lane runs again there.

### Completion rule for phase 14

A row closes only with its verify reading in
`specs/045-merge-fixes/results/barrier-fwq-closeout.md`: the command,
the exit, the host, the tool versions and the figure. A row that stops at
a scope boundary stands open with the file it needs, and the pull request
body carries it.
