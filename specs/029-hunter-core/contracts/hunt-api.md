# Contract: the public hunt surface (`hunt.hpp`)

**Feature**: specs/029-hunter-core | **Surface owner**: `sg::hunt` |
**Audit point**: `bd10d76` | **Spec**: FR-001 to FR-033, D-15, hard rules 5 and 6

## What this contract covers

`include/speedgun-ng/hunt.hpp` is the one header the hunt surface enters
through. It is a new public header at `bd10d76`: `grep -rn '#include
"hunt' source/` answers nothing, and `include/speedgun-ng/` holds no hunt
header, so no existing declaration moves and no existing header gains an
include. The header carries no `detail/` type in any signature (D-15), reaches
the harness archive through no internal header, and pairs one contract line with
every contract macro, which is what the `dbc-gate` target compares
(`tools/dbc/dbc_pair_gate.py:198-199` walks the tree by suffix; the `dbc-gate`
target is declared in `cmake/dbc-gate.cmake:41`).

Every contract below is written in the house form: a `\pre`, a `\post`, and for a
type that owns state an `\invariant`, in the style
`detail/pass_plan.hpp:63-107` shows. No contract predicate names a lambda
(hard rule 6): each predicate below is a named function or a comparison over
plain values.

## Header shape

```text
#ifndef SPEEDGUN_NG_HUNT_HPP
#define SPEEDGUN_NG_HUNT_HPP

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/speedgun-ng_export.hpp"

namespace sg::hunt
{
```

The include-guard form, the export macro name and the license block copy
`include/speedgun-ng/benchmark.hpp` at its head, so the two public headers read
alike. `enum class MetricMethod : std::uint8_t;` is forward-declared in
`namespace sg` inside this header, the pattern
`source/harness/detail/pass_plan.hpp:27` already carries, so `hunt.hpp` does not
include `benchmark.hpp` and no include cycle appears. `class BenchmarkHandle;`
is forward-declared there too, so `runHunt` names the public handle of
`benchmark.hpp:1840` without including that header. The forward declaration
names the type and not its enumerators, so a record that holds the settled
method holds `std::optional<MetricMethod>`: the header cannot write an
enumerator as a default, and an unset method reads as a plan nothing settled.

## The value types

```text
enum class HuntNorm : std::uint8_t { INSTR, CYCLE, ITER };
enum class HuntRank : std::uint8_t { RATE, CV };

struct HuntBudget
{
  enum class Kind : std::uint8_t { SECONDS, PASSES };
  Kind kind = Kind::SECONDS;
  std::uint64_t amount = 0;
};

struct HuntOptions
{
  std::string selection = "all";
  std::optional<HuntNorm> norm;
  HuntRank rank = HuntRank::RATE;
  std::size_t top = 20;
  double noise = 10.0;
  std::size_t repeats = 3;
  std::uint64_t seed = 0;
  std::optional<HuntBudget> budget = std::nullopt;
};
```

| Name | Contract |
|---|---|
| `HuntNorm` | `\pre` none. `\post` `INSTR` divides by the instructions count of the same pass, `CYCLE` by the cycles count of the same pass, `ITER` by the iteration count of the attempt. No enumerator names `SENSITIVITY`, `DELTA` or `SCORE`: N14b adds those when its method exists (D-3) |
| `HuntRank` | `\pre` none. `\post` `RATE` orders by median rate descending, `CV` by coefficient of variation ascending; both break a tie by catalog address, so one sample list gives one order |
| `HuntBudget` | `\pre` `amount` is greater than zero. `\post` the record states the budget in the unit the caller named, and no conversion to the other unit happens here. `\invariant` `amount` above zero |
| `HuntOptions` | `\pre` `top` stands in 1 to 10000, `repeats` in 1 to 1000, `noise` above `0.0`. `selection` is non-empty for the hunt a record drives: `runHunt` requires it at `hunt.cpp:1101-1105`. The empty text is the spelling of hunt options that never met `--hunt`, the state the option scan parks at `hunt_cli.cpp:332-334` and `hunt.hpp:122` documents. `\post` the record holds exactly one run's settled values; a default-constructed record is the run `--hunt` names with no other hunt option (D-1, D-3, D-6 to D-9). `\invariant` `budget` is empty or holds an `amount` above zero |

`selection` holds the text the option carried, because the plan line and the
document repeat it. A `@file` selection holds the joined address lines rather
than the file path: a document that names a file the reader does not have
explains nothing.

