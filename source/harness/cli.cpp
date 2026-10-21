#include <algorithm>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <limits>
#include <memory>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <vector>

#include <getopt.h>

#include "detail/compare.hpp"
#include "detail/internal.hpp"
#include "detail/json_report.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_clock.hpp"
#include "speedgun-ng/counters_metrics.hpp"
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
// specs/021 FR-021, FR-022: the point list of `--percentiles=`. One
// element is optional spaces, one or more digits, an optional point with
// digits after it, and optional spaces; it stands in the open interval
// (0, 100). A list holds at least one element and at most
// kMaxPercentilePoints of them. The shape is checked by hand rather than
// left to `strtod`, which reads `.5` and `50.` as numbers and a trailing
// space as the end of the element.
constexpr std::size_t kMaxPercentilePoints = 16;

// specs/026 FR-048, FR-049, FR-045: the value each compare option answers
// from `getopt_long`. A long option with no letter of its own takes a number
// of 256 or more, so it cannot collide with a short option. The number 256
// belongs to the metric method option of the counters feature.
constexpr int kCompareOptionValue = 257;
constexpr int kCompareRuleOptionValue = 258;
constexpr int kCompareStrictOptionValue = 259;

[[noreturn]] auto rejectPercentileList(const char* option,
                                       const char* text) -> void
{
  std::fprintf(stderr,
               "%s: expected comma-separated points in (0, 100), got '%s'\n",
               option,
               text);
  std::exit(2);
}

auto parsePercentileList(const char* text,
                         const char* option,
                         std::vector<double>& points) -> void
{
  const char* at = text;
  bool more = true;
  while (more) {
    while (std::isspace(static_cast<unsigned char>(*at)) != 0) {
      ++at;
    }
    const char* const begin = at;
    while (std::isdigit(static_cast<unsigned char>(*at)) != 0) {
      ++at;
    }
    const bool hasInteger = at != begin;
    bool shaped = true;
    if (*at == '.') {
      ++at;
      const char* const afterPoint = at;
      while (std::isdigit(static_cast<unsigned char>(*at)) != 0) {
        ++at;
      }
      shaped = at != afterPoint;
    }
    while (std::isspace(static_cast<unsigned char>(*at)) != 0) {
      ++at;
    }
    points.push_back(std::strtod(begin, nullptr));
    const double point = points.back();
    if (!shaped || !hasInteger || (*at != '\0' && *at != ',') || point <= 0.0
        || point >= 100.0 || points.size() > kMaxPercentilePoints)
    {
      rejectPercentileList(option, text);
    }
    more = *at == ',';
    if (more) {
      ++at;
    }
  }
}

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

// The batch filter pattern of FR-024 and FR-050, compiled once for either
// path to read: the expression the option names, no pattern when the option
// stands absent, or one text naming the expression the regex library refused
// (FR-051).
auto compileFilterPattern(const std::string& pattern)
    -> std::expected<std::optional<std::regex>, std::string>
{
  if (pattern.empty()) {
    return std::optional<std::regex> {};
  }
  try {
    return std::optional<std::regex>(pattern);
  } catch (const std::regex_error& error) {
    return std::unexpected("invalid regular expression '" + pattern
                           + "': " + error.what());
  }
}

