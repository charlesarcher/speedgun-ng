# Research: Hunter Budgets and Promotion (033)

Date: 2026-10-28. Branch `033-hunter-budgets`. Each entry records the
decision, the rationale, and the alternatives considered, per the
plan workflow. The spec's Clarifications and Decisions stand behind
entries D-1 and D-6; nothing here reopens them.

## D-1 The budget clause grammar

**Decision**: one `--hunt-budget=` value of comma-separated clauses;
suffixes `s` (stage seconds, integer), `p` (stage passes, integer),
`e` (sample seconds, fraction allowed), `g` (multiplex cap, whole
count 1 to 10000); each kind at most once; the whole value quoted in
every refusal.

**Rationale**: the lane owner chose this form (spec Clarifications,
2026-10-28). It carries house precedent on both sides it needs:
`--jitter-probe` packs comma clauses into one value
(`specs/027-jitter-probe/contracts/cli-options.md`), and comma
splitting already lives in the hunt selection value. Spec 028's
one whole value per use keeps the grammar atomic; the hunt family
is command-line only (`options.cpp:713-715`), so source replacement
never arises for the budget. `parseSeconds`
(`options.cpp`) proves fractional seconds in house grammar;
`scanHuntNumber` proves the whole-count shape for `g`.

**Alternatives considered**: a repeated option, one suffixed budget
per occurrence (rejected: replacement of a partly-spelled budget set
across sources needs a rule the option does not have); colon
positional clauses (rejected: an unset middle clause needs a stated
empty form); millisecond `e` words (rejected: a second time unit
inside one option whose other time clauses count seconds, against
XI.7's one-term rule).

## D-2 Sample-budget arithmetic

**Decision**: `stageRepeats = min(stageRule, max(1, floor(sampleNs /
(calibrationNsPerPass × passRuns(plan, stage)))))`; `stageRule`
is 1 at stage 1 of a two-stage plan and `options.repeats`
elsewhere. the result re-prices the
estimate and feeds the standing stage-drop decision.

**Rationale**: the divisor is the standing calibration
term against the stage's own window count (`passRuns`,
`hunt_plan.cpp:404`), the same figure family the estimate at
`hunt_plan.cpp:204` computes, so the derivation reads no new data
and no clock (spec 029 D-2's rule,
FR-001). The clamp at 1 is the floor a plan cannot go under; the
clamp at the requested count keeps a budget a bound only.

**Alternatives considered**: per-event cost tables (rejected: no
such data exists and inventing one breaks the no-new-seam rule);
dropping events to fit (rejected: that is the two-stage budget's
job, P-1).

## D-3 The multiplex cap mechanism

**Decision**: a planner-side `muxGroups` table (consecutive groups of
at most the cap in plan order); the run seats one group per
multiplex pass through the standing loop.

**Rationale**: the split is disclosure data the plan must print
before the run, so the planner owns it ahead of the seat. The
standing loop seats passes today; one group per pass reuses it and
keeps spec 029 D-23 (no second pass loop, no second counter path).
The scripted open-failure seam (029 D-18) can answer a too-wide
group in a test if the table ever miscounts.

**Alternatives considered**: capping inside `planPasses` (rejected:
`planPasses` returns the empty plan on a no-limit device and settles
`MULTIPLEX` elsewhere; the cap is a hunt rule and the
planner's own rules stay as shipped);
letting the kernel rotate one over-wide group unchanged (rejected:
that is the status the budget exists to escape).

## D-4 JSON presence is conditional

**Decision**: the four keys (`sample_budget`, `multiplex_cap`,
`derived_repeats`, `mux_groups`) are written only when their budget
stands; they join `properties` and `x-order`, and stay out of
`required`.

**Rationale**: FR-022's byte rule (an unset run writes the standing
bytes apart from `schemaVersion`) requires absence.
Absence also keeps pre-1.7 hunt documents valid and loadable with no
reader version branch, matching spec 029's recorded deviation of the
hunt keys from `required`. Key-name style follows the snake family
already inside the node (`estimate_ns`, `planned_events`).

**Alternatives considered**: null-when-unset like the standing
`budget` key (rejected: it writes new bytes on runs that never asked
for a budget, breaking FR-022 and the pre-1.7 byte-diff test);
nested `budgets` object (rejected: a new closed node for three
scalars, against X.2).

## D-5 Canonical clause text for disclosure and JSON

**Decision**: the writer and the console rebuild the clause text from
the stored value (whole nanoseconds for `e`, whole count for `g`),
the same way `writeHunt` rebuilds `budget` from `amount` plus kind
(`json_report.cpp:706-732`).

**Rationale**: round-trip equality (FR-021) cannot depend on the
caller's spelling (`0.50e` versus `0.5e`); one rebuild site keeps
console and document stating one text.

**Alternatives considered**: storing the raw text in `HuntOptions`
(rejected: two facts for one budget, a second source of truth).

## D-6 The promoted set

**Decision**: `EntryHunt::toCounterSet(topN)` answers the ranked
addresses, rank ascending, capped at `topN`; the CLI file is the
deduplicated union of the per-entry lists in entry order (spec D-6,
D-7).

**Rationale**: rank order is the item's whole point (culprits first);
the union covers a batch hunt without a new concept, and the single
benchmark case (the common hunt) is unaffected. The method joins the
record that already owns the ranks; `RunHunt` holds no ranks today.

**Alternatives considered**: a `RunHunt` method unioning internally
(rejected: `RunHunt` owns no per-entry ranks; growing it to own them
duplicates `EntryHunt`); emitting only the first entry (rejected:
silent loss on a batch run).

## D-7 The shared list-file reader

**Decision**: `readSelectionFile` moves to
`detail::readAddressFileLines(path, context)`; the hunt selection and
the `counter` gather arm both read through it; each route keeps its
own refusal sentence.

**Rationale**: the spec fixes one list-file grammar for both options;
the header `detail/hunt_selection.hpp` already states the rule that
"what decides whether a text selects what it claims stands here
once." The `@` interception sits in the gather arm so the three
option sources carry one grammar.

**Alternatives considered**: a second reader for `--counter`
(rejected: two homes for one grammar); reusing
`EventCatalog::expand`'s library route (rejected: the library route
refuses the empty line outright and the file rules differ on
comments).

## D-8 Emission point and failure shape

**Decision**: `writeHuntEmitSet` runs from `cli.cpp` after the batch,
beside the `writeReport` call; an unwritable destination prints the
standing report-file sentence and sets status 2; the success path
leaves status untouched.

**Rationale**: the ranked records exist on `ReportData` only after
the batch; the report failure already answers "a named file that
cannot be written" with exactly the line-plus-status-2 shape the
spec cites (PC-5), so the run needs no new failure rule.

**Alternatives considered**: writing inside `runHuntNamed` per
benchmark (rejected: the emit is a run-level union; a per-entry
write would truncate the earlier entries).

## D-9 The example's promotion demo

**Decision**: the extended `hunt_rank.cpp` builds a ranked record
through the pure `rankEvents` over scripted samples and prints the
`toCounterSet` answer; the `.expected` re-records.

**Rationale**: on a host without PMU access the live hunt ranks
nothing (every event skips), so a live-run demo would print an empty
file everywhere; the pure rank is host-independent and is the same
function the run calls.

**Alternatives considered**: showing the CLI file pair in the example
(rejected: a docs example runs one binary with fixed argv; the pair
belongs to the quickstart and the acceptance record).
