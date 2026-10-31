# Feature Specification: Topology Placement

**Feature Branch**: `022-topology-placement`

**Created**: 2026-10-19

**Status**: Draft

**Input**: User description, opening quoted, the full text held by the
specify invocation record:

"Topology placement, item N4 (old H10) of the roadmap
`/workspace/speedgun-ng-harness-roadmap.md`. Short name:
topology-placement. Target machine: thrawn (AMD Threadripper 9960X, 24
cores and 48 threads, one socket, several CCDs with one L3 each). Lane:
placement (N4, then N12, N9, N11). It runs in parallel with N1 on gary
and N2 on corsairone. Goal: the harness reads the core layout of the
machine with hwloc, records it in the report, and places execution
contexts by a preset name. For example, a user asks for 'one context for
each L3 group' and does not write cpu numbers. NUMA memory nodes are out
of scope. N4 places contexts on one NUMA node only. NUMA node discovery,
memory-node presets, and the tests with the NPS BIOS setting belong to
the later feature N12 'NUMA memory-node placement (NPS)'. No machine in
the fleet has two sockets, so N4 tests placement by L3/CCD groups, cores
and SMT siblings. [...] Capabilities C-1 to C-5. Recommendations
accepted: the placement slot stays option (a); hwloc read one time per
process; a fake topology for the tests; lowest cpu index first; a preset
with more contexts than places exits with status 2; NUMA stays out."

## Clarifications

### Session 2026-10-19

- **Q-1 Archive that owns the read**: Clarified 2026-10-19: the harness
  archive. One harness translation unit includes `<hwloc.h>`, which
  specs/003-vendor-hwloc FR-021 allows for a `source/` unit, and
  `speedgun-ng_harness` takes the vendored include and link `PRIVATE`,
  still unexported. Recorded at FR-004 and the Constraints section.
- **Q-2 Group inside the placement field**: Clarified 2026-10-19:
  `cpu@group`, for example `24@0`, with `NO_PIN` printed as feature 020
  prints it today. Recorded at FR-021.
- **Q-3 Reading the layout with no run**: Clarified 2026-10-19: one
  listing flag `--topology` prints the layout and exits with status 0,
  the pattern of the standing list and catalog modes. Recorded at FR-036
  and SC-12.
- **Q-4 A sweep count above the fit limit**: Clarified 2026-10-19: the
  process exits with status 2 and one line before any run of the batch
  starts. Recorded at FR-018.
- **Q-5 Shape of the injected seam**: Clarified 2026-10-19:
  harness-internal install calls a test uses before its first run, one
  holding a plain machine description and one holding the description text
  the read consumes; no public header, no environment variable, and no
  file carries it. Recorded at FR-006.
- **Q-6 A preset run of one context over an unreadable layout**: Clarified
  2026-10-20: it prints one line and exits with status 2 whatever the context
  count. FR-005 names the layout failure ahead of any count, so FR-019's rule
  that a run of one context keeps feature 020's behavior presumes a readable
  layout. The candidate that let the context count gate decide first was
  rejected, because it would run a preset-bearing benchmark on a machine the
  harness cannot describe. Recorded at FR-005, T020 and T042.
- **Q-7 A one-context run under a preset that places nothing**: Clarified
  2026-10-20: it keeps feature 020's behavior. The fit check of FR-018 runs
  only where a plan could exist, so a preset whose place count stands at zero
  over a run of one context takes FR-019's path. The candidate that applied
  FR-018 and stopped the run with status 2 was rejected, because no context
  exists to place there and the refusal would name a plan the run never
  builds. Recorded at FR-019, the contract row and T020.
- **Q-8 A failing `hwloc_topology_init`**: Clarified 2026-10-20: that answer
  is a contract violation and the FR-005 degradation path leaves it out. The
  init answer rides a one-line `SG_REQUIRE`, and a failing predicate under
  the enforce
  semantic of `cmake/dbc.cmake:24` aborts through the `[[noreturn]]` dispatch
  of `include/speedgun-ng/dbc.hpp:198-214`. The candidate that folded an init
  failure into FR-005's one line and status 2 was rejected: no scenario can
  make init fail, so that branch would be undrivable against the branch gate,
  and a host that cannot allocate the topology object has no layout to report
  either way. FR-005's degradation covers a description the library rejects
  and a failed load, both of which a scenario drives. Recorded at FR-005,
  T010, the row II entry of plan.md and its coverage note.

## Decisions

The owner accepted these five on 2026-10-19 as recommendations for this
prompt. They are governance for this feature; each lands in the
requirement named beside it.

- **D-1 The placement slot stays option (a).** Each preset gives one cpu
  to each context, and the harness puts that cpu into the existing
  `PlacementSlot`. `PlacementFn` and `setPlacementFunction` keep their
  signatures, so the public API does not move (FR-008, FR-030).
- **D-2 hwloc is the topology source**, read one time per process before
  the first context of the first run that needs it. No harness code
  reads a sysfs file for the layout (FR-002, FR-004).
- **D-3 The tests inject the machine.** A test supplies the layout the
  harness reads, so no test depends on the host shape, needs a
  privilege, or performs a real pin. The installed `PlacementFn`
  replacement of feature 020 records the calls (FR-006, SC-04).
