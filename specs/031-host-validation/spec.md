# Feature Specification: Host Validation

**Feature Branch**: `031-host-validation`

**Created**: 2026-10-29

**Status**: Draft

**Input**: User description, opening quoted:

> Give every host that builds and tests speedgun-ng a way to prove
> what it is: the `validate` target writes a record for the host's
> CPU type into `validation/`, and a CI check reads the committed
> records. No hardware CI tier: the records travel as committed
> files, and the gates keep passing on any host (ruling R-9 of
> 2026-10-23).

The request fixes the rulings P-1 to P-6; they stand in Decisions as
D-1 to D-6. The request's acceptance-criteria shape carries as AC-1
(injected root, host-agnostic suite), AC-2 (coverage and construct
rules) and AC-3 (the per-CPU-type acceptance rows close here).

## Spec identity and run-time binding

- Spec number: `031`. `specs/` on master `1049cce` holds `001` to
  `036` with `031` the one free number in that range (`033` landed as
  the hunter-budgets lane). `specs/030-ci-speedup` and
  `specs/034-host-noise-precheck` each record `031` as reserved for
  this item, and this spec takes it. `gh pr list --state open` named
  one open pull request, `#57` on `033-hunter-budgets`; that pull
  request merged before this branch was cut, so no open pull request
  carries `031` or `host-validation`.
- Branch: `031-host-validation`, cut from master `1049cce` on
  2026-10-29. The request names `2a48411` as the base. `1049cce` stands 128 commits
  ahead of it, having taken `033`, `035` and `036`. The moved base stands
  recorded here, and the lane builds on `1049cce`.
- Version: master stands at `0.24.0` (`CMakeLists.txt:26`), harness
  `SOVERSION 15` (`CMakeLists.txt:320`), archive `SOVERSION 4`
  (`CMakeLists.txt:91`). The lane moves none of the three: the feature
  adds a target, host-check sources, committed records and a reader,
  and it touches no public ABI (FR-040).
- Report schema: master stands at `1.8`
  (`kSchemaVersion` at `source/harness/json_report.cpp:39`). The lane
  adds no key to that document: the host record is its own document
  with its own pinned schema (D-1, FR-020).
- Option values: no option row joins the harness option table and no
  option value moves. The host-check program and the reader take their
  own arguments through the standing option rules (D-5, FR-004).

## Roadmap position

- Item N0b of the harness roadmap. The request names roadmap section 4
  for the no-version rule and section 10.6, whose rules stay in force
  in full. Ruling H-7 of 2026-10-24 pairs this lane beside the
  hunter-budgets lane, and the work itself is hardware-neutral, so it
  runs beside the other lanes.
- `specs/032-fix-merged-features` FR-041 moves the hunter acceptance
  run to this feature and names this feature's trees: `tools/hostcheck/`
  and `validation/`. Those two names stand as the layout this spec
  fills in.
- The acceptance hosts stand at two hosts the lane cannot reach from its
  bench at branch time. Their runs stand recorded when those hosts run the
  target, and the gap names each one (FR-041).

## Lane identity

- The acceptance rows on the far hosts stand at two CPU shapes: a Zen 5
  host and a Zen 5 Threadripper host (24 cores, 48 threads, several L3
  groups). A third shape, the desktop Intel part of the host that stands
  down since 2026-10-26, waits for that host's return (AC-3, FR-041).
- The suite stays host-agnostic. The record writing and the reader run
  against planted records through the injected file root, so the suite
  needs no particular CPU, passes under `taskset -c 0,1` and passes on
  a 2-cpu host (AC-1, FR-035).
- The lane host contributes its own record: the host that builds and
  tests this lane runs the target once and commits the record its CPU
  type names (FR-041).

## Audit point

- Repository: `master` at `1049cce` (project version `0.24.0`, report
  schema `1.8`, harness `SOVERSION 15`, archive `SOVERSION 4`), read
  2026-10-29 on the lane host.
- The identity the catalog keys on stands at `PmuIdent`
  (`source/counters/detail/pmu.hpp:62`: `vendor`, `family`, `model`,
  `stepping`), read once per process by `pmuIdentCurrent()`
  (`source/counters/linux_pmu/table_parse.cpp:55`). The key renders at
  `mapfileKey` and `mapfileModelKey`
  (`source/counters/detail/pmu.hpp:77`, `:81`; defined at
  `table_parse.cpp:684` and `:673` respectively) in the form
  `<vendor>-<family>-<MODEL>[-<stepping>]` with family decimal and
  model and stepping uppercase hex. The directory the identity selects
  comes from `pmuSelectDirectory`. The stream overload
  (`table_parse.cpp:695`) reads the CSV rows of the mapfile it is handed;
  the cached overload (`:747`) reads the mapfile bytes the build embeds
  and opens no path.
- The counters seam test already consumes that internal surface:
  `sg::counters::detail::pmuIdentCurrent()` at
  `test/source/counters_linux_pmu_seam_test.cpp:1860`, which prints
  `seam ident:` at `:1861` and `seam selected directory:` at `:1866`.
- The machine facts stand in `MachineLayout`
  (`source/harness/detail/internal.hpp:106`: `threads`, `cores`,
  `groups`, `packages`, `mask`, `nodes`, `memoryReadable`) and print
  through `printTopologyListing`
  (`source/harness/topology.cpp:551`): the counts line `topology:
  packages=… l3-groups=… cores=… threads=… mask=…` (`:555`), the group
  rows `l3 <n>: <ranges>` (`:571`), the node rows `node <n>:
  <ranges>` (`:596`) and the `node: not available` line (`:580`). The
  range spelling comes from `appendRangeText` (`topology.cpp:529`).
  `--topology` drives the same path (`source/harness/cli.cpp:663` to
  `:677`) and its unreadable arm prints one line and exits 2
  (`:670`).
