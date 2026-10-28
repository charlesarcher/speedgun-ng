# NPS1 - the placement runs on thrawn

- State: `NPS1`, the state the boot ran on. `lscpu` reads
  `NUMA node(s): 1`, `NUMA node0 CPU(s): 0-47`.
- Boot: 2026-10-28, the lane host's standing boot.

## The runs

```
$ ./build/example/docs_topology_placement --topology
topology: packages=1 l3-groups=4 cores=24 threads=48 mask=0-47
l3 0: 0-5,24-29
l3 1: 6-11,30-35
l3 2: 12-17,36-41
l3 3: 18-23,42-47
node 0: 0-47
exit 0

$ ./build/example/docs_numa_placement --placement=node
speedgun-ng 0.22.0 build=Release
topology: packages=1 l3-groups=4 cores=24 threads=48
bmSharded/threads:2      iterations=2435620632  real time/iter (ns)=    0.211  cpu time/iter (ns)=    0.210  overhead floor (ns)=  100.000  endpoint samples=4
  contexts=2  ctx0=SPIN/0@0@0/applied  ctx1=SPIN/1@0@0/applied
exit 0
```

## The merged pre-check beside the placement runs

```
$ ./build/example/docs_numa_placement --placement=node '--jitter-probe=idle,mode=precheck,precheck=50'
exit 0
```

The pre-check measured the planned cpus (planned) and one free cpu of
each L3 group the plan leaves (candidate); the probe block summary:

```
gapsPerSecond: 433.65
gapsPerSecond: 281.88
gapsPerSecond: 303.53
gapsPerSecond: 433.65 281.88 303.53
```

The warnings and the pinning-candidate line it printed:

```
host settings: isolcpus=not available nohz_full=not available rcu_nocbs=not available
warning: cpu 0 gapsPerSecond 433.65 maxNs 36078
warning: cpu 1 gapsPerSecond 281.88 maxNs 17808
pinning candidates: cpu 1 281.88 cpu 0 433.65
```

## The suite

```
$ ctest --test-dir build -R harness_numa_placement_test
exit 0
```

## The gap

No gap: the state stands reachable and the runs stand recorded.
