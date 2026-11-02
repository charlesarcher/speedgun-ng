# acceptance run: fwq and ftq probes (specs/043-fwq-ftq)

Run id: 20261102T165728Z
Script: tools/noise/fwq_ftq_acceptance.sh (FR-027, FR-028, SC-008)

### Host state

- Date: 2026-11-02
- Host: chilly
- Kernel: Linux 7.3.1-1-cachyos x86_64
- CONFIG_HZ: 1000 (source: /proc/config.gz)
- Machine: AMD Ryzen 9 7950X3D 16-Core Processor, 16 cores per socket, 2 threads per core
- L3 group ids: 0, 1
- Probe cpus: cpu 0 of L3 group 0 over cpus 0-7,16-23; cpu 8 of L3 group 1 over cpus 8-15,24-31; 
- Load: a busy loop pinned to the SMT sibling of the probe cpu, run as
  `taskset -c <cpu id> sh -c 'while :; do :; done'`
- SMT state: 1
- CPU frequency governor: powersave
- Huge pages: [always] madvise never
- Kernel command line: `initrd=\initramfs-linux-cachyos.img root=UUID=fc417ad6-10f5-422c-b527-d76f7936d3f4 rw zswap.enabled=0 nowatchdog quiet splash mitigations=off vsyscall=emulate clearcpuid=514`
- isolated cpus: ``
- nohz_full cpus: ``

The two L3 groups carry the acceptance axis of the lane host (D-10):
FWQ and FTQ each run on one idle cpu of each group, at rest and under a
load on the SMT sibling thread, eight sections. The isolated and
nohz_full lists stand empty when the kernel command line carries no
isolcpus and no nohz_full setting, so the probe shares the machine with
the ordinary host load.

## FWQ on cpu 0 of L3 group 0, idle

- probe cpu: 0
- L3 group: 0 over cpus 0-7,16-23
- load state: idle
- gaps: 10485760
- load command line: none

```sh
stdbuf --output=0 /home/archerc/code/speedgun-ng/build/example/benchmark_example --filter=bmTouch --min-time=10 --repetitions=1 --jitter-probe=0\,method=fwq\,mode=precheck\,precheck=10000\,gaps=10485760 
```

Exit status: 139

```text
speedgun-ng 0.29.0 build=Release
method: fwq
workCount: 1
targetNs: 1000
calibrationSpanNs: 337922256
precheck cpu 0 (candidate)
gapCount: 0
gapsPerSecond: 0.00
minNs: 0
medianNs: 0
meanNs: 0.00
p90Ns: 0
p99Ns: 0
p999Ns: 0
p9999Ns: 0
maxNs: 0
totalNs: 0
totalPercent: 0.00
runtimeNs: 0
thresholdNs: 420
loopPeriodNs: 42
droppedGaps: 0
interrupts delta: 4984 (source: /proc/interrupts)
softirq HI delta: 0 (source: /proc/softirqs)
softirq TIMER delta: 168 (source: /proc/softirqs)
softirq NET_TX delta: 0 (source: /proc/softirqs)
softirq NET_RX delta: 0 (source: /proc/softirqs)
softirq BLOCK delta: 0 (source: /proc/softirqs)
softirq IRQ_POLL delta: 0 (source: /proc/softirqs)
softirq TASKLET delta: 0 (source: /proc/softirqs)
softirq SCHED delta: 720 (source: /proc/softirqs)
softirq HRTIMER delta: 0 (source: /proc/softirqs)
softirq RCU delta: 384 (source: /proc/softirqs)
switch delta: 0, schedule delta: 0 (source: /proc/schedstat)
host settings: isolcpus=not available nohz_full=not available rcu_nocbs=not available
governor: cpu 0 powersave (source: /sys devices system cpu scaling_governor)
irq affinity: cpu 0 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,27,28,29,30,31,32,33,34,35,36,37,39,40,42,43,52,61,69,77,87,88,104,135,137,138,139,155,157,158,159,160,161,162,163 (source: /proc/irq)
pinning candidates:
bmTouch                  iterations=16772277551  real time/iter (ns)=    0.646  cpu time/iter (ns)=    0.645  overhead floor (ns)=  100.000  endpoint samples=2
jitter probe: cpus 0
gapCount: 0
gapsPerSecond: 0.00
minNs: 0
medianNs: 0
meanNs: 0.00
p90Ns: 0
p99Ns: 0
p999Ns: 0
p9999Ns: 0
maxNs: 0
totalNs: 0
totalPercent: 0.00
runtimeNs: 0
thresholdNs: 420
loopPeriodNs: 42
droppedGaps: 0
```

