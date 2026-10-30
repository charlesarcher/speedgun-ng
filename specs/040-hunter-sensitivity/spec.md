# Feature Specification: Hunter Sensitivity and Baseline Delta (N14b)

**Feature Branch**: `040-hunter-sensitivity`

**Created**: 2026-10-30

**Status**: Draft

**Input**: User description, opening quoted. The request of 2026-10-30
is the owner's pasted prompt for roadmap item N14b (section 3 and
sections 12.3 and 12.5 of the harness roadmap). The prompt's rulings
bind this feature and stand carried in Decisions; its standing facts
and contention notes stand carried in the sections that read them.

> Rank the hunt's events by what they do to the run, item N14b of the
> roadmap (section 3, section 12.3 and 12.5): the `sensitivity` rank
> names the Spearman rank correlation and the slope of the event rate
> against the time per iteration across the benchmark's args or sizes,
> and the `delta` rank names the relative change against a baseline
> JSON report loaded through `loadReport`. The console carries the two
> columns, `--hunt-emit-rules` writes the promoted events as N5 rules,
> and the library answers them through
> `sg::hunt::HuntResult::toCompareRules`.

## Clarifications

### Session 2026-10-30

- Q: What figure shall the threshold of an emitted rule carry? → A:
  the run's noise threshold, the same figure for every promoted
  event; the rule's direction stands from the measured delta's sign
  (`lower` for a drop, `higher` for a rise), and a promoted event
  with no measured delta stands `lower` (FR-021, D-7).
- Q: What shall a run with `--hunt-rank=delta` and no
  `--hunt-baseline` do? → A: print one line naming the value and exit
  2, the standing refusal shape (FR-011).
- Q: What shall the run do when the baseline document's top-level
  hunt object names a norm different from the contender's
  norm? → A: print one line naming the mismatch and exit 2, beside
  the schema-pairing refusal of FR-005.

## Spec identity and run-time binding

- Spec number: `040`. `specs/` on master `bea6af5b` holds `001`
  through `038` (read 2026-10-30). `031` stands reserved for N0b host
  validation (kennykiller), `037` for N11 execution graphs (chilly),
  and `039` for N9 context groups (thrawn, prompt ready), so `040` is
  the next free number and was taken as the prompt plans.
  `git ls-remote --heads` names no ref carrying `040` (read
  2026-10-30). `gh pr list` could not run: the CLI answers
  unauthenticated on this machine, so the open-pull-request check
  stands on the lane table and the remote branch list, and the rebase
  re-checks the number beside the other bindings.
- Branch: cut from master `bea6af5b` (PR #59, spec 038 N8 report view
  generators, merged 2026-10-30). The branch stands
  `040-hunter-sensitivity`.
- Version: master stands at `0.25.0` (`CMakeLists.txt:30`), harness
  `SOVERSION` `16` (`CMakeLists.txt:333`), archive `SOVERSION` `4`
  (`CMakeLists.txt:95`). The branch takes the next free minor,
  `0.26.0`, at its cut; specs/037 and specs/039 each take one minor at
  their own merge, so a rebase past a landing takes the next free
  minor, and the commit that moves the number records the check (the
  reservation rule of specs/029 FR-090).
- ABI: `RankedEvent` gains the rank fields and `EntryHunt` gains
  `toCompareRules`, so the harness public ABI moves and the harness
  `SOVERSION` rises to `17`. The counters-library surface moves
  nothing, so the archive `SOVERSION` stays `4` (hard rule 12). The
  check stands recorded beside the version-move commit.
- Report schema: master stands at `1.8`
  (`source/harness/json_report.cpp:39`). The rank fields join the
  per-entry `hunt` object, a closed node, so the schema takes the next
  free minor, `1.9`, at the cut. Specs/037's graph record moves the
  same minor at its own merge, so the second branch to cut past a
  landing takes the next value and records the check (hard rule 11).
- Option values: the hunt block stands taken through `280`
  (`--hunt-emit-set`), and `281` to `289` stand free for this feature
  (`source/harness/detail/options.hpp:96-107`). `--hunt-emit-rules`
  takes `281` and `--hunt-baseline` takes `282`, both as named
  constants, long spelling only, no short letter, no environment
  source. No standing name or value moves, and the name and value
  uniqueness test (`test/source/harness_cli_table_test.cpp`) carries
  the rows.
