# Research: Full Command Line

**Date:** 2026-10-22 · **Spec:** [spec.md](./spec.md) · **Branch:**
`028-full-command-line` from `bd10d76`

The spec numbers its own decisions D-1 to D-27. This file numbers the
plan's decisions D-01 to D-33: a bare `D-nn` in an artifact names this
file's entry, and a spec decision cited outside `spec.md` is written
`spec D-nn`, because the two numberings name different decisions from
D-10 to D-27. Every `file:line` anchor below was re-measured on this
tree at `bd10d76` in the pass that wrote this file; the anchors the
audit waves recorded were re-read here and the corrected numbers are
the ones used.

## D-01: The table and the parse move to one new unit

**Decision:** the option table, the option scan, the source gather, the
value readers and the shuffle order live in the new translation unit
`source/harness/options.cpp`, declared in the new internal header
`source/harness/detail/options.hpp`. The numeric value constants move
from `source/harness/cli.cpp:69` to `:71` and `:345` into that header.

**Rationale:** FR-010 puts the parse in `source/harness/detail/`, and
spec D-20 moves the table with it. The harness archive builds from the
`GLOB_RECURSE` at `CMakeLists.txt:196` to `:199`, so a new `.cpp` under
`source/harness/` needs no CMake edit, the way specs/026's
`compare.cpp` needed none. The table, the scan that walks it, the help
printer that reads its text and the uniqueness test that checks its
names all read one object, which is what FR-001 and FR-018 ask.

**Alternatives:** keeping the table in `cli.cpp` and moving only the
scan splits the row data from the code that reads it and leaves the
drift tool scraping one file for a table another file owns; rejected.
Two units, one for the table and one for the gather, give a header
pair where one pair closes the contract; rejected.

## D-02: One row type, one table, and the `getopt` array built from it

**Decision:** the table is one array of `detail::OptionRow` (E-01 of
[data-model.md](./data-model.md)) built with `std::to_array` in
`options.cpp` behind `detail::optionRows()`, which answers
`std::span<const OptionRow>`. The `option` array `getopt_long` takes is
built from those rows at the head of the scan, in table order, with the
row's `name` pointer, a `has_arg` derived from the row's argument kind
and the row's `value`.

**Rationale:** `getopt_long` needs `const char*` names and its own
`has_arg` spelling; the row type needs the help text, the environment
flag and the owning requirement. Deriving the `option` array from the
rows keeps one list (FR-001, FR-018) and keeps the two agreeing by
construction. The names are string literals, so `name` stays a valid
`const char*` for the whole run with no allocation on the scan path.

**Alternatives:** two parallel arrays, one `option[]` and one metadata
row per option, is the shape the base has (the table at
`source/harness/cli.cpp:346` to `:371` beside the four value constants
at `:69` to `:71`) and is what FR-001 forbids; rejected. A
`constexpr` table in the header hides the row count from the drift
scraper's line shape and puts a table in a header that only one
translation unit reads; rejected.

## D-03: The parse answers an expected; the entry point owns the print

**Decision:** two internal calls split the work.
`detail::scanCommandLine(argc, argv)` walks `argv` and answers
`std::expected<CommandLineOptions, std::string>` (E-06).
`detail::gatherOptionSources(scan, environment, optionFile)` combines
the sources and answers `std::expected<ResolvedOptions, std::string>`
(E-07). Neither calls `std::exit`. `speedgunMain` prints the one fault
line and returns 2, the way the compare mode already returns its status
through `detail::runCompareMode` (`source/harness/cli.cpp:524` to
`:530`).

**Rationale:** FR-010 and FR-011 remove the process exit from the parse
path, and SC-013 requires every source fault to reach status 2 through
the parse. The four `std::exit(2)` reject helpers of the base
(`source/harness/cli.cpp:80`, `:132`, `:152`, `:174`) and the nine
in-scan `return 2` sites (`:431`, `:445`, `:461`, `:479`, `:492`,
`:506`, `:517`, `:542`, `:562`) collapse into one error text plus one
print. The split into scan and gather is forced by precedence: the
option file path and the `--env=off` switch are only known after the
scan, and the environment must be read after them (spec D-1).

**Alternatives:** one call that scans, reads the environment and reads
the file needs the process environment and the file system inside the
parse, which FR-012 forbids; rejected. Keeping the helpers and passing
a status out is the shape the constitution's contract rules disfavor.

## D-04: The environment arrives as data

**Decision:** `detail::environmentRows()` reads the process environment
once, in `cli.cpp`, and answers
`std::vector<EnvironmentRow>` (E-02) of the `SPEEDGUN_*` names and
their values. `gatherOptionSources` takes that vector as an argument.
A unit test builds the vector by hand. The end-to-end inheritance
check forks the test binary and execs it with an explicit environment
array, the pattern of `test/source/harness_cli_placement_test.cpp:324`
(fork, capture, wait) joined with the `execv` of the test binary itself
at `test/source/harness_custom_stats_test.cpp:385`, with `execve`
carrying the added `SPEEDGUN_*` rows.

