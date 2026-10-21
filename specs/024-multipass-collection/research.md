# Phase 0 Research: Multi-pass metric collection

Feature: 024-multipass-collection. Branch `024-multipass-collection`, audit
point `7a75e95`. Each entry records the decision, the rationale, and the
alternatives considered, in the style the 022 plan and research use: `Decision`
(the chosen option letter, spelled out), `Rationale`, `Alternatives considered`
(each rejected with its reason).

## D-01 - Learning the programmable-counter limit

**Decision**: The `perf` PMU provider reads the CPUID leaves the tables
below names, takes the vendor from the leaf 0 vendor words, and answers
the programmable-counter limit for each vendor, as the research D-01
table gives, per device it enumerates. For an Intel or Zhaoxin core it
decodes leaf 0AH, the architectural performance-monitoring fields:
`EAX[7:0]` is the version,
`EAX[15:8]` is the number of general-purpose counters that support
programmable events per logical CPU, `EAX[23:16]` is their bit width,
`EAX[31:24]` is the length of the `EBX` programmable-counter bit vector, and
`EBX` carries that vector when the version exceeds 1. The core answer is
`EAX[15:8]` when the version is not zero, and nothing when it is. Measured on the development host `corsairone`
(i9-9920X, Cascade Lake) on 2026-10-20: `EAX=0x07300404`, `EBX=0x00000000`,
`EDX=0x00000603`: version 4, four general-purpose counters, 48-bit width. The
`EBX` vector reads zero on this host, so the decode treats it as decoration
and never subtracts it; the count field is authoritative. The spec's host
record names the same CPU, and its PC-3 records the kernel behavior that
motivates all of this: the kernel grants a group of 64 core-PMU events opened
at once and rotates it, with ratios of 0.428174 and 0.543263 measured on this
host (`docs/pages/counters-overhead.md`, cited by PC-3), so failure-probing
observes nothing.

**The AMD rule**: an AMD core answers from the extended leaves. The leaf
`0x80000022` words count only when the maximum extended leaf (leaf
`0x80000000`, `EAX`) reaches `0x80000022`. With `PerfMonV2` (`0x80000022`
`EAX` bit 0) set, the core answer is `EBX[3:0]`, the core PMU counter count
the part publishes. Without it, `PerfCtrExtCore` (`0x80000001` `ECX` bit 23)
answers 6, and the pre-extension core PMU answers 4. The maximum-extended
leaf gates the read because a leaf above the answered maximum answers the
maximum leaf's registers, whose low bits are unrelated data.

**The device table**: the answer is a fact of the device a seed names.

| Device | Published limit |
|---|---|
| `cpu` | the vendor's core answer, adjusted below |
| `cpu_core`, `cpu_atom` | the vendor's core answer, adjusted below (the Intel hybrid names) |
| every other device | no limit |

**The hybrid rule**: a host whose device list carries the hybrid core names
(`cpu_core` or `cpu_atom`) and whose leaf `0x23` answers publishes no limit
for its core devices, because one host-wide core figure would be wrong for
both core types. A hybrid host whose leaf `0x23` answers nothing keeps the
vendor's core answer.

**The watchdog table**: `/proc/sys/kernel/nmi_watchdog` decides whether the
kernel watchdog holds a counter at open time.

| Watchdog value | Meaning |
|---|---|
| 1 | enabled: a cycles event of the watchdog occupies a counter |
| 0 | released |
| -1 | the read or the parse failed, treated as any value other than 1 |

The adjustment applies the watchdog to the hardware core answer; the first
matching row wins, and a result of zero answers no value.

| Condition | Published limit |
|---|---|
| the watchdog value is not 1 (a failed read gives -1) | the hardware answer |
| Intel or Zhaoxin with a fixed cycles counter | the hardware answer |
| Intel or Zhaoxin without a fixed cycles counter | the hardware answer minus one |
| AMD, which has no fixed cycles counter and whose watchdog pins a cycles event | the hardware answer minus one |
| any other vendor | the hardware answer minus one |

Fixed counters count against the limit: leaf 0AH `EDX[4:0]` counts them
beside the programmable ones, and a version of at least 2 with `EDX[4:0]`
of at least 2 is the fixed cycles counter that decides the Intel and
Zhaoxin watchdog rows. An AMD paired event and an Arm chained event each
occupy two counters, so the published limit is a ceiling.

**The conservatism direction**: the rules above err low on purpose, because
the two errors cost differently. An under-count costs one extra pass: the
planner splits where the columns could have fit one plan, and the run measures
one pass more. An over-count is what risks a multiplexed pass: a plan the
hardware cannot hold opens anyway, the kernel rotates it, and the run reports
values measured at a running ratio below 1. Every rule below guards the
over-count and pays in passes when it guards too far.

| Rule | Which way it errs | The error it guards |
|---|---|---|
| the watchdog rows | low by one | an over-count, whose plan opens past the counter the watchdog holds |
| the hybrid rule | low to nothing | an over-count, because one core figure cannot fit both core types |
| the hybrid common subset | low by two | an over-count, whose plan the efficiency cores cannot hold open |
| the Arm answer | publishes nothing | an over-count, read from a register no unprivileged reader reaches |

