# Intel final acceptance: the hwlat surface

**Spec**: [spec.md](../spec.md), section `Acceptance extension: Intel final acceptance
(2026-11-07)` | **Plan**: [plan.md](../plan.md) | **Date**: 2026-11-07
**Branch**: `docs/intel-final-acceptance`

## Identity

| Field | Reading | Command |
|---|---|---|
| Host | CorsairOne | `uname -n` |
| Cpu | Intel(R) Core(TM) i9-9920X CPU @ 3.50GHz, family 6, model 85 | `grep -m1 "model name" /proc/cpuinfo` |
| Kernel | Linux 7.3.0-4-cachyos, x86_64 | `uname -a` |
| Library | speedgun-ng 0.34.0, report schema `1.16` | run header, `schemaVersion` |
| Source base | `615c0964547e4e9768b8e2f2d79991033a0eb5bd` | `git rev-parse origin/master` |
| Measured head | `615c0964547e4e9768b8e2f2d79991033a0eb5bd` | `git rev-parse HEAD` |
| Root session | none | `sudo -n -l`, exit 1, `sudo: a password is required` |

Toolchain and gate versions stand in Part D of this record. Every capture of this
record sits outside the repository under `/tmp/ia`. Build trees sit under `build/` of
this checkout.

## Method and deviation

Each timed row below ran alone at 15:55 and 15:56 local time, with the load average and
the count of other build or test processes printed at each start. The counts read 0 in
every row and the one-minute load average ran from 1.91 to 3.50, decaying from a coverage
suite that had ended 46 seconds before the first row.

An earlier pass of these same rows ran at 14:22, inside the window of that coverage
suite. Its captures stand at `/tmp/ia/partA`, `/tmp/ia/partB` and `/tmp/ia/partC`. No
figure from that pass enters this record.

## Host facts (IA48-001)

Read before any run, from SC-005, SC-007 and FR-019 to FR-024 of the spec and from
[quickstart.md](../quickstart.md). No command below writes.

| Fact | Reading | Command | Exit |
|---|---|---|---|
| Cpus | 24, affinity and online set 0-23 | `nproc`, `cat /sys/devices/system/cpu/online`, `taskset -pc $$` | 0 |
| `perf_event_paranoid` | -1 | `cat /proc/sys/kernel/perf_event_paranoid` | 0 |
| `nmi_watchdog` | 0 | `cat /proc/sys/kernel/nmi_watchdog` | 0 |
| Kernel config | `CONFIG_HWLAT_TRACER=y`, `CONFIG_OSNOISE_TRACER=y` | `zcat /proc/config.gz` with a filter for both names | 0 |
| Kernel config, file route | neither `/lib/modules/$(uname -r)/config` nor `/boot/config-$(uname -r)` is readable | the standing loop over both paths | 1 |
| tracefs | `tracefs /sys/kernel/tracing tracefs rw,nosuid,nodev,noexec,relatime 0 0`, mode `drwx------ root:root` | `grep tracefs /proc/mounts`, `stat -c "%A %U:%G %n" /sys/kernel/tracing` | 0 |
| debugfs route | `ls: cannot access '/sys/kernel/debug/tracing': Permission denied` | `ls -ld /sys/kernel/debug/tracing` | 2 |
| `current_tracer` | `cat: /sys/kernel/tracing/current_tracer: Permission denied` | `cat /sys/kernel/tracing/current_tracer` | 1 |
| Detector presence | `Permission denied` on `/sys/kernel/tracing/events/hwlat_detector`, `tracer_types`, `hwlat_detector/mode`, `osnoise`, `instances` | `ls` and `cat` on each path | 1 or 2 |
| `msr` nodes | `crw------- root root 202, 0 /dev/cpu/0/msr`, the same for cpu 12 | `ls -l /dev/cpu/0/msr /dev/cpu/12/msr` | 0 |
| Kernel command line | no `isolcpus`, no `nohz_full`, no `rcu_nocbs` | `cat /proc/cmdline` | 0 |
| `NMI` row of `/proc/interrupts` | readable, an all-zero row | `grep -m2 NMI /proc/interrupts` | 0 |

The last row contradicts the standing artifact. `results/user-space.md` records that a
reader without rights could not read `/proc/interrupts`; on this host today the read
answers at exit 0. The two surfaces differ. The interrupt table is one file, and the
library reads the `msr` node for the SMI source, so the library still reports that
source as unavailable.

## The method surface without rights (IA48-002.1, IA48-002.6)

The command is the quickstart's method run with its `sudo` prefix removed and no `cpus=`
key added:

```
./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle,method=hwlat --json=/tmp/ia/clean/method-1.json
```

Two runs, one document each, exit 0 for both. The lines the second row quotes:

```
jitter: SMI count not available
smiDelta: not available
nmiDelta: 0
smiGapCount: 0
```

Run 1 printed `jitter: probe on cpus 1`, `jitter: loop period 540 ns, threshold 5400 ns`
and `jitter: cpu 1 kept 611 dropped 0`. Run 2 printed the same shape with its own figures.
The documents carry `schemaVersion` `1.16`, `jitter.method` `HWLAT`, `jitter.mode`
`ALONGSIDE`, `jitter.cpus` `[1]`, and the per-cpu keys `smiDelta`, `nmiDelta` and
`smiGapCount`. `smiDelta` holds JSON `null`, the writer's mark for an unavailable source.
`nmiDelta` reads 0 and `smiGapCount` reads 0.

## The engine surface without rights (IA48-002.2)

```
./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle,mode=precheck --os-noise=hwlat --json=/tmp/ia/clean/detector.json
```

Exit 0. The path stands denied, so this row records a denial:

```
tracer: not available
host settings: isolcpus=not available nohz_full=not available rcu_nocbs=not available
```

The document carries a `precheck` object with the keys `cpus`, `hostSettings`, `serial`,
`skippedGroups` and `windowMs`, and no `tracer` key. No tracer figure stands anywhere in
it. FR-023 asks for the disclosure line where the tracer stands busy. The host holds no
rights here, so the console line above is the whole disclosure.

## The example (IA48-002.3)

```
./build/example/docs_hwlat_probe
```

Exit 0, probe on cpus 0, 12 and 1, `jitter: loop period 458 ns`. Per cpu:

```
smi delta: not available (source: /dev/cpu/0/msr)
nmi delta: 0 (source: /proc/interrupts)
smi gaps: 0
```

and the summary rows `smiDelta: not available not available not available`, `nmiDelta: 0
0 0`, `smiGapCount: 0 0 0`, `jitter: SMI count not available` and `jitter: probe runs
with interrupts enabled`. The committed `.expected` block of this example belongs to the
host that recorded it. The lines above are this host's reading.

## Refusals (IA48-002.4)

| Command | Line | Exit |
|---|---|---|
| `./build/example/benchmark_example --filter=bmTouch --jitter-probe=idle,method=smi` | `--jitter-probe: the method 'smi' is not tsc, fwq, ftq or hwlat` | 2 |
| `./build/example/benchmark_example --filter=bmTouch --jitter-probe=idle,mode=precheck --os-noise=hwlat-detect` | `--os-noise: the engine 'hwlat-detect' is not osnoise, timerlat or hwlat` | 2 |
| `./build/example/benchmark_example --filter=bmTouch --jitter-probe=idle,method=hwlat --os-noise=hwlat` | `--os-noise: the batch carries no pre-check window for the tracer` | 2 |

## The round trip (IA48-002.5)

```
./build/example/first_benchmark --compare=/tmp/ia/clean/method-1.json,/tmp/ia/clean/method-2.json \
  --compare-rule="jitter:run:any:smiGapCount:lower:abs=0"
```

Exit 0. The header names both documents and the rule row answers:

```
compare: baseline /tmp/ia/clean/method-1.json (speedgun 0.34.0, schema 1.16)
compare: contender /tmp/ia/clean/method-2.json (speedgun 0.34.0, schema 1.16)
bmTouch/threads:2        jitter:run:any:smiGapCount:lower:abs=0           0.000           0.000         n/a          +0.000  OK
```

The same pair under a detector rule answers at exit 2:

```
error: the rule 'hwlat:any:maxGapNs:lower:abs=200000' names a detector figure and the baseline report carries no tracer record
```

Beside the round trip stands a parser read of the method document itself:

```
python3 -c "import json; d = json.load(open('/tmp/ia/clean/method-1.json')); print(d['schemaVersion']); print(d['precheck']['tracer'])"
```

It prints `1.16` and then `KeyError: 'precheck'` at exit 1. The method document carries no
pre-check object, so the `precheck.tracer` key stays unreadable here and the key name is
named as blocked.

## The root rows (IA48-003, IA48-004, IA48-005)

Blocked. The missing right is a session that can write tracefs and read the `msr` nodes.
`sudo -n -l` answers `sudo: a password is required` at exit 1. The operator action is to
open that session and run the two quickstart root commands, quoted here without their
`sudo` prefixes:

```
./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle,method=hwlat --json=/tmp/hwlat-method.json
./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle,mode=precheck --os-noise=hwlat --json=/tmp/hwlat-detector.json
```

| Row | Status | Figure |
|---|---|---|
| IA48-004.1 pre-flight: `current_tracer`, `tracing_on` | blocked | none |
| IA48-004.2 snapshot of the six D-8 values, `tracing_on` read beside them | blocked | none |
| IA48-005.1 six values after a normal end | blocked | none |
| IA48-005.2 six values after a controlled interruption | blocked | none |

The six values are `current_tracer`, `tracing_cpumask`, `tracing_thresh`,
`hwlat_detector/mode`, `hwlat_detector/width` and `hwlat_detector/window`. `tracing_on`
is a reading of its own outside the six. The stop rule stands unused: it applies where
`current_tracer` reads other than `nop` or where `tracing_on` reads 1, and neither file is
readable here, so no run executed and no console shape exists to quote. Where a row
stands blocked, `tracing_on` names the contract value of FR-021 at 0 and no figure.

The threshold residue is documentary. D-8, plan risk R-4 and the guide page name it: the
kernel leaves `tracing_thresh` at the value the run wrote, and no user-space read reaches
it on this host. The row carries no figure.

## A zero SMI count is a measurement (IA48-006)

`smiGapCount: 0` and `nmiDelta: 0` are measured figures from a source the library could
read: the interrupt table. `smiDelta` reads `not available`, because the `msr` node
refuses, so this record names that refusal where a count would otherwise stand. None of
these figures is proof about live SMIs. The claim that a planted SMI is found stands on
the fixture tests, and the claim that this host saw none stands on this row alone.

## Part D - the instrumented lanes and the gates

### Versions the lane used (IA-END-001, IA-END-002)

| Tool | Reading |
|---|---|
| `g++`, `gcov` | GCC 16.2.1 20260810 |
| `clang++` | clang 23.1.3 (LLVM 23.1.3) |
| `lcov` | LCOV 2.5.0-beta |
| `clang-tidy` | LLVM version 23.1.3 |
| `cppcheck` | 2.22.0 |
| `doxygen` | 1.18.0 |
| `clang-format` | 18.1.8 |
| `codespell` | 2.4.3 |
| `python3` on `PATH` | `/usr/bin/python3`, Python 3.14.8, PyYAML 6.0.3 |
| gate target interpreter | `/home/archerc/.local/bin/python3.11`, CPython 3.11.16, no PyYAML |
| `ccache`, `cmake`, `ctest` | 4.11.3, 4.4.2, 4.4.2 |

One step of the lane's own script stood listed twice, and its second copy answered 127
for a missing file. That is a fault of this lane's script, of no gate. The table above
comes from a clean re-run of the same step at 21:34 local time.

### The coverage lane (IA-END-001.1, IA-END-001.7)

| Step | Command | Exit | Reading |
|---|---|---|---|
| Configure | `cmake --preset=ci-coverage` | 0 | tree at `build/coverage` |
| Build | `cmake --build build/coverage -j "$(nproc)"` | 0 | 185 entries registered |
| Zero | `lcov --zerocounters -d build/coverage` | 0 | a local step, added by this lane |
| Suite | `ctest -j 1 --output-on-failure --no-tests=error` in `build/coverage` | 8 | 181 of 185 passed in 4,001.05 s |
| Cut rows, by hand | the four binaries, run from `build/coverage` | 0 each | see the rows below |
| Capture and gate, first | `cmake --build build/coverage -t coverage` | 2 | the chain stops at the line check |
| Capture and gate, second | the same target after the four hand runs | 2 | make's exit; the chain now stops at the branch check |
| Gate, by hand | `bash tools/dbc/coverage_gate.sh build/coverage/coverage.info` | 1 | `coverage-gate: branch shortfall 26 rows exceeds the  D-14 recorded allowance of 21` |

The four cut entries are timeouts at their ceilings and nothing failed on its data:
`base_cli_matrix` at 120 s, and `harness_jitter_cli_test`, `harness_hunt_cli_test` and
`example_run` at 300 s. Each ran again by hand in the same tree, outside ctest and
therefore outside its ceiling, and each answered 0:

| Entry | Ceiling | Elapsed by hand | Exit |
|---|---|---|---|
| `harness_jitter_cli_test` | 300 s | 509 s | 0 |
| `harness_hunt_cli_test` | 300 s | 569 s | 0 |
| `base_cli_matrix` | 120 s | 286 s | 0 |
| `example_run` | 300 s | 775 s | 0 |

`harness_hunt_cli_test` at 569 s against a 300 s ceiling matches the standing reading of
565 s in `results/gates.md`. No ceiling is a defect of the code, and `--timeout` cannot
widen one: ctest reads the lesser of the entry's `TIMEOUT` property and its own
`--timeout`.

### The trace, both captures

| Capture | Lines | Functions | Branches |
|---|---|---|---|
| After the serial suite alone | 99.2% (18476 of 18621) | 98.2% (1926 of 1962) | 98.4% (9101 of 9249) |
| After the four hand runs | 100.0% (18629 of 18629) | 98.4% (1931 of 1963) | 99.7% (9223 of 9249) |

The gate reads the second trace at exit 1.

The line metric reaches 100.0 percent once the four entries run to their ends, so those
four ceiling crossings stood behind the whole line shortfall. The gate then reaches the
branch check and fails at 26 rows against the recorded allowance of 21. The allowance
counts the CI union, whose readings the gate's own comment lists at 16 to 19 rows, and the
same comment says a developer host reads higher because an entry there can stop early and
leave the arms past its stop dark. This run is a developer-host run, and it reports what
this host leaves dark.

On the failure the gate prints its own list: 338 rows, one per source line and branch id
whose count reads zero, over 41 files. The six files with the most carry 159 of them:

| File | Rows at zero |
|---|---|
| `source/harness/json_load.cpp` | 29 |
| `source/harness/cli.cpp` | 28 |
| `source/harness/jitter_probe.cpp` | 27 |
| `source/harness/options.cpp` | 27 |
| `source/harness/runner.cpp` | 25 |
| `source/harness/noise_tracer.cpp` | 23 |

No file in that list stands among the ten files this change edits, and no line here is a
claim that a path is dead code. R-1, R-2 and W-1 resolve to `specs/045-merge-fixes` FR-015
and carry no definition here. The tool prints 98.4 percent of functions, and no gate reads
that metric; the file's own records hold 32 function entries at a count of zero, ten in
`source/harness/runner.cpp` and ten in `execution_context.hpp`. The standing record names
the lambda of `stripTopdownColumns` as the position whose inconsistent data the current
suppression hides, and that lambda stands in this list, so the G-25 repair stays required
work and this change reports it without changing any gate.

### The suite lanes (IA-END-001.2 to IA-END-001.6)

