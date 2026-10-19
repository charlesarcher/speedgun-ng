# Feature Specification: Metric Formulas

**Feature Branch**: `021-metric-formulas`

**Created**: 2026-10-19

**Status**: Draft

**Input**: User description, opening quoted. The request of 2026-10-19 is
the owner's pasted prompt for roadmap item N2, formerly H12. Its
hard-rules, version-rule and rebase paragraphs restate constitution VIII
and IX plus the three-lane rules, so they are summarized where they bind
this feature and not copied.

> Metric formulas, item N2 (old H12) of the roadmap. Short name:
> metric-formulas. Target machine: corsairone (Intel, one socket). Lane:
> counters (N2, then N6, N10). It runs in parallel with N1 on gary and N4
> on thrawn.
>
> The counters library reads the metric formulas from the vendored perf
> tables (`external/pmu-events`) and computes metrics by name, for example
> instructions per cycle. Today the table parser skips every row with
> `MetricExpr` or `MetricName` (`source/counters/linux_pmu/table_parse.cpp`,
> `addEntry`, R-010, US6). After N2, a user can ask for a metric by its
> perf name, and the catalog listing shows the metrics.
>
> Capabilities: C-1 formula parse, C-2 metric by name, C-3 catalog, C-4
> harness use, C-5 docs.
>
> Recommendations (accepted): an in-house recursive-descent parser for the
> perf `MetricExpr` grammar, operators, `if else`, `min`, `max`, `d_ratio`,
> `#` constants, no new vendored library; an unknown function, an unknown
> event or an unsupported construct makes the metric not available, the
> catalog marks it, a request by name fails with a clear one-line error, a
> command-line request exits with status 2; metrics that reference other
> metrics resolve at load time and a cycle is rejected; `ScaleUnit` applies
> the scale and keeps the unit text; the parser runs over all metric rows
> of every vendored table and the fake provider carries the value tests,
> with the real-PMU check on corsairone; the fold stays after the run and a
> metric reads no counters in the timed path beyond its events.
>
> Out of scope: multi-pass collection when a metric needs more counters
> than the hardware has (N6), the top-down tree (N10), percentiles (N1).

## Clarifications

### Session 2026-10-19

- Q: Does the host table reachability fix stay inside this feature, or
  ship as its own change? → A: Keep it here. FR-034 to FR-036 ship with
  N2, because a metric feature that lists nothing on its own development
  host cannot be demonstrated or accepted on it.
- Q: How does a metric name match a metric of the host? → A: An exact,
  case-sensitive match against the `MetricName` of the selected directory,
  with no fuzzy lookup at request time. When two rows of the selected
  directory carry one name, the first row is kept and the second is refused
  with a reason that names the duplicate, as FR-011 states.
- Q: When does a metric name on the command line that matches nothing
  fail? (FR-028) → A: Before any measurement starts. The harness prints
  one line and exits with status 2, and no benchmark work runs.
- Q: Which `ScaleUnit` spellings does the library convert? → A: A pinned
  conversion table that covers every distinct spelling of the selected
  directory, verified by a sweep of the vendored tree. A spelling outside
  the table is refused at load and names the text.
- Q: Where does the CPU stepping the mapping key needs live? → A: In the
  private identifier the table loader already builds, beside vendor,
  family and model. No public header changes, so the counters archive
  `SOVERSION` stays 2 and the harness `SOVERSION` stays 6.

## Decisions

Charles accepted these as recommendations for this prompt on 2026-10-19.
They are recorded here per X.1 so that no implementer reopens them, and
each one names the reading it settles.

- **D-1 The parser is in-house.** A recursive-descent parser for the
  `MetricExpr` grammar lives inside the counters library. No new vendored
  library and no new runtime dependency (Additional Constraints: zero
  external runtime dependencies).
- **D-2 Unsupported means not available.** A metric the host cannot
  express is a data condition, so it is a recoverable error: the metric is
  listed with a refusal reason, a request by name returns that error, and a
  command-line request prints one line and exits with status 2. No contract
  fires and no process abort.
- **D-3 Cross-metric references resolve at load time.** A metric whose
  expression names another metric of the same table is expanded into one
  expression while the table loads. A reference cycle refuses every metric
  in the cycle. Nothing resolves at fold time.
- **D-4 The grammar is the set the pinned tables use.** The accepted
  constructs are the arithmetic operators, parentheses, decimal,
  hexadecimal and exponent literals, event names with dotted suffixes, the
  `:` privilege suffix and the `unit@modifier@` spelling with its backslash
  escapes, the conditional form `<value> if <test> else <value>`, the
  functions `min`, `max`, `d_ratio` and `has_event`, the helper
  `source_count`, and the `#`-prefixed literal constants. The measured
  inventory below names that set and nothing else. A construct outside it
  makes the metric not available with a reason that names the construct.
  The two spellings the request names, `#slots` and `#smt_on`, are absent
  from the pinned tree, which spells the SMT constant `#SMT_on`.
- **D-5 The unit text travels beside the result.** `MetricResult` keeps its
  four fields. A named metric discloses its unit text beside the folded
  result, so the exported layout of the counters archive stays as it is and
  its `SOVERSION` stays 2.
