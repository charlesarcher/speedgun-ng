# Feature Specification: Vendor-neutral Top-down Analysis

**Feature Branch**: `042-topdown`

**Created**: 2026-11-01

**Status**: Draft

**Input**: User description: the owner's request for roadmap item N10,
vendor-neutral top-down analysis. Condensed below; the request is the
authority for scope, numbering and the accepted answers.

> Add a top-down layer on the three parts already in the tree: the metric
> rows of the vendored kernel tables that N2 publishes by name (022), the
> pass planner of N6 (024) and the JSON report of N3 (023). The layer adds
> no counter path and no formula grammar. Every node is one N2 formula.
> Select the top-down nodes of the running CPU from the table groups, give
> level 1 vendor-neutral names, keep the vendor name for every deeper node
> behind a vendor prefix, print the tree on the console, write it to JSON
> with its table source, and answer one clear unsupported message where the
> CPU has no source. Intel Skylake-X is the acceptance host; AMD Zen 5 runs
> when the other lane has a gap. Arm and Intel hybrid hosts exist nowhere,
> and no host exists for Intel parts that measure through `PERF_METRICS`.
> Never invent a node: a node comes from a table or it does not exist.

## Decisions

The owner's request carries seven open questions with recommended answers.
Each one is taken as the decision below, and each one names the reading it
settles. The remaining entries settle rulings the request lists as in
force. No implementer reopens them.

- **D-1 Selection order.** Selection runs per model first, then per
  architecture at level 1, then unsupported. A model whose table carries a
  top-down group gets that table: `TopdownL1` to `TopdownL6` on Intel and
  `PipelineL1` and `PipelineL2` on AMD name the same thing in two spellings,
  and the selection treats one predicate over both. A model whose architecture
  has level-1 rows only gets level 1. Anything else answers unsupported with a
  reason.
- **D-2 Names.** Level 1 carries the vendor-neutral names
  `topdown.frontend_bound`, `topdown.bad_speculation`,
  `topdown.backend_bound` and `topdown.retiring`. Every deeper node keeps
  the vendor's own name behind a vendor prefix:
  `topdown.intel.tma_l1_bound`, `topdown.amd.backend_bound_by_memory`. The
  vendor name of every node stands beside its neutral name in the tree and
  in the report.
- **D-3 The fifth level-1 value.** AMD publishes `topdown.smt_contention`
  as a fifth level-1 value on AMD hosts alone. The console block and the
  report both state that the four shared values add to less than 100 percent
  without it.
- **D-4 Public surface (Q-1).** The counters library gains the small public
  header `include/speedgun-ng/counters_topdown.hpp` with the node type and
  one selection entry point. That header is a public addition, so the
  archive `SOVERSION` rises by one, from 4 to 5. The harness gains one member
  in the exported record `sg::BenchmarkResult`, so its exported layout moves
  and the harness `SOVERSION` rises from 19 to 20. specs/037 made the same move
  from 18 to 19 for the same kind of addition (`CMakeLists.txt:369-374`), and
  each rise carries the comment that names the record and the requirement.
- **D-5 One option (Q-2).** The harness gains `--topdown[=<level>]` at
  internal value 350, the first value of the block 350 upward. Levels run 1
  to 6 and the default is 1. A request deeper than the selected table holds
  prints the deepest level the table holds and says so on the same block.
  No short option is added. One mechanism stays open for the plan: the command
  line scan collapses a bare optional value to the seed text `0`
  (`source/harness/options.cpp:1001-1006`), so the option word alone and the
  attached `=0` reach the parse arm as one text, and FR-019 with FR-050 needs
  two answers. The plan names the change that separates them and keeps the four
  optional rows that exist today answering as they do now.
- **D-6 Host constants (Q-3).** Every Intel level-1 row reaches the constant
  `#core_wide` through the table's own slot rows, measured at the audit
  point, so this feature adds a reader for `#core_wide` with a seam test. No
  top-down row reaches `#num_dies`, `#SYSTEM_TSC_FREQ` or `#has_pmem`, so
  those three keep their present refusal. Every node the host cannot resolve
  stays visible: the report lists it with its reason.
- **D-7 Arm (Q-4).** No Arm table joins the tree in this feature. An Arm
  identity answers unsupported and quotes its own MIDR. An Arm source needs a
  new license verdict, a new `RECORD` line and a host, and it belongs to a
  later item.
- **D-8 Intel parts measured through `PERF_METRICS` (Q-5).** A per-model
  table that expresses top-down as formulas is enough; this feature adds no
  special counter path for those parts. Their acceptance is synthetic: seam
  tests over synthetic identities. No hardware result is claimed for them.
- **D-9 Thresholds (Q-6) ride as text, and nothing evaluates them.** A
  row's threshold text is copied into the tree, the console block and the
  report unchanged. Measured at the audit point with the library's own parser
  (`parseFormula`, `source/counters/linux_pmu/metric_formula.hpp:102`): 0 of
  the 102 threshold texts of `skx-metrics.json` parse. 80 name `&`, one names
  `|`, and the 21 plain comparisons of the shape `tma_alu_op_utilization >
  0.4` end in "malformed formula: trailing text", because a comparison stands
  in this grammar only inside a conditional test. Reading the threshold shape
  is new grammar work, and the request bars it, so the feature prints the text
  and states no verdict. The rejected readings were a threshold reader of one's
  own, and marking a node against a threshold no part can evaluate.
- **D-10 The hunter link (Q-7).** Out of scope. The hunting lane uses this
  tree after this feature lands.
- **D-11 The threshold reaches the tree through the table parse.** The parse
  of a metric row keeps name, group, scope, scale text and description
  today, and drops the row's threshold text. This feature carries the
  threshold through the row carrier, the parsed row and the listing, so one
  parse answers every consumer.
- **D-12 The table source is read from the vendored manifest.** The report's
  source record names the pinned reference, the selected directory and the
  `RECORD` sha256 of the file that holds the rows. The manifest stays the
  single source of truth: the build that embeds the tables also carries the
  reference line and the per-file hash to the library, and the library
  publishes them. No hash is retyped by hand, and no path is opened at run
  time.
- **D-13 The CPU identity is this feature's own seam.** The library's CPU
  record holds vendor, family, model and stepping alone: no core type for a
  hybrid part and no Arm `MIDR`. This feature defines one identity record
  with those fields plus a core type and an `MIDR`, fills the four existing
  fields from the library's reader in production, leaves the two new fields
  absent in production, and takes the record as a parameter so a test hands in
  synthetic CPUID words, a synthetic core type and a synthetic `MIDR`. The
  selection function reads no CPUID itself. A `MIDR` reader and a hybrid core
  reader are later items: no Arm table and no hybrid host stand in this
  feature.
- **D-14 Unsupported answers.** An unsupported host gets one message that
  names the vendor, the family and model or the `MIDR`, and the reason. The
  report records `topdown` as unsupported. The exit status of the run does
  not change. A regression rule that names a top-down node against a report
  or a host with no top-down exits with status 2 and one quoted line.
- **D-15 The main page pays for its own lines.** `README.md` stands at
  1004 lines and the page cap is 1004, so the cap does not move. The new
  section, its option row and its name rows are paid for by trimming the
  page by the same measured number of lines in the same change, and the
  commit body records the figures.
