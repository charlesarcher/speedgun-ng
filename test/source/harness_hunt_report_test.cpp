// ============================================================================
// The hunt console tables (specs/029-hunter-core T006, filled by T121; US2;
// FR-012, FR-013, FR-040, FR-041, FR-052; SC-06, SC-08;
// research PD-13, PD-14, PD-15; data-model E-07, I-8;
// quickstart.md US2 walkthrough).
//
// The seam is the printer: a scenario builds or runs the record and reads the
// bytes `sg::detail::printResult` sends to the process stdout, captured to a
// scratch file the way `harness_pass_report_test.cpp` captures them, so every
// claim is a printed byte of the report. The line shapes are
// the seven rows of data-model E-07 in the PD-15 order, compared character for
// character.
//
// One scenario per shape, the shape named in the scenario name the harness
// prints. Ten scenarios drive a real run of the scripted host of
// `detail/hunt_fixture.hpp`, the host the CLI lane scripts the same way, so the
// bytes they compare are the bytes a reader of `--hunt` holds. Every figure of
// an expected line follows from the fixture's own arithmetic: the event of
// index k folds `huntValue(k)` per window, the anchors of `cpu` fold
// `huntInstr(1)`, and the rate scales that ratio by one thousand (D-6), so
// `cpu/hunt002` stands at `instr=75.000` because 300 / 4000 x 1000 answers 75.
// No expectation below is a copy of what a run happened to print. Every
// scripted window folds the same figure, so a column the run repeated has no
// spread and prints `cv=0.00`.
//
// The figure no run of this file states is the estimate of the plan line: the
// nanoseconds of the pass the hunt timed (T119), so its digits belong to the
// host. They stand behind the `<host-calibrated>` token this file's captures
// substitute, the same stand-in
// `test/source/detail/pass_identity.expected` carries for the floor of a
// finished row, and every byte around them is compared. The walkthrough row is
// the one case whose estimate digits stand compared whole, and it stands there
// because the record carries the calibration the walkthrough states.
//
// Three arms stand pinned off a record the scenario builds, the way
// `harness_hunt_cli_test.cpp` pins the cut reason, because the run that
// answers them leaves the bytes of the line unpinnable: the `noisy` mark,
// since a scripted window folds one constant and no scripted `cv` passes
// the noise threshold; the `estimate=not available` arm, which only the
// entry of a dry run answers, since a hunt that measured no pass holds no
// figure (T119); and the walkthrough row of the AC-7 plan, whose 96'000'000 ns
// is the walkthrough's own 1'920'000 ns calibration over 50 passes, a figure no
// run of this host carries (D-2, FR-012, FR-013). Each states its reason where
// it is pinned. The budget scenarios of specs/033 stand on a built record the
// same way: a clause text, a derived count and a group size are figures the
// planner answers, so each of them hands over the record whose bytes it
// compares (FR-020 to FR-023).
//
// The list below is what CTest runs, so an entry absent from it never runs, and
// `argv[1]` selects one entry of it. Each entry runs in a re-exec of this
// binary, the pattern `harness_hunt_collect_test.cpp` carries: the counters
// system takes one object path per process, and one scenario drives a host no
// other scenario of this file drives. Frameworkless check()/fail() convention,
// the one `harness_pass_report_test.cpp` carries.
// ============================================================================

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "detail/hunt_fixture.hpp"
#include "detail/hunt_topdown.hpp"
#include "detail/internal.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"
#include "speedgun-ng/counters_fake.hpp"
#include "speedgun-ng/counters_system.hpp"
#include "speedgun-ng/counters_topdown.hpp"
#include "speedgun-ng/hunt.hpp"

// `detail/internal.hpp` declares the plan printer, where the test lane reads
// it. The two printers that stand below it print the ranked table and the
// skipped list from `report.cpp` with no header of their own, so they are
// declared here the way `harness_hunt_plan_test.cpp` declares the budget body
// of `hunt_cli.cpp`: the definition lives at namespace scope and the test
// names the same function.
namespace sg::detail
{
auto printHuntRanking(const hunt::EntryHunt& hunt,
                      const hunt::HuntNorm norm) -> void;
auto printHuntSkips(const hunt::EntryHunt& hunt,
                    const hunt::RunHunt& run) -> void;
}  // namespace sg::detail

namespace
{

// The scratch file every capture of this binary reads back. The name stands
// in the system temporary directory under this process number, so a run from
// the checkout root leaves no file behind and a second copy of this binary
// shares no file.
auto scratchPath() -> std::filesystem::path
{
  return std::filesystem::temp_directory_path()
      / ("harness_hunt_report_" + std::to_string(static_cast<long>(getpid()))
         + "_out.txt");
}

// The token that stands for the figure of a hunt line that belongs to the
// host: the estimate of the plan line, the nanoseconds of one pass the hunt
// timed (T119).
constexpr std::string_view kCalibrated = "<host-calibrated>";

// The scripted host: the hunt fixture at the limit of 6 the AC-1 row states,
// with one event leaf re-scripted permission-blocked so a selected address can
// reach the skipped list. The counters system takes these object paths once per
// process, so the limit a scenario asks for is the limit of the process that
// runs it, and `main` gives every scenario a process of its own.
constexpr std::size_t kHostLimit = 6;
constexpr std::size_t kRefusedEvent = 7;

// The other host this file drives: the same fixture with no device publishing a
// programmable-counter limit, the shape D-19 reads. The scenario that asks for
// it holds its process alone, so the shape reaches no other scenario here.
constexpr std::optional<std::size_t> kNoLimitHost = std::nullopt;

// The one benchmark case the runs filter down to.
constexpr const char* kCaseName = "bmHuntReport";

// The row the US2 walkthrough prints: the AC-7 plan of the 200 catalog columns
// at the AC-1 limit of 6, priced by a calibration of 1'920'000 ns for one
// pass. `harness_hunt_plan_test.cpp` proves these figures at record level in
// `overBudgetCut`, so the printed case below and that verdict read one plan.
constexpr std::size_t kQuickstartEvents = sg::test::kHuntEvents;
constexpr std::size_t kQuickstartPasses = 50;
constexpr std::uint64_t kQuickstartCalibrationNs = 1'920'000;
constexpr std::uint64_t kQuickstartBudgetPasses = 8;

// The columns that plan reaches on the host of this file: the 200 leaves of the
// scripted catalog beside the one leaf the host refuses outright.
constexpr std::size_t kAcSevenColumns = sg::test::kHuntEvents - 1;

[[noreturn]] auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS HUNT REPORT TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool condition, const char* what) -> void
{
  if (!condition) {
    fail(what);
  }
}

struct Scenario
{
  std::string_view name;
  void (*body)();
};

// ---------------------------------------------------------------------------
// The scripted host and the captured run
// ---------------------------------------------------------------------------

// The address of one scripted event, in the spelling the catalog composes.
[[nodiscard]] auto address(const std::size_t index) -> std::string
{
  return sg::test::huntAddress(sg::test::kHuntCpu, sg::test::huntEvent(index));
}

// `--hunt=` text of the events named, in the order the scenario lists them.
[[nodiscard]] auto selection(const std::vector<std::size_t>& indexes)
    -> std::string
{
  std::string text;
  for (const auto index : indexes) {
    if (!text.empty()) {
      text += ',';
    }
    text += address(index);
  }
  return text;
}

// The host of this process, laid down once: the fixture at `limit` beside the
// refused leaf of `kRefusedEvent`, which is the address one scenario selects so
// that the skipped line and the `events=ran/planned+refused` denominator of the
// summary print (FR-041, D-12). An empty `limit` leaves every device unable to
// answer a programmable-counter count (spec PC-2). The counters system takes an
// object path once per process, so a request for a second shape fails the
// scenario that asks for it in place of driving the host already laid.
auto ensureHost(const std::optional<std::size_t> limit) -> void
{
  static bool laid = false;
  static std::optional<std::size_t> laidLimit;
  if (laid) {
    check(laidLimit == limit,
          "the host of this process already stands at another limit");
    return;
  }
  laid = true;
  laidLimit = limit;
  auto provider = std::make_unique<sg::counters::FakeProvider>();
  auto& host = sg::test::huntHost(*provider, limit);
  host.addCounter(sg::test::kHuntCpu,
                  sg::test::huntEvent(kRefusedEvent),
                  "none",
                  "a refused event",
                  sg::counters::Availability::PERMISSION_BLOCKED);
  const auto settled =
      sg::counters::System::local().registerProvider(std::move(provider));
  check(settled.has_value(), "the hunt fixture registers on the local system");
}

