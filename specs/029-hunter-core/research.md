# Research: Hunter mode core (event ranking)

**Feature**: specs/029-hunter-core | **Audit point**: `bd10d76` | **Date**: 2026-10-22

Every decision below carries its measured evidence, the file and line the
measurement rests on at `bd10d76`, and the alternatives that were rejected. A
bare `D-n` names a spec decision in [spec.md](spec.md); a zero-padded `D-0n`
names a plan decision and appears in the index of [plan.md](plan.md). Section
numbers quote the spec headings. Line numbers stand unused, so the anchors here
do not move when the spec grows.

## D-01 Shape of the feature

**Decision**: hunt is a harness column family. `EventCatalog` selects the
events, `planHunt` turns each one into a column, `runHunt` drives the existing
`Runner` so the existing N6 pass loop measures them, and `rankEvents` post-
processes the samples. The public surface is the new header
`include/speedgun-ng/hunt.hpp` in `namespace sg::hunt`.

**Rationale**: D-23 fixes the mechanism, and the mechanism exists: `planPasses`
takes a leaf list per column plus one optional limit per source
(`detail/pass_plan.hpp:98-107`), holds no column-count constant, and needs no
width change for a plan of 200 columns (spec PC-2). The pass loop, the merge,
the seat and the threaded barrier all already serve a column list the caller
supplied, so the hunt adds selection and post-processing on top; the measurement
path stays the single existing one. `test/time_source_gate.sh` and Additional Constraints then
hold by construction: hunt reads nothing that the spine does not already read.

**Alternatives considered**:

- A standalone hunt driver that plans and samples through `sg::counters`
  directly. Rejected: it is the second counter path D-23 forbids, and it
  duplicates the pass loop that spec hard rule 15 names. It would also need its
  own gap handling, its own endpoint-sampling fold and its own threaded barrier
  to reach the disclosure parity every other column has, which is a second
  runner to maintain for one report.
- Hunt as named counter columns through the existing `--counter` slot.
  Rejected: `kMaxUserCounters` 16 bounds named columns
  (`runner.cpp:866,873`) and D-6 states plainly that the constant bounds named
  columns alone and must not bound hunt columns. The slot also carries no
  per-event identity, so the anchor subtraction, the per-event sample list and
  the skipped list would need a side table keyed by address beside a column set
  that reports no address, which is more machinery than the column family.
- Hunt inside the library as a `sg::counters` analysis layer. Rejected:
  selection, budget, ranking and reporting are run policy, and the library
  deliberately does not own run policy (the specs/024 structure decision holds
  for the same reason). It would also move a public ABI in the counters archive
  for a harness concern.

## D-02 Core device rule

**Decision**: a device takes part in `--hunt=all` when its device part is
exactly `cpu`, `cpu_core` or `cpu_atom`, or when its entry name carries the
`armv8_` prefix. The rule lives hunt-side in `EventCatalog`.

**Rationale**: D-8 fixes the set, and the audit measured the parts the library
publishes: `linux_pmu/rules.cpp` seeds `cpu_core` and `cpu_atom` (lines 516 and
540) and `linux_pmu/provider.cpp` seeds `cpu` (lines 1502, 1513, 1529 and 1552)
(spec PC-6). The `armv8_` entry names are the armv8 template names the embedded
tables carry, so an entry-name prefix rule reaches them without a device-part
rule; `json_load.cpp:868` already compares that prefix, which shows the spelling
is stable in the shipped data. The library's own device-part switch is at
`limitForDevice` (`rules.cpp:873`); writing the rule hunt-side leaves that
function alone and keeps the library's device routing unchanged (spec Scope).

**Alternatives considered**:

- A new published `Object` predicate such as `isCoreDevice()`. Rejected: the
  library publishes catalog facts. Selection policy belongs hunt-side, and the
  predicate would move the counters archive `SOVERSION` for a harness
  preference.
- Prefix matching on the device part (`path.starts_with("cpu")`). Rejected: it
  would pull a future `cpu_uncore`-shaped part into the core scan and cannot be
  tested against the exact set the audit measured.

