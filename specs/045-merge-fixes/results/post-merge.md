# Post-merge record: thrawn (specs/045-merge-fixes PM45-001 to PM45-003)

Head under test: the head of `fix/post-merge-78-79`, cut from `origin/master` at
`858c55d9` (`0.31.0`, schema `1.14`, harness SOVERSION 21, archive SOVERSION 6).
The lane carries the spec 046 row PM46-001 beside the four rows here. No version,
schema or SOVERSION change, and no CI or gate change.

Host: thrawn, kernel `7.3.0-5-cachyos`, gcc and gcov `16.2.1 20260810`, cmake and
ctest `4.4.4`, lcov `2.6-0`, 48 cpus, AuthenticAMD family `26`, model `8`,
stepping `1`. Date: 2026-11-04.

Host-local measures in place before the runs, none of them in the repository:
`~/.lcovrc` with `check_data_consistency = 0`, which lcov `2.6` needs for the
lambda record shape gcov `16.2.1` emits, and the coverage tree configured with
`CMAKE_CXX_COMPILER_LAUNCHER` and `CMAKE_C_COMPILER_LAUNCHER` empty, which the
`.gcda` stamp of `table_parse.cpp` needs. Both stand under `~`.

## 1. PM45-001: the branch reading at `858c55d9`

`cmake --preset=ci-coverage`, `cmake --build build/coverage`, the instrumented
suite with `ctest --test-dir build/coverage --output-on-failure --no-tests=error
-j 8`, then `cmake --build build/coverage --target coverage`.

```
100% tests passed out of 176
174 - harness_topdown_run_test (Skipped)
lines.......: 100.0% (17812 of 17812 lines)
branches....: 99.4% (8775 of 8829 branches)
coverage-gate: branch shortfall 54 rows exceeds the D-14 recorded allowance of 21
```

The skipped entry carries the host reason of its registration: one scenario names
a vendor this build holds no vendored table for, and the identity of this host
names a directory beside that fixture. The other 27 scenarios of that entry run.

The 54 short rows by file:

```
17  source/counters/linux_pmu/topdown_select.cpp
13  source/harness/cli.cpp
 7  source/harness/runner.cpp
 3  source/harness/options.cpp
 2  source/harness/execution_context.cpp
 2  source/harness/host_record_check.cpp
 2  source/harness/jitter_probe.cpp
 2  source/harness/report.cpp
 1  source/counters/linux_pmu/encode.cpp
 1  source/counters/linux_pmu/table_parse.cpp
 1  source/harness/hunt_ranks.cpp
 1  source/harness/hunt_topdown.cpp
 1  source/harness/json_load.cpp
 1  source/harness/statistics.cpp
```

## 2. PM45-001: the tests the lane adds

Each test lands in its own commit with the rows it reaches. No production source
moved, no `LCOV_EXCL` was added, no `--omit-lines`, no `--ignore-errors` and no
`--remove` joined the extract step, and the marker count from
`git grep -c LCOV_EXCL -- source include` reads 84.

| row | arms | test that reaches it |
| --- | ---- | -------------------- |
| `source/counters/linux_pmu/encode.cpp:158` | `"5 -> 8"` | `counters_linux_pmu_seam_test.cpp`, `fixedCounterScenario` |
| `source/counters/linux_pmu/table_parse.cpp:497` | `"36 -> 37"` | `counters_linux_pmu_seam_test.cpp`, `fixedCounterScenario` |
| `source/counters/linux_pmu/topdown_select.cpp:103` | `"3 -> 5"` | `counters_topdown_select_test.cpp`, `theFillOfOneNameFillsTheRecordOrLeavesIt` |
| `source/counters/linux_pmu/topdown_select.cpp:300` | `"5 -> 9"`, `"7 -> 9"` | `counters_topdown_select_test.cpp`, `aGroupTextOfNoLevelOrOfNoParentStandsOutOfTheTree` |
| `source/counters/linux_pmu/topdown_select.cpp:324` | `"12 -> 13"` | `counters_topdown_select_test.cpp`, the same scenario |
| `source/counters/linux_pmu/topdown_select.cpp:568` | `"116 -> 118"` | `counters_topdown_select_test.cpp`, the same scenario |
| `source/counters/linux_pmu/topdown_select.cpp:604` | `"155 -> 156"` | `counters_topdown_select_test.cpp`, the same scenario |
| `source/harness/cli.cpp:1465` | `"439 -> 448"` | `harness_jitter_cli_test.cpp`, `scenarioFamilyOverlapPrecheckRun` |
| `source/harness/cli.cpp:1766` | `"595 -> 599"` | `harness_jitter_cli_test.cpp`, `scenarioFamilyNarrowedEmptyTwo` |
| `source/harness/hunt_topdown.cpp:73` | `"2 -> 3"` | `harness_hunt_topdown_test.cpp`, `scenarioNoNodeAndNoRowAnswersUnmapped` |
| `source/harness/jitter_probe.cpp:1686` | `"108 -> 111"` | `harness_jitter_cli_test.cpp`, `scenarioQuantumRefused` |
| `source/harness/options.cpp:473` | `"6 -> 9"` | `harness_topdown_option_test.cpp`, `aBadLevelAnswersOneLine` |
| `source/harness/report.cpp:395` | `"19 -> 22"` | `harness_hunt_report_test.cpp`, `a-node-of-no-countable-state-prints-no-figure` |
| `source/harness/runner.cpp:753` | `"23 -> 27"`, `"24 -> 27"` | `harness_topdown_run_test.cpp`, `a-row-of-no-slot-for-its-column-answers-no-figure` |
| `source/harness/runner.cpp:760` | `"28 -> 33"` | `harness_topdown_run_test.cpp`, the same scenario |

