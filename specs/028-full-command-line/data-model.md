# Data Model: Full Command Line

**Date:** 2026-10-22 · **Spec:** [spec.md](./spec.md)

Entities E-01 to E-13 extend the spec's Key Entities to the types the
code needs. Every one of them is internal: the public headers gain no
symbol (FR-076, D-24), so each lands under `source/harness/`. Field
spellings follow N-12 (camelBack members) and N-4 (UPPER_SNAKE
enumerators). The `file:line` anchors were re-measured on this tree at
`bd10d76` in the pass that wrote this file.

## E-01: `OptionRow`, one row of the option table

The one table is an array of these rows in `source/harness/options.cpp`
behind `detail::optionRows()` (D-01, D-02).

| Field | Type | Invariant |
|---|---|---|
| `name` | `const char*` | the long spelling without the leading `--`; a string literal, so the pointer is valid for the whole run and `getopt_long` can read it |
| `value` | `int` | the internal value the scan switches on; unique across the table (FR-004, SC-001); the master letters stay, and 256 to 259 keep their options |
| `argument` | `OptionArgument` | `NONE`, `REQUIRED` or `OPTIONAL`; only `--interleave` is `OPTIONAL` (spec D-17) |
| `fromEnvironment` | `bool` | set for exactly the twelve rows FR-020 names (FR-021, D-27) |
| `help` | `const char*` | the text `--help` prints and the guide reference copies (FR-015, FR-018, FR-069) |
| `owner` | `const char*` | the requirement that owns the row (FR-001) |

The `option` array `getopt_long` takes is derived from these rows at the
head of the scan: `name` passes through, `has_arg` follows from
`argument`, `flag` stays null and `val` is `value`.

## E-02: `EnvironmentRow`, one `SPEEDGUN_*` name and its value

| Field | Type | Invariant |
|---|---|---|
| `name` | `std::string` | the variable name as the process holds it, upper case with `_` separators (FR-019) |
| `value` | `std::string` | the raw text; the reader of the row's value kind interprets it (FR-024, FR-025, FR-027) |

`detail::environmentRows()` answers the vector in `cli.cpp` and the
gather takes it as an argument, so no test touches the process
environment (FR-012, D-04). A name that matches no flagged row stays in
the vector and is ignored there (FR-026).

## E-03: `OptionFileText`, one `--options` file

| Field | Type | Invariant |
|---|---|---|
| `path` | `std::string` | the text `--options=` named; every fault line of FR-031, FR-033 and FR-036 names it |
| `text` | `std::string` | the whole file as read; `cli.cpp` reads it through `detail::readTextFile` and a refused file answers FR-034 before the gather runs (D-05) |

A parsed line is a local pair of `std::string_view` plus the 1-based
line number over the raw lines, so a comment and a blank line hold a
number (D-12).

## E-04: `OptionSource`, where a value came from

| Enumerator | Meaning |
|---|---|
| `COMMAND_LINE` | the word the operator typed |
| `OPTION_FILE` | a line of the `--options` file |
| `ENVIRONMENT` | a flagged row's `SPEEDGUN_*` variable |

The order of the three is the precedence of spec D-1 (FR-037). The
`source` text of the `run` object (E-09) is `file` for `OPTION_FILE` and
`environment` for `ENVIRONMENT`; a `COMMAND_LINE` use is never disclosed
(FR-039, FR-040).

## E-05: `OptionUse`, the disclosure entry

| Field | Type | Invariant |
|---|---|---|
| `row` | `const OptionRow*` | points into the table; the row's `name` and its table position carry the option text and the order (FR-039, FR-040) |
| `source` | `OptionSource` | the source that won for this option |
| `value` | `std::string` | the text as the source spelled it; a repeatable joins its items with `;` (FR-040) |

The gather answers one use per option that a non-command-line source
settled, in table order. `--interleave` given on the command line
produces no use: the seed travels in E-10 and the console entry for it
is `interleave=<seed>` (FR-039).

## E-06: `CommandLineOptions`, the scan result

| Field | Type | Invariant |
|---|---|---|
| `options` | `RunOptions` | the existing internal struct (`source/harness/detail/internal.hpp:434`) with the new fields of E-11, E-10, E-03 and the rules-file path filled from the words typed |
| `uses` | `std::vector<OptionUse>` | one use per command-line option, in scan order; the rule list keeps its order for FR-063 |
| `stop` | `StopKind` | `NONE`, `HELP` or `VERSION`: the first of `--help`, `--version` or a usage fault the walk met (FR-017, D-08) |

The scan answers this, or one usage error text, and never calls
`std::exit` (FR-010).

## E-07: `ResolvedOptions`, the gather result

