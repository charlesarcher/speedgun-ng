# Phase 5 Analyzer Job Graph Verification

**Surface**: the `static-analysis` job, the `test` job and the ccache steps
of `.github/workflows/ci.yml`, read as one graph beside `CMakePresets.json`,
`.clang-tidy` and the records under `specs/030-ci-speedup/results/ph5`.
**Head sha**: 898e038dd4ceb9d2761ac746ae43e1933a5e51a8
**Run id**: local
**Command**: the seven check blocks below plus the gate commands under
Check 7, each one run from the repository root
**Numbers**: 13 jobs, 11 building jobs, 3 shards, 15 shard steps, 7 passing
checks, no failing check

Every check read one working-tree state, the one the object hashes below pin.
The branch moved four times while this node ran, and the phase arrived in
steps: at 88c649f the shard job was absent, at 6767c20 the test job carried
`ci-ubuntu-test`, at 4a8047a the shard job existed with eleven steps, and at
the head below it carries fifteen with the report steps of T028. At this
reading the workflow change sits above the head commit as a working-tree edit,
and the blob hash above pins the bytes every check read.

```console
$ date -Is
2026-10-25T16:09:42-05:00
$ git rev-parse HEAD
898e038dd4ceb9d2761ac746ae43e1933a5e51a8
$ git hash-object .github/workflows/ci.yml CMakePresets.json
ed8ed1022b1bf3373b36d63e2d7ac263b4a64d53
f15a466deb573eed1a210e647e6d7c12b00fee63
$ git status --porcelain
 M .github/workflows/ci.yml
 M specs/030-ci-speedup/results/ph3/verify.md
 A specs/030-ci-speedup/results/ph5/analyzer-job.md
 A specs/030-ci-speedup/results/ph5/shard-artifacts.md
 A specs/030-ci-speedup/results/ph5/test-job.md
 A specs/030-ci-speedup/results/ph5/verify.md
 M test/source/harness_cli_table_test.cpp
 M test/source/harness_json_load_test.cpp
?? specs/030-ci-speedup/results/ph4/timing.md
$ ls specs/030-ci-speedup/results/ph5
analyzer-job.md  shard-artifacts.md  test-job.md  verify.md
```

## The records of the phase

| Record | State |
| --- | --- |
| `analyzer-job.md` | present: the reading of T026 |
| `test-job.md` | present: the reading of T027 |
| `shard-artifacts.md` | present: the reading of T028 |

The three records match the graph these checks print, at the same head and
the same uncommitted workflow edit: eleven steps of T026 and T027 plus the
four report steps of T028, `needs: [lint]`, three `scope` values, the
`ci-ubuntu` configure in the shard job and the `ci-ubuntu-test` configure in
the `test` job.

## Check 1: job count

```console
$ python3 -c "import yaml; d=yaml.safe_load(open('.github/workflows/ci.yml')); print(len(d['jobs']))"
13
```

The job names in file order, with the line count of the file:

```text
lint,coverage,sanitize,tsan,static-analysis,test,test-rocky,shared-audit,downstream-consumer,consumer-release,dbc-gate,prose-lint,docs
lines: 1236
```

`static-analysis` sits between `tsan` and `test`, the position the job graph
contract gives it.

## Check 2: the cache invariant walk

Steps are read by name. `lint` and `docs` hold no `cmake --build` step, so
they carry no cache duty and print SKIP. For the other eleven the walk wants
the restore ahead of the first build, the save behind the last one, both
`CCACHE_DIR` and `CCACHE_MAXSIZE` in the job `env`, `ccache -z` directly
ahead of each maximal run of steps named `Build` and `ccache -s` directly
behind it.

```text
lint                 SKIP no cmake --build step
coverage             PASS
sanitize             PASS
tsan                 PASS
static-analysis      PASS
test                 PASS
test-rocky           PASS
shared-audit         PASS
downstream-consumer  PASS
consumer-release     PASS
dbc-gate             PASS
prose-lint           PASS
docs                 SKIP no cmake --build step
```