**Alder Lake and Raptor Lake**: a hybrid host that keeps the vendor's core
answer keeps one figure for `cpu_core` and `cpu_atom` together, and that
figure is the common subset, the count both core types hold. It sits two
counters below what the performance cores really hold, so the ceiling
published on those parts is low by two. A pass planned under it opens within
the reach of either core type, at the price of the extra pass the direction
above names.

**The Arm hosts**: an Arm core publishes no limit, so its runs keep the single
kernel-rotated plan that PC-3 measured on the Intel host. `PMCR_EL0.N` counts
the programmable event counters of an Arm core and exception level 0 cannot
read it: the kernel leaves `PMUSERENR_EL0.EN` clear, so a read at that level
traps. The caps directory of the device,
`/sys/bus/event_source/devices/<name>/caps`, carries event fields and format
fields and no counter count. No value is published on an Arm host until the
group probe lands as follow-up work, and the planner reads that empty answer
as the row of E-02 that publishes nothing.

**Rationale**: Leaf 0AH is the only user-readable, hardware-shaped answer to
"how many programmable counters does this core hold" that the pinned Intel
catalog can rely on. The Intel SDM documents the leaf as the architectural
performance-monitoring discovery interface; `perf` itself and the Linux
kernel's CPU initialization read the same fields, and the kernel's counter
count for an architectural host comes from `EAX[15:8]`. The pinned event-table
trees map event names to encodings, descriptions and errata, and publish no
counter count; PC-3 measured that no file in `source/` or
`include/speedgun-ng/` reads or publishes a count today. The kernel refuses to read the performance-capabilities
MSR (`IA32_PERF_CAPABILITIES`, `0x345`) from user space: it is an
`RDMSR`-only model-specific register behind `perf_event_open`'s
`perf_read_caps` gate, and the kernel itself reads it in-kernel; a user-space
`rdmsr` needs `MSR` module access and root, which is outside the no-root CI
contract. The sysfs tree `/sys/devices/cpu/events` lists event names
(bus-cycles, instructions, and the like, verified on this host) and publishes
no counter count; `cpu/cpus` holds no counter-count attribute either. The
dogfood rule of the 022 plan (the harness reads every machine fact through
`sg::counters`, never through `/proc`, sysfs or its own CPUID call) makes the
provider the only legal reader in this repository, and the spec's host record
carries family 6, model 85 for the pinned hardware, parts that enumerate the
architectural performance-monitoring leaf. The count must stay fakeable
because CI runs without a usable PMU (rootless, no hardware `perf_event_open`
access, as the 022 plan records): the spec's FR-005
and FR-006 make the published fact plus the fake parameter the single source
the planner reads, so one mechanism serves both worlds (D-02, D-03).

**Alternatives considered**: (a) Failure-probe the host by opening
progressively larger groups and watching for `ENOSPC`. Rejected: the spec's
PC-3 measured the premise false on the master audit point, the kernel grants
oversize groups and rotates them, so the probe observes nothing; the spec's
D-3 records this. (b) Read a kernel-internal count from sysfs. Rejected: no
such attribute exists (`/sys/devices/cpu/events` lists names, measured), and
even if a future kernel grew one, reading it would break the dogfood rule and
the no-root story twice over. (c) Hard-code the limit per model in a new table
inside the pinned catalog. Rejected: the CPUID leaf answers every part that
carries the catalog without a per-model row, and a row that disagrees with the
leaf is a second source of truth. (d) The AMD leaf
`0x80000022` is adopted as the AMD rule above; an earlier draft rejected it
because the pinned catalog is the Intel core tree (PC-3 records the core-PMU
measurements on that host's family), and the amendment takes the extended
leaf as AMD's own answer, gate and fallbacks included. (e) Let the harness read CPUID
directly. Rejected: the time-source and dogfood rules put every hardware fact
inside the counters library, and the handle-selector path needs the fact
through the library surface anyway.

Sources: Intel SDM Volume 3, the architectural performance monitoring
enumeration via CPUID leaf 0AH (fields as decoded above; the same
layout appears in the kernel's `tools/arch/x86/kcpuid/cpuid.csv`: version
`eax 7:0`, `num_counters_gp` `eax 15:8`, width `eax 23:16`, vector length
`eax 31:24`); the kernel's use of the version and count fields in
`arch/x86/kernel/cpu/zhaoxin.c`; the host facts measured in D-01's probe.

## D-02 - Where the limit is published in the public API

**Decision**: `ObjectSeed` (include/speedgun-ng/counters_provider.hpp:62)
gains a trailing member `programmableCounters` of type
`std::optional<std::size_t>`, defaulting to no value, and the catalog cursor
`sg::counters::Object` (include/speedgun-ng/counters_system.hpp:34) gains
`auto programmableCounters() const -> std::optional<std::size_t>` answering
the published value of the object's event source. A provider that can answer
fills the field on each device object it owns; a provider that cannot answer
leaves it empty. The PMU provider fills it on the core devices it enumerates
(`cpu`, `cpu_core`, `cpu_atom`) per the vendor rules, the hybrid rule and the
watchdog adjustment of D-01; every other device, and every core device of a
hybrid host whose leaf `0x23` answers, publishes no limit; the clock, push
and fake providers leave it empty unless scripted. The harness planner reads the fact through
`System::object(path)` and nothing else.

**Rationale**: The limit is a catalog fact of an event source, the same kind
of fact `CatalogEntry` already publishes per counter and the object
description publishes per object; publishing it on the object it describes
keeps "what can answer" exactly where the spec's FR-005 puts it ("per event
source"). `ObjectSeed` is the registration-time struct a provider already
fills for every object, so publication reuses the existing path
(`ProviderIface::enumerate`, counters_provider.hpp:437) with one trailing
optional: every existing designated-initializer call site keeps compiling
because an omitted trailing member default-constructs. `std::optional` is the
standard spelling of "a source that cannot answer", so no sentinel value or
`hasLimit` flag enters the API. The harness resolves a metric's leaves by
object path already (`runner.cpp:1163` resolves each leaf address through
`sys.object`, `runner.cpp:1174-1180` finds its catalog entry), so reading the
fact costs the planner one lookup per distinct
source and no new coupling.

**Alternatives considered**: (a) A free function
`sg::counters::programmableCounterLimit(const System&, std::string_view)`
queried by path. Rejected: it duplicates a lookup `Object` already caches and
presents a per-object fact as a system global; the query form also invites
callers to ask the machine when the catalog holds the answer. (b) A per-leaf field on
`CatalogEntry` (counters_core.hpp:246). Rejected: the limit is not a property
of one event; copying it onto every leaf multiplies one fact across the
catalog and invites disagreement between copies. (c) A virtual on
`ProviderIface` (counters_provider.hpp:424). Rejected: providers reach the
system only through the `ObjectSink` enumeration path today, and a virtual
that the system must consult at registration duplicates that channel for one
field. (d) A new dedicated facts header with a registry singleton. Rejected:
the catalog cursor is the registry, X.2 forbids the second mechanism.

## D-03 - How the fake provider takes the limit

**Decision**: `FakeProvider` (include/speedgun-ng/counters_fake.hpp:103)
gains a fluent setter in the house's scripted-fact pattern:
`auto setProgrammableCounters(std::string_view objectPath,
std::size_t limit) -> FakeProvider&`. The call records the limit beside the
named object's seed data (`FakeProvider::ObjectSeedData`,
counters_fake.hpp:237), and the next `enumerate` publishes it through the
`ObjectSeed` field of D-02. An object never given the call publishes no
limit, which reproduces the kernel-rotation host. The scripted limit applies
to opens exactly as the PMU provider's leaf does: the fake still opens any
leaf set it is asked to; the planner owns the split.