## D-03 Discovery walk

**Decision**: discovery walks `System::local().object("machine")` and then its
`children()`, the same shape `catalog.cpp:81-88` uses for `--catalog`, and
composes each address as `std::string(object.path()) + "/" + std::string(entry.name)`
over `Object::counters()` (`counters_system.hpp:47,109,135,241`).

**Rationale**: `CatalogEntry` holds `name`, `description`, `unit`, `avail`,
`mode`, `targets`, `frequencyHz` and `scaled`, and no address field (spec
PC-1). `Object::path()` is the device part, so the composition is the only
address a catalog entry has, and it is the spelling `--counter=` already
accepts. Reusing the children walk means one traversal shape in the harness and
one host behaviour for a provider that adds objects.

**Alternatives considered**:

- `System::objects("pmu")` with filters. Rejected for the default scan: the kind
  filter answers a provider-declared kind, while D-8's rule is about the device
  part and the entry name, so the walk has to inspect both anyway and the filter
  adds nothing. It stays available for a later list mode that wants kinds.
- Asking the metric tables for the event list. Rejected: the metric table lists
  formulas. Raw events come from the catalog, the only event-name source D-4
  names.

## D-04 Anchors

**Decision**: the two anchors are `cpu-cycles` and `instructions` on the device
that owns the event, address-composed the same way as any other event
(`cpu/cpu-cycles`, `cpu/instructions` on a `cpu` device). A device with no
`cpu-cycles` entry or no `instructions` entry normalizes its events by
iterations and says so in the plan line.

**Rationale**: the audit measured `cycles` and `ref-cycles` missing from the
`cpu` catalog and `cpu-cycles` present with `instructions` (spec PC-7), and the
vendor alias of `cpu-cycles` is a metric-formula spelling of the catalog entry,
not a second catalog entry, so D-10's ban on invented aliases is satisfied by
using the catalog name itself.

**Alternatives considered**:

- `cycles` as the anchor name. Rejected: absent from the catalog, so the plan
  would refuse at resolve on every x86 host.
- A metric-formula anchor from `counters_metrics.hpp`. Rejected: a formula is a
  derived value, and the per-pass normalization the spec defines is a ratio of
  two counted values of the same pass, so a formula would mix the two kinds.
- Substituting a different anchor per vendor table. Rejected: no vendor rule in
  this tree names a second cycle event that is present everywhere, and a
  per-vendor anchor is a table generator, which D-4 defers.

## D-05 Statistics reuse

**Decision**: move `statisticsOf` out of the anonymous namespace of
`runner.cpp:49-82` into `source/harness/detail/statistics.hpp` with its
definition in `source/harness/statistics.cpp`; the three runner call sites
(`runner.cpp:2569,2601,2634`) call the named function and keep their
empty-list guards.

**Rationale**: D-16 requires the reuse and spec PC-4 measured the exact
mismatch: `statisticsOf` uses the average of the two central elements for an
even count and the `n - 1` sample deviation, while `passSpread`
(`pass_plan.cpp:367-378`) takes the lower central element, and the counters
median (`source/counters/plan.cpp:1405`) does too. Reusing `statisticsOf` is the
only choice that makes a hunt CV equal the printed `cv=` column, which AC-3
requires. The header-plus-one-translation-unit pair is the house shape for a
named internal decision unit (`detail/pass_plan.hpp` with `pass_plan.cpp`,
`detail/percentiles.hpp` with `percentiles.cpp`), and a named function is what
`tools/dbc/dbc_pair_gate.py` pairs against its doc lines.

**Alternatives considered**:

- Reusing `passSpread`'s median. Rejected: it disagrees with the printed `cv=`
  column for an even sample count, so AC-3 would fail on the host of record.
- Reusing the counters-library median. Rejected: same disagreement, and it
  reaches into the counters archive for a harness reporting concern.
- A hunt-local median and CV. Rejected by hard rule 16 and D-16: a second
  statistics helper in the same library is the drift the audit recorded.
