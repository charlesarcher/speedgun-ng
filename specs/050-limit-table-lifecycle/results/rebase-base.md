# Rebase base: the figures at `origin/master` `02892b57`

`specs/050-limit-table-lifecycle/results/base-state.md` states the lane's base
figures at `858c55d9`, the commit this lane branched from. The pull request now
stands on `origin/master` `02892b57`, so this record repeats four of those
readings there. It gives the suite reading, the interface count, the standing
base fault, and the version lines, each beside the command that printed it.
Every command ran on this host on 2026-11-06 in this wave, and each figure
names the log of its own run.

## What this record measures

Three trees carry the work, and no figure comes from a build tree inside the
repository worktree at `$HOME/code/speedgun-ng`. That worktree kept
`HEAD` at `fba814d2` on `050-limit-table-lifecycle` through every command here.
Every `path:line` citation below names a file under `/tmp/050-base-master`, and
one citation stands outside it: `base-state.md:31` names a lane record inside
the repository worktree.

The base source is `/tmp/050-base-master`, made by the one authorized write:

```text
git worktree add --detach /tmp/050-base-master 02892b57
```

The suite build is `/tmp/050-base-build` and the gate build is
`/tmp/050-base-dev`. The two configure commands, verbatim:

```text
cmake -S /tmp/050-base-master -B /tmp/050-base-build --preset=ci-rocky
cmake -S /tmp/050-base-master -B /tmp/050-base-dev --preset=ci-ubuntu \
    -DPython3_EXECUTABLE=/usr/bin/python3
```

The second command stands on two lines here, and one backslash joins it.

`CMakeCache.txt` in each tree states what those presets settled. The suite tree
reads `CMAKE_BUILD_TYPE=Release` with `SPEEDGUN_DBC_MATRIX=ON`, and the gate
tree reads `CMAKE_BUILD_TYPE=Release` with `clang-tidy` and `cppcheck` wired in
and `Python3_EXECUTABLE` pinned to `/usr/bin/python3`. Both trees configure
with the `GNU 16.2.1` compiler, and `cmake --version` answers `4.4.5`. The host
is `kennykiller`, the kernel is `7.3.1-1-cachyos`, and `nproc` answers `20`.

A detached worktree checks out no submodule, and the first configure failed for
that reason: `import_autotools_submodule(hwloc_vendor): the vendored tree
'external/hwloc' is missing or empty`. No authorized command populates a
submodule, so this wave copied the six vendored working trees from the
repository worktree with `rsync -a --exclude=.git`. `git ls-tree` states the
same six gitlink ids at `02892b57` and at `fba814d2`, so the copy carries the
submodules the base names. The configure then completed with no error.

Three other waves built on this host while these commands ran, so no wall time
in this record is an idle-host figure. Each one is a same-host reading under
that load, and the suite timing in particular sits on the high side for that
reason.

## The suite reading

```text
ctest --test-dir /tmp/050-base-build -j 20 --output-on-failure
```

`ctest --test-dir /tmp/050-base-build -N` states `Total Tests: 185` at the base
tree. One full suite run, the command line above, closes with
`99% tests passed, 2 tests failed out of 185` and
`Total Test time (real) = 130.04 sec`, and `ctest` exits 8. The shell `time`
reading stands at `/tmp/050-base-ctest-newbase-time.txt` and answers
`2m10.082s` for that run, which ran under the load three other waves put on
this host. `/tmp/050-base-ctest-newbase.log` holds the run end to end. The
same run lists three entries that did not run, all three `Skipped`:

```text
The following tests did not run:
	 47 - counters_pmu_test (Skipped)
	 62 - counters_overhead (Skipped)
	183 - harness_topdown_run_test (Skipped)
```

The run names two failed entries: `82 - views_test (Failed)` and
`149 - harness_hunt_cli_test (Failed)`. Each one was re-run alone, and each
message below comes from that single-entry run.

### `views_test` alone

`ctest --test-dir /tmp/050-base-build -R '^views_test$' --output-on-failure`
exits 8 and prints one line of substance in 0.00 sec, logged at
`/tmp/050-base-views-rerun.log`:

```text
SPEEDGUN-NG TEST FAIL: pass: the root CMakeLists.txt can be read
```

