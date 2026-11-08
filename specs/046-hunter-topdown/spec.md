# Feature Specification: Hunter Top-down Areas

**Feature Branch**: `046-hunter-topdown`

**Created**: 2026-11-03

**Status**: Draft

**Input**: User description: the owner's request for roadmap item N14c, the
link between the hunter lane and the top-down tree. Condensed below; the
request is the authority for scope, numbering and the accepted answers.

> N14a (029), N14a-2 (033) and N14b (040) rank counter events in hunter mode
> (`--hunt`). N10 (042) gives the top-down tree of the running CPU. N14c links
> the two. It adds no counter path and no formula grammar. Each ranked event
> gains a top-down area: the deepest N10 node whose formula uses the event;
> else the event category in the vendor list; else unmapped. The area shows as
> a path in the console and in JSON, for example `backend_bound >
> memory_bound` on Intel and `backend_bound_by_memory` on AMD. Where N10
> supports the host, the hunt block also prints level 1 for the same run, so a
> reader can compare a culprit with its area.

## Spec identity and run-time binding

- Base: `master` at `b1dd76707788ab71bcea184877c7e9b727cde881`, dated 2026-11-02,
  version 0.30.0, schema 1.13, harness `SOVERSION` 20, archive `SOVERSION` 5.
  Every symbol below is a name first and a line number second: a line number
  cites the audit point, and a search for the name is the way to re-find it when
  the lines move.
- Branch: `046-hunter-topdown`, cut 2026-11-03 from the base above.
- Number: 046. Counted at branch time: `specs/` holds 001 to 044, with no 041,
  no 045 and no 046 directory; `git ls-remote --heads origin` carries no `045-*`
  and no `046-*` branch; `gh pr list --state all` answers 73 as the highest
  number. 045 stays reserved for the merge-fix lane, so 046 is free.

## Decisions

The request carries six open questions with recommended answers. Each one is
taken as the decision below and names the reading it settles. No implementer
reopens them.

- **D-1 Events of a node (Q-1).** The event list of a node is public already:
  `counters::TopdownNode::events` (`include/speedgun-ng/counters_topdown.hpp:239`)
  holds the names the node's formula needs, and `N10` also carries the node's
  `formula`, `level`, `name`, `parent` and `vendorName`. The area reads that
  list, so the selection code of `N10` keeps its shape. The one counters change
  this feature needs is the vendor grouping accessor D-16 names.
- **D-2 Option (Q-2).** No new option. The area column stands in every hunter
  run, and level 1 stands beside it wherever N10 supports the host. The block
  360 to 369 stays free and unreserved in
  `source/harness/detail/options.hpp`, which this feature leaves alone. A later
  off switch takes 360 and its own block comment.
- **D-3 Category fallback (Q-3), re-founded on measured data.** The request's
  reading of Q-3 names a `Topic` or `category` field, and that field does not
  exist. Counted over the pinned `external/pmu-events` tree: 542 JSON files
  parse, and the key census over every event and metric row holds
  `EventName`, `Unit`, `MetricGroup`, `MetricExpr` and their siblings, with no
  `Topic`, `Category` or `Layer` key anywhere. What the vendor list does carry
  is its own grouping: each event row stands in one file of the selected
  directory, and `PmuTableEntry::fileName`
  (`source/counters/detail/pmu.hpp:80`) holds that name. So the fallback names
  that file stem, verbatim: `cache`, `frontend`, `virtual-memory`, `branch`,
  `memory`, `pipeline`, `branch-prediction`, `l2-cache`, and the rest of the
  vendor's own spellings. No word is renamed, no event list is written by hand,
  and an event the selected directory holds no row for is unmapped. The
  vocabulary is therefore the vendor's, and it differs per directory: Skylake-X
  answers `cache` where Zen 5 answers `l2-cache` (AP-7).
- **D-4 Several nodes (Q-4).** The deepest node that names the event wins. On a
  tie the first such node in table order wins. The document records the chosen
  path alone, so the schema stays small; the other nodes a shared event belongs
  to stay out of the record.
- **D-5 Arm (Q-5).** `N10` holds no Arm table, so every event of an Arm identity
  answers `unmapped`. No Arm table joins the tree here.
- **D-6 N14b rules (Q-6).** No rule text changes. A `topdown:` rule keeps the
  meaning `N10` gives it, and the area is a printed and recorded fact that
  enters no rule.
- **D-7 The area is derived.** The area follows from the ranked address and the
  tree of the host. It opens no event, reads no counter and reads no clock, and
  it changes no rank, no budget, no emitted set and no emitted rule.
- **D-8 Public surface.** `sg::hunt::RankedEvent` gains one member,
  `std::string topdownArea`, appended after `delta` in the shape the specs/040
  block uses (`include/speedgun-ng/hunt.hpp:454-472`). An empty value means no
  area was settled for that record: a document older than minor 1.14, or a
  record a caller built by hand. A run of this feature settles a non-empty value
  for every ranked event, `unmapped` included. The exported layout moves, so the
  harness `SOVERSION` rises from 20 to 21, counted from the master value at
  branch time. The open question of whether the 042 lane should also have moved
  to 21 stands with the owner and this feature does not answer it. The counters
  surface gains the accessor D-16 names, so the archive
  `SOVERSION` rises from 5 to 6. Both moves ride the comment rule: each names
  the record and the requirement.
- **D-9 Level-1 companion.** The hunt run collects level 1 of its own `N10`
  selection in one short pass beside the hunt stages, through the runner `N10`
  already drives, and prints it with the printer `N10` already has. The collection
  rides its own internal option record, never the process option record that
  `source/harness/cli.cpp:756` fills and `:1505` hands to the entry runner, so a
  hunter run gains neither the `topdown: core-wide coverage=` line of
  `cli.cpp:1584-1593` nor a pass disclosure of its own (FR-035). The refusal of a
  refused host rides the one-per-process helper the explicit path already calls at
  `cli.cpp:1583` (FR-017). The collection joins no hunt stage, so no stage gains an
  event, no multiplex group changes, and no rank, budget or emitted byte moves
  (FR-010, FR-015). The answer travels an internal channel to the console only: no
  object joins the document, which keeps the areas of its own rows (D-11). The block
  prints once per benchmark, after the hunt block of that benchmark closes: after
  `printHuntSkips` (`source/harness/report.cpp:813-815`), which prints the skipped
  rows and the summary line from one function (`:1377-1403`). That function keeps its
  shape, because two callers declare it by hand
  (`test/source/harness_hunt_report_test.cpp:92` and `example/docs/hunt_rank.cpp:28`),
  so the block rides after that function and not inside it. The printer's call at
  `report.cpp:765` stands for every run with no `--hunt` and stands suppressed for a
  hunter run, and the base prints one block per benchmark there, so the count per
  benchmark stands. The exit status stands unchanged (FR-016, FR-033, FR-035).
- **D-10 Naming of the path.** The path joins the level-1 vendor-neutral name and
  the vendor names below it with `" > "`, exactly as `N10` spells them
  (`topdown.backend_bound` above `topdown.intel.tma_l1_bound`, and the AMD
  spellings on an AMD table). A node at level 1 answers a one-name path.
- **D-11 Schema.** `topdownArea` is one key of the per-event record. The record
  is a closed object, so the key joins `x-order` and `properties` and stays out
  of `required`, the shape specs/040 set for `correlation`, `slope` and `delta`.
  `kSchemaVersion` moves from 1.13 to 1.14 and the version-history comment gains
  one line.
- **D-12 Byte identity.** `--hunt-emit-set` and `--hunt-emit-rules` read `rank`,
  `address` and `delta` only, so their bytes stand unchanged. The two fixtures
  of a run with no `--hunt` (`test/data/hunt_no_hunt_console.txt` and
  `test/data/hunt_no_hunt_document.json`) stand unchanged: the identity entries
  that read them mask the schema-version value, the `speedgun` object,
  `overheadFloorNs`, the banner and the overhead figures, and those fixtures carry
  a version string of their own that no reader compares. The new value is proved
  by a unit that reads the writer's own constant and the two version-binding
  readers, and by `loadReport` accepting a document at 1.14.
- **D-13 Console cell.** The hunt block is one `hunt: `-tagged line per row with
  `key=value` cells, one leading space each, and no field width. The area joins
  as `area=<path>` after the specs/040 cells and before the closing
  ` multiplexed` flag or ` mark=<word>` cell of the same row.
- **D-14 Hosts.** `corsairone` (Intel Skylake-X) is the acceptance host. The AMD
  rows ride the `chilly` or `gary` lane when it has a gap. No Arm host and no
  Intel hybrid host exists, so those two rows are synthetic-test rows only. CI
  runners are AMD parts whose vendored directory carries no top-down row, so a
  CI run answers the refusal path and is never the acceptance of an area value.
- **D-15 Coverage.** 100 percent line, the branch allowance at 21, and no new
  exclusion marker. The mapper is a pure function over its three inputs, so
  every arm of it reaches a unit case with no host and no kernel interface.
