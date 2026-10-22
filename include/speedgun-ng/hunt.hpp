#ifndef SG_HUNT_HPP
#define SG_HUNT_HPP

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

/**
 * @file hunt.hpp
 * @brief The hunter-mode surface of specs/029: the catalog selection,
 * the plan, the collected samples, the ranked result and the run
 * record.
 *
 * Feature 029-hunter-core. Namespace `sg::hunt`. The surface measures
 * nothing: it declares no clock, no counter read, no `/proc`, no sysfs
 * and no CPUID, so every time and count figure a record holds comes
 * from an argument, the catalog or the run that filled it (I-7,
 * FR-003). No internal harness type reaches any signature (D-15).
 */

namespace sg
{

// specs/029 D-15, the pattern source/harness/detail/pass_plan.hpp:27
// carries: the settled method and the harness entry handle are named
// here so hunt.hpp includes neither benchmark.hpp nor any harness
// header and no include cycle appears. The definitions stand in
// `speedgun-ng/benchmark.hpp`; the declaration names the type and not
// its enumerators, so a record that holds the settled method holds
// `std::optional<MetricMethod>`.
enum class MetricMethod : std::uint8_t;
class BenchmarkHandle;

}  // namespace sg

namespace sg::hunt
{

/**
 * @brief The denominator one hunt run normalizes an event count by
 * (E-02, D-6).
 *
 * The names are printed in lower case by the console and the JSON
 * writer through one internal name table each, indexed by the
 * enumerator (E-02).
 *
 * \pre none
 * \post `INSTR` divides by the instructions count of the same pass,
 *       `CYCLE` by the cycles count of the same pass, `ITER` by the
 *       iteration count of the attempt. No enumerator names
 *       `SENSITIVITY`, `DELTA` or `SCORE`: specs/031 adds those when
 *       its method exists (D-3)
 */
enum class HuntNorm : std::uint8_t
{
  INSTR,
  CYCLE,
  ITER
};

/**
 * @brief The key one hunt run orders its ranks by (E-02, D-3).
 *
 * \pre none
 * \post `RATE` orders by median rate descending, `CV` by coefficient
 *       of variation ascending; both break a tie by catalog address,
 *       so one sample list gives one order
 */
enum class HuntRank : std::uint8_t
{
  RATE,
  CV
};

/**
 * @brief The bound one hunt run collects under, in the unit the
 * caller named (E-03, FR-012).
 *
 * A budget of `120s` is `{SECONDS, 120}` and `8p` is `{PASSES, 8}`;
 * a value with no suffix is refused by the CLI before this record
 * exists, so the kind is never a default the caller forgot.
 *
 * \pre `amount` is greater than zero
 * \post the record states the budget in the unit the caller named,
 *       and no conversion to the other unit happens here
 * \invariant `amount` above zero
 */
struct HuntBudget
{
  enum class Kind : std::uint8_t
  {
    SECONDS,
    PASSES
  };

