# Analyzer reading of the rebase: head `fba814d2` against base `02892b57`

The analyzer wave of the rebase wrote this record on 2026-11-06 on the lane
host, `kennykiller`. It answers one question: does the rebased lane add an
analyzer finding to any file it changes? Two build logs carry every figure
below, and each one names the tree that produced it. The host ran three other
waves while both builds ran, so every wall time here is a shared-host figure
and not an idle-host one.

## What the bar says

The bar reads: this lane adds no analyzer finding to the three files it
changes. The three files are `source/counters/system.cpp`,
`source/counters/detail/core.hpp` and
`include/speedgun-ng/counters_system.hpp`.

```bash
git diff --name-only 02892b57..HEAD
```

That command names all three files above. It names one further file under
`source/` and `include/`, namely
`source/counters/detail/limit_source_record.hpp`. That file stands outside the
three files the plan names, and this record measured it anyway: its head log and
its base log each hold zero `file:line:column:check` keys for it, because it
declares two functions and a seam and carries no statement to analyze.

A finding counts as added when the head log holds a `file:line:column:check`
key for one of the three files and the base log for the same file does not
hold that key. Counting raw lines cannot answer the bar, because a header
finding repeats in every translation unit that reaches that header. The rebase
moved the base from `858c55d9` to `02892b57`, so the old pair in
`specs/050-limit-table-lifecycle/results/base-state.md` cannot settle the bar.
This record measures both sides again, one full-tree log per side.

## How this record counted

Both trees carry the `ci-ubuntu` cache, a fact this wave read from the
`CMakeCache.txt` of each tree: a Release developer build with clang-tidy and
cppcheck wired in, and with ccache in front of the compiler only. Each compile
rule runs `cmake -E __run_co_compile`, which takes the compiler behind a
`--tidy=` launcher and a `--cppcheck=` launcher, so a rebuild re-runs both
analyzers and a full-tree build is also a full-tree analyzer log. Three tools
answered on this host today, and each version below came from a probe run for
this record:

| Tool | Reading |
| --- | --- |
| `g++ --version` | `g++ (GCC) 16.2.1 20260810` |
| `clang-tidy --version` | `LLVM version 23.1.4` |
| `cppcheck --version` | `Cppcheck 2.22.0` |

A finding counts once per file, line, column and check name, and its message
text plays no part. Two units that answer those four values alike therefore
carry one finding between them. That key is the key of the old base record, at
`specs/050-limit-table-lifecycle/results/base-state.md`, so the two records
count alike.

Both builds ran `make` with `--output-sync=target`, so the output of one unit
stays in one block and the parser can name the unit that reported each line.
The unit column of the two figure tables counts the units whose block reports
that file, and the unit count of a log counts its `Building CXX object` lines.
The two tools spell paths differently: clang-tidy spells a path from the tree
root, and cppcheck sometimes spells one with a `..` segment. The parser
resolves each spelling against the tree it read, keeps the paths under
`source/`, `include/`, `test/`, `example/`, `tools/`, `external/` and
`harness/`, and drops the rest. `/tmp/rba-parse.py` holds the parser, and
`/tmp/rba-head.json` and `/tmp/rba-base.json` hold its output for the two
logs.

Every position of the three files came from clang-tidy in both logs, and
cppcheck reported no position in any of the three. The key still spans both
tools, as the old record's key did.

## The head figures

One command produced the head log in `build/dev`, the tree this wave owns. It
ran at `fba814d2` after `cmake --build build/dev --target clean`, it exited 0,
and its output went to `/tmp/rba-head-build.log`:

```bash
cmake --build build/dev -j 8 -- --output-sync=target
```

The run took 206 seconds of wall time on a host that three other waves were
using, so that figure is not an idle-host figure. The log holds 52822
`: warning:` lines and 319 `Building CXX object` lines. One of those 319
builds a precompiled header for a vendored library and reports no finding, so
the log covers 318 C++ translation units. Five warning lines name C files of
the vendored `hwloc` sources under `build/dev/`, so the parser keeps 52817
lines across 279 distinct path strings.

| Edited file | Raw lines | Distinct positions | Units that report it |
| --- | --- | --- | --- |
| `source/counters/system.cpp` | 22 | 22 | 1 |
| `source/counters/detail/core.hpp` | 95 | 19 | 5 |
| `include/speedgun-ng/counters_system.hpp` | 625 | 5 | 125 |

A header finding repeats in every unit that reaches the header, so the raw
column divides by the unit column to the distinct column in each row. The raw
column is not the count the bar asks about.

**`source/counters/system.cpp`**, over its 22 distinct positions:

| Positions | Check name |
| --- | --- |
| 10 | `readability-trailing-comma` |
| 5 | `modernize-use-trailing-return-type` |
| 2 | `misc-include-cleaner` |
| 2 | `performance-inefficient-string-concatenation` |
| 1 | `abseil-string-find-str-contains` |
| 1 | `performance-inefficient-vector-operation` |
| 1 | `readability-container-contains` |

**`source/counters/detail/core.hpp`**, over its 19 distinct positions:

| Positions | Check name |
| --- | --- |
| 11 | `cppcoreguidelines-pro-bounds-avoid-unchecked-container-access` |
| 3 | `readability-identifier-length` |
| 1 | `bugprone-easily-swappable-parameters` |
| 1 | `cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays` |
| 1 | `portability-avoid-pragma-once` |
| 1 | `readability-trailing-comma` |
| 1 | `readability-use-anyofallof` |

**`include/speedgun-ng/counters_system.hpp`**, over its 5 distinct
positions:

| Positions | Check name |
| --- | --- |
| 2 | `cppcoreguidelines-special-member-functions` |
| 2 | `readability-identifier-length` |
| 1 | `readability-trailing-comma` |

One position in `source/counters/detail/core.hpp` carries two check names at
once, so the analyzer reports them as one comma-separated pair. The multiset
keeps that pair as one entry, because the counting key reads the whole
bracket.

## The base figures and their source

The base figures come from a build this wave ran itself, in a tree of its own,
against the new base. The new-base wave owns the detached source tree
`/tmp/050-base-master`, checked out at `02892b57`; this wave read that tree
and wrote nothing into it. These two commands configured and built
`build/rbase-analyzer`, the build tree this wave created for the purpose:

```bash
cmake -S /tmp/050-base-master -B build/rbase-analyzer --preset=ci-ubuntu \
      -DPython3_EXECUTABLE=/usr/bin/python3
cmake --build build/rbase-analyzer --target clean
cmake --build build/rbase-analyzer -j 6 -- --output-sync=target
```

The build wrote `/tmp/rba-base-build.log`, exited 0, and took 293 seconds of
wall time, again on a host three other waves were using. An earlier attempt at
the same build stopped after 149 seconds with exit code 2. That log reached
173 `Building CXX object` lines and 11843 `: warning:` lines, broke off inside
a block, and held no `make` error line naming a target; its single `: error:`
line names `test/source/dbc_test.cpp:232`, the same analyzer line the finished
log holds. The wave re-ran that one entry, the clean build above, and the
re-run reached exit 0. Every base figure below comes from the re-run log,
never from the stopped one.

That log holds 52733 `: warning:` lines, 318 `Building CXX object` lines of
which one is a precompiled header, and two `: error:` lines at the same two
test files and columns the head log names. It covers 317 C++ translation
units. The parser keeps 52728 of the warning lines across 278 distinct path
strings, and it drops the same five vendored `hwloc` C lines the head log
yields.

| Edited file | Raw lines | Distinct positions | Units that report it |
| --- | --- | --- | --- |
| `source/counters/system.cpp` | 22 | 22 | 1 |
| `source/counters/detail/core.hpp` | 95 | 19 | 5 |
| `include/speedgun-ng/counters_system.hpp` | 620 | 5 | 124 |

The check-name multiset of the base matches the head for all three files,
entry for entry.

Two other readings of the base exist, and neither is this one. The figures of
`specs/050-limit-table-lifecycle/results/base-state.md` belong to the old base
`858c55d9`, where the three files showed 22, 19 and 5 distinct positions from
logs of 1, 4 and 110 units. This wave measured `02892b57` instead, and its
unit counts read 1, 5 and 124. A further difference sits in the log itself:
the head log covers 318 units and the base log 317, because the lane adds
`test/source/counters_limit_lifecycle_test.cpp` with `specs/050` work, and
that file adds one unit and one target to the build.

One figure here answers a prediction the orchestrator sent. `specs/041` added
`objectLimitSource` and the `LimitSource` enumeration above the `System`
class, and `include/speedgun-ng/counters_system.hpp` holds them at the base:
line 199 holds `enum class LimitSource : std::uint8_t`, line 229 holds the
`objectLimitSource` declaration, and line 240 opens the `System` class. That
API draws no finding of its own, so the base figure for the header measures 5
positions and not more.

## The head-versus-base pair

The bar reads: this lane adds no analyzer finding to the three files it
changes. The pair below answers that bar, and the bar holds for all three
files.

| File | Head | Base | Alike | Shifted | New |
| --- | --- | --- | --- | --- | --- |
| `source/counters/system.cpp` | 22 | 22 | 7 | 15 | 0 |
| `source/counters/detail/core.hpp` | 19 | 19 | 1 | 18 | 0 |
| `include/speedgun-ng/counters_system.hpp` | 5 | 5 | 1 | 4 | 0 |

Every key the head holds while the base does not names a column and a check
name that the base holds at some other line. Fifteen such keys sit in
`source/counters/system.cpp`, 18 in `source/counters/detail/core.hpp`, and 4
in `include/speedgun-ng/counters_system.hpp`. Each one pairs with a base key
of the same column and the same check name, and within each column and check
group both sides hold the same number of keys. No head key stands alone, and
no base key lost its head twin.

