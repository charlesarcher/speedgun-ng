# The osnoise acceptance run on thrawn

- Engine: `osnoise`.
- Run: 2026-10-29, the lane host's standing boot, `uname -r` reads
  `7.3.0-5-cachyos`.
- The host tracing state the run found: `current_tracer` reads
  `nop`, `tracing_cpumask` reads `ffff,ffffffff`, `tracing_on`
  reads `0`. The host state stands restored after the run: the same
  three reads answer `nop`, `ffff,ffffffff` and `0` (R-04).

## The runs

```
$ ./build/example/benchmark_example --topology
topology: packages=1 l3-groups=4 cores=24 threads=48 mask=0-47
l3 0: 0-5,24-29
l3 1: 6-11,30-35
l3 2: 12-17,36-41
l3 3: 18-23,42-47
node 0: 0-47
exit 0

$ sudo ./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
     --jitter-probe=idle,mode=precheck --os-noise=osnoise --json=/tmp/osnoise.json
exit 0
```

The pre-check block the run printed, the tracer lines beside the
standing block:

```
tracer: engine osnoise cpus 1
tracer cpu 1 nmi: not available
tracer cpu 1 irq: not available
tracer cpu 1 softirq: not available
tracer cpu 1 thread: not available
tracer cpu 1 hardware: not available
```

The tracer cpus stand the candidate set the plans resolve: the
compact placement of two threads plans cpus 0 and 24, both inside
`l3 0`, so the candidate set is one free cpu of that group, cpu 1
(FR-008). The standing block, the warning lines and the pinning
line stand unchanged around the tracer lines; the exit status is 0
(FR-016, SC-004).

## The per-CCD figure table

The CCD names stand the `--topology` listing's L3 groups, one to
one onto the host's four CCDs (`CCD0` stands `l3 0: 0-5,24-29`):

| CCD | tracer cpu | nmi | irq | softirq | thread | hardware |
| --- | --- | --- | --- | --- | --- | --- |
| CCD0 | 1 | not available | not available | not available | not available | not available |

The JSON document carries the same record, the five figures `null`
(`/tmp/osnoise.json`, `schemaVersion` `1.7`).

## The count-file re-confirmation

The re-confirmation rode the run. The host tree carries no
per-cpu count file of the form the parser pins: the `osnoise`
directory of the tracing tree lists `cpus`, `options`, `per_cpu`,
`period_us`, `print_stack`, `runtime_us`,
`stop_tracing_total_us`, `stop_tracing_us`,
`timerlat_align_us` and `timerlat_period_us`, and
`osnoise/per_cpu/cpu0/` lists `timerlat_fd` alone; no
`osnoise/cpu0` file stands, with the tracer set and tracing on.
The figures the engine states stand the trace buffer's per-period
lines and the `events/osnoise/` trace events, so the five figures
of the record stand `not available` on this host, the per-source
disclosure of FR-010.

**Question for the pull request**: the count-file format the
parser pins (one line per source, `<source-word>: <count>`, the
words `nmi`, `irq`, `softirq`, `thread`, `hw`) stands one reading
short of any reachable tree; the lane host's kernel exposes the
figures through the trace buffer alone. The mismatch rides the
question path the research of R-3 fixed; a later lane re-anchors
the parser and the fixture format to a tree that carries the
files, or the lane takes the trace-line surface the spec holds out
of scope today.

## The suite

```
$ ctest --test-dir build -R harness_noise_test
exit 0
```
