# Phase 11 The Coverage Lane Keeps One Job

**Surface**: the `Test` step of the `coverage` job in
`.github/workflows/ci.yml` and the `ctest` line it runs, plus the commit that
changed it and the commit that takes it back (T055, D-8, FR-044, P-11).
**Head sha**: `bd10d76` and `fea69c5` for the master readings, `e77b713`,
`5ccd69a` and `80de3f9` for the branch readings; the revert lands on the head
that follows `80de3f9`.
**Run id**: 39708015373 and 40042928839 for master, 40242340926, 40252253175 and
40255220576 for the branch.
**Command**: `gh api
repos/charlesarcher/speedgun-ng/actions/jobs/<job>` for the `Test` step's
`started_at` and `completed_at`, and `gh run view --log --job=<job>` for the
`ctest` line each step ran.
**Numbers**: 326 s and 334 s at `-j 2` on master, 229 s and 228 s at `-j 1` on
the branch, 111 s at `-j "$(nproc)"`.

## The rule

T055 offers two shapes. Keep `ctest -j 1` on the instrumented suite, or run it
over the whole runner with a `RESOURCE_LOCK` over the exec-child entries T008
names. D-8 sets the trigger for the second shape: the lane starts at one job and
moves if serializing costs more than the 334 s the step held. The trigger is a
number, so the decision is arithmetic.

## The readings

| Run | Head | `Test` step | The `ctest` line |
| --- | --- | --- | --- |
| 39708015373 | `bd10d76` | 326 s | `ctest -j 2 --output-on-failure --no-tests=error` |
| 40042928839 | `fea69c5` | 334 s | `ctest -j 2 --output-on-failure --no-tests=error` |
| 40242340926 | `e77b713` | 229 s | `ctest -j 1 --output-on-failure --no-tests=error` |
| 40252253175 | `5ccd69a` | 228 s | `ctest -j 1 --output-on-failure --no-tests=error` |
| 40255220576 | `80de3f9` | 111 s | `ctest -j "$(nproc)" --output-on-failure --no-tests=error` |

## Decision

Serialization at one job costs 229 s and 228 s in the two runs that held it. D-8
names 334 s as the price that would force the move, so the trigger does not fire
and `20e48a9 CI: Run the coverage suite over the whole runner` goes back.

The step fell from 334 s to 228 s with no change of parallelism at all. The entry
set, the budgets and the labels are what moved it, which is the argument of the
whole feature: the suite got cheaper because it stopped being run twice inside
itself.

Run 40255220576 is the counterfactual of the rejected shape: 111 s, a further
117 s off a job that held 1010 s. That is what the move would have bought, and it
is recorded so the figure is not lost. Three readings say against taking it at
this price.

D-8 does not offer the move at this price, which is the arithmetic above.

The locks are not the obstacle either way, so nobody should read the second
shape as blocked. `results/ph2/collisions.md` asks for exactly one new lock,
`harness_pass_report_out` over `harness_pass_report_test` and
`harness_pass_report_test_identity`, applied at `test/CMakeLists.txt:1360`;
master's `counters_trap_checked_out` pair stays at `:370` and `:388`. Every other
shared name in the audit carries the process id or comes from `mktemp -d`. A move
would have been two lines.

What is unproven is the trace. The instrumented children write profile counters
into one `.gcda` per object, and the phase has no reading of a parallel
instrumented lane whose trace was compared against a serialized one. Master ran
the lane at `-j 2` for as long as the record goes back and its gate passed, so
parallelism is not shown to corrupt a trace, and it is not shown to leave one
alone. SC-003 is the criterion that would notice a difference, and
`results/ph11/trace-equality.md` is the reading that settles it. 117 s of a
36-minute job is not worth moving the artifact SC-003 is measured on while that
question stands open.

## At the head the trace step aborts

The lane's suite is green at the final head and its next step is not. Run
40259383390, job 121368562541: `Test` ended `success` with
`100% tests passed out of 134` at `-j 1` in 219 s, and `Process coverage info`
ended `failure` with

```text
lcov: ERROR: (inconsistent) ".../source/harness/hunt_plan.cpp":365: function
'_ZZN2sg4hunt10rankEventsE..._clESF_' is hit but no contained lines are hit.
(use "lcov --ignore-errors inconsistent ..." to bypass this error)
```

The two commands of `cmake/coverage.cmake` are not equal in what they tolerate.
`COVERAGE_TRACE_COMMAND` at `:57` to `:69` captures with
`--ignore-errors mismatch,negative,inconsistent`; `COVERAGE_EXTRACT_COMMAND` at
`:80` to `:91` of that head filtered, omitted and extracted with no
`--ignore-errors` at all, so the same class that the capture forgives stopped
the extract. That command carried
`--omit-lines "SG_(REQUIRE|ENSURE|INVARIANT|ASSERT)(_ALWAYS)?"` at `:87`,
and `source/harness/hunt_plan.cpp:363` to `:368` is one `SG_ENSURE` invocation
whose arguments hold a lambda:

```cpp
SG_ENSURE(result.method == method
              && std::ranges::all_of(
                  result.entries,
                  [multiplexed](const RankedEvent& entry)
                  { return entry.multiplexed == multiplexed; }),
```

