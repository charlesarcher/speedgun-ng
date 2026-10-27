# Contract: the `precheck` JSON record

Phase 1 output. The top-level object the JSON document gains
(spec FR-006, FR-024, D-8), standing after `jitter` and before
`benchmarks`. The schema `docs/schema/report-v1.json` carries the
keys and the moved top-level `x-order`; the minor moves to `1.6` in
the same commit. `loadReport` reads the record back through the
standing simdjson path; the harness checker reads every written
document against the key set, the key order and the value kinds.

## Key order

```json
["schemaVersion", "speedgun", "run", "jitter", "precheck", "benchmarks", "hunt"]
```

The top-level `x-order` list of the schema moves to carry
`precheck` after `jitter`; the list above is the standing top-level
order with the new member in place.

## The `precheck` object

| Key | Kind | Meaning |
| --- | --- | --- |
| `serial` | boolean | the `precheck=serial` flag |
| `windowMs` | integer | the window length, at or above 1 (the `precheck=` value or the 200 ms default) |
| `cpus` | array | one entry per measured cpu, ascending, planned cpus before candidates |
| `hostSettings` | object | the once-per-run disclosure |
| `skippedGroups` | array | the group indexes the candidate resolution left, ascending |

## A `cpus` entry

| Key | Kind | Meaning |
| --- | --- | --- |
| `cpu` | integer | the cpu index |
| `role` | string | `planned` or `candidate` |
| `stats` | object | the sixteen standing stats fields of the console probe block, same names, same kinds as the `jitter` stats object of specs/027 |
| `attribution` | object | the window deltas, each figure carrying its source name |

## The `attribution` object

| Key | Kind | Meaning |
| --- | --- | --- |
| `interrupts` | integer or `null` | the window delta of the cpu's interrupt count; source `/proc/interrupts` |
| `softirqs` | object | one entry per kind the file states, integer or `null` per kind; source `/proc/softirqs` |
| `switchDelta` | integer or `null` | the run-queue delay figure; source `/proc/schedstat` |
| `scheduleDelta` | integer or `null` | the timeslices figure; source `/proc/schedstat` |

A delta of zero is a figure like any other; `null` stands only for a
source that is missing, unreadable or unparsable, the file form of
the console's `not available` (spec FR-018).

## The `hostSettings` object

| Key | Kind | Meaning |
| --- | --- | --- |
| `isolcpus`, `nohz_full`, `rcu_nocbs` | string or `null` | the kernel command line's settings, `null` where the host does not state them |
| `governors` | object | one entry per measured cpu, its key the cpu index; string or `null`; source the `scaling_governor` file |
| `irqAffinity` | object | one entry per measured cpu, its key the cpu index; the comma-joined IRQ list or `null`; source `/proc/irq` |

## Rules

- A run without the pre-check writes no `precheck` key; the document
  matches master's apart from the `schemaVersion` value (spec
  FR-024, SC-007).
- The compare flows read `schemaVersion` alone for the major check;
  the new top-level key stands beside it (specs/026 D-13).
- The schema test fails when a document carries a duplicate key or
  an order the `x-order` list does not state.
