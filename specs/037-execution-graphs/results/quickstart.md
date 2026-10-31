# The quickstart runs on the lane host

The scenarios of [quickstart.md](../quickstart.md), run end to end at the
feature head: each command, its exit code, and the lines it answered
(SC-001 to SC-010, X.4 of the constitution).

- Head: `15a4954`, the feature head the runs stand on. The interrupt run
  of the stop path stands at `66eca8d`, the record commit that landed
  the runs above.
- Boot: 2026-10-30 10:18 CDT, kernel `Linux 7.3.0-3-cachyos x86_64`,
  release CachyOS. The acceptance record names the same boot.
- Host: the harness listing reads `cores=16 threads=32` on 1 package and
  2 L3 groups, mask `0-31`: the 16-core, 32-thread lane shape the
  quickstart expects, beside the 24-core, 48-thread record of specs/035.
- Build: the shared `build/` tree on the `ci-rocky` release developer
  mode configuration, built at the head, exit 0, no reconfigure.
- Lock: every command below runs one at a time under
  `flock /tmp/speedgun_build.lock -c`.
- Suite: `ctest --test-dir build --output-on-failure` reads 151 of 151,
  exit 0, 272.00 sec real.

## The graph file

The chain the quickstart names stands at
`/home/archerc/.hermes/cache/scratch/speedgun-quickstart-t048/stages.json`, the file's own bytes:

```json
{
  "nodes": [
    { "name": "a", "slot": 0, "deps": [] },
    { "name": "b", "slot": 1, "deps": ["a"] },
    { "name": "c", "slot": 2, "deps": ["b"] }
  ]
}
```

The example registers `bmChain` with `threads(3)`, so the three nodes
match the run's three contexts (FR-012). The scratch path stands in the
table and the blocks below as `<scratch>`.

## The scenarios

| Scenario | Command | Exit | The lines it answers |
| --- | --- | --- | --- |
| The chain orders the starts | `./build/example/docs_execution_graphs --graph=<scratch>/stages.json` | 0 | `  graph nodes=a,b,c edges=a->b,b->c` under the `contexts=3` line |
| The record carries the run | the same run with `--json=<scratch>/report.json` | 0 | the `graph` record names `a`, `b`, `c` at slots 0, 1, 2 each `finished`, the two edges, and two empty arrays, ahead of `benchmarks` |
| The read-back holds | `ctest --test-dir build -R harness_graph_report_test` | 0 | `loadReport` folds the written record against the run's facts |
| A cycle refuses | the example binary over a two-node file that names each node, and the suite arm `cycleRefuses` | 2 | one line naming the cycle, no context starts |
| A rejected file refuses | the example binary over a truncated file, and the suite arm `parserRejectRefuses` | 2 | one line naming the file and the parser's fault, no context starts |
| A count mismatch refuses | the example binary with `--threads=2` over the three-node file, and the suite arm `countMismatchRefuses` | 2 | one line naming the counts, no context starts |
| A failed node holds its dependents | the suite arm `failedRunCarriesHeldRecord` | 0 | the child answers the standing run-failure status 1, prints its `FAILED` line, and the record names each held node with `a` as its reason |
| The graph-free bytes stand | `ctest --test-dir build -R harness_graph_report_test` | 0 | the arm `noGraphBytesMatchMaster` folds the document and the console against master's bytes |
| The example folds | `ctest --test-dir build -R report_shape` | 0 | nine folds, each recorded block equal to the binary's own run |

## The chain run and the record run

The chain run prints the block under the run's disclosure line, the nodes
in the ascending slot order FR-023 fixes and the edges in the file's
order:

```
$ ./build/example/docs_execution_graphs --graph=<scratch>/stages.json
speedgun-ng 0.25.0 build=Release
bmChain/threads:3        iterations=1803161838  real time/iter (ns)=    0.387  cpu time/iter (ns)=    0.387  overhead floor (ns)=  110.000  endpoint samples=6
  contexts=3  ctx0=SPIN/NO_PIN  ctx1=SPIN/NO_PIN  ctx2=SPIN/NO_PIN
  graph nodes=a,b,c edges=a->b,b->c
exit 0
```

