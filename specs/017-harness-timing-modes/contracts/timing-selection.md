# Contract: The timing selection calls

`BenchmarkHandle` (`include/speedgun-ng/benchmark.hpp:594`) gains four
calls, and the `sg` namespace gains the default-unit pair. Each call
writes one field of the family record (E-01) before the first run of that
family.

## The handle calls (FR-004, FR-005, FR-006, FR-015)

```cpp
// BenchmarkHandle members; each returns the handle for chaining
auto measureProcessCpuTime() -> BenchmarkHandle&;
auto useRealTime() -> BenchmarkHandle&;
auto useManualTime() -> BenchmarkHandle&;
auto unit(TimeUnit timeUnit) -> BenchmarkHandle&;
```

| Rule | Requirement |
| --- | --- |
| The CPU column of that family shall read `machine/process_cpu`. | FR-004 |
| The switch shall combine with the real-time call and with the manual-time call. | FR-004 |
| Real time shall become the primary time of that family: the qualification test, the growth rule, and the aggregate time statistics read it. | FR-005 |
| The primary time shall become the sum of the accepted iteration times. | FR-006 |
| The printed time values of that family shall carry the unit and its label. | FR-015 |
| Each call shall stand behind the `m_entry != nullptr && !m_entry->runStarted` precondition the existing setters use. | FR-018, `source/harness/registry.cpp:59` |
| Each call shall return this handle, so the chain of H1 and H2 keeps its shape. | FR-018 |

## The exclusion (FR-008)

| Rule | Requirement |
| --- | --- |
| `useRealTime` shall carry `SG_REQUIRE(!m_entry->manualTime, ...)`. | FR-008 |
| `useManualTime` shall carry `SG_REQUIRE(!m_entry->realTime, ...)`. | FR-008 |
| The second of the two calls is the violation, in either order. | FR-008, research D-04 |
| `measureProcessCpuTime` shall stay legal with either selection. | FR-008 |
| The cited revision reports the same conflict as a check on the second call (`benchmark_register.cc:435-445`). | reference read |

## The default unit (FR-016)

```cpp
auto setDefaultTimeUnit(TimeUnit timeUnit) -> void;
[[nodiscard]] auto getDefaultTimeUnit() -> TimeUnit;
```

| Rule | Requirement |
| --- | --- |
| The default shall start at `TimeUnit::NANOSECONDS`. | FR-016 |
| A registration shall capture the default of the moment of that registration into the family record. | FR-016 |
| A later `setDefaultTimeUnit` call shall leave every registered benchmark at its captured unit. | FR-016, spec Edge Cases |
| `unit` on a handle shall overwrite that family's captured unit. | FR-015 |
| The command-line option stays with the full command-line spec of the roadmap. | FR-016 |

## Naming (FR-019)

| Name | Rule |
| --- | --- |
| `TimeUnit`, `ScopedPauseTiming` | N-1 PascalCase |
| `TimeUnit::NANOSECONDS` and its siblings | N-4 `UPPER_SNAKE_CASE`, no prefix |
| `measureProcessCpuTime`, `useRealTime`, `useManualTime`, `unit`, `setDefaultTimeUnit`, `getDefaultTimeUnit` | N-2 camelBack; the acronym inside `measureProcessCpuTime` spells as one word under N-1 |
| `kMaxPausePerRun` | N-8 `k` prefix, PascalCase remainder |
| no new macro | N-3 has nothing to prefix |

## Contract blocks (FR-020)

Each declaration carries a doxygen block with `\pre` and `\post`, and the
definition in `source/harness/registry.cpp` enforces each clause with the
matching macro kind. The pair gate
(`tools/dbc/dbc_pair_gate.py`, `cmake/dbc-gate.cmake`) then finds one
enforced kind per documented kind inside the interface scope, the shape
the 016 convergence established: a method block states `\pre` and `\post`,
and a class-level `\invariant` stands on the class block with its
enforcement in the members.