## `EventCatalog` (E-01)

```text
struct CatalogAddress
{
  std::string address;
  std::string device;
  std::size_t source = 0;
};

class SPEEDGUN_NG_EXPORT EventCatalog
{
public:
  [[nodiscard]] auto expand(std::string_view selection)
      -> std::expected<void, counters::Error>;

  [[nodiscard]] auto addresses() const -> const std::vector<std::string>&;

  [[nodiscard]] auto split(std::uint64_t seed) const
      -> std::pair<std::vector<CatalogAddress>, std::vector<SkippedEvent>>;

  [[nodiscard]] static auto devicePart(std::string_view address)
      -> std::string_view;

  [[nodiscard]] auto isCoreDevice(std::string_view address) const -> bool;
};
```

| Name | Contract |
|---|---|
| `expand` | `\pre` `selection` holds `all`, one glob pattern, or a catalog address list whose entries are separated by commas, newlines or both; the text of an `@file` arrives as its address lines under the same separator rule. `\post` on success `addresses()` holds the selected catalog addresses in catalog order, deduplicated by address, for `all` and the pattern in catalog-scan order and for a list in the order of the text; an unknown address, a pattern that matches nothing, an empty address line and a raw event code shaped `r[0-9a-fA-F]+` are recoverable errors naming the value (D-10, D-11); no raw code resolves on any host (spec PC-6) |
| `addresses` | `\pre` `expand` returned no error. `\post` no address repeats, and every address has the spelling `Object::path() + "/" + entry.name` the catalog composes (spec PC-1, PC-8) |
| `split` | `\pre` `expand` returned no error. `\post` the first member holds one entry per address whose `Availability` is `COUNTABLE` in the order the seed shuffle of `--hunt-seed` gave, and the second member holds one `SkippedEvent` per refused entry carrying that entry's `Availability` as its kind; no address stands in both members, because the discovery answer decides membership uniquely |
| `devicePart` | `\pre` none. `\post` the answer is the text before the last `/`, or the whole address when it holds no `/`; `splitAddress` at `runner.cpp:39-47` answers the same way for the same spelling |
| `isCoreDevice` | `\pre` none. `\post` true for the device part `cpu`, `cpu_core` and `cpu_atom` (`source/counters/linux_pmu/counter_limits.cpp:284-295` matches the three and `source/counters/linux_pmu/provider.cpp:352,447-448` routes them, spec PC-1) and for a device part that starts with `armv8_` (the prefix rule of D-1; new hunt-side code with no library helper); false for every other device, uncore included (D-1) |
| `\invariant` | `addresses()` is empty until `expand` succeeds |

Discovery reads `System::local().object("machine")`, its `children()`, and
`Object::counters()` (`counters_system.hpp:47,109,241`; the walk
`catalog.cpp:81-88` already performs). `CatalogEntry` holds no address field, so
no other composition exists (spec PC-1).

## `planHunt` and its records (E-04)

```text
struct HuntColumn
{
  std::string event;
  std::string cycles;
  std::string instructions;
  std::size_t source = 0;
};

struct HuntStage
{
  std::vector<std::size_t> columns;
  std::size_t repeats = 1;
};

struct HuntPlan
{
  std::vector<HuntColumn> columns;
  std::vector<std::vector<std::size_t>> passes;
  std::optional<MetricMethod> method;
  std::vector<HuntStage> stages;
  std::optional<std::uint64_t> estimateNs;
  std::size_t anchorsPerPass = 0;
  bool normalized = true;
};

[[nodiscard]] auto planHunt(const std::vector<CatalogAddress>& events,
                            const std::vector<std::optional<std::size_t>>& sourceLimits,
                            const std::vector<std::pair<std::string, std::string>>& anchors,
                            const HuntOptions& options,
                            std::optional<std::uint64_t> calibrationNsPerPass)
    -> std::expected<HuntPlan, counters::Error>;
```

`\pre` `events` holds unique addresses in the order the caller asked for; the
`source` index of every entry stands within `sourceLimits`; `anchors` holds one
pair per device in `sourceLimits` order, a pair of empty strings where the device
lacks `cpu-cycles` or `instructions`; `options` satisfies its own precondition.

