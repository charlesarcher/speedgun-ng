# Phase 0 Research: Vendor-neutral top-down analysis

Feature and branch: 042-topdown. Entries give Decision, Rationale, and
Alternatives considered, in the shape
`specs/024-multipass-collection/research.md` uses. Host, measured here:
`hostname` answers `CorsairOne`, `uname -r` answers `7.3.0-4-cachyos`, `nproc`
answers 24, `lscpu` answers `GenuineIntel`, i9-9920X, stepping 4, 12 cores, 2
threads per core; `/proc/cpuinfo` answers `cpu family: 6`, `model: 85`.

## D-01 - The selection seam and the identity record

**Decision**: `include/speedgun-ng/counters_topdown.hpp` publishes
`CpuIdentity` (vendor, family, model, stepping, a core-type string, an
`std::optional<std::uint32_t>` MIDR), a reader `cpuIdentity()`, a row reader,
and one selection entry point taking the identity, the asked level and an
optional row set. Selection reads no CPUID, counter, clock, or event (FR-002,
FR-003, spec D-13). Production fills the four fields of `pmuIdentCurrent()`
(`source/counters/detail/pmu.hpp:84`) and leaves the core type empty and the
MIDR unset; a test supplies all three.

**Rationale**: The identity is the input a test must bend: this host answers
one identity and FR-004 to FR-006 need four more. `PmuIdent` is private
(`pmu.hpp:62-72`) and its reader caches in a static, so the public record
copies four fields and adds two absent ones.

**Alternatives considered**: (a) Extend `PmuIdent` with the two fields.
Rejected: it is the encoder's and the matcher's shared record, and fields no
production reader fills put test state in a hot struct. (b) Grow a MIDR or
core-type reader now. Rejected: `external/pmu-events/RECORD:7` excludes
non-x86 tables and spec D-7 defers Arm. (c) A public signature over
`PmuIdent`. Rejected: it exports a private header.

## D-02 - Level and parent rules over both vendor spellings

**Decision**: Split the group text on `;`. A row is a node exactly when one
token matches `TopdownL([1-6])` or `PipelineL([12])`, and that digit is its
level (FR-014). Its parent is the remaining token ending in `_group` whose
stem is the `MetricName` of another row of the same directory; no such token
means no parent, the state of a level-1 root (FR-013).

**Rationale**: One token rule covers both vendors. Measured with a `python3`
walk of `external/pmu-events/arch/x86/skylakex/skx-metrics.json`: 275 rows; 98
of the 102 level rows carry one resolvable parent token and the 4 level-1 rows
carry none, since their `tma_L1_group` stem names no row. Measured the same
way over `arch/x86/amdzen4/pipeline.json` and
`arch/x86/amdzen5/pipeline.json`: 14 rows each, 5 `PipelineL1` rows with no
parent, 8 `PipelineL2` rows shaped `PipelineL2;frontend_bound_group` with one
parent each, and the group-free helper `total_dispatch_slots`, which the level
rule drops.

**Alternatives considered**: (a) Match the `tma_` prefix alone. Rejected: the
AMD files spell the token without it, measured above. (b) Derive the level
from a name or the parent chain. Rejected: FR-014 names the group text alone.

## D-03 - The hybrid predicate

**Decision**: Refuse as hybrid when the selected directory publishes rows of
two core classes, read from each row's own `Unit` text: `PmuTableEntry::unit`
(`pmu.hpp:45`), filled at `source/counters/linux_pmu/table_parse.cpp:419` for
a metric row and `:515` for an event row, and carried as
`ParsedMetricRow::scope`
(`source/counters/linux_pmu/metric_formula.hpp:107-114`).

**Rationale**: FR-006 states the rule over published scopes. Measured with a
`python3` walk of `external/pmu-events/arch/x86`: `cpu_atom` and `cpu_core`
both appear (walk of `arch/x86`) in `alderlake`, `arrowlake`, `lunarlake`,
`meteorlake` and `pantherlake`; the first four publish metric rows of both
classes and `pantherlake` carries both on event rows.

**Alternatives considered**: (a) The counters brief's closed set of four
names. Rejected: it omits `pantherlake`, which
`external/pmu-events/arch/x86/mapfile.csv:29` selects for models `CC` and
`D5`, and `specs/042-topdown/spec.md:815-817` lists five. (b) A list of five
names. Rejected: a new hybrid directory would answer a tree while its own rows
state two classes.

