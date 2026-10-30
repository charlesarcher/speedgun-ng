# Spec 039 clarification questions, round 1

Feature: `039-context-groups`, branch `039-context-groups`. Method: the
speckit-clarify discipline, applied once, no spec edit. Each question
below names a spot where two readings of the spec diverge on scope, on
user-visible bytes, on the public surface, or on the shape of a test, and
where no Decision, Requirement, Assumption or grounded tree fact already
stands a default. Every question cites the `path:line` that makes it
real.

Lane rulings R-1 to R-5 bound every option: the group's own barrier, two
sources of one partition, the vendored reader, the report beside the
context disclosure, and the synthetic seam for every test layout.

## Q1: A `cpus` layout entry has no run that can carry it

**Spec section (quoted, User Story 2, FR-014, FR-019):**

> 3. **Given** a layout file of `cpus` entries over a run whose plan
>    pins those cpus, **When** the run starts, **Then** each context
>    joins the entry naming the cpu its plan named.

> - **FR-014**: A `cpus` member SHALL name a machine cpu index, and a
>   context SHALL join the entry naming the cpu the run's plan placed
>   it on; an entry naming a cpu that context's plan does not name
>   SHALL refuse (D-6).

> - **FR-019**: A layout file given beside a placement preset that names
>   a partition SHALL refuse one line naming both, with exit status 2;
>   a layout beside `--placement=none` SHALL stand allowed (D-2).

**Question:** A `cpus` entry needs the run's plan to name a cpu, and the
tree builds a plan only for an instance carrying a preset other than
`none` (`source/harness/runner.cpp:1627-1631`,
`source/harness/placement_preset.cpp:119-122`), while D-3 makes every
such preset name a partition and FR-019 then refuses the layout beside
it: which run can a `cpus` entry ever match? (FR-014, FR-019)

**Why it matters:** SC-005 asks the cpu form to carry one of its three
cases, and the spec's own combination rule leaves no run that can start
one.

| Option | Reading | Implications |
|---|---|---|
| A | `slots` stands as the grammar's one membership key: the `cpus` key leaves this spec, and every clause that names cpus follows it out. | D-6, FR-014, FR-018's last clause, D-8, User Story 2's third scenario and SC-005's third case move; the file names a grouping every run can carry; X.2 keeps a key no run reads out of the grammar. |
| B | The `cpus` form survives by naming the pin too, so a layout beside `--placement=none` pins the cpus its entries name. | The layout takes a pin, the reading the Out of scope row bars; specs/025's placement behavior and FR-009's byte-shape clause move with it. |
| C | The form stays as written, refuses on every run that can carry a layout, and SC-005's third case records that refusal. | The grammar ships a key no run accepts; User Story 2's third scenario cannot pass as written; the suite proves a fault the story sells as a feature. |

**Recommended:** Option A. The tree's only pin source is a preset plan
(`source/harness/runner.cpp:1635-1641`), R-2 keeps one source per run,
and X.2 bars a grammar key that no legal run can carry.

## Q2: What a grouped run keeps of the fixture pair's ordering

**Spec section (quoted, D-7, FR-022, FR-026):**

> - **D-7**: The fixture publication. The fixture pair stays where
>   specs/016 fixed it: context 0 owns it, its setup half runs before any
>   body's measured work, and its teardown half runs after the stop
>   round. A grouped run publishes context 0's setup outcome to every
>   body through a route that carries no round across groups, and the
>   ungrouped run keeps the standing route where its start round carries
>   the publication (FR-021, FR-026).

> - **FR-022**: Each group SHALL stand behind one start round and one
>   stop round of its own, each carrying exactly that group's contexts
>   as participants, and a context SHALL arrive at its own group's
>   rounds only (R-1, D-1).

> - **FR-026**: A grouped run SHALL publish context 0's fixture setup
>   outcome to every body before any body's measured work, and the
>   publication route SHALL carry no round across groups (D-7).

