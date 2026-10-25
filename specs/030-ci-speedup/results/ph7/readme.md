# T041: the pre-push block on the main page

**Head sha**: e77b713 for the workflow reading; the block lands with this commit
**Run id**: none; the evidence is local gates on the page itself
**Command**: `python3 tools/docs/example_drift.py .` and
`python3 tools/prose/prose_gate.py --check prose --mode tree`, each from the
repository root
**Numbers**: `README.md:86` to `README.md:108`, 23 lines, inside `## Quick start`

## The block

The section `## Quick start` runs from `README.md:18` to `README.md:108`; the
next heading, `## How you do it`, stands at `README.md:109`. The new material
fills the tail of that section: a sentence naming `tools/ci/prepush.sh` and what
no option costs, a sentence naming the `prepush` configure preset and the
`prepush-quick` and `prepush-full` test presets, a plain fenced `sh` block with
the five command lines, and a sentence that puts the `taskset` line at step 9 and
names CI as authoritative. The `quick` tier is named with its 132 entries and
the `slow` tier with its 2, the figures `results/ph7/tiers.md` reads off the two
configured trees.

23 lines carry the block, against the 25 of FR-050: 11 lines of sentences, 5
command lines, the marker comment, two fences and 4 blank lines.

## Why the fence carries a region marker

The first draft set the commands in an indented block. A plain fence on the main
page is a copy of an example region, and the drift gate says so when one lacks a
marker:

```text
README.md: block at line 95 carries no region marker
README.md: 856 lines, the cap is 854
drift rc=1
```

So the five lines live in `example/quickstart.sh`, in the `tiers` region at
lines 27 to 33, beside the `library`, `consumer` and `options` regions the page
already copies. The region file keeps the house rule that each region is a block
a reader copies: every flag stands spelled out and the line runs as written from
the repository root. `README.md:95` is the marker comment
`<!-- sg-example: example/quickstart.sh#tiers -->` that binds the fence to it.

## Verification

```text
$ python3 tools/docs/example_drift.py .
exit 0

$ python3 tools/prose/prose_gate.py --check prose --mode tree
prose-lint: 662 sources, 119648 units examined, 0 findings, 0 skipped
exit 0

$ cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake
exit 0

$ cmake -P cmake/spell.cmake
exit 0
```

The page cannot carry more: `tools/docs/example_drift.py:197` holds the line cap
and `README.md` measures 857 lines, the reading T042 moves the cap to.

VERDICT: PASS. One fenced `sh` block with a sentence each side, inside
`## Quick start`, naming both tiers and `tools/ci/prepush.sh` in 23 lines.
