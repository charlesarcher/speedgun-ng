# The CI rows, and who owns each one

Head `4100bfd8`, CI run 42009222592, base `858c55d9`, CI run 41946446748 for the base. Read with
`gh run view <run> --json jobs --jq '.jobs[] | [.name, .conclusion, .databaseId] | @tsv'` and
`gh run view --job <id> --log-failed`.

## The two runs side by side

| Job | At the base | At this head |
|---|---|---|
| lint, prose-lint, dbc-gate, shared-audit, downstream-consumer, consumer-release, tsan, static-analysis (library, examples, tests) | pass | pass |
| test | pass | **fail** |
| test-rocky | fail | fail |
| sanitize | fail | fail |
| coverage | fail | fail |
| docs | skipped | skipped |

Master's head is red on three jobs before this branch exists. The red entries are named below with the job id
that names them, A reader can check each row with the job id it names.

## The failures, and which one the branch owns

**`harness_limit_disclosure_test`, job `test` (id 127184012657).** The only failure the base job did not carry.
The unit aborted:

```text
/usr/include/c++/15/optional:1179: ... operator*() const & [with _Tp = long unsigned int]:
Assertion 'this->_M_is_engaged()' failed
Aborted (core dumped)
HARNESS LIMIT DISCLOSURE TEST FAIL: the scripted child exits 0
```

Two causes, one in the product and one in the unit. `source/harness/runner.cpp` printed the disclosure line by
dereferencing the catalog figure whenever the record named the probe, with no check that a figure stands, and the
unit seated that finder with the internal recorder. The provider never earned it. On this developer host
the CPUID decode answers `cpu`, so a figure always stands and the dereference reads a real number. On a runner
whose decode answers nothing and whose kernel refuses a group of more than one, the device publishes no figure and
the same expression reads an empty `optional`.

The lane's abort was the only failure the base `test` job did not carry. The contract holds the pair already: the
postcondition on `objectLimitSource` in `include/speedgun-ng/counters_system.hpp:222-224` states that the answer is
`none` exactly where `programmableCounters()` answers no value. The test broke that contract by seating a finder
where the host published no figure, and the run then read a number that was not there. The fix is inside the
contract: a scenario seats a finder only where a figure stands, and where no figure stands it asserts the pairing
rule instead. The console line of FR-050 stays.

**`harness_hunt_cli_test`, job `coverage` (id 127184012510).** The base job passed it, 28.03 seconds, suite
`100% tests passed out of 176`, and the head job failed it, 22.28 seconds. This one is not the branch's, and the
first reading of it was wrong. The failing check stands in `scenarioTopdownLevelNamesTheBlockOfAHunterRun` at
`test/source/harness_hunt_cli_test.cpp:3117`, and counts occurrences of `topdown: level=` in one captured
run, and that header's only producer is `source/harness/report.cpp:360-369`, gated on the vendored top-down table
answering for the runner's CPU identity. The disclosure line shares no bytes with that needle. The check stands
ahead of its own host guard, and its else arm at `:3136-3140` is reachable only where the count is already one,
where it demands zero. The same entry fails on master's own head in `test-rocky` and `sanitize`, and
`harness_topdown_run_test` reports `Skipped` in every job of both runs because the library refuses that host
identity, which is the condition the check assumes away. Whether the entry passes depends on the runner's CPU identity. This branch moves neither.

## The entries that stand at the base

| Entry | Jobs | Base evidence |
|---|---|---|
| `prose_gate_fixtures` | `test-rocky` | base job 126998556049 fails the same entry; the fixture's `tree_counts()` runs `git ls-files` and the Rocky container refuses with `fatal: detected dubious ownership in repository at '/__w/speedgun-ng/speedgun-ng'`; the owner is the workflow's missing `safe.directory`, and `git log -S tree_counts` lands on master's own `3639c4a5` |
| `harness_hunt_cli_test` | `test-rocky`, `sanitize` | base job 126998556049 and base job 126998556106 both fail this entry |
| `harness_hunt_topdown_test` | `test-rocky` | base job 126998556049 fails the same entry, scenario `oneCallReadsNoSourceOfItsOwn` |
| `coverage` job as a whole | `coverage` | base job 126998556062 exits 2 at the trace step while its suite reads `100% tests passed out of 176`; the upload step answers `No files were found with the provided path: build/coverage/coverage.info`, which issue #76 records |

