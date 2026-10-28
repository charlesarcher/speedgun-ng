# The quickstart runs on the lane host

The scenarios of [quickstart.md](../quickstart.md), run end to end on
thrawn at the `NPS1` state, with the commands, the exit codes and the
figures they printed (SC-001 to SC-006).

| Scenario | Command | Exit | The figures |
| --- | --- | --- | --- |
| The listing names the nodes | `./build/example/docs_topology_placement --topology` | 0 | one line per node under the L3 lines, `node 0: 0-47` |
| The node plan binds and discloses | `./build/example/docs_numa_placement --placement=node` | 0 | `ctx0=SPIN/0@0@0/applied  ctx1=SPIN/1@0@0/applied` |
| The unbound paths stand unchanged | `ctest --test-dir build -R harness_numa_placement_test` | 0 | all scenarios green |
| An unknown preset refuses | `./build/example/docs_numa_placement --placement=nodes` | 2 | one line, `--placement: 'nodes' is not one of the six preset names` |
| The node plan over no memory information | in the suite, through the injected read | 2 | one line, exit status 2, no context starts |
| A denied bind refuses | in the suite, through the fake binder | 2 | one line naming the context and the node, no benchmark output |
| The round trip holds | `ctest --test-dir build -R harness_json` | 0 | the written and reloaded documents compare equal |
| The example folds | `ctest --test-dir build -R report_shape_numa_sample` | 0 | the recorded block folds equal against the binary's own run |

The suite scenario runs:

```
$ ctest --test-dir build -R harness_numa_placement_test
exit 0
$ ctest --test-dir build -R harness_json
exit 0
$ ctest --test-dir build -R report_shape_numa_sample
exit 0
```

The listing and the node-planned run of the `NPS1` boot stand in
[nps1.md](./nps1.md); the records of the states the menu offers stand
beside it, [nps2.md](./nps2.md) and [nps4.md](./nps4.md) out of reach
with the gap named.

## The CI run

The run reference lands here when the pushed head's CI completes
(SC-001 to SC-006).
