# Feature Specification: Hunter Mode Core (Event Ranking)

**Feature Branch**: `029-hunter-core`

**Created**: 2026-10-22

**Status**: Ready

**Input**: User description, opening quoted. The request of 2026-10-22 is
the owner's pasted prompt for roadmap item N14a, section 12 of
`/workspace/speedgun-ng-harness-roadmap.md`. Its hard-rules, version-rule
and rebase paragraphs restate constitution VIII and IX plus the
three-lane rules, so they are summarized where they bind this feature and
not copied.

> Hunter mode core (event ranking), item N14a of the roadmap (section
> 12). Short name: hunter-core. Target machine: corsairone (Intel, one
> socket). Lane: counters. N2 and N6 are done. N14a is next, then N6-P,
> N10, N14c, N13c and N13d. N14a runs in parallel with N7 (gary, spec
> 028) and N13a (thrawn, spec 027). The Zen 5 acceptance host is gary,
> when its lane has a gap.
>
> Goal: find the counter events that change a benchmark most. `--hunt`
> measures many catalog events, or all core events of the host, over N6
> passes. It normalizes each event against the cycles and instructions
> anchors of its own pass. It ranks the events by rate and by noise. It
> prints a ranked table and writes a JSON hunt section. N14a-2 adds the
> sample and multiplex budgets, `--hunt-emit-set` and
> `--counter=@<file>`. N14b adds sensitivity, the baseline delta and the
> N5 rules output. N14c adds the top-down area.

## Spec identity and run-time binding

- Spec number 029. Checked at run time on 2026-10-22: `specs/` on master
  holds 001 to 026, no remote branch matches `02[7-9]`, and `gh pr list`
  shows no open pull request. 027 is reserved for N13a (thrawn) and 028
  for N7 (gary), so 029 is free and was taken as planned.
- Branch `029-hunter-core` from master `bd10d76` (dated 2026-10-21,
  version 0.16.0, harness `SOVERSION 10`, archive `SOVERSION 3`).
- The master facts below cite `bd10d76` lines as evidence. At run time,
  find each item by its symbol; line numbers stand as audit evidence
  alone.
- Version rule. The planned merge order is N13a (0.17.0), N7 (0.18.0),
  N14a (0.19.0). The planned version is a reservation: at rebase, read
  `CMakeLists.txt` on master and take the next free minor. If N14a
  merges before the other lanes, it takes the lowest free minor and the
  other lanes rebase. Record the value and the reason in the merge
  notes.
- Schema: N14a reserves schema minor 1.4 (hard rule 11). At rebase, take
  the next free minor and add the keys of the lanes that merged before.
- ABI check of hard rule 12, recorded here and corrected at the audit
  point: `hunt.hpp` adds new public calls, so the harness public ABI
  changes and the harness `SOVERSION` rises from 10 to 11. PC-3
  measured that `FakeProvider` is `final` and scripts no open failure;
  D-18 requires a test that sets one, so the seam joins
  `counters_fake.hpp`, which the archive library exports. That changes
  the counters library ABI and the archive `SOVERSION` rises from 3 to
  4.

## Decisions

These stand as the recommendations of this prompt. They are recorded
here per X.1 so that no implementer reopens them, and each one names
the reading it settles.

- **D-1 `--hunt` with no value means `--hunt=all`.** All catalog events
  of the core PMU devices, with no uncore device. A core device is
  identified by the device part of the catalog address: `cpu`,
  `cpu_core`, `cpu_atom` and `armv8_*`. Uncore events enter only through
  an explicit list or glob. PC-8 records the address form on master.
- **D-2 The budget is two-stage.** `--hunt-budget=<seconds>s` or
  `<passes>p`. Stage 1 runs all events with one repeat. Stage 2 runs the
  top K = 3 × top N with the full repeats. With no budget, all events
  run with the full repeats. The plan code takes the calibration
  estimate as data and reads no clock.
- **D-3 The rank key is `--hunt-rank=rate|cv`, default `rate`.** The
  keys `sensitivity`, `delta` and `score` exit 2 with a message naming
  N14b.
- **D-4 Events come only from the `sg::counters` catalog**, which reads
  the vendored `external/pmu-events` tables. N14a adds no list of its
  own, no table, no generator, and no license change.
- **D-5 N14a is one spec.** N14a-2, N14b and N14c get their own specs
  later.
- **D-6 Normalization `--hunt-norm=instr|cycle|iter`, default `instr`.**
  The rate is per 1000 instructions of the same pass ("PKI"). `cycle`
  normalizes per 1000 core cycles; `iter` reports the raw per-iteration
  value.
- **D-7 Repeats and noise.** `--hunt-repeats=<n>`, default 3, range 1 to
  1000. An event is noisy when its CV is above `--hunt-noise=<percent>`,
  default 10. Noisy events stay in the JSON, the console marks them, and
  they do not enter the top N.
- **D-8 Top N `--hunt-top=<n>`, default 20, range 1 to 10000.**
- **D-9 Seed `--hunt-seed=<u64>`, default 0.** The report states the
  seed. The order inside a stage comes from the seed and the catalog
  order, never from a clock.
- **D-10 Skipped events.** An event whose `Availability` is not
  `COUNTABLE` at discovery goes to the skipped list with its
  `Availability` kind. An event whose window reports a non-`COUNTABLE`
  `Availability` during the run also goes to the skipped list. The run
  continues and exits 0. An unknown address, a glob that matches no
  event, or a bad list file exits 2 and names the value.
- **D-11 Raw codes (`r<hex>`) exit 2.** Master has no raw form, so N14a
  refuses them with status 2. Raw codes stay an open question: they
  change the counters ABI and would raise the archive `SOVERSION` to 4.
- **D-12 Hosts with no PMU access (CI runners): all events go to the
  skipped list, the run prints the plan and exits 0.**
- **D-13 Anchors.** If cycles or instructions is not `COUNTABLE`, the
  run falls back to `iter` only and discloses it.
  `--hunt-norm=instr|cycle` exits 2 only when the user set it.