The record run adds the document. The console answers the same four
lines, and the document's graph object reads:

```
$ ./build/example/docs_execution_graphs --graph=<scratch>/stages.json --json=<scratch>/report.json
exit 0

"graph":{"runs":[{"benchmark":"bmChain/threads:3","nodes":[{"name":"a","slot":0,"outcome":"finished"},{"name":"b","slot":1,"outcome":"finished"},{"name":"c","slot":2,"outcome":"finished"}],"edges":[{"from":"a","to":"b"},{"from":"b","to":"c"}],"held":[],"stopped":[]}]}
```

The written document's top-level key order reads `schemaVersion`,
`speedgun`, `graph`, `benchmarks`. The `precheck` key stands only for a
run that carries a pre-check, and the graph object stands after it and
before `benchmarks`, the placement the schema's `x-order` states. The
schema's own order reads `schemaVersion`, `speedgun`, `run`, `jitter`,
`precheck`, `graph`, `benchmarks`, `hunt`, and the record arm reads the
order with a pre-check present.

## The read-back, the byte-identity fold and the example fold

The record arm and the byte arm run in the report test, and the shape
fold runs the example binary against its recorded block:

```
$ ctest --test-dir build -R harness_graph_report_test
1/1 Test #149: harness_graph_report_test ........   Passed    0.42 sec
100% tests passed out of 1
exit 0

$ ctest --test-dir build -R report_shape
9/9 Test #151: report_shape_execution_graphs_sample ...   Passed    0.87 sec
100% tests passed out of 9
exit 0
```

The report test's ten arms each run in their own child, and each arm
stands driven by name at the head:

```
$ for arm in consoleBlockBesideContextsLine blockOncePerRun \
      recordKeysOrderAndReadBack nodeOrderAscendingSlots \
      reasonNamesFailedNode recordCarriesNoTiming \
      schemaCarriesGraphKeysAndOrder plantedGraphDocumentsFailTheSchema \
      noGraphBytesMatchMaster reportDataGraphMemberRaisesAbi; do
      ./build/test/harness_graph_report_test "$arm"; done
exit 0 for every arm
```

`recordKeysOrderAndReadBack` writes the record and reads it back with
`loadReport`, folding the entries against the run's facts;
`noGraphBytesMatchMaster` masks the `schemaVersion` value and the
`speedgun.version` value and folds the rest against master's document
bytes and master's two console lines. The example's recorded block in
`example/docs/execution_graphs.expected` carries the graph line, and the
fold of `report_shape_execution_graphs_sample` compares it line for line
against the binary's own run.

## The three refusals

Each refusal answers one line on the error stream, exits 2, and starts no
context: the standard output of each run is empty.

```
$ ./build/example/docs_execution_graphs --graph=<scratch>/cycle.json
the graph file '<scratch>/cycle.json': 'x', 'y' form a cycle
exit 2

$ ./build/example/docs_execution_graphs --graph=<scratch>/broken.json
the graph file '<scratch>/broken.json' holds no JSON the parser reads: TAPE_ERROR: The JSON document has an improper structure: missing or superfluous commas, braces, missing keys, etc.  This is a fatal and unrecoverable error.
exit 2

$ ./build/example/docs_execution_graphs --threads=2 --graph=<scratch>/stages.json
the graph file '<scratch>/stages.json': the node 'c' names slot 2 outside the run's 2 contexts
exit 2
```

The suite arms answer the same refusals through planted files, each in
its own process:

```
$ ./build/test/harness_graph_file_test cycleRefuses
exit 0
$ ./build/test/harness_graph_file_test parserRejectRefuses
exit 0
$ ./build/test/harness_cli_graph_test countMismatchRefuses
harness_cli_graph_test: countMismatchRefuses passed
exit 0
```

