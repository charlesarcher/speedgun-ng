# Research: Topology Placement

Phase 0 output. Every decision carries the requirement it serves, the
reason it was taken, and the alternative that was rejected. The audit
point is `ab32400b86321f98f13b9c66d4488e15831b47f6`.

## R-01: The harness archive owns the hwloc include

- **Decision**: `source/harness/topology.cpp` is the one harness
  translation unit that includes `<hwloc.h>`. `speedgun-ng_harness` takes
  the vendored include directory and the imported target `PRIVATE`, the
  way `speedgun-ng_speedgun-ng` does today, which supplies the staged
  headers and the build-order edge, and the imported target stays
  unexported. The harness archive carries no vendored member: its
  `sg_hwloc_*` references stay undefined, and the vendored archive reaches
  the final link through that `PRIVATE` link, which CMake records as a
  link-only interface for a static library and which the shared CI
  configuration needs directly because `MERGE_INTO` is ignored there
  (`cmake/ImportAutotoolsSubmodule.cmake:466-472`).
- **Rationale**: the layout is harness-private state (FR-003), the code
  that consumes it is the runner, the report and the CLI, and all three
  live in the harness archive. specs/003-vendor-hwloc FR-021 confines
  hwloc includes to `source/` translation units, which a harness unit
  satisfies. The library archive's gate unit, its 2.14.0 tripwire and its
  `nm` proof stay exactly as feature 003 shipped them.
- **Alternatives considered**:
  - Read in the library archive and hand the harness a seam across the
    archive boundary. Rejected: a seam across the two archives needs an
    exported symbol, and an exported hwloc-facing symbol is a public
    surface the privacy contract forbids.
  - Move the vendored merge from the library archive to the harness.
    Rejected: the library archive keeps the version tripwire and the
    existing `nm` proof, and moving the merge would rewrite feature 003's
    gates for a feature that does not need it.
- **Verification task**: T005 measures what the built archives carry. It
  records `nm` output showing the harness archive's undefined prefixed
  references and an executable resolving them with the vendored archive on
  its link line. The capture is the static tree only: the shared
  configuration builds the harness as a shared object, so T005 records there
  only that it configures, builds and installs with no harness archive
  present. The
  purity scan and the library archive's three assertions keep their
  current form.
- **Measured at T005 on the static tree**: the harness archive holds 12
  undefined `sg_hwloc_*` references and zero defined ones, and the
  `first_benchmark` executable holds the same names defined with the
  vendored archive on its link line. The shared tree configures, builds
  and installs with no harness archive present, its harness shared
  object carrying the vendored members locally.

## R-02: `MachineLayout` is a plain value

- **Decision**: one struct with three ordered vectors (L3 groups, cores,
  hardware threads), ordinal indexes as identity, each thread carrying
  its core ordinal and its group ordinal, plus the cpu mask the layout was
  built from.
- **Rationale**: the presets need only group membership, core membership,
  sibling order and cpu index. Ordinal identity keeps the value copyable
  and comparable, which is what the determinism test (SC-03) and the
  injected layouts need. hwloc's own object pointers stay inside
  `topology.cpp` and die with the topology.
- **Alternatives considered**: keep the hwloc topology alive and query it
  per run. Rejected: FR-002 wants one read, the tests would then need
  hwloc, and a live topology object is a resource with a lifetime the
  runner would have to manage.

## R-03: The preset resolver is a pure function

- **Decision**: `resolvePreset(Preset, const MachineLayout&, int contexts)`
  returns `PlacementPlan` by value. `places` always carries the preset's
  place count, and `cpu` stands empty exactly when the context count exceeds
  it.
- **Rationale**: one function holds the whole placement rule, so the five
  presets, the fit limit and the determinism rule are testable without a
  thread, a pin, a privilege, or the host (SC-04). An empty `cpu` is the only
  failure the resolver can produce, and the caller turns it into the one-line
  status 2 stop, printing the `places` value the plan still carries (FR-018).
- **Alternatives considered**: a strategy object per preset behind an
  interface. Rejected under Principle X: five names, one switch, no
  second implementation to justify the indirection.

## R-04: The test seam is an install call in the internal header

- **Decision**: two installs in `source/harness/detail/internal.hpp`.
  `installTestLayout(std::optional<MachineLayout>)` short-circuits the
  read for the resolver and report tests.
  `installTestTopologyDescription(std::optional<std::string>)` hands the
  read a synthetic description string, so the read calls
  `hwloc_topology_set_synthetic()` and folds its return into the same
  readable decision. The cache is never invalidated: an
  install takes effect only before the first read of that process, so a
  scenario that needs its own read takes a fresh process by the fork and execv
  pattern of `test/source/harness_context_barrier_test.cpp:116-126`.
