# Master acceptance on the AMD Zen 4 host

**Spec**: [specs/046-hunter-topdown/spec.md](../spec.md), `SC-009` and `SC-007`
**Plan**: [plan.md](../plan.md), section `Zen 4 acceptance plan on chilly (2026-11-10) - the hunter part and the shared plan`
**Host**: `chilly` (the name `uname -n` answers), AMD Zen 4
**Date**: 2026-11-10
**Branch**: `fix/zen4-acceptance` from the tip, one squashed commit, records only
**Base and measured head**: `1f1f9b20cec97198e5cd88778167431eef2c6eb6` (`1f1f9b20`), the `origin/master` tip at the fetch of this phase

`SC-009` asks one thing: on an AMD host, the hunt run names AMD top-down nodes and
no Intel node. `SC-007` asks three counts over the printed area cells: a node of the
selected table, a group of the listing, `unmapped`. This host answers both in the
documented command shape, with no added option. The rule of `Z4-046-002` runs over
all twelve hunt consoles, six of the documented shape and six of the multiplex
group.

Every capture of the first pass sits under `$HOME/z4/p1/`. The final-head sweep of
the claimed set stands at `$HOME/z4/p3/` on head
`24e379266e6fa49119972ef1000db23effa5d8c5`; its host-fact re-read prints that head
in `$HOME/z4/p3/facts-reread.log`, and its claimed rows answer in the pull-request
body. Every figure below names the command and the capture that printed it. A figure that moves from run to run prints as an
instability reading and is claimed by no run, per the non-repeat clause of
`Z4-046-001`.

## The host

| Item | Answer | Command and capture |
|---|---|---|
| Host name | `chilly` | `uname -n`, `$HOME/z4/p1/identity.log` |
| Kernel | `Linux 7.3.1-1-cachyos` x86_64 | `uname -a`, `$HOME/z4/p1/identity.log` |
| Vendor | AMD. The identity capture prints no `vendor_id` line; the device list names `amd_iommu_0`, and the selected table line reads `table=arch/x86/amdzen4/` | `ls /sys/bus/event_source/devices/`, `$HOME/z4/p1/devices.log`; the block line of each hunt console |
| Model name | AMD Ryzen 9 7950X3D 16-Core Processor | `grep -m1 'model name' /proc/cpuinfo`, `$HOME/z4/p1/identity.log` |
| CPU family | 25 | `grep -m1 'cpu family' /proc/cpuinfo`, `$HOME/z4/p1/identity.log` |
| Model | 97 | the `model` line of `/proc/cpuinfo`, the identity step, `$HOME/z4/p1/identity.log` |
| Stepping | 2 | the `stepping` line of `/proc/cpuinfo`, the identity step, `$HOME/z4/p1/identity.log` |
| Logical cpus | 32 | the cpu-count line of the identity step of `Z4-000.1`, `$HOME/z4/p1/identity.log` |
| Compiler | gcc (GCC) 16.2.1 20260810 | the gcc line of the identity step, `$HOME/z4/p1/identity.log` |
| cmake | cmake version 4.4.4 | the cmake line of the identity step, `$HOME/z4/p1/identity.log` |
| `kernel.perf_event_paranoid` | 2, the value the host carries | `cat /proc/sys/kernel/perf_event_paranoid`, `$HOME/z4/p1/identity.log`, read again in `$HOME/z4/p1/hunt-iso-paranoid-read.log` |
| `kernel.nmi_watchdog` | 0 | `cat /proc/sys/kernel/nmi_watchdog`, `$HOME/z4/p1/identity.log` and `$HOME/z4/p1/devices.log` |
| `caps/pmu_name` | absent: `cat: /sys/devices/cpu/caps/pmu_name: No such file or directory`. This is the standing shape on this vendor, where the Intel records read a name from that file | `cat /sys/devices/cpu/caps/pmu_name`, `$HOME/z4/p1/identity.log` and `$HOME/z4/p1/devices.log` |
| Device list | 12 devices: `amd_iommu_0`, `breakpoint`, `cpu`, `ibs_fetch`, `ibs_op`, `kprobe`, `msr`, `power`, `power_core`, `software`, `tracepoint`, `uprobe` | `ls /sys/bus/event_source/devices/`, `$HOME/z4/p1/devices.log` |
| `cpu` device `type` | 4 | `cat /sys/bus/event_source/devices/cpu/type`, `$HOME/z4/p1/devices.log` |
| Elevation | no root session. `sudo -n -l` answers `sudo: a password is required`; `pkexec --disable-internal-agent id` answers `Error executing command as another user: No authentication agent found.` The capture's next line reads `sudo exit: 0`, which is the exit of the recorded step and not of `sudo` alone. No run of this phase ran elevated. | `sudo -n -l`, `pkexec --disable-internal-agent id`, `$HOME/z4/p1/identity.log` |
| Version line | `speedgun-ng 0.34.0 build=Release` | the header line of each of the twelve hunt consoles, and of the three isolation captures |
| Master sha measured | `1f1f9b20cec97198e5cd88778167431eef2c6eb6`, printed on two lines of the capture, and `ANCESTOR_OK` from `git merge-base --is-ancestor 1f1f9b20cec97198e5cd88778167431eef2c6eb6 origin/master` | `$HOME/z4/p1/facts.log`; the branch gate of `Z4-000.1` |
| Schema | 1.16 | `grep -m1 'kSchemaVersion' source/harness/json_report.cpp`, `$HOME/z4/p1/facts.log` |
| Harness `SOVERSION`, archive `SOVERSION` | 22, 8 | the `SOVERSION` grep of the facts step. `$HOME/z4/p1/facts.log` keeps the line numbers: `456:    SOVERSION 22`, and the 8 on the `CMakeLists.txt:454` line, which is a comment that names the archive `SOVERSION` and is not a `SOVERSION` statement of its own |
| `LCOV_EXCL` markers | 84 | `grep -c LCOV_EXCL -r source include`, `$HOME/z4/p1/facts.log` |
| Remote branch | no remote `fix/zen4-acceptance`. `git ls-remote origin refs/heads/fix/zen4-acceptance` printed no ref line; the capture keeps only `remote branch: X11 forwarding request failed on channel 0` | `$HOME/z4/p1/facts.log` |

