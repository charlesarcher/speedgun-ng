# Quickstart: NUMA Memory-Node Placement

Feature: specs/035-numa-placement. Every scenario below runs on the
lane host or in the suite; the commands stand in code spans so the
prose gate reads none of them. The contract details stand in
[contracts/](./contracts/) and the entity rules in
[data-model.md](./data-model.md); this file carries the runs alone.

## Prerequisites

- The build stands configured: `cmake --preset=ci-rocky` and
  `cmake --build build -j 8` (the daily loop of the README).
- The acceptance host stands thrawn (the NPS BIOS setting), and the
  suite itself passes on any host: the synthetic layouts carry the
  node shapes the tests read.

## Scenarios

| Scenario | Command | Exit | Expected |
| --- | --- | --- | --- |
| The listing names the nodes | `./build/speedgun-ng --topology` | 0 | one line per node under the L3 lines, in the read's node order |
| The node plan binds and discloses | `./build/speedgun-ng --placement=node --benchmark family=threads --benchmark_context=2` | 0 | each context row's placement field reads `cpu@group@node` |
| The unbound run stands unchanged | the standing run with `--placement=l3` | 0 | the placement fields read `cpu@group`, no node tail |
| An unknown preset refuses | `./build/speedgun-ng --placement=nodes` | 2 | one line, the standing unknown-preset refusal |
| The node plan over no memory information | in the suite, through the injected read | 2 | one line, exit status 2, no context starts |
| A denied bind refuses | in the suite, through the fake binder | 2 | one line naming the context and the node, no benchmark output |
| The round trip holds | `ctest --test-dir build -R harness_json` | 0 | the written and reloaded documents compare equal |
| The example folds | `ctest --test-dir build -R report_shape_numa` | 0 | the recorded block folds equal against the binary's own run |

## The suite

The new executable runs beside the standing placement tests:

- `ctest --test-dir build -R harness_numa_placement_test` drives the
  plan, the binder seam and the refusal rows over synthetic layouts
  of one, two and four nodes.
- `ctest --test-dir build -R harness_cli_placement_test` drives the
  preset grammar and the listing rows.
- The full suite runs `ctest --test-dir build -j 1` for the coverage
  contract and passes under `taskset -c 0,1` and on a 2-cpu
  single-node host: the nodes come from the injected topology, and
  no test carries a fixed node number.

## The NPS evidence

The acceptance runs stand on thrawn, one record per NPS state the
board's menu offers, under `specs/035-numa-placement/results/`:

1. Set the NPS state in the board's menu and reboot; the record names
   the state and the boot.
2. Run the listing, the node-planned run and the merged pre-check
   beside it; the record carries the commands, the exit codes and
   the listing the run printed.
3. A state the menu cannot produce stands recorded as out of reach,
   with the gap named (FR-019, SC-005).

The records link from this file as the lane lands them.