- The seams the tests stand on: `installTestProcRoot` and
  `procRootPath` (`source/harness/detail/proc_root.hpp:52`, `:35`),
  `installTestLayout` (`internal.hpp:790`),
  `installTestOnlineCpus` (`:796`), `installTestOnlineLine` (`:803`),
  `installTestTopologyDescription` (`:823`), `installTestLoadFailure`
  (`:828`).
- The hunter rows the record attaches: `specs/029-hunter-core/results/`
  carries `gary-hunt-all.md`, `corsairone-hunt-all.md`,
  `corsairone-hunt-all-top20.txt` and `corsairone-hunt-cpuset.md`.
  `specs/029-hunter-core/spec.md` SC-010 states the runs AC-10 names.
- The pre-check and placement rows:
  `specs/034-host-noise-precheck/results/quickstart.md` and
  `specs/035-numa-placement/results/` (`plan.md`, `nps1.md`,
  `nps2.md`, `nps4.md`, `quickstart.md`).
- The schema-pinning precedent: `docs/schema/report-v1.json` carries a
  normative `x-order` (`:7`) beside the checker that reads it, and
  `docs/schema/compare-v1.json` stands beside it.
- The record precedent: `specs/033-hunter-budgets/results/quickstart.md`
  and `specs/034-host-noise-precheck/results/quickstart.md` carry the
  `| Scenario | Command | Exit | Observation |` shape.
- The keep-set rule: `specs/032-fix-merged-features` FR-026 requires
  the list of patterns and allowance rows that no longer match code,
  with the removal held for approval; the allowance itself stands in
  `specs/030-ci-speedup` D-14.

## Standing facts this lane reads

- `specs/032-fix-merged-features` FR-041: the hunter acceptance run the
  earlier prompt asked for moves to this feature, and that feature
  recorded the move and closed the item. The rows land here as
  evidence (FR-016).
- `specs/029-hunter-core` SC-010: `--hunt=all` ran once with its top 20
  attached under that spec's `results/`, and once on the Zen 5 host.
  Those files stay where they stand; this feature cites them.
- `specs/032-fix-merged-features` FR-024: the repository files carry
  requirement text with no person's name and no host name. D-7 keeps
  that rule for this feature's files.
- The counters library stands the only CPUID reader in the tree, and
  the counters-only constraint of Additional Constraints binds every
  source and header this feature adds, `tools/` included.
- The roadmap stands outside the repository; sections 4 and 10.6 read
  from the lane record this request carries.

## Clarifications

### Session 2026-10-29

- Q: Does the gap rule of FR-041 reach a CPU type whose host stands
  present but whose acceptance run was never made? A: yes. Such a CPU
  type stands recorded as a gap naming the run it waits on, and its
  record may carry an empty evidence list (FR-041, SC-011). A hunter run
  on the lane host was rejected: FR-016 and FR-017 carry the rows the
  earlier prompt named, and a run for a third CPU type closes none of
  them.
- Q: Where do the acceptance rows stand when the host that ran them is
  out of reach? A: a row binds when its record is written, so the lane
  commits its own record, an empty evidence list and a named gap, and each
  waiting row stands in the results record. Authoring a record for a host
  that never ran would make the record a claim no host made (FR-016,
  FR-017, SC-011).
- Q: What does each CTest entry run? A: the reader entry validates the
  committed records from files alone; the writer entry runs the arms that
  read no host, because its argument parse stands before any host read.
  The refusal arms run in the library test through the writing entry point
  with a planted identity, so CI reads no `/proc`, no `/sys` and no PMU
  (FR-037, Out of scope).
- Q: Which mapfile does the directory rule read? A: the CSV under the
  root the run names, through the stream overload of `pmuSelectDirectory`,
  on the write side and the read side. The cached overload reads the bytes
  the build embeds and opens no path, so it cannot answer for a planted
  tree (FR-008, FR-025, FR-028).
- Q: Which coverage figure is the bar? A: the figure the standing gate
  reports for the measured roots, lines at 100.0% and any branch shortfall
  inside the standing allowance of 19 recorded rows, while every file this
  feature adds reads 100.0% on both metrics. Retiring an allowance row
  stays the approval question FR-038 states (FR-037, SC-008).
- Q: What does the root-page block cost? A: `README.md` stands at its
  `PAGE_LINE_CAP` of 912 lines, so the block moves that cap to the
  measured length and adds the dated note beside it, the reading every
  earlier root-page addition recorded. Trimming unrelated prose to hold
  the cap would hide the cost (FR-036, SC-010).
- A narrow-mask run takes its machine facts from the mask the process
  holds, so a `taskset -c 0,1` run takes an output root under the build
  directory and cannot overwrite the committed record (FR-034).
- Q: Do the two Zen 5 hosts render one record or two? A: the question
  stays open and the design does not turn on it. No record holds their
  idents, and the mapfile spreads family 26 over two directories
  (`external/pmu-events/arch/x86/mapfile.csv:46`, `:47`). Two hosts keep
  two records when their idents differ and share one record when they do
  not; a shared record carries both hosts' rows and its machine block
  describes the host that wrote last (D-7, FR-006, FR-016).
- Q: Does the target run the reader after the writer? A: yes, write then
  check, and a failing step fails the target, so one `validate` run prints
  the record line and one line per committed record (FR-001).
- Q: What does a write do when the record would pass 16 KiB? A: it refuses,
  one line, exit 2, standing bytes untouched, because a document nobody can
  read is not a record (FR-018, FR-030).
- Q: What does the record hold where machine memory reads unavailable? A:
  the JSON `null` for the node block, the value form of the standing console
  line, so the schema states one kind per key (FR-013, FR-020).
