# Implementation Plan: Host Validation

**Branch**: `031-host-validation` | **Date**: 2026-10-29 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `specs/031-host-validation/spec.md`

## Summary

The harness already answers "which CPU is this": `pmuIdentCurrent()`
reads the ident (`source/counters/linux_pmu/table_parse.cpp:55`),
`mapfileKey` renders the key the catalog matches
(`source/counters/detail/pmu.hpp:77`), `pmuSelectDirectory` names the
directory, and `printTopologyListing` prints the machine shape
(`source/harness/topology.cpp:551`). Nothing writes those answers down.
This feature adds one internal library pair,
`source/harness/detail/host_record.hpp` and
`source/harness/host_record.cpp`, that turns the two blocks plus an
evidence list into a JSON document guarded by a pinned schema, a second
pair `host_record_check.{hpp,cpp}` that reads the committed documents
back and names the stale ones, two thin programs under
`tools/hostcheck/` that hand the live host facts to the entry points they
call, the way `test/source/counters_linux_pmu_seam_test.cpp:1860` reads
the ident for a test, one `validate` target, one pinned schema
(`docs/schema/host-record-v1.json`) and one committed record per CPU
type under `validation/`.

## Technical Context

**Language/Version**: C++23, `CMAKE_CXX_EXTENSIONS=OFF`
(`CMakePresets.json`, `ci-std`).

**Primary dependencies**: the `speedgun-ng::speedgun-ng` counters
library (the CPU ident, the key renderers, the mapfile selection) and
the `speedgun-ng::harness` library (the machine layout, the JSON writer
pair, `checkReport`). simdjson stays the only JSON reader, hwloc stays
the only topology reader. No new dependency.

**Storage**: plain files. `docs/schema/host-record-v1.json` (pinned
schema), `validation/<key>.json` (one committed record per CPU type),
and an output root an argument selects.

**Testing**: CTest through `test/CMakeLists.txt`; TDD waves (Principle
III). The suite gains five entries: three test executables over the
library pairs and two entries over the programs. The reader entry
validates the committed records from files alone and registers after the
lane commits the record it reads, so no commit of the branch leaves
`ctest` red; the writer entry runs one malformed-argument invocation and
pins its refusal line with `PASS_REGULAR_EXPRESSION`, the shape the
`counters_trap_checked_rejects_unknown_mode` row sets in
`test/CMakeLists.txt`. Each entry point stands in
`source/harness/detail/`, parses its arguments before any host read, asks the
injected seam for what it reads of the host, the writer's `HostFactsSeam` and
the reader's `DirectoryChooserFactory` over the parsed root, and answers a
status its thin `main` maps, so the usage arms of both
programs, the four staleness causes, the run-level refusals, the
empty-listing line and the arms that read a host stand asserted in-process
(FR-004, FR-025, FR-029, FR-037).

**Target Platform**: Linux on GCC and Clang; the record names x86
CPUIDs because the detection is x86 (the ident renders empty elsewhere
and the refusal of FR-010 answers it).

**Project Type**: library plus developer-mode tooling; no service, no
daemon, no network.

**Performance Goals**: none. The feature reads the host once, writes a
file of at most 16 KiB, the figure `kHostRecordCapBytes` names in
`source/harness/detail/host_record.hpp`, and exits. It joins no critical
path, so Principle VII adds no baseline.

**Constraints**: 100% line and branch coverage over `source/` (the
`COVERAGE_EXTRACT_COMMAND` row of `cmake/coverage.cmake`); no `LCOV_EXCL`
marker and no new
`--ignore-errors` entry joins this change, and no allowance row leaves the
gate; no sleep, no clock, no fixed cpu number; no
public name; no option row; no version or `SOVERSION` move; the
`time_source_gate` scan covers every `.cpp` and `.hpp` this feature
adds under `tools/`.

**Scale/Scope**: one record per CPU type, at most a handful of records,
one schema file, two programs, four new files under `source/`, two
under `tools/`, one under `cmake/`.

**Execution mode**: TDD (Principle III). Each wave stands written and
observed failing before the code task that turns it green exists, and
`Test Plan` records the waves and the gate each runs.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1
design.*

| Principle | Gate | Status |
| --- | --- | --- |
| I. Standard-first | P3 baseline only: the pinned Core Guidelines through the `ci-ubuntu` launchers. No P0 designation, no P2 exception. | PASS |
| II. DBC | Every new interface documents `\pre`, `\post`, `\invariant` in `source/harness/detail/host_record.hpp` and `host_record_check.hpp` and enforces the same at the site; a malformed record is a read failure, a violated precondition aborts. | PASS |
| III. R-DCUT | Interfaces, then tests, then code; the UML views below; the test plan names its gate. | PASS |
| IV | Self-documenting names, no magic values, doxygen on each interface, no TODO. | PASS |
| IV.1 | `## Quick start` gains 25 lines or fewer carrying one marked shell region of `example/quickstart.sh`, the shape that section already carries for `#library` and `#consumer`; no `Name` or `Option` row joins, because no public name and no option joins (FR-036, D-6). | PASS |
| V | `.clang-format` clean; naming N-1 to N-12; file names `host_record.cpp`, `host_record_check.cpp`, `host_record_write.cpp`. | PASS |
| VI | Tests ship with each component; every file this feature adds under `source/` reads 100.0% lines and 100.0% branches, and each program keeps a thin `main` outside those roots while the entry point it calls stands inside them; the record's meaning is a measurement claim, so the round trip and the staleness rules carry the tests. | PASS |
| VII | No critical path, so no new baseline; the feature runs once per host, outside any measured loop. | PASS |
| VIII | Every hard gate stands; the feature adds five CTest entries, three test executables and two program entries, so `test`, `test-rocky`, `coverage`, `sanitize` and `tsan` run them with no workflow edit. | PASS |
| IX | Four artifacts, TDD recorded, one release-preset build per feature. | PASS |
| X | Minimum shape: no run database, no second CPUID reader, no second topology reader, no wall clock, no service; every changed line traces to an FR. | PASS |
| XI | Prose rules hold in every file this plan names, and the prose gate runs over each commit. | PASS |

