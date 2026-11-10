# Rebase: the two sanitizer legs (SC-001)

This record measures the two sanitizer trees at head
`fba814d21bdc911c95a0cdc85eb98a86e2db2701`, the head that
`git rev-parse HEAD` answers on `kennykiller`. The host carries 20 cores, and
`/usr/bin/clang++` reports clang version 23.1.4 for both presets. Three other
waves build on this host at the same time, so no wall time below is an
idle-host figure.

## The two command lines

The lane brief fixes the suite command of each leg, and this record ran both,
in this order, each one after the other:

```text
ctest --test-dir build/sanitize -j 20 --output-on-failure
ctest --test-dir build/tsan -j 20 --output-on-failure
```

Each suite command follows the configure step and the build step of its own
preset. No step of one leg ran while a step of the other leg ran:

```text
cmake --preset=ci-sanitize
cmake --build --preset=ci-sanitize -j 20
cmake --preset=ci-tsan
cmake --build --preset=ci-tsan -j 20
```

The presets fix both trees, their build types, and their sanitizer flags:

```text
ci-sanitize: binaryDir build/sanitize, CMAKE_BUILD_TYPE Sanitize,
             CMAKE_CXX_FLAGS_SANITIZE -U_FORTIFY_SOURCE -O2 -g
             -fsanitize=address,undefined -fno-omit-frame-pointer -fno-common
ci-tsan:     binaryDir build/tsan, CMAKE_BUILD_TYPE Release,
             CMAKE_CXX_FLAGS_RELEASE carries -fsanitize=thread at -O2,
             SPEEDGUN_DBC_MATRIX ON
```

Both presets inherit `ci-linux`, `dev-mode`, and `ci-cache`, so ccache fronts
every compile and `-Werror` stands in the flag list. Both select
`/usr/bin/clang++`, which reports clang version 23.1.4. The plain tree at
`build` names `/usr/bin/c++`, whose compiler id reads `GNU`, and that
difference matters for one entry below.

## The first leg: the `ci-sanitize` tree

The four steps of this leg ran one after the other, and their exits and wall
times stand below. The five logs of this leg are
`/tmp/050-san-r-cfg.log`, `/tmp/050-san-r-build.log`,
`/tmp/050-san-r-build-k.log`, `/tmp/050-san-r-ctest.log` and
`/tmp/050-san-r-rerun.log`.

```text
cmake --preset=ci-sanitize
cmake --build --preset=ci-sanitize -j 20
cmake --build --preset=ci-sanitize -j 20 -- -k
ctest --test-dir build/sanitize -j 20 --output-on-failure
```

Those four steps exit 0, 2, 2 and 8, and they take 0.3, 29, 30 and 156 sec of
wall time. The configure step names `build/sanitize` as the binary
directory. The first build step exits 2, because one test source does not
compile under this flag set; the section on failures quotes the message. The
leg then repeated the build with `-- -k`, which is `make -k`, so the rest of
the tree still reached the suite. The two build steps together recompiled 158
translation units and linked 171 programs, and one executable is absent:
`build/sanitize/test/harness_limit_disclosure_test`. Both builds reuse the
directory the old-base record built, so this is an incremental build and not a
clean one.

`ctest --test-dir build/sanitize -N` answers `Total Tests: 181`, and the tree
states 181 entries in `build/sanitize/test/CTestTestfile.cmake`. The plain tree
at the same head states 186 in its own CTest file, so this tree carries 5 fewer.
Two mechanisms drop entries, and `results/sanitizers.md` names both at the old
base. First, `test/CMakeLists.txt:158` reads `if(SPEEDGUN_DBC_MATRIX)` and
guards the five `dbc_matrix_*` rows, which `ci-tsan` sets and `ci-sanitize`
leaves unset. Second, `test/CMakeLists.txt:330` reads
`if(NOT CMAKE_CXX_FLAGS_RELEASE MATCHES "fsanitize=thread")` and guards the
four entries that replace the global allocation operators, with copies at
`:788`, `:1053` and `:1424`. Only the first mechanism acts on this tree.