The figures of this section:

- calibration count (`workCount`): 1
- measured unit time (`unitTimeNs`): absent ns
- noise fraction (`noisePercent`): absent
- `gaps=` value: 10485760
- kept span (`seriesRuntimeNs`): absent ms
- drop count (`droppedSamples`): absent
- detected periods (`periodNs`): rank 1 at absent ns, rank 2 at absent ns, rank 3 at absent ns, rank 4 at absent ns, rank 5 at absent ns

## FWQ on cpu 0 of L3 group 0, loaded

- probe cpu: 0
- L3 group: 0 over cpus 0-7,16-23
- load state: loaded
- gaps: 10485760
- load cpu: 16, the SMT sibling of probe cpu 0
- load command line: `taskset -c 16 sh -c 'while :; do :; done'`

```sh
stdbuf --output=0 /home/archerc/code/speedgun-ng/build/example/benchmark_example --filter=bmTouch --min-time=10 --repetitions=1 --jitter-probe=0\,method=fwq\,mode=precheck\,precheck=10000\,gaps=10485760 
```

Exit status: 139

```text
speedgun-ng 0.29.0 build=Release
method: fwq
workCount: 1
targetNs: 1000
calibrationSpanNs: 1690131198
precheck cpu 0 (candidate)
gapCount: 0
gapsPerSecond: 0.00
minNs: 0
medianNs: 0
meanNs: 0.00
p90Ns: 0
p99Ns: 0
p999Ns: 0
p9999Ns: 0
maxNs: 0
totalNs: 0
totalPercent: 0.00
runtimeNs: 0
thresholdNs: 420
loopPeriodNs: 42
droppedGaps: 0
interrupts delta: 21473 (source: /proc/interrupts)
softirq HI delta: 0 (source: /proc/softirqs)
softirq TIMER delta: 267 (source: /proc/softirqs)
softirq NET_TX delta: 0 (source: /proc/softirqs)
softirq NET_RX delta: 0 (source: /proc/softirqs)
softirq BLOCK delta: 0 (source: /proc/softirqs)
softirq IRQ_POLL delta: 0 (source: /proc/softirqs)
softirq TASKLET delta: 0 (source: /proc/softirqs)
softirq SCHED delta: 54 (source: /proc/softirqs)
softirq HRTIMER delta: 0 (source: /proc/softirqs)
softirq RCU delta: 250 (source: /proc/softirqs)
switch delta: 0, schedule delta: 0 (source: /proc/schedstat)
host settings: isolcpus=not available nohz_full=not available rcu_nocbs=not available
governor: cpu 0 powersave (source: /sys devices system cpu scaling_governor)
irq affinity: cpu 0 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,27,28,29,30,31,32,33,34,35,36,37,39,40,42,43,52,61,69,77,87,88,104,135,137,138,139,155,157,158,159,160,161,162,163 (source: /proc/irq)
pinning candidates:
bmTouch                  iterations=23693530908  real time/iter (ns)=    0.912  cpu time/iter (ns)=    0.910  overhead floor (ns)=  130.000  endpoint samples=2
jitter probe: cpus 0
gapCount: 0
gapsPerSecond: 0.00
minNs: 0
medianNs: 0
meanNs: 0.00
p90Ns: 0
p99Ns: 0
p999Ns: 0
p9999Ns: 0
maxNs: 0
totalNs: 0
totalPercent: 0.00
runtimeNs: 0
thresholdNs: 420
loopPeriodNs: 42
droppedGaps: 0
```

The figures of this section:

