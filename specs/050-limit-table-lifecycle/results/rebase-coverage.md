# The coverage pass of the rebased head

The pass ran on `fba814d21bdc911c95a0cdc85eb98a86e2db2701`, the sha that
`git rev-parse HEAD` answered before the first command below, on 2026-11-06.
The pass filled `build/coverage` through the `ci-coverage` preset, and every
figure here comes from that pass and no other. That preset builds the
`Coverage` configuration, whose flags read `-Og -g --coverage`
(`CMakePresets.json:107`), so the tree carries no `-DNDEBUG` and every
contract check stands live.

`results/coverage.md` is the old-base record. It measured the head before the
rebase, at `a41fc034`, and this record names it as such at each comparison.

Three other waves built and ran on this host while this pass ran. No wall
time below is an idle-host figure.

## The five steps of the pass

```text
cmake --preset=ci-coverage
cmake --build build/coverage -j 20
ctest --test-dir build/coverage -j 20 --output-on-failure
cmake --build build/coverage -t coverage
awk '/^SF:/{sf=substr($0,4)} /^DA:/{split(substr($0,4),d,","); ln[sf]++; if (d[2]+0>0) lh[sf]++} /^BRDA:/{split(substr($0,6),b,","); bn[sf]++; if (b[4]=="0") bd[sf]++} END{for (s in ln) printf "%s %d of %d lines, %d of %d branches\n", s, lh[s], ln[s], bn[s]-bd[s], bn[s]}' build/coverage/coverage.info
```

| Step | Exit code | Log file |
| --- | --- | --- |
| configure | 0 | `/tmp/050-rebase-cov/1-configure.log` |
| build | 0 | `/tmp/050-rebase-cov/2-build.log` |
| suite | 8 | `/tmp/050-rebase-cov/3-ctest.log` |
| trace | 1 | `/tmp/050-rebase-cov/4-trace.log` |
| rows | 0 | `/tmp/050-rebase-cov/8-per-file.txt` |

The configure step ran in 0.56 sec and wrote no finding. The build step ran
in 2 min 5 sec wall, and its log holds no warning and no error line. The
suite step ran in 9 min 47 sec wall, and `ctest` exited 8.

The tree registers 181 entries, and `ctest --test-dir build/coverage -N`
answers `Total Tests: 181` (`/tmp/050-rebase-cov/10-registered.log`). None of
them carries the name `dbc_matrix`, because the guard at
`test/CMakeLists.txt:158` registers the matrix only in a tree whose preset
sets `SPEEDGUN_DBC_MATRIX`, and `ci-coverage` sets it nowhere
(`CMakePresets.json:138`, `:164`, `:171`).

The trace step exited 1, and its log stops at 4096 bytes inside the extract
step, so the log carries no closing line of the wrapper that ran it. The two
files it had written stand whole: each ends on `end_of_record`, the raw trace
holds 6497946 bytes and the extracted trace 660484. This wave therefore re-ran
the extract step of `cmake/coverage.cmake` alone, and it exited 0 three times
in a row. The three logs stand at
`/tmp/050-rebase-cov/4b-extract.log`,
`/tmp/050-rebase-cov/4c-extract-repeat.log` and
`/tmp/050-rebase-cov/4d-extract-repeat2.log`. The file in the tree now holds
md5 `461f74387489e7489cec4267e390576f`, and the two repeat readings answer
the same md5, so every figure below reads that file. The rows step is one
`awk` pass over the `SF:`, `DA:` and `BRDA:` rows of that trace, and it is the
reader of every pair below.

The step log `/tmp/050-rebase-cov/4-trace.log` holds the seven `inconsistent`
warnings of the capture step. They name the fixture macros of `SG_BENCHMARK_F`
in three files: `test/source/harness_callback_test.cpp`,
`test/source/harness_fixture_test.cpp` and `example/docs/fixtures.cpp`. The
capture keeps them because the
project passes `--ignore-errors mismatch,negative,inconsistent`.

The suite line reads `99% tests passed, 1 tests failed out of 181`. Three
entries did not run, and each answers the skip code: `counters_pmu_test` (42),
`counters_overhead` (57) and `harness_topdown_run_test` (179). The failure
names no counters file. Entry 62, `counters_limit_lifecycle_test`, the new
entry of this lane, Passed in 0.03 sec, at log line 216.