- **D-14 List file (`--hunt=@<file>`):** one catalog address per line,
  `#` starts a comment. A bad file exits 2 and names the value.
- **D-15 Library surface:** `include/speedgun-ng/hunt.hpp` with
  `sg::hunt::EventCatalog`, `planHunt`, `runHunt` and `rankEvents`, plus
  the options structs `HuntOptions` and `HuntBudget` (hard rule 8: an
  options struct, never a bool). No `detail/` type appears in a public
  signature.
- **D-16 Conflicts.** `--hunt` with `--counter` or `--metric` exits 2.
  `--hunt` with `--compare` exits 2. `--hunt` with
  `--metric-method=multiplex` uses multiplex for all events and
  discloses it.
- **D-17 JSON.** A top-level `hunt` object (seed, norm, rank, repeats,
  noisePercent, plan with events, passes, method and stages) and a
  `hunt` object in each benchmark entry (events: address, rate, cv,
  noisy, multiplexed, rank or null; skipped: address and Availability
  kind). A run without `--hunt` writes no hunt object.
- **D-18 Group-open failure.** When the group open of a pass fails, the
  events of that pass run again one at a time, each with the two
  anchors. An event that still fails goes to the skipped list with its
  kind. The run exits 0. A test sets a `FakeProvider` open failure.
  PC-9 records what the N6 pass loop does today.
- **D-19 Column plan.** One `ColumnLeaves` per hunt column: the event
  plus cycles plus instructions. The shared anchors count one time in
  each pass, so a counter limit L gives L - 2 events in each pass. When
  the device has no limit, the plan uses `MULTIPLEX` and discloses it
  (D-16 covers the user-requested multiplex case).
- **D-20 Console order.** The ranked table of the top N prints after the
  benchmark tables, then the skipped list, then the plan. The plan
  itself prints before the run: the events, the passes and the time
  estimate.
- **D-21 This lane creates `test/source/harness_cli_table_test.cpp`.**
  Hard rule 10 gives the file to the first lane that merges. Checked at
  `bd10d76`: the file does not exist on master, so N14a creates it. It
  checks that each option name and each option value is unique.
- **D-22 Option wiring stays split for the N7 rebase.** Option values
  270 to 279 are N14a's (named constants, hard rule 9; 280 to 289 stay
  free for N14a-2; no short option). The option parsing lives in its own
  file `source/harness/hunt_cli.cpp`; `cli.cpp` gains only the table
  rows, so the rebase onto N7's unified table is small.
- **D-23 The hunt planner plans through the N6 planner.** `planHunt`
  emits `ColumnLeaves` sets for `planPasses`. N14a adds no second pass
  loop and no second counter path (hard rule: every counter access goes
  through `sg::counters`; no `perf_event_open` call outside the counters
  library).

## Audit point

The audit ran against master `bd10d76` on the branch `029-hunter-core`
before `/speckit.plan`. The Preconditions below record each PC with its
evidence. Find every item by its symbol.

## Preconditions

All nine preconditions carry their evidence below, read from the tree
at `bd10d76` by symbol on 2026-10-22.

### PC-1: the catalog API, the refusal states and the address form. PASS-with-note.

- `System::object(path)` answers `std::expected<Object, Error>` with
  near-miss diagnostics; `Object::counters()` answers
  `std::vector<CatalogEntry>` by value, so a caller keeps the vector
  while holding views into it; `Object::counter(name)` resolves one
  leaf. `Object::programmableCounters()` answers the per-device
  programmable-counter limit as `std::optional`.
- `CatalogEntry` fields stand exactly as cited; there is no address
  field. The address is `path + "/" + name`, composed at registration
  and read back through `Counter::address`.
- `Availability` carries all six kinds the skipped list names.
  Discovery (`probeDevice` with one test-open per target kind) is the
  only place a hunt can collect `PERMISSION_BLOCKED`,
  `NOT_ENCODABLE` or `SCOPE_REFUSED`: a device-scoped entry whose cpu
  probe was refused reads `SCOPE_REFUSED`, and planning one
  non-`COUNTABLE` leaf anyway fails the whole spine compile with one
  recoverable error naming the catalog state. `ABSENT` arises from an
  unresolvable address or the scripted seam.
- Mid-run, the only non-`COUNTABLE` state the fold publishes is `GAP`
  (an unmeasured window end). D-10's second branch and D-18's retry
  trigger are exactly that state.
- Uncore devices each form their own object with a distinct device
  part (`uncore_imc_0`, `uncore_cha_3`, `amd_df` after the label
  folds). The library's core-device predicate names `cpu`, `cpu_core`
  and `cpu_atom` only; the `armv8_` prefix rule of D-1 is new
  hunt-side code with no library helper.

### PC-2: the N6 planner takes hunt-width plans. PASS-with-note.

- `planPasses` is a width-agnostic first fit: per pass a shared leaf
  claims one counter (`claimedAddresses`), each column's deduplicated
  ask is tested against the published limits, and the closing asserts
  pin request order and the per-pass bound. No column-count constant
  exists in it.
- `ColumnLeaves::source` indexes the caller's `sourceLimits` vector,
  which the harness builds lazily by device path from
  `Object::programmableCounters()`. Naming an unpriced source trips
  the contract and plans nothing.
- Leaf set over the limit means more passes; a column wider than every
  limit opens a pass flagged `exclusive` that the kernel rotates.
- Device reports no limit means `MULTIPLEX` already: `planPasses`
  answers an empty plan when no source answers and `settleMethod`
  settles `MULTIPLEX` for every request. D-19's no-limit row needs no
  planner change.
- The spine compile carries no leaf-count or column-count ceiling. The
  real bound is the one perf group per device inside it: a refused
  member makes the whole group open fail. The value there is the
  device's schedulable set: the published limit answers it, where no
  code constant exists. `kMaxUserCounters` 16 caps named `--counter`
  columns alone; hunt columns stand outside that cap.
