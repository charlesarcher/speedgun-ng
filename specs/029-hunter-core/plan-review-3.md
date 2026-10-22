# Plan review 3: the 029 artifacts after the second remediation

**Audit point**: `5f4be45` ("Docs: Close the second 029 review round"), branch
`029-hunter-core`.
**Inputs**: [spec.md](spec.md) as the authority, [plan-review-2.md](plan-review-2.md)
as the finding list (findings 11, 13, 18 plus N-1 to N-7).
**Method**: each of the ten items checked against the committed text at the line
that carries it, with every code citation opened at the cited site
(`dbc.hpp:10-12`, `runner.cpp:2202,2301`,
`counters_measurement.hpp:404-414`). Decision-token discipline checked by
grep over both prose files, every surviving bare `D-nn` read against the spec
heading it cites. Table integrity checked mechanically over all eight artifacts.
No file outside this one was touched.

**Result in one line**: all ten items are closed in the committed text and the
regression sweep is clean; nothing in this round reaches the bar of an
implementer building the wrong thing.

## Re-check of the ten open items

1. **f11: FIXED.** `contracts/hunt-cli.md:99-102` now reads "A hunt option with
   no `--hunt` keeps the hunt off: the value parses, the run behaves like a run
   with no `--hunt`, and a scripted value round-trips exactly like
   `--hunt=none` does (spec Assumptions)", which is the spec's own reading at
   spec.md:832-833 (`--hunt=none` is the same as passing no `--hunt`; it exists
   so a scripted value round-trips) and no requirement refuses the bare option.
   The refusal table (`contracts/hunt-cli.md:77-90`) holds no row for it, the
   conflict table holds only `--counter`, `--metric` and `--compare`
   (`contracts/hunt-cli.md:95-97`), and the plan's refusal fixture list
   (plan.md:504) carries neither the bare option nor a hunt option without
   `--hunt`.
2. **f13: FIXED.** The plan series is `PD-01` to `PD-20` at every site: the
   decision table at plan.md:270-290, the artifact map at plan.md:176, the phase
   table at plan.md:466-475 (`PD-20, PD-18` ... `PD-19`), and the cross-file
   cites at data-model.md:154, :174, :190, :271, :382, :388,
   contracts/hunt-api.md:299, :351, contracts/hunt-cli.md:153,
   contracts/hunt-json.md:111, :138. research.md headers are
   `## PD-01` (research.md:12) through `## PD-20` (research.md:581).
   Grep for a bare two-digit `D-nn` outside `PD-` returns plan.md:150 (`D-23`),
   :235 (`D-21`), :251 (`D-23`), :304 (`spec D-22`), :416 (`spec D-15`), :496
   (`D-12`), :504 (`D-13`), :505 (`D-16`), :527 (`D-11`), :545 (`spec D-22`) and
   research.md:20, :32, :352 (`D-23`), :114 (`D-13`), :336, :352 (`D-18`), :462
   (`D-11`), :486, :494, :510, :511 (`D-22`). Every one of those numbers is
   read against its spec heading (spec.md:102 `D-11` raw codes, :105 `D-12`
   no-PMU hosts, :107 `D-13` anchors, :112 `D-15` library surface, :117 `D-16`
   conflicts, :126 `D-18` group-open failure, :140 `D-21` the CLI table test,
   :144 `D-22` option wiring, :149 `D-23` planning through the N6 planner) and
   carries that content, not plan-decision content. No `PD-` token above
   `PD-20` exists.
3. **f18: FIXED.** research.md:317-322 (PD-11 rationale) reads "the default
   `enforce` contract semantic (the default value 2 of `SG_CONTRACTS_SEMANTIC`
   at `dbc.hpp:10-12`)"; opened `include/speedgun-ng/dbc.hpp:10-12`, which is
   exactly `#ifndef SG_CONTRACTS_SEMANTIC` / `#  define SG_CONTRACTS_SEMANTIC 2`
   / `#endif`, and `detail::enforce` at dbc.hpp:154 is the `[[noreturn]]`
   response, so the abort the decision rests on belongs to `enforce`. The
   dispatch for the default value 2 is the `#elif SG_CONTRACTS_SEMANTIC != 0`
   branch at dbc.hpp:198-213, which reports the record and then calls
   `std::abort()`; grep for `observ` in research.md returns no line at all, so
   the obsolete `observe` claim is gone.
