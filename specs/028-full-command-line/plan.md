# Implementation Plan: Full Command Line

**Branch**: `028-full-command-line` | **Date**: 2026-10-22 | **Spec**:
[spec.md](./spec.md)

**Note**: the `dev` presets are machine-local and a fresh clone carries
none, so every command in this plan runs the checked-in release preset
`ci-rocky`, a Release developer-mode build into `build`.

**Input**: [specs/028-full-command-line/spec.md](./spec.md)

## Summary

The harness reads its options from one table, and this feature makes
that table the only place an option is recognized. Seven long options
join it (`--options`, `--env`, `--threads`, `--interleave`,
`--compare-rules`, `--help`, `--version`), and each row carries its
argument kind, its help text, whether the environment may set it, and
the requirement that owns it. The scan moves out of `speedgunMain` into
a new internal unit, answers the parsed set or one usage error, and
stops calling `std::exit`: the entry point prints the line and returns
2. Two more sources feed the same rows, a `SPEEDGUN_*` environment
block and an `--options=<file>` text file, and the value of an option
comes from the highest source that names it. A run that took anything
from a source other than the command line, or that ran interleaved,
writes one `run` object into the report document and prints one console
line naming what it took. `--threads=<list>` replaces the thread counts
of every family before the expansion, and `--interleave` walks the
selected instances in a seeded order while the printed rows and the fit
keep registration order. `--compare-rules=<file>` reads rules in the
specs/026 grammar and appends them in the order spec D-23 fixes.

The timed loop is untouched. The shuffle and the parse sit on the cold
path before the loop, the run loop keeps its one interrupt-flag reading
and its `runner.run(*entry)` line, and the three shape gates
(`test/cli_shape.sh`, `test/loop_shape.sh`, `test/time_source_gate.sh`)
stay green without a change to any of them.

**Type**: [feature]

**Execution mode**: TDD (Principle III). Every test task is observed red
against the code that stands when the task runs, and the code task that
turns it green follows it. Where a case is green on arrival because an
earlier task in the same phase already landed the code node, the task
proves liveness by a planted mutation that makes the case fail with the
named expectation, and the revert is byte-identical.

## Technical Context

| Item | Value |
| --- | --- |
| Language | C++23, `-std=c++23`, the pedantic warning floor of the `cmake-pedantic` hidden preset |
| Configure (daily) | `cmake --preset=ci-rocky`, `binaryDir` `${sourceDir}/build` |
| Configure (static analysis) | `cmake --preset=ci-ubuntu`, same `binaryDir`, clang-tidy and cppcheck wired in |
| Sanitizers | `cmake --preset=ci-sanitize` into `build/sanitize`, `cmake --preset=ci-tsan` into `build/tsan` |
| Coverage | `cmake --preset=ci-coverage` into `build/coverage`, `CMAKE_BUILD_TYPE=Coverage` |
| Build and test | `cmake --build build`, then `ctest --test-dir build --output-on-failure` |
| Harness target | `speedgun-ng_harness` at `CMakeLists.txt:200`, sources globbed at `:196` to `:199`, `SOVERSION 10` at `:261` |
| Library archive | `SOVERSION 3` at `CMakeLists.txt:53`; the project `VERSION 0.18.0` at `:9` |
| JSON | the vendored simdjson 4.6.11: `simdjson::builder::string_builder` writes (`source/harness/json_report.cpp:26`), `simdjson::dom` reads (`source/harness/json_load.cpp:1198`) |
| Standard library facilities new to this tree | `getopt_long` with `opterr` and `optopt` (already used at `source/harness/cli.cpp:377`), `environ`, `std::to_array`, `std::span`, `std::mt19937_64`, `std::raise` |
| Facilities unchanged | `sched_getaffinity` for every cpu count, `std::regex` for the batch filter, `std::expected` for the internal fault channel |
| Platform | Linux only, as the spec's Audit point records |

## Decisions per capability

### C-1: One option table (FR-001 to FR-014)

