# Research: Host Validation

**Feature**: `specs/031-host-validation` | **Date**: 2026-10-29
**Plan**: [plan.md](./plan.md)

Every reading below was taken from the tree at `1049cce`, the master base
of branch `031-host-validation`. Each decision names the rule that forces
it, the shape chosen, and what the rejected alternative would have cost.

## R-01: one library pair per side of the document

**Decision**: `source/harness/host_record.cpp` writes and
`source/harness/host_record_check.cpp` reads, each behind its own
private header.

**Rationale**: coverage measures `source/*` through the `--extract` roots of
`cmake/coverage.cmake`, and the harness library glob of the root
`CMakeLists.txt` already picks up every `.cpp` in `source/harness/`, so new
translation units need no target edit. Two files let the writer wave
and the reader wave land in parallel, which the task breakdown uses.

**Alternatives rejected**: one file for both sides, which serializes two
waves over one translation unit; the logic under `tools/`, which the
coverage extract does not measure and which FR-037 then cannot read.

## R-02: the document goes through the standing writer pair

**Decision**: the record renders through a file-local `JsonObjectWriter`
and `JsonArrayWriter` over `simdjson::builder::string_builder`, exactly
as `compare.cpp:765` and `:794` do for the compare document.

**Rationale**: key order is call order, so one write path produces the
byte order the schema pins as `x-order`, and the guard test of FR-021
compares the two. The report writer
(`json_report.cpp:90`, `:119`) holds the same pair; copying it is the
shape the tree already chose for a second document.

**Alternatives rejected**: exporting the report writer's file-local
helpers, which would widen an internal interface for no second caller
until this feature needed it; hand-built JSON text, which loses the
escaping and the ordering guarantee that make the byte round trip of
FR-021 testable.

## R-03: `checkReport` validates the record unchanged

**Decision**: the reader calls `sg::detail::checkReport`
(`json_report.hpp:322`, `json_load.cpp:2362`) with the schema resolved
inside the tree root the run names,
`<root>/docs/schema/host-record-v1.json`, one constant the library names
and no option that overrides it.

**Rationale**: the checker is document-agnostic. It already validates
the compare document against `docs/schema/compare-v1.json`
(`harness_compare_cli_test.cpp:969`), and it checks kinds, required
keys, `x-order`, enums, closed objects, array items and root duplicate
keys, which is the whole of FR-020 and FR-021. A bare repo-relative path
cannot reach the file: CTest starts a test in the build directory of the
`CMakeLists.txt` that added it, `build/test` for these entries, and none
of the 144 `add_test` rows of `test/CMakeLists.txt` names a
`WORKING_DIRECTORY`. The root the run already carries resolves the schema,
the mapfile and the records, so an absolute root works wherever the
runner starts, and a missing schema answers a refusal the way a missing
mapfile does.

**Alternatives rejected**: a second schema checker, a second source of
truth for the same rules; a JSON-Schema library, a new dependency the
purity rule of FR-022 forbids.

## R-04: the identity arrives as values

**Decision**: `HostIdent` is plain data (vendor, family, model,
stepping, key, directory). The program fills it from
`pmuIdentCurrent()` (`table_parse.cpp:55`), `mapfileKey` (`:684`) and
`pmuSelectDirectory` (`:695`); the library reads no host.

**Rationale**: `pmuIdentCurrent()` caches CPUID in a function-local
static (`pmu.hpp:84`) and no seam installs an identity, so a library
that called it could only ever be tested on the host it runs on, which
ends AC-1 and FR-031. Values keep every rule testable on any runner and
keep the counters internals out of the harness target's includes.

**Alternatives rejected**: a new `installTestIdent` seam, a second
global for one caller (Principle X.2); a public identity API, forbidden
by FR-040; a library call into `sg::counters::detail`, a new
cross-library internal include for one function.

## R-05: the machine block comes from `MachineLayout`

**Decision**: `hostMachineFromLayout(const MachineLayout&)` renders the
counts, the mask, the L3 group rows and the node rows, spelling ranges
the way `appendRangeText` does (`topology.cpp:529`), and writes `null`
for the node rows where `memoryReadable` stands false.

**Rationale**: `MachineLayout` already holds every fact the record
names, and `printTopologyListing` (`topology.cpp:551`) prints the same
facts for a human, so record and listing agree by construction rather
than by review (FR-014). `null` is the JSON spelling of the standing
`node: not available` line.

**Alternatives rejected**: a `/proc/cpuinfo` reader, a second identity
source that FR-007 forbids and that no test could feed on a runner;
reading hwloc again inside the record code, a second topology reader
against the one the harness already owns.

## R-06: the staleness question arrives as a chooser

**Decision**: `staleReasons(record, treeRoot, chooser)` takes
`const std::function<DirectoryChoice(const HostIdent&)>&`. The programs hand
`hostRecordCheckMain` a `DirectoryChooserFactory`, a
`std::function<DirectoryChooser(const std::filesystem::path&)>`, and its
member `makeDirectoryChooser(treeRoot)` builds a chooser that opens
`<treeRoot>/external/pmu-events/arch/x86/mapfile.csv` at the first question it
is asked, once per run, through
`pmuSelectDirectory(std::istream&, const PmuIdent&)` and answers `FOUND`,
`NO_ROW` or `UNREADABLE_MAPFILE`. A run that holds no record asks nothing and
opens nothing; a test installs its own factory and plants each answer. The
builder and the mapfile opener stand under `tools/hostcheck/`, in a header the
two programs share, because the harness library may not include a counters
internal header (R-04).

