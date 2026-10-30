# The preset grouping over a multi-group shape

- Unit: specs/039-context-groups T034, the preset grouping recorded in
  its two kinds (FR-038, SC-001).
- Runs: two host runs over the shape this host states, and one seam run
  whose test installs a shape no lane host states.
- Host: `gary`. Its own L3 read answers two groups, the cpus
  `0-7,16-23` and the cpus `8-15,24-31`; 16 cores, 32 threads, and one
  memory node covering the cpus `0-31`.
- Tree: `build-w7a`, the release developer-mode tree the driver reads,
  built from this branch's sources.
- Driver: [run_groups.sh](./run_groups.sh). Its transcript for these
  three runs stands at `reports/preset-core.log`, and the pattern
  `*.log` stands ignored at `.gitignore:35`, so each run below carries
  the three transcript lines the driver wrote for it.
- Re-read: the three commands ran again beside this record and answered
  the same topology line, the same context line and the same context
  records; the timing columns of the row move between runs.

## The host run: `--placement=core` over sixteen contexts

A host run, the CLI over the shape this host states.

```
$ build-w7a/example/docs_threads --placement=core --threads=16 --iterations=1 --no-percentiles --json=specs/039-context-groups/results/reports/preset-core.core16.json
exit 0
the bytes stand in specs/039-context-groups/results/reports/preset-core.core16.console.txt
```

The console block, whole from
`reports/preset-core.core16.console.txt`:

```
speedgun-ng 0.26.0 build=Release
topology: packages=1 l3-groups=2 cores=16 threads=32
bmSharded/threads:16     iterations=        16  real time/iter (ns)= 1165.000  cpu time/iter (ns)=  799.375  overhead floor (ns)=  190.000  endpoint samples=32
  contexts=16  ctx0=SPIN/0@0/applied/l3-0  ctx1=SPIN/1@0/applied/l3-0  ctx2=SPIN/2@0/applied/l3-0  ctx3=SPIN/3@0/applied/l3-0  ctx4=SPIN/4@0/applied/l3-0  ctx5=SPIN/5@0/applied/l3-0  ctx6=SPIN/6@0/applied/l3-0  ctx7=SPIN/7@0/applied/l3-0  ctx8=SPIN/8@1/applied/l3-1  ctx9=SPIN/9@1/applied/l3-1  ctx10=SPIN/10@1/applied/l3-1  ctx11=SPIN/11@1/applied/l3-1  ctx12=SPIN/12@1/applied/l3-1  ctx13=SPIN/13@1/applied/l3-1  ctx14=SPIN/14@1/applied/l3-1  ctx15=SPIN/15@1/applied/l3-1
```

The bytes disclose:

- Two groups over sixteen contexts: `l3-0` holds contexts 0 to 7 and
  `l3-1` holds contexts 8 to 15, two groups of eight beside the host's
  two L3 groups, every context standing in exactly one of them
  (FR-002, FR-006).
- The `core` arm seats one context per core, and each of this host's L3
  groups holds eight cores, so the arm's groups hold eight contexts
  each (D-3).
- Each name carries the unit and the ordinal the read states, the
  `l3-<ordinal>` form a preset's names take (D-4, FR-008).
- Each context reads as wait mode, placement, applied flag and group:
  the group segment stands last and the standing segments keep their
  order (FR-028).
- The machine line prints, because the fit step reports a preset in
  effect (FR-028).

The report: `reports/preset-core.core16.json`, the document that same
command wrote at exit 0. It carries `schemaVersion` `1.10` and
`speedgun.version` `0.26.0`, the values FR-032 lands, and sixteen
context records whose `group` keys name `l3-0` eight times and `l3-1`
eight times (D-10, FR-029).

## The host run: `--placement=l3` over two contexts

A host run over the same shape, the arm that seats one context per L3
unit (`source/harness/placement_preset.cpp:127-131`), so two contexts
fill the host's two units.

```
$ build-w7a/example/docs_threads --placement=l3 --threads=2 --iterations=1 --no-percentiles --json=specs/039-context-groups/results/reports/preset-core.l3two.json
exit 0
the bytes stand in specs/039-context-groups/results/reports/preset-core.l3two.console.txt
```

The console block, whole from
`reports/preset-core.l3two.console.txt`:

```
speedgun-ng 0.26.0 build=Release
topology: packages=1 l3-groups=2 cores=16 threads=32
bmSharded/threads:2      iterations=         2  real time/iter (ns)=  640.000  cpu time/iter (ns)=  385.000  overhead floor (ns)=   90.000  endpoint samples=4
  contexts=2  ctx0=SPIN/0@0/applied/l3-0  ctx1=SPIN/8@1/applied/l3-1
```

The bytes disclose:

- Two groups of one context each: `l3-0` over context 0 and `l3-1` over
  context 1, the single-member shape the `l3` arm takes (D-3).
- The placement text names the cpu and the group ordinal, `0@0` and
  `8@1`, and the group segment spells that same unit as a name
  (FR-008, FR-028).

The report: `reports/preset-core.l3two.json`, the document that same
command wrote at exit 0. It carries two context records whose `group`
keys name `l3-0` and `l3-1` (D-10, FR-029).

## The seam run: four groups of two over an installed shape

A seam run, the named `ctest` invocation whose test installs a shape no
lane host states.

```
$ ctest --test-dir build-w7a -R '^harness_group_run_test$' --verbose
exit 0
the bytes stand in specs/039-context-groups/results/reports/preset-core.seam-ctest.txt
```

The driver spells that command line through `printf %q`, which escapes
the two anchors; the block above carries the same command in a form a
reader can paste back, beside the exit code and the capture path as the
transcript spells them (FR-038).

The console block, whole from
`reports/preset-core.seam-ctest.txt`:

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
1/1 Test #97: harness_group_run_test ...........   Passed    0.10 sec

The following tests passed:
	harness_group_run_test

100% tests passed out of 1

Label Time Summary:
quick    =   0.77 sec*proc (1 test)

Total Test time (real) =   0.10 sec
```

The named test: `harness_group_run_test`, scenario `core-four-groups`,
whole at `test/source/harness_group_run_test.cpp:406-431`.

The installed shape: `machineShape(4, 2, 1, 0)` at
`test/source/harness_group_run_test.cpp:408`, four L3 groups of two
single-thread cores over the cpus `0-7` and no memory node. No lane
host states that shape. The scenario registers eight contexts under
`placement("core")` at `test/source/harness_group_run_test.cpp:411` and
runs two repetitions.

The partition the scenario asserts, whole at
`test/source/harness_group_run_test.cpp:422-423`:

```cpp
  const std::vector<std::string> expected {
      "l3-0", "l3-0", "l3-1", "l3-1", "l3-2", "l3-2", "l3-3", "l3-3"};
```

- Four groups named `l3-0` to `l3-3`, two contexts each: SC-001.
- Each group's rounds carry two participants and the run measured both
  of its repetitions, so each group's start round and stop round
  released at its own count (FR-022, FR-024).
- The capture's line `97: harness_group_run_test core-four-groups: ok`
  is that scenario's own assertion holding, line 16 of the capture.

The same invocation runs the binary's other scenarios and the capture
holds their lines; this record reads the one line it names.
