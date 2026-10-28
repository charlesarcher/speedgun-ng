# Feature Specification: NUMA Memory-Node Placement

**Feature Branch**: `035-numa-placement`

**Created**: 2026-10-28

**Status**: Draft

**Input**: User description, opening quoted:

> Extend the topology placement, item N12 of the roadmap (section 4,
> split from H10), with the NUMA memory nodes of the host: the
> discovery of the nodes, presets that plan and bind the contexts to
> them, and acceptance runs on thrawn under each NPS BIOS setting. A
> benchmark that spins on a cpu one node away from the memory it
> touches measures cross-node traffic the report never names, so the
> placement names the node beside the cpus.

The request fixes the rulings P-1 to P-7 of the lane prompt; they
stand in Decisions as D-1 to D-7.

## Spec identity and run-time binding

- Spec number: `035`. `specs/` on master `e4a98bc` holds `001` through
  `034` (read 2026-10-28). `031` is reserved for N0b host validation,
  `033` for N14a-2 hunter budgets. `git ls-remote --heads` names no
  ref carrying `035` or `numa-placement`, and `gh pr list` names no
  open pull request (read 2026-10-28).
- Branch: `035-numa-placement`, cut from master `e4a98bc` on
  2026-10-28. Master stands on that hash at branch time, so no
  moved-base record applies.
- Version: master stands at `0.21.0` (`CMakeLists.txt:15`), harness
  `SOVERSION 13` (`CMakeLists.txt:298`), archive `SOVERSION 4`
  (`CMakeLists.txt:80`). The lane takes the next free minor,
  `0.22.0`. The preset names ride the standing string grammar and the
  binding seam is a detail seam, so no public-ABI change lands: the
  harness `SOVERSION` stays `13` and the archive `SOVERSION` stays
  `4` (D-6). The check is recorded beside the version move, in the
  commit that moves it (FR-003).
- Report schema: master stands at `1.6` beside
  `source/harness/json_report.cpp:36`, the top-level `x-order` at
  `docs/schema/report-v1.json:7`. The node rides the standing
  `placement` string of each context
  (`docs/schema/report-v1.json:350`), so the schema gains no key and
  the minor stays `1.6` (D-5).
- Option values: no option row is added and no option value moves.
  The node plans ride the standing `--placement` row and the
  `BenchmarkHandle::placement(name)` handle of spec 025
  (`source/harness/options.cpp:620`, the
  `BenchmarkHandle::placement(name)` handle of
  `include/speedgun-ng/benchmark.hpp:2561`), whose six names this
  feature lands at `source/harness/placement_preset.cpp:46`.

## Roadmap position

- Item N12 of the harness roadmap, section 3 and the section 4
  allocation table. N4 (the topology placement, spec 025) is merged
  through PR #42, and N13b (the host noise pre-check, spec 034) is
  merged through PR #54; the NPS acceptance runs of this feature then
  measure the noise for each NPS BIOS setting with the pre-check the
  merged tree carries. Roadmap section 10.6 stays in force in full.

## Lane identity

- The acceptance shape stands on the request's lane host: a
  Threadripper 9960X, 24 cores, 48 threads, one socket, several CCDs
  with one L3 each, 128 GB ECC, and the NPS BIOS setting. The NPS
  runs need a BIOS change and a reboot per setting, so the evidence
  plan stands written before the code tasks that depend on it land
  (FR-019). The suite itself stays
  host-agnostic: the nodes come from the machine read the placement
  already performs, the binding goes through an injected seam, and
  the suite passes on a 2-cpu single-node host (FR-019). The request
  names the host; the spec carries the shape alone, the file-hygiene
  rule of `specs/032-fix-merged-features` FR-024 binding repository
  files.

## Audit point

- Repository: `master` at `e4a98bc` (project version `0.21.0`, report
  schema `1.6`, harness `SOVERSION 13`, archive `SOVERSION 4`), read
  2026-10-28 on the lane host.
