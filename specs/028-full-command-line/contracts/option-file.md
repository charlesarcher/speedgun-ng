# Contract: The Option File

**Surfaces:** `--options=<file>`, `detail::OptionFileText`, the option
file argument of `detail::gatherOptionSources`, `detail::readTextFile`
(research D-05, D-11, D-12; data model E-03). **Covers:** FR-029 to
FR-036; SC-002, SC-013.

## The read

`cli.cpp` reads the file after the scan, through `detail::readTextFile`
(the reader moving out of the anonymous namespace of
`source/harness/json_load.cpp:26` to `:48`, research D-05), and hands
the path and the whole text to the gather. The gather splits the text
into lines and never opens a file (FR-012).

## The line grammar

A line sets one option in long spelling without the leading `--`, as
`name` or `name=value` (FR-029).

- A line whose first non-space character is `#` is a comment (FR-030).
- A blank line, spaces and tab included, sets nothing (FR-030).
- A line whose first non-space character is `-` is a fault (FR-036).
- A line naming `options`, `env`, `help` or `version` is refused by name
  (FR-031): the file cannot re-root its own sources and cannot print.
- A required-argument line with no `=` is a fault (FR-036).
- A name with no row in the table is a fault (FR-033): the table is the
  only place an option is recognized.
- A flag line written `name=value` reaches the flag reader of FR-025,
  which answers its own refusal for a word outside the four.

Line numbers count from 1 over the raw lines of the text, so a comment
and a blank line hold a number and a fault line points at the line the
reader sees in an editor (D-12).

## The values

A line sets its option under the command-line value rules (FR-035): the
same reader, the same refusal. A repeatable accumulates within one file
in line order, and the whole list is the file's list when the gather
compares sources (FR-035, FR-038).

## The faults

Each answers one line on standard error at status 2, and it stands when a
higher source names the same option with a value that reads (FR-033,
SC-003).

| Case | Line |
| --- | --- |
| the file cannot be read | `--options: the file '<path>' cannot be opened: <reason>` |
| a second `--options` | `--options cannot stand twice` |
| a refused name | `<path>:<line>: the option '<name>' cannot stand in an option file` |
| a line starting with `-` | `<path>:<line>: expected an option name without --, got '<text>'` |
| a required-argument line with no `=` | `<path>:<line>: option '--<name>' needs a value` |
| a name with no row | `<path>:<line>: unknown option '<name>'` |
| a value its reader refuses | `<path>:<line>: ` then the reader's own line |

The unreadable-file line reuses the text of the moved reader, which is
what the report loader already answers for a document path (research
D-05). The second `--options` line names the option alone, as FR-032
states: the first file's path is not repeated.

## Oracle

`harness_cli_sources_test` drives the gather with `OptionFileText`
values built in the test: a file that sets a flag, a repeatable that
accumulates in line order, a comment and a blank line that hold line
numbers (a fault on the line after them names the number), each of the
six fault lines above byte for byte, and the precedence of FR-037
against a command line that sets the same option. The unreadable-file
line is checked at the entry point, where the read stands, through the
`base_cli_matrix` runner over a path under a directory that does not
exist.
