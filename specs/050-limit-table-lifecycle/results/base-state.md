# Base state: the limit table lifecycle (specs/050-limit-table-lifecycle)

T001 wrote this record on 2026-11-05 on the lane host, and every later phase of
this lane compares its own figures against it. Each figure names its source: a
command run on this host today, or a lane record cited by file and line. This
record stands at the base. The lane fixes none of the base failures it names,
because each one reaches code that the lane does not touch.

## Host, toolchain and commit

`uname`, `g++`, `nproc`, `/proc/cpuinfo` and `git` answer the seven rows below,
each one run on this host today.

| Fact | Reading |
| --- | --- |
| Host name (`uname -n`) | `kennykiller` |
| Kernel (`uname -r`) | `7.3.1-1-cachyos` |
| Compiler (`g++ --version`) | `g++ (GCC) 16.2.1 20260810` |
| Cores (`nproc`) | 20 |
| CPU (`grep -m1 'model name' /proc/cpuinfo`) | `Intel(R) Core(TM) Ultra 7 265K` |
| Base commit (`git rev-parse --short origin/master`) | `858c55d9` |
| Lane head at writing (`git rev-parse --short HEAD`) | `500a7940` |

`git diff --name-only 858c55d9..HEAD` names nine spec artifacts and no code
file, so every command in this record measures base code even where it runs at
the head. The `build/` tree stands configured through the `ci-rocky` preset, a
Release developer build with `CMAKE_BUILD_TYPE` set to `Release`.

## The suite at the base

`ctest --test-dir build -N | tail -1` answers `Total Tests: 181` today. The two
full-suite verdicts come from the lane's own measurement record at
`/tmp/050-design-notes.md:111-116`. The log file named in each row carries that
same reading end to end.

| Command | Verdict | Time | Log of that run |
| --- | --- | --- | --- |
| `ctest --test-dir build -j 20 --output-on-failure` | 180 of 181 passed | 94.3 s | `/tmp/050-base-ctest.log` |
| `taskset -c 0,1 ctest --test-dir build -j 2 --output-on-failure` | 180 of 181 passed | 367.4 s | `/tmp/050-base-ctest-taskset.log` |

The first log closes with `Total Test time (real) =  94.30 sec` and one failed
entry, and the second closes with `Total Test time (real) = 367.37 sec` and the
same one failed entry. Both list the same three entries under "did not run".
Each skips through `SKIP_RETURN_CODE 2`, the property set at the
`test/CMakeLists.txt` line named below. Each of the three binaries exits 2 on
this host today, and that property turns an exit of 2 into a `Skipped` verdict.

| Skipped entry | CTest id | Property line | Reason the host prints today |
| --- | --- | --- | --- |
| `counters_pmu_test` | 47 | `test/CMakeLists.txt:470` | No provider seeded a cpu object at this `perf_event_paranoid`, so no hardware event is reachable |
| `counters_overhead` | 59 | `test/CMakeLists.txt:639` | No provider seeded a cpu object, so the overhead has no regime to compare against the clock |
| `harness_topdown_run_test` | 179 | `test/CMakeLists.txt:2294` | Two of 27 scenarios skip, and the run ends on the skip code |

Nine further entries carry the same `SKIP_RETURN_CODE 2` property, and all nine
pass on this host. The three rows above are the entries that report `Skipped`.

## The one entry that fails at the base

`ctest --test-dir build -R harness_hunt_cli_test --output-on-failure` ran on
this host today at the lane head, and it is the measurement this record adds to
the two full-suite readings. The run selects CTest entry 145, reports
`***Failed` in 47.57 sec, and `ctest` exits 8 with
`0% tests passed, 1 tests failed out of 1`. The entry prints two failing
assertions and nothing else:

```text
a-topdown-level-names-the-block-of-a-hunter-run: status 1 from "/home/archerc/code/speedgun-ng/build/test/harness_hunt_cli_test":
HARNESS HUNT CLI TEST FAIL: one run of one benchmark answers one block, so the named level admits no second block of the companion (FR-016, FR-034)

HARNESS HUNT CLI TEST FAIL: the re-executed scenario passes
```

`test/source/harness_hunt_cli_test.cpp:3129` holds the message of the first
assertion, inside `scenarioTopdownLevelNamesTheBlockOfAHunterRun()` at `:3116`.
The check at `:3128` counts the `topdown: level=` headers of a run that names
two hunt addresses and passes `--topdown=2`. It needs that count to equal
exactly 1. The rule it carries is the one-block rule: one run of one benchmark
answers one top-down block. The level the option names therefore admits no
second block from the companion tree (FR-016, FR-034).

`test/source/harness_hunt_cli_test.cpp:3464` holds the message of the second
assertion, inside the driver that walks the scenario list. The check at `:3463`
re-executes the test binary once per scenario name and needs each child to exit
cleanly, through `WIFEXITED(status) && WEXITSTATUS(status) == 0`. The child for
the scenario named above exits 1 with the one-block failure, so the driver fails
this check for the same single cause. Both lines report one fault, and that
fault lies outside the three files this lane changes (`plan.md:373-379`).

## CI run 41946446748

`specs/050-limit-table-lifecycle/plan.md:383-394` records the verdicts of CI run
`41946446748`, taken on the base commit `858c55d9`. Eleven jobs pass, three jobs
fail, and the `docs` job stands skipped. The `tsan` job passes, and D-6 keeps it
the arbiter of a race.

| Job | Verdict | Entries that job names |
| --- | --- | --- |
| eleven jobs, `dbc-gate`, `consumer-release` and the three static-analysis shards among them | Passed | none |
| `test-rocky` | Failed | `prose_gate_fixtures`, `harness_hunt_cli_test`, `harness_hunt_topdown_test` |
| `sanitize` | Failed | one entry, `harness_hunt_cli_test` |
| `coverage` | Failed | `lcov` answers `(inconsistent)` for `source/harness/hunt_topdown.cpp:155` |
| `docs` | Skipped | the job did not run |

The `prose_gate_fixtures` failure rests on a vocabulary probe and a
`git ls-files` ownership fault, as `plan.md:384-385` states it. The downloaded
job log `/tmp/050-ci-base-failed.log` carries the same verdicts. It lists ids
30, 145 and 181 as the `test-rocky` failures at line 3458, and id 140 as the
`sanitize` failure at line 8461. None of these jobs reads the counters code, so
the lane carries them forward unchanged.

## The analyzer reading of the checked build

The checked build is the `ci-ubuntu` tree, the one that wires in clang-tidy and
cppcheck, and its log is `/tmp/050-dev-build.log`. The counting key matters
before any figure: a finding counts once per file, line, column and check name,
and its message text plays no part. Two translation units that answer those four
values alike therefore carry one finding between them.

| Edited file | Raw findings | Distinct findings | Translation units |
| --- | --- | --- | --- |
| `source/counters/system.cpp` | 22 | 22 | 1 |
| `source/counters/detail/core.hpp` | 76 | 19 | 4 |
| `include/speedgun-ng/counters_system.hpp` | 550 | 5 | 110 |

A header finding repeats in every unit that reaches that header. The raw count
therefore divides by the unit count to the distinct count of each row. The tree
total reads 51287 warning lines, the answer of `grep -c ': warning:'` over that
log, and the command answers 51287 again today. Those lines span 303 distinct
path strings, and those strings name 273 files, because one file can stand under
two spellings. `source/counters/detail/core.hpp` carries 76 of those lines, and
`source/counters/linux_pmu/../detail/core.hpp` carries 19 of them.
`/tmp/050-base-analyzer-findings.json` holds the per-file table re-derived from
that same log: 303 rows of path and count. A figure that names one file cites
that table.

## The base coverage of the three files