- **D-6 The page carries this capability inside the existing hardware
  counter metrics section.** The drift gate counts twenty capability
  sections and the main page sits at its line cap. The feature's reader
  fact is one more way to attach a metric, and a reader who looks for it
  looks in that section. The alternative, a twenty-first section, is the
  recommendation's literal reading and stays open if the section cap cannot
  hold the added lines; FR-031 records the measurement that settles it.
- **D-7 The host-selection defect is in this feature.** The mapping-file
  key omits the CPU stepping, so a model-55 part selects no vendored table
  (PC-1). Without that table the metric catalog of the development host is
  empty and C-2, C-3 and C-4 deliver nothing where they are written. The
  fix is the fourth key field that the pinned mapfile's own patterns ask
  for. It ships in this feature with its own tests.
- **D-8 The fold stays after the run.** Metric expressions join the plan
  that the run compiles, and the values come from the recorder's windows.
  The timed loop gains no counter read beyond the events the plan already
  opens.
- **D-9 A literal constant reads a published host fact.** The tree the
  counters library exposes already answers how many packages, cores and
  threads the host carries and whether simultaneous multithreading is on,
  and a catalog entry carries a calibration frequency. A constant with no
  published fact refuses its metric and names the constant. No new host
  probe is added: a fact the library does not publish is a counters
  change of its own, and this feature refuses the metric while it waits.

## Audit point

| Field | Value |
| --- | --- |
| Audit point | `ab32400b86321f98f13b9c66d4488e15831b47f6` |
| Audit-point date | 2026-10-19 11:40:39 -0500 (commit date) |
| Default branch at run time | `master`, tip `ab32400`, equal to the `origin/master` tip |
| Working tree at run time | clean at that commit (`git status --porcelain` printed nothing) |
| Development host | `corsairone`, Intel i9-9920X, CPUID family 6, model 85 (`0x55`), stepping 4 |
| Project version | 0.10.0 (`PROJECT_VERSION` in `CMakeLists.txt`) |
| Counters archive `SOVERSION` | 2 (`speedgun-ng_speedgun-ng` target property) |
| Harness `SOVERSION` | 6 (`speedgun-ng_harness` target property) |

The request binds no spec number, no commit and no source line. The
number resolved at run time: `specs/` held 001 to 020, so this feature is
021. The two sibling features take their own next free number on their own
machine at their own run time.

## Preconditions

Each check read the audit-point tree on corsairone.

### PC-1: the table parser and `PmuTableEntry`. PASS for the parser, PARTIAL for host reachability.

- `source/counters/linux_pmu/table_parse.cpp` defines `addEntry`. Its
  first test returns early for a row that carries
  `MetricExpr` or `MetricName`, and the comment above it names R-010 and
  US6. That skip is the code this feature replaces.
- `source/counters/detail/pmu.hpp` defines `PmuTableEntry` with `name`,
  `description`, `unit` and a `fields` vector of name and value pairs. It
  carries no metric field, and the header comment states that metric rows
  ship as catalog data for a future release.
- `source/counters/linux_pmu/embedded_tables.hpp` compiles the vendored
  tree into the library as one buffer per directory with an index over it.
  No run-time path lookup reaches the tables.
- Defect, new at this audit point: `mapfileKey` in `table_parse.cpp`
  builds the mapping-file key as `<vendor>-<family>-<MODEL>`, and
  `pmuSelectDirectory` applies the mapfile's first column as a full match
  against that key. The pinned
  `arch/x86/mapfile.csv` splits model `0x55` by stepping, through the rows
  `GenuineIntel-6-55-[01234],v1.37,skylakex,core` and
  `GenuineIntel-6-55-[56789ABCDEF],v1.25,cascadelakex,core`. A key without
  a stepping field matches neither row, and no other row of the 46 matches
  it. Verified by replaying all 46 rows against the key
  `GenuineIntel-6-55`: zero matches, so the selection is empty. Upstream
  added the stepping to the CPUID string in 2018 for exactly this
  split (Citations). `PmuIdent` in `source/counters/detail/pmu.hpp` carries
  vendor, family and
  model and no stepping, so the field the key needs does not exist.
  No file under `specs/` names the stepping.
- Consequence for this host: corsairone is family 6, model 85, stepping 4,
  so upstream selects `skylakex` and this library selects nothing. Every
  metric row this feature can list on corsairone depends on that fix.

### PC-2: the expression API. PASS.

- `detail::ExprCore` in
  `include/speedgun-ng/counters_measurement.hpp` is
  a leaf vector and a node vector, node kinds 0 leaf, 1 add, 2 subtract,
  3 divide, 4 scale, `scaleAll` for a scalar factor, and
  `splice` for composing spines.
- `Expression<D>`, the typed wrapper, lives in the same header. It is
  built today from resolved
  `Counter<C>` leaves and composed with `+`, `-`, `/` and scalar
  multiplication under the dimension rules; `fold`, `foldPairs` and the
  raw view then read the recorder.
- `detail::foldPairsCore`, `detail::metricCore` and `compileCore` live in
  that header too: the first two fold, and the third compiles spines into
  a plan.
- `MetricResult` in `include/speedgun-ng/counters_core.hpp` holds
  `value`, `runningRatio`, `availability` and `scaled`. It carries no unit
  field, so D-5 keeps it that way.
