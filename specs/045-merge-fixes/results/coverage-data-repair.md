# Coverage-data repair: the measurement record

Spec: [../spec.md](../spec.md), section "Phase 13: Repair the
coverage-data fault" (owner ruling G-25, 2026-11-07). Tasks: COV-001 to
COV-007 in [../tasks.md](../tasks.md). Every command ran on thrawn, the
acceptance host of ruling G-20.

## Host and tools

| Item | Value |
| --- | --- |
| Host | thrawn, kernel `7.3.0-5-cachyos`, x86-64 |
| CPUs | 48 online (`0-47`), `sched_getaffinity` returns all 48 |
| GCC, gcov | 16.2.1 20260810 |
| Clang | 23.1.3 |
| lcov, genhtml | 2.6-0 |
| cmake, ctest | 4.4.4 |
| ccache | 4.14.1 |
| Base head | `615c0964547e4e9768b8e2f2d79991033a0eb5bd` |

The host `~/.lcovrc` sets `check_data_consistency = 0`. Every read in
this record passes `--config-file /dev/null`, so the lcov
data-consistency check stays live and the host file stands out of
reach. The figure the gate reads does not change: the host file
changes no line, branch or marker count.

## COV-001: the fault at the base, reproduced from a fresh tree

The tree went first, then the three steps of the lane, each read twice:
once with the class tolerated, once with it removed.

```sh
rm -rf build/coverage
cmake --preset=ci-coverage -DCMAKE_C_COMPILER_LAUNCHER= -DCMAKE_CXX_COMPILER_LAUNCHER=
cmake --build build/coverage -j 24
ctest --test-dir build/coverage --output-on-failure --no-tests=error -j 24
```

| Step | Exit | Reading |
| --- | --- | --- |
| configure | 0 | `ci-coverage`, launchers empty |
| build | 0 | 185 targets, 15 min cold |
| `ctest -j 24` | 8 | 185 configured, 183 passed, 1 timed out, 1 skipped |

The failed entry is `harness_jitter_cli_test`: `***Timeout 300.11 sec`,
against the 300 s `ctest` ceiling. The entry that did not run is
`harness_topdown_run_test`, marked `***Skipped` by its own guard and
listed under `The following tests did not run`. The suite wall time read
`464.99 sec total`.

The first build of the lane carried the `sccache` launcher of the
preset and stopped at the capture with
`test/CMakeFiles/harness_jitter_cli_test.dir/source/harness/jitter_cli_test.cpp.gcda:stamp mismatch with notes file`
and `Message summary: 1 error message: gcov: 1`. That is stale build
data: the launcher served an object from an older instrumented tree, so
its `.gcda` no longer matched the `.gcno` notes. The row above is the
rerun with the launchers empty, which is the clean reading.

### The capture, both ways

With the class tolerated, the capture exits 0 and prints seven records
of one shape:

```
lcov: WARNING: (inconsistent) mismatched end line for <name> at
$HOME/code/speedgun-ng/test/source/harness_callback_test.cpp:278: 278 -> 285
while capturing from ...
```

The seven records stand in `test/source/harness_callback_test.cpp`,
`test/source/harness_fixture_test.cpp` and `example/docs/fixtures.cpp`.
Each names a function a fixture macro generates, and each line is a
`SG_BENCHMARK_F` or `SG_BENCHMARK_FRIEND_F` invocation.

With the class removed the same capture ends at the first of those
records:

```
lcov: ERROR: (inconsistent) mismatched end line for
_ZN12_GLOBAL__N_135SgBenchmarkFixture_OrderFixture_run15sgBenchmarkCaseERN2sg5StateE
at .../test/source/harness_callback_test.cpp:278: 278 -> 285 while capturing from
build/coverage/test/CMakeFiles/harness_callback_test.dir/source/harness_callback_test.cpp.gcda
CAPTURE_EXIT=1
```

No trace is written. FR-017 keeps the class on this read and O-4 carries
the owner question.

### The extract, the list and the summary, both ways

The extract step of the base carried `--ignore-errors inconsistent`.
With that flag gone the extract still exits 0 on lcov 2.6 and writes
`coverage.info`; the complaint moves to the reads of the written trace.

```
lcov: ERROR: (corrupt) unable to read trace file 'build/coverage/coverage.info':
lcov: ERROR: (inconsistent) ".../source/harness/hunt_topdown.cpp":155: function
'_ZZN2sg6detail18topdownDeepestNodeE...NKUlRKNS1_11TopdownNodeEE_clESB_'
is hit but no contained lines are hit.
	To skip consistency checks, see the 'check_data_consistency' section in man lcovrc(5).
LIST_EXIT=1  SUMMARY_EXIT=1
```

The continuous integration lanes of pull requests #86 and #87 stopped at
the same shape one line earlier in the trace, naming
`"source/harness/runner.cpp":708`.

### The three records that carry the fault

The trace of the base, read with the jitter entry completed:

| File | `FNL` | `FNA` counts at that range | `DA` at that line |
| --- | --- | --- | --- |
| `source/harness/hunt_topdown.cpp` | `7,155,155` | 0, 124, 0 | absent |
| `source/harness/runner.cpp` | `16,708,708` | 0, 172, 0 | absent |
| `source/harness/runner.cpp` | `18,821,821` | 0, 140, 0, 0, 0, 138, 0, 0, 0, 138, 0, 0, 2, 0 | absent |