`\post` `columns` holds one column per entry of `events`, each naming the two
anchors of its own device (I-1); `passes` is the `planPasses` answer for the
columns and `sourceLimits`, so a pass that is not exclusive holds at most
`max(L - 2, 0)` events of a device that publishes `L`, and at most `L` leaves
counting its anchors (I-3, FR-011); `method` is the `settleMethod` answer, which
is `MULTIPLEX` exactly when no needed device publishes a limit (FR-011, D-19,
spec PC-2); `stages` holds one stage when `options.budget` is empty: every
column at `options.repeats`. With a budget `stages` holds two: stage 1 over
every column at one repeat and stage 2 over the leading `3 x top` columns of the
plan at `options.repeats` (D-2, FR-013, FR-014); the budget rule below drops
stage 2 whole when
the two-stage estimate does not fit; `estimateNs` holds
`calibrationNsPerPass x passes x repeats`, the stage-1 figures on a split plan,
or stands empty; `normalized` is false exactly when `anchorsPerPass` is zero.

`\post` (refusal) an event whose device publishes a limit below 3 with both
anchors present, a `norm` set to `instr` or `cycle` whose anchor of that name
stands refused (D-13), and a `source` index outside `sourceLimits` are each a
recoverable error naming the cause. The caller prints one line naming the value
and exits 2 (FR-081).

An empty `events` list is not a refusal: it is the shape of a host whose
discovery refused every event, and `planHunt` then answers zero columns, empty
`passes`, `anchorsPerPass` zero and `normalized` false, so the run prints the
plan line, the skipped list and the `events=0/N` summary and exits 0 (D-12,
FR-041).

`\invariant` `passes` covers every column index exactly once, so no column
stands in two passes.

`planHunt` reads no clock, no counter, no `/proc`, no sysfs and no CPUID
(I-7). It emits `std::vector<std::vector<ColumnLeaves>>` in the shape
`detail/pass_plan.hpp:41-107` states and calls `planPasses` unchanged: the
planner is width-agnostic and holds no column-count constant (spec PC-2), so a
plan of 200 columns reaches no change in it. `sourceLimits` is built per device
path from `Object::programmableCounters()` (`counters_system.hpp:85`), the one
source of the limit (spec PC-2).

**Budget rule** (D-2, FR-013, no reduction of its own): a budgeted plan runs
stage 1 over every event at one repeat and stage 2 over the leading `3 x top`
events of the plan order at `options.repeats`. No measured rate re-ranks that
set: FR-014 derives a stage from the seed and the catalog order only, so the
stages `planHunt` answers are the stages the run drives (PD-13). When the
two-stage estimate does not fit the budget, the plan drops stage 2 entirely and
the run prints one `budget cut` line naming the reason. Stage 1 never shrinks.

## `EventSamples`, `rankEvents` and the result records (E-05, E-06)

```text
enum class SkippedKind : std::uint8_t
{
  PERMISSION_BLOCKED,
  NOT_ENCODABLE,
  ABSENT,
  SCOPE_REFUSED,
  GAP
};

struct SkippedEvent
{
  std::string address;
  SkippedKind kind = SkippedKind::ABSENT;
};

struct HuntSample
{
  std::size_t pass = 0;
  std::size_t stage = 1;
  std::uint64_t eventValue = 0;
  std::optional<std::uint64_t> anchorInstr;
  std::optional<std::uint64_t> anchorCycle;
  std::uint64_t iterations = 0;
  std::optional<counters::Availability> disclosure;
};

struct EventSamples
{
  std::string address;
  std::vector<HuntSample> samples;
  std::optional<SkippedKind> skipped;
};

struct RankedEvent
{
  std::string address;
  double rate = 0.0;
  double cv = 0.0;
  std::size_t samples = 0;
  bool noisy = false;
  bool repeated = false;
  bool multiplexed = false;
  std::optional<std::size_t> rank;
};

struct EntryHunt
{
  std::optional<MetricMethod> method;
  std::size_t passCount = 0;
  std::size_t top = 20;
  std::vector<RankedEvent> entries;
  std::vector<SkippedEvent> skipped;
  std::size_t retriedPasses = 0;
};

[[nodiscard]] auto rankEvents(const std::vector<EventSamples>& collected,
                              const HuntOptions& options,
                              std::optional<MetricMethod> method) -> EntryHunt;
```