The build is the documented preset pair of `Z4-000.3`: `cmake --preset=ci-rocky` and
`cmake --build build -j"$(nproc)"`, a Release developer-mode build into `build`. The
configure capture `$HOME/z4/p1/cmake-configure.log` names the GNU 16.2.1 compiler id
and ends with the line that writes the `build` directory of the checkout. The build
capture `$HOME/z4/p1/build.log` runs 1076 lines and ends at
`[100%] Built target harness_topdown_run_test`. The build log prints no exit code, so
this record takes the build as answered by the binaries it ran: every console prints
`speedgun-ng 0.34.0 build=Release`.

One capture that `Z4-000.3` names stands absent: `$HOME/z4/p1/version.log`. The
version line it was to carry reads in the header of every hunt console, and
`$HOME/z4/p1/facts.log` prints `VERSION 0.34.0`.

## The commands

`results/acceptance-hunt.md` names the two hunt commands of the standing acceptance,
with `--hunt-budget 120s` in place of the `--timeout 120` that the harness has no
option for. Three runs of each, each run alone in its window:

```sh
./build/example/memory_walk --hunt --hunt-budget 120s
./build/example/first_benchmark --hunt --hunt-budget 120s
```

The multiplex group is the same two commands, three runs each, with one option added,
as the second console group of the same head:

```sh
./build/example/memory_walk --hunt --hunt-budget 120s --metric-method multiplex
./build/example/first_benchmark --hunt --hunt-budget 120s --metric-method multiplex
```

The isolation arms are the arms this host answers. They ran as a check at this
completing head:

```sh
./build/example/memory_walk --hunt --hunt-budget 30s --metric-method passes
./build/example/memory_walk --hunt=cpu/branch-instructions --hunt-budget 5s
cat /proc/sys/kernel/perf_event_paranoid
```

