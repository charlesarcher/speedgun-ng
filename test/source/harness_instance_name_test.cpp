// ============================================================================
// TDD test for the instance names a family expands to (T017; US2,
// capability C-2).
//
// One benchmark body stands for every family, and the instance name each
// family shape owes is a literal of this file, hand-computed from the
// naming rule of FR-010: the family name plus one '/'-joined segment per
// argument, that segment labeled `label:value` where argName set one, and
// the family name alone where no family call states a family. The names
// are read back through list mode, which prints one name per line and runs
// no benchmark function, and through the rows of one filtered run, which
// is how FR-011 selects a single instance; the catalog path prints none
// of them. The FR-012 duplicate is read from the standard error of a
// re-exec of this binary, the way harness_family_test.cpp reads the FR-007
// warning: the two streams go to two files. A scripted FakeProvider owns
// the machine time leaves, so no run grows into a long calibration.
// T028 (US4, capability C-9) adds the suite and case pair of FR-021: the
// three derivation rows of contracts/result-fields.md asserted literally,
// the pair read through the two fields of the result value at the
// internal seam (detail/internal.hpp, the way harness_gap_test.cpp
// reaches it), the uniqueness of the pair across the instance list, the
// stable suite grouping of R-12 asserted as the exact sequence of
// instance names the walk produces, and the console row keeping the
// single H1 name column.
// Hand-computed expectations, frameworkless check()/fail() convention.
// ============================================================================

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

#include "detail/internal.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS INSTANCE NAME TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// The one body every family runs. Its count is the number of instances a
// selection reached, and list mode has to leave it at zero.
int instanceRuns = 0;

auto bmInstance(sg::State& state) -> void
{
  ++instanceRuns;
  for (auto _ : state) {
  }
}

// The body of the later registration of a clashing pair. FR-012 owes that
// the instance never runs, and only a body of its own can say so: it marks
// the report with its name, and the mark staying absent is the reading.
auto bmDropped(sg::State& state) -> void
{
  std::fputs("bmDupLaterRan\n", stdout);
  for (auto _ : state) {
  }
}

// The fixture side of the FR-002 double registration: a fixture
// registration whose name is already taken meets the same refusal the plain
// registration meets. The fixture carries no state, the name is the point.
class ClashFixture : public sg::Fixture
{
};

auto bmClash(sg::State& state) -> void
{
  std::fputs("clashBodyRan\n", stdout);
  for (auto _ : state) {
  }
}

auto captureRun(const std::vector<std::string>& arguments,
                int expected = 0) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_instance_name_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  const char* const path = "harness_instance_name_out.txt";
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

auto readFile(const char* path) -> std::string
{
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

// The instance names of one family, in the order list mode printed them:
// the family name alone, or the family name and its '/'-joined segments
// (FR-010).
auto namesUnder(const std::string& report,
                const std::string& family) -> std::vector<std::string>
{
  std::vector<std::string> found;
  for (const std::string& line : linesOf(report, family)) {
    if (line.size() == family.size() || line[family.size()] == '/') {
      found.push_back(line);
    }
  }
  return found;
}

auto nonEmptyLineCount(const std::string& text) -> int
{
  int lines = 0;
  std::istringstream stream(text);
  for (std::string line; std::getline(stream, line);) {
    if (!line.empty()) {
      ++lines;
    }
  }
  return lines;
}

auto scriptedProvider() -> void
{
  auto provider = std::make_unique<sg::counters::FakeProvider>();
  provider->addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  provider->addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  provider->setPoints("machine", "monotonic", {0}, 4000);
  provider->setPoints("machine", "thread_cpu", {0}, 2000);
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(provider));
  check(registered.has_value(), "the fake provider registers");
}

struct NameCase
{
  const char* family;
  void (*build)(sg::BenchmarkHandle& handle);
  std::vector<std::string> instances;
};

