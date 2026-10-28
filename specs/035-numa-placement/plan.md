# Implementation Plan: NUMA Memory-Node Placement

**Branch**: `035-numa-placement` | **Date**: 2026-10-28 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `/specs/035-numa-placement/spec.md`

## Summary

The topology placement grows the item N12 NUMA memory-node placement.
The machine read already walks one hwloc topology one time per run and
maps every cpu to its package, L3 group and core
(`source/harness/topology.cpp`); this feature extends the same read to
the NUMA node kind, adds one preset name, `node`, that plans the
contexts of a threaded run one per memory node inside the process
mask, and binds each planned context to that node's memory at context
start, on the context thread, beside the cpu pin
(`source/harness/execution_context.cpp:434`). The binding stands
behind one detail seam the tests replace, applies through the hwloc
call set the vendored `sg_hwloc` archive carries (pinned `2.14.0`,
`source/hwloc/hwloc_gate.cpp:13`), and refuses one line with exit
status 2 where the system denies it. The `--topology` listing gains
one line per node under the L3 lines, in the L3 lines' shape, truthful
to the read. The context's placement field carries the node beside the
group (`cpu@group@node`) with no new key, so the schema minor stays
`1.6`. The public surface grows by nothing: the preset names ride the
standing string grammar, and the harness `SOVERSION` stays `13`, the
archive `SOVERSION` stays `4`. The lane records the placement runs and
the merged pre-check run on thrawn for every NPS BIOS setting the
board's menu offers.

## Technical Context

**Language/Version**: C++23 (`cxx_std_23` on every target; the
constitution pins the language)

**Primary Dependencies**: the standing placement units
(`source/harness/topology.cpp`, `source/harness/placement_preset.cpp`,
`source/harness/execution_context.cpp`) and their seams: the layout
seam (`installTestLayout`, `topology.cpp:306`), the online-set seam
(`installTestOnlineCpus`, `topology.cpp:322`), and the placement
function seam (`setPlacementFunction`,
`include/speedgun-ng/execution_context.hpp:202`); the vendored hwloc
`2.14.0` (`external/hwloc`, the `sg_hwloc` archive, the pin assertion
at `source/hwloc/hwloc_gate.cpp:13`, the covering check
`tools/hwloc/hwloc_nm_proof.sh`); the detail layout types
(`source/harness/detail/internal.hpp:94`, `MachineLayout`,
`LayoutGroup`, the `Preset` enum at `:106`). No new dependency joins:
hwloc is already vendored, and libnuma stays out (D-1 of the spec).

**Storage**: N/A: no file and no database. The node read lives in the
process-lifetime layout; the one document the feature writes is the
report file `--json` names, and the NPS records stand under
`specs/035-numa-placement/results/`.

**Testing**: the repository's own harness (`speedgun-ng_test`
pattern: one executable per area plus `add_test`), one new test
executable `harness_numa_placement_test`, the preset grammar tests
extended in `harness_cli_placement_test.cpp`, the topology listing
tests extended beside them; synthetic topologies of one, two and four
nodes installed through `installTestLayout`; the binding seam replaced
by a fake, so no test binds real memory and no test sleeps or reads a
clock; tests run with `ctest -j 2` and pass under `taskset -c 0,1`
and on a 2-cpu single-node host.

**Target Platform**: Linux only (`ci-ubuntu`, `ci-rocky`). The daily
loop drives `ci-rocky`; once per feature, `ci-ubuntu` builds with
clang-tidy and cppcheck wired in. The acceptance shape stands on the
request's lane host (thrawn, the NPS BIOS setting); every test passes
on a 2-cpu single-node host (FR-017).

**Project Type**: library feature: one preset name added to the
standing grammar, one detail seam added, the machine read and the
context start extended. No new target, no new option row, no public
type added.

**Performance Goals**: the binding stands outside every timed window:
one hwloc bind call per context start, beside the pin call the
context start already makes, and no timed path reads the node data.
No benchmark baseline is claimed, so none is promised.