The 181 entries split into 176 passed, 3 skipped and 2 failed, and line 401 of
the suite log reads `99% tests passed, 2 tests failed out of 181`. The skipped
entries are `counters_pmu_test`, `counters_overhead` and
`harness_topdown_run_test`. The two that fail are `harness_hunt_cli_test` and
`harness_limit_disclosure_test`. The suite ran from 02:27:04 to 02:29:40 local
time, and CTest reports `Total Test time (real) = 156.30 sec` for it. Three
other waves build on this host throughout, so that figure is not an idle-host
figure.

Each of the four counts the lane needs is zero over the five logs of this leg.

```text
$ cat /tmp/050-san-r-*.log | grep -c 'AddressSanitizer'
0
$ cat /tmp/050-san-r-*.log | grep -c 'LeakSanitizer'
0
$ cat /tmp/050-san-r-*.log | grep -c 'runtime error'
0
$ cat /tmp/050-san-r-*.log | grep -c 'ThreadSanitizer'
0
```

The same commands answer zero for `SUMMARY:` and for `data race`, so no report
of either sanitizer stands anywhere in this leg. One suite failure rests on the
entry's own assertion, and the other rests on a binary that no build produced.

## The second leg: the `ci-tsan` tree

The same four steps ran on the thread sanitizer tree, after the first leg
ended. Its logs are `/tmp/050-tsan-r-cfg.log`, `/tmp/050-tsan-r-build.log`,
`/tmp/050-tsan-r-build-k.log`, `/tmp/050-tsan-r-ctest.log` and the three
`/tmp/050-tsan-r-rerun-*.log` files.

```text
cmake --preset=ci-tsan
cmake --build --preset=ci-tsan -j 20
cmake --build --preset=ci-tsan -j 20 -- -k
ctest --test-dir build/tsan -j 20 --output-on-failure
```

Those four steps exit 0, 2, 2 and 8, and they take 1, 29, 15 and 290 sec of
wall time. The configure step answers 0 in its 26 lines and ends
`Build files have been written to: $HOME/code/speedgun-ng/build/tsan`.
The first build step exits 2 on the same source that stops the other leg, and
the keep-going build then linked every other program. The two build steps
together recompiled 154 translation units and linked 167 programs, and one
executable is absent: `build/tsan/test/harness_limit_disclosure_test`.

`ctest --test-dir build/tsan -N` answers `Total Tests: 182`, and the tree states
182 entries in `build/tsan/test/CTestTestfile.cmake`. Against the plain tree's
186 this tree carries 4 fewer, and the same two mechanisms explain both trees.
The `ci-tsan` preset sets `SPEEDGUN_DBC_MATRIX`, so the five `dbc_matrix_*`
rows that `test/CMakeLists.txt:158` guards stand here and not on the sanitize
tree. The guard at `test/CMakeLists.txt:330`, copied at `:788`, `:1053` and
`:1424`, drops the four entries that own the global allocation operators here,
because this preset's release flags carry `-fsanitize=thread`. The four are
`counters_noalloc_test`, `harness_capture_test`,
`harness_jitter_probe_alloc_test` and `harness_argument_test`. The plain tree
and the sanitize tree register all four, and the tsan tree registers none.

The 182 entries split into 176 passed, 3 skipped and 3 failed, and line 407 of
the suite log reads `98% tests passed, 3 tests failed out of 182`. The skipped
entries are again `counters_pmu_test`, `counters_overhead` and
`harness_topdown_run_test`. The three that fail are `harness_hunt_cli_test`,
`harness_limit_disclosure_test` and `harness_context_barrier_test`. The suite
ran from 02:42:39 to 02:47:29 local time, and CTest reports
`Total Test time (real) = 290.21 sec` for it. Three other waves build on this
host throughout, so that figure is not an idle-host figure.

Each of the four counts the lane needs is zero over the logs of this leg too.

```text
$ cat /tmp/050-tsan-r-*.log | grep -c 'ThreadSanitizer'
0
$ cat /tmp/050-tsan-r-*.log | grep -c 'runtime error'
0
$ cat /tmp/050-tsan-r-*.log | grep -c 'AddressSanitizer'
0
$ cat /tmp/050-tsan-r-*.log | grep -c 'LeakSanitizer'
0
```

The same commands answer zero for `SUMMARY:` and for `data race`. The thread
sanitizer is the arbiter of a race on this lane, and it reports none here.