**Question:** The standing start round is the publication route: context
0 writes its setup slot before arriving
(`source/harness/runner.cpp:2119-2126`) and every other body reads that
slot after the same round (`source/harness/runner.cpp:2128-2134`), while
the standing stop round is what places context 0's teardown half behind
every body's window (`source/harness/runner.cpp:2149-2157`). Under a
partition, which route carries the setup outcome to a body of another
group, and which stop round does the teardown half wait on? (FR-022,
FR-026)

**Why it matters:** two standing guarantees ride on that one round: a
failed setup leaves every context out of the callable
(`source/harness/runner.cpp:2131-2136`), and a fixture's teardown starts
once no body measures. The teardown half stands open in the text under
review and in the branch's own spec commit.

| Option | Reading | Implications |
|---|---|---|
| A | Context 0 runs its setup half in its own item, dispatched ahead of the measured bodies. The harness thread waits on that node through the standing poll (`source/harness/runner.cpp:1970-1979`), and the same route carries the teardown half. | The dispatch edge reaches every group, so no round crosses a group (R-1); both standing guarantees hold; the attempt gains two dispatch steps outside every measured window. |
| B | The halves stay inside the body: the setup half ahead of context 0's own start round, the teardown half after context 0's own stop round. | The smallest diff; a body outside context 0's group reads the setup slot with no synchronizing round behind it; a body of another group can stand inside its window while the teardown runs. |
| C | The fixture pair moves to the harness thread, which runs both halves around the attempt. | D-7 and specs/016 fix context 0 as the owner; a fixture holding thread-local state sees a foreign thread; specs/016's contract moves with it. |

**Recommended:** Option A. It keeps D-7's owner and R-1's rule that no
round spans groups, keeps both standing guarantees, and reuses the wait
the harness thread already performs at
`source/harness/runner.cpp:1970-1979`. The spec commit `Meta: Specify the context groups`
carried that route for the setup half, and that clause stands absent
now.

## Q3: A preset group on a layout that states no L3 unit

**Spec section (quoted, FR-006, FR-008, D-3):**

> - **FR-006**: A preset that names a partition SHALL group the
>   contexts by the topology unit its own plan names at each position,
>   read from the plan in place: the L3 group under `core`, `l3`, `smt`
>   and `compact`, the memory node under `node` (D-3).

> - **FR-008**: A group named by a preset SHALL take the name
>   `<unit>-<ordinal>` for the unit it groups: `l3-0`, `node-1` (D-4).

>   - `core`, `l3`, `smt` and `compact`: the L3 group the plan names at
>     that position (`plan.group`), so contexts one preset expects to
>     pace together meet at one round.

**Question:** `resolvePreset` copies each thread's group ordinal into
the plan (`source/harness/placement_preset.cpp:113-117`), and that
ordinal stands at `kNoGroup` when the read states no L3 cache
(`source/harness/detail/internal.hpp:51-64`), a shape the `core` and
`smt` arms still fill with cpus: what group and what name does such a
run carry? (FR-006, FR-008)

**Why it matters:** FR-037 asks full branch coverage over the added
code, and the standing seam builds a cores-without-L3 shape today, where
`test/source/harness_topology_test.cpp:402-410` asserts
`layout.groups.empty()`, so the rule needs a reading the suite can
prove.

| Option | Reading | Implications |
|---|---|---|
| A | A preset whose plan units all stand at `kNoGroup` names no partition, and the run keeps FR-004's ungrouped shape. | The disclosure already reads that value as no unit and drops its `@group` segment (`source/harness/placement_preset.cpp:599`); a `core` or `smt` run that measures today keeps its bytes; one scenario covers the branch. |
| B | One group holds every context and takes the name `l3-4294967295`, the `kNoGroup` value read as an ordinal. | D-4's character rule admits that text; the console and the document name a unit ordinal no layout states; a reader cross-reads a group that stands nowhere. |
| C | The fit step refuses one line when a partition-naming preset meets a layout that states no unit for it. | One more exit status 2 path; a run that measures today leaves with a refusal, a specs/025 behavior move that FR-009 keeps out of this lane. |

