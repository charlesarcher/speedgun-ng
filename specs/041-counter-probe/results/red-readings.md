# Red readings, and the green that followed

Every unit of this lane reads red at `858c55d9` before the code that turns it
green (FR-070). The readings below name the command and the answer.

## M1, the group probe (T010, T012)

The unit at `test/source/counters_group_probe_test.cpp` was written first, and the
build of its target at the base answers:

```text
/home/archerc/code/speedgun-ng/test/source/counters_group_probe_test.cpp:32:10: fatal error: ../source/counters/detail/group_probe.hpp: No such file or directory
```

The red reading is the missing declaration of
`source/counters/detail/group_probe.hpp`, which is the file T011 writes. The
implementation held aside for the reading, and the same command after it landed:

```text
$ cmake --build build --target counters_group_probe_test
[100%] Built target counters_group_probe_test
$ ctest --test-dir build -R counters_group_probe_test --output-on-failure
1/1 Test #51: counters_group_probe_test ........   Passed    0.00 sec
```

Every scenario drives the seam, so the run holds 0 real `perf_event_open` calls
(FR-073) and costs no more than a few hundred scripted opens.

| Scenario | Requirement the check strings name |
|---|---|
| `runsShortAtSevenAnswersSix` | FR-014, FR-015, F-1, F-5 |
| `refusedOpenAnswersTheSizeBefore` | FR-012, FR-015 |
| `permissionRefusalAnswersNothing` (EACCES, EPERM, EINVAL) | FR-018 |
| `shortReadAnswersNothing` | FR-014, FR-015 |
| `sizeOneRunsShortAnswersNothing` | FR-015, FR-017 |
| `everySizeFittingAnswersNothing` | FR-016, D-4 |
| `groupFdNamesTheLeader` | FR-012 |
| `attributeCarriesTheMeasuredShape` | FR-010, FR-013, FR-014, D-9 |

The last three of the six `GroupProbeStatus` values, and the two `errno` arms of
FR-018, each reach a case above, so the enum and the refusal test carry no arm
the suite never runs.

## M2, the device rule and the event row (T020, T021)

The scenarios at `test/source/counters_counter_limits_test.cpp` were written
first, and the build of their unit answers:

```text
counters_counter_limits_test.cpp:43:29: error: 'isProbeCoreDevice' has not been declared in 'sg::counters::detail'
counters_counter_limits_test.cpp:44:29: error: 'probeEventForDevice' has not been declared in 'sg::counters::detail'
```

The two declarations are what T021 adds. After them:

```text
$ ctest --test-dir build -R counters_counter_limits_test --output-on-failure
1/1 Test #63: counters_counter_limits_test .....   Passed    0.00 sec
$ ./build/test/counters_counter_limits_test
counters_counter_limits_test PASS: every decision table row matches
```

`probeDeviceTableScenario` holds ten name rows and `probeEventTableScenario`
holds six vendor rows, so the `armv8_` prefix arm, the empty Arm row of D-5,
and the `OTHER` arm each reach a case with no host (FR-031 to FR-034). The
base rows of the unit stand unchanged (FR-044).

## M4: the pass that seats a device's limit (T040, T041)

Red, read at `b1375f64` with the two new files held out of the tree:

```text
source/counters/detail/limit_fill.hpp: No such file or directory
```

The unit names a pass the tree does not have yet, and `provider.cpp` includes
that header, so the whole counters module stops at the missing declaration.

Green, with `source/counters/detail/limit_fill.hpp`,
`source/counters/linux_pmu/limit_fill.cpp` and the enumerate pass seated on it:

```text
counters_limit_fill_test: PASS
```

Three findings came out of the first green run. One was a defect in the new
pass; two were in the unit's own device names. A pass that seated no figure recorded nothing, so
a record an earlier pass had left on the same path survived, and the reader
named `DECODE` beside an absent figure (FR-040a). The pass now records on every
arm, and a record of `NONE` erases. The other two were the unit's own device
names: one name carries one search for the life of the process (FR-022), so the
row that asks for no search had to run before the rows that settle one.