The three files this lane changes read as follows in
`build/coverage/coverage.info`, counted over their `DA:` rows and their `BRDA:`
rows. A dark branch row is a `BRDA:` row whose taken count reads 0.

| File | Lines | Branch rows | Dark branch lines |
| --- | --- | --- | --- |
| `source/counters/system.cpp` | 282 of 282 | 137 of 140 | 156, 414, 530 |
| `source/counters/detail/core.hpp` | 79 of 79 | 31 of 34 | 324, 343, 345 |
| `include/speedgun-ng/counters_system.hpp` | 20 of 20 | 6 of 6 | none |

The summary field of the first record reads `BRF:134`, while that record holds
140 `BRDA:` rows. The summary field therefore disagrees with the rows it
summarizes, so the lane counts rows, and 137 of 140 is the figure this record
carries. The tree summary pair reads 97.5 percent of 17786 lines and 94.9
percent of 8829 branches. That pair leaves 450 dark branch rows over the whole
tree.

## The two standing counts

Two tree-wide counts bound the change.
`grep -rn LCOV_EXCL source include | wc -l` answers 84 today across
`source/` and `include/`, and `plan.md:60` sets that count as the ceiling the
lane may not exceed. `cmake --build build/dev --target dbc-gate` prints the
interface count, and `spec.md:195-198` records it as 347 interfaces with no gap.
This record measured the two gate scripts that target runs, with output under
`/tmp`, so no file under `build*/` was touched. Both scripts printed 347
interfaces with 0 gaps today.

## The two pinned gate tools

T002 appended this section on the same day, and it names the pair that the two
gate scripts run. Both tools stand outside the repository, in a tool virtual
environment at `/tmp/050-tools-venv`, and the two rows below give the paths.

| Tool | Path | Version |
| --- | --- | --- |
| `clang-format` | `/tmp/050-tools-venv/bin/clang-format` | `18.1.8` |
| `codespell` | `/tmp/050-tools-venv/bin/codespell` | `2.4.3` |

Both tools stood in that directory when T002 opened, so T002 verified them in
place and installed nothing new. The version probe
`/tmp/050-tools-venv/bin/clang-format --version` prints
`clang-format version 18.1.8`, and the speller probe
`/tmp/050-tools-venv/bin/codespell --version` prints `2.4.3`. The `lint` job of
CI installs the pair through pip, and that install line pins the formatter
while it leaves `codespell` unpinned
(`.github/workflows/ci.yml:33-34`). The pip index answers 2.4.3 as the current
`codespell` release today, so the two versions above are the pair CI installs.

Both gate scripts ran at the lane head `500a7940` on this host today, and each
one exits 0. `cmake` reads `-D` before `-P`, so the flag sets the cache entry
that the script reads. The same flag placed after `-P` reaches the script as a
plain script argument. The `default` macro of each script then keeps the bare
tool name in force. This host carries `clang-format` 23.1.4 in `/usr/bin` and no
`codespell` on `PATH`, so a run without the pins measures the host.

| Command | Verdict | Wall time |
| --- | --- | --- |
| `cmake -D FORMAT_COMMAND=/tmp/050-tools-venv/bin/clang-format -P cmake/lint.cmake` | exits 0 and names no file | 6.5 s |
| `cmake -D SPELL_COMMAND=/tmp/050-tools-venv/bin/codespell -P cmake/spell.cmake` | exits 0 with no misspelling | 0.7 s |

## The red build of the new test

T005 appended this section on 2026-11-05, and it records the red that stands
before the storage move. `System()` still stands private at
`include/speedgun-ng/counters_system.hpp:293` at the lane head `500a7940`, so
the new test cannot compile and its CTest entry cannot run. The two command
lines below ran in that order on this host today, and each one names its own
log.

```bash
cmake --preset=ci-rocky
cmake --build build > /tmp/050-red-build.log 2>&1
```

