# Quickstart: Host Noise Pre-Check

Validation guide for `specs/034-host-noise-precheck`. The scenarios
below prove the feature end to end; the contracts and the data model
carry the shapes. Implementation detail stands in `tasks.md`.

## Build and test

```sh
cmake --preset=ci-rocky
cmake --build build
ctest --test-dir build --output-on-failure
```

The feature's own executable:
`ctest --test-dir build -R harness_precheck --output-on-failure`.
The grammar rows extend
`harness_jitter_cli_test`. The suite passes under `taskset -c 0,1`
and on a 2-cpu host (spec FR-021):

```sh
taskset -c 0,1 ctest --test-dir build -R harness_precheck --output-on-failure
```

## The pre-check on a real host

```sh
./build/example/benchmark_example --filter=bmTouch --jitter-probe=idle,mode=precheck
```

Expected: the pre-check block prints before the benchmark tables,
one block per measured cpu; the attribution lines name their
sources; the host settings print once; the pinning line names every
planned cpu with its gap rate, lowest first; the exit status is 0
([contracts/console-report.md](./contracts/console-report.md)).

## The strict refusal

```sh
./build/example/benchmark_example --filter=bmTouch \
  --jitter-probe=idle,mode=precheck,strict,threshold=1
```

Expected: the full window completes, one line names the first noisy
planned cpu in cpu order with the gap rate and the max gap, and the
exit status is 2 with no benchmark row printed (spec FR-014). A host
whose planned cpus all stand quiet gives status 0 and the usual
tables.

## The serial shape

```sh
./build/example/benchmark_example --filter=bmTouch \
  --jitter-probe=idle,mode=precheck,precheck=serial
```

Expected: one window per measured cpu, ascending cpu order, the
planned cpus before the candidates; the blocks print in the same
order ([contracts/cli-options.md](./contracts/cli-options.md)).

## The usage errors

```sh
./build/example/benchmark_example --filter=bmTouch --jitter-probe=idle,mode=precheck,precheck=searial
./build/example/benchmark_example --filter=bmTouch --jitter-probe=idle,mode=precheck,precheck=0
./build/example/benchmark_example --filter=bmTouch --jitter-probe=idle,mode=precheck,precheck=4294967296
```

Each prints one line naming the value and exits 2 (spec FR-003).

## The JSON round trip

```sh
./build/example/benchmark_example --filter=bmTouch \
  --jitter-probe=idle,mode=precheck --json=/tmp/precheck.json
python3 -c 'import json; d=json.load(open("/tmp/precheck.json")); print(list(d))'
ctest --test-dir build -R 'harness_precheck|harness_json_load' --output-on-failure
```

The key list prints `precheck` between `jitter` and `benchmarks` at
schema `1.6`, and the round-trip tests read the written file back
through `loadReport` with equal values
([contracts/report-json.md](./contracts/report-json.md)). A run
without `--jitter-probe` writes a document whose bytes match
master's apart from the `schemaVersion` value (spec FR-024).

## The fake-file suite path

The tests prove the whole feature without real noise: the fake
`/proc` and `/sys` trees stand installed through the file-root seam,
and the windows close through the scripted reader, so no test sleeps
and no test reads a clock (spec FR-019, FR-011). The seam lives at
`source/harness/detail/proc_root.hpp`; the reader seam stays
`installTestReader`.

## Out of scope for the gates

- Real noise and real PMU counts: CI runs on the fake files alone
  (spec AC-b1).
- The N13d and N13e lanes: they reuse the seam and carry their own
  items.
- A root-only attribution source: the feature discloses without
  root, and a source it cannot read reads `not available`.