## M5: the disclosure line and the run record (T043 to T045)

The units were written first. With the five harness files of the module held out
of the tree (`git stash push -- source/harness/cli.cpp source/harness/runner.cpp
source/harness/json_load.cpp source/harness/json_report.cpp
source/harness/detail/json_report.hpp`), the new unit reads red on the reader it
asks for:

```text
test/source/harness_limit_disclosure_test.cpp:349:36: error: 'loadCounterLimits' is not a member of 'sg::detail'
test/source/harness_limit_disclosure_test.cpp:389:36: error: 'loadCounterLimits' is not a member of 'sg::detail'
test/source/harness_limit_disclosure_test.cpp:401:36: error: 'loadCounterLimits' is not a member of 'sg::detail'
```

and the report unit reads red on the three names the writer carries:

```text
test/source/harness_json_report_test.cpp:842:37: error: 'CounterLimitEntry' is not a member of 'sg::detail'
test/source/harness_json_report_test.cpp:852:35: error: 'counterLimitEntries' is not a member of 'sg::detail'
test/source/harness_json_report_test.cpp:853:15: error: 'noteCounterLimit' is not a member of 'sg::detail'
```

Green, with the record seated in `source/harness/json_report.cpp`, its reader in
`source/harness/json_load.cpp`, the disclosure at the limit gathering of
`source/harness/runner.cpp`, and the entry point handing the process table to the
writer at `source/harness/cli.cpp`. The scripted run of the new unit answers:

```text
$ ./build/test/harness_limit_disclosure_test
harness_limit_disclosure_test: ok
$ ./build/test/harness_limit_disclosure_test probed --filter '^bmDisclosure$' \
    --iterations=1 --json=/tmp/probed.json
speedgun-ng 0.32.0 build=Release
counter limit: device 'cpu' count 6 source probe
bmDisclosure             iterations=         1  real time/iter (ns)=  390.000 ...
```

and the document of that same run carries the record whose figure the line names:

```text
"counterLimits":[{"device":"cpu","limit":6,"source":"probe"}]
```

The decoded run of the same scripted device prints no line and writes
`{"device":"cpu","limit":6,"source":"decode"}`; the run with no metric column
prints no line and writes no `counterLimits` key. `loadCounterLimits` answers the
entries of the 1.15 document, no entry for a document older than the minor, and a
refusal naming the pointer for a planted word outside the three, a planted limit
of another kind and a planted device of another kind.

Four findings came out of the readings.

1. The search of FR-011 does not run on the hosts this suite builds on. The
   provider's decode answers a limit for `cpu` on an x86 host whatever the
   scripted kernel grants, `hostCounterLimitRegisters()` reads the real CPUID
   rather than the scripted `cpuidWords`, and `probeEventNameForDevice` answers
   nothing for an `armv8_*` device or a vendor of `OTHER`. No scripted kernel
   turns a needed `cpu` device into a probed one on `ci-ubuntu` or `ci-rocky`.
   The probed scenario therefore seats the finder it is about through
   `recordLimitSource`, the internal recorder M3 ships, beside the figure the
   provider published, over a scripted kernel and a scripted devices root. The
   pair the harness reads is the pair under test, and the printed line and the
   written record are the run's own answer. A platform specification is what
   puts a probed device under a run on this suite.
2. A child that scripts only the PMU provider ends with no time source:
   `speedgunMain` registers the clock provider and stops when the refusal leaves
   the run with no `machine/monotonic` leaf (`source/harness/cli.cpp:678-687`).
   The scripted scenario registers the clock provider first, the way that entry
   stands it, and the entry point's own registration is then the refused pass
   R-03 states.
