# Intel final acceptance: the counter-count part

**Spec**: [spec.md](../spec.md) | **Plan**: [plan.md](../plan.md) | **Date**: 2026-11-07
**Branch**: `docs/intel-final-acceptance` | **Shared plan**: `specs/048-hwlat-probe/plan.md` | **Gates**:
`specs/048-hwlat-probe/results/acceptance-final-intel.md`

## Identity

| Field | Reading | Command |
|---|---|---|
| Host | CorsairOne | `uname -n` |
| Cpu | Intel(R) Core(TM) i9-9920X CPU @ 3.50GHz, family 6, model 85 | `grep -m1 "model name" /proc/cpuinfo` |
| PMU name | `skylake`, one `cpu` device, non-hybrid | `cat /sys/devices/cpu/caps/pmu_name` |
| Kernel | Linux 7.3.0-4-cachyos, x86_64 | `uname -a` |
| Library | speedgun-ng 0.34.0 | run header |
| Source base | `615c0964547e4e9768b8e2f2d79991033a0eb5bd` | `git rev-parse origin/master` |
| Measured head | `615c0964547e4e9768b8e2f2d79991033a0eb5bd` | `git rev-parse HEAD` |
| Root session | none | `sudo -n -l` -> `sudo: a password is required`, exit 1 |

## The device and the kernel settings (IA41-001.1)

| Fact | Reading | Path | Mode |
|---|---|---|---|
| Device list | one `cpu` device, plus `breakpoint`, `software`, `tracepoint`, `kprobe`, `uprobe`, `msr`, `cstate_core`, `cstate_pkg`, `intel_pt`, `power` and the `uncore_*` devices | `/sys/bus/event_source/devices/` | readable |
| Device type | `4` | `/sys/bus/event_source/devices/cpu/type` | `-r--r--r-- root root` |
| Event file | present, and it decodes to a config word this record does not print | `/sys/bus/event_source/devices/cpu/events/branch-instructions` | `-r--r--r-- root root` |
| `pmu_name` | `skylake` | `/sys/devices/cpu/caps/pmu_name` | readable |
| `perf_event_paranoid` | -1 | `/proc/sys/kernel/perf_event_paranoid` | `-rw-r--r-- root root` |
| `nmi_watchdog` | 0 | `/proc/sys/kernel/nmi_watchdog` | `-rw-r--r-- root root` |

`perf stat -a` and any typed probe tool stand outside this record on purpose: the first
argument of `specs/041-counter-probe/tools/group_probe.c` is a raw config word and its
output prints one. `tools/probe_window.c` is the other standing shape. `group_probe`
stands not run. No row here prints a raw config value.

## The example, twice (IA41-001.2)

```
./build/example/docs_counter_probe
```

| Run | Exit | Line | Benchmark line |
|---|---|---|---|
| 1 | 0 | `counter limit: cpu limit=4 source=decode probe=4` | `bmProbeTick iterations=1515109752 real time/iter (ns)= 0.448` |
| 2 | 0 | `counter limit: cpu limit=4 source=decode probe=4` | `bmProbeTick iterations=1503440892 real time/iter (ns)= 0.446` |

Both runs stood alone, each with a load average and a count of other build or test
processes at its start; the counts read 0. An earlier pass ran at 14:23 inside the window
of a coverage suite and contributes no figure. Captures sit under `/tmp/ia/clean`.

The published limit is 4 with the source word `decode`, and the example's own probe
answers 4. The library pays no open on this host, because its decode answers, and the run
prints no group figures. The committed `.expected` block of this example is another
host's reading, so this record claims neither a match nor a failure of a byte compare.

## The watchdog rule as the library applies it (IA41-001.3)

The library subtracts the watchdog's own counter from the count it reports, once, on the
decoded figure. On this host `nmi_watchdog` reads 0, so the deduction is a no-op on a
part with a fixed cycles counter. This record adds no hand subtraction anywhere.

## The parity row (IA41-002.1)