The count arm's line names both counts:

```
--graph: the graph's 3 nodes do not match the run's 4 contexts
```

## The held run

The scripted chain of three contexts names a body that throws on the
context of slot 0. The run answers the standing run-failure status of
specs/015, status 1, and the words the gate settled ride the record:

```
$ ./build/test/harness_cli_graph_test failedRunCarriesHeldRecord
harness_cli_graph_test: failedRunCarriesHeldRecord passed
exit 0

$ cat /tmp/speedgun-cli-graph-<pid>-failed.out
speedgun-ng 0.25.0 build=Release
bmGraphChainFail/threads:3 FAILED: the scripted graph body failure
graph-order=0,1,2

$ cat /tmp/speedgun-cli-graph-<pid>-failedRecord.json
"graph":{"runs":[{"benchmark":"bmGraphChainFail/threads:3","nodes":[{"name":"a","slot":0,"outcome":"failed"},{"name":"b","slot":1,"outcome":"held"},{"name":"c","slot":2,"outcome":"held"}],"edges":[{"from":"a","to":"b"},{"from":"b","to":"c"}],"held":[{"node":"b","reason":"a"},{"node":"c","reason":"a"}],"stopped":[]}]}
```

The failed attempt prints no row, so it prints no graph block: the block
stands beside a row. The settlement-above-return change of
`source/harness/runner.cpp` reads the gate's words ahead of the
attempt's answer, so the failed attempt's words ride the record channel,
and the document names each held node with the failed node that gates it.

The gate's own arms read the same shape at the gate, and the entry
point's assembly arm reads the record's arrays:

```
$ ./build/test/harness_graph_gate_test failedDependentsHoldWithReason
exit 0
$ ./build/test/harness_graph_gate_test twoFailuresNameTheirGater
exit 0
$ ./build/test/harness_graph_gate_test independentFinishAndStatusFollowsRunFailure
exit 0
$ ./build/test/harness_cli_graph_test recordEntryCarriesHeldAndStopped
harness_cli_graph_test: recordEntryCarriesHeldAndStopped passed
exit 0
```

The run of the mixed graph prints its failed row and the bodies each
context ran, the held context running none:

```
speedgun-ng 0.25.0 build=Release
bmGateMixedFail/threads:3 FAILED: the scripted graph body failure
gate-bodies=1,0,1 contexts=3
```

## The production stop path

The stop request settles every gated node stopped and its dependents
stopped with it, each with no reason (FR-019, FR-023). The gate's arm
drives the arrival between the barrier rounds:

```
$ ./build/test/harness_graph_gate_test stopSettlesStoppedWithDependents
exit 0
```

The words the gate answers are `finished` for the node that started,
`stopped` for the gated node and `stopped` for its dependent, and the
assembly puts a stopped word in the record's `stopped` array with no
reason:

```
$ ./build/test/harness_cli_graph_test recordEntryCarriesHeldAndStopped
exit 0
```

The entry that arm assembles over a settlement of one failed node, one
held node, one stopped node and one finished node reads
`nodes` with the words `failed`, `held`, `stopped`, `finished`,
`held` with the entry `b` gated by `a`, and `stopped` with the name `c`.
The console arm reads the two settlement lines from two prints of the
same chain, each directly after that run's `contexts=` line. The held
line stands in the print of the entry whose first node failed, and that
print carries no stopped line (`harness_graph_report_test.cpp:420`,
`:425`); the stopped line stands in the print of the entry the stop
request settled, and that print carries no held line
(`harness_graph_report_test.cpp:434`, `:438`). The two prints:

```
  graph nodes=top,mid,end edges=top->mid,mid->end
  graph held=mid(top),end(top)

  graph nodes=top,mid,end edges=top->mid,mid->end
  graph stopped=mid,end
```