- The closed
  `Availability` enumeration in the same header holds `COUNTABLE`, `PERMISSION_BLOCKED`,
  `NOT_ENCODABLE`, `ABSENT`, `SCOPE_REFUSED`, `GAP`. The refusal reason a
  formula can carry is new disclosure beside it, and this spec asks for a
  reason that names the refusing construct (FR-007).

### PC-3: the catalog listing. PASS.

- `printObject` in `source/harness/catalog.cpp` prints one line per
  catalog counter under
  an object path, with `description`, `unit`, `mode`, `avail` and
  `refusal` fields, and recurses over children. `printCatalog` is the
  entry point the `--catalog` option reaches in `source/harness/cli.cpp`.
- `source/harness/cli.cpp` holds the long-option table: `filter`,
  `list`, `repetitions`, `min-time`, `iterations`, `warmup-time`,
  `dry-run`, `counter`, `catalog`. An unknown option prints one line and
  the scanner returns 2.
- The option rows the drift gate checks sit in the option table of the
  command-line section of `README.md`.

### PC-4: how a metric reaches a report column, and the counters-only rule. PASS.

- The metric-column builder in `source/harness/runner.cpp` builds the
  metric columns of one
  repetition, first from the family's attached metrics, then from the
  `--counter` addresses. An address resolves through the catalog;
  a leaf the catalog lacks marks the column unavailable,
  prints the `no such leaf in the catalog (FR-023)` line, and the run
  continues. That is the column path a named metric joins (C-4), and the
  exit-2 rule applies where a name matches no metric at all (FR-028).
- `README.md` carries the hardware counter metrics section under the title
  `Hardware counter metrics`. Its snippet
  builds `*retired / *cycles` and calls `BenchmarkHandle::addMetric`, the
  library-side path C-2 feeds.
- The counters-only constraint of Additional Constraints binds every C++
  source and header under `source/`, `include/`, `example/`, `test/` and
  `tools/`, and exempts `source/counters/`, `include/speedgun-ng/counters*.hpp`
  and the `counters_` tests and gate scripts under `test/`. The parser
  lives inside the exemption; harness-side use stays on `sg::counters`.
  `test/time_source_gate.sh` enforces it.
- Which `SOVERSION` this feature can move: the counters archive (2). The
  harness target is the one at stake only if a harness-side signature
  changes shape; an added command-line option changes no exported layout.

### PC-5: the documentation budget. PASS, and the page has no headroom.

- `README.md` is 638 lines and `PAGE_LINE_CAP` is 638
  (`tools/docs/example_drift.py`), so any added line needs a recorded
  raise. `SECTION_LINE_CAP` is 25, and `SECTION_LINE_CAPS` holds two
  recorded exceptions.
- `SECTIONS` and `SECTION_OWNERS` in that file hold the twenty
  capability titles and their owner files. The hardware counter metrics
  section is owned by `counter_metrics.cpp`, which exists under
  `example/docs/` with the region `#metrics`.
- `USER_GUIDE_PAGES` names `docs/pages/building.md` and
  `docs/pages/harness.md`. `docs/pages/harness.md` is the page that holds
  harness detail, and the counters detail a reader of this feature needs
  belongs there.
- `example/CMakeLists.txt` shows the `run_docs_<name>` target form the
  drift and run gates require.

## Roadmap position

The roadmap file lives outside this repository in the owner's notes and is
absent from this machine, so the request of 2026-10-19 is the authority for
the numbering below.

- This feature is N2 on the counters lane. N6 (multi-pass collection) and
  N10 (the top-down metric tree) follow it on the same lane.
- N1 (percentiles) and N4 (topology placement) start on two other machines
  at the same time. N1 and N4 are harness features and share no source
  file with this one.
- N12 (NUMA memory-node placement) sits on the harness lane behind N4 and
  touches nothing here.

## Vendored table inventory read

Measured over the pinned `external/pmu-events/arch/x86` tree at the audit
point. These numbers set the grammar (D-4) and the parse test (FR-038).

| Fact | Measured |
| --- | --- |
| Architecture directories | 46 |
| Rows carrying `MetricExpr` | 5958 |
| Rows carrying `MetricName` | 5894 |
| Rows carrying `MetricGroup` | 5138 |
| Rows carrying `ScaleUnit` | 3222 |
| Rows whose expression names a metric of the same directory | 2973 |
| Rows with a conditional `<value> if <test> else <value>` form | 267 |
| Rows with a `#`-prefixed literal constant | 275 |
| Rows whose expression carries a backslash escape inside a `@modifier@` | 707 |
| Rows whose expression carries a `:` privilege suffix on an event | 88 |
| Comparison operators inside conditionals | `<` 38, `>` 127, and `<=`, `>=`, `==`, `!=` 0 |
| Function calls inside expressions | `max` 250, `min` 190, `d_ratio` 74, `has_event` 35, `source_count` 31 |
| Literal constants present, by occurrences | `#SMT_on` 148, `#num_packages` 65, `#has_pmem` 33, `#num_dies` 23, `#num_cpus_online` 23, `#core_wide` 12, `#num_cores` 12, `#SYSTEM_TSC_FREQ` 11 |
| Occurrences of `#slots` or the lowercase spelling `#smt_on` | 0 |
| `ScaleUnit` spellings, most frequent | `100%` (2579), `1per_instr` (174), `1MB/s` (163), `1e3per_1k_instr` (58), `64Bytes` (54) |
| Metric rows in the directory the fix of D-7 selects for corsairone (`skylakex`) | 279 |
| `skylakex` detail | 156 rows carry `ScaleUnit`, 19 carry a literal constant, 19 carry a conditional or `has_event`, 38 carry a backslash escape, `d_ratio` appears 0 times |
| `skylakex` examples this spec quotes | `cpi` is `CPU_CLK_UNHALTED.THREAD / INST_RETIRED.ANY` with `ScaleUnit` `1per_instr`; `l2_mpi` is `L2_LINES_IN.ALL / INST_RETIRED.ANY`; `tma_info_thread_ipc` is `INST_RETIRED.ANY / tma_info_thread_clks`, and `tma_info_thread_clks` is `CPU_CLK_UNHALTED.THREAD`; `C6_Pkg_Residency` is `cstate_pkg@c6\-residency@ / msr@tsc@` with `ScaleUnit` `100%` and group `Power` |