**Rationale**: FR-025 requires the check to see a dropped mapfile row
with no hardware read, and the only matcher is
`pmuSelectDirectory` (`table_parse.cpp:695`, the stream overload serves
fixtures). One parameter carries the question, so no second matcher
joins and no global installs.

**Alternatives rejected**: a library call straight into counters, the
cross-library include R-04 rejects; staleness in the program, logic the
coverage gate cannot see; a `struct` interface with one
implementation, ceremony for a single call site.

## R-07: three verdicts, `broken` wins over `stale`

**Decision**: the reader answers `valid`, `broken` (key set, key order,
value kind, or size beyond the cap) and `stale` (format lag, absent
evidence path, missing cited line, no catalog directory). A broken
record reports that verdict and no staleness.

**Rationale**: FR-030. A staleness reading computed over a document
whose keys are wrong describes nothing, and one exit status for both
keeps the CI line short. The size cap belongs to the document, so a
record over the cap breaks.

**Alternatives rejected**: a fourth `oversize` verdict, which widens the
verdict set the docs and the tests enumerate for one cause; reporting
both, which invites a reader line that contradicts itself.

## R-08: the refresh keeps what the run cannot derive

**Decision**: writing over a standing record replaces `cpu` and
`machine` and keeps its `evidence` array (FR-006).

**Rationale**: the evidence list names recorded runs the host cannot
reproduce; the identity and machine facts are what the run exists to
read. Two hosts that render one key therefore share one record whose
evidence grows.

**Alternatives rejected**: full overwrite, which deletes rows on every
run; keeping the machine block too, which strands a record on the shape
of whichever host wrote first.

## R-09: refusal reuses the standing shape

**Decision**: one `fprintf` line on standard error, `std::exit(2)`, no
file written before every read succeeds; the writer renders the whole
text in memory, checks the size cap, then writes once.

**Rationale**: `rejectHuntValue` (`hunt_cli.cpp:41`) and the
`--topology` refusal (`cli.cpp:670`) set the shape, and 2 is the
project's status for a usage or refusal exit. A single write makes the
no-partial-file rule of FR-011 hold without a cleanup path.

**Alternatives rejected**: exceptions through `main`, a second error
style in a library that aborts on a violated precondition; write then
validate, which leaves a partial record on a failed read.

## R-10: the target is the house pattern

**Decision**: `add_custom_target(validate DEPENDS <the two programs>`
`COMMAND cmake -D ... -P cmake/validate.cmake`) appended in
`cmake/dev-mode.cmake` beside the other utility targets, outside `ALL`, with
`VALIDATE_ROOT` and `VALIDATE_OUT` knobs.

**Rationale**: the utility targets in that file are the shape a
contributor already runs, script mode maps program statuses the way
`cmake/prose-lint.cmake:40` does, and `cmake/folders.cmake` already
files such targets under `UtilityTargets`.

**Alternatives rejected**: a new preset, matrix noise for a check that
needs no configuration; a shell script, a second runner language with no
exit mapping on a generator that is not Make; a target in `ALL`, which
puts a host-dependent write on every build (FR-002).

## R-11: the committed-set check runs as a CTest entry

**Decision**: one `add_test` entry runs the reader with
`${CMAKE_SOURCE_DIR}` and `validation/`, registered like the `example_drift`
entry of `test/CMakeLists.txt`.

**Rationale**: `test`, `test-rocky`, `coverage`, `sanitize` and `tsan`
run CTest, so the committed records are validated on every job with no
workflow edit and no hardware assumption (FR-024, FR-026, ruling R-9).

**Alternatives rejected**: a dedicated CI job, a workflow edit and a new
place for the same rule; a label-only test that the quick tier skips,
which leaves the records unchecked on the fast path.

## R-12: no clock in the document

**Decision**: the record carries no wall-clock instant, and no rule
fires by calendar (FR-033, D-8).

**Rationale**: `test/time_source_gate.sh` bans the clock calls across
`source/`, `include/`, `example/`, `test/` and `tools/`, the harness
reads no wall clock today, and a calendar rule would fail CI on a date
alone, against ruling R-9.

**Alternatives rejected**: a timestamp field plus an age rule, which
breaks CI on a calendar and needs a clock the gate forbids; a timestamp
that nothing reads, data that decays and explains nothing.

## R-13: the root page carries the target, the build page carries the detail

**Decision**: `README.md` `## Quick start` gains 25 lines or fewer whose
block quotes region `#validate` of `example/quickstart.sh`;
`docs/pages/building.md` `## Quality gates` gains the section with the
record shape, the staleness rules and the exits.

**Rationale**: Principle IV.1 makes a root-page block an obligation for
user-visible behavior, and spec 030 is the precedent for a tooling change that
adds no public name: it carried its CI tiers in `## Quick start` in 25 lines or
fewer (its FR-050). That section already quotes shell regions from
`example/quickstart.sh` ( `library`, `consumer`, `options`, `tiers`) , so the
drift tool keeps one owner per block without a new example program, and no
`Name` or `Option` row joins, because both tables resolve against things this
feature does not add.

**Alternatives rejected**: docs pages only, which returns "no main-page
section" for a command a contributor runs, against IV.1 and the 030
precedent; a new `How you do it` section, whose owner must be a built
benchmark and whose 25-line cap the tool enforces per H3; a new
benchmark example to own a root-page block, a program built to satisfy a
docs gate.