The table is one array of `detail::OptionRow` in `source/harness/options.cpp`
behind `detail::optionRows()` (D-01, D-02, E-01). The `option` array
`getopt_long` takes is built from the rows at the head of the scan, so
no option is recognized anywhere else (FR-002). The master rows keep
their spelling and their internal values, the letters of the base table
at `source/harness/cli.cpp:346` to `:371` and the four constants 256 to
259 (`:69` to `:71`, `:345`); the seven new rows take the named
constants 290 to 296 that spec D-24 assigns, and 297 to 319 stay free
(FR-004). No row carries a short letter and the option string carries
none (FR-005).

The scan sets `opterr = 0` and passes `":"` (FR-006), then reads the
return code and `optopt` as measured in D-06: `':'` is a missing value,
`'?'` with `optopt` at 0 is a long-option fault whose text sits at
`argv[optind - 1]`, and `'?'` with a character in `optopt` is a short
option. The parse computes the candidate set for an ambiguous prefix
itself (spec D-25, FR-009) and computes the nearest name by Levenshtein
distance over the row names (FR-007). The texts are in
[contracts/option-table.md](./contracts/option-table.md).

The parse answers `std::expected` and calls `std::exit` nowhere
(FR-010, FR-011); the four reject helpers of the base and the nine
in-scan `return 2` sites collapse into one print in the entry point
(D-03). The `std::exit(2)` of the expansion at
`source/harness/family.cpp:382` stays (FR-014). The scan and the gather
take the environment rows and the option-file text as arguments, so a
test drives every source with data (FR-012, D-04, D-05).

### C-2: Help and version (FR-015 to FR-018)

`--help` prints one line per row in table order, the name with its
argument kind and the row's help text, and answers 0; `--version`
prints `speedgun-ng <version>` and answers 0 (D-08). Both print on
standard output and stop before any benchmark. The scan answers the
first of `--help`, `--version` or a usage fault that its left-to-right
walk meets, so `--help --nonsense=1` prints the help and
`--nonsense=1 --help` prints the fault. The help text has one source,
the row, which is what the guide reference test compares the page
against (D-22). See
[contracts/help-and-version.md](./contracts/help-and-version.md).

### C-3: Environment defaults (FR-019 to FR-028)

A row with the environment flag takes its default from `SPEEDGUN_` plus
the long name upper-cased with `-` changed to `_`, computed from the row
(D-09). The flagged rows are exactly the twelve FR-020 names; a mode
option, an output option and another lane's option carry none (FR-021,
D-27). `SPEEDGUN_INTERLEAVE=<seed>` turns `--interleave` on with that
seed (FR-022). The percentile pair is checked over the environment rows
before precedence (FR-023, D-10). A repeatable splits on `;` and a flag
takes `1`, `0`, `true`, `false` (FR-024, FR-025, D-11); an unknown
`SPEEDGUN_*` name is ignored (FR-026); a value its own reader refuses
names the variable and the value (FR-027). `--env=off` turns the
environment off for one run and stands on the command line alone
(FR-028). See [contracts/environment.md](./contracts/environment.md).

### C-4: The option file (FR-029 to FR-036)

`--options=<file>` reads one option per line in long spelling without
the leading `--`. The grammar, the comment and blank rules, the four
refused names, the second `--options` fault and the unreadable-file
fault are in [contracts/option-file.md](./contracts/option-file.md);
the line grammar and the numbering rule are D-12. The file text reaches
the gather as data (D-05), and the shared text reader moves out of
`source/harness/json_load.cpp:26` so the rules file of C-7 reads through
the same code.

### C-5: Thread counts from the command line (FR-044 to FR-050)

The parsed list is written into `RegistryEntry::threadCounts`
(`source/harness/detail/internal.hpp:199`) for every entry of
`detail::registry()` before `detail::expandRegistry`
(`source/harness/cli.cpp:570`), so the expansion at
`source/harness/family.cpp:404` and `:425` crosses the argument lists
with the new counts and the naming rule at `family.cpp:246` gives the
FR-048 names with no change (D-14). A `min:max` item expands through
`detail::buildThreadCounts` (`internal.hpp:315`), the doubling rule the
handle's `threadRange` already uses. The option caps nothing: a count
above the affinity count runs and the console discloses it (FR-047),
and the fit check at `cli.cpp:607` keeps the base refusal line (FR-049).
See
[contracts/threads-and-interleave.md](./contracts/threads-and-interleave.md).

### C-6: Interleaved execution (FR-051 to FR-060)

