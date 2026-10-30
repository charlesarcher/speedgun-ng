# Context groups: the evidence quickstart

The lane's evidence stands one record per acceptance run, beside the
script that drove every run:
[results/run_groups.sh](./results/run_groups.sh). Each record names the
kind of every run it quotes, its command line, its exit code and its
artifacts. This index links every record and the driver, and names the
command that re-runs them (FR-038, SC-010). A host run reads the shape
the lane host states; a seam run names the `ctest` invocation whose
test installs a shape no lane host states.

| The question | The record | The verdict |
| --- | --- | --- |
| Which groups does a preset name over a multi-group shape? | [results/preset-core.md](./results/preset-core.md) | three runs exit 0: sixteen contexts fill `l3-0` and `l3-1` of eight each, two contexts under `l3` fill two single-member groups, and the seam scenario names four groups of two |
| Does the `node` preset group contexts by the memory node the plan names? | [results/preset-node.md](./results/preset-node.md) | both runs exit 0: two installed nodes take four contexts in alternating memberships, and the host's one node names one group |
| Does one layout file name a run's partition end to end, on any host? | [results/layout.md](./results/layout.md) | both runs exit 0: the file's four names pace eight contexts, two each, with no pin and no machine line |
| Does every fault shape and forbidden pairing stop with one line and status 2? | [results/refusals.md](./results/refusals.md) | 18 of 18 cases answer one line, exit status 2 and no report file |
| Does a run with no partition keep the bytes `master` prints and writes? | [results/ungrouped.md](./results/ungrouped.md) | the 157-byte disclosure line stands byte-identical on both trees, and neither document carries a group key |
| Does every hard gate of Principle VIII read green on this branch? | [results/verdict.md](./results/verdict.md) | 15 rows, one per gate: 152 of 152 tests on `ci-rocky` and 147 of 147 on `ci-ubuntu`, zero findings on the sanitizer, thread-sanitizer, analyzer, naming, prose, spell, format, time-source, drift and DBC reads, and coverage at 100.0% lines with 7,343 of 7,360 branches inside the recorded allowance |

The sixth record is the gate verdict,
[results/verdict.md](./results/verdict.md). It stands beside the five
acceptance records and reads the lane's gates one at a time: the
command, the tree it ran in, and the reading in the gate's own numbers
(VIII). It names the two wrinkles that move a count between two trees,
states the coverage claim in the lane's numbers, and lists the findings
that stand outside the lane (FR-038, SC-010).

Each record names the host whose topology its host runs read: the
recorded pass stands on `gary`, 16 cores over two L3 groups and one
memory node. Each record also quotes the transcript lines its runs
produced. The transcripts themselves stand at `reports/*.log`, which
`.gitignore:35` keeps untracked, so a reader who wants the files
re-runs the driver.

## Re-running the five records

One pass re-runs all five records and rewrites every capture in place.
The branch tree:

```
cmake --preset=ci-rocky -B build-w7a && cmake --build build-w7a
```

The ungrouped comparison reads its far side from a worktree of the lane
base `bea6af5b`, built outside this checkout. The recipe, its
submodules init beside it, stands whole in
[results/ungrouped.md](./results/ungrouped.md):

```
git worktree add /tmp/sg039-master bea6af5b
cd /tmp/sg039-master
git -c protocol.file.allow=always submodule update --init external/hwloc external/simdjson external/yaml-cpp external/hdrhistogram_c external/zlib external/quill external/pmu-events
cmake --preset=ci-rocky -B build && cmake --build build -j 10
```

Then the driver, from this checkout's root:

```
bash specs/039-context-groups/results/run_groups.sh --build build-w7a --master-build /tmp/sg039-master/build
```

The pass answers exit 0 when every reading matched, exit 1 on any
finding, and exit 2 on a usage error; a `finding:` line names what
differed. `--build` defaults to `build-w7a`. The far side stands
optional. Without `--master-build` the ungrouped run records the branch
bytes alone and states the gap in its transcript. With it, that run
answers two commands, one per tree, and diffs the two disclosure lines.
The driver's closing line counts that run once among the five; the
record carries both commands.

## The feature docs

- The main page's section: `README.md`, `Context groups`.
- The guide's section: `docs/pages/harness.md`, `Context groups`.
- The contracts: [contracts/group-partition.md](./contracts/group-partition.md)
  and [contracts/group-layout.md](./contracts/group-layout.md).