- Note: `limitForDevice` answers only for `cpu`, `cpu_core` and
  `cpu_atom`, and a hybrid host answering leaf 0x23 publishes no limit.
  Arm hosts therefore plan as `MULTIPLEX`, never as L - 2 per pass; the
  limit-of-6 shape of AC-1 is reachable through the scripted seam.

### PC-3: FakeProvider scripts values, availability and the limit, and has no open-failure seam. FAIL.

- Values per read: `setPoints(objectPath, name, points, tailDelta,
  deltaSeed)` with a per-window playback cursor and `readActions()`.
- Availability per counter: `addCounter(..., avail, ...)` is a catalog
  fact frozen at open; the only runtime state is `GAP`, scripted per
  leaf with `setGapActions`.
- The limit: `setProgrammableCounters(objectPath, limit)` before
  `System::open`; unscripted objects answer no limit.
- Open failure: no API. `FakeProvider::open` answers `nullptr` only for
  an undeclared object or counter, and `compileCore` refuses an unknown
  or non-`COUNTABLE` leaf before it ever calls `open`, so even that
  route is unreachable from a harness run. The class is `final`, so a
  test cannot subclass it either.
- The only open-failure seams today are hand-written `ProviderIface`
  subclasses at the counters-library test level; the harness CLI level
  cannot reach them. The scripted-host fixture
  (`test/source/detail/pass_fixture.hpp`) scripts `cpu`-family objects
  and limits before registration and is the shape to build on.
- Consequence, carried by D-18: the open-failure seam joins the public
  fake (scripted per address or per leaf set), and by hard rule 12 the
  archive `SOVERSION` rises to 4 (the ABI record above).

### PC-4: the N1 median/CV aggregate is one TU-local helper, not a reusable one. PASS-with-note.

- The public aggregate is `sg::Statistics` (`benchmark.hpp:Statistics`),
  the record with `mean`, `median`, `sampleStdDev`,
  `coefficientOfVariation`; no public function produces it.
- The only computing site is `statisticsOf` in the anonymous namespace
  of `source/harness/runner.cpp`: sort, even-count median of the two
  central values, `n - 1` standard deviation, CV guarded so a zero mean
  answers 0.0. The three call sites (time, metric, counter columns)
  guard the empty list with the zero record.
- `report.cpp`, `compare.cpp:aggregateValueOf`, `json_report.cpp` and
  `json_load.cpp` all read the record instead of recomputing.
- Two other medians exist with different conventions and must not rank
  a hunt: `pass_plan.cpp:passSpread` takes the lower central value and
  `counters/plan.cpp` the upper. Reusing `statisticsOf` is what makes a
  hunt CV equal the printed `cv=` column.
- The plan extracts `statisticsOf` into a named header under
  `source/harness/detail/`, keeps it a named function for the dbc-gate
  form, and calls it from `runner.cpp` and the hunt alike (FR-030).
  `sg::Statistics` arrives whole; a narrower CV-only helper is not
  worth a second convention.

### PC-5: the JSON writer, reader and schema seams take the two hunt objects. PASS-with-note.

- Writer: `json_report.cpp:writeReport` opens the root object writer and
  appends the top-level keys; a top-level `hunt` object is one
  `root.key("hunt")` block with the RAII writers owning commas and
  brackets. The per-entry object goes in `writeEntry` after the pass
  keys of 024, the last-appended position.
- Reader: `loadReport` gates the schema major only; the per-entry read
  is `readEntry`, all scalars use the `.get(out)` error-code form
  through the `field`/`optionalField` helpers, and each refused code
  carries its own test (hard rule 7).
- The root object of the schema is open and the per-benchmark object is
  closed: `checkNode` refuses an unlisted key only in the closed object
  and enforces the relative `x-order` there. So both hunt keys join
  `properties` and `x-order`; the per-entry key must, or hunt
  documents fail the schema check.
- Deviation from the 024 precedent, by D-17: the hunt keys stay out of
  `required`, because a non-hunt run writes no hunt object, and the
  reader uses `optionalField` for them.
- The tests that pin forward compatibility stay green by construction:
  an unknown key in an open object loads unchanged, a pre-024 document
  still loads, and written documents are schema-checked through
  `SG_SCHEMA_PATH`.

### PC-6: no raw r hex event form exists in sg::counters. PASS.

- No raw production exists: the encode path accepts sysfs format specs
  (`<field>:<low>-<high>`) and `(field-name, integer)` pairs only, and
  the two accepted event spellings (kernel alias fields, vendored
  table rows) are both name-based. Names are looked up, never decoded:
  `r4a03` today is an unknown-name catalog error.
- Note for D-11: master treats a raw `--counter` value as an ordinary
  address that fails to resolve, marks the column `ABSENT` and keeps
  running. The exit-2 refusal therefore lives in
  `source/harness/hunt_cli.cpp`, and the hunt never composes a column
  from an unresolved address.

### PC-7: no catalog address can begin with @. PASS-with-note.

- Measured over the whole pinned table tree (542 files, 46,823 event
  rows): event names use `[0-9A-Za-z_.]` only, start alphanumeric, and
  none contains `@`. Metric rows use `%` but never reach a device
  catalog.
- A kernel and data convention holds this property; no code invariant
  enforces it:
  nothing refuses an `@` in a future table row or sysfs alias, so the
  `@` interception belongs where the option value is read, exactly as
  the future `--counter=@<file>` will need.
- Addresses split on the last `/`; a leading `@` does not disturb
  resolution.

### PC-8: the catalog gives the anchors as cpu/cpu-cycles and cpu/instructions. PASS.

- The address form is `<device>/<name>`: `system.cpp:buildLeaves`
  composes it, and the device part is the sysfs directory name under
  `/sys/bus/event_source/devices`.
- Kernel aliases provide the anchors; the vendored rows carry other
  events: the catalog
  exposes `cpu/cpu-cycles` and `cpu/instructions` (and the same two
  names on `cpu_core` and `cpu_atom` of a hybrid part); the bare
  `cycles` string exists only in tolerant test lists. The hunt uses the
  catalog address and invents no alias.