- calibration count (`workCount`): 1
- measured unit time (`unitTimeNs`): absent ns
- noise fraction (`noisePercent`): absent
- `gaps=` value: 10485760
- kept span (`seriesRuntimeNs`): absent ms
- drop count (`droppedSamples`): absent
- detected periods (`periodNs`): rank 1 at absent ns, rank 2 at absent ns, rank 3 at absent ns, rank 4 at absent ns, rank 5 at absent ns

## FWQ on cpu 8 of L3 group 1, idle

- probe cpu: 8
- L3 group: 1 over cpus 8-15,24-31
- load state: idle
- gaps: 10485760
- load command line: none

```sh
stdbuf --output=0 /home/archerc/code/speedgun-ng/build/example/benchmark_example --filter=bmTouch --min-time=10 --repetitions=1 --jitter-probe=8\,method=fwq\,mode=precheck\,precheck=10000\,gaps=10485760 
```

Exit status: 139

```text
speedgun-ng 0.29.0 build=Release
method: fwq
workCount: 1
targetNs: 1000
calibrationSpanNs: 341243868
precheck cpu 8 (candidate)
gapCount: 0
gapsPerSecond: 0.00
minNs: 0
medianNs: 0
meanNs: 0.00
p90Ns: 0
p99Ns: 0
p999Ns: 0
p9999Ns: 0
maxNs: 0
totalNs: 0
totalPercent: 0.00
runtimeNs: 0
thresholdNs: 200
loopPeriodNs: 1
droppedGaps: 0
interrupts delta: 6277 (source: /proc/interrupts)
softirq HI delta: 0 (source: /proc/softirqs)
softirq TIMER delta: 73 (source: /proc/softirqs)
softirq NET_TX delta: 0 (source: /proc/softirqs)
softirq NET_RX delta: 0 (source: /proc/softirqs)
softirq BLOCK delta: 0 (source: /proc/softirqs)
softirq IRQ_POLL delta: 0 (source: /proc/softirqs)
softirq TASKLET delta: 0 (source: /proc/softirqs)
softirq SCHED delta: 100 (source: /proc/softirqs)
softirq HRTIMER delta: 0 (source: /proc/softirqs)
softirq RCU delta: 532 (source: /proc/softirqs)
switch delta: 0, schedule delta: 0 (source: /proc/schedstat)
host settings: isolcpus=not available nohz_full=not available rcu_nocbs=not available
governor: cpu 8 powersave (source: /sys devices system cpu scaling_governor)
irq affinity: cpu 8 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,27,28,29,30,31,32,33,34,35,36,37,39,40,42,43,52,61,69,77,87,88,95,117,137,138,139,155,157,158,159,160,161,162,163 (source: /proc/irq)
pinning candidates:
bmTouch                  iterations=14319074001  real time/iter (ns)=    0.891  cpu time/iter (ns)=    0.889  overhead floor (ns)=  120.000  endpoint samples=2
jitter probe: cpus 8
gapCount: 0
gapsPerSecond: 0.00
minNs: 0
medianNs: 0
meanNs: 0.00
p90Ns: 0
p99Ns: 0
p999Ns: 0
p9999Ns: 0
maxNs: 0
totalNs: 0
totalPercent: 0.00
runtimeNs: 0
thresholdNs: 200
loopPeriodNs: 1
droppedGaps: 0
```

The figures of this section:

- calibration count (`workCount`): 1
- measured unit time (`unitTimeNs`): absent ns
- noise fraction (`noisePercent`): absent
- `gaps=` value: 10485760
- kept span (`seriesRuntimeNs`): absent ms
- drop count (`droppedSamples`): absent
- detected periods (`periodNs`): rank 1 at absent ns, rank 2 at absent ns, rank 3 at absent ns, rank 4 at absent ns, rank 5 at absent ns

## FWQ on cpu 8 of L3 group 1, loaded

- probe cpu: 8
- L3 group: 1 over cpus 8-15,24-31
- load state: loaded
- gaps: 10485760
- load cpu: 24, the SMT sibling of probe cpu 8
- load command line: `taskset -c 24 sh -c 'while :; do :; done'`

