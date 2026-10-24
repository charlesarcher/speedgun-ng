# Feature Specification: Full Command Line

**Feature Branch**: `028-full-command-line`

**Created**: 2026-10-22

**Status**: Draft

**Input**: User description, opening quoted, the full text held by the
specify invocation record:

"Full command line, item N7 of the roadmap
`/workspace/speedgun-ng-harness-roadmap.md` (section 3). Short name:
full-command-line. Target machine: one AMD Zen 5 socket. Lane: harness.
N1, N3 and N5 are done. N7 is next, then N14b, N8 and N9. N7 runs in
parallel with N14a (spec 029) and N13a (spec 027). Goal: give
every speedgun binary one uniform command line. All options go in one
internal table with named values. The table also makes the `--help` text.
N7 adds these items: environment defaults; an option file; `--threads` on
the command line; a seeded random run order (`--interleave`); the N5 rules
file that spec 026 deferred to N7. A run with no new option keeps the
console report and the exit status of master. Its JSON bytes are the same
apart from the `schemaVersion` value. [...]"

## Readings the spec takes

- The spec reads the `run` object and the disclosure console line as one
  statement of the same facts in two places: `run` carries the members
  `interleave` and `options`, the second an array of objects with the keys
  `option`, `source` and `value`, one entry per option taken from a source
  other than the command line, in table order. A repeatable value joins its
  entries with `;`, the separator D-6 states. The console line for a run
  that took `--min-time` from a file and ran `--interleave=7` reads:

  `options from a file or the environment: --min-time=0.5; interleave=7`

  The two places carry the same facts because a reader of the document and
  a reader of the console ask one question (FR-039, FR-040).
- The spec reads the percentile pair fault as a check on the environment
  pair alone, ahead of the precedence rule, because a reader who sets both
  variables made a mistake worth reporting whatever the command line
  carries. The command-line pair keeps the master last-word answer of
  status 0 (D-5, D-27, FR-023).
- The spec reads `--help` beside a usage fault as one event, the first the
  scan meets in left-to-right order, because the scan stops at that event:
  `--help --nonsense=1` prints the help and answers 0 while
  `--nonsense=1 --help` prints the fault and answers 2, and `--help` beside
  `--version` prints the help alone (D-14, FR-017).
- The spec reads an interleave seed as one text read the same in either
  form, because one reader serves both forms: unsigned decimal digits, the
  whole text consumed, the value held in 64 bits, so `-1`, `0x10`, `7x`,
  `true` and a digit string above the 64-bit limit answer the FR-054 fault,
  and `SPEEDGUN_INTERLEAVE=0` means interleaving on with seed 0 (D-4,
  FR-054).

## Run-time binding

This spec binds no spec number, commit, or source line beyond the base it
records here.

- **Spec number**: 028. `master` at `bd10d76` carries `specs/001` through
  `specs/026` and nothing above 026. The remote heads carry the branches
  `020-execution-contexts-core`, `021-metric-formulas`,
  `021-percentiles-raw-capture`, `022-topology-placement`,
  `023-json-reports`, `024-multipass-collection` and
  `026-regression-gate`; none carries a spec directory that `master`
  lacks. `gh pr list` answers an empty list on 2026-10-22. 027 stays
  reserved for N13a, so 028 is the next number free on `master`
  and on every branch. A rebase that finds 028 taken takes the next free
  number and records the change here.
- **Branch base**: `bd10d76174a402ff694883291458bed470002459`
  (2026-10-21), the head of `master` at branch creation: version 0.16.0
  (`CMakeLists.txt:9`), harness `SOVERSION` 10 (`CMakeLists.txt:261`),
  `speedgun-ng` archive `SOVERSION` 3 (`CMakeLists.txt:53`). Specs 021 to
  026 are merged.
- **Names**: every symbol named below is an anchor read at the audit
  point on that commit, and the prompt states the same facts at the same
  lines. A plan re-reads each one on the base it runs against, finds each
  item by its symbol first, and records any drift.

## Decisions

D-1 to D-16 restate recommendations 1 to 12 of the feature prompt
(recommendation 2 is D-2 to D-7). Each is recorded again at the FR that
carries it. D-17 to D-27 are derived at this audit point from the code
facts PC-1 to PC-8, and each names the fact it rests on. D-22, D-25 and
D-27 stand under Open questions, where a reading the prompt leaves open
belongs.

- **D-1 Precedence**: the command line, then the option file, then the
  environment, then the master defaults (C-3, C-4, FR-037, FR-038).
- **D-2 Environment naming**: the variable is `SPEEDGUN_` plus the long
  name in upper case with each `-` changed to `_`, so `--min-time` reads
  `SPEEDGUN_MIN_TIME` (C-3, FR-019).
- **D-3 Environment rows**: only `filter`, `repetitions`, `min-time`,
  `iterations`, `warmup-time`, `counter`, `metric`, `percentiles`,
  `no-percentiles`, `placement`, `threads` and `interleave` carry the
  environment flag. Mode options, output options and the options of other
  lanes carry none (C-3, FR-020, FR-021).
- **D-4 Interleave from the environment**: `SPEEDGUN_INTERLEAVE=<seed>`
  turns `--interleave` on with that seed, read by the one seed reader that
  FR-054 states (C-3, FR-022).
- **D-5 The percentile pair**: `SPEEDGUN_PERCENTILES` and
  `SPEEDGUN_NO_PERCENTILES` set together answer status 2, read on the
  environment pair ahead of the precedence rule of D-1 (C-3, FR-023,
  D-27).
- **D-6 Environment value forms**: a repeatable list uses `;` between
  items; a flag takes `1`, `0`, `true` or `false` and nothing else; an
  unknown `SPEEDGUN_*` variable is ignored (C-3, FR-024, FR-025, FR-026).
- **D-7 One-run environment switch**: `--env=off` turns the environment
  off for one run, and the option stands on the command line alone
  (C-3, FR-028).
- **D-8 Disclosure**: a new top-level JSON object `run` and one console
  line state the options that came from the environment or a file, and the
  seed. `run` carries the two members FR-040 names, and the console line
  carries the same facts in the form FR-039 states. A run with none of
  those writes no `run` object (C-8, FR-039 to FR-041).
- **D-9 Option file form**: `--options=<file>`; each line holds one option
  in its long form without `--`; `#` starts a comment; a blank line is
  ignored; the lines `options=`, `env=`, `help` and `version` answer
  status 2 with the file name and the line number; a second `--options`
  answers status 2 (C-4, FR-029 to FR-032).
- **D-10 Google Benchmark aliases**: the `--benchmark_*` spellings stay
  out of scope; a later item owns them (Scope).
- **D-11 Thread list**: `--threads=<list>` takes a comma list of counts
  and of `min:max` ranges that grow by doubling, replaces the registered
  thread counts of every family, answers status 2 for a count below one or
  a malformed list, sets no cpu cap, discloses a count above the affinity
  count, and keeps the master refusal when a placement preset cannot seat
  a count (C-5, FR-044 to FR-049).
- **D-12 Interleave order**: a hand-written Fisher-Yates over
  `std::mt19937_64` with default seed 0, so the order is the same under
  every standard library; the seed binds only in the `--interleave=<seed>`
  form; the passes and the repetitions of one instance stay together; the
  rows print in registration order; the complexity fit prints after the
  last row of each family in registration order; an interrupt prints the
  rows that ran (C-6, FR-051 to FR-058).
- **D-13 Rules file**: `--compare-rules=<file>`; each line holds one rule
  in the `--compare-rule` grammar of specs/026; `#` starts a comment; the
  file rules follow the `--compare-rule` options of the same source in
  line order; a bad line answers status 2 with the file name and the line
  number. The grammar does not change (C-7, FR-061 to FR-065).
- **D-14 Help and version**: both print on standard output and answer
  status 0, and the help lines come from the table. The scan answers the
  first of `--help`, `--version` or a usage fault that its left-to-right
  walk meets, so a mixed command line carries that event alone (C-2,
  FR-015 to FR-018).
- **D-15 Scan errors**: `opterr` stands at 0 and the option string is
  `":"`, so the parse reports each error itself. An unknown option prints
  one line naming it, with the nearest table name when the edit distance
  is 2 or less. A missing value prints `option '--<name>' needs a value`.
  An ambiguous prefix names its candidates. All three answer status 2
  (C-1, FR-006 to FR-009).
- **D-16 Internal parse**: the parse lives in `source/harness/detail/`,
  answers the parsed set or one usage error, and never calls `std::exit`;
  the environment table and the file reader reach it through its argument,
  so a test runs it with no process environment. Each master option keeps
  its spelling, its value and its exit status (C-1, FR-010 to FR-013).
- **D-17 Argument kinds** (derived from PC-1): `--interleave` is the one
  new option that takes an optional argument, because the prompt requires
  a bare `--interleave` beside the seeded form and the scanner reads a
  separate word as no value for an optional argument. Every other new
  option takes a required argument or no argument. The attached form and
  `SPEEDGUN_INTERLEAVE=<seed>` share the one seed reader of FR-054
  (FR-051, FR-052, FR-054).
- **D-18 One run loop** (derived at the audit from PC-3): the shuffle changes the order
  the loop walks and where the row prints. The line
  `const auto result = runner.run(*entry);` at `source/harness/cli.cpp:645`
  stays, and the interrupt flag keeps its one reading inside the loop, so
  `test/cli_shape.sh` needs no change (FR-060, PC-3, PC-7).
- **D-19 Two print paths** (derived at the audit from PC-3 and AC-7): a run with no
  `--interleave` prints each row as it runs, exactly as master does. A
  shuffled run holds its rows and prints them after the run in
  registration order. Both paths call the same row printer
  (`source/harness/report.cpp:410`), so one row has one printed form
  (FR-056, FR-059).