- **D-4 Order is deterministic.** Inside a group the lowest cpu index
  comes first, so one preset on one mask yields one answer (FR-009).
- **D-5 A preset that cannot fill exits.** A preset with more contexts
  than places exits with status 2; no wrap-around and no partial run
  (FR-018).

## Audit point

| Field | Value |
| --- | --- |
| Audit point | `ab32400b86321f98f13b9c66d4488e15831b47f6` |
| Audit-point date | 2026-10-19 11:40:39 -0500 (commit date; the author date is 2026-10-19 09:56:04 -0500) |
| Default branch at run time | `master`, tip `ab32400`, equal to the `origin/master` tip; the rows below were re-read at the rebase point |
| Working tree at run time | clean at that commit; the vendored submodules stand uninitialized on this machine, so hwloc's own sources were unreadable here: the structural hwloc claims below come from `CMakeLists.txt` and `specs/003-vendor-hwloc` (Assumptions), the tripwire claim from `source/hwloc/hwloc_gate.cpp`, and the one behavioral measurement from a run against the system hwloc 2.15.0 recorded in Assumptions |
| Feature number at run time | 021, the next free number under `specs/`, assigned by this command on this machine; renumbered to 022 at the rebase row below, the percentiles lane having merged as 021 first |
| Short name at run time | `topology-placement` |
| Git branch at run time | none created: `.specify/extensions.yml` is absent, so this command created the directory on `master` and the orchestrator owns every commit |
| Rebase point for this ledger | `c7f1769`, the tip after the percentiles lane merged as 021 |
| Project version at the audit point | 0.10.0 at `ab32400`; 0.11.0 at the rebase point (the `project()` VERSION declaration in `CMakeLists.txt`) |
| Harness `SOVERSION` at the audit point | 6 at `ab32400`; 7 at the rebase point (the harness target `SOVERSION` in `CMakeLists.txt`) |
| Archive `SOVERSION` at the audit point | 2 (the `speedgun-ng` target `SOVERSION` in `CMakeLists.txt`) |
| Package compatibility at the audit point | `SameMinorVersion` (the `COMPATIBILITY` clause in `cmake/install-rules.cmake`) |
| Main page at the audit point | `README.md` at 638 lines against the drift gate's page cap at `ab32400`; 659 lines against 659 at the rebase point, equal again (PC-5) |
| Constitution version at the audit point | 2.20.0, lineage row and footer equal |

Every citation below was read at that content, and re-read at the rebase
point wherever a row above records a different value there.

**Numbering across the three parallel features.** N1 and N2 resolve
their own numbers on their own machines at their own run time, and each
of them can read 021 as next free. The feature that merges first keeps
021; a later one renumbers its directory and branch to the next free
number at its rebase, and the `Refs:` footer follows the number it
merges with. The percentiles lane merged first as
`021-percentiles-raw-capture` at `c7f1769`, so this feature is the later
one: its directory, branch and footer carry 022.

## Preconditions

Each precondition was checked in the code and the cited artifacts at the
audit point and re-checked at the rebase point. The result and its evidence
follow.

### PC-1: the H5 placement seam stands, and it names this feature. PASS.

- `include/speedgun-ng/execution_context.hpp` at the audit point carries
  `PlacementSlot` (one `cpuIndex`, `kNoPin`, `noPin()`, `cpu()`),
  `ContextPolicy` (`waitMode`, `placement`), `PlacementResult`
  (`applied`, `description`), `PlacementFn` taking the slot and the
  native thread handle, `setPlacementFunction`, and
  `detail::placementRecordOf`.
- The header comment on `PlacementFn` states that this signature is the
  seam H10 replaces with topology placement and that nothing else reads
  it.
- `source/harness/execution_context.cpp` defines the default
  `pinToPlacementSlot`, holds it in the one registry slot, and offers
  `setPlacementFunction`.

### PC-2: hwloc is vendored, private, and called by no feature today. PASS.

- `CMakeLists.txt` ingests `external/hwloc` through
  `import_autotools_submodule`, links it `PRIVATE` into
  `speedgun-ng_speedgun-ng`, and merges it into that archive.
- `source/hwloc/hwloc_gate.cpp` is the single translation unit that
  includes `<hwloc.h>`; it carries the version tripwire for hwloc 2.14.0
  and one link-proof pointer, and zero runtime lines.
- A search over `source/`, `test/`, and `example/` finds no hwloc call
  outside that gate unit.
- `specs/003-vendor-hwloc` FR-021: no header under `include/` shall
  include an hwloc header, and hwloc headers shall be includable only
  from `source/` translation units, preferably through one internal
  wrapper unit.

### PC-3: the C-7 disclosure contract stands. PASS.

- `specs/020-execution-contexts-core/contracts/report-disclosure.md`
  states the disclosure line
  `contexts=N ctx0=<MODE>/<PLACEMENT>/<APPLIED> ...`, the cut of
  `<PLACEMENT>` to the width of the longest account the default function
  produces with `~` marking the cut, the noise warning condition, and
  the one host fact (the cpus of the process affinity mask from
  `sched_getaffinity`, or `std::thread::hardware_concurrency` when that
  call fails).
- `README.md`, in the disclosure example, shows the printed pair
  `contexts=2  ctx0=SPIN/NO_PIN  ctx1=SPIN/NO_PIN`.

### PC-4: the `threads(n)` path owns the contexts and their policy. PASS.