| Lane | Command | Exit | Entries | Result |
|---|---|---|---|---|
| Plain, CI form | `ctest --test-dir build --output-on-failure --no-tests=error -C Release -j "$(nproc)"` | 0 | 190 | 100% passed, 336.74 s |
| Two cpus | `taskset -c 0,1 ctest --test-dir build --output-on-failure --no-tests=error -C Release -j 2` | 0 | 190 | 100% passed, 1,459.90 s |
| Four cpus | `taskset -c 0,1,2,3 ctest --test-dir build --output-on-failure --no-tests=error -C Release -j 4` | 0 | 190 | 100% passed, 738.27 s |
| Address and undefined | block B of the plan, `CXX=clang++` | 8 | 185 | 183 of 185 in 538.16 s |
| Thread sanitizer | block C of the plan, `CXX=clang++` | 8 | 186 | 182 of 186 in 702.09 s |

The plain lane ran 190 entries, the entry count `ctest -N` prints. CI runs the same flags
with `working-directory: build`, and `--test-dir build` carries the same directory. The
two-cpu and four-cpu rows run the full entry set, so the `prepush-quick` reading of
`build/prepush` stands not run, and this lane names no `--timeout`.

Every red name above stands in a tracked record already, so no base-tree build stands
needed: `specs/045-merge-fixes/results/acceptance-corsairone-cov.md` names the same ceiling
crossings and names `report_shape_fwq_ftq_sample` at 110.21 s, where the sample gives up
after its own bound of 110 seconds. Each red name ran again alone in the same tree at
`-j 1`, with the ceiling ctest states for it, the entry count `ctest -N` gives for that
name, and the load average at both ends:

| Name and tree | Ceiling | Elapsed | Load at start | Load at end | Exit |
|---|---|---|---|---|---|
| `base_cli_matrix` in `sanitize` | 120 | 121 s | 1.00 1.12 1.11 1/1173 1377843 | 1.01 1.09 1.09 1/1173 1378959 | 8 |
| `example_run` in `sanitize` | 300 | 301 s | 1.01 1.09 1.09 1/1173 1378964 | 1.22 1.21 1.14 1/1175 1382822 | 8 |
| `base_cli_matrix` in `tsan` | 120 | 120 s | 1.22 1.21 1.14 1/1175 1382827 | 1.06 1.16 1.13 2/1172 1383784 | 8 |
| `example_run` in `tsan` | 300 | 300 s | 1.90 2.34 1.73 2/1171 1387023 | 1.18 1.60 1.57 2/1175 1390588 | 8 |
| `harness_jitter_cli_test` in `tsan` | 300 | 301 s | 1.06 1.16 1.13 1/1171 1383789 | 4.06 2.70 1.78 1/1171 1386382 | 8 |
| `report_shape_fwq_ftq_sample` in `tsan` | 120 | 80 s | 4.06 2.70 1.78 1/1171 1386387 | 1.90 2.34 1.73 2/1171 1387018 | 0 |

A timeout at a ceiling repeats alone. The ceiling belongs to the entry, and such a row
says nothing about the code. The one entry that failed on its data inside the
thread-sanitizer lane answers 0 alone: `report_shape_fwq_ftq_sample` in `tsan`, in 80 s.
That reading names load as its cause, and this run leaves the code unimplicated.

### The gates (IA-END-002)

| Gate | Command | Exit | Reading |
|---|---|---|---|
| Format | `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake` | 0 | clang-format 18.1.8 |
| Spell | `cmake -P cmake/spell.cmake` | 0 | codespell 2.4.3, run again after the records |
| Drift | `python3 tools/docs/example_drift.py .` | 0 | no drift |
| Drift, four flags | `python3 tools/docs/example_drift.py --sections --names --options --forbidden .` | 0 | no drift |
| Time source | `bash test/time_source_gate.sh .` | 0 | no finding |
| Prose, tree | `cmake -D PROSE_MODE=tree -D PROSE_CHECK=prose -P cmake/prose-lint.cmake` | 0 | 1082 sources, 209,402 units, 0 findings |
| Prose, range | after the squash | | |
| DBC target | `cmake --build build --target dbc-gate` | 2 | `dbc gates require PyYAML (install python3-yaml): No module named 'yaml'` from `/home/archerc/.local/bin/python3.11`, a host fact |
| DBC doxygen | `doxygen build/dbc-gate/Doxyfile` | 0 | XML written |
| DBC document gate | `/usr/bin/python3 tools/dbc/dbc_doc_gate.py ...` | 0 | `doc-gate: 349 interfaces, 0 gaps` |
| DBC pair gate | `/usr/bin/python3 tools/dbc/dbc_pair_gate.py --src include/speedgun-ng --src2 source tools ...` | 0 | `pair-gate: 349 interfaces, 0 gaps` |
| Analyzer, library | block E, first scope | 0 | 146 translation units, 3,920 findings, 0 on the gate checks |
| Analyzer, tests | block E, second scope | 0 | 134 translation units, 18,983 findings, 0 on the gate checks |
| Analyzer, examples | block E, third scope | 0 | 43 translation units, 667 findings, 0 on the gate checks |
| pmu events | `python3 tools/pmu_events/update_pmu_events.py --check` | 0 | tables current |
| Host records | `bash tools/ci/host_record_gate.sh build/test/host_record_check . validation` | 0 | records current |
| README | `wc -l README.md` | 0 | 1,002 lines against the cap of 1,004 |
| Markers | `git grep -c LCOV_EXCL -- source include` summed | 0 | 84 markers over 13 files against the cap of 84 |
| Release build | `cmake --preset=ci-rocky`, `cmake --build build --config Release -j "$(nproc)"` | 0 | SC-004 |
| Install | `cmake --install build --config Release --prefix prefix` | 0 | tree under `prefix/` |
| Install audits | the twelve dependency audits and the link-manifest audit | 0 each | no third-party symbol, no library outside the runtime |

The analyzer owns no translation unit on this branch, so its expected count for owned
files is zero by that fact. The clear of both compiler launchers stands disclosed: CI
needs none because its ccache stands cold, and the host clears them so a cache hit cannot
stand in for an analyzer pass.

### The consumer-release job (IA-END-002.5)

Every step of the CI job ran in order, in both trees, and each answered as the job
expects. The cache asserts read `CMAKE_BUILD_TYPE:STRING=Release`,
`speedgun-ng_DEVELOPER_MODE:BOOL=OFF` and `speedgun-ng_CONTRACTS:STRING=ignore`, and the
archive symbol check found no `sg::dbc::check` symbol in `build/consumer-release/libspeedgun-ng.a`.

| Step | Expected | Measured |
|---|---|---|
| `dbc_trap_fixture` under `ulimit -c 0` | non-zero and not 124 | non-zero, exit 134 |
| `counters_trap_fixture`, nine modes | 0 each | `mbf=0 fold=0 foe=0 pxt=0 pdec=0 rxt=0 sxt=0 pmo=0 pfs=0` |
| `counters_trap_fixture overrun` | non-zero and not 124 | 134 |
| `counters_trap_checked_test` under `ctest -R counters_trap` | 0 | 0 |
| `dbc_test` | exit 1, first line named | exit 1, `DBC TEST FAIL: pre: violation delivered to the observer` |

124 is a kill and never a pass, and no row above reads one.

## What this record does not claim

- No row here rests on a CI run. CI results stand in the CI logs.
- No root row carries a figure, and no blocked row reads as a pass.
- No `SC-011` statement stands in this record. The two standing citations of that id at
  `results/gates.md` and `tasks.md` stay as historical text.
- Tasks T054 to T056 stand unrun: they record a user-space run, a detector run and two raw
  snapshot blocks, and the second needs a root session this host withholds.
- No row quotes a commit sha as evidence of its own commit. The single commit of this
  branch and the pull request carry the head sha, and every measurement above ran at a
  tree whose code matches `615c0964547e4e9768b8e2f2d79991033a0eb5bd`, which the pull
  request states as the base.


## Rerun at head `87d745d6c7e62d4da80198c689ddc7078f08ff3d` (2026-11-08)

**Base and measured head for this section**: `87d745d6c7e62d4da80198c689ddc7078f08ff3d`
(`origin/master`). The sections above stand as written for `615c0964…`. This head adds
`d7e86254 Harness: Repair the contract-closure data fault`, the repair that the standing
record named as pending under ruling G-25. That commit edits `cmake/coverage.cmake`,
`tools/dbc/coverage_gate.sh`, four harness sources and four test sources, so every lane
stands measured again. Host `CorsairOne`, cpu i9-9920X with `pmu_name` `skylake`, kernel
Linux 7.3.0-4-cachyos, GCC 16.2.1, clang 23.1.3, `perf_event_paranoid` -1,
`nmi_watchdog` 0, and no root session: `sudo -n -l` answers `a password is required` and
`pkexec --disable-internal-agent id` answers `No authentication agent found` at exit 127.

### The unprivileged surface (IA48-002)

Every row of the first pass reproduces on this head. Each run stood alone with its load
witness.

| Run | Exit | Reading |
|---|---|---|
| Method, run 1 | 0 | `smiDelta: not available`, `nmiDelta: 0`, `smiGapCount: 0`, `jitter: SMI count not available` |
| Method, run 2 | 0 | the same three values |
| Engine without rights | 0 | `tracer: not available`, and the document carries no `precheck.tracer` key |
| `docs_hwlat_probe` | 0 | `smi delta: not available (source: /dev/cpu/0/msr)`, then the same for cpus 12 and 1, and `tracer: not available` |
| Refusal a, `method=smi` | 2 | refusal text |
| Refusal b, `os-noise=hwlat-detect` | 2 | `--os-noise: the batch carries no pre-check window for the tracer` |
| Refusal c, `method=hwlat` with `--os-noise` | 2 | refusal text |
| Round trip, `jitter:run:any:smiGapCount:lower:abs=0` | 0 | `bmTouch/threads:2 … 0.000 0.000 n/a +0.000 OK` |
| Rule over `hwlat:any:maxGapNs` | 2 | `error: the rule … names a detector figure and the baseline report carries no tracer record` |

A zero `nmiDelta` and a zero `smiGapCount` stay measured figures. Neither is proof about
live SMIs, and the planted-SMI claim stands on the fixture tests.

### The root rows, still blocked (IA48-004, IA48-005)

No route to a root session opened between the two passes. The facts: `sudo -n -l` answers
`sudo: a password is required`; `pkexec --disable-internal-agent id` answers
`No authentication agent found` at exit 127; `/sys/kernel/tracing` and `/sys/kernel/debug`
read `drwx------ root:root`; `/dev/cpu/0/msr` reads `crw------- root root 202, 0`; the
group database holds no `tracing` group and this session carries no supplementary group
that reaches either file.

The six names, `tracing_on` beside them, and the controlled-interruption variant carry no
figure. The operator action that closes them runs in one privileged shell at the built tree:

```sh
T=/sys/kernel/tracing
for f in current_tracer tracing_cpumask tracing_thresh hwlat_detector/mode \
         hwlat_detector/width hwlat_detector/window; do
  printf 'before %s %s\n' "$f" "$(cat $T/$f)"
done
printf 'before tracing_on %s\n' "$(cat $T/tracing_on)"
./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle,mode=precheck --os-noise=hwlat --json=/tmp/hwlat-detector.json
for f in current_tracer tracing_cpumask tracing_thresh hwlat_detector/mode \
         hwlat_detector/width hwlat_detector/window; do
  printf 'after %s %s\n' "$f" "$(cat $T/$f)"
done
printf 'after tracing_on %s\n' "$(cat $T/tracing_on)"
```

Then the same seven reads again around a run ended by `kill -INT` on the run's own process
id, with the shell status of that run printed beside it. Six before-and-after rows belong
to each variant, and `tracing_on` belongs beside them as a reading of its own. The restore
never writes a prior value back, and `tracing_thresh` keeps the run threshold as the
kernel's remembered default, which is documentary (D-8, plan R-4).

### The coverage lane (IA-END-001.1)

| Step | Exit | Reading |
|---|---|---|
| `cmake --preset=ci-coverage` | 0 | tree at `build/coverage` |
| `cmake --build build/coverage -j "$(nproc)"` | 0 | 185 entries |
| `lcov --zerocounters -d build/coverage` | 0 | a local step, named as an addition |
| `ctest -j 1 --output-on-failure --no-tests=error` | 8 | 181 of 185 passed in 3,976.71 s |
| Four cut entries, by hand, no ceiling | 0 each | 285 s, 508 s, 571 s, 776 s |
| `cmake --build build/coverage -t coverage` | 2 | make's exit; the chain stops at the branch check |
| `bash tools/dbc/coverage_gate.sh build/coverage/coverage.info` | 1 | `branch shortfall 24 rows exceeds the  D-14 recorded allowance of 21` |

One capture stands taken on this head, and it stands taken after the four hand runs, so
this section carries no serial-suite-alone reading for this head. The reading above for the
earlier head, 99.2 percent of 18,621 lines, stands where it stands.

| Capture | Lines | Functions | Branches |
|---|---|---|---|
| After the four hand runs | 100.0% (18,657 of 18,657) | 99.0% (1,936 of 1,956) | 99.7% (9,249 of 9,273) |

The repair reached the two shapes it named. Cold functions fell from 32 to 20, the lambda
of `stripTopdownColumns` no longer stands in the cold list, and the stricter gate read,
which now ignores no lcov error class, printed `no messages were reported`. The
data-consistency complaint is gone from the read. The branch shortfall stands at 24 against
the allowance of 21, three rows over. The hosted coverage job fails too. At this pull
request's head it reads `lines 100.0% (18676 of 18676)`, `functions 98.7% (1953 of 1978)`
and `branches 99.8% (9800 of 9823)`, and the gate line reads:
`branch shortfall 23 rows exceeds the D-14 recorded allowance of 21`. The run at the base
commit of the same source read 27 rows there. The shortfall therefore sits near its allowance
on both hosts, and the hosted count moves between two runs of identical source. The line rule
passes on both. No marker, allowance or suppression changed here, and G-25 stands landed. A
later head closes that shortfall, and the section on `2d4d8c57` carries the reading.

### The suite lanes (IA-END-001.2 to IA-END-001.6)

| Lane | Exit | Entries | Result |
|---|---|---|---|
| Plain, CI form | 0 | 190 | 100% passed, 337.81 s |
| `taskset -c 0,1` with `-j 2` | 0 | 190 | 100% passed, 1,464.29 s |
| `taskset -c 0,1,2,3` with `-j 4` | 0 | 190 | 100% passed, 742.87 s |
| Address and undefined sanitizer | 8 | 185 | 183 of 185 in 516.31 s |
| Thread sanitizer | 8 | 186 | 182 of 186 in 659.14 s |

### The sanitizer diagnosis (IA-END-001.8)

The two instrumented lanes answer red on four names between them, and no name fails on its
data. The table carries a fifth, `harness_hunt_cli_test`, because that name crosses its
ceiling alone and at the hosted shape while it passed inside the 24-job lanes. Each name
ran again alone, on a machine with nothing else running, from the working directory ctest
gives that entry, so the ceiling stood as the only bound:

| Name | Ceiling | Plain tree | Address and undefined, alone | Thread, alone | Factor |
|---|---|---|---|---|---|
| `base_cli_matrix` | 120 s | 75.26 s | 139 s | 134 s | 1.8 / 1.8 |
| `harness_jitter_cli_test` | 300 s | 125.62 s | 180 s | 590 s | 1.4 / 4.7 |
| `harness_hunt_cli_test` | 300 s | 256.15 s | 345 s | 335 s | 1.3 / 1.3 |
| `example_run` | 300 s | 189.10 s | 361 s | 400 s | 1.9 / 2.1 |
| `report_shape_fwq_ftq_sample` | 120 s | 32.57 s | 37 s | 79 s | 1.1 / 2.4 |

Alone on an idle machine, three names cross their ceiling in the address and undefined lane
(`base_cli_matrix` 139 s against 120 s, `harness_hunt_cli_test` 345 s against 300 s,
`example_run` 361 s against 300 s) and four cross it in the thread lane (`base_cli_matrix`
134 s, `harness_jitter_cli_test` 590 s, `harness_hunt_cli_test` 335 s, `example_run` 400 s).
A ceiling that the uninstrumented build already fills most of leaves a sanitizer no room,
and a factor between 1.1 and 4.7 then decides the row.

