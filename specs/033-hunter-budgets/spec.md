# Feature Specification: Hunter Budgets and Promotion (N14a-2)

**Feature Branch**: `033-hunter-budgets`

**Created**: 2026-10-28

**Status**: Draft

**Input**: User description, opening quoted from the lane prompt
`speedgun-ng-033-hunter-budgets-specify-prompt.md` (reconstructed copy,
2026-10-28):

> Finish the hunter mode's budget story, item N14a-2 of the roadmap
> (section 12.8, split 5): the sample budget and the multiplex budget
> that spec 029 deferred, and the promotion path that writes the
> ranked events back as a counter set the run can read again. After
> this feature, a hunt that finds its culprits can hand them to
> `--counter` with no hand copying.

The request re-states rulings P-1 to P-6; each stands in Decisions
with the ruling it carries named. The decisions the roadmap and spec
029 record stand as quoted there.

## Spec identity and run-time binding

- Spec number: `033`. `specs/` on master `e4a98bc8` holds `001`
  through `030`, `032` and `034` (read 2026-10-28). `031` is reserved
  for N0b host validation and `033` stands free; `specs/034` records
  that reservation on its own identity line. `git ls-remote --heads`
  names no ref carrying `033`, and `gh pr list --state open` answers
  empty (read 2026-10-28). The number was free and is taken.
- Branch: cut from master at branch time. The request names
  `2a48411` as the base; master stands at `e4a98bc8`, a descendant of
  `2a48411` (verified by `git merge-base --is-ancestor` today), so the
  branch takes the master tip and the named base stands covered.
  X.1 records this reading.
- Version: master stands at `0.21.0` (`CMakeLists.txt:15`). The lane
  takes the next free minor at branch time, `0.22.0` while no other
  lane moves; a taken number moves the lane up under the reservation
  rule spec 029 records.
- ABI: harness `SOVERSION` stands at `13` (`CMakeLists.txt:298`),
  archive `SOVERSION` at `4` (`CMakeLists.txt:80`). `toCounterSet`
  joins the public hunt surface, so the harness public ABI changes
  and its `SOVERSION` rises by one. The counters-library surface
  moves nothing, so the archive `SOVERSION` stays `4` unless the
  scripted seam changes. The check is recorded beside the commit that
  moves the version (hard rule 12).
- Report schema: master stands at `1.6`
  (`source/harness/json_report.cpp:36`). The top-level `hunt` object
  in `docs/schema/report-v1.json` is closed (`additionalProperties:
  false` with its `x-order` list), so the budget keys are a minor
  bump: the schema takes `1.7`, or the next free minor at rebase, with
  the `x-order` moved in the same commit.
- Option values: the hunt block `270` to `279` stands taken, with
  `278` and `279` unused (`source/harness/detail/options.hpp:94`
  names the constants, and `:93` records that `280` to `289` stay
  free for the next feature). That block belongs to this lane.
  `--hunt-emit-set` takes `280`, the first free value, and `281` to
  `289` stay free. `--counter=@<file>` extends the standing
  `--counter` value grammar and adds no row. PC-3 re-checks that no
  catalog address can collide with the `@` form.
- Lane host: `gary`, the AMD Zen 5 acceptance host named by the
  2026-10-26 move (Ryzen 9 9950X3D, read from `/proc/cpuinfo` today).
  `corsairone` is down since 2026-10-26, and this lane plans no Intel
  run.

## Roadmap position

- Item N14a-2 of the harness roadmap (sections 4, 8, and 12.8, split
  5). N14a core stands merged as spec 029. The rank keys
  `sensitivity`, `delta` and `score` and the rules output
  `--hunt-emit-rules` are N14b's and stay out; the top-down area is
  N14c's and stays out. Roadmap section 10.6 rules stay in force in
  full.
- Provenance: the roadmap file `/workspace/speedgun-ng-harness-roadmap.md`
  stands absent on this machine (verified 2026-10-28). This request
  is the copy reconstructed 2026-10-28 from the roadmap sections 4, 8
  and 12 and from `specs/029-hunter-core` on master. The decisions
  those two sources record stand as quoted; any ruling the lost
  original fixed beyond them re-settles in this lane's `/speckit-clarify`
  step before the branch takes its number. Roadmap acceptance
  criterion 12.7-6, the promotion round trip, is cited as this record
  holds it.
- The lane may run N0b host validation (reserved `031`) beside this
  work (ruling H-7 as the request re-states it). No artifact is shared
  between the two.

## Lane identity

- Zen 5 acceptance stands on gary (D-11). The suite itself stays
  host-agnostic: the catalog and the limits come from the injected
  provider, and the suite passes under `taskset -c 0,1`.

## Standing facts this lane reads

- Spec 029's two-stage budget (its D-2, FR-013), the rank keys with
  their N14b refusal (its D-3, FR-032), the list-file rules (its
  D-14, FR-006), the conflict refusals (its D-16, FR-081), the JSON
  hunt objects (its D-17, FR-050 to FR-053) and the counters-only
  hard rule stand unchanged. This spec amends FR-060's scope line and
  no other standing rule (D-12).
