# Task List: spec 028 full command line

**Feature**: `specs/028-full-command-line`
**Branch**: `028-full-command-line`, base `bd10d76`
**Execution mode**: TDD (Principle III). Every test task is observed red
before the code task that turns it green.

**Notes**: a bare `D-nn` is an entry of [research.md](./research.md); a spec
decision is written `spec D-nn`. A task owns the files it names as its write
target; a shared file named by two tasks is extended by the later task and
never rewritten by it.

## Phase 0: branch, baseline record and the base matrix oracle

- [x] T001: Record the base command-line behavior by running the
  twenty-six base command lines through a binary built at `bd10d76` and
  writing one row per command line with its status and its fault
  fragment, writes `test/data/base_cli_matrix.txt` , proves a python
  check over the file prints 26 rows each carrying a status of `0` , `1`
  or `2` and a fault fragment where the base prints one (FR-003, FR-013,
  FR-059, SC-007, SC-013).
- [x] T002: Write the matrix runner that drives the built harness over
  every row of that file and compares the status and the fault fragment,
  writes `test/base_cli/run_matrix.py` , proves it prints `26 of 26`
  over a binary built at `bd10d76` and prints one failure line naming
  the row when a copy of the file carries a planted wrong status
  (FR-003, SC-013).
- [x] T003: Register the matrix target and the table test target the way
  `harness_compare_test` is registered at `test/CMakeLists.txt:634` to
  `:639` , writes `test/CMakeLists.txt` , proves
  `ctest --test-dir build -R base_cli_matrix` runs the script against
  the built harness and reports `26 of 26` , and proves
  `ctest --test-dir build -R harness_cli_table_test` names a target that
  builds and runs: that test reads the source text of
  `source/harness/cli.cpp` , so it links at this head, it stays red
  until T007 lands the table, and it carries no `WILL_FAIL` property
  (FR-003).
- [x] T004: State the table contract first, the 27 rows of
  `contracts/option-table.md` in order and the seven new names `options`
  , `env` , `threads` , `interleave` , `compare-rules` , `help` and
  `version` at the named constants 290 to 296, writes
  `test/source/harness_cli_table_test.cpp` , proves
  `ctest --test-dir build -R harness_cli_table_test` fails on the named
  case that reads 27 rows and the seven new names at the constants 290
  to 296 (FR-001, FR-004, FR-079, SC-001).
- [x] T005: Pin the shuffle with the two permutation literals for
  `count = 12` , seed 7 as `4 0 2 6 10 9 1 5 11 8 7 3` and seed 8 as
  `7 0 6 10 8 2 11 5 9 4 3 1` , writes
  `test/source/harness_cli_interleave_test.cpp` , proves the two
  permutation literals for count 12 at seeds 7 and 8 equal a scratch
  measurement under `g++` and `clang++` and that the file does not link
  against the harness, `detail::shuffledOrder` standing in no task
  before T044 defines it and registers the target (FR-053, SC-005).

## Phase 1: the table, the scan and the parse split

- [x] T006: Declare the option surface: `OptionRow` , `OptionArgument` ,
  `OptionSource` , `EnvironmentRow` , `OptionFileText` ,
  `CommandLineOptions` , `ResolvedOptions` , `OptionUse` , the scan and
  gather declarations, the value readers and `shuffledOrder` , with the
  seven new value constants 290 to 296 beside the four carried from
  `source/harness/cli.cpp:69` to `:71` and `:345` , writes
  `source/harness/detail/options.hpp` , proves it compiles inside
  `source/harness/options.cpp` (FR-001, FR-004, FR-006).
- [x] T007: Write the table as one `std::to_array` array behind
  `detail::optionRows()` and build the `getopt_long` option array from
  the rows, writes `source/harness/options.cpp` , proves the row case of
  `harness_cli_table_test` passes with 27 rows, the getopt array built
  from the rows and `opterr` at 0 (FR-001, FR-002, FR-003, FR-005,
  FR-079).
- [x] T008: Follow the scraper to the table: `cli_options()` at
  `tools/docs/example_drift.py:458` to `:466` reads
  `source/harness/options.cpp` and the fixture stub table at
  `test/example-drift-fixture/run_example_drift_fixtures.py:117` to
  `:124` moves with it, writes `tools/docs/example_drift.py` and
  `test/example-drift-fixture/run_example_drift_fixtures.py` , proves
  the fixture suite passes with its planted missing row still failing,
  and proves `python3 tools/docs/example_drift.py .` reports the seven
  README option rows that T055 adds and nothing else: the main page
  cannot carry the new rows before they exist, so a zero exit here would
  mean the scraper stopped comparing (FR-075, SC-009).
- [x] T009: Move the scan onto the table, so `speedgunMain` calls
  `detail::scanCommandLine` and the `longOptions` array, the option
  string and the four reject helpers of the base go, writes
  `source/harness/cli.cpp` , `source/harness/options.cpp` (the scan body
  moves behind the seam, answering the base faults as it does today)
  and `test/CMakeLists.txt` , registering `harness_cli_sources_test` in
  the pass that defines the seam and so first makes it linkable, proves
  `ctest --test-dir build -R 'harness_cli_test|base_cli_matrix'` passes
  and `bash test/cli_shape.sh` stays green, and states that
  `harness_cli_sources_test` runs red until T011 splits the parse from
  the exit. The same pass re-marks the four `base_cli_matrix` rows of
  `--help`, `--version`, `--threads` and `--interleave`: once the scan
  walks the 27-row table those names are recognized, so the base's
  `getopt` line no longer appears and the shortest fault line is the
  harness line naming the option table, which is the change
  `contracts/option-table.md` records for those rows (FR-002, FR-005,
  FR-010, FR-012, FR-014, SC-002, SC-013).
- [x] T010: State the scan faults first, each one line on standard error
  at status 2: `unknown option '<text>'; --help lists the options` ,
  `unknown option '<text>', nearest '--<name>'` ,
  `ambiguous option '<text>': <candidate>, <candidate>` ,
  `option '--<name>' needs a value` , writes
  `test/source/harness_cli_sources_test.cpp` , proves the file compiles
  under `-Wall -Wextra -Werror` with the generated export header on the
  include path and that its object carries exactly one undefined seam,
  `detail::scanCommandLine` : the target cannot link before T009 defines
  that seam, so T009 owns the registration in `test/CMakeLists.txt` ,
  this task runs no ctest, and the red state is the link error
  (FR-009, FR-010, FR-011, FR-012, SC-003).
- [x] T011: Split the parse from the exit so `scanCommandLine` answers
  `std::expected<CommandLineOptions, std::string>` and calls no
  `std::exit` , writes `source/harness/options.cpp` and
  `source/harness/detail/options.hpp` , proves
  `harness_cli_sources_test` passes with the scan answering an expected
  value, the nearest-name suggestion in the form
  `unknown option '<text>', nearest '--<name>'` , and the nine in-scan
  status-2 returns gone (FR-009, FR-010, FR-011, FR-012, SC-013).
- [x] T012: Print the fault at the entry point, which writes the fault
  line of the scan to standard error and returns 2, writes
  `source/harness/cli.cpp` and follows the two count modes of
  `test/source/harness_gap_test.cpp:357` and `:364` that reported 0
  because the library exited for them, proves
  `ctest --test-dir build -R harness_cli_test` passes, proves
  `ctest --test-dir build -R harness_gap_test` passes, and proves no
  usage fault reaches a library exit (FR-010, FR-012, FR-035, SC-013).
- [x] T013: Follow the changed usage text in the two usage cases at
  `test/source/harness_cli_test.cpp:806` and `:810` : the unknown-name
  case keeps the `unknown option` substring the new line still carries,
  and the missing-value case follows the needs-a-value line
  `option '--<name>' needs a value` that replaces the base's second
  line, writes
  `test/source/harness_cli_test.cpp` , proves
  `ctest --test-dir build -R harness_cli_test` passes with the two usage
  cases that lose the second line (FR-010, FR-013).
- [x] T014: Follow the placement case whose substring check at
  `test/source/harness_cli_placement_test.cpp:449` reads the base
  unknown-option line, writes
  `test/source/harness_cli_placement_test.cpp` , proves
  `ctest --test-dir build -R harness_cli_placement_test` passes over the
  new unknown-option line (FR-010, FR-013).
- [x] T015: Share the value readers, the flag reader taking `1` , `0` ,
  `true` and `false` and the list and seed readers answering
  `--threads: expected counts and min:max ranges, got '<text>'` and
  `--interleave: the seed '<text>' is not an unsigned decimal value` ,
  writes `source/harness/options.cpp` , proves the reader cases of
  `harness_cli_sources_test` pass for the integer, double, flag, list
  and seed forms, each fault naming the value (FR-007, FR-008, FR-024,
  FR-025, FR-054).
- [x] T016: Keep every base option by pinning the master name to value
  mapping as a literal table, the sixteen letters and the four constants
  256 to 259, writes `test/source/harness_cli_table_test.cpp` , proves
  the name-to-value mapping case passes for all 20 base options (FR-003,
  FR-079, SC-001).
- [x] T017: Keep the table unique across the master rows and the seven
  new rows, writes `test/source/harness_cli_table_test.cpp` , proves the
  uniqueness case passes: no repeated name, no repeated value, every
  value inside its block, the master rows inside the letters and 256 to
  259 and the new rows inside 290 to 296 (FR-004, FR-079).

## Follow-up found while implementing Phase 1

- [x] T063: Name the typed word when a row that takes no value is given
  one, the `case '?'` branch of the scan deciding on the typed word so a
  word that starts with `--` answers
  `unknown option '--<name>'; --help lists the options` at status 2 and a
  single-dash word keeps the `-<letter>` line, writes
  `source/harness/options.cpp` and the stating case in
  `test/source/harness_cli_sources_test.cpp` , proves `--help=1` answers
  that one line at status 2 with no second line on either stream, proves
  matrix case23 (`-j`) still answers the `-<letter>` line, and proves
  `ctest --test-dir build -R harness_cli_sources_test` carries the new
  case (FR-007, SC-003).

  Verified 2026-10-22: `--help=1` answers the line the base answers at status 2 with nothing on standard output, the matrix still reads 26 of 26 so `-j` and `--nonsense=1` keep their lines, and the new table case is live under a planted needle.

## Phase 2: help and version

