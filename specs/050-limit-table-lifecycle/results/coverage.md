# T021: the coverage pass of the head (SC-002)

The pass ran on `a41fc03441fdcc0de0b351e7e86d31f374c3eaf2`, the sha that
`git rev-parse HEAD` answered before the first command below, on 2026-11-05.
The pass filled `build/coverage` through the `ci-coverage` preset, and every
figure here comes from that pass and no other. The section
`The base coverage of the three files` of `results/base-state.md` carries
the base pair of each file, and the two bars close this record.

## The five steps of the pass

```text
cmake --preset=ci-coverage
cmake --build build/coverage -j 20
ctest --test-dir build/coverage -j 1 --output-on-failure --no-tests=error
cmake --build build/coverage -t coverage
bash tools/dbc/coverage_gate.sh build/coverage/coverage.info
```

| Step | Exit code | Log file |
| --- | --- | --- |
| configure | 0 | `/tmp/050-T021-1-configure.log` |
| build | 0 | `/tmp/050-T021-2-build.log` |
| suite | 8 | `/tmp/050-T021-3-ctest.log` |
| trace | 2 | `/tmp/050-T021-4-trace.log` |
| gate | 3 | `/tmp/050-T021-5-gate.log` |

The tree registers 177 entries, five fewer than the 182 of the plain tree.
`ci-coverage` leaves `SPEEDGUN_DBC_MATRIX` unset, while `ci-rocky` sets it
ON (`CMakePresets.json:113` and `:168`), and the guard at
`test/CMakeLists.txt:155-158` leaves the five `dbc_matrix_*` entries
unregistered in trees whose preset leaves the switch unset. The same
mechanism stands behind the sanitizer tree in `results/sanitizers.md`.

The suite line reads `98% tests passed, 4 tests failed out of 177`. None of
the four names a counters file. Entry 59, `counters_limit_lifecycle_test`,
the new entry of `d9a4790e`, Passed in 0.02 sec. The four failures read:

- line 173: `harness_jitter_cli_test`, 0.57 sec, `the idle stop prints one
  line (FR-017)`.
- line 277: `harness_hunt_plan_test`, 0.26 sec, `one line names the value the
  option read (FR-081)`.
- line 287: `harness_hunt_cli_test`, 0.59 sec, `--hunt=cpu/nope exited 2 with
  (274 vs 61)`.
- line 341: `report_shape_fwq_ftq_sample`, 50.20 sec, 156 sample lines
  against 155 printed.

The plain tree runs the same programs under different flags, and that is why
these four read differently here. `ci-rocky` inherits
`CMAKE_CXX_FLAGS_RELEASE` with `-DNDEBUG` (`CMakePresets.json:67`), so the
contract checks compile out of the plain tree. `ci-coverage` builds
`CMAKE_BUILD_TYPE=Coverage`, whose
`CMAKE_CXX_FLAGS_COVERAGE` reads `-Og -g --coverage
-fprofile-update=atomic -fno-inline -fkeep-inline-functions`
(`CMakePresets.json:99`), and that set carries no
`-DNDEBUG`, so the contract checks stand live in this tree. The live check
turns the 61-character shipped refusal of `--hunt=cpu/nope` into the
274-character trap report of line 287. The other three entries read CLI text
or measured shapes, and the live checks change that text too. The plain-tree
readings bound the difference: at the head, `ctest --test-dir build -j 20`
and `taskset -c 0,1 ctest --test-dir build -j 2` each pass 181 of 182
entries. Those three entries pass there, and `harness_hunt_cli_test` fails
there for its base reason, the one `base-state.md` records, not for this
one. The difference therefore belongs to the flags of the two presets, and
not to the change under test.

The trace step wrote the file first and died after. Its log prints `Writing
data to $HOME/code/speedgun-ng/build/coverage/coverage.info` at
21:41, then a summary run of the plain host `lcov` fails on one inconsistent
record, and the make rule ends with `Error 2`. The trace file stands written,
so every figure below reads it.

The gate step exits 3. Its log reads:

```text
coverage-gate: lcov failed to read build/coverage/coverage.info (exit 1)
```

The plain `lcov` inside the gate stops on one inconsistent record, so the
gate reaches no rate line and exits at the tool-fault branch of
`tools/dbc/coverage_gate.sh:40-42`, before the line check of `:45-48`. The
pass changed no command to move that exit. The in-tree gate cannot run its
measure checks on this host, and the CI `coverage` job carries that verdict.
The allowance of 21 rows at
`tools/dbc/coverage_gate.sh:102` is the tree-wide ledger of the in-tree
gate, and the gate applies it to the union trace of the CI `coverage` job.
It says nothing about this host trace, and no row here applies to it.

## The per-file pairs of the changed files

The bar of the lines is 100 percent of the `DA:` rows of each changed file.
The bar of the branches is that the two files with dark rows add none beyond
the six of the base. One pass over the `SF:`, `DA:` and `BRDA:` rows answers
each pair:

