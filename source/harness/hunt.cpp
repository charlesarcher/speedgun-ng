// specs/029 FR-002 to FR-005, FR-021, FR-022: the event-catalog discovery and
// the hunt run driver. T022 defines `EventCatalog` here and T026 `runHunt`,
// both public in speedgun-ng/hunt.hpp; T036 adds the two-stage merge. T119:
// the driver prices one pass of the plan from `ResultRow::timePerIterationNs`
// of a window of the plan's own shape measured, so the planner holds a figure
// without reading a clock; no file of this layer reads one (FR-012, D-2, D-6).

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "speedgun-ng/hunt.hpp"

#include "detail/hunt_plan.hpp"
#include "detail/hunt_selection.hpp"
#include "detail/internal.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_system.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg::hunt
{

namespace
{

// D-1's core-device rule, written hunt-side: the three exact device parts and
// the `armv8_` prefix. The library's own match at `counter_limits.cpp:284-295`
// holds the three exact parts and publishes no helper.
constexpr std::array<std::string_view, 3> kCoreDevices {
    "cpu",
    "cpu_core",
    "cpu_atom",
};
constexpr std::string_view kCorePrefix {"armv8_"};

// PD-06: the generator the seed shuffle of `--hunt-seed` drives. The pair is
// the numerically reported constants, so one seed gives one order on every host
// (FR-014, D-9).
constexpr std::uint64_t kLcgMultiplier = 6364136223846793005ULL;
constexpr std::uint64_t kLcgIncrement = 1442695040888963407ULL;

// The position no leaf of a stage's run holds, the value a column of another
// stage carries for its event and for an anchor its device lacks.
constexpr std::size_t kNoLeaf = std::numeric_limits<std::size_t>::max();

[[nodiscard]] auto failure(std::string text) -> counters::Error
{
  return counters::Error {
      .message = std::move(text),
      .suggestions = {},
  };
}

[[nodiscard]] auto isSpace(const char symbol) noexcept -> bool
{
  return symbol == ' ' || symbol == '\t' || symbol == '\r' || symbol == '\v'
      || symbol == '\f';
}

[[nodiscard]] auto trim(std::string_view text) -> std::string_view
{
  while (!text.empty() && isSpace(text.front())) {
    text.remove_prefix(1);
  }
  while (!text.empty() && isSpace(text.back())) {
    text.remove_suffix(1);
  }
  return text;
}

/// @brief The address entries of a selection text, split on commas and
/// newlines with the spaces of each entry removed (FR-002, FR-006).
[[nodiscard]] auto splitEntries(const std::string_view selection)
    -> std::vector<std::string_view>
{
  std::vector<std::string_view> entries;
  std::size_t start = 0;
  while (true) {
    const std::size_t stop = selection.find_first_of(",\n", start);
    const std::size_t width =
        stop == std::string_view::npos ? std::string_view::npos : stop - start;
    entries.push_back(trim(selection.substr(start, width)));
    if (stop == std::string_view::npos) {
      break;
    }
    start = stop + 1;
  }
  return entries;
}

/// @brief D-1, PD-02: the core-device rule over one device part.
[[nodiscard]] auto coreDevicePart(const std::string_view device) noexcept
    -> bool
{
  return std::find(kCoreDevices.begin(), kCoreDevices.end(), device)
      != kCoreDevices.end()
      || device.starts_with(kCorePrefix);
}

// E-01: one device of the walked catalog: its path, the programmable-counter
// limit it publishes, and the two anchor addresses of its own. A device that
// lacks either anchor holds neither (I-1).
struct DeviceRow
{
  std::string path;
  std::optional<std::size_t> limit;
  std::string cycles;
  std::string instructions;
};

// E-01: one catalog entry in the walk's order, with the availability discovery
// disclosed for it (FR-002, FR-010).
struct CatalogRow
{
  std::string address;
  counters::Availability availability = counters::Availability::COUNTABLE;
};

// The walked catalog: the devices in walk order, which is the order of the
// limit list handed to the planner, and the entries in catalog-scan order.
struct CatalogView
{
  std::vector<DeviceRow> devices;
  std::vector<CatalogRow> rows;
};

/// @brief E-01: the index of one device in the walk order, the first position
/// winning a device that stands in more than one entry (spec PC-2).
[[nodiscard]] auto sourceOf(const CatalogView& view,
                            const std::string_view device) -> std::size_t
{
  const auto found = std::find_if(view.devices.begin(),
                                  view.devices.end(),
                                  [device](const DeviceRow& row)
                                  { return row.path == device; });
  // Every address of the walk names the device it was read from, so the device
  // stands in the list the same walk built (E-01, spec PC-8).
  SG_ASSERT(found != view.devices.end(),
            "the device part of a walked address names one walked device, so "
            "its index in the limit list stands");
  return static_cast<std::size_t>(found - view.devices.begin());
}

// The walk order of `detail::catalogWalkOrder`: every child object of the
// machine object contributes its entries and stands as one source of the limit
// list. The machine object's own entries are its clock leaves, which count no
// event, so they join neither list (E-01). The order of the walk is the
// catalog order and the order of the limit list (spec PC-2, PC-8).
auto walkCatalog(const counters::Object& root, CatalogView& view) -> void
{
  for (const auto* walked : detail::catalogWalkOrder(root)) {
    auto device = DeviceRow {.path = std::string(walked->path()),
                             .limit = walked->programmableCounters(),
                             .cycles = {},
                             .instructions = {}};
    for (const auto& entry : walked->counters()) {
      const std::string leaf = std::string(entry.name);
      const std::string address = device.path + "/" + leaf;
      if (leaf == detail::kCyclesLeaf) {
        device.cycles = address;
      } else if (leaf == detail::kInstrLeaf) {
        device.instructions = address;
      }
      view.rows.push_back(
          CatalogRow {.address = address, .availability = entry.avail});
    }
    if (device.cycles.empty() || device.instructions.empty()) {
      device.cycles.clear();
      device.instructions.clear();
    }
    view.devices.push_back(std::move(device));
  }
}

/// @brief The discovery read: `System::local().object("machine")` and its
/// `children()`, the walk `catalog.cpp` already performs. No clock, no counter
/// read, no sysfs and no CPUID stand here (FR-003, I-7).
[[nodiscard]] auto discoverCatalog() -> CatalogView
{
  const auto machine = counters::System::local().object("machine");
  // `System::System()` emplaces the `machine` root when the one system
  // instance is made and the counters layer offers no way to remove an
  // object, so the root of the walk stands for the life of the process.
  SG_ASSERT(machine.has_value(),
            "the counters system publishes the machine object the hunt "
            "selection walks");
  auto view = CatalogView {};
  walkCatalog(*machine, view);
  return view;
}

// E-01's invariant is that the expansion stands empty until `expand` answers
// no error, so `expand` fills a list of its own and commits it to the catalog
// only on the route that answers no error; the checks below read that committed
// list at the points the documented contract names (D-15: `hunt.hpp` states the
// surface this file implements and nothing else).
[[nodiscard]] [[maybe_unused]] auto holdsNoRepeat(
    const std::vector<std::string>& addresses) -> bool
{
  for (std::size_t left = 0; left < addresses.size(); ++left) {
    for (std::size_t right = left + 1; right < addresses.size(); ++right) {
      if (addresses[left] == addresses[right]) {
        return false;
      }
    }
  }
  return true;
}

/// @brief E-01, spec PC-1: true when one address carries the
/// `Object::path() + "/" + entry.name` spelling the catalog composes.
[[nodiscard]] [[maybe_unused]] auto carriesCatalogSpelling(
    const std::string_view address) -> bool
{
  // The last separator splits device from leaf: a device path of its own can
  // hold slashes, so only the one the catalog composed has to bound two
  // non-empty parts.
  const auto separator = address.rfind('/');
  return separator != std::string_view::npos && separator != 0U
      && separator + 1U < address.size();
}

/// @brief E-05, AC-10, contracts/hunt-api.md: true when one sample can give a
/// rate under the norm the rank reads.
///
/// `rateOf` answers a rate over a denominator that stands: the anchor of the
/// norm as that window read it, or the iteration count of a run that
/// normalizes by neither. A sample whose anchor read zero contributes no rate,
/// exactly as a disclosed one contributes none, so both stand in one predicate
/// here.
[[nodiscard]] auto yieldsRate(const HuntSample& sample,
                              const HuntNorm norm) -> bool
{
  if (sample.disclosure.has_value()) {
    return false;
  }
  if (norm == HuntNorm::ITER) {
    return sample.iterations != 0;
  }
  const auto anchor =
      norm == HuntNorm::CYCLE ? sample.anchorCycle : sample.anchorInstr;
  return anchor.value_or(0) != 0;
}

/// @brief E-05, I-4, FR-022, D-10, AC-10: the kind the skipped entry of one
/// measured column carries.
///
/// \pre `rows` holds at least one sample of one planned address; `rated` states
///      whether any of them gives a rate under the norm the rank reads
/// \post the first disclosure of the run answers its own kind through
///       `detail::skippedKindOf`, so a column that measured and gapped stands
///       in `entries` and in `skipped` at once (FR-022) and a column whose
///       every window disclosed answers the refusal the pass loop left on the
///       column (D-18, FR-021)
/// \post a column no window gave a rate carries `ABSENT`, the denominator that
///       is not there: `rankEvents` reads an address with no rate and no kind
///       as a broken precondition and aborts the process over it (AC-10, the
///       corsairone defect record)
[[nodiscard]] auto skippedOf(const EventSamples& rows,
                             const bool rated) -> std::optional<SkippedKind>
{
  const auto disclosed = std::find_if(
      rows.samples.begin(),
      rows.samples.end(),
      [](const HuntSample& sample) { return sample.disclosure.has_value(); });
  if (disclosed != rows.samples.end()) {
    return detail::skippedKindOf(*disclosed->disclosure);
  }
  return rated ? std::nullopt
               : std::optional<SkippedKind> {SkippedKind::ABSENT};
}

// One stage's run: the leaf list the run opens its columns in, and for each
// planned column the position of its event and of its two anchors inside that
// list, `kNoLeaf` where this stage measures the column not.
struct StageLeaves
{
  std::vector<std::string> leaves;
  std::vector<std::size_t> event;
  std::vector<std::size_t> cycles;
  std::vector<std::size_t> instructions;
};

/// @brief The position of one leaf in a stage's leaf list, the first position
/// winning an address two columns of the pass share - the anchors of a device
/// stand once per pass (I-2).
[[nodiscard]] auto leafPosition(std::vector<std::string>& leaves,
                                const std::string& address) -> std::size_t
{
  const auto found = std::find(leaves.begin(), leaves.end(), address);
  if (found != leaves.end()) {
    return static_cast<std::size_t>(found - leaves.begin());
  }
  leaves.push_back(address);
  return leaves.size() - 1;
}

[[nodiscard]] auto buildStageLeaves(const HuntPlan& plan,
                                    const HuntStage& stage) -> StageLeaves
{
  auto leaves = StageLeaves {
      .leaves = {},
      .event = std::vector<std::size_t>(plan.columns.size(), kNoLeaf),
      .cycles = std::vector<std::size_t>(plan.columns.size(), kNoLeaf),
      .instructions = std::vector<std::size_t>(plan.columns.size(), kNoLeaf),
  };
  for (const std::size_t column : stage.columns) {
    const HuntColumn& entry = plan.columns[column];
    leaves.event[column] = leafPosition(leaves.leaves, entry.event);
    if (!entry.cycles.empty()) {
      leaves.cycles[column] = leafPosition(leaves.leaves, entry.cycles);
    }
    if (!entry.instructions.empty()) {
      leaves.instructions[column] =
          leafPosition(leaves.leaves, entry.instructions);
    }
  }
  return leaves;
}

/// @brief T119, FR-012, FR-013: the nanoseconds one pass of `plan` costs, the
/// figure the estimate of the plan line weighs the pass count against.
///
/// \pre `instance` names a registered instance and `plan` holds at least one
///      pass
/// \post the figure is the real-time column - `ResultRow::timePerIterationNs`
///       - that one window of `plan` measured, driven
///       through the existing N6 pass loop of the `Runner` over the leaves of
///       that pass and the reads its window answers (D-23: no second pass loop
///       and no second counter path stands in this file)
/// \post no clock stands behind this figure: the time source gate (D-6)
///       reserves the monotonic clock for the counters layer, and
///       `ResultRow::timePerIterationNs` is that layer's own netted monotonic
///       fold of the window, in nanoseconds (FR-007)
/// \post `std::nullopt` when that pass refused its seat or measured nothing:
///       a host that runs no pass holds no figure to state, and the plan line
///       prints the estimate of a hunt with no figure as `not available`
///       (E-07)
[[nodiscard]] auto measurePassNs(sg::Instance& instance, const HuntPlan& plan)
    -> std::optional<std::uint64_t>
{
  if (plan.passes.empty()) {
    return std::nullopt;
  }
  const auto leaves = buildStageLeaves(plan,
                                       HuntStage {
                                           .columns = plan.passes.front(),
                                           .repeats = 1,
                                       });
  auto runOptions = detail::RunOptions {};
  runOptions.counterAddresses = leaves.leaves;
  runOptions.repetitions = 1;
  // One hunt window measures one iteration, the shape every hunt window has
  // (FR-013), so the figure prices a pass and not a long run.
  runOptions.fixedIterations = 1;
  runOptions.metricMethod = plan.method;
  const sg::BenchmarkResult result = detail::Runner {runOptions}.run(instance);
  if (result.outcome == sg::RunOutcome::FAILED || result.rows.empty()) {
    return std::nullopt;
  }
  const auto measured = result.rows.front().timePerIterationNs;
  if (measured <= 0.0) {
    return std::nullopt;
  }
  return static_cast<std::uint64_t>(measured);
}

// One pass's leaf list with the anchors of its own device joined: the seat
// compiles the events and the anchors, each counted once (I-2, I-3).
auto joinLeaf(std::vector<std::size_t>& pass, std::size_t position) -> void
{
  if (position == kNoLeaf
      || std::find(pass.begin(), pass.end(), position) != pass.end())
  {
    return;
  }
  pass.push_back(position);
}

/// @brief The plan one stage hands to the runner: the run's column space is the
/// stage's leaf list, so `passes` hold leaf positions and every seat compiles
/// one pass's events beside the anchors of their devices (D-23, I-3).
///
/// \pre `plan` holds the passes of the whole hunt and `leaves` its stage
/// \post `columns` holds one entry per leaf, an event entry naming the two
///       anchor addresses of its device and an anchor entry naming none, so the
///       retry of PD-12 appends one pass per event of a refused pass with both
///       anchors and nothing else; `passes` cover every leaf of every pass they
///       seat and no leaf repeats in one pass (I-2)
[[nodiscard]] auto buildHandoff(const HuntPlan& plan,
                                const HuntStage& stage,
                                const StageLeaves& leaves) -> HuntPlan
{
  auto handoff = HuntPlan {
      .columns = {},
      .passes = {},
      .method = plan.method,
      .stages = {HuntStage {
          .columns = {},
          .repeats = stage.repeats,
      }},
      .estimateNs = plan.estimateNs,
      .anchorsPerPass = plan.anchorsPerPass,
      .normalized = plan.normalized,
  };
  handoff.columns.reserve(leaves.leaves.size());
  for (const auto& leaf : leaves.leaves) {
    handoff.columns.push_back(HuntColumn {
        .event = leaf,
        .cycles = {},
        .instructions = {},
        .source = 0,
    });
  }
  for (std::size_t column = 0; column < plan.columns.size(); ++column) {
    const HuntColumn& entry = plan.columns[column];
    if (leaves.event[column] == kNoLeaf) {
      continue;
    }
    handoff.columns[leaves.event[column]].cycles = entry.cycles;
    handoff.columns[leaves.event[column]].instructions = entry.instructions;
    for (const std::size_t position : {leaves.event[column],
                                       leaves.cycles[column],
                                       leaves.instructions[column]})
    {
      if (position != kNoLeaf) {
        handoff.columns[position].source = entry.source;
      }
    }
    handoff.stages.front().columns.push_back(leaves.event[column]);
  }
  for (const auto& pass : plan.passes) {
    std::vector<std::size_t> seated;
    for (const std::size_t column : pass) {
      if (leaves.event[column] == kNoLeaf) {
        continue;
      }
      joinLeaf(seated, leaves.event[column]);
      joinLeaf(seated, leaves.cycles[column]);
      joinLeaf(seated, leaves.instructions[column]);
    }
    if (!seated.empty()) {
      handoff.passes.push_back(std::move(seated));
    }
  }
  return handoff;
}

/// @brief The anchor reading of one window, empty where the column's device
/// holds no such anchor or where that leaf disclosed the window (E-05).
[[nodiscard]] auto anchorReading(const sg::ResultRow& row,
                                 const std::size_t position)
    -> std::optional<std::uint64_t>
{
  if (position == kNoLeaf) {
    return std::nullopt;
  }
  SG_ASSERT(position < row.metrics.size(),
            "one row carries a metric slot per column of the run that built "
            "it, so an anchor position of the run stands in every row (FR-014)");
  const sg::MetricValue& anchor = row.metrics[position];
  if (anchor.availability != sg::counters::Availability::COUNTABLE) {
    return std::nullopt;
  }
  return static_cast<std::uint64_t>(anchor.value);
}

/// @brief E-05: the collected rows of one stage's run, one `HuntSample` per
/// measured repetition of one column, each with the anchor readings of its own
/// window, its iteration count and its disclosure.
///
/// \pre `leaves` names the columns of the stage that produced `rows`
/// \post a column the stage measures keeps the samples of this stage alone, so
///       stage 2 values win over stage 1 ones (PD-13); a column the stage does
///       not measure keeps the rows of the stage that measured it (I-4)
auto collectStageSamples(std::vector<EventSamples>& collected,
                         const StageLeaves& leaves,
                         const std::vector<sg::ResultRow>& rows,
                         const std::size_t stage) -> void
{
  for (std::size_t column = 0; column < collected.size(); ++column) {
    if (leaves.event[column] != kNoLeaf) {
      collected[column].samples.clear();
    }
  }
  for (const auto& row : rows) {
    if (row.kind != sg::RowKind::REPETITION) {
      continue;
    }
    for (std::size_t column = 0; column < collected.size(); ++column) {
      const std::size_t position = leaves.event[column];
      if (position == kNoLeaf) {
        continue;
      }
      SG_ASSERT(position < row.metrics.size(),
                "one row carries a metric slot per column of the run that "
                "built it, so an event position of the run stands in every row "
                "(FR-014)");
      const sg::MetricValue& value = row.metrics[position];
      auto sample = HuntSample {
          .pass = value.pass.value_or(0),
          .stage = stage,
          .eventValue = static_cast<std::uint64_t>(value.value),
          .anchorInstr = anchorReading(row, leaves.instructions[column]),
          .anchorCycle = anchorReading(row, leaves.cycles[column]),
          .iterations = row.iterations,
          .disclosure = {},
      };
      if (value.availability != sg::counters::Availability::COUNTABLE) {
        sample.disclosure = value.availability;
      }
      collected[column].samples.push_back(sample);
    }
  }
}

/// @brief D-13: the text of the refusal of an explicit norm whose anchor
/// discovery refused, empty where the norm plans.
///
/// The plan refuses the norm of a device that holds no anchor, its pair naming
/// two empty strings (D-13, the post of `planHunt`); what stands here is the
/// case only the run sees, an anchor address the catalog holds and discovery
/// refused, because `planHunt` reads no skipped list.
///
/// \pre `plan` was built for `options` over the events discovery counted
/// \post an unset norm or `ITER` answers the empty text; `INSTR` and `CYCLE`
///       answer a text naming the anchor of that name and the event of the
///       column that was to divide by it (D-13, FR-081)
[[nodiscard]] auto normAnchorRefusal(const HuntPlan& plan,
                                     const HuntOptions& options,
                                     const std::vector<SkippedEvent>& skipped)
    -> std::optional<std::string>
{
  if (!options.norm.has_value() || *options.norm == HuntNorm::ITER) {
    return std::nullopt;
  }
  const std::string_view word =
      *options.norm == HuntNorm::CYCLE ? "cycle" : "instr";
  for (const auto& column : plan.columns) {
    const std::string& anchor =
        *options.norm == HuntNorm::CYCLE ? column.cycles : column.instructions;
    const auto refused = std::find_if(skipped.begin(),
                                      skipped.end(),
                                      [&anchor](const SkippedEvent& entry)
                                      { return entry.address == anchor; });
    if (refused != skipped.end()) {
      return "the hunt norm " + std::string(word) + " names the anchor '"
          + anchor + "', which discovery refused for the hunt of '"
          + column.event + "'";
    }
  }
  return std::nullopt;
}

/// @brief FR-002, E-01: the expanded list one selection text answers, the
/// three shapes of FR-002 to FR-007 taking one named route each.
///
/// `all` takes the events of the core devices and excludes uncore (D-1); the
/// two anchor leaves of a device ride its passes as denominators and stand out
/// of the candidate list (I-1).
auto appendAll(const CatalogView& view,
               std::vector<std::string>& expanded) -> void
{
  for (const auto& row : view.rows) {
    const auto device = detail::devicePartOf(row.address);
    const auto leaf = detail::leafPartOf(row.address);
    if (coreDevicePart(device) && leaf != detail::kCyclesLeaf
        && leaf != detail::kInstrLeaf)
    {
      expanded.push_back(row.address);
    }
  }
}

/// @brief FR-007: the catalog addresses one glob pattern matches, in catalog
/// order; the refusal names the pattern that matched none.
[[nodiscard]] auto appendPattern(const CatalogView& view,
                                 const std::string_view pattern,
                                 std::vector<std::string>& expanded)
    -> std::optional<counters::Error>
{
  for (const auto& row : view.rows) {
    if (detail::globMatches(pattern, row.address)) {
      expanded.push_back(row.address);
    }
  }
  if (expanded.empty()) {
    return failure("the hunt pattern '" + std::string(pattern)
                   + "' matches no catalog address");
  }
  return std::nullopt;
}

/// @brief FR-002, FR-004, FR-006: the addresses of a comma and newline
/// separated list, in the order the text gave them; a repeated address keeps
/// its first position and drops the repeat (E-01).
[[nodiscard]] auto appendList(const CatalogView& view,
                              const std::vector<std::string_view>& entries,
                              std::vector<std::string>& expanded)
    -> std::optional<counters::Error>
{
  for (const auto entry : entries) {
    if (entry.empty()) {
      return failure("the hunt selection holds an empty address line (FR-006)");
    }
    const auto known = std::any_of(view.rows.begin(),
                                   view.rows.end(),
                                   [entry](const CatalogRow& row)
                                   { return row.address == entry; });
    if (!known) {
      return failure("the hunt selection names '" + std::string(entry)
                     + "', which the catalog holds no such address "
                       "(FR-004)");
    }
    if (std::find(expanded.begin(), expanded.end(), entry) == expanded.end()) {
      expanded.emplace_back(entry);
    }
  }
  return std::nullopt;
}

/// @brief The instance of one registered entry: the first expansion whose
/// name is the handle's own, or that handle's name with only the thread
/// segment on it, the instance a hunt of that handle drives (FR-092).
[[nodiscard]] auto instanceNamed(const std::string_view name) -> sg::Instance*
{
  for (auto& instance : sg::detail::instances()) {
    if (instance.name == name
        || (instance.name.size() > name.size() + 9U
            && instance.name.compare(0, name.size(), name) == 0
            && instance.name.compare(name.size(), 9, "/threads:") == 0))
    {
      return &instance;
    }
  }
  return nullptr;
}

}  // namespace

auto EventCatalog::expand(std::string_view selection)
    -> std::expected<void, counters::Error>
{
  // E-01: the expansion belongs to this catalog and to the selection it is
  // given, so a second `expand` replaces it and a refusal leaves none behind.
  m_addresses = std::nullopt;
  const auto entries = splitEntries(selection);
  for (const auto entry : entries) {
    if (detail::hasRawCodeShape(entry)) {
      const std::string text = "the hunt selection names the raw event code '"
          + std::string(entry) + "'; no host resolves a raw code (FR-005)";
      return std::unexpected(failure(text));
    }
  }
  const auto view = discoverCatalog();
  const auto whole = trim(selection);
  auto expanded = std::vector<std::string> {};
  if (whole == detail::kAllSelection) {
    appendAll(view, expanded);
  } else {
    const bool glob = whole.find_first_of("*?") != std::string_view::npos
        && whole.find_first_of(",\n") == std::string_view::npos;
    const auto refusal = glob ? appendPattern(view, whole, expanded)
                              : appendList(view, entries, expanded);
    if (refusal.has_value()) {
      // The half-built list stands only in `expanded`, so a refused selection
      // commits no expansion of its own (E-01).
      SG_INVARIANT(!m_addresses.has_value(),
                   "a refused selection commits no expansion, so the expansion "
                   "stands empty until `expand` answers no error (E-01)");
      return std::unexpected(*refusal);
    }
  }
  m_addresses = std::move(expanded);
  SG_ENSURE(holdsNoRepeat(*m_addresses),
            "the expansion holds each selected address once, because every "
            "selection shape deduplicates by address (E-01)");
  return {};
}

auto EventCatalog::addresses() const -> const std::vector<std::string>&
{
  SG_REQUIRE(m_addresses.has_value(),
             "the caller reads the expansion after `expand` answered no error "
             "(E-01)");
  SG_ENSURE(holdsNoRepeat(*m_addresses)
                && std::all_of(m_addresses->begin(),
                               m_addresses->end(),
                               carriesCatalogSpelling),
            "no address of the expansion repeats and each carries the "
            "`path` + `/` + `name` spelling the catalog composes (spec PC-1, "
            "PC-8)");
  return *m_addresses;
}

auto EventCatalog::split(const std::uint64_t seed) const
    -> std::pair<std::vector<CatalogAddress>, std::vector<SkippedEvent>>
{
  SG_REQUIRE(m_addresses.has_value(),
             "the split reads the availability of the addresses one `expand` "
             "answered without error (E-01, FR-010)");
  const auto view = discoverCatalog();
  // The expansion filled its list out of this same walk, so the walk stands for
  // the availability of every address of the expansion (FR-002, FR-010).
  auto answer =
      std::pair<std::vector<CatalogAddress>, std::vector<SkippedEvent>> {};
  for (const auto& address : addresses()) {
    const auto row = std::find_if(view.rows.begin(),
                                  view.rows.end(),
                                  [&address](const CatalogRow& candidate)
                                  { return candidate.address == address; });
    // Every address of the expansion is an address of the walk it came from.
    SG_INVARIANT(row != view.rows.end(),
                 "the split reads the availability of a walked address, so the "
                 "walk holds its entry (FR-010)");
    if (row->availability == counters::Availability::COUNTABLE) {
      answer.first.push_back(CatalogAddress {
          .address = address,
          .device = std::string(detail::devicePartOf(address)),
          .source = sourceOf(view, detail::devicePartOf(address)),
      });
    } else {
      answer.second.push_back(SkippedEvent {
          .address = address,
          .kind = detail::skippedKindOf(row->availability),
      });
    }
  }
  if (seed != 0) {
    // Fisher-Yates over the deduplicated order, the LCG of PD-06 driving the
    // draw; seed 0 keeps the catalog order (D-9, FR-014).
    std::uint64_t state = seed;
    for (std::size_t width = answer.first.size(); width > 1; --width) {
      state = (state * kLcgMultiplier) + kLcgIncrement;
      const auto draw = static_cast<std::size_t>(state % width);
      std::swap(answer.first[width - 1], answer.first[draw]);
    }
  }
  SG_ENSURE(answer.first.size() + answer.second.size()
                == addresses().size(),
            "one entry per address of the expansion stands in exactly one "
            "member of the split, so no address stands in both (FR-010)");
  return answer;
}

auto EventCatalog::devicePart(const std::string_view address)
    -> std::string_view
{
  const auto part = detail::devicePartOf(address);
  [[maybe_unused]] const auto leaf = address.substr(part.size());
  SG_ENSURE(leaf.empty() || leaf.find_last_of('/') == 0,
            "the device part is the text before the last `/`, or the whole "
            "address when it holds none (E-01)");
  return part;
}

auto EventCatalog::isCoreDevice(const std::string_view address) const -> bool
{
  const auto device = detail::devicePartOf(address);
  const bool core = coreDevicePart(device);
  SG_ENSURE(!core || !device.starts_with("uncore"),
            "the core-device rule of D-1 admits the core devices and keeps "
            "every uncore device out of the candidate list (D-1)");
  return core;
}

}  // namespace sg::hunt