- Contention, measured 2026-10-30 at the push: specs/039's pushed
  branch (`origin/039-context-groups`, cut from the same master
  `bea6af5b`) stands at version `0.26.0`, harness `SOVERSION` `17`
  and schema `1.10`, and its option comment leaves `281` to `289`
  free. The two lanes claim `0.26.0` and `SOVERSION` `17` each, and
  the schema minors split (`1.9` here, `1.10` there). The
  reservation rule covers the second merge: the lane that merges
  first keeps its values, and the other takes the next free minor,
  the next free schema value and the next `SOVERSION` at its rebase,
  the commit that moves the numbers recording the check.
- Lane host: `thrawn` (AMD Threadripper 9960X, Zen 5, 24 cores, 48
  threads, one socket), the machine this lane stands self-contained
  on per the owner's ruling of 2026-10-30: gary stands another
  lane's host and stays out of this one. The acceptance runs stand
  recorded on thrawn, one record per run naming the commands, the
  exit codes and the artifacts, the way specs/029 and specs/033
  recorded theirs (R-5 as this lane binds it).

## Roadmap position

- Item N14b of the harness roadmap (section 3, sections 12.3 and
  12.5). Specs/029 (N14a core) and specs/033 (N14a-2 budgets and
  promotion) stand merged. The `score` rank stays the roadmap's
  combined default (decision 12.8-3) and stays out; the top-down area
  is N14c's and stays out.
- Provenance: the roadmap file stands at
  `/home/archerc/code/speedgun-roadmap.md` on this machine; the
  `/workspace/` path the prompt names stands absent here (Citations).

## Standing facts this lane reads

- The hunt surface stands public at `include/speedgun-ng/hunt.hpp`:
  `HuntNorm`, `HuntRank` (`RATE`, `CV`), `HuntBudget`, `HuntOptions`,
  `EntryHunt` with `toCounterSet`, `RunHunt`, and the public calls
  `planHunt`, `rankEvents`, `runHunt`.
- The hunt option rows stand at `source/harness/detail/options.hpp`:
  `--hunt` at `270` through `--hunt-emit-set` at `280`, and `281` to
  `289` stand free for this feature (`:96-107`).
- The comparison rules stand at specs/026: the grammar
  `<value>[@<agg>]:<lower|higher>:<threshold>[:<filter>]` with the
  `counter:<label>` value name, the threshold as `<n>%` or `abs=<n>`,
  the refusal shape, and the rules file the `--compare-rules` option
  reads stands the same format. The exported parser stands at
  `include/speedgun-ng/benchmark.hpp:2906` (`parseCompareRule`).
- The baseline loader stands public at
  `include/speedgun-ng/benchmark.hpp:2878` (`loadReport`), the one
  input surface (R-1).
- The statistics stand at `source/harness/statistics.cpp`:
  `sg::detail::statisticsOf`, the one summary the harness reports and
  a hunt rank calls (specs/029 FR-093). The complexity fit carries the
  standing least-squares sums at `source/harness/complexity.cpp:69`
  (specs/019).
- The per-entry hunt JSON stands a closed node
  (`source/harness/json_report.cpp:503-544`): `address`, `rate`, `cv`,
  `noisy`, `multiplexed`, `rank`, with null for a rank the run left
  unset; `source/harness/json_load.cpp` reads the node back.
- The results-evidence precedent stands at
  `specs/029-hunter-core/results/` and
  `specs/038-report-view-generators/results/`: one record per run
  naming the commands, the exit codes and the artifacts, the
  quickstart linking each record.
- The README `Hunter mode` section stands at `README.md:740-758`
  inside its 25-line cap, with the quick-reference rows at
  `README.md:925-933`.

## Decisions

These stand as the recommendations of the prompt and of this audit,
recorded per X.1 so no implementer reopens them. Each names the
reading it settles.

- **D-1 The ranks stand the standing record.** The sensitivity and the
  delta are figures of the hunt record the run measured: `RankedEvent`
  gains the fields, and no second record joins. The roadmap's
  `sg::hunt::HuntResult` name maps to `EntryHunt`, the record the
  surface shipped (PC-1), so `toCompareRules` joins `EntryHunt` beside
  `toCounterSet`, the pair the prompt names (R-1).