- A header-only `inline` definition. Rejected: it puts an algorithm body in a
  header the harness compiles five times, and the house keeps bodies in `.cpp`
  files beside their internal headers.

## D-06 Seed order

**Decision**: one in-repo shuffle: a 64-bit linear congruential generator
seeded from `--hunt-seed`, driving a Fisher-Yates walk over the deduplicated
catalog order. Seed 0 is a defined permutation and means nothing special.

**Rationale**: D-9 requires the same seed to give the same plan on the same
catalog and forbids a clock in the order. A generator plus a shuffle written
here fixes the permutation for every compiler and every standard library
revision, which is what the CI matrix needs: the `test` job runs GCC and Clang,
and a comparison test that depends on a library's shuffle implementation would
be a cross-compiler flake waiting to appear. The order only decides pass
composition: the ranked table is ordered by the rank key of D-07, so the shuffle
spreads correlated events across passes without touching the printed order.

**Alternatives considered**:

- `std::shuffle` with `std::mt19937_64`. Rejected: the generator is specified
  and the permutation is not, so the standard library implementation decides it.
  The spec permits a standard seeded shuffle "all CI compilers agree on", and
  none is specified well enough to be depended on.
- A sort by a hash of the address and the seed. Rejected: it is the same size as
  the shuffle, and a hash collision reorders nothing, so the plan stops spreading
  correlated events without saying so.
- Catalog order untouched under a seed. Rejected: it defeats D-9's purpose,
  which is spreading correlated events across multiplexing groups.

## D-07 Rank inputs

**Decision**: `rankEvents` is a pure function over the per-event sample list.
Per event it takes the per-pass normalized rates, calls `statisticsOf` for the
median and the coefficient of variation, compares the CV against the
`--hunt-noise` threshold on the unrounded value, and orders the top N by median
rate descending for `rate` or by CV ascending for `cv`, with ties broken by
catalog address.

**Rationale**: the report already summarizes every row through `statisticsOf`,
so a pure ranker over the same samples equals the printed columns by
construction (AC-3). Breaking ties by address makes the order a total order, so
two runs of the same samples print the same table (FR-015). The threshold
comparison reads the unrounded value, matching the specs/024 pass-spread
warning, where the print is the one rounding.

**Alternatives considered**:

- Ranking inside the runner fold. Rejected: the fold runs per pass and holds no
  cross-pass view, so the rank would need runner state to be complete. It would
  also put a policy decision on the timed path, which Principle VII discourages.
- Ranking on the merged metric field of the 024 merge. Rejected: the merge keeps
  one value per column from its measuring pass, and the hunt needs every sample
  of every pass to compute a CV.
- A median-of-medians rank. Rejected: it disagrees with the printed aggregate,
  and AC-3 pins the equality.

## D-08 Normalization

**Decision**: per pass, before the median. `instr` divides the event value by
the `instructions` count of the same pass and multiplies by 1000; `cycle` uses
`cpu-cycles` of the same pass; `iter` reports the raw median per iteration. A
pass that answers zero for its anchor contributes no rate for that pass and is
counted out, the way a gap repetition is counted out of `Statistics::samples`.

**Rationale**: D-3 fixes the formula, and doing it per pass is what removes the
multiplexing bias the formula exists for: each pass scales its own columns by
its own anchor reading, so a pass the kernel rotated does not depress the rate
of the events inside it. Zero-anchor exclusion keeps a division by zero out of
the rank and keeps the printed record's sample count true to the windows
measured.

**Alternatives considered**:

- Normalizing the merged median by the merged anchor median. Rejected: it mixes
  passes, which is the bias the normalization removes.
- A ratio to the anchor of pass 0. Rejected: pass 0 is the reference pass for the
  time columns of the 024 merge, which says nothing about its anchor readings.
- Percent-of-peak normalization. Rejected: it is not the formula D-3 names and it
  makes the value host-relative, so two hosts would print incomparable numbers.

## D-09 Scripted group refusal

