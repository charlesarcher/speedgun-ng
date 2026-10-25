# Phase 2 Time Budgets

**Surface**: the budget loop of `test/CMakeLists.txt` and the matching loop of
`example/CMakeLists.txt`, read by `test/tier_labels.sh`.
**Head sha**: e63030a with the budget block of this task in the working tree
**Run id**: local
**Command**: `cmake --preset=ci-rocky -B build/t011`, then
`cmake --preset=ci-rocky -B build/t011m -D SPEEDGUN_DBC_MATRIX=ON`, then
`ctest --test-dir <dir> -N --show-only=json-v1` for each directory, then
`ctest --test-dir <dir> -R test_tier_labels --output-on-failure`
**Numbers**: the table under The budgets

FR-031 wants a time budget on every entry so a hang ends inside the change
that caused it. The loop reads the directory `TESTS` property once and sets
`TIMEOUT 300` only where the registration left the property unset, so a value
a registration names stays where it was set. 300 s is the number the audit
uses for the ceiling and the CTest default of 1500 s is never reached.

## The budgets

| Directory | Entries | With a `TIMEOUT` | Without |
| --- | --- | --- | --- |
| `build/t011` (matrix off) | 134 | 134 | 0 |
| `build/t011m` (matrix on) | 135 | 135 | 0 |

The values that are not 300 s are the eight rows master sets to 120 s, and
they read 120.0 in the listing at the head above:

| Entry | `TIMEOUT` |
| --- | --- |
| `harness_calibration_test` | 120 |
| `base_cli_matrix` | 120 |
| `report_shape` | 120 |
| `report_shape_guide_sample` | 120 |
| `report_shape_percentiles_sample` | 120 |
| `report_shape_json_reports_sample` | 120 |
| `report_shape_jitter_sample` | 120 |
| `report_shape_hunt_rank_sample` | 120 |

Eight rows, which is the count T012 names at `test/CMakeLists.txt:580`,
`:742`, `:1392`, `:1408`, `:1425`, `:1440`, `:1456` and `:1473`. The 300 s
`example_run` of `:1487` is unchanged: the loop leaves a set property alone, so
its own value is the one the listing shows.

## The example directory

`example/CMakeLists.txt` registers no entry today: `example_run` and the
snippet entries live in `test/CMakeLists.txt` (`:1498` onward), which the
collision audit of `ph2/collisions.md` records. The loop there is therefore a
guard for a registration the directory may gain, and on this head it reads an
empty list. `ctest -N` counts are identical with and without it.

## Exceptions

FR-032's exception block is T044's work and is not written yet. The reader in
`test/tier_labels.sh` already looks for it: a comment line opening with
`budget exception:` names an entry that may keep no budget, and while no such
line exists the audit requires a budget on all 134 entries, which is the state
above.
