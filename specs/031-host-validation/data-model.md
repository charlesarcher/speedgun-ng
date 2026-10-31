# Data Model: Host Validation

Phase 1 output of `/speckit.plan`. No standing entity changes shape: the
record is a new document beside the report and the compare document
(`specs/023-json-reports/data-model.md`), and the machine facts it
carries read from the standing `MachineLayout`
(`source/harness/detail/internal.hpp:106`). Each entity names its
fields, its validation rules and its owner. No entity is public
(FR-040).

## Entity map

```text
HostIdent (new, internal, plain data)     ── written by the program
HostMachine (new, internal, plain data)   ── written by hostMachineFromLayout
                                             over MachineLayout (standing)
EvidenceRow (new, internal)               ── authored, kept by a refresh
HostRecord (new, JSON)  ┌─ cpu: HostIdent
                        ├─ machine: HostMachine
                        └─ evidence: [EvidenceRow]     the document the
                                                       schema governs
RecordSchema (new, docs/schema/host-record-v1.json)    guards HostRecord
Verdict (new, internal, one per record read)
  ├─ kind: VALID | BROKEN | STALE
  ├─ detail: string (empty when VALID)
  └─ exitStatus: 0 | 2                    the reader's answer
DirectoryChoice (new, internal, one per identity asked)
  ├─ answer: FOUND | NO_ROW | UNREADABLE_MAPFILE
  └─ directory: string (the selected path when FOUND)
```

## E-21: `HostIdent`

The CPU identity, as the detection renders it and as the record names
it. Values in, no host read inside the library (R-04).

- Fields: `vendor` (the leaf 0 vendor string, `GenuineIntel` or
  `AuthenticAMD` on the parts the catalog names), `family`, `model` and
  `stepping` (`null` where the host answers no leaf 1 for some parts,
  the key standing present either way), `key` (the record key
  `mapfileKey` renders, the CPU type: the model key `mapfileModelKey`
  plus the stepping segment, the segment a host leaves out when it
  answers no leaf 1), `directory` (what `pmuSelectDirectory` selects,
  empty when the mapfile names nothing).
- Validation: `vendor` non-empty and `family` set, else the identity is
  unnamed and FR-010 refuses; `key` equals
  `<vendor>-<family>-<MODELhex>[-<steppinghex>]` and the record file name
  adds the `.json` suffix (FR-009); `directory` empty refuses, because a
  record for a CPU the catalog cannot name proves nothing (FR-008,
  FR-010).
- Owner: `source/harness/detail/host_record.hpp`; the program fills it
  from `pmuIdentCurrent()` and `mapfileKey`, and answers the directory
  through `pmuSelectDirectory(std::istream&, const PmuIdent&)` over the
  mapfile under the run's root
  (`tools/hostcheck/host_record_write.cpp`).

## E-22: `HostMachine`

The machine shape of the run, the same facts `--topology` prints.

- Fields: `packages`, `l3Groups`, `cores`, `threads` (counts, at least
  1 each), `mask` (the affinity mask text, the `appendRangeText`
  spelling of `topology.cpp:529`), `l3` (one row per L3 group: ordinal
  and its cpu range text), `nodes` (the node rows the listing prints, one
  per node the mask leaves in the read's node order, each its number and
  cpu range text, or `null`, the key standing present either way).
- Validation: the counts equal the installed `MachineLayout`'s, so the
  listing and the record agree by construction (FR-012, FR-014);
  `nodes` stands `null` exactly where `memoryReadable` stands false, the
  JSON spelling of the standing `node: not available` line (FR-013); a
  node the mask leaves out stands no row, the listing's own reading
  (`topology.cpp:591`), so a masked run records the nodes its mask
  reaches; no field names a cpu count the process does not hold (FR-034).
- Owner: `source/harness/host_record.cpp`, through
  `hostMachineFromLayout(const MachineLayout&)`.

## E-23: `EvidenceRow`

One recorded run the host record points at. Authored by hand, kept by a
refresh (R-08).

- Fields: `path` (repository-relative, the file that holds the run),
  `command` (the command line that produced it), `exit` (the status that
  run returned), `line` (one line copied from that file, the anchor the
  reader looks for).
- Validation: `path` stands inside the tree and `line` stands in that
  file, else the record reads stale (FR-025); `command` and `line`
  non-empty; the row cites, and no row carries a figure the cited file
  already holds (FR-018, D-2).
- Owner: `source/harness/detail/host_record.hpp`; the authoring belongs
  to the results records of the cited features (FR-016, FR-017).

## E-24: `HostRecord`

The document: one per CPU type under `validation/`, named after E-21's
`key` with the `.json` suffix.

- Fields, in the schema order: `recordVersion` (the format version the
  writer speaks, `"1.0"` at first), `cpu` (E-21: vendor, family, model,
  stepping, the key, and the selected directory, so the reader can name
  the mapping it checked), `machine` (E-22), `evidence` (a list of E-23,
  possibly empty, FR-041).
- Validation: the key set, order and kinds equal the pinned schema's, no
  key stands unknown and every key of a closed object stands present, so
  an unanswered value reads `null` (FR-013, FR-020, FR-021); the text
  stays within the 16 KiB cap (FR-018); the bytes carry no wall-clock
  instant (FR-033); the bytes carry no authored host name (FR-042); a
  refresh replaces `cpu` and `machine` and keeps `evidence` (FR-006).
- Owner: `source/harness/host_record.cpp` writes,
  `source/harness/host_record_check.cpp` reads.

## E-25: the pinned record schema

