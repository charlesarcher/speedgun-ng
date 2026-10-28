# Research: NUMA Memory-Node Placement

Feature: specs/035-numa-placement (roadmap N12). Every decision below
resolves an open point of the spec; the audit point of the spec names
the tree the findings read (master `e4a98bc`).

## D-1: the binding method

- **Decision**: hwloc membind, through the vendored `sg_hwloc`
  archive the placement already reads.
- **Rationale**: the topology the plan resolves against is the hwloc
  read the harness performs one time per run
  (`source/harness/topology.cpp`), so the node bitmaps and the bind
  call stand on the same objects the plan names; the pin already
  rides the platform call set the context start makes
  (`pthread_setaffinity_np`, `execution_context.cpp:118`), and the
  bind rides beside it. hwloc 2.14.0 carries the membind entry
  points with the by-node, thread-scope form the feature needs.
- **Alternatives considered**: libnuma (`numa_alloc_onnode`,
  `mbind`): a second dependency and a second topology reader for the
  same objects hwloc already states; rejected (spec D-1). First-touch
  policy through allocation placement: it binds nothing at context
  start and the disclosure could not name the node the context
  binds to; rejected.

## D-2: the `node` preset and its plan

- **Decision**: one preset name, `node`, in the standing grammar;
  one context per node inside the process mask; cycle the nodes when
  the context count exceeds the node count; fill in hwloc's node
  order when the count sits below.
- **Rationale**: the standing five presets plan by machine objects
  the read states (`l3` plans per L3 group, `core` per core), and
  the feature table of the roadmap names memory-node presets in the
  same shape. The deterministic node-order fill follows the `l3`
  plan's own ordering precedent; the cycling rule stands new, with
  no standing precedent, and the plan's `places` bounds it at the
  distinct-cpu count of the mask-touching nodes, beside the standing
  fit check. The hwloc node order keeps the plan deterministic for
  the fixture tests and comparable across boots (the clarify answer
  of 2026-10-28).
- **Alternatives considered**: an argument form (`node=<n>`) that
  names one node: new grammar the standing parse does not carry, and
  the standing rule keeps bare names; deferred to a follow-up
  feature with its own grammar record. Round-robin spread over all
  nodes at any count: it leaves no node without a context on
  unbalanced counts and reads arbitrary beside the machine's own
  order; rejected.

## D-3: the listing lines

- **Decision**: one line per node under the L3 lines, in the L3
  lines' shape, `node <n>: <cpu-list>`; the node set, the numbers
  and the counts stay the ones the hwloc read states.
- **Rationale**: the L3 lines already stand as the reader's model
  (`topology.cpp:434`), one fold rule covers both line families, and
  the truthfulness rule keeps the listing a mirror of the read
  (the clarify answer of 2026-10-28).
- **Alternatives considered**: memory size beside the cpus: the size
  unit invites a formatting rule the spec carries no requirement
  for, and the read states it per node with no harness need; a
  header count alone (`nodes=2`): it hides the cpu lists the plan
  reads off; rejected.

## D-4: the binding policy and the refusal

- **Decision**: the bind stands at the context start, on the context
  thread, after the pin call, outside the replaceable placement
  function; a denied bind on a planned context is a usage error,
  one line, exit status 2, before any benchmark starts.
- **Rationale**: the placement function seam is public and
  replaceable (`setPlacementFunction`,
  `execution_context.hpp:202`); a bind inside it would vanish under
  a user-installed function and the disclosure would name a node
  nothing bound. The context start already owns the pin-failure
  account path (`execution_context.cpp:440`), and the refusal rides
  it.
- **Alternatives considered**: binding inside the default placement
  function: the user function replaces the whole function, so the
  bind would stand behind a contract change or a silent skip;
  rejected (spec D-6). A warning that leaves the run unbound: the
  report would name a node nothing bound; rejected.

## D-5: the disclosure shape

- **Decision**: the placement field reads `cpu@group@node` where the
  plan binds one and the standing `cpu@group` bytes where it does
  not; no new JSON key; the schema minor stays `1.6`.
