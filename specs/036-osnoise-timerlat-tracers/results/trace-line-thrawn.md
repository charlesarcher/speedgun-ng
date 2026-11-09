# The trace-line acceptance runs on thrawn

The record of the N13e trace-line phase of this lane (owner ruling
G-28, spec FR-029). It carries the audit point the fixture bytes stand
taken from, the two acceptance runs of FR-030, the unrooted suite row
apart, and the stale wording the phase settles.

- Host: thrawn, AMD Zen 5 Threadripper 9960X, 24 cores, 48 threads,
  kernel `7.3.0-5-cachyos`.
- Audit point: 2026-11-09, each engine set, the tracer mask on one
  cpu, and tracing on.
- Head: every dated row stands preliminary and names the pre-squash
  head it stands taken at. The acceptance rows below stand taken at
  the tree of the shared squash of the two lanes of
  `fix/fwq-example-n13e`: the squashed commit carries the code these
  rows measured (spec 045 phase 15, BFW-012).

## The audit point (2026-11-09)

Both engines write the per-cpu buffer
`sys/kernel/tracing/per_cpu/cpu<id>/trace`, the path the detector read
of specs/048 already reads.

The `osnoise` buffer carries one line per period, once per
`period_us`. The header legend names the nine figures after the
timestamp: RUNTIME IN US, NOISE IN US, percent OF CPU AVAILABLE, MAX
SINGLE NOISE IN US, then the interference counters HW, NMI, IRQ, SIRQ
and THREAD. The reading stands positional: the five counters stand the
last five figures of a period line. The legend bytes and the two
period lines, byte for byte:

```text
#           TASK-PID      CPU# |||||   TIMESTAMP    RUNTIME      NOISE  AVAILABLE  IN US     HW    NMI    IRQ   SIRQ THREAD
       osnoise/1-2542397 [001] ....1 1241576.199778: 1000000       1659  99.83410      19      0      0   1006    176      4
       osnoise/1-2542397 [001] ....1 1241577.199779: 1000000       1403  99.85970       8      0      0   1000    147      0
```

The `timerlat` buffer carries one pair of activation lines per
activation, one per context:

```text
          <idle>-0       [001] d.h1. 1241586.794794: #1     context    irq timer_latency       334 ns
      timerlat/1-2542785 [001] ....1 1241586.794797: #1     context thread timer_latency      3448 ns
```

The reads beside the lines, taken on the same host on the same day:

```
$ cat /sys/kernel/tracing/osnoise/period_us
1000000
exit 0

$ cat /sys/kernel/tracing/per_cpu/cpu1/buffer_size_kb
1410
exit 0
```

The count-file re-confirmation rode the same read. The `osnoise`
directory of the tracing tree carries no per-cpu count file of the
pinned form:

```
$ ls /sys/kernel/tracing/osnoise/per_cpu/cpu1/
timerlat_fd
exit 0
```

No `osnoise/cpu1` file stands with either engine set. Setting
`current_tracer` clears the buffer, so the lines of a buffer stand the
run's own (D-11).

The host tracing state the audit point found, and the state it left:
`current_tracer` reads `nop`, `tracing_cpumask` reads `ffff,ffffffff`,
and `tracing_on` reads `0`. The audit point wrote no tracefs file.

## The red-first reading (N13E-002)

The test rows landed first, with the count-file read still standing. The
suite answered red, and these are the failing checks it printed:

```
FAILED: theFiguresFoldThePeriodLinesOfEachCpuBuffer: the five figures sum the counter columns of the first session cpu's buffer (FR-027, SC-008)
FAILED: theFiguresFoldThePeriodLinesOfEachCpuBuffer: the five figures sum the counter columns of the second session cpu's buffer (FR-027, SC-008)
FAILED: theTimerlatLinesCountTheTwoContexts: the verified pair counts the irq figure at 1 and the thread figure at 1 (FR-028, SC-008)
FAILED: theMixedBufferAnswersTheSumsOfItsParsedLines: the sums stand the parsed lines of the buffer alone (FR-028, SC-008)
FAILED: theZeroCounterColumnStandsAFilledZero: a counter column reading 0 on every period line answers a filled 0 (FR-028, SC-008)
FAILED: the figures read the post-window content (FR-009, R-05)
FAILED: the figures read the post-window content (FR-009, R-05)
FAILED: the figures read the post-window content (FR-009, R-05)
8 checks failed
```

The first five lines carry rows a to d. The last three carry row i, the
serial-window and `mode=both` families of `theWindowLifecycleRows`.

The guard rows stood green before the code row and stand green after it:
`theCpuWithNoParsedLineStandsEveryFigureAbsent`, `thePeriodShapeRows`,
`theActivationShapeRows`, `theLegendAndPlantedHashLinesStandNoLine`, and
the report row `theTimerlatFiguresPrintTwoFilledAndThreeAbsent`. The two
shape arrays answer the absence rule one line at a time, so a read that
guesses a figure turns them red. The three standing fixture constants of
the report row kept their bytes through the phase.

## The acceptance runs (FR-030)

Both rows open at the tree of the shared squash of
`fix/fwq-example-n13e`, behind the rights a root shell carries for the
tracefs writes. A run whose host refuses the tracefs writes stands
BLOCKED with the command and the refusal.