**Recommended:** Option A. The tree already reads `kNoGroup` as no unit
and prints nothing for it
(`source/harness/placement_preset.cpp:599`), and R-1 names the ungrouped
shape as what a run with no partition keeps.

## Q4: `--group-layout` beside `--hunt`

**Spec section (quoted, D-11, Out of scope):**

> - **D-11**: Groups bind the instance's threaded run. The jitter
>   probe's own barrier (`source/harness/jitter_probe.cpp:424,430`),
>   the hunt path, the idle reads and the noise tracers keep their
>   standing rendezvous shapes.

> - Groups inside the jitter probe, the hunt path, the noise tracers or
>   the placement of memory.

**Question:** The standing entry point refuses a hunt beside a placement
preset, one line naming both options at exit status 2, for the stated
reason that one run carries one plan
(`source/harness/hunt_cli.cpp:720-748`), and a hunt drives a threaded
instance (`source/harness/hunt.cpp:702-717`): does `--group-layout` join
that refusal list, or does a hunt run a grouped instance? (FR-003,
FR-019)

**Why it matters:** the answer is one refusal line, one hunt test case,
and whether the group rounds ever appear inside a hunt's passes.

| Option | Reading | Implications |
|---|---|---|
| A | The row joins the standing combination refusal: one line naming `--hunt` and `--group-layout`, exit status 2. | The standing reason covers the second source of one partition; the check sits beside the `--placement` arm of `validateHuntCombination`; one case joins the hunt CLI tests. |
| B | The hunt accepts the row, because D-11 binds groups to the instance's threaded run and the hunt drives that run through the standing runner. | The hunt's passes then run grouped bodies and grouped disclosures; the Out of scope row that keeps groups out of the hunt path needs a narrower reading. |
| C | The hunt accepts the row, reads the layout and runs ungrouped. | The command line names a pacing the run does not carry, with no fault line; D-10's one-spelling rule loses here. |

**Recommended:** Option A. `--hunt` already keeps a placement plan away
for the reason the code states at `source/harness/hunt_cli.cpp:741-744`,
and FR-003 names the layout the second source of that same partition.

## Q5: The `topology:` line for a run the layout groups

**Spec section (quoted, FR-028, FR-033):**

> - **FR-028**: A grouped run's console line for one context SHALL name
>   the group as its last segment, after the placement flag where that
>   flag prints, and SHALL keep the standing segments in the standing
>   order (`report.cpp:168-191`).

> - **FR-033**: An ungrouped run's console output SHALL stand
>   byte-identical to master's for the same fixture, apart from the
>   schema stamp, and its document SHALL carry no `group` key (D-12).

**Question:** The entry point prints the `topology:` line only when the
fit step reports a preset in effect (`source/harness/cli.cpp:744-755`,
`source/harness/report.cpp:317-333`), and a layout beside
`--placement=none` leaves that flag false because every instance carries
`Preset::NONE` (`source/harness/placement_preset.cpp:636-637`): does a
grouped layout run print the machine line? (FR-028, FR-033)

**Why it matters:** that line is the console's only statement of the
machine counts, and a new line for layout runs moves the folded samples
the `report_shape` gate compares in `docs/pages/harness.md`.

| Option | Reading | Implications |
|---|---|---|
| A | The standing gate holds: a layout run prints no `topology:` line, and the group names carry the pacing fact. | FR-033's byte rule stands untouched; the context line stays the only console move; the line's reason was a preset that planned cpus, and a slots layout pins nothing. |
| B | The gate widens: any run that carries a partition prints the standing `topology:` line. | The reader sees the machine the groups came from; every layout run gains one console line, with the docs samples and the folded shapes with it. |
| C | A grouped run prints one new `groups=` count beside the standing `contexts=` field. | A second console shape the disclosure section names nowhere; D-10 keeps the group set in the per-context records alone; the gate grows again. |