The closure at `:366` to `:367` stands inside the statement that the omission
rule matches, so its only line leaves the trace while its function record stays
with a non-zero count, and lcov 2.3 reads that pair as `inconsistent`. The
command text, the file, the line and the mangled closure name are what the job
log prints; the omission rule and the source lines are read from the tree. The
lambda runs whenever `result.entries` holds at least one record,
`std::ranges::all_of` over an empty range calls nothing, so the abort needs an
entry that ranks a non-empty table, and several entries in every lane do that.

FR-041 (`spec.md:776`) forbids adding an ignore class to the extract, summary or
`genhtml` command, and `cmake/coverage.cmake` stands in no node's file list, so
neither half of the fix is inside this branch's scope. `results/ph11/tables.md`
quotes the same error beside the step timings, and the residual row of `b0d0f9b`
at `spec.md:255` reads "a run that shows a stage abort on that class reopens the
point", which is what this run is.

## Which entries reach that closure

Excluding one entry from the lane was offered as the fix that stays inside FR-041.
It is measured here, and there is no single entry to exclude. A throwaway
instrumented tree of this head (`ci-coverage` preset at
`/home/archerc/sg030-t57/attrib`) was rebuilt with only the hunt targets, each
entry was run alone after `find -name '*.gcda' -delete`, and the per-function
counts came from `gcov --json-format -j hunt_plan.cpp.gcda`, which reports
gcov's own function records: the maximum `execution_count` among the functions of
`hunt_plan.cpp` whose `start_line` is 365.

| Entry run alone | `ctest` | Executes at line 365 |
| --- | --- | --- |
| `harness_hunt_rank_test` | 0 | 50 |
| `harness_hunt_plan_test` | 0 | 1 |
| `harness_hunt_collect_test` | 0 | 77 |
| `harness_hunt_report_test` | 0 | 223 |
| `harness_hunt_json_test` | 0 | 30 |
| `harness_hunt_cli_test` | 8 | 653 |
| `harness_pass_collect_test` | 0 | 0 |
| `harness_hunt_json_test_no_hunt` | 0 | 0 |

Six of the lane's 134 entries reach that closure on their own, and the suite
record in CI shows the same closures at counts up to 321169, so the exclusion that
would clear the abort is the whole hunt group. Dropping it would take
`source/harness/hunt.cpp`, `source/harness/hunt_plan.cpp` and
`source/harness/detail/hunt_selection.hpp` out of the trace the lane produces,
which is the file list SC-003 compares and the denominator the gate divides by.
The exclusion that clears the abort has to widen past what FR-047 means by
keeping every entry.

The last two rows are controls, and both print nothing at that line: the pass
entry shares no code with the hunt tree, and the second is the json entry with
`--no-hunt`, which never asks for a plan. The cli row is a real reach on the way to
something else: that entry exits 8 in the instrumented tree, having executed the
closure 653 times before it fails, so its counts land in the gcda regardless of how
its own assertions settle. The sample target named in the build carries no test in
that tree, so it contributes no reading.

`--rc omit_lines_groups=0` is not available either: `grep -rn omit_lines` over the
lcov 2.3 tree finds the `omit_lines` regex and its examples and no grouping switch,
so the span an `SG_*` match removes is not configurable.

## The cause, and the fix that lands

The closure lcov refused is the rank post-condition of `rankEvents`, and
the record lcov reads is the lambda the `SG_ENSURE` argument carries:
the macro expands its predicate three times, so GCC builds three
specializations of one lambda, and gcov attributes every expansion to
the line of the macro call. The tracefile shows the shape
(`FNL:7,365,365` for the closure, its body on line 368, and no `DA`
record for line 368; `FNA:7` hit counts of `0`, `0` and `3718` over the
three `...RankedEventEE0_`, `E1_` and `E2_` types). The omit-lines rule
drops line 365, so the closures stand hit with no contained line, and
lcov stops the extract on "function is hit but no contained lines are
hit". The fix moves the predicate into a named helper of the anonymous
namespace of `source/harness/hunt_plan.cpp`, `everyEntryMarks`, with the
lambda on lines of its own, and the contract names the helper beside
the method compare: the contract stands the same, and it costs nothing
when contracts are off. The lambda inside a macro argument that expands
it three times is a defect of its own, and this is not code shaped for
coverage. It is the only lambda inside an `SG_*` contract macro in
`source/` and `include/`. With the helper in place the extract, list,
summary and genhtml steps run without an inconsistent record for
`hunt_plan.cpp`, and no ignore class lands anywhere: the capture
command keeps the classes master has, the extract command carries none,
and the summary command and `genhtml` hold none. No entry is excluded
and no file leaves the trace, so the lane can reach the gate with all
54 source files in its artifact, which is the shape SC-003 reads. What
the gate prints there is recorded in `results/ph11/gates.md` when the
run that carries the fix closes.


## No base reading exists in CI

SC-003 compares the branch trace against the base tree's, and the base commit has
no CI trace to compare against. `f507b8f` is `origin/master` and its own push run
40203022166 ended `failure` on the `lint` job, with every downstream job
`skipped`, so no `coverage.info` was ever produced at the base commit. The only
coverage trace CI holds is the artifact of 40042928839, whose head is `fea69c5`,
five pushes before the base; it carries 48 source files and no record of the hunt
tree at all, because the hunt tree landed after it. It is therefore not the base
side of any comparison, and reading it as one would be a mistake.

The base side comes from a base-tree build of `f507b8f` run with the lane's own
command, and `results/ph11/trace-equality.md` carries that reading.