Each range is one line wide, and that line is the line of an `SG_*`
contract macro: the `SG_ASSERT` of `topdownDeepestNode`, the first
`SG_REQUIRE` of `stripTopdownColumns`, the second `SG_REQUIRE` of the
same function. gcov records a closure written inside such a predicate at
the macro invocation line. The extract step's
`--omit-lines "SG_(REQUIRE|ENSURE|INVARIANT|ASSERT)(_ALWAYS)?"` pattern
removes that line's `DA` record. The written trace then holds a function
with a positive hit count and no hit line inside its own range, and the
lcov consistency check refuses it. The zero counts are the closures of
the same conjunction whose predicate the short-circuit never reached.

### The sweep over every hit function record

A balanced-parenthesis sweep of the 572 `SG_*` invocations in `source`,
`include`, `test`, `example` and `tools` finds 503 that span more than
one line and three that hold a closure. No single-line invocation holds
a closure, and no wrapper macro builds one.

A sweep of the 1977 hit function records in the extract set finds six
emptied by the omission and no others: the five closures at
`runner.cpp:708` and `runner.cpp:821` and the one at
`hunt_topdown.cpp:155`. The eight hit functions inside the
`dbc.hpp` `LCOV_EXCL` blocks keep no record at all, because `geninfo`
applies the marker at capture and drops the function record with the
line. The `if (cpus.empty()) {` pattern matches four lines
(`topology.cpp:395`, `topology.cpp:591`, `compare.cpp:1758`,
`host_record.cpp:454`), each inside a larger function that keeps its
own hit lines, so no record empties. Nothing in the raw trace is
inconsistent before the omission.

`SG_*` expands its predicate three times, twice inside
`SG_CT_REJECT`, so one closure written in a predicate yields three
closure types and one executed body. That is why the range at
`runner.cpp:821` carries fourteen function records and five of them
hold a positive count.

### The branch arms the repair moves into the measured region

A closure inside a contract macro contributes no branch row the gate
can read: gcov 16 collapses the whole macro statement onto the
invocation line, and the omission removes that line. Moved into a named
helper, the same comparisons become readable branch rows. The audit
counts seven that stand dark unless a test drives them: three from the
`ResultRow` closure at `runner.cpp:821`, one from the `RowRaw` closure,
two from the nested context-row closure, and one from the
`hunt_topdown.cpp:155` closure. The `runner.cpp:708` closure adds no arm
of its own. COV-003 therefore carries the rule that every arm of every
new helper stands driven by a test, and the head measurement of COV-006
proves it.

### The three causes, classified

1. **Stale build data.** The `sccache` launcher served an object whose
   `.gcda` did not match its `.gcno`, and the capture stopped with a
   `gcov` class error. The command that clears it is an instrumented
   configure with the compiler launchers empty, on a tree with no
   `.gcda` file. This cause is a build-habit fault, and the lane now
   states it.
2. **Instrumentation shape.** Seven end-line records at fixture-macro
   lines in `test/` and `example/`. The repair reaches the public macro
   family in `include/speedgun-ng/benchmark.hpp`, which the phase puts
   out of scope. The capture keeps the class for this shape, and O-4
   carries the question.
3. **Source shape.** The three contract closures above. This is the
   cause the repair answers, and COV-003 answers it.

### The figures at the base

The gate read at the base, with the jitter entry cut by the `ctest`
ceiling:

```
lines.......: 99.8% (18595 of 18628 lines)
functions...: 98.4% (1931 of 1963 functions)
branches....: 99.4% (9194 of 9249 branches)
coverage-gate: line coverage is not 100%
```

The 33 dark lines are 32 in `source/harness/jitter_probe.cpp` and one
at `source/harness/options.cpp:1874`, and all 33 sit in the code the
timed jitter entry never reached. Run alone in the same instrumented
tree, `./build/coverage/test/harness_jitter_cli_test` printed
`harness_jitter_cli_test: ok` in 392 s, past the 300 s ceiling. With
that entry's counters in the trace, the same reads give:

```
lines.......: 100.0% (18627 of 18627 lines)
branches....: 99.7% (9223 of 9249 branches)
```

The branch shortfall reads 26 rows against the allowance of 21. The
ledger of COV-004 lists those rows.

## Artifact analysis before implementation

Two readers ran the cross-artifact check over `spec.md`, `plan.md` and
`tasks.md` on 2026-11-07: one for traceability, identifier hygiene,
contradiction, verifiability, scope drift and criterion measurability,
one for constitution compliance. The first returned 20 findings, 4
blocking, 11 minor, 5 notes, in
`~/scratch/coverage-data-repair/analyze-consistency.md`. The four
blocking findings and every minor one stand remediated in the
artifacts before any implementation row closed:

| Finding | The contradiction | The edit |
| --- | --- | --- |
| F-03 | SC-008 and the Scope line freeze `cmake/` and `tools/dbc/`, and phase 13 edits two files there | both lines now bind to phases 0 to 12, and phase 13 names the two files |
| F-04 | D-12 removed the class from four reads while FR-017 and O-4 keep the capture | D-12 names the three reads of the written trace |
| F-05 | FR-019 demanded a route with no class anywhere, which the kept capture makes impossible | FR-019 names the extract, list, summary and `genhtml` reads, and the capture's records stand in the record |
| F-06 | the commit rules forbid a mixed commit, and phase 13 lands one | the rule binds parts A and B, and phase 13 states its own single-commit form |

The minor edits added the four-cpu lane to SC-011, the cold-row files
to the in-scope list, `quickstart.md` and the cold-row files to the
plan's file table, the summed form of the marker count, the gate's real
reading at `tools/dbc/coverage_gate.sh:109` and `:130-136`, the six
closure sites in place of a bare "three", the three closure predicates
in the edge-case list, the whole-tree rule for FR-016, and the settled
helper shape in D-13.