**The abort rule stayed dormant.** The abort rule of the spec section binds on a
documented-shape run that exits non-zero. No such run exists here: all twelve manifest
lines read `exit=0`, and each capture ends with its own `exit: 0` line
(`$HOME/z4/p1/manifest.txt`). Nothing aborted, so no abort was isolated, and the Zen 5
abort shape of `results/acceptance-amd-zen5.md` does not reproduce at this head. That
record keeps its abort row at `858c55d9`, and this record opens no second defect row.

Two witnesses of the run driver stand recorded as they printed, because a reader of
this section meets them in the captures:

- `$HOME/z4/p1/manifest.txt` carries the line `ABORT BRANCH 2026-11-09T20:21:43-06:00`.
  The driver writes it from its own manifest test
  (`grep -v ' exit=0 '` over the manifest, `$HOME/z4/p1/driver.sh`), and that test
  cannot match a manifest line, so the branch fires over a completed set. The twelve
  exit readings answer the runs; the marker line answers the driver.
- The witness line of each capture counts
  `ctest|cc1plus|cc1|gcc|g++|as|ninja|make` (`$HOME/z4/p1/driver.sh`), the standing
  Intel witness list. The widened list of the plan, which adds `perf`, `strace`,
  `bpftrace` and `trace-cmd`, prints in no capture. No trace tool ran in any of these
  windows, so the count answers the same question for this phase.

## The documented-shape six runs (SC-009, SC-007, Z4-046-001)

Per run, the exit, the printed-line count, the plan line, and the witness:

| Run | Exit | Console lines | Plan line | `area=` cells | Node / group / unmapped | `grep -ci intel` | Witness: load, other processes |
|---|---|---|---|---|---|---|---|
| `hunt-memory_walk-1` | 0 | 383 | `hunt: plan events=344 passes=86 method=PASSES estimate=56760ns budget=120s stage1=344 stage2=60` | 344 | 11 / 325 / 8 | 0 | 12.08, 0 |
| `hunt-memory_walk-2` | 0 | 383 | `hunt: plan events=344 passes=86 method=PASSES estimate=57620ns budget=120s stage1=344 stage2=60` | 344 | 11 / 325 / 8 | 0 | 11.84, 0 |
| `hunt-memory_walk-3` | 0 | 383 | `hunt: plan events=344 passes=86 method=PASSES estimate=38700ns budget=120s stage1=344 stage2=60` | 344 | 11 / 325 / 8 | 0 | 11.84, 0 |
| `hunt-first_benchmark-1` | 0 | 383 | `hunt: plan events=344 passes=86 method=PASSES estimate=69660ns budget=120s stage1=344 stage2=60` | 344 | 11 / 325 / 8 | 0 | 11.84, 0 |
| `hunt-first_benchmark-2` | 0 | 383 | `hunt: plan events=344 passes=86 method=PASSES estimate=69660ns budget=120s stage1=344 stage2=60` | 344 | 11 / 325 / 8 | 0 | 11.84, 0 |
| `hunt-first_benchmark-3` | 0 | 383 | `hunt: plan events=344 passes=86 method=PASSES estimate=73100ns budget=120s stage1=344 stage2=60` | 344 | 11 / 325 / 8 | 0 | 11.69, 0 |

The counts stand, the estimate moves. The captures are
`$HOME/z4/p1/hunt-memory_walk-<n>.log` and
`$HOME/z4/p1/hunt-first_benchmark-<n>.log` for `n` = 1, 2, 3, and the figures come
from `$HOME/z4/p1/hunt-figures.txt` and from the twelve captures.

Three lines print the same way in all six runs:

```text
topdown: level=1 passes=1 table=arch/x86/amdzen4/
topdown: reference=linux-7.2.4
hunt: plan summary events=332/344 passes=86 budget=120s
```

The summary keeps 332 of 344 events. Twelve events drop to a gap, and the twelve names
repeat in the same order in every run of the six (`hunt: skipped <name> gap`):

