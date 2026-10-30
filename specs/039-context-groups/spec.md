# Feature Specification: Context Groups for the Threaded Run

**Feature Branch**: `039-context-groups`

**Created**: 2026-10-30

**Status**: Draft

**Input**: User description, opening quoted:

> Give the contexts their groups, item N9 of the roadmap (section 3,
> origin ruling H11): the contexts of one instance run in groups, each
> group behind one barrier of its own, the groups named by the
> placement presets or by a YAML layout file. The tests build the
> machine layouts they need on the standing synthetic seam.

The request fixes the rulings R-1 to R-5 of the lane prompt; they
stand in Decisions as D-1 to D-5. The further decisions cover the
shapes the prompt leaves open: the unit each preset groups by, the
group's name, the two rendezvous points a group owns, the fixture
publication, the layout grammar with its refusal list, and the batch
fit step that stops a layout the run cannot carry.

## Spec identity and run-time binding

- Spec number: `039`. `specs/` on master `bea6af5b` holds `001`
  through `038` with `031` and `037` absent (read 2026-10-30). `031`
  stands reserved for N0b host validation on the kennykiller lane,
  `037` for N11 execution graphs on the chilly lane. The open
  pull-request list answers no row, and `git branch -a` names no ref
  carrying `039` or `context-group` (read 2026-10-30). The directory
  `specs/039-context-groups` stands free and the branch takes it.
