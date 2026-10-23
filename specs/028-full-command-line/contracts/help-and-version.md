# Contract: Help and Version

**Surfaces:** `--help`, `--version`, the `stop` field of
`CommandLineOptions` (research D-08; data model E-06). **Covers:**
FR-015 to FR-018; SC-008.

## The output

`--help` prints one line per table row, in table order, on standard
output, then answers status 0. The line shape is fixed:

```text
  --<name><argument part>  <help text>
```

Two leading spaces, the long name with its leading `--`, then the
argument part: ` <value>` for a required argument, ` [<value>]` for an
optional one, nothing for none, then two spaces, then the row's `help`
text. No header line, no column padding beyond the two spaces, no
trailing blank line.

`--version` prints one line on standard output and answers status 0:

```text
speedgun-ng <version>
```

`<version>` is the project version the console report already carries,
`SG_PROJECT_VERSION` (`CMakeLists.txt:231`, printed by
`detail::printContext` at `source/harness/report.cpp:317`).

## The rules

Both options print on standard output, stop before any benchmark runs,
and are accepted on the command line alone (FR-017): neither carries the
environment flag, and the option file refuses both names (FR-031).

The scan answers the first of `--help`, `--version` or a usage fault
that its left-to-right walk meets (FR-017). So `--help --nonsense=1`
prints the help at status 0, `--nonsense=1 --help` prints the unknown
option line at status 2, and `--help --version` prints the help alone.

The help text has one source: the `help` field of the row (FR-018). No
second list of descriptions exists anywhere in the tree, and the guide
reference table copies the same field (FR-074, research D-22).

## Contract lines

The two options carry no internal function of their own: the scan
answers `stop` and the entry point prints. The entry point's
postcondition keeps its existing predicate over the three run-wide flags
at `source/harness/cli.cpp:690`, because both answers return before the
run loop starts (research D-25).

## Oracle

`harness_cli_table_test` builds the expected help text from
`detail::optionRows()` and compares it against the recorded run of
`--help`, so the printed text and the table cannot drift; it also
asserts the version line against `SG_PROJECT_VERSION`, that both answers
run no benchmark (no `benchmark` column line on standard output), and
that both answer 0. `harness_cli_sources_test` asserts the walk order
over the three orderings above. `base_cli_matrix` records that the base
refused both words at status 2, and marks those two rows as changed.