  Kind kind = Kind::SECONDS;
  std::uint64_t amount = 0;
};

/**
 * @brief The settled values of one hunt run (E-03, D-1, D-3, D-6 to
 * D-9).
 *
 * `selection` holds the text the option carried, because the plan
 * line and the document repeat it; a `@file` selection holds the
 * joined address lines, because a document
 * that names a file the reader does not have explains nothing. The
 * CLI refuses a value outside the range its option states before
 * this record exists.
 *
 * \pre `top` stands in 1 to 10000, `repeats` in 1 to 1000, `noise`
 *      above `0.0`, `selection` non-empty
 * \post the record holds exactly one run's settled values; a
 *       default-constructed record is the run `--hunt` names with no
 *       other hunt option (D-1, D-3, D-6 to D-9)
 * \invariant `budget` is empty or holds an `amount` above zero
 */
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

// E-06's refusal kinds stand ahead of E-01 because
// EventCatalog::split answers a SkippedEvent list, and ahead of E-05
// because EventSamples carries one. The ranked records of E-06 stand
// with the entry record below.

/**
 * @brief The refusal of one event, as one skipped entry carries it
 * (E-06, D-10, FR-021, FR-050).
 *
 * The five enumerators are the `counters::Availability` values a
 * skipped entry carries, reached through one named mapping function.
 * `GAP` names the mid-run window disclosure. A group-open refusal
 * that still fails on the solo retry maps its error the way the
 * counters discovery probe maps an open refusal: `EINVAL` or
 * `EOPNOTSUPP` answers `NOT_ENCODABLE`, and `EACCES` or `EPERM`
 * answers `PERMISSION_BLOCKED` (D-18, spec PC-1). The console prints
 * every kind with the spelling `report.cpp availabilityName` prints.
 */
enum class SkippedKind : std::uint8_t
{
  PERMISSION_BLOCKED,
  NOT_ENCODABLE,
  ABSENT,
  SCOPE_REFUSED,
  GAP
};

/**
 * @brief One event the run will not measure, with the kind of the
 * refusal (E-06, D-10).
 */
struct SkippedEvent
{
  std::string address;
  SkippedKind kind = SkippedKind::ABSENT;
  std::optional<std::size_t> stage;
};

/**
 * @brief One catalog address with the device that owns it and the
 * index of that device in the limit list (E-01).
 *
 * The address has the spelling `Object::path() + "/" + entry.name`
 * the catalog composes; `CatalogEntry` holds no address field, so no
 * other composition exists (spec PC-1).
 */
struct CatalogAddress
{
  std::string address;
  std::string device;
  std::size_t source = 0;
};

/**
 * @brief The selection of events a hunt run takes, expanded against
 * the catalog (E-01).
 *
 * One instance per hunt run. Discovery reads
 * `System::local().object("machine")`, its `children()` and
 * `Object::counters()`, the walk `catalog.cpp` already performs; no
 * clock and no counter read enter this class (FR-003).
 *
 * \invariant `addresses()` is empty until `expand` succeeds
 */
class SPEEDGUN_NG_EXPORT EventCatalog
{
public:
  /**
   * @brief Expands the selection text into the catalog addresses it
   * selects, in catalog order.
   *
   * \pre `selection` holds `all`, one glob pattern, or a catalog
   *      address list whose entries are separated by commas, newlines
   *      or both; the text of an `@file` arrives as its address lines
   *      under the same separator rule
   * \post on success `addresses()` holds the selected catalog
   *       addresses in catalog order, deduplicated by address, for
   *       `all` and the pattern in catalog-scan order and for a list
   *       in the order of the text; an unknown address, a pattern
   *       that matches nothing, an empty address line and a raw event
   *       code shaped `r[0-9a-fA-F]+` are recoverable errors naming
   *       the value (D-10, D-11); no raw code resolves on any host
   *       (spec PC-6)
   */
  [[nodiscard]] auto expand(std::string_view selection)
      -> std::expected<void, counters::Error>;

  /**
   * @brief The deduplicated selection in catalog order (E-01).
   *
   * \pre `expand` returned no error
   * \post no address repeats, and every address has the spelling
   *       `Object::path() + "/" + entry.name` the catalog composes
   *       (spec PC-1, PC-8)
   */
  [[nodiscard]] auto addresses() const -> const std::vector<std::string>&;

  /**
   * @brief Splits the selection into the columns the plan seats and
   * the entries discovery refused, under the seed shuffle of
   * `--hunt-seed` (E-01, FR-002, FR-010, FR-014).
   *
   * \pre `expand` returned no error
   * \post the first member holds one entry per address whose
   *       `Availability` is `COUNTABLE` in the order the seed shuffle
   *       of `--hunt-seed` gave, and the second member holds one
   *       `SkippedEvent` per refused entry carrying that entry's
   *       `Availability` as its kind; no address stands in both
   *       members, because the discovery answer decides membership
   *       uniquely
   */
  [[nodiscard]] auto split(std::uint64_t seed) const
      -> std::pair<std::vector<CatalogAddress>, std::vector<SkippedEvent>>;

