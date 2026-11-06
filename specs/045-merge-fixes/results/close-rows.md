# Close-rows record: thrawn (specs/045-merge-fixes PM45-004 to PM45-007)

Head under test: the head of `fix/coverage-rows-to-allowance`, cut from
`origin/master` at `2cfdd47a` (`0.31.0`, schema `1.14`, harness SOVERSION 21,
archive SOVERSION 6). The owner ruling of 2026-11-05 sets the rule for this
phase: no allowance moves, and the rows stand closed by real work. No
`LCOV_EXCL` marker joined the tree, and the marker count from
`git grep -c LCOV_EXCL -- source include` reads 84. No `--omit-lines`, no
`--ignore-errors` and no `--remove` joined the extract step, and
`tools/dbc/coverage_gate.sh` and `cmake/coverage.cmake` stand as `master` carries
them.

Host: thrawn, kernel `7.3.0-5-cachyos`, gcc and gcov `16.2.1 20260810`, cmake and
ctest `4.4.4`, lcov `2.6-0`, 48 cpus, AuthenticAMD family `26`, model `8`,
stepping `1`. Date: 2026-11-05. The two host-local measures of the earlier record
stand unchanged: `~/.lcovrc` with `check_data_consistency = 0`, and the coverage
tree configured with `CMAKE_CXX_COMPILER_LAUNCHER` and
`CMAKE_C_COMPILER_LAUNCHER` empty.

## 1. The reading

At `2cfdd47a`, the base of this phase, the reading of the previous record stands:
lines 100.0% (17811 of 17811), branches 99.6% (8792 of 8829), and a branch
shortfall of 37 rows against the allowance of 21.

At the head of this phase:

```
100% tests passed out of 176
174 - harness_topdown_run_test (Skipped)
lines.......: 100.0% (17822 of 17822 lines)
branches....: 99.8% (8812 of 8831 branches)
coverage-gate: branch shortfall 19 rows, within the D-14 recorded allowance of 21
```

The gate passes. The totals move because one production change joined the lane:
the two readbacks of the top-down selection became functions, which adds eleven
instrumented lines and two branch records. The shortfall reads 19 against the
same allowance of 21 that `master` carries.

## 2. The rows this phase closes

Eighteen arms. Each stands in a commit with the test that reaches it.

The readback of the postcondition of the selection (8 arms). The pass that builds
a tree gives each clause by construction, so no table a host can hand the
selection breaks one. The two readbacks became internal functions with their own
contract, called from the selection under the same `SG_ENSURE`, and the test
holds each false return with a tree and a table it builds by hand.

| clause of the readback | arms | test that reaches it |
| --- | --- | --- |
| a node level below one, and a node level past the depth | 2 | `counters_topdown_select_test.cpp`, `theShapeReadbackRefusesEachBrokenShape` |
| a level-1 node that names a parent, and a deeper node that names none | 1 | the same scenario |
| the parent search ends at its bound with no match | 1 | the same scenario |
| no upstream node, and an upstream node not one level above | 2 | the same scenario |
| a node that names no row, and a node whose row stands unpublished | 2 | `counters_topdown_select_test.cpp`, `thePublishedReadbackRefusesEachUnstatedNode` |

| row | arms | test that reaches it |
| --- | --- | --- |
| `source/harness/cli.cpp:1622` | 1 | `harness_hunt_cli_test.cpp`, `the-coverage-line-reads-the-sibling-list`, the root with no topology file |
| `source/harness/cli.cpp:1625` | 1 | the same scenario, the root with a one-cpu list |
| `source/harness/cli.cpp:2047` | 1 | `harness_hunt_cli_test.cpp`, `an-identity-no-table-carries-answers-no-area` |
| `source/harness/execution_context.cpp:469` | 1 | `harness_numa_placement_test.cpp`, `bindReadsPlanCpu`, the cpu outside the plan |
| `source/harness/execution_context.cpp:470` | 1 | the same scenario, the bind the fake binder denies |
| `source/harness/cli.cpp:1675` | 2 | `harness_jitter_cli_test.cpp`, `scenarioFamilyUnavailableStrictThree` |
| `source/harness/cli.cpp:1766` | 1 | `harness_jitter_cli_test.cpp`, `scenarioFamilyNarrowedEmptyJoined` |
| `source/harness/cli.cpp:1883` | 1 | `harness_jitter_cli_test.cpp`, `scenarioFamilyInterleavedLast` |
| `source/harness/json_load.cpp:131` | 1 | `harness_json_load_test.cpp`, `theFirstKeyOfASeriesItemStopsTheFold` |