**Rationale:** FR-012 requires a test to drive every source with no
process environment and no `setenv` call, and no `getenv`, `setenv` or
`environ` use exists anywhere in `source/`, `include/`, `example/` or
`test/` at this head, so the seam is new. The house already separates
a machine read from its decision this way:
`detail::selectRunnableCpuCount` (`source/harness/report.cpp:303`,
declared at `source/harness/detail/internal.hpp:382`) is driven
directly with data by `test/source/harness_context_report_test.cpp:850`.
`execve` with an explicit `envp` gives a real child process that
inherits the rows without touching the test process's environment.

**Alternatives:** `setenv` in the test is what FR-081 and FR-012 rule
out, and it would leak into every later case in the same binary;
rejected. A `std::function` seam over a name lookup adds an indirection
where a vector of rows carries the same power; rejected.

## D-05: The option file arrives as text, and the text reader is shared

**Decision:** `gatherOptionSources` takes
`std::optional<OptionFileText>` (E-03: the path and the whole text).
`cli.cpp` reads the file after the scan through `detail::readTextFile`,
and a file the system refuses prints the FR-034 line there and returns
2. `detail::readTextFile` is the reader now sitting in the anonymous
namespace of `source/harness/json_load.cpp:26` to `:48`: it moves to
`options.cpp`, is declared in `source/harness/detail/internal.hpp`, and
`json_load.cpp` calls it at its two sites (`:1159`, `:1194`). Its fault
texts stay byte-identical, so the reader tests keep their expectations.

**Rationale:** FR-012 names the option-file text as the argument. The
rules file of C-7 needs the same read, and the loader already owns a
reader that handles the open failure, the read failure and the chunked
append; a second copy would be the duplication the house rule against.
The fault text `the file '<path>' cannot be opened: <reason>` is also
the shape FR-034 and FR-065 ask for.

**Alternatives:** passing the path into the gather and reading inside it
puts the file system behind the parse seam and forces a temporary file
for every file case; rejected. Writing a second small reader in
`options.cpp` duplicates twenty lines and two fault texts; rejected.

## D-06: What `getopt_long` answers when `opterr` stands at 0

**Decision:** the scan sets `opterr = 0` and passes the option string
`":"` (spec D-15), then reads the return code and `optopt`:

- `':'` is a missing option argument. `optopt` carries the row's value,
  so the row is found by value and named.
- `'?'` with `optopt` at 0 is a long-option fault: unknown name or
  ambiguous prefix. `optind` has already moved past the word, so the
  text sits at `argv[optind - 1]`.
- `'?'` with `optopt` holding a character is a short option, which the
  empty option string makes unknown.

The scan decides ambiguous against unknown by its own prefix match over
the rows: two or more rows start with the typed text, so the candidates
are computed there (spec D-25).

**Rationale:** these are measured facts of this tree's C library, taken
in this pass with the final row shape: `--i` and `--compare-r` answer
`?` with `optopt` 0 and `optind` 2; `--nonsense=1` answers `?` with
`optopt` 0; `--filter` with no value answers `:` with `optopt` 102;
`-j` answers `?` with `optopt` 106. The base prints two lines for each
of those cases (the C library's own diagnostic beside the house line at
`source/harness/cli.cpp:505`), which is what SC-003 and FR-007 to
FR-009 forbid. `opterr = 0` is what silences the first line, and the
leading `:` in the option string is what separates a missing value from
the rest.

**Alternatives:** leaving `opterr` at 1 and intercepting `stderr` cannot
produce one line; rejected. Reading the candidate set from the C
library is impossible: it reports no list once `opterr` stands at 0,
which is why spec D-25 puts the computation in the parse.

## D-07: The three usage lines, and the nearest name

**Decision:** the fault texts are these, each one line on standard
error at status 2:

- unknown: `unknown option '<text>'; --help lists the options`, and
  when a row name sits within two edits of the typed name, the line
  ends `unknown option '<text>', nearest '--<name>'` instead.
- ambiguous: `ambiguous option '<text>': <candidate>, <candidate>` in
  table order.
- missing value: `option '--<name>' needs a value` (FR-008 verbatim).

The distance is plain Levenshtein over the long names, computed on the
cold path of a fault.

**Rationale:** FR-007 asks one line naming the name and the nearest
table name within two edits; FR-009 asks the candidates in table order.
The base line `unknown option; see the option table of` plus the
contract path (`source/harness/cli.cpp:503` to `:506`) names nothing.
The three checks that read the base line, at
`test/source/harness_cli_test.cpp:806` and `:810` and
`test/source/harness_cli_placement_test.cpp:449`, look for the words
`unknown option`, which the new line keeps at its front, so they stay
green and the new assertions name the typed text and the nearest name.
Pointing at `--help` keeps the line short and gives the reader
the whole table, which the base pointer did through a file path.

**Alternatives:** keeping the contract path in the line repeats the
base's indirection and lengthens every fault; rejected. A candidate
list for an unknown name (a prefix match that found nothing) would
print an empty list; rejected.

## D-08: `--help` and `--version` answer the first event in walk order