`SkippedKind` states the refusal of one event. The five enumerators are the
`Availability` values a skipped entry carries
(`counters_core.hpp:88-95`: `PERMISSION_BLOCKED`, `NOT_ENCODABLE`, `ABSENT`,
`SCOPE_REFUSED`, `GAP`) through one named mapping function (D-10, FR-021,
FR-050; the window gap state is disclosed at `counters_measurement.hpp:406-413`
through `fold` and `foldPairs`). A group-open refusal that still fails on the
solo retry carries the kind of the catalog state its refusal names, the state
the counters discovery probe settled for the entry (`provider.cpp:773-786`,
spec PC-1, T118): `EINVAL`, `EOPNOTSUPP` and `ENOENT` settle `NOT_ENCODABLE`,
every other errno settles `PERMISSION_BLOCKED`, and a refusal that names no
state settles `ABSENT` (D-18).
The console prints every kind with the spelling
`report.cpp:38 availabilityName` prints.

| Name | Contract |
|---|---|
| `rankEvents` `\pre` | every address in `collected` with no `skipped` kind holds at least one sample free of a disclosure, the guard `statisticsOf` carries at `statistics.cpp:23-25`; `method` is the method the plan that produced `collected` settled, `std::nullopt` for samples no plan produced; an address with exactly one such sample is reported and never handed to the summary call, so the deviation assert at `statistics.cpp:46-48` stays out of reach (PD-14) |
| `rankEvents` `\post` | one `RankedEvent` per address in `collected` and one `SkippedEvent` per address that carries a `skipped` kind; an address stands at most once in each list, and an address that measured and gapped stands in both (I-4, D-10); `rate` is the median of the per-pass normalized rates and `cv` is the coefficient of variation, both from the single `statisticsOf` call over those rates, so `cv` is one hundred times the `cv=` column ratio of the same samples (I-6); `noisy` is the comparison of the unrounded `cv` against `options.noise`; at most `options.top` entries carry a rank, those ranks are `1` to `k` with no gap, and an entry that is noisy or unrepeated carries none (I-5); `method` states the method the caller handed and `multiplexed` stands set for every entry exactly when that method is `MULTIPLEX`, so the rank route names one method for one table (AC-2, D-19); entries come first in rank order and then in address order |
| `rankEvents` `\post` (exclusion) | a sample whose anchor read zero for its pass contributes no rate for that pass, and a sample with a disclosure contributes none at all, the way a gap repetition stands out of `Statistics::samples` today |
| `rankEvents` `\invariant` | no address repeats in `entries`, and no address repeats in `skipped` |
| `RankedEvent` | `\pre` none: the record is data. `\post` `cv` is `0.0` when `samples` is one, and `rank` is empty when `noisy` is true or `repeated` is false |

`rankEvents` is pure: it reads no clock and no counter, and two calls over equal
arguments give equal results (I-7, FR-014, D-9). The method settles no figure of
the table: the rate, the deviation and the rank of an entry stand the same under
every method its caller hands.

## `runHunt` (public)

`runHunt` is public in `hunt.hpp` (D-15). It drives the harness runner behind its
own definition, whose `Runner` and `Instance` types stay harness internals
(`detail/internal.hpp:246,561`). The signature names public types alone: the
forward-declared `BenchmarkHandle`, `HuntOptions` and the entry record, and the
definition lives in `source/harness/hunt.cpp`:

```text
[[nodiscard]] auto runHunt(BenchmarkHandle& benchmark,
                           const HuntOptions& options,
                           std::optional<std::uint64_t> calibrationNsPerPass)
    -> std::expected<EntryHunt, counters::Error>;
```

`\pre` `options` satisfies its precondition, `benchmark` names a registered
entry, and the harness registered its providers so the catalog answers.

`\post` the plan ran through the existing N6 pass loop, so no second pass loop
and no second counter path exists (D-23, hard rule 15); every event that produced
no value stands in `skipped` with its kind and the run finished with exit status
0 (D-18); a pass whose group open refused retried its events one at a time inside
the same loop, and `retriedPasses` counts those passes; a seat that fails for
pass 0 keeps the existing failure route with exit 1, because it is the same
compile that serves warm-up and calibration (spec PC-9); `entries` covers every
planned column, an event that only stage 1 measured holding a record built from
its stage 1 samples. The harness run path assembles `ReportData::hunt` (the
E-08 `RunHunt`) from the plan and the stages that ran.

`\invariant` `EntryHunt::passCount` equals the pass count of the plan that ran,
counting the passes a retry appended.

