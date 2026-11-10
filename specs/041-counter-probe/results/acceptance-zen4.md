# Zen 4 acceptance: the counter-count part

**Spec**: [spec.md](../spec.md) | **Plan**: [plan.md](../plan.md) | **Date**: 2026-11-10
**Branch**: `fix/zen4-acceptance`, records only | **Shared plan**: `specs/046-hunter-topdown/plan.md`
**Host**: `chilly` (the name `uname -n` answers), an AMD Zen 4 part
**Ids**: `IA41-001`, `IA41-002`, `IP-001`, `IP-002`, `Z4-041-001`, `Z4-041-002`, `Z4-000.7`, `SC-010`

This record writes figures. Every figure names its command, its capture line,
or its source line. Every cite into `example/`, `source/`,
`specs/041-counter-probe/tools/`, and `specs/041-counter-probe/results/` reads
at the measured head that the Identity table prints, and the line number is the
line read there. `IA41-001` and `IP-001` ask for the limit line, the parity,
the opens and reads, and the watchdog rule on hardware. `Z4-041-001` and
`Z4-041-002` give the shape of this record, and `Z4-000.7` brings the two suite
rows into it. The Zen 4 row of `SC-010` and of `IA41-002` stands answered here.
The pass that wrote this file ran no build, no test, no benchmark, no tracer,
and no command that changes system state.

## Identity

| Field | Reading | Source |
|---|---|---|
| Host | `chilly` | `identity.log:2` |
| Cpu | AMD Ryzen 9 7950X3D 16-Core Processor, cpu family 25, model 97, stepping 2, 32 logical cpus | `identity.log:4`, `:5`, `:6`, `:7`, `:8` |
| Kernel | `Linux chilly 7.3.1-1-cachyos #1 SMP PREEMPT_DYNAMIC Sat, 31 Oct 2026 06:43:52 +0000 x86_64 GNU/Linux` | `identity.log:3` |
| Library | speedgun-ng 0.34.0 | the run header of every probe capture, `speedgun-ng 0.34.0 build=Release`; `facts.log:2` prints `VERSION 0.34.0` |
| Schema, `SOVERSION`s | 1.16, harness 22, archive 8 | `facts.log:3`, `:4`, `:5` |
| `LCOV_EXCL` markers | 84 | `facts.log:6` |
| Base head | `1f1f9b20cec97198e5cd88778167431eef2c6eb6` | `facts.log:7`; `facts.log:9` prints `ANCESTOR_OK`, so that head stands an ancestor of `origin/master` |
| Measured head | `1f1f9b20cec97198e5cd88778167431eef2c6eb6` | `facts.log:8`, and `git rev-parse HEAD` in the tree that ran the captures |
| Branch | `fix/zen4-acceptance`, records only | this phase |
| Compiler, cmake | gcc (GCC) 16.2.1 20260810, cmake version 4.4.4 | `identity.log:31`, `:32` |
| Root session | none | `identity.log:27` prints `sudo: a password is required`; `identity.log:30` prints `Error executing command as another user: No authentication agent found.`

The base head and the measured head are one sha, so every figure below was read
at one head. The captures carry their own capture date: the identity step ran
at `2026-11-09T20:16:53-06:00` (`identity.log:1`), the device step at
`2026-11-09T20:21:54-06:00` (`devices.log:1`), and the three consoles at
`2026-11-09T20:21:51-06:00` through `2026-11-09T20:21:53-06:00`. The record
date is the phase date, 2026-11-10.

## The device and the kernel settings (IA41-001, IP-001)

