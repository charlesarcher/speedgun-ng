# Contract: The Option Table and the Scan

**Surfaces:** `detail::optionRows()`, `detail::OptionRow`,
`detail::scanCommandLine` (research D-01, D-02, D-03, D-06, D-07; data
model E-01, E-06). **Covers:** FR-001 to FR-014, FR-079, FR-082; SC-001.

FR-078, FR-080 and FR-085 have no contract subject by decision: the
version rule of the spec's Roadmap position and research D-19 own
FR-078, the coverage gate of the `ci-coverage` preset and SC-010 own
FR-080, and research D-31 with the DBC pair gate own FR-085. SC-014 has
no contract subject either: it is the gate set itself, held by T061 for
the coverage chain and T062 for the rest of the set.

## The table

One array in `source/harness/options.cpp`, in this order, behind
`detail::optionRows()`. The master rows keep their spelling and their
internal value; the new rows take the numbers spec D-24 reserves.

| `name` | `argument` | environment flag | `value` | owner |
| --- | --- | --- | --- | --- |
| `filter` | required | yes | `'f'` | specs/015 |
| `list` | none | no | `'l'` | specs/015 |
| `repetitions` | required | yes | `'r'` | specs/015 |
| `min-time` | required | yes | `'m'` | specs/015 |
| `iterations` | required | yes | `'i'` | specs/015 |
| `warmup-time` | required | yes | `'w'` | specs/015 |
| `dry-run` | none | no | `'d'` | specs/015 |
| `counter` | required | yes | `'c'` | specs/015 |
| `metric` | required | yes | `'M'` | specs/015 |
| `catalog` | none | no | `'C'` | specs/015 |
| `percentiles` | required | yes | `'p'` | specs/015 |
| `no-percentiles` | none | yes | `'P'` | specs/015 |
| `placement` | required | yes | `'A'` | specs/024 |
| `topology` | none | no | `'T'` | specs/024 |
| `json` | required | no | `'j'` | specs/015 |
| `json-raw` | required | no | `'J'` | specs/023 |
| `compare` | required | no | 257 | specs/026 |
| `compare-rule` | required | no | 258 | specs/026 |
| `compare-strict` | none | no | 259 | specs/026 |
| `metric-method` | required | no | 256 | specs/025 |
| `options` | required | no | 290 | FR-029 |
| `env` | required | no | 291 | FR-028 |
| `threads` | required | yes | 292 | FR-044 |
| `interleave` | optional | yes | 293 | FR-051 |
| `compare-rules` | required | no | 294 | FR-061 |
| `help` | none | no | 295 | FR-015 |
| `version` | none | no | 296 | FR-016 |

Twenty-seven rows. The values 297 to 319 stay free.

**Invariants.** Every `value` is unique (FR-004, SC-001). No row carries
a short letter and the option string of the scan carries none (FR-005).
Every name is lower case, digits and `-` only, and matches the
`README.md` option table row for row (FR-068, checked by the drift
tool). The environment name of a flagged row is `SPEEDGUN_` plus the
name upper-cased with `-` changed to `_`, computed from the row
(FR-019, D-09).

## The scan

```cpp
auto detail::scanCommandLine(int argc, char** argv)
    -> std::expected<CommandLineOptions, std::string>;
```

**Precondition:** `argc` and `argv` hold a command line in the shape a
shell hands to `main`. **Postcondition:** the answer holds
`CommandLineOptions` (E-06) or one fault line, and the call changes no
process state beyond the `getopt` scanner's own.

The scan sets `opterr = 0` and passes the option string `":"` (FR-006),
resets `optind` to 0 as the base does at `source/harness/cli.cpp:376`,
and reads the return code with these meanings (measured in research
D-06):

| Code | `optopt` | Reading |
| --- | --- | --- |
| `':'` | the row's `value` | the option took no value: the missing-value fault |
| `'?'` | 0 | a long-option fault; the typed text sits at `argv[optind - 1]` |
| `'?'` | the row's `value` | a row that takes no value was given one; the typed text sits at `argv[optind - 1]` and the unknown-name line answers it (measured: `--help=1` answers `optopt = 295`) |
| `'?'` | a character | a short option, unknown because the option string is empty |
| `'-'` | unused | a non-option argument, which stays the batch filter |

The scan calls no `std::exit` and prints nothing (FR-010, FR-011). The
entry point prints the fault line and returns 2. The `std::exit(2)` of
the family expansion at `source/harness/family.cpp:382` stays (FR-014).

## The three usage faults

Each answers one line on standard error at status 2, and no other line
appears on either stream (FR-007, FR-008, FR-009; SC-003).

| Case | Line |
| --- | --- |
| unknown name | `unknown option '<text>'; --help lists the options` |
| unknown name within two edits of a row | `unknown option '<text>', nearest '--<name>'` |
| ambiguous prefix | `ambiguous option '<text>': <candidate>, <candidate>` |
| missing value | `option '--<name>' needs a value` |

The candidates of the ambiguous line are the row names that start with
the typed text, in table order, and the scan computes them itself
because the C library reports no list once `opterr` stands at 0 (spec
D-25). The nearest name is the row name of least Levenshtein distance,
reported only when that distance stands at or below two.

## The exit statuses

The status the entry point answers belongs to this contract: 0 for the
help line and the version line with no benchmark run, and 2 for each
usage fault the scan answers, one line on standard error (FR-007 to
FR-009, FR-015, FR-016). The exit status table of
`specs/015-benchmark-harness-core/contracts/cli.md:83` gains those two
rows beside the rows the environment value, option file, thread list,
interleave seed and rules file faults answer (FR-082).

## Oracle

`harness_cli_table_test` reads `detail::optionRows()` and asserts the
name and value uniqueness, the value block, the absence of short
letters, the master name to value mapping as a literal table (FR-013),
the ambiguity line and the nearest-name line. `harness_cli_sources_test`
drives `scanCommandLine` with `argv` arrays built in the test and
compares each fault line byte for byte. `base_cli_matrix` runs the
built binary over the twenty-six base command lines and compares the
status and the fault fragment; the rows for an unknown option, an
ambiguous prefix and a missing value are marked as changed because the
base printed two lines for each, and the `--help` and `--version` rows
are marked as changed because the base refused both at status 2.