- **D-20 Table location and the drift gate** (derived at the audit from PC-5): the
  option table moves with the parse into the new internal unit. The
  option-row check of `tools/docs/example_drift.py` reads the table out of
  `source/harness/cli.cpp` today (`:458` to `:466`), so that check follows
  the table to its new file in the same commit, with a liveness probe
  beside it (FR-002, FR-075).
- **D-21 The `run` object in the schema** (derived at the audit from PC-6): the report
  schema leaves the top-level object open today
  (`docs/schema/report-v1.json:9`), so `run` joins the top-level
  properties as an optional key, the required list stays as it stands, and
  the schema names the key while its members stay open, so the two members
  FR-040 states need no entry of their own. The reader ignores the
  contents of the object. A test loads a document that carries the object
  and answers no fault (FR-043).
- **D-23 Rule list order** (derived from D-1 and D-13): the rule list does
  not combine across sources: the highest source that names rules settles
  it, and a rule of a lower source is read before it drops, so its fault
  still reaches the operator. A source that names rules keeps the order
  recommendation 8 states, the order its entries were written: the
  `--compare-rule` options of that source in the order they stand, then
  the rules of the file that that source's `--compare-rules` names, in
  line order. A rules file holds rules alone
  and names no file of its own (FR-038, FR-061, FR-063).
- **D-24 Value assignment** (derived at the audit from hard rule 9): `--options` takes
  290, `--env` 291, `--threads` 292, `--interleave` 293,
  `--compare-rules` 294, `--help` 295 and `--version` 296, each a named
  constant. Values 297 to 319 stay free for the lanes that follow. The
  values 256 to 259 keep their options (FR-003, FR-004).
- **D-26 The version line** (derived at the audit from PC-6): `--version` prints the
  project version the console report already carries
  (`source/harness/report.cpp:317`, the `SG_PROJECT_VERSION` macro written
  at `CMakeLists.txt:231`), on one line, and answers status 0 (FR-016).
## Audit point

Read on 2026-10-22 at `bd10d76` on the machine this feature develops on:
one socket, 16 cores, 32 hardware threads, two L3 groups, AMD
Ryzen 9 9950X3D. No claim in this spec depends on that shape. Every test
reads the cpu count from `sched_getaffinity`, and the thread and
interleave tests pass on a four-cpu CI runner and on a two-cpu host.

The roadmap file the prompt names,
`/workspace/speedgun-ng-harness-roadmap.md`, is absent from this machine,
so no claim here rests on it. The prompt text is the authority for scope.

The six audit-point reports this section draws on sit under
`.omo/waves/028-audit/` in the working tree, machine-local and never
committed. Every anchor below was re-read against the tree at the base
commit.

## Preconditions

### PC-1: the option parse, its table, its values and its exits. PASS.

`speedgunMain` stands at `source/harness/cli.cpp:311`. The long-option
table is the `static option longOptions[]` at `:346` to `:371` and
holds twenty rows in this order: `filter`, `list`, `repetitions`,
`min-time`, `iterations`, `warmup-time`, `dry-run`, `counter`,
`metric`, `catalog`, `percentiles`, `no-percentiles`, `placement`,
`topology`, `json`, `json-raw`, `compare`, `compare-rule`,
`compare-strict`, `metric-method`. The scan is
`getopt_long(argc, argv, "", longOptions, nullptr)` at `:377`, so the
option string carries no short letter; `optind` is reset at `:376` and
`opterr` is never set. The letters `f`, `l`, `r`, `m`, `i`,
`w`, `d`, `c`, `M`, `C`, `p`, `P`, `A`, `T`, `j` and
`J` in the rows are switch codes.

The value constants: `kCompareOptionValue` 257,
`kCompareRuleOptionValue` 258 and `kCompareStrictOptionValue` 259 at
`:69` to `:71`, and `kMetricMethodOption` 256 as a local constant at
`:345`.

Four helpers end the process: `rejectPercentileList` exits at `:80`,
`parseCount` at `:132`, `parseSeconds` at `:152`,
`parsePositiveSeconds` at `:174`. The scan and the entry point answer
status 2 with `return 2` at `:431` (`--json` with an empty path),
`:445` (`--json-raw` with a word outside `chunks` and `none`),
`:461` (`--metric-method` with a word outside the three), `:479`
(`--compare` with no comma or an empty side), `:492`
(`--compare-rule` with a parse fault), `:506` (the `default` arm of
the switch, the unknown-option line), `:517` (`--filter` with an
invalid expression), `:542` (`--placement` with a name outside the
five) and `:562` (`--topology` against an unreadable layout).
`detail::metricNameFailure` answers 2 at `:297`. The unknown-option
text is `unknown option; see the option table of` plus the contract path,
printed at `:503`. The entry point closes with
`SG_ENSURE((status != 0) == (anyFailed || anyInterrupted || reportFailed),`
at `:690`.

The scanner resolves an unambiguous prefix of a long name, so the new names
make `--i`, `--t`, `--compare-r`, `--compare-ru` and
`--compare-rul` ambiguous (FR-013).


### PC-2: the thread-count surface. PASS.

`BenchmarkHandle::threads(int)` is declared at
`include/speedgun-ng/benchmark.hpp:2314` with a precondition that the
count stands at 1 or above, and defined at `source/harness/registry.cpp:535`,
where `SG_REQUIRE(count >= 1, ...)` precedes the store. `threadRange`
(`benchmark.hpp:2351`, `registry.cpp:613`) and `denseThreadRange`
(`benchmark.hpp:2370`, `registry.cpp:630`) sit beside it. All three
replace `RegistryEntry::threadCounts`
(`source/harness/detail/internal.hpp:199`), whose default `{1}` keeps a
family that called no thread method on one context. The doubling growth is
`detail::buildThreadCounts` (`internal.hpp:315`, `registry.cpp:574`):
one count per doubling from the low bound, each at or below the high
bound.

Expansion crosses the argument lists of a family with its thread counts at
`source/harness/family.cpp:404` and `:425`. `instanceName`
(`family.cpp:223`) appends `/threads:` and the count only when the
count stands above 1 (`:246`), so a count of one names no segment.
`checkFamilySize` (`family.cpp:290`) warns above `kMaxFamilySize`
(100, `internal.hpp:35`), keeps every instance, and the run continues.

The refusal a preset answers for a count it cannot seat is
`detail::placementFitStatus` (`source/harness/placement_preset.cpp:173`,
called at `cli.cpp:607`): one line naming the preset, the place count and
the requested count, and status 2, before any run. The `std::exit(2)` this
feature keeps sits at `family.cpp:382`, the refusal of a family preset
name outside the five.

### PC-3: the run loop, the fit print and the interrupt path. PASS.

The loop runs from `cli.cpp:633` to `:668`. Its body holds the comment
that names the loop at `:631`, the fit print at the family boundary
(`runner.fitForFamilyEnd(entry)` at `:637`), the run at
`const auto result = runner.run(*entry);` (`:645`), the row print
(`detail::printResult(result)` at `:646`), the single interrupt-flag
reading (`:652`) and the break at `:666`. The last family's fit prints
after the loop at `:672`. `handleInterrupt` stands at `:37`, and
`std::signal(SIGINT, handleInterrupt)` at `:617` installs it; an
interrupt leaves status 1.

`test/cli_shape.sh` locates the loop with the awk range that starts at the
line holding `runner.run(*entry)` (`test/cli_shape.sh:32`), counts the
`interruptFlag().load()` readings inside the range and requires exactly
one (`:39`), and requires the `bool interrupted =` spelling (`:47`). A
change that moves the print step into a second loop, or that adds a loop
with its own flag reading, breaks that gate.

### PC-4: the comparison surface the rules file reuses. PASS.

`CompareRule` stands at `include/speedgun-ng/benchmark.hpp:701` and
`CompareOptions` at `:766`. `parseCompareRule` is exported at `:2589`
and answers `std::expected<CompareRule, std::string>`, so its fault text
already names the field and the text it read; the CLI line at `cli.cpp:492`
adds the option name and nothing else. `compareReports` stands at `:2618`,
and the default rule of specs/026 FR-033 stands inside it
(`source/harness/detail/compare.hpp:49`). `detail::runCompareMode` is
declared at `source/harness/detail/compare.hpp:77` and called at
`cli.cpp:525`, ahead of the expansion, so compare mode runs no benchmark.

specs/026 D-11 records the deferral this feature closes: "option files,
environment defaults and a rules file belong to N7 ... N7 reads a rules
file through the same parse call (Scope)"
(`specs/026-regression-gate/spec.md:137` to `:140`). The same spec keeps
"the full command line for the gate, option files, environment defaults
and a rules file (N7)" out of its own list (`:1071`).

### PC-5: the documentation surface and its gates. PASS.

`README.md` runs 773 lines and `PAGE_LINE_CAP` reads 773
(`tools/docs/example_drift.py:155`), so the page stands exactly at its
cap. `SECTION_LINE_CAP` reads 25 (`:156`) and `SECTION_LINE_CAPS`
(`:161`) holds no entry for the section this feature extends. `SECTIONS`
(`:32` to `:57`) already lists `The command line`, so this feature adds
no section title and no section map entry (`:63`).

The section runs from `README.md:357` to `:370`, fourteen lines of a
25-line budget, and its snippet comes from the region `command-line-read`
of `example/docs/command_line.cpp` (`README.md:359`). The command lines
a reader types sit in the region `options` of
`example/docs/command_line.sh`. The quick-reference option table runs from
`README.md:745` to `:766` and holds one row per long option.

The drift gate reads the long-option table out of
`source/harness/cli.cpp` with `cli_options()` (`:458` to `:466`) and
reports two findings when the two lists part: an option row that names no
long option (`:685`) and a long option with no option row (`:691`). The
expression it matches, `option\s+\w+\s*\[\]\s*=\s*\{(.*?)\n\s*\};`,
matches a `constexpr option kLongOptions[]` row list, so the table can move
and keep the check, provided the path follows it (D-20).