Blocked. The standing row of this spec was read with `perf_event_paranoid` at 2. Raising
or lowering that value is a write to `/proc/sys/kernel/perf_event_paranoid`, which needs
root, and no root session stands available: `sudo -n -l` answers at exit 1. This host
reads -1, which is not the equal of the standing 2 reading, so the parity row carries no
figure. The device modes and the root status stand in the row above.

## The other rows (IA41-002.2)

The Arm row, the Intel hybrid row and the Zen 4 row stay pending. This host is a
non-hybrid Intel part with `pmu_name` `skylake`, so it can answer neither. No completed
row this host measured contradicts them.


## Rerun at head `87d745d6c7e62d4da80198c689ddc7078f08ff3d` (2026-11-08)

| Item | Reading | Command |
|---|---|---|
| PMU name | `skylake` | `cat /sys/devices/cpu/caps/pmu_name` |
| Device `type` | `4` | `cat /sys/devices/cpu/type` |
| `nmi_watchdog` | `0` | `cat /proc/sys/kernel/nmi_watchdog` |
| `perf_event_paranoid` | `-1` | `cat /proc/sys/kernel/perf_event_paranoid` |
| Devices | one `cpu` device among `breakpoint`, `cpu`, `msr`, `power`, `software`, `tracepoint`, uncore units, `intel_pt`, `kprobe`, `uprobe`, cstate units | `ls /sys/bus/event_source/devices/` |

Two runs of `./build/example/docs_counter_probe`, each alone with its load witness at its
start, each exit 0, both printing the same limit line:

| Run | Exit | Limit line | Benchmark line |
|---|---|---|---|
| 1 | 0 | `counter limit: cpu limit=4 source=decode probe=4` | `bmProbeTick iterations=1573739252 real time/iter (ns)= 0.445` |
| 2 | 0 | `counter limit: cpu limit=4 source=decode probe=4` | `bmProbeTick iterations=1573733351 real time/iter (ns)= 0.445` |

`cpu limit=4` from `source=decode` reproduces on this head, and `probe=4` agrees with it.
The parity row stays blocked for the same reason as above: this session holds no right over
`/dev/cpu/*/msr`, which reads `crw------- root root 202, 0`. The Arm, Intel hybrid and
Zen 4 rows stay pending. No raw event configuration value stands in this record.

## Acceptance closeout corrections (2026-11-08)

One dated section holds every correction. A line number written as `:n` names this file as
it stands above here, so an older row keeps its text and its heading while a row below
names the line it replaces. Every figure names the command or the file line behind it. This
pass ran no build, no test, no tracer and no command that changes system state.

This section was written against `af54d3f0`, the head this branch carries. A
cite into a spec reads at `af54d3f0`, and a source or example anchor reads at
the merged base `a55728af`, where it stands unchanged. A `:n` line at or below
98 stands at `a55728af` too.

Clauses behind this section: `specs/041-counter-probe/spec.md:984-998` (read at `af54d3f0`) (IP-001, IP-002),
`specs/048-hwlat-probe/spec.md:1815-1819` (read at `af54d3f0`) (ruling 11),
`specs/048-hwlat-probe/spec.md:1825-1830` (read at `af54d3f0`) (ruling 13),
`specs/048-hwlat-probe/spec.md:1902-1905` (read at `af54d3f0`) (ruling 27) and
`specs/048-hwlat-probe/spec.md:1676-1679` (read at `af54d3f0`) (SC-IC-5).

### The two device type files (IP-001)

| Item | Reading | Command |
|---|---|---|
| `cpu` device type | `4` | `cat /sys/bus/event_source/devices/cpu/type` |
| `msr` device type | `11` | `cat /sys/bus/event_source/devices/msr/type` |
| `perf_event_paranoid` | `-1` | `cat /proc/sys/kernel/perf_event_paranoid` |
| `nmi_watchdog` | `0` | `cat /proc/sys/kernel/nmi_watchdog` |