One name reads differently by shape, and both readings stand here. `harness_hunt_cli_test`
passed inside the 24-job lanes at 289.30 s and at 281.75 s, took 345 s and 335 s alone, and
timed out at 300 s when the lane ran the hosted shape of four cores and four jobs. A second
24-job thread lane at `2d4d8c57`, measured by the closeout pass on this host and cited here, cut
that entry at 300.06 s where this lane passed it at 281.75 s, so the flip sits between two runs of
one shape. The spread between 281 s and 345 s stands unexplained by this record, and a name that
flips pass to cut inside one shape holds no headroom against its ceiling. A name that moves that
much between shapes holds no headroom against a fixed ceiling, and it sits nearest its
bound of the five.

The hosted shape reproduces the gap. With the machine masked to `0,1,2,3` and `-j 4`, the
thread lane timed out `base_cli_matrix`, `harness_jitter_cli_test` and `example_run`, and
the address lane timed out `base_cli_matrix`, `harness_hunt_cli_test` and `example_run`.
The sample passed there at 91.98 s and at 47.48 s. Four-way concurrency is therefore not
what separates this host from the hosted lane.

`report_shape_fwq_ftq_sample` differs in kind. Its 120 s ceiling is not the bound that binds
it. `tools/docs/report_shape.py` grants the example it drives 110 s of its own, and inside
the thread lane the example reached that grant at 110.26 s, with the message
`docs_fwq_ftq: the run failed: Command […] timed out after 110 seconds`. The same example
alone in the same tree takes 80 s under the thread sanitizer, 36 s under the address
sanitizer and 33 s uninstrumented, so the concurrency of a 24-job lane carries it over the
grant while the sanitizer alone does not.

The hosted job at this head reads the same entries far shorter, which is where the ceilings
came from: 21.72 s for `base_cli_matrix` against its plain reading here, 31.66 s for
`harness_jitter_cli_test`, 42.35 s for `example_run`, 39.10 s for the sample, and 2.31 s for
`harness_hunt_cli_test`. The last gap is not speed. That entry walks the plan the host
offers, a runner without a PMU offers none, and this host offers 497 events, so the entry
does two hundred times the work here. Those hosted readings stand in this section as the
frame of a diagnosis, and no acceptance row above rests on them.

Three follow-ups belong to a later change, and this records branch performs none of them:
give the instrumented lanes a ceiling of their own, raise the 110 s grant in
`tools/docs/report_shape.py` above the longest instrumented example, and bound
`harness_hunt_cli_test` on a host whose PMU is live.

### The gates (IA-END-002)

Forty-seven steps ran in the order of the plan and every one answered exit 0 except
`cmake --build build --target dbc-gate`, which answers exit 2 with
`dbc gates require PyYAML (install python3-yaml): No module named 'yaml'` from
`/home/archerc/.local/bin/python3.11`, a host fact the first pass recorded too.

| Gate | Reading |
|---|---|
| Format, spell, drift twice, time source | exit 0 each |
| Prose, tree form | 1,087 sources, 211,204 units, 0 findings |
| DBC document gate and pair gate, by hand | `349 interfaces, 0 gaps` each |
| Analyzer | library 146 translation units, 3,920 findings, 0 on the gate checks; tests 134, 18,998, 0; examples 43, 667, 0; 0 owned units |
| pmu tables, host records | exit 0 each |
| README and markers | 1,002 lines against 1,004; 84 markers over 13 files against 84 |
| Release build, install, twelve dependency audits, link-manifest audit | exit 0 each |
| Consumer-release job | cache reads `Release`, `DEVELOPER_MODE OFF`, `CONTRACTS ignore`; no gated check symbol in the archive; `dbc_trap_fixture` exits 134; nine counter modes read 0; overrun exits 134; `dbc_test` exits 1 with `DBC TEST FAIL: pre: violation delivered to the observer` |

124 is a kill and never a pass, and no row above reads one.

### What this rerun does not claim

- No acceptance row rests on a hosted result. The hosted numbers appear only where a
  diagnosis needs a second machine in the frame, and each carries its job.
- The four root rows carry no figure.
- The record reports the branch shortfall and accepts nothing by it. The allowance
  stands at 21.
- `harness_hunt_topdown_test` and `prose_gate_fixtures` stand red on the hosted Rocky job at
  run `42487329495`, head `b90711fb`, and they stand green in every local lane above; no local
  row explains that gap, and this record states it as an open difference between the two
  runners. The next head carries a repository-metadata fault in their place, named in the
  section on the merged head.

## Coverage at the merged head `2d4d8c57` (2026-11-08)

The rerun above stands measured at `87d745d6`. Master moved below the squash of that
measurement: `e2f6a50f Harness: Clear the capture suppression` landed between the two, and it
changes the shape the coverage gate reads. The fixture macros now pass the named functions
`::sg::detail::fixtureCaseEntry` and `::sg::detail::fixtureFactoryEntry`, so gcov emits one
record per function, `--ignore-errors inconsistent` stands gone from every repository
coverage command, and that commit reports 20 rows against the same allowance on the owner's
host. The chain ran again here at `2d4d8c57`, on the same machine and the same toolchain.

| Step | Exit | Reading |
|---|---|---|
| `cmake --preset=ci-coverage` | 0 | tree at `build/coverage` |
| `cmake --build build/coverage -j "$(nproc)"` | 0 | 185 entries |
| `lcov --zerocounters -d build/coverage` | 0 | a local step, named as an addition |
| `ctest -j 1 --output-on-failure --no-tests=error` | 8 | 181 of 185 in 3,978.64 s, four timeouts |
| Four cut entries, by hand, no ceiling | 0 each | 285 s, 509 s, 571 s, 775 s |
| `bash tools/dbc/coverage_gate.sh build/coverage/coverage.info` | 0 | `branch shortfall 20 rows, within the  D-14 recorded allowance of 21 (accepted classes)` |
| `cmake --build build/coverage -t coverage` | 2 | `ERROR: required module GD.pm not found on this system` from the html step, a host fact about perl-GD |

| Capture | Lines | Functions | Branches |
|---|---|---|---|
| After the four hand runs | 100.0% (18,664 of 18,664) | 99.0% (1,958 of 1,978) | 99.8% (9,253 of 9,273) |

`no messages were reported` stands in the geninfo read and in the lcov read, so the capture
needs no error class ignored. The branch rule closes here at 20 rows against the allowance of
21, four rows under the reading at `87d745d6`, and the reduction arrives with the macro
change. No marker, allowance or suppression moved for this reading. The four hand re-runs
repeat the previous head within one second, so the ceiling arithmetic of those entries stands
unchanged, and the coverage target's exit 2 has a new cause here: the branch check passes, and
the html step needs perl-GD, which this host lacks.

### The hosted Rocky readings, head by head

The line that names `harness_hunt_topdown_test` and `prose_gate_fixtures` as red on the hosted
Rocky job belongs to run `42487329495`, at head `b90711fb`. Run `42495085606`, at head
`ab1102e5`, reached no test: the Rocky container answered `Error: Failed to download metadata
for repo 'epel'` after mirror 404 answers, and the job exited 1 at 15:32:16Z. Both heads carry
the same coverage red, `branch shortfall 23 rows exceeds the  D-14 recorded allowance of 21`,
and twelve jobs pass at each. The hosted coverage job at this pull request's head answers
red, run `42520411116`, job
`128852626791`, at head `5fe3d244`, reads `lines 100.0% (18675 of 18675)`, `functions 98.7%
(1953 of 1978)` and `branches 99.8% (9800 of 9823)`, the gate line reads `branch shortfall 23
rows exceeds the  D-14 recorded allowance of 21`, and `gmake: *** [Makefile:252: coverage] Error
2` stands behind it. The branch rule therefore closes on this host and stays open on the runner at
one head. The denominators differ too, 9,273 branch positions here against 9,823 there, which is
the tool-version difference named at the earlier head, so the two counts read as two readings of one
rule, each named by the host that produced it.
  this head and stand green in every local lane above; no local row explains that gap, and
  this record states it as an open difference between the two runners.

## Acceptance closeout: measured Intel evidence (2026-11-08)

This section measures again at the closeout base head `2d4d8c57975a86db6c77536f7090aba2bdaf6417`
and corrects the sections above. It adds no feature and changes no source file. The logs behind
every figure stand outside the repository, as IC-007 asks (`spec.md:1656-1662` (read at `af54d3f0`)). A bare log name
names a file under `/tmp/ic/base`, a `hand-*` or `cov-*` name names a file under `/tmp/ia3/cov`,
a `cov-*`, `rocky-*` or `test-*` name that carries a hosted timestamp names a file under
`/tmp/ic/base/hosted`, and `record:NN` names this file as it stands above this heading.
`spec.md` names `specs/048-hwlat-probe/spec.md`. Each local log opens with its own command, its
own load witness and its own exit, so a row can cite one line and a reader can re-run the command
beside it. Sections above keep their text, their headings and their marks, and each correction
stands in this section (`spec.md:1645-1655` (read at `af54d3f0`)). This section was written
against `af54d3f0`, the head this branch carries. Each cite into a spec, plan or
record names the head it was read at: a `spec.md` line above 1476, a `plan` line
above 836, and a record line above 640 are text this branch wrote and read at
`af54d3f0`; a line below those marks stands at the merged base `a55728af` too. A
log or hosted cite keeps the head its own row names, as ruling 10 of
`spec.md:1816-1819` (read at `af54d3f0`) asks.



### Identity at the base head (`IA48-001`, `IA-END-003`)

One logger run produced every identity reading at this head, and `identity.log` holds its
transcript. Its load witness reads `0.75 1.02 1.21 1/1645 2171711` at `identity.log:5`, with
no other compiler or CTest process. The directory that holds the local logs for this section
is `/tmp/ic/base`, the coverage chain sits under `/tmp/ia3/cov`, the frozen coverage trace
sits at `/tmp/ic/final/base-coverage`, and the hosted logs this section quotes sit under
`/tmp/ic/base/hosted`. Each log opens with its own command line, its own load witness and its
own exit, so a table can cite a short file name and still name one line.

| Item | Reading | Source line |
|---|---|---|
| Head | `2d4d8c57975a86db6c77536f7090aba2bdaf6417` | `identity.log:10` |
| Branch | `fix/intel-acceptance-closeout` | `identity.log:14` |
| Kernel | `Linux 7.3.0-4-cachyos x86_64 GNU/Linux` | `/tmp/ic/w2-probe.md:282` |
| Logical cores | 24 | `identity.log:39` |
| Affinity list, online list, present list | `0-23` for each | `identity.log:38`, `:40`, `:41` |
| `perf_event_paranoid` | `-1` | `identity.log:34` |
| `nmi_watchdog` | `0` | `identity.log:35` |
| `/dev/cpu/0/msr` | mode `crw-------`, owner `root root`, device `202, 0` | `identity.log:43` |
| `/sys/kernel/tracing` | mode `drwx------`, owner `root root` | `identity.log:45` |
| `sudo -n -l` | `sudo: a password is required` | `identity.log:49` |
| `pkexec --disable-internal-agent id` | `Error executing command as another user: No authentication agent found.` | `identity.log:50` |
| `g++ --version` | `g++ (GCC) 16.2.1 20260810` | `identity.log:23` |
| `clang++ --version` | `clang version 23.1.3` | `identity.log:24` |
| `gcov --version` | `gcov (GCC) 16.2.1 20260810` | `identity.log:25` |
| `lcov --version` | `lcov: LCOV version 2.5.0-beta`, at `$HOME/.local/bin/lcov` | `identity.log:26`, `:21` |
| `cmake --version` | `cmake version 4.4.2`, at `$HOME/.local/bin/cmake` | `identity.log:27`, `:19` |
| `ccache --version` | `ccache version 4.11.3` | `identity.log:28` |
| Shell interpreter | `/usr/bin/python3`, CPython `3.14.8`, `yaml` module `6.0.3` | `identity.log:30`, `:31`, `:32` |
| Tree-configured interpreter | `$HOME/.local/bin/python3.11`, CPython `3.11.16`, no PyYAML | `san-cfg.log:83`, `tsan-cfg.log:83` |

Two interpreters stand on this host, and the two gates that need PyYAML read the second one.
`record:340` (read at `a55728af`) records that host fact for the earlier head, and `record:218` (read at `a55728af`) records it for the
gate target. `IA-END-003` (`spec.md:1198-1203` (read at `a55728af`)) asks for a `$HOME`-relative path in a record,

so every home path in this section carries `$HOME`. `identity.log` holds no `uname -srmo`
line, so the kernel row cites the probe report that ran the command.

### The two instrumented lanes (`IA-END-001`, `SC-IC-5`)

Ruling 2 (`spec.md:1766-1773` (read at `af54d3f0`)) gives one directory to each configuration and asks a lane to
wipe its own directory before it configures. Each lane then built with 24 jobs and ran one
suite in the same 24-job shape the release lane uses.

```sh
rm -rf build/sanitize && CXX=clang++ cmake --preset=ci-sanitize
cmake --build build/sanitize -j "$(nproc)"
cd build/sanitize && ASAN_OPTIONS=strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 ctest --output-on-failure --no-tests=error -j "$(nproc)"

rm -rf build/tsan && CXX=clang++ cmake --preset=ci-tsan
cmake --build build/tsan -j "$(nproc)"
cd build/tsan && TSAN_OPTIONS=halt_on_error=1:second_deadlock_stack=1 ctest --output-on-failure --no-tests=error -j "$(nproc)"
```

| Lane | Tree | Option string from the preset | Entries | Result | Wall | Exit | Source |
|---|---|---|---|---|---|---|---|
| Address and undefined behaviour | `build/sanitize` | `-U_FORTIFY_SOURCE -O2 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-common` | 185 | 183 of 185 | 538.87 s | 8 | `CMakePresets.json:126` (read at `a55728af`), `san-cache.log:10`, `:11`, `san-count.log:11`, `san-test.log:411`, `:417`, `:423` |
| Thread sanitizer | `build/tsan` | `-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3 -O2 -g -DNDEBUG -fsanitize=thread -fno-omit-frame-pointer -fno-common` | 186 | 181 of 186 | 702.64 s | 8 | `CMakePresets.json:135` (read at `a55728af`), `tsan-cache.log:10`, `:11`, `tsan-count.log:11`, `tsan-test.log:410`, `:416`, `:425` |

The two summary lines read `99% tests passed, 2 tests failed out of 185` at `san-test.log:411`
and `97% tests passed, 5 tests failed out of 186` at `tsan-test.log:410`. Both lanes answer exit
8, so both stand red. No row reports an address error, an undefined behaviour fault or a data
race: the reds are ceiling crossings, and the section below prices them. Each lane opened with its own load witness, `15.10 5.87 2.92 1/1667 2181482` at
`san-test.log:5` and `13.58 6.59 4.69 1/1667 2208081` at `tsan-test.log:5`, each with the
affinity list `0-23` and no other compiler or CTest process.

| Lane | Name | Printed line | Source line |
|---|---|---|---|
| Address and undefined | `base_cli_matrix` | `***Timeout 120.04 sec` | `san-test.log:369` |
| Address and undefined | `example_run` | `***Timeout 300.04 sec` | `san-test.log:373` |
| Thread | `base_cli_matrix` | `***Timeout 120.02 sec` | `tsan-test.log:360` |
| Thread | `report_shape_fwq_ftq_sample` | `***Failed  110.27 sec` | `tsan-test.log:363` |
| Thread | `harness_hunt_cli_test` | `***Timeout 300.06 sec` | `tsan-test.log:376` |
| Thread | `example_run` | `***Timeout 300.03 sec` | `tsan-test.log:378` |
| Thread | `harness_jitter_cli_test` | `***Timeout 300.13 sec` | `tsan-test.log:407` |

