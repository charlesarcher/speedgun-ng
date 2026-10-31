# Contract: the record reader

Phase 1 output. The check side of the feature (spec FR-024 to FR-030,
D-3). The library entry points do the work, one thin program runs them,
and the lines and the exit status are the whole answer.

## Entry points

```cpp
// source/harness/detail/host_record_check.hpp
enum class RecordVerdict { VALID, BROKEN, STALE };

// The schema, relative to whatever tree root the run names.
inline constexpr std::string_view kHostRecordSchemaRelPath =
    "docs/schema/host-record-v1.json";

struct RecordReading {
  std::string cpuType;      // cpu.key, else the entry name
  RecordVerdict verdict {};
  std::string detail;       // the rule that fired, empty when VALID
};

enum class DirectoryAnswer { FOUND, NO_ROW, UNREADABLE_MAPFILE };

struct DirectoryChoice {
  DirectoryAnswer answer;
  std::string directory;    // the selected path when FOUND, empty otherwise
};

using DirectoryChooser = std::function<DirectoryChoice(const HostIdent&)>;

using DirectoryChooserFactory =
    std::function<DirectoryChooser(const std::filesystem::path&)>;

[[nodiscard]] auto readHostRecord(const std::filesystem::path& file,
                                  const std::filesystem::path& schemaPath)
    -> std::optional<HostRecord>;

[[nodiscard]] auto staleReasons(const HostRecord& record,
                                const std::filesystem::path& treeRoot,
                                const DirectoryChooser& chooser)
    -> std::vector<std::string>;

struct RecordCheck {
  std::vector<RecordReading> readings;  // one per .json entry, none on refusal
  std::optional<std::string> refusal;   // the run-level path, set at most once
};

// schemaPath resolves under treeRoot.
[[nodiscard]] auto checkRecords(const std::filesystem::path& recordRoot,
                                const std::filesystem::path& treeRoot,
                                const std::filesystem::path& schemaPath,
                                const DirectoryChooser& chooser) -> RecordCheck;

auto hostRecordCheckMain(int argc, char** argv,
                         const DirectoryChooserFactory& factory) -> int;
```

- `\pre` `hostRecordCheckMain`: `factory` answers a chooser for the tree
  root it names, the programs handing over the builder that opens
  `<treeRoot>/external/pmu-events/arch/x86/mapfile.csv` at the chooser's
  first question, because the harness library may not include a counters
  internal header (plan R-06, R-04).
- `readHostRecord` answers no record for three causes and asks no
  readability check ahead of it: a file the reader cannot open, a
  document the schema refuses, and a document that holds no value for a
  key the record needs. The reading of such an entry carries the
  `BROKEN` verdict with its reason beside the `host-record` label, the
  guard's own words or the name of the key the read could not answer,
  whichever reported it. The third cause answers at every contract
  semantic, the ignore one of `speedgun-ng_CONTRACTS` included, where no
  contract line stands between the read and the answer (FR-021, FR-030).
- `\pre` `staleReasons`: the record passed the schema, so its fields
  carry values. It reads files under `treeRoot` and asks `chooser` one
  question, whose `NO_ROW` answer is rule 4's reason. An
  `UNREADABLE_MAPFILE` answer reaches no per-record reason.
- `\pre` `checkRecords`: `treeRoot` and `recordRoot` stand absolute,
  `schemaPath` arrives resolved from the same root as
  `treeRoot / kHostRecordSchemaRelPath` so the two paths cannot disagree,
  and a readable schema at that path stands on the caller (FR-020).
  Nothing here asks the root to hold an entry. A schema check that fails
  for any reason, a read error included
  (`source/harness/json_load.cpp:2365-2368`), answers through the one
  mapping that turns a failed check into the `BROKEN` reading, so
  `host_record_check.cpp` holds no line apart for that case and
  `hostRecordCheckMain` stays the only party that refuses on the schema
  path (FR-004, FR-020).
- `\post` every returned reading names a CPU type, and a `VALID` reading
  carries an empty `detail`. The reading takes the CPU type from
  `cpu.key`, and from the entry's own name with the `.json` suffix
  removed where the document yields no key, because a planted key-set
  break can drop that key.
- `\post` a chooser answer of `UNREADABLE_MAPFILE` sets `refusal` to the
  mapfile path and leaves `readings` empty, so the empty set and the
  unanswered rule stay two different answers.
- `\invariant` `checkRecords` owns the listing: the `.json` entries
  directly under `recordRoot` and no other entry there, read through the
  error-code form of `std::filesystem::directory_iterator`, the shape of
  `source/counters/linux_pmu/provider.cpp:361`, so a root that
  stands absent and a root that holds no `.json` entry arrive as the same
  empty listing and share one path. Both answer
  `RecordCheck { {}, {} }`, no readings and no refusal, and
  `hostRecordCheckMain` turns that answer into the line and the status
  FR-029 names (D-10).