- `--hunt` with `--metric-method=multiplex` stays legal and discloses
  the multiplexing (spec 029 edge cases); beside a `g` cap on a host
  that publishes limits it binds nothing, the standing limit-sized
  passes standing (FR-003).
- Spec 028's one option table binds the new row and the grammar
  extension; the hunt family stays command-line only (no environment
  name, `options.cpp:713-715`), and the three-source gather is the
  rule of the `--counter` row the `@` form rides. Spec 034's
evidence table shape (scenario, command, exit,
  observation) binds the acceptance record.
- Spec 032's record fixes the contamination signal and the margin the
  timing bounds of this spec sit inside.

## Clarifications

### Session 2026-10-28

- Q: How do the sample and multiplex budgets spell themselves inside
  `--hunt-budget=`, and does the sample budget admit fractional
  seconds? → A, the recommended option, chosen by the lane owner on
  2026-10-28: one
  value with comma-separated suffix clauses, each kind at most once,
  any order: `s` and `p` unchanged (two-stage total, integer), `e`
  the sample budget in seconds with fractions allowed (the
  `parseSeconds` shape of `--min-time`), `g` the multiplex cap as a
  whole count 1 to 10000 (the `--hunt-top` range); for example
  `--hunt-budget=120s,0.5e,32g`. A repeated kind, an empty clause,
  leading or trailing blank, and a clause with no suffix are each the
  one-line exit-2 refusal naming the whole value (FR-004; D-4). The
  two candidates the recommendation beat, and why: a repeated option
  needs new replacement law for a partly-spelled budget set;
  colon-positional clauses need a stated empty form for an unset
  middle clause. The comma form has direct house
  precedent in `--jitter-probe`'s value grammar (spec 027 contract)
  and in the hunt selection value's comma splitting. `/speckit-analyze`
  or a later clarify pass may revise this with a new recorded answer.

## Audit point

- Repository: `master` at `e4a98bc8` (project version `0.21.0`,
  report schema `1.6`, harness `SOVERSION 13`, archive `SOVERSION 4`),
  read 2026-10-28 on gary. Find every item by its symbol; line
  numbers stand as audit evidence alone.

### PC-1: the hunt surface stands as spec 029 shipped it. PASS.

- `include/speedgun-ng/hunt.hpp` carries `HuntNorm`, `HuntRank`,
  `HuntBudget`, `HuntOptions`, `SkippedEvent`, `EventCatalog`,
  `HuntColumn`, `HuntStage`, `HuntPlan`, `RankedEvent` (`:392`),
  `EntryHunt` (`:412`) and `RunHunt` (`:450`).
- `RankedEvent` holds `address`, `rate`, `cv`, `samples`, `noisy`,
  `repeated`, `multiplexed` and the optional `rank`; a null rank
  marks an event excluded from the top N. `EntryHunt` is the
  per-benchmark hunt result and holds the ranked entries and the
  skipped list. `RunHunt` is the run-level record beside
  `ReportData::hunt`.
- `planHunt` takes `calibrationNsPerPass` as data, prices
  `estimateNs` from it, and reads no clock, counter, `/proc`, sysfs
  or CPUID (its documented post-condition). The stage-split rule and
  its drop reason `over-budget` stand on the same record.

### PC-2: the `--counter` address path carries no column-count cap. PASS.

- The gather arm of the `counter` row keeps every address it is
  given: one per command-line word or option-file line, `;` items
  from the environment, repeated rows accumulating
  (`source/harness/options.cpp:1537-1551`, `sourceItems` at `:1361`).
- The run builds one report column per settled address
  (`source/harness/runner.cpp:1225,1239`); no count bound stands on
  that path. `kMaxUserCounters` (`include/speedgun-ng/benchmark.hpp:199`)
  bounds the sixteen named counters a benchmark body opens through
  `State::setCounter` (`source/harness/runner.cpp:914-926`), and it
  bounds no command-line counter column. Spec 029's plan records the
  same reading (`plan.md:76`). A default hunt tops 20, so an emitted
  set of twenty addresses must and can read again.

### PC-3: no catalog address can begin with `@`. PASS.

- Spec 029 PC-7 measured the pinned table tree and found no `@` in
  any event name. Re-measured 2026-10-28: 46,837 `EventName` rows in
  `external/pmu-events` carry no `@` anywhere. The `@` characters in
  the tree stand inside `MetricExpr` formula text alone (for example
  `external/pmu-events/arch/x86/amdzen5/pipeline.json:46`), and
  metric rows reach no device catalog.
- The hunt selection intercepts `@` where the option value is read,
  one level deep, through `readSelectionFile`
  (`source/harness/hunt_cli.cpp:130`, used by `expandHuntSelection`
  at `:502`). The `--counter` grammar extension joins that
  interception shape at the place the `--counter` value is read.

### PC-4: the budget value shape carries text and refuses bare numbers. PASS.

- `parseHuntBudget` (`source/harness/hunt_cli.cpp:440`) takes `<n>s`
  or `<n>p`, refuses a text with no unit and a count that is not
  above zero, and quotes the whole value in its one line.
