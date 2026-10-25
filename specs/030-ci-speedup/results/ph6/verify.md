# Phase 6 Matrix Registration Verification

**Surface**: the five `dbc_matrix` entries `test/CMakeLists.txt`
registers, the `SPEEDGUN_DBC_MATRIX` keys of the five lane presets in
`CMakePresets.json`, the `SPEEDGUN_SLOW_TESTS` rows behind those names,
the audit `test/tier_labels.sh` runs over them, the cache steps and the
`ctest` lines of `.github/workflows/ci.yml`, and the three source gates.
Every reading is taken from the working tree, and this record edits
nothing outside itself.
**Head sha**: 88be8d071b92fdf7b99fc4f9ef4fdeb75415cd1b
**Run id**: local
**Command**: the commands under Check 1 to Check 7, each run from the
repository root, plus the two supporting reads named inside Check 1
**Numbers**: the exit status and the printed count under each check, and
the verdict table at the end

Seven files sit in `specs/030-ci-speedup/results/ph6`. Six of them are
the records of T033 to T038 (`entries.md`, `lane-flag.md`, `lanes.md`,
`compiler-records.md`, `list-diff.md`, `dropping-lanes.md`) and one is
T032's `cell-matrix.md`. Each check below carries the claim those records
make, the command that reads it back, and the number that command
printed.

Five build trees belong to this node, each named under `build/`:
`ph6-verify-on`, `ph6-verify-off`, `ph6-verify-cov`, `ph6-verify-san` and
`ph6-verify-jobs`. The configures ran while `git rev-parse HEAD` read
`dae612e949bdd91abba07c2d9e6555e90cadc844`. Two driver commits landed
during the reading, and the head this record names holds the same three
inputs: `git diff --stat` over the range prints one line for
`.github/workflows/ci.yml` and nothing for the two registration files.

```text
$ git diff --stat dae612e949bdd91abba07c2d9e6555e90cadc844 HEAD -- \
    test/CMakeLists.txt CMakePresets.json .github/workflows/ci.yml
 .github/workflows/ci.yml | 2 +-
 1 file changed, 1 insertion(+), 1 deletion(-)
[exit status: 0]
```

The one line is the `static-analysis` install step, which gained the
`lcov` package name. Check 5 and Check 6 read the workflow after that
commit, so both hold at the head above. The blob of
`test/CMakeLists.txt` at that head is `913ebb67a4c05de75a8711ab8b3fd86a1a0465b3`
and the blob of `CMakePresets.json` is `07e235b279cc9f372f8d20434efa3ab04445a537`, which
are the bytes the trees were configured from.

## Check 1: five entries in a `ci-ubuntu-test` tree, none in a coverage or sanitizer tree

```text
$ cmake --preset=ci-ubuntu-test -B build/ph6-verify-on
$ cmake --preset=ci-coverage -B build/ph6-verify-cov
$ cmake --preset=ci-sanitize -B build/ph6-verify-san
CFG tag=on preset=ci-ubuntu-test exit=0
CFG tag=cov preset=ci-coverage exit=0
CFG tag=san preset=ci-sanitize exit=0
[exit status: 0]
```

```text
$ for t in on cov san; do
    ctest --test-dir build/ph6-verify-$t -N -R dbc_matrix
  done
tag=on exit=0 matrix=5
  Test #21: dbc_matrix_off_o0
  Test #22: dbc_matrix_off_o2
  Test #23: dbc_matrix_on_o0
  Test #24: dbc_matrix_on_o2
  Test #25: dbc_matrix_observe_off_o0
tag=cov exit=0 matrix=0
tag=san exit=0 matrix=0
[exit status: 0]
```

`matrix=` counts the name lines the listing printed and `exit=` is that
listing's status. The registering tree holds the five names at numbers 21
to 25, matching `entries.md` and `lanes.md`. The two other trees hold
none, which is the D-5 split the same two records state.

Two supporting reads. The first shows the trees are the lanes they claim:

```text
$ for t in cov san; do ctest --test-dir build/ph6-verify-$t -N | grep 'Total Tests'; done
tag=cov Total Tests: 134CMAKE_BUILD_TYPE:STRING=Coverage ENABLE_COVERAGE:BOOL=ON
tag=san Total Tests: 134CMAKE_BUILD_TYPE:STRING=Sanitize ENABLE_COVERAGE:BOOL=OFF
[exit status: 0]
```

Both are real lane trees: 134 entries apiece, the `Coverage` build
configuration with `ENABLE_COVERAGE` on, and the `Sanitize` configuration.
The `134` matches the `lanes.md` and `list-diff.md` counts for those two
lanes. The second supporting read is the cache switch:

```text
$ grep -c SPEEDGUN_DBC_MATRIX build/ph6-verify-{on,off,cov,san}/CMakeCache.txt
build/ph6-verify-on/CMakeCache.txt:1
build/ph6-verify-off/CMakeCache.txt:1
build/ph6-verify-cov/CMakeCache.txt:0
build/ph6-verify-san/CMakeCache.txt:0
[exit status: 0]
```

The two lines the counts name read
`SPEEDGUN_DBC_MATRIX:UNINITIALIZED=ON` for `on` and
`SPEEDGUN_DBC_MATRIX:UNINITIALIZED=OFF` for the tree of Check 3, so the
switch is the only difference between them.

## Check 2: each command names the script, the source dir, its cell and the job count

```text
$ ctest --test-dir build/ph6-verify-on -N --show-only=json-v1
$ ctest --test-dir build/ph6-verify-jobs -N --show-only=json-v1
tree=/tmp/ph6-verify-on.json matrix_entries=5
  dbc_matrix_observe_off_o0 argc=5 script=semantics_matrix.sh dir=/home/archerc/code/speedgun-ng cell=dbc_matrix_observe_off_o0 jobs=2 PROCESSORS=2
  dbc_matrix_off_o0 argc=5 script=semantics_matrix.sh dir=/home/archerc/code/speedgun-ng cell=dbc_matrix_off_o0 jobs=2 PROCESSORS=2
  dbc_matrix_off_o2 argc=5 script=semantics_matrix.sh dir=/home/archerc/code/speedgun-ng cell=dbc_matrix_off_o2 jobs=2 PROCESSORS=2
  dbc_matrix_on_o0 argc=5 script=semantics_matrix.sh dir=/home/archerc/code/speedgun-ng cell=dbc_matrix_on_o0 jobs=2 PROCESSORS=2
  dbc_matrix_on_o2 argc=5 script=semantics_matrix.sh dir=/home/archerc/code/speedgun-ng cell=dbc_matrix_on_o2 jobs=2 PROCESSORS=2
tree=/tmp/ph6-verify-jobs.json matrix_entries=5
  dbc_matrix_observe_off_o0 argc=5 script=semantics_matrix.sh dir=/home/archerc/code/speedgun-ng cell=dbc_matrix_observe_off_o0 jobs=3 PROCESSORS=3
  dbc_matrix_off_o0 argc=5 script=semantics_matrix.sh dir=/home/archerc/code/speedgun-ng cell=dbc_matrix_off_o0 jobs=3 PROCESSORS=3
  dbc_matrix_off_o2 argc=5 script=semantics_matrix.sh dir=/home/archerc/code/speedgun-ng cell=dbc_matrix_off_o2 jobs=3 PROCESSORS=3
  dbc_matrix_on_o0 argc=5 script=semantics_matrix.sh dir=/home/archerc/code/speedgun-ng cell=dbc_matrix_on_o0 jobs=3 PROCESSORS=3
  dbc_matrix_on_o2 argc=5 script=semantics_matrix.sh dir=/home/archerc/code/speedgun-ng cell=dbc_matrix_on_o2 jobs=3 PROCESSORS=3
[exit status: 0]
```

