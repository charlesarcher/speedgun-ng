// ============================================================================
// TDD test for the instance names SG_BENCHMARK_TEMPLATE expands to (T023;
// US3, capability C-5; FR-014, R-13).
//
// One function template stands for every case, and the instance name each
// registration owes is a literal of this file, hand-computed from the naming
// rule of FR-014: the function name, then '<', then the type list stringified
// exactly as it is written at the macro site with one space after every
// comma, then '>'. The names are read back through list mode, which prints
// one name per line and runs no benchmark function, and through the rows of
// one filtered run, which is how FR-011 selects a single instance. That run
// carries the second reading: each instantiation records the type name it was
// instantiated for, a string the compiler picks from a specialization, so a
// name wired to the wrong instantiation fails on the record and not only on
// the report. A scripted FakeProvider owns the machine time leaves, so no run
// grows into a long calibration. Hand-computed expectations, frameworkless
// check()/fail() convention.
// ============================================================================

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <unistd.h>

#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS TEMPLATE TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// The count of benchmark functions the selections of this test reached.
// List mode has to leave it at zero.
int instanceRuns = 0;

// The type names the bodies recorded, in the order the bodies ran. Each
// instantiation contributes the string the compiler chose for it, so the
// vector says which instantiation ran.
std::vector<std::string> instantiated;

// The compile-time chosen string of one type argument. A primary template
// that answers "unknown" is a failure this test reads back: it means an
// instantiation ran for a type no specialization of this file names.
template<typename T>
auto typeName() -> const char*
{
  return "unknown";
}

template<>
auto typeName<int>() -> const char*
{
  return "int";
}

template<>
auto typeName<double>() -> const char*
{
  return "double";
}

template<>
auto typeName<std::pair<int, int>>() -> const char*
{
  return "std::pair<int, int>";
}

// The one body of the two sortOf cases. Its type list is the type list the
// macro site writes, so `sortOf` over one argument and over two are the two
// shapes FR-014 names.
template<typename... T>
auto sortOf(sg::State& state) -> void
{
  ++instanceRuns;
  for (auto _ : state) {
  }
  (instantiated.push_back(typeName<T>()), ...);
}

// The body of the qualified-type-argument case: one type argument written
// with a space inside the angle brackets.
template<typename T>
auto pairOf(sg::State& state) -> void
{
  ++instanceRuns;
  for (auto _ : state) {
  }
  instantiated.push_back(typeName<T>());
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
  argv.push_back(const_cast<char*>("harness_template_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  const char* const path = "harness_template_out.txt";
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

// FR-014: one type argument registers one instance, and list mode prints the
// name as the function name, '<', that one type, '>'. The name is the literal
// this file states, hand-computed from the naming rule.
auto singleTypeArgumentScenario() -> void
{
  const std::string listed = captureRun({"--list"});
  check(instanceRuns == 0, "list mode runs no benchmark function (FR-011)");
  check(lineOf(listed, "sortOf<int>") == "sortOf<int>",
        "one type argument registers one instance named sortOf<int> (FR-014)");
  check(linesOf(listed, "sortOf<int>").size() == 1,
        "the registration prints exactly one instance name (FR-014)");
  check(linesOf(listed, "sortOf<int >").empty(),
        "the name carries no spacing the macro site did not write (FR-014, "
        "R-13)");
}

// FR-014, R-13: two type arguments still register one instance, and the type
// list enters the name stringified as written at the macro site, comma
// separated with one space after the comma. The spelling without the space is
// the wrong answer this case rules out.
auto twoTypeArgumentsScenario() -> void
{
  const std::string listed = captureRun({"--list"});
  check(lineOf(listed, "sortOf<int, double>") == "sortOf<int, double>",
        "two type arguments register one instance named sortOf<int, double> "
        "(FR-014)");
  check(linesOf(listed, "sortOf<int, double>").size() == 1,
        "the two-argument registration prints exactly one instance name "
        "(FR-014)");
  check(linesOf(listed, "sortOf<int,double>").empty(),
        "the stringified list puts one space after the comma, as the macro "
        "site writes it (FR-014, R-13)");
  check(linesOf(listed, "sortOf<").size() == 2,
        "the two sortOf registrations give the two names and no other "
        "(FR-014)");
}

// FR-014, R-13: a qualified type argument written with a space enters the
// instance name exactly as written at the macro site. The preprocessor splits
// such an argument at its comma, so the name is the reading that says the
// split was joined back the way the site spells it.
auto qualifiedTypeArgumentScenario() -> void
{
  const std::string listed = captureRun({"--list"});
  check(lineOf(listed, "pairOf<std::pair<int, int>>") ==
            "pairOf<std::pair<int, int>>",
        "a qualified type argument written with a space enters the name "
        "exactly as written at the macro site (FR-014, R-13)");
  check(linesOf(listed, "pairOf<").size() == 1,
        "the qualified argument registers one instance and not two (FR-014)");
  check(linesOf(listed, "pairOf<std::pair<int>").empty(),
        "the type argument is never cut at its comma (FR-014, R-13)");
}

// FR-014: the instance is not only named, it is the instantiation its name
// states. One filtered run per name, and the body of each instantiation
// records the type name it was instantiated for, so a name wired to the wrong
// instantiation fails on the record.
auto instantiationRunsScenario() -> void
{
  struct RunCase
  {
    const char* instance;
    std::vector<std::string> types;
  };

  const std::vector<RunCase> runCases = {
      RunCase {"sortOf<int>", {"int"}},
      RunCase {"sortOf<int, double>", {"int", "double"}},
      RunCase {"pairOf<std::pair<int, int>>", {"std::pair<int, int>"}},
  };

  for (const RunCase& runCase : runCases) {
    instantiated.clear();
    const int before = instanceRuns;
    const std::string run =
        captureRun({"--filter",
                    std::string("^") + runCase.instance + "$",
                    "--iterations=1"});
    check(linesOf(run, runCase.instance).size() == 1,
          "one full template instance name reports one row (FR-011)");
    check(instanceRuns - before == 1,
          "one full template instance name runs one instance (FR-011)");
    if (instantiated != runCase.types) {
      std::fprintf(stderr, "instance %s\n", runCase.instance);
      for (const std::string& type : runCase.types) {
        std::fprintf(stderr, "expected %s\n", type.c_str());
      }
      for (const std::string& type : instantiated) {
        std::fprintf(stderr, "recorded %s\n", type.c_str());
      }
      fail("the run reached the instantiation the instance name names "
           "(FR-014)");
    }
  }
}

}  // namespace

// The three registrations whose names FR-014 owes, at namespace scope the way
// SG_BENCHMARK registers, so the type list of each is written exactly once at
// the macro site and stringified from there.
SG_BENCHMARK_TEMPLATE(sortOf, int)
SG_BENCHMARK_TEMPLATE(sortOf, int, double)
SG_BENCHMARK_TEMPLATE(pairOf, std::pair<int, int>)

auto main() -> int
{
  scriptedProvider();

  singleTypeArgumentScenario();
  twoTypeArgumentsScenario();
  qualifiedTypeArgumentScenario();
  instantiationRunsScenario();

  std::puts("harness_template_test: ok");
  return 0;
}
