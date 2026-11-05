# Part B research: the jitter probe items (FR-006 to FR-012)

Spec: `specs/045-merge-fixes/spec.md`. Base: master `b1dd7670`. The line
numbers below name the state at that sha; the implement phase finds each
item by its symbol when a line moves.

Method: this research reads the Part B sources and tests at the base,
traces the failure chain each requirement names, and states one decision
per requirement with the code facts and the rejected alternatives. The
constitution principles that bind this lane: I (the pinned Core
Guidelines baseline on every changed line), II (contracts on the new
interfaces), V (naming and format), VIII (the hard gates the lane
clears), IX (the artifacts of this directory), X (surgical scope, no
speculative machinery), XI (this text obeys the discourse rules).

## FR-006: the window pacing of the scripted clock

**Decision (D-4)**

1. The standing scripted clock of `harness_jitter_window_test` changes
   its step: the tail delta of the `machine/tsc` leaf goes from 1 to
   `kTargetUnitTimeNs` (1000). The step rides one named constant beside
   `kSeriesJump` (`test/source/harness_jitter_window_test.cpp:117`), and
   the `setPoints` calls that reset the standing clock spell that
   constant (`test/source/harness_jitter_window_test.cpp:93`, `1452`,
   `1492`, `1532`, `1781`, `1800`). The jump and restore hand-offs of
   the gate bodies keep their shape: the restore sets the paced step
   where it set 1 before, and the jump sets `kSeriesJump` (1,000,000)
   as before.
2. The roundtrip scenario (`theSeriesRoundTripsThroughLoadReport`,
   `test/source/harness_jitter_window_test.cpp:2300` area) drops the
   `DipSeriesReader` install and runs on the paced standing clock. Its
   assertions read the document shape and the fold presence, so no
   expected magnitude moves.
3. The dip-clock scenarios keep `DipSeriesReader`. Their assertions are
   run-level figures (the series values, the spectrum periods, the
   console lines), so the placement question stays out of their scope.
4. The probe changes nothing. The alongside loop of a series method
   keeps every unit until the buffer fills, so the probe drops no
   sample a real run would keep. The commit body records no probe
   change, and FR-006 names that rule for the case where one appears.

**Rationale**

- The runner stamps every timed window through the system path:
  `windowStampNow` (`source/harness/runner.cpp:55-78`) compiles a plan
  over the `machine/tsc` leaf of the catalog and reads one stamp per
  open and close (`runner.cpp:2643-2645`, `2792-2824`). No installed
  seam reader stands in that path, so the window stamps always live on
  the fake provider's shared slot.
- The fake provider hands each window its own playback cursor and
  publishes forward only (`source/counters/fake_provider.cpp:74-90`,
  `149-156`): a window reads `last + tailDelta`, and the hand-off moves
  the shared slot only when the window's last stands at or above it. A
  tail-only reprogram keeps the cursor's last value
  (`fake_provider.cpp:275-287`), so the jump and restore hand-offs
  shift the step and keep the scale.
- The FWQ fold recovers each sample's stamp as `seriesStartNs` plus the
  running sum of the unit times (`source/harness/jitter_probe.cpp:1186`
  to `1193`), and the running sum telescopes to the raw stamp of the
  read that kept the sample. The placement test is the half-open window
  test over that recovered stamp (`jitter_probe.cpp:1204`). So a sample
  lands inside a window when the probe made its read between the
  window's open read and its close read, on the shared slot's scale.
- The calibration decides the probe's read cadence. One trial measures
  the unit time as the delta of two reads around the body
  (`calibrationTrial`, `jitter_probe.cpp:228-249`); the doubling runs
  while the unit stands below the target (`jitter_probe.cpp:279`) and
  the cap ends it (`jitter_probe.cpp:287`). The target stands at
  `kTargetUnitTimeNs` = 1000 and the cap at `kMaxWorkCount` =
  16,777,216 (`include/speedgun-ng/benchmark.hpp:2443-2444`). Under the
  standing one-tick clock the unit time reads 1 at every count, so the
  doubling runs to the cap and the search keeps `workCount` =
  16,777,216. One FWQ unit then costs a body of 16,777,216 iterations,
  several milliseconds of wall time, and the probe reads once per
  unit. The paced step of 1000 meets the target at the first trial, so
  the search keeps `workCount` = 1 and one unit costs one read.