`docs/pages/harness.md` runs 1340 lines and carries no option reference
today; its last capability section is `## Multi-pass collection` at
`:1217`. The page cap binds the main page alone; the User-guide pages
(`:382`) take the substance scan.

### PC-6: the report document and its version. PASS.

The writer stamps `kSchemaVersion` as `1.1`
(`source/harness/json_report.cpp:31`) and writes the key at `:583`. The
top-level keys are `schemaVersion`, `speedgun` and `benchmarks`
(`docs/schema/report-v1.json:7`, `:8`), and the top-level object is open:
`additionalProperties` set to `true` at `:9`. A new top-level `run`
object therefore loads today as an unknown key, which is the fact D-21
rests on. The console version line prints `speedgun-ng` with
`SG_PROJECT_VERSION` and the build type at `source/harness/report.cpp:317`,
and the macro is written at `CMakeLists.txt:231`.

### PC-7: the test surface the message changes reach. PASS.

`test/source/harness_cli_test.cpp:806` expects `unknown option` for
`--nonsense=1`, and `:810` expects the same text for `--metric` with no
value, because the missing-argument answer falls into the `default` arm at
`cli.cpp:506`. `test/source/harness_cli_placement_test.cpp:449` matches
the same text. The two targets are registered at `test/CMakeLists.txt:651`
to `:654` and `:695` to `:702`. No test under `test/` calls `setenv`,
`putenv` or `getenv`, so the environment seam this feature needs has no
existing call to remove and no existing pattern to follow.

### PC-8: the number state and the version state. PASS.

`master` carries `specs/001` to `specs/026`, version 0.16.0
(`CMakeLists.txt:9`), harness `SOVERSION` 10 (`:261`) and archive
`SOVERSION` 3 (`:53`). No branch and no open pull request carries a spec
directory 027 or above. The public surface this feature touches is the
command line of the harness entry point, and `speedgunMain` keeps its
signature, so the ABI check of hard rule 12 answers "no change" and the
harness `SOVERSION` stays at the value `master` carries, 10 at the base
(FR-076).



## Roadmap position

N7 of the harness roadmap. It sits in the harness lane: N1 percentiles and
raw capture landed as spec 021, N3 JSON reports landed as spec 023, N5 the
regression gate landed as spec 026. N7 runs now, and N14b, N8 and N9
follow it.

N7 is the item that makes the command line one thing: each of specs/015
through specs/026 added its own rows
to one scanner by hand, and this feature moves all of them into one table
that also produces the help text. It closes the deferral specs/026 D-11
recorded for the rules file, and it opens the environment and file sources
that a CI job needs to drive the regression gate.

N13a (spec 027) and N14a (spec 029) run in parallel.
Neither is a dependency: N7 needs specs/023 and specs/026, both merged.

## Parallel work and the version rule

- **Shared files**: the project `VERSION` and the two `SOVERSION` lines
  in `CMakeLists.txt`, `README.md` with its quick-reference table and its
  option table, `PAGE_LINE_CAP` and `SECTIONS` in
  `tools/docs/example_drift.py`, `example/CMakeLists.txt` and the
  `example_run` list, the target lists of `test/CMakeLists.txt`, the
  option table of `source/harness/cli.cpp` until D-20 moves it,
  `test/source/harness_cli_table_test.cpp`, and
  `docs/schema/report-v1.json`. `master` carries no changelog file.
- **Option value ranges**: `master` holds 256 to 259. N13a holds 260 to
  269, N14a 270 to 279, N14a-2 280 to 289. N7 takes 290 to 319 and uses
  290 to 296 (D-24). N7 moves 256 to 259 into the table and keeps each
  spelling, value and behavior (FR-003).
- **One uniqueness test file**: `test/source/harness_cli_table_test.cpp`
  is the only option-uniqueness test. No lane has made it yet, so N7 makes
  it and the later lanes extend it (FR-079).
- **Schema minors**: N13a takes 1.2, N7 takes 1.3, N14a takes 1.4. At
  rebase, the next free minor is the one to take, and each lane adds the
  keys of the lanes that merged before it to
  `docs/schema/report-v1.json`.
- **Overlap with N13a and N14a**: those lanes add the jitter-probe
  options and the `--hunt*` rows. If either merges first, N7 rebases and
  moves their rows into the table with their values and their behavior
  intact, and adds their options to the option reference page and the
  README option table. If N7 merges first, they rebase onto the table.
  Neither lane gains an environment variable from N7 (FR-021).
- **Conflict rule**: rebase onto the new `master`; no merge. Keep the
  lines of every feature in each shared list, take the next free minor
  version and schema minor, set `PAGE_LINE_CAP` to the measured page
  length through a recorded decision, run the full gate set, and push once
  with `--force-with-lease`, showing `git ls-remote origin <branch>`
  after the push.
- **Version rule**: the fleet carries three numbers now. PR #46 takes
  0.17.0 and PR #47 takes 0.19.0, so this branch takes 0.18.0, the value
  `CMakeLists.txt:9` carries. The planned value is a reservation: at
  rebase, read `CMakeLists.txt` on `master` and take the next free minor.
  The harness `SOVERSION` stays at the value `master` carries unless the
  ABI check finds a public change, and the archive `SOVERSION` stays 3
  (FR-076 to FR-078).

## User Scenarios & Testing *(mandatory)*

### User Story 1 - One table, one help text, and a run that behaves as master does (Priority: P1)

A developer types `--help` into any speedgun binary and reads every
option in one list, each with the words the option table carries. A
developer who types no new option gets the report and the exit status that
master produced for the same command line.

**Why this priority**: the table is the substrate every other capability
reads, and the unchanged run is the promise the whole feature is measured
against. Without it the feature is a set of new flags beside an old list.

**Independent Test**: run the suite with a fixed command line on this
branch and on `master`, compare the console text and the exit status byte
for byte, then compare the two JSON documents and find the
`schemaVersion` line as the only difference. Then run `--help` and
`--version` and read the exit status.

**Acceptance Scenarios**:

1. **Given** a built suite, **When** the binary runs `--help`, **Then**
   standard output carries one line per option table row in table order,
   each naming the option and its help text, and the status is 0.
2. **Given** a built suite, **When** the binary runs `--version`, **Then**
   standard output carries the project version on one line and the status
   is 0.
3. **Given** a command line master runs today, **When** the same command line
   runs on this branch, **Then** the console report and the exit status
   match master, and the JSON document differs in `schemaVersion` alone.
4. **Given** the option table, **When** the uniqueness test reads it,
   **Then** no name repeats, no value repeats, and the values 256 to 259
   still name `--metric-method`, `--compare`, `--compare-rule` and
   `--compare-strict`.

---

### User Story 2 - Set the run from the environment and from a file, and see where each setting came from (Priority: P1)

A CI job sets `SPEEDGUN_MIN_TIME` and `SPEEDGUN_THREADS` once and runs
many binaries. Another job keeps its settings in a file and passes
`--options=<file>`. A developer who overrides one setting on the command
line sees that the command line won. Every run that took a setting from a
source other than the command line says so once on the console and once in
the JSON document.

**Why this priority**: this is the capability the roadmap asked for, and
the disclosure is what makes a setting taken from the environment
debuggable: the line names the source the value came from.

**Independent Test**: set one option in each of the three sources with
three different values, run once, and read which value took effect, the
console line and the `run` object.

**Acceptance Scenarios**:

1. **Given** `SPEEDGUN_MIN_TIME=0.5`, **When** the run starts with no
   `--min-time`, **Then** the run uses 0.5 seconds and the disclosure
   names `--min-time` with the source `environment`.
2. **Given** `SPEEDGUN_MIN_TIME=0.5` beside `--min-time=0.2`, **When**
   the run starts, **Then** the run uses 0.2 seconds and the disclosure
   names the command line as the source of that value.
3. **Given** `SPEEDGUN_COUNTER=machine/monotonic` beside
   `--counter=machine/process_cpu`, **When** the run starts, **Then** the
   run carries one counter column, the one the command line named.
4. **Given** an option file with a `min-time=0.5` line beside
   `SPEEDGUN_MIN_TIME=0.9`, **When** the run starts with
   `--options=<file>`, **Then** the run uses 0.5 seconds.
5. **Given** a run where every option came from the command line and no
   `--interleave` stands given, **When** the run writes its document,
   **Then** the document holds no `run` object and the console prints no
   disclosure line.

---

### User Story 3 - One clear line for one wrong setting, whichever source carried it (Priority: P1)

A developer mistypes `--min-tim=5`, writes `min time=0.5` in an option
file, sets `SPEEDGUN_THREADS=0`, or points `--compare-rules` at a file
with a broken rule. Each mistake answers one line that names the source and
the value, and the run stops with status 2 before any benchmark runs.

**Why this priority**: the feature multiplies the places a bad value can
come from, and hard rule 3 binds every one of them to one line and status
2. A wrong environment variable that runs silently costs a CI job its
meaning.

**Independent Test**: run each fault form from each source, and check the
status, the line count across both streams, and that the line names the
source and the value.

**Acceptance Scenarios**:

1. **Given** an option name that stands in no table row, **When** the run
   starts, **Then** one line names the option, names the nearest table name
   when the edit distance is 2 or less, and the status is 2.
2. **Given** `--metric` with no value, **When** the run starts, **Then**
   one line reads `option '--metric' needs a value` and the status is 2.
3. **Given** `--i=10`, **When** the run starts, **Then** one line names
   `--iterations` and `--interleave` as the candidates and the status is
   2.
4. **Given** `SPEEDGUN_WARMUP_TIME=soon`, **When** the run starts,
   **Then** one line names `SPEEDGUN_WARMUP_TIME` and its value, and the
   status is 2.
