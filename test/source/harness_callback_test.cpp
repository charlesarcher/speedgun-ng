// ============================================================================
// TDD test for the setup and teardown callbacks a handle accepts (T036; US5,
// capability C-7, FR-018, FR-019).
//
// One registration carries the pair through handle.setup and handle.teardown,
// and each callback increments a counter of its own, so the pair is readable
// as two counts, one per slot. The count each slot owes is the number
// of runs the report shows for that instance: the warm-up, calibration, and
// measured runs are runs too (FR-018), and the warm-up stage counts its runs
// from the sampling actions the scripted leaves hand out, because the report
// keeps only the measured row of a warm-up phase. The two slots are read
// apart, so the last attachment winning one slot (E-07, R-10) is a counter
// that stays at zero sitting next to one that moves.
//
// A scripted FakeProvider owns the machine time leaves, so no run grows into a
// long calibration and the reported time/iter of a fixed-N run is the scripted
// delta over N literally: a setup callback that burns a loop far past one pass
// of the body stays outside the window (FR-018). The state handed to a
// callback is the running instance's own, so range(index), rangeCount() and
// iterations() read there as they read in the body (FR-008, FR-018).
//
// The order of the fixture pair and the callback pair is the order the cited
// revision uses: the callback pair wraps each run of a plain registration, the
// fixture pair wraps the object's lifetime inside each run of a fixture
// registration, and an event log of one run prints that order instead of
// leaving it to be inferred from four counters.
//
// Hand-computed expectations, frameworkless check()/fail() convention.
// ============================================================================

#include <algorithm>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS CALLBACK TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// The scripted steps of every scenario. The measured window of one run owns
// two sampling actions, so its fold is kMonotonicStep whatever the run's body
// and the callback pair do (FR-005, FR-017 of the harness core).
constexpr std::uint64_t kMonotonicStep = 4000;
constexpr std::uint64_t kThreadCpuStep = 2000;

// Each callback of the untimed scenario burns this many dependent adds, far
// past one pass of the body's loop, so a window that reached the pair would
// report a per-iteration time orders of magnitude above the scripted delta.
constexpr std::uint64_t kCallbackBurn = 1'000'000;

// The provider of every scenario, kept by pointer so a scenario can read the
// sampling actions its runs consumed.
sg::counters::FakeProvider* fake = nullptr;

auto scriptedProvider() -> void
{
  // One provider per process: the counters system refuses registration once
  // the system is open (007 FR-009, source/counters/system.cpp:290-296), and
  // the scenarios read deltas of this one provider, so registering it once
  // keeps every count they assert.
  if (fake != nullptr) {
    return;
  }
  auto provider = std::make_unique<sg::counters::FakeProvider>();
  fake = provider.get();
  provider->addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  provider->addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  provider->setPoints("machine", "monotonic", {0}, kMonotonicStep);
  provider->setPoints("machine", "thread_cpu", {0}, kThreadCpuStep);
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(provider));
  check(registered.has_value(), "the fake provider registers");
}

auto burn() -> void
{
  std::uint64_t mark = 0;
  for (std::uint64_t index = 0; index < kCallbackBurn; ++index) {
    mark += index;
  }
  sg::doNotOptimize(mark);
}

auto captureRun(const std::vector<std::string>& arguments,
                int expected = 0) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_callback_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  const char* const path = "harness_callback_out.txt";
  const int saved = dup(STDOUT_FILENO);
  if (saved < 0 || std::freopen(path, "w", stdout) == nullptr) {
    fail("cannot redirect the report");
  }
  const int status =
      sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
  std::fflush(stdout);
  if (dup2(saved, STDOUT_FILENO) < 0) {
    fail("cannot restore stdout");
  }
  close(saved);
  if (status != expected) {
    std::fprintf(
        stderr, "speedgunMain returned %d, expected %d\n", status, expected);
    fail("the exit status does not follow contracts/cli.md (FR-036)");
  }

  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