- **D-16 Acceptance is local.** Every acceptance claim comes from the
  owner's own machines, recorded in the pull-request body with the host, the
  kernel and the test counts. No acceptance is gated on hosted runner CI,
  and this feature changes no CI file.
- **D-17 Version and schema numbers.** The feature takes version 0.29.0:
  the lane planned for 0.29.0 holds no merge at branch time, checked against
  `specs/`, the remote branches and the open pull requests. The report gains
  one key, so the schema moves from 1.11 to 1.12. The report reader plants
  `1.12` as the example of a later minor in two tests
  (`test/source/harness_json_load_test.cpp:107`, `:1199-1202`), so the move
  moves those plants one minor up as well. If the other lane lands first, this
  lane takes the next free minor and the next free schema minor and re-runs
  every gate.

- **D-18 The fixed-counter bound.** The gap of PC-7 blocks every live value
  this feature reports, so it is fixed before the acceptance runs, in its own
  commit with its reproducing test first, in the shape the counters lane uses
  for a defect. If the counters lane lands it first, this lane rebases onto
  that commit and drops its own. The bound keeps its contract: a number outside
  the range the instruction accepts still answers "not allowed".
- **D-19 The SMT fact on a host with no core objects (PC-6).** Two of the four
  Intel level-1 rows name `#SMT_on` in their own formula, and the reader needs
  core and thread objects this host publishes none of. This lane carries the
  small shape: when the object tree answers no core object, the host-fact reader
  reads the sibling list of one cpu under `sysfs` and answers the flag from it,
  the object tree keeps priority, and the path a test fakes stands behind the
  same seam the constants use. The larger shape stays out: a provider the
  harness registers beside the clock and pmu registrations, seeding `package`,
  `core` and `thread` objects from the layout hwloc already computes
  (`source/harness/topology.cpp:442`, `source/harness/cli.cpp:566-579`). That
  shape answers every host fact from one source and reaches harness runs only,
  so it is a system-tree feature of its own.
- **D-20 Rows that print under another object (PC-11).** The loader drops
  nothing: it answers a verdict for every metric row of the selected directory,
  and specs/022 bars a silent drop. The duty lands on the reader. The selection
  reads the whole published listing, keeps the rows that print under `cpu/` or
  an uncore object, and one test names a level whose rows print under two
  objects. No loader change and no new refusal kind.
- **D-21 The entry point offers its PMU provider once (PC-8).** The local fix
  is one line at `source/harness/cli.cpp:579`: offer the provider once per
  process, since every later construction is refused anyway. That takes the two
  thousand-call windows from about 63 minutes to seconds and makes the full
  suite and the `taskset -c 0,1` run of the acceptance list finishable on this
  host, so this lane carries it in its own commit with its test first. The wider
  shape stays out: a process-static PMU state in the counters library, so a
  second provider costs nothing for every caller of the library, belongs to the
  counters lane.



## Audit point

Measured on this machine on 2026-11-01 before any file changed.

- Base: `master` at `d454bcffd40ee66edd06bf5d7df0997b4f9561cf`, dated
  2026-10-31 in `git log`, the owner's message `CI: Add the record check and
  the pre-push step`.
- Version: `0.28.0` in `CMakeLists.txt`. Harness library `SOVERSION` 19,
  archive `SOVERSION` 4, checked in the same file. Report schema `1.11` as the
  one constant `kSchemaVersion` (`source/harness/json_report.cpp:43`); the
  schema document types the field as a string and holds no number
  (`docs/schema/report-v1.json:11-13`).
- Option values: 256 to 259, 260, 270 to 277, 280 to 282, 290 to 296, 320,
  330 and 340 are taken; 297 to 319 and 321 to 329 stay free, and 350 is
  free (`source/harness/detail/options.hpp`).
- Spec number: `specs/` on `master` ends at `040-hunter-sensitivity`. No
  branch on the remote and no pull request carries `041` or `042`. The
  feature takes `042`, and the branch `042-topdown` stands on the base hash
  above.
- Vendored tables: `external/pmu-events`, `RECORD` line 1 names
  `linux-7.2.4`, line 5 the license, line 6 the scope, line 7 the
  exclusions, and `arch` holds `x86` alone.
- Host: `CorsairOne`, `Intel(R) Core(TM) i9-9920X CPU @ 3.50GHz`, vendor
  `GenuineIntel`, family 6, model 85, stepping 4, one socket, 24 threads.
  Kernel `7.3.0-4-cachyos`.

## Preconditions

- **PC-1: N2 publishes the rows a tree needs, minus one field. PASS with one
  gap.** `catalogMetrics` answers every row of the selected table, published
  rows and refusals alike, each with its group text, unit text and
  description (`include/speedgun-ng/counters_metrics.hpp:91-114`), and
  `metricByName` adds the spine (`:81-82`). The row's threshold text is
  absent from every carrier: `PmuTableEntry` holds name, description, unit,
  fields, metric name, formula text, scale text and group
  (`source/counters/detail/pmu.hpp:35-58`), and `addMetricEntry` reads no
  threshold key (`source/counters/linux_pmu/table_parse.cpp:413-433`).
  D-11 closes the gap.
- **PC-2: the identity seam exists, minus two fields. PASS with one gap.**
  `PmuIdent` holds vendor, family, model and stepping
  (`source/counters/detail/pmu.hpp:62-72`), read from CPUID by
  `pmuIdentCurrent` (`source/counters/linux_pmu/table_parse.cpp:55-106`).
  The seam a test uses is `pmuSelectDirectory(std::istream&, const
  PmuIdent&)` (`source/counters/detail/pmu.hpp:99-100`). No field names the
  core type of a hybrid part, and no Arm `MIDR` reader exists. D-13 closes
  the gap.
- **PC-3: the grammar reads every construct the top-down rows use. PASS.**
  The formula tree carries `MINIMUM`, `MAXIMUM`, `CONDITIONAL`, `HAS_EVENT`,
  `SOURCE_COUNT`, `GREATER` and `LESS`, and a node carries the device and
  modifier of an at-style reference (`source/counters/linux_pmu/metric_formula.hpp:52-70`).
  A top-level conditional, `min`, `max`, `d_ratio` and an escaped comma all
  parse. Measured over the 102 top-down rows of `skx-metrics.json`: 7 rows
  carry a conditional, 12 carry `min` or `max`, 8 carry an at-style
  reference with an escaped comma, and none carries a nested conditional.
- **PC-4: the table states every parent. PASS, measured.** `MetricGroup` is
  a `;` separated list. Each row of level 2 to 6 carries exactly one group
  of the shape `tma_<name>_group` whose stem names another top-down row of
  the same file, and no row carries two of them. Measured over all 98 deeper
  rows: 98 rows with one parent, 0 rows without, 0 rows with two. The per-row
  table is `skx-topdown-rows.csv` in the audit record. AMD spells the same
  fact without the `tma_` prefix: `amdzen4/pipeline.json` and
  `amdzen5/pipeline.json` each hold 14 rows, five `PipelineL1` rows with no
  parent, eight `PipelineL2` rows with one `<name>_group` parent token each,
  and one helper row (`total_dispatch_slots`) that carries no group and is not
  a node.
- **PC-5: the table's slot rows reach a constant this host cannot read. GAP,
  measured, and D-6 closes it.** `tma_info_core_core_clks` is the one row
  that names `#core_wide`, and `tma_info_thread_slots` is
  `4 * tma_info_core_core_clks`, which all four level-1 rows divide by. On
  this host `--catalog` answers `refusal=UNKNOWN_CONSTANT reason=no reader
  answers the constant #core_wide`. Measured through `--catalog`, which is the
  loader's own voice: of the 102 rows that carry a TopdownL token, 69 carry a
  refusal and 33 publish with no refusal, so levels 3 to 6 already answer here
  while levels 1 and 2 answer nothing. The mix over the 69: 39 rows refuse on
  `#core_wide`, 9 on `#SMT_on`, 20 because a reference names no event the scope
  reaches, and 1 on an unsupported construct. Refusing rows over rows the table
  states, per level: level 1 four of four, level 2 eight of eight, level 3
  fifteen of 25, level 4 twenty-one of 34, level 5 ten of 17, level 6 eleven of
  14. The 33 that publish split ten, thirteen, seven and three over the same
  four levels. PC-7 keeps the fixed counters out of all of them, and PC-11
  states where the rows print.