- `source/harness/runner.cpp`, the threaded instance path, creates
  contexts only when `instance.threadCount > 1`, each under
  `ContextPolicy {SPIN, PlacementSlot::noPin()}`.
- The disclosure fill beside it reads `detail::placementRecordOf` per
  context, and the vector stays empty for a single-context run.

### PC-5: the docs budget stands at its cap. PASS.

- `README.md` measures 659 lines at the rebase point and 638 at the audit
  point.
- `tools/docs/example_drift.py` sets `PAGE_LINE_CAP = 659` there and 638 at
  the audit point,
  `SECTION_LINE_CAP = 25`, and
  `SECTION_LINE_CAPS = {"Registration": 38, "Fixtures": 43}`.

### PC-6: the vendored hwloc build carries no XML input. PASS.

- `CMakeLists.txt`, at the hwloc configure step, passes
  `--enable-embedded-mode` together with `--disable-libxml2` to the build,
  so the harness build has no XML topology reader. The test seam of D-3 and FR-006 therefore
  stands on a machine description the harness holds itself, and a
  synthetic XML file is not a route this feature can take without
  re-configuring the vendored dependency.

### PC-7: thrawn's layout is measured, and it fits the presets. PASS.

- Read on thrawn at run time from sysfs: four L3 groups with
  `shared_cpu_list` `0-5,24-29`, `6-11,30-35`, `12-17,36-41`, and
  `18-23,42-47`; `/sys/devices/system/cpu/online` is `0-47`;
  `/sys/devices/system/node` holds `node0` alone.
- The fit limits that follow for a full process mask: `l3` fills at most
  4 contexts, `smt` at most 2, `core` at most 24, `compact` at most 48.

## Roadmap position

This is N4 (old H10) of the speedgun harness roadmap. 020 (H5) gave the
harness the execution context, its policy, and the placement slot that
holds one cpu index with no pin. N4 gives the harness the ability to
name where the contexts of a threaded run execute: it reads the layout
of the machine once, turns a preset name plus a context count into one
cpu per context, and discloses what it did.

The lane on thrawn runs N4, then N12 (NUMA memory-node placement with
the NPS BIOS setting), then N9 (context groups), then N11 (execution
graphs). N4 builds no memory-node placement, no context group, no
graph, and no cpu-list input.

Every test of this spec runs on the fake provider and an injected
layout: the feature needs no hardware, no PMU, and no privilege.

## Parallel work: three lanes at one time

N1 (percentiles, gary), N2 (metric formulas, corsairone) and N4 (this
feature, thrawn) start from the same audit point and are orthogonal.

- The shared files are only: the project `VERSION` line in
  `CMakeLists.txt`, the two `SOVERSION` lines, `README.md`, the
  quick-reference table, the option table, `docs/pages/harness.md`,
  `include/speedgun-ng/benchmark.hpp`, `PAGE_LINE_CAP`, `SECTIONS` and
  `SECTION_OWNERS` in `tools/docs/example_drift.py`,
  `example/CMakeLists.txt`, and the `test` target lists.
  `tools/docs/report_shape.py` is shared with the percentiles lane, and this
  feature edits it only if a new line class reaches a compared block, which
  T018 confirms it does not. The `example_run`
  gate needs no edit: `test/example_run.sh` enumerates
  `example/docs/*.cpp`, so a new docs example joins the run on its own.
  No changelog exists at the audit point.
- A change that must touch a file outside this feature stops and records
  the reason in the plan before the edit.
- A conflict resolves by rebase onto the new `master`. Both features
  keep their lines in each shared list, and `PAGE_LINE_CAP` rises again
  by the size of the section this feature added. Then the full gate set
  runs, and the branch pushes once with `--force-with-lease`.
- Merge order: N1 before N4, because both touch the runner and the
  report. N2 and N4 overlap least.

## Reference read: the preset vocabulary

Google Benchmark is a reference read. No vendored copy
enters this feature and the harness links nothing from it.

The naming check the request asks for ran against Google Benchmark and
against the placement vocabulary the owner already uses:

| Read | Result |
| --- | --- |
| `src/flags.cc` at `main` and at `v1.5.0`, `v1.8.5`, `v1.9.1`, `v1.9.4` | no placement, pin, or affinity option exists; the string `pin` appears nowhere in the option table |
| `include/benchmark/benchmark.h` at those revisions | the thread surface `Threads`, `ThreadRange`, `DenseThreadRange`, which feature 020 already mirrors |
| `src/sysinfo.cc` at `main` | `ThreadAffinityGuard`, which pins the timer thread so a CPU-time estimate reads one cpu; a host fact, and no user-facing placement name |
| OpenMP `OMP_PLACES` | the place kinds `cores`, `threads`, `sockets`, `numa` |
| Intel `KMP_AFFINITY` | `compact`, which fills the siblings of a core before the next core, and `scatter`, which takes one thread per core before it returns to fill |
| Slurm `--cpu-bind` | `cores` and `threads` as bind kinds |
| `numactl --physcpubind` | an explicit cpu list, the input shape this feature refuses (FR-017) |
| hwloc object kinds | Package, Core, PU, L3 cache: the objects C-1 counts. The NUMA node kind exists in that model and stays out of this feature (Scope) |