namespace sg::detail
{

// The two enums are one ladder with the countable state at the head of the
// counters side, so the shift states the name-for-name mapping of D-10 and
// no kind table stands in between (FR-021, FR-050). The two pins below hold
// the head of each ladder at zero, the premise the shift of
// `skippedKindOf` rests on: a reordering that moves either head fails the
// build (T118).
static_assert(static_cast<std::size_t>(sg::counters::Availability::COUNTABLE)
              == 0U);
static_assert(
    static_cast<std::size_t>(sg::hunt::SkippedKind::PERMISSION_BLOCKED) == 0U);
static_assert(
    static_cast<std::size_t>(sg::counters::Availability::PERMISSION_BLOCKED)
    == static_cast<std::size_t>(sg::hunt::SkippedKind::PERMISSION_BLOCKED) + 1);
static_assert(
    static_cast<std::size_t>(sg::counters::Availability::NOT_ENCODABLE)
    == static_cast<std::size_t>(sg::hunt::SkippedKind::NOT_ENCODABLE) + 1);
static_assert(static_cast<std::size_t>(sg::counters::Availability::ABSENT)
              == static_cast<std::size_t>(sg::hunt::SkippedKind::ABSENT) + 1);
static_assert(
    static_cast<std::size_t>(sg::counters::Availability::SCOPE_REFUSED)
    == static_cast<std::size_t>(sg::hunt::SkippedKind::SCOPE_REFUSED) + 1);
static_assert(static_cast<std::size_t>(sg::counters::Availability::GAP)
              == static_cast<std::size_t>(sg::hunt::SkippedKind::GAP) + 1);

auto skippedKindOf(const sg::counters::Availability availability) noexcept
    -> sg::hunt::SkippedKind
{
  SG_ASSERT(availability != sg::counters::Availability::COUNTABLE,
            "the split hands over the refused states only, so the countable "
            "state never reaches the kind of a skipped entry (D-10)");
  return static_cast<sg::hunt::SkippedKind>(
      static_cast<std::uint8_t>(availability) - 1U);
}

// The words `availabilityName` of `source/counters/plan.cpp:110-129` puts
// into the refusal of a seat, each paired with the state it names. The
// counters layer keeps its own list file-local, so this table is the reader's
// side of one published sentence: it settles no state of its own, and every
// state in it is a state the discovery probe of
// `source/counters/linux_pmu/provider.cpp` answers for an entry. The order
// follows `Availability`.
constexpr std::array<std::pair<std::string_view, sg::counters::Availability>, 5>
    kRefusalStateWords {
        {{"permission_blocked", sg::counters::Availability::PERMISSION_BLOCKED},
         {"not_encodable", sg::counters::Availability::NOT_ENCODABLE},
         {"absent", sg::counters::Availability::ABSENT},
         {"scope_refused", sg::counters::Availability::SCOPE_REFUSED},
         {"gap", sg::counters::Availability::GAP}},
    };

auto availabilityOfRefusal(const sg::counters::Error& refusal) noexcept
    -> std::optional<sg::counters::Availability>
{
  // The counters layer composes the sentence below when the catalog state of
  // a leaf refuses the seat it asked for
  // (`source/counters/plan.cpp:485-490`). The state follows the sentence and
  // runs to the `;` that closes it, so the read starts at the sentence and no
  // leaf name, device path or error text of its own answers for the state.
  constexpr std::string_view kReportedLead = "the catalog reports ";
  const std::string_view message = refusal.message;
  const auto start = message.find(kReportedLead);
  if (start == std::string_view::npos) {
    return std::nullopt;
  }
  const auto begin = start + kReportedLead.size();
  const auto end = message.find(';', begin);
  const std::string_view word = message.substr(begin, end - begin);
  for (const auto& [stateWord, state] : kRefusalStateWords) {
    if (stateWord == word) {
      return state;
    }
  }
  return std::nullopt;
}

}  // namespace sg::detail