The address and undefined lane answers two red names at this head. The section above lists
three for that lane at `record:517-518` (read at `a55728af`), and the difference is a lane reading. At this head
`harness_hunt_cli_test` passed inside the lane at 288.89 s (`san-test.log:372`), and the
earlier head measured 345 s for it alone (`record:512` (read at `a55728af`)). Ruling 10 (`spec.md:1816-1819` (read at `af54d3f0`)) keeps
each figure at the head that measured it, so the two readings stand in their own rows.

Ruling 20 (`spec.md:1869-1874` (read at `af54d3f0`)) puts both instrumented suites in the set a closeout repeats at
its final head, and `SC-IC-1` (`spec.md:1666-1668` (read at `af54d3f0`)) asks each lane to carry its command, its
exit and its count. Both lanes answered at the head named above them.

### Three shapes per red name (`IC-003`, ruling 6)

Ruling 6 (`spec.md:1792-1796` (read at `af54d3f0`)) gives each name that fails in either instrumented lane three
runs: the original suite run, one isolated run in the same tree with the same compiler, the
same sanitizer options and the same CTest ceiling, and one run bounded to `taskset -c 0,1,2,3`
with `-j 4`, the width a hosted runner answers. The isolated shape ran one name at a time with
`ctest -R` in the same tree. The bounded shape ran the red names of one tree together under the
four-core mask:

```sh
cd build/sanitize && ASAN_OPTIONS=... UBSAN_OPTIONS=... taskset -c 0,1,2,3 ctest -R '^(base_cli_matrix|example_run)$' --output-on-failure -j 4
cd build/tsan && TSAN_OPTIONS=halt_on_error=1:second_deadlock_stack=1 taskset -c 0,1,2,3 ctest -R '^(base_cli_matrix|example_run|harness_hunt_cli_test|harness_jitter_cli_test|report_shape_fwq_ftq_sample)$' --output-on-failure -j 4
```

`diag-props.log` holds the ceiling and the working directory CTest answered for each name in
the thread tree, and each control log opens with its own command line and load witness.

| Tree | Name | Shape | Printed line | Bound that binds | Source |
|---|---|---|---|---|---|
| `build/sanitize` | `base_cli_matrix` | suite | `***Timeout 120.04 sec` | 120.0 s ceiling | `san-test.log:369` |
| `build/sanitize` | `base_cli_matrix` | isolated | `***Timeout 120.13 sec` | 120.0 s ceiling | `alone-sanitize-base_cli_matrix.log:10` |
| `build/sanitize` | `base_cli_matrix` | bounded | `***Timeout 120.04 sec` | 120.0 s ceiling | `bounded4-sanitize.log:11` |
| `build/sanitize` | `example_run` | suite | `***Timeout 300.04 sec` | 300.0 s ceiling | `san-test.log:373` |
| `build/sanitize` | `example_run` | isolated | `***Timeout 300.03 sec` | 300.0 s ceiling | `alone-sanitize-example_run.log:10` |
| `build/sanitize` | `example_run` | bounded | `***Timeout 300.01 sec` | 300.0 s ceiling | `bounded4-sanitize.log:13` |
| `build/tsan` | `base_cli_matrix` | suite | `***Timeout 120.02 sec` | 120.0 s ceiling | `tsan-test.log:360`, `diag-props.log:5` |
| `build/tsan` | `base_cli_matrix` | isolated | `***Timeout 120.13 sec` | 120.0 s ceiling | `alone-tsan-base_cli_matrix.log:11` |
| `build/tsan` | `base_cli_matrix` | bounded | `***Timeout 120.04 sec` | 120.0 s ceiling | `bounded4-tsan.log:15` |
| `build/tsan` | `example_run` | suite | `***Timeout 300.03 sec` | 300.0 s ceiling | `tsan-test.log:378`, `diag-props.log:10` |
| `build/tsan` | `example_run` | isolated | `***Timeout 300.03 sec` | 300.0 s ceiling | `alone-tsan-example_run.log:11` |
| `build/tsan` | `example_run` | bounded | `***Timeout 300.04 sec` | 300.0 s ceiling | `bounded4-tsan.log:19` |
| `build/tsan` | `harness_hunt_cli_test` | suite | `***Timeout 300.06 sec` | 300.0 s ceiling | `tsan-test.log:376`, `diag-props.log:15` |
| `build/tsan` | `harness_hunt_cli_test` | isolated | `***Timeout 300.15 sec` | 300.0 s ceiling | `alone-tsan-harness_hunt_cli_test.log:11` |
| `build/tsan` | `harness_hunt_cli_test` | bounded | `***Timeout 300.04 sec` | 300.0 s ceiling | `bounded4-tsan.log:17` |
| `build/tsan` | `harness_jitter_cli_test` | suite | `***Timeout 300.13 sec` | 300.0 s ceiling | `tsan-test.log:407`, `diag-props.log:20` |
| `build/tsan` | `harness_jitter_cli_test` | isolated | `***Timeout 300.07 sec` | 300.0 s ceiling | `alone-tsan-harness_jitter_cli_test.log:11` |
| `build/tsan` | `harness_jitter_cli_test` | bounded | `***Timeout 300.04 sec` | 300.0 s ceiling | `bounded4-tsan.log:43` |
| `build/tsan` | `report_shape_fwq_ftq_sample` | suite | `***Failed  110.27 sec` | 110 s grant at `tools/docs/report_shape.py:368` (read at `a55728af`) | `tsan-test.log:363`, `report_shape.py:368` (read at `a55728af`) |
| `build/tsan` | `report_shape_fwq_ftq_sample` | isolated | `Passed   79.05 sec` | 110 s grant at `tools/docs/report_shape.py:368` (read at `a55728af`) | `alone-tsan-report_shape_fwq_ftq_sample.log:11` |
| `build/tsan` | `report_shape_fwq_ftq_sample` | bounded | `Passed  101.74 sec` | 110 s grant at `tools/docs/report_shape.py:368` (read at `a55728af`) | `bounded4-tsan.log:14` |

The grant column names the bound that binds. `report_shape_fwq_ftq_sample` carries a 120.0 s
CTest ceiling at `diag-props.log:25`, and that ceiling binds nothing: the driver it runs grants
the example it launches 110 s of its own at `tools/docs/report_shape.py:368` (read at `a55728af`), so the suite row
stops at the grant, and both passes sit under it. Every other name cuts at its CTest ceiling in
all three shapes, at 120.0 s for `base_cli_matrix` and at 300.0 s for the other three
(`diag-props.log:5`, `:10`, `:15`, `:20`). The props step answered for the thread tree, and the
address lane prints its own crossings at the same two figures, 120.04 s and 120.13 s above a
120 s mark and 300.01 s to 300.06 s above a 300 s mark.

No isolated or bounded run clears a name of the verdict its lane printed. IC-003 records a run
that passes on a retry as a diagnostic (`spec.md:1617-1624` (read at `af54d3f0`)), and G-20 holds that a host
characteristic explains nothing on its own (`spec.md:1687-1689` (read at `af54d3f0`)). The two passes above stand as
diagnostics beside a red lane row. Each bounded lane ended red, and each isolated log ended at
exit 8 (`alone-sanitize-base_cli_matrix.log:23`, `alone-tsan-base_cli_matrix.log:24`). The
bounded thread lane printed:

```text
20% tests passed, 4 tests failed out of 5
Total Test time (real) = 600.09 sec
```

`bounded4-tsan.log:46` and `:51` hold those two lines. The bounded address lane printed:

```text
0% tests passed, 2 tests failed out of 2
Total Test time (real) = 300.02 sec
```

`bounded4-sanitize.log:46` and `:51` hold those two lines.

The walk price stands beside the child counts. A launch of an example that only prints takes
2.19 s to 2.39 s in the address and undefined behaviour tree (`walk-price.log:3` to `:11`), and
1.67 s to 1.85 s in the release tree (`walk-price.log:13` to `:18`). `base_cli_matrix` drives 26
child launches: `test/data/base_cli_matrix.txt` holds 26 `case` rows, and the hand run printed
`26 of 26 with all rows matching` (`hand-base_cli_matrix.log:11`). `example_run` drives 74: the
driver launches each example twice, once for `--list` at `test/example_run.sh:48` (read at `a55728af`) and once for
the run at `:57`, and the hand run printed 37 `PASS:` lines (`hand-example_run.log:11` to
`:47`). The cause this closeout defers sits in two files it does not own:
`source/harness/cli.cpp:707-710` (read at `a55728af`) offers the PMU provider before the command-line scan at `:717`,

and `source/counters/system.cpp:230-246` (read at `a55728af`) holds the walk's seed loop, so every one of those
launches pays the walk. Both files stand on the excluded list at `spec.md:1522-1530` (read at `af54d3f0`), so this
closeout records the reproduction, defers the change, and leaves the row open under ruling 4
(`spec.md:1780-1785` (read at `af54d3f0`)) and ruling 8 (`spec.md:1802-1809` (read at `af54d3f0`)).

### Coverage: two shapes, two denominators (`IC-002`, `SC-IC-3`)

Two shapes carry branch counts, and they carry different denominators. This host's frozen
trace holds 9,273 branch positions at `cov-gate.log:13`, and the hosted runner's trace holds
9,823 at `cov-5fe3d244.log:2665`. The two denominators stand 550 positions apart, so a
comparison holds only inside one shape.

Shape one, this host, over 9,273 positions:

| Head | Rows | Exit | Source |
|---|---|---|---|
| `87d745d6` | 24 | 1 | `record:467` (read at `a55728af`) |
| `2d4d8c57` | 20 | 0 | `cov-gate.log:16`, `:17` |
| the closeout head | no figure | | the coverage chain in a clean `build/coverage` at that head, then `bash tools/dbc/coverage_gate.sh build/coverage/coverage.info` |

The repair between the two measured heads moved 4 rows here: 24 rows at `record:467` (read at `a55728af`) and
20 rows at `cov-gate.log:16`. The 20-row reading comes from the chain of pull request #99,
and this closeout cites that chain, since this closeout ran no coverage lane of its own.
That chain ran at `2d4d8c57`, the base head of this closeout, and the tracked delta of
that pull request over it is docs-only: its own header line names `2d4d8c57`
(`/tmp/ia3/phase1.sh:2`), and its pull request body states that the code tree reads
byte-identical to `2d4d8c57` (`/tmp/ia3/pr_body.md:6`). The gate answers with the line
below and exit 0 at `cov-gate.log:17`.

```text
coverage-gate: branch shortfall 20 rows, within the  D-14 recorded allowance of 21 (accepted classes)
```

The chain behind that reading ran the serial suite first. It answered
`98% tests passed, 4 tests failed out of 185` in 3,978.64 s at exit 8
(`cov-test.log:395`, `:401`, `:409`). The four cut entries then ran again by hand, each from
its own working directory and with no ceiling over it, and each answered exit 0. Their
elapsed time comes from the start and end stamps of each log.

| Entry | Elapsed | Exit | Source |
|---|---|---|---|
| `base_cli_matrix` | 285 s | 0 | `hand-base_cli_matrix.log:2`, `:12`, `:13` |
| `harness_jitter_cli_test` | 509 s | 0 | `hand-harness_jitter_cli_test.log:2`, `:42`, `:43` |
| `harness_hunt_cli_test` | 571 s | 0 | `hand-harness_hunt_cli_test.log:2`, `:12`, `:13` |
| `example_run` | 775 s | 0 | `hand-example_run.log:2`, `:48`, `:49` |

The capture then read `lines.......: 100.0% (18664 of 18664 lines)`,
`functions...: 99.0% (1958 of 1978 functions)` and `branches....: 99.8% (9253 of 9273 branches)`
with `no messages were reported`, at `trace-counts.log:10`, `:11`, `:12` and `:14`.

Shape two, the hosted runner, over 9,823 positions. Every row below answers exit 2 and ends
with `gmake: *** [Makefile:252: coverage] Error 2`.

| Head | Run | Coverage job | Rows | Source |
|---|---|---|---|---|
| `87d745d6` | `42450370778` | `128638209199` | 27 | `cov-87d745d6.log:2790`, `:2821`, `:2822` |
| `b90711fb` | `42487329495` | `128752079417` | 23 | `cov-b90711fb.log:2790`, `:2817`, `:2818` |
| `ab1102e5` | `42495085606` | `128775782974` | 23 | `cov-ab1102e5.log:2792`, `:2819`, `:2820` |
| `2d4d8c57` | `42505274033` | `128806578980` | 23 | `cov-2d4d8c57.log:2775`, `:2802`, `:2803` |
| `5fe3d244` | `42520411116` | `128852626791` | 23 | `cov-5fe3d244.log:2791`, `:2818`, `:2819` |

Shape two answers with the line below at `cov-5fe3d244.log:2791`.

```text
coverage-gate: branch shortfall 23 rows exceeds the  D-14 recorded allowance of 21
```

The CI coverage job stands red at `2d4d8c57`, which is master's own run at the base head of
this closeout, so it stands red before this closeout touches anything. Two hosted runs of
identical source at pull request #98 answered 27 rows and then 23 rows over the same 9,823
positions (`record:484-487` (read at `a55728af`)), so the count of cold rows carries a spread of its own. Ruling 28
reads a count within a couple of rows of a standing figure as a normal repeat of it
(`spec.md:1916-1922` (read at `af54d3f0`)), and four heads above `87d745d6` answer the same 23 rows. The allowance stands at 21, and `tools/dbc/coverage_gate.sh:108` (read at `a55728af`)
holds `allowance=21`, with its class ledger at `tools/dbc/coverage_gate.sh:61` (read at `a55728af`) to `:107` and
D-14 in `specs/030-ci-speedup/spec.md`.

At line level, the hosted list at `5fe3d244` names 23 arms across 13 source lines
(`cov-5fe3d244.log:2792` to `:2814`). The frozen local trace
`/tmp/ic/final/base-coverage/coverage.info`, read with the extract step's own flags
(`lcov --branch-coverage --filter exception,branch,brace` with the two `--omit-lines` patterns
and the three extract roots of `cmake/coverage.cmake:112-125`), leaves 11 of those 13 lines
with zero-hit arms, and `source/counters/linux_pmu/group_io.cpp:395` (read at `a55728af`) and `:439` stand dark
only in the hosted list. That is a line-level observation, and a count comparison of 20 against
23 has no place in it, because the gate counts decision arms after lcov's filters. The frozen
trace holds 332 branch records at a zero count over 9,897 branch records
(`trace-counts.log:3`, `:5`), while lcov's own arithmetic on that same file leaves 20
uncovered positions over 9,273 (`trace-counts.log:12`), so the two lists are not comparable
record for record. The three-row gap between 20 and 23 stands unresolved to those
two lines from the artifacts this closeout holds.

### The coverage target, blocked at its HTML step (`ruling 3`)

The target ran unchanged from the repository root and answered exit 2
(`cov-target.log:3`, `:472`). Its last tool line reads:

```text
ERROR: required module GD.pm not found on this system (see www.cpan.org).
```

`cov-target.log:467` holds that line, and `cov-target.log:468` to `:471` hold the make faults
under it. The host holds perl v5.44.0 (`perl-gd.log:24`, `:25`) and lacks the module:
`perl -MGD -e1` answers `Can't locate GD.pm in @INC` at exit 2 (`perl-gd.log:20`, `:21`, `:23`).
The distribution carries `cachyos-extra-v4/perl-gd 2.91-2.1`, `extra/perl-gd 2.91-2` and
`extra/gd 2.3.3-9` (`perl-gd.log:31`, `:32`, `:34`, `:36`), and neither package stands
installed: `pacman -Qi perl-gd` and `pacman -Qi gd` each answer `was not found` at exit 1
(`perl-gd.log:4` to `:9`). The step reads
BLOCKED with the operator action `pacman -S perl-gd gd`, and the gate keeps its own verdict
from its own run, as ruling 3 requires (`spec.md:1774-1779` (read at `af54d3f0`)).