- **Rationale**: FR-006 requires a test to write the seam itself, with
  no public header, no environment variable and no file. The internal
  header is already the archive's private surface, and the same pattern
  carries the report test's control of the runnable-cpu count today
  (`selectRunnableCpuCount` at external linkage for the test). The
  description install is what makes the read's own branches coverable: the
  coverage gate is aggregate and unwaivable, and a description the library
  rejects returns -1, which is the only way a test can reach the branch
  that leaves `readable` false.
- **Alternatives considered**: an environment variable naming a
  description file. Rejected: it hides the machine under test from the
  reader and adds a parser to cover. The vendored configure keeps the
  synthetic route open: `--disable-libxml2` drops the XML library and leaves
  the synthetic component in the build, and the pinned tree shows it:
  `config/hwloc.m4:205` of the pinned submodule commit `b5660dff` sets
  `hwloc_components="noos xml synthetic xml_nolibxml"`, the component's
  source `hwloc/topology-synthetic.c` stands in that tree, and
  `include/hwloc.h:1972` of the same commit declares
  `hwloc_topology_set_synthetic`. Those three anchors sit in a submodule
  that stands uninitialized on this machine, as the spec's audit point
  records, so they were read from the pinned commit's sources fetched
  from upstream at that commit, with no local checkout involved. The
  measurement the audit point recorded ran against the system hwloc
  2.15.0: `hwloc_topology_set_synthetic("2 2 2")` built an eight-unit
  machine with no cores and no L3, and a malformed string returned -1.
  The same probe measured the mask dependence: the `2 2 2` cpu set
  intersects to 4 units under `taskset -c 0-3` and to none under
  `taskset -c 8-11`, and the `1` description intersects to 1 under a mask
  holding cpu 0 and to none under `taskset -c 4-7`.
- **Verification task**: T010 records the three pinned-commit anchors named above
  from the checked-out submodule and confirms the synthetic route through the
  vendored build at `b5660dff`, the measurement that closes the pinned-tree
  claim.

## R-05: Placement still travels through the existing slot

- **Decision**: when a plan stands, the runner builds
  `ContextPolicy {SPIN, PlacementSlot::cpu(plan.cpu[i])}`; with no plan
  the policy keeps `noPin()` as it does today.
- **Rationale**: D-1 and FR-008. The pin, the applied flag, the
  `unapplied` account and the noise warning of feature 020 all keep
  working untouched, and no public signature moves (FR-030).
- **Alternatives considered**: a new placement call that takes a cpu set.
  Rejected: it would move the exported `PlacementFn` layout and force a
  `SOVERSION` move for no user gain.

## R-06: The default placement function composes `cpu@group`

- **Decision**: `pinToPlacementSlot` returns the description
  `cpu@group`, for example `24@0`, and keeps `NO_PIN` for a slot that
  names no cpu. The group ordinal comes from the layout.
- **Rationale**: the description is the placement function's own account,
  and the disclosure printer adds nothing to it, which is the separation
  feature 020 states. One field keeps the disclosure line's field count,
  so the report-shape checks and the README region move by one token and
  nothing else. `kPlacementFieldWidth` is `digits10 + 1` of
  `std::uint32_t`, ten characters, which holds the pair for every cpu
  index a real host carries.
- **Alternatives considered**: a second per-context field for the group,
  and a `L3:2/24` spelling. Rejected: the first changes the field count
  of a line three contracts already fix, and the second repeats the group
  word inside every context of every row.

## R-07: Names

- **Decision**: handle option `placement(std::string_view)`, command line
  `--placement=<name>`, listing flag `--topology`.
- **Rationale**: `--min-time`, `--warmup-time` and `--counter` set the
  hyphenated command-line style; `--list` and `--catalog` set the listing
  style; the handle method matches the camelCase of `threads` and
  `threadRange`. The preset names stay the five words of the spec.
- **Alternatives considered**: `place(...)` and `--pin=...`. Rejected:
  `pin` names the mechanism a user does not care about, and Google
  Benchmark's historical `--pin` was a boolean about migration, which
  invites a false equivalence.

## R-08: The fit check runs once, before the batch

- **Decision**: `speedgunMain` walks the selected instances after
  expansion and filtering, asks the resolver for each instance that
  carries a preset other than `none` and whose context count stands above
  1, and stops with one line and status 2 on the first miss. An instance
  with no preset, or with `none`, skips the check.
- **Rationale**: FR-018 and Clarifications Q-4. A family with
  `threadRange` can carry a count the machine cannot fill; stopping
  before the first run keeps the printed report a complete account of the
  batch that ran.
