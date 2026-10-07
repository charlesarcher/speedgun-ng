# Contract: gate baseline

FR-021 requires the plan to name the commit that passes every hard gate
before the first rename commit. That commit does not exist yet. This
contract records the runs that were observed and the steps that produce
the missing commit. It does not invent a SHA.

## Observed runs

| Run | Head | Conclusion | Standing |
| --- | --- | --- | --- |
| [37553123469](https://github.com/charlesarcher/speedgun-ng/actions/runs/37553123469) | `6d32efcab3c13c3d41470c1c621d78839ce11543` | failure | Audit point. Lint failed. Every other job was skipped. |
| [37533305027](https://github.com/charlesarcher/speedgun-ng/actions/runs/37533305027) | `daa4b6db8c46783d7f0ad05190d246ffc5abcfef` | failure | `test`, `test-rocky`, `sanitize`, `coverage`, and `tsan` failed. |
| [37175047162](https://github.com/charlesarcher/speedgun-ng/actions/runs/37175047162) | `6aafd2dc8a1310ee335820ad8dc4dedd0eb0ae17` | success | Last green run. It predates the counters names FR-013 renames. |

None of these heads is the gate baseline.

## Predecessor

1. Repair the 21 format-check files the specification lists. Change no
   identifier. That repair is tasks.md T001. A rename commit does not
   absorb it.
2. Land the repair on the default branch.
3. Wait for a green Continuous Integration run on that branch.
4. Record the run's head SHA in `plan.md`, with the passing test names,
   the gate results, and the name-check finding count per translation
   unit.

No rename commit opens before step 4 is in `plan.md`.

## Comparisons that use the baseline

- FR-001 and SC-001: the passing test set, by name.
- FR-002 and SC-002: normalized machine code, contracts ignored.
- FR-003 and SC-003: sampling figures within five percent on the host
  research D-07 names.