- `\invariant` `checkRecords` opens no mapfile of its own and asks the
  chooser it is given only per record, so one open stands behind a run
  that has records and no open stands behind one that has none
  (plan R-06).
- `\invariant` the schema path reaches the reader through no option: no
  `--schema` stands on either CLI, and the derivation from the tree root
  answers it (FR-004, FR-020).
- `\invariant` no entry point reads a clock, a cpu, `/proc` or `/sys`
  (FR-026, FR-033).

## The staleness rules, in the order the reader asks

| # | Rule | Detail the line names |
| --- | --- | --- |
| 1 | `recordVersion` lags the pinned schema's version | the two versions (FR-025) |
| 2 | an evidence `path` stands absent under the tree | that path (FR-025) |
| 3 | the cited `line` stands absent in that file | that path (FR-025) |
| 4 | `chooser` answers `NO_ROW` for the identity | the key and the identity (FR-025) |

The rules run in order and every cause the record carries stands named,
so one run reports all of them. Rule 4 needs no hardware: the chooser
selects through `pmuSelectDirectory(std::istream&, const PmuIdent&)`
(`source/counters/detail/pmu.hpp:99`,
`source/counters/linux_pmu/table_parse.cpp:695`), and a test injects its
own lambda and plants each answer. The cached overload
(`source/counters/linux_pmu/table_parse.cpp:747`) reads the bytes the
build embeds and opens no path, so it answers nothing about a planted
tree (plan R-06, AC-1).

## The three answers of the chooser

| Answer | What it answers |
| --- | --- |
| `FOUND` | rule 4's negation for that identity: the mapfile selects a directory for it in this tree, so the record reads no staleness from the routing. `directory` carries that selection, the path the writer recorded beside the same key, and the reader compares it against nothing (FR-008, FR-025) |
| `NO_ROW` | rule 4 for that record: `stale`, the key and the identity named, exit 2 (FR-025) |
| `UNREADABLE_MAPFILE` | the run: one refusal line naming the mapfile path, exit 2, and no per-record line, because rule 4 stands unanswered for every record (FR-004, FR-025) |

The reader compares no directory of its own. A record whose stored
`cpu.directory` no longer matches what the tree selects reads `valid`,
because the routing moved and the record states what was true when the
writer wrote it, and the identity fields the record carries are what a
reviewer reads against the tree (FR-024, FR-025).

The chooser a factory builds opens the mapfile at the first question and
reads the same rows for the rest, so a tree that holds
no mapfile answers `UNREADABLE_MAPFILE` for every identity it is asked
about, a tree that holds one answers `FOUND` or `NO_ROW` per identity, and
a run that holds no record asks nothing. The two mapfile
scenarios of `quickstart.md` differ in exactly that way: a planted file
with no matching row answers `NO_ROW` and prints one stale line, a tree
without the file answers `UNREADABLE_MAPFILE` and prints the refusal.

## The lines

```text
validation/<key>.json  <key>  valid
validation/<key>.json  <key>  broken  host-record: <the rule and the key or size>
validation/<key>.json  <key>  stale  <the rule and the path or identity>
```

- The records are the `.json` entries directly under the record root.
  Every other entry there reads as nothing, and the lines stand in the
  file-name order of that set (FR-024, FR-029).
- The CPU-type column reads `cpu.key`, or the entry's own name with the
  `.json` suffix removed where the document yields no key (FR-024).
- A `.json` entry the reader cannot open yields one line naming it and
  exit 2 (FR-029).
- The verdict column carries one of the three words FR-030 names:
  `valid`, `broken`, `stale`.
- A record that breaks names the break and no staleness (FR-030). Its
  line quotes the document kind `host-record`, the label the schema file
  carries, as plain text beside what `checkReport` reported: the checker
  reads no kind keyword, so the label tells a reader which document the
  record read against (FR-019, FR-020, FR-022).
- `checkRecords` answers the readings and prints nothing;
  `hostRecordCheckMain` prints the lines, and it is the entry point that
  names a record root holding no `.json` entry.
- A record root that holds no `.json` entry prints one line naming the
  root and exits 2, the empty case of FR-029 (D-10).
- An `UNREADABLE_MAPFILE` answer prints the refusal line of the mapfile
  path and exits 2 before any per-record line, so the run names one path
  and no verdict (FR-004, FR-025).
- A schema absent under the tree root answers the run the same way, with
  one line naming `<root>/docs/schema/host-record-v1.json`: the guard
  stands unanswered, so no record can read `valid` or `broken` (FR-004,
  FR-020).

## Exits

