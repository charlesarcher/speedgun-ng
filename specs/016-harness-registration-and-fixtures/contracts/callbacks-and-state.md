# Contract: setup and teardown callbacks and the callback state

`BenchmarkHandle` (`include/speedgun-ng/benchmark.hpp:450`) gains the
callback pair. The callbacks wrap each run in the untimed region, and
the state they receive carries a restricted operation set (R-05).

## The callback calls (FR-018)

```cpp
// BenchmarkHandle members; each returns the handle for chaining (R-01)
auto setup(std::function<void(State&)> callback) -> BenchmarkHandle&;
auto teardown(std::function<void(State&)> callback) -> BenchmarkHandle&;
```

| Rule | Requirement |
| --- | --- |
| Each callback shall run once around each run: the warm-up, calibration, and measured runs included. | FR-018 |
| The callbacks run in the untimed region; their work adds nothing to the reported time. | FR-018 |
| One callback per slot; the last attachment wins (R-10). | FR-018 |
| The state handed to a callback shall carry the instance arguments. | FR-018 |
| The state stays a `State&`; no new public view type enters the surface. | FR-018 |
| This spec runs one thread; the once-per-thread-group rule stays with the threads spec of the roadmap. | FR-019 |

## Order in one run (R-10)

```text
sampleRun(iterations)                 source/harness/runner.cpp:317-366
  State state(...)                    :324
  family.setup(state)                 added (R-10)
  fixture.setUp(state)                added (FR-017)
  entry.callable(state)               :326
  fixture.tearDown(state)             added (FR-017)
  family.teardown(state)              added (R-10)
  state.topUpWindow()                 :336
```

| Rule | Requirement |
| --- | --- |
| The order in one run shall be: setup callback, fixture `setUp`, callable, fixture `tearDown`, teardown callback. | FR-018 |
| The window pair stays exactly two samples per run: the callback work runs before `begin()` opens the window and after the loop closes it. | FR-018 |
| The timed loop shall keep the H1 shape: no added work, no allocation, no lock, no contract check. | FR-009 |

## The callback-state rule (R-05)

| Operation | In a run | In a callback or fixture hook |
| --- | --- | --- |
| `range(index)`, `rangeCount()`, `iterations()` | legal | legal |
| `begin()`, `skipWithError`, `skipWithMessage` | legal | `SG_REQUIRE` precondition violation |
| `end()` | legal | legal: static, returns `Cursor {0}`, touches no state |

| Rule | Requirement |
| --- | --- |
| The callback state is a harness-built `State` carrying the instance arguments and no recorder window. | FR-018 |
| `begin()`, `skipWithError`, and `skipWithMessage` shall be precondition violations in a callback state (R-05). | FR-018 |
| `State::end()` is static at `benchmark.hpp:277`, so no precondition can guard it; it returns `Cursor {0}` and touches no state, so a callback call stays legal (R-05). | FR-018 |
| Making `end()` non-static would replace an H1 signature, which FR-022 forbids. | FR-022 |
| The argument reads of FR-008 are legal in a callback state: `range(index)` and `rangeCount()` read the span the instance owns (R-04). | FR-008, FR-018 |

## Contract discipline (FR-022, FR-024)

| Rule | Requirement |
| --- | --- |
| `registerBenchmark` (`benchmark.hpp:556`), `SG_BENCHMARK` (`benchmark.hpp:582`), `speedgunMain` (`benchmark.hpp:576`), the H1 setters (`benchmark.hpp:478-522`), and the H1 `State` methods (`benchmark.hpp:237-322`) shall keep their signatures. | FR-022 |
| Every new interface shall document `\pre`, `\post`, and `\invariant`, and the source shall enforce each contract through the `dbc` macros (Principle II). | FR-024 |
