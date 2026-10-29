# The timerlat acceptance run on thrawn

- Engine: `timerlat`.
- Run: 2026-10-29, the lane host's standing boot, `uname -r` reads
  `7.3.0-5-cachyos`, the shape of [osnoise.md](./osnoise.md).
- The host tracing state the run found and the run left restored:
  `current_tracer` `nop`, `tracing_cpumask` `ffff,ffffffff`
  (R-04).

## The runs

```
$ sudo ./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
     --jitter-probe=idle,mode=precheck --os-noise=timerlat --json=/tmp/timerlat.json
exit 0
```

The pre-check block the run printed, the tracer lines beside the
standing block:

```
tracer: engine timerlat cpus 1
tracer cpu 1 nmi: not available
tracer cpu 1 irq: not available
tracer cpu 1 softirq: not available
tracer cpu 1 thread: not available
tracer cpu 1 hardware: not available
warning: cpu 0 gapsPerSecond 558.70 maxNs 33348
warning: cpu 24 gapsPerSecond 497.22 maxNs 38052
pinning candidates: cpu 24 497.22 cpu 0 558.70
```

The tracer cpus stand the same candidate set the osnoise run
resolved: cpu 1, the free cpu of `l3 0` the compact plan of two
threads leaves (FR-008). The exit status is 0 (FR-016, SC-004).

## The per-CCD figure table

The CCD names stand the `--topology` listing's L3 groups (`CCD0`
stands `l3 0: 0-5,24-29`, the listing of
[osnoise.md](./osnoise.md)):

| CCD | tracer cpu | nmi | irq | softirq | thread | hardware |
| --- | --- | --- | --- | --- | --- | --- |
| CCD0 | 1 | not available | not available | not available | not available | not available |

The JSON document carries the same record, the five figures `null`
(`/tmp/timerlat.json`, `schemaVersion` `1.7`).

## The count-file re-confirmation

The re-confirmation rode the run and stands the finding of
[osnoise.md](./osnoise.md): the host tree carries no
`osnoise/cpu<id>` count file, with the `timerlat` tracer set and
tracing on, so the five figures stand `not available` on this
host. The same question stands for the pull request: the pinned
count-file format awaits a tree that carries the files, or the
lane takes the trace-line surface the spec holds out of scope
today.

## The suite

```
$ ctest --test-dir build -R harness_noise_test
exit 0
```
