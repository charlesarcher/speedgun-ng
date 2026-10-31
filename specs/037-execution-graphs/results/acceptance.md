# The graph acceptance run on chilly

- Shape: the harness listing reads `cores=16 threads=32` on 1 package
  and 2 L3 groups, mask `0-31`: 16 cores, 32 threads. The spec expects a
  16-core, 32-thread lane host (FR-030), and this host reads that shape.
  specs/035 recorded a 24-core, 48-thread host (thrawn, the Threadripper
  9960X lane box).
- Boot: 2026-10-30 08:41 CDT, kernel `Linux 7.3.0-3-cachyos x86_64`,
  release CachyOS.
- Script: `tools/graph/acceptance.py`, run by hand on the lane host
  (FR-031).
- Binary: `./build/example/docs_execution_graphs`, whose `bmChain`
  registers a threaded family, run with `--threads=16` so the graph's 16
  nodes match the run's 16 contexts (FR-012).

## The graph file

The script writes the graph the run reads, one node per core: `root` at
slot 0, `leaf1` to `leaf14` at slots 1 to 14 each depending on `root`,
and `join` at slot 15 depending on every leaf. The file names its nodes
in descending slot order, so the ascending order the run answers is the
graph's own order. The node count matches the run's context count
(FR-012). The file stood at the path the commands below name:

```json
{
  "nodes": [
    { "name": "join", "slot": 15, "deps": ["leaf1", "leaf2", "leaf3", "leaf4", "leaf5", "leaf6", "leaf7", "leaf8", "leaf9", "leaf10", "leaf11", "leaf12", "leaf13", "leaf14"] },
    { "name": "leaf14", "slot": 14, "deps": ["root"] },
    { "name": "leaf13", "slot": 13, "deps": ["root"] },
    { "name": "leaf12", "slot": 12, "deps": ["root"] },
    { "name": "leaf11", "slot": 11, "deps": ["root"] },
    { "name": "leaf10", "slot": 10, "deps": ["root"] },
    { "name": "leaf9", "slot": 9, "deps": ["root"] },
    { "name": "leaf8", "slot": 8, "deps": ["root"] },
    { "name": "leaf7", "slot": 7, "deps": ["root"] },
    { "name": "leaf6", "slot": 6, "deps": ["root"] },
    { "name": "leaf5", "slot": 5, "deps": ["root"] },
    { "name": "leaf4", "slot": 4, "deps": ["root"] },
    { "name": "leaf3", "slot": 3, "deps": ["root"] },
    { "name": "leaf2", "slot": 2, "deps": ["root"] },
    { "name": "leaf1", "slot": 1, "deps": ["root"] },
    { "name": "root", "slot": 0, "deps": [] }
  ]
}
```

## The runs

The listing names the host's shape, the graph run orders the context
starts of a threaded run over that shape, and the record run writes the
document read back below. Each command runs from the repository root
over the built tree:

```
$ ./build/example/docs_execution_graphs --topology
topology: packages=1 l3-groups=2 cores=16 threads=32 mask=0-31
l3 0: 0-7,16-23
l3 1: 8-15,24-31
node 0: 0-31
exit 0

$ ./build/example/docs_execution_graphs --threads=16 --iterations=1 --graph=/home/archerc/.hermes/cache/scratch/speedgun-graph-h31yat7d/fan-in-fan-out.json
speedgun-ng 0.25.0 build=Release
bmChain/threads:16       iterations=        16  real time/iter (ns)=  510.000  cpu time/iter (ns)=  398.125  overhead floor (ns)=  100.000  endpoint samples=32
  contexts=16  ctx0=SPIN/NO_PIN  ctx1=SPIN/NO_PIN  ctx2=SPIN/NO_PIN  ctx3=SPIN/NO_PIN  ctx4=SPIN/NO_PIN  ctx5=SPIN/NO_PIN  ctx6=SPIN/NO_PIN  ctx7=SPIN/NO_PIN  ctx8=SPIN/NO_PIN  ctx9=SPIN/NO_PIN  ctx10=SPIN/NO_PIN  ctx11=SPIN/NO_PIN  ctx12=SPIN/NO_PIN  ctx13=SPIN/NO_PIN  ctx14=SPIN/NO_PIN  ctx15=SPIN/NO_PIN
  graph nodes=root,leaf1,leaf2,leaf3,leaf4,leaf5,leaf6,leaf7,leaf8,leaf9,leaf10,leaf11,leaf12,leaf13,leaf14,join edges=leaf1->join,leaf2->join,leaf3->join,leaf4->join,leaf5->join,leaf6->join,leaf7->join,leaf8->join,leaf9->join,leaf10->join,leaf11->join,leaf12->join,leaf13->join,leaf14->join,root->leaf14,root->leaf13,root->leaf12,root->leaf11,root->leaf10,root->leaf9,root->leaf8,root->leaf7,root->leaf6,root->leaf5,root->leaf4,root->leaf3,root->leaf2,root->leaf1
exit 0

$ ./build/example/docs_execution_graphs --threads=16 --iterations=1 --graph=/home/archerc/.hermes/cache/scratch/speedgun-graph-h31yat7d/fan-in-fan-out.json --json=/home/archerc/.hermes/cache/scratch/speedgun-graph-h31yat7d/report.json
speedgun-ng 0.25.0 build=Release
bmChain/threads:16       iterations=        16  real time/iter (ns)= 1120.625  cpu time/iter (ns)=  650.625  overhead floor (ns)=  110.000  endpoint samples=32
  contexts=16  ctx0=SPIN/NO_PIN  ctx1=SPIN/NO_PIN  ctx2=SPIN/NO_PIN  ctx3=SPIN/NO_PIN  ctx4=SPIN/NO_PIN  ctx5=SPIN/NO_PIN  ctx6=SPIN/NO_PIN  ctx7=SPIN/NO_PIN  ctx8=SPIN/NO_PIN  ctx9=SPIN/NO_PIN  ctx10=SPIN/NO_PIN  ctx11=SPIN/NO_PIN  ctx12=SPIN/NO_PIN  ctx13=SPIN/NO_PIN  ctx14=SPIN/NO_PIN  ctx15=SPIN/NO_PIN
  graph nodes=root,leaf1,leaf2,leaf3,leaf4,leaf5,leaf6,leaf7,leaf8,leaf9,leaf10,leaf11,leaf12,leaf13,leaf14,join edges=leaf1->join,leaf2->join,leaf3->join,leaf4->join,leaf5->join,leaf6->join,leaf7->join,leaf8->join,leaf9->join,leaf10->join,leaf11->join,leaf12->join,leaf13->join,leaf14->join,root->leaf14,root->leaf13,root->leaf12,root->leaf11,root->leaf10,root->leaf9,root->leaf8,root->leaf7,root->leaf6,root->leaf5,root->leaf4,root->leaf3,root->leaf2,root->leaf1
exit 0
```