### Three deferred items outside the owned files

`spec.md:1522-1530` (read at `af54d3f0`) excludes the first two files below, and `spec.md:2069-2082` (read at `af54d3f0`) keeps
`specs/048-hwlat-probe/results/gates.md` outside the owned list, so each row carries its
reproduction and no figure of this closeout's own.


| Item | File | Reproduction |
|---|---|---|
| The hosted shortfall against the allowance | `tools/dbc/coverage_gate.sh:61` (read at `a55728af`) to `:108`, with `allowance=21` at `:108` | run the coverage lane at that head, then `bash tools/dbc/coverage_gate.sh build/coverage/coverage.info` from the repository root |
| The 550-position gap between the two denominators | `cmake/coverage.cmake:112-125`, the extract step both shapes share | compare `lcov --branch-coverage --summary build/coverage/coverage.info` on this host with the `Summary coverage rate` block of any hosted coverage log above |
| The escaped pipe in a printed gate command (2026-11-09) | `specs/048-hwlat-probe/results/gates.md:23` (read at `a55728af`), a file this phase may not change | copy that cell's raw text into bash from the repository root: bash parses the backslash-escaped pipe, so the copy raises no syntax error, and the pipe word plus `tail`, `-n` and `1` reach `ctest` as arguments, so no `tail` step reads the entry-count answer; `bash -n` on that copy confirms the parse, and `ctest --test-dir build -N` read at its last line gives the reading the cell means |

### The two hosted test jobs at the base head (`ruling 19`)

Master's own push run `42505274033` stands at `2d4d8c57`, and its two test jobs split.
`/tmp/ic/base/hosted` holds both job logs and the run's job list.

| Job | Job id | Answer | Source |
|---|---|---|---|
| `test` | `128806578854` | success, `100% tests passed out of 190` in 366.75 s | `meta-42505274033.json`, `test-2d4d8c57.log:2420`, `:2426` |
| `test-rocky` | `128806578926` | failure at exit 8, `99% tests passed, 2 tests failed out of 190` in 388.62 s | `rocky-2d4d8c57.log:3491`, `:3497`, `:3508` |

The first cause is `prose_gate_fixtures`, entry 30, and its status line reads:

```text
 26/190 Test  #30: prose_gate_fixtures ............................***Failed    2.16 sec
```

`rocky-2d4d8c57.log:2978` holds that line, and `rocky-2d4d8c57.log:3131` holds the line it ends
on:

```text
git ls-files failed in /__w/speedgun-ng/speedgun-ng (exit 128): fatal: detected dubious ownership in repository at '/__w/speedgun-ng/speedgun-ng'
```

The remedy that message names stands at `rocky-2d4d8c57.log:3134`:

```sh
git config --global --add safe.directory /__w/speedgun-ng/speedgun-ng
```

The container refuses a `git` call that arrives from a different user id, so the row reads as an
environment fault, and the remedy belongs to the workflow or the job step. No CI file stands
inside this closeout's file list (`spec.md:1522-1530` (read at `af54d3f0`)), so this row carries no figure. The same
entry passes in the Ubuntu job of the same run in 9.90 s (`test-2d4d8c57.log:2211`).

The second cause is `harness_hunt_topdown_test`, entry 190, and its status line reads:

```text
171/190 Test #190: harness_hunt_topdown_test ......................***Failed    0.14 sec
```

`rocky-2d4d8c57.log:3424` holds that line. One scenario fails, and the two lines it prints read:

```text
HARNESS HUNT TOPDOWN TEST FAIL: three calls leave the process holding the mappings it held before, so no sampled page stands behind an area (FR-001, D-7)
harness_hunt_topdown_test oneCallReadsNoSourceOfItsOwn: FAIL
```

`rocky-2d4d8c57.log:3439` and `:3440` hold those two lines. The entry passes in the Ubuntu job of
the same run in 0.26 s (`test-2d4d8c57.log:2379`), so the reading stands Rocky-only. Its cause
stands in a test file outside the file list at `spec.md:1522-1530` (read at `af54d3f0`), so the row stands open as a
follow-up. `record:587` (read at `a55728af`) carries the same two names as an open difference between the two
runners at `87d745d6`.

This closeout read both failures from the hosted logs above and ran nothing inside a container,

so neither row claims a local reproduction, and a hosted run informs a diagnosis while local runs
decide acceptance (`spec.md:1687-1689` (read at `af54d3f0`)). At `ab1102e5` the Rocky job ends on a repository
metadata fault, `Error: Failed to download metadata for repo 'epel'` at
`rocky-ab1102e5.log:504` with exit 1 at `:505`, and that reading stays history. At the base head
the toolchain step completes at `rocky-2d4d8c57.log:1477`, and that log holds no
`Failed to download metadata` line.

### Corrections to the sections above (`IC-006`)

`record` below names this file as it stands above this heading, `041 record` names
`specs/041-counter-probe/results/acceptance-final-intel.md`, `plan` names
`specs/048-hwlat-probe/plan.md`, and a bare number names the file its row names. Each row cites the
line, the words that line carries, the reading that answers it, and the row it replaces. No row above
changes its text, its heading or its mark; IC-006 keeps a false line standing and puts the replacement
here (`spec.md:1645-1655` (read at `af54d3f0`)). `/tmp/ic/w3-hwlat.md` holds the read that found these defects.
This section was written against `af54d3f0`, the head this branch carries. A bare
record number names a line at `af54d3f0`, and a line at or below 640 stands at the
merged base `a55728af` too. A `/tmp/ic/` reading names the pass that took it.

| File | Line | What the line says | What a run or the spec says | Row it replaces |
|---|---|---|---|---|
| record | 454-455 | "The restore never writes a prior value back, and `tracing_thresh` keeps the run threshold as the kernel's remembered default" | FR-021 makes the restore write `tracing_on` at 0 and write each saved value back to the file it came from (`spec.md:750-757` (read at `a55728af`)), `noiseTracerRestore` writes `0` then writes the saved set back (`source/harness/noise_tracer.cpp:624` (read at `a55728af`), `:626`, through `writeSavedBack` at `:268-277`), and the test states the threshold back at the host value (`test/source/harness_hwlat_tracer_test.cpp:658-659` (read at `a55728af`)). D-8 reads "The restore writes each saved value back" (`spec.md:302` (read at `a55728af`)). Ruling 2 of the 2026-11-07 rulings names `tracing_on` as the file whose prior value the restore never writes (`spec.md:1291-1294` (read at `a55728af`)). | the restore sentence beside the operator block in "The root rows, still blocked" |
| record | 191-193 | "the kernel leaves `tracing_thresh` at the value the run wrote, and no user-space read reaches it on this host" | The restore writes that file back (`source/harness/noise_tracer.cpp:626` (read at `a55728af`), `test/source/harness_hwlat_tracer_test.cpp:658-659` (read at `a55728af`)). D-8 names the residue as the threshold a later run takes when it names none (`spec.md:310-313` (read at `a55728af`)), and plan R-4 says no user-space write reaches it (`plan:355-361` (read at `a55728af`)). | the threshold-residue sentence in "The root rows" |
| record | 414 | Reading cell reads `refusal text` for `method=smi` | The run prints `--jitter-probe: the method 'smi' is not tsc, fwq, ftq or hwlat` at exit 2, the text built at `source/harness/jitter_probe.cpp:1935-1936` (read at `a55728af`) and printed with its prefix at `source/harness/cli.cpp:855-863` (read at `a55728af`), as record:128 already prints. SC-IA-1 wants the output on the row (`spec.md:1207-1209` (read at `a55728af`)). | the Refusal a row of the rerun refusal table |
| record | 415 | `--os-noise=hwlat-detect` reads `--os-noise: the batch carries no pre-check window for the tracer` | That command reaches the engine-word refusal: the parser builds `the engine '<word>' is not osnoise, timerlat or hwlat` at `source/harness/noise_tracer.cpp:66-67` (read at `a55728af`), `source/harness/options.cpp:1896-1897` (read at `a55728af`) refuses, and `source/harness/cli.cpp:719-720` (read at `a55728af`) prints it at exit 2. The missing-window line comes from `refuseMissingPrecheckWindow` at `source/harness/cli.cpp:191-193` (read at `a55728af`) and `source/harness/cli.cpp:799-800` (read at `a55728af`), which an engine word the parser rejects can never reach. IC-006 asks the two shapes to read differently (`spec.md:1645-1655` (read at `af54d3f0`)). | the Refusal b row of the rerun refusal table |
| record | 416 | Reading cell reads `refusal text` for `method=hwlat` with `--os-noise` | That command reaches the missing-window line `--os-noise: the batch carries no pre-check window for the tracer` (`source/harness/cli.cpp:191-193` (read at `a55728af`), `:799-800`), as record:130 already prints. | the Refusal c row of the rerun refusal table |
| record | 218 | the tree-configured interpreter as an absolute path under a user home directory, CPython 3.11.16, no PyYAML | IA-END-003 bars a path that names a user's home directory and asks for a build-relative or `$HOME`-relative path (`spec.md:1198-1203` (read at `a55728af`)), and closeout ruling 27 keeps that rule for every new row (`spec.md:1911-1914` (read at `af54d3f0`)). The host fact stands unchanged in `$HOME/.local/bin/python3.11`. | the gate target interpreter row of "Versions the lane used" |
| record | 340 | the same absolute interpreter path beside the `dbc-gate` fault | The same two clauses: `spec.md:1198-1203` (read at `a55728af`) and `spec.md:1911-1914` (read at `af54d3f0`). | the DBC target row of the gate table |
| record | 561 | the same absolute interpreter path, "a host fact the first pass recorded too" | The same two clauses: `spec.md:1198-1203` (read at `a55728af`) and `spec.md:1911-1914` (read at `af54d3f0`). | the gate sentence in "The gates" of the rerun |
| record | 54 | "`results/user-space.md` records that a reader without rights could not read `/proc/interrupts`" | That file does not exist; `specs/048-hwlat-probe/results/` holds `acceptance-final-intel.md` and `gates.md`. Ruling 6 of the 2026-11-07 rulings names both `results/user-space.md` and `results/detector.md` as files that never stood written (`spec.md:1307-1309` (read at `a55728af`)), and ruling 40 keeps T054 to T056 unrun (`spec.md:1463-1464` (read at `a55728af`)). The standing statement lives at assumption A-5 (`spec.md:1040-1042` (read at `a55728af`)) and at `quickstart.md:44-49` (read at `a55728af`), and ruling 43 records that A-5 no longer holds on this host (`spec.md:1473-1476` (read at `a55728af`)). | the sentence under the host-facts table |
| record | 437-448 | the operator block reads each file with plain `cat` | The quickstart helper strips the mode list down to the active word with `sed` (`quickstart.md:61-65` (read at `a55728af`)), ruling 33 asks for the mode as that helper prints it and without brackets (`spec.md:1431-1435` (read at `a55728af`)), and the constraint on this phase repeats it (`spec.md:1706-1707` (read at `af54d3f0`)). `cat` on `hwlat_detector/mode` prints the whole list, for example `none [round-robin] per-cpu` (`test/source/harness_hwlat_tracer_test.cpp:1822-1823` (read at `a55728af`)). | the operator block in "The root rows, still blocked" |
| record | 177-185 | a four-row phase table plus one sentence that names the six values | SC-IA-3 asks for the six saved values as six separate rows with the value before the run and the value after it (`spec.md:1213-1217` (read at `a55728af`)), and closeout ruling 24 asks for those six rows beside one residue row that carries no figure (`spec.md:1897-1901` (read at `af54d3f0`)). | the four-row blocked table and its following sentence |
| record | 339 | "Prose, range \| after the squash \| \| \|" with no exit and no reading | Ruling 31 wants the range form after the commit with its base sha and no mark where a form examined nothing (`spec.md:1417-1419` (read at `a55728af`)), and ruling 34 puts the unit count behind the mark (`spec.md:1436-1440` (read at `a55728af`)). | the Prose, range row of the gate table |
| record | 351 | the Release build row holds `SC-004` in its Reading cell | SC-IA-1 wants a command, an exit code and an output on the row (`spec.md:1207-1209` (read at `a55728af`)); a criterion id names no build output. | the Release build row of the gate table |
| record | 342-343 | both DBC gate commands stand cut with `...` | Ruling 30 keeps the argument list whole and prints the interface count (`spec.md:1413-1416` (read at `a55728af`)). | the two DBC gate rows of the gate table |
| record | 300-301, 344-346 | command cells read "block B of the plan", "block C of the plan" and "block E, first, second and third scope" | Ruling 25 wants the sanitizer option strings copied out of the workflow (`spec.md:1393-1395` (read at `a55728af`)) and ruling 28 wants the counts out of the log beside the scope (`spec.md:1404-1408` (read at `a55728af`)). | the two sanitizer rows of the suite-lane table and the three analyzer rows of the gate table |
| record | 465 | "Four cut entries, by hand, no ceiling \| 0 each \| 285 s, 508 s, 571 s, 776 s" | Ruling 26 names each extra command with its exit and elapsed time (`spec.md:1396-1399` (read at `a55728af`)); the four entry names stand only at record:238-248, and the four times this head printed read 285 s, 509 s, 571 s and 775 s. | the hand re-run row of the rerun coverage table |
| record | 491-497 | the five rerun lane rows carry Lane, Exit, Entries and Result | Ruling 24 keeps the command in the row because parallelism decides verdicts on this host (`spec.md:1386-1392` (read at `a55728af`)), and the first pass prints those commands at record:297-301. | the five rows of the rerun suite-lane table |
| record | 558-561 and 563-572 | "Forty-seven steps ran in the order of the plan" and a gate table with no Command column | SC-IA-1 wants a command with its exit and its output on every row (`spec.md:1207-1209` (read at `a55728af`)); the first pass prints one command per gate row at record:331-353. | the gate paragraph and the whole rerun gate table |
| record | 481-487 | hosted coverage readings inside the host coverage lane, with no job id | IA-END-001 bars a CI result as acceptance (`spec.md:1178-1182` (read at `a55728af`)), and closeout G-20 keeps a hosted run inside a diagnosis (`spec.md:1687-1689` (read at `af54d3f0`)). The hosted figures carry no job id or run id. | the four sentences that begin "The hosted coverage job fails too" |
| record | 543-548 | five hosted entry times | The closeout assumption keeps a hosted count with the job that answered it (`spec.md:1747-1748` (read at `af54d3f0`)). The row names no job. | the hosted-time sentence in "The sanitizer diagnosis" |
| record | 522-524 | `harness_hunt_cli_test` "passed inside the 24-job lanes at 289.30 s and at 281.75 s" | No lane row in that record prints a per-entry time, and this head times the name out inside the thread lane at 300.06 s (tsan-test.log:376, tsan:421). | the spread sentence in "The sanitizer diagnosis" |
| record | 405-406 | "Each run stood alone with its load witness." | Closeout ruling 17 prints a load witness with each measurement and voids a figure taken while another lane runs (`spec.md:1852-1855` (read at `af54d3f0`)); no rerun row prints a load figure, and the first pass prints them at record:25-28. | the sentence above the rerun unprivileged table |
| record | 16 and 385-388 | "Measured head \| `615c0964…` \| `git rev-parse HEAD`" beside "every measurement above ran at a tree whose code matches `615c0964…`" | Ruling 10 labels each reading with the head it ran at (`spec.md:1323-1325` (read at `a55728af`)); one table row cannot hold both the base sha and a branch commit under `git rev-parse HEAD`. | the Measured head row of the Identity table |
| record | 391-399 | "Rerun at head `87d745d6…`" | G-23 keeps evidence from an older head out of final acceptance (`spec.md:1714-1718` (read at `af54d3f0`)), closeout ruling 10 keeps an `87d745d6` figure labeled as history (`spec.md:1816-1819` (read at `af54d3f0`)), and `e2f6a50f Harness: Clear the capture suppression` stands between that head and `2d4d8c57`. | the head paragraph of "Rerun at head `87d745d6…`" |
| record | 102-103 | "FR-023 asks for the disclosure line where the tracer stands busy" | FR-023 names one disclosure line and attaches it to the last of its four causes, `current_tracer` already reading `hwlat` (`spec.md:763-770` (read at `a55728af`)), and `source/harness/report.cpp:1702-1708` (read at `a55728af`) prints that line only beside a busy tracer. | the two sentences under the engine row |