namespace sg::hunt
{

auto runHuntNamed(std::string_view name,
                  const HuntOptions& options,
                  std::optional<std::uint64_t> calibrationNsPerPass,
                  std::optional<MetricMethod> forcedMethod,
                  bool dryRun,
                  bool timedPass) -> std::expected<Hunted, counters::Error>
{
  auto* instance = instanceNamed(name);
  if (instance == nullptr) {
    return std::unexpected(
        failure("the hunt names '" + std::string(name)
                + "', which the expanded registry holds no instance for"));
  }
  auto catalog = EventCatalog {};
  if (const auto expanded = catalog.expand(options.selection);
      !expanded.has_value())
  {
    return std::unexpected(expanded.error());
  }
  const auto view = discoverCatalog();
  const auto [events, discoverySkips] = catalog.split(options.seed);

  std::vector<std::optional<std::size_t>> sourceLimits;
  std::vector<std::pair<std::string, std::string>> anchors;
  sourceLimits.reserve(view.devices.size());
  anchors.reserve(view.devices.size());
  for (const auto& device : view.devices) {
    // `Object::programmableCounters()` is the one source of the limit, and the
    // anchor pair of a device that lacks either anchor stands empty (spec PC-2,
    // I-1).
    sourceLimits.push_back(device.limit);
    anchors.emplace_back(device.cycles, device.instructions);
  }
  auto figure = calibrationNsPerPass;
  if (!figure.has_value() && !dryRun && timedPass) {
    // T119, FR-012: the estimate prices the passes of this plan against the
    // cost of one pass, and the planner reads no clock (D-2), so the figure
    // has to reach it as data. The sampling overhead a row publishes prices
    // one read of an already-open window: it leaves the seat, the window and
    // the reads of a pass unpriced, and that is the pair of orders a budget
    // cut decides on. So the hunt measures one pass of its own plan, through
    // the pass loop the stages run through (FR-013, D-23). The probe plan
    // stands only to name that pass: a figure moves the stages and the estimate
    // of a plan, never its columns or its passes, so the pass measured off the
    // probe is the pass the plan that runs seats.
    if (const auto probe =
            planHunt(events, sourceLimits, anchors, options, std::nullopt);
        probe.has_value())
    {
      figure = measurePassNs(*instance, *probe);
    }
  }
  auto planned = planHunt(events, sourceLimits, anchors, options, figure);
  if (!planned.has_value()) {
    return std::unexpected(planned.error());
  }
  if (forcedMethod.has_value()) {
    // D-16: a method the caller states outright outranks the settlement the
    // host's published limits would answer, and the plan carries the stated
    // word to the printers and the runner alike.
    planned->method = forcedMethod;
  }
  const HuntPlan& plan = *planned;
  if (const auto refusal = normAnchorRefusal(plan, options, discoverySkips);
      refusal.has_value())
  {
    return std::unexpected(failure(*refusal));
  }

  auto collected = std::vector<EventSamples> {};
  auto refused = std::vector<detail::RefusedRetry> {};
  collected.reserve(plan.columns.size());
  for (const auto& column : plan.columns) {
    collected.push_back(
        EventSamples {.address = column.event, .samples = {}, .skipped = {}});
  }
  auto runRecord = RunHunt {
      .selection = options.selection,
      .options = options,
      .passes = plan.passes.size(),
      .method = plan.method,
      .estimateNs = plan.estimateNs,
      .plannedAddresses = {},
      .stages = {},
      .ranEvents = 0,
      .stageTwoEvents = 0,
  };
  if (!plan.normalized && !runRecord.options.norm.has_value()) {
    // The record carries the norm the run measured under: a plan that found
    // no anchors ranks by iterations whoever asked (D-13).
    runRecord.options.norm = HuntNorm::ITER;
  }
  for (const auto& column : plan.columns) {
    runRecord.plannedAddresses.push_back(column.event);
  }

  // D-23, hard rule 15: the plan runs through the existing pass loop of the
  // Runner, one call per stage (PD-13). No second pass loop and no second
  // counter path stands in this file.
  std::size_t retriedPasses = 0;
  std::size_t lonePasses = 0;
  for (std::size_t index = 0; index < plan.stages.size(); ++index) {
    const HuntStage& stage = plan.stages[index];
    // `splitStages` owns the shape and enforces it on its own answer: stage 1
    // covers every column and stage 2 carries `min(3 x top, count)` of them,
    // with `top` and the column count both above zero (FR-013, D-2).
    SG_ASSERT(!stage.columns.empty(),
              "each stage of the split carries columns to measure (FR-013)");
    const auto leaves = buildStageLeaves(plan, stage);
    auto runOptions = detail::RunOptions {};
    runOptions.counterAddresses = leaves.leaves;
    // A dry-run hunt lets every column reach one sample: the repetition lane
    // stands cut to one, so an unrepeated column carries no rank (I-5).
    runOptions.repetitions = dryRun ? 1 : stage.repeats;
    runOptions.dryRun = dryRun;
    // One hunt window measures one iteration: the pass count and the repeats
    // carry the length of a hunt run, so the calibration grow stays out of the
    // way and one repetition opens exactly one window per pass (FR-013).
    runOptions.fixedIterations = 1;
    runOptions.hunt = options;
    // The method the plan settled is the method the run settles too: a forced
    // request settles itself through `settleMethod` (E-01 rows 4 and 5), so the
    // pass loop splits the stage's leaves the way the plan split its columns
    // (FR-011, D-19).
    runOptions.metricMethod = plan.method;

    const std::size_t retriedBefore = instance->huntRetriedPasses().size();
    const std::size_t refusedBefore = instance->huntRefusedEvents().size();
    instance->huntPlan = buildHandoff(plan, stage, leaves);
    auto runner = detail::Runner {runOptions};
    const sg::BenchmarkResult result = runner.run(*instance);
    instance->huntPlan = std::nullopt;
    if (result.outcome == sg::RunOutcome::FAILED) {
      // specs/029 spec PC-9: the stage that ends on the failure route ends
      // the hunt with it, so the run answers the error route and no record.
      return std::unexpected(failure(result.reason));
    }

    const auto& retried = instance->huntRetriedPasses();
    for (std::size_t record = retriedBefore; record < retried.size(); ++record)
    {
      lonePasses += retried[record].loneColumns.size();
    }
    retriedPasses += retried.size() - retriedBefore;
    const auto& refusedEvents = instance->huntRefusedEvents();
    for (std::size_t record = refusedBefore; record < refusedEvents.size();
         ++record)
    {
      refused.push_back(refusedEvents[record]);
    }

    collectStageSamples(collected, leaves, result.rows, index + 1);
    runRecord.stages.push_back(HuntStageRange {
        .first = stage.columns.front(),
        .count = stage.columns.size(),
        .repeats = stage.repeats,
    });
    if (index > 0) {
      runRecord.stageTwoEvents += stage.columns.size();
    }
  }

  // `rankEvents` takes the columns the run measured: its precondition names a
  // rate for every address that carries no skipped kind, so a column that
  // produced no sample at all - the run stopped mid-collection - joins no list
  // (E-06, PD-14) and a column no window could rate carries the kind its rate
  // would have stood beside (AC-10).
  auto measured = std::vector<EventSamples> {};
  measured.reserve(collected.size());
  const auto rankNorm = options.norm.value_or(HuntNorm::INSTR);
  for (auto& rows : collected) {
    if (rows.samples.empty()) {
      continue;
    }
    const auto rated = std::any_of(rows.samples.begin(),
                                   rows.samples.end(),
                                   [rankNorm](const HuntSample& sample)
                                   { return yieldsRate(sample, rankNorm); });
    rows.skipped = skippedOf(rows, rated);
    if (rated) {
      // FR-041: the summary counts the addresses a window rated, the
      // figure `hunt: plan summary events=` prints.
      ++runRecord.ranEvents;
    }
    measured.push_back(std::move(rows));
  }
  // The plan's settled method rides the record from the rank call, so the row
  // marks and the method of the record are one answer (AC-2, D-19).
  auto entry = rankEvents(measured, options, plan.method);
  entry.top = options.top;
  // The pass count of the plan that ran, the passes a retry appended included
  // (PD-12, D-18).
  entry.passCount = plan.passes.size() + lonePasses;
  // PD-12, D-18: the record counts every pass the run opened, so no collected
  // sample stands on a pass the record leaves out.
  for (const auto& rows : measured) {
    for ([[maybe_unused]] const auto& sample : rows.samples) {
      SG_INVARIANT(sample.pass < entry.passCount,
                   "the record counts the passes of the plan that ran, the "
                   "passes a retry appended included (PD-12, D-18)");
    }
  }
  entry.retriedPasses = retriedPasses;
  // specs/029 D-18, FR-021, T073, T118: the event whose solo retry refused
  // too stands in the skipped list with the kind of the state its own refusal
  // names, beside the halves the discovery and the disclosures already filled.
  // The refusal takes the place of a gap the same address collected with: a
  // pass that refused its seat leaves its columns gap-disclosed in the rows of
  // the passes that did run, and one address names one reason, the one the
  // host answered (I-4).
  for (const auto& refusal : refused) {
    std::erase_if(entry.skipped,
                  [&refusal](const SkippedEvent& skipped)
                  { return skipped.address == refusal.address; });
    entry.skipped.push_back(SkippedEvent {
        .address = refusal.address,
        .kind = detail::skippedKindOf(refusal.availability),
    });
  }
  // An address discovery refused can stand in no measured list, so the two
  // halves of the skipped list hold no address twice (I-4, FR-021).
  entry.skipped.insert(
      entry.skipped.end(), discoverySkips.begin(), discoverySkips.end());

  return Hunted {.entry = std::move(entry), .run = std::move(runRecord)};
}

auto runHunt(BenchmarkHandle& benchmark,
             const HuntOptions& options,
             std::optional<std::uint64_t> calibrationNsPerPass)
    -> std::expected<EntryHunt, counters::Error>
{
  SG_REQUIRE(options.top > 0 && options.repeats > 0 && options.noise > 0.0
                 && !options.selection.empty(),
             "the caller hands the settled values of one run, every count and "
             "threshold inside the range its own option states and the "
             "selection `--hunt` named");
  // The library route leaves `timedPass` down: the figure of its estimate is
  // the caller's to hand, and the driver runs no extra pass of the caller's
  // benchmark (T119, FR-012). The run-level record of the pair answers only
  // the internal seam: the public route keeps the entry record its signature
  // states (T126, PD-10).
  auto hunted = runHuntNamed(benchmark.name(), options, calibrationNsPerPass);
  SG_ENSURE(hunted.has_value() ? hunted->entry.top == options.top
                               : !hunted.error().message.empty(),
            "a hunt that ran states the top it ran with, and a refused hunt "
            "names the cause the caller prints (FR-081)");
  if (!hunted.has_value()) {
    return std::unexpected(std::move(hunted).error());
  }
  return std::move(hunted->entry);
}

}  // namespace sg::hunt