**Decision**: one fluent parameter on the exported `FakeProvider`:
`setGroupLeafLimit(std::string_view objectPath, std::size_t leaves) -> FakeProvider&`.
`FakeProvider::open` (`counters_fake.hpp:252`, `final` class at line 103)
refuses a leaf set that carries more than `leaves` leaves on that object, with a
refusal message naming the object and the count. An object with no scripted
limit keeps today's behaviour.

**Rationale**: D-18 requires one scripted refusal that makes the group of one
pass fail while the same event alone opens, and PC-3 measured that the existing
seam cannot express it: `FakeScript` holds `points`, `tailDelta`, `deltaSeed`,
`gaps`, `addCounter`, `setPoints` and `setGapActions`, `open` refuses an
undeclared object or counter only, and an unknown leaf is refused by the spine
compile before `open` runs. A per-object leaf count is a fact of the machine
(the schedulable set), so the fake scripts a machine fact; the fake holds no
call-sequence scripting for it, and the wide group fails while a three-leaf
group of one event plus
two anchors succeeds. The scripted state joins an exported class's members,
which is one of the two reasons the archive `SOVERSION` moves to 4 (hard rule
12).

**Alternatives considered**:

- `failNextOpen(count)`: refuse the next `count` opens. Rejected: it couples the
  test to how many opens the runner performs, so any change in the pass loop
  breaks the test for a reason unrelated to the behaviour it pins.
- A per-address refusal list. Rejected: it cannot make the wide group fail and
  the narrow group succeed at once, which is exactly the shape D-18 needs.
- A `std::function` refusal predicate. Rejected: a callback member in an
  exported class is a wider ABI move than a size, it complicates the DBC
  contract of `open`, and hard rule 6 already pushes contract predicates away
  from lambdas.
- A test-local `ProviderIface` implementation. Rejected: `compileCore` rejects
  an unknown leaf, so a provider that does not declare the harness's time leaves
  cannot reach `open` from a harness-level test at all, which is the wall PC-3
  measured.

## D-10 Hunt data location

**Decision**: two public records. `hunt::EntryHunt` becomes
`std::optional<hunt::EntryHunt> hunt` on `BenchmarkResult`, and
`hunt::RunHunt` becomes `std::optional<hunt::RunHunt> hunt` on `ReportData`
(`benchmark.hpp:624-641`). `hunt.hpp` forward-declares
`enum class MetricMethod : std::uint8_t;` in `namespace sg` and includes
neither `benchmark.hpp` nor any harness internal header; `benchmark.hpp`
includes `hunt.hpp`.

**Rationale**: the writer takes `const ReportData&` (`json_report.cpp:571`) and
the reader returns `ReportData` (`json_load.cpp:1190`), so a hunt document that
round-trips through `loadReport` needs its hunt data reachable in public types.
The forward declaration of a fixed-underlying scoped enum is already the house
pattern at `detail/pass_plan.hpp:27`, so no include cycle appears.

**Alternatives considered**:

- Internal side tables keyed by entry index. Rejected: `loadReport` could not
  fill them without a public field anyway, and two sources of truth for one
  record is the drift X.3 warns about.
- Storing the hunt record as a JSON string on the result. Rejected: it hides a
  structured record inside a string, forces the reader to parse twice, and gives
  no typed access for the console printer or the comparison of AC-8.
- Public hunt fields spread over `BenchmarkResult` beside `passCount`. Rejected:
  eleven flat fields on a record every lane edits widens the conflict surface
  with specs/027 and specs/028 for no gain; one nested record is one line at the
  merge.

## D-11 Seat error route

**Decision**: `seatPass` keeps returning `std::expected<void, std::string>`. In
the pass loop, the seat result of a pass after the first is handled by a branch:
a run whose plan carries hunt columns marks the failed pass for the retry of
D-12, and a run with no hunt columns keeps the `SG_INVARIANT` at
`runner.cpp:2287-2292` exactly as master writes it.