Three readings follow. A metric reference is the common case, so
load-time expansion and cycle refusal carry real work (D-3). The grammar
of D-4 covers every construct the pinned tree uses, so an unsupported
construct is the rare verdict and not the common one. A literal constant
reaches 275 rows, so D-9 carries real work as well.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Ask for a metric by its perf name (Priority: P1)

A developer wants instructions per cycle for one benchmark. Today they
write the division by hand: two counter lookups, one `/`, one `addMetric`
call, and they must know that `instructions` and `cpu-cycles` are the two
events the ratio needs. After this feature they ask for the metric by the
name the host's table carries, `tma_info_thread_ipc` on the `skylakex`
table, and the library hands back the expression that table describes.

**Why this priority**: this is the feature. Everything else is a way of
reaching it.

**Independent Test**: on a tree where the metric rows are readable, ask for
one named metric of the host's table, fold it over a recorder filled by the
fake provider, and compare the value with the hand-computed ratio of the
same fake counts. The story is complete with no catalog line and no command
line.

**Acceptance Scenarios**:

1. **Given** a host whose table carries `cpi` and a fake provider whose
   counts give 2 cycles per instruction, **When** a caller asks for `cpi`
   and folds the result over the recorder, **Then** the value is 2, the
   availability is `COUNTABLE`, and the unit text is the `1per_instr`
   spelling the table gives.
2. **Given** a name no metric of the host carries, **When** a caller asks
   for it, **Then** the request returns a recoverable error whose message
   is one line and names the near-miss metric names it came closest to.
3. **Given** a metric whose expression carries a `ScaleUnit` of `100%`,
   **When** a caller folds it over fake counts, **Then** the value carries
   the `100` factor and the unit text is the `100%` spelling the table
   gives.
4. **Given** `tma_info_thread_ipc`, whose expression names
   `tma_info_thread_clks`, **When** a caller asks for
   `tma_info_thread_ipc`, **Then** the returned expression carries the
   inner metric's event and one fold produces the value.
5. **Given** a metric whose expression tests `#SMT_on`, **When** the fake
   host publishes a thread count above its core count, **Then** the
   conditional selects the branch its table text gives for SMT on, and a
   fake host that publishes equal counts selects the other branch.

---

### User Story 2 - See what the host can measure (Priority: P2)

A developer runs `--catalog` to learn what a machine offers. Today the
listing shows countable leaves and nothing else. After this feature the
same listing shows the host's metrics, each with its group, its unit text,
and either a countable state or a refusal reason that says why the metric
is not there.

**Why this priority**: discovery is what makes C-2 usable by someone who
did not write the feature, and the refusal reasons are the audit trail for
D-2.

**Independent Test**: run the listing on the development host and check
that every metric row of the selected table appears exactly once, that each
line carries the group and the unit text the table gives, and that a metric
the host cannot express carries a reason.

**Acceptance Scenarios**:

1. **Given** the pinned table for the host, **When** the listing runs,
   **Then** the counter lines keep their current shape and one line per
   metric follows them.
2. **Given** a metric that references an event the host cannot encode,
   **When** the listing runs, **Then** that metric is listed with the
   refusal kind its constituent carries.
3. **Given** a metric refused at load for an unsupported construct,
   **When** the listing runs, **Then** the line names the construct.
4. **Given** a host with no selected table, **When** the listing runs,
   **Then** the listing prints no metric line and no error, and the run
   that asks for a metric by name reports that the host carries no metric
   table.

---

### User Story 3 - Put a metric in the report from the command line (Priority: P2)

A developer runs an existing benchmark binary and wants one metric column
without editing code. They name the metric on the command line beside
`--counter`, and the report gains one column labelled with the metric name.

**Why this priority**: it turns the library capability into something the
owner can use on any benchmark in the tree, and it is the surface the
rebase notes flag as shared with N1.

**Independent Test**: run a registered benchmark on the development host
with one metric that exists named and one name that matches no metric
named, and check the column, the refusal column, and the one-line status-2
failure.

**Acceptance Scenarios**:

1. **Given** a benchmark and a metric the host can count, **When** the run
   names that metric, **Then** the row carries the metric label with the
   `ratio=`, `scaled=` and `gap=` fields the metric columns already print.
2. **Given** a metric that exists and cannot be counted on this host,
   **When** the run names it, **Then** the column prints the refusal kind,
   the run continues, and the exit status is the run's own.
