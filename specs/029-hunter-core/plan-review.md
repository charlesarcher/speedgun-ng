# Plan Review: 029 Hunter Core (Event Ranking)

Adversarial review of `plan.md`, `research.md`, `data-model.md`, the four files under
`contracts/` and `quickstart.md` against `spec.md` at the recorded audit point `bd10d76`.
Every code claim below was re-checked in this tree, not read from the artifacts.

No artifact is truncated: each ends on a complete section and a closed sentence, and none
carries a TODO, TBD or placeholder marker. `tasks.md` is absent, which is correct at this
stage (constitution IX writes it in `/speckit.tasks`).

Process note: a second session committed these eight artifacts as `a9e397c` while this review
was in progress, and its committed text differs from the working tree first read. Every
finding and quotation below was re-checked against the committed files at `a9e397c`.

Checked and clean, so not repeated as findings: the harness and JSON line citations match
the tree (`cli.cpp:73,256,326-339,345,346`; `runner.cpp:49,866,873,1400,2286-2292,2569,
2601,2634`; `json_load.cpp:103,654,1060,1156,1190`; `json_report.cpp:31,506,508,571,
583-598`; `report.cpp:36,216,251,410`; `counters_system.hpp:47,85,109,135,241`;
`counters_core.hpp:87-95,247`; `counters_fake.hpp:43,103,252,258-272`; `pass_plan.hpp:27,
41,105,132`; `internal.hpp:224,434,444,455,486`; `benchmark.hpp:220,261,624,2561`;
`report-v1.json:7-9,28-51,72`; `example_drift.py:32,63,155,156,161,458`;
`harness_json_schema_test.cpp:21-22,102`; `harness_cli_test.cpp:800,1166,1417`;
`harness_compare_cli_test.cpp:492,516`; `harness_compare_test.cpp:1191-1202`;
`docs/pages/harness.md:1067`; `README.md:621,766`; `example/CMakeLists.txt:74`).
`ci-rocky` and `ci-ubuntu` exist, `ci-sanitize` and `ci-tsan` exist as presets, every gate
script named exists, every existing test file named exists, and
`test/source/harness_cli_table_test.cpp` is absent exactly as D-21 claims. AC-1 to AC-10
each carry a planned test or a record: AC-1 to AC-7 in `plan.md`'s test plan, AC-8 in phase
P10, AC-9 in P9 and `quickstart.md`, AC-10 in the Evidence tables and the `results/`
directory. Finding 12 records the one spec-mandated behaviour (D-16's
`--hunt` with `--metric-method=multiplex`) that no artifact mentions. The constraints
list of the task is honoured in the plan except where a finding says otherwise: hunt.hpp
options structs, no second pass loop, no `perf_event_open` outside counters, exit-2
one-line refusals, the dbc contract form, simdjson error-code reads, no sleeps, clocks or
fixed cpu counts in the test plan, 100 percent coverage with no exclusion route, harness
SOVERSION 11 with the recorded reason, the `hunt_cli.cpp` split for N7, and the `hunt`
prefix on stdout for a `--json` run.

## Findings

1. Location: `specs/029-hunter-core/data-model.md` E-03 `repeats` row (line 64),
   `contracts/hunt-api.md` `HuntOptions::repeats = 10` (line 75), `contracts/hunt-cli.md`
   `--hunt-repeats` row.
   Problem: the default is 10, while spec D-7 and FR-020 fix the default at 3 with range 1
   to 1000; a default-constructed `HuntOptions` collects ten repeats where the spec collects
   three, and `contracts/hunt-json.md` prints `"repeats": 3`, so the contract's own default
   and its example disagree.
   Fix: set the default to 3 in all three places and recheck every `repeats`-derived example
   number in `quickstart.md`.