```sh
stdbuf --output=0 /home/archerc/code/speedgun-ng/build/example/benchmark_example --filter=bmTouch --min-time=10 --repetitions=1 --jitter-probe=8\,method=fwq\,mode=precheck\,precheck=10000\,gaps=10485760 
```

Exit status: 139

```text
speedgun-ng 0.29.0 build=Release
method: fwq
workCount: 1
targetNs: 1000
calibrationSpanNs: 1458770586
precheck cpu 8 (candidate)
gapCount: 0
gapsPerSecond: 0.00
minNs: 0
medianNs: 0
meanNs: 0.00
p90Ns: 0
p99Ns: 0
p999Ns: 0
p9999Ns: 0
maxNs: 0
totalNs: 0
totalPercent: 0.00
runtimeNs: 0
thresholdNs: 420
loopPeriodNs: 42
droppedGaps: 0
interrupts delta: 7394 (source: /proc/interrupts)
softirq HI delta: 0 (source: /proc/softirqs)
softirq TIMER delta: 1 (source: /proc/softirqs)
softirq NET_TX delta: 0 (source: /proc/softirqs)
softirq NET_RX delta: 0 (source: /proc/softirqs)
softirq BLOCK delta: 0 (source: /proc/softirqs)
softirq IRQ_POLL delta: 0 (source: /proc/softirqs)
softirq TASKLET delta: 0 (source: /proc/softirqs)
softirq SCHED delta: 50 (source: /proc/softirqs)
softirq HRTIMER delta: 0 (source: /proc/softirqs)
softirq RCU delta: 648 (source: /proc/softirqs)
switch delta: 0, schedule delta: 0 (source: /proc/schedstat)
host settings: isolcpus=not available nohz_full=not available rcu_nocbs=not available
governor: cpu 8 powersave (source: /sys devices system cpu scaling_governor)
irq affinity: cpu 8 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,27,28,29,30,31,32,33,34,35,36,37,39,40,42,43,52,61,69,77,87,88,95,117,137,138,139,155,157,158,159,160,161,162,163 (source: /proc/irq)
pinning candidates:
bmTouch                  iterations=14423370379  real time/iter (ns)=    0.978  cpu time/iter (ns)=    0.975  overhead floor (ns)=  150.000  endpoint samples=2
jitter probe: cpus 8
gapCount: 0
gapsPerSecond: 0.00
minNs: 0
medianNs: 0
meanNs: 0.00
p90Ns: 0
p99Ns: 0
p999Ns: 0
p9999Ns: 0
maxNs: 0
totalNs: 0
totalPercent: 0.00
runtimeNs: 0
thresholdNs: 420
loopPeriodNs: 42
droppedGaps: 0
```

The figures of this section:

- calibration count (`workCount`): 1
- measured unit time (`unitTimeNs`): absent ns
- noise fraction (`noisePercent`): absent
- `gaps=` value: 10485760
- kept span (`seriesRuntimeNs`): absent ms
- drop count (`droppedSamples`): absent
- detected periods (`periodNs`): rank 1 at absent ns, rank 2 at absent ns, rank 3 at absent ns, rank 4 at absent ns, rank 5 at absent ns

## FTQ on cpu 0 of L3 group 0, idle

- probe cpu: 0
- L3 group: 0 over cpus 0-7,16-23
- load state: idle
- gaps: 131072
- load command line: none

```sh
stdbuf --output=0 /home/archerc/code/speedgun-ng/build/example/benchmark_example --filter=bmTouch --min-time=10 --repetitions=1 --jitter-probe=0\,method=ftq\,mode=precheck\,precheck=10000\,gaps=131072 
```

Exit status: 0