`detail::shuffledOrder(count, seed)` is a Fisher-Yates walk over
`std::mt19937_64` with a modulo reduction, and the expected
permutations for `count = 12` at seeds 7 and 8 are pinned in the test as
literals measured under both compilers (D-15). The seed binds only in
the attached form, which is what `optional_argument` gives: a following
word stays a separate argument (FR-052). The seed text is unsigned
decimal digits alone, consumed whole, at or below the largest 64-bit
unsigned value (FR-054). The run loop keeps its shape and its one flag
reading; a shuffled run holds its rows and prints them, and each
family's fit, in registration order after the loop (D-16). The
interrupt case runs a body that raises `SIGINT` at a named instance
(FR-057, D-17). See
[contracts/threads-and-interleave.md](./contracts/threads-and-interleave.md).

### C-7: The rules file (FR-061 to FR-065)

`--compare-rules=<file>` reads the text through the shared reader and
calls `sg::parseCompareRule` (`include/speedgun-ng/benchmark.hpp:2589`)
per line; the grammar stays as specs/026 wrote it (D-13). The rules of one
source keep the order their entries were written, spec D-23, and the rule
list does not combine across sources: the highest source that names rules
settles it, and a lower rule reaches the parser before it drops
(FR-038). [contracts/rules-file.md](./contracts/rules-file.md) states
both.

### C-8: Precedence and disclosure (FR-037 to FR-043)

The gather takes the sources in the order command line, option file,
environment, master default (FR-037), a repeatable set by a higher source
replaces the whole lower list, and the rule list is settled by the highest
source that names rules, a lower rule read before it drops (FR-038,
FR-063). The disclosure is one console line
and one `run` object (D-18, E-08, E-09): the line prints after the
version line and before the first row, and the object stands between
`speedgun` and `benchmarks` in the document and in the schema's
`x-order` (`docs/schema/report-v1.json:7`). The reader ignores the
object's contents and a document that carries it answers no fault
(FR-043, D-30). See
[contracts/disclosure-and-schema.md](./contracts/disclosure-and-schema.md).

### C-9: Documentation and example (FR-066 to FR-074)

The main-page section `The command line` (`README.md:357` to `:370`)
grows by 6 lines to 20 of the 25 the cap allows, the quick-reference
option table (`README.md:745` to `:766`) gains seven rows, and
`PAGE_LINE_CAP` (`tools/docs/example_drift.py:155`) moves from the
measured 773 to the measured 786 with the arithmetic recorded in the
gate's comment (D-21). `docs/pages/harness.md` gains the full option
reference in its section `The command line of an example` (the heading at
`:603`), beside the fenced block at `:611` to `:626`, and the two-way
comparison against the table is a case of `harness_cli_table_test`
(D-22). The shell region `options` of `example/docs/command_line.sh`
gains the typed command lines and the fenced block at
`docs/pages/harness.md:611` to `:626` grows to match byte for byte
(D-23). See [contracts/page-and-examples.md](./contracts/page-and-examples.md).

### Discipline (FR-075 to FR-086)

The option-row check of `tools/docs/example_drift.py` follows the table
to `source/harness/options.cpp` in the same task as the move, and the
liveness fixture moves with it (D-20, FR-075). The public headers gain
no symbol, so the harness `SOVERSION` stays at the value `master`
carries and the archive `SOVERSION` stays 3 (D-24, FR-076, FR-077). The
project version and the schema minor are read from master at rebase
(D-19, FR-078, FR-086). The fix branch 028b takes the patch 0.18.1, the
next version above master at its last rebase. The
new tests set no environment, run no sleep, read no clock and name no
cpu count (FR-081, D-29). The exit status table of
`specs/015-benchmark-harness-core/contracts/cli.md` gains the new rows
(D-25, FR-082). The three shape gates stay green with no edit (FR-083).
The `run` object is written with the vendored builder and the reader
keeps its error-code form (D-30, FR-084). Every new contract carries its
macro and its `\pre` or `\post` line in the form the pair gate reads
(D-31, FR-085). Coverage reaches 100% of the new lines and branches
with no exclusion marker (D-32, FR-080).

The same move fixes what a rebase does when a parallel lane merges
first. If PR #46 or PR #47 lands first, the rebase of this branch takes
these steps:

1. Move the option rows of the lane that merged first, the values 260
   to 269 of one lane and 270 to 276 of the other, from `cli.cpp` into
   `optionRows()` in `source/harness/options.cpp` and their constants
   into `source/harness/detail/options.hpp`, remove the old table from
   `cli.cpp`, and leave `tools/docs/example_drift.py` finding every row
   there.
2. Join the two `test/source/harness_cli_table_test.cpp` files keeping
   every case of each lane.
3. Add the schema keys of the lane that merged first to
   `docs/schema/report-v1.json`.
4. Keep the lines of each lane in `README.md`, `docs/pages/harness.md`,
   `tools/docs/example_drift.py` (`PAGE_LINE_CAP`, `SECTIONS`,
   `SECTION_LINE_CAPS`) and `test/CMakeLists.txt`, and set
   `PAGE_LINE_CAP` to the measured length.
5. Apply the version rule of FR-078 and the rule that the harness
   `SOVERSION` stays at the value `master` carries.

These steps stand open while both pull requests stay open, and no merge
of this branch is taken.

## Source Layout

Created:

- `source/harness/detail/options.hpp`: the `OptionRow` type, the
  `OptionArgument` and `OptionSource` enums, the seven new value
  constants beside the four carried from `cli.cpp:69` to `:71`,
  `optionRows()`, `EnvironmentRow`, `OptionFileText`,
  `CommandLineOptions`, `ResolvedOptions`, `OptionUse`, the scan and
  gather declarations, the value readers, `shuffledOrder`.
- `source/harness/options.cpp`: the table as one `std::to_array` array,
  the `getopt` array built from it, the scan, the gather, the nearest
  name and the candidate set, the environment name computation, the
  option-file and rules-file line readers, the thread list reader, the
  seed reader, the flag reader, `shuffledOrder`, `readTextFile` moved
  from `json_load.cpp`.
- `test/source/harness_cli_table_test.cpp`: the table checks, the
  ambiguity lines, the help and version output, the master name to value
  mapping, the guide reference comparison.
- `test/source/harness_cli_sources_test.cpp`: the scan and gather over
  built `argv` arrays, environment vectors and file texts; precedence,
  the one-line faults, the disclosure entries.
- `test/source/harness_cli_threads_test.cpp`: the thread list reader,
  the registry override, the instance names, the fit refusal, the
  disclosure of a count above the affinity count.
- `test/source/harness_cli_interleave_test.cpp`: the pinned shuffle
  vectors, the held rows and the fit position, the `SIGINT` body, the
  seed faults, the loop-shape claims.
- `test/data/base_cli_matrix.txt`: the twenty-six base command lines with
  their statuses and fault fragments, generated from a binary built at
  `bd10d76` (D-26).
- `test/base_cli/run_matrix.py`: the runner that drives the built binary
  over that file and compares.
- `specs/028-full-command-line/contracts/*.md`, `research.md`,
  `data-model.md`, `quickstart.md`, `tasks.md` (the last written by the
  task node).

Edited:

- `source/harness/cli.cpp`: the table, the scan, the four reject helpers
  and the nine in-scan `return 2` sites go; the entry point calls the
  scan, reads the environment rows, reads the option file, calls the
  gather, handles help and version, prints the disclosure line, applies
  `--threads` to the registry, walks the shuffled order and holds rows;
  the run loop keeps its `runner.run(*entry)` line and its one flag
  reading.
- `source/harness/detail/internal.hpp`: `RunOptions` gains the new
  fields, `readTextFile` is declared.
- `source/harness/compare.cpp`: the rule list the compare mode walks is
  assembled here in the order spec D-23 fixes for the source that settles
  it, once the rules file of C-7 has been read (FR-063, FR-064, FR-065).
- `source/harness/json_load.cpp`: the local `readFile` at `:26` to `:48`
  is deleted and its two call sites call `detail::readTextFile`.
- `source/harness/json_report.cpp`: `kSchemaVersion` at `:31` moves to
  the minor taken at rebase, `writeReport` gains the disclosure
  parameter and writes the `run` object between `speedgun` and
  `benchmarks`.
- `source/harness/detail/json_report.hpp`: the `writeReport`
  declaration follows the parameter.
- `docs/schema/report-v1.json`: `run` joins `x-order` at `:7` and
  `properties` at `:10`; `required` at `:8` stays.