- `CatalogEntry` carries no address field, so the hunt discovery
  composes `Object::path() + "/" + entry.name`.
- `--hunt=all` over the core device parts also selects `cpu/enabled`,
  `cpu/running` and the `topdown-*` aliases, ordinary catalog entries
  under D-1's device rule.
- One hunt column stays on one device: a pass anchor comes from the
  same device as its event. The `armv8_*` core rule of D-1 is a new
  prefix rule with no code precedent (no such string exists on
  master).

### PC-9: a mid-run group-open failure aborts on a contract, it does not fail the run. PASS-with-note.

- The seat for a pass is `seatPass` in `runner.cpp`; it collapses an
  open refusal and a compile refusal into one "does not compile"
  message. Pass 0 is the only clean failure: the run marks FAILED and
  the CLI exits 1.
- Every later seat checks the seat with `SG_INVARIANT` only. Under the
  default contract semantic that is `std::abort()` (SIGABRT): the
  process dies with no row, no report, and status 134. The pass loop
  documents the opposite assumption (the frozen catalog keeps every
  pass compilable), and a later open failure violates exactly it.
- Consequence, carried by D-18 and FR-021: the per-pass retry needs
  the seat to return its error inside the loop; the invariant route
  answers with the contract handler. The retry itself runs each event
  of the failed pass with
  the two anchors, inside the same pass loop, adding no second loop.

## Roadmap position

- N1 (statistics), N2 (formulas), N3 (JSON reports), N5 (regression
  gate) and N6 (multi-pass collection) are merged on master.
- N14a follows N6 in the counters lane. Next in the lane: N6-P (group
  probe), N10 (top-down tree input), N14c, N13c, N13d.
- Parallel lanes: N13a is spec 027 (thrawn), N7 is spec 028 (gary). N7
  moves all options into one internal table; this lane keeps its option
  code in `hunt_cli.cpp` per D-22 so the rebase stays small.
- Zen 5 acceptance happens on gary when its lane has a gap; AC-10 records
  both hardware runs.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Find the events that move the benchmark (Priority: P1)

A performance engineer points the harness at a benchmark with
`--hunt=all`. The harness lists the catalog events of the host core
PMUs, plans passes through the multi-pass planner (each pass carries its
cycles and instructions anchors), runs the benchmark the requested
number of repeats over those passes, normalizes every event against its
own pass anchors, and prints a ranked table: rate first, then the CV,
with noisy events marked and kept out of the top N. A skipped list names
every event the host refused, with the reason.

**Why this priority**: this is the item's whole point. Without ranking,
the other pieces (budget, JSON, docs) have no purpose.

**Independent Test**: run the binary against a scripted provider whose
events carry known values (AC-3): a high-rate fake event ranks first, a
flat high-CV fake event is marked noisy and excluded from the top N.
Delivers the ranked table and the exit status.

**Acceptance Scenarios**:

1. **Given** a scripted host with 200 fake events and a counter limit of
   6, **When** the user runs `--hunt=all --hunt-repeats=3`, **Then** the
   plan has passes of 4 events each and every pass carries the cycles
   and instructions anchors exactly once (AC-1).
2. **Given** a host whose device reports no counter limit, **When** the
   user runs `--hunt=all`, **Then** the plan method is `MULTIPLEX` and
   the output discloses it (AC-2).
3. **Given** events the host refuses (`PERMISSION_BLOCKED`, `ABSENT`,
   and friends), **When** the user runs `--hunt=all`, **Then** each
   refused event appears in the skipped list with its kind and the run
   exits 0 (AC-4).
4. **Given** a host with no PMU access at all (a CI runner), **When**
   the user runs `--hunt=all`, **Then** every event lands in the skipped
   list, the plan still prints, and the run exits 0.

---

### User Story 2 - Bound the time and see the plan (Priority: P2)

Before anything runs, the harness prints the plan: the events, the
passes, the method and a time estimate. With `--hunt-budget=120s` the
run is two-stage: stage 1 sweeps every event once, stage 2 re-runs the
top 3 × N events with the full repeats. With `--hunt-budget=8p` the
budget is a pass count. Without a budget every event runs with the full
repeats.

**Why this priority**: a full hunt on a real host is hundreds of events;
without a budget it cannot finish inside a work break, which is the
common case on corsairone and gary.

**Independent Test**: give the planner a fixed calibration estimate and
a fixed seed and check the two-stage plan (AC-7). Delivers the bounded
run and the printed plan.

**Acceptance Scenarios**:

1. **Given** 200 events, a limit of 6, top 4 and a pass budget, **When**
   the planner runs with the fixed seed and estimate, **Then** stage 1
   covers all events at one repeat and stage 2 covers the top 12 at the
   full repeats (AC-7).
2. **Given** `--hunt-budget=120s`, **When** the plan prints, **Then**
   the estimate line names the seconds budget and the two stages.
3. **Given** `--hunt-budget=garbage`, **When** the run starts, **Then**
   one line names the value and the exit status is 2 (AC-5).

---

### User Story 3 - Read the hunt from the report (Priority: P2)

A hunt writes a top-level `hunt` object (seed, norm, rank, repeats,
noise threshold, and the plan: events, passes, method, stages) and a
`hunt` object inside each benchmark entry (per-event rate, CV, noisy,
multiplexed and rank; per-event skipped reasons). `loadReport` reads
both back with equal values. A run without `--hunt` writes no hunt
object: its JSON differs from the master output only in the schema
version value, and its console report and status are byte-identical.

**Why this priority**: the JSON report is how a hunt feeds later tools
(the N14b rules output, N5 comparisons); round-trip fidelity is the
contract N3 established.

**Independent Test**: write a hunt report through the writer, read it
with `loadReport`, compare (AC-6). Delivers the machine-readable
record.

**Acceptance Scenarios**:

1. **Given** a hunt run, **When** the report is written and read back,
   **Then** every hunt field reads equal (AC-6).