```text
speedgun-ng 0.29.0 build=Release
method: ftq
workCount: 1
targetNs: 100
calibrationSpanNs: 252
precheck cpu 0 (candidate)
gapCount: 0
gapsPerSecond: 0.00
minNs: 0
medianNs: 0
meanNs: 0.00
p90Ns: 0
p99Ns: 0
p999Ns: 0
p9999Ns: 0
maxNs: 0
totalNs: 0
totalPercent: 0.00
runtimeNs: 0
thresholdNs: 200
loopPeriodNs: 1
droppedGaps: 0
interrupts delta: 6364 (source: /proc/interrupts)
softirq HI delta: 0 (source: /proc/softirqs)
softirq TIMER delta: 21 (source: /proc/softirqs)
softirq NET_TX delta: 0 (source: /proc/softirqs)
softirq NET_RX delta: 0 (source: /proc/softirqs)
softirq BLOCK delta: 0 (source: /proc/softirqs)
softirq IRQ_POLL delta: 0 (source: /proc/softirqs)
softirq TASKLET delta: 0 (source: /proc/softirqs)
softirq SCHED delta: 41 (source: /proc/softirqs)
softirq HRTIMER delta: 0 (source: /proc/softirqs)
softirq RCU delta: 503 (source: /proc/softirqs)
switch delta: 0, schedule delta: 0 (source: /proc/schedstat)
host settings: isolcpus=not available nohz_full=not available rcu_nocbs=not available
governor: cpu 0 powersave (source: /sys devices system cpu scaling_governor)
irq affinity: cpu 0 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,27,28,29,30,31,32,33,34,35,36,37,39,40,42,43,52,61,69,77,87,88,104,135,137,138,139,155,157,158,159,160,161,162,163 (source: /proc/irq)
pinning candidates:
bmTouch                  iterations=14264158660  real time/iter (ns)=    0.975  cpu time/iter (ns)=    0.973  overhead floor (ns)=  130.000  endpoint samples=2
jitter probe: cpus 0
gapCount: 0
gapsPerSecond: 0.00
minNs: 0
medianNs: 0
meanNs: 0.00
p90Ns: 0
p99Ns: 0
p999Ns: 0
p9999Ns: 0
maxNs: 0
totalNs: 0
totalPercent: 0.00
runtimeNs: 0
thresholdNs: 200
loopPeriodNs: 1
droppedGaps: 0
sampleCount: 100000
minValue: 0
medianValue: 15
meanValue: 1017.48
p90Value: 2093
p99Value: 2104
p999Value: 2115
p9999Value: 2158
maxValue: 2167
noisePercent: 53.05
seriesRuntimeNs: 10000000000
droppedSamples: 0
unitTimeNs: 252
quantumNs: 100000
resampleStepNs: 0
spectrumPoints: 65536
period1Ns: 13540495
period2Ns: 13653333
period3Ns: 13624948
period4Ns: 13596680
period5Ns: 13568530
period1Power: 1.00
period2Power: 0.72
period3Power: 0.57
period4Power: 0.48
period5Power: 0.39
jitter: probe on cpus 0
jitter: loop period 1 ns, threshold 200 ns
jitter: cpu 0 kept 0 dropped 0
jitter: split and percentiles use the kept gaps only
```

The figures of this section:

- calibration count (`workCount`): 1
- measured unit time (`unitTimeNs`): 252 ns
- noise fraction (`noisePercent`): 53.05
- `gaps=` value: 131072
- kept span (`seriesRuntimeNs`): 10000.0 ms
- drop count (`droppedSamples`): 0
- detected periods (`periodNs`): rank 1 at 13540495 ns, rank 2 at 13653333 ns, rank 3 at 13624948 ns, rank 4 at 13596680 ns, rank 5 at 13568530 ns

## FTQ on cpu 0 of L3 group 0, loaded

- probe cpu: 0
- L3 group: 0 over cpus 0-7,16-23
- load state: loaded
- gaps: 131072
- load cpu: 16, the SMT sibling of probe cpu 0
- load command line: `taskset -c 16 sh -c 'while :; do :; done'`

```sh
stdbuf --output=0 /home/archerc/code/speedgun-ng/build/example/benchmark_example --filter=bmTouch --min-time=10 --repetitions=1 --jitter-probe=0\,method=ftq\,mode=precheck\,precheck=10000\,gaps=131072 
```

Exit status: 0