5. **Given** an option file whose third line reads `min time=0.5`,
   **When** the run starts, **Then** one line names the file, the line
   number and the text, and the status is 2.
6. **Given** a rules file whose second line reads `time@median:lower:`,
   **When** the run starts, **Then** one line names the file, the line
   number and the rule fault, and the status is 2.



---

### User Story 4 - Run every benchmark at the thread counts the command line names (Priority: P2)

A developer runs `--threads=1,8` and gets every family at one context and
at eight, with the one-context rows keeping their untouched names and the
eight-context rows carrying `/threads:8`. A developer writes
`--threads=1:16` and gets the doubling set.

**Why this priority**: it is one of the five items the roadmap named, and
it rides on a thread-count surface that already exists, so it replaces one
field and adds no mechanism.

**Independent Test**: run a suite with `--threads=1,<n>` where `<n>` sits
at or below the affinity cpu count and above one, and read the instance
names and the row counts.

**Acceptance Scenarios**:

1. **Given** a suite of registered families, **When** the run starts with
   `--threads=1,4`, **Then** every family runs at one context and at four,
   and the four-context names carry `/threads:4`.
2. **Given** a family that called `threadRange(2, 8)`, **When** the run
   starts with `--threads=1`, **Then** the family runs at one context
   alone.
3. **Given** `--threads=0`, **When** the run starts, **Then** one line
   names the option and the text, and the status is 2.
4. **Given** `--threads=1:8`, **When** the run starts, **Then** the counts
   are 1, 2, 4 and 8, the growth `detail::buildThreadCounts` states.
5. **Given** a count above the affinity cpu count, **When** the run starts,
   **Then** the run proceeds and the console discloses the count.

---

### User Story 5 - Run the instances in a seeded random order and read the rows in registration order (Priority: P2)

A developer who suspects that one benchmark warms the machine for the next
one runs `--interleave=7`. The run order changes, the seed is recorded,
and the report still reads in registration order, so a diff against an
earlier run still lines up.

**Why this priority**: it changes the run order, so it needs the
determinism and the print-order rules settled before it is useful, and it
sits behind the table and the sources in value.

**Independent Test**: run a suite of ten or more instances twice with
`--interleave=7` and once with `--interleave=8`, and compare the run
order, the printed row order and the fit placement.

**Acceptance Scenarios**:

1. **Given** a suite of ten instances, **When** the run starts twice with
   `--interleave=7`, **Then** both runs use the same instance order.
2. **Given** the same suite, **When** the run starts with
   `--interleave=8`, **Then** the instance order differs from the order
   `--interleave=7` produced.
3. **Given** a shuffled run, **When** the rows print, **Then** they print
   in registration order and the complexity fit of a family prints after
   the last row of that family.
4. **Given** a shuffled run, **When** SIGINT arrives after three instances,
   **Then** the rows that ran print in registration order and the status is
   1.
5. **Given** `--interleave` with no `=`, **When** the run starts,
   **Then** the seed is 0 and the console states it.
6. **Given** `--interleave=7x`, **When** the run starts, **Then** one
   line names the option and the text and the status is 2.

---

### User Story 6 - Give the comparison its rules from a file (Priority: P2)

A CI job keeps its regression rules in one file in the repository and runs
`--compare-rules=rules.txt` beside `--compare=<baseline>,<contender>`.
The verdict matches the verdict the same rules give as repeated
`--compare-rule` options.

**Why this priority**: it closes the deferral specs/026 D-11 recorded, and
the rule grammar and the comparison already exist, so the new part is one
reader and one order rule.

**Independent Test**: write the same rules twice, once as repeated
`--compare-rule` options and once as a file, run both over one report
pair, and compare the table and the status.

**Acceptance Scenarios**:

1. **Given** a report pair and two rules, **When** the rules come from a
   file, **Then** the table and the status match the run where the same
   rules came from options.
2. **Given** one `--compare-rule` option and one rules file with a second
   rule, **When** the run starts, **Then** the option rule is applied
   before the file rule.
3. **Given** a rules file with a `#` comment line and a blank line,
   **When** the run starts, **Then** neither line yields a rule.
4. **Given** a rules file path that cannot be read, **When** the run
   starts, **Then** one line names the path and the status is 2.
5. **Given** one `--compare-rule` option beside an option file with a
   `compare-rule=` line, **When** the run starts, **Then** the table
   carries the command-line rule alone: the command line settles the rule
   list, and the file rule is read before it drops (D-23).

---

### User Story 7 - Read the help text, the version, and the option reference (Priority: P3)

A reader opens the main page, reads the command-line section, copies the
block, and runs the example. A reader who needs the whole reference finds
it on the harness page, where every table row is listed with its argument
kind, its environment variable and its help text.

**Why this priority**: the documentation is a constitutional obligation of
every user-visible change, and it follows from the table instead of
leading it.

**Independent Test**: run the drift gate and the page-versus-table test,
and run the example through `example_run`.

**Acceptance Scenarios**:

1. **Given** the main page, **When** the drift gate reads it, **Then** the
   command-line section runs at most 25 lines, its snippet comes from the
   example it names, and the page sits at or below `PAGE_LINE_CAP`.
2. **Given** the option table and the harness page, **When** the page test
   compares them, **Then** every table row appears on the page and no
   option on the page stands outside the table.
3. **Given** the README option table, **When** the drift gate compares it
   with the option table, **Then** every long option has a row and every
   row names a long option.

### Edge Cases

- An option name given as an unambiguous prefix of one table row: the
  option resolves to that row, and the disclosure and the help text name
  the full spelling.
- An option name given as an ambiguous prefix: one line names the
  candidates in table order, and the status is 2.
- `--help` beside a usage fault: the event that stands first in the
  command line answers, so `--help --nonsense=1` prints the help with
  status 0 and `--nonsense=1 --help` prints the fault with status 2.
- `--help` beside `--version`: the help prints alone, with status 0.
- An option given twice on the command line: the last word wins for a
  single-value option, and both entries stand for a repeatable option, the
  master behavior.
- A repeatable option set in two sources: the higher source replaces the
  lower list (FR-038). A rule set in two sources: the highest source
  that names rules settles the rule list, and a lower rule is read before
  it drops, so a bad one still answers its fault line (D-23).
- An option file that holds the same option twice: the rule of the command
  line applies inside one source.
- An option file that cannot be read: one line naming the path, status 2.
- An option file that is empty: no option is set, and the run proceeds on
  the other sources.
- An option file whose last line carries no newline: the line reads whole.
- A line in an option file that names `--env`, `--help`, `--version` or
  `--options`: one line naming the file, the line number and the rejected
  name, status 2.
- A comment marker that follows text on the same line: the whole line is
  the option text, because the marker starts a line.
- `SPEEDGUN_THREADS` set to `;1`: one line names the variable and the
  value, status 2.
- `SPEEDGUN_INTERLEAVE` set to `-1`, `0x10`, `7x` or `true`: one line
  names the variable and the value, status 2. `SPEEDGUN_INTERLEAVE=0`
  turns interleaving on with seed 0.
- `SPEEDGUN_NOT_AN_OPTION` set to anything: ignored.
- A variable that names an environment row with an empty value: one line
  names the variable and the value, status 2.
- `--min-time=1e-12` and `--min-time=4e-10`: the cast to nanoseconds leaves
  nothing, so each answers the min-time fault line, status 2, while
  `--warmup-time=0` keeps its answer.
- A number word that opens with a blank, such as `repetitions= 3` from the
  command line, the environment or a file line: the reader takes the whole
  text, so the value answers its own fault line, status 2.
- `SPEEDGUN_PERCENTILES` and `SPEEDGUN_NO_PERCENTILES` set together beside
  a command line that overrides one half: one line names both variables,
  status 2. Two percentile options on the command line alone: the last one
  wins and the status stays 0, the master behavior.
- `--env=off` with the environment set: the environment is ignored for
  that run, and the disclosure names no environment source.
- `--env=on`: one line naming the option and the word, status 2.
- A `--threads` list with a repeated count: the count stands once, and the
  duplicate-instance rule of expansion drops the second instance.
- A `--threads` list that pushes a family above `kMaxFamilySize`: the
  master warning prints, every instance is kept, and the run continues.
- A `--threads` count above the affinity cpu count beside a placement
  preset that cannot seat it: the preset refusal of PC-2 answers, one line
  and status 2.
- A `--threads` range whose low bound stands above its high bound: one
  line names the option and the text, status 2.
- `--interleave` beside `--list`, `--catalog`, `--topology` or a
  compare mode: the mode answers as master does and the shuffle changes
  nothing.
- `--interleave` with a repetition count above one: the repetitions of one
  instance stay together.
- A shuffled run where SIGINT arrives before the first instance finishes:
  no row prints, the status is 1, and the seed is stated.
- A rules file with a rule that names no benchmark: the specs/026
  filter-no-match fault answers, one line and status 2.
- A rules file beside a `--compare-rule` fault: both rules reach the
  parser, and the option fault answers first because the scan reads the
  command line before the gather reads a file.
- A `--compare-rules` file beside a `--compare-rule` option of the same
  source: both lists stand, the option rule first, because one source
  keeps the order its entries were written (D-23).
- An option file with a `compare-rules=` line: the line is read, because
  the forbidden list of FR-031 stays at `options`, `env`, `help` and
  `version`.
- A `run` object in a document a later reader loads: the reader takes it
  and answers no fault.
- A run that takes one option from the environment and also runs
  `--interleave`: the disclosure line and the `run` object name both.



## Requirements *(mandatory)*

### One option table (C-1)

- **FR-001**: The system shall hold every long option in one internal
  table, and each row shall carry the long name, the internal option
  value, the argument kind, the environment flag, the help text and the
  requirement that owns the row.
- **FR-002**: The option scanner shall read that table, and no option shall
  be recognized from any other list (D-20).
