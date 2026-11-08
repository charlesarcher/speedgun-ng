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
passes on both. No marker, allowance or suppression changed here, and G-25 stands landed.

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
timed out at 300 s when the lane ran the hosted shape of four cores and four jobs. The
spread between 281 s and 345 s stands unexplained by this record. A name that moves that
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
  this head and stand green in every local lane above; no local row explains that gap, and
  this record states it as an open difference between the two runners.