- **PC-6: `#SMT_on` answers nothing on this host. GAP, open.** The reader
  exists and counts the core and thread objects of the open system
  (`source/counters/linux_pmu/metric_tables.hpp:104-146`). This host
  publishes no object of either kind, so `--catalog` answers `this host
  publishes no value for the constant #SMT_on` for `tma_bad_speculation`.
  Two level-1 rows name the constant in their own formula,
  `tma_backend_bound` and `tma_bad_speculation`, and 12 rows of the file name
  it. `--catalog` reports it as the refusal reason of one of the two, because
  the loader stops at the first gap it reaches. The plan names the fix and its
  owner.
- **PC-7: every fixed counter reads zero through the fast read path. GAP, root
  cause found, owner named below.** With `kernel.perf_event_paranoid` at -1
  the open is granted: 499 leaves countable and 497 readable through `rdpmc`,
  and a raw `perf_event_open` for the same event counts 450969862 cycles, so
  the hardware answers. The fast path refuses the published index: for a fixed
  counter the kernel marks the page index with bit 30, so `cpu/cpu-cycles`
  publishes index `0x40000002`, and the bound `index <= kRnpmcMaxIndex`
  (`source/counters/linux_pmu/fast_read.cpp:48-51`,
  `source/counters/detail/pmu.hpp:284-289`) rejects it. The read then yields
  value 0 with the gap flag (`source/counters/linux_pmu/group_io.cpp:436-437`),
  and `counters_pmu_test` reports `zero count: cpu/cpu-cycles mode 1 value
  0.000`. Eight of the 499 countable leaves are fixed-counter spellings, and one
  such leaf turns a whole window into a gap: `cache-references` reads 87 and 97
  through the raw path while the fold reports 0 with the gap flag. The fix is
  two lines: bound the counter number with bit 30 masked off, keep the
  unchanged operand for the instruction, and correct the comment. It belongs to
  the counters path, and this feature depends on it for every live value: level
  1 of Skylake-X reaches `tma_info_thread_slots`, and the fixed spellings ride
  the same windows. Whether this lane carries it or the counters lane lands it
  first is the owner's call, recorded as D-18. Live values on the acceptance
  host wait on this gap.
- **PC-8: one baseline test cannot finish inside its timeout. GAP, measured,
  owned outside this feature.** `harness_cli_test` reaches 300 seconds under
  `ctest` at the base hash, before this feature, and that 300 seconds is the
  default its registration leaves in force
  (`test/CMakeLists.txt:729-738`, `:2123-2128`). It does not hang. Two loops
  of one thousand in-process entry-point calls (`test/source/harness_cli_test.cpp:1277-1289`)
  each cost 1.7 to 1.9 seconds, because every call builds a fresh PMU provider
  and re-probes the host: one provider build issues 49,330 event opens, 995 of
  them granted. About 2,040 calls then need about 63 minutes. The cause sits at
  `source/harness/cli.cpp:579`: the entry point constructs a `PmuProvider` on
  every call, and the open system refuses it after its first call
  (`source/counters/system.cpp:218`), so each call pays 1.35 to 1.72 seconds of
  device walk and about 45,000 event opens for a provider that is thrown away.
  D-21 names the fix and its owner. `prose_gate_fixtures` fails under `ctest -j 12` and passes
  alone, so it is a load condition on this host.
- **PC-9: the seams a unit test needs exist. PASS.** `metric_fixture.hpp`
  builds a 100-row table and a fake host whose object tree answers the host
  constants in four shapes: `SMT_ON`, `SMT_OFF`, `CORELESS`, `THREADLESS`
  (`test/source/detail/metric_fixture.hpp:83-292`, `:303-368`). The seam
  `pmuParseTableFile` reads a synthetic table from a file under
  `SG_SEAM_TABLE_DIR` (`source/counters/detail/pmu.hpp:107-112`). No metric
  test opens an event.
- **PC-10: the pass planner takes a set of columns. PASS.** `planPasses`
  takes the leaves of every column grouped by source and the limit of every
  source, and answers the passes (`source/harness/detail/pass_plan.hpp`).
  The limit is read through the published object fact, and this feature
  changes no limit code: `source/counters/linux_pmu/counter_limits.cpp`
  belongs to the other lane.
- **PC-11: a metric row prints under the object its formula resolves to.
  MEASURED, and the tree has to read past it.** Of the 279 metric rows the
  Skylake-X directory publishes, 150 print under `machine/` and 129 print
  under another object: 119 under `cpu/`, 4 under `cstate_pkg`, 3 under
  `cstate_core`, 2 under `uncore_iio_0` and 1 under `msr`
  (`source/counters/linux_pmu/metric_formula.cpp:1047-1049`, and the catalog
  printer at `source/harness/catalog.cpp:62`, which prints a row only under its
  own object). A reader that filters on `machine/` therefore sees 150 rows and
  calls the rest missing. Nothing is dropped: the loader answers a verdict for
  every metric row, and `test/source/counters_metric_lookup_test.cpp:782-783`
  holds that rule for all 46 directories. The tree reads the whole listing, and
  a test names a level whose rows print under two objects.
- **PC-12: no reader answers the manifest hash, and a published row names no
  file. GAP, open.** D-12 states the source record the tree owes the operator.
  `CMakeLists.txt:962-988` reads the `ref:` line of `external/pmu-events/RECORD`
  at configure time and refuses a moved reference, so the pinned reference
  already reaches the build. Nothing carries the file hash: no C++ reader opens
  `RECORD`, and `PmuTableEntry` (`source/counters/detail/pmu.hpp:35-58`) holds
  no file name, so a published row cannot say which file it came from. The plan
  names the change in `cmake/embed_pmu_blob.cmake` that carries the reference,
  the selected directory and the hash of each table file into the library as
  build-time data, and the seam a test uses to fake them.

## Roadmap position