- **FR-003**: The table shall carry every long option `master` carries
  today with its spelling, its internal value and its behavior unchanged,
  the values 256 to 259 among them (PC-1).
- **FR-004**: Every new option shall take a named constant inside the
  reserved range 290 to 319, and no two rows shall share a name or a value
  (D-24).
- **FR-005**: The scanner shall accept no short option: the option string
  shall carry no short letter, and the letters the `master` rows hold
  shall stay internal switch codes (PC-1).
- **FR-006**: The scanner shall set `opterr` to 0 and use the option
  string `":"`, so the parse reports every error itself (D-15).
- **FR-007**: An option name that matches no table row shall print one line
  naming that name and answer status 2, and the line shall name the nearest
  table name when the edit distance to it is 2 or less.
- **FR-008**: An option given with no value shall print one line reading
  `option '--<name>' needs a value` and answer status 2 (D-15, PC-7).
- **FR-009**: An option name that is an ambiguous prefix of table rows
  shall print one line naming the candidate names in table order and answer
  status 2 (D-25).
- **FR-010**: The parse shall live in `source/harness/detail/`, shall
  answer the parsed option set or one usage error, and shall call
  `std::exit` nowhere in the parse path (D-16).
- **FR-011**: The entry point shall print the usage error the parse answers
  and return status 2, and the four `std::exit(2)` calls of the reject
  helpers shall be gone (PC-1).
- **FR-012**: The parse shall take the environment and the option-file text
  in through its argument, so a test drives every source with no process
  environment and no `setenv` call (D-16, PC-7).
- **FR-013**: A test shall check that each full `master` option name
  resolves to the option it resolved to on `master`, and shall check that
  `--i`, `--t`, `--compare-r`, `--compare-ru` and `--compare-rul`
  answer the ambiguity line against the final table.
- **FR-014**: The `std::exit(2)` of the expansion's preset-name refusal at
  `source/harness/family.cpp:382` shall stay as `master` has it (PC-2).

### Help and version (C-2)

- **FR-015**: `--help` shall print one line per table row in table order,
  each line the option name with its argument kind and its help text, and
  shall answer status 0 (D-14).
- **FR-016**: `--version` shall print the project version on one line and
  answer status 0 (D-26).
- **FR-017**: `--help` and `--version` shall print on standard output,
  shall stop the run before any benchmark runs, and shall stand on the
  command line alone. The scan shall answer the first of `--help`,
  `--version` or a usage fault that its left-to-right walk meets, so
  `--help --nonsense=1` prints the help and answers status 0 while
  `--nonsense=1 --help` prints the fault and answers status 2, and
  `--help` beside `--version` shall print the help alone.
- **FR-018**: The help text shall come from the table, and no second list
  of option descriptions shall exist in the harness.

### Environment defaults (C-3)

- **FR-019**: A row marked with the environment flag shall take its default
  from the variable `SPEEDGUN_` plus the long name in upper case with each
  `-` changed to `_` (D-2).
- **FR-020**: The rows marked with the environment flag shall be exactly
  `filter`, `repetitions`, `min-time`, `iterations`, `warmup-time`,
  `counter`, `metric`, `percentiles`, `no-percentiles`, `placement`,
  `threads` and `interleave` (D-3).
- **FR-021**: A mode option, an output option and an option owned by another
  lane shall carry no environment variable; `--list`, `--catalog`,
  `--topology`, `--dry-run`, `--json`, `--json-raw`, `--compare`,
  `--compare-rule`, `--compare-strict`, `--compare-rules`,
  `--metric-method`, `--options`, `--env`, `--help` and `--version`
  are among them (D-3).
- **FR-022**: `SPEEDGUN_INTERLEAVE=<seed>` shall turn `--interleave` on
  with that seed, read by the seed reader of FR-054 so that both forms
  accept the same text (D-4).
- **FR-023**: `SPEEDGUN_PERCENTILES` and `SPEEDGUN_NO_PERCENTILES` set
  together shall print one line naming both variables and answer status 2.
  The check shall read the environment pair alone, ahead of the precedence
  rule of FR-037, so a command line that overrides one half leaves the
  fault standing. Two percentile options on the command line shall keep the
  `master` answer: the last one wins and the status stays 0 (D-5, D-27).
- **FR-024**: A repeatable option read from the environment shall split its
  value on `;` into one entry per item (D-6).
- **FR-025**: A flag read from the environment shall accept `1`, `0`,
  `true` and `false`, and any other text shall print one line naming the
  variable and its value and answer status 2. A variable of any row whose
  value stands empty shall answer that one line ahead of the row's own
  reader, and the line shall name the variable (D-6).
- **FR-026**: A variable that starts with `SPEEDGUN_` and matches no row
  marked with the environment flag shall be ignored (D-6).
- **FR-027**: An environment value that the option's own reader rejects
  shall print one line naming the variable and the value and answer
  status 2, and that answer stands when a higher source names the same
  option with a value that reads (D-6, SC-003).
- **FR-028**: `--env=off` shall turn the environment off for one run, any
  other word shall print one line naming the option and the word and answer
  status 2, and the option shall stand on the command line alone (D-7).

### Option file (C-4)

- **FR-029**: `--options=<file>` shall read one option per line in the long
  form without the leading `--` (D-9).
- **FR-030**: A line whose first non-space character is `#` shall be a
  comment, and a blank line shall set no option (D-9).
- **FR-031**: A line that names `options`, `env`, `help` or `version`
  shall print one line naming the file, the line number and the rejected
  name, and answer status 2 (D-9).
- **FR-032**: A second `--options` option shall print one line naming the
  option and answer status 2. An `--options` word whose path stands empty
  shall print one line naming the option and answer status 2 (D-9).
- **FR-033**: A line the parse cannot read shall print one line naming the
  file, the line number and the text, and answer status 2, and that answer
  stands when a higher source names the same option with a line that reads
  (D-22).
- **FR-034**: A file the system cannot read shall print one line naming the
  path and answer status 2.
- **FR-035**: An option file line shall set its option under the same value
  rules as the command line, and a repeatable option shall accumulate
  within one file in line order.
- **FR-036**: A line that starts with `-`, and a line for a
  required-argument option that carries no `=`, shall print one line
  naming the file, the line number and the text, and answer status 2
  (D-22).

### Precedence and disclosure (C-8)

- **FR-037**: The value of an option shall come from the highest source
  that sets it, in the order command line, option file, environment,
  `master` default. Every value from every source shall reach the reader
  of its option, whether or not a higher source names the option, so the
  fault of a bad value reaches the operator before any drop (D-1,
  FR-027, FR-033, FR-046, FR-054).
- **FR-038**: A repeatable option set by a higher source shall replace the
  whole list of a lower source, and shall append nothing to it. The
  comparison rule list shall not combine across sources: the highest
  source that names rules shall settle it. A rule of a lower source shall
  reach the rule parser before it drops, so the fault of a bad rule
  reaches the operator whatever source spelled it, and a source shall
  keep the order of the rules it contributes (D-1, D-23, FR-063).
- **FR-039**: When any option took its value from the environment or an
  option file, or when `--interleave` stands given, the run shall write
  one top-level JSON object `run` and print one console line carrying the
  same facts. That line shall open with the words
  `options from a file or the environment:`, then carry one
  `--<name>=<value>` entry per disclosed option in table order, followed
  by `interleave=<seed>` when `--interleave` stands given, with `; `
  between entries. For a run that took `--min-time` from a file and ran
  `--interleave=7` the line reads:

  `options from a file or the environment: --min-time=0.5; interleave=7`
- **FR-040**: The `run` object shall carry two members: `interleave`, an
  unsigned integer present only when `--interleave` stands given, and
  `options`, an array of one object per option taken from a source other
  than the command line, in table order, each object holding the string
  keys `option`, `source` and `value` for the option name, the source that
  set it and the value that took effect. A repeatable value shall join its
  entries with `;`, the separator D-6 states. The console line shall carry
  the same facts (D-8).
- **FR-041**: A run where no option came from the environment or an option
  file and no `--interleave` stands given shall write no `run` object and
  print no disclosure line.
- **FR-042**: The console disclosure line shall print after the version line
  and before the first benchmark row.
- **FR-043**: The report schema shall describe `run` as an optional
  top-level key whose members stay open, shall keep the top-level required
  list unchanged, and the reader shall accept a document that carries the
  object and answer no fault (D-21, PC-6).

### Threads (C-5)

- **FR-044**: `--threads=<list>` shall set the thread counts of every
  benchmark family, in place of the counts the family registered (D-11).
- **FR-045**: The list shall take a comma-separated set of counts and of
  `min:max` ranges, and a range shall expand by doubling from its low
  bound to at most its high bound, the growth `detail::buildThreadCounts`
  states (D-11, PC-2).
- **FR-046**: A count below one, a malformed item, and a range whose low
  bound stands above its high bound shall each print one line naming its
  source and the text and answer status 2, whatever source named the list
  and whatever a higher source names beside it (D-11).
- **FR-047**: The option shall set no cpu cap: a count above the count of
  cpus the process affinity allows shall run, and the console shall
  disclose the count (D-11).
- **FR-048**: The instance names the option produces shall follow the
  `master` name rule: no `/threads:` segment for a count of one, and
  `/threads:<n>` for a count above one (PC-2).
- **FR-049**: When a placement preset cannot seat a count, the run shall
  answer the `master` behavior: one line naming the preset, the place
  count and the requested count, and status 2 before any run (PC-2).
- **FR-050**: The option shall apply to every family whatever thread call
  the family made: `threads`, `threadRange` and `denseThreadRange` all
  yield to it (PC-2).

### Interleaving (C-6)

- **FR-051**: `--interleave` shall run the selected instances in a seeded
  random order, `--interleave=<seed>` shall set the seed through the seed
  reader of FR-054, and the bare form shall use seed 0 (D-12, D-17).