**Decision:** the scan records the first of `--help`, `--version` or a
usage fault that its left-to-right walk meets (FR-017). A fault stops
the scan and answers the error text. `--help` stops the scan and prints
one line per row in table order: the name with its argument kind, then
the help text. `--version` stops the scan and prints
`speedgun-ng <version>` on one line (spec D-26). Both answer status 0
and both run no benchmark. `--help` beside `--version` prints the help
alone, the reading FR-017 states.

**Rationale:** the base rejects both words with the unrecognized-option
line at status 2, measured in the base matrix at rows 15 and 16, so
this feature changes command lines the base refused. The Unchanged run
constraint of the spec covers command lines the base runs, so those
two rows are inside the change. The version text is the one the console
report carries: `SG_PROJECT_VERSION` (`CMakeLists.txt:231`) printed by
`detail::printContext` (`source/harness/report.cpp:317`).

**Alternatives:** answering the fault first regardless of order would
make `--help --nonsense=1` print a fault, which FR-017 forbids. Printing
help on standard error for a usage fault is the
common convention elsewhere and would break FR-017's single stream and
the one-line rule; rejected.

## D-09: The environment name of a row, and the unknown `SPEEDGUN_` name

**Decision:** a row's environment name is `SPEEDGUN_` plus the long
name upper-cased with `-` changed to `_` (spec D-2), computed from the
row rather than stored in it. `environmentRows()` returns every name
that starts with `SPEEDGUN_`; the gather matches each against the
flagged rows and ignores a name that matches none (FR-026). The
flagged set is the `fromEnvironment` field of the row, and it holds
exactly the twelve rows FR-020 names.

**Rationale:** computing the name from the row keeps one source for the
spelling, which is what the guide reference test (D-22) needs to check
the page against the table. A stored column would let the two spellings
drift apart.

**Alternatives:** a literal name in each row is one more field whose
spelling nothing checks, and it gives the drift nothing to compare;
rejected. Reading only the
twelve known names from the environment cannot answer FR-026's ignore
rule with a test that plants an unknown name; rejected.

## D-10: The percentile pair is read on the environment pair alone

**Decision:** the gather checks `SPEEDGUN_PERCENTILES` against
`SPEEDGUN_NO_PERCENTILES` over the environment rows before precedence
runs, and a pair standing together answers one line naming both
variables at status 2 (FR-023, spec D-5, spec D-27). The command-line
pair keeps the base answer: the last word wins and the status stays 0.

**Rationale:** precedence (spec D-1) would let a command-line half
erase an environment contradiction the operator should see. The base
behavior of the command-line pair is a measured row of the base matrix
(row 14: `--percentiles=50 --no-percentiles` answers status 0), and the
Unchanged run constraint keeps it.

**Alternatives:** one rule for both sources changes a command line the
base runs, which the constraint forbids. Reading the pair after
precedence cannot see the pair at all when one half is overridden.

## D-11: The value forms the environment and the file share

**Decision:** one reader per value kind, shared by the environment and
the option file. A repeatable splits on `;` into items (FR-024). A flag
takes `1`, `0`, `true`, `false` (FR-025). Every value then goes through
the same reader the command line uses, so a rejected value names the
variable or the file line beside the option (FR-027).

**Rationale:** FR-035 says a file line sets its option under the
command-line value rules; the environment rows reach the same rules.
One reader per kind is what makes the three sources answer the same
text for the same bad value, and it is what keeps the coverage duty
(FR-080) to one set of branches.

**Alternatives:** per-source readers duplicate the number grammar of
`--min-time`, `--repetitions` and `--percentiles`; rejected.

## D-12: The option-file line grammar

**Decision:** a line is `name`, or `name=value`, in long spelling
without the leading `--`. The first non-space character `#` starts a
comment; a blank line sets nothing (FR-030). A line whose first
non-space character is `-` is a fault, and so is a required-argument
line with no `=` (FR-036, spec D-22). The names `options`, `env`,
`help` and `version` are refused by name (FR-031). Line numbers count
from 1 over the raw lines, so a comment and a blank line hold a number.

**Rationale:** the fault lines of FR-031, FR-033 and FR-036 all name
the file, the line number and the text, so the reader keeps the raw
line and its number beside the parsed pair. Refusing a line that starts
with `-` is what stops `--filter=x` from reading as a name of `-` plus
a suffix, and refusing the four names is what stops a file from
re-rooting the sources or printing help.

**Alternatives:** accepting `--filter=x` in a file would let one file
carry two grammars; rejected. Counting only the lines that set
something would make the fault line number disagree with the file the
reader opens in an editor; rejected.

## D-13: The rules file reuses the rule parser of specs/026

**Decision:** `--compare-rules=<file>` reads the text through
`detail::readTextFile`, splits it on newlines, drops a comment and a
blank line, and calls `sg::parseCompareRule`
(`include/speedgun-ng/benchmark.hpp:2589`) on each remaining line. A
refused line answers one line naming the file, the line number and the
parser's own text at status 2 (FR-064). The rules of one source keep the
order their entries were written, spec D-23, and the rule list does not
combine across sources: the highest source that names rules settles it, a
lower rule reaching the parser before it drops (FR-038).