N2 (022) publishes a metric row as a formula. N6 (024) splits a set of
formulas into passes. N3 (023) writes and reads the JSON report. N10 is the
layer above all three: it chooses the rows, states the tree, asks N6 for the
passes, and puts the result in the console block and the JSON document. The
planning lane N6-P (planned 041) changes how the counter limit is found and
touches the same shared files; the two lanes meet at the rebase rules in the
Constraints section. The hunting lane's use of this tree (N14c) waits for
this feature.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Read the level-1 breakdown of my benchmark (Priority: P1)

A performance engineer finishes a run on a supported CPU and wants the four
top-down categories beside the timings: how much of the machine's issue
capacity went to the front end, to bad speculation, to the back end, and to
retiring work. One option turns the breakdown on. The engineer reads it in
the console under the benchmark's own table.

**Why this priority**: Level 1 is the entry point of every top-down method,
and the ruling the owner set requires level 1 on every supported CPU. Without
it nothing else in the feature has a use.

**Independent Test**: Run one benchmark with the option and no level, against
a host whose table carries level-1 rows. The block prints the level-1 names
in percent, each with the vendor name it came from, and the report carries
the same nodes with their values. With a fake event source the values are
exact.

**Acceptance Scenarios**:

1. **Given** a run on a host whose table carries level-1 rows, **When** the
   operator asks for top-down without a level, **Then** the console prints one
   block per benchmark with the level-1 values in percent and the level shown.
2. **Given** the same run, **When** the report is written, **Then** the
   document carries one top-down object for that entry with the source, the
   level and one record per node.
3. **Given** a fake event source with known values, **When** the run
   completes, **Then** each level-1 value equals the value the fixture set.
4. **Given** an AMD host, **When** the block prints, **Then** it prints
   `topdown.smt_contention` with the four shared values and the line that
   states the four add to less than 100 percent without it.

---

### User Story 2 - Drill down to the level I ask for (Priority: P2)

The same engineer sees that the back end dominates and wants the split
underneath it. One value on the same option asks for level 2 or deeper. The
tree prints indented under each parent, and the plan says how many passes the
collection needed.

**Why this priority**: Deeper levels are the reason the tree exists, and they
depend on level 1. They follow it.

**Independent Test**: Ask for level 2, 3 and 6 against the same table, and
check the parent of each printed node, the indent, and the pass count the
plan states. Ask for a level deeper than the table holds and read the
statement that names the deepest level the table holds.

**Acceptance Scenarios**:

1. **Given** a table whose deepest level is 4, **When** the operator asks for
   level 6, **Then** the block prints level 4 and says the table holds no
   deeper level.
2. **Given** a run at level 3, **When** the block prints, **Then** every
   deeper node stands indented under its own parent and carries the vendor
   name from the table.
3. **Given** a run at a level whose rows need more counters than the source
   can hold at once, **When** the plan is stated, **Then** the plan text names
   the passes the collection runs.
4. **Given** a node whose table row carries threshold text, **When** the
   block prints, **Then** the block prints that text beside the node,
   unchanged, and states no verdict on it.

---

### User Story 3 - Read the tree from the report and gate on it (Priority: P2)

A maintainer compares two runs in a script. The maintainer reads the report
back, and writes a rule over one node: backend bound may not rise by more
than five points. The rule fails the run with one line when the node is worse.

**Why this priority**: The machine-readable form and the gate are what make
the tree usable in a pipeline, and both depend on the tree existing.

**Independent Test**: Write a report with the tree, read it back, and compare
the values. Run the same report through the rule set with a rule that names a
node, one that names no node, and one against a report with no tree.

**Acceptance Scenarios**:

1. **Given** a report with a top-down object, **When** the loader reads it,
   **Then** every node, its parent, its vendor name, its value and the source
   come back unchanged.
2. **Given** a rule that names `topdown:backend_bound` with a comparison,
   **When** the comparison runs against two reports, **Then** the verdict uses
   that node's value.
3. **Given** a rule that names a node the report lacks, **When** the
   comparison runs, **Then** the program prints one quoted line and exits with
   status 2.
4. **Given** a report written without the option, **When** it is compared with
   an older report, **Then** it differs from the same run before this feature
   only in the schema version.

---

### User Story 4 - Understand a host that has no source (Priority: P3)

An engineer runs the same benchmark on a CPU with no table. One message names
the vendor, the identity and the reason. The run's exit status says nothing
about it, and the report records the state so a later reader knows the tree
is absent on purpose.

**Why this priority**: A clear refusal is what keeps an unsupported host from
looking like a broken tool, and it matters for every CPU without a table:
older AMD parts, hybrid parts, Arm parts, and unknown vendors.

**Independent Test**: Select a tree for each unsupported identity through the
test seam and check the message, the record and the unchanged exit status.
Check that a rule naming a node on such a host exits with status 2.

**Acceptance Scenarios**:

1. **Given** an identity with no top-down group in its table, **When** the
   selection runs, **Then** it answers unsupported with the vendor, the
   family and model, and the reason.
2. **Given** an Arm identity with a `MIDR`, **When** the selection runs,
   **Then** the answer names the `MIDR` and says no table is vendored for it.
3. **Given** an unsupported host and the option given, **When** the run
   finishes, **Then** the run's exit status matches the same run without the
   option and the report records top-down as unsupported.
4. **Given** an unsupported host and a rule that names a node, **When** the
   comparison runs, **Then** it prints one quoted line and exits with status
   2.

---

### User Story 5 - Copy a working block from the page (Priority: P3)

A reader opens the main page, reads one short section about the tree, copies
the block, and builds a program that prints level 1 or the unsupported
message. The guide page carries the longer explanation.

**Why this priority**: The constitution makes a main-page section with a
runnable example part of every feature that changes what a user sees.

**Independent Test**: The drift gate compares the page block with the region
in the example file, the option row with the option table, and the section
list with the page. The run gate executes the example and fails when a
registered benchmark prints no row.

**Acceptance Scenarios**:

1. **Given** the new section, **When** the drift gate runs, **Then** the
   section is in the section list, its owner file exists, and its block
   matches the region in that file.
2. **Given** the example, **When** the run gate executes it, **Then** its
   registered benchmark prints a row, on a supported host with the values and
   on an unsupported host with the message.

### Edge Cases

- A row whose parent no other group names: the row is not a node, and the
  tree holds no invented parent.
- A row that names another metric which is itself refused: the row refuses
  with the same reason, and the report lists it.
- A level deeper than the selected table holds: the deepest level prints, and
  the block says so.
- A node whose value is a gap because the event read nothing: the node prints
  as a gap, with no percent value.
- A threshold text the formula grammar refuses: it rides as text and nothing
  evaluates it.
- A table with a helper row that carries no group, such as AMD's slot count:
  it is not a node.
- A hybrid Intel identity: unsupported, because the core type stands absent
  and no hybrid host stands in the acceptance set, even though the mapping
  file selects a directory for those models.
- An unknown vendor, a family with no mapping row, and an AMD family whose
  directory holds no pipeline table: three separate reasons in one message
  shape. AMD families 23 and 25 map to `amdzen1`, `amdzen2` and `amdzen3`,
  and none of the three holds a pipeline table (FR-007).
- A run with several benchmarks: one block per benchmark, each with its own
  values.
- A run whose filter matches no benchmark: no block, and the existing message
  for an empty filter.
- `--topdown` with the value 0, the value 7, or the value `x`: one quoted
  line each, exit status 2.
