# Phase 5 Static-Analysis Job Carries Both Analyzers in Three Shards

**Surface**: the `static-analysis` job of `.github/workflows/ci.yml` (T026,
FR-015, FR-018, D-1, P-08).
**Head sha**: 4a8047a10d43fe582599db7583696f526a44e822
**Run id**: local
**Command**: the parse line under Reading, run from the repository root
**Numbers**: the parse output under Reading, the shards under Scopes, the key
under Cache key, the rows under Budget

The head is the commit the working tree sits on. The job is a pending edit
above that commit, so the reading is of the working tree. The job sits between
`tsan` and `test`, carries `needs: [lint]`, runs on `ubuntu-latest`, and holds
no `if:` condition, so no owner gate holds it back.

## Reading

The one command that reads the job back out of the YAML:

```text
python3 -c "import yaml; d=yaml.safe_load(open('.github/workflows/ci.yml')); j=d['jobs']['static-analysis']; print(j['needs'], j['strategy']['matrix']['include'] if 'strategy' in j else j.get('matrix'), [str(s.get('name')) for s in j['steps']])"
```

Its output for this state of the working tree:

```text
['lint'] [{'scope': 'library'}, {'scope': 'tests'}, {'scope': 'examples'}] ['None', 'Install analyzers', 'Record compiler version', 'Restore ccache', 'Configure', 'Zero ccache before Build', 'Build library scope', 'Build tests scope', 'Build examples scope', 'ccache stats after Build', 'Save ccache']
```

Eleven steps, in the order the contract lists: checkout, the analyzer install,
the compiler version record, the restore, the `ci-ubuntu` configure,
`ccache -z`, the shard build, `ccache -s`, the save. The leading `None` is the
unnamed checkout step. The matrix is an `include` list of three entries, each
carrying one `scope` value, so all three values land in the printed shape.

## Scopes

| Shard | Build scope | Command |
| --- | --- | --- |
| library | the two library targets | `cmake --build build --target speedgun-ng_speedgun-ng speedgun-ng_harness -j "$(nproc)"` |
| tests | the `test` directory | `cmake --build build/test -j "$(nproc)"` |
| examples | the `example` directory | `cmake --build build/example -j "$(nproc)"` |

Selection is step level `if:` conditions on `matrix.scope`: one build step per
scope, each named for its shard. A single run block that switches on the matrix
value was the alternative; three gated steps keep each command on one line and
name the shard in the job log of the shard that ran it.

The names and the paths were read from a configured `ci-ubuntu` tree on this
host. `make -C build help` lists `speedgun-ng_speedgun-ng` and
`speedgun-ng_harness`, and `build/test/Makefile` and `build/example/Makefile`
both exist, so each command addresses a target directory the configure creates.

## Analyzers

`ci-ubuntu` inherits the `clang-tidy` and `cppcheck` presets, whose cache
variables are:

```text
CMAKE_CXX_CLANG_TIDY = clang-tidy;--header-filter=^${sourceDir}/;--exclude-header-filter=^${sourceDir}/external/
CMAKE_CXX_CPPCHECK   = cppcheck;--inline-suppr
```

Both tools therefore run in front of the compiler for every translation unit a
shard compiles. No step in the job adds a check filter, a quiet flag, a header
filter change or any other narrowing, which is what keeps the finding set of the
three shards comparable against master's list (FR-016, FR-017, SC-011).

The install step installs `clang-tidy`, `cppcheck` and `ccache`: the three
packages a shard needs at build time. `CMakeLists.txt` consults no package at
configure time beyond the compiler, and every `python3` mention in
`test/CMakeLists.txt` sits inside an `add_test`, so it runs when a test runs, a
thing this job does none of.

## Cache key

The job level `env` block:

```text
CCACHE_DIR        = ${{ github.workspace }}/.ccache
CCACHE_KEY_PREFIX = ccache-${{ github.job }}-${{ matrix.scope }}-ci-ubuntu
CCACHE_MAXSIZE    = 256MB
```

The key is
`ccache-static-analysis-<scope>-ci-ubuntu-<cxx-hash>-<tree-hash>-<run-id>` and
the restore key is that string with the run id dropped and a trailing hyphen,
which is the ph3 key of D-9 with `matrix.scope` added to the prefix. All three
shards run under one job name, so `github.job` gives them an identical string;
the matrix value in the prefix is what stops two shards saving one key. The
`matrix` context is available in a job level `env` block, so the prefix is
complete before the restore reads it.

The restore step has id `ccache`, uses `actions/cache/restore@v4`, and carries
`CCACHE_CXX_HASH` from `hashFiles('.ccache-cxx-version')` and
`CCACHE_TREE_HASH` from `hashFiles('CMakePresets.json', 'external/**')`. The
version step runs ahead of it and writes that file with
`${CXX:-c++} --version | head -n 1`, so the hash names a file that exists. The
save step is last, uses `actions/cache/save@v4`, is gated to
`github.event_name == 'push'`, and writes under
`steps.ccache.outputs.cache-primary-key`.

## Budget

| Quantity | Value |
| --- | --- |
| Cap of one shard | 256 MB |
| Three shards | 768 MB |
| Ten caps of `ph3/maxsize.md` | 8192 MB |
| Eleven jobs together | 8960 MB |
| Budget read as 10 GiB | 10240 MB, headroom 1280 MB |
| Budget read as 10 decimal GB | 9536 MB, headroom 576 MB |

Each shard runs on its own runner, with its own `CCACHE_DIR` and its own key, so
its cap is charged on its own: three shards at 256 MB add 768 MB to the ten caps
that sum to 8192 MB in `specs/030-ci-speedup/results/ph3/maxsize.md`, which
gives 8960 MB. That sits under a 10 GiB budget with 1280 MB spare, and under the
tighter decimal reading with 576 MB spare. The job carries one `env` block, so a
per-job read of the caps prints 8448 MB; the figure `actions/cache` charges is
8960 MB, and the per-shard count is the one that matters.