```text
speedgun-ng 0.29.0 build=Release
method: ftq
workCount: 1
targetNs: 100
calibrationSpanNs: 462
precheck cpu 0 (candidate)
gapCount: 0
gapsPerSecond: 0.00
minNs: 0
medianNs: 0
meanNs: 0.00
p90Ns: 0
p99Ns: 0
p999Ns: 0
p9999Ns: 0
maxNs: 0
totalNs: 0
totalPercent: 0.00
runtimeNs: 0
thresholdNs: 840
loopPeriodNs: 84
droppedGaps: 0
interrupts delta: 6277 (source: /proc/interrupts)
softirq HI delta: 0 (source: /proc/softirqs)
softirq TIMER delta: 35 (source: /proc/softirqs)
softirq NET_TX delta: 0 (source: /proc/softirqs)
softirq NET_RX delta: 0 (source: /proc/softirqs)
softirq BLOCK delta: 0 (source: /proc/softirqs)
softirq IRQ_POLL delta: 0 (source: /proc/softirqs)
softirq TASKLET delta: 0 (source: /proc/softirqs)
softirq SCHED delta: 71 (source: /proc/softirqs)
softirq HRTIMER delta: 0 (source: /proc/softirqs)
softirq RCU delta: 493 (source: /proc/softirqs)
switch delta: 0, schedule delta: 0 (source: /proc/schedstat)
host settings: isolcpus=not available nohz_full=not available rcu_nocbs=not available
governor: cpu 0 powersave (source: /sys devices system cpu scaling_governor)
irq affinity: cpu 0 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,27,28,29,30,31,32,33,34,35,36,37,39,40,42,43,52,61,69,77,87,88,104,135,137,138,139,155,157,158,159,160,161,162,163 (source: /proc/irq)
pinning candidates:
bmTouch                  iterations=13955096365  real time/iter (ns)=    0.779  cpu time/iter (ns)=    0.777  overhead floor (ns)=  170.000  endpoint samples=2
jitter probe: cpus 0
gapCount: 0
gapsPerSecond: 0.00
minNs: 0
medianNs: 0
meanNs: 0.00
p90Ns: 0
p99Ns: 0
p999Ns: 0
p9999Ns: 0
maxNs: 0
totalNs: 0
totalPercent: 0.00
runtimeNs: 0
thresholdNs: 840
loopPeriodNs: 84
droppedGaps: 0
sampleCount: 100066
minValue: 0
medianValue: 809
meanValue: 494.50
p90Value: 1003
p99Value: 1047
p999Value: 1059
p9999Value: 1064
maxValue: 1091
noisePercent: 54.67
seriesRuntimeNs: 10006600000
droppedSamples: 0
unitTimeNs: 462
quantumNs: 100000
resampleStepNs: 0
spectrumPoints: 65536
period1Ns: 13568530
period2Ns: 13540495
period3Ns: 13624948
period4Ns: 13457084
period5Ns: 13347454
period1Power: 1.00
period2Power: 0.77
period3Power: 0.55
period4Power: 0.32
period5Power: 0.30
jitter: probe on cpus 0
jitter: loop period 84 ns, threshold 840 ns
jitter: cpu 0 kept 0 dropped 0
jitter: split and percentiles use the kept gaps only
```

The figures of this section:

- calibration count (`workCount`): 1
- measured unit time (`unitTimeNs`): 462 ns
- noise fraction (`noisePercent`): 54.67
- `gaps=` value: 131072
- kept span (`seriesRuntimeNs`): 10006.6 ms
- drop count (`droppedSamples`): 0
- detected periods (`periodNs`): rank 1 at 13568530 ns, rank 2 at 13540495 ns, rank 3 at 13624948 ns, rank 4 at 13457084 ns, rank 5 at 13347454 ns

## FTQ on cpu 8 of L3 group 1, idle

- probe cpu: 8
- L3 group: 1 over cpus 8-15,24-31
- load state: idle
- gaps: 131072
- load command line: none

```sh
stdbuf --output=0 /home/archerc/code/speedgun-ng/build/example/benchmark_example --filter=bmTouch --min-time=10 --repetitions=1 --jitter-probe=8\,method=ftq\,mode=precheck\,precheck=10000\,gaps=131072 
```

Exit status: 0