All eleven building jobs hold the invariant. The shard job is among them:
`Zero ccache before Build` sits ahead of the three `Build` steps and
`ccache stats after Build` sits behind them.

## Check 3: one cache key per shard

```text
PASS CCACHE_KEY_PREFIX: ccache-${{ github.job }}-${{ matrix.scope }}-ci-ubuntu
matrix scopes ['library', 'tests', 'examples'] -> 3 shards, so 3 keys
```

The prefix names `matrix.scope`, so the three shards run under three distinct
keys and no shard saves over the entry of another shard.

## Check 4: the test job is analyzer-free

```text
PASS names no analyzer
PASS configures ['ci-ubuntu-test']
```

Neither analyzer name appears in any step of the job, and the single preset
line names `ci-ubuntu-test`. That preset inherits `ci-build`, `ci-linux`,
`dev-mode` and `ci-cache` in `CMakePresets.json`, so the compiler, the flags,
the developer mode and the ccache launcher stay.

## Check 5: the shard job shape

```text
PASS needs=['lint']
PASS matrix scopes=['library', 'tests', 'examples']
PASS artifact name=analyzer-findings-${{ matrix.scope }}
PASS upload if='always()'
PASS findings gate steps=[(12, 'Fail on findings', 'test', 'test')]
roster:
   0  actions/checkout@v4            if=None
   1  Install analyzers              if=None
   2  Record compiler version        if=None
   3  Restore ccache                 if=None
   4  Configure                      if=None
   5  Zero ccache before Build       if=None
   6  Build library scope            if=matrix.scope == 'library'
   7  Build tests scope              if=matrix.scope == 'tests'
   8  Build examples scope           if=matrix.scope == 'examples'
   9  ccache stats after Build       if=None
  10  Count translation units        if=always()
  11  Collect findings               if=always()
  12  Fail on findings               if=always()
  13  Upload findings                if=always()
  14  Save ccache                    if=github.event_name == 'push'
```

Five assertions pass. The two build-scope assertions of check 2 aside, the
job holds 15 steps: the eleven of T026 and T027 plus `Count translation
units`, `Collect findings`, `Fail on findings` and `Upload findings`. The
artifact name carries `matrix.scope`, the upload carries `if: always()`, and
the gate step ends with `test "$n" -eq 0`, the exit that FR-016 leans on. The
gate detector reads every step body for a construct that fails a step: a bare
`test`, an `exit` with a nonzero status, a `::error::` annotation, or an
analyzer exit-code flag.

```text
--- step 6: Build library scope  if=matrix.scope == 'library'
set -euo pipefail
cmake --build build --target speedgun-ng_speedgun-ng speedgun-ng_harness \
  -j "$(nproc)" 2>&1 | tee analyzer-build.log
--- step 12: Fail on findings  if=always()
n=$(wc -l < findings.txt)
printf 'shard %s: %s findings\n' '${{ matrix.scope }}' "$n"
test "$n" -eq 0
```

The three Build steps pipe through `tee` into `analyzer-build.log` and each
one opens with `set -euo pipefail`. The probe below shows what the option
carries: a bare pipeline reports the status of `tee`, so a failed build would
look green and the report steps would read a truncated log.

```console
$ bash -c 'set -e; false | tee /tmp/ph5pipe.log'; echo bare_pipeline_exit=$?
bare_pipeline_exit=0
$ bash -c 'set -eo pipefail; false | tee /tmp/ph5pipe.log'; echo pipefail_pipeline_exit=$?
pipefail_pipeline_exit=1
```

`.clang-tidy:21` carries `WarningsAsErrors:
'readability-identifier-naming'`, so the planted finding of FR-016 is an
error and stops the compile ahead of the report steps. The probe below runs
one naming violation through the project config and through a bare check list
on the same host. The project config exits 1, the bare list exits 0, which is
what makes that `WarningsAsErrors` line load bearing.

