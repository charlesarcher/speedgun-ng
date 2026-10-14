# Contract: The pause capacity and the default-unit resolution

The two H3 leftovers (FR-024 to FR-027) change rules the tree states
today. Each rule below replaces the audit-point text at the cited
place.

## The pause capacity (FR-024, FR-025, FR-026)

| Rule | Requirement | Replaces |
| --- | --- | --- |
| A run shall carry no fixed pause allowance: `kMaxPausePerRun`, its guard, its recorder capacity term, its bound tests, and its guide text all leave the tree. | FR-024 | The `kMaxPausePerRun` constant and its `SG_REQUIRE` guard in `State::pauseTiming` (`include/speedgun-ng/benchmark.hpp`), the capacity argument of the `plan->recorder` call in `Runner::run` (`source/harness/runner.cpp`), the bound scenarios and the `ninthPause` mode in `test/source/harness_pause_test.cpp`, and the pause-limit paragraphs of `docs/pages/harness.md` |
| From a family's second attempt the recorder shall reserve, per leaf column, the pause room its previous attempt observed (two points per closed pair per iteration), capped at `kMaxPauseSamplesPerLeaf` raw points per leaf column. | FR-051 | research D-10, Q-10 |
| An attempt without pause history or over the cap shall fold each closed pause pair into its row's running totals at the pair's commit, from the same two raw points, printing columns equal to the raw path. | FR-052 | research D-10, Q-10 |
| A run that closes no pause shall reserve no pause room beyond the endpoint points. | FR-053 | research D-10 |
| The result records the fold and the report discloses a folded run with a token; raw runs keep their bytes. | FR-054 | research D-10 |
| A benchmark that closes a `ScopedPauseTiming` in every iteration shall complete the run with every column netted by the H3 pause rule. | FR-026 | 017 E-03 netting, unchanged |
| An open pause at loop exit shall stay the existing contract violation. | FR-026 | the H3 exit check, unchanged |

## The default-unit resolution (FR-027)

| Rule | Requirement | Replaces |
| --- | --- | --- |
| A macro or `registerBenchmark` registration shall store no unit; the family record shall hold `std::optional<TimeUnit>`. | FR-027 | the registration-time capture in `addRegistryEntry` (`source/harness/registry.cpp`) |
| `speedgunMain` shall snapshot `getDefaultTimeUnit()` at its entry into the batch. | FR-027 | research D-11 |
| A run's result unit shall resolve as: the `unit(...)` handle value when set, else the batch snapshot, else the process default read at that run's start. | FR-027 | the `BenchmarkResult::timeUnit` doxygen sentence (`include/speedgun-ng/benchmark.hpp`) |
| A `setDefaultTimeUnit` call in `main` before `speedgunMain` shall reach every macro registration; `unit(...)` shall still win over it. | FR-027 | `harness_time_unit_test.cpp` captured-at-registration scenario |
| The README "The time unit" section and the `docs/pages/harness.md` "Time units" paragraph shall state the resolution rule at run start, in the same commit range as the code. | FR-041 | the captured-at-default sentences |

## Consumers

`test/source/harness_pause_test.cpp` asserts the every-iteration run,
the bounded reservation, the empty pause room of a non-pausing body,
the fold path's equality with the raw path, and the absence of the old
constant; `test/source/harness_time_unit_test.cpp` asserts the
resolution order; `test/barrier_shape.sh` and
`test/time_source_gate.sh` stay green unchanged (SC-08, SC-11 to
SC-13).