| record | 182 | "IA48-005.2 six values after a controlled interruption \| blocked \| none" | IA48-003 asks a blocked row to name the right and the action (`spec.md:1146-1148` (read at `a55728af`) as record:165-175 applies it), and the first pass names no signal command there; the `kill -INT` step appears only at record:451-455. | the IA48-005.2 row of the blocked table |
| record | 383-384 | "Tasks T054 to T056 stand unrun … the second needs a root session this host withholds" | SC-IC-1 asks a row that no run answered to read BLOCKED with the right it lacks (`spec.md:1666-1668` (read at `af54d3f0`)), and closeout User Story 3 asks for the command an operator runs (`spec.md:1586-1587` (read at `af54d3f0`)). The row names the right and no command. | the T054 to T056 bullet of "What this record does not claim" |
| record | 45 and 47-49 | three denied inspection reads with no operator action | The rows answer SC-IA-1 because each prints its command and exit (`spec.md:1207-1209` (read at `a55728af`)). IA48-003 asks for an action on the two SC-005 surfaces only (`spec.md:1140-1148` (read at `a55728af`)), so these three stay inside that rule and gain an action for the reader who wants the value. | the three host-facts rows |
| 041 record | 96-97 | "The parity row stays blocked for the same reason as above: this session holds no right over `/dev/cpu/*/msr`" | The parity row turns on the write to `/proc/sys/kernel/perf_event_paranoid` that 041 record:64-68 names, and IC-006 states that an unreadable `msr` node explains no refusal of ordinary perf group access (`spec.md:1645-1655` (read at `af54d3f0`)). | the parity sentence of the rerun counter section |
| quickstart | 172-173 | "the record carries the seven tracefs reads that belong before and after each run" | Ruling 33 replaced "snapshot seven" with the six saved values plus `tracing_on` as a reading of its own (`spec.md:1431-1435` (read at `a55728af`)). | the sentence above the two root commands |
| plan | 725 | "the two `quickstart.md` runs of lines 173 and 175" | The two commands stand at `quickstart.md:176-177` (read at `a55728af`) and `quickstart.md:178-179` (read at `a55728af`). | the Hwlat root runs row of the plan's command table |
| spec.md | 1876-1887 | "A research pass read two matrix rows as broken, on the reading that each side stops at a usage fault and so compares two failures" | The tracked data answers both rows on their own terms: `test/data/base_cli_matrix.txt:10` reads `case10 | --percentiles | 2 | option '--percentiles' needs a value` and `test/data/base_cli_matrix.txt:21` reads `case21 | --min-time | 2 | option '--min-time' needs a value`, so each row names the fault text it expects and the driver compares a refusal with its own expected line. No row of that file names `--interval` or `-r`. | nothing above, because no tracked line carried the claim |

### Root-surface rows (`IA48-003`, `SC-IA-2`)

Two routes to a privileged shell stand closed on this host. `sudo -n -l` answers
`sudo: a password is required` (`identity.log:49`), and `pkexec --disable-internal-agent id`
answers `Error executing command as another user: No authentication agent found.`
(`identity.log:50`). The file groups that need that shell carry the matching modes:
`/dev/cpu/0/msr` reads `crw------- 1 root root 202, 0` (`identity.log:43`),
`/sys/kernel/tracing` reads `drwx------ 9 root root` (`identity.log:45`), and
`/sys/kernel/debug` reads `drwx------ 65 root root` (`identity.log:44`). This session runs as
`uid=1000`, and `identity.log:36` lists `998(wheel)` among its groups. The root rows stay
blocked because `sudo -n -l` answers that a password is required (`identity.log:49`) and
`pkexec` finds no authentication agent here (`identity.log:50`). The session collects no
password, automates no elevation, loads no module and adds no tracing group
(`spec.md:1701-1703` (read at `af54d3f0`)). Every row below reads BLOCKED, names the reason in words an operator can
act on, and carries no figure, as IA48-003 requires where root stands absent
(`spec.md:1140-1148` (read at `a55728af`)) and as closeout User Story 3 asks (`spec.md:1594-1596` (read at `af54d3f0`)).

The commands print without their `sudo` prefix, as ruling 3 of the 2026-11-07 rulings requires
(`spec.md:1295-1298` (read at `a55728af`)). Each runs inside the privileged shell the operator opens, from the
directory that holds `build/example`. Ruling 8 of that section stops the row where
`current_tracer` reads a word other than `nop` or where `tracing_on` reads 1
(`spec.md:1313-1316` (read at `a55728af`)). The first block below reads the six saved values with the quickstart
helper, so the mode lands as one word without brackets (`quickstart.md:60-67` (read at `a55728af`),
`spec.md:1431-1435` (read at `a55728af`)):

```sh
T=/sys/kernel/tracing
snap() { for f in current_tracer tracing_cpumask tracing_thresh \
           hwlat_detector/mode hwlat_detector/width hwlat_detector/window; do
           printf '%s\t%s\n' "$f" \
             "$(sed -e 's/.*\[\([^]]*\)\].*/\1/' $T/$f)"
         done; }
snap > /tmp/tracing-before.txt
printf 'tracing_on %s\n' "$(cat $T/tracing_on)" > /tmp/tracing-on-before.txt

./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle,mode=precheck --os-noise=hwlat --json=/tmp/hwlat-detector.json

snap > /tmp/tracing-after.txt
printf 'tracing_on %s\n' "$(cat $T/tracing_on)" > /tmp/tracing-on-after.txt
diff /tmp/tracing-before.txt /tmp/tracing-after.txt && echo restored
```

The second block runs the same reads around a run the operator ends, and it adds the signal step
and the shell status of that run:

```sh
snap > /tmp/tracing-before-2.txt
./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle,mode=precheck --os-noise=hwlat --json=/tmp/hwlat-detector-2.json &
run_pid=$!
kill -INT "$run_pid"
wait "$run_pid"; echo "shell status $?"
snap > /tmp/tracing-after-2.txt
diff /tmp/tracing-before-2.txt /tmp/tracing-after-2.txt && echo restored
```

The third block is the other root surface, the `method=hwlat` run, which stands at
`quickstart.md:176-177` (read at `a55728af`). That run is the one shape that opens a model-specific-register node:
`source/harness/jitter_probe.cpp:618-622` (read at `a55728af`) opens it once per probe cpu for the `hwlat` method,
`source/harness/latency_counters.cpp:207-219` (read at `a55728af`) opens the node read-only, and
`source/harness/latency_counters.cpp:227-244` (read at `a55728af`) does one `pread` of 8 bytes at address `0x34`.

```sh
./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle,method=hwlat --json=/tmp/hwlat-method.json
```

| Row | Surface it needs | Operator command | Why this session stops | Figure |
|---|---|---|---|---|
| IA48-004.1 pre-flight on `current_tracer` and `tracing_on` | tracefs | `cat $T/current_tracer` and `cat $T/tracing_on` | the parent directory reads `drwx------ root root` (`identity.log:45`) | none |
| IA48-004.2 snapshot of the six saved values | tracefs | `snap > /tmp/tracing-before.txt` | the same directory mode | none |
| `tracing_on` beside those six | tracefs | `printf 'tracing_on %s\n' "$(cat $T/tracing_on)"` | the same directory mode | none |
| IA48-003.1 the `method=hwlat` run under root | tracefs and the model-specific-register nodes | the third block | a write to `current_tracer` and an open of `/dev/cpu/0/msr`, which reads `crw------- 1 root root 202, 0` (`identity.log:43`) | none |
| IA48-003.2 the pre-check run with `--os-noise=hwlat` under root | tracefs | the run line of the first block | a write to `current_tracer` inside the same directory | none |
| IA48-005.1 the six values after a normal end | tracefs | `snap > /tmp/tracing-after.txt` with the `diff` line beside it | the same directory mode | none |
| IA48-005.2 the six values after a controlled interruption | tracefs | the whole second block | the same directory mode, and the run itself needs the write above | none |
| A figure for the SMI counter of this host | the model-specific-register nodes | the third block | `openMsrNode` opens `/dev/cpu/0/msr` read-only (`source/harness/latency_counters.cpp:207-219` (read at `a55728af`)), and that open refuses this user | none |
| The parity row of `specs/041-counter-probe` | `/proc/sys/kernel/perf_event_paranoid` | `echo 2 > /proc/sys/kernel/perf_event_paranoid` | the file reads `-rw-r--r-- 1 root root` (`/tmp/ic/w2-probe.md:289`), and this host reads `-1` (`identity.log:34`) | none |
| The coverage report's HTML step | the perl GD module | `pacman -S perl-gd gd` | `pacman -S` needs the same privileged shell this session lacks | none |

The threshold residue keeps its own row and no figure, because a residue has no number to print
(`spec.md:1897-1901` (read at `af54d3f0`)). The planted-value row stays closed by the fixture tests, and the
real-counter reading stands BLOCKED with the root-only device file named (`spec.md:1902-1905` (read at `af54d3f0`)).
The parity row above names a write, because `041 record:64-68` (read at `a55728af`) names that write as the duty of
its own row, and IC-006 states that the historical reading at `perf_event_paranoid` 2 describes
a restricted environment, so an unreadable `msr` node on its own explains no refusal of ordinary
perf group access (`spec.md:1645-1655` (read at `af54d3f0`)).

### Figures this section leaves unsource

- `identity.log` holds no `uname -srmo` line, so the kernel row cites the earlier read-only pass
  at `/tmp/ic/w2-probe.md:282`.
- No cache read prints a `Python3_EXECUTABLE` line, because `san-cache.log:3` asks for one and the
  cache prints none, so the configured interpreter rests on CMake's own line at `san-cfg.log:83`
  and `tsan-cfg.log:83`.
- No props line prints a ceiling for `build/sanitize`, so the address lane's two ceilings rest on
  its own printed crossings, and `diag-props.log` answered for the thread tree.
- The coverage chain logs print no head sha of their own, so that chain's head rests on
  `/tmp/ia3/phase1.sh:2` and `/tmp/ia3/pr_body.md:6`.
- The closeout head carries no coverage figure, because the coverage lane has not run at it.
- The release suite, the two bounded release lanes, the gate commands and the `dbc-gate` target at
  this head stand outside the logs this section read, so those rows of the final-head set carry no
  count here (`spec.md:1869-1874` (read at `af54d3f0`)).
- The three-row gap between the local count and the hosted count stands unresolved, and the two
  denominators stand unreconciled.
- Both hosted Rocky failures stand read from the hosted logs, because this closeout ran nothing
  inside a container.

## Corrections at the merged base (2026-11-09)

This block was written at head `af54d3f0` (`af54d3f0e52901339e8703fc7991eb3da0db4435`), the head this branch carries, whose
base is the merged head `a55728af17ec02ea4fbe5e5ff7f8a537ddaa224a`. Each cite prints
the head it was read at:
`read at a55728af` for text the base carries, `read at af54d3f0` for text this
branch wrote, and `read in this wave's tree` for a row this wave wrote in
another record and no head carries yet. A bare `:n` in this block names the artifact the sentence names last,
and this file when it names none; a line of this file at or below 640 stands
at `a55728af` too, and a line above reads at `af54d3f0`.

SC-IC-5 of `Success Criteria - this extension` of
`specs/048-hwlat-probe/spec.md` (read at `af54d3f0`) owns the shape of this
section, and that id stands nowhere at `a55728af`. Ruling 27 of the 2026-11-08
closeout list of `specs/048-hwlat-probe/spec.md` (read at `af54d3f0`) keeps every
path out of a row a closeout writes, so the rows below print no path and quote one
cell alone. Every line a row names keeps its text and its heading, and each row
names the task row that carries it.