2. **Given** a run without `--hunt`, **When** its JSON is compared to
   the same run on master, **Then** the bytes differ only in the
   `schemaVersion` value (AC-6).
3. **Given** a schema minor already taken at rebase, **When** the
   branch rebases, **Then** it takes the next free minor and carries the
   earlier lanes' keys in `docs/schema/report-v1.json`.

---

### User Story 4 - Read the page and copy a working block (Priority: P3)

The README gains a "Hunter mode" section of at most 25 lines, owned by
the runnable example `example/docs/hunt_rank.cpp`. The example uses a
scripted provider whose events are not countable, so the recorded
`.expected` output shows the skipped list on every host, including CI
runners and hosts without PMU access. `docs/pages/harness.md` carries
the detail.

**Why this priority**: the constitution (IV.1) requires the main-page
section and the CI-run example with every user-visible feature.

**Independent Test**: `run_docs_hunt_rank` passes on this machine and
matches its `.expected` on any host (AC-9).

**Acceptance Scenarios**:

1. **Given** the docs example, **When** CI runs it, **Then** the output
   matches the `.expected` file without depending on the host PMU
   (AC-9).
2. **Given** the README after the change, **When** the drift gate runs,
   **Then** the "Hunter mode" section is at most `SECTION_LINE_CAP`
   lines and no new `SECTION_LINE_CAPS` entry exists.

---

### Edge Cases

- A glob matches no catalog event: one line names the value, exit 2.
- An unknown catalog address in a list: one line names the value, exit
  2.
- A raw code such as `r4a03`: exit 2, names the value (D-11).
- A list file that does not exist, or a line with an empty address: exit
  2, names the value; `#` comment lines and blank lines are ignored.
- `--hunt` together with `--counter`, `--metric` or `--compare`: exit 2
  (D-16). `--hunt` with `--metric-method=multiplex` is legal: every
  event is multiplexed and the output says so.
- `--hunt-rank=sensitivity|delta|score`: exit 2 with a message naming
  N14b.
- Out-of-range `--hunt-repeats` (below 1 or above 1000) or `--hunt-top`
  (below 1 or above 10000): one line names the value, exit 2.
- Cycles or instructions not countable: the run normalizes by iteration
  count only and discloses the fallback; the fallback becomes an exit-2
  refusal only when the user explicitly asked for `instr` or `cycle`
  (D-13).
- The group open of a pass fails mid-run: the events of that pass retry
  one at a time with the two anchors; an event that still fails goes to
  the skipped list; the run exits 0 (D-18).
- The same address listed twice, or through two globs: one column
  (deduplication by catalog address).
- Duplicates between the hunt list and the fixed time leaves: the fixed
  columns are not hunt columns; only catalog events rank.
- Fewer distinct events than the limit: one pass, still with the two
  anchors.
- Zero events after discovery (empty explicit list): exit 2 names the
  value; `--hunt=all` on a host with an empty catalog skips everything
  and exits 0 per D-12.

## Requirements *(mandatory)*

Requirements use the capability ids of the request (C-1 to C-8).

### Discovery (C-1)

- **FR-001**: `sg::hunt::EventCatalog` SHALL build its event list from
  the `sg::counters` catalog (`System::object(path)` then
  `Object::counters()`), from user lists given as
  `--hunt=<list>|all|none|@<file>` entries, and from globs over catalog
  addresses.
- **FR-002**: Discovery SHALL deduplicate by catalog address and SHALL
  keep the catalog order as the base order (before the seed shuffle of
  D-9).
- **FR-003**: Discovery SHALL add no event table, no table generator,
  and no license change (D-4). Host tests use the scripted seam
  (`FakeProvider`); there is no sysfs seam.
- **FR-004**: `--hunt=all` SHALL select exactly the catalog events whose
  address device part names a core PMU device (`cpu`, `cpu_core`,
  `cpu_atom`, `armv8_*`), and SHALL exclude uncore devices (D-1).
- **FR-005**: A list entry naming a raw code (`r` plus hex digits) SHALL
  exit 2 and name the value (D-11).
- **FR-006**: The list file form SHALL accept one catalog address per
  line, treat `#` as a comment start, and exit 2 naming the value for a
  missing file or a malformed line (D-14).

### Plan (C-2)

- **FR-010**: `planHunt` SHALL build one column per hunt event holding
  the event plus the cycles and instructions anchors, and SHALL express
  the plan as `ColumnLeaves` input to the N6 planner (`planPasses`). The
  anchors SHALL count once per pass (D-19, D-23).
- **FR-011**: With a device counter limit L the plan SHALL hold at most
  L - 2 hunt events per pass. With no limit the plan SHALL use
  `MULTIPLEX` and disclose it (AC-2).
- **FR-012**: The plan SHALL print before the run: the selected events,
  the pass layout, the method, and a time estimate. The estimate SHALL
  come from calibration data given to the planner, and the planner SHALL
  read no clock (D-2).
- **FR-013**: `--hunt-budget=<n>s` or `<n>p` SHALL select the two-stage
  run: stage 1, every event at one repeat; stage 2, the top K = 3 × top
  N events at the full repeats. No budget SHALL run every event at the
  full repeats. A malformed budget SHALL exit 2 naming the value
  (D-2, AC-5).
- **FR-014**: Stage order SHALL derive from the seed and the catalog
  order only (D-9).

### Run (C-3)

- **FR-020**: `runHunt` SHALL run the benchmark over the plan through
  the N6 pass loop with `--hunt-repeats` repeats (default 3, range 1 to
  1000). It SHALL add no second pass loop and no second counter path.
- **FR-021**: On a group-open failure of a pass, `runHunt` SHALL retry
  that pass's events one at a time, each with the two anchors; an event
  that still fails SHALL go to the skipped list with its kind and the
  run SHALL exit 0 (D-18).
- **FR-022**: An event whose window discloses a non-`COUNTABLE`
  `Availability` during the run SHALL go to the skipped list; the run
  SHALL continue and exit 0 (D-10).

### Rank (C-4)