**Rationale:** the grammar stays exactly as specs/026 wrote it
(FR-061), and the parser already answers its fault text. A rules file
holds rules alone, so a line naming another file reaches the parser and
is refused there, which is the answer FR-061 states.

**Alternatives:** a file-level wrapper that pre-validates lines would
duplicate the grammar; rejected. Letting a rules file name another
rules file needs a cycle guard for no requirement; rejected.

## D-14: `--threads` lands on the registry before the expansion

**Decision:** the parsed list is written into
`RegistryEntry::threadCounts`
(`source/harness/detail/internal.hpp:199`) for every entry of
`detail::registry()` (`source/harness/detail/internal.hpp:244`) in one
loop that stands before `detail::expandRegistry`
(`source/harness/cli.cpp:570`). A `min:max` item expands through
`detail::buildThreadCounts`
(`source/harness/detail/internal.hpp:315`, defined at
`source/harness/registry.cpp:574`), the doubling rule the handle's own
`threadRange` uses (`include/speedgun-ng/benchmark.hpp:2351`,
`source/harness/registry.cpp:613`).

**Rationale:** the expansion crosses the argument lists with the thread
counts at `source/harness/family.cpp:404` and `:425`, and
`detail::instanceName` appends `/threads:` and the count only above one
(`source/harness/family.cpp:246`), so replacing the counts before the
expansion gives FR-048's names with no change to the naming rule and
makes the option apply whatever the family called (FR-050). The fit
check at `source/harness/cli.cpp:607`
(`detail::placementFitStatus`, `source/harness/placement_preset.cpp:173`)
already refuses a preset that cannot seat a count, so FR-049 needs no
new code. `checkFamilySize` (`source/harness/family.cpp:290`) against
`kMaxFamilySize` (`source/harness/detail/internal.hpp:35`) keeps its
warning for a list that grows a family past one hundred instances.

**Alternatives:** plumbing the list into the expansion path reaches
three call sites in `family.cpp` for what one loop over the registry
closes. Overriding the counts after the expansion cannot work: the
instances are already built.

## D-15: The shuffle is `mt19937_64` with a modulo reduction, and the vector is pinned

**Decision:** `detail::shuffledOrder(count, seed)` (declared in
`detail/options.hpp`, defined in `options.cpp`) fills
`0 .. count - 1`, then walks `i` from `count - 1` down to 1 and swaps
the entry at `i` with the entry at `engine() % (i + 1)` from a
`std::mt19937_64` seeded with the option's seed (spec D-12, FR-053).
The expected permutations are pinned in the test as literals: for
`count = 12`, seed 7 answers
`4 0 2 6 10 9 1 5 11 8 7 3` and seed 8 answers
`7 0 6 10 8 2 11 5 9 4 3 1`. Both readings were measured in this pass
under `g++` 16 and `clang++` 23 and agree.

**Rationale:** `std::uniform_int_distribution` is not fixed across
standard libraries, so a shuffle that used it would answer a different
order per library and FR-053 would be false. The raw engine stream is
fixed by the standard. The reduction bias of `% (i + 1)` over a 64-bit
stream against a count below `2^32` is below one part in `2^32`, far
under the sampling noise of any benchmark this harness runs.

**Alternatives:** `std::shuffle` over a `std::mt19937_64` is the one
line answer and is what the library offers; it binds the order to the
distribution implementation and breaks FR-053 across libraries;
rejected. A hand-written xorshift would fix the order by construction
and drop the standard engine the spec names.

## D-16: Two print paths, held rows, and where the fit prints

**Decision:** the run loop keeps its shape and its one flag reading.
The walk order comes from `shuffledOrder` on the cold path before the
loop. Without `--interleave` the loop prints each row as it runs, the
base path at `source/harness/cli.cpp:646`. With it, the loop pushes the
result, the raw row and the context row of each pass into a held vector
and prints nothing; after the loop, one cold pass walks the instances in
registration order, prints each held row, and prints a family's fit
after the last held row of that family. The held rows are pushed into
`ReportData` in that same registration order, so the console and the
JSON document agree.

**Rationale:** spec D-19 fixes the two paths, and FR-059 requires the
base console text when the option stands absent. `test/cli_shape.sh`
locates the loop from the `runner.run(*entry)` line
(`source/harness/cli.cpp:645`) to the closing brace at `:668`, requires
exactly one `interruptFlag().load()` reading inside it (`:39` of the
script) and the `bool interrupted =` local (`:47`), so the held-row
print and the fit print sit outside that range and the gate stays green
unchanged (FR-060, spec D-18). The fit object a family produces is
independent of the walk order: `runner.fitForFamilyEnd(entry)`
(`source/harness/cli.cpp:637`) summarizes the family's results, so
collecting it in the run order and printing it in the registration
order gives FR-056's position.