- **D-16 Vendor grouping accessor.** One read-only function joins the counters
  library, because no public header reaches the vendored row of an event today:
  `pmuLoadTable` and `PmuTableEntry::fileName` sit in the private seam
  `source/counters/detail/pmu.hpp`, and no `source/harness/` file includes that
  header. The new function reads the embedded table of the directory the
  identity selects and answers the pairs it holds:
  `topdownEventGroups(const CpuIdentity&) -> std::vector<TopdownEventGroup>`,
  with `TopdownEventGroup { std::string event; std::string group; }`, declared in
  `include/speedgun-ng/counters_topdown.hpp` and defined in
  `source/counters/linux_pmu/topdown_select.cpp`, the one counters source file
  the request names. It reads no counter, no clock and no path: `pmuLoadTable`
  answers build-time data, cached per directory. It answers every row of the
  directory, event rows and metric rows alike, and it reads no `MIDR` field of the
  identity beyond the directory that identity selects (D-13). An identity that selects
  no directory answers an empty listing, so an Arm identity and an unknown vendor leave
  every event unmapped (D-5). The listing carries bare event names, so the
  harness strips the device prefix of a hunt address, the part before the first
  `/`, before it reads the index (FR-004). The in-tree helper `leafPartOf`
  (`source/harness/detail/hunt_selection.hpp:99-104`) cuts after the last `/`
  instead, and the two agree while a device path holds one name, the shape
  `source/counters/linux_pmu/provider.cpp:338` builds. The accessor owns the first
  row: its answer holds one pair per event name, so the harness index carries no dedup
  rule and reads the listing once per run. Measured over the 46 pinned `arch/x86`
  directories, which hold 46,837 event rows and 5,944 metric rows, six directories
  repeat a name inside one file: `arrowlake` 196 names over 247 surplus rows of 1,166
  rows, `lunarlake` 80, `meteorlake` 78, `alderlake` 61 and `pantherlake` 61, one
  surplus row each, and `amdzen4` 2. Five of the six are hybrid Intel directories that
  reach the rule through a second `Unit` device, and `amdzen4/recommended.json` repeats
  two metric names, `umc_cas_cmd_rate` and `umc_cas_cmd_read_ratio`. No pinned directory
  holds one name in two files, so the unit FR-037 names needs no hybrid host.
  `arrowlake/DL1.DIRTY_EVICTION` stands under `cpu_atom` and
  `cpu_lowpower` as the named case FR-031 reaches. The rule rides one
  insert-if-absent line, documented at the site. The postcondition of the accessor names
  a fact about the listing it built, that it holds one pair per name, and names no fact
  about the contents of a vendor file.
- **D-17 Two asks, one run.** The run makes two `N10` asks and they are not the
  same ask. The area reads the deepest tree the table holds, because an event a
  level-4 node names has no area at level 1: measured on this host, a level-1 ask
  settles a path for 7 of the 497 addresses of AP-3 and a level-6 ask for 58
  (AP-9). The companion prints level 1, as the request states. The two asks share
  the identity, the selection and the table read; `TopdownSelection::level`
  carries the difference.

## Audit point

Measured on `corsairone` on 2026-11-03, before any section was written.

- **AP-1**: `master` at `b1dd7670` builds green through `ci-rocky` with GCC
  16.2.1. `ctest` answers 178 entries and 3 failures, all three pre-existing and
  all three in files the 045 lane owns: `counters_kernel_calls_test` (the real
  open answers no refusal for a malformed attribute), `counters_pmu_provider_test`
  (the kernel-text claim of specs/007 FR-037) and `harness_jitter_window_test` (the FWQ item key order).
  This feature touches none of those files and carries none of those failures.
- **AP-2**: `cmake -P cmake/lint.cmake`, `cmake -P cmake/spell.cmake` and
  `python3 tools/docs/example_drift.py --sections --names --options --forbidden .`
  each answer 0 at the base. The two DBC gates run by hand, because the `dbc-gate`
  target cached a python with no PyYAML. This lane ran them on 2026-11-03 at
  `b1dd7670` and both answered `345 interfaces, 0 gaps`. The three commands are a
  `doxygen` pass over an XML-only Doxyfile whose `INPUT` is
  `include/speedgun-ng` and whose `OUTPUT_DIRECTORY` is `build/dbc-gate`, then
  `/usr/bin/python3 tools/dbc/dbc_doc_gate.py --xml build/dbc-gate/xml --registry
  tools/dbc/macros.yaml --out <m>`, then `/usr/bin/python3
  tools/dbc/dbc_pair_gate.py --src include/speedgun-ng --src2 source tools
  --registry tools/dbc/macros.yaml --doc-matrix <m> --out <m2>`. `--xml` names the
  xml directory, and a path that names `index.xml` reads `XML directory not found`.
  A mapper under `source/harness/detail/` adds no interface, because the gate reads
  `include/speedgun-ng` and `source`.
- **AP-2b**: `tools/prose/prose_gate.py --base origin/master` at the branch point
  is a void run that checked nothing: `resolved range is degenerate (base equals
  head):
  nothing was checked`. Every prose claim of this feature is made over
  `--base b1dd7670` once commits exist, and over `--check commit` for each commit.
  Both forms list tracked paths, so a file that stands untracked reads
  `0 sources, 0 units examined`, and that run is void. The tree form reads a new file
  after `git add -N` names it, and the check clears with `git reset -q` after it. A
  fenced block is exempt for markdown, so a command that carries two hyphens rides a
  fence, or one line inside one span: the span scanner reads one line at a time.
- **AP-3**: one live hunt on this host, `example/memory_walk --hunt`: the plan
  line reads `events=497 passes=249 method=PASSES` and the ranked table prints
  20 ranked rows. The top rows are `topdown-slots-issued`,
  `topdown-total-slots`, `topdown-slots-retired`, `ref-cycles` and
  `branch-instructions`. A second hunt on `example/first_benchmark` answers the
  same plan and `events=493/497` in its summary line.
- **AP-4**: the same address reaches the table under two spellings, and the two
  are different rows: `cpu/branch-instructions` and
  `cpu/BR_INST_RETIRED.ALL_BRANCHES` both carry 227.048 in the run of AP-3. The
  vendored list names one of them in its formulas, so the second spelling is a
  grouping row or an unmapped row (FR-002).
- **AP-5**: `README.md` measures 1000 lines against `PAGE_LINE_CAP` 1004, so the
  page holds 4 lines, and `SECTION_LINE_CAP` 25 caps the span of one `###` section:
  `### Hunter mode` stands at 24 and `### Top-down analysis` at 21. Two runs of the
  drift gate over a planted region for the new example answer `1011 lines, the cap
  is 1004` and `section 'Hunter mode' runs 35 lines, the cap is 25`, and one run
  also answers that the region's file belongs to another section, so
  `SECTION_OWNERS` gains the example under `Hunter mode`. A marked block therefore
  costs its region, six surrounding lines, one quick-reference row and one
  ownership entry. The payable trims read 8 in one run and 5 in another, so the plan
  measures the number and names each line (FR-026, Budget note).
- **AP-6**: `lcov` on this host dies at startup: `Perl API version v5.42.0 of
  DateTime.c does not match v5.44.0`. `tools/dbc/coverage_gate.sh` answers exit 3
  for that reason, which names the tool and holds no verdict on a ratio. The
  coverage row of the acceptance report states that, and a ratio from a host with
  a working `lcov` stands beside it (SC-003).
- **AP-7**: Over the pinned Skylake-X directory, the vendored event rows stand
  in `cache.json` (157 rows), `memory.json` (115), `pipeline.json` (102),
  `frontend.json` (49), `virtual-memory.json` (28), `other.json` (6),
  `floating-point.json` (13) and five `uncore-*.json` files (3312), 3782 rows in
  all. Cross-read against the printed rows of AP-3 on a basis of vendored formula
  rows, counted whether this host publishes them or not: 109 of the 497 addresses
  are named by a top-down formula, and of the other 388 the directory holds a row
  for 361, so the grouping answers 361 and 27 stand unmapped. SC-007 scores the
  printed-row basis of AP-9, which answers 58 paths for the same run. The four words Q-3 names
  would cover 181 of them, because Skylake-X publishes no `branch.json`: every
  branch event of this directory is named by a formula instead. The 109 is a
  count over vendored rows, published on this host or not, and it is the upper
  bound a refusal-free host reaches (AP-9). Zen 4 publishes
  `branch.json` and `cache.json`, and Zen 5 publishes `branch-prediction.json`,
  `l2-cache.json` and `l3-cache.json` in place of one `cache.json`, so the
  grouping vocabulary is the vendor's and differs per directory (D-3).
- **AP-8**: A hunt run builds its `detail::RunOptions` in two places,
  `measurePassNs` (`source/harness/hunt.cpp:419-427`) and the per-stage loop
  (`source/harness/hunt.cpp:1040-1069`), and neither sets `topdownLevel`, so the
  run collects no top-down row today. `detail::stripTopdownColumns`
  (`source/harness/runner.cpp:699`, called at `:3528`) is what turns collected
  top-down leaves into `BenchmarkResult::topdown`.
- **AP-9**: The tree the mapper reads decides how many rows carry a path. One
  ask per level through `topdownTree` with the running identity, measured on this
  host on 2026-11-03:

  | ask | published nodes | refused rows | distinct event leaves |
  |---|---|---|---|
  | 1 | 4 | 0 | 7 |
  | 2 | 12 | 0 | 17 |
  | 3 | 31 | 6 | 37 |
  | 4 | 46 | 18 | 56 |
  | 6 | 49 | 23 | 58 |

  Cross-read against the 497 printed rows of AP-3: a level-1 tree settles a path
  for 7 rows and a level-6 tree for 58, so the split over the printed table reads
  58 node paths, 412 list groupings and 27 unmapped at the deep ask (D-17). Over
  the 20 ranked rows alone the same host reads 1, 10 and 9.
