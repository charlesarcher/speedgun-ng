// ============================================================================
// TDD test for the fixture surface (T027; US4, capability C-6, FR-015,
// FR-016, FR-017, FR-020).
//
// Four fixture classes stand for the macro shapes of FR-016: one defined and
// registered by SG_BENCHMARK_F, one defined by SG_BENCHMARK_DEFINE_F and
// registered later by SG_BENCHMARK_REGISTER_F, one class template registered
// by SG_BENCHMARK_TEMPLATE_F, and one method named DISABLED_x. The instance
// name each shape owes is a literal of this file, hand-computed from the
// naming rule of the cited revision: `FixtureClass/Method`, and
// `BaseClass<types>/Method` for the template. The names are read back through
// list mode, which prints one name per line and runs no fixture method, and
// through the rows of one filtered run, which is how FR-011 selects a single
// instance.
//
// The counters QueueFixture keeps in setUp and tearDown are the reading of
// FR-017: each has to equal the number of runs the report shows, the warm-up
// run included, and the scripted machine/monotonic steps have to leave the
// reported time/iter at the scripted delta over N, so the work the pair burns
// outside the timed loop adds nothing to it. The constructor and destructor
// counters are the reading of R-09: one fixture object per run, destroyed
// before the next run builds its own.
//
// A scripted FakeProvider owns the machine time leaves, so no run grows into
// a long calibration and every reported time is a hand-computed figure. The
// run count of the warm-up scenario comes from the provider's sampling
// actions, because the report keeps only the measured row. Frameworkless
// check()/fail() convention.
// ============================================================================

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS FIXTURE TEST FAIL: %s\n", what);
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
// and the fixture pair do (FR-017, FR-018 of the harness core).
constexpr std::uint64_t kMonotonicStep = 4000;
constexpr std::uint64_t kThreadCpuStep = 2000;

// Each setUp and tearDown of QueueFixture burns this many dependent adds, far
// past one pass of the body's loop, so a window that reached the pair would
// report a per-iteration time orders of magnitude above the scripted delta.
constexpr std::uint64_t kPairBurn = 1'000'000;

// A fixture object belongs to one run, so the readings the scenarios compare
// live at file scope and not inside the object (FR-017, R-09).
int queueSetUps = 0;
int queueTearDowns = 0;
int queueConstructs = 0;
int queueDestructs = 0;
int queuePushRuns = 0;
int otherRuns = 0;
int typedSetUps = 0;
int typedTearDowns = 0;
int typedRuns = 0;
int disabledRuns = 0;

// The provider of every scenario, kept by pointer so a scenario can read the
// sampling actions its runs consumed.
sg::counters::FakeProvider* fake = nullptr;

