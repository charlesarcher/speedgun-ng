# Contract: The pre-check block's sample shape

`tools/docs/report_shape.py` folds the pre-check block's host-owned
words beside the run-owned values the standing mask folds (the
specs/018 contract, the specs/019 and specs/020 extensions). The
extension lands in the same commit as the printer, the way the
earlier features extended it, and every token the built-in fields
print today keeps its bytes (FR-017, FR-018).

## The pre-check block's host-owned words

| Word | Rule | Source |
| --- | --- | --- |
| `inf`, `-inf`, `nan`, `-nan` | A window that kept one gap divides by its zero stamp span; the figure the block carries and the word the unlucky run prints fold to one decimal shape (`#.#`). | FR-016, E-03 |
| the governor a host runs, and the `not available` wording | The value after `governor: cpu <n>` folds; the source token `(source: /sys devices system cpu scaling_governor)` keeps its bytes. | FR-017 |
| the kernel setting values after `isolcpus=`, `nohz_full=`, `rcu_nocbs=` | Each value folds, the `not available` wording beside a stated one; the keys keep their bytes. | FR-017 |
| the irq affinity list, and the `not available` wording | The value after `irq affinity: cpu <n>` folds; the source token `(source: /proc/irq)` keeps its bytes. | FR-017 |
| the attribution delta figures, and the `not available` wording | The figure after `interrupts delta:`, after `softirq <kind> delta:`, and each schedstat delta folds; the kind word and the source tokens `(source: /proc/interrupts)`, `(source: /proc/softirqs)`, `(source: /proc/schedstat)` keep their bytes. | FR-017 |

## What still fails

A renamed field, a renamed source, a renamed softirq kind, a missing
or added line, and a reordered key fold different, the way the
standing values do.

## Fixture pairs

`test/example-drift-fixture/run_report_shape_fixtures.py` carries the
golden pairs of the table above, checked in both directions, beside
the standing pairs (S-5 of specs/018).