A third reader had already swept the base trace and the `SG_*` sites;
its findings stand in the COV-001 section above. A fourth reader tried
to break the plan and produced the counter-evidence that moved D-13 and
O-4: the capture's end-line emission stands outside the
data-consistency check, a per-translation-unit capture invents
`(inconsistent)` records the full route never carries, and a helper
that carries a contract of its own ends the process under the coverage
tree's `enforce` mode.

## COV-002: the audit of the pull request #89 range

The two commits of that range, `48e89812` and `c87ede7a`, read hunk by
hunk. Every hunk is ignored-error plumbing or a comment. No hunk
changes a gate, a filter, a roots list, an allowance or a CI setting,
so the count of unrelated changes reads 0. The class entered the
extract step, the `--list` step and the `--summary` read there, and the
capture list carried it before that range.

The removal at this head:

| Read | Location | Classes carried at the head |
| --- | --- | --- |
| capture | `cmake/coverage.cmake:77` | `mismatch,negative,inconsistent` |
| extract | `cmake/coverage.cmake:116` | none |
| `--list` | `cmake/coverage.cmake:158` | none |
| `--summary` | `tools/dbc/coverage_gate.sh:32` | none |
| `genhtml` | `cmake/coverage.cmake:142` | none |

`grep -rn 'ignore-errors' cmake tools` names one line, the capture
list. The `--filter exception,branch,brace` set, the two `--omit-lines`
patterns, the extract roots, `allowance=21` and the marker set stand
unchanged.

## COV-003: the repair

Five helpers carry the six closures. Each returns its answer, carries
no contract of its own, and stands declared in the internal header that
declares its enclosing function.

| Helper | Header | Carries |
| --- | --- | --- |
| `metricPositionsNameColumns` | `source/harness/detail/internal.hpp` | the closure at `runner.cpp:711` |
| `rowsHoldOneEntryPerLabel` | same | the closure at `runner.cpp:823` |
| `rawColumnsHoldGridWidth` | same | the closure at `runner.cpp:834` |
| `contextColumnsHoldGridWidth` | same | the closures at `runner.cpp:841` and `:845` |
| `nodesNamingAddressStandAtOrBelow` | `source/harness/detail/hunt_topdown.hpp` | the closure at `hunt_topdown.cpp:159` |

The `SG_REQUIRE` and `SG_ASSERT` macros, their message strings and
their evaluation order stand at the three call sites, byte-identical.
The predicate text before and after each site stands quoted in the
commit body.

Five scenarios drive the arms: four in
`test/source/harness_topdown_report_test.cpp` and one in
`test/source/harness_hunt_report_test.cpp`. A gcov probe of the five
helper bodies under the project's test flags shows every branch record
in them taken at least once, and each closure inside a helper now
carries hit lines of its own.

## COV-001 and COV-003 to COV-005: the head reading

The tree went fresh again for this reading, with the host `~/.lcovrc`
moved out of the home directory for the whole lane, so the repository
commands ran unchanged and the lcov data-consistency check stayed live.
The host file carries `check_data_consistency = 0`, written on
2026-11-03 as a host workaround for the fault this phase repairs.
It stands moved to `~/scratch/coverage-data-repair/lcovrc.host-local`
and left there, because its presence hides the fault from every local
read. One command restores it:
`mv ~/scratch/coverage-data-repair/lcovrc.host-local ~/.lcovrc`.

| Step | Exit | Reading |
| --- | --- | --- |
| configure, launchers empty | 0 | `ci-coverage` |
| build | 0 | 185 targets |
| `ctest -j 24` | 0 | `100% tests passed out of 185`, `183 - harness_topdown_run_test (Skipped)`, `438.80 sec total` |
| capture | 0 | seven `(inconsistent)` end-line records, the tolerated class |
| extract | 0 | no complaint |
| `--list` | 0 | the table below |
| gate `--summary` | 1 | `coverage-gate: branch shortfall 24 rows exceeds the D-14 recorded allowance of 21` |
| `genhtml` | 0 | `no messages were reported` |

```
lines.......: 100.0% (18657 of 18657 lines)
functions...: 99.0% (1936 of 1956 functions)
branches....: 99.7% (9249 of 9273 branches)
```

The data fault is gone. No read of the written trace printed
`is hit but no contained lines are hit`, and the only
`(inconsistent)` output is the seven end-line records of the fixture
macros, which the capture list tolerates and O-4 reports.

The branch denominator grew from 9249 to 9273 because the repair moved
24 arms out of the collapsed macro spans and into readable records, and
the taken count grew from 9223 to 9249: the 24 new arms stand taken, and
the two cold rows the phase closed stand taken. The cold count moved
from 26 to 24.

`harness_jitter_cli_test` passed inside `ctest` at this head in
285.53 s against the 300 s ceiling. At the base the same entry crossed
the ceiling at 300.11 s. The margin is 14 s on this host, and the entry
stands named here for that reason.

## The cold-row ledger at the head

Twenty-four rows stand open against the allowance of 21. The gate
prints its own row list on the failure path; the count below is the
count of `BRDA` rows outside the exception class, which is the count the
gate's arithmetic uses.

### Class A, the unwind tail of a full expression: 16 rows

