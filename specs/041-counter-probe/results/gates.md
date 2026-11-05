# The cheap gates over the head (T058)

Head `5bebfd57`, base `origin/master` `858c55d9`, host `Linux 7.3.1-1-cachyos x86_64`,
compiler `c++ (GCC) 16.2.1 20260810`. Each row names the command and what it answered,
at that head, in a release developer-mode tree from `cmake --preset=ci-rocky`. The three
heavy rows read elsewhere: `results/sanitizers.md`, `results/coverage-head.txt`, and
`results/analyzer.md`.

| Gate | Command | Answer |
| --- | --- | --- |
| format | `cmake -P cmake/lint.cmake` | exit 0 |
| spell | `cmake -P cmake/spell.cmake` | exit 0 |
| example drift | `python3 tools/docs/example_drift.py` | exit 0 |
| prose, whole tree | `python3 tools/prose/prose_gate.py --check prose --mode tree` | exit 0, 0 findings over 1036 sources |
| DBC doc and pair gate | `cmake --build build --target dbc-gate` | 348 interfaces, 0 gaps, both gates |
| time source | `bash test/time_source_gate.sh` | exit 0 |
| suite | `ctest --test-dir build --output-on-failure` | exit 0: 100 percent of 185 passed, 0 failed, 0 `Not Run`, 1 skipped |
| suite, two cpus | `taskset -c 0,1 ctest --test-dir build --output-on-failure` | exit 0: 100 percent of 185 passed, 0 failed, 0 `Not Run`, 1 skipped |

The one skipped entry is `harness_topdown_run_test`, the skip that unit already
carried at the base: it reserves its skip code for a host that grants the
top-down nodes, and `test/tier_labels.sh` holds its row. Both suite rows list no
`Not Run` entry, so both carry a verdict. The two-cpu row is the reading FR-072
asks for: the same 185 entries pass with the runner held to two processors.

## Commit messages read at the pre-squash head

`python3 tools/prose/prose_gate.py --check commit --mode range --base
origin/master` answered 29 findings across the lane's 18 pre-squash commits, and
the reading is kept here because the squash has to clear it. The two commits the
lane added last, `test: Plant the refusals the reader never saw` and `test: Keep
the probe scenario name in camelBack`, hold none of them:

- `CM.BODY-WRAP` on most commits: the bodies were wrapped at the keyboard, and
  the configured column is 72 (`tools/prose/prose_rules.yaml:12`). The gate reads
  the wrap; the constitution's 72-column rule is the reason a body gets wrapped
  even where a check stays quiet.
- `CM.SECTION-UNKNOWN` on the five commits titled `Specs:` and the one titled
  `Build:`. The configured sections are `CMake`, `CI`, `Constitution`,
  `Counters`, `Docs`, `Harness`, `Meta`, `dbc`, `deploy`, `runner`, `test`.
  Master's own convention puts spec artifacts under `Docs:` and the version
  ledger under `CMake:`, and the squashed commit follows both.
- `CM.TITLE-LENGTH` on one commit, `CM.FOOTER-APPROVAL` on one, and three prose
  findings inside bodies.

Ruling G-23 lands this lane as one commit, so T060 carries the whole reading: a
title of 50 characters or fewer with a configured section, a body wrapped at 72,
and the two trailer lines. The check runs again on the squashed head.

## The rebase, and one trailer the task text carried

`git fetch origin` left `origin/master` at `858c55d9`, the same sha this branch started
from, so T061 moved nothing and the tree of the single commit stands byte for byte where
the 19 pre-squash commits left it. The heavy rows were read at that tree and the cheap
gates were re-run on the squashed commit, which is the reading `specs/041-counter-probe`
names in its pull request body.

T060 wrote the trailer as `Refs: specs/041-counter-probe N6-P`. That trailing token names
no field of the template in the constitution (Pull Request Quality), and a token in a
trailer line reads as part of the reference, so the commit carries the canonical
`Refs: specs/041-counter-probe` and the approval footer as its last two lines. There is no
issue for this feature to `Fix`, so the pull request links the open coverage-gate issue
with `See #76` and the spec stands as the tracker.

## The rows retaken on the tree this branch carries

The disclosure fix and the wording pass sit in the same commit as the feature, so every
cheap row and every heavy row was read again on that content. Host `gary`, Linux
7.3.1-1-cachyos x86_64, c++ (GCC) 16.2.1 20260810, 2026-11-05, release developer-mode
tree from `cmake --preset=ci-rocky`. The earlier rows above keep their own heads, and
each names them.

| Gate | Command | Answer |
| --- | --- | --- |
| format | `cmake -P cmake/lint.cmake` | exit 0 |
| spell | `cmake -P cmake/spell.cmake` | exit 0 |
| example drift | `python3 tools/docs/example_drift.py` | exit 0 |
| prose, whole tree | `python3 tools/prose/prose_gate.py --check prose --mode tree` | exit 0, 0 findings over 1042 sources |
| DBC doc and pair gate | `cmake --build build --target dbc-gate` | 348 interfaces, 0 gaps, both gates |
| README length | `wc -l < README.md` | 1002 lines, against the limit of 1004 |
| exclusion markers | `grep -c LCOV_EXCL -r source include \| awk -F: '{s+=$NF} END {print s}'` | 84, the figure FR-074 fixes |
| suite | `ctest --test-dir build --output-on-failure` | exit 0: 100 percent of 185 passed, 0 failed, 0 `Not Run`, 1 skipped |
| suite, two cpus | `taskset -c 0,1 ctest --test-dir build --output-on-failure` | exit 0: 100 percent of 185 passed, 0 failed, 0 `Not Run`, 1 skipped |
| time source | `bash test/time_source_gate.sh` | exit 0, `time_source_gate: clean` |

The first attempt at the two-cpu row answered `18% tests passed, 152 tests failed out of
185` with 152 entries marked `Not Run`. That marker means the executable could not be
started. The heavy-rows script had finished its own clean stage, which empties the tree, and its
reconfigure plus a `dbc-gate` build left the tree configured while the test binaries
stayed absent. The row above is the re-take after a full `cmake --build build`,
and it lists no `Not Run` entry.

The skipped entry is the same `harness_topdown_run_test` skip the base carries, and
`test/tier_labels.sh` holds its row. The heavy rows read on this content are in
`results/sanitizers.md`, `results/coverage-head.txt`, and `results/analyzer.md`, and the
decode-silent reading is in `results/ci.md`.
