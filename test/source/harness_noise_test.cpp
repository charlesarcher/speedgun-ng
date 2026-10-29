// ============================================================================
// The public surface and the engine session of the noise tracer
// (specs/036-osnoise-timerlat-tracers T001, T002, T008 to T010): the
// struct defaults of data-model.md E-16 and E-17, the source-order
// binding rows of FR-005, and the engine session rows over fake
// tracefs trees (FR-007, FR-010, FR-016, FR-017). The carrier rows drive the
// scan and the gather the way `harness_cli_sources_test.cpp` drives them, and
// they stand red until T007 lands the option row and the value parser. The
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
#include <iomanip>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#include <sched.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "detail/internal.hpp"
#include "detail/noise_tracer.hpp"
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
    std::filesystem::create_directories(files.root / "sys/kernel/tracing");
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
            "the record carries the tracer the file settled " "(FR-005, D-6)");
      if (precheck->tracer.has_value()) {
        check(
            precheck->tracer->engine == "osnoise",
            "the file's engine stands over the suite call's " "(FR-005, D-6)");
        check(precheck->tracer->cpus.size() == 1
                  && precheck->tracer->cpus.front().cpu == cpus.at(0),
              "the file's cpu stands over the suite call's " "(FR-005, D-6)");
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

// The first cpu index the process affinity mask does not hold, below the
// set's own bound: the token the outside-the-mask refusal reads (FR-003).
auto pickCpuOutsideMask() -> std::string
{
  cpu_set_t mask {};
  CPU_ZERO(&mask);
  if (sched_getaffinity(0, sizeof(mask), &mask) != 0) {
    fail("the process affinity mask is unreadable (FR-003)");
    return {};
  }
  for (std::size_t cpu = 0; cpu < static_cast<std::size_t>(CPU_SETSIZE); ++cpu)
  {
    if (CPU_ISSET(cpu, &mask) == 0) {
      return std::to_string(cpu);
    }
  }
  fail("the mask leaves no cpu index free (FR-003)");
  return {};
}

// The text of one captured stream.
auto readStream(const std::string& path) -> std::string
{
  std::ifstream file(path);
  std::ostringstream text;
  text << file.rdbuf();
  return text.str();
}

// The count of the lines one captured stream carries.
auto lineCount(const std::string& text) -> std::size_t
{
  std::size_t count = 0;
  for (const char current : text) {
    if (current == '\n') {
      ++count;
    }
  }
  return count;
}

// The exit status and both captures of one forked entry-point child: a
// refusal answers from inside the entry point, so the run stands in a
// child either way (the harness_jitter_cli_test pattern).
struct Captured
{
  int status;
  std::string out;
  std::string err;
};

// One speedgunMain run in a forked child with both streams captured: the
// arguments stand the child's command line, and the child inherits the
// seams the caller installed ahead of the fork.
auto runRefusalChild(const std::string& tag,
                     const std::vector<std::string>& arguments) -> Captured
{
  const std::string outPath = "harness_noise_" + tag + "-out.txt";
  const std::string errPath = "harness_noise_" + tag + "-err.txt";
  std::fflush(stdout);
  std::fflush(stderr);
  const pid_t pid = fork();
  if (pid < 0) {
    fail("cannot fork the child");
    return Captured {3, {}, {}};
  }
  if (pid == 0) {
    if (std::freopen(outPath.c_str(), "w", stdout) == nullptr
        || std::freopen(errPath.c_str(), "w", stderr) == nullptr)
    {
      _exit(3);
    }
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
    return Captured {3, {}, {}};
  }
  if (!WIFEXITED(waitStatus)) {
    fail("the child did not exit normally");
    return Captured {3, {}, {}};
  }
  return Captured {
      WEXITSTATUS(waitStatus), readStream(outPath), readStream(errPath)};
}

// FR-003, SC-003: a refused value stands one line on standard error,
// carrying the option prefix, and exits 2 with no benchmark output.
auto checkRefusalShape(const Captured& run, const std::string& prefix) -> void
{
  check(run.status == 2, "the refused value stands status 2 (FR-003, SC-003)");
  check(lineCount(run.err) == 1,
        "the refusal stands one line (FR-003, SC-003)");
  check(run.err.find(prefix) != std::string::npos,
        "the refusal names the option and the fault (FR-003)");
  check(run.out.empty(),
        "the refusal prints no benchmark output (FR-003, SC-003)");
}

// T004, FR-002, D-4, R-06: the engine words settle the carrier through the
// scan and the gather, and an absent cpus key names the candidate set.
auto theEngineWordSettlesTheCarrier() -> void
{
  const Gathered osnoise = gatherSources({"--os-noise=osnoise"}, "");
  check(osnoise.settled.has_value(),
        ("the gather accepts the engine word (FR-002): " + osnoise.fault)
            .c_str());
  if (osnoise.settled.has_value()) {
    const auto& settled = osnoise.settled->options;
    check(settled.noiseTracer.has_value()
              && *settled.noiseTracer == sg::TracerEngine::OSNOISE,
          "the word osnoise settles the engine (FR-002, R-06)");
    check(settled.noiseTracerCpus.empty(),
          "the absent key names the candidate set (FR-008, R-09)");
  }

  const Gathered timerlat = gatherSources({"--os-noise=timerlat"}, "");
  check(timerlat.settled.has_value(),
        ("the gather accepts the engine word (FR-002): " + timerlat.fault)
            .c_str());
  if (timerlat.settled.has_value()) {
    const auto& settled = timerlat.settled->options;
    check(settled.noiseTracer.has_value()
              && *settled.noiseTracer == sg::TracerEngine::TIMERLAT,
          "the word timerlat settles the engine (FR-002, R-06)");
    check(settled.noiseTracerCpus.empty(),
          "the absent key names the candidate set (FR-008, R-09)");
  }
}

// T004, FR-002, R-06: the optional cpus key settles the tracer cpus
// through the standing cpu-list reader, the form the --topology listing
// prints.
auto theCpusKeySettlesTheTracerCpus() -> void
{
  const auto cpus = pickCpus(2);
  const std::string value = "--os-noise=timerlat,cpus="
      + std::to_string(cpus.at(0)) + "," + std::to_string(cpus.at(1));
  const Gathered gathered = gatherSources({value.c_str()}, "");
  check(
      gathered.settled.has_value(),
      ("the gather accepts the cpus key (FR-002): " + gathered.fault).c_str());
  if (!gathered.settled.has_value()) {
    return;
  }
  const auto& settled = gathered.settled->options;
  check(settled.noiseTracer.has_value()
            && *settled.noiseTracer == sg::TracerEngine::TIMERLAT,
        "the engine word settles the carrier (FR-002, R-06)");
  check(settled.noiseTracerCpus == cpus,
        "the cpus key settles the list through the standing reader "
        "(FR-002, R-06)");
}

// T004, FR-002: a second cpus key in one value stands the repeated-key
// refusal; the option file's value parses where the gather settles it,
// so the fault names the path, the line and the key.
auto theSecondCpusKeyStandsTheRepeatedKeyRefusal() -> void
{
  const auto cpus = pickCpus(2);
  const std::string line = "os-noise=osnoise,cpus=" + std::to_string(cpus.at(0))
      + ",cpus=" + std::to_string(cpus.at(1)) + "\n";
  const Gathered gathered = gatherSources({}, line);
  check(!gathered.settled.has_value(),
        "the gather refuses the repeated key (FR-002)");
  check(gathered.fault.find("--os-noise: ") != std::string::npos,
        "the refusal names the option (FR-002, FR-003)");
  check(gathered.fault.find("'cpus'") != std::string::npos,
        "the refusal names the key (FR-002)");
}

// T004, FR-002, D-4: --os-noise with no argument takes the standing
// missing-argument path, the line the scan answers at the row.
auto theBareOptionTakesTheMissingArgumentPath() -> void
{
  const auto scan = scanOf({"--os-noise"});
  if (scan.has_value()) {
    fail("the scan refuses the bare option (FR-002, D-4)");
    return;
  }
  check(scan.error().find("--os-noise") != std::string::npos
            && scan.error().find("needs a value") != std::string::npos,
        "the missing-value line names the option (FR-002, D-4)");
}

// T005, FR-003: an engine word outside the two refuses.
auto theEngineWordOutsideTheTwoRefuses() -> void
{
  const auto run = runRefusalChild("engine-word", {"--os-noise=undefined"});
  checkRefusalShape(run, "--os-noise: ");
}

// T005, FR-003: a cpus value that is no cpu index or range refuses.
auto theCpusTokenOutsideTheListRulesRefuses() -> void
{
  const auto run =
      runRefusalChild("cpus-token", {"--os-noise=osnoise,cpus=notacpu"});
  checkRefusalShape(run, "--os-noise: ");
}

// T005, FR-003: a cpu outside the process affinity mask refuses.
auto theCpuOutsideTheAffinityMaskRefuses() -> void
{
  const std::string value = "--os-noise=osnoise,cpus=" + pickCpuOutsideMask();
  const auto run = runRefusalChild("cpu-mask", {value});
  checkRefusalShape(run, "--os-noise: ");
}

// T005, FR-003: a cpu the online set does not hold refuses; the child
// inherits the online set the caller installs, so the refusal needs no
// host state.
auto theCpuOutsideTheOnlineSetRefuses() -> void
{
  const auto cpus = pickCpus(2);
  const std::string value =
      "--os-noise=osnoise,cpus=" + std::to_string(cpus.at(0));
  sg::detail::installTestOnlineCpus(std::vector<std::uint32_t> {cpus.at(1)});
  const auto run = runRefusalChild("cpu-online", {value});
  sg::detail::installTestOnlineCpus(std::nullopt);
  checkRefusalShape(run, "--os-noise: ");
}

// T005, FR-003: a cpu standing twice in one list refuses, the index and
// the whole value named.
auto theCpuStandingTwiceRefuses() -> void
{
  const auto cpus = pickCpus(1);
  const std::string cpu = std::to_string(cpus.at(0));
  const std::string value = "--os-noise=osnoise,cpus=" + cpu + "," + cpu;
  const auto run = runRefusalChild("cpu-twice", {value});
  checkRefusalShape(run, "--os-noise: ");
}

// T005, FR-003: a repeated key refuses, the key named.
auto theRepeatedKeyRefuses() -> void
{
  const auto cpus = pickCpus(2);
  const std::string value = "--os-noise=osnoise,cpus="
      + std::to_string(cpus.at(0)) + ",cpus=" + std::to_string(cpus.at(1));
  const auto run = runRefusalChild("key-twice", {value});
  checkRefusalShape(run, "--os-noise: ");
}

// T005, FR-003: a bad value from an option file line names the path, the
// line and the token, the standing file-fault shape.
auto theOptionFileFaultNamesThePathTheLineAndTheToken() -> void
{
  const std::string optionPath = "harness_noise_file-fault.txt";
  {
    std::ofstream file(optionPath);
    file << "os-noise=osnoise,cpus=notacpu\n";
  }
  const std::string option = "--options=" + optionPath;
  const auto run = runRefusalChild("file-fault", {option});
  std::remove(optionPath.c_str());
  checkRefusalShape(run, "--os-noise: ");
  check(run.err.find(optionPath) != std::string::npos,
        "the refusal names the path (FR-003)");
  check(run.err.find(":1:") != std::string::npos,
        "the refusal names the line (FR-003)");
  check(run.err.find("notacpu") != std::string::npos,
        "the refusal names the token (FR-003)");
}

// ---- The tracer engine session (T008 to T010, FR-007, FR-010,
// FR-016, FR-017, R-01 to R-05, R-07): the fake tracefs trees the
// scenarios install through the file-root seam, the setup rows, the
// figure rows and the not-available rows. The fake tree stands a
// scratch directory the test owns, named with the pid so parallel
// runs stand apart (T005 of the pre-check suite).

// The seam's root stands restored when the scenario ends, the way the
// standing install/clear seams stand cleared by the test.
class TracingRootGuard
{
public:
  TracingRootGuard()
  {
    // One fresh root per instance: the scenarios must not read the
    // files another scenario wrote (T010).
    static int sequence = 0;
    m_root = std::filesystem::path("harness_noise_tracing_root_")
        / ("pid" + std::to_string(::getpid()) + "-"
           + std::to_string(sequence++));
    std::filesystem::create_directories(m_root);
    sg::detail::installTestProcRoot(m_root);
  }

  ~TracingRootGuard()
  {
    sg::detail::installTestProcRoot(std::nullopt);
    std::error_code removeFailed;
    std::filesystem::remove_all(m_root, removeFailed);
  }

  TracingRootGuard(const TracingRootGuard&) = delete;
  auto operator=(const TracingRootGuard&) -> TracingRootGuard& = delete;

  auto root() const -> const std::filesystem::path& { return m_root; }

private:
  std::filesystem::path m_root;
};

// One fake file under the tracing root, the text written whole (T008).
auto writeTracingFile(const std::filesystem::path& root,
                      const std::string& relative,
                      const std::string& text) -> void
{
  const auto path = root / std::filesystem::path(relative);
  std::filesystem::create_directories(path.parent_path());
  std::ofstream file(path);
  check(static_cast<bool>(file), "the fake tracing file opens (T008)");
  file << text;
}

// The standing files of one fake tracing tree: the word `nop`, the
// mask text, and `tracing_on` at `0` (T008).
auto installStandingTree(const std::filesystem::path& root,
                         const std::string& maskText) -> void
{
  writeTracingFile(root, "sys/kernel/tracing/current_tracer", "nop\n");
  writeTracingFile(root, "sys/kernel/tracing/tracing_cpumask", maskText + "\n");
  writeTracingFile(root, "sys/kernel/tracing/tracing_on", "0\n");
}

// The per-cpu count file of one cpu, the path R-03 pins (T009).
auto cpuCountFile(std::uint32_t cpu) -> std::string
{
  return "sys/kernel/tracing/osnoise/cpu" + std::to_string(cpu);
}

// The mask text of one cpu set at `width` hex digits per word,
// comma-separated, the leftmost word naming the highest cpus, bit i of
// word w naming cpu w * width * 4 + i (T008, R-02, R-05): the shape
// the mask write renders the tracer cpus in.
auto maskTextOf(const std::vector<std::uint32_t>& cpus,
                std::size_t width) -> std::string
{
  const std::size_t bitsPerWord = width * 4;
  std::size_t wordCount = 1;
  for (const auto cpu : cpus) {
    const std::size_t needed = cpu / bitsPerWord + 1;
    if (needed > wordCount) {
      wordCount = needed;
    }
  }
  std::string text;
  for (std::size_t at = 0; at < wordCount; ++at) {
    const std::size_t word = wordCount - 1 - at;
    std::uint64_t bits = 0;
    for (const auto cpu : cpus) {
      if (cpu / bitsPerWord == word) {
        bits |= std::uint64_t {1} << (cpu % bitsPerWord);
      }
    }
    std::ostringstream digits;
    digits << std::hex << std::setw(static_cast<int>(width))
           << std::setfill('0') << bits;
    if (!text.empty()) {
      text += ',';
    }
    text += digits.str();
  }
  return text;
}

// T008, FR-007, FR-017, R-02, R-04, R-05: the setup reads the standing
// word and mask, writes the engine word, the tracer mask in the word
// width the standing mask carried, and `tracing_on` at `1`; the
// restore writes `tracing_on` at `0` and the standing word and mask.
auto theSetupWritesTheEngineTheMaskAndTheStart() -> void
{
  const auto cpus = pickCpus(2);
  TracingRootGuard guard;
  installStandingTree(guard.root(), "ffffffff");
  const auto session =
      sg::detail::noiseTracerSetup(sg::TracerEngine::OSNOISE, cpus);
  check(session.has_value(), "the setup answers the session (T008, R-02)");
  if (!session.has_value()) {
    return;
  }
  check(session->engine == sg::TracerEngine::OSNOISE,
        "the session carries the engine (E-18)");
  check(session->cpus == cpus, "the session carries the cpu list (E-18)");
  check(session->previousTracer == "nop",
        "the session carries the standing word (T008, R-04)");
  check(session->previousCpuMask == "ffffffff",
        "the session carries the standing mask text (T008, R-04)");
  check(
      readStream((guard.root() / "sys/kernel/tracing/current_tracer").string())
          == "osnoise\n",
      "the setup writes the engine word (T008, FR-007)");
  check(readStream((guard.root() / "sys/kernel/tracing/tracing_cpumask")
                       .string())
            == maskTextOf(cpus, 8) + "\n",
        "the setup writes the tracer mask in the standing width "
        "(T008, R-02, R-05)");
  check(readStream((guard.root() / "sys/kernel/tracing/tracing_on").string())
            == "1\n",
        "the setup writes tracing_on at 1 (T008, R-02)");

  sg::detail::noiseTracerRestore(*session);
  check(readStream((guard.root() / "sys/kernel/tracing/tracing_on").string())
            == "0\n",
        "the restore writes tracing_on at 0 (T008, R-04)");
  check(
      readStream((guard.root() / "sys/kernel/tracing/current_tracer").string())
          == "nop\n",
      "the restore returns the standing word (T008, FR-007)");
  check(
      readStream((guard.root() / "sys/kernel/tracing/tracing_cpumask").string())
          == "ffffffff\n",
      "the restore returns the standing mask (T008, FR-007)");
}

// T008, R-02, R-05: the mask render takes its width from the standing
// text, no width stands fixed: a standing word of two digits renders a
// two-digit mask.
auto theMaskRenderTakesTheWidthTheStandingTextCarried() -> void
{
  const auto cpus = pickCpus(1);
  TracingRootGuard guard;
  installStandingTree(guard.root(), "ff");
  const auto session =
      sg::detail::noiseTracerSetup(sg::TracerEngine::TIMERLAT, cpus);
  check(session.has_value(), "the setup answers the session (T008)");
  if (!session.has_value()) {
    return;
  }
  check(
      readStream((guard.root() / "sys/kernel/tracing/tracing_cpumask").string())
          == maskTextOf(cpus, 2) + "\n",
      "the tracer mask renders in the standing width (T008, R-05)");
  sg::detail::noiseTracerRestore(*session);
}

// T009, FR-010, FR-017, R-07: one entry per tracer cpu in session
// order, the five figures read from the per-cpu count files, the
// source word `hw` mapping to the hardware figure.
auto theFiguresReadOneEntryPerTracerCpu() -> void
{
  const auto cpus = pickCpus(2);
  TracingRootGuard guard;
  installStandingTree(guard.root(), "ffffffff");
  writeTracingFile(guard.root(),
                   cpuCountFile(cpus.at(0)),
                   "nmi: 1\nirq: 2\nsoftirq: 3\nthread: 4\nhw: 5\n");
  writeTracingFile(guard.root(),
                   cpuCountFile(cpus.at(1)),
                   "nmi: 6\nirq: 7\nsoftirq: 8\nthread: 9\nhw: 10\n");
  // The reversed list binds the session order: the read answers the
  // cpus as the session carries them (T009, R-07).
  const std::vector<std::uint32_t> sessionCpus {cpus.at(1), cpus.at(0)};
  const auto session =
      sg::detail::noiseTracerSetup(sg::TracerEngine::OSNOISE, sessionCpus);
  check(session.has_value(), "the setup answers the session (T009)");
  if (!session.has_value()) {
    return;
  }
  const auto figures = sg::detail::noiseTracerFigures(*session);
  check(figures.size() == 2,
        "the read answers one entry per tracer cpu (T009, FR-010)");
  if (figures.size() != 2) {
    return;
  }
  check(figures.at(0).cpu == cpus.at(1) && figures.at(1).cpu == cpus.at(0),
        "the entries stand in the session order (T009, R-07)");
  check(figures.at(0).nmi == 6 && figures.at(0).irq == 7
            && figures.at(0).softirq == 8 && figures.at(0).thread == 9
            && figures.at(0).hardware == 10,
        "the figures read the count file of the first session cpu "
        "(T009, FR-010)");
  check(figures.at(1).nmi == 1 && figures.at(1).irq == 2
            && figures.at(1).softirq == 3 && figures.at(1).thread == 4
            && figures.at(1).hardware == 5,
        "the figures read the count file of the second session cpu, "
        "hw mapping to hardware (T009, FR-010)");
}

// T009, FR-010, R-07: a source the count file states no figure for
// stands absent for that source alone.
auto theMissingSourceWordStandsAbsentForThatSourceAlone() -> void
{
  const auto cpus = pickCpus(2);
  TracingRootGuard guard;
  installStandingTree(guard.root(), "ffffffff");
  writeTracingFile(guard.root(),
                   cpuCountFile(cpus.at(0)),
                   "nmi: 1\nirq: 2\nthread: 4\nhw: 5\n");
  writeTracingFile(guard.root(),
                   cpuCountFile(cpus.at(1)),
                   "nmi: 6\nirq: 7\nsoftirq: 8\nthread: 9\n");
  const auto session =
      sg::detail::noiseTracerSetup(sg::TracerEngine::OSNOISE, cpus);
  check(session.has_value(), "the setup answers the session (T009)");
  if (!session.has_value()) {
    return;
  }
  const auto figures = sg::detail::noiseTracerFigures(*session);
  check(figures.size() == 2,
        "the read answers one entry per tracer cpu (T009, FR-010)");
  if (figures.size() != 2) {
    return;
  }
  check(figures.at(0).nmi.has_value() && figures.at(0).irq.has_value()
            && !figures.at(0).softirq.has_value()
            && figures.at(0).thread.has_value()
            && figures.at(0).hardware.has_value(),
        "the omitted softirq word stands absent for that source alone "
        "(T009, FR-010)");
  check(figures.at(1).nmi.has_value() && figures.at(1).irq.has_value()
            && figures.at(1).softirq.has_value()
            && figures.at(1).thread.has_value()
            && !figures.at(1).hardware.has_value(),
        "the omitted hw word stands absent for that source alone "
        "(T009, FR-010)");
}

// T009, R-07: a cpu whose count file states nothing at read time
// stands every figure absent: the empty file, the missing file and the
// malformed line each stand the whole entry absent, and no exception
// reaches the run (FR-016).
auto theCpuWithNoCountFiguresStandsEveryFigureAbsent(std::size_t which) -> void
{
  const auto cpus = pickCpus(1);
  const auto cpu = cpus.at(0);
  const std::string tag =
      "empty file, missing file or malformed line " "case "
      + std::to_string(which);
  TracingRootGuard guard;
  installStandingTree(guard.root(), "ffffffff");
  if (which == 0) {
    writeTracingFile(guard.root(), cpuCountFile(cpu), "");
  } else if (which == 1) {
    // No count file stands written for the cpu (T009, R-07).
  } else {
    writeTracingFile(
        guard.root(), cpuCountFile(cpu), "nmi: 1\nirq broken\nsoftirq: 3\n");
  }
  const auto session =
      sg::detail::noiseTracerSetup(sg::TracerEngine::OSNOISE, cpus);
  check(session.has_value(), "the setup answers the session (T009)");
  if (!session.has_value()) {
    return;
  }
  const auto figures = sg::detail::noiseTracerFigures(*session);
  check(figures.size() == 1,
        "the read answers one entry per tracer cpu (T009, FR-010)");
  if (figures.size() != 1) {
    return;
  }
  check(!figures.at(0).nmi.has_value() && !figures.at(0).irq.has_value()
            && !figures.at(0).softirq.has_value()
            && !figures.at(0).thread.has_value()
            && !figures.at(0).hardware.has_value(),
        ("the cpu stands every figure absent (T009, R-07): " + tag).c_str());
}

// T010, FR-016, FR-017, D-5, R-02: a scratch root without the tracing
// subtree answers the empty session, and no exception reaches the run.
auto theMissingTracingRootAnswersTheEmptySession() -> void
{
  TracingRootGuard guard;
  const auto session =
      sg::detail::noiseTracerSetup(sg::TracerEngine::OSNOISE, pickCpus(1));
  check(!session.has_value(),
        "the root without the tracing subtree answers the empty "
        "session (T010, D-5)");
}

// T010, FR-016, D-5, R-02: a `current_tracer` the uid cannot write
// answers the empty session; the tests run unprivileged and need no
// root (FR-017).
auto theUnwritableTracerWordAnswersTheEmptySession() -> void
{
  const auto cpus = pickCpus(1);
  TracingRootGuard guard;
  installStandingTree(guard.root(), "ffffffff");
  const auto wordPath = guard.root() / "sys/kernel/tracing/current_tracer";
  check(::chmod(wordPath.c_str(), 0444) == 0,
        "the word file stands read-only (T010)");
  const auto session =
      sg::detail::noiseTracerSetup(sg::TracerEngine::OSNOISE, cpus);
  check(!session.has_value(),
        "the unwritable word answers the empty session (T010, D-5)");
}

// T010, FR-016, D-5, R-02: the tracing subtree without the word file
// stands the engine write failing to open, the empty session.
auto theFailedEngineWriteAnswersTheEmptySession() -> void
{
  TracingRootGuard guard;
  std::filesystem::create_directories(guard.root() / "sys/kernel/tracing");
  const auto session =
      sg::detail::noiseTracerSetup(sg::TracerEngine::TIMERLAT, pickCpus(1));
  check(!session.has_value(),
        "the failed engine write answers the empty session (T010, D-5)");
}

// T010, FR-016, D-5, R-02: a denied mask write and a denied start
// write each answer the empty session, the word file standing
// writable.
auto theDeniedSetupWritesAnswerTheEmptySession(std::size_t which) -> void
{
  const auto cpus = pickCpus(1);
  TracingRootGuard guard;
  installStandingTree(guard.root(), "ffffffff");
  const char* denied = which == 0 ? "sys/kernel/tracing/tracing_cpumask"
                                  : "sys/kernel/tracing/tracing_on";
  check(::chmod((guard.root() / denied).c_str(), 0444) == 0,
        "the denied file stands read-only (T010)");
  const auto session =
      sg::detail::noiseTracerSetup(sg::TracerEngine::OSNOISE, cpus);
  check(!session.has_value(),
        "the denied setup write answers the empty session (T010, D-5)");
}

}  // namespace