- **Row 1. The operator command of the parity row at `:1143`.** Owner:
  `IP-002` of the `Requirements restated for the merged base (IP-001 to
  IP-002)` list of `specs/041-counter-probe/spec.md` (read at `af54d3f0`), the
  clause that asks a blocked row for both command lines, and ruling 11 of the
  2026-11-08 closeout list of `specs/048-hwlat-probe/spec.md` (read at
  `af54d3f0`). Quoted claim, the operator command
  cell of `:1143`:

  ```text
  `echo 2 > /proc/sys/kernel/perf_event_paranoid`
  ```

  The pair that governs: the section `The operator command pair for the
  restricted reading (2026-11-09)` of
  `specs/041-counter-probe/results/acceptance-final-intel.md:201-223`
  (read in this wave's tree) carries the two command lines an operator types. `sysctl -w kernel.perf_event_paranoid=2`
  answers the restricted reading, and `sysctl -w kernel.perf_event_paranoid=-1`
  answers the way back. That pair names both directions, and the same row names
  the value the host answered before the change in its read-first step. The
  `echo` line above opens one direction and carries no restore line, so the
  `sysctl` pair governs the parity row of `specs/041-counter-probe`, and this row
  replaces that command cell. The figure cell of the line at `:1143` keeps
  `none`, because no run at the head this phase measures has answered the row.
  Carrier: `IA48-060` of `specs/048-hwlat-probe/tasks.md` (read in this wave's
  tree), the task row that lands both command lines in that cell.

- **Row 2. Two figures that carry no head label.** Owner: `SC-IC-7` of
  `Success criteria - this phase (SC-IC-7 to SC-IC-11)` of
  `specs/048-hwlat-probe/spec.md` (read at `af54d3f0`), the duty ruling 29 of
  `Rulings - this phase (29 to 42)` of the same spec states. The two figures are
  the four hand re-runs of the table under `Coverage: two shapes, two
  denominators`, at 285 s, 509 s, 571 s and 775 s each at exit 0, and the five red
  names of the table under `Three shapes per red name`: `base_cli_matrix`,
  `example_run`, `harness_hunt_cli_test`, `harness_jitter_cli_test` and
  `report_shape_fwq_ftq_sample`. Both tables printed at
  `2d4d8c57975a86db6c77536f7090aba2bdaf6417`. `Identity at the base head` reads
  that head from `identity.log:10`, the coverage prose names that head as the head
  the chain ran at before its four entries re-ran by hand, and the suite prose
  states that both instrumented lanes answered at the head named above them.
  Neither table holds a head cell, and both tables stand above this section, so
  this row carries the label and no line above it moves. `IA48-059` of
  `specs/048-hwlat-probe/tasks.md` (read in this wave's tree) reads every figure row
  against the two heads ruling 29 names, so the head cell inside those two tables
  lands with that row. This section now holds two rows, where the task line of
  `IA48-060` names one and states that the section holds no other row. That line
  stands in this wave's tree, and `IA48-062` stands unassigned among the reserved
  ids, so this row waits on the spec line that states which id carries it.

## Pass 1 control figures at the candidate head (2026-11-09)

Pass 1 of ruling 29 of `Rulings - this phase (29 to 42)` of
`specs/048-hwlat-probe/spec.md` (read at `af54d3f0`) ran the whole final-head set
at a candidate head. The run stood on `af54d3f0`, the short form of
`af54d3f0e52901339e8703fc7991eb3da0db4435`, the sha `git rev-parse af54d3f0`
prints, whose base is the merged head `a55728af`
(`a55728af17ec02ea4fbe5e5ff7f8a537ddaa224a`). `f0-identity.log` prints that sha
at exit 0 and prints one modified path, `.specify/feature.json`, from
`git status --porcelain`. Every figure below is a Pass 1 control figure measured
at `af54d3f0`, and a Pass 2 run at the head Pass 2 runs at replaces each of them,
so no row here closes a row of the final-head set at the head Pass 2 runs at. No
standing row moves and no standing table changes its text: a figure that belongs
beside a standing table lands here as a dated row that names that table by its
heading. Where two standing sections carry one heading, the dated row names the
dated section that holds it.

Each figure names one step log under `/tmp/ic/p1/`, and each log opens with its
own command line, its own load witness and its own exit. Ruling 30 of the same
list holds a figure where that lane's own witness line reports no other
compiler, test, capture or trace process and a one-minute load average below
2.0. Every witness of this pass reports `other ctest or compiler processes: 0`,
so the one-minute average decides each row, and the column `Ruling 30` carries
the answer beside every figure below.

**Figures ruling 30 voids.** Seven steps ran while the host carried work, and
ruling 30 reads each figure of those steps void. Each void figure stands quoted
as the step printed it, with its own load line, and Pass 2 re-takes each one
alone with its witness beside it.

| Figure | Step | Load witness |
|---|---|---|
| the coverage suite result | `f1-cov-test.log` | `19.62 13.44 6.04 2/1881 2604685` |
| the entry count of `build/sanitize` | `f2-san-count.log` | `15.04 6.41 3.23 2/1664 2708582` |
| the entry count of `build/tsan` | `f3-tsan-count.log` | `16.98 8.74 5.50 1/1718 2790214` |
| the entry count of `build` | `f4-rel-count.log` | `13.80 6.73 5.17 1/1626 2963290` |
| the analyzer test scope | `f7-analyzer-test.log` | `5.64 4.95 3.72 1/1566 3107318` |
| the analyzer example scope | `f7-analyzer-example.log` | `3.25 7.88 5.69 1/1578 3113339` |
| the analyzer finding count | `f7-analyzer-findings.log` | `6.41 8.34 5.89 3/1586 3114283` |

Two further steps carry a witness at or above that bound and carry no measured
figure at all, so the rule reaches no figure there. The counter reset of the
coverage chain printed `21.24 13.65 6.07 1/1881 2604661` and an exit code alone,
and the release build printed `2.00 3.83 4.27 1/1607 2952509` and an exit code
alone.

**The coverage chain.** Dated rows beside the section
`The coverage lane (IA-END-001.1, IA-END-001.7)`, which keeps its text. The chain ran in a fresh `build/coverage`
tree at `af54d3f0`.

| Step | Command | Exit | Reading at `af54d3f0` | Load witness | Ruling 30 |
|---|---|---|---|---|---|
| Configure | `rm -rf build/coverage && cmake --preset=ci-coverage` | 0 | a fresh `build/coverage` tree | `0.67 0.65 0.46 1/1751 2591046` | stands |
| Build | `cmake --build build/coverage -j "$(nproc)"` | 0 | the tree built | `0.70 0.66 0.46 1/1750 2591356` | stands |
| Zero | `lcov --zerocounters -d build/coverage` | 0 | `Deleting all .da files in build/coverage and subdirectories` | `21.24 13.65 6.07 1/1881 2604661` | void |
| Suite | `cd build/coverage && ctest -j 1 --output-on-failure --no-tests=error` | 8 | the suite row below | `19.62 13.44 6.04 2/1881 2604685` | void |
| Capture and gate | `cmake --build build/coverage -t coverage` | 2 | the gate line, then the picture step fault | `1.35 2.74 2.72 1/1744 2675115` | stands |
| Gate | `bash tools/dbc/coverage_gate.sh build/coverage/coverage.info` | 0 | the gate line below | `1.10 1.20 1.35 1/1633 2698148` | stands |
| Save | `cp -a build/coverage/coverage.info /tmp/ic/p1/coverage.info` | 0 | a frozen trace of 691840 bytes | `1.10 1.20 1.35 1/1633 2698194` | stands |

The suite step reads void, so the entry count, the four red names and the
elapsed total of that run carry no figure for the final-head set until Pass 2
re-takes the suite alone. The four cut entries each reached a ceiling: the
summary line reads `98% tests passed, 4 tests failed out of 185` and each of the
four lines names `base_cli_matrix`, `harness_jitter_cli_test`,
`harness_hunt_cli_test` and `example_run` with `(Timeout)`.

The gate line stands at its own witness:

```text
coverage-gate: branch shortfall 20 rows, within the  D-14 recorded allowance of 21 (accepted classes)
```

`f1-cov-gate.log` prints that line at exit 0 with the witness
`1.10 1.20 1.35 1/1633 2698148`, and the same file prints the denominators
`  source files: 87`, `  lines.......: 100.0% (18683 of 18683 lines)`,
`  functions...: 99.0% (1961 of 1981 functions)` and
`  branches....: 99.8% (9263 of 9283 branches)`. The trace under those lines is
the frozen file of the save step, and that file holds the counts the void suite
run left behind it, so the reading of 20 rows rests on a trace Pass 2 re-takes.
The row for the closeout head of the shape-one table of
`Coverage: two shapes, two denominators (IC-002, SC-IC-3)` keeps its `no figure`
cell for that reason.

The capture step answered exit 2 after printing the gate line, and its last tool
line reads:

```text
ERROR: required module GD.pm not found on this system (see www.cpan.org).
```

`f1-cov-target.log` prints that line, then four make faults, the last of them
`make: *** [Makefile:252: coverage] Error 2`, at the witness
`1.35 2.74 2.72 1/1744 2675115`. A dated row stands beside
`The coverage target, blocked at its HTML step (ruling 3)`, whose text stands
unchanged, and it repeats the host fact of the standing row: `f0-identity.log`
reads `error: package 'perl-gd' was not found` and
`error: package 'gd' was not found` from `pacman -Q perl-gd gd`.

**The entry count of each tree Pass 1 counted.** Dated rows beside the `Entries`
column of `The two instrumented lanes (IA-END-001, SC-IC-5)` and of
`The suite lanes (IA-END-001.2 to IA-END-001.6)` under
`Part D - the instrumented lanes and the gates`, both of which keep their text.
Each count comes from `ctest -N | tail -2` in the named tree.

| Tree | Step | Count line | Load witness | Ruling 30 |
|---|---|---|---|---|
| `build/sanitize` | `f2-san-count.log` | `Total Tests: 185` | `15.04 6.41 3.23 2/1664 2708582` | void |
| `build/tsan` | `f3-tsan-count.log` | `Total Tests: 186` | `16.98 8.74 5.50 1/1718 2790214` | void |
| `build` | `f4-rel-count.log` | `Total Tests: 190` | `13.80 6.73 5.17 1/1626 2963290` | void |

Every row above reads void, so no count of this pass answers the `Entry count`
row of `Lane Table` of `specs/048-hwlat-probe/plan.md` at `af54d3f0`. Pass 1
carried no count step for the coverage tree at all, so no count stands for
`build/coverage` in this pass, and the coverage lane of Pass 2 takes it with its
own witness.

**The suite of each tree, its bounded shape, and its second run.** Dated rows
beside `The two instrumented lanes (IA-END-001, SC-IC-5)` and beside the three
release rows of `The suite lanes (IA-END-001.2 to IA-END-001.6)` under
`Part D - the instrumented lanes and the gates`, both of which keep their text.
Each tree was wiped and configured by its own preset step, built with 24 jobs,
and then run in the shapes below at `af54d3f0`.

| Tree | Shape | Step | Exit | Summary line | Total test time | Red names | Load witness | Ruling 30 |
|---|---|---|---|---|---|---|---|---|
| `build/coverage` | the suite, `-j 1` | `f1-cov-test.log` | 8 | `98% tests passed, 4 tests failed out of 185` | `3932.83 sec` | `base_cli_matrix`, `harness_jitter_cli_test`, `harness_hunt_cli_test`, `example_run` | `19.62 13.44 6.04 2/1881 2604685` | void |
| `build/sanitize` | the suite, `-j "$(nproc)"` | `f2-san-test.log` | 8 | `99% tests passed, 2 tests failed out of 185` | `526.37 sec` | `base_cli_matrix`, `example_run` | `1.69 4.04 2.80 2/1667 2709804` | stands |
| `build/sanitize` | the four-cpu shape, `taskset -c 0,1,2,3` with `-j 4` | `f2-san-cpu03.log` | 8 | `98% tests passed, 3 tests failed out of 185` | `891.39 sec` | `base_cli_matrix`, `harness_hunt_cli_test`, `example_run` | `1.21 1.47 2.23 1/1633 2741792` | stands |
| `build/sanitize` | the suite a second time, `-j "$(nproc)"` | `f2-san-test2.log` | 8 | `99% tests passed, 2 tests failed out of 185` | `502.32 sec` | `base_cli_matrix`, `example_run` | `1.38 2.18 2.64 1/1697 2761854` | stands |
| `build/tsan` | the suite, `-j "$(nproc)"` | `f3-tsan-test.log` | 8 | `97% tests passed, 5 tests failed out of 186` | `749.79 sec` | `base_cli_matrix`, `harness_jitter_cli_test`, `harness_hunt_cli_test`, `report_shape_fwq_ftq_sample`, `example_run` | `1.58 5.37 4.71 1/1712 2791650` | stands |
| `build/tsan` | the four-cpu shape, `taskset -c 0,1,2,3` with `-j 4` | `f3-tsan-cpu03.log` | 8 | `98% tests passed, 4 tests failed out of 186` | `1534.78 sec` | `base_cli_matrix`, `harness_jitter_cli_test`, `harness_hunt_cli_test`, `example_run` | `1.20 1.47 1.88 1/1705 2862694` | stands |
| `build/tsan` | the suite a second time, `-j "$(nproc)"` | `f3-tsan-test2.log` | 8 | `98% tests passed, 4 tests failed out of 186` | `669.72 sec` | `base_cli_matrix`, `harness_jitter_cli_test`, `harness_hunt_cli_test`, `example_run` | `1.31 2.03 2.50 2/1655 2909225` | stands |
| `build` | the suite, `-j "$(nproc)"` | `f4-rel-test.log` | 0 | `100% tests passed out of 190` | `331.13 sec` | none | `1.44 4.26 4.47 1/1626 2964517` | stands |
| `build` | the narrow shape, `taskset -c 0,1` with `-j 2` | `f4-rel-cpu01.log` | 0 | `100% tests passed out of 190` | `1413.02 sec` | none | `1.96 5.10 5.15 5/1556 3001997` | stands |
| `build` | the four-cpu shape, `taskset -c 0,1,2,3` with `-j 4` | `f4-rel-cpu03.log` | 0 | `100% tests passed out of 190` | `904.41 sec` | none | `1.41 1.75 2.59 2/1568 3050744` | stands |

The instrumented suite rows, both bounded shapes and all three release shapes
stand at witnesses below 2.0. The address lane reported `base_cli_matrix` and
`example_run` in its first run and the same two names in its second run, and its
four-cpu shape added `harness_hunt_cli_test`. The thread lane reported five
names in its first run and four in its second: `report_shape_fwq_ftq_sample`
stands among the five and stands absent from the four, where the entry passed.
So the pair of runs reported one set of red names in the address lane at
`af54d3f0` and two sets in the thread lane, and the thread lane's difference is
one name. The release suite reported no red name in any of the three shapes.
`SC-IC-9` of `Success criteria - this phase (SC-IC-7 to SC-IC-11)` reads those
readings at the head Pass 2 runs at. Pass 1 took the narrow shape
`taskset -c 0,1` with `-j 2` in the release tree and took it in neither
instrumented tree, so the narrow shape of the address lane and of the thread
lane waits on Pass 2.

**The control pairs at the ceiling (ruling 34).** Dated rows beside the isolated
shape of `Three shapes per red name (IC-003, ruling 6)`, which keeps its text.
Ruling 34 of `Rulings - this phase (29 to 42)` gives each name two runs in the
tree that reported it: one run inside ctest under the ceiling that entry
carries, and one direct run of the same program from the working directory and
with the environment that entry carries, under no ceiling. The driver prints a
props line read from `ctest --test-dir <tree> --show-only=json-v1` above each
pair, and the wall of each direct run comes from the start and end stamps of its
own log. Each ceiling run below reads exit 8.

| Tree | Name | Ceiling | Cut exit | The line at the ceiling | Suite total | Direct exit | Direct wall | Closing line of the direct run | Ceiling witness | Direct witness |
|---|---|---|---|---|---|---|---|---|---|---|
| `build/coverage` | `base_cli_matrix` | 120.0 | 8 | `1/1 Test #83: base_cli_matrix ..................***Timeout 120.08 sec` | `120.10 sec` | 0 | 284 s | `26 of 26 with all rows matching` | `1.51 1.53 1.68 7/1762 2645325` | `1.37 1.47 1.64 2/1757 2646296` |
| `build/coverage` | `example_run` | 300.0 | 8 | `1/1 Test #175: example_run ......................***Timeout 300.04 sec` | `300.04 sec` | 0 | 740 s | `PASS: docs_user_counters (3 instances)` | `1.10 1.30 1.52 2/1751 2648546` | `1.17 1.36 1.51 1/1746 2651801` |
| `build/coverage` | `harness_hunt_cli_test` | 300.0 | 8 | `1/1 Test #148: harness_hunt_cli_test ............***Timeout 300.15 sec` | `300.15 sec` | 0 | 573 s | `harness_hunt_cli_test: ok` | `1.53 1.54 1.52 1/1643 2659510` | `1.29 1.51 1.52 1/1644 2662093` |
| `build/coverage` | `harness_jitter_cli_test` | 300.0 | 8 | `1/1 Test #92: harness_jitter_cli_test ..........***Timeout 300.13 sec` | `300.14 sec` | 0 | 486 s | `harness_jitter_cli_test: ok` | `1.84 1.49 1.48 1/1638 2667134` | `1.95 3.29 2.47 1/1633 2670563` |
| `build/sanitize` | `base_cli_matrix` | 120.0 | 8 | `1/1 Test #83: base_cli_matrix ..................***Timeout 120.06 sec` | `120.06 sec` | 0 | 139 s | `26 of 26 with all rows matching` | `1.78 3.81 3.69 1/1640 2727107` | `1.56 3.07 3.43 1/1685 2728213` |
| `build/sanitize` | `example_run` | 300.0 | 8 | `1/1 Test #175: example_run ......................***Timeout 300.02 sec` | `300.02 sec` | 0 | 347 s | `PASS: docs_user_counters (3 instances)` | `1.40 2.39 3.12 1/1686 2730465` | `1.51 1.82 2.65 1/1688 2737031` |
| `build/tsan` | `base_cli_matrix` | 120.0 | 8 | `1/1 Test #86: base_cli_matrix ..................***Timeout 120.13 sec` | `120.13 sec` | 0 | 186 s | `26 of 26 with all rows matching` | `1.27 3.33 4.79 2/1586 2832521` | `1.44 2.67 4.37 1/1588 2833616` |
| `build/tsan` | `example_run` | 300.0 | 8 | `1/1 Test #176: example_run ......................***Timeout 300.02 sec` | `300.02 sec` | 0 | 538 s | `PASS: docs_user_counters (3 instances)` | `1.06 1.95 3.78 1/1534 2835079` | `1.56 1.59 3.09 2/1582 2838348` |
| `build/tsan` | `harness_hunt_cli_test` | 300.0 | 8 | `1/1 Test #149: harness_hunt_cli_test ............***Timeout 300.13 sec` | `300.14 sec` | 0 | 408 s | `harness_hunt_cli_test: ok` | `1.09 1.28 2.27 1/1803 2844941` | `1.08 1.20 1.96 1/1793 2847602` |
| `build/tsan` | `harness_jitter_cli_test` | 300.0 | 8 | `1/1 Test #94: harness_jitter_cli_test ..........***Timeout 300.15 sec` | `300.15 sec` | 0 | 820 s | `harness_jitter_cli_test: ok` | `1.30 1.32 1.74 1/1794 2851274` | `1.88 2.46 2.18 1/1807 2854231` |
| `build/tsan` | `report_shape_fwq_ftq_sample` | 120.0 | 0 | `1/1 Test #171: report_shape_fwq_ftq_sample ......   Passed   92.48 sec` | `92.49 sec` | 0 | 92 s | printed no line | `1.68 1.71 2.03 2/1709 2861199` | `1.31 1.58 1.95 1/1712 2861960` |

`harness_jitter_cli_test` carries `RUN_SERIAL` in the props line of both trees,
and no other name above carries it. `report_shape_fwq_ftq_sample` stands outside
the pair shape at `af54d3f0`: its run under the ceiling answered exit 0 with
`1/1 Test #171: report_shape_fwq_ftq_sample ......   Passed   92.48 sec` in
92.49 s, and its direct run answered exit 0 and printed no line at all, so this
pass holds no ceiling crossing for that name at the candidate head, where the
standing red-name table of `The two instrumented lanes (IA-END-001, SC-IC-5)`
lists one at the head named above it.

**The walk price.** A dated row beside the walk-price paragraph of
`Three shapes per red name (IC-003, ruling 6)`. The step
`bash /tmp/ic/price_walk_release.sh` priced two launches of
`./build/example/docs_labels` in the release tree at `af54d3f0`, the `--list`
form and the `--help` form, three times each, one launch after the other, at the
witness `1.65 2.55 2.88 1/1568 3095378`, and each launch answered exit 0:

```text
release-docs_labels---list#1 exit=0 seconds=1.76
release-docs_labels---help#1 exit=0 seconds=1.84
release-docs_labels---list#2 exit=0 seconds=1.71
release-docs_labels---help#2 exit=0 seconds=1.70
release-docs_labels---list#3 exit=0 seconds=1.69
release-docs_labels---help#3 exit=0 seconds=1.73
```

The step closes with `test "$worst"` and an exit code of 0, so the exits of the
six launches rest on the lines above. The Pass 1 release range, 1.69 s to
1.84 s, sits inside the release range that standing paragraph reports. The
instrumented leg stands untaken: Pass 1 ran no pricing script in the address
tree, the thread tree or the coverage tree, so the paragraph's reading for the
address and undefined behaviour tree carries no Pass 1 figure, and the walk
price of lane 2 of `Lane Table` of `specs/048-hwlat-probe/plan.md` waits on
Pass 2.

**The five hardware consoles of lane 9.** Dated rows beside the five rows of
`Lane Table` of `specs/048-hwlat-probe/plan.md` (read at `af54d3f0`) that carry
lane 9. Each console ran alone in the release tree `build` at `af54d3f0`, and
every witness in this block reads under 2.0.

`Hunt, one scenario and one plan line`. `f6-hunt-scenario.log` ran
`./build/test/harness_hunt_cli_test bare-hunt-whole-catalog` at exit 0 with one
line of output, `bare-hunt-whole-catalog: ok`, at the witness
`1.62 2.12 2.67 1/1559 3097228`. `f6-hunt-plan.log` ran
`./build/example/first_benchmark --hunt --dry-run` at exit 0 at the witness
`1.50 2.11 2.67 2/1560 3097173`, and its plan line answers a missing estimate
and no budget:

```text
hunt: plan events=497 passes=249 method=PASSES estimate=not available budget=none
```

`Hunt, walk`. Three consoles of `./build/example/memory_walk` in the
`--hunt --hunt-budget 120s` shape, each alone:

| Console | Exit | Plan line | First rank row | Load witness | Ruling 30 |
|---|---|---|---|---|---|
| `f6-walk-1.log` | 0 | `hunt: plan events=497 passes=249 method=PASSES estimate=938979ns budget=120s stage1=497 stage2=60` | `hunt: rank 1 cpu/topdown-fetch-bubbles instr=2067.127 cv=3.45 correlation=n/a slope=n/a delta=n/a area=unmapped` | `1.33 2.18 2.72 2/1569 3096576` | stands |
| `f6-walk-2.log` | 0 | `hunt: plan events=497 passes=249 method=PASSES estimate=925782ns budget=120s stage1=497 stage2=60` | `hunt: rank 1 cpu/topdown-fetch-bubbles instr=2186.575 cv=6.15 correlation=n/a slope=n/a delta=n/a area=unmapped` | `1.31 2.16 2.71 1/1566 3096646` | stands |
| `f6-walk-3.log` | 0 | `hunt: plan events=497 passes=249 method=PASSES estimate=948441ns budget=120s stage1=497 stage2=60` | `hunt: rank 1 cpu/topdown-fetch-bubbles instr=2166.831 cv=9.30 correlation=n/a slope=n/a delta=n/a area=unmapped` | `1.44 2.18 2.71 1/1565 3096703` | stands |

`Hunt, benchmark`. Three consoles of `./build/example/first_benchmark` in the
same shape, each alone:

| Console | Exit | Plan line | First rank row | Load witness | Ruling 30 |
|---|---|---|---|---|---|
| `f6-bench-1.log` | 0 | `hunt: plan events=497 passes=249 method=PASSES estimate=1579407ns budget=120s stage1=497 stage2=60` | `hunt: rank 1 cpu/topdown-slots-issued instr=859.897 cv=0.40 correlation=n/a slope=n/a delta=n/a area=unmapped` | `1.41 2.16 2.70 1/1565 3096767` | stands |
| `f6-bench-2.log` | 0 | `hunt: plan events=497 passes=249 method=PASSES estimate=1567704ns budget=120s stage1=497 stage2=60` | `hunt: rank 1 cpu/topdown-slots-issued instr=863.006 cv=0.27 correlation=n/a slope=n/a delta=n/a area=unmapped` | `1.45 2.16 2.70 2/1565 3096827` | stands |
| `f6-bench-3.log` | 0 | `hunt: plan events=497 passes=249 method=PASSES estimate=1592355ns budget=120s stage1=497 stage2=60` | `hunt: rank 1 cpu/topdown-slots-issued instr=862.296 cv=0.26 correlation=n/a slope=n/a delta=n/a area=unmapped` | `1.45 2.16 2.70 1/1565 3096883` | stands |

Each of those six consoles also prints `topdown: level=1 passes=4 table=arch/x86/skylakex/`
and `topdown: reference=linux-7.2.4` ahead of its table file list. The twenty
numbered rank rows, the rows that close with a `mark=` cell and the four
`hunt: skipped` lines of each console stand in
`specs/046-hunter-topdown/results/acceptance-hunt.md`.

`Counter count`. Three consoles of `./build/example/docs_counter_probe`, each
alone:

| Console | Exit | Limit line | Load witness | Ruling 30 |
|---|---|---|---|---|
| `f6-probe-1.log` | 0 | `counter limit: cpu limit=4 source=decode probe=4` | `1.42 2.14 2.69 1/1560 3096935` | stands |
| `f6-probe-2.log` | 0 | `counter limit: cpu limit=4 source=decode probe=4` | `1.54 2.15 2.69 1/1558 3097008` | stands |
| `f6-probe-3.log` | 0 | `counter limit: cpu limit=4 source=decode probe=4` | `1.54 2.13 2.68 2/1559 3097075` | stands |

The three benchmark lines stand beside these witnesses in
`specs/041-counter-probe/results/acceptance-final-intel.md`.

**The static-analysis lane.** Dated rows beside the three `Analyzer` rows of
`The gates (IA-END-002)` under `Part D - the instrumented lanes and the gates`,
which keeps its text, and a dated row beside the `Static analysis` row of
`Lane Table`. The four steps ran in this order at `af54d3f0`:

```sh
rm -rf build/analyzer && cmake -S . -B build/analyzer --preset=ci-ubuntu -D Python3_EXECUTABLE=/usr/bin/python3 -D CMAKE_C_COMPILER_LAUNCHER= -D CMAKE_CXX_COMPILER_LAUNCHER=
cmake --build build/analyzer --target speedgun-ng_speedgun-ng speedgun-ng_harness -j "$(nproc)" 2>&1 | tee /tmp/ic/p1/analyzer-build.log
cmake --build build/analyzer/test -j "$(nproc)" 2>&1 | tee -a /tmp/ic/p1/analyzer-build.log
cmake --build build/analyzer/example -j "$(nproc)" 2>&1 | tee -a /tmp/ic/p1/analyzer-build.log
```



The three scope steps printed 323 `Building CXX object` lines into
`analyzer-build.log`, 146, 134 and 43 of them in the order above. The finding
count reads void, and it reads wrong for a second reason. The step's first
extraction printed `sed: -e expression #1, char 72: Unmatched ) or \)`, its
second extraction printed nothing, the file the step counts holds no line, and
the gate check then printed `gate checks readability-identifier-naming hits 0`
over that empty file. A count over an empty file is not a clear, so this pass
carries no finding count and no gate-check count for any scope.

**The gate tree and its commands.** Dated rows beside `The gates (IA-END-002)`
under `Part D - the instrumented lanes and the gates` and beside
`Figures this section leaves unsource`, both of which keep their text.
`f5-dev-cfg.log` wiped and configured `build/dev` with `--preset=ci-ubuntu` and
`-D Python3_EXECUTABLE=/usr/bin/python3` at exit 0 and the witness
`1.63 2.51 2.87 1/1568 3095504`. `f5-dev-cache.log` read that cache at exit 0
and printed four lines:

```text
CMAKE_BUILD_TYPE:STRING=Release
CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/c++
CMAKE_CXX_COMPILER_LAUNCHER:UNINITIALIZED=ccache
Python3_EXECUTABLE:UNINITIALIZED=/usr/bin/python3
```

`f5-dbc-target.log` ran `cmake --build build/dev -t dbc-gate -j "$(nproc)"` at
exit 0 and the witness `1.66 2.50 2.86 1/1568 3095846`, and printed:

```text
doc-gate: 349 interfaces, 0 gaps (/home/archerc/code/sg042-closeout/build/dev/dbc-gate/doc-matrix.json)
pair-gate: 349 interfaces, 0 gaps (/home/archerc/code/sg042-closeout/build/dev/dbc-gate/pair-matrix.json)
```

`f5-gates.log` then ran ten commands in one step and answered exit 2 at the
witness `1.35 2.25 2.75 1/1569 3096390`. Each answer stands in that file in the
order the step ran the commands.

| Duty | Command | Answer inside `f5-gates.log` |
|---|---|---|
| DBC gate, source scope | `/usr/bin/python3 tools/dbc/dbc_gate.py --scope source --mode check --profile full --allow-any-priority --report build/dev/dbc-report.csv` | the interpreter answers that it cannot open that path, and the checkout carries no such file |
| DBC matrix | `/usr/bin/python3 tools/dbc/dbc_matrix.py` | the same answer for that absent path |
| Header gate | `/usr/bin/python3 tools/dbc/header_gate.py --mode check` | the same answer for that absent path |
| Prose, tree | `/usr/bin/python3 tools/prose/prose_gate.py --check prose --mode tree --rules tools/prose/prose_rules.yaml` | `prose-lint: 1090 sources, 217665 units examined, 0 findings, 0 skipped` |
| Prose, commit form | `/usr/bin/python3 tools/prose/prose_gate.py --check commit --mode tree --rules tools/prose/prose_rules.yaml` | `--mode tree combined with --check commit is a usage error: commit checking needs a range` |
| Spell | `cmake -P cmake/spell.cmake` | `Used config files:` with `    1: .codespellrc`, and no finding line |
| Prose target | `cmake --build build/dev -t prose-gate -j "$(nproc)"` | `make: *** No rule to make target 'prose-gate'.  Stop.` |
| Spell target | `cmake --build build/dev -t spell-gate -j "$(nproc)"` | `make: *** No rule to make target 'spell-gate'.  Stop.` |
| Header target | `cmake --build build/dev -t header-gate -j "$(nproc)"` | `make: *** No rule to make target 'header-gate'.  Stop.` |
| Snippets target | `cmake --build build/dev -t snippets-gate -j "$(nproc)"` | `make: *** No rule to make target 'snippets-gate'.  Stop.` |

The four target names come from the driver line of that step and answer nothing
in the tree. `cmake --build build/dev --target help` at `af54d3f0` prints 622
lines that carry `prose-lint`, `prose-lint-fixtures`, `spell-check`, `spell-fix`,
`dbc-gate`, `coverage`, `validate`, `format-check` and `format-fix`, and none of
`prose-gate`, `spell-gate`, `header-gate` and `snippets-gate`. The other driver
of this phase, `/tmp/ic/final_chain2.sh`, holds 111 named steps and carries none
of those four names: it reads the same duties through `f5-prose-prose`,
`f5-prose-cmake`, `f5-prose-range`, `f5-spell`, `f5-dbc-doc`, `f5-dbc-pair`,
`f5-drift-bare`, `f5-drift-flags`, `f5-format` and `f5-time-source`. Pass 1 ran
no step of that driver, so none of those ten steps carries a log under
`/tmp/ic/p1/`.

**Figures Pass 1 did not take.** Pass 1 ran the ten lanes of the final-head set
and left the readings below open. Each names the pass that takes it.

- The two `Counter device` reads of lane 9, `cat /sys/devices/cpu/caps/pmu_name`
  and `cat /sys/bus/event_source/devices/cpu/type`. Pass 1 ran no step for
  either, so the `Counter device` row of `Lane Table` carries no Pass 1 figure,
  and `specs/041-counter-probe/results/acceptance-final-intel.md` states the
  same gap. `f6-pmu-name` and `f6-cpu-type` of `/tmp/ic/final_chain2.sh` carry
  both commands for Pass 2.
- The entry count of the coverage tree. The `Entry count` row asks for
  `ctest --test-dir build -N` and then the same command for `build/coverage`
  and for `build/dev`; Pass 1 counted `build`, `build/sanitize` and `build/tsan`
  instead, and every one of those three counts reads void, so `build/coverage`
  and `build/dev` carry no Pass 1 count at all.
- The narrow shape `taskset -c 0,1` with `-j 2` in the address tree and in the
  thread tree, which the `Address and undefined` and `Thread` rows ask for after
  each red name. Pass 2 takes both.
- The instrumented leg of `Walk price`. `/tmp/ic/price_walk_instrumented.sh`
  stands present and unrun, and `f2-walk-price-san` of `/tmp/ic/final_chain2.sh`
  carries it for Pass 2.
- The seven figures ruling 30 voids, each listed with its own load line above.
  Pass 2 re-takes each one alone.
- The rank movement of each hunt command across its three consoles, and the
  reading of the `area=` cells beside the `topdown: level=` line. The
  `Hunt, walk` and `Hunt, benchmark` rows take both at the head Pass 2 runs
  at, beside
  `specs/046-hunter-topdown/results/acceptance-hunt.md`.
- The `lcov --branch-coverage --summary` commands of the `Coverage denominators`
  and `Coverage, cold rows` rows. Pass 1 read its denominators from the capture
  step's own print, so neither row carries a Pass 1 figure.
- Lane 8 in the forms its rows give. Pass 1 ran the `dbc-gate` target, the
  direct prose script and the direct spell script; the two manual DBC gates, the
  `dbc_gate_fixtures` entry, and the two `cmake -D PROSE_MODE=...` forms stand
  untaken, and the ten `f5-*` steps of `/tmp/ic/final_chain2.sh` carry them for
  Pass 2.
- The eleven rows whose duty the phase states elsewhere: `Lint`,
  `Install and audits`, `Host records`, `pmu tables`, `Two counts`,
  `Downstream jobs`,
  `Parity at paranoid 2`, `Root surfaces` and `Push`. Pass 1 ran none of them,
  and Pass 1 carried no hosted job, no container run and no push of any kind.