- The failing run shows the starvation: PR #73 run 41576309608 left the
  document with `"jitter": null`, and the 100,000-iteration run of the
  roundtrip scenario took about four microseconds. A window of that
  span catches no probe read when one read costs milliseconds, so every
  entry fold places no sample and the entry stands null. With
  `workCount` = 1 the probe reads many times per window span, the
  entries place their samples, and the document carries a real jitter
  record.
- The expected magnitudes stand unchanged for three reasons. First, the
  effective threshold rises to ten times the measured minimum loop
  period (`kRiseFactor`, `jitter_probe.cpp:57`, `505-511`), so the
  standing step of 1000 raises the threshold to 10,000 and the standing
  clock still keeps no gap; the jump of 1,000,000 stays above it. The
  gap verdicts of every gate scenario hold. Second, the pinned gap
  magnitudes ride the jump hand-offs (`maxNs == kSeriesJump` at window
  test lines `1470`, `1518-1522`), and those stay byte-identical.
  Third, the method scenarios pin key order, key presence, equal unit
  times and shape, and no assertion pins the standing step. The
  scenario comment that names the 2 ms unit time
  (`test/source/harness_jitter_window_test.cpp:2118-2120`) describes
  the broken cadence and changes with the fix, and XI binds the new
  wording.

**Alternatives**

- A longer benchmark body that stretches every window in wall time:
  rejected. The starvation is load-sensitive, the bound stays a
  host-dependent race, and X.4 asks for a check that passes by
  construction. The paced step places the samples by construction.
- A benchmark body that reads the clock per iteration: rejected. Each
  extra runner read advances the shared slot one tick, the probe's own
  cursor falls one tick further behind, and the forward-only hand-off
  never lets a lagging window catch up
  (`fake_provider.cpp:149-156`). The change widens the lag and starves
  the placement it means to feed.
- A seam reader that delegates to the provider window and adds the dip
  on top: rejected for the roundtrip need. The dip extra displaces the
  stamp scale the placement reads, and the standing paced step already
  gives the scenario its samples. The dip scenarios keep their reader,
  because their assertions never read a placement.
- A probe change that places samples by index or relaxes the window
  test: rejected. FR-006 changes the probe only when it drops a sample
  a real run would keep, and it drops none; the fold's stamp recovery
  is exact by the telescoping sum.

## FR-007: the signed sine-arm delta with the clamp at 0

**Decision**

The sine arm of `scriptedDelta`
(`test/source/harness_fwq_ftq_test.cpp:232-238`) computes the delta in
a signed 64-bit type and clamps the result at 0:

```cpp
if (scriptPlan.sinePeriod != 0) {
  const auto phase = kTwoPi * static_cast<double>(unit)
      / static_cast<double>(scriptPlan.sinePeriod);
  const std::int64_t delta = static_cast<std::int64_t>(scriptPlan.windowStep)
      + static_cast<std::int64_t>(std::llround(
          static_cast<double>(scriptPlan.sineAmplitude) * std::sin(phase)));
  return delta < 0 ? 0 : static_cast<std::uint64_t>(delta);
}
```

The product rounds before the add, the add runs in `std::int64_t`, and
a negative result clamps to 0. The reader then adds the delta to the
thread's stamp, so the stamp series stays monotone.

**Rationale**

- The sanitize job read the conversion to `unsigned long` at
  `test/source/harness_fwq_ftq_test.cpp:237`: the cast wraps the
  product alone, and `static_cast<std::uint64_t>` of a negative double
  is undefined behaviour. The FWQ arm runs 100,000 units with an
  amplitude of 20 (`test/source/harness_fwq_ftq_test.cpp:940-941`), so
  the sine turns negative on a large fraction of the reads and UBSan
  flags the conversion on each one.