| Fact | Reading | Path | Mode |
|---|---|---|---|
| Device list | `amd_iommu_0`, `breakpoint`, `cpu`, `ibs_fetch`, `ibs_op`, `kprobe`, `msr`, `power`, `power_core`, `software`, `tracepoint`, `uprobe` | `/sys/bus/event_source/devices/` | readable |
| `cpu` device type | `4` | `/sys/bus/event_source/devices/cpu/type` | `-r--r--r-- 1 root root 4096 Nov  9 20:21` |
| `caps/pmu_name` | the standing absence on this vendor: `cat: /sys/devices/cpu/caps/pmu_name: No such file or directory` | `/sys/devices/cpu/caps/pmu_name` | absent |
| `perf_event_paranoid` | `2` | `/proc/sys/kernel/perf_event_paranoid` | mode not captured |
| `nmi_watchdog` | `0` | `/proc/sys/kernel/nmi_watchdog` | mode not captured |
| `msr` node | a root-only node | `/dev/cpu/0/msr` | `crw------- 1 root root 202, 0 Nov  6 16:37`

The readings come from one capture of the device step: the device list at
`devices.log:3-14`, the `cpu` type and its mode line at `devices.log:15` and
`:16`, the `caps/pmu_name` refusal at `devices.log:17`, the two controls at
`devices.log:18` and `:19`, and the `msr` node at `devices.log:20`. The
identity step reads the same facts: the two controls at `identity.log:9` and
`:10`, the `caps/pmu_name` refusal at `identity.log:11`, and the `cpu` type at
`identity.log:12`.

The device list names one core device, `cpu`. It names no `cpu_core` and
`cpu_atom` pair, and it names no `armv8_` device. The `msr` node gates nothing:
a group open is a `perf_event_open` syscall at
`example/docs/counter_probe.cpp:287-292` whose attribute carries the core
device's own type number, read from that device's `type` file at
`example/docs/counter_probe.cpp:196` and stored in `attr.type` at
`example/docs/counter_probe.cpp:253`. The syscall opens no file under
`/dev/cpu`. The library search passes that device's own type number at
`source/counters/linux_pmu/limit_fill.cpp:70-74`, and it asks core device names
at `source/counters/linux_pmu/counter_limits.cpp:357-361`.

Two tools stand not run, and no row here prints a raw configuration word.
`specs/041-counter-probe/tools/group_probe.c` stands not run: its first
argument is a raw config word
(`specs/041-counter-probe/tools/group_probe.c:112-113` reads the two
arguments), and it fixes `attr.type` to `PERF_TYPE_HARDWARE` at `:36`, so it
takes no device-type argument and answers no row that needs one. The History
labels row of `results/acceptance-final-intel.md:185` records that reading.
`specs/041-counter-probe/tools/probe_window.c` stands not run: no row of this
phase names its shape, as the `Boundaries this phase keeps` clause of the Zen 4
section of `spec.md` states. `G-31` bars new raw event code support, and no
leaf word, configuration word, or raw event value stands in this record.

## The example, three times (Z4-041-001)

```sh
./build/example/docs_counter_probe
```

| Run | Capture | Exit | Limit line | Benchmark line, verbatim | Load witness |
|---|---|---|---|---|---|
| 1 | `$HOME/z4/p1/probe-1.log` | 0 | `counter limit: cpu limit=6 source=decode probe=6` | `bmProbeTick              iterations=1823728121  real time/iter (ns)=    0.389  cpu time/iter (ns)=    0.389  overhead floor (ns)=  100.000  endpoint samples=2` | `11.39 15.22 12.82 6/1372 788614` |
| 2 | `$HOME/z4/p1/probe-2.log` | 0 | `counter limit: cpu limit=6 source=decode probe=6` | `bmProbeTick              iterations=1792858378  real time/iter (ns)=    0.391  cpu time/iter (ns)=    0.390  overhead floor (ns)=  100.000  endpoint samples=2` | `11.12 15.10 12.79 7/1372 788631` |
| 3 | `$HOME/z4/p1/probe-3.log` | 0 | `counter limit: cpu limit=6 source=decode probe=6` | `bmProbeTick              iterations=1809824519  real time/iter (ns)=    0.388  cpu time/iter (ns)=    0.388  overhead floor (ns)=  110.000  endpoint samples=2` | `11.12 15.10 12.79 11/1372 788645` |