auto bmHuntReport(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto ensureCase() -> void
{
  static const bool kRegistered = []
  {
    auto handle = sg::registerBenchmark(&bmHuntReport, kCaseName);
    handle.iterations(1);
    return true;
  }();
  static_cast<void>(kRegistered);
}

// One byte of the printed report, the redirection seam of
// `harness_hunt_cli_test.cpp`: stdout points at the scratch file for the span
// of the entry point and the scenario reads the file back, so every claim
// below weighs bytes the process wrote and nothing else. `hostLimit` names the
// host the run stands on, the limit of 6 by default.
[[nodiscard]] auto captureRun(const std::vector<std::string>& arguments,
                              std::optional<std::size_t> hostLimit = kHostLimit)
    -> std::string
{
  ensureHost(hostLimit);
  ensureCase();

  std::vector<char*> argv {const_cast<char*>(kCaseName)};
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  const int saved = dup(STDOUT_FILENO);
  if (saved < 0 || std::freopen(scratchPath().c_str(), "w", stdout) == nullptr)
  {
    fail("cannot redirect the report");
  }
  const int status =
      sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
  std::fflush(stdout);
  if (dup2(saved, STDOUT_FILENO) < 0) {
    fail("cannot restore stdout");
  }
  close(saved);
  if (status != 0) {
    std::fprintf(stderr, "speedgunMain returned %d, expected 0\n", status);
    fail("the hunt run of the scenario did not answer status 0");
  }

  std::ifstream file(scratchPath());
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// The command line of every scenario: the one case of this file, one
// iteration, so a run costs what its own hunt plan costs.
[[nodiscard]] auto withCase(const std::vector<std::string>& arguments)
    -> std::vector<std::string>
{
  std::vector<std::string> full {
      "--filter", std::string("^") + kCaseName + "$", "--iterations=1"};
  for (const auto& argument : arguments) {
    full.push_back(argument);
  }
  return full;
}

// `--hunt=` text of the whole scripted catalog: the 200 columns of the AC-7
// row, in catalog order.
[[nodiscard]] auto allSelection() -> std::string
{
  std::vector<std::size_t> indexes;
  indexes.reserve(sg::test::kHuntEvents);
  for (std::size_t index = 0; index < sg::test::kHuntEvents; ++index) {
    indexes.push_back(index);
  }
  return selection(indexes);
}

// The bytes of one print, captured the way `captureRun` captures a run - the
// seam of `harness_hunt_cli_test.cpp` for a call that needs no run.
template<typename Print>
[[nodiscard]] auto capturePrint(const Print& print) -> std::string
{
  const int saved = dup(STDOUT_FILENO);
  if (saved < 0 || std::freopen(scratchPath().c_str(), "w", stdout) == nullptr)
  {
    fail("cannot redirect the printer");
  }
  print();
  std::fflush(stdout);
  if (dup2(saved, STDOUT_FILENO) < 0) {
    fail("cannot restore stdout");
  }
  close(saved);
  std::ifstream file(scratchPath());
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// ---------------------------------------------------------------------------
// The report text read line by line
// ---------------------------------------------------------------------------

// The host-priced figure of a line behind the token that stands for it: the
// digits of a settled `estimate=` are masked, so the unit stays visible and
// the `not available` arm of the field keeps its bytes (T119, E-07).
[[nodiscard]] auto maskFigure(std::string line) -> std::string
{
  constexpr std::string_view kField = "estimate=";
  const auto at = line.find(kField);
  if (at == std::string::npos
      || !std::isdigit(static_cast<unsigned char>(line[at + kField.size()])))
  {
    return line;
  }
  auto stop = at + kField.size();
  while (stop < line.size()
         && std::isdigit(static_cast<unsigned char>(line[stop])))
  {
    ++stop;
  }
  line.replace(at + kField.size(), stop - (at + kField.size()), kCalibrated);
  return line;
}

// The hunt lines of a capture, in the order they printed, the token standing
// for each figure of the host: the block is what the shape scenarios compare
// byte for byte, and it carries every hunt line and no other line of the
// report.
[[nodiscard]] auto huntBlock(const std::string& text) -> std::string
{
  std::string block;
  std::istringstream stream(text);
  std::string line;
  while (std::getline(stream, line)) {
    if (line.rfind("hunt: ", 0) == 0) {
      block += maskFigure(line);
      block += '\n';
    }
  }
  return block;
}

// The line a needle first stands on, counted from the first line of the
// report, or `npos` when no line holds it - the position seam of
// `harness_hunt_cli_test.cpp`, which is how PD-15 is readable from the bytes.
[[nodiscard]] auto lineIndex(const std::string& text,
                             const std::string& needle) -> std::size_t
{
  const std::size_t hit = text.find(needle);
  if (hit == std::string::npos) {
    return std::string::npos;
  }
  std::size_t index = 0;
  for (std::size_t at = 0; at < hit; ++at) {
    if (text[at] == '\n') {
      ++index;
    }
  }
  return index;
}

// The first row of the case of this file: a line that starts with its name and
// carries the iteration figure of a run. A position weighed against the plan
// line is only PD-15 if it is a row, so the scenario names the line it means.
[[nodiscard]] auto firstRowIndex(const std::string& text) -> std::size_t
{
  std::istringstream stream(text);
  std::string line;
  std::size_t index = 0;
  while (std::getline(stream, line)) {
    if (line.rfind(kCaseName, 0) == 0
        && line.find("iterations=") != std::string::npos)
    {
      return index;
    }
    ++index;
  }
  return std::string::npos;
}

[[nodiscard]] auto countOf(const std::string& text,
                           const std::string& needle) -> std::size_t
{
  std::size_t count = 0;
  for (std::size_t at = text.find(needle); at != std::string::npos;
       at = text.find(needle, at + needle.size()))
  {
    ++count;
  }
  return count;
}

// The plan line of a capture, masked the way the block masks it.
[[nodiscard]] auto planLine(const std::string& text) -> std::string
{
  std::istringstream stream(huntBlock(text));
  std::string line;
  while (std::getline(stream, line)) {
    if (line.rfind("hunt: plan events=", 0) == 0) {
      return line;
    }
  }
  return {};
}

// The plan figures of the contract example of contracts/hunt-cli.md: 14 passes
// under a 120-second budget. The caller names the planned addresses and the
// estimate, so a scenario that pins a field no run can answer hands over the
// record the contract states for it. The two budgets of specs/033, the state
// the sample budget left on the plan and the groups the cap split are figures
// of that kind: the scenario that pins a budget line names them, and the
// scenario that pins the standing bytes leaves the defaults.
[[nodiscard]] auto contractRecord(
    const std::optional<std::uint64_t>& estimateNs,
    const std::vector<std::string>& plannedAddresses,
    const std::size_t ranEvents,
    const std::optional<std::uint64_t>& sampleBudgetNs = std::nullopt,
    const std::optional<std::size_t>& multiplexCap = std::nullopt,
    const sg::hunt::HuntSampleState& sample = {},
    const std::vector<std::size_t>& muxGroups = {}) -> sg::hunt::RunHunt
{
  return sg::hunt::RunHunt {
      .selection = "all",
      .options =
          sg::hunt::HuntOptions {
              .selection = "all",
              .norm = std::nullopt,
              .rank = sg::hunt::HuntRank::RATE,
              .top = 20,
              .noise = 10.0,
              .repeats = 3,
              .seed = 0,
              .budget =
                  sg::hunt::HuntBudget {
                      .kind = sg::hunt::HuntBudget::Kind::SECONDS,
                      .amount = 120,
                  },
              .sampleBudgetNs = sampleBudgetNs,
              .multiplexCap = multiplexCap,
          },
      .passes = 14,
      .method = sg::MetricMethod::PASSES,
      .estimateNs = estimateNs,
      .plannedAddresses = plannedAddresses,
      .stages = {},
      .ranEvents = ranEvents,
      .stageTwoEvents = 0,
      .cutReason = "over-budget",
      .sample = sample,
      .muxGroups = muxGroups,
  };
}

// ---------------------------------------------------------------------------
// E-07 row 1: the plan line, and its two estimate arms
// ---------------------------------------------------------------------------

// FR-012, FR-040, E-07: the plan line of one hunt run prints the planned
// column count, the passes of the plan, the settled method word, the estimate
// and the budget - and nothing else: no stage field stands on a plan of one
// stage, no normalization field on a plan that found its anchors.
auto scenarioPlanLineShape() -> void
{
  const auto run = captureRun(withCase({"--hunt=" + selection({0, 1, 2})}));
  check(planLine(run)
            == "hunt: plan events=3 passes=1 method=PASSES estimate="
                   "<host-calibrated>ns budget=none",
        "the plan line prints E-07 row one, one pass of four events per pass "
        "at the limit of 6, and no field the run did not settle (FR-012)");

  // The other arm of the estimate field: the nanoseconds of a pass the hunt
  // timed, so a hunt that ran nothing to time prints the word. The entry of a
  // dry run is the route that answers it (T119); the byte shape of the line
  // stands pinned here on the record the contract states.
  const auto record = contractRecord(std::nullopt, {}, 0);
  check(capturePrint([&record] { sg::detail::printHuntPlan(record); })
            == "hunt: plan events=0 passes=14 method=PASSES "
               "estimate=not available budget=120s\n",
        "a record with no estimate prints the word the contract states for the "
        "field, beside the budget in the unit it was given (FR-012, E-07)");
  std::printf("plan-line-shape: ok\n");
}

// ---------------------------------------------------------------------------
// The two-stage plan line (FR-013, D-2, E-07)
// ---------------------------------------------------------------------------

// A budgeted plan names both stages with the events each measures: stage 1
// covers every column at one repeat and stage 2 the leading three times the
// top, so four events under a top of 1 answer `stage1=4 stage2=3`. A budget
// that holds those runs cuts nothing (FR-013).
auto scenarioPlanLineTwoStage() -> void
{
  const auto run = captureRun(withCase({"--hunt=" + selection({0, 1, 2, 3}),
                                        "--hunt-top=1",
                                        "--hunt-budget=4p"}));
  check(planLine(run)
            == "hunt: plan events=4 passes=1 method=PASSES estimate="
                   "<host-calibrated>ns budget=4p stage1=4 stage2=3",
        "the plan line of a plan that fits its budget names both stages with "
        "the event count each carries (FR-013, E-07, US2 scenario 2)");
  check(countOf(run, "hunt: plan budget cut") == 0,
        "a plan that fits its budget prints no cut line (FR-013)");
  std::printf("plan-line-two-stage: ok\n");
}

// ---------------------------------------------------------------------------
// The walkthrough row: the AC-7 plan line above its cut line (quickstart.md,
// FR-012, FR-013, E-07, D-2)
// ---------------------------------------------------------------------------

// The 8-pass budget over the AC-7 plan: the 50 passes of stage 1 stand, the 3
// passes of the 12-column stage-2 set at 3 repeats do not fit on top of them,
// so the record holds one stage and the printer answers the cut line directly
// under the plan line. The estimate is the walkthrough's own arithmetic, the
// nanoseconds of one pass over the 50 passes of stage 1 at one repeat, so the
// field carries the scripted figure here and a run of the same plan carries
// its host's. That is the one reason this case stands on the record the
// walkthrough states (D-2, FR-012, FR-013).
auto scenarioQuickstartPlanCut() -> void
{
  auto record = contractRecord(kQuickstartCalibrationNs * kQuickstartPasses,
                               sg::test::huntCatalog(),
                               kQuickstartEvents);
  record.passes = kQuickstartPasses;
  record.options.budget = sg::hunt::HuntBudget {
      .kind = sg::hunt::HuntBudget::Kind::PASSES,
      .amount = kQuickstartBudgetPasses,
  };
  record.stages = {sg::hunt::HuntStageRange {
      .first = 0,
      .count = kQuickstartEvents,
      .repeats = 1,
  }};
  check(capturePrint([&record] { sg::detail::printHuntPlan(record); })
            == "hunt: plan events=200 passes=50 method=PASSES "
               "estimate=96000000ns budget=8p\n"
               "hunt: plan budget cut stage=2 reason=over-budget\n",
        "the printed plan line carries the walkthrough figures of the AC-7 "
        "plan: 200 columns, the 50 passes of stage 1, the estimate those "
        "passes price, the 8-pass budget, and the cut line naming the dropped "
        "stage directly below it (quickstart.md, FR-012, FR-013, E-07)");
  std::printf("quickstart-plan-cut: ok\n");
}

// ---------------------------------------------------------------------------
// The plan line of the AC-7 row under a budget that fits (FR-013, E-07, D-2)
// ---------------------------------------------------------------------------

// The other arm of the walkthrough row, driven as a run over the scripted
// catalog: the 60-pass budget hosts the 50 passes of stage 1 plus the 3 passes
// of the 12-column stage-2 set at 3 repeats, 59 runs in all, so the plan keeps
// both stages and drops nothing. The runs the two stages cost are a figure of
// the plan, so the verdict holds whatever the host prices a pass at. The host
// of this file refuses `cpu/hunt007` outright, so the 200 leaves of the
// catalog reach 199 planned columns beside one skipped line, and 199 columns
// still fill 50 passes of 4 columns at the limit of 6. The figure that
// separates this case from the 4-pass case above is the budget field beside the
// two stage fields: `budget=60p` here over 199 columns of stage 1, where that
// case reads `budget=4p` over 4 columns of stage 1.
auto scenarioPlanLineBudgetFits() -> void
{
  const auto run = captureRun(withCase(
      {"--hunt=" + allSelection(), "--hunt-top=4", "--hunt-budget=60p"}));
  check(planLine(run)
            == "hunt: plan events=199 passes=50 method=PASSES estimate="
                   "<host-calibrated>ns budget=60p stage1=199 stage2=12",
        "the plan line of the AC-7 plan under the 60-pass budget names both "
        "stages with the counts of the walkthrough row: every planned column "
        "in stage 1, the 3 x top set of 12 in stage 2 (FR-013, E-07, SC-07)");
  check(countOf(run, "hunt: plan budget cut") == 0,
        "the 60-pass budget hosts the 59 runs the two stages cost, so the run "
        "prints no cut line at all (FR-013, D-2)");
  check(countOf(run, "hunt: rank ") == kAcSevenColumns,
        "the two stages measure into one row set: the table carries one row "
        "for every planned column (PD-13, I-4)");
  check(countOf(run,
                 "hunt: plan summary events=199/200 passes=50 budget=60p")
            == 1,
        "the summary carries the 199 measured columns over the 200 the hunt "
        "stood before, with the budget in the passes it was given (FR-041, "
        "D-12)");
  std::printf("plan-line-budget-fits: ok\n");
}

// ---------------------------------------------------------------------------
// The MULTIPLEX method line (D-16, D-19, E-07)
// ---------------------------------------------------------------------------

// A run whose plan settled the multiplexing method prints the capital word in
// the plan line and the lowercase word beside every row, because a method the
// caller stated outright outranks the settlement the published limits would
// answer (D-16).
auto scenarioPlanLineMultiplex() -> void
{
  const auto run = captureRun(
      withCase({"--hunt=" + selection({0, 1}), "--metric-method=multiplex"}));
  check(planLine(run)
            == "hunt: plan events=2 passes=1 method=MULTIPLEX estimate="
                   "<host-calibrated>ns budget=none",
        "the plan line carries the method word the data model spells for a "
        "multiplexed hunt (D-19, E-07)");
  const auto block = huntBlock(run);
  const std::string expectedRows {
      "hunt: rank 1 cpu/hunt001 instr=50.000 cv=0.00 correlation=n/a "
          "slope=n/a delta=n/a area=unmapped multiplexed\n"
      "hunt: rank 2 cpu/hunt000 instr=25.000 cv=0.00 correlation=n/a "
          "slope=n/a delta=n/a area=unmapped multiplexed\n"};
  check(block.find(expectedRows) != std::string::npos,
        "every row of the multiplexed hunt carries the word, in the rank order "
        "of the rate column (D-19, PD-07)");
  std::printf("plan-line-multiplex: ok\n");
}

// ---------------------------------------------------------------------------
// The plan line of a host that publishes no limit (D-19, D-20, E-07)
// ---------------------------------------------------------------------------

// The settlement the published limits answer, with no option stating a method
// and no budget in play: the host of this process publishes no
// programmable-counter limit at all, so no needed device publishes one and the
// plan settles the multiplexing method on its own reading (D-19, spec PC-2).
// `harness_hunt_collect_test.cpp` reads that word off `hunt.method` on the same
// host shape; here it stands in the printed plan line, and the scenario holds
// the process the shape needs. The rate of a row divides by the instructions
// anchor of `cpu`, 4'000, so 300 answers 75.000, 200 answers 50.000 and 100
// answers 25.000 (I-1, D-6).
auto scenarioPlanLineNoLimitHost() -> void
{
  const auto run =
      captureRun(withCase({"--hunt=" + selection({0, 1, 2})}), kNoLimitHost);
  check(huntBlock(run)
            == "hunt: plan events=3 passes=1 method=MULTIPLEX estimate="
                   "<host-calibrated>ns budget=none\n"
                   "hunt: rank 1 cpu/hunt002 instr=75.000 cv=0.00 "
                   "correlation=n/a slope=n/a delta=n/a area=unmapped "
                   "multiplexed\n"
                   "hunt: rank 2 cpu/hunt001 instr=50.000 cv=0.00 "
                   "correlation=n/a slope=n/a delta=n/a area=unmapped "
                   "multiplexed\n"
                   "hunt: rank 3 cpu/hunt000 instr=25.000 cv=0.00 "
                   "correlation=n/a slope=n/a delta=n/a area=unmapped "
                   "multiplexed\n"
                   "hunt: plan summary events=3/3 passes=1 budget=none\n",
        "the plan line of a hunt whose host publishes no limit carries the "
        "settled method word with no option stating it, every row carries the "
        "word of the multiplexed plan beside the figure its own anchor folds, "
        "and no budget stands anywhere (D-19, D-20, E-07)");
  check(countOf(run, "hunt: skipped ") == 0,
        "the one multiplexed pass measured every column of the plan, so no "
        "column stands in the skipped line beside the three rows (D-19, PD-07)");
  std::printf("plan-line-no-limit-host: ok\n");
}

// ---------------------------------------------------------------------------
// The budget cut line (D-2, FR-013, E-07)
// ---------------------------------------------------------------------------

// A plan that cannot hold stage 2 drops it whole and says so on the line
// directly under the plan line, with the reason the planner recorded. The two
// lines stand together above the rows, which is where the reader holds the
// budget before the figures it explains.
auto scenarioPlanLineBudgetCut() -> void
{
  const auto run = captureRun(
      withCase({"--hunt=" + selection({0, 1, 2, 3}), "--hunt-budget=1p"}));
  check(huntBlock(run)
            .starts_with("hunt: plan events=4 passes=1 method=PASSES "
                         "estimate=<host-calibrated>ns budget=1p\n"
                         "hunt: plan budget cut stage=2 reason=over-budget\n"),
        "the cut line stands directly under the plan line, names the stage the "
        "budget dropped and the reason the planner recorded, and the plan "
        "line above it prints no stage field (FR-013, E-07)");
  std::printf("plan-line-budget-cut: ok\n");
}

// ---------------------------------------------------------------------------
// The plan line of a run that stands under no 033 budget (FR-022, FR-023)
// ---------------------------------------------------------------------------

// The estimate every scenario of the block below hands over: the nanoseconds
// of one pass, a figure of the record the scenario builds, so the digits of the
// printed field are the scenario's own.
constexpr std::uint64_t kRecordEstimateNs = 1'260'000;

// An unset budget prints nothing new. A record that stands under no sample
// budget and no multiplex cap prints the bytes it printed before 033. Both
// arms of the standing cut rule stand compared: the record whose budget dropped
// stage 2 prints the cut line under its plan line, and the record of a plan
// that split nothing prints the plan line alone.
auto scenarioPlanLineStandingBytes() -> void
{
  auto cut = contractRecord(kRecordEstimateNs, {address(0), address(1)}, 2);
  cut.stages = {sg::hunt::HuntStageRange {
      .first = 0,
      .count = 2,
      .repeats = 1,
  }};
  check(capturePrint([&cut] { sg::detail::printHuntPlan(cut); })
            == "hunt: plan events=2 passes=14 method=PASSES "
               "estimate=1260000ns budget=120s\n"
               "hunt: plan budget cut stage=2 reason=over-budget\n",
        "the record of a budgeted plan that holds one stage prints the standing "
        "plan line and the standing cut line, with no clause segment and no "
        "disclosure line beside either (FR-022, FR-023)");

  const auto standing =
      contractRecord(kRecordEstimateNs, {address(0), address(1)}, 2);
  check(capturePrint([&standing] { sg::detail::printHuntPlan(standing); })
            == "hunt: plan events=2 passes=14 method=PASSES "
               "estimate=1260000ns budget=120s\n",
        "a record that holds no sample budget and no multiplex cap prints one "
        "line, the standing plan line, byte for byte (FR-022, FR-023)");
  std::printf("plan-line-standing-bytes: ok\n");
}

// ---------------------------------------------------------------------------
// The two budget clauses of the plan line (specs/033 FR-003, FR-004, FR-020,
// FR-023, R-10, N-06)
// ---------------------------------------------------------------------------

// The sample budget joins the plan line as the canonical `e` text the stored
// nanoseconds rebuild, and it joins alone while no cap stands. The derived
// count of this record equals the stage rule of three, so the count moved
// nowhere and no disclosure line follows the plan line.
auto scenarioPlanLineSampleClause() -> void
{
  const auto record = contractRecord(kRecordEstimateNs,
                                     {address(0)},
                                     1,
                                     500'000'000ULL,
                                     std::nullopt,
                                     sg::hunt::HuntSampleState {
                                         .derivedRepeats = 3,
                                     });
  check(capturePrint([&record] { sg::detail::printHuntPlan(record); })
            == "hunt: plan events=1 passes=14 method=PASSES "
               "estimate=1260000ns budget=120s sample=0.5e\n",
        "the plan line spells the sample budget in the canonical `e` text the "
        "stored nanoseconds answer, with no cap clause beside it (FR-004, "
        "FR-020, FR-023, N-06)");
  std::printf("plan-line-sample-clause: ok\n");
}

// The cap joins the plan line as the canonical `g` text, and it joins alone
// while no sample budget stands. The cap of 4 over the 4 planned columns of
// the multiplexed plan splits one group of 4, which the line below spells
// (FR-003, FR-020, FR-023, N-06).
auto scenarioPlanLineMuxClause() -> void
{
  auto record = contractRecord(kRecordEstimateNs,
                               {address(0), address(1), address(2), address(3)},
                               4,
                               std::nullopt,
                               4,
                               sg::hunt::HuntSampleState {},
                               std::vector<std::size_t> {4});
  record.method = sg::MetricMethod::MULTIPLEX;
  check(capturePrint([&record] { sg::detail::printHuntPlan(record); })
            == "hunt: plan events=4 passes=14 method=MULTIPLEX "
               "estimate=1260000ns budget=120s mux=4g\n"
               "hunt: plan mux groups=1 sizes=4\n",
        "the plan line spells the cap in the canonical `g` text with no sample "
        "clause beside it, and the group the cap drew prints below it "
        "(FR-003, FR-020, FR-023, N-06)");
  std::printf("plan-line-mux-clause: ok\n");
}

// Both clauses stand between the standing `budget=` segment and the standing
// stage and normalization segments, the cap after the sample budget. The sample
// budget of 2 whole seconds rebuilds with no fraction (FR-004, FR-020, FR-023,
// R-10).
auto scenarioPlanLineBudgetClauseOrder() -> void
{
  auto record = contractRecord(kRecordEstimateNs,
                               {address(0), address(1), address(2), address(3)},
                               4,
                               2'000'000'000ULL,
                               32);
  record.options.norm = sg::hunt::HuntNorm::ITER;
  record.stages = {
      sg::hunt::HuntStageRange {.first = 0, .count = 4, .repeats = 1},
      sg::hunt::HuntStageRange {.first = 0, .count = 3, .repeats = 3},
  };
  check(capturePrint([&record] { sg::detail::printHuntPlan(record); })
            == "hunt: plan events=4 passes=14 method=PASSES "
               "estimate=1260000ns budget=120s sample=2e mux=32g stage1=4 "
               "stage2=3 normalized=iter\n",
        "the two clauses print in their fixed order after the budget and "
        "before the stages and the norm, and a whole second carries no "
        "fraction (FR-004, FR-020, FR-023, R-10)");
  std::printf("plan-line-budget-clause-order: ok\n");
}

// ---------------------------------------------------------------------------
// The sample-budget disclosure line (specs/033 FR-001, FR-002, FR-023, N-03,
// N-06)
// ---------------------------------------------------------------------------

// The sample budget leaves one line in one of its three states, below the plan
// line and below the cut line. A derived count reads against the stage rule it
// came under, an overrun reads the count the record carries beside the word
// that marks it, and a stage no calibration figure priced reads the word
// `unpriced`.
auto scenarioPlanLineSampleDisclosure() -> void
{
  const auto derived = contractRecord(kRecordEstimateNs,
                                      {address(0)},
                                      1,
                                      500'000'000ULL,
                                      std::nullopt,
                                      sg::hunt::HuntSampleState {
                                          .derivedRepeats = 2,
                                      });
  check(capturePrint([&derived] { sg::detail::printHuntPlan(derived); })
            == "hunt: plan events=1 passes=14 method=PASSES "
               "estimate=1260000ns budget=120s sample=0.5e\n"
               "hunt: plan sample repeats=2 of 3\n",
        "a derived count below the stage rule prints the count it derived and "
        "the rule that capped it (FR-001, FR-023, N-03)");

  auto cut = derived;
  cut.stages = {sg::hunt::HuntStageRange {
      .first = 0,
      .count = 1,
      .repeats = 1,
  }};
  check(capturePrint([&cut] { sg::detail::printHuntPlan(cut); })
            == "hunt: plan events=1 passes=14 method=PASSES "
               "estimate=1260000ns budget=120s sample=0.5e\n"
               "hunt: plan budget cut stage=2 reason=over-budget\n"
               "hunt: plan sample repeats=2 of 3\n",
        "the disclosure line stands below the cut line, so both lines that "
        "spell the run's budgets come first (FR-013, FR-023)");

  // The composed state of a two-stage plan: the budget fitted below one repeat
  // of one stage while the stage the run prices derived two repeats, so the
  // record holds the overrun and the count at once and the line prints the
  // count the document writes under `derived_repeats` (FR-002, FR-023, N-06,
  // the console parity of contracts/hunt-json.md).
  const auto composed = contractRecord(kRecordEstimateNs,
                                       {address(0)},
                                       1,
                                       500'000'000ULL,
                                       std::nullopt,
                                       sg::hunt::HuntSampleState {
                                           .derivedRepeats = 2,
                                           .overBudget = true,
                                       });
  check(capturePrint([&composed] { sg::detail::printHuntPlan(composed); })
            == "hunt: plan events=1 passes=14 method=PASSES "
               "estimate=1260000ns budget=120s sample=0.5e\n"
               "hunt: plan sample repeats=2 over-budget\n",
        "the overrun line prints the derived count the record carries beside "
        "the word that marks the overrun (FR-002, FR-023, N-06)");

  const auto overrun = contractRecord(kRecordEstimateNs,
                                      {address(0)},
                                      1,
                                      500'000'000ULL,
                                      std::nullopt,
                                      sg::hunt::HuntSampleState {
                                          .derivedRepeats = 1,
                                          .overBudget = true,
                                      });
  check(capturePrint([&overrun] { sg::detail::printHuntPlan(overrun); })
            == "hunt: plan events=1 passes=14 method=PASSES "
               "estimate=1260000ns budget=120s sample=0.5e\n"
               "hunt: plan sample repeats=1 over-budget\n",
        "a budget that fitted below one repeat of every stage prints the one "
        "repeat the record carries, and the word names the state (FR-001, "
        "FR-023, N-03)");

  const auto unpriced = contractRecord(kRecordEstimateNs,
                                       {address(0)},
                                       1,
                                       500'000'000ULL,
                                       std::nullopt,
                                       sg::hunt::HuntSampleState {
                                           .unpriced = true,
                                       });
  check(capturePrint([&unpriced] { sg::detail::printHuntPlan(unpriced); })
            == "hunt: plan events=1 passes=14 method=PASSES "
               "estimate=1260000ns budget=120s sample=0.5e\n"
               "hunt: plan sample unpriced\n",
        "a stage no calibration figure priced prints the word the contract "
        "states for it, beside the clause of the budget that stood (FR-002, "
        "FR-023, N-03)");
  std::printf("plan-line-sample-disclosure: ok\n");
}

// ---------------------------------------------------------------------------
// The multiplex-cap disclosure line (specs/033 FR-003, FR-023, N-04, N-06)
// ---------------------------------------------------------------------------

// The cap of 32 over the 200 catalog columns splits 6 full groups and one
// group of the 8 that remain. The line under the plan line spells the count of
// the groups and every size in plan order, comma-joined with no space between
// two sizes. The multiplexed plan runs the one pass that opened them.
auto scenarioPlanLineMuxGroups() -> void
{
  auto sizes = std::vector<std::size_t>(6, 32);
  sizes.push_back(8);
  auto record = contractRecord(kRecordEstimateNs,
                               sg::test::huntCatalog(),
                               sg::test::kHuntEvents,
                               std::nullopt,
                               32,
                               sg::hunt::HuntSampleState {},
                               sizes);
  record.method = sg::MetricMethod::MULTIPLEX;
  record.passes = 1;
  check(capturePrint([&record] { sg::detail::printHuntPlan(record); })
            == "hunt: plan events=200 passes=1 method=MULTIPLEX "
               "estimate=1260000ns budget=120s mux=32g\n"
               "hunt: plan mux groups=7 sizes=32,32,32,32,32,32,8\n",
        "the groups line prints the count of the groups the cap drew and every "
        "size in plan order, comma-joined with no space between them "
        "(FR-003, FR-023, N-04, N-06)");
  std::printf("plan-line-mux-groups: ok\n");
}

// A plan the run settled on `PASSES` opens no group, so a standing cap prints
// its clause on the plan line and prints no groups line at all (FR-003, FR-023,
// US3 scenario 3).
auto scenarioPlanLineMuxPassesNoGroups() -> void
{
  const auto record =
      contractRecord(kRecordEstimateNs,
                     {address(0), address(1), address(2), address(3)},
                     4,
                     std::nullopt,
                     32);
  check(capturePrint([&record] { sg::detail::printHuntPlan(record); })
            == "hunt: plan events=4 passes=14 method=PASSES "
               "estimate=1260000ns budget=120s mux=32g\n",
        "a PASSES settlement under a standing cap prints the cap clause and "
        "no groups line, because the run grouped nothing (FR-003, FR-023, "
        "US3 scenario 3)");
  std::printf("plan-line-mux-passes-no-groups: ok\n");
}

// ---------------------------------------------------------------------------
// The whole block in the PD-15 order (FR-040, FR-041, PD-15, D-12)
// ---------------------------------------------------------------------------

// One run, every line it prints for the hunt, byte for byte: the ranked table
// of the top two, the marked tail of the event that left it on the size of the
// table alone, the skipped list with the kind the catalog listing prints for
// the refused event, and the summary whose denominator counts the planned
// columns plus that refusal (FR-041, D-12). PD-15 is the order of the same
// bytes: the plan line above the first row and everything that ranks below it.
//
// specs/046 FR-016, FR-017, FR-033: the same run is a hunter run, so the block
// of the companion opens after that hunt block closes. The block holds the
// table path, the digests and the node names of this host, so its bytes stand
// compared nowhere (FR-027); its place and its count stand compared here. A
// host the selection refuses prints one refusal line and no block, which is
// the other arm of the same claim.
// The ask the companion makes: the gatherer of the pair, held to level 1, which
// is the whole difference D-17 states between the two asks of one run.
[[nodiscard]] auto companionAsk() -> sg::counters::TopdownSelection
{
  auto ask = sg::detail::topdownAreaSelection(sg::detail::RunOptions {});
  ask.level = 1;
  return ask;
}

auto scenarioHuntBlockOrder() -> void
{
  const auto run = captureRun(withCase(
      {"--hunt=" + selection({0, 1, 2, kRefusedEvent}), "--hunt-top=2"}));
  check(huntBlock(run)
            == "hunt: plan events=3 passes=1 method=PASSES estimate="
                   "<host-calibrated>ns budget=none\n"
                   "hunt: rank 1 cpu/hunt002 instr=75.000 cv=0.00 "
                   "correlation=n/a slope=n/a delta=n/a area=unmapped\n"
                   "hunt: rank 2 cpu/hunt001 instr=50.000 cv=0.00 "
                   "correlation=n/a slope=n/a delta=n/a area=unmapped\n"
                   "hunt: rank - cpu/hunt000 instr=25.000 cv=0.00 "
                   "correlation=n/a slope=n/a delta=n/a "
                   "area=unmapped mark=beyond-top\n"
                   "hunt: skipped cpu/hunt007 permission-blocked\n"
                   "hunt: plan summary events=3/4 passes=1 budget=none\n",
        "the hunt lines of one run print the shapes of E-07 in the order PD-15 "
        "fixes, the tail marked beyond-top and the summary counting the "
        "refusal in its denominator (FR-040, FR-041, SC-08)");
  const auto row = firstRowIndex(run);
  check(row != std::string::npos, "the run printed the row of its case");
  check(lineIndex(run, "hunt: plan events=") < row,
        "the plan line stands above the first row it bought (PD-15)");
  check(row < lineIndex(run, "hunt: rank 1 "),
        "the table that ranks stands behind the row it ranked (PD-15)");
  check(lineIndex(run, "hunt: rank 1 ")
            < lineIndex(run, "hunt: plan summary events="),
        "the summary closes the hunt lines, below the list it summarizes "
        "(PD-15)");

  // The one block of the companion, read beside the block it follows. The
  // library states which arm this host answers, so both arms carry a claim and
  // neither reads a host fact the scenario has to assume (FR-016, FR-017).
  const auto answered = sg::counters::topdownTree(companionAsk());
  const auto refusal = answered.has_value()
      ? std::string {}
      : std::string {answered.error().message()};
  const auto header = std::string {"topdown: level="};
  if (answered.has_value()) {
    check(countOf(run, header) == 1,
          "one hunter run prints one level-1 block, so the standing call "
          "stands suppressed on that route (FR-016, D-9)");
    check(lineIndex(run, header)
              > lineIndex(run, "hunt: plan summary events="),
          "the block opens after the hunt block of its benchmark closes, "
          "which is after the summary line (FR-016, D-9)");
  } else {
    check(countOf(run, header) == 0,
          "a host the selection refuses prints no block, because no tree "
          "stands to answer one (FR-017)");
    check(countOf(run, refusal) == 1,
          "the refusal of the companion prints the one line the run prints "
          "for its host, once for the process (FR-017)");
  }
  std::printf("hunt-block-order: ok\n");
}

// ---------------------------------------------------------------------------
// The one row set of a two-stage run (PD-13, I-4, E-07)
// ---------------------------------------------------------------------------

// The stage-1-only column of a two-stage run, read off the printed table. The
// plan of four columns at a top of 1 measures stage 2 over the leading 3 x top
// columns of the plan order, so `cpu/hunt003` stands in stage 1 alone and
// carries one sample (FR-014). One sample is no repetition, so the column
// ranks nowhere and its row stands in the marked tail, where the figure it
// carries is the one its stage 1 window folded: 400 over the 4'000 instructions
// of `cpu` answers 100.000, the highest figure of the run, while the ranked row
// is the best column stage 2 re-measured (PD-13, PD-14, D-7).
auto scenarioStageOneOnlyRow() -> void
{
  const auto run = captureRun(withCase({"--hunt=" + selection({0, 1, 2, 3}),
                                        "--hunt-top=1",
                                        "--hunt-budget=8p"}));
  check(huntBlock(run)
            == "hunt: plan events=4 passes=1 method=PASSES estimate="
                   "<host-calibrated>ns budget=8p stage1=4 stage2=3\n"
                   "hunt: rank 1 cpu/hunt002 instr=75.000 cv=0.00 "
                   "correlation=n/a slope=n/a delta=n/a area=unmapped\n"
                   "hunt: rank - cpu/hunt000 instr=25.000 cv=0.00 "
                   "correlation=n/a slope=n/a delta=n/a "
                   "area=unmapped mark=beyond-top\n"
                   "hunt: rank - cpu/hunt001 instr=50.000 cv=0.00 "
                   "correlation=n/a slope=n/a delta=n/a "
                   "area=unmapped mark=beyond-top\n"
                   "hunt: rank - cpu/hunt003 instr=100.000 cv=0.00 "
                   "correlation=n/a slope=n/a delta=n/a "
                   "area=unmapped mark=unrepeated\n"
                   "hunt: plan summary events=4/4 passes=1 budget=8p\n",
        "one row set stands for the two stages: the column only stage 1 "
        "measured prints once, in the tail, marked unrepeated, with the figure "
        "its stage 1 samples folded, and the ranked row is a column stage 2 "
        "re-measured (PD-13, PD-14, I-4, E-07)");
  check(countOf(run, "hunt: rank - cpu/hunt003 ") == 1,
        "the stage-1-only column reaches the table exactly once, so no second "
        "row stands for the two stages (PD-13, I-4)");
  std::printf("stage-one-only-row: ok\n");
}

// ---------------------------------------------------------------------------
// The rank row and the three marks (E-06, E-07, PD-14)
// ---------------------------------------------------------------------------

// The row shape both arms of the table print, and the word the tail carries
// for each reason an event stands out of the top N. A noisy event is the arm
// no scripted host answers - every scripted window folds one constant, so no
// scripted `cv` passes the noise threshold - so the table stands built, and
// the mark is the console's own reading of the flags the record carries.
auto scenarioRankRowMarks() -> void
{
  auto table = sg::hunt::EntryHunt {};
  table.entries = {
      sg::hunt::RankedEvent {
          .address = address(1),
          .rate = 50.0,
          .cv = 0.0,
          .samples = 3,
          .noisy = false,
          .repeated = true,
          .multiplexed = true,
          .rank = 1,
          .sensitivityCorrelation = std::nullopt,
          .sensitivitySlope = std::nullopt,
          .delta = std::nullopt,
      },
      sg::hunt::RankedEvent {
          .address = address(2),
          .rate = 30.0,
          .cv = 24.8,
          .samples = 3,
          .noisy = true,
          .repeated = true,
          .multiplexed = false,
          .rank = std::nullopt,
          .sensitivityCorrelation = std::nullopt,
          .sensitivitySlope = std::nullopt,
          .delta = std::nullopt,
      },
      sg::hunt::RankedEvent {
          .address = address(3),
          .rate = 100.0,
          .cv = 0.0,
          .samples = 1,
          .noisy = false,
          .repeated = false,
          .multiplexed = false,
          .rank = std::nullopt,
          .sensitivityCorrelation = std::nullopt,
          .sensitivitySlope = std::nullopt,
          .delta = std::nullopt,
      },
      sg::hunt::RankedEvent {
          .address = address(4),
          .rate = 20.0,
          .cv = 3.0,
          .samples = 3,
          .noisy = false,
          .repeated = true,
          .multiplexed = false,
          .rank = std::nullopt,
          .sensitivityCorrelation = std::nullopt,
          .sensitivitySlope = std::nullopt,
          .delta = std::nullopt,
      },
  };

  const std::string expectedRows {
      "hunt: rank 1 cpu/hunt001 instr=50.000 cv=0.00 correlation=n/a "
          "slope=n/a delta=n/a multiplexed\n"
      "hunt: rank - cpu/hunt002 instr=30.000 cv=24.80 correlation=n/a "
          "slope=n/a delta=n/a mark=noisy\n"
      "hunt: rank - cpu/hunt003 instr=100.000 cv=0.00 correlation=n/a "
          "slope=n/a delta=n/a mark=unrepeated\n"
      "hunt: rank - cpu/hunt004 instr=20.000 cv=3.00 correlation=n/a "
          "slope=n/a delta=n/a mark=beyond-top\n"};
  check(capturePrint([&table] {
          sg::detail::printHuntRanking(table, sg::hunt::HuntNorm::INSTR);
        })
            == expectedRows,
        "the ranked row prints its number and the tail its mark, and one flag "
        "of the record answers one word (E-07, PD-14, D-7, D-8)");
  std::printf("rank-row-marks: ok\n");
}

// FR-013, D-13: one ` area=` cell stands after the ` delta=` cell of specs/040
// and before the flag or the mark that closes the row. The area of a path
// answer holds spaces, so the cell prints that text whole and the two arms of
// the row tail print the one cell.
auto scenarioAreaCellAfterDeltaAndBeforeTheFlag() -> void
{
  constexpr std::string_view kArea {
      "topdown.backend_bound > topdown.intel.tma_l1_bound"};
  auto table = sg::hunt::EntryHunt {};
  table.entries = {
      sg::hunt::RankedEvent {
          .address = address(1),
          .rate = 50.0,
          .cv = 0.0,
          .samples = 3,
          .noisy = false,
          .repeated = true,
          .multiplexed = true,
          .rank = 1,
          .sensitivityCorrelation = std::nullopt,
          .sensitivitySlope = std::nullopt,
          .delta = std::nullopt,
          .topdownArea = std::string {kArea},
      },
      sg::hunt::RankedEvent {
          .address = address(2),
          .rate = 30.0,
          .cv = 24.8,
          .samples = 3,
          .noisy = true,
          .repeated = true,
          .multiplexed = false,
          .rank = std::nullopt,
          .sensitivityCorrelation = std::nullopt,
          .sensitivitySlope = std::nullopt,
          .delta = std::nullopt,
          .topdownArea = std::string {kArea},
      },
  };
  const std::string expectedRows {
      "hunt: rank 1 cpu/hunt001 instr=50.000 cv=0.00 correlation=n/a "
      "slope=n/a delta=n/a area=topdown.backend_bound > "
      "topdown.intel.tma_l1_bound multiplexed\n"
      "hunt: rank - cpu/hunt002 instr=30.000 cv=24.80 correlation=n/a "
      "slope=n/a delta=n/a area=topdown.backend_bound > "
      "topdown.intel.tma_l1_bound mark=noisy\n"};
  check(capturePrint(
            [&table]
            { sg::detail::printHuntRanking(table, sg::hunt::HuntNorm::INSTR); })
            == expectedRows,
        "the area cell stands between the delta cell and the flag, and the "
        "mark " "row prints the same cell before its mark (FR-013, D-13)");
  std::printf("area-cell-after-delta-and-before-the-flag: ok\n");
}

// FR-014: a record whose area stands empty prints no cell and no separator, so
// its line keeps every byte `example/docs/hunt_rank.expected:6-10` pins. Those
// five rows carry the figures of that block, and the check compares the block
// whole, because a stray space there moves a byte a document already quotes.
auto scenarioNoAreaCellForAnEmptyMember() -> void
{
  auto table = sg::hunt::EntryHunt {};
  table.entries = {
      sg::hunt::RankedEvent {
          .address = "cpu/hunt_branches",
          .rate = 2000.0,
          .cv = 0.0,
          .samples = 3,
          .noisy = false,
          .repeated = true,
          .multiplexed = false,
          .rank = 1,
          .sensitivityCorrelation = std::nullopt,
          .sensitivitySlope = std::nullopt,
          .delta = std::nullopt,
      },
      sg::hunt::RankedEvent {
          .address = "cpu/hunt_loads",
          .rate = 1000.0,
          .cv = 0.0,
          .samples = 3,
          .noisy = false,
          .repeated = true,
          .multiplexed = false,
          .rank = 2,
          .sensitivityCorrelation = std::nullopt,
          .sensitivitySlope = std::nullopt,
          .delta = std::nullopt,
      },
      sg::hunt::RankedEvent {
          .address = "cpu/hunt_rtls",
          .rate = 0.0,
          .cv = 0.0,
          .samples = 1,
          .noisy = false,
          .repeated = false,
          .multiplexed = false,
          .rank = std::nullopt,
          .sensitivityCorrelation = std::nullopt,
          .sensitivitySlope = std::nullopt,
          .delta = std::nullopt,
      },
      sg::hunt::RankedEvent {
          .address = "cpu/hunt_stalls",
          .rate = 3000.0,
          .cv = 33.33,
          .samples = 3,
          .noisy = true,
          .repeated = true,
          .multiplexed = false,
          .rank = std::nullopt,
          .sensitivityCorrelation = std::nullopt,
          .sensitivitySlope = std::nullopt,
          .delta = std::nullopt,
      },
      sg::hunt::RankedEvent {
          .address = "cpu/hunt_stores",
          .rate = 0.0,
          .cv = 0.0,
          .samples = 3,
          .noisy = false,
          .repeated = true,
          .multiplexed = false,
          .rank = std::nullopt,
          .sensitivityCorrelation = std::nullopt,
          .sensitivitySlope = std::nullopt,
          .delta = std::nullopt,
      },
  };
  const std::string expectedRows {
      "hunt: rank 1 cpu/hunt_branches instr=2000.000 cv=0.00 "
      "correlation=n/a slope=n/a delta=n/a\n"
      "hunt: rank 2 cpu/hunt_loads instr=1000.000 cv=0.00 "
      "correlation=n/a slope=n/a delta=n/a\n"
      "hunt: rank - cpu/hunt_rtls instr=0.000 cv=0.00 "
      "correlation=n/a slope=n/a delta=n/a mark=unrepeated\n"
      "hunt: rank - cpu/hunt_stalls instr=3000.000 cv=33.33 "
      "correlation=n/a slope=n/a delta=n/a mark=noisy\n"
      "hunt: rank - cpu/hunt_stores instr=0.000 cv=0.00 "
      "correlation=n/a slope=n/a delta=n/a mark=beyond-top\n"};
  check(capturePrint([&table] {
          sg::detail::printHuntRanking(table, sg::hunt::HuntNorm::INSTR);
        })
            == expectedRows,
        "an empty area prints no cell and no separator, so the row keeps the "
        "bytes the recorded block pins (FR-014)");
  std::printf("no-area-cell-for-an-empty-member: ok\n");
}

// FR-005, FR-009: a hunt run stamps every ranked row it prints. The scripted
// catalog of `detail/hunt_fixture.hpp` names no vendor event, so no node of a
// tree and no row of a grouping names these addresses, and the cell reads
// `unmapped` on every host. The recorded example holds no such proof: its rows
// come from `rankEvents`, which stamps nothing.
auto scenarioEveryRankedRowOfAHuntRunCarriesAnAreaCell() -> void
{
  const auto run = captureRun(withCase(
      {"--hunt=" + selection({0, 1, 2, kRefusedEvent}), "--hunt-top=2"}));
  auto rows = std::vector<std::string> {};
  auto lines = std::istringstream {huntBlock(run)};
  for (std::string line; std::getline(lines, line);) {
    if (line.starts_with("hunt: rank ")) {
      rows.push_back(line);
    }
  }
  check(rows.size() == 3,
        "the run printed the two ranked rows and the marked row of the tail, "
        "so " "the claim below reads a full table (E-07)");
  for (const std::string& row : rows) {
    const auto area = row.find(" area=unmapped");
    check(area != std::string::npos,
          "every ranked row of a hunt run carries one area cell, the marked row "
          "included, and the scripted catalog answers `unmapped` for it "
          "(FR-005, FR-009)");
    check(row.find(" delta=") < area,
          "the area cell stands after the delta cell of specs/040 (FR-013)");
    const auto mark = row.find(" mark=");
    check(mark == std::string::npos || area < mark,
          "the area cell stands before the mark that closes the row (FR-013, "
          "D-13)");
  }
  std::printf("every-ranked-row-of-a-hunt-run-carries-an-area-cell: ok\n");
}

// ---------------------------------------------------------------------------
// The skipped list and the summary line (E-06, E-07, D-10, FR-041)
// ---------------------------------------------------------------------------

// One line per event the run will not measure, with the kind in the words the
// catalog prints: the record holds an address and a kind, so the line holds
// those two figures and nothing else (T123, FR-041). The summary closes the
// list with the events that rated over the universe the hunt stood before and
// the budget in the unit it was given.
auto scenarioSkippedListLine() -> void
{
  auto table = sg::hunt::EntryHunt {};
  table.skipped = {
      sg::hunt::SkippedEvent {
          .address = address(5),
          .kind = sg::hunt::SkippedKind::PERMISSION_BLOCKED,
      },
      sg::hunt::SkippedEvent {
          .address = address(6),
          .kind = sg::hunt::SkippedKind::NOT_ENCODABLE,
      },
  };
  // Nothing stands in the plan, so both refusals join the denominator: the
  // host that refuses everything reads `events=0/N` (D-12, FR-041).
  const auto record = contractRecord(1'260'000, {}, 0);

  check(capturePrint([&] {
          sg::detail::printHuntSkips(table, record);
        })
            == "hunt: skipped cpu/hunt005 permission-blocked\n"
               "hunt: skipped cpu/hunt006 not-encodable\n"
               "hunt: plan summary events=0/2 passes=14 budget=120s\n",
        "the skipped line prints the address and the kind the catalog prints, "
        "and the summary prints the counts and the budget under which the plan "
        "stood (FR-041, D-10)");
  std::printf("skipped-list-line: ok\n");
}

// ---------------------------------------------------------------------------
// The plan line of a run that normalized by iterations (D-13, E-07)
// ---------------------------------------------------------------------------

// A run that measured under the iteration norm says so on the plan line and
// labels its rate column with the same word. The rate is the event figure over
// the one iteration of the hunt window, so 100 / 1 answers `iter=100.000`.
auto scenarioNormalizedIterLine() -> void
{
  const auto run =
      captureRun(withCase({"--hunt=" + selection({0}), "--hunt-norm=iter"}));
  check(huntBlock(run)
            == "hunt: plan events=1 passes=1 method=PASSES estimate="
                   "<host-calibrated>ns budget=none normalized=iter\n"
                   "hunt: rank 1 cpu/hunt000 iter=100.000 cv=0.00 "
                   "correlation=n/a slope=n/a delta=n/a area=unmapped\n"
                   "hunt: plan summary events=1/1 passes=1 budget=none\n",
        "the norm the run measured under is the norm both the plan line and "
        "the rate column name (D-13, E-07)");
  std::printf("normalized-iter-line: ok\n");
}

// ---------------------------------------------------------------------------
// The silence of `--hunt=none` (FR-001, FR-052, I-8)
// ---------------------------------------------------------------------------

// The disabled hunt is the run with no hunt: no line behind the `hunt:` prefix
// at all, while the benchmark it says nothing about still prints its row -
// without that row the silence would prove nothing.
auto scenarioHuntNoneSilence() -> void
{
  const auto run = captureRun(withCase({"--hunt=none"}));
  check(countOf(run, "hunt:") == 0,
        "the run with --hunt=none prints no hunt line at all (FR-052, I-8)");
  check(firstRowIndex(run) != std::string::npos,
        "the silent run still printed the row of its benchmark, so the "
        "silence above is not an empty report (FR-001)");
  std::printf("hunt-none-silence: ok\n");
}

// ---------------------------------------------------------------------------
// The result that carries a hunt and no run record (PD-10, E-08)
// ---------------------------------------------------------------------------

// The mixed reading of the one guard that opens the hunt block: the result
// carries the hunt its run measured, while the run-level record of that hunt
// did not travel with it, so nothing of the hunt prints - neither the plan
// line above the first row nor the lines that rank behind it - and the row of
// the benchmark prints its figures. The same record printed beside its run
// record is the witness that the silence is the missing record and not an
// empty hunt (PD-10, E-08, I-8).
auto scenarioPlanLineAbsentRun() -> void
{
  auto row = sg::ResultRow {};
  auto result = sg::BenchmarkResult {};
  result.name = kCaseName;
  result.rows = {row};
  auto table = sg::hunt::EntryHunt {};
  table.method = sg::MetricMethod::PASSES;
  table.entries = {
      sg::hunt::RankedEvent {
          .address = address(0),
          .rate = 50.0,
          .cv = 0.0,
          .samples = 3,
          .noisy = false,
          .repeated = true,
          .multiplexed = false,
          .rank = 1,
          .sensitivityCorrelation = std::nullopt,
          .sensitivitySlope = std::nullopt,
          .delta = std::nullopt,
      },
  };
  result.hunt = table;

  const auto record = contractRecord(std::nullopt, {address(0)}, 1);
  const auto withRun = capturePrint(
      [&result, &record] { sg::detail::printResult(result, record); });
  check(lineIndex(withRun, "hunt: plan events=") != std::string::npos,
        "the record of the same hunt beside its run record prints the plan "
        "line, so the table below is not empty by construction");
  check(countOf(withRun, "hunt: rank 1 ") == 1,
        "the ranked table of the record prints behind the row");

  const auto withoutRun = capturePrint(
      [&result] { sg::detail::printResult(result, std::nullopt); });
  check(countOf(withoutRun, "hunt:") == 0,
        "a result that carries a hunt and no run record prints no hunt line "
        "at all, the plan line and the table that ranks included");
  check(firstRowIndex(withoutRun) != std::string::npos,
        "the run without its record still printed the row of its benchmark, "
        "so the silence above is not an empty report");
  std::printf("plan-line-absent-run: ok\n");
}

// ---------------------------------------------------------------------------
// The block of the companion (specs/046-hunter-topdown T074, T075; FR-015,
// FR-016, FR-033, FR-035, FR-036; D-9)
// ---------------------------------------------------------------------------

// The whole line that first holds `needle`, its end of line removed, and the
// empty text where no line holds it. A block whose figures a hand-built record
// states stands compared whole, so a scenario names the line it weighs.
[[nodiscard]] auto lineHolding(const std::string& text,
                               const std::string& needle) -> std::string
{
  const std::size_t hit = text.find(needle);
  if (hit == std::string::npos) {
    return {};
  }
  const std::size_t start = text.rfind('\n', hit) + 1;
  const std::size_t end = text.find('\n', hit);
  return text.substr(start, end - start);
}

// Every line of a capture that opens no top-down block: the row, its
// disclosures and the hunt lines, which are the bytes of the entry record.
[[nodiscard]] auto withoutTopdownLines(const std::string& text) -> std::string
{
  std::string kept;
  std::istringstream stream(text);
  std::string line;
  while (std::getline(stream, line)) {
    if (line.rfind("topdown: ", 0) == 0) {
      continue;
    }
    kept += line;
    kept += '\n';
  }
  return kept;
}

// One level of one table, in the shape `stripTopdownColumns` publishes: one
// node, one figure, and the directory the row came from. A scenario hands two
// such records to one print, so the directory the block names is the record
// that reached the console (FR-016).
[[nodiscard]] auto levelRecord(const std::uint64_t level,
                               const std::string& directory)
    -> sg::TopdownRecord
{
  auto node = sg::TopdownNode {};
  node.name = "topdown.backend_bound";
  node.vendorName = "tma_backend_bound";
  node.level = 1;
  node.value = 25.0;
  node.availability = sg::counters::Availability::COUNTABLE;
  node.threshold = "tma_backend_bound > 0.2";
  auto record = sg::TopdownRecord {};
  record.supported = true;
  record.directory = directory;
  record.askedLevel = level;
  record.level = level;
  record.nodes = {std::move(node)};
  return record;
}

// The record of one hunted benchmark beside the run-level record of the hunt
// that answered it: the pair that opens the hunt block (E-07, PD-10).
struct HuntedPair
{
  sg::BenchmarkResult result;
  sg::hunt::RunHunt run;
};

[[nodiscard]] auto huntedPair() -> HuntedPair
{
  auto record = sg::hunt::RankedEvent {
      .address = address(0),
      .rate = 50.0,
      .cv = 0.0,
      .samples = 3,
      .noisy = false,
      .repeated = true,
      .multiplexed = false,
      .rank = 1,
      .sensitivityCorrelation = std::nullopt,
      .sensitivitySlope = std::nullopt,
      .delta = std::nullopt,
  };
  auto result = sg::BenchmarkResult {};
  result.name = kCaseName;
  result.rows = {sg::ResultRow {}};
  result.hunt = sg::hunt::EntryHunt {
      .method = sg::MetricMethod::PASSES,
      .passCount = 14,
      .top = 20,
      .entries = {std::move(record)},
      .skipped = {},
      .retriedPasses = 0,
  };
  return HuntedPair {result, contractRecord(std::nullopt, {address(0)}, 1)};
}

// One hunter run prints one block. The carrier of the companion prints it, and
// the standing call of the entry record stands suppressed beside it, which is
// the split D-9 states and FR-016 words (FR-016, FR-033, D-9).
auto scenarioAHunterRunPrintsOneTopdownBlock() -> void
{
  const auto pair = huntedPair();
  auto result = pair.result;
  result.topdown = levelRecord(2, "arch/x86/entrydir");
  const auto carrier = sg::detail::TopdownCompanion {
      .record = levelRecord(1, "arch/x86/companiondir"),
      .passCount = 3,
  };
  const auto printed = capturePrint(
      [&]
      { sg::detail::printResult(result, pair.run, std::nullopt, carrier); });

  check(countOf(printed, "topdown: level=") == 1,
        "the hunter run prints one block, because the standing call of the "
        "entry record stands suppressed where the carrier prints (FR-016, D-9)");
  check(lineHolding(printed, "topdown: level=")
            == "topdown: level=1 passes=3 table=arch/x86/companiondir",
        "the block that prints carries the companion record, under the pass "
        "count of the collection that measured it (FR-016, FR-033)");
  check(lineIndex(printed, "topdown: level=")
            > lineIndex(printed, "hunt: plan summary events="),
        "the block opens after the hunt block of that benchmark closes "
        "(FR-016, D-9)");
  // A collection that reached no strip leaves a record that states no support,
  // and the block of such a carrier prints no line: the hunt block closes and
  // the run reads the bytes it read before the companion stood (FR-017).
  const auto silent = sg::detail::TopdownCompanion {
      .record = sg::TopdownRecord {},
      .passCount = 0,
  };
  const auto silentPrint = capturePrint(
      [&] -> void
      { sg::detail::printResult(result, pair.run, std::nullopt, silent); });
  check(countOf(silentPrint, "topdown:") == 0,
        "the carrier of a record that states no support prints no line at all, "
        "the hunt rows included (FR-017, FR-016)");
  check(
      lineHolding(silentPrint, "hunt: plan summary events=")
          == lineHolding(printed, "hunt: plan summary events="),
      "the silent carrier moves no byte of the hunt block it follows (FR-015)");
  std::printf("a-hunter-run-prints-one-topdown-block: ok\n");
}

// The print of the block reads the same predicate the assembly of the node
// read. A record that reached the console by another route, a hand-built one
// here, prints no figure it could not have assembled: the node below carries a
// figure beside a state that is no countable one, and its line answers the
// words of a node the run cannot answer (PC-17, FR-030).
auto scenarioANodeOfNoCountableStatePrintsNoFigure() -> void
{
  const auto pair = huntedPair();
  auto result = pair.result;
  auto record = levelRecord(1, "arch/x86/entrydir");
  record.nodes.front().availability = sg::counters::Availability::GAP;
  const auto carrier = sg::detail::TopdownCompanion {
      .record = std::move(record),
      .passCount = 3,
  };
  const auto printed = capturePrint(
      [&]
      { sg::detail::printResult(result, pair.run, std::nullopt, carrier); });
  check(lineHolding(printed, "topdown:  topdown.backend_bound")
            == "topdown:  topdown.backend_bound  tma_backend_bound  not "
               "available  tma_backend_bound > 0.2",
        "a node that carries a figure beside a state that is not countable "
        "prints its two names, its threshold and no figure (FR-030, PC-17)");
  std::printf("a-node-of-no-countable-state-prints-no-figure: ok\n");
}

// The entry record of a hunter run states its own figures and no further
// disclosure, because the companion answers a record of its own (FR-015,
// FR-035, FR-033). The last check reads one record twice: the print without the
// carrier is the bytes of the base, and the print with it is the bytes of the
// head.
auto scenarioEntryRunFiguresStandEqual() -> void
{
  const auto pair = huntedPair();
  auto result = pair.result;
  auto row = result.rows.front();
  row.metrics = {sg::MetricValue {.value = 12.5, .pass = 7}};
  result.rows = {row};
  result.metricLabels = {address(0)};
  result.metricMethod = sg::MetricMethod::PASSES;
  result.passCount = 12;
  result.passSpread = 1.5;
  const auto carrier = sg::detail::TopdownCompanion {
      .record = levelRecord(1, "arch/x86/companiondir"),
      .passCount = 4,
  };
  const auto withCarrier = capturePrint(
      [&]
      { sg::detail::printResult(result, pair.run, std::nullopt, carrier); });

  check(lineHolding(withCarrier, "  passes  method=")
            == "bmHuntReport                 passes  method=passes  count=12"
               "  spread=1.5%",
        "the one disclosure line reads the pass figures of the entry record "
        "(FR-015, FR-035)");
  check(countOf(withCarrier, "  passes  method=") == 1,
        "the collection of the companion answers no disclosure line of its own "
        "(FR-035)");
  check(countOf(withCarrier, " pass=7") == 1,
        "the pass cell of the row of the entry record stands (FR-015)");
  check(lineHolding(withCarrier, "topdown: level=")
            == "topdown: level=1 passes=4 table=arch/x86/companiondir",
        "the header of the block states the passes of the collection that ran, "
        "and no figure of the entry record (FR-033)");

  const auto withoutCarrier =
      capturePrint([&] { sg::detail::printResult(result, pair.run); });
  check(withoutTopdownLines(withCarrier) == withoutTopdownLines(withoutCarrier),
        "every byte the entry record owns stands equal between the print that "
        "carries the companion and the print that carries none, which is the "
        "base and the head of one record (FR-015)");
  std::printf("entry-run-figures-stand-equal-between-base-and-head: ok\n");
}

// The assertion of `topdownDeepestNode` reads its answer over the whole list
// through `nodesNamingAddressStandAtOrBelow`. The node that names another
// address answers the first arm of that answer, and the two levels of the nodes
// that name one address answer the other two (FR-001, FR-002, FR-003, D-4).
auto scenarioDeepestNodeAssertAnswersEachArm() -> void
{
  const auto nodeOf = [](const char* name,
                         const int level,
                         const char* event) -> sg::counters::TopdownNode
  {
    auto node = sg::counters::TopdownNode {};
    node.name = name;
    node.level = level;
    node.events = {std::string {event}};
    return node;
  };
  auto tree = sg::counters::TopdownTree {};
  tree.depth = 3;
  tree.nodes = {
      nodeOf("topdown.frontend_bound", 3, "cpu/other_leaf"),
      nodeOf("topdown.backend_bound", 1, "cpu/deep_leaf"),
      nodeOf("topdown.backend_bound.core_bound", 2, "cpu/deep_leaf"),
  };

  check(sg::detail::nodesNamingAddressStandAtOrBelow(tree, 2, "cpu/deep_leaf"),
        "the level-2 node stands at its own level and at or above the level-1"
        " node that names the same address (FR-003, D-4)");
  check(!sg::detail::nodesNamingAddressStandAtOrBelow(tree, 1, "cpu/deep_leaf"),
        "a node that names the same address stands deeper than the level-1 node"
        " the answer holds, so the answer reads false (FR-003, D-4)");
  check(sg::detail::nodesNamingAddressStandAtOrBelow(tree, 0, "cpu/absent"),
        "an address no node of the tree names takes no part, so the answer"
        " reads true over the whole list (FR-002)");
  std::printf("deepest-node-assert-answers-each-arm: ok\n");
}

// The scenarios of this file: what CTest runs with no argument, and what
// `argv[1]` selects from. One entry per shape of data-model E-07.
[[nodiscard]] auto scenarios() -> const std::vector<Scenario>&
{
  static const std::vector<Scenario> kList {
      Scenario {"plan-line-shape", &scenarioPlanLineShape},
      Scenario {"plan-line-two-stage", &scenarioPlanLineTwoStage},
      Scenario {"plan-line-budget-fits", &scenarioPlanLineBudgetFits},
      Scenario {"plan-line-multiplex", &scenarioPlanLineMultiplex},
      Scenario {"plan-line-no-limit-host", &scenarioPlanLineNoLimitHost},
      Scenario {"plan-line-budget-cut", &scenarioPlanLineBudgetCut},
      Scenario {"plan-line-standing-bytes", &scenarioPlanLineStandingBytes},
      Scenario {"plan-line-sample-clause", &scenarioPlanLineSampleClause},
      Scenario {"plan-line-mux-clause", &scenarioPlanLineMuxClause},
      Scenario {"plan-line-budget-clause-order",
                &scenarioPlanLineBudgetClauseOrder},
      Scenario {"plan-line-sample-disclosure",
                &scenarioPlanLineSampleDisclosure},
      Scenario {"plan-line-mux-groups", &scenarioPlanLineMuxGroups},
      Scenario {"plan-line-mux-passes-no-groups",
                &scenarioPlanLineMuxPassesNoGroups},
      Scenario {"quickstart-plan-cut", &scenarioQuickstartPlanCut},
      Scenario {"hunt-block-order", &scenarioHuntBlockOrder},
      Scenario {"stage-one-only-row", &scenarioStageOneOnlyRow},
      Scenario {"rank-row-marks", &scenarioRankRowMarks},
      Scenario {"area-cell-after-delta-and-before-the-flag",
                &scenarioAreaCellAfterDeltaAndBeforeTheFlag},
      Scenario {"no-area-cell-for-an-empty-member",
                &scenarioNoAreaCellForAnEmptyMember},
      Scenario {"every-ranked-row-of-a-hunt-run-carries-an-area-cell",
                &scenarioEveryRankedRowOfAHuntRunCarriesAnAreaCell},
      Scenario {"skipped-list-line", &scenarioSkippedListLine},
      Scenario {"normalized-iter-line", &scenarioNormalizedIterLine},
      Scenario {"hunt-none-silence", &scenarioHuntNoneSilence},
      Scenario {"plan-line-absent-run", &scenarioPlanLineAbsentRun},
      Scenario {"a-hunter-run-prints-one-topdown-block",
                &scenarioAHunterRunPrintsOneTopdownBlock},
      Scenario {"a-node-of-no-countable-state-prints-no-figure",
                &scenarioANodeOfNoCountableStatePrintsNoFigure},
      Scenario {"entry-run-figures-stand-equal-between-base-and-head",
                &scenarioEntryRunFiguresStandEqual},
      Scenario {"deepest-node-assert-answers-each-arm",
                &scenarioDeepestNodeAssertAnswersEachArm},
  };
  return kList;
}

}  // namespace

// The walk gives every scenario a process of its own, the pattern
// `harness_hunt_collect_test.cpp` carries: the counters system takes an object
// path once per process, and one scenario drives a host no other scenario of
// this file drives. A child that fails carries its own verdict, so the walk
// stops on the first one.
auto runInOwnProcess(const char* self, const std::string_view name) -> void
{
  const std::string command =
      "\"" + std::string {self} + "\" " + std::string {name};
  const int status = std::system(command.c_str());
  check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
        "the re-executed scenario passes");
}

auto main(const int argc, char** argv) -> int
{
  const auto& list = scenarios();
  // One name selects one scenario through `argv[1]`, so a name that stands
  // twice would hide the scenario it repeats.
  for (std::size_t first = 0; first < list.size(); ++first) {
    check(list[first].body != nullptr, "every scenario carries a body");
    for (std::size_t second = first + 1; second < list.size(); ++second) {
      check(list[first].name != list[second].name,
            "no two scenarios of this file carry the same name");
    }
  }
  if (argc > 1) {
    const std::string_view mode = argv[1];
    for (const auto& scenario : list) {
      if (scenario.name == mode) {
        scenario.body();
        return 0;
      }
    }
    fail("the scenario name is not one of this file's list");
  }
  for (const auto& scenario : list) {
    runInOwnProcess(argv[0], scenario.name);
  }
  std::printf("harness_hunt_report_test: ok\n");
  return 0;
}