| File | Line | Arms | Shape |
| --- | --- | --- | --- |
| `source/harness/cli.cpp` | 280 | `29 -> 30`, `29 -> 31`, `32 -> 33`, `32 -> 34` | the temporary of the concatenation behind the ternary of the failure line; one landing pad each |
| `source/harness/runner.cpp` | 838 | `95 -> 96`, `95 -> 97`, `98 -> 99`, `98 -> 100` | `push_back` of one `TopdownNode` with three `std::string` members |
| `source/harness/host_record_check.cpp` | 266 | `21 -> 22`, `21 -> 23` | the class the D-14 ledger already carries at this line |
| `source/harness/options.cpp` | 1541 | `13 -> 14`, `13 -> 15` | the concatenation inside the refusal lambda of `applySourceValue` |
| `source/counters/linux_pmu/topdown_select.cpp` | 128 | `24 -> 25`, `24 -> 26` | two `std::string` constructions in the `push_back` of one `TableFile` |
| `source/harness/noise_tracer.cpp` | 486 | `18 -> 19`, `18 -> 20` | the cleanup landing pad of the expression; block 18 has two incoming edges alone, both marked throw |

The gcov block these rows sit on stands entered by an unwind alone. No
input reaches it, and forcing one needs an allocation failure, which is
not a legitimate input test.

### Class B, a guard the shape of the data closes: 8 rows

| File | Line | Arms | Why no input reaches the arm |
| --- | --- | --- | --- |
| `source/harness/latency_counters.cpp` | 183 | `4 -> 8` | `namesVendorText` stands called from one place, `SG_ENSURE(!answer || namesVendorText(text), ...)` at `:206`. A supported answer needs `*vendor == kSmiVendorId`, and `valueOfKey` takes the head and the value out of one line of the same text, so a text that answers supported names the vendor key beside the vendor word |
| `source/harness/latency_counters.cpp` | 184 | `2 -> 8`, `6 -> 7` | the same two clauses; measured over the run: `smiSourceSupported` called 42, the answer true 34, the helper called 34, the three false arms 0 |
| `source/harness/latency_counters.cpp` | 193 | `5 -> 6` | `rowNamesFigure` stands called from `SG_ENSURE` at `:261`. `figureAtColumn` takes one whitespace token of that row and `figureOfToken` answers a figure only when `from_chars` consumes the whole token, so the token is digits alone and `std::to_string` of its figure is that token without its leading zeros, a substring of the row. Measured: 71 calls, the find arm true 59, false 0 |
| `source/harness/report.cpp` | 815 | `81 -> 82` | `seriesRun` states that the first fold carries a series, and `worst` moves only to a fold that carries one, so the second operand has no path to its false arm |
| `source/harness/statistics.cpp` | 84 | `8 -> 11` | `order` stands sorted by the coordinate the tie scan reads, so `less(points[order[end]], value)` answers false on every element and the clause that would close the scan has no false arm |
| `source/counters/linux_pmu/topdown_select.cpp` | 492 | `3 -> 19` | one arm per kind of `TopdownUnsupport`, and gcov adds a sixth for a value outside the enumeration; the five kinds stand read |
| `source/harness/hunt_ranks.cpp` | 127 | `13 -> 23` | the arm needs `spearmanCorrelation` to answer a value while `leastSquaresSlope` answers none. Spearman needs a non-zero rank spread on the first coordinate; the slope needs `count * sum(x^2) - (sum x)^2` at zero, which means every first coordinate stands equal. The two conditions contradict each other |

Both helpers of the first three rows stand file-local, so no test
reaches them beside their two call sites. Moving them to an internal
header would make their arms countable, and FR-003 forbids moving code
to lose a counter, so the phase leaves them open with the evidence
above. FR-007 states this rule and FR-021 forbids a marker here.

### Rows this phase closed with real input: 2

| File | Line | Arm | Test |
| --- | --- | --- | --- |
| `source/harness/compare.cpp` | 602 | `49 -> 50` | `harness_compare_test`, `theDetectorNameWithoutItsPartsFaults`, one row with the detector word behind an aggregate marker: `hwlat@median:lower:abs=5` |
| `source/harness/jitter_probe.cpp` | 197 | `3 -> 5` | `harness_hwlat_probe_test`, `theNmiRowGoneAtTheClosingEdge`: the planted `proc/interrupts` carries the figure at the opening edge and names no NMI row at the closing edge, written inside the reader call of that edge |

### The gap

The allowance reads 21 and the head reads 24. The three rows of the gap
sit in the two classes above, and no legitimate input reaches any of
them. FR-007 keeps them open and forbids changing the allowance or
inventing an exception, so the gate exits 1 at this head on the
shortfall while the coverage-data fault itself stands repaired and the
line figure reads 100.0 percent. The pull request body carries the
count, the ledger and this gap for the owner's ruling.

## COV-005: the lanes other than coverage, at the head tree

`sched_getaffinity` answers 48 cpus and the online list reads `0-47`, so
the required `taskset -c 0,1` lane and the four-cpu lane both stand
runnable on real allowed cpus.

| Lane | Exit | Passed | Failed | Did not run | Timeout |
| --- | --- | --- | --- | --- | --- |
| `ci-rocky` build | 0 | - | - | - | - |
| `ctest -j 24`, `ci-rocky` | 0 | 189 of 190 | 0 | 1 (`harness_topdown_run_test (Skipped)`) | none |
| `taskset -c 0,1 ctest`, serial | 0 | 189 of 190 | 0 | 1 (same) | none |
| `taskset -c 0,1,2,3 ctest -j 4` | 0 | 189 of 190 | 0 | 1 (same) | none |
| `ci-sanitize` build, `ctest -j 24` | 0 | 184 of 185 | 0 | 1 (same) | none |
| `ci-tsan` build, `ctest -j 24` | 8 | 185 of 186 | 1 | 1 (same) | none |