**Final preset list.** Five names, singular to match the harness option
vocabulary (`--min-time`, `--repetitions`, `threads()`), each one word
the tools above already use:

| Preset | The place a context gets | Places a run can fill |
| --- | --- | --- |
| `none` | nothing, the default and today's behavior | unlimited |
| `core` | the lowest-index thread of a distinct core | one per core |
| `l3` | the lowest-index thread of a distinct L3 group | one per L3 group |
| `smt` | a sibling thread of one core, the core with the lowest-index thread | one per sibling of that core |
| `compact` | the next free thread, all siblings of a core before the next core, all cores of an L3 group before the next group | one per thread in the mask |

`scatter` stays out of the list: `core` already gives one context per
core, and a preset with no user story behind it is surface nobody
tests.

Sources for the read: the Google Benchmark user guide
(https://google.github.io/benchmark/user_guide.html), GNU libgomp
`OMP_PLACES`
(https://gcc.gnu.org/onlinedocs/gcc-10.5.0/libgomp/OMP_005fPLACES.html),
the OpenMP 5.0 affinity ICV
(https://www.openmp.org/spec-html/5.0/openmpse53.html), the Intel thread
affinity interface
(https://www.intel.com/content/www/us/en/docs/dpcpp-cpp-compiler/developer-guide-reference/2023-1/thread-affinity-interface.html),
and the HPC wiki binding page (https://hpc-wiki.info/hpc/Binding/Pinning).

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Run one benchmark per L3 group (Priority: P1)

A user measuring a cache-sensitive loop wants each context of a threaded
run sitting on a different L3 cache group. The user names the shape, the
harness reads the machine, and each context gets one cpu of its own
group. No cpu number appears in the source or the command line.

**Why this priority**: this is the whole point of the feature, and the
shape the target machine exists to test: thrawn carries four L3 groups,
so a four-context run gets one per group.

**Independent Test**: run a threaded benchmark with the `l3` preset on a
machine whose layout is injected with four groups, and read the cpu each
context got. Four contexts land on four different groups; a fifth
context stops the run.

**Acceptance Scenarios**:

1. **Given** a layout of four L3 groups and a run of four contexts,
   **When** the user selects the `l3` preset, **Then** each context runs
   on the lowest-index cpu of a distinct group, and the report names
   that cpu and group for each context.
2. **Given** the same layout, **When** the user selects `l3` for a run
   of five contexts, **Then** the process prints one line naming the
   preset, the count it can fill, and the count asked for, and exits
   with status 2 before any context starts.
3. **Given** a process whose affinity mask covers cpus `0-11` only,
   **When** the user selects `l3` for two contexts, **Then** the two
   contexts land on the two groups inside that mask.

---

### User Story 2 - Read where the contexts ran (Priority: P1)

A user who reads a report needs to know what the machine looked like and
where each context ran, beside the wait mode the report already
shows.

**Why this priority**: a placement nobody can see is a measurement the
reader cannot reproduce or trust, and the disclosure already exists as
the place for it.

**Independent Test**: run a threaded benchmark with a preset on an
injected layout and read the topology line plus the per-context
placement fields.

**Acceptance Scenarios**:

1. **Given** a run with a preset in effect, **When** the report prints,
   **Then** one line gives the package, L3 group, core, and thread
   counts of the layout the harness read.
2. **Given** the same run, **When** the disclosure prints for each
   context, **Then** each placement field carries that context's cpu and
   its L3 group, and the `NO_PIN` account keeps the bytes feature 020
   shipped.
3. **Given** a run with no preset, **When** the report prints, **Then**
   the bytes match what the report prints today, with `NO_PIN` in every
   placement field and no topology line.

---

### User Story 3 - Ask for a shape the machine cannot give (Priority: P2)

A user types a preset name the harness does not know, or asks a shape
the mask cannot fill. The run stops at once with one clear line and a
nonzero exit, and no half-run reaches the report.

**Why this priority**: the failure is the common case on a small mask,
and the standing rule for a bad option value is one line and status 2.

**Independent Test**: select each bad input in turn (an unknown name, a
cpu list, an over-subscribed preset) and read the exit status and the
stderr line.

**Acceptance Scenarios**:

1. **Given** any layout, **When** the user selects `l2`, **Then** the
   process prints one line and exits with status 2.
2. **Given** any layout, **When** the user passes `0-3` as the value,
   **Then** the process prints one line and exits with status 2.
3. **Given** a layout with one core and no siblings, **When** the user
   selects `smt` for two contexts, **Then** the process prints one line
   and exits with status 2, and no result row prints.

---

### User Story 4 - Compare the siblings of one core (Priority: P2)

A user who wants the SMT contention cost puts every context of a run on
the hardware threads of a single core, and compares that row against the
same benchmark with one context per core.

**Why this priority**: it is the second shape the request names, it is
cheap once the layout is read, and thrawn's 48 threads make it the
sharpest measurement in the fleet.

**Independent Test**: run the same benchmark with `smt` and with `core`
on an injected layout with two siblings per core, and read the cpus in
the disclosure.

**Acceptance Scenarios**:

1. **Given** a layout with two siblings per core, **When** the user
   selects `smt` for two contexts, **Then** both contexts run on the two
   siblings of the core with the lowest-index thread.
2. **Given** the same layout, **When** the user selects `core` for two
   contexts, **Then** the two contexts run on two different cores.
3. **Given** the same layout, **When** the user selects `compact` for
   three contexts, **Then** two contexts run on the siblings of one core
   and the third runs on the next core.

---

### User Story 5 - Read the page, copy the block, run the example (Priority: P3)

A user reading the main page finds the preset table, copies the block,
builds it, and runs it on any Linux host.

**Why this priority**: the feature is invisible until the page teaches
it, and the docs gate is the standing proof that the page runs.

**Independent Test**: run the docs drift gate and the example run entry
at the feature head.

**Acceptance Scenarios**:

1. **Given** the main page at the feature head, **When** the drift gate
   runs, **Then** the "Topology placement" section matches the snippet of
   its runnable example and stays within its line cap.
2. **Given** the example on a host of any shape, **When** CI runs it,
   **Then** it selects a preset that host can fill and prints a
   placement.

### Edge Cases

- A process mask smaller than the machine: the layout, the counts, and
  the fit limit follow the mask, and a preset that fits the machine but
  exceeds the mask exits with status 2.
- A machine with one L3 group: `l3` fills one context, and a two-context
  run with `l3` exits with status 2.
- A host with SMT disabled: `smt` fills one context, and a two-context
  run with `smt` exits with status 2.
- A single-context run: no context exists to place, so today's behavior
  stands and the placement of a lone runner stays out of scope.
- A handle preset and a command-line preset set at once: the handle wins,
  and both names must be valid.
- A threaded family with several counts (`threadRange`,
  `denseThreadRange`): the whole family is checked before the batch
  starts, and one count above the preset's fit limit stops the process
  with status 2 and one line.
- A placement that the host refuses at run time (a pin call that fails):
  the record keeps the `unapplied` account of feature 020, and the run
  continues.
- A layout the harness cannot read: a run with a preset exits with
  status 2 with one line; a run with no preset behaves as it does today.
- A run that ends before it samples: the disclosure stays empty, as
  feature 020 states, and the topology line follows the rule of FR-020.
- Two contexts of one run on siblings of one core: the noise warning of
  feature 020 keeps its condition, which names the same cpu index, so
  the sibling pair prints no warning.

## Requirements *(mandatory)*

### Topology read (C-1)

- **FR-001**: The harness shall read the layout of the host it runs on:
  the packages, the L3 cache groups, the cores, and the hardware threads,
  with the core and the L3 group of every hardware thread named.
- **FR-002**: The read shall happen at most once per process, on the
  first run that needs it, before that run starts any context, and every
  later run of that process shall reuse the same layout.
- **FR-003**: The layout shall live in a harness-private record. No new
  public type, header, or target shall carry it.
- **FR-004**: The layout shall come from the host's own description of
  itself through the vendored hwloc (Constraints). No harness code shall
  read a sysfs file for it.
- **FR-005**: The layout, the counts, and every fit limit shall cover the
  cpus this process is allowed to run on, so a restricted process sees a
  smaller machine. A run that selects a preset on a host whose layout cannot
  be read, a description the library rejects or a failed load (Q-8), shall
  print one line and exit with status 2 whatever the context count (Q-6), and
  a run that selects no preset shall behave as it does today.
- **FR-006**: The layout the harness reads shall be replaceable by a
  machine description a test supplies, naming the group count, the core
  count, the siblings per core, and the process mask. The seam shall be two
  harness-internal install calls a test makes before its first run: one
  installs the machine description in place of the read, and one hands the
  read a description text whose rejection drives the unreadable branch. An
  install shall take effect before the first read of that process, and a test
  that needs a second read shall take it in a fresh process. A test shall hold that description in code: no public header, no
  environment variable, and no file shall carry it. No test shall depend
  on the shape of the host, need a privilege, or perform a real pin
  (D-3, PC-6, Clarifications Q-5).

### Presets (C-2)

- **FR-007**: The system shall provide five named presets: `none`,
  `core`, `l3`, `smt`, and `compact`, each defined by the table under
  "Reference read: the preset vocabulary".
- **FR-008**: A preset shall give each context of a threaded run exactly
  one cpu, and no cpu shall serve two contexts of one run (D-1).
- **FR-009**: The assignment shall depend on the layout and the context
  index alone: context index `i` takes the `i`-th place of the preset's
  ordered list, and that list shall walk each group it touches in
  ascending cpu index. Two runs of one preset on one mask shall give the
  same cpu for every context index (D-4).
- **FR-010**: `none` shall be the default and shall place nothing, which
  is the behavior feature 020 ships.
- **FR-011**: `core` shall give each context the lowest-index hardware
  thread of a distinct core, walking cores in ascending order of that
  thread.
- **FR-012**: `l3` shall give each context the lowest-index hardware
  thread of a distinct L3 group, walking groups in ascending order of
  their lowest hardware thread.
- **FR-013**: `smt` shall give every context of the run a hardware
  thread of one core: the core holding the lowest-index thread of the
  mask, and within that core the siblings in ascending order.
- **FR-014**: `compact` shall fill every hardware thread of a core
  before the next core, and every core of an L3 group before the next
  group, walking cores and groups in ascending order.

### Selection (C-3)

- **FR-015**: A benchmark option on the handle shall select a preset for
  the instances of that handle, and a command-line option shall select
  the preset for every threaded run of the process. Where both stand, the
  handle option shall win.
- **FR-016**: The value of either option shall be one preset name. A name
  the system does not provide shall print one line and exit with status
  2 (FR-007).
- **FR-017**: A value that names cpus, a cpu list, or a range shall print
  one line and exit with status 2.
- **FR-018**: A preset whose place count stands below the context count
  of the run shall print one line naming the preset, the count it can
  fill, and the count the run asks for, and shall exit with status 2
  before any context starts, with no context run and no partial result
  printed (D-5). A family that carries several context counts shall be
  checked as a batch before the first run of that batch starts, and one
  count above the fit limit shall stop the process the same way
  (Clarifications Q-4).
- **FR-019**: A preset shall apply to a run of more than one context. A
  run of one context shall keep the behavior feature 020 ships.

### Report (C-4)

- **FR-020**: When a preset stands in effect for any run of a report, the
  console report shall print one topology line in the context block above
  the rows, giving the package count, the L3 group count, the core count,
  and the hardware thread count of the layout the harness read. One line
  stands per report, beside the version and build-type line.
- **FR-021**: The disclosure of each context of a threaded run shall
  carry that context's cpu and the L3 group it belongs to. The placement
  field shall render the pair as `cpu@group`, for example `24@0`, and
  `NO_PIN` shall keep the bytes feature 020 prints. The group shall ride
  inside the placement field, so the field count of the disclosure line
  of `contracts/report-disclosure.md` stays as written
  (Clarifications Q-2).
- **FR-022**: A run with no preset shall print the bytes it prints today:
  no topology line, and `NO_PIN` in every placement field.
- **FR-023**: The noise warning of feature 020 FR-026 shall keep its
  condition and its text; this feature adds no warning condition.
- **FR-024**: The topology line and the disclosure shall read no clock
  and shall add no sampling action to any run, the separation feature
  020 FR-027 states.

### Main-page documentation and runnable example (C-5)

- **FR-025**: `README.md` shall gain one section "Topology placement" of
  at most 25 lines, and its snippet shall come from a runnable example
  under `example/docs/` that CI builds and runs through a `docs_` target
  and an `example_run` entry.
- **FR-026**: The quick-reference table shall gain a row for every
  user-visible name this feature adds.
- **FR-027**: `docs/pages/harness.md` shall carry the detail: the preset
  table, the fit limit a machine imposes, and the report shape with the
  topology line and the placement fields.
- **FR-028**: The drift gate shall pass at the feature head, and
  `PAGE_LINE_CAP` shall rise by the measured size of the new section and
  its rows through a recorded decision (Budget note).
- **FR-029**: The example shall pick a preset the host it runs on can
  fill, so CI needs no machine of a fixed shape.

### Surface, version, and discipline

- **FR-030**: `PlacementSlot`, `kNoPin`, `ContextPolicy`,
  `PlacementResult`, `PlacementFn`, `setPlacementFunction`, and
  `detail::placementRecordOf` shall keep their signatures (D-1).
- **FR-031**: No header under `include/` shall include an hwloc header,
  and no public header shall gain a type (specs/003-vendor-hwloc
  FR-021).
- **FR-032**: The project `VERSION` shall take the next free minor
  version at merge time, `0.12.0` now that the percentiles lane took
  `0.11.0` at `8eac15d`.
- **FR-033**: The harness `SOVERSION` shall stay at the value `master` carries at the rebase (7 at the point above) and the archive
  `SOVERSION` shall stay 2 unless the plan shows an exported layout
  moving, which is the move that number records
  (`docs/pages/building.md`, "Archive versions").
- **FR-034**: Every new path shall reach 100 percent line and branch
  coverage with no exclusion marker, no waiver row and no residual, and
  every Principle VIII gate shall pass at the feature head.
- **FR-035**: Every time and counter value on the new paths shall come
  from `sg::counters`, and the time-source gate shall stay green.
- **FR-036**: A command-line listing flag shall print the layout the
  harness reads: the package, L3 group, core, and hardware thread counts,
  and the cpu membership of each L3 group. On a layout it reads it shall
  exit with status 0 and run no benchmark, the pattern of the standing
  list and catalog modes. On a layout it cannot read it shall print one
  line and exit with status 2, the status FR-005 sets for an unreadable
  layout; the candidate of a status-0 exit over an unreadable layout was
  rejected, because it would print counts a reader cannot tell apart from
  a real machine. It shall print the process mask the layout was built from on
  the same line, so a reader can tell a narrowed process from a full machine.
  An unknown flag shall keep the standing unknown-option
  behavior (Clarifications Q-3, plan decision R-07).

## Key Entities *(include if feature involves data)*

- **Machine layout**: the packages, L3 groups, cores, and hardware
  threads of one process's reach, with the core and the L3 group of each
  thread. Read once per process.
- **Preset**: a named rule that turns a context index into one hardware
  thread of the layout, with the count of places it can fill.
- **Placement plan**: the ordered list of cpus one preset yields for one
  context count, held beside the context count it serves.
- **Disclosure record**: per context, the wait mode, the placement
  account (cpu and L3 group), and whether the placement landed.
- **Topology line**: the run-level counts of the layout, printed once per
  run when a preset is in effect.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-01**: On thrawn with a full mask, a four-context run with `l3`
  places the four contexts on four different L3 groups, and a
  five-context run with `l3` exits with status 2 with one line.
- **SC-02**: On thrawn with a full mask the fit limits hold: `l3` fills
  4, `smt` fills 2, `core` fills 24, `compact` fills 48 (PC-7).
- **SC-03**: Two runs of one preset on one mask give the same cpu for
  every context index, on the injected layouts and on the host.
- **SC-04**: Every placement test passes on an injected layout covering
  one group, four groups, siblings on, siblings off, and a mask smaller
  than the machine, with no privilege, no real pin, and no host
  dependency.
- **SC-05**: A run with no preset prints byte-identical report bytes to
  the run before this feature, and the report shape checks pass.
- **SC-06**: Each of the three bad inputs (an unknown name, a cpu list,
  an over-subscribed preset) exits with status 2 after exactly one line
  on stderr and no result row.
- **SC-07**: The report shows each context's cpu and L3 group, and the
  topology line appears exactly once per report when a preset is in
  effect.
- **SC-08**: Every new path carries 100 percent line and branch coverage,
  and every Principle VIII gate, sanitizer and coverage included, passes
  at the feature head.
- **SC-09**: The main-page section stays within 25 lines, its example
  runs in CI, and the drift gate passes with `PAGE_LINE_CAP` raised by
  the measured amount.
- **SC-10**: No public signature moves and both `SOVERSION` values stay,
  or the plan records the exported layout that moved (FR-030, FR-033).
- **SC-11**: A run on thrawn outside CI prints the layout and the
  placement of a preset run. It is a check the owner reads, and no gate.
- **SC-12**: The listing flag prints the layout and exits with status 0
  with no benchmark run, and its counts equal the topology line of a
  preset run on the same host.

## Constraints

- Coverage: 100 percent line and branch on every new path. No
  `LCOV_EXCL` marker, no waiver, no residual, no `PARTIAL` row and no
  `OPEN` row. Code shall not be shaped to move a coverage count. A gap
  that stays stops the work and reports the cause with `gcov -b`
  evidence.
- Tests: no sleep and no clock in a test. The fake provider and barriers
  drive timing. Tests run under `ctest -j 2`.
- The coverage preset keeps `-fprofile-update=atomic` and keeps
  `-D_GLIBCXX_ASSERTIONS` out.
- Dogfood: every time and counter value comes from `sg::counters`, and
  the time-source gate stays green.
- An invalid option or value prints one line and exits with status 2, the
  standing rule of the command line.
- Docs ship with the feature: the README section stays within
  `SECTION_LINE_CAP`, its snippet comes from a runnable example under
  `example/docs/` with a `docs_` target and an `example_run` entry, the
  drift gate stays green, quick-reference rows are added, and detail
  goes to `docs/pages/harness.md`.
- History: rebase, and push with `--force-with-lease`. After each push,
  show `git ls-remote origin <branch>`. Find and fix every failure the CI
  logs show; a failure never travels to the owner.
- Commit messages follow the Pull Request Quality template: a title of
  50 characters or fewer, a why-body wrapped at 72, blank lines between
  body paragraphs, and one footer block (`Approved-by:` then `Refs:`)
  with no blank line inside. `Approved-by:` appears only for a decision
  the owner made; the decisions of this spec are recommendations the owner
  accepted.
- hwloc stays a private dependency: the link stays `PRIVATE`, the
  imported target stays unexported, and no public header names it
  (specs/003-vendor-hwloc FR-021, privacy contract). The layout read
  stands in the harness archive, which takes the vendored include and
  link `PRIVATE`, and no hwloc symbol crosses an installed target
  boundary (Clarifications Q-1).
- Linux only, the supported platform of `CMakePresets.json`.

## Scope

In: FR-001 to FR-036: the layout read, the five presets, the handle and
command-line selection, the layout listing flag, the topology line and
the placement fields of the disclosure, the main-page section with its
runnable example, the quick-reference rows, the `docs/pages/harness.md`
detail, the injected layout tests, and the version effect.

Out, each owned by a later roadmap item or a later decision:

- NUMA memory nodes, memory-node presets, and the NPS BIOS tests: N12,
  the next item on the thrawn lane. N4 places contexts on one memory
  node, and no NUMA node appears in a preset, a report field or a test.
- Context groups and YAML: N9.
- Execution graphs, continuations, and dependencies: N11.
- An explicit cpu list, a cpu range, or a cpuset slot as input.
- Placement of the runner of a single-context run.
- A JSON report (H7).
- A `scatter` preset.

## Assumptions

- The request of 2026-10-19 is the owner's pasted text and is the sole
  authority for scope. The roadmap file it names sits outside this
  repository and is absent from this machine, so the roadmap position
  section carries the lane facts from the request itself.
- thrawn is the target machine and its measured layout (PC-7) is the
  reference for the fit limits. The wiki machine record for thrawn came
  from a store listing and stands partly unchecked; the sysfs read at
  run time is the evidence used here.
- No machine in the fleet has two sockets, so a multi-package layout
  reaches the tests through an injected layout, and the NPS BIOS setting
  belongs to N12.
- The vendored hwloc at the pinned commit can describe packages, L3
  caches, cores and processing units on Linux through its normal backend.
  Its topology and allowed sets follow the machine and the control-group
  cpuset, measured at the audit point against the system hwloc 2.15.0 with
  the pinned submodule standing uninitialized here: under `taskset -c 0,1,8,9`
  the library still reports 48 processing units and a topology set of `0-47`.
  The mask FR-005 asks for is therefore the process affinity mask the
  operating system reports, the one `source/harness/report.cpp` already
  reads, and the layout covers the intersection of the two. The submodule
  stands uninitialized on this machine, so the plan confirms the pinned
  sources at implement time; the requirement text of FR-001 and FR-005
  does not depend on which call provides the mask.
- The harness archive takes the vendored hwloc include and link
  `PRIVATE` for the layout read, and one harness translation unit owns
  that include (Clarifications Q-1). The library archive's gate unit, its
  version tripwire and its `nm` proof stay as feature 003 shipped them.
- A preset applies per run; the instance name keeps no placement
  segment, so the same instance name can carry two placements and the
  disclosure separates them.
- The default wait mode of a threaded run stays `SPIN` with the preset
  selecting placement alone.

## Budget note

`README.md` stands at 659 lines with `PAGE_LINE_CAP = 659`: no headroom
at the rebase point (PC-5). The new "Topology placement" section and its
quick-reference rows raise the page, so `PAGE_LINE_CAP` rises by the
measured size of what this feature adds, recorded at implement time the
way feature 020 recorded its own move. `SECTION_LINE_CAP` stays 25,
prose is trimmed first, and no snippet is dropped. Two earlier artifacts
move with this change: the option table of
`specs/015-benchmark-harness-core/contracts/cli.md`, which the new option
rows join in the same commit as the parser that adds each option, and the
placement value set of
`specs/020-execution-contexts-core/contracts/report-disclosure.md`, which
gains `cpu@group` in the same commit as the printer. No other earlier spec
is edited.

## Dependencies

- Feature 020 (H5): `PlacementSlot`, `PlacementFn`,
  `setPlacementFunction`, `detail::placementRecordOf`, the threaded
  context creation in `source/harness/runner.cpp`, and the disclosure
  contract in `specs/020-execution-contexts-core/contracts/`.
- Feature 003 (vendor hwloc): the pinned submodule, the `sg_` symbol
  prefix, the embedded build with libxml2 off, the single-unit include
  rule of FR-021, and the `nm` and purity proofs in `tools/hwloc/`.
- Feature 015 (harness core): the command-line option table and the
  status 2 rule for a bad value (`source/harness/cli.cpp`).
- The docs drift gate (`tools/docs/example_drift.py`), the report shape
  checks (`tools/docs/report_shape.py`), and the prose gate
  (`tools/prose/`).
- The three parallel features of the same audit point, through the
  shared file list under "Parallel work".

## Citations

- `include/speedgun-ng/execution_context.hpp`: `PlacementSlot`, `kNoPin`,
  `ContextPolicy`, `PlacementResult`, `PlacementFn`,
  `setPlacementFunction`, `detail::placementRecordOf`.
- `source/harness/execution_context.cpp`: the default pin, its registry
  slot, and the install.
- `source/harness/runner.cpp`: the threaded context creation and its
  `SPIN`/`noPin()` policy.
- `source/hwloc/hwloc_gate.cpp`: the single hwloc include, the 2.14.0
  tripwire, the link proof.
- `CMakeLists.txt`: hwloc ingestion and privacy, the harness target,
  `SOVERSION` 7, and the version comments of 020 and 021.
- `specs/003-vendor-hwloc/spec.md`: FR-021, the include rule.
- `specs/020-execution-contexts-core/contracts/report-disclosure.md`:
  the disclosure line, the cut rule, the noise warning, the one host
  fact.
- `README.md`: the disclosure example, the Threads section, the
  quick-reference rows.
- `tools/docs/example_drift.py`: the section registration and the caps.
- `docs/pages/building.md`: the `SOVERSION` rule and the releases that
  moved it.
- sysfs on thrawn at run time: `cpu*/cache/index3/shared_cpu_list`,
  `cpu/online`, `node/`.

## Open questions

None open. The five recommendations the request carried stand as D-1 to
D-5, and each is accepted. Two points the plan resolves inside those
bounds, with no owner decision needed:

- The spelling of the two selection option names and the listing flag.
  The handle option names the placement and takes one preset name; the
  command-line options follow the standing hyphenated style of
  `--min-time`, `--warmup-time`, `--list`, and `--catalog`. The
  requirement text fixes the behavior (FR-015 to FR-018, FR-036), and the
  plan records the chosen spellings. Clarifications Q-3 fixed the listing
  flag as `--topology`, and plan decision R-07 records the handle option
  and the selection flag, so no spelling stands open at the plan's close.
- The translation unit that owns the hwloc include. The single-include
  rule of `specs/003-vendor-hwloc` FR-021 binds the choice, and FR-001 to
  FR-004 fix the behavior. Clarifications Q-1 settled the rule, and plan
  decision R-01 records `source/harness/topology.cpp` as the one harness
  unit that includes `<hwloc.h>`, so no choice stands open at the plan's
  close.

## Constitution Checklist

- Main-page impact check (IV.1): yes. This feature changes what the root
  `README.md` teaches, so the page gains the "Topology placement"
  section, its runnable `example/docs/` region, and its quick-reference
  rows in the same change, within the budget recorded above.