- The five placement preset names `none`, `core`, `l3`, `smt`,
  `compact` stand at `source/harness/placement_preset.cpp:46`
  (`kPresetNames`, matched in full and nothing else), behind the
  public `BenchmarkHandle::placement(name)` contract at
  `include/speedgun-ng/benchmark.hpp:2553`.
- The machine read stands at `source/harness/topology.cpp`: the
  listing header at `:419` (the `topology: packages=...` line),
  one line per L3 group at `:434`,
  and the online-set test seam `installTestOnlineCpus` at `:322`.
  The unit is the one harness translation unit that includes
  `<hwloc.h>` (spec 025 FR-004, R-01), against the vendored hwloc
  pinned at `2.14.0` (`source/hwloc/hwloc_gate.cpp:13`), covered by
  `tools/hwloc/hwloc_nm_proof.sh`.
- The placement function seam stands public at
  `include/speedgun-ng/execution_context.hpp:185` (`PlacementFn`)
  and `:202` (`setPlacementFunction`); the default placement
  function produces the `cpu@group` pair of the slot
  (`source/harness/report.cpp:156`), disclosed per context in the
  console row and in the context JSON field
  `docs/schema/report-v1.json:350`.
- Spec 025 keeps the memory nodes out of its scope:
  `specs/025-topology-placement/spec.md:722` (memory-node presets and
  the NPS tests are N12) and
  `specs/025-topology-placement/contracts/cli-placement.md:50` (the
  row that names no memory node in the listing, amended to the
  touched-node rule of FR-006). This spec amends
  the listing contract line, the standing exit-status table beside
  it, the standing `--placement` row's name list in
  `specs/015-benchmark-harness-core/contracts/cli.md`, and no other
  standing rule (D-3, FR-013, FR-014).
- The timing-margin record stands at
  `specs/032-fix-merged-features/results/record.md`: a test bound
  that walks near the sanitizer's own load reads flaky, and a bound
  keeps an order of magnitude of margin inside the contamination
  signal it discriminates against.

## Standing facts this lane reads

- The preset names, the fit check and the listing rules stand in
  `specs/025-topology-placement/contracts/cli-placement.md` and
  `contracts/topology-placement.md`; this spec amends the
  no-memory-node scope line, the listing rule row beside it, the
  exit-status table, and the standing `--placement` row's name list
  in `specs/015-benchmark-harness-core/contracts/cli.md`, and no
  other standing rule.
- The pre-check block and the `proc_root` seam of spec 034 stand
  beside the placement disclosure and stay unchanged.
- The hwloc dependency is vendored and pinned; the lane adds no
  dependency and no second topology reader (D-1).
- The hwloc object kinds of the spec 025 data model already carry
  the NUMA node kind; the read extends to it (`hwloc_obj_type` scan
  at `source/harness/topology.cpp:175` walks the same tree).

## Clarifications

### Session 2026-10-28

- Q: How does the node appear inside the standing `placement` field
  of a bound context? → A: The node rides the same `@`-joined shape
  as the group: the field reads `cpu@group@node` where the plan
  binds one, and the field keeps the standing `cpu@group` bytes when
  it does not (FR-011).
- Q: Do the jitter probe's own measurement windows run with the
  binding applied when a node preset stands? → A: The probe windows
  keep the standing memory policy the process carries: the binding
  applies only at the context starts the node preset plans (D-4,
  FR-010).
- Q: What line format does the `--topology` listing print for each
  memory node? → A: The node lines mirror the L3 lines' shape,
  `node <n>: <cpu-list>`, and the node set stays truthful to the
  topology hwloc states: the node numbers, the cpus and the count
  come from the read, with no abstraction beyond the listing shape
  (FR-006).
- Q: When the context count is smaller than the node count, which
  nodes does the `node` plan place contexts on? → A: The plan fills
  the nodes in hwloc's node order: the nodes `0` to `k-1` take one
  context each and the later nodes take none (FR-009).
- Q: Which NPS BIOS settings must the recorded evidence cover? → A:
  Every NPS state the board's menu offers; a state the menu cannot
  produce is recorded as out of reach in the results record
  (FR-019, SC-005).

## Decisions