- The JSON writes the budget as the canonical clause text
rebuilt from the stored value, or
  `null` (`source/harness/json_report.cpp:706-732`); the schema holds
  the type `['string', 'null']`; the reader refuses text with no unit
  and text with no amount, each with its own test
  (`source/harness/json_load.cpp:1250-1262`). A new budget value
  joins that text shape.

### PC-5: a file that cannot be written refuses with one line and status 2. PASS.

- `writeReport` names the path and the reason on stderr when the
  report file cannot be opened or written
  (`source/harness/json_report.cpp:1042-1061`), and the caller sets
  status 2 (`source/harness/cli.cpp:1536-1541`). The emitted-set file
  follows that standing shape.

### PC-6: the schema objects that gain keys are closed. PASS.

- `docs/schema/report-v1.json`: the top-level `hunt` object carries
  sixteen properties, `additionalProperties: false`, and the matching
  `x-order`; the per-benchmark `hunt` object carries six properties,
  closed the same way. New keys move `properties` and `x-order` in
  the same commit, the rule spec 034 followed at `1.6`.

### PC-7: the option table and its uniqueness test stand ready. PASS.

- `source/harness/detail/options.hpp` records the blocks: hunt `270`
  to `279` with `278` and `279` unused, `280` to `289` free for the
  next feature (`:93`), and the command-line lane's `290` to `296`.
  `280` is the first free value.
- `test/source/harness_cli_table_test.cpp` exists on master and
  asserts the uniqueness of every option name and value (spec 029
  FR-083). The eight hunt rows carry no environment source
  (`source/harness/options.cpp:719-766`), and the new row follows
  that shape.

### PC-8: the scripted seam carries the budgets and the round trip. PASS.

- `FakeProvider` (`include/speedgun-ng/counters_fake.hpp`) scripts
  values, availability, per-device programmable-counter limits and
  open failures (spec 029 PC-3 and D-18). The budget plan reads its
  estimate as data (PC-1), so the derived repeats, the group split,
  the emitted file and its round trip stand testable in CI with no
  PMU hardware, no clock and no sleep (roadmap section 12.7 shape).

### PC-9: the documentation budget stands thin beside its caps. PASS-with-note.

- `tools/docs/example_drift.py:194-195`: `PAGE_LINE_CAP` 864,
  `SECTION_LINE_CAP` 25. The README stands at 861 lines, three under
  the cap, and its `Hunter mode` section at 20 lines, five under the
  cap. The extension keeps the section within 25 (ruling P-5); the
  page headroom is three lines, so the measured README length at
  implement decides whether the cap rises through a recorded decision
  (spec 029 FR-072 precedent).

## Decisions

X.1 records each settled reading; a later implementer reopens none of
them. The ruling each decision carries is named beside it.

- **D-1 (P-1) The two-stage budget stands unchanged.** The sample and
  multiplex budgets extend D-2 of spec 029 and replace nothing. A run
  with no budget keeps the standing shape: every event, the full
  repeats.
- **D-2 The sample budget bounds one event's measurement.** A stage's
  measurement of one event takes no more than the budget states. The
  plan derives the stage's repeat count from the calibration estimate
  it already takes as data: the largest count that fits, never below
  one, at most the repeats the stage's own rule gives: one at stage
  1 of a two-stage plan, `--hunt-repeats` elsewhere (a budget
  bounds; it buys no extra repeats). A composed two-stage plan
  owns one derived figure, the stage-2 count. The plan reads no clock (spec 029 D-2's
  rule). The plan line and the report disclose the budget and the
  derived repeats, and the time estimate re-prices from the derived
  figure. When even one repeat exceeds the estimate, the stage runs
  one repeat and the plan discloses the overage: a plan that reads no
  clock promises nothing below one repeat, and a refusal there would
  make the budget unusable on a slow host. When no calibration figure
  prices the stage, the stage rule repeats stand and the plan
  discloses the unpriced budget.
- **D-3 The multiplex budget caps each multiplex group.** A plan that
  settles on the `MULTIPLEX` method holds at most the capped count of
  events in one group. The planner splits the planned addresses into
  consecutive groups of that size in plan order and discloses the
  split: the group count and the group sizes, beside the standing
  plan lines. A `PASSES` plan already fits each device's published
  limit, and the cap binds nothing there.