## The two published fields

```text
// benchmark.hpp
std::optional<hunt::EntryHunt> hunt;  // in BenchmarkResult
std::optional<hunt::RunHunt> hunt;    // in ReportData (benchmark.hpp:629-650)
```

`\pre` none. `\post` both stand empty for a run that carried no hunt, which is
the fact the printers, the writer and the reader test (I-8); `loadReport` fills
both when a document carries the keys, so a hunt document round-trips without a
private side table (PD-10). `RunHunt` fields are in
[data-model.md](../data-model.md) E-08. Every record this contract declares is
public in `hunt.hpp`; the two records the run path keeps to itself,
`detail::StageSplit` and `detail::RetriedPass`, live in
`source/harness/detail/hunt_plan.hpp` with the stage and retry cores.

## Scripted refusal the tests depend on

```text
// counters_fake.hpp, exported class at line 103
auto setGroupLeafLimit(std::string_view objectPath, std::size_t leaves)
    -> FakeProvider&;
```

`\pre` `objectPath` names an object the script declares. `\post` `open` refuses a
leaf set that carries more leaves of that object than `leaves`, with the object
path and the leaf count in the message; an object with no scripted limit behaves
as it does today; the setter is fluent, and the last call for an object wins.
The scripted state joins the members of an exported `final` class, which is the
reason the archive `SOVERSION` moves to 4 (hard rule 12). The seam is required
and not speculative: `open` refuses an undeclared object or counter only and the
spine compile rejects an unknown leaf first, so no harness-level test reaches a
group-open refusal without it (spec PC-3, D-18).

## Verification

| Behaviour | Test |
|---|---|
| One column per event with both anchors of its device; the limit rows 1, 3, 4, 6, 8 and no limit; the 200 events at limit 6 row of AC-1 | `test/source/harness_hunt_plan_test.cpp` |
| Discovery order, dedup by address, the `isCoreDevice` rule including `armv8_`, the four discovery refusal kinds | `harness_hunt_plan_test.cpp`, `harness_hunt_collect_test.cpp` |
| Budget parse rows and the stage rule of D-2, the explicit-norm refusal of D-13, the below-3 device-limit refusal | `harness_hunt_plan_test.cpp`, `harness_hunt_cli_test.cpp` |
| Rate, `cv`, noise threshold, rank order, tie break by address, the single-sample rule, the factor-one-hundred scaling between a hunt `cv` and the printed `cv=` column | `harness_hunt_rank_test.cpp` |
| A budgeted run whose stage 2 measures fewer columns still publishes an entry and a rank-table row for a stage-1-only address | `harness_hunt_collect_test.cpp`, `harness_hunt_json_test.cpp`, `harness_hunt_report_test.cpp` |
| Retry inside the loop, `retriedPasses`, the exit-0 promise, the threaded window counts with no sleep | `harness_hunt_collect_test.cpp` |
| Nineteen clean windows plus one gapped window stand in `entries` and in `skipped` with kind `GAP` (D-10, FR-022) | `harness_hunt_collect_test.cpp` |
| Both records empty for a run with no `--hunt` | `harness_hunt_json_test.cpp`, `harness_hunt_report_test.cpp` |
| The scripted group refusal, wide set refused and narrow set admitted | `test/source/counters_fake_test.cpp` |
| Contract line pairing for every name above | the `dbc-gate` build target |

No test above sleeps, reads a clock, or assumes a CPU count: a test that needs a
CPU set calls `sched_getaffinity`, the way `harness_threads_test.cpp` does
(FR-092). Every branch above is reachable from the scripted provider, so none of
them carries an exclusion marker (FR-091).

## Scripted refusal message the tests read

```text
// counters_fake.hpp, exported class at line 103
auto lastRefusalMessage() const -> std::string;
```

`lastRefusalMessage` (`include/speedgun-ng/counters_fake.hpp:286`, defined at
`source/counters/fake_provider.cpp:351`) answers the message of the last
`open` the scripted host refused for a scripted leaf count, and an empty
message says no open was ever refused for one. `\pre` none, `\post` none: the
accessor reads the scripted state, so a test reads it after the refusal it
asks about and before the next `open`.
`test/source/harness_hunt_collect_test.cpp:853` reads the object path and the
leaf count out of that message, which is how the retry scenario names the
ceiling it hit (spec PC-3, D-18).