**Rationale**: PC-9 measured the consequence: the invariant at that site fires
when a group-open refusal follows a successful pre-loop seat, and under the
default `observe` contract semantic a violation prints to `stderr` and calls
`std::abort` (`source/dbc/dbc.cpp:110-113`), so the run would die instead of
skipping an event. The invariant's stated assumption is that the frozen catalog
keeps every pass compilable; a schedulable-set refusal at open breaks that
assumption at run time, so the condition is a runtime condition and not a
violated contract. Keeping the invariant for a run with no hunt columns keeps
AC-6's byte-identity promise and the specs/024 contract, whose
`harness_pass_report_test.cpp` byte-identity golden is the covering check.

**Alternatives considered**:

- Replacing the invariant with a branch for every run. Rejected: it weakens a
  shipped contract for a behaviour one option asks for, and it changes a
  non-hunt run's outcome under `enforce` builds, which the 024 lane pins.
- An always-on contract designation for the invariant. Rejected: it keeps the
  abort and D-18 needs a skip.
- A second seat function for hunt. Rejected: two seat functions over one leaf
  list is the duplicated decision X.2 forbids; the branch sits at the one call
  site that already holds the result.

## D-12 Retry inside the pass loop

**Decision**: the pass loop reads a growable pass-column list. A seat failure
for pass `p` removes `p`'s events from the plan, appends one pass per event of
`p` (each one event with the two anchors), and lets the same loop run them. An
event that still fails to seat alone is marked `unavailable` the way an
unavailable column is marked today and joins the skipped list with the refusal
kind its availability names; the run finishes and exits 0. A pass-0 seat failure
keeps the existing `fail` route (`runner.cpp:1434-1436`) with exit 1, because
the pre-loop seat is the same compile that serves warm-up and calibration.

**Rationale**: D-18 states the route and hard rule 15 forbids a second pass
loop. The loop today indexes a fixed `passColumns` vector under
`pass < passCount` (`runner.cpp:2277` and its `passColumns[pass]` at 2290), so
appending entries and moving `passCount` is the one change that keeps one loop
and one column set. Reusing the `unavailable` marking means the retry needs no
new skip channel into the fold.

**Alternatives considered**:

- A nested retry loop inside the pass loop. Rejected: a loop inside the loop for
  the same work is what hard rule 15 names, and it duplicates the repetition
  body.
- Re-running the whole run at a narrower plan. Rejected: it re-runs the
  benchmark, doubles the printed rows, and breaks the one-result-per-instance
  shape the report and the JSON writer assume.
- Pre-emptive one-event passes whenever a device publishes no limit. Rejected:
  it gives up pass packing on the hosts that need it most, and it cannot know
  the schedulable set, which the audit shows can be narrower than the published
  general-purpose count.

## D-13 Two-stage run

**Decision**: one `Runner::run` call per stage over the same instance: stage 1
at one repetition over the full plan, stage 2 at `--hunt-repeats` over the
reduced plan. The row set the report and the JSON writer publish comes from the
last stage that ran, and the hunt records merge per address, stage 2 values
winning.

**Rationale**: the runner is the only path that seats a plan, opens windows,
folds and merges, so two calls reuse the whole spine and add no second pass
loop. Publishing the last stage's rows keeps one printed row set, which keeps a
hunt run's benchmark tables readable and keeps `report_shape` shape checks
single-valued.

**Alternatives considered**:

- One stage with per-column repetition counts. Rejected: the repetition loop
  sits inside the pass loop and measures every column of the pass each
  repetition, so a per-column count needs a per-column skip inside the timed
  body, which would change what every other column of that pass measures.
- Stage 2 as a fresh instance run through the public `run` handle. Rejected: it
  prints a second row set and produces a second `BenchmarkResult`, which the
  report and the writer would have to be told to ignore.
- Stage 1 skipped when the plan already fits `--hunt-repeats`. Rejected: D-2
  fixes the two-stage shape, and stage 1 is what selects the top `3N` set.

## D-14 Single-sample events

