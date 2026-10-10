// ============================================================================
// TDD test for instance argument access (T008; US1, capability C-3).
//
// Each instance of a two-argument family reads its own pair through
// range(index), and the value the body accumulates is what the test reads
// back (FR-008). rangeCount() reports the family arity and zero for a
// family with no family call. A range read inside the timed loop allocates
// nothing, asserted with the allocation-counting operator new and delete
// of harness_capture_test.cpp (FR-008, FR-009). The precondition
// violations (range(0) on a zero-argument family and range(1) on a
// one-argument family) end a re-exec of this binary, the way
// harness_registry_test.cpp:209-212 observes a contract violation. A
// scripted FakeProvider owns the machine time leaves, so no run grows into
// a long calibration. The FR-018 half of FR-008, that the arguments read
// legally in the callback state, is covered by harness_callback_test.cpp
// a second time: that suite reads range(0) of a one-argument
// instance, rangeCount() and iterations() of a two-argument instance, and
// rangeCount() of a zero-argument instance, which is the superset.
// Hand-computed expectations, frameworkless check()/fail() convention.
// ============================================================================

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <new>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"

// The allocation-counting operator new and delete of
// harness_capture_test.cpp, armed only around the runs whose timed loop
// owes no allocation (FR-008, FR-009).
std::uint64_t allocationCount = 0;
bool countAllocations = false;

auto allocate(const std::size_t size) -> void*
{
  if (countAllocations) {
    ++allocationCount;
  }
  return std::malloc(size == 0 ? 1 : size);
}

auto operator new(std::size_t size) -> void*
{
  void* const block = allocate(size);
  if (block == nullptr) {
    throw std::bad_alloc();
  }
  return block;
}

// The vendored simdjson allocates its parser with the nothrow form, so
// that form has to hand out the same memory as the free below. Left to
// the library, a nothrow allocation returns through this file's
// deallocation, which AddressSanitizer reports as alloc-dealloc-mismatch
// (operator new vs free).
auto operator new(std::size_t size, const std::nothrow_t&) noexcept -> void*
{
  return allocate(size);
}

// GCC's warning pass attributes every allocation in a translation unit
// to the default `operator new` and never consults the replacement
// declared above, so it reports `-Wmismatched-new-delete` at each `free`
// below. The pairing is correct: these allocations hand out `malloc`
// memory and the deletes return it with `free`. The suppression covers
// exactly that false positive, the way test/source/counters_noalloc_test.cpp
// does (constitution I, X.2).
#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic ignored "-Wmismatched-new-delete"
#endif

auto operator delete(void* block) noexcept -> void
{
  std::free(block);
}

auto operator delete(void* block, std::size_t) noexcept -> void
{
  std::free(block);
}

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS ARGUMENT TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
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

auto captureRun(const std::vector<std::string>& arguments) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_argument_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  const char* const path = "harness_argument_out.txt";
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
  if (status != 0) {
    std::fprintf(stderr, "speedgunMain returned %d\n", status);
    fail("the run did not exit zero");
  }

  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// The run of a violation mode: no capture, because the process ends at
// the violation and its report never completes.
auto runDirectly(char* program,
                 const std::vector<std::string>& arguments) -> int
{
  std::vector<char*> argv;
  argv.push_back(program);
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }
  return sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
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

// The FR-006 observation of harness_registry_test.cpp:209-212: the
// precondition ends the re-exec of this binary, so a nonzero status is
// the report that the check fired.
auto violationStatus(const std::string& self, const std::string& mode) -> int
{
  return std::system(("\"" + self + "\" " + mode + " 2>/dev/null").c_str());
}

// Case a: the two args calls state two instances of a two-argument
// family, and each instance's run reads its own pair back through
// range(0) and range(1) (FR-001, FR-008).
std::vector<std::int64_t> seenPairs;

auto bmPair(sg::State& state) -> void
{
  seenPairs.push_back(state.range(0));
  seenPairs.push_back(state.range(1));
  for (auto _ : state) {
  }
}

auto twoArgumentFamily() -> void
{
  auto handle = sg::registerBenchmark(&bmPair, "bmPair");
  handle.args({8, 64}).args({1, 2});
  const std::string report =
      captureRun({"--filter", "^bmPair/", "--iterations=1"});
  check(linesOf(report, "bmPair/").size() == 2,
        "the two args calls expanded two instances (FR-001)");
  check(seenPairs == std::vector<std::int64_t> {8, 64, 1, 2},
        "each instance read its own pair through range(index) (FR-008)");
}