`rootCMakeText()` walks up at most eight parent directories from the current
directory and returns the first `CMakeLists.txt` that contains `project(`
(`test/source/views_test.cpp:57-68`), and the check at
`test/source/views_test.cpp:313` asks that the text be non-empty. CTest starts
the entry in the build tree: `ctest` in `-N --verbose` mode prints
`82: Working Directory: /tmp/050-base-build/test`, and no ancestor of that
directory holds a root `CMakeLists.txt` that declares a project. Running the
same binary from `/tmp/050-base-master`, with the fixture directory as its
argument, prints `views_test PASS` and exits 0
(`/tmp/050-base-views-cwd2.log`). This entry measures where the build tree
stands relative to the source tree, so it speaks about this tree layout and
says nothing about the code at `02892b57`. The lane's normal layout puts the
build tree inside the source tree, within two hops of the root file.

## The interface count

```text
cmake --build /tmp/050-base-dev --target dbc-gate
```

The command exits 0 and prints two counts, logged at
`/tmp/050-base-dbc-gate.log`:

```text
Running DBC documentation and pairing gates
doc-gate: 348 interfaces, 0 gaps (/tmp/050-base-dev/dbc-gate/doc-matrix.json)
pair-gate: 348 interfaces, 0 gaps (/tmp/050-base-dev/dbc-gate/pair-matrix.json)
```

Both gates count `348` interfaces and state `0` gaps at the base commit. The
gate tree finds its interpreter at the pinned path, and its configure log
records `Found Python3: /usr/bin/python3` at version `3.14.9`.

## The standing base fault

```text
ctest --test-dir /tmp/050-base-build -R '^harness_hunt_cli_test$' --output-on-failure
```

`harness_hunt_cli_test` fails at the new base. The single-entry run above
selects entry 149, prints `***Failed` after 44.56 sec, and `ctest` exits 8 with
`0% tests passed, 1 tests failed out of 1`. `/tmp/050-base-hunt-rerun.log`
holds that run. The entry prints two failing assertions and nothing else:

```text
a-topdown-level-names-the-block-of-a-hunter-run: status 1 from "/tmp/050-base-build/test/harness_hunt_cli_test":
HARNESS HUNT CLI TEST FAIL: one run of one benchmark answers one block, so the named level admits no second block of the companion (FR-016, FR-034)

HARNESS HUNT CLI TEST FAIL: the re-executed scenario passes
```

`awk` reads both assertion sites from the base copy of the source, and each
quoted line below carries its own line number:

```text
3133:  check(countOf(named, "topdown: level=") == 1,
3134:        "one run of one benchmark answers one block, so the named level admits "
3135:        "no second block of the companion (FR-016, FR-034)");
3682:    check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
3683:          "the re-executed scenario passes");
```

The first site sits inside `scenarioTopdownLevelNamesTheBlockOfAHunterRun()`,
which opens at `test/source/harness_hunt_cli_test.cpp:3121`, and its check
counts the `topdown: level=` headers of a run that names two hunt addresses and
passes `--topdown=2`. The second site sits in the driver loop that re-execs one
scenario per process, and its `fprintf` opens the block above. That call sits at
`test/source/harness_hunt_cli_test.cpp:3675`. Both messages therefore
carry one fault: the child fails the one-block check, and the parent reports the
child's non-zero exit.

## The version lines

`awk` reads the three lines this record names from the base `CMakeLists.txt`:

```text
70:    VERSION 0.32.0
155:    SOVERSION 7
431:    SOVERSION 21
```

Line 70 sits in the `project(` block that opens at `CMakeLists.txt:5`. Line 155
sits in the `set_target_properties` block for `speedgun-ng_speedgun-ng`, which
opens at `CMakeLists.txt:116`. Line 431 sits in the block for
`speedgun-ng_harness`, which opens at `CMakeLists.txt:350`. The version block of
`test/source/views_test.cpp:312-322` asks those same three numbers, and the
binary run from the source tree passed that block.

## The disclosure entry under the sanitize preset at the base