## D-04 - Threshold and refusal ride the existing carriers

**Decision**: `MetricThreshold` gains one field per carrier and nothing
evaluates it: `PmuTableEntry` (`pmu.hpp:35-58`), `addMetricEntry`
(`table_parse.cpp:413-433`), `ParsedMetricRow` and `MetricDefinition`
(`metric_formula.hpp:110-121`, `:171`), the copy in `Resolver::publish`
(`source/counters/linux_pmu/metric_formula.cpp:1010-1054`), `MetricView` and
`MetricListing` (`include/speedgun-ng/counters_metrics.hpp:63-68`, `:91-104`),
and the tree's row and node. Refusals reuse `MetricListing::refusal` with
`.reason` (`counters_metrics.hpp:101-103`) and the nine words of
`kMetricRefusalNames`
(`source/counters/linux_pmu/metric_formula.cpp:1630-1640`). Kinds stay nine
and the loader stays untouched (spec D-20), and the selection reads the whole
listing (`counters_metrics.hpp:113-114`).

**Rationale**: Measured with a `python3` walk of `skx-metrics.json`: all 102
top-down rows carry the threshold key, 80 with `&`, one with `|`, 21 plain,
and `parseFormula` (`metric_formula.hpp:102-103`) reads a comparison only
inside a conditional test, so spec D-9 holds and the text rides. Measured with
`./build/example/first_benchmark --catalog` (24999 lines): 279 `mode=metric`
rows, 150 under `machine` and 129 under another object; of the 102 level rows
33 publish and 69 refuse (39 `#core_wide`, 9 `#SMT_on`, 20 an unreachable
reference, 1 `UNSUPPORTED`); levels 3 to 6 print under two objects each
(15/10, 21/13, 10/7, 11/3), so level 4 is the test level.

**Alternatives considered**: A tenth refusal kind, or a loader change.
Rejected: spec D-20, and every arm of `Resolver::publish` already answers a
verdict.

## D-05 - The source record is build-time data

**Decision**: `cmake/embed_pmu_blob.cmake` takes the RECORD path and the
reference `CMakeLists.txt:961` already asserts and compares at `:980`; the
index line at `cmake/embed_pmu_blob.cmake:55` gains the hash and a file with
no hash line is fatal. `EmbeddedFile`
(`source/counters/linux_pmu/embedded_tables.hpp:23-28`) gains `sha256`,
`EmbeddedDir` (`:33-40`) gains `reference`, and a lookup beside
`embeddedFindDir` (`:49-50`) answers it. `pmuLoadTable` already walks
`dir->files[i].name` (`table_parse.cpp:797-803`), so it tags each entry it
appends and the record lists the distinct files of the selected rows in embed
order (`embed_pmu_blob.cmake:41-42`).

**Rationale**: FR-015 names one entry per contributing file; spec D-12 bars a
run-time path read and a retyped hash. Measured:
`head -9 external/pmu-events/RECORD` answers `ref: linux-7.2.4` at line 1,
`----` at line 8 and `<sha256><two spaces><path>` from line 9; a `python3`
walk answers 15 JSON files in `arch/x86/skylakex/` with all 102 top-down rows
in `skx-metrics.json`, and 14 rows each in `arch/x86/amdzen4/pipeline.json`
and `arch/x86/amdzen5/pipeline.json`.

**Alternatives considered**: (a) Open `RECORD` at run time. Rejected: spec
D-12. (b) One hash per directory. Rejected: FR-015 names the file that holds
the rows, and the library would name no source, the gap PC-12 records.

## D-06 - `#core_wide` is a coverage flag, read from the run mask

**Decision**: A fifth `HostFact` row `core_wide` in `kHostFacts`
(`source/counters/linux_pmu/metric_tables.hpp:161-166`), over the standing
reader type `bool(System&, double&)` (`:93-97`). It answers 1 when every core
touched by the calling thread's affinity mask is covered with all its siblings
and 0 when a touched core is partial, reading the mask with
`sched_getaffinity` and each sibling list through the reader and seam of D-07.
An unreadable mask or list answers nothing, which keeps the standing refusal
text (`metric_formula.cpp:1313-1317`); `#num_dies` and `#has_pmem` keep their
refusal (FR-043).