- Q: What names a record file, and which entries under the record root are
  records? A: the writer names the record after the key with the `.json`
  suffix, and the reader takes the `.json` entries directly under the record
  root, ignores every other entry there, and fails on one it cannot open. A
  root that holds no such entry is the empty case (FR-009, FR-029).
- Q: Where does the writer parse its arguments, and which of its arms run as
  a CTest entry? A: in the writing entry point under `source/harness/detail/`
  before any host read, the way the reading entry point does. One
  malformed-argument invocation stands as the program's CTest entry, pinned
  by a passing pattern, and the unknown option, the missing value and the
  extra argument run in-process through that entry point, so all four arms
  stand measured and no runner reads CPUID (FR-004, FR-037).
- Q: Who stands the output root when it stands absent? A: `cmake/validate.cmake`
  creates the output root before it invokes the writer, and the writer creates
  nothing beyond the record file it names, so a run straight against the program
  needs the directory the quickstart shows making (FR-002, FR-011).
- Q: What does the default output root resolve against? A: the tree root the run
  names, so a writer run without `--out` stands its record at
  `<root>/validation`, never at a path the process working directory happens to
  name. The reader takes the same root beside its record root, so one tree answers
  both programs whatever directory started the run (FR-002, FR-028).
- Q: What answers a mapfile whose rows the matcher cannot interpret, and a
  catalog directory the identity names that the root does not hold? A: the
  standing refusal answers both. Rows the matcher cannot interpret take the
  unreadable-mapfile answer on both sides, the writer's seam and the reader's
  chooser: one line naming that mapfile, status 2, and no partial record. A
  `std::regex_error` that reaches `main` is a crash, and a crash answers
  nothing. The missing catalog directory is refused by the writer beside its
  empty-directory arms, because no reader compares that field
  (FR-004, FR-010, FR-011, FR-025).
- Q: Does the rule against a fixed cpu count reach a planted document? A: the
  rule forbids asking the machine its cpu count. A figure written into a planted
  layout or record is data the program under test interprets, and the same
  binaries answer alike under a narrow cpu mask (FR-034, FR-038).

## Decisions

Decisions the request fixes for this feature, recorded as requirements:

- **D-1: the record is data, and the schema is checked in (P-1).** One
  JSON record per CPU type stands under `validation/`, written by the
  target, read by the check, and guarded by a pinned schema and its
  round-trip test the way `docs/schema/report-v1.json` is guarded
  (FR-001, FR-009, FR-020, FR-021).
- **D-2: the target writes no benchmark results into the repo (P-2).**
  The record carries the identity, the machine facts and the evidence
  references. The bulky figures stay in the recorded command output the
  evidence cites, the way the standing records cite their commands
  (FR-003, FR-015, FR-018).
- **D-3: the check is a reader (P-3).** The check validates the
  committed records and fails on a schema break or a stale record. It
  runs nothing on hardware, needs no token, and adds no
  `--ignore-errors` entry anywhere (FR-024, FR-026, FR-027).
- **D-4: the CPU identity comes from the counters library (P-4).** The
  record names the CPU type through the detection the catalog already
  performs: the ident `pmuIdentCurrent()` reads, the key `mapfileKey`
  renders and the directory `pmuSelectDirectory` selects. The feature
  adds no second CPUID reader, no new event table and no license
  change (FR-007, FR-008).
- **D-5: the refusal shape (P-5).** A run on a host the detection
  cannot name, or where a required read fails, prints one line on
  standard error and exits 2, and it writes no record. The standing
  option rules bind the new target invocation path (FR-004, FR-010,
  FR-011).
- **D-6: the docs (P-6).** The root page carries the target in 25
  lines or fewer: `## Quick start` takes one purpose sentence, one
  marked shell block and one result sentence, the shell-region shape that
  section already carries for `example/quickstart.sh#library` and
  `#consumer`. The block reads from `example/quickstart.sh`, the
  file that already owns that section's blocks, so the drift tool keeps
  one owner per block and no benchmark has to call a name this feature
  does not add (FR-040). The record shape, the staleness rules and the
  exit statuses stand in `docs/pages/building.md` under
  `## Quality gates`. No row joins the `Name` or `Option` table: the
  page resolves each `Name` row against the public headers, and no
  public name and no option joins (FR-040, FR-004).
- **D-7: the record identity and the tree layout.** The CPU type is the
  key `mapfileKey` renders for the host, and the record file name is
  that key with the `.json` suffix. Two hosts whose ident renders one key
  share one record:
  the later run refreshes the identity and machine facts and keeps the
  evidence entries it cannot derive (FR-006, FR-009). Two hosts of one
  microarchitecture keep two records when their idents differ in family,
  model or stepping, and share one record when they do not. A shared
  record carries the rows of both hosts, and its machine block describes
  the host that wrote last (FR-006, FR-016). Repository files name
  CPU types and shapes, and no host name and no person name is authored
  into a file this feature adds (FR-042), the rule `specs/032` FR-024
  states for the merged features. An evidence entry cites a recorded run by the path
  the repository already gives it, so a run recorded under a host-named
  path stays traceable; the feature authors no such name itself.
- **D-8: freshness reads content.** A record stands stale when its
  format version lags the pinned schema, when an evidence path it cites
  stands absent, when the line an entry cites no longer stands in that
  file, or when the identity no longer selects a catalog directory in
  the tree (FR-025). The record carries no wall-clock instant: the
  harness reads no wall clock today, the counters-only constraint binds
  the read, and a rule that fires by calendar would break ruling R-9,
  which keeps the gates passing on any host. Where this feature reads
  time it reads the injected counters time source, and no test sleeps
  and no test reads a clock (FR-033).
