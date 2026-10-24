// ============================================================================
// The hunt collection runs (specs/029-hunter-core T017, T036, T071, T073;
// US1, US2;
// FR-013, FR-021, FR-022, FR-041, FR-092; AC-2 to AC-4, SC-02 to SC-04,
// SC-07; research D-2, D-10, D-12, D-18, D-19, PD-13; data-model E-04 to
// E-08, E-10).
//
// The seam is `sg::hunt::runHuntNamed` over the scripted host of
// `detail/hunt_fixture.hpp`, which publishes the catalog, the anchors and the
// programmable-counter limits, so a scenario needs no PMU, no sysfs seam, no
// sleep and no clock (FR-092). The seam answers the entry record and the
// run-level record as one pair (T126). The printed half of a claim reads
// `sg::detail::printResult` behind the stdout capture of
// `harness_pass_report_test.cpp`, with the collected `EntryHunt` standing on
// the result's own `hunt` field the way a finished run leaves it and the
// pair's run-level record handed over beside it; the record half reads the
// fields of that `EntryHunt`. The scripted window counts come from
// `FakeProvider::readActions`, the scripted provider's own counter.
//
// "the run exiting 0" is read here as `runHuntNamed` answering the pair:
// the error is the route that ends the run nonzero, so an answered record
// is the collected side of the exit status the contract names. The
// command line's own status is T018's seam.
//
// Each scenario scripts a provider of its own, and the counters system takes
// one object path per process, so each scenario runs in a re-exec of this
// binary, the pattern of `harness_pass_collect_test.cpp`. The list below is
// what CTest runs, so a scenario absent from it never runs, and `argv[1]`
// selects one entry of it. Frameworkless check()/fail() convention, the one
// `harness_pass_report_test.cpp` carries.
// ============================================================================

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <sched.h>
#include <sys/wait.h>
#include <unistd.h>

#include "detail/hunt_fixture.hpp"
#include "detail/hunt_plan.hpp"
#include "detail/internal.hpp"
#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"
#include "speedgun-ng/counters_fake.hpp"
#include "speedgun-ng/counters_system.hpp"
#include "speedgun-ng/hunt.hpp"

namespace
{

using sg::hunt::EntryHunt;
using sg::hunt::HuntBudget;
using sg::hunt::HuntOptions;
using sg::hunt::RankedEvent;
using sg::hunt::RunHunt;
using sg::hunt::SkippedEvent;
using sg::hunt::SkippedKind;

// The file one scenario's printed report is captured to, the scratch file of
// `harness_pass_report_test.cpp` spelled for this binary.
constexpr const char* kScratchPath = "harness_hunt_collect_out.txt";

// The scripted limit the retry arms split over: six leaves per pass on `cpu`,
// the anchors counted, the row `kHuntRows` carries for AC-1.
constexpr std::size_t kCollectLimit = 6;

// The events a retry arm seats in one pass: four events over two anchors fill
// the scripted limit exactly, so the group open is the widest the plan asks
// for and a narrower scripted leaf ceiling refuses exactly that open.
constexpr std::size_t kWidePassEvents = 4;

// The events the two retry arms select: two passes of four at the scripted
// limit, so the run reaches a second seat with the first pass measured.
constexpr std::size_t kRetryPassEvents = 2 * kWidePassEvents;

// The leaves the narrowest retry pass asks for: one event beside the two
// anchors of its device (D-18).
constexpr std::size_t kSoloRetryLeaves = 3;

// The windows of the gapped arm: twenty repeats of one column, one of whose
// windows carries the scripted gap, so nineteen clean windows stand (FR-022).
constexpr std::size_t kGapRepeats = 20;

// The contexts of the threaded arm (FR-092).
constexpr int kThreadCount = 2;

// The swings of the leaf the noisy arm scripts: four endpoint samples whose
// consecutive differences spread wide, so the coefficient of variation of the
// column clears any threshold the scenario can pick.
constexpr std::uint64_t kNoisySeed = 1;

// The events the two-stage arm selects: four of them at the scripted limit of
// six, which one pass seats, so each stage of the run costs one pass run.
constexpr std::size_t kStageEvents = 4;

// The columns stage 2 re-measures: three times the `--hunt-top=1` the scenario
// sets, one short of the events it selects (D-2, FR-013).
constexpr std::size_t kStageTwoColumns = 3;

[[noreturn]] auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS HUNT COLLECT TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool condition, const char* what) -> void
{
  if (!condition) {
    fail(what);
  }
}

auto closeTo(const double value, const double expected) -> bool
{
  return std::abs(value - expected) <= 0.001 * (std::abs(expected) + 1.0);
}

// The scripted provider of the running scenario, kept because the record
// claims and the group-refusal message are read off it after the run.
sg::counters::FakeProvider* host = nullptr;

// ---------------------------------------------------------------------------
// The scripted host
// ---------------------------------------------------------------------------

// Lay the hunt host out at the limit the scenario names and register it, the
// fluent shape `huntHost` states: the scenario owns the reference and scripts
// its own fact onto it before this call returns.
auto registerHuntHost(std::optional<std::size_t> limit,
                      const std::function<void(sg::counters::FakeProvider&)>&
                          script = {}) -> sg::counters::FakeProvider&
{
  auto provider = std::make_unique<sg::counters::FakeProvider>();
  sg::test::huntHost(*provider, limit);
  if (script) {
    script(*provider);
  }
  auto* scripted = provider.get();
  const auto settled =
      sg::counters::System::local().registerProvider(std::move(provider));
  check(settled.has_value(), "the hunt fixture registers on the local system");
  host = scripted;
  return *scripted;
}

// Refuse one scripted leaf outright: the availability the discovery answer
// gives for it is the kind a skipped entry must then carry (D-10). The tree
// freezes at registration (system.cpp:101), so every refusal is scripted
// before `registerProvider` through the callback of `registerHuntHost`.
auto refuseLeaf(sg::counters::FakeProvider& provider,
                std::string_view leaf,
                sg::counters::Availability kind) -> void
{
  provider.addCounter(sg::test::kHuntCpu, leaf, "none", "refused event", kind);
}

// The selection text of the catalog addresses a scenario asks for, the comma
// list the option spells (contracts/hunt-cli.md).
auto selectionOf(const std::vector<std::size_t>& indexes) -> std::string
{
  std::string text;
  for (const std::size_t index : indexes) {
    if (!text.empty()) {
      text += ',';
    }
    text +=
        sg::test::huntAddress(sg::test::kHuntCpu, sg::test::huntEvent(index));
  }
  return text;
}

auto selectionOfNames(const std::vector<std::string>& leaves) -> std::string
{
  std::string text;
  for (const auto& leaf : leaves) {
    if (!text.empty()) {
      text += ',';
    }
    text += sg::test::huntAddress(sg::test::kHuntCpu, leaf);
  }
  return text;
}

auto optionsFor(std::string selection,
                const std::size_t top = 20,
                const double noise = 10.0,
                const std::size_t repeats = 3) -> HuntOptions
{
  HuntOptions options;
  options.selection = std::move(selection);
  options.top = top;
  options.noise = noise;
  options.repeats = repeats;
  return options;
}

// The arming of the two retry arms. `planPasses` seats by first fit, so pass 0
// carries at least as many leaves of a device as any later pass, and no
// ceiling fixed before the run refuses a later pass while admitting pass 0 -
// which pass 0 must keep admitting, because its seat failure is the run's one
// clean failure (spec PC-9, T071). The scripted group ceiling is a plain
// member the provider reads at each `open`, so the benchmark body lowers it
// between the two seats: the runner calls the body once per attempt, and a
// hunt run fixes one iteration per window with no warm-up and no calibration,
// so the attempts of the first pass are the first `repeats` calls. Each arm
// asserts the attempt count its run ends on, so a change to that shape fails
// the arm, so an arm at the wrong moment cannot pass unseen.
std::size_t armAfterAttempts = 0;
std::size_t armedCeiling = 0;
std::atomic<std::size_t> bodyAttempts {0};

auto armRetryCeilingAfterPass(const std::size_t repeats,
                              const std::size_t ceiling) -> void
{
  armAfterAttempts = repeats;
  armedCeiling = ceiling;
}

// The catalog addresses of the first `count` events, the selection of an arm
// that takes one event per catalog entry from the front.
auto selectionCount(const std::size_t count) -> std::string
{
  std::vector<std::size_t> indexes;
  for (std::size_t index = 0; index < count; ++index) {
    indexes.push_back(index);
  }
  return selectionOf(indexes);
}

auto bmHuntBody(sg::State& state) -> void
{
  for (auto index : state) {
    sg::doNotOptimize(index);
  }
  // The retry arms' mid-run arming, stated above `armRetryCeilingAfterPass`.
  ++bodyAttempts;
  if (armAfterAttempts != 0 && bodyAttempts == armAfterAttempts) {
    host->setGroupLeafLimit(sg::test::kHuntCpu, armedCeiling);
  }
}

auto needInstance(const std::string& name) -> sg::Instance*
{
  for (auto& instance : sg::detail::instances()) {
    if (instance.name == name) {
      return &instance;
    }
  }
  check(false, "the expansion produced the named instance");
  return nullptr;
}

// One scripted hunt run: the pair `runHuntNamed` answered and the sampling
// actions it cost the scripted provider. A run that answers fills both
// records together; a refused hunt fills neither. `figureNs` is the
// nanoseconds of one pass the caller hands the planner (FR-012): the
// library route is handed its figure, while the command-line route times
// one (T119).
struct HuntAccount
{
  std::optional<EntryHunt> hunt;
  std::optional<RunHunt> run;
  std::uint64_t reads = 0;
  std::uint64_t opens = 0;
};

auto runHuntMeasured(sg::BenchmarkHandle& handle,
                     const HuntOptions& options,
                     std::optional<std::uint64_t> figureNs = std::nullopt)
    -> HuntAccount
{
  const std::uint64_t before = host->readActions();
  const std::uint64_t opensBefore = host->windowOpens();
  auto answered = sg::hunt::runHuntNamed(handle.name(), options, figureNs);
  const std::uint64_t after = host->readActions();
  const std::uint64_t opensAfter = host->windowOpens();
  if (answered.has_value()) {
    return HuntAccount {std::optional<EntryHunt> {std::move(answered->entry)},
                        std::optional<RunHunt> {std::move(answered->run)},
                        after - before,
                        opensAfter - opensBefore};
  }
  return HuntAccount {
      std::nullopt, std::nullopt, after - before, opensAfter - opensBefore};
}