Each capture opens with its command line, its date, and its two witness lines,
and each prints `speedgun-ng 0.34.0 build=Release` above the two report rows.
Each capture ends with `exit: 0`. Each witness line reads `witness other ctest
or compiler processes: 0`, so no window stands void and no run needs a re-take
(`Z4-041-001`). The three one-minute load averages read 11.39, 11.12 and 11.12.
The load average carries no numeric bound: `chilly` is a workstation with a
standing desktop load, and that character is the reading beside every witness
in this record (`Z4-041-001`).
The witness line counts build and test process names. The trace tools the plan
names (`perf`, `strace`, `bpftrace`, `trace-cmd`) stand absent on this host, so
the count covers the classes this host can host.

The three consoles print one limit line, so the limit figure repeats across the
three-run group. The iteration counts read 1823728121, 1792858378 and
1809824519; the three `real time/iter` cells read 0.389 ns, 0.391 ns and 0.388
ns; the `overhead floor` cells read 100.000 ns, 100.000 ns and 110.000 ns.

The committed `example/docs/counter_probe.expected` of FR-061 carries
`speedgun-ng 0.33.0 build=Release`, its own iteration count, and its own time
cells, and it names no host. It is another host's reading. This record claims
neither a match nor a failure of a byte compare against it (ruling G-20,
`SC-007`).

## The parity row, measured (IP-001, IP-002)

| Row | Status | Basis |
|---|---|---|
| The published limit against the probe answer at `perf_event_paranoid` `2`, the value this host answers | measured | the three consoles print `counter limit: cpu limit=6 source=decode probe=6` at exit `0`: the published limit `6`, the source word `decode`, and the example's own probe answer `6`; the value `2` reads at `identity.log:9` and `devices.log:18` |
| The same parity at `perf_event_paranoid` `-1` | BLOCKED | the BLOCKED row of the section below, with no figure |

The source word `decode` is the word the decoded arm seats:
`source/counters/linux_pmu/limit_fill.cpp:88-95` seats `LimitSource::DECODE` at
`:94-95`, and the example prints that enumerator as the word `decode` at
`example/docs/counter_probe.cpp:355-356`. The published count reaches the
console from the AMD rule:
`source/counters/linux_pmu/counter_limits.cpp:323-324` routes the AMD vendor to
`amdCoreLimit` at `:247-263`. Its gate at `:254-258` answers the `EBX[3:0]`
core-counter count of leaf `0x80000022` when the `PerfMonV2` bit stands (`:60`
names the leaf, `:84` the bit, `:92` the `EBX[3:0]` mask, `:257` the answer),
and the `PerfCtrExtCore` arm at `:259-261` answers the standing `6` of `:95` on
a part without that bit. The consoles print `limit=6` from `source=decode`, so
the decode answered on this host and the library paid no open for its figure
(`FR-030`). This record names the gate and not its input, and it prints no leaf
word.

The agreement row answers the claim of the value this host carries, and that
claim alone (`IP-002`). The `-1` row below borrows no figure from it.

## Opens and reads, derived (IP-001, Z4-041-001)

No tracer ran, so both figures stand derived. The walk is the example's own.

| Figure | Kind | Basis, with the arithmetic |
|---|---|---|
| Opens `28` | derived | `example/docs/counter_probe.cpp:329-340` walks sizes upward from `1` and stops at the first size that does not run whole, and `example/docs/counter_probe.cpp:283-298` pays one `perf_event_open` per copy of a size. The answer `6` means sizes `1..6` ran whole and size `7` stopped, so the walk ran sizes `1, 2, 3, 4, 5, 6, 7`: 1 + 2 + 3 + 4 + 5 + 6 + 7 = 28 |
| Reads `7` | derived | `example/docs/counter_probe.cpp:300` admits one read of the group leader for each size whose opens all landed, and `example/docs/counter_probe.cpp:303` pays that read. Sizes `1..7` pay one each: 1 + 1 + 1 + 1 + 1 + 1 + 1 = 7, of which six answer "ran whole" at `:308-309` and size `7` answers "does not run whole" |
| Closes `28` | derived | `example/docs/counter_probe.cpp:312-314` closes every descriptor one size opened, the refusals included |
| Cap binds nothing | derived | `example/docs/counter_probe.cpp:88` sets `kProbeCap = 32`, and the walk stopped at size `7` |