Wall times: `90.40 s` on all cpus, `279.22 s` on two cpus, `135.48 s` on
four cpus. The entry that did not run is `harness_topdown_run_test`,
marked `***Skipped` by its own guard on this host, in every lane.

The sanitizer readings carry no finding: no `ERROR: AddressSanitizer`, no
`runtime error`, no `LeakSanitizer` line in the `ci-sanitize` log, and no
`WARNING: ThreadSanitizer` line in the `ci-tsan` log.

### The one failed entry, and it stands open

`harness_context_barrier_test` failed in the `ci-tsan` tree:

```
HARNESS CONTEXT BARRIER TEST FAIL: the YIELD participant never sleeps
inside the barrier: its voluntary-switch counter stands flat across the
round (D-03, Q-3, FR-014)
```

The entry passed in the `ci-rocky` lane, in both pinned lanes and in the
`ci-sanitize` lane. Run three times in a row against the same `ci-tsan`
tree it answered fail, pass, fail. The check counts voluntary context
switches of a participant that calls `sched_yield`, and the count stands
flat when the scheduler declines to switch, so the row is a host
scheduling reading and not a race: the log carries no ThreadSanitizer
line. The branch touches no file of that entry or of the barrier it
tests. The row stands open as an intermittent `ci-tsan` failure of that entry.
The base reading settles that it predates this branch: a `ci-tsan` tree
configured and built from `615c0964` in a separate worktree answered
four passes and one failure over five runs of the same entry, and that
run's failure named the `spinWait` scenario. The branch touches no file
of that entry and none of the barrier it tests.

### The cheap gates

| Gate | Exit | Reading |
| --- | --- | --- |
| `format-check` | 0 | no file out of style |
| `cmake -P cmake/spell.cmake` from the repository root | 0 | no finding |
| `prose_gate.py --check prose --mode tree` | 0 | `1082 sources, 209344 units examined, 0 findings, 0 skipped` |
| `tools/docs/example_drift.py` | 0 | no drift; README `1002` lines against the cap of 1004 |
| `test/time_source_gate.sh .` | 0 | `time_source_gate: clean` |
| `dbc-gate` (doc gate, pair gate) | 0 | 349 interfaces, 0 gaps on each |
| `ci-ubuntu` build with clang-tidy and cppcheck | 0 | no `warning:` and no `error:` line |
| `git grep -c LCOV_EXCL -- source include`, summed | - | 84 markers |

### The shared-symbol audit and the downstream consumer, reproduced locally

The repair gives five helpers external linkage inside `sg::detail`, so
the shared audit stands run at the head. `ci-linux-audit` with
`BUILD_SHARED_LIBS=ON` configured, built and installed with exit 0, and
the installed object answers every CI audit:

| Audit | Reading |
| --- | --- |
| `nm -D --defined-only` for `hwloc`, `simdjson`, `hdr`, the zlib pattern, `yaml-cpp`, `4YAML` | no match |
| `ldd` for `hwloc`, `simdjson`, `hdr[-_]?histogram`, `zlib|libz`, `yaml-cpp`, `quill` | no match |
| the quill link interface in `speedgun-ngTargets.cmake` | names no quill |
| the quill symbol allowlist in `.dynsym` | exactly the two permitted thread helpers |
| the `8fmtquill` marker | no match |
| the five new helper names in `.dynsym` | no match: the hidden visibility keeps them out of the exported surface |

The build log carries six warning lines, all from the vendored hwloc C
tree (`-Wdiscarded-qualifiers`), which the standing records already
carry and no gate reads.

The downstream consumer premise stands reproduced without a system
install: the three vendored dependencies the CI job installs to
`/usr/local` were built and installed under the scratch prefix instead,
the library installed from `ci-linux-audit` in Release, and the consumer
configured, built and ran against those prefixes. Each step exited 0,
and the consumer printed its benchmark row and
`consumer: pmu catalog entries 457`.


## The follow-up order (owner ruling G-26, 2026-11-08)

The owner order of 2026-11-08 set four items on thrawn: remove the
remaining capture suppression, fix the coverage-data fault behind it,
bring the uncovered-branch shortfall to at most the recorded allowance of
21 rows, and resolve the thread-sanitizer failure and the per-lane entry
counts. The order lifts the two freezes that kept O-4 open. The IDs stand
in `spec.md` as FR-023 to FR-026, SC-015 to SC-018, D-14 and D-15, and the
rows stand in `tasks.md` as COV-008 to COV-011.

### The capture-side shape, reproduced

The capture's complaint is a different shape from the contract-closure
fault. `SG_BENCHMARK_F` and its siblings registered the case callable and
the fixture factory with two lambdas written at the macro invocation line,
which is also the line of the header of the method the same macro starts.
gcov attributes every line of a macro expansion to that one line, so the
generated method and the two lambdas share a start line with two different
end lines. lcov 2.6 then groups them under one function record and reports
the disagreement:

```
lcov: WARNING: (inconsistent) mismatched end line for
  _ZN12_GLOBAL__N_135SgBenchmarkFixture_OrderFixture_run15sgBenchmarkCaseERN2sg5StateE
  at test/source/harness_callback_test.cpp:278: 278 -> 285
```

The written trace carries the collision directly. One range record serves
three names:

```
FNL:18,278,285
FNA:18,2,...SgBenchmarkFixture_OrderFixture_run::sgBenchmarkCase...
FNA:18,2,...sgBenchmarkRegistrar_OrderFixture_run lambda taking State&...
FNA:18,2,...sgBenchmarkRegistrar_OrderFixture_run lambda taking none...
```

A two-shape probe on this host, `~/scratch/coverage-data-repair/macroprobe/probe.cpp`,
compiles the current registration and a named-function registration in one
translation unit, runs both, and captures with `lcov -c`. The lambda shape
emits `FNL:4,108,113` with two names and the warning
`mismatched end line ... 113 -> 108`. The named shape emits one record per
function, `FNL:2,67,70`, `FNL:3,73,78` and `FNL:5,115,120`, and no warning.

### The repair: named entry functions (D-14)

`SG_BENCHMARK_FIXTURE_REGISTER` and
`SG_BENCHMARK_TEMPLATE_INSTANTIATE_REGISTRAR` now pass
`::sg::detail::fixtureCaseEntry<Derived>` and
`::sg::detail::fixtureFactoryEntry<Derived>` to `registerFixtureBenchmark`.
The two templates stand in `include/speedgun-ng/benchmark.hpp` in
`namespace detail`, each with a doxygen contract block and no `SG_*` of its
own. The call syntax at a macro site does not change, and the six fixture
macros keep their names, their generated classes and their instance names.

At the measured head the raw capture holds 365 records that carry more than
one name, all of them same-range aliases in vendored headers and inline
library code, and none of them a fixture-macro record. The capture, the
extract, the `--list` step, the gate's `--summary` read and `genhtml` print
no `(inconsistent)` line, and `grep -rn 'ignore-errors' cmake tools` names
one line: the capture's `mismatch,negative`.

### The two `hwlat` gates (D-15)

`namesVendorText` and `rowNamesFigure` stood in the anonymous namespace of
`source/harness/latency_counters.cpp`, marked `[[maybe_unused]]`, and
reached a caller only inside a postcondition. A refused answer stops the run
at the contract, so no test input could take the false arm of either gate,
and four branch arms stood cold. Both now stand declared in
`source/harness/detail/latency_counters.hpp` with their expressions
unchanged, and the scenario `theGatePredicatesAnswerEachArm` in
`test/source/harness_latency_counters_test.cpp` asks each of them the empty
text, the text naming the vendor key alone, the text naming the vendor word
alone, the text naming both, no figure, a figure present in the row and a
figure absent from the row.

### The thread-sanitizer entry: mechanism and fix

`harness_context_barrier_test` failed about one run in eight in the
thread-sanitizer tree. The failure is the scenario's own assertion. No
`WARNING: ThreadSanitizer` line stands anywhere in the logs.

The bracket was the observer's. It read the participant's
`voluntary_ctxt_switches` before the observer released the round, so the
window covered the participant's spin on the go flag, the barrier call, and
the participant's spin on the permit flag. The participant was bracketed a
second time from inside itself with five reads of its own counter, and in
every failing run the switch landed between the ready store and the entered
store, inside the spin on the go flag, before `arriveAndWait` was entered.
The bracket across `arriveAndWait` itself stayed flat in all 60 instrumented
rounds. The participant's minor fault count stood flat across each switch,
so the switch is not a page fault. The same shape without `-fsanitize=thread`
gives no switch. Under libtsan every memory access runs through the
sanitizer runtime, and that runtime can put an instrumented thread to sleep;
a sleep is a voluntary switch. The assertion was reading the sanitizer's
bookkeeping around the test's own handshake and calling it a sleep inside
the barrier.

The fix moves the bracket to the participant, at the two ends of the barrier
call, and holds the round open with a fixed count of steps whose sum the
observer checks, the shape `scenarioNoClockNoTimeout` already uses. The
observer-side reader of the counter is gone; the status file stays for the
task state, which is the BLOCK arm's mark. The participant reads its own
count through `/proc/self/task/<id>/status`. The `getrusage` route stands
closed: the time-source gate of D-6 bans `getrusage` and names one
allowance, and no spec, plan, override or suppression comment creates
another. The file's
earlier comment, that an in-thread read of the status file costs the reader
one voluntary switch of its own, was measured again on this host and did not
survive: 2000 consecutive reads from the calling thread cost zero voluntary
switches, plain and under the thread sanitizer, and each read answers 1276
bytes.

Counts on thrawn, thread-sanitizer tree, eight in flight, which is the shape
ctest uses:

| Reading | yieldWait | spinWait | insideOutside |
| --- | --- | --- | --- |
| The branch's own binary, 480 runs | 62 failed | in the same sample | 0 failed |
| The fixed binary, 240 runs | 0 failed | 0 failed | 0 failed |
| The fixed binary, 960 runs | 1 failed | 0 failed | 0 failed |

The idle protocol shows nothing, because the fault is rare on an idle
machine: the branch's own binary read 1 failure in 60 idle runs and 1 in 200
idle runs.

The assertion still fails for a real product fault. With the YIELD arm
replaced by the standard atomic wait, the scenario fails 11 runs in 20. With
a millisecond of sleep added to the YIELD arm, or to the SPIN arm, it fails
20 in 20. The BLOCK arm still proves its mark the other way: its count must
move and the task state must read `S` before the release.

Two facts this pass adds that the earlier report did not carry. The SPIN arm
is flaky at the same rate as the YIELD arm, at about 13 percent at eight in
flight. And at the head before the fix, only 12 of 20 rounds took the wait
arm at all; the fixed entry holds the round open long enough that 20 of 20
rounds take it, so the scenario now measures the wait it claims to measure.