**Gate weakening requested**: no. **Gate strengthening requested**: no.
No gate fails, so **Complexity Tracking stays empty**.

## Project Structure

### Documentation (this feature)

```text
specs/031-host-validation/
├── spec.md                       # requirements, decisions, success criteria
├── plan.md                       # this file
├── research.md                   # R-01 to R-13: the readings this plan fixes
├── data-model.md                 # E-21 to E-27: record, schema, verdict, choice
├── quickstart.md                 # the scenarios a reviewer runs, one block each
├── contracts/
│   ├── host-record-json.md       # the record document: keys, order, kinds, nulls
│   ├── record-schema-file.md     # the pinned schema and what the guard asserts
│   ├── validate-target.md        # the target, its knobs, its output line, its exits
│   └── record-check.md           # the reader: verdicts, one line shape, staleness rules
├── checklists/requirements.md    # spec quality review (already recorded)
└── results/
    ├── plan.md                   # the evidence plan: host shape, records, commands
    ├── quickstart.md             # every quickstart scenario, exit, observation
    ├── taskset.md                # the `taskset -c 0,1` and 2-cpu runs
    └── ci-run.md                 # the pushed-head CI verdict
```

### Source (this feature)

```text
docs/schema/host-record-v1.json        # pinned schema, normative x-order (FR-020)
README.md                              # + the quick-start block of FR-036
example/quickstart.sh                  # + region #validate the block quotes
docs/pages/building.md                 # + ### Validating the host record (FR-036)
validation/                            # committed records, one per CPU type (FR-001)
└── <key>.json

source/harness/detail/host_record.hpp  # HostIdent, HostMachine, EvidenceRow,
                                       # HostRecord, kHostRecordVersion,
                                       # hostRecordWriteMain (II)
source/harness/host_record.cpp         # the writer: parse, key order, cap, merge (FR-004, FR-006..FR-019)
source/harness/detail/host_record_check.hpp  # verdict type, reader entry points (II)
source/harness/host_record_check.cpp   # reader: schema, staleness, refusal, statuses (FR-024..FR-030)
tools/hostcheck/host_record_write.cpp  # thin main: facts seam, map status to exit (FR-004, FR-007)
tools/hostcheck/host_record_check.cpp  # thin main and the real chooser, handed over as a factory (R-04, FR-025, FR-028)
cmake/validate.cmake                   # script mode: run the two programs, map exits
cmake/dev-mode.cmake                   # + add_custom_target(validate) outside ALL (FR-001)
test/CMakeLists.txt                    # + 5 entries: 3 test TUs, 2 program entries

test/source/harness_host_record_test.cpp        # writer, facts, evidence, refresh, cap
test/source/harness_host_record_schema_test.cpp # pinned schema, planted breaks, round trip
test/source/harness_host_record_check_test.cpp  # verdicts, staleness causes, no entry
```

**Structure Decision**: the document logic lives in the harness library,
where the coverage gate measures it (its `COVERAGE_EXTRACT_COMMAND` row
extracts `source/*` from `cmake/coverage.cmake`) and where `checkReport`, the
`MachineLayout` already stand. The two programs under `tools/hostcheck/`
stay thin: they read the live host and hand values over, the way
`test/source/counters_linux_pmu_seam_test.cpp:1860` reads the ident, and
`:48` reaches the counters detail header with a relative include. No
target gains an include edge, no library grows a public name, and
`tools/**.cpp` stays outside the coverage extract by the standing globs,
so the plan states its uncovered lines exactly: the two `main` functions
stand outside the measured roots,
and their CTest entries carry them as their evidence (FR-037, SC-008).

## Design

### Decision index

- **R-01: one library pair per side of the document** (FR-001, FR-024).
  `host_record.{hpp,cpp}` writes, `host_record_check.{hpp,cpp}` reads.
  Two files let the writer wave and the reader wave land in parallel.
- **R-02: the document is written through the standing writer pair**
  (FR-020, FR-023). `compare.cpp:765` and `:794` are the precedent for a
  second top-level document over
  `simdjson::builder::string_builder`; key order is call order, so the
  schema's `x-order` and the call order are one fact kept twice and
  pinned by the guard test of FR-021.
- **R-03: `checkReport` validates the record unchanged** (FR-020,
  FR-021). It is generic: `harness_compare_cli_test.cpp:969` already
  reads a second schema through it. No second checker joins.
- **R-04: the identity enters as values** (FR-007, FR-008). `HostIdent`
  carries vendor, family, model, stepping, the rendered key and the
  selected directory as plain data. The program reads the first four from
  `pmuIdentCurrent()` and renders the key with `mapfileKey`; the
  directory answers for the mapfile under the root the run names,
  `pmuSelectDirectory(std::istream&, const PmuIdent&)`
  (`source/counters/detail/pmu.hpp:99`,
  `source/counters/linux_pmu/table_parse.cpp:695`) over
  `<root>/external/pmu-events/arch/x86/mapfile.csv`, so the directory the
  record stores is the selection this tree's mapfile gave at the time of
  the write; R-06 states why the stream overload
  stands on both sides. The library never calls CPUID and
  reads no host, so no second reader joins and every rule stays testable
  on a host the catalog cannot name.
