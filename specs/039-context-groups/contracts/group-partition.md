# Contract: the group partition (specs/039)

The harness declares these clauses, the runner and the tests enforce
them, and no caller outside `source/harness/` reaches them. The
doxygen contracts of `source/harness/detail/internal.hpp` govern on
disagreement (Constitution II).

## A. The shared rules

- A-1: A partition covers the instance's contexts exactly once: the
  slots of all groups form the set `0` to `n-1` with no repeat
  (FR-002).
- A-2: A group holds at least one slot, and its name stands unique
  within the partition (FR-002, FR-008, FR-015).
- A-3: The empty partition is a valid answer and stands the ungrouped
  shape of FR-004.
- A-4: A partition answer is deterministic: one plan and one count give
  one partition, in first-appearance order of the slots.
- A-5: No entry point reads a clock, allocates a thread, opens a file
  besides the layout path it was handed, or throws to its caller.
- A-6: A name spells `<unit>-<ordinal>` for a preset unit, the file's
  text for a layout entry, or `g<index>` where an entry named none
  (D-4).

## B. The per-function contracts

### B-1 `groupPartition(Preset preset, const PlacementPlan& plan, std::size_t contexts)`

Answers `std::vector<ContextGroup>`, empty where no partition stands.

- `\pre` `contexts >= 1`, and `plan` answers at least `contexts`
  entries in `cpu` or stands empty for `Preset::NONE`.
- `\pre` The plan came from `resolvePreset` for the same preset,
  layout and count, so `group` and `node` answer position by position.
- `\post` A-1 to A-4 hold over the answer.
- `\post` `Preset::NONE`, a one-context run, or a plan whose unit
  stands `kNoGroup` at every position answers the empty vector
  (FR-006, FR-007, FR-010).
- `\post` The answer changes nothing in `plan`: the cpu assignment and
  the node binding keep the values the resolver wrote (FR-009).

### B-2 `groupOfContext(const std::vector<ContextGroup>& partition, std::size_t position)`

Answers the index of the group holding `position`.

- `\pre` The partition satisfies A-1, and `position` stands below its
  slot count.
- `\post` The answer stands below `partition.size()`, and the group it
  names holds `position` (FR-022).

### B-3 the fit entry `groupLayoutFitStatus(const std::string& path, const std::vector<Instance*>& selected)`

Answers `std::pair<int, std::string>`: a status of `0` with an empty
line, or the usage status `2` with the one line the entry point prints.
The path is `ResolvedOptions::groupLayout`, empty when no source named
the row, and the selection arrives as pointers, the shape
`placementFitStatus` takes beside it.

- `\pre` The option scan settled, so the layout path stands as the
  winning source spelled it, and the selection holds one instance at
  least.
- `\post` No instance ran and no measurement started when the answer
  names a status of `2` (FR-020).
- `\post` The line names the layout path, and a line about a slot names
  the count it failed against (FR-021).
- `\post` The file reads once, at the first selected instance's count: a
  count that first instance fails answers the reader's fault line
  (FR-018), and a later instance's count answers the batch line
  (FR-020).

## C. The refusal and disclosure table

| Case | Channel | Status |
| --- | --- | --- |
| a layout that fits the batch | none | run proceeds |
| a slot count beside an instance of another count | stderr, one line | 2 |
| a layout beside a partition-naming preset | stderr, one line naming the path and the preset value | 2 |
| a layout beside `--hunt` | stderr, one line naming both option names | 2 |
| a grouped run's context line | stdout, the group segment last | 0 |
| an ungrouped run's context line | stdout, byte-identical to master | 0 |
| a report of a grouped run | the `group` key per context | 0 |
| a report of an ungrouped run | no `group` key | 0 |
