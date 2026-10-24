#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <expected>
#include <memory>
#include <numeric>
#include <optional>
#include <regex>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <sched.h>
#include <unistd.h>

#include "detail/cli_options.hpp"
#include "detail/compare.hpp"
#include "detail/internal.hpp"
#include "detail/jitter_probe.hpp"
#include "detail/json_report.hpp"
#include "detail/options.hpp"
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

// FR-015's run-not-started pre: the predicate the suite-level probe
// binding requires, named beside the other helpers so the contract
// macro carries no lambda (the lane's review rule).
[[maybe_unused]] auto noRunStarted() -> bool
{
  return std::ranges::none_of(
      registry(), [](const auto& entry) { return entry->runStarted; });
}

// FR-020, R-11: the prefix of a disabled instance. The gate is a prefix
// test on the expanded instance name, and selection is where it stands.
constexpr std::string_view kDisabledPrefix = "DISABLED_";

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

// FR-047: the cpu count the process affinity allows, read for the one line
// that discloses a count standing above it. The mask stands zeroed before
// the call and a call the system refuses leaves it as it stands, so the
// count of a refused call is zero all by itself and the line stands only for
// a run that asked a count of its own.
auto affinityCpuCount() -> int
{
  cpu_set_t affinity {};
  CPU_ZERO(&affinity);
  (void)sched_getaffinity(0, sizeof(affinity), &affinity);
  return CPU_COUNT(&affinity);
}

// FR-039, FR-040, FR-042: the console line of the settled sources. Every
// entry names its option in the `--<name>=<value>` form the option table
// spells, the entries join with `; `, and the tail carries the seed of a run
// that interleaved. No entry and no seed prints nothing at all, which is the
// run that took all of its options off the command line (FR-041). The
// document of the same run carries these facts in its `run` object.
auto printOptionDisclosure(const std::vector<detail::OptionUse>& uses,
                           const detail::InterleaveSeed& interleave) -> void
{
  std::vector<std::string> parts;
  for (const detail::OptionUse& use : uses) {
    parts.push_back("--" + std::string(use.row->name) + "=" + use.value);
  }
  if (interleave.given) {
    parts.push_back("interleave=" + std::to_string(interleave.seed));
  }
  if (parts.empty()) {
    return;
  }
  std::string line = "options from a file or the environment: ";
  for (std::size_t index = 0; index < parts.size(); ++index) {
    if (index != 0) {
      line += "; ";
    }
    line += parts[index];
  }
  std::printf("%s\n", line.c_str());
}

}  // namespace