The two type numbers differ, so the kernel holds `msr` as a device with its own number. A
group open is a `perf_event_open` syscall
(`example/docs/counter_probe.cpp:287-292` (read at `a55728af`)) whose attribute carries the core device's own
type number: `example/docs/counter_probe.cpp:196` (read at `a55728af`) reads that device's `type` file and
`example/docs/counter_probe.cpp:253` (read at `a55728af`) stores the value in `attr.type`. The syscall opens no
file under `/dev/cpu`. The library asks the core device names of
`source/counters/linux_pmu/counter_limits.cpp:357-361` (read at `a55728af`) and passes that device's own type
number at `source/counters/linux_pmu/limit_fill.cpp:72` (read at `a55728af`). The `msr` device enters no probe
path.

### The parity rows (ruling 11, IP-002)

The measured agreement line stands as this record prints it at `:95`:

> `cpu limit=4` from `source=decode` reproduces on this head, and `probe=4` agrees with it.

| Row | Status | Basis |
|---|---|---|
| Decode against probe parity at the permissions this host answers now | measured | `:92-93` print `counter limit: cpu limit=4 source=decode probe=4` at exit `0`, and `:95` states the agreement quoted above |
| The same parity with `perf_event_paranoid` at `2` | BLOCKED | Control: `/proc/sys/kernel/perf_event_paranoid`. Operator action: set `2`, then restore the `-1` this host carries. Missing right: a root session holding that write. |
| Root session this pass | none | `sudo -n -l` -> `sudo: a password is required`, exit `1` |

The reason that holds for the blocked row is the write to
`/proc/sys/kernel/perf_event_paranoid`, the reason this record already gives at `:64-68`. The
node mode reads `crw------- 1 root root 202, 0` under `ls -l /dev/cpu/0/msr`, measured again
this pass. That mode belongs to a character node no group open opens, so it gates no parity
row. The sentence at `:96-97` keeps its text and its heading, and the BLOCKED row above
replaces it. That row carries the control, the value and the missing right, and no measured
figure.

### Opens and reads (ruling 13, IP-001)

| Figure | Kind | Basis |
|---|---|---|
| The library answers the limit with `0` device files open | measured | `:92` prints `source=decode`, the word `source/counters/linux_pmu/limit_fill.cpp:94-95` (read at `a55728af`) writes, and the decoded arm at `source/counters/linux_pmu/limit_fill.cpp:88-95` (read at `a55728af`) holds no open call |
| The example's own search opens up to `15` files and reads up to `5` before it answers `4` | derived | `example/docs/counter_probe.cpp:329-338` (read at `a55728af`) walks the sizes upward, `example/docs/counter_probe.cpp:283-298` (read at `a55728af`) pays one open per copy of a size, and `example/docs/counter_probe.cpp:303` (read at `a55728af`) pays one read of the group leader for each size whose opens all landed |

The run measures the source word at `:92`, the arm that word names holds no open call, and no
tracer read a syscall table anywhere in this closeout. An answer of `4` comes from that walk
running sizes `1`, `2`, `3`, `4` and `5`, the last one short. The opens are
1 + 2 + 3 + 4 + 5 = 15 and the reads are one per size, so `5`.
`example/docs/counter_probe.cpp:88` (read at `a55728af`) sets the size cap to `32`, so no cap binds this walk. The
standing proof record prints the same arithmetic for an answer of `6` at
`specs/041-counter-probe/results/requirement-1-zen5-paranoid2.txt:58`: sizes opened `7`,
opens paid `28`, reads `7`. The same file states at `:53` that every group open answered `0`
up to k = 96, the condition under which a short size pays its opens and its read too, and
`specs/041-counter-probe/results/requirement-1-shipped-shape.txt:329` carries the same pair,
`28` opens and `7` reads, for that answer. No tracer ran, so `15` and `5` stand derived from
the code lines named above, and the runs at `:92-93` measured the answer `4` they follow
from.

### History labels (ruling 27, SC-IC-5)

