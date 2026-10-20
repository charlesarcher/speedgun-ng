# Data Model: Topology Placement

Phase 1 output. Every type here is internal to the harness archive: no
type in this file is declared in an installed header (FR-003, FR-031).
The one public addition is a member function on the existing
`BenchmarkHandle`, listed in plan.md under "Public API surface added".

## Entity map

```text
MachineLayout 1 ──── * LayoutGroup 1 ──── * LayoutCore 1 ──── * LayoutThread
      │                                          │
      │ one per process, cached                  └── groupOrdinal back-ref
      └── mask: the process affinity mask the operating system reports,
          intersected with the hwloc topology set

Preset (5 names) + MachineLayout + contextCount ──▶ PlacementPlan 0..1
                                                        │
                                                        └── one cpu per context index

RunOptions ──placement, topologyMode──▶ speedgunMain
Registry entry / Instance ──placement──▶ Runner ──▶ ContextPolicy
```

## E-01: `MachineLayout`

The layout of one process, read once and reused (FR-001, FR-002).

| Field | Type | Rule |
| --- | --- | --- |
| `threads` | ordered vector of `LayoutThread` | ordered by ascending cpu index; every cpu sits inside the process mask (FR-005) |
| `cores` | ordered vector of `LayoutCore` | ordered by the lowest cpu index of the core |
| `groups` | ordered vector of `LayoutGroup` | ordered by the lowest cpu index of the group; the L3 cache unit of the host (FR-012) |
| `packages` | count | the package count inside the mask (FR-020) |
| `mask` | cpu set | the process affinity mask from `sched_getaffinity` intersected with the hwloc topology set; the walk skips every processing unit outside it (FR-005), and it is kept so a test can assert the mask rule and the listing can print it |
| `readable` | bool | false only when the host read failed; an installed layout always stands readable (FR-005) |

Validation: a thread names a core when the read finds cores, and a core
names a group when the read finds an L3 cache; a description without
either leaves that vector empty and the matching ordinal of every thread
holding `kNoGroup`, the value `std::numeric_limits<std::uint32_t>::max()`
that `kNoPin` of `include/speedgun-ng/execution_context.hpp:100` holds for a
cpu index. Every group holds at least one core, and
when the read finds cores the thread count equals the sum of the sibling
counts; a read that finds no core leaves `cores` empty and the thread count
stands on its own, the shape the synthetic description `1` yields. The
counts the report prints come from these vectors and nowhere else.

## E-02: `LayoutThread`

| Field | Type | Rule |
| --- | --- | --- |
| `cpuIndex` | `std::uint32_t` | the operating-system cpu index, the value a `PlacementSlot` carries |
| `coreOrdinal` | index into `MachineLayout::cores` | the core this thread is a hardware thread of |
| `groupOrdinal` | index into `MachineLayout::groups` | the L3 group that owns the core |

## E-03: `LayoutCore`

| Field | Type | Rule |
| --- | --- | --- |
| `groupOrdinal` | index into `MachineLayout::groups` | the group holding this core |
| `threads` | ordered vector of `LayoutThread` indexes | ascending cpu index, so the first entry is the lowest-index sibling (FR-011, FR-013) |

## E-04: `LayoutGroup`

| Field | Type | Rule |
| --- | --- | --- |
| `lowestCpu` | `std::uint32_t` | the lowest cpu index in the group, the ordering key of `l3` (FR-012) |
| `cores` | ordered vector of `LayoutCore` indexes | ascending, the walk order of `compact` (FR-014) |

## E-05: `Preset`

An enumeration with five values and one parser.

| Value | Source of the name | Places |
| --- | --- | --- |
| `NONE` | the default, feature 020's behavior | unlimited |
| `CORE` | OpenMP `OMP_PLACES=cores`, Slurm `--cpu-bind=cores` | one per core |
| `L3` | hwloc L3 cache object | one per group |
| `SMT` | the sibling set of one core | one per sibling of that core |
| `COMPACT` | `KMP_AFFINITY=compact` | one per thread in the mask |