// The instance names of every family shape, hand-computed from FR-010:
// one argument, two arguments, an argName label on one position, and no
// family call at all.
const std::vector<NameCase> kNameCases = {
    // arg: one argument, so one segment.
    NameCase {"bmBits",
              [](sg::BenchmarkHandle& handle) { handle.arg(8); },
              {"bmBits/8"}},
    // args over two lists of arity two: two '/'-joined segments.
    NameCase {"bmQueue",
              [](sg::BenchmarkHandle& handle)
              { handle.args({8, 64}).args({1, 1024}); },
              {"bmQueue/8/64", "bmQueue/1/1024"}},
    // argName labels the segment of one argument position; the family
    // arity is one, so one label is legal (FR-006).
    NameCase {"bmLabelled",
              [](sg::BenchmarkHandle& handle)
              { handle.argName("size").arg(8).arg(64); },
              {"bmLabelled/size:8", "bmLabelled/size:64"}},
    // No family call: one instance, named by the family name alone.
    NameCase {"bmPlain", [](sg::BenchmarkHandle&) {}, {"bmPlain"}},
};

// FR-010: the name of every family shape, read back from list mode as the
// literal this file states.
auto nameFormScenario() -> void
{
  for (const NameCase& nameCase : kNameCases) {
    auto handle = sg::registerBenchmark(&bmInstance, nameCase.family);
    nameCase.build(handle);
  }

  const std::string listed = captureRun({"--list"});
  check(instanceRuns == 0, "list mode runs no benchmark function (FR-011)");
  for (const NameCase& nameCase : kNameCases) {
    const std::vector<std::string> printed =
        namesUnder(listed, nameCase.family);
    if (printed != nameCase.instances) {
      for (const std::string& name : nameCase.instances) {
        std::fprintf(stderr, "expected %s\n", name.c_str());
      }
      for (const std::string& name : printed) {
        std::fprintf(stderr, "listed   %s\n", name.c_str());
      }
      fail("an instance name follows the form of FR-010");
    }
  }
  check(lineOf(listed, "bmPlain") == "bmPlain",
        "a family with no family call owes the family name alone, with no "
        "'/' (FR-010)");
}

// FR-011: the filter matches instance names, so one full instance name
// selects exactly one instance. The rows of the report and the count the
// body keeps are the two readings of that selection.
auto filterScenario() -> void
{
  const int before = instanceRuns;
  const std::string run =
      captureRun({"--filter", "^bmQueue/8/64$", "--iterations=1"});
  check(linesOf(run, "bmQueue/8/64").size() == 1,
        "one full instance name reports one row (FR-011)");
  check(instanceRuns - before == 1,
        "one full instance name runs one instance (FR-011)");
  for (const char* const other :
       {"bmBits", "bmLabelled", "bmPlain", "bmQueue/1/1024"})
  {
    check(linesOf(run, other).empty(),
          "one full instance name runs no other instance (FR-011)");
  }
}

// FR-011: list mode prints one instance name per line, and the catalog
// option returns before the selection and prints none of them.
auto listAndCatalogScenario() -> void
{
  const int before = instanceRuns;
  const std::string listed = captureRun({"--list"});
  int names = 0;
  for (const NameCase& nameCase : kNameCases) {
    names += static_cast<int>(nameCase.instances.size());
  }
  check(nonEmptyLineCount(listed) == names,
        "list mode prints one instance name per line (FR-011)");
  check(listed.find("time/iter") == std::string::npos,
        "list mode carries no measured row (FR-011)");

  const std::string catalog = captureRun({"--catalog"});
  for (const NameCase& nameCase : kNameCases) {
    for (const std::string& name : nameCase.instances) {
      check(catalog.find(name) == std::string::npos,
            "the --catalog path prints no instance name (FR-011)");
    }
  }
  check(instanceRuns == before,
        "neither list mode nor the catalog path runs a benchmark function "
        "(FR-011)");
}