The older records keep their lines. Each row names what its item is.

| Item | What it is | Reading |
|---|---|---|
| `/tmp/041-notes/group_probe_ship.c` | the tool copy outside the repository that a standing proof record depends on | `specs/041-counter-probe/results/requirement-1-shipped-shape.txt:100` records a copy of `tools/group_probe.c` extended by hand, `:107` builds it and `:123-124` run it; `ls /tmp/041-notes` answers `No such file or directory`, exit `2`, measured this pass, so that build line cannot run on this host |
| `specs/041-counter-probe/tools/group_probe.c` | the tracked tool that stands in its place | `specs/041-counter-probe/tools/group_probe.c:112-113` reads two arguments and `:36` fixes `attr.type` to `PERF_TYPE_HARDWARE`, so the tracked tool takes no device type argument and answers no row that needs one |
| `Linux 7.3.1-1-cachyos` with `perf_event_paranoid=2` | the kernel string and the `2` reading of another host | `specs/041-counter-probe/results/probe-window-trials.txt:17-18` prints that pair under the label `the acceptance host` at `:1`, and the file names no host and no cpu; `uname -srmo` answers `Linux 7.3.0-4-cachyos x86_64 GNU/Linux` and `cat /proc/sys/kernel/perf_event_paranoid` answers `-1` here, both measured this pass; `specs/041-counter-probe/results/requirement-1-shipped-shape.txt:5` and `:7` name Gary with that kernel string and that value, and `specs/041-counter-probe/results/requirement-1-zen5-paranoid2.txt:5-6` record the same osrelease with the same value |

### The watchdog rule, applied once (IP-001)

`source/counters/linux_pmu/limit_fill.cpp:93` (read at `a55728af`) holds the one application: the decoded arm at
`source/counters/linux_pmu/limit_fill.cpp:88-95` (read at `a55728af`) calls `usableCounters` once on the decoded
figure. The probed arm at `source/counters/linux_pmu/limit_fill.cpp:96-110` (read at `a55728af`) holds no call,
because a search answers the number of copies the kernel ran whole and that grant already
holds the watchdog's counter, the reading recorded at
`source/counters/linux_pmu/limit_fill.cpp:99-102` (read at `a55728af`).
`source/counters/linux_pmu/counter_limits.cpp:342-343` (read at `a55728af`) returns the figure unchanged when
`nmi_watchdog` reads a value other than `1`, and `cat /proc/sys/kernel/nmi_watchdog` answers
`0` here, measured this pass. No second application stands claimed anywhere in this record,
and IP-001 refuses one as a correction.

## The operator command pair for the restricted reading (2026-11-09)

This reading stands in its own dated section, because the section above states
that one dated section holds every correction it makes, and this row corrects no
line above it. Every line above keeps its text, its heading and its mark.