Parsing rule: the text must equal one name in full. A text that names
cpus, a list, or a range is refused with the same one-line status 2 stop
as an unknown name (FR-016, FR-017).

## E-06: `PlacementPlan`

The answer of `resolvePreset` for one layout and one context count
(FR-008, FR-009).

| Field | Type | Rule |
| --- | --- | --- |
| `cpu` | vector of `std::uint32_t`, one per context index | all distinct; the value handed to `PlacementSlot::cpu` |
| `group` | vector of group ordinals, one per context index | the group of `cpu[i]`, the value the disclosure prints |
| `places` | count | the preset's place count for this layout, the number the fit message prints (FR-018) |

Postcondition: an empty `cpu` means `contextCount` exceeds `places`; when it
does not, `cpu.size() == contextCount`, every entry distinct, and `cpu[i]`
equals the `i`-th place of the preset's ordered walk, the shape the resolver
contract of `contracts/topology-placement.md` states. Two calls
with one layout and one count return equal vectors (SC-03). A preset whose
walk finds no object of its kind has no place to give: `places` stands 0 and
`cpu` stands empty, the shape the fit message prints (FR-018). `compact`
walks threads, so only a layout with no thread leaves it empty (FR-011 to
FR-014).

## E-07: `RunOptions` additions

The internal option carrier `source/harness/detail/internal.hpp:324`
gains:

| Field | Type | Rule |
| --- | --- | --- |
| `placement` | `std::optional<std::string>` | the raw `--placement` text; parsed once, before the first run |
| `topologyMode` | `bool` | `--topology` was given: print the layout, exit 0, run nothing (FR-036) |

## E-08: Registry entry and Instance additions

A family carries its placement choice from the handle to every instance
the expansion produces, the way the thread count set travels today.

| Field | Type | Rule |
| --- | --- | --- |
| entry `placement` | `std::optional<std::string>` | set by `BenchmarkHandle::placement`; the last call wins, the way the thread calls replace an earlier set |
| instance `placement` | resolved preset, or the process default | the handle value wins over the command-line value (FR-015); an unresolved name stops the run with status 2 (FR-016) |

## E-09: Disclosure record additions

The record feature 020 already carries (`waitMode`, `placement` text,
`applied`) keeps its shape. Only the text changes: `NO_PIN` when no slot
names a cpu, `cpu@group` when one does (FR-021). The printer adds nothing. A
thread whose group ordinal stands at `kNoGroup` renders the cpu index alone,
with no `@` part, and the standing cut of `kPlacementFieldWidth` still bounds
the field (FR-021, feature 020 FR-025).

## E-10: Layout listing (the `--topology` output)

One block, printed and then exit 0 (FR-036):

```text
topology: packages=1 l3-groups=4 cores=24 threads=48 mask=0-47
l3 0: 0-5,24-29
l3 1: 6-11,30-35
l3 2: 12-17,36-41
l3 3: 18-23,42-47
```

The counts equal the topology line of a preset run on the same host
(SC-12).

## State transitions

```text
layout:  unread ──first need──▶ host read ──▶ cached
            │                     │   └── installTestTopologyDescription can
            │                     │        replace the description it reads
            │                     └── failure ──▶ unreadable (preset run exits 2)
            └── installTestLayout ──▶ installed (wins, no host read)
```

An install takes effect only before the first read of the process, so a
scenario that needs its own read takes a fresh process. No layout mutates after the read; a threaded run never writes it.

## What is deliberately absent

- No NUMA node field, in the layout, the plan, the report or the tests
  (spec Scope: N12 owns memory nodes).
- No cpu-list or cpuset input type (FR-017, spec Scope).
- No public type: the whole model lives behind
  `source/harness/detail/internal.hpp`.