- **FR-030**: `rankEvents` SHALL be a pure function over the collected
  data returning, per event: the normalized median rate, the CV, and the
  noise flag (CV above `--hunt-noise` percent, default 10). It SHALL
  reuse the N1 statistics (median, CV) and SHALL NOT copy them (PC-4).
- **FR-031**: Normalization SHALL follow `--hunt-norm`: `instr` (default)
  normalizes per 1000 instructions of the same pass; `cycle` per 1000
  core cycles; `iter` reports the per-iteration median (D-6).
- **FR-032**: `--hunt-rank` SHALL accept `rate` (default) and `cv`;
  `sensitivity`, `delta` and `score` SHALL exit 2 with a message naming
  N14b (D-3).
- **FR-033**: Noisy events SHALL stay in the JSON, SHALL be marked on
  the console, and SHALL NOT enter the top N (D-7).

### Console (C-5)

- **FR-040**: The console SHALL print, after the benchmark tables: the
  ranked table of the top `--hunt-top` events (default 20, range 1 to
  10000), then the skipped list with each `Availability` kind, then the
  plan summary (D-8, D-20).
- **FR-041**: A host without PMU access SHALL show every event in the
  skipped list, print the plan, and exit 0 (D-12).

### JSON (C-6)

- **FR-050**: The writer SHALL emit a top-level `hunt` object: seed,
  norm, rank, repeats, noisePercent, and the plan (events, passes,
  method, stages). Each benchmark entry SHALL carry a `hunt` object:
  events (address, rate, cv, noisy, multiplexed, rank or null) and
  skipped (address, Availability kind) (D-17).
- **FR-051**: `loadReport` SHALL read both hunt objects back with equal
  values; the writer SHALL use the `simdjson` builder via
  `builder::string_builder` and the reader the error-code form, with a
  test per error code (constitution Additional Constraints).
- **FR-052**: A run without `--hunt` SHALL write no hunt object; its
  JSON SHALL differ from master output only in `schemaVersion`, and its
  console report and exit status SHALL be identical (AC-6).
- **FR-053**: `docs/schema/report-v1.json` SHALL gain the hunt keys at
  schema minor 1.4, or the next free minor at rebase, carrying the keys
  of lanes that merged earlier (hard rule 11).

### Budget (C-7)

- **FR-060**: Only the two-stage budget of D-2 is in scope. The sample
  and multiplex budgets belong to N14a-2.

### Documentation and example (C-8)

- **FR-070**: The README SHALL gain a "Hunter mode" section of at most
  25 lines, sourced from the runnable `example/docs/hunt_rank.cpp` with
  a `run_docs_` target, an `example_run` entry and a host-independent
  `.expected` file (AC-9). The example SHALL use a scripted provider
  whose events are not countable, so the output shows the skipped list
  on every host.
- **FR-071**: `docs/pages/harness.md` SHALL carry the option detail; the
  quick-reference table and the option table rows SHALL update in the
  shared lists.
- **FR-072**: The `PAGE_LINE_CAP` SHALL be re-measured and recorded in
  `tools/docs/example_drift.py` through a recorded decision if the
  README outgrows the cap of 773 lines.

### Options and CLI

- **FR-080**: Options: `--hunt[=<list>|all|none|@<file>|glob]`,
  `--hunt-repeats=`, `--hunt-norm=`, `--hunt-rank=`, `--hunt-top=`,
  `--hunt-noise=`, `--hunt-budget=`, `--hunt-seed=`. Long options only,
  named constants with values 270 to 279 (hard rule 9).
- **FR-081**: Every invalid option or value SHALL print exactly one line
  naming the value and exit with status 2 (hard rule 3; AC-5).
- **FR-082**: Option parsing SHALL live in `source/harness/hunt_cli.cpp`;
  `cli.cpp` SHALL gain only the table rows (D-22).
- **FR-083**: `test/source/harness_cli_table_test.cpp` SHALL be created
  by this change (it does not exist on master) and SHALL assert the
  uniqueness of every option name and every option value (D-21, hard
  rule 10).

### Version, tests, and discipline

- **FR-090**: `CMakeLists.txt` SHALL rise to version 0.19.0, or the next
  free minor at rebase (recorded reason), with harness `SOVERSION` 11
  (the new `hunt.hpp` calls) and archive `SOVERSION` 4 (the scripted
  open-failure seam of D-18 joins the exported `FakeProvider`, per
  PC-3).
- **FR-091**: The change SHALL hold 100% line and branch coverage with
  no exclusion marker, no waiver, and no residual row; coverage uses
  `lcov --filter brace` and no code is shaped to move brace counts. A
  gap stops the work and reports the cause with `gcov -b` evidence.
- **FR-092**: Tests SHALL use no sleep, no clock, and no fixed CPU
  count; CPU sets come from `sched_getaffinity`. Tests pass on 4-CPU and
  2-CPU runners without skipping (hard rule 2).
- **FR-093**: Every new public contract SHALL satisfy the dbc-gate form
  (`SG_REQUIRE` per `@pre`, `SG_ENSURE` per `@post`, `SG_ASSERT` per doc
  line), with named helpers (no lambda inside an `SG_*` macro) and
  `[[maybe_unused]]` on contract-only helpers (hard rules 5, 6).
- **FR-094**: The commit series SHALL follow the constitution form, with
  `Refs: specs/029-hunter-core` and `Approved-by: OmO` in one footer
  block; rebase (never merge) and one `--force-with-lease` push per
  round, followed by `git ls-remote origin <branch>` shown in the record.

## Key Entities

- **Hunt selection**: the set of catalog addresses chosen for the hunt,
  after expansion (`all`, list, glob, file) and deduplication by
  address.
- **Hunt column**: one event measured with the two anchors (cycles,
  instructions) of its pass; the unit of ranking.
- **Hunt pass**: the N6 pass the column ran in, with its method
  (`PASSES` or `MULTIPLEX`) and its own anchor values.
- **Ranked event**: address, normalized rate (per the norm), CV, noisy
  flag, multiplexed flag, rank or null (null when excluded or skipped).