namespace detail
{
namespace
{

// R-06: one flag for the process. The signal handler sets it, and the
// runner reads it after each run.
constinit std::atomic<bool> interruptFlagValue {false};

// specs/028 FR-019, D-2: the prefix of every variable the harness reads.
// The gather computes each full name from its option row, so this is the
// one place the prefix is spelled for the process side of the seam.
constexpr std::string_view kEnvironmentPrefix = "SPEEDGUN_";

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

// FR-015: the suite-level binding `runJitterProbe` writes. The batch
// reads it where the command line named none, and the command-line
// value wins when both stand (FR-015, R-01).
auto suiteJitterProbe() -> std::optional<JitterProbeOptions>&
{
  static std::optional<JitterProbeOptions> binding;
  return binding;
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

// The row one entry of the process environment carries, or no row when the
// entry holds no `=` or names nothing the `SPEEDGUN_` prefix covers. The
// split of one entry stands apart from the walk of the array so the shape of
// a single entry is the seam a test reads (FR-021, FR-026).
auto environmentRowOf(std::string_view entry) -> std::optional<EnvironmentRow>
{
  const std::size_t cut = entry.find('=');
  if (cut == std::string_view::npos) {
    return std::nullopt;
  }
  const std::string_view name = entry.substr(0, cut);
  if (!name.starts_with(kEnvironmentPrefix)) {
    return std::nullopt;
  }
  return EnvironmentRow {std::string(name), std::string(entry.substr(cut + 1))};
}

// specs/028 FR-019, FR-026, D-04: the process environment reaches the
// gather as data, so the table is read once here at the entry point and
// nowhere else in the library. The rows carry the `SPEEDGUN_*` names with
// the text each holds, and a name no flagged row claims travels along to
// be ignored by the gather (FR-026). The table arrives as a parameter, so
// the walk takes a table the caller holds: a null table holds no entry and
// answers no row, the way an empty one does (FR-012). The sources test
// reads the split of one entry through `environmentRowOf` and calls this
// walk against a table it builds, against a null table and against the
// running process, while the end-to-end inheritance check execs a built
// binary with an explicit environment array.
auto environmentRows(char* const* table) -> std::vector<EnvironmentRow>
{
  std::vector<EnvironmentRow> rows;
  if (table == nullptr) {
    return rows;
  }
  // The platform hands every process a NULL-terminated array, so the walk
  // below is pointer arithmetic over the table it was handed.
  // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  for (char* const* entry = table; *entry != nullptr; ++entry) {
    if (const auto row = environmentRowOf(*entry)) {
      rows.push_back(*row);
    }
  }
  // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  return rows;
}

}  // namespace detail

/**
 * @brief State the jitter probe options of the suite's next run
 * (FR-015).
 *
 * The options bind the suite the way the command-line option does.
 * The availability rule of FR-037 binds the probe the run starts,
 * whatever entry point configured it: the run discloses `not
 * available` and continues with status 0, and `strict` stops it with
 * status 2.
 *
 * \pre the run has not started for any benchmark (E-02, FR-015)
 * \pre `options.cpus` carries the probe cpu request: an empty vector
 *      names the `idle` preset, and a non-empty vector names cpus
 *      inside the process affinity mask (FR-011, FR-016, D-4)
 * \pre `options` carries a `thresholdNs` value at or above 1 when the
 *      field carries one, `gapCount` inside the `kMinGapCount` through
 *      `kMaxGapCount` range, and `precheckMs` stands at or above 1
 *      (FR-008, FR-009, D-13)
 * \post the suite's next run starts the probe before its first
 *       selected instance and stops, drains and joins it after the
 *       last, over these options (FR-006, R-01)
 * \post the command-line value wins when both stand (FR-015, R-01)
 */
auto runJitterProbe(const JitterProbeOptions& options) -> void
{
  // The field bounds the contract states; the command line enforces
  // the same bounds where it parses (FR-008, FR-009, D-13).
  SG_REQUIRE(!options.thresholdNs.has_value() || *options.thresholdNs >= 1,
             "the threshold stands at or above 1 ns (FR-008, D-13)");
  SG_REQUIRE(
      options.gapCount >= kMinGapCount && options.gapCount <= kMaxGapCount,
      "the gap count stands inside the bounded range (FR-009, D-13)");
  SG_REQUIRE(options.precheckMs >= 1,
             "the pre-check duration stands at or above 1 ms (FR-008, D-13)");
  SG_REQUIRE(noRunStarted(),
             "the run has not started for any benchmark (E-02, FR-015)");
  detail::suiteJitterProbe() = options;
  SG_ENSURE(detail::suiteJitterProbe().has_value(),
            "the suite carries the options of its next run (FR-015)");
}

// D-5: the mask stands restored after the instance ends. The restore
// applies the saved mask the read returned, so the call stands
// answered on every path a test drives; a host that refuses it breaks
// the contract the assertion owns, the way the pin's UNAPPLIED
// account names the miss (specs/025 FR-005).
auto restoreSavedMask(const cpu_set_t& saved) noexcept -> void
{
  [[maybe_unused]] const bool restored =
      sched_setaffinity(0, sizeof(saved), &saved) == 0;
  SG_ASSERT(
      restored,
      "the affinity mask stands restored after the instance " "ends (D-5)");
}

// D-5: the narrowed mask the run carries is a subset of the mask the
// same thread read from the kernel a moment earlier, so the write
// answers success the way the read did; a host that refuses it breaks
// the contract the assertion owns, the way the restore above does.
auto applyNarrowedMask(const cpu_set_t& narrowed) noexcept -> void
{
  [[maybe_unused]] const bool applied =
      sched_setaffinity(0, sizeof(narrowed), &narrowed) == 0;
  SG_ASSERT(applied,
            "the affinity mask stands narrowed for the instance's run (D-5)");
}

// E-07: the record the document carries, built from the joined states
// the fold reads (R-04). The record block is one unit of the report,
// and the entry point below stands more than seven hundred lines, so
// the build stands in a function of its own.
auto buildJitterReportRecord(const JitterProbeOptions& probe,
                             bool unavailable,
                             bool rawGaps,
                             const std::string& placementTail,
                             const std::vector<const detail::ProbeCpuState*>&
                                 states) -> detail::JitterReportRecord
{
  detail::JitterReportRecord record;
  record.mode = probe.mode;
  if (unavailable) {
    // E-07: the unavailable answer carries the requested mode, the
    // configured threshold, no measured period, and the disclosure;
    // the run continues with status 0 (FR-037, D-10). No period
    // stands measured, so the record carries a threshold value, the
    // 200 ns default when the option named none.
    record.thresholdNs = probe.thresholdNs.value_or(kDefaultThresholdNs);
    record.disclosure = "not available" + placementTail;
    return record;
  }
  // E-07, E-09: the document sorts the cpus ascending, the one
  // order the disclosure text carries.
  std::vector<const detail::ProbeCpuState*> ordered(states.begin(),
                                                    states.end());
  std::ranges::sort(
      ordered,
      [](const detail::ProbeCpuState* left, const detail::ProbeCpuState* right)
      { return left->cpu < right->cpu; });
  std::string cpuList;
  for (const auto* state : ordered) {
    if (!cpuList.empty()) {
      cpuList.push_back(',');
    }
    cpuList += std::to_string(state->cpu);
  }
  record.disclosure = "probe on cpus " + cpuList + "; loop period "
      + std::to_string(ordered.front()->loopPeriodNs) + " ns, threshold "
      + std::to_string(ordered.front()->thresholdNs) + " ns";
  // E-07: the record carries the measured points the disclosure
  // names; every probe cpu's fold carries the same pair (FR-007).
  record.thresholdNs = ordered.front()->thresholdNs;
  record.loopPeriodNs = ordered.front()->loopPeriodNs;
  for (const auto* state : ordered) {
    record.disclosure += "; cpu " + std::to_string(state->cpu) + " kept "
        + std::to_string(state->kept) + " dropped "
        + std::to_string(state->droppedGaps);
  }
  record.disclosure += "; split and percentiles use the kept gaps only";
  record.disclosure += placementTail;
  // FR-028: the gap list rides the `--json-raw=gaps` value alone;
  // the list holds the kept records only (D-6).
  record.rawGaps = rawGaps;
  for (const auto* state : ordered) {
    record.cpus.push_back(state->cpu);
    detail::JitterReportCpu entry;
    entry.cpu = state->cpu;
    entry.stats = detail::gapStatsOf(*state);
    if (record.rawGaps) {
      entry.gaps.assign(
          state->buffer.begin(),
          state->buffer.begin() + static_cast<std::ptrdiff_t>(state->kept));
    }
    record.stats.push_back(std::move(entry));
  }
  for (const auto* state : ordered) {
    record.droppedGaps += state->droppedGaps;
  }
  return record;
}

// D-5: the narrowing reads the mask the run already answered once -
// the value's parse or the plan's resolution read it first, and both
// stop the run beside their refusal - so the read stands answered the
// way those reads did; a host that refuses it breaks the contract the
// assertion owns, the way the narrowed write and the restore do. A
// filter a benchmark's own context installs stands on that context's
// thread and never reaches the calling thread the narrowing runs on.
auto readNarrowingMask(cpu_set_t& saved) noexcept -> void
{
  [[maybe_unused]] const bool readable =
      sched_getaffinity(0, sizeof(saved), &saved) == 0;
  SG_ASSERT(readable,
            "the narrowing reads the mask it narrows for the instance (D-5)");
}

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

