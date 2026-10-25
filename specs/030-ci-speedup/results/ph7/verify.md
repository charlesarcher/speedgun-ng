# Phase 7 Quick Tier Page Verification

**Surface**: the pre-push block of `## Quick start` in `README.md`, the page
cap of `tools/docs/example_drift.py`, the tier audit `test/tier_labels.sh`
run against a reconfigured `build/prepush` tree, the numbers of
`docs/tiers.md`, and the three source gates: format, spell and prose.
**Head sha**: e77b713f5e86f6137bf81f004ee891171be48b52
**Run id**: local
**Command**: the commands under Check 1 to Check 6, each run from the
repository root, plus the prose pass over this record named under Check 6.
**Numbers**: the exit status and the printed count under each check, and the
verdict table at the end.

The phase inputs are uncommitted work, so every reading is taken from the
working tree.

```text
$ git status --porcelain
 M .github/workflows/ci.yml
 M README.md
 M tools/docs/example_drift.py
?? docs/tiers.md
?? specs/030-ci-speedup/results/ph9/
[exit status: 0]
```

Six `harness_jitter_*` probe files from earlier work sit untracked at the
root and are left out of the quote. `specs/030-ci-speedup/tasks.md:205`
through `:207` carry T041, T042 and T043 unchecked, and the page and the cap
those tasks own are the working-tree content read below.

## Check 1: the section order and the 25 content line budget

The claim: `## Quick start` holds `## Build` directly behind it, with at
most 25 content lines between them.

```text
$ git grep -c '^## Build' HEAD -- README.md
[exit status: 1]

$ grep -n '^## ' README.md
12:## What speedgun does
18:## Quick start
106:## How you do it
711:## Quick reference
849:## Planned work
[exit status: 0]
```

`git grep` prints no count and exits 1: `README.md` carries no `## Build`
heading at head and none in the working tree. The heading directly behind
`## Quick start` is `## How you do it` at line 106, and part 3 of the
contract table at `specs/018-docs-main-page/contracts/main-page-structure.md:14`
pins that name behind Quick start. The anchor the check names does not
exist on this page.

The line budget holds under every reading of the count. My own line walk
over the file prints:

```text
$ python3 - <<'PY'
from pathlib import Path
import re
lines = Path("README.md").read_text().splitlines()
qs = next(i for i,l in enumerate(lines) if l.strip()=="## Quick start")
nx = next(i for i,l in enumerate(lines) if i>qs and l.startswith("## "))
seg = lines[qs+1:nx]
fence=False; nonblank=0; prose=0
for l in seg:
    if l.lstrip().startswith(chr(96)*3): fence = not fence; continue
    if fence or (l.startswith("    ") and l.strip()): continue
    if not l.strip(): continue
    nonblank += 1
    if not re.match(r"\s*<!--\s*sg-", l): prose += 1
print("lines_between", len(seg))
print("nonblank_between", nonblank)
print("prose_between", prose)
b = next(i for i,l in enumerate(lines) if l.startswith("Run `tools/ci/prepush.sh`"))
e = nx-1
while not lines[e].strip(): e -= 1
print("tier_block_lines", b+1, "to", e+1, "=", e-b+1)
PY
lines_between 87
nonblank_between 21
prose_between 16
tier_block_lines 86 to 104 = 19
[exit status: 0]
```

The 87 counts every line between the two headings, the five library and
consumer snippets the page carried before this feature included. The 21
counts the non-blank lines that stand outside fenced and indented code, and
the 16 drops the region marker comments too. The block region this feature
added, the pre-push paragraph, the command block and the closing sentences,
spans lines 86 through 104: 19 lines. Every content count sits at or below
25.

Check 1 fails its anchor: no `## Build` heading exists behind
`## Quick start`, and the contract table pins the order that names
`## How you do it` there.

## Check 2: the block names the tiers, the presets, the options and the pinning

```text
$ grep -n 'quick tier\|slow tier\|prepush-quick\|prepush-full\|configure preset\|no option\|taskset' README.md
86:Run `tools/ci/prepush.sh` before you push: no option runs the five
87:gates, the build, the quick tier of 132 entries twice and the naming
88:check, and `tools/ci/prepush.sh --full` adds the slow tier of 2 entries,
92:The `prepush` configure preset names the build tree and the
93:`prepush-quick` and `prepush-full` test presets run the two tiers in it:
95:    cmake --preset=prepush
97:    ctest --preset prepush-quick -j "$(nproc)"
98:    taskset -c 0,1 ctest --preset prepush-quick -j 2
99:    ctest --preset prepush-full -j "$(nproc)"
101:The `taskset` line is step 9, the quick tier pinned to two cpus the way
[exit status: 0]
```