- **D-4 (P-2) The surface stays narrow: one new row and one grammar
  extension.** `--hunt-emit-set=<file>` takes one option-table row,
  value `280`, a named constant, long spelling, no short letter, and
  no environment source (the hunt rows' shape). Both budgets ride the
  `--hunt-budget=` value grammar. The clause form carries the answer
  settled at the clarify pass of 2026-10-28 and chosen by the lane
  owner (see Clarifications): one value with comma-separated suffix clauses,
  each kind named at most once, in any order. The standing `s` and
  `p` clauses keep their meaning and their integer parse (D-1). The
  sample budget takes `e` in seconds, fractional allowed, the
  `parseSeconds` shape of `--min-time` (finite, above zero, at most
  the standing whole-seconds bound, nothing below one nanosecond).
  The multiplex cap takes `g` as a whole count, 1 to 10000, the
  `--hunt-top` range. A full value reads, for example,
  `--hunt-budget=120s,0.5e,32g`. Rejected spellings, each the
  one-line exit-2 refusal naming the whole value: a repeated kind, an
  empty clause, leading or trailing blank, a clause with no suffix.
  The budget arrives on the command line only, as the hunt family
  always has; one value keeps the grammar whole whatever source a
  future hunt row might carry.
  X.1 records the candidates behind the recommendation: a repeated
  option with one suffixed budget per occurrence (reuses the
  single-suffix parse, and replacement of a partly-spelled set across
  sources needs a rule this option does not have); colon-separated
  positional clauses (shortest spelling, and an unset middle clause
  needs an empty form). The comma form answers both, because
  `--jitter-probe` carries the comma-clause precedent
  (`specs/027-jitter-probe/contracts/cli-options.md`, value grammar)
  and comma splitting already lives in the hunt selection value.
- **D-5 (P-4) The refusal rules keep their wording.** A bad budget
  value, a bad glob, an unknown norm and a conflicting option each
  print one line naming the value and exit 2 (roadmap 12.7-5; spec 029
  FR-081). `--hunt-emit-set` beside no hunt, including `--hunt=none`,
  is such a conflict. `--hunt-rank=sensitivity|delta|score` keeps
  refusing with a message naming N14b (spec 029 D-3); the rank keys
  stay N14b's.
- **D-6 The emitted file holds the ranked addresses.** Per benchmark
  entry, the emitted list holds the entry's top ranked events in rank
  order, capped at the run's `--hunt-top`; an event whose rank stands
  null (noisy, excluded, skipped or unmeasured) stands excluded. A
  run with several benchmark entries writes the deduplicated union of
  the per-entry lists: entry order first, rank order within each
  list. The file holds one catalog address per line in that order,
  with no header line. A hunt that ranks nothing writes an empty file
  and the run exits 0. The standing path rules of the other file
  options apply to the value.
- **D-7 The library surface.** `toCounterSet(topN)` joins
  `sg::hunt::EntryHunt`, the per-benchmark hunt record master carries.
  Roadmap 12.5 names the holder `HuntResult`; `EntryHunt` is that
  record's master name, and this re-anchor is X.1 record. The method
  answers the addresses of the top `topN` ranked entries in rank
  order. It carries the public contract form, and no `detail/` type
  reaches a signature (hard rules 5 and 8; spec 029 D-15 precedent).
  The run path composes the emitted file from the records (D-6).
- **D-8 The file form of `--counter`.** `--counter=@<file>` reads one
  level: the lines of the file are addresses. The reader is the
  list-file reader spec 029 prints: `#` starts a comment, blank lines
  are ignored; a file that does not exist, a line with an empty
  address, and a file that holds no address each print one line
  naming the value and exit 2. An address the host cannot resolve
  keeps the standing `--counter` shape: an `ABSENT` column with its
  stderr line, and the run continues. No count cap binds the file
  form (PC-2). The interception sits where the value is read, so the
  command line, an option file and the environment all carry the same
  grammar.
- **D-9 (P-3) The disclosure.** The plan and the report disclose the
  budgets the run used, console and JSON beside the fields spec 029
  prints. The JSON follows the standing budget text shape (PC-4) with one
  amendment the byte rule needs (FR-022): a field whose budget
  stands holds the canonical clause text rebuilt from the stored
  value, and a field whose budget
  does not stand writes no key; the derived plan figures (the sample
  budget's derived repeats, the multiplex group cap and group count)
  stand as numbers when they stand.
  `loadReport` reads every new key back with equal values, with a
  test per refusal code. A run without the budgets gives the same
  console report and the same JSON bytes apart from the
  `schemaVersion` value.
- **D-10 (P-5) Docs and example.** The README `Hunter mode` section
  grows within its 25-line cap; the option and field detail goes to
  `docs/pages/harness.md`; the quick-reference rows and the option
  table rows move in the shared lists; the standing example
  `example/docs/hunt_rank.cpp` extends to cover the promotion round
  trip inside its scripted, host-independent shape.
- **D-11 (P-6) The acceptance record.** gary carries the Zen 5
  acceptance: `--hunt=all` once with the budgets in force, and the
  emitted set read back with `--counter=@<file>`, recorded under
  `specs/033-hunter-budgets/results/` with the commands and the exit
  codes in the table shape spec 034's `results/quickstart.md` stands.
  No Intel run: corsairone waits.
- **D-12 The FR-060 amendment.** Spec 029 FR-060 (`spec.md:631`)
  reads: the two-stage budget of D-2 is the only budget in scope, and
  the sample and multiplex budgets belong to N14a-2. This spec brings
  the sample and multiplex budgets into scope; the amended scope line
  names spec 033 as their new home and leaves the two-stage rule
  standing. The full-repeats clause of spec 029 FR-013 and the
  `splitStages` stage guarantees amend in the same move: under a
  sample budget a stage's rule figure is the derived count, the
  guarantees read that figure as they read the requested one, and
  the stage postconditions with the stage record's documentation
  move to it. No other standing rule moves.

## Out of scope

- the rank keys `sensitivity`, `delta` and `score`, and the rules
  output `--hunt-emit-rules` (N14b);
- any new option row besides `--hunt-emit-set`, and any short option;
- nested `@` inside a list file (one level stands, the standing rule);
- raw event codes (spec 029 D-11);
- compare-rule keys naming hunt fields, and any change to
  `compare.cpp`;
- Intel hardware runs (corsairone down since 2026-10-26); Arm runs
  (scripted tests only).

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Hand the culprits to `--counter` (Priority: P1)

A performance engineer hunts a benchmark with `--hunt=all` and a
budget, and adds `--hunt-emit-set=culprits.txt`. The run writes the
ranked events to that file, one catalog address per line, best first.
The next run carries `--counter=@culprits.txt` and measures exactly
those events as counter columns, with no hand copying. The round trip
is roadmap acceptance criterion 12.7-6.

**Why this priority**: the promotion closes the hunt loop, and the
request names the round trip as the item's acceptance criterion.

**Independent Test**: a scripted run through the `FakeProvider` seam
writes the emitted file, and a second scripted run reads it back with
`--counter=@<file>` and reports one counter column per emitted
address (SC-001). It delivers the promotion with no PMU hardware.

**Acceptance Scenarios**:

1. **Given** a scripted host whose hunt ranks twenty events, **When**
   the run carries `--hunt-emit-set=<file>`, **Then** the file holds
   those twenty addresses, one per line, rank order first.
2. **Given** that file, **When** a scripted run carries
   `--counter=@<file>`, **Then** the report carries one counter
   column per emitted address and no column reads `ABSENT`.
3. **Given** `--hunt-emit-set` with no `--hunt` in the run, **When**
   the run starts, **Then** one line names the value and the exit
   status is 2.
4. **Given** gary's acceptance run, **When** the emitted set is read
   back, **Then** the columns match the emitted addresses and the
   record with its commands and exit codes stands under
   `specs/033-hunter-budgets/results/`.

---

### User Story 2 - Bound one event's measurement (Priority: P1)

The engineer sets the sample budget through `--hunt-budget=`. A
stage's measurement of one event then takes no more than the budget
states: the plan derives the stage's repeat count from the calibration
estimate and prints the budget and the derived repeats in the plan it
discloses before the run. A budget that even one repeat exceeds still
runs one repeat and says so; the budget never strands a slow host.

**Why this priority**: on a real host the per-event cost is the cost
that multiplies over hundreds of events; without this bound a hunt of
one slow benchmark can spend a whole work break on a single culprit.

**Independent Test**: give the planner a fixed calibration estimate
and a fixed seed and check the derived repeats, the re-priced estimate
and the disclosure lines (SC-002). It delivers the bounded stage with
no clock and no sleep.

**Acceptance Scenarios**:

1. **Given** a fixed estimate and a sample budget that fits two of
   three requested repeats, **When** the plan builds, **Then** the
   derived repeats are two, the estimate re-prices, and the plan line
   names the budget and the derived repeats.
2. **Given** a budget below one repeat's estimate, **When** the plan
   builds, **Then** one repeat stands and the plan line discloses the
   overage.
3. **Given** a malformed budget value, **When** the run starts,
   **Then** one line names the value and the exit status is 2.

---

### User Story 3 - Bound one multiplexed pass (Priority: P2)

On a device that publishes no counter limit, the standing plan
puts every event of a pass into one multiplexed group. The multiplex budget caps that group:
the planner splits a large catalog into consecutive groups of at most
the capped count, discloses the split, and the run produces passes
the host can open.

**Why this priority**: a large catalog is exactly the case the budget
story opens, and an over-wide group is the refusal shape the host
answers with; the two-stage budget bounds the hunt's total, this
budget bounds its widest group.

**Independent Test**: script 200 events on a no-limit device with a
cap of 32 and check the seven groups and their disclosure (SC-003).

**Acceptance Scenarios**:

1. **Given** 200 scripted events, no device limit, and a cap of 32,
   **When** the plan builds, **Then** it holds seven groups: six of
   32 and one of 8, in plan order, and the disclosure names the
   counts.
2. **Given** a cap at or above the event count, **When** the plan
   builds, **Then** one group stands and the disclosure says so.
3. **Given** a plan that settles on the `PASSES` method, **When** the
   run carries a multiplex cap, **Then** the cap binds nothing and
   the standing plan prints.

---

### User Story 4 - Read the page and copy a working block (Priority: P3)

The README's `Hunter mode` section teaches the budgets and the round
trip inside its 25 lines; `docs/pages/harness.md` carries the option
and field detail; the standing hunt example shows the promotion
inside its scripted shape, so the recorded output stands on every
host.

**Why this priority**: the constitution (IV.1) requires the main-page
section, the runnable example and the quick-reference rows with every
user-visible feature.

**Independent Test**: the drift gate passes with the section within
25 lines, and the example's `run_docs_` entry matches its `.expected`
file on a CI runner (SC-008).

**Acceptance Scenarios**:

1. **Given** the README after the change, **When** the drift gate
   runs, **Then** the `Hunter mode` section stands within
   `SECTION_LINE_CAP` and no new `SECTION_LINE_CAPS` entry exists.
2. **Given** the extended example, **When** CI runs it, **Then** the
   output matches the `.expected` file on a host without PMU access.

---

### Edge Cases

- `--hunt-emit-set` with an empty value: one line names the value,
  exit 2.
- The emitted file cannot be opened or written: the standing
  report-file shape, one stderr line naming the path and the reason,
  and status 2 (PC-5).
- A hunt that ranks nothing with `--hunt-emit-set` set: the file
  stands written empty and the run exits 0. That empty file read back
  through `--counter=@` meets the standing "holds no catalog address"
  refusal.
- A list file that holds comments or blank lines only: the same
  refusal names the value.
- A line inside the file naming an unknown address: the standing
  `--counter` shape, an `ABSENT` column with its stderr line, the run
  continues (D-8).
- A line inside the file naming a raw code such as `r4a03`: the same
  standing shape; the raw-code exit-2 refusal stays a hunt-selection
  rule (spec 029 D-11), and the file form changes only where
  addresses come from.
- `--hunt=none` beside `--hunt-emit-set`: the no-hunt refusal (D-5).
- The sample budget and the two-stage budget together: the derived
  repeats stand within both, the estimate re-prices with them, and
  spec 029's stage-drop decision reads the re-priced figure.
- The multiplex cap and the two-stage budget together: the cap applies
  inside every stage of the split plan.
- A sample budget with no calibration figure to price the stage: the
  stage rule repeats stand and the plan discloses the unpriced budget
  (D-2).
- A budget value that spells one kind twice (`0.5e,2e`), holds an
  empty clause (`120s,,32g`), or carries a clause with no suffix
  (`120s,0.5`): the one-line exit-2 refusal names the whole value
  (FR-004).
- A sub-nanosecond `e` word (`0.0000000001e`) and an `e` or `g` word
  at or above the standing bounds: the same refusal (FR-004).
- `--counter=@<file>` from the environment or an option file: the
  same file rules apply (D-8).
- `toCounterSet(topN)` asked for more events than the record ranks:
  every ranked address answers, in rank order; `topN` zero answers
  none.

## Requirements *(mandatory)*

### Budgets (A)

- **FR-001**: When a sample budget stands set and a calibration
  figure prices the stage, the planner SHALL derive the stage's
  repeat count as the largest count that measures each event of the
  stage within the budget, never below one and at most the repeats
  the stage's own rule gives (one at stage 1 of a two-stage plan,
  `--hunt-repeats` elsewhere), and SHALL read no clock (D-2).
- **FR-002**: The plan line and the report SHALL disclose the sample
  budget and its derived repeats; the priced figure the stage-drop
  decision reads SHALL re-price from them, and the plan's time
  estimate SHALL re-price when the derived count moves the
  estimate's first stage; a composed two-stage plan keeps its
  stage-1 estimate. One repeat above the budget SHALL stand with one repeat
  and a disclosed overage; a budget no figure prices SHALL stand with
  the stage rule repeats and a disclosure (D-2).
- **FR-003**: A plan whose settlement is `MULTIPLEX`, the answer a
  host gives when no needed device publishes a counter limit,
  SHALL hold at most the multiplex budget's count of events in one
  group, SHALL split the planned addresses into consecutive groups
  of that size in plan order, and SHALL disclose the group count
  and the group sizes. A `PASSES` settlement SHALL bind nothing
  from the cap, and so shall a forced `--metric-method=multiplex`
  on a host whose settlement is `PASSES` (D-3, spec 029 D-16).
- **FR-004**: Both budgets SHALL ride the `--hunt-budget=` value
  grammar in comma-separated suffix clauses (Clarifications,
  2026-10-28): `s` and `p` keep their standing meaning and integer
  parse, `e` names the sample budget in seconds with fractions
  allowed above zero within the standing whole-seconds bound, and `g`
  names the multiplex cap as a whole count in 1 to 10000. A value
  SHALL hold each kind at most once, in any order. A repeated kind,
  an empty clause, leading or trailing blank, a clause with no
  suffix, and any other malformed text SHALL each print one line
  naming the value and exit 2. Each budget SHALL stay optional and
  SHALL compose with the other budget and with the two-stage budget
  (D-4, D-5).
- **FR-005**: Spec 029's stage-split decision SHALL price its stages
  from the derived repeats, so the three budgets compose and the
  stage-drop reason keeps its meaning (D-2, D-3).

### Promotion (B)

- **FR-010**: The option table SHALL gain exactly one row:
  `--hunt-emit-set=<file>`, value `280` as a named constant, long
  spelling only, no short letter, no environment source (D-4, PC-7,
  hard rules 9 and 10).
- **FR-011**: A run with a hunt and `--hunt-emit-set=<file>` SHALL
  write that file with the run's ranked addresses: per benchmark
  entry the top `--hunt-top` ranked events in rank order, a
  multi-entry run the deduplicated union of those lists in entry then
  rank order, one address per line, no header. An event with a null
  rank SHALL stay out. A hunt that ranks nothing SHALL write an empty
  file with exit status 0 (D-6).
- **FR-012**: A file that cannot be opened or written SHALL print one
  stderr line naming the path and the reason and leave status 2
  (PC-5). `--hunt-emit-set` with no hunt, including `--hunt=none`,
  and an empty value SHALL each print one line naming the value and
  exit 2 (D-5).
- **FR-013**: `toCounterSet(topN)` SHALL join `sg::hunt::EntryHunt`
  and answer the addresses of the top `topN` ranked entries in rank
  order, with the public contract form and no `detail/` type in the
  signature (D-7, hard rules 5 and 8).
- **FR-014**: `--counter=@<file>` SHALL read one level, the file's
  address elements standing as the lines under the list-file
  separator rule: `#` starts a comment, blank lines are ignored, a
  line may hold comma-separated addresses, and a missing file, a
  comma beside nothing (the empty address), and a file holding no
  address each print one line naming the value and exit 2. An unresolvable address SHALL keep the
  standing `ABSENT` column shape. No count cap SHALL bind the file
  form (PC-2). The interception SHALL sit where the value is read, so
  the command line, an option file and the environment carry one
  grammar (D-8).
- **FR-015**: The emitted set SHALL read again: a run of
  `--counter=@<file>` over an emitted file SHALL report one counter
  column per emitted address. The round trip SHALL run in CI through
  the scripted seam, with no PMU hardware, no clock and no sleep
  (roadmap criterion 12.7-6; 12.7 shape).

### Report and schema (C)

- **FR-020**: The top-level `hunt` JSON object SHALL gain the two new
  budget fields in the conditional text shape (a field is written
  when its budget stands, absent when it does not), beside the
  derived numeric plan figures of FR-002 and FR-003. The closed schema
  objects SHALL gain every new key in `properties` and in `x-order`
  in the same commit; the schema minor takes `1.7` or the next free
  minor at rebase (D-9, PC-6, hard rule 11).
- **FR-021**: `loadReport` SHALL read every new key back with equal
  values; the writer SHALL use the simdjson builder and the reader
  the error-code form, with a test per refusal code (hard rule 7).
- **FR-022**: A run without the budgets SHALL write no new field,
  give the standing console report and status, and differ in JSON
  bytes only in the `schemaVersion` value (D-9).
- **FR-023**: The console plan line and report SHALL disclose the
  budgets the run used beside the fields spec 029 prints (D-9).

### Documentation and example (D)

- **FR-030**: The README `Hunter mode` section SHALL grow within
  `SECTION_LINE_CAP` (25); `docs/pages/harness.md` SHALL carry the
  option and field detail; the quick-reference and option-table rows
  SHALL move in the shared lists; a README that outgrows
  `PAGE_LINE_CAP` SHALL re-measure and record it through a recorded
  decision (P-5, PC-9).
- **FR-031**: `example/docs/hunt_rank.cpp` SHALL extend to cover the
  promotion round trip inside its scripted, host-independent shape;
  its `.expected` file, its `run_docs_` entry and the `example_run`
  gate SHALL stay host-independent and green (P-5, D-10).

### Version, tests and discipline (E)

- **FR-040**: `CMakeLists.txt` SHALL rise to the next free minor at
  branch time (`0.22.0` while no other lane moves); the harness
  `SOVERSION` SHALL rise from `13` to `14` for the new public call,
  the archive `SOVERSION` SHALL stay `4`, and the ABI check SHALL
  stand recorded beside the version-move commit (hard rule 12).
- **FR-041**: The change SHALL hold 100% line and branch coverage
  with no `LCOV_EXCL` marker and no new `--ignore-errors` entry.
- **FR-042**: Tests SHALL use the scripted seam: no sleep, no clock,
  no fixed cpu or event numbers; the catalog and limits come from the
  injected provider; the suite SHALL pass under `taskset -c 0,1`; no
  lambda stands inside an `SG_*` macro; simdjson alone writes and
  reads JSON; every counter access goes through `sg::counters` (hard
  rules 2, 5, 6, 7 and 15).
- **FR-043**: The timing bounds a test asserts SHALL sit an order of
  magnitude inside the contamination signal spec 032's record fixes.
- **FR-044**: `test/source/harness_cli_table_test.cpp` SHALL keep
  every option name and value unique as the new row joins, its row
  count and hunt-block bound moving in the row's own commit (hard
  rule 10).