auto linesOf(const std::string& report,
             const std::string& name) -> std::vector<std::string>
{
  std::vector<std::string> found;
  std::istringstream stream(report);
  std::string line;
  while (std::getline(stream, line)) {
    if (line.rfind(name, 0) == 0) {
      found.push_back(line);
    }
  }
  return found;
}

// The measured rows of one instance: a measured row carries the iteration
// field and an aggregate row does not, so their count is the number of runs
// the report shows (FR-012, FR-035). Every run count of this file is read
// here, never guessed.
auto runRowsOf(const std::string& report,
               const std::string& name) -> std::vector<std::string>
{
  std::vector<std::string> rows;
  for (const std::string& line : linesOf(report, name)) {
    if (line.contains("iterations=")) {
      rows.push_back(line);
    }
  }
  return rows;
}

auto doubleFieldOf(const std::string& line, const std::string& key) -> double
{
  const std::size_t at = line.find(key);
  if (at == std::string::npos) {
    std::fprintf(stderr, "no field '%s' in '%s'\n", key.c_str(), line.c_str());
    fail("the report carries no such field");
  }
  return std::strtod(line.c_str() + at + key.size(), nullptr);
}

auto closeTo(const double actual, const double expected) -> bool
{
  const double gap = actual > expected ? actual - expected : expected - actual;
  return gap <= 1e-3 + 1e-9 * (actual > 0 ? actual : expected);
}

// One fixed-N run costs the plan's sampling-cost calibration plus its two
// endpoint actions, so the difference reveals how many extra runs a warm-up
// phase added (the calibration test's probe).
auto probePlanActions(const std::string& instance) -> std::uint64_t
{
  const std::uint64_t before = fake->readActions();
  captureRun({"--filter", "^" + instance + "$", "--iterations=1"});
  const std::uint64_t consumed = fake->readActions() - before;
  check(consumed > 2, "the probe run carries the plan's calibration");
  return consumed - 2;
}

// The counters and readings each scenario compares. A callback belongs to no
// object of its own, so the readings live at file scope and not inside a
// registration (FR-018).
int pairSetups = 0;
int pairTeardowns = 0;
int pairBodies = 0;
int untimedSetups = 0;
std::vector<std::int64_t> argReads;
std::vector<std::uint64_t> rangeCountReads;
std::vector<std::uint64_t> iterationReads;
std::vector<std::uint64_t> emptyRangeCounts;
int firstSetups = 0;
int secondSetups = 0;
int firstTeardowns = 0;
int secondTeardowns = 0;
int orderSetups = 0;
int orderTeardowns = 0;
int endCalls = 0;
int fixtureSetUps = 0;
int fixtureTearDowns = 0;
std::vector<std::string> events;

auto bmPair(sg::State& state) -> void
{
  ++pairBodies;
  for (auto _ : state) {
  }
}

// The body of the untimed scenario: one pass of a short loop, so the burn the
// setup callback adds is orders of magnitude larger than the work inside the
// window.
auto bmUntimed(sg::State& state) -> void
{
  for (auto _ : state) {
    for (int index = 0; index < 64; ++index) {
      sg::doNotOptimize(index);
    }
  }
}

