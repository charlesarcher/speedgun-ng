# Quickstart: Osnoise and Timerlat Tracer Integration

Validation guide for `specs/036-osnoise-timerlat-tracers`. The
scenarios below prove the feature end to end; the contracts and the
data model carry the shapes. Implementation detail stands in
`tasks.md`.

## Build and test

```sh
cmake --preset=ci-rocky
cmake --build build
ctest --test-dir build --output-on-failure
```

The feature's own executables:

```sh
ctest --test-dir build -R 'harness_noise' --output-on-failure
```

The option-table rows extend `harness_cli_table_test`. The suite
passes under `taskset -c 0,1` and on a 2-cpu host (spec FR-018):

```sh
taskset -c 0,1 ctest --test-dir build -R harness_noise --output-on-failure
```

CI carries no kernel tracer and no root: every tracefs read of the
suite resolves fake files installed through the file-root seam (spec
FR-017).

## The tracer run on a real host

The setup writes need the rights a root shell carries; the feature
never asks for them and never refuses a host that runs without them.

```sh
./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle,mode=precheck --os-noise=osnoise
```

Expected: the pre-check block prints with the tracer lines beside it,
one figure line per source per tracer cpu, the engine word and the
cpus named on the header line; the standing block stands byte-identical
to the run without the option; the exit status is 0
([contracts/console-report.md](./contracts/console-report.md)).

The timerlat engine stands the same shape:

```sh
./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle,mode=precheck --os-noise=timerlat
```

## The not-available case

A run without the rights the setup writes need, and a host without
tracefs at the standing path, or without the engine, prints the
disclosure on the tracer lines alone and exits 0 (spec FR-016, SC-004):

```sh
./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle,mode=precheck --os-noise=osnoise; echo "exit $?"
```

Expected: the line `tracer: not available` stands beside the standing
block, the JSON document stands byte-identical to the run without the
option apart from the `schemaVersion` value, and the exit status is 0.

## The refusal cases

```sh
./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle,mode=precheck --os-noise=hrtimers
./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle,mode=precheck --os-noise=osnoise,cpus=0,cpus=1
./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --os-noise=osnoise
./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle --os-noise=osnoise
```

Each prints one line naming the fault and exits 2 with no benchmark
output (spec FR-003, FR-011, SC-003): the engine word outside the two;
the repeated key; the missing `--jitter-probe` option; the
`ALONGSIDE` mode that runs no pre-check window.

## The serial shape

```sh
./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle,mode=precheck,precheck=serial --os-noise=osnoise
```

Expected: the tracer stands set across every serial window, the run
writes no engine reset between the windows, and the figures name the
noise the tracer cpus saw across the sequence; the record's standing
`serial` and `windowMs` fields name the window shape beside them (spec
FR-009).

## The JSON round trip

```sh
./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle,mode=precheck --os-noise=osnoise --json=/tmp/noise.json
ctest --test-dir build -R 'harness_noise|harness_json_load' --output-on-failure
```

The document carries the `tracer` object inside `precheck`, after
`skippedGroups`, at schema `1.7`
([contracts/report-json.md](./contracts/report-json.md)), and the
round-trip tests read the written file back through `loadReport` with
equal values. A run without `--os-noise` writes a document whose bytes
match master's apart from the `schemaVersion` value (spec FR-015).

## The fake-file suite path

The tests prove the whole feature without a kernel tracer and without
root: the fake tracefs trees stand installed through the file-root
seam (`installTestProcRoot`, `source/harness/detail/proc_root.hpp`),
and the pre-check window stays bounded through the scripted reader
(`installTestReader`), so no test sleeps and no test reads a clock
(spec FR-019). The fake tree carries the engine word, the mask file,
the on-off file and the per-cpu count files the fixture pins.

## The acceptance runs on thrawn

The lane host carries the CCD shape the feature stands on: one probe
cpu per L3 group, each recorded figure naming the CCD and the engine
it came from (spec FR-020, D-7). The command set per engine, run from
the repository root on the built tree:

```sh
./build/example/docs_topology_placement --topology
./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle,mode=precheck --os-noise=osnoise --json=/tmp/osnoise.json
./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle,mode=precheck --os-noise=timerlat --json=/tmp/timerlat.json
ctest --test-dir build -R harness_noise
```

One record per run stands under
[results/](./results/): the osnoise record stands at
[results/osnoise.md](./results/osnoise.md) and the timerlat record
at [results/timerlat.md](./results/timerlat.md), each record
carrying the commands, the exit codes, the engine and the per-CCD
tables, and the count-file re-confirmation the runs stood beside
the figures (spec SC-006). No CI gate stands on the records.

## Out of scope for the gates

- Real kernel noise and the tracer's own load: CI runs the fake files
  alone (spec FR-017).
- A tracer tuning key: the grammar carries the engine and the cpu list
  alone (spec Out of scope).
- The CCD mapping in the runtime record: the record names cpus alone,
  and the mapping stands in the evidence (spec FR-014, D-7).
