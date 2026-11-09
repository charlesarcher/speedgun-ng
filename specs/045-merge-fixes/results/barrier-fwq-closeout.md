# Phase 14 closeout: the barrier wait entry and the FWQ sample

Host thrawn: Linux `7.3.0-5-cachyos`, AMD EPYC 9960X (24 cores, 48 threads),
GCC and gcov `16.2.1`, clang `23.1.3`, lcov and genhtml `2.6-0`, cmake and
ctest `4.4.4`, 48 cpus online and all 48 in `sched_getaffinity`. Dates
2026-11-08.

Base: `origin/master` at `2d4d8c57975a86db6c77536f7090aba2bdaf6417`. The
branch is `fix/barrier-entry-fwq`. Every reading below stands taken at the
working tree of that base with the phase's edits applied, before the squash,
and stands labeled preliminary. The pull-request body names the measured
head with the same rows.

## 1. What the fixture held at the base (FR-027, SC-025)

Four things stand separate in `test/source/harness_context_barrier_test.cpp`:

| Thing | Symbol |
| --- | --- |
| the test handshake | `Observer::ready`, `Observer::go`, `watch` |
| the entry flag | `Observer::entered`, stored by `participantDrive` before the call |
| the status-file read | `readStatus`, `stateCharOf`, `selfVoluntarySwitches` |
| the barrier itself | `ContextBarrier::arriveAndWait` |

The edges a wait-mode claim needs, in the operations that carry them:

| Edge | At the base |
| --- | --- |
| the participant's arrival is counted before it waits | held: `m_arrivalCount.fetch_add` at `source/harness/execution_context.cpp:854` |
| the filling arrival releases every waiting participant | held: the release on `m_generation.fetch_add` at `:865`, read by the acquire at `:889`, `:893` and `:895` |
| the participant entered a wait arm before the observer arrived | lacking: `observer.entered` stands stored before `arriveAndWait` is called, so it names no arm and stands set whether or not a wait follows |

The run the base cannot rule out: the observer holds the round with a fixed
count of steps, and a participant that never enters an arm still answers
every assertion. `results/coverage-data-repair.md:621-623` records the
reading: 12 of 20 rounds at the head before the fix took no wait arm at all.

## 2. The observation seam (FR-028, SC-027)

`sg::detail::WaitArmObserver` is a `noexcept` function pointer of `WaitMode`,
declared in `source/harness/detail/internal.hpp`. `installTestWaitArmObserver`
takes an `std::optional` of it, defined in `source/harness/execution_context.cpp`
over a function-local static, the shape `placementSlot` carries at `:128-132`.
An empty optional clears the slot.

`ContextBarrier::arriveAndWait` calls `detail::noteWaitArm` once per waiting
arrival, at the head of the arm its chain enters, after the branch that
answers a filling arrival. The call names the arm of its own site:

| Arm | Call |
| --- | --- |
| SPIN | `detail::noteWaitArm(WaitMode::SPIN)` |
| YIELD | `detail::noteWaitArm(WaitMode::YIELD)` |
| BLOCK | `detail::noteWaitArm(WaitMode::BLOCK)` |

No call stands inside a spin loop. The production path reads one static and
takes one branch per waiting arrival, and nothing per iteration.

The record reaches the observer through one release store: the fixture stores
the arm word with `memory_order_release`, and the gate acquires that word. A
first form of the gate acquired a separate relaxed counter and read the word
after it; 8 of 240 runs then read a word that had not arrived. The gate reads
the word it acquires.

The seam stands internal. The diff names no file under `include/`. The
harness target carries `CXX_VISIBILITY_PRESET hidden` at `CMakeLists.txt:130`,
and the tests link the static archive, since `BUILD_SHARED_LIBS` stands off by
default. The shared-symbol reading stands in section 7.

## 3. The fixture gate and the substitutions (FR-029, SC-019, SC-020)

`watchEntryOrDeparture` holds the observer's own arrival until the arm word or
the participant's departure stands published. `releaseAndBracket` then reads
the word and names the arm the scenario's policy selects. The four scenarios
carry the proof: `spinWait`, `yieldWait`, `insideOutside` and `round`. The
first three gate on the record of the arm their policy names. `round` carries
the arrival that fills the round: three worker records, and no record for the
arrival that waits on nothing.

