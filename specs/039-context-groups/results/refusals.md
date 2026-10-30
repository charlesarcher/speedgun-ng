# The refusal set, eighteen cases

- Unit: specs/039-context-groups T037, the refusal set recorded in one
  record (FR-038, SC-004).
- Cases: the sixteen fault rows of table C of
  [contracts/group-layout.md](../contracts/group-layout.md), in the table's
  own order, then the two pairings FR-019 names: a file beside a preset
  that names a partition, and a file beside `--hunt` (FR-018, FR-019).
- Runs: eighteen host runs, the CLI on the lane host. A refusal lands
  before any instance runs, so no case prints a row. Cases 01 to 16 and
  case 18 decide on their document and their option pair, so those bytes
  stand the same on any host. Case 17 reads this host's shape, because
  the pairing turns on the partition `core` names for the read in hand
  (FR-019, D-2).
- Tree: `build-w7a`, the release developer-mode tree the driver reads,
  built from this branch's sources.
- Driver: [run_groups.sh](./run_groups.sh). Its transcript for these
  eighteen runs stands at `reports/refusals.log`, and the pattern `*.log`
  stands ignored at `.gitignore:35`, so the record below carries the
  transcript's own lines and the captures stand committed beside it.
- Count: eighteen cases, eighteen one-line answers, eighteen exit codes
  of 2, eighteen absent report files: 18 of 18 (SC-004).

## The shape every case shares

The driver runs one command per case: eight contexts, one iteration, the
case's own report path and the case's own layout document, with one extra
option for cases 17 and 18. The documents stand under
`reports/fixtures/`, the driver rewrites them at the top of every pass,
and the bytes that refused stand committed with this record. The capture
of each case's answer stands at
`reports/refusals/<case>.txt`, named per case in the transcript block
below.

## The transcript, whole

The eighteen runs as `reports/refusals.log` spells them, in the order the
driver ran them:

```
$ build-w7a/example/docs_threads --threads=8 --iterations=1 --no-percentiles --json=specs/039-context-groups/results/reports/refusals/01-unreadable-path.json --group-layout=specs/039-context-groups/results/reports/fixtures/unreadable-path.yml
exit 2
the line stands captured in specs/039-context-groups/results/reports/refusals/01-unreadable-path.txt
$ build-w7a/example/docs_threads --threads=8 --iterations=1 --no-percentiles --json=specs/039-context-groups/results/reports/refusals/02-unparsable-document.json --group-layout=specs/039-context-groups/results/reports/fixtures/unparsable-document.yml
exit 2
the line stands captured in specs/039-context-groups/results/reports/refusals/02-unparsable-document.txt
$ build-w7a/example/docs_threads --threads=8 --iterations=1 --no-percentiles --json=specs/039-context-groups/results/reports/refusals/03-no-groups-key.json --group-layout=specs/039-context-groups/results/reports/fixtures/no-groups-key.yml
exit 2
the line stands captured in specs/039-context-groups/results/reports/refusals/03-no-groups-key.txt
$ build-w7a/example/docs_threads --threads=8 --iterations=1 --no-percentiles --json=specs/039-context-groups/results/reports/refusals/04-root-sequence.json --group-layout=specs/039-context-groups/results/reports/fixtures/root-sequence.yml
exit 2
the line stands captured in specs/039-context-groups/results/reports/refusals/04-root-sequence.txt
$ build-w7a/example/docs_threads --threads=8 --iterations=1 --no-percentiles --json=specs/039-context-groups/results/reports/refusals/05-empty-groups.json --group-layout=specs/039-context-groups/results/reports/fixtures/empty-groups.yml
exit 2
the line stands captured in specs/039-context-groups/results/reports/refusals/05-empty-groups.txt
$ build-w7a/example/docs_threads --threads=8 --iterations=1 --no-percentiles --json=specs/039-context-groups/results/reports/refusals/06-entry-not-mapping.json --group-layout=specs/039-context-groups/results/reports/fixtures/entry-not-mapping.yml
exit 2
the line stands captured in specs/039-context-groups/results/reports/refusals/06-entry-not-mapping.txt
$ build-w7a/example/docs_threads --threads=8 --iterations=1 --no-percentiles --json=specs/039-context-groups/results/reports/refusals/07-no-slots-key.json --group-layout=specs/039-context-groups/results/reports/fixtures/no-slots-key.yml
exit 2
the line stands captured in specs/039-context-groups/results/reports/refusals/07-no-slots-key.txt
$ build-w7a/example/docs_threads --threads=8 --iterations=1 --no-percentiles --json=specs/039-context-groups/results/reports/refusals/08-empty-slots.json --group-layout=specs/039-context-groups/results/reports/fixtures/empty-slots.yml
exit 2
the line stands captured in specs/039-context-groups/results/reports/refusals/08-empty-slots.txt
$ build-w7a/example/docs_threads --threads=8 --iterations=1 --no-percentiles --json=specs/039-context-groups/results/reports/refusals/09-member-wrong-kind.json --group-layout=specs/039-context-groups/results/reports/fixtures/member-wrong-kind.yml
exit 2
the line stands captured in specs/039-context-groups/results/reports/refusals/09-member-wrong-kind.txt
$ build-w7a/example/docs_threads --threads=8 --iterations=1 --no-percentiles --json=specs/039-context-groups/results/reports/refusals/10-member-out-of-range.json --group-layout=specs/039-context-groups/results/reports/fixtures/member-out-of-range.yml
exit 2
the line stands captured in specs/039-context-groups/results/reports/refusals/10-member-out-of-range.txt
$ build-w7a/example/docs_threads --threads=8 --iterations=1 --no-percentiles --json=specs/039-context-groups/results/reports/refusals/11-repeated-slot.json --group-layout=specs/039-context-groups/results/reports/fixtures/repeated-slot.yml
exit 2
the line stands captured in specs/039-context-groups/results/reports/refusals/11-repeated-slot.txt
$ build-w7a/example/docs_threads --threads=8 --iterations=1 --no-percentiles --json=specs/039-context-groups/results/reports/refusals/12-uncovered-context.json --group-layout=specs/039-context-groups/results/reports/fixtures/uncovered-context.yml
exit 2
the line stands captured in specs/039-context-groups/results/reports/refusals/12-uncovered-context.txt
$ build-w7a/example/docs_threads --threads=8 --iterations=1 --no-percentiles --json=specs/039-context-groups/results/reports/refusals/13-cpus-key.json --group-layout=specs/039-context-groups/results/reports/fixtures/cpus-key.yml
exit 2
the line stands captured in specs/039-context-groups/results/reports/refusals/13-cpus-key.txt
$ build-w7a/example/docs_threads --threads=8 --iterations=1 --no-percentiles --json=specs/039-context-groups/results/reports/refusals/14-key-outside-grammar.json --group-layout=specs/039-context-groups/results/reports/fixtures/key-outside-grammar.yml
exit 2
the line stands captured in specs/039-context-groups/results/reports/refusals/14-key-outside-grammar.txt
$ build-w7a/example/docs_threads --threads=8 --iterations=1 --no-percentiles --json=specs/039-context-groups/results/reports/refusals/15-repeated-name.json --group-layout=specs/039-context-groups/results/reports/fixtures/repeated-name.yml
exit 2
the line stands captured in specs/039-context-groups/results/reports/refusals/15-repeated-name.txt
$ build-w7a/example/docs_threads --threads=8 --iterations=1 --no-percentiles --json=specs/039-context-groups/results/reports/refusals/16-name-outside-grammar.json --group-layout=specs/039-context-groups/results/reports/fixtures/name-outside-grammar.yml
exit 2
the line stands captured in specs/039-context-groups/results/reports/refusals/16-name-outside-grammar.txt
$ build-w7a/example/docs_threads --threads=8 --iterations=1 --no-percentiles --json=specs/039-context-groups/results/reports/refusals/17-beside-preset.json --group-layout=specs/039-context-groups/results/reports/fixtures/four-groups.yml --placement=core
exit 2
the line stands captured in specs/039-context-groups/results/reports/refusals/17-beside-preset.txt
$ build-w7a/example/docs_threads --threads=8 --iterations=1 --no-percentiles --json=specs/039-context-groups/results/reports/refusals/18-beside-hunt.json --group-layout=specs/039-context-groups/results/reports/fixtures/four-groups.yml --hunt=all
exit 2
the line stands captured in specs/039-context-groups/results/reports/refusals/18-beside-hunt.txt
```