- The signed sum never goes negative in the two planted arms: the step
  is 1000 and the amplitude is 20, so the sum stays inside 980 to 1020.
  The clamp exists for the general shape of the plan: a small
  `windowStep` with a large amplitude names a negative delta, and a
  negative delta would run the stamp series backwards and wrap every
  unsigned delta the folds read. The clamp at 0 keeps the stamps
  monotone and the folds exact.
- The expected magnitudes stand unchanged. The two sine arms assert
  relations and tolerances: the FWQ arm reads the median and the
  resample step at 1000 and the period inside 990,000 to 1,010,000 ns
  (`test/source/harness_fwq_ftq_test.cpp:961-970`), and the FTQ arm
  reads the count relations and the same period band
  (`test/source/harness_fwq_ftq_test.cpp:1374-1390`). The rounding
  shift of one unit per read moves no median over a full-period sine
  and no tolerance-bounded period. The arms without a sine take the
  `windowStep` path, so their exact-count assertions stand untouched.
- `std::llround` on the product and a later add of the integer step
  give the same value as a round of the whole sum, because the step is
  an integer and the round-half-away rule translates over integers.

**Alternatives**

- A cast of the whole sum in unsigned after a double-side clamp:
  rejected. It clears the sanitizer finding, and the spec takes the
  signed-type form; the signed type states the arithmetic the arm
  means, and the clamp states the boundary in one place.
- A `std::max` over the double before the cast: rejected for the same
  reason; it hides the type discipline the requirement names and keeps
  the negative case at the mercy of the cast.
- A non-negative amplitude in the plan: rejected. It fixes the two
  planted arms and leaves the undefined conversion on every future
  plan that names a dip below the step.

## FR-008: the series hand-over and the latch-time snapshot

**Decision (D-5)**

1. The join rule stays the rule of the run-level and per-benchmark
   folds: the runner and the report read `ProbeCpuState` only behind
   the join edge of `stopAndJoin`
   (`source/harness/cli.cpp:2020-2025`, `source/harness/report.cpp:873`
   and `978`). The probe thread keeps writing the series into the
   state its item owns; no second buffer joins.
2. A new per-cpu snapshot type joins
   `source/harness/detail/jitter_probe.hpp`: one object per probe cpu
   that carries the series prefix copied at the latch line, its kept
   count, the dropped-sample count, the five calibration figures
   (`workCount`, `unitTimeNs`, `targetNs`, `quantumNs`,
   `calibrationSpanNs`) and the method. The item builds it once at the
   latch line, after `seriesKeptAtLatch` stands stored
   (`source/harness/jitter_probe.cpp:589`).
3. One atomic hand-off per cpu publishes the snapshot: the item
   release-stores one flag at the latch line, and the mid-run fold
   sites acquire-load it before they read the snapshot. The store sits
   at the latch line, the loads sit at the fold sites, and both stand
   off the hot path. The alongside loops gain no lock and no atomic.
4. The mid-run folds read the snapshot only: a fold over the snapshot
   answers the same `JitterSeriesStats` the state folds answer, and
   the mode=both record site (`source/harness/cli.cpp:1379-1385`) and
   the method noise gate (`source/harness/cli.cpp:1302-1325`) read it.
   The header documents the new fold and the snapshot with the
   `\pre`/`\post` pairs the DBC gate requires (II).

**Rationale**

- The tsan job read the main thread reading `seriesFigures` at
  `source/harness/jitter_probe.cpp:708` while the probe thread wrote
  the same state. The racing pair at the base: the mode=both fold
  calls `seriesStatsOf(state, state.seriesKeptAtLatch)`
  (`cli.cpp:1384`), whose precondition reads `state.seriesKept`
  (`jitter_probe.cpp:1016`) and whose figures read
  `state.droppedSamples` (`jitter_probe.cpp:718`), while the
  alongside loop writes both in `keepSeriesOrDrop`
  (`jitter_probe.cpp:339-347`). The latch barrier orders the prefix
  elements and the calibration figures, so the race sits on the two
  counts the loop keeps advancing.