**Constraints**: 100% line and branch coverage with no `LCOV_EXCL`
marker and no new `--ignore-errors` entry (the D-14 allowance stands
recorded beside the gate); no sleep and no clock in a test; one line
and exit status 2 for every refusal the spec names; no fixed cpu or
node number anywhere; no lambda inside an `SG_*` macro;
`[[maybe_unused]]` on contract-only helpers; simdjson only for JSON;
the hwloc include stays in its one translation unit (spec 025 FR-004,
R-01), so the binding call stands there or behind a narrow wrapper in
the same unit.

**Scale/Scope**: the machine read extended (the node kind beside the
package, L3 and core walks of `topology.cpp:175`), the preset table
grown by one row (`placement_preset.cpp:46`), the plan resolver
extended, the context start extended (the bind beside the pin at
`execution_context.cpp:434`), the listing extended, the report
disclosure extended (`placementText`, `placement_preset.cpp:477`,
beside the standing width cut of `report.cpp`), one detail
seam added, one new test executable, one example added with its
`.expected` file, the NPS records, and the docs rows.

**Execution mode**: TDD (Principle III). Every test task is written
and observed failing before the code task that turns it green exists;
the Test Plan records the waves and the gates each runs.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1
design.*

| Principle | Gate | Status |
| --- | --- | --- |
| I. Standard-first | C++23 surface. The binding rides the hwloc call set the project already vendors and pins; the node walk extends the standing hwloc scan of `topology.cpp:175`; the plan resolver extends the standing plan struct; no compiler extension joins | PASS |
| II. Design by contract | The preset row carries the standing grammar contract; the bind seam carries `\pre`/`\post`; the resolver, the refusal rules and the listing carry `SG_REQUIRE`/`SG_ENSURE` at their seams | PASS |
| III. R-DCUT | Requirements FR-001 to FR-022 map to the decisions R-01 to R-12 below and to `tasks.md` (landing with `/speckit.tasks`). Execution mode: TDD, recorded above and in the Test Plan | PASS |
| IV. Documentation | README node-plan section of at most 25 lines with its quick-reference rows (IV.1), `docs/pages/harness.md` option and field detail, one runnable example with its `.expected` file (FR-020 to FR-022) | PASS |
| V. Style | clang-format, clang-tidy, cppcheck clean at the feature head; every new name follows V.1: PascalCase types, camelBack functions, `k`-prefixed constants, lower_case file names | PASS |
| VI. Coverage | 100% line and branch on new paths, no exclusion marker; every branch of the node walk, the preset parse, the plan resolver, the fill and cycle rules, the binding seam, the refusal rules, the `not available` paths, and the disclosure runs under synthetic layouts and the fake binder (FR-016 to FR-018) | PASS |
| VII. Performance | One bind call per context start, outside every timed window; no timed path reads the node data; the listing reads the layout the run already read; no benchmark baseline is claimed, so none is promised | PASS |
| VIII. CI gates | Every hard gate runs at the feature head: format-check, spell-check, tidy, cppcheck, sanitizers, tsan, coverage, the docs gates (drift, snippet compile, example run, report shape), prose, DBC, time-source, name-check | PASS |
| IX. Spec-driven | Spec, plan and research stand in `specs/035-numa-placement/`; `tasks.md` lands with `/speckit.tasks` before code | PASS |
| X. Anti-slop | One seam for the binding (R-06), one node walk beside the standing hwloc scan (R-02), one preset row reusing the standing parse and fit check (R-03); no per-node argument grammar, no fallback binder, no second topology reader; the public surface grows by nothing | PASS |
| XI. Prose | The gate walks the tracked tree; these artifacts land with the branch's commits and the gate clears them at the head, the way specs/034 recorded | PASS at the head |

**Gate weakening requested**: no. **Gate strengthening requested**:
no. No budget figure moves: the README section lands inside the
standing 25-line section cap, and `PAGE_LINE_CAP` moves only if the
measured re-record asks, the way the specs/023, specs/025, specs/027
and specs/034 moves preceded.

No gate fails, so **Complexity Tracking stays empty**.

## Project Structure

### Documentation (this feature)

