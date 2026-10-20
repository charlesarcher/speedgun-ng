# Contract: Preset placement of execution contexts

Phase 1 output. The rule the layout read, `resolvePreset` and the runner
follow for every threaded run (FR-001 to FR-019).

## The layout read

| Rule | Requirement |
| --- | --- |
| The harness reads the machine layout at most once per process, on the first need, through the vendored hwloc, inside the one harness unit that includes `<hwloc.h>`. | FR-001, FR-004 |
| The layout names packages, L3 groups, cores and threads, each ordered by ascending cpu index, and the mask it was built from. | FR-001, FR-002, FR-003 |
| No installed header names the layout type. | FR-003, FR-031 |
| The read covers the intersection of the process affinity mask the operating system reports and the hwloc topology set. hwloc's own sets follow the machine and the control group; the process affinity is a separate fact the harness reads itself, so a restricted process sees a smaller layout. | FR-005 |
| A test can install a layout, and an installed layout wins over the read. | FR-006, Q-5 |
| An install takes effect before the first read of that process; the read itself stays a once-per-process event, so a scenario that needs its own read takes a fresh process. | FR-002, FR-006 |

## The preset table

| Preset | Place for context index `i` | Places for a layout |
| --- | --- | --- |
| `none` | no cpu, no pin | unlimited |
| `core` | the lowest-index thread of the `i`-th core, cores ascending by that thread | the core count |
| `l3` | the lowest-index thread of the `i`-th L3 group, groups ascending by their lowest thread | the group count |
| `smt` | the `i`-th sibling of the core holding the lowest-index thread of the mask | the sibling count of that core |
| `compact` | the next free thread: all siblings of a core before the next core, all cores of a group before the next group | the thread count |

Every walk orders by ascending cpu index inside the group it is walking
(FR-009).

## Resolver contract

```text
resolvePreset(Preset preset, const MachineLayout& layout, int contexts)
  \pre contexts >= 1
  \post `places` equals the preset's place count for the layout
  \post `cpu` stands empty exactly when contexts exceeds `places`, and that
        empty plan is where the fit message reads the count it prints
  \post otherwise `cpu` holds `contexts` distinct cpus, each inside the
        layout's mask, and the group ordinal of each
```

- `SG_REQUIRE` on `contexts >= 1`: a run reaches the resolver only with a
  context count above 1, and the resolver keeps the bound for any other
  caller (Principle II).
- A run whose preset is `none`, and a run with no preset at all, never
  reaches the resolver: the batch fit check skips them and the report
  prints no topology line (FR-010, FR-018, FR-022).
- `SG_ENSURE` that the returned cpus are pairwise distinct (FR-008).
- Determinism: one layout and one count produce equal vectors on every
  call (FR-009, SC-03).

## Fit and stop rule

| Situation | Behavior |
| --- | --- |
| `contexts` at or below the place count | the run proceeds with the plan |
| `contexts` above the place count, over a run of more than one context | one line naming the preset, the place count, and the requested count; exit status 2; no context block prints, no context starts and no row prints (FR-018, Q-7) |
| A family whose counts include an unfittable count | the check runs over the whole selected batch before the first run starts, and the process stops the same way (Clarifications Q-4) |
| An unknown preset name on either option | one line, exit status 2 (FR-016) |
| A value naming cpus, a list, or a range | one line, exit status 2 (FR-017) |
| The layout cannot be read and a preset stands in effect | one line, exit status 2 (FR-005) |
| The layout cannot be read and no preset stands | the run behaves as feature 020 ships (FR-005) |

The line goes to standard error, the standing rule for a bad option
value, and the process exits with status 2 (spec Constraints).

## thrawn fit limits

Measured at the audit point from sysfs (spec PC-7), for a process with
the full mask:

| Preset | Places |
| --- | --- |
| `l3` | 4 |
| `smt` | 2 |
| `core` | 24 |
| `compact` | 48 |

A narrower mask lowers each number to what the mask holds (FR-005).

## Scope of the rule

- A run of one context over a readable layout keeps feature 020's behavior:
  no context exists to place (FR-019, Q-6, Q-7).
- A context a caller creates through the public API keeps naming its own
  cpu through `PlacementSlot::cpu`: the preset rule covers harness runs
  only.
- No preset names a memory node (spec Scope, N12).

## Consumers

- `test/source/harness_topology_test.cpp` drives the read, the two install
  seams and the mask intersection.
- `test/source/harness_placement_preset_test.cpp` drives the preset table and
  the fit rule over installed layouts.
- `source/harness/runner.cpp` turns the plan the resolver returns into the
  context policies of one threaded run (FR-008, FR-010, FR-019).
