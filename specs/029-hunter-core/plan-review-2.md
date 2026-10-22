# Plan review 2: the 029 artifacts after remediation

**Audit point**: `bd10d76`, branch `029-hunter-core` at `568a124`.
**Inputs**: [spec.md](spec.md) as the authority, [plan-review.md](plan-review.md)
as the round-one ledger (37 findings), and the eight artifacts: [plan.md](plan.md),
[research.md](research.md), [data-model.md](data-model.md), [quickstart.md](quickstart.md),
[contracts/hunt-api.md](contracts/hunt-api.md), [contracts/hunt-cli.md](contracts/hunt-cli.md),
[contracts/hunt-json.md](contracts/hunt-json.md),
[contracts/report-schema-key.md](contracts/report-schema-key.md).
**Method**: every artifact re-read whole; every code citation opened at the cited
line in this tree; every bare `D-n`, `FR-0nn`, `PC-n`, `AC-n` and `SC-0nn`
re-mapped against the spec headings; the orchestrator rulings supplied with this
round checked as binding overrides, one by one: the repeats default of 3, the
one-stage shape of an un-budgeted plan, the whole-stage drop with no invented
reduction, the `armv8_` device-part rule, `runHunt` public in `hunt.hpp`, the
five skipped kinds with their errno mapping, the eight option constants inside
the 270 to 279 block, and `none` never being refused. Nothing was taken from an
artifact's own quotation of itself.
Nothing in this checkout was edited, built or committed, so this review lives at
`/tmp/plan-review-2.md` rather than beside the round-one ledger; say the word and
it moves to `specs/029-hunter-core/plan-review-2.md`. Not checked, because it is
outside the artifacts: whether the spec's own 151-refused hardware record on
corsairone still matches the 021 record it cites, and whether `harness_cli_test`
currently passes (a 57-minute suite, outside this round's runs).

**Result in one line**: 34 of the 37 round-one findings are fixed in the
committed text; findings 11, 13 and 18 are not; seven new defects were found, one
of which (N-1) makes the CLI refuse the host class the whole test suite runs on.

## Re-check of the 37 findings

1. FIXED. The repeats default reads `3` at data-model.md:66, hunt-api.md:77 and
   hunt-cli.md:31, and the quickstart walkthrough derives its pass counts from
   3 (quickstart.md:122).
2. FIXED. data-model.md:102 states one stage at `options.repeats` when no budget
   stands, hunt-api.md:184-186 states it in the `\post`, and data-model.md:343
   states it in the lifecycle table; no artifact keeps "stage 1 at one repeat"
   for a plan without a budget.
3. FIXED. hunt-api.md:208-212 and data-model.md:106-108 hold the ruling verbatim:
   stage 2 drops whole with one printed cut line, stage 1 never shrinks, no
   repeat count is reduced; the surviving bare `D-21` (plan.md:235) names the
   spec's CLI-table-file decision, which is what that decision is.
4. FIXED. quickstart.md:110-125 now prints the 50-pass plan line, the cut line,
   and states that stage 1 still runs all 200 events; the `60p` reading gives 50
   plus 9 pass runs, 59 total (quickstart.md:122).
5. FIXED. research.md:51-53 reads the `armv8_` prefix off the device part of the
   address, data-model.md:25 and hunt-api.md:129 say the same, and the proof is
   a scripted device path (research.md:64-66, quickstart.md:79-81).
6. FIXED. hunt-api.md:301 states `runHunt` public in `hunt.hpp`, the signature in
   the plan table matches (plan.md:407), and no `hunt_run.hpp` or
   `hunt_collect.hpp` exists anywhere in the directory (grep: absent).
7. FIXED. hunt-json.md:65-66 carries top-level `events` and `stages`,
   report-schema-key.md:67-69 places both in the `x-order`, and data-model.md:375
   maps `RunHunt::plannedAddresses` to `hunt.events`.
8. FIXED. `noisePercent` at hunt-json.md:38 and :61, in the schema `x-order`
   (report-schema-key.md:67) and in the mapping row (data-model.md:374).
9. FIXED. The hunt-object table (hunt-json.md:102-118) gives the six `entries[]`
   keys at 109 to 114 with no per-entry `estimate_ns` and no per-entry `budget`;
   the only `budget` and `estimate_ns` rows are the top-level ones at
   hunt-json.md:63 and :67, and the mapping table dropped the old "EntryHunt
   budget and estimate summary" row (data-model.md:372-390).
10. FIXED. data-model.md:154 and I-6 (data-model.md:293) state the factor one
    hundred against the printed `cv=` column, hunt-json.md:111 repeats it, and
    plan.md:508 pins it in `harness_statistics_test` plus the rank test
    (hunt-api.md:373).
11. NOT FIXED. The `--hunt=none` half is right (hunt-cli.md:52, :113, :181,
    data-model.md:324), but hunt-cli.md:97-98 still refuses "a hunt option with
    no `--hunt`" with no spec id behind it, which is the third invented option
    rule of this finding and contradicts the scanner's own last-wins reading.
12. FIXED. hunt-cli.md:111 carries the legal `--metric-method=multiplex`
    combination and plan.md:505 puts it in the legal-combination fixture.
13. NOT FIXED. The plan series was renumbered `D-01` to `D-20`, but the
    disambiguation rule (plan.md:264-266, research.md:7) only covers the padded
    `D-01` to `D-09`; plan decisions `D-10` to `D-20` are spelled exactly like
    spec decisions `D-10` to `D-20`, so plan.md:135, :303, :320 and the whole
    phase table (plan.md:466-475) plus research.md:314 read as spec decisions
    with different content. Note 1 below gives the fix.
14. FIXED. `SkippedKind` holds the five Availability values and nothing else
    (data-model.md:176-184, hunt-api.md:217-223), with the errno mapping the
    ruling names (`EINVAL`/`EOPNOTSUPP` to `NOT_ENCODABLE`, `EACCES`/`EPERM` to
    `PERMISSION_BLOCKED`); `GROUP_REFUSED` is absent from the directory.
15. FIXED. research.md:57-61 cites `counter_limits.cpp:284-295` and
    `provider.cpp:352,447-448` and states plainly that no core-device name table
    exists to read; opened both files: `limitForDevice` is defined at
    counter_limits.cpp:288 and matches `cpu`, `cpu_core`, `cpu_atom` at 294-295,
    and provider.cpp:352 and :447-448 carry the same three parts.
16. FIXED. The out-of-range `provider.cpp:1502` is gone; the replacement cites
    (352, 447-448) sit inside the 963-line file and say what the claim needs.
17. FIXED. research.md:62-66 no longer attributes the `armv8` strings to
    `json_load.cpp:868`; it states the prefix rule is new hunt-side code and
    points the proof at the scripted device. Neither string is in the harness
    reader (grep over `source/harness/`: no `armv8`).
18. NOT FIXED, half moved. The citation is real now: research.md:317-321 cites
    `include/speedgun-ng/dbc.hpp:128-135,168,213` and all four sites are what
    they say (stderr report at 128-135, `std::abort()` at 168 and 213). But
    research.md:319 calls the default semantic `observe`, and `observe` is
    semantic 1, whose dispatch reports and returns with no abort
    (dbc.hpp:171-185); the default is 2, `enforce`, set at dbc.hpp:10-12 and
    documented at `cmake/dbc.cmake:13` and `docs/pages/dbc-migration.md:18`. The
    abort D-11 rests on belongs to `enforce`, which is what spec PC-9 names.
19. FIXED. research.md:137-141 cites `source/harness/pass_plan.cpp:404-430`
    (opened: `passSpread` spans 404-430) and places the counters median at
    `source/counters/plan.cpp:309` (opened: `costs[costs.size() / 2]`, the upper
    central element, which is what `statisticsOf` computes at runner.cpp:68-69).
20. FIXED. Every bare id in the six artifacts now lands on the heading that
    holds its claim: core rule D-1, norm D-6, rank key D-3, statistics FR-030
    plus PC-4, column cap PC-2, budget D-2, multiplex settling D-19, anchors
    PC-8, device parts PC-1, raw codes PC-6, option range D-22, estimate FR-012,
    no-hunt identity FR-052, round trip AC-6 plus SC-006, table test FR-083. One
    residue, in the notes below: plan.md:66 writes `SC-06` and `SC-07`, ids the
    spec does not have.
21. FIXED. `FR-015` is gone from the directory; rankEvents now cites FR-014 with
    D-9 (hunt-api.md:297, research.md:203), which is the deterministic-order
    requirement.
22. FIXED. hunt-api.md:4 reads "Spec: FR-001 to FR-033, D-15, hard rules 5 and 6";
    the FR-061/FR-062 references are gone.
23. FIXED. hunt-cli.md:5-6 reads "FR-080 to FR-083, D-1, D-9, D-10, D-11, D-12,
    D-16, D-21, D-22, hard rules 2, 3, 9 and 10": the Rank block is dropped and
    D-16 (the conflict list the file tabulates) is cited. The bare D-10/D-11 in
    that header is the collision of finding 13.
24. FIXED. plan.md:177 indexes "E-01 to E-11", and plan.md:201-202 tag
    detail/statistics.hpp and statistics.cpp with (E-11).
25. FIXED. plan.md:190-191 reads "Four new source files, one new public header,
    two new internal headers, seven new test translation units", matching the
    tree at plan.md:201-235; the third internal header hunt_run.hpp appears
    nowhere in the directory.
26. FIXED. plan.md:190 counts seven test translation units, plan.md:466 gates on
    "the seven test executables registered", and the shared-list row plan.md:456
    now states harness_cli_table_test.cpp is "created here ... no other lane
    creates it" (also plan.md:235).
27. FIXED. data-model.md:7-9 names the two internal records,
    `detail::StageSplit` and `detail::RetriedPass`, and hunt-api.md:344-346
    declares the rest public in `hunt.hpp` with those two in
    `source/harness/detail/hunt_plan.hpp`.
28. FIXED. spec.md:765 now reads "harness 10 to 11, archive 3 to 4", and the
    plan's ABI rows cite FR-090 rather than the constraint line
    (plan.md:431-432, with FR-090 restated at plan.md:72 and :425).
29. FIXED. The stage-1-only case is settled the same way in all four places:
    research.md:374-378, data-model.md:172, :289 and the lifecycle row
    data-model.md:344, hunt-api.md:325 and its verification row hunt-api.md:374,
    hunt-json.md:108 ("one object per planned column, including a column only
    stage 1 measured"), and the fixture plan.md:503.
30. FIXED. hunt-cli.md:55 accepts `--hunt=uncore*` as "a well-formed pattern
    that selects every catalog address starting with `uncore`", and the
    invented well-formedness refusal is gone: the only glob refusals left are a
    pattern that matches nothing and an empty address line, both in the refusal
    column of the option row at hunt-cli.md:26, which is what spec.md defines.
31. FIXED as to the contradiction, wrong as to the code: `--dry-run` is now a
    legal combination everywhere (hunt-cli.md:112, :155, :180, plan.md:505) and
    the conflict check lists only `--counter`, `--metric` and `--compare`
    (hunt-cli.md:92-94, verification row hunt-cli.md:178), so the contract no
    longer both refuses and honours it. What the new text asserts about the run
    is false; see N-3.
32. FIXED. hunt-cli.md:143 prints `hunt: skipped armv8_pmuv3/cycle
    not-encodable stage=2` and hunt-json.md:97 writes
    `"address": "armv8_pmuv3/cycle"`, both in the composed `path + "/" + name`
    form, so a fixture copied from either survives `splitAddress`.
33. FIXED. One separator rule now: "commas, newlines or both" at
    hunt-api.md:125 and data-model.md:21, with the `@file` row of
    hunt-cli.md:56 stating "newline or comma separated".
34. FIXED. plan.md:279 names `hunt::RunHunt`, matching data-model.md E-08
    (data-model.md:206) and the `ReportData` member the contract declares, and
    `RunPlan` is gone from the directory.
35. FIXED. quickstart.md:181-182 read `cmake --preset=ci-coverage` then `cmake
    --build build/coverage --target coverage`, and quickstart.md:184-186 name
    the `build/coverage` tree as the boundary.
36. FIXED. The `dev` preset sentence is gone; quickstart.md:25 states a fresh
    clone carries no `CMakeUserPresets.json` and leaves `ci-rocky` and
    `ci-ubuntu` as the routes, which is what AGENTS.md requires.
37. FIXED. hunt-cli.md:132 reads `estimate=1260000ns` for 14 passes at one
    repeat, 90000 ns per pass, an exact integer under the estimate rule at
    data-model.md:358.

## New findings

Ordered by how badly an implementer following the text builds the wrong thing.
None of these is in the round-one list.

### N-1. The empty-selection plan refusal makes the no-PMU host exit 2, which the spec forbids

Location: `contracts/hunt-api.md`:192-195 (the `\post` (refusal) of `planHunt`),
read against spec.md:105-106 (D-12), spec.md:608-610 (FR-041), spec.md:402-404
(AC-4 scenario 4), spec.md:823-826 (the refused-catalog edge, and "CI runners
have no PMU access"), `contracts/hunt-cli.md`:153 and data-model.md:202-204.

Problem: hunt-api.md:192-195 makes "an `events` list that is empty" a
recoverable error and says "the caller prints one line naming the value and
exits 2 (FR-081)". The empty list is exactly the shape of a host that refuses
everything: `split` hands back no countable address for `--hunt=all`, so
`planHunt` is called with an empty `events` and, under this line, the run exits
2. FR-041, D-12, AC-4 scenario 4, hunt-cli.md:153 and data-model.md:202-204 all
require the opposite for that host: the plan prints, every event stands in the
skipped list, the summary reads `events=0/N`, and the run exits 0. spec.md:825
puts every CLI-level hunt test on exactly that class of host, so the first run
of the suite walks into the contradiction. The same postcondition also has to
carry FR-081, which is about a bad option value and says nothing about a host
that refuses everything.

Fix: split the two cases at hunt-api.md:192-195. A selection that names no event
at all (an empty `--hunt=` list, an `@file` with no address line) stays the
exit-2 refusal. A selection whose events are all refused at discovery is not a
refusal: `planHunt` answers a `HuntPlan` with zero columns, empty `passes`,
`anchorsPerPass` 0 and `normalized` false, and the CLI prints the plan line, the
skipped list and the `events=0/N` summary and exits 0 (FR-041, D-12). Then say in
hunt-cli.md's console table which line a zero-column plan prints, and name the
exit code for the other two refusals of the same postcondition (a device limit
below 3, see N-6).

### N-2. The test plan refuses a bare `--hunt`, the form D-1 is written around

Location: plan.md:504 (the "CLI refusals (P8)" fixture row, last case "`--hunt`
with no value"), read against `contracts/hunt-cli.md`:26 and :51 and
data-model.md:322.

Problem: hunt-cli.md:26 declares the option `optional_argument` with "no value"
in the accepted column, hunt-cli.md:51 and data-model.md:322 both read a bare
`--hunt` as selection `all`, and that is the spec's D-1 selection rule.
plan.md:504 hands P8 a refusal fixture for the same form. `tasks.md` and the
first CLI commit inherit the fixture, and `--hunt` with no value exits 2 on the
flag the rest of the feature is documented behind.

Fix: drop "`--hunt` with no value" from the refusal list at plan.md:504 and put
it in the legal-combination row at plan.md:505 as "bare `--hunt` selects the
whole core catalog (D-1)".

### N-3. `--dry-run` is described as skipping the measurement it exists to perform

Location: `contracts/hunt-cli.md`:112, :155 and :180, plan.md:505, against
source/harness/runner.cpp:1094, :2202 and :2301, and README.md:757.

Problem: all four lines say the hunt `--dry-run` "prints the plan line and the
run exits before measuring", and hunt-cli.md:155 concludes "the ranked table,
the excluded tail and the summary have no samples to print". `--dry-run` does no
such thing: runner.cpp:2202 sets the repetitions of a dry run to 1 and
runner.cpp:2301 settles one iteration, so the pass loop runs and the row prints;
README.md:757 documents the option as "Runs one iteration and prints the row
shape". A hunt on top of it therefore collects real single-iteration samples:
one `HuntSample` per column, a `cv` of `0.0` for a single sample
(data-model.md:154), a table whose rows are all unrepeated, and a summary line.
Following the contract as written means either inventing an early exit for
`--hunt --dry-run` that no requirement asks for, or writing the
`harness_hunt_cli_test.cpp` case at hunt-cli.md:180 against a console the runner
cannot produce.

Fix: state what the code does. `--dry-run` composes with the hunt: the conflict
rows are `--counter`, `--metric` and `--compare` only (hunt-cli.md:92-94, and the
conflict-check verification row at hunt-cli.md:178 lists the same three), so
adding `--dry-run` there is not on the table. What should read instead is: the
plan line prints, every pass runs one iteration, and each column reaches the
table with one sample, a `cv` of `0.0` and no rank, because an unrepeated event
carries none (hunt-api.md:291, I-5). Restate hunt-cli.md:112, :155 and :180 and
plan.md:505 in those terms and cite runner.cpp:2202 and :2301 as the seam.

### N-4. The exit-2 refusal D-13 requires is missing everywhere, and nothing can carry it

Location: spec.md:107-109 (D-13) and spec.md:512, against
`contracts/hunt-cli.md`:27 and :83, hunt-api.md:70-80 (`HuntOptions`),
data-model.md:374 and plan.md:504.

Problem: D-13 makes the `iter` fallback silent by default and says
"`--hunt-norm=instr|cycle` exits 2 only when the user set it", and spec.md:512
repeats it as the refusal for an explicitly requested norm. No artifact carries
the rule: hunt-cli.md:27 and :83 refuse only a word outside `instr`, `cycle` and
`iter`; hunt-cli.md:151 makes a missing anchor print `normalized=iter` and carry
on; hunt-api.md's planHunt postcondition refuses only a limit below 3, an empty
list and a bad source index; plan.md:504's "bad norm" case is the bad word. So
`--hunt=all --hunt-norm=cycle` on a device without `instructions` silently
reports iteration normalization where the spec wants exit 2 naming the cause.
The mechanism is missing too: hunt-api.md:73 holds `HuntNorm norm =
HuntNorm::INSTR;` and hunt-cli.md:27 gives the same default, so a defaulted
value and a user-written `instr` are the same bytes and "only when the user set
it" cannot be evaluated at all.

Fix: carry the explicitness. Make `RunOptions` (data-model.md E-09) and
`HuntOptions` hold `std::optional<HuntNorm>`, empty for the default, and test it
where the anchors resolve: a set norm of `instr` or `cycle` whose anchor of that
name is missing is a recoverable error naming the value, exit 2. Add the refusal
line to hunt-cli.md beside :83, the case to plan.md:504, and a quickstart row
beside :72.

### N-5. An event that gaps mid-run must join the skipped list and may not, at the same time

Location: spec.md:583-585 (FR-022), data-model.md:315 and I-4 at
data-model.md:286-289, against data-model.md:140, hunt-api.md:291 and
hunt-json.md:115-118.

Problem: FR-022 sends "an event whose window discloses a non-`COUNTABLE`
`Availability` during the run" to the skipped list, and the lifecycle row
data-model.md:315 agrees ("skipped (`GAP`) ... sample excluded, `SkippedEvent`").
I-4 permits the overlap: it says an address stands "at most once in `entries`,
and at most once in `skipped`" and that "every planned address holds one
`entries` record" (data-model.md:286-289), which is membership in both lists for
an event that measured and gapped. Three lines close that door. hunt-api.md:291
attributes to I-4 a rule I-4 never states, "no address in both". data-model.md:140
sets `RankedEvent::skipped` only "when the address produced no value", and
hunt-json.md:115 gives "one object per event that produced no value". A 20-pass
event that gaps on pass 7 and measures the other 19 produced values, so under
those three lines it never joins `skipped` and FR-022 goes unreported. The
artifacts already half admit the overlap: hunt-json.md:118 gives
`skipped[].stage` the value "null for a discovery skip", which is only
meaningful for a skip taken during a run that also measured, and hunt-json.md:108
requires an `entries` object for every planned column.

Fix: settle it the way I-4 and FR-022 already read. An event with any gapped
window keeps the entry its clean samples build (I-4 and hunt-json.md:108 hold)
and also joins `skipped` with kind `gap` and its `stage`. Drop "no address in
both" from hunt-api.md:291, restate the two produced-no-value clauses at
data-model.md:140 and hunt-json.md:115 as "an address whose open was refused or
whose window disclosed a non-`COUNTABLE` availability", keep the `GAP` row of
data-model.md:315, and add the 19-clean-plus-one-gap case to
`harness_hunt_collect_test.cpp` (hunt-api.md:375) asserting both memberships.

### N-6. A device limit below 3 has two dispositions and the CLI has neither

Location: data-model.md:108-113 and quickstart.md:72-73, against
hunt-api.md:192-195; `contracts/hunt-cli.md` has no row for it (a grep for
`limit` in that file returns nothing).

Problem: data-model.md:111-113 says that when `L` is below 3 with both anchors
present "the plan reports the cause in its printed reason and the run prints an
empty result", and quickstart.md:72-73 repeats it. hunt-api.md:192-195 makes the
same situation a postcondition refusal whose caller "prints one line naming the
value and exits 2". One is a successful run with an empty result, the other an
exit 2, and hunt-cli.md has neither a refusal row nor a console row, so P8 has
nothing to write a fixture from. The empty-result reading also has no mechanism:
`HuntPlan` (hunt-api.md:153-162) holds no reason field for a "printed reason" to
come from. spec.md:559 (FR-011) bounds the events per pass and is silent on the
disposition.

Fix: pick one, and N-1's split argues for the refusal: keep hunt-api.md:192-195,
give hunt-cli.md a refusal row for the below-3 device with its one-line message
and exit 2, add the case to plan.md:504, and restate data-model.md:111-113 and
quickstart.md:72-73 as that exit-2 line, replacing the empty result.

### N-7. The gap citation points at an unrelated declaration

Location: data-model.md:180 (`counters_measurement.hpp:180`), the only gap-site
citation in the eight artifacts (grep for `counters_measurement` hits that line
and nothing else); hunt-api.md:277-281 restates the same mapping with no site at
all.

Problem: the file is 1267 lines long, so 180 exists, but it is `inline auto
splice(detail::ExprCore& dst, const detail::ExprCore& src)`, an expression-tree
helper with no availability in it. The window gap disclosure the sentence rests
on is documented at counters_measurement.hpp:406-413, where the comment states a
field is "meaningful only when `availability != Availability::GAP`" and names
`fold` and `foldPairs` as the calls that disclose each window's own gap state.
The claim is right; a reader following the citation lands in the wrong class,
which is the shape finding 17 had when it was caught.

Fix: cite counters_measurement.hpp:406-413 at data-model.md:180, and put the same
citation in hunt-api.md:280 beside the `GAP` enumerator.

## Noted, below the finding bar

Each of these is one line of text and no implementer will build the wrong thing
from it, so none is counted.

1. The cheapest close for finding 13: plan.md:264-266 disambiguates only the
   padded `D-01` to `D-09`. Either renumber the plan series past `D-20` or write
   "plan D-13" at every site, including hunt-cli.md:5-6 and the phase table at
   plan.md:466-475.
2. `std::pair` is part of the public surface (`split` at hunt-api.md:113,
   `planHunt`'s `anchors` at hunt-api.md:167) and `<utility>` stands in no part
   of the include list at hunt-api.md:30-38.
3. data-model.md:22 calls `addresses()` "the deduplicated selection in catalog
   order after the seed shuffle", but the seed only arrives at `split(seed)`
   (data-model.md:23, hunt-api.md:113), so the order of a catalog that has not
   been split yet is unstated.
4. data-model.md:24 cites `runner.cpp:41-49` for `splitAddress`; the definition
   spans runner.cpp:39-47.
5. plan.md:66 writes `SC-06` and `SC-07` where spec.md numbers them `SC-006` and
   `SC-007`.

VERDICT: REVISE - 10 findings remain; findings 11, 13 and 18 are still open, and
the seven new ones contradict the spec on the host every CI test runs on, on a
bare `--hunt`, on `--dry-run`, on the D-13 norm refusal and on who owns the
skipped list.
