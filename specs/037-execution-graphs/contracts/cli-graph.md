# Contract: the `--graph` option row

Phase 1 output (specs/037). The row joins the standing option table of
`specs/015-benchmark-harness-core/contracts/cli.md` and the table
shape of `specs/025-topology-placement/contracts/cli-placement.md`.
The environment list of specs/028 FR-020 stays the twelve rows it
names: the `graph` row carries no environment name.

## Option table

| Long option | Argument | Value | Effect | Requirement |
| --- | --- | --- | --- | --- |
| `--graph` | `OPTIONAL`: `--graph` alone, `--graph=<file>` | `297` | orders the context starts of every threaded run by the named graph file | FR-001, FR-002, FR-009 |

The row carries the environment flag off, help text, and the owner
`specs/037`. The value `297` is the first free value of the free block
`297` to `319` recorded at `source/harness/detail/options.hpp:76`, and
the block comment moves in the same commit as the row. The value is a
reservation under the rule of the Spec identity section: pull request #58
takes `297` for its `os-noise` row, so the lane takes `298` when #58
lands first. The standing
checks over the table names and values
(`test/source/harness_cli_table_test.cpp:780`) cover the row, and the
standing range check at `:763` reads the window `290` to `319`. The
`--help` and `--options` output print the row (FR-001).

## Grammar

| Form | Behavior |
| --- | --- |
| `--graph=<file>` | the run reads the named file (FR-009) |
| `--graph` alone | the path stands taken from the standing lower source, the option file line (specs/028 FR-035); the row reads no environment variable |
| `--graph` with no path in any source | one line naming the row and the missing path, exit status 2, before any context starts (FR-009) |
| an option file line `--graph=<file>` | the row settles under the same value rules as the command line (specs/028 FR-035) |
| `--graph=<file>` beside a placement preset | the preset plans the cpus and the graph orders the starts; each disclosure keeps its own line (FR-017) |
| `--topology` beside `--graph` | the listing prints and the run exits 0, the standing rule that the listing runs before selection |

The graph parses once, before the first context starts, the reading of
the one layout read at specs/025 FR-002 and the command line's
parse-once shape at `source/harness/cli.cpp:560-563` (FR-011,
specs/028 FR-011, FR-038).

## Refusals and exit statuses

| Case | Output | Status |
| --- | --- | --- |
| each fault of FR-010, in the listed order | one line naming the first fault the run meets, alone; no benchmark output | 2 |
| a threaded run whose context count differs from the node count | one line naming both counts | 2 |
| a batch with no threaded run | one line | 2 |
| a single-context run beside the graph | no graph, no record entry, the standing bytes | 0 |
| a node failed during the run | the standing run-failure report | the standing status of specs/015 |

The status-2 shape stays reserved for what the run refuses before it
starts (FR-021).

## Consumers

- `test/source/harness_cli_graph_test.cpp` drives the grammar, the
  bare row, the option-file source, the count refusals and the
  parse-once rule.
- `test/source/harness_cli_table_test.cpp` covers the row in the name
  and value checks.
- `README.md` and `docs/pages/harness.md` carry the row (FR-032,
  FR-033).
