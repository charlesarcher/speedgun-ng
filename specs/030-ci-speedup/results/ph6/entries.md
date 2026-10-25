# Phase 6 Matrix Entry Registration

**Surface**: the `if(SPEEDGUN_DBC_MATRIX)` block at `:158` to `:195` of
`test/CMakeLists.txt`, the five `ctest` entries it registers, and the
two comments whose singular wording the split made wrong.
**Head sha**: 3cf4d639a3ae85af42d2dd087a5aa83094eb7943
**Run id**: local
**Command**: the six commands under The readings, each run from the
repository root
**Numbers**: the table under Checks, one row per command, with the count
or exit each produced

One entry named `dbc_semantics_matrix` ran all five cells inside one
`ctest` entry, so a single scheduling slot held the whole matrix and
`ctest` could not place the cells apart. T033 splits that registration
into the five entries of the `run_named_cell` case table at
`tools/dbc/semantics_matrix.sh:377` to `:383`. Each entry passes its own
cell name as argument two and `SPEEDGUN_TEST_JOBS` as argument three,
the order the usage line at `:6` names (FR-019, FR-003, D-4). The five
rows of `SPEEDGUN_SLOW_TESTS` at `test/CMakeLists.txt:1625` to `:1633`
already carried these names, and that list is untouched here.

## The readings

### The two configure commands

```text
$ cmake --preset=ci-ubuntu-test -B build/entries-on
-- Generating done (0.4s)
-- Build files have been written to: /home/archerc/code/speedgun-ng/build/entries-on
exit 0

$ cmake --preset=ci-coverage -B build/entries-off
-- Generating done (0.5s)
-- Build files have been written to: /home/archerc/code/speedgun-ng/build/entries-off
exit 0
```

`ci-ubuntu-test` sets `SPEEDGUN_DBC_MATRIX` to `ON`; `ci-coverage`
leaves it unset, so the guard registers nothing there (D-5).

### The two listings

```text
$ ctest --test-dir build/entries-on -N -R dbc_matrix
Test project /home/archerc/code/speedgun-ng/build/entries-on
  Test #21: dbc_matrix_off_o0
  Test #22: dbc_matrix_off_o2
  Test #23: dbc_matrix_on_o0
  Test #24: dbc_matrix_on_o2
  Test #25: dbc_matrix_observe_off_o0

Total Tests: 5
exit 0
```

```text
$ ctest --test-dir build/entries-off -N -R dbc_matrix
Test project /home/archerc/code/speedgun-ng/build/entries-off

Total Tests: 0
exit 0
```

### The tier audit

```text
$ bash test/tier_labels.sh build/entries-on ctest
read: 139 entries, 132 quick, 7 slow
ok: every entry carries one label and a budget, and neither registration breaks FR-048
exit 0
```

Each of the five entries takes the `slow` label from its
`SPEEDGUN_SLOW_TESTS` row, so the five join the two rows
`snippet_compile` and `harness_cli_test` already held: 132 plus 7 is
139, the full count.

### The two gates

```text
$ cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake
exit 0

$ cmake -P cmake/spell.cmake
Used config files:
    1: .codespellrc
exit 0
```

## Checks

| Check | Expect | Exit | Decisive line | Verdict |
| --- | --- | --- | --- | --- |
| configure `ci-ubuntu-test` | 0 | 0 | generating done for `build/entries-on` | PASS |
| list in that tree | the five names, 5 | 0 | `Total Tests: 5` | PASS |
| configure `ci-coverage` | 0 | 0 | generating done for `build/entries-off` | PASS |
| list in that tree | empty, 0 | 0 | `Total Tests: 0` | PASS |
| tier audit | its ok line | 0 | `ok: every entry carries one label and a budget, and neither registration breaks FR-048` | PASS |
| lint | 0 | 0 | no output, so no file is badly formatted | PASS |
| spell | 0 | 0 | no hit line after the config file list | PASS |

## Properties each entry carries

Read from `build/entries-on/test/CTestTestfile.cmake` at lines 47 to 56,
which is what `ctest` schedules:

| Entry | Script arguments past the script path | `PROCESSORS` | `LABELS` | `TIMEOUT` |
| --- | --- | --- | --- | --- |
| `dbc_matrix_off_o0` | source dir, `dbc_matrix_off_o0`, `2` | 2 | slow | 300 |
| `dbc_matrix_off_o2` | source dir, `dbc_matrix_off_o2`, `2` | 2 | slow | 300 |
| `dbc_matrix_on_o0` | source dir, `dbc_matrix_on_o0`, `2` | 2 | slow | 300 |
| `dbc_matrix_on_o2` | source dir, `dbc_matrix_on_o2`, `2` | 2 | slow | 300 |
| `dbc_matrix_observe_off_o0` | source dir, `dbc_matrix_observe_off_o0`, `2` | 2 | slow | 300 |

`SPEEDGUN_TEST_JOBS` is a cache `STRING` that defaults to `2` at
`test/CMakeLists.txt:19`, so `PROCESSORS` equals the value each entry
hands its own build, and no entry reads `nproc`. Each entry holds those
slots for the whole of its cell, which is the FR-003 reading the
`PROCESSORS` row of the registration contract records.

## What this task changed

One source file: `test/CMakeLists.txt`. Five `add_test` calls, one
comment line above each naming the cell it registers, then one
`set_tests_properties` call over the five names that carries
`PROCESSORS ${SPEEDGUN_TEST_JOBS}`. The comment above the guard and
the comment above the property call now speak of the entries in the
plural. The guard, the Linux block that holds it, and the slow list all
stand where they were. No entry carries a retry, a sleep, a clock read
for its verdict, or a repeat rule (FR-048), and `test_tier_labels`
reads that back from the two registration files.

## Gates over this record

```text
$ python3 tools/prose/prose_gate.py --check prose --mode tree --paths test/CMakeLists.txt specs/030-ci-speedup/results/ph6/entries.md
prose-lint: 1 sources, 664 units examined, 0 findings, 0 skipped
exit 0
$ cmake -P cmake/spell.cmake
Used config files:
    1: .codespellrc
exit 0
```

The prose gate in tree mode lists tracked files, so the count above
reads 1 source: `test/CMakeLists.txt` is tracked and this record is
unstaged while the driver holds the commit. The same command was re-run
at `/tmp/entries-prose-check`, a throwaway repository holding a copy of
`tools/prose/prose_gate.py`, `tools/prose/prose_rules.yaml`, this record
and the edited `test/CMakeLists.txt`, both staged in that index. There
the summary reads 2 sources, 0 findings, which covers this record. The
main working tree keeps its own index: nothing outside these two files
was staged or changed here.

VERDICT: PASS