**Alternatives:** one print path that always holds rows changes the
base console order for an interrupt and the streaming behavior a reader
watches, which FR-059 forbids. Sorting the rows at print time by name
would order them by text; the registration order is the instance vector
order, which the walk already carries.

## D-17: The interrupt oracle is a body that raises `SIGINT`

**Decision:** the interleaving test registers an instance whose body
calls `std::raise(SIGINT)` at a named iteration of a named instance,
then asserts that the rows that ran print in registration order and the
status is the base interrupt status of 1 (FR-057). No sleep, no clock
(FR-081).

**Rationale:** the spec's scenario says the interrupt "arrives after
three instances", which is a timed event with no oracle in the tree.
`raise` from inside the body reaches the handler installed at
`source/harness/cli.cpp:617` synchronously, so the test names the
instant and no timer stands in the case. The entry point already resets
the flag
per invocation (`:614`), so a suite that runs the entry point twice
stays clean.

**Alternatives:** a timer thread or a `sleep` before the signal is what
FR-081 bars. Sending the signal from the test process to itself with
`kill(getpid())` reaches the same handler but names no instance, so the
"after three instances" reading stays unchecked.

## D-18: The disclosure line, and where the `run` key stands

**Decision:** the console line prints after `detail::printContext`
(`source/harness/cli.cpp:618`) and before the first row (FR-042). Its
text is `options from a file or the environment: ` then one
`--<name>=<value>` entry per disclosed option in table order, then
`interleave=<seed>`, with `; ` between entries (FR-039). The JSON
object `run` stands between `speedgun` and `benchmarks`: the writer
emits it after the `speedgun` object (`source/harness/json_report.cpp:585`
to `:590`) and before `benchmarks` (`:591`), and
`docs/schema/report-v1.json` lists it in that position in `x-order`
(`:7`) with `required` (`:8`) unchanged.

**Rationale:** the schema checker walks the document's own keys and
requires each one's index in `x-order` to stand at or after the
previous one (`source/harness/json_load.cpp:1098` to `:1120`), and an
optional key that stands absent is never visited, so a key that is
present must sit in the stated position. The base document key order is
`schemaVersion`, `speedgun`, `benchmarks`, so `run` joins as the third
of four and the base order stays a subsequence.

**Alternatives:** putting `run` last keeps the base keys in their
positions and reads after the benchmark entries, which buries the run
facts under the entries; rejected. Putting it first would put the run
facts above the version the reader checks first.

## D-19: The schema minor and the project version are taken at rebase

**Decision:** the writer stamps `kSchemaVersion` as `1.3` and the
project version takes `0.18.0`. The fleet carries three numbers: PR #46
takes `0.17.0` and PR #47 takes `0.19.0`, so this lane's minor is
`0.18.0`, the value `CMakeLists.txt:9` carries. Both numbers are read
from master at rebase; the schema minor keeps the reserved value the
Parallel work bullet of the spec records for N7 while specs/027 still
holds `1.2` unlanded.

**Rationale:** FR-078 and FR-086 name a value that depends on a merge
that has not happened, so no test at this head can assert it. The
harness `SOVERSION` at `CMakeLists.txt:261` and the archive `SOVERSION`
at `:53` stay at 10 and 3 (D-24).

**Alternatives:** a number stated once and never re-read at rebase is the
failure mode the parallel-work rule warns about, so this entry keeps the
read beside the number. Bumping the schema minor to `1.2` here would
collide with specs/027.

## D-20: The option-row check follows the table

**Decision:** `cli_options()` (`tools/docs/example_drift.py:458` to
`:466`) reads `source/harness/options.cpp` and matches the
`std::to_array` table with a regex over the declaration line, keeping
the row regex `\{\s*"([a-z0-9-]+)"` that pulls the names. The liveness
fixture writes its stub table to the same path in the same shape
(`test/example-drift-fixture/run_example_drift_fixtures.py:117` to
`:124`), and the planted missing-row case
(`test/example-drift-fixture/run_example_drift_fixtures.py:270` to
`:280`) keeps its expectation of the finding `has no option row`.

**Rationale:** the check at `tools/docs/example_drift.py:681` to `:694`
compares the README option rows against the scraped list in both
directions, so a table that moves without the scraper moving turns the
drift gate red on a tree whose table and page agree. FR-075 asks for
the move and for the probe that shows the check can still fail; the
fixture is that probe, and it already exists.

**Alternatives:** leaving the scraper on `cli.cpp` and duplicating the
names in a comment there keeps the gate green while checking nothing;
rejected, and it is the exact "0 units examined" failure class the
house rule about gate readings names.

## D-21: The README budget and the page cap

**Decision:** the section `The command line` at `README.md:357` to
`:370` runs 14 of the 25 lines `SECTION_LINE_CAP` allows
(`tools/docs/example_drift.py:156`) and grows by 6 lines to 20. The
quick-reference option table at `README.md:745` to `:766` gains one row
per new long option: `--options`, `--env`, `--threads`,
`--interleave`, `--compare-rules`, `--help`, `--version`, seven rows
(FR-068). `PAGE_LINE_CAP` (`tools/docs/example_drift.py:155`) moves
from the measured 773 to the measured 786, and the gate's own comment
records the arithmetic the way the 024 note at `:146` to `:154` does
(FR-071). `SECTIONS` (`:32` to `:58`) gains nothing: the title
`The command line` already stands at `:44` (FR-073).

