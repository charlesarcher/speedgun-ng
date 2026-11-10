# Base Readings: specs/051-raw-events (T001, T002)

**Mode**: read-only over the checkout. `build` carries the `ci-rocky`
configuration when the task ends, and this document is the only write.
**Base**: `0fce6651`, the specs/049 squashed head and the parent of
this lane's documents.

```
git log --oneline -1 0fce6651
0fce6651 Docs: Remove names and user paths from old records
```

The branch head at the read is `739323a6`, so the source tree carries
the base tree plus this lane's documents.

```
git rev-parse HEAD
739323a6eaaa135de3afac75beacc4854e51927b
```

Each block below holds one command line, then the output that command
printed at this base.

## The daily loop at the lane base (T001)

```
cmake --preset=ci-rocky
-- Configuring done (0.1s)
-- Generating done (0.2s)
-- Build files have been written to: /home/archerc/code/speedgun-ng/build
```

The tree held a `ci-ubuntu` configuration before this read: its cache
named `CMAKE_CXX_CLANG_TIDY` and `CMAKE_CXX_CPPCHECK`, and a preset
switch keeps a cache variable an earlier preset set. The build
discipline section of tasks.md names the purge for exactly this
situation, so it ran between the configure and the build.

```
cmake -B build -U CMAKE_CXX_CLANG_TIDY -U CMAKE_CXX_CPPCHECK
-- Configuring done (0.1s)
-- Generating done (0.1s)
-- Build files have been written to: /home/archerc/code/speedgun-ng/build
```

Each of `grep -c 'CXX_CLANG_TIDY' build/CMakeCache.txt` and the
`CXX_CPPCHECK` form answers `0` after that reconfigure.

```
cmake --build build -j 10
[  0%] Built target quill
[  2%] Built target hwloc_vendor-ep
[  2%] Linking CXX executable counters_trap_noguard_fixture
[  5%] Built target zlibstatic
...
[100%] Built target docs_topdown
[100%] Built target report_view
[100%] Built target docs_counter_probe
```

The build exits 0 in 3.2 seconds and prints 0 `Building CXX object`
lines: every object was already current at this base, so the work was
the link steps and the vendored archive merge.

```
ctest --test-dir build --output-on-failure --no-tests=error -j 10
```

The suite exits 0. Its summary block:

```
100% tests passed out of 190

Label Time Summary:
quick    = 153.76 sec*proc (183 tests)
slow     = 346.10 sec*proc (7 tests)

Total Test time (real) =  71.29 sec

The following tests did not run:
	188 - harness_topdown_run_test (Skipped)
```

The verdict figures: 189 `Passed` entries, 0 `Failed` entries, and 0
`Not Run` entries. One entry skips: test 188,
`harness_topdown_run_test`. The wall time reads 71.29 sec.

```
./build/example/docs_command_line --catalog | wc -l
467
```

```
ctest --test-dir build -N | tail -1
Total Tests: 190
```

Run notes: the first suite attempt over this tree ended on signal 15
after 63.5 seconds at entry 185 of 190, with no other build holding
the tree at that moment. The recorded attempt above ran to completion
and exits 0.

## Host and kernel facts (T002)

```
grep -m1 'vendor_id' /proc/cpuinfo
vendor_id	: AuthenticAMD
```

```
grep -m1 'cpu family' /proc/cpuinfo
cpu family	: 26
```

```
grep -m1 '^model' /proc/cpuinfo
model		: 68
```

```
cat /sys/bus/event_source/devices/cpu/type
4
```

```
cat /sys/bus/event_source/devices/cpu/events/instructions
event=0xc0
```

## Marker and number reads at base

```
git grep -c LCOV_EXCL -- source include | awk -F: '{s+=$2} END {print s}'
84
```

The carriers of the base version figure moved during this lane at
commit b0cd97e2 (T043); the reads below stand as the base figures at
head 0fce6651, named by `results/commits.md`.

```
grep -n '^    VERSION 0.34.0' CMakeLists.txt
82:    VERSION 0.34.0
```

```
grep -n '^    SOVERSION 8' CMakeLists.txt
172:    SOVERSION 8
```

```
grep -n '^    SOVERSION 22' CMakeLists.txt
456:    SOVERSION 22
```

```
grep -n 'kSchemaVersion = "1.16"' source/harness/json_report.cpp
58:constexpr std::string_view kSchemaVersion = "1.16";
```

The configure prints no version line of its own, so the version figure
comes from the generated package file.

```
grep PACKAGE_VERSION build/speedgun-ngConfigVersion.cmake
set(PACKAGE_VERSION "0.34.0")
```

That grep prints 16 lines of the generated file, and the line quoted
above carries the version figure.

```
wc -l README.md
1003 README.md
```

```
sed -n '256p' tools/docs/example_drift.py
PAGE_LINE_CAP = 1004
```

The page measures 1003 lines against the cap of 1004, so the in-place
README row of FR-010 has one line of headroom.