The park watch of the BLOCK scenario stands bounded by `kParkWatchReads`, a
fixed count of status reads. An unbounded watch holds the scenario open when a
build's blocking mode never parks, which the third substitution below is.

Each substitution stands applied to a build of this tree, 240 runs of the
scenario it answers, 8 in flight:

| Row | Change | Answer |
| --- | --- | --- |
| R1 | the chain routes a SPIN participant into the BLOCK arm | 240 of 240 fail: `the record names the wait arm the participant's own policy names` |
| R2 | the chain routes a YIELD participant into the BLOCK arm | 240 of 240 fail: the same line |
| R3 | a participant that runs on no context is given a spinning arm | 240 of 240 fail: `a participant that runs on no context waits in BLOCK: its task state stands parked in the kernel before the release` |
| R4 | the blocking wait stands removed and its arm left empty | 240 of 240 end by `SIGABRT`, exit 134, through the postcondition at `source/harness/execution_context.cpp:961-963` |

R4 answers with the abort in every run: the participant thread reaches the
postcondition before the observer thread reaches the record check, so the
named record line never prints. The gate's second arm still stands needed: it
is what keeps the scenario from waiting on a record that cannot arrive when
the round released before the participant reached the chain.

An arm-body swap stands outside this set. The record names the arm of the
call site, and two spinning bodies both keep the voluntary-switch count flat,
so no fixture reading separates them. O-6 carries the class and the tool that
would close it.

## 4. The stress matrix (FR-032, SC-024)

```
seq 1 240 | xargs -P 8 -I{} build/test/harness_context_barrier_test
seq 1 240 | xargs -P 8 -I{} build/tsan/test/harness_context_barrier_test
```

| Tree and shape | Runs | Failures |
| --- | --- | --- |
| `ci-rocky` release, 8 in flight | 240 | 0 |
| `ci-rocky` release, 8 in flight, pinned to cpus 0 and 1 | 240 | 0 |
| `ci-rocky` release, 48 in flight | 480 | 0 |
| `ci-rocky` release, 48 in flight, a full suite running beside | 480 | 0 |
| `ci-tsan`, 8 in flight | 240 | 0 |

`ctest --repeat` knows `until-fail`, `until-pass` and `after-timeout`, so no
`ctest` form of this count exists. The direct-binary form with 8 in flight is
the shape the phase 13 counts stand taken at.

## 5. The blocking arm and a wake that answered nothing (specs/020 FR-014)

Three full release-suite runs at this tree before the change, one plain, one
pinned to two cpus and one pinned to four, each answered 189 of 190 with the
same entry failed: `harness_context_barrier_test`, scenario `round`. The
child's stderr carried the postcondition line:

```
[postcondition] a round that released without advancing the generation (FR-014)
(predicate: m_generation.load(std::memory_order_acquire) != enteredRound)
```

That check stands at the end of `arriveAndWait`. The two spinning arms answer
their own exit condition inside their loop, so the blocking arm stands as the
only way to leave the wait while the round stands held: the standard lets
`std::atomic::wait` return although the value it was given still stands. The
arm now re-checks that condition and waits again inside the loop, the shape
the two spinning arms already carry. The arm keeps its mode, its place in the
chain, its read of no clock and its absence of any timeout, so specs/020
FR-014 and D-03 stand held as stated.

| Reading | Answer |
| --- | --- |
| three full release suites at this tree, before | 3 of 3 runs failed, scenario `round` |
| the same three suites at the base | 0 of 3 |
| 480 runs at 48 in flight, a full suite running beside | 480 of 480 failed |
| the same protocol at the base | 0 of 480 failed |
| 480 runs at 48 in flight, a full suite running beside, after | 0 of 480 failed |
| 240 runs at 8 in flight, release, after | 0 of 240 failed |
| 240 runs at 8 in flight, pinned to two cpus, after | 0 of 240 failed |
| 240 runs at 8 in flight, `ci-tsan`, after | 0 of 240 failed |
| three full release suites after | 190 of 190, 190 of 190, 190 of 190 |

The fault did not answer on demand outside those loaded shapes. A bare loop
of 240 or 480 direct runs, at 8 or 48 in flight, answered 0 failures at the
pre-change content too, and three further full suites answered 0. The record
therefore states the fault as seen three times in suite runs and 480 times
under the loaded protocol, and states the re-check as the change that makes
the seen violation impossible.