## The read route that makes this branch testable at home

The two owned failures need a host whose CPUID decode answers nothing, which this developer host is not. Install
it as a surrogate: insert `return registers;` directly after `CounterLimitRegisters registers {};` in
`source/counters/linux_pmu/counter_limits.cpp`, configure with `-D CMAKE_CXX_FLAGS=-D_GLIBCXX_ASSERTIONS`, rebuild
and run the suite. The patch is a local reading instrument: it is reverted before any commit, and this branch
carries no copy of it.

## One host fact about building outside the repository

`views_test` climbs from its working directory to the directory whose `CMakeLists.txt` carries the `project(`
block. A build tree under `/tmp` has no such ancestor, so the read answers empty and the unit fails with
`pass: the root CMakeLists.txt can be read`. Run that binary from the repository root, and expect it to pass.

## The surrogate read twice

The same surrogate, one line in `source/counters/linux_pmu/counter_limits.cpp` and
`-D CMAKE_CXX_FLAGS=-D_GLIBCXX_ASSERTIONS`, read twice on `gary`, 2026-11-05, in a tree of
its own at `/tmp/041-surr`.

| Content | Suite | Assertion lines | Named failure |
| --- | --- | --- | --- |
| The unit as the head carries it | 99 percent, 1 of 180 failed | 1 | `harness_limit_disclosure_test`, inside `operator*` on an empty `std::optional`, the CI abort |
| The unit as this commit carries it | 99 percent, 1 of 180 failed | 0 | `views_test` only |

The first row is the CI failure reproduced at home, which is the reason the read route
exists. The second row is the same host, the same silent decode and the same assertions
with the fixed unit: the disclosure unit passes, no entry aborts, and the single failure is
`views_test`, the entry that climbs from its working directory to a `CMakeLists.txt`
carrying the `project(` block and cannot reach one from a tree outside the repository. Run
that binary from the repository root and it passes, as the section above says.

The surrogate reaches the no-figure state by a different door than a CI runner does. The
surrogate zeroes the decoded registers, so the vendor reads `OTHER`, no event name exists
for the device, and the search is asked nothing. A CI runner with a working decode can
still reach no-figure when the kernel refuses a group of more than one. Both doors end on
the same state, no figure and no finder, which is the state the unit now asserts instead of
contradicting.

## The run at `76d84fe8`

Run 42031187929, head `76d84fe8`, read with `gh pr checks 82` and
`gh run view --job <id> --log-failed`. Job ids: `test` 127258076279, `tsan` 127258076211,
`test-rocky` 127258076205, `coverage` 127258076231, `sanitize` 127258076091.

| Job | Verdict | Entry named in the log |
| --- | --- | --- |
| `test` | fail | `harness_hunt_cli_test` |
| `tsan` | fail | `harness_hunt_cli_test` |
| `test-rocky` | fail | `prose_gate_fixtures`, `harness_hunt_cli_test`, `harness_hunt_topdown_test` |
| `coverage` | fail | none, the lcov consistency step stops the job |
| `sanitize` | pass | none |
| `lint`, `prose-lint`, `dbc-gate`, `shared-audit`, `downstream-consumer`, `consumer-release`, `static-analysis` x3, `tsan` at the earlier head | pass | none |

**The branch's own failure is gone.** No job names `harness_limit_disclosure_test`, and the
`test` job that aborted at the replaced head now carries a different single entry.

**`harness_hunt_cli_test` moved jobs, not states.** At the replaced head the base's Rocky
job answered this for the same scenario:

```text
a-topdown-level-names-the-block-of-a-hunter-run: status 1 from ".../build/test/harness_hunt_cli_test":
HARNESS HUNT CLI TEST FAIL: one run of one benchmark answers one block, so the named level admits
no second block of the companion (FR-016, FR-034)
HARNESS HUNT CLI TEST FAIL: the re-executed scenario passes
```

This head's `test` job answers the same three lines, and so does its `tsan` job. Base's
`test` job passed the entry, this head's `test` job failed it, and base's `sanitize` job
failed it while this head's passed it. Four readings across two runs and four runners, with
the check text unchanged: the entry follows the runner's CPU identity, as the section above
shows from the code that answers it.