- Branch: `039-context-groups`, cut from master `bea6af5b`
  (`Meta: Mark the landed tasks`, PR #59, spec 038) on 2026-10-30.
- Version: master stands at `0.25.0` (`CMakeLists.txt:30`). The lane
  takes the next free minor, `0.26.0`. Spec 037 takes a minor at its
  own merge, so the landing that moves second re-reads the line and
  takes the next free value, and the commit records the check
  (FR-002, D-2).
- Harness `SOVERSION`: master stands at `16` (`CMakeLists.txt:333`).
  The group record joins the public header, the exported layouts
  move, and the harness `SOVERSION` rises to `17`. The archive
  `SOVERSION` stays `4` (`CMakeLists.txt:95`), and the commit that
  moves the numbers records the ABI check beside them (FR-005).
- Report schema: master stands at `1.8`
  (`source/harness/json_report.cpp:39`, `kSchemaVersion`). The groups
  land beside the context disclosure, so the document gains a key and
  the minor moves. Spec 037's graph record moves the same minor at
  its own merge, and that lane cut first, so this lane takes the next
  value past its `1.9`: the schema minor lands at `1.10` (R-4,
  FR-025). A landing that finds `1.9` or `1.10` taken moves one step
  higher and records the reading.
- Option values: the free block `298` to `319` stands open
  (`source/harness/detail/options.hpp:78-80`, `kOsNoiseOptionValue`
  holds `297` at `:93`). Spec 037's `--graph` row takes `298` at its
  own merge, so this lane's one new row takes `299` and the block
  comment names the split (FR-010). The hunt block `280` to `289`
  stands specs/033's.

## Roadmap position

- Item N9 of the harness roadmap, section 3, origin ruling H11. The
  specs/020 spec lists the item among its deferrals, naming
  `context groups, presets, and YAML (H11)` at its line 890. The
  placement lanes it reads stand merged: specs/025
  (the presets and the `cpu@group` field) and specs/035 (the memory
  nodes and `cpu@group@node`). specs/020 owns the context surface, the
  `ContextBarrier` and the two rendezvous points this feature splits.

## Lane identity

- The item stands hardware-adjacent. The multi-CCD and multi-NPS
  shapes are the acceptance axis for the preset grouping: the
  disclosure names one group per unit the plan touches. The suite
  stays host-agnostic: every layout a test exercises stands built on
  the synthetic seam, and the suite passes under `taskset -c 0,1` and
  on a two-cpu host (FR-030).
- The lane prompt names thrawn as the evidence host (24 cores, 48
  threads, several CCDs with one L3 each, an NPS BIOS setting). The
  host this branch was cut on is `gary` (Ryzen 9 9950X3D, 16 cores,
  32 threads, two L3 groups, one socket). The recorded runs name the
  host that ran them, and the synthetic seam carries the multi-group
  shapes on either host (R-5, FR-032).

## Audit point

- Repository: `master` at `bea6af5b` (project version `0.25.0`,
  report schema `1.8`, harness `SOVERSION 16`, archive `SOVERSION 4`,
  147 tests green on the release preset), read 2026-10-30 on the
  branch host.
- The context surface stands public at
  `include/speedgun-ng/execution_context.hpp`: `ContextId` (`:52`),
  `WaitMode` (`:91`), `PlacementSlot` (`:114`), `ContextPolicy`
  (`:151`), `PlacementResult` (`:168`), `ContextControl` (`:218`),
  `ExecutionContext` (`:268`), `dispatch` (`:629`), `ContextBarrier`
  (`:656`, one arrival count and one generation word, defined
  `source/harness/execution_context.cpp:821-900`).
- `include/speedgun-ng/barrier.hpp` holds the optimizer barrier
  family (`:58`, `:173`) and governs this feature's rule that no
  round sits inside a measured window; the thread rendezvous is
  `sg::ContextBarrier`.
- The threaded run stands at `source/harness/runner.cpp`: the
  contexts build once per instance run (`:1626-1644`) with the plan's
  cpu at each position (`:1640`), the attempt builds
  `startBarrier` and `stopBarrier` over `instance.threadCount`
  participants (`:2066-2067`), the bodies arrive at `:2126` and
  `:2149`, and `launchBodies` dispatches index `i` to context `i`
  (`:1916-1980`).
- `PlacementPlan` (`source/harness/detail/internal.hpp:143-152`)
  carries `cpu`, `group` and `node` per position. `resolvePreset`
  fills all three (`source/harness/placement_preset.cpp:105-212`) and
  the runner reads `cpu` alone (`runner.cpp:1640`), so `group` and
  `node` stand computed and unused today.
- The six preset names stand at `placement_preset.cpp:45-50`, parsed
  by `parsePreset` (`:67-75`); the fit step that stops a batch stands
  at `placementFitStatus` (`:631-693`, called `cli.cpp:744-747`).
- The disclosure stands at `source/harness/report.cpp:168-191`
  (`contexts=%zu`, then `ctx%zu=<mode>/<placement>[/<applied>]`), the
  record at `include/speedgun-ng/benchmark.hpp:454-462`
  (`ContextDisclosure`), the JSON at
  `source/harness/json_report.cpp:580-593` (the `contexts` array with
  `waitMode`, `placement`, `applied`), parsed at
  `source/harness/json_load.cpp:1869-1897`.
- The synthetic seam stands at
  `source/harness/detail/internal.hpp:820-823`
  (`installTestTopologyDescription`, consumed at
  `source/harness/topology.cpp:141-143`), beside
  `installTestLayout` (`internal.hpp:790`, `topology.cpp:336-339`).
- The vendored reader stands at specs/006: `yaml-cpp` `0.9.0`,
  included today by `source/yaml/yaml_gate.cpp:10` alone, gated by
  `tools/yaml/yaml_purity_scan.sh` (A7, A8) and
  `tools/yaml/yaml_nm_proof.sh`. specs/006 FR-014 bars yaml-cpp
  headers from `include/` and names `source/` as the includer scope;
  specs/006 `spec.md:164` names this kind of consumer as the work a
  later spec lists the vendoring as prerequisite for.
- The option table stands at `source/harness/options.cpp:470-714`
  (38 rows), the row record and value constants at
  `source/harness/detail/options.hpp:81-130`, the refusal path at
  `cli.cpp:506-510` and `:556-561` (one line on standard error, exit
  status 2). The combination refusal `--hunt cannot combine with ...`
  stands at `source/harness/hunt_cli.cpp:155`.
- The results-evidence precedent stands at
  `specs/036-osnoise-timerlat-tracers/results/` and
  `specs/038-report-view-generators/results/`.

## Standing facts this lane reads

- A threaded attempt carries two rendezvous points: one before the
  bodies' measured work and one after it. Both take every context of
  the instance, and the standing comment states that no arrival and no
  wait sits inside a measured window (`runner.cpp:2062-2065`).
- `ContextBarrier` takes its participant count at construction, is
  non-copyable and non-movable, and its destructor requires that no
  arrival stands outstanding
  (`include/speedgun-ng/execution_context.hpp:656-681`,
  `source/harness/execution_context.cpp:821-838`). It reads no clock
  and carries no timeout (`execution_context.cpp:845`).
- A body whose counter plan cannot compile asserts, because a
  skipped body would strand its round and a stranded round is a hung
  run (`runner.cpp:2078-2089`).
- Fixture setup runs on context 0 before the start round, and the
  start round is what publishes context 0's setup outcome to the
  other bodies (`runner.cpp:2119-2134`).
- A family expands to one instance per thread count
  (`source/harness/family.cpp:404-433`), so one batch runs instances
  of several context counts under one option set.
- The fit step stops a batch before any instance runs when a preset
  cannot place the contexts it names (`placement_preset.cpp:656-669`,
  specs/025 FR-018).
- The plan's `group` value is the L3 group ordinal of the cpu at that
  position for every preset arm, and the `node` value is the memory
  node the context binds to under the `node` preset
  (`placement_preset.cpp:113-117,127-131,139-189`).
- The disclosure field a pinned context prints already carries an
  `@group` segment holding that L3 ordinal (`placementText`,
  `placement_preset.cpp:587-622`). This spec's groups are a separate
  fact, and their name carries a unit prefix so the two never read as
  one token (D-4).
- Console disclosure lines are folded by the `report_shape` gate and
  quoted in `docs/pages/harness.md`, so a shape change touches the
  samples and the drift gate (FR-028, FR-033).

## Clarifications

### Session 2026-10-30

- Q: A `cpus` layout entry has no run that can carry it: a plan cpu
  stands only under a preset plan, and R-2 bars a layout beside a
  preset. → A: The release names slots, and the key `cpus` refuses one
  line naming its reason (D-6, FR-014). Grammar no legal run can read
  is the shape X.2 bars, and a later spec that lets a layout pin its
  contexts brings the form back.
- Q: What does a run with more than one start round keep of the
  fixture pair's ordering, where the standing round carried both the
  setup publication and the teardown's place behind every window? → A:
  Context 0 keeps the pair, the setup outcome publishes through a route
  that crosses no round, and the teardown runs on context 0 after every
  body of the attempt has settled (D-7, FR-026). The pair keeps its
  owner and both orderings.
- Q: What partition does a preset name where the machine read states
  no L3 unit? → A: None: the run keeps the ungrouped shape, the way
  the standing disclosure prints no unit for `kNoGroup` (FR-004,
  FR-006).
- Q: `--group-layout` beside `--hunt`? → A: The standing combination
  refusal grows to carry the layout, one line and exit status 2
  (FR-019).
- Q: Does a run the layout groups print the standing machine line? →
  A: It prints none: the line keeps its preset-in-effect rule, and a
  slots layout pins nothing (FR-028).

### Session 2026-10-30, round 2

- Q: Which preset does the pairing refusal read when a family states
  one on the handle and the option states `none`? → A: The instance's
  resolved preset, read at the standing fit checkpoint before any
  instance runs (FR-019, FR-020). The fit walk already reads the
  resolved instances, so the handle name and the option name carry one
  rule.
- Q: Which number does a `node` group name carry when the mask excludes
  the read's first node? → A: The plan's own node number, the value the
  placement field's `@node` tail already prints (D-4). One unit carries
  one number on the line.

### Session 2026-10-30, round 3

- Q: Does the pairing refusal key on the preset's name or on the
  partition that preset names for the read in hand? → A: On the
  partition. A preset above `none` that names no partition stands
  allowed beside the file, the file stays the run's one partition
  source, and the machine line keeps its single standing rule
  (FR-019, FR-028, D-2). The refusal set FR-018 and FR-019 name stays 18
  cases.
- Q: Does that reading bring back the layout's cpu form? → A: It does
  not. A partition-naming preset never stands beside the file, the file
  names slots only, and a preset that pins while naming no partition
  pins from its own plan (D-6, FR-014).

## Decisions

- **D-1 (R-1)**: The barrier is the group's own. Each group stands
  behind one start round and one stop round of its own, and a context
  arrives at its own group's rounds only. No round carries contexts of
  two groups. A run carrying no partition keeps the standing pair of
  rounds over all its contexts, the shape master runs.
- **D-2 (R-2)**: Two sources, one shape, one partition. The partition
  is one value the run carries, named by the placement preset or by
  the layout file. A layout file beside a preset that names groups
  refuses one line at exit status 2, the standing refusal shape
  (`hunt_cli.cpp:155` is the combination-refusal precedent). The
  `none` preset names no partition, so it keeps the standing shape. A
  preset above `none` that names no partition for the read in hand
  names no groups either, so it stands allowed, and the file stays that
  run's one partition source (FR-019).
- **D-3**: The unit each preset groups by. A preset's groups are the
  units its own plan already names for its positions, read from the
  plan this lane finds in place:
  - `core`, `l3`, `smt` and `compact`: the L3 group the plan names at
    that position (`plan.group`), so contexts one preset expects to
    pace together meet at one round.
  - `node`: the memory node the plan names at that position
    (`plan.node`), the unit that preset distributes across.
  - `none`: no partition.
  The plan's cpu assignment stays untouched: grouping reads the plan,
  it changes none (specs/025 FR-008 to FR-014, specs/035 FR-009 stay
  in force byte for byte).
- **D-4**: The group's name. A preset-named group takes `<unit>-<ordinal>`:
  `l3-0`, `l3-1` for the L3 unit, `node-0`, `node-1` for the memory
  node. The number is the plan's own unit value, the read's L3 ordinal
  and the read's node number (`topology.cpp:112-125`,
  `topology.cpp:225`), so a mask that excludes the first node names its
  group `node-1`, the number the placement field's `@node` tail already
  prints (`placement_preset.cpp:607-618`). A layout-named group takes the `name` text of its entry, or
  `g<index>` in document order where the entry names none. One name
  is unique within the run. A name stands 1 to 32 characters of
  letters, digits, `-`, `_` and `.`, so it cannot collide with the
  `@group` and `@node` segments of the placement field and cannot
  break the `/`-separated console line.