- **FR-052**: The seed shall bind only in the attached form, and a
  following word shall stay a separate argument (D-12).
- **FR-053**: The order shall come from a Fisher-Yates shuffle over
  `std::mt19937_64`, so one seed answers one order under every standard
  library (D-12).
- **FR-054**: A seed text shall read as a seed when it holds unsigned
  decimal digits alone, the whole text consumed, with a value at or below
  the largest 64-bit unsigned integer. A text that reads no seed shall
  print one line naming the option or the variable and the text and answer
  status 2, and that answer stands when a higher source names the option
  with a text that reads, so `-1`, `0x10`, `7x`, `true` and a digit string
  above that limit each answer the fault in either form.
  `SPEEDGUN_INTERLEAVE=0` shall mean interleaving on with seed 0.
- **FR-055**: The passes and the repetitions of one instance shall stay
  together in the shuffled order (D-12).
- **FR-056**: A shuffled run shall hold its rows and print them in
  registration order after the run, and the complexity fit of a family
  shall print after the last row of that family in registration order
  (D-12, D-19).
- **FR-057**: An interrupt during a shuffled run shall print the rows that
  ran in registration order and answer the `master` interrupt status
  (D-12).
- **FR-058**: The console and the `run` object shall state the seed
  (D-12).
- **FR-059**: A run with no `--interleave` shall print each row as it
  runs, with the console text and the exit status `master` produces
  (D-19).
- **FR-060**: The line `const auto result = runner.run(*entry);` shall
  stand in the run loop, and the entry-point shape gate shall stay green
  with one interrupt-flag reading inside that loop (D-18, PC-3).

### Rules file (C-7)

- **FR-061**: `--compare-rules=<file>` shall read one rule per line in the
  `--compare-rule` grammar of specs/026, and that grammar shall not change.
  A rules file shall hold rules alone, so a line that names another file
  reads as a rule the parser refuses under FR-064 (D-13, PC-4).
- **FR-062**: A line whose first non-space character is `#` shall be a
  comment, and a blank line shall yield no rule (D-13).
- **FR-063**: The rules of a file shall follow the `--compare-rule`
  options of the same source, in line order, and the rules a source
  contributes shall keep the order that source wrote them in. The rule
  list shall not combine across sources: the highest source that names
  rules shall settle it, and a rule of a lower source shall reach the
  parser before it drops under FR-038 (D-13, D-23). The row shall be
  scalar within one source: every path that source names shall be read
  while that source is read, and the last of those paths shall name the
  rules that join (FR-035). A path word that stands empty shall print
  one line naming the row and answer status 2 (FR-033).
- **FR-064**: A line the rule parser refuses shall print one line naming
  the file, the line number and the parser's fault, and answer status 2
  (D-13).
- **FR-065**: A rules file the system cannot read shall print one line
  naming the path and answer status 2.



### Documentation and example (C-9)

- **FR-066**: The main-page section `The command line` shall carry the new
  sources, the new options and the disclosure, and shall stay at most 25
  lines (PC-5).
- **FR-067**: The snippet of that section shall come from the region
  `command-line-read` of `example/docs/command_line.cpp`, and the
  example shall build and run in CI through the docs example targets
  (PC-5).
- **FR-068**: The main-page option table shall gain one row for each new
  long option, in the row form the table already uses (PC-5).
- **FR-069**: `docs/pages/harness.md` shall carry the full option
  reference: every table row with its name, its argument kind, its
  environment variable where it carries one, and its help text.
- **FR-070**: A test shall compare the option reference page with the
  option table in both directions, so a row missing from the page and an
  option missing from the table are both findings.
- **FR-071**: `PAGE_LINE_CAP` shall rise to the measured page length
  through a recorded decision in the gate's own comment, the way specs/023
  and specs/025 recorded theirs (PC-5).
- **FR-072**: The shell region `options` of
  `example/docs/command_line.sh` shall gain the command lines a reader
  types for the new options, and the snippet-compile check shall stay
  clean.
- **FR-073**: No new capability section title shall join `SECTIONS`, and
  the section shall keep its marked block and its recorded source file
  (PC-5).
- **FR-074**: The option reference page shall name the environment variable
  beside each option that carries one, and shall state the precedence order
  of D-1.

### Surface, version and discipline

- **FR-075**: The option-row check of `tools/docs/example_drift.py` shall
  read the table from the file that holds it after D-20 moves it, and a
  liveness probe shall show that the check can fail (PC-5).
- **FR-076**: The parse shall stay internal, and the public headers shall
  gain no symbol. The oracle of the ABI check of hard rule 12 is one
  diff run from the checkout against `origin/master`:
  `git diff --name-only origin/master -- include/speedgun-ng` prints no
  path, and the harness `SOVERSION` shall stay at the value `master`
  carries while it prints nothing; that result is recorded in this spec
  (PC-8).
- **FR-077**: The `speedgun-ng` archive `SOVERSION` shall stay 3
  (PC-8).
- **FR-078**: The project version shall take the next free minor at rebase
  time, 0.18.0 under the planned merge order. The fix branch 028b takes
  the patch 0.18.1, the next version above master at its last rebase.
- **FR-079**: One test file, `test/source/harness_cli_table_test.cpp`,
  shall check that every option name and every option value in the table is
  unique, and that the values 256 to 259 keep their options.
- **FR-080**: Every line and branch of the new code shall be covered, with
  no exclusion marker, no waiver row and no partial row, and the coverage
  filter for brace lines shall stay as `master` has it.
- **FR-081**: The tests shall set no process environment, run no sleep, read
  no clock and name no cpu count; every cpu count and thread count a test
  uses shall come from `sched_getaffinity`; and every test shall pass on
  a four-cpu CI runner and on a two-cpu host without skipping.
- **FR-082**: The closing postcondition of the entry point and the exit
  status table of `specs/015-benchmark-harness-core/contracts/cli.md`
  shall carry the statuses this feature answers (PC-1).
- **FR-083**: The time-source gate, the loop-shape gate and the entry-point
  shape gate shall stay green, and no per-iteration path shall change.
- **FR-084**: The `run` object shall be written with the vendored JSON
  writer, the reader shall use its error-code form with one test per error
  code it can answer, and no new library shall enter the tree.
- **FR-085**: Every new precondition shall carry its `SG_REQUIRE`, every
  new postcondition its `SG_ENSURE` and every new assertion its doc line
  in the form `tools/dbc/dbc_pair_gate.py` reads, and a helper or local
  that only a contract reads shall carry `[[maybe_unused]]`.
- **FR-086**: The report schema shall take the next free schema minor,
  1.3 under the planned merge order, and the writer shall stamp it (PC-6).

### Counter addresses this host does not find

- **FR-087**: For a counter that a user names from any source, an address this
  host does not find shall exit 2 on one line naming the source that held it,
  the address, and what the host found there. The listing and the catalog
  shall answer before the addresses are read, and an address that a higher
  source dropped shall answer its own line behind the source that named it.

This requirement is a ruling, and it is recorded here because it moves a
status spec 015 sets. For a counter that a user names, an address this host
does not find exits 2 (FR-087). FR-023 of spec 015 stays for internal leaves:
a caller that hands addresses to the runner keeps the absent column it
answers today, and a leaf a metric needs keeps its refusal kind with the run
going on. The status table of
`specs/015-benchmark-harness-core/contracts/cli.md:102` answers zero for a run
whose counter address the host cannot resolve; this requirement answers 2 for
the address a user named, and spec 015 is not edited for it. The oracle row
`case17` of `test/data/base_cli_matrix.txt` carries the new status.

## Key Entities *(include if feature involves data)*

- **Option table row**: one long option with its internal value, its
  argument kind, its environment flag, its help text and the requirement
  that owns it. One row is the definition of the option for the scan, the
  help text, the environment read and the reference page.
- **Option source**: one of command line, option file, environment, and the
  `master` default that stands when none of the three sets a value.
- **Environment variable**: `SPEEDGUN_` plus a long name, the carrier of a
  default for a row marked with the environment flag.
- **Option file**: a text file of one option per line in long form, read by
  `--options=<file>`, with its own fault line form naming the file and the
  line number.
- **Rules file**: a text file of one comparison rule per line in the
  specs/026 grammar, read by `--compare-rules=<file>`.
- **Thread count list**: the comma list `--threads` carries, counts and
  `min:max` ranges mixed, the set that replaces the registered counts of
  every family.
- **Interleave seed**: the unsigned decimal whole number, held in 64 bits,
  that fixes one shuffled run order, 0 for the bare form, stated on the
  console and in the document.
- **Disclosure entry**: one option with the source it came from and the
  value that took effect, carried as the three keys `option`, `source` and
  `value`, one entry per option taken from a source other than the command
  line, in table order.
- **Run object**: the top-level JSON object with the members `options` and
  `interleave` that carry the disclosure entries and the seed, present
  only when a source other than the command line set a value or
  `--interleave` stands given.
- **Usage error**: the one line a source fault answers, with the status 2
  the entry point returns. The line names the source and the value.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: The uniqueness test finds no repeated option name and no
  repeated option value in the table, and the values 256 to 259 still name
  `--metric-method`, `--compare`, `--compare-rule` and
  `--compare-strict` (FR-003, FR-079).
- **SC-002**: One test sets the same option in the command line, the option
  file and the environment with three different values, and the value that
  takes effect is the command-line one; the same test with the command line
  removed reads the option file value, and with both removed the
  environment one (FR-037, FR-038).
- **SC-003**: Each of these answers exactly one line naming the source and
  the value, and status 2, with no benchmark row printed: a bad environment
  value, a bad option-file line, a forbidden option-file line, a bad
  `--threads` list, a bad interleave seed, a bad rules-file line, and an
  unknown option (FR-007 to FR-009, FR-027, FR-033, FR-036, FR-046, FR-054,
  FR-064). Each answer stands whatever source named the value, and it
  stands when a higher source names the same option with a value that
  reads. A command line that carries a usage fault ahead of `--help`
  answers that fault alone, and one that carries `--help` ahead of the
  fault answers the help text with status 0 (FR-017).