The rebase sanitizer wave reports `harness_limit_disclosure_test` as `Not Run`
in a `build/sanitize` tree at the lane head, and the lane needs to know whether
`02892b57` carries the same fault. The base carries the entry: the source
`test/source/harness_limit_disclosure_test.cpp` stands there, and
`test/CMakeLists.txt:856` registers it with `add_test`. The verdict turns on the
C++ compiler, so this section holds two readings and names the compiler of each.
No preset names a C++ compiler, so a bare `cmake --preset=ci-sanitize` takes the
host default `/usr/bin/c++`, which is GCC 16.2.1, and GCC states no
`-Wunused-but-set-global`. The sanitizer wave reports that its tree at the head
builds through `/usr/bin/clang++` at 23.1.4, so the second reading pins that
compiler.

### The host-default reading: GCC 16.2.1

The commands, run from `/tmp/050-base-master`:

```text
cmake --preset=ci-sanitize
cmake --build build/sanitize -j 20 --target harness_limit_disclosure_test
ctest --test-dir /tmp/050-base-master/build/sanitize -R '^harness_limit_disclosure_test$' --output-on-failure
```

`cmake --preset=ci-sanitize` exits 0 and writes `/tmp/050-base-master/build/sanitize`,
the `binaryDir` its preset states. Its cache reads `CMAKE_BUILD_TYPE=Sanitize`
and `CMAKE_GENERATOR=Unix Makefiles`, a single-configuration generator, and its
log records `The CXX compiler identification is GNU 16.2.1`. The whole-tree
build `cmake --build build/sanitize -j 20` stopped part way, and the cause sits
in this host: `/tmp` is a 16 GB tmpfs that stood at 100 percent full. The log
records `final link failed: No space left on device` in ten link steps, and an
assembler assertion stopped three compiles, thirteen targets in all,
`harness_limit_disclosure_test` among them. That log ends mid-line with no exit
line, and this wave reads the truncation as the same full-disk condition. This
wave then removed its own suite tree `/tmp/050-base-build`, which freed 4.0 GB,
and the single-target build linked the entry in 10.798 s and exited 0. The entry
run then exits 0 and states a pass:

```text
    Start 72: harness_limit_disclosure_test
1/1 Test #72: harness_limit_disclosure_test ....   Passed    1.41 sec

100% tests passed out of 1
```

`ctest -N --verbose` names one executable for it, at
`/tmp/050-base-master/build/sanitize/test/harness_limit_disclosure_test`, with
its working directory beside it. This reading says nothing about the warning the
head wave meets, because GCC carries no `-Wunused-but-set-global` to raise.

### The pinned reading: clang 23.1.4

The commands, run from `/tmp/050-base-master`:

```text
CXX=/usr/bin/clang++ cmake --preset=ci-sanitize
cmake --build build/sanitize --target harness_limit_disclosure_test
ctest --test-dir build/sanitize -R '^harness_limit_disclosure_test$' --output-on-failure
```

The configure exits 0, its cache reads `CMAKE_CXX_COMPILER=/usr/bin/clang++`,
and its log records `The CXX compiler identification is Clang 23.1.4`. The C
compiler stays GNU 16.2.1 there, and only the vendored hwloc warnings come from
it. The single-target build exits 2 after `2m43.480s` of wall time and prints
two errors, held at `/tmp/050-base-sanitize-clang-build.log`:

```text
/tmp/050-base-master/test/source/harness_limit_disclosure_test.cpp:131:5: error: variable 'gOpened' set but not used [-Werror,-Wunused-but-set-global]
/tmp/050-base-master/test/source/harness_limit_disclosure_test.cpp:132:5: error: variable 'gClosed' set but not used [-Werror,-Wunused-but-set-global]
2 errors generated.
```

`awk` reads the two source lines those errors name, and the base holds both:

```text
131: int gOpened = 0;
132: int gClosed = 0;
```

No binary follows the failed compile, so the entry run ends `Not Run` and exits
8, held at `/tmp/050-base-sanitize-clang-ctest.log`:

```text
Unable to find executable: /tmp/050-base-master/build/sanitize/test/harness_limit_disclosure_test
1/1 Test #72: harness_limit_disclosure_test ....***Not Run   0.00 sec

0% tests passed, 1 tests failed out of 1

The following tests FAILED:
	 72 - harness_limit_disclosure_test (Not Run)           quick
```

### What the two readings support

