# Phase 3 Cache Statistics in the Job Log

**Surface**: the `ccache -z` and `ccache -s` steps around each `Build` step of
`.github/workflows/ci.yml`, and the absence of `CCACHE_BASEDIR` and
`CCACHE_SLOPPINESS` from the file (T014, FR-009, FR-011).
**Head sha**: 5d496c9 with the cache steps of T013 in the working tree
**Run id**: local
**Command**: the parser line and the two greps under Reading, run from the
repository root
**Numbers**: the pair count and the finding list under Reading

`ccache -z` zeroes the counters and `ccache -s` prints them, so the pair around
a build puts the hit rate of that build in the job log. The parser walks the
steps of every building job and asks, for each maximal run of steps whose name
begins `Build`, whether the step before it is `ccache -z` and the step after it
is `ccache -s`.

## Reading

```text
python3 -c "import yaml; d=yaml.safe_load(open('.github/workflows/ci.yml')); ..."
```

| Quantity | Value |
| --- | --- |
| Building jobs | 10 |
| `Build` step groups across them | 11 |
| Groups with `ccache -z` immediately before | 11 |
| Groups with `ccache -s` immediately after | 11 |
| Steps naming `CCACHE_BASEDIR` | 0 |
| Steps naming `CCACHE_SLOPPINESS` | 0 |

`grep -c 'CCACHE_BASEDIR\|CCACHE_SLOPPINESS' .github/workflows/ci.yml` prints
`0`, and the same pattern over `CMakePresets.json` prints `0`, so nothing widens
the cache key behind the compiler's back and the key of D-9 stays the only
identity a lane restores under.

`consumer-release` is the one job with two build groups: the library build and
the `counters_trap_checked` fixture build of its ignore developer tree. Each
group carries its own pair, so the log shows two hit rates for that lane.

The `docs` job has a step named `Build docs`, which runs the documentation
tools and no compiler through ccache; it carries no pair, and no cache step at
all, which is the state `ph3/cache-steps.md` records for the two non-building
jobs.