The window token needs a word about the probe's time base. The probe
window counts raw time-stamp counter ticks: the `machine/tsc` leaf
publishes a count that asserts no rate (specs/008), and the window
loop compares tick deltas against the `precheck` figure multiplied by
1,000,000. thrawn's counter stands near 4.19 GHz (bogomips
`8387.89`), so the standing 3,000 ms token spans 0.72 s of host time.
That is shorter than the one-second period the audit point names, so
the token holds no period line: a `precheck=3000` run was measured on
the same host the same day, exited 0, and answered all five figures
`not available` with the buffer empty at the read. The acceptance runs
therefore stand at `precheck=18000`, a 4.3 s window in host time,
which spans the period four times over.

### The osnoise run

```
$ sudo ./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
    --jitter-probe=idle,mode=precheck,precheck=18000 --os-noise=osnoise \
    --json=$HOME/scratch/fwq-example-n13e/osnoise-final.json
exit 0
```

| Item | Reading |
| --- | --- |
| Window | 4.3 s of host time, `precheck=18000` |
| Resolved tracer cpus | cpu 1 |
| Per-cpu `buffer_size_kb` read | 1410, exit 0 |
| The five source figures | hardware 0, nmi 0, irq 5367, softirq 449, thread 1186 |
| `current_tracer` found, and left | `nop`, and `nop` |
| `tracing_cpumask` found, and left | `ffff,ffffffff`, and `ffff,ffffffff` |
| `tracing_on` found, and left | `0`, and `0` |

The figures stand beside the CCD naming the standing records use. The
compact placement of two threads plans cpus 0 and 24, both inside
`l3 0: 0-5,24-29`, which the standing records name `CCD0`, so the
candidate set stands the one free cpu of that group (FR-008). The
record names the resolved cpu list and the per-cpu `buffer_size_kb`
read beside the figures, and the state each run found and the state it
left: the saved word, the saved mask and `tracing_on` at `0` (SC-010).

### The timerlat run

```
$ sudo ./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
    --jitter-probe=idle,mode=precheck,precheck=18000 --os-noise=timerlat \
    --json=$HOME/scratch/fwq-example-n13e/timerlat-final.json
exit 0
```

| Item | Reading |
| --- | --- |
| Window | 4.3 s of host time, `precheck=18000` |
| Resolved tracer cpus | cpu 1 |
| Per-cpu `buffer_size_kb` read | 1410, exit 0 |
| The irq and thread figures | irq 4296, thread 4296 |
| The other three figures | the standing `not available` wording (D-10) |
| `current_tracer` found, and left | `nop`, and `nop` |
| `tracing_cpumask` found, and left | `ffff,ffffffff`, and `ffff,ffffffff` |
| `tracing_on` found, and left | `0`, and `0` |

The buffer size stands beside the figures because the sums stand the
totals of the lines the buffer holds at the read (D-11). At the 4.3 s
window the osnoise buffer holds four period lines. The timerlat buffer
holds thousands of activation lines against the 1,410 kB ring: the run
answers 4296 for each context, the two counts equal as the pair rule
states, and the ring capacity stands named beside them.

## The unrooted suite row

The suite row runs apart from the two sudo runs, on the standing tree,
with no root and no kernel tracer:

```
$ ctest --test-dir build -R 'harness_noise_test|harness_noise_report_test'
100% tests passed out of 2
exit 0
```

The row stands taken again on 2026-11-09 at the tree of the shared
squash, apart from the two sudo runs (SC-009). The coverage lane of
the shared set answers SC-011 at the same tree: 100.0% lines
(18779 of 18779), a branch shortfall of 21 rows inside the standing
allowance of 21, the marker count steady at 84 with no new marker, and
no cold row where the retired reader stood.

## The stale wording this phase settles (FR-029)

Ruling G-28 settles the surfaces that keep naming the retired count
files and the cumulative-totals reading. The console contract and the
quickstart link row move in this phase. The rest stand listed here,
with their text unchanged.

| Surface | Where it stands |
| --- | --- |
| The public header comment | `include/speedgun-ng/benchmark.hpp`, the pre-check tracer record comment naming the count files the engine states |
| The spec 034 report-json line | `specs/034-host-noise-precheck/contracts/report-json.md`, the tracer figure rows naming the cumulative figures the count files state |
| This lane's option contract | `contracts/cli-options.md`, the edge-case row for a tracer cpu whose count files state nothing at read time |
| This lane's report-json contract rows | `contracts/report-json.md`, the `hardware` row naming the source the kernel's files word `hw`, and the sentence that states the cumulative totals since the tracer's start |
| The data model | `data-model.md`, the row that states the harness reads the count files alone |
| The research rows | `research.md` R-3 and its open-questions row on the file spelling |
| The quickstart wording | `quickstart.md`, the fake-file suite path naming the per-cpu count files |
| The specs/048 rows | that lane's `spec.md`, `plan.md` and `research.md` rows naming the retired path and the retired reader symbol, listed as that lane's stale record |
| The two standing edge-case rows of `spec.md` | the row for a tracer cpu whose count files state nothing, and the `mode=both` row that states the cumulative totals since the tracer's start |

The standing 2026-10-29 records keep their text as the record of what
was decided then (D-12).