4. **N-1: FIXED.** `contracts/hunt-api.md:192-197` restricts the
   `\post` (refusal) to three causes (a device limit below 3, an explicit
   `norm` whose anchor stands refused, a `source` index outside `sourceLimits`)
   and `contracts/hunt-api.md:199-203` states the other half: "An empty `events`
   list is not a refusal: it is the shape of a host whose discovery refused
   every event, and `planHunt` then answers zero columns, empty `passes`,
   `anchorsPerPass` zero and `normalized` false, so the run prints the plan line,
   the skipped list and the `events=0/N` summary and exits 0 (D-12, FR-041)".
   The console table carries the row: `contracts/hunt-cli.md:157` "Host that
   refuses everything | the plan line, no ranked row, the skipped list with one
   line per refused event, and the summary reading `events=0/N`, the plan line
   carrying `events=0`". The line shapes at data-model.md:195 and :201 can print
   both. Matches spec.md:105-106 (D-12) and the CI-host assumption at
   spec.md:821-826.
5. **N-2: FIXED.** plan.md:504 (CLI refusals, P8) lists bad glob, unknown
   address, bad budget, bad norm, bad rank key, repeats and top bounds, `r4a03`,
   the three conflicts, missing `@file`, empty address line, the explicit-norm
   refusal and the below-3 device; no bare `--hunt`. The legal row at
   plan.md:505 ends "a bare `--hunt` (selects the whole core catalog, D-1)",
   matching spec.md:66 (D-1), `contracts/hunt-cli.md:26` (`optional_argument`)
   and :42-43.
6. **N-3: FIXED.** `contracts/hunt-cli.md:116` states the accepted
   combination as "the plan line prints, every pass runs one iteration
   (`runner.cpp:2202,2301`), and each column reaches the table with one sample,
   a `cv` of `0.0` and no rank, because an unrepeated event carries none (I-5)";
   the console row at :159 and the verification row at :184 say the same, and
   plan.md:505's dry-run case matches word for word. Opened the two cited sites:
   runner.cpp:2202 is `const std::uint64_t repetitionsOfRun = dryRun ? 1 :
   repetitions;` and runner.cpp:2301 is `std::uint64_t iterations = dryRun ? 1 :
   settled;`, so the pass loop runs and the row prints. No "exits before
   measuring" claim survives anywhere (grep for `before measuring` is empty in
   the eight artifacts).
7. **N-4: FIXED.** `contracts/hunt-api.md:74` holds
   `std::optional<HuntNorm> norm;` and data-model.md:62 (E-03) holds
   `std::optional<HuntNorm>` with the default "empty, which plans `instr`" and
   the note "a set value marks the word explicit (D-13)". The refusal for an
   explicit word whose anchor is refused is written where the CLI refuses:
   `contracts/hunt-cli.md:84-85` gives both lines
   (`--hunt-norm: 'cycle' has no countable anchor on this host` and the `instr`
   twin, each tagged D-13), `contracts/hunt-cli.md:155` marks the silent
   fallback as belonging to the unset norm only, `contracts/hunt-api.md:193-194`
   makes it a `planHunt` refusal, plan.md:504 carries it as a P8 fixture,
   quickstart.md:74-76 walks the case, and `contracts/hunt-api.md:381` lists it
   as a verification row. That closes the mechanism gap: a defaulted value and a
   written `instr` are no longer the same bytes.