## The eighteen lines

Each line stands whole in the capture named for its case, and each case
reads one row of the contract's fault table. Cases 01 to 17 name the
layout path, and a line that names one entry names that entry by its
`name` where it has one, by its zero-based index where it has none
(FR-021, A-3). Case 18 names the two option names and carries no path,
the reading the pairings section records.

| case | the row it reads | the one line | status | report file |
| --- | --- | --- | --- | --- |
| `01-unreadable-path` | table C row 1, an unreadable path (FR-018) | `group layout specs/039-context-groups/results/reports/fixtures/unreadable-path.yml: cannot open the file` | 2 | none |
| `02-unparsable-document` | table C row 2, a document the reader rejects (FR-018) | `group layout specs/039-context-groups/results/reports/fixtures/unparsable-document.yml: the reader refuses the document: illegal block entry` | 2 | none |
| `03-no-groups-key` | table C row 3, a root mapping with no `groups` key (FR-018) | `group layout specs/039-context-groups/results/reports/fixtures/no-groups-key.yml: the root holds no 'groups' sequence` | 2 | none |
| `04-root-sequence` | table C row 4, a root node that is a sequence (FR-018) | `group layout specs/039-context-groups/results/reports/fixtures/root-sequence.yml: the root holds no 'groups' sequence` | 2 | none |
| `05-empty-groups` | table C row 5, an empty `groups` sequence (FR-018) | `group layout specs/039-context-groups/results/reports/fixtures/empty-groups.yml: the 'groups' sequence stands empty` | 2 | none |
| `06-entry-not-mapping` | table C row 6, an entry that is no mapping (FR-018) | `group layout specs/039-context-groups/results/reports/fixtures/entry-not-mapping.yml: entry 0 is no mapping` | 2 | none |
| `07-no-slots-key` | table C row 7, an entry with no `slots` key (FR-018) | `group layout specs/039-context-groups/results/reports/fixtures/no-slots-key.yml: entry 'first' holds no 'slots' sequence` | 2 | none |
| `08-empty-slots` | table C row 8, an empty `slots` sequence (FR-018) | `group layout specs/039-context-groups/results/reports/fixtures/empty-slots.yml: entry 'first' holds an empty 'slots' sequence` | 2 | none |
| `09-member-wrong-kind` | table C row 9, a `slots` member of the wrong kind (FR-018) | `group layout specs/039-context-groups/results/reports/fixtures/member-wrong-kind.yml: entry 'first' holds a 'slots' member that is no context position` | 2 | none |
| `10-member-out-of-range` | table C row 10, a member outside the context range (FR-018, FR-013) | `group layout specs/039-context-groups/results/reports/fixtures/member-out-of-range.yml: entry 'first' names context 9, outside the run's 8 contexts` | 2 | none |
| `11-repeated-slot` | table C row 11, a repeated slot (FR-018, FR-013) | `group layout specs/039-context-groups/results/reports/fixtures/repeated-slot.yml: context 7 stands named by two entries` | 2 | none |
| `12-uncovered-context` | table C row 12, a context no entry names (FR-018, FR-013) | `group layout specs/039-context-groups/results/reports/fixtures/uncovered-context.yml: context 7 stands unnamed by every entry` | 2 | none |
| `13-cpus-key` | table C row 13, the key `cpus` (FR-014, D-6) | `group layout specs/039-context-groups/results/reports/fixtures/cpus-key.yml: entry 'first' carries the key 'cpus'; this release groups by slots` | 2 | none |
| `14-key-outside-grammar` | table C row 14, a key outside the grammar (FR-017) | `group layout specs/039-context-groups/results/reports/fixtures/key-outside-grammar.yml: entry 'first' carries the key 'nodes', outside the grammar` | 2 | none |
| `15-repeated-name` | table C row 15, a repeated group name (FR-015) | `group layout specs/039-context-groups/results/reports/fixtures/repeated-name.yml: the name 'first' stands repeated` | 2 | none |
| `16-name-outside-grammar` | table C row 16, a `name` outside the characters or the length (FR-015) | `group layout specs/039-context-groups/results/reports/fixtures/name-outside-grammar.yml: entry 0 carries the name 'bad name', outside the allowed characters and length` | 2 | none |
| `17-beside-preset` | FR-019, a file beside a preset that names a partition (D-2) | `--group-layout 'specs/039-context-groups/results/reports/fixtures/four-groups.yml' cannot combine with --placement='core'` | 2 | none |
| `18-beside-hunt` | FR-019, a file beside `--hunt` | `--hunt cannot combine with --group-layout` | 2 | none |