- **D-1 The binding method is hwloc membind.** The binding applies
  through the hwloc call set the vendored library carries, behind
  one named detail seam the tests replace the way
  `installTestOnlineCpus` replaces the online set. libnuma stays out
  of the dependency set; the lane adds no dependency and no second
  topology reader.
- **D-2 The preset grammar carries the node plans.** One new name
  joins the five standing names: `node`, which plans the contexts of
  a threaded run one cpu at a time, cycling the mask-touching nodes
  in the read's node order. The plan's `places` stands at the
  distinct-cpu count of those nodes, so the standing fit check
  passes unchanged and the unfittable refusal stands reachable at
  cpu exhaustion; a context count below the node count fills the
  nodes in hwloc's node order, one context each, and the later
  nodes stand without a context. The standing `l3` plan wraps
  nothing (specs/025 pins the refusal at the place count), and this
  arm stands beside it with no wrap-around of its own. No new
  option row, no option value, no argument form: the bare name is
  the whole grammar this feature adds.
- **D-3 The listing gains the node lines.** Under the L3 lines the
  listing prints one line per memory node the process mask touches,
  in the L3 lines' shape, naming the node number and the cpus of
  that node inside the process mask; a node whose cpus all stand
  outside the mask prints no line, the way the plan gives it no
  context. The node numbers and the counts stay the ones the hwloc
  read states: the listing renames and regroups nothing. A host
  whose read states no memory information prints `not available`
  for the node lines alone and keeps the rest of the listing
  byte-identical to today.
- **D-4 The binding policy.** The binding applies at context start,
  on the context thread, beside the cpu pin the placement applies.
  The plan binds each context to the node its cpu belongs to. The
  jitter probe's measurement windows run before any context starts
  and keep the standing memory policy the process carries. A binding
  the system denies is a usage error: one line naming the context
  and the node, exit status 2, before any benchmark starts. The run
  validates the binding before the contexts spawn: the main thread
  binds and restores each planned context's node in plan order, one
  pass over the plan, so a denial the host will state stands refused
  ahead of every context, and the refusal line names the context and
  the node; a bind that fails at a context start despite the
  validation stands the same refusal path.
- **D-5 The disclosure rides the standing placement field.** The
  placement field of each context carries the node beside the group
  where the plan binds one, console and JSON, with no new key; the
  schema minor stays `1.6`. A run with no node preset gives the same
  console report and the same JSON bytes apart from the version
  figures the report carries: the report version moves with FR-003,
  and the schema version stands at `1.6` on both sides.
- **D-6 No public-ABI change.** The preset names ride the standing
  string grammar, the binding seam is a detail seam, and the
  `PlacementFn` contract stands unchanged. The harness `SOVERSION`
  stays `13`, the archive `SOVERSION` stays `4`, and the check is
  recorded beside the version move.
- **D-7 The NPS evidence lands before the lane closes.** The BIOS
  change and the reboot plan, the runs per setting, and the recorded
  evidence stand in the results directory, each record naming the
  NPS setting and the boot it ran on, with the merged pre-check run
  beside the placement runs. The record set covers every state the
  board's menu offers, and a state the menu cannot produce stands
  recorded as out of reach with the gap named.

## Out of scope

- A per-node argument form of the preset (`node=<n>` and the like):
  the standing grammar carries bare names, and a named-node form is
  a follow-up feature with its own grammar record.
- A memory-binding method outside the hwloc call set: libnuma and
  first-touch policy stay out (D-1).
- Interleave, bind-all and other memory policies of a whole process:
  the feature binds the contexts of a threaded run, and the process
  policy stands as master carries it.
- Changes to `PlacementFn`, to the fit-check rules of spec 025, and
  to the N5 rule keys: no rule names a placement field.
- The N13d, N13e and N10 lanes: they reuse the seams this feature
  leaves and change none.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - A run planned onto the nodes binds and names them (Priority: P1)

A user runs a threaded benchmark family with
`--placement=node` on a multi-node host. Each context runs on a cpu
of one memory node, the context's memory stays on that node, and the
console row and the JSON context field name the node beside the
group. A run whose memory sits one node away from its spinning
cpus now reads as the cross-node run it is.