- **D-2 The sensitivity runs over one family of one run.** One point
  per entry of one benchmark family of one run, a family standing the
  entries of one suite, the registered family; the case name carries
  the args or the labels. The
  point carries the entry's time per iteration and the event's
  normalized rate under the norm the run settled. The Spearman rank
  correlation and the least-squares slope of the rate against the time
  per iteration stand over the points. At least 3 points; fewer stands
  `n/a` (section 12.3).
- **D-3 The rank keys order by the figure they name.**
  `--hunt-rank=sensitivity` orders by the correlation descending,
  ties by the slope and then the catalog address;
  `--hunt-rank=delta` orders by the magnitude of the relative change
  descending, ties by the catalog address. Only an event that carries
  the key's figure carries a rank under that key (the standing rule of
  specs/029 I-5), and the noisy events stand unranked under both keys
  (the standing rule of specs/029 D-7).
- **D-4 The delta matches by N5's rules.** The baseline's benchmark
  entries match the contender's by the rules the comparison runs
  (specs/026, R-2); the events match by catalog address inside the
  matched entry, the rate figures standing in the baseline document's
  per-entry hunt object. The delta stands
  `100 x (contender - baseline) / baseline`, a percent, of the matched
  event's median rate; the sign follows the change.
- **D-5 A figure the match does not find stands the disclosure.** A
  baseline event the contender carries none of, a contender event the
  baseline holds none of, and a baseline rate at or below zero each
  stand the `n/a` disclosure, and the run exits 0 (R-1). No error
  joins a missing figure.
- **D-6 The baseline loads through `loadReport` alone.** The path the
  `--hunt-baseline=<file>` value names is the one input (R-1). A file
  `loadReport` refuses, a schema pairing the comparison refuses, and
  a baseline whose hunt object names a norm different from the
  contender's norm, exit 2 with one line naming the value or the
  mismatch (FR-005).
- **D-7 The promoted events stand the ranked set.** `--hunt-emit-rules`
  promotes the same set specs/033's `--hunt-emit-set` promotes, the
  top `--hunt-top` ranked events in rank order, expressed as N5 rules
  in the standing grammar (R-3). The rule's threshold carries the
  run's noise threshold, and its direction stands from the measured
  delta's sign, `lower` where no delta stands (FR-021).
- **D-8 The statistics stand one surface.** The correlation joins the
  standing statistics surface beside `statisticsOf`, and the slope
  reuses the standing least-squares sums of the complexity fit or a
  helper moved beside them, one implementation either way (PC-2); the
  hunt adds no second copy (R-4).

## Audit point

### PC-1: the hunt surface stands as specs/029 and 033 shipped it. PASS.

Read 2026-10-30: the header carries the records above and the three
public calls; `source/harness/hunt_cli.cpp:342` refuses
`sensitivity`, `delta` and `score` with a message naming N14b; the
refusal message stands at `:49-56` with the contract at `:327-328`.
This feature shrinks the refused set to `score` alone.

### PC-2: the statistics stand where the figures can join them. PASS.

`sg::detail::statisticsOf` stands the one summary helper
(`source/harness/statistics.cpp:20`); the complexity fit carries
least-squares sums over sizes (`source/harness/complexity.cpp:69`). No
Spearman helper and no slope-over-time helper exist on master (read
2026-10-30). The figures stand new code on the standing surface, one
implementation each (D-8).

### PC-3: the baseline loader and the rules parser stand public. PASS.

`loadReport` stands exported at `include/speedgun-ng/benchmark.hpp:2878`;
`parseCompareRule` stands exported at `:2906`. Both carry the contract
form. No second reader and no second grammar exist.

### PC-4: the option table stands ready for two rows. PASS.

The hunt block holds `270` to `277` and `280`
(`source/harness/detail/options.hpp:99-107`), and `:95-98` records that
`281` to `289` stand free for the next feature. The two rows take
`281` and `282`; `283` to `289` stay free.

### PC-5: the per-entry hunt JSON node stands closed. PASS.

