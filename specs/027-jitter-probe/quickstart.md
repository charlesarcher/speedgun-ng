# Quickstart: Jitter Probe

Validation a reviewer runs at the feature head. Every step is a command
in this repository; nothing here needs a privilege. The host examples
name thrawn (24 cores, 48 threads, 4 L3 groups); the probe runs need a
host with 4 or more cpus (AC-1).

## Build and test

```sh
cmake --preset=ci-rocky
cmake --build build
ctest --test-dir build --output-on-failure -j 2
```

The configure and the build exit 0. The ctest run exits 0 and carries
four new executables in its list: `harness_jitter_probe_test`,
`harness_jitter_window_test`, `harness_jitter_preset_test`,
`harness_jitter_cli_test`.

## Run the probe alongside

```sh
./build/example/benchmark_example --filter=bmTouch --jitter-probe=2
```

`bmTouch` carries no placement, so the run also discloses the
affinity-mask change D-5 records. PASS when the console report prints
the probe block after the benchmark tables, one column for the one probe
cpu, with the fields of E-03 in order, and the per-benchmark line
`jitter: worst cpu <n> gaps=<k> max=<ns> total=<percent>` prints under
the rows (SC-001, FR-023, FR-024). The exit status is 0.

## Read the JSON back

```sh
./build/example/benchmark_example --filter=bmTouch --jitter-probe=2 \
  --json=/tmp/jitter.json
python3 -c 'import json; d=json.load(open("/tmp/jitter.json")); print(list(d))'
ctest --test-dir build -R 'harness_jitter_window|harness_json_load' \
  --output-on-failure
```

The key list prints
`['schemaVersion', 'speedgun', 'jitter', 'benchmarks']`: the root
`jitter` object stands after `speedgun` and before `benchmarks` (FR-026),
and each benchmark entry carries its own `jitter` object after
`noiseWarning` (FR-027). The round-trip tests read the written file back
through `loadReport` with equal values (SC-001, FR-029).

## The idle preset on thrawn

```sh
./build/example/docs_threads --placement=l3 --filter=bmSharded \
  --jitter-probe=idle
```

`bmSharded` plans one context per L3 group, so all 4 groups stand in
scope for the preset. PASS when the run resolves one probe cpu per L3
group, the lowest free cpu of each, and the disclosure names all of them
(US2, FR-016). A group with no free cpu is skipped and disclosed (US2,
FR-017).

## The usage errors

```sh
./build/example/benchmark_example --jitter-probe=99999
./build/example/benchmark_example --jitter-probe=2,threshold=0
./build/example/benchmark_example --jitter-probe=2,gaps=0
./build/example/docs_threads --placement=l3 --filter=bmSharded \
  --jitter-probe=0
```

PASS when each command prints one line and exits with status 2 before
any benchmark runs (SC-002, FR-038, FR-008, FR-009). The last names the
cpu and the benchmark in its line: on thrawn, cpu 0 is a planned cpu of
the `l3` run, and its SMT sibling cpu 24 is refused the same way
(FR-018, D-3).

## Without the option

```sh
ctest --test-dir build -R 'harness_jitter_cli|report_shape' \
  --output-on-failure
```

`harness_jitter_cli_test` carries the probe-free no-probe case, and
`report_shape` keeps every registered comparison folding equal on every
probe-free binary (SC-003, FR-013).

For a by-hand check against master itself:

```sh
git worktree add /tmp/sg027-base master
cmake -S /tmp/sg027-base -B /tmp/sg027-base/build
cmake --build /tmp/sg027-base/build
./build/example/benchmark_example --filter=bmTouch \
  --json=/tmp/head.json > /tmp/head.txt
/tmp/sg027-base/build/example/benchmark_example --filter=bmTouch \
  --json=/tmp/base.json > /tmp/base.txt
diff /tmp/head.txt /tmp/base.txt
python3 -c 'import json; k=lambda o: [list(x) for x in (o, *o["benchmarks"], *o["benchmarks"][0]["rows"])]; print(k(json.load(open("/tmp/head.json"))) == k(json.load(open("/tmp/base.json"))))'
```

Expected: the diff shows no difference outside the measured numbers, and
the key-sequence check prints `True`. Two real runs differ in their
measured numbers; this by-hand diff is the manual check FR-013 records
beside the scripted runs above.

## The example

```sh
cmake --build build --target run_docs_jitter_probe
ctest --test-dir build -R 'example_run|example_drift|snippet_compile' \
  --output-on-failure
```

The example `example/docs/jitter_probe.cpp` runs one benchmark
with `--jitter-probe=idle,mode=precheck` through the library (FR-035,
US5). PASS when the runner prints the probe block and the `.expected`
check holds on the three host-independent facts it pins: the block
header, the field names, and the cpu count rule. The drift gate holds
the README section "Jitter probe" equal to the example region, within
25 lines (FR-034, SC-007).

## Out of scope for the gates

The thrawn acceptance script `tools/noise/jitter_probe_thrawn.sh` runs
the N4 presets and the N13-V probes, and records its results under
`specs/027-jitter-probe/results/` (FR-042, FR-043, SC-008). The
coverage, sanitizer and tsan gates of SC-006 run in CI. The `jitter:`
rule names of US4 ride the merged N5 parser and validate through the
parser surface (SC-004).