**Why this priority**: the binding and its disclosure are the
feature's own value; without them the node plans name nothing the
report carries.

**Independent Test**: with an injected topology of two nodes and a
fake binding seam, a run with the `node` preset discloses the node
of each context in console and JSON, and the binding call carries
the node the plan states.

**Acceptance Scenarios**:

1. **Given** an injected topology of two nodes and eight contexts,
   **When** the run plans with the `node` preset, **Then** the
   contexts alternate over the two nodes and each row names the
   node beside the group.
2. **Given** a run with no node preset, **When** the run completes,
   **Then** the console report and the JSON document carry the same
   bytes as master apart from the version figures the report
   carries.

### User Story 2 - The listing names the memory nodes (Priority: P2)

A user runs `--topology` before planning and reads one line per
memory node under the L3 lines, each naming the node number and the
cpus of that node inside the process mask. The plan the user picks
stands on the lines the listing prints.

**Why this priority**: the discovery is what the preset plans
against, and the listing is the surface the user reads first.

**Independent Test**: with an injected topology, the listing prints
one line per node inside the mask, and a host whose read states no
memory information prints `not available` for the node lines alone.

**Acceptance Scenarios**:

1. **Given** an injected topology of two nodes with cpus split
   across them, **When** the listing runs, **Then** one line per
   node stands under the L3 lines, naming the node number and the
   cpus inside the mask.
2. **Given** a host whose topology read states no memory
   information, **When** the listing runs, **Then** the node lines
   print `not available` and the exit status stays 0.

### User Story 3 - A plan that cannot bind refuses before it runs (Priority: P2)

A user asks for a node plan the host cannot honor: the binding call
is denied, or the node plan cannot fit the contexts inside the
process mask. The run refuses with one line and exit status 2 before
any benchmark starts, the standing refusal shape of the placement.

**Why this priority**: a silently unbound run reads as a placed run
and records a figure the report cannot explain; the refusal keeps
the report trustworthy.

**Independent Test**: with a fake binding seam that denies the
call, the run refuses one line, exit status 2, with no benchmark
output.

**Acceptance Scenarios**:

1. **Given** a fake binding seam that denies the call, **When** a
   run with the `node` preset starts, **Then** one line names the
   context and the node, the exit status is 2, and no benchmark
   output prints.
2. **Given** a context count the node plan cannot fit inside the
   process mask, **When** the run starts, **Then** the standing
   unfittable-count refusal prints one line and exits 2.

### User Story 4 - The NPS runs stand recorded (Priority: P2)

The lane records the placement runs and the merged pre-check run on
thrawn for each NPS BIOS setting it exercises, under the results
directory, each record naming the setting and the boot. A later lane
reads the records and knows which NPS value the figures carry.

**Why this priority**: the NPS shape is the acceptance shape the
roadmap names, and the evidence is what the later noise lanes read.

**Independent Test**: the results directory carries one record per
run setting, each naming the NPS value, the boot, the commands and
the exit codes, and the quickstart evidence links it.

**Acceptance Scenarios**:

1. **Given** a BIOS change to one NPS setting and a reboot,
   **When** the acceptance runs execute, **Then** the record names
   the NPS value, the boot, the commands, the exit codes and the
   listing the run printed.
2. **Given** the recorded evidence, **When** the lane closes,
   **Then** the results directory links every record and the
   quickstart carries the commands.

### User Story 5 - The docs teach the node plans (Priority: P3)

A reader of the README and the guide page learns the `node` preset,
the listing's node lines and the refusal rules from the same
sections that teach the standing presets, and the runnable example
shows a node-planned run whose bytes the shape gate guards.

**Why this priority**: the docs gates bind every feature, and the
drift gates fail a page whose quoted run drifts away from what the
binary prints.

**Independent Test**: the example builds and runs, its `.expected`
file matches the run through the shape fold, and the snippet
compiles.

**Acceptance Scenarios**:

1. **Given** the guide page's recorded example run, **When** the
   shape gate runs the example binary, **Then** the quoted block
   folds equal against the run.
