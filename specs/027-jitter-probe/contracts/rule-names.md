# Contract: the jitter value names of the comparison gate

Phase 1 output. The names the merged N5 value-name parser carries:
merges, the prefix semantics, and the rule a comparison run holds a
probe report to (FR-030 to FR-033, FR-038, D-11, SC-004).

## The name grammar

```text
jitter:<cpu>:<field>:<direction>:<mode>=<bound>
jitter:any:<field>:<direction>:<mode>=<bound>
jitter:run:<cpu>:<field>:<direction>:<mode>=<bound>
jitter:run:any:<field>:<direction>:<mode>=<bound>
```

| Part | Meaning | Requirement |
| --- | --- | --- |
| `<cpu>` | a probe cpu index the report's `jitter` object names | FR-030, FR-026 |
| `any` | the worst cpu for that field: the probe cpu carrying the largest value; the rule is a uniform maximum over every field of `JitterStats` alike, the configuration and span fields included, so a rule naming one of them reads the largest value the report carries for it | FR-030, D-11 |
| `<field>` | one of the sixteen field names of `JitterStats`, in camelCase | FR-030, FR-002 |
| `<direction>:<mode>=<bound>` | the standing comparison suffix the N5 rule grammar carries | FR-033, D-11 |

The example D-11 states the whole shape in one rule:
`--compare-rule=jitter:any:p99Ns:lower:abs=5000`.

The field names are exactly the sixteen field names data-model.md E-03
carries, in camelCase as that table names them. A field name outside
the sixteen is the parser's usage error, one line and exit status 2
(FR-038).

## Prefix semantics

| Prefix | Values the rule reads | Requirement |
| --- | --- | --- |
| bare `jitter:` | the per-benchmark stats of the entry the rule names, the default | FR-031, D-11 |
| `jitter:run:` | the whole-run stats of the run-level `jitter` object | FR-031, D-11 |

`any` selects the worst cpu for that field over the values the prefix
names (FR-030). A named `<cpu>` that the report's `jitter` object does
not carry is the same usage error as the probe-free case below, one
line and exit status 2 (FR-032, FR-038).

## Status 2 on a probe-free report

| Rule | Requirement |
| --- | --- |
| A `jitter:` rule evaluated on a report that carries no probe prints one line and exits with status 2. | FR-032, D-11, SC-004 |
| The line is the comparison run's usage error, and the rejection lands where the rule first touches the report's `jitter` object. | FR-032, R-12 |
| A rule with no `run` prefix reads the per-benchmark values of the named benchmark (D-11). | FR-031 |

## Landing through the N5 parser

| Rule | Requirement |
| --- | --- |
| The names join `parseCompareRule(std::string_view)`, public API of the merged N5 (`include/speedgun-ng/benchmark.hpp:2500`, `source/harness/compare.cpp:722`). | FR-033, PC-7 |
| When N13a is ready before N5 merges, the branch waits for N5 and rebases. | FR-033 |
| The feature copies no parser code: one parser carries the value-name grammar, and a probe-local parser is the divergence FR-033 exists to prevent. | FR-033, R-12 |
| The names ride the option `--compare-rule` (short letter `Q` on the N5 branch); no second option joins. | FR-033, R-12 |
| `specs/026-regression-gate/contracts/` carries the value names in the same commit as the parser change. | FR-033 (plan, Files outside the feature directory) |

## Consumers

- The N5 comparison surface: the rule names through the merged
  `parseCompareRule`; the worst-cpu field names; the `jitter:run:`
  prefix; the status-2 stop on a probe-free report; and the status-1
  comparison failure on a bound the probe moves past (FR-030 to FR-033,
  SC-004).
- `test/source/harness_jitter_window_test.cpp` carries the probe report
  the comparison reads; the comparison tests ride the merged N5 surface (FR-033).