8. **N-5: FIXED.** No artifact claims the two lists are disjoint. I-4 at
   data-model.md:287-290 now states only "An address stands at most once in
   `addresses()`, at most once in `entries`, and at most once in `skipped` for
   one run. Every planned address holds one `entries` record", and
   `contracts/hunt-api.md:300` states the overlap outright: "an address that
   measured and gapped stands in both (I-4, D-10)". The two produced-no-value
   clauses are restated as availability causes at data-model.md:140 and
   `contracts/hunt-json.md:115` ("one object per event whose open was refused or
   whose window disclosed a non-`COUNTABLE` availability; an event that measured
   and gapped stands here and in `entries` (D-10)"). The 19-clean-plus-gap case
   is a verification row at `contracts/hunt-api.md:385` in
   `harness_hunt_collect_test.cpp`. The only surviving "no address stands in
   both members" is `contracts/hunt-api.md:128`, the `split` postcondition at
   discovery, where no measured entry exists yet - a different pair of lists,
   not the run's.
9. **N-6: FIXED.** data-model.md:110-113 (Limit rule) reads "`L` below 3 with
   both anchors present means no event fits: `planHunt` answers a recoverable
   error naming the device and the limit, the caller prints one line and the run
   exits 2 (FR-081)"; quickstart.md:72-73 says the same for limit 1 or 2;
   `contracts/hunt-cli.md:86` adds the refusal row
   (`hunt: device 'cpu' publishes 2 slots and a hunt pass needs 3`) under the
   section rule at `contracts/hunt-cli.md:73-74` ("Every refusal prints exactly
   one line naming the value to stderr and exits with status 2 (FR-081)");
   plan.md:504 carries "a device limit below 3 (FR-081)". Grep for `empty
   result` across the eight artifacts returns nothing, so the empty-result
   reading is gone, and the refusal needs no `HuntPlan` reason field.
10. **N-7: FIXED.** data-model.md:180 cites
    `counters_measurement.hpp:406-413`, "disclosed by `fold` and" for the window
    gap state. Opened the file at 404-414: the comment states the end-point-only
    semantics of `availability`, names `fold` and `foldPairs` as the calls that
    "disclose each window's own gap state (FR-004, FR-005)" and gives `ratio` as
    "meaningful only when `availability != Availability::GAP`".
    `contracts/hunt-api.md:289` repeats the same citation beside the `GAP`
    enumerator, which is what the round-two fix asked for. Grep for
    `counters_measurement` across the artifacts returns only those two lines.

## Regression sweep

**(a) The six shared facts still agree across the eight artifacts.**

- Repeats default `3`: data-model.md:66, contracts/hunt-api.md:78
  (`std::size_t repeats = 3;`), contracts/hunt-cli.md:31, the document example
  at contracts/hunt-json.md:47, and quickstart.md:127, whose "50 stage-1 passes
  plus 9 more pass runs, 59 in total" is derived from `--hunt-repeats` 3.
  Grep for `repeats = 10` and `default 10` across the eight files returns
  nothing.
- `noisePercent`: contracts/hunt-json.md:38 and :61, the schema `x-order` at
  contracts/report-schema-key.md:67, the mapping row at data-model.md:375. No
  `noise_pct` or `noisePercentage` variant survives.
- `RunHunt` / `runHunt`: data-model.md:207 (E-08) and the mapping rows at
  :373-381, plan.md:279 (PD-10, `hunt::RunHunt` on `ReportData`), plan.md:407
  (`runHunt` in `hunt.hpp`), contracts/hunt-api.md:52 and :351. No `HuntRun` or
  `run_hunt` spelling exists.
- Option values 270 to 277: plan.md:286 (PD-17) and :413, research.md:482-486
  (the eight named constants, 270 to 277), the value column of
  contracts/hunt-cli.md:26-33, quickstart.md:53 and :212. The reserved block is
  270 to 279 at every one of those sites, with 278 and 279 left unused
  (research.md:486).
- Five skipped kinds: data-model.md:176-177 (`PERMISSION_BLOCKED`,
  `NOT_ENCODABLE`, `ABSENT`, `SCOPE_REFUSED`, `GAP` - the five `Availability`
  values and nothing else), contracts/hunt-api.md:225-231 with the one named
  mapping at :285-289, the five console spellings at
  contracts/hunt-json.md:117, plan.md:496 (the four discovery kinds plus
  mid-run `GAP`), quickstart.md:80-82.
- Stage rule: data-model.md:102-107, contracts/hunt-api.md:185-189 (the `\post`)
  and :215-220 (the budget rule), plan.md:282 (PD-13), quickstart.md:121-125,
  and the printed cut line at data-model.md:197. All read: one stage at
  `options.repeats` with no budget; with a budget stage 1 covers every event at
  one repeat and stage 2 the top `3 x top` at `options.repeats`; an
  unfittable budget drops stage 2 whole with one printed cut line; no stage ever
  keeps a reduced repeat count.

**(b) Sampled cites, ten of them, token against content.** plan.md:235 `D-21`
against spec.md:140 (this lane creates `harness_cli_table_test.cpp`) - match.
plan.md:251 `D-23` against spec.md:149 (plan through the N6 planner) - match.
plan.md:304 and :545 `spec D-22` against spec.md:144 (option wiring stays
split) - match. plan.md:416 `spec D-15` against spec.md:112 (library surface in
`hunt.hpp`) - match. plan.md:496 `D-12` against spec.md:105 (no-PMU hosts skip
everything and exit 0) - match. plan.md:504 `D-13` against spec.md:107
(anchors, and the explicit-word rule) - match. plan.md:505 `D-16` for the legal
`--metric-method=multiplex` combination against spec.md:117-120, whose third
sentence is exactly that case - match. research.md:114 `D-13` in PD-04 (no
invented alias) - match. research.md:336 and :352 `D-18` in PD-12 against
spec.md:126 (group-open failure retries the events) - match.
contracts/hunt-api.md:288 `D-10` against spec.md:96 (skipped events) - match.
No `PD-` token is used where a spec decision was meant either: the six
`PD-` cites outside the two prose files (data-model.md:154, :174, :190, :271,
:382, :388, contracts/hunt-api.md:299, :351, contracts/hunt-cli.md:153,
contracts/hunt-json.md:111, :138) each name plan content - PD-14 single-sample
events, PD-12 retry, PD-15 console order, PD-20 version and schema, PD-10 hunt
data location.

**(c) Table integrity.** A script over all eight artifacts (plan.md, research.md,
data-model.md, quickstart.md and the four contracts) flags any table row that
does not end with a pipe and any non-blank line that follows a table row without
a leading pipe: zero hits in all eight files, so no row is wrapped mid-row.

## New findings

None. Nothing in the committed text would make an implementer build the wrong
thing.

Noted, below the finding bar - one line each, no counted finding:

1. contracts/hunt-api.md:128 attributes "no address stands in both members" to
   I-4 for `split`; I-4 (data-model.md:287-290) no longer states it. The claim is
   true of the two discovery lists, which is why it is a note and not the N-5
   shape again.
2. contracts/hunt-cli.md:153 cites `D-7` for the ranked-table size; the top-N
   cap is spec D-8 (spec.md:92) while D-7 (spec.md:88) covers the `noisy` mark of
   the same row. contracts/hunt-cli.md:31-33 carries the D-8 range on the option
   row itself, so the number an implementer reads is right.
3. The limit rows in the verification table at contracts/hunt-api.md:379 and the
   P4 exit gate at plan.md:469 list "1, 3, 4, 6, 8, no limit" without repeating
   that the 1 and 2 rows are exit-2 refusals; quickstart.md:70-73 and
   data-model.md:110-113 state each row's expectation, so the fixture is
   recoverable from either.
4. `--hunt-norm` shows the default `instr` in the table column at
   contracts/hunt-cli.md:27 while the field is `std::optional<HuntNorm>`
   (contracts/hunt-api.md:74). The column reads as the plan default, and
   contracts/hunt-cli.md:155 draws the unset-versus-explicit line, so nothing
   contradicts D-13.

VERDICT: PASS