**Rationale**: `FakeProvider` already scripts host facts fluently per object
(`addObject`, `addCounter`, `setPoints`, `setGapActions`; counters_fake.hpp:
135, 156, 191, 225), so a fifth setter in the same shape is the smallest test
surface the spec's FR-006 asks for ("the fake provider takes the counter
limit as a test parameter"). Per-object path matches the per-object
publication of D-02 and lets one fake stand in for the two-provider edge case
(one object scripted with a limit, one without). Post-registration calls
re-enumerate nothing live, so the setter must precede `System::open`, which
matches every other scripted fact and the recorded fixture rule this
repository already keeps (providers register before the system opens).

**Alternatives considered**: (a) A constructor parameter on `FakeProvider`.
Rejected: the fake has one constructor used by every existing fixture, and a
mandatory parameter would churn every call site for a fact most fixtures do
not script. (b) A field on `FakeScript` (the per-window script). Rejected:
the limit is not a per-window value; it exists before any window opens.
(c) A global override read by the fake. Rejected: ambient state breaks the
parallel `ctest -j2` contract and X.2's no-ambient-rule.

## D-04 - Read site shape: branchless probe plus pure decode

**Decision**: The PMU provider's CPUID work grows one site in
`source/counters/linux_pmu/` beside the existing leaf reads in
`table_parse.cpp` (`<cpuid.h>` there at line 46, leaf 0 at line 71, leaf 1 at
line 90). Two pieces: a branchless probe that executes
`__get_cpuid_count` once per leaf for the leaves D-01 names and returns the
raw words with no decision of its own, and the pure free functions of the
PMU detail header (`vendorOf`, `intelCoreLimit`, `amdCoreLimit`,
`fixedCycles`, `watchdogValueFrom`, `usableCounters`, `isHybrid`,
`limitForDevice`) that decide the answer. The probe holds no decision; the decode holds no
instruction. `<cpuid.h>` is the house's standard header for this (time-source
gate bans `<x86intrin.h>`-family headers, not `<cpuid.h>`; `table_parse.cpp`
is the precedent).

**Rationale**: The decision function is unit-testable over every arm with
synthetic registers, which is what keeps the new code at 100 percent branch
coverage on hosts that cannot reproduce "no leaf" or "version 0" in hardware
(FR-032, no new `LCOV_EXCL` marker). The probe stays branchless because the
guard needs no code: every register word is zero-initialized before its
`__get_cpuid_count`, and a leaf above the answered maximum leaves its
registers untouched, so an absent leaf reads as zeros; the gates
(`maxLeaf`, `maxExtended`, the version fields) live in the pure functions,
which keeps the probe straight-line and the arm countable.
Executing leaf 0AH on every x86-64 host is safe: the instruction exists on
every x86-64 part and the clamped return is defined by that behavior.
The `ebx` parameter enters the signature because the SDM defines the
vector; the decode ignores it, and the signature plus one comment recording
the measured `EBX=0` state of the fleet keeps the decision honest.