## The printed node order and the figures beside it

The graph block the run printed, the nodes in the ascending slot order
FR-023 fixes and the edges in the graph file's order:

```
  graph nodes=root,leaf1,leaf2,leaf3,leaf4,leaf5,leaf6,leaf7,leaf8,leaf9,leaf10,leaf11,leaf12,leaf13,leaf14,join edges=leaf1->join,leaf2->join,leaf3->join,leaf4->join,leaf5->join,leaf6->join,leaf7->join,leaf8->join,leaf9->join,leaf10->join,leaf11->join,leaf12->join,leaf13->join,leaf14->join,root->leaf14,root->leaf13,root->leaf12,root->leaf11,root->leaf10,root->leaf9,root->leaf8,root->leaf7,root->leaf6,root->leaf5,root->leaf4,root->leaf3,root->leaf2,root->leaf1
```

The figures beside it: the timed row of the same run and the context
disclosure under it:

```
bmChain/threads:16       iterations=        16  real time/iter (ns)=  510.000  cpu time/iter (ns)=  398.125  overhead floor (ns)=  100.000  endpoint samples=32
  contexts=16  ctx0=SPIN/NO_PIN  ctx1=SPIN/NO_PIN  ctx2=SPIN/NO_PIN  ctx3=SPIN/NO_PIN  ctx4=SPIN/NO_PIN  ctx5=SPIN/NO_PIN  ctx6=SPIN/NO_PIN  ctx7=SPIN/NO_PIN  ctx8=SPIN/NO_PIN  ctx9=SPIN/NO_PIN  ctx10=SPIN/NO_PIN  ctx11=SPIN/NO_PIN  ctx12=SPIN/NO_PIN  ctx13=SPIN/NO_PIN  ctx14=SPIN/NO_PIN  ctx15=SPIN/NO_PIN
```

The document the record run wrote names the same order in its node
array, each node with its slot and its outcome:

```
bmChain/threads:16: root@0 finished, leaf1@1 finished, leaf2@2 finished, leaf3@3 finished, leaf4@4 finished, leaf5@5 finished, leaf6@6 finished, leaf7@7 finished, leaf8@8 finished, leaf9@9 finished, leaf10@10 finished, leaf11@11 finished, leaf12@12 finished, leaf13@13 finished, leaf14@14 finished, join@15 finished
nodes=16 edges=28 held=0 stopped=0
```

## The CI check

No CI job names the script. The grep the record reads:

```
$ grep -rn acceptance.py .github/workflows
exit 1
```

The workflows declare 13 jobs (`lint`, `coverage`, `sanitize`, `tsan`,
`static-analysis`, `test`, `test-rocky`, `shared-audit`,
`downstream-consumer`, `consumer-release`, `dbc-gate`, `prose-lint` and
`docs`), and none names `acceptance.py` (FR-031, SC-009).

## The gap

No gap: the host reads the shape the spec expects, the fan-out and
fan-in graph ran over its cores, and the record carries the printed
order, the figures, the boot, the commands and the exit codes.
