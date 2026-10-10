// ============================================================================
// TDD test for the catalog listing (T025; US3, FR-036, FR-037, SC-009).
//
// A scripted FakeProvider owns a known leaf set, so every field of every
// published line is checked against the declaration. The listing runs no
// benchmark function: each benchmark body sets a flag the assertions read.
// Frameworkless check()/fail() convention.
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

#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"

namespace
{

bool benchmarkRan = false;

[[noreturn]] auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS CATALOG TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

auto bmCatalog(sg::State& state) -> void
{
  benchmarkRan = true;
  for (auto _ : state) {
  }
}

auto captureRun(const std::vector<std::string>& arguments,
                int expected = 0) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_catalog_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }

  const char* const path = "harness_catalog_out.txt";
  const int saved = dup(STDOUT_FILENO);
  if (saved < 0 || std::freopen(path, "w", stdout) == nullptr) {
    fail("cannot redirect the listing");
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

auto lineWith(const std::string& report,
              const std::string& needle) -> std::string
{
  std::istringstream stream(report);
  std::string line;
  while (std::getline(stream, line)) {
    if (line.find(needle) != std::string::npos) {
      return line;
    }
  }
  std::fprintf(stderr, "no line carries '%s'\n", needle.c_str());
  fail("the listing carries no such line");
}

auto scriptedProvider() -> void
{
  using sg::counters::Availability;
  using sg::counters::ReadMode;

  auto provider = std::make_unique<sg::counters::FakeProvider>();
  provider->addCounter(
      "machine", "monotonic", "nanoseconds", "monotonic wall time");
  provider->setPoints("machine", "monotonic", {0}, 4000);
  provider->addCounter(
      "machine", "thread_cpu", "nanoseconds", "thread cpu time");
  provider->setPoints("machine", "thread_cpu", {0}, 2000);
  provider->addObject("scripted", "scripted", "the scripted object");
  provider->addCounter("scripted",
                       "events",
                       "none",
                       "a countable leaf",
                       Availability::COUNTABLE,
                       ReadMode::FAST_RDPMC);
  provider->addCounter("scripted",
                       "blocked",
                       "none",
                       "a permission-blocked leaf",
                       Availability::PERMISSION_BLOCKED);
  provider->addCounter(
      "scripted", "missing", "ops", "an absent leaf", Availability::ABSENT);
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(provider));
  check(registered.has_value(), "the fake provider registers");
}

}  // namespace

SG_BENCHMARK(bmCatalog);

auto main() -> int
{
  scriptedProvider();

  const std::string listing = captureRun({"--catalog"});

  // FR-037: one line per published counter, six fields, the address in
  // the counter-option spelling.
  const std::string events = lineWith(listing, "catalog: scripted/events");
  check(events.find("description=a countable leaf") != std::string::npos,
        "the line carries the description (FR-037)");
  check(events.find("unit=none") != std::string::npos,
        "the line carries the unit (FR-037)");
  check(events.find("mode=fast-rdpmc") != std::string::npos,
        "the line carries the read mode (FR-037)");
  check(events.find("avail=countable") != std::string::npos
            && events.find("refusal=-") != std::string::npos,
        "a countable leaf carries no refusal kind (FR-037)");

  const std::string blocked = lineWith(listing, "catalog: scripted/blocked");
  check(blocked.find("avail=permission-blocked") != std::string::npos
            && blocked.find("refusal=permission-blocked") != std::string::npos,
        "a leaf that cannot count carries its refusal kind (FR-037)");
  const std::string missing = lineWith(listing, "catalog: scripted/missing");
  check(missing.find("unit=ops") != std::string::npos
            && missing.find("refusal=absent") != std::string::npos,
        "the absent leaf prints its unit and refusal kind (FR-037)");
  check(lineWith(listing, "catalog: machine/monotonic").find("unit=nanoseconds")
            != std::string::npos,
        "the machine leaves join the listing (FR-037)");

  // FR-036: the listing runs no benchmark function and exits zero.
  check(!benchmarkRan, "the listing runs no benchmark function (FR-036)");
  check(listing.find("iterations=") == std::string::npos,
        "the listing prints no result row (FR-036)");

  std::puts("harness_catalog_test: ok");
  return 0;
}