- **D-9: the seams are the standing ones.** `/proc` and `/sys` reads
  resolve under the standing file root; the machine facts read through
  the standing topology seams; the record directory is an output root
  the target injects, so the tests and the reader run against planted
  records (FR-031, FR-032, FR-035).
- **D-10: an empty record set stands a failure.** The check names an
  empty `validation/` directory and exits 2, because a tree that
  commits no record carries no host evidence at all (FR-029).

## Out of scope

- No hardware CI tier: no runner, no self-hosted job, no token, no
  scheduled hardware run, and no CI step that reads `/proc`, `/sys` or
  a PMU (D-3, FR-026).
- No version move, no `SOVERSION` move, no report-schema move and no
  option-table change; no public name joins `include/speedgun-ng/`
  (FR-040).
- No second CPUID reader, no CPUID use outside the counters library, no
  new event table, no new architecture table and no license change
  (FR-007).
- No benchmark figure, no recorded run output and no timing distribution
  under `validation/` (FR-003).
- The hunter budgets (`033`), the NUMA placement lane (`035`) and the
  tracer lane (`036`) keep their own items; this feature cites their
  records and changes none of them.
- The `sg::NoiseWarning` disclosure, the jitter and pre-check record
  shapes and the report schema stay as they stand.
- No new hard runtime dependency.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - A host proves what it is (Priority: P1)

A contributor runs the `validate` target on a host that builds and
tests the library. The host checks build and run, and one JSON record
lands under `validation/`, named for the host's CPU type. The record
names the CPU identity the catalog keys on, the machine facts the
listing prints, and the record format it was written against. Nothing
else in the tree changes.

**Why this priority**: the record is the feature's deliverable. The
reader, the docs and the evidence rows all stand on it.

**Independent Test**: with an identity supplied as values and a
layout installed through the standing seam, one writing run produces
one record whose name is the key the detection renders with the `.json`
  suffix, whose identity
fields equal the supplied identity, and whose machine facts equal the
installed layout; the run prints one line and exits 0.

**Acceptance Scenarios**:

1. **Given** a supplied ident and an installed layout, **When** the
   writing entry point runs against an empty output root, **Then** one
   record stands under that root, named by that ident's key with the
   `.json` suffix, and
   the run exits 0.
2. **Given** a record that already stands for that CPU type, **When**
   the target runs again, **Then** the identity and machine facts
   refresh, the evidence entries stand kept, and one record still
   stands for that CPU type.
3. **Given** the record the lane host wrote, **When** the reader runs
   over the tree, **Then** that record validates against the pinned
   schema and exits 0.

---

### User Story 2 - CI reads the committed records (Priority: P1)

A CI job reads the records committed under `validation/`. Each record
must validate against the pinned schema, and each record must still
match the tree: its cited evidence must stand, and its identity must
still select a catalog directory. The job names every CPU type whose
record breaks or stands stale, and it runs nothing on hardware.

**Why this priority**: the committed record is only worth having while
a check keeps it true. This story is the gate that makes the records
carry weight on any host (ruling R-9).

**Independent Test**: against a planted tree root, a well-formed record
reads exit 0; a planted key-set break, a planted order break and a
planted kind break each read one line and exit 2; each planted stale
cause names its CPU type and the rule that fired.

**Acceptance Scenarios**:

1. **Given** planted records that all validate, **When** the reader
   runs, **Then** it prints one line per record, exits 0 and reads no
   `/proc` or `/sys` file.
2. **Given** a planted record whose cited evidence path stands absent,
   **When** the reader runs, **Then** one line names that CPU type and
   the absent path, and the exit status is 2.
3. **Given** a planted record whose identity selects no catalog
   directory in the tree, **When** the reader runs, **Then** one line
   names that CPU type and the rule, and the exit status is 2.
4. **Given** a planted record whose format version lags the pinned
   schema, **When** the reader runs, **Then** one line names that CPU type
   as stale with the two versions, and the exit status is 2.

---

### User Story 3 - The acceptance rows travel with the record (Priority: P2)

A maintainer opens the record for a CPU type and reads the acceptance
rows that stand for that type: the `--hunt=all` run and its top 20, the
placement runs and the pre-check runs. Each row names a file in the
repository, the command that wrote it, the exit status it ended with,
and the one line the row cites. The bulky figures stay in the cited
file.

**Why this priority**: the per-CPU-type acceptance rows move to this
feature, and the record is where they belong. The rows depend on
records that already stand, so this story follows the first two.

**Independent Test**: a planted record with three evidence entries
reads back with all three, the reader confirms each cited path and each
cited line, and deleting one cited file makes that entry the stale
cause the reader names.

**Acceptance Scenarios**:

1. **Given** a CPU type whose hunter rows stand recorded, **When** its
   record is written, **Then** the record carries the hunter row and
   its top-20 row, each naming path, command, exit status and cited
   line.
2. **Given** the placement and pre-check rows of the merged features,
   **When** the record of the CPU type that ran them is read,
   **Then** those rows stand on that record and cite their own recorded
   runs.
3. **Given** a record whose cited line no longer stands in the cited
   file, **When** the reader runs, **Then** it names that CPU type as
   stale with the path it could not confirm.

---

### User Story 4 - An unnamed host refuses (Priority: P2)

A host the detection cannot name runs the target. One line on standard
error names the identity it read, the exit status is 2, and no record
is written. A required read that fails reads the same way: one line
naming the read, exit 2, nothing written. A bad argument on the new
invocation path reads the same way.

**Why this priority**: the refusal is what keeps an unverifiable record
out of the tree, and the standing error shape is a contract every
invocation path keeps.

