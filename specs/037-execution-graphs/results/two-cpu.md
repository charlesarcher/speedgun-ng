# The two-cpu suite run

- Head: `2e53b02`, the feature head the run stands on.
- Boot: 2026-10-26 06:24:40 CDT, kernel `Linux 7.3.0-3-cachyos x86_64`,
  release CachyOS. The acceptance record names the same boot.
- Host: 32 cpus online. The run's mask names cpus `0,1`, so every test
  of the suite runs on a two-cpu host (FR-027, SC-008).
- Build: the shared `build/` tree on the `ci-rocky` release developer
  mode configuration, built at the head, exit 0, no reconfigure.
- Command: the suite pinned to cpus `0,1`, taken under
  `/tmp/speedgun_build.lock`. The block below names the command.

## The pinned run

The command runs from the repository root over the built tree. The tail
of the log, verbatim:

```
$ taskset -c 0,1 ctest --test-dir build --output-on-failure
Test project /home/archerc/code/speedgun-ng/build
        Start   1: speedgun-ng_test
  1/151 Test   #1: speedgun-ng_test ...............................   Passed    0.00 sec
...
151/151 Test #151: report_shape_execution_graphs_sample ...........   Passed    1.21 sec

100% tests passed out of 151

Label Time Summary:
quick    = 108.47 sec*proc (144 tests)
slow     = 392.88 sec*proc (7 tests)

Total Test time (real) = 279.91 sec
```

The count reads 151 of 151, and the log names no failed test.

## The graph arms

The graph units run on the pair, each in the suite run above:

```
133/151 Test #133: harness_graph_model_test .......................   Passed    0.00 sec
147/151 Test #147: harness_graph_file_test ........................   Passed    0.01 sec
148/151 Test #148: harness_graph_gate_test ........................   Passed    0.13 sec
149/151 Test #149: harness_graph_report_test ......................   Passed    0.38 sec
150/151 Test #150: harness_cli_graph_test .........................   Passed    0.14 sec
151/151 Test #151: report_shape_execution_graphs_sample ...........   Passed    1.21 sec
```

Each of the five graph binaries repeats on the pair until a pass stops
the run, and 20 passes each stand:

```
$ taskset -c 0,1 ctest --test-dir build -R 'harness_graph_(model|file|gate|report)_test|harness_cli_graph_test' --repeat until-fail:20
5/5 Test #150: harness_cli_graph_test ...........   Passed    0.12 sec

100% tests passed out of 5

Total Test time (real) =  12.08 sec
```

The progress story of the graph gate reads on the pair. The chain and
the diamond run over the two cpus (`suitePassesOnTwoCpus`), the node
count follows the cpus the online-set seam installs
(`onlineChainGraph` reads `onlineCpuSet().size()`), and the gate sleeps
on the condition variable, so a three-context chain finishes on two
cpus (FR-049).

## The clock and sleep rule

The gate of the time-source scan runs on the pair, and the scan of the
graph units finds no sleep call and no clock read:

```
$ taskset -c 0,1 ctest --test-dir build -R time_source_gate
1/1 Test #60: time_source_gate .................   Passed    0.10 sec

100% tests passed out of 1

$ grep -n 'sleep_for\|usleep\|nanosleep\|steady_clock\|system_clock\|high_resolution_clock\|clock_gettime' test/source/harness_graph_*.cpp test/source/harness_cli_graph_test.cpp
(no output)
```

## The shape fold

The run at `29e96f4`, and the run before it at `7b5a157`, read 150 of
151. One arm failed, and it named the line the recorded block omitted:

```
151/151 Test #151: report_shape_execution_graphs_sample ...........***Failed    1.87 sec
sample output block holds 4 lines, the run printed 5
line 5: the block omits this noise warning line: 'warning: spinning contexts oversubscribed (3 spin, 2 cpus)'
```

The class of that line is the host's answer. `bmChain` registers three
SPIN contexts, so FR-026 fires the oversubscription warning on a host
that gives the run fewer cpus than it has spinning contexts
(`json_report.cpp:833`, printed at `report.cpp:241`). The recorded
`example/docs/execution_graphs.expected` block stands on a wide host and
carries no warning, and the shape fold compares line for line.
`2e53b02` moved the shape `warning: spinning contexts oversubscribed
(# spin, # cpus)` into `HOST_CONDITION` in `tools/docs/report_shape.py`,
beside the other counts the cpus the host gives the run set (specs/033
FR-042). The fixture pair and the specs/020 disclosure contract moved
with it. The fold of the same arm now passes on the pair, and the eight
other sample folds keep their verdicts:

```
$ taskset -c 0,1 ctest --test-dir build -R report_shape
9/9 Test #151: report_shape_execution_graphs_sample ...   Passed    1.22 sec

100% tests passed out of 9
```

## The gap

No gap: the suite reads 151 of 151 under the two-cpu mask at `2e53b02`,
the five graph arms pass 20 repeats each on the pair, `time_source_gate`
stands green, no test sleeps, no test reads a clock, and no test carries
a fixed node count (FR-026, FR-027, SC-008).