  // specs/028 FR-002, FR-010, FR-011, SC-013: the scan stands behind
  // its own seam and reads the one option table, so the entry point
  // holds no option list of its own. It applies what the scan answers,
  // and the fault the scan hands back is one line on standard error at
  // status 2, which is how the entry point ends the run.
  auto scan = detail::scanCommandLine(argc, argv);
  if (!scan.has_value()) {
    std::fprintf(stderr, "%s\n", scan.error().c_str());
    return 2;
  }

  // specs/028 FR-017, FR-018: the scan records the first of `--help`,
  // `--version` or a usage fault that its walk meets, so the word that
  // stands first is the word that answers. Both print options write on
  // standard output and end the run at status 0, ahead of the filter
  // compile, the placement parse and the run loop.
  if (scan->stop == detail::StopKind::HELP) {
    detail::printHelp();
    return 0;
  }
  if (scan->stop == detail::StopKind::VERSION) {
    detail::printVersion();
    return 0;
  }

  // specs/028 FR-019 to FR-028, FR-037, SC-002: the environment reaches
  // the run through the gather, which settles every option at the value
  // of the highest source that named it. The gather reads no process
  // environment and opens no file, so the rows are read once here (D-04,
  // D-05). The fault the gather hands back is one line on standard error at
  // status 2, the shape the scan fault above uses, and it stands ahead of
  // the filter compile, the placement parse and the run loop (SC-003).
  std::vector<detail::EnvironmentRow> environment;
  if (!scan->options.environmentOff) {
    environment = detail::environmentRows(environ);
  }

  // specs/028 FR-029, FR-034, D-05: the entry point owns the read of the
  // option file and hands the gather the text as data. A file the system
  // refused is one line naming the path and the reason the open answered,
  // at status 2, ahead of the filter compile, the placement parse and the
  // run loop (SC-003, SC-013).
  std::optional<detail::OptionFileText> optionFile;
  if (!scan->options.optionsFile.empty()) {
    const auto text = detail::readTextFile(scan->options.optionsFile);
    if (!text.has_value()) {
      std::fprintf(stderr, "--options: %s\n", text.error().c_str());
      return 2;
    }
    optionFile = detail::OptionFileText {
        scan->options.optionsFile,
        std::move(*text),
    };
  }

  const auto resolved = detail::gatherOptionSources(
      *scan, environment, optionFile, &detail::readCompareRulesFile);
  if (!resolved.has_value()) {
    std::fprintf(stderr, "%s\n", resolved.error().c_str());
    return 2;
  }

  detail::RunOptions options = std::move(resolved->options);

  // FR-015: the gather pass parses the option file's value where the
  // file settles it and leaves the command line's own text for the
  // parse below, so a jitter-probe value that stands settled at this
  // point came from the option file: no environment row names this
  // option. The flag records that fact, and the family-probe build
  // reads it (FR-015, R-06).
  const bool jitterProbeFromOptionFile = options.jitterProbe.has_value();

  // specs/028 FR-063, spec D-23: the compare mode reads the settled list,
  // which the highest source that names rules sets in the order spec D-23
  // states, so a rule of the words or of a file can stand here (FR-061).
  options.compareRules = std::move(resolved->rules);

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

  // FR-011, FR-038, R-06: the command line's text parses once here,
  // before the expansion and before any run; every rejection stands one
  // line and exits 2. The option file's value parsed where it settled,
  // so the text stands only for the command line's own (FR-027).
  if (options.jitterProbeText.has_value()) {
    auto parsed = JitterProbeOptions {};
    std::string message;
    if (!detail::parseJitterProbeValue(
            *options.jitterProbeText, parsed, message))
    {
      std::fprintf(stderr, "--jitter-probe: %s\n", message.c_str());
      return 2;
    }
    options.jitterProbe = parsed;
  }