// ---------------------------------------------------------------------------
// The printed side, the capture seam of `harness_pass_report_test.cpp`
// ---------------------------------------------------------------------------

template<class Call>
auto captureThrough(const std::string& path, Call&& call) -> void
{
  std::FILE* saved = nullptr;
  const int savedFd = dup(STDOUT_FILENO);
  if (savedFd >= 0) {
    saved = fdopen(savedFd, "w");
  }
  check(saved != nullptr, "the capture kept the real stdout");

  std::FILE* replacement = std::fopen(path.c_str(), "we");
  check(replacement != nullptr, "the capture opened its scratch file");
  const int replacementFd = fileno(replacement);
  check(replacementFd >= 0, "the capture file carries a descriptor");
  const bool redirected = dup2(replacementFd, STDOUT_FILENO) == STDOUT_FILENO;
  check(redirected, "the capture redirected stdout");
  if (std::setvbuf(stdout, nullptr, _IONBF, 0) != 0) {
    fail("the capture could not unbuffer the redirected stdout");
  }
  std::fflush(stdout);

  call();

  std::fflush(stdout);
  const bool restored = dup2(savedFd, STDOUT_FILENO) == STDOUT_FILENO;
  check(restored, "the capture restored stdout");
  if (std::setvbuf(stdout, nullptr, _IOLBF, BUFSIZ) != 0) {
    fail("the restored stdout could not set its buffering mode");
  }
  std::fclose(saved);
  std::fclose(replacement);
}