## 3. The 19 rows that stand

Two classes remain. Neither stands closed by a test, and neither stands hidden:
no marker, filter or allowance covers them, and the gate reads them.

### 3.1 The unwind tail of a full expression: 14 rows

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

### 3.2 The guards the shape of the data closes: 5 rows

| row | arms |
| --- | --- |
| `source/counters/linux_pmu/topdown_select.cpp:492` | `"3 -> 19"` |
| `source/harness/statistics.cpp:84` | `"8 -> 11"` |
| `source/harness/report.cpp:814` | `"81 -> 82"` |
| `source/harness/hunt_ranks.cpp:127` | `"13 -> 23"` |
| `source/harness/jitter_probe.cpp:1459` | `"20 -> 32"` |


- `source/counters/linux_pmu/topdown_select.cpp:492`. The switch holds one arm per kind of the closed enumeration `TopdownUnsupport`, and gcov adds a sixth arm for a value outside it. The five kinds stand read: the selection test answers each of them, and the hunter report test hands each of them to `message()`.
- `source/harness/statistics.cpp:84`. The order the ranking sorts by is the order the tie scan reads, so the comparison that closes the scan answers false on every element it reads. The arm that closes the scan on that comparison stands beyond the reach of any point set the function can be handed.
- `source/harness/report.cpp:814`. `seriesRun` states that the first fold carries a series, and the loop that chooses the worst fold passes a fold of no series. The chosen fold therefore carries one whenever `seriesRun` stands, so the second operand of the guard has no path to its false arm.
- `source/harness/hunt_ranks.cpp:127`. `spearmanCorrelation` answers nothing when the ranks of one coordinate hold no spread, and `leastSquaresSlope` answers nothing on the same condition of the same points. The arm needs a point set with a spread for the ranks and none for the least-squares denominator.
- `source/harness/jitter_probe.cpp:1459`. The call guards an empty series at line 1432 and a step of zero at line 1443 and line 1450. The vector the read copies holds one entry per kept sample, and the resample path answers at least one sample for an input of no sample, so the empty answer at line 1459 stands beyond reach.

## 4. The runs at the head

```
$ cmake --build build -j 14                                       exit 0
$ ctest --test-dir build --output-on-failure --no-tests=error -j 24
100% tests passed out of 181                                      exit 0
$ taskset -c 0,1 ctest --test-dir build --output-on-failure --no-tests=error -j 2
100% tests passed out of 181                                      exit 0
$ ctest --test-dir build/sanitize --output-on-failure --no-tests=error -j 8
100% tests passed out of 176                                      exit 0
$ ctest --test-dir build/tsan --output-on-failure --no-tests=error -j 8
100% tests passed out of 177                                      exit 0
$ ctest --test-dir build/coverage --output-on-failure --no-tests=error -j 8
100% tests passed out of 176                                      exit 0
$ cmake --build build/coverage --target coverage                  exit 0
```

The thread-sanitizer lane passed at this head. The entry that failed at the head
of the previous record, `harness_context_barrier_test`, stands named by wrinkle 3
of `specs/039-context-groups/results/verdict.md` as flaky under a thread
sanitizer on this host.

The other gates at the head:

```
cmake --preset=ci-ubuntu and cmake --build build -j 14   exit 0, no clang-tidy
                                                         or cppcheck finding
cmake --build build --target format-check                exit 0
cmake --build build --target spell-check                 exit 0
python3 tools/prose/prose_gate.py --check all --base master   0 findings
bash test/time_source_gate.sh .                          clean
git grep -c LCOV_EXCL -- source include                  84 markers
```

## 5. Two readings the earlier record states wrong

- `source/harness/cli.cpp:1625`. The earlier record says thrawn answers
  `whole_core` and the `one_thread` word stands dark. The measurement says the
  opposite: this host publishes a two-cpu sibling list, so the standing run
  answers `one_thread`, and the dark arm is the `whole_core` word. The scenario
  of section 2 feeds a sibling list of one cpu and closes it.
- `source/harness/execution_context.cpp:470`. The earlier record counts the short
  arm there as the refused bind. gcov attributes the short-circuit edge of the
  `&&` to both line 469 and line 470, so the cpu-outside-the-plan shape moves
  both, and the refused-bind arm at line 470 already stood taken from the
  `refuseStart` scenario. The new scenario drives a denied bind as well and reads
  the refusal off the context.

## 6. What stands open

PM45-001 closes with the reading at 19. The 19 rows of section 3 stand in the two
classes above, and the allowance of D-14 covers the shortfall without a change to
the gate, the ledger, the marker set or the extract step.
