# Contract: the graph seam and the synthetic layouts

Phase 1 output (specs/037). One named detail seam the tests replace
(FR-025, D-6), the way `installTestOnlineCpus` replaces the online set
(`source/harness/topology.cpp:352`) and `installTestMemoryBinder`
replaces the binder (`source/harness/detail/internal.hpp:802`).

## The seam

| Member | Rule |
| --- | --- |
| install | `installTestGraph(std::optional<detail::Graph>)` installs a synthetic graph the run reads in place of the file |
| clear | an empty optional restores the file read, the clear-on-null shape `installTestLayout` uses (`topology.cpp:336`) |
| slot | `graphInput()` answers the installed graph when one stands, and the graph the entry point read once when none stands; consulted at each threaded attempt, so an install decides the attempt that follows it |
| entry point | the entry point reads the file once, before the first context starts (FR-011), and installs the graph it read into the same slot |

The seam is detail: no public name joins (D-8). The declaration stands
in `source/harness/detail/internal.hpp` beside the standing installers,
and the definition stands in `source/harness/graph.cpp`.

## The synthetic layouts

The tests drive every arm through injected graphs (FR-025):

| Layout | Shape | Arms it drives |
| --- | --- | --- |
| chain | `a -> b -> c` over three contexts | the start order, the release by the completion state, the record order |
| fan-out | one source, two dependents | FR-014 release of every dependent |
| fan-in | two sources, one dependent | FR-014 start after the last |
| diamond | `a -> b`, `a -> c`, `b -> d`, `c -> d` | fan-in, the ascending-slot release order of FR-013 among `b` and `c`, the record fold |
| cycle | two nodes naming each other | the FR-010 cycle refusal (file arm) |
| unsatisfiable | a slot two nodes name, a slot none names | the FR-010 refusal (file arm) |
| rejected file | a document the parser rejects | the FR-010 parser refusal (file arm) |
| failure body | a body that throws on one context | the `failed` word, the held dependents, the reason |
| stop | the stop request beside a gated node | the `stopped` word and its dependents |

## The suite rules beside the seam

- No test sleeps and no test reads a clock: the start order comes from
  the injected graph (FR-026).
- The cpus come through `installTestOnlineCpus`, and the suite passes
  under `taskset -c 0,1` and on a 2-cpu host with no fixed node count
  (FR-027).
- The file arms run through planted files under a temporary directory,
  the standing fake-tree shape of specs/034 R-11.
- CI needs no hardware (D-6 of the spec).

## Consumers

- `test/source/harness_graph_gate_test.cpp` installs every layout.
- `test/source/harness_graph_file_test.cpp` plants every file arm.
- `test/source/harness_graph_model_test.cpp` drives the pure rules
  with no run.