**Alternatives considered**: (a) Read the leaf inside an `if
(maxLeaf >= 0x0A)` at the call site. Rejected: the false arm of that `if`
never runs on CI hosts, so the branch cannot be covered without a new
exclusion marker, which FR-032 forbids. (b) Fold read and decode into one
function. Rejected: the merged body's hardware-dependent arms are exactly the
uncodable coverage hole this split removes. (c) A member on the PMU provider
class. Rejected: a static decision over plain integers is a free function; a
class member would hide it from the unit test that exists to drive it
(X.2).

## D-05 - Where the planner lives

**Decision**: The pass planner is harness-internal: a new header
`source/harness/detail/pass_plan.hpp` and a new definition file
`source/harness/pass_plan.cpp`, the same shape `percentiles.hpp` and
`percentiles.cpp` have (declared in the `detail` header, defined in the
archive, tested from outside through the public run path and directly as
internal-linkage-visible units). Three pure functions:
`planPasses(columns, sourceLimits)` returning the ordered pass plan (E-03),
`settleMethod(method, anyLimit, metricColumns)` returning the settled method
(E-01 table), and `passSpread(passTimes)` returning the spread or nothing
(E-06). First fit in request order: metric columns first in the order
`--metric` and `addMetric` registered them (the order `metricLabels` keeps,
benchmark.hpp:456), then user-counter columns in the order they opened
(benchmark.hpp:460); a column joins the first pass whose per-source used
counts plus that column's distinct leaf addresses per source still fit every
source's published limit; a column whose count exceeds some source's limit
alone stands in a pass of its own (spec D-2); sources that publish no limit
count toward no limit and ride every pass that needs them (spec D-4 and the
two-provider edge). Leaves are deduplicated by address per pass, so two
metrics sharing `cpu/instructions` claim the counter once.

**Rationale**: The counters library compiles a leaf set and lets the kernel
rotate; that contract is what `--metric-method=multiplex` must reproduce
exactly, so the library cannot own the split: a library that split
on its own would make the multiplex method impossible (FR-024 pins the
single-plan behavior). The harness already owns run policy (calibration,
repetitions, contexts), and the spec's FR-008 keeps the split outside the
library's read path for a second reason: planning reads only published facts
and column leaf tables. Keeping the functions pure in one TU makes every
decision a decision-table test on the fake (D-14).

**Alternatives considered**: (a) Plan inside `compileCore` and hand the runner
multiple plans. Rejected: it moves run policy into the measurement layer,
breaks the multiplex method, and drags the split under the library's SOVERSION
story for no benefit. (b) A `PassPlanner` class holding the limit and columns.
Rejected: the inputs are two small vectors and the output is one plan; a class
is ceremony X.2 rejects (D-16 of 022 is the precedent). (c) A generic
multi-source bin-packer (first-fit-decreasing or better). Rejected: the spec's
D-1 fixes first fit in request order because the assignment must be
predictable to the caller who reads the disclosure; an optimal packer shuffles
columns silently, which is exactly the "guessing" XI forbids in behavior.
(d) Put the planner in `cli.cpp` or the runner file itself. Rejected:
`runner.cpp` is the most contended file across lanes; a new pure TU cannot
collide beyond one include, and the pure functions want a test target of
their own (R-1).

## D-06 - How pass runs reuse the existing compile/open path

**Decision**: The pass loop wraps the measured repetitions. Calibration stays
where it runs today and happens once, before the first pass; every pass
reuses its iteration count and chunk plan (spec FR-010). For the
single-context path the runner builds the leaf list once per pass: the two
fixed clock leaves first (`machine/monotonic` at runner.cpp:1119, the vendor
cpu leaf chosen at runner.cpp:1127-1135, both clock-provider leaves that
consume no programmable counter, spec D-4), then the columns that pass owns,
and calls the one existing `compileCore` site (runner.cpp:1252-1253). For the
threaded path each context body compiles its own plan inside the same body
today (runner.cpp:1727-1730, the spec's "runner.cpp:1721-1732" site, beside
the barrier pair at runner.cpp:1709-1710); the pass loop sits outside the
context dispatch, so per pass the run dispatches all contexts, each compiles
through that unchanged site, and every context joins before the pass closes
its plan. Windows therefore open and close per pass (FR-012), pass `k`
finishes in every context before pass `k+1` opens anywhere (FR-013), and the
placement chosen once stands for all passes (the `contexts` disclosure comes
from pass 0, FR-015). The plan compile/open and the merge run outside any
timed window (FR-008).

**Rationale**: The existing sites already do the work: leaf vector to
`compileCore`, provider-grouped read groups per `source/counters/plan.cpp:500
to 560`, per-group `open`, disclosure columns past each group's leaves. A
pass is exactly "the same run with a smaller leaf list", so reuse wraps the
sites in a loop. Putting the loop outside the threaded
dispatch makes FR-013 structural: within one pass the existing start/stop
barriers already align contexts, and the join before the next pass is the
existing join, so no new synchronization primitive appears anywhere; FR-034
tests that count. Keeping calibration outside the loop keeps the
iteration count identical across passes, which is what makes the merged
repetition rows comparable (D-7) and is the spec's recommended resolution.