```text
cpu/fp_ret_x87_fp_ops.mul_ops        cpu/fp_ret_sse_avx_ops.add_sub_flops
cpu/fp_ret_x87_fp_ops.div_sqrt_ops   cpu/fp_ret_sse_avx_ops.mult_flops
cpu/fp_ret_x87_fp_ops.all            cpu/fp_ret_sse_avx_ops.div_flops
cpu/fp_retired_ser_ops.x87_ctrl_ret  cpu/fp_ret_sse_avx_ops.mac_flops
cpu/fp_retired_ser_ops.x87_bot_ret   cpu/fp_ret_sse_avx_ops.bfloat_mac_flops
cpu/fp_retired_ser_ops.sse_ctrl_ret  cpu/fp_ret_sse_avx_ops.all
```

Every witness count of other build, test, capture and trace processes reads 0, so no
window stands void and no run stands re-taken. The one-minute load average reads
11.69 to 12.08 over the six runs. `chilly` is a workstation host with a standing
desktop load, and this record states that character beside every witness: the load
average carries no bound, and the count is the bound the rule names.

## The three counts and the cell texts (SC-007, Z4-046-002)

The split over the printed cells holds in every run of both groups. The cell counts add
to the printed-cell count, and the printed-cell count equals the plan line's event
count, 11 + 325 + 8 = 344 = `events=344`:

| Cells | Kind | Values the cell answers |
|---|---|---|
| 11 | A rank that names a node of the selected AMD table | `topdown.bad_speculation > topdown.amd.bad_speculation_mispredicts` 5, `topdown.backend_bound > topdown.amd.backend_bound_memory` 3, `topdown.frontend_bound` 1, `topdown.smt_contention` 1, `topdown.retiring > topdown.amd.retiring_fastpath` 1 |
| 325 | A rank that names a group of the listing | `floating-point` 136, `cache` 109, `memory` 29, `other` 18, `core` 17, `branch` 15, `recommended` 1 |
| 8 | A rank that neither the tree nor the listing names | `unmapped` |

The 11 node cells name five distinct level-1 node names, with the distribution of the
line above: `topdown.bad_speculation` 5, `topdown.backend_bound` 3,
`topdown.frontend_bound` 1, `topdown.smt_contention` 1, `topdown.retiring` 1
(`NODES(mw1)` of `$HOME/z4/p1/hunt-figures.txt`, and the cell texts of each of the
twelve consoles).

The 325 group cells answer seven of the vendor file stems of the selected directory.
The selected table names 10 JSON files, the set of `ls
external/pmu-events/arch/x86/amdzen4/`: `branch.json`, `cache.json`, `core.json`,
`data-fabric.json`, `floating-point.json`, `memory-controller.json`, `memory.json`,
`other.json`, `pipeline.json`, `recommended.json`. Seven stems answer group cells;
`data-fabric`, `memory-controller` and `pipeline` answer 0 group cells in these runs.
Each run prints the ten names and their digests in its block.

The 8 `unmapped` cells carry the same eight event names in every run of the six:
`cpu/branch-instructions`, `cpu/branch-misses`, `cpu/cache-misses`,
`cpu/cache-references`, `cpu/enabled`, `cpu/ref-cycles`, `cpu/running`,
`cpu/stalled-cycles-frontend`. The run prints the cell `unmapped` and no reason beside
it, as the Zen 5 record states. FR-001 of this spec settles the value: the deepest
node whose formula names the event, else the list grouping, else `unmapped`.

**No Intel node.** `grep -ci intel` answers 0 in every one of the twelve consoles:

| Capture | `grep -ci intel` | Capture | `grep -ci intel` |
|---|---|---|---|
| `hunt-memory_walk-1.log` | 0 | `huntmx-memory_walk-1.log` | 0 |
| `hunt-memory_walk-2.log` | 0 | `huntmx-memory_walk-2.log` | 0 |
| `hunt-memory_walk-3.log` | 0 | `huntmx-memory_walk-3.log` | 0 |
| `hunt-first_benchmark-1.log` | 0 | `huntmx-first_benchmark-1.log` | 0 |
| `hunt-first_benchmark-2.log` | 0 | `huntmx-first_benchmark-2.log` | 0 |
| `hunt-first_benchmark-3.log` | 0 | `huntmx-first_benchmark-3.log` | 0 |