```text
specs/035-numa-placement/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output (/speckit.plan command)
├── data-model.md        # Phase 1 output (/speckit.plan command)
├── quickstart.md        # Phase 1 output (/speckit.plan command)
├── contracts/           # Phase 1 output (/speckit.plan command)
│   ├── cli-placement.md     # The preset grammar, the listing, the refusals
│   ├── console-report.md    # The placement disclosure of a bound context
│   ├── report-json.md       # The context JSON field, no new key
│   └── report-shape.md      # The shape fold rule for the node tail
└── tasks.md             # Phase 2 output (/speckit.tasks command - NOT created by /speckit.plan)
```

### Source (this feature)

```text
source/harness/
├── topology.cpp                 # The node walk beside the standing scan
├── placement_preset.cpp         # The `node` row of kPresetNames
├── execution_context.cpp        # The bind beside the pin at context start
├── placement_preset.cpp         # placementText carries the node tail
├── report.cpp                   # the field prints beside the width cut
└── detail/
    ├── internal.hpp             # LayoutNode beside LayoutGroup, Preset::NODE
    └── memory_bind.hpp          # The bind seam (new)

include/speedgun-ng/execution_context.hpp   # Unchanged (D-6)
include/speedgun-ng/benchmark.hpp           # Unchanged (D-6)

test/source/
├── harness_numa_placement_test.cpp  # New executable
└── harness_cli_placement_test.cpp   # Grammar and listing rows extended

example/docs/numa_placement.cpp + .expected   # The recorded node-planned run
tools/docs/example_drift.py         # SECTIONS/SECTION_OWNERS row for the example
```

**Structure Decision**: the feature stands inside the harness library
the way spec 025 built it: the machine read stays in its one hwloc
translation unit, the preset grammar stays in the preset unit, and the
binding stands at the context start beside the pin call. The one new
detail header carries the bind seam, and the one new test executable
owns the synthetic-topology scenarios. No public header moves, so the
exported-layout check of the harness `SOVERSION` stays untouched.

## Design

### Decision index

- **R-01 (FR-005, D-1)**: the node walk extends the standing hwloc
  scan. `topology.cpp:175` walks `HWLOC_OBJ_CORE`, `HWLOC_OBJ_L3CACHE`
  and `HWLOC_OBJ_PACKAGE` today; the walk adds
  `HWLOC_OBJ_NUMANODE`, reads each node's cpuset, and stores one
  `LayoutNode` beside the standing `LayoutGroup` vector, inside the
  one-time read the run performs.
- **R-02 (FR-006, D-3)**: the listing prints the node lines under the
  L3 lines in the L3 lines' shape, one line per node the mask
  touches. The line composes from the layout the run already read,
  with the node number and the cpus the read states; the mask
  filters the cpus the way the L3 lines filter theirs, and a node
  the mask excludes prints no line. Nothing renames or regroups.
- **R-03 (FR-001, D-2)**: the preset table grows by one row,
  `{"node", Preset::NODE}`, parsed in full and nothing else beside the
  five standing rows of `placement_preset.cpp:46`. The resolver gains
  the node plan: the layout's nodes inside the process mask, in the
  read's node order, one cpu per context, cycling the nodes; the
  plan's `places` stands at the distinct-cpu count of those nodes, so
  the standing fit check passes unchanged and the unfittable refusal
  stands reachable at cpu exhaustion; a count below the node count
  fills the nodes in node order.
- **R-04 (FR-009)**: the plan resolver's postcondition predicate
  extends: every planned cpu stands inside the mask and stands on the
  node the context binds to, beside the standing
  `planCpusDistinct` check (`placement_preset.cpp:56`).
- **R-05 (FR-010, D-4)**: the bind stands at the context start beside
  the pin. The context start calls the placement function at
  `execution_context.cpp:434`; the bind call stands after it, driven
  by the plan's node assignment for that context, through the seam of
  R-06. The bind rides outside the replaceable placement function, so
  a user-installed `PlacementFn` cannot bypass it and the public
  contract stands unchanged (D-6).
