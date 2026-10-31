# Quickstart: Execution Graphs

Feature: specs/037-execution-graphs. Every scenario below runs on the
lane host or in the suite; the commands stand in code spans so the
prose gate reads none of them. The contract details stand in
[contracts/](./contracts/) and the entity rules in
[data-model.md](./data-model.md); this file carries the runs alone.

## Prerequisites

- The build stands configured: `cmake --preset=ci-rocky` and
  `cmake --build build -j 24`.
- The suite itself passes on any host: the graphs come through
  `installTestGraph`, the cpus through `installTestOnlineCpus`, and no
  test carries a fixed node count.
- The acceptance record stands on the lane host (a 16-core, 32-thread
  box), the shape beside the 24-core, 48-thread record of specs/035.

## A graph file

```json
{
  "nodes": [
    { "name": "a", "slot": 0, "deps": [] },
    { "name": "b", "slot": 1, "deps": ["a"] },
    { "name": "c", "slot": 2, "deps": ["b"] }
  ]
}
```

## Scenarios

| Scenario | Command | Exit | Expected |
| --- | --- | --- | --- |
| The chain orders the starts | `./build/example/docs_execution_graphs --graph=stages.json` | 0 | the console block names the nodes `a`, `b`, `c` in the ascending slot order FR-023 fixes, beside the `contexts=` line |
| The record carries the run | the same run with `--json=report.json` | 0 | the top-level `graph` record names the nodes in the ascending slot order FR-023 fixes, the edges and the settled words, after `precheck` and before `benchmarks` |
| The read-back holds | `ctest --test-dir build -R harness_graph_report_test` | 0 | `loadReport` folds the record against the run's facts |
| A cycle refuses | a file whose two nodes name each other, through the suite | 2 | one line naming the cycle, no context starts |
| A rejected file refuses | a planted file the parser rejects, through the suite | 2 | one line naming the file and the fault, no context starts |
| A count mismatch refuses | a three-node graph over a two-context run, through the suite | 2 | one line naming both counts, no context starts |
| A failed node holds its dependents | an injected graph with a body that throws, through the suite | the standing run-failure status | the record names each held node with the failed node as its reason |
| The graph-free bytes stand | `ctest --test-dir build -R harness_graph_report_test` | 0 | a run with no `--graph` matches master's console and JSON bytes apart from `schemaVersion` and `speedgun.version` |
| The example folds | `ctest --test-dir build -R report_shape` | 0 | the recorded block folds equal against the binary's own run |

## The suite

The new executables run beside the standing harness tests:

- `ctest --test-dir build -R harness_graph_model_test` drives the
  model, the words, the order rules and the seam.
- `ctest --test-dir build -R harness_graph_file_test` drives every
  FR-010 arm through planted files.
- `ctest --test-dir build -R harness_graph_gate_test` drives the
  chain, the fan-out, the fan-in, the diamond, the held arms, the
  stopped arm and the barrier arrival through injected graphs.
- `ctest --test-dir build -R harness_graph_report_test` drives the
  console block, the record keys, the order, the read-back and the
  byte identity.
- `ctest --test-dir build -R harness_cli_graph_test` drives the row,
  the bare form, the option-file source and the count refusals.
- The suite runs `ctest -j 2`; the coverage run uses
  `ctest --test-dir build/coverage -j 1`. The suite passes under
  `taskset -c 0,1` and on a 2-cpu host. `PERL5LIB` stands at
  `/home/archerc/perl5/lib/perl5` for the lcov run.

## The gates

- `test/time_source_gate.sh` stays green over every new unit: the
  graph reads no clock and no test reads one (FR-016, FR-026).
- The coverage run reads 100.0% lines and 100.0% branches with no
  `LCOV_EXCL` marker (FR-028).
- `cmake --build build -t dbc-gate` and
  `ctest --test-dir build -R dbc_gate_fixtures` run the DBC
  completeness gate over every new unit (Principle VI, FR-028).
- `cmake --preset=ci-ubuntu` and `cmake --build build` run the
  clang-tidy and cppcheck report once for the feature (Principle IX).
- The `ci-sanitize` and `ci-tsan` runs carry the sanitizers and the
  thread sanitizer over the graph suite (Principle VIII).
- `ctest -R example_drift`, `ctest -R snippet_compile`,
  `ctest -R example_run` and `ctest -R report_shape` fold the example
  and the page rows (FR-032 to FR-034).

## The acceptance record

The acceptance run stands on the lane host under
`specs/037-execution-graphs/results/`, produced by
`tools/graph/acceptance.py` and read by no CI job (FR-030, FR-031):

1. Run the script; it writes the fan-out and fan-in graph over the 16
   cores, the printed node order, the figures beside it, the
   boot, the commands and the exit codes.
2. The record names the host's 16-core, 32-thread shape beside the
   24-core, 48-thread record of specs/035.
3. The results directory links the record when the lane lands it.
