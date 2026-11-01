# Implementation-readiness analysis: specs/044-fix-rocky-pmu-stack

Reviewer lens: can an implementer execute `tasks.md` top to bottom without
getting stuck or making a wrong turn? The review read the eight artifacts of
the spec directory and the six files the tasks edit, at the base `d454bcff`
(the branch state the working tree carries). Principle XI binds this page:
short sentences, no narration, exact names.

## Verification summary

What checked out, with the evidence that pins it:

- **Base line numbers.** `FastContext` stands at `pmu.hpp:337` with the
  five members the data model lists; the inline destructor stands at
  `pmu.hpp:534-537` and delegates to `fastContextClose` (`pmu.hpp:528`).
  `fast_read.cpp` carries `dbc.hpp` at `:26`, `SG_ENSURE` at `:124`,
  `SG_REQUIRE` at `:272`, `:279`, `:320`, `:323`, `fastContextOpen`'s
  member-assignment shape at `:228-232`, and `fastContextClose`'s release
  at `:385-401` under the `#if !defined(SG_PMU_FAST_X86)` / `#else` split
  at `:149` / `:176` / `:403`. The seam test's six designated-initializer
  sites stand exactly where T012 names them (`:720`, `:779`, `:896`,
  `:910`, `:965`, `:1019`), the value-initialized array at `:1010`, the
  default constructions at `:1039` and `:1831`, the count helpers at
  `:172` and `:191`, and the lifecycle registrations at `:3819-3821`.
  `kPassReads` stands at `jitter_probe.cpp:50`; `installTestReader` at
  `jitter_probe.hpp:144`; `GapSeriesReader` at `:535-549`; the FR-028
  install-clear pattern at `:863-865`; the fold checks at `:816-820` and
  the per-pass magnitude checks at `:1064-1068`.
- **The CountingReader is buildable from the named public API.**
  `FakeProvider::open(const LeafSet&, const Target&)` is public
  (`counters_fake.hpp:309`) and ignores the target. `LeafSet` carries
  `addresses` and a `disclosureColumn` that defaults to
  `kNoDisclosureColumn` (`counters_provider.hpp:147-155`), so a
  single-address `machine/tsc` set writes exactly the one point the
  probe's one-cell `PointSink` holds. `WindowReader` defaults `m_thunk`
  to `defaultThunk` (`counters_provider.hpp:402`), so the decorator needs
  no `setThunk` call and each probe read reaches `readPoints` through one
  virtual call. `installTestReader(std::unique_ptr<counters::WindowReader>)`
  takes the decorator by the base pointer. The test already includes
  `detail/jitter_probe.hpp` and `speedgun-ng/counters_fake.hpp`, and the
  probe resolves the thunk from the one session reader
  (`jitter_probe.cpp:218-226, 434`), so the count covers every armed cpu.
- **The wait-target arithmetic serves the checks.** One measurement pass
  takes `kPassReads` reads per cpu (the initial read at
  `jitter_probe.cpp:243` plus the loop to `:245`), so a pass's worth of
  rides is `kPassReads` times the armed cpu count, and the rebase wait's
  one read per armed cpu matches the two-cpu and one-cpu shapes the six
  runs arm. The fake's playback cursor is per window
  (`fake_provider.cpp:70-94, 137-145`): the runner's window stamps
  (`windowStampNow`, `runner.cpp:54-77`) open their own windows and never
  advance the decorator's delegate cursor, so inside a gate body the
  cursor advances only by probe reads and a single-cpu run's kept gaps
  carry the exact pass magnitude the checks at `:1064-1068` and `:1279`
  require. The close edge seeds from the delegate's hand-off, so it
  stands above every kept stamp. The kept-buffer check `gapCount == 1`
  with `droppedGaps >= 1` (`:1093-1098`) needs the full ride count, so
  the 1,024 target is load-bearing and no decoration.