auto readAll(const std::string& path) -> std::string
{
  std::ifstream file(path);
  check(static_cast<bool>(file), "the captured file opens for reading");
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

auto splitLines(const std::string& text) -> std::vector<std::string>
{
  std::vector<std::string> lines;
  std::string current;
  for (const char symbol : text) {
    if (symbol == '\n') {
      lines.push_back(current);
      current.clear();
      continue;
    }
    current.push_back(symbol);
  }
  check(current.empty(), "every captured line ends with its newline");
  return lines;
}

auto linesHolding(const std::vector<std::string>& lines,
                  const std::string& needle) -> std::vector<std::string>
{
  std::vector<std::string> held;
  for (const auto& line : lines) {
    if (line.find(needle) != std::string::npos) {
      held.push_back(line);
    }
  }
  return held;
}

// The printed console tail of a collected run: the benchmark's own row, then
// the hunt lines `EntryHunt` owes, behind the capture (contracts/hunt-cli.md,
// D-20). The run that produced the pair is the real scripted run, so the
// bytes are those of a finished run, whole and unplanted.
auto printedLines(sg::Instance& instance,
                  const EntryHunt& hunt,
                  const RunHunt& run) -> std::vector<std::string>
{
  sg::detail::RunOptions options;
  options.fixedIterations = 1;
  options.repetitions = 1;
  sg::detail::Runner runner(options);
  sg::BenchmarkResult result = runner.run(instance);
  result.hunt = hunt;
  captureThrough(kScratchPath, [&] { sg::detail::printResult(result, run); });
  return splitLines(readAll(kScratchPath));
}

// The ranked row of one address; null when the run measured no such address.
auto entryOf(const EntryHunt& hunt,
             const std::string& address) -> const RankedEvent*
{
  for (const auto& entry : hunt.entries) {
    if (entry.address == address) {
      return &entry;
    }
  }
  return nullptr;
}

// The skipped entry of one address; null when the run measured it.
auto skippedOf(const EntryHunt& hunt,
               const std::string& address) -> const SkippedEvent*
{
  for (const auto& skipped : hunt.skipped) {
    if (skipped.address == address) {
      return &skipped;
    }
  }
  return nullptr;
}

// The spelling `report.cpp availabilityName` prints for one kind, the one
// spelling the console owes (hunt.hpp, E-06).
auto kindName(const SkippedKind kind) -> const char*
{
  switch (kind) {
    case SkippedKind::PERMISSION_BLOCKED:
      return "permission-blocked";
    case SkippedKind::NOT_ENCODABLE:
      return "not-encodable";
    case SkippedKind::ABSENT:
      return "absent";
    case SkippedKind::SCOPE_REFUSED:
      return "scope-refused";
    case SkippedKind::GAP:
      return "gap";
  }
  return "unknown";
}

auto rankCount(const EntryHunt& hunt) -> std::size_t
{
  std::size_t ranked = 0;
  for (const auto& entry : hunt.entries) {
    if (entry.rank.has_value()) {
      ++ranked;
    }
  }
  return ranked;
}

// ---------------------------------------------------------------------------
// AC-3, SC-03: the noisy event stands in the record and out of the top N
// ---------------------------------------------------------------------------

auto scenarioNoisyEventStandsOutOfTheTop() -> void
{
  // One flat high-rate event and one that swings: the calibration loop reads
  // every leaf hundreds of times before the repetition windows, so a finite
  // point list is long consumed by then. The seeded delta tail is the one
  // script that keeps stepping through the repetitions, and its varying steps
  // are what give the column a spread (hunt_fixture.hpp, FR-030).
  const std::string loud =
      sg::test::huntAddress(sg::test::kHuntCpu, sg::test::huntEvent(199));
  const std::string swing =
      sg::test::huntAddress(sg::test::kHuntCpu, sg::test::huntEvent(2));
  registerHuntHost(
      kCollectLimit,
      [](sg::counters::FakeProvider& provider)
      {
        provider.setPoints(
            sg::test::kHuntCpu, sg::test::huntEvent(2), {0}, 0, kNoisySeed);
      });

  const auto options = optionsFor(selectionOf({199, 2}), 1);
  auto handle = sg::registerBenchmark(&bmHuntBody, "bmHuntNoisy");
  sg::detail::expandRegistry();
  const HuntAccount run = runHuntMeasured(handle, options);
  check(run.hunt.has_value(),
        "a noisy event is a mark and not an error, so the run answers its "
        "record and exits 0 (AC-3, D-18)");
  const EntryHunt& hunt = *run.hunt;

  check(hunt.skipped.empty(),
        "a noisy event is not a refused event, so it stands in no skipped "
        "list (AC-3)");
  check(hunt.entries.size() == 2,
        "both scripted events stand in the record, the noisy one kept in it "
        "(AC-3, I-4)");
  const auto* loudEntry = entryOf(hunt, loud);
  const auto* swingEntry = entryOf(hunt, swing);
  check(loudEntry != nullptr && swingEntry != nullptr,
        "the record carries a row per scripted address (E-06)");
  check(loudEntry->rank.has_value() && loudEntry->rank.value() == 1,
        "the high-rate scripted event ranks first (AC-3)");
  check(swingEntry->noisy && !swingEntry->rank.has_value(),
        "the flat scripted event above the CV threshold is marked noisy and "
        "kept out of the top N (D-7, AC-3)");
  check(swingEntry->cv > options.noise,
        "the noisy mark is the unrounded CV read against --hunt-noise (D-7)");
  check(!loudEntry->noisy && closeTo(loudEntry->cv, 0.0),
        "a scripted event with no spread carries no noise and a CV of zero "
        "(I-6)");
  check(rankCount(hunt) == 1 && hunt.top == 1,
        "at most --hunt-top entries carry a rank (I-5)");

  const auto lines = printedLines(*needInstance("bmHuntNoisy"), hunt, *run.run);
  check(linesHolding(lines, "hunt: rank 1 " + loud).size() == 1,
        "the console prints the high-rate event as rank 1 (D-20, AC-3)");
  const auto tailLines = linesHolding(lines, "hunt: rank - " + swing);
  check(tailLines.size() == 1 &&
            tailLines[0].find(" mark=noisy") != std::string::npos,
        "the excluded tail prints the noisy event with rank - and its mark "
        "(D-7, D-20)");
  std::printf("noisy-out-of-top: ok\n");
}

// ---------------------------------------------------------------------------
// AC-4, SC-04, D-10: one skipped entry per refused Availability
// ---------------------------------------------------------------------------

auto scenarioRefusedAvailabilityJoinsSkipped() -> void
{
  const std::vector<std::pair<std::string, SkippedKind>> refused {
      {"hunt_refused_perm", SkippedKind::PERMISSION_BLOCKED},
      {"hunt_refused_enc", SkippedKind::NOT_ENCODABLE},
      {"hunt_refused_abs", SkippedKind::ABSENT},
      {"hunt_refused_scope", SkippedKind::SCOPE_REFUSED},
  };
  const std::vector<sg::counters::Availability> kinds {
      sg::counters::Availability::PERMISSION_BLOCKED,
      sg::counters::Availability::NOT_ENCODABLE,
      sg::counters::Availability::ABSENT,
      sg::counters::Availability::SCOPE_REFUSED,
  };
  registerHuntHost(kCollectLimit,
                   [&](sg::counters::FakeProvider& provider)
                   {
                     for (std::size_t index = 0; index < refused.size();
                          ++index) {
                       refuseLeaf(provider, refused[index].first, kinds[index]);
                     }
                   });

  HuntOptions options = optionsFor(selectionOfNames({refused[0].first,
                                                     refused[1].first,
                                                     refused[2].first,
                                                     refused[3].first,
                                                     sg::test::huntEvent(0)}));
  auto handle = sg::registerBenchmark(&bmHuntBody, "bmHuntRefused");
  sg::detail::expandRegistry();
  const HuntAccount run = runHuntMeasured(handle, options);
  check(run.hunt.has_value(),
        "a refused event is a skipped entry and the run exits 0 (AC-4, D-10)");
  const EntryHunt& hunt = *run.hunt;

  check(hunt.skipped.size() == refused.size(),
        "one scripted event per refused Availability stands in the skipped "
        "list (AC-4, I-4)");
  for (const auto& [leaf, kind] : refused) {
    const std::string address = sg::test::huntAddress(sg::test::kHuntCpu, leaf);
    const auto* skipped = skippedOf(hunt, address);
    check(skipped != nullptr && skipped->kind == kind,
          "the skipped entry carries the Availability of its own refusal "
          "(D-10, FR-050)");
    check(entryOf(hunt, address) == nullptr,
          "a refused address stands in no ranked row (I-4)");
  }
  const auto* counted = entryOf(
      hunt, sg::test::huntAddress(sg::test::kHuntCpu, sg::test::huntEvent(0)));
  check(counted != nullptr && counted->rank.has_value(),
        "the countable event beside four refusals still ranks (AC-4)");
  check(hunt.entries.size() == 1,
        "no refused address reaches the entries list (I-4)");

  const auto lines =
      printedLines(*needInstance("bmHuntRefused"), hunt, *run.run);
  for (const auto& [leaf, kind] : refused) {
    const std::string line = "hunt: skipped "
        + sg::test::huntAddress(sg::test::kHuntCpu, leaf) + " "
        + kindName(kind);
    check(linesHolding(lines, line).size() == 1,
          "the console prints one skipped line per refused event with the kind "
          "spelling report.cpp carries (D-10, D-20)");
  }
  std::printf("skipped-per-availability: ok\n");
}

// ---------------------------------------------------------------------------
// FR-022: a gapped window stands in both lists
// ---------------------------------------------------------------------------

auto scenarioGappedWindowStandsInBothLists() -> void
{
  registerHuntHost(kCollectLimit);
  const std::size_t index = 5;
  const std::string address =
      sg::test::huntAddress(sg::test::kHuntCpu, sg::test::huntEvent(index));
  // The plan's sampling-cost calibration runs 64 warm-up plus 257 measured
  // actions before the benchmark's first endpoint, and each repetition
  // window then costs exactly two sampling actions: action 322 is the first
  // fold of the first window, so exactly one of the run's windows for that
  // column measures nothing (FR-022, the geometry harness_capture_test
  // documents).
  host->setGapActions(sg::test::kHuntCpu, sg::test::huntEvent(index), {322});

  const auto options = optionsFor(selectionOf({index}), 20, 10.0, kGapRepeats);
  auto handle = sg::registerBenchmark(&bmHuntBody, "bmHuntGap");
  sg::detail::expandRegistry();
  const HuntAccount run = runHuntMeasured(handle, options);
  check(run.hunt.has_value(),
        "a gapped window is a disclosure and not an error (FR-022, D-10)");
  const EntryHunt& hunt = *run.hunt;

  const auto* entry = entryOf(hunt, address);
  const auto* skipped = skippedOf(hunt, address);
  check(entry != nullptr,
        "the event of a gapped window still stands in entries, because the "
        "run measured it (I-4, FR-022)");
  check(entry != nullptr && entry->samples == kGapRepeats - 1,
        "nineteen clean windows contribute and the gapped one contributes "
        "nothing (FR-022, E-05)");
  check(skipped != nullptr && skipped->kind == SkippedKind::GAP,
        "the same address stands in the skipped list with kind gap (FR-022, "
        "I-4)");
  check(hunt.skipped.size() == 1 && hunt.entries.size() == 1,
        "one gapped window gives one skipped entry and one record row "
        "(FR-022)");

  const auto lines = printedLines(*needInstance("bmHuntGap"), hunt, *run.run);
  check(linesHolding(lines, "hunt: skipped " + address + " gap").size() == 1,
        "the console prints the gapped address with the kind word gap "
        "(FR-022, D-20)");
  std::printf("gapped-window-both-lists: ok\n");
}

// ---------------------------------------------------------------------------
// D-12, FR-041: the host that refuses everything
// ---------------------------------------------------------------------------

auto scenarioAllRefusedHost() -> void
{
  // Every catalog leaf refuses, the anchors included: this is the host whose
  // discovery answers nothing, the shape that is not a refusal (D-12).
  registerHuntHost(
      kCollectLimit,
      [](sg::counters::FakeProvider& provider)
      {
        for (std::size_t index = 0; index < sg::test::kHuntEvents; ++index) {
          refuseLeaf(provider,
                     sg::test::huntEvent(index),
                     sg::counters::Availability::PERMISSION_BLOCKED);
        }
        refuseLeaf(provider,
                   sg::test::kHuntCycle,
                   sg::counters::Availability::PERMISSION_BLOCKED);
        refuseLeaf(provider,
                   sg::test::kHuntInstr,
                   sg::counters::Availability::PERMISSION_BLOCKED);
      });

  const auto options = optionsFor("all");
  auto handle = sg::registerBenchmark(&bmHuntBody, "bmHuntAllRefused");
  sg::detail::expandRegistry();
  const HuntAccount run = runHuntMeasured(handle, options);
  check(run.hunt.has_value(),
        "the host that refuses everything is not an error: the run exits 0 "
        "(D-12, FR-041)");
  const EntryHunt& hunt = *run.hunt;

  check(hunt.entries.empty(),
        "the all-refused host has no ranked row (D-12, FR-041)");
  check(hunt.skipped.size() == sg::test::kHuntEvents,
        "the skipped list carries one entry per refused catalog event "
        "(D-12, FR-041)");
  check(hunt.passCount == 0, "no pass stands for a plan with no event (D-12)");
  const bool allPermissionBlocked =
      std::all_of(hunt.skipped.begin(),
                  hunt.skipped.end(),
                  [](const SkippedEvent& skipped)
                  { return skipped.kind == SkippedKind::PERMISSION_BLOCKED; });
  check(allPermissionBlocked,
        "every skipped entry carries the Availability its own refusal gave "
        "(D-10)");

  const auto lines =
      printedLines(*needInstance("bmHuntAllRefused"), hunt, *run.run);
  const auto planLines = linesHolding(lines, "hunt: plan events=0");
  check(planLines.size() == 1,
        "the plan line prints with events=0 on the host that refuses "
        "everything (D-12, FR-041)");
  check(linesHolding(lines, "hunt: rank ").empty(),
        "the all-refused host prints no ranked row (D-12, FR-041)");
  check(linesHolding(lines, "hunt: skipped ").size() == sg::test::kHuntEvents,
        "the skipped list prints one line per refused event (D-12, FR-041)");
  check(linesHolding(lines,
                     "hunt: plan summary events=0/"
                         + std::to_string(sg::test::kHuntEvents))
                .size()
            == 1,
        "the summary reads events=0/N on the all-refused host (D-12, FR-041)");
  std::printf("all-refused-host: ok\n");
}

// ---------------------------------------------------------------------------
// AC-10: the host that opens everything and yields no rate
// ---------------------------------------------------------------------------

// The events of the arm: three of them seat in one pass at the scripted limit
// of six, so nothing refuses and every window opens.
constexpr std::size_t kUnmeasurableEvents = 3;

// The corsairone defect record (`specs/029-hunter-core/results/
// corsairone-hunt-all.md`): the host opens every window and every event
// counts, while the anchor of the norm reads zero, so no sample contributes a
// rate (contracts/hunt-api.md: "a sample whose anchor read zero for its pass
// contributes no rate"). `rankEvents` refuses to summarize an empty rate list,
// so the collection path owes the precondition it hands that function; this
// arm pins the run answering a skipped ranking that states its reason, over
// the abort the record captured.
auto scenarioUnmeasurableAnchorSkipsEveryEvent() -> void
{
  registerHuntHost(kCollectLimit,
                   [](sg::counters::FakeProvider& provider) {
                     provider.setPoints(
                         sg::test::kHuntCpu, sg::test::kHuntInstr, {0}, 0);
                   });

  const auto options = optionsFor(selectionCount(kUnmeasurableEvents));
  auto handle = sg::registerBenchmark(&bmHuntBody, "bmHuntUnmeasurable");
  sg::detail::expandRegistry();
  const HuntAccount run = runHuntMeasured(handle, options);
  check(run.hunt.has_value(),
        "the host that yields no rate ends as a record, not as the abort the "
        "record captured (AC-10)");
  const EntryHunt& hunt = *run.hunt;

  check(hunt.entries.size() == kUnmeasurableEvents,
        "every address whose windows opened still stands as a rank row (E-06)");
  const bool noRate =
      std::all_of(hunt.entries.begin(),
                  hunt.entries.end(),
                  [](const RankedEvent& entry) { return entry.samples == 0; });
  check(noRate,
        "an anchor that never counted leaves no sample for any address to "
        "summarize");
  check(hunt.skipped.size() == kUnmeasurableEvents,
        "an address with no rate carries the skipped entry that states why "
        "(FR-021, I-4)");
  const bool allAbsent =
      std::all_of(hunt.skipped.begin(),
                  hunt.skipped.end(),
                  [](const SkippedEvent& skipped)
                  { return skipped.kind == SkippedKind::ABSENT; });
  check(allAbsent,
        "the kind names the denominator that is not there, not a gap the rows "
        "never show (D-10)");
  const auto lines =
      printedLines(*needInstance("bmHuntUnmeasurable"), hunt, *run.run);
  check(linesHolding(lines, "hunt: rank ").size() == kUnmeasurableEvents,
        "the ranking still prints its rows, the way the narrow host runs did");
  check(linesHolding(lines, "hunt: skipped ").size() == kUnmeasurableEvents,
        "every address stands beside the reason it ranked nothing, so no "
        "empty table prints without a word why (AC-10)");
  check(linesHolding(lines,
                     "hunt: plan summary events=0/"
                         + std::to_string(kUnmeasurableEvents))
                .size()
            == 1,
        "the summary reads events=0/N on the host that measured nothing");
  std::printf("unmeasurable-anchor-skips-every-event: ok\n");
}

// ---------------------------------------------------------------------------
// AC-2, SC-02, D-19: the device that publishes no limit
// ---------------------------------------------------------------------------

auto scenarioNoLimitHostMarksEveryEvent() -> void
{
  registerHuntHost(std::nullopt);
  const auto options = optionsFor(selectionOf({0, 1, 2}));
  auto handle = sg::registerBenchmark(&bmHuntBody, "bmHuntMultiplex");
  sg::detail::expandRegistry();
  const HuntAccount run = runHuntMeasured(handle, options);
  check(run.hunt.has_value(),
        "the no-limit host settles into a measured run (AC-2)");
  const EntryHunt& hunt = *run.hunt;

  check(hunt.method.has_value()
            && hunt.method.value() == sg::MetricMethod::MULTIPLEX,
        "the no-limit device settles MULTIPLEX (AC-2, D-19, spec PC-2)");
  check(!hunt.entries.empty(),
        "the run still measures its scripted events (AC-2)");
  const bool allMultiplexed =
      std::all_of(hunt.entries.begin(),
                  hunt.entries.end(),
                  [](const RankedEvent& entry) { return entry.multiplexed; });
  check(allMultiplexed,
        "every event of a MULTIPLEX plan is marked multiplexed (D-19, SC-02)");

  const auto lines =
      printedLines(*needInstance("bmHuntMultiplex"), hunt, *run.run);
  check(linesHolding(lines, "MULTIPLEX").size() >= 1,
        "the plan line carries the method word MULTIPLEX (D-19, D-20)");
  const auto rankLines = linesHolding(lines, "hunt: rank ");
  check(rankLines.size() == hunt.entries.size(),
        "the console prints one row per measured event (D-20)");
  const bool everyRowWords =
      std::all_of(rankLines.begin(),
                  rankLines.end(),
                  [](const std::string& line)
                  { return line.find("multiplexed") != std::string::npos; });
  check(everyRowWords,
        "every row of a MULTIPLEX run carries the word multiplexed (D-19)");
  std::printf("multiplex-no-limit: ok\n");
}

// ---------------------------------------------------------------------------
// D-18, FR-021: the group refusal the pass loop retries one event at a time
// ---------------------------------------------------------------------------

// The scripted widest leaf set one window may carry on `cpu` while the plan
// still seats four events and two anchors per pass at the scripted limit.
auto scriptGroupCeiling(const std::size_t leaves) -> void
{
  host->setGroupLeafLimit(sg::test::kHuntCpu, leaves);
}

auto scenarioWidePassRetriesOneAtATime() -> void
{
  registerHuntHost(kCollectLimit);
  // Eight events fill two passes at the scripted limit of six, and the body
  // drops the ceiling to the three leaves a solo retry carries once the first
  // pass has measured its attempts: the second group is the refused one, the
  // first keeps its failure route, and the one-at-a-time retry inside the same
  // pass loop measures the events (D-18, FR-021, T071).
  const auto options = optionsFor(selectionCount(kRetryPassEvents));
  armRetryCeilingAfterPass(options.repeats, kSoloRetryLeaves);
  auto handle = sg::registerBenchmark(&bmHuntBody, "bmHuntRetry");
  sg::detail::expandRegistry();
  const HuntAccount run = runHuntMeasured(handle, options);
  check(run.hunt.has_value(),
        "a retried pass is a retry and not an error, so the run exits 0 "
        "(D-18, FR-021)");
  const EntryHunt& hunt = *run.hunt;

  check(hunt.retriedPasses == 1,
        "retriedPasses counts the pass whose group open refused (D-18)");
  check(hunt.skipped.empty(),
        "an event the solo retry measured joins no skipped list (D-18)");
  check(hunt.entries.size() == kRetryPassEvents,
        "every event of the plan, those of the refused pass included, still "
        "reaches the record (D-18, FR-021)");
  check(hunt.passCount == 2 + kWidePassEvents,
        "the record counts the two planned passes and the four the retry "
        "appended (PD-12, D-18)");
  const std::string message = host->lastRefusalMessage();
  check(message.find(std::string(sg::test::kHuntCpu)) != std::string::npos
            && message.find("scripted count of "
                            + std::to_string(kSoloRetryLeaves))
                   != std::string::npos,
        "the refusal the retry answered is the scripted group refusal of the "
        "armed ceiling, naming the device it refused (PD-09, T071)");
  check(bodyAttempts == options.repeats * (1 + kWidePassEvents),
        "the first pass ran its own attempts and only the four solo retry "
        "passes ran after the ceiling was armed, which is the arming moment "
        "this arm reads its refusal from (T071)");
  std::printf("group-leaf-retry: ok\n");
}

auto scenarioSoloRefusalMapsItsKind() -> void
{
  registerHuntHost(kCollectLimit);
  // Two leaves per window: even the solo retry cannot seat an event beside its
  // two anchors, so the event that still refuses joins the skipped list with
  // the kind of its own refusal, and the run still exits 0 (D-18, FR-021,
  // spec PC-1, T073).
  const auto options = optionsFor(selectionCount(kRetryPassEvents));
  armRetryCeilingAfterPass(options.repeats, kSoloRetryLeaves - 1);
  auto handle = sg::registerBenchmark(&bmHuntBody, "bmHuntSoloRefusal");
  sg::detail::expandRegistry();
  const HuntAccount run = runHuntMeasured(handle, options);
  check(run.hunt.has_value(),
        "an event that still refuses after the retry is a skipped entry, so "
        "the run exits 0 (D-18, FR-021)");
  const EntryHunt& hunt = *run.hunt;

  check(hunt.retriedPasses == 1,
        "the retry pass is counted even when the solo retry refuses (D-18)");
  check(hunt.entries.size() == kRetryPassEvents,
        "the events of the refused pass keep their row, gap-disclosed, while "
        "their refusal is what the skipped list names (PD-14, T073)");
  check(hunt.skipped.size() == kWidePassEvents,
        "every event of the refused pass stands once in the skipped list after "
        "its own retry refused, the gap it collected with replaced by the "
        "refusal (D-18, FR-021, I-4)");
  const bool mapped = std::all_of(hunt.skipped.begin(),
                                  hunt.skipped.end(),
                                  [](const SkippedEvent& skipped) {
                                    return skipped.kind == SkippedKind::ABSENT;
                                  });
  check(mapped,
        "the kind of a skipped retry entry is the kind its own refusal names "
        "to: the scripted group refusal states no catalog state, so the "
        "mapping answers ABSENT, the answer the pass loop's guessed "
        "NOT_ENCODABLE could never give (D-18, FR-021, spec PC-1, T073, T118)");
  // T118, T128: the state published for the refused column is the state whose
  // kind the skipped line spells, so one address states one thing on both
  // surfaces. A refused hunt column stands out of the printed rows: `--hunt`
  // prints the rows of the run before the hunt and refuses `--counter`
  // together with `--hunt`, so the published state is read off the record the
  // pass loop left, beside the line the same address prints below.
  const auto& refusedEvents =
      needInstance("bmHuntSoloRefusal")->huntRefusedEvents();
  check(refusedEvents.size() == hunt.skipped.size(),
        "the pass loop recorded one refused event per skipped entry (T073, "
        "T118)");
  const bool oneState = std::all_of(
      refusedEvents.begin(),
      refusedEvents.end(),
      [&hunt](const sg::detail::RefusedRetry& refused)
      {
        const auto entry =
            std::find_if(hunt.skipped.begin(),
                         hunt.skipped.end(),
                         [&refused](const SkippedEvent& skipped)
                         { return skipped.address == refused.address; });
        return entry != hunt.skipped.end()
            && sg::detail::skippedKindOf(refused.availability) == entry->kind
            && refused.availability == sg::counters::Availability::ABSENT;
      });
  check(oneState,
        "the availability the column publishes and the kind the skipped line "
        "spells are the two spellings of one state, and a refusal that states "
        "no state leaves both at absent (FR-021, T118, T128)");
  check(bodyAttempts == options.repeats,
        "the refused pass and its solo retries cost no attempt at all, which "
        "is the arming moment this arm reads its refusal from (T073)");

  const auto lines =
      printedLines(*needInstance("bmHuntSoloRefusal"), hunt, *run.run);
  const auto skippedLines = linesHolding(lines, "hunt: skipped ");
  check(skippedLines.size() == hunt.skipped.size(),
        "the console prints one line per still-refused event (D-20)");
  const bool spelled =
      std::all_of(hunt.skipped.begin(),
                  hunt.skipped.end(),
                  [&lines](const SkippedEvent& skipped)
                  {
                    return linesHolding(lines,
                                        "hunt: skipped " + skipped.address + " "
                                            + kindName(skipped.kind))
                               .size()
                        == 1;
                  });
  check(spelled,
        "each skipped line spells the kind of its own record entry (D-10, "
        "D-20)");
  std::printf("solo-refusal-maps-kind: ok\n");
}

// ---------------------------------------------------------------------------
// spec PC-9: a seat that fails for pass 0 keeps the present failure route
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// D-18, I-1: the retry of a column whose device holds neither anchor
// ---------------------------------------------------------------------------

// The group leaf limit of the armed refusal: one leaf below the three an
// event and the two anchors of its device need, one leaf above the single
// leaf an anchorless event needs alone, and the smallest limit the hunt plan
// admits for a device that holds no anchor (I-1).
constexpr std::size_t kAnchorlessRetryLimit = 2;
constexpr std::size_t kAnchorlessDeviceLimit = 3;
// The uncore events this scenario selects: the two the fixture scripts and
// the two its scripted callback adds beside the limit.
constexpr std::size_t kUncoreSelectedEvents = 4;

[[nodiscard]] auto uncoreAddress(const std::size_t offset) -> std::string
{
  return sg::test::huntAddress(
      sg::test::kHuntUncore,
      sg::test::huntEvent(sg::test::kHuntEvents + offset));
}

// D-18, FR-021, PD-12, I-1: the retry of a column takes the two anchors of
// that column's plan entry as the leaves of its pass and answers an anchor
// that stands empty with no leaf at all - the plan entry of a device that
// holds neither anchor carries neither (I-1). At the ceiling above, the retry
// of a `cpu` event asks three leaves and refuses while the retry of an
// anchorless event asks one and measures: two arms of one refusal answer one
// ceiling with two different leaf counts.
auto scenarioAnchorlessRetrySeatsItsEventAlone() -> void
{
  // The uncore device publishes a limit here and still holds neither anchor,
  // so its fourth event overflows the first pass beside the cpu events and
  // stands in the pass this scenario refuses (I-1, FR-011).
  registerHuntHost(
      kCollectLimit,
      []([[maybe_unused]] sg::counters::FakeProvider& provider)
      {
        provider.setProgrammableCounters(sg::test::kHuntUncore,
                                         kAnchorlessDeviceLimit);
        for (const std::size_t offset : {std::size_t {2}, std::size_t {3}}) {
          const auto name = sg::test::huntEvent(sg::test::kHuntEvents + offset);
          provider.addCounter(
              sg::test::kHuntUncore, name, "none", "hunt fixture uncore event");
          provider.setPoints(sg::test::kHuntUncore, name, {0}, 10.0);
        }
      });
  // The eight events of `all` beside four uncore events: the first pass seats
  // four cpu events beside the two anchors of `cpu` and three uncore events,
  // so the second pass carries the four remaining cpu events and the fourth
  // uncore event. Arming the limit of `cpu` below that seat refuses the pass,
  // and the retry that follows opens one pass per event of it (D-18, PD-12).
  const auto options = optionsFor(
      selectionCount(kRetryPassEvents) + ',' + uncoreAddress(0) + ','
      + uncoreAddress(1) + ',' + uncoreAddress(2) + ',' + uncoreAddress(3));
  armRetryCeilingAfterPass(options.repeats, kAnchorlessRetryLimit);
  auto handle = sg::registerBenchmark(&bmHuntBody, "bmHuntAnchorlessRetry");
  sg::detail::expandRegistry();
  const HuntAccount run = runHuntMeasured(handle, options);
  check(run.hunt.has_value(),
        "a hunt whose refused pass holds an event of a device with no anchor "
        "answers the entry record (D-18, FR-021)");
  const EntryHunt& hunt = *run.hunt;
  const auto& refused =
      needInstance("bmHuntAnchorlessRetry")->huntRefusedEvents();
  check(hunt.retriedPasses == 1, "one pass retried (D-18)");
  // The decision's product, one number: five events of the refused pass each
  // opened a retry pass, and exactly one of them - the anchorless event, whose
  // two anchors stand empty and therefore add no leaf - seated inside the
  // ceiling of two and measured (I-1).
  check(hunt.passCount == 2 + kWidePassEvents + 1,
        "the published pass count carries the two planned passes, the four "
        "the refused events opened and the one the anchorless event measured "
        "(PD-12, D-18, I-1)");
  check(run.opens == 2,
        "the first pass and the anchorless retry opened a group each, and no "
        "anchored retry opened one at this ceiling (I-1, FR-021)");
  check(bodyAttempts == options.repeats * 2,
        "the body ran in the first pass and in the one retry whose one-leaf "
        "seat the ceiling holds (I-1)");
  check(refused.size() == kWidePassEvents,
        "the refusal list holds the four anchored retries and not the "
        "anchorless retry, which seated its event alone (D-18, FR-021, I-1)");
  bool onlyAnchoredRefused = refused.size() == kWidePassEvents;
  for (const auto& event : refused) {
    if (event.address.rfind("cpu/", 0) != 0) {
      onlyAnchoredRefused = false;
    }
  }
  check(onlyAnchoredRefused,
        "every refusal names a cpu event, because the anchorless event seated "
        "its one leaf and refused nothing (I-1, FR-021)");
  // The four anchorless events do stand in the skipped list, and for their own
  // reason: no anchor of their device divides their value, so no rate of the
  // norm answers them (E-05, D-13). Their retry is not that reason - it
  // measured, which the pass count above reads.
  check(hunt.skipped.size() == kWidePassEvents + kUncoreSelectedEvents,
        "the skipped list carries the four refused anchored retries beside "
        "the four anchorless events the norm divides by nothing (E-05, D-13, "
        "FR-021)");
  bool everyAnchorlessSkipped = true;
  for (std::size_t offset = 0; offset < kUncoreSelectedEvents; ++offset) {
    if (skippedOf(hunt, uncoreAddress(offset)) == nullptr) {
      everyAnchorlessSkipped = false;
    }
  }
  check(everyAnchorlessSkipped,
        "each anchorless event answers a skipped entry of its own address, the "
        "state its device discloses and not a seat refusal (E-05, FR-021)");
  check(hunt.entries.size() == kRetryPassEvents + kUncoreSelectedEvents,
        "every event of the selection keeps its entry, the measured anchorless "
        "column beside the refused cpu ones (FR-021, PD-12)");
  std::printf("anchorless-retry-one-leaf: ok\n");
}

auto scenarioPassZeroSeatFailureKeepsExitOne() -> void
{
  registerHuntHost(kCollectLimit);
  // No window can carry even one leaf, so the first seat of pass 0 fails
  // and no retry shape answers it: the same compile serves warm-up and
  // calibration, so the run keeps its present failure route (spec PC-9).
  scriptGroupCeiling(0);

  const auto options = optionsFor(selectionOf({0, 1}));
  auto handle = sg::registerBenchmark(&bmHuntBody, "bmHuntPassZero");
  sg::detail::expandRegistry();
  const HuntAccount run = runHuntMeasured(handle, options);
  check(!run.hunt.has_value(),
        "a pass-0 seat failure answers the error route, the one that ends the "
        "run with exit 1, and not a record (spec PC-9)");
  check(!host->lastRefusalMessage().empty(),
        "the run reached the scripted provider before it failed its seat "
        "(spec PC-9)");
  std::printf("pass-zero-seat-failure: ok\n");
}

// The same route for the refusal a real host answers: the group of the first
// pass is too wide while a narrower set is admitted, which is exactly the
// refusal the retry route of D-18 answers for a pass after the first. Pass 0
// keeps its failure route, because the same compile serves warm-up and
// calibration, so the run answers the error the command line prints at exit 1
// (spec PC-9, contracts/hunt-api.md, T071).
auto scenarioPassZeroGroupRefusalExitsOne() -> void
{
  registerHuntHost(kCollectLimit);
  scriptGroupCeiling(kSoloRetryLeaves);

  const auto options = optionsFor(selectionCount(kWidePassEvents));
  auto handle = sg::registerBenchmark(&bmHuntBody, "bmHuntPassZeroGroup");
  sg::detail::expandRegistry();
  const HuntAccount run = runHuntMeasured(handle, options);
  check(!run.hunt.has_value(),
        "a refused pass-0 group answers the error route, the one the command "
        "line ends with exit 1, and not a record (spec PC-9, T071)");
  const std::string message = host->lastRefusalMessage();
  check(message.find(std::string(sg::test::kHuntCpu)) != std::string::npos
            && message.find(std::to_string(kWidePassEvents + 2))
                   != std::string::npos,
        "the seat failed on the scripted group refusal, naming the object and "
        "the leaf count it refused (PD-09)");
  check(needInstance("bmHuntPassZeroGroup")->huntRetriedPasses().empty(),
        "the refused pass-0 group opens no solo retry pass: the refusal is "
        "answered, not swallowed into the retry route (D-18, T071)");
  check(bodyAttempts == 0,
        "a run whose first seat refused samples nothing at all (spec PC-9)");
  std::printf("pass-zero-group-refusal-exits-one: ok\n");
}

// A hunt of a benchmark the registry has not expanded, and a hunt of a
// selection the catalog cannot resolve, each answer the error route with a
// message naming what they were given, and neither leaves a record behind. The
// first stands before `expandRegistry`, the state where the instance list
// holds nothing; every scenario of this file is a re-exec of its own, so that
// state belongs to this run alone (FR-092, FR-004, FR-005, E-01).
auto scenarioHuntRefusalsNameTheirInput() -> void
{
  registerHuntHost(kCollectLimit);
  auto handle = sg::registerBenchmark(&bmHuntBody, "bmHuntUnexpanded");

  const auto unexpanded =
      sg::hunt::runHunt(handle, optionsFor(selectionOf({0})), std::nullopt);
  check(!unexpanded.has_value(),
        "a hunt of a name the registry holds no instance for answers the error "
        "route (FR-092)");
  check(unexpanded.has_value()
            || unexpanded.error().message.find("bmHuntUnexpanded")
                != std::string::npos,
        "the refusal names the instance the registry holds none of (FR-092)");

  sg::detail::expandRegistry();
  const std::string missing {std::string(sg::test::kHuntCpu) + "/rose"};
  const auto unresolvable =
      sg::hunt::runHunt(handle, optionsFor(missing), std::nullopt);
  check(!unresolvable.has_value(),
        "a hunt of an address the catalog holds none of answers the error "
        "route (FR-004)");
  check(unresolvable.has_value()
            || unresolvable.error().message.find(missing) != std::string::npos,
        "the refusal names the address it could not resolve (FR-004)");

  const auto rawCode =
      sg::hunt::runHunt(handle, optionsFor("r4a03"), std::nullopt);
  check(!rawCode.has_value(),
        "a hunt of a raw event code answers the error route (FR-005)");
  check(rawCode.has_value()
            || rawCode.error().message.find("raw event code")
                != std::string::npos,
        "the refusal states the raw code it was given (FR-005)");
  std::printf("hunt-refusals-name-their-input: ok\n");
}

// The device that publishes no anchor pair answers a measured record on the
// iteration norm: the anchor-less columns admit no denominator, so no column
// is rated, and every column states the denominator it lacks in its own row
// (I-1, E-05, D-19).
auto scenarioUncoreColumnsRankByIterations() -> void
{
  registerHuntHost(kCollectLimit);
  const auto first = sg::test::huntAddress(
      sg::test::kHuntUncore, sg::test::huntEvent(sg::test::kHuntEvents));
  const auto last = sg::test::huntAddress(
      sg::test::kHuntUncore, sg::test::huntEvent(sg::test::kHuntEvents + 1));
  const auto options = optionsFor(first + "," + last);
  auto handle = sg::registerBenchmark(&bmHuntBody, "bmHuntUncore");
  sg::detail::expandRegistry();
  const HuntAccount run = runHuntMeasured(handle, options);
  check(run.hunt.has_value(),
        "the device that publishes no anchor still answers a measured "
        "record (I-1, D-19)");
  check(run.run->options.norm.has_value()
            && *run.run->options.norm == sg::hunt::HuntNorm::ITER,
        "a plan that found no anchors states the iteration norm it fell "
        "back to (D-13)");
  check(run.hunt->entries.size() == 2,
        "both uncore columns stand in the record (I-1)");
  const bool noRate =
      std::all_of(run.hunt->entries.begin(),
                  run.hunt->entries.end(),
                  [](const RankedEvent& entry)
                  { return entry.samples == 0 && !entry.rank.has_value(); });
  check(noRate,
        "a window of a column whose device lacks both anchors yields no "
        "rate: the no-leaf position admits no denominator (E-05)");
  check(run.hunt->skipped.size() == 2
            && std::all_of(run.hunt->skipped.begin(),
                           run.hunt->skipped.end(),
                           [](const SkippedEvent& skipped) {
                             return skipped.kind == SkippedKind::ABSENT;
                           }),
        "both uncore columns state the absent denominator, none a gap the "
        "rows never show (FR-021, D-10)");
  check(run.run->ranEvents == 0,
        "the summary rates no event of the anchor-less plan (E-05, FR-041)");
  check(run.run->method.has_value()
            && *run.run->method == sg::MetricMethod::MULTIPLEX,
        "the source that publishes no limit settles MULTIPLEX (D-19, "
        "spec PC-2)");
  std::printf("uncore-columns-by-iterations: ok\n");
}

// D-13, FR-081: the explicit norm whose anchor discovery refused is the one
// refusal the plan cannot see - the catalog holds the anchor, the discovery
// read refused it, and the run is the only reader of both facts.
auto scenarioRefusedAnchorRefusesTheNamedNorm() -> void
{
  registerHuntHost(kCollectLimit,
                   [](sg::counters::FakeProvider& provider)
                   {
                     refuseLeaf(provider,
                                sg::test::kHuntCycle,
                                sg::counters::Availability::PERMISSION_BLOCKED);
                   });
  const std::vector<std::string> names {sg::test::huntEvent(0),
                                        std::string(sg::test::kHuntCycle)};
  HuntOptions options = optionsFor(selectionOfNames(names));
  options.norm = sg::hunt::HuntNorm::CYCLE;
  auto handle = sg::registerBenchmark(&bmHuntBody, "bmHuntNormAnchor");
  sg::detail::expandRegistry();
  const auto answered =
      sg::hunt::runHuntNamed(handle.name(), options, std::nullopt);
  const std::string anchor =
      sg::test::huntAddress(sg::test::kHuntCpu, sg::test::kHuntCycle);
  const std::string event =
      sg::test::huntAddress(sg::test::kHuntCpu, sg::test::huntEvent(0));
  check(!answered.has_value()
            && answered.error().message == "the hunt norm cycle names the "
                                           "anchor '"
                + anchor + "', which discovery refused for the hunt of '" + event
                + "'",
        "the explicit norm whose anchor discovery refused ends the hunt, "
        "naming the norm, the anchor and the event that was to divide by it "
        "(D-13, FR-081)");
  check(host->readActions() == 0 && host->windowOpens() == 0,
        "the refused norm costs no window and no read: the refusal is "
        "settled from the discovery answer alone (FR-003, D-13)");
  std::printf("refused-anchor-refuses-norm: ok\n");
}

// ---------------------------------------------------------------------------
// FR-012, spec PC-9, T119: the driver that times one pass of its own plan
// ---------------------------------------------------------------------------

// The device whose limit cannot hold an event beside its two anchors refuses
// the plan itself, so the driver answers the planner's sentence whole.
auto scenarioLimitBelowAnchorsRefusesThePlan() -> void
{
  registerHuntHost(std::optional<std::size_t> {2});
  const auto options = optionsFor(selectionOf({0}));
  auto handle = sg::registerBenchmark(&bmHuntBody, "bmHuntTinyLimit");
  sg::detail::expandRegistry();
  const auto answered =
      sg::hunt::runHuntNamed(handle.name(), options, std::nullopt);
  check(!answered.has_value()
            && answered.error().message
                   == "hunt: device 'cpu' publishes a limit of 2, below the "
                      "3 leaves one event and its two anchors need",
        "the limit that cannot seat one event beside its anchors refuses the "
        "plan, and the driver hands the planner's sentence through whole "
        "(spec PC-2, FR-081)");
  std::printf("limit-below-anchors-refusal: ok\n");
}

// The timed route over the host that runs no pass: with no column to seat
// there is no pass to measure, so the hunt answers its record with no figure
// and the plan line prints the estimate as not available.
auto scenarioTimedPassWithoutAFigure() -> void
{
  registerHuntHost(
      kCollectLimit,
      [](sg::counters::FakeProvider& provider)
      {
        for (std::size_t index = 0; index < sg::test::kHuntEvents; ++index) {
          refuseLeaf(provider,
                     sg::test::huntEvent(index),
                     sg::counters::Availability::PERMISSION_BLOCKED);
        }
        refuseLeaf(provider,
                   sg::test::kHuntCycle,
                   sg::counters::Availability::PERMISSION_BLOCKED);
        refuseLeaf(provider,
                   sg::test::kHuntInstr,
                   sg::counters::Availability::PERMISSION_BLOCKED);
      });
  const auto options = optionsFor("all");
  auto handle = sg::registerBenchmark(&bmHuntBody, "bmHuntNoFigure");
  sg::detail::expandRegistry();
  const auto answered = sg::hunt::runHuntNamed(
      handle.name(), options, std::nullopt, std::nullopt, false, true);
  check(answered.has_value(),
        "the host that refuses every event still answers the pair on the "
        "timed route: there stands no pass to time (D-12, T119)");
  check(!answered->run.estimateNs.has_value(),
        "a hunt with no pass to measure holds no figure to state, so the "
        "estimate stays unstated (E-07, FR-012)");
  check(answered->entry.passCount == 0 && answered->run.passes == 0,
        "no pass stands for the plan with no event, timed route included "
        "(D-12)");
  const auto lines = printedLines(
      *needInstance("bmHuntNoFigure"), answered->entry, answered->run);
  check(linesHolding(lines, "hunt: plan events=0").size() == 1,
        "the timed route prints the same plan line for the empty plan "
        "(D-12, FR-041)");
  check(linesHolding(lines, "estimate=not available").size() == 1,
        "the plan line of a hunt that timed no pass prints the estimate as "
        "not available (FR-012, E-07)");
  std::printf("timed-pass-without-figure: ok\n");
}

// The timed route over the host whose monotonic leaf stands flat: the probe
// seats a pass, opens its window and folds its rows, yet the netted real
// time of the window is nothing at all - the row's own zero divisor of the
// runner's disclosure. A pass that folded no clock time holds no figure, so
// the estimate stands refused exactly as it does when no pass stood.
auto scenarioTimedPassFoldsNoClockTime() -> void
{
  registerHuntHost(kCollectLimit,
                   [](sg::counters::FakeProvider& provider)
                   {
                     provider.setPoints(sg::test::kPassClockObject,
                                        sg::test::kPassRealTimeLeaf,
                                        {0});
                   });
  const auto options = optionsFor(selectionOf({0, 1}));
  auto handle = sg::registerBenchmark(&bmHuntBody, "bmHuntFlatClock");
  sg::detail::expandRegistry();
  const auto answered = sg::hunt::runHuntNamed(
      handle.name(), options, std::nullopt, std::nullopt, false, true);
  check(answered.has_value(),
        "a window that folds no clock time stands no failure: the hunt "
        "answers its record (E-07)");
  check(answered->run.passes > 0,
        "the plan seats its passes, so the probe held a pass to time and "
        "the missing figure comes from the empty fold, not from an empty "
        "plan (FR-012, E-07)");
  check(!answered->run.estimateNs.has_value(),
        "a pass that folds no monotonic time holds no figure to state: the "
        "estimate of the hunt stands unstated (E-07, FR-012)");
  const auto lines = printedLines(
      *needInstance("bmHuntFlatClock"), answered->entry, answered->run);
  check(linesHolding(lines, "estimate=not available").size() == 1,
        "the plan line of a hunt whose probe folded no time prints the "
        "estimate as not available (FR-012, E-07)");
  std::printf("timed-pass-folds-no-time: ok\n");
}

// ---------------------------------------------------------------------------
// E-06, PD-14: the benchmark that skips through its loop collects nothing
// ---------------------------------------------------------------------------

// The body that skips closes its window at once, so every attempt of the
// hunt ends disclosed by the skip and the run carries no repetition row. The
// columns that produced no sample join neither the ranked rows nor the
// skipped list: the skipped list holds host answers alone.
auto bmHuntSkipBody(sg::State& state) -> void
{
  for (auto index : state) {
    state.skipWithMessage("scripted skip");
    sg::doNotOptimize(index);
  }
}

auto scenarioSkippedRunJoinsNoList() -> void
{
  registerHuntHost(kCollectLimit);
  const auto options = optionsFor(selectionOf({0, 1}));
  auto handle = sg::registerBenchmark(&bmHuntSkipBody, "bmHuntSkipped");
  sg::detail::expandRegistry();
  const HuntAccount run = runHuntMeasured(handle, options);
  check(run.hunt.has_value(),
        "the benchmark that skips is not a failed run: the hunt answers its "
        "record (E-06, PD-14)");
  const EntryHunt& hunt = *run.hunt;
  check(hunt.entries.empty(),
        "a column that produced no sample joins no ranked row (E-06, PD-14)");
  check(hunt.skipped.empty(),
        "a column the benchmark skipped through no fault of the host states "
        "no skipped kind (FR-021, I-4)");
  check(run.run->ranEvents == 0 && run.run->plannedAddresses.size() == 2,
        "the record planned both addresses and rated neither (E-08)");
  std::printf("skipped-run-joins-no-list: ok\n");
}

// ---------------------------------------------------------------------------
// FR-092: the two-context threaded run
// ---------------------------------------------------------------------------

// The CPUs this process may run on, read from the kernel itself (FR-092).
auto affinityCount() -> std::size_t
{
  cpu_set_t mask {};
  if (sched_getaffinity(0, sizeof(mask), &mask) != 0) {
    fail("sched_getaffinity could not read this process's CPU set");
  }
  return static_cast<std::size_t>(CPU_COUNT(&mask));
}

auto scenarioTwoContextWindowOpens() -> void
{
  registerHuntHost(kCollectLimit);
  const std::size_t cpus = affinityCount();
  check(cpus >= static_cast<std::size_t>(kThreadCount),
        "the two-context arm runs on a CPU set sched_getaffinity reports as "
        "two CPUs wide (FR-092)");

  const auto options = optionsFor(selectionOf({0, 1, 2, 3}));
  auto threaded = sg::registerBenchmark(&bmHuntBody, "bmHuntThreaded");
  threaded.threads(kThreadCount);
  auto single = sg::registerBenchmark(&bmHuntBody, "bmHuntSingle");
  sg::detail::expandRegistry();

  const HuntAccount threadedRun = runHuntMeasured(threaded, options);
  const HuntAccount singleRun = runHuntMeasured(single, options);
  check(threadedRun.hunt.has_value() && singleRun.hunt.has_value(),
        "both context shapes of the same plan answer a record (FR-092)");

  check(threadedRun.hunt->passCount == singleRun.hunt->passCount,
        "the threaded run plans the same passes as the single-context run");
  // Windows are provider objects, and the scripted provider counts each
  // open: the run seats one plan on the main thread, and each threaded
  // context compiles its own spine on its own thread once per threaded
  // attempt, the warm-up, the calibration, and the measured attempt
  // (FR-018). The single-context shape pays for the seat alone. No sleep
  // and no clock stands in this scenario.
  check(singleRun.opens == 1,
        "the single-context run opens the one window its seat compiles");
  check(threadedRun.opens
            == 1 + static_cast<std::uint64_t>(kThreadCount) * 3U,
        "the scripted provider counts the seat window beside one window "
        "per context for each of the three threaded attempts (FR-018, "
        "FR-092)");
  check(threadedRun.hunt->entries.size() == singleRun.hunt->entries.size(),
        "both context shapes report the same events (FR-013)");
  std::printf("two-context-window-opens: ok\n");
}

// ---------------------------------------------------------------------------
// PD-13, FR-013, SC-007: the two-stage budget run answers one merged record
// ---------------------------------------------------------------------------

auto scenarioTwoStageBudgetMergesOneRecord() -> void
{
  registerHuntHost(kCollectLimit);
  // Four events seat in one pass at the scripted limit, so the two stages cost
  // the four pass runs of stage 1 plus the three of stage 2: the budget of
  // eight passes fits both and no stage is cut (D-2, FR-013).
  HuntOptions options = optionsFor(selectionOf({0, 1, 2, 3}), 1);
  options.budget = HuntBudget {HuntBudget::Kind::PASSES, 8};
  const std::string again =
      sg::test::huntAddress(sg::test::kHuntCpu, sg::test::huntEvent(2));
  const std::string once =
      sg::test::huntAddress(sg::test::kHuntCpu, sg::test::huntEvent(3));

  auto handle = sg::registerBenchmark(&bmHuntBody, "bmHuntTwoStage");
  sg::detail::expandRegistry();
  const HuntAccount run = runHuntMeasured(handle, options);
  check(run.hunt.has_value(),
        "a two-stage budget run answers its record, so the run exits 0 "
        "(FR-013)");
  const EntryHunt& hunt = *run.hunt;

  check(hunt.entries.size() == kStageEvents,
        "one merged row set covers every planned column, the column stage 2 "
        "left out among them (PD-13, I-4)");
  const auto* onceRow = entryOf(hunt, once);
  check(onceRow != nullptr,
        "the address only stage 1 measured still stands in the record "
        "(PD-13, I-4)");
  check(onceRow != nullptr && onceRow->samples == 1 && !onceRow->repeated
            && !onceRow->rank.has_value(),
        "that row is built from its stage 1 samples alone, so it holds one "
        "sample and no rank (PD-14)");
  const auto* againRow = entryOf(hunt, again);
  check(againRow != nullptr && againRow->samples == options.repeats
            && againRow->repeated,
        "the column stage 2 re-measured carries its stage 2 samples, which "
        "win over its stage 1 ones (PD-13)");
  check(rankCount(hunt) == 1 && hunt.top == 1,
        "the merged row set ranks at most --hunt-top entries, however many "
        "stages filled it (I-5)");

  const RunHunt& record = *run.run;
  check(record.stages.size() == 2,
        "the record carries one range for each stage that ran (E-08, FR-013)");
  check(record.stages[0].first == 0 && record.stages[1].first == 0,
        "each stage range names the first planned address it measured (E-08)");
  check(record.stages[0].count == kStageEvents && record.stages[0].repeats == 1,
        "stage 1 covers every column at one repeat (E-04, FR-013)");
  check(record.stages[1].count == kStageTwoColumns
            && record.stages[1].repeats == options.repeats,
        "stage 2 covers the top three columns at the full repeats (D-2, "
        "FR-013)");
  check(record.plannedAddresses.size() == kStageEvents
            && record.ranEvents == kStageEvents
            && record.stageTwoEvents == kStageTwoColumns,
        "the record counts the planned columns, those that produced a sample "
        "and those stage 2 measured (E-08)");
  check(hunt.passCount == record.passes,
        "the one pass of the plan stands as the pass count of the run that "
        "measured it in both stages (E-06, E-08)");
  check(hunt.retriedPasses == 0,
        "the merge leaves the retried-pass lane alone: no group open refused "
        "in either stage (D-18)");
  check(hunt.skipped.empty(),
        "the merge invents no skipped entry beside the columns it merged "
        "(I-4)");

  const auto lines =
      printedLines(*needInstance("bmHuntTwoStage"), hunt, record);
  const auto rankLines = linesHolding(lines, "hunt: rank ");
  check(rankLines.size() == hunt.entries.size(),
        "the console prints one row set for the merged record, not one row "
        "set per stage (PD-13, PD-15)");
  const bool oncePerAddress = std::all_of(
      hunt.entries.begin(),
      hunt.entries.end(),
      [&rankLines](const RankedEvent& entry)
      { return linesHolding(rankLines, entry.address).size() == 1; });
  check(oncePerAddress,
        "no address of the plan prints twice across the merged row set "
        "(I-4, PD-13)");
  const auto tailLines = linesHolding(lines, "hunt: rank - " + once);
  check(tailLines.size() == 1
            && tailLines[0].find(" mark=unrepeated") != std::string::npos,
        "the stage-1-only address prints in the marked tail marked "
        "unrepeated (PD-14, D-20)");
  const std::string stageField =
      " budget=8p stage1=" + std::to_string(kStageEvents)
      + " stage2=" + std::to_string(kStageTwoColumns);
  check(linesHolding(lines, stageField).size() == 1,
        "the plan line names both stages with the events each measures "
        "(FR-012, FR-013, E-07)");
  check(linesHolding(lines, "hunt: plan budget cut").empty(),
        "a budget that fits both stages prints no cut line (FR-013, E-07)");
  check(linesHolding(lines, "hunt: skipped ").empty(),
        "a merged run that refused nothing prints no skipped line (PD-15)");
  const std::string summary = "hunt: plan summary events="
      + std::to_string(kStageEvents) + "/" + std::to_string(kStageEvents)
      + " passes=" + std::to_string(record.passes);
  check(linesHolding(lines, summary).size() == 1,
        "the summary counts the merged columns once, the stage-1-only column "
        "included (E-07, PD-15)");
  std::printf("two-stage-merged-record: ok\n");
}

// ---------------------------------------------------------------------------
// FR-012, FR-013: a seconds budget weighs the plan against the figure of a pass
// ---------------------------------------------------------------------------

// A budget is read in passes: the seconds of the budget divided by the
// nanoseconds of one pass (`allowedPasses`). One second divided by a pass of a
// microsecond is a million passes, and divided by a pass that costs a second is
// one pass, so the same plan fits the first and does not fit the second. The
// figure reaches that division from the caller of the library entry, which is
// why both arms stand here: they answer the same way on every host, while the
// command line reaches the same two shapes through a figure it times, whose
// digits belong to the host (T119, FR-012, FR-013, E-07).
auto scenarioSecondsBudgetWeighsThePassFigure() -> void
{
  registerHuntHost(kCollectLimit);
  // Four columns seat in one pass at the scripted limit; a budgeted plan runs
  // stage 1 once and stage 2 - the three columns of `--hunt-top=1` - at the
  // full repeats, so four passes of work stand against the one second.
  auto options = optionsFor(selectionOf({0, 1, 2, 3}), 1);
  options.budget = HuntBudget {HuntBudget::Kind::SECONDS, 1};

  auto handle = sg::registerBenchmark(&bmHuntBody, "bmHuntSecondsBudget");
  sg::detail::expandRegistry();

  const HuntAccount cheap =
      runHuntMeasured(handle, options, std::uint64_t {1'000});
  check(cheap.hunt.has_value(),
        "the hunt of a pass that costs a microsecond answers its record "
        "(FR-013)");
  const RunHunt& fit = *cheap.run;
  check(fit.estimateNs.has_value() && *fit.estimateNs == std::uint64_t {1'000},
        "the estimate is the figure of one pass weighed against the passes "
        "stage 1 runs, one pass at one repeat here (FR-012, E-08)");
  check(fit.stages.size() == 2,
        "a budget that a million passes fit leaves both stages standing "
        "(FR-013)");
  const auto fitLines =
      printedLines(*needInstance("bmHuntSecondsBudget"), *cheap.hunt, fit);
  check(linesHolding(fitLines,
                     "estimate=1000ns budget=1s stage1=4 stage2=3")
                .size()
            == 1,
        "the plan line of a fit plan states the figure the budget weighed and "
        "names both stages (FR-012, FR-013, E-07)");
  check(linesHolding(fitLines, "hunt: plan budget cut").empty(),
        "a plan the budget covers prints no cut line (FR-013, E-07)");

  const HuntAccount dear =
      runHuntMeasured(handle, options, std::uint64_t {1'000'000'000});
  check(dear.hunt.has_value(),
        "the hunt of a pass that costs a whole second answers its record "
        "(FR-013)");
  const RunHunt& cut = *dear.run;
  check(cut.stages.size() == 1 && cut.cutReason == "over-budget",
        "one second holds one pass, so stage 2 is dropped whole and names the "
        "reason the cut line prints (D-2, FR-013)");
  const auto cutLines =
      printedLines(*needInstance("bmHuntSecondsBudget"), *dear.hunt, cut);
  const auto cutPlan = linesHolding(cutLines, "estimate=1000000000ns");
  check(cutPlan.size() == 1
            && cutPlan.front().find("budget=1s") != std::string::npos
            && cutPlan.front().find(" stage2=") == std::string::npos,
        "the cut plan states the figure that decided it and names the one "
        "stage it ran (FR-012, FR-013, E-07)");
  check(!linesHolding(cutLines,
                      "hunt: plan budget cut stage=2 reason=over-budget")
              .empty(),
        "a budget the plan does not fit prints the cut line naming the stage "
        "it dropped (FR-013, E-07)");
  std::printf("seconds-budget-weighs-figure: ok\n");
}

// ---------------------------------------------------------------------------
// FR-012, T119: the estimate weighs the handed figure against every pass
// ---------------------------------------------------------------------------

// `hunt_plan.cpp` forms the estimate as the product of the figure the caller
// hands and the passes the plan seats, so a claim about that product needs a
// figure of its own. A live overhead floor is a reading of one window read, and
// an instrumented binary lifts such a reading by several orders of magnitude
// while still leaving it under a second, so the figure handed below stands
// beyond any floor a host can answer. The equality then fails on every host, in
// a normal build included, exactly when the estimate copies the floor in place
// of weighing the handed figure (the T119 defect). The plan carries no budget:
// `splitStages` reads the figure only to weigh a budget, so the handed figure
// moves no stage and no pass count and reaches the estimate whole.
auto scenarioHandedFigureWeighsEveryPass() -> void
{
  // A pass of a second. The widest floor a host has been seen to answer is
  // measured in nanoseconds of one window read, orders under this figure.
  constexpr std::uint64_t kHandedFigureNs {1'000'000'000};
  // The passes the scripted host seats eight events in: the limit is six
  // leaves counting the two anchors, so four events fill one pass and eight
  // need two (E-04).
  constexpr std::uint64_t kHandedFigurePasses {2};

  registerHuntHost(kCollectLimit);
  // One repeat, the stage count of an un-budgeted plan: stage 1 then runs the
  // plan at one repeat and the estimate is the figure weighed by the passes
  // alone (FR-012, E-08).
  const auto options =
      optionsFor(selectionCount(2 * kWidePassEvents), 20, 10.0, 1);
  auto handle = sg::registerBenchmark(&bmHuntBody, "bmHuntHandedFigure");
  sg::detail::expandRegistry();

  const HuntAccount account = runHuntMeasured(
      handle, options, std::optional<std::uint64_t> {kHandedFigureNs});
  check(account.hunt.has_value(),
        "the hunt of a handed figure answers its pair (FR-012)");
  const RunHunt& run = *account.run;
  check(run.passes == kHandedFigurePasses,
        "the eight scripted events seat in the two passes the scripted limit "
        "gives (E-04)");
  check(run.estimateNs.has_value()
            && *run.estimateNs == kHandedFigureNs * kHandedFigurePasses,
        "the record estimate is the handed figure weighed against both passes "
        "of the plan (T119, FR-012)");
  const auto lines =
      printedLines(*needInstance("bmHuntHandedFigure"), *account.hunt, run);
  const std::string product = "estimate="
      + std::to_string(kHandedFigureNs * kHandedFigurePasses) + "ns";
  check(linesHolding(lines, product).size() == 1,
        "the plan line prints that product whole, a figure no host can answer "
        "as the floor of its own run (T119, FR-012)");
  std::printf("estimate-figures-the-passes: ok\n");
}

// ---------------------------------------------------------------------------
// The scenario list CTest runs
// ---------------------------------------------------------------------------

struct Scenario
{
  std::string_view name;
  void (*body)();
};

// The scenarios of this file: what CTest runs with no argument, and what
// `argv[1]` selects from (T017, US1).
auto scenarioKindMappingAnswersEveryName() -> void
{
  using sg::counters::Availability;
  using sg::hunt::SkippedKind;
  // The five refused states answer their same-named kinds through the one
  // mapping of D-10, and the refusal of a seat names the state it maps.
  check(sg::detail::skippedKindOf(Availability::PERMISSION_BLOCKED)
            == SkippedKind::PERMISSION_BLOCKED,
        "permission blocked answers its own kind");
  check(sg::detail::skippedKindOf(Availability::NOT_ENCODABLE)
            == SkippedKind::NOT_ENCODABLE,
        "not encodable answers its own kind");
  check(sg::detail::skippedKindOf(Availability::ABSENT) == SkippedKind::ABSENT,
        "absent answers its own kind");
  check(sg::detail::skippedKindOf(Availability::SCOPE_REFUSED)
            == SkippedKind::SCOPE_REFUSED,
        "scope refused answers its own kind");
  check(sg::detail::skippedKindOf(Availability::GAP) == SkippedKind::GAP,
        "gap answers its own kind");
  // The refusal of a seat states the catalog state that refused it in the
  // sentence `compileCore` composes at `source/counters/plan.cpp:485-490`.
  // Each text below is that sentence with one state word in its place, so the
  // reader is pinned on the words the counters layer really emits (T118).
  const auto refusalOf = [](const std::string_view text)
  {
    return sg::counters::Error {
        .message = std::string(text),
        .suggestions = {},
    };
  };
  const auto stateOf = [&](const std::string_view word)
  {
    const std::string sentence =
        "counter 'cpu/hunt001' is not countable on this host: the catalog "
        "reports "
        + std::string(word)
        + "; pick a countable counter or branch on the catalog state before "
          "composing (FR-024)";
    return sg::detail::availabilityOfRefusal(refusalOf(sentence));
  };
  check(stateOf("permission_blocked") == Availability::PERMISSION_BLOCKED,
        "a refusal reporting permission_blocked maps to that state (D-18)");
  check(stateOf("not_encodable") == Availability::NOT_ENCODABLE,
        "a refusal reporting not_encodable maps to that state (D-18, "
        "spec PC-1)");
  check(stateOf("absent") == Availability::ABSENT,
        "a refusal reporting absent maps to that state (D-18)");
  check(stateOf("scope_refused") == Availability::SCOPE_REFUSED,
        "a refusal reporting scope_refused maps to that state (D-18)");
  check(stateOf("gap") == Availability::GAP,
        "a refusal reporting gap maps to that state (D-18)");
  check(!stateOf("countable").has_value(),
        "the countable state stands outside the kinds a skip carries, so a "
        "refusal reporting it answers no state (D-10)");
  // The reader fails closed: an errno name is a word of the host's own error
  // text and states no catalog state, so a text that carries one and no
  // sentence answers nothing (T118).
  const auto errnoWords = sg::detail::availabilityOfRefusal(
      refusalOf("window open failed: EINVAL"));
  check(!errnoWords.has_value(),
        "an errno name alone names no state (D-18, spec PC-1)");
  const auto groupRefusal = sg::detail::availabilityOfRefusal(
      refusalOf("refused package-1/core-9 carrying 8 leaves"));
  check(!groupRefusal.has_value(),
        "a refusal without the state sentence names no state (D-18)");
  std::printf("kind-mapping-cases: ok\n");
}

// ---------------------------------------------------------------------------
// FR-081, D-15, T126: the public library route answers the entry record
// ---------------------------------------------------------------------------

// The public `runHunt` over the scripted host: the entry record stands in
// the answer, carrying the rows and the plan figures of the hunt that ran.
// The run-level record answers the internal pair alone, so the library
// signature keeps the one record it states (PD-10, T126).
auto scenarioLibraryRouteAnswersTheEntryRecord() -> void
{
  registerHuntHost(kCollectLimit);
  const auto options = optionsFor(selectionOf({0, 1}));
  auto handle = sg::registerBenchmark(&bmHuntBody, "bmHuntLibraryRoute");
  sg::detail::expandRegistry();

  const auto answered =
      sg::hunt::runHunt(handle, options, std::optional<std::uint64_t> {});
  check(answered.has_value(),
        "the library route answers its entry record on the scripted host, "
        "the route that exits 0 (FR-081)");
  if (!answered.has_value()) {
    return;
  }
  check(answered->top == options.top && answered->entries.size() == 2,
        "the answered record carries one row per planned event, ranked to "
        "the stated top (E-06)");
  check(answered->passCount == 1,
        "the two scripted events seat in one pass of the scripted limit "
        "(E-04)");
  for (const auto& entry : answered->entries) {
    check(entry.samples > 0,
          "every answered row holds at least one clean sample (E-05)");
  }
  std::printf("library-route-entry-record: ok\n");
}

[[nodiscard]] auto scenarios() -> const std::vector<Scenario>&
{
  static const std::vector<Scenario> kList {
      Scenario {"noisy-out-of-top", &scenarioNoisyEventStandsOutOfTheTop},
      Scenario {"skipped-per-availability",
                &scenarioRefusedAvailabilityJoinsSkipped},
      Scenario {"gapped-window-both-lists",
                &scenarioGappedWindowStandsInBothLists},
      Scenario {"all-refused-host", &scenarioAllRefusedHost},
      Scenario {"unmeasurable-anchor",
                &scenarioUnmeasurableAnchorSkipsEveryEvent},
      Scenario {"multiplex-no-limit", &scenarioNoLimitHostMarksEveryEvent},
      Scenario {"group-leaf-retry", &scenarioWidePassRetriesOneAtATime},
      Scenario {"solo-refusal-maps-kind", &scenarioSoloRefusalMapsItsKind},
      Scenario {"anchorless-retry-one-leaf",
                &scenarioAnchorlessRetrySeatsItsEventAlone},
      Scenario {"kind-mapping-cases", &scenarioKindMappingAnswersEveryName},
      Scenario {"pass-zero-seat-failure",
                &scenarioPassZeroSeatFailureKeepsExitOne},
      Scenario {"pass-zero-group-refusal-exits-one",
                &scenarioPassZeroGroupRefusalExitsOne},
      Scenario {"hunt-refusals-name-their-input",
                &scenarioHuntRefusalsNameTheirInput},
      Scenario {"two-context-window-opens", &scenarioTwoContextWindowOpens},
      Scenario {"two-stage-merged-record",
                &scenarioTwoStageBudgetMergesOneRecord},
      Scenario {"seconds-budget-weighs-figure",
                &scenarioSecondsBudgetWeighsThePassFigure},
      Scenario {"estimate-figures-the-passes",
                &scenarioHandedFigureWeighsEveryPass},
      Scenario {"library-route-entry-record",
                &scenarioLibraryRouteAnswersTheEntryRecord},
      Scenario {"uncore-columns-by-iterations",
                &scenarioUncoreColumnsRankByIterations},
      Scenario {"refused-anchor-refuses-norm",
                &scenarioRefusedAnchorRefusesTheNamedNorm},
      Scenario {"limit-below-anchors-refusal",
                &scenarioLimitBelowAnchorsRefusesThePlan},
      Scenario {"timed-pass-without-figure", &scenarioTimedPassWithoutAFigure},
      Scenario {"timed-pass-folds-no-time", &scenarioTimedPassFoldsNoClockTime},
      Scenario {"skipped-run-joins-no-list", &scenarioSkippedRunJoinsNoList},
  };
  return kList;
}

}  // namespace

auto main(const int argc, char** argv) -> int
{
  const std::string self =
      argc > 0 && argv[0] != nullptr ? argv[0] : "harness_hunt_collect_test";
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
  // Each scenario scripts a provider of its own and the counters system takes
  // these object paths once per process, so the walk re-executes this binary
  // once per entry, the pattern of `harness_pass_collect_test.cpp`.
  for (const auto& scenario : list) {
    const std::string command =
        "\"" + self + "\" " + std::string(scenario.name);
    const int status = std::system(command.c_str());
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "the re-executed scenario passes");
  }
  std::printf("harness_hunt_collect_test: ok\n");
  return 0;
}
