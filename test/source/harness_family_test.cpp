// ============================================================================
// TDD test for the argument families (T007; US1, capability C-1).
//
// One benchmark body stands for every family, and each family call of
// FR-001 and FR-002 is one row of the literal kFamilyCases table: the call
// on one side, the instance set it owes on the other, every row
// hand-computed from the semantics of the cited revision. The set is read
// back through list mode, which prints the expanded instance names with no
// benchmark function run (FR-003), and through the rows of one filtered
// run. The FR-006 preconditions end a re-exec of this binary, the way
// harness_registry_test.cpp:209-212 observes a contract violation, and the
// FR-007 warning is read from the standard error of such a re-exec. A
// scripted FakeProvider owns the machine time leaves, so no run grows into
// a long calibration. Hand-computed expectations, frameworkless
// check()/fail() convention.
// ============================================================================

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS FAMILY TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// The one body every family runs. Its count is the number of instances
// the family expanded to, and list mode has to leave it at zero (FR-003).
int familyRuns = 0;

auto bmFamily(sg::State& state) -> void
{
  ++familyRuns;
  for (auto _ : state) {
  }
}

auto captureRun(const std::vector<std::string>& arguments,
                int expected = 0) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_family_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  const char* const path = "harness_family_out.txt";
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

// The FR-006 observation of harness_registry_test.cpp:209-212: the
// precondition ends the re-exec of this binary, so a nonzero status is the
// report that the check fired.
auto violationStatus(const std::string& self, const std::string& mode) -> int
{
  return std::system(("\"" + self + "\" " + mode + " 2>/dev/null").c_str());
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

}  // namespace