The standing blocked row of `The parity rows` at `:146` (read at `af54d3f0`) names
the control, the action and the missing right, and it carries no command line.
`IP-002` of `specs/041-counter-probe/spec.md` (read at `af54d3f0`), restated for
the merged base, asks for the command an operator types and for the command that
sets the value back, so this row carries the pair. No figure stands here, because
no run at the final head has answered it. The dated line that lands beside the
standing blocked row stands under `IA41-016` of
`specs/041-counter-probe/tasks.md` (read in this wave's tree).

This section was written against `af54d3f0`, the head this branch carries, and the
section itself stands in the tree this wave wrote there, so no head carries it
yet. Each log cite below keeps the head the identity step of the section above
printed.

| Row | Status | Basis |
|---|---|---|
| The same parity with `perf_event_paranoid` at `2`, at the final head | BLOCKED | Read first: `cat /proc/sys/kernel/perf_event_paranoid` answers `-1`, printed at `identity.log:34` of the run whose head the section above names; that log answers the affinity list at `:38`, so `:38` names no `perf_event_paranoid` value. Set: `sysctl -w kernel.perf_event_paranoid=2`. Restore: `sysctl -w kernel.perf_event_paranoid=-1`. No session here can type either line: `sudo -n -l` answers `sudo: a password is required` at exit `1` (`identity.log:49`), and `pkexec --disable-internal-agent id` answers that no authentication agent is found (`identity.log:50`). The parity row answers when an operator opens a root session, reads the value, sets `2`, runs `./build/example/docs_counter_probe`, restores `-1`, and reads the value again. |

## Corrections at the merged base (2026-11-09)

This block was written against `af54d3f0`, the head this branch carries, whose
base is the merged head `a55728af17ec02ea4fbe5e5ff7f8a537ddaa224a`, the base
head that the `Base head` row of `The base this work reads` of
`specs/041-counter-probe/spec.md` (read at `af54d3f0`) names. Every cite in the
table below names an id, a ruling with its list, or a heading of
`specs/048-hwlat-probe/spec.md` or `specs/041-counter-probe/spec.md`, so each
one is text this branch wrote and reads at `af54d3f0`. At `a55728af` neither
spec carries an `SC-IC-` id, an `IP-` id, or a ruling list for this phase. A bare `:n` in this block names the artifact the sentence names last,
and this file when it names none; a line of this file at or below 98 stands
at `a55728af` too, and the `:201-223` range of the section above stands
in the tree this wave wrote, where no head carries it yet. Every line above
keeps its text, its heading and its mark. Each row below names the line it
replaces and gives the cite that holds, beside the head its target was read at.
The four lines that re-point those cites stand under `IA41-015` of
`specs/041-counter-probe/tasks.md` (read in this wave's tree). This pass ran no
build, no test, no tracer and no command that changes system state.

| Line | Cite as written | What the cited text holds | The cite that holds |
|---|---|---|---|
| `:113` | a range into `specs/048-hwlat-probe/spec.md` (read at `af54d3f0`), labeled ruling 11 | it opens on the closing line of ruling 9, and it holds ruling 10, "The record labels each head it measured, and the reruns are not carries" | ruling 11 of the 2026-11-08 closeout list of `specs/048-hwlat-probe/spec.md` (read at `af54d3f0`), "Parity gets two rows", the clause `### The parity rows` at `:137` cites |
| `:114` | a range into `specs/048-hwlat-probe/spec.md` (read at `af54d3f0`), labeled ruling 13 | it opens on the closing lines of ruling 11, and it opens ruling 12, "Three consoles per command, normal method only" | ruling 13 of the 2026-11-08 closeout list of `specs/048-hwlat-probe/spec.md` (read at `af54d3f0`), "Opens and reads: one measured number, one derived number, no new tool", the clause `### Opens and reads` at `:157` cites |
| `:115` | a range into `specs/048-hwlat-probe/spec.md` (read at `af54d3f0`), labeled ruling 27 | it holds ruling 25, "A planted reading closes its own row and no more" | ruling 27 of the 2026-11-08 closeout list of `specs/048-hwlat-probe/spec.md` (read at `af54d3f0`), "History keeps its paths, and a new row does not", the clause `### History labels` at `:178` cites |
| `:116` | a range into `specs/048-hwlat-probe/spec.md` (read at `af54d3f0`), labeled SC-IC-5 | it opens on the closing lines of SC-IC-4, which speaks about the `dbc-gate` target | SC-IC-5 of `Success Criteria - this extension` of `specs/048-hwlat-probe/spec.md` (read at `af54d3f0`) |

The shifted SC-IC-5 cite stands in two other records too. The dated section
`Acceptance closeout corrections (2026-11-08)` of
`specs/046-hunter-topdown/results/acceptance-hunt.md` and the dated section
`Acceptance closeout correction (2026-11-08)` of
`specs/046-hunter-topdown/results/pr-body.md` each open with the same mislabeled
range, and each of those three sections now answers with SC-IC-5 of
`Success Criteria - this extension` of `specs/048-hwlat-probe/spec.md` (read at
`af54d3f0`).

The section `The operator command pair for the restricted reading (2026-11-09)`,
the last section above at `:201-223`, is the row that supplies the two command
lines the standing blocked row at `:146` lacks. The blocked row there names the
control, the value the host answered before the change, the line that sets `2`, the line that
sets the value back, and the right this session lacks.
`IP-002` of the `Requirements restated for the merged base (IP-001 to IP-002)`
list of `specs/041-counter-probe/spec.md` (read at `af54d3f0`) asks a blocked row
for that shape, and that same clause names `:146` as the row that carried none of
it.

## Pass 1 control figures at the candidate head (2026-11-09)

Pass 1 of ruling 29 of `Rulings - this phase (29 to 42)` of
`specs/048-hwlat-probe/spec.md` (read at `af54d3f0`) ran the whole final-head set
at a candidate head. The run stood on `af54d3f0`, the short form of
`af54d3f0e52901339e8703fc7991eb3da0db4435`, the sha `git rev-parse af54d3f0`
prints, whose base is the merged head `a55728af`
(`a55728af17ec02ea4fbe5e5ff7f8a537ddaa224a`). Every figure below is a Pass 1
control figure measured at `af54d3f0`, and a Pass 2 run at the head Pass 2 runs
at replaces each of them. No standing row moves and no standing table changes
its text: a figure that belongs beside a standing table lands here as a dated
row that names that table by its heading.

Each figure names one step log under `/tmp/ic/p1/`, and each log opens with its
own command line, its own load witness and its own exit. Ruling 30 of the same
list holds a figure where that lane's own witness line reports no other
compiler, test, capture or trace process and a one-minute load average below
2.0. Each witness quoted below carries both readings.

**The three counter consoles.** Dated rows beside `The example, twice
(IA41-001.2)`, which keeps its text. The command
`./build/example/docs_counter_probe` ran three times in the release tree
`build`, each run alone at `af54d3f0`, each at exit 0, each printing the same
limit line.

| Console | Exit | Limit line | Benchmark line | Load witness | Ruling 30 |
|---|---|---|---|---|---|
| `f6-probe-1.log` | 0 | `counter limit: cpu limit=4 source=decode probe=4` | `bmProbeTick              iterations=1573614428  real time/iter (ns)=    0.460  cpu time/iter (ns)=    0.459  overhead floor (ns)=  677.000  endpoint samples=2` | `1.42 2.14 2.69 1/1560 3096935` | stands |
| `f6-probe-2.log` | 0 | `counter limit: cpu limit=4 source=decode probe=4` | `bmProbeTick              iterations=1573776980  real time/iter (ns)=    0.445  cpu time/iter (ns)=    0.445  overhead floor (ns)= 1054.000  endpoint samples=2` | `1.54 2.15 2.69 1/1558 3097008` | stands |
| `f6-probe-3.log` | 0 | `counter limit: cpu limit=4 source=decode probe=4` | `bmProbeTick              iterations=1573736284  real time/iter (ns)=    0.446  cpu time/iter (ns)=    0.445  overhead floor (ns)=  671.000  endpoint samples=2` | `1.54 2.13 2.68 2/1559 3097075` | stands |

Each of the three witnesses also reads `other ctest or compiler processes: 0`,
so all three rows stand. The limit line repeats across the three consoles at
this head: `cpu limit=4`, the source word `decode`, and the example's own probe
answer `4`. The three iteration counts read 1573614428, 1573776980 and
1573736284, and the three real time per iteration read 0.460 ns, 0.445 ns and
0.446 ns. Each console opens with `speedgun-ng 0.34.0 build=Release`.

**The two device reads.** Pass 1 took neither reading, so this row carries no
figure. The harvest of the pass lists no step that reads
`/sys/devices/cpu/caps/pmu_name` and no step that reads
`/sys/bus/event_source/devices/cpu/type`, and no log under `/tmp/ic/p1/` holds
either answer, so the pass printed nothing to read. The `Counter device` row of
`Lane Table` of `specs/048-hwlat-probe/plan.md` names both commands and the
order it asks for them in, and the Pass 2 run of that row answers them. The
`PMU name` row of `Identity`, the `cpu` device type row of `The two device type
files (IP-001)` and the `pmu_name` and `Device type` rows of `The device and the
kernel settings (IA41-001.1)` keep their older readings at this head and gain no
figure here.

**The blocked row this section leaves open.** The parity row with
`perf_event_paranoid` at `2` stays blocked at `af54d3f0` and carries no figure.
Ruling 42 of `Rulings - this phase (29 to 42)` of
`specs/048-hwlat-probe/spec.md` (read at `af54d3f0`) gives that row its shape,
and `The operator command pair for the restricted reading (2026-11-09)` above
holds the value the host answered before any change, the line that sets `2` and
the line that sets it back. Pass 1 read that control at `-1` in its identity
step `f0-identity.log`, and no step of that pass wrote to it.

## Second pass figures at the final head (2026-11-09)

This section reports the second pass. It ran at head
`208bc520f85a8a6a95253feb80ebaba1cc9d7bef` over base `c9187ba3` on `corsairone`
with the host idle. Ruling 30 of `Rulings - this phase (29 to 42)` of
`specs/048-hwlat-probe/spec.md` governs the witness, and no row below carries a
void marking. Each row names its step under `/tmp/ic/p2/`.

**The three counter consoles at `208bc520`.** `f6-probe-1`, `f6-probe-2` and
`f6-probe-3` exit 0 in 4.0, 4.0 and 3.0 s. Each ends with the same line:
`counter limit: cpu limit=4 source=decode probe=4`. `f6-pmu-name` exits 0 and
answers `skylake`. `f6-cpu-type` exits 0 and answers `4`.

**The vendor tables, the two counts and the two counts of rows.** `f8-pmu-tables`
exits 0. The build-tree count `f8-count-build` answers 190, and its guard
`f8-count-build-guard` reads 190 count lines. The dev-tree count `f8-count-dev`
answers 185, and its guard `f8-count-dev-guard` reads 185 count lines. These are
the two standing counts the phase asks for, and the first pass never took them.
`f8-readme-count` reads `1003 README.md`, and `f8-marker-count` reads
`source/simulation/marker.cpp:2`.

**The interpreter each build tree names.** Each reading comes from that tree's own
`CMakeCache.txt`, and the key differs between trees. `build` and `build/coverage`
carry `_Python3_EXECUTABLE:INTERNAL=/home/archerc/.local/bin/python3.11`.
`build/dev` carries `Python3_EXECUTABLE:FILEPATH=/usr/bin/python3`. The
interpreter the `coverage` tree names answers 1 for a YAML import at step
`f1-cov-py-yaml`, so that tree cannot run a gate that imports YAML. The system
interpreter answers 0 for the same import at step `f5-dev-py-yaml`, and
`f5-dev-py-version` prints its version. The driver reads whichever key the cache
carries, and a step that finds neither key fails with its own message.

**The counter traps.** `f9-counters-trap` exits 0 and prints its precondition
line `[precondition] hard_stop recorder samples within capacity (FR-027)
(predicate: head < capa...`. `f9-counters-trap-checked` exits 0 and ends `Total
Test time (real) =   0.11 sec`. `f9-check-symbols` exits 0 and ends `PASS: no
sg::dbc::check symbols`.

**The blocked row stays blocked at `208bc520`.** The parity row with
`perf_event_paranoid` at `2` carries no figure, because this host answers `-1` and
the phase forbids an unattended write to that control. The operator line is `sudo
sysctl -w kernel.perf_event_paranoid=2`, the restore line is `sudo sysctl -w
kernel.perf_event_paranoid=-1`, and the identity reading of the same run is the
`f0-identity` step, which exits 1 on the package query and prints the control at
`-1`. Ruling 42 of `Rulings - this phase (29 to 42)` of
`specs/048-hwlat-probe/spec.md` gives that row its shape.