## The entries that fail and what each one says

Three entries fail across the two legs. Each one failed alone too, so each
message below stands in a single-entry run and not only in a 20-way run.

### `harness_hunt_cli_test`

This entry fails on both legs, in the suite and alone. The sanitize suite shows
entry 145 as `***Failed` in 39.22 sec at line 385, and a run of that entry alone
fails in 58.03 sec. The tsan suite shows entry 146 as `***Failed` in 38.50 sec
at line 168, and a run alone fails in 56.50 sec. Both legs print the same three
lines. The first names the scenario
`a-topdown-level-names-the-block-of-a-hunter-run` and reports `status 1` from
the test binary. The second is a long line, so this record cites it instead of
quoting it: line 387 of the sanitize suite log and line 170 of the tsan suite
log, and it ends with the tags `FR-016` and `FR-034`. The third reads in full:

```text
HARNESS HUNT CLI TEST FAIL: the re-executed scenario passes
```

No sanitizer line stands next to these three lines, so the entry fails on its
own assertion.

### `harness_limit_disclosure_test`

This entry reaches no test process on either leg. It is `Not Run` on both:
entry 73 on sanitize and entry 76 on tsan. Alone, each answers
`***Not Run   0.00 sec`, and CTest first prints `Could not find executable`,
then `Unable to find executable:`. Each rerun then lists where it looked, and
the four forms it names first are `Release`, `Debug`, `MinSizeRel` and
`RelWithDebInfo`; `Deployment` and `Development` follow, each under the tree's
own `test` directory. No file of that name stands in any of them, so those
lines name searched paths and not found binaries.

A compiler diagnostic explains the gap, and no sanitizer is involved. Two
lines of one test source stop the build, and both carry the clang-only tag
`[-Werror,-Wunused-but-set-global]`: one reads
`error: variable 'gOpened' set but not used` at `:131:5`, and the other reads
the same shape for `gClosed` at `:132:5`. The file is
`test/source/harness_limit_disclosure_test.cpp`, and this lane did not touch
it. The command below answers empty, so the source stands byte-identical at the
head and at `origin/master`, and `test/CMakeLists.txt` differs only by our own
`counters_limit_lifecycle_test` block:

```text
git diff fba814d2 02892b57 -- test/source/harness_limit_disclosure_test.cpp
```


The compiler is the whole cause, and no preset chose it. No configure preset
of `CMakePresets.json` names a compiler or an `environment` key, so a fresh
cache would pick the host default. My two trees reuse the directories the
old-base record built, and `results/sanitizers.md:11` of that record states
that both presets select `/usr/bin/clang++`. A reconfigure keeps a cached
compiler instead of detecting one again, so each tree's `CMakeCache.txt` reads
`CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++`, and neither configure log of
this wave holds a `compiler identification` line. The flag list then turns the
warning into a stop: the `flags-gcc-clang` preset ends its `CMAKE_CXX_FLAGS`
with `-Werror`, which both sanitizer presets inherit through `ci-linux`.
GCC carries no such diagnostic, which is why the trees built on GNU hold this
entry. The plain tree at the same head names `/usr/bin/c++` with the compiler
id `GNU`, holds a built copy of the target, and a single-entry run there, at my
parent's direction, answers
`1/1 Test #78: harness_limit_disclosure_test ....   Passed    0.34 sec`.

The entry reaches CI, so this gap is not a host-only curiosity. The `sanitize`
job stands at `.github/workflows/ci.yml:147` and sets `CXX: clang++` at `:153`,
and the `tsan` job stands at `:230` and sets `CXX: clang++` at `:236`. Each
job installs the Ubuntu `clang` package, at `:182` and at `:265`, a release far
below 23, and each runs the whole suite with `-j "$(nproc)"` at `:221` and at
`:300`. This record does not decide what that runner's clang version
does with these two lines of source. `T028` at
`specs/050-limit-table-lifecycle/tasks.md:524` pushes the branch, opens the
pull request, and reads the run that event starts, and that reading settles it.

### `harness_context_barrier_test`

