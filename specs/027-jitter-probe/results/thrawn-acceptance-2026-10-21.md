# thrawn acceptance run: jitter probe (specs/027-jitter-probe)

Run id: 20261022T025849Z
Script: tools/noise/jitter_probe_thrawn.sh (FR-042, FR-043, SC-008)

## Host state

- Date: 2026-10-21
- Kernel: Linux 7.2.9-2-cachyos x86_64
- SMT state: 1
- CPU frequency governor: powersave
- Huge pages: [always] madvise never
- Kernel command line: `quiet nowatchdog splash rw rootflags=subvol=/@ root=UUID=e17463dd-6b0a-4388-969e-e3eacec6d983`
- isolated cpus: ``
- nohz_full cpus: ``

The visible BIOS-side settings stand in the mitigation records the kernel exposes:

- gather_data_sampling: Not affected
- ghostwrite: Not affected
- indirect_target_selection: Not affected
- itlb_multihit: Not affected
- l1tf: Not affected
- mds: Not affected
- meltdown: Not affected
- mmio_stale_data: Not affected
- old_microcode: Not affected
- reg_file_data_sampling: Not affected
- retbleed: Not affected
- spec_rstack_overflow: Mitigation: Reduced Speculation
- spec_store_bypass: Mitigation: Speculative Store Bypass disabled via prctl
- spectre_v1: Mitigation: usercopy/swapgs barriers and __user pointer sanitization
- spectre_v2: Mitigation: Enhanced / Automatic IBRS; IBPB: conditional; STIBP: always-on; PBRSB-eIBRS: Not affected; BHI: Not affected
- srbds: Not affected
- tsa: Not affected
- tsx_async_abort: Not affected
- vmscape: Mitigation: IBPB before exit to userspace

The machine: AMD Ryzen Threadripper 9960X, 24 cores, 48 threads, four
L3 groups. The isolated and nohz_full lists stand empty: the kernel
command line carries no isolcpus and no nohz_full setting, so the
probe shares the machine with the ordinary host load (FR-043).

## The N4 preset check

Each N4 preset runs on `docs_threads`, whose `bmSharded` registers
four contexts. The report discloses the planned cpu and the L3 group
of each context (FR-042). The `--threads` dimension of FR-042 stands
unrunnable on this tree: the long option table of
`source/harness/cli.cpp` (the `getopt_long` call site) carries no
`threads` entry, `contracts/cli-options.md` declares none, and no
requirement of the spec adds one. The runs below cover the presets at
the context counts the registered benchmarks carry, and the `smt`
preset run shows the standing refusal when a plan cannot place every
requested context.

## N4 preset none

```sh
/home/archerc/code/speedgun-ng/build/example/docs_threads --placement=none --filter=bmSharded 
```

Exit status: 0

```text
speedgun-ng 0.14.0 build=Release
bmSharded/threads:4      iterations=2937971336  real time/iter (ns)=    0.244  cpu time/iter (ns)=    0.243  overhead floor (ns)=  110.000  endpoint samples=8
  contexts=4  ctx0=SPIN/NO_PIN  ctx1=SPIN/NO_PIN  ctx2=SPIN/NO_PIN  ctx3=SPIN/NO_PIN
```

## N4 preset core

```sh
/home/archerc/code/speedgun-ng/build/example/docs_threads --placement=core --filter=bmSharded 
```

Exit status: 0

```text
speedgun-ng 0.14.0 build=Release
topology: packages=1 l3-groups=4 cores=24 threads=48
bmSharded/threads:4      iterations=1667744256  real time/iter (ns)=    0.376  cpu time/iter (ns)=    0.375  overhead floor (ns)=  100.000  endpoint samples=8
  contexts=4  ctx0=SPIN/0@0/applied  ctx1=SPIN/1@0/applied  ctx2=SPIN/2@0/applied  ctx3=SPIN/3@0/applied
```

## N4 preset l3

```sh
/home/archerc/code/speedgun-ng/build/example/docs_threads --placement=l3 --filter=bmSharded 
```

Exit status: 0

```text
speedgun-ng 0.14.0 build=Release
topology: packages=1 l3-groups=4 cores=24 threads=48
bmSharded/threads:4      iterations=2381004436  real time/iter (ns)=    0.317  cpu time/iter (ns)=    0.316  overhead floor (ns)=  100.000  endpoint samples=8
  contexts=4  ctx0=SPIN/0@0/applied  ctx1=SPIN/6@1/applied  ctx2=SPIN/12@2/applied  ctx3=SPIN/18@3/applied
```

## N4 preset smt

```sh
/home/archerc/code/speedgun-ng/build/example/docs_threads --placement=smt --filter=bmSharded 
```

Exit status: 2

```text
--placement: 'smt' places 2 of the 4 requested contexts
```

## N4 preset compact

```sh
/home/archerc/code/speedgun-ng/build/example/docs_threads --placement=compact --filter=bmSharded 
```

