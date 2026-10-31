# Contract: the host record document

Phase 1 output. The document one host writes to
`validation/<key>.json`, guarded by
`docs/schema/host-record-v1.json` (spec FR-001, FR-009, FR-019 to
FR-023). The shape follows the standing documents: the bytes come from
the writer's call order, the values come from the host and from nothing
else, and the file holds no date, no host name and no pid
(`source/harness/compare.cpp:1978-1994`).

## The document

The example is a fixture: its figures, its identity and its one evidence
row are the ones the writer tests plant. The record the lane commits
carries an empty evidence list (FR-041, SC-011).

`mapfileModelKey` renders the model key `<vendor>-<family>-<MODEL>` and
`mapfileKey` renders the record key, that key plus the one uppercase hex
digit of the stepping (`source/counters/linux_pmu/table_parse.cpp:673`,
`:684-692`). A host that answers no leaf 1 carries no stepping, so its
record key stands equal to its model key (FR-007, FR-009). The fixture's
ident answers leaf 1 at stepping 2, so its record key
`GenuineIntel-6-C6-2` carries one segment more than its model key
`GenuineIntel-6-C6`, and the file name reads
`GenuineIntel-6-C6-2.json`.

```json
{"recordVersion":"1.0","cpu":{"vendor":"GenuineIntel","family":6,"model":198,"stepping":2,"key":"GenuineIntel-6-C6-2","directory":"arch/x86/arrowlake/"},"machine":{"packages":1,"l3Groups":2,"cores":20,"threads":20,"mask":"0-19","l3":[{"ordinal":0,"cpus":"0-7"},{"ordinal":1,"cpus":"8-19"}],"nodes":[{"number":0,"cpus":"0-19"}]},"evidence":[{"path":"specs/034-host-noise-precheck/results/quickstart.md","command":"ctest --test-dir build -R harness_precheck_test --output-on-failure","exit":0,"line":"1/1 Test #1: harness_precheck_test ... Passed"}]}
```

## Keys

| Key | Kind | Absent as | Meaning | Source |
| --- | --- | --- | --- | --- |
| `recordVersion` | string | never | the format version the writer speaks | FR-019 |
| `cpu.vendor` | string | never | the leaf 0 vendor string | FR-007 |
| `cpu.family` | integer | never | the family the ident carries | FR-007 |
| `cpu.model` | integer | never | the model the ident carries | FR-007 |
| `cpu.stepping` | integer | `null` | the stepping, `null` where the host answers no leaf 1 | FR-007 |
| `cpu.key` | string | never | the record key `mapfileKey` renders, the CPU type; the file name adds `.json` | FR-007, FR-009 |
| `cpu.directory` | string | never | the catalog directory the mapfile selects | FR-008 |
| `machine.packages` | integer | never | package count | FR-012 |
| `machine.l3Groups` | integer | never | L3 group count | FR-012 |
| `machine.cores` | integer | never | core count of the process affinity | FR-012 |
| `machine.threads` | integer | never | thread count of the process affinity | FR-012 |
| `machine.mask` | string | never | the affinity mask, the `appendRangeText` spelling | FR-012 |
| `machine.l3` | array | never | one row per L3 group: `ordinal` (integer), `cpus` (string) | FR-012 |
| `machine.nodes` | array | `null` | the node rows the listing prints, one per node the mask leaves: `number` (integer), `cpus` (string) | FR-013 |
| `evidence` | array | never, empty allowed | the recorded runs, one row each | FR-015, FR-041 |
| `evidence[].path` | string | never | repository-relative file that holds the run | FR-015 |
| `evidence[].command` | string | never | the command line that produced it | FR-015 |
| `evidence[].exit` | integer | never | the status that run returned | FR-015 |
| `evidence[].line` | string | never | one line copied from that file, the anchor the reader looks for | FR-015, FR-025 |

## Rules

