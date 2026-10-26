# Pull Request Text

This is the text of pull request #51 (`030-ci-speedup` to `master`), kept here as
the record of what the request claims (T059, FR-053, FR-056, FR-060 to FR-064,
SC-012). Every reading in it names its run and head sha in
`results/ph11/tables.md` and the other phase records.

## What this lands

CI at the base commit `f507b8f` takes 41 minutes to a verdict on a push and
cannot be checked locally in any bounded time. This branch keeps every job, every
entry and every gate, and moves the work: the analyzer runs in three shards
instead of inside the `test` build, every lane builds and tests at the runner's
core count, `ccache` fronts each lane's compile, the apt and dnf package lists are
cached, the semantics matrix registers one entry per cell on the three lanes that
need it, every entry carries a tier label and a time budget, `docs` waits for the
analyzers, and the older run of a pull request is canceled when a newer push
lands. Locally, `tools/ci/prepush.sh` mirrors the CI gates in thirteen timed steps
and `--full` adds the slow tier, the consumer shapes and the sanitizers.

No file under `include/` or `source/` changes. `VERSION 0.20.0`, `SOVERSION 4` and
`SOVERSION 12` are master's (`CMakeLists.txt:15`, `:63`, `:281`, byte-equal to
`git show origin/master:CMakeLists.txt`), and the diff adds no command-line
option, no environment variable and no schema key: the diff touches no file under
`include/` or `source/` at all, which is where the option tables and the schema
keys live, and it adds no `getenv` line. `SPEEDGUN_HOST_PROCESSORS` is a CMake
variable from `cmake_host_system_information` (`test/CMakeLists.txt:27`) and
`SPEEDGUN_SLOW_TESTS` is a CMake list (`:1623`), neither read from the process
environment.

## The two tables

Before and after walls, and the per-job `Build` and `Test` step table, are in
`results/ph11/tables.md` with the `gh` command that read each number, the run id
and the head sha beside it. The headline: master's `test` job spends 2277 s of its
2621 s wall compiling 227 translation units and running both analyzers over them;
this branch's `test` job builds in 216 s and tests in 217 s, and the analysis
stands in three shards of 122, 213 and 154 units beside it. The six-run median
SC-002 asks for is 2208.5 s, which misses its 15-minute bound for the reason the
record gives; the median job wall is 449 s at the head against master's 215 s.