Both `ctest` listings exit 0. Every command vector holds five arguments:
`/usr/bin/bash`, the script path ending in `semantics_matrix.sh`, the
source dir, the entry's own cell name, and the job count in the position
the script's usage line names as its third argument.
`build/ph6-verify-jobs` is the same preset configured with
`-DSPEEDGUN_TEST_JOBS=3`, whose cache line reads
`SPEEDGUN_TEST_JOBS:STRING=3`, so the fifth argument and `PROCESSORS`
both move from 2 to 3. That is the proof the two carry
`SPEEDGUN_TEST_JOBS` and a literal count, and 5 entries out of 5 match
in each tree.

## Check 3: the slow rows and the tier audit, both switch settings

```text
$ python3 /tmp/ph6-verify-slow.py /tmp/ph6-verify-on.json
tree=/tmp/ph6-verify-on.json
slow_rows_total=7
slow_rows=dbc_matrix_off_o0,dbc_matrix_off_o2,dbc_matrix_on_o0,dbc_matrix_on_o2,dbc_matrix_observe_off_o0,snippet_compile,harness_cli_test
slow_matrix_rows=5 registered_matrix_entries=5
matrix_row_set_vs_registered=EQUAL
slow_labelled_entries_in_tree=7
slow_labelled=dbc_matrix_observe_off_o0,dbc_matrix_off_o0,dbc_matrix_off_o2,dbc_matrix_on_o0,dbc_matrix_on_o2,harness_cli_test,snippet_compile
[exit status: 0]
```

`SPEEDGUN_SLOW_TESTS` holds 7 rows: the five matrix rows plus
`snippet_compile` and `harness_cli_test`. Read on its matrix subset, the
row set equals the 5 registered names exactly, and each of the five
carries the `slow` label inside the tree, which is the number the audit
line below reports as `7 slow`. A reader who takes the sentence to mean
seven rows in the list in total finds 7 with two names belonging to
earlier phases.

The audit runs four times, once per tree of this node:

```text
$ bash test/tier_labels.sh build/ph6-verify-on ctest
read: 139 entries, 132 quick, 7 slow
ok: every entry carries one label and a budget, and neither registration breaks FR-048
AUDIT_ON_RC=0
$ bash test/tier_labels.sh build/ph6-verify-off ctest
read: 134 entries, 132 quick, 2 slow
ok: every entry carries one label and a budget, and neither registration breaks FR-048
AUDIT_OFF_RC=0
$ bash test/tier_labels.sh build/ph6-verify-cov ctest
read: 134 entries, 132 quick, 2 slow
ok: every entry carries one label and a budget, and neither registration breaks FR-048
AUDIT_EXTRA_RC=0
$ bash test/tier_labels.sh build/ph6-verify-san ctest
read: 134 entries, 132 quick, 2 slow
ok: every entry carries one label and a budget, and neither registration breaks FR-048
AUDIT_EXTRA_RC=0
[exit status: 0]
```

The `on` tree is `ci-ubuntu-test` as the preset ships it, so
`SPEEDGUN_DBC_MATRIX=ON`; `ph6-verify-off` is the same preset with
`-DSPEEDGUN_DBC_MATRIX=OFF`. The ok line prints in all four runs and all
four exits are 0, so the audit passes with the five entries present at
139 entries and absent at 134, which is what `entries.md` and the
`stale-slow-row` tolerance in `test/tier_labels.sh` claim.

## Check 4: the switch in the five lane presets

```text
$ python3 /tmp/ph6-verify-preset.py
ci-coverage own=ABSENT effective=None inherits=['coverage-linux', 'dev-mode', 'ci-cache']
ci-sanitize own=ABSENT effective=None inherits=['ci-linux', 'dev-mode', 'ci-cache']
ci-tsan own=ON effective=ON inherits=['ci-build', 'ci-linux', 'dev-mode', 'ci-cache']
ci-ubuntu-test own=ON effective=ON inherits=['ci-build', 'ci-linux', 'dev-mode', 'ci-cache']
ci-rocky own=ON effective=ON inherits=['ci-build', 'ci-linux', 'dev-mode', 'ci-cache']
own_key_count=3
[exit status: 0]
```

