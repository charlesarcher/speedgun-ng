# Contract: the report shape fold for the node plans

Extension of `contracts/report-shape.md` of specs/034 (specs/035).
The mask folds digit runs to one `#` and keeps the separators, so the
placement field's node tail folds with no rule of its own; this file
records the two shapes the gate compares and pins the fixture pairs.

## The placement field's folded shapes

| Field | Folded shape |
| --- | --- |
| `cpu@group` (standing) | `cpu@#` |
| `cpu@group@node` (bound) | `cpu@#@#` |

| Rule | Requirement |
| --- | --- |
| The bound and unbound shapes fold different, and the gate fails a run whose preset and field disagree. | FR-011, FR-002 |
| The recorded example run carries the bound shape on every host the gates run on: the node tail stands whenever the `node` preset binds, a one-node host included. | FR-022 |
| No new fold joins the mask: the standing digit rule and the `@` separators carry the shapes. | D-5 |

## Fixture pairs

`test/example-drift-fixture/run_report_shape_fixtures.py` carries the
golden pair of the table above, checked in both directions, beside
the standing pairs (S-5 of specs/018).