At `88fc715` twelve of fifteen jobs are green: `lint`, `dbc-gate`, `prose-lint`,
`shared-audit`, `consumer-release`, `downstream-consumer`, `test`, `test-rocky`,
`sanitize` and the three analyzer shards. `docs` ends `skipped` on every
pull-request run through a condition master wrote and this branch left alone
(`.github/workflows/ci.yml:1416`, byte-identical to `:774` of master's file). Two
jobs are red and both are named below; a rerun of those two lanes at the same
commit brought `tsan` back green and left `coverage` red.

## `ccache` on this host and in CI

Local bucket `/home/archerc/.cache/ccache`, 5.0 GiB cap, 99 MiB used, ccache
4.11.3; `ccache -s` after the recorded runs reads 187 hits of 1244 cacheable
calls (15.03%), counters since the last `ccache -z` (`results/ph4/timing.md`). In
CI each building lane zeroes and prints its own counters around its `Build` step
(`results/ph3/cache-stats.md`); the cold run 40242954391 reports `Hits: 0 / n
(0.00%)` in all thirteen lanes and the warm run of the same commit reads `Hits: 43
/ 125 (34.40%)` (`results/ph3/cold.md`, `results/ph3/warm.md`).

One finding belongs beside those numbers: every `Save` step in `ci.yml` carries
`if: github.event_name == 'push'` (eleven of eleven) and `on.push.branches` names
only `master`, so a `pull_request` run restores and never writes, and its
`Save ccache` step reads `completed skipped` even in a green job
(`results/ph9/install-steps.md`). The cache the branch warms is master's.

## The test list difference, one reason per line

`results/ph6/list-diff.md` reads `ctest -N` per lane on master and on the branch.
`coverage` 134 to 134, `sanitize` 134 to 134, `tsan` 130 to 135, `test` 134 to
139, `test-rocky` 134 to 139. Every added line is one of the five `dbc_matrix_*`
entries T033 registers on the three lanes that turn the matrix on, and no line
disappears; `tsan` starts from 130 because master's `ci-tsan` tree already skipped
two entries. Nothing else moves.

## The finding lists

`results/ph5/finding-diff.md`: the branch's three shards print the same sorted
finding list as a throwaway branch that holds the base tree `f507b8f` and runs the
branch's analyzer job, and the planted `readability-identifier-naming` violation of
`results/ph5/planted.md` fails exactly the shard that builds its unit, on a warm
cache (FR-017: CMake analyzes before the compile on a cache hit).

## The twelve reverted commits

| Commit | Verdict | Route inside the approved scope |
| --- | --- | --- |
| `00e5848` Serialize the coverage capture | need: yes | FR-044 keeps the coverage lane at a parallelism the counter merge tolerates (`results/ph11/coverage-lane.md` settles it at `-j 1`) |
| `5fe94fb` Fold the children's profile trees back in, `872086c` Relocate the children's profile counters | need: yes, as a pair | FR-044 and FR-049 reach the same trace inside `ci.yml`, with a CI run as the evidence and no new ignore class |
| `a7e7ce5` g++-14 pin, `c5d7a5f` g++-16 pin, `ea75686` toolchain PPA source, `e9a3907` match gcov to the compiler | need: no | FR-042 keeps pins, toolchain sources and gcov-matching out; the lane uses the runner's default compiler |
| `45d5e42` ignore the class in genhtml, `f82406e` when listing, `b0d0f9b` give lcov its error class early | need: observed at the head | FR-041 adds no ignore class to extract, summary or `genhtml`; the head showed a stage aborting on that class, the event the residual table names as reopening the point, and the named-helper fix of `source/harness/hunt_plan.cpp` closes it |
| `dcbdfa6` Separate an unreadable trace from a gap | need: probably | FR-043 splits the two exits in `tools/dbc/coverage_gate.sh` (commit `5d496c9`, exit 3 when lcov cannot read the trace) |
| `3f3b56c` Raise the instrumented test timeout | need: yes for the need, no for the shape | FR-031 to FR-034 set a per-test budget with named exceptions and measured CI numbers, and keep `--timeout` and raised `timeout-minutes` out |

`spec.md:225` holds the residual-need list this table answers to, row by row.

## The reds at the head, named

`tsan` failed one entry, `harness_context_barrier_test`, on the assertion that a
participant's voluntary-context-switch counter moves inside the barrier. It failed
on the YIELD participant at `88fc715` (job 121368562553, 1.07 s, `99% tests
passed, 1 tests failed out of 135`) and on the SPIN participant at `5ccd69a` (job
121346611402). The rerun of that lane at `88fc715` passed the entry in 0.45 s with
`100% tests passed out of 135` (attempt 2, job 121374177607). Two failures on two
different participants and one clean pass at one head make the entry
intermittent. Master's `tsan` lane passed it in 0.74 s at `-j 2` (job
120718088244). The entry declares `PROCESSORS 4` and CTest runs a `PROCESSORS 4`
entry alone at `-j 2` and at `-j 4`, measured (12.04 s, 9.04 s and 6.03 s for one
heavy and three light 3 s entries); the attempt 2 log holds four entries in flight
at its peak, which is what `-j "$(nproc)"` resolves to on the runner, so the entry
fills the slot set and stands alone on master and on the branch alike. The test
source is untouched by this branch, and the lane is green at the head.

`coverage` is red in its `Process coverage info` step, not in its suite: `Test`
ended `success` with `100% tests passed out of 134` at `-j 1`. lcov stopped the
extract command:

```text
lcov: ERROR: (inconsistent) ".../source/harness/hunt_plan.cpp":365: function
'_ZZN2sg4hunt10rankEventsE..._clESF_' is hit but no contained lines are hit.
(use "lcov --ignore-errors inconsistent ..." to bypass this error)
```