**Rationale**: The flag settles a branch and carries no width: in
`skx-metrics.json` line 853 holds `if #core_wide < 1 else` and line 1461
spells `4 * tma_info_core_core_clks`, and
`grep -rln core_wide external/pmu-events/arch/x86 | wc -l` answers 12 files.
The value settles at load, because a conditional test settles then and the
table caches per directory (`metric_formula.cpp:1492-1511`), so the reader
asks the thread; the harness narrows the calling thread's affinity for the
instance before `runner.run` (`source/harness/cli.cpp:1504-1544`), so the mask
is the run's target set.

**Alternatives considered**: The counters brief's table answering 0 for the
twelve directories that name the constant. Rejected: FR-042 wants both answers
and a seam test that drives them, and a constant never answers true.

## D-07 - The SMT fallback and the root seam it needs

**Decision**: `readSmtState` (`metric_tables.hpp:144-153`) keeps the object
tree first. When the tree answers no core object it calls one new function
over a root parameter: read `<root>/cpu<N>/topology/core_cpus_list`, fall back
to `thread_siblings_list`, answer 1 when the list holds `,` or `-` and 0 when
it holds one id; an unreadable list answers nothing. `N` comes from
`::sched_getcpu()` (`source/counters/linux_pmu/fast_read.cpp:279`) and -1
answers nothing, so no cpu is invented (FR-048). The seam is the harness
header moved down to `source/detail/proc_root.hpp`
(`source/harness/detail/proc_root.hpp:35`, `:52`) and included from both
layers.

**Rationale**: Two counts that answer decide the value and the file stays
closed, and the file answers only when the tree holds no core object.
Measured: `cat /sys/devices/system/cpu/cpu0/topology/core_cpus_list` answers
`0,12`, `thread_siblings_list` answers `0,12`, and my `--catalog` run prints
`this host publishes no value for the constant '#SMT_on'` on 9 of the 102
level rows. The counters archive sits below the harness, so it cannot include
a harness header, and one root with one install serves both layers.

**Alternatives considered**: (a) A topology provider seeding package, core and
thread objects. Rejected: spec D-19 assigns it to a system-tree feature and it
reaches harness runs only. (b) Read `smt/active`. Rejected: FR-054 names the
sibling list of one cpu.

## D-08 - The fast-path bound takes the fixed-counter mark

**Decision**: Mask the mark inside `fastIndexValid` (`fast_read.cpp:48-51`):
bound `index & ~kFixedCounterIndexMark`, keep the operand `_rdpmc(index - 1)`
unchanged (`:300-303`), keep the refusal text naming the published index
(`:139-143`), and correct the comment that claims the gate bounds the operand
alone (`pmu.hpp:284-289`). The constant stands beside `kRnpmcMaxIndex`
(`:289`).

**Rationale**: Measured symptom:
`./build/test/counters_pmu_test | grep -i "zero count"` answers
`zero count: cpu/cpu-cycles mode 1 value 0.000`, set by a non-OK verdict with
the gap flag (`source/counters/linux_pmu/group_io.cpp:435-438`). Measured
cause: `/tmp/n42/pc7-index-probe` answers
`cpu-cycles (fixed 1) index=0x40000002 cap_user_rdpmc=1 pmc_width=48` and
`cache-references (gp) index=0x00000001`, so the mark rides fixed spellings
alone. The mask keeps the bound over counter numbers and leaves the bit-29
mark refused, which is today's answer. The seam opens no event
(`test/source/counters_linux_pmu_seam_test.cpp:384-400`): `0x40000002`
publishes, `0` and one over the bound refuse, and a `fastDecode` at the marked
index answers OK.

**Alternatives considered**: Widen the bound, by raising `kRnpmcMaxIndex` or
accepting an index up to the mark plus the bound. Rejected: the gate guards
the `_rdpmc` operand, so an unmarked index of 1073742850 would reach an
operand no host answers, a fault where the present answer is a refusal.

## D-09 - One PMU provider per process

**Decision**: At `cli.cpp:579`, register the PMU provider once per process
behind a function-local static, beside the clock registration at `:566`. A
second entry-point call builds no provider and pays no device walk (FR-055,
spec D-21). The counters rule stands untouched: a provider cannot register
after the system opens (`source/counters/system.cpp:216-223`).