This entry fails only on the tsan leg and only inside the suite. Line 380 shows
entry 127 as `***Failed` in 1.03 sec, and line 382 says the SPIN participant
never sleeps inside the barrier, because its voluntary-switch counter stands
flat across the round, tagged `D-03`, `Q-3` and `FR-014`. Line 383 adds
`HARNESS CONTEXT BARRIER TEST FAIL: the scenario passes: spinWait`. The same
entry alone answers `Passed` in 0.15 sec, and no `ThreadSanitizer` line stands
near any of this, so the arbiter of a race says nothing about it.

## The base comparison

`results/sanitizers.md` in this same directory is the old-base record, and its
line 3 names the head it measured: `a41fc03441fdcc0de0b351e7e86d31f374c3eaf2`,
which stands before this rebase. Its two verdict lines stand verbatim below, at
`:52` and `:88`, each with the line that follows it.

```text
No `AddressSanitizer`, `LeakSanitizer`, or `SUMMARY:` line stands in the log,
and a search for `runtime error` answers nothing. Therefore the address
```

```text
The log holds no `WARNING: ThreadSanitizer` line and no `SUMMARY:` line, so the
arbiter of a race reports none on this head. A plain search for the phrase
```

Each quote stops mid-sentence, because that record wraps a sentence over
several lines. Its first sentence closes at `:54` with `report nothing here`,
and its second closes at `:90` with `answers no matching line`. This rebase
record reaches the same verdict from its own logs, and the eight zero counts
above are that reading.

The entry counts move in one clean way. The old-base legs carried 177 entries
on sanitize and 178 on tsan, and that record states at `:36` that its plain tree
carried 182. At this head the plain tree states 186, and my legs carry 181 and
182. Each of my trees therefore sits four entries above its old-base twin, the
same four the rebase adds to the plain tree, so neither guard dropped a new
entry.

The failing entries differ. That record fails one entry per leg, and it is
`harness_hunt_cli_test`, which fails here too with the same scenario name and
the same two messages. This record adds two readings to it:
`harness_limit_disclosure_test` stands `Not Run` on both legs, and
`harness_context_barrier_test` fails the tsan suite yet passes alone.

The suite command differs, and that explains the new tsan failure and nothing
else. That record ran each leg with `-j 1` and with `--no-tests=error`, so one
entry ran at a time. This brief fixes `-j 20`, so 20 entries share 20 cores and
one host with three other waves. `harness_context_barrier_test` counts voluntary
context switches inside a barrier, so a loaded host bears on it, and its alone
reading answers `Passed`.

## What no reading here claims

- The four zero counts state that no report line stands in the logs of two runs
  on one host. They state nothing about a bug that no sanitizer was watching,
  and nothing about an entry that no run reached.
- No claim stands that this code carries no race. The thread sanitizer saw what
  these 176 entries exercised here, at one level of load, on one kernel.
- No claim stands about coverage, clang-tidy, cppcheck, or the CI matrix. Other
  waves of this rebase measure those gates in their own trees.
- No claim stands about what the CI runner's clang does with that test source.
  The cause on this host rests on two readings of this wave: the diff of that
  file between `fba814d2` and `02892b57` answers empty, and each tree caches
  `/usr/bin/clang++`. The runner's side waits on the `T028` reading, and no
  figure here runs ahead of it.
- No claim stands about the five `dbc_matrix_*` rows on the sanitize tree, or
  about the four entries that own the global allocation operators on the tsan
  tree. Neither tree registers them, so neither produced a reading for them.
- No claim stands about a GNU build of the whole suite. Both legs ran
  `/usr/bin/clang++`, and the one GNU reading here is the single-entry run the
  parent directed.
- No timing claim stands. Three other waves build and run on this host
  throughout, so no wall time above is an idle-host figure and none is a
  regression signal.
- The prose verdict of this record changed with its own tracking state, and
  both readings stand. The gate of this lane reads tracked files only. When
  this file was new and untracked, it answered the first line below and called
  nothing clean. After the lane staged the file, it answered the second line,
  and `codespell` 2.4.3 answered with no finding on both runs. The untracked
  reading is the one that carries no verdict, and it reads in full:

  ```text
  prose-lint: 0 sources, 0 units examined, 0 findings, 0 skipped
  ```

  The staged reading states no finding and no skip, and its unit count grows
  with the length of this file, so a reader should re-run the command for the
  count that stands today.