- **R-05: the machine block comes from `MachineLayout`** (FR-012,
  FR-014). `hostMachineFromLayout(const MachineLayout&)` in the library
  renders the counts, the mask and the rows through the same range
  spelling `appendRangeText` uses (`topology.cpp:529`), so the listing
  and the record agree by construction. `nodes` is `null` where
  `memoryReadable` stands false, the JSON spelling of the standing
  `node: not available` line.
- **R-06: the staleness question arrives as a chooser of three answers**
  (FR-008, FR-025, FR-028). The five declarations stand as
  `contracts/record-check.md:32-56` writes them, the seam R-04 asks for:

  ```cpp
  using DirectoryChooserFactory =
      std::function<DirectoryChooser(const std::filesystem::path&)>;

  [[nodiscard]] auto readHostRecord(const std::filesystem::path& file,
                                    const std::filesystem::path& schemaPath)
      -> std::optional<HostRecord>;

  [[nodiscard]] auto staleReasons(const HostRecord& record,
                                  const std::filesystem::path& treeRoot,
                                  const DirectoryChooser& chooser)
      -> std::vector<std::string>;

  [[nodiscard]] auto checkRecords(const std::filesystem::path& recordRoot,
                                  const std::filesystem::path& treeRoot,
                                  const std::filesystem::path& schemaPath,
                                  const DirectoryChooser& chooser) -> RecordCheck;

  auto hostRecordCheckMain(int argc, char** argv,
                           const DirectoryChooserFactory& factory) -> int;
  ```

  `staleReasons` takes the chooser as a `DirectoryChooser`, and the
  `DirectoryChoice` it answers carries a path in `directory` only where
  `answer` reads `DirectoryAnswer::FOUND`. The program's factory,
  `makeDirectoryChooser(treeRoot)` defined in
  `tools/hostcheck/host_record_check.cpp`, returns a chooser that opens
  `<treeRoot>/external/pmu-events/arch/x86/mapfile.csv` at the first
  question a record asks, reads those rows once, and answers every later
  identity from that one open through
  `pmuSelectDirectory(std::istream&, const PmuIdent&)`
  (`source/counters/detail/pmu.hpp:99`,
  `source/counters/linux_pmu/table_parse.cpp:695`). R-04 keeps that include in
  the program, so the entry point reaches the chooser through its factory and
  a test plants a factory answering any of the three answers.
  `DirectoryAnswer::FOUND` answers that this identity selects a directory in
  this tree, and `staleReasons` takes the answer only as the negation of rule
  4: it compares no path, so the
  `directory` field stands in the record as the writer wrote it beside the
  same key. A record whose stored directory no longer matches the tree
  therefore reads valid, because the routing moved and the record states
  what was true when it was written, and a reviewer reads the record's own
  identity fields against the tree. `DirectoryAnswer::NO_ROW` answers rule 4
  for that record, and `DirectoryAnswer::UNREADABLE_MAPFILE` answers the run:
  one refusal line naming the mapfile, status 2, and no per-record line,
  because rule 4 stands unanswered for every record. Rows the matcher cannot
  interpret answer that same refusal, the one catch in
  `tools/hostcheck/mapfile.hpp` standing before a throw could reach `main`.
  A test injects its own lambda and plants each answer. The stream overload stands on both sides of
  the record,
  because the cached overload
  (`source/counters/linux_pmu/table_parse.cpp:747`) reads the mapfile
  bytes the build embeds and opens no path, so it cannot answer for a
  planted tree. No hardware read, and no second matcher.
- **R-07: verdicts stay three, `broken` wins over `stale`** (FR-030). A
  record that fails the schema reports that verdict alone, because a
  staleness reading over a document whose keys are wrong describes
  nothing.
- **R-08: the refresh merge keeps what the run cannot derive** (FR-006).
  The writer reads a standing record, keeps its `evidence` array, and
  replaces `cpu` and `machine`. A record that cannot be read
  stands refused and keeps its bytes (FR-011).
- **R-09: the refusal reuses the standing shape** (FR-004, FR-010,
  FR-011). One `fprintf` line on standard error, then the status 2 that
  the thin `main` maps to the process exit: the library answers and the
  program ends, the way `rejectHuntValue` of
  `source/harness/hunt_cli.cpp` and the `--topology` refusal
  (`cli.cpp:670`) print one line and end. No record file is created before
  every read succeeds, so a failed run leaves no partial file: the
  writer builds the whole text in memory, then writes once.
- **R-10: the target is the house pattern** (FR-001, FR-002).
  `add_custom_target(validate DEPENDS <the two programs> COMMAND
  cmake -D VALIDATE_ROOT=... -P cmake/validate.cmake)` beside the other
  utility targets of `cmake/dev-mode.cmake`, never in `ALL`; the
  script maps program statuses the way `cmake/prose-lint.cmake:40` does
  and takes `VALIDATE_ROOT` and `VALIDATE_OUT` knobs.
- **R-11: the check runs as a CTest entry** (FR-024, FR-026). An
  `add_test` entry over `${CMAKE_SOURCE_DIR}` and
  `${CMAKE_SOURCE_DIR}/validation` reads the committed set in `test`,
  `test-rocky`, `coverage`, `sanitize` and `tsan` with no workflow edit,
  the `example_drift` shape (`add_test(NAME example_drift ...)`), which also
  passes `${CMAKE_SOURCE_DIR}`. The entry sets no `WORKING_DIRECTORY`:
  CTest starts it in the test binary's directory, `build/test`, and that
  property stands in none of the 144 entries of `test/CMakeLists.txt`, so
  the absolute root carries the schema path and the record root wherever
  the run starts.
  The entry registers after the lane commits its record, because the
  mapping prints the empty-listing line and answers status 2 (FR-029).
- **R-12: no clock in the document** (FR-033, D-8). No wall-clock field
  stands: the harness reads no wall clock today, the counters-only
  constraint binds the read, and a rule that fires by calendar breaks
  ruling R-9. Freshness reads content alone.