// FR-012: the clash of one instance name between two registrations, read
// from the standard error of a re-exec of this binary. bmDup with the
// argument 8 and the family bmDup/8 with no family call expand to the one
// name bmDup/8, so the line that names both names names that string.
auto duplicateScenario(const std::string& self) -> void
{
  const int status = std::system(("\"" + self + "\" duplicate"
                                  + " >harness_instance_name_run.txt"
                                  + " 2>harness_instance_name_err.txt")
                                     .c_str());
  check(status == 0,
        "a duplicate instance name leaves the exit status as a normal run "
        "carries it (FR-012)");

  const std::string error = readFile("harness_instance_name_err.txt");
  check(nonEmptyLineCount(error) == 1,
        "a duplicate instance name draws one line on the standard error "
        "stream (FR-012)");
  check(error.find("bmDup/8") != std::string::npos,
        "that line names the clashing instance name (FR-012)");

  const std::string run = readFile("harness_instance_name_run.txt");
  check(linesOf(run, "bmDup/8").size() == 1,
        "the earlier instance stays and the later is dropped (FR-012)");
  check(run.find("bmDupLaterRan") == std::string::npos,
        "the later instance never reaches its function (FR-012)");
  check(linesOf(run, "bmOther/").size() == 1,
        "every other instance still runs (FR-012)");
}

// FR-012 read from the other side of the pair: the plain registration
// claims the name first, so the family expansion is the walk that finds the
// clash and drops its own instance.
auto familyClashScenario(const std::string& self) -> void
{
  const int status = std::system(("\"" + self + "\" family-clash"
                                  + " >harness_instance_name_run.txt"
                                  + " 2>harness_instance_name_err.txt")
                                     .c_str());
  check(status == 0,
        "a family instance clashing with an earlier name leaves the exit "
        "status as a normal run carries it (FR-012)");

  const std::string error = readFile("harness_instance_name_err.txt");
  check(nonEmptyLineCount(error) == 1,
        "that clash draws one line on the standard error stream (FR-012)");
  check(error.find("bmFam/9") != std::string::npos,
        "that line names the clashing instance name (FR-012)");

  const std::string run = readFile("harness_instance_name_run.txt");
  check(linesOf(run, "bmFam/9").size() == 1,
        "the earlier instance stays and the family instance is dropped "
        "(FR-012)");
  check(run.find("bmDupLaterRan") != std::string::npos,
        "the earlier plain registration is the one that runs (FR-012)");
}

// FR-002 at the fixture registration: the name belongs to a plain
// registration, so the fixture registration prints the refusal and hands
// back a handle that runs nothing.
auto fixtureClashScenario(const std::string& self) -> void
{
  const int status = std::system(("\"" + self + "\" fixture-clash"
                                  + " >harness_instance_name_run.txt"
                                  + " 2>harness_instance_name_err.txt")
                                     .c_str());
  check(status == 0,
        "a fixture registration of a taken name leaves the exit status as a "
        "normal run carries it (FR-002)");

  const std::string error = readFile("harness_instance_name_err.txt");
  check(nonEmptyLineCount(error) == 1,
        "the double registration draws one line on the standard error stream "
        "(FR-002)");
  check(error.find("registered twice") != std::string::npos,
        "that line says the benchmark is registered twice (FR-002)");

  const std::string run = readFile("harness_instance_name_run.txt");
  check(linesOf(run, "bmClash").size() == 1,
        "the first registration keeps its row and the fixture registration "
        "runs nothing (FR-002)");
  check(linesOf(run, "bmOther").size() == 1,
        "the rest of the suite still runs (FR-002)");
}

// One derivation row of contracts/result-fields.md, asserted literally:
// the family name registered, the instance name it expands to, and the
// suite and case the pair owes. The contract's QueueFixture row is a
// fixture instance (Q-1: the suite is the fixture class name); the
// fixture macros do not exist yet, so the same instance name is reached
// by registering the family QueueFixture/push, whose derivation of the
// pair is identical.
struct SuiteCaseRow
{
  const char* family;
  const char* instance;
  const char* suite;
  const char* caseName;
  void (*build)(sg::BenchmarkHandle& handle);
};

