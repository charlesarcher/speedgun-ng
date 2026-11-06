# T020: the two sanitizer legs (SC-001)

Both legs ran on `a41fc03441fdcc0de0b351e7e86d31f374c3eaf2`, the head that
`git rev-parse HEAD` answered at 20:06 before the first command below. That
command read the same sha again at 21:12, so one head stands under every figure
in this record.

## The host toolchain

The host is `kennykiller`, Linux 7.3.1-1-cachyos on x86_64 with 20 cores, and
both presets select `/usr/bin/clang++`. That compiler reports clang version
23.1.4 for the target `x86_64-pc-linux-gnu`, and its sanitizer runtimes sit
under `/usr/lib/clang/23`. `cmake --version` answers 4.4.5, `ld --version`
answers GNU Binutils 2.47, and ccache fronts every compile of both trees. GCC
16.2.1 stands on the host too, and no command in either leg invokes it.

## The `ci-sanitize` leg

The leg runs the three commands below in this order, each one alone on the
host:

```text
cmake --preset=ci-sanitize
cmake --build build/sanitize -j 20
ctest --test-dir build/sanitize -j 1 --output-on-failure --no-tests=error
```

| Step | Exit code | Log file |
| --- | --- | --- |
| configure | 0 | `/tmp/050-sanitize-cfg.log` |
| build | 0 | `/tmp/050-sanitize-build.log` |
| suite | 8 | `/tmp/050-sanitize-ctest.log` |

`ctest --test-dir build/sanitize -N` answers `Total Tests: 177`, and the suite
log holds 177 result lines, so this leg carries 177 entries. The plain tree at
the head carries 182 entries, as `base-state.md:216-218` records, so the
sanitizer tree registers 5 fewer. The 177 entries of this leg split into 173
passed, 3 skipped and 1 failed. Line 362 reads
`99% tests passed, 1 tests failed out of 177`, and `ctest` exits 8 after it. The
build step reports no error line in its 177 log lines, and its last line reads
`[100%] Built target harness_topdown_run_test`.

One entry fails: entry 141, `harness_hunt_cli_test`, and line 283 of the suite
log shows it as `***Failed` in 57.89 sec. Line 284 names the scenario
`a-topdown-level-names-the-block-of-a-hunter-run`, and lines 285 and 287 carry
its two assertion messages. The base record at `base-state.md:73` and `:81`
carries the same two messages, and it locates them at
`test/source/harness_hunt_cli_test.cpp:3129` and `:3464`. The three entries that
do not run are `counters_pmu_test`, `counters_overhead` and
`harness_topdown_run_test`, marked Skipped at lines 85, 109 and 356.

No `AddressSanitizer`, `LeakSanitizer`, or `SUMMARY:` line stands in the log,
and a search for `runtime error` answers nothing. Therefore the address
sanitizer and the undefined behaviour sanitizer report nothing here, and the one
failure rests on the entry's own assertion. The suite ran from 20:21:03 to
20:29:38 local time, and CTest reports 515.14 seconds of test time for it.

## The `ci-tsan` leg

The second leg repeats those three steps on the thread sanitizer tree, one
after the other:

```text
cmake --preset=ci-tsan
cmake --build build/tsan -j 20
ctest --test-dir build/tsan -j 1 --output-on-failure --no-tests=error
```

| Step | Exit code | Log file |
| --- | --- | --- |
| configure | 0 | `/tmp/050-tsan-cfg.log` |
| build | 0 | `/tmp/050-tsan-build.log` |
| suite | 8 | `/tmp/050-tsan-ctest.log` |

`ctest --test-dir build/tsan -N` answers `Total Tests: 178`, and the suite log
holds 178 result lines. The 178 entries of this leg split into 174 passed, 3
skipped and 1 failed. Line 364 reads
`99% tests passed, 1 tests failed out of 178`, and `ctest` exits 8 after it. The
build step again reports no error line, and its last line reads
`[100%] Built target harness_topdown_report_test`.

The failing entry here is `harness_hunt_cli_test` again, entry 142, and line 285
shows it as `***Failed` in 55.59 sec. Lines 286 to 289 carry the same scenario
name and the same two assertion messages that the sanitize leg prints, word for
word. The three entries that do not run are again `counters_pmu_test`,
`counters_overhead` and `harness_topdown_run_test`, at lines 93, 117 and 358.

The log holds no `WARNING: ThreadSanitizer` line and no `SUMMARY:` line, so the
arbiter of a race reports none on this head. A plain search for the phrase
`data race` over that log answers no matching line. The suite ran from 20:30:23
to 20:42:53 local time, and CTest reports 750.29 seconds of test time for it.

## Why the two trees hold different entry counts

The tsan tree registers five entries that the sanitize tree lacks, and they are
the `dbc_matrix_*` rows. The ci-tsan preset sets `SPEEDGUN_DBC_MATRIX`, which
`test/CMakeLists.txt:158` guards those rows with, and ci-sanitize leaves it
unset. The sanitize tree holds four entries that the tsan tree lacks:
`counters_noalloc_test`, `harness_capture_test`,
`harness_jitter_probe_alloc_test` and `harness_argument_test`. Those four stand
under a guard at `test/CMakeLists.txt:330` that skips them whenever the release
flags carry `fsanitize=thread`, with copies at `:751`, `:1001` and `:1372`. The
thread sanitizer runtime owns the same global allocation operators those four
targets replace, which is why the guard exists.

## The two legs against CI run 41946446748

`base-state.md:89-107` records the base verdicts of that run: the `sanitize` job
fails one entry, `harness_hunt_cli_test`, and the `tsan` job passes every entry.
The sanitize leg here fails exactly the entry that the base `sanitize` job
names, so no entry the base passed fails on that leg.

The tsan leg answers differently, because the base `tsan` job passes every entry
while this leg fails `harness_hunt_cli_test`. That entry fails on this host in
the uninstrumented tree at the base commit, and `base-state.md:38-39` records
both base readings as 180 of 181 passed. The plain tree at the head fails the
same entry with the same two messages, as `base-state.md:59-71` records. The
record therefore reads the tsan difference as host-dependent, because both base
readings on this host fail the same entry before this lane starts.

Both legs stand on this record, and D-6 keeps the thread sanitizer the arbiter
of a race. Neither leg reports a race or a memory error, so the two readings
agree about the code this lane changes.

## Why this reading replaced an earlier one

A first leg ran from 20:08 to 20:17 with a second agent's suite in parallel, and
both suites wrote the same log path. That file now sits aside under a
`.contaminated` name, and no figure above rests on that reading. The two legs
above ran one after the other, with nothing else of mine in between. A
`pgrep -af 'cmake --build|ctest --test-dir'` check before each leg answers no
other build or suite process on the host. `harness_jitter_cli_test` is a timing
entry of the suite, and it passes on both legs of this record. It takes 45.32
sec at line 173 of the sanitize log and 125.73 sec at line 177 of the tsan log.
`harness_graph_report_test` passes in 9.88 sec at sanitize line 348 and in 5.37
sec at tsan line 350. Those two entries were the extra failures of the
shared-host run, so they carry no reading of the change.