// Case b: rangeCount() reports the arity of the running instance's
// family, and zero for a family with no family call at all (FR-008).
std::vector<std::size_t> seenCounts;

auto bmCounted(sg::State& state) -> void
{
  seenCounts.push_back(state.rangeCount());
  for (auto _ : state) {
  }
}

auto argumentCountReported() -> void
{
  auto pairHandle = sg::registerBenchmark(&bmCounted, "bmCountedPair");
  pairHandle.args({8, 64});
  (void)sg::registerBenchmark(&bmCounted, "bmCountedPlain");
  const std::string report =
      captureRun({"--filter", "^bmCounted", "--iterations=1"});
  check(linesOf(report, "bmCounted").size() == 2,
        "the family and the plain registration both ran");
  check(seenCounts == std::vector<std::size_t> {2, 0},
        "rangeCount reports the arity and zero for no family call (FR-008)");
}

// Case c: a range read inside the timed loop allocates nothing. The
// body compares the armed counter against its own baseline, and the
// totals of every run's loop have to stay at zero (FR-008, FR-009).
std::uint64_t loopAllocations = 0;

auto bmRangeRead(sg::State& state) -> void
{
  const std::uint64_t baseline = allocationCount;
  for (auto _ : state) {
    if (state.range(0) + state.range(1) == 0 || allocationCount != baseline) {
      fail("a range read inside the timed loop allocated (FR-008, FR-009)");
    }
  }
  loopAllocations += allocationCount - baseline;
}

auto rangeReadAllocatesNothing() -> void
{
  auto handle = sg::registerBenchmark(&bmRangeRead, "bmRangeRead");
  handle.args({8, 64}).args({1, 2});
  allocationCount = 0;
  countAllocations = true;
  captureRun({"--filter", "^bmRangeRead/", "--iterations=8"});
  countAllocations = false;
  check(loopAllocations == 0,
        "the range reads inside the timed loops allocated nothing (FR-008)");
}

// Case d: range(0) on a family with no family call is a precondition
// violation, so this body ends the process that runs it.
auto bmZeroRead(sg::State& state) -> void
{
  (void)state.range(0);
  for (auto _ : state) {
  }
}

auto readZeroArgumentFamily(char* program) -> void
{
  scriptedProvider();
  (void)sg::registerBenchmark(&bmZeroRead, "bmZeroRead");
  (void)runDirectly(program, {"--filter", "^bmZeroRead$", "--iterations=1"});
}

// Case e: an index at or above the argument count is the same
// violation, one argument deep.
auto bmOneRead(sg::State& state) -> void
{
  (void)state.range(1);
  for (auto _ : state) {
  }
}

auto readPastFamilyArity(char* program) -> void
{
  scriptedProvider();
  auto handle = sg::registerBenchmark(&bmOneRead, "bmOneRead");
  handle.arg(8);
  // The instance name of a one-argument family carries its argument
  // segment, so the filter names the instance (FR-010, FR-011).
  (void)runDirectly(program, {"--filter", "^bmOneRead/8$", "--iterations=1"});
}

}  // namespace

auto main(const int argc, char** argv) -> int
{
  const std::string self = argc > 0 ? argv[0] : "harness_argument_test";
  const std::string mode = argc > 1 ? argv[1] : "";

  // Each of these reads is a precondition violation, so the process
  // ends before it can report a zero status of its own.
  if (mode == "range-zero-args") {
    readZeroArgumentFamily(argv[0]);
    return 0;
  }

  if (mode == "range-past-arity") {
    readPastFamilyArity(argv[0]);
    return 0;
  }

  scriptedProvider();

  twoArgumentFamily();
  argumentCountReported();
  rangeReadAllocatesNothing();

  // The two violation modes end their own subprocesses, observed the
  // way harness_registry_test.cpp:209-212 observes a violation.
  check(violationStatus(self, "range-zero-args") != 0,
        "range(0) on a zero-argument family is a precondition violation "
        "(FR-008)");
  check(violationStatus(self, "range-past-arity") != 0,
        "range(1) on a one-argument family is a precondition violation "
        "(FR-008)");

  std::puts("harness_argument_test: ok");
  return 0;
}