- The record site cannot wait for the join: the pre-check block prints
  before the batch's tables (`cli.cpp:1341-1346`), and the mode=both
  probe runs beside the whole batch. Deferring the record behind
  `stopAndJoin` would move a user-visible block after every benchmark
  row.
- The method noise gate at `cli.cpp:1306-1313` folds the whole live
  kept set in mode=both when `strict` stands set, so it races the same
  way. The latch-time prefix is the pre-check window's series, which
  is the set the gate means to judge, so the snapshot read is the
  race fix and the semantic fix at once. No scenario arms that pair
  today, and the fix covers the site before a scenario does.
- The record's gap fold (`cli.cpp:1368`) reads the gap fields. A
  series method keeps no gap record in its alongside loop, so those
  fields stand stable behind the latch for `fwq` and `ftq`. A `tsc`
  both-run keeps writing them, and the implement phase answers that
  site with the same snapshot shape: the snapshot carries the gap
  figures the fold reads, or the record reads the gap fold behind the
  join for the `tsc` case. The choice lands in the plan with the
  coverage mapping of FR-011.
- The hand-off form follows the sites: one copy at one line, one flag,
  two readers. The hot loops touch neither. A mutex and atomics on the
  counts sit on the hot path and FR-008 bans both.

**Alternatives**

- Build the record after `stopAndJoin`: rejected above; the print
  order is a report contract.
- Make `seriesKept` and `droppedSamples` atomics: rejected; the sample
  path updates them per unit, so the hot path gains an atomic.
- Guard the state with a mutex: rejected for the same ban.
- Copy the whole `ProbeCpuState` at the latch: rejected; it copies the
  gap buffer and the series twice, and the fold reads the prefix, the
  drop count and the calibration figures alone. X.2 keeps the copy at
  the fields the fold reads.

## FR-009: the prose fixture in one process

**Decision (D-6)**

1. The runner imports the gate as a module: it loads
   `tools/prose/prose_gate.py` once through `importlib`, and every
   gate invocation except the timing case calls the module's entry
   point in process. The runner captures the entry's output with
   `contextlib.redirect_stdout` and `redirect_stderr`, reads the exit
   code from the return value, and switches the working directory per
   case with `os.chdir`, restored after each case. The rule data loads
   per call, because the loader mutants of T012 exist to prove the
   load-time validation fires.
2. The runner keeps a tally: `GATE_RUNS` records one row per gate
   invocation (phase, discovery mode, working directory, rules file, exit
   code, and the sources, units, findings and skipped counters of its
   summary line), and the matrix writes it under `gate_runs`. Two asserts
   hold the tally: the invocation count of each phase against the count its
   design prescribes, and the counters each tree-mode run reports against
   the counters the gate's own discovery names over that tree. No clock
   stands in the runner.
3. The internal gate-run assert stays untouched: the timing case
   (`run_prose_gate_fixtures.py:1008-1062`) keeps its subprocess run of
   the gate over the repository root, its 10-second check, its exit
   0-or-1 acceptance and its `GITHUB_ACTIONS` skip. The subprocess form
   is the shape the production gate takes, and the assert measures
   that shape.

**Rationale**

- The entry spent 16.7 s alone on corsairone. The runner drives about
  forty gate subprocesses: the loader mutants (five), the prose and
  marker cases, the parity pair, the eighteen commit cases with one
  gate call each plus a prose call where the case expects one
  (`run_prose_gate_fixtures.py:1160-1196`), and the mixed-range case.
  Each subprocess pays interpreter start plus the rule load. The
  in-process calls remove that cost per case and keep the git work,
  which stays subprocess because the harness never runs git in the
  invoking repository (`run_prose_gate_fixtures.py:14-15`).