3. **Given** a name that matches no metric of the host, **When** the run
   names it, **Then** the harness prints one line and exits with status 2.

---

### User Story 4 - Read the page and copy a working block (Priority: P3)

A reader of the main page wants the shortest path to a metric they did not
compose. The page section on hardware counter metrics shows a block that
asks for a metric by name, and the block compiles as printed because CI
builds and runs it.

**Why this priority**: constitution IV.1 makes the page part of the
feature, and the drift gate is what keeps the claim true.

**Independent Test**: check out the tree, copy the block over the canonical
includes, build it, run it, and read the printed row. The run gate does the
same in CI.

**Acceptance Scenarios**:

1. **Given** the page section, **When** the drift gate compares it with the
   example region, **Then** the two match and the run gate prints the row
   the result sentence names.
2. **Given** the quick-reference table, **When** the drift gate checks its
   rows against the public headers, **Then** every new row resolves.
3. **Given** the main page after the section grows, **When** the gate
   checks the page budget, **Then** the recorded `PAGE_LINE_CAP` covers the
   measured page.

### Edge Cases

- A metric expression with unbalanced parentheses, a trailing operator, an
  empty expression, or two operators in a row: the metric is refused at
  load with a reason naming the construct, and the rest of the table loads.
- A metric expression whose only event is absent on the host: the metric
  stays listed and carries `ABSENT` with its reason.
- A literal constant the library publishes no fact for: the metric is
  refused and names the constant, and the value is never guessed at.
- A `has_event` test for an event the host cannot count: the conditional
  selects its `else` value, and a metric whose chosen branch counts nothing
  carries that branch's availability.
- A conditional whose test compares two metrics that are both refused: the
  outer metric is refused with the inner refusal named.
- A `source_count` helper over an event name no table source defines: the
  helper answers zero, and the expression keeps evaluating.
- A metric that references a metric which is itself refused: the outer
  metric is refused with a reason that names the refused inner name.
- A reference cycle of one metric that names itself, and a cycle of two or
  more: every metric in the cycle is refused and names the cycle.
- A `ScaleUnit` text the library has no scale for: the metric is refused
  with the text quoted, and the value is not silently reported unscaled.
- A metric whose terms do not share one dimension under the existing typed
  algebra: refused at load, because the existing composition cannot build
  it.
- A duplicate `MetricName` inside one directory: the first row wins and the
  second is refused with a reason that names the duplicate.
- Two host CPUs, same model, different steppings: they select two
  directories and two metric sets (the D-7 fix is what makes this true).
- A metric name that is also an event name: the metric listing keeps both
  lines, and a request by name follows the metric reading and says so in
  the disclosure it prints.
- A metric named twice on the command line: one column, and the second
  occurrence is not an error.
- A metric whose events exceed what the hardware can open at once: the
  existing plan-compiler refusal reaches the caller as it does today.
  Splitting the metric across passes is N6.

## Requirements *(mandatory)*

### Formula parse (C-1)

- **FR-001**: When the counters library loads a vendored architecture
  table, it shall read each row that carries a metric name and a metric
  expression as a metric definition, keeping the name, the group text, the
  scale-and-unit text and the description with their table spellings.
- **FR-002**: The metric definition store shall live beside the event table
  of the same directory, loaded once per directory on first use, with no
  run-time path lookup.
- **FR-003**: When a metric expression is read, the parser shall accept
  this grammar: decimal, hexadecimal and exponent numeric literals; event
  names with dotted suffixes such as `INST_RETIRED.ANY`; the `:` privilege
  suffix such as `INST_RETIRED.ANY:u`; event names in the `unit@modifier@`
  spelling, including the backslash escapes the tables carry inside a
  modifier; the operators `+`, `-`, `*`, `/`; parentheses; and unary minus.
- **FR-004**: The parser shall accept the four functions the pinned tables
  use: `min`, `max`, `d_ratio` and `has_event`, and the helper
  `source_count`.
- **FR-005**: The parser shall accept the conditional form
  `<value> if <test> else <value>`, where a test compares two values with
  `<` or `>`, the two comparisons the pinned tree uses, or asks a
  `has_event` or `source_count` helper.
- **FR-006**: When an expression carries a `#`-prefixed literal constant,
  the load shall resolve it from a host fact the counters library already
  publishes: the object tree it selects by kind gives the package, core and
  thread counts and the simultaneous-multithreading state, and a catalog
  entry's calibration frequency gives the time-stamp-counter frequency. A
  constant with no such source refuses the metric and names the constant.
- **FR-007**: When an expression contains a construct outside the grammar
  of FR-003 to FR-005, the metric shall be recorded as not available with a
  reason that names the construct, and the load shall continue with the
  remaining rows of the table.
- **FR-008**: When a metric expression names a metric of the same table,
  the load shall expand that reference into the referring expression before
  the metric becomes visible to a caller.
- **FR-009**: When metric references form a cycle, the load shall refuse
  every metric in the cycle with a reason that names the cycle, and every
  metric outside the cycle shall stay available.
- **FR-010**: When a metric's terms cannot compose into one expression
  under the dimension rules the existing typed composition enforces, the
  metric shall be refused at load with a reason that names the operands.
- **FR-011**: When a directory carries two rows with one metric name, the
  load shall keep the first row and refuse the second with a reason that
  names the duplicate name.