- **R-13: the root page carries the target, the build page carries the
  detail** (FR-036). `## Quick start` gains 25 lines or fewer quoting
  region `#validate` of `example/quickstart.sh`. Spec 030 named its CI
  tiers in `## Quick start` and kept its command block on
  `docs/pages/prepush.md:13`, where the `tiers` region of
  `example/quickstart.sh:27-33` carries it. This feature carries its
  commands in a marked shell region of its own, the shape of that
  `tiers` region, and its detail stands on `docs/pages/building.md`.
  No quick-reference row joins.

### Risks

- A committed record could stand for a CPU the pinned mapfile drops
  later. R-06 turns that into a named stale
  verdict, which is the feature working.
- The lane host reaches neither acceptance host. FR-041 records the gap
  and `results/plan.md` names the run each waiting row stands on. The
  lane commits its own record with an empty evidence list: a row binds
  when its record is written, so authoring a row for a host that never
  ran would make the record a claim no host made (FR-016, FR-017, SC-011).
  Whether the two Zen 5 hosts render one key or two stays open, because
  no record holds their idents and rows `:46` and `:47` of
  `external/pmu-events/arch/x86/mapfile.csv` spread family 26 over two
  directories: row `:46` matches the models it names and row `:47` sends
  the rest to `amdzen6`. Two idents that differ keep two records; two
  that match share one record whose machine block describes the host that
  wrote last (D-7, FR-006).
- A `validate` run on a developer machine rewrites a record while a
  pull request is open. R-08 keeps the evidence rows, and the reader
  stays green, so the diff is the machine block alone; the record is
  data a reviewer reads.
- The 300 s CTest budget could catch a program entry on a slow runner.
  Both programs read a handful of files; the standing label and timeout
  loops over the directory `TESTS` property in `test/CMakeLists.txt` stay
  untouched, and the `test_tier_labels` row audits the registration.

### Modules and dependency order

```text
M-A ──► { M-B  M-C  M-G } ──► { M-D  M-E } ──► M-F ──► M-H
        (parallel)             (parallel)
```

The `depends on` clauses of [tasks.md](./tasks.md) drive the order, and the
task ids group work by story, so a clause can name a higher number: T001
follows T002, and the Phase 8 measures follow the two clears T055 and T059.

- **M-A: schema and model.** `docs/schema/host-record-v1.json`,
  `source/harness/detail/host_record.hpp` and
  `detail/host_record_check.hpp` (types and contracts only),
  `contracts/host-record-json.md`, `contracts/record-schema-file.md`,
  `data-model.md`. The schema carries a `required` array on every object
  it describes: the root, `cpu`, `machine`, each `machine.l3[]` row, each
  `machine.nodes[]` row and each `evidence[]` row, because the guard
  reports an absent key only through that list (the `node.required` loop
  of `checkNode` in `source/harness/json_load.cpp`). Covers FR-019, FR-020,
  FR-021 through the schema file and the two headers. Public surface:
  none. Test: none yet, the headers carry no code path; the schema test
  unit registers in T001, once its source stands written. Depends on:
  nothing.
- **M-B: the writer.** `source/harness/host_record.cpp`, whose entry point
  `hostRecordWriteMain(argc, argv, facts)` stands declared in
  `detail/host_record.hpp` with `facts` a `HostFactsSeam` of that header.
  Covers FR-003, FR-004, FR-005, FR-006, FR-009, FR-012, FR-013, FR-015 to
  FR-019, FR-023, FR-037. Test: `harness_host_record_test`. Depends on:
  M-A.
- **M-C: the reader.** `source/harness/host_record_check.cpp`, whose
  header declares `RecordCheck { readings, refusal }`, `DirectoryAnswer`,
  `DirectoryChoice`, `DirectoryChooser`, `DirectoryChooserFactory`,
  `kHostRecordSchemaRelPath`, `checkRecords` and
  `hostRecordCheckMain(argc, argv, factory)`. A
  `refusal` carries the one run-level path, so an empty record set and an
  unanswered rule stay two answers. The constant takes the `k` prefix an
  exported `inline constexpr std::string_view` in a `detail/` header
  carries, as `kDefaultCompareRule` stands in
  `source/harness/detail/compare.hpp`, and it names
  `docs/schema/host-record-v1.json` inside any tree, so the reader resolves
  the schema under the root it is given and no `--schema` option reaches
  either CLI. Covers FR-022, FR-024 to FR-027, FR-029, FR-030;
  FR-028 belongs to the two programs below. Test:
  `harness_host_record_check_test`. Depends on: M-A; it reads the bytes M-B
  writes, so the round-trip assertion lands with the wave that holds both.
- **M-D: the two programs.** `tools/hostcheck/host_record_write.cpp`,
  `tools/hostcheck/host_record_check.cpp`. Covers FR-004, FR-007,
  FR-008, FR-010, FR-011, FR-014, FR-028. Tests: the writer's one
  malformed-argument entry of `test/CMakeLists.txt`, which reads no host,
  the writer's usage arms of `harness_host_record_test`, the reader's usage
  arms of `harness_host_record_check_test`, and the refusal arms reached
  through the two entry points with planted values. Each thin main takes
  `argc` and `argv` alone, parses nothing, and maps the status its entry
  point answered to the exit its run carries. No `--schema` option joins
  either CLI. Depends on: M-B, M-C.
- **M-E: the build surface.** `cmake/validate.cmake`, the target in
  `cmake/dev-mode.cmake`, the five `test/CMakeLists.txt` entries, three
  test executables and two program entries, and `FORMAT_PATTERNS` if a
  new directory needs it. Covers FR-001, FR-002, FR-037's registration
  half. Depends on: M-D.
