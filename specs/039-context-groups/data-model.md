# Data model: the context groups (specs/039)

The lane adds one record, one derived value and one report key. The
standing plan and layout records carry everything else it reads.

## The types the feature adds

- **`sg::detail::ContextGroup`** (`source/harness/detail/internal.hpp`):
  `name` (a `std::string`, D-4) and `slots` (a
  `std::vector<std::size_t>`, ascending context positions). Aggregate
  members take camelBack (N-12). A group holds at least one slot
  (FR-002).
- **The partition**: a `std::vector<ContextGroup>` in first-appearance
  order of slot `0`, then the next slot no earlier group holds. An
  empty vector states no partition, the shape FR-004 keeps.
- **`sg::ContextDisclosure::group`**
  (`include/speedgun-ng/benchmark.hpp:454-462`): the group name of one
  context, empty where the run carries no partition. The record stays
  an aggregate; the member rides last (FR-005, FR-029).

## The standing types the feature reads

- **`detail::PlacementPlan`** (`detail/internal.hpp:143-152`): `cpu`,
  `group`, `node`, `places`. `group` holds the L3 group ordinal, `node`
  the memory node number, and `kNoGroup` (`internal.hpp:51-52`) states
  no unit. The partition reads `group` and `node` and writes nothing
  (D-3, FR-009).
- **`detail::Preset`** (`detail/internal.hpp:124-132`): the six names;
  the partition's first argument (FR-006, FR-007).
- **`sg::ContextBarrier`** (`include/speedgun-ng/execution_context.hpp:656`):
  the round mechanism, one object per group per point (P-2).
- **`detail::MachineLayout`** (`detail/internal.hpp:106-119`): the
  groups and nodes the preset plan walked, read by the tests that
  install a shape.

## The group naming

| Source | Key per context | Name | Example |
| --- | --- | --- | --- |
| `core`, `l3`, `smt`, `compact` | `plan.group` | `l3-` plus the ordinal | `l3-2` |
| `node` | `plan.node` | `node-` plus the number | `node-1` |
| layout entry | the entry's `slots` | the entry's `name`, else `g<index>` | `fast`, `g0` |
| `none`, `kNoGroup`, one context | none | no partition | none |

## The layout grammar

```yaml
groups:
  - name: fast        # optional, 1 to 32 of [A-Za-z0-9._-], unique
    slots: [0, 2]     # required, ascending not required, covers once
  - slots: [1, 3]     # names g1 by document order
```

One root mapping, one `groups` key, at least one entry, one `name` and
one `slots` per entry. No nesting, no anchor, no second document, no
other key (FR-012, FR-017).

## The report key

| Key | Path | Type | Presence |
| --- | --- | --- | --- |
| `group` | `/benchmarks/i/contexts/j/group` | string | only where the run carries a partition |

The schema at `docs/schema/report-v1.json` carries it as an optional
key of the context item, which stays closed under
`additionalProperties: false` (FR-030, R-4).
