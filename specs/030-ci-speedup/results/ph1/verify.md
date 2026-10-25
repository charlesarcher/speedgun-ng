# Phase 1 Verification

**Surface**: the four phase-1 setup deliverables: the clang-format pass on
`test/source/harness_cli_table_test.cpp` (T001), the `ci-cache` configure
preset (T002), the `tools/ci/prepush.sh` scaffold (T003), and the
convention record for this directory (T004).
**Head sha**: 14ba1088feaf435fc22517423a9e40556b31a1a0
**Run id**: local
**Command**: the command block below, each line run from the repository
root
**Numbers**: the table under Results, one row per numbered check

No source file was changed to produce this record. A check that failed is
reported as it stands.

## The commands

```text
cd /home/archerc/code/speedgun-ng
cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake
git diff --name-only
python3 -c "import json;d=json.load(open('CMakePresets.json'));print([x['name'] for x in d['configurePresets']])"
cmake --preset=ci-cache
bash -n tools/ci/prepush.sh
./tools/ci/prepush.sh --help
./tools/ci/prepush.sh --nope
test -x tools/ci/prepush.sh
python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/030-ci-speedup/results
codespell specs/030-ci-speedup/results/
cmake -P cmake/spell.cmake
python3 tools/docs/example_drift.py .
```

## Results

| Check | Expect | Exit | Decisive line | Verdict |
| --- | --- | --- | --- | --- |
| 1 lint | 0 | 0 | no output, so no file is badly formatted | PASS |
| 2 dirty files | one name | 0 | three names, listed below | FAIL |
| 3 preset list | contains ci-cache | 0 | the list names ci-cache between ci-build and ci-ubuntu | PASS |
| 4 configure ci-cache | nonzero | 1 | CMake Error: Cannot use hidden configure preset | PASS |
| 5 scaffold | 0, 0, 2 | 0, 0, 2 | help printed, then usage with exit 2 for the unknown option | PASS |
| 6 executable bit | 0 | 0 | no output | PASS |
| 7 prose and spell | 0 each | 0, 0, 0, 0 | prose-lint 0 findings, codespell 0 hits, drift silent | PASS |

Check 2 named these files at the time of the run:

```text
CMakePresets.json
specs/030-ci-speedup/results/README.md
test/source/harness_cli_table_test.cpp
```

PHASE1: FAIL: check 2

## Check 2

The check reads the working tree, and the tree holds the phase-1 work of
two more nodes alongside the formatting change: `CMakePresets.json`
carries the `ci-cache` preset from T002, and
`specs/030-ci-speedup/results/README.md` is the convention record from
T004. The driver owns the commits for this branch, so none of it is
committed while the nodes are open. Under that arrangement a single dirty
file can only be observed after T001 is committed alone, and the check
reads three files at head 14ba108.

The formatting deliverable itself is sound: check 1 exits 0, and the diff
for that file is one hunk at line 886, the ternary chain T001 names. The
FAIL belongs to the ordering of the commits, to the state of the tree at
the moment the command ran.

## Gates over this record

Both commands from the convention record run over this directory, and
both exit 0 with zero findings:

```text
python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/030-ci-speedup/results
codespell specs/030-ci-speedup/results/
```

The prose gate in tree mode takes its candidates from `git ls-files`, so
an unstaged file is absent from the count it prints. This record is
untracked while the driver holds the commit, so the count reads 1 source
and covers the README. The same command was therefore re-run in a
throwaway repository at `/tmp/ph1-prose-check`, holding a copy of
`tools/prose/prose_gate.py`, `tools/prose/prose_rules.yaml` and the whole
`specs/030-ci-speedup/results/` directory, staged in that index. There
the summary reads 2 sources, 0 findings, which covers this record. The
main working tree keeps its own index: nothing outside this file was
staged or changed here.

## Path

The convention record fixes phase directories from `ph2` to `ph11`, so
`ph1` sits outside that range. The task brief for this verification names
`specs/030-ci-speedup/results/ph1/verify.md`, and neither spec.md nor
`contracts/presets.md` or `contracts/prepush-script.md` fixes a path for
a phase-1 record, so the brief's path stands.