const std::vector<SuiteCaseRow> kSuiteCaseRows = {
    SuiteCaseRow {"fib/bits",
                  "fib/bits/1/8/64",
                  "fib",
                  "bits/1/8/64",
                  [](sg::BenchmarkHandle& handle) { handle.args({1, 8, 64}); }},
    SuiteCaseRow {"QueueFixture/push",
                  "QueueFixture/push/8",
                  "QueueFixture",
                  "push/8",
                  [](sg::BenchmarkHandle& handle) { handle.args({8}); }},
    // The equal-name rule: the instance name equals its suite, so it
    // stays its case (R-08).
    SuiteCaseRow {
        "plain", "plain", "plain", "plain", [](sg::BenchmarkHandle&) {}},
};

// The expanded instance of one name, or null when the expansion left no
// such instance.
auto findInstance(const std::string& name) -> sg::Instance*
{
  for (auto& instance : sg::detail::instances()) {
    if (instance.name == name) {
      return &instance;
    }
  }
  return nullptr;
}

// FR-021, R-07, R-08: the pair is read through the two fields of the
// result value; the console row keeps its single name column. The seam is the
// one harness_gap_test.cpp reaches: expandRegistry, instances, and a Runner
// over a parsed RunOptions.
auto suiteCaseScenario() -> void
{
  for (const SuiteCaseRow& row : kSuiteCaseRows) {
    auto handle = sg::registerBenchmark(&bmInstance, row.family);
    row.build(handle);
  }

  sg::detail::expandRegistry();

  sg::detail::RunOptions options;
  options.fixedIterations = 1;
  sg::detail::Runner runner(options);

  for (const SuiteCaseRow& row : kSuiteCaseRows) {
    sg::Instance* const instance = findInstance(row.instance);
    check(instance != nullptr,
          "the expansion produced the contract's instance name (FR-010)");
    const sg::BenchmarkResult result = runner.run(*instance);
    check(result.name == row.instance,
          "result.name still carries the full instance name (H1, FR-021)");
    check(result.suite == row.suite,
          "the suite is the family name up to its first '/' (R-07, R-08)");
    check(result.caseName == row.caseName,
          "the case is the instance name with the leading suite and its '/' "
          "removed (R-08)");
  }
}

// FR-021: the pair is unique for each instance, following the unique
// instance name of FR-012. Across the whole instance list, no two
// instances share one (suite, caseName) pair.
auto pairUniquenessScenario() -> void
{
  sg::detail::expandRegistry();

  std::vector<const sg::Instance*> seen;
  for (const auto& instance : sg::detail::instances()) {
    for (const sg::Instance* const other : seen) {
      check(other->suite != instance.suite
                || other->caseName != instance.caseName,
            "no two instances share one (suite, caseName) pair (FR-021)");
    }
    seen.push_back(&instance);
  }
}

// R-12: after expansion the instance list groups stably by suite in
// first-appearance order, and rows inside one suite keep registration
// order. The three families below register in the stated order; the two
// order/ families share the suite order through the '/' in their family
// names, and the second family sits between them, so a list that merely
// kept registration order differs from the list the walk owes.
auto suiteOrderScenario() -> void
{
  auto first = sg::registerBenchmark(&bmInstance, "order/first");
  first.args({1}).args({2});
  auto middle = sg::registerBenchmark(&bmInstance, "second");
  middle.arg(3);
  auto later = sg::registerBenchmark(&bmInstance, "order/second");
  later.arg(4);

  sg::detail::expandRegistry();

  const std::vector<std::string> expected = {
      "order/first/1", "order/first/2", "order/second/4", "second/3"};
  std::vector<std::string> walked;
  for (const auto& instance : sg::detail::instances()) {
    if (instance.name.rfind("order/", 0) == 0 || instance.name == "second"
        || instance.name.rfind("second/", 0) == 0)
    {
      walked.push_back(instance.name);
    }
  }
  if (walked != expected) {
    for (const std::string& name : walked) {
      std::fprintf(stderr, "walked  %s\n", name.c_str());
    }
    for (const std::string& name : expected) {
      std::fprintf(stderr, "wanted  %s\n", name.c_str());
    }
    fail("the walk groups stably by suite in first-appearance order and "
         "keeps registration order inside one suite (R-12)");
  }
}

