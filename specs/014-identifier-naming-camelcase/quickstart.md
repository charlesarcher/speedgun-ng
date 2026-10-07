# Quickstart: validating the identifier rename

This guide names the commands that prove the feature. It does not
contain the rename implementation. Research D-01 through D-08 hold the
decisions each step applies.

## Prerequisites

- Linux, GCC and Clang, CMake 3.20 or newer, C++23.
- `clang-tidy` 23.1.1 and `clang-apply-replacements` 23.1.1 on `/usr/bin`.
- `objdump` (GNU Binutils 2.47) and `llvm-cxxfilt` (LLVM 23.1.1).
- `clang-rename` is absent. The rename does not use it.
- The gate baseline in `contracts/gate-baseline.md` is recorded in
  `plan.md` before the first rename commit.

## Step 1: gate baseline

Confirm `plan.md` names a SHA that is not
`6d32efcab3c13c3d41470c1c621d78839ce11543` and is not
`6aafd2dc8a1310ee335820ad8dc4dedd0eb0ae17`.

Expected: the named SHA has a green Continuous Integration run, and the
plan records the passing test names, the gate results, and the
name-check finding count per translation unit. A missing SHA means the
predecessor in `contracts/gate-baseline.md` has not landed. Stop. Do
not open a rename commit.

## Step 2: one rename commit

From that SHA, apply one group from research D-02. Then:

`cmake --preset=dev`

`cmake --build --preset=dev`

`ctest --preset=dev`

Run the format check the repository uses for CI.

Expected: the build exits 0, the tests that passed at the gate baseline
pass, and the format check exits 0. A failure stops the sequence. Repeat
for each group. A whitespace change with no renamed identifier stays out
of the commit.

## Step 3: string literal fixture

Before the first rename commit is accepted, one fixture holds a string
literal whose text matches an old spelling. After
`clang-apply-replacements`, that literal is unchanged.

Expected: the literal text matches the pre-rename text. A changed
literal fails D-01, and that commit stays open.

## Step 4: machine-code comparison

At the gate baseline and at the rename head:

`cmake --preset=ci-ubuntu -Dspeedgun-ng_CONTRACTS=ignore`

`cmake --build build`

For each owned object file:

`objdump -d --no-show-raw-insn <owned-object>`

Replace each owned mangled symbol with a stable token from `nm` and
`llvm-cxxfilt`. Strip the address column. Leave instruction text and
immediates. An address column is not a difference.

Expected: `diff` of the two normalized texts is empty, apart from the
logged strings an always-on contract makes from a renamed predicate
(FR-002).

## Step 5: macro collision

At the rename head, preprocess a translation unit that includes every
public header and the Linux headers a library translation unit includes:

`clang++ -dM -E -std=c++23 <that-unit>`

Expected: no enumerator token and no `kPascalCase` constant token is a
defined macro. The risk tokens `NONE`, `GAP`, `SYSCALL`, `CPU`,
`THREAD`, `ABSENT`, `BYTES`, and `OPS` are in the checked set. The
checked set is every new enumerator and every new constant. Record the
result in this feature directory.

## Step 6: name check

At the rename head, run the name check with the closing-commit
configuration in `contracts/naming-check.md`.

Expected: zero findings.

Plant one misnamed identifier in `include/speedgun-ng/dbc.hpp`. Run
the name-check step again.

Expected: the plant fails the step. Remove the plant before the
closing commit is finished. The tree returns to zero findings
(SC-011).

## Step 7: version and package request

Confirm `CMakeLists.txt:7` is `0.5.0`, `CMakeLists.txt:47` is
`SOVERSION 2`, and `cmake/install-rules.cmake:39` is `SameMinorVersion`.

Configure a consumer that requests version 0.4 against the installed
0.5 package.

Expected: the request rejects the package.

## Step 8: rename-map search

Run the search `contracts/rename-map.md` names.

Expected: every changed shipped-header spelling, including a detail
name, appears in the map. An old shipped-header spelling outside the
map and the exception list fails the search.

## Step 9: documents and the consumer job

`docs/pages/counters-overhead.md:299` no longer names `pmu_probe_fast`.

The downstream consumer job still matches `consumer: pmu catalog entries`
and `embedded:`.

Re-measure the overhead figures on `Linux 7.2.4-1-cachyos x86_64`, AMD
Ryzen 9 9950X3D. Compare them with the gate-baseline figures.

Expected: each gated median stays within five percent (research D-07).

## Step 10: remaining gates

At the rename head, run the hard gates FR-005 names: the test set, the
contract gate, the format check, the spell check, the prose check, the
sanitizer presets, the thread-sanitizer job, the leak audits, the
downstream consumer job, and the line, branch, and contract coverage
gates. Build the release preset once:

`cmake --preset=ci-ubuntu`

`cmake --build build`

Expected: every gate exits 0.
