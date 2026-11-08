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