- **M-F: the committed set and the evidence.** `validation/<key>.json`
  from the lane host, whose evidence list stands empty, the waiting rows
  of FR-016 and FR-017 named as gaps in
  `specs/031-host-validation/results/plan.md`, and the records under that
  directory; a CPU type that owns no acceptance row carries none, stated
  there in one line. Covers FR-016, FR-017, FR-041, FR-042. Depends on:
  M-E.
- **M-G: the docs.** `example/quickstart.sh` region `#validate`, the
  `## Quick start` block that quotes it, and the `docs/pages/building.md`
  section; then the drift, snippet, run and report-shape gates. Covers
  FR-036. Depends on: M-A's wording, so it runs beside M-B and M-C.
- **M-H: the gate sweep.** Coverage, both sanitizer builds, the thread
  sanitizer, the `ci-ubuntu` analyzer build, `taskset -c 0,1`, the
  2-cpu reading, `prose-lint`, `spell-check`, the time-source scan, the
  source scan for a file root read outside `procRootPath`, and the
  keep-set report of FR-038. Covers FR-031 to FR-035, FR-038, FR-039,
  FR-040. Depends on: M-F.

### Logical view: one `validate` run

```text
cmake --build build --target validate
└─ cmake -D VALIDATE_ROOT=<tree> -D VALIDATE_OUT=<tree>/validation
   -P cmake/validate.cmake
   ├─ tools/hostcheck/host_record_write          thin main, no parse
   │   └─ hostRecordWriteMain(argc, argv, facts)
   │       ├─ parse --root and --out             before any host read (FR-002)
   │       │                                     no --out → <root>/validation
   │       ├─ facts ident: pmuIdentCurrent()     counters, one CPUID read
   │       ├─ mapfileKey(ident)                  record key, name adds .json
   │       ├─ pmuSelectDirectory(mapfile, ident) "" → refusal, status 2 (R-06)
   │       │                                   uninterpretable rows answer
   │       │                                   the same, no throw escapes
   │       ├─ ident.directory under the root   absent → refusal, status 2
   │       ├─ facts machine: layout()            MachineLayout, hwloc once
   │       ├─ read the standing record           absent → fresh
   │       │                                   unreadable → refusal, status 2
   │       ├─ merge: cpu + machine replace, evidence keeps    (R-08)
   │       ├─ render in x-order, size check                   (FR-018, FR-023)
   │       ├─ write once                                        (R-09)
   │       └─ one line: the CPU type and the path, status 0
   └─ tools/hostcheck/host_record_check   thin main, no parse, real chooser
       └─ hostRecordCheckMain(argc, argv, factory)
           ├─ parse --root and --records             before any read (FR-028)
           ├─ resolve root/docs/schema/host-record-v1.json, refuse there
           │   when unreadable, before the call      (R-03, FR-004, FR-020)
           ├─ factory(root) → makeDirectoryChooser   one open, first ask (R-06)
           ├─ checkRecords → RecordCheck{readings, refusal}  the listing
           ├─ each record: checkReport(schema, bytes)       broken? (FR-020)
           ├─ else staleReasons(record, treeRoot, chooser)  stale?  (FR-025)
           ├─ one line per record: type, verdict, detail
           ├─ map the answer: a refusal prints its line and answers 2; neither
               readings nor refusal prints FR-029's line naming the root,
               status 2, and no per-record line stands beside either
                           (FR-004, FR-024, FR-025, FR-027, FR-029)

One run prints the record line, then one line per committed record
(FR-001, FR-005, FR-024), and a failing step fails the target. Each thin
main maps the status its entry point answered to the exit the target reads,
so the same refusal answers a developer and the gate (FR-004, FR-037).

### Logical view: the verdict decision

```text
record bytes
├─ schema (key set, x-order, kinds, closed object) ── mismatch → broken
└─ ok
   ├─ recordVersion behind the schema         → stale: format lag
   ├─ evidence path absent from the tree      → stale: absent path
   ├─ cited line absent from that file        → stale: moved line
   ├─ chooser answers DirectoryAnswer::NO_ROW → stale: no mapping
   └─ none of these                           → valid
