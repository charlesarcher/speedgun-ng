# Data Model: NUMA Memory-Node Placement

Feature: specs/035-numa-placement. The entities stand beside the
standing placement model of `specs/025-topology-placement/data-model.md`;
this file names what the feature adds and what it changes, and the
spec's decisions D-1 to D-7 own the shapes.

## Added entities

### `LayoutNode` (detail, `source/harness/detail/internal.hpp`)

One memory node the machine read states.

| Field | Kind | Rule |
| --- | --- | --- |
| `number` | unsigned integer | the node number hwloc states, in the read's order |
| `cpus` | sorted cpu indexes | every pu of the node the read states, unfiltered; the mask filters at plan and listing time |

Validation: the numbers stand unique, in ascending order of first
appearance in the read; the cpu sets of two nodes never share a cpu
(the hwloc model states disjoint node cpus); a node with an empty
cpu set stands in the vector and the plan gives it no context.

### `MachineLayout` extension (detail)

| Added field | Kind | Rule |
| --- | --- | --- |
| `nodes` | `std::vector<LayoutNode>` | empty when the read states no memory information |
| `memoryReadable` | bool | false when the read states no memory information; the `not available` paths read it |

The standing fields (`threads`, `cores`, `groups`, `packages`,
`mask`, `readable`) stand unchanged.

### `Preset::NODE` (detail enum row)

One row beside the five standing rows. The parse matches the name
`node` in full and nothing else; the standing names keep their
bytes.

### The node plan (resolver output, detail)

Per context of a threaded run: the planned cpu index and the node
number that cpu belongs to. The resolver's postcondition predicate
extends the standing `planCpusDistinct`: every planned cpu stands
inside the process mask, stands on the node the context binds to,
and two contexts of the cycle never share a cpu.

Fill and cycle rules (D-2):

- context count greater than the node count: the nodes cycle in the
  read's node order, the way the `l3` plan repeats its groups;
- context count equal to the node count: one context per node;
- context count below the node count: the nodes `0` to `k-1` of the
  read's order take one context each, the later nodes take none.

### The binder seam (detail, `source/harness/detail/memory_bind.hpp`)

The installer type, the clear-on-null shape `installTestLayout`
uses (`topology.cpp:306`):

| Member | Rule |
| --- | --- |
| install | a test installs a binder that records the node the call names, or denies the call on demand |
| clear | a null installer restores the production binder |
| production binder | one hwloc membind call by node, thread scope, on the calling thread, inside the one hwloc translation unit |

Validation: the binder runs only for a context the plan bound; a
context whose plan carries no node binds nothing and the call never
fires.

### The disclosure text (console and JSON)

The placement field of a context:

| Shape | Case |
| --- | --- |
| `cpu@group@node` | the plan bound the context to a node |
| `cpu@group` | the standing bytes, unchanged, for every other run |
| the standing no-pin text | the slot names no cpu, unchanged |

Validation: the node stands last, joined by the same `@` separator;
the group and the node stand as numbers the read states; no new key
stands beside the field in the JSON context object.

## Changed entities

### The topology listing

One line per node under the L3 lines:

| Line | Case |
| --- | --- |
| `node <n>: <cpu-list>` | the read states the node and the mask touches its cpus |
| `not available` | the read states no memory information: the node lines print the wording where the cpu lists would stand, and the rest of the listing keeps its bytes; the exact wording stands in `contracts/cli-placement.md` and the fixture pins it |

### The refusal set

| Message shape | Case | Status |
| --- | --- | --- |
| one line naming the context and the node | the binder denies a planned context's bind | 2 |
| one line naming the preset | the `node` preset stands over a read with no memory information | 2 |
| the standing unknown-preset line | a name outside the six | 2 |
| the standing unfittable-count line | the node plan cannot fit the contexts inside the mask | 2 |
| the standing unreadable-layout line | the machine read fails outright | 2 |

## State transitions

None. The layout, the plan and the binder hold no lifecycle: the read
stands one time per run, the plan resolves before the first context
starts, and the bind fires at each context start. The refusal paths
stand ahead of the first benchmark, so no partial state survives a
refused run.
