# The commit and constitution check

T049 of specs/037-execution-graphs Phase 8: the lane's commits read
against the Pull Request Quality template, every hard gate of
constitution VIII names a landing commit in the lane, and the
reservations of the spec's Spec identity section read against the
landed tree (constitution VIII, IX, XI, Pull Request Quality).

- Head: `4c9f3b9`, the feature head the readings stand on.
- Base: `origin/master` at `1049cce`, the merge base of the lane.
- Lane: 67 commits over `origin/master..HEAD`.
- Suite: 151 of 151 at `66eca8d`, the last head a build stood on. The
  two commits above it carry Markdown alone.
- Scope: this record touches no file outside
  `specs/037-execution-graphs/results/`.
- Landing: the record file first landed in `e853f7f`, the sibling's
  commit, which took it as an untracked file through a directory path.

## The commit template

The gate's commit check over the lane:

```
$ python3 tools/prose/prose_gate.py --check commit --mode range --base origin/master --head HEAD
prose-lint: 67 sources, 1513 units examined, 2 findings, 0 skipped
exit 1
```

Both findings stand in `e853f7f`, the interrupt record:

```
commit e853f7f: XI1.DOUBLE-HYHEN family=XI.1 '--': use a colon,
semicolon, comma, or parentheses (Principle XI.1)
```

The site is that commit's Verify line, which quotes a command whose
flags are double hyphens. A commit body carries no code-span
exemption, so the quoted flags read as findings. The other 66 messages
of the lane read clean, and the fix is a message rewrite, the history
being the orchestrator's.

The tree check:

```
$ cmake -P cmake/prose-lint.cmake
prose-lint: 120 sources, 7873 units examined, 2 findings, 0 skipped
exit 1
```

The wrapper runs the commit check beside the prose scan, so it answers
the same two findings. The prose scan on its own reads clean:

```
$ python3 tools/prose/prose_gate.py --check prose --mode tree
prose-lint: 792 sources, 143015 units examined, 0 findings, 0 skipped
exit 0
```

The same reading of the lane's messages, taken from
`git log --format=%B`:

| Check | Reading |
| --- | --- |
| Title shape | 67 of 67 read `<Section>: <one line imperative>`; the sections are `CI`, `CMake`, `Docs`, `Harness`, `Meta`, `runner`, `test` |
| Title length | longest 49 characters against the cap of 50 |
| Blank line | 67 of 67 separate title from body |
| Body wrap | longest body line 72 columns against the cap of 72 |
| `Refs:` footer | 67 of 67 carry `Refs: specs/037-execution-graphs` |
| `Approved-by:` footer | 67 of 67 carry `Approved-by: OmO`, the reviewer's own name |
| `Fixes #n` | none: `gh issue list --state open` and `gh pr list --state open` each answer no issue, so no issue exists to name |
| Prose rules | 2 findings, both at `e853f7f`, named above |

The prose gate at `15a4954` reported 26 findings in the lane's commit
messages and named the orchestrator, the owner of the history, as the
one to clear them. The message surgery closed them: an interactive
rebase over the whole lane, `HEAD@{88}` to `HEAD@{1}` of the reflog,
rewrote messages alone. Every commit keeps its tree, so the lane
carries the content every gate ran on.

## Linearity and atomicity

| Check | Reading |
| --- | --- |
| `git merge-base origin/master HEAD` | `1049cce`, equal to `origin/master` |
| `git log --oneline origin/master..HEAD` | 67 lines |
| `git rev-list --count --first-parent origin/master..HEAD` | 67, so no commit joins two lines |
| `git log --merges origin/master..HEAD` | empty |

The lane is a linear sequence on top of `origin/master`. The base moved
from `5443448`, the hash the spec records at branch time, to `1049cce`
through the rebase the surgery ran, and the reservations moved with it.

Each commit names one idea. The test task of each module lands before
the code task it turns green, each gate task lands as its own commit,
and the wave closes land after the waves they close: `3ce30de` closes
wave one, `6749036` wave two, `65653f4` wave three, `dd02b6c` wave
four.

## The gate-to-commit table

Every hard gate of constitution VIII names the commit that carries its
clearing. The surgery rewrote the lane's history, so the pre-surgery
hash stands beside each current one, and the two share a tree.

| Gate | Task | Lane commit | Pre-surgery | The verdict the commit records |
| --- | --- | --- | --- | --- |
| `format-check` | T039 | `33a8880` | `3d43b3e` | clang-format 23.1.4 names 111 files, 17 of them the lane's, and every finding on them stands cleared |
| `spell-check` | T040 | `06d06b7` | `f03aba2` | eleven findings, each cleared: two spellings, one dictionary entry, eight in the orchestrator's file |
| prose lint | T041 | `15a4954` | `31f4b78` | one tree finding cleared, and the 26 message findings cleared by the surgery |
| clang-tidy, cppcheck, name-check | T042 | `09ace2b`, `ed562d7` | `1cfff3f`, `80f367f` | 231 units behind both analyzers, the name check reads zero findings, three findings cleared |
| ASan and UBSan | T043 | `4d7523b` | `450fcdd` | the `ci-sanitize` build compiles and runs clean; the libstdc++ `std::regex` report carries a written P2 justification |
| tsan | T044 | `aa45b43` | `aac77aa` | the suite passes under the thread sanitizer, and 30 repeats of the graph arms report no race |
| coverage | T045 | `1970377`, `c234442` | `bb8dbad`, `7b5a157` | 100.0% lines and 100.0% branches on the graph paths, and the gate reads green at the head |
| docs gates | T046 | `887d3da`, `b2eca38` | `29e96f4`, `2e53b02` | 14 of 14 docs entries pass, the shape fold of the graph sample included |
| two-cpu suite and time source | T047 | `3a303ee` | `81d0f4a` | 151 of 151 on cpus `0,1`, `time_source_gate` green, recorded in [two-cpu.md](./two-cpu.md) |
| DBC gate | T050 | `b50b56b` | `97ed807` | doc-gate and pair-gate each read 317 interfaces 0 gaps, and 13 of 13 fixtures pass |