- **AP-10**: Two runs of one unchanged binary answer different emit bytes. At the
  base, `example/memory_walk --hunt --hunt-budget=90s` with both emit options
  wrote `cpu/topdown-fetch-bubbles` first in one run and left it out of the top 20
  in the next, while it promoted `cpu/mem-stores` and `cpu/branch-misses`. The
  emitted set follows the measurement, because `toCounterSet` pairs `rank` with
  `address` and `toCompareRules` reads the sign of `delta`. A live before-and-after
  byte comparison therefore proves nothing, and a seed is no control: any hunt
  that names a non-zero `--hunt-seed` aborts at the base on the assertion at
  `source/counters/fold.cpp:166`, `the ratio pair is written together or left
  together`. That abort is a base defect for a fix lane, and this feature carries
  none of it (FR-011, SC-008).
- **AP-11**: The two-cpu check is red at the base, and the command decides the
  list. The lane's form, the first block below, answers 3 failures over 171
  entries in 680 s at `b1dd7670`: the three names of AP-1. The earlier form, the
  second block, answers 5 failures over 171 in 672 s: the same three beside
  `prose_gate_fixtures` and `report_shape_fwq_ftq_sample`, which pass alone
  under the same mask. That second form carries a 120 s timeout, and
  `example_run` takes 168.89 s under two cpus, so its own bound should have
  named a fourth failure and did not. Both forms list the same 171 entries.

  ```bash
  taskset -c 0,1 ctest --preset prepush-quick -j 2
  taskset -c 0,1 ctest --test-dir build -L quick -j 2 --timeout 120
  ```

  The plain 24-way run answers 3 failures over 178 entries (SC-005, FR-030).
- **AP-12**: The second `N10` ask costs nothing. Measured on this host on
  2026-11-03: the first `topdownTree` ask of a process reads the table in 503 ms,
  the second answers in 0.57 ms from the same read, and the level-6 tree holds 49
  nodes over 58 distinct leaf addresses (AP-9). The deep ask of D-17 therefore
  adds no measurable time to a hunt run (Principle VII). A hunter run on a supported
  host pays that first read, 503 ms, and no base hunter run paid it. A hunt run answers
  in minutes, so the change discloses the cost and does not tune it away.
## Roadmap position

`N14c` sits behind `N14a` (029), `N14a-2` (033), `N14b` (040) and `N10` (042),
and it links the last two. It adds no counter path, no formula grammar, no
option, no rule form and no vendor table. Two lanes share files with it: 045
(no file in the scope below) and 047 (`source/harness/json_load.cpp` and
`test/source/harness_json_load_test.cpp`, in different functions). The second of
the two to merge rebases, keeps both sides, and runs every gate again.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Name the area of each culprit (Priority: P1)

A performance engineer runs `--hunt` on one benchmark and reads the ranked
table. Each ranked row now names the part of the pipeline its event measures,
as a path of vendor names read verbatim from the tree, such as
`topdown.backend_bound > topdown.intel.tma_l1_bound` on Intel and one name such as
`backend_bound_by_memory` on AMD (D-10). The engineer stops guessing which of twenty events are cache events and
which are front-end events.

**Why this priority**: The area is the feature. Without it the hunt names an
event and leaves the reader to look the event up in a vendor table.

**Independent Test**: Run `--hunt` on one compute-bound and one memory-bound
benchmark on the Intel acceptance host, and drive the mapper with synthetic
identities and planted trees. Each ranked row carries an `area=` cell.
An event a node of the run's tree names, at level 2 or deeper, names the deepest
such node. An event no node names names the file stem of its own vendored row,
and an event with no such row reads `unmapped`. Count the three kinds and report
the three counts, where AP-9 gives the figures the same host answers today.

**Acceptance Scenarios**:

1. **Given** a hunter run on a host whose table holds a tree, **When** the
   ranked table prints, **Then** every ranked row carries one `area=` cell
   after the specs/040 cells.
2. **Given** an event a deeper node names and a shallower node also names,
   **When** its area settles, **Then** the deeper node of the tree the run reads
   names the area (D-4, D-17).
3. **Given** two nodes at the same depth that both name one event, **When** its
   area settles, **Then** the node that stands first in table order names the
   area.
4. **Given** an event no node names, and one the selected directory holds a row
   for, **When** its area settles, **Then** the area is the file stem of that row
   (D-3).
5. **Given** an event no node names and no row of the selected directory names,
   **When** its area settles, **Then** the area reads `unmapped`.

---

### User Story 2 - Read the area back from the report (Priority: P1)

A tool author loads a written report with `loadReport` and reads the area of
each ranked event beside its rate, its `cv` and its rank. A document written
before this feature loads as it does today and leaves the area unset.

**Why this priority**: The console is one reader. Every other reader goes
through the document, and the closed schema refuses a key the schema omits.

**Independent Test**: Write a hunt document, load it, and compare each area
with the value the run settled. Then load a 1.13 document and read the member as
unset.

**Acceptance Scenarios**:

1. **Given** a hunter run that writes a report, **When** the document is read by
   `loadReport`, **Then** each ranked event comes back with the area the run
   settled.
2. **Given** a document at minor 1.13 or older, **When** `loadReport` reads its
   ranked events, **Then** each area stands unset and no refusal is raised.
3. **Given** the written document, **When** the schema checker reads it, **Then**
   the closed per-event object accepts `topdownArea` at its stated position and
   still refuses an unknown key.
4. **Given** a run with no `--hunt`, **When** its bytes are compared with the
   bytes of the same run at the base, **Then** the schema-version value is the
   only difference (D-12).

---

### User Story 3 - Compare a culprit with its level-1 split (Priority: P2)

A user reads the level-1 percentages of the same run under the ranked table, so
a row that names a back-end path can be weighed against the
share of the machine the back end took. On a host with no table, the `N10`
refusal shows one time and the run ends as it does today.

**Why this priority**: The comparison is the reason the area is a path. It rides
the tree the run already selects, so it costs no new counter path.

**Independent Test**: Run `--hunt` on a supported host and read the level-1
block after the ranked table. Run it with a synthetic identity that has no
table, and read one refusal line with an unchanged exit status.

**Acceptance Scenarios**:

1. **Given** a hunter run on a host `N10` supports, **When** the hunt block
   prints, **Then** the level-1 values of that run print after the ranked table.
2. **Given** a hunter run on a host `N10` refuses, **When** the hunt block
   prints, **Then** the `N10` refusal line shows one time and the exit status
   stands as it does with the option absent.
3. **Given** a hunter run with `--dry-run`, **When** the plan prints, **Then**
   every hunt plan cell stands as it does at the base, and the level-1 block answers
   the one-iteration figures that the same rule already gives every pass of a dry
   run (D-9).

---

### User Story 4 - Same column on AMD (Priority: P3)

A user on an AMD host reads the same column with AMD node names, and no Intel
name shows on an AMD row.

**Why this priority**: The AMD table exists and the rule is mechanical, so the
column is right on AMD by construction. The host is the scarce part: the AMD row
rides another lane's gap.

**Independent Test**: Synthetic seam cases for a Skylake-X identity, a Zen 4
identity, a Zen 5 identity, an unsupported AMD identity, an Arm `MIDR` and an
unknown vendor. The AMD host run rides when that lane has a gap, and its absence
is a recorded pending row.

**Acceptance Scenarios**:

1. **Given** a synthetic Zen 4 or Zen 5 identity, **When** an area settles,
   **Then** every name in the path is an AMD name the selected table publishes.
2. **Given** an unsupported identity, **When** any area settles, **Then** the
   area reads `unmapped`.
3. **Given** an Arm `MIDR`, **When** any area settles, **Then** the area reads
   `unmapped` and the tree answers the `N10` Arm refusal (D-5).

---

### Edge Cases

- An event two nodes name at one depth: the first node in table order wins (D-4).
- An event a formula names through an alias the table spells twice: only the
  spelling the formula carries reaches a node, and the other spelling answers the
  grouping of the file that holds it, or `unmapped` where no file holds it
  (AP-4).
- An event whose address names a device other than the core device: the area
  rule is the same rule, and an uncore event answers the grouping of its own
  `uncore-*.json` file, such as `uncore-cache`.
- Two devices that carry the same event name in two different files: the first
  row the listing holds wins, and the rule states that at the site (D-16).
- A kernel-discovered alias the vendored directory holds no row for: the area is
  `unmapped`, and the alias keeps its rank. AP-7 counts 27 such rows of the 497
  on this host.
- An event the run planned and the host refused: the skipped list stays as it is
  today and carries no area cell.
- A host with no table: every area is `unmapped`, the refusal shows one time, and
  no level-1 block prints.
- A hybrid Intel identity and an Arm `MIDR`: `N10` refuses both, so every area is
  `unmapped` (FR-005, D-5).
- A document at an older minor: the key is absent, the member is unset, and the
  loader raises no refusal.
- A document that carries `topdownArea` with a kind other than a string: the
  reader refuses it with its usual pointer into the document.
- A hunter run whose every event is refused (`events=0/N`): the ranked table is
  empty, so no area cell prints and the summary line stands as it does today.
- A benchmark with a hunt and no ranked row at all: no area cell prints, and the
  document carries no `topdownArea` inside an empty `entries` array.
- A run that asks for a hunt beside `--counter`, `--metric`, `--compare`,
  `--placement` or `--group-layout`: one line on `stderr` naming the value, exit
  status 2, as today.

## Requirements *(mandatory)*

### The area (A)