2. Location: `specs/029-hunter-core/data-model.md` E-04 Stage rule (line 100) and
   `contracts/hunt-api.md` `planHunt` postcondition (line 182, "`stages` holds the two
   stages").
   Problem: data-model states "with no budget the plan holds two stages", while spec D-2
   (spec.md line 73) says that with no budget all events run at the full repeats; a plain
   `--hunt` run planned this way takes one repeat per event, so almost every event falls under
   the two-sample rank gate and AC-3's ranking cannot form, and the lifecycle table at
   `data-model.md` line 336 bakes the same shape into the run record, while hunt-api's
   postcondition repeats the two-stage statement with no budget condition.
   Fix: make a plan without a budget one stage at `options.repeats` holding every event, gate
   the split on `options.budget` in both files, and correct the lifecycle table's stage rows.

3. Location: `specs/029-hunter-core/data-model.md` E-04 budget reductions (lines 102-107)
   and `contracts/hunt-api.md` `planHunt` budget rule.
   Problem: both invent reductions that spec D-2 does not define (a `PASSES` budget cuts
   stage 2, a `SECONDS` budget cuts stage 2's repeats) and attribute them to "D-21", which is
   the CLI-table file decision; D-2 fixes stage 2 as the top `3 x top` events at the full
   repeats, so AC-7's fixed expectation of twelve events at the full repeats is not what the
   plan produces.
   Fix: either implement D-2 literally and drop the reductions, or get a new spec decision
   recorded for them and cite that decision id in both files.

4. Location: `specs/029-hunter-core/quickstart.md` US2 walkthrough line
   (`hunt: plan events=200 passes=50 method=PASSES estimate=96000000ns budget=8p`).
   Problem: with a limit of 6, 200 events need 50 passes for stage 1 alone, so an 8-pass
   budget cannot hold stage 1, and the expected `budget cut stage=2 reason=fits-budget`
   contradicts spec US2 scenario 1, which expects stage 2 to carry the top 12 under a pass
   budget.
   Fix: pick the semantics from finding 3, then restate the walkthrough numbers so the
   budget, the pass count and the cut reason are consistent with the printed plan line.

5. Location: `specs/029-hunter-core/research.md` D-02, `data-model.md` E-01 rule 2,
   `contracts/hunt-api.md` `isCoreDevice` row.
   Problem: all three match `armv8_` against the entry name, while spec D-1 and FR-004 put
   the rule on the address device part (`armv8_*`); a device named `armv8_pmuv3` whose entry
   is `cycle` produces the address `armv8_pmuv3/cycle`, so the entry-name rule selects
   nothing on the Arm hardware AC-10 runs on.
   Fix: prefix-match `Object::path()` on `armv8_`, keep `cpu`, `cpu_core` and `cpu_atom` as
   exact device-part matches, and script a fake device whose path carries that prefix.

6. Location: `specs/029-hunter-core/contracts/hunt-api.md` "runHunt (internal)" block.
   Problem: it moves `runHunt` to a new internal header
   `source/harness/detail/hunt_run.hpp` returning `std::expected<std::pair<EntryHunt,
   RunHunt>, counters::Error>`, while spec D-15 names `runHunt` in `include/speedgun-ng/
   hunt.hpp`, `plan.md`'s API table declares it in `speedgun-ng/hunt.hpp` returning
   `std::expected<EntryHunt, Error>`, and `hunt_run.hpp` appears in no file list of `plan.md`'s
   Project Structure.
   Fix: choose one surface; if `runHunt` stays internal, record the deviation from D-15 in
   `plan.md`, correct its API table's return type, and add the header to the source tree and
   the file counts.

7. Location: `specs/029-hunter-core/contracts/hunt-json.md` top-level hunt table and
   `contracts/report-schema-key.md` hunt node x-order.
   Problem: the top-level `hunt` object carries `passes`, `method` and the stage counts but
   no `events` array and no `stages` array, while spec D-17 and FR-050 require the plan with
   events, passes, method and stages, so AC-6's written document cannot show which events
   were planned.
   Fix: add `events` (the planned addresses in order) and `stages` (one object per stage with
   its event range and repeats) to both the writer contract and the schema key contract.

8. Location: `specs/029-hunter-core/contracts/hunt-json.md` top-level row for
   `HuntOptions::noise` and `data-model.md` JSON mapping row for `norm`, `rank`, `top`,
   `noise`, `repeats`, `seed`.
   Problem: the JSON key is spelled `noise`, while spec D-17 and FR-050 name the top-level key
   `noisePercent`, so the round-trip fixture and the schema entry disagree with the spec.
   Fix: rename the key to `noisePercent` in both artifacts, the schema node and the reader.

9. Location: `specs/029-hunter-core/contracts/hunt-json.md` per-entry rows for
   `estimate_ns` and `budget`, `contracts/report-schema-key.md` required entry keys,
   `data-model.md` E-06 and its mapping row "EntryHunt budget and estimate summary".
   Problem: `EntryHunt` as declared in `contracts/hunt-api.md` holds neither an estimate nor a
   budget, so the eight required entry keys have no in-memory home and FR-051's round-trip
   equality (AC-6) cannot hold for them; data-model.md papers over it with a mapping row that
   duplicates the `RunHunt` keys at entry level.
   Fix: drop `estimate_ns` and `budget` from the per-entry object (they are run-level facts
   already in the top-level object) or add both fields to `EntryHunt` and its mapping row.

10. Location: `specs/029-hunter-core/data-model.md` invariant I-6, `research.md` D-05 and
    D-07, `contracts/hunt-json.md` `entries[].cv` row.
    Problem: they state the hunt `cv` is a percent and simultaneously "equals the printed
    `cv=` column", but `source/harness/runner.cpp:79` sets `coefficientOfVariation` to
    `sampleStdDev / mean` and `source/harness/report.cpp:448,479,499` print that ratio as
    `cv=%8.4f`, so the two differ by a factor of 100 and AC-3's "matches the table it prints"
    fails; `--hunt-noise=10` as a percent threshold is unreachable against a ratio.
    Fix: state the scaling once (hunt cv = 100 x `statisticsOf` cv), reword I-6 and AC-3's
    equality as a factor-of-100 relation to the printed column, and pin the scaling in
    `harness_hunt_rank_test.cpp`.

11. Location: `specs/029-hunter-core/contracts/hunt-cli.md` refusal row
    `--hunt=none`, `data-model.md` option disposition table, `plan.md` Test Plan CLI refusal
    row, `quickstart.md` refusal table.
    Problem: all four refuse `--hunt=none` with exit 2, while spec.md's Assumptions state
    that `--hunt=none` disables the hunt, the same as passing no `--hunt`, and exists so a
    scripted option value round-trips; FR-001 lists `none` as a legal selection form.
    Fix: make `--hunt=none` a no-hunt run everywhere, or record the deviation and amend the
    spec assumption; the scripted-value use case has to survive either way.

12. Location: `specs/029-hunter-core/contracts/hunt-cli.md` conflict rows and
    `specs/029-hunter-core/plan.md` Test Plan "CLI refusals" row.
    Problem: neither covers `--hunt` with `--metric-method=multiplex`, which spec D-16 and
    the Edge Cases make legal with a required disclosure (spec.md lines 119 and 504); the
    artifacts grep clean of that combination outside spec.md, so an implementer following the
    conflict list can refuse it and no planned test would notice, leaving AC-1 to AC-10
    complete on paper with one spec-mandated behaviour untested.
    Fix: add a legal-combination row to the CLI contract, the disclosure wording for the
    console and the JSON `method` value, and a `harness_hunt_cli_test.cpp` case for it.

13. Location: `specs/029-hunter-core/plan.md` Test Plan "CLI table" row (line 501) and
    `research.md` D-18 ("270 to 279 are claimed by the hunt rows"), against
    `contracts/hunt-cli.md` (lines 34 and 156) and `quickstart.md` ("values 270 to 277
    claimed by the hunt rows").
    Problem: two artifacts assert that the test proves 270 to 279 claimed and 280 to 289 free,
    while the other two assert 270 to 277 claimed with 278 to 289 free, and `research.md` D-17
    assigns only 270 to 277 ("leaving 278 and 279 for the hunt lane's later options"), so the
    assertion as planned in plan.md fails against the table that same plan writes.
    Fix: pin the assertion to 270 to 277 claimed and 278 to 289 free in plan.md and D-18,
    matching FR-080's range and D-22's reservation of 280 to 289.

14. Location: `specs/029-hunter-core/data-model.md` E-06 `SkippedKind` and
    `contracts/hunt-api.md` `SkippedKind` block.
    Problem: they add a sixth kind `GROUP_REFUSED` "with no catalog counterpart", while spec
    D-10, D-18, FR-021, FR-050 and the Key Entities entry pin a skipped entry's kind to one of
    the five `Availability` values verified at `include/speedgun-ng/counters_core.hpp:87-95`,
    so the JSON kind vocabulary and AC-4's kind claim diverge from the spec.
    Fix: map a group-open refusal to an existing `Availability` kind (or amend the spec's
    entity list and FR-050 in the same change) and say which kind the group refusal carries.

15. Location: `specs/029-hunter-core/research.md` D-02, `contracts/hunt-api.md`
    `isCoreDevice` Source and rationale rows, `quickstart.md` Notes.
    Problem: they cite `linux_pmu/rules.cpp:516`, `:540` and `limitForDevice` at `:873`, but
    `source/counters/linux_pmu/rules.cpp` does not exist in the tree; the device-part
    predicate lives at `source/counters/linux_pmu/counter_limits.cpp:284-295` (`limitForDevice`
    defined at 288) and `source/counters/linux_pmu/provider.cpp:352,447-448`.
    Fix: re-cite all three artifacts to `counter_limits.cpp` and `provider.cpp`, and state
    that no core-device name table exists for the hunt side to read.

16. Location: `specs/029-hunter-core/research.md` D-02 and `contracts/hunt-api.md`
    `isCoreDevice` rationale, both citing `linux_pmu/provider.cpp:1502,1513,1529,1552`.
    Problem: `source/counters/linux_pmu/provider.cpp` is 963 lines, so all four cited lines are
    out of range and the claimed seeding of `cpu` at those lines cannot be read.
    Fix: re-cite the real sites (`provider.cpp:352` and `provider.cpp:447-448`) or point at
    `counter_limits.cpp:294` for the same fact.

17. Location: `specs/029-hunter-core/research.md` D-02 alternative rejection.
    Problem: it argues the `armv8_` prefix "is stable in the shipped data" because
    `json_load.cpp:868` "already compares that prefix"; line 868 of
    `source/harness/json_load.cpp` builds a `StatisticValue`, and the string `armv8` occurs
    nowhere under `source/`, `include/`, `test/`, `tools/` or `docs/`, which is what spec PC-8
    itself records ("no such string exists on master").
    Fix: delete the claim, and carry the prefix rule on its own scripted fake device rather
    than on claimed precedent.

18. Location: `specs/029-hunter-core/research.md` D-11 decision and its rejected
    alternative.
    Problem: both cite `source/dbc/dbc.cpp:110-113` for the observe-then-abort route, but
    there is no `source/dbc/` directory; the abort routes are
    `include/speedgun-ng/dbc.hpp:33,168,213`, and the default semantic D-11 relies on has to
    be named where dbc actually sets it.
    Fix: re-cite `include/speedgun-ng/dbc.hpp` for the default response and recheck the
    "prints to stderr, then aborts" wording against it.

19. Location: `specs/029-hunter-core/research.md` D-05 median-convention paragraph.
    Problem: it places `passSpread` at `pass_plan.cpp:367-378` (the function is at
    `source/harness/pass_plan.cpp:404-430`) and places the counters median at
    `source/counters/plan.cpp:1405` in a 728-line file, and it claims that median takes the
    lower central element "too", while `source/counters/plan.cpp:309` indexes `costs[n / 2]`,
    the upper central element, which is exactly the split spec PC-4 records.
    Fix: correct both line spans and restore PC-4's lower-versus-upper split, since D-05's
    "no local median is needed" rests on the two being identical.

20. Location: `specs/029-hunter-core/research.md`, `specs/029-hunter-core/data-model.md` and
    all four files under `specs/029-hunter-core/contracts/`.
    Problem: spec decision and requirement ids are cited wrongly throughout, so an implementer
    following a citation lands on an unrelated decision: the core-device rule is cited as D-8
    (it is D-1), normalization as D-3 (it is D-6), the rank key as D-14 (it is D-3), the
    statistics reuse as D-16 (it is FR-030 with PC-4), the `kMaxUserCounters` bound as D-6 (it
    is PC-2), the budget reductions as D-21 (they belong to D-2, which defines none), the
    multiplex settling as D-7 and FR-022 (it is D-19), the anchors as PC-7 (they are PC-8),
    device parts as PC-6 (they are PC-1), raw codes as PC-1 (they are PC-6), the option value
    range as PC-5 (it is D-22), the plan estimate as FR-021 (it is FR-012), the no-hunt
    identity as FR-040 (it is FR-052), the JSON round trip as AC-8 and SC-06 (they are AC-6 and
    SC-006), the CLI-table requirement as FR-082 (it is FR-083), and the pass-per-device rule
    as D-8.
    Fix: re-map every `D-`, `FR-`, `PC-` and `AC-` reference in those six artifacts against
    spec.md before `/speckit.tasks`, since `tasks.md` inherits the wrong ids.

21. Location: `specs/029-hunter-core/research.md` D-07 decision paragraph and
    `contracts/hunt-api.md` `rankEvents` Source row.
    Problem: both cite FR-015 for the deterministic print and the stable rank order, and
    `spec.md` defines no FR-015 (its Plan requirements stop at FR-014 and the Rank block
    starts at FR-020), so the citation resolves to nothing.
    Fix: cite FR-014 with D-9 for the ordering and drop the FR-015 reference from both files.

22. Location: `specs/029-hunter-core/contracts/hunt-api.md` header line "Spec: FR-060 to
    FR-062, D-15, hard rules 5, 6 and 16".
    Problem: `spec.md` defines FR-060 (budget scope) and nothing at FR-061 or FR-062, and the
    surface this contract pins is governed by FR-001 to FR-033 plus D-15, so the contract's
    own traceability header points at requirements that do not exist.
    Fix: restate the header as FR-001 to FR-033 with D-15 and the hard rules the file
    actually exercises.

23. Location: `specs/029-hunter-core/contracts/hunt-cli.md` header line "Spec: FR-030 to
    FR-033, FR-080 to FR-082, D-1, D-9, D-11, D-12, D-21, D-22, hard rules 2, 3, 9 and 10".
    Problem: the CLI requirements are FR-080 to FR-083 and FR-083 is the one that creates
    `test/source/harness_cli_table_test.cpp`, which this contract's own Option table test
    section depends on; FR-030 to FR-033 are Rank requirements and D-16 owns the conflict
    list this contract tabulates.
    Fix: cite FR-080 to FR-083 and D-16, and drop the Rank block from the header.

24. Location: `specs/029-hunter-core/plan.md` Project Structure line for `data-model.md`
    ("E-01 to E-10, invariants I-1 to I-8").
    Problem: `data-model.md` is indexed "E-01 to E-11" with E-11 carrying `statisticsOf`, and
    `plan.md`'s Source tree has no E-11 line even though it plans
    `source/harness/statistics.cpp` as exactly that move.
    Fix: change the index to E-01 to E-11 and tag `statistics.cpp` and `detail/statistics.hpp`
    with E-11.

25. Location: `specs/029-hunter-core/plan.md` Source paragraph (line 189, "Four new source
    files, one new public header, one new internal header, six new test translation units, one
    new docs example").
    Problem: the tree in the same section adds two internal headers
    (`source/harness/detail/statistics.hpp`, `source/harness/detail/hunt_plan.hpp`), and
    `contracts/hunt-api.md` adds a third (`source/harness/detail/hunt_run.hpp`, line 299), so
    the count the Shared-list ledger keys its file inventory off is wrong before implementation
    starts.
    Fix: state two internal headers (three if finding 6 keeps `hunt_run.hpp`) and list the
    header in the tree.

26. Location: `specs/029-hunter-core/plan.md` Source paragraph (line 189) and its Shared-list
    ledger row for `test/source/harness_cli_table_test.cpp`.
    Problem: the paragraph counts six new test translation units while the same section's tree
    lists seven files under `test/source/` (the seventh is `harness_cli_table_test.cpp`, which
    D-21 creates here), P1's gate requires "the seven test executables registered", and
    `quickstart.md` expects all seven named executables to pass; the ledger then lists that
    seventh file among the shared files other lanes touch, which understates the rebase surface
    the Constraints list warns about.
    Fix: count seven new test translation units and keep `harness_cli_table_test.cpp` in the
    new-files list with the note that no other lane creates it.

27. Location: `specs/029-hunter-core/data-model.md` preamble ("the two records the run path
    keeps to itself live in `source/harness/detail/hunt_plan.hpp` in `namespace sg::detail`").
    Problem: it names no two records, and `contracts/hunt-api.md` declares `EventSamples`,
    `HuntSample`, `RankedEvent`, `SkippedEvent`, `EntryHunt` and `HuntPlan` public in
    `include/speedgun-ng/hunt.hpp` under `sg::hunt`, so the internal-versus-public split of
    the run-path records is undecided between the two artifacts and against D-15's surface.
    Fix: name the two internal records in the preamble and make the contract's declaration
    block match it.

28. Location: `specs/029-hunter-core/plan.md` Constraints bullet and its SOVERSION table
    (archive 3 to 4), against `specs/029-hunter-core/spec.md` Constraints bullet "harness 10
    to 11, archive stays 3".
    Problem: the spec contradicts itself (its Spec identity section and FR-090 both move the
    archive 3 to 4) and the plan follows FR-090 with the PC-3 reason, so the plan is right and
    the constraint it cites is stale; nothing in `plan.md` records that it departed from the
    Constraints list.
    Fix: amend the spec Constraints bullet to "archive 3 to 4" and cite FR-090 in the plan's
    ABI table instead of the constraint line.

29. Location: `specs/029-hunter-core/research.md` D-13, `data-model.md` Run lifecycle table
    ("stage 2 ... budget cut") and `contracts/hunt-json.md` "one object per hunt column".
    Problem: `source/harness/detail/internal.hpp:494` gives `Runner::run` one
    `BenchmarkResult` per run and `source/harness/detail/pass_plan.hpp:119` requires the fold
    results to match `plan.columns.size()`, so a stage-2 plan over the top `3 x top` columns
    publishes rows for those columns only, while invariant I-4 and the JSON contract require
    `entries` to carry every hunt column, including events that ran only in stage 1.
    Fix: state where stage-1-only ranked events live when the published `BenchmarkResult`
    covers stage 2 (a synthesized `EntryHunt` from stage 1's samples), and add a two-stage
    case to `harness_hunt_report_test.cpp` that asserts a stage-1-only address still reaches
    both the console table and `entries`.

30. Location: `specs/029-hunter-core/contracts/hunt-cli.md` accepted-glob row versus its
    Refusals row `--hunt=uncore*` "not a well-formed pattern".
    Problem: the same file states `*` matches any run of characters including none, so
    `uncore*` is a legal pattern that matches every uncore address, and `spec.md`'s only glob
    refusal is a pattern that matches no catalog event; the contract invents a
    well-formedness rule it never defines and then refuses a pattern its own rule accepts.
    Fix: drop the well-formedness refusal or define it precisely, and state what
    `--hunt=uncore*` does: accept it and select nothing because uncore is out of `all`'s
    scope, or refuse it with the matches-no-catalog-event line the spec defines.

31. Location: `specs/029-hunter-core/contracts/hunt-cli.md` Console row for `--dry-run`
    versus its Verification row.
    Problem: the Console section says a hunt `--dry-run` prints the plan line "and nothing
    else does", while Verification tests "the conflict check for `--counter`, `--metric`,
    `--compare` and `--dry-run`, in both command-line orders", so the contract both refuses
    and honours the same combination, and `--dry-run` is a real option
    (`source/harness/cli.cpp:353`).
    Fix: decide whether `--dry-run` conflicts with a hunt, and remove it from whichever list
    is wrong.

32. Location: `specs/029-hunter-core/contracts/hunt-cli.md` Console row
    `hunt: skipped armv8_cycle group-refused` and `contracts/hunt-json.md` example
    `"address": "armv8_cycle"`.
    Problem: both print an address with no device part, against the artifacts' own invariant
    I-1 and spec PC-8 (an address is `Object::path() + "/" + entry.name`, the split
    `source/harness/runner.cpp:39-46` performs), so a fixture copied from either example
    cannot round-trip through `splitAddress` the way E-04 requires.
    Fix: use the composed form (for example `armv8_pmuv3/cycle`) in both examples.

33. Location: `specs/029-hunter-core/contracts/hunt-api.md` `expand` precondition
    ("`selection` is non-empty and holds all, comma-separated catalog addresses, or one glob
    pattern") and its `SelectionExpansion` note that `@<file>` text arrives "joined into one
    newline-separated list", read against `data-model.md` E-01 and the `@<file>` row of
    `contracts/hunt-cli.md`.
    Problem: `contracts/hunt-cli.md` accepts file text that is newline separated or a comma
    list, `data-model.md` E-01 states only "a newline list", and the `expand` precondition as
    written admits only a comma list, so the three artifacts give three separator rules for
    one field and a newline-joined list from a `@<file>` violates the precondition the same
    file states.
    Fix: state one separator set in the `\pre` (both newline and comma) and drop the
    joined-list aside or make it match.

34. Location: `specs/029-hunter-core/plan.md` decision table row D-10 ("public
    `hunt::RunPlan` on `ReportData` and public `hunt::EntryHunt` on `BenchmarkResult`").
    Problem: `data-model.md` E-08 names that record `RunHunt` and
    `contracts/hunt-api.md` declares `std::optional<hunt::RunHunt> hunt` in `ReportData`
    (line 329), which `contracts/hunt-json.md` then writes through
    `writeHunt(builder, const RunHunt&)`, so the run-level record carries two names across the
    artifacts and `tasks.md` plus the first commit will settle one and leave the other file
    stale.
    Fix: settle the public name (the data model and both contracts already say `RunHunt`), use
    it in plan.md's D-10 row and in its Public API table, and keep it out of any `detail/`
    header as D-15 requires.

35. Location: `specs/029-hunter-core/quickstart.md` Coverage section
    (`cmake --preset coverage`, then `cmake --build build --target coverage`).
    Problem: `CMakePresets.json` has no `coverage` configure preset (the public one is
    `ci-coverage`, and its hidden `coverage-linux` base sets `binaryDir` to
    `${sourceDir}/build/coverage`), so the configure step fails and the build step would point
    at the `ci-rocky` tree anyway; `.github/workflows` runs `cmake --preset=ci-coverage`.
    Fix: use `cmake --preset=ci-coverage` and build the `coverage` target in that tree, and
    name the tree so the hard rule 14 boundary stays visible.

36. Location: `specs/029-hunter-core/quickstart.md` Prerequisites paragraph ("a machine that
    has its own `CMakeUserPresets.json` may use its `dev` preset instead").
    Problem: committed instructions must name only checked-in presets; `CMakePresets.json`
    has no `dev` preset (only the hidden `dev-mode` base), `CMakeUserPresets.json` is
    machine-local and uncommitted, and `AGENTS.md` states an unoptimized dev preset cannot
    report a finding the optimizer's analysis produces, which is the whole point of the
    release presets P1's gate runs on.
    Fix: delete the sentence and leave `ci-rocky` and `ci-ubuntu` as the only build routes in
    the document.

37. Location: `specs/029-hunter-core/contracts/hunt-cli.md` Console row
    `hunt: plan events=42 passes=14 method=PASSES estimate=1248000ns budget=120s`.
    Problem: `data-model.md` line 96 fixes the estimate at `calibrationNsPerPass x passes x
    repeats`, and 1248000 is not divisible by 14, so no integer calibration reproduces the
    line a fixture will be written from; the same file's `spend=1184000ns` summary is
    reachable, so only the estimate is wrong.
    Fix: restate the example with an estimate that is a whole multiple of `passes x repeats`,
    for example `estimate=1260000ns` at 14 passes, one repeat and 90000 ns per pass.

VERDICT: FAIL
