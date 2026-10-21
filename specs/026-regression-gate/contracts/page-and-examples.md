# Contract: The Main Page, the Example and the Budget

## The main-page section

`README.md` gains one section, `### Regression gate`, in the order the
page teaches: after `### JSON reports` at `README.md:579`. It carries
one purpose sentence, one snippet block copied from the marked region
of `example/docs/regression_gate.cpp`, and one result sentence naming
the status answer and the state words (FR-059, IV.1). The section title
joins `SECTIONS` in `tools/docs/example_drift.py:30` to `:54` and
`SECTION_OWNERS` at `:59` with `regression_gate.cpp` as its owner.

## The example

`example/docs/regression_gate.cpp` is one complete program (FR-060):
one small registered family, and a `main` that builds two `ReportData`
values in code, calls `parseCompareRule` and `compareReports`, prints
one line per entry with its state word, then calls `speedgunMain`
(D-16). `example/CMakeLists.txt` gains `add_docs_example(regression_gate)`
beside the list at `:50` to `:72`, which wires the `run_docs_` runner
into `run-examples`. `test/example_run.sh` runs the binary, and its
`--list` check at the gate's own lines requires the registered family.

`example/docs/regression_gate.expected` records the printed comparison
lines in a marked region; the README's result sentence quotes them, so
the recorded block equals what the example prints (FR-060). The lines
are deterministic because the two values are built in code.

## The quick reference

The quick-reference table gains one row per new public name the page
teaches (FR-061): `compareReports`, `parseCompareRule`, `CompareRule`
and `CompareState`, each with owner, one-line job and the anchor to the
new section. The option table gains `--compare`, `--compare-rule` and
`--compare-strict` rows, the drift gate checking them against the
`longOptions` table of `speedgunMain`
(`source/harness/cli.cpp:301` to `:318`).

## The guide page

`docs/pages/harness.md` gains one `## Regression gate` section after
`## JSON reports` (`docs/pages/harness.md:1018`): the rule grammar in
full, the seven state words with their meanings, the fault lines, the
comparison document and its schema path, and the shell blocks that
drive the mode (FR-062). A shell block belongs to the guide page that
quotes it, the ownership rule of `SECTION_OWNERS`.

## The line budget

`PAGE_LINE_CAP` in `tools/docs/example_drift.py:132` stands at 713,
the measured length of the page at the base. The new section, its
quick-reference rows and its option rows grow the page; the change
raises the cap to the measured length as the feature lands, the form
023 recorded. `SECTION_LINE_CAP` at `:133` stays 25 and the new section
is written to fit it; no entry joins `SECTION_LINE_CAPS` (`:138`).

## What the page must not claim

- No performance number for the comparison: the gate runs after the
  run and reports no timing claim.
- No claim that a `NOISY` row failed: the word and the verdict are
  separate while strict stands off (FR-044).
- No claim that the gate reads a live run: the two documents are its
  only input (FR-006).