**Rationale:** the section keeps its marked block, the snippet region
`command-line-read` of `example/docs/command_line.cpp`, which the drift
tool requires inside the section span. Six lines carry the three
sources, the precedence order, the disclosure line and the two run
options in the page's sentence style.

**Alternatives:** a new capability section title is what FR-073 forbids
and would need a `SECTIONS` entry, a section map entry and an
example source file. Raising `SECTION_LINE_CAPS` for this section is
allowed
by the tool and is the lazier move; it is rejected because the section
fits under the ordinary cap.

## D-22: The guide reference is checked by a C++ test; the README rows by the drift tool

**Decision:** `docs/pages/harness.md` gains the full option reference
inside its section `The command line of an example` (the heading at
`:603`): one row per table row with the name, the argument kind, the
environment variable where the row carries one, and the help text (FR-069,
FR-074). The
two-way comparison against the table is a case of the new
`harness_cli_table_test`: it reads the page from the source directory it
is given as an argument, the way `harness_compare_test` receives
`test/data` at `test/CMakeLists.txt:638` to `:639`, and it compares
name, argument kind, environment name and help text in both directions.
The README option rows stay the drift tool's check (D-20).

**Rationale:** the C++ test reads the table as data through
`detail::optionRows()`, so the table is the source of truth for the
page and the help text equality is checked by the same code that prints
`--help` (FR-018). The drift tool has no C++ view of the table and
would need a second scrape for the environment column.

**Alternatives:** extending the drift tool to the guide page reuses its
row machinery but cannot see the environment flag or the help text
without a deeper scrape of the row shape; rejected as the weaker check.
A recorded run of `--help` compared by the shape fold checks the printed
lines and not the page; rejected as a second oracle for one fact.

## D-23: The example grows its shell region and keeps its snippet region

**Decision:** `example/docs/command_line.sh` region `options` (the
lines at `:14` to `:27`) gains the typed command lines for the new
options, and the fenced block at `docs/pages/harness.md:611` to `:626`
grows to match it byte for byte. The snippet region
`command-line-read` of `example/docs/command_line.cpp` (`:4` to `:14`)
stays as it is, since FR-067 only requires that the example builds and
runs in CI, which `example_run` (`test/CMakeLists.txt:1216` to `:1220`)
already drives. The typed lines name files under `test/data/` nowhere:
they name paths a reader creates, and the block stays illustrative text
that the drift tool compares.

**Rationale:** FR-072 asks for the command lines a reader types in that
region, and the drift tool compares a marked block against its region
byte for byte, so the two move in one task. The snippet-compile check
(`test/CMakeLists.txt:1140` to `:1144`) compiles C++ blocks only, so a
shell block growth leaves it clean.

**Alternatives:** a new example program for the command line would need
a `SECTIONS` map entry for a section that already has one; rejected.
Recording a run of the new options as a sample block would add a fifth
`report_shape` registration for facts the reference table already
states.

## D-24: The run object reaches the writer through an internal call

**Decision:** `detail::writeReport` gains a parameter carrying the
disclosure (E-08 and E-09 of [data-model.md](./data-model.md)): the
resolved option uses and the interleave seed. `ReportData`
(`include/speedgun-ng/benchmark.hpp`) gains no field, and the public
headers gain no symbol. The `run` object is written with the same
`simdjson::builder::string_builder` helpers the rest of the document
uses (`source/harness/json_report.cpp:26`, `:580` to `:599`).

**Rationale:** FR-076 keeps the public surface still, and a field on a
public struct changes an exported layout, which is exactly what moved
the harness `SOVERSION` at `CMakeLists.txt:249` to `:261` for specs/023
and specs/024. The disclosure is a run fact the writer needs and a
library caller of `ReportData` never has, so an internal parameter
carries it. The check the plan ran: the public header diff of this
feature is empty, so `SOVERSION` stays 10 for the harness
(`CMakeLists.txt:261`) and 3 for the archive (`:53`) (FR-076, FR-077).

**Alternatives:** a field on `ReportData` would move the `SOVERSION`
for a fact no library consumer reads; rejected against FR-076. Writing
the `run` object in `cli.cpp` and splicing it into the document text
would put JSON writing outside the writer; rejected.

## D-25: The exit status table and the closing postcondition

**Decision:** the exit status table of
`specs/015-benchmark-harness-core/contracts/cli.md` (the `## Exit
statuses` section at `:73`, the table at `:83`) gains rows for `--help`
and `--version` at 0 and for the source faults at 2 (FR-082). The
closing postcondition of the entry point
(`source/harness/cli.cpp:690`) keeps its predicate over `anyFailed`,
`anyInterrupted` and `reportFailed`, and a new run-wide local is not
needed: a usage fault returns before the loop.

