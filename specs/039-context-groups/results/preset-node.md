# The `node` grouping over two installed nodes and one host node

- Unit: specs/039-context-groups T035, the `node` grouping recorded in
  its two kinds (FR-038, SC-002).
- Runs: one seam run whose test installs two memory nodes, beside the
  host run over the one memory node this host states.
- Host: `gary`. Its own memory read answers one node: the file
  `/sys/devices/system/node/node0/cpulist` names the cpus `0-31`, so
  every cpu of the host stands in one node.
- Tree: `build-w7a`, the release developer-mode tree the driver reads,
  built from this branch's sources.
- Driver: [run_groups.sh](./run_groups.sh). Its transcript for these two
  runs stands at `reports/preset-node.log`, and the pattern `*.log`
  stands ignored at `.gitignore:35`, so each run below carries the three
  transcript lines the driver wrote for it.
- Re-read: both commands ran again beside this record and answered the
  same topology line, the same context line and the same context
  records; the timing columns of the row move between runs.

## The seam run over the installed two-node shape

A seam run, the named `ctest` invocation whose test installs a shape no
lane host states.

```
$ ctest --test-dir build-w7a -R '^harness_group_run_test$' --verbose
exit 0
the bytes stand in specs/039-context-groups/results/reports/preset-node.seam-ctest.txt
```

The driver spells that command line through `printf %q`, which escapes
the two anchors; the block above carries the same command in a form a
reader can paste back, beside the exit code and the capture path as the
transcript spells them (FR-038).

The console block, whole from
`reports/preset-node.seam-ctest.txt`:

```
UpdateCTestConfiguration  from :/home/archerc/code/speedgun-ng/build-w7a/DartConfiguration.tcl
Parse Config file:/home/archerc/code/speedgun-ng/build-w7a/DartConfiguration.tcl
Test project /home/archerc/code/speedgun-ng/build-w7a
Constructing a list of tests
Done constructing a list of tests
Updating test list for fixtures
Added 0 tests to meet fixture requirements
Checking test dependency graph...
Checking test dependency graph end
test 97
    Start 97: harness_group_run_test

97: Test command: /home/archerc/code/speedgun-ng/build-w7a/test/harness_group_run_test
97: Working Directory: /home/archerc/code/speedgun-ng/build-w7a/test
97: Test timeout computed to be: 300
97: harness_group_run_test core-four-groups: ok
97: harness_group_run_test node-two-nodes: ok
97: harness_group_run_test one-group: ok
97: harness_group_run_test l3-single-groups: ok
97: harness_group_run_test single-context: ok
97: harness_group_run_test ungrouped: ok
97: harness_group_run_test file-four-groups: ok
97: harness_group_run_test file-defaults: ok
97: harness_group_run_test file-preset-pace: ok
97: harness_group_run_test file-refused: ok
97: harness_group_run_test setup-failure: ok
97: harness_group_run_test teardown-order: ok
97: harness_group_run_test: ok
1/1 Test #97: harness_group_run_test ...........   Passed    0.09 sec

The following tests passed:
	harness_group_run_test

100% tests passed out of 1

Label Time Summary:
quick    =   0.72 sec*proc (1 test)

Total Test time (real) =   0.09 sec
```

The named test: `harness_group_run_test`, scenario `node-two-nodes`,
whole at `test/source/harness_group_run_test.cpp:436-456`.

The installed shape: `machineShape(4, 2, 2, 2)` at
`test/source/harness_group_run_test.cpp:438`, four L3 groups of two
two-thread cores over the cpus `0-15`, beside two memory nodes that
split that range in equal slices: node 0 holds the cpus `0-7` and node
1 holds the cpus `8-15`. The builder states itself the miniature of the
standing two-node seam shape `fourGroupShape` at
`test/source/harness_cli_placement_test.cpp:260-297`, the four-group,
24-core, 48-thread layout whose node 0 holds the cpus `0-23` and whose
node 1 holds the cpus `24-47`. No lane host carries either shape.

The scenario registers four contexts under `placement("node")` at
`test/source/harness_group_run_test.cpp:442` and asserts the names its
contexts disclosed, whole at
`test/source/harness_group_run_test.cpp:447-448`:

```cpp
  const std::vector<std::string> expected {
      "node-0", "node-1", "node-0", "node-1"};
```

The memberships alternate: contexts 0 and 2 stand in `node-0` and
contexts 1 and 3 stand in `node-1`, two groups of two, the `node` arm
cycling the two nodes the plan names (D-3, FR-006). Each name carries
the node number the read states (D-4, FR-008). The capture's line
`97: harness_group_run_test node-two-nodes: ok` is that scenario's own
assertion holding, line 17 of the capture (SC-002).

The run measured its row over those two groups, so each group's start
round and stop round released at its own count of two (FR-022, FR-024).

## The host run beside it: one node names one group

A host run, the CLI over the shape this host states.

```
$ build-w7a/example/docs_threads --placement=node --threads=4 --iterations=1 --no-percentiles --json=specs/039-context-groups/results/reports/preset-node.host.json
exit 0
the bytes stand in specs/039-context-groups/results/reports/preset-node.host.console.txt
```

The console block, whole from
`reports/preset-node.host.console.txt`:

```
speedgun-ng 0.26.0 build=Release
topology: packages=1 l3-groups=2 cores=16 threads=32
bmSharded/threads:4      iterations=         4  real time/iter (ns)= 1032.500  cpu time/iter (ns)=  722.500  overhead floor (ns)=   90.000  endpoint samples=8
  contexts=4  ctx0=SPIN/0@0@0/applied/node-0  ctx1=SPIN/1@0@0/applied/node-0  ctx2=SPIN/2@0@0/applied/node-0  ctx3=SPIN/3@0@0/applied/node-0
```

The bytes disclose:

- One group over four contexts: every context names `node-0`, the host's
  one memory node holding every context of the run (FR-006).
- Each placement text names the cpu, the L3 ordinal and the node:
  `0@0@0`, `1@0@0`, `2@0@0` and `3@0@0`, and the group segment spells
  that node as a name (D-4, FR-008, FR-028).
- The partition holds one group, so the run's rounds carry the
  participant set the standing shape carries (D-9).
- The machine line prints, because the fit step reports a preset in
  effect (FR-028).

The report: `reports/preset-node.host.json`, the document that same
command wrote at exit 0. It carries four context records, each naming
`node-0` in its `group` key (D-10, FR-029).

The two runs carry SC-002's claim and its one-node case: the preset
groups by the memory node the plan names, so two nodes give two named
groups and one node gives one.
