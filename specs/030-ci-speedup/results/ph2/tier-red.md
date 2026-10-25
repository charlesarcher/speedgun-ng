# Phase 2 Tier Audit Seen Red

**Surface**: `test/tier_labels.sh`, the `test_tier_labels` entry it registers
in `test/CMakeLists.txt`, and the run that closes T010 (FR-026, FR-031,
FR-047, FR-048).
**Head sha**: 16ffda368107126fc0420a2dea00bfdfe606da64
**Run id**: local
**Command**: the two lines under The command, each run from a shell
**Numbers**: the table under Reading, one row per reading

`test_tier_labels` is registered inside the Linux block, so a fresh
`ci-rocky` tree holds it. The tree registers 134 entries, none of them
labeled, so the entry fails the run exactly the way the contract
of [test-registration.md](../../contracts/test-registration.md) says it
should: written first, seen failing, made green by the tier block of T011
and the budget block of T012 (Principle III).

## The command

Both lines ran on this host. The second one also ran with the shell sitting
at `/`, and it printed the same listing with the same exit status, which is
what passing `${CMAKE_BINARY_DIR}` and `${CMAKE_CTEST_COMMAND}` buys.

```text
cmake --preset=ci-rocky -B build/tier_test
ctest --test-dir build/tier_test -R test_tier_labels --output-on-failure
```

## Reading

| Reading | Value | Verdict |
| --- | --- | --- |
| entries in `ctest -N` | 134 | the suite, with the new entry |
| entries in `ctest -N -L quick` | 0 | below 134, so the tier block is missing |
| entries in `ctest -N -L slow` | 0 | below 134, so the tier block is missing |
| names in both label lists | 0 | no overlap fault, both lists are empty |
| failure lines | 259 | the audit bites |
| `unlabeled` lines | 134 | every entry, closed by T011 |
| `no-budget` lines | 125 | every entry but the nine with a `TIMEOUT`, closed by T012 |
| `stale-slow-row` lines | 0 | `SPEEDGUN_SLOW_TESTS` is T011's and reads as empty |
| `determinism` lines | 0 | neither registration carries a forbidden construct |
| `skip-rule` lines | 0 | the eight master `SKIP_RETURN_CODE` rows match the baseline |
| exit status of the `ctest` line | 8 | red, as required |

The nine entries that hold a `TIMEOUT` are the eight master 120 s rows plus
the 300 s `example_run`, so 134 minus 9 is the 125 above.

## The first ten failure lines

```text
FAIL: unlabeled: speedgun-ng_test carries no tier label
FAIL: unlabeled: dbc_test carries no tier label
FAIL: unlabeled: dbc_literal_fixture carries no tier label
FAIL: unlabeled: dbc_trap_checked carries no tier label
FAIL: unlabeled: dbc_compile_fail carries no tier label
FAIL: unlabeled: dbc_standalone_test carries no tier label
FAIL: unlabeled: dbc_dependency_scan carries no tier label
FAIL: unlabeled: hwloc_purity_scan carries no tier label
FAIL: unlabeled: hwloc_nm_proof carries no tier label
FAIL: unlabeled: simdjson_purity_scan carries no tier label
```

The line above the block, printed first by every run, is the reading the
faults are taken from: `read: 134 entries, 0 quick, 0 slow`.

## The control that proves each check fires

Three of the five checks are silent in this tree, so the same script was run
against a two-entry scratch project under `/tmp` with the faults planted
one at a time in a copy of the two registration files. Every planted fault
printed its own line, and the planted rows that should stay silent did not.

| Planted | Line printed |
| --- | --- |
| one entry with both labels | `both-labels: alpha carries the quick label and the slow label` |
| a slow row naming no entry | `stale-slow-row: SPEEDGUN_SLOW_TESTS names ghost_entry, which this suite does not register` |
| an entry with no `TIMEOUT` | `no-budget: alpha holds no TIMEOUT and no exception row` |
| a `sleep` on a code line | `determinism: ...:1582 sleeps: sleep 1` |
| a `string(TIMESTAMP` read | `determinism: ...:1583 reads a clock for its verdict` |
| a `while` loop and an until-pass retry | two `determinism` lines, one per construct |
| a `SKIP_RETURN_CODE 3` on an entry master does not skip | `skip-rule: dbc_test sets SKIP_RETURN_CODE 3, master sets no skip rule` |
| a row naming a matrix entry, no matrix registered | nothing, the lane exemption held |
| a comment row naming an entry as an exception | its `no-budget` line disappeared |

The comment exemption is the one the file at hand needs. The prose at
`test/CMakeLists.txt:1107` says a spin participant waits without sleeping,
numbered 1122 here because this entry's registration sits above it,
and that line stays a pass because a hit inside a comment line is prose.
The registration comment of the new entry says the same word two hundred
lines earlier, and it stays a pass for the same reason.

The lane exemption is the other half of the same care: the five
`dbc_matrix_*` entries are registered only where the preset turns
`SPEEDGUN_DBC_MATRIX` on, so the coverage and sanitize lanes hold none and a
slow row for one of them is stale only where the lane registers at least
one. The `ci-rocky` tree above registers none.

## What closes the run

| Fault | Closing task |
| --- | --- |
| `unlabeled` | T011, the `SPEEDGUN_SLOW_TESTS` list and the label block |
| `no-budget` | T012, the budget block over the directory `TESTS` property |
| `stale-slow-row` | stays closed once T011 fills the list, and T039 closes it for good |

`bash -n test/tier_labels.sh` exits 0 and the file carries the executable
bit, the way the other pure-script entries do.