Every cell of every run starts `topdown.`, names one of the seven stems above, or reads
`unmapped`. `Z4-046-002` answers on all twelve captures. The twelve grep counts and the cell-class
totals above come from the captures themselves and from the figure file that
`Z4-046-002.1` names, `$HOME/z4/p1/hunt-figures.txt`.

## The multiplex console group (Z4-046-001.1, Z4-046-002)

The same six-run shape over the second group. All six answer exit 0 and 715 console
lines, and the group claims nothing of its own:

| Run | Exit | Console lines | `area=` cells | Node / group / unmapped | `grep -ci intel` | Witness: load, other processes |
|---|---|---|---|---|---|---|
| `huntmx-memory_walk-1` | 0 | 715 | 344 | 11 / 325 / 8 | 0 | 11.69, 0 |
| `huntmx-memory_walk-2` | 0 | 715 | 344 | 11 / 325 / 8 | 0 | 11.69, 0 |
| `huntmx-memory_walk-3` | 0 | 715 | 344 | 11 / 325 / 8 | 0 | 11.69, 0 |
| `huntmx-first_benchmark-1` | 0 | 715 | 344 | 11 / 325 / 8 | 0 | 11.39, 0 |
| `huntmx-first_benchmark-2` | 0 | 715 | 344 | 11 / 325 / 8 | 0 | 11.39, 0 |
| `huntmx-first_benchmark-3` | 0 | 715 | 344 | 11 / 325 / 8 | 0 | 11.39, 0 |

The plan line of each run carries the method word, and the table line and the reference
line read as in the documented group:

```text
hunt: plan events=344 passes=86 method=MULTIPLEX estimate=<moves from run to run> budget=120s stage1=344 stage2=60
topdown: level=1 passes=1 table=arch/x86/amdzen4/
topdown: reference=linux-7.2.4
```

The estimates read 46440ns, 54180ns and 41280ns for the three walk runs, and 74820ns,
104920ns and 76540ns for the three benchmark runs (`$HOME/z4/p1/huntmx-*.log`).

The two groups differ in the rows, and the difference prints here so a reader sees what
the multiplex console does and does not answer. Each multiplex run prints 344 rows, every
one of them a `hunt: rank -` row that carries `mark=unrepeated`, and no rank-numbered
row at all. Each prints 344 gap lines, and its summary line reads
`hunt: plan summary events=0/344 passes=86 budget=120s`. So the group prints no ranked
set, and it carries no figure of the ranked split.

**The agreement line.** Over the twelve consoles of both methods at `1f1f9b20`, the
printed-cell count is 344, the three area counts are 11, 325 and 8, the selected table
is `arch/x86/amdzen4/`, the reference is `linux-7.2.4`, and `intel` appears 0 times. The
multiplex group agrees with the documented-shape group on every one of those figures.

## The level-1 block (SC-007, Z4-046-001)

The block prints after the ranked rows and before the summary line of its own benchmark.
It prints once per benchmark, in the documented-shape consoles; the multiplex consoles
print no level-1 block. Runs 2 and 3 print their blocks in their captures, and the
record claims the cell readings of every run from those captures. The two blocks of
run 1 of each benchmark follow verbatim, with the ten table files and their digests
inside:

`memory_walk`, from `$HOME/z4/p1/hunt-memory_walk-1.log`:

```text
topdown: level=1 passes=1 table=arch/x86/amdzen4/
topdown: reference=linux-7.2.4
topdown: file branch.json ac8090f9e26304336989ab64f72f79890b5255194e8f4278971fa1ee42e24088
topdown: file cache.json a039ef5e47f117e255c67a3b8cceea892f3189302ab85ec2f7db99605f7c7458
topdown: file core.json efb5a78c1cd1d4ce3f326bacca966a5476ab762b8dcbabb23b89340ed6ab115f
topdown: file data-fabric.json 94f7d3e9466362510e5118772205b38e9a5c852f4042e2a4d6482e405b966786
topdown: file floating-point.json e3d5306fbd05a3b08e6e55901d23980f2a93ce416dc383087ff18f416890d645
topdown: file memory-controller.json 9350ae698708c1245c0cf9f9db3640863226801ea975a610328ba21b5bcb2bb9
topdown: file memory.json ce2e89008b5aede8c5e97cc745a04088c5ad3cce805fd36200428a81ec721f30
topdown: file other.json 09b93b2f7ba49bf86b3f2f77beca686e09b877561580906e569263d19194d10b
topdown: file pipeline.json e62ee36bfffb3fe48220bfeb91a868ed5568adb7e507033c982a2a44e806e301
topdown: file recommended.json 7bd4d67419b415ef0412f46731143a0201fdf377eaa007a21ada4afb5f57f6cf
topdown:  topdown.frontend_bound  frontend_bound  84.62%
topdown:  topdown.bad_speculation  bad_speculation  0.74%
topdown:  topdown.backend_bound  backend_bound  2.75%
topdown:  topdown.smt_contention  smt_contention  0.00%
topdown:  topdown.retiring  retiring  12.76%
topdown: the four shared level-1 values add to less than 100 percent without topdown.smt_contention
```