### The entry count of every lane (FR-026)

| Lane | Entries | Passed | Did not run | Entries the lane does not register |
| --- | --- | --- | --- | --- |
| `ci-coverage` | 185 | 184 | 1 | the five `dbc_matrix_*` entries |
| `ci-rocky` | 190 | 189 | 1 | none |
| `ci-sanitize` | 185 | 184 | 1 | the five `dbc_matrix_*` entries |
| `ci-tsan` | 186 | 185 | 1 | the four allocation-counting proofs |

The five `dbc_matrix_*` entries register only where the preset sets
`SPEEDGUN_DBC_MATRIX`, because each cell configures and builds a tree of its
own (`test/CMakeLists.txt:158`, 030 FR-019, D-5, P-01). The four entries
`counters_noalloc_test`, `harness_argument_test`, `harness_capture_test` and
`harness_jitter_probe_alloc_test` replace the global `operator new` and
`operator delete` in their own translation unit, and the thread-sanitizer
runtime defines the same operators, so the two definitions cannot link
(`test/CMakeLists.txt:330` and the comments beside the other three).

One entry skips in every lane on this host: `harness_topdown_run_test` exits
with the skip code its registration carries and prints `HARNESS TOPDOWN RUN
TEST SKIP: the identity of this host names a directory beside the fixture's`.
The host cpu identity matches no vendored topdown table directory, so the
scenario states its reason and leaves.

### The coverage reading at the follow-up head

Host thrawn, kernel `7.3.0-5-cachyos`, GCC and gcov 16.2.1, clang 23.1.3,
lcov and genhtml 2.6-0, cmake and ctest 4.4.4, 48 cpus online and all 48 in
`sched_getaffinity`. The host `~/.lcovrc`, which carries
`check_data_consistency = 0`, stands moved to
`~/scratch/coverage-data-repair/lcovrc.host-local` for the whole lane, so
the repository commands run unchanged with the lcov data-consistency check
live. Restore with
`mv ~/scratch/coverage-data-repair/lcovrc.host-local ~/.lcovrc`.

Fresh instrumented tree at `99d73a5a9f0a0ccc32e95df31d17ac7da2d6f6cb`,
configured with empty compiler launchers, built whole, run whole, in an
isolated worktree at `~/scratch/cov2/wt` with the host otherwise idle:

| Step | Exit | Reading |
| --- | --- | --- |
| `ctest -j 24` | 0 | `100% tests passed out of 185`, 1 skipped, 441.42 s |
| capture | 0 | no `(inconsistent)` line, no other message |
| extract | 0 | no complaint |
| `--list` | 0 | table printed |
| gate `--summary` | 0 | `branch shortfall 20 rows, within the D-14 recorded allowance of 21` |
| `genhtml` | 0 | no message |
| the whole `coverage` target | 0 | all five steps above |

```
lines.......: 100.0% (18663 of 18663 lines)
functions...: 99.0% (1958 of 1978 functions)
branches....: 99.8% (9253 of 9273 branches)
```

Markers: 84. README: 1002 lines against the cap of 1004.

### The cold-row ledger at the follow-up head: 20 rows

The gate counts the difference between the branch positions the extracted
trace holds and the positions it reports taken: 9273 minus 9253, twenty
rows. Each row below is a branch position whose taken field reads zero in
`build/coverage/coverage.info` at the measured head, with the block id the
trace gives. The rows inside an `eU0` block stand in the exception-unwind
class the extract step's `--filter exception` drops before the rate, and
they are listed because they are the same source position.

Class A, the unwind tail of a full expression: 16 rows.

| File | Line | Arms | Block | Evidence |
| --- | --- | --- | --- | --- |
| `source/harness/cli.cpp` | 280 | `29 -> 30`, `29 -> 31`, `32 -> 33`, `32 -> 34` | `f0`, `0` | the block stands entered by an unwind alone |
| `source/harness/runner.cpp` | 838 | `95 -> 96`, `95 -> 97`, `98 -> 99`, `98 -> 100` | `f0`, `0` | the same shape behind the report grid |
| `source/harness/host_record_check.cpp` | 266 | `21 -> 22`, `21 -> 23` | `f0`, `0` | the `EvidenceRow` construction, D-14 row two |
| `source/harness/options.cpp` | 1541 | `13 -> 14`, `13 -> 15` | `f0`, `0` | the schema read tail |
| `source/harness/topdown_select.cpp` | 128 | `24 -> 25`, `24 -> 26` | `f0`, `0` | the group-name tail |
| `source/harness/noise_tracer.cpp` | 486 | `18 -> 19`, `18 -> 20` | `f0`, `0` | the cleanup landing pad; no value arm stands there |

Class B, a guard the shape of the data closes: 4 rows.

| File | Line | Arm | Block | Evidence |
| --- | --- | --- | --- | --- |
| `source/harness/report.cpp` | 815 | `81 -> 82` | `f0` | the column guard the grid width closes |
| `source/harness/statistics.cpp` | 84 | `8 -> 11` | `0` | the spread guard a sample set of one closes |
| `source/harness/topdown_select.cpp` | 492 | `3 -> 19` | `0` | the leaf guard the published table closes |
| `source/harness/hunt_ranks.cpp` | 127 | `13 -> 23` | `0` | Spearman needs a non-zero rank spread and the least-squares slope needs zero x-variance, which contradict |