- **D-5 (R-3)**: The YAML reads through the vendored reader.
  `yaml-cpp` parses the layout; the grammar this spec names is the
  whole vocabulary; no new dependency joins; the include stays inside
  `source/` and the specs/006 purity and nm gates stay green
  (FR-014).
- **D-6**: The layout file names groups by slots, one group per
  entry. A slot is a context position, `0` based, and the entries of
  one file cover the instance's contexts exactly once. The cpu form
  the lane prompt sketches stands outside this release: a plan cpu
  exists only under a preset plan (`runner.cpp:1627-1631`), and R-2
  leaves no partition-naming preset beside the file to carry one, so
  the file has no cpu source of its own in this release. A preset that
  stands beside the file while naming no partition pins from its own
  plan, and the file still names slots only. The key `cpus` refuses one
  line naming its reason, and a later spec that lets a layout pin its
  contexts brings the form back (FR-014, Out of scope).
- **D-7**: The fixture publication. The fixture pair stays where
  specs/016 fixed it: context 0 owns it, its setup half runs before any
  body's measured work, and its teardown half runs after the stop
  round. A grouped run publishes context 0's setup outcome to every
  body through a route that carries no round across groups, and the
  ungrouped run keeps the standing route where its start round carries
  the publication (FR-021, FR-026).
- **D-8**: The batch fit. A layout names the slots of one instance.
  The standing fit step reads the layout before any instance runs and
  stops the batch when the layout's slot count differs from the
  context count of any instance the batch runs, or when a cpu entry
  names no cpu of that instance's plan. One line, exit status 2
  (specs/025 FR-018 is the precedent for the stop).