- **FR-012**: A `has_event` test shall be settled from what the host can
  count, and a metric whose conditional selects a branch the host cannot
  count shall carry the availability state that branch's leaves disclose.
- **FR-013**: A metric definition shall never become a countable event: a
  metric name shall never encode as an event, and the existing refusal that
  keeps metric rows out of the countable catalog (R-010, US6) shall stay in
  force for the event path.
- **FR-014**: The parser shall add no runtime dependency to the project.

### Metric by name (C-2)

- **FR-015**: A caller shall be able to ask the counters library for one
  metric of the host by name and receive an expression that a scope or a
  recorder folds into a `MetricResult`, together with the group text and
  the unit text of that metric. The match is exact and case-sensitive
  against the `MetricName` of the selected directory.
- **FR-016**: When no metric of the host carries the requested name, the
  request shall return a recoverable error whose message is one line and
  whose diagnostics carry the near-miss names, on the pattern the object
  lookup already follows. When two rows of the selected directory carry one
  `MetricName`, the load keeps the first row and refuses the second with a
  reason that names the duplicate, and a request for that name reaches the
  kept row.
- **FR-017**: When the host selects no vendored table, a metric request
  shall return a recoverable error that says the host carries no metric
  table.
- **FR-018**: When a metric carries a scale-and-unit text, the value the
  fold publishes shall carry the scale applied, and the unit text shall
  reach the caller beside the result with `MetricResult` unchanged. The
  scales shall come from one pinned conversion table that covers every
  distinct spelling of the selected directory, and a spelling the table
  lacks shall refuse the metric at load and name the text.
- **FR-019**: A metric request shall read no counters and shall perform no
  per-read parsing: the first request for a metric of a directory performs
  that directory's load, and each later request reuses it.
- **FR-020**: When a metric is not available, the disclosure a caller reads
  shall carry the refusal kind and the reason recorded at load.

### Catalog listing (C-3)

- **FR-021**: When the catalog option runs, the listing shall print one
  line per metric of the host, naming the object path, the metric name, the
  group text, the unit text, the availability state and the refusal reason.
- **FR-022**: The counter lines of the listing shall keep their current
  spelling, order and content.
- **FR-023**: A metric that is not available shall stay in the listing with
  its reason, so that the listing accounts for every metric row of the
  host's table.
- **FR-024**: The listing shall print no metric line for a host with no
  selected table and shall report no error for that case.

### Harness use (C-4)

- **FR-025**: A benchmark shall be able to attach a named host metric as a
  report column through the metric path the harness publishes today, so
  that the row carries the column label with the `ratio=`, `scaled=` and
  `gap=` fields.
- **FR-026**: When the command line names a metric, the run shall add one
  report column for it through the column path that the counter option
  uses, and the same metric named twice shall give one column.
- **FR-027**: When a named metric exists on the host and cannot be counted,
  the column shall print the refusal kind in place of the number and the
  run shall continue.
- **FR-028**: When a name on the command line matches no metric of the
  host, the harness shall print one line and exit with status 2 before any
  measurement starts, so that no benchmark work runs for a name that
  cannot resolve. Columns shall appear in the order the command line names
  them.
- **FR-029**: A metric column's value shall come from the counters library
  through the plan and the recorder, so that the time-source gate stays
  green and the timed loop performs no counter read beyond the events the
  plan opens.

### Documentation (C-5)

- **FR-030**: The main page shall teach this capability in the section the
  drift gate recognizes, with one purpose sentence, one marked snippet and
  one result sentence, and the snippet shall come from an example that CI
  builds and runs through its own `run_docs_` target and the run gate.
- **FR-031**: The section placement and its size shall be measured and
  recorded in this spec's budget note: the section title set, the owner
  file, the section line count against `SECTION_LINE_CAP`, the page line
  count against `PAGE_LINE_CAP`, and the recorded raise.
- **FR-032**: The quick-reference table shall carry a row for each public
  name this feature adds, and the drift gate's option check shall pass with
  the new command-line option in the option table.
- **FR-033**: The detail a reader needs beyond the page shall live in a
  page under `docs/pages/`, in `docs/pages/harness.md` when no other page
  holds the matching subject.

### Host table reachability (C-6)

- **FR-034**: When the mapping file is consulted, the key shall carry the
  vendor, the family, the model and the CPU stepping in the form the
  patterns of the pinned mapfile match, so that two CPUs of one model and
  different steppings select two directories. The stepping shall be
  carried in the private identifier the loader builds: no public header
  changes, the counters archive `SOVERSION` stays 2 and the harness
  `SOVERSION` stays 6.
- **FR-035**: The selection shall be verified by fixtures for a stepping
  below the model-55 boundary and a stepping at or above it, and both
  fixtures shall select the directory their row names.
- **FR-036**: Every mapfile row of the pinned tree that carries a stepping
  class shall be matched by at least one fixture key, so that a re-pin that
  adds a stepping class shows a failing test.

### Version, tests, and discipline

- **FR-037**: This feature shall ship as the next free minor version at
  merge time, 0.11.0 when it merges first. It shall leave both `SOVERSION`
  numbers unchanged unless a change to an exported layout makes the 0.x
  rule in `CMakeLists.txt` move one, and any move names the layout that
  moved.
