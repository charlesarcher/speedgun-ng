# Contract: the main page, the example, and the caps

Phase 1 output. What the documentation surface carries (FR-025 to
FR-029, Budget note).

## Main-page section

One new section, "Topology placement", in the harness part of `README.md`:

- At most 25 lines, the standing `SECTION_LINE_CAP`
  (`tools/docs/example_drift.py:113`).
- Its snippet region comes from `example/docs/topology_placement.cpp`,
  built as `docs_topology_placement` and run by `example_run.sh`
  (FR-025).
- The section states the five preset names, the fit idea, and the
  `--topology` listing, and points to
  `docs/pages/harness.md#topology-placement` for the detail.
- `SECTIONS` and `SECTION_OWNERS` of `tools/docs/example_drift.py` gain
  the `Topology placement` entry with its owner file
  `topology_placement.cpp`; the gate rejects an unregistered section at
  `:540` and raises `KeyError` at `:532`.
- Every fenced block of the section carries a region marker naming a
  region of its owner file, and the page keeps exactly one sample output
  block, `example/first_benchmark.expected#report` at `README.md:80`, so
  the listing travels as prose, inline code, or a marked region of this
  feature's example.

## Runnable example

`example/docs/topology_placement.cpp`:

- Registers `bmCompact` with `threads(2)` and the handle preset `compact`,
  the widest of the five, whose place count equals the thread count of the
  mask, so a bare run places on any host that can run a two-context
  benchmark at all (FR-029).
- Registers `bmPair` with `threads(2)` and no handle preset, so the
  command line chooses its shape: the handle preset wins over the command
  line, so a family the command line is meant to place carries none
  (FR-015).
- Carries the marked region the page copies, the standing
  `add_docs_example` convention of `example/CMakeLists.txt:40-47`.
- Prints a placement, so the run `example_run` makes of this example
  carries the `cpu@group` disclosure.

## Quick-reference and option rows

The main page keeps two tables and the drift gate checks both: the name
check at `tools/docs/example_drift.py:621-628` requires every name row to
appear in an installed header, and the option check at
`tools/docs/example_drift.py:629-642` requires a row in the option table
of `README.md:640` for every long option of `speedgunMain`. One row per
user-visible name this feature adds (FR-026):

| Table | Row this feature adds |
| --- | --- |
| Quick reference of names | `placement` on `BenchmarkHandle`: states the preset that places this family's contexts |
| Option table of `README.md:640` | `--placement`: places every threaded run of the process by one preset |
| Option table of `README.md:640` | `--topology`: prints the machine layout and exits |

## Detail page

`docs/pages/harness.md` gains "Topology placement" with the preset
table, the fit-limit idea, the report shape with the topology line, and
the listing output (FR-027).

## Caps

| Cap | Value at the rebase point `c7f1769` | Move |
| --- | --- | --- |
| `SECTION_LINE_CAP` | 25 | unchanged |
| `PAGE_LINE_CAP` | 659, with `README.md` at 659 (`tools/docs/example_drift.py:112`) | raised by the measured size of the new section and its rows, recorded in the tasks ledger at implementation time |

Prose is trimmed before the cap moves, and no snippet is dropped (spec
Budget note).

## Consumers

- `example_drift`: the page region equals the example's marked region,
  and the section stays within its cap.
- `example_run`: `docs_topology_placement` builds and runs.
- `report_shape`: the README sample block (`first_benchmark`), the guide
  page's recorded run (`docs_reading_the_report`) and the percentiles recorded
  run (`docs_percentiles`) keep their shape; none runs with a preset, so their
  bytes stand unchanged.
- `prose`: every new page line and comment passes.