Case 01 reads the open fault, so its document stands at a fixture name no
pass writes. Rows 3 and 4 answer one line text, and each row names the
path, which is what the contract states for both.

## The readings the set shares

- Status: eighteen lines of the transcript read `exit 2`, and a count
  over the file answers 18.
- Report file: eighteen lines read `no report file stands written`, and a
  count over the file answers 18. The driver checks for the file after
  the run, so the line records the run's own work. The directory holds
  the eighteen captures and no report:

  ```
  $ ls specs/039-context-groups/results/reports/refusals | wc -l
  18
  $ find specs/039-context-groups/results/reports/refusals -name '*.json' | wc -l
  0
  ```

- One line: each capture holds one line, so the version line, the row and
  the context disclosure a run prints stand absent from all eighteen
  captures. The refusal came before any instance ran, which is the stop
  FR-018 names.

  ```
  $ grep -l -e bmSharded -e 'contexts=' specs/039-context-groups/results/reports/refusals/*.txt | wc -l
  0
  ```

## The two pairings

Case 17 reads the pairing FR-019 settles on the partition a preset names
for the read in hand. The document is the accepted `four-groups.yml`
above, so the pairing alone decides: `--placement=core` names the host's
two L3 groups as a partition, and one run carries one partition source
(D-2, FR-003). The line names both the file and the preset value.

Case 18 reads the pairing with `--hunt`, the shape
`--hunt cannot combine with --placement` keeps
(`source/harness/hunt_cli.cpp:153-155`). Its line names both option names
and carries no layout path, while FR-021 states every refusal line names
the layout path. The record quotes the bytes as they stand and leaves the
reading of the two clauses to the verdict record (T050).

## The two streams

The driver captures standard output and standard error into one file, so
these captures prove one line over the two streams. The separation of the
streams stands asserted by the named test: `harness_group_cli_test`
redirects the two apart through `freopen` and reads them apart
(`test/source/harness_group_cli_test.cpp:635-678`), and its cases state
the refusal line on standard error with the benchmark absent from
standard output (`test/source/harness_group_cli_test.cpp:680-698`,
`test/source/harness_group_cli_test.cpp:818-835`).

## The re-run

```
$ bash specs/039-context-groups/results/run_groups.sh
```

The pass answers exit 0 when every case holds, and rewrites the eighteen
captures and the fifteen fault documents beside the accepted file in
place (T033).