- A report whose top-down object is absent, empty, or holds a node with no
  parent: the loader reads the first two, and refuses the third with the
  existing refusal shape for a malformed document.
- Two AMD level-1 values that sum with `smt_contention` to 100 percent: the
  block prints the note in both cases.

## Requirements *(mandatory)*

### Selection (C-1)

- **FR-001**: When a run asks for the top-down tree, the counters library
  shall select its source in this order: the directory the vendored mapping
  file gives for the running CPU's vendor, family and model; then the rows of
  the top-down groups of that directory; then, when the directory holds no
  top-down row, the level-1 rows of the architecture's own source; then an
  unsupported answer with a reason. (D-1.)
- **FR-002**: The selection shall read no counter, open no event, read no
  clock and call no CPUID reader of its own: its answer follows from the
  identity it is given and the rows it is given.
- **FR-003**: The identity the selection reads shall carry the vendor, the
  family, the model and the stepping, which the production path fills from
  the library's own CPU reader, and two further fields, the core type of a
  hybrid part and an Arm `MIDR`, which stand absent on every production host
  of this feature because no reader for either exists (D-13). A test shall
  hand in synthetic CPUID words, a synthetic core type and a synthetic
  `MIDR`. (D-13.)
- **FR-004**: When the identity names a vendor the vendored tree holds no
  table for, the selection shall answer unsupported with the vendor text and
  the reason.
- **FR-005**: When the identity names an Arm part, the selection shall answer
  unsupported, name the `MIDR`, and state that no Arm table is vendored.
  (D-7.)
- **FR-006**: When the mapping file selects a hybrid directory for the
  identity, the selection shall answer unsupported and state that this
  feature makes no claim for a hybrid part, because the core type stands
  absent (FR-003) and no hybrid host stands in the acceptance set. (D-13,
  SC-12.)
- **FR-007**: When the selected table holds no row of any top-down group, in
  either the Intel or the AMD spelling (FR-014), the selection shall answer
  unsupported and name the directory it read.
- **FR-008**: When the selected table holds level-1 rows and no deeper row,
  the selection shall answer a level-1 tree, whatever deeper levels the
  request asked for.
- **FR-009**: The selection shall answer the same tree for the same identity
  and the same table on every call, and it shall answer no tree the table
  does not hold.

### The tree (C-2)

- **FR-010**: Each node of the tree shall carry the vendor-neutral name, the
  vendor's own name, the parent, the level, the formula of the row as N2
  publishes it, the events that formula needs, and the row's threshold text
  where the table carries one.
- **FR-011**: Level 1 shall carry the four shared names
  `topdown.frontend_bound`, `topdown.bad_speculation`,
  `topdown.backend_bound` and `topdown.retiring`, and the fifth name
  `topdown.smt_contention` on an AMD table alone. (D-2, D-3.)
- **FR-012**: Every node below level 1 shall keep the vendor's own name
  behind a vendor prefix in the form `topdown.<vendor>.<table name>`.
- **FR-013**: The parent of a node shall come from the table's own group
  text: a group token of the shape `<stem>_group` whose stem names another
  top-down row of the same directory. Intel spells the token
  `tma_<name>_group` and AMD spells it `<name>_group`, and both spellings are
  data. When the table names no parent for a row, the tree shall hold no node
  for that row, and no part of the feature shall invent one.
- **FR-014**: The level of a node shall come from the table's own level group
  text, `TopdownL1` to `TopdownL6` on Intel and `PipelineL1` and `PipelineL2`
  on AMD. A row that carries no level group is not a node.
- **FR-015**: The tree shall state the source it came from: the pinned
  reference of the vendored tree, the selected directory, and, for each file
  of that directory that contributed a row to the tree, the file's name as
  the embedded table names it and the manifest hash of that file. (D-12,
  PC-12.)
- **FR-016**: The tree shall list every top-down row of the selected level
  and of every level above it that the host refuses, each with its refusal
  kind and its reason. A refused row shall stay out of the collected set and
  shall not disappear from the record. The selection shall read the whole
  published listing, wherever a row prints (PC-11).
- **FR-017**: The table's threshold text shall reach the tree, the console
  block and the report as text, and the feature shall evaluate no threshold:
  the formula grammar refuses every threshold text of the selected table
  (D-9).

### Collection (C-3)

- **FR-018**: When a run asks for a level, the harness shall collect the
  rows of that level and of every level above it.
- **FR-019**: When no level is asked for, the harness shall collect level 1.
  The option word with no attached value asks for level 1, and an attached
  value of 0 is a bad value under FR-050, so the two arrivals answer
  differently. (D-5.)
- **FR-020**: When the asked level is deeper than the selected table holds,
  the harness shall collect the deepest level the table holds and the console
  block shall say that the table holds no deeper level.
- **FR-021**: The harness shall hand the formulas of one top-down run to the
  multi-pass planner of specs/024 as one set, and the run shall state the
  passes the planner produced.
- **FR-022**: The feature shall read the programmable-counter limit only
  through the published object fact, and shall change no code under
  `source/counters/linux_pmu/` beyond the three repairs D-18, D-19 and D-21
  name, each in its own commit with its test first. The limit code
  `source/counters/linux_pmu/counter_limits.cpp` stands untouched, and the
  harness registration line D-21 changes is harness code.

- **FR-023**: When a run carries metric columns and asks for the tree, the
  harness shall collect both in one plan, one set of passes, and shall keep
  every metric column it would have reported before the option.
- **FR-024**: The split and the collection shall run outside the timed window
  of the benchmark, on the existing pass path, and shall change no time
  column.

### The console block (C-4)

- **FR-025**: When a run with the option finishes a benchmark, the harness
  shall print one top-down block for that benchmark after its result tables.
- **FR-026**: The block shall print the level it shows, then the level-1
  values in percent, then every deeper node indented under its own parent.
- **FR-027**: Each printed node shall show its vendor-neutral name and its
  value, and the block shall show the vendor name of every node that carries
  one.
- **FR-028**: When the table is an AMD table, the block shall print
  `topdown.smt_contention` and the line that states the four shared values
  add to less than 100 percent without it. (D-3.)
- **FR-029**: When a node's row carries threshold text, the block shall
  print that text beside the node, unchanged, and shall state no verdict on
  it. No exit status changes for a threshold.
- **FR-030**: When a collected node reads a gap, the block shall print the
  node as a gap and shall print no value for it.
- **FR-031**: The block shall list each refused row it knows about at the
  level it shows, one line each, with the reason.

### The report and its reader (C-5)

- **FR-032**: When a run with the option writes the report, the document
  shall carry one top-down object for that entry: the source, the level shown,
  and one node record per node with its neutral name, its vendor name, its
  parent and its value.
- **FR-033**: The object shall carry the refused rows with their reasons, and
  on an unsupported host it shall record top-down as unsupported.
- **FR-034**: The report reader shall return every field of the object
  unchanged, and the values a run wrote shall equal the values the reader
  returns.
- **FR-035**: When a run writes a report without the option, the document
  shall be identical to the same run before this feature, and the schema
  version shall be the only difference.
- **FR-036**: The schema document shall gain the top-down object, and the
  schema version shall move to the next free minor above the base. (D-17.)

### Regression rules (C-6)