The builds and the suite ride the wave gates. The `ci-rocky` release
build stands behind every record above, the `ci-ubuntu` build stands
behind the analyzer gate, and `ctest` reads 151 of 151 at the head. The
performance metric of VII stays in the constitution's Open deferrals,
the way the plan's Constitution Check records.

## The pre-surgery hashes

`git merge-base --is-ancestor` answers no for every pre-surgery hash in
the table. Each names a commit that exists in the object store, and
each shares its tree with exactly one lane commit, the mapping the
table records. The surgery rewrote messages, so the hashes moved while
the content stood.

One hash of the task's list reads unresolvable: `3d43b78`. The format
gate commit is `3d43b3e`, whose lane commit is `33a8880`.

## The reservations

The spec's Spec identity section plans each number as a reservation
that reads the landed master at rebase time. Pull requests #57 and #58
landed, so the lane took the later value in every row.

| Reservation | Planned at branch time | The tree reads | Site |
| --- | --- | --- | --- |
| Project version | `0.23.0`, then `0.24.0` | `0.25.0` | `CMakeLists.txt:30` |
| harness `SOVERSION` | `14`, then `15` | `16` | `CMakeLists.txt:330` |
| archive `SOVERSION` | `4` | `4` | `CMakeLists.txt:95` |
| Report schema | `1.7`, then `1.8` | `1.9` | `source/harness/json_report.cpp:40` |
| `--graph` option value | `297` | `298` | `source/harness/detail/options.hpp:99` |

The `os-noise` row of #58 holds `297` at `options.hpp:94`, and the
table contract test names both values and their uniqueness.

## The task list

`tasks.md` is the orchestrator's file, and this record leaves it
untouched. Its worktree state at this head reads T039 to T045, T047,
T048, T050, T051 and T052 ticked. T049 lands with this record. T046
stands unticked while its work landed: `887d3da` names 14 of 14 docs
entries passing, and `b2eca38` carries the shape fold its fold needs.

## The standing limitation

T048 recorded one limitation, and it stands at this head. The stop
request's arrival at the graph gate has no caller in the entry point:

```
$ grep -rn settleStopped source include
source/harness/detail/graph.hpp:249:  auto settleStopped() -> void;
source/harness/graph_gate.cpp:159:auto settleStoppedAll(const Graph& graph, States& states) -> void
source/harness/graph_gate.cpp:314:auto GraphGate::settleStopped() -> void
source/harness/graph_gate.cpp:318:    settleStoppedAll(m_graph, m_states);
exit 0
```

The declaration and the definition stand, and the suite drives the
path. No scripted run of the entry point settles a stopped word, so
every scripted run's record carries an empty `stopped` array. Wiring
the request into the entry point changes the stop path's behavior, and
that needs its own spec cycle. The stopped words reach the console
block and the record through the gate arm, the assembly arm and the
console arm, the way [quickstart.md](./quickstart.md) records.

The production drive of the stop path answers the same way. The docs
example over the chain, signalled two seconds into a long iteration
count, exits 1 with the skip line
`bmChain/threads:3 SKIPPED: interrupted by the user (SIGINT)`, prints
no `contexts=` line, so no graph block stands beside it, and its
document names every node `finished` with an empty `stopped` array
(FR-019, FR-022, FR-023, specs/020 FR-050).

## The plan's constitution table

Each row of the plan's Constitution Check reads against the landed
tree, beside the artifact that answers it.

| Principle | Verdict | The artifact |
| --- | --- | --- |
| I. Standard-first | PASS | no compiler extension joins, and the analyzer gate at `09ace2b` builds clean |
| II. Design by contract | PASS | the DBC gate at `b50b56b`: 317 interfaces, 0 gaps, both halves |
| III. R-DCUT | PASS | every task names its FRs, its module, its test and its commit title |
| IV. Documentation | PASS | the docs gates at `887d3da` read 14 of 14, and the main-page section carries its example run |
| V. Style | PASS | `format-check` at `33a8880` and the name check at `09ace2b` |
| VI. Coverage | PASS | 100.0% lines and branches on the graph paths at `c234442`, and the DBC matrix reads complete |
| VII. Performance | PASS with the standing deferral | the gate stands outside every measured window, and the baseline stays in the constitution's Open deferrals |
| VIII. Hard gates | PASS apart from the prose gate | the table above names a landing commit for every gate; the prose gate reads 2 findings at the head, both in `e853f7f` |
| IX. Spec-driven | PASS | spec, plan, research, data model, contracts, quickstart and results stand in the feature directory |
| X. Anti-slop | PASS | one row, one reader, one gate, one record, one seam, and `c234442` deletes an unreachable arm |
| XI. Prose | PASS over the tree, 2 findings over the messages | the prose scan reads 0 findings over 792 sources, and the message check reads the two at `e853f7f` |

No gate weakens and no gate strengthens. Complexity Tracking stays
empty.