- The bound measures the whole entry's wall time in a parallel run by
  construction: under `ctest -j` the runner's own wall time already
  carries the contention, so one self-timing covers the parallel case
  with no second measurement path.
- The internal assert's subprocess keeps its production shape, and the
  two bounds stay independent: the assert bounds one gate scan, the
  self-timing bounds the whole entry.

**Alternatives**

- Move the timing case in process with the rest: rejected. The assert
  would stop measuring the interpreter start the production gate pays,
  and the requirement names the assert untouched.
- Cache the loaded rules across cases: rejected. The loader mutants
  re-validate per load, and a cache would answer a mutant with the
  previous file's rules.
- Split the entry into several ctest entries: rejected. The bound and
  the single-process shape are the requirement, and more entries move
  the cost back onto the suite's process budget.

## FR-010: the spec 043 Phase 10 rows

**Decision**

The Phase 10 row set of `specs/043-fwq-ftq/tasks.md` is T048 to T057
plus T061; the Phase 11 rows (T058, T062, T059) stand outside SC-007's
scope and keep their state. The 045 lane runs every Phase 10 gate it
can execute at the 045 head and ticks the row when its verify commands
pass:

- T048, T049, T051, T052, T053, T054, T055, T056: runnable in full,
  ticked when green. The Part B commit whose work proves each gate
  ticks its rows, and the commit carries the Part B footer
  `Refs: specs/043-fwq-ftq N13c-F`.
- T050: the lane runs the coverage contract and records the numbers in
  the PR body, and the row stays open. Its verify text demands
  branches at 100.0 percent and a shortfall of 0 rows; the 045
  allowance holds 100 percent lines with a shortfall of 21 or less
  (SC-002, FR-011). The open reason names the allowance.
- T057: stays open. The row reads the acceptance record T045 of the
  043 lane wrote and the absence of the script from CI; the 045 lane
  runs no acceptance script of the 043 lane. The reason names the
  owning lane. The records exist at the base
  (`specs/043-fwq-ftq/results/acceptance-2026-11-02.md` and the rerun
  file, eight `##` sections each), and the row still waits for the
  lane that owns it.
- T061: stays open. The row pushes `043-fwq-ftq` and records that
  branch's CI run reference; the 045 lane pushes `045-merge-fixes`
  under the commit rules of the 045 spec. The reason names the owning
  lane.

The verify text of every row stands unchanged. A row that stays open
carries its reason as a note line under the row, and no Verify line
moves. The alternative of recording the reasons in the PR body alone
loses the record the row's reader reads beside the row.

**Rationale**

- PR #72 left Phase 10 unrun, and every row stands unticked at the
  base. The gates of T048 to T056 are the standing suite gates
  (VIII), and the 045 lane runs them anyway for SC-001 to SC-006, so
  the ticking costs the commands the lane runs regardless.
- The contradiction rows name the allowance or the lane, per FR-010
  and the clarification answers: T050's shortfall of 0 against the 045
  allowance of 21, T057's and T061's artifacts of the 043 lane.

**Alternatives**

- Tick T050 with a note that the shortfall differs: rejected; a ticked
  row whose verify text fails is a false record (US-5).
- Reword the 043 verify text to the 045 allowances: rejected; FR-010
  and the spec forbid any 043 verify text change.
- Leave every row open until the PR head: rejected; the commit rules
  tie each tick to the commit whose work proves it, and a bulk tick at
  the head hides which commit carried which gate.

## FR-011: the coverage of the PR #72 code

**Decision**

Every line the PR #72 code added shall be covered by a real test, and
the branch shortfall over the whole tree shall read 21 or less. The
implement phase runs `ci-coverage`, reads the dark lines and the short
branch positions by file, and maps each one to a named arm in the Part
B test files. The mapping method:

- The series and calibration surfaces of
  `source/harness/jitter_probe.cpp` map to the scripted arms of
  `test/source/harness_fwq_ftq_test.cpp`: the `ScriptedReader` and the
  `ScriptPlan` table (`test/source/harness_fwq_ftq_test.cpp:146-380`)
  drive the calibration search, the unit loops, the drop counting and
  the latch prefix with exact read counts.