**Independent Test**: with a planted ident the catalog selects nothing
for, and with the layout read made to fail, the writing entry point
prints one line on standard error, exits 2 and leaves the output root
empty; a malformed argument on each new invocation path does the same
before any host read.

**Acceptance Scenarios**:

1. **Given** an ident whose vendor stands empty, **When** the writing
   entry point runs, **Then** one line names the identity read, the exit
   status is 2 and no record is written.
2. **Given** an ident a planted mapfile selects no directory for,
   **When** the writing entry point runs, **Then** one line names that
   identity and the exit status is 2.
3. **Given** the machine-layout read made to fail, **When** the writing
   entry point runs, **Then** one line names the failed read, the exit
   status is 2 and no record is written.
4. **Given** an argument the reader cannot parse, **When** the reader
   runs, **Then** one line prints on standard error and the exit status
   is 2.

---

### User Story 5 - The docs teach the target (Priority: P3)

A reader looking for the way to prove a host finds it on the root page
under the quick start: one purpose sentence, one block the drift tool
owns, one result sentence, 25 lines or fewer, naming the target, the
reader and both exit statuses; the record shape and the staleness rules
stand one page away under the quality gates of the build page.

**Why this priority**: the docs gates bind every feature, and the root
page is the reader path IV.1 asks for when a change is user-visible.

**Independent Test**: the root-page addition counts 25 lines or fewer,
each block in it carries a region marker the drift tool compares, and
the drift, snippet-compile, run and report-shape gates pass over the
page.

**Acceptance Scenarios**:

1. **Given** the root page, **When** the line count of the quick-start
   addition is taken, **Then** it stands at 25 lines or fewer.
2. **Given** the two pages, **When** a reader follows them, **Then** the
   root page names the target, the reader and both exit statuses, and the
   build page names the record path shape and the staleness rules.
3. **Given** the root page, **When** the drift gate reads it, **Then**
   the section list, the `Name` rows and the `Option` rows still match
   the tree, because no row joins either table.

### Edge Cases

- An ident that selects no catalog directory (a part newer than the
  pinned mapfile): one line, exit 2, no record (FR-010).
- A host whose vendor stands empty (a non-x86 build where the
  detection leaves the ident unset): one line, exit 2, no record
  (FR-010).
- The machine layout reads unreadable: one line naming the read, exit
  2, no record, the standing `--topology` refusal shape (FR-011).
- Machine memory reads unavailable: the node block stands the JSON `null`
  and the record still writes (FR-013, FR-020).
- Two hosts of one CPU type: the second run refreshes the one record
  and keeps the evidence entries it cannot derive (FR-006).
- A read-only output root: one line naming the write that failed, exit
  2, no partial record left behind (FR-011).
- An argument the target or the reader cannot parse: one line, exit 2
  (FR-004).
- An empty `validation/` directory: the reader names it and exits 2
  (D-10, FR-029).
- A planted record larger than the recorded 16 KiB cap: the reader names
  it and exits 2 (FR-018, FR-030).
- A cited evidence file that moved or whose cited line was rewritten:
  the record stands stale and the line names the path (FR-025).
- A pinned mapfile row that a later change drops: the CPU type that
  keyed on it stands stale, and no hardware read is needed to see it
  (FR-025).
- A 2-cpu host and a `taskset -c 0,1` run: the suite passes, and the
  record names the mask the process holds (FR-012, FR-034).

## Requirements *(mandatory)*

### The target (A)

- **FR-001** (D-1, AC-1): The build shall offer a `validate` target that
  builds the writer and the reader, runs the write and then the check, and
  writes one record for the host's CPU type under `validation/`; a failing
  step shall fail the target. The target shall stand outside the default
  build target, so an ordinary build writes no record.
- **FR-002** (D-9): The write path shall take the tree root and an output
  root as arguments, the output root defaulting to the `validation/`
  directory of the source tree, so the tests, the quickstart and the
  target run against a temporary root and a planted tree.
- **FR-003** (D-2, P-2): A `validate` run shall write no record outside
  its output root, and no benchmark result, recorded command output or
  cited figure shall be copied there. A build artifact of the two programs
  stands under the build directory, where the developer build puts every
  target.
- **FR-004** (D-5): Each argument of the new invocation paths (the
  host-check program and the reader) shall follow the standing option
  rules: an unusable argument shall print one line on standard error
  and exit 2, and no row of the harness option table shall gain an
  entry.
- **FR-005**: A successful run shall print one line naming the CPU type
  and the record path it wrote, and shall exit 0.
- **FR-006** (D-7): When a record already stands for the host's CPU
  type, the target shall refresh it in place: the identity and machine
  facts the run reads shall replace the standing ones, and the evidence
  entries the run cannot derive shall stand kept.

### The CPU identity (B)

- **FR-007** (D-4, P-4): The CPU identity of a record shall come from
  the detection the catalog already performs: the vendor, family, model
  and stepping `pmuIdentCurrent()` reads and the key `mapfileKey`
  renders. No second CPUID reader shall join the tree, no CPUID use
  shall join any source outside the counters library, no event table
  shall join the change, and no license shall change.
- **FR-008** (D-4): The record shall name the catalog directory the
  identity selects, as `pmuSelectDirectory` selects it from the pinned
  mapfile.
- **FR-009** (D-7, D-1): Exactly one record shall stand per CPU type, and
  its file name under the output root shall be the key `mapfileKey` renders
  for that type with the `.json` suffix. A host that answers no leaf 1
  renders the model key, so its record name carries no stepping segment
  (FR-007).
- **FR-010** (D-5, P-5): On a host the detection cannot name, meaning
  an empty vendor, an unset family or model, or an identity that
  selects no catalog directory, the target shall print one line on
  standard error naming the identity it read, shall exit 2, and shall
  write no record.