**Rationale:** the postcondition compares the status with the three
run-wide flags, and `test/cli_shape.sh:52` to `:66` requires that
comparison to stand. The new statuses arrive before the loop starts, so
the flags stay untouched.

**Alternatives:** widening the postcondition to cover the usage path
would need a flag the loop never sets and would weaken the interrupt
argument the script's header records.

## D-26: The base-behavior oracle is a checked-in argv matrix

**Decision:** the oracle for FR-003, FR-013 and FR-059 is a checked-in
text, `test/data/base_cli_matrix.txt`, holding the twenty-six command
lines of the base reading with, for each, the status and the fragment of
the fault line that a base binary printed. It was generated from a
binary built at `bd10d76`. A new ctest target, `base_cli_matrix`, runs
the built example binary over those twenty-six command lines and
compares the status and the fragment against the file, with the seven
rows this feature changes marked in the file as changed and compared
against the new expected text. The full base reading stays the record
in `.omo/waves/028-baseline/` and is cited, not copied.

**Rationale:** no ctest target builds two harnesses, so a byte-for-byte
comparison against a master binary has no runner in the tree. Statuses
and fault fragments carry no numbers, so they need no mask and the
comparison is exact. FR-013's mapping of every master name becomes a
literal table in `harness_cli_table_test`, which is the form the scan
of the spec called reachable. For SC-007 the test asserts that a run
with no new option writes a document whose top-level key set is the
base set of `schemaVersion`, `speedgun` and `benchmarks` and that the
console report keeps the base column labels, checked through the shape
fold the docs gate already uses (`tools/docs/report_shape.py`).

**Alternatives:** a golden console report with the numbers masked
re-implements the fold of `tools/docs/report_shape.py` inside a C++
test; rejected as a second comparator. Recording the comparison as a
manual run only leaves FR-003 and FR-059 with no gate at all; rejected.

## D-27: FR-021's other-lane clause has no subject at this head

**Decision:** the test pins the flagged set by equality: the twelve rows
FR-020 names carry the flag, and the fifteen options FR-021 names carry
none. The clause about an option owned by another lane becomes checkable
at rebase, when the rows specs/027 and specs/029 add join the table, and
the rebase task re-reads the flagged set against the grown table. No
test at this head can name a row that does not exist.

**Rationale:** the base table holds twenty rows
(`source/harness/cli.cpp:346` to `:371`), and the jitter-probe rows and
the `--hunt*` rows are absent at `bd10d76`. A set equality over the
flagged rows is the checkable half of FR-021 and it fails the moment a
lane row gains the flag by mistake.

**Alternatives:** writing a test that names a `--hunt` row would assert
a string no row carries and pass vacuously; rejected.

## D-28: FR-053's "every standard library" reaches past the matrix

**Decision:** the pinned vector of D-15 is the oracle. The CI matrix
builds with `gcc` and `clang`, and both use libstdc++ on the runners, so
the claim covers one standard library through the gate set. The
construction carries the rest: the shuffle reads the engine stream and
no distribution, and `std::mt19937_64` is specified by the standard down
to its output stream. The plan records that reading; no test can reach
a library the tree never builds.

**Rationale:** a claim past the gate set needs its argument written
down. The vector pins the order for the library the tree builds; the
absence of any distribution call is what makes the claim travel.

**Alternatives:** adding a libc++ build leg to the matrix for one
requirement is a build-configuration change no requirement asks for;
rejected.

## D-29: FR-081's two-cpu host has no runner

**Decision:** the rule is enforced by construction and by review: no
test names a cpu count, every count comes from `sched_getaffinity`, and
the thread tests use a count read from the mask the way
`detail::selectRunnableCpuCount` (`source/harness/report.cpp:303`)
separates a mask read from the host count. The plan records that the
two-cpu reading is unverified locally: the machines this repository's
CI runs on are four-cpu runners, and no job runs a two-cpu host.

**Rationale:** a claim with no runner is a claim to label, the way the
audit point labels the machine shape. The affinity pattern is the
mechanism that makes the claim plausible, and `ctest -j 2` stays the
runner as the spec's test discipline states.

**Alternatives:** a test that skips when the mask reports fewer cpus
turns the requirement into a skip, which FR-081 forbids; rejected.

## D-30: FR-084's per-error-code duty under a reader that ignores the contents

**Decision:** the reader's answerable codes are the ones the loader can
reach for a document as a whole: an unreadable path, text the parser
refuses, a missing required key, a value of the wrong kind at a stated
pointer, and a key a closed object does not list. The `run` key answers
none of them, because spec D-21 has the reader ignore the object's
contents. The new reader case is one document that carries `run` with
open members and answers no fault (FR-043), and the per-code cases stay
the existing loader tests, which the plan lists as the covering set.

**Rationale:** FR-084's per-code duty cannot reach a code through a key
the reader does not read. Naming the reachable codes and the tests that
cover them is the form of the requirement that can be checked, and the
schema
checker's own walk (`source/harness/json_load.cpp:1060` to `:1140`)
shows the codes it can answer for the new object when a document is
checked against `docs/schema/report-v1.json`.

