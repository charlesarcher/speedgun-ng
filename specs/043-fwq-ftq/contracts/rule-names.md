# Contract: the thirty-two jitter rule names

Phase 1 output. The delta over the standing name grammar of
`specs/027-jitter-probe/contracts/rule-names.md`: the field row gains
16 series names, the table `kJitterFields` grows from 16 to 32, and the
bare prefix gains the per-benchmark series (spec FR-021, FR-030). The
four grammar forms, the status-2 stop on a probe-free report and every
other row of the standing contract stand unchanged. The standing file
is amended in the same commit as the table (spec FR-030).

## The field row

Line 20 becomes, in the standing columns
`| Part | Meaning | Requirement |`:

| `<field>` | one of the thirty-two field names of the standing stats record and the series record, in camelCase | specs/043 FR-021, FR-030 |

## The name list

One sentence replaces lines 26 to 29:

```text
The names are the sixteen field names data-model.md E-03 carries, plus
the sixteen series names `sampleCount`, `minValue`, `medianValue`,
`meanValue`, `p90Value`, `p99Value`, `p999Value`, `p9999Value`,
`maxValue`, `noisePercent`, `seriesRuntimeNs`, `droppedSamples`,
`unitTimeNs`, `quantumNs`, `resampleStepNs` and `spectrumPoints`. The
table `kJitterFields` of `source/harness/compare.cpp:251-292` grows
from 16 to 32 entries, and a name outside the thirty-two answers one
line and exit status 2 with the words `names none of the thirty-two
jitter fields` (specs/043 FR-021).
```

`runtimeNs` stays the gap figure of the standing `stats` item, and the
series figure takes the new name `seriesRuntimeNs`, so the lookup
answers one entry (spec FR-021).

## Prefix row

One row joins the prefix table after line 36, in the standing columns
`| Prefix | Values the rule reads | Requirement |`:

| bare `jitter:` with a series name | the per-benchmark `series` array of the entry the rule names | specs/043 FR-021 |

`jitter:run:<cpu|any>:<field>` reads the run record, the standing row
of line 36 (spec FR-021). The per-benchmark item carries no `method`
key and no `periods` key, so a rule names a figure alone (spec
FR-020).

## The `any` reading

One sentence joins after line 41:

```text
The `any` reading stays the uniform maximum over every field, so `any`
names the best cpu for a count figure. A noise rule over an FTQ run
names `noisePercent` with `higher` (specs/043 FR-021).
```

## The landing row

One row joins the landing table after line 59, in the standing columns
`| Rule | Requirement |`:

| The rule-name table, the count word and this contract move in one commit. | specs/043 FR-021, FR-030 |

## The standing rows this delta changes none of

- The four grammar forms of lines 10 to 13 stand unchanged, and the
  standing suffix keeps its reading (specs/026, specs/027 FR-033).
- The standing 16 stats names keep their values, and a `tsc` run keeps
  the standing gap rule (spec FR-021, FR-031).
- The status-2 stop on a probe-free report stands as specs/027 states
  it, one line and exit status 2 (specs/027 FR-032).
- The comparison gate gains no option: the names ride `--compare-rule`
  (specs/027 FR-033, spec FR-034).

## Gate

- `harness_jitter_window_test` reads the 16 new names, the fault text,
  the run-scoped name and the `any` reading over a report that carries
  the new keys (spec FR-021, US4 scenario 4).
- `harness_precheck_test` reads the noise refusal, which fires in the
  pre-check phase with one line and status 2 (spec FR-021).
- `harness_compare_test` reads the standing rule grammar over the
  moved record (specs/026, specs/027 FR-033).

## Consumers

- `test/source/harness_jitter_window_test.cpp`: the 16 names, the
  fault text, the run-scoped prefix, and the `any` reading of a count
  figure (spec FR-021).
- `test/source/harness_precheck_test.cpp`: the noise gate and the
  unused `noise=` disclosure (spec FR-021).
- `specs/027-jitter-probe/contracts/rule-names.md` gains the names,
  the count word and the `any` reading in the same commit as the table
  (spec FR-030).