Rows this follow-up closed by measurement: 4. `latency_counters.cpp:183`
(one arm), `:184` (two arms) and `:193` (one arm) stand taken at this head,
after D-15 moved the two gates where a scenario can ask them a refused
answer. The two rows the first pass closed on real input,
`compare.cpp:602` and `jitter_probe.cpp:197`, stay closed.

The shortfall stands at 20 against the recorded allowance of 21, so the
gate exits 0 and no marker, widened allowance or invented exception stands
in this change.

### The lanes other than coverage at the follow-up head

Same host, same tree, the release trees built fresh from
`99d73a5a9f0a0ccc32e95df31d17ac7da2d6f6cb` in the isolated worktree.

| Lane | Exit | Passed | Failed | Did not run | Wall |
| --- | --- | --- | --- | --- | --- |
| `ctest -j 24`, `ci-rocky` | 0 | 189 of 190 | 0 | 1 (`harness_topdown_run_test (Skipped)`) | 82.18 s |
| `taskset -c 0,1 ctest`, serial | 0 on the repeat run | 189 of 190 | 0 | 1 (same) | 278.10 s |
| `taskset -c 0,1 ctest`, first run | 8 | 188 of 190 | 1 | 1 (same) | 283.05 s |
| `taskset -c 0,1,2,3 ctest -j 4` | 0 | 189 of 190 | 0 | 1 (same) | 134.28 s |
| `ci-sanitize` | 0 | 184 of 185 | 0 | 1 (same) | 140.48 s |
| `ci-tsan` | 0 | 185 of 186 | 0 | 1 (same) | 293.90 s |

No AddressSanitizer, UndefinedBehaviorSanitizer or LeakSanitizer finding
stands in the sanitizer log: zero lines. No `WARNING: ThreadSanitizer` line
stands in the thread-sanitizer log: zero lines, and the suite passes whole,
which it did not at the base.

The barrier entry, run directly:

| Tree | Protocol | Result |
| --- | --- | --- |
| `ci-tsan` | 8 runs in a row | 8 passed |
| `ci-tsan` | 240 runs, 8 in flight | 240 passed |
| `ci-rocky` | 240 runs, 8 in flight | 240 passed |

The one failed entry in the first pinned run is `report_shape_fwq_ftq_sample`,
which runs `tools/docs/report_shape.py` over the `docs_fwq_ftq` example and
compares the report shape with `docs/pages/harness.md`. It passed on the
repeat pinned run, and it passed four times in a row when run alone under
the same two-cpu pinning (14.29, 12.44, 14.23 and 21.20 seconds). The
example registers with `SG_BENCHMARK` and reaches no fixture macro, so this
change reaches none of the code that entry exercises. It stands recorded as
an observed intermittent entry, unconnected to this change.

### The cheap gates and the analyzers at the follow-up head

| Gate | Exit | Reading |
| --- | --- | --- |
| `ci-ubuntu` configure and build, clang-tidy and cppcheck | 0 | build clean of errors |
| `dbc-gate`, documentation half | 0 | `doc-gate: 349 interfaces, 0 gaps` |
| `dbc-gate`, pairing half | 0 | `pair-gate: 349 interfaces, 0 gaps` |
| `cmake -P cmake/lint.cmake` | 0 | no badly formatted file |
| `cmake -P cmake/spell.cmake` | 0 | no unknown word |
| `prose_gate.py --mode tree` | 0 | `1083 sources, 209879 units examined, 0 findings` |
| `tools/docs/example_drift.py .` | 0 | README at 1002 lines against the cap of 1004 |
| `test/time_source_gate.sh .` | 0 | clean |
| `LCOV_EXCL` markers | 0 | 84, at the cap |

The analyzer tree carries `Checks: "*"` minus a short exclusion list, and
`WarningsAsErrors` names one check, `readability-identifier-naming`. No
finding of that check stands in the tree, and the build exits 0. The
error-severity clang-tidy findings stand in three files this change does not
touch: `test/source/dbc_test.cpp`,
`test/source/harness_jitter_probe_alloc_test.cpp` and
`test/source/harness_topdown_run_test.cpp`. cppcheck 2.22 ran with
`--inline-suppr` and answered nothing.

### The shared-symbol audit and the downstream consumer at the follow-up head

Reproduced locally at the head tree, because the audit runs in continuous
integration and the owner ruling of 2026-11-07 keeps acceptance on this
host. Build trees stand under `~/scratch`, not in the repository tree.

A `BUILD_SHARED_LIBS=ON` install of the library answers clean on every
vendored-symbol test: no hwloc, simdjson, HdrHistogram, zlib or yaml-cpp
definition stands in the dynamic symbol table, no `4YAML` marker stands in
it, and `ldd` names none of those four as a runtime dependency. The quill
test admits the two helpers the allowlist carries,
`quill::v1::detail::get_thread_id` and `quill::v1::detail::get_thread_name`,
and nothing else: two symbols, no quill runtime dependency and no
`8fmtquill` marker. The two entry functions this change adds,
`sg::detail::fixtureCaseEntry` and `sg::detail::fixtureFactoryEntry`, stand
absent from the dynamic symbol table, so the header addition exports nothing.
The object exports 250 symbols in total.

The downstream consumer premise was reproduced the same way as at the base:
system copies of `hdrhistogram_c`, `yaml-cpp` and `quill` installed into a
scratch prefix, the library installed beside them, then `test/consumer`
configured against those prefixes with no vendored copy in its own path.
Configure, build and run each exit 0, and the run prints the benchmark line
and `consumer: pmu catalog entries 457`.