- **FR-045**: The commit series SHALL follow the constitution
  template with `Refs: specs/033-hunter-budgets`, and the Zen 5
  acceptance record SHALL stand under
  `specs/033-hunter-budgets/results/` with commands and exit codes in
  spec 034's table shape (D-11).

## Key Entities

- **Sample budget**: the bound on one event's measurement inside a
  stage, held as the canonical `e` clause text rebuilt from the
stored value (for example
  `0.5e`).
- **Multiplex cap**: the bound on the events one multiplexed group
  holds, held as the canonical `g` clause text rebuilt from the
stored value (for example
  `32g`).
- **Derived repeats**: the repeat count the planner derives from the
  calibration estimate under the sample budget, with its overage or
  unpriced disclosure.
- **Group split**: the consecutive multiplex groups of a capped plan,
  with the group count and sizes the run discloses.
- **Promoted counter set**: the emitted file: the run's ranked
  addresses, one per line, best first, readable by
  `--counter=@<file>`.
- **Emit list**: the per-entry ranked address list `toCounterSet`
  answers; the emitted file is the deduplicated union of the emit
  lists.
- **Disclosure fields**: the new budget text fields and derived
  figures on the top-level `hunt` object, read back by `loadReport`.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: In CI with no PMU hardware, a scripted hunt with
  `--hunt-emit-set` writes the ranked file, and a scripted run of
  `--counter=@<file>` reports exactly those addresses as columns,
  none `ABSENT` (criterion 12.7-6, FR-011, FR-014, FR-015).