- **FR-011** (D-5): When a read the record needs fails, including the
  machine-layout read and the record write itself, the target shall
  print one line on standard error naming the read that failed, shall
  exit 2, and shall leave no partial record behind.

### The machine facts (C)

- **FR-012**: The record shall carry the machine facts the topology
  listing prints: the package count, the L3 group count, the core
  count, the thread count and the affinity mask in the range spelling
  `appendRangeText` renders.
- **FR-013**: Where the listing prints L3 group rows and node rows the
  record shall carry them, and where machine memory reads unavailable the
  node block shall stand the JSON `null`, the value form of the standing
  `node: not available` console line (FR-020).
- **FR-014**: The machine facts shall come from the same layout
  `--topology` prints, and no second topology reader shall join the
  change.

### The evidence rows (D)

- **FR-015** (D-2): The record shall carry an evidence list, and each
  entry shall name a path in the repository, the command that produced
  that file, the exit status it ended with, and the one line of that
  file the entry cites.
- **FR-016** (AC-3, FR-041 of specs/032, SC-010 of specs/029): Once the
  host of a CPU type writes its record, the hunter rows of that CPU type
  shall stand as evidence in it: the `--hunt=all` run and its top 20. A
  CPU type that owns no such run carries no such row, and the results
  record says so in one line.
- **FR-017**: Under the same reading, the placement rows and the
  pre-check rows of the merged features shall stand as evidence in the
  record of the CPU type whose host ran them, each naming its own
  recorded run.
- **FR-018** (D-2, P-2): A record shall carry references and no bulk:
  the bulky figures shall stay in the cited file, and each record file
  shall stay within the recorded size cap of 16 KiB. A write whose record
  would pass the cap stands refused: one line, exit 2, and a standing
  record keeps its bytes.
- **FR-019**: The record shall name the format version it was written
  against, and the pinned schema file shall name the document kind.

### The schema (E)

- **FR-020** (D-1): A pinned schema file under `docs/schema/` shall
  define the record: the key set, the normative key order carried as
  `x-order`, and the value kind of each key, the way
  `docs/schema/report-v1.json` stands.
- **FR-021** (D-1): A round-trip test shall guard the schema: a written
  record shall read back with equal values, the schema's key set and
  order shall equal the writer's, and planted key-set, order and kind
  breaks shall each fail the guard, so a green run cannot come from a
  check that inspects nothing.
- **FR-022** (AC-2): simdjson shall stand the only JSON reader the
  change uses.
- **FR-023**: A written record shall hold its keys in the schema's
  order, so two runs of one host differ only in the fields whose inputs
  differ.

### The reader (F)

- **FR-024** (D-3, AC-1): A check shall read every record committed
  under the record root, shall validate each against the pinned schema,
  and shall print one line per record naming its CPU type and its
  verdict.
- **FR-025** (D-8): A record shall stand stale when its format version
  lags the pinned schema, when an evidence path it cites stands absent
  from the tree, when the line an entry cites no longer stands in that
  file, or when its identity selects no catalog directory in the mapfile
  under the root it is given; the check shall name each such CPU type
  with the rule that fired, and the mapfile question shall go through the
  stream overload of `pmuSelectDirectory`, so a planted tree drives the
  rule (FR-008, R-06 of the plan).
- **FR-026** (D-3, ruling R-9): The check shall run nothing on
  hardware, shall read no `/proc` or `/sys` file, shall need no token
  and no network, and shall add no `--ignore-errors` entry to any
  configuration.
- **FR-027**: The check shall exit 0 when every record validates and
  none stands stale, and shall exit 2 after one line per offending
  record.
- **FR-028** (D-9): The check shall take the tree root and the record
  root as arguments, so the tests run it against planted records and a
  planted tree.
- **FR-029** (D-10): The reader shall take the `.json` entries directly
  under the record root as the records, ignore every other entry there, and
  stand a failure when that set stands empty: one line naming the root, exit
  2. A `.json` entry the reader cannot open stands a failure too.
- **FR-030**: The reader's verdicts shall stand as three: valid, broken
  and stale. A record breaks when its key set, key order or value kind
  disagrees with the pinned schema, or when it exceeds the recorded size
  cap; a broken record shall report the break and shall report no
  staleness.

### The seam and the test surface (G)

- **FR-031**: Every `/proc` and `/sys` read of the feature shall
  resolve under the standing file root of `proc_root.hpp`, and no new
  file-root seam shall join the tree.
- **FR-032**: The machine facts of a test shall read through the
  standing topology seams, and the record writing of a test shall take
  the identity as a value, because no seam installs a CPU identity and
  none shall join.
- **FR-033** (AC-1): No test shall sleep, no test shall read a clock,
  the record shall carry no wall-clock instant, and where the feature
  reads time it shall read the injected counters time source.
- **FR-034** (AC-1): No fixed cpu number shall stand in code or in a
  test of this feature, and the suite shall pass under `taskset -c 0,1`
  and on a 2-cpu host. A narrow-mask run shall take an output root under
  the build directory, so the machine facts that mask yields never
  overwrite the committed record.
- **FR-035** (AC-1): The record writing and the reader shall run
  against planted records through the injected roots, so the suite
  needs no particular CPU and CI runs it on the standing runners.

### Documentation (H)

- **FR-036** (D-6, IV.1): `README.md` shall carry the target in 25
  lines or fewer inside `## Quick start`, in the shape of one purpose
  sentence, one block marked with a region of `example/quickstart.sh`
  and one result sentence naming both exit statuses, and
  `docs/pages/building.md` shall carry the section that names the
  record path shape, the staleness rules and the exit statuses of both
  paths. No row shall join the root page's `Name` table or `Option`
  table, because the feature adds no public name and no option
  (FR-040, FR-004). `README.md` measures its own `PAGE_LINE_CAP` in
  `tools/docs/example_drift.py`, so the change moves that cap to the
  measured length and adds the dated note beside it that names the
  requirement and the arithmetic.