`writeEntryHunt` (`source/harness/json_report.cpp:503-544`) writes
every key of the closed node and stands null for a rank the run left
unset; `json_load.cpp` reads the node back; the schema file carries
the node with its `x-order`. The rank fields join the node under the
same rule.

### PC-6: the version bindings stand contented and covered. PASS-with-note.

Master stands at `0.25.0`, harness `SOVERSION` `16`, archive
`SOVERSION` `4`, schema `1.8` (all read 2026-10-30). Specs/037 and
specs/039 reserve one minor and one schema minor each at their own
merge; the reservation rule of specs/029 FR-090 and FR-053 covers the
second cut. Note: `gh pr list` stood unauthenticated today, so the
open-pull-request check rides the remote branch list and the lane
table; the rebase re-checks beside the other bindings.

### PC-7: the page cap and the example seam stand. PASS.

The `Hunter mode` section spans `README.md:740-758`, inside the
25-line cap; the `hunt-rank` region and its `.expected` file stand at
`example/docs/hunt_rank.cpp`; a new region joins the same seam, and
the drift gates carry the rows against the option table (IV.1).

### PC-8: the scripted seam stands ready. PASS.

`counters_fake.hpp` scripts values, availability and the limit
(specs/029 PC-3, D-18); specs/033's round-trip tests stand at
`test/source/`. The sensitivity and delta tests build the
`EventSamples` and `EntryHunt` records directly, so every branch of
the figures and the disclosures stands reached with no PMU (R-5).

## Out of scope

- The `score` rank key (roadmap decision 12.8-3).
- The top-down area (N14c).
- Any change to the comparison grammar, its parser, or the
  `--compare-rules` reader (R-3): the emitted lines stand the standing
  grammar.
- Any second baseline reader or baseline format (R-1).
- Any change to the hunt plan, the budgets, the discovery, or the pass
  loop: specs/029 and 033 carry them.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Rank the events by what they do to the run (Priority: P1)

A maintainer measures one registered benchmark over a family of at
least 3 arg points on thrawn with `--hunt=all` and
`--hunt-rank=sensitivity`. The ranked table gains the sensitivity
column: each cell carries the Spearman rank correlation and the slope
of that event's rate against the time per iteration over the family's
entries, and the table orders by the correlation. The events whose
rate tracks the time per iteration stand at the top.

**Why this priority**: the ranking is the item's core question, and
every other part of the feature reads its figures.

**Independent Test**: A run over a scripted family with at least 3
arg points prints the column filled and the table ordered by it; a
family under 3 points stands `n/a` in every cell of the column.

**Acceptance Scenarios**:

1. **Given** a family of 4 arg points whose scripted rates rise with
   the time per iteration, **When** the run carries
   `--hunt-rank=sensitivity`, **Then** the table orders by the
   correlation, the top cell carries the correlation `1` and the
   slope stands beside it.
2. **Given** a family of 2 arg points, **When** the run asks for the
   sensitivity rank, **Then** every sensitivity cell stands `n/a`, no
   event carries a rank under the key, and the run exits 0.
3. **Given** an event a noisy flag marks, **When** the table prints,
   **Then** the event carries no rank under the sensitivity key and
   its figures stand in the document.

### User Story 2 - See what the baseline changed (Priority: P1)

A maintainer runs the same hunt against a recorded baseline document
with `--hunt-baseline=baseline.json`. The ranked table gains the delta
column: the relative change of each event's median rate against the
matching baseline figure, a signed percent. An event, an entry or a
figure the match does not find stands `n/a`, and the matching rules
stand disclosed beside the figures.

**Why this priority**: the delta answers the run-over-run question the
hunt exists for, and the rules output carries its figures.

**Independent Test**: A run against a scripted baseline document with
known rates prints the expected deltas and the `n/a` disclosures; a
baseline file the loader refuses ends the run with status 2 and one
line naming the value.

**Acceptance Scenarios**:

1. **Given** a baseline whose matched entry carries a known rate for
   the event, **When** the contender measures a rate 10 percent above
   it, **Then** the delta cell prints `10.0%` beside the disclosure
   line naming the matching rules.
2. **Given** a baseline entry with no figure for an event the
   contender measured, **When** the table prints, **Then** the event's
   delta cell stands `n/a` and the run exits 0.