## 6. The FWQ sample in three forms (FR-030, SC-022)

The three forms ran at the base and at this tree. The failure of record stands
on corsairone: the `ci-coverage` lane answered 110.21 s with the child timed
out by the 110 s grant of `tools/docs/report_shape.py`.

| Form | Base | This tree |
| --- | --- | --- |
| release suite, `-j 48` | 24.29 s, 25.14 s, 25.52 s | 15.84 s |
| release, entry alone | 19.18 s, 20.5 s | 11.17 s, 11.37 s, 11.40 s |
| `taskset -c 0,1` suite, `-j 2` | 17.54 s | 8.93 s |
| `taskset -c 0,1,2,3` suite, `-j 4` | not recorded | 13.14 s |
| `ci-sanitize` suite | not recorded | 16.53 s |
| `ci-tsan` suite | not recorded | 53.02 s |
| `ci-coverage` suite | 55.92 s in lane, 50.31 s alone | 30.32 s |

On thrawn no form of the failure reproduced: three full release suites, one
full instrumented suite, 20 pinned runs of the entry, and 8, 24 and 48-way
process contention all passed at the base. The row stands unresolved under
D-21, and the mechanism stands proved by the sweep below.

The three alternatives SC-022 separates, each with the line it prints:

| Alternative | Line |
| --- | --- |
| the 110 s grant of `tools/docs/report_shape.py:368` | the script's own failure line, exit 1 from the script |
| the 120 s `ctest` ceiling of `test/CMakeLists.txt:2197` | `***Timeout` |
| a shape finding | `sample output block holds 156 lines, the run printed 125` |

The mechanism: the example's ring stands at its default, so the window fills
the 65,536-point cap, and `dftPeriods` runs `N x N/2` inner steps
(`source/harness/spectrum.cpp:147-167`). The pair `seriesStatsOf` and
`addSpectrumTo` runs twice per probe cpu, once for the report record at
`source/harness/cli.cpp:582-583` and once for the console fold at
`source/harness/report.cpp:908-909`, so the example pays six cap transforms.
The ring sweep separates that from the other candidates: with everything else
fixed, `gaps=1024` answers 155 ms, `gaps=8192` answers 454 ms, `gaps=32768`
answers 4.89 s and the default answers 17.81 s.

## 7. The fix and its unit proof (FR-031, SC-028)

`ProbeCpuState` carries `mutable std::optional<SpectrumResult> spectrumAnswer`.
`addSpectrumTo` reads the stored answer when it stands present at the step the
fold reads, and computes and stores it otherwise. The step stands part of the
answer, because the resample reads it. The two fold sites keep their `const`
pointer to the state, so `source/harness/cli.cpp` and
`source/harness/report.cpp` stand untouched.

`theSpectrumAnswerFoldsOncePerProbeCpu` in
`test/source/harness_fwq_ftq_test.cpp` folds a scripted series, plants a
sentinel in the stored answer, folds again, and reads the sentinel back. It
also folds at another step and reads a recomputed answer.

| Build | Result |
| --- | --- |
| this tree | passes, exit 0 |
| the reuse branch forced off | fails: `the second fold reads the stored answer; a build that computes the transform twice loses the sentinel` |

## 8. The lanes at this tree (FR-032, SC-023, SC-024)

