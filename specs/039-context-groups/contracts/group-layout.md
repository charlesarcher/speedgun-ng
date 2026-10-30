# Contract: the group layout reader (specs/039)

The harness declares these clauses, `group_layout.cpp` enforces them,
and the CLI and the fit walk read the answers. The doxygen contracts of
`source/harness/detail/internal.hpp` govern on disagreement
(Constitution II).

## A. The shared rules

- A-1: One answer carries the partition, the other carries one line of
  text: no fault answer leaves a half-built partition behind (FR-018).
- A-2: The reader enforces the whole grammar of FR-012, FR-013, FR-015,
  FR-017 and FR-018 and answers no fault the grammar does not name.
- A-3: Every fault line names the layout path. A line about one entry
  names the entry by its `name` where it has one, else by its
  zero-based index (FR-021).
- A-4: The reader trims no text, folds no case, expands no path, and
  reads no environment variable (D-5).
- A-5: The reader keeps yaml-cpp inside `source/`: no header under
  `include/` names it, and the specs/006 scans keep their readings
  (FR-016).
- A-6: The answer is order-faithful: the groups stand in document
  order, and each group's slots stand ascending after the read.

## B. The per-function contract

### B-1 `parseGroupLayout(const std::string& path, std::size_t contexts)`

Answers `std::expected<std::vector<ContextGroup>, std::string>`.

- `\pre` `contexts >= 1`. The path stands as the winning option source
  spelled it; the reader opens it and nothing else.
- `\post` The value satisfies A-1, A-2, A-6 and FR-002: the slots cover
  `0` to `contexts-1` exactly once.
- `\post` An entry with no `name` answers `g` plus its zero-based
  document index (FR-015).
- `\post on the error form:` the text names the path, names the entry
  where the fault stands in one, and stands one line with no newline
  (A-3, FR-021).
- `\post` A fault about the key `cpus` names the key and states that
  this release groups by slots (FR-014).

## C. The fault table

| Fault | Line names | Status |
| --- | --- | --- |
| unreadable path | the path, the open fault | 2 |
| unparsable document | the path, the reader's reason | 2 |
| root mapping with no `groups` | the path | 2 |
| root node that is a sequence | the path | 2 |
| empty `groups` sequence | the path | 2 |
| entry that is not a mapping | the path, the index | 2 |
| entry with no `slots` | the path, the entry | 2 |
| empty `slots` sequence | the path, the entry | 2 |
| a `slots` member of the wrong kind | the path, the entry | 2 |
| member outside the context range | the path, the entry, the value | 2 |
| a repeated slot | the path, the slot | 2 |
| a context no entry names | the path, the slot | 2 |
| the key `cpus` | the path, the entry, the key | 2 |
| any key outside the grammar | the path, the entry, the key | 2 |
| repeated group name | the path, the name | 2 |
| a `name` outside the characters or the length | the path, the text | 2 |