3. **Given** a baseline path that names a document the loader refuses,
   **When** the run starts, **Then** one stderr line names the value
   and the run ends with status 2.

### User Story 3 - Hand the findings to the gate (Priority: P2)

A maintainer adds `--hunt-emit-rules=rules.txt` to the delta run. The
run writes the promoted events as N5 rules in the standing grammar,
one per line, and the next comparison reads the file again through
`--compare-rules` under the standing validation (FR-021).

**Why this priority**: the promotion turns the hunt's figures into a
gate input, and it rides the figures the first two stories compute.

**Independent Test**: A run writes the file; a comparison reads the
file again under the standing validation and answers the verdicts the
paired rules name; the round trip stands recorded under the results
directory.

**Acceptance Scenarios**:

1. **Given** a delta run with promoted events, **When** the run
   carries `--hunt-emit-rules=rules.txt`, **Then** the file holds one
   `counter:<event>@median:<lower|higher>:<n>%` line per promoted
   event, and the comparison reads the file again under the standing
   validation: the compared documents pair the prefix with the value
   space their rows carry, the pairing the evidence record measures
   and the open question records.
2. **Given** `--hunt-emit-rules` beside no hunt, **When** the run
   starts, **Then** one line names the value and the run ends with
   status 2.
3. **Given** a hunt that promotes no event, **When** the run writes
   the file, **Then** the file stands empty and the run exits 0.

### User Story 4 - Read the ranks from the report (Priority: P2)

A reader of the JSON document finds the rank fields the run computed
in the per-entry hunt object: the correlation, the slope and the delta
per event, null where the run measured none. `loadReport` reads the
fields back with equal values.

**Why this priority**: the document is the machine surface the gate
and the tooling read, and the schema guard holds it equal.

**Independent Test**: A written document loads back with equal field
values; the schema guard tests move with the keys at the new minor.

**Acceptance Scenarios**:

1. **Given** a run that computed the figures, **When** the document is
   written and loaded back, **Then** every rank field reads back equal
   and the schema guard passes at the new minor.
2. **Given** a run under 3 arg points, **When** the document prints,
   **Then** the sensitivity fields stand null and no key of the node
   stands absent.

### User Story 5 - Read the page and copy a working block (Priority: P3)

A reader of the main page finds the two ranks taught in the
`Hunter mode` section, one marked snippet compiled from an
`example/` region, and the quick-reference rows for the two new
options, all inside the section's line cap.

**Why this priority**: the page is the entry point, and the drift
gates hold it equal to the code.

**Independent Test**: The drift gates pass: the region compiles, the
example runs, and the rows match the option table.

**Acceptance Scenarios**:

1. **Given** the updated section, **When** the drift gates run,
   **Then** the snippet compiles over the canonical includes, the
   example prints the rows its result sentence names, and the
   quick-reference rows match the option table.

### Edge Cases

- A family of 1 or 2 arg points stands the sensitivity `n/a` (FR-002).
- An event some entries of the family measure and others stand no
  sample of stands its points over the entries that measured it; under
  3 points it stands `n/a` (D-2).
- A baseline document with no hunt object in the matched entry stands
  the delta `n/a` for every event of the match (D-5).
- A baseline rate at or below zero stands the delta `n/a` (D-5).
- A point set with no spread in either coordinate stands the
  correlation `n/a`, because no rank order exists to correlate.
- A `--hunt-rank=sensitivity` run whose events all stand `n/a` prints
  the table in the standing order with empty rank fields and exits 0
  (D-3).
- A `--hunt-rank=delta` run beside no `--hunt-baseline` prints one
  line naming the value and ends with status 2 (FR-011).
- A baseline whose hunt object names a norm different from
  the contender's norm ends the run with status 2 and one line naming
  the mismatch (FR-005).
- A rules file path that cannot be opened or written prints one line
  naming the value and ends with status 2 (FR-022).
- The standing refusals stand unchanged: a hunt beside `--counter`,
  `--metric` or `--compare`, and a value outside the range its option
  states, each end with status 2.

## Requirements *(mandatory)*

Requirements group under the letters (A) to (F). Every requirement
names at least one test (FR-051).

### Ranks (A)