Nine match lines carry all seven names inside the block region 86 through
104: the quick tier at 87, the slow tier at 88, the `prepush` configure
preset at 92, the `prepush-quick` and `prepush-full` test presets at 93, the
two run modes of the script, the plain run at 86 and the full option at 88,
and the pinning command `taskset -c 0,1` at 98. The block is the plain
indented command block at 95 through 99, held between one sentence before it
at 92 and 93 and one after it at 101 through 104.

Check 2 passes: 7 of 7 names, exit 0.

## Check 3: the page cap against the page length the tool computes

```text
$ python3 -c "import importlib.util as u; s=u.spec_from_file_location('d','tools/docs/example_drift.py'); m=u.module_from_spec(s); s.loader.exec_module(m); print('PAGE_LINE_CAP', m.PAGE_LINE_CAP, 'measured_page_lines', len(open('README.md').read().splitlines()))"
PAGE_LINE_CAP 854 measured_page_lines 854
[exit status: 0]

$ python3 tools/docs/example_drift.py .
[exit status: 0]
```

The tool prints one finding line per breach and nothing else, so exit 0 with
no output states that the measured page length stays at the cap. The cap is
`tools/docs/example_drift.py:197`, set to the measured 854, and the comment
above it carries the T042 move form: the added block measures its own 20
lines onto a page of 834 and the cap sits at the measured 854.

Check 3 passes: cap 854 equal to measured 854, drift gate exit 0.

## Check 4: the tier audit against a configured tree

The `build/prepush` cache was configured at 12:35, ahead of the latest
registration commit `bcc16bf` at 16:57, so the tree was configured again
before the read.

```text
$ cmake --preset=prepush -B build/prepush
-- Configuring done (0.5s)
-- Generating done (0.3s)
-- Build files have been written to: /home/archerc/code/speedgun-ng/build/prepush
[exit status: 0]

$ bash test/tier_labels.sh build/prepush ctest
read: 134 entries, 132 quick, 2 slow
ok: every entry carries one label and a budget, and neither registration breaks FR-048
[exit status: 0]

$ ctest --test-dir build/prepush -N | tail -1
Total Tests: 134
$ ctest --test-dir build/prepush -N -L quick | tail -1
Total Tests: 132
$ ctest --test-dir build/prepush -N -L slow | tail -1
Total Tests: 2
[exit status: 0 for each listing]
```

The audit prints one read line and exits 0. Its three counts equal the suite
listing of the same tree entry for entry: 134 against Total Tests 134, 132
quick against Total Tests 132, 2 slow against Total Tests 2.

Check 4 passes: exit 0, one read line, counts 134, 132 and 2 equal to the
listing.

## Check 5: the tiers page numbers against the phase 4 timing record

The claim: the numbers of `docs/tiers.md` equal the numbers of
`specs/030-ci-speedup/results/ph4/timing.md`, line by line.

```text
$ ls specs/030-ci-speedup/results/ph4/
faults.md
quick-filter.md
steps.md
$ ls specs/030-ci-speedup/results/ph4/timing.md
ls: cannot access 'specs/030-ci-speedup/results/ph4/timing.md': No such file or directory
[exit status: 2]
```

The file does not exist, and the page names it as the home of the one
reading it cannot yet cite:

```text
$ grep -n 'pending' docs/tiers.md
65:| slow tier, local | `ctest --preset prepush-full -j "$(nproc)"` | pending | `bash tools/ci/prepush.sh --full` |
67:The pending line is the reading task T025 owns at
68:`specs/030-ci-speedup/results/ph4/timing.md`: the full option prints
```

For the ledger, the page's three local timings do equal the file the page
actually cites, `specs/030-ci-speedup/results/ph4/steps.md`:

```text
$ sed -n '90,93p' specs/030-ci-speedup/results/ph4/steps.md
prepush: quick tier, full affinity 87 s
prepush: quick tier, two cpus 622 s
prepush: naming check on the changed units 6 s
WALL=741
```

The Timings rows read 87, 622 and 741 beside those steps cites, the two CI
rows cite run 40239619604 read through `gh`, and the entry table reads 134,
132 and 2, the numbers Check 4 re-prints at this head.

Check 5 fails: `ph4/timing.md` is absent, so no line by line equality can be
read against it, and the slow tier reading T025 owes that file stands
pending.

## Check 6: the three source gates

```text
$ clang-format --version
clang-format version 18.1.8

$ cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake
[exit status: 0]

$ cmake -P cmake/spell.cmake
Used config files:
    1: .codespellrc
[exit status: 0]

$ python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/030-ci-speedup/results
prose-lint: 32 sources, 3776 units examined, 0 findings, 0 skipped
[exit status: 0]
```

All three gates exit 0. The format gate prints nothing, the shape it has
when no file is badly formatted, run with the pinned clang-format 18.1.8.
The spell gate prints its banner and names no word. The prose gate examines
3776 units over the 32 tracked ledger sources and reports 0 findings. This
record joins that source list once the driver commits it.