- **SC-004**: A run with `--threads=1,<n>`, where `<n>` sits above one
  and at or below the `sched_getaffinity` cpu count, runs every benchmark
  at both counts; the one-count names carry no `/threads:` segment and the
  `<n>` counts carry `/threads:<n>` (FR-044, FR-048).
- **SC-005**: Over a suite of ten or more instances and a fixed test
  vector, `--interleave=7` gives the same run order on two runs,
  `--interleave=8` gives a different order, and the printed rows and the
  complexity fit keep registration order in both runs (FR-051, FR-056).
- **SC-006**: `--compare-rules=<file>` answers the same table and the same
  status as the same rules given as repeated `--compare-rule` options
  (FR-061).
- **SC-007**: A run with no new option prints the console report `master`
  prints and answers the same status; its JSON document differs from the
  `master` document in the `schemaVersion` value alone (FR-003, FR-059,
  FR-086).
- **SC-008**: `--help` prints one line per table row in table order and
  answers 0; `--version` prints the project version and answers 0; neither
  runs a benchmark; and `--help` beside `--version` prints the help alone
  (FR-015 to FR-017).
- **SC-009**: A run that takes one option from the environment writes a
  `run` object whose `options` array holds one object with the keys
  `option`, `source` and `value` for that option, prints the matching
  console line, and a run with no such source writes no object and prints
  no line (FR-039 to FR-041).
- **SC-010**: The coverage gates report 100% line and 100% branch coverage
  over the new code with no exclusion marker (FR-080).
- **SC-011**: The main-page section stays at or below 25 lines, its snippet
  comes from the example that CI builds and runs, the option reference page
  matches the table in both directions, and
  `python3 tools/docs/example_drift.py .` reports no finding (FR-066 to
  FR-071).
- **SC-012**: The loop-shape gate reports the same per-iteration
  instruction cycle as it reports at `bd10d76` under both compilers it
  drives, and the entry-point shape gate reports no finding (FR-060,
  FR-083).
- **SC-013**: Every fault a source can answer reaches status 2 through the
  parse rather than through a process exit, and no `std::exit(2)` call
  remains in `source/harness/cli.cpp` (FR-010, FR-011).
- **SC-014**: The full gate set passes on the branch: the `ci-ubuntu` and
  `ci-rocky` builds, `ctest`, `ci-sanitize`, `ci-tsan`, clang-tidy and
  cppcheck with no new finding, `format-check`, `spell-check`,
  `prose-lint`, the time-source gate, the page-drift gates, the DBC gates,
  and the coverage gates (FR-080 to FR-085).



## Constraints

- **Unchanged run.** A command line that holds no new option answers the
  console text, the document bytes apart from `schemaVersion`, and the
  exit status `master` answers (FR-003, FR-059, SC-007).
- **Exit statuses.** Status 0 answers a normal run, `--help` and
  `--version`; status 1 answers a failed benchmark or an interrupt;
  status 2 answers a usage fault in any source. Each fault prints exactly
  one line (FR-007 to FR-009, FR-027, FR-033, FR-046, FR-054, FR-064).
- **No process exit in the parse.** The parse answers a value; the entry
  point ends the process (FR-010, FR-011). The one `std::exit(2)` this
  feature leaves alone sits in the expansion (FR-014).
- **Internal surface.** The parse, the table, the environment seam and the
  file reader stay inside `source/harness/detail/`; the public headers
  gain no symbol (FR-076).
- **Value ranges.** New option values sit in 290 to 319 and the master
  values keep their options (FR-003, FR-004).
- **Counters-only.** No new time or counter read enters the harness, and
  `test/time_source_gate.sh` stays clean (FR-083).
- **No timed-path change.** The feature touches no per-iteration code, so
  the loop-shape gate stays at its base reading (FR-083).
- **Test discipline.** No sleep, no clock, no fixed cpu count, no
  `setenv`; the cpu count comes from `sched_getaffinity`; `ctest -j 2`
  stays the runner; the coverage preset stays as `master` has it
  (FR-081).
- **Coverage.** 100% line and 100% branch with no `LCOV_EXCL` marker under
  `source/harness/`, and no code shaped to move a coverage count (FR-080).
- **JSON discipline.** The vendored simdjson writes the `run` object
  through `builder::string_builder`; the reader keeps its error-code form;
  no new library enters the tree (FR-084).
- **Docs budget.** `SECTION_LINE_CAP` stays 25, no entry joins
  `SECTION_LINE_CAPS`, the section stays the existing one, and the page cap
  moves by the measured amount through a recorded decision (FR-066, FR-071,
  FR-073).
- **No host assumptions.** No test names a cpu count or a host size, and
  every test passes on a four-cpu CI runner without skipping (FR-081).
- **Prose and commits.** Principle XI and the commit lint bind every word
  and every commit; commit messages carry blank lines between body
  paragraphs and one footer block with `Refs:` then `Approved-by: OmO`.
- **Rebase discipline.** Rebase, push with `--force-with-lease`, and show
  `git ls-remote origin <branch>` after each push.

## Scope

In: FR-001 to FR-087. The one option table with its fields, the help and
version options, the environment defaults with their twelve rows, the
option file, the precedence rule, the disclosure object and console line,
`--threads`, `--interleave` with its print order, the comparison rules
file, the internal parse with its environment seam, the scan-error
messages, the uniqueness and precedence tests, the main-page section with
its example, the option reference page, the schema and version effect, and
the `PAGE_LINE_CAP` move.

Out, each owned by a later roadmap item or another lane:

- the `--benchmark_*` Google Benchmark aliases, a later item of the
  roadmap (D-10);
- context groups and YAML layouts (N9), report generators and plots (N8),
  and the multi-pass fields of the report document (N6, merged);
- the jitter-probe options of N13a and the `--hunt*` options of N14a and
  N14b, whose rows this feature moves into the table at rebase and whose
  values and behavior stay theirs;
- repetitions spread across instances, and shell completion;
- any change to the comparison rule grammar of specs/026, the percentile
  column qualifier that specs/026 D-16 left for N7, and any new value name
  a rule can read (Open questions);
- any environment variable for an option outside the twelve rows of D-3;
- a short option for any option, and any change to the letters the master
  rows hold;
- the exit status of a compare verdict, which specs/026 settled and this
  feature leaves alone.

## Assumptions

- The roadmap file the prompt names sits outside this repository and is
  absent from this machine, so no claim here rests on it. The prompt text
  is the authority for scope.
- The scanner keeps the glibc behavior of matching an unambiguous prefix of
  a long name, because the prompt lists the prefixes that become ambiguous
  (PC-1).
- One option file per run is enough: a second `--options` is a fault
  (FR-032), and a nested `options=` line is a fault (FR-031).
- The `run` object lists the value that took effect in the form the option
  itself prints, because a reader compares it with the row the run printed.
  A repeatable value joins its entries with `;`, the separator D-6 states,
  so a printed entry reads back as a setting.
- A shuffled run holds its rows in memory until the run ends. A suite of
  thousands of instances holds thousands of result values, the same order
  of magnitude the JSON document already gathers before it writes
  (`ReportData` at `source/harness/detail/internal.hpp:434` region of
  `RunOptions` and the `report` local of the entry point).
- The thread-count option replaces the registered counts rather than
  intersecting with them, because the prompt states "replaces the
  registered thread counts of every benchmark".
- The disclosure console line is one line whatever the number of entries,
  because hard rule 3 binds a fault to one line and a run to a readable
  header.

## Budget note

`README.md` stands at 773 lines and `PAGE_LINE_CAP` reads 773 (PC-5), so
the page has no room at all. The section `The command line` runs fourteen
lines of its 25-line budget, so it can take eleven more, and the option
table gains one row for each new long option: `--options`, `--env`,
`--threads`, `--interleave`, `--compare-rules`, `--help` and
`--version`, seven rows. The cap moves to the measured page length when
the section lands, the smallest fitting amount, recorded in the gate's own
comment the way specs/023, specs/025 and specs/026 recorded theirs. Prose
is trimmed first, no snippet is dropped, and no earlier spec is edited. No
new section title joins `SECTIONS`, because the section already exists
(PC-5). If a rebase brings an N13a or N14a section in first, this feature's
cap move is measured against the page as the rebase leaves it.

## Dependencies

- The entry point and the option scanner: `speedgunMain`, the long-option
  table, the reject helpers and the usage-error path of
  `source/harness/cli.cpp`, and the exit status table of
  `specs/015-benchmark-harness-core/contracts/cli.md` (PC-1, PC-8).
- The thread-count surface: `BenchmarkHandle::threads`, `threadRange` and
  `denseThreadRange`, `RegistryEntry::threadCounts`,
  `detail::buildThreadCounts`, `instanceName` and
  `detail::placementFitStatus` (PC-2).
- The run loop, the fit print and the interrupt path of the entry point,
  and the row printers of `source/harness/report.cpp` (PC-3).
- The comparison surface of specs/026: `parseCompareRule`,
  `CompareRule`, `CompareOptions`, `compareReports` and
  `detail::runCompareMode` (PC-4).
- The report surface of specs/023: the writer of
  `source/harness/json_report.cpp`, the reader of
  `source/harness/json_load.cpp`, and the schema file
  `docs/schema/report-v1.json` (PC-6).
- The documentation gates and their budgets: `README.md`,
  `tools/docs/example_drift.py` (the `SECTIONS` list, the section map,
  `PAGE_LINE_CAP`, `SECTION_LINE_CAP`, `SECTION_LINE_CAPS`,
  `cli_options`), `docs/pages/harness.md`, and the docs example targets
  (PC-5).
