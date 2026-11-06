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
| `any` | the worst cpu for that field: the probe cpu carrying the largest value; the rule is a uniform maximum over every field of the table alike, the configuration and span fields included, so a rule naming one of them reads the largest value the report carries for it | FR-030, D-11 |
| `<field>` | one of the 35 field names the jitter table carries: the sixteen of `JitterStats`, then the sixteen of the series fold, then the three of the attribution, in camelCase | FR-030, FR-002, specs/043 FR-021, specs/048 FR-028 |
| `<direction>:<mode>=<bound>` | the standing comparison suffix the N5 rule grammar carries | FR-033, D-11 |

The example D-11 states the whole shape in one rule:
`--compare-rule=jitter:any:p99Ns:lower:abs=5000`.

The field names are exactly the names those tables carry, in camelCase
as those tables name them: the sixteen of data-model.md E-03, then the
sixteen specs/043 adds for the series fold (E-22), then the three
specs/048 adds for the attribution: `smiDelta`, `nmiDelta` and
`smiGapCount`, listed below. A field name outside the 35 is the
parser's usage error, one line and exit status 2, and the line reads
`names none of the thirty-five jitter fields` (FR-038, specs/043
FR-021, specs/048 FR-028).

| Series field name | Value the rule reads | Requirement |
| --- | --- | --- |
| `sampleCount` | the samples the series fold kept | specs/043 FR-021 |
| `minValue`, `medianValue`, `meanValue` | the spread of the kept unit times | specs/043 FR-021 |
| `p90Value`, `p99Value`, `p999Value`, `p9999Value` | the kept series percentiles | specs/043 FR-021 |
| `maxValue` | the largest kept unit time | specs/043 FR-021 |
| `noisePercent` | the noise percent the fold reports | specs/043 FR-021 |
| `seriesRuntimeNs` | the span the kept samples cover | specs/043 FR-021 |
| `droppedSamples` | the samples the buffer dropped | specs/043 FR-021 |
| `unitTimeNs` | the unit time the method measures | specs/043 FR-021 |
| `quantumNs` | the FTQ quantum, 0 for a FWQ run | specs/043 FR-021 |
| `resampleStepNs` | the FWQ resample step, 0 for a FTQ run | specs/043 FR-021 |
| `spectrumPoints` | the DFT bins the transform answered | specs/043 FR-021 |

| Attribution field name | Value the rule reads | Requirement |
| --- | --- | --- |
| `smiDelta` | the change of the SMI count across the window the fold covers | specs/048 FR-025, FR-028 |
| `nmiDelta` | the change of the NMI count across the same window | specs/048 FR-025, FR-028 |
| `smiGapCount` | the count of kept gaps the run marked SMI | specs/048 FR-025, FR-028 |

A fold that carries no series, the fold of a `TSC` run included, reads
0 for every series field (specs/043 FR-021, FR-032). A fold that
carries no value for one of the three attribution names reads 0 under
the same rule (specs/048 FR-028).

## Prefix semantics

| Prefix | Values the rule reads | Requirement |
| --- | --- | --- |
| bare `jitter:` | the per-benchmark stats of the entry the rule names, the default, and the entry's series where the entry carries one | FR-031, D-11, specs/043 FR-021 |
| `jitter:run:` | the whole-run stats of the run-level `jitter` object, and its series | FR-031, D-11, specs/043 FR-021 |

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
- The series rule names: the 16 names resolve in the four heads, the
  fault names the grown table, the run-scoped name reads the run
  record's series while the bare name reads the entry's, and the `any`
  reading answers the largest count of the probe cpus (specs/043
  FR-021, FR-030, FR-031, D-11).