- **FR-037**: The rule language shall accept a rule name of the form
  `topdown:<node>` with the comparison, direction and tolerance the existing
  rule forms carry, and the rule shall compare that node's value between two
  reports. `<node>` is the node's name with its leading `topdown.` dropped, so
  a level-1 node reads `topdown:backend_bound` and a deeper node reads
  `topdown:intel.tma_l1_bound`. The rule's own separators are colons, and a
  node name carries dots, so one split on the first unescaped colon names the
  node and leaves the direction and tolerance text intact.
- **FR-038**: When a rule names a top-down node the report does not carry, the
  comparison shall print one quoted line and exit with status 2.
- **FR-039**: When a rule names a top-down node against a host or a report
  with no top-down, the comparison shall print one quoted line and exit with
  status 2. (D-14.)

### Unsupported answers (C-7)

- **FR-040**: When the selection answers unsupported and the run asked for
  the tree, the harness shall print one message naming the vendor, the family
  and model or the `MIDR`, and the reason.
- **FR-041**: When the selection answers unsupported, the run's exit status
  shall equal the status of the same run without the option. (D-14.)

### Host facts (C-8)

- **FR-042**: The counters library shall answer the constant `#core_wide` for
  a host that publishes it, through a reader with a seam test, because every
  Intel level-1 row reaches that constant through the table's slot rows.
  (D-6, PC-5.)
- **FR-043**: The constants `#num_dies`, `#SYSTEM_TSC_FREQ` and `#has_pmem`
  shall keep their present refusal, because no top-down row reaches any of
  them. (D-6.)

### Documentation (C-9)

- **FR-044**: The main page shall carry a section for this feature of at most
  25 lines: one purpose sentence, one marked block, one result sentence, with
  its quick-reference rows and its option row, and the page shall stay within
  its line cap by the trim recorded in the same change. (D-15,
  constitution IV.1.)
- **FR-045**: The feature shall carry a runnable example under `example/`
  that prints the level-1 values on a host with a source and the unsupported
  message on a host without one, and that the run gate executes.
- **FR-046**: The guide shall carry the longer explanation: the names, the
  levels, the source record, the unsupported reasons, and the rule form.

### Version, tests, and discipline (C-10)

- **FR-047**: The project version, the two `SOVERSION` values and the schema
  version shall move by the checks recorded in the Audit point and in D-4 and
  D-17, and each check shall be recorded in the `CMakeLists.txt` comment the
  earlier features use.
- **FR-048**: Each new behavior shall have its test in the same commit as the
  code it covers, and every unit test of this feature shall run without the
  kernel event interface, without a sleep, without a clock read and with no
  made-up cpu number: the cpus a test uses come from the process's own
  affinity.

- **FR-049**: The whole suite shall pass under `taskset -c 0,1`.
- **FR-050**: Every bad value shall print one quoted line and exit with status
  2: the option with the attached value 0, with the attached value 7, with the
  attached value `x`, and a rule that names no known top-down node.
- **FR-051**: New code shall hold 100 percent line and branch coverage with
  the existing branch allowance unchanged at 21, and this feature shall add no
  coverage exclusion of any kind.
- **FR-052**: The fixtures shall carry both AMD pipeline tables, because their
  level-2 rows carry different names: Zen 4 names `backend_bound_cpu` where
  Zen 5 names `backend_bound_by_cpu`, and seven more rows differ the same way.
  A selection test shall name the level-2 nodes of each. (D-2, FR-013.)
- **FR-053**: The fast read path shall bound the counter number a page index
  carries beneath the fixed-counter marker, and the repair shall have a unit
  test that fails at the base hash and passes at this change, with the fixed
  spellings read through a fake source and no event opened by the test.
  (D-18, PC-7.)
- **FR-054**: The host-fact reader shall answer `#SMT_on` on a host whose
  object tree answers no core object, from the sibling list of one cpu under
  `sysfs`, and the object tree shall keep priority when it answers a core
  object. A seam test shall drive both shapes. (D-19, PC-6.)
- **FR-055**: The entry point shall offer its PMU provider once per process,
  and a test shall show a second entry-point call that builds no second
  provider. (D-21, PC-8.)
- **FR-056**: On an AMD table the block and the record shall carry all five
  level-1 values together, so a reader can add them, and with a fake event
  source the five sum to 100 percent within 0.1 percentage point. (D-3,
  SC-03.)
- **FR-057**: The report of this feature shall name each part with no host
  (Intel parts measured through `PERF_METRICS`, Intel hybrid parts, Arm parts)
  as a pending row, and shall state no hardware claim for one. (SC-11,
  SC-12.)

## Clarifications

### Session 2026-11-01

- Q: Does the feature evaluate a node's threshold and mark a node that passes
  it? → A: No. The library's own parser refuses all 102 threshold texts of
  `skx-metrics.json`: 80 name `&`, one names `|`, and the plain comparisons
  end in "malformed formula: trailing text", because a comparison stands in
  the grammar only inside a conditional test. Reading that shape is new
  grammar work and the request bars it, so the text rides unchanged and no
  verdict stands on it (D-9, FR-017, FR-029).
- Q: How does the tree know a parent on a table that is not Intel? → A: From
  the same data, spelled differently. Intel writes `tma_<name>_group` and AMD
  writes `<name>_group`, and both are one group token whose stem names another
  row of the directory. FR-013 and FR-014 state the rule over both spellings,
  and PC-4 measures 14 AMD rows: five level-1 rows with no parent, eight
  level-2 rows with one parent each, and one helper row with no group
  (`total_dispatch_slots`) that is not a node.
- Q: Does a hybrid Intel part get a tree? → A: No, and the reason is not an
  absent directory. The mapping file maps models 97, 9A, B7, BA and BF to
  `alderlake`, so a directory is selected; the refusal is that the hybrid core
  type stands absent in this feature and no hybrid host stands in the
  acceptance set (FR-006, FR-003, SC-12).
- Q: Which identity fields can the production path fill? → A: The four the
  library already reads: vendor, family, model, stepping. The hybrid core type
  and the Arm `MIDR` stand absent on every production host here, because no
  reader exists for either, and a test hands in both (FR-003, D-13).
- Q: Does the source record name one file? → A: No. The Skylake-X directory
  holds 16 JSON files, and a level's rows come from more than one of them, so
  the record carries one entry per contributing file: the name the embedded
  table gives and that file's manifest hash (FR-015, PC-12).

- Q: Which top-down rows of Skylake-X does the host-constant gap refuse, and
  is a level-1 row among them? → A: Yes, all four. `#core_wide` sits in
  `tma_info_core_core_clks`, `tma_info_thread_slots` is four times that row,
  and all four level-1 rows divide by it. Measured through `--catalog`: 69 of
  the 102 rows carry a refusal (39 on `#core_wide`, 9 on `#SMT_on`, 20 on a
  reference the scope cannot reach, 1 on an unsupported construct), and the
  other 33 publish with no refusal, all of them at levels 3 to 6. PC-5 records
  the per-level figures, PC-11 states where rows print, and D-6 adds the
  reader.
- Q: Does a hybrid part or an Arm part get a tree in this feature? → A: No.
  Neither has a vendored directory, and Arm needs a license verdict and a
  host first. FR-005 and FR-006 carry the two refusals.