- The shape gates: `test/cli_shape.sh` and `test/loop_shape.sh` (PC-3).
- The existing CLI tests whose expected text changes:
  `test/source/harness_cli_test.cpp` and
  `test/source/harness_cli_placement_test.cpp`, and the target lists of
  `test/CMakeLists.txt` (PC-7).
- The vendored simdjson: the writer's `builder::string_builder` and the
  reader's error-code form, under the privacy rules of specs/004 FR-013 to
  FR-017.
- The constitution gates of Principle VIII: the counters-only scan, the
  loop-shape gate, the entry-point shape gate, the naming rules, Principle
  II, IV.1, the coverage gates, `ci-sanitize`, `ci-tsan`, clang-tidy and
  cppcheck, the DBC gates, and the prose and commit lint.


## Citations

Every anchor below was read on `bd10d76`, the head of `master` at branch
creation, on 2026-10-22. The audit-point reports that carry the readings sit
under `.omo/waves/028-audit/`, machine-local and never committed. A plan
re-reads each anchor on the base it runs against and finds each item by its
symbol first.

| Anchor | What it shows |
| --- | --- |
| `source/harness/cli.cpp:37` | `handleInterrupt`, the SIGINT handler that sets the interrupt flag |
| `source/harness/cli.cpp:69` to `:71` | `kCompareOptionValue` 257, `kCompareRuleOptionValue` 258, `kCompareStrictOptionValue` 259 |
| `source/harness/cli.cpp:80`, `:132`, `:152`, `:174` | the four `std::exit(2)` calls of the reject helpers this feature removes (FR-011) |
| `source/harness/cli.cpp:296` to `:297` | `detail::metricNameFailure` answers 2 |
| `source/harness/cli.cpp:311` | `speedgunMain` |
| `source/harness/cli.cpp:345` | `kMetricMethodOption` 256, the last local option constant |
| `source/harness/cli.cpp:346` to `:371` | `static option longOptions[]`, the twenty rows this feature moves into the table |
| `source/harness/cli.cpp:376`, `:377` | `optind = 0` and `getopt_long(argc, argv, "", longOptions, nullptr)`: no short letters, `opterr` unset |
| `source/harness/cli.cpp:411`, `:415` | `case 'p'` sets `showPercentiles` true and `case 'P'` sets it false, the master last-word answer FR-023 keeps |
| `source/harness/cli.cpp:431` to `:562` | the nine `return 2` usage answers of the scan |
| `source/harness/cli.cpp:503`, `:506` | the unknown-option text and the `default` arm that also carries the missing-argument case |
| `source/harness/cli.cpp:525` | the `detail::runCompareMode` call, ahead of expansion |
| `source/harness/cli.cpp:607` | the `detail::placementFitStatus` call |
| `source/harness/cli.cpp:617` | `std::signal(SIGINT, handleInterrupt)` |
| `source/harness/cli.cpp:630` to `:668` | the run loop, its comment, the fit print at `:637`, the run at `:645`, the row print at `:646`, the one flag reading at `:652`, the break at `:666` |
| `source/harness/cli.cpp:672`, `:690` | the last family fit after the loop, and the closing `SG_ENSURE` on status |
| `source/harness/family.cpp:223`, `:246` | `instanceName` and the `/threads:` segment it appends above one context |
| `source/harness/family.cpp:290`, `:382` | `checkFamilySize` above `kMaxFamilySize`, and the preset-name `std::exit(2)` this feature leaves alone |
| `source/harness/family.cpp:404`, `:425` | the thread-count loops of family expansion |
| `source/harness/detail/internal.hpp:35` | `kMaxFamilySize` 100 |
| `source/harness/detail/internal.hpp:199` | `RegistryEntry::threadCounts`, default `{1}` |
| `source/harness/detail/internal.hpp:315` | `detail::buildThreadCounts`, the doubling growth |
| `source/harness/detail/internal.hpp:434`, `:486`, `:494`, `:500` | `RunOptions`, `Runner`, `run(Instance&)`, `fitForFamilyEnd` |
| `source/harness/registry.cpp:535`, `:574`, `:613`, `:630` | `threads`, `buildThreadCounts`, `threadRange`, `denseThreadRange` |
| `include/speedgun-ng/benchmark.hpp:1320`, `:1496`, `:2314`, `:2351`, `:2370` | `threads()`, `m_threads`, `threads(int)`, `threadRange`, `denseThreadRange` |
| `include/speedgun-ng/benchmark.hpp:701`, `:766`, `:2589`, `:2618` | `CompareRule`, `CompareOptions`, `parseCompareRule`, `compareReports` |
| `source/harness/detail/compare.hpp:49`, `:77` | the default rule of specs/026 FR-033, and `runCompareMode` |
| `source/harness/placement_preset.cpp:173` | `placementFitStatus`, the preset refusal of a count it cannot seat |
| `source/harness/report.cpp:313`, `:317`, `:410` | `printContext`, the version line, `printResult` |
| `source/harness/json_report.cpp:31`, `:583` | `kSchemaVersion` `1.1` and the key the writer stamps |
| `docs/schema/report-v1.json:7` to `:9` | the top-level key order, the required list, and `additionalProperties: true` |
| `CMakeLists.txt:9`, `:53`, `:231`, `:261` | VERSION 0.16.0, archive `SOVERSION` 3, `SG_PROJECT_VERSION`, harness `SOVERSION` 10 |
| `test/cli_shape.sh:32`, `:39`, `:47` | the awk range that finds the run loop, the one-flag-reading count, the `bool interrupted =` spelling |
| `test/source/harness_cli_test.cpp:806`, `:810` | the two tests that expect `unknown option`, one for an unknown name and one for a missing value |
| `test/source/harness_cli_placement_test.cpp:449` | a third test that matches the same text |
| `test/CMakeLists.txt:651` to `:654`, `:695` to `:702` | the CLI test targets the message change reaches |
| `tools/docs/example_drift.py:32` to `:57`, `:63` | `SECTIONS`, which already lists `The command line`, and the section map |
| `tools/docs/example_drift.py:155`, `:156`, `:161` | `PAGE_LINE_CAP` 773, `SECTION_LINE_CAP` 25, `SECTION_LINE_CAPS` |
| `tools/docs/example_drift.py:458` to `:466`, `:557`, `:685`, `:691` | `cli_options()` reading the table out of `cli.cpp`, the page-cap check, and the two option-row findings |
| `README.md:357` to `:370` | the section `The command line`, fourteen lines, with its region marker at `:359` |
| `README.md:646`, `:745` to `:766` | the quick-reference table and the option table with its twenty rows |
| `docs/pages/harness.md:1217` | the last capability section, `## Multi-pass collection`; the page holds no option reference |
| `specs/015-benchmark-harness-core/contracts/cli.md:73`, `:83` | the exit status section and its table |
| `specs/026-regression-gate/spec.md:137` to `:140`, `:1071` | D-11, the deferral of the rules file to N7, and the scope line that keeps it out |

## Open questions

Four open questions stand: D-22, D-25, D-27, and the percentile column
qualifier specs/026 D-16 left for N7. None blocks planning. Each carries
the reading this spec takes and the reason the reading needs a review, so
a plan can build on it before the code lands.

1. **Option file line forms** (D-22): the prompt gives the line form as one
   option per line in the long form without `--`, and says nothing about a
   line for a no-argument option or a line missing its `=`. The spec reads
   a no-argument line as whole without `=`, and reads a line for a
   required-argument option that carries no `=`, and a line that starts
   with `-`, as file faults naming the file, the line number and the text
   (FR-033, FR-036). Another line-form reading moves two fault requirements
   and their lines, so the reading is recorded as open.
2. **The ambiguity candidate list** (D-25): `opterr` stands at 0, so the
   scanner reports an ambiguous prefix with no candidate list of its own.
   The spec reads the candidate set as a thing the parse computes from the
   table and names in table order (FR-009, FR-013). Whether the line names
   candidates at all is a wording choice with two readings.
3. **The percentile pair under a command-line override** (D-27): the spec
   reads the fault of D-5 as a check on the environment pair alone, ahead
   of the precedence rule, so two environment variables that contradict
   each other answer status 2 even when the command line overrides one
   half. The reading rests on the fact that a reader who sets both
   variables made a mistake worth reporting, and the command-line pair
   keeps the master answer FR-023 states. The other reading, the pair
   check after precedence, cannot see a pair with one half overridden.
4. **The percentile column qualifier specs/026 D-16 left for N7**: specs/026
   deferred whether a comparison rule may name the percentile column it
   reads. This feature takes no decision: the rule grammar stays as
   specs/026 wrote it (FR-061), so the question stays open here.

## Constitution Checklist (Principle IV.1)

- **IV.1 spec completeness**: YES. The spec names the feature branch, the
  base commit and the date, records the run-time binding of every anchor,
  states the decisions with their derivation, lists the preconditions with
  measured anchors, gives seven user stories with acceptance scenarios and
  an edge-case list, states 86 functional requirements across the nine
  roadmap capabilities, gives 14 measurable success criteria that map the
  acceptance criteria of the prompt to the repository gates, states the
  constraints, the scope in and out, the assumptions, the documentation
  budget, the dependencies and the citations, records four readings the
  spec takes, and leaves four open questions.
- **IV.2 plan and tasks follow from the spec**: the plan reads the anchors
  by symbol on the base it runs against, and the task list shall cover
  every FR and every SC.
- **X anti-slop**: the parse, the table and the two file readers reuse the
  surfaces PC-2 and PC-4 already provide; no new abstraction stands behind
  one caller, and `--threads` replaces one field rather than adding a
  mechanism.
- **XI prose**: no em dash, no contrastive `not` form, no filler or
  voucher phrasing; the gate over these files reports the examined-unit
  count and 0 findings.