| Field | Type | Invariant |
|---|---|---|
| `options` | `RunOptions` | every option at the value of the highest source that named it (FR-037); a repeatable holds one list, the winner's (FR-038) |
| `uses` | `std::vector<OptionUse>` | the disclosed uses in table order, empty when nothing came from a file or the environment (FR-039, FR-041) |
| `interleave` | `InterleaveSeed` | E-10, settled across the sources |
| `rules` | `std::vector<CompareRule>` | the rules of the highest source that names rules, in spec D-23 order (FR-038, FR-063, E-12) |

The gather answers this, or one usage error text naming the source, the
line number where a file is named, and the value.

## E-08: `RunDisclosure`, what the writer is given

| Field | Type | Invariant |
|---|---|---|
| `uses` | `std::span<const OptionUse>` | the disclosed uses of E-07 in table order |
| `interleave` | `std::optional<std::uint64_t>` | the seed, empty when `--interleave` stands absent (FR-040) |

`detail::writeReport` takes it as a parameter (D-24), and
`ReportData` stays as the public header declares it (FR-076).

## E-09: the `run` object, the document entity

The top-level key `run` of the report document, written only when a use
exists or the seed stands (FR-039, FR-041).

| Key | Type | Invariant |
|---|---|---|
| `interleave` | unsigned integer | present only when `--interleave` was given, on the command line or through `SPEEDGUN_INTERLEAVE` (FR-040, FR-058) |
| `options` | array of objects | one object per disclosed use, in table order; each object holds the string keys `option`, `source`, `value` (FR-040) |

The object stands between `speedgun` and `benchmarks` in the written
document and in the schema's `x-order` (`docs/schema/report-v1.json:7`),
`required` at `:8` stays as the base states it, and the members of the
object stay open (FR-043, D-18). The reader reads no key of it (D-30).

## E-10: `InterleaveSeed`, the interleaving state

| Field | Type | Invariant |
|---|---|---|
| `given` | `bool` | set by `--interleave`, by `--interleave=<seed>`, or by `SPEEDGUN_INTERLEAVE` (FR-051, FR-022) |
| `seed` | `std::uint64_t` | the value the reader read; 0 for the bare form (FR-051) |

The seed text is unsigned decimal digits alone, consumed whole, at or
below the largest 64-bit unsigned value; `-1`, `0x10`, `7x`, `true` and
an over-limit digit string answer the FR-054 fault naming the option or
the variable and the text. `SPEEDGUN_INTERLEAVE=0` means given with seed
0.

## E-11: `ThreadList`, the `--threads` value

A `std::vector<int>` the reader answers from the option text, written
into `RegistryEntry::threadCounts`
(`source/harness/detail/internal.hpp:199`) for every entry of
`detail::registry()` (`:244`) before the expansion at
`source/harness/cli.cpp:570` (D-14).

| Item form | Reading |
|---|---|
| `<count>` | one count; below one is the FR-046 fault |
| `<min>:<max>` | the counts of `detail::buildThreadCounts` (`internal.hpp:315`): doubling from `min` to at most `max`; `min` above `max` is the FR-046 fault |

The list is empty when the option stands absent, and the registered
counts then stand (FR-044). The list caps nothing: a count above the
affinity cpu count runs and the console discloses it (FR-047).

## E-12: `RulesFile`, the `--compare-rules` value

| Field | Type | Invariant |
|---|---|---|
| `path` | `std::string` | the text `--compare-rules=` named; FR-064 and FR-065 name it |
| `rules` | `std::vector<CompareRule>` | one rule per line that the specs/026 parser read, in line order (FR-061, FR-062) |

A line the parser refuses answers one line naming the file, the 1-based
line number and the parser's own text at status 2 (FR-064). The rules
append to the `--compare-rule` uses of the same source in that order. The
rule list does not combine across sources: the highest source that names
rules settles it, and a rule of a lower source reaches the parser before
it drops, so its fault still answers (FR-038, FR-063, D-13).

## E-13: `HeldRow`, a row a shuffled run holds

| Field | Type | Invariant |
|---|---|---|
| `index` | `std::size_t` | the position of the instance in the registration order; the print pass walks these in that order (FR-056) |
| `result` | `BenchmarkResult` | the run result the loop produced at `source/harness/cli.cpp:645` |
| `rowRaw` | `RawColumns` | the raw column set the runner holds beside it, the same value the base path pushes at `:648` |
| `contextRaw` | `RawColumns` | the context column set beside it, as at `:649` |

A family's fit is held beside the rows of its family and prints after
the last of them in registration order (D-16). The held rows reach
`ReportData` in the same order the console prints them, so the document
and the console agree.
