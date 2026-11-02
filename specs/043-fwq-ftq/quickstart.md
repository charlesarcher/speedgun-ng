# Quickstart: Fixed Work Quanta and Fixed Time Quanta Probes

Validation guide for `specs/043-fwq-ftq`. The scenarios below prove the
feature end to end; the contracts and the data model carry the shapes.
Implementation detail stands in `tasks.md`.

## Build and test

```sh
cmake --preset=ci-rocky
cmake --build build
ctest --test-dir build --output-on-failure
```

The feature's own executables:

```sh
ctest --test-dir build -R 'harness_fwq_ftq_test|harness_spectrum_test|harness_jitter_bytes_test' --output-on-failure
```

The grammar rows extend `harness_jitter_cli_test`, the record rows
extend `harness_jitter_window_test`, and the version rows stand in
`harness_cli_table_test` at
`test/source/harness_cli_table_test.cpp:1382-1393`. The suite passes
under a two-cpu mask and on a 2-cpu host, the section below
(spec FR-024).

No test sleeps and no test reads a clock: the series and the time
edges come from `installTestReader` and the scripted provider (spec
FR-024).

## The fixed work quanta run

```sh
./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --min-time=10 --repetitions=1 \
  --jitter-probe=idle,method=fwq,mode=precheck,gaps=10485760
```

Expected: the four calibration lines print on each probe cpu before that
cpu's window, naming `method`, `workCount`, `targetNs` at 1,000 ns and
`calibrationSpanNs`; the block after the benchmark tables prints the 30
series lines, one column per probe cpu; `droppedSamples` reads 0 at
the 10,485,760-entry buffer; the exit status is 0
([contracts/console-report.md](./contracts/console-report.md)).

## The fixed time quanta run

```sh
./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --min-time=10 --repetitions=1 \
  --jitter-probe=idle,method=ftq,quantum=100,mode=precheck,gaps=131072
```

Expected: the same block shape, the figures in counts, `quantumNs` at
100,000, and the calibration target at one thousandth of the quantum;
the trailing open quantum records no count; the exit status is 0
(spec FR-015, FR-016).

## The calibration disclosure

```sh
./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle,method=fwq 2>&1 | grep -E 'workCount|unitTimeNs|targetNs'
```

Expected: one figure set per probe cpu, printed before the window
starts; the search doubles the count from 1 in at most 25 trials and
bisects in at most 24 steps, two reads per trial, at most 98 reads
(spec FR-010, FR-011, SC-003).

## The period list

```sh
./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle,method=ftq,quantum=1000,gaps=131072 \
  --json=/tmp/fwq-ftq.json
python3 -c "import json;d=json.load(open('/tmp/fwq-ftq.json'));print(d['jitter']['method']);print(d['jitter']['series'][0]['periods'])"
ctest --test-dir build -R 'harness_spectrum_test' --output-on-failure
```

Expected: `FTQ` prints, then at most five period records, strongest
first, each with `periodNs` and `powerRatio`; a period stands only
above three times the median of bins 1 through N/2; the transform
reads at most 65,536 points (spec FR-017, FR-018). The host's timer
tick period stands at 1 ms on a `CONFIG_HZ=1000` kernel, and the
acceptance record names the period the run found.

## The usage errors

```sh
./build/example/benchmark_example --jitter-probe=idle,method=x
./build/example/benchmark_example --jitter-probe=idle,method=ftq,quantum=0
./build/example/benchmark_example --jitter-probe=idle,method=ftq,quantum=x
./build/example/benchmark_example --jitter-probe=idle,method=fwq,work=0
./build/example/benchmark_example --jitter-probe=idle,method=fwq,quantum=1000
./build/example/benchmark_example --jitter-probe=idle,method=ftq,work=1000
./build/example/benchmark_example --jitter-probe=idle,method=fwq,strict,noise=150
```