- **Rationale**: the field is one token the console row and the JSON
  context carry today (`report.cpp:156`,
  `docs/schema/report-v1.json:350`), the `@`-joined shape keeps one
  separator rule, and the clarify answer of 2026-10-28 fixed it. The
  compose reads the layout's cpu-to-node map, so the public
  `PlacementSlot` struct stands unchanged.
- **Alternatives considered**: a separate JSON `node` field: a key
  addition and a schema minor for a value one token carries; a
  `cpu@node` replacement of the group: it hides the L3 group the
  standing disclosures name; rejected.

## D-6: the ABI and version check

- **Decision**: no public type, function or struct changes; the
  harness `SOVERSION` stays `13`, the archive `SOVERSION` stays
  `4`, and the version moves to `0.22.0`.
- **Rationale**: the preset names ride the standing string grammar
  (`kPresetNames`, `placement_preset.cpp:46`), the node data stands
  in the detail layout (`internal.hpp:94`), and the seam is a detail
  seam. The check stands recorded in the version commit, the way
  specs/025 and specs/034 recorded theirs.
- **Alternatives considered**: publishing the node plan as a public
  accessor: no requirement names one, and the public surface grows
  for nothing; rejected.

## D-7: the NPS evidence

- **Decision**: one record per NPS state the board's menu offers,
  each naming the setting, the boot, the commands, the exit codes
  and the listing, with the merged pre-check run beside the
  placement runs; an unreachable state stands recorded as out of
  reach with the gap named.
- **Rationale**: the acceptance shape of the roadmap names the NPS
  BIOS setting, the reboots make the runs the lane's scarcest
  resource, and the record set that names every state stands
  complete on its own terms (the clarify answer of 2026-10-28). The
  pre-check of spec 034 runs beside the placement runs and measures
  the host per state, which is the noise figure the later N13 lanes
  read.
- **Alternatives considered**: the extremes only: the middle states
  are the ones a many-CCD host distinguishes, and the later lanes
  would ask for them; the current setting alone: the record set
  would stand incomplete by choice; rejected.

## The seams and the tests

- **Decision**: one detail seam, `installTestMemoryBinder`, in the
  new detail header; synthetic topologies of one, two and four
  nodes through `installTestLayout`; the suite passes under
  `taskset -c 0,1` and on a 2-cpu single-node host.
- **Rationale**: the layout seam already replaces the whole machine
  read for the standing tests (`topology.cpp:306`), so the node data
  rides it with no new injection point; the binder seam stands
  beside it with the clear-on-null shape the layout seam uses.
- **Alternatives considered**: binding real memory in tests and
  checking the policy: it reads a policy the host may deny and
  breaks the no-hardware rule; a sysfs-based reader: a second
  reader beside hwloc; rejected.

## The disclosure and the shape gate

- **Decision**: the example records a node-planned run with a fixed
  context count of two, and the report-shape contract records the
  fold rule for the node tail.
- **Rationale**: the shape gate folds digit runs, so the bound field
  folds to `cpu@#@#` and the unbound to `cpu@#`; the recorded run
  carries the bound shape on every host the gates run on, because
  the node tail stands whenever the preset binds, on a one-node
  host included. The count of two keeps the row count stable across
  hosts the way the 034 example's quiet host kept its block stable.
- **Alternatives considered**: an unbound example: it exercises none
  of the feature; a larger context count: the row count varies with
  the host's node count under a cycle plan; rejected.

## The report-shape fold rule

- **Decision**: the placement field's node tail folds by the
  standing digit rule with no new fold: `cpu@#@#` stands distinct
  from `cpu@#`, and the contract records the two shapes.
- **Rationale**: the mask folds digit runs to `#` and keeps the
  separators, so the tail needs no rule of its own; the contract
  records the shapes so the fixture pairs stand pinned.
- **Alternatives considered**: folding the whole field to one
  token: it would hide the group the unbound runs name; rejected.