- **Skipped event**: address plus the `Availability` kind that excluded
  it (`PERMISSION_BLOCKED`, `NOT_ENCODABLE`, `ABSENT`, `SCOPE_REFUSED`,
  `GAP`).
- **Hunt plan**: events, passes, method, and the stage split (two-stage
  budget or one stage), plus the printed time estimate.
- **Hunt report objects**: the top-level `hunt` object (run parameters
  and plan) and the per-benchmark `hunt` object (ranked and skipped
  events).
- **Options structs**: `HuntOptions` (norm, rank, top, noise, repeats,
  seed, budget) and `HuntBudget` (seconds or passes), per hard rule 8.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: On a scripted host of 200 events with a limit of 6, every
  planned pass holds exactly 4 hunt events and each of cycles and
  instructions exactly once (AC-1).
- **SC-002**: A no-limit device yields a `MULTIPLEX` plan and a visible
  disclosure (AC-2).
- **SC-003**: A scripted high-rate event ranks first; a flat scripted
  event above the CV threshold is marked noisy and absent from the top N
  (AC-3).
- **SC-004**: Every non-`COUNTABLE` scripted event appears in the
  skipped list with its kind, and the run exits 0 (AC-4).
- **SC-005**: Each of these prints one line naming the value and exits
  2: a bad glob, an unknown address, a bad budget, a bad norm, a bad
  rank key, a bad repeat count, a raw code, `--hunt` with `--counter`
  or `--metric` (AC-5).
- **SC-006**: The hunt JSON round-trips through `loadReport` with equal
  values; a non-hunt run's JSON bytes equal master apart from
  `schemaVersion` and its console output and status are identical
  (AC-6).
- **SC-007**: With a fixed seed and estimate, the two-stage budget
  produces exactly the expected plan (AC-7).
- **SC-008**: 100% line and branch coverage with no exclusion; dbc-gate,
  consumer-release, test-rocky, tsan and sanitize green (AC-8).
- **SC-009**: The README section is at most 25 lines; the docs example
  shows the skipped list on every host (AC-9).
- **SC-010**: `--hunt=all` runs once on corsairone with its top 20
  attached under `specs/029-hunter-core/results/`, and once on gary
  (Zen 5) when its lane has a gap (AC-10). On corsairone the recorded
  `perf_event_paranoid` value (2 at the time of this writing) may push
  kernel-scoped events into the skipped list; the disclosure belongs
  to the recorded result. The run stands passed.

## Constraints

The hard rules of the request bind this spec; they restate constitution
VIII and IX. Summarized where they bind:

- Coverage and no-exclusion rule (hard rule 1; FR-091).
- Test determinism rule (hard rule 2; FR-092).
- One-line exit-2 option errors (hard rule 3; FR-081).
- Docs-and-example rule with the 25-line section cap (hard rule 4;
  FR-070, FR-071).
- dbc-gate pairing (hard rule 5; FR-093), `[[maybe_unused]]` and no
  lambda in contracts (hard rule 6).
- simdjson-only JSON with the error-code read form (hard rule 7;
  FR-051).
- Options structs, never bools, for new public calls (hard rule 8;
  D-15).
- Option value ranges 270 to 279, named constants, no short options
  (hard rule 9; FR-080).
- One CLI-table test file (hard rule 10; FR-083).
- Schema minor reservation and rebase rule (hard rule 11; FR-053).
- SOVERSION raised only on a public ABI change, check recorded (hard
  rule 12; recorded above and in FR-090: harness 10 to 11, archive 3 to 4).
- Self-triaged CI: every job read after each push, coverage artifact
  included (hard rule 13; FR-094).
- Rebase, `--force-with-lease`, `git ls-remote` shown, PR never merged
  by this work (hard rule 13 mechanics; FR-094).
- Coverage preset stays as master has it; `ctest -j 2`; no CI change
  without approval (hard rule 14).
- Counters-only: every counter access goes through `sg::counters`; no
  `perf_event_open` outside the counters library; no new event table,
  generator, or license change (hard rule 15; FR-003).
- The README "Hunter mode" section comes from the runnable example
  (hard rule 16; FR-070).
- Prose: no spelling or word splits to pass the prose gate (hard rule
  13 prose note); constitution XI applies to all text.

## Scope

Out of scope:

- the sample and multiplex budgets, `--hunt-emit-set` and
  `--counter=@<file>` (N14a-2);
- sensitivity, the baseline delta and `--hunt-emit-rules` (N14b);
- the top-down area (N14c);
- new event tables and raw event codes;
- Arm hardware runs (scripted tests only);
- the group probe (N6-P);
- compare-rule keys naming hunt fields (none may be added in N14a, per
  N5 rule stability);
- changes to `compare.cpp`.

In scope: capabilities C-1 to C-8 as FR sections above, the shared-list
updates listed in the request (version and SOVERSION lines, README,
quick reference, `example_drift.py` caps, example lists, test lists,
CLI table plus its test file, `docs/schema/report-v1.json`), the branch
mechanics, and the AC-10 hardware records.

## Assumptions

- The catalog address form is `<device>/<name>` (for example
  `machine/monotonic`); PC-1 verifies it and PC-8 records the anchor
  addresses the catalog gives on each core device (sysfs spells one of
  them `cpu-cycles`; the catalog address, whatever it is, is the one
  used; no alias is invented).
- No catalog address begins with `@`, so `--hunt=@<file>` is
  unambiguous; PC-7 verifies the answer for the shared `@` form.
- The N6 planner accepts a leaf set wider than today's metric columns
  without change (PC-2); if the leaf-count ceiling of the spine
  compile rejects the hunt width, the planner splits further (still
  inside the N6 planner, D-23).
- A "repeat" is one full pass-loop run of the benchmark over the plan
  (the N6 pass loop semantics). A re-arm inside one pass counts zero
  repeats.
- The calibration estimate exists as data after the N6 calibration run
  (the iteration count fixed once per benchmark); the estimate per pass
  is that run time times the repeat and pass counts, computed by the
  plan code from that data.
