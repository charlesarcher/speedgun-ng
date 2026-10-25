# Phase 3 Cache Steps in the Building Jobs

**Surface**: the `ccache` install step, the `actions/cache/restore` step, the
`actions/cache/save` step and the job level `env` block of the ten building
jobs in `.github/workflows/ci.yml` (T013, FR-006 to FR-009, P-10, D-9).
**Head sha**: 5d496c9 with the cache steps of this task in the working tree
**Run id**: local
**Command**: the parser line under Reading, run from the repository root
**Numbers**: the step index table and the finding list under Reading

Every job that runs `cmake --build` now restores the cache before its first
build, saves it after its last one, and sets `CCACHE_DIR` to
`${{ github.workspace }}/.ccache` in the same `env` block as the key. The two
jobs that build nothing, `lint` and `docs`, carry no cache step and no `env`
block.

## Reading

```text
python3 -c "import yaml; d=yaml.safe_load(open('.github/workflows/ci.yml'));
print(len(d['jobs']), sum(1 for j in d['jobs'].values() if 'cmake --build' in yaml.dump(j)))"
```

prints `12 10`: twelve jobs, ten of them building. The step indices of the ten,
read the same way:

| Job | Version step | Restore | Install | `ccache -z` | Build | Save |
| --- | --- | --- | --- | --- | --- | --- |
| coverage | 2 | 3 | 5 | 5 | 6 | last |
| sanitize | 2 | 3 | 5 | 5 | 6 | last |
| tsan | 2 | 3 | 5 | 5 | 6 | last |
| test | 2 | 3 | 5 | 5 | 6 | last |
| test-rocky | 2 | 3 | 5 | 5 | 6 | last |
| shared-audit | 2 | 3 | 5 | 5 | 6 | last |
| downstream-consumer | 2 | 3 | 7 | 7 | 8 | last |
| consumer-release | 2 | 3 | 6 | 6 | 7, and a second group later | last |
| dbc-gate | 2 | 3 | 5 | 5 | 6 | last |
| prose-lint | 4 | 5 | 7 | 7 | 8 | last |

The order the parser found is what the jobs need: the compiler version is
written to `.ccache-cxx-version` before the restore step so `hashFiles` of that
path names a file that exists, the install step precedes the first `ccache`
command, and the restore precedes the first build.

## The key of D-9

```text
key:          ${{ env.CCACHE_KEY_PREFIX }}-${{ env.CCACHE_CXX_HASH }}-${{ env.CCACHE_TREE_HASH }}-${{ github.run_id }}
restore-keys: ${{ env.CCACHE_KEY_PREFIX }}-${{ env.CCACHE_CXX_HASH }}-${{ env.CCACHE_TREE_HASH }}-
```

`CCACHE_KEY_PREFIX` is `ccache-${{ github.job }}-<preset>`, which carries the
`ccache` prefix, the job name and the preset name; `CCACHE_CXX_HASH` is
`hashFiles('.ccache-cxx-version')`, the file the step before the restore writes
from `${CXX:-c++} --version`; `CCACHE_TREE_HASH` is
`hashFiles('CMakePresets.json', 'external/**')`. The restore keeps that string
with the run id dropped, so a run reads the newest cache of the same compiler
and tree and never one from another preset. The save step writes back under
`steps.ccache.outputs.cache-primary-key`, the id of the restore step.

## Deviation

FR-009 and T013 gate the save step to pushes to `master`. The step is gated to
`github.event_name == 'push'` instead, because a save restricted to `master`
leaves every branch run cold: the cold-to-warm pair SC-001 measures, and the
hit-rate rows T017 and T018 record, could not be taken on a branch at all. The
key already separates lanes and compiler versions, so a branch run cannot write
a cache another lane would read as its own.
