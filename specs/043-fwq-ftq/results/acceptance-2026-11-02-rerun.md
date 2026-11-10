# acceptance run: fwq and ftq probes (specs/043-fwq-ftq)

Run id: 20261102T181349Z
Script: tools/noise/fwq_ftq_acceptance.sh (FR-027, FR-028, SC-008)
Head: d18fb18d, the head after T071 (14983d77) and T072 (d18fb18d) land

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
- Session load: the run added no build, no test suite, and no coverage
  capture
- Standing host load: a model inference server (`strata`) held about 5
  cores at the start and stood idle by the end
- Load average: 8.83 8.64 9.15 at the start (12:13:49-06:00), 2.22
  5.87 8.00 at the end (12:17:28-06:00), 1, 5 and 15 minute values from
  /proc/loadavg

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
stdbuf --output=0 $HOME/code/speedgun-ng/build/example/benchmark_example --filter=bmTouch --min-time=10 --repetitions=1 --jitter-probe=0\,method=fwq\,mode=precheck\,precheck=10000\,gaps=10485760 
```

Exit status: 0

```text
speedgun-ng 0.29.0 build=Release
method: fwq
workCount: 1136
targetNs: 1000
calibrationSpanNs: 16968
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
interrupts delta: 6718 (source: /proc/interrupts)
softirq HI delta: 0 (source: /proc/softirqs)
softirq TIMER delta: 238 (source: /proc/softirqs)
softirq NET_TX delta: 0 (source: /proc/softirqs)
softirq NET_RX delta: 0 (source: /proc/softirqs)
softirq BLOCK delta: 0 (source: /proc/softirqs)
softirq IRQ_POLL delta: 0 (source: /proc/softirqs)
softirq TASKLET delta: 0 (source: /proc/softirqs)
softirq SCHED delta: 40 (source: /proc/softirqs)
softirq HRTIMER delta: 0 (source: /proc/softirqs)
softirq RCU delta: 319 (source: /proc/softirqs)
switch delta: 0, schedule delta: 0 (source: /proc/schedstat)
host settings: isolcpus=not available nohz_full=not available rcu_nocbs=not available
governor: cpu 0 powersave (source: /sys devices system cpu scaling_governor)
irq affinity: cpu 0 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,27,28,29,30,31,32,33,34,35,36,37,39,40,42,43,52,61,69,77,87,88,104,135,137,138,139,155,157,158,159,160,161,162,163 (source: /proc/irq)
pinning candidates:
bmTouch                  iterations=26771053322  real time/iter (ns)=    0.632  cpu time/iter (ns)=    0.630  overhead floor (ns)=  120.000  endpoint samples=2
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
sampleCount: 4942366
minValue: 966
medianValue: 1008
meanValue: 2023.80
p90Value: 1008
p99Value: 1092
p999Value: 2016
p9999Value: 6745830
maxValue: 14317968
noisePercent: 52.27
seriesRuntimeNs: 10002384462
droppedSamples: 0
unitTimeNs: 1008
quantumNs: 0
resampleStepNs: 1008
spectrumPoints: 65536
period1Ns: 3002740
period2Ns: 1611226
period3Ns: 1501370
period4Ns: 0
period5Ns: 0
period1Power: 1.00
period2Power: 0.99
period3Power: 0.94
period4Power: 0.00
period5Power: 0.00
jitter: probe on cpus 0
jitter: loop period 1 ns, threshold 200 ns
jitter: cpu 0 kept 0 dropped 0
jitter: split and percentiles use the kept gaps only
```

The figures of this section:

- calibration count (`workCount`): 1136
- measured unit time (`unitTimeNs`): 1008 ns
- noise fraction (`noisePercent`): 52.27
- `gaps=` value: 10485760
- kept span (`seriesRuntimeNs`): 10002.4 ms
- drop count (`droppedSamples`): 0
- detected periods (`periodNs`): rank 1 at 3002740 ns, rank 2 at 1611226 ns, rank 3 at 1501370 ns, rank 4 at 0 ns, rank 5 at 0 ns

## FWQ on cpu 0 of L3 group 0, loaded

- probe cpu: 0
- L3 group: 0 over cpus 0-7,16-23
- load state: loaded
- gaps: 10485760
- load cpu: 16, the SMT sibling of probe cpu 0
- load command line: `taskset -c 16 sh -c 'while :; do :; done'`

```sh
stdbuf --output=0 $HOME/code/speedgun-ng/build/example/benchmark_example --filter=bmTouch --min-time=10 --repetitions=1 --jitter-probe=0\,method=fwq\,mode=precheck\,precheck=10000\,gaps=10485760 
```

Exit status: 0

```text
speedgun-ng 0.29.0 build=Release
method: fwq
workCount: 640
targetNs: 1000
calibrationSpanNs: 16506
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
interrupts delta: 16858 (source: /proc/interrupts)
softirq HI delta: 0 (source: /proc/softirqs)
softirq TIMER delta: 133 (source: /proc/softirqs)
softirq NET_TX delta: 0 (source: /proc/softirqs)
softirq NET_RX delta: 0 (source: /proc/softirqs)
softirq BLOCK delta: 0 (source: /proc/softirqs)
softirq IRQ_POLL delta: 0 (source: /proc/softirqs)
softirq TASKLET delta: 0 (source: /proc/softirqs)
softirq SCHED delta: 41 (source: /proc/softirqs)
softirq HRTIMER delta: 0 (source: /proc/softirqs)
softirq RCU delta: 181 (source: /proc/softirqs)
switch delta: 0, schedule delta: 0 (source: /proc/schedstat)
host settings: isolcpus=not available nohz_full=not available rcu_nocbs=not available
governor: cpu 0 powersave (source: /sys devices system cpu scaling_governor)
irq affinity: cpu 0 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,27,28,29,30,31,32,33,34,35,36,37,39,40,42,43,52,61,69,77,87,88,104,135,137,138,139,155,157,158,159,160,161,162,163 (source: /proc/irq)
pinning candidates:
bmTouch                  iterations=35318351310  real time/iter (ns)=    0.396  cpu time/iter (ns)=    0.396  overhead floor (ns)=  110.000  endpoint samples=2
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
sampleCount: 3808252
minValue: 882
medianValue: 1092
meanValue: 2625.95
p90Value: 1092
p99Value: 1134
p999Value: 16548
p9999Value: 6787788
maxValue: 27426924
noisePercent: 66.41
seriesRuntimeNs: 10000297566
droppedSamples: 0
unitTimeNs: 1008
quantumNs: 0
resampleStepNs: 1092
spectrumPoints: 65536
period1Ns: 3407872
period2Ns: 1703936
period3Ns: 1135957
period4Ns: 851968
period5Ns: 681574
period1Power: 1.00
period2Power: 0.99
period3Power: 0.99
period4Power: 0.97
period5Power: 0.96
jitter: probe on cpus 0
jitter: loop period 84 ns, threshold 840 ns
jitter: cpu 0 kept 0 dropped 0
jitter: split and percentiles use the kept gaps only
```

The figures of this section:

- calibration count (`workCount`): 640
- measured unit time (`unitTimeNs`): 1008 ns
- noise fraction (`noisePercent`): 66.41
- `gaps=` value: 10485760
- kept span (`seriesRuntimeNs`): 10000.3 ms
- drop count (`droppedSamples`): 0
- detected periods (`periodNs`): rank 1 at 3407872 ns, rank 2 at 1703936 ns, rank 3 at 1135957 ns, rank 4 at 851968 ns, rank 5 at 681574 ns

## FWQ on cpu 8 of L3 group 1, idle

- probe cpu: 8
- L3 group: 1 over cpus 8-15,24-31
- load state: idle
- gaps: 10485760
- load command line: none

```sh
stdbuf --output=0 $HOME/code/speedgun-ng/build/example/benchmark_example --filter=bmTouch --min-time=10 --repetitions=1 --jitter-probe=8\,method=fwq\,mode=precheck\,precheck=10000\,gaps=10485760 
```

Exit status: 0

```text
speedgun-ng 0.29.0 build=Release
method: fwq
workCount: 1136
targetNs: 1000
calibrationSpanNs: 16842
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
interrupts delta: 4981 (source: /proc/interrupts)
softirq HI delta: 0 (source: /proc/softirqs)
softirq TIMER delta: 79 (source: /proc/softirqs)
softirq NET_TX delta: 0 (source: /proc/softirqs)
softirq NET_RX delta: 0 (source: /proc/softirqs)
softirq BLOCK delta: 0 (source: /proc/softirqs)
softirq IRQ_POLL delta: 0 (source: /proc/softirqs)
softirq TASKLET delta: 0 (source: /proc/softirqs)
softirq SCHED delta: 581 (source: /proc/softirqs)
softirq HRTIMER delta: 0 (source: /proc/softirqs)
softirq RCU delta: 395 (source: /proc/softirqs)
switch delta: 0, schedule delta: 0 (source: /proc/schedstat)
host settings: isolcpus=not available nohz_full=not available rcu_nocbs=not available
governor: cpu 8 powersave (source: /sys devices system cpu scaling_governor)
irq affinity: cpu 8 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,27,28,29,30,31,32,33,34,35,36,37,39,40,42,43,52,61,69,77,87,88,95,117,137,138,139,155,157,158,159,160,161,162,163 (source: /proc/irq)
pinning candidates:
bmTouch                  iterations=28326955132  real time/iter (ns)=    0.361  cpu time/iter (ns)=    0.361  overhead floor (ns)=  120.000  endpoint samples=2
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
sampleCount: 9581444
minValue: 966
medianValue: 1008
meanValue: 1043.68
p90Value: 1008
p99Value: 1008
p999Value: 1974
p9999Value: 7308
maxValue: 10077690
noisePercent: 7.44
seriesRuntimeNs: 10000000415
droppedSamples: 0
unitTimeNs: 1008
quantumNs: 0
resampleStepNs: 1008
spectrumPoints: 65536
period1Ns: 66060288
period2Ns: 33030144
period3Ns: 22020096
period4Ns: 16515072
period5Ns: 13212057
period1Power: 1.00
period2Power: 0.97
period3Power: 0.49
period4Power: 0.26
period5Power: 0.17
jitter: probe on cpus 8
jitter: loop period 1 ns, threshold 200 ns
jitter: cpu 8 kept 0 dropped 0
jitter: split and percentiles use the kept gaps only
```

The figures of this section:

- calibration count (`workCount`): 1136
- measured unit time (`unitTimeNs`): 1008 ns
- noise fraction (`noisePercent`): 7.44
- `gaps=` value: 10485760
- kept span (`seriesRuntimeNs`): 10000.0 ms
- drop count (`droppedSamples`): 0
- detected periods (`periodNs`): rank 1 at 66060288 ns, rank 2 at 33030144 ns, rank 3 at 22020096 ns, rank 4 at 16515072 ns, rank 5 at 13212057 ns

## FWQ on cpu 8 of L3 group 1, loaded

- probe cpu: 8
- L3 group: 1 over cpus 8-15,24-31
- load state: loaded
- gaps: 10485760
- load cpu: 24, the SMT sibling of probe cpu 8
- load command line: `taskset -c 24 sh -c 'while :; do :; done'`

```sh
stdbuf --output=0 $HOME/code/speedgun-ng/build/example/benchmark_example --filter=bmTouch --min-time=10 --repetitions=1 --jitter-probe=8\,method=fwq\,mode=precheck\,precheck=10000\,gaps=10485760 
```

Exit status: 0

```text
speedgun-ng 0.29.0 build=Release
method: fwq
workCount: 640
targetNs: 1000
calibrationSpanNs: 16086
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
interrupts delta: 4746 (source: /proc/interrupts)
softirq HI delta: 0 (source: /proc/softirqs)
softirq TIMER delta: 58 (source: /proc/softirqs)
softirq NET_TX delta: 0 (source: /proc/softirqs)
softirq NET_RX delta: 0 (source: /proc/softirqs)
softirq BLOCK delta: 0 (source: /proc/softirqs)
softirq IRQ_POLL delta: 0 (source: /proc/softirqs)
softirq TASKLET delta: 0 (source: /proc/softirqs)
softirq SCHED delta: 648 (source: /proc/softirqs)
softirq HRTIMER delta: 0 (source: /proc/softirqs)
softirq RCU delta: 78 (source: /proc/softirqs)
switch delta: 0, schedule delta: 0 (source: /proc/schedstat)
host settings: isolcpus=not available nohz_full=not available rcu_nocbs=not available
governor: cpu 8 powersave (source: /sys devices system cpu scaling_governor)
irq affinity: cpu 8 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,27,28,29,30,31,32,33,34,35,36,37,39,40,42,43,52,61,69,77,87,88,95,117,137,138,139,155,157,158,159,160,161,162,163 (source: /proc/irq)
pinning candidates:
bmTouch                  iterations=37751911722  real time/iter (ns)=    0.371  cpu time/iter (ns)=    0.371  overhead floor (ns)=  100.000  endpoint samples=2
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
sampleCount: 9825453
minValue: 798
medianValue: 1008
meanValue: 1017.76
p90Value: 1050
p99Value: 1050
p999Value: 2016
p9999Value: 8064
maxValue: 100884
noisePercent: 21.59
seriesRuntimeNs: 10000000458
droppedSamples: 0
unitTimeNs: 1008
quantumNs: 0
resampleStepNs: 1008
spectrumPoints: 65536
period1Ns: 600548
period2Ns: 4128768
period3Ns: 381851
period4Ns: 1048576
period5Ns: 702769
period1Power: 1.00
period2Power: 0.81
period3Power: 0.74
period4Power: 0.65
period5Power: 0.60
jitter: probe on cpus 8
jitter: loop period 42 ns, threshold 420 ns
jitter: cpu 8 kept 0 dropped 0
jitter: split and percentiles use the kept gaps only
```

The figures of this section:

- calibration count (`workCount`): 640
- measured unit time (`unitTimeNs`): 1008 ns
- noise fraction (`noisePercent`): 21.59
- `gaps=` value: 10485760
- kept span (`seriesRuntimeNs`): 10000.0 ms
- drop count (`droppedSamples`): 0
- detected periods (`periodNs`): rank 1 at 600548 ns, rank 2 at 4128768 ns, rank 3 at 381851 ns, rank 4 at 1048576 ns, rank 5 at 702769 ns

## FTQ on cpu 0 of L3 group 0, idle

- probe cpu: 0
- L3 group: 0 over cpus 0-7,16-23
- load state: idle
- gaps: 131072
- load command line: none

```sh
stdbuf --output=0 $HOME/code/speedgun-ng/build/example/benchmark_example --filter=bmTouch --min-time=10 --repetitions=1 --jitter-probe=0\,method=ftq\,mode=precheck\,precheck=10000\,gaps=131072 
```

Exit status: 0

```text
speedgun-ng 0.29.0 build=Release
method: ftq
workCount: 1
targetNs: 100
calibrationSpanNs: 294
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
interrupts delta: 4802 (source: /proc/interrupts)
softirq HI delta: 0 (source: /proc/softirqs)
softirq TIMER delta: 161 (source: /proc/softirqs)
softirq NET_TX delta: 0 (source: /proc/softirqs)
softirq NET_RX delta: 0 (source: /proc/softirqs)
softirq BLOCK delta: 0 (source: /proc/softirqs)
softirq IRQ_POLL delta: 0 (source: /proc/softirqs)
softirq TASKLET delta: 0 (source: /proc/softirqs)
softirq SCHED delta: 430 (source: /proc/softirqs)
softirq HRTIMER delta: 0 (source: /proc/softirqs)
softirq RCU delta: 253 (source: /proc/softirqs)
switch delta: 0, schedule delta: 0 (source: /proc/schedstat)
host settings: isolcpus=not available nohz_full=not available rcu_nocbs=not available
governor: cpu 0 powersave (source: /sys devices system cpu scaling_governor)
irq affinity: cpu 0 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,27,28,29,30,31,32,33,34,35,36,37,39,40,42,43,52,61,69,77,87,88,104,135,137,138,139,155,157,158,159,160,161,162,163 (source: /proc/irq)
pinning candidates:
bmTouch                  iterations=39085986621  real time/iter (ns)=    0.357  cpu time/iter (ns)=    0.357  overhead floor (ns)=  100.000  endpoint samples=2
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
medianValue: 1976
meanValue: 2017.43
p90Value: 2087
p99Value: 2096
p999Value: 2129
p9999Value: 2135
maxValue: 2139
noisePercent: 5.68
seriesRuntimeNs: 10000000000
droppedSamples: 0
unitTimeNs: 294
quantumNs: 100000
resampleStepNs: 0
spectrumPoints: 65536
period1Ns: 1400042
period2Ns: 4201025
period3Ns: 2099839
period4Ns: 78959036
period5Ns: 1050088
period1Power: 1.00
period2Power: 0.96
period3Power: 0.86
period4Power: 0.50
period5Power: 0.49
jitter: probe on cpus 0
jitter: loop period 1 ns, threshold 200 ns
jitter: cpu 0 kept 0 dropped 0
jitter: split and percentiles use the kept gaps only
```

The figures of this section:

- calibration count (`workCount`): 1
- measured unit time (`unitTimeNs`): 294 ns
- noise fraction (`noisePercent`): 5.68
- `gaps=` value: 131072
- kept span (`seriesRuntimeNs`): 10000.0 ms
- drop count (`droppedSamples`): 0
- detected periods (`periodNs`): rank 1 at 1400042 ns, rank 2 at 4201025 ns, rank 3 at 2099839 ns, rank 4 at 78959036 ns, rank 5 at 1050088 ns

## FTQ on cpu 0 of L3 group 0, loaded

- probe cpu: 0
- L3 group: 0 over cpus 0-7,16-23
- load state: loaded
- gaps: 131072
- load cpu: 16, the SMT sibling of probe cpu 0
- load command line: `taskset -c 16 sh -c 'while :; do :; done'`

```sh
stdbuf --output=0 $HOME/code/speedgun-ng/build/example/benchmark_example --filter=bmTouch --min-time=10 --repetitions=1 --jitter-probe=0\,method=ftq\,mode=precheck\,precheck=10000\,gaps=131072 
```

Exit status: 0

```text
speedgun-ng 0.29.0 build=Release
method: ftq
workCount: 1
targetNs: 100
calibrationSpanNs: 294
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
interrupts delta: 4778 (source: /proc/interrupts)
softirq HI delta: 0 (source: /proc/softirqs)
softirq TIMER delta: 86 (source: /proc/softirqs)
softirq NET_TX delta: 0 (source: /proc/softirqs)
softirq NET_RX delta: 0 (source: /proc/softirqs)
softirq BLOCK delta: 0 (source: /proc/softirqs)
softirq IRQ_POLL delta: 0 (source: /proc/softirqs)
softirq TASKLET delta: 0 (source: /proc/softirqs)
softirq SCHED delta: 387 (source: /proc/softirqs)
softirq HRTIMER delta: 0 (source: /proc/softirqs)
softirq RCU delta: 79 (source: /proc/softirqs)
switch delta: 0, schedule delta: 0 (source: /proc/schedstat)
host settings: isolcpus=not available nohz_full=not available rcu_nocbs=not available
governor: cpu 0 powersave (source: /sys devices system cpu scaling_governor)
irq affinity: cpu 0 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,27,28,29,30,31,32,33,34,35,36,37,39,40,42,43,52,61,69,77,87,88,104,135,137,138,139,155,157,158,159,160,161,162,163 (source: /proc/irq)
pinning candidates:
bmTouch                  iterations=35774037209  real time/iter (ns)=    0.393  cpu time/iter (ns)=    0.393  overhead floor (ns)=  110.000  endpoint samples=2
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
sampleCount: 100000
minValue: 0
medianValue: 1013
meanValue: 1002.98
p90Value: 1056
p99Value: 1064
p999Value: 1068
p9999Value: 1077
maxValue: 1079
noisePercent: 7.05
seriesRuntimeNs: 10000000000
droppedSamples: 0
unitTimeNs: 294
quantumNs: 100000
resampleStepNs: 0
spectrumPoints: 65536
period1Ns: 4201025
period2Ns: 525002
period3Ns: 466680
period4Ns: 2099839
period5Ns: 700021
period1Power: 1.00
period2Power: 0.94
period3Power: 0.70
period4Power: 0.69
period5Power: 0.55
jitter: probe on cpus 0
jitter: loop period 42 ns, threshold 420 ns
jitter: cpu 0 kept 0 dropped 0
jitter: split and percentiles use the kept gaps only
```

The figures of this section:

- calibration count (`workCount`): 1
- measured unit time (`unitTimeNs`): 294 ns
- noise fraction (`noisePercent`): 7.05
- `gaps=` value: 131072
- kept span (`seriesRuntimeNs`): 10000.0 ms
- drop count (`droppedSamples`): 0
- detected periods (`periodNs`): rank 1 at 4201025 ns, rank 2 at 525002 ns, rank 3 at 466680 ns, rank 4 at 2099839 ns, rank 5 at 700021 ns

## FTQ on cpu 8 of L3 group 1, idle

- probe cpu: 8
- L3 group: 1 over cpus 8-15,24-31
- load state: idle
- gaps: 131072
- load command line: none

```sh
stdbuf --output=0 $HOME/code/speedgun-ng/build/example/benchmark_example --filter=bmTouch --min-time=10 --repetitions=1 --jitter-probe=8\,method=ftq\,mode=precheck\,precheck=10000\,gaps=131072 
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
thresholdNs: 420
loopPeriodNs: 42
droppedGaps: 0
interrupts delta: 4809 (source: /proc/interrupts)
softirq HI delta: 0 (source: /proc/softirqs)
softirq TIMER delta: 115 (source: /proc/softirqs)
softirq NET_TX delta: 0 (source: /proc/softirqs)
softirq NET_RX delta: 0 (source: /proc/softirqs)
softirq BLOCK delta: 0 (source: /proc/softirqs)
softirq IRQ_POLL delta: 0 (source: /proc/softirqs)
softirq TASKLET delta: 0 (source: /proc/softirqs)
softirq SCHED delta: 677 (source: /proc/softirqs)
softirq HRTIMER delta: 0 (source: /proc/softirqs)
softirq RCU delta: 86 (source: /proc/softirqs)
switch delta: 0, schedule delta: 0 (source: /proc/schedstat)
host settings: isolcpus=not available nohz_full=not available rcu_nocbs=not available
governor: cpu 8 powersave (source: /sys devices system cpu scaling_governor)
irq affinity: cpu 8 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,27,28,29,30,31,32,33,34,35,36,37,39,40,42,43,52,61,69,77,87,88,95,117,137,138,139,155,157,158,159,160,161,162,163 (source: /proc/irq)
pinning candidates:
bmTouch                  iterations=38758833129  real time/iter (ns)=    0.359  cpu time/iter (ns)=    0.358  overhead floor (ns)=  100.000  endpoint samples=2
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
minValue: 542
medianValue: 2257
meanValue: 2243.74
p90Value: 2270
p99Value: 2276
p999Value: 2278
p9999Value: 2280
maxValue: 2281
noisePercent: 1.63
seriesRuntimeNs: 10000000000
droppedSamples: 0
unitTimeNs: 462
quantumNs: 100000
resampleStepNs: 0
spectrumPoints: 65536
period1Ns: 1400042
period2Ns: 525002
period3Ns: 3276800000
period4Ns: 1050088
period5Ns: 381822
period1Power: 1.00
period2Power: 0.96
period3Power: 0.82
period4Power: 0.75
period5Power: 0.74
jitter: probe on cpus 8
jitter: loop period 42 ns, threshold 420 ns
jitter: cpu 8 kept 0 dropped 0
jitter: split and percentiles use the kept gaps only
```

The figures of this section:

- calibration count (`workCount`): 1
- measured unit time (`unitTimeNs`): 462 ns
- noise fraction (`noisePercent`): 1.63
- `gaps=` value: 131072
- kept span (`seriesRuntimeNs`): 10000.0 ms
- drop count (`droppedSamples`): 0
- detected periods (`periodNs`): rank 1 at 1400042 ns, rank 2 at 525002 ns, rank 3 at 3276800000 ns, rank 4 at 1050088 ns, rank 5 at 381822 ns

## FTQ on cpu 8 of L3 group 1, loaded

- probe cpu: 8
- L3 group: 1 over cpus 8-15,24-31
- load state: loaded
- gaps: 131072
- load cpu: 24, the SMT sibling of probe cpu 8
- load command line: `taskset -c 24 sh -c 'while :; do :; done'`

```sh
stdbuf --output=0 $HOME/code/speedgun-ng/build/example/benchmark_example --filter=bmTouch --min-time=10 --repetitions=1 --jitter-probe=8\,method=ftq\,mode=precheck\,precheck=10000\,gaps=131072 
```

Exit status: 0

```text
speedgun-ng 0.29.0 build=Release
method: ftq
workCount: 1
targetNs: 100
calibrationSpanNs: 504
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
interrupts delta: 4736 (source: /proc/interrupts)
softirq HI delta: 0 (source: /proc/softirqs)
softirq TIMER delta: 104 (source: /proc/softirqs)
softirq NET_TX delta: 0 (source: /proc/softirqs)
softirq NET_RX delta: 0 (source: /proc/softirqs)
softirq BLOCK delta: 0 (source: /proc/softirqs)
softirq IRQ_POLL delta: 0 (source: /proc/softirqs)
softirq TASKLET delta: 0 (source: /proc/softirqs)
softirq SCHED delta: 667 (source: /proc/softirqs)
softirq HRTIMER delta: 0 (source: /proc/softirqs)
softirq RCU delta: 84 (source: /proc/softirqs)
switch delta: 0, schedule delta: 0 (source: /proc/schedstat)
host settings: isolcpus=not available nohz_full=not available rcu_nocbs=not available
governor: cpu 8 powersave (source: /sys devices system cpu scaling_governor)
irq affinity: cpu 8 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,27,28,29,30,31,32,33,34,35,36,37,39,40,42,43,52,61,69,77,87,88,95,117,137,138,139,155,157,158,159,160,161,162,163 (source: /proc/irq)
pinning candidates:
bmTouch                  iterations=37797785129  real time/iter (ns)=    0.371  cpu time/iter (ns)=    0.371  overhead floor (ns)=  100.000  endpoint samples=2
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
minValue: 492
medianValue: 1196
meanValue: 1190.18
p90Value: 1199
p99Value: 1201
p999Value: 1203
p9999Value: 1231
maxValue: 1301
noisePercent: 8.52
seriesRuntimeNs: 10000000000
droppedSamples: 0
unitTimeNs: 504
quantumNs: 100000
resampleStepNs: 0
spectrumPoints: 65536
period1Ns: 2099839
period2Ns: 466680
period3Ns: 839989
period4Ns: 1050088
period5Ns: 381822
period1Power: 1.00
period2Power: 0.68
period3Power: 0.59
period4Power: 0.51
period5Power: 0.51
jitter: probe on cpus 8
jitter: loop period 42 ns, threshold 420 ns
jitter: cpu 8 kept 0 dropped 0
jitter: split and percentiles use the kept gaps only
```

The figures of this section:

- calibration count (`workCount`): 1
- measured unit time (`unitTimeNs`): 504 ns
- noise fraction (`noisePercent`): 8.52
- `gaps=` value: 131072
- kept span (`seriesRuntimeNs`): 10000.0 ms
- drop count (`droppedSamples`): 0
- detected periods (`periodNs`): rank 1 at 2099839 ns, rank 2 at 466680 ns, rank 3 at 839989 ns, rank 4 at 1050088 ns, rank 5 at 381822 ns

### Result

The eight sections above carry their command lines, exit statuses and
captured output. The figures block of each section names the calibration
count, the measured unit time, the noise fraction, the `gaps=` value,
the kept span in milliseconds, the drop count and the detected periods
(FR-027, FR-028, SC-008).
