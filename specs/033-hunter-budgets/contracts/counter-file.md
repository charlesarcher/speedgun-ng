# Contract: the emitted set and the `@`-file forms (033)

Two halves of one round trip (roadmap criterion 12.7-6; FR-010 to
FR-015, R-07 to R-09).

## `--hunt-emit-set=<file>` (new row, value 280)

| Item | Rule |
| --- | --- |
| Argument | required; a path (standing path rules: relative resolves against the working directory) |
| Guard | legal only beside a hunt; `--hunt-emit-set` without `--hunt`, and beside `--hunt=none`, is the standing conflict: one line naming the value, exit 2 |
| Empty value | one line naming the value, exit 2 |
| Content | per benchmark entry: `toCounterSet(--hunt-top)`, rank ascending; the run writes the deduplicated union of the per-entry lists, entry order then rank order; one catalog address per line; no header |
| Exclusions | an event with a null rank (noisy, excluded, skipped, unmeasured) never enters |
| Empty ranking | an empty file; the run keeps its standing exit |
| Failure | path not openable or write failed: one stderr line naming the path and the `errno` reason, status 2, the standing `writeReport` sentence shape |

## `toCounterSet(topN)` on `sg::hunt::EntryHunt`

```cpp
[[nodiscard]] auto toCounterSet(std::size_t topN) const
    -> std::vector<std::string>;
```

`\pre` none. `\post` the answer holds at most `topN` addresses, the
ranked entries in ascending rank, `topN` above the ranked count
answers them all, `topN` zero answers none. Measures nothing; reads
no clock.

## `--counter=@<file>` (value-grammar extension; no new row)

Reads through `detail::readAddressFileLines` (the shared list-file
reader, the spec 029 grammar):

| Item | Rule |
| --- | --- |
| Form | the whole value starts with `@`; one level; the address elements follow spec 029's separator rule (lines cut at `#`, split on commas) |
| Refusals (one line naming the value, exit 2) | file does not exist; a comma beside nothing (the empty address); the file holds no address |
| Unknown address | the entry point's standing `--counter` shape: one line naming the value, exit 2, the run stops; the entry point resolves every address every source named before the first measurement (`detail::counterAddressFailure`, spec 028 FR-087); a library caller that hands addresses to the runner keeps the runner's `ABSENT` column (spec 015 FR-023) |
| Count | no cap on the number of addresses (spec 029 PC-2 reading) |
| Sources | command line, option file, environment carry the same grammar per item, the environment's `;` items splitting first (the interception sits in the gather arm) |

## Round trip (FR-015, CI, scripted host)

A scripted hunt with `--hunt-emit-set=F` writes `F`; a scripted run
of `--counter=@F` reports one counter column per line of `F`, none
`ABSENT`. No PMU hardware, no clock, no sleep.