| Case | Status | Whose line |
| --- | --- | --- |
| every record valid | 0 (FR-027) | `checkRecords` readings, printed by `hostRecordCheckMain` |
| any record broken or stale | 2 (FR-027) | `checkRecords` readings, printed by `hostRecordCheckMain` |
| the record root holds no `.json` entry | 2 (FR-029) | `checkRecords` answers the empty listing, `hostRecordCheckMain` prints the root |
| unknown option, missing value, extra argument | 2, one line on standard error (FR-004) | `hostRecordCheckMain` at its parse |
| the record root or the tree root stands absent | 2, one line on standard error | `hostRecordCheckMain`: the record root through the empty listing, the tree root through the schema refusal |
| a `.json` entry cannot be opened | 2, one line on standard error (FR-029) | `checkRecords` sets `refusal` to that entry, `hostRecordCheckMain` prints it |
| the mapfile stands unreadable | 2, one refusal line naming it, no per-record line (FR-004, FR-025) | the chooser answers, `checkRecords` sets `refusal`, `hostRecordCheckMain` prints it |
| the schema stands absent under the tree root | 2, one refusal line naming it, no per-record line (FR-004, FR-020) | `hostRecordCheckMain` refuses before the call |

The cases answer in a fixed order. `hostRecordCheckMain` parses, resolves
`treeRoot / kHostRecordSchemaRelPath`, refuses there when that file cannot
be read, then calls `checkRecords` and maps what comes back: a set
carrying `refusal` prints that line, a set with neither readings nor
refusal prints the one line FR-029 names for the record root, and any
other set prints one line per reading. A record root that stands absent
and one that holds no `.json` entry share that line, because the listing
answers both the same way. A tree root that stands absent arrives through the
schema case, whose line names the schema path under it, and a tree that holds
neither the schema nor the mapfile answers that same refusal, the schema case
standing first. The chooser opens the mapfile at the first question a record
asks, so a run whose record set stands empty never opens it and prints the
empty-root line, while the `quickstart.md` scenario that holds no mapfile
plants a record, so its line names the mapfile (FR-004, FR-020, FR-029).

## The program

`tools/hostcheck/host_record_check.cpp` is the whole program: its `main` parses
nothing and maps the status of `hostRecordCheckMain` to the exit. That entry
point works from the tree root from `--root`, the record root from `--records`,
the schema at `treeRoot / kHostRecordSchemaRelPath`, and the chooser factory it
is handed. It asks that factory for the chooser built from the parsed tree root
and passes that chooser to `checkRecords`, so one root answers the tree, the
schema and the mapfile question, and a caller that hands over an absolute root
needs no start directory of its own (FR-020, FR-028). The reader program
supplies the factory member `makeDirectoryChooser` and shares the mapfile opener
with the writer program through a header in the same directory (R-04). An
`UNREADABLE_MAPFILE` answer or an absent schema carries the run: one refusal
line naming that path, exit 2, and no per-record line, because rule 4 and the
schema guard stand unanswered for every record. The `host_record_check` CTest
entry runs this program over the committed records and reads files alone
(FR-026, FR-037).

## Refusals

The refusal shape is the standing one: one `fprintf` line on standard
error and status 2, the form of `rejectHuntValue`
(`source/harness/hunt_cli.cpp:41`) and of the `--topology` refusal
(`source/harness/cli.cpp:670`). The line starts `error: `, names the path
quoted by `detail::quotedText`, and names the reason. The standing form
calls `std::exit(2)` itself, while these entry points answer the status
and leave the exit to `main` (FR-004, FR-011, R-09).

Three sentences carry the run-level answers, taken from the `fprintf` calls of
`hostRecordCheckMain` in `source/harness/host_record_check.cpp`. Each names
one path in the quotes `detail::quotedText` answers, and each stands alone on
standard error with status 2, so no column shape stands beside it:

```text
error: the record schema '<path>' cannot be read
error: the record run cannot read '<path>'
error: the record root '<path>' holds no record
```

- The schema sentence carries a tree root that holds no schema at
  `<root>/docs/schema/host-record-v1.json`, the guard standing unanswered
  (FR-020).
- The run sentence carries the path a refusal names: a `.json` entry the
  reader cannot open, or the mapfile behind `UNREADABLE_MAPFILE`. The
  readings a listing gathered before that entry belong to no line, because
  one refusal answers alone (FR-029).
- The root sentence carries a record root that holds no `.json` entry, and
  names the root (FR-029, D-10).

The library entry points answer a status: `readHostRecord`, `checkRecords`
and `hostRecordCheckMain` return what they found, and `main` in
`tools/hostcheck/host_record_check.cpp` turns a refused status into the
process exit of 2. One reading therefore reaches the terminal as one line,
and the exit status follows from it (FR-027, FR-029).