- On hosts where the whole core catalog is refused (`perf_event_paranoid`
  at 2 blocks kernel-scoped counting; corsairone recorded 151 of 279
  refused in the 021 hardware record), `--hunt=all` still runs and the
  skipped list carries the refusals with their kinds.
- CI runners have no PMU access, so the CLI-level hunt tests run
  through the scripted seam and the docs example stays
  host-independent.
- The seed shuffle is a deterministic permutation seeded by
  `--hunt-seed` applied to the catalog order (any standard seeded
  shuffle of the standard library that all CI compilers agree on; PC
  audit records the choice at plan time).
- `--hunt=none` disables the hunt (same as passing no `--hunt`); it
  exists so a scripted option value round-trips.

## Dependencies

- N6 multi-pass planner and pass loop (`planPasses`, `mergePasses`, the
  pass loop in `runner.cpp`, `MetricMethod`).
- N2 JSON writer/reader (`json_report.cpp`, `json_load.cpp`,
  `loadReport`, `docs/schema/report-v1.json`).
- N1 statistics (median, CV) as aggregates; reused and copied nowhere.
- Vendored kernel PMU tables `external/pmu-events` (RECORD
  `linux-7.2.4`, `tools/perf/pmu-events/arch/x86`, skylakex and
  amdzen1 to amdzen6 included), embedded by
  `cmake/EmbedPmuEvents.cmake`; license verdict is spec 007 task T002
  (`specs/007-counters-and-timers/tasks.md`, recorded in its `plan.md`).
- The scripted test seam `FakeProvider` (values, availability, open
  failures).
- Parallel lane contracts: N7's option table move (spec 028), N13a
  (spec 027), the shared-file conflict rules of the prompt (rebase,
  keep every lane's lines, next free version and schema minor, measured
  `PAGE_LINE_CAP`, full gate set, one forced push).

## Citations

Evidence at `bd10d76`, checked by symbol on 2026-10-22:

- `include/speedgun-ng/counters_system.hpp`: `System::object` (241),
  `Object::counters` (109), `Object::counter` (135).
- `include/speedgun-ng/counters_core.hpp`: `Availability` with
  `COUNTABLE`, `PERMISSION_BLOCKED`, `NOT_ENCODABLE`, `ABSENT`,
  `SCOPE_REFUSED`, `GAP` (87 to 95), `CatalogEntry` (246 to 257) with
  `name`, `description`, `unit`, `avail`, `mode`, `targets`,
  `frequencyHz`, `scaled`.
- `include/speedgun-ng/counters_fake.hpp`: `FakeScript` (43),
  `FakeProvider` (103).
- `source/harness/detail/pass_plan.hpp`: `ColumnLeaves` (41),
  `planPasses` (105), `mergePasses` (132).
- `source/counters/linux_pmu/counter_limits.cpp`: the per-device counter
  limit.
- `source/harness/json_report.cpp`: `kSchemaVersion` at `1.1` (31).
- `include/speedgun-ng/benchmark.hpp`: `MetricMethod` (220),
  `loadReport` (2561), the aggregate names `mean`, `median`, `sd`, `cv`,
  `min`, `max`, `n` (1992), the pass spread disclosure fields (535 to
  539).
- `source/harness/cli.cpp`: `kMetricMethodOption` 256 (345),
  `kCompareOptionValue` 257, `kCompareRuleOptionValue` 258,
  `kCompareStrictOptionValue` 259 (69 to 71).
- `external/pmu-events` (`RECORD`, `arch/`), `cmake/EmbedPmuEvents.cmake`.
- `tools/docs/example_drift.py`: `PAGE_LINE_CAP` 773 (155),
  `SECTION_LINE_CAP` 25 (156), `SECTION_LINE_CAPS` (161).
- `tools/dbc/dbc_pair_gate.py`: the contract pairing gate.
- `docs/schema/report-v1.json`, `docs/schema/compare-v1.json`.
- `specs/007-counters-and-timers/tasks.md`: T002 license verdict task.
- Absence checks: no raw-code parse in `sg::counters` (grep for the
  `r<hex>` form answers nothing at `bd10d76`); no
  `test/source/harness_cli_table_test.cpp` on master.
- `CMakeLists.txt`: `VERSION 0.16.0` (9), archive `SOVERSION 3` (53),
  harness `SOVERSION 10` (261).
- `.specify/memory/constitution.md`: I to XI and Pull Request Quality.

## Open questions

- Raw event codes (`r<hex>`): refused in N14a per D-11. Adding them
  changes the counters ABI (the encode path) and would raise the
  archive `SOVERSION` to 4. Needs a later spec.
- gary acceptance timing: AC-10's Zen 5 run waits for a gap in the N7
  lane. Recorded as a schedule risk. No design question waits on it.

## Budget note

- README: the "Hunter mode" section fits `SECTION_LINE_CAP` (25); no
  `SECTION_LINE_CAPS` entry is added. `PAGE_LINE_CAP` is 773; the
  measured README length after the change is recorded at implement
  time, and the cap rises through a recorded decision only if the
  measured length passes it.
- `docs/pages/harness.md` carries the option detail outside the README
  budget.

## Constitution Checklist

- Main-page impact check (IV.1): YES. The feature adds user-visible
  behavior, so FR-070 and FR-071 carry its README section, its runnable
  `example/docs/` owner file, its quick-reference and option rows, and
  its `docs/pages/harness.md` detail in the same change, inside the
  budget this spec's Budget note records.
- Coverage (VI): FR-091 holds 100% line and branch with no exclusion.
- Performance (VII): the hunt runs outside timed columns it does not
  measure; the ranking is pure post-processing.
- CI (VIII): FR-094 and the Constraints list bind the gate set.
- Anti-slop (X): the Decisions section records the settled readings;
  no speculative generality beyond the capabilities list (D-5 fixes the
  scope).
- Prose (XI): this spec and all feature text follow the discourse
  rules; no word is split for a gate.
