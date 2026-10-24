#include <array>
#include <cstdio>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "speedgun-ng/hunt.hpp"

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"
#include "speedgun-ng/counters_fake.hpp"

// The hunt lines belong to the report printers of the library: `printHuntPlan`
// owns the plan line, `printHuntRanking` the ranked table, and `printHuntSkips`
// the skipped list with the plan summary that closes the block (FR-040, FR-041,
// PD-15). They are defined in `source/harness/report.cpp` below the report's
// own declaration and carry no header of their own, so they are declared here
// the way `test/source/harness_hunt_report_test.cpp` declares them: the
// definitions stand at namespace scope and this file names the same functions.
namespace sg::detail
{
auto printHuntPlan(const hunt::RunHunt& run) -> void;
auto printHuntRanking(const hunt::EntryHunt& hunt,
                      const hunt::HuntNorm norm) -> void;
auto printHuntSkips(const hunt::EntryHunt& hunt,
                    const hunt::RunHunt& run) -> void;
}  // namespace sg::detail

namespace
{

// The scripted host of the hunt: one core device, five events discovery seats
// and two it refuses, so one recorded run is the golden on any host (PD-19).
// The selection names the five ranked addresses and the two refusals
// outright, so the two anchor counters the plan normalizes by stay out of
// the ranked table.
constexpr std::string_view kDevice {"cpu"};
constexpr std::string_view kSelection {
    "cpu/hunt_branches,cpu/hunt_loads,cpu/hunt_rtls,cpu/hunt_stalls,"
    "cpu/hunt_stores,cpu/hunt_blocked,cpu/hunt_noenc"};
constexpr std::uint64_t kAnchor = 1'000'000;
constexpr std::size_t kLimit = 4;

constexpr std::array<std::pair<std::string_view, sg::counters::Availability>, 7>
    kScript {
        {{"hunt_branches", sg::counters::Availability::COUNTABLE},
         {"hunt_loads", sg::counters::Availability::COUNTABLE},
         {"hunt_rtls", sg::counters::Availability::COUNTABLE},
         {"hunt_stalls", sg::counters::Availability::COUNTABLE},
         {"hunt_stores", sg::counters::Availability::COUNTABLE},
         {"hunt_blocked", sg::counters::Availability::PERMISSION_BLOCKED},
         {"hunt_noenc", sg::counters::Availability::NOT_ENCODABLE}},
    };

// The script stands before the provider registers: the catalog freezes at the
// open boundary (FR-006, FR-009).
auto installScriptedHost() -> bool
{
  auto host = std::make_unique<sg::counters::FakeProvider>();
  host->addObject(kDevice, "core", "the scripted core the hunt walks");
  host->setProgrammableCounters(kDevice, kLimit);
  for (const auto& [leaf, availability] : kScript) {
    host->addCounter(kDevice, leaf, "none", "the scripted event", availability);
  }
  // The anchors every column of the plan rides, outside the selection.
  host->addCounter(kDevice, "cpu-cycles", "none", "the scripted cycles anchor");
  host->addCounter(
      kDevice, "instructions", "none", "the scripted instructions anchor");
  return sg::counters::System::local()
      .registerProvider(std::move(host))
      .has_value();
}

// The rows of the scripted columns are laid down here: a collected rate
// carries the sampling count of the host (PD-19). The window fields are
// pass, stage, event value, both anchors, iterations, disclosure.
auto collectedRows() -> std::vector<sg::hunt::EventSamples>
{
  const auto steps =
      std::vector<std::pair<std::string_view, std::vector<std::uint64_t>>> {
          {"hunt_branches", {2, 2, 2}},
          {"hunt_loads", {1, 1, 1}},
          {"hunt_rtls", {0}},
          {"hunt_stalls", {4, 2, 3}},
          {"hunt_stores", {0, 0, 0}},
      };
  auto collected = std::vector<sg::hunt::EventSamples> {};
  for (const auto& [leaf, windows] : steps) {
    auto samples = std::vector<sg::hunt::HuntSample> {};
    for (const auto window : windows) {
      samples.push_back(
          {samples.size() + 1, 1, window * kAnchor, kAnchor, kAnchor, 1, {}});
    }
    collected.push_back({std::string {kDevice} + "/" + std::string {leaf},
                         std::move(samples),
                         {}});
  }
  return collected;
}

// sg-example:begin hunt-rank
// The ranked table of a hunt, with the refusals discovery handed back folded
// into the skipped list of the record. `rankEvents` decides the order, the
// noise mark and the size of the table (FR-014, FR-021, I-5).
auto rankHuntEvents(const std::vector<sg::hunt::EventSamples>& collected,
                    const std::vector<sg::hunt::SkippedEvent>& refused,
                    const sg::hunt::HuntOptions& options) -> sg::hunt::EntryHunt
{
  // No plan collected these scripted rows, so the record states no method.
  auto hunt = sg::hunt::rankEvents(collected, options, std::nullopt);
  hunt.skipped.insert(hunt.skipped.end(), refused.begin(), refused.end());
  return hunt;
}

// sg-example:end hunt-rank

// The run-level record the three printers read. Every figure of a hunt line is
// a field of it, so this file states no line shape of its own: the plan the
// planner answered fills what the run path fills from the passes it measured
// (E-07, E-08, PD-10). The example hands the planner no calibration figure and
// names no budget, so the two fields carry E-07's words `not available` and
// `none` out of that record.
auto makeRunRecord(const sg::hunt::HuntPlan& plan,
                   const std::vector<sg::hunt::EventSamples>& collected,
                   const sg::hunt::HuntOptions& options) -> sg::hunt::RunHunt
{
  auto record = sg::hunt::RunHunt {};
  record.selection = options.selection;
  record.options = options;
  record.passes = plan.passes.size();
  record.method = plan.method;
  record.estimateNs = plan.estimateNs;
  for (const auto& column : plan.columns) {
    record.plannedAddresses.push_back(column.event);
  }
  for (const auto& stage : plan.stages) {
    record.stages.push_back(sg::hunt::HuntStageRange {
        .first = stage.columns.front(),
        .count = stage.columns.size(),
        .repeats = stage.repeats,
    });
  }
  // FR-041: the summary counts the addresses a window rated, and every scripted
  // column of this file arrives anchored and sampled, so the count is the
  // number of collected addresses.
  record.ranEvents = collected.size();
  return record;
}

}  // namespace