- **R-06 (FR-016, D-1)**: one detail seam, `installTestMemoryBinder`,
  carries the binding. The production binder calls the hwloc membind
  entry points on the calling thread with the node's bitmap
  (`HWLOC_MEMBIND_BIND` by node, thread scope) inside the one hwloc
  translation unit; the tests install a fake that records the node
  the call names and can deny the call. A null installer clears, the
  way `installTestLayout` clears (`topology.cpp:306`).
- **R-07 (FR-013, D-4)**: a denied bind is a usage error, and the
  refusal stands ahead of every context: the run validates the
  binding before the contexts spawn, the main thread binding and
  restoring each planned context's node in plan order, one pass over
  the plan; a denial in that pass prints one line naming the context
  and the node and exits 2 before any context starts. A bind that
  fails at a context start despite the validation turns beside the
  standing pin-failure account path (`execution_context.cpp:440`)
  into the same one-line status-2 refusal.
- **R-08 (FR-011, D-5)**: `placementText`
  (`source/harness/placement_preset.cpp:477`, the unit that composes
  the field beside the layout) gains the node tail. The field reads
  `cpu@group@node` where the run's resolved plan binds the context,
  and the standing `cpu@group` bytes where it does not: the compose
  consults the layout's cpu-to-node map beside the run's resolved
  preset state in the same unit, so a run with no node preset keeps
  the standing bytes and no slot change joins the public struct; the
  standing width cut of the field in
  `source/harness/report.cpp` stands unchanged for the longer
  bytes.
- **R-09 (FR-007, D-3)**: a read that states no memory information
  prints `not available` for the node lines alone. The layout carries
  the state beside `readable`, and the listing branch keeps the rest
  of the bytes.
- **R-10 (FR-014)**: a node preset over a read that states no memory
  information refuses one line, exit status 2, before any context
  starts, beside the standing unfittable-count refusal of
  `placement_preset.cpp:498`.
- **R-11 (FR-019, D-7)**: the NPS record set covers every state the
  board's menu offers, one record per state, each naming the setting,
  the boot, the commands, the exit codes and the listing, with the
  pre-check run beside the placement runs; an unreachable state stands
  recorded as out of reach with the gap named.
- **R-12 (FR-020 to FR-022)**: the README section stands inside the
  25-line cap, the quick-reference rows join the standing placement
  rows, the guide page carries the option and field detail, and the
  example records a node-planned run whose bytes the shape gate folds.

### Risks

- **The hwloc node read on a one-node host**: the walk states one
  node; the plan binds every context to it and the disclosure names
  it. The synthetic one-node layouts pin the shape in tests.
- **The bind on a host whose kernel refuses the policy**: the seam
  reports the denial and R-07 refuses the run; no silent fallback
  binder exists to test around.
- **The disclosure field grows by one token on a bound context**: the
  report-shape fold of `tools/docs/report_shape.py` folds digit runs,
  so the bound and unbound shapes fold differently by one `@#` group;
  the example's recorded run carries the bound shape, and the fold
  rule for the field stands recorded in
  `contracts/report-shape.md` beside the 034 folds.
- **The NPS runs need reboots**: the evidence plan precedes any code
  task that depends on it, and an unreachable state stands recorded
  as out of reach, with the gap named (D-7).

### Logical view: the node plan

```text
layout (one hwloc read)
  └── nodes[n] { number, cpuset }     # beside groups, cores, threads
preset node
  └── resolve(count, mask)
        ├── nodes the mask touches, in the read's node order
        ├── count > n : cycle the nodes
        └── count < n : fill nodes 0..count-1
context start
  ├── placementSlot()(...)             # the standing pin, unchanged
  └── binder(node of the planned cpu)  # the seam, outside the slot
```

### Logical view: one bind call

```text
binder(node)
  ├── production: hwloc membind by node, thread scope, one call
  └── test fake: records the node, or denies on demand
refusal path
  └── denied on a planned context : one line, status 2, no benchmark
```

### Physical view