2. **Given** the README section, **When** the prose gate runs,
   **Then** the section stands inside the 25-line cap with no
   finding.

### Edge Cases

- A single-node host: the `node` preset plans every context onto
  that one node's cpus, and the disclosure names the node.
- A node whose cpus all stand outside the process mask: the plan
  gives the node no context, the way the L3 plan skips a group the
  mask does not touch.
- A context count smaller than the node count: the plan fills the
  nodes in hwloc's node order, one context each, and the later
  nodes stand without a context.
- A context count the node plan cannot fit inside the process mask:
  the standing unfittable-count refusal, one line, exit status 2.
- A binding the system denies: one line naming the context and the
  node, exit status 2, before any benchmark starts.
- A topology read that states no memory information: the node lines
  disclose `not available`, and a node preset requested over such a
  read refuses one line, exit status 2.
- A machine read the harness cannot read at all: the standing
  refusal of spec 025 stands unchanged.
- A run that carries the jitter probe beside a node preset: the
  probe's measurement windows run with the standing memory policy,
  and the binding stands only at the context starts.

## Requirements *(mandatory)*

### The surface, the version and the schema (A)

- **FR-001**: The preset grammar SHALL carry the name `node` beside
  the five standing names, matched in full and nothing else, and the
  standing `--placement` row and `BenchmarkHandle::placement(name)`
  handle SHALL carry it with no new option row and no option value.
- **FR-002**: A run with no node preset SHALL produce the same
  console report and the same JSON bytes as master apart from the
  version figures the report carries: the report version moves with
  FR-003, and the schema version stands at `1.6` on both sides.
- **FR-003**: The version SHALL stand at `0.22.0` with the harness
  `SOVERSION` at `13` and the archive `SOVERSION` at `4`, and the
  commit that moves the version SHALL record the ABI check (D-6).
- **FR-004**: The report schema SHALL stay at minor `1.6`; the node
  SHALL ride the standing `placement` field of the context with no
  new key (D-5).

### The discovery and the listing (B)

- **FR-005**: The machine read SHALL name the memory nodes of the
  process mask, with the node number and the cpus of each node, read
  one time per run through the standing topology read.
- **FR-006**: The `--topology` listing SHALL print one line per
  memory node the process mask touches, under the L3 lines, in the
  L3 lines' shape, `node <n>: <cpu-list>`, naming the node number
  hwloc states and the cpus of that node inside the process mask;
  the node numbers and the counts stay the ones the hwloc read
  states, with no abstraction beyond the line shape (D-3).
- **FR-007**: A topology read that states no memory information
  SHALL print one line reading `node: not available` in the node
  family's place, keep the rest of the listing unchanged, and exit
  0.
- **FR-008**: A machine read the harness cannot read SHALL refuse
  one line with exit status 2, the standing rule of spec 025,
  unchanged.

### The node plan and the binding (C)

- **FR-009**: The `node` preset SHALL plan the contexts of a
  threaded run one cpu at a time, cycling the mask-touching nodes in
  the read's node order; the plan's `places` SHALL stand at the
  distinct-cpu count of those nodes, so the standing fit check
  passes unchanged; a context count smaller than the node count
  SHALL fill the nodes in hwloc's node order, one context each, and
  leave the later nodes with none (D-2).
- **FR-010**: The binding SHALL apply at context start, on the
  context thread, beside the cpu pin, and SHALL bind the context to
  the node its planned cpu belongs to; the jitter probe's
  measurement windows SHALL keep the standing memory policy (D-4).
- **FR-011**: The placement field of each bound context SHALL name
  the node beside the group in the same `@`-joined shape, reading
  `cpu@group@node` where the plan binds one and keeping the standing
  `cpu@group` bytes where it does not, in the console row and in the
  JSON context field, with no new key (D-5).
- **FR-012**: The cpus of the plan SHALL stay pairwise distinct and
  inside the process mask, the standing postcondition of spec 025's
  resolver, and the plan SHALL pass the standing fit check against
  the context count.

### The refusals (D)