**Alternatives considered**: (a) A second plan open per pass held
simultaneously. Rejected: the spec's FR-013 forbids overlapping windows, and
simultaneous opens would consume counters in all passes at once. (b) Re-run
calibration inside each pass. Rejected: FR-010 states once; re-calibrating
would let passes disagree on iteration count and break the merge's row
alignment. (c) Let each thread run its own passes independently. Rejected: FR-
013 names the overlap rule for the whole run; independent threads would let
two passes hold windows at once and inflate the spread with scheduling noise.
(d) Compile all passes' plans up front and open lazily. Rejected: a compiled
plan holds open provider windows from compile time, which breaks FR-012's
per-pass close immediately.

## D-07 - Merge

**Decision**: The merge is a pure function over the per-pass folded results.
Each metric column's value comes from the pass that measured it, at the
column's own position (FR-014); user-counter columns likewise carry their
measuring pass's value (FR-014). The reference pass is pass 0 (spec D-5): its
time columns, overhead floor, endpoint samples, context disclosures, chunk
disclosure and percentile columns feed the merged rows (FR-015; the percentiles
edge: the reference pass's chunk times feed the time-column percentiles, and
each metric column's percentile comes from its own pass's chunks).
`MetricValue::pass` (E-08) records the measuring pass index per column and
per repetition row. A metric column whose measuring pass's window folded a gap
keeps `Availability::GAP` with no invented value (FR-016, spec D-2's
discipline); a refusal kind rides unchanged. The merged `BenchmarkResult`
carries `metricMethod`, `passCount`, and the spread or nothing (E-07).

**Rationale**: Column provenance is data the planner already knows, so the
merge reads the plan; clock and counters stay unconsulted. Pass 0 is the
reference because it is the pass every run has, and its time columns are the ones
today's overhead and percentile machinery computed; choosing it keeps the
one-pass run's merge a copy, which is what makes the D-9 byte rule observable
in code (`passCount == 1` takes the existing fold path untouched).
Percentile columns must follow their own pass because chunk times from a
different pass would state a distribution the printed metric never saw; the
spec's edge case for percentiles states exactly this.

**Alternatives considered**: (a) Median or mean across passes for shared
events. Rejected: an event is measured once per run by construction (each
column has one measuring pass), and averaging across passes whose wall times
differ blends states the run did not see. (b) Take the endpoint sample count
and floor from the pass with the most samples. Rejected: FR-015 names pass 0;
a max rule would make the published floor depend on split shape and break the
one-pass byte rule's shape. (c) Re-fold metrics from raw points after the
last pass. Rejected: raw points are not retained across passes (the plan
closes with its windows, D-06), and retention would grow memory by the pass
count and add nothing the reader needs.

## D-08 - Spread computation shape

**Decision**: `passSpread(passTimes) -> std::optional<double>` is a pure
function over the merged reference time per iteration of each pass (the same
number the report would show for that pass; for a threaded run that is the
merged value, spec D-6 and clarification Q3). Decision table, in this order,
one return: fewer than two passes yields no value; median zero yields no
value; otherwise `(max - min) / median * 100` as a percentage, where the
median of an even count is the lower of the two central values. The warning
predicate is a second pure line: a value exists and exceeds 5.0 (strict). The
warning never changes any outcome (FR-018). The printer passes the value to a
one-decimal `printf` field, so the printed decimal of every fixture of
data-model E-06 comes from one rounding at print time; the warning
comparison uses the unrounded value.

