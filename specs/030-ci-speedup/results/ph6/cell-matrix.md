# Phase 6 Cell Matrix

**Surface**: the five cells of `tools/dbc/semantics_matrix.sh`, as the cell
argument of T032 leaves them.
**Head sha**: f7653e161cbd992d8ce6b393b0890c4a010031ab
**Run id**: local
**Command**: the four commands under Readings, each run from the repository
root
**Numbers**: the table under The cells, one row per cell, and the readings
under Readings

FR-023 wants a cell to name itself, its concurrency and its compiler in the
job log of the lane that ran it, and E-06 holds one row per cell. The table
below is those rows with every field read out of the script itself: the
`ALL_CELLS` list at `:75` to `:81`, the `run_named_cell` dispatch at `:375`
to `:384`, the compiler default at `:62`. The spec's own cites for the same
rows (`:323` to `:326`, `:330`) and for the concurrency read (`:60`) predate
the T032 edit, so the numbers here are the ones this head carries.

## The cells

`run_cell` (`:204`) takes `name`, `ndebug`, `opt`, `contracts`,
`expect_semantic` and `build_dbc` (`:203`). Columns two to seven are those
arguments as `:377` to `:382` pass them. The optimization flag reaches
`CMAKE_CXX_FLAGS` (`:232`) and `ndebug` on adds `-DNDEBUG` to it (`:217`).
The fifth argument is the `SG_CONTRACTS_SEMANTIC` value the probe TUs must
print, and E-06 heads that column `expected asserts`. The sixth says whether
the cell builds and runs `dbc_test` before its probes.

| Cell in the script | `ctest` entry (P-04) | NDEBUG | Flag | Semantics | Expected asserts | Builds `dbc_test` | Family |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `NDEBUG=off_-O0` | `dbc_matrix_off_o0` | off | `-O0` | enforce | 2 | yes | GCC |
| `NDEBUG=off_-O2` | `dbc_matrix_off_o2` | off | `-O2` | enforce | 2 | yes | GCC |
| `NDEBUG=on_-O0` | `dbc_matrix_on_o0` | on | `-O0` | enforce | 2 | yes | GCC |
| `NDEBUG=on_-O2` | `dbc_matrix_on_o2` | on | `-O2` | enforce | 2 | yes | GCC |
| `observe_NDEBUG=off_-O0` | `dbc_matrix_observe_off_o0` | off | `-O0` | observe | 1 | no | GCC |

Family is the one column the script cannot answer per cell. It reads a single
compiler, `CXX`, which defaults to `g++` at `:62`, and prints it in the header
lines at `:135` to `:138`. Every reading below ran its cells under GCC
16.2.1 on this host, so that header line is the source for the column. Which
family each registering lane carries is E-07's field, and
`ph6/compiler-records.md` (T036) holds it.

## Readings

All four readings are local, at the head above, on a host where
`bash -n tools/dbc/semantics_matrix.sh` exits 0.

### One cell, bounded concurrency

```sh
bash tools/dbc/semantics_matrix.sh "$PWD" dbc_matrix_on_o0 2
```

Exit 0, 2 m 58.148 s of wall time, log at `/tmp/cell_on_o0.log`. The opening
lines and the closing ones, verbatim:

```text
semantics_matrix.sh: cell=dbc_matrix_on_o0
semantics_matrix.sh: jobs=2
semantics_matrix.sh: compiler=g++ (GCC) 16.2.1 20260810
semantics_matrix.sh: cxx=g++
...
NDEBUG=on_-O0          2          PASS (1)         elided
CELL dbc_matrix_on_o0 PASS (SG_CONTRACTS_SEMANTIC recorded above)
semantics_matrix.sh: cleaned /tmp/sg-dbc-matrix.4D76rL
CELL_RC=0
```

One cell, one scratch tree, and the tree is gone at exit. The three lines
FR-023 asks for are the first three, so the same text lands in the job log of
whatever lane runs the entry.

### Two refusals

```text
bash tools/dbc/semantics_matrix.sh "$PWD" dbc_bogus
FAIL: unknown cell 'dbc_bogus'; accepted cells: all dbc_matrix_off_o0 dbc_matrix_off_o2 dbc_matrix_on_o0 dbc_matrix_on_o2 dbc_matrix_observe_off_o0
exit 1

bash tools/dbc/semantics_matrix.sh "$PWD" dbc_matrix_on_o0 0
FAIL: jobs must be a positive integer, got '0'
exit 1
```

An unknown name lists the accepted ones (`:92`) and a zero or non-positive
concurrency is refused by the guard at `:99`. Both exit 1 before any
configure, so neither costs a build.

### The no-argument route

```sh
bash tools/dbc/semantics_matrix.sh "$PWD"
```

Driver-run. Log at `/tmp/all_cells.log`, start 18:08:50 and stop 18:21:25 as
written to `/tmp/all_cells.start`: 12 m 35 s between the two stamps, and
`ALL_RC=0`. The stamps carry the driver's clock, offset from this host's
`date`, so only the gap between them is a reading. The header lines of this
run read `cell=all` and `jobs=2`, the default of `:73` with
`SPEEDGUN_TEST_JOBS` unset. The summary the run printed:

```text
NDEBUG=off_-O0         2          PASS (1)         abort(rc=134)
NDEBUG=off_-O2         2          PASS (1)         abort(rc=134)
NDEBUG=on_-O0          2          PASS (1)         elided
NDEBUG=on_-O2          2          PASS (1)         elided

negative probe (observe): SG_CONTRACTS_SEMANTIC=1

ALL 4 CELLS PASS (SG_CONTRACTS_SEMANTIC=2); observe probe SEMANTIC=1
```

Four enforce cells at semantic 2 with exactly-once held, the observe cell at
1, and `assert(false)` governed by `NDEBUG` alone across `-O0` and `-O2`,
which is the FR-017 claim the matrix exists to carry.

## Checks on the script

`bash -n` exits 0. `grep -c 'nproc' tools/dbc/semantics_matrix.sh` prints 2,
and both hits are comments (`:13`, `:71`); a pattern that skips comment lines
finds nothing, so no command reads the cpu count. The concurrency of a cell
comes from argument three, then `SPEEDGUN_TEST_JOBS`, then 2 (`:73`).

## What this task changed

One file, this record. The script edit it certifies was already in the tree
when it was written: argument two selects one cell (`:72`), argument three
bounds that cell's build concurrency (`:73`), the `nproc` read is gone, and
the header lines at `:135` to `:138` are the record FR-023 wants in the job
log.
