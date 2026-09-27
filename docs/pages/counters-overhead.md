# Counters sampling cost (SC-004, SC-010)

Documented measurement of the per-action cost of one
`recorder_handle::sample()` on this feature's plans, published as
min/median/max per regime (constitution Principle VII; T051, T056).
Fold cost is measured with the sampling path outside the loop, so the
two costs stay separable. This file is **not** a CI gate and **not** a
per-pull-request assertion: Principle VII's baseline infrastructure is
an open deferral, and regression wiring lands with the baseline spec.
Regenerate by running the registered test and copying its output:

```sh
cmake --build build/dev -t counters_overhead
./build/dev/test/counters_overhead
```

The test exits 2 when the host probes no fast read mechanism, and CTest
reports that as a skip with the reason in the output.

## Protocol

- Clock: `std::chrono::steady_clock`
- Build: the `dev` preset (`-Og`, `enforce` contracts)
- Measured quantity: wall time of one `sample()` call
- Warm-up: 64 actions per plan before measurement
- Sample count: 257 timed actions per plan, sorted, then reported as
  min / median / max
- Source of the numbers: `plan::sample_overhead_ns_min()`,
  `sample_overhead_ns_median()`, `sample_overhead_ns_max()` (FR-032);
  the first call on a plan runs the calibration, so compile itself
  performs no hardware read (FR-021)
- Fold cost: 1000 first-to-last folds over a 64-point recorder, divided
  by the fold count, sampling outside the timed region (SC-010)
- Validity: the plan's own `SG_ENSURE` checks that
  `min <= median <= max`; the test fails if the ordering breaks

## Load context

Host: `Linux 7.2.4-1-cachyos x86_64`, AMD Ryzen 9 9950X3D, 16 cores

`/proc/loadavg`:

```
0.79 1.36 0.79 2/717 2325537
```

The host was not idle, so these figures record a loaded system. Read
them as the order of magnitude of each regime. A platform baseline
is a different claim and this page does not make it.

## Regimes on this host

| plan | min ns | median ns | max ns | read mode |
| --- | --- | --- | --- | --- |
| clock only, one vDSO read per action | 50 | 60 | 80 | `syscall` |
| core PMU group, one `read()` per leader per action | 210 | 210 | 250 | `syscall` |
| first-to-last fold over 64 recorded points, sampling outside the loop | | 389 | | none |

A fold window from `i` to `j` costs two sampling actions plus the fold:
`2 * sample_overhead_ns_median()`, so 120 ns for the clock plan and
420 ns for the PMU group plan on this host.

## Fast regime: not probe-passing here

The fast regime has no measurement on this host. The test names the
probe evidence in place of a number it did not measure. Two
independent gates refuse the mapped-page mechanism:

- `/sys/devices/system/cpu/tsc_khz` is absent, so the kernel publishes
  no calibrated time-stamp frequency. The clock provider omits its
  `fast_tsc` leaf entirely (FR-034), which leaves no `fast_tsc` catalog
  entry to sample.
- `/sys/bus/event_source/devices/cpu/rdpmc` exists and names a page
  size, but the file is mode 0400 and owned by root, so this caller
  cannot open it. `perf_event_paranoid` is 2, and the kernel grants
  user counter reads at 1 or below, so the probe stops there first.

The catalog carries the refusal, so the reason travels with the
numbers:

```
perf event source 'cpu', PMU type 4; user counter reads stay in syscall
mode: perf_event_paranoid is 2; the kernel grants user counter reads at
1 or below, so the mapped-page read stays unprobed
```

To publish the fast side, run the same test on a host with
`perf_event_paranoid` at 1 or below, a readable `rdpmc` page, and
`tsc_khz` present. The test then prints the two regimes side by side
and the ratio between them. The expected shape is the one the design
names: a fast plan in the tens-of-cycles regime against a syscall plan
in the microsecond regime, a factor of roughly 20 to 50 on an
uncontended core. That expectation is a design target; the number this
page carries is whatever the host measured.

## Achieved modes

Every catalog entry discloses the read mode its plan will use (FR-023,
C-PRO-4), and the plan reads through the disclosed mechanism: a
fast-capable host serves the mapped-page read, and a fast window the
kernel refuses is a recoverable open failure, never a silent downgrade
to a read the catalog does not describe. On this host all 592 entries
across the machine and core-PMU objects disclose `syscall`.

## Privilege context

`/proc/sys/kernel/perf_event_paranoid` is 2. Per-thread events for the
calling process are permitted at that level, so hardware entries probe
as `countable` and the group read returns real counts. Multiplexing
does not occur on an idle group, so the enabled/running pair reports a
ratio of 1.000000 here. A host that oversubscribes its counters
produces a ratio below 1 and the fold reports `scaled` set; that
evidence is developer-machine material recorded in the pull request per
tasks.md T043 scenario 6.
