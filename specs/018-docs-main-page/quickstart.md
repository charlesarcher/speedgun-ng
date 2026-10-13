# Quickstart: Main Page Documentation Rewrite

Phase 1 output. Runnable validation for `specs/018-docs-main-page/`.
The requirements live in [spec.md](spec.md); the design, section map, and
test plan live in [plan.md](plan.md); the entity rules live in
[data-model.md](data-model.md); the gate behavior lives in
[contracts/](contracts/).

## Prerequisites

Linux on a C++23 compiler with CMake 3.20 or newer. The committed presets
drive everything: `ci-ubuntu` into `build`, and the machine-local `dev`
preset is never assumed. `ci-ubuntu` inherits `dev-mode`, so
`BUILD_EXAMPLES` is ON in the CI trees and example binaries land under
`build/example/`. Python 3 is present in every job that runs a gate.

Run the suite from the repository root:

```sh
cmake --preset=ci-ubuntu
cmake --build build
ctest --test-dir build --output-on-failure
```

## Scenario 1: the page alone builds and runs a benchmark (US1, SC-002)

On a clean Linux checkout, follow the quick start of `README.md`
verbatim: its configure, build, and install commands; then create a
directory outside the tree, paste the page's `find_package` block and the
page's first benchmark program unchanged, and build it with the page's
commands; then run it.

Pass: the install step produces a package the page's `find_package` finds,
the link succeeds with the page's two-target line, and the run exits 0
with a report row on stdout. The comparison with the page's sample block
matches column for column because the sample is masked by shape (US1
case 4). Any deviation fails the scenario.

## Scenario 2: first screen teaches the library (US2, SC-001)

Extract the first screen of `README.md`: every line before the second
level-2 heading. First walk the six-fact precondition: each FR-002 fact
(contract C-A rule 9) must appear as text on that screen: the language
standard, Linux as the supported platform, the Google Benchmark
lineage, the counters library as the source of every time and counter
value, hardware counter metrics that attach to a benchmark, and contract
checks that guard the API. Then hand that text, alone, to a fresh model
with no other repository access and ask: what is this, where does it run,
where do the numbers come from. Record the answers. Repeat the same
three questions with three readers new to the repository, once before
merge.

Pass: all six facts present, and every answer names a C++23 benchmark
framework for Linux in the Google Benchmark style, Linux, and the
counters library of this project.
Both devices are review checks at the pull request; neither is a CI gate
(clarify session 2026-10-13).

## Scenario 3: drift gate green and planted drift caught (US3, US5, SC-004)

```sh
ctest --test-dir build -R example_drift --output-on-failure
ctest --test-dir build -R "example_drift_fixtures|report_shape_fixtures" --output-on-failure
```

Pass: both exit 0 and the fixture logs list every case of
`contracts/example-drift.md` R-8 and `contracts/report-shape.md` S-5
with their expected verdicts. For a manual
reproduction, edit one character inside one marked main-page block inside
a scratch worktree copy, run the tool directly, and observe exit 1 naming
the file, the region, and the line; restore the copy. Never plant drift in
the shared tree.

## Scenario 4: snippets compile (US3, SC-003)

```sh
cmake --build build --target first_benchmark benchmark_example docs_snippets
ctest --test-dir build -R "report_shape|snippet_compile" --output-on-failure
cmake -S . -B /tmp/off018 -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=OFF
ctest --test-dir /tmp/off018 -R "^example_drift" --output-on-failure
! ctest --test-dir /tmp/off018 -N | grep -qw report_shape
grep -qiE "one host's run" README.md
```

Pass: all example targets build in the release CI configuration (every
`cpp` page snippet lives in a region inside a compiled example file);
`snippet_compile` runs unconditionally (it needs page text, headers, and
a compiler; a built example is unnecessary): it first proves it can fail by
syntax-rejecting its planted broken probe, then syntax-compiles every
emitted `cpp` block standalone over the canonical includes (a fragment
region fails there; shell and cmake blocks are skipped by rule; the
emitted count equals the page's marked `cpp`-fence count, so an empty
run on
the shipped page fails); the sample output block matches the run by
shape: a digit difference passes, a renamed or missing column fails,
and the block's prose carries the one-host label sentence pinned by
`report-shape.md` S-1 rule 2 (the `grep -qiE` above is its executable
check, FR-007); and
in a `BUILD_EXAMPLES=OFF` configure the drift gate and its fixtures pass
from source text alone (FR-035) while the `report_shape` test is absent
by its guard (the `grep -qw` assertion fails if the guard ever leaks).