// specs/024 FR-023, FR-026, research D-09: the word of `--metric-method`, read
// against the three words of contracts/metric-method-option.md. The comparison
// is exact and case-sensitive, so `Auto` is a fourth word, and the reader
// answers with nothing for one of those: the entry point owns the report, so
// that a refusal returns the usage status and the entry point keeps the
// process exit.
auto parseMetricMethod(const char* text) -> std::optional<MetricMethod>
{
  const std::string_view word {text};
  if (word == "auto") {
    return MetricMethod::AUTO;
  }
  if (word == "passes") {
    return MetricMethod::PASSES;
  }
  if (word == "multiplex") {
    return MetricMethod::MULTIPLEX;
  }
  return std::nullopt;
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

// FR-027: one batch snapshot for the process. `speedgunMain` writes it
// at its entry, and a run outside `speedgunMain` reads it empty and
// falls back to the process default of its own start.
auto batchDefaultTimeUnit() -> std::optional<TimeUnit>&
{
  static std::optional<TimeUnit> batch;
  return batch;
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

// FR-028, FR-042: a metric name the host's table carries nothing for ends the
// run before any measurement starts, so a typo costs no measured time. The
// walk is in option order and states the first failure, with its near-miss
// names on the same line.
[[nodiscard]] auto metricNameFailureLine(
    const std::string& name, const counters::Error& error) -> std::string
{
  std::string line = "--metric " + name + ": " + error.message;
  for (std::size_t here = 0; here < error.suggestions.size(); ++here) {
    line += here == 0 ? " (near: " : ", ";
    line += error.suggestions[here];
  }
  if (!error.suggestions.empty()) {
    line += ")";
  }
  return line;
}

auto metricNameFailure(const std::vector<std::string>& names) -> int
{
  const std::vector<counters::MetricListing> listed =
      counters::catalogMetrics();
  for (const auto& name : names) {
    const bool carried = std::any_of(listed.begin(),
                                     listed.end(),
                                     [&](const counters::MetricListing& row)
                                     { return row.name == name; });
    if (carried) {
      continue;
    }
    const auto miss = counters::metricByName(name);
    SG_ASSERT(!miss.has_value(),
              "a name the catalog lists nothing for is a miss the lookup "
              "answers with an error (FR-016)");
    std::fprintf(
        stderr, "%s\n", metricNameFailureLine(name, miss.error()).c_str());
    return 2;
  }
  return 0;
}

}  // namespace detail

/**
 * @brief Run the benchmark suite the command line names (FR-040).
 *
 * @pre none
 * @post the batch snapshot of the process default unit stands taken at
 *       this entry, the suite ran, and the report printed (FR-027)
 */
auto speedgunMain(int argc, char** argv) -> int
{
  auto& sys = counters::System::local();

  // FR-027: the entry point snapshots the process default once per
  // batch, so a setDefaultTimeUnit call in `main` reaches every
  // registration that names no unit, the macro registrations included.
  detail::batchDefaultTimeUnit() = getDefaultTimeUnit();

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
  // The scanner carries no short letters, so the long-only option answers a
  // value outside the letter range: no option a later feature adds can reach
  // the same switch arm by claiming a letter.
  constexpr int kMetricMethodOption = 256;
  static option longOptions[] = {
      {"filter", required_argument, nullptr, 'f'},
      {"list", no_argument, nullptr, 'l'},
      {"repetitions", required_argument, nullptr, 'r'},
      {"min-time", required_argument, nullptr, 'm'},
      {"iterations", required_argument, nullptr, 'i'},
      {"warmup-time", required_argument, nullptr, 'w'},
      {"dry-run", no_argument, nullptr, 'd'},
      {"counter", required_argument, nullptr, 'c'},
      {"metric", required_argument, nullptr, 'M'},
      {"catalog", no_argument, nullptr, 'C'},
      {"percentiles", required_argument, nullptr, 'p'},
      {"no-percentiles", no_argument, nullptr, 'P'},
      {"placement", required_argument, nullptr, 'A'},
      {"topology", no_argument, nullptr, 'T'},
      {"json", required_argument, nullptr, 'j'},
      {"json-raw", required_argument, nullptr, 'J'},
      // specs/026 FR-048, FR-049, FR-045: the three options of the
      // regression gate. The mode stands off until `--compare` names a
      // baseline.
      {"compare", required_argument, nullptr, kCompareOptionValue},
      {"compare-rule", required_argument, nullptr, kCompareRuleOptionValue},
      {"compare-strict", no_argument, nullptr, kCompareStrictOptionValue},
      {"metric-method", required_argument, nullptr, kMetricMethodOption},
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
      case 'M':
        options.metricNames.emplace_back(optarg);
        break;
      case 'C':
        options.catalogMode = true;
        break;
      case 'p':
        // specs/021 FR-020, FR-021: the command line wins over the family.
        options.showPercentiles = true;
        parsePercentileList(optarg, "--percentiles", options.percentilePoints);
        break;
      case 'P':
        options.showPercentiles = false;
        break;
      case 'A':
        // specs/025 FR-016: the text stands raw here; the parse below
        // runs it once before the first run.
        options.placement = std::string(optarg);
        break;
      case 'T':
        options.topologyMode = true;
        break;
      case 'j':
        // FR-030: an empty path is a usage error, and it reports before
        // anything runs.
        if (std::string_view(optarg).empty()) {
          std::fprintf(stderr,
                       "--json: the path the option names stands " "empty\n");
          return 2;
        }
        options.jsonPath = optarg;
        break;
      case 'J':
        // FR-029: `chunks` is the one content the option names besides
        // `none`; any other word is a usage error (FR-034).
        if (std::string_view(optarg) == "chunks") {
          options.rawContent = RawContent::CHUNKS;
        } else if (std::string_view(optarg) == "none") {
          options.rawContent = RawContent::NONE;
        } else {
          std::fprintf(stderr, "--json-raw: 'chunks' and 'none' are the "
                               "values the option names\n");
          return 2;
        }
        break;
      case kMetricMethodOption: {
        // specs/024 FR-025, FR-026: the word is the method the run is asked
        // for and wins over the family's own request the way `--percentiles`
        // wins over `showPercentiles`; the assignment overwrites, so a
        // repeated flag keeps its last word. A word outside the three ends the
        // run here, before the expansion, the listing and the first
        // measurement, so a typo costs no measured time.
        const auto method = parseMetricMethod(optarg);
        if (!method.has_value()) {
          std::fprintf(
              stderr,
              "--metric-method '%s' is not auto, passes or multiplex\n",
              optarg);
          return 2;
        }
        options.metricMethod = method;
        break;
      }
      case kCompareOptionValue: {
        // specs/026 FR-048: the text is the two report paths split at the
        // first comma. A missing comma or an empty side is a usage fault:
        // one line naming the option and the text read, status 2 (FR-051).
        const std::string_view pair = optarg;
        const std::size_t comma = pair.find(',');
        if (comma == std::string_view::npos || comma == 0
            || comma + 1 == pair.size())
        {
          std::fprintf(
              stderr,
              "--compare: expected '<baseline>,<contender>', got " "'%s'\n",
              optarg);
          return 2;
        }
        options.compareBaseline = std::string(pair.substr(0, comma));
        options.compareContender = std::string(pair.substr(comma + 1));
        break;
      }
      case kCompareRuleOptionValue: {
        // specs/026 FR-049: one rule per option, in the order given. The
        // parse call names the field of its fault and the text it read, so
        // the line below adds the option and nothing else (FR-051).
        const auto rule = parseCompareRule(optarg);
        if (!rule.has_value()) {
          std::fprintf(stderr, "--compare-rule: %s\n", rule.error().c_str());
          return 2;
        }
        options.compareRules.push_back(std::move(*rule));
        break;
      }
      case kCompareStrictOptionValue:
        // specs/026 FR-045: a NOISY row fails the run; the state word
        // stands as printed.
        options.compareStrict = true;
        break;
      default:
        std::fprintf(stderr, "unknown option; see the option table of "
                             "specs/015-benchmark-harness-core/contracts/"
                             "cli.md\n");
        return 2;
    }
  }

  // specs/026 FR-024, FR-050, FR-051: the batch filter is compiled while
  // the scan is all that has happened, so an invalid pattern answers one
  // line and status 2 on the compare path beside the run path, and the
  // pattern is compiled once for either path to read.
  const auto compiled = compileFilterPattern(options.filter);
  if (!compiled.has_value()) {
    std::fprintf(stderr, "--filter: %s\n", compiled.error().c_str());
    return 2;
  }
  std::optional<std::regex> filterPattern = *compiled;

  // specs/026 FR-048, FR-052, D-15: the compare mode stands ahead of the
  // expansion, so the comparison runs no benchmark, reads no clock and
  // samples no sensor (FR-007, SC-005).
  if (!options.compareBaseline.empty()) {
    const auto exit = detail::runCompareMode(options);
    SG_ENSURE(
        (exit.status != 0) == exit.failed,
        "the compare status follows the table of contracts/cli.md " "(FR-052)");
    return exit.status;
  }

  // specs/025 FR-016, E-05: the process preset name is parsed once here,
  // before the expansion and before any run, so a rejected text stops
  // the batch with one line and status 2.
  std::optional<detail::Preset> processPreset;
  if (options.placement.has_value()) {
    const auto parsed = detail::parsePreset(*options.placement);
    if (!parsed.has_value()) {
      std::fprintf(stderr,
                   "--placement: '%s' is not one of the five preset names\n",
                   options.placement->c_str());
      return 2;
    }
    processPreset = parsed;
  }

  if (options.catalogMode) {
    detail::printCatalog();
    return 0;
  }

  if (options.topologyMode) {
    // FR-036, Q-3, R-07: the listing answers the layout read once, runs
    // no benchmark and exits 0; an unreadable layout prints one line and
    // exits 2. The listing stands ahead of the expansion and the filter,
    // the way the catalog block above does.
    const auto& machine = detail::layout();
    if (!machine.readable) {
      std::fprintf(
          stderr,
          "--topology: the harness cannot read the machine " "layout\n");
      return 2;
    }
    detail::printTopologyListing(machine);
    return 0;
  }

  // FR-003, R-03: expansion completes here, before the filter and
  // before the first run, so a run carries no expansion work.
  detail::expandRegistry(processPreset);

  std::vector<Instance*> selected;
  for (auto& instance : detail::instances()) {
    if (instance.name.starts_with(kDisabledPrefix)) {
      continue;
    }
    if (!filterPattern || std::regex_search(instance.name, *filterPattern)) {
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

  // FR-028, FR-042: an unknown metric name ends the run before any
  // measurement starts, before a lookup of its own (FR-016).
  if (!options.metricNames.empty()) {
    const int nameStatus = detail::metricNameFailure(options.metricNames);
    if (nameStatus != 0) {
      return nameStatus;
    }
  }

  // specs/025 FR-005, FR-018, Q-6, Q-7: the fit walk, before the first
  // run and before any measurement starts.
  const auto fitOutcome = detail::placementFitStatus(selected);
  if (fitOutcome.first != 0) {
    return fitOutcome.first;
  }

  // One entry-point invocation owns one interrupt flag: a suite that
  // calls the entry point again starts with no pending interrupt.
  detail::interruptFlag().store(false);
  SG_ENSURE(!detail::interruptFlag().load(),
            "the entry point starts with no pending interrupt (R-06)");
  std::signal(SIGINT, handleInterrupt);
  detail::printContext(fitOutcome.second);

  int status = 0;
  // The two run-wide flags are read by the postcondition below, which the
  // default semantic compiles away, so a consumer build needs the marker.
  [[maybe_unused]] bool anyFailed = false;
  [[maybe_unused]] bool anyInterrupted = false;
  // specs/023 FR-001: the values of the batch gather beside the rows the
  // console prints, and the document writer reads no clock and samples
  // nothing on its way to the file (FR-005).
  ReportData report;
  detail::Runner runner(options);
  // The loop variable keeps the spelling `entry`: `test/cli_shape.sh`
  // locates this loop by the `runner.run(*entry)` line and reads the
  // flag exactly once inside it.
  for (auto* entry : selected) {
    // FR-022, FR-023: the complexity fit of the family that ended prints
    // between the rows of two families, after every row of the family it
    // summarizes.
    if (const auto fit = runner.fitForFamilyEnd(entry)) {
      detail::printResult(*fit);
      // FR-002: a fit stands as its own entry of the document, beside
      // the entries of the runs it summarizes.
      report.results.push_back(*fit);
      report.rowRaw.emplace_back();
      report.contextRaw.emplace_back();
    }
    const auto result = runner.run(*entry);
    detail::printResult(result);
    report.results.push_back(result);
    report.rowRaw.push_back(runner.rowRaw().back());
    report.contextRaw.push_back(runner.contextRaw().back());
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

  // FR-022: the run is over, so the fit of the last family prints after its
  // rows.
  if (const auto fit = runner.fitForFamilyEnd(nullptr)) {
    detail::printResult(*fit);
    report.results.push_back(*fit);
    report.rowRaw.emplace_back();
    report.contextRaw.emplace_back();
  }

  // FR-030: a document the file system refused reports on stderr and
  // leaves the exit status at the usage error the table names, which
  // outranks the 1 a failed benchmark leaves.
  [[maybe_unused]] bool reportFailed = false;
  if (!options.jsonPath.empty()) {
    if (!detail::writeReport(options.jsonPath, report)) {
      reportFailed = true;
      status = 2;
    }
  }

  SG_ENSURE((status != 0) == (anyFailed || anyInterrupted || reportFailed),
            "the exit status follows the table of contracts/cli.md (FR-036)");

  return status;
}

}  // namespace sg