- **FR-001**: When a run with a hunt measures one benchmark family
  across 3 or more entries of one suite, the registered family (D-2),
  the run
  shall compute, per event address the measured entries share, a
  sensitivity figure: the Spearman rank correlation and the
  least-squares slope of the event's normalized rate against the
  entry's time per iteration, the rates under the norm the run
  settled. One entry stands one point (D-2).
- **FR-002**: A family with fewer than 3 such entries, and a point set
  with no spread in either coordinate, shall stand the sensitivity
  figure `n/a`; the console cell, the JSON fields and the rank order
  shall say so, and the run shall exit 0 (D-2, D-3).
- **FR-003**: When `--hunt-baseline=<file>` stands set beside a hunt,
  the run shall load the baseline through `loadReport` alone (R-1),
  shall match the baseline's benchmark entries to the contender's by
  the rules the comparison runs (R-2), shall match the events by
  catalog address inside the matched entry, and shall compute the
  delta as `100 x (contender - baseline) / baseline`, a signed percent
  of the matched event's median rate (D-4, D-6).
- **FR-004**: A figure the match does not find, and a baseline rate at
  or below zero, shall stand the `n/a` disclosure with the run exiting
  0; the run shall print the matching rules beside the figures one
  time, and no error shall join a missing figure (D-5, R-1).
- **FR-005**: A baseline file `loadReport` refuses, a schema pairing
  the comparison refuses, and a baseline whose top-level hunt object
  names a norm different from the contender's norm, shall
  each print one stderr line naming the value or the mismatch and
  exit 2 (D-6).
- **FR-006**: The correlation and the slope shall run through the
  standing statistics surface: the correlation joins the surface
  beside `statisticsOf`, the slope reuses the standing least-squares
  sums or a helper moved beside them, and the hunt shall add no second
  copy (D-8, R-4). The ranks shall read no clock, no counter, no
  `/proc`, no sysfs and no CPUID beyond the samples the run collected.

### Sort keys and console (B)

- **FR-010**: `--hunt-rank` shall accept `sensitivity` and `delta`
  beside the standing `rate` and `cv`; `score` alone shall keep the
  refusal message, which shall name the roadmap item that owns it
  (PC-1).
- **FR-011**: `--hunt-rank=sensitivity` shall order the ranked table
  by the correlation descending, ties by the slope and then the
  catalog address; `--hunt-rank=delta` shall order by the magnitude of
  the relative change descending, ties by the catalog address. Only an
  event that carries the key's figure shall carry a rank under that
  key, and the noisy events shall stand unranked under both keys (D-3).
  A run that names `--hunt-rank=delta` beside no `--hunt-baseline`
  shall print one line naming the value and exit 2, the standing
  refusal shape of FR-023.
- **FR-012**: The ranked table shall carry the sensitivity column
  (the correlation and the slope) and the delta column beside the
  standing event, rate, CV and noisy flag; an unmeasured figure shall
  stand `n/a` in its cell. The top `--hunt-top` cap, the skipped list
  and the plan summary shall stand unchanged.

### Promotion and baseline option (C)

- **FR-020**: The option table shall gain exactly two rows:
  `--hunt-emit-rules=<file>` at value `281` and
  `--hunt-baseline=<file>` at value `282`, both named constants, long
  spelling only, no short letter, no environment source (PC-4, hard
  rules 9 and 10).
- **FR-021**: A run with a hunt and `--hunt-emit-rules=<file>` shall
  write the promoted events, the top `--hunt-top` ranked events in
  rank order (D-7), as N5 rules in the standing grammar, one per
  line, for example `counter:<event>@median:lower:<n>%`; the file
  stands the comparison's input, and `--compare-rules` reads it
  again under the standing validation (R-3): the compared documents
  pair the rules' prefix with the value space the compared rows
  carry, a pairing the evidence record measures and the open
  question records. The rule's
  threshold shall carry the run's noise threshold, the figure
  `--hunt-noise` states, the same value for every promoted event. The
  rule's direction shall stand from the measured delta's sign:
  `lower` for a drop, `higher` for a rise; a promoted event with no
  measured delta shall stand `lower` (D-7).