- **FR-038**: Every metric row of every vendored table shall reach a
  verdict in a unit test: an expression or a refusal with a reason. A row
  shall never be dropped without a verdict.
- **FR-039**: Value tests shall use the fake provider. No unit test shall
  open a real PMU. The real-PMU check shall run on corsairone and be
  recorded with the run that shows it.
- **FR-040**: New code shall hold 100% line and 100% branch coverage with
  no exclusion marker, no waiver and no partial row. A remaining gap shall
  stop the work and be reported with `gcov -b` evidence.
- **FR-041**: No test shall sleep or read a clock. Synchronization in tests
  shall use the existing fake provider and barriers.
- **FR-042**: An invalid option value on the command line shall print one
  line and exit with status 2, matching the existing scanner behavior.

## Key Entities

- **Metric definition**: one row of a vendored table. Name, group text,
  scale-and-unit text, description, the parsed expression, and a verdict:
  available or refused with a reason.
- **Refusal reason**: the machine-visible statement of why a metric is not
  available. It names the construct, the missing event, the duplicate name,
  the cycle or the missing table. It pairs with the `Availability` state a
  constituent carries.
- **Metric reference**: a metric name inside another metric's expression.
  It exists during load and disappears at expansion.
- **Named-metric result**: a folded `MetricResult` beside the group text
  and the unit text of the metric that produced it.
- **Metric column**: one report column a named metric produces, on the
  existing column entity.
- **Mapping key**: the vendor, family, model and stepping string the
  selector matches table rows against. C-6 adds the fourth field.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-01**: On the pinned tree, each of the 5958 metric rows reaches one
  verdict, expression or refusal with a reason, in a single unit-test run,
  and no row is silently dropped.
- **SC-02**: On corsairone, after C-6 lands, the catalog listing shows one
  line per metric of the `skylakex` table, 279 rows measured at the audit
  point, and the counter lines of the listing are unchanged.
- **SC-03**: A developer names one metric on the command line of an
  existing benchmark binary and reads that metric's value in the report row
  without editing any file.
- **SC-04**: A metric read costs the timed loop nothing: the count of
  provider reads per iteration in the no-allocation and overhead checks
  stays at the value measured before this feature.
- **SC-05**: The per-directory metric load happens once: a test that asks
  for three metrics of one directory shows one parse pass over that
  directory, measured by the fixture's own call counter.
- **SC-06**: On corsairone, one recorded run shows five metrics of the
  selected table as `COUNTABLE` with a finite value, and `cpi` and `l2_mpi`
  are among them because their table text carries no literal constant and
  no conditional. `cpi` agrees with the hand-written ratio of the
  main-page example within 1% on the same run.
- **SC-07**: Every command-line failure this feature adds shows one line
  and exit status 2 in a scripted check.
- **SC-08**: The new code holds 100% line and 100% branch coverage with no
  exclusion, and the whole gate set of Principle VIII is green on the
  merged state.
- **SC-09**: The project gains zero new runtime dependencies and zero new
  vendored trees.

## Constraints

- Constitution I, II, VI, VIII, X and XI bind in full. 100% line, branch
  and DBC coverage, no exclusion marker, no partial or open row.
- The counters-only rule of Additional Constraints binds the harness side;
  the parser sits inside the counters exemption with the rest of the table
  code.
- Zero external runtime dependencies stays true (D-1).
- The vendored tree is data, and it stays read-only in this feature: no
  file under `external/` is edited, and a re-pin is its own change through
  `tools/pmu_events/update_pmu_events.py`.
- Contract discipline: an unsupported formula is a recoverable error. A
  contract guards only the caller-side preconditions the existing
  expression API already states.
- `ctest -j 2`, and the coverage preset keeps `-fprofile-update=atomic`
  and stays without `-DGLIBCXX_ASSERTIONS`.
- Rebase onto `master`, never merge, and push with `--force-with-lease`.
  The shared files this feature may touch: `README.md`, the quick-reference
  and option rows, `PAGE_LINE_CAP` and possibly `SECTION_LINE_CAPS` and
  `SECTIONS` in `tools/docs/example_drift.py`, `example/CMakeLists.txt` and
  the example run list, `test/CMakeLists.txt`, and `VERSION` in
  `CMakeLists.txt`. Any other file outside the feature needs its reason
  recorded in the plan first.

## Scope

In: FR-001 to FR-042. The metric rows of the vendored x86 tables, the
grammar of D-4 with its conditionals, helpers and literal constants,
load-time expansion with cycle refusal, the by-name library request, the
metric lines of the catalog listing, the command-line metric column on the
existing column path, the mapping-key fix of D-7, the page section with its
runnable example, and the version effect.

Out, each with its owner:

- Multi-pass collection when a metric needs more counters than the host can
  open at once: N6.
- The top-down metric tree and its levels: N10.
- Percentiles in the report: N1.
- Metric formulas for architectures other than x86, and the arm tables: a
  later specification that names them.
- The `#slots` constant and any construct the pinned tree does not carry:
  a re-pin that introduces one refuses the rows that use it until a later
  specification widens the grammar.
- A new host fact the counters library does not publish, for a constant
  the object tree and the catalog cannot answer: D-9 refuses the metric,
  and the fact is a counters change of its own.
- A user-supplied metric definition file, metric name aliases, and metric
  expression overrides: no requirement asked for them.