## 3. PM45-001: the branch reading at the lane head

The same commands at the lane head:

```
100% tests passed out of 176
174 - harness_topdown_run_test (Skipped)
lines.......: 100.0% (17811 of 17811 lines)
branches....: 99.6% (8792 of 8829 branches)
coverage-gate: branch shortfall 37 rows exceeds the D-14 recorded allowance of 21
```

The shortfall reads 37 rows against the 54 of `858c55d9`: 17 rows closed. The
gate still fails, and the row stands open for the ruling PM45-002 asks for.

## 4. PM45-002: the 37 rows and their reasons

Four groups stand beyond the reach of any real test on this host, and one group
stands reachable. The lane adds no test shaped for a row of the four, and hides
none of them.

### 4.1 The unwind tails of a full expression: 14 rows

| row | arms |
| --- | --- |
| `source/counters/linux_pmu/topdown_select.cpp:128` | `"24 -> 26"`, `"24 -> 25"` |
| `source/harness/cli.cpp:280` | `"29 -> 31"`, `"32 -> 34"`, `"29 -> 30"`, `"32 -> 33"` |
| `source/harness/host_record_check.cpp:266` | `"21 -> 23"`, `"21 -> 22"` |
| `source/harness/options.cpp:1541` | `"13 -> 15"`, `"13 -> 14"` |
| `source/harness/runner.cpp:782` | `"95 -> 97"`, `"98 -> 100"`, `"95 -> 96"`, `"98 -> 99"` |

- `source/counters/linux_pmu/topdown_select.cpp:128`. The arms stand on the two `std::string` constructions inside the `push_back` of one `TableFile`. gcov gives each construction a landing pad for an allocation that fails. The vector holds at most the file count of one embedded record, and no caller hands an allocator to that vector.
- `source/harness/cli.cpp:280`. The arms stand on the concatenation behind the ternary of the failure line. gcov gives the temporary `std::string` a landing pad for an allocation that fails. The walk reaches the line with an origin and with no origin through `metricNameFailure`, which the seam test already calls.
- `source/harness/host_record_check.cpp:266`. D-14 row two already carries these two arms. The schema guard ahead of the read requires `path`, `command`, `exit` and `line` on every row, and a read that meets no value answers an empty one beside a note naming the key, so the `EvidenceRow` construction reaches its end on every document the reader can be handed.
- `source/harness/options.cpp:1541`. The arms stand on the concatenation inside the refusal lambda of `applySourceValue`. gcov numbers the concatenation as a cold block with a landing pad for an allocation that fails, and both arms read zero on every host the lane measured.
- `source/harness/runner.cpp:782`. The arms stand on the `push_back` of one `TopdownNode` into the record. gcov gives the node's three `std::string` members a landing pad each for an allocation that fails. The record holds at most the nodes of one tree.

### 4.2 The readback of a postcondition: 8 rows

| row | arms |
| --- | --- |
| `source/counters/linux_pmu/topdown_select.cpp:617` | `"3 -> 15"`, `"4 -> 15"` |
| `source/counters/linux_pmu/topdown_select.cpp:620` | `"5 -> 15"` |
| `source/counters/linux_pmu/topdown_select.cpp:627` | `"10 -> 11"` |
| `source/counters/linux_pmu/topdown_select.cpp:633` | `"11 -> 15"`, `"12 -> 15"` |
| `source/counters/linux_pmu/topdown_select.cpp:652` | `"4 -> 7"`, `"5 -> 6"` |

