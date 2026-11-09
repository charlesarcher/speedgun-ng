# Feature Specification: Fix the counters and FWQ/FTQ gates

**Feature Branch**: `045-merge-fixes`

**Created**: 2026-11-03

**Status**: Draft

**Input**: User description: "Fix the problems that PRs #71, #72 and #73
left on master, item T-MERGE-FIX of the roadmap. Short name:
merge-fixes. Master `b1dd7670` carries red CI and coverage gaps that
three merged PRs left. The work brings every gate back to green with no
CI change, no new coverage exception, and no feature."

## Run-time binding

- Base: master `b1dd7670` (PR #73, N10, 2026-11-02): version 0.30.0,
  schema 1.13, harness SOVERSION 20, archive SOVERSION 5.
- Spec number: 045. The implement phase checks `specs/` on master, the
  remote branches and the pull request list before it creates the
  branch; when 045 is taken, the work takes the next free number and
  records it.
- Branch: `045-merge-fixes` from `origin/master` at branch time. The
  base sha and the date are recorded in this spec and in the first
  commit body.
- Version, schema and SOVERSION shall not change. This spec moves no
  public layout. When a fix must move a public layout, the work stops
  and the agent reports.
- This spec adds no option and no option value.
- Line numbers in the source below name the state at `b1dd7670`. The
  implement phase finds each item by its symbol when a line moves.

## Scope

Part A (counters, PRs #70 and #71) covers:
`source/counters/linux_pmu/` (`kernel_calls.*`, `group_io.cpp`,
`provider.cpp`), `include/speedgun-ng/counters_provider.hpp`,
`test/source/counters_pmu_provider_test.cpp`,
`test/source/counters_kernel_calls_test.cpp` and the other
`test/source/counters_*` files the fixes need.

Part B (jitter probe, PR #72, N13c and its follow-up N13c-F) covers
`source/harness/jitter_probe.cpp`,
`source/harness/detail/jitter_probe.hpp`, the FWQ/FTQ sources of spec
043, `test/source/harness_jitter_window_test.cpp`,
`test/source/harness_fwq_ftq_test.cpp`,
`test/source/harness_jitter_cli_test.cpp`,
`test/prose-gate-fixture/`, `test/CMakeLists.txt` (test entries only),
`specs/043-fwq-ftq/tasks.md`, comments in `CMakeLists.txt`.

Both parts own `specs/045-merge-fixes/`.

Out of scope for parts A and B: `.github/`, `tools/ci/`, the coverage
gate configuration, any version number, any CI change. `tools/dbc/` and
`cmake/` carry no change in parts A and B; phase 13 names the two files
of those directories it edits.

Part A and part B land in separate commits. Each commit carries one
topic and the tests of that topic. "Passes the gates alone" means the
commit's own build passes and its topic's tests pass; the hard
coverage gate binds at the PR head, and each commit records its line
and branch counts in the PR body.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - A green master on every host (Priority: P1)

The owner wants every `ctest` entry to pass on thrawn, on corsairone
and in each CI job. A red result then means a real defect and never a
host difference.

**Why this priority**: a red master blocks every later change and
hides real defects behind host noise.

**Independent test**: run the full suite and the suite under
`taskset -c 0,1` on both hosts; every entry passes.

**Acceptance Scenarios**:

1. **Given** the fix branch at its head, **When** the full `ctest`
   runs on thrawn and on corsairone, **Then** every entry passes, or
   a failure stays only when the record names it with its host
   artifact reason.
2. **Given** the same head, **When** the CI test, sanitize, tsan and
   coverage jobs run, **Then** every job passes (CI is information
   only; G-20).

### User Story 2 - Unit tests that do not read the host (Priority: P1)

A developer wants counters tests that script the CPU identity and the
kernel answers. The same test then gives the same result on AMD, on
Intel, in a VM and with any `perf_event_paranoid` value.

**Why this priority**: host-dependent tests fail on machines the
author never saw, which is the root cause of the red jobs this spec
repairs.

**Independent test**: run `counters_pmu_provider_test` and
`counters_kernel_calls_test` on a host whose CPU matches no vendored
table and on a host with `perf_event_paranoid` set to `-1`; both pass.

**Acceptance Scenarios**:

1. **Given** a host with no vendored CPU row, **When** the provider
   test runs, **Then** it passes, because the test scripts the CPU
   identity through the seam.
2. **Given** `perf_event_paranoid = -1`, **When** the kernel calls
   test runs, **Then** it passes, because the test scripts the kernel
   answers through the seam.

### User Story 3 - A jitter probe that holds its samples and has no race (Priority: P1)

A user of `--jitter-probe=...,method=fwq|ftq` wants the window to hold
the samples of the watched entry, with no undefined behaviour and no
data race, so the report carries a real jitter record.

**Why this priority**: a report with `"jitter": null` is a broken
feature, and a race is undefined behaviour.

**Independent test**: `harness_jitter_window_test` passes in every
job; the sanitize and tsan presets report no finding in the jitter
tests.

**Acceptance Scenarios**:

1. **Given** the scripted clock of the window test, **When** the FWQ
   probe runs, **Then** the window holds the samples of the watched
   entry and the document carries a real jitter record.
2. **Given** the tsan preset, **When** `harness_jitter_cli_test`
   runs, **Then** no race is reported.

### User Story 4 - Full coverage with no new exception (Priority: P2)

The owner wants 100 percent line coverage and a branch shortfall of 21
or less over the whole tree, with fewer markers, never more.

**Why this priority**: the coverage gate is a hard gate; a shortfall
above the allowance blocks every merge.

**Independent test**: `ci-coverage` with the gate passes on thrawn;
the marker count by file falls from 94, or stays with a reason for
each kept marker.

**Acceptance Scenarios**:

1. **Given** the fix head, **When** the coverage gate runs, **Then**
   lines sit at 100 percent and the branch shortfall reads 21 or less.
2. **Given** the fix head, **When** the marker scan runs, **Then** the
   tree carries no new `LCOV_EXCL` line and every kept marker has a
   reason.

### User Story 5 - Correct records (Priority: P3)

A reader wants the spec 043 task list and the `CMakeLists.txt`
comments to tell the truth about what landed and when.

**Why this priority**: a wrong record misleads the next maintainer,
and the repair costs one comment edit.

**Independent test**: the spec 043 Phase 10 rows carry their gate
results, and the SOVERSION comment names PR #70 as the change that
moved the archive SOVERSION to 5.

**Acceptance Scenarios**:

1. **Given** spec 043, **When** a reader reads Phase 10, **Then**
   every row is ticked with its gate result or left open with the
   reason.
2. **Given** `CMakeLists.txt`, **When** a reader reads the harness
   SOVERSION comment, **Then** the comment names PR #70.

### Edge Cases

- A host whose CPU identity matches no vendored table: the provider
  test scripts the identity and passes.
- A host with `perf_event_paranoid = -1`, where the kernel refuses
  nothing: the kernel calls test scripts the refusal and keeps one
  real-call check only for a result every host gives.
- A scripted delta that turns negative under UBSan: the delta is
  computed in a signed type and clamped.
- A parallel `ctest -j` run against the 10-second prose fixture
  bound: all fixtures run in one process, and the bound stays.
- A fix that must move a public layout: the work stops and reports.
- The probe dropping a sample that a real run would keep: the probe
  changes, with the reason in the commit body.

## Requirements *(mandatory)*

### Functional requirements

**Part A: counters** (Refs G-19, G-21, T-LCOV-3)

- **FR-001**: When a test host carries a CPU identity absent from
  every vendored table, `counters_pmu_provider_test` shall pass. The
  test shall script the CPU identity through the internal KernelCalls
  seam. The seam shall gain one internal field that returns the
  `CpuidWords` pair that feeds `pmuIdentCurrent`; the real build fills
  that field from CPUID. `pmuIdentCurrent` shall drop its static
  ident cache and shall read the field on each call, because the
  ident sits on cold paths and a cache that ignores the install
  silently reads the host. The counter-limit register probe shall
  stay real, and no test shall assert any value derived from it; the
  watchdog pattern applies. The commit body shall state the reason.
- **FR-002**: When a test host sets `perf_event_paranoid` to `-1`,
  `counters_kernel_calls_test` shall pass. The test shall script the
  kernel refusal through the seam. A real `perf_event_open` call
  shall appear in a test only when the test asserts a result that
  every host gives; this allowance carves the one exception over
  FR-013, and the malformed-attribute refusal check stays.
- **FR-003**: When the marker repair lands, the tree shall carry no
  marker that PR #71 added or widened:
  - `include/speedgun-ng/counters_provider.hpp` shall lose the
    `LCOV_EXCL_LINE` on the defaulted copy constructor (a gcc 16
    host record).
    - `source/counters/linux_pmu/group_io.cpp` shall carry
    `LCOV_EXCL_BR_LINE` on the two `switch (slot.source)` lines in
        the place where `LCOV_EXCL_LINE` sits now. This swap is a
    narrowing replacement on the same lines, never an addition:
    FR-015 and SC-003 ban net-new exclusions and new exception
    classes, and the ledger records the swap. The swap stands only
    when both lines stay line-covered at the fix head; when a line
    would sit dark, the work covers it through a scripted seam answer
    or keeps the `LCOV_EXCL_LINE` in the kept list with its reason.
    - Every `gapped = true` line a scripted answer reaches (the group
    short read and the refused fast read) shall be covered by a
    scripted gap through the seam. When no scripted answer reaches a
    line (the unstable-pair arm, whose page rewrite no test can
    force), the kept block it belongs to shall appear in the kept
    list with its reason, and the tree shall gain no new marker.
- **FR-004**: When the coverage gate runs after FR-001 to FR-003,
  every counters line that sits under no kept marker shall be covered
  on thrawn, on corsairone and in CI; a kept marker excludes its
  lines from the coverage denominator, and the marker ledger records
  the exception. The PR #71 coverage job read lines 16051 of 16059
  (`group_io.cpp` 2, `provider.cpp` 6) and a branch shortfall of 25.
  The thrawn coverage run at `d214be37` read one dark counters line:
  `source/counters/linux_pmu/topdown_select.cpp:567`, the `return
  false` arm of the `shapeHolds` readback that asks whether a node
  names a published row of the table. Ruling G-22 R2 takes that arm
  out and keeps the clause in the `SG_ENSURE` postcondition of
  `topdownTree`, where the contract states it. The arm stands
  unreachable: the metric layer refuses the second row of a duplicated
  name with `MetricRefusal::COLLISION`, so the first row of a name is
  the only row of that name the layer can publish, and the tree pass
  adds a node only for a row its `published` flag states. The fold
  adds no marker and no test, and every counters line under no marker
  shall read covered.
- **FR-005**: When the fix lands, `git grep -c LCOV_EXCL -- source include`
shall read less than 94, or shall read 94 with a written reason for each kept
marker. Each marker a scripted answer now reaches shall be removed. The six
markers G-21 keeps shall stay: four in `dbc.hpp` on the abort path and two in
`simulation/marker.cpp` on the comment-only `#else` branch. The PR body shall
give the count by file and the kept list, one reason per marker.

**Part B: jitter probe** (Refs specs/043-fwq-ftq N13c-F)

- **FR-006**: When `harness_jitter_window_test` runs with its scripted clock,
the FWQ probe window shall hold the samples of the watched entry, the document
shall carry a real jitter record (the stats array and the series array with a
sample count above 0; `passCount` rides the metric gate and reads 0 for an entry
that carries no metric column), and the test shall use no real clock. The test
pacing shall put every scripted sample inside the window: the paced clock shall
guarantee that every scripted sample's recovered stamp falls inside the timed
window that spanned it, added beneath the existing jump and restore hand-offs.
The pinned gap magnitudes stand unchanged; the console disclosure figures (the
loop period and the threshold) follow the paced cadence, because the probe
measures them from the standing step. The probe shall change only when the probe
drops samples that a real run would keep, with the reason in the commit body.
(PR #73 run 41576309608 left the document with `"jitter": null` and `passCount: 0`; the
scripted clock ran 100000 iterations in about four microseconds.)
- **FR-007**: When the scripted plan runs the sine arm of
    `harness_fwq_ftq_test`, the sine arm shall compute the delta in a
  signed 64-bit type (`windowStep` plus the amplitude times the sine,
  truncated toward zero, the value the standing path answered) and
  shall clamp the result at 0. The sanitize job read the
  conversion to `unsigned long` at
  `test/source/harness_fwq_ftq_test.cpp` line 237. The sanitize and
  tsan presets shall report no finding in the jitter tests.
- **FR-008**: When the probe thread finishes, the main thread shall
  read the series figures only after the join. The probe thread shall
    write the series into its own result object. The mode=both
  pre-check record shall read a latch-time snapshot object that the
  probe thread copies at the latch line, one per cpu, holding
  everything the mid-run fold reads: the prefix, the dropped count,
  the calibration figures and the kept-at-latch figure the fold
  derives from them. Each per-cpu snapshot shall be published once at
  its latch through one atomic hand-off at the latch and fold sites;
  those sites sit off the hot path. The join rule governs the
  run-level and per-benchmark folds. The hot path shall gain no lock
  and no atomic. The mid-run fold lives in `source/harness/cli.cpp`,
  which counts as a spec 043 FWQ/FTQ source and sits inside Part B
  scope. (The tsan job read the main thread
  reading `seriesFigures` at `source/harness/jitter_probe.cpp` line
  708 while the probe thread wrote the same state.)
- **FR-009**:
  When `prose_gate_fixtures` runs, it shall run every fixture in one
  process, by importing the gate as a module, while the whole-tree
  scan of the repository keeps its own subprocess run. No assert in
  the fixture shall read a wall clock, and the entry shall carry no
  time bound of its own. The fixture shall tally every gate
  invocation, and two asserts shall hold that tally. The first shall
  assert the gate invocations of each fixture phase against the count
  that phase's design prescribes: 32 for `--check both`, made of 4
  loader mutants, 1 loader clean load, 1 prose stage, 1 marker, 1
  authorship, 1 clean tree, 2 parity, 1 whole tree, 19 commit runs and
  1 mixed-range run. The second shall assert, for every tree-mode run,
  that the source, unit and skip counters it reports equal the
  counters the gate's own discovery and extraction name over that same
  tree, and that the run exits 0 or 1. At this head the whole-tree run
  answers 991 sources and 186350 units, the prose stage 5 sources and
  61 units, the marker stage 1 source and 4 units, the clean tree 1
  source and 27 units, and the loader clean load 0 sources. (The 10
  second bound measured the host: the same scan took 0.062 s under
  plan R-04 and 9.8 s on corsairone, and the entry bound that 6025a71f
  added measured 12.7 s there.)
- **FR-010**: When the implement phase runs Phase 10 of
  `specs/043-fwq-ftq`, every row shall be ticked only when its gate
  passes, and a row whose gate fails shall stay open with the reason.
  The lane shall run every Phase 10 gate it can execute. A row whose
  verify text contradicts the 045 allowances (T050's shortfall of 0)
  or reads an artifact of the 043 lane (T057's acceptance record,
  T061's push) shall stay open with a reason that names the 045
  allowance or the owning lane, and no 043 verify text shall change.
  PR #72 left Phase 10 unrun.
- **FR-011**: When the coverage gate runs after part A and part B,
  every line of the PR #72 code shall be covered by real tests and
  the branch shortfall over the whole tree shall be 21 or less. The PR
  #72 coverage state read 44 dark lines and 52 short branch positions
  (T070). SC-002's 100 percent governs strictly: every PR #72 line
  shall be covered by a real test, and the kept-list remedy applies
  only to lines already under a kept marker, so for Part B it never
  fires. A PR #72 line that no real test can reach stops the work and
  the agent reports, like the public-layout rule. The PR body shall
  give the number.
- **FR-012**: When a reader reads a `CMakeLists.txt` comment about the
  harness SOVERSION, the comment shall name PR #70 (T-LCOV) as the
  change that moved the archive SOVERSION to 5, never spec 044. The
  change shall touch comments alone; no number shall change. The fix
  rides in the Part B commit that carries the record fixes, with the
  Part B footer, because Part B scope owns the `CMakeLists.txt`
  comments.

**Common**

- **FR-013**: When a seam-driven counters test runs (the
  `kernel_calls`, `pmu_provider`, `pmu_window` and
  `linux_pmu_seam` tests and every other counters test the fix
  touches), it shall use no sleep, no
  clock read, no real CPU identity, no sysfs, no procfs and no real
  `perf_event_open` answer. `counters_pmu_test` stays the suite's
  host-aware integration check with its skip-with-reason design. The
  watchdog file read inside a provider enumerate stays real, because
  no test asserts its value. Part B tests are governed by FR-006 to
  FR-009: their scripted clocks, real threads and the runner's
  wall-time self-measurement stand as those requirements state them.
  The suite shall pass under `taskset -c 0,1`.
- **FR-014**: When an option value is bad, the program shall exit 2
  with one quoted line. User-visible behavior shall change by the
  fixes above alone; this spec shall add no option and no option
  value.
- **FR-015**: When the fix lands, the tree shall carry no exclusion
  marker, no `--omit-lines` pattern, no `--ignore-errors` setting and
  no `--remove` setting added for coverage; "added for coverage"
  means net-new exclusion lines and new exception classes, so the
  FR-003 narrowing swap stands outside the ban.   The accepted exceptions
  (R-1, R-2, the W-1 omit patterns, branch allowance 21) shall stay
  as they are. The time-source gate (`test/time_source_gate.sh`)
  shall pass: the bound files of Part B introduce no new time
  source.

### Key Entities

- **KernelCalls seam**: the internal test seam of the counters
  library. It carries the scripted CPU identity words and the
  scripted kernel answers; the real build fills it from CPUID and
  the real kernel.
- **Jitter window**: the sample window of the FWQ/FTQ probe for one
  watched entry. It holds the samples the report reads.
- **Marker ledger**: the count of `LCOV_EXCL` markers by file, with
  the kept list and one reason per kept marker.
- **Spec 043 Phase 10 rows**: the task rows of
  `specs/043-fwq-ftq/tasks.md` that carry gate results.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Full `ctest` and `ctest` under `taskset -c 0,1` pass on
  thrawn and on corsairone, with zero unexplained failures. A failure
  stays only when the record names it and shows its host artifact
  reason, for example `loop_shape` on GCC 14 on the box.
- **SC-002**: `ci-coverage` with the gate on thrawn reads 100 percent
  lines; the branch shortfall reads 21 or less, with the number
  given; no unexplained dark line sits in the tree.
- **SC-003**: `git grep -c LCOV_EXCL -- source include` reads less
  than 94, or reads 94 with a reason for each kept marker. No new
  marker line sits in the tree.
- **SC-004**: The sanitize and tsan presets with the suite report no
  finding.
- **SC-005**: `cmake -P cmake/lint.cmake`, `cmake -P cmake/spell.cmake`, `tools/docs/example_drift.py`, `tools/prose/prose_gate.py --base origin/master`
and `tools/dbc/dbc_pair_gate.py` pass.
- **SC-006**: `prose_gate_fixtures` passes in a parallel `ctest -j`
  run within its 10-second bound.
- **SC-007**: Every spec 043 Phase 10 row is ticked with its gate
  result, or left open with the reason.
- **SC-008**: `.github/`, `tools/ci/`, the version, the schema and the
  SOVERSION carry no change. `tools/dbc/` and `cmake/` carry no change
  in phases 0 to 12; phase 13 names the two files there that it edits.

## Local acceptance (G-20)

Acceptance is local only. CI on a GitHub runner is information, never
acceptance. The acceptance run happens on corsairone first, at the
PR head; the thrawn-side runs are the development and stand-in runs
that lead to that head. For each host, the PR body carries the host name, the
kernel, the GCC version and the head sha, with:

1. full `ctest`; `ctest` under `taskset -c 0,1`; each failure named,
   with its host artifact verdict;
2. `ci-coverage` with the gate: line percent, branch percent,
   shortfall against 21, dark lines by file;
3. `git grep -c LCOV_EXCL -- source include` by file;
4. the sanitize and tsan presets with the suite;
5. the five tools of SC-005.

corsairone runs first. The local lcov repair (perl XS mismatch between
`~/.local/lib/perl5` and `/usr/bin/perl`) happens on the host alone,
never in the repository. When the repair fails, or when corsairone is
wholly unreachable, the PR body carries the stand-in host's counts
for all five items and states the corsairone gap: a failed repair
means coverage comes from thrawn only, and an unreachable host means
the thrawn-side counts stand for the whole acceptance.

## Commit and record rules

- Every text the change generates follows ASD-STE100 Simplified
  Technical English and Principle XI.
- Commit author and committer: OmO. No person name sits in any field
  or message, and no person name sits in the repository, except the
  copyright line of LICENSE (G-17). This lane checks its own records:
  `git grep -nE 'arche[r]c|char[l]es|@gmai[l]' -- specs/045-merge-fixes`
  prints nothing; the bracket classes stop that line from matching itself. The
  194 matches in fourteen other feature specs and in the prose-gate fixture
  data under `test/` move to ticket T-NAMES (owner ruling, 2026-11-04), with
  the 6 in the CI workflow, `CMakeLists.txt` and `CODEOWNERS`.
- Commit titles: `<Section>: <one-line imperative>` at 50 characters
  or fewer, imperative mood ("Add X", never "Added X"). Body lines: 72
  or fewer, and the body states the why of the change. Blank lines
  between paragraphs. Issue references use a GitHub keyword when an
  issue exists.
- Footer block: the last two lines, no blank line between them, as real git
trailers. Part A commits: `Refs: G-19 G-21 T-LCOV-3`. Part B commits: `Refs: specs/043-fwq-ftq N13c-F`; the commit that ticks the
spec 043 Phase 10 rows rides in the Part B commit whose work it records.
Spec-file commits, meaning commits that touch `specs/045-merge-fixes/` only,
carry `Refs: specs/045-merge-fixes`. No mixed commit arises in parts A and
B: the `specs/045-merge-fixes/` files land only in spec-file commits, and
the Part A and Part B commits touch only their code and tests, one topic
per commit. Then `Approved-by: OmO`. Phase 13 replaces that rule for
itself: its one squashed commit carries its code, its tests and its
`specs/045-merge-fixes/` records together, and its footer is the phase 13
form.
- History: rebase only, no merge commits, no CI change, no rewrite of
  master history.
- First push: `git push -u origin 045-merge-fixes`. Each later push:
  `git push --force-with-lease=045-merge-fixes:<sha>`, then
  `git ls-remote origin 045-merge-fixes` must equal
  `git rev-parse HEAD`. When the lease fails, stop and report.
  Never merge.
- PR title: "Fix the counters and FWQ/FTQ gates". Last two lines of
  the PR body:
  `Refs: specs/045-merge-fixes G-19 G-21 T-LCOV-3 specs/043-fwq-ftq N13c-F`
  then `Approved-by: OmO`.

## Decisions

The feature description recommends one answer per open question, and
this spec takes each one:

- **D-1 (FR-001 seam field)**: one new field in the internal
  KernelCalls seam returns the `CpuidWords` pair, and
  `pmuIdentCurrent` drops its static cache and reads the field on
  each call. The seam is internal (detail), so no public layout moves
  and no SOVERSION changes.
- **D-2 (FR-002 real call)**: script the refusal through the seam.
  One real-call check stays only when it asserts a result every host
  gives.
- **D-3 (FR-003 gapped)**: cover with a scripted gap through the seam
  every `gapped = true` line a scripted answer reaches. When no
  scripted answer reaches a line, list the kept block with its
  reason. Add no new marker.
- **D-4 (FR-006 pacing)**: change the scripted clock so each
  iteration advances the time by a step that puts the samples inside
  the window; the clock guarantees that every scripted sample's
  recovered stamp falls inside the timed window that spanned it,
  added beneath the existing jump and restore hand-offs, with the
  expected magnitudes unchanged. Change the probe only when the
  probe drops samples a real run would keep.
- **D-5 (FR-008 hand-over)**: the probe thread writes the series into
  its own result object; the main thread reads it after the join.
    The mode=both pre-check record reads a latch-time snapshot object
  the probe thread copies at the latch line, one per cpu, holding
  everything the mid-run fold reads, published through one atomic
  hand-off at the latch and fold sites, which sit off the hot path.
  `source/harness/cli.cpp`, the file of the mid-run fold, counts as a
  spec 043 FWQ/FTQ source. No atomic and no lock in the hot path.
- **D-6 (FR-009 fixtures)**: run all fixtures in one Python process, by
importing the gate as a module, every gate call except the timing case, which
keeps its subprocess form. The 10-second bound measures the whole entry's wall
time in a parallel run, and the runner times itself and fails above it.
- **D-7 (FR-005 kept markers)**: keep a marker only when no scripted
  seam answer reaches the line. A marker no ruling covers stays in
  the kept list as "waits for ruling"; the work invents no ruling.
- **D-8 (spec files)**: `specs/045-merge-fixes/` holds the seven
  artifacts (`spec.md`, `plan.md`, `tasks.md`, `research.md` with its
  two annexes, `data-model.md`, `quickstart.md`),
  `checklists/requirements.md`, and `results/` with the acceptance
  counts of each host. No contracts directory, because no interface
  changes.
- **D-9 (version)**: no version change. This is a fix spec with no
  feature.

## Clarifications

### Session 2026-11-03

- Q: How does the mode=both pre-check record become race-free, given
  the join rule cannot apply to it? (FR-008, D-5) → A: the probe
  thread copies the prefix, its dropped count and the calibration
  figures into a dedicated per-cpu snapshot object at the latch line;
  the pre-check fold reads only that object mid-run, and the join rule
  keeps governing the run-level and per-benchmark folds. (The tsan
  finding is the mid-run fold at `source/harness/cli.cpp` line 1384
  over `seriesKeptAtLatch`, which the probe thread writes after its
  latch arrival.)
- Q: Which spec 043 Phase 10 rows can the 045 lane tick? (FR-010) →
  A: the lane runs every Phase 10 gate it can execute and
  ticks what passes; rows whose verify text contradicts the 045
  allowances or reads an artifact of the 043 lane stay open with a
  reason that names the allowance or the owning lane, and no 043
  verify text changes.
- Q: What does the 10-second prose fixture bound measure? (FR-009) →
  A: the whole entry's wall time in a parallel run; the runner times
  itself and fails above 10 seconds, and the internal gate-run assert
  stays untouched.
- Q: What invariant must the paced scripted clock satisfy? (FR-006) →
  A: every scripted sample's recovered stamp falls inside the timed
  window that spanned it, with the pacing added beneath the existing
  jump and restore hand-offs and the expected magnitudes unchanged.
- Q: Which concrete form does the FR-007 fix take? (FR-007) → A: the
  sine arm computes the delta in a signed 64-bit type and clamps the
  result at 0.

- Q: Which magnitudes stand unchanged under the paced clock?
  (FR-006) → A: the pinned gap magnitudes; the console loop-period
  and threshold figures follow the paced cadence, because the probe
  measures them from the standing step.

- Q: What does the new KernelCalls seam field return, and how does
  the ident cache behave? (FR-001, D-1) → A: one field returns the
  `CpuidWords` pair that feeds `pmuIdentCurrent`; `pmuIdentCurrent`
  drops its static ident cache and reads the field on each call, and
  the counter-limit register probe stays real.
- Q: Does the FR-013 ban bind every counters test, and what happens
  to the watchdog file read? (FR-013) → A: the ban binds the
  seam-driven tests; `counters_pmu_test` stays the host-aware
  integration check with its skip-with-reason design, and the
  watchdog read stays real because no test asserts its value.
- Q: Which requirement governs the real `perf_event_open` check?
  (FR-002, FR-013) → A: FR-002 carves the exception over FR-013, and
  the malformed-attribute refusal check stays.
- Q: Which `gapped = true` lines does FR-003 name? (FR-003, D-3) → A:
  every line a scripted answer reaches (the group short read and the
  refused fast read); the unstable-pair arm keeps its marker with a
  ledger reason.
- Q: Does US-1 acceptance require zero failures? (US-1, SC-001) → A:
  zero unexplained failures; a failure stays only when the record
  names it with its host artifact reason.

- Q: Does the mode=both snapshot carry the kept-at-latch figure, and
  is `source/harness/cli.cpp` in scope? (FR-008, D-5) → A: the
  snapshot carries everything the mid-run fold reads, the kept figure
  folds from the snapshot's prefix and dropped count, and `cli.cpp`
  counts as a spec 043 FWQ/FTQ source inside Part B scope.
- Q: What orders the probe thread's latch-time copy before the fold's
  read? (FR-008, D-5) → A: each per-cpu snapshot is published once at
  its latch through one atomic hand-off at the latch and fold sites,
  which sit off the hot path.
- Q: Does the coverage gate bind at every commit or at the PR head?
  (FR-011, SC-002) → A: at the PR head; "passes the gates alone"
  means the commit's own build and its topic's tests pass, and each
  commit records its counts in the PR body.
- Q: What happens when a PR #72 line cannot be covered by a real
  test? (FR-011) → A (revised in round 3; the first answer
  contradicted SC-002): every PR #72 line shall be covered by a real
  test, and a line that no real test can reach stops the work and is
  reported, like the public-layout rule; the kept-list remedy applies
  only to lines already under a kept marker.
- Q: Which trailer does the Phase-10 ticking commit carry? (commit
  rules) → A: the Part B footer `Refs: specs/043-fwq-ftq N13c-F`,
  riding in the Part B commit whose work it records; "spec-file
  commits" means commits touching `specs/045-merge-fixes/` only.

- Q: Do kept-marker lines sit inside the coverage denominator?
  (FR-004, SC-002) → A: no; a kept marker excludes its lines from the
  denominator, and FR-004 and SC-002 read "every line not under a
  kept marker", with the marker ledger as the record.
- Q: What rule governs the real counter-limit register probe?
  (FR-001, FR-013) → A: the watchdog pattern applies: the probe stays
  real and no test asserts any value derived from it.
- Q: Does FR-013 bind the Part B jitter tests? (FR-013) → A: no;
  FR-013 governs the seam-driven counters tests, and Part B tests
  stand under FR-006 to FR-009.
- Q: What happens when corsairone is wholly unreachable? (local
  acceptance) → A: the same fallback as a failed lcov repair: the
  stand-in host's counts for all five items, with the PR body stating
  the corsairone gap.
- Q: Is the `LCOV_EXCL_LINE` to `LCOV_EXCL_BR_LINE` swap a forbidden
  addition? (FR-003, FR-015) → A: no; the swap is a narrowing
  replacement on the same lines, and FR-015 bans net-new exclusions
  and new exception classes.

- Q: Which order do the two acceptance runs take? (local acceptance)
  → A: corsairone runs first at the PR head; the thrawn-side runs are
  the development and stand-in runs that lead to that head.
- Q: How does an unreachable PR #72 line avoid breaking the 100
  percent reading? (FR-011, SC-002) → A: SC-002 governs strictly; a PR #72 line
  that no real test can reach stops the work and is reported.
- Q: When does the LINE to BR swap stand? (FR-003, FR-004) → A: only
  when both switch lines stay line-covered at the fix head; a dark
  line takes a scripted seam answer or keeps the LINE marker with a
  ledger reason.
- Q: Which commit carries the FR-012 comment fix? (FR-012) → A: the
  Part B commit that carries the record fixes, with the Part B
  footer.
- Q: Which footer does a commit that touches code and spec 045 files
  together carry? (commit rules) → A: none, because no mixed commit
  arises; the spec 045 files land only in spec-file commits.

## Assumptions

- The roadmap file
  `/workspace/speedgun-ng-harness-roadmap.md` (item T-MERGE-FIX,
  rulings G-17, G-19, G-20, G-21) sits outside this repository and is
  absent on this machine; the feature description carries the
  operative content, and the rulings are cited from it.
- thrawn and corsairone are machines the owner controls. A run on
  this machine stands in for thrawn; corsairone runs when reachable,
  and its absence is reported under the fallback the acceptance
  section states.
- `gh` carries no credentials on this machine, so the pull request
  opens when the owner runs it; the branch and its pushes carry the
  work.

## Open, owner

These wait for the owner. The work changes nothing for them, and this
spec records no ruling for them.

- **O-1 Authors**: the 202 commits of PRs #71 to #73 on master carry a
  person name as git author. A change needs a rewrite of master
  history. The work does not rewrite; new commits use OmO.
- **O-2 SOVERSION**: PR #73 moved public layouts (the `benchmark.hpp`
  top-down record, `MetricView` and `MetricListing` threshold text)
  and kept harness SOVERSION 20 and archive 5; its comment says "no
  number moves twice" in an unreleased window, and no recorded ruling
  says this. Keeping 20 or moving to 21 is the owner's decision. The
  work changes no number.

## Constitution Checklist

- Main-page impact check (IV.1): no page change. The main page
  already documents the jitter record the fix restores, and no
  feature lands.


## Phase 13: Repair the coverage-data fault (owner ruling G-25, 2026-11-07)

**Input**: Owner ruling G-25 of 2026-11-07. Remove
`--ignore-errors inconsistent` from the repository coverage commands
and fix the coverage-data fault behind it. The evidence of this phase
keeps the class on one read, the capture, and O-4 carries that
exception for the owner. The ruling names pull
request #89 as the change that added the class to the extract, list
and summary reads, and records that the same complaint already stands
in the capture. The evidence of record is the fault itself:
`lcov: ERROR: (inconsistent) "source/harness/runner.cpp":708: function
'_ZZN2sg6detail19stripTopdownColumnsE..._clEm' is hit but no contained
lines are hit`, read on the continuous integration coverage job of
pull requests #86, #87 and #89, and the partial coverage union of
specs/048-hwlat-probe.

This phase reuses spec 045. It adds no numbered spec and replaces no
earlier requirement, task row, ruling or record of this file, except
that it narrows the two lines that freeze `cmake/` and `tools/dbc/` to
the phases those lines bound. The
description's FR-001 to FR-007 are this phase's FR-016 to FR-022; its
US-1 and US-2 are this spec's US-6 and US-7. The task rows carry the
IDs COV-001 to COV-007 in `tasks.md`.

### Run-time binding (phase 13)

- Base: `origin/master` at `615c0964547e4e9768b8e2f2d79991033a0eb5bd`
  (version 0.34.0, report schema 1.16, harness SOVERSION 22, archive
  SOVERSION 8, `git grep -c LCOV_EXCL -- source include` at 84, README
  1002 lines against the cap of 1004). No open pull request stood at
  the live check of 2026-11-07. The implement phase rechecks the base
  at branch time and records the reading.
- Branch: `fix/coverage-data-repair`, cut from the latest
  `origin/master`. `615c0964` shall be an ancestor of `origin/master`
  at cut time. The branch stood absent on the remote at the live
  check; when it exists, the work inspects its head, reports, and
  does not overwrite or reuse it. Old merged branches stay
  unresumed. The base SHA stands in the record.
- One branch, one squashed commit, one pull request. The work never
  merges, closes an issue, or claims a new ruling.
- Version, schema, harness SOVERSION and archive SOVERSION shall not
  change. No public API change is planned. When a repair must move a
  public layout, the work stops and reports.
- Target host: thrawn, AMD Zen 5 Threadripper 9960X, 24 cores, 48
  threads. Acceptance is local (G-20). CI results are diagnostics.
- Line numbers below name the state at `615c0964`. The implement phase
  finds each item by its symbol when a line moves.

### Scope (phase 13)

In scope:

- `specs/045-merge-fixes/`, its `results/` records, and its quickstart.
- The four reads that carry the `inconsistent` class at the base: the
  capture list, the extract command and the `--list` step of
  `cmake/coverage.cmake`, and the `--summary` read of
  `tools/dbc/coverage_gate.sh`. The phase removes the class from the
  three reads of the written trace and keeps it on the capture under
  O-4. The `genhtml` step of the same target carries no class at the
  base and proves the repair.
- The contract predicate sites the evidence names: every contract
  predicate in the extract roots whose predicate builds a closure, the
  source files that hold one, and the regression tests of those files
  under `test/`.
- The cold rows of the FR-022 ledger, the source files that hold them,
  and the tests that hand those files their legitimate input.

Out of scope:

- The records and task rows of `specs/041-counter-probe/`,
  `specs/046-hunter-topdown/` and `specs/048-hwlat-probe/`.
- Trace-line parsing (G-28, approved and standing), raw codes (G-31)
  and automatic ranks (G-32), lifecycle follow-ons, and name cleanup
  (T-NAMES). G-28 stays approved work of its own lane; this repair
  does not start it, and G-31 and G-32 stay future features.
- Any gate, filter, allowance, marker or CI setting beyond the four
  reads named above. A change to an unrelated gate or CI setting stops
  the work for owner approval.

### User scenarios (phase 13)

#### US-6 A maintainer reads a consistent trace (Priority: P1)

A maintainer runs the coverage target on a fresh instrumented tree and
reads one consistent full trace. No read of the route ignores an
inconsistent record, so the figures the gate prints are the figures of
a trace the tool accepted line by line.

**Why this priority**: the coverage gate is a hard gate (VI). A figure
read from a trace the tool rejected carries no evidence.

**Independent test**: `cmake --preset=ci-coverage`, the suite in
`build/coverage`, then `cmake --build build/coverage -t coverage`:
every step exits 0, and no step prints an `(inconsistent)` line.

**Acceptance scenarios**:

1. **Given** a fresh instrumented tree at the phase head, **When** the
   capture, the extract, the list and the summary read run with no
   `inconsistent` class anywhere in the route, **Then** each step
   exits 0 and prints its figures.
2. **Given** the same run, **When** the reader greps the log for
   `(inconsistent)`, **Then** the grep prints no line that names a
   contract function record. Every other `(inconsistent)` line stands
   in the record with its file, its line and its count.

#### US-7 A maintainer sees every cold line and its real input (Priority: P2)

A maintainer reads the cold rows of the trace and finds, for each one,
the file, the line, the arms and the input that would take them. No
row stands closed by a waiver, an omitted path or a widened allowance.

**Why this priority**: the owner's ruling of 2026-11-05 sets the rule
for the cold rows: they stand closed by real work, and the allowance of
21 (D-14 of `specs/030-ci-speedup`) does not move.

**Independent test**: the cold-row ledger in
`results/coverage-data-repair.md` names every remaining row with its
arms and its class, and the gate reading matches the ledger.

**Acceptance scenarios**:

1. **Given** the phase head, **When** the gate reads its shortfall,
   **Then** the record gives the taken count, the total count and the
   shortfall from the `branches` line of the summary read, and the
   ledger lists the cold rows of `build/coverage/coverage.info` with
   their own row count beside that shortfall. The shortfall stands at
   21 or fewer.
2. **Given** a cold row no legitimate input reaches, **When** the
   reader looks it up in the ledger, **Then** the row names its exact
   file, line and arms beside the evidence, and stands open.

### Edge cases (phase 13)

- The first `(inconsistent)` line names one location. The route holds
  three contract predicates whose predicate builds a closure at the
  base: `source/harness/runner.cpp:708`, `source/harness/runner.cpp:821`
  and `source/harness/hunt_topdown.cpp:155`, and the record lists one
  `(inconsistent)` read record for each. The second of the three captures
  by value, so a search that looks only for `[&` misses it.
- A repair that moves a lambda out of a contract span also moves its
  line and branch records back into the denominator. The phase keeps
  the totals inside the standing gates and covers the new records with
  real tests.
- A host-local lcov configuration can relax the data-consistency
  check. `~/.lcovrc` on thrawn carries `check_data_consistency = 0`
  from 2026-11-03. The acceptance run keeps the check live and states
  how it did so; a local config bypass is not acceptance.
- A test entry that stops at its wall bound leaves the source that
  entry drives dark, and the lane that stops there reads no records for
  it. The record carries the ceiling, the entry, the step that runs the
  entry to its end, and the two readings. An incomplete capture is
  never called accepted.
- A contract predicate that reads false ends the process. A test that
  drives such an arm calls the helper directly and asserts the answer,
  so the helper stands declared in the internal header that declares
  the enclosing function; the test does not raise a contract violation.

### Requirements (phase 13)

- **FR-016**: When the phase starts, the work shall reproduce the
  fault from a fresh local instrumented build and classify it. The
  record shall preserve the raw gcov and lcov diagnostics, the tool
  versions and the exact commands. The record shall read the affected
  function records and the interaction between the contract macros
  and the `--omit-lines` filter, and shall separate stale build data,
  instrumentation defects and source shape. The reproduction reads the
  whole instrumented tree, because a capture of one translation unit
  reports `(inconsistent)` records that the merged trace of the full
  suite does not carry.
- **FR-017**: When the phase lands, no repository read of the written
  trace shall suppress the `inconsistent` class. The capture stands as
  the one read that keeps the class, under O-4. The removal covers the
  extract command, the `--list` step and the `--summary` read of
  `tools/dbc/coverage_gate.sh`. Every other existing class stays as it
  stands; the phase adds no class and broadens none. This is the narrow
  G-25 removal, and it carries no permission to change a gate or a
  filter. The capture list carries a second `inconsistent` cause, the
  end-line record of the function that `SG_BENCHMARK_F` generates,
  which `0eed39c2` recorded for specs/016. FR-016 shall classify it.
  When its repair reaches the public macro family, the work stops and
  reports under the run-time binding, and the capture list stands
  unchanged with the reason in the record.
- **FR-018**: When the cause stands known, the repair shall remove it
  without changing the contract or the run behavior. A named semantic
  helper outside an `SG_` macro is a candidate only when the evidence
  shows a contract lambda and filter mismatch. The repair shall not
  move code merely to lose counters, add a dummy path or remove a
  check. Expression evaluation, side effects, Release behavior, and the
  violation record's kind, message, file and line stand preserved. The
  predicate text in that record names the call that carries the
  constraint, and the record quotes that text before and after the
  repair. When the evidence names a different cause,
  the phase documents that cause before the repair.
- **FR-019**: When the phase head is measured, a full clean capture,
  extraction, listing and summary shall complete with no inconsistent
  data ignored at the extract, list and summary reads and at the
  `genhtml` step. The capture list keeps the class under O-4, and the
  record gives its end-line records with their files, lines and count. Clean means a `build/coverage` tree with no `*.gcda`,
  no `coverage.raw.info` and no `coverage.info`, configured with both
  compiler launchers empty, built, and run through the whole suite. The extract roots, the filters, the accepted R-1, R-2
  and W-1 patterns and the allowance of 21 stand unchanged. The pull
  request #89 diff stands audited on its own: the record enumerates
  which edits are ignored-error plumbing, which are comments, and
  whether any unrelated gate change stands in the range. A change to
  an unrelated gate or CI setting stops the work for owner approval.
- **FR-020**: When the phase measures coverage, it shall cover the
  remaining legitimate inputs with real tests. The partial union of
  specs/048-hwlat-probe, which its record reports as 46 cold branch
  records and a branch shortfall 25 over the allowance, with the jitter
  command-line trace absent, stands as a lead and never as the
  acceptance baseline. The phase runs the full suite and computes
  fresh totals. An incomplete capture is never called accepted.
- **FR-021**: When the phase lands, the tree shall keep only the two
  already-approved closed-enumeration `BR` markers on
  `switch (slot.source)` in `source/counters/linux_pmu/group_io.cpp`
  under G-29, beside the separately accepted existing exceptions. No
  new `LCOV_EXCL` marker, omit pattern, remove pattern or ignore-error
  setting joins the tree. A retry marker, an unstable-pair marker or
  the ledger's claim of a prior approval gives no new exception. The
  phase runs no blanket marker purge; a disputed class is reported for
  investigation.
- **FR-022**: When a required cold branch position is not reachable
  through the legitimate input contract, the record shall list the
  exact file, line and arms with its evidence, and the row shall stay
  open inside the allowance of 21. A cold line stops the work and is
  reported, as the ruling on FR-011 states. The phase shall not change
  the allowance and shall not invent an exception.

### Success criteria (phase 13)

- **SC-009**: On a fresh instrumented tree at the measured head, the
  capture, extract, list and summary steps each exit 0, and no step
  prints an `(inconsistent)` line that names a contract function
  record. Line coverage reads 100 percent; the branch shortfall reads
  21 or fewer on thrawn, taken from the `branches` line that
  `tools/dbc/coverage_gate.sh` reads at the measured head, with the
  same reading given at the base. The record gives the numerator, the
  denominator, the cold rows with their count and the gate exit.
- **SC-010**: `git grep -c LCOV_EXCL -- source include` summed over its
  files reads 84 or fewer, and the two `switch (slot.source)` `BR`
  markers stand as the only markers this phase touches.
- **SC-011**: The full `ctest`, the full `ctest` under `taskset -c 0,1`,
  the run on four real allowed cpus, the address and undefined-behavior
  sanitizer tree and the thread sanitizer tree report their passed figure, their failed figure and
  their `did not run` block with its `(Skipped)` and `(Disabled)`
  rows, each given separately. Every `***Timeout` row stands named
  with the ceiling it crossed and the seconds that entry needs outside
  `ctest`. A base failure is identified and left open.
- **SC-012**: The cheap gates pass at the head: the `ci-rocky` release
  developer build, format, spell, prose range and tree with the unit
  count it examined, example drift, time-source, DBC doc and pair gates,
  the analyzer build, and the downstream consumer build. README stays at
  or below 1004 lines. The remaining page-drift items, `snippet_compile`,
  `report_shape` and the two fixture suites, run as entries of the
  `ctest` of SC-011. The DBC doc gate reads the public headers alone and
  the pair gate carries no row for a `detail` declaration, so the
  contract text of an internal helper stands checked by review.
- **SC-013**: The record `results/coverage-data-repair.md` names the
  host, the CPU, the kernel, the GCC, clang, gcov and lcov versions,
  the base SHA, the measured squashed SHA, the commands, the counts,
  the coverage figures and every failure. The quickstart carries a
  reproducible command example of this repair.
- **SC-014**: The pull request body names the measured squashed SHA,
  and `git ls-remote origin refs/heads/fix/coverage-data-repair`
  equals `git rev-parse HEAD` after every push. A machine without the
  `gh` credential delivers the branch, the push proof and the body text,
  as the assumptions state.

### Local acceptance (phase 13)

Acceptance is local (G-20) on thrawn. The corsairone-first order of the
Local acceptance section above binds phases 0 to 12 and does not bind
this phase; corsairone stands unrequired here. Before any pinned run the work
inspects `sched_getaffinity` and the online cpu list and records both.
A test chooses a real allowed cpu, never an invented id. The required
lane is `taskset -c 0,1`; when cpus 0 and 1 stand outside the allowed
set, that lane is recorded as blocked, and a run on another allowed
pair is extra evidence and never a substitute pass. A run on four
real allowed cpus also stands required.

No test of this phase uses a sleep, a wall-clock read or a racing page
rewrite. The phase uses the deterministic seams the repository already
carries. A production time API is never a fake test.

The data-consistency check stands live in every read of the route,
the FR-016 reproduction among them: the run keeps the host-local
`~/.lcovrc` of 2026-11-03 out of lcov's reach and records the form it
used. No repository command gains a setting that hides the file.

### Commit and record rules (phase 13)

- History, authors and footers of earlier work stand as they are
  (G-26, G-27). New commits carry author and committer `OmO`. No
  person name and no user home path stands in a tracked record; the
  LICENSE exemption stands.
- One squashed commit carries the repair. Title: `<Section>: <one-line
  imperative>` at 50 characters or fewer. Body lines: 72 columns or
  fewer. Footer, as real trailers in order:
  `Refs: specs/045-merge-fixes G-25` then `Approved-by: OmO`. That order
  follows the landed history of this repository and stands ahead of the
  template paragraph order of the constitution, which the governance
  section resolves in favor of the written record here. The commit
  carries no `Fixes #n`: no tracking issue stands for the coverage fault,
  and the body says so. The `Approved-by:` footer records the owner's
  ruling G-25; the owner's approval at merge time stands as the approval
  of the landing commit.
- Rebase, never merge. The master history stands unrewritten.
- First push: `git push -u origin fix/coverage-data-repair`. Each
  later push saves the expected full remote SHA first, requires it to
  equal the last observed SHA, and uses
  `git push --force-with-lease=fix/coverage-data-repair:<expected-full-sha>`.
  A failed lease is reported and never refreshed and retried. After
  each push `git ls-remote origin refs/heads/fix/coverage-data-repair`
  shall equal `git rev-parse HEAD`.
- Measurement names the final squashed SHA. A commit never embeds its
  own SHA. When a records-only amend follows a measured head, the
  record discloses both SHAs and the exact diff, and the acceptance
  runs again at the final head with those counts in the pull request
  body.
- No new CI change stands in this phase.

### Decisions (phase 13)

- **D-10 (reuse)**: the phase rides spec 045 as the owner directed. A
  new numbered spec would split the coverage history of this lane
  across two features.
- **D-11 (cause first)**: the reproduction of FR-016 precedes the
  removal of FR-017 and the repair of FR-018 in the same branch, so
  the record shows the fault, then the repair, then the clean reading.
- **D-12 (narrow tooling)**: the tooling diff removes one class from
  the three reads of the written trace; the capture list keeps it under
  O-4. The `mismatch` and `negative` classes, the extract
  roots, the `--filter` set, the two `--omit-lines` patterns and the
  allowance of 21 stand untouched, because FR-019 fixes them.
- **D-13 (helper candidate)**: the named semantic helper is chosen only
  from the evidence of FR-016. The candidate shape keeps the `SG_*`
  macro, its message and its evaluation order at the call site, and
  moves the predicate body into a function that returns its answer and
  carries no contract of its own, so a test can call it with a false
  predicate and keep its process. The coverage tree builds with
  `speedgun-ng_CONTRACTS:STRING=enforce`, and `enforce` reaches
  `std::abort()` at `include/speedgun-ng/dbc.hpp:213`.

### Clarifications (phase 13)

#### Round 2, 2026-11-07

The acceptance lens re-read the phase against the tooling and the recorded
lane practice, and the phase then states the measurement it binds.

- Q: Does SC-009's "no step prints an `(inconsistent)` line" bind both
  emissions of the class? (SC-009, US-6) → A: it binds the contract
  record this phase repairs. The capture's end-line emission stands in
  the record with its file, its line and its count. Measured at
  `615c0964`: seven end-line records, in `test/source/harness_callback_test.cpp:278`,
  `test/source/harness_fixture_test.cpp:226`, `:249`, `:291` twice, and
  `example/docs/fixtures.cpp:30` twice, and the extract roots keep
  neither `test/` nor `example/`.
- Q: What verdict does SC-011 give an entry stopped at its ceiling, and
  which ctest lines give the four counts? (SC-011) → A: the record
  gives the passed figure, the failed figure, and the `did not run`
  block with its `(Skipped)` and `(Disabled)` rows, and every
  `***Timeout` row stands named with the ceiling it crossed and the
  seconds that entry needs outside `ctest`. Measured: `99% tests
  passed, 1 tests failed out of 185`, `harness_jitter_cli_test
  ***Timeout 300.11 sec`, `183 - harness_topdown_run_test (Skipped)`,
  `CTEST_EXIT=8`.
- Q: Which host's reading does SC-009 bind? (SC-009) → A: the thrawn
  reading the gate computes from the `branches` line of the summary
  read at the measured head, with the same reading given at the base.
  The CI reading stands as a diagnostic. Measured at `615c0964`:
  `branches....: 99.7% (9223 of 9249 branches)`, a shortfall of 26
  against the allowance of 21.
- Q: What keeps the check live, and does the FR-016 reproduction sit
  inside that rule? (local acceptance, FR-016) → A: every read of the
  route, the reproduction among them, runs with the host file out of
  lcov's reach, and the record names the form used. Measured: with the
  host rc the named complaint never appears; with it out of reach the
  read answers `ERROR: (corrupt) ... (inconsistent) ...` and the gate
  answers exit 3. The gate exits: 2 usage, 3 tool failure, 1
  shortfall, 0 clean.
- Q: What makes a build "fresh" and a capture "clean"? (FR-016, FR-019)
  → A: a `build/coverage` tree with no `*.gcda`, no `coverage.raw.info`
  and no `coverage.info`, configured with both compiler launchers
  empty, built, and run through the whole suite. The launcher rule
  stands because a ccache hit for an instrumented translation unit
  leaves a `.gcda` whose stamp disagrees with its `.gcno`: measured
  `jitter_cli_test.cpp.gcda:stamp mismatch with notes file`, then
  `lcov: ERROR: (gcov) GCOV failed`, `CAPTURE_EXIT=1`, no trace
  written. The two launchers and the counter removal stand as the
  host-local measures of `results/acceptance-thrawn.md` and
  `results/close-rows.md`; the repository command stands unchanged.
- Q: Which count does the cold-row ledger match? (US-7, SC-009) → A:
  the shortfall comes from the `branches` line of the summary read, and
  the ledger lists the cold rows of `build/coverage/coverage.info` with
  their own row count beside it. The two counts differ: the gate reads
  the `branches` figure at `tools/dbc/coverage_gate.sh:109`, computes
  the shortfall at `:117-119`, and prints `<file> line <N> branch <M>`
  rows taken from the `BRDA` records at `:130-136`, sorted and unique,
  only on the failure path. The record gives the taken count, the total count, the
  shortfall and the row count.

### Assumptions (phase 13)

- thrawn is the host of record and the machine this lane runs on: 48
  online cpus, all 48 in the affinity mask, kernel 7.3.0-5-cachyos,
  GCC and gcov 16.2.1, clang 23.1.3, lcov 2.6-0, cmake and ctest
  4.4.4 at the live check of 2026-11-07.
- The roadmap rulings G-20, G-25, G-29, G-31 and G-32 sit outside this
  repository; the description of 2026-11-07 carries their operative
  content, and this phase cites them from it.
- The `gh` credentials of this machine stand absent at the live check,
  so the pull request opens through the owner's credentials when the
  branch and its record are ready. The branch, the push proof and the
  body text stand delivered either way.

### Open, owner (phase 13)

- **O-3 (disputed marker class)**: FR-021 forbids a blanket marker
  purge in this phase. A marker class the phase disputes stands
  reported for investigation and stays open for the owner. The
  `group_io.cpp` retry and unstable-pair blocks carry nine markers
  whose ledger reason `marker-ledger.md` gives as T140, while FR-021
  states that a retry or unstable-pair marker gives no new exception.
  The phase reports the conflict and changes nothing.
- **O-4 (the capture list)**: the capture's `inconsistent` complaint is
  the end-line record of the function `SG_BENCHMARK_F` generates, read
  seven times at the base in `test/` and `example/`. Three routes
  stand open for the owner: keep the class for that shape, narrow the
  capture scope so `test/` and `example/` leave the raw trace, or
  change the public macro family. The phase takes none of them, keeps
  the capture list as it stands, and removes the class from the three
  reads of the written trace. The capture's end-line emission stands
  outside the data-consistency check: `lcovutil.pm:7624` raises it while
  `define_function` runs, and the check lives at `:10716`, so the capture
  ends without the class even on a host whose lcov configuration relaxes
  that check.
- O-1 and O-2 of this spec stand unchanged by this phase.

### Phase 13 follow-up (owner ruling G-26, 2026-11-08)

**Input**: Owner order of 2026-11-08, on thrawn: remove the remaining
capture suppression, fix the coverage-data fault behind it, bring the
uncovered-branch shortfall to at most the recorded allowance of 21 rows,
resolve the thread-sanitizer failure and the per-lane entry-count
differences, and put up the pull request. The order lifts the two
freezes that kept O-4 open: the capture class must go, and the repair of
the fixture macro shape is in scope.

This follow-up closes O-4 and replaces no earlier requirement, ruling or
record of this file. It adds FR-023 to FR-026, SC-015 to SC-018, D-14 and
D-15, and the task rows COV-008 to COV-011 in `tasks.md`.

- **FR-023**: No repository coverage command carries the `inconsistent`
  class, the capture list included. The `mismatch` and `negative` classes
  of the capture stand unchanged.
- **FR-024**: The branch shortfall of the measured head stands at or below
  the recorded allowance of 21 rows, with no new marker, no widened
  allowance and no invented exception.
- **FR-025**: `harness_context_barrier_test` answers the same way in every
  run of the thread-sanitizer tree. A fix that hides the assertion, adds a
  sleep or widens a timeout fails this requirement.
- **FR-026**: The record states the entry count of every lane, the entries
  one lane does not register, and the build condition that removes each
  one.
- **SC-015**: `grep -rn 'ignore-errors' cmake tools` names the `mismatch`
  and `negative` classes of the capture and nothing else, and a whole-tree
  capture at the measured head prints no `(inconsistent)` line.
- **SC-016**: The gate exits 0 on the extracted trace of the measured head.
- **SC-017**: Eight runs of the entry in the thread-sanitizer tree answer
  the same way, and the same entry keeps passing in every other lane.
- **SC-018**: The record carries one count line per lane, with the missing
  entries named and their condition cited.
- **D-14 (named entry functions)**: `SG_BENCHMARK_FIXTURE_REGISTER` hands
  `registerFixtureBenchmark` the two named functions
  `::sg::detail::fixtureCaseEntry` and `::sg::detail::fixtureFactoryEntry`
  in place of the two lambdas it wrote at the macro line. gcov attributes
  every line of a macro expansion to the macro invocation line, so the
  generated method and both lambdas shared one start line with two
  different end lines, and lcov read that as a contradiction. The call
  syntax at a macro site does not change, the two functions stand in
  `sg::detail`, and each carries no contract of its own, the shape D-13
  set.
- **D-15 (the two `hwlat` gates)**: `namesVendorText` and `rowNamesFigure`
  move from the anonymous namespace of `source/harness/latency_counters.cpp`
  into `source/harness/detail/latency_counters.hpp`, with their expressions
  unchanged, so a scenario can ask each of them a refused answer. The call
  sites keep the same postcondition. This is the same shape D-13 set, and
  it closes four rows of the FR-022 ledger by measurement.
- **O-4 closed**: the capture's end-line shape stands repaired at its
  source by D-14, so the capture list carries no class and the three routes
  O-4 left open stand unneeded.

The per-lane entry counts stand recorded under FR-026. They differ by
build condition, not by accident: the five `dbc_matrix_*` entries register
only where the preset sets `SPEEDGUN_DBC_MATRIX` (`test/CMakeLists.txt:158`,
030 FR-019, D-5, P-01), and four entries that replace the global `operator
new` and `operator delete` in their own translation unit stand unregistered
in the thread-sanitizer tree, because that runtime defines the same
operators and the two definitions cannot link
(`test/CMakeLists.txt:330` and the comments beside the other three).


## Phase 14: Prove the barrier wait entry and repair the FWQ sample (owner order, 2026-11-08)

**Input**: Owner order of 2026-11-08, on thrawn. Two rows stand open in
this lane. `test/source/harness_context_barrier_test.cpp` proves a flat
voluntary-switch count across a SPIN or YIELD round, and no reading in
that fixture shows the participant reached its wait arm before the
observer became the round's last arrival; a scheduled order can satisfy
the assertion with no wait at all. `report_shape_fwq_ftq_sample` fails
inside a full suite run and passes standalone, and no record names its
cause. Short name: barrier-entry-fwq.

This phase rides spec 045 as the owner directed. It adds no numbered
spec, no option, no feature and no version reservation, and it replaces
no earlier requirement, task row, ruling or record of this file. It adds
FR-027 to FR-033, SC-019 to SC-028, D-16 to D-22, and the task rows
BFW-001 to BFW-008 in `tasks.md`. In this phase "the record" names
`specs/045-merge-fixes/results/barrier-fwq-closeout.md`; the pull-request
body names the measured head. The barrier contract of
`specs/020-execution-contexts-core` and the probe contract of
`specs/043-fwq-ftq` stand unchanged.

### Run-time binding (phase 14)

- Base: `origin/master` at `2d4d8c57975a86db6c77536f7090aba2bdaf6417`
  (version 0.34.0, report schema 1.16, harness SOVERSION 22, archive
  SOVERSION 8, `git grep -c LCOV_EXCL -- source include` at 84, README
  1002 lines against the cap of 1004). No open pull request stood at the
  live check of 2026-11-08. The base SHA shall stand an ancestor of
  `origin/master` at cut time, and the implement phase rechecks it.
- Branch: `fix/barrier-entry-fwq`, cut from the latest `origin/master`.
  The branch stood absent on the remote at the live check. When it
  exists, the work inspects its head, reports, and stops for scope
  reconciliation; it overwrites nothing and revives no merged branch.
- One branch, one squashed commit, one pull request. The work never
  merges and closes no issue.
- Version, schema, harness SOVERSION and archive SOVERSION shall not
  change. No public header changes. The wait-entry proof reaches no
  public signature, no public layout and no exported symbol. When a
  proof needs a public change, the work stops and reports.
- Target host: thrawn, AMD Zen 5 Threadripper 9960X, 24 cores, 48
  threads. Acceptance is local (G-20). Continuous-integration results
  stand as diagnostics.
- Line numbers in the earlier phases of this file name the state at
  their own base. This phase finds every item by its symbol.

### Scope (phase 14)

In scope:

- `test/source/harness_context_barrier_test.cpp`: the fixture, its
  scenarios, its regression cases and its substitution evidence.
- `source/harness/execution_context.cpp` and, when the proof needs one,
  `source/harness/detail/internal.hpp`: the wait-arm observation seam,
  the shape D-13 set and the `installTest*` family already carries.
- `example/docs/fwq_ftq.cpp`, `test/source/harness_fwq_ftq_test.cpp` and
  `test/source/harness_jitter_window_test.cpp`: the work the FWQ sample
  runs and the unit proofs of the cause.
- `source/harness/jitter_probe.cpp` and
  `source/harness/detail/jitter_probe.hpp`: narrow edits only where a
  proved FWQ cause stands there.
- `tools/docs/report_shape.py`, its fixture tests, and the FWQ/FTQ block
  of `docs/pages/harness.md`: only for a proved parser, shape or work
  cause, and the example/page pair stays in sync.
- `specs/045-merge-fixes/`, its `results/` records and its quickstart.

Out of scope:

- `test/CMakeLists.txt`, the coverage and DBC gate scripts, the CMake
  presets, the continuous-integration configuration, `README.md`, the
  shared schema and version files, and every public header. A cause that
  reaches one of these stops the work and reports the needed scope.
- `specs/041-counter-probe/`, `specs/046-hunter-topdown/` and
  `specs/048-hwlat-probe` records;
  `test/source/harness_hunt_cli_test.cpp`,
  `test/source/harness_jitter_cli_test.cpp`, `test/example_run.sh`, its
  fixtures, and the base-CLI matrix driver, data and fixtures.
- Trace-line parsing (G-28, approved and standing), raw event codes
  (G-31) and automatic ranks (G-32). This phase implements none of them.
- Any timeout ceiling, retry, allowance, marker, filter or gate setting.
  The 110 s grant in `tools/docs/report_shape.py` and the 120 s ceiling
  of `report_shape_fwq_ftq_sample` stand as they are.

### User scenarios (phase 14)

#### US-8 A maintainer reads a wait-arm entry, and a last arrival (Priority: P1)

A maintainer reads the barrier fixture and finds, for every SPIN, YIELD
and BLOCK round, a record that the participant entered the wait arm its
own policy names, taken after the participant's arrival made it a
non-last arrival and before the observer arrived. The arrival that
fills the round publishes no record.

**Why this priority**: the flat voluntary-switch count is the evidence
that a wait mode costs no sleep. A flat count over a round that never
waited proves nothing about the mode, so the mode claim of specs/020
D-03 and Q-3 stands unproven until the entry stands shown.

**Independent test**: the `spinWait`, `yieldWait` and `insideOutside`
scenarios of `harness_context_barrier_test` each publish one entry
record, and each of the four substitutions FR-029 names fails its
scenario in all 240 runs of the stress matrix.

**Acceptance scenarios**:

1. **Given** a SPIN participant and an observer that has not yet
   arrived, **When** the participant's arrival stands counted and its
   wait arm stands entered, **Then** the observer sees the entry record
   before it arrives, and the round's release follows that arrival.
2. **Given** the same fixture, **When** a reader removes the wait arm a
   mode names, **Then** that mode's scenario fails on every run, and the
   failure names the missing record.
3. **Given** a round whose last arrival fills it, **When** the reader
   counts the entry records of that round, **Then** the filling arrival
   stands with no record, and the scenario that holds this case stays
   separate from the three wait-mode scenarios.

#### US-9 A user runs the docs FWQ example through the shape gate (Priority: P1)

A user runs `report_shape_fwq_ftq_sample` inside a full suite run, in a
two-cpu pinned run and in a four-cpu run, and the entry answers the same
way it answers standalone: the example exits 0 and the recorded block
matches the run by shape. No repeat and no larger wall allowance stands
needed.

**Why this priority**: the page-drift gate is a hard gate (VIII), and a
page that quotes an example the suite cannot run carries no evidence.

**Independent test**: the release tree runs
`ctest --test-dir build -R report_shape_fwq_ftq_sample`, and the same
entry runs inside each acceptance lane of the local-acceptance section
below.

**Acceptance scenarios**:

1. **Given** the measured head, **When** the entry runs inside a full
   parallel suite, inside the two-cpu pinned suite and inside the
   four-cpu suite, **Then** it exits 0 in each lane, and the record
   gives its wall time per lane.
2. **Given** the cause the record names, **When** a reader asks for the
   time budget, **Then** the record gives the calibration figures, the
   probe cpu list, the pre-check window, the child exit, the timeout
   source and the concurrency context of the run that failed.
3. **Given** the fix, **When** the reader compares the example region
   with the page block, **Then** `tools/docs/example_drift.py` exits 0,
   and the method word, the calibration group and the series lines keep
   their shape.

#### US-10 A reviewer reads acceptance at the shipped commit (Priority: P2)

A reviewer reads one record and finds every required lane measured at
the SHA the pull request ships, with its counts, its cold rows and its
push proof. No row stands called final at an earlier SHA.

**Why this priority**: G-23 makes the squashed head the measured head,
and a record that names an earlier head carries no acceptance.

**Independent test**: the record names one SHA, `git rev-parse HEAD`
equals it, the tracked tree stands clean, and
`git ls-remote origin refs/heads/fix/barrier-entry-fwq` equals it.

**Acceptance scenarios**:

1. **Given** the squashed head, **When** each required lane command
   runs, **Then** the record names that SHA beside the command, the
   exit and the counts.
2. **Given** any later change to a tracked file, **When** the work
   commits it, **Then** every required lane runs again at the new head,
   and the record names the new SHA.

### Edge cases (phase 14)

- The observer's own arrival is the round's last arrival in every
  measured scenario. The arrival that fills a round waits on nothing,
  so it enters no arm and publishes no entry record. A proof that reads
  one arm record for every arrival is wrong.
- A participant that stands preempted between its arrival and its wait
  arm can reach the arm behind the release. The gate holds the
  observer's arrival until the record or the participant's departure
  stands published, so the release cannot precede the entry.
- A build that removes an arm answers twice in the same run: the gate
  opens on the departure and the scenario names the missing record, and
  the barrier's own postcondition at
  `source/harness/execution_context.cpp:897-899` ends the child by
  `SIGABRT`, because the contract semantic stands `enforce` in every
  required tree.
- A hook that runs before the call, a read of the participant's `entered`
  flag, and a hook that runs before the mode chain decides an arm each
  fail this requirement: none of the three shows the arm the participant
  entered.
- A wrong-arm substitution that fails on some runs and passes on others
  is inadequate. The regression cases gate the observer's arrival on the
  entry record, so the substituted arm answers the same way every run.
- The legitimate no-wait case stands separate: a delayed participant
  that arrives last fills the round and waits on nothing. It keeps its
  own scenario and its own expectation.
- No scenario adds a sleep, a wall-time threshold, a time-source read or
  a fake cpu. The gap that holds a round open stays a fixed count of
  steps checked against its own sum.
- The thread-sanitizer tree slows every instrumented access. The entry
  record rides an atomic, so the sanitizer sees one release and one
  acquire, and the stress matrix of the local-acceptance section reports
  the entry's runs in that tree.
- The FWQ sample carries two grants: the 110 s the script gives the
  child and the 120 s `ctest` gives the entry. A record that names one
  of them names which. A child killed by the script grant exits through
  the script's failure line, and an entry killed by the `ctest` ceiling
  prints `***Timeout`; the two readings differ.
- A standalone pass establishes no root cause. The diagnosis reads the
  suite form, the pinned form and the instrumented form, and keeps the
  child's stderr, the cpu set and the calibration figures of each.
- The example region stands quoted in `README.md` and its recorded run
  stands in `docs/pages/harness.md`. An example edit that changes a
  quoted line needs the `README.md` block in the same change, and
  `README.md` stands out of this phase's scope. Such an edit stops the
  work and reports the scope it needs; O-7 carries the one such edit the
  diagnosis found.

### Requirements (phase 14)

- **FR-027** (BFW-001): When the phase starts, the work shall reproduce
  the barrier fixture's behavior on the base and separate four things
  the fixture now holds together: the test handshake, the entry flag,
  the status-file read, and the barrier's real behavior. The
  record shall state the happens-before edges a wait-mode claim needs,
  in terms of the release and acquire operations that carry them, and
  shall show which of those edges the fixture at the base holds and
  which it lacks. No ordering claim shall rest on a fixed loop, a wall
  duration, a log timing or a count of successful repeats.
- **FR-028** (BFW-002): The wait-entry proof shall observe the wait arm
  the participant enters, at a point inside that arm and after
  the barrier state requires a wait. The call site shall stand once per
  waiting arrival, at the head of the arm, after the branch that answers
  a filling arrival, and no call shall stand inside a spin loop. The
  observation shall reach the observer through a release store and an
  acquire load. The seam shall
  stand internal: no public header, no public signature, no public
  layout and no exported symbol shall change, and the installed
  observer shall stand clearable. The seam shall follow the shape D-13
  and the `installTest*` family carry: declared in an internal header,
  defined beside the barrier, installed by a test and cleared with an
  empty optional. The phase shall keep the SPIN, YIELD and BLOCK arms,
  their release and acquire edges, and the last-arrival release
  semantics as specs/020 states them.
- **FR-029** (BFW-003): The fixture shall carry the entry proof in four
  named scenarios: `spinWait`, `yieldWait`, `insideOutside` and `round`.
  The first three shall gate the observer's arrival on the entry record
  of the arm their own policy names. `round` shall carry the case of the
  arrival that fills the round: it waits on nothing and publishes no
  record. The entry gate holds the round open across the fixed step gap,
  so a delayed observer needs no scenario of its own. Four substitutions
  stand required, each a build of the phase head with one arm changed: a
  SPIN participant given the BLOCK wait, a YIELD participant given the
  BLOCK wait, a participant that runs on no context given a spinning arm,
  and one arm removed. Each substitution shall fail its scenario in every
  run of the 240-run matrix in the release tree, and the record shall
  carry the patch, the command and the failure line. An arm-body swap
  stands outside the set, and O-6 carries it with the tool that would
  close it. No case shall add a sleep, a time-source read, a CPU-time
  threshold or a fake cpu.
- **FR-030** (BFW-004): The work shall run the FWQ sample on the current
  code in three forms, the full parallel suite, the two-cpu pinned suite,
  and the `ci-coverage` tree, and shall record what each form shows: the
  child's stderr, the child's exit, which grant ended the run, the cpu
  set the run held, the calibration figures and work count, the pre-check
  window, and the resource and concurrency context. The record shall
  distinguish the 110 s grant of `tools/docs/report_shape.py` from the
  120 s ceiling of `ctest`. A form that passes stands recorded as a
  comparison, and a pass answers no cause. The symptom of record stands
  the corsairone `ci-coverage` reading: 110.21 s with the child timed out
  by the script grant.
- **FR-031** (BFW-005): The fix shall address the cause FR-030 proves,
  with bounded and meaningful work in the test or the example, or with a
  production correction where the proof names a production cause. The
  fix shall keep the FWQ and FTQ calibration, the console and schema
  shape, and the method disclosure intact. A unit proof shall drive the
  cause through scripted time and legitimate internal inputs, and a real
  example run shall stand identified as a separate check. The fix shall
  not remove the tested method, skip the comparison, ignore a failure,
  add a retry, or raise either timeout. The fix shall carry a regression
  case, and an example change shall keep the example and the page block
  in sync.
- **FR-032** (BFW-006): Every required local acceptance command shall
  run at the final squashed head, and the pull-request body, or one
  untracked artifact beside it, shall name that head beside each
  reading. The tracked record carries dated rows labeled preliminary
  with the head each was taken at. The lanes, the stress matrix and the coverage
  reading stand in the local-acceptance section below. A lane that a
  host refuses stands recorded as blocked with the command and the
  refusal; an equivalent mask is no substitute. A base failure stands
  identified and open.
- **FR-033** (BFW-007): The phase shall open one pull request whose body
  names the measured SHA, the evidence, the cold rows and the push
  proof. After every push, `git ls-remote origin
  refs/heads/fix/barrier-entry-fwq` shall equal `git rev-parse HEAD`.
  The work shall not merge the pull request and shall close no issue.

### Success criteria (phase 14)

- **SC-019**: Each of `spinWait`, `yieldWait` and `insideOutside`
  publishes one entry record naming the wait arm its own policy names,
  and the arrival that fills the round publishes none. Each record
  stands published before the observer's arrival in every run of the
  stress matrix.
- **SC-020**: Each of the four substitutions FR-029 names, applied to a
  build of the phase head, fails its scenario in all 240 runs of the
  matrix in the release tree, and the record carries the patch, the
  command and the failure line. The unmodified head passes the same 240
  runs with zero failures in the release tree and in the
  thread-sanitizer tree.
- **SC-021**: `report_shape_fwq_ftq_sample` exits 0 in the full parallel
  suite, in the two-cpu pinned suite and in the four-cpu suite at the
  measured head, and the record gives its wall time in each. The entry
  also exits 0 in the sanitizer, thread-sanitizer and instrumented lanes
  on thrawn, where the base already passes inside the grant. The
  example's calibration group, series lines and method word keep their
  shape, and `tools/docs/example_drift.py` exits 0.
- **SC-022**: The record names the FWQ cause with the measurements that
  separate it from the three alternatives: the 110 s grant of
  `tools/docs/report_shape.py`, which answers `the run failed` and exit 1
  from the script; the 120 s `ctest` ceiling of `test/CMakeLists.txt`,
  which answers `***Timeout`; and a shape finding, the recorded reading
  `sample output block holds 156 lines, the run printed 125`. Or it names
  the row unresolved with the next precise experiment and its expected
  reading. An unreproduced failure stands unresolved, and no claim of a
  fix stands made for it.
- **SC-023**: The coverage reading at the measured head gives its
  numerator, denominator, branch shortfall and marker count, and the
  shortfall stands at or below 21 with no new marker, filter, allowance
  or ignore-error class. Every cold row stands listed with its file,
  line, arms and evidence.
- **SC-024**: The record gives one count line per lane with its passed,
  failed, did-not-run and timeout figures, the cheap-gate exits, the
  host and tool versions, the commands and the exit codes. The stress
  matrix rows stand listed with their failures, and the deterministic
  proof stands separate from the stress readings.
- **SC-025**: The record separates the four things FR-027 names, each by
  its symbol, and states which happens-before edges the fixture at the
  base holds and which it lacks. The statement names no fixed loop, wall
  duration, log timing or repeat count as support.
- **SC-026**: The pull-request body names the measured head, the
  evidence, the cold rows and the push proof, and after each push
  `git ls-remote origin refs/heads/fix/barrier-entry-fwq` equals
  `git rev-parse HEAD`. No tracked file carries the SHA of its own
  commit.
- **SC-027**: The seam stands internal and clearable: the diff names no
  file under `include/`, an empty optional clears the slot, and a
  `BUILD_SHARED_LIBS=ON` install read of `.dynsym` shows no new
  `sg::detail` name against the standing export count of 250. The call
  stands once per waiting arrival at the head of an arm, and no call
  stands inside a spin loop.
- **SC-028**: The FWQ fix carries a unit case that drives a scripted
  window, folds one probe cpu twice, and reads the stored transform
  answer between the two folds. The case fails a build that computes the
  transform twice, and it passes at the phase head in the release tree
  and in the `ci-coverage` tree.

### Local acceptance (phase 14)

Acceptance is local (G-20) on thrawn. Before any pinned run the work
records `sched_getaffinity` and the online cpu list. A test chooses a
real allowed cpu. The required lane is `taskset -c 0,1`; when cpus 0 and
1 stand outside the allowed set, that lane stands recorded as blocked
with the refusal, and a run on another allowed pair stands as extra
evidence. A run on four real allowed cpus also stands required.

Required lanes at the measured head:

1. A fresh full release suite (`ci-rocky`), configured, built and run
   whole.
2. The same suite under `taskset -c 0,1`.
3. The same suite on four real allowed cpus.
4. The `ci-sanitize` tree.
5. The `ci-tsan` tree.
6. The `ci-coverage` tree through the `coverage` target and the gate.
7. The cheap gates: format, spell, the prose gate over the range and
   over the tree, example drift, example run, time-source, the DBC
   documentation and pairing gates, the analyzer build, and the
   downstream consumer build.

The stress matrix stands predeclared here. The barrier entry runs 240
times with 8 in flight in the release tree and in the
thread-sanitizer tree, the protocol phase 13 ran:

```
seq 1 240 | xargs -P 8 -I{} build/test/harness_context_barrier_test
seq 1 240 | xargs -P 8 -I{} build/tsan/test/harness_context_barrier_test
```

The direct-binary form with 8 in flight is the shape the phase 13
counts stand taken at: 62 failures in 480 runs of the branch's own
binary, 0 in 240 and 1 in 960 of the fixed binary, and 12 of 20 rounds
taking a wait arm before the fix.

The FWQ entry runs 20 times in the release tree, one `ctest`
invocation per run:

```
seq 1 20 | xargs -I{} ctest --test-dir build -R '^report_shape_fwq_ftq_sample$'
```

The record lists every failure of the matrix. The scenario readings of
SC-019 stand reported apart from these counts, and the four substitution
counts of SC-020 stand listed with them.

Coverage stands measured under the rules of phase 13: 100 percent lines
and branches subject only to the accepted R-1, R-2 and W-1 exceptions, a
tree-wide branch shortfall at or below 21, and at or below 84 markers.
The two approved closed-enum `BR` markers on `switch (slot.source)` in
`source/counters/linux_pmu/group_io.cpp` stay narrow. No new
`LCOV_EXCL` marker, ignore-error class, omit or remove pattern, widened
filter, allowance or host lcovrc bypass stands added, and G-25's removal
stays gone.

No test of this phase uses a sleep, a wall-clock threshold or a fake
cpu. The phase uses the deterministic seams the repository already
carries, plus the one seam FR-028 allows.

### Commit and record rules (phase 14)

- History, authors and footers of earlier work stand as they are
  (G-26, G-27). New commits carry author and committer `OmO`, and the
  prose follows ASD-STE100. No new person name and no user home path
  stands in a tracked record; the LICENSE exemption stands. Tracked
  paths name `$HOME` or a build-relative root.
- One squashed commit carries the phase. Title: `<Section>: <one-line
  imperative>` at 50 characters or fewer. Body lines: 72 columns or
  fewer. Footer, as real trailers in order:
  `Refs: specs/045-merge-fixes BFW-001 to BFW-007` then
  `Approved-by: OmO`.
  The two lanes stand in that one commit: both answer the two open rows
  of one spec, no file stands in both, and reverting that one commit
  returns both rows together. The order follows the landed history of this
  repository, as phase 13 recorded. The commit carries no `Fixes #n`:
  no tracking issue stands for either row, and the body says so.
- During the work, each change pairs with its test, and the record
  `results/barrier-fwq-closeout.md` carries reproducible commands and
  dated interim findings, each labeled preliminary with its SHA.
- The work fetches `origin`, rebases onto the latest `origin/master`,
  and never merges. It finishes the tracked code, tests, docs and
  records, squashes the branch into one commit, requires a clean tracked
  tree, and saves `git rev-parse HEAD` as the measured head. Every
  required lane then runs at that head, and its readings stand in the
  pull-request body or an external untracked artifact. No tracked file
  carries its own SHA. A change to a tracked file after a measurement
  produces a new squashed head and a fresh run of every required lane.
- First push: `git push -u origin fix/barrier-entry-fwq`. Each later
  push saves the expected full remote SHA, requires it to equal the last
  observed value, and uses
  `git push --force-with-lease=fix/barrier-entry-fwq:<expected-full-sha>`.
  A failed lease stops the work and stands reported; the lease stands
  never refreshed and retried. After each push `git ls-remote origin
  refs/heads/fix/barrier-entry-fwq` shall equal `git rev-parse HEAD`.
- `.specify/feature.json` stays a local file and stands out of the pull
  request.
- No new continuous-integration change, gate or exclusion approval stands
  in this phase.

### Decisions (phase 14)

- **D-16 (reuse)**: the phase rides spec 045 as the owner directed. Both
  rows stand in the lane this spec already carries, and a new numbered
  spec would split the record of that lane.
- **D-17 (the arm entered)**: the proof reads the wait arm entered.
  An arrival count, a flag set before the call, and a hook before the
  mode chain each stand short of the claim, so the seam stands inside
  the arm the participant takes.
- **D-18 (seam shape)**: the seam follows D-13 and the `installTest*`
  family: an internal declaration, a definition beside the barrier, an
  installed observer, and an empty optional that clears it. The
  production path keeps its arms, its ordering and its cost shape, and
  the observer stands reachable only from a test translation unit.
- **D-19 (cause before fix)**: FR-027 and FR-030 precede FR-028, FR-029
  and FR-031 in the same branch, so the record shows the gap and the
  failure first, then the proof and the fix, then the clean reading.
- **D-20 (bounded work)**: the FWQ fix bounds the work the unit proofs
  carry, or the production path the proof names. A timeout ceiling, a
  retry and a widened allowance stand outside the fix, because they hide
  the row and answer no cause. An example edit that changes a quoted line
  needs the `README.md` block in the same change, and `README.md` stands
  out of this phase's scope, so such an edit stops the work and reports
  the scope it needs.
- **D-21 (unresolved is a result)**: when the FWQ failure does not
  reproduce, the row stands unresolved with the next precise experiment.
  A pass in one lane stands recorded as a pass in that lane, and no
  claim of a fixed failure stands made from it.
- **D-22 (a caught violation is a finding)**: when a contract check fires
  on the production path during this phase's own runs, the branch fixes
  the path and records what was seen, the runs that show it and the runs
  that do not. A test that fails in some loaded suite runs stands outside
  acceptance, and a skip, a retry and a widened allowance stand outside
  the fix.

### Clarifications (phase 14)

#### Round 1, 2026-11-08

The clarification pass read the phase against the fixture, the barrier,
the gate configuration and the recorded lane readings at `2d4d8c57`.

- Q: Which artifact names the measured head, when a tracked file carries
  no SHA of its own commit? (FR-032, SC-024, SC-026, US-10) → A: the
  pull-request body, or one untracked artifact under `$HOME/scratch/bfw/`,
  carries the measured head with its readings. The tracked record carries
  the commands and the dated interim findings, each labeled preliminary
  with its SHA. The records-only amend route phase 13 took stands closed
  to this phase.
- Q: In which lanes does the stress matrix run, and which command gives
  240 runs with 8 in flight? (local acceptance) → A: the release tree and
  the thread-sanitizer tree, the direct-binary form phase 13 ran:
  `seq 1 240 | xargs -P 8 -I{} build/test/harness_context_barrier_test`
  and the same over `build/tsan`. `ctest --repeat` knows only
  `until-fail`, `until-pass` and `after-timeout`, so no `ctest` form of
  this count exists.
- Q: How many substitutions does SC-020 name, and does an arm-body swap
  stand inside the set? (FR-029, SC-020) → A: four arm-level rows: one
  row per mode that hands that mode another mode's arm, and one row that
  removes an arm. An arm-body swap stands outside the set, and O-6
  carries it with the tool that would close it.
- Q: Do the delayed participant and the delayed observer each need their
  own scenario? (FR-029, US-8) → A: superseded by round 2, which settled
  four named scenarios. The delayed participant stands carried by the
  filling arrival of `round`, and the entry gate holds the round open
  across the fixed step gap, so a delayed observer needs no scenario of
  its own. The three wait-mode cases already hold the
  delayed-observer shape, because the observer gates its own arrival on
  the entry record. `scenarioRound` is the home of the last-arrival case.
- Q: Must the FWQ failure reproduce in all three forms? (FR-030) → A: no.
  Each form runs and each stands recorded. Measured at the base on
  thrawn: 19.18 s standalone in the release tree, 17.80 s standalone
  under `taskset -c 0,1`, 24.29 s to 25.52 s inside a full `-j 24`
  release suite, and 50.31 s standalone in the instrumented tree against
  58.59 s inside that lane. SC-022 and D-21 keep an unreproduced row
  unresolved.
- Q: Does the phase owe a pass for the FWQ entry in the sanitizer,
  thread-sanitizer and instrumented lanes? (SC-021) → A: yes on thrawn,
  where the base passes inside the grant at 24.58 s, 49.07 s and 58.59 s.
  The 110.21 s reading on corsairone stands as that host's limit and
  answers nothing here.
- Q: Who checks that no exported symbol changes, when no required lane
  runs a symbol audit? (FR-028) → A: hidden visibility
  (`CMakeLists.txt:130`) and the local audit phase 13 reproduced: the new
  `sg::detail` names stand absent from `.dynsym`, and the export count
  stands at 250. No new lane runs.
- Q: Where does the seam call stand inside an arm shaped as a loop, and
  does one extra load per arrival break the cost shape? (FR-028, D-18) →
  A: one call per waiting arrival at the head of the arm, after the
  branch that answers a filling arrival
  (`source/harness/execution_context.cpp:859-896`). A call per spin
  iteration would be a regression: `:873` fixes the cost model, and
  `specs/020-execution-contexts-core/plan.md:97` names the barrier a P0
  path.
- Q: Does the installed-or-clear branch cost a cold row against a ledger
  at 20 of an allowance of 21? (SC-023) → A: it counts, and the fixture
  takes both arms, install and clear, so the net new cold rows read
  zero. The headroom stands at one row.
- Q: Does the filling arrival publish a record that names no arm?
  (SC-019) → A: it publishes no record. The seam call stands after the
  branch that answers a filling arrival, and the separate scenario
  asserts the absence, with exactly one record per waiting participant.
- Q: Does FR-027 ask for a fixture split, and does it bar the fixed step
  gap? (FR-027) → A: it asks for a written separation. The record names
  each of the four things by its symbol: `observer.ready` with
  `watch(observer.go)`, `observer.entered`, `readStatus` with
  `stateCharOf`, and `barrier.arriveAndWait`. The fixed step gap stays,
  and the ordering claim rests on the entry gate.
- Q: Which alternatives does SC-022 name? (SC-022, FR-030) → A: three.
  The 110 s grant of `tools/docs/report_shape.py:368`, the 120 s `ctest`
  ceiling of `test/CMakeLists.txt:2197`, and a shape finding such as a
  block count that disagrees with the printed lines
  (`results/acceptance-corsairone-cov.md:133-135`); that third class
  reaches a page whose run prints one noise warning per noisy planned
  cpu (`source/harness/report.cpp:1736-1752`) while the fold's host
  condition list carries no mask for the line.

#### Round 2, 2026-11-08

The second pass read the phase against the code at `2d4d8c57` and
against the round-1 answers, and the phase then rewrote the passages
that contradicted an answer.

- Q: Does the arrival that fills the round publish a record that names
  no arm? (SC-019, US-8) → A: it publishes no record. The seam call
  stands after the branch at `source/harness/execution_context.cpp:859`,
  so a reader counts the records of a round and finds none for that
  arrival.
- Q: Which artifact names the measured head? (FR-032, US-10, SC-026) →
  A: the pull-request body, or one untracked artifact beside it. The
  tracked record carries dated rows labeled preliminary with the head
  each was taken at.
- Q: Does FR-030 require a reproduction? (FR-030, SC-022) → A: it
  requires the three forms to run and each to stand recorded. Measured
  at the base on thrawn: the release suite answers 24.29 s, 25.14 s and
  25.52 s in three runs; the two-cpu pinned suite answers 17.54 s with
  190 of 190 passed; 20 pinned runs of the entry answer 13.36 s to
  18.50 s; the `ci-coverage` tree answers 50.31 s standalone and 55.92 s
  inside its lane. The failure stands unreproduced on this host, and the
  corsairone reading of 110.21 s, the child timed out by the script
  grant, stands as the symptom of record.
- Q: What does a missing record answer with, and can the gate wait
  forever? (edge cases) → A: the gate opens on the record or on the
  participant's departure, so no scenario waits on a record that cannot
  arrive. A removed arm also trips the postcondition at
  `source/harness/execution_context.cpp:897-899`, and the contract
  semantic stands `enforce` in every required tree
  (`cmake/dbc.cmake:24`).
- Q: How many scenarios carry the proof, and what are they named?
  (FR-029) → A: four, named as the fixture names them: `spinWait`,
  `yieldWait`, `insideOutside` and `round`. The entry gate holds the
  round open across the fixed step gap, so a delayed observer needs no
  scenario of its own.
- Q: How many substitutions, in which trees, at what count? (FR-029,
  SC-020) → A: four rows, 240 runs each in the release tree. The
  unmodified head owes 240 clean runs in the release tree and in the
  thread-sanitizer tree. The arm-body swap stands outside the set, and
  O-6 carries it.
- Q: Who reads the exported-symbol clause of FR-028? (FR-028, SC-027) →
  A: one reading, not a lane: a `BUILD_SHARED_LIBS=ON` install under
  `$HOME/scratch`, then `nm -D --defined-only` over the installed
  object, expecting no new `sg::detail` name against the standing 250
  exports.
- Q: Which alternatives does SC-022 separate? (SC-022) → A: the script
  grant, the `ctest` ceiling, and a shape finding, each named with the
  line it prints.

The pass also settled three terms. The fixture's `entered` flag stands
called the entry flag, the act of changing one arm in a build stands
called a substitution, and `ci-coverage` names the instrumented tree.

### Assumptions (phase 14)

- thrawn is the host of record: 48 online cpus, all 48 in the affinity
  mask, kernel 7.3.0-5-cachyos, GCC and gcov 16.2.1, clang 23.1.3, lcov
  2.6-0, cmake and ctest 4.4.4 at the live check of 2026-11-08.
- The owner rulings G-20, G-23, G-25, G-26, G-27, G-28, G-29, G-31 and
  G-32 sit outside this repository; the order of 2026-11-08 carries
  their operative content, and this phase cites them from it.
- The `gh` credential of this machine stands absent at the live check, so
  the pull request opens through the owner's credential when the branch
  and its record stand ready. The branch, the push proof and the body
  text stand delivered either way.

### Open, owner (phase 14)

- **O-5 (the two grants)**: the 110 s grant inside
  `tools/docs/report_shape.py` and the 120 s `ctest` ceiling of
  `report_shape_fwq_ftq_sample` stand unchanged by this phase. A change
  to either stands outside the scope FR-031 allows, and the owner owns
  that decision.
- **O-6 (the arm-body swap gate)**: the fixture's entry record names the
  arm the code path enters, so it answers a substituted arm and a removed
  arm. It cannot separate two arms whose bodies stand swapped: the record
  still names the arm, and both spinning bodies keep the voluntary-switch
  count flat. The only deterministic tool for that class is a disassembly
  gate over `ContextBarrier::arriveAndWait` with a planted-swap liveness
  probe, the shape `test/loop_shape.sh` and `test/barrier_shape.sh`
  carry. That tool needs a new script and a new entry in
  `test/CMakeLists.txt`, and this phase edits neither. The class stands
  open for the owner.
- **O-7 (the example's ring bound)**: the diagnosis found the sample's
  cost dominated by the transform over a ring the example sets to its
  default. Bounding that ring in `example/docs/fwq_ftq.cpp` answers the
  cost, and the region stands quoted byte for byte in `README.md`, so the
  edit needs one line there. `README.md` stands out of this phase's
  scope, so the bound stands reported here and the owner owns it.
- **O-8 (the probe's time base)**: the probe's per-cpu figures carry
  names in nanoseconds while the read that fills them answers raw
  counter ticks. On this host the two differ by a factor of 4.19. The
  row stands outside this phase, which changes no time base, and the
  owner owns it.
- O-1 to O-4 of this spec stand unchanged by this phase.