### The record set and its gates (I)

- **FR-037** (AC-2): Coverage of the code this feature adds under
  `source/`, the roots `cmake/coverage.cmake` measures, shall read 100%
  lines and 100% branches, the figure `tools/dbc/coverage_gate.sh`
  reports for them: lines at the full figure and any branch shortfall
  inside the standing allowance, while the files this feature adds read
  100.0% on both. The two thin program entry points under `tools/` stand
  outside those roots by the standing globs and shall each run as a CTest
  entry: the reader over the committed records, the writer over one
  malformed-argument invocation pinned by a passing pattern. Each entry
  point shall stand in `source/harness/detail/` and shall parse its
  arguments before any host read, so every usage arm of either program
  runs in-process through that entry point and stands covered by the roots
  above (FR-004). No `LCOV_EXCL` marker and no new `--ignore-errors` entry shall
  join.
- **FR-038**: The lane shall report which coverage keep-set patterns and
  which allowance rows no longer match code, with the coverage trace as
  evidence, and shall ask before any of them is removed (FR-026 of
  specs/032, the waiver note of 2026-10-26).
- **FR-039** (AC-2): No lambda shall stand inside an `SG_*` macro,
  contract-only helpers shall carry `[[maybe_unused]]`, and simdjson
  shall stand the only JSON reader.
- **FR-040**: No project version, no `SOVERSION`, no report schema and
  no option table shall move, and no public name shall join
  `include/speedgun-ng/`.
- **FR-041** (AC-3): The lane shall commit the record its own host
  writes. A CPU type whose record waits stands recorded in the results
  record with the shape it waits for and the run it waits on, and a CPU
  type that owns no acceptance row stands recorded there with that fact.
  The evidence list of a record whose acceptance runs stand waiting may
  stand empty.
- **FR-042** (D-7): No host name and no person name shall be authored
  into any file this feature adds; the CPU type and the machine shape
  carry the identity (FR-024 of specs/032). A cited evidence path reads
  as the repository spells it, because renaming a recorded run would
  break the trace a record exists to carry.

## Key Entities *(include if feature involves data)*

- **The host record** (new, one JSON document per CPU type under
  `validation/`): the identity (vendor, family, model, stepping, the
  rendered key, the selected catalog directory), the machine facts
  (package, L3 group, core and thread counts, the mask, the group rows,
  the node rows), the record format version, and the evidence list.
- **The pinned record schema** (new, under `docs/schema/`): the key
  set, the normative `x-order` and the value kinds of the host record;
  the round-trip test and the reader both read it.
- **The evidence entry** (a member of the record): a repository path,
  the command that produced it, its exit status, and the one cited
  line. Its shape makes staleness decidable from the tree alone.
- **The CPU identity** (existing): `PmuIdent` and the key renderers in
  the counters library; the feature reads them and changes none of
  them (FR-007).
- **The machine layout** (existing): `MachineLayout` and
  `printTopologyListing`; the feature reads the same facts the listing
  prints (FR-012, FR-014).
- **The writer** (new, under `tools/hostcheck/`): the thin program the
  `validate` target builds and runs first; it reads the identity and the
  machine facts of the host and hands them to the writing entry point.
- **The reader** (new, under `tools/hostcheck/`): the thin program the
  target runs second and the CTest entry runs alone; it validates the
  committed records and names the CPU types whose records break or stand
  stale.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: The write case stands: against an empty output root with a
  supplied identity and an installed layout, one run writes one record
  named by the rendered key with the `.json` suffix, prints one line that
  names the CPU type and the path, and exits 0.
- **SC-002**: The refresh case stands: a second run on the same CPU
  type leaves one record whose identity and machine facts match the
  second run and whose evidence entries match the first.
- **SC-003**: The reader cases stand: planted valid records exit 0 with
  one line each; each planted schema break and each planted stale cause
  print one line naming the CPU type and the rule and exit 2; an empty
  record root prints one line and exits 2.
- **SC-004**: The refusal cases stand: an unnamed identity, an identity
  that selects no directory, a failed layout read and a failed write
  each print one line on standard error, exit 2, and leave no record
  and no partial record behind.
- **SC-005**: The round-trip case stands: a written record reads back
  with equal values, and the planted key-set, order and kind breaks each
  fail the guard.
- **SC-006**: The committed set validates from a clean checkout on any
  host: the reader reads files alone, names no stale CPU type, and
  needs no token, no network and no PMU.
- **SC-007**: Every record stays within the 16 KiB size cap and carries
  no cited figure, only references.
- **SC-008**: Coverage over the measured roots reads the figure the
  standing gate reports, lines at 100.0% and any branch shortfall inside
  the standing allowance, and every file this feature adds under
  `source/` reads 100.0% lines and 100.0% branches; the two program entry
  points carry their CTest runs as their evidence. No `LCOV_EXCL` marker
  and no new `--ignore-errors` entry joins this change, and no allowance
  row leaves the gate.
- **SC-009**: The suite passes under `taskset -c 0,1` and on a 2-cpu
  host, and no test of this feature sleeps, reads a clock or names a
  fixed cpu number.
- **SC-010**: The docs hold: the root-page addition stands at 25 lines
  or fewer and names the target, the reader and both exit statuses, each
  block on the page keeps the region marker the drift tool compares, the
  page cap stands moved to the measured length, and `example_drift`,
  `example_drift_fixtures`, `snippet_compile`, `example_run`,
  `report_shape`, `prose-lint` and `spell-check` pass.
