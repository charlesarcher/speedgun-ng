# Quickstart: Hardware Latency Probe with SMI Attribution

Validation guide for `specs/048-hwlat-probe`. The two runs below prove
the feature end to end; the contracts and the data model carry the
shapes. Implementation detail stands in `tasks.md`.

## Build and test

```sh
cmake --preset=ci-rocky
cmake --build build
ctest --test-dir build --output-on-failure
```

The feature's own executables:

```sh
ctest --test-dir build -R 'harness_latency_counters|harness_hwlat' --output-on-failure
```

The suite passes pinned to two cpus, the shape the coverage host uses:

```sh
taskset -c 0,1 ctest --preset prepush-quick -j 2
```

Every kernel read of the suite resolves fake files installed through the
file-root seam, so the suite needs no root, no `msr` node and no tracefs
(spec FR-032).

## The method run without root

```sh
./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle,method=hwlat --json=/tmp/hwlat-method.json
```

Expected: the jitter block prints the standing figures, and each probe
cpu line adds the SMI change, the NMI change and the count of marked
gaps. Each kept gap of a `--json-raw=gaps` run carries its own mark
([contracts/console-report.md](./contracts/console-report.md)). The exit
status stands 0.

On a host that withholds the sources, the same command answers
`not available`: the `msr` node opens for root only, and
`/proc/interrupts` carries no `NMI` row for an unprivileged reader. The
console states each source once, the gap figures keep their standing
shape, the three new keys stand absent from the document, and the exit
status stands 0 (spec FR-001, FR-011, FR-016).

## The detector run as root

The setup writes need the rights a root shell carries. The feature never
asks for them, and it never refuses a host that lacks them. Record each
value the run saves before the run, so the same command proves the
restore afterwards (spec SC-007). The mode file reads back a list with
the active word inside brackets, and the run saves that word alone, so
the helper prints the bracketed word for `hwlat_detector/mode`:

```sh
snap() { for f in current_tracer tracing_cpumask tracing_thresh \
           hwlat_detector/mode hwlat_detector/width hwlat_detector/window; do
           printf '%s\t%s\n' "$f" \
             "$(sed -e 's/.*\[\([^]]*\)\].*/\1/' /sys/kernel/tracing/$f)"
         done; }
snap > /tmp/tracing-before.txt
```

Then the run:

```sh
./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle,mode=precheck --os-noise=hwlat --json=/tmp/hwlat-detector.json
snap > /tmp/tracing-after.txt
diff /tmp/tracing-before.txt /tmp/tracing-after.txt && echo restored
```

Expected: during the window `current_tracer` holds `hwlat`, and the mode
file reads back with `per-cpu` inside brackets, the form the kernel
prints. The record names `per-cpu` as the saved value of that file. The
width, the window and the threshold hold the run's constants. After the
run the diff names no file: each snapshot line carries one saved value,
and `tracing_on` stands `0`, the shape the standing restore writes.
The tracer block states the window, the width and the threshold once, and
one figure line per tracer cpu with the longest gap, the gap count and
the NMI count (spec FR-019 to FR-021, FR-027).

A second shell can show the state during the window:

```sh
watch -n 0.5 'cat /sys/kernel/tracing/current_tracer /sys/kernel/tracing/hwlat_detector/mode'
```

## The two surfaces together

```sh
./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle,mode=precheck,method=hwlat --os-noise=hwlat
```

Each surface records on its own keys, and no figure of one enters the
other. The detector holds each tracer cpu's interrupts off for its width,
so the probe's gaps grow on a cpu both surfaces share: the probe marks a
gap SMI only when the SMI count moved in the span that ends at that gap,
and the kernel's figures count only the gaps its own sampler found
(spec FR-014, FR-027, D-9).

## The refusal cases