## Tree provenance

T003 first configured the shared `build` tree to the `ci-ubuntu` preset
at head `89c9d9876212ac2efe3df71328fdbce4c5da58fd` and cleaned it.
Before any later read of that tree it was returned with
`cmake --preset=ci-rocky -U CMAKE_CXX_CLANG_TIDY -U CMAKE_CXX_CPPCHECK`,
and the cache then reads `CMAKE_BUILD_TYPE:STRING=Release` and
`SPEEDGUN_DBC_MATRIX:UNINITIALIZED=ON` with no analyzer variables.
All ci-rocky figures above predate that event, and the analyzer
counts below come from the separate `build-tidy` tree.

## Analyzer counts at base (T003)

Every figure in this section comes from the `ci-ubuntu` preset configured
into `build-tidy`, a tree apart from the shared `build` tree. The flag
`SPEEDGUN_DBC_MATRIX` reads OFF in `build-tidy`, so the registered test
count there differs from the ci-rocky tree, and the 190 figure above
belongs to the ci-rocky tree at this head.

```
cmake --preset=ci-ubuntu -B build-tidy
-- Configuring done (1.0s)
-- Generating done (0.1s)
-- Build files have been written to: /home/archerc/code/speedgun-ng/build-tidy
```

```
cmake --build build-tidy --target clean
```

That clean exits 0 and prints no line.

```
cmake --build build-tidy -j 10 2>&1 | tee /tmp/sg051-base-analyzer.log
BUILD_EXIT=0
```

The build exits 0. The wall clock reads 246.3 seconds, and the log holds
258314 lines.

### The name check

```
grep -c 'readability-identifier-naming' /tmp/sg051-base-analyzer.log
10
```

T003 names a nonzero read of this grep a report-and-stop condition, so the
task stopped at that read and sent both greps below to the record owner.
The owner approved recording the pair and corrected the grep string in
`tasks.md`.

All 10 hits are echoed source text: the log quotes a suppression comment
inside a snippet of an unrelated diagnostic. One such line exists in the
tree, line 18 of `test/source/detail/gcov_flush.hpp`.

```
git grep -n 'NOLINT(readability-identifier-naming)' -- source include test tools
test/source/detail/gcov_flush.hpp:18:__gcov_dump();  // NOLINT(readability-identifier-naming) library symbol
```

The log repeats that source line 10 times, in this echoed shape:

```
   18 | __gcov_dump();  // NOLINT(readability-identifier-naming) library symbol
```

The two greps that name a diagnostic answer 0.

```
grep -c '\[readability-identifier-naming\]' /tmp/sg051-base-analyzer.log
0
```

```
grep -c ': warning:.*readability-identifier-naming' /tmp/sg051-base-analyzer.log
0
```

### Warning counts per edited file

CMake drives each compile through `__run_co_compile`, which runs cppcheck,
then clang-tidy, then the compile under the ccache launcher. The cache
serves the compile step, and both analyzers see every unit. A count
attributes a diagnostic to the file path that diagnostic names.

```
grep -c 'Building CXX object' /tmp/sg051-base-analyzer.log
324
```

One count per edited file, in the order the task names them.

```
grep -c '/source/counters/detail/pmu.hpp:.*: warning:' /tmp/sg051-base-analyzer.log
117
```

```
grep -c '/source/counters/system.cpp:.*: warning:' /tmp/sg051-base-analyzer.log
22
```

```
grep -c '/source/counters/plan.cpp:.*: warning:' /tmp/sg051-base-analyzer.log
38
```

```
grep -c '/source/counters/linux_pmu/provider.cpp:.*: warning:' /tmp/sg051-base-analyzer.log
28
```

```
grep -c '/source/counters/linux_pmu/group_io.cpp:.*: warning:' /tmp/sg051-base-analyzer.log
47
```

```
grep -c '/source/harness/runner.cpp:.*: warning:' /tmp/sg051-base-analyzer.log
325
```

```
grep -c '/source/harness/options.cpp:.*: warning:' /tmp/sg051-base-analyzer.log
235
```

The `source/harness/runner.cpp` figure carries a caveat: clang-tidy stops
at a syntax error on `source/harness/runner.cpp:3005`, so the 325 count
covers the part of that unit the analyzer reached.

### Error-severity lines at base

```
/home/archerc/code/speedgun-ng/source/harness/runner.cpp:3005:53: error: syntax error [syntaxError]
/home/archerc/code/speedgun-ng/test/source/harness_jitter_probe_alloc_test.cpp:513:11: error: Non-local variable 'plantedEscapeSink' will use pointer to local variable 'allocation'. [danglingLifetime]
/home/archerc/code/speedgun-ng/test/source/harness_topdown_run_test.cpp:3558:3: error: Non-local variable 'selfPath' will use pointer to local variable 'self'. [danglingLifetime]
```

Three log lines carry error severity, and the build exits 0 with them
present. The head read of T034 has to show these three lines identically.