auto bmHunt(sg::State& state) -> void
{
  const auto cells = static_cast<std::uint64_t>(state.range(0));
  std::uint64_t sum = 0;
  for (auto _ : state) {
    for (std::uint64_t cell = 0; cell < cells; ++cell) {
      sum += cell;
    }
    sg::doNotOptimize(sum);
  }
}

SG_BENCHMARK(bmHunt).arg(8).arg(64);

// The block prints after the tables the way a run leaves it, and its rows
// come from the scripted records above, one fixed pass (PD-19).
auto main(int argc, char** argv) -> int
{
  if (!installScriptedHost()) {
    std::fprintf(stderr, "hunt_rank: the scripted host refused to register\n");
    return 1;
  }
  const int status = sg::speedgunMain(argc, argv);
  auto catalog = sg::hunt::EventCatalog {};
  if (const auto expanded = catalog.expand(kSelection); !expanded) {
    std::fprintf(stderr, "hunt: %s\n", expanded.error().message.c_str());
    return status != 0 ? status : 2;
  }
  auto options = sg::hunt::HuntOptions {};
  options.selection = kSelection;
  options.top = 2;
  const auto [events, refused] = catalog.split(0);
  // The limit and the anchors are the facts this file scripted above; the
  // planner reads them as data and settles the plan the line below prints.
  const auto limits =
      std::vector<std::optional<std::size_t>> {std::optional {kLimit}};
  const auto anchors = std::vector<std::pair<std::string, std::string>> {
      {{std::string {kDevice} + "/cpu-cycles",
        std::string {kDevice} + "/instructions"}},
  };
  const auto plan =
      sg::hunt::planHunt(events, limits, anchors, options, std::nullopt);
  if (!plan.has_value()) {
    std::fprintf(stderr, "hunt: %s\n", plan.error().message.c_str());
    return status != 0 ? status : 2;
  }
  const auto rows = collectedRows();
  const auto record = makeRunRecord(*plan, rows, options);
  const auto hunt = rankHuntEvents(rows, refused, options);
  // One call per line family, in the order the report prints them: the plan
  // line, the ranked table, then the skipped list with its summary (PD-15).
  sg::detail::printHuntPlan(record);
  sg::detail::printHuntRanking(
      hunt, record.options.norm.value_or(sg::hunt::HuntNorm::INSTR));
  sg::detail::printHuntSkips(hunt, record);
  return status;
}