- **FR-001**: A pure function shall map the catalog address of one ranked event,
  the `N10` tree answer of the host and the vendor grouping of the event to that
  event's area: the deepest node whose formula names the event, else the
  grouping D-3 names, else `unmapped`. The tree it reads shall be the answer
  `topdownTree` gives for the identity of the run at the deepest ask the type
  takes, level 6, whose answer the table clamps to its own depth (D-17). The
  function shall read no counter, open no event, read
  no clock and ask no kernel interface: its answer follows from the values it is
  given. The identity shall reach it through the `N10` seam, so a test hands in
  a synthetic `CpuIdentity` (D-1, D-7).
- **FR-002**: The mapper shall compare an event name to a node's event list by
  the spelling each carries, and it shall answer no node for an event whose
  spelling a formula lacks (AP-4).
- **FR-003**: On a tie over depth, the mapper shall answer the node that stands
  first in the table order of the tree it was given (D-4).
- **FR-004**: The fallback shall name the vendored list file the selected
  directory carries the event in, as the file stem: the vendor's own file name with
  the `.json` suffix dropped, so `cache.json` answers `cache`. No other change to a
  vendor name is allowed, and the harness shall hold no list of event names (D-3,
  D-16).
- **FR-037**: The harness index shall read the listing of the accessor of D-16 after it
  strips the device prefix that D-16 names, and shall answer the group of the first row of
  a repeated name. A unit shall plant a listing that repeats a name and settle that rule in
  process, with no host claim (D-16).
- **FR-005**: Where the tree answer is unsupported, in every vendor and every
  refusal kind, the mapper shall answer `unmapped` for every address (D-5).
- **FR-006**: The mapper shall answer the same area for the same three inputs on
  every call.
- **FR-007**: The area shall be a path: the level-1 vendor-neutral name, then the
  vendor names of the nodes below it, joined by `" > "` in the spelling `N10`
  publishes. A level-1 node shall answer a one-name path (D-10).
- **FR-008**: The counters change shall stay the one accessor D-16 names: one
  header declaration, one definition in `topdown_select.cpp`, no new counters
  source file, no formula grammar, no vendor table and no change to the `N10`
  selection. The node event list shall be read through
  `counters::TopdownNode::events` alone (D-1).

### The hunt run (B)

- **FR-009**: Every record of `EntryHunt::entries` of a hunter run shall carry
  its area before the console, the document and any later reader see it, the
  ranked records and the unranked ones alike, since that list covers every
  planned column of the run. The stamp shall run after the ranks of every entry
  settle, at the end of the batch pass where the specs/040 stamps stand
  (`source/harness/cli.cpp:1987`), and before the deferred console
  (`:1563-1564`, printed at `:2036-2042`), the document writer, and the two emit
  paths (`:2182-2204`).
- **FR-010**: The area shall change no rank, no sort key, no `cv` threshold, no
  budget, no emitted set, no emitted rule and no emitted-set or emitted-rules
  byte (D-12).
- **FR-011**: The emitted lines shall be a function of `rank`, `address` and
  `delta` alone, so an area that stands or falls cannot move one byte of
  `--hunt-emit-set` or `--hunt-emit-rules`. The proof shall be a unit that settles
  areas on one `EntryHunt` and compares `toCounterSet` and `toCompareRules`
  against the same record with its areas unset, beside a diff of
  `source/harness/hunt_plan.cpp` and `source/harness/hunt_cli.cpp` that names no
  hunk at all, since those two files hold all four emit functions. No live run
  shall carry this proof: AP-10 measures that the emitted bytes of one unchanged
  binary differ from run to run.
- **FR-012**: The feature shall add no option, no short option and no
  environment variable. `source/harness/detail/options.hpp` shall stand
  unchanged (D-2).

- **FR-033**: One identity, one selection and one table read per run shall feed
  both uses, and the two asks shall differ in `TopdownSelection::level` alone
  (D-17). The level of the companion and the level of the mapper shall be two
  values held apart in one run, so the deep tree cannot answer the block and the
  level-1 tree cannot answer the area. The header of the block prints `passes=` from
  the collection that ran (`source/harness/report.cpp:365-368`), so the companion
  shall carry the pass count of its own pass, one on every host, and the entry figure
  of `include/speedgun-ng/benchmark.hpp:936` shall stand, the figure a `--topdown` run
  prints today. A unit shall plant the identity and the table on either path in process
  through `RunOptions::topdownIdentity` and
  `RunOptions::topdownTable` (`source/harness/detail/internal.hpp:759` and `:775`),
  and no public record of the harness shall gain a field for it.
- **FR-034**: When a run names `--topdown` with a level, that level shall be the
  level of the one block, and the implicit level 1 of D-9 shall apply only where no
  `--topdown` stands. The combination shall add no refusal (D-2).
- **FR-035**: Two lines of a process shall stay behind the option the user named:
  the `topdown: core-wide coverage=` line (`source/harness/cli.cpp:1584-1593`) and
  any pass disclosure of the companion's own collection. An internal bit beside
  `topdownLevel` shall tell the ask of the companion from the ask of `--topdown`, so
  a hunter run gains neither line and a `--topdown` run keeps both (D-9). The guard of
  `printTopdownRefusal` (`source/harness/cli.cpp:303`) shall widen to that bit, and the
  coverage line at `:1584` shall stay gated on `topdownLevel` alone, since the
  one-per-process rule is the single call at `:1583` and no stored state.
- **FR-036**: The companion's answer shall reach the printer through a path that leaves
  `BenchmarkResult::topdown` unset, because that field writes the document
  (`source/harness/json_report.cpp:867-869`) and feeds the pass disclosure
  (`source/harness/report.cpp:305`). A unit shall write the document of a hunter run and
  name the absence of a topdown object in it. The companion's pass shall run with the
  instance handoff clear, so neither the retried list nor the refused list prints
  (`source/harness/detail/internal.hpp:326-330`, `source/harness/runner.cpp:3076` and
  `:3112`).

### The console (C)

- **FR-013**: The ranked row shall print one `area=<path>` cell after the
  correlation, slope and delta cells and before the closing flag or mark cell of
  that row, with one leading space and no field width, in the shape of the cells
  beside it (D-13). The cell shall print where a row closes, and not inside
  `printSensitivityCells`, whose post condition names exactly one correlation,
  slope and delta cell (`source/harness/report.cpp:1179-1182`). An implementer who
  moves the cell into that function shall move the post condition with it.
- **FR-014**: Both row kinds shall print the cell when the member holds a value:
  the ranked row and the unranked row that ends with `mark=`. A record whose
  `topdownArea` stands empty prints no cell. The specs/040 cells answer the word
  `n/a` for a field a run measured and could not fill, and an empty area names
  something else: a record no hunt stamped. The rule also keeps every recorded
  block of a hand-built row byte-for-byte as it stands, since a hunt run stamps
  every record of its own list (FR-009).
- **FR-015**: The skipped rows, the plan line, the cut line, the sample line, the
  multiplex line and the summary line shall print exactly as they do at the base.
  The `events=` and `passes=` cells of the plan line read the `RunHunt` record
  (`source/harness/report.cpp:1272-1274`, filled at `source/harness/hunt.cpp:1011`),
  so they cannot move and no report clause asks whether they moved (D-9). Because the
  companion asks on a record of its own (FR-035), the entry record collects no further pass, so the pass
  disclosure, the `pass=` cells and the row figures of a hunter run stand equal
  between the base and the head. The base answers `count=12` at `--topdown=2` beside
  `count=36` at `--topdown=4` with a hunt on one benchmark, which is a base-to-base
  reading of an explicit level. The acceptance report states the disclosure line, the
  `pass=` cells and the row time of one named command at the base and at the head.
- **FR-016**: When the host supports `N10`, the hunt block shall print the
  level-1 values of the same run once for each benchmark, after that benchmark's hunt
  block closes, that is after `printHuntSkips`, through the `N10` block printer and
  its existing rules. The call at
  `source/harness/report.cpp:765` shall stand for a run with no `--hunt` and shall
  stand suppressed for a hunter run, so one block prints per benchmark either way
  (D-9).
- **FR-017**: When the host refuses `N10`, the existing refusal line shall print
  one time per process, whether the ask came from `--topdown` or from the companion
  of a hunter run, and the exit status shall stand unchanged (D-9).
- **FR-018**: A run with no `--hunt` shall print console bytes the committed
  console fixture holds, under the masking rule that entry already applies, and
  that fixture shall stand unchanged by this feature.

### The document (D)

- **FR-019**: Each ranked event of the entry hunt object shall carry
  `topdownArea` as a string, written at the position after `delta`, and the writer
  shall write the key only where the member holds a value, the rule the console
  keeps (FR-014, D-11). The header comment of `writeEntryHunt`
  (`source/harness/json_report.cpp:569-572`) states that no key of the node is ever
  absent, and that comment shall move with the rule.
- **FR-020**: `kSchemaVersion` shall stand at `1.14`, the version-history comment
  shall name the new key and this spec, and the schema document shall list
  `topdownArea` in the `x-order` and the `properties` of the closed per-event
  object and keep it out of that object's `required` list.
- **FR-021**: The reader shall read the key with the presence rule the loader
  uses for a key a later minor added: a present string keeps its value, an
  absent key leaves the member unset, and a value of another kind keeps its own
  refusal. The loader shall gain no minor-version comparison.
- **FR-022**: A report of a run with no `--hunt` shall differ from the report the
  same run wrote at the base in the schema-version value and in
  `speedgun.version`, which carries the moved project version, and in nothing else
  (D-12). The two committed fixtures of that run change by zero lines: each pins a
  stale minor of its own and no reader compares it.
