# Contract: Options and Surface

## The family handle

```cpp
// include/speedgun-ng/benchmark.hpp, BenchmarkHandle
/**
 * @brief Turn the percentile output of this family on or off.
 *
 * \pre none
 * \post The family's rows carry percentile lines when the run leaves the
 *       output on, and carry none when it stands off.
 */
auto showPercentiles(bool show) -> BenchmarkHandle&;
```

The call selects the output and carries no percentile list (D-09). It writes
`RegistryEntry::showPercentiles`, the same shape `minTime` and `repetitions`
use.

## The command line

| Option | Argument | Effect |
|---|---|---|
| `--percentiles=<list>` | required | turns the output on for every family of the run and sets the list |
| `--no-percentiles` | none | turns the output off for every family of the run |

`<list>` is a comma-separated list of decimal values, each in the open
interval (0, 100). The scanner parses it with the same shape as the existing
parsers in `source/harness/cli.cpp:50-108`: one line on standard error naming
the option and the offending text, then `std::exit(2)`, before any benchmark
runs (FR-022).

**Exit-2 forms the tests cover**: an empty list, an empty element
(`99,,50`), `0`, `100`, a negative, a value above 100, a non-numeric element,
a value with no digits before the point, and a list longer than 16 entries.

**Accepted forms**: `50`, `50,90,99,99.9`, spaces around an element, a
duplicate element, and an unsorted list. Duplicates collapse and the printed
order is ascending (FR-024).

## Precedence

The command line wins over the family, the rule `--repetitions` already takes:

1. `--no-percentiles` turns the run off, whatever the family asked.
2. `--percentiles=<list>` turns the run on with that list, whatever the family
   asked.
3. With neither option, the family setting stands, and a family that asked
   nothing runs with the output off and the default list (FR-023).

## Version and SOVERSION

The project version takes the next free minor, 0.11.0 when this feature
merges first. The harness `SOVERSION` moves 6 to 7: `BenchmarkHandle` gains a
method and `ResultRow` gains fields, both public in the harness archive, and
the constitution's Refactoring and Evolution rule keeps `SOVERSION` consistent
with a public API change (D-15). The `speedgun-ng` archive `SOVERSION` stays
2, because the counters library gains nothing.