```text
awk '/^SF:/{sf=substr($0,4)} /^DA:/{split(substr($0,4),d,","); ln[sf]++; if (d[2]+0>0) lh[sf]++} /^BRDA:/{split(substr($0,6),b,","); bn[sf]++; if (b[4]=="0") bd[sf]++} END{for (s in ln) printf "%s %d of %d lines, %d of %d branches\n", s, lh[s], ln[s], bn[s]-bd[s], bn[s]}' build/coverage/coverage.info
```

The pass prints full paths, and the rows below keep the repository-relative
names. Each row names its file, states its measure, gives this pass, and
quotes the base of `results/base-state.md`.

- `source/counters/system.cpp`, line measure: 281 of 281 lines, base 282 of
  282 lines.
- `source/counters/system.cpp`, branch measure: 137 of 140 branches, base 137
  of 140 branches.
- `source/counters/detail/core.hpp`, line measure: 79 of 79 lines, base 79 of
  79 lines.
- `source/counters/detail/core.hpp`, branch measure: 31 of 34 branches, base
  31 of 34 branches.
- `include/speedgun-ng/counters_system.hpp`, line measure: 20 of 20 lines,
  base 20 of 20 lines.
- `include/speedgun-ng/counters_system.hpp`, branch measure: 6 of 6 branches,
  base 6 of 6 branches.

Every line this lane wrote reads covered. Every `DA:` row of the three
changed files carries a nonzero count, so the first bar stands. The
`system.cpp` total holds one row fewer than the base total, because
`d9a4790e` deleted twelve source lines there, and `git show d9a4790e` prints
that deletion. The added lines stand on rows that already carry nonzero
counts, so no new `DA:` row appears. The rows are the reading, never a
summary field: the `BRF:` field of the `system.cpp` record reads 134, while
that record holds 140 `BRDA:` rows, and `base-state.md:148` records the same
fact at the base.

## The dark branch rows

The next command names the dark branch line numbers of every file:

```text
awk '/^SF:/{sf=substr($0,4)} /^BRDA:/{split(substr($0,6),b,","); if (b[4]=="0") print sf":"b[1]}' build/coverage/coverage.info
```

That filter prints no row for `include/speedgun-ng/counters_system.hpp`. It
prints three rows for each of the two other files.
`source/counters/system.cpp` holds dark branch lines 156, 402, and 518,
where the base record holds 156, 414, and 530.
`source/counters/detail/core.hpp` holds dark branch lines 332, 351, and 353,
where the base record holds 324, 343, and 345. The file
`include/speedgun-ng/counters_system.hpp` holds none, at the base and here.

Six dark rows stand at the head, three in each file, and no dark branch row
appeared beyond the six. The base carries six too. The change `d9a4790e`
deleted twelve lines above line 414 of `system.cpp` and added eight lines
above line 324 of `core.hpp`, and the hunk headers of `git show d9a4790e`
name those two shifts. Each moved number stands at the same source statement
as its base number, so the second bar holds. The head reads: the
`pathComponents` loop at line 156, the `pathCarries` test at line 402, and
the child-prefix test at line 518, where the base rows stood at lines 156,
414, and 530. The dark row of line 156 keeps its base number, because the
deletion begins below it.

## The tree-wide pair of the host trace

This pair is one reading of the host trace, and the lane records it without
barring it. The wrapped read is the only reader that finishes:

```text
/tmp/lcov-soft --branch-coverage --summary build/coverage/coverage.info
```

It exits 0 and answers 97.6 percent lines, 17360 of 17785 lines. It answers
94.9 percent branches, 8383 of 8829 branches. The pair leaves 446 dark
branch rows tree-wide. The base pair read 97.5 percent over 17786 lines and
94.9 percent over 8829 branches, leaving 450 dark rows. The trace itself
holds 537 `BRDA:` rows at taken zero across its 83 records. The summary
space counts coverpoints, and one coverpoint can carry several rows.

## The host lcov defect

The host carries `LCOV version 2.6-0`, and it loses data consistency on
three records of this trace. The plain read names one site and stops:

```text
lcov --branch-coverage --summary build/coverage/coverage.info
```

It exits 1 and names `source/harness/hunt_topdown.cpp` at line 155. The
trace step and the gate are two further plain reads, and they stop the same
way: the trace step names `source/harness/runner.cpp` at line 707, and the
gate names `source/harness/hunt_topdown.cpp` at line 155. The wrapped read
at `/tmp/lcov-soft` adds `--ignore-errors inconsistent,corrupt`, and it
finishes. It names three sites, in this order:

```text
source/harness/hunt_topdown.cpp:155
source/harness/runner.cpp:707
source/harness/runner.cpp:820
```

Each site reports a function that is hit while no contained line is hit, and
the message summary of the wrapped read prints `inconsistent: 3`. All three
sites sit outside the three changed files.

## The standing marker count

No marker was added, and the count stays at the ceiling of 84:

```text
grep -rn LCOV_EXCL source include | wc -l
```

It answers 84.