- **FR-023**: Every pinned assertion of the current minor, and every comment that
  names the current minor, shall move to 1.14 in the same commit as the writer. The
  two later-minor plants stand at 1.13 today, the current minor, so they shall move
  to 1.15, one minor above the new current.

### Documentation and example (E)

- **FR-024**: `docs/pages/harness.md` shall gain the area in its hunter subsection:
  the cell, the three kinds of answer, the rule for a tie, the AMD spelling, the
  level-1 companion and the version rule for the new key.
- **FR-025**: `example/docs/hunt_topdown.cpp` shall be a runnable example with a
  marked region the guide page `docs/pages/harness.md` quotes, a row that prints, and
  its own `.expected` region of recorded output. The example shall build its rows the
  way `example/docs/hunt_rank.cpp` builds them, from records the example writes, so
  its bytes stand on any host and no host word enters the recorded region. The shape
  gate shall register that recorded region by the `hunt_rank` precedent in
  `test/CMakeLists.txt`, so a machine exists to check it.
- **FR-026**: `README.md` shall carry the new public member as one quick-reference
  row and no marked block. The gate reads the main page alone for the page cap and
  the section caps, and three shapes were measured in a copy of the tree with
  `--sections --names --options --forbidden`: a marked block in `### Hunter mode`
  beside the row reads `1020 lines, the cap is 1004` and `section 'Hunter mode' runs
  43 lines, the cap is 25`; the same block moved to `### Top-down analysis` reads
  exactly 1004 lines, and adding the row there reads 1005; the block at
  `docs/pages/harness.md` with the row alone reads 0 findings at 1001 lines.
  `SECTION_LINE_CAPS` stands unchanged, since raising a cap is a gate change that
  needs its own justification (constitution VIII).
- **FR-027**: Every recorded console block that quotes a hunt row shall be
  re-blessed in the same commit that changes the row, and every block of a run with
  no hunt shall keep its bytes. Two machines read those blocks, and they differ.
  The `area=` cell needs no new rule, because the example of FR-025 writes its own
  records, and `printHuntRanking` stands at namespace scope
  (`source/harness/report.cpp:573`) for a caller to reach. A recorded region shall hold
  no real level-1 block, so `tools/docs/report_shape.py` gains no fold rule. That printer sits inside an
  anonymous namespace (`source/harness/report.cpp:56-444`), a real block carries
  `table=arch/x86/skylakex/`, `reference=linux-7.2.4`, thirteen digests and the node
  names of the host, and those counts belong to the table, since `skylakex` prints 13
  of its 15 files while `zen5` prints 12 and `zen4` prints 10. No host-free recording
  of a real block exists, so the guide page teaches the block in prose and a unit
  proves its shape by capturing the console. A later change that puts a real block
  inside a recorded region shall add a value fold and a multiplicity fold in the shape
  of `fold_cpu_lists` and `fold_host`, and a third planted pair whose only difference
  is the count of `file` lines.

### Version, tests and discipline (F)

- **FR-028**: `CMakeLists.txt` shall state version 0.31.0, the harness
  `SOVERSION` at 21 and the archive `SOVERSION` at 6, each with the comment that
  names the record and the requirement. Both version-binding readers, the one in
  the command-line table unit and the one in the views unit, shall read the moved
  numbers in the same commit, and each shall read a `SOVERSION` property line
  beside its target, so a comment that names an old number cannot settle the
  check. Both readers hold a substring test today that a comment settles:
  `test/source/harness_cli_table_test.cpp:1455` and `:1457`, and
  `test/source/views_test.cpp:281` and `:283`. The three numbers, both readers and the
  accessor of D-16 shall stand in one commit: a move of the archive number alone makes
  `soVersionNumbers` read `6 20` and fails both readers
  (`test/source/harness_cli_table_test.cpp:1459-1461`,
  `test/source/views_test.cpp:285-287`).
- **FR-029**: Every new behavior shall carry its unit in the same commit, with
  the unit observed red at the base first. Synthetic identities shall cover
  Skylake-X, Zen 4, Zen 5, an unsupported AMD part, an Arm `MIDR` and an unknown
  vendor, including one tie and one event no node names. A path case, where a node
  names an event, needs a synthetic identity with a planted `TopdownTable`: an
  identity alone answers zero nodes and the refusals on a host of another vendor.
- **FR-030**: No new unit shall read a clock, sleep or open the kernel event
  interface. A unit shall name no device the host lacks, and a scripted seam may
  name any device, since a seam answers from its own record. The two-cpu check
  shall be `taskset -c 0,1 ctest --preset prepush-quick -j 2`, run at the base and
  at the head, and the head shall add no name to the failures the base answers.
- **FR-031**: Coverage shall read 100 percent line. The branch shortfall shall not rise
  above the figure the base answers, and `tools/dbc/coverage_gate.sh:102` carries that
  figure as `allowance=21`, which specs/031, specs/035 and specs/043 cite. Principle VI
  states 100 percent branch, so the two differ at the base and before this change. The
  change shall add no new shortfall, shall state the number it answers, and shall name
  that divergence in the acceptance report (Governance). The change shall add no
  `LCOV_EXCL`, `--omit-lines`, `--ignore-errors` or `--remove`.
- **FR-032**: ASD-STE100 Simplified Technical English shall hold in every line the
  change writes, in code, prose, docs and commits.

## Clarifications

### Session 2026-11-03, round 6

A reviewer read the byte consequences of the companion route and named four that had
no owner. Each has one now.

- Q: Where does the block stand, given one function prints the skipped rows and the
  summary line? → A: After `printHuntSkips` returns. Two callers declare that function
  by hand, so its shape stays (D-9, FR-016).
- Q: What channel carries the companion's answer? → A: A path that leaves
  `BenchmarkResult::topdown` unset, because that field writes the document and feeds the
  pass disclosure. The pass runs with the instance handoff clear (FR-036).
- Q: Does the fold rule cover a real block? → A: No recorded region holds a real block,
  so `report_shape.py` gains no rule. The recipe for a later change that adds one stands
  at FR-027.
- Q: Does a hunter run move any base byte beyond the new cells? → A: No. With the ask
  off the entry record, the disclosure, the `pass=` cells and the row time stand equal.
  The `count=12` against `count=36` pair is a base-to-base reading (FR-015).
- Q: What does the second ask cost? → A: A hunter run pays the first table read of its
  process, 503 ms measured. AP-12 states it.

### Session 2026-11-03, round 5

An artifact writer read the dry-run path and found one measured-false clause from
round 4:

- Q: Does a dry run open no counter? → A: No, and it never did. A dry run runs one
  iteration per pass: `source/harness/hunt.cpp:1050-1053` sets one repetition and the
  dry-run bit on the stage record, and `docs/pages/harness.md:2439` states the rule.
  The companion follows the rule it rides, so a hunter dry run prints the block at
  its place with one-iteration figures (US3 AC-3, D-9).
- Q: How many decisions does the spec hold? → A: Seventeen, D-1 to D-17. The plan
  record named an eighteenth and does not now.

### Session 2026-11-03, round 4

A convergence reviewer re-measured every round-3 ruling at `b1dd7670`. Four findings
were measured false or unsatisfiable, and the spec now reads:

- Q: Do the plan cells move? → A: No. `events=` and `passes=` read the `RunHunt`
  record (`source/harness/report.cpp:1272-1274`, filled at
  `source/harness/hunt.cpp:1011`), which no clause of this feature moves. What does
  move is the entry run: its pass disclosure, its `pass=` cells and its row time. The
  base answers `count=12` at one level and `count=36` at another on one benchmark
  (FR-015).
- Q: Which record carries the companion? → A: Its own internal record. The entry
  record is the process option record (`cli.cpp:756` and `:1505`), and two lines gate
  on its level: the refusal at `cli.cpp:1583` and `topdown: core-wide coverage=` at
  `cli.cpp:1584-1593`. The companion rides its own record and an internal bit, so a
  hunter run gains the refusal when its host refuses and gains no coverage line and no
  second disclosure (FR-035, D-9).
- Q: How many blocks print? → A: One per benchmark, after that benchmark's skipped
  rows and before its summary line. A run over three benchmarks prints three blocks at
  the base under `--topdown`, and the suppression rule keeps that count (D-9, FR-016).
- Q: Where does the new example live? → A: On `docs/pages/harness.md`, with one
  quick-reference row on the main page. Three shapes were measured with the four drift
  flags: a main-page block with the row reads 1020 lines and a `Hunter mode` span of
  43, the block moved to `### Top-down analysis` reads exactly 1004 and 1005 with the
  row, and the guide-page home reads 0 findings at 1001 lines (FR-025, FR-026).
- Q: Can the level-1 block be scripted out of host words? → A: No. Its printer sits in
  an anonymous namespace at `source/harness/report.cpp:56-444`, and a real block
  carries a table path, a kernel reference, thirteen digests and the node names of the
  host. The fold rule of `tools/docs/report_shape.py` is the route, with a planted
  pair that reads red without the rule and red when the block stands missing. The
  `area=` cell needs no rule, because the example writes its own records and
  `printHuntRanking` stands at namespace scope (`report.cpp:573`) (FR-027).
- Q: Which two-cpu command names the base list? → A: The preset form. It answers three
  failures over 171 entries in 680 s at the base, the three of AP-1. The earlier
  `-L quick --timeout 120` form answers five, and its own timeout would have killed
  `example_run`, which takes 168.89 s under two cpus (AP-11, FR-030, SC-005).