`first_benchmark`, from `$HOME/z4/p1/hunt-first_benchmark-1.log`:

```text
topdown: level=1 passes=1 table=arch/x86/amdzen4/
topdown: reference=linux-7.2.4
topdown: file branch.json ac8090f9e26304336989ab64f72f79890b5255194e8f4278971fa1ee42e24088
topdown: file cache.json a039ef5e47f117e255c67a3b8cceea892f3189302ab85ec2f7db99605f7c7458
topdown: file core.json efb5a78c1cd1d4ce3f326bacca966a5476ab762b8dcbabb23b89340ed6ab115f
topdown: file data-fabric.json 94f7d3e9466362510e5118772205b38e9a5c852f4042e2a4d6482e405b966786
topdown: file floating-point.json e3d5306fbd05a3b08e6e55901d23980f2a93ce416dc383087ff18f416890d645
topdown: file memory-controller.json 9350ae698708c1245c0cf9f9db3640863226801ea975a610328ba21b5bcb2bb9
topdown: file memory.json ce2e89008b5aede8c5e97cc745a04088c5ad3cce805fd36200428a81ec721f30
topdown: file other.json 09b93b2f7ba49bf86b3f2f77beca686e09b877561580906e569263d19194d10b
topdown: file pipeline.json e62ee36bfffb3fe48220bfeb91a868ed5568adb7e507033c982a2a44e806e301
topdown: file recommended.json 7bd4d67419b415ef0412f46731143a0201fdf377eaa007a21ada4afb5f57f6cf
topdown:  topdown.frontend_bound  frontend_bound  50.88%
topdown:  topdown.bad_speculation  bad_speculation  0.73%
topdown:  topdown.backend_bound  backend_bound  1.67%
topdown:  topdown.smt_contention  smt_contention  0.00%
topdown:  topdown.retiring  retiring  46.75%
topdown: the four shared level-1 values add to less than 100 percent without topdown.smt_contention
```

The block carries figures on this host, where the Intel record's block printed
`not available` on every node, and it carries the same shape the Zen 5 record quotes,
including `topdown.smt_contention` and the line that names the sum short of 100 percent.

The percentage cells move from run to run, and no run claims them. The
`topdown.frontend_bound` reading of the three walk runs reads 84.62%, 71.27% and
71.98%, and of the three benchmark runs 50.88%, 50.05% and 50.14%
(`$HOME/z4/p1/hunt-memory_walk-<n>.log`, `$HOME/z4/p1/hunt-first_benchmark-<n>.log`).
The other four node cells move in the same way; `topdown.smt_contention` reads 0.00% in
every run of both groups.

The same rule covers the rows that carry a rank number. The 20 ranked rows split
differently in every run, which is the standing note AP-9 of this spec:

| Run | Node | Group | Unmapped |
|---|---|---|---|
| `hunt-memory_walk-1` | 1 | 15 | 4 |
| `hunt-memory_walk-2` | 0 | 17 | 3 |
| `hunt-memory_walk-3` | 1 | 18 | 1 |
| `hunt-first_benchmark-1` | 1 | 16 | 3 |
| `hunt-first_benchmark-2` | 0 | 16 | 4 |
| `hunt-first_benchmark-3` | 1 | 15 | 4 |

