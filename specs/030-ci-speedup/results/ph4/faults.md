# Phase 4 Fault Paths

**Surface**: the fault paths of `tools/ci/prepush.sh` that FR-030 names: the
unknown option, the missing tool, the formatter version stop, the failing-step
line, and the `--self-check` option that drives all four in place.
**Head sha**: 92ca8c9 with the fault-path block of this task in the working
tree
**Run id**: local
**Command**: the four lines under The paths and the self-check option of the
same script, run from the repository root
**Numbers**: the exit status beside each path, and the four `self-check:` lines

FR-030 wants a failing local run to say which check failed, in the same shape
the CI log gives. Each path below was driven directly, with no repository file
touched, and each printed the line the contract asks for before it exited.

## The paths

| Path | Command | Line printed | Exit |
| --- | --- | --- | --- |
| unknown option | `bash tools/ci/prepush.sh --bogus` | `unknown option: --bogus` then the usage block | 2 |
| help | `bash tools/ci/prepush.sh --help` | the usage block, ending with the exit table | 0 |
| missing tool | `env PATH=/usr/bin:/bin bash tools/ci/prepush.sh` | `prepush: missing tool: cmake` | 1 |
| failing step | `bash tools/ci/prepush.sh` with a spelling finding in the tree | `prepush: spell gate failed with 1: cmake -P cmake/spell.cmake` | 1 |
| formatter version | a `clang-format` stub printing `version 9.9.9` on `PATH`, driven by the self-check | the version line naming the expected version, before any format run | 1 |

The failing-step row is a line `/tmp/ph4b/p1.log` printed for real: the tree
carried one codespell finding in
`specs/030-ci-speedup/results/ph2/tier-list.md`, the spell gate stopped the run
at step 4 and the script exited 1 with the gate name, the exit status and the
command line on one line. Steps 5 and after printed nothing, which is the
fail-fast shape FR-030 asks for.

## The self-check

```text
$ bash tools/ci/prepush.sh --self-check
self-check: unknown option pass
self-check: missing tool pass
self-check: formatter version pass
self-check: failing step pass
$ echo $?
0
```

`self_check` (`tools/ci/prepush.sh:135`) drives each path in a subshell with a
stub on `PATH` and checks the exit status it expects, so the option exits 0
only when all four behaved and prints a `fail` line for the one that did not.
It writes only to `mktemp -d` directories under `/tmp` and removes them at
exit, so no repository file and no build tree is touched. The node's own run of
the same option is `/tmp/ph4/selfcheck.log` with `selfcheck.rc`, and it reads
the same four lines.

## Cleared in the same commit

Two Principle XI findings inside this script, both from the commit that filled
in the step bodies, are cleared here: a double hyphen in the exit-status line
of the header (XI.1) and a contrastive in the comment above `require_tools`
(XI.2). `python3 tools/prose/prose_gate.py --check prose --mode tree --paths
tools/ci/prepush.sh` now reads `0 findings`, and `bash -n` exits 0.