The size-`7` step has two readings that follow from the same lines, and this
record states which one it prints. A size pays one open call per copy, and only
a refusal stops the copies of that size
(`example/docs/counter_probe.cpp:293-296`), so the `28` opens stand for both
readings. The 7th read stands where all seven opens of size `7` landed and the
leader read answered a multiplexed pair, the pair the fit test reads at
`:308-309`; a refusal inside size `7` answers the same `probe=6` with `28`
opens and `6` reads. This record prints `28` and `7`, because the standing AMD
proof record measures the no-refusal condition on this vendor at
`results/requirement-1-zen5-paranoid2.txt:53` ("No refusal up to k = 96. Every
group open answered 0."), and `results/requirement-1-zen5-paranoid2.txt:58`
prints the same pair for the same answer `6`: sizes opened `7`, opens paid
`28`, reads `7`. No size reported the gate refusals `EACCES` or `EPERM`,
because `example/docs/counter_probe.cpp:293-294` seats that verdict and
`example/docs/counter_probe.cpp:331-332` answers `probe=no limit` with it, and
the three consoles print `probe=6`. The library pays no open on this host at
all, so the `28` opens and the `7` reads belong to the example's own search
(`FR-030`).

## The watchdog rule, applied once (IP-001)

| Item | Reading | Cite, read at the measured head |
|---|---|---|
| The decoded arm applies the deduction once | one call | `source/counters/linux_pmu/limit_fill.cpp:88-95` holds the decoded arm, and `:93` calls `usableCounters` once on the decoded figure |
| The probed arm applies none | no call | `source/counters/linux_pmu/limit_fill.cpp:96-111` holds the probed arm: `probeEventNameForDevice` at `:103` and `searchedLimit` at `:104`, and no watchdog call. The reason stands at `:99-102`: a whole group answers the counters the kernel granted, and that grant already misses the counter the watchdog holds |
| The gate that returns the count unchanged | `watchdogValue != 1` | `source/counters/linux_pmu/counter_limits.cpp:342-343` returns the hardware count unchanged when the watchdog value is not `1`, and `:282-290` reads that value from `/proc/sys/kernel/nmi_watchdog` |
| This host's reading | `0` | `devices.log:19`, `identity.log:10` |

`nmi_watchdog` reads `0` on `chilly`, so the gate at `:342-343` returns the
count unchanged and the deduction is a no-op on this part: the published figure
`6` reaches the console unchanged. For the rule itself, this vendor stands
outside the fixed-cycles arm at `:349-353`, which answers Intel-like vendors
only, so a watchdog of `1` on this part would take one counter through
`publishMinusOne` at `:134-140`, called at `:354`. This record claims no second
application of the rule anywhere, and `IP-001` refuses one as a correction.

## The `-1` parity row, BLOCKED (IP-002)

| Row | Status | Basis |
|---|---|---|
| The parity of the two limits with `perf_event_paranoid` at `-1` | BLOCKED | Control: `/proc/sys/kernel/perf_event_paranoid`. Read first: `2`, printed at `identity.log:9` of the identity step and at `devices.log:18` of the device step of the same pass. Set: `sysctl -w kernel.perf_event_paranoid=-1`. Restore: `sysctl -w kernel.perf_event_paranoid=2`. Missing right: no root session. `identity.log:27` prints `sudo: a password is required`, and `identity.log:30` prints `Error executing command as another user: No authentication agent found.` No figure.

The row carries the control, the value this host answered first, the line that
sets, the line that restores, and the right this session lacks. It carries no
figure, and the measured row above lends it none (`IP-002`). The lane weakened
no control: the pass driver runs no `sysctl` line, and both readings of the
control print the carried value `2`.

Two readings stand beside this row, and this record states them. First,
`identity.log:28` prints `sudo exit: 0` under the `sudo` refusal, where the
standing Intel record prints exit `1` for that refusal. The refusal text, not
that exit line, is the answer, and no session of the pass held the write.
Second, the row answers when an operator opens a root session, reads the value,
sets `-1`, runs `./build/example/docs_counter_probe`, restores `2`, and reads
the value again.

## The suite rows (Z4-000.7)

| Command | Entries | Passed | Failed | Skipped | Wall time | Capture |
|---|---|---|---|---|---|---|
| `ctest --test-dir build --output-on-failure` | 190 | 189 | 0 | 1 | 332.33 s | `$HOME/z4/p1/suite-full.log` |
| `taskset -c 0,1 ctest --test-dir build --output-on-failure` | 190 | 189 | 0 | 1 | 461.33 s | `$HOME/z4/p1/suite-pinned.log` |

Each capture prints `100% tests passed out of 190`, 189 lines marked `Passed`,
and one line naming the entry that did not run: `188 - harness_topdown_run_test
(Skipped)`. The capture prints no reason beside that name. Each witness line
reads `witness other ctest or compiler processes: 0`, and the one-minute load
averages read 11.12 for the full row and 20.61 for the pinned row: the standing
desktop load of this workstation, which carries no bound. Both rows stand local
only (ruling G-20).

The final-head rows of `Z4-001` stand in the pull-request body and not in this
record. `Z4-000.7` of `tasks.md` places the `Z4-001` re-run rows in the body of
`Z4-001.3` and keeps them out of every record file. The final sweep stands at
`$HOME/z4/p3/` on head `24e379266e6fa49119972ef1000db23effa5d8c5`, its host-fact
re-read in `$HOME/z4/p3/facts-reread.log`, and its rows answer in the
pull-request body. The suite counts repeat between the two passes of `Z4-001`
(190 entries, 189 passed, 0 failed, 1 skipped); durations do not repeat, each
row carries its witness loadavg triple, and the standing fleet model service
`/opt/strata/engine/strata --serve` runs beside the suite windows as this host's
baseline; the entry-alone discriminator for the noise-sensitive shape test
stands in `$HOME/z4/p3/shape-alone.log`. The two rows above stand on their own
captures.

## Pending rows and boundaries (IA41-002, SC-010)

- The Arm row of `SC-010` stays pending, with no hardware claim. `chilly` names
  no `armv8_` device in its device list (`devices.log:3-14`).
- The Intel hybrid row of `SC-010` stays pending with its `FR-013a`
  device-shape limit. `chilly` names one core device, `cpu`, and no `cpu_core`
  and `cpu_atom` pair (`devices.log:3-14`).
- The Zen 5 readings keep their lines:
  `results/requirement-1-zen5-paranoid2.txt` and
  `results/requirement-1-zen5-paranoid-minus1.txt`. This record cites the first
  for the standing `28` and `7` pair and for the no-refusal condition, and
  reopens nothing (`IA41-002`).
- The Intel non-hybrid rows stand measured at their heads in
  `results/acceptance-final-intel.md`, and that record's standing BLOCKED
  parity row keeps its text.

The AMD Zen 4 row of `SC-010` and of `IA41-002` is answered by this record at
`1f1f9b20cec97198e5cd88778167431eef2c6eb6`. A permission refusal stands as
blocked hardware acceptance and never as success (`IA41-002`), and no row here
rests on one. This record adds one dated bullet beside the `SC-010` bullet of
`spec.md`, under `Measurable Outcomes`, and moves no standing line there. It
ticks `Z4-041-001.1`, `Z4-041-001.2`, `Z4-041-002.1`, `Z4-041-002.2` and
`Z4-000.7` of `tasks.md`. It changes no product code, no test, no build file,
no gate, no schema, and no `README.md`.
