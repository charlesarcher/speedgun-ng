# Contract: The pause capacity and the default-unit resolution

The two H3 leftovers (FR-024 to FR-027) change rules the tree states
today. Each rule below replaces the audit-point text at the cited line.

## The pause capacity (FR-024, FR-025, FR-026)

| Rule | Requirement | Replaces |
| --- | --- | --- |
| A run shall carry no fixed pause allowance: `kMaxPausePerRun`, its guard, its recorder capacity term, its bound tests, and its guide text all leave the tree. | FR-024 | `runner.cpp:80` (constant), `:356-359` (capacity term), its guard, `harness_pause_test.cpp` bound scenarios and the `ninthPause` mode, `docs/pages/harness.md:296-298` and `:341-344` |
| A run's recorder points shall bound by `2 + 2 * iterations` per leaf, and the recorder shall reserve that many points at plan build between attempts. | FR-025 | research D-10 |
| A benchmark that closes a `ScopedPause` in every iteration shall complete the run with every column netted by the H3 pause rule. | FR-026 | 017 E-03 netting, unchanged |
| An open pause at loop exit shall stay the existing contract violation. | FR-026 | the H3 exit check, unchanged |

## The default-unit resolution (FR-027)

| Rule | Requirement | Replaces |
| --- | --- | --- |
| A macro or `registerBenchmark` registration shall store no unit; the family record shall hold `std::optional<TimeUnit>`. | FR-027 | `registry.cpp:50-56` (the registration-time capture) |
| `speedgunMain` shall snapshot `getDefaultTimeUnit()` at its entry into the batch. | FR-027 | research D-11 |
| A run's result unit shall resolve as: the `unit(...)` handle value when set, else the batch snapshot, else the process default read at that run's start. | FR-027 | `benchmark.hpp:188-192` doxygen sentence |
| A `setDefaultTimeUnit` call in `main` before `speedgunMain` shall reach every macro registration; `unit(...)` shall still win over it. | FR-027 | `harness_time_unit_test.cpp` captured-at-registration scenario |
| `README.md:263` and `docs/pages/harness.md:402-405` shall state the resolution rule at run start, in the same commit range as the code. | FR-041 | the captured-at-default sentences |

## Consumers

`test/source/harness_pause_test.cpp` asserts the every-iteration run and
the absence of the old constant; `test/source/harness_time_unit_test.cpp`
asserts the resolution order; `test/barrier_shape.sh` and
`test/time_source_gate.sh` stay green unchanged (SC-08).
