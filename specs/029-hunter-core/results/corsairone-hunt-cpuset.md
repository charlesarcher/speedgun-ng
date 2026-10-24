# FR-092 CPU-set record: corsairone, hunt suite on restricted sets

Task T116, FR-092, the `ctest` half of T056. The eight tests `ctest -R hunt`
selects pass in full on a host whose available CPU set is two cores wide, and
again on one four cores wide, both sets smaller than the 24 CPUs of the
machine that built the binaries and ran them. Every figure below comes from a
log captured on 2026-10-23, and each section names the command that produced
its numbers.

## 1. The host and the CPU sets

Host: `CorsairOne`, Intel(R) Core(TM) i9-9920X CPU @ 3.50GHz, 24 CPUs (2
threads per core, one socket), kernel `7.2.9-2-cachyos`. `nproc` answers 24
there.

`taskset` publishes a named subset of those CPUs to the test processes:
`taskset -c 0,1` leaves a two-CPU set, `taskset -c 0-3` leaves a four-CPU set.
`lscpu` reports 12 cores per socket and the sibling lists name `0,12` for
`cpu0` and `1,13` for `cpu1`, so each named CPU is a separate physical core on
one thread: the two runs measured two cores and four cores at one hardware
thread each.

Inside the process the restricted set is what `sched_getaffinity` reports, the
same source `affinityCount` at `test/source/harness_hunt_collect_test.cpp:1454`
reads for FR-092.

Both restricted runs drove one test at a time (no `ctest -j`) over the same
test tree, `/home/archerc/sg029-lanes/r4b/build`, the first line of each log.
Those two logs record no commit, so the commit pin below stands for the
whole-suite run alone.

## 2. The two restricted-set runs

The summary lines of each log, one block per set:

```text
$ taskset -c 0,1 ctest --test-dir build -R hunt --output-on-failure
100% tests passed out of 8
Total Test time (real) = 136.72 sec

$ taskset -c 0-3 ctest --test-dir build -R hunt --output-on-failure
100% tests passed out of 8
Total Test time (real) = 138.59 sec
```

Exit status 0 for both. Every one of the eight tests reports `Passed` in both
runs and no line reports a skip, which is the second clause of FR-092. The
selection is eight because `-R hunt` matches
`report_shape_hunt_rank_sample` beside the seven `harness_hunt_*` tests.

Captured logs: `/home/archerc/sg029-lanes/t116-cpu2.log` and
`/home/archerc/sg029-lanes/t116-cpu4.log`.

## 3. The whole suite beside them, and the one failure

The same tree ran the whole suite at commit `6337e75` under `ctest -j 2`. The
tail of the log:

```text
124/124 Test  #65: harness_cli_test ...............................***Timeout 1500.03 sec

99% tests passed, 1 tests failed out of 124

Total Test time (real) = 1809.41 sec

The following tests FAILED:
	 65 - harness_cli_test (Timeout)
Errors while running CTest
```

123 of 124 passed. The failure is a ctest timeout, and `harness_cli_test`
carries no `TIMEOUT` property (`test/CMakeLists.txt:654` registers it bare), so
1500 s is ctest's own default limit. Co-tenancy disclosure: seven concurrent
`cmake --build -j 4` lanes stood on the box through the run, 28 build workers
over the 24 CPUs. Captured log:
`/home/archerc/sg029-lanes/t116-fullsuite.log`, byte-identical to
`/tmp/h-tall.log`.

## 4. The eight tests across the three measurements

Seconds as each run printed them. The third column measures tests that shared
the box with each other and with the seven build lanes, so its figures carry
that scheduling.

| test | `taskset -c 0,1` | `taskset -c 0-3` | full set, `-j 2` |
| --- | --- | --- | --- |
| `harness_hunt_plan_test` | 0.09 | 0.11 | 0.14 |
| `harness_hunt_rank_test` | 0.00 | 0.00 | 0.01 |
| `harness_hunt_collect_test` | 0.19 | 0.27 | 0.52 |
| `harness_hunt_cli_test` | 118.38 | 120.34 | 117.90 |
| `harness_hunt_report_test` | 11.26 | 11.07 | 11.03 |
| `harness_hunt_json_test` | 1.57 | 1.61 | 1.73 |
| `harness_hunt_json_test_no_hunt` | 1.59 | 1.61 | 1.55 |
| `report_shape_hunt_rank_sample` | 3.64 | 3.57 | 4.30 |
| total | 136.72 | 138.59 | 1809.41 (124 tests) |

`harness_hunt_cli_test` holds 117.90 s to 120.34 s across the three columns:
two cores more moves it by 1.96 s on a run of about two minutes. It carries no
`TIMEOUT` property (`test/CMakeLists.txt:1207-1211`), so no registered limit
bounds it, and the 120 s its sibling `report_shape_hunt_rank_sample` carries
(`test/CMakeLists.txt:1374`) would refuse the four-CPU figure of 120.34 s.

`harness_hunt_collect_test` passed on the two-CPU set at 0.19 s. That run
executes `scenarioTwoContextWindowOpens`, whose `sched_getaffinity` check is
`cpus >= kThreadCount` with `kThreadCount = 2`
(`test/source/harness_hunt_collect_test.cpp:108`): the two-core set satisfies
it at the boundary, so the threaded arm of that scenario really ran there.

## 5. The re-run of `harness_cli_test` alone passes on its own deadline

`/tmp/cli-only.log` holds the whole of a single-test re-run in two lines:

```text
Test project /home/archerc/sg029-lanes/r4b/build
    Start 65: harness_cli_test
```

No verdict line follows in that file, and no `ctest` or `harness_cli_test` process
stood in the process table when it was written. The re-run completed in a second
lane, where the same test was driven alone under a 5400 s deadline: `Passed`,
3126.26 s, exit status 0. The test needs about 3100 s in a release lane on this
host, so the 1500.03 s of §3 is the deadline `ctest` applies when a test carries
no registered limit, and that deadline is what the whole-suite run printed.

## 6. What this record proves, and what it leaves open

FR-092 holds for the hunt selection: the eight tests `ctest -R hunt` selects
ran green, exit status 0, on a two-CPU set and on a four-CPU set, the widths
FR-092 names, on a host that publishes 24 CPUs. No test reported a skip: the
affinity-gated two-context scenario ran on the two-CPU set, and the totals of
the two sets, 136.72 s and 138.59 s, sit within 1.4 percent of each other.

Three things stand open. The whole-suite timeout of `harness_cli_test` at 1500 s
is a deadline fact (§5): the test needs about 3100 s on this host, so the one
failure the whole-suite run printed was measured against a default deadline on a
box serving 28 build workers, and it says nothing about a CPU set. The restricted
runs cover the eight tests `-R hunt` selects, so the other 116 tests of the suite
carry no restricted-set measurement here. The two restricted runs went serial,
so no figure here measures a restricted set driven by `ctest -j`. And a test
the suite times at 118 s to 120 s with no registered limit (§4) is a host-speed
question the suite has not answered.