**Recommended:** Option A. The flag's standing reason is a preset that
planned cpus on the machine, and a slots layout names positions the
machine does not decide, so the line would state a plan the run does not
carry.

## Considered and dropped

- Where the group segment sits in the console line: FR-028 fixes it as
  the last segment and keeps the standing segments in their standing
  order.
- A group-level record or count in the report: D-10 carries the groups
  through the per-context record alone.
- The order of the group set in the console and the document: the
  console prints contexts by index and the document keeps the standing
  `contexts` array order, so D-10's one spelling settles it.
- What an ungrouped context record answers for the new field: FR-031
  states that a document with no `group` key loads as the record naming
  no group.
- A `cpus` layout beside `--placement=none`: User Story 6's second
  scenario and FR-018's last clause already name that refusal.
- A one-context instance beside a one-entry layout: FR-010 and the Edge
  Cases row fix the run's shape.
- A default name that collides with a stated name: FR-015's uniqueness
  rule and FR-018's repeated-name fault cover it.
- Rounds across passes and repetitions: the Edge Cases row fixes the
  partition for the instance and the rounds per attempt, and FR-001
  repeats the rule.
- The layout path from the environment: FR-011 states the row carries no
  environment source, which empties that half of D-10; an option file
  settles through the standing row seam.
- The `l3` and `compact` presets over a layout stating no L3 group: the
  standing fit step already stops that batch
  (`source/harness/placement_preset.cpp:660-669`).
- The `node` preset over a read stating no memory information: the
  standing refusal at
  `source/harness/placement_preset.cpp:650-655` answers it.
- Which refusal line wins when two faults stand at once: FR-018, FR-019
  and FR-020 each promise one line at exit status 2, FR-021 names what
  every line carries, and the order reads on the standing scan-then-fit
  sequence (`source/harness/cli.cpp:506-561`,
  `source/harness/cli.cpp:744-747`).
- The size of the refusal test set: SC-004 points at the list FR-018
  names, and FR-037 asks one scenario per FR of sections A to G.
- Whether a test may read `/sys`: R-5 and FR-036 settle it.
- The version, `SOVERSION` and schema minor values: FR-032 states the
  re-read rule at landing.
- A group dimension in the row shape, a public dispatch-by-group API, a
  second barrier kind: FR-035 and the Out of scope rows settle them.
- The evidence host and its core count: the Lane identity section
  records both hosts and the seam that carries the shapes.
- Coverage and gate targets: FR-037, FR-039, Principle VI and Principle
  VIII settle them.

## Reconciliation

The orchestrator weighed each recommendation against the alternatives
and the lane rulings. Four stand as recommended, one lands differently.

- Q1: Option A, adopted. The cpu form loses its carrier the moment R-2
  bars the layout beside a preset, so the grammar drops the key and
  `cpus` refuses one line naming its reason (D-6, FR-014). The lane
  prompt's phrase naming cpus beside slots stands recorded as a
  deviation in Assumptions, and the later spec that lets a layout pin
  its contexts revives the form.
- Q2: Option A adopted in its goal, replaced in its mechanism. A
  pre-dispatched setup node ends the `Fixture` object's life, because
  the pair's object stands a local of the body's own invocation
  (`runner.cpp:2118`, `:2123`), and holding it across two dispatches
  moves shared fixture code this item does not own. The spec states the
  rule and leaves the mechanism to plan.md: context 0 keeps the pair,
  the setup outcome publishes through a route that crosses no round,
  and the teardown runs on context 0 after every body has settled
  (D-7, FR-026).
- Q3: Option A, adopted. `kNoGroup` names no unit, so the preset names
  no partition and the run keeps the ungrouped shape (FR-006).
- Q4: Option A, adopted. The layout joins the standing `--hunt`
  combination refusal (FR-019).
- Q5: Option A, adopted. The machine line keeps its preset-in-effect
  rule, and a layout run prints none (FR-028).