The node walk, the plan resolver and the binder stand in the harness
archive the context start already links. The hwloc membind call stands
inside `topology.cpp`'s translation unit (the one unit that includes
`<hwloc.h>`, spec 025 FR-004), and the seam's contract lives in the
new detail header, so no second hwloc include joins.

### Public API surface added

None. The preset names are a string grammar the standing public
handle carries, the seam is a detail seam, and the `PlacementFn`
contract stands unchanged (D-6). The report JSON gains no key.

### Version and `SOVERSION`

The version moves to `0.22.0` (FR-003). The harness `SOVERSION`
stays `13` and the archive `SOVERSION` stays `4`: no public type,
function or struct changes, and the check stands recorded in the
commit that moves the version. The report schema stays at minor
`1.6` (FR-004): the context `placement` field is a string and its
value grows the node tail with no key change.

## Test Plan

TDD, the waves in dependency order; every task's test is observed
failing before its code lands.

- **Wave 1 (the node walk and the listing)**: synthetic layouts of
  one, two and four nodes through `installTestLayout`; the walk reads
  the nodes beside the groups; the listing prints the node lines in
  shape and order, the mask filter, and the `not available` path.
  Gate: the new executable plus the extended placement CLI tests.
- **Wave 2 (the preset and the plan)**: the `node` row parses in full
  and nothing else, gated on the placement CLI tests; the resolver's
  cycle, fill, mask-touch and exhaustion rules and the
  distinct-and-on-node postcondition, gated on the new executable.
  The refusal scenarios stand in wave 3 beside T015 and T016.
- **Wave 3 (the bind and the refusal)**: the fake binder records the
  node the call names and denies on demand; the eager validation pass
  binds and restores each planned context's node in plan order
  before the contexts spawn, and a denial in that pass refuses one
  line, status 2, ahead of every context; the bind stands at the context start beside
  the pin; a bind that fails at a start stands the same refusal; a
  user-installed `PlacementFn` cannot bypass the bind. Gate: the new
  executable.
- **Wave 4 (the disclosure)**: `placementText` composes
  `cpu@group@node` for a bound context and the standing bytes for the
  rest; the console row and the JSON field carry it; a run with no
  node preset stands byte-identical apart from the version figures
  the report carries.
  Gate: the report tests and the json round-trip suite.
- **Wave 5 (the gates)**: the example records its `.expected`; the
  drift, snippet-compile, example-run and report-shape gates run;
  format, spell, tidy, cppcheck, sanitizers, tsan, coverage and prose
  run at the head; the suite passes under `taskset -c 0,1` and on a
  2-cpu single-node host. Gate: the full `ctest` matrix, then the NPS
  evidence runs on thrawn (R-11).

## Constitution Check

*Re-check after Phase 1 design.*

| Principle | Status |
| --- | --- |
| I. Standard-first | PASS: the design names the standing hwloc call set and the standing plan types; no new dependency, no compiler extension |
| II. Design by contract | PASS: the seam and the resolver carry `\pre`/`\post` and the `SG_*` checks at their edges |
| III. R-DCUT | PASS: FR-001 to FR-022 map to R-01 to R-12 and to the Test Plan waves; TDD stands |
| IV. Documentation | PASS: the docs artifacts stand in the FR set and the example carries its `.expected` |
| V. Style | PASS: the new names follow V.1; the hwloc include stays in its one unit |
| VI. Coverage | PASS: every new branch runs under synthetic layouts and the fake binder; the one-node, two-node and four-node layouts, the cycle and fill rules, the denial and the `not available` path all stand reachable |
| VII. Performance | PASS: one bind per context start, outside the timed windows; no timed path reads the node data |
| VIII. CI gates | PASS: the wave 5 list runs every hard gate at the head |
| IX. Spec-driven | PASS: the spec, the plan, the research, the data model, the contracts and the quickstart stand in the feature directory |
| X. Anti-slop | PASS: one seam, one walk extension, one preset row; no fallback, no argument grammar, no second reader |
| XI. Prose | PASS at the head: the gate clears the artifacts with the branch's commits |

**Complexity Tracking stays empty**: no gate fails and no violation
needs justification.