- Q: Does the AMD level-1 set add to 100 percent? → A: The four shared values
  do not, and the fifth value `topdown.smt_contention` closes the gap. FR-028
  prints the statement.
- Q: Does an unsupported host change the exit status? → A: It does not. The
  report records the state and the status stands (FR-041).
- Q: Does the feature raise the page line cap? → A: It does not. The page
  stays at 1004 lines and the new section pays for itself (D-15).

## Key Entities

- **Top-down node**: one row of the table, published as a tree member. It
  carries a vendor-neutral name, the vendor's own name, its parent, its
  level, the row's formula, the events that formula needs, and the row's
  threshold text.
- **Level**: the depth a table states for a row. Intel spells it `TopdownL1`
  to `TopdownL6`; AMD spells it `PipelineL1` and `PipelineL2`. A run asks for
  one level and gets that level with every level above it.
- **Source**: where the tree came from: the pinned reference of the vendored
  tree, the selected directory, and the manifest hash of the file that holds
  the rows.
- **CPU identity**: the vendor, family, model, stepping, hybrid core type and
  Arm `MIDR` the selection reads. A seam in this feature, a fact the counters
  library reads once in production.
- **Refused row**: a top-down row the host will not publish, with its refusal
  kind and its reason. It rides the tree record and stays out of the
  collected set.
- **Unsupported answer**: one message with a vendor, an identity and a
  reason, plus the record the report carries.
- **Threshold text**: the table's own condition for a node, kept as text and
  marked on the console when a node passes it.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-01**: Over a fixture table, the level-1 tree of an Intel identity holds
  exactly the four shared names with no parent, and each node carries the
  fixture's formula and threshold text. Over an AMD identity, level 1 holds
  five names and the fifth is `topdown.smt_contention`.
- **SC-02**: Over the same fixtures, every node below level 1 names a parent
  that the fixture table states, and no node names a parent the table omits.
  The fixture row with no stated parent yields no node.
- **SC-03**: With a fake event source, the four Intel level-1 values and the
  five AMD level-1 values equal the fixture values within 0.05 percentage
  point, and the five AMD values sum to 100 percent within 0.1 percentage
  point.
- **SC-04**: Each of the four unsupported identities (an AMD part whose table
  holds no row of any top-down group, an Intel hybrid identity, an Arm `MIDR`,
  an unknown vendor) answers one message with vendor, identity and reason, records
  unsupported in the report, and leaves the exit status of the run unchanged.
- **SC-05**: The option with the attached value 0, the attached value 7, the
  attached value `x` and a rule naming an unknown node each print exactly one
  quoted line and exit with status 2. Four cases, four lines, four statuses.
  The option word with no attached value prints a tree at level 1 and exits
  with status 0.
- **SC-06**: A report written without the option differs from the report the
  same run produced before this feature in one line: the schema version.
- **SC-07**: Every field of a written top-down object comes back unchanged
  through the report reader, for a full tree, for a tree with refusals, and
  for an unsupported record.
- **SC-08**: No unit test of this feature opens the kernel event interface.
  The whole suite passes under `taskset -c 0,1`, and no new test reads a clock
  or sleeps.
- **SC-09**: The coverage gate reads 100 percent line and 100 percent branch
  with the allowance at 21, and the change adds no exclusion marker.
- **SC-10**: On the Intel acceptance host, `--topdown=1` and `--topdown=3`
  each print a tree for one compute-bound and one memory-bound benchmark, and
  the pull-request body records the host, the kernel, the values, the refused
  rows and the test counts.
- **SC-11**: On the AMD acceptance host, `--topdown=2` prints level 1 with
  `topdown.smt_contention` and the 100 percent statement, and prints the eight
  level-2 rows of that directory each under its own level-1 parent, when that
  lane has a gap. Absence of that host is recorded as a pending row, with no hardware
  claim for it.
- **SC-12**: The parts with no host (Intel parts measured through
  `PERF_METRICS`, Intel hybrid, Arm) are covered by synthetic seam tests only,
  and the report of the feature names them as pending rows.
- **SC-13**: A rule that names a level-1 node (`topdown:backend_bound`) and a
  rule that names a deeper node (`topdown:intel.tma_l1_bound`) each resolve to
  that node's value in both reports of a comparison. Two rules, two verdicts,
  no extra quoting.

## Constraints

- Constitution I, II, VI, VII, VIII, X and XI bind in full. 100 percent line,
  branch and DBC coverage, one contract per interface, no exclusion marker.
- ASD-STE100 Simplified Technical English in every generated line, the prose
  gate included, and the commit template of Pull Request Quality.
- Counters-only time and counters: the harness side reads time and counts
  only through plans, recorders and folds of `sg::counters`.
- No new vendored data. The rows this feature needs are already vendored under
  the existing verdict, and no new table joins the tree (D-7).
- No new runtime dependency, and no path lookup at run time: the tables and
  their source record reach the library as build-time data.
- No change to `source/counters/linux_pmu/counter_limits.cpp`, to the limit
  code, or to any CI file.
- Options structs carry every new parameter: no new bool parameter.
- JSON only through the JSON libraries the tree already uses.
- Bad values exit with status 2 and one quoted line.
- Rebase only, no merge commits, and one push per round with
  `--force-with-lease`, followed by `git ls-remote origin 042-topdown`.
- Shared files with every lane, each a rebase point: the version and
  `SOVERSION` lines of `CMakeLists.txt`, `README.md`, `example/CMakeLists.txt`,
  `test/CMakeLists.txt`, `docs/schema/report-v1.json` and
  `source/harness/detail/options.hpp`.

## Scope

In: FR-001 to FR-057. The selection function over its identity seam, the tree
with its parents, levels, names and threshold text, the source record built
from the vendored manifest, the level option, one formula set to the pass
planner, the console block, the report object with its reader and its schema,
the `topdown:` rule form, the unsupported message, the `#core_wide` reader, the
three prerequisite repairs D-18, D-19 and D-21 name (FR-053 to FR-055), the
public header, the version and `SOVERSION` checks, the docs section, the
runnable example, and the tests.

Out, each with its owner:

- An Arm table, an Arm license verdict and an Arm host: a later item (D-7).
- Intel hybrid parts: the mapping file selects a hybrid directory for them,
  and this feature refuses them (FR-006). No mapping-file row changes here.
- A special counter path for Intel parts measured through `PERF_METRICS`: none
  (D-8).
- The hunting lane's use of this tree (N14c): the hunting lane, after this
  feature lands (D-10).
- The programmable-counter limit and how it is found: the planning lane
  (planned 041).
- Threshold evaluation of any kind: none. A threshold is text, printed
  unchanged (D-9).

## Assumptions

- The owner's request of 2026-11-01 is the authority for scope, numbering and
  the accepted answers. The roadmap file it names sits outside this repository
  and is absent from this machine.
- The Intel acceptance host is this machine: `CorsairOne`, family 6, model 85,
  stepping 4, one socket. The AMD acceptance host is a second machine the owner
  runs, and its availability is not this feature's to schedule.
- The vendored tree is the only table source, and its manifest stays the
  authority for the reference and the file hashes.
