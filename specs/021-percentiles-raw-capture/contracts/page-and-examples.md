# Contract: Main Page, Example, and Budget

## The main-page section

A section titled `Percentiles` in the "How you do it" run of `README.md`,
after "Reading the report" and before "User counters", at most 25 lines: one
purpose sentence, one marked snippet, one result sentence (Principle IV.1).

The snippet region lives in `example/docs/percentiles.cpp` between
`// sg-example:begin percentiles` and `// sg-example:end percentiles`. The
result sentence names the rows the run prints, and
`example/docs/percentiles.expected` holds them.

## The example

`example/docs/percentiles.cpp` runs one family whose body costs microseconds,
so the 1% rule of D-03 leaves at least 100 chunk values inside a run of a few
hundred milliseconds and the section can name p50, p90 and p99 (D-16). The
family turns the output on through `showPercentiles(true)`, so the snippet
shows the handle call, and the result sentence names the option a reader can
use instead.

Registration: `add_docs_example(percentiles)` in `example/CMakeLists.txt`,
which builds `docs_percentiles`, gives `run_docs_percentiles`, and hangs it on
`run-examples`.

## Quick-reference rows

Two rows in the Quick reference table: the `showPercentiles` call and the
`--percentiles=` / `--no-percentiles` pair.

## The detail page

`docs/pages/harness.md` gains a `Percentiles` section carrying the chunk rule,
the rank rule with its minimum sample counts, the merged-threaded rule, and
the not-available rule. The main-page section links to it.

## The gate list

`SECTIONS` in `tools/docs/example_drift.py` gains `Percentiles`, so the gate
checks the section, its region, and its expected file.

## The line budget

`README.md` stands at 638 lines of a 638 cap at `ab32400`. The section, its
rows, and the "Reading the report" sentence that the endpoint-sample change
moves cannot fit. `PAGE_LINE_CAP` moves by the measured amount when the
section lands, the smallest fitting amount, recorded in the spec's budget note
and in the gate's own comment, the way 020 recorded its 570 to 638 move.
`SECTION_LINE_CAP` stays 25, prose is trimmed first, no snippet is dropped,
and no earlier spec is edited. If a rebase brings N2 or N4 in first, the
measurement is taken against the page as the rebase leaves it.