- [x] T018: State help and version first, one line per row in table
  order in the shape ` --<name><argument part> <help text>` , the
  version line `speedgun-ng <version>` , status 0 for both, and the
  first of `--help` , `--version` or a usage fault that the
  left-to-right walk meets winning, writes
  `test/source/harness_cli_table_test.cpp` , proves `ctest` fails on the
  cases for one line per row in table order, the version line, status 0
  and the first event in scan order winning (FR-015, FR-016, FR-017,
  FR-018, SC-008).
- [x] T019: Print help and version, the help line built from the `name`
  , `argument` and `help` fields of each row and the version line from
  `SG_PROJECT_VERSION` , writes `source/harness/options.cpp` , proves
  the help and version cases pass from the row text alone (FR-015,
  FR-016).
- [x] T020: Answer help and version at the entry point, writes
  `source/harness/cli.cpp` , proves
  `ctest --test-dir build -R harness_cli_test` passes, both print on
  standard output before any benchmark, and `base_cli_matrix` still
  passes. Matrix follow: rows `case02` and `case03` moved to status 0
  with no fault line, because `--help` and `--version` now answer the
  printed line at status 0 (FR-017, FR-018).
- [x] T021: Record the new statuses in the exit status table at
  `specs/015-benchmark-harness-core/contracts/cli.md:83` , writes
  `specs/015-benchmark-harness-core/contracts/cli.md` , proves the exit
  status table carries the help, version and new option rows (FR-082).

## Phase 3: the environment source

- [x] T022: state the environment names first, the twelve flagged rows
  each carrying `SPEEDGUN_` plus the long name upper-cased with `-`
  changed to `_` , and the fifteen unflagged rows carrying none, writes
  test/source/harness_cli_sources_test.cpp, proves ctest fails on the
  twelve names `SPEEDGUN_FILTER` , `SPEEDGUN_REPETITIONS` ,
  `SPEEDGUN_MIN_TIME` , `SPEEDGUN_ITERATIONS` , `SPEEDGUN_WARMUP_TIME` ,
  `SPEEDGUN_COUNTER` , `SPEEDGUN_METRIC` , `SPEEDGUN_PERCENTILES` ,
  `SPEEDGUN_NO_PERCENTILES` , `SPEEDGUN_PLACEMENT` , `SPEEDGUN_THREADS`
  , `SPEEDGUN_INTERLEAVE` and on the mode, output and other-lane options
  carrying none (FR-019, FR-020, FR-021).

  Verified 2026-10-22: the twelve flagged names and the fifteen unflagged rows stand in the file; the binary compiles clean and links red on exactly `detail::gatherOptionSources`, so the runtime reading follows T024 and T035.
- [x] T023: compute the name from the row, so `min-time` answers
  `SPEEDGUN_MIN_TIME` and `no-percentiles` answers
  `SPEEDGUN_NO_PERCENTILES` , writes source/harness/options.cpp, proves
  the name cases pass with the upper-cased long name and the dash change
  (FR-019, FR-020).

  Verified 2026-10-22: `environmentName(row)` computes the variable from the row, no name list stands, and `harness_cli_sources_test` now links and prints `ok`.
- [x] T024: bring the environment as data, `cli.cpp` calling
  `detail::environmentRows()` once through `environ` and handing the
  vector to `detail::gatherOptionSources` , writes
  source/harness/detail/internal.hpp and source/harness/cli.cpp, proves
  the gather cases pass over a built vector and that no test calls
  `setenv` , and records the two-cpu reading of FR-081 as unverified on
  this fleet, every CI runner reporting four cpus and no job running a
  two-cpu host, the record D-29 states (FR-019, FR-081, SC-002).

  Verified 2026-10-22: `detail::environmentRows()` stands at `source/harness/cli.cpp:156` as one walk of `environ` with no `setenv`, `putenv` or `getenv`, and the entry point hands it to `detail::gatherOptionSources` at `:253`. End-to-end readings from a scratch directory: `SPEEDGUN_FILTER=^bmRead SPEEDGUN_REPETITIONS=2` answers only the `bmRead` rows at status 0; `SPEEDGUN_MIN_TIME=abc` answers status 2 with zero lines on standard output and exactly one line on standard error, `SPEEDGUN_MIN_TIME: --min-time: expected a finite positive number of seconds at most 9223372036, got 'abc'`; `SPEEDGUN_FILTER=^bmWrite --filter=^bmRead` runs only `bmRead`, which is FR-037 end to end; `base_cli_matrix` reads 26 of 26; `cli_shape` and the time-source gate stay clean. FR-081 record: the two-cpu reading is unverified on this fleet - every CI runner reports four cpus and no job runs a two-cpu host - so the rule is held by construction (no test names a cpu count, counts come from `sched_getaffinity`) rather than by a run, which is what D-29 states.
- [x] T025: state the environment value rules, writes
  test/source/harness_cli_sources_test.cpp, proves ctest fails on the
  semicolon split of a repeatable into items each read under the
  command-line rule, on the four flag words `1` , `0` , `true` and
  `false` with the refusal line
  `SPEEDGUN_NO_PERCENTILES: expected 1, 0, true or false, got '<value>'`
  , on the ignored unknown `SPEEDGUN_*` name that answers no line and no
  status change, and on the fault that opens `SPEEDGUN_<NAME>: ` and
  carries the reader's own line naming the variable and the value
  (FR-024, FR-025, FR-026, FR-027, SC-003).

  Verified 2026-10-22: the semicolon split, the four flag words with the refusal line, the ignored unknown name and the composed reader fault all stand as cases; same link gap as T022.
- [x] T026: read the environment values through the readers the command
  line already uses, writes source/harness/options.cpp, proves those
  cases pass through the shared readers (FR-024, FR-025, FR-026,
  FR-027).

  Verified 2026-10-22: every flagged row reaches the command line's own reader through one switch on `row.value`, the refusal line opens with the variable, and the four flag-word cases are live (inverting the reader answers exactly those four).
- [x] T027: state the interleave and percentile rules, writes
  test/source/harness_cli_sources_test.cpp, proves ctest fails on
  `SPEEDGUN_INTERLEAVE` turning the option on with the seed its reader
  answers and `SPEEDGUN_INTERLEAVE=0` meaning given with seed 0, and on
  the percentile pair checked over the environment rows alone ahead of
  the precedence rule, answering
  `SPEEDGUN_PERCENTILES and SPEEDGUN_NO_PERCENTILES cannot both stand`
  at status 2 (FR-022, FR-023).

  Verified 2026-10-22: the interleave seed case and the percentile pair case over the environment rows alone stand; same link gap as T022.
- [x] T028: bind the environment interleave and pair check, the pair
  read over the environment rows alone so a command-line half does not
  hide it while the command-line pair keeps the master answer where the
  last word wins and the status stays 0, writes
  source/harness/options.cpp, proves those cases pass (FR-022, FR-023).

  Verified 2026-10-22: the seed cases and the environment-only pair check pass. Polarity correction made in this pass: the `no-percentiles` row is the negating flag, so `SPEEDGUN_NO_PERCENTILES=1` answers `showPercentiles = false` and `=0` answers true, matching the scan's `case 'P'`; the sources test had pinned the inverse and was corrected to the option's meaning (FR-025).
- [x] T029: turn the environment off for one run, any other word
  answering `--env: expected 'off', got '<word>'` at status 2, writes
  source/harness/options.cpp and source/harness/cli.cpp, proves
  `--env=off` stands on the command line alone and clears the rows,
  with `environmentRows()` never called and no environment use disclosed
  (FR-028).

  Verified 2026-10-23: the `env` scan arm stands at `source/harness/options.cpp:759` and sets `RunOptions::environmentOff` (`detail/internal.hpp:495`) for the one word `off`, any other word answering `--env: expected 'off', got '<word>'`. The entry point asks the environment only through that flag: `source/harness/cli.cpp:297` reads `detail::environmentRows()` when the flag is clear and hands the gather an empty row list when it stands. Measured through `build-ulw/example/docs_command_line` from a scratch directory: `SPEEDGUN_FILTER=bmWrite --env=off --filter=bmRead` answers status 0 with the one `bmRead` row and no disclosure line, `SPEEDGUN_FILTER=bmRead` alone answers the one row it names, and `--env=on` answers status 2 with that line on standard error and zero lines on standard output. `harness_cli_sources_test` and `base_cli_matrix` (26 of 26) pass in `build-ulw` (FR-028).

## Phase 4: the option file, precedence and the disclosure

- [x] T030: move the text reader out of the anonymous namespace of
  `source/harness/json_load.cpp:26` to `:48` , writes
  source/harness/options.cpp and source/harness/detail/internal.hpp,
  proves `detail::readTextFile` stands in the internal header and ctest
  -R harness_json_load_test passes (FR-030, FR-084).

  Verified 2026-10-22: `detail::readTextFile` is declared at `detail/internal.hpp:680` and defined at `options.cpp:908`, the archive answers it as `T`, and `harness_json_load_test` passes; `json_load.cpp` keeps its local reader until T031.
- [x] T031: point the reader at the shared code, writes
  source/harness/json_load.cpp, proves the local `readFile` is gone,
  both call sites call `detail::readTextFile` and harness_json_load_test
  passes (FR-030).

  Verified 2026-10-22: `readFile` is gone from `json_load.cpp` (zero matches), both call sites at `:1134` and `:1169` call `detail::readTextFile`, the `<cerrno>` and `<fstream>` includes left with it, `harness_json_load_test`, `harness_json_schema_test` and `harness_compare_test` pass in `build-t031`, and clang-format-18 is clean.
- [x] T032: state the option file grammar, one option per line in long
  spelling without the leading dashes as `name` or `name=value` , a
  first non-space `#` marking a comment and a blank line setting nothing
  while both still hold a number counted from 1, writes
  test/source/harness_cli_sources_test.cpp, proves ctest fails on the
  four refused names `options` , `env` , `help` and `version` with
  `<path>:<line>: the option '<name>' cannot stand in an option file` ,
  on an unknown name answering `<path>:<line>: unknown option '<name>'`
  , on the second `--options` answering `--options cannot stand twice`
  with no path repeated, and on the unreadable file answering
  `--options: the file '<path>' cannot be opened: <reason>` , each one
  line on standard error at status 2 (FR-029, FR-031, FR-032, FR-033,
  FR-034, SC-003).

  Verified 2026-10-22: the grammar cases stand in `test/source/harness_cli_sources_test.cpp` (1007 lines); the file builds with zero `error:` lines and the run answers `2 checks failed`, both naming the unreadable-file case whose read stands at the entry point and lands with T035. The fifteen environment and scan cases stayed green.