- **D-9**: One group is legal. A layout with one entry, and a preset
  whose contexts all land in one unit, give one group holding every
  context: the run's rounds then carry the same participant set the
  standing shape carries, disclosed by name.
- **D-10**: The report carries the groups through the per-context
  record alone. Each context's record names its group, and the group
  set reads off those names, so the document holds one spelling of
  the fact (X.2). The `run` object's standing `options` disclosure
  carries the layout path when the option came from a file or the
  environment.
- **D-11**: Groups bind the instance's threaded run. The jitter
  probe's own barrier (`source/harness/jitter_probe.cpp:424,430`),
  the hunt path, the idle reads and the noise tracers keep their
  standing rendezvous shapes.
- **D-12**: The lane's tests take the ungrouped shape as the
  regression axis. A scenario runs the same fixture with and without
  a partition, and the ungrouped side's console bytes stand
  byte-identical to master's apart from the schema stamp (FR-026,
  SC-006).

## Out of scope

- A switch that groups one partition and suppresses another. The
  preset and the file are the two sources this spec names, and the
  `none` preset is the standing suppression.
- Groups across instances, across families, or across runs in one
  report. One instance owns one partition.
- A group-carried row shape: the merged row, its divisor and its
  percentile columns keep their standing flat per-context shape
  (`runner.cpp:2215`, `:2731-2741`).
- A layout grammar that nests, inherits, spans several files, or
  states a cadence, a rate, or a pin. Pins stay the presets' work
  (specs/025, specs/035).
- Groups inside the jitter probe, the noise tracers or the placement
  of memory.
- A layout entry naming cpus, and a layout that pins its own
  contexts: the cpu stands only where specs/025 and specs/035 place
  it, so a layout run has none to name (D-6).
- A public handle API that names a group or dispatches by group. The
  `ThreadRun` view keeps `contextCount`, `context` and `dispatch` as
  specs/020 fixed them.
- A second barrier kind. `sg::ContextBarrier` is the mechanism.
- A YAML reader for any other file of the harness. Option files and
  comparison-rule files keep their line grammar.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - The preset groups the contexts behind their own round (Priority: P1)

A user runs a benchmark with eight contexts over a machine of four
L3 groups under `--placement=l3`. The run partitions the contexts by
the L3 group each one's plan names, so four groups of two stand
behind rounds of their own. Each group starts and paces as one unit,
and no round waits for a context of another group.

**Why this priority**: the group round is the feature. Without it the
presets name placement and nothing else, and the roadmap item stands
unshipped.

**Independent Test**: with a four-group synthetic layout and eight
contexts, the run completes and the disclosure names four groups of
two members; the same fixture under `--placement=none` completes with
no group named.

**Acceptance Scenarios**:

1. **Given** a layout of four L3 groups and eight contexts under
   `--placement=l3`, **When** the instance runs, **Then** the
   disclosure names four groups, each holding two contexts, and every
   context names the group of its own plan unit.
2. **Given** a two-node layout of four contexts under
   `--placement=node`, **When** the instance runs, **Then** the
   contexts alternate into two groups named by their node.
3. **Given** a grouped run of two groups, **When** the attempt
   completes, **Then** each group's members stand done, which proves
   each group's round released with its own participant count.
4. **Given** a one-context instance under any preset, **When** the
   instance runs, **Then** the run carries no group and no threaded
   round.

### User Story 2 - The layout file names the groups explicitly (Priority: P1)

A user writes a layout file with two entries, one naming slots `0`
and `1`, one naming slots `2` and `3`, and runs with
`--group-layout=<path>`. The run builds exactly the two groups the
file names, and the disclosure names each context's group.

**Why this priority**: the file is the second source, and the shape a
user hand-writes is the one the presets cannot name.

**Independent Test**: with a written layout over a fixture layout,
the disclosure names exactly the file's groups and names; the file's
absent `name` entries take the document-order default.

**Acceptance Scenarios**:

1. **Given** a layout file of two `slots` entries and four contexts,
   **When** the run starts, **Then** the disclosure names the two
   groups with the file's names and the file's memberships.
2. **Given** a layout file whose entries carry no `name`, **When**
   the run starts, **Then** the groups take `g0` and `g1` in document
   order.
3. **Given** a layout file whose one entry names every context,
   **When** the run starts, **Then** the run carries one group naming
   every context, and its rounds carry the standing participant set
   (D-9).

### User Story 3 - A malformed layout refuses one line (Priority: P1)

A user hand-writes a layout and mis-types it. Each malformed shape
refuses one line on standard error with exit status 2 before any
instance runs, naming the file and the entry the fault stands in.

**Why this priority**: the file is user-authored input on the path
that decides how a run paces, and a silent misread would pace a run
wrong.

**Independent Test**: each refusal shape in the list FR-016 names
runs one command, and each answers one line and exit status 2.

**Acceptance Scenarios**:

1. **Given** a file that cannot be read, **When** the run starts,
   **Then** one line names the path and the exit status is 2.
2. **Given** a file the reader cannot parse, **When** the run starts,
   **Then** one line names the path and the exit status is 2.
3. **Given** a document with no `groups` key, an empty sequence, or
   an entry naming no member, **When** the run starts, **Then** one
   line names the fault and the exit status is 2.
4. **Given** an entry with no `slots` key, or a key named `cpus`,
   **When** the run starts, **Then** one line names the entry or the
   key and the exit status is 2.