The one failure stands at log line 356: `harness_hunt_cli_test`, CTest entry
145, `***Failed` in 96.45 sec. The lane rule asks for a re-run of a failed
entry alone, so this pass ran the command below after the trace closed. It
exits 8 on one entry again, in 95.84 sec, and its message reads
(`/tmp/050-rebase-cov/9-entry-145-alone.log`).

```text
ctest --test-dir build/coverage -R '^harness_hunt_cli_test$' --output-on-failure
```

```text
HARNESS HUNT CLI TEST FAIL: one run of one benchmark answers one block, so the named level admits no second block of the companion (FR-016, FR-034)

HARNESS HUNT CLI TEST FAIL: the re-executed scenario passes
```

The trace step closed before that re-run, so the re-run moved no count in any
figure here. The two lines carry the base fault of the entry. The lane record
`results/base-state.md:67-70` prints them word for word from the plain tree at
the base, and that tree compiles the contract checks out with `-DNDEBUG`
(`CMakePresets.json:74`). The same two lines stand in this instrumented run,
where the checks stand live. The entry therefore reads as it reads at the
base, on the one-block rule whose message sits at
`test/source/harness_hunt_cli_test.cpp:3134` inside
`scenarioTopdownLevelNamesTheBlockOfAHunterRun()` at `:3121`, and the second
line at `:3683` reports the same fault through the scenario driver. The
no-`-DNDEBUG` build explains no entry of this pass.

The old-base record reads four failures, and it attributes three of them to
the live contract checks, in its words at `results/coverage.md:56`: "the
contract checks stand live in this tree". Those three entries pass in this
pass, where the same checks
stand live. Each row names its entry, the reading of the old-base record, and
this pass.

- `harness_jitter_cli_test`: failed there on the idle stop printing one line
  (FR-017), Passed here in 250.96 sec.
- `harness_hunt_plan_test`: failed there on one line naming the value the
  option read (FR-081), Passed here in 2.16 sec.
- `report_shape_fwq_ftq_sample`: failed there on 156 sample lines against 155
  printed, Passed here in 65.37 sec.
- `harness_hunt_cli_test`: failed there on the refusal reading this bullet
  quotes, and Failed here on the one-block rule. The old-base reading reads
  `--hunt=cpu/nope exited 2 with (274 vs 61)`, and this pass reaches no such
  reading, because that scenario holds its exit here.

That last reading is the one a live contract check produces: the old-base
record says the live check turns the 61-character shipped refusal into the
274-character trap report. The check stands live here too, so the reading
belongs to the base head of that record and to no head of this pass.

## The per-file pairs of the changed files

The bar of the lines is 100 percent of the `DA:` rows of each changed file.
The bar of the branches is that the three files add no dark row beyond the six
the lane carries. One sweep over the `SF:`, `DA:` and `BRDA:` rows answers
each pair. That sweep stands in the steps section above, and it is the form
`results/coverage.md:97` uses. It reads the rows of the trace, so no summary
field of a record enters a figure here.

The sweep prints full paths, and the rows below keep the repository-relative
names. Each row names its file, states this pass, and states the old-base
record beside it.

- `source/counters/system.cpp`: 281 of 281 lines, 137 of 140 branches. The
  old-base record reads 281 of 281 lines and 137 of 140 branches, and the lane
  base of `results/base-state.md:144` reads 282 of 282 lines and 137 of 140
  branches.
- `source/counters/detail/core.hpp`: 79 of 79 lines, 31 of 34 branches. The
  old-base record reads 79 of 79 lines and 31 of 34 branches.
- `include/speedgun-ng/counters_system.hpp`: 20 of 20 lines, 6 of 6 branches.
  The old-base record reads 20 of 20 lines and 6 of 6 branches.

Every `DA:` row of the three files carries a nonzero count, so the first bar
stands. All six figures answer the old-base record unchanged, and the reason
is a command any reader can run at this head:

```text
git diff a41fc034 HEAD -- source/counters/system.cpp source/counters/detail/core.hpp
```

It prints nothing, so the two source files stand byte-identical across the
rebase, and their pairs cannot move. The header answers 42 added lines across
that rebase: the `<cstdint>` include, the `LimitSource` enumeration and the
`objectLimitSource` declaration. Its record still holds 20 `DA:` rows and 6
`BRDA:` rows, so none of the added lines carries an instrumented row.

