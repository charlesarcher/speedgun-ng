# Spec 039 clarification questions, round 2

Feature: `039-context-groups`, branch `039-context-groups`. Method: the
speckit-clarify discipline, a second pass over the amended spec, no spec
edit. Round 1's five answers stand recorded in the spec's Clarifications
section, carry the weight of a Decision, and stand unasked here. The lane
rulings R-1 to R-5 and the constitution stay outside the question set. Each
question below names a spot where two readings of the amended spec still
diverge on user-visible bytes or on the shape of a required test. No
Decision, Requirement, Assumption, Clarification entry or Edge Case stands a
default at such a spot. Two questions; the spec otherwise stands clear.

## Q1: The preset a layout stands beside when the family names it

**Spec section (quoted, D-2, FR-019):**

> - **D-2 (R-2)**: Two sources, one shape, one partition. The partition is
> one value the run carries, named by the placement preset or by the layout
> file. A layout file beside a preset that names groups refuses one line at
> exit status 2, the standing refusal shape (`hunt_cli.cpp:155` is the
> combination-refusal precedent). The `none` preset names no partition, so
> it keeps the standing shape.

> - **FR-019**: A layout file given beside a placement preset that names a
> partition SHALL refuse one line naming both, with exit status 2; a layout
> beside `--placement=none` SHALL stand allowed. A layout file given beside
> `--hunt` SHALL refuse one line naming both, the shape
> `--hunt cannot combine with --placement` keeps
> (`source/harness/hunt_cli.cpp:720-748`) (D-2, D-11).

**Question:** The two sources of one partition settle at different depths.
The precedent D-2 cites reads the settled option value alone
(`source/harness/hunt_cli.cpp:745`). A preset a family stated on its handle
reaches the run through the registry
(`source/harness/registry.cpp:552-563`), and the expansion lets that name
win over the process default (`source/harness/family.cpp:374-386`). A family
that states `l3` on its handle, beside `--placement=none` and a layout file,
then runs a preset that names a partition. FR-019 declares that pairing
allowed and names no second preset source: which preset does the refusal
read (D-2, FR-019, FR-003)?

**Why it matters:** the answer decides whether that pairing answers one
refusal line, or a run where two sources name one partition with no
precedence between them. The choice also moves the refusal set SC-004
counts, which names 18 command-line cases today.

| Option | Reading | Implications |
|---|---|---|
| A | The refusal reads the preset the selected instances resolved to, at the standing fit checkpoint, so a family's own name beside a layout answers the same one line. | `placementFitStatus` already walks the resolved instances before any run (`source/harness/placement_preset.cpp:631-638`, `source/harness/cli.cpp:744-747`), so one branch there keeps FR-018's promise that no instance starts; FR-019's allowed pairing then reads as every selected instance resolving to `none`; one family case joins SC-004. |
| B | The refusal reads the settled option value alone, the shape the hunt check keeps, and the spec adds which source wins when a family overrides it. | The 18 cases and the standing seam stay as written; the spec gains a precedence clause D-2 does not carry today, and a run can print a group set that neither named source explains. |
| C | The check moves to the expansion, where an unparsable preset name already refuses at exit status 2 (`source/harness/family.cpp:378-382`). | The line prints ahead of selection, so a family the filter drops refuses a run it never starts; the checkpoint that reads the machine stands unused, and the layout path FR-021 names reaches no line. |

**Recommended:** Option A. FR-019 names the preset the run carries, and the
fit checkpoint is where the tree holds that value resolved for the instances
about to run. The step already walks those instances ahead of every
measurement and beside the refusal lines it prints. An option-only reading
leaves the handle route naming a second partition that no clause settles.

## Q2: The ordinal a preset group name carries for the node unit

**Spec section (quoted, D-4, FR-008, SC-002):**

> - **D-4**: The group's name. A preset-named group takes
> `<unit>-<ordinal>`: `l3-0`, `l3-1` for the L3 unit, `node-0`, `node-1` for
> the memory node. A layout-named group takes the `name` text of its entry,
> or `g<index>` in document order where the entry names none.

> - **FR-008**: A group named by a preset SHALL take the name
> `<unit>-<ordinal>` for the unit it groups: `l3-0`, `node-1` (D-4).

> - **SC-002**: A run of four contexts over a two-node layout under
> `--placement=node` discloses two groups of two, named `node-0` and
> `node-1`, holding the alternating contexts (FR-006, FR-008).

**Question:** D-4 spells each preset group as `<unit>-<ordinal>`, and the
two units reach the plan from different sources. The L3 ordinal stands
assigned in first-seen order inside the process mask
(`source/harness/topology.cpp:112-125`), so it runs from zero with no gap.
The node value the plan carries is the number the read states
(`source/harness/topology.cpp:225`), dealt out at
`source/harness/placement_preset.cpp:171,183`. A `node` run whose mask
leaves node 0 out plans its contexts on unit 1: does its single group answer
`node-1`, the plan's own value, or `node-0`, a count over the units the run
carries (D-4, FR-008, FR-006)?

