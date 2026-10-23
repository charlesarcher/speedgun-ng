#include <algorithm>
#include <atomic>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <expected>
#include <memory>
#include <numeric>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <vector>

#include <sched.h>
#include <unistd.h>

#include "detail/compare.hpp"
#include "detail/internal.hpp"
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

  // specs/028 FR-063, spec D-23: the compare mode reads the settled list,
  // which carries the rules of every source in the order spec D-23 states,
  // where the words of the command line alone no longer stand (FR-061).
  options.compareRules = std::move(resolved->rules);

  // specs/029 FR-081, D-16: the conflict check reads the settled members of the
  // scan, so a `--counter` that stands after `--hunt` is the same conflict as
  // one that stands before it. It stands ahead of the compare mode, which reads
  // no catalog and runs no benchmark.
  detail::validateHuntCombination(options);

  // specs/029 FR-006, FR-081, PD-17: the selection expands here, where the
  // clock and PMU providers stand registered and before the first run, so an
  // unknown address, a pattern that matches nothing and an unreadable `@path`
  // cost no measured time.
  detail::expandHuntSelection(options);

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
  detail::Runner runner(options);
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
    // specs/029 T126: the record of the hunt that built this row's hunt
    // object rides the row, so the deferred print of a shuffled run prints
    // the same hunt block the immediate print of an ordered run prints.
    std::optional<hunt::RunHunt> hunt;
  };

  std::vector<std::optional<HeldRow>> held(selected.size());
  // The loop variable keeps the spelling `entry`: `test/cli_shape.sh`
  // locates this loop by the `runner.run(*entry)` line and reads the
  // flag exactly once inside it.
  for (const std::size_t position : walk) {
    auto* entry = selected[position];
    // FR-022, FR-023: the complexity fit of the family that ended prints
    // between the rows of two families, after every row of the family it
    // summarizes. A shuffled run holds its fits for the print pass, where a
    // family ends at its last row in registration order (FR-056).
    if (!interleaved) {
      if (const auto fit = runner.fitForFamilyEnd(entry)) {
        detail::printResult(*fit);
        // FR-002: a fit stands as its own entry of the document, beside
        // the entries of the runs it summarizes.
        report.results.push_back(*fit);
        report.rowRaw.emplace_back();
        report.contextRaw.emplace_back();
      }
    }
    auto result = runner.run(*entry);
    // specs/029 FR-040, FR-043: `--hunt` hunts the instances the filter
    // selected, and the record rides the result the printers and the writer
    // read; a hunt that refuses stands its error line on stderr with the
    // failed outcome's exit status (FR-081, spec PC-9).
    auto runRecord = std::optional<hunt::RunHunt> {};
    if (options.hunt.has_value()) {
      // specs/029 T080 superseded by T119, FR-012: the run hands the hunt no
      // calibration figure. The sampling overhead a row carries prices one
      // read of an open window; the hunt entry times one pass of its own
      // plan, so the estimate of the plan line weighs the pass count against
      // the cost of a pass. No figure here means no figure lies there.
      auto hunted = hunt::runHuntNamed(entry->name,
                                       *options.hunt,
                                       std::nullopt,
                                       options.metricMethod,
                                       options.dryRun,
                                       true);
      if (hunted.has_value()) {
        result.hunt = std::move(hunted->entry);
        runRecord = std::move(hunted->run);
        // specs/029 FR-031, PD-10, T126: the run-level record the hunt
        // answered rides the report data the document writer reads, so the
        // root `hunt` object stands in the document of a hunt run. The
        // record reaches here with the answer of its own hunt, and the
        // console printer below reads the same record. A run with no hunt
        // leaves both empty and the document keeps its former bytes (I-8,
        // FR-052).
        report.hunt = runRecord;
        // D-19: a hunt run states its method outright, so the method of the
        // forced collection rides the result the document writer reads even
        // where no counter column of its own settled one.
        if (options.metricMethod.has_value()) {
          result.metricMethod = *options.metricMethod;
        }
      } else {
        std::fprintf(stderr, "hunt: %s\n", hunted.error().message.c_str());
        // A refused hunt carries the failed outcome its status comes from
        // (FR-081), so the flag rides it too: the FR-036 table compares the
        // status against the flags, and a status standing over no flag aborts
        // on the postcondition below before the shell sees it.
        anyFailed = true;
        status = 1;
      }
    }
    if (interleaved) {
      held[position] = HeldRow {result,
                                runner.rowRaw().back(),
                                runner.contextRaw().back(),
                                runRecord};
    } else {
      detail::printResult(result, runRecord);
      report.results.push_back(result);
      report.rowRaw.push_back(runner.rowRaw().back());
      report.contextRaw.push_back(runner.contextRaw().back());
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
      detail::printResult(row->result, row->hunt);
      report.results.push_back(row->result);
      report.rowRaw.push_back(row->raw);
      report.contextRaw.push_back(row->contextRaw);
      if (endsFamily(index)) {
        if (const auto fit = runner.fitForFamily(selected[index]->family)) {
          detail::printResult(*fit);
          report.results.push_back(*fit);
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
    // specs/028 FR-039, FR-040: the document carries the same facts the
    // console line carries. A disclosure naming no use and no seed writes no
    // `run` key at all (FR-041).
    std::optional<std::uint64_t> seed;
    if (resolved->interleave.given) {
      seed = resolved->interleave.seed;
    }
    const detail::RunDisclosure disclosure {resolved->uses, seed};
    if (!detail::writeReport(options.jsonPath, report, disclosure)) {
      reportFailed = true;
      status = 2;
    }
  }

  SG_ENSURE((status != 0) == (anyFailed || anyInterrupted || reportFailed),
            "the exit status follows the table of contracts/cli.md (FR-036)");

  return status;
}

}  // namespace sg