**Decision**: an event with one sample reports `cv` as `0.0` and cannot enter
the top N; it prints in the marked tail with `rank=-` and a mark naming it
unrepeated, and the JSON record carries the same rank null. Stage-1 selection
of the top `3N` set ranks those single samples by rate.

**Rationale**: `statisticsOf` asserts a non-empty list and a count above one for
the deviation (`runner.cpp:52-55,74-76`), so one sample has no CV: reporting a
number there would be an invented fact, and calling the helper with it would
trip an assert. Keeping such an event out of the top N follows D-7 exactly: an
event whose noise is unknown did not qualify. Stage-1 selection still
needs an ordering of those samples, and rate is the only figure they carry.

**Alternatives considered**:

- Reporting `cv` as `not available` in JSON. Rejected: `null` for a number is
  the document's shape for "no value" and the schema key type is a number;
  a string in a number slot breaks the kind check in `checkNode`.
- Treating one sample as clean CV 0 and letting it rank. Rejected: a single
  sample would beat a measured event, so the top N would fill with unmeasured
  events on a tight budget.
- Excluding single-sample events from `hunt.events` entirely. Rejected: D-17
  names the array as the place the plan's columns are recorded, and the
  two-stage plan guarantees such events exist.

## D-15 Console order

**Decision**: the plan prints before the run. After the benchmark tables the
console prints, in order: the ranked table (top N), the marked tail of excluded
events, the skipped list, and the plan summary line with the budget spend. All
hunt lines go to stdout with the `hunt:` prefix, and invalid-option refusals go
to stderr.

**Rationale**: D-20 fixes the order and the stream. `printResult` in
`report.cpp:410` owns the per-benchmark tables and `printPassDisclosure` at
line 251 is the precedent for a disclosure printed by the same path, so the
hunt printers join `report.cpp` and reuse `availabilityName` (line 36) and
`metricMethodName` (line 216) instead of writing a second name table.

**Alternatives considered**:

- Printers in a new `hunt_report.cpp`. Rejected: the printers need three helpers
  that live in `report.cpp`, and exporting them to a new file would widen the
  internal surface for no separation gain. `report.cpp` is a shared file with
  specs/027 either way; the hunt edits append functions at the end.
- Plan printed after the run only. Rejected: D-20 requires the pre-run plan so a
  reader sees the passes before the rows scroll past.
- Skipped list on stderr. Rejected: it is reported output, D-20 names stdout, and
  `example_run` reads stdout to check that a declared outcome printed.

## D-16 Refusal point

**Decision**: option values are validated and stored at the scan. Selection
expansion (list, glob, `@file`, `all`) runs after the scan and before the first
run, where the providers are registered, and every refusal there prints one line
naming the value to stderr and exits 2. A raw event code of the shape
`r[0-9a-fA-F]+` is refused there with its own message. A bad shape (raw code,
malformed glob, empty address line) is refused before the catalog is consulted,
so `r4a03` on a host that happens to encode it still exits 2 (spec D-11).

**Rationale**: `speedgunMain` registers the clock and PMU providers before the
option scan (`cli.cpp:326-339`), so a refusal inside the scan can consult the
catalog. Doing the shape check first keeps the refusal host-independent, which
is what a CI host without PMU access needs to test the same line.

**Alternatives considered**:

- Expanding inside the scan per option occurrence. Rejected: the catalog is
  empty for a `--hunt=` that precedes provider registration in a suite that
  registers its own provider, so the same text would refuse on one host and
  resolve on another, and the order of the suite's registrations would decide.
- Returning an error string and letting `cli.cpp` print it. Rejected: it moves
  hunt's refusal text into the file specs/028 is rebuilding; `hunt_cli.cpp`
  prints its own line and exits, the way `rejectPercentileList` at
  `cli.cpp:73-81` does for the percentile list.

## D-17 Option wiring