5. **Given** a slot named twice, a slot above the context count, or a
   context no entry names, **When** the run starts, **Then** one line
   names the fault and the exit status is 2.
6. **Given** two entries sharing one name, or a name outside the
   allowed characters, **When** the run starts, **Then** one line
   names the name and the exit status is 2.
7. **Given** a layout file beside `--placement=l3`, **When** the run
   starts, **Then** one line names both names and the exit status is
   2.

### User Story 4 - The disclosure names the group beside the placement (Priority: P2)

A user reads the console and the report to see how the run paced.
Each context line names its group beside its wait mode and placement,
and the report's per-context record carries the same name, so a
reader recovers the group set from the document.

**Why this priority**: the pacing a run chose is a fact the run
measured under, and an undisclosed group would hide it.

**Independent Test**: with a grouped fixture run, the console line
carries the group segment last and the report's record carries the
`group` key with the same text; `loadReport` answers it.

**Acceptance Scenarios**:

1. **Given** a grouped run, **When** the console prints, **Then**
   each context line ends with its group name.
2. **Given** the same run's document, **When** the reader loads it,
   **Then** each context record carries `group` with the printed
   name, and the schema of specs/023 accepts the document.

### User Story 5 - The ungrouped run stands today's shape (Priority: P2)

A user runs without a preset and without a layout. The run carries no
partition, keeps the standing pair of rounds over all its contexts,
and its console output stands byte-identical to master's.

**Why this priority**: the standing shape is the default of every
existing user, and the acceptance criterion names it.

**Independent Test**: the same fixture runs with no partition on this
branch and on master, and the console bytes match apart from the
schema stamp.

**Acceptance Scenarios**:

1. **Given** an ungrouped threaded run, **When** the console prints,
   **Then** the context lines carry no group segment.
2. **Given** an ungrouped threaded run, **When** the report is
   written, **Then** no context record carries the `group` key.

### User Story 6 - The batch that cannot carry the layout stops (Priority: P2)

A user runs a family of thread counts `2,4,8` with one layout file
naming four slots. The fit step stops the batch before any instance
runs, one line naming the layout and the counts, exit status 2.

**Why this priority**: a layout is a promise about one instance's
context count, and a batch that breaks it must fail loudly at the
standing checkpoint.

**Independent Test**: the three-count family beside a four-slot
layout answers one line and exit status 2 with no row printed.

**Acceptance Scenarios**:

1. **Given** a batch of context counts `2,4,8` and a four-slot
   layout, **When** the run starts, **Then** one line names the
   counts and the exit status is 2.
2. **Given** a layout file beside `--hunt`, **When** the run starts,
   **Then** one line names both names and the exit status is 2
   (FR-019).

### User Story 7 - The evidence stands recorded and the docs teach groups (Priority: P3)

The lane records its acceptance runs: the preset grouping over a
multi-group shape, the layout run end to end, and the refusal set,
one record per run under the results directory naming the commands,
the exit codes and the artifacts. The main page gains the groups
section with its example region and its quick-reference rows.

**Why this priority**: the records are what a later lane reads, and
the docs gates bind every feature (IV.1).

**Independent Test**: the results directory carries one record per
run with the quickstart linking each; the example builds, runs and
prints; the drift and prose gates read no finding.

**Acceptance Scenarios**:

1. **Given** the acceptance runs execute, **When** the lane closes,
   **Then** each record names the commands, the exit codes and the
   artifacts, and the quickstart links every record.
2. **Given** the main-page section, **When** the drift gates run,
   **Then** the section stands equal to the region it quotes and the
   quick-reference rows name the public surface.

### Edge Cases

- A run whose preset lands every context in one unit carries one
  group naming every context, and its rounds carry the standing
  participant set (D-9).
- A layout entry naming one slot is legal: that context's round has
  one participant and releases on its own arrival.
- A batch whose instances all share one context count carries one
  layout; a batch of differing counts refuses (FR-020).
- A one-context instance of a batch carries no threaded round, so the
  layout names one group of that one context and the disclosure
  prints no context lines for it.
- A `cpus` key refuses at the grammar, one line naming the key and
  its reason, before the fit step reads the file (FR-014).
- A machine read that states no L3 unit leaves the L3-keyed presets
  naming no partition, and those runs keep the ungrouped shape
  (FR-006).
- A family that states `placement("l3")` on the handle beside a layout
  refuses at the fit checkpoint, the case the option states
  `--placement=none` and the expansion lets the handle name win
  (`family.cpp:374-386`).
- A mask that excludes the read's first node names the group by the
  number the plan carries, `node-1` beside a placement field that ends
  `@1` (D-4).
- A preset above `none` that names no partition for the read, the
  `core` plan over a read that states cores and no L3 unit, stands
  allowed beside a layout file: the preset pins, the file groups, and
  the machine line prints as the standing rule prints it (FR-019,
  FR-028).
- A user thread runner that returns with a group's member
  undispatched meets the standing ledger fault before the run waits
  on any round (FR-027).
- A multipass or multi-repetition run builds its group rounds once
  per attempt; the partition stands fixed for the instance (FR-001,
  FR-034).
- A grouped run whose fixture setup fails publishes the failure to
  every body, discards the attempt and prints the standing setup
  fault, and its teardown half stands skipped (FR-026).
- A name the file states with a trailing space or an empty string
  refuses; the reader trims no text.
- A layout file of zero bytes, and one whose root node is a sequence,
  refuse as the grammar's own faults.