The production drive of a failing chain, above, answers the words in the
document and prints no block beside no row. The stop request's arrival
at the gate has no caller in the entry point: the grep over the source
tree names the declaration and the definition alone, so no scripted run
of the entry point settles a stopped word, and every scripted run's
record carries an empty `stopped` array.

```
$ grep -rn settleStopped source include
source/harness/detail/graph.hpp:249:  auto settleStopped() -> void;
source/harness/graph_gate.cpp:159:auto settleStoppedAll(const Graph& graph, States& states) -> void
source/harness/graph_gate.cpp:314:auto GraphGate::settleStopped() -> void
source/harness/graph_gate.cpp:318:    settleStoppedAll(m_graph, m_states);
exit 0
```

The interrupt of specs/020 FR-050 stands the production drive of the
same path. The signal arrives two seconds into a chain run of three
contexts over a long iteration count. The run answers status 1 with the
skip line and the one reason, and prints no `contexts=` line, so no
block stands beside it:

```
$ ./build/example/docs_execution_graphs --graph=<scratch>/stages.json --iterations=20000000000 --json=<scratch>/stop-report.json
speedgun-ng 0.25.0 build=Release
bmChain/threads:3        SKIPPED: interrupted by the user (SIGINT)
exit 1

$ cat <scratch>/stop-report.json
"graph":{"runs":[{"benchmark":"bmChain/threads:3","nodes":[{"name":"a","slot":0,"outcome":"finished"},{"name":"b","slot":1,"outcome":"finished"},{"name":"c","slot":2,"outcome":"finished"}],"edges":[{"from":"a","to":"b"},{"from":"b","to":"c"}],"held":[],"stopped":[]}]}
```

The run read the interrupt at its loop's end: the row carries the full
60000000000 iterations of the three contexts, every node names
`finished`, and the `stopped` array stands empty. A node the stop
request settles reaches the block and the record through the gate's own
arrival above, and no run of the entry point answers its words.

## The suite

The five graph executables, each run by name at the head:

```
$ ctest --test-dir build -R harness_graph_model_test
100% tests passed out of 1
exit 0
$ ctest --test-dir build -R harness_graph_file_test
100% tests passed out of 1
exit 0
$ ctest --test-dir build -R harness_graph_gate_test
100% tests passed out of 1
exit 0
$ ctest --test-dir build -R harness_graph_report_test
100% tests passed out of 1
exit 0
$ ctest --test-dir build -R harness_cli_graph_test
100% tests passed out of 1
exit 0
$ ctest --test-dir build -R time_source_gate
100% tests passed out of 1
exit 0
```

The gates of the quickstart's closing section stand recorded by the tasks
that own them: the coverage figures and the DBC matrix in the records of
T045 and T050, the two-cpu suite in [two-cpu.md](./two-cpu.md), the
acceptance run in [acceptance.md](./acceptance.md), and the docs gates in
the record of T046.

## The gap

Four facts stand recorded as they answered:

- The count row of the quickstart names a three-node graph over a
  two-context run. That run meets the slot rule of FR-010 rule 6 first:
  the line names slot 2 against the run's 2 contexts. The count rule of
  FR-012 answers its own line, naming 3 nodes against 4 contexts, over a
  graph whose slots all stand inside the run's count, the shape the suite
  arm drives. Both refusals exit 2 with one line and no context starts.
- The record run carries no pre-check, so its document writes no
  `precheck` key and its top-level order reads `schemaVersion`,
  `speedgun`, `graph`, `benchmarks`. The record arm reads the order with
  a pre-check present, where the graph object stands between `precheck`
  and `benchmarks`.
- The stop request's arrival at the graph gate stands driven by the suite
  alone: no caller in the entry point reaches it, so the stopped words
  reach the console block and the record through the gate arm, the
  assembly arm and the console arm.
- The interrupt of specs/020 FR-050 stands driven through the entry
  point: the chain run the signal stops answers status 1 with the skip
  line, names every node `finished`, and leaves the `stopped` array
  empty.