```text
speedgun-ng 0.29.0 build=Release
method: ftq
workCount: 1
targetNs: 100
calibrationSpanNs: 462
precheck cpu 8 (candidate)
gapCount: 0
gapsPerSecond: 0.00
minNs: 0
medianNs: 0
meanNs: 0.00
p90Ns: 0
p99Ns: 0
p999Ns: 0
p9999Ns: 0
maxNs: 0
totalNs: 0
totalPercent: 0.00
runtimeNs: 0
thresholdNs: 200
loopPeriodNs: 1
droppedGaps: 0
interrupts delta: 6205 (source: /proc/interrupts)
softirq HI delta: 0 (source: /proc/softirqs)
softirq TIMER delta: 159 (source: /proc/softirqs)
softirq NET_TX delta: 0 (source: /proc/softirqs)
softirq NET_RX delta: 0 (source: /proc/softirqs)
softirq BLOCK delta: 0 (source: /proc/softirqs)
softirq IRQ_POLL delta: 0 (source: /proc/softirqs)
softirq TASKLET delta: 0 (source: /proc/softirqs)
softirq SCHED delta: 68 (source: /proc/softirqs)
softirq HRTIMER delta: 0 (source: /proc/softirqs)
softirq RCU delta: 2 (source: /proc/softirqs)
switch delta: 0, schedule delta: 0 (source: /proc/schedstat)
host settings: isolcpus=not available nohz_full=not available rcu_nocbs=not available
governor: cpu 8 powersave (source: /sys devices system cpu scaling_governor)
irq affinity: cpu 8 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,27,28,29,30,31,32,33,34,35,36,37,39,40,42,43,52,61,69,77,87,88,95,117,137,138,139,155,157,158,159,160,161,162,163 (source: /proc/irq)
pinning candidates:
bmTouch                  iterations=16070077442  real time/iter (ns)=    0.846  cpu time/iter (ns)=    0.843  overhead floor (ns)=  120.000  endpoint samples=2
jitter probe: cpus 8
gapCount: 0
gapsPerSecond: 0.00
minNs: 0
medianNs: 0
meanNs: 0.00
p90Ns: 0
p99Ns: 0
p999Ns: 0
p9999Ns: 0
maxNs: 0
totalNs: 0
totalPercent: 0.00
runtimeNs: 0
thresholdNs: 200
loopPeriodNs: 1
droppedGaps: 0
sampleCount: 100059
minValue: 0
medianValue: 2133
meanValue: 1364.82
p90Value: 2165
p99Value: 2171
p999Value: 2175
p9999Value: 2180
maxValue: 2182
noisePercent: 37.45
seriesRuntimeNs: 10005900000
droppedSamples: 0
unitTimeNs: 462
quantumNs: 100000
resampleStepNs: 0
spectrumPoints: 65536
period1Ns: 13540495
period2Ns: 13596680
period3Ns: 13512577
period4Ns: 13457084
period5Ns: 13653333
period1Power: 1.00
period2Power: 0.83
period3Power: 0.76
period4Power: 0.47
period5Power: 0.44
jitter: probe on cpus 8
jitter: loop period 1 ns, threshold 200 ns
jitter: cpu 8 kept 0 dropped 0
jitter: split and percentiles use the kept gaps only
```

The figures of this section:

- calibration count (`workCount`): 1
- measured unit time (`unitTimeNs`): 462 ns
- noise fraction (`noisePercent`): 37.45
- `gaps=` value: 131072
- kept span (`seriesRuntimeNs`): 10005.9 ms
- drop count (`droppedSamples`): 0
- detected periods (`periodNs`): rank 1 at 13540495 ns, rank 2 at 13596680 ns, rank 3 at 13512577 ns, rank 4 at 13457084 ns, rank 5 at 13653333 ns

## FTQ on cpu 8 of L3 group 1, loaded

- probe cpu: 8
- L3 group: 1 over cpus 8-15,24-31
- load state: loaded
- gaps: 131072
- load cpu: 24, the SMT sibling of probe cpu 8
- load command line: `taskset -c 24 sh -c 'while :; do :; done'`

```sh
stdbuf --output=0 /home/archerc/code/speedgun-ng/build/example/benchmark_example --filter=bmTouch --min-time=10 --repetitions=1 --jitter-probe=8\,method=ftq\,mode=precheck\,precheck=10000\,gaps=131072 
```

