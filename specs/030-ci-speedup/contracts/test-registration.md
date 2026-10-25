# Contract: Test Registration

**Surface**: `test/CMakeLists.txt`, `example/CMakeLists.txt` and the new
`test/tier_labels.sh`, the properties every `ctest` entry carries, and the
`test_tier_labels` entry that audits them. **Closes**: FR-001 to FR-005,
FR-019 to FR-027, FR-031 to FR-034, FR-047, FR-048.

## Properties each entry carries

| Property | Rule | Audit |
| --- | --- | --- |
| `LABELS` | exactly one of `quick` or `slow`, applied by the trailing block that reads the directory `TESTS` property (P-05) | `test_tier_labels` |
| `TIMEOUT` | 300 s where the registration holds none, and every lower master value kept (P-07) | `test_tier_labels`, the CI per-entry times |
| `PROCESSORS` | equal to the compile concurrency or thread count the entry starts, default 1 | the runner log, `ctest` scheduling |
| `RESOURCE_LOCK` | set on every pair that shares a pin, a reservation, a file or a device node | the collision list of P-13 |
| `SKIP_RETURN_CODE` | kept exactly as master sets it (FR-047) | the registration diff |

No entry carries a retry, a sleep, a clock read for its verdict, or
`--repeat until-pass` (FR-005, FR-048). `test_tier_labels` reads both
registration files and fails on a hit for any of those four constructs,
so the rule has a named vehicle inside the suite (FR-048). `RUN_SERIAL`
appears nowhere as a cure.

## Tier list

`SPEEDGUN_SLOW_TESTS` holds one row per slow entry with its reason.
Today's rows:

| Entry | Reason |
| --- | --- |
| the five `dbc_matrix_*` entries | each configures and builds a tree of its own |
| `snippet_compile` | compiles documentation snippets, 27 to 50 s measured |

Every other of the 133 entries takes `quick`. CI passes no label filter
in any `ctest` line, so a label changes local speed and never CI scope
(FR-027).

## `test_tier_labels`

A `ctest` entry that runs the `ctest` binary against the build directory
three times and compares the listings:

| Check | Fault |
| --- | --- |
| `ctest -N -L quick` and `ctest -N -L slow` share a name | both labels on one entry |
| the union of the two lists misses a name from `ctest -N` | an unlabeled entry |
| an entry in `SPEEDGUN_SLOW_TESTS` is absent from the suite | a stale list row |
| an entry holds no `TIMEOUT` and no exception row | a budget the 1500 s default would cover |

It is written first and seen failing against an unlabeled suite, then made
green by the label block (Principle III).

## Matrix entries

Five entries, each `dbc_matrix_<cell>` per E-06, each invoking
`tools/dbc/semantics_matrix.sh <source-dir> <cell>`, each with
`PROCESSORS` equal to `SPEEDGUN_TEST_JOBS` and its own `mktemp -d` build
tree. Registration happens under `if(SPEEDGUN_DBC_MATRIX)` inside the
existing Linux block, so `coverage` and `sanitize` carry none (D-5).

| Entry | `PROCESSORS` | Registered in |
| --- | --- | --- |
| `dbc_matrix_off_o0` | `SPEEDGUN_TEST_JOBS` | `test`, `test-rocky`, `tsan` |
| `dbc_matrix_off_o2` | `SPEEDGUN_TEST_JOBS` | `test`, `test-rocky`, `tsan` |
| `dbc_matrix_on_o0` | `SPEEDGUN_TEST_JOBS` | `test`, `test-rocky`, `tsan` |
| `dbc_matrix_on_o2` | `SPEEDGUN_TEST_JOBS` | `test`, `test-rocky`, `tsan` |
| `dbc_matrix_observe_off_o0` | `SPEEDGUN_TEST_JOBS` | `test`, `test-rocky`, `tsan` |

The base reading each replaces is 308.53 s in `test`, 316.64 s in
`test-rocky`, 322.86 s in `coverage`, 213.67 s in `sanitize` and 273.94 s
in `tsan`, inside `Test` steps of 315 to 333 s.

## Budget exceptions

The exceptions block sits beside the budget loop as a comment list, one
row per entry:

| Field | Content |
| --- | --- |
| entry | the `ctest` name |
| reason | what the entry does that needs the time |
| measured | the per-lane number from a CI run of this branch |
| value | the `TIMEOUT` the registration keeps or takes |

The eight master 120 s values at `test/CMakeLists.txt:580`, `:742`,
`:1392`, `:1408`, `:1425`, `:1440`, `:1456` and `:1473`, and the 300 s
`example_run` at `:1487`, are listed with their master readings and keep
them (FR-031). No `ctest` line carries `--timeout` and no job raises
`timeout-minutes` (FR-034).

## Shared resources, from the audit of P-13

| Resource | Entries | Lock or fix |
| --- | --- | --- |
| the checker's fixed scratch file | `counters_trap_checked` and its fixture | the master lock `counters_trap_checked_out` at `:297`, kept |
| `harness_cli_column_note.txt` in the working directory | `harness_cli_test` | a lock, or a per-entry working directory (P-13) |
| `/tmp` scratch paths under `tools/dbc` | the dbc entries | a lock, or the path made unique per entry |
| cpu pins and reservations | entries with `PROCESSORS` above 1 | `PROCESSORS`, which makes `ctest` schedule them apart |

## Verify

```sh
ctest --preset prepush-quick -N | tail -3
ctest -N -L quick | tail -3
ctest -N -L slow | tail -3
ctest -R test_tier_labels --output-on-failure
```

The two label lists are disjoint, their union equals `ctest -N`, the slow
list holds the six rows above, and `test_tier_labels` passes. On a
configured `ci-ubuntu-test` tree, `ctest -N` shows the five matrix
entries; on a `ci-coverage` tree it shows none.