- Q: What else did the sweep find? → A: Six wording defects, now fixed: the Scope
  range stopped at FR-032, one amendment bullet stood twice in the Out list, `D-8`
  carried a broken sentence, two clauses cited a seam that no requirement names, the
  kernel-text claim cited `specs/012` where the rule lives in `specs/007`, and `FR-004`
  spelled the fallback as the vendor file while `D-3` spelled it as the stem. The
  stem wins: `cache.json` answers `cache` (FR-004).

### Session 2026-11-03, round 3

Four design workers read the spec against the code. Rulings:

- Q: How does the companion collect level 1? → A: In one short pass beside the hunt
  stages, through the runner `N10` already drives. No hunt stage gains an event, so
  no multiplex group, rank, budget or emitted byte moves, and the plan lines stand
  (D-9, FR-010, FR-015). The alternative that joined level 1 to the entry record was
  refused, because it can move rank order.
- Q: How does the block reach the console? → A: Through an internal channel, console
  only. No object joins the document, which keeps the areas of its own rows alone
  (D-9, D-11).
- Q: What does a `--dry-run` hunter run print? → A: Corrected in round 5 below: the
  plan as it stands at the base, and the block at its place with one-iteration
  figures.
- Q: What spelling does an area take? → A: The vendor name of each node, verbatim at
  every level, joined with `" > "` (D-10). The short form `backend_bound >
  memory_bound` in US1 would need a name cut FR-004 forbids, so US1 now carries the
  verbatim spelling.
- Q: Which page pays for the new example block? → A: Superseded below by round 4:
  neither page pays by a swap. The guide page holds the block and `README.md` carries
  one row (FR-025, FR-026).
- Q: What carries the emit proof? → A: One unit over one `EntryHunt`, plus a diff of
  `source/harness/hunt_plan.cpp` and `source/harness/hunt_cli.cpp` that names no
  hunk, since those two files hold all four emit functions (FR-011).
- Q: What do the two committed fixtures of a no-hunt run do? → A: Nothing. Each
  pins a stale minor of its own, no reader compares it, and both change by zero
  lines. The live document of such a run names two differences: the schema version
  and `speedgun.version` (D-12, FR-022).
- Q: How many interfaces do the DBC gates answer? → A: 345 each at the base, measured
  by this lane with the two hand commands of AP-2. The head grows by two: the
  accessor D-16 names and its record. The report states the number it measures
  (SC-006).
- Q: What does the deep ask of D-17 cost? → A: 503 ms for the first table read of a
  process and 0.57 ms for the second, so the level-6 ask rides a read the run pays
  for anyway (AP-12).
- Q: What prints for a member no stamp settled? → A: No cell, and no key in the
  document (FR-014, FR-019). Two workers asked for `area=n/a` beside the specs/040
  figures; the empty rule wins, because a hand-built row keeps every recorded byte
  it holds today. `example/docs/hunt_ranks.expected` stands for the same reason, and
  no page quotes that file today.

### Session 2026-11-03, round 2

Two reviewers read the spec against the code and against the gates. Rulings:

- Q: Where does the companion block print? → A: One block prints per run. The hunt
  block holds it after the ranked rows, and the call at
  `source/harness/report.cpp:765` stands for every run with no `--hunt` and stands
  suppressed for a hunter run (D-9, FR-016).
- Q: Which record carries the companion ask? → A: The entry `detail::RunOptions`,
  the record the pass-disclosure line reads. The two hunt-stage records of
  `source/harness/hunt.cpp` keep their plan bytes, so ranks, budget and emitted
  bytes cannot move (D-9, FR-010, FR-015, FR-033).
- Q: What does an explicit `--topdown` do beside `--hunt`? → A: Its level names the
  one block. The implicit level 1 applies where no `--topdown` stands, and the
  combination answers no refusal (FR-034).
- Q: What prints when `topdownArea` stands empty? → A: No cell prints and no key
  writes, because a hunt run stamps every record of its own list. The rule keeps
  every recorded block of a hand-built row unchanged, `example/docs/hunt_ranks.cpp`
  among them (FR-014, FR-019). One reviewer asked for the cell always; the empty
  rule wins on that byte cost.
- Q: Can a recorded block hold a word the host supplies? → A: Superseded below by
  round 4: the cell needs no rule and the block cannot avoid one (FR-027).
- Q: Does a pinned directory hold one event name twice? → A: Five hybrid
  directories hold one name twice inside one file under a second `Unit` device, and
  no directory holds one name in two files. `arrowlake/DL1.DIRTY_EVICTION` names the
  case the coverage of FR-031 reaches (D-16).
- Q: Which minor do the later-minor plants stand at? → A: 1.15, one minor above the
  new current. They stand at 1.13 today, the current minor (FR-023).
- Q: Does a synthetic AMD identity alone answer nodes? → A: No. It answers zero
  nodes and the refusals on this host, so a path case plants a `TopdownTable`
  beside the identity (FR-029, SC-001).
- Q: How does the grouping listing meet a hunt address? → A: The harness drops the
  device prefix, the part before the first `/`, before it reads the index, because
  the listing carries bare names (D-16, FR-004).
- Q: Does the drift gate need flags to test the page budget? → A: Yes. The bare
  invocation checks the blocks alone, and `--sections --names --options --forbidden`
  reach the page budget, the section spans, the API table and the option table
  (SC-006).
- Q: Do the two no-hunt fixtures move for the schema line? → A: No. Their own
  masking drops the version value, so both fixtures stand unchanged and one unit
  names the value the writer emits (D-12, FR-018, SC-003).
- Q: Can the main page pay for a marked block today? → A: No. The drift gate answers
  1011 lines against a cap of 1004 and a `Hunter mode` span of 35 against 25 for
  that edit, so the plan chooses among the harness page, a named trim, and a
  per-section cap carrying the written justification a gate change demands (AP-5,
  FR-026).
- Q: Does the request's read-only clause still hold? → A: D-16 amends it at a
  stated cost, and the Scope section carries the amendment for the pull request.
- Q: Which basis does SC-007 score? → A: The printed rows of AP-9, where this host
  answers 58 paths, 412 groupings and 27 unmapped. The 109 of AP-7 counts vendored
  formula rows, published here or not (AP-7).

### Session 2026-11-03, round 1

- Q: At which level does the mapper ask its tree? → A: At the deepest ask the
  selection type takes, level 6, which every table clamps to its own depth. The
  companion prints level 1, so one run makes two asks (D-17, AP-9). The level-1
  ask alone settles a path for 7 of the 497 addresses of AP-3, where the deep ask
  settles 58.
- Q: Can the emitted bytes of a run prove FR-011? → A: No. One unchanged binary
  answers different emit bytes from run to run, because the emitted set follows the
  measured ranks, and a non-zero `--hunt-seed` aborts at the base at
  `source/counters/fold.cpp:166`. The proof moves to a unit over one `EntryHunt`
  and to a diff that names no hunk in the four emit functions (AP-10, FR-011,
  SC-008). The abort is a base defect this feature names and does not carry.
- Q: Does the byte-identity fixture machine prove the schema line? → A: It proves
  everything except that line, because its masking rule drops the schema version,
  the `speedgun` object, `overheadFloorNs`, the banner and the overhead figures.
  The claim stands as the fixture diff plus the masked identity (D-12, SC-003).
- Q: Does the two-cpu suite pass at the base? → A: No. The lane's command answers
  5 failures over 171 entries at `b1dd7670`, three of them the AP-1 files, one a
  10 second timing bound that passes alone, and one recorded block that carries cpu
  letters a wider host printed. The clause is a base-to-head diff, and the head
  adds no name (AP-11, FR-030, SC-005).
- Q: How many interfaces do the DBC gates count at the base? → A: 345, each gate,
  run by hand with `/usr/bin/python3` (AP-2). Round 1 stated one new interface; a
  later run counted two, the accessor and its record, and round 3 keeps that count.
- Q: Is the prose gate green at the branch point? → A: It answers void: the
  resolved range is degenerate while base equals head. Every prose claim rides
  `--base b1dd7670`, `--check commit` per commit, and the tree form for untracked
  files (AP-2b, SC-006).
- Q: Can this host print a coverage ratio? → A: No. `lcov` dies in its own perl and
  the gate answers exit 3, the tool-failure exit. The ratio comes from the coverage
  job of the pull request, and a `gcov`-based read rides the report as a labeled
  second opinion (AP-6, SC-004).
- Q: Does the level-1 companion change the hunt plan? → A: On a supported host it
  may, and that is the request's own instruction. The plan keeps its shape and
  every hunt cell, and its two counting figures follow the plan the run executes.
  The report states both numbers with and without the companion (FR-015, US3 AC-3).
- Q: Does an `area=` word inside a recorded block put the shape gate red on another
  host? → A: It would, since that gate folds digits and reads letters, so FR-027
  requires the masking that gate already applies to stand over the new cell.

### Session 2026-11-03, round 0

- Q: Does the vendored event list carry a `Topic` or `category` field, as Q-3 of
  the request assumes? → A: No. Counted over the pinned tree: 542 JSON files
  parse and no row of any file carries a `Topic`, `Category` or `Layer` key;
  `MetricGroup` stands on metric rows alone. The vendor's own grouping is the
  file an event row stands in, and `PmuTableEntry::fileName` holds it. The
  fallback names that file stem verbatim, so no word is invented and no event
  list is written by hand (D-3, D-16, AP-7).