**Why it matters:** the name is a console byte and a document byte, so the
two readings leave different output. The same console line already ends its
placement field with the plan's node number
(`source/harness/placement_preset.cpp:607-618`), so a fresh numbering prints
two numbers for one unit on one line.

| Option | Reading | Implications |
|---|---|---|
| A | The name carries the unit value the plan names at that position: the L3 ordinal for the L3 unit, the read's node number for the node unit. | SC-001 and SC-002 hold unchanged on the shapes the suite builds today (`test/source/harness_cli_placement_test.cpp:292-299`); a masked node run prints `node-1` beside a placement field ending `@1`; the listing lines of `--topology` and the group name read as one numbering (`source/harness/topology.cpp:596`). |
| B | The ordinal counts the units the run carries in first-use order, so the first unit of every run stands at zero. | Every grouped run names `node-0` upward with no gap, and SC-002 needs no change; the same line then reads `@1` in one segment and `node-0` in the next for one unit, and a reader maps a group to a listing line by hand. |
| C | The name states the unit word with no ordinal, and the memberships carry the rest. | Two groups of one run then share one name, which FR-015's uniqueness rule and FR-018's repeated-name fault refuse; SC-001's four names collapse into one word. |

**Recommended:** Option A. The plan's unit value names the unit the context
paced under, and the standing disclosure already spells that number for a
bound context on the same line. A fresh numbering would state one unit twice
with two values. The L3 half needs no reading: the mask gives those ordinals
from zero, so both readings agree there.

## Considered and dropped

- `--group-layout` beside `--jitter-probe`, the noise tracers, `--topology`
  or `--list`: SC-004 closes the refusal set at the 18 cases FR-018 and
  FR-019 name, FR-034 keeps those paths' standing shapes, and the probe
  reads plans through its own idle seam (`source/harness/cli.cpp:862-883`).

- A one-context instance beside a one-entry layout, and whether such a run
  carries a group at all: FR-010 fixes the run's shape, and an unthreaded
  result prints no context block (`source/harness/report.cpp:640`), so both
  readings write equal bytes.

- Whether a user thread runner may dispatch one group at a time: the Out of
  scope row keeps the `ThreadRun` view as specs/020 fixed it, and FR-027
  seats the standing ledger fault ahead of every round
  (`source/harness/runner.cpp:1955-1961`).

- A group of one context and the round it owns: the Edge Cases row states
  the shape, and `ContextBarrier` takes any participant count above zero
  (`source/harness/execution_context.cpp:821-826`).

- The L3 ordinal of a name where a mask excludes a group: the read assigns
  those ordinals in first-seen order inside the mask
  (`source/harness/topology.cpp:112-125`), so an `l3` name runs from zero
  under either reading.

- The layout path inside the written document: D-10 states where it stands,
  and the standing `run` object rule writes a use of a non-command-line
  source alone (`source/harness/json_report.cpp:905-910`).

- One layout value from an option file and another from the command line:
  FR-011 bars the environment source alone, and the standing scalar settle
  seam keeps one word per row (`source/harness/options.cpp:720-727`).

- Which line answers when a bad layout stands beside `--topology` or a
  filter that matches nothing: the standing order runs the scan, the listing
  and the fit checkpoint in sequence (`source/harness/cli.cpp:707-719`,
  `source/harness/cli.cpp:744-747`).

- Whether the schema makes `group` a required key of the context record:
  FR-029 states it stands present for a partitioned run alone, FR-031 states
  how a document without it loads, and FR-030 moves the guard tests with it.

- Whether a two-digit minor reads at all: the reader's version check reads
  the major alone (`source/harness/json_load.cpp:2432-2437`), so the value
  FR-032 states reads today.

- A `slots` member spelled as a range, a quoted digit or a boolean: FR-013
  names a context position, and FR-018 refuses a member of the wrong kind.

- Whether a group round reaches a measured window, or a second round kind
  joins: FR-023 and FR-024 hold the two standing points, and the Out of
  scope row bars a second barrier kind.

- The group's share of the merged row and its columns: FR-035 and the Out of
  scope row keep the flat shape the standing merge prints
  (`source/harness/runner.cpp:2215`).

- Groups across instances, across families or across runs in one report: the
  Out of scope rows keep one partition per instance, and FR-001 fixes that
  partition for the instance.

## Reconciliation

The orchestrator weighed both recommendations against the tree and the
lane rulings. Both stand as recommended.

- Q1: Option A, adopted. The pairing refusal reads each selected
  instance's resolved preset at the standing fit checkpoint, so a name
  a handle stated carries the same refusal as the name the option
  settled (FR-019, FR-020). The fit walk already reads the resolved
  instances before any run, so the rule costs one branch and keeps the
  refusal ahead of any measured work.
- Q2: Option A, adopted. A `node` group name carries the read's own
  node number, the number the placement field's `@node` tail already
  prints (`placement_preset.cpp:607-618`), so one unit states one
  number on the line (D-4). The L3 half needs no ruling: the read
  numbers L3 units from zero inside the mask.

The Q1 recommendation's aside, that the allowed pairing then reads as
every selected instance resolving to `none`, does not stand. Round 3
takes the reading, and the allowed case keys on the partition rather
than on the name.