**Decision**: eight named constants in `cli.cpp` at values 270 to 279:
`kHuntOption` 270, `kHuntNormOption` 271, `kHuntRankOption` 272,
`kHuntTopOption` 273, `kHuntNoiseOption` 274, `kHuntRepeatsOption` 275,
`kHuntBudgetOption` 276, `kHuntSeedOption` 277, leaving 278 and 279 for the
hunt lane's later options. `hunt_cli.cpp` holds the parse functions, the value
ranges, the refusal texts, the `@file` reader, the glob matcher, the raw-code
refusal and the conflict check. `cli.cpp` gains the eight rows of
`option longOptions[]` and eight switch arms that call one `hunt_cli` function
each. `--hunt` is the tree's first `optional_argument` row, so its value form is
`--hunt=all` with no space.

**Rationale**: values 256 to 259 are taken (`cli.cpp:69-71,345`) and 270 to 279
are free (spec PC-5). D-22 keeps the parse bodies out of `cli.cpp` because
specs/028 (N7) rebuilds that file, and a row-and-call edit is the smallest
possible rebase there. `example_drift.py`'s `cli_options` (line 458) reads the
single `option ... [] = { ... }` table by regular expression, so all eight rows
must live in that table for the README option-row parity to see them. The
conflict check runs after the scan, where the settled `RunOptions` shows whether
`counterAddresses`, `metricNames` or `compareBaseline` are populated
(`detail/internal.hpp:444,455,463-476`).

**Alternatives considered**:

- Continuing 260 upward with no gap. Rejected: the spec reserves 270 to 279 so
  the hunt family keeps a contiguous block that a later hunt option joins
  without a renumber, and a 260 start would collide with the next feature that
  takes the next free numbers.
- One `--hunt-*` subcommand parser in `cli.cpp` behind a helper. Rejected: it is
  the same amount of code in the shared file, which is exactly what D-22 avoids.
- Short letters for the hunt options. Rejected by D-22 and by the scanner shape
  at `cli.cpp:343-345`, which keeps the scan long-only so no later option can
  reach a switch arm by claiming a letter.

## D-18 Option table test

**Decision**: `test/source/harness_cli_table_test.cpp` reads the source of
`cli.cpp` through a compiled-in `SG_CLI_SOURCE` path, parses the one
`option longOptions[]` table and the named value constants, and asserts that no
name repeats, that no value repeats, that 270 to 279 are claimed by the hunt
rows, and that 280 to 289 stand free. The fixture twin of a table with a planted
duplicate and a planted value clash lives under `test/source/detail/`.

**Rationale**: hard rule 10 gives the file to the first lane that merges, and
`git ls-files | grep harness_cli_table` shows no such file at `bd10d76`, so this
lane creates it and hard rule 9 makes it a requirement (FR-082, AC-4). The
option table is a function-local `static` array in `speedgunMain`
(`cli.cpp:346`), so no linked test can read it; reading the source is what the
gate `example_drift.py:458-465` already does for the same table, so the test
follows a pattern the tree runs in CI. The `SG_SCHEMA_PATH` compiled-in path
(`test/source/harness_json_schema_test.cpp:21-22`) is the registration pattern,
and the golden path hand-off at `test/CMakeLists.txt:1089-1091` is the
alternative the fixture files use.

**Alternatives considered**:

- A bash gate script under `test/`. Rejected: the rule belongs beside the option
  table a contributor edits, and a CTest case reports a failure in the same run
  as the tests around it.
- Moving the table to file scope so a test can read it. Rejected: it changes the
  file specs/028 is rebuilding, for a reach no other test needs.
- Checking uniqueness inside `hunt_cli.cpp` at run time. Rejected: a duplicate
  that a compiler accepts must be reported by a build-time check. A run that
  happens to select the wrong arm cannot report it.

## D-19 Docs example ownership

**Decision**: `example/docs/hunt_rank.cpp` owns the "Hunter mode" section
through its marked regions, registers benchmarks that print rows on every host,
and runs its hunt under a scripted provider whose events are not countable, so
the skipped list prints the same on every host. `example/docs/hunt_rank.expected`
holds the recorded run in the `# sg-example:begin report` form the other
recorded goldens use, and `report_shape_hunt_rank_sample` compares the binary's
stdout against the quoted block by shape.