**Alternatives:** making the reader parse the `run` object to gain
error codes would contradict spec D-21; rejected.

## D-31: The contract form the DBC pair gate reads

**Decision:** every new internal function carries its contract as a
line-start Doxygen command in its declaration comment, `/// \pre` and
`/// \post`, the form `tools/dbc/dbc_doc_gate.py:67` matches, and each
documented kind is enforced by the registered macro of the same kind:
`SG_REQUIRE` for a precondition, `SG_ENSURE` for a postcondition,
`SG_ASSERT` for an assertion (`tools/dbc/macros.yaml`). The doc gate's
scope is `include/speedgun-ng` (`cmake/dbc-gate.cmake:27`), so the
internal declarations carry the lines for the house contract while the
pair gate pairs only the public rows against enforcements found in
`include/speedgun-ng` and `source` (`cmake/dbc-gate.cmake:46` to `:53`).

**Measured scope:** the doc gate reads the Doxygen XML of
`include/speedgun-ng` alone (`cmake/dbc-gate.cmake:27`), and the pair
gate fills `enforced_kinds` only for the rows that XML produced, from
the macro names registered in `tools/dbc/macros.yaml` scanned through
`include/speedgun-ng` and `source` (`tools/dbc/dbc_pair_gate.py`). The
gate at `bd10d76` reports 258 interfaces with 0 gaps and none of them an
internal seam, so the four kinds stand registered in the registry while
the doc line of a declaration under `source/harness/detail/` reaches no
row: those lines are carried by review, and the `SG_REQUIRE` and
`SG_ENSURE` calls of the new definitions still count as enforcements
through the `source` scan.

**Rationale:** FR-085 asks for the form the pair gate reads, and that
form is the line-start command with a non-empty text; a mid-sentence
`\pre` is inlined into a paragraph and does not count. The internal
headers of specs/026 already carry the form
(`source/harness/detail/compare.hpp`), so the new header follows the
house.

**Alternatives:** a prose sentence naming the precondition is what the
gate rejects as an empty or missing section; rejected.

## D-32: The coverage duty runs through seams a test drives directly

**Decision:** every branch the new code can take is reachable from a
call that takes data: the scan takes `argc` and `argv` arrays built in
the test, the gather takes the environment rows and the file text, the
thread list reader and the seed reader answer a value or a fault text,
and `shuffledOrder` takes a count and a seed. No new code path needs a
process environment, a signal, a clock or a cpu count to reach it, and
no `LCOV_EXCL` marker enters `source/harness/` (FR-080, SC-010).

**Rationale:** the house pattern for a branch the environment decides
is the free function at external linkage that a test drives with data
(D-04). The brace-line coverage filter of `cmake/coverage.cmake` stays
as master has it.

**Alternatives:** an end-to-end-only route through `speedgunMain`
leaves the fault texts of the gather reachable only through a forked
child, which the coverage preset measures worse; rejected as the
primary route, kept as the secondary check of D-04.

## D-33: The parallel-work mechanics

**Decision:** the branch rebases onto master and never merges, pushes
once with `--force-with-lease`, and takes at rebase time the next free
minor for the project version and the next free minor for the schema
minor (D-19). The files this feature shares with a parallel lane are
`CMakeLists.txt` (the version line at `:9` and the two `SOVERSION`
lines at `:53` and `:261`), `README.md` (the section and the option
table), `tools/docs/example_drift.py` (`PAGE_LINE_CAP` at `:155`),
`test/CMakeLists.txt` (the new targets), `example/CMakeLists.txt` and
the docs example list.

**Rationale:** three features run at once on three machines, and the
shared files are where a merge commit turns into a conflict; the rebase
rule keeps the base linear.

**Alternatives:** a merge commit keeps both version numbers and leaves
the resolution to whoever builds next; rejected against the house rule.

## Open questions

Four open questions stand: D-22, D-25, D-27, and the percentile column
qualifier specs/026 D-16 left for N7. Each carries its reason under Open
questions in `spec.md`, and the plan builds to the reading stated there.

## Limits the plan labels

- The percentile column qualifier has no requirement behind it, and the
  plan builds nothing for it.
- The project version and the schema minor (FR-078, FR-086) are recorded
  as 0.18.0 and 1.3, and both are read from master again at rebase (D-19).
  A different merge order moves both numbers: PR #46 carries 0.17.0 and
  PR #47 carries 0.19.0.
- FR-053's claim across standard libraries reaches past the gate set, and
  a libc++ build leg is a build-configuration change no requirement asks
  for. The plan argues the claim from the construction and labels it
  (D-28).
- FR-081's two-cpu reading has no runner in the tree and the plan labels
  it unverified (D-29).
- FR-021's clause about an option another lane owns has no subject at
  this head; the check lands at rebase (D-27).
- FR-039 spells every disclosed console entry as `--<name>=<value>`, so
  an environment use prints the option name and the JSON `source` key
  names the source. Printing the variable name in its place is a change
  to FR-039 and FR-040, and the plan takes the spec's form.