`own` is the key in the preset's own `cacheVariables` block and
`effective` is the value after walking `inherits`, so the two dropping
presets take the key from no ancestor either. `own_key_count=3` says the
key appears in 3 of the configure presets in the file, which is the
`lane-flag.md` table and the `test-registration.md` matrix table read
back from the file.

## Check 5: the cache invariant of phase 3 still holds

```text
$ python3 /tmp/ph6-verify-ci.py
jobs_head=13 jobs_base=12
job=lint steps=7 build=[] restore=[] save=[] env_ccache=False
job=coverage steps=13 build=[6, 9] restore=[3] save=[12] env_ccache=True
job=sanitize steps=10 build=[6] restore=[3] save=[9] env_ccache=True
job=tsan steps=10 build=[6] restore=[3] save=[9] env_ccache=True
job=static-analysis steps=15 build=[6, 7, 8] restore=[3] save=[14] env_ccache=True
job=test steps=25 build=[6] restore=[3] save=[24] env_ccache=True
job=test-rocky steps=11 build=[6] restore=[3] save=[10] env_ccache=True
job=shared-audit steps=25 build=[6] restore=[3] save=[24] env_ccache=True
job=downstream-consumer steps=23 build=[4, 5, 6, 8, 12] restore=[3] save=[22] env_ccache=True
job=consumer-release steps=19 build=[7, 12, 16] restore=[3] save=[18] env_ccache=True
job=dbc-gate steps=11 build=[6] restore=[3] save=[10] env_ccache=True
job=prose-lint steps=12 build=[8] restore=[5] save=[11] env_ccache=True
job=docs steps=6 build=[] restore=[] save=[] env_ccache=False
building_jobs=11
building_list=consumer-release,coverage,dbc-gate,downstream-consumer,prose-lint,sanitize,shared-audit,static-analysis,test,test-rocky,tsan
restore_only_jobs=
--- step counts vs base f507b8f ---
job=lint head=7 base=7 delta=+0
job=coverage head=13 base=8 delta=+5
job=sanitize head=10 base=5 delta=+5
job=tsan head=10 base=5 delta=+5
job=static-analysis head=15 base=ABSENT
job=test head=25 base=20 delta=+5
job=test-rocky head=11 base=6 delta=+5
job=shared-audit head=25 base=20 delta=+5
job=downstream-consumer head=23 base=16 delta=+7
job=consumer-release head=19 base=12 delta=+7
job=dbc-gate head=11 base=6 delta=+5
job=prose-lint head=12 base=7 delta=+5
job=docs head=6 base=6 delta=+0
total_head=187 total_base=118
jobs_that_lost_a_step=0
cache_order_problems=0
[exit status: 0]
```

Indexes are 0-based step positions. In each of the 11 jobs that run
`cmake --build`, the `actions/cache/restore` index sits below the first
build index and the `actions/cache/save` index sits above the last build
index, and each save is the job's final step. `cache_order_problems=0`
collects three tests per building job: restore before the first build,
save after the last one, and `CCACHE_DIR` in the job `env`. The two
non-building jobs `lint` and `docs` hold no cache step and no `env`
block, the phase 3 shape. `jobs_that_lost_a_step=0` compares each job's
step count with the same job at the base `f507b8f`: 11 jobs gained 5 or 7
steps, `lint` and `docs` hold their base counts, `static-analysis` is new
with 15 steps, and no job is smaller than it was. The phase 3 record
counted 10 building jobs of 12; the 11th is `static-analysis`, the shard
phase 5 added, and it carries the same restore, build and save triple.

## Check 6: the test lane preset and the label filters

