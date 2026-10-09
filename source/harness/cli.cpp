#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <regex>
#include <string>
#include <vector>

#include <getopt.h>

#include "detail/internal.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_clock.hpp"
#include "speedgun-ng/counters_pmu.hpp"
#include "speedgun-ng/counters_system.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg
{

namespace
{

void handleInterrupt(const int) noexcept
{
  detail::interruptFlag().store(true);
}

// FR-034: an invalid value is a usage error that reports and exits
// nonzero without running anything. The signed read is deliberate:
// `strtoull` wraps `-5` to a huge count, so a negative count would
// parse, run, and hang. The signed read turns it into a usage error
// that reports (T044).
auto parseCount(const char* text, const char* option) -> std::uint64_t
{
  errno = 0;
  char* end = nullptr;
  const long long value = std::strtoll(text, &end, 10);
  if (errno != 0 || end == text || *end != '\0' || value <= 0) {
    std::fprintf(
        stderr, "%s: expected a positive integer, got '%s'\n", option, text);
    std::exit(2);
  }
  return static_cast<std::uint64_t>(value);
}

auto parseSeconds(const char* text, const char* option) -> std::int64_t
{
  errno = 0;
  char* end = nullptr;
  const double seconds = std::strtod(text, &end);
  if (errno != 0 || end == text || *end != '\0' || seconds < 0.0) {
    std::fprintf(stderr,
                 "%s: expected a non-negative number of seconds, got " "'%s'\n",
                 option,
                 text);
    std::exit(2);
  }
  return static_cast<std::int64_t>(seconds * 1'000'000'000.0);
}

// FR-034 with the option table of contracts/cli.md: the min-time option
// takes a positive count of seconds, where the warmup-time option admits zero.
auto parsePositiveSeconds(const char* text, const char* option) -> std::int64_t
{
  errno = 0;
  char* end = nullptr;
  const double seconds = std::strtod(text, &end);
  if (errno != 0 || end == text || *end != '\0' || !(seconds > 0.0)) {
    std::fprintf(stderr,
                 "%s: expected a positive number of seconds, got " "'%s'\n",
                 option,
                 text);
    std::exit(2);
  }
  return static_cast<std::int64_t>(seconds * 1'000'000'000.0);
}

}  // namespace

namespace detail
{

auto interruptFlag() -> std::atomic<bool>&
{
  static std::atomic<bool> flag;
  return flag;
}

}  // namespace detail

auto speedgunMain(int argc, char** argv) -> int
{
  auto& sys = counters::System::local();

  // R-03: the entry point registers the clock provider; a test that
  // scripted the machine leaves through the fake provider owns that
  // path already, and the refusal is a pass (FR-042). The pass is
  // conditional on the leaves being present, so a refusal that leaves
  // the run with no time source reports the failure and stops.
  const auto registered =
      sys.registerProvider(std::make_unique<counters::ClockProvider>());
  if (!registered.has_value()) {
    const auto machine = sys.object("machine");
    const bool scripted = machine.has_value()
        && machine->counter<counters::Dim<1, 0>>("monotonic").has_value();
    if (!scripted) {
      std::fprintf(stderr,
                   "the clock provider cannot register: %s\n",
                   registered.error().message.c_str());
      return 1;
    }
  }

  // The metric leaves reach the catalog through the pmu provider; a host
  // that cannot count a leaf keeps that metric unavailable with its
  // refusal kind while the benchmark still runs (FR-023, SC-008).
  (void)sys.registerProvider(std::make_unique<counters::PmuProvider>());

  detail::RunOptions options;
  static option longOptions[] = {
      {"filter", required_argument, nullptr, 'f'},
      {"list", no_argument, nullptr, 'l'},
      {"repetitions", required_argument, nullptr, 'r'},
      {"min-time", required_argument, nullptr, 'm'},
      {"iterations", required_argument, nullptr, 'i'},
      {"warmup-time", required_argument, nullptr, 'w'},
      {"dry-run", no_argument, nullptr, 'd'},
      {"counter", required_argument, nullptr, 'c'},
      {"catalog", no_argument, nullptr, 'C'},
      {nullptr, 0, nullptr, 0},
  };

  int choice = 0;
  // The option scanner keeps process state, and a suite can call the
  // entry point more than once (a test does), so the scan restarts here.
  optind = 0;
  while ((choice = getopt_long(argc, argv, "", longOptions, nullptr)) != -1) {
    switch (choice) {
      case 'f':
        options.filter = optarg;
        break;
      case 'l':
        options.listMode = true;
        break;
      case 'r':
        options.repetitions = parseCount(optarg, "--repetitions");
        break;
      case 'm':
        options.minTimeNs = parsePositiveSeconds(optarg, "--min-time");
        break;
      case 'i':
        options.fixedIterations = parseCount(optarg, "--iterations");
        break;
      case 'w':
        options.warmupTimeNs = parseSeconds(optarg, "--warmup-time");
        break;
      case 'd':
        options.dryRun = true;
        break;
      case 'c':
        options.counterAddresses.emplace_back(optarg);
        break;
      case 'C':
        options.catalogMode = true;
        break;
      default:
        std::fprintf(stderr, "unknown option; see the option table of "
                             "specs/015-benchmark-harness-core/contracts/"
                             "cli.md\n");
        return 2;
    }
  }

  if (options.catalogMode) {
    return detail::printCatalog() ? 0 : 1;
  }

  std::vector<RegistryEntry*> selected;
  if (options.filter.empty()) {
    for (auto& entry : registry()) {
      selected.push_back(entry.get());
    }
  } else {
    const std::regex pattern(options.filter);
    for (auto& entry : registry()) {
      if (std::regex_search(entry->name, pattern)) {
        selected.push_back(entry.get());
      }
    }
  }

  if (options.listMode) {
    for (auto* entry : selected) {
      std::printf("%s\n", entry->name.c_str());
    }
    return 0;
  }

  if (selected.empty()) {
    std::fprintf(stderr,
                 "no benchmark matches the filter '%s'\n",
                 options.filter.c_str());
    return 0;
  }

  // One entry-point invocation owns one interrupt flag: a suite that
  // calls the entry point again starts with no pending interrupt.
  detail::interruptFlag().store(false);
  std::signal(SIGINT, handleInterrupt);
  detail::printContext();

  int status = 0;
  detail::Runner runner(options);
  for (auto* entry : selected) {
    const auto result = runner.run(*entry);
    detail::printResult(result);
    if (result.outcome == RunOutcome::FAILED) {
      status = 1;
    }
    if (detail::interruptFlag().load()) {
      status = 1;
    }

    SG_ENSURE((status != 0)
                  == (result.outcome == RunOutcome::FAILED
                      || detail::interruptFlag().load()),
              "the exit status follows the table of contracts/cli.md (FR-036)");

    if (detail::interruptFlag().load()) {
      break;
    }
  }

  return status;
}

}  // namespace sg