**`coverage` stops at the same step as the base.** This head's job answers `lcov: ERROR:
(inconsistent)` for `source/harness/hunt_topdown.cpp` at line 155, where a function is
reported hit with no line inside it hit, and the base job answers the same class at the same
step. `hunt_topdown.cpp` is not a file this lane touches, and issue
#76 records the stop.

**`sanitize` passed at this head and failed at the base.** The entry that failed at the base
is `harness_hunt_cli_test`, the runner-dependent one. This branch claims nothing from that
difference, and states it because a reader will see it in the check list.

## A third run, and the flip table it settles

Run 42039505695, head `6f664be6`, a records-only amend. Job ids: `test` 127286576625,
`tsan` 127286576973, `sanitize` 127286576825, `test-rocky` 127286576842, `coverage`
127286576756.

`test` passes. That is the job that carried this lane's abort, and it is green.
`test-rocky` answers `prose_gate_fixtures` and `harness_hunt_topdown_test`, and this time
`harness_hunt_cli_test` passes there. `tsan` and `sanitize` each answer
`harness_hunt_cli_test`. `coverage` answers `harness_hunt_cli_test` in its suite, then stops
at the upload step with `No files were found with the provided path:
build/coverage/coverage.info`, the symptom issue #76 records.

Where `harness_hunt_cli_test` stands, across three runs and five jobs:

| Job | Base `858c55d9` | `76d84fe8` | `6f664be6` |
| --- | --- | --- | --- |
| `test` | pass | fail | pass |
| `test-rocky` | fail | fail | pass |
| `sanitize` | fail | pass | fail |
| `tsan` | pass | fail | fail |
| `coverage` | pass | fail | fail |

Fifteen readings of one entry, two heads of code that differ in nothing touching it, and the
entry appears in two of five jobs, then four of five, then three of five, quoting the same
check text every time. `harness_topdown_run_test` reports `Skipped` in every job of every
one of those runs. The entry follows the runner's CPU identity, and the check that reads it
stands ahead of its own host guard, so a runner that answers nothing fails it on master as
readily as here.

What this lane owns, read across the three runs: `harness_limit_disclosure_test` appears in
no job of the two heads that carry its fix, and in the `test` job of the head that did not.

## The one-run `example_run` entry

Run 42045142257, head `98ac932e`, a records-only amend, job `test` 127306248846. The job
answered one entry:

```text
FAIL: docs_labels registered bmScanBlocks/cells:256 but printed no row
175 - example_run (Failed)
```

The route of that check is `test/example_run.sh`: it asks each example binary for its
registered instances with `--list`, runs it with `--min-time=0.01`, and requires a line
beginning with each instance name. The entry exited 0, so nothing failed inside the run, and
the missing line is the first row after the version line.

This branch cannot put a line into that output. The disclosure print sits in the gathering
loop that resolves a run's metric columns, `source/harness/runner.cpp:1582-1612`, and
`example/docs/labels.cpp` registers `bmScanBlocks` with no counter metric, so the loop never
runs for that binary and the line is unreachable there on any host. A scratch tree built with
the print forced for every gathering, then reverted, ran `example_run` at 100 percent of 1
and printed no lane line inside `docs_labels`, which is what the same code path predicts.

Four readings of `example_run` over one unchanged implementation: pass at the base, pass at
`4100bfd8`, `76d84fe8` and `6f664be6`, fail once at `98ac932e`. The entry is recorded here as
a fact about that runner, with the command that re-reads it, and the lane claims nothing from
it.

## Where this record stops

Run 42051319650, head `bbf0f6f0`, the head these records ship beside. `lint`, `prose-lint`,
`dbc-gate`, `shared-audit`, `downstream-consumer`, `consumer-release`, the three
`static-analysis` rows, `test`, `tsan` and `sanitize` pass. `test-rocky` answers
`prose_gate_fixtures` and `harness_hunt_topdown_test`, job 127328328464, and passes
`harness_hunt_cli_test` there. `coverage` answers `harness_hunt_cli_test` in its suite, job
127328328686, and stops at the trace step. Job ids `test` 127328328269, `tsan` 127328328498, `sanitize` 127328328744.

Five readings of `harness_hunt_cli_test` now, one of them at the base: two of five jobs,
four, three, one. The entry tracks the runner, and `harness_limit_disclosure_test` appears in
no job of any head that carries its fix.

This record stops at the run it names, on purpose. Reads of later runs of this branch, taken
with the same route, live in the pull request body, which moves without a push, so a fresh CI
sample never leaves a committed record stating a stale head.
