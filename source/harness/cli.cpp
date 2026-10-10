#include <atomic>
#include <cerrno>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <memory>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
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

// The largest whole number of seconds whose nanoseconds fit the signed
// count the run control carries.
constexpr std::int64_t kMaxWholeSeconds =
    std::numeric_limits<std::int64_t>::max() / 1'000'000'000;

// FR-020, R-11: the prefix of a disabled instance. The gate is a prefix
// test on the expanded instance name, and selection is where it stands.
constexpr std::string_view kDisabledPrefix = "DISABLED_";

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
  if (errno != 0 || end == text || *end != '\0' || seconds < 0.0
      || !std::isfinite(seconds)
      || seconds > static_cast<double>(kMaxWholeSeconds))
  {
    std::fprintf(
        stderr,
        "%s: expected a finite number of seconds at most %lld, got " "'%s'\n",
        option,
        static_cast<long long>(kMaxWholeSeconds),
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
  if (errno != 0 || end == text || *end != '\0' || !(seconds > 0.0)
      || !std::isfinite(seconds)
      || seconds > static_cast<double>(kMaxWholeSeconds))
  {
    std::fprintf(stderr,
                 "%s: expected a finite positive number of seconds at most "
                 "%lld, got '%s'\n",
                 option,
                 static_cast<long long>(kMaxWholeSeconds),
                 text);
    std::exit(2);
  }
  return static_cast<std::int64_t>(seconds * 1'000'000'000.0);
}

}  // namespace

namespace detail
{
namespace
{

// R-06: one flag for the process. The signal handler sets it, and the
// runner reads it after each run.
constinit std::atomic<bool> interruptFlagValue {false};

}  // namespace

static_assert(std::atomic<bool>::is_always_lock_free,
              "R-06: the signal handler reaches the flag with no "
              "static-initialization guard");

auto interruptFlag() -> std::atomic<bool>&
{
  return interruptFlagValue;
}

// FR-042: the entry point needs a time source. The counters system opens
// the host provider, and that provider publishes the machine object, so
// only the monotonic leaf can be missing (FR-002).
auto timeSourcePresent(counters::System& sys) -> bool
{
  const auto machine = sys.object("machine");
  SG_ASSERT(machine.has_value(),
            "the counters system opens the host provider, which publishes the "
            "machine object (FR-002)");
  return machine->counter<counters::Dim<1, 0>>("monotonic").has_value();
}

}  // namespace detail

/**
 * @brief Run the benchmark suite the command line names (FR-040).
 *
 * @pre none
 * @post none
 */
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
    if (!detail::timeSourcePresent(sys)) {
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
    detail::printCatalog();
    return 0;
  }

  // FR-003, R-03: expansion completes here, before the filter and
  // before the first run, so a run carries no expansion work.
  detail::expandRegistry();

  std::vector<Instance*> selected;
  std::optional<std::regex> pattern;
  if (!options.filter.empty()) {
    try {
      pattern.emplace(options.filter);
    } catch (const std::regex_error& error) {
      std::fprintf(stderr,
                   "--filter: invalid regular expression '%s': %s\n",
                   options.filter.c_str(),
                   error.what());
      return 2;
    }
  }
  for (auto& instance : detail::instances()) {
    // FR-020, R-11: the gate stands here, on the expanded instance name.
    // A disabled instance stays in the registry and in the expansion; the
    // filter never matches it and list mode never prints it, and a filter
    // whose only match is disabled leaves the empty-selection report of
    // contracts/cli.md untouched.
    if (instance.name.starts_with(kDisabledPrefix)) {
      continue;
    }
    if (!pattern || std::regex_search(instance.name, *pattern)) {
      selected.push_back(&instance);
    }
  }

  if (options.listMode) {
    for (auto* instance : selected) {
      std::printf("%s\n", instance->name.c_str());
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
  SG_ENSURE(!detail::interruptFlag().load(),
            "the entry point starts with no pending interrupt (R-06)");
  std::signal(SIGINT, handleInterrupt);
  detail::printContext();

  int status = 0;
  // The two run-wide flags are read by the postcondition below, which the
  // default semantic compiles away, so a consumer build needs the marker.
  [[maybe_unused]] bool anyFailed = false;
  [[maybe_unused]] bool anyInterrupted = false;
  detail::Runner runner(options);
  // The loop variable keeps the spelling `entry`: `test/cli_shape.sh`
  // locates this loop by the `runner.run(*entry)` line and reads the
  // flag exactly once inside it.
  for (auto* entry : selected) {
    const auto result = runner.run(*entry);
    detail::printResult(result);
    // FR-036: the flag is read once per pass, so the status update, the
    // per-pass check and the break all see one instant of the flag.
    const bool interrupted = detail::interruptFlag().load();
    if (result.outcome == RunOutcome::FAILED) {
      anyFailed = true;
      status = 1;
    }
    if (interrupted) {
      anyInterrupted = true;
      status = 1;
    }

    SG_ENSURE(!(result.outcome == RunOutcome::FAILED && status == 0),
              "a failed benchmark sets a nonzero exit status (FR-036)");

    if (interrupted) {
      break;
    }
  }

  SG_ENSURE((status != 0) == (anyFailed || anyInterrupted),
            "the exit status follows the table of contracts/cli.md (FR-036)");

  return status;
}

}  // namespace sg
