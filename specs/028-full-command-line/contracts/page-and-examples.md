# Contract: The Page, the Guide and the Example

**Surfaces:** `README.md`, `docs/pages/harness.md`,
`example/docs/command_line.sh`, `tools/docs/example_drift.py`,
`test/example-drift-fixture/run_example_drift_fixtures.py` (research
D-20, D-21, D-22, D-23). **Covers:** FR-066 to FR-075, FR-079; SC-011.

## The main page

The section `The command line` (`README.md:357` to `:370`) keeps its
title, its marked snippet block and its owner, and grows from 14 lines to
20 against the cap of 25 (`SECTION_LINE_CAP` at
`tools/docs/example_drift.py:156`); no entry joins `SECTION_LINE_CAPS` at
`:161` (FR-066, FR-073). The six added lines carry the three sources, the
precedence order, the disclosure line and the two run options, in the
page's sentence style.

The quick-reference option table (`README.md:745` to `:766`) gains one row
per new long option, in the existing two-column form: `--options`,
`--env`, `--threads`, `--interleave`, `--compare-rules`, `--help`,
`--version` (FR-068). The page grows by 13 lines, so `PAGE_LINE_CAP`
(`tools/docs/example_drift.py:155`) moves from the measured 773 to the
measured 786, and the comment above it records the arithmetic the way the
note at `:146` to `:154` does (FR-071). `SECTIONS` at `:32` to `:58` and
the section map entry at `:75` gain nothing: the title already
stands at `:44` with `command_line.cpp` recorded beside it (FR-073).

## The guide reference

`docs/pages/harness.md` gains the full option reference inside its
section `The command line of an example` (the heading at `:603`): one row
per table row, in table order, with the name, the argument kind, the
environment variable where the row carries one, and the help text, plus
the precedence sentence spec D-1 states (FR-069, FR-074).

The comparison against the table runs both ways in
`harness_cli_table_test`: every row of the page names a row of the table
with the same argument kind, environment name and help text, and every
row of the table appears on the page (FR-070). The test reads the page
from a source directory passed as an argument, the way
`harness_compare_test` receives `test/data` at `test/CMakeLists.txt:638`
to `:639`.

## The example

The region `options` of `example/docs/command_line.sh` (`:14` to `:27`)
gains the command lines a reader types for the option file, the
environment, the thread list, the interleaving and the rules file, in the
file's existing form of one typed line per case (FR-072). The fenced
block at `docs/pages/harness.md:611` to `:626` grows to match the region
byte for byte, which is what the drift tool compares.

The snippet region `command-line-read` of `example/docs/command_line.cpp`
(`:4` to `:14`) stays as it is: FR-067 asks that the example build and
run in CI, which `example_run` (`test/CMakeLists.txt:1216` to `:1220`)
already drives, and the snippet-compile check at `test/CMakeLists.txt:1140`
to `:1144` compiles C++ blocks only, so a shell block growth leaves it
clean.

## The drift tool follows the table

`cli_options()` (`tools/docs/example_drift.py:458` to `:466`) reads
`source/harness/options.cpp` in place of `source/harness/cli.cpp`, and its
table regex matches the new declaration while the row regex
`\{\s*"([a-z0-9-]+)"` stays. The liveness fixture writes its stub table
to the new path in the new shape
(`test/example-drift-fixture/run_example_drift_fixtures.py:117` to
`:124`), so the planted missing-row case at `:270` to `:280` keeps its
expectation of the finding `has no option row` (FR-075).

The scraper move and the table move stand in one task: a table that moves
while the scraper stays turns the gate red on a tree whose table and
page agree, and a scraper that reads a file with no table raises `no
long-option table found` in place of checking nothing (research D-20).

## Oracle

`harness_cli_table_test` covers the guide reference in both directions
(FR-069, FR-070, FR-074). The drift gate covers the README option rows in
both directions, the section line count against the cap, the page length
against `PAGE_LINE_CAP`, and the block-to-region equality of the shell
region (FR-066, FR-068, FR-071, FR-072, FR-073). The drift fixture
harness covers the liveness of the option-row check (FR-075).
`example_run` and `snippet_compile` cover FR-067.
