// ============================================================================
// The public surface of the noise tracer (specs/036-osnoise-timerlat-tracers
// T001, T002): the struct defaults of data-model.md E-16 and E-17, and the
// source-order binding rows of FR-005. The carrier rows drive the scan and
// the gather the way `harness_cli_sources_test.cpp` drives them, and they
// stand red until T007 lands the option row and the value parser. The
// suite-order row runs `speedgunMain` in a forked child the way
// `harness_jitter_cli_test.cpp` builds its `childFileOverSuite` scenario,
// and it stands red until T024 lands the integration. The staging stands
// in the tasks table. Frameworkless check()/fail() convention.
// ============================================================================

#include <cstdint>
#include <cstdio>
#include <expected>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#include <sched.h>
#include <sys/wait.h>
#include <unistd.h>

#include "detail/options.hpp"
#include "detail/proc_root.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_fake.hpp"
#include "speedgun-ng/counters_system.hpp"

namespace
{

int failures = 0;

auto fail(const char* what) -> void
{
  ++failures;
  std::printf("FAILED: %s\n", what);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// The cpu indexes the binding values name, each one inside the process
// affinity mask (specs/036 FR-008): the parser validates every cpu of a
// `cpus=` list against the mask and the online set, so the values stand
// inside both on every host the suite runs on.
auto pickCpus(std::size_t count) -> std::vector<std::uint32_t>
{
  cpu_set_t mask {};
  CPU_ZERO(&mask);
  if (sched_getaffinity(0, sizeof(mask), &mask) != 0) {
    fail("the process affinity mask is unreadable (FR-008)");
    return {};
  }
  std::vector<std::uint32_t> cpus;
  for (std::size_t index = 0;
       index < static_cast<std::size_t>(CPU_SETSIZE) && cpus.size() < count;
       ++index)
  {
    if (CPU_ISSET(static_cast<unsigned int>(index), &mask) != 0) {
      cpus.push_back(static_cast<std::uint32_t>(index));
    }
  }
  if (cpus.size() < count) {
    fail("the affinity mask holds too few cpus for the scenario (FR-008)");
  }
  return cpus;
}

// The scripted provider: the leaves the runner resolves its plans
// through, plus the machine/tsc leaf the probe's availability check
// reads (R-02, Q-1). The tsc series steps by one tick, so the probe
// records no gap and the batch stays quiet.
auto scriptedProvider() -> sg::counters::FakeProvider&
{
  static auto* provider = []
  {
    auto owned = std::make_unique<sg::counters::FakeProvider>();
    auto& ref = *owned;
    ref.addCounter("machine", "monotonic", "nanoseconds", "wall clock");
    ref.setPoints("machine", "monotonic", {0}, 4000);
    ref.addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
    ref.setPoints("machine", "thread_cpu", {0}, 2000);
    ref.addCounter("machine",
                   "tsc",
                   "none",
                   "raw tick counter",
                   sg::counters::Availability::COUNTABLE,
                   sg::counters::ReadMode::FAST_TSC);
    ref.setPoints("machine", "tsc", {0}, 1);
    const auto registered =
        sg::counters::System::local().registerProvider(std::move(owned));
    check(registered.has_value(), "the scripted provider registers (R-02)");
    return &ref;
  }();
  return *provider;
}

// A trivial benchmark: an accepted value proceeds into the batch, so
// the suite-order scenario needs one registered function.
auto bmQuiet(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

// One command line through the scan, in the shape a shell hands to `main`.
auto scanOf(std::initializer_list<const char*> words)
    -> std::expected<sg::detail::CommandLineOptions, std::string>
{
  const std::vector<std::string> owned(words.begin(), words.end());
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_noise_test"));
  for (const auto& word : owned) {
    argv.push_back(const_cast<char*>(word.c_str()));
  }
  return sg::detail::scanCommandLine(static_cast<int>(argv.size()),
                                     argv.data());
}

// The gather over the sources one row hands it: the scanned command line,
// no environment rows, and the text of the option file as data, the shape
// the gather contract fixes (E-02, E-03). The answer holds the settled
// options or the one fault text, never both.
struct Gathered
{
  std::optional<sg::detail::ResolvedOptions> settled;
  std::string fault;
};

auto gatherSources(std::initializer_list<const char*> words,
                   const std::string& fileText) -> Gathered
{
  Gathered gathered;
  const auto scan = scanOf(words);
  if (!scan.has_value()) {
    gathered.fault = "the scan refused the command line: " + scan.error();
    return gathered;
  }
  const std::optional<sg::detail::OptionFileText> file =
      sg::detail::OptionFileText {std::string("harness_noise_options.txt"),
                                  fileText};
  const auto answer = sg::detail::gatherOptionSources(*scan, {}, file);
  if (answer.has_value()) {
    gathered.settled = *answer;
  } else {
    gathered.fault = answer.error();
  }
  return gathered;
}

// E-16, T002: a default construction carries no tracer, the way the
// struct contract states it (specs/036 FR-005, R-06).
auto theDefaultOptionsCarryNoTracer() -> void
{
  const sg::JitterProbeOptions options;
  check(options.tracerEngine == sg::TracerEngine::NONE,
        "the default engine names no tracer (E-16, R-06)");
  check(options.tracerCpus.empty(),
        "the default cpu list names the candidate set (E-16, FR-008)");
}

// E-17, T002: a default record carries no tracer record, the way the
// record contract states it (specs/036 FR-015, R-10).
auto theDefaultRecordCarriesNoTracer() -> void
{
  const sg::PrecheckRecord record;
  check(!record.tracer.has_value(),
        "the default record leaves the tracer empty (E-17, R-10)");
}

// FR-005, D-6: the command line's value stands over the option file's.
// Both sources name the option, the file names the other engine and the
// other cpu, and the settled carrier names the command line's pair
// alone (FR-002, FR-005, R-06).
auto theCommandLineSettlesOverTheOptionFile() -> void
{
  const auto cpus = pickCpus(2);
  const std::string cliValue =
      "--os-noise=osnoise,cpus=" + std::to_string(cpus.at(0));
  const std::string fileLine =
      "os-noise=timerlat,cpus=" + std::to_string(cpus.at(1)) + "\n";
  const Gathered gathered = gatherSources({cliValue.c_str()}, fileLine);
  check(gathered.settled.has_value(),
        ("the gather accepts the two sources (FR-005): " + gathered.fault)
            .c_str());
  if (!gathered.settled.has_value()) {
    return;
  }
  const auto& settled = gathered.settled->options;
  check(settled.noiseTracer.has_value()
            && *settled.noiseTracer == sg::TracerEngine::OSNOISE,
        "the command line's engine stands over the file's (FR-005, D-6)");
  const std::vector<std::uint32_t> wanted {cpus.at(0)};
  check(settled.noiseTracerCpus == wanted,
        "the command line's cpu list stands over the file's (FR-005, D-6)");
}

// The scratch names of the suite-order scenario, one set per row, so a
// second copy of this binary shares no file.
struct SuiteOrderFiles
{
  std::filesystem::path root;
  const char* optionPath;
  const char* jsonPath;
  std::string outPath;
  std::string errPath;
};

// One speedgunMain run in a forked child with both streams captured:
// the suite binding stands a process-wide fact, so the scenario runs
// in a child either way (the harness_jitter_cli_test pattern). The
// child binds `runJitterProbe` with the other engine and the other
// cpu, names the option in the option file with the first cpu, and
// runs the entry point with no `--jitter-probe` word: the suite
// binding carries the pre-check mode the run needs.
auto runSuiteOrderChild(const SuiteOrderFiles& files,
                        const std::vector<std::uint32_t>& cpus) -> int
{
  // The streams stand flushed ahead of the fork, so the child's
  // redirect cannot flush the parent's buffered lines a second time.
  std::fflush(stdout);
  std::fflush(stderr);
  const pid_t pid = fork();
  if (pid < 0) {
    fail("cannot fork the child");
    return 3;
  }
  if (pid == 0) {
    if (std::freopen(files.outPath.c_str(), "w", stdout) == nullptr
        || std::freopen(files.errPath.c_str(), "w", stderr) == nullptr)
    {
      _exit(3);
    }
    scriptedProvider();
    // The fake tracing tree the session reads and writes through the
    // file-root seam (specs/036 FR-017): the suite needs no kernel
    // tracer and no rights, so the record the row reads stands the
    // run's own (FR-014).
    std::filesystem::create_directories(files.root
                                        / "sys/kernel/tracing");
    {
      std::ofstream(files.root / "sys/kernel/tracing/current_tracer")
          << "nop\n";
      std::ofstream(files.root / "sys/kernel/tracing/tracing_cpumask")
          << "ffffffff\n";
      std::ofstream(files.root / "sys/kernel/tracing/tracing_on") << "0\n";
    }
    sg::detail::installTestProcRoot(files.root);
    sg::JitterProbeOptions suiteOptions;
    suiteOptions.mode = sg::JitterProbeMode::PRECHECK;
    suiteOptions.tracerEngine = sg::TracerEngine::TIMERLAT;
    suiteOptions.tracerCpus = {cpus.at(1)};
    sg::runJitterProbe(suiteOptions);
    {
      std::ofstream file(files.optionPath);
      file << "os-noise=osnoise,cpus=" << std::to_string(cpus.at(0)) << "\n";
    }
    const std::vector<std::string> arguments {
        "--filter=bmQuiet",
        "--repetitions=1",
        "--iterations=2",
        std::string("--options=") + files.optionPath,
        std::string("--json=") + files.jsonPath};
    std::vector<char*> argv;
    argv.push_back(const_cast<char*>("harness_noise_test"));
    for (const auto& argument : arguments) {
      argv.push_back(const_cast<char*>(argument.c_str()));
    }
    const int status =
        sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
    std::fflush(stdout);
    std::fflush(stderr);
    std::exit(status);
  }
  int waitStatus = 0;
  if (waitpid(pid, &waitStatus, 0) < 0) {
    fail("cannot wait for the child");
    return 3;
  }
  if (!WIFEXITED(waitStatus)) {
    fail("the child did not exit normally");
    return 3;
  }
  return WEXITSTATUS(waitStatus);
}

// FR-005, D-6: the option file's value stands over the suite call's.
// The fresh process binds the suite call with the other engine and the
// other cpu, and the document the run writes carries the file's engine
// and the file's cpu alone, so the suite call's pair stands dropped.
// The row stands red until T024 lands the integration, the staging the
// tasks table records.
auto theOptionFileSettlesOverTheSuiteCall() -> void
{
  const auto cpus = pickCpus(2);
  const SuiteOrderFiles files {
      std::filesystem::path("harness_noise_suite-order-fakeroot"),
      "harness_noise_suite-order.txt",
      "harness_noise_suite-order.json",
      "harness_noise_suite-order-out.txt",
      "harness_noise_suite-order-err.txt"};
  const int status = runSuiteOrderChild(files, cpus);
  check(status == 0, "the suite-order run stands status 0 (FR-005)");
  const auto loaded = sg::loadReport(files.jsonPath);
  check(loaded.has_value(),
        "the run writes the document the row reads (FR-005)");
  if (loaded.has_value()) {
    const auto& precheck = loaded->precheck;
    check(precheck.has_value(),
          "the document carries the pre-check record (FR-005)");
    if (precheck.has_value()) {
      check(precheck->tracer.has_value(),
            "the record carries the tracer the file settled "
            "(FR-005, D-6)");
      if (precheck->tracer.has_value()) {
        check(precheck->tracer->engine == "osnoise",
              "the file's engine stands over the suite call's "
              "(FR-005, D-6)");
        check(precheck->tracer->cpus.size() == 1
                  && precheck->tracer->cpus.front().cpu == cpus.at(0),
              "the file's cpu stands over the suite call's "
              "(FR-005, D-6)");
      }
    }
  }
  std::remove(files.optionPath);
  std::remove(files.jsonPath);
  std::remove(files.outPath.c_str());
  std::remove(files.errPath.c_str());
  std::error_code removeFailed;
  std::filesystem::remove_all(files.root, removeFailed);
}

}  // namespace

SG_BENCHMARK(bmQuiet);

auto main() -> int
{
  theDefaultOptionsCarryNoTracer();
  theDefaultRecordCarriesNoTracer();
  theCommandLineSettlesOverTheOptionFile();
  theOptionFileSettlesOverTheSuiteCall();
  std::printf(failures == 0 ? "ok\n" : "%d checks failed\n", failures);
  return failures == 0 ? 0 : 1;
}