The line a key names moves because the lane moves source lines. For each pair,
one unchanged run of the two file versions carries both lines: the base line
and its head twin hold the same text, and the diff leaves the run around them
alone. The offsets match the hunks that `git diff -U0 02892b57..HEAD` prints.
The keys that need no pairing sit at head lines 48, 51, 55, 57, 67, 96 and 131
of `source/counters/system.cpp`, at line 1 of
`source/counters/detail/core.hpp`, and at line 36 of
`include/speedgun-ng/counters_system.hpp`.

| File | Head lines | Base twin lines |
| --- | --- | --- |
| `source/counters/system.cpp` | 206 to 276 | 219 to 289 |
| `source/counters/system.cpp` | 312 to 589 | 324 to 601 |
| `source/counters/detail/core.hpp` | 137 | 136 |
| `source/counters/detail/core.hpp` | 250 to 332 | 242 to 324 |
| `include/speedgun-ng/counters_system.hpp` | 175 and 185 | 169 and 179 |
| `include/speedgun-ng/counters_system.hpp` | 253 | 240 |
| `include/speedgun-ng/counters_system.hpp` | 375 | 344 |

The hunks account for each offset. In `source/counters/system.cpp` the lane
deletes 14 lines at base line 179 and adds one at head line 188, so a base
line after that point sits 13 lines above its twin, and the one line added at
head line 281 lifts the rest to 12. In `source/counters/detail/core.hpp` the
lane adds one line at head line 49 and seven at head line 208, which gives 1
and then 8. In `include/speedgun-ng/counters_system.hpp` it adds six lines at
head line 83, seven at head line 245 and 21 at head line 259, then deletes
three at base line 335, which gives 6, then 13, then 31 below head line 368.

The check-name multiset matches entry for entry at the two sides, for each of
the three files, so the head adds no check name and drops none. The unit
counts move without moving any distinct count: the header reports through 125
units at the head and 124 at the base, and the two logs cover 318 units and
317 units.

## What no reading here claims

This record answers the bar, and four limits follow from that.

The lane changes a fourth code file,
`source/counters/detail/limit_source_record.hpp`, which takes 6 added lines
and 4 deleted lines in the diff. This record names no figure for it, because
the brief of this wave names three files.

The two logs hold findings for many other files, among them the files under
`test/`, `example/` and `tools/`. This record reads none of those counts, and
the two tree totals, 52817 kept lines at the head and 52728 at the base, bound
nothing. Each log also holds two analyzer `error:` lines at the same places on
both sides: `test/source/dbc_test.cpp:232` and
`test/source/harness_topdown_run_test.cpp:3483`. Both files read alike at the
two sides, neither names one of the three files, and the counting key of this
record counts warning lines, as the old record's key did.

The 46 standing positions earn no verdict here. The lane fixes none of them at
either side, and the bar speaks about additions alone. A reader who wants
those findings gone asks a different question.

Three readings sit outside this record on purpose. Compiler diagnostics form a
separate reading. Both builds exit 0 under a `-Werror` flag set, so a compiler
finding would have stopped a build before any analyzer count existed. The two
pinned gate tools, the prose gate, the coverage trace, the sanitizer runs and
the CTest suite have records of their own in this lane, among them
`acceptance.md`, `coverage.md` and `sanitizers.md` beside this file. A
single-translation-unit log answers a narrower question than this one: it
reports one unit for a header, and it can miss a position that only some units
draw. Both logs here are full-tree logs.

One figure carries a host caveat. Three other waves ran on this host while
both builds ran, so the 206 seconds of the head build and the 293 seconds of
the base build are shared-host wall times.

## Verification of this record

This wave ran three checks on the file it wrote. The gate counted one source,
so its verdict covers this file.

```bash
test -s specs/050-limit-table-lifecycle/results/rebase-analyzer.md
python3 tools/prose/prose_gate.py --check prose --mode tree \
  --paths specs/050-limit-table-lifecycle/results/rebase-analyzer.md
/tmp/050-tools-venv/bin/codespell \
  specs/050-limit-table-lifecycle/results/rebase-analyzer.md
```

| Check | Verdict |
| --- | --- |
| `test -s` on this file | exits 0, so the file holds text |
| prose gate | exits 0, and the block below prints its line |
| `codespell` 2.4.3, the pinned tool | exits 0 and names no misspelling |

```text
prose-lint: 1 sources, 268 units examined, 0 findings, 0 skipped
```

The three counts the bar asks about, restated:

| File | Head | Base |
| --- | --- | --- |
| `source/counters/system.cpp` | 22 | 22 |
| `source/counters/detail/core.hpp` | 19 | 19 |
| `include/speedgun-ng/counters_system.hpp` | 5 | 5 |

The bar holds at `02892b57`, the base this lane now rebases onto. Two logs,
one parser and two JSON files carry every figure here, under `/tmp` with the
`rba-` prefix: `/tmp/rba-head-build.log`, `/tmp/rba-base-build.log`,
`/tmp/rba-parse.py`, `/tmp/rba-head.json` and `/tmp/rba-base.json`. That
directory does not survive a reboot of this host. The wave wrote no file
inside the repository outside this record. Its scratch files share that prefix
under `/tmp`, and the two build trees it owns are `build/dev` and
`build/rbase-analyzer`.