- **SC-002**: With a fixed estimate and seed, the derived repeats,
  the re-priced estimate and the disclosure match the fixture for
  each of: a fitting budget, a budget below one repeat, an unpriced
  budget, and the two-stage budget composed with the sample budget
  (FR-001, FR-002, FR-005).
- **SC-003**: 200 scripted events with no device limit and a cap of
  32 give seven groups of 32, 32, 32, 32, 32, 32 and 8 in plan order,
  disclosed; a cap of at least 200 gives one group; a `PASSES` plan
  prints unchanged (FR-003; US3 scenario 1).
- **SC-004**: Each of these prints one line naming the value and exits
  2: a malformed budget clause; `--hunt-emit-set` without `--hunt`;
  an empty emit value; `--counter=@` with a missing file, an empty
  address line, or a file holding no address (FR-004, FR-012,
  FR-014).
- **SC-005**: A run without the budgets gives the standing console
  report and status, and JSON bytes identical to master apart from the
  `schemaVersion` value (FR-022).
- **SC-006**: `loadReport` reads every new key equal, and each new
  refusal code carries its own test (FR-021).
- **SC-007**: 100% line and branch coverage with no exclusion marker;
  the suite passes under `taskset -c 0,1`; the sanitize, tsan,
  clang-tidy, cppcheck, format, spell and prose gates stand green
  (FR-041, FR-042).
