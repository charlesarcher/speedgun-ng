# Contract: The `metricMethod` handle selector and the `MetricMethod` enum

**Feature**: 024-multipass-collection | **Data-model entity**: E-01 |
**Research**: D-09, D-10 | **Implements**: FR-023, FR-024, FR-025

The caller's family-side choice of how a metric-heavy run collects its
events. The command-line path is the contract
[`metric-method-option.md`](metric-method-option.md); the printed outcome is
[`pass-disclosure.md`](pass-disclosure.md).

## Public API

```cpp
enum class MetricMethod : std::uint8_t
{
  AUTO,
  PASSES,
  MULTIPLEX,
};

class BenchmarkHandle
{
public:
  auto metricMethod(MetricMethod method) -> BenchmarkHandle&;
  // ... existing surface unchanged: BENCHMARK_DEFINE, addMetric (both
  // forms), addCounter, showPercentiles, noPercentiles, ...
};
```

Spelling: the closed enumeration is the public vocabulary, the enumerators
carry the enumerator naming constitution V states, and the selector is
camelBack (constitution V).

## Contract

| Name | Contract |
|---|---|
| `MetricMethod` | The three collection strategies E-01 names. `AUTO` is the caller's "you decide"; `PASSES` names the planner path; `MULTIPLEX` names the single kernel-rotated plan. No other value exists (FR-023). |
| `metricMethod(method)` | Sets this family's requested method. `\pre` none. `\post` the entry carries that request, and a run of that family settles to it wherever the host and the metric columns allow (data-model E-01 table); the settled value is published as `BenchmarkResult::metricMethod` whatever it settled to (FR-025). |

`metricMethod` returns the handle for chaining, matching the existing
`addMetric`, `addCounter`, `showPercentiles` and `noPercentiles` fluent shape
(`include/speedgun-ng/benchmark.hpp:1703,1795`).

## Resolution

The run reads one method from two sources, in priority order:

1. The `--metric-method` flag when the command line carried it (the CLI
   contract),
2. the entry's `metricMethod` request,
3. `AUTO`.

The flag wins the family: one run of one family settles one method for every
instance it runs, whatever each family asked for (FR-025). `AUTO` settles at
run time per the E-01 table: `PASSES` wherever a limit is published and the
columns need the planner or fit in one pass alike; `MULTIPLEX` on a host
where no needed source publishes a limit (research D-10's reading of FR-020
and FR-024).

## The handle path carries no bad value

The selector's parameter is the closed enum: no string is parsed, no integer
is accepted, and a caller cannot spell a fourth method. The spec's bad-value
edge states that unrepresentability: the type system answers it on this
surface, and the exit-status-2 refusal belongs to the option surface alone
(FR-026; the option contract).

## Worked examples

```cpp
BENCHMARK_DEFINE(fit)
{
  return sg::Benchmark{}.iterations(100).addMetric([](auto& c) {
    return c.cpuClocks() / c.instructions();
  });
}

// This family always asks for passes; a `--metric-method=multiplex` run of
// it still settles to multiplex, because the command line wins.
BENCHMARK(fit)->metricMethod(sg::MetricMethod::PASSES);
```

The host without a published limit keeps the run single-plan even here: the
request is kept, the settle names `MULTIPLEX`, and the disclosure says so
(FR-023, FR-024; the `MULTIPLEX` row of data-model E-01).

## Scope boundary

The selector carries no plan, no limit, no column list and no schedule: it
names a method and nothing else. Passing it changes no timing behavior on its
own; the run settles and discloses. `MetricMethod` values are plain data on
`BenchmarkResult`, so a program reading a result can tell which method
produced it without re-running (FR-017).
