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

Result: pass. `plan.md` names `5b9ed47631742f4f6b6b0174371e5155a287cef7`
with run 37617561034 green. The name check emitted no per-file count at
that SHA, as the plan records.

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

Result: pass. Nine rename commits, `f7591cc` through `fb7ba67`. The
closing dev build exits 0, `ctest --preset=dev` exits 0, and the format
check exits 0 at the head.

## Step 3: string literal fixture

Before the first rename commit is accepted, one fixture holds a string
literal whose text matches an old spelling. After
`clang-apply-replacements`, that literal is unchanged.

Expected: the literal text matches the pre-rename text. A changed
literal fails D-01, and that commit stays open.

Result: pass. `test/source/counters_overhead.cpp` carries the literals
`"clock, syscall (vDSO)"` and `"pmu group, fast_rdpmc"`; the rename
moved the surrounding identifiers (`syscallRegime`, `fastRegime`) and
left both literals at their pre-rename text.

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

Result: pass. Sixty-seven owned object pairs between `5b9ed47` and the
rename head, both built with `speedgun-ng_CONTRACTS=ignore`. Every
normalized diff is empty. The record is in `plan.md`.

## Step 5: macro collision

At the rename head, preprocess a translation unit that includes every
public header and the Linux headers a library translation unit includes:

`clang++ -dM -E -std=c++23 <that-unit>`

Expected: no enumerator token and no `kPascalCase` constant token is a
defined macro. The risk tokens `NONE`, `GAP`, `SYSCALL`, `CPU`,
`THREAD`, `ABSENT`, `BYTES`, and `OPS` are in the checked set. The
checked set is every new enumerator and every new constant. Record the
result in this feature directory.

Result: pass. No enumerator and no `kPascalCase` token is a defined
macro. The record is in `plan.md`.

## Step 6: name check

At the rename head, run the name check with the closing-commit
configuration in `contracts/naming-check.md`.

Expected: zero findings.

Plant one misnamed identifier in `include/speedgun-ng/dbc.hpp`. Run
the name-check step again.

Expected: the plant fails the step. Remove the plant before the
closing commit is finished. The tree returns to zero findings
(SC-011).

Result: pass. Zero findings over the 47 owned translation units. The
planted `bad_name_plant` in `dbc.hpp` reported
`include/speedgun-ng/dbc.hpp:40:8: error: invalid case style for global
function 'bad_name_plant'` and exited 1; removal returned exit 0. The
second plant the contract Proof names landed in a `.cpp` under
`source/`: `bad_name_plant` appended to `source/counters/fold.cpp`
reported `source/counters/fold.cpp:394:5: error: invalid case style for
global function 'bad_name_plant'` and exited 1; removal returned exit 0
over the same TU. Both Proof plantings fail the step, so
`HeaderFilterRegex` reports `include/speedgun-ng/` and `source/`.

## Step 7: version and package request

Confirm `CMakeLists.txt:7` is `0.5.0`, `CMakeLists.txt:47` is
`SOVERSION 2`, and `cmake/install-rules.cmake:39` is `SameMinorVersion`.

Configure a consumer that requests version 0.4 against the installed
0.5 package.

Expected: the request rejects the package.

Result: pass. `CMakeLists.txt:7` is `0.5.0`, `CMakeLists.txt:47` is
`SOVERSION 2`, `install-rules.cmake:39` stays `SameMinorVersion`. A
consumer requesting 0.4 against the installed 0.5 package fails to
configure: "The version found is not compatible with the version
requested."

## Step 8: rename-map search

Run the search `contracts/rename-map.md` names.

Expected: every changed shipped-header spelling, including a detail
name, appears in the map. An old shipped-header spelling outside the
map and the exception list fails the search.

Result: pass. The map carries the changed spellings with kind and
detail marks. The token search over `include/`, `source/`, `test/`,
`example/`, `tools/`, `docs/`, `.github/workflows/`, `README.md`, and
`AGENTS.md` finds no old spelling of a renamed entity outside the map.
Hits on unchanged identifiers that share a spelling (the `m_columns`
member of `PointSink`, the `m_impl` members that keep the `m_` prefix,
kept members per FR-018) were reviewed and stand. The `plan` token in
`AGENTS.md` names the Spec Kit artifact, not the C++ type.

## Step 9: documents and the consumer job

`docs/pages/counters-overhead.md:299` no longer names `pmu_probe_fast`.

The downstream consumer job still matches `consumer: pmu catalog entries`
and `embedded:`.

Re-measure the overhead figures on `Linux 7.2.4-1-cachyos x86_64`, AMD
Ryzen 9 9950X3D. Compare them with the gate-baseline figures.

Expected: each gated median stays within five percent (research D-07).

Result: recorded with the host-state evidence in `plan.md`. The raw
medians move beyond five percent in the fast direction, and the bare
`rdtsc` pair moves with them by nineteen percent; the library-minus-bare
delta and the machine code are unchanged.

## Step 10: remaining gates

At the rename head, run the hard gates FR-005 names: the test set, the
contract gate, the format check, the spell check, the prose check, the
sanitizer presets, the thread-sanitizer job, the leak audits, the
downstream consumer job, and the line, branch, and contract coverage
gates. Build the release preset once:

`cmake --preset=ci-ubuntu`

`cmake --build build`

Expected: every gate exits 0.

Result: pass. CI run `37678329308` at convergence head `a734c6c`
concluded success across all eleven jobs: lint, prose-lint, test,
test-rocky, shared-audit, dbc-gate, sanitize, tsan, coverage,
downstream-consumer, and consumer-release. The coverage gate first
failed at head `69d6fcd` on one branch of the machine-root leg of
`Object::children`, whose gcov attribution moves with the host
topology: the same object code recorded one hit on that leg in one
run and zero in the next. The T066 branch-exclusion pair took the
leg out of the count, and the local coverage pass then read 867 of
867 branches at 100 percent. CI run `37760326237` at convergence
head `ef257e9` repeated the pass over the whole head: the three
commits `5e770f4`, `bf8327c` and `59378c4` postdated the recorded
run, and two of them edit C++. All eleven executed jobs concluded
success, the docs job skipped, and the coverage gate held with the
machine-root leg excluded. CI run `37762327363` at convergence head
`835481b` repeated the pass again: all eleven executed jobs
concluded success, the docs job skipped. The five commits after
that head sat unpushed, so no run covered them until T053 pushed
them; CI run `37768182864` at convergence head `eb1f444` then
repeated the pass: all eleven executed jobs concluded success, the
docs job skipped.

## Rename command

Each rename commit runs clang-tidy 23.1.1 with a temporary copy of the
new naming options, exports the fixes, and applies them with
clang-apply-replacements 23.1.1. clang-format 18.1.8 then reformats
only the files that commit touches. A name the fixer misses is edited
by hand in that same commit. A string literal is not an identifier and
stays unchanged.