The base carries the same two diagnostics under clang. The two set-but-unused
globals stand at `test/source/harness_limit_disclosure_test.cpp:131` and `:132`
at `02892b57`, and the preset flag set ends in `-Werror`, so clang 23.1.4 turns
that warning into the error that stops the target and leaves the entry with no
executable. The `Not Run` verdict the sanitizer wave meets therefore has a base
that reproduces it, and this record states nothing about which change moves it.

One detail of that verdict needs care. CTest prints its whole search list when
an executable is missing, and that list already names `Release`, `Debug`,
`MinSizeRel`, `RelWithDebInfo`, `Deployment` and `Development` under `test/`.
A base tree holds none of those directories, because a single-configuration
generator creates none of them, and `ls -d` answers so for each:

```text
ls: cannot access 'build/sanitize/test/Development': No such file or directory
ls: cannot access 'build/sanitize/test/Deployment': No such file or directory
ls: cannot access 'build/sanitize/test/RelWithDebInfo': No such file or directory
```

## What no reading here claims

No figure here measures the lane head `fba814d2` or any commit between the two
bases, because every command ran in `/tmp/050-base-master` at `02892b57`.

No figure here restates or contests `base-state.md`. This wave ran nothing at
`858c55d9`, so the gap between that record's `Total Tests: 181`
(`specs/050-limit-table-lifecycle/results/base-state.md:31`) and the `185`
stated here gets no cause from this record.

No figure here covers a whole sanitizer run, a coverage run, `clang-tidy`, or
`cppcheck`. The sanitize work reached one entry and its own build, and
`dbc-gate` answers the two counts above with no verdict on contract quality.

No figure here describes a CI job, and no wall time here is an idle-host
figure, because three other waves built on this host through every run.

No figure here names the cause of the `harness_hunt_cli_test` fault. This
record states where its two assertions stand at the base, and the lane owns the
rest of that question.

`views_test` states nothing about the code at the base, because its one message
comes from where the build tree stands relative to the source tree.

The six vendored trees this wave copied carry the gitlink ids `git ls-tree`
states at `02892b57`, and no reading here touched a vendored tree at any other
revision.

## Worklog: the trees this wave made and what stands now

The one git write this wave ran is the authorized `git worktree add` line of the
first section. `git rev-parse HEAD` in `$HOME/code/speedgun-ng` still
answers `fba814d2` on `050-limit-table-lifecycle` after every command above, and
`git worktree list` names that worktree plus `/tmp/050-base-master` at
`02892b57` in detached HEAD. This wave removed no worktree.

Two trees stand, on the orchestrator's instruction of 2026-11-06. The analyzer
wave builds its base pair from `/tmp/050-base-master`, so that detached tree
keeps its checkout and its copied vendored trees. The gate tree
`/tmp/050-base-dev` keeps its cache and its `/tmp/050-base-dev/dbc-gate/`
output, so a reader can open both matrix files named above.

Two trees are gone. `/tmp/050-base-build`, the suite tree, went once the suite
reading and both single-entry reruns were captured, and it gave back 4.0 GB to
the full tmpfs. `/tmp/050-base-master/build/sanitize` went once the GCC reading
was captured, and it gave back 11.6 GB. That path holds a fresh tree now,
because the clang reading needed one: it keeps one configure and one target at
402 MB, and `ls` finds no binary under `test/`, because that compile fails. A
later wave can read the same two diagnostics there in about three minutes.

## Verification of this record

`test -s` on this file answers non-empty. The prose gate discovers its
candidates with `git ls-files`, so an unstaged file stays outside its view, and
the first run of the pinned command over this path reported no source at all.
`git ls-files` names this file among its candidates now, and the same command
answers:

```text
prose-lint: 1 sources, 286 units examined, 0 findings, 0 skipped
```

`/tmp/050-tools-venv/bin/codespell` at 2.4.3 reads this file with the
repository's `.codespellrc`, prints nothing, and exits 0. The same command
flagged a misspelling this wave added to the file on purpose and then removed
it, so the clean answer reflects the text.

Eleven lines of this record pass 80 columns. Each one sits inside a fenced
block and holds a command or program output that cannot wrap without changing
what it quotes. All scratch output of this wave stayed in `/tmp`, and no file
inside the repository worktree changed except this one.
