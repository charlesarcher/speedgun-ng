# Quickstart: Metric Formulas

**Feature**: `022-metric-formulas` | **Plan**: [plan.md](./plan.md)

The shortest path from a fresh clone to a metric number, then the two
surfaces a reader uses after that.

## Build and check

```bash
cmake --preset=ci-rocky
cmake --build build
ctest --test-dir build --output-on-failure
```

The same loop on a machine with clang-tidy and cppcheck installed is
`cmake --preset=ci-ubuntu` before the build. Feature work runs both once per
feature, as `AGENTS.md` states.

## Ask for one metric in code

```cpp
#include <speedgun-ng/counters.hpp>
#include <speedgun-ng/counters_metrics.hpp>

auto const found = sg::counters::metricByName("cpi");
if (not found)
{
  std::puts(found.error().message().c_str());  // one line, near-miss names
}
else
{
  auto const view = *found;                     // expression, group, unit
  std::printf("%s = %g %s\n",
              "cpi", view.expression.fold(recorder).value, view.unit.data());
}
```

The block above is the shape of the region `example/docs/counter_metrics.cpp`
publishes, so the page and the compiled example agree. A metric folded over
the fake provider gives the number the same counts give a hand-composed
`CPU_CLK_UNHALTED.THREAD / INST_RETIRED.ANY`.

## See what the host offers

```bash
./build/example/first_benchmark --catalog
```

Counter lines come first and keep their shape. Metric lines follow, one per
metric row of the selected table, each with its group, its unit text and
either an availability value or the reason it is not there. A metric refused
at load is listed with its reason, so the listing accounts for the whole
table.

## Put a metric in a report

```bash
./build/example/first_benchmark --metric cpi --metric l2_mpi
```

Two columns appear, in that order, labeled with the names. A name the host
has no metric for prints one line and exits with status 2 before any
measurement runs, so a typo costs no measured time. A metric the host cannot
count prints its refusal kind in the column and the run continues.

## What this host selects

`corsairone` is family 6, model 85, stepping 4. Before this feature the
mapping-file key omitted the stepping, so a model-55 part matched no row of
the pinned mapping file and selected no table. With the stepping in the key
the host selects `skylakex`, and the listing above shows 279 metric rows
instead of none. The two fixtures in `counters_pmu_test.cpp` cover both sides
of the boundary the pinned file draws at stepping 5.

## Checks a reviewer runs

```bash
ctest --test-dir build -R 'counters_metric|counters_pmu_test|harness_catalog_test|harness_cli_test'
ctest --test-dir build -R 'example_run|snippet_compile|report_shape'
bash test/cli_shape.sh build
python3 tools/prose/prose_gate.py --check prose
```

The first line covers the grammar, the lookup, the selection, the listing and
the option. The second covers the page obligation of Principle IV.1. The
third checks the option table against `README.md`, and the last is the prose
gate.
