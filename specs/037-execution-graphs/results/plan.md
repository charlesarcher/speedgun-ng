# The graph acceptance plan

The acceptance run stands on the lane host: one record of a fan-out and
fan-in graph over the host's cores, the way specs/035-numa-placement
recorded the NPS evidence (FR-030, D-9, R-14). One script produces the
record and no CI job runs it (FR-031).

## The command set

`tools/graph/acceptance.py` runs each command from the repository root
over the built tree and records the command, its exit code and what it
printed:

```
./build/example/docs_numa_placement --topology
./build/example/docs_numa_placement --threads=<cores> --iterations=1 --graph=<file>
./build/example/docs_numa_placement --threads=<cores> --iterations=1 --graph=<file> --json=<file>
grep -rn acceptance.py .github/workflows
```

The core count comes from the harness's own topology listing, so the
node count follows the host and the plan carries no fixed count. The
graph file stands in a scratch directory the script owns: one source
node at slot 0, one leaf per core between them, and one sink that
depends on every leaf, the nodes named in descending slot order.

## The record format

One file, `acceptance.md`, beside this plan:

- the host's core and thread shape beside the shape specs/035 recorded;
- the boot: the kernel, the release and the date;
- the graph file the script wrote;
- each command, its exit code and the output it printed;
- the printed node order, the figures beside it, and the node array
  order of the document the run wrote;
- the CI check that names no job for the script.

## The record

- [The graph acceptance run](./acceptance.md): the run this lane
  recorded, the graph over the host's cores, the printed order, the
  figures beside it and the CI check.