// FR-021, R-07: the console row keeps the single H1 name column carrying
// the full instance name; the split reaches the caller through the two
// fields of the result value; no suite column prints.
auto consoleRowScenario() -> void
{
  const std::string report =
      captureRun({"--filter", "^order/", "--iterations=1"});
  for (const char* const name :
       {"order/first/1", "order/first/2", "order/second/4"})
  {
    check(!lineOf(report, name).empty(),
          "each row of the filtered run starts with the full instance name "
          "(H1 name column, FR-021)");
  }
  check(report.find("suite") == std::string::npos,
        "the console row prints no separate suite column (R-07, FR-021)");
}

}  // namespace

auto main(const int argc, char** argv) -> int
{
  const std::string self = argc > 0 ? argv[0] : "harness_instance_name_test";
  const std::string mode = argc > 1 ? argv[1] : "";

  // The FR-012 re-exec: the two clashing registrations and one instance
  // that has to keep running, in a process whose two streams the parent
  // can separate into two files.
  if (mode == "duplicate") {
    scriptedProvider();
    auto kept = sg::registerBenchmark(&bmInstance, "bmDup");
    kept.arg(8);
    (void)sg::registerBenchmark(&bmDropped, "bmDup/8");
    auto other = sg::registerBenchmark(&bmInstance, "bmOther");
    other.args({1, 2});
    const std::vector<std::string> arguments = {"--iterations=1"};
    std::vector<char*> inner;
    inner.push_back(argv[0]);
    for (const auto& argument : arguments) {
      inner.push_back(const_cast<char*>(argument.c_str()));
    }
    return sg::speedgunMain(static_cast<int>(inner.size()), inner.data());
  }

  // FR-012 with the family instance as the later of the pair: the plain
  // registration holds the name, so the family expansion is the walk that
  // finds the clash.
  if (mode == "family-clash") {
    scriptedProvider();
    auto plain = sg::registerBenchmark(&bmDropped, "bmFam/9");
    (void)plain;
    auto family = sg::registerBenchmark(&bmInstance, "bmFam");
    family.arg(9);
    const std::vector<std::string> arguments = {"--iterations=1"};
    std::vector<char*> inner;
    inner.push_back(argv[0]);
    for (const auto& argument : arguments) {
      inner.push_back(const_cast<char*>(argument.c_str()));
    }
    return sg::speedgunMain(static_cast<int>(inner.size()), inner.data());
  }

  // FR-002 at the fixture registration: the plain registration holds the
  // name, so the fixture registration prints the refusal.
  if (mode == "fixture-clash") {
    scriptedProvider();
    auto plain = sg::registerBenchmark(&bmClash, "bmClash");
    (void)plain;
    (void)sg::registerFixtureBenchmark(
        [](sg::State& state)
        {
          for (auto _ : state) {
          }
        },
        "bmClash",
        [] { return std::make_unique<ClashFixture>(); });
    auto other = sg::registerBenchmark(&bmInstance, "bmOther");
    other.arg(2);
    const std::vector<std::string> arguments = {"--iterations=1"};
    std::vector<char*> inner;
    inner.push_back(argv[0]);
    for (const auto& argument : arguments) {
      inner.push_back(const_cast<char*>(argument.c_str()));
    }
    return sg::speedgunMain(static_cast<int>(inner.size()), inner.data());
  }

  scriptedProvider();

  nameFormScenario();
  filterScenario();
  listAndCatalogScenario();
  duplicateScenario(self);
  familyClashScenario(self);
  fixtureClashScenario(self);

  suiteCaseScenario();
  pairUniquenessScenario();
  suiteOrderScenario();
  consoleRowScenario();

  std::puts("harness_instance_name_test: ok");
  return 0;
}