`docs/schema/host-record-v1.json`: the document kind as a `title` label,
the format version, the key sets, the normative `x-order`, the kinds, the
closed objects, and, on the two nullable keys, a `type` list naming their
kind and `null` (`cpu.stepping`, `machine.nodes`).

- Validation: the kind label reaches no comparison, because the record
  names its format version while the file names the kind (FR-019); the
  checker reads the key set, `x-order`, kinds, enums,
  closed objects, array items and root duplicate keys, and reads no
  other keyword (the rule `specs/023-json-reports/contracts/schema-file.md`
  sets and `checkReport` implements); the schema's key set and order
  equal the writer's call order, pinned by a test (FR-021).
- Owner: `docs/schema/`, guarded by
  `test/source/harness_host_record_schema_test.cpp`.

## E-26: `Verdict`

The reader's answer for one record, and the line the check prints.

- Fields: `kind` (`VALID`, `BROKEN`, `STALE`, printed as the three words
  FR-030 names), `detail` (the rule that fired and the path or key it fired
  on, empty under `VALID`),
  `exitStatus` (0 for an all-valid run, 2 when any record breaks or
  stales, when a record cannot be opened, when the record root stands
  absent or holds no `.json` entry, when the schema stands absent under
  the tree root, or when the mapfile stands unreadable).
- Validation: one record yields one line naming the CPU type, the kind
  and the detail (FR-024); the CPU type reads `cpu.key`, or the entry's
  own name with the `.json` suffix removed where the document yields no
  key; a broken record reports no staleness (FR-030); the exit status
  follows the lines alone, so the run reports every record it read, and a
  run-level refusal answers 2 with its own line standing alone (FR-027,
  FR-004).
- Owner: `source/harness/host_record_check.cpp`.

## E-27: `DirectoryChoice`

The answer to one question: which catalog directory the mapfile under the
run's tree root holds for one identity. One open per run, one answer per
identity asked (plan R-06).

- Fields: `answer` (`FOUND`, `NO_ROW`, `UNREADABLE_MAPFILE`), `directory`
  (the selected path under `FOUND`, empty otherwise).
- Validation: `FOUND` answers rule 4 with its negation for the identity asked,
  the reader comparing the carried path against nothing and the record keeping
  the directory the writer wrote (FR-008, FR-025); `NO_ROW` answers staleness
  rule 4 for the record asked, `stale` with the key and the identity named
  (FR-025); `UNREADABLE_MAPFILE` answers the run with one refusal line naming
  the mapfile path and exit 2, and no per-record line stands beside it, because
  rule 4 cannot be answered for any record (FR-004, FR-025).
- Owner: the factory `hostRecordCheckMain` is handed, whose member
  `makeDirectoryChooser` stands in `tools/hostcheck/host_record_check.cpp`
  beside the mapfile opener the two programs share under `tools/hostcheck/`,
  the harness library holding no counters internal include (R-04); a test
  installs its own factory and plants each answer (FR-025, AC-1).

## State transitions

The writing path:

```text
validate
  └─ read identity
       ├─ vendor empty or family unset ... one line, exit 2, no file
       ├─ directory empty ................ one line, exit 2, no file
       └─ named
            └─ read layout
                 ├─ read failed .......... one line, exit 2, no file
                 └─ ok
                      └─ standing record?
                           ├─ unreadable ... one line, exit 2, bytes kept
                           ├─ present ...... keep evidence, replace the rest
                           └─ absent ....... fresh
                                └─ render → size cap?
                                             ├─ over ..... one line, exit 2
                                             └─ within ... one write, one line, exit 0
```

The chooser path, the question each record asks:

```text
the run answers its two refusals, then the chooser answers
├─ schema absent ................ one line naming the schema, exit 2, no record line
├─ record root absent or empty .. one line naming it, exit 2, no record line
└─ both stand: the chooser opens the mapfile at its first question
   ├─ UNREADABLE_MAPFILE ........ one line naming the mapfile, exit 2, no record line
   ├─ NO_ROW .................... that record reads stale under rule 4
   └─ FOUND ..................... that record reads no staleness from it
```

The reading path, in the order the entry point answers:

```text
the entry point answers the two run-level cases, then reads the records
├─ the schema absent under the tree root .......  one line, exit 2 (FR-020)
├─ the record root absent or holds no .json ....  one line, exit 2 (FR-029)
└─ the records, .json entries directly under the record root
   ├─ one entry that cannot be opened ..........  one line, exit 2 (FR-029)
   ├─ one record file
   │   └─ parse → checkReport(schema, bytes)
   │        ├─ key set, order, kind or size disagree .............  broken,  exit 2
   │        └─ ok
   │             └─ staleReasons(record, treeRoot, chooser)
   │                  ├─ format version behind the schema ........  stale,  exit 2
   │                  ├─ evidence path absent ....................  stale,  exit 2
   │                  ├─ cited line absent .......................  stale,  exit 2
   │                  ├─ chooser answers NO_ROW ..................  stale,  exit 2
   │                  └─ none ....................................  valid,  exit 0
   └─ every other entry there ..................  reads as nothing (FR-029)
```

## What is deliberately absent

- No public type and no public function: the target and the reader are
  the surface (FR-040, research R-04).
- No wall-clock instant, no run identifier, no host name, no person name
  and no `perf_event_paranoid` value in the record: the cited run output
  holds those (FR-018, FR-033, FR-042).
- No second identity source, no second topology reader, no second mapfile
  matcher, no JSON reader beside simdjson (FR-007, FR-012, FR-022,
  FR-025).
- No run database, no history and no per-run log: one record per CPU
  type, refreshed in place (D-1, D-7).
- No machine-readable staleness file, no badge, no status page: the
  reader's lines and exit status are the answer (P-3, FR-024).