```sh
./build/example/benchmark_example --filter=bmTouch --jitter-probe=idle,method=smi
./build/example/benchmark_example --filter=bmTouch --jitter-probe=idle,mode=precheck --os-noise=hwlat-detect
./build/example/benchmark_example --filter=bmTouch --jitter-probe=idle,method=hwlat --os-noise=hwlat
```

Each prints one quoted line naming the fault and exits 2 before any
benchmark runs: the method word outside the four, the engine word outside
the three, and the engine word on a run with no pre-check window (spec
FR-003, FR-005).

## The record and the round trip

```sh
python3 -c "import json;d=json.load(open('/tmp/hwlat-detector.json'));print(d['schemaVersion']);print(d['precheck']['tracer'])"
```

Expected: `1.16`, and a `tracer` object whose engine reads `hwlat` and
whose `cpus` entries carry `maxGapNs`, `gapCount` and `nmiCount` beside
the standing fields. A method run carries `smiDelta`, `nmiDelta` and
`smiGapCount` in `jitter.stats[]` and in `precheck.cpus[].stats`, and
`smi` beside each raw gap item. `loadReport` reads the six figures back
with equal values, and a document of schema `1.15`, the minor that stands
before this one and carries none of these keys, loads unchanged (spec FR-004,
FR-025 to FR-029).

## The comparison names

```sh
./build/example/first_benchmark \
  --compare=/tmp/hwlat-method.json,/tmp/hwlat-method-2.json \
  --compare-rule='jitter:run:any:smiGapCount:lower:abs=0' \
  --compare-rule='hwlat:any:maxGapNs:lower:abs=200000'
```

The three probe figures ride the standing jitter grammar, whose field
table now carries 35 rows, and the three detector figures ride the new
head word `hwlat`, each name with the standing direction and threshold
suffix ([contracts/rule-names.md](./contracts/rule-names.md)). A figure
the record carries as absent reads 0, and the run gains no new status. A
name whose document carries no tracer record at all, or names a cpu the
tracer record does not carry, answers the standing shape for a missing
value at status 2 (spec FR-028).

## The fake-file suite path

The tests prove both surfaces without root and without a kernel tracer.
The fake tree stands under one installed root
(`installTestProcRoot`, `source/detail/proc_root.hpp`) and carries the
`msr` node, the `cpuinfo` vendor line, the `interrupts` row, the six
tracefs files and the per-cpu buffer text. Time comes from the scripted
window reader (`installTestReader`), and the counters from the fake
provider, so no test sleeps and no test reads a clock (spec FR-032,
FR-033). A path that stands a directory and a lowered file-size limit
carry the refused read and the refused write, so each error path stands
testable (spec FR-034).

## The acceptance runs on corsairone

Two root runs stand recorded, one per surface, from the built tree at the
lane head:

```sh
sudo ./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle,method=hwlat --json=/tmp/hwlat-method.json
sudo ./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle,mode=precheck --os-noise=hwlat --json=/tmp/hwlat-detector.json
```

One record per run stands under [results/](./results/):
[results/user-space.md](./results/user-space.md) and
[results/detector.md](./results/detector.md). Each carries the command,
the exit code, the figures the console printed, and the document keys the
run wrote. The detector record carries the before-and-after table of the
six saved files beside the `tracing_on` value, the kernel's own
`dmesg` lines for the detector if any, and the note that the kernel keeps
the run's threshold as its remembered default (spec SC-005, SC-007). The
gate record stands at [results/gates.md](./results/gates.md): the drift,
lint, spell, time-source and prose gates, the `dbc-gate` target, the
coverage reading with its line and branch figures, the release build, the
analyzer build, the sanitizer lane, the thread-sanitizer lane, and the
pinned suite reads (spec SC-003, SC-004).

## What this guide does not cover

The AMD answer of the vendor gate stands in the guide page and in
`harness_latency_counters_test`: a part with no documented SMI counter
answers `not available` and names no substitute (spec FR-010). The
restore of a run the kernel stopped by force stands out of reach of every
handler, and the record names each saved value so an operator can put the
host back by hand (spec D-15).
