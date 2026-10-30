# The layout run, one file over eight contexts

- Unit: specs/039-context-groups T036, the layout route recorded end to end
  in its two kinds (FR-038, SC-005).
- Runs: one host run, the CLI over a file whose slots name context
  positions, so the document reads the same on any host and the lane host
  carries it; and one seam run whose test installs a four-group shape no
  lane host states.
- Host: `gary`. Its own L3 read answers two groups, the cpus
  `0-7,16-23` and the cpus `8-15,24-31`; 16 cores, 32 threads, and one
  memory node covering the cpus `0-31`. The four groups the file names
  stand over the run's eight context positions, so this host's two units
  carry no weight in the run's partition.
- Tree: `build-w7a`, the release developer-mode tree the driver reads,
  built from this branch's sources.
- Driver: [run_groups.sh](./run_groups.sh). Its transcript for these two
  runs stands at `reports/layout.log`, and the pattern `*.log` stands
  ignored at `.gitignore:35`, so each run below carries the three
  transcript lines the driver wrote for it. That transcript holds no
  `finding:` line, so every reading the script states matched, and the
  pass answered exit 0.
- Re-read: the group count over the report and the named reader test ran
  again beside this record and answered the same bytes, the timing
  figures of the `ctest` summary moving between reads. A pass of the
  driver rewrites every capture in place, so the committed bytes stand as
  the recorded pass left them.
- Re-run: `bash specs/039-context-groups/results/run_groups.sh`, exit 0
  on a tree built from these sources (T033).

## The file the run reads

The driver writes the document into `reports/fixtures/four-groups.yml` at
the top of every pass, so the bytes that ran stand committed beside this
record. Whole from that file:

```yaml
groups:
  - name: first
    slots: [0, 1]
  - name: second
    slots: [2, 3]
  - name: third
    slots: [4, 5]
  - name: fourth
    slots: [6, 7]
```

- One root key `groups` over four entries, each entry a mapping with a
  `name` and one `slots` sequence, the grammar FR-012 fixes (FR-012).
- Each `slots` member names a context position of the eight contexts this
  run states, and the four entries cover positions `0` to `7` exactly
  once (FR-013).
- The four names stand inside the characters and the length FR-015
  allows, and each reads once (FR-015).
- The document names no cpu. A slots layout pins nothing, which is why
  these four entries over these eight contexts stand on any host, the
  lane host included, beside the two L3 groups this host states
  (D-6, FR-013).

## The host run

A host run, the CLI over the shape this host states.

```
$ build-w7a/example/docs_threads --threads=8 --iterations=1 --no-percentiles --group-layout=specs/039-context-groups/results/reports/fixtures/four-groups.yml --json=specs/039-context-groups/results/reports/layout.host.json
exit 0
the bytes stand in specs/039-context-groups/results/reports/layout.host.console.txt
```

The console block, whole from `reports/layout.host.console.txt`:

```
speedgun-ng 0.26.0 build=Release
bmSharded/threads:8      iterations=         8  real time/iter (ns)=  876.250  cpu time/iter (ns)=  625.000  overhead floor (ns)=   90.000  endpoint samples=16
  contexts=8  ctx0=SPIN/NO_PIN/first  ctx1=SPIN/NO_PIN/first  ctx2=SPIN/NO_PIN/second  ctx3=SPIN/NO_PIN/second  ctx4=SPIN/NO_PIN/third  ctx5=SPIN/NO_PIN/third  ctx6=SPIN/NO_PIN/fourth  ctx7=SPIN/NO_PIN/fourth
```

The bytes disclose:

- The handle states no placement (`example/docs/threads.cpp:13`) and
  `--threads=8` states the context count, so the file is this run's one
  partition source (FR-003, FR-011).
- The disclosure line names the group as the last segment of each
  context, after the wait mode and the placement text, and the standing
  segments keep their order (FR-028).
- The groups are the file's four, in the file's order, two contexts
  each: `first` over contexts 0 and 1, `second` over 2 and 3, `third`
  over 4 and 5, `fourth` over 6 and 7 (FR-002, FR-003).
- The capture holds three lines and no machine line. The file names no
  preset, so the fit step reports none in effect and the machine line
  keeps its one printing rule (FR-028).
- The placement text reads `NO_PIN` and the applied flag reads `false`
  for every context, so the run pinned no cpu and grouped by the file
  alone (D-6).

The report: `reports/layout.host.json`, the document that same command
wrote at exit 0. It carries `schemaVersion` `1.10` and
`speedgun.version` `0.26.0`, the values FR-032 lands, and eight context
records whose `group` keys name the file's four names twice each, which
is the group set the console printed (D-10, FR-029). The context array,
whole from the document:

```json
"contexts":[{"waitMode":"SPIN","placement":"NO_PIN","applied":false,"group":"first"},{"waitMode":"SPIN","placement":"NO_PIN","applied":false,"group":"first"},{"waitMode":"SPIN","placement":"NO_PIN","applied":false,"group":"second"},{"waitMode":"SPIN","placement":"NO_PIN","applied":false,"group":"second"},{"waitMode":"SPIN","placement":"NO_PIN","applied":false,"group":"third"},{"waitMode":"SPIN","placement":"NO_PIN","applied":false,"group":"third"},{"waitMode":"SPIN","placement":"NO_PIN","applied":false,"group":"fourth"},{"waitMode":"SPIN","placement":"NO_PIN","applied":false,"group":"fourth"}]
```

A count over the same bytes:

```
$ grep -o '"group":"[a-z]*"' specs/039-context-groups/results/reports/layout.host.json | sort | uniq -c
      2 "group":"first"
      2 "group":"fourth"
      2 "group":"second"
      2 "group":"third"
```

## The seam run

A seam run, the named `ctest` invocation whose test installs a shape no
lane host states.

```
$ ctest --test-dir build-w7a -R '^harness_group_run_test$' --verbose
exit 0
the bytes stand in specs/039-context-groups/results/reports/layout.seam-ctest.txt
```

The driver spells that command line through `printf %q`, which escapes
the two anchors; the block above carries the same command in a form a
reader can paste back, beside the exit code and the capture path as the
transcript spells them (FR-038).

The capture, whole from `reports/layout.seam-ctest.txt`:

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
quick    =   0.73 sec*proc (1 test)

Total Test time (real) =   0.09 sec
```

The named test: `harness_group_run_test`, scenario `file-four-groups`,
whole at `test/source/harness_group_run_test.cpp:549-578`.

The installed shape: `machineShape(4, 2, 1, 0)` at
`test/source/harness_group_run_test.cpp:551`, four L3 groups of two
single-thread cores over the cpus `0-7` and no memory node. No lane host
states that shape (FR-036). The scenario writes a file whose four entries
name `west`, `east`, `north` and `south` over the slots `[0, 4]`,
`[1, 5]`, `[2, 6]` and `[3, 7]`, registers eight contexts with no preset
stated, and runs one repetition.

The partition the scenario asserts, whole at
`test/source/harness_group_run_test.cpp:569-570`:

```cpp
  const std::vector<std::string> expected {
      "west", "east", "north", "south", "west", "east", "north", "south"};
```

- The run measured its row and the eight disclosed names equal that
  list, so the file's groups, names and memberships reached the run
  (FR-002, FR-003).
- Four groups of two contexts each paced behind four pairs of rounds,
  every pair released by its own two arrivals (FR-022, FR-024).
- The capture's line `97: harness_group_run_test file-four-groups: ok`
  is that scenario's own assertion holding, line 22 of the capture.

The same invocation runs the binary's other scenarios and the capture
holds their lines; this record reads the lines it names.

## SC-005's three cases

A layout file run names the groups, the names and the memberships the
file states, three cases of three (SC-005).

| case | the run that reads it | what its bytes state |
| --- | --- | --- |
| the explicit-name case | the host run above, beside the seam run's `file-four-groups` over a shape no host states | the host run names the file's four names, `first` to `fourth`, two contexts each in the file's order; the seam run names `west`, `east`, `north`, `south` over the alternating slots `[0, 4]`, `[1, 5]`, `[2, 6]`, `[3, 7]` |
| the default-name case | the seam run's `file-defaults`, the capture's line `97: harness_group_run_test file-defaults: ok` | the two entries that name no `name` carry `g0` and `g1`, `g` beside the zero-based document index (FR-015, D-4) |
| the one-group case | `harness_group_layout_test`, case `one-group-every-context` at `test/source/harness_group_layout_test.cpp:219-223` | one entry named `all` over every context answers one group holding all of them (D-9) |

The one-group case stands at the reader, the level where a one-entry
document is decided, and the named test read green beside this record:

```
$ ctest --test-dir build-w7a -R '^harness_group_layout_test$' --output-on-failure
Test project /home/archerc/code/speedgun-ng/build-w7a
    Start 94: harness_group_layout_test
1/1 Test #94: harness_group_layout_test ........   Passed    0.01 sec

100% tests passed out of 1

Label Time Summary:
quick    =   0.01 sec*proc (1 test)

Total Test time (real) =   0.01 sec
```

The capture's `97: harness_group_run_test one-group: ok` line is the same
one-group shape through the other source, a preset whose four contexts
all land in one L3 unit (D-9).