The splits come from the `ranked=20(n/g/u)` readings of
`$HOME/z4/p1/hunt-figures.txt`. They are instability readings, claimed by no run. The
three area counts over the printed cells, the 344-cell count, and the eight `unmapped`
event names repeat, so the record claims those. The multiplex consoles print no
rank-numbered rows at all, so they answer no ranked split in either group.

## The marks

The marks read the shape the Intel record's marks section states, at this host's row
count. Of the 324 rows that carry no rank number, 284 carry `mark=unrepeated` in every
run of the six. The remaining 40 divide between `beyond-top` and `noisy`, and that
division moves: 31/9, 26/14 and 26/14 for the three walk runs, 36/4, 31/9 and 36/4 for
the three benchmark runs. The 20 ranked rows carry no mark. The counts come from the
`mark=` cells of `$HOME/z4/p1/hunt-memory_walk-<n>.log` and
`$HOME/z4/p1/hunt-first_benchmark-<n>.log`, and the division is an instability reading
claimed by no run.

In the multiplex group all 344 rows print as unranked rows and all 344 carry
`mark=unrepeated`; `mark=beyond-top` and `mark=noisy` print 0 times in each of the six
captures (`$HOME/z4/p1/huntmx-*.log`).

## The suites (Z4-000.4, Z4-000.5)

| Command | Entries | Passed | Failed | Skipped | Wall time | Capture |
|---|---|---|---|---|---|---|
| `ctest --test-dir build --output-on-failure` | 190 | 189 | 0 | 1 | 332.33 s | `$HOME/z4/p1/suite-full.log` |
| `taskset -c 0,1 ctest --test-dir build --output-on-failure` | 190 | 189 | 0 | 1 | 461.33 s | `$HOME/z4/p1/suite-pinned.log` |

Each capture ends with `100% tests passed out of 190`, the label time summary, and
`Total Test time (real)`. The two wall times are those lines: 332.33 s and 461.33 s. No
test failed on either row, and the entry count, the passed count and the skipped count
repeat between the two passes, as `Z4-046-001` asks of a suite row.

One entry is reported skipped: `harness_topdown_run_test`. The captures print
`188/190 Test #188: harness_topdown_run_test ... ***Skipped` and the closing line
`The following tests did not run: 188 - harness_topdown_run_test (Skipped)`. The entry
leaves with the skip code the registration reserves, code 2
(`test/CMakeLists.txt:2419`, `SKIP_RETURN_CODE 2`). The reason is the identity of this
host, the same reason the Zen 5 record names: the scenario
`scenarioTheDeeperLevelsCollectEveryNodeAboveThem` asks `hostSelectsTheFixtureDirectory()`
(`test/source/harness_topdown_run_test.cpp:391-397`), which is false where the identity
selects a directory beside the fixture's Intel one, and the scenario skips with
`the identity of this host names a directory beside the fixture's`
(`test/source/harness_topdown_run_test.cpp:2427`). `ctest --output-on-failure` prints
the output of a failed entry only, so the captures print no skip-reason text and no
scenario count, and this record claims no scenario count for that entry. The entry is
not a host gap: the hunter consoles above read the `arch/x86/amdzen4/` table on this
same host, and the feature's own units `harness_hunt_topdown_test`,
`harness_topdown_report_test` and `harness_topdown_option_test` passed in both suites.

## The isolation check (Z4-046-001.3)

The arms ran once as a check at this completing head, and every exit reads 0:

| Arm | Command | Exit | Capture | What it prints |
|---|---|---|---|---|
| Short passes run | `./build/example/memory_walk --hunt --hunt-budget 30s --metric-method passes` | 0 | `$HOME/z4/p1/hunt-iso-passes30.log` | `hunt: plan events=344 passes=86 method=PASSES estimate=37840ns budget=30s stage1=344 stage2=60`, 344 cells, 11 / 325 / 8, `intel` 0, summary `events=332/344 passes=86 budget=30s`, twelve gap lines, ranked split 1/17/2 |
| One-event hunt | `./build/example/memory_walk --hunt=cpu/branch-instructions --hunt-budget 5s` | 0 | `$HOME/z4/p1/hunt-iso-named.log` | `hunt: plan events=1 passes=1 method=PASSES estimate=470ns budget=5s stage1=1 stage2=1`, one `area=unmapped` row, the level-1 block, summary `events=1/1 passes=1 budget=5s` |
| `paranoid` read | `cat /proc/sys/kernel/perf_event_paranoid` | 0 | `$HOME/z4/p1/hunt-iso-paranoid-read.log` | `2`, the value the host carries, read again after the hunts |
| Repeats at the carried value | the six documented-shape runs and the six multiplex runs | 0 | `$HOME/z4/p1/hunt-*-<n>.log`, `$HOME/z4/p1/huntmx-*-<n>.log` | the six-run and six-run tables above |

The repeats arm is the three runs of each command at the carried `paranoid` value 2, in
both methods. The two arms that this host cannot answer stand as BLOCKED rows in the
shape the counter record uses for the same controls, and they carry no figure:

| Arm (BLOCKED) | Control | Value this host reads first | Set line | Restore line | Missing right |
|---|---|---|---|---|---|
| The tracer arm | a trace over one failing run | no tracer to read | no tracer on this host, and the phase installs no tool | not applicable | `strace` is absent: the driver runs `command -v strace` inside the branch it took, and that branch wrote no `$HOME/z4/p1/abort-strace.log` |
| The `paranoid` -1 arm | `kernel.perf_event_paranoid` at -1 | 2 (`$HOME/z4/p1/identity.log`, `$HOME/z4/p1/hunt-iso-paranoid-read.log`) | `sysctl -w kernel.perf_event_paranoid=-1` | `sysctl -w kernel.perf_event_paranoid=2` | no root session: `sudo: a password is required`, and `Error executing command as another user: No authentication agent found.` (`$HOME/z4/p1/identity.log`) |

No control stands weakened, and no value was set: the read arm prints 2, the value the
host carried before the runs and after them.

## Pending rows after this record

- `SC-010` keeps its pending rows: the Intel parts measured through `PERF_METRICS`, the
  Intel hybrid parts, and the Arm parts. This phase measured none of them, and no row
  here borrows a figure for them.
- `SC-009`'s AMD Zen 5 row stands measured at `858c55d9`, with its multiplex note and
  its abort row, in `results/acceptance-amd-zen5.md`. This record changes nothing there.
- The Intel rows stand at their heads: `615c0964547e4e9768b8e2f2d79991033a0eb5bd`,
  `87d745d6c7e62d4da80198c689ddc7078f08ff3d` and the closeout head
  `2d4d8c57975a86db6c77536f7090aba2bdaf6417` of `results/acceptance-final-intel.md`. No
  completed row reopens.
- The `SC-010` AMD Zen 4 hardware row is answered by this record: the Zen 4 hunter
  reading stands measured at `1f1f9b20` on `chilly`.
- The `SC-009` command-shape row needs no option on this host: the documented command
  completed, so the Zen 5 note that the shape needs `--metric-method multiplex` on that
  host keeps its own scope and gains no Zen 4 clause.
- No base build ran. The base build stands out of scope of this phase, named and not
  run.

## What this record changes in the spec

`SC-009` of [spec.md](../spec.md) gains one dated row, added with this record: AMD Zen 4
measured on `chilly` at `1f1f9b20cec97198e5cd88778167431eef2c6eb6`, with the three
counts 11, 325 and 8, the selected `arch/x86/amdzen4/` table, the `intel` count of 0
over all twelve consoles, and one line that the multiplex group agrees. The standing
`SC-009` text, the Zen 5 reading at `858c55d9`, and the sentence that names Zen 4 a
pending row keep their words; the dated row is added beside the bullet, and the Zen 4
pending sentence is answered by `SC-010`'s Zen 4 hardware row, which now points to this
record. `SC-010` keeps the Intel hybrid, Arm and `PERF_METRICS` rows pending. The ticks
of `tasks.md` name this record: `Z4-000.1` to `Z4-000.5`, `Z4-046-001.1` to
`Z4-046-001.3`, `Z4-046-002.1`, `Z4-046-003.1` and `Z4-046-003.2`.