- Q: Can the harness read that grouping today? → A: No. `pmuLoadTable` and
  `PmuTableEntry` live in the private counters seam
  `source/counters/detail/pmu.hpp`, and no `source/harness/` file includes it.
  One read-only accessor joins the counters public surface (D-16), and that is
  the only counters change in this feature.
- Q: Does a hunter run already collect the level-1 formulas? → A: No. The hunt
  builds its own `detail::RunOptions` twice, at `measurePassNs` and at the
  per-stage loop, and neither names `topdownLevel`, so the run collects no
  top-down row (AP-8). FR-005 asks those two records for level 1 and reads
  `BenchmarkResult::topdown` back.
- Q: Do the two `RunOptions` of a hunt run and the level-1 formulas share one
  collection plan? → A: That is the plan's to settle, and the constraint it must
  obey is FR-010: no rank, no budget, no emitted byte and no pass line of the
  hunt may move. Two shapes satisfy FR-005: the level-1 leaves ride the plan of
  the run itself, or a short level-1 collection runs beside the hunt stages
  through the existing runner. The plan names the shape it takes and records the
  reason.
- Q: Which of the three kinds of area does a real run answer? → A: Measured over
  AP-3's 497 addresses: 109 are named by a top-down formula, 361 answer their
  list grouping, and 27 answer `unmapped` (AP-7).
- Q: Does an alias the vendored directory holds no row for reach an area? → A: It
  answers `unmapped`. The alias keeps its rank and its rate, and the mapper
  invents no grouping for a name the list does not hold (FR-004).

## Key Entities

- **Area**: the part of the pipeline one ranked event measures. It is a path, a
  list grouping, or `unmapped`. It follows from the address and the host's tree,
  and it holds no measurement of its own.
- **Area path**: the level-1 vendor-neutral name with the vendor names below it,
  joined by `" > "` (D-10). Its length is the depth of the chosen node.
- **Deepest node**: the node of the host's tree, farthest from level 1, whose
  formula names the event. Ties settle by table order (D-4).
- **List grouping**: the file the vendor's own list carries an event in, inside
  the directory the host selects. It is the fallback area, spelled as the vendor
  spells it (D-3). Its vocabulary is per directory: `cache` on Skylake-X,
  `l2-cache` on Zen 5.
- **Unmapped**: the answer for an event no node names and no row of the selected
  directory names, and for every event of a host the tree refuses.
- **Level-1 companion**: the level-1 block of the same hunter run, printed after
  the ranked rows through the `N10` printer (D-9).
- **Area stamp**: the pass that settles `topdownArea` for every ranked event of
  every entry of a run, after the ranks settle.
- **Tree answer**: the `N10` result for the identity of the run: a tree with its
  nodes and refusals, or an unsupported answer with its message.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Cases of a synthetic identity beside a planted table answer the area
  of fixed events for Skylake-X, Zen 4, Zen 5, an unsupported AMD part, an Arm
  `MIDR` and an unknown vendor: 6 identities, one tie case, one event no node
  names, and every arm of the mapper reached. The same identities with no planted
  table answer the refusals and the grouping listing.
- **SC-002**: A hunter run writes a document, `loadReport` returns every area the
  run settled, and a document at minor 1.13 or older loads with every area unset.
- **SC-003**: The two committed fixtures of a run with no `--hunt` stand
  unchanged, `no-hunt-bytes` of `test/source/harness_hunt_json_test.cpp:2065` stays
  green under its own masking rule over them, and one unit names the written schema
  value 1.14 beside the two version-binding readers (D-12). Only that unit reads the
  two fixtures: `test/source/harness_hunt_report_test.cpp` names no fixture
  directory.
- **SC-004**: `cmake --preset=ci-coverage`, `cmake --build build/coverage`,
  `ctest --test-dir build/coverage -j 1 --output-on-failure --no-tests=error` and
  `cmake --build build/coverage -t coverage` run, and the ratio reads 100 percent
  line with a branch shortfall of 21 or lower, the number stated, with no new
  exclusion marker in the diff. On this host `tools/dbc/coverage_gate.sh` answers
  exit 3, the tool-failure exit (AP-6), so the ratio comes from the coverage job of
  the pull request, and the report names the local exit 3 with its line. A
  `gcov`-based read of the same `.gcda` set rides the report as a second opinion
  and is labeled as that.
- **SC-005**: `ctest --test-dir build -j 24 --output-on-failure` adds no name to
  the three failures of AP-1, and the two-cpu command of FR-030 adds no name to the
  three the base answers under it (AP-11). Both run at `b1dd7670` and at the head,
  and the report carries both lists. No new unit reads a clock, sleeps or opens the
  kernel event interface.
- **SC-006**: `cmake -P cmake/lint.cmake`, `cmake -P cmake/spell.cmake` and
  `python3 tools/docs/example_drift.py --sections --names --options --forbidden .`
  answer 0; the bare invocation checks neither the page budget nor a quick-reference
  row. The aggregate local gate
  `tools/ci/prepush.sh --full` answers 0, and `tools/ci/prepush.sh --self-check`
  answers 0. The prose gate answers 0 findings
  over `--base b1dd7670`, over `--check commit` for every commit of the range, and
  over the tree form for the new files (AP-2b). The two DBC gates answer 0 gaps
  each, and their interface count grows by two: one row for the record D-16 names and
  one for the accessor, since the counter weighs each public struct and each non-exempt
  function (`tools/dbc/dbc_doc_gate.py:511-537`). The base answers 345 at both gates and
  the head answers 347, and the acceptance report states both.
- **SC-007**: On `corsairone`, `--hunt` on one compute-bound and one memory-bound
  benchmark prints the area cell on every ranked row, prints the level-1 block,
  and the acceptance report gives the three counts: node-mapped, list-group,
  unmapped. AP-9 states the split this host answers today, 58, 412 and 27 over
  the 497 printed rows at the level D-17 asks, and a report that reads far from
  those figures is read as a defect of the mapper or of the host record.
- **SC-008**: The emit unit of FR-011 answers the same strings with the areas
  settled and unsettled, and the diff of the four emit functions names no hunk.
  The acceptance report states AP-10, so a reader knows a live byte comparison was
  refused as a control that cannot hold.
- **SC-009**: On an AMD host with a gap in its lane, the same run names AMD nodes
  and no Intel node. Measured on AMD Zen 5 on 2026-11-05 at `858c55d9`
  (`ryzen 9 9950x3d`, family 26, model 68): the run selected
  `arch/x86/amdzen5/`, named 11 AMD nodes, named 334 listing groups, named 8
  `unmapped` ranks, and named no Intel node — the string `intel` appears 0 times
  in the logs. See `results/acceptance-amd-zen5.md`. The one reading that host
  did not answer is the command as `results/acceptance-hunt.md` writes it: that
  shape aborts in the pass planner and the fold on that host, so the run needed
  `--metric-method multiplex` to reach its rows. That abort is a defect of
  specs/024's ratio pair, not of this feature, and it stands as its own open row
  in the same record. AMD Zen 4 stays a pending row, with no hardware claim for
  it.
- **SC-010**: The parts with no host (Intel parts measured through
  `PERF_METRICS`, Intel hybrid, Arm, AMD Zen 4 and Zen 5 while no lane has a gap)
  stand as synthetic-test rows, and the report of the feature names them as
  pending rows.

## Constraints

- Constitution I, II, VI, VII, VIII, X and XI bind in full. 100 percent line,
  branch and DBC coverage, one contract per interface, no exclusion marker.
- ASD-STE100 Simplified Technical English in every generated line, the prose gate
  included, and the commit template of Pull Request Quality.
- Counters-only time and counters: the harness side reads time and counts only
  through plans, recorders and folds of `sg::counters`. The area reads neither.
- No new vendored data, no new runtime dependency, no path lookup at run time.
- Options structs carry every new parameter: no new bool parameter. JSON only
  through the JSON libraries the tree already uses.
- A bad value exits with status 2 and one quoted line, as the harness rule stands
  today. No new option adds a new bad value here.
- Rebase only, no merge commits, no CI change. First push normal, later pushes
  `--force-with-lease=046-hunter-topdown:<sha last pushed>`, then
  `git ls-remote origin 046-hunter-topdown` equals `git rev-parse HEAD`.
- Shared files with every lane, each a rebase point: the version and `SOVERSION`
  lines of `CMakeLists.txt`, `README.md`, `example/CMakeLists.txt`,
  `test/CMakeLists.txt`, `docs/schema/report-v1.json` and
  `source/harness/detail/options.hpp`.
- Files of the 045 lane stay untouched: `source/counters/linux_pmu/kernel_calls.*`,
  `group_io.*`, `provider.*`, `include/speedgun-ng/counters_provider.hpp`,
  `test/source/counters_pmu_provider_test.cpp`,
  `test/source/counters_kernel_calls_test.cpp`,
  `source/harness/jitter_probe.cpp`, the FWQ and FTQ files,
  `test/prose-gate-fixture/` and `specs/043-fwq-ftq/`.

## Scope

In: FR-001 to FR-037. The pure mapper with its header, the area stamp in the
batch stamping pass, the `RankedEvent` member, the console cell, the level-1
companion of a hunter run, the JSON key, the reader, the schema, the version and
`SOVERSION` moves, the docs subsection, the runnable example with its recorded
output, the quick-reference row, the fold rule the shape gate needs, and the units. Inside the counters library the
change is D-16 and no more: the declaration in
`include/speedgun-ng/counters_topdown.hpp`, the definition in
`source/counters/linux_pmu/topdown_select.cpp`, and the units of both.