**Rationale**: Measured here: three runs of
`./build/example/first_benchmark --version` take 1.741, 1.729 and 1.727 s, and
that answer leaves at `cli.cpp:597`, after the scan at `:586`, so the time is
the two registrations alone. PC-8 measures the same cost per in-process call
and about 63 minutes for 2,040 calls, which is why `harness_cli_test` cannot
finish inside the default 300 s (`test/CMakeLists.txt:2121-2128`).

**Alternatives considered**: (a) Process-static PMU state in the counters
library. Rejected: spec D-21 gives it to the counters lane and it changes
every caller's cost. (b) Raise the test timeout. Rejected: it hides 63 minutes
and leaves FR-049's `taskset -c 0,1` run unfinishable.

## D-10 - One option row, its own scan arm, and the bare value

**Decision**: `--topdown` joins the option table at internal value 350
(`source/harness/detail/options.hpp:97` holds 340 last) as an optional
argument appended after the hunt rows (`source/harness/options.cpp:765-771`),
and takes its own arm in the scan switch beside `case kHuntOption:`
(`:920-923`): a null `optarg` is the bare word and means level 1 (FR-019),
attached text is read as a level, and `0`, `7` and `x` each answer one quoted
line at status 2 (FR-050) in `parseCount` 's shape (`:218-232`). No arm joins
`applySourceValue`, so the row lands in its refusing default (`:1517-1528`)
and no option file or environment carries it.

**Rationale**: The scan's default arm turns an absent optional value into the
seed text `0` (`options.cpp:1003-1006`), so a row routed there cannot tell
`--topdown` from `--topdown=0`, which FR-019 and FR-050 need to answer
differently; `--hunt` avoids it the same way (`:920-923`). Measured: 42 rows
in the array and 42 in `docs/pages/harness.md`; `kContract` moves 42 to 43
(`test/source/harness_cli_table_test.cpp:244`).

**Alternatives considered**: (a) A presence flag or a bool in `RunOptions`:
`specs/042-topdown/spec.md:929` bars a new bool. (b) A seventh `OptionRow`
field holding the bare seed: the row carries six fields
(`options.hpp:133-140`).

## D-11 - Tree nodes ride the one column vector, then leave it

**Decision**: After the `--metric` loop
(`source/harness/runner.cpp:1315-1326`) append one `MetricColumn` per
collected node through the same `detail::metricColumnFor` call (`:1323-1325`)
and no column for a refused row; one strip call at the end of result finishing
moves each figure into the `topdown` record and removes that position from
`metricLabels` and each row's `metrics` and `metricStats` (`:3245-3251`), the
percentiles, the retained chunk columns (`:3128-3137`) and the per-context
fold views (`:1885-1896`).

**Rationale**: That vector is the only input to the leaf grouping
(`:1348-1382`), the object-fact limit (`:1364`), `settleMethod` (`:1395-1398`)
and `planPasses` (`:1404-1406`), so appending yields one pass plan and one
pass set for metrics and tree together (FR-021, FR-023) outside every timed
window (FR-024).

**Alternatives considered**: (a) A second vector and a second plan: FR-021
names one set and a second fold, merge and ledger duplicate three sources of
truth. (b) Hand the leaves to `planPasses` with no column: the fold, the merge
and the ledger index by column position.

## D-12 - The console block hook and its lines

**Decision**: One printer over the record in `source/harness/report.cpp`,
called from `printResult` after the hunt lines (`:678-682`) and before the
single-context return (`:687-689`), tagged `topdown: ` in the style of the
pass line (`:312`). The header names the level shown, the level asked and the
source record; a node line carries two spaces per level below one, the neutral
name, the vendor name and the percent, or `not available` for a gap (FR-026,
FR-030); threshold text is appended unchanged with no verdict (FR-029); each
refused row gets one line with its level, kind word and reason (FR-031); an
asked level deeper than the table says so (FR-020); an AMD table adds the
fifth value and says the four shared values sum under 100 percent without it
(FR-028, FR-056).

**Rationale**: The standing order prints the pass disclosure at `:651`, the
jitter note at `:657-669` and the hunt block at `:678-682`, then returns at
`:687`, so one hook between the hunt block and that return gives every run
shape, threaded runs included, exactly one block after that benchmark's own
tables (FR-025).

**Alternatives considered**: One block at the end of the run: FR-025 wants one
per benchmark.

## D-13 - One record, one schema minor, two SOVERSION rises