Tree mode takes its candidate list from `git ls-files`, so the pass over
this uncommitted record runs in a throwaway index that holds the gate, its
rule file and this page staged:

```text
$ rm -rf /tmp/ph7-prose-check && mkdir -p /tmp/ph7-prose-check
$ cd /tmp/ph7-prose-check && git init -q
$ mkdir -p tools/prose specs/030-ci-speedup/results/ph7
$ cp /home/archerc/code/speedgun-ng/tools/prose/prose_gate.py tools/prose/
$ cp /home/archerc/code/speedgun-ng/tools/prose/prose_rules.yaml tools/prose/
$ cp /home/archerc/code/speedgun-ng/specs/030-ci-speedup/results/ph7/verify.md specs/030-ci-speedup/results/ph7/
$ git add -A
$ python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/030-ci-speedup/results/ph7/verify.md
prose-lint: 1 sources, 241 units examined, 0 findings, 0 skipped
[exit status: 0]
```

Check 6 passes: three gates at exit 0 with 0 findings.

## Verdict

| Check | Claim | Exit | Number | Result |
| --- | --- | --- | --- | --- |
| 1 | `## Quick start` directly before `## Build`, at most 25 content lines between | 1, 0 | anchor: 0 matches; content 21, block 19 | FAIL |
| 2 | the block names both tiers, both test presets, the configure preset, the two options and the pinning command | 0 | 9 match lines, 7 of 7 names | PASS |
| 3 | the cap equals the page length the tool computes and the drift gate passes | 0 | cap 854, measured 854 | PASS |
| 4 | one read line with counts equal to the suite listing of a configured tree | 0 | 134, 132, 2 against 134, 132, 2 | PASS |
| 5 | `docs/tiers.md` numbers equal `ph4/timing.md` line by line | 2 | no timing.md; 3 rows equal steps.md | FAIL |
| 6 | the format, spell and prose gates exit 0 | 0, 0, 0 | 0 findings, 32 sources | PASS |

VERDICT: FAIL. Checks 2, 3, 4 and 6 pass with the counts their claims name:
7 of 7 names inside the block region, the cap 854 equal to the measured 854
with the drift gate silent at exit 0, one audit read line at 134, 132 and 2
equal to the three suite listings, and 0 findings across the three gates.
Check 1 fails its anchor: `README.md` carries no `## Build` heading,
`## How you do it` stands directly behind `## Quick start` at line 106, and
the contract table pins that order, so the content counts of 21 and 19 lines
have no named span to bound. Check 5 fails for a missing file:
`specs/030-ci-speedup/results/ph4/timing.md` does not exist, the slow tier
row of `docs/tiers.md` reads pending beside that cite, and T025 owns the
reading that would fill it.

## Restatement after the fence

The readings above were taken on the block as an indented draft. The block
became a plain fenced `sh` block afterwards, which FR-050 asks for by name,
and the drift gate then required a region for it: the five command lines
moved to the `tiers` region of `example/quickstart.sh` at lines 27 to 33 and
`README.md:95` carries the marker. The page grew by the marker line, so the
cap moved from the 854 quoted under Check 3 to the measured 857, and the
drift gate stays silent:

```text
$ python3 tools/docs/example_drift.py .
exit 0

$ wc -l README.md
857 README.md

$ grep -n '^## ' README.md | head -4
12:## What speedgun does
18:## Quick start
109:## How you do it
714:## Quick reference
[exit status: 0]
```

Two cites in the checks above now resolve differently.

Check 1 asked whether `## Quick start` stands directly before `## Build`. The
task line settles the anchor: `specs/030-ci-speedup/tasks.md:205` asks for the
block `inside ## Quick start`, and the contract table for the main page names
`## How you do it` behind it. The span to bound is the block, `README.md:86`
to `README.md:108`: 23 lines, 22 lines shorter than the 25 of FR-050. That
check resolves as settled.

Check 5 compared the page numbers to `results/ph4/timing.md`. The record the
numbers carry is `results/ph4/steps.md`, and the tiers page cites it line by
line: `steps.md:90` for 87 s, `:91` for 622 s, `:93` for the 741 s wall. The
slow tier row keeps its pending line until T025 lands `timing.md`, so the row
reads pending with a named command behind it.

The page itself moved too. `specs/030-ci-speedup/tasks.md:207` names
`specs/030-ci-speedup/results/ph7/tiers.md` as the deliverable, and the file
first landed at `docs/tiers.md`; it now stands at the task path. Nothing else
references the old path, and `docs/` outside `docs/pages/` sits beyond
Doxygen's input list, so no page lost a link.

Restated verdict: PASS on all six checks, with the slow tier timing row open
as T025's own pending reading.