3. A scripted run inside a test process seats its own device in the table the
   entry point reads at write time, because the runner's limit gathering is the
   table's writer. The report unit's seating check weighs the seating against
   what an earlier scripted run of the same binary left behind.
4. The minor move reads more pinned text than the plan named. Fourteen readings
   across the seven units the plan lists state `1.14`, and the top-level
   `x-order` string stands pinned in two units of that list besides
   (`harness_noise_report_test.cpp`, and `harness_graph_report_test.cpp`, which
   the plan does not name at all). Both now carry `counterLimits` between
   `graph` and `benchmarks`, and the suite is green with them.

The record's `required` list stands inside the array's item, beside the item's
`x-order` and `additionalProperties`, the shape every closed item of the schema
file keeps (`periods`, `files`, `refused`). The top-level `required` list carries
three keys and gains no fourth, which is the reading FR-051's sentence "no key of
the record SHALL stand in `required`" takes with the file as it stands.

The whole suite, read after the module landed:

```text
$ cmake --build build
$ ctest --test-dir build --output-on-failure
100% tests passed out of 185

The following tests did not run:
	183 - harness_topdown_run_test (Skipped)
```

The skip is the base skip of that entry (its `SKIP_RETURN_CODE 2` row), read the
same way on the base tree. `dbc-gate` answers 348 interfaces with 0 gaps on both
its halves, the base figure: the module changed no file under `include/`, so the
harness `SOVERSION` stays 21 and the exported interface set stands unchanged
(FR-052, FR-053).

The page seat for the record's keys is "The shape of the document" of
`docs/pages/harness.md`: that section held prose only, with no key rows beside
the version rule, so the record joins the page there with a `| Key | Kind |
Meaning |` table in the shape the graph record's table uses, and the version rule
states the minor `1.15`. The console line stands in "Hardware counter metrics",
where the device's limit is the fact under discussion.

## M5: the record and the line that names a finder (T043 to T045)

Red, read at the head with `source/harness` and `docs/schema` held at their base
state and the unit in the tree:

```text
test/source/counters_limit_fill_test.cpp: the pass of M4 stands, unchanged
test/source/harness_limit_disclosure_test.cpp:349:36: error: 'loadCounterLimits'
  is not a member of 'sg::detail'
```

The unit names a reader the harness does not have yet, so the disclosure unit
stops at the missing declaration while the counters side stays green.

Green, with the accumulator of `detail::json_report.hpp`, the writer's
`counterLimits` array, the loader's reader and the runner's seat:

```text
harness_limit_disclosure_test: ok
```

## M6: the example (T050, T051)

An example carries no unit of its own, so its red reading is its registration: at
the base the target does not exist.

```text
No rule to make target 'docs_counter_probe'
```

Green, on the acceptance host:

```text
speedgun-ng 0.32.0 build=Release
bmProbeTick              iterations=3915069546  real time/iter (ns)=    0.178  ...
counter limit: cpu limit=6 source=decode probe=6
```

The last line is SC-003's equality on this host: the decode answered 6 and the
example's own search answered 6, and `results/requirement-1-shipped-shape.txt`
reads the same bound through the attribute the shipped probe composes.

## T013: the probe unit under the two-cpu mask and under the sanitizer tree

```text
taskset -c 0,1 ctest --test-dir build -R 'counters_group_probe_test|counters_limit_fill_test|counters_limit_source_test'
  counters_group_probe_test .... Passed    counters_limit_source_test .... Passed
  counters_limit_fill_test ..... Passed    100% tests passed out of 3
taskset -c 0,1 ctest --test-dir build/sanitize -R counters_group_probe_test
  counters_group_probe_test .... Passed    100% tests passed out of 1
```

The masked release row answers all three probe units, and the same unit passes with
Address and Undefined sanitizers on and the runner held to two processors. The whole
sanitizer suite reads 100 percent of 180 with no report line in `results/sanitizers.md`,
which is where the entry counts of the sanitizer trees are explained.
