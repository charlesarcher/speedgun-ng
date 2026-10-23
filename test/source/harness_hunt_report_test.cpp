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
// prints. Seven scenarios drive a real run of the scripted host of
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
// The figure no scenario states is the estimate of the plan line: the
// nanoseconds of the pass the hunt timed (T119), so its digits belong to the
// host. They stand behind the `<host-calibrated>` token this file's captures
// substitute, the same stand-in
// `test/source/detail/pass_identity.expected` carries for the floor of a
// finished row, and every byte around them is compared.
//
// Three arms stand pinned off a record the scenario builds, the way
// `harness_hunt_cli_test.cpp` pins the cut reason, because the run that
// answers them leaves the bytes of the line unpinnable: the `stage=` arm of
// the skipped line, since no run path fills `SkippedEvent::stage`; the `noisy`
// mark, since a scripted window folds one constant and no scripted `cv` passes
// the noise threshold; and the `estimate=not available` arm, which only the
// entry of a dry run answers, since a hunt that ran nothing to time holds no
// figure (T119). Each states its reason where it is pinned.
//
// The list below is what CTest runs, so an entry absent from it never runs, and
// `argv[1]` selects one entry of it. Frameworkless check()/fail() convention,
// the one `harness_pass_report_test.cpp` carries.
// ============================================================================

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <unistd.h>

#include "detail/hunt_fixture.hpp"
#include "detail/internal.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"
#include "speedgun-ng/counters_fake.hpp"
#include "speedgun-ng/counters_system.hpp"
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

// The scratch file every capture of this binary reads back.
constexpr const char* kScratchPath = "harness_hunt_report_out.txt";

// The token that stands for the figure of a hunt line that belongs to the
// host: the estimate of the plan line, the nanoseconds of one pass the hunt
// timed (T119).
constexpr std::string_view kCalibrated = "<host-calibrated>";

// The scripted host: the hunt fixture at the limit of 6 the AC-1 row states,
// with one event leaf re-scripted permission-blocked so a selected address can
// reach the skipped list. The counters system takes these object paths once
// per process, so one host stands for every scenario of this file, and a
// scenario that wanted a second one would need a re-exec of its own.
constexpr std::size_t kHostLimit = 6;
constexpr std::size_t kRefusedEvent = 7;

// The one benchmark case the runs filter down to.
constexpr const char* kCaseName = "bmHuntReport";

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

