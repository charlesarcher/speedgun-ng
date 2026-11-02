# Contract: the fold of the series lines

Phase 1 output. The delta over the standing sample-shape contract of
`specs/034-host-noise-precheck/contracts/report-shape.md`: the fold
gains three rows for the method word, the series figures and the period
figures (spec FR-019). The standing rows, the digit mask and the
`What still fails` list stand unchanged. The standing file is amended
in the same commit as the printer (spec FR-030).

## Rows

Three rows join the table at lines 12 to 20 after line 20, in the
standing columns `| Word | Rule | Source |`:

| Word | Rule | Source |
| --- | --- | --- |
| the method word | `method: tsc`, `method: fwq` and `method: ftq` carry no fold of their own: the word keeps its bytes, so a run of one method fails a block recorded from another. | specs/043 FR-019 |
| the series figures | The digits of every series figure fold, the standing digit rule; the field name, the line order and the count of the 30 lines keep their bytes. | specs/043 FR-019 |
| the period figures | `period<n>Ns` and `period<n>Power` fold their digits, and a rank a cpu does not carry prints 0 on both sides; the per-cpu value list folds to one entry the way the standing probe block does. | specs/043 FR-019, `tools/docs/report_shape.py:186-197` |

One sentence joins after line 26:

```text
The calibration lines print one unit per measured cpu, so the standing
per-cpu multiplicity fold covers them the way it covers the stats lines
(`tools/docs/report_shape.py:103-110`, `:200-247`).
```

One bullet joins `## Fixture pairs` at lines 28 to 32:

```markdown
- The pairs carry one FWQ block against one FTQ block, and the fold
  reads them different.
```

## The standing rows this delta changes none of

- The governor row, the kernel setting values, the irq affinity list,
  the attribution deltas and the tracer rows of specs/036 keep their
  folds (specs/034 FR-017, specs/036 FR-023).
- A renamed field, a renamed source, a missing line, an added line and
  a reordered line fold different, the standing list at lines 22 to
  26 (specs/034 FR-018).
- The mask of the digit runs, the whitespace runs and the `build=`
  field stands unchanged (`tools/docs/report_shape.py:131-155`).

## Gate

- `report_shape_fixtures` reads the golden pairs in both directions,
  the pair harness of specs/018 S-5 at `test/CMakeLists.txt:1823`.
- `report_shape_fwq_ftq_sample` folds `example/docs/fwq_ftq.expected`
  against a real run of `docs_fwq_ftq` (spec FR-029, SC-007).
- `example_drift` reads the recorded run and the quoted regions on the
  guide page (`test/CMakeLists.txt:1802`, spec FR-023).

## Consumers

- `test/example-drift-fixture/run_report_shape_fixtures.py`: the pairs
  of the three rows, checked in both directions.
- `tools/docs/report_shape.py`: the fold, the digit mask at `:131-155`
  and the per-cpu list fold at `:186-197`.
- `specs/034-host-noise-precheck/contracts/report-shape.md` gains the
  three rows in the same commit as the printer (spec FR-030).