auto bmQuiet(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmOrder(sg::State& state) -> void
{
  events.push_back("call");
  for (auto _ : state) {
  }
}

// The fixture of the order scenario, kept in this file so the case needs no
// other translation unit (FR-015, FR-017).
class OrderFixture : public sg::Fixture
{
public:
  OrderFixture() { events.push_back("ctor"); }

  ~OrderFixture() override { events.push_back("dtor"); }

  auto setUp(sg::State& state) -> void override
  {
    (void)state;
    ++fixtureSetUps;
    events.push_back("setUp");
  }

  auto tearDown(sg::State& state) -> void override
  {
    (void)state;
    ++fixtureTearDowns;
    events.push_back("tearDown");
  }
};

SG_BENCHMARK_F(OrderFixture, run)

(sg::State& state)
{
  events.push_back("call");
  for (auto _ : state) {
  }
}

// Every registration this file needs, in one call before the first run
// starts: registerBenchmark owes its \pre to a run that has not started, so no
// scenario may register once another scenario has run one (FR-003 of the
// harness core).
auto registerAll() -> void
{
  // The three callback states of the R-05 scenario, one per forbidden call.
  // Each body is quiet; the violation lives in the setup callback, where the
  // state carries the instance arguments but opens no sampling window.
  auto beginGuard = sg::registerBenchmark(&bmQuiet, "bmCbGuardBegin");
  beginGuard.setup([](sg::State& state) { (void)state.begin(); });

  auto errorGuard = sg::registerBenchmark(&bmQuiet, "bmCbGuardSkipError");
  errorGuard.setup([](sg::State& state)
                   { state.skipWithError("a skip in a callback state"); });

  auto messageGuard = sg::registerBenchmark(&bmQuiet, "bmCbGuardSkipMessage");
  messageGuard.setup([](sg::State& state)
                     { state.skipWithMessage("a skip in a callback state"); });

  // The legal call of the R-05 rule: end() is static, touches no state,
  // and stays callable from a callback state.
  auto endLegal = sg::registerBenchmark(&bmQuiet, "bmCbEndLegal");
  endLegal.setup(
      [](sg::State& state)
      {
        (void)state.end();
        ++endCalls;
      });

  auto pair = sg::registerBenchmark(&bmPair, "bmCbPair");
  pair.setup([](sg::State&) { ++pairSetups; });
  pair.teardown([](sg::State&) { ++pairTeardowns; });

  auto untimed = sg::registerBenchmark(&bmUntimed, "bmCbUntimed");
  untimed.setup(
      [](sg::State&)
      {
        ++untimedSetups;
        burn();
      });

  // Two instances of one argument each, so range(0) of the running instance
  // is 11 in one run and 23 in the other.
  auto arguments = sg::registerBenchmark(&bmQuiet, "bmCbArgs");
  arguments.arg(11).arg(23);
  arguments.setup([](sg::State& state) { argReads.push_back(state.range(0)); });

  // One instance of two arguments, so the callback state owes rangeCount() of
  // two and iterations() of the fixed count the command line states.
  auto reads = sg::registerBenchmark(&bmQuiet, "bmCbState");
  reads.args({11, 23});
  reads.setup(
      [](sg::State& state)
      {
        rangeCountReads.push_back(state.rangeCount());
        iterationReads.push_back(state.iterations());
      });

  // No family call at all: the one instance carries zero arguments, and a
  // callback state reports that count without ever reading range(0).
  auto empty = sg::registerBenchmark(&bmQuiet, "bmCbNoArgs");
  empty.setup([](sg::State& state)
              { emptyRangeCounts.push_back(state.rangeCount()); });

  auto twice = sg::registerBenchmark(&bmQuiet, "bmCbTwice");
  twice.setup([](sg::State&) { ++firstSetups; });
  twice.setup([](sg::State&) { ++secondSetups; });
  twice.teardown([](sg::State&) { ++firstTeardowns; });
  twice.teardown([](sg::State&) { ++secondTeardowns; });

  auto order = sg::registerBenchmark(&bmOrder, "bmCbOrder");
  order.setup(
      [](sg::State&)
      {
        ++orderSetups;
        events.push_back("setup");
      });
  order.teardown(
      [](sg::State&)
      {
        ++orderTeardowns;
        events.push_back("teardown");
      });
}

// The event log of one run, printed against the sequence the case owes.
auto sameSequence(const std::vector<std::string>& got,
                  const std::vector<std::string>& want) -> bool
{
  if (got != want) {
    for (const std::string& event : want) {
      std::fprintf(stderr, "want %s\n", event.c_str());
    }
    for (const std::string& event : got) {
      std::fprintf(stderr, "got  %s\n", event.c_str());
    }
    return false;
  }
  return true;
}

// FR-018: handle.setup and handle.teardown each run once around each run. One
// fixed-count run prints one measured row, so the row count is the run count,
// and each slot has to move exactly that far.
auto pairPerRunScenario() -> void
{
  scriptedProvider();

  const int setupsBefore = pairSetups;
  const int teardownsBefore = pairTeardowns;
  const std::string report =
      captureRun({"--filter", "^bmCbPair$", "--iterations=1"});
  const int runs = static_cast<int>(runRowsOf(report, "bmCbPair").size());
  check(runs == 1,
        "a fixed iteration count prints one measured row (FR-012, " "FR-013)");
  check(pairBodies == runs, "the filtered run reached the body once");
  check(pairSetups - setupsBefore == runs,
        "the setup callback runs once per run (FR-018)");
  check(pairTeardowns - teardownsBefore == runs,
        "the teardown callback runs once per run (FR-018)");
}

// FR-018: the pair wraps the warm-up, calibration, and measured runs alike, so
// over three repetitions the two slots stay equal to each other and equal to
// the rows the report prints. The warm-up stage then counts its runs from the
// sampling actions the scripted leaves handed out, because the report keeps
// only the measured row of a warm-up phase.
auto repetitionsScenario() -> void
{
  scriptedProvider();

  const int setupsBefore = pairSetups;
  const int teardownsBefore = pairTeardowns;
  const std::string repeated = captureRun(
      {"--filter", "^bmCbPair$", "--iterations=1", "--repetitions=3"});
  const int repeatedRuns =
      static_cast<int>(runRowsOf(repeated, "bmCbPair").size());
  check(repeatedRuns == 3,
        "three repetitions print three measured rows (FR-012)");
  check(pairSetups - setupsBefore == repeatedRuns,
        "each repetition owns its own setup callback (FR-018)");
  check(pairTeardowns - teardownsBefore == repeatedRuns,
        "and its own teardown callback (FR-018)");
  check(pairSetups - setupsBefore == pairTeardowns - teardownsBefore,
        "the two slots stay a pair across repetitions (FR-018)");

  const std::uint64_t planActions = probePlanActions("bmCbPair");
  const int warmSetupsBefore = pairSetups;
  const int warmTeardownsBefore = pairTeardowns;
  const std::uint64_t actionsBefore = fake->readActions();
  captureRun(
      {"--filter", "^bmCbPair$", "--iterations=1", "--warmup-time=0.000001"});
  const int warmRuns =
      static_cast<int>((fake->readActions() - actionsBefore - planActions) / 2);
  check(warmRuns >= 2,
        "the warm-up phase ran before the measured run " "(FR-011)");
  check(pairSetups - warmSetupsBefore == warmRuns,
        "the warm-up run gets the pair too (FR-018)");
  check(pairTeardowns - warmTeardownsBefore == warmRuns,
        "and its own teardown callback (FR-018)");
}

// FR-018: the callbacks run in the untimed region. The scripted machine and
// monotonic leaves step by kMonotonicStep per sampling action, so the row of a
// four-iteration run reads exactly that step over four, and the setup callback
// of this instance burns a loop far past one pass of the body. Any part of the
// callback inside the window would raise the figure by orders of magnitude.
auto untimedScenario() -> void
{
  scriptedProvider();

  const std::string report =
      captureRun({"--filter", "^bmCbUntimed$", "--iterations=4"});
  const std::vector<std::string> rows = runRowsOf(report, "bmCbUntimed");
  check(rows.size() == 1, "the scripted run prints one measured row (FR-012)");
  check(untimedSetups == 1, "the burning setup callback ran once (FR-018)");
  check(closeTo(doubleFieldOf(rows[0], "time/iter (ns)="),
                static_cast<double>(kMonotonicStep) / 4.0),
        "the reported time is the scripted delta over N, so the callback's "
        "work adds nothing to it (FR-018)");
}

// FR-018: the state handed to a callback carries the instance arguments, so
// over the two instances of one family the values range(0) reports are the two
// arguments of this file and nothing else.
auto argumentReadScenario() -> void
{
  scriptedProvider();

  const std::string report =
      captureRun({"--filter", "bmCbArgs", "--iterations=1"});
  const int runs = static_cast<int>(runRowsOf(report, "bmCbArgs").size());
  check(runs == 2,
        "the two-instance family prints two measured rows (FR-010, " "FR-012)");
  check(argReads.size() == static_cast<std::size_t>(runs),
        "each run's setup callback read range(0) once (FR-018)");

  std::vector<std::int64_t> sorted = argReads;
  std::sort(sorted.begin(), sorted.end());
  check(sorted == std::vector<std::int64_t> {11, 23},
        "a setup callback reads the running instance's argument (FR-008, "
        "FR-018)");
}

// FR-018: the callback state carries rangeCount() and iterations() as a run
// state does. The two-argument instance owes a count of two and the fixed
// iteration count the command line states; the instance with no family call
// owes a count of zero, and never reads range(0), which its own argument span
// makes a precondition violation.
auto stateReadScenario() -> void
{
  scriptedProvider();

  const std::string report =
      captureRun({"--filter", "bmCbState", "--iterations=7"});
  check(runRowsOf(report, "bmCbState").size() == 1,
        "the two-argument instance prints one measured row (FR-012)");
  check(!rangeCountReads.empty() && !iterationReads.empty(),
        "the callback state reached both reads (FR-018)");
  for (const std::uint64_t count : rangeCountReads) {
    check(count == 2,
          "a callback state carries the instance's rangeCount() (FR-008, "
          "FR-018)");
  }
  for (const std::uint64_t iterations : iterationReads) {
    check(iterations == 7,
          "a callback state carries the run's iterations() (FR-018)");
  }

  const std::string plain =
      captureRun({"--filter", "^bmCbNoArgs$", "--iterations=1"});
  check(runRowsOf(plain, "bmCbNoArgs").size() == 1,
        "the instance with no family call prints one measured row (FR-010)");
  check(emptyRangeCounts.size() == 1, "its callback ran once (FR-018)");
  for (const std::uint64_t count : emptyRangeCounts) {
    check(count == 0,
          "an instance with no family call reports a zero rangeCount() in a "
          "callback state");
  }
}

// E-07, R-10: one callback per slot, and the last attachment wins it. The slot
// attached first has to stay at zero while the slot attached last moves with
// the runs, in both slots of the pair.
auto lastAttachmentScenario() -> void
{
  scriptedProvider();

  const std::string report =
      captureRun({"--filter", "^bmCbTwice$", "--iterations=1"});
  const int runs = static_cast<int>(runRowsOf(report, "bmCbTwice").size());
  check(runs == 1,
        "the twice-attached instance prints one measured row " "(FR-012)");
  check(firstSetups == 0, "the first setup attachment never runs (E-07, R-10)");
  check(secondSetups == runs,
        "the last setup attachment wins the slot (FR-018, R-10)");
  check(firstTeardowns == 0,
        "the first teardown attachment never runs (E-07, R-10)");
  check(secondTeardowns == runs,
        "the last teardown attachment wins that slot (FR-018, R-10)");
}

// FR-017, FR-018, R-09, R-10: the callback pair wraps each run of a plain
// registration, and the fixture pair sits inside the object's lifetime of each
// run of a fixture registration. The event log prints that order for one run;
// the four counters say both pairs moved once per run of their own instance.
auto orderScenario() -> void
{
  scriptedProvider();

  events.clear();
  const std::string plain = captureRun(
      {"--filter", "^bmCbOrder$", "--iterations=1", "--repetitions=2"});
  const int plainRuns = static_cast<int>(runRowsOf(plain, "bmCbOrder").size());
  check(plainRuns == 2, "two repetitions print two measured rows (FR-012)");
  check(sameSequence(
            events, {"setup", "call", "teardown", "setup", "call", "teardown"}),
        "the callback pair wraps each run in that order (R-10)");

  events.clear();
  const std::string fixture = captureRun(
      {"--filter", "^OrderFixture/run$", "--iterations=1", "--repetitions=2"});
  const int fixtureRuns =
      static_cast<int>(runRowsOf(fixture, "OrderFixture/run").size());
  check(fixtureRuns == 2,
        "the fixture instance prints one row per repetition " "(FR-012)");
  check(sameSequence(events, {"ctor", "setUp", "call", "tearDown", "dtor",
                              "ctor", "setUp", "call", "tearDown", "dtor"}),
        "the fixture pair sits inside the object's lifetime of each run "
        "(FR-017, R-09)");

  check(orderSetups == plainRuns && orderTeardowns == plainRuns,
        "the callback pair moved once per run of the plain instance (FR-018)");
  check(
      fixtureSetUps == fixtureRuns && fixtureTearDowns == fixtureRuns,
      "the fixture pair moved once per run of the fixture instance " "(FR-"
                                                                     "017)");
  check(orderSetups == fixtureSetUps && orderTeardowns == fixtureTearDowns
            && orderSetups == orderTeardowns,
        "the four counters agree: each pair runs once per run of its own "
        "instance (FR-017, FR-018)");
}

// FR-018, R-05: the callback state is the running instance's own state, so
// range(index), rangeCount() and iterations() read there, while begin(),
// skipWithError and skipWithMessage are precondition violations. A violation
// is observed the way dbc_test.cpp observes one: a fork of this process runs
// the guarded instance and the SG_REQUIRE response aborts the child, which the
// parent reads as SIGABRT.
auto guardAborts(const char* instance) -> bool
{
  pid_t pid = fork();
  if (pid < 0) {
    fail("cannot fork the guarded run");
  }
  if (pid == 0) {
    if (std::freopen("/dev/null", "w", stdout) == nullptr
        || std::freopen("/dev/null", "w", stderr) == nullptr)
    {
      _exit(2);
    }
    char* argv[] = {const_cast<char*>("harness_callback_test"),
                    const_cast<char*>("--filter"),
                    const_cast<char*>(instance),
                    const_cast<char*>("--iterations=1")};
    (void)sg::speedgunMain(4, argv);
    _exit(0);
  }

  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    fail("cannot wait for the guarded run");
  }
  return WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT;
}

auto guardScenario() -> void
{
  for (const char* instance :
       {"^bmCbGuardBegin$", "^bmCbGuardSkipError$", "^bmCbGuardSkipMessage$"})
  {
    check(guardAborts(instance),
          "the guarded call aborts in a callback state (R-05)");
  }
}

// FR-018: end() is static, touches no state, and stays legal in a
// callback state: the callback returns and the run completes.
auto endLegalScenario() -> void
{
  const std::string run =
      captureRun({"--filter", "^bmCbEndLegal$", "--iterations=1"});
  check(runRowsOf(run, "bmCbEndLegal").size() == 1,
        "the instance with an end() call in its setup callback runs "
        "(FR-018)");
  check(endCalls == 1,
        "end() in a callback state returns without aborting (R-05)");
}

}  // namespace

auto main() -> int
{
  scriptedProvider();
  registerAll();

  pairPerRunScenario();
  repetitionsScenario();
  untimedScenario();
  argumentReadScenario();
  stateReadScenario();
  lastAttachmentScenario();
  orderScenario();
  endLegalScenario();
  guardScenario();

  std::puts("harness_callback_test: ok");
  return 0;
}