```

A chooser answering `DirectoryAnswer::UNREADABLE_MAPFILE` answers the first
record and ends the run before these rules reach a second: one refusal line
naming the mapfile, status 2, and no per-record line, because rule 4 stands
unanswered for every record (R-06).

### Physical view

The harness library gains two translation units and two private headers,
so `source/harness/*.cpp` keeps its glob row in `CMakeLists.txt` and
nothing links differently. simdjson stays a `PRIVATE` link edge of the
harness target, and the purity scan's rule (no
`simdjson` token under `include/`, no `find_package(simdjson)` in any
build file) holds untouched. The two programs link
`speedgun-ng::harness` and `speedgun-ng::speedgun-ng` and reach the two
internal headers by relative include, the pattern
`test/source/counters_linux_pmu_seam_test.cpp:48` sets. Their
`add_executable` lines stand in `test/CMakeLists.txt`, the pattern that
file already sets for `../tools/quill/quill_dependency_check.cpp`, the
`quill_dependency_check` target, so a build artifact of the two programs lands
under `build/test/`, where the developer build puts every target, while
the record alone stands under the output root (FR-003). The
`validate` target is developer-mode only, beside `format-check`, `spell-check` and
`prose-lint` in `cmake/dev-mode.cmake`, filed under `UtilityTargets` by
`cmake/folders.cmake`.

### Docs surface

`README.md` `## Quick start` gains one purpose sentence, one block
quoting `example/quickstart.sh#validate`, and one result sentence naming
both exits, in 25 lines or fewer. Spec 030 named its CI tiers in that
section and kept its command block on `docs/pages/prepush.md`; this
feature carries its commands in a marked shell region, the shape of the
`tiers` region of `example/quickstart.sh:27-33` that section already
uses. `docs/pages/building.md` `## Quality gates` gains
`### Validating the host record`: the record path shape, the four
staleness rules and the two exit statuses. No `Name` row and no `Option`
row joins either quick-reference table (FR-040, FR-004). `README.md`
stands at the `PAGE_LINE_CAP` it measures, 964 lines as `master` left
both, so the block moves that cap to the measured 974 and adds the dated
note beside it, the shape of the dated notes that stand above
`PAGE_LINE_CAP` in `tools/docs/example_drift.py` and that every earlier
root-page addition left.

### Public API surface added

| Name | Shape | Contract |
| --- | --- | --- |
| none | n/a | n/a |

No other public name joins. `include/speedgun-ng/` gains no file, no
declaration and no macro, so the quick-reference `Name` table gains no
row (FR-040, FR-036).

### Version and `SOVERSION`

`VERSION 0.27.0`, harness `SOVERSION 18` and archive `SOVERSION 4`, the three
hand-kept rows of `CMakeLists.txt`, stay as they stand: this branch moves no
public type, function or struct, so no version and no `SOVERSION` move applies
(FR-040, roadmap section 4). The report schema stays `1.10` at `kSchemaVersion`
in `source/harness/json_report.cpp`: the record is a separate document
with its own schema file, and `docs/schema/report-v1.json` gains nothing.

## Test Plan

Each wave is written first and observed failing against the standing
tree, then the code turns it green (Principle III). Every FR names its
test in the coverage map below.

1. **Wave 1, the pinned schema and the record model (FR-019, FR-020,
   FR-021).**
   `test/source/harness_host_record_schema_test.cpp` reads
   `docs/schema/host-record-v1.json` through the `SG_SCHEMA_PATH`
   compile definition, asserts the key set and `x-order` equal the
   writer's and the file names the kind `host-record` and the version
   `1.0` that `kHostRecordVersion` names in
   `source/harness/detail/host_record.hpp`, plants a missing key, an
   added key, a swapped order, a
   retyped value and a duplicate root key, and requires each planted
   document to fail the guard. A `required` array stands on every object
   the file describes: the root, `cpu`, `machine`, each `machine.l3[]`
   row, each `machine.nodes[]` row and each `evidence[]` row, because the
   guard reports an absent key only through that list (the
   `node.required` loop of `checkNode` in
   `source/harness/json_load.cpp`), where a closed object
   catches a key present and wrong. Gate:
   `ctest --test-dir build -R harness_host_record_schema_test`.
2. **Wave 2, the writer (FR-005, FR-006, FR-009, FR-012, FR-013, FR-015
   to FR-018, FR-023, FR-033).**
   `test/source/harness_host_record_test.cpp` runs
   `hostRecordWriteMain(argc, argv, facts)` with planted `HostIdent` and
   `HostMachine` values answered through the `HostFactsSeam` `facts` into a
   temporary root: the file name equals `cpu.key` with the
   `.json` suffix, the blocks hold the planted values, `nodes` reads `null`
   where memory is unreadable, the evidence list
   round-trips, a second write keeps the standing evidence, the byte-for-byte
   round trip through the reader holds, no field carries a wall-clock instant,
   and a write whose record would pass the size cap refuses with one line,
   status 2 and the standing bytes alone. Gate:
   `ctest --test-dir build -R harness_host_record_test`.
3. **Wave 3, the reader (FR-024 to FR-026, FR-029, FR-030).**
   `test/source/harness_host_record_check_test.cpp` drives `checkRecords`
   and `staleReasons` over planted trees: one valid record answers one
   valid reading and an empty `detail`; each planted break answers `broken`
   and names the record; each of the four staleness causes names the CPU
   type and the rule; `checkRecords` answers the empty listing, neither
   readings nor `refusal`, for a record root that stands absent and for one
   that holds no `.json` entry, one error-code directory read behind both,
   the shape `source/counters/linux_pmu/provider.cpp:361` sets; a `.json`
   entry that cannot be opened sets `refusal` to that entry; the test
   injects its chooser lambda and plants each answer, and each planted root
   holds a copy of the schema beside the record, whose bytes the test reads
   from the file `${SG_SCHEMA_PATH}` names, the define that target carries
   the way `harness_json_schema_test` carries it, because the reader
   resolves the schema inside the root it is given, so no host CPU and no
   repo path matters.
   Gate: `ctest --test-dir build -R harness_host_record_check_test`.
4. **Wave 4, the programs and the refusal shape (FR-001 to FR-004,
   FR-007, FR-008, FR-010, FR-011, FR-014, FR-027, FR-028).** Each entry point
   parses before it asks for a host fact, so the usage arms of both
   programs and the writer's refusal arms run in-process.
   `test/source/harness_host_record_test.cpp` drives
   `hostRecordWriteMain(argc, argv, facts)`: an unknown option, a missing
   value and an extra argument each print one line and answer status 2
   before `facts` is asked, and an ident that selects no directory, a
   failed layout read
   installed through `installTestLoadFailure` and a read-only output root
   each print one line and answer status 2 with planted values. The writer
   CTest entry runs the one arm that may run as a process, a
   malformed-argument invocation whose printed refusal line
   `PASS_REGULAR_EXPRESSION` pins, the property shape the
   `counters_trap_checked_rejects_unknown_mode` row carries; the file holds
   no `WILL_FAIL` row.
   CMake reads matching output as a pass whatever status the process exits
   with, so the failing status stands asserted in-process, where the
   library answers it. The reader's usage arms stand in
   `harness_host_record_check_test`, whose injected chooser answers
   `DirectoryAnswer::NO_ROW` for one record through `staleReasons`. Every
   printed line of a refused run belongs to the entry point, which owns
   resolution and mapping and takes its chooser from the
   `DirectoryChooserFactory` its test plants, so five runs plant five trees:
   no schema file under the root prints one line naming
   `<root>/docs/schema/host-record-v1.json` before the call, no mapfile
   prints one line naming that mapfile once the planted factory has answered
   `DirectoryAnswer::UNREADABLE_MAPFILE`, a record root that stands absent
   prints the FR-029 line naming the root, a record root that stands
   present and holds no `.json` entry prints that same line, and a `.json`
   entry the reader cannot open prints one line naming that entry. Each
   answers status 2 and prints no per-record line, and a failed open stands
   refused before the chooser stands asked. The chooser answers only per
   record and opens the mapfile at its first question, so neither empty-
   listing run opens it, and the quickstart scenario without a mapfile
   plants a record, so its line names the mapfile. A root that holds
   neither the schema nor the mapfile answers the schema refusal, because
   resolution comes first. `checkRecords` prints nothing in any of them.
   Over a set whose records all pass that entry answers status 0 with one
   line per record, the arm T026 carries. The reader's entry over the
   committed `validation/` set arrives with wave 5, registered with
   absolute arguments (FR-004, FR-025, FR-035, FR-037). Gate:
   `ctest --test-dir build -R 'host_record'`.
5. **Wave 5, the target and the committed set (FR-001, FR-002, FR-024,
   FR-026, FR-035).** The committed-set entry passes `${CMAKE_SOURCE_DIR}`
   and `${CMAKE_SOURCE_DIR}/validation` and sets no `WORKING_DIRECTORY`:
   CTest starts an entry in the test binary's directory, `build/test`, and
   `test/CMakeLists.txt` sets that property in none of its 144 entries, so
   the schema and the records resolve through the absolute root the entry
   is handed. `cmake --build build --target validate` against a
   `VALIDATE_OUT` under the build tree runs the write, then the check,
   writes one record, prints the record line and one line per committed
   record, and exits 0; a failing step fails the target. The entry over
   the committed `validation/` registers after the lane commits its
   record, so no commit of the branch leaves `ctest` red; it reads the
   real files, needs no PMU, no `/proc` and no `/sys`, and exits 0, and
   the live identity read of FR-007 stands in that run. Gate:
   `ctest --test-dir build -R host_record_check` and one `--target
   validate` run recorded in `results/quickstart.md`.
6. **Wave 6, host-shape neutrality (FR-031 to FR-034).** The suite runs
   under `taskset -c 0,1`; every read of a test resolves under an
   installed root or an installed layout; no test names a cpu number.
   Gate: the full `ctest --test-dir build` under `taskset -c 0,1`,
   recorded in `results/taskset.md`.
7. **Wave 7, the docs (FR-036).** The `## Quick start` addition counts
   25 lines or fewer, its block quotes region `#validate` of
   `example/quickstart.sh`, it names both commands and both exits, and
   the build-page section carries the record shape and the staleness
   rules. `README.md` and `PAGE_LINE_CAP` both stand at the
   964 lines `master` measured, so the same commit moves that cap to the
   measured 974 and adds the dated note beside it in
   `tools/docs/example_drift.py`. Gate: `example_drift`, `example_drift_fixtures`,
   `snippet_compile`, `example_run`, `report_shape`.
8. **Wave 8, the gates (FR-037 to FR-042).** Coverage runs from the
   instrumented tree of the `ci-coverage` preset and reads the figure
   `tools/dbc/coverage_gate.sh` reports for the measured roots: lines at
   100.0%, and any branch shortfall inside the standing allowance of 21
   rows, the figure at the `allowance` row of
   `tools/dbc/coverage_gate.sh` and the accepted classes at
   `specs/030-ci-speedup/spec.md:377-380` (D-14), the rows
   themselves read off the zero-taken branch positions of the trace,
   while every file this feature adds under `source/` reads 100.0% lines
   and 100.0% branches
   and the two program entry points carry their CTest runs as their
   evidence; the ledger holds the 2 rows this feature recorded beside the
   19 accepted classes, reading 21; ASan/UBSan and
   TSan run clean; the `ci-ubuntu` analyzer build reports no finding;
   `time_source_gate.sh` stays green over `tools/hostcheck/`;
   `prose-lint`, `spell-check` and `format-check` stay green; the keep-set
   list of FR-038 is written into `results/ci-run.md` with the trace, and
   no pattern or allowance row is removed. Gate: the pushed CI run read
   end to end.

### FR coverage map

| FR | Covering test (the wave that runs it) |
| --- | --- |
| FR-001 | the `--target validate` run: the write, then the check, one failing step failing the target (W5); the writer entry carries `host_record_write` (W4) and the committed-set entry carries `host_record_check` (W5) |
| FR-002 | the writing entry point run with a planted tree root and a temporary output root, and the `--target validate` run with `VALIDATE_OUT` under the build tree (W2, W5) |
| FR-003 | `harness_host_record_test`: the output root holds the record alone (W2); the two programs build under `build/test/` (W4) |
| FR-004 | the writer entry with one malformed argument; the usage arms of both programs in-process, `harness_host_record_test` through `hostRecordWriteMain(argc, argv, facts)` and `harness_host_record_check_test` through `hostRecordCheckMain`, where the planted run-level lines stand with them: the absent-schema refusal, the unreadable-mapfile refusal, the empty-listing line naming the record root, and the line naming an entry that cannot be opened (W4) |
| FR-005 | `harness_host_record_test`: one line, status 0, path named (W2) |
| FR-006 | `harness_host_record_test`: the refresh keeps `evidence`, replaces the rest (W2) |
| FR-007 | the planted-value arms in `harness_host_record_test`, and the identity block of the record the `--target validate` run writes against `pmuIdentCurrent()` (W2, W5) |
| FR-008 | `harness_host_record_test`: the directory field equals the path the `DirectoryAnswer::FOUND` answer of the chooser the seam builds carries (W2) |
| FR-009 | `harness_host_record_test`: the name of the written file equals the rendered record key with the `.json` suffix (W2); the record `T031` commits carries that name (W5) |
| FR-010 | `harness_host_record_test`: empty vendor, unset family, an unmapped ident and an ident whose `directory` names no catalog directory under the root each refuse with no file written (W4) |
| FR-011 | `installTestLoadFailure` arm, the read-only root arm, both refusing with no partial file; the lane plants mapfile rows the matcher cannot interpret and reads the same unreadable-mapfile refusal, status 2 (W4) |
| FR-012 | `harness_host_record_test`: counts and mask equal the installed layout's listing values (W2) |
| FR-013 | the `nodes: null` arm beside the readable arm (W2) |
| FR-014 | the same `MachineLayout` drives both the listing bytes and the record block (W2) |
| FR-015 | `harness_host_record_test`: evidence fields round-trip (W2) |
| FR-016 | `harness_host_record_check_test`: the committed lane record reads valid with an empty evidence list; `results/plan.md` names the hunter row and the host it waits on (W5, W8) |
| FR-017 | `results/plan.md` names the placement rows and the pre-check rows as gaps with the hosts that ran them, and the refresh arm of `harness_host_record_test` keeps authored rows standing (W2, W5, W8) |
| FR-018 | the size-cap refusal over a planted record, its bytes standing alone, and the no-figure assertion (W2) |
| FR-019 | `harness_host_record_schema_test`: `recordVersion` present and a string, and the schema file names the kind `host-record` (W1) |
| FR-020 | `harness_host_record_schema_test` over the pinned file (W1); `harness_host_record_check_test` over a planted root holding no schema file, whose run prints one refusal line naming it (W4) |
| FR-021 | the five planted breaks and the byte round trip (W1, W2) |
| FR-022 | the reader reaches simdjson only through `checkReport`; `simdjson_purity_scan` stays green (W8) |
| FR-023 | two writes of one planted host differ in no byte (W2) |
| FR-024 | `harness_host_record_check_test`: one line per record, named type (W3) |
| FR-025 | the four planted staleness causes, each naming its rule, the fourth planted as a `DirectoryAnswer::NO_ROW` chooser answer, and the `DirectoryAnswer::UNREADABLE_MAPFILE` answer that ends the run with one line naming the mapfile (W3, W4) |
| FR-026 | the committed-set entry reads files alone (W5); `time_source_gate` and the `/proc` and `/sys` absence asserted in W3, W8 |
| FR-027 | status 0 over a set whose records all pass, the arm T026 carries and wave 5 registers for the committed set, and status 2 over each planted offender, read through `hostRecordCheckMain` (W4) |
| FR-028 | the entry passes `${CMAKE_SOURCE_DIR}` and a temp root as arguments (W4, W5) |
| FR-029 | `checkRecords` answers the empty listing for a record root that stands absent and one that holds no `.json` entry, and sets `refusal` to a `.json` entry it cannot open (W3); the printed line naming the record root, for both empty cases, and the line naming that entry, status 2 with them, run through `hostRecordCheckMain` (W4) |
| FR-030 | a planted break with a planted stale cause reports the break alone (W3) |
| FR-031 | the file-root arm of the reader's evidence reads (W6); the scan of the added sources for an absolute `/proc` or `/sys` read outside `procRootPath` runs with the construct scans (W8) |
| FR-032 | the writer run on `installTestLayout` and `installTestTopologyDescription` values (W2) |
| FR-033 | no sleep, no clock in the new tests; the record holds no time field, asserted over a written document (W2, W8) |
| FR-034 | `results/taskset.md`: the suite green under `taskset -c 0,1` with an output root under the build directory; no literal cpu in the new sources (W6) |
| FR-035 | every test writes and reads under a temporary root (W2, W3, W4) |
| FR-036 | the docs gates of W7 over the root-page block, the build-page section and the moved `PAGE_LINE_CAP` |
| FR-037 | the `coverage` job and `tools/dbc/coverage_gate.sh`: lines at 100.0% over the measured roots with any branch shortfall inside the 21-row allowance, every file under `source/` at 100.0% on both metrics, and the two program entries run as CTest entries, while the parse and the usage arms of both entry points stand inside those roots (W4, W5, W8) |
| FR-038 | the keep-set list in `results/ci-run.md`, with the zero list as evidence, and no row removed before the owner answers (W8) |
| FR-039 | the `dbc-gate` job, the `static-analysis` job, and a `grep` that no `SG_*` macro carries a lambda in the new sources (W8) |
| FR-040 | `consumer-release`, `shared-audit` and the version lines unchanged in the diff (W8) |
| FR-041 | `validation/` holds the lane host's record with its evidence list empty; `results/plan.md` names each waiting row, its CPU type and the run it waits on (W5, W8) |
| FR-042 | `prose-lint` plus a names scan over the added files (W8) |

## Constitution Check

*Re-check after Phase 1 design.* Every row of the first table holds: the
two library pairs carry their contracts (II), the eight waves pair every
FR with an observable check (III, VI, X.4), the record document joins the
coverage extract with no exclusion (VI), no gate moves (VIII), the four
artifacts stand (IX), the design carries no speculative seam (X.2, the
one `std::function` of R-06 answers the planted-mapfile requirement of
FR-025 and AC-1), and prose rules bind every file named here (XI).
**PASS.**

## Complexity Tracking

> Empty: no constitution violation stands, so no justification row
> applies.