- A layout path the harness cannot open refuses with the standing
  open-fault text shape (`readTextFile`, `options.cpp:1150-1174`).
- A grouped run of a benchmark whose rows carry per-context
  percentiles keeps the standing flat row shape; the group names ride
  the disclosure, the columns (FR-035).

## Requirements *(mandatory)*

### The partition and its sources (A)

- **FR-001**: The run SHALL carry one partition of an instance's
  contexts into one or more groups, each group naming the contexts
  that start and pace as one unit, and the partition SHALL stand
  fixed for the instance across its passes and repetitions (R-2).
- **FR-002**: A group SHALL hold at least one context, and every
  context of the instance SHALL stand in exactly one group.
- **FR-003**: The run SHALL name its partition from one source: the
  placement preset or the layout file (R-2).
- **FR-004**: A run whose source names no partition SHALL carry none:
  the contexts SHALL run as one ungrouped set behind the standing
  pair of rounds, the shape master runs (R-1).
- **FR-005**: The group name and the per-context group record SHALL
  join the public header that owns the context disclosure, and the
  project version, the harness `SOVERSION` and the archive
  `SOVERSION` SHALL stand at the values FR-032 records, with the ABI
  check named in the commit that moves them (D-2).

### The preset as a source (B)

- **FR-006**: A preset that names a partition SHALL group the
  contexts by the topology unit its own plan names at each position,
  read from the plan in place: the L3 group under `core`, `l3`, `smt`
  and `compact`, the memory node under `node` (D-3). Where the
  machine read states no such unit, the plan carries `kNoGroup`
  (`source/harness/detail/internal.hpp:51-52`), and the preset SHALL
  name no partition: the run keeps the ungrouped shape of FR-004.
- **FR-007**: The `none` preset SHALL name no partition, and a run
  with no preset stated and no layout stated SHALL name no partition
  (D-2).
- **FR-008**: A group named by a preset SHALL take the name
  `<unit>-<ordinal>` for the unit it groups: `l3-0`, `node-1` (D-4).
- **FR-009**: Grouping SHALL leave the plan's cpu assignment and the
  memory bind unchanged: the presets' placement behavior of specs/025
  and specs/035 SHALL stand byte-shaped as those specs fix it.
- **FR-010**: A partition SHALL apply above one context: a one-context
  instance SHALL carry no group and no threaded round, the standing
  shape.

### The layout file as a source (C)

- **FR-011**: The command line SHALL gain one row, `--group-layout`,
  taking the internal value `299`, reading its argument as a file
  path, standing in the table `options.cpp:470-714` with the standing
  row record, and appearing in `--help` and in the guide's option
  reference the way the table drives both. The row SHALL carry no
  environment source, the shape `--compare-rules` keeps.
- **FR-012**: The file SHALL read as a YAML mapping whose root key
  `groups` holds a sequence of at least one entry, one entry per
  group. Each entry SHALL be a mapping with an optional `name` and one
  `slots` key holding a sequence of at least one member
  (R-3, D-5, D-6).
- **FR-013**: A `slots` member SHALL name a context position, `0`
  based, within the instance's context count, and the entries' slots
  SHALL cover every context of the instance exactly once (D-6).
- **FR-014**: A key named `cpus` SHALL refuse one line on standard
  error with exit status 2, naming the key, the entry and the reason:
  this release names slots, because a layout run carries no plan cpu
  to group by (D-6, Out of scope).
- **FR-015**: A `name` SHALL stand 1 to 32 characters drawn from
  letters, digits, `-`, `_` and `.`, SHALL stand unique across the
  entries, and an entry naming none SHALL take `g` plus its
  zero-based document index (D-4).
- **FR-016**: The reader SHALL be the vendored `yaml-cpp` of specs/006
  through an include under `source/`; no new dependency SHALL join; no
  header under `include/` SHALL name yaml-cpp; the specs/006 purity
  scan and nm proof SHALL stay green (R-3).
- **FR-017**: The grammar SHALL accept no other key, no nesting and
  no second document, and a key outside it SHALL refuse.

### The refusals (D)

- **FR-018**: Each fault below SHALL refuse one line on standard
  error with exit status 2 before any instance runs: an unreadable
  path; a document the reader rejects; a root mapping with no `groups`
  key; a root node that is a sequence; an empty `groups` sequence; an
  entry that is not a mapping; an entry with no `slots` key; a `slots`
  sequence that is empty; a member of the wrong kind; a member outside
  the context range; a repeated slot; a context no entry names; a key
  named `cpus`; any key outside FR-017's grammar; a repeated group
  name; a `name` outside FR-015's characters or length (R-2, R-3).
- **FR-019**: A layout file given beside a placement preset that names
  a partition for the machine read in hand SHALL refuse one line naming
  both, with exit status 2; a layout beside `--placement=none` SHALL
  stand allowed, and so SHALL a layout beside a preset that names no
  partition for that read, where the preset's pins stand and the file
  names the run's one partition. The check reads each selected
  instance's resolved preset: the name a handle stated
  (`registry.cpp:552-563`, `family.cpp:374-386`) carries the same
  refusal as the name the option settled. The check reads at the
  standing fit checkpoint before any instance runs (FR-020). A layout
  file given beside `--hunt` SHALL refuse one line naming both, the
  shape `--hunt cannot combine with --placement` keeps
  (`source/harness/hunt_cli.cpp:720-748`) (D-2, D-11).