- `source/counters/linux_pmu/topdown_select.cpp:617`. The readback lambda of the `SG_ENSURE` that closes the tree: no node of a level below one, and no node deeper than the depth the tree answers. The pass that builds the vector gives both by construction, and a contract that fails calls `std::abort()` at `include/speedgun-ng/dbc.hpp:213`, which leaves no gcov counters behind.
- `source/counters/linux_pmu/topdown_select.cpp:620`. The same readback: a level-one node names no parent, and a node of a deeper level names one.
- `source/counters/linux_pmu/topdown_select.cpp:627`. The same readback: the loop that looks for the parent ahead of the node in the vector leaves by the `break` of the match it finds, so the arm that leaves by the bound stands dark.
- `source/counters/linux_pmu/topdown_select.cpp:633`. The same readback: the parent found stands one level above the node.
- `source/counters/linux_pmu/topdown_select.cpp:652`. The readback of the published flag: every node names a row the table published. The pass adds a node for a row only once that flag stands.

### 4.3 The guards the shape of the data closes: 5 rows

| row | arms |
| --- | --- |
| `source/counters/linux_pmu/topdown_select.cpp:432` | `"3 -> 19"` |
| `source/harness/statistics.cpp:84` | `"8 -> 11"` |
| `source/harness/report.cpp:814` | `"81 -> 82"` |
| `source/harness/hunt_ranks.cpp:127` | `"13 -> 23"` |
| `source/harness/jitter_probe.cpp:1459` | `"20 -> 32"` |

- `source/counters/linux_pmu/topdown_select.cpp:432`. The switch holds one arm per kind of the closed enumeration `TopdownUnsupport`, and gcov adds a sixth arm for a value outside it. The five kinds stand read: the selection test answers each of them, and the hunter report test hands each of them to `message()`.
- `source/harness/statistics.cpp:84`. The order the ranking sorts by is the order the tie scan reads, so the comparison that closes the scan answers false on every element it reads. The arm that closes the scan on that comparison stands beyond the reach of any point set the function can be handed.
- `source/harness/report.cpp:814`. `seriesRun` states that the first fold carries a series, and the loop that chooses the worst fold passes a fold of no series. The chosen fold therefore carries one whenever `seriesRun` stands, so the second operand of the guard has no path to its false arm.
- `source/harness/hunt_ranks.cpp:127`. `spearmanCorrelation` answers nothing when the ranks of one coordinate hold no spread, and `leastSquaresSlope` answers nothing on the same condition of the same points. The arm needs a point set with a spread for the ranks and none for the least-squares denominator.
- `source/harness/jitter_probe.cpp:1459`. The call guards an empty series at line 1432 and a step of zero at line 1443 and line 1450. The vector the read copies holds one entry per kept sample, and the resample path answers at least one sample for an input of no sample, so the empty answer at line 1459 stands beyond reach.

### 4.4 The arms of this host: 5 rows

| row | arms |
| --- | --- |
| `source/harness/cli.cpp:1622` | `"504 -> 508"` |
| `source/harness/cli.cpp:1625` | `"505 -> 507"` |
| `source/harness/cli.cpp:2047` | `"800 -> 802"` |
| `source/harness/execution_context.cpp:469` | `"8 -> 14"` |
| `source/harness/execution_context.cpp:470` | `"8 -> 14"` |

- `source/harness/cli.cpp:1622`. The command line prints the coverage line when `coreWideCoverage()` answers a value. thrawn publishes a sibling list for the cpu the process runs on, so the answer stands on every run of this host and the arm that prints nothing stands dark here.
- `source/harness/cli.cpp:1625`. The same line answers `whole_core` on thrawn, whose cpu holds two threads. The `one_thread` word needs a host whose sibling list holds one cpu.
- `source/harness/cli.cpp:2047`. The area pass hands a tree on thrawn, whose identity names a vendored table, so the arm that leaves the areas unset needs a host the selection refuses.
- `source/harness/execution_context.cpp:469`. The bind runs when the planned cpu stands in the resolved node plan. The plan this host resolves holds the cpus of the run, and the placement the context applies stands inside that plan, so the search answers inside on every run of this host.
- `source/harness/execution_context.cpp:470`. The refusal arm needs a `bind` the system denies. The context thread stands inside the mask of the run, and `set_mempolicy` answers for a node the host holds, so no run of this host reaches the refusal.

### 4.5 Reachable, and left open by this lane: 5 rows

| row | arms |
| --- | --- |
| `source/harness/cli.cpp:1675` | `"533 -> 537"`, `"534 -> 537"` |
| `source/harness/cli.cpp:1766` | `"596 -> 599"` |
| `source/harness/cli.cpp:1883` | `"673 -> 677"` |
| `source/harness/json_load.cpp:131` | `"3 -> 43"` |