SG_BENCHMARK(bmQuiet);

auto main() -> int
{
  theDefaultOptionsCarryNoTracer();
  theDefaultRecordCarriesNoTracer();
  theEngineWordSettlesTheCarrier();
  theCpusKeySettlesTheTracerCpus();
  theSecondCpusKeyStandsTheRepeatedKeyRefusal();
  theBareOptionTakesTheMissingArgumentPath();
  theEngineWordOutsideTheTwoRefuses();
  theCpusTokenOutsideTheListRulesRefuses();
  theCpuOutsideTheAffinityMaskRefuses();
  theCpuOutsideTheOnlineSetRefuses();
  theCpuStandingTwiceRefuses();
  theRepeatedKeyRefuses();
  theOptionFileFaultNamesThePathTheLineAndTheToken();
  theSetupWritesTheEngineTheMaskAndTheStart();
  theMaskRenderTakesTheWidthTheStandingTextCarried();
  theFiguresReadOneEntryPerTracerCpu();
  theMissingSourceWordStandsAbsentForThatSourceAlone();
  theCpuWithNoCountFiguresStandsEveryFigureAbsent(0);
  theCpuWithNoCountFiguresStandsEveryFigureAbsent(1);
  theCpuWithNoCountFiguresStandsEveryFigureAbsent(2);
  theMissingTracingRootAnswersTheEmptySession();
  theUnwritableTracerWordAnswersTheEmptySession();
  theFailedEngineWriteAnswersTheEmptySession();
  theDeniedSetupWritesAnswerTheEmptySession(0);
  theDeniedSetupWritesAnswerTheEmptySession(1);
  theCommandLineSettlesOverTheOptionFile();
  theOptionFileSettlesOverTheSuiteCall();
  std::printf(failures == 0 ? "ok\n" : "%d checks failed\n", failures);
  return failures == 0 ? 0 : 1;
}