| Lane | Command | Exit | Reading |
| --- | --- | --- | --- |
| release suite, three runs | `ctest --test-dir build ... -C Release -j 48` | 0 | 190 of 190 each, 71.39 s, 71.19 s, 70.74 s, 1 skipped |
| two-cpu pinned | `taskset -c 0,1 ctest ... -j 2` | 0 | 190 of 190 passed, 210.82 s |
| four-cpu | `taskset -c 0,1,2,3 ctest ... -j 4` | 0 | 190 of 190 passed, 123.73 s |
| sanitizer | `ctest --test-dir build/sanitize ... -j 48` | 0 | 185 of 185 passed, 163.38 s |
| thread sanitizer | `TSAN_OPTIONS=... ctest --test-dir build/tsan ... -j 48` | 0 | 186 of 186 passed, 253.41 s |
| instrumented | `ctest --test-dir build/coverage ... -j 48` | 0 | 185 of 185 passed, 397.00 s |
| coverage target | `cmake --build build/coverage -t coverage` | 0 | lines 100.0% (18681 of 18681), branches 99.8% (9263 of 9283), gate: shortfall 20 rows within the allowance of 21 |
| markers | `git grep -c LCOV_EXCL -- source include` | 0 | 84, the standing cap |
| format | `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake` | 0 | clean |
| spell | `cmake -P cmake/spell.cmake` | 0 | clean |
| prose, tree | `python3 tools/prose/prose_gate.py --mode tree` | 0 | 1087 sources, 212966 units, 0 findings, 0 skipped |
| prose, fixtures | `python3 test/prose-gate-fixture/run_prose_gate_fixtures.py --check both ...` | 0 | clean |
| example drift | `python3 tools/docs/example_drift.py .` | 0 | clean |
| example drift fixtures | `python3 test/example-drift-fixture/run_example_drift_fixtures.py .` | 0 | clean |
| report shape fixtures | `python3 test/example-drift-fixture/run_report_shape_fixtures.py .` | 0 | clean |
| example run | `bash test/example_run.sh build` | 0 | clean |
| example run fixtures | `bash test/example-run-fixture/run_example_run_fixtures.sh` | 0 | clean |
| snippet compile | `bash test/snippet-compile/run.sh . clang++` | 0 | clean |
| time source | `bash test/time_source_gate.sh .` | 0 | clean |
| DBC gates | `cmake --build build -t dbc-gate` | 0 | clean |
| DBC gate fixtures | `ctest --test-dir build -R dbc_gate_fixtures` | 0 | clean |
| analyzer build | `cmake --preset=ci-ubuntu -B build-an` and a whole build | 0 | clean; see the reading below |
| exported names | `cmake --preset=ci-linux-audit -B build-shared -D BUILD_SHARED_LIBS=ON`, install, `nm -D --defined-only` | 0 | harness 210 at the base and 210 at this tree, the sorted lists answer no difference; `libspeedgun-ng.so` 250 with 0 third-party names |
| consumer build | `cmake -S test/consumer -B build-consumer -D CMAKE_PREFIX_PATH=...` | 0 | configures, builds, links, runs |
| FWQ entry, 20 runs | `seq 1 20 \| xargs -I{} ctest --test-dir build -R '^report_shape_fwq_ftq_sample$'` | 0 | 20 of 20 passed, 9.99 s to 12.24 s |

The analyzer reading over the two changed source files, from a whole
`ci-ubuntu` build at this tree against the same build at the base:
`jitter_probe.cpp` answers 145 findings at both, and `execution_context.cpp`
answers 23 at both. One number moves inside a finding that stands at the base
as well: `arriveAndWait` reads a cognitive complexity of 26 at the base and 28
at this tree, because the re-check adds a loop to that function. The check
fires on that function at both ends, the build promotes no warning to an error,
and no new check appears on any changed line.

The instrumented lane needed one repair of its own. The first run of that lane
failed five entries and the gate answered 97.9% lines, because the tree still
held `.gcda` profile files of the earlier base run. The log carried
`libgcov profiling error` for `execution_context.cpp.gcda` and
`overwriting an existing profile data with a different checksum`, and that
line reached the captured stderr of the fork-and-capture rows. With every
`.gcda` removed the same lane answers 185 of 185 and the gate answers 100.0%
lines. No source, no filter and no allowance changed.

## 9. Rows left open

- O-5: the 110 s grant and the 120 s ceiling stand unchanged.
- O-6: an arm-body swap stands outside the substitution set, and the
  disassembly gate that would close it needs a new script and a new entry in
  `test/CMakeLists.txt`.
- O-7: bounding the example's ring answers the cost directly, measured at
  17.6x on the instrumented binary, and the example region stands quoted in
  `README.md`, which stands outside this phase's scope.
- O-8: the probe's per-cpu figures carry names in nanoseconds while the read
  that fills them answers raw counter ticks; on this host the two differ by a
  factor of 4.19.
- The FWQ failure of record stands unreproduced on thrawn, with the mechanism
  proved and the next experiment named: the same entry inside a full
  `ci-coverage` suite on a host that carries the corsairone profile.