- The window-fold surfaces map to the scenario readers of
  `test/source/harness_jitter_window_test.cpp` (`CountingReader`,
  `DipSeriesReader`, `GapSeriesReader`, `FlatSeriesReader`, lines
  `142-179` and `869-991`) and to the state-level fold scenarios with
  the planted series (`fillFwqSeries`, `fillFtqSeries`, lines
  `950-976`).
- The option ranges and the refusal rows map to
  `test/source/harness_jitter_cli_test.cpp`; the spectrum surfaces map
  to `test/source/harness_spectrum_test.cpp` and
  `test/source/harness_jitter_bytes_test.cpp`.
- The mid-run fold sites of FR-008 map to the snapshot fold's own
  scenarios in `harness_jitter_cli_test.cpp` and
  `harness_fwq_ftq_test.cpp`.

The kept-list remedy never fires for Part B: no Part B line sits under
a marker, and `grep -rn LCOV_EXCL source/harness` reads no match at
the base. SC-002 governs strictly, so a PR #72 line that no real test
can reach stops the work and the agent reports, like the
public-layout rule. The PR body carries the dark-line count by file
and the shortfall figure.

**Rationale**

- The PR #72 coverage state read 44 dark lines and 52 short branch
  positions (T070 of spec 043). The dark surfaces are the ones the
  043 tests drive with plan tables, so each line has a scripted answer
  that reaches it; the arms exist in the files the 045 scope names.
- The branch allowance stands at 21 (FR-015 keeps it unchanged), so
  the 52 short positions fall to 21 or less through the same arms, and
  the gate prints the shortfall figure the PR body records.

**Alternatives**

- A kept marker on a dark PR #72 line: rejected; FR-011 names the
  remedy for lines already under a kept marker, and no Part B line
  sits under one.
- An `--omit-lines` entry or an exclusion class: rejected; FR-015 bans
  net-new exclusions and the accepted exceptions stay as they are.

## FR-012: the CMakeLists comment

**Decision**

The two comment sites credit the archive SOVERSION 5 move to spec
044. PR #70 (T-LCOV) made that move: the version-block comment at
`CMakeLists.txt:45-51` (`the archive SOVERSION stays 5, the number
specs/044 moved`) and the harness-target comment at
`CMakeLists.txt:399-407` (`keeps its SOVERSION 5, the number specs/044
landed`). The change touches comments alone; no number changes
(`SOVERSION 5` and `SOVERSION 20` stand), and the fix rides the Part B
commit that carries the record fixes, with the Part B footer.

**Rationale**

- The archive SOVERSION moved to 5 in the change PR #70 (T-LCOV)
  landed, and the comments at those sites name spec 044. A reader of
  the version block learns the wrong history (US-5).
- Part B scope owns the `CMakeLists.txt` comments (the scope list of
  the 045 spec), so the fix rides the Part B record commit and carries
  the Part B footer; the number lines stand untouched, so SC-008
  holds.

**Alternatives**

- A separate commit: rejected; the commit rules carry one topic per
  commit, and the record fixes of Part B are the topic the comment fix
  joins.
- Fixing the comment wording beyond the credit: rejected; X.3 keeps
  the change at the lines the requirement names.

## Verification at a glance

| Item | The gate that proves it |
| --- | --- |
| FR-006 | `harness_jitter_window_test` green in every job; the entries carry real jitter records |
| FR-007 | `ci-sanitize` and `ci-tsan` report no finding in the jitter tests |
| FR-008 | `ci-tsan` reports no race; the mode=both scenarios pass |
| FR-009 | `prose_gate_fixtures` passes in a parallel `ctest -j` inside the bound |
| FR-010 | the Phase 10 rows carry their tick or their open reason |
| FR-011 | `ci-coverage` reads 100 percent lines and a shortfall of 21 or less |
| FR-012 | the comment names PR #70; no number changes |