**Rationale**: Both pieces are decision tables over plain doubles, driven to
every arm by three-value fake fixtures with no clock: the "fewer than two"
arm, the zero-median arm (FR-017's edge: percentiles follow the master
`not available` precedent, see `printPercentileLine` at report.cpp:299-318),
the warning arm, and the silent arm (5.0 and below). The single-entry,
single-return shape is the gcov discipline this repository records: gcov
attributes phantom arcs to closing braces, to loops with dead fall-through
edges, and to functions returning a local by value, which is why the
repository's lcov invocation filters braces and branch artifacts
(`cmake/coverage.cmake:83`, `--filter exception,branch,brace`, per the 022
plan's D-16) and why new code avoids those shapes on purpose. A median over
three values uses one `std::ranges::nth_element`-free path on purpose:
sort a copy of at most the pass count (a handful), take the middle, so no
iterator gymnastics creates an extra arc. No `LCOV_EXCL` marker is added
anywhere (FR-032, PC-5).

**Alternatives considered**: (a) Relative to the median as a fraction
(`0.111`) with the printer scaling. Rejected: the spec names percent, and
storing the percent keeps published field, printer and contract in one unit;
one rounding site owns the printed number. (b) Mean absolute deviation or coefficient of
variation. Rejected: FR-017 names min, max and median; a richer statistic
would invite thresholds nobody set. (c) A `struct Spread { double value; bool
warn; }`. Rejected: the warning is one comparison of a published value; a
struct is a second place truth lives (X.2). (d) Compute the spread inside the
runner loop. Rejected: a pure function takes the decision out of the timed
machinery and into a table (III.1).

## D-09 - Method option plumbing

**Decision**: `--metric-method=auto|passes|multiplex` joins the long-option
table (source/harness/cli.cpp:303-305, beside `percentiles` and
`no-percentiles`), takes its argument, and stores into the run options beside
`showPercentiles`, which is the exact "command line wins over the family"
shape `case 'p'` demonstrates at cli.cpp:345-348. A value outside the three
words prints one line to stderr in the house's refusal voice, printing
`--metric-method 'X' is not auto, passes or multiplex`, and exits 2, the same contract shape
`metricNameFailure` prints before any measurement (cli.cpp:215-247, invoked at
cli.cpp:413) and the same status the unknown-option default returns
(cli.cpp:352-355). Repeated flags: the scan overwrites, so the last wins, the
natural `getopt_long` behavior the option table already has for
`--percentiles`. The family-side selector `metricMethod(MetricMethod)` sits on
`BenchmarkHandle` beside `showPercentiles` (benchmark.hpp:1795); the
handle path takes the closed enum, so a bad library-side value cannot exist,
which answers the spec's bad-value edge: the exit-2 rule belongs to the CLI
surface alone. Resolution at run time is one pure line: CLI value
present wins; else entry value; else `auto`.

**Rationale**: One flag carrying one of three words matches `--percentiles`
(a value-carrying flag with a closed grammar) and keeps the method visible in
`--help` and in the README option row that `example_drift` compares with
the table in `cli.cpp`. Exit 2 is the harness's option-refusal
status; the refusal prints before expansion and before any measurement, so a
typo costs no measured time, the same reason `metricNameFailure` stands where
it does. The typed handle selector removes the handle-side bad-value case by
construction, which is the strongest form of the spec's edge: the library's
metric path cannot be given a non-method, nothing else on the handle path
parses text, and the exit-2 rule stands on the CLI alone.

**Alternatives considered**: (a) Three boolean flags (`--multiplex`,
`--force-passes`). Rejected: three spellings for one enum allow
"both" selections the spec's one-method-per-run rule forbids. (b) Reuse a
generic `--set key=value`. Rejected: no such mechanism exists; inventing one
for a three-valued option is a second CLI grammar (X.2). (c) Reject the bad
value inside the runner per family. Rejected: FR-026 requires the refusal
before any measurement; the CLI-side check is where 022 put the metric-name
check for the same reason.

## D-10 - Disclosure console rule (resolving FR-017, FR-020, FR-021 and FR-024)

**Decision**: The printed disclosure line appears when the run carries at
least one metric column and the settled method names itself on the console:
the line prints when the settled method is `MULTIPLEX`, and prints when the
settled method is `PASSES` with more than one pass or with the caller's
forced choice recorded. An `auto` run that settles to `PASSES` with one pass
prints no new line: its report is master's byte for byte (D-9, SC-03, US3
scenario 3). The published result fields (`metricMethod`, `passCount`,
`passSpread`) carry the settled values on every run that carries metric
columns, including the `auto` multiplex case spec D-11 and Q1 name. The forced
record is a second bit on the entry, set by the selector and by the CLI flag;
it decides printing only (FR-021's "forced a method" wording).

**Rationale**: The spec pins every case except one combination: an `auto`
run on a host that publishes no limit settles to `MULTIPLEX` (FR-024), FR-020
says a multiplex run's report "shall call out the method on the same
disclosure line", and FR-021 says a run "which ran one pass under the default
method" prints master's bytes. A no-limit `auto` run never entered the pass
mechanism (no limit, no planner, no passes), so "ran one pass" describes the
planner's single-pass output on a limit host, and FR-020 and spec D-11 win the
no-limit host: it prints the method line, exactly as US3 scenario 4's
multiplex run does. The reading that keeps every pinned scenario true is:
byte-identity for the fitted `auto` run (the case SC-03's golden fixture
captures, measured against master on a limit-published host or its fake
equivalent), method line for every `MULTIPLEX` settling and every forced or
multi-pass `PASSES` settling. The flag exists because "forced" is caller
state the settled method cannot recover: a forced `passes` run that fits in
one pass settled to `PASSES` with count 1 (US2 scenario 2) and must still
print, while the identical auto run must not.

**Alternatives considered**: (a) Never print for `auto`, fields only.
Rejected: it leaves the no-limit `auto` run undisclosed, contradicting spec D-11
("the report discloses the multiplex method") and FR-020 read plainly. (b)
Print for every metric-bearing run. Rejected: it breaks FR-021 and SC-03 for
the fitted `auto` run. (c) Reconstruct "forced" from `MetricMethod` equality
(option word equals settled method). Rejected: `auto` and a settled `PASSES`
coincide on exactly the silent case, and forced `multiplex` on a no-limit host
coincides with the printed one; equality confuses both (the flag is one
`bool`, not a state machine).

## D-11 - Report printer and pass tag

**Decision**: One new report function `printPassDisclosure(const
BenchmarkResult&)` in `source/harness/report.cpp`, called from `printResult`
next to the existing disclosure and warning call sites (report.cpp:537, 558);
`printContextDisclosure` (report.cpp:173-196) is not touched (spec N4, D-8).
The line follows the tagged-line pattern of the chunks and percentile lines
(`printf("%-24s %10s  ...")` at report.cpp:306, the chunks line at
report.cpp:358-362, the golden shape of
`example/docs/percentiles.expected`): the tag is `passes`, and the fields are
`method=`, `count=`, and `spread=` when a spread value exists. The warning
line follows the noise-warning voice (`warning: ...`, report.cpp:251-255) and
prints when the spread predicate says so:
`warning: pass spread 11.1% exceeds 5%`. The pass tag on metric fields
(FR-019): the metric field loop at report.cpp:489-504 gains one conditional
`pass=%zu` field after `gap=`, printed when `result.passCount > 1`; a
one-pass run prints today's fields untouched, keeping the byte rule. The
exact shapes live in `contracts/pass-disclosure.md` and are golden-tested
against a fixture captured in P5.

**Rationale**: A new function keeps this lane's lines out of the percentiles
lane's function body (D-8), and the tagged-line format is the house format
for "one line under the row block that names the run" (the `%-24s %10s`
prefix aligns the tag column under the row labels). Riding the existing
metric-field loop for the pass tag is what FR-019 says ("within the report's
existing metric field spelling"): the value, ratio, scaled and gap tokens
keep their positions, one token extends. The `passCount > 1` gate keeps
every one-pass output byte-clean without touching field logic.

**Alternatives considered**: (a) Extend `printContextDisclosure`. Rejected:
D-8 names it untouched, and it is threaded-only while the pass line is not.
(b) Put the pass count inside `printContext`'s version line. Rejected: the
version line describes the build; this line describes the run. (c) Tag each metric with its
pass inside the metric label (`instructions@p0 (cpu/instructions)`).
Rejected: labels come from the catalog path already
(runner.cpp:1163,1174-1180);
hiding an index in a label breaks the label equality SC-03 and the golden
metric columns depend on. (d) A separate aligned passes block after all rows.
Rejected: FR-019 places the tag on the metric fields; a second block would
duplicate the association.

## D-12 - Version and SOVERSIONs

**Decision**: Project `VERSION` moves to the next free minor at rebase time:
`0.13.0` when nothing merged first (CMakeLists.txt:7 carries `0.12.0` now;
FR-031 and 022's identical instruction: check the file at landing). The
counters archive `SOVERSION` moves 2 to 3, and the harness archive 7 to 8,
with the CMakeLists comment listing the names that moved the layout (the
0.x rule is stated at CMakeLists.txt:41-46; the precedent notes sit at
CMakeLists.txt:215-224). Counters moves because `ObjectSeed` gains a
member and `Object` and `FakeProvider` gain members on exported classes; the
harness moves because `BenchmarkHandle` gains a selector, `BenchmarkResult`
gains three members and `MetricValue` gains one (the rule treats added data
members as breaking for consumers compiled against the old layout).

**Rationale**: The 0.x rule at CMakeLists.txt:41-46 explains why the number
is hand-kept: "the shared-object version carries the ABI number, which a
consumer links against", so deriving it from the major version would let
every 0.x release claim one ABI. The trigger is the one the harness comment at
CMakeLists.txt:215-224 records twice: 020 moved the number to 6 because
"BenchmarkResult gains the contexts disclosure ... the exported layouts
change and the hand-kept number moves", and 021 moved it to 7 because
"BenchmarkHandle gains the percentile selector, so the exported layouts move
again". Every item here is that kind of change.

**Alternatives considered**: (a) Hide `ObjectSeed` behind a pimpl so the
layout survives. Rejected: the rule exists precisely to make consumers
relink before reading a changed layout; a pimpl here is speculative generality.
(b) Land the fields inside a reserved padding block. Rejected: no padding
block exists; inventing one is worse than the bump the rule already prices.

## D-13 - Documentation and example plan

**Decision**: A new README `###` section "Multi-pass collection" (at most 25
lines, the `SECTION_LINE_CAP` at `tools/docs/example_drift.py:125`), owned by
a new docs example `example/docs/metric_passes.cpp` built through
`add_docs_example` (`example/CMakeLists.txt:40-48`, the `docs_`/`run_docs_`
pattern; `test/example_run.sh:7` runs every `docs_` target automatically).
The section goes between "Hardware counter metrics" and "Skips", beside the
other metric-facing prose; `SECTIONS` and `SECTION_OWNERS` gain the entry
(example_drift.py:30-52, 57-...). The quick reference gains the public-name
rows (`metricMethod` on `BenchmarkHandle`, `programmableCounters` on
`Object`), the option table gains the `--metric-method` row (the
`--percentiles` row at README.md:654 is the pattern), and README sits at its
cap today (666 lines = `PAGE_LINE_CAP`), so the recorded `PAGE_LINE_CAP`
raise moves to the measured post-change count with a comment (FR-028, FR-030;
the cap raises 022 D-14 records). `docs/pages/harness.md` carries the full
contract detail beside the metric sections (FR-027). `--help` gains the flag
line (FR-026), and the option row has one gate: `example_drift`'s option check
compares the README rows with the table in `source/harness/cli.cpp`.
The section body stays at README voice: what the passes do, the three method
words, the disclosure line, one copied run region. The example registers a
family with metric columns that exceed a scripted limit of 4 on the host's
real PMU where present; the recorded `.expected` file follows
`example/docs/percentiles.expected` (captured on `corsairone`, whose limit is
4), and `example_run.sh` runs it on any PMU host in CI's PMU-capable
configuration. Where no PMU exists, the example prints its rows without
metric columns, and its golden records that shape too, the same host-shape
handling the metric examples already carry.

**Rationale**: The drift gate requires every `###` capability section of the
main page to own its snippets through a registered example file (its D-12/D-13
note in example_drift.py), so a prose-only section is a gate finding; the docs
function already gives the target, runner and gate wiring for one filename.
Recording `.expected` on `corsairone` matches the spec's PC-1 (recorded output
lives there) and keeps the CI-host shape out of the golden the way the
existing metric goldens handle host dependence.

**Alternatives considered**: (a) Extend the existing `counter_metrics` docs
example. Rejected: one owner file per section is the gate's rule; folding a
new section's snippets into another section's owner breaks it. (b) Keep the
detail in README and skip `harness.md`. Rejected: FR-027 requires the detail
page; README carries the 25-line span. (c) A new docs page beside the
`harness.md` section. Rejected: passes are harness run behavior; a new page
fragments the run story (FR-027 names the existing pages).

## D-14 - Test strategy and fake-provider coverage

**Decision**: Five new test executables (plan's Source tree) plus extensions
to `harness_cli_test.cpp`. The fake provider scripts the limit per object
(D-03) and scripts counter values per window, so the planner,
merge, spread and disclosure tables run to every arm with exact integers and
no hardware: limits 1, 4 and 8 against column-shape fixtures (fit exactly,
overflow by one, over-limit alone, the 64-column set, empty set, duplicate
leaves), merge provenance fixtures, and every spread fixture of data-model
E-06. The threaded pass-order test uses two contexts and two
passes over the fake and counts window opens per pass through the
`FakeProvider`'s existing action accounting (`readActions`,
counters_fake.hpp:205) plus per-pass plan lifetime observations; it contains
no sleep, no wall-clock wait and no polling loop (III.3). The byte-identity
test captures master's output for one fitted metric run at the audit point
into a fixture file and diffs the rebuilt output against it. The fixture
reaches its test binary as the first command-line argument: the `add_test`
call passes the path through a generator expression over
`CMAKE_CURRENT_SOURCE_DIR`, and the test opens the file with `std::ifstream`.
The PMU-side
tests drive the counter-limit decoders with synthetic registers
(every rule row) and assert the publication fact on `corsairone` only in
the recorded script of D-15, never in CI. Coverage runs under the existing
preset: `-fprofile-update=atomic` stays, no `-D_GLIBCXX_ASSERTIONS` (FR-037),
`ctest -j2`, and the grep for new `LCOV_EXCL` markers in this feature's files
returns nothing (FR-032).

**Rationale**: The limit being a fake parameter (FR-006) is the keystone: on
CI hosts the split branch is reachable without hardware, which is exactly the
reason the spec made the fact fakeable. The integer fixtures of data-model
E-06 print exact decimals, so goldens stay stable (III.3). The pass-order test
reads counts after the join, so the assertion rides the thread join's
happened-before edge; time decides nothing (III.3's ban on lucky timing).

**Alternatives considered**: (a) Drive split tests against the real PMU on a
PMU host in CI. Rejected: CI hosts are rootless and reach no hardware PMU
(the 022 plan records the missing `perf_event_open` access), and the split
decision does not need hardware. (b) Test the
threaded order with a barrier callback inside the benchmark body. Rejected:
body callbacks land inside timed windows; the count-after-join approach keeps
the observation outside measurement. (c) Capture byte-identity by diffing
against an inline hard-coded string. Rejected: a file fixture captured from a
master build is the house pattern (`reading_the_report.expected`) and survives
lane rebases better than prose in a test.

## D-15 - Real-PMU comparison script placement

**Decision**: `tools/pmu_events/multipass_comparison.sh` runs on
`corsairone`: it builds the shipped binary with this feature, runs the pinned
over-limit metric set (the SC-01 set: `cpu_clks`, `instructions`,
`branch-instructions`, `branches`, `cpu-cycles`, `bus-cycles`) under
`--metric-method=multiplex` and `passes` and `auto`, compares the split run's
per-metric values against a master-build single-metric reference run,
records wall time and a quiet-window noise marker, and writes its output to
the feature directory as evidence. It is not registered in CMake and no CI
job runs it (FR-035). The script's name and directory join the existing PMU
event-table tooling under `tools/pmu_events/`.

**Rationale**: `tools/` is where the repository keeps host-facing utilities
(`tools/pmu_events` already holds the PMU table tooling); anything under
`test/` is or looks like a gate, and FR-035 forbids the gate. Placing the
comparison next to the table tools keeps PMU-facing scripts in one place for
the reader who audits PMU claims.

**Alternatives considered**: (a) `test/` with an environment guard that skips
it on CI. Rejected: a skipped CI test is exactly the "silently vacuous"
failure mode the constitution's verification discipline rejects (the memory
ledger records two such incidents for this repository's gates). (b) A CMake
manual-run target outside CTest. Rejected: it would still live in the build
tree of every CI job for a script no CI host can run at all; a shell script
under `tools/` needs no build. (c) Ad-hoc commands recorded only in this
document. Rejected: FR-035 asks for a script, so the SC-01 evidence stays
reproducible.