- `tools/docs/example_drift.py`: `cli_options()` at `:458` to `:466`
  reads the new table file, `PAGE_LINE_CAP` at `:155` moves to the
  measured value with the arithmetic in the comment above it.
- `test/example-drift-fixture/run_example_drift_fixtures.py`: the stub
  table at `:117` to `:124` moves to the new path and shape.
- `README.md`: the section at `:357` to `:370` and the option table at
  `:745` to `:766`.
- `docs/pages/harness.md`: the option reference in the section at `:603`
  to `:684`, and the fenced block at `:611` to `:626` follows the shell
  region.
- `example/docs/command_line.sh`: the region `options` at `:14` to `:27`.
- `example/docs/command_line.cpp`: the `command-line-read` region grows
  the option file and the environment rows, and its recorded output
  region follows (T058, T059).
- `test/source/harness_cli_placement_test.cpp`: the unknown-option
  substring check at `:449` follows the new fault line.
- `test/source/harness_cli_test.cpp`: the usage fault cases follow the
  new fault lines the scan answers (T013).
- `test/source/harness_compare_cli_test.cpp`: the rules file cases, the
  rule order of spec D-23, the refused line and the unreadable path.
- `test/source/harness_json_report_test.cpp`: the console disclosure
  line, the `run` object in the document, and the stamped schema minor.
- `test/CMakeLists.txt`: four new test targets and the `base_cli_matrix`
  target, each with the source directory or the data directory as an
  argument.
- `specs/015-benchmark-harness-core/contracts/cli.md`: the exit status
  table at `:83`.
- `CMakeLists.txt`: the project `VERSION` at `:9` at rebase; the two
  `SOVERSION` lines stay.

## Docs and example plan

The main-page section keeps its marked block, so the snippet region
`command-line-read` of `example/docs/command_line.cpp` stays where it
is. The section grows 14 to 20 lines against the cap of 25
(`tools/docs/example_drift.py:156`), and no entry joins
`SECTION_LINE_CAPS` at `:161`. The option table gains seven rows, one
per new long option, in the existing row form. The page grows by 13
lines, so `PAGE_LINE_CAP` moves 773 to 786 and the comment above it
records the arithmetic the way the note at `:146` to `:154` does. The
guide page gains the reference table: name, argument kind, environment
variable where the row carries one, help text, in table order, plus the
precedence sentence spec D-1 states (FR-074). The shell region gains the
command lines a reader types for the option file, the environment, the
thread list, the interleaving and the rules file; the guide's fenced
block copies it byte for byte.

## Test plan

New ctest targets, each registered the way `harness_compare_test` is at
`test/CMakeLists.txt:634` to `:639`:

| Target | Covers |
| --- | --- |
| `harness_cli_table_test` | FR-001, FR-004, FR-005, FR-009, FR-013, FR-015, FR-016, FR-018, FR-069, FR-070, FR-074, FR-079; SC-001, SC-008 |
| `harness_cli_sources_test` | FR-006 to FR-008, FR-010 to FR-012, FR-019 to FR-038, FR-039 to FR-041; SC-002, SC-003 |
| `harness_cli_threads_test` | FR-044 to FR-050; SC-004 |
| `harness_cli_interleave_test` | FR-014, FR-051 to FR-060, FR-083; SC-005, SC-012 |
| `base_cli_matrix` | FR-003, FR-013, FR-059; SC-007, SC-013 |

The two-cpu clause of FR-081 is unverified on this fleet: every CI
runner reports four cpus and no job runs a two-cpu host, so that clause
rests on the affinity construction D-29 records and on review (D-29,
FR-081).

Extended targets: `harness_cli_test` for the entry point over the new
options and for the usage cases at `test/source/harness_cli_test.cpp:806`
and `:810`, whose `unknown option` expectation the new line keeps while
its new assertions name the typed text;
`harness_compare_cli_test` for the rules file (FR-061 to FR-065,
SC-006); `harness_json_report_test` for the `run` object and the
stamped minor (FR-039, FR-040, FR-043, FR-086); `harness_json_load_test`
for a document that carries `run` (FR-043, FR-084, D-30);
`harness_cli_placement_test.cpp:449` keeps its substring check over the
new line.