```text
$ python3 /tmp/ph6-verify-ctest.py
job=consumer-release   step=16 label_flags=none line=ctest --test-dir build/consumer-release-dev -R counters_trap \
job=coverage           step= 8 label_flags=none line=ctest -j 1 --output-on-failure --no-tests=error
job=dbc-gate           step= 8 label_flags=none line=ctest -R dbc_gate_fixtures --output-on-failure --no-tests=error
job=prose-lint         step=10 label_flags=none line=ctest --test-dir build/dev -R prose_gate_fixtures --output-on-fa
job=sanitize           step= 8 label_flags=none line=ctest --output-on-failure --no-tests=error -j "$(nproc)"
job=test               step=23 label_flags=none line=ctest --output-on-failure --no-tests=error -C Release -j "$(npro
job=test-rocky         step= 9 label_flags=none line=ctest --output-on-failure --no-tests=error -C Release -j "$(npro
job=tsan               step= 8 label_flags=none line=ctest --output-on-failure --no-tests=error -j "$(nproc)"
ctest_lines=8 label_filter_hits=0
test_job_configure=['cmake --preset=ci-ubuntu-test']
[exit status: 0]
```

The scan walks every line of every `run` block in the file, keeps the 8
that name `ctest`, splits each into shell tokens and looks for `-L` and
`-LE`, including the joined forms such as `-Lquick`. `label_filter_hits=0`
is FR-027 holding: the labels reach no CI scope. The last line is the
`Configure` step of the `test` job, and it names `ci-ubuntu-test`, the
preset Check 4 and Check 1 read. The `line=` column is cut at 64
characters, so a long line shows a prefix; the token test ran over the
whole line.

## Check 7: the three source gates

```text
$ cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake
LINT_RC=0
[exit status: 0]

$ cmake -P cmake/spell.cmake
Used config files:
    1: .codespellrc
SPELL_RC=0
[exit status: 0]

$ python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/030-ci-speedup/results
prose-lint: 30 sources, 3345 units examined, 0 findings, 0 skipped
PROSE_RC=0
[exit status: 0]
```

All three exit 0. `lint.cmake` prints nothing before its status, the shape
it has when no file is badly formatted. `spell.cmake` prints its
configuration banner, which is the shape it has when `codespell` flags no
word; it walks the working tree, so it read this record. The prose gate
counts 30 sources and reports 0 findings over the ledger directory.

Tree mode takes its candidate list from `git ls-files`
(`tools/prose/prose_gate.py:598`), so an untracked file is outside its
count. This record stays untracked while the driver holds the commit, and
the driver may not be asked to stage it, so the same rule data and the
same evaluation function were driven over this file directly:

```text
$ python3 /tmp/ph6-verify-prose.py
prose-lint: 1 sources, 298 units examined, 0 findings, 0 skipped
PROSE_SELF_RC=0
[exit status: 0]
```

## Verdict

| Check | Claim | Exit | Number | Result |
| --- | --- | --- | --- | --- |
| 1 | five entries in a `ci-ubuntu-test` tree, none in a coverage or a sanitize tree | 0, 0, 0 | 5, 0, 0 | PASS |
| 2 | each command names script, source dir, own cell and `SPEEDGUN_TEST_JOBS`, with `PROCESSORS` equal to it | 0 | 5 of 5 entries, 2 to 3 with the switch | PASS |
| 3 | the five names are the matrix rows of `SPEEDGUN_SLOW_TESTS`, and the audit prints its ok line with the switch on and off | 0 | 7 rows, 5 matrix rows EQUAL; ok line at 139 and at 134 entries | PASS |
| 4 | three presets set the switch, two leave it out | 0 | 3 keys of 5 lanes, `own_key_count=3` | PASS |
| 5 | every building job restores before its first build, saves after its last, and no job lost a step | 0 | 11 building jobs, 0 order problems, 0 jobs with fewer steps | PASS |
| 6 | the `test` job configures `ci-ubuntu-test` and no `ctest` line carries `-L` or `-LE` | 0 | 8 `ctest` lines, 0 label hits | PASS |
| 7 | the format, spell and prose gates exit 0 | 0, 0, 0 | 0 findings, 30 sources | PASS |

VERDICT: PASS. All seven checks ran, each with exit 0 and the count its claim names: 5 entries against 0, 5 of 5 command vectors carrying `SPEEDGUN_TEST_JOBS`, 5 matrix rows equal to the 5 registered names with the audit ok line at both switch settings, 3 preset keys, 11 building jobs with 0 order problems and 0 jobs that lost a step, 8 `ctest` lines with 0 label filters, and 0 findings across the three gates.
