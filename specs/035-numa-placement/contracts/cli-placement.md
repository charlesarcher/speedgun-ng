# Contract: the placement options with the node plans

Amendment of `specs/025-topology-placement/contracts/cli-placement.md`
(specs/035). The standing rows stand unchanged except where this file
amends them; the amendment lands in the same commit as the parser.

## Option table

No row is added and no value moves. The standing `--placement` row
(`source/harness/options.cpp:620`) carries the node plans through the
preset name, and the `BenchmarkHandle::placement(name)` handle carries
the same names for a suite
(`include/speedgun-ng/benchmark.hpp:2561`).

## Preset grammar

| Input | Behavior |
| --- | --- |
| `--placement=node` | the process default preset becomes `node` |
| `--placement=none` | accepted as the standing name: no plan, no binding, the behavior master ships |
| `--placement=l2` | one line on standard error, exit status 2, no benchmark runs (the standing refusal) |
| an unknown name | the standing unknown-preset line, exit status 2 |
| `--placement` with no argument | the standing `getopt_long` missing-argument path, exit status 2 |

The six names stand matched in full and nothing else:
`none`, `core`, `l3`, `smt`, `compact`, `node`
(`source/harness/placement_preset.cpp:46` grows by one row).

## Listing output

The listing keeps the standing header and L3 lines and adds the node
family under the L3 lines, in the L3 lines' shape:

```text
topology: packages=1 l3-groups=4 cores=24 threads=48 mask=0-47
l3 0: 0-5,24-29
l3 1: 6-11,30-35
l3 2: 12-17,36-41
l3 3: 18-23,42-47
node 0: 0-11,24-35
node 1: 12-23,36-47
```

The no-memory-information listing prints the node family as one
line:

```text
node: not available
```

| Rule | Requirement |
| --- | --- |
| One line per memory node the process mask touches, in the read's node order, naming the node number and the cpus of that node inside the process mask; a node the mask excludes prints no line. | FR-006 |
| The node numbers, the cpus and the counts stay the ones the hwloc read states; the listing renames and regroups nothing. | FR-006, D-3 |
| A read that states no memory information prints one line reading `node: not available` in the node family's place and keeps the rest of the listing byte-identical. | FR-007 |
| A machine read the harness cannot read refuses one line, exit status 2. | FR-008, standing |

## Refusal rows

| Input | Behavior |
| --- | --- |
| a denied bind on a planned context | one line naming the context and the node, exit status 2, before any benchmark runs |
| `node` over a read with no memory information | one line, exit status 2, before any context starts |
| a context count the node plan cannot fit inside the mask | the standing unfittable-count line, exit status 2 |

## Exit statuses

| Status | Case |
| --- | --- |
| 0 | a normal run, and the listing on a readable layout, with or without node plans |
| 2 | the standing refusal set, plus the denied bind and the no-memory-information refusal |

## Consumers

- `test/source/harness_cli_placement_test.cpp` drives the grammar and
  the listing rows.
- `test/source/harness_numa_placement_test.cpp` drives the plan, the
  binder seam and the refusal rows with synthetic layouts.
- `example/docs/numa_placement.cpp` keeps its recorded run under the
  shape gate.