- **SC-011**: The acceptance rows close one record at a time: each
  hunter, placement and pre-check row stands as an evidence entry on the
  record of the CPU type that ran it as soon as that host writes, and
  each CPU type whose host stands absent or whose acceptance run stands
  missing stands recorded as a gap with the run it waits on.
- **SC-012**: The gates pass: the sanitizer and thread-sanitizer jobs,
  clang-tidy and cppcheck, `format-check`, `spell-check`, `prose-lint`,
  the time-source scan, the dbc gate and the coverage job.

## Constraints

The request's hard rules bind this spec; they restate constitution VI,
VII and VIII:

- No hardware CI tier: the reader runs on files alone, and the gates
  keep passing on any host (ruling R-9 of 2026-10-23).
- Coverage stays 100% lines and 100% branches over the code this feature
  adds, with no `LCOV_EXCL` marker, no new waiver row, no removed
  allowance row, no new `--ignore-errors` entry, and no code shaped for
  coverage.
- No sleep and no clock in a test; the injected roots carry the reads,
  and the record carries no wall-clock instant.
- No fixed cpu numbers: the suite passes under `taskset -c 0,1` and on
  a 2-cpu host.
- No lambda inside an `SG_*` macro; `[[maybe_unused]]` on contract-only
  helpers; simdjson only for JSON.
- No new hard runtime dependency; the counters library stays the only
  CPUID reader and the only time source.
- No version, `SOVERSION`, report-schema or option-table move, and no
  public ABI change.
- The roadmap's section 10.6 rules stay in force in full.
- Each new commit follows the commit template and passes `prose-lint`.

## Assumptions

- The CPU type is the identity the catalog keys on, and the record file
  name is the key `mapfileKey` renders, mixed case kept, because that
  key is already the string the catalog matches on. A lowercase or
  hashed file name would be a second name for one fact.
- The size cap of 16 KiB per record is an informed default: it holds
  tens of evidence entries and the whole identity and machine-fact
  block, and the request names no figure. The cap is recorded so the
  reader can enforce it and a later change can move it deliberately.
- The evidence entries a host cannot derive are authored into the
  record file and kept by later runs (D-7). The alternative, a target
  that derives the rows from history, would need a run database the
  feature does not have.
- Whether two hosts of one microarchitecture render one key or two turns
  on their idents, which no record holds. The mapfile spreads family 26
  over two directories: row `:46` of
  `external/pmu-events/arch/x86/mapfile.csv` matches some models and row
  `:47` sends the rest to `amdzen6`, so the record set holds one Zen 5
  record or two and the directory each host selects turns on its model. A
  shared record carries the rows of both hosts, and its machine block
  describes the host that wrote last (D-7, FR-006, FR-016).
- Host names stay out of repository files (FR-042). The sibling lanes
  split on this: `specs/034` follows `specs/032` FR-024 and names the
  host shape alone, while `specs/035` and `specs/036` name their host.
  The stricter rule stands here, because the record already carries a
  better name for a host: its CPU type.
- The roadmap stands outside the repository; its section 4 and section
  10.6 rules, and rulings R-9 and H-7, read from the lane record this
  request carries.
- The lane host stands as an Intel desktop host at branch time, so the
  lane can commit one real record of its own. The two Zen 5 hosts stand
  outside the lane's reach, so their records arrive when those hosts
  run the target; the gap stands recorded (FR-041).
- The `perf_event_paranoid` value of a record's host belongs to the
  cited run output, and the record cites it the way the hunter record
  already discloses it.
- The docs placement (D-6) reads the gates as they stand on master:
  each root-page block carries a region marker the drift tool compares,
  `## Quick start` already owns shell blocks from `example/quickstart.sh`,
  and the `Name` rows resolve against the public headers. Spec 030 is the
  precedent for a tooling change with no library behavior: it named its CI
  tiers in `## Quick start` in 25 lines or fewer and kept its command
  block on the linked page `docs/pages/prepush.md`. This feature carries
  the commands themselves, because the target and the reader are the
  behavior, and a shell region joins no build target.

## Constitution Checklist

- Main-page impact check (IV.1): the feature adds a target and a
  committed document, so the root page carries the content in 25 lines
  or fewer inside `## Quick start`, the shape spec 030 took for the CI
  tiers (FR-036, D-6). No `Name` row and no `Option` row joins either
  quick-reference table, because no public name and no option joins
  (FR-040).
- Principle II: every new interface carries `\pre`, `\post` and
  `\invariant` in its header and is enforced at the site; the record
  writer's postconditions (one record per CPU type, schema order) and
  the reader's verdict invariant stand as contracts, with the refusal
  paths as errors and contract violations kept apart.
- Principles VI and VIII: the coverage bar, the determinism rules and
  the gate set are restated as binding requirements (Constraints,
  FR-037, SC-012).
- Principle VII: the feature reads the host and writes a document; no
  critical path joins it, so no P0 technique and no new baseline stands
  in scope.
- Principle IX: the full workflow applies; the change touches build
  configuration (a target), CI and committed data.
- Principle X.1: the open readings (staleness shape, the wall-clock
  instant, host naming, the empty record root, the size cap) each stand
  recorded with the chosen answer in Decisions and Assumptions.
- Principle X.2: no run database, no second CPUID reader, no second
  topology reader, no wall clock and no hardware tier stand in scope;
  the feature reuses the standing seams and the standing error shape.
- Principle X.3: each commit touches the files its requirement names,
  and the pinned `external/pmu-events` tables and the report schema
  stand untouched.
- Principle XI: this file holds no em-dash, no contrastive framing, no
  filler vocabulary and no vouching word, and every command, option,
  path and identifier sits in a code span.