## The dark branch rows

The next sweep names every branch row the trace holds at taken zero, in the
form the in-tree gate prints at `tools/dbc/coverage_gate.sh:128`.

```text
awk '/^SF:/{sf=substr($0,4)} /^BRDA:/{split(substr($0,6),b,","); if (b[4]=="0") print sf " line " b[1] " branch " b[3]}' build/coverage/coverage.info
```

It prints six rows across the two source files, and none for the header.

- `source/counters/system.cpp line 156 branch 16 -> 33`, where the
  `pathComponents` loop stands at line 156.
- `source/counters/system.cpp line 402 branch 41 -> 62`, where the
  `pathCarries` test stands at line 402.
- `source/counters/system.cpp line 518 branch 8 -> 22`, where the
  child-prefix test stands at line 518.
- `source/counters/detail/core.hpp line 332 branch 3 -> 15`, where the
  `splitWords` loop stands at line 332.
- `source/counters/detail/core.hpp line 351 branch 4 -> 21`, where the
  `editDistance` test stands at line 351.
- `source/counters/detail/core.hpp line 353 branch 8 -> 21`, where the
  `sharesWord` test stands at line 353.

Six dark rows stand here, three in each source file, and the old-base record
reads the same six line numbers at its head. No dark branch row joined the
three changed files across the rebase, so the second bar stands. The tree of
this pass holds 40 files with a dark row, and the 318 dark rows of the whole
trace stand in the next section.

## The tree-wide pair of the host trace

The plain read of the trace stops on the host defect:

```text
lcov --branch-coverage --summary build/coverage/coverage.info
```

It exits 1 (`/tmp/050-rebase-cov/6-lcov-plain.log`) and names
`source/harness/runner.cpp` at line 708, where a function stands hit while no
contained line stands hit. The host carries `LCOV version 2.6-0`. A read that
skips the consistency check finishes, and it is the reader of this pair:

```text
lcov --branch-coverage --ignore-errors inconsistent,corrupt --summary build/coverage/coverage.info
```

It exits 0 (`/tmp/050-rebase-cov/7-lcov-soft.log`) and answers 99.9 percent
lines, 18021 of 18028 lines. It answers 99.7 percent branches, 8900 of 8925
branches. Its message summary prints `inconsistent: 3`, and the three sites
stand outside the three changed files:

- `source/harness/hunt_topdown.cpp:155`
- `source/harness/runner.cpp:708`
- `source/harness/runner.cpp:821`

The old-base record names the same three sites at its head, and its two
`runner.cpp` sites stand one line higher, at 707 and 820.

The rows answer the lines pair unchanged: the trace holds 18028 `DA:` rows and
18021 of them carry a nonzero count. The seven dark rows sit in two files, one
in `source/harness/cli.cpp` and six in `source/harness/hunt_topdown.cpp`. The
branches live in two count spaces, so this record states both. The trace holds
9511 `BRDA:` rows: 9193 carry a nonzero taken count, 318 stand at taken zero,
and none reads `-`. The summary counts coverpoints, and one coverpoint carries
several rows, so its 25 dark branches and the 318 dark rows are two views of
one trace. Forty files hold a dark row.

The old-base record reads 97.6 percent over 17785 lines and 94.9 percent over
8829 branches, and it counts 425 dark lines, 446 dark branches and 537 dark
`BRDA:` rows across 83 records. This trace holds 86 records, three more. The
dark line count falls from 425 to seven, and the entries that stopped carry
the difference. The in-tree gate names that mechanism at
`tools/dbc/coverage_gate.sh:90-92`: "A developer host reads higher, because an
entry there can stop early and leave the arms past its stop dark." The
old-base pass had four entries that stopped, and the files of their programs
hold its dark lines. This pass has one entry that stops, and the file of its
program reads whole here: `source/harness/hunt_cli.cpp` answers 371 of 371
lines, where that pass answers 176 of 363. The same holds for
`source/harness/cli.cpp` at 919 of 920 lines against 786 of 912, for
`source/harness/jitter_probe.cpp` at 932 of 932 against 893 of 930, and for
`source/harness/options.cpp` at 865 of 865 against 836 of 863. Totals move with
the code across a rebase, so each pair above stands on its own file length.