- **FR-020**: The standing fit step SHALL read the layout before any
  instance runs and SHALL stop the batch, one line and exit status 2,
  when the layout's slot count differs from the context count of any
  instance the batch runs (D-8, specs/025 FR-018).
- **FR-021**: Every refusal line SHALL name the layout path, and a
  line naming one entry SHALL name the entry's index or name.

### The group's own rounds (E)

- **FR-022**: Each group SHALL stand behind one start round and one
  stop round of its own, each carrying exactly that group's contexts
  as participants, and a context SHALL arrive at its own group's
  rounds only (R-1, D-1).
- **FR-023**: No round SHALL carry contexts of two groups, and a
  grouped run SHALL carry no round the standing shape did not carry
  in some form: the two standing rendezvous points split by group,
  they do not multiply (R-1).
- **FR-024**: The group rounds SHALL keep the standing round rules:
  the participant count stands fixed at construction, no round sits
  inside a measured window, no round reads a clock or carries a
  timeout, and a stranded arrival stands a contract violation
  (`runner.cpp:2062-2065`, `execution_context.cpp:821-900`).
- **FR-025**: An ungrouped run SHALL keep the standing pair of rounds
  over every context of the instance, byte-shaped as master runs it
  (R-1, D-1).
- **FR-026**: A run whose contexts meet more than one start round
  SHALL publish context 0's fixture setup outcome to every body before
  any body's measured work through a route that carries no round
  across groups, and SHALL run context 0's fixture teardown after
  every body of the attempt has settled, on context 0's own thread
  (D-7). A run whose contexts meet one round keeps the standing
  publication and the standing teardown point.
- **FR-027**: A thread runner that returns with a group member
  undispatched SHALL meet the standing ledger fault before the run
  waits on any group round (FR-024).

### The disclosure (F)

- **FR-028**: A grouped run's console line for one context SHALL name
  the group as its last segment, after the placement flag where that
  flag prints, and SHALL keep the standing segments in the standing
  order (`report.cpp:168-191`). The standing machine line keeps its one
  rule of printing where the fit step reports a preset in effect
  (`cli.cpp:744-755`), so a run grouped by a layout that carries no
  preset prints none.
- **FR-029**: The report's per-context record SHALL carry a `group`
  key naming the same text the console prints, and the key SHALL
  stand present only for a run that carries a partition (D-10).
- **FR-030**: The report schema SHALL stand at the minor FR-032
  records, SHALL carry the optional `group` key of the context record,
  and the schema guard, the writer conformance and the reader
  round-trip tests SHALL move with the key (R-4).
- **FR-031**: The report reader SHALL answer the group name for each
  context record it loads, and a document that carries no `group` key
  SHALL load as the record naming no group.
- **FR-032**: The lane SHALL land the version `0.26.0`, the harness
  `SOVERSION` `17`, the archive `SOVERSION` `4` and the schema minor
  `1.10`, each re-read at landing against the tree the base stands at
  and moved one step higher where a landing took it, the check named
  in the commit that moves the numbers.
- **FR-033**: An ungrouped run's console output SHALL stand
  byte-identical to master's for the same fixture, apart from the
  schema stamp, and its document SHALL carry no `group` key (D-12).

### The run's other paths (G)

- **FR-034**: The jitter probe, the hunt path, the idle reads and the
  noise tracers SHALL keep their standing rendezvous shapes, and
  groups SHALL bind the instance's threaded run alone (D-11).
- **FR-035**: A grouped run SHALL keep the standing row shape: the
  merged row, its divisor, and the per-context percentile and raw
  columns SHALL not gain a group dimension.

### Tests, evidence and docs (H)

- **FR-036**: Every machine layout a test exercises SHALL stand built
  on the standing synthetic seam or the installed-layout seam
  (`internal.hpp:790`, `:820-823`); no test SHALL read `/sys` or the
  host's real topology, and the suite SHALL pass under
  `taskset -c 0,1` (R-5).
- **FR-037**: The suite SHALL carry a scenario for each FR of
  sections A to G, SHALL reach 100 percent line and branch coverage
  over the code this lane adds with no exclusion marker and no new
  ignored error class (VI), and SHALL carry no sleep and no clock
  read in a test.
- **FR-038**: The lane SHALL record its acceptance runs on the lane
  host under the results directory, one record per run naming the
  commands, the exit codes and the artifacts: the preset grouping
  over a multi-group shape, the `node` grouping over a multi-node
  shape, the layout run end to end, the refusal set, and the
  ungrouped comparison; the quickstart SHALL link every record.
- **FR-039**: The main page SHALL gain the groups section, its
  example region and its quick-reference rows in the same change as
  the behavior, and the example SHALL build and run and print the
  rows its result sentence names (IV.1).
- **FR-040**: The public header this lane grows SHALL carry doxygen
  `\pre`, `\post` and `\invariant` on every interface it adds, and
  the DBC gate SHALL read no finding (II).

## Key Entities

- **Group**: one named partition member: a name (D-4) and the
  contexts it holds, at least one. A group owns the two rounds its
  contexts meet.
- **Partition**: the set of groups covering an instance's contexts
  exactly once, named by one source, fixed for the instance. The
  absence of a partition is a standing shape, an empty group list.
- **Layout file**: the YAML document naming one entry per group with
  a name and either slots or cpus (FR-012). Its grammar is closed.
- **Group round**: one `sg::ContextBarrier` of the group's own
  participant count at one of the two standing rendezvous points.