- **The move-operation placement compiles conceptually.** The
  declarations need no include the header lacks; the out-of-line
  definitions sit outside the platform branch, where `dbc.hpp` and the
  `pmu.hpp` declarations are both visible, and one definition serves the
  x86 and the stub build. No translation unit copies a `FastContext`:
  the only assignment is the partial-open site the move assignment
  serves, and every other use is a `unique_ptr` hand-back or a
  reference. The pair gate's scanner handles trailing-return
  out-of-line definitions (`skip_trailing_return`, `after_params`), so
  the header-doxygen to `fast_read.cpp`-enforcement pairing the
  contracts name is reachable.

## Findings

1. **Medium-low: artifact `tasks.md` T013 (and `research.md` item 1);
   issue:** the task names the four sites' construction lines but only
   says "the multiplex scenario's field mutations go through the mapped
   pointer". The multiplex scenario reads `page` and `rawPage` at about
   twenty later sites (`:731-845` and `:784-843`), so the stack objects
   cannot survive as separate values: they must become the mapped
   pointers (or references into the mappings), or the assertions measure
   a stale object. **Remediation:** extend T013 with one sentence: "the
   multiplex scenario's `page` and `rawPage` become the mapped
   pointers, and every later read and write of them (`:731-845`,
   `:784-843`) goes through the mapped pointer." The wrong turn fails
   loudly at the `:737-739` check, so T016 catches it, but the task
   should state the shape the edit takes.

2. **Low: artifact `tasks.md` T022 and T023, `research.md` risk 3;
   issue:** the gate-body run citations drift from the code. The
   roundtrip run's `captureRun` stands at `:741`; `:723` (`:723`
   opens the scenario's comment block); the `bmGate` pass wait spans
   `:146-157` and the jump wait `:158-169` (T023 names `:146-164` and
   `:166-172`, which straddle each other). **Remediation:** correct the
   citations to `:741`, `:146-157`, and `:158-169`, or note them as
   region pointers in place of exact lines. No other citation in the
   eight artifacts missed its target.

3. **Low: artifact `tasks.md` T023, `research.md` "The waits";
   issue:** the `bmGate` pass wait's new target (`kPassReads` times the
   armed cpu count, cumulative from the install) stands already met at
   the first gate-body call in every scenario: the measurement pass
   completes behind the latch inside `JitterProbe::start`
   (`jitter_probe.cpp:243-252, 306, 462`) before `start()` returns, so
   the wait never spins. The property it guards (the threshold measured
   at the one-tick series before the jump) holds structurally, so the
   vacuous wait is harmless; an implementer checking the behavior
   against the E-05 rationale ("one pass's worth of rides inside the
   window") will find the wait delivers nothing. **Remediation:** state
   in T023 that the pass wait's target is met by construction at body
   entry and the wait stands as a guard, so the implementer does not
   read the no-op as a defect to fix or a target to raise.

4. **Low: artifact `tasks.md` T021; issue:** the test file includes no
   `<atomic>`; `std::atomic` reaches it only through
   `detail/jitter_probe.hpp`'s own include. The `ci-ubuntu` build runs
   clang-tidy over every translation unit, and an include-what-you-use
   style finding is avoidable. **Remediation:** add `#include <atomic>`
   beside the test's standard includes in T021's description.

5. **Informational: artifact `tasks.md` T014; issue (no defect):** the
   move-assignment test's "exactly one above the baseline" mid-block
   check is consistent with the helpers: after the assignment the target
   holds the source's pair (baseline + 1) and at scope exit the counts
   return to the baseline. The wording reads ambiguous on first pass.
   **Remediation:** none required; the description matches the code.

## Verdict

**Ready.** No task blocks, no task contradicts the code it edits in a
way that sends the implementer down a wrong path without a visible
failure, and every design claim the tasks lean on (the seam's public
API, the per-window cursor, the latch-ordered measurement pass, the
one-release close path) checked out against the source at the base.
The five findings are citation drifts and one under-specified edit
shape; findings 1 through 4 carry exact remediations an implementer can
apply in minutes before Phase 1 starts. The commit groups, the grep
gate of T024, and the branch state (at `d454bcff` with the spec
directory and the feature pointer the only pending paths) all stand as
`tasks.md` describes.