Each prints one quoted line on standard error and exits 2, before the
first run starts and with no benchmark output (spec FR-003, FR-021,
SC-005). A bad value on an option file line names the path, the line
and the token.

## The default run stays byte-identical

```sh
./build/example/benchmark_example --filter=bmTouch --threads=2 --placement=compact \
  --jitter-probe=idle --json=/tmp/tsc.json
ctest --test-dir build -R 'harness_jitter_bytes_test|harness_json_schema_test' --output-on-failure
```

The document carries the standing keys and no `method` key and no
`series` key, at schema `1.12`
([contracts/report-json.md](./contracts/report-json.md)). The bytes
match master's apart from the `schemaVersion` value and the version
figures the report carries, and every standing console line stays
byte-identical (spec FR-006, FR-026, SC-004).

## The example

```sh
cmake --build build --target docs_fwq_ftq
./build/example/docs_fwq_ftq --min-time=0.01
ctest --test-dir build -R "report_shape_fwq_ftq_sample|example_run|example_drift|snippet_compile" --output-on-failure
```

Expected: one FWQ family and one FTQ family print, each with its
calibration lines and its series block; the fold prints no difference;
the drift gate reads the 18-line section, and the page stands at 991
lines against `PAGE_LINE_CAP = 1004` (`tools/docs/example_drift.py:240`,
spec FR-023, FR-029, SC-007).

## The suite under a two-cpu mask

```sh
taskset -c 0,1 ctest --test-dir build --output-on-failure
```

Expected: the same pass line as the full-affinity run. The probe cpus
come from `sched_getaffinity`, so no test names a fixed cpu number. The
narrow-mask rows print their standing `SKIPPED` note at
`test/source/harness_jitter_window_test.cpp:623-626` and
`test/source/harness_precheck_test.cpp:443-445` (spec FR-024, SC-006).

## The acceptance runs

The script is a script and never a CI gate, the standing shape of
`tools/noise/jitter_probe_acceptance.sh:4-5` (spec FR-028):

```sh
bash tools/noise/fwq_ftq_acceptance.sh
```

The command list, in the order the script runs it, two methods, two L3
groups and two load states, eight runs:

```sh
./build/example/benchmark_example --filter=bmTouch --min-time=10 --repetitions=1 \
  --jitter-probe=<cpu>,method=fwq,mode=precheck,gaps=10485760
./build/example/benchmark_example --filter=bmTouch --min-time=10 --repetitions=1 \
  --jitter-probe=<cpu>,method=ftq,mode=precheck,gaps=131072
```

Each run names one idle cpu of one L3 group, and the loaded pair adds a
busy loop pinned to that cpu's SMT sibling. `--min-time=10` with
`--repetitions=1` gives one 10 s window per run, and the `gaps=` value
keeps every sample of it.

Expected: one markdown record per script run stands under
[results/](./results/) at the stamped name the script prints,
`acceptance-<date>.md`, the shape of
`specs/027-jitter-probe/results/acceptance-2026-10-21.md`. Each record
carries eight sections, each naming the command line, the exit status
and the captured output, and each shows a drop count of 0. Each record
names the host, the kernel, `CONFIG_HZ`, the L3 group, the calibration
count, the measured unit time, the noise fraction, the `gaps=` value,
the kept span in milliseconds and the detected periods (spec FR-027,
FR-028, SC-008). `CONFIG_HZ` comes from `/proc/config.gz`, then
`/boot/config-$(uname -r)`, and the record reads `unknown` when both
stand absent. No CI gate stands on the records.

## Out of scope for the gates

- Real noise figures: the suite scripts its series, and no test
  asserts a figure of the host (spec FR-025).
- A real `CONFIG_HZ` value: the acceptance record names the period the
  host's own value states (spec FR-027).
- The GitHub runner hosts: every acceptance figure comes from the lane
  host, and CI runs the scripted suite alone (spec D-10, G-20).
- A second probe framework, and two methods in one run: one method
  per run, and a user who wants both runs the probe twice (spec D-1).