- **Context disclosure**: the per-context record the console and the
  report share, this lane growing it by the group name beside the
  standing wait mode, placement text and applied flag.
- **Fit step**: the standing pre-run checkpoint that stops a batch
  over an unfittable placement, this lane growing it to read the
  layout (FR-020).

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A run of eight contexts over a four-L3-group layout
  under `--placement=l3` discloses exactly four groups of two
  contexts, each named `l3-0` to `l3-3`, and each context names the
  group its plan unit names (FR-006, FR-008).
- **SC-002**: A run of four contexts over a two-node layout under
  `--placement=node` discloses two groups of two, named `node-0` and
  `node-1`, holding the alternating contexts (FR-006, FR-008).
- **SC-003**: Every grouped scenario of the suite completes, which
  proves each group's rounds released with exactly that group's
  participant count; the suite's grouped scenario count stands at
  twelve or more across the two sources (FR-022, FR-037).
- **SC-004**: Each refusal shape FR-018 names, with the two pairings
  FR-019 names, answers one line on standard error with exit status 2
  and writes no report file: 18 of 18 cases (FR-018, FR-019, FR-021).
- **SC-005**: A layout file run names exactly the groups, names and
  memberships the file states, for the explicit-name case, the
  default-name case and the one-group case: 3 of 3 cases (FR-012 to
  FR-015, D-9).
- **SC-006**: The same fixture run without a partition prints
  console bytes identical to the bytes master prints, apart from the
  schema stamp, and its report carries no `group` key: 0 differing
  bytes on the compared regions (FR-033, D-12).
- **SC-007**: The report of a grouped run round-trips: the reader
  answers the group name for every context record, and the schema
  guard refuses a document that names a group outside its schema
  (FR-030, FR-031).
- **SC-008**: The suite passes with 100 percent line and branch
  coverage over the added code, no exclusion marker, sanitizer-clean,
  thread-sanitizer-clean, and green under `taskset -c 0,1` (FR-036,
  FR-037, VIII).
- **SC-009**: The prose, spell, format, drift and DBC gates read zero
  findings on the lane's range, and the main page's groups section
  equals its example region byte for byte (FR-039, IV.1, XI).
- **SC-010**: The results directory carries one record per acceptance
  run, five records at least, each naming commands, exit codes and
  artifacts, and the quickstart links every one of them (FR-038).

## Assumptions

- The plan already carries the unit each preset groups by, so the
  partition reads a value the lane finds computed and unused
  (`plan.group`, `plan.node`) and adds no new topology walk. Where the
  prompt's phrase "their own topology units" means a per-preset unit
  table that differs from D-3, the partition function is the one
  place to change and the group names carry the change (D-3).
- A grouped run's measured rows stay flat: the group is a pacing and
  disclosure fact, a column-shape change stands a separate item
  (FR-035).
- The lane ships the slot form of the layout and refuses the key
  `cpus`. The lane prompt's phrase "the cpus or the slots each group
  takes" names the cpu form beside it, and the form has no carrier on
  this tree: a plan cpu exists only under a preset plan, and R-2 leaves
  no partition-naming preset to pair with the file. The deviation
  stands recorded in D-6 and in Out of
  scope, and a later spec that lets a layout pin brings the form back
  (FR-014).
- The lane host's L3 shape differs from the lane prompt's named host;
  the synthetic seam carries the four-group and two-node shapes on
  either, and the records name the host that ran them (FR-036,
  FR-038).
- The `group` key joins the document as an optional key of the
  per-context record, the specs/023 minor rule: a key added inside
  major 1 moves the minor (R-4, FR-030).
- One layout file per batch stands enough: the prompt names one file
  as the source, and a batch of differing context counts refuses at
  the fit step (FR-020, D-8).
- specs/006's gate translation unit stays the link proof, and this
  lane's reader becomes the second `source/` includer specs/006
  FR-014 allows; the purity scan's A7 and A8 and the nm proof stay
  green unchanged (FR-016).

## Constitution Checklist

- Main-page impact check (IV.1): FR-039 carries the section, the
  example region and the quick-reference rows in the same change.
- Design By Contract (II): FR-040 carries the contracts of the added
  interfaces; the group partition function states its pre- and
  postconditions in one place.
- R-DCUT (III): the artifacts carry EARS requirements, user stories,
  the logical and physical design in plan.md, and the test plan; the
  lane runs in TDD mode, recorded in plan.md.
- Style (V): new identifiers take the N-1 to N-12 spellings; the
  group record's public members take camelBack (N-12).
- Coverage (VI): FR-037 states 100 percent line and branch over the
  added code with no exclusion marker.
- Performance (VII): the group rounds sit outside every measured
  window, and the lane records the round count per attempt so a
  regression in pacing shows in the disclosure.
- Hard gates (VIII): FR-036 and FR-037 state the sanitizer, tsan,
  two-cpu and coverage readings; FR-039 states the drift and prose
  readings.
- Spec-driven development (IX): this spec, plan.md, tasks.md and the
  implementation form the four artifacts of the item.
- Anti-slop (X.2): the lane adds no grouping switch, no second
  barrier kind, no group dimension in the row shape, and no public
  dispatch-by-group API (Out of scope).
- Prose (XI): this artifact and the lane's comments and commits carry
  no em-dash, no contrastive framing, no filler or marketing
  vocabulary, and stand checked by `prose-lint`.