Exit status: 0

```text
speedgun-ng 0.29.0 build=Release
method: ftq
workCount: 1
targetNs: 100
calibrationSpanNs: 546
precheck cpu 8 (candidate)
gapCount: 0
gapsPerSecond: 0.00
minNs: 0
medianNs: 0
meanNs: 0.00
p90Ns: 0
p99Ns: 0
p999Ns: 0
p9999Ns: 0
maxNs: 0
totalNs: 0
totalPercent: 0.00
runtimeNs: 0
thresholdNs: 420
loopPeriodNs: 42
droppedGaps: 0
interrupts delta: 5453 (source: /proc/interrupts)
softirq HI delta: 0 (source: /proc/softirqs)
softirq TIMER delta: 170 (source: /proc/softirqs)
softirq NET_TX delta: 0 (source: /proc/softirqs)
softirq NET_RX delta: 0 (source: /proc/softirqs)
softirq BLOCK delta: 0 (source: /proc/softirqs)
softirq IRQ_POLL delta: 0 (source: /proc/softirqs)
softirq TASKLET delta: 0 (source: /proc/softirqs)
softirq SCHED delta: 299 (source: /proc/softirqs)
softirq HRTIMER delta: 0 (source: /proc/softirqs)
softirq RCU delta: 2 (source: /proc/softirqs)
switch delta: 0, schedule delta: 0 (source: /proc/schedstat)
host settings: isolcpus=not available nohz_full=not available rcu_nocbs=not available
governor: cpu 8 powersave (source: /sys devices system cpu scaling_governor)
irq affinity: cpu 8 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,27,28,29,30,31,32,33,34,35,36,37,39,40,42,43,52,61,69,77,87,88,95,117,137,138,139,155,157,158,159,160,161,162,163 (source: /proc/irq)
pinning candidates:
bmTouch                  iterations=21117552486  real time/iter (ns)=    0.711  cpu time/iter (ns)=    0.710  overhead floor (ns)=  100.000  endpoint samples=2
jitter probe: cpus 8
gapCount: 0
gapsPerSecond: 0.00
minNs: 0
medianNs: 0
meanNs: 0.00
p90Ns: 0
p99Ns: 0
p999Ns: 0
p9999Ns: 0
maxNs: 0
totalNs: 0
totalPercent: 0.00
runtimeNs: 0
thresholdNs: 420
loopPeriodNs: 42
droppedGaps: 0
sampleCount: 100000
minValue: 0
medianValue: 1138
meanValue: 799.25
p90Value: 1182
p99Value: 1194
p999Value: 1197
p9999Value: 1200
maxValue: 1236
noisePercent: 35.34
seriesRuntimeNs: 10000000000
droppedSamples: 0
unitTimeNs: 546
quantumNs: 100000
resampleStepNs: 0
spectrumPoints: 65536
period1Ns: 6553600000
period2Ns: 3276800000
period3Ns: 19621556
period4Ns: 19739759
period5Ns: 20289783
period1Power: 1.00
period2Power: 0.56
period3Power: 0.11
period4Power: 0.10
period5Power: 0.10
jitter: probe on cpus 8
jitter: loop period 42 ns, threshold 420 ns
jitter: cpu 8 kept 0 dropped 0
jitter: split and percentiles use the kept gaps only
```

The figures of this section:

- calibration count (`workCount`): 1
- measured unit time (`unitTimeNs`): 546 ns
- noise fraction (`noisePercent`): 35.34
- `gaps=` value: 131072
- kept span (`seriesRuntimeNs`): 10000.0 ms
- drop count (`droppedSamples`): 0
- detected periods (`periodNs`): rank 1 at 6553600000 ns, rank 2 at 3276800000 ns, rank 3 at 19621556 ns, rank 4 at 19739759 ns, rank 5 at 20289783 ns

### Result

The eight sections above carry their command lines, exit statuses and
captured output. The figures block of each section names the calibration
count, the measured unit time, the noise fraction, the `gaps=` value,
the kept span in milliseconds, the drop count and the detected periods
(FR-027, FR-028, SC-008).
