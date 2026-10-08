# Contract: the command line and the console report

Phase 1 output. The surface `speedgunMain` parses with `getopt_long`
(FR-033), the report shape FR-035 fixes, and the exit statuses FR-036
fixes.

## Options (FR-034)

| Long option | Argument | Effect | Requirement |
| --- | --- | --- | --- |
| `--filter` | regular expression | runs exactly the benchmarks whose registered name matches | FR-034, US2 |
| `--list` | none | prints the matching names, runs no benchmark function | FR-034, US2 |
| `--repetitions` | integer ≥ 1 | repetition count | FR-007, FR-012 |
| `--min-time` | seconds, `> 0` | minimum time per run | FR-007, FR-015 |
| `--iterations` | integer ≥ 1 | explicit iteration count; calibration is skipped | FR-013 |
| `--warmup-time` | seconds, `≥ 0` | minimum warm-up time | FR-011 |
| `--dry-run` | none | one iteration, one repetition, no warm-up | FR-014 |
| `--counter` | catalog address, repeatable | attaches the leaf to every selected benchmark | FR-021 |
| `--catalog` | none | prints the catalog listing, then exits without running a benchmark | FR-037 |

- A benchmark value set through the handle shall win over the command
  line (FR-015).
- An unparseable, zero, or negative count or time shall be a
  recoverable error at the 007 FR-046 tier: the executable reports it,
  runs no benchmark, and exits nonzero (FR-034, clarification).
- `getopt_long` shall appear in the implementation alone; no public
  header names it (FR-050).

## Catalog listing (FR-037)

One line per published counter, six fields: the object path, the
counter name, the description, the unit, the read mode, and the
availability state, with the refusal kind for a leaf that cannot
count (PC-3, PC-9). The listing runs no benchmark function and exits
zero; a failed listing exits nonzero (US3, FR-036). Metrics join the
listing in the metric-formulas spec of the roadmap.

## Report shape (FR-026, FR-035)

Context lines above the rows carry the host, the cpu, the library
version, and the build type; a measured field comes from the counters
library, and a field the library does not publish stays out of the
line (R-08).

```text
host: <catalog fact, when published>   cpu: <catalog fact, when published>
speedgun-ng 0.6.0   build: <build type>

benchmark            iterations    time/iter (ns)   overhead floor (ns)   <metric columns>
BM_copy                    18432            311.4                  22.7   ipc=1.87 ratio=1.00 scaled=0 gap=0
```

- One row per repetition and one row per aggregate, in fixed columns
  (FR-035).
- Every measured row carries the plan's median sampling overhead in
  ns and states that each run window includes the cost of its two
  endpoint samples (FR-026).
- Each metric column carries the value with its running ratio, scaled
  flag, and gap state (FR-024).
- A skipped benchmark prints its reason and no statistics (FR-031).
- A failed benchmark prints the exception text (FR-032).
- Aggregates print when repetitions exceed 1: mean, median, standard
  deviation, coefficient of variation, min, max, for time and for each
  metric (FR-027).

## Exit statuses (FR-036)

| Condition | Status |
| --- | --- |
| a benchmark failed, the catalog listing failed, or SIGINT interrupted a run | nonzero |
| the filter matched no benchmark | zero, with a message |
| list mode | zero |
| measured or skipped benchmarks | zero |

## Interrupt behavior (clarification, FR-032)

SIGINT ends the current run at the next iteration boundary; the
benchmark reports as skipped with an interrupt reason; the plan, the
recorder, and every descriptor and mapping release; the process exits
nonzero (R-06).
