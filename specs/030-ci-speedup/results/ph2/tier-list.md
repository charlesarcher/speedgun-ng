# Phase 2 Tier Labels

**Surface**: the `SPEEDGUN_SLOW_TESTS` list and the trailing label block of
`test/CMakeLists.txt`, read by `test/tier_labels.sh`.
**Head sha**: e63030a with the tier block of this task in the working tree
**Run id**: local
**Command**: `cmake --preset=ci-rocky -B build/t011`, then
`cmake --preset=ci-rocky -B build/t011m -D SPEEDGUN_DBC_MATRIX=ON`, then
`ctest --test-dir <dir> -N`, `-N -L quick`, `-N -L slow` and
`-N --show-only=json-v1` for each of the two directories, then
`ctest --test-dir <dir> -R test_tier_labels --output-on-failure`
**Numbers**: the table under The partition

FR-025 wants every entry to carry exactly one tier label, `quick` by
default, with a written reason beside each slow entry. The list carries the
six rows T011 names, one reason comment per row, and the block after it reads
the directory `TESTS` property, so a registration that names no label is
labelled by the same rule that labelled the 133 existing entries of the
directory. The rows name the five `dbc_matrix_*` entries phase 6 registers
(T033) and `snippet_compile`; a row whose entry the lane does not register is
inert, which the block comment states.

## The partition

| Directory | Configure | Entries | `quick` | `slow` | Both | Neither |
| --- | --- | --- | --- | --- | --- | --- |
| `build/t011` | `ci-rocky`, matrix off | 134 | 133 | 1 | 0 | 0 |
| `build/t011m` | `ci-rocky`, `-D SPEEDGUN_DBC_MATRIX=ON` | 135 | 134 | 1 | 0 | 0 |

The counts come from `ctest -N --show-only=json-v1`: each entry's `LABELS`
property is read from the listing, the two label sets are intersected for the
overlap column, and the neither column is the roster minus the union.

`test_tier_labels` exits 0 on both directories:

```text
1/1 Test #24: test_tier_labels .................   Passed    0.18 sec
100% tests passed out of 1
```

## The stale-row gate

`test/tier_labels.sh` fails a `SPEEDGUN_SLOW_TESTS` row that names no entry,
except where no registered entry carries the `dbc_matrix_` prefix: the five
matrix rows are stale only on a lane that registers at least one of them, so
the check opens when T033 registers the per-cell entries and closes nothing
today. The lane above that registers the matrix names it
`dbc_semantics_matrix`, which carries no matrix prefix, which is why
`build/t011m` reads 1 slow entry and not 6.

T039 tightens this gate against the filled list (FR-025, FR-026).

## CI scope

No `ctest` line in `.github/workflows/ci.yml` passes `-L`, so the two labels
move a local `prepush-quick` run and never CI scope (FR-027). The eight CI
`ctest` steps are counted in `ph7/no-filter.md`.
