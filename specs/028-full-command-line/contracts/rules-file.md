# Contract: The Rules File

**Surfaces:** `--compare-rules=<file>`, `detail::readRulesFile`, the
`rules` field of `ResolvedOptions` (research D-05, D-13; data model
E-12). **Covers:** FR-061 to FR-065; SC-006.

## The read

`--compare-rules=<path>` reads the text through `detail::readTextFile`
and calls `sg::parseCompareRule`
(`include/speedgun-ng/benchmark.hpp:2589`) once per line. The grammar is
the specs/026 grammar, unchanged: this feature adds no rule form, no
field and no threshold spelling (FR-061).

- A line whose first non-space character is `#` is a comment (FR-062).
- A blank line yields no rule (FR-062).
- Any other line is a rule, in line order (FR-061).
- A rules file names no file of its own: a line written
  `compare-rules=other.txt` reaches the rule parser and is refused there
  under FR-064 (spec D-23, FR-061).

Line numbers count from 1 over the raw lines, as in the option file.

## The order

Rules combine; no source replaces another (FR-063). The order is spec
D-23:

| Position | Source |
| --- | --- |
| 1 | the `--compare-rule` options of the command line, in the order typed |
| 2 | the file `--compare-rules` names, in line order |
| 3 | the `compare-rule` lines of the `--options` file, in line order |
| 4 | the file that a `compare-rules` line of that option file names, in line order |

The list reaches `detail::runCompareMode` in that order, so the printed
rows and the verdict follow it.

## The faults

Each answers one line on standard error at status 2.

| Case | Line |
| --- | --- |
| the file cannot be read | `--compare-rules: the file '<path>' cannot be opened: <reason>` |
| a line the rule parser refuses | `<path>:<line>: ` then the parser's own fault text |

The parser's text already names the field and the rule text, as specs/026
wrote it, so the file line adds the path and the number to it (FR-064).
The unreadable line reuses the moved reader's text, the same shape the
option file uses (FR-065, research D-05).

## Oracle

`harness_compare_cli_test` gains the rules-file cases: a file of three
rules over the fit fixture pair, a comment and a blank line that hold
line numbers, the refusal line for a rule the parser rejects with the
path and the number, the unreadable-path line, and the four-position
order of spec D-23 checked through the printed row order of a compare run
that sets rules in all four places. The existing specs/026 rule-grammar
cases stay as they are: the grammar does not move.