- **FR-022**: `--hunt-emit-rules` beside no hunt, an empty value, and
  a file that cannot be opened or written shall each print one line
  naming the value and exit 2 (PC-4, the standing refusal shape of
  specs/033 FR-012). A hunt that promotes no event shall write an
  empty file with exit status 0.
- **FR-023**: `--hunt-baseline` beside no hunt and an empty value
  shall each print one line naming the value and exit 2.
- **FR-024**: `EntryHunt::toCompareRules(...)` shall answer the
  promoted events' rule texts, with the public contract form (II) and
  no `detail/` type in the signature; the pair `toCounterSet` of
  specs/033 stands the shape (hard rules 5 and 8, D-1).

### JSON and schema (D)

- **FR-030**: The per-entry hunt object shall carry, per entry of
  `hunt.entries`, the rank fields the run computed: the sensitivity
  correlation, the sensitivity slope and the delta, null for a figure
  the run did not measure; the node shall stay closed, every key shall
  stand in every hunt document, and no key shall stand absent (PC-5).
- **FR-031**: `loadReport` shall read the rank fields back with equal
  values; the writer and reader tests shall move with the keys (PC-5).
- **FR-032**: `docs/schema/report-v1.json` shall gain the keys at
  schema minor `1.9`, the next free minor at this cut, or the next
  free minor at rebase, with the `x-order` moved in the same commit
  (hard rule 11, the reservation rule of specs/029 FR-053).

### Documentation and example (E)

- **FR-040**: The README `Hunter mode` section shall move with the
  feature inside its 25-line cap: the sentences that teach the two
  ranks, one marked snippet sourced from an `example/` region CI
  builds and runs, and the result sentence; the quick-reference rows
  for the two new options shall join in the same change (IV.1, PC-7).
- **FR-041**: The drift gates shall carry the change: the region with
  its `.expected` file, an `example_run` entry, and the
  quick-reference rows checked against the option table (PC-7).

### Version, tests and discipline (F)

- **FR-050**: The version block shall take the next free minor,
  `0.26.0` at this cut, and the harness `SOVERSION` shall rise to
  `17`; the archive `SOVERSION` shall stay `4`; the commit that moves
  the numbers shall record the checks beside them (PD-20 precedent).
- **FR-051**: The sensitivity computation, the delta computation, the
  matching and its disclosures, the sort keys, the refusals, the rules
  round trip and the JSON round trip shall stand tested on the
  scripted seam (`FakeProvider`, R-5), each requirement of (A) to (E)
  carrying at least one test.
- **FR-052**: The acceptance runs on thrawn shall stand recorded under
  `specs/040-hunter-sensitivity/results/`, one record per run naming
  the commands, the exit codes and the artifacts, the way specs/029
  and specs/033 recorded theirs; the quickstart shall link each
  record.
- **FR-053**: The change shall clear the standing gates: 100 percent
  line and branch coverage with no `LCOV_EXCL` marker and no new
  `--ignore-errors` entry, the sanitizer and tsan gates, the suite
  under `taskset -c 0,1`, the DBC gate, the time-source scan
  (`test/time_source_gate.sh`), the format and spell checks, and the
  prose rules of XI.

## Key Entities

- **`RankedEvent`**: gains the rank figures the run measured: the
  sensitivity correlation, the sensitivity slope and the delta, each
  standing empty when the run measured none. The record stays data;
  the console, the document and the rank order read it.
- **`EntryHunt`**: gains `toCompareRules(...)`, the rule texts of the
  promoted events, beside `toCounterSet`.
- **`HuntOptions`**: gains the baseline path the CLI settled, empty
  when the run named none.
- **Baseline document**: a report `loadReport` answered; its
  per-entry hunt objects stand the rate figures the delta matches
  against (specs/029 FR-050).

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: On thrawn, one run over a family with at least 3 arg
  points prints the sensitivity column filled for the shared events
  and orders the table by it on request; the record stands under the
  results directory with the commands, the exit codes and the ranked
  table.
- **SC-002**: One run with `--hunt-baseline` against a recorded
  baseline document prints the delta column and the matching-rules
  disclosure beside the figures; the record stands under the results
  directory.