- The request's level-row counts for levels 3 to 6 (25, 34, 17, 14) match the
  vendored Skylake-X file exactly. Its counts for levels 1 and 2 (8 and 16)
  match no directory in that tree. Measured over the 40 Intel directories: 19
  state 4 and 8, three state 4 and 6 (`alderlaken`, `grandridge`,
  `sierraforest`), the four hybrid-era directories (Alder Lake, Arrow Lake,
  Lunar Lake, Meteor Lake) state 8 and 14, and 14 state no top-down row at
  all. The selected directory governs (FR-013, FR-014). The level width is
  table data, so the tree counts the rows of a level to find its width.
- `#core_wide` means the property the kernel's own generator gives it: a flag
  that tells a row whether its slots are counted core-wide. The reader's exact
  source is a plan decision, and this spec states only that the fact is
  published, fakeable and needed by every Intel level-1 row.
- PC-7 and PC-8 are host conditions at the audit point. This feature reports
  them, and its live-value acceptance waits on them.

## Budget note

Measured at the audit point, before any section was written.

- Page: `README.md` measures 1004 lines and `PAGE_LINE_CAP` is 1004
  (`tools/docs/example_drift.py:240`), so the page sits at its cap. D-15 keeps
  the cap and pays for the new lines with a trim in the same change.
- Section: `SECTION_LINE_CAP` is 25 (`tools/docs/example_drift.py:241`) with
  two recorded overrides, `Registration` 38 and `Fixtures` 43
  (`tools/docs/example_drift.py:246-249`). The new section holds 25 lines and
  needs no override.
- Lists: `SECTIONS` holds 33 titles (`tools/docs/example_drift.py:33-68`) and
  `SECTION_OWNERS` holds one key per title
  (`tools/docs/example_drift.py:73-108`). Both gain the new section, and the
  owner file gains a registration line in `example/CMakeLists.txt`, because a
  docs example with no built binary fails the run gate
  (`test/example_run.sh:37-46`).
- Tables: the name table starts at `README.md:837` and the option table at
  `README.md:954` with 42 rows. This feature adds one option row and the name
  rows its public header publishes.
- Guide: `docs/pages/harness.md` measures 2253 lines and carries no cap,
  because the cap check runs on the main page alone.
- Coverage: the gate is `tools/dbc/coverage_gate.sh`, its allowance is
  `allowance=21` (`tools/dbc/coverage_gate.sh:102`), its ledger comment holds
  two rows (`tools/dbc/coverage_gate.sh:55-101`), and the line gate at
  `tools/dbc/coverage_gate.sh:45-48` demands 100 percent. This feature adds no
  ledger row and no exclusion.
- Names: a new public header needs a `NAME_SCAN_HEADERS` entry
  (`tools/docs/example_drift.py:530-538`), because the quick-reference check
  reads the public headers through that list. `SECTIONS` gains its title only
  together with its `SECTION_OWNERS` key: the tool raises `KeyError` at
  `tools/docs/example_drift.py:683` when one list moves without the other.
- Tests: one counters test registers in six lines
  (`test/CMakeLists.txt:515-522`) and one harness test takes the same shape.
  Two new test files join that list.

## Dependencies

- specs/007-counters-and-timers: the providers, the plan path and the fake
  provider every test drives.
- specs/012-counters-defect-resolution: the embedding of the vendored tables
  into the library, which carries the source record.
- specs/022-metric-formulas: `metricByName`, `catalogMetrics`, `MetricView`,
  the formula grammar and the host-constant readers.
- specs/023-json-reports: the report writer, the reader and the schema
  document.
- specs/024-multipass-collection: the pass planner this feature hands one
  formula set to.
- specs/026-regression-gate: the rule language the `topdown:` form joins.
- specs/015-benchmark-harness-core: the run spine, the option surface and the
  status-2 rule.
- specs/018-docs-main-page and specs/019-harness-user-counters-and-statistics:
  the drift gate, the run gate and the metric column path.

## Citations

- `include/speedgun-ng/counters_metrics.hpp`: `MetricRefusal` (33),
  `MetricView` (63), `metricByName` (81), `MetricListing` (91),
  `catalogMetrics` (113).
- `source/counters/detail/pmu.hpp`: `PmuTableEntry` (35-58), `PmuIdent`
  (62-72), `mapfileKey` (77), `pmuIdentCurrent` (84), `pmuSelectDirectory`
  (89, 99-100), `pmuParseTableFile` (107-112).
- `source/counters/linux_pmu/table_parse.cpp`: `pmuIdentCurrent` (55-106),
  `addMetricEntry` (413-433), the mapfile scan (695-774), table load
  (776-806).
- `source/counters/linux_pmu/metric_tables.hpp`: `countObjects` (104-114),
  `readSmtState` (130-146), `kHostFacts` (150-160).
- `source/counters/linux_pmu/metric_formula.hpp`: `FormulaKind` (52-70),
  `ParsedMetricRow` (115-130), `parsedMetricTable` (147-148), `MetricHost`
  (156-160).
- `source/counters/linux_pmu/embedded_tables.hpp`: `EmbeddedFile` (23-28),
  `EmbeddedDir` (32-40), `embeddedFindDir` (49-58), `embeddedFileBytes`
  (62-72).
- `cmake/embed_pmu_blob.cmake` (40-59, 100-118) and
  `CMakeLists.txt` (961-988) for the reference check.
- `external/pmu-events/RECORD` (1-7) and
  `external/pmu-events/arch/x86/mapfile.csv` (47 lines, `skylakex` at
  `GenuineIntel-6-55-[01234]`).
- `source/harness/detail/options.hpp`: the value block comment (76-82) and the
  constants (83-97).
- `source/harness/detail/pass_plan.hpp`: `ColumnLeaves`, `PassPlan`,
  `planPasses`.
- `tools/docs/example_drift.py`: `SECTIONS` (33-68), `SECTION_OWNERS`
  (73-108), `PAGE_LINE_CAP` (240), `SECTION_LINE_CAP` (241, 246-249), the page
  and section checks (653-694).
- `tools/dbc/coverage_gate.sh`: line gate (45-48), ledger (55-101), allowance
  (102), verdict (111-118).
- `test/CMakeLists.txt`: one counters test (515-522), the seam root (483-489).
- `test/source/detail/metric_fixture.hpp`: `fixtureRows` (83-292), the host
  shapes (303-368).
- `README.md`: the name table (837), the option table (954).

## Open questions

None open. The owner's seven questions stand answered by D-4 to D-10, the
rulings by D-1 to D-3 and D-14, and the numbers by D-17. Three host gaps are
carried work with a requirement each: FR-053 the fixed-counter bound (PC-7),
FR-054 the SMT fact (PC-6), FR-055 the provider offered once (PC-8). One stays
open for the plan to design: PC-12 names the source-record plumbing FR-015
needs.

## Constitution Checklist

- Main-page impact check (IV.1): YES. The feature changes what a user sees, so
  FR-044 to FR-046 carry its main-page section, its runnable `example/` owner
  file with a marked region, its quick-reference and option rows, and its
  guide-page section in the same change, inside the budget the Budget note
  records.
- Coverage (VI): FR-051 holds the 100 percent gates and the allowance at 21.
- Contract (II): every new interface documents `\pre` and `\post` in its header
  and enforces them in its source, in the shape PC-9 quotes.
- Anti-slop (X): the decisions above record each reading a requirement admits,
  and the Scope section names what this feature will not build.