Two readings of the old-base figures appear above. The prose of
`results/coverage.md` carries the tree-wide pair, the dark-row count and the
record count. The per-file pairs come from the copy this wave kept of that
pass's trace, at `/tmp/050-rebase-cov/prior-wave-coverage.info.keep`, taken
before this pass overwrote `build/coverage`. The same sweep over that copy
answers 17360 of 17785 lines and 537 dark `BRDA:` rows, the two figures the
old-base record prints, so both readings ran through one reader.

## The gate verdict

The wave brief names `tools/coverage/coverage_gate.sh`, and no such file
stands in the tree: `ls tools/coverage` answers `No such file or directory`.
The in-tree gate stands at `tools/dbc/coverage_gate.sh`, and this pass ran it
the way `cmake/coverage.cmake:113-121` wires it.

```text
cmake -E env LCOV=/usr/bin/lcov bash tools/dbc/coverage_gate.sh build/coverage/coverage.info
```

It exits 3 (`/tmp/050-rebase-cov/5-gate.log`), and the one line the gate
itself prints reads:

```text
coverage-gate: lcov failed to read build/coverage/coverage.info (exit 1)
```

The gate stops inside the tool-fault branch that opens at
`tools/dbc/coverage_gate.sh:40`, and the statement at `:42` reads `exit 3`.
The line check stands at `:45-48` and never runs, so the gate reaches no
measure on this host. The branch allowance of 21 rows stands at `:102`, and no
reading reaches it either. The host `lcov` is the thing that fails, on the
three inconsistent records named above, and this pass changed no command to
move that exit. The verdict on 100 percent line coverage therefore rests with
the CI `coverage` job, which runs the same gate over the union trace of a full
suite, and the comment at `tools/dbc/coverage_gate.sh:86-92` states that the
allowance counts the union of that job. No row of this record applies to that
verdict.

## The standing marker count

No marker joined the tree, and the count stays at the ceiling of 84.

```text
grep -rn LCOV_EXCL source include | wc -l
```

It answers 84, the standing count. Two further reads bound the same figure.
`git grep -c LCOV_EXCL -- source include tools test` sums to 84, so `test/`
and `tools/` hold no marker. The 84 rows split by kind into 27
`LCOV_EXCL_LINE`, 19 `LCOV_EXCL_BR_LINE`, 13 `LCOV_EXCL_BR_START`, 13
`LCOV_EXCL_BR_STOP`, 6 `LCOV_EXCL_START` and 6 `LCOV_EXCL_STOP`.

## The checks of this record

- The record stands non-empty, and `wc -c` answers a nonzero size.
- `grep -c '281 of 281 lines, 137 of 140 branches'` over this file answers 2.
- `grep -c '79 of 79 lines, 31 of 34 branches'` over this file answers 2.
- `grep -c '20 of 20 lines, 6 of 6 branches'` over this file answers 2.
- Each of the three pair strings stands twice here, once in section two and
  once in the bullet above it.
- `grep -rn LCOV_EXCL source include | wc -l` answers 84.
- The prose gate answers `prose-lint: 1 sources, 271 units examined, 0
  findings, 0 skipped` and exits 0
  (`/tmp/050-rebase-cov/11-prose-gate.log`). It counted this record as one
  source, so its verdict covers the file.
- `codespell` of `/tmp/050-tools-venv` exits 0 and prints its config banner
  alone (`/tmp/050-rebase-cov/12-codespell.log`).
- Six lines pass 80 columns: the verbatim message of entry 145 and five
  command lines a reader copies whole. Every prose line stays within 80.

## The constants and the writes of this wave

This wave wrote one file in the tree, this record, and it touched one build
tree, `build/coverage`. No `git` command of this wave wrote anything, and no
command of it started a second instrumented run.

The tree held 267 `.gcda` files that a previous pass left. The wave deleted
them after the build step, whose own run leaves zero `.gcda` behind, so no
count above carries a suite run before this one. The wave kept the trace of
that pass at `/tmp/050-rebase-cov/prior-wave-coverage.info.keep` before its
first write into `build/coverage`, and section four names the use of that copy.

The suite ran with `-j 20`, the reading this wave's brief names, where the
old-base record ran with `-j 1`. The extract step ran the command the tree
caches at `build/coverage/CMakeCache.txt` under `COVERAGE_EXTRACT_COMMAND`,
word for word. The single-entry re-run of `harness_hunt_cli_test` ran after
the trace closed, so it moved no figure in this record.