- **Alternatives considered**: skip the unfittable instance and continue,
  and run what fits then exit. Rejected: a report with a silently missing
  count reads like a complete sweep, and a partial batch that exits
  nonzero leaves the reader to reconstruct what ran.

## R-09: The topology line sits in the context block

- **Decision**: `printContext(bool placementInEffect)` gains the topology line,
  printed once per report when any selected instance runs with a preset other
  than `none` over a run of more than one context; the flag is the carrier
  because the selected set is local to `speedgunMain`.
- **Rationale**: the context block already carries the version and build
  type above the rows, and the layout is a property of the process, and
  of the report, and of no single row. One line keeps a 40-benchmark run
  readable.
- **Alternatives considered**: one line per result. Rejected: the counts
  cannot differ between results of one process, so repetition adds noise
  and 40 chances for the report-shape gate to drift.

## R-10: The docs example carries a fixed, always-fittable preset

- **Decision**: `example/docs/topology_placement.cpp` registers
  `bmCompact` with `threads(2)` and the handle preset `compact`, the
  widest of the five whose place count equals the thread count of the
  mask, and `bmPair` with `threads(2)` and no handle preset so the
  command line can show a shape.
- **Rationale**: FR-029. `compact` fills two contexts on any host that
  can run a two-context benchmark at all, so no runner needs a machine of
  a fixed shape, and the handle preset makes a bare run place contexts.
- **Alternatives considered**: reading the layout in the example and
  picking the richest preset the host can fill. Rejected: the layout
  lives behind `source/harness/detail/internal.hpp`, so a docs example
  that teaches the public surface cannot reach it. A fixed preset with a
  guard that skips the example. Rejected: a skipped example proves
  nothing to the drift gate.

## R-11: Four test executables

- **Decision**: `harness_topology_test`, `harness_placement_preset_test`,
  `harness_placement_report_test`, `harness_cli_placement_test`, each
  added to `test/CMakeLists.txt` with the standing
  `add_executable`/`add_test` pair.
- **Rationale**: the repository's test layout is one executable per area,
  and the four areas differ in what they link and inject: the resolver
  test needs no harness entry point at all, the CLI test needs the option
  table, and the report test needs the capture helper the existing
  harness report tests use.
- **Alternatives considered**: one combined executable. Rejected: the
  coverage and sanitizer runs would lose the ability to run the pure
  resolver tests without the entry point, and the failure surface would
  merge.

## R-12: The hwloc proofs gain the harness archive as a second subject

- **Decision**: `tools/hwloc/hwloc_nm_proof.sh` keeps its three assertions
  on `libspeedgun-ng.a` and gains the harness archive as a second subject
  with its own assertions: the harness archive holds undefined
  `sg_hwloc_*` references and zero defined ones, and an executable that
  links both archives resolves them with the vendored archive on its link
  line. The purity scan keeps its three checks: build-file discovery
  calls, zero hwloc references under `include/`, and the root call-site
  prohibitions. That scan set never opens `source/`, so the single-unit
  rule of FR-004 stays a rule the reviewer and the include check enforce.
- **Rationale**: the privacy contract of feature 003 is a gate, and this
  feature is the first to put real hwloc calls outside the gate unit.
- **Alternatives considered**: leave the proofs alone. Rejected: the
  proofs would then describe a build that no longer matches what they
  check.

## R-13: One host read, everything else injected

- **Decision**: `harness_topology_test` performs one real read on the CI
  machine and asserts the package and thread counts stand above zero while
  the core and group counts stand at zero or above, that every thread of the
  layout sits inside the process mask, and that groups, cores and threads are
  ordered by ascending cpu index. Every other test installs a
  layout. The read path's own branches run through the installed topology
  description of R-04: a synthetic machine with no L3 and no cores, a
  one-cpu machine, and a description the library rejects.
- **Rationale**: FR-005 and SC-04 together: the hwloc path must be
  covered for the coverage rule, and no placement rule may depend on the
  host shape. The host test asserts invariants that hold on any Linux
  host, so it cannot fail on a runner with one cache group.
- **Alternatives considered**: skip the host read and exclude the hwloc
  walk from coverage. Rejected: the constitution forbids the exclusion
  marker.

## Open items resolved by this phase

| Spec item | Resolution |
| --- | --- |
| Option spellings (Open questions) | R-07 |
| The unit that owns the hwloc include | R-01, Q-1 |
| The disclosure rendering of the group | R-06, Q-2 |
| The listing surface for the layout | R-07, FR-036 |
| The sweep fit rule | R-08, Q-4 |
| The injected-layout seam | R-04, Q-5 |