- NUMA memory-node placement: N12, a harness feature.

## Assumptions

- The request of 2026-10-19 is the owner's pasted text and is the authority
  for scope and numbering. The roadmap file it names sits outside this
  repository and is absent from this machine.
- corsairone is the development and real-PMU host for this feature, as the
  request states. This session ran on it and read CPUID family 6, model 85,
  stepping 4 from `/proc/cpuinfo`.
- The pinned `external/pmu-events` tree stays as it is through this
  feature, so the inventory above is the set the parser must satisfy.
- Metric names in this spec, `cpi`, `l2_mpi` and `tma_info_thread_ipc`
  among them, are the `skylakex` names measured at the audit point, and
  they stand as examples. The names each machine's own table carries are
  the set that matters, and the implementation reads them from the tables.
- A metric is host data: no metric is promised on a machine whose table
  does not carry it, and no test asserts a metric count across machines.
- The existing recorder, plan compiler and column machinery carry the
  measurement work; this feature adds a source of expressions and a way to
  name them.

## Budget note

Measured at the audit point, before any section is written:

- Page: `README.md` 638 lines, `PAGE_LINE_CAP` 638. The raise this feature
  needs equals the measured size of its added lines, recorded here when the
  section lands and requested through the same change, on the pattern
  specs/020 records.
- Section: the hardware counter metrics section as it stands is 20 lines
  including its heading, snippet and result sentence, inside
  `SECTION_LINE_CAP` 25. D-6 places the feature there. The two lines of
  prose and one snippet this feature adds exceed 25, so the change either
  trims that section's existing prose to hold the new lines under the cap
  or takes a recorded `SECTION_LINE_CAPS` entry for it, and FR-031 records
  which of the two the measurement shows.
- Example: one new region in `example/docs/counter_metrics.cpp`, the owner
  file the gate already names for that section, so `SECTION_OWNERS` and
  `SECTIONS` stay untouched.
- Quick reference: one row per public name added. Option table: one row.

## Dependencies

- specs/007-counters-and-timers: the table parser, the seam, the
  countable-catalog rule R-010 and US6 that FR-009 preserves.
- specs/012-counters-defect-resolution: the embedded tables (FR-036, US5)
  that FR-002 loads through.
- specs/015-benchmark-harness-core: the command line, the option table, the
  status-2 usage rule, and the counters-only rule with its gate.
- specs/019-harness-user-counters-and-statistics: the metric column path
  FR-021 and FR-022 join.
- specs/018-docs-main-page: the drift and run gates FR-026 to FR-028 clear.
- specs/020-execution-contexts-core: the current version and `SOVERSION`
  state this feature builds on.

## Citations

- `source/counters/linux_pmu/table_parse.cpp`: `addEntry`, its
  `MetricExpr` and `MetricName` refusal, `mapfileKey`, and
  `pmuSelectDirectory` with its pattern build and full match.
- `source/counters/detail/pmu.hpp`: `PmuTableEntry`, `PmuIdent`.
- `source/counters/linux_pmu/embedded_tables.hpp`: the embedded registry.
- `external/pmu-events/arch/x86/mapfile.csv`: the two model-55 rows and the
  46 rows replayed against the key `GenuineIntel-6-55`.
- Upstream: `tools/perf/pmu-events/arch/x86/mapfile.csv`, and the 2018
  change "perf vendor events: Add stepping in CPUID string for x86"
  (https://lore.kernel.org/linux-perf-users/20181122033611.15890-25-acme@kernel.org/),
  which splits model 55 at stepping 4 and 5.
- `include/speedgun-ng/counters_core.hpp`: `Availability`, `MetricResult`.
- `include/speedgun-ng/counters_measurement.hpp`: `ExprCore`,
  `foldPairsCore`, `metricCore`, `compileCore`, `Expression`.
- `source/harness/catalog.cpp`: `printObject` and `printCatalog`.
- `source/harness/cli.cpp`: the option table, the status-2 path, the
  catalog branch.
- `source/harness/runner.cpp`: the metric columns, the counter address
  path, the missing-leaf line.
- `README.md`: the hardware counter metrics section, the quick reference,
  the option table with the `--counter` and `--catalog` rows.
- `tools/docs/example_drift.py`: `SECTIONS`, `SECTION_OWNERS`,
  `PAGE_LINE_CAP`, `SECTION_LINE_CAP`, `SECTION_LINE_CAPS`,
  `USER_GUIDE_PAGES`.
- `CMakeLists.txt`: `PROJECT_VERSION` 0.10.0, the 0.x rule stated beside
  the archive `SOVERSION` 2, and the harness `SOVERSION` 6.
- `.specify/memory/constitution.md` II, VI, VIII, X, XI, Additional
  Constraints (counters-only, dependencies), Refactoring and Evolution
  (version and `SOVERSION`).

## Open questions

None open. The request's open points are settled by D-1 to D-8, and PC-1
records the one defect the feature must clear before it can show a metric
on its development host.

## Constitution Checklist

- Main-page impact check (IV.1): YES. The feature changes user-visible
  behavior, so FR-030 to FR-033 carry its main-page section, its runnable
  `example/docs/` region, its quick-reference and option rows, and its
  `docs/pages/` detail in the same change, inside the budget FR-027
  records.