```console
$ clang-tidy --checks=-,readability-braces-around-statements ph5probe.cpp -- -std=c++23 > /dev/null 2>&1; echo bare_check_exit=0
bare_check_exit=0
$ clang-tidy --config-file=.clang-tidy /tmp/ph5cfg/bad.cpp -- -std=c++23 2>&1 | tail -n 6; echo repo_config_exit=${PIPESTATUS[0]}
      |     badName
/tmp/ph5cfg/bad.cpp:1:5: warning: use a trailing return type for this function [modernize-use-trailing-return-type]
    1 | int bad_name() { return 1; }
      | ~~~ ^
      | auto           -> int
1 warning treated as error
repo_config_exit=1
```

## Check 6: no analyzer argument beyond the presets

```text
PASS no step adds --checks, --quiet or header-filter
```

The presets own both argument sets, so the workflow adds nothing to them:

```text
"CMAKE_CXX_CPPCHECK": "cppcheck;--inline-suppr"
"CMAKE_CXX_CLANG_TIDY": "clang-tidy;--header-filter=^${sourceDir}/;--exclude-header-filter=^${sourceDir}/external/"
```

## The runner label of the shard job

Twelve jobs pin `ubuntu-26.04` and the shard job pins `ubuntu-latest`. The
label decides the clang-tidy build the finding set comes from, so that line
is worth a look before the finding diff of T029.

```console
$ python3 -c "import yaml; j=yaml.safe_load(open('.github/workflows/ci.yml'))['jobs']; print([(k, v.get('runs-on')) for k, v in j.items()])"
[('lint', 'ubuntu-26.04'), ('coverage', 'ubuntu-26.04'), ('sanitize', 'ubuntu-26.04'), ('tsan', 'ubuntu-26.04'), ('static-analysis', 'ubuntu-latest'), ('test', 'ubuntu-26.04'), ('test-rocky', 'ubuntu-26.04'), ('shared-audit', 'ubuntu-26.04'), ('downstream-consumer', 'ubuntu-26.04'), ('consumer-release', 'ubuntu-26.04'), ('dbc-gate', 'ubuntu-26.04'), ('prose-lint', 'ubuntu-26.04'), ('docs', 'ubuntu-26.04')]
```

## Check 7 and the record gates

The spell gate walks the whole tree. The prose gate runs in tree mode over
every tracked file under `specs/030-ci-speedup/results`, then over this record
alone. `git add -N` of this record came first, which is what makes it visible
to tree mode. The source count of the broad run grows while sibling nodes land
more records in that directory.

```console
$ git add -N specs/030-ci-speedup/results/ph5/verify.md
$ python3 tools/prose/prose_gate.py --check prose --mode tree --paths $(git ls-files specs/030-ci-speedup/results)
prose-lint: 22 sources, 2158 units examined, 0 findings, 0 skipped
exit=0
$ cmake -P cmake/spell.cmake
Used config files:
    1: .codespellrc
exit=0
$ python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/030-ci-speedup/results/ph5/verify.md
prose-lint: 1 sources, 220 units examined, 0 findings, 0 skipped
exit=0
```

Both gates are clean: 0 findings for this record alone, 0 findings across the
results tree, and the spell gate at exit 0.

## Verdict

Seven checks pass at the pinned state: 13 jobs, the cache invariant in all
eleven building jobs, three shard keys, an analyzer-free `test` job on
`ci-ubuntu-test`, the five shard-job assertions, no analyzer argument beyond
the presets, and both source gates clean for this record.

Two readings sit outside the seven checks and belong to the next node.
The shard job pins `ubuntu-latest` while twelve jobs pin a fixed label.
`shard-artifacts.md` records no GitHub run, so the finding union of T029 and
the planted run of T030 stay open until a run exists.

VERDICT: PASS
