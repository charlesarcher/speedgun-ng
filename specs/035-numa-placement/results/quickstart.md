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

## The hard gates at the head

The sanitizer, tsan and coverage suites ran at the head the branch
carries (T024):

```
$ ctest --test-dir build/sanitize -j 1    # 100% tests passed out of 138
$ ctest --test-dir build/tsan -j 1        # 100% tests passed out of 139
$ ctest --test-dir build/coverage -j 1    # 100% tests passed out of 138
```

The coverage contract read:

```
lines.......: 100.0% (12934 of 12934 lines)
coverage-gate: branch shortfall 19 rows, within the D-14 recorded
              allowance of 19 (accepted classes)
```

No `LCOV_EXCL` marker stands in the tree, and the branch carries no
new `--ignore-errors` entry.

Under `taskset -c 0,1` (T025, FR-017) the suite reads 142 of 143:
the one failure is the `report_shape_jitter_sample` fold, whose
recorded block names the two L3 groups the full host carries, so a
run held to two cpus prints one candidate section fewer; the host
dependence is the `034-host-noise-precheck` shape gate's, present on
master and untouched by this branch. The feature's own tests stand green
under the same constraint:

```
$ taskset -c 0,1 ctest --test-dir build -R 'numa_placement|placement_preset|report_shape_numa'
exit 0, 100% tests passed out of 3
$ taskset -c 0,1 ctest --test-dir build -R 'harness_cli_placement_test|execution_context'
exit 0, 100% tests passed out of 2
```

The 2-cpu single-node host stands out of reach like the `NPS2` and
`NPS4` boots, with the gap named in [nps2.md](./nps2.md).

## The CI run

The pull request [#56](https://github.com/charlesarcher/speedgun-ng/pull/56)
carries the branch, and the run of the pushed head `3d45dd7` reads
green job by job (T027, run `40774867120`, read 2026-10-28 with a
supplied token):

| Job | Conclusion |
| --- | --- |
| `lint` | success |
| `test` | success |
| `test-rocky` | success |
| `sanitize` | success |
| `tsan` | success |
| `static-analysis` (library, tests, examples) | success |
| `coverage` | success |
| `dbc-gate` | success |
| `prose-lint` | success |
| `shared-audit` | success |
| `downstream-consumer` | success |
| `consumer-release` | success |
| `docs` | skipped: the deploy runs on the master push |

The coverage job's gate read:

```
lines.......: 100.0% (12946 of 12946 lines)
coverage-gate: branch shortfall 18 rows, within the D-14 recorded
              allowance of 19 (accepted classes)
```

The run's two host facts stand recorded beside the feature: the
runner states two online cpus, so the run scenarios state the
synthetic layout's cpus through the online-set seam and the rows
carry the pin's own account (FR-005, FR-017), and the rocky job's
container states the capability the bind's `set_mempolicy` call
needs, so the recorded node-planned example folds there (FR-013,
FR-022).
