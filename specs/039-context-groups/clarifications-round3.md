# Spec 039 clarification questions, round 3

Feature: `039-context-groups`, branch `039-context-groups`. Method: the
speckit-clarify discipline, a third pass over the twice-amended spec, no
spec edit. Round 1's five answers and round 2's two stand in the spec's
Clarifications section, carry the weight of a Decision, and stand
unasked here. Round 2 dropped fourteen candidates, and this pass names
none of them again. The lane rulings R-1 to R-5, the constitution,
`plan.md`, `tasks.md` and the two files under `contracts/` stand outside
the question set.

The bar this pass holds: a question names a split that moves scope, a
user-visible byte, the public surface, or a required test's shape. It
stands checked against the tree with a `path:line` citation. No clause
of the spec, the plan, the contracts, the constitution or a ruling
answers it. One question clears that bar, and the pass reads one.

## Q1: The pairing where a stated preset names no partition

**Spec section (quoted, FR-006, FR-019, FR-028):**

> Where the machine read states no such unit, the plan carries `kNoGroup`
> (`source/harness/detail/internal.hpp:51-52`), and the preset SHALL name
> no partition: the run keeps the ungrouped shape of FR-004.

> - **FR-019**: A layout file given beside a placement preset that names
>   a partition SHALL refuse one line naming both, with exit status 2; a
>   layout beside `--placement=none` SHALL stand allowed. The check reads
>   each selected instance's resolved preset, the name a handle stated
>   (`registry.cpp:552-563`, `family.cpp:374-386`) carrying the same
>   refusal as the name the option settled, and it reads at the standing
>   fit checkpoint before any instance runs (FR-020).

> The standing machine line keeps its rule of printing where the fit step
> reports a preset in effect (`cli.cpp:744-755`), and a run grouped by a
> layout SHALL print none.

**Question:** FR-019's refusal keys on the words "a placement preset
that names a partition". FR-006 states a class where a stated preset
names none, because the read states no unit for it. The fit step's
in-effect flag turns on for any resolved name other than `none` over
more than one context (`source/harness/placement_preset.cpp:636-657`).
That flag reads no unit fact, and `source/harness/cli.cpp:744-755`
prints the machine line from it. The standing seam builds a read that
states cores and no L3 unit
(`test/source/harness_topology_test.cpp:402-410`), and the `core` and
`smt` arms still plan cpus there. Such a run beside a layout file sits
outside both rows FR-019 wrote, and FR-028 promises it a machine line
and none at once. Does that pairing refuse, or does the run group by the
file beside the preset's pins (FR-006, FR-019, FR-028)?

**Why it matters:** the answer moves one console line's bytes in one run
shape, and it decides whether FR-028's last clause stands whole. It
settles whether that pairing case joins the refusal set SC-004 counts
and the rows of `test/source/harness_group_cli_test.cpp` (T010).
The same reading settles a second shape: a one-context instance beside
the name of a partition-naming preset. There the two readings answer
exit 2 and exit 0.

| Option | Reading | Implications |
|---|---|---|
| A | The refusal keys on the resolved name, so any name other than `none` refuses beside a layout file, and the file stands alone with `none`. | FR-019 gains FR-006's class, one case joins SC-004 at 19, and T010 gains that refusal row. FR-028's two clauses never meet, and no recorded sample moves. The line names a pairing whose stated reason, two sources of one partition, does not stand there. |
| B | The refusal keys on the partition that preset names for that instance, so the pairing stands allowed and the file is that run's one source. | FR-028's last clause narrows to the run the fit step leaves with no preset in effect, and the machine line keeps one rule. SC-004 stays 18, and T023's absent-line row names the no-preset case. The line prints beside a grouped disclosure over a read that states no L3 unit, a shape no docs sample runs. |
| C | The refusal keys on the partition, and the file wins the disclosure gate, so every layout run prints no machine line, the pinned one included. | FR-028's last clause stands whole, and FR-033's byte rule stands untouched. The standing rule gains its first exception, and the console states cpus a preset pinned with no machine counts beside them. |

**Recommended:** Option B. D-2 bars two sources of one partition, and a
preset that names no partition names none. That run keeps one source and
earns no refusal. The machine line's standing reason is a preset that
planned cpus on the machine (`clarifications-round1.md`, Q5), and that
reason holds in this shape. FR-019's text already keys on the words
"names a partition". The reading narrows one clause of FR-028 and leaves
the refusal set at eighteen cases. It moves no recorded byte on a host
that states an L3 unit.

