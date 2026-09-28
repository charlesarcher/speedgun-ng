# Data Model: Standalone Counters Library

**Feature**: `007-counters-and-timers` | **Date**: 2026-09-25 | **Plan**: [plan.md](plan.md)

Entities per spec "Key Entities", with fields, validation rules, and state transitions. Requirement ids cite [spec.md](spec.md); mechanism ids cite [research.md](research.md).

## E-01 System

The root handle to the local machine's counting world.

| Field | Type / shape | Notes |
|---|---|---|
| `providers` | ordered list of registered providers | accepted pre-open only (FR-009) |
| `objects` | object tree, root is the machine object (FR-001, FR-004) | canonical paths unique per parent |
| `catalog` | merged view over all object catalogs | immutable after open (FR-009) |
| `open` | boundary state | see state table |

Validation: registering a provider after open is a recoverable error (FR-009); duplicate object path under one parent and duplicate counter name within one object are recoverable errors (FR-008 edge case, US3 scenario 6). Concurrent catalog reads post-open are safe by construction (R-015).

State transitions:

```text
configuring --(first use / explicit open; provider registration closed)--> open(immutable)
```

## E-02 Countable object

| Field | Type / shape | Notes |
|---|---|---|
| `kind` | string (`machine`, `package`, `core`, `uncore_imc`, `amd_iommu`, `menagerie`, provider-defined) | FR-001 |
| `path` | canonical structured path, e.g. `package-1/core-3` | canonical spelling everywhere (FR-002, A24) |
| `alias` | optional platform instance name, e.g. `uncore_imc_0` | resolves to the same object (FR-002) |
| `description` | string | FR-001 |
| `parent` | link, absent only for machine | tree (FR-001) |
| `counters` | catalog of named entries | every counter belongs to exactly one object (FR-004) |

Validation: `objects(kind=..., attribute filters)` returns exactly the matching objects (FR-003); path or alias resolution failure is a recoverable error with did-you-mean suggestions (FR-008).

## E-03 Catalog entry

| Field | Type / shape | Notes |
|---|---|---|
| `name` | string, unique within the owning object | FR-005 |
| `description` | string | FR-005 |
| `unit` | unit string plus its dimension mapping | closed switch: recognized unit to `time^T x events^C`; unrecognized is a resolution error (FR-017) |
| `availability` | `countable` / `permission_blocked` / `not_encodable` / `absent` | described-ness from data and countability from probe are separate predicates (FR-006) |
| `read_mode` | achieved mode where applicable: `fast_tsc` / `fast_rdpmc` / `syscall` / `push_load` | recorded by the provider's enumeration-time probe, before the catalog freezes at the open boundary (FR-023); disclosed per catalog entry |

Validation: user code branches on this state alone; no compile-time platform branching in the public API (FR-007).

## E-04 Provider (contract entity)

| Field / operation | Shape | Notes |
|---|---|---|
| `enumerate` | objects plus entries (name, description, unit mapping, availability) | setup-time; type-erased registration (R-004) |
| `open` | window reader yielding one cumulative `uint64` point per managed leaf per sampling action; unit, description, availability, and the multiplex pair are catalog and plan facts reachable after measurement | FR-011, FR-026 |

Validation: a new source adds catalog entries plus the window implementation and touches no core file (FR-012; proven by the giraffe example and the out-of-tree provider test).

## E-05 Counter / expression

| Field | Type / shape | Notes |
|---|---|---|
| resolved leaf (`counter<D>`) | leaf-slot index plus compile-time dimension tag `dim<T, C>` | produced by name resolution (E-03) |
| composite (`expression<D>`) | typed tree at construction; compiled to a fold program by the plan | `+`/`-` identical tags; `/` subtracts exponents; scalar scale unrestricted (FR-015) |
| dimension tag | `template<int T, int C>` | construction-time only, erased on the read path (FR-016) |

Validation: dimension violations are compile errors (R-003); zero-leaf expressions and empty plans are construction-time recoverable errors (spec edge case); expression construction performs zero hardware reads (FR-021).

## E-06 Point and delta

| Concept | Definition | Notes |
|---|---|---|
| point | one leaf's cumulative `uint64` reading at an instant | the only reading any leaf ever yields (FR-011, A25) |
| delta | `point[j] - point[i]`, unsigned modular arithmetic at `2^64` | a single hardware wrap subtracts out (FR-013); only deltas carry dimensions or enter the algebra (FR-014) |
| monotonicity | a backwards (non-modular) move for a push leaf is a contract violation at fold time | FR-035; hardware leaves are the wrap case by modular arithmetic (spec edge case) |

## E-07 Plan