The configure step exits 0, and it leaves its own reading in the log file
`/tmp/050-red-configure.log`. A re-configure is needed here, because
`test/CMakeLists.txt` gained one target and one CTest entry since the suite
count above. `ctest --test-dir build -N | tail -1` now answers
`Total Tests: 182`, one entry above the count of 181 that the suite section
above reports. The build exits with code 2, and the count command
`grep -c 'is private' /tmp/050-red-build.log` answers 10 over that log file.

The first diagnosis in the log names the private constructor behind the first
`System` object of the first case.

```text
/home/archerc/code/speedgun-ng/test/source/counters_limit_lifecycle_test.cpp:88:10: error: ‘sg::counters::System::System()’ is private within this context
   88 |   System first;
/home/archerc/code/speedgun-ng/include/speedgun-ng/counters_system.hpp:293:3: note: declared private here
  293 |   System();
```

The log holds ten such diagnoses in all, because five name `System::System()`
and five name `System::~System()` at the same five lines. Those five lines
read 88, 90, 113, 115 and 132 of the new test, the five `System` objects that
its three cases build. `include/speedgun-ng/counters_system.hpp:293` holds the
constructor and `:294` holds the destructor, each below the `private:` label
at `:292`.

The log holds one `Building CXX object` line, and that line names the new test
file alone. Eighty-five `Built target` lines stand beside it, and each of them
reports a target that built. All ten `error:` lines name
`test/source/counters_limit_lifecycle_test.cpp`, so no second translation unit
reports a fault in this run. The log holds no `warning:` line, and its three
`make` lines name that one object file, its target, and `all`.

`cmake --build` starts no test binary, so the log carries no test verdict and
no failed entry. The failing object stops the run in progress, so `make`
reaches no target scheduled after it. Of the 126 executables in `build/test`,
the log names 35 as built, and the other 91 carry no line there. Each of those
91 binaries keeps a timestamp of 2026-11-04 23:28 or 23:29 from the build that
`/tmp/050-build.log` records. One binary stands absent from that tree, because
`build/test/counters_limit_lifecycle_test` never came to exist in the failed
run.

The five base behavior figures below stand as the lane records already hold
them, because the new test cannot run at the base. Each row names the source
that measured its figure against the base archive, and each source states that
reading in its own table.

| Shape of the new test | Reading at the base | Measurement of that figure |
| --- | --- | --- |
| one built `System` with `cpu` scripted 4 | 4 | `probe_single.cpp` in `/tmp/050-clarify-round2.md:101` |
| the pair, first scripted 4 and second scripted 2, both alive | 2 and 2 | `probe5.cpp` section A1 in `/tmp/050-clarify-round1.md:59-62` |
| one built `System` with one declared path it never scripted | no value | `probe_single.cpp` in `/tmp/050-clarify-round2.md:101` |
| the `machine` root of that same built `System` | no value | `probe_single.cpp` in `/tmp/050-clarify-round2.md:101`, specs/024 data-model E-02 |
| a second `System` declares `cpu` with one leaf, scripts nothing, reads it | 4 | `contracts/limit-table-ownership.md:80`, `probe_unscripted.cpp` in `/tmp/050-clarify-round2.md:99` |

Two rows carry the figures that move, because each `System` then owns a table
of its own. `tasks.md:116-119` states the head pair as 4 and 2 beside the base
pair of 2 and 2. The last row matters, because the accessor at
`source/counters/system.cpp:187` holds one static table for the whole process.
That shared table lets a `System` that scripted nothing read the value another
`System` scripted, and FR-006 ends that behavior. `T010` compares that case's
head answer of no value against this base figure, and the acceptance record
quotes both readings.

## Verification of this record

`test -s specs/050-limit-table-lifecycle/results/base-state.md` exits 0 today,
and the two gates below answer 0 findings and exit 0 today:

```bash
python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/050-limit-table-lifecycle
uvx --from codespell codespell specs/050-limit-table-lifecycle
```