auto main(const int argc, char** argv) -> int
{
  const std::string self = argc > 0 ? argv[0] : "harness_family_test";
  const std::string mode = argc > 1 ? argv[1] : "";

  // FR-007: a family above the bound of 100 instances warns once, keeps
  // every instance, and finishes with the status a normal run carries. The
  // warning goes to the standard error stream, so the two streams are
  // separate files here and the warning is the whole of one of them.
  if (mode == "oversize") {
    scriptedProvider();
    auto handle = sg::registerBenchmark(&bmFamily, "bmOversize");
    handle.denseRange(1, 101);
    const std::vector<std::string> arguments = {
        "--filter", "^bmOversize/", "--iterations=1"};
    std::vector<char*> inner;
    inner.push_back(argv[0]);
    for (const auto& argument : arguments) {
      inner.push_back(const_cast<char*>(argument.c_str()));
    }
    return sg::speedgunMain(static_cast<int>(inner.size()), inner.data());
  }

  // FR-006: each of these calls is a precondition violation, so the
  // process ends before it can report a zero status of its own.
  if (mode == "bad-multiplier") {
    auto handle = sg::registerBenchmark(&bmFamily, "bmBadMultiplier");
    handle.rangeMultiplier(1);
    return 0;
  }

  if (mode == "bad-range") {
    auto handle = sg::registerBenchmark(&bmFamily, "bmBadRange");
    handle.range(1024, 8);
    return 0;
  }

  if (mode == "bad-step") {
    auto handle = sg::registerBenchmark(&bmFamily, "bmBadStep");
    handle.denseRange(1, 4, 0);
    return 0;
  }

  if (mode == "bad-argnames") {
    auto handle = sg::registerBenchmark(&bmFamily, "bmBadArgNames");
    handle.args({1, 2}).argNames({"a"});
    return 0;
  }

  if (mode == "bad-apply") {
    auto handle = sg::registerBenchmark(&bmFamily, "bmBadApply");
    handle.args({1, 2});
    handle.apply([](sg::BenchmarkHandle& inner) { inner.arg(3); });
    return 0;
  }

  if (mode == "bad-builder") {
    (void)sg::createRange(1024, 8);
    return 0;
  }

  scriptedProvider();

  // FR-002: the builders return the lists a family call accepts, with the
  // semantics of the cited revision and the default multiplier of 8.
  check(
      sg::createRange(8, 1024) == std::vector<std::int64_t> {8, 64, 512, 1024},
      "createRange defaults to the multiplier 8 (FR-002, FR-004)");
  check(sg::createRange(1, 8, 2) == std::vector<std::int64_t> {1, 2, 4, 8},
        "createRange takes the multiplier it is given (FR-002)");
  check(sg::createDenseRange(1, 4) == std::vector<std::int64_t> {1, 2, 3, 4},
        "createDenseRange steps by the default 1 (FR-002, FR-005)");
  check(sg::createDenseRange(0, 10, 5) == std::vector<std::int64_t> {0, 5, 10},
        "createDenseRange takes the step it is given (FR-002, FR-005)");
  // FR-004: a bound pair that reaches below zero grows the negative side by
  // the same powers, mirrored, and zero joins the list once. FR-005: a dense
  // range steps across zero on the step it is given.
  check(sg::createRange(-8, 8, 2)
            == std::vector<std::int64_t> {-8, -4, -2, -1, 0, 1, 2, 4, 8},
        "createRange mirrors the negative list around zero (FR-004)");
  check(sg::createRange(-16, -1, 2)
            == std::vector<std::int64_t> {-16, -8, -4, -2, -1},
        "createRange grows a wholly negative pair away from zero (FR-004)");
  check(sg::createDenseRange(-4, 4, 2)
            == std::vector<std::int64_t> {-4, -2, 0, 2, 4},
        "createDenseRange steps across zero (FR-005)");
  // FR-004: a bound pair whose bounds coincide yields that one value, a pair
  // with a power of the multiplier strictly inside yields that power as well
  // as both bounds, and a pair with none inside yields both bounds.
  check(sg::createRange(8, 8) == std::vector<std::int64_t> {8},
        "createRange yields the single value of a coincident pair (FR-004)");
  check(sg::createRange(3, 5, 2) == std::vector<std::int64_t> {3, 4, 5},
        "createRange inserts the power that falls inside the pair (FR-004)");
  check(sg::createRange(5, 6, 2) == std::vector<std::int64_t> {5, 6},
        "createRange yields both bounds when no power falls inside (FR-004)");
  check(sg::createRange(-6, -5, 2) == std::vector<std::int64_t> {-6, -5},
        "a wholly negative pair with no power inside yields both bounds "
        "(FR-004)");
  // FR-004: a bound beyond the last power joins the end of the list, and a
  // bound at the limit of the type stops the power growth before it wraps.
  check(
      sg::createRange(-10, 10, 2)
          == std::vector<std::int64_t> {-10, -8, -4, -2, -1, 0, 1, 2, 4, 8, 10},
      "a bound beyond the last power joins both ends (FR-004)");
  const auto huge =
      sg::createRange(1, std::numeric_limits<std::int64_t>::max());
  check(huge.size() == 22 && huge.front() == 1
            && huge[huge.size() - 2] == 1152921504606846976LL
            && huge.back() == std::numeric_limits<std::int64_t>::max(),
        "power growth stops at the type limit and the high bound still lands "
        "(FR-004)");

  struct FamilyCase
  {
    const char* name;
    void (*build)(sg::BenchmarkHandle& handle);
    std::vector<std::string> instances;
  };

  // The expected set of every family call, in the order the expansion
  // owes it. Each row is hand-computed from the cited semantics.
  const std::vector<FamilyCase> kFamilyCases = {
      // arg: one argument list appended per call.
      FamilyCase {"bmArg",
                  [](sg::BenchmarkHandle& handle)
                  { handle.arg(8).arg(16).arg(32); },
                  {"bmArg/8", "bmArg/16", "bmArg/32"}},
      // args over an initializer list: one list per call.
      FamilyCase {"bmArgsList",
                  [](sg::BenchmarkHandle& handle)
                  { handle.args({8, 64}).args({8, 1024}); },
                  {"bmArgsList/8/64", "bmArgsList/8/1024"}},
      // args over a vector: the same rule through the other overload.
      FamilyCase {"bmArgsVector",
                  [](sg::BenchmarkHandle& handle)
                  {
                    handle.args(std::vector<std::int64_t> {2, 3})
                        .args(std::vector<std::int64_t> {5, 7});
                  },
                  {"bmArgsVector/2/3", "bmArgsVector/5/7"}},
      // range at the default multiplier 8: both bounds, and the powers
      // of 8 strictly between them.
      FamilyCase {"bmRange",
                  [](sg::BenchmarkHandle& handle) { handle.range(8, 1024); },
                  {"bmRange/8", "bmRange/64", "bmRange/512", "bmRange/1024"}},
      // rangeMultiplier then range: the multiplier reaches that later
      // range call of the family.
      FamilyCase {"bmHalf",
                  [](sg::BenchmarkHandle& handle)
                  { handle.rangeMultiplier(2).range(1, 8); },
                  {"bmHalf/1", "bmHalf/2", "bmHalf/4", "bmHalf/8"}},
      // A multiplier set between two range calls reaches the later call
      // and leaves the earlier one at the default of 8.
      FamilyCase {
          "bmLater",
          [](sg::BenchmarkHandle& handle)
          { handle.range(8, 64).rangeMultiplier(2).range(1, 4); },
          {"bmLater/8", "bmLater/64", "bmLater/1", "bmLater/2", "bmLater/4"}},
      // ranges: one AddRange per bound pair.
      FamilyCase {
          "bmRanges",
          [](sg::BenchmarkHandle& handle)
          { handle.ranges({{8, 16}, {100, 200}}); },
          // `ranges` grows each bound pair and takes the two grown lists
          // as one product, so the family arity is two (FR-001, FR-004).
          {"bmRanges/8/100",
           "bmRanges/16/100",
           "bmRanges/8/200",
           "bmRanges/16/200"}},
      // denseRange at the default step: the bounds and every value
      // between them.
      FamilyCase {"bmDense",
                  [](sg::BenchmarkHandle& handle) { handle.denseRange(1, 4); },
                  {"bmDense/1", "bmDense/2", "bmDense/3", "bmDense/4"}},
      // denseRange with a step.
      FamilyCase {"bmStepped",
                  [](sg::BenchmarkHandle& handle)
                  { handle.denseRange(0, 10, 5); },
                  {"bmStepped/0", "bmStepped/5", "bmStepped/10"}},
      // argsProduct: the first list is the outer loop.
      FamilyCase {
          "bmProduct",
          [](sg::BenchmarkHandle& handle)
          { handle.argsProduct({{1, 2}, {3, 4}}); },
          // The odometer of the cited revision advances the first list
          // fastest (FR-001).
          {"bmProduct/1/3", "bmProduct/2/3", "bmProduct/1/4", "bmProduct/2/4"}},
      // apply: the callable receives the handle and states the family.
      FamilyCase {"bmApplied",
                  [](sg::BenchmarkHandle& handle) {
                    handle.apply([](sg::BenchmarkHandle& inner)
                                 { inner.denseRange(1, 3); });
                  },
                  {"bmApplied/1", "bmApplied/2", "bmApplied/3"}},
      // argName labels the segment of one argument position. The family
      // arity here is one, so one label is legal: a label list whose
      // length differs from the family arity is a precondition
      // violation (FR-006), which `args({8, 64})` would have made.
      FamilyCase {"bmLabelled",
                  [](sg::BenchmarkHandle& handle)
                  { handle.argName("size").arg(8).arg(64); },
                  {"bmLabelled/size:8", "bmLabelled/size:64"}},
      // argNames labels every position of the family arity.
      FamilyCase {"bmNamed",
                  [](sg::BenchmarkHandle& handle)
                  { handle.args({2, 3}).args({5, 7}).argNames({"w", "h"}); },
                  {"bmNamed/w:2/h:3", "bmNamed/w:5/h:7"}},
      // FR-002: a family call accepts the list createRange builds.
      // FR-002: a family call accepts the list createRange builds, and
      // one `args` call appends one list, so the list becomes one
      // instance of that arity: the product takes the list as one factor.
      FamilyCase {"bmBuilt",
                  [](sg::BenchmarkHandle& handle)
                  { handle.args(sg::createRange(8, 1024)); },
                  {"bmBuilt/8/64/512/1024"}},
      FamilyCase {"bmBuiltScaled",
                  [](sg::BenchmarkHandle& handle)
                  { handle.args(sg::createRange(1, 8, 2)); },
                  {"bmBuiltScaled/1/2/4/8"}},
      // FR-002: and the list createDenseRange builds.
      FamilyCase {"bmBuiltDense",
                  [](sg::BenchmarkHandle& handle)
                  { handle.args(sg::createDenseRange(0, 10, 5)); },
                  {"bmBuiltDense/0/5/10"}},
  };

  for (const FamilyCase& testCase : kFamilyCases) {
    auto handle = sg::registerBenchmark(&bmFamily, testCase.name);
    check(handle.name() == testCase.name, "the registration names itself");
    testCase.build(handle);
  }

  // FR-003: the expansion is complete before the CLI selects anything, so
  // list mode prints every instance name of every family and runs no
  // benchmark function at all.
  const std::string listed = captureRun({"--list"});
  check(familyRuns == 0, "list mode runs no benchmark function (FR-003)");
  check(listed.find("time/iter") == std::string::npos,
        "list mode carries no measured row (FR-003)");
  for (const FamilyCase& testCase : kFamilyCases) {
    const std::vector<std::string> printed = namesUnder(listed, testCase.name);
    if (printed != testCase.instances) {
      for (const std::string& name : testCase.instances) {
        std::fprintf(stderr, "expected %s\n", name.c_str());
      }
      for (const std::string& name : printed) {
        std::fprintf(stderr, "listed   %s\n", name.c_str());
      }
      fail("a family expands to the instance set of the cited revision "
           "(FR-001 to FR-005)");
    }
  }

  // FR-003 again from the other side: the filter reads instance names, so
  // the expansion it selects against was already complete.
  const std::string selected =
      captureRun({"--filter", "^bmProduct/1/4$", "--list"});
  check(linesOf(selected, "bmProduct/").size() == 1
            && lineOf(selected, "bmProduct/1/4").size() > 0,
        "one instance name selects one instance (FR-003)");

  // The same set as behaviour: one run per instance, in the order the
  // expansion produced.
  const std::string product =
      captureRun({"--filter", "^bmProduct/", "--iterations=1"});
  const std::vector<std::string> rows = linesOf(product, "bmProduct/");
  check(rows.size() == 4, "the product family runs one run per instance");
  check(familyRuns == 4, "every instance of the family reaches the function");
  check(rows[0].rfind("bmProduct/1/3", 0) == 0,
        "the report keeps the product order of the cited revision");

  // FR-007: 101 instances is one past the bound. One warning names the
  // bound, every instance still runs, and the status is the normal one.
  const int oversize =
      std::system(("\"" + self + "\" oversize" + " >harness_family_run.txt"
                   + " 2>harness_family_warn.txt")
                      .c_str());
  check(oversize == 0, "a family above the bound keeps the run's exit status");
  const std::string warning = readFile("harness_family_warn.txt");
  check(nonEmptyLineCount(warning) == 1,
        "a family above the bound draws one warning (FR-007)");
  check(warning.find("100") != std::string::npos,
        "the warning names the bound of 100 instances (FR-007)");
  check(
      linesOf(readFile("harness_family_run.txt"), "bmOversize/").size() == 101,
      "a family above the bound keeps every instance (FR-007)");

  // FR-006: each violation ends its own subprocess.
  check(violationStatus(self, "bad-multiplier") != 0,
        "a range multiplier below 2 is a precondition violation (FR-006)");
  check(violationStatus(self, "bad-range") != 0,
        "a low bound above a high bound is a precondition violation (FR-006)");
  check(violationStatus(self, "bad-step") != 0,
        "a denseRange step below 1 is a precondition violation (FR-006)");
  check(violationStatus(self, "bad-argnames") != 0,
        "a label count unequal to the family arity is a precondition "
        "violation (FR-006)");
  check(violationStatus(self, "bad-apply") != 0,
        "an apply that states another arity is a precondition violation "
        "(FR-006)");
  check(violationStatus(self, "bad-builder") != 0,
        "the builders carry the same preconditions (FR-002, FR-006)");

  std::puts("harness_family_test: ok");
  return 0;
}