| Field | Shape | Notes |
|---|---|---|
| leaf slots | slot id, provider read descriptor, point-column offset | flat; read path holds no tree and no name lookup, and enters a window through the direct-call thunk its constructor installed, so a window installing no thunk pays one vtable lookup per sampling action (FR-022) |
| group layout | PMU leader per PMU, members attached | every group and read mode in a plan opens against the one target that plan bound, so a target or clock-identity mismatch across group members is unrepresentable (FR-024) |
| read modes | per-leaf achieved mode from probe | `fast_tsc` / `fast_rdpmc` / `syscall` / `push_load` (FR-023) |
| fold program | per composite: column references with exponents and ops | evaluated only on demand (FR-021) |
| arena geometry | capacity x columns layout | allocated at construction (FR-029) |
| targeting binding | thread or cpu | bound at plan open; plan is a per-thread object (FR-031) |
| overhead calibration | min/median/max distribution of the plan's `sample()` cost | FR-032; R-014 |

Validation: every group and read mode in a plan opens against the one target that plan bound, so a target or clock-identity mismatch across group members is unrepresentable; the construction errors a single target still fails on are a leaf the catalog reports as not `countable` and a window a provider refuses to open, both recovered before any hardware read (FR-024). State transitions:

```text
compiling --(finalize: arena + calibration)--> bound(thread/cpu) --(per-thread use)--> in use
```

## E-08 Recorder

A value handle (pointer into the plan arena, head index, policy state; trivially copyable, FR-029) over `capacity` point columns.

| Field | Shape | Notes |
|---|---|---|
| `capacity` | point columns, fixed at construction | capacity 1 holds one sample; folds require at least 2 (FR-025, clarification 1) |
| `policy` | `hard_stop` (default) or `ring`, chosen by factory tag | compile-time policy (FR-025, R-006) |
| `head` | write index | ring update masks branchlessly (FR-028) |
| `wrapped`, `dropped` | ring bookkeeping | folds must consult `dropped` (FR-028) |

Validation: `sample()` is `noexcept`, zero allocation, zero lock (FR-026); under `hard_stop` a sample past capacity aborts in every configuration (FR-027); a non-power-of-two ring capacity fails construction recoverably (R-006); every recorded column satisfies the one-sampling-action invariant under one plan binding (FR-047). State transitions:

```text
empty --(sample)--> filling --(reaches capacity)--> full
full (hard_stop) --(sample)--> contract violation (abort, all configs)
full (ring) --(sample)--> wrapped (dropped count increments per overwrite)
```

## E-09 Metric result

| Field | Type | Notes |
|---|---|---|
| `value` | `double` | fold output; promotion at first division/scaling, exactness note beyond `2^53` documented (spec Assumptions) |
| `running_ratio` | `double` | from enabled/running deltas when the source has that pair; sources without one disclose `1.0` (FR-019, clarification 2); composite ratio is the product of constituent ratios each raised to its algebraic exponent |
| `scaled` | `bool` | false for ratio-1.0 sources; structurally impossible to omit (FR-019) |

Fold API: `fold(rec, i, j)` window, `fold_pairs(rec)` per-interval series, first-to-last fold; valid ranges require `i < j` within the recorded extent, tier-3 checked (FR-018). Folds never read providers and are callable any number of times for any subset (FR-021).

## E-10 Provenance record

Per-leaf raw view exposed by every composite (FR-020):

| Field | Notes |
|---|---|
| object path | canonical spelling (FR-002) |
| name, description, unit | from the catalog entry |
| raw values | the leaf's point column |
| point identity | index within the recorder |
| multiplex ratio | the leaf's ratio disclosure |

## E-11 Event table record

The vendored tree's provenance manifest (`external/pmu-events/RECORD`):

| Field | Notes |
|---|---|
| upstream ref + date + URL | FR-044 |
| per-file sha256 | verified by `--check` (FR-045) |
| exclusion list | explicit, recorded (FR-044) |
| gate constant agreement | configure-time tripwire plus `--check` exit 1 on drift (FR-043, R-012) |

## Cross-entity invariants

1. One window, one truth: all leaves of a compiled plan are read within a single sampling action, so any composite folded from one recorder sees identical deltas (spec correctness promise 1; FR-047 `SG_INVARIANT` per column).
2. No number without provenance: every metric result carries ratio and scaled fields, and every composite exposes constituent columns with E-10 records (spec correctness promise 2; FR-019, FR-020).
3. The recorder owns the buffer; composites are parsers over it (A25): folds consume columns of a single recorder only (FR-047).
4. Clock leaves attach to the machine root (FR-004); cross-object composition is legal under the shared window (US3 scenario 4).
