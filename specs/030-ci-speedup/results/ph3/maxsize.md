# Phase 3 Lane Cache Caps Against the 10 GB Budget

**Surface**: the job level `env` blocks of the ten building jobs in
`.github/workflows/ci.yml` (T016, FR-007).
**Head sha**: 92ca8c989ff97f4eda41516f2e71682e49c3ee13
**Run id**: local
**Command**: the line in the fenced block under Reading
**Numbers**: the table under Caps, with the sum under Budget

The head is the commit the working tree sits on. The ccache `env` blocks
of the ten building jobs are pending edits above that commit, so the
reading is of the working tree.

Every job that runs a `cmake --build` step carries one `CCACHE_MAXSIZE`
entry in the same `env` block that holds `CCACHE_DIR`. The `lint` job and
the `docs` job build nothing through ccache, so they carry no `env` block
and appear nowhere in the reading.

## Reading

The one command that reads the caps back out of the YAML, run from the
repository root:

```text
python3 -c "import yaml,re; d=yaml.safe_load(open('.github/workflows/ci.yml')); v={n:j.get('env',{}).get('CCACHE_MAXSIZE') for n,j in d['jobs'].items()}; print({k:x for k,x in v.items() if x})"
```

Its output for this state of the working tree:

```text
{'coverage': '1280MB', 'sanitize': '1280MB', 'tsan': '1280MB', 'test': '768MB', 'test-rocky': '768MB', 'shared-audit': '768MB', 'downstream-consumer': '640MB', 'consumer-release': '640MB', 'dbc-gate': '384MB', 'prose-lint': '384MB'}
```

## Caps

The build scope column names what the lane compiles, read from its
`Configure` and `Build` steps.

| Job | Preset | Build scope | Cap |
| --- | --- | --- | --- |
| coverage | ci-coverage | whole tree, coverage instrumentation | 1280MB |
| sanitize | ci-sanitize | whole tree, address and undefined sanitizer | 1280MB |
| tsan | ci-tsan | whole tree, thread sanitizer | 1280MB |
| test | ci-ubuntu | whole tree, uninstrumented | 768MB |
| test-rocky | ci-rocky | whole tree, uninstrumented | 768MB |
| shared-audit | ci-linux-audit | whole tree as shared libraries | 768MB |
| downstream-consumer | ci-linux-audit | library plus one consumer | 640MB |
| consumer-release | ci-linux-ignore | release library plus five named targets | 640MB |
| dbc-gate | ci-ubuntu | the dbc-gate target | 384MB |
| prose-lint | ci-ubuntu | the prose-lint-fixtures target | 384MB |

The three instrumented lanes sit above the plain ones because an
instrumented object file is larger than the uninstrumented object it came
from, so the same translation units occupy more cache. Below the plain
whole-tree lanes sit the two lanes that compile the library and a short
list of extra targets, then the two lanes that build one named target
each.

## Budget

The ten caps sum to 8192 MB, which is 8 GiB:

```text
3 x 1280 + 3 x 768 + 2 x 640 + 2 x 384 = 3840 + 2304 + 1280 + 768 = 8192 MB
```

`actions/cache` charges one repository budget shared by every key under
one ref, and all ten lanes restore under the ref of the run, so the sum
is the figure that must stay under the 10 GB budget.

| Quantity | Value |
| --- | --- |
| Sum of the ten caps | 8192 MB |
| Budget read as 10 GiB | 10240 MB, headroom 2048 MB |
| Budget read as 10 decimal GB | 9536 MB, headroom 1344 MB |

The budget holds under either reading of its unit, with at least 1344 MB
spare. A cap is a ceiling: `ccache` evicts the least used entries to
hold a lane under its own cap. The sum therefore describes the case
where every lane fills to its cap, the largest the lane set can reach.
Expected occupancy stays below that line.

No warm run exists at this head, so these figures follow the tier
reasoning above. The `ccache -s` output of the cold and warm records of
T017 and T018 is the reading that would show a lane running against its
cap, and only that reading justifies moving one.