// The host, laid down once: the fixture plus the refused leaf of
// `kRefusedEvent`, which is the address one scenario selects so that the
// skipped line and the `events=ran/planned+refused` denominator of the summary
// print (FR-041, D-12).
auto ensureHost() -> void
{
  static const bool kAccepted = []
  {
    auto provider = std::make_unique<sg::counters::FakeProvider>();
    auto& host = sg::test::huntHost(*provider, kHostLimit);
    host.addCounter(sg::test::kHuntCpu,
                    sg::test::huntEvent(kRefusedEvent),
                    "none",
                    "a refused event",
                    sg::counters::Availability::PERMISSION_BLOCKED);
    const auto settled =
        sg::counters::System::local().registerProvider(std::move(provider));
    return settled.has_value();
  }();
  check(kAccepted, "the hunt fixture registers on the local system");
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
// below weighs bytes the process wrote and nothing else.
[[nodiscard]] auto captureRun(const std::vector<std::string>& arguments)
    -> std::string
{
  ensureHost();
  ensureCase();

  std::vector<char*> argv {const_cast<char*>(kCaseName)};
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  const int saved = dup(STDOUT_FILENO);
  if (saved < 0 || std::freopen(kScratchPath, "w", stdout) == nullptr) {
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

  std::ifstream file(kScratchPath);
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

// The bytes of one print, captured the way `captureRun` captures a run - the
// seam of `harness_hunt_cli_test.cpp` for a call that needs no run.
template<typename Print>
[[nodiscard]] auto capturePrint(const Print& print) -> std::string
{
  const int saved = dup(STDOUT_FILENO);
  if (saved < 0 || std::freopen(kScratchPath, "w", stdout) == nullptr) {
    fail("cannot redirect the printer");
  }
  print();
  std::fflush(stdout);
  if (dup2(saved, STDOUT_FILENO) < 0) {
    fail("cannot restore stdout");
  }
  close(saved);
  std::ifstream file(kScratchPath);
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
// record the contract states for it.
[[nodiscard]] auto contractRecord(
    const std::optional<std::uint64_t>& estimateNs,
    const std::vector<std::string>& plannedAddresses,
    const std::size_t ranEvents) -> sg::hunt::RunHunt
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
          },
      .passes = 14,
      .method = sg::MetricMethod::PASSES,
      .estimateNs = estimateNs,
      .plannedAddresses = plannedAddresses,
      .stages = {},
      .ranEvents = ranEvents,
      .stageTwoEvents = 0,
      .cutReason = "over-budget",
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
      "hunt: rank 1 cpu/hunt001 instr=50.000 cv=0.00 multiplexed\n"
      "hunt: rank 2 cpu/hunt000 instr=25.000 cv=0.00 multiplexed\n"};
  check(block.find(expectedRows) != std::string::npos,
        "every row of the multiplexed hunt carries the word, in the rank order "
        "of the rate column (D-19, PD-07)");
  std::printf("plan-line-multiplex: ok\n");
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
// The whole block in the PD-15 order (FR-040, FR-041, PD-15, D-12)
// ---------------------------------------------------------------------------

// One run, every line it prints for the hunt, byte for byte: the ranked table
// of the top two, the marked tail of the event that left it on the size of the
// table alone, the skipped list with the kind the catalog listing prints for
// the refused event, and the summary whose denominator counts the planned
// columns plus that refusal (FR-041, D-12). PD-15 is the order of the same
// bytes: the plan line above the first row and everything that ranks below it.
auto scenarioHuntBlockOrder() -> void
{
  const auto run = captureRun(withCase(
      {"--hunt=" + selection({0, 1, 2, kRefusedEvent}), "--hunt-top=2"}));
  check(huntBlock(run)
            == "hunt: plan events=3 passes=1 method=PASSES estimate="
                   "<host-calibrated>ns budget=none\n"
                   "hunt: rank 1 cpu/hunt002 instr=75.000 cv=0.00\n"
                   "hunt: rank 2 cpu/hunt001 instr=50.000 cv=0.00\n"
                   "hunt: rank - cpu/hunt000 instr=25.000 cv=0.00 "
                   "mark=beyond-top\n"
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
  std::printf("hunt-block-order: ok\n");
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
      },
  };

  const std::string expectedRows {
      "hunt: rank 1 cpu/hunt001 instr=50.000 cv=0.00 multiplexed\n"
      "hunt: rank - cpu/hunt002 instr=30.000 cv=24.80 mark=noisy\n"
      "hunt: rank - cpu/hunt003 instr=100.000 cv=0.00 mark=unrepeated\n"
      "hunt: rank - cpu/hunt004 instr=20.000 cv=3.00 mark=beyond-top\n"};
  check(capturePrint([&table] {
          sg::detail::printHuntRanking(table, sg::hunt::HuntNorm::INSTR);
        })
            == expectedRows,
        "the ranked row prints its number and the tail its mark, and one flag "
        "of the record answers one word (E-07, PD-14, D-7, D-8)");
  std::printf("rank-row-marks: ok\n");
}

// ---------------------------------------------------------------------------
// The skipped list and the summary line (E-06, E-07, D-10, FR-041)
// ---------------------------------------------------------------------------

// One line per event the run will not measure, with the kind in the words the
// catalog listing prints, and the stage that lost the event where a stage did:
// no run path fills `SkippedEvent::stage` today, so that arm stands built here
// beside the arm a run prints. The summary closes the list with the events that
// rated over the universe the hunt stood before and the budget in the unit it
// was given (FR-041).
auto scenarioSkippedListLine() -> void
{
  auto table = sg::hunt::EntryHunt {};
  table.skipped = {
      sg::hunt::SkippedEvent {
          .address = address(5),
          .kind = sg::hunt::SkippedKind::PERMISSION_BLOCKED,
          .stage = std::nullopt,
      },
      sg::hunt::SkippedEvent {
          .address = address(6),
          .kind = sg::hunt::SkippedKind::NOT_ENCODABLE,
          .stage = 2,
      },
  };
  // Nothing stands in the plan, so both refusals join the denominator: the
  // host that refuses everything reads `events=0/N` (D-12, FR-041).
  const auto record = contractRecord(1'260'000, {}, 0);

  check(capturePrint([&] {
          sg::detail::printHuntSkips(table, record);
        })
            == "hunt: skipped cpu/hunt005 permission-blocked\n"
               "hunt: skipped cpu/hunt006 not-encodable stage=2\n"
               "hunt: plan summary events=0/2 passes=14 budget=120s\n",
        "the skipped line prints the kind the catalog prints and the stage "
        "that lost the event, and the summary prints the counts and the "
        "budget under which the plan stood (FR-041, D-10)");
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
                   "hunt: rank 1 cpu/hunt000 iter=100.000 cv=0.00\n"
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

// The scenarios of this file: what CTest runs with no argument, and what
// `argv[1]` selects from. One entry per shape of data-model E-07.
[[nodiscard]] auto scenarios() -> const std::vector<Scenario>&
{
  static const std::vector<Scenario> kList {
      Scenario {"plan-line-shape", &scenarioPlanLineShape},
      Scenario {"plan-line-two-stage", &scenarioPlanLineTwoStage},
      Scenario {"plan-line-multiplex", &scenarioPlanLineMultiplex},
      Scenario {"plan-line-budget-cut", &scenarioPlanLineBudgetCut},
      Scenario {"hunt-block-order", &scenarioHuntBlockOrder},
      Scenario {"rank-row-marks", &scenarioRankRowMarks},
      Scenario {"skipped-list-line", &scenarioSkippedListLine},
      Scenario {"normalized-iter-line", &scenarioNormalizedIterLine},
      Scenario {"hunt-none-silence", &scenarioHuntNoneSilence},
  };
  return kList;
}

}  // namespace

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
    scenario.body();
  }
  std::printf("harness_hunt_report_test: ok\n");
  return 0;
}