- **FR-013**: A binding the system denies SHALL refuse one line
  naming the context and the node, with exit status 2, before any
  benchmark starts: the run SHALL validate the binding before the
  contexts spawn, binding and restoring each planned context's node
  in plan order, and the refusal line SHALL name the context and the
  node; a bind that fails at a context start SHALL stand the same
  refusal (D-4).
- **FR-014**: A node preset requested over a topology read that
  states no memory information SHALL refuse one line with exit
  status 2, before any context starts.
- **FR-015**: An unknown preset name SHALL refuse one line with
  exit status 2, the standing rule of spec 025, unchanged.

### The seams and the suite (E)

- **FR-016**: The binding SHALL stand behind one named detail seam
  that tests replace, the way `installTestOnlineCpus` replaces the
  online set, and the tests SHALL drive it with synthetic
  topologies of one, two and four nodes (D-1).
- **FR-017**: The suite SHALL pass under `taskset -c 0,1` and on a
  2-cpu single-node host, with the cpus and the nodes read from the
  injected topology and the real machine read carrying no fixed
  node number.
- **FR-018**: No test SHALL sleep and no test SHALL read a clock:
  the binding carries no timing figure, and the timing bounds the
  suite keeps stand an order of magnitude inside the contamination
  signal they discriminate against (the record of spec 032).

### The evidence and the docs (F)

- **FR-019**: The results directory SHALL carry one record per NPS
  BIOS setting the board's menu offers, naming the setting, the
  boot, the commands, the exit codes and the listing the run
  printed, with the merged pre-check run beside the placement runs;
  a state the menu cannot produce SHALL stand recorded as out of
  reach, with the gap named (D-7).
- **FR-020**: The README SHALL carry a section of at most 25 lines
  teaching the node plans, and the quick-reference table SHALL carry
  the rows.
- **FR-021**: `docs/pages/harness.md` SHALL carry the option and
  field detail beside the placement sections spec 025 landed.
- **FR-022**: One runnable example under `example/docs/` SHALL carry
  a node-planned run with its `.expected` file, and the shape gate
  SHALL fold it against the binary's own run.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A run with the `node` preset discloses the node of
  every bound context in the console rows and in the JSON context
  fields, and a run without the preset stands byte-identical to
  master apart from the version figures the report carries.
- **SC-002**: The `--topology` listing prints one line per memory
  node the process mask touches and no line for a node the mask
  excludes, and the lines fold stable across hosts through the
  shape gate's mask.
- **SC-003**: Every refusal of D prints one line and exits with
  status 2, and no benchmark output stands ahead of the refusal.
- **SC-004**: The coverage gate reads 100.0% lines and a branch
  shortfall inside the D-14 recorded allowance, with no
  `LCOV_EXCL` marker and no new `--ignore-errors` entry.
- **SC-005**: The NPS evidence stands recorded for every state the
  board's menu offers, an unreachable state stands recorded as out
  of reach, and the quickstart links each record.
- **SC-006**: The docs gates stand green: the example drifts from
  nothing, the snippet compiles, and the prose gate reads no
  finding.

## Assumptions

- The cycling rule of the node plan (D-2) stands as a stated
  default: the request names memory-node presets without fixing the
  many-contexts shape, and the standing fit check bounds it at the
  distinct-cpu count of the mask-touching nodes.
- The binding applies to the allocations the context thread makes
  after the bind; memory another thread allocates follows its own
  policy, and the docs say so beside the binding rules.
- The hwloc read of the NUMA node kind rides the same one-time
  topology read the placement performs; no second read, no second
  reader.

## Constitution Checklist

- Main-page impact check (IV.1): the feature changes what the root
  `README.md` teaches, so the page and its `example/` regions move
  in the same change (FR-020, FR-022).
- Coverage (VI): 100% line and branch coverage, no exclusions, the
  D-14 allowance stands recorded beside the gate.
- Performance (VII): the binding adds no timed-path work beyond the
  one bind call per context start, outside the timed windows.
- Prose (XI): every generated line clears the prose gate; the code
  spans stand on one line.