- **SC-008**: The README `Hunter mode` section stands within 25
  lines; the drift gate passes; the example matches its `.expected`
  on a CI runner (FR-030, FR-031).
- **SC-009**: gary's record under `specs/033-hunter-budgets/results/`
  carries the budgeted `--hunt=all` run and the emitted-set read-back
  with their commands and exit codes; no Intel run exists and the
  record says so (FR-045, D-11).

## Constraints

The hard rules of the hunt lane bind this spec as they bind spec 029,
restated where they bind here:

- coverage with no exclusion (FR-041); test determinism, cpus from
  the affinity mask (FR-042); one-line exit-2 option errors (FR-004,
  FR-012, FR-014); the docs rule with the 25-line section cap
  (FR-030); dbc-gate pairing and no lambda in contracts (FR-013,
  FR-042); simdjson-only JSON with the error-code read form
  (FR-021); options structs, never bools; the option value block with
  named constants and no short option (FR-010, PC-7); one CLI-table
  test file (FR-044); the schema minor reservation and rebase rule
  (FR-020); `SOVERSION` moves on an ABI change with the check
  recorded (FR-040); the CI and rebase mechanics of the lane prompts;
  the coverage preset stays; counters-only (FR-042); the README
  section comes from the runnable example (FR-031); constitution XI
  governs every text.