Out, each with its owner:

- Any counters change beyond D-16: none. `topdown_select.*` keeps its selection
  logic, and no counters file of the 045 lane is touched.
- A `Topic` or `category` field for events: the pinned data holds none (AP-7),
  and this feature invents none. A vendor field that arrives upstream later is a
  new ingestion item that belongs to another lane.
- An option or an off switch: none (D-2).
- Rule forms over areas, and any use of the area inside `--compare-rules`: none
  (D-6).
- Threshold evaluation, new node names, new vendor tables, Arm tables: the `N10`
  lane (042) and its later items.
- The two `SOVERSION` questions of the 042 lane: the owner.
- The 045 lane's files and the 047 lane's functions: their lanes.

**Amendment the request implies, in scope.** The request allowed read-only use of
`include/speedgun-ng/counters_topdown.hpp`, and D-16 adds one public read-only
accessor there. The cost is one declaration, one definition, and archive SOVERSION 5
to 6. The refused alternative was a second vendored-json reader inside the harness,
parsing again data the library already parses (AP-7). The pull request body names the
amendment.

## Assumptions

- The owner's request of 2026-11-03 is the authority for scope, numbering and the
  accepted answers. The roadmap file it names sits outside this repository and is
  absent from this machine.
- The acceptance host is this machine: `corsairone`, an Intel Skylake-X part, one
  socket. The two AMD hosts belong to other lanes and this feature cannot
  schedule them.
- The area of an event whose address names a non-core device follows the same
  rule as a core event, and no device-specific list stands in the way.
- `N10` keeps the shape it has at the base: `TopdownNode::events`, the printer,
  the refusal message and the selection seam. A change to that shape in another
  lane is a rebase point for this feature.
- The vendored list groups its events by file and holds no category field, so
  the grouping of D-3 is the only vendor-native category in the pinned data
  (AP-7).
- The counters library can read the vendored row of an event from build-time
  data for any identity the mapping file selects, so the D-16 accessor answers
  the same listing on every host and needs no probe.
- The 042 `SOVERSION` question stays open at branch time. The rise to 21 counts
  from the master value, as the request states.

## Budget note

Measured at the audit point, before any section was written (AP-5).

- Page: `README.md` measures 1000 lines and `PAGE_LINE_CAP` is 1004
  (`tools/docs/example_drift.py:256`). Four lines are free. Eight further lines
  are payable by the trims the memory of 2026-11-02 names: blank lines directly
  above a heading, with the two protected blanks kept.
- Section: `SECTION_LINE_CAP` is 25 (`tools/docs/example_drift.py:257`).
  `### Hunter mode` holds 24 lines and `### Top-down analysis` holds 21. A block
  for the new example fits either owner only with a trim inside the same section,
  and the choice of owner section is a plan decision the drift gate settles.
- Lists: `SECTIONS` and `SECTION_OWNERS` move together, or the drift tool raises a
  `KeyError`. `NAME_SCAN_HEADERS` already lists `hunt.hpp`, so a quick-reference
  row for the new public member needs no new list entry.
- Example: `example/CMakeLists.txt` gains one `add_docs_example` line beside the
  source file it names, because the run gate fails a docs example with no built
  binary (`test/example_run.sh`).
- Pins: the minor move touches the assertion sites and the comment lines the
  memory of 2026-11-02 counts, and both version-binding readers.

## Dependencies

- specs/029-hunter-core: the hunt run, the ranked record and the console block.
- specs/033-hunter-budgets: the plan line, the budget cells and the emitted set.
- specs/040-hunter-sensitivity: the correlation, slope and delta cells, the delta
  rank and the batch stamping pass this feature joins.
- specs/042-topdown: the tree, the node event list, the printer, the refusal
  message and the identity seam. Read-only (D-1).
- specs/023-json-reports: the writer, the reader, the schema document and the
  version rule.
- specs/024-multipass-collection: the pass planner the level-1 formulas ride.
- specs/007-counters-and-timers: the fake provider every unit drives.
- specs/018-docs-main-page: the drift gate, the run gate and the page budget.
- specs/015-benchmark-harness-core: the run spine and the status-2 rule.

## Citations

- `include/speedgun-ng/hunt.hpp`: `RankedEvent` (454-472), `EntryHunt` (482-532),
  `toCounterSet` (507), `toCompareRules` (530).
- `include/speedgun-ng/counters_topdown.hpp`: `CpuIdentity` (62), `TopdownNode`
  (232-241) with `events` (239), `TopdownTree` (296), `TopdownUnsupported`
  (356-378), `topdownTree` (436).
- `source/harness/hunt_plan.cpp`: `rankEvents` (281-434), `toCounterSet`
  (440-467), `toCompareRules` (469-501).
- `source/harness/report.cpp`: `printSensitivityCells` (1183-1200),
  `printHuntRanking` (1336-1366), `printTopdownBlock` (359-442), `printResult`
  (578, 812-816).
- `source/harness/cli.cpp`: `printTopdownRefusal` (1583), the coverage line
  (1584-1593), the option record (756) and its hand to the entry runner (1505), the
  per-entry hunt (1757-1800), the batch stamping pass (1943-1996).
- `source/harness/hunt_cli.cpp`: `validateHuntCombination` (767-851),
  `writeHuntEmitSet` (859 and below), `writeHuntEmitRules` (903 and below).
- `source/harness/json_report.cpp`: `kSchemaVersion` (47), the version history
  (31-46), `writeEntryHunt` (573-620). `source/harness/json_load.cpp`:
  `readRankedEvent` (1884-1964), the optional-key shape (1935-1962),
  `checkNode` (2935-3027), `loadReport` (3101-3247).
- `docs/schema/report-v1.json`: the closed per-event object (948-977), the entry
  object (920-995), `schemaVersion` (11-13).
- `source/harness/detail/internal.hpp`: `RunOptions::topdownLevel` (751),
  `topdownTable` (759), `topdownIdentity` (775), `printResult` (1342-1345).
- `tools/docs/example_drift.py`: `SECTIONS` (34-71), `SECTION_OWNERS` (76-113),
  `PAGE_LINE_CAP` (256), `SECTION_LINE_CAP` (257), `NAME_SCAN_HEADERS` (546-555).
- `tools/dbc/coverage_gate.sh`: the line gate, the ledger and `allowance=21`
  (102).
- `source/counters/detail/pmu.hpp`: `PmuTableEntry::fileName` (80),
  `pmuLoadTable` (127-128). `source/counters/linux_pmu/topdown_select.hpp`: the
  private surface (38) the D-16 accessor joins.
- `source/harness/hunt.cpp`: `measurePassNs` (419-427) and the per-stage loop
  (1040-1069), each of which builds a `detail::RunOptions` that names no
  `topdownLevel` today (D-9).

## Open questions

None open. The six questions of the request stand answered by D-1 to D-6, and the
two bindings the request leaves to the owner (the 042 `SOVERSION` question and the
AMD host rows) are recorded as such.

## Constitution Checklist

- Main-page impact check (IV.1): YES. The feature changes what a hunter run
  prints and what its document carries, so FR-024 to FR-027 carry its page block,
  its runnable `example/` region, its recorded output, its quick-reference row and
  its guide-page subsection in the same change, inside the budget AP-5 and the
  Budget note record.
- Coverage (VI): FR-031 holds the 100 percent line gate, the allowance at 21 and
  the no-exclusion rule.
- Contract (II): every new interface documents preconditions, postconditions and
  invariants with doxygen `\pre`, `\post` and `\invariant` in its header, and its
  implementation enforces the same contracts in the same commit. A pure-data
  record takes `none` on all three tags, as the records beside it do.
- Anti-slop (X): the decisions above record each reading a requirement admits,
  and the Scope section names what this feature will not build.


---

## Acceptance extension: Intel final acceptance (2026-11-07)

`specs/048-hwlat-probe` carries the identity, the changed-file list, the shared
constraints and the clarification rounds for this phase. This section adds the
hunter part and nothing else.

### User Story 2 - A reader can verify the Intel mapping on real hardware (Priority: P1)

A reader wants to see the Intel top-down mapping and the probe and decode
equality on an Intel part, and wants the record to say which claims come from a
run and which come from a fixture.

**Why this priority**: the standing record reads from an AMD host and from
synthetic tables, and the Intel rows of SC-010 stayed pending.

**Independent Test**: the record names the hunt commands, the ranked counts, the
area paths, the level-1 block and the table the mapping came from, and maps each
reading to an acceptance criterion of this spec.

### Requirements - the hunter part (IA46-001 to IA46-002)

- **IA46-001**: The lane SHALL run the commands of
  `specs/046-hunter-topdown/results/acceptance-hunt.md` at the measured head,
  three times each as the acceptance extension of
  `specs/048-hwlat-probe/spec.md` rules, and SHALL NOT build an older head. The lane SHALL keep the normal method, and where
  it fails the lane SHALL record the failure before it tries an alternative that
  stands labeled apart.
- **IA46-002**: The record SHALL give the ranked events, the mapped and unmapped
  counts, the Intel area paths, the level-1 companion block, its source and the
  pass-method disclosure. The lane SHALL compare each figure with the vendored
  Skylake-X table, SHALL require no level that table does not carry, and SHALL
  report no AMD, Arm or hybrid hardware result as an Intel result. The lane SHALL
  map each reading to an existing acceptance criterion. No row stands complete
  without output.