## Considered and dropped

- A plan that names a unit at some positions and stands `kNoGroup` at
  others: the read assigns each ordinal through `ordinalOf`
  (`source/harness/topology.cpp:112-125`,
  `source/harness/topology.cpp:190`). FR-002 and A-1 of
  `contracts/group-partition.md` bar a partition that leaves a context
  outside a group. FR-006 names no partition where the plan carries
  `kNoGroup`, and FR-004 then keeps the ungrouped shape. Round 1's Q3
  already refused a name read off the sentinel.
- Where the grouped attempt's extra teardown dispatch and its extra node
  poll sit against the attempt-level stamp: the stamp wraps
  `launchBodies` at `source/harness/runner.cpp:2189-2191`. FR-024 keeps
  the standing round rules, and `plan.md` places one publication slot
  and one extra node poll outside every window. T020 seats the teardown
  node after `launchBodies` returns. Round 1's Q2 states the rule and
  leaves the mechanism to `plan.md`.
- An anchor or an alias inside a layout document, with `slots` reading
  through it: FR-018's fault list stands closed. A-2 of
  `contracts/group-layout.md` bars a reader fault the grammar does not
  name. An alias the document never defines reads as a document the
  reader rejects, the first fault of FR-018.
- A `group` value of the wrong kind inside a stored document: the
  standing optional-key rule at `source/harness/json_load.cpp:79-85`
  keeps a present key to its own kind and its own refusal. FR-031 states
  the absent reading, and FR-030 moves the guard and the round-trip
  tests with the key.
- An empty path word on the new row: FR-018's first fault names an
  unreadable path, and the Edge Cases row names the standing open-fault
  text (`source/harness/options.cpp:1150-1174`). The empty-path line at
  `source/harness/detail/options.hpp:271-280` answers the other standing
  form. FR-021 governs both, and SC-004 names no empty-path case.
- Where the new key sits in the context item's order: the item stands
  closed at `docs/schema/report-v1.json:386-402`. FR-030 moves the guard
  with the key, and T026 moves the schema and the writer in one commit.
  `data-model.md` puts the record's member last.
- A group name reaching a user's `threadRunner` view or its `State`: the
  Out of scope row keeps `ThreadRun` as specs/020 fixed it. `plan.md`
  names one added member, and the group index rides an internal slot
  beside the body index (`source/harness/runner.cpp:724-733`).
- The group name against the placement field's cut: the cut reads that
  field alone (`source/harness/report.cpp:153-166`). FR-028 keeps the
  standing segments in the standing order, so the name prints whole.
- The comparison of two documents under the new key:
  `source/harness/compare.cpp:1753-1760` moves rows and the raw columns
  alone, so no comparison reads a context disclosure.
- The oversubscription warning under a partition:
  `source/harness/json_report.cpp:812-843` counts spinning contexts, and
  FR-002 keeps the context set whole, so no count moves.
- `plan.node` standing empty under an L3-keyed preset: D-3 names one
  unit per preset, and `source/harness/placement_preset.cpp:199-202`
  fills `node` for the `node` arm alone. The partition reads one vector
  per preset on that rule.
- `--threads` overriding the handle's counts beside a layout: FR-020
  reads any instance the batch runs, and
  `source/harness/cli.cpp:687-690` settles the counts ahead of the fit
  checkpoint (`source/harness/cli.cpp:744-747`). B-3 of
  `contracts/group-partition.md` reads after the option scan settled.

## Reconciliation

The orchestrator weighed the recommendation against the lane rulings and
the tree. Option B stands.

- Q1: Option B, adopted. The pairing refusal keys on the partition the
  resolved preset names for the read in hand (FR-019, D-2). A preset
  above `none` that names no partition, the `core` plan over a read
  that states no L3 unit, leaves the file as the run's one partition
  source, so the run stands allowed and the refusal set stays 18 cases.
  FR-028 narrows to the machine line's single standing rule: the line
  prints where the fit step reports a preset in effect, and a layout run
  that carries no preset prints none.
- R-2 reading: the ruling bars two sources of one partition. The literal
  shorthand "the layout beside a preset refuses" is narrowed here to the
  preset that names a partition, and the narrowing stands recorded in
  D-2, FR-019 and Assumptions. R-1, R-3, R-4 and R-5 stand untouched.
- The round-3 note on `clarifications-round2.md` stands accepted: that
  file now carries its own Reconciliation section, and its Q1 aside
  about the allowed pairing reads after this ruling.
