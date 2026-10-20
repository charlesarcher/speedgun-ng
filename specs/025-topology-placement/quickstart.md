# Quickstart: Topology Placement

Validation a reviewer runs at the feature head. Every step is a command
in this repository; nothing here needs a privilege.

## Build and test

```sh
cmake --preset=ci-rocky
cmake --build build
ctest --test-dir build --output-on-failure -j 2
```

The four new executables appear in the ctest list:
`harness_topology_test`, `harness_placement_preset_test`,
`harness_placement_report_test`, `harness_cli_placement_test`.

## Read the machine

```sh
./build/example/benchmark_example --topology
```

On thrawn the block reads:

```text
topology: packages=1 l3-groups=4 cores=24 threads=48 mask=0-47
l3 0: 0-5,24-29
l3 1: 6-11,30-35
l3 2: 12-17,36-41
l3 3: 18-23,42-47
```

The exit status is 0 and no benchmark ran (FR-036, SC-12).

## Place by L3 group

```sh
./build/example/docs_threads --placement=l3 --filter=bmSharded
```

`bmSharded` registers four contexts. Expected: one topology line in the
context block, and one disclosure for that row with four `cpu@group`
fields on four different groups (SC-01, SC-07).

## Ask for more than the machine holds

```sh
./build/example/docs_threads --placement=smt
```

`smt` places two contexts on thrawn and `bmSharded` asks for four:

```text
--placement=smt: the preset places 2 contexts and this run asks for 4
```

on standard error, exit status 2, with no context block, no context started and no result row (SC-06, FR-018).

## Compare the two SMT shapes

```sh
./build/example/docs_topology_placement --placement=smt --filter=bmPair   # siblings of one core
./build/example/docs_topology_placement --placement=core --filter=bmPair  # one context per core
```

`bmPair` registers two contexts and carries no preset of its own, so the
command line chooses its shape. The disclosure of the first run names two
cpus on one core; the second names two cpus on two cores (US4). A bare
`./build/example/docs_topology_placement` runs `bmCompact`, whose handle
preset `compact` stands in place of the command line (FR-015).

## Narrow the mask

```sh
taskset -c 0-11 ./build/example/benchmark_example --topology
taskset -c 0-11 ./build/example/docs_threads --placement=l3
```

The listing shows two groups, and the four-context `l3` run stops with
status 2 (FR-005, spec Edge Cases: a preset that fits the machine but
exceeds the mask exits with status 2).

## Proofs and gates

```sh
bash tools/hwloc/hwloc_purity_scan.sh .
bash tools/hwloc/hwloc_nm_proof.sh . build
python3 tools/prose/prose_gate.py --check prose --mode tree
python3 tools/docs/example_drift.py
ctest --test-dir build -R 'report_shape|example_run' --output-on-failure
```

Expected: the purity scan still finds zero hwloc references under
`include/`, and the `nm` proof covers the harness archive and the library
archive (R-12).

## Sanitizers and coverage

```sh
cmake --preset=ci-sanitize && cmake --build --preset=ci-sanitize && \
  ctest --preset=ci-sanitize
cmake --preset=ci-tsan && cmake --build --preset=ci-tsan && \
  ctest --preset=ci-tsan
cmake --preset=ci-ubuntu   # clang-tidy and cppcheck wired in
cmake --build build
```

then the coverage tree this repository already drives in CI:

```sh
cmake --preset=ci-coverage && cmake --build build/coverage && \
  ctest --test-dir build/coverage --output-on-failure && \
  cmake --build build/coverage --target coverage
```

Expected: the new files `source/harness/topology.cpp` and
`source/harness/placement_preset.cpp` report 100 percent line and branch
coverage with no exclusion marker (FR-034, SC-08).

## Out of scope for the gates

The thrawn runs above are the owner's real-host check (SC-11). CI never
depends on a machine of a fixed shape: the placement rules run on
injected layouts, and the docs example names the widest preset, whose
place count equals the thread count of the mask (FR-029, R-10).