- [x] T033: read the option file as data, the gather splitting the text
  into lines and opening no file, a repeatable accumulating within one
  file in line order and reaching the flag reader of FR-025 when the
  line is written `name=value` , writes source/harness/options.cpp,
  proves those cases pass through the shared text reader (FR-025,
  FR-029, FR-031, FR-032, FR-033, FR-034, FR-035).

  Verified 2026-10-22: `source/harness/options.cpp` runs 1382 lines with the gather splitting the text into lines, opening no file, holding 1-based numbers for comment and blank lines, refusing the four names and naming an unknown one, accumulating a repeatable in line order and reaching the flag reader; the eight option-file cases moved from red to green. `--options=<path>` parses but the entry point opens no file until T035, so a run with a written file behaves as if it were empty - measured with a `filter = ^bmRead` file against `docs_command_line`, which ran both families. `base_cli_matrix` reads 26 of 26 and `harness_cli_table_test` fails one check (the guide row T057 owns).
- [x] T034: state the precedence rules, writes
  test/source/harness_cli_sources_test.cpp, proves ctest fails on the
  four-source order of command line, option file, environment and master
  default, and that a repeatable list is settled whole by the higher
  source, the comparison rule list included, its lower rules read before
  they drop
  (FR-036, FR-037, FR-038, FR-041).

  Verified 2026-10-23: the precedence cases in `test/source/harness_cli_sources_test.cpp` pass in `build-ulw` (CTest #67). The cases cover the command line over the option file, the option file over the environment, the environment over the built-in default, and a repeatable option replaced whole by the higher source (C1, FR-036 to FR-041). A rule-list case written against the older reading is stale: D-23 and FR-038 as they stand on 2026-10-23 replace a lower source's rule list with the higher source's list, so a later change owns that test half (FR-038, FR-063).
- [x] T035: gather the four sources in `detail::gatherOptionSources` ,
  which touches no file system and no process state and answers
  `ResolvedOptions` or one fault line, writes
  source/harness/options.cpp, proves those cases pass with every repeatable
  list settled whole by the higher source, the rule list included and its
  lower rules read before they drop (FR-036, FR-037, FR-038,
  FR-041).

  Verified 2026-10-23: `detail::gatherOptionSources` stands at `source/harness/options.cpp:1538` and settles the four sources in the order C1 fixes, the command line first so a seed or a count of a lower source answers nothing there. The precedence cases of T034 pass with it, and `--threads=2` under `SPEEDGUN_THREADS=8` answers the command-line count alone through `example/docs_command_line` (FR-036 to FR-041).
- [x] T036: state the disclosure, writes
  test/source/harness_json_report_test.cpp, proves ctest fails on the
  console line opening
  `options from a file or the environment: ` and joining entries in
  table order with `; ` and ending `interleave=<seed>` , on the run
  object standing between `speedgun` and `benchmarks` with the keys
  `option` , `source` and `value` and the `interleave` key present only
  when the option was given, and on the stamped minor (FR-039, FR-040,
  FR-043, FR-086, SC-011). Stating note: a run-object case calls
  `detail::writeReport` with the `RunDisclosure` parameter T037 adds, and
  that call compiles once the parameter stands, so T036 states the
  disclosure over the surface that stands today (the console line, the
  stamped minor, and the absence of the `run` key in a written document)
  and T037 states the run-object cases in the same file beside the writer
  it lands with.
- [x] T037: write the run object with the vendored
  `simdjson::builder::string_builder` helpers, `detail::writeReport`
  taking a `RunDisclosure` beside the `ReportData` it already takes and
  `ReportData` gaining no field, writes source/harness/json_report.cpp
  , source/harness/detail/json_report.hpp and the run-object cases in
  test/source/harness_json_report_test.cpp, proves those cases pass
  with the vendored builder and the disclosure parameter (FR-039,
  FR-040, FR-084, FR-086).

  Verified 2026-10-23: `RunDisclosure` stands at
  `source/harness/detail/json_report.hpp:198` carrying `uses` and the
  optional seed, `detail::writeReport` takes it as a defaulted third
  parameter at `:229` with the `\pre` and `\post` lines of D-31 beside the
  pair the base already carried, and `writeRunObject` at
  `source/harness/json_report.cpp:591` writes the `run` object between
  `speedgun` and `benchmarks` through the vendored builder, the `Builder`
  alias of `simdjson::builder::string_builder` standing at `:27`.
  `ReportData` gained no field, so `git diff --name-only origin/master --
  include/speedgun-ng` prints nothing and the harness `SOVERSION` stays 10.
  `ctest --test-dir build-ulw -R 'harness_json_report_test|report_shape'`
  passes 6 of 6 with zero failed, which carries the run-object cases the
  task states beside the writer.
- [x] T038: place the key in the schema, writes
  docs/schema/report-v1.json, proves `run` joins `x-order` between
  `speedgun` and `benchmarks` and joins `properties` , `required` keeps
  the three base keys, `additionalProperties` stays true, and ctest -R
  report_shape passes over a written document (FR-043, SC-011).

## Phase 5: thread counts from the command line

- [x] T039: state the thread list reader, writes
  test/source/harness_cli_threads_test.cpp, proves ctest fails on a
  single count, on the `min:max` expansion into the counts of
  `detail::buildThreadCounts` doubling from `min` while at most `max` ,
  and on the refusal of a count below one or a `min` above `max` with
  `--threads: expected counts and min:max ranges, got '<text>'` at
  status 2, the run reaching ctest at T040 where the seam this file
  declares, `detail::readThreadList` , first stands (FR-044, FR-045,
  FR-046, FR-050, SC-003).

  Verified 2026-10-22: the six cases stand in the file, it compiles clean under the repo warning set, and linked against the fresh harness archive it runs `ok` with exit 0 - the seam `detail::readThreadList` already stands from the environment work, so the cases pass ahead of registration. The target still reaches ctest only when T040 registers it.
- [x] T040: read the thread list as a comma-separated sequence of items,
  the same reader answering the `SPEEDGUN_THREADS` items, and register
  `harness_cli_threads_test` in the pass that defines the reader, writes
  source/harness/options.cpp and test/CMakeLists.txt, proves those cases
  pass through detail::buildThreadCounts for the range form and that the
  target reaches ctest from this pass: `detail::readThreadList` is the
  seam the file declares and `detail::buildThreadCounts` already stands
  at `source/harness/registry.cpp:574` (FR-044, FR-045, FR-046).

  Verified 2026-10-22: the target is registered at `test/CMakeLists.txt:693` to `:697`, builds with zero `error:` lines and passes as a ctest case; `base_cli_matrix` still passes in the same tree. The reader itself was measured green under T039.
- [x] T041: write the counts into the registry in one loop over
  `detail::registry()` standing before `detail::expandRegistry` at
  `source/harness/cli.cpp:570` , writes source/harness/cli.cpp, proves
  harness_cli_threads_test passes with `RegistryEntry::threadCounts` set
  for every entry before the expansion and the family.cpp names
  unchanged, no `/threads:` segment at one thread and `/threads:<n>`
  above one (FR-044, FR-048, FR-050, SC-004).

  Verified 2026-10-22: the loop over `registry()` stands at `source/harness/cli.cpp:320` to `:332`, ahead of `detail::expandRegistry`, and the printed instance names above are its reading: the counts reach every family record before the expansion, the base names are unchanged at the default, and the `/threads:` segment appears only above one count. `example/family.cpp` needed no change.
- [x] T042: state the over-affinity case, writes
  test/source/harness_cli_threads_test.cpp, proves ctest fails on a
  count above the affinity count running and disclosing itself in the
  console line, with the cpu count read through `sched_getaffinity` so
  that no test names a cpu count (FR-047, FR-081, SC-004).

  Verified 2026-10-22: `aCountAboveTheAffinityCountRunsAndDisclosesItself` stands in `test/source/harness_cli_threads_test.cpp`, the count comes from `sched_getaffinity` (33 contexts asked on a 32-cpu machine) and no cpu-count literal stands in the file. The run answers `1 checks failed`, naming only `the console discloses the over-affinity count in a line naming the option and the count (FR-047)`; the two run halves pass on the T064 scan arm, and liveness was shown by flipping the status expectation (2 failures) and by swapping the disclosure needle (the check then passes on the noise warning, which is why the predicate excludes the benchmark row).
- [x] T043: disclose the over-affinity count and state the console line
  cases, writes source/harness/cli.cpp and
  test/source/harness_cli_sources_test.cpp, proves those cases pass and
  the fit refusal line of `detail::placementFitStatus` stays as the base
  prints it, naming the preset, the place count and the requested count
  at status 2 before any run, and proves the console line text, its
  place after the version line of `detail::printContext` and before the
  first benchmark row, its `interleave=<seed>` tail and its absence for
  a command-line-only run (FR-040, FR-042, FR-047, FR-049).

  Verified 2026-10-23: `affinityCpuCount()` stands at `source/harness/cli.cpp:66` as `sched_getaffinity` with `CPU_COUNT` and a `sysconf` fallback, the over-affinity line prints at `:464` before the first row, and the disclosure line builds at `:95` behind `detail::printContext`. Measured readings: `--threads=33 --iterations=1` prints `--threads=33 stands above the 32 cpus the affinity allows` as the second line and then both families at `/threads:33`; `--threads=4` prints no such line; `SPEEDGUN_REPETITIONS=2` prints `options from a file or the environment: --repetitions=2`; a plain run prints no disclosure line. `harness_cli_threads_test` passes (CTest #68) and `harness_cli_sources_test` carries the two new cases (FR-040, FR-042, FR-047).

## Phase 6: interleaved execution

- [x] T044: add `detail::shuffledOrder` , a fill of `0 .. count - 1` and
  the walk from `count - 1` down to 1 that swaps the entry at `i` with
  the entry at `engine() % (i + 1)` from a `std::mt19937_64` seeded with
  the seed, and register `harness_cli_interleave_test` in the pass that
  defines that seam, writes source/harness/options.cpp and
  test/CMakeLists.txt, proves the pinned literals of T005 pass: seed 7
  answers `4 0 2 6 10 9 1 5 11 8 7 3` and seed 8 answers
  `7 0 6 10 8 2 11 5 9 4 3 1` for `count = 12` , and proves the target
  reaches ctest from this pass, the test file declaring
  `detail::shuffledOrder` (FR-051, FR-053).

  Verified 2026-10-23: `detail::shuffledOrder` stands at `source/harness/options.cpp:1013` as an `iota` fill, a `std::mt19937_64` engine seeded by the caller and a Fisher-Yates walk from the last slot down, the whole of the randomness being one `engine() % index` draw per slot. `std::shuffle` is unusable here because the standard pins neither its distribution bounds nor its drawing order, and the doc comment carries the two rows `contracts/threads-and-interleave.md` fixes. `test/CMakeLists.txt` registers `harness_cli_interleave_test` (CTest #69) and it passes: the two pinned permutations, the permutation postcondition at counts 1, 2, 12 and 40, and the two seeds answering different orders (FR-051, FR-053, SC-005).
- [x] T045: state the seed forms, writes
  test/source/harness_cli_interleave_test.cpp, proves ctest fails on the
  bare `--interleave` answering seed 0, `--interleave 7` answering seed
  0 with `7` left a separate argument, and each of `-1` , `0x10` , `7x`
  , `true` and an over-limit digit string answering
  `--interleave: the seed '<text>' is not an unsigned decimal value` at
  status 2 (FR-052, FR-054, SC-003).
- [x] T046: give the `interleave` row the optional argument kind and add
  `detail::readSeed` , writes source/harness/options.cpp and
  source/harness/cli.cpp, proves those cases pass through the shared
  seed reader, with `SPEEDGUN_INTERLEAVE=0` meaning given with seed 0
  (FR-052, FR-054).

  Verified 2026-10-23: the `interleave` row carries `OptionArgument::OPTIONAL` (`detail/options.hpp:31`), the scan arm stands at `source/harness/options.cpp:785` with the bare form taking seed 0 and an attached word going through `detail::readSeed`, and the gather settles `resolved.interleave = scan.interleave` ahead of every lower source. `harness_cli_interleave_test` passes the four seed-form cases, `test/data/base_cli_matrix.txt:20` moved to status 0 for `--interleave=7` and the matrix reads 26 of 26, and `--interleave=nonsense` answers `--interleave: the seed 'nonsense' is not an unsigned decimal value` at status 2 (FR-052, FR-054).
- [x] T047: state the held rows, writes
  test/source/harness_cli_interleave_test.cpp, proves ctest fails on a
  shuffled run that prints nothing inside the loop and prints every held
  row in registration order after it, each family's fit line after the
  last held row of that family, the passes and repetitions of one
  instance staying together, and the master print order standing when
  the option is absent (FR-055, FR-056, FR-059).

  Verified 2026-10-22: the three held-row cases stand in `test/source/harness_cli_interleave_test.cpp` (736 lines, unregistered until T044) over the `bmIlAlpha`, `bmIlBeta`, `bmIlRaises`, `bmIlQuad` and `bmIlGamma` suite, with the pinned order strings `bmIlAlpha run; bmIlBeta run; bmIlQuad/2 run; bmIlQuad/4 run; bmIlGamma run` and the fit line after the last row of its family. Compiled clean against the fresh `speedgun-ng_harness` archive and linked with a scratch stub of `detail::shuffledOrder` reproducing the T005 literals: the binary answers `12 checks failed`, the seven T045 seed reds plus these three and the two T049 interrupt reds, and the FR-059 master-order case stands green as the regression guard. No sleep, no clock read, no cpu-count literal.
- [x] T048: compute the walk before the run loop and print the held rows
  after it, writes source/harness/cli.cpp, proves those cases pass with
  `runner.run(*entry)` and its one interrupt-flag reading still inside
  the gate range, and `bash test/cli_shape.sh` green (FR-055, FR-056,
  FR-058, FR-059, FR-060, SC-005, SC-012).

  Verified 2026-10-23: the walk settles at `source/harness/cli.cpp:490` before the first measurement, an identity `iota` for a plain run and `detail::shuffledOrder` for a seeded one, `struct HeldRow` and the `held` vector stand at `:500`, and the print pass walks the holds in registration order after the loop. Measured through `example/docs_command_line --interleave=7 --iterations=1`: both families answer one row, `bmRead` prints before `bmWrite` in registration order while the run order differs, and the fit line stands once per family. `bash test/cli_shape.sh` answers 0 (FR-055, FR-056, FR-057, FR-059).
- [x] T049: state the interrupt case, writes
  test/source/harness_cli_interleave_test.cpp, proves ctest fails on a
  body that calls `std::raise(SIGINT)` at a named instance and answers
  the rows that ran in registration order with the master interrupt
  status, with no sleep and no clock read (FR-057, FR-081).

  Verified 2026-10-22: `an interrupted shuffled run prints the rows that ran in registration order` and `answers the master interrupt status` stand in the same file, seed 8 whose walk `3 0 2 1` puts the `std::raise(SIGINT)` body third so the second instance never runs. The linked binary fails both: the rows print nothing and the status reads 2 against the expected 1. The expectation is derived from `detail::shuffledOrder` itself rather than restating the permutation, and liveness was shown in a scratch copy where one planted row turns exactly the master-order case red (12 to 13).
- [x] T050: pin the loop shape over the new walk, writes
  test/source/harness_cli_interleave_test.cpp, proves
  `bash test/cli_shape.sh` reports its planted probes and stays green
  with the single flag reading inside the range at
  `test/cli_shape.sh:32` and `:39` (FR-060, FR-083, SC-012).

  Verified 2026-10-23: `theLoopShapeStandsOverTheWalk` in `test/source/harness_cli_interleave_test.cpp` pins the run line, the `: walk)` header, the range end, one `detail::interruptFlag().load()` inside the range, one `bool interrupted =` and zero flag reads behind the loop, and it compares `anyFailed` with `anyInterrupted` in the postcondition. Liveness rests on two planted copies of `cli.cpp` under a scratch root: a second read in the print pass answers exactly `the held-row print pass reaches no interrupt flag (FR-056, FR-060)`, a duplicated read in the range answers exactly `the loop range reads the interrupt flag once per pass (FR-036, FR-060)`, and the real tree answers `ok`. `bash test/loop_shape.sh build-ulw` answers 0 (FR-060, SC-012).

## Phase 7: the rules file

- [x] T051: state the rules file, writes
  test/source/harness_compare_cli_test.cpp, proves ctest fails on three
  rules read in line order over the fit fixture pair, a comment and a
  blank line that hold line numbers, a refused line answering
  `<path>:<line>: ` plus the parser's own text, and an unreadable path
  answering
  `--compare-rules: the file '<path>' cannot be opened: <reason>` , and
  the row order within the source that settles, beside the lower rule
  read for its fault before it drops (FR-061, FR-062,
  FR-063, FR-064, FR-065, SC-003, SC-006).
- [x] T052: read the rules file through `detail::readTextFile` and call
  `sg::parseCompareRule` once per line, writes
  source/harness/options.cpp and source/harness/cli.cpp, proves those
  cases pass with the specs/026 grammar unchanged and line numbers
  counted from 1 over the raw lines (FR-061, FR-062, FR-063).

  Verified 2026-10-23: `detail::readCompareRulesFile` reads the path through `detail::readTextFile` and hands each line to `parseCompareRule`, the fault naming `<path>:<line>` with the line counted from 1, and the `compare-rules` scan arm refuses a second occurrence. Measured through `build-ulw/example/docs_command_line` from a scratch directory: a refused path answers status 2 with zero lines on standard output and `--compare-rules: the file '/proc/nope/rules.txt' cannot be opened: No such file or directory` on standard error; a file whose second line is `not-a-rule` answers status 2 with `rules_ok.txt:2: the value name of the rule 'not-a-rule' is not readable` and the line number counted from 1; a line naming a direction `upper` answers `rules_two.txt:2: the direction of the rule 'time@max:upper:10%' is not readable: 'upper' is neither lower nor higher`. The line-number cases pass in `harness_cli_sources_test` and `harness_compare_cli_test` stays green (FR-061, FR-062, FR-063, FR-064).
- [x] T053: gather the rule list of one source in the order of spec D-23,
  its `--compare-rule` options in the order they stand then the file that
  source's `--compare-rules` names in line order, the higher source
  settling the list and a lower rule read before it drops,
  writes source/harness/compare.cpp, proves `harness_compare_cli_test`
  passes with the printed rows and the verdict following that order
  (FR-064, FR-065, SC-006).

  Verified 2026-10-23: `detail::gatherOptionSources` builds the one rule list at `source/harness/options.cpp:1558` in the order FR-063 and spec D-23 state for one source: the `--compare-rule` words of that source, then the file its `--compare-rules` word names in line order. At that head a rule of a lower source still reached the list beside the higher source's rules. The reading of FR-038 and FR-063 taken on 2026-10-23 replaces that whole list with the higher source's, so a later change owns the gather half and any case that pins the older cross-source order. The `compare-rule` row carries no environment flag, the list D-3 fixes, so a variable names no rule and FR-026 ignores the name. Measured through `build-ulw/example/docs_command_line --compare=r_base.json,r_cont.json --compare-rules=rules_good.txt --compare-rule=time@min:lower:5%`: three rows in that order, `time@min`, `time@median`, `time@max`, at status 0, while `SPEEDGUN_COMPARE_RULE=time@median:upper:200%` adds no fourth row. `harness_compare_cli_test` passes in `build-ulw` (CTest #64) (FR-063, FR-064, SC-006).
- [x] T054: show the rules option in help, writes
  test/source/harness_cli_table_test.cpp, proves the `compare-rules` row
  appears in the recorded `--help` output built from
  `detail::optionRows()` and in the guide reference comparison (FR-061,
  FR-074).

  Verified 2026-10-23: the pair stands in
  `test/source/harness_cli_table_test.cpp` around `:1264`, where
  `liveRow("compare-rules")` answers a row of the live table read through
  `optionRows()` and `lineWith(run.out, "  --compare-rules")` reads the recorded
  answer of `--help`, the guide side being read through `guidePageText`,
  `cellsOf` and `pageRowOf`. `harness_cli_table_test` prints `ok` with exit 0,
  CTest #66 of `build-ulw`, where the same two checks stood red with
  `9 checks failed` when the case landed on 2026-10-22.

## Phase 8: docs, example, gates, version and ABI

- [x] T055: grow the main page, writes README.md, proves
  `python3 tools/docs/example_drift.py .` reports only the page cap
  finding, with the section at 20 lines against the cap of 25 and rows
  for `--options` , `--env` , `--threads` , `--interleave` ,
  `--compare-rules` , `--help` and `--version` (FR-066, FR-067, FR-068,
  SC-009).

  Verified 2026-10-22: `README.md` carries 27 option rows matching `detail::optionRows()` and the command-line section spans 20 lines against the 25 cap; `python3 tools/docs/example_drift.py .` exits 0 with no findings.
- [x] T056: raise the page cap from the measured 773 to the measured 786
  and point `cli_options()` at `source/harness/options.cpp` , writes
  tools/docs/example_drift.py, proves
  `python3 tools/docs/example_drift.py .` exits 0 with the arithmetic of
  the 13 added lines in the comment above the cap and the row regex
  still matching the moved table (FR-068, FR-075).

  Verified 2026-10-22: `PAGE_LINE_CAP` stands at the measured 786 with the arithmetic in the comment above it, and `cli_options()` already read `source/harness/options.cpp` from the earlier table move, so that half was a recorded no-op at `tools/docs/example_drift.py:466`.
- [x] T057: write the option reference under the existing guide heading
  for the command line of an example, writes docs/pages/harness.md,
  proves `python3 tools/docs/example_drift.py .` exits 0 with one row
  per table row in table order carrying the name, the argument kind, the
  environment variable where the row has one and the help text, the
  precedence sentence of spec D-1, and the fenced block equal byte for
  byte to the shell region (FR-069, FR-070, FR-071, FR-074, SC-008).

  Verified 2026-10-22: `docs/pages/harness.md` carries the 27-row reference in table order with the argument kind, the environment name on the twelve flagged rows and the help text of each row, plus the precedence paragraph of spec D-1 and the fenced block equal to the `options` region of `example/docs/command_line.sh`. `python3 tools/docs/example_drift.py .` exits 0 with no findings, the page prose reads 1165 units with 0 findings, and `harness_cli_table_test` now prints `ok` with exit 0: the standing `compare-rules` guide-row red that T054 left open is closed by this page. Liveness was shown by deleting the `compare-rules` row in a scratch tree, which answers exactly one failure naming that case. Coordination point: T058 grows the shell `options` region, so the fenced block has to grow with it in the same pass.
- [x] T058: extend the example, writes example/docs/command_line.sh and
  example/docs/command_line.cpp, proves `bash test/example_run.sh` over
  the build tree passes with the options region holding the option file,
  the environment, the thread list, the interleaving and the rules file,
  and the snippet region unchanged and still compiling (FR-072, FR-073,
  SC-009).

  Verified 2026-10-22: the `options` region of `example/docs/command_line.sh` runs 14 lines and carries the option file, the environment form, the thread list in both the item and `min:max` forms, `--interleave=7` and `--compare-rules=<path>` paired with `--compare` over the specs/026 fixtures; the fenced block of `docs/pages/harness.md` mirrors it byte for byte, which the drift gate reads (`python3 tools/docs/example_drift.py .` exits 0 with no findings), `bash -n` parses the script and `bash test/example_run.sh build-t058` exits 0 with `PASS: docs_command_line (2 instances)`. `example/docs/command_line.cpp` is untouched, so the snippet region still compiles and `snippet_compile` passes. No page cap moved: `check_sections` runs on `README.md` only and `docs/pages/harness.md` carries no length gate.
- [x] T059: compare the page against the table, writes
  test/source/harness_cli_table_test.cpp, proves the two-way case passes
  with every table row appearing on the page at the same argument kind,
  environment name and help text and every page row naming a table row,
  the page read from a source directory passed as an argument (FR-074,
  SC-008).

  Verified 2026-10-23: `thePageAndTheTableAgreeInBothDirections` stands at `test/source/harness_cli_table_test.cpp:1318` and walks both ways over the guide page the CTest argument names: every table row on the page at the same argument kind, the same environment name and the same help text, every page row naming a table row, 27 rows each way and 12 of them naming an environment variable. `harness_cli_table_test` answers `ok` (CTest #66). Liveness rests on three planted copies of `docs/pages/harness.md` under a scratch root: `interleave` claiming `required` answers the argument-kind finding, `list` renamed to `listing` answers both directions (the missing page row and the orphan page row), and `json` claiming `SPEEDGUN_JSON` answers the environment-name finding plus the 12-row count (FR-069, FR-070, FR-074, SC-008).
- [x] T060: take the version at rebase, writes CMakeLists.txt, proves
  the project version equals the next free minor on master, that
  `git diff --name-only origin/master -- include/speedgun-ng` prints
  nothing, and that the harness `SOVERSION` stays 10 at
  `CMakeLists.txt:261` while the archive `SOVERSION` stays 3 at `:53`
  (FR-076, FR-077, FR-078, FR-086).

  Verified 2026-10-22, read again 2026-10-23: `origin/master` stood at
  `bd10d76` with `VERSION 0.16.0`. The fleet now carries `0.17.0` on
  PR #46 and `0.19.0` on PR #47, so this branch takes `0.18.0` and
  `CMakeLists.txt:9` carries it. Read through
  `build-fix-oracle/example/docs_command_line`, `--version` answers
  `speedgun-ng 0.18.0` at status 0 with nothing on standard error.
  `harness_cli_table_test` takes its version expectation from the
  `project()` line rather than from a literal, and no test under
  `test/source/` pins the build version as a literal.
  `git diff --name-only origin/master -- include/speedgun-ng` prints
  nothing, so the harness `SOVERSION` stays at the value `master` carries,
  10 at each edge, and the archive `SOVERSION` stays 3 at `:53`. The
  `project()` comment above the value names
  `specs/028-full-command-line` FR-078 for the minor this lane took.
- [x] T061: audit the test list and the coverage reading, writes no
  file, proves `ctest --test-dir build` lists `harness_cli_table_test` ,
  `harness_cli_sources_test` , `harness_cli_threads_test` and
  `harness_cli_interleave_test` , each registered by the task that first
  makes it linkable, and every one of them passes, and proves the
  coverage chain `cmake --preset=ci-coverage` ,
  `cmake --build build/coverage` ,
  `ctest --test-dir build/coverage --output-on-failure` and
  `cmake --build build/coverage --target coverage` prints 100 percent
  lines and 100 percent branches with no exclusion marker under
  `source/harness/` (FR-080, SC-010, SC-014).

  Verified 2026-10-23: `ctest --test-dir build-028cov -N` lists #66
  `harness_cli_table_test`, #67 `harness_cli_sources_test`, #68
  `harness_cli_threads_test` and #69 `harness_cli_interleave_test` of
  `Total Tests: 120`, the four pairs standing at `test/CMakeLists.txt:664` to
  `:693`, each registered by the task that first makes it linkable (T003, T010,
  T040, T044), and the run answers `100% tests passed out of 120`. The chain
  over counters deleted first, `cmake --build build-028cov` then
  `ctest --test-dir build-028cov -j 4` then
  `cmake --build build-028cov --target coverage`, which captures, extracts with
  `--filter exception,branch,brace` and `--omit-lines` over the contract macros,
  and ends with `bash tools/dbc/coverage_gate.sh coverage.info`, prints
  `lines.......: 100.0% (8938 of 8938 lines)` and
  `branches....: 100.0% (4369 of 4369 branches)` with the gate exiting 0. The
  tree stands in `build-028cov` because the preset writes `build/coverage`, the
  directory the sibling sessions of this checkout own. The grep for
  `LCOV_EXCL` over `source/harness/` answers 0 files and `cmake/coverage.cmake`
  carries no diff against the base commit. Eleven branch sides opened with one
  case, `oneWordTwiceKeepsTheLastWordOfOneSource`, which spells eleven rows
  twice inside one option file, the shape the file pass reports to
  `applySourceValue` as `sameSource` (FR-040). The last two sides needed a
  change to the code: the guard of the `case 'p'` arm lost a second operand that
  answers what the flag test beside it answers, and the `answered` field of
  `Runner::FitSamples` fed one term that no walk can reach, `fitForFamily` being
  named once per family by each print path while `expandRegistry` keeps the
  instances of one family in one block.
- [x] T062: carry the contract macros and run the rest of the gate set,
  writes source/harness/detail/options.hpp and
  source/harness/options.cpp, proves
  `cmake --build build --target dbc-gate` passes with
  `detail::scanCommandLine` , `detail::gatherOptionSources` ,
  `detail::readThreadList` , `detail::readSeed` and
  `detail::shuffledOrder` each carrying `SG_REQUIRE` for its
  precondition, `SG_ENSURE` for its postcondition and the line-start
  `/// \pre` and `/// \post` lines of the form D-31 states, and proves
  the rest of the SC-014 set green: `cmake --preset=ci-ubuntu` and
  `cmake --build build` , the configure carrying clang-tidy and
  cppcheck, `cmake --preset=ci-rocky` and `cmake --build build` ,
  `cmake --preset=ci-sanitize` with `cmake --build build/sanitize` and
  `ctest --test-dir build/sanitize` , `cmake --preset=ci-tsan` with
  `cmake --build build/tsan` and `ctest --test-dir build/tsan` ,
  `cmake --build build --target format-check` ,
  `bash test/loop_shape.sh build` , `bash test/time_source_gate.sh .` ,
  `cmake --build build --target spell-check` and
  `cmake --build build --target prose-lint` (FR-083, FR-085, SC-014).

  Verified 2026-10-23: the five seams carry the pair in the form the gate reads. `SG_REQUIRE` stands at `source/harness/options.cpp:594`, `:931`, `:985`, `:1021` and `:1573`; `SG_ENSURE` stands at `:853`, `:938`, `:993`, `:1034` and `:1664`; the two `SG_ASSERT` sites stand at `:368` and `:1426`, the last three re-read on 2026-10-23 after a fix that grew the file by eighteen lines; `detail/options.hpp` carries five `\pre` lines and five `\post` lines over the five declarations. `cmake --build build-ulw --target dbc-gate` answers exit 0, and so do `format-check`, `spell-check` and the prose gate over 554 sources and 94891 units. The `prose-lint` target itself answers `0 sources, 0 units examined` because its range is degenerate while nothing is committed (base equals head), so the reading of the same gate is the tree mode with the 23 new files entered by `git add -N` and left by `git reset`, `bash test/cli_shape.sh`, `bash test/loop_shape.sh build-ulw` and `bash test/time_source_gate.sh .`. Presets: `ci-ubuntu` configures with clang-tidy and cppcheck wired in and builds at exit 0; `ci-rocky` configures, builds and answers 120 of 120 in `build-028rocky`; `ci-tsan` configures, builds and answers 117 of 117, three targets standing down under the pre-existing `fsanitize=thread` guards at `test/CMakeLists.txt:255`, `:566` and `:879`. `ci-sanitize` stays an explicit BLOCKER on this machine: its build answers 2 with ten `-Werror=maybe-uninitialized` errors per object at `/usr/include/c++/16/bits/std_function.h:395`, raised from `cli.cpp` and `compare.cpp`, and the copies of both files at `HEAD` answer the same ten errors on the same first line under that tree's own flags, so the block belongs to GCC 16.2.1 and to the `std::regex` use specs/026 landed and CI's GCC 15 stays the authority for the gate (FR-083, FR-085).

## Follow-up found while implementing Phase 5

- [x] T064: carry the settled thread counts, writes source/harness/options.cpp, source/harness/detail/internal.hpp and test/data/base_cli_matrix.txt, proves `--threads=<text>` reaches `detail::readThreadList` through a `case kThreadsOptionValue:` scan arm instead of the unknown-option line, `RunOptions` carries `std::vector<int> threadCounts` with an empty list meaning no source named the option, the gather fills it from the command line, the option file and the environment instead of discarding it, and the matrix row that pins `--threads=2` to status 2 is re-marked to status 0 (FR-044, FR-045, FR-046, FR-050, SC-004).

  Verified 2026-10-22: `RunOptions` carries `std::vector<int> threadCounts` (`source/harness/detail/internal.hpp:482`), the scan arm `case kThreadsOptionValue:` stands at `source/harness/options.cpp:745` and the gather arm at `:1158` fills the field instead of discarding it. Measured through `example/docs_command_line` from a scratch directory: `--threads=2` answers `/threads:2` on both families, `--threads=1` answers no segment, `--threads=1,4` answers both shapes, `--threads=2:8` answers `/threads:2`, `/threads:4` and `/threads:8`, `SPEEDGUN_THREADS=2` matches the command-line form, `SPEEDGUN_THREADS=8 --threads=2` answers only `/threads:2`, and `--threads=0` answers `--threads: expected counts and min:max ranges, got '0'` at status 2 with empty standard output. `test/data/base_cli_matrix.txt:19` moved to status 0 and the matrix reads 26 of 26; `cli_shape` stays 0.

## Follow-up found while implementing Phase 4

- [x] T065: open the option file the command line names, writes source/harness/cli.cpp, proves the entry point reads the path of `--options=<path>` through `detail::readTextFile` and hands the text to `detail::gatherOptionSources` instead of `std::nullopt`, that a written file sets its options end to end, and that a path the system refuses answers `--options: the file '<path>' cannot be opened: <reason>` at status 2 with one line on standard error and nothing on standard output (FR-034, FR-029, FR-030).

  Verified 2026-10-23: the `options` scan arm stands at `source/harness/options.cpp:748`, refuses a second occurrence with `--options cannot stand twice`, and the entry point reads the path through `detail::readTextFile` at `source/harness/cli.cpp` before the filter compile, handing the text to `detail::gatherOptionSources` ahead of the run. Measured from a scratch directory holding `filter=bmRead`: `--options=filt.txt` runs the one `bmRead` row at status 0 and no `bmWrite` row, and `--options=/proc/nope/options.txt` answers status 2 with zero lines on standard output and one line `--options: the file '/proc/nope/options.txt' cannot be opened: No such file or directory` (FR-029, FR-030, FR-034).

- [x] T066: correct the schema version the reader test plants, writes
  test/source/harness_json_load_test.cpp, proves the planted-disagreement
  case searches the written document for the schema minor the writer
  stamps today and `harness_json_load_test` passes (FR-086).

  Verified 2026-10-23: the needle stands at
  `test/source/harness_json_load_test.cpp:529` and reads
  `"schemaVersion":"1.3"`, the minor `detail::kSchemaVersion` answers after
  T037, where it had searched for `1.1`; `ctest --test-dir build-ulw -R
  harness_json_load_test` answers Passed (CTest #62).

## Coverage map

Every FR and SC id defined in [spec.md](./spec.md) has one row;
the tasks column lists the task ids whose closing list cites it.

| Requirement | Tasks |
| --- | --- |
| FR-001 | T004, T006, T007 |
| FR-002 | T007, T009 |
| FR-003 | T001, T002, T003, T007, T016 |
| FR-004 | T004, T006, T017 |
| FR-005 | T007, T009 |
| FR-006 | T006 |
| FR-007 | T015, T063 |
| FR-008 | T015 |
| FR-009 | T010, T011 |
| FR-010 | T009, T010, T011, T012, T013, T014 |
| FR-011 | T010, T011 |
| FR-012 | T009, T010, T011, T012 |
| FR-013 | T001, T013, T014 |
| FR-014 | T009 |
| FR-015 | T018, T019 |
| FR-016 | T018, T019 |
| FR-017 | T018, T020 |
| FR-018 | T018, T020 |
| FR-019 | T022, T023, T024 |
| FR-020 | T022, T023 |
| FR-021 | T022 |
| FR-022 | T027, T028 |
| FR-023 | T027, T028 |
| FR-024 | T015, T025, T026 |
| FR-025 | T015, T025, T026, T033 |
| FR-026 | T025, T026 |
| FR-027 | T025, T026 |
| FR-028 | T029 |
| FR-029 | T032, T033 |
| FR-030 | T030, T031 |
| FR-031 | T032, T033 |
| FR-032 | T032, T033 |
| FR-033 | T032, T033 |
| FR-034 | T032, T033, T065 |
| FR-035 | T033 |
| FR-036 | T034, T035 |
| FR-037 | T034, T035 |
| FR-038 | T034, T035 |
| FR-039 | T036, T037 |
| FR-040 | T036, T037, T043 |
| FR-041 | T034, T035 |
| FR-042 | T043 |
| FR-043 | T036, T038 |
| FR-044 | T039, T040, T041, T064 |
| FR-045 | T039, T040, T064 |
| FR-046 | T039, T040, T064 |
| FR-047 | T042, T043 |
| FR-048 | T041 |
| FR-049 | T043 |
| FR-050 | T039, T041, T064 |
| FR-051 | T044 |
| FR-052 | T045, T046 |
| FR-053 | T005, T044 |
| FR-054 | T015, T045, T046 |
| FR-055 | T047, T048 |
| FR-056 | T047, T048 |
| FR-057 | T049 |
| FR-058 | T048 |
| FR-059 | T001, T047, T048 |
| FR-060 | T048, T050 |
| FR-061 | T051, T052, T054 |
| FR-062 | T051, T052 |
| FR-063 | T051, T052 |
| FR-064 | T051, T053 |
| FR-065 | T051, T053 |
| FR-066 | T055 |
| FR-067 | T055 |
| FR-068 | T055, T056 |
| FR-069 | T057 |
| FR-070 | T057 |
| FR-071 | T057 |
| FR-072 | T058 |
| FR-073 | T058 |
| FR-074 | T054, T057, T059 |
| FR-075 | T008, T056 |
| FR-076 | T060 |
| FR-077 | T060 |
| FR-078 | T060 |
| FR-079 | T004, T007, T016, T017 |
| FR-080 | T061 |
| FR-081 | T024, T042, T049 |
| FR-082 | T021 |
| FR-083 | T050, T062 |
| FR-084 | T030, T037 |
| FR-085 | T062 |
| FR-086 | T036, T037, T060, T066 |
| Requirement | Tasks |
| --- | --- |
| SC-001 | T004, T016 |
| SC-002 | T009, T024 |
| SC-003 | T010, T025, T032, T039, T045, T051 |
| SC-004 | T041, T042, T064 |
| SC-005 | T005, T048 |
| SC-006 | T051, T053 |
| SC-007 | T001 |
| SC-008 | T018, T057, T059 |
| SC-009 | T008, T055, T058 |
| SC-010 | T061 |
| SC-011 | T036, T038 |
| SC-012 | T048, T050 |
| SC-013 | T001, T002, T011, T012 |
| SC-014 | T061, T062 |

### Reverse check

- FRs with no task: 0
- SCs with no task: 0
- Tasks citing no FR or SC id: 0
- Cited ids absent from the spec: 0

## Phase 9: Convergence

The `/speckit.converge` pass of 2026-10-23 read the working tree of
`028-full-command-line` at base `bd10d76` against `spec.md`, `plan.md`,
the eight contracts and the constitution, then ran the gates. Every gate
this machine can settle answers clean: `ctest` 120 of 120, the same suite
pinned to two cpus for the FR-081 clause at 120 of 120 with nothing
skipped, the coverage chain at 100.0% lines (8938 of 8938) and 100.0%
branches (4369 of 4369) at gate exit 0, the DBC gate with 274 interfaces
and 0 gaps, `format-check`, `spell-check`, `prose-lint`, the time-source
gate, the loop-shape gate, the entry-point shape gate and
`python3 tools/docs/example_drift.py .`; the `ci-ubuntu` build answers
exit 0. The `ci-tsan` suite answers 117 of 117 passed at exit 0, after
one earlier pass stalled once on the test
`harness_execution_context_test` under heavy parallel load; that test
passed twice on its own afterwards and carries no `TIMEOUT` property.
The `ci-sanitize` leg stays open: GCC 16.2.1 refuses
`-Werror=maybe-uninitialized` inside
`/usr/include/c++/16/bits/std_function.h` reached from `std::regex`, and
it refuses the same way in translation units this feature never touched,
so the CI runner on GCC 15 stays the authority for that leg. The intent
inventory closes at 86 FRs and 14 SCs with no gap in either direction.
Nine findings remain, three of them readings the ledger records that
measurement contradicts.

- [x] T067: drive the two end-to-end environment cases through a forked
  child that execs the test binary with an explicit `envp`, writes
  `test/source/harness_cli_sources_test.cpp`, proves the entry point
  still answers status 2 for a bad `SPEEDGUN_ITERATIONS` value and reads
  one `SPEEDGUN_FILTER` row, and proves no `setenv` or `unsetenv` call
  stands anywhere under `test/` (the two calls sit at
  `harness_cli_sources_test.cpp:1363` and `:1685`, beside the file's own
  comments at `:58` and `:384` and `contracts/environment.md:103`, which
  say the route is a fork plus an `exec` carrying the rows in `envp`, the
  pattern of research D-04) per FR-012, FR-081, SC-003 (contradicts).

  Verified 2026-10-23: `test/source/harness_cli_sources_test.cpp` stands at
  1844 lines, md5 `3599e828844efc2539323380d5bbd684`, with one helper block
  (`kChildWord`, `runChild`, `runEntryWithEnv`, `runChildMode`) that copies
  `environ`, appends the named rows, forks, and execs `/proc/self/exe`
  through `execve` with that `envp`. A whole-tree grep of `test/` for
  `setenv`, `unsetenv` and `putenv` answers 0 matches. The suite builds in
  `build-impl-envroute` and prints `ok` at status 0, twice in a row, and the
  registered CTest #67 passes. `grep -c setenv` over the file answers 0,
  `SPEEDGUN_FILTER` is read in 9 places, `execve` stands twice. Liveness of
  the fast 29 ms run: two planted always-false checks, one inside
  `theEnvironmentSwitchLeavesTheProcessNamesUnread` and one inside
  `aRefusedValueOfEveryKindNamesItsSource`, answer `2 checks failed` at
  status 1 naming both, so both bodies run. The file was then restored byte
  for byte to the md5 above, rebuilt, and prints `ok` again. At the real
  surface, `SPEEDGUN_ITERATIONS=nonsense` beside a filter answers status 2
  with the one line naming the variable and the reader fault.
- [x] T068: re-read the two reserved numbers from the master the branch
  stands on and stamp the pair that rule gives, writes
  `CMakeLists.txt`, `source/harness/json_report.cpp`,
  `docs/schema/report-v1.json` and the `1.3` pins of
  `test/source/harness_json_report_test.cpp`, proves the project version
  and the schema minor each equal the value the fleet numbers leave for
  this lane at that head (`bd10d76` answers `VERSION 0.16.0` at
  `CMakeLists.txt:9` and `kSchemaVersion` `1.1` at `json_report.cpp:31`;
  PR #46 carries `0.17.0` and PR #47 carries `0.19.0`, so this branch
  takes `0.18.0` with `1.3`), and corrects the
  Phase 8 note of T060 where it names a version `master` carries per
  FR-078, FR-086, D-19 (contradicts).

  Verified 2026-10-23: the branch stands on `origin/master` `bd10d76`,
  which answers `VERSION 0.16.0` at `CMakeLists.txt:9` and
  `kSchemaVersion` `1.1` at `source/harness/json_report.cpp:31`. The
  version half is `0.18.0`: `CMakeLists.txt:9` carries it, PR #46 carries
  `0.17.0` and PR #47 carries `0.19.0`, so the pair this branch answers is
  `0.18.0` with `1.3` (FR-078, FR-086). An earlier reading here paired
  `0.17.0` with this branch, the staleness this task exists to catch. The
  schema minor keeps the reservation the Schema minors bullet of Parallel
  work records, where N13a takes `1.2`, N7 takes `1.3` and N14a takes
  `1.4`; `1.3`
  stands at `source/harness/json_report.cpp:33` with its test pins at
  `test/source/harness_json_report_test.cpp:508` and `:526`.
  `docs/schema/report-v1.json` carries no version literal to stamp: its
  `schemaVersion` property is a plain string at `:11` to `:13`. The stale
  reading sat in `research.md` D-19, which named the version as the next
  free minor master carries; that entry now names `0.18.0` beside `1.3`
  and names `1.2` as the minor specs/027 holds while it stays
  unlanded. No number moved in the tree this pass.
- [x] T069: record the base console and document oracle beside the status
  matrix, writes `test/data/base_cli_console.txt` and
  `test/base_cli/run_matrix.py`, proves a run with no new option prints
  the digit-masked console text the `bd10d76` binary prints and answers
  the same status, and writes a JSON document whose top-level key set
  matches the base document with `schemaVersion` as the only differing
  value (the runner compares the exit status and the shortest
  digit-masked standard-error line only, and `test/data/` holds no console
  text and no base document to compare against) per SC-007, FR-003,
  FR-059, FR-086, US1 Independent Test (partial).

  Verified 2026-10-23: `test/data/base_cli_console.txt` stands with 15
  records over the 26 rows of `test/data/base_cli_matrix.txt`, each record
  the case name, the digit-masked console text and the sorted key path set
  of the document that run wrote, captured from a binary built out of
  master at `bd10d76`. The file's comment names why 11 cases carry no
  record: three print a host's own table (`--catalog`, `--topology`,
  `--counter`), and the cases this feature answers on purpose
  (`--help`, `--version`, `--threads`, `--interleave`, the one-line usage
  fault) have no base answer to hold. `test/base_cli/run_matrix.py` replays
  every row against the binary the caller names, compares a recorded case
  line by line and reads its document back for the key set with values and
  `schemaVersion` left out. Over
  `build-impl-oracle/example/docs_command_line` it answers `26 of 26 with
  all rows matching` at status 0. Liveness: one planted record line
  (`bmWrite/cells:#` widened to name a `/planted:7` segment) answers
  `case04: console line 2 expected 'bmWrite/cells:#/planted:7', observed
  'bmWrite/cells:#'` and `25 of 26 with 1 row(s) mismatching` at status 1.
  The file was restored byte for byte, md5
  `609406850271b273b69dde1c39637c3c`, and the replay answers 26 of 26
  again.

  Corrected by the sanitizer gates on 2026-10-23. The `ci-tsan` suite
  reported 116 of 117 and the `ci-coverage` suite reported one failure,
  `base_cli_matrix` in both trees, on seven of the fifteen records. Each
  failure was one blank: the report right-aligns a number to the widest
  value of its run, so a build that measures a narrower value in the same
  column answers `iterations= 614064197` where the release build answers
  `iterations=614064197`, and squeezing a blank run to one space left
  `iterations= #` against the recorded `iterations=#`. The rule in
  `test/base_cli/run_matrix.py` now drops the blanks that follow an `=`
  too, and `read_console` applies that rule to the recorded lines, so both
  sides of the comparison are normalized the same way. A first attempt that
  normalized the live text only made the reading worse, 16 of 26, which is
  what showed the records are compared as they stand. Measured after the
  change: the release, `ci-tsan` and `ci-coverage` binaries each answer
  `26 of 26 with all rows matching` at status 0, and a record planted with
  the word `endpoint samplez` answers `25 of 26 with 1 row(s) mismatching`
  at status 1, so the rule hides padding and nothing else.
- [x] T070: answer the status and the absent benchmark row of every
  source fault through the entry point, writes
  `test/source/harness_cli_sources_test.cpp`,
  `test/source/harness_cli_threads_test.cpp` and
  `test/source/harness_cli_interleave_test.cpp`, proves a bad environment
  value, a bad option-file line, a forbidden option-file line, a bad
  `--threads` list, a bad interleave seed and a bad rules-file line each
  answer one line, status 2, and no benchmark row (the thread-list and
  seed cases assert the reader's refusal text and comment that the entry
  point prints it at status 2, a route only the option-file case at
  `harness_cli_sources_test.cpp:1061` and the environment case at `:1372`
  exercise) per SC-003, FR-027, FR-033, FR-036, FR-046, FR-054, FR-064
  (partial).

  Verified 2026-10-23. Two cases and one helper landed in
  `test/source/harness_cli_sources_test.cpp` (`oneFaultLine`, then
  `aRefusedThreadRowFaultsTheEntry` at `:1878` and
  `aRefusedSeedRowFaultsTheEntry` at `:1906`); the file stands at 1987
  lines, `+113` over the 1874 before it, md5
  `3e672c022dca47e1e7ac69d9dab0922e`. The two files this item also names,
  the thread suite and the interleave suite, needed no write: the reader
  refusals they were meant to hold already stand there, twelve refusal-text
  matches in one and three in the other. The seed row of the option table is
  named `interleave`, so FR-019 computes its environment name as
  `SPEEDGUN_INTERLEAVE`, while `SPEEDGUN_SEED` names no row and FR-026
  ignores it, the second independent reading of that fact this round. All
  three suites print `ok` at status 0 in `build-impl-srcentry` and
  `clang-format-18` is clean over the three of them. At the real surface
  eight command lines answer the shape the criterion names, each at status 2
  with one line on standard error, nothing on standard output and no
  benchmark row: the environment words `SPEEDGUN_THREADS=nonsense` and
  `SPEEDGUN_INTERLEAVE=nonsense`, the option file lines `threads=nonsense`
  and `interleave=nonsense`, the command line `--threads=0`, an option file
  naming an unknown name, and an option file carrying a forbidden row, read
  twice over with `options=` and with `help=`, each answering that the
  option cannot stand in an option file. Liveness: two planted
  always-false checks, one in each new case, answer `2 checks failed`; the
  file was restored byte for byte to the md5 above, rebuilt, and prints `ok`
  again.
- [x] T071: run the two-count case the success criterion names, writes
  `test/source/harness_cli_threads_test.cpp`, proves `--threads=1,<n>`
  over the test family runs every benchmark at both counts, that the
  one-count row names no `/threads:` segment and the `<n>` row carries
  `/threads:<n>`, read through `sched_getaffinity` so no cpu count stands
  in the file (the file holds the reader cases and the over-affinity run
  and no case drives two counts; `--list --threads=1,4` answers
  `bmRead/cells:256` and `bmRead/cells:256/threads:4` today and a run
  prints both rows, so the behavior stands with no test holding it, and
  the Phase 5 note of T041 reads the name shapes as proven by that file)
  per SC-004, FR-044, FR-048, FR-050 (partial).

  Verified 2026-10-23: the case stands in
  `test/source/harness_cli_table_test.cpp` at `:1453` as
  `aTwoCountThreadListRunsAtBothCounts`. That file is where the in-flight
  write scope put it while `test/source/harness_cli_threads_test.cpp` stood
  in another lane's hands, and the behavior it pins is the one this task
  names. Two runs of the entry point in process: the listing
  `--threads=1,2 --list` answers the whole line pair `bmTwoCounts` and
  `bmTwoCounts/threads:2`, and the measured run prints exactly two rows
  naming the family, the bare one and the `/threads:2` one, under
  `--iterations=1 --no-percentiles` so the row count is the count of the
  counts. `bmTwoCounts` is a new empty-body family in that target, which
  registered no benchmark before, shaped as
  `test/source/harness_cli_threads_test.cpp:105` shapes `bmThreads`. Both
  counts come from the option text, so no machine cpu count stands in the
  file. `./build-impl-twocount/test/harness_cli_table_test` answers `ok` at
  exit 0 on two runs and `ctest -R '^harness_cli_table_test$'` passes as
  test 66. Liveness came from a scratch copy under `/tmp` whose expected
  second listing name was planted as `/threads:9`: that run answers exactly
  one failed check, the new case, exit 1. `clang-format-18 --dry-run -Werror`
  prints nothing over the file.
- [x] T072: answer the ambiguity line for the four prefixes no case names,
  writes `test/source/harness_cli_table_test.cpp`, proves `--t`,
  `--compare-r`, `--compare-ru` and `--compare-rul` each answer the
  ambiguity line with their candidates in table order beside the `--i`
  case that stands at `harness_cli_sources_test.cpp:362` (the built binary
  answers all five at status 2 over the 27 row table today, so the cases
  record behavior no test holds) per FR-013, FR-009 (partial).

  Verified 2026-10-23, with the file home moved from the one this item
  names. The four rows stand in `kAmbiguous` at
  `test/source/harness_cli_sources_test.cpp:378` to `:389`, driven by
  `theAmbiguousPrefixNamesBothCandidates`, because the `--i` row this item
  says they sit beside is a row of that same table and that file: the six
  ambiguity rows now stand in one table in one suite. The named
  `test/source/harness_cli_table_test.cpp` holds the option table, help and
  version cases and was not touched. Measured at the real surface over
  `build-impl-oracle/example/docs_command_line`, each of `--i`, `--t`,
  `--compare-r`, `--compare-ru` and `--compare-rul` answers status 2 with no
  line on standard output and exactly the text the row holds: `topology,
  threads` for the `t` prefix and the pair `compare-rule`, `compare-rules`
  for the three `compare` prefixes. The file stands at 1874 lines, `+41` and `-11`
  over the 1844 before it, and the target built in `build-impl-abbrev`
  prints `ok` at status 0. Liveness without touching the file that
  `harness_cli_sources_test.cpp` work was in flight on: a copy at
  `/tmp/abbrev-live/partner.cpp` with the `t` prefix row's candidate list
  reordered answers `1 checks failed` at status 1, naming the case text for
  that prefix as the one failure.
- [x] T073: carry the source faults in the exit status table, writes
  `specs/015-benchmark-harness-core/contracts/cli.md`, proves a row stands
  for the status 2 answer of the environment, option file, rules file,
  thread list and seed sources beside the six rows the command-line scan
  already carries at `:110` to `:115`, and the prose gate reads zero
  findings over the file per FR-082, SC-003 (partial).

  Verified 2026-10-23: seven rows now stand after the six scan rows, for
  the `SPEEDGUN_*` value (FR-027), the option file line (FR-033, FR-036),
  the option file that cannot be opened (FR-034), the rules file line
  (FR-064), the rules file that cannot be opened (FR-065), the thread list
  (FR-045, FR-046) and the seed text (FR-054). Each row names the status 2
  answer and the line the harness prints. The readings came off a
  `ci-rocky` build in `build-impl-contract` driving
  `docs_command_line --list`, for instance the line
  `SPEEDGUN_THREADS: --threads: expected counts and min:max ranges, got 'nonsense'`
  and `--options: the file '<path>' cannot be opened: No such file or directory`. `env SPEEDGUN_SEED=nonsense` answers status 0, and
  `SPEEDGUN_SEED` names no row of this feature, so an environment name the
  table does not carry stays ignored. The file's diff reads 29 added lines,
  of which 14 stand from the earlier scan-fault work.
  `python3 tools/prose/prose_gate.py --mode tree --paths
  specs/015-benchmark-harness-core/contracts/cli.md` answers 1 source, 119
  units examined, 0 findings.
- [x] T074: clear the new cppcheck error findings, writes
  `source/harness/options.cpp` and the three test files that read the
  table by pointer, proves cppcheck over `source/harness` answers the one
  finding it answers at `bd10d76` (the base tree answers
  `runner.cpp:2260 syntaxError` alone; this head answers that one plus
  `options.cpp:1398 returnDanglingLifetime` and `options.cpp:1633
  danglingTemporaryLifetime`, and three more name the same
  address-of-a-table-row idiom at `harness_json_report_test.cpp:567`,
  `harness_cli_sources_test.cpp:455` and `harness_cli_table_test.cpp:1096`,
  over the `static constexpr` array `optionRows` views, a false positive
  of the kind `include/speedgun-ng/dbc.hpp:140` carries a suppression
  for), and corrects the Phase 8 note of T062, which reads no new finding,
  per SC-014, FR-085 (contradicts).

  Verified 2026-10-23. Five suppression sites stand, each on the offending
  statement and each under its own reason line: `source/harness/options.cpp`
  at `:1399` and `:1636`, `test/source/harness_json_report_test.cpp` at
  `:568`, `test/source/harness_cli_sources_test.cpp` at `:500`, and
  `test/source/harness_cli_table_test.cpp` at `:1102`. The reason the sites
  state is true: `optionRows` is defined at `source/harness/options.cpp:404`
  and answers a view over `static constexpr auto kRows`, storage that
  stands for the process, so taking the address of one of its rows dangles
  nowhere. The reading matches the precedent the item names at
  `include/speedgun-ng/dbc.hpp:140`. Over `source/harness` with
  `--inline-suppr`, `cppcheck` 2.17.1 answers one finding, the base one at
  `source/harness/runner.cpp:2260:53` with id `syntaxError`, and exits 0. A
  scratch copy of `options.cpp` with the two suppression lines deleted
  answers both findings again at `:1399` with id `returnDanglingLifetime`
  and `:1635` with id `danglingTemporaryLifetime`, so the lines carry their
  weight. The scratch copy was removed in the same command that made it and
  `git status` over `source/harness` answers its usual eight entries.

  Corrected here, owed by the Phase 8 note of T062: that note's `ci-ubuntu`
  reading covers the build exit code and says nothing about the finding
  list, and the reading that no new finding stands was wrong. Before any
  suppression line existed at this head, the list held the base finding
  beside `options.cpp:1398` with id `returnDanglingLifetime` and
  `options.cpp:1633` with id `danglingTemporaryLifetime`, and three further
  findings of the same address-of-a-row idiom stood in the three test files
  named above. The five lines T074 landed, each checked against the
  definition at `options.cpp:404`, are what make a clean scan true at this
  head.
- [x] T075: restores the base codespell skip list, writes `.codespellrc`,
  proves `cmake -P cmake/spell.cmake` answers zero findings over a source
  tree holding no scratch build dir, proves `.codespellrc` equals the
  file `master` carries once the branch change to it stands dropped, and
  records in this ledger why the
  `*/build-*` pattern is gone, because no FR, task or contract of this
  feature names that file (Principle VIII, unrequested).

  Verified 2026-10-23: `git diff --stat origin/master -- .codespellrc`
  prints nothing, so `.codespellrc` equals the file `master` carries and
  the skip list is the base list
  `*/.git,*/build,*/build-tidy,*/prefix,*/node_modules,*/.omo,*/external`.
  The commit that added `*/build-*` was dropped. Two reasons stand beside
  that. No FR of this feature names `.codespellrc`: the spell gate is a
  Principle VIII gate this feature inherits as `master` configures it, so a
  widened skip list is work no requirement asked for. The pattern also
  reaches past build trees: codespell 2.4.3 matches the skip list with
  `fnmatch` over the full path and `*` matches `/`, so `*/build-*` skips
  four tracked pages, each named `build-integration.md`, at
  `specs/004-vendor-simdjson/contracts/build-integration.md`,
  `specs/005-vendor-hdrhistogram/contracts/build-integration.md`,
  `specs/006-vendor-yaml-cpp/contracts/build-integration.md` and
  `specs/009-vendor-quill/contracts/build-integration.md`. The match is
  measured: a planted misspelling in a copy of such a path is reported
  under the base list and absent under the list that adds `*/build-*`.
  The reading that matters for the gate is where the gate runs: the CI
  `lint` job calls `cmake -P cmake/spell.cmake` at
  `.github/workflows/ci.yml:42` from a checkout that configures no build
  (`.github/workflows/ci.yml:25` to `:27`). Measured inside a `git archive
  HEAD` export, that call answers exit 0 and prints no finding, so the
  tracked tree is clean under the base list and the four vendor pages are
  read. Inside this checkout the same call answers exit 65 and prints
  2,226 findings, and every line names a path under `build-fix/` or
  `build-fix-oracle/`, two untracked scratch build trees this machine
  holds beside `build` and `build-tidy`, which the base list skips. Adding
  those two directory names to the skip list on the command line, with no
  file changed, prints no finding there either (FR-083).

## Phase 10: Convergence

The second `/speckit.converge` pass of 2026-10-23 read the tree at head
`3ab94fe` against `spec.md`, `plan.md`, the eight contracts and the
constitution, then re-ran the gates. `ctest -j 8` answers 120 of 120, and
the same suite pinned to two cpus answers 120 of 120 at exit 0 in 178.81
seconds, which settles the FR-081 clause on a machine holding 32 cores.
The coverage chain closes at 100.0% lines (8938 of 8938) and 100.0%
branches (4369 of 4369) at gate exit 0. The prose gate reads 560 sources,
95514 units and 0 findings, and `spell-check`, `dbc-gate` (274 interfaces,
0 gaps), `format-check`, `cli_shape`, `loop_shape`, `time_source_gate`,
`cppcheck --inline-suppr --quiet source/harness` and
`python3 tools/docs/example_drift.py .` add nothing beside the base
readings. The reverse check still reads 86 FRs, 14 SCs and 100 coverage
map rows with no gap in either direction, and 75 tasks closed.

One item stands. The exit status table of
`specs/015-benchmark-harness-core/contracts/cli.md` carries seven of the
ten status-2 answers this feature prints: the doubled `--options`, the
`--env` word beside `off` and the environment percentile pair have no row,
and the table's own prose at `:91` to `:97` omits them too. `ci-sanitize`
stays open on this machine for the reason Phase 9 records.

The reading this pass left open now stands in the requirement text: a bad
option value from any source prints exactly one line naming the source and
the text, and the run answers status 2, even when a higher source names the
same option (FR-027, FR-033, FR-046, FR-054, FR-064, SC-003).
`contracts/environment.md` and `contracts/option-file.md` carry the same
rule. A run answers differently at the head this round measured:
`source/harness/options.cpp:1190` to `:1202` walks the sources top-down and
stops at the value it takes, so `SPEEDGUN_ITERATIONS=nonsense` beside
`--iterations=1` answers status 0 with no line while the same variable
beside `--min-time=1` answers status 2. A later change owns that reader
half; the requirement text states the rule it must reach.

- [x] T076: carry the three remaining status-2 answers in the exit status
  table, writes `specs/015-benchmark-harness-core/contracts/cli.md`,
  proves a row stands for the doubled `--options` (FR-032), for the
  `--env` word beside `off` (FR-028) and for the environment percentile
  pair (FR-023), each naming the status 2 answer and the line the harness
  prints at `source/harness/options.cpp:761`, `:773` and `:1644`, the
  introducing sentence names all three beside the faults it already lists,
  and the prose gate reads zero findings over the file per FR-082, SC-003
  (partial).