`cmake/coverage.cmake:61` gives the capture command
`--ignore-errors mismatch,negative,inconsistent`; at `88fc715` the extract
command carried no ignore class, which FR-041 then required. The cause of
the abort is the lambda inside the closure's `SG_ENSURE` argument at
`source/harness/hunt_plan.cpp:365`: the macro expands its predicate three
times, GCC builds three specializations of one lambda, and gcov attributes
every expansion to the macro line alone, so the omit-lines rule leaves the
closures hit with no contained line and lcov stops the extract. The fix
moves the predicate into the named helper `everyEntryMarks` of
`source/harness/hunt_plan.cpp` and lands in no ignore class: the extract,
summary and `genhtml` commands carry none, no entry is excluded, and no
file leaves the trace; `results/ph11/coverage-lane.md` carries the trace
reading and the six-entry measurement behind it. The rerun of the
lane at the same commit failed the same record in the same step (attempt 2, job
121374177396, `Test` green at `100% tests passed out of 134`), so the abort is
deterministic. The lane's own `Retain the coverage trace` step ran under
`if: always()` and uploaded the trace lcov had written before it stopped: 54
source files, 1387 function records, and inside `source/harness/hunt_plan.cpp` no
`DA` record between lines 360 and 370 while the closure records under `rankEvents`
carry counts up to 321169. The owner has since ruled the cause fixed
rather than tolerated: the helper closes the abort, and the branch adds
no ignore class anywhere on the extract, the summary or `genhtml`.

## Deviations this PR states

1. SC-007's five-minute bound for `tools/ci/prepush.sh` is missed on the lane
   host: 740 s of printed steps against 300 s. The breach has a floor: step 9
   pins the quick tier to two cpus and that tier holds 647.40
   `sec*proc`, so 323.7 s is its minimum before any other step runs
   (`results/ph4/timing.md`, `results/ph7/tiers.md`).
2. SC-002's median of six runs at 15 minutes or less is unreachable as written:
   three of the six are master pushes at 1732, 2466 and 2697 s, so the median
   cannot fall under 1287 s whatever the branch does. The measured median is
   2208.5 s. The install-step times SC-002 also asks for are recorded per run, the
   cold run is timed, and the PR #47 audit point is timed with its slowest job
   named (`results/ph11/tables.md`, `results/ph9/install-steps.md`).
3. FR-047 says an entry keeps the `SKIP_RETURN_CODE` rule master sets for it, and
   master sets none for `harness_jitter_cli_test`. The round added a skip for
   the reason that follows, and this review round removes it again: the idle
   scenario takes its cpus from `sched_getaffinity` now, the layout it
   installs names the highest cpu of the mask for the probe to start on, and
   the contract abort at `source/harness/execution_context.cpp:659` is gone
   from the four-cpu runner and from a `taskset -c 0,1` run beside it
   (`results/ph11/jitter-skip.md` holds the CI readings behind the original
   finding). No skip rule stands on the entry, and CI skips no test.
4. The coverage gate demands 100 percent lines and branches and the base tree
   reads 99.4 and 97.5 (`spec.md:218`), a shortfall spec 032 owns (FR-040,
   FR-064). At this head the lane does not reach that gate at all, for the lcov
   reason named above, so SC-003's CI-side reading waits on that decision. The
   trace equality SC-003 states is proved locally against the base tree in
   `results/ph11/trace-equality.md`.

## Green on this host

Host `CorsairOne`, Linux 7.3.0-4-cachyos, Intel i9-9920X at 3.50 GHz, 24 cores,
gcc 16.2.1 20260810, clang 23.1.3, clang-format 18.1.8, cmake 4.4.2, ccache
4.11.3. Every gate the plan's Implementation Strategy names is run against this
head and recorded with its exit status and its seconds in
`results/ph11/gates.md`, with the lanes that CI measures instead of this host
named there. `harness_cli_test` exceeds its 300 s budget on a host that grants the
cpu PMU (3110.37 s measured at the base commit, `results/ph2/cli-slow.md`), so a
local full-suite run reports that one entry as a timeout in each lane; CI's
runners withhold the counters and the entry passes there.

## Tracking

No issue tracks this feature.

Refs: specs/030-ci-speedup
Approved-by: OmO