  /**
   * @brief The device part of one address, the text before the last
   * `/` (E-01).
   *
   * \pre none
   * \post the answer is the text before the last `/`, or the whole
   *       address when it holds no `/`; `splitAddress` at
   *       `runner.cpp:39-47` answers the same way for the same
   *       spelling
   */
  [[nodiscard]] static auto devicePart(std::string_view address)
      -> std::string_view;

  /**
   * @brief True when the device of one address is a core device
   * (E-01, D-1).
   *
   * \pre none
   * \post true for the device part `cpu`, `cpu_core` and `cpu_atom`
   *       and for a device part that starts with `armv8_` (the prefix
   *       rule of D-1, new hunt-side code with no library helper);
   *       false for every other device, uncore included (D-1)
   */
  [[nodiscard]] auto isCoreDevice(std::string_view address) const -> bool;
};

/**
 * @brief The plan's one column: one event and the two anchor
 * addresses of its own device (E-04, I-1).
 *
 * Both anchor addresses are empty for a device that lacks either
 * anchor (I-1). `source` is the index of the event's device in the
 * limit list handed to the planner.
 */
struct HuntColumn
{
  std::string event;
  std::string cycles;
  std::string instructions;
  std::size_t source = 0;
};

/**
 * @brief One stage of the plan: the column indices it measures and
 * the repeats each column gets (E-04, D-2).
 *
 * Stage 1 covers every column at one repeat of a budgeted plan;
 * stage 2 covers the top `3 x top` columns at `options.repeats`.
 * Stage 1 never shrinks and no stage keeps a reduced repeat count
 * (FR-013).
 */
struct HuntStage
{
  std::vector<std::size_t> columns;
  std::size_t repeats = 1;
};

/**
 * @brief The plan one hunt run executes (E-04).
 *
 * `passes` holds the column indices of each pass as the planner
 * answered them; a pass that is not exclusive holds at most
 * `max(L - 2, 0)` events of a device that publishes `L` and at most
 * `L` leaves counting its anchors (I-3). `anchorsPerPass` is 2 when
 * both anchors exist, else 0, and `normalized` is false exactly
 * then: the plan normalized by iterations because a device lacked an
 * anchor.
 */
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

/**
 * @brief One measured repetition of one column of one pass, with the
 * anchor readings of its own window (E-05).
 *
 * A sample with a disclosure stands out of the rate list, the way a
 * gap repetition stands out of `Statistics::samples` today.
 */
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

/**
 * @brief The collected rows of one event address, the input of the
 * rank table (E-05).
 *
 * A test that scripted a machine builds these records directly, so
 * the rank decision table reaches every branch with no PMU. The
 * rows stand in pass then stage order.
 */
struct EventSamples
{
  std::string address;
  std::vector<HuntSample> samples;
  std::optional<SkippedKind> skipped;
};

/**
 * @brief One row of the ranked table, with the marks the console
 * tail prints (E-06).
 *
 * `rate` is the median of the per-pass normalized rates and `cv` is
 * the coefficient of variation as a percent, one hundred times the
 * printed `cv=` ratio of the same samples (I-6).
 *
 * \pre none: the record is data
 * \post `cv` is `0.0` when `samples` is one, and `rank` is empty
 *       when `noisy` is true or `repeated` is false
 */
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

/**
 * @brief The published per-benchmark hunt result (E-06).
 *
 * `rankEvents` and `runHunt` fill it and `BenchmarkResult::hunt`
 * holds it. `entries` covers every planned column, the ranked ones
 * first; an event only stage 1 measured holds a record built from
 * its stage 1 samples (I-4).
 */
struct EntryHunt
{
  std::optional<MetricMethod> method;
  std::size_t passCount = 0;
  std::size_t top = 20;
  std::vector<RankedEvent> entries;
  std::vector<SkippedEvent> skipped;
  std::size_t retriedPasses = 0;
};

/**
 * @brief The range of planned addresses one stage measured (E-08).
 *
 * The JSON writer spells the three fields `first`, `count` and
 * `repeats` of the closed `hunt.stages` node.
 */
struct HuntStageRange
{
  std::size_t first = 0;
  std::size_t count = 0;
  std::size_t repeats = 0;
};

/**
 * @brief The run-level hunt record on `ReportData::hunt` (E-08,
 * PD-10).
 *
 * The harness run path assembles it from the plan and the stages
 * that ran; it stands empty for a run that carried no hunt (I-8).
 */
struct RunHunt
{
  std::string selection;
  HuntOptions options;
  std::size_t passes = 0;
  std::optional<MetricMethod> method;
  std::optional<std::uint64_t> estimateNs;
  std::vector<std::string> plannedAddresses;
  std::vector<HuntStageRange> stages;
  std::size_t ranEvents = 0;
  std::size_t stageTwoEvents = 0;
};

/**
 * @brief The plan one hunt run executes, over the discovered events,
 * the per-device limits and the anchors (E-04, FR-011, FR-013).
 *
 * `events` comes from `EventCatalog::split`; `sourceLimits` holds one
 * entry per device: the programmable-counter count that device
 * publishes, or nothing when it cannot answer, built per device path
 * from `Object::programmableCounters()`, the one source of the limit
 * (spec PC-2). `anchors` holds the `cpu-cycles` and `instructions`
 * addresses per device. `calibrationNsPerPass` feeds `estimateNs`
 * alone. An empty `events` list is not a refusal: it is the shape of
 * a host whose discovery refused every event, and the call then
 * answers zero columns, empty `passes`, `anchorsPerPass` zero and
 * `normalized` false, so the run prints the plan line, the skipped
 * list and the `events=0/N` summary and exits 0 (D-12, FR-041). The
 * call adds no pass loop of its own and reads no clock, no counter,
 * no `/proc`, no sysfs and no CPUID (FR-020, I-7).
 *
 * \pre `events` holds unique addresses in the order the caller asked
 *      for; the `source` index of every entry stands within
 *      `sourceLimits`; `anchors` holds one pair per device in
 *      `sourceLimits` order, a pair of empty strings where the device
 *      lacks `cpu-cycles` or `instructions`; `options` satisfies its
 *      own precondition
 * \post `columns` holds one column per entry of `events`, each naming
 *       the two anchors of its own device (I-1); `passes` is the
 *       `planPasses` answer for the columns and `sourceLimits`, so a
 *       pass that is not exclusive holds at most `max(L - 2, 0)`
 *       events of a device that publishes `L`, and at most `L` leaves
 *       counting its anchors (I-3, FR-011); `method` is the
 *       `settleMethod` answer, which is `MULTIPLEX` exactly when no
 *       needed device publishes a limit (FR-011, D-19, spec PC-2);
 *       `stages` holds one stage when `options.budget` is empty:
 *       every column at `options.repeats`; with a budget `stages`
 *       holds two: stage 1 over every column at one repeat and stage
 *       2 over the top `3 x top` columns at `options.repeats` (D-2,
 *       FR-013), the budget rule dropping stage 2 whole when the
 *       two-stage estimate does not fit; `estimateNs` holds
 *       `calibrationNsPerPass x passes x repeats`, the stage-1
 *       figures on a split plan, or stands empty; `normalized` is
 *       false exactly when `anchorsPerPass` is zero
 * \post an event whose device publishes a limit below 3 with both
 *       anchors present, a `norm` set to `instr` or `cycle` whose
 *       anchor of that name stands refused (D-13), and a `source`
 *       index outside `sourceLimits` are each a recoverable error
 *       naming the cause; the caller prints one line naming the value
 *       and exits 2 (FR-081)
 * \invariant `passes` covers every column index exactly once, so no
 *            column stands in two passes
 */
[[nodiscard]] auto planHunt(
    const std::vector<CatalogAddress>& events,
    const std::vector<std::optional<std::size_t>>& sourceLimits,
    const std::vector<std::pair<std::string, std::string>>& anchors,
    const HuntOptions& options,
    std::optional<std::uint64_t> calibrationNsPerPass)
    -> std::expected<HuntPlan, counters::Error>;

/**
 * @brief The ranked table of the collected samples: one entry per
 * address, the skipped list beside it (E-06, I-5, I-6).
 *
 * The call is pure: it reads no clock and no counter, and two calls
 * over equal arguments give equal results (I-7, FR-014, D-9).
 *
 * \pre every address in `collected` with no `skipped` kind holds at
 *      least one sample free of a disclosure, the guard
 *      `statisticsOf` carries at `runner.cpp:52-55`; an address with
 *      exactly one such sample is reported and never handed to the
 *      summary call, so the deviation assert at `runner.cpp:74-76`
 *      stays out of reach (PD-14)
 * \post one `RankedEvent` per address in `collected` and one
 *       `SkippedEvent` per address that carries a `skipped` kind; an
 *       address stands at most once in each list, and an address that
 *       measured and gapped stands in both (I-4, D-10); `rate` is the
 *       median of the per-pass normalized rates and `cv` is the
 *       coefficient of variation, both from the single `statisticsOf`
 *       call over those rates, so `cv` is one hundred times the `cv=`
 *       column ratio of the same samples (I-6); `noisy` is the
 *       comparison of the unrounded `cv` against `options.noise`; at
 *       most `options.top` entries carry a rank, those ranks are `1`
 *       to `k` with no gap, and an entry that is noisy or unrepeated
 *       carries none (I-5); `multiplexed` states the settled method;
 *       entries come first in rank order and then in address order
 * \post a sample whose anchor read zero for its pass contributes no
 *       rate for that pass, and a sample with a disclosure
 *       contributes none at all, the way a gap repetition stands out
 *       of `Statistics::samples` today
 * \invariant no address repeats in `entries`, and no address repeats
 *            in `skipped`
 */
[[nodiscard]] auto rankEvents(const std::vector<EventSamples>& collected,
                              const HuntOptions& options) -> EntryHunt;

/**
 * @brief The hunt of one registered entry: plan, collection over the
 * harness pass loop, and ranked result (E-06, D-15, D-23).
 *
 * Public in this header (D-15); the `Runner` and `Instance` types it
 * drives behind its definition in `source/harness/hunt.cpp` stay
 * harness internals, and the signature names public types alone.
 *
 * \pre `options` satisfies its precondition, `benchmark` names a
 *      registered entry, and the harness registered its providers so
 *      the catalog answers
 * \post the plan ran through the existing N6 pass loop, so no second
 *       pass loop and no second counter path exists (D-23, hard rule
 *       15); every event that produced no value stands in `skipped`
 *       with its kind and the run finished with exit status 0 (D-18);
 *       a pass whose group open refused retried its events one at a
 *       time inside the same loop, and `retriedPasses` counts those
 *       passes; a seat that fails for pass 0 keeps the existing
 *       failure route with exit 1, because it is the same compile
 *       that serves warm-up and calibration (spec PC-9); `entries`
 *       covers every planned column, an event that only stage 1
 *       measured holding a record built from its stage 1 samples
 * \invariant `EntryHunt::passCount` equals the pass count of the plan
 *            that ran, counting the passes a retry appended
 */
[[nodiscard]] auto runHunt(BenchmarkHandle& benchmark,
                           const HuntOptions& options,
                           std::optional<std::uint64_t> calibrationNsPerPass)
    -> std::expected<EntryHunt, counters::Error>;

}  // namespace sg::hunt

#endif  // SG_HUNT_HPP