- The key order the table lists is normative, and the schema carries it
  as `x-order`; the writer's call order matches, and one test compares
  the two (FR-020, FR-021).
- Every object stands closed: an unknown key reads as a break, so a
  document a later writer grows cannot pass for a current one (FR-020).
- `machine.nodes` stands `null` exactly where the machine memory reads
  unavailable, the JSON spelling of the standing `node: not available`
  console line, so the schema states one kind per key (FR-013, FR-020).
- `machine.nodes` carries the node rows `--topology` prints: one per node
  the process mask leaves, in the read's node order, and a node the mask
  leaves out stands no row, the shape of
  `source/harness/topology.cpp:583-596`, where `:591` skips it. A masked
  run therefore records the nodes its mask reaches, and no row names a cpu
  outside it (FR-013, FR-034).
- The bytes hold no wall-clock instant, no run identifier and no
  authored host or person name (FR-018, FR-033, FR-042).
- The record stands under the output root, which with no `--out` resolves
  as `<root>/validation` under the tree root the run carries, so the
  directory that started the process names no path (FR-002, FR-028).
- The file name is `cpu.key`, the record key, with the `.json` suffix and
  nothing else, so one CPU type has one record. Two hosts that render one key
  share one record: it carries the evidence rows of both hosts, and its
  `machine` block describes the host that wrote last (FR-006, FR-009, D-7).
- `evidence` may stand empty. A row binds when its record is written, so
  the lane commits its own record with an empty list and each waiting row
  stands named as a gap in `specs/031-host-validation/results/plan.md`; a
  CPU type that owns no acceptance row carries none, stated there in one
  line (FR-016, FR-017, FR-041, SC-011).
- `cpu.directory` answers for the mapfile under the root the run names,
  `<root>/external/pmu-events/arch/x86/mapfile.csv`, read through the
  stream overload `pmuSelectDirectory(std::istream&, const PmuIdent&)`
  (`source/counters/detail/pmu.hpp:99`,
  `source/counters/linux_pmu/table_parse.cpp:695`), so one record and one
  tree stand checkable together (FR-008, FR-025, plan R-06).
- The identity and the machine block arrive through one seam,
  `using HostFactsSeam = std::function<HostFacts(const std::filesystem::path&)>`
  answering `HostFacts { ident, layout }` for the root the parse named, so
  the mapfile whose directory the identity reports stands under that root,
  a machine read that fails answers its own refusal before any render, and
  a test plants both facts on a runner that reads no host (FR-007, FR-011,
  FR-032, FR-035).
- `recordVersion` carries the text `1.0`, which `kHostRecordVersion` names once
  in `source/harness/detail/host_record.hpp`: the renderer emits it and
  staleness rule 1 compares against it, so a bump moves the written bytes, the
  rule and the schema row in one change (FR-019, FR-021, FR-025).
- The text stays within 16 KiB. A write whose record would pass the cap
  stands refused: one line, exit 2, and a standing record keeps its bytes.
  The reader breaks on a document over the cap (FR-018, FR-030).
- Two runs on one host write the same bytes (FR-023).
- The file ends with the document and no newline, the way the compare
  document ends (`compare.cpp:1993`). The reader accepts trailing
  whitespace, so an editor that adds a newline changes nothing it reads.

## The refresh

A write over a standing record reads that record first (FR-006, R-08):

```text
recordVersion  replaced by the version this writer speaks
cpu            replaced
machine        replaced
evidence       kept, row for row, in the stored order
```

A standing record that cannot be read stands refused with one line and
exit 2, and its bytes stay (FR-011).

## Byte rules

- The bytes come from the builder's view, in the call order above, with
  no date, no host name and no pid, the rule
  `source/harness/compare.cpp:1978` states for the compare document
  (FR-023, FR-033).
- simdjson reads the document and nothing else reads JSON (FR-022).
- A planted document that disagrees with the schema in key set, key
  order, kind or size reads as a break (FR-021, FR-030).