**Decision**: The report gains a `topdown` record written beside the hunt
record and read back by `source/harness/json_load.cpp`;
`docs/schema/report-v1.json` gains the key in root `x-order` and in
`properties` (`:7`, `:10`); `kSchemaVersion` moves 1.11 to 1.12
(`source/harness/json_report.cpp:43`); the harness SOVERSION moves 19 to 20
(`CMakeLists.txt:375`) and the counters archive SOVERSION moves 4 to 5
(`:110`), because `MetricView` and `MetricListing` gain a field.

**Rationale**: The tests assert the stamp, so all eight sites move
with it, named by `specs/042-topdown/spec.md:138-147` and checked here: the
six asserts from `test/source/harness_json_report_test.cpp:546` to
`harness_noise_report_test.cpp:457`, and two plants of the next minor at
`harness_json_load_test.cpp:107` and `:1202` that take `1.13`.

**Alternatives considered**: Keep 1.11 and let `additionalProperties: true`
absorb the key: a reader could not tell a record's absence from an old writer.

## D-14 - The rule prefix, the node name, and the unsupported answer

**Decision**: `topdown:` joins `kLabelPrefixes`
(`source/harness/compare.cpp:55-56`) so `labelPrefixOf` (`:97-105`) recognises
it and the standing grammar (`:381-399`) reads prefix plus value name
unchanged. A level-1 node names `topdown:backend_bound` and a deeper node
names `topdown:intel.tma_l1_bound` (`specs/042-topdown/spec.md:693-697`), so
the rule text maps to the record's neutral name at level 1 and to its
qualified name below. An unsupported host answers one message naming the
vendor, the family and model or the MIDR, and the reason; the report records
`topdown` as unsupported and the run's exit status does not change (spec
D-14). A rule naming a node against a report with no record exits 2 with one
quoted line.

**Rationale**: The prefix table is the only place a value name's owner is
decided, and `:393-398` already refuses a prefix that names nothing. A typo
therefore reads as a fault, and no rule compares a node against an empty
value.

**Alternatives considered**: Reuse `metric:` for node values: a node is not a
metric column and D-11 strips its column.

## D-15 - The main page pays its own lines

**Decision**: `PAGE_LINE_CAP` stays 1004 (`tools/docs/example_drift.py:240`,
`README.md` measured 1004 lines here), so the new section, its option row and
its name rows are paid for by trimming the same measured number of lines from
the page in the same change, with the figures in the commit body (spec D-15).
The capability adds one title to `SECTIONS` (`:33-68`, 33 today) and one entry
to `SECTION_OWNERS` (`:73-108`), stays inside `SECTION_LINE_CAP` 25 (`:241`,
exceptions at `:246-249`) under `check_sections` (`:653-696`), and the option
table at `README.md:954-997`, measured with 42 rows, gains its row.

**Rationale**: The gate checks the page in both directions, so an added line
over an unmoved cap fails the drift check while a raised cap fails spec D-15.

**Alternatives considered**: (a) Raise the cap with a measured comment, the
practice the chain at `example_drift.py:234-239` records. Rejected: spec D-15
states the cap does not move. (b) A third page: the guide holds two pages only
(`docs/conf.py.in:6-9`).

## D-16 - A host-independent example, and no recorded console block

**Decision**: The README section's marked block comes from
`example/docs/topdown.cpp`, added through `add_docs_example`
(`example/CMakeLists.txt:40-50`) and named in `SECTION_OWNERS`, because
`check_sections` refuses a section that holds no marked block
(`tools/docs/example_drift.py:674-687`). The example answers from the
selection entry point over an identity it carries, so its rows stand on any
host. No recorded run joins it: no `topdown.expected` and no `report_shape`
registration (`test/CMakeLists.txt:1842-1856`). New test binaries register as
ordinary `add_test` entries, which `test/CMakeLists.txt:2107-2114` labels
`quick`, and stay out of `SPEEDGUN_SLOW_TESTS` (`:2086-2104`) so the audit in
`test/tier_labels.sh` (`MASTER_SKIP`, `:76-86`) finds one label each.

**Rationale**: A recorded console block would encode this host: the same tree
answers published or refused on the `#core_wide` and `#SMT_on` readings of
D-06 and D-07, `test/example_run.sh` fails a row that never prints
(`test/CMakeLists.txt:1965-1968`).

**Alternatives considered**: (a) A recorded block with a `report_shape`
registration: the shape gate cannot express "row absent on this host". (b) No
example at all: `check_sections` then refuses the section for holding no
marked block.