auto scriptedProvider() -> void
{
  // The catalog freezes at the first open and the first in-process
  // speedgunMain opens it, so a later registration is refused by the
  // counters contract (007 FR-009, source/counters/system.cpp:293).
  // The scenarios read deltas of this one provider, so registering it
  // once per process keeps every count they assert.
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

// The fixture of the pair, timing, and object-count scenarios: the pair
// counts itself and burns work outside the timed loop, and the constructor
// and destructor carry the object count (FR-015, FR-017, R-09).
class QueueFixture : public sg::Fixture
{
public:
  QueueFixture() { ++queueConstructs; }

  ~QueueFixture() override { ++queueDestructs; }

  auto setUp(sg::State& state) -> void override
  {
    (void)state;
    ++queueSetUps;
    burn();
  }

  auto tearDown(sg::State& state) -> void override
  {
    (void)state;
    ++queueTearDowns;
    burn();
  }

private:
  static auto burn() -> void
  {
    std::uint64_t mark = 0;
    for (std::uint64_t index = 0; index < kPairBurn; ++index) {
      mark += index;
    }
    sg::doNotOptimize(mark);
  }
};

// A fixture with no pair of its own, whose method is defined by one macro and
// registered later by another (FR-016).
class OtherFixture : public sg::Fixture
{
};

// The fixture class template that SG_BENCHMARK_TEMPLATE_F instantiates over
// one type argument (FR-016).
template<typename Type>
class TypedFixture : public sg::Fixture
{
public:
  auto setUp(sg::State& state) -> void override
  {
    (void)state;
    ++typedSetUps;
  }

  auto tearDown(sg::State& state) -> void override
  {
    (void)state;
    ++typedTearDowns;
  }
};

// The fixture whose method carries the DISABLED_ prefix, which the prefix
// test of R-11 never reaches because the instance name starts with the class
// name (FR-020).
class DisabledFixture : public sg::Fixture
{
};

// The macro sites sit in the same scope as their fixture classes, as the
// generated derived class names the class it derives from.

SG_BENCHMARK_F(QueueFixture, push)

(sg::State& state)
{
  ++queuePushRuns;
  for (auto _ : state) {
    for (int index = 0; index < 64; ++index) {
      sg::doNotOptimize(index);
    }
  }
}

SG_BENCHMARK_DEFINE_F(OtherFixture, one)

(sg::State& state)
{
  ++otherRuns;
  for (auto _ : state) {
  }
}

SG_BENCHMARK_REGISTER_F(OtherFixture, one);

SG_BENCHMARK_TEMPLATE_F(TypedFixture, run, int)

(sg::State& state)
{
  ++typedRuns;
  for (auto _ : state) {
  }
}

SG_BENCHMARK_F(DisabledFixture, DISABLED_x)

(sg::State& state)
{
  ++disabledRuns;
  for (auto _ : state) {
  }
}

auto readFile(const char* path) -> std::string
{
  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

auto captureRun(const std::vector<std::string>& arguments,
                int expected = 0) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_fixture_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  const char* const path = "harness_fixture_out.txt";
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

  return readFile(path);
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

auto lineOf(const std::string& report, const std::string& name) -> std::string
{
  std::istringstream stream(report);
  std::string line;
  while (std::getline(stream, line)) {
    if (line.rfind(name, 0) == 0) {
      return line;
    }
  }
  return {};
}

// The measured rows of one instance: a measured row carries the iteration
// field and an aggregate row does not, so their count is the number of runs
// the report shows (FR-012, FR-035).
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
  return std::abs(actual - expected) <= 1e-3 + 1e-9 * std::abs(expected);
}

// One fixed-N run costs the plan's sampling-cost calibration plus its two
// endpoint actions, so the difference reveals the calibration length the warm-
// up scenario has to reach past (the calibration test's probe).
auto probePlanActions() -> std::uint64_t
{
  const std::uint64_t before = fake->readActions();
  captureRun({"--filter", "^QueueFixture/push$", "--iterations=1"});
  const std::uint64_t consumed = fake->readActions() - before;
  check(consumed > 2, "the probe run carries the plan's calibration");
  return consumed - 2;
}

// FR-016: SG_BENCHMARK_F defines the method and registers it in one shape,
// and the instance name is the fixture class, a '/', and the method. List
// mode is the reading, and it has to leave every fixture body at its
// starting count.
auto instanceNameScenario() -> void
{
  const std::string listed = captureRun({"--list"});
  check(lineOf(listed, "QueueFixture/push") == "QueueFixture/push",
        "SG_BENCHMARK_F registers the instance named QueueFixture/push "
        "(FR-016)");
  check(linesOf(listed, "QueueFixture/push").size() == 1,
        "one SG_BENCHMARK_F registration prints that name once (FR-016)");
  check(queuePushRuns == 0 && otherRuns == 0 && typedRuns == 0
            && disabledRuns == 0,
        "list mode runs no fixture method (FR-011)");
}

// FR-017: the pair runs once around each run. The run count is read from the
// report and never guessed, because a warm-up or calibration run is a run too
// and owns its own pair. The warm-up stage then counts its runs from the
// sampling actions the scripted leaves handed out, since the report keeps
// only the measured row of a warm-up phase.
auto pairPerRunScenario() -> void
{
  scriptedProvider();

  const int singleSetUpBefore = queueSetUps;
  const int singleTearDownBefore = queueTearDowns;
  const std::string single =
      captureRun({"--filter", "^QueueFixture/push$", "--iterations=1"});
  const int singleRuns =
      static_cast<int>(runRowsOf(single, "QueueFixture/push").size());
  check(singleRuns == 1,
        "a fixed iteration count prints one measured row " "(FR-012, FR-013)");
  check(queueSetUps - singleSetUpBefore == singleRuns,
        "setUp runs once per run (FR-017)");
  check(queueTearDowns - singleTearDownBefore == singleRuns,
        "tearDown runs once per run (FR-017)");

  const int repeatedSetUpBefore = queueSetUps;
  const int repeatedTearDownBefore = queueTearDowns;
  const std::string repeated = captureRun(
      {"--filter", "^QueueFixture/push$", "--iterations=1", "--repetitions=3"});
  const int repeatedRuns =
      static_cast<int>(runRowsOf(repeated, "QueueFixture/push").size());
  check(repeatedRuns == 3,
        "three repetitions print three measured rows (FR-012)");
  check(queueSetUps - repeatedSetUpBefore == repeatedRuns,
        "each repetition owns its own setUp (FR-017)");
  check(queueTearDowns - repeatedTearDownBefore == repeatedRuns,
        "each repetition owns its own tearDown (FR-017)");

  const std::uint64_t planActions = probePlanActions();
  const int warmSetUpBefore = queueSetUps;
  const int warmTearDownBefore = queueTearDowns;
  const std::uint64_t actionsBefore = fake->readActions();
  captureRun({"--filter",
              "^QueueFixture/push$",
              "--iterations=1",
              "--warmup-time=0.000001"});
  const int warmRuns =
      static_cast<int>((fake->readActions() - actionsBefore - planActions) / 2);
  check(warmRuns >= 2,
        "the warm-up phase ran before the measured run (FR-011)");
  check(queueSetUps - warmSetUpBefore == warmRuns,
        "the warm-up run gets the pair too (FR-017, Q-9)");
  check(queueTearDowns - warmTearDownBefore == warmRuns,
        "and its own tearDown (FR-017, Q-9)");
}

// FR-017: the pair runs in the untimed region. Its burn is far longer than one
// pass of the body's loop, and the scripted machine/monotonic leaf steps by
// kMonotonicStep per sampling action, so the row of a four-iteration run reads
// exactly that step over four. Any part of the pair inside the window would
// raise the figure by orders of magnitude.
auto untimedScenario() -> void
{
  scriptedProvider();
  const std::string report =
      captureRun({"--filter", "^QueueFixture/push$", "--iterations=4"});
  const std::vector<std::string> rows = runRowsOf(report, "QueueFixture/push");
  check(rows.size() == 1, "the scripted run prints one measured row (FR-012)");
  check(closeTo(doubleFieldOf(rows[0], "time/iter (ns)="),
                static_cast<double>(kMonotonicStep) / 4.0),
        "the reported time is the scripted delta over N, so the pair's work "
        "adds nothing to it (FR-017)");
}

// R-09: the factory builds one fixture object inside the run and destroys it
// after that run's tearDown, so over N runs the two counters are equal and
// each has reached N.
auto oneObjectPerRunScenario() -> void
{
  scriptedProvider();
  const int constructedBefore = queueConstructs;
  const int destructedBefore = queueDestructs;
  const std::string report = captureRun(
      {"--filter", "^QueueFixture/push$", "--iterations=1", "--repetitions=3"});
  const int runs =
      static_cast<int>(runRowsOf(report, "QueueFixture/push").size());
  check(runs == 3, "three repetitions print three measured rows (FR-012)");
  const int built = queueConstructs - constructedBefore;
  const int released = queueDestructs - destructedBefore;
  check(built >= runs, "each run builds its own fixture object (R-09)");
  check(built == released,
        "and destroys it before the next run builds one (R-09)");
}

// FR-016: SG_BENCHMARK_DEFINE_F defines the method on its own and
// SG_BENCHMARK_REGISTER_F registers that definition later, under the one name
// OtherFixture/one. The list line, the row of a filtered run, and the count
// the body keeps are the three readings of that later registration.
auto laterRegistrationScenario() -> void
{
  scriptedProvider();
  const std::string listed = captureRun({"--list"});
  check(lineOf(listed, "OtherFixture/one") == "OtherFixture/one",
        "DEFINE_F plus REGISTER_F registers the instance named "
        "OtherFixture/one (FR-016)");

  const int before = otherRuns;
  const std::string report =
      captureRun({"--filter", "^OtherFixture/one$", "--iterations=2"});
  check(runRowsOf(report, "OtherFixture/one").size() == 1,
        "the later registration runs and prints one row (FR-016)");
  check(otherRuns - before == 1,
        "the defined method reaches its body " "(FR-016)");
}

// FR-016: SG_BENCHMARK_TEMPLATE_F instantiates the fixture class template
// over the type list, so the instance name carries the type arguments as
// written, inside angle brackets, between the class and the method.
auto templateScenario() -> void
{
  scriptedProvider();
  const std::string listed = captureRun({"--list"});
  check(lineOf(listed, "TypedFixture<int>/run") == "TypedFixture<int>/run",
        "SG_BENCHMARK_TEMPLATE_F registers the instance named "
        "TypedFixture<int>/run (FR-016)");

  const int before = typedRuns;
  const std::string report =
      captureRun({"--filter", "^TypedFixture<int>/run$", "--iterations=1"});
  check(runRowsOf(report, "TypedFixture<int>/run").size() == 1,
        "the template fixture instance runs and prints one row (FR-016)");
  check(typedRuns - before == 1, "and its method reaches its body (FR-016)");
  check(typedSetUps == typedTearDowns,
        "the template fixture pair runs as a pair (FR-017)");
}

// FR-020, R-11: the prefix test runs on the full instance name, and a fixture
// instance name starts with its fixture class, so a method named DISABLED_x is
// no disabled instance. It is listed, a filter reaches it, and it runs.
auto disabledMethodScenario() -> void
{
  scriptedProvider();
  const std::string listed = captureRun({"--list"});
  check(lineOf(listed, "DisabledFixture/DISABLED_x")
            == "DisabledFixture/DISABLED_x",
        "a fixture method named DISABLED_x is listed (FR-020, R-11)");

  const int before = disabledRuns;
  const std::string report = captureRun(
      {"--filter", "^DisabledFixture/DISABLED_x$", "--iterations=1"});
  check(runRowsOf(report, "DisabledFixture/DISABLED_x").size() == 1,
        "a fixture method named DISABLED_x runs (FR-020, R-11)");
  check(disabledRuns - before == 1,
        "and its body reaches the run (FR-020, R-11)");
}

}  // namespace

auto main() -> int
{
  scriptedProvider();

  instanceNameScenario();
  pairPerRunScenario();
  untimedScenario();
  oneObjectPerRunScenario();
  laterRegistrationScenario();
  templateScenario();
  disabledMethodScenario();

  check(queueConstructs == queueDestructs,
        "no fixture object outlives the run that built it (R-09)");

  std::puts("harness_fixture_test: ok");
  return 0;
}