  // FR-015: the option sources stand in order: the command line's
  // value, then the option file's, then the suite-level call's; the
  // call answers only where no higher source named the probe.
  if (!options.jitterProbe.has_value()) {
    options.jitterProbe = detail::suiteJitterProbe();
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

  // specs/028 FR-044, FR-048, FR-050, SC-004, D-14: the settled counts
  // reach every family record here, ahead of the expansion that crosses
  // the argument lists with the counts at `family.cpp:404` and `:425`, so
  // the option applies whatever thread call the family made and the names
  // follow the master rule at `family.cpp:246`: no `/threads:` segment at
  // one count, the segment above one. An empty list means no source named
  // the option, and the registered counts stand. The loop runs once per
  // family before any measurement, far outside the timed run loop.
  if (!options.threadCounts.empty()) {
    for (auto& entry : registry()) {
      entry->threadCounts = options.threadCounts;
    }
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

  // FR-047: a count above the affinity count runs, and the console discloses
  // it in one line naming the option and the count. The line stands below
  // the version line of the context block and above the first row.
  int widest = 0;
  for (const int count : options.threadCounts) {
    widest = std::max(widest, count);
  }
  if (widest > affinityCpuCount()) {
    std::printf("--threads=%d stands above the %d cpus the affinity allows\n",
                widest,
                affinityCpuCount());
  }

  // FR-039, FR-040, FR-042: the settled sources disclose themselves here,
  // after the version line of the context block and before the first row of
  // the run.
  printOptionDisclosure(resolved->uses, resolved->interleave);

  int status = 0;
  // The two run-wide flags are read by the postcondition below, which the
  // default semantic compiles away, so a consumer build needs the marker.
  [[maybe_unused]] bool anyFailed = false;
  [[maybe_unused]] bool anyInterrupted = false;
  // specs/023 FR-001: the values of the batch gather beside the rows the
  // console prints, and the document writer reads no clock and samples
  // nothing on its way to the file (FR-005).
  ReportData report;

  // FR-015: the per-family probes the handle named. The option file's
  // value stands over the handle's, so a family runs its own probe
  // only when no option file named the option; the handle's value
  // stands over the command line's for that family (FR-015, R-06).
  // R-01 holds one probe session on the process at a time, so a
  // family's own probe starts before its first instance in the walk
  // and stops after its last, and the batch's own probe stands down
  // for the run beside one (FR-006, R-01); the walk's spans hand the
  // session over at each family's first and last instance, outside
  // the timed windows.
  struct FamilyProbe
  {
    const RegistryEntry* family;
    sg::JitterProbeOptions options;
    std::vector<std::uint32_t> cpus;
    std::optional<detail::JitterProbe> holder;
    bool unavailable = false;
    bool started = false;
    bool joined = false;
    std::size_t first = 0;
    std::size_t last = 0;
  };

  std::vector<FamilyProbe> familyProbes;
  if (!jitterProbeFromOptionFile) {
    for (std::size_t position = 0; position < selected.size(); ++position) {
      auto* entry = selected[position];
      if (!entry->family->jitterProbeOptions.has_value()) {
        continue;
      }
      auto found = std::find_if(familyProbes.begin(),
                                familyProbes.end(),
                                [&](const FamilyProbe& probe)
                                { return probe.family == entry->family; });
      if (found == familyProbes.end()) {
        familyProbes.push_back(FamilyProbe {entry->family,
                                            *entry->family->jitterProbeOptions,
                                            {},
                                            {},
                                            false,
                                            false,
                                            false,
                                            position,
                                            position});
      } else {
        found->last = position;
      }
    }
  }
  // The probe holder rides the batch state: the lifecycle starts before
  // the first run and stops after the selected loop ends (T010, R-01).
  // A family the handle named holds the session beside its own probe,
  // so the batch's stands down while one runs (FR-015, R-01).
  std::optional<detail::JitterProbe> jitterProbe;
  bool jitterUnavailable = false;
  // specs/027 E-09: the disclosure facts the placement safety gathers
  // before and during the batch (FR-017, FR-036, D-5, D-12, R-08).
  std::vector<std::uint32_t> probeCpus;
  std::vector<std::uint32_t> skippedGroups;
  std::vector<std::string> maskNarrowedNames;
  bool cpusetLimited = false;
  // FR-015: the batch's probe stands only when no family carries its
  // own, the one session the process holds (R-01) running the family's
  // own probe beside its instances.
  const bool batchProbed =
      options.jitterProbe.has_value() && familyProbes.empty();
  if (batchProbed) {
    probeCpus = options.jitterProbe->cpus;
    if (probeCpus.empty()) {
      // D-4, R-07: the idle resolution runs against the selected
      // instances' plans before the batch starts.
      std::vector<std::uint32_t> planned =
          detail::plannedCpusOf(selected, detail::layout());
      if (options.jitterProbe->mode == JitterProbeMode::PRECHECK) {
        // FR-006, R-10: no probe thread runs beside a benchmark in the
        // precheck mode - start() returns only after every pre-check
        // phase ends - so the planned cpus stand as the probe's own and
        // no group stands skipped (D-4, R-07).
        std::ranges::sort(planned);
        planned.erase(std::unique(planned.begin(), planned.end()),
                      planned.end());
        probeCpus = std::move(planned);
      } else {
        const auto idle = detail::resolveIdleCpus(detail::layout(), planned);
        probeCpus = idle.cpus;
        skippedGroups = idle.skippedGroups;
      }
      if (probeCpus.empty()) {
        // FR-017: no free cpu at all stops the batch with one line
        // naming the preset word, before any benchmark runs. The two
        // causes stand apart: a precheck batch with no pinned instance
        // plans no cpu at all, and a batch whose plans leave no group
        // with a free cpu resolves none.
        if (options.jitterProbe->mode == JitterProbeMode::PRECHECK) {
          std::fprintf(
              stderr,
              "--jitter-probe: no cpu is planned for the idle preset\n");
        } else {
          std::fprintf(stderr,
                       "--jitter-probe: the idle preset names no free cpu\n");
        }
        return 2;
      }
    }
    sg::JitterProbeOptions probeOptions = *options.jitterProbe;
    probeOptions.cpus = probeCpus;
    jitterProbe.emplace(probeOptions);
    jitterProbe->start();
    if (jitterProbe->states().empty()) {
      // The availability answer (Q-1): strict stops the batch before
      // any run, and the plain run discloses and continues (FR-037,
      // D-10).
      if (options.jitterProbe->strict) {
        std::fprintf(
            stderr,
            "--jitter-probe: the machine/tsc leaf is not " "available\n");
        return 2;
      }
      jitterUnavailable = true;
    } else {
      // FR-018, D-3, R-10: the overlap check binds the alongside
      // phase; the precheck mode allows every overlap, and in `both`
      // the pre-check phase began with the start, before the stop
      // lands.
      const auto overlap = detail::probeOverlapOf(
          detail::layout(), selected, probeCpus, options.jitterProbe->mode);
      if (overlap.has_value()) {
        std::fprintf(stderr,
                     "--jitter-probe: probe cpu %u overlaps the planned "
                     "cpus of '%s'\n",
                     overlap->cpu,
                     overlap->benchmark.c_str());
        jitterProbe->stopAndJoin();
        return 2;
      }
    }
  }
  // FR-015: the cpus each family's own probe resolves, and the overlap
  // check over the family's own plans, before any run (FR-018, FR-037,
  // R-10); the start lands in the walk, before the family's first
  // instance (FR-006, R-01).
  for (auto& probe : familyProbes) {
    std::vector<Instance*> familyInstances;
    for (auto* entry : selected) {
      if (entry->family == probe.family) {
        familyInstances.push_back(entry);
      }
    }
    std::vector<std::uint32_t> planned =
        detail::plannedCpusOf(familyInstances, detail::layout());
    if (probe.options.cpus.empty()) {
      if (probe.options.mode == JitterProbeMode::PRECHECK) {
        // FR-006, R-10: the precheck mode plans no probe thread beside
        // a benchmark, so the planned cpus stand as the probe's own.
        std::ranges::sort(planned);
        planned.erase(std::unique(planned.begin(), planned.end()),
                      planned.end());
        probe.cpus = std::move(planned);
      } else {
        const auto idle = detail::resolveIdleCpus(detail::layout(), planned);
        probe.cpus = idle.cpus;
        skippedGroups.insert(skippedGroups.end(),
                             idle.skippedGroups.begin(),
                             idle.skippedGroups.end());
      }
      if (probe.cpus.empty()) {
        if (probe.options.mode == JitterProbeMode::PRECHECK) {
          std::fprintf(
              stderr,
              "--jitter-probe: no cpu is planned for the idle preset\n");
        } else {
          std::fprintf(stderr,
                       "--jitter-probe: the idle preset names no free cpu\n");
        }
        return 2;
      }
    } else {
      probe.cpus = probe.options.cpus;
    }
    // FR-018, D-3, R-10: the overlap check binds the alongside phase
    // over the family's own plans before any run; the precheck mode
    // allows every overlap, and in `both` the pre-check phase begins
    // with the start.
    if (probe.options.mode != JitterProbeMode::PRECHECK) {
      const auto overlap = detail::probeOverlapOf(
          detail::layout(), familyInstances, probe.cpus, probe.options.mode);
      if (overlap.has_value()) {
        std::fprintf(stderr,
                     "--jitter-probe: probe cpu %u overlaps the planned "
                     "cpus of '%s'\n",
                     overlap->cpu,
                     overlap->benchmark.c_str());
        return 2;
      }
    }
  }

  // FR-019, D-5, R-10: the empty-intersection refusal stands before
  // the walk, next to the overlap check the batch probe runs: the
  // process mask and the probe cpus are known before any run, so an
  // entry whose mask the probe cpus empty stops the batch before the
  // first row prints (FR-036). A family the handle named runs its own
  // probe's narrowing in the walk, where its availability settles.
  if (batchProbed && !jitterUnavailable
      && options.jitterProbe->mode != JitterProbeMode::PRECHECK)
  {
    cpu_set_t walkMask {};
    readNarrowingMask(walkMask);
    std::vector<std::uint32_t> walkMaskCpus;
    for (std::size_t cpu = 0; cpu < static_cast<std::size_t>(CPU_SETSIZE);
         ++cpu)
    {
      if (CPU_ISSET(cpu, &walkMask) != 0) {
        walkMaskCpus.push_back(static_cast<std::uint32_t>(cpu));
      }
    }
    for (auto* entry : selected) {
      // R-01: this precheck stands under `batchProbed`, which holds
      // only when no family carries a probe of its own, so no family
      // probe covers an entry on any path that reaches here.
      if (entry->threadCount > 1 && entry->placement != detail::Preset::NONE) {
        continue;
      }
      const auto narrowing = detail::maskNarrowingOf(probeCpus, walkMaskCpus);
      if (narrowing.remaining.empty()) {
        std::fprintf(stderr,
                     "--jitter-probe: the probe cpus leave no cpu for '%s'\n",
                     entry->name.c_str());
        // R-01: the batch probe is emplaced on every path that reaches
        // this precheck, so the session this stop lands on is the live
        // one (FR-006, R-01).
        jitterProbe->stopAndJoin();
        return 2;
      }
    }
  }

  detail::Runner runner(
      options, options.jitterProbe.has_value() || !familyProbes.empty());
  // specs/028 FR-051, FR-055, D-12: the order the run walks. A seeded run
  // walks the selected instances in the order `detail::shuffledOrder`
  // answers, and every other run walks them in the order they registered.
  // The order settles here, before the first measurement, so the walk costs
  // the loop below nothing.
  const bool interleaved = resolved->interleave.given;
  std::vector<std::size_t> walk(selected.size());
  std::iota(walk.begin(), walk.end(), std::size_t {0});
  if (interleaved) {
    walk = detail::shuffledOrder(selected.size(), resolved->interleave.seed);
  }

  // FR-056, FR-057: a shuffled run holds the row of each instance beside the
  // raw columns of the run that made them, and prints what it holds after the
  // walk. The entry of an instance the walk never reached stays empty.
  struct HeldRow
  {
    BenchmarkResult result;
    std::vector<RowRaw> raw;
    std::vector<std::vector<RowRaw>> contextRaw;
    std::vector<sg::detail::TimedWindow> windows;
    const std::vector<detail::ProbeCpuState>* states;
  };

  std::vector<std::optional<HeldRow>> held(selected.size());
  // E-06, R-04, FR-015: the states each result's folds read, one entry
  // per result of the batch in the order the results push: the family
  // probe's own states for a family the handle named, the batch probe's
  // for every other, and none for a fit row.
  std::vector<const std::vector<detail::ProbeCpuState>*> resultProbes;
  // R-08, R-04: a probed run prints its benchmark tables behind the join
  // edge, so the per-benchmark attribution line reads the folds the
  // split fills after the stop; the printed order keeps the order the
  // streaming carries, and a probe-free run keeps today's bytes.
  const bool consoleDeferred = jitterProbe.has_value() || !familyProbes.empty();
  // E-06, R-04: the windows each entry's runs stamped, one entry per
  // result of the batch in the order the results push, the alignment the
  // split reads below.
  std::vector<std::vector<sg::detail::TimedWindow>> entryWindows;
  // The loop variable keeps the spelling `entry`: `test/cli_shape.sh`
  // locates this loop by the `runner.run(*entry)` line and reads the
  // flag exactly once inside it.
  for (const std::size_t position : walk) {
    auto* entry = selected[position];
    // FR-015: the probe that covers this instance: the family's own
    // probe when the handle named one, the batch probe for every
    // other (FR-015, R-06).
    FamilyProbe* covering = nullptr;
    for (auto& probe : familyProbes) {
      if (probe.family == entry->family) {
        covering = &probe;
        break;
      }
    }
    // FR-015: the family's own probe starts before its first instance
    // in the walk, outside the timed windows (FR-006); R-01 holds one
    // session on the process at a time, so a live session hands over
    // here, the way the walk's spans meet.
    if (covering != nullptr && !covering->started
        && position == covering->first)
    {
      // R-01: the batch probe stands only when no family carries a
      // probe of its own (`batchProbed`), so no batch session is live
      // when a family's own probe starts.
      for (auto& other : familyProbes) {
        if (other.holder.has_value() && !other.joined) {
          other.holder->stopAndJoin();
          other.joined = true;
        }
      }
      auto startedOptions = covering->options;
      startedOptions.cpus = covering->cpus;
      covering->holder.emplace(startedOptions);
      covering->holder->start();
      covering->started = true;
      if (covering->holder->states().empty()) {
        // The availability answer (Q-1): strict stops the batch, and
        // the plain run discloses and continues (FR-037, D-10).
        if (covering->options.strict) {
          std::fprintf(
              stderr,
              "--jitter-probe: the machine/tsc leaf is not available\n");
          for (auto& probe : familyProbes) {
            if (probe.holder.has_value() && !probe.joined) {
              probe.holder->stopAndJoin();
              probe.joined = true;
            }
          }
          return 2;
        }
        covering->unavailable = true;
      }
    }
    const bool familyActive =
        covering != nullptr && covering->started && !covering->unavailable;
    const bool batchActive = batchProbed && !jitterUnavailable;
    const std::vector<std::uint32_t>& coveringCpus =
        familyActive ? covering->cpus : probeCpus;
    // FR-022, FR-023: the complexity fit of the family that ended prints
    // between the rows of two families, after every row of the family it
    // summarizes. A shuffled run holds its fits for the print pass, where a
    // family ends at its last row in registration order (FR-056); a probed
    // run holds its print behind the join edge the same way (R-08).
    if (!interleaved) {
      if (const auto fit = runner.fitForFamilyEnd(entry)) {
        report.results.push_back(*fit);
        entryWindows.emplace_back();
        resultProbes.push_back(nullptr);
        // FR-002: a fit stands as its own entry of the document, beside
        // the entries of the runs it summarizes.
        report.rowRaw.emplace_back();
        report.contextRaw.emplace_back();
        if (!consoleDeferred) {
          detail::printResult(report.results.back());
        }
      }
    }
    // FR-019, D-5: an instance the runner does not pin narrows the
    // calling thread's affinity mask by the probe cpus for the run -
    // `sched_setaffinity(0, ...)` names the calling thread, never the
    // process. A pinned instance is exactly one with more than one
    // context and a preset: the runner pins only instances with more
    // than one context, and then only when the preset is not `none`,
    // so a single-context instance that carries a preset is unpinned
    // and narrows too. A live probe stands behind every narrowing: the
    // precheck mode runs no probe thread beside a benchmark, and an
    // unavailable probe runs none at all, so neither narrows (FR-006,
    // R-10, FR-037). The order is what makes that safe: the probe threads
    // start before the narrowing and keep their own masks, and the
    // runner creates the benchmark's worker threads after it, so they
    // inherit the narrowed mask the benchmark runs under. The change
    // is disclosed and the mask restored after the instance ends; the
    // empty intersection stops with one line naming the benchmark.
    const bool probeLive = familyActive || batchActive;
    const JitterProbeMode coveringMode = familyActive
        ? covering->options.mode
        : (batchActive ? options.jitterProbe->mode
                       : JitterProbeMode::ALONGSIDE);
    cpu_set_t savedMask {};
    bool maskNarrowed = false;
    if (detail::narrowingApplies(*entry, probeLive, coveringMode)) {
      CPU_ZERO(&savedMask);
      readNarrowingMask(savedMask);
      std::vector<std::uint32_t> maskCpus;
      for (std::size_t cpu = 0; cpu < static_cast<std::size_t>(CPU_SETSIZE);
           ++cpu)
      {
        if (CPU_ISSET(cpu, &savedMask) != 0) {
          maskCpus.push_back(static_cast<std::uint32_t>(cpu));
        }
      }
      const auto narrowing = detail::maskNarrowingOf(coveringCpus, maskCpus);
      cpusetLimited = cpusetLimited || narrowing.limited;
      if (narrowing.remaining.empty()) {
        std::fprintf(stderr,
                     "--jitter-probe: the probe cpus leave no cpu for '%s'\n",
                     entry->name.c_str());
        // The family holders stand emplaced beside the handles that
        // named their probes (R-01), so the stops land on the live
        // sessions before the run leaves them. R-01 keeps the batch
        // probe out of reach here too: it stands only under
        // `batchProbed`, which holds when no family carries a probe of
        // its own.
        for (auto& probe : familyProbes) {
          if (probe.holder.has_value() && !probe.joined) {
            probe.holder->stopAndJoin();
            probe.joined = true;
          }
        }
        return 2;
      }
      cpu_set_t narrowed {};
      CPU_ZERO(&narrowed);
      for (const auto cpu : narrowing.remaining) {
        CPU_SET(cpu, &narrowed);
      }
      applyNarrowedMask(narrowed);
      maskNarrowed = true;
      maskNarrowedNames.push_back(entry->name);
    }
    const auto result = runner.run(*entry);
    if (maskNarrowed) {
      restoreSavedMask(savedMask);
    }
    // E-06, R-04, FR-015: the states this result's folds read: the
    // family probe's own for a family the handle named, the batch
    // probe's for every other, and none when no probe stands.
    const std::vector<detail::ProbeCpuState>* coveringStates = nullptr;
    if (covering != nullptr && covering->holder.has_value()) {
      coveringStates = &covering->holder->states();
    } else if (batchProbed) {
      coveringStates = &jitterProbe->states();
    }
    if (interleaved) {
      held[position] = HeldRow {result,
                                runner.rowRaw().back(),
                                runner.contextRaw().back(),
                                runner.takeRunWindows(),
                                coveringStates};
    } else {
      report.results.push_back(result);
      entryWindows.push_back(runner.takeRunWindows());
      resultProbes.push_back(coveringStates);
      report.rowRaw.push_back(runner.rowRaw().back());
      report.contextRaw.push_back(runner.contextRaw().back());
      if (!consoleDeferred) {
        detail::printResult(report.results.back());
      }
    }
    // FR-015: the family's probe stops after the family's last
    // instance in the walk, outside the timed windows (FR-006).
    if (covering != nullptr && !covering->joined && position == covering->last)
    {
      covering->holder->stopAndJoin();
      covering->joined = true;
    }
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

  // FR-006, R-01: the probe spans the batch, so the stop lands after
  // the selected loop ends, before the print passes and the split.
  if (jitterProbe.has_value()) {
    jitterProbe->stopAndJoin();
  }
  // FR-015: a family probe the walk's interruption left live stops
  // here; a family the walk completed stopped at its last instance.
  for (auto& probe : familyProbes) {
    if (probe.holder.has_value() && !probe.joined) {
      probe.holder->stopAndJoin();
      probe.joined = true;
    }
  }

  // FR-056, FR-057: the held rows print after the walk in registration
  // order, and the fit of a family follows the last held row of that family.
  // The rows an interrupted run holds are those of the instances its walk
  // reached, in the same order.
  if (interleaved) {
    const auto endsFamily = [&selected, &held](const std::size_t index)
    {
      for (std::size_t later = index + 1; later < selected.size(); ++later) {
        if (held[later].has_value()) {
          return selected[later]->family != selected[index]->family;
        }
      }
      return true;
    };
    for (std::size_t index = 0; index < selected.size(); ++index) {
      const auto& row = held[index];
      if (!row.has_value()) {
        continue;
      }
      if (!consoleDeferred) {
        detail::printResult(row->result);
      }
      report.results.push_back(row->result);
      entryWindows.push_back(std::move(row->windows));
      resultProbes.push_back(row->states);
      report.rowRaw.push_back(row->raw);
      report.contextRaw.push_back(row->contextRaw);
      if (endsFamily(index)) {
        if (const auto fit = runner.fitForFamily(selected[index]->family)) {
          if (!consoleDeferred) {
            detail::printResult(*fit);
          }
          report.results.push_back(*fit);
          entryWindows.emplace_back();
          // R-08: a fit is no run of its own, so it carries no probe
          // states - the split reads one pointer per entry, and the
          // held rows' pointers stand ahead of it (FR-022, E-06).
          resultProbes.push_back(nullptr);
          report.rowRaw.emplace_back();
          report.contextRaw.emplace_back();
        }
      }
    }
  }

  // FR-022: the run is over, so the fit of the last family prints after its
  // rows. The print pass answered every family a shuffled run ran, so this
  // answers nothing for such a run.
  if (const auto fit = runner.fitForFamilyEnd(nullptr)) {
    if (!consoleDeferred) {
      detail::printResult(*fit);
    }
    report.results.push_back(*fit);
    entryWindows.emplace_back();
    resultProbes.push_back(nullptr);
    report.rowRaw.emplace_back();
    report.contextRaw.emplace_back();
  }

  // R-04, E-06: the split reads the buffers behind the join edge the
  // stop gives, folding each probe cpu's kept gaps into the windows
  // each entry's runs stamped, in ascending cpu order; an entry
  // carries one fold per cpu whose kept gaps landed inside its
  // windows, and a cpu whose gaps landed inside none stands absent
  // (E-06, FR-021).
  for (std::size_t index = 0; index < report.results.size(); ++index) {
    const auto* coveringStates = resultProbes.at(index);
    if (coveringStates == nullptr) {
      continue;
    }
    // E-06: the document sorts the cpus ascending, the one order the
    // disclosure text carries (E-07).
    std::vector<const detail::ProbeCpuState*> ordered;
    ordered.reserve(coveringStates->size());
    for (const auto& state : *coveringStates) {
      ordered.push_back(&state);
    }
    std::ranges::sort(ordered,
                      [](const detail::ProbeCpuState* left,
                         const detail::ProbeCpuState* right)
                      { return left->cpu < right->cpu; });
    auto& stored = report.results.at(index);
    for (const auto* state : ordered) {
      const auto fold = detail::gapStatsOf(*state, entryWindows.at(index));
      if (fold.gapCount != 0) {
        stored.jitterFolds.push_back(
            sg::JitterEntryFold {.cpu = state->cpu, .stats = fold});
      }
    }
  }

  // R-08: the benchmark tables print here, behind the join edge, in
  // the order the batch pushed them.
  if (consoleDeferred) {
    for (const auto& stored : report.results) {
      detail::printResult(stored);
    }
  }

  // specs/027 E-09: the placement-safety disclosure rides the record
  // T012 prints: the skipped groups, the cpuset limit and the mask
  // narrowing (FR-017, FR-036, D-5, D-12, R-08).
  const std::string skippedText = detail::skippedGroupsText(skippedGroups);
  std::string placementTail;
  if (!skippedText.empty()) {
    placementTail += "; " + skippedText;
  }
  if (cpusetLimited) {
    placementTail += "; cpusets limit the probe cpus";
  }
  for (const auto& name : maskNarrowedNames) {
    placementTail += "; affinity mask narrowed for '" + name + "'";
  }

  // FR-023, FR-036, R-08: the probe block and the disclosure print in
  // the end-of-report region, after the benchmark tables; a probe-free
  // run prints none of them (FR-013, SC-003). A family probe's record
  // and lines read the states the family's own probe joined, the way
  // the batch probe's read its own (FR-015).
  const bool anyFamilyProbe = !familyProbes.empty();
  // FR-015: the states the record and the lines read, carried as the
  // pointers the non-copyable states lend (R-04).
  std::vector<const detail::ProbeCpuState*> batchStatePointers;
  if (batchProbed) {
    for (const auto& state : jitterProbe->states()) {
      batchStatePointers.push_back(&state);
    }
  }
  std::vector<const detail::ProbeCpuState*> familyStates;
  for (const auto& probe : familyProbes) {
    if (probe.holder.has_value()) {
      for (const auto& state : probe.holder->states()) {
        familyStates.push_back(&state);
      }
    }
  }
  const bool familyUnavailable =
      std::all_of(familyProbes.begin(),
                  familyProbes.end(),
                  [](const FamilyProbe& probe) { return probe.unavailable; });
  if (batchProbed || anyFamilyProbe) {
    if (batchProbed) {
      detail::printJitterReport(!jitterUnavailable, batchStatePointers);
    } else {
      detail::printJitterReport(!familyUnavailable, familyStates);
    }
    // E-09: the placement-safety items print as new lines in the same
    // region; the JSON disclosure text carries the same content as one
    // string (FR-017, FR-036, D-5, D-12, R-08, FR-026).
    if (!skippedText.empty()) {
      std::printf("jitter: %s\n", skippedText.c_str());
    }
    if (cpusetLimited) {
      std::printf("jitter: cpusets limit the probe cpus\n");
    }
    for (const auto& name : maskNarrowedNames) {
      std::printf("jitter: affinity mask narrowed for '%s'\n", name.c_str());
    }
  }

  // E-07: the record the document carries, built in the function
  // above from the joined states the fold reads (R-04).
  const auto jitterRecord = (batchProbed || anyFamilyProbe)
      ? std::optional(batchProbed ? buildJitterReportRecord(
                                        *options.jitterProbe,
                                        jitterUnavailable,
                                        options.rawContent == RawContent::GAPS,
                                        placementTail,
                                        batchStatePointers)
                                  : buildJitterReportRecord(
                                        familyProbes.front().options,
                                        familyUnavailable,
                                        options.rawContent == RawContent::GAPS,
                                        placementTail,
                                        familyStates))
      : std::nullopt;

  // FR-030: a document the file system refused reports on stderr and
  // leaves the exit status at the usage error the table names, which
  // outranks the 1 a failed benchmark leaves.
  [[maybe_unused]] bool reportFailed = false;
  if (!options.jsonPath.empty()) {
    // specs/028 FR-039, FR-040: the document carries the same facts the
    // console line carries. A disclosure naming no use and no seed writes no
    // `run` key at all (FR-041).
    std::optional<std::uint64_t> seed;
    if (resolved->interleave.given) {
      seed = resolved->interleave.seed;
    }
    const detail::RunDisclosure disclosure {resolved->uses, seed};
    if (!detail::writeReport(
            options.jsonPath, report, disclosure, jitterRecord))
    {
      reportFailed = true;
      status = 2;
    }
  }

  SG_ENSURE((status != 0) == (anyFailed || anyInterrupted || reportFailed),
            "the exit status follows the table of contracts/cli.md (FR-036)");

  return status;
}

}  // namespace sg
