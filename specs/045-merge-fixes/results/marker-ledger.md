# Marker ledger at the Part A head

`git grep -c LCOV_EXCL -- source include` reads 84: 94 at base, minus
the 10 marker lines commit A2 removed (one at
`counters_provider.hpp:111`, four in the `group_io.cpp` T140 block,
five in the `provider.cpp` empty-selection block). The two
LINE-to-BR swaps at `group_io.cpp` change no count. Part B adds no
marker (FR-015).

## Count by file

| File | Markers |
| --- | --- |
| `source/counters/clock_provider.cpp` | 17 |
| `source/counters/plan.cpp` | 13 |
| `source/counters/system.cpp` | 12 |
| `source/counters/linux_pmu/group_io.cpp` | 11 |
| `source/counters/linux_pmu/fast_read.cpp` | 7 |
| `source/counters/fold.cpp` | 5 |
| `source/counters/linux_pmu/provider.cpp` | 4 |
| `source/counters/linux_pmu/table_parse.cpp` | 4 |
| `include/speedgun-ng/dbc.hpp` | 4 |
| `include/speedgun-ng/counters_provider.hpp` | 2 |
| `source/counters/detail/core.hpp` | 2 |
| `source/simulation/marker.cpp` | 2 |
| `source/counters/linux_pmu/encode.cpp` | 1 |

## Kept list, one reason per marker

- `include/speedgun-ng/dbc.hpp` (4): the abort path of the contract
  checker; no supported build executes an abort and returns (G-21).
- `source/simulation/marker.cpp` (2): the comment-only `#else` branch
  of a supported build set (G-21).
- `include/speedgun-ng/counters_provider.hpp` (2): the defaulted
  default constructor emits no code (the T066 record, older than
  PR #71).
- `source/counters/linux_pmu/group_io.cpp` (11): the two
  `switch (slot.source)` BR markers name the no-case arc of a switch
  over a closed enumeration, every enumerator of which carries a
  scripted count; the retry block and the unstable-pair block name
  the page-rewrite arms the protocol states, and no fixture answer
  forces a kernel page rewrite inside one call (the T140 record).
- `source/counters/linux_pmu/provider.cpp` (4): the past-the-enum arm
  of a closed switch; the disassembly names a range check (T066).
- `source/counters/linux_pmu/table_parse.cpp` (4): the by-value
  return epilogue and the registry arm the generator guarantees
  against (T066).
- `source/counters/linux_pmu/fast_read.cpp` (7): the instruction arm
  and the pair-stability arm; no fixture page stands in for the
  processor serving the instruction (T066).
- `clock_provider.cpp`, `plan.cpp`, `system.cpp`, `fold.cpp`,
  `detail/core.hpp`, `encode.cpp`: outside Part A scope; they carry
  their recorded T066 reasons unchanged.

No kept marker sits in the "waits for ruling" state: each one names
its record (G-21 or T066) or the closed-enumeration shape it covers.