- Linux stays the supported platform; no new runtime dependency; the
  timing-margin rule (FR-043).

## Assumptions

- The budget clause grammar stands as the answer recorded at the
  clarify pass of 2026-10-28 (Clarifications, D-4) and chosen by the
  lane owner the same day; the branch number no longer waits on it.
  A later clarify or analyze pass may revise it with a new recorded
  answer; the conditional JSON presence and the one-line exit-2
  refusal hold any revision.
- The multi-entry union default (D-6) reads the common single-benchmark
  hunt unambiguously; a batch hunt hands every entry's culprits to the
  next run.
- The sample budget bounds: it caps the stage's own rule repeats and buys no
  extra (D-2).
- Roadmap 12.5's `HuntResult` re-anchors to master's `EntryHunt`
  (D-7).
- The branch cuts from the master tip; the request's named base
  `2a48411` stands covered as an ancestor.
- `corsairone` stays down for the window of this feature, so the Zen 5
  record on gary stands as the item's hardware evidence (request
  rulings, D-11).
- The roadmap criterion 12.7-6 stands cited as this record holds it;
  the roadmap file itself sits outside this machine (Provenance).
- One emitted file per run stands: a hunt run names one destination,
  the same way the JSON report option names one path.

## Constitution Checklist

- Main-page impact check (IV.1): YES. FR-030 and FR-031 carry the
  README section, the extended example region, the quick-reference
  rows and the `docs/pages/harness.md` detail in the same change.
- Coverage (VI): FR-041 holds 100% line and branch with no exclusion.
- Performance (VII): the budgets are plan-side data over the standing
  estimator; the promoted columns ride the standing counter-column
  path; no new critical path appears.
- CI (VIII): FR-041, FR-042 and the Constraints list bind the gate
  set.
- Anti-slop (X): Decisions record each settled reading; the surface
  stays one row and one grammar extension (D-4).
- Prose (XI): this spec follows the discourse rules; the record
  carries no open marker.