Gates that must stay green with no edit to the gate: `test/cli_shape.sh`
(the loop shape, D-16), `test/loop_shape.sh` (the timed loop, untouched),
`test/time_source_gate.sh` (no clock on the new paths), the DBC doc and
pair gates (D-31), the prose and spell gates, `snippet_compile`,
`report_shape` and its fixtures, `example_run`, and the coverage gates
at 100% line and branch with no `LCOV_EXCL` under `source/harness/`
(FR-080, SC-010). `tools/docs/example_drift.py` and its fixture harness
change in one task with the table move (FR-075, D-20).

## Version and ABI

The public headers gain no symbol: the parse, the table, the seams and
the disclosure types all sit in `source/harness/detail/`, and the
disclosure reaches the writer through a parameter of the internal
`detail::writeReport` rather than a field of the public `ReportData`
(D-24). The check is one diff run from the checkout against
`origin/master`:

```sh
git diff --name-only origin/master -- include/speedgun-ng
```

That command printed no path at `bd10d76`, and the `SOVERSION` lines at
`CMakeLists.txt:261` and `:53` read 10 and 3, so the harness `SOVERSION`
stays 10 and the archive `SOVERSION` stays 3 (FR-076, FR-077). The
project version takes the next free minor on master at rebase, recorded
as 0.18.0 under the planned merge order, and the schema minor takes the
next free minor the same way, recorded as 1.3 (D-19, FR-078, FR-086).

## Parallel work

Three features run at once on three machines. This branch rebases onto
master and never merges, and pushes once with `--force-with-lease`. The
version and the schema minor are read from master at rebase time, and
the values recorded here (0.18.0 and 1.3) hold only under the planned
merge order (D-33). The shared files are `CMakeLists.txt` (the version
line and the two `SOVERSION` lines), `README.md`,
`tools/docs/example_drift.py`, `test/CMakeLists.txt`,
`example/CMakeLists.txt` and the docs example list; a rebase resolves
them by re-measuring the page length and re-reading the version line.

## Risk Table

| Risk | Impact | Mitigation |
| --- | --- | --- |
| The run loop changes shape and `test/cli_shape.sh` goes red | The performance discipline gate fails the branch | The walk order and the held-row print sit outside the loop range the script reads (`:32`, `:39`, `:47`); the gate runs in the first task of the interleaving phase |
| The table moves and the drift scraper keeps reading `cli.cpp` | The docs gate reports a clean tree that it did not read | The scraper and the fixture move in the same task as the table (D-20), and the fixture's planted missing row proves the check can fail |
| A usage fault still reaches a process exit | SC-013 fails and a test cannot see the fault text | The parse answers `std::expected`; the entry point prints and returns; `base_cli_matrix` compares every status against the base file |
| The shuffle uses a distribution and the order moves between libraries | FR-053 becomes false | The pinned vectors of D-15 are measured under both compilers, and the shuffle reads the engine stream alone |
| A held-row print changes the base console order | FR-059 fails | Two print paths (spec D-19); the base path keeps the row print at `cli.cpp:646` and the base matrix test covers it |
| The `run` key lands out of the schema order | The report shape check fails on every disclosure run | The key position is fixed in both the writer and `x-order` in one task (D-18), and the schema check runs over a written document in the report test |
| A new option takes a value block another lane reserved | Two branches answer different values for one name | The blocks are spec D-24's, 290 to 296, and the uniqueness test (FR-079) reads the final table after every rebase |
| The environment seam leaks a real `SPEEDGUN_*` row into an unrelated test | A test passes or fails on the machine's environment | No test reads the process environment: the rows arrive as a vector, and the one end-to-end case builds its child environment explicitly (D-04) |

## Task Planning Summary

| Phase | Tasks |
| --- | --- |
| Phase 0: branch, baseline record and the base matrix oracle | 5 |
| Phase 1: the table, the scan and the parse split | 12 |
| Phase 2: help and version | 4 |
| Phase 3: the environment source | 8 |
| Phase 4: the option file, precedence and the disclosure | 9 |
| Phase 5: thread counts from the command line | 5 |
| Phase 6: interleaved execution | 7 |
| Phase 7: the rules file | 4 |
| Phase 8: docs, example, gates, version and ABI | 8 |
| Total | 62 |