- **SC-003**: `--hunt-emit-rules` writes a file the comparison reads
  again under the standing validation; the round trip stands recorded
  under the results directory, the measured pairing of the rules'
  prefix with the compared documents' value space beside it and the
  open question carrying the ruling.
- **SC-004**: A run under 3 arg points stands the `n/a` column and no
  rank under the key; the noisy events stand unranked under both keys.
- **SC-005**: A written document loads back with the rank fields
  equal, and the schema guard tests pass at the new minor.
- **SC-006**: The gates stand green: 100 percent line and branch
  coverage, no exclusion markers, sanitizer and tsan clean, the suite
  green under `taskset -c 0,1`, the DBC, time-source, format and
  spell gates green.
- **SC-007**: The section, its region and the quick-reference rows
  move in the same change, and the drift gates pass.

## Constraints

- The shared files the lanes touch stand
  `source/harness/detail/options.hpp` and `source/harness/options.cpp`,
  the CLI table test, `docs/schema/report-v1.json`,
  `source/harness/json_report.cpp` and `json_load.cpp`, the
  `CMakeLists.txt` version block and the README quick-reference; the
  landings serialize on these files, and every binding stands measured
  at the branch cut against the tree master then stands at.
- The option values stand the hunt block's own reserve (`281` to
  `289`), no contention the other lanes reach.
- The rank stays a pure computation over collected data: the planner
  and the rank read no clock (the standing rule of specs/029 D-2).
- The README section stands inside its 25-line cap.

## Assumptions

- The time per iteration of an entry stands the primary time per
  iteration the report already carries for the entry; no second
  timing joins.
- The Spearman correlation averages the ranks at ties, the standard
  practice, and a point set with no spread in either coordinate stands
  `n/a`.
- The delta carries a sign; the sort key orders by magnitude (D-3).
- The baseline's rates stand in the baseline document's per-entry hunt
  objects; a baseline without a hunt object in the matched entry
  stands the disclosure for every event of the match (D-5).
- The emitted rule's threshold carries the run's noise threshold, and
  its direction stands from the measured delta's sign, `lower` where
  no delta stands (FR-021, D-7).

## Dependencies

- specs/026: the rules grammar, the matching rules, the refusal shape
  (R-2, R-3).
- specs/029: the hunt core, the rank records, the standing refusal
  shapes.
- specs/033: the promotion shape, the option-row rules.
- specs/028: the `--compare-rules` option and its file format.
- specs/019: the statistics surface and the complexity fit's
  least-squares sums.

## Citations

- Roadmap: `/home/archerc/code/speedgun-roadmap.md`, section 3 (the
  lane table), 12.3 (sensitivity and delta), 12.5 (output and
  promotion), 12.6 (the CLI and library names, `--hunt-baseline` and
  `HuntResult` among them). The `/workspace/` path the prompt names
  stands absent on this machine; the file above stands the copy read
  today.
- The prompt's rulings R-1 to R-5 stand carried in Decisions and
  Requirements, each at the requirement that binds it.
- The numbered hard rules the Requirements cite (5, 8, 9, 10, 11 and
  12) stand the N14a lane prompt's rule list: the prompt this spec
  comes from carries no numbered list, and the constitution and the
  roadmap copy carry none, so each number reads through specs/029,
  which quotes and summarizes the list at its Input and its
  Decisions: specs/029's D-22 states rule 9's meaning and its ABI
  record states rule 12's.

## Open questions

- The emitted rules' prefix versus the comparison's value space
  (FR-021): the emitted lines name `counter:`, the prefix the prompt's
  example fixes, and the comparison pairs `counter:` with the State
  user counters, while the hunt's promoted events measured through
  `--counter` stand under the `metric:` namespace. The measured
  arrangements therefore refuse the emitted file at the value
  resolution, and the derived gate file the evidence record derives
  answers the verdicts. The prefix choice, the comparison's value
  space, or both stand the owner's call; the lane ships the prompt's
  form and records the measured pairing.

## Budget note

The prompt names no time budget. The hunt's own budgets stand
(specs/033); the two ranks add no pass, run no second collection and
read no clock.

## Constitution Checklist

- Main-page impact check (IV.1): yes. The `Hunter mode` section, its
  `example/` region and the quick-reference rows move with the feature
  in the same change (FR-040, FR-041).
