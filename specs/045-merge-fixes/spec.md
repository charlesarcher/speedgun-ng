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

Out of scope: `.github/`, `tools/dbc/`, `tools/ci/`, `cmake/`, the
coverage gate configuration, any version number, any CI change.

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
- **SC-008**: `.github/`, `tools/dbc/`, `tools/ci/`, `cmake/`, the
  version, the schema and the SOVERSION carry no change.

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
carry `Refs: specs/045-merge-fixes`. No mixed commit arises: the `specs/045-merge-fixes/` files land only
in spec-file commits, and the Part A and Part B commits touch only their code
and tests, one topic per commit. Then `Approved-by: OmO`.
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