## Scenario 5: prose and forbidden-token scans (SC-005, SC-010)

```sh
cmake -P cmake/prose-lint.cmake
python3 tools/docs/example_drift.py .
```

Pass: the prose gate exits 0 for the pull-request range, covering
`README.md` and `docs/pages/building.md`. The drift tool's default run
includes the section, name, option, and forbidden-token scans of
`contracts/main-page-structure.md` C-B and `contracts/example-drift.md`
R-7: all twelve FR-009 sections present, each with a marked block;
every quick-reference name present in the headers; the option table
equal to the long-option table of `speedgunMain`; and zero forbidden
tokens in prose (fence bodies and inline code exempt).

## Scenario 6: the maintainer facts survived the move (US4, SC-006)

Open the section map in [plan.md](plan.md). For each of its nine rows,
grep for every command, tool name, and tripwire description that the
row enumerates (the old text is visible at `git show 8655201:README.md`,
the audit commit; `HEAD` will hold the landing commits) in
`docs/pages/building.md`.

Pass: every probed fact appears on the build page, and
`README.md` carries one link to it near the end (US4 case 2).

## Scenario 7: documentation home reads by audience (US4, SC-007)

```sh
cmake "-DPROJECT_SOURCE_DIR=$PWD" "-DPROJECT_BINARY_DIR=$PWD/build" \
      -P cmake/docs-ci.cmake
```

Pass: the build completes, the index lists the user pages (about,
building, harness) before the internals group, and the Doxygen home page
renders the rewritten `README.md` with no visible fence-info tokens
(`docs/pages` input plus `USE_MDFILE_AS_MAINPAGE`).

## Scenario 8: the rule landed for later features (US5, SC-008)

```sh
grep -n "IV.1" .specify/memory/constitution.md
grep -c "2.19.0" .specify/memory/constitution.md
grep -nE "^\| *\[?2\.19\.0\]? *\|" .specify/memory/constitution.md
grep -nE "\*\*Version\*\*: 2\.19\.0.*\*\*Last Amended\*\*: [0-9]{4}-[0-9]{2}-[0-9]{2}" \
     .specify/memory/constitution.md
ctx=$(grep -iA8 "example compile gate" .specify/memory/constitution.md)
for n in example_drift example_drift_fixtures snippet_compile report_shape \
         report_shape_fixtures; do
  printf '%s\n' "$ctx" | grep -q "$n" || { echo "missing $n"; exit 1; }
done
printf '%s\n' "$ctx" | grep -qi "lint" || { echo "missing lint step"; exit 1; }
[ "$(grep -l "Main-page impact check (IV.1)" \
        .specify/templates/spec-template.md \
        .specify/templates/tasks-template.md | wc -l)" -eq 2 ]
```

Pass: every command exits 0. Principle IV carries sub-rule IV.1 "Main-
page documentation per feature"; the loop proves the Principle VIII gate
entry enumerates all six component names inside its context window (on
pre-amendment text the context is empty and the first loop iteration
fails, so the check discriminates); the lineage grep and the footer grep
each hit once on 2.19.0, the footer line carrying an ISO date in its
Last Amended field (the exact date is set by the landing day, so the
assertion is on the footer form); the final assertion requires the
phrase "Main-page impact check (IV.1)", pinned verbatim by research
D-08, in both templates; and the Sync Impact Report position above the
body is the one manual review read.

## Scenario 9: whole-suite and release-preset parity (Principle IX)

```sh
ctest --test-dir build --output-on-failure          # every gate green
cmake --preset=ci-ubuntu && cmake --build build     # release build, once per feature
```

Pass: the full suite exits 0 with `--no-tests=error` satisfied, and the
release-configuration build compiles. A task closed only against an
unoptimized build stays open (Principle IX).

## Scenario 10: budget caps (SC-011)

```sh
[ "$(wc -l < README.md)" -le 400 ] || { echo "over 400 total"; exit 1; }
awk '/^## How you do it/{f=1;next} /^## Quick reference/{f=0} f' README.md \
  | awk '/^### /{if(s&&NR-s>25){print t; bad=1} s=NR; t=$0}
         END{if(s&&NR-s+1>25){print t; bad=1} exit bad}'
```

Pass: both commands exit 0. The caps are binary (D-14): the page is 400
lines or fewer including the quick reference, and no feature section of
"How you do it" exceeds 25 lines counting from its H3 heading through
the line before the next heading. Either cap failing is an acceptance
failure of the page-rewrite task, fixed by tightening prose, never by
moving content back to the old shape.