- `source/harness/cli.cpp:1675`. Reachable. The loop needs a second family probe that stands never started beside the live one at the point where the availability answer refuses a strict run. The scenario that holds that refusal, `childFamilyUnavailable`, stands with one family.
- `source/harness/cli.cpp:1766`. Reachable, one arm left. The lane added the second family to the empty narrowing scenario, which closes the arm of no session. The arm that needs a session already joined needs a walk that joins one probe before that family's last instance, which the interleaved order of two probed families gives.
- `source/harness/cli.cpp:1883`. Reachable. The stop stands on a probe the walk started and has not joined. A walk that joins a family's probe ahead of that family's last instance, the interleaved order of two probed families, reaches the arm.
- `source/harness/json_load.cpp:131`. Reachable. The fold reports the parts of one record in order and keeps the first error. The arm needs a document whose first field of that record reads as an error, which the reader meets on a document of the matching shape.

## 5. PM45-003: the runs at the lane head

The runs stand at the squashed head of `fix/post-merge-78-79`, the one commit cut
from `858c55d9`, whose sha the pull request names. This record is amended into
that commit, so the sha moves and no line of code these commands read moves with
it. The head before the squash reads `3341b1a5`.

The build of the `ci-rocky` tree at the head, `cmake --build build -j 14`,
exits 0.

Full suite:

```
$ ctest --test-dir build --output-on-failure --no-tests=error -j 24
100% tests passed out of 181
179 - harness_topdown_run_test (Skipped)
exit 0
```

Two-cpu mask, every entry the tree registers, the way the thrawn record of T202
reads it:

```
$ taskset -c 0,1 ctest --test-dir build --output-on-failure --no-tests=error -j 2
100% tests passed out of 181
179 - harness_topdown_run_test (Skipped)
exit 0
```

Address and undefined-behavior sanitizer preset, `cmake --preset=ci-sanitize` and
`cmake --build build/sanitize -j 14`:

```
$ ctest --test-dir build/sanitize --output-on-failure --no-tests=error -j 8
100% tests passed out of 176
174 - harness_topdown_run_test (Skipped)
exit 0
```

Thread sanitizer preset, `cmake --preset=ci-tsan` and `cmake --build build/tsan
-j 14`:

```
$ ctest --test-dir build/tsan --output-on-failure --no-tests=error -j 8
100% tests passed out of 177
175 - harness_topdown_run_test (Skipped)
exit 0
```

The same suite at the pre-squash head failed one entry,
`harness_context_barrier_test`, with `the YIELD participant never sleeps inside
the barrier: its voluntary-switch counter stands flat across the round` and no
report line from the sanitizer behind it. wrinkle 3 of
`specs/039-context-groups/results/verdict.md` names that entry flaky under a
thread sanitizer on this host and green without one, and counts the two scenarios
that read `voluntary_ctxt_switches`. Three runs of the same binary direct from the
shell exit 0 there, and `git diff --name-only origin/master..HEAD` names no file
that entry reads.

`ci-coverage` stands at section 3: lines 100.0%, branches 99.6%, and the branch
shortfall at 37 rows against the allowance of 21. The instrumented suite there
reads `100% tests passed out of 176` with the same one entry skipped, and the
`ci-rocky` tree registers 181 entries because `SPEEDGUN_DBC_MATRIX` stands on
there. The capture step exits 2 on the gate verdict that follows the report, the
reading wrinkle 2 of that same record names.

## 6. What the lane asks of the owner

The lane closes 17 rows with real tests and reaches the reading 37 against the
D-14 allowance of 21. The gap stands at 16 rows.

Thirty-two of the 37 stand in the four groups of sections 4.1 to 4.4: the unwind
tails of a full expression, the readback of a postcondition, the guards the shape
of the data closes, and the arms of this host's topology. Row two of D-14 already
carries two of them, the `host_record_check.cpp:266` pair, and the note beside
that row names the same class for seven further rows of that file, which the
extract step's `--filter exception` drops before the rate.

Five rows stand reachable, and section 4.5 names the scenario each one needs: two
need the interleaved walk of two probed families, two need a second family probe
beside the availability refusal, and one needs a document whose first field of one
record reads as an error. The lane leaves them for the next round.

A ruling stands owed either way. Those five rows carry at most five, which leaves
32 against an allowance of 21, and the ledger of D-14 is where an accepted class
is recorded. PM45-001 bars this lane from moving the allowance, the `--omit-lines`
set, the `--ignore-errors` set or the extract step, so that change stands with
you.

The reading of D-14 counts the union of the CI coverage job, where every entry
runs to its end. thrawn skips one entry, `harness_topdown_run_test`, for the host
reason of section 1, and the arms that entry covers stand dark here. The CI
reading of the same head can therefore sit below 37, and this record states the
host it measured.