**Rationale**: IV.1 requires a main-page snippet from an example CI builds and
runs, `example_run.sh:35-36` iterates `example/docs/*.cpp` and fails a docs
example with no built binary, so `add_docs_example(hunt_rank)` in
`example/CMakeLists.txt` is required, and `test/example_run.sh` checks that a
declared outcome printed. The spec asks the example to print its skipped list on
every host, and the scripted refusal is the only host-independent way to get
one: on a CI host without PMU access every real event is refused, whose kind set
varies with the host, so a scripted host is what makes the golden stable. The
`report_shape` per-example registration at `test/CMakeLists.txt:1185-1192` is the
pattern for the shape check, and `report_shape.py:123` binds the golden by
`<example>.expected`.

**Alternatives considered**:

- Real PMU events in the example with a host-dependent golden. Rejected: the
  spec requires host independence, and `example_run` runs the binary on CI hosts
  with no PMU access at all.
- A shell block owned by `docs/pages/harness.md`. Rejected: `SECTION_OWNERS`
  gives each main-page section exactly one `.cpp` owner, and a shell block
  cannot satisfy `snippet_compile` for the C++ block the section carries.
- No `.expected` file. Rejected: FR-070 and AC-9 require a recorded golden the
  gate can compare, and the shape comparison is what tolerates the digits that
  do move.

## D-20 Version, ABI and schema reservations with the rebase rules

**Decision**: `VERSION 0.19.0` planned past the `0.17.0` specs/027 and `0.18.0`
specs/028 reservations; harness `SOVERSION` 10 to 11 for the public hunt calls
and the two new record fields; archive `SOVERSION` 3 to 4 for the scripted
refusal on the exported `FakeProvider`; `kSchemaVersion` (`json_report.cpp:31`)
`1.1` to `1.4` with `docs/schema/report-v1.json` at minor 1.4. All four numbers
are reservations with the same rebase rule: read what master states at rebase
time, take the next free value, carry the keys of the lanes that merged first,
and record the value with its reason in the merge notes.

**Rationale**: hard rules 11 and 12 fix the pattern, and the comment blocks at
`CMakeLists.txt:43-52` and `:238-260` show the form each move is recorded in.
Both ABI moves rest on an exported-layout fact; no rule alone carries them: a
new
exported header with new calls plus `BenchmarkResult::hunt` and
`ReportData::hunt` on the harness side; a new scripted member inside the
exported `final` class `FakeProvider` (`counters_fake.hpp:103`, the member block
at 258-272) on the archive side. On the schema side the root object is open
(`report-v1.json:9`, `additionalProperties: true`) while the per-entry object is
closed with its own `x-order` and `required` (lines 28-51, ending at
`spreadWarning`), so an entry key that is absent makes a document invalid and
the reader's optional-field path cannot rescue it (spec PC-5). That is the
reason `hunt` joins `x-order` and `properties` and stays out of `required`, the
exception D-17 records and hard rule 11's sentence about adding a required key
does not cover.

**Alternatives considered**:

- Bumping the schema major. Rejected: the reader knows one major and refuses any
  other (`json_load.cpp:1214-1219`), so a major bump would make every stored
  document unreadable by the next build for keys this build adds.
- Adding `hunt` to the entry `required` list. Rejected: `readEntry` reads a
  present-or-absent key through `optionalField` (`json_load.cpp:103`) and a
  required entry key that stands absent fails the schema check before the reader
  can call it, so documents written before this feature would fail validation.
- Leaving the archive `SOVERSION` alone because only a test helper changed.
  Rejected: hard rule 12 and the 0.x rule stated at `CMakeLists.txt:43-47`; the
  scripted member is inside an exported class a consumer links against.
- Settling on `0.17.0` now. Rejected: the version line is a shared file with the
  two lanes that reserved `0.17.0` and `0.18.0`, and a lane that merges later
  takes the next free minor; `0.19.0` is the value the reservations imply today
  and the merge notes record the value that landed.