Exit status: 0

```text
speedgun-ng 0.14.0 build=Release
topology: packages=1 l3-groups=4 cores=24 threads=48
bmSharded/threads:4      iterations=1404220208  real time/iter (ns)=    0.514  cpu time/iter (ns)=    0.513  overhead floor (ns)=  211.000  endpoint samples=8
  contexts=4  ctx0=SPIN/0@0/applied  ctx1=SPIN/24@0/applied  ctx2=SPIN/1@0/applied  ctx3=SPIN/25@0/applied
```

## The N13-V probes

The idle preset resolves one probe cpu per L3 group (FR-016): the
lowest free cpu of each group the plans touch.

## N13-V idle one probe per L3 group

```sh
/home/archerc/code/speedgun-ng/build/example/docs_threads --placement=l3 --filter=bmSharded --jitter-probe=idle 
```

Exit status: 0

```text
speedgun-ng 0.14.0 build=Release
topology: packages=1 l3-groups=4 cores=24 threads=48
bmSharded/threads:4      iterations=2288536096  real time/iter (ns)=    0.319  cpu time/iter (ns)=    0.318  overhead floor (ns)=  110.000  endpoint samples=8
jitter: worst cpu 19 gaps=1103 max=234360 total=0.26
  contexts=4  ctx0=SPIN/0@0/applied  ctx1=SPIN/6@1/applied  ctx2=SPIN/12@2/applied  ctx3=SPIN/18@3/applied
jitter probe: cpus 1,7,13,19
gapCount: 1069 1192 1092 1104
gapsPerSecond: 657.48 733.03 671.55 677.38
minNs: 210 210 210 210
medianNs: 378 378 378 377
meanNs: 3820.70 3333.23 3686.23 3893.82
p90Ns: 10920 10080 10374 10583
p99Ns: 15960 14448 15078 14153
p999Ns: 31248 20957 31836 18606
p9999Ns: 36624 25787 32381 234360
maxNs: 36624 25787 32381 234360
totalNs: 4084331 3973205 4025368 4298773
totalPercent: 0.25 0.24 0.25 0.26
runtimeNs: 1625905722 1626133740 1626086238 1629798030
thresholdNs: 200 200 200 200
loopPeriodNs: 1 1 1 1
droppedGaps: 0 0 0 0
jitter: probe on cpus 1,7,13,19
jitter: loop period 1 ns, threshold 200 ns
jitter: cpu 1 kept 1069 dropped 0
jitter: cpu 7 kept 1192 dropped 0
jitter: cpu 13 kept 1092 dropped 0
jitter: cpu 19 kept 1104 dropped 0
jitter: split and percentiles use the kept gaps only
```

The overlap check: cpu 0 is a planned cpu of the l3 plan, so the
alongside run refuses it with one line and exit status 2 before any
benchmark runs (FR-018, D-3).

## N13-V overlap error on a planned cpu

```sh
/home/archerc/code/speedgun-ng/build/example/docs_threads --placement=l3 --filter=bmSharded --jitter-probe=0 
```

Exit status: 2

```text
--jitter-probe: probe cpu 0 overlaps the planned cpus of 'bmSharded/threads:4'
speedgun-ng 0.14.0 build=Release
topology: packages=1 l3-groups=4 cores=24 threads=48
```

The overhead measurement (D-8): the wall time of the same benchmark
run with and without the probe alongside. `bmTouch` runs unpinned, so
the with-probe run narrows the process affinity mask around the probe
cpu and discloses the change (D-5, FR-019).

```text
without probe: 1.127 s
with probe:    1.201 s
```

## N13-V full report with the probe

```sh
/home/archerc/code/speedgun-ng/build/example/benchmark_example --filter=bmTouch --jitter-probe=2 
```

Exit status: 0

```text
speedgun-ng 0.14.0 build=Release
bmTouch                  iterations=1735910433  real time/iter (ns)=    0.399  cpu time/iter (ns)=    0.398  overhead floor (ns)=  160.000  endpoint samples=2
jitter: worst cpu 2 gaps=37456 max=76860 total=0.49
jitter probe: cpus 2
gapCount: 37466
gapsPerSecond: 7178.29
minNs: 209
medianNs: 336
meanNs: 678.09
p90Ns: 420
p99Ns: 10163
p999Ns: 21587
p9999Ns: 64260
maxNs: 76860
totalNs: 25405194
totalPercent: 0.49
runtimeNs: 5219349450
thresholdNs: 200
loopPeriodNs: 1
droppedGaps: 0
jitter: probe on cpus 2
jitter: loop period 1 ns, threshold 200 ns
jitter: cpu 2 kept 37466 dropped 0
jitter: split and percentiles use the kept gaps only
jitter: affinity mask narrowed for 'bmTouch'
```

## Result

The runs above carry their exit status and captured output inline.
The host state of this record and the gap note on the `--threads`
dimension close what FR-043 asks the results to carry.
