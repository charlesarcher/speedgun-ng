# Contract: the placement disclosure of a bound context

Amendment of `specs/025-topology-placement/contracts/report-topology.md`
(specs/035). The standing field stands unchanged except where this
file amends it.

## The placement field

The field composes in
`source/harness/placement_preset.cpp:477` (`placementText`, beside
the layout the run reads); `source/harness/report.cpp:156` stands
the print and width-cut site the compose feeds:

| Case | Field bytes |
| --- | --- |
| no pin | the standing no-pin text, unchanged |
| pinned, no node plan | `cpu@group`, the standing bytes, unchanged |
| pinned under the `node` preset | `cpu@group@node` |

| Rule | Requirement |
| --- | --- |
| The node stands last, joined by the same `@` separator, named by the number the hwloc read states. | FR-011 |
| The field carries no new key in the JSON context object; the schema minor stays `1.6`. | FR-004, D-5 |
| A run with no node preset keeps every field byte-identical to master apart from the version figures the report carries. | FR-002 |

## Console row

The context rows print the field in the standing column; a bound
context's field grows the node tail and no other byte of the row
moves.

## Example

```text
ctx0=SPIN/3@1@0/applied
```

The row reads `ctx<i>=<wait>/<placement field>/<applied flag>`, the
standing shape of `printContextDisclosure` (`report.cpp:167`), with
the placement field carrying the node tail; the account words of
spec 020 stand unchanged, and the fixture pairs of the shape gate pin
the folded bytes.
