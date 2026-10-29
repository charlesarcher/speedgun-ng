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

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <expected>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iomanip>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <sched.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "detail/internal.hpp"
#include "detail/jitter_probe.hpp"
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
    ref.addCounter("machine", "monotonic", "nanoseconds", "host time");
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
// binding carries the pre-check mode, the probe cpus and the bounded
// window the run needs, so the record the row reads stands built
// (specs/034 R-01).
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
    // The probe cpus the window measures and the bounded window the
    // batch needs: the batch's plan names no cpu, so the suite binding
    // carries both (specs/034 R-01, FR-010).
    suiteOptions.cpus = {cpus.at(1)};
    suiteOptions.precheckMs = 1;
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
// The coverage build's forked child stands the gcov dump error the
// compare row's tolerance names, so the count reads the stream beside
// it (the specs/032 record's shape).
auto checkRefusalShape(const Captured& run, const std::string& prefix) -> void
{
  std::string err;
  std::size_t start = 0;
  while (start <= run.err.size()) {
    const auto end = run.err.find('\n', start);
    const std::string line = run.err.substr(
        start, end == std::string::npos ? std::string::npos : end - start + 1);
    if (line.find("libgcov profiling error:") == std::string::npos) {
      err += line;
    }
    if (end == std::string::npos) {
      break;
    }
    start = end + 1;
  }
  check(run.status == 2, "the refused value stands status 2 (FR-003, SC-003)");
  check(lineCount(err) == 1, "the refusal stands one line (FR-003, SC-003)");
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
// through the standing cpu-list reader, the form the `--topology` listing
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

// T004, FR-002, D-4: `--os-noise` with no argument takes the standing
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

// ---- The pre-check integration rows (T020 to T023, FR-008, FR-009,
// FR-011, FR-016, R-05, R-07, R-09, R-10): the run stands the tracer up
// before the probe items are dispatched and reads the figures after
// JitterProbe::start() returns; the cpus the carrier names and the
// candidate default; the not-available run stays whole at exit 0; the
// missing-window refusal. The scenarios drive speedgunMain in this
// process over the injected layout and the fake tracefs trees, and the
// window stays bounded by the scripted reader, the timing bound
// FR-019 records.

// The integration rows' benchmark: two contexts on the `l3` plan over
// the injected layout, so the measured set carries the planned cpus and
// the candidates beside them (specs/034 R-01).
auto bmPinnedNoise(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

// The two-group layout of the integration rows, built over four picked
// cpus the way the standing pre-check scenarios build theirs: two
// groups of two single-thread cores, so a group beside a planned cpu
// names a free candidate (specs/034 T007).
auto twoGroupNoiseLayout(const std::vector<std::uint32_t>& cpus)
    -> sg::detail::MachineLayout
{
  sg::detail::MachineLayout machine;
  machine.readable = true;
  machine.packages = 1;
  for (std::uint32_t core = 0; core < 4; ++core) {
    sg::detail::LayoutCore layoutCore;
    layoutCore.groupOrdinal = core / 2;
    sg::detail::LayoutThread thread;
    thread.cpuIndex = cpus.at(core);
    thread.coreOrdinal = core;
    thread.groupOrdinal = core / 2;
    layoutCore.threads.push_back(core);
    machine.threads.push_back(thread);
    machine.mask.push_back(cpus.at(core));
    machine.cores.push_back(layoutCore);
  }
  for (std::uint32_t group = 0; group < 2; ++group) {
    sg::detail::LayoutGroup layoutGroup;
    layoutGroup.lowestCpu = cpus.at(group * 2);
    layoutGroup.cores = {group * 2U, group * 2U + 1U};
    machine.groups.push_back(layoutGroup);
  }
  return machine;
}

// The first line of one fake tree file, trimmed of the line break: the
// form the control files carry.
auto firstLineOf(const std::filesystem::path& path) -> std::string
{
  std::ifstream file(path);
  std::string line;
  std::getline(file, line);
  return line;
}

// One line of one captured stream.
auto lineCountOf(const std::string& text) -> std::size_t
{
  std::size_t count = 0;
  for (const char current : text) {
    if (current == '\n') {
      ++count;
    }
  }
  return count;
}

// The fake count file paths of one tracer cpu list (R-03).
auto countPathsOf(const std::filesystem::path& root,
                  const std::vector<std::uint32_t>& cpus)
    -> std::vector<std::filesystem::path>
{
  std::vector<std::filesystem::path> paths;
  for (const auto cpu : cpus) {
    paths.push_back(root / cpuCountFile(cpu));
  }
  return paths;
}

// The tally the scripted reader leaves: the reads it served, the
// sampled reads that saw the tracer down, the first check's answer,
// and the distinct mask texts the checks saw. The probe holder owns
// the reader and destroys it before the run returns, so the
// destructor hands the tally to the test through the file (FR-011).
struct TracerTally
{
  std::uint64_t reads = 0;
  std::uint64_t downs = 0;
  bool firstUp = false;
  std::set<std::string> masks;
};

// The scripted reader of the integration rows (the standing
// reader-install seam): the stamp steps by one tick, so the probe
// keeps no gap, and every 1024th read checks the fake tree's control
// files. The first read rewrites the tracer cpus' count files with the
// post-window figures, so the record's figures pin the figure read
// that stands behind the window (FR-009, R-05). The window stays
// bounded by the reader's stamp series, the timing bound the
// scripted reader carries (FR-011).
class TracerWindowReader final : public sg::counters::WindowReader
{
public:
  TracerWindowReader(std::filesystem::path root,
                     std::vector<std::filesystem::path> countPaths,
                     std::string postFigures,
                     std::string expectedWord,
                     std::uint64_t stampStep,
                     std::filesystem::path tallyPath)
      : m_root(std::move(root))
      , m_countPaths(std::move(countPaths))
      , m_postFigures(std::move(postFigures))
      , m_expectedWord(std::move(expectedWord))
      , m_stampStep(stampStep)
      , m_tallyPath(std::move(tallyPath))
  {
  }

  ~TracerWindowReader()
  {
    std::ofstream file(m_tallyPath);
    file << "reads=" << m_reads.load() << "\n";
    file << "downs=" << m_downs.load() << "\n";
    file << "firstUp=" << (m_firstUp.load() ? 1 : 0) << "\n";
    const std::lock_guard<std::mutex> guard(m_masksMutex);
    for (const auto& mask : m_masks) {
      file << "mask=" << mask << "\n";
    }
  }

  auto readPoints(sg::counters::PointSink& sink) noexcept -> void override
  {
    static thread_local std::uint64_t stamp = 0;
    const auto index = m_reads.fetch_add(1, std::memory_order_relaxed);
    if (index == 0) {
      for (const auto& path : m_countPaths) {
        std::ofstream file(path);
        file << m_postFigures;
      }
    }
    if (index % 1024 == 0) {
      const auto word =
          firstLineOf(m_root / "sys/kernel/tracing/current_tracer");
      const auto switchText =
          firstLineOf(m_root / "sys/kernel/tracing/tracing_on");
      const auto maskText =
          firstLineOf(m_root / "sys/kernel/tracing/tracing_cpumask");
      if (word == m_expectedWord && switchText == "1") {
        m_firstUp.store(true, std::memory_order_relaxed);
      } else {
        m_downs.fetch_add(1, std::memory_order_relaxed);
      }
      {
        const std::lock_guard<std::mutex> guard(m_masksMutex);
        m_masks.insert(maskText);
      }
    }
    sink.put(stamp += m_stampStep);
  }

private:
  std::filesystem::path m_root;
  std::vector<std::filesystem::path> m_countPaths;
  std::string m_postFigures;
  std::string m_expectedWord;
  std::uint64_t m_stampStep = 1;
  std::filesystem::path m_tallyPath;
  std::atomic<std::uint64_t> m_reads {0};
  std::atomic<std::uint64_t> m_downs {0};
  std::atomic<bool> m_firstUp {false};
  std::mutex m_masksMutex;
  std::set<std::string> m_masks;
};

auto readTracerTally(const std::filesystem::path& path) -> TracerTally
{
  TracerTally tally;
  std::ifstream file(path);
  std::string line;
  while (std::getline(file, line)) {
    if (line.starts_with("reads=")) {
      tally.reads = std::stoull(line.substr(6));
    } else if (line.starts_with("downs=")) {
      tally.downs = std::stoull(line.substr(6));
    } else if (line.starts_with("firstUp=")) {
      tally.firstUp = line.substr(8) == "1";
    } else if (line.starts_with("mask=")) {
      tally.masks.insert(line.substr(5));
    }
  }
  return tally;
}

// One speedgunMain run in this process with stdout captured, the way
// the jitter window test captures: the report lands in a scratch file
// beside the redirected descriptor, and the run stands status 0.
auto captureRun(const std::vector<std::string>& arguments) -> std::string
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_noise_test"));
  for (const auto& argument : arguments) {
    argv.push_back(const_cast<char*>(argument.c_str()));
  }
  static int runIndex = 0;
  const std::string path =
      "harness_noise_run_out_" + std::to_string(++runIndex) + ".txt";
  const int saved = ::dup(STDOUT_FILENO);
  if (saved < 0 || std::freopen(path.c_str(), "w", stdout) == nullptr) {
    fail("cannot redirect the report");
  }
  const int status =
      sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
  std::fflush(stdout);
  if (::dup2(saved, STDOUT_FILENO) < 0) {
    fail("cannot restore stdout");
  }
  ::close(saved);
  if (status != 0) {
    fail("the integration run stands status 0 (FR-016)");
  }
  const auto console = readStream(path);
  std::remove(path.c_str());
  return console;
}

// The figures the count files carry before the run, and the figures the
// reader's first read writes in their place: the record must name the
// post-window ones (FR-009, R-05).
inline const std::string kPreWindowFigures =
    "nmi: 1\nirq: 2\nsoftirq: 3\nthread: 4\nhw: 5\n";
inline const std::string kPostWindowFigures =
    "nmi: 11\nirq: 12\nsoftirq: 13\nthread: 14\nhw: 15\n";

// The five post-window figures, in the NMI, IRQ, softirq, thread and
// hardware order the record carries (FR-010).
auto checkPostWindowFigures(const sg::PrecheckTracerCpuRecord& entry,
                            const char* what) -> void
{
  check(entry.nmi.has_value() && *entry.nmi == 11 && entry.irq.has_value()
            && *entry.irq == 12 && entry.softirq.has_value()
            && *entry.softirq == 13 && entry.thread.has_value()
            && *entry.thread == 14 && entry.hardware.has_value()
            && *entry.hardware == 15,
        what);
}

// T020, FR-009, R-05, R-06 of research.md: the engine stands up before
// the probe items are dispatched and the figures stand read after
// JitterProbe::start() returns. The reader's checks stand the
// lifecycle's probes: every read of both serial windows sees the
// engine up, the record's figures name the post-window ones, and the
// tree stands restored after the run (R-04).
auto theWindowLifecycleRows(const std::vector<std::uint32_t>& planned,
                            const std::vector<std::uint32_t>& candidates)
    -> void
{
  // The serial pre-check window: the tracer runs the named candidate
  // across both serial windows, no reset write between them (FR-009,
  // R-06 of research.md).
  {
    TracingRootGuard guard;
    const std::uint32_t tracerCpu = candidates.at(0);
    installStandingTree(guard.root(), "ffffffff");
    writeTracingFile(guard.root(), cpuCountFile(tracerCpu), kPreWindowFigures);
    sg::detail::installTestReader(std::make_unique<TracerWindowReader>(
        guard.root(),
        countPathsOf(guard.root(), {tracerCpu}),
        kPostWindowFigures,
        "osnoise",
        1000,
        "harness_noise_window_serial_tally.txt"));
    const auto console =
        captureRun({"--filter=bmPinnedNoise",
                    "--repetitions=1",
                    "--iterations=2",
                    "--jitter-probe=idle,mode=precheck," "precheck=serial",
                    "--os-noise=osnoise,cpus=" + std::to_string(tracerCpu),
                    "--json=harness_noise_window_serial" ".json"});
    const auto tally = readTracerTally("harness_noise_window_serial_tally.txt");
    const auto loaded = sg::loadReport("harness_noise_window_serial.json");
    check(loaded.has_value(), "the serial run writes the document (FR-005)");
    if (loaded.has_value()) {
      check(loaded->precheck.has_value() && loaded->precheck->serial,
            "the serial window shape rides the record (specs/034 R-06)");
    }
    if (loaded.has_value()) {
      check(loaded->precheck.has_value(),
            "the document carries the pre-check record (FR-014)");
      if (loaded->precheck.has_value()) {
        const auto& tracer = loaded->precheck->tracer;
        check(tracer.has_value(),
              "the record carries the tracer (FR-014, R-05)");
        if (tracer.has_value()) {
          check(tracer->engine == "osnoise",
                "the record names the engine the carrier named (FR-014)");
          check(
              tracer->cpus.size() == 1 && tracer->cpus.front().cpu == tracerCpu,
              "the record names the cpus the carrier named (FR-008)");
          if (tracer->cpus.size() == 1) {
            checkPostWindowFigures(
                tracer->cpus.front(),
                "the figures read the post-window content (FR-009, R-05)");
          }
        }
        check(loaded->precheck->cpus.size() == planned.size()
                                      + candidates.size(),
              "the pre-check measures the planned cpus and the candidates "
              "(specs/034 FR-010)");
      }
    }
    check(tally.reads > 0, "the probe reads ran (R-05)");
    check(tally.downs == 0,
          "every read of both serial windows saw the engine up (FR-009)");
    check(tally.firstUp, "the engine stood up before the reads ran (R-05)");
    check(
        console.find("tracer: engine osnoise cpus " + std::to_string(tracerCpu))
            != std::string::npos,
        "the block names the engine and the cpus (FR-013)");
    check(readStream(guard.root() / "sys/kernel/tracing/current_tracer")
              == "nop\n",
          "the restore returns the standing word (R-04)");
    check(readStream(guard.root() / "sys/kernel/tracing/tracing_cpumask")
              == "ffffffff\n",
          "the restore returns the standing mask (R-04)");
    check(readStream(guard.root() / "sys/kernel/tracing/tracing_on") == "0\n",
          "the restore writes tracing_on at 0 (R-04)");
    std::remove("harness_noise_window_serial.json");
    std::remove("harness_noise_window_serial_tally.txt");
  }

  // The `both` mode: the tracer's default cpus stand the candidates, so
  // the planned cpus stand free of the default tracer load (R-09,
  // R-06 of research.md), and the figures stand read behind the latch
  // the stop lands behind (R-05).
  {
    TracingRootGuard guard;
    installStandingTree(guard.root(), "ffffffff");
    for (const auto cpu : candidates) {
      writeTracingFile(guard.root(), cpuCountFile(cpu), kPreWindowFigures);
    }
    sg::detail::installTestReader(std::make_unique<TracerWindowReader>(
        guard.root(),
        countPathsOf(guard.root(), candidates),
        kPostWindowFigures,
        "osnoise",
        1,
        "harness_noise_window_both_tally.txt"));
    const auto console = captureRun({"--filter=bmPinnedNoise",
                                     "--repetitions=1",
                                     "--iterations=2",
                                     "--jitter-probe=idle,mode=both,precheck=1",
                                     "--os-noise=osnoise",
                                     "--json=harness_noise_window_both.json"});
    const auto tally = readTracerTally("harness_noise_window_both_tally.txt");
    const auto loaded = sg::loadReport("harness_noise_window_both.json");
    check(loaded.has_value(), "the both run writes the document (FR-005)");
    if (loaded.has_value()) {
      check(loaded->precheck.has_value(),
            "the document carries the pre-check record (FR-014)");
      if (loaded->precheck.has_value()) {
        const auto& tracer = loaded->precheck->tracer;
        check(tracer.has_value(),
              "the record carries the tracer (FR-014, R-05)");
        if (tracer.has_value()) {
          check(tracer->engine == "osnoise",
                "the record names the engine (FR-014)");
          check(tracer->cpus.size() == candidates.size(),
                "the absent key names the candidates (FR-008, R-09)");
          bool plannedFree = true;
          for (const auto& entry : tracer->cpus) {
            plannedFree = plannedFree
                && std::find(planned.begin(), planned.end(), entry.cpu)
                    == planned.end();
          }
          check(plannedFree,
                "the planned cpus stand free of the default tracer load "
                "(FR-009, R-09)");
          for (const auto& entry : tracer->cpus) {
            checkPostWindowFigures(
                entry,
                "the figures read the post-window content (FR-009, R-05)");
          }
        }
      }
    }
    check(tally.reads > 0, "the probe reads ran (R-05)");
    check(tally.firstUp, "the engine stood up before the reads ran (R-05)");
    check(readStream(guard.root() / "sys/kernel/tracing/current_tracer")
              == "nop\n",
          "the restore returns the standing word (R-04)");
    check(readStream(guard.root() / "sys/kernel/tracing/tracing_on") == "0\n",
          "the restore writes tracing_on at 0 (R-04)");
    std::remove("harness_noise_window_both.json");
    std::remove("harness_noise_window_both_tally.txt");
  }
}

// T021, FR-008, R-09: a cpus list runs as named, a planned cpu
// included, with no overlap refusal; an absent key names the candidate
// set of the injected topology, one cpu per L3 group the plans touch.
// The mask the setup wrote stands the observable the reader's checks
// read off the fake tree.
auto theTracerCpuRows(const std::vector<std::uint32_t>& planned,
                      const std::vector<std::uint32_t>& candidates) -> void
{
  // The named list: one planned cpu and one candidate, the list as
  // named, no overlap refusal (R-09).
  {
    TracingRootGuard guard;
    const std::vector<std::uint32_t> tracerCpus {planned.at(0),
                                                 candidates.at(0)};
    installStandingTree(guard.root(), "ffffffff");
    for (const auto cpu : tracerCpus) {
      writeTracingFile(guard.root(), cpuCountFile(cpu), kPreWindowFigures);
    }
    sg::detail::installTestReader(std::make_unique<TracerWindowReader>(
        guard.root(),
        countPathsOf(guard.root(), tracerCpus),
        kPostWindowFigures,
        "osnoise",
        1,
        "harness_noise_cpus_named_tally.txt"));
    std::string list;
    for (const auto cpu : tracerCpus) {
      if (!list.empty()) {
        list += ',';
      }
      list += std::to_string(cpu);
    }
    captureRun({"--filter=bmPinnedNoise",
                "--repetitions=1",
                "--iterations=2",
                "--jitter-probe=idle,mode=precheck,precheck=1",
                "--os-noise=osnoise,cpus=" + list,
                "--json=harness_noise_cpus_named.json"});
    const auto tally = readTracerTally("harness_noise_cpus_named_tally.txt");
    const auto loaded = sg::loadReport("harness_noise_cpus_named.json");
    check(loaded.has_value(), "the named run writes the document (FR-005)");
    if (loaded.has_value()) {
      check(loaded->precheck.has_value(),
            "the document carries the pre-check record (FR-014)");
      if (loaded->precheck.has_value()) {
        const auto& tracer = loaded->precheck->tracer;
        check(tracer.has_value(), "the record carries the tracer (FR-014)");
        if (tracer.has_value()) {
          check(tracer->cpus.size() == tracerCpus.size(),
                "the named list runs as named (FR-008)");
          bool named = tracer->cpus.size() == tracerCpus.size();
          for (std::size_t index = 0; named && index < tracerCpus.size();
               ++index)
          {
            named = tracer->cpus.at(index).cpu == tracerCpus.at(index);
          }
          check(
              named,
              "the record names the cpus the list named, ascending " "(FR-"
                                                                     "008)");
          check(tally.downs == 0, "every read saw the engine up (FR-009)");
        }
      }
    }
    const std::set<std::string> wantedMask {maskTextOf(tracerCpus, 8)};
    check(tally.masks == wantedMask,
          "the setup wrote the mask of the named list (FR-008, R-02)");
    std::remove("harness_noise_cpus_named.json");
    std::remove("harness_noise_cpus_named_tally.txt");
  }

  // The absent key: the candidate set of the injected topology (FR-008,
  // R-09).
  {
    TracingRootGuard guard;
    installStandingTree(guard.root(), "ffffffff");
    for (const auto cpu : candidates) {
      writeTracingFile(guard.root(), cpuCountFile(cpu), kPreWindowFigures);
    }
    sg::detail::installTestReader(std::make_unique<TracerWindowReader>(
        guard.root(),
        countPathsOf(guard.root(), candidates),
        kPostWindowFigures,
        "osnoise",
        1,
        "harness_noise_cpus_default_tally.txt"));
    captureRun({"--filter=bmPinnedNoise",
                "--repetitions=1",
                "--iterations=2",
                "--jitter-probe=idle,mode=precheck,precheck=1",
                "--os-noise=osnoise",
                "--json=harness_noise_cpus_default.json"});
    const auto tally = readTracerTally("harness_noise_cpus_default_tally.txt");
    const auto loaded = sg::loadReport("harness_noise_cpus_default.json");
    check(loaded.has_value(), "the default run writes the document (FR-005)");
    if (loaded.has_value()) {
      check(loaded->precheck.has_value(),
            "the document carries the pre-check record (FR-014)");
      if (loaded->precheck.has_value()) {
        const auto& tracer = loaded->precheck->tracer;
        check(tracer.has_value(), "the record carries the tracer (FR-014)");
        if (tracer.has_value()) {
          check(tracer->cpus.size() == candidates.size(),
                "the absent key names the candidates (FR-008, R-09)");
          bool named = tracer->cpus.size() == candidates.size();
          for (std::size_t index = 0; named && index < candidates.size();
               ++index)
          {
            named = tracer->cpus.at(index).cpu == candidates.at(index);
          }
          check(named,
                "the record names the candidates, one per touched group "
                "(FR-008, R-09)");
          check(tally.downs == 0, "every read saw the engine up (FR-009)");
        }
      }
    }
    const std::set<std::string> wantedMask {maskTextOf(candidates, 8)};
    check(tally.masks == wantedMask,
          "the setup wrote the mask of the candidates (FR-008, R-02)");
    std::remove("harness_noise_cpus_default.json");
    std::remove("harness_noise_cpus_default_tally.txt");
  }
}

// The not-available rows' shared shape: the run exits 0, the user-space
// pre-check stands whole, the document carries no tracer key, and the
// tracer lines stand the one disclosure line after the host settings
// and before the warning lines (FR-016, SC-004, R-10, R-14).
auto checkNotAvailableShape(const std::string& console,
                            const std::optional<sg::PrecheckRecord>& record)
    -> void
{
  check(record.has_value(),
        "the document carries the pre-check record (FR-024)");
  if (!record.has_value()) {
    return;
  }
  check(!record->tracer.has_value(),
        "the not-available run writes no tracer key (FR-015, R-10)");
  check(!record->cpus.empty(),
        "the user-space pre-check stands whole (FR-016, SC-004)");
  const auto disclosure = console.find("tracer: not available");
  check(disclosure != std::string::npos,
        "the tracer lines stand the disclosure (FR-016)");
  const auto host = console.find("host settings:");
  const auto candidatesLine = console.find("pinning candidates:");
  check(host != std::string::npos && candidatesLine != std::string::npos
            && host < disclosure && disclosure < candidatesLine,
        "the disclosure stands after the host settings and before the "
        "warning lines (R-14)");
}

// T022, FR-016, SC-004, R-10: a not-available host keeps the run whole
// at exit 0, and a run whose setup writes fail stands the shape.
auto theWholeRunRows() -> void
{
  // The root without the tracing subtree: the setup's first read fails.
  {
    TracingRootGuard guard;
    const auto withTracer = captureRun(
        {"--filter=bmPinnedNoise",
         "--repetitions=1",
         "--iterations=2",
         "--jitter-probe=idle,mode=precheck,precheck=1,threshold=1000000",
         "--os-noise=osnoise",
         "--json=harness_noise_unavailable.json"});
    const auto withoutTracer = captureRun(
        {"--filter=bmPinnedNoise",
         "--repetitions=1",
         "--iterations=2",
         "--jitter-probe=idle,mode=precheck,precheck=1,threshold=1000000",
         "--json=harness_noise_unavailable_free.json"});
    const auto loaded = sg::loadReport("harness_noise_unavailable.json");
    check(loaded.has_value(),
          "the not-available run writes the document (FR-016)");
    checkNotAvailableShape(
        withTracer, loaded.has_value() ? loaded->precheck : std::nullopt);
    const auto loadedFree =
        sg::loadReport("harness_noise_unavailable_free.json");
    check(loadedFree.has_value(),
          ("the tracer-free run writes the document (FR-012): "
           + std::string(loadedFree.has_value() ? "none" : loadedFree.error()))
              .c_str());
    if (loadedFree.has_value()) {
      check(loadedFree->precheck.has_value()
                && !loadedFree->precheck->tracer.has_value(),
            "the tracer-free run writes no tracer key (FR-012, R-10)");
    }
    check(withoutTracer.find("tracer:") == std::string::npos,
          "a run that requests no tracer prints no tracer line (FR-012)");
    std::remove("harness_noise_unavailable.json");
    std::remove("harness_noise_unavailable_free.json");
  }

  // The fake tree's files stand read-only: the setup's writes fail.
  {
    TracingRootGuard guard;
    installStandingTree(guard.root(), "ffffffff");
    for (const char* file : {"sys/kernel/tracing/current_tracer",
                             "sys/kernel/tracing/tracing_cpumask",
                             "sys/kernel/tracing/tracing_on"})
    {
      check(::chmod((guard.root() / file).c_str(), 0444) == 0,
            "the fake control files stand read-only (T022)");
    }
    const auto console = captureRun(
        {"--filter=bmPinnedNoise",
         "--repetitions=1",
         "--iterations=2",
         "--jitter-probe=idle,mode=precheck,precheck=1,threshold=1000000",
         "--os-noise=osnoise",
         "--json=harness_noise_denied.json"});
    const auto loaded = sg::loadReport("harness_noise_denied.json");
    check(loaded.has_value(),
          ("the denied run writes the document (FR-016): "
           + std::string(loaded.has_value() ? "none" : loaded.error()))
              .c_str());
    checkNotAvailableShape(
        console, loaded.has_value() ? loaded->precheck : std::nullopt);
    std::remove("harness_noise_denied.json");
  }
}

// T023, FR-011, R-07: a settled `--os-noise` value beside a batch that
// carries no pre-check window refuses one line at exit 2 before
// anything runs, and the compare mode refuses the same way.
auto theMissingWindowRefusalRows() -> void
{
  const auto run = runRefusalChild("window-absent",
                                   {"--os-noise=osnoise",
                                    "--filter=bmQuiet",
                                    "--repetitions=1",
                                    "--iterations=2"});
  checkRefusalShape(run, "--os-noise: ");

  const auto probe = pickCpus(1);
  const auto alongside =
      runRefusalChild("window-alongside",
                      {"--os-noise=osnoise",
                       "--jitter-probe=" + std::to_string(probe.at(0)),
                       "--filter=bmQuiet",
                       "--repetitions=1",
                       "--iterations=2"});
  checkRefusalShape(alongside, "--os-noise: ");

  // The compare mode refuses the same way (R-07): one line on the
  // refused value's standard error, the line the standing
  // bookkeeping-error filter excepts.
  const auto compare = runRefusalChild(
      "window-compare",
      {"--os-noise=osnoise", "--compare=missing.json,missing2.json"});
  check(compare.status == 2,
        "the compare mode's refused value stands status 2 (FR-011)");
  check(lineCountOf(compare.err) == 1
            || (lineCountOf(compare.err) == 2
                && compare.err.find("libgcov profiling error:")
                    != std::string::npos),
        "the compare mode's refusal stands one line (FR-011)");
  check(compare.err.find("--os-noise: ") != std::string::npos,
        "the compare mode's refusal names the option (FR-011)");
  check(compare.out.empty(),
        "the compare mode's refusal prints no benchmark output (FR-011)");
}

}  // namespace

SG_BENCHMARK(bmQuiet);

// The value-grammar arms the gather routes through the one settle arm:
// a key outside the value's names, a bare item before the key, the
// items after one key extending the list, and the empty item a
// trailing comma names (FR-002, FR-003).
auto theParserArmRows() -> void
{
  {
    const Gathered gathered =
        gatherSources({"--os-noise=osnoise,strict=1"}, "");
    check(!gathered.settled.has_value(),
          "a key outside the value's names refuses (FR-002)");
    check(
        gathered.fault.find("is not one the value names") != std::string::npos,
        ("the refusal names the key (FR-003): " + gathered.fault).c_str());
  }
  {
    const Gathered gathered = gatherSources({"--os-noise=osnoise,2"}, "");
    check(!gathered.settled.has_value(),
          "a bare item before the key refuses (FR-002)");
    check(
        gathered.fault.find("stands outside the cpus key") != std::string::npos,
        ("the refusal names the token (FR-003): " + gathered.fault).c_str());
  }
  {
    const Gathered gathered = gatherSources({"--os-noise=osnoise,cpus=2,"}, "");
    check(!gathered.settled.has_value(),
          "the empty item a trailing comma names refuses (FR-002)");
    check(
        gathered.fault.find("is not a cpu index or range") != std::string::npos,
        ("the refusal names the empty item (FR-003): " + gathered.fault)
            .c_str());
  }
}

// The setup and figure arms of the session over the fake trees: the
// failure arms of FR-016 and the render and parse arms of R-02 and
// R-07, each answered the empty session, the absent figures or the
// figures the file states, with no exception reaching the run.
auto theSessionArmRows() -> void
{
  // The mask render grows past the standing word: a tracer cpu above
  // the standing word's capacity stands a second word, the bits of the
  // first word cleared (R-02, R-05).
  {
    TracingRootGuard guard;
    installStandingTree(guard.root(), "ffffffff");
    const auto session = sg::detail::noiseTracerSetup(
        sg::TracerEngine::OSNOISE, std::vector<std::uint32_t> {35});
    check(session.has_value(),
          "the setup answers the session over the two-word mask (R-02)");
    if (session.has_value()) {
      const auto written =
          firstLineOf(guard.root() / "sys/kernel/tracing/tracing_cpumask");
      check(written == maskTextOf({35}, 8),
            ("the mask renders the two words the cpus need (R-02, R-05): "
             + written)
                .c_str());
      sg::detail::noiseTracerRestore(*session);
    }
  }
  // The standing word text that names no word: the first line all
  // blanks stands an empty previous word, the session carrying it for
  // the restore (R-04).
  {
    TracingRootGuard guard;
    installStandingTree(guard.root(), "ffffffff");
    writeTracingFile(
        guard.root(), "sys/kernel/tracing/current_tracer", "   \nnop\n");
    const auto session =
        sg::detail::noiseTracerSetup(sg::TracerEngine::OSNOISE, pickCpus(1));
    check(session.has_value(),
          "the blank standing word answers the session (R-04)");
    if (session.has_value()) {
      check(session->previousTracer.empty(),
            "the blank standing word stands the empty previous word " "(R-04)");
      sg::detail::noiseTracerRestore(*session);
    }
  }
  // The standing mask text that names no word: the width read answers
  // nothing, the setup the empty session (R-05, FR-016).
  {
    TracingRootGuard guard;
    installStandingTree(guard.root(), "ffffffff");
    writeTracingFile(
        guard.root(), "sys/kernel/tracing/tracing_cpumask", " ,ffffffff\n");
    const auto session =
        sg::detail::noiseTracerSetup(sg::TracerEngine::OSNOISE, pickCpus(1));
    check(!session.has_value(),
          "the blank mask word answers the empty session (R-05, D-5)");
  }
  // The control file that stands a directory: the read carries to no
  // end, the setup the empty session (FR-016, R-01).
  {
    TracingRootGuard guard;
    installStandingTree(guard.root(), "ffffffff");
    const auto wordPath = guard.root() / "sys/kernel/tracing/current_tracer";
    const auto replaced = std::filesystem::remove(wordPath);
    check(replaced, "the word file stands replaced by a directory (R-01)");
    check(std::filesystem::create_directory(wordPath),
          "the word file stands replaced by a directory (R-01)");
    const auto session =
        sg::detail::noiseTracerSetup(sg::TracerEngine::OSNOISE, pickCpus(1));
    check(!session.has_value(),
          "the unreadable word answers the empty session (FR-016, D-5)");
  }
  // The mask file missing: the standing read answers nothing, the
  // setup the empty session (FR-016, R-04).
  {
    TracingRootGuard guard;
    installStandingTree(guard.root(), "ffffffff");
    check(std::filesystem::remove(guard.root()
                                  / "sys/kernel/tracing/tracing_cpumask"),
          "the mask file stands removed (R-04)");
    const auto session =
        sg::detail::noiseTracerSetup(sg::TracerEngine::OSNOISE, pickCpus(1));
    check(!session.has_value(),
          "the missing mask answers the empty session (FR-016, D-5)");
  }
  // The mask file read-only: the mask write fails, the setup the empty
  // session; the tests run unprivileged and need no root (FR-016,
  // FR-017).
  {
    TracingRootGuard guard;
    installStandingTree(guard.root(), "ffffffff");
    const auto maskPath = guard.root() / "sys/kernel/tracing/tracing_cpumask";
    check(::chmod(maskPath.c_str(), 0444) == 0,
          "the mask file stands read-only (FR-016)");
    const auto session =
        sg::detail::noiseTracerSetup(sg::TracerEngine::OSNOISE, pickCpus(1));
    check(!session.has_value(),
          "the unwritable mask answers the empty session (FR-016, D-5)");
  }
  // The on-off file read-only: the start write fails, the setup the
  // empty session (FR-016).
  {
    TracingRootGuard guard;
    installStandingTree(guard.root(), "ffffffff");
    const auto onPath = guard.root() / "sys/kernel/tracing/tracing_on";
    check(::chmod(onPath.c_str(), 0444) == 0,
          "the on-off file stands read-only (FR-016)");
    const auto session =
        sg::detail::noiseTracerSetup(sg::TracerEngine::OSNOISE, pickCpus(1));
    check(
        !session.has_value(),
        "the unwritable on-off file answers the empty session " "(FR-016, "
                                                                "D-5)");
  }
}

// The figure-read arms of the pinned format: the blank line the file
// carries between two sources stands the figures around it, and the
// malformed line shapes stand every figure of the cpu absent (R-07).
auto theFigureArmRows() -> void
{
  const auto cpus = pickCpus(1);
  const auto cpu = cpus.at(0);
  const std::string_view shapes[] {
      "nmi: 1\n\nirq: 2\nsoftirq: 3\nthread: 4\nhw: 5\n",
      "nmi: abc\nirq: 2\nsoftirq: 3\nthread: 4\nhw: 5\n",
      "nmi: 1\nnmi: 2\nirq: 2\nsoftirq: 3\nthread: 4\nhw: 5\n",
      "foo: 1\nnmi: 1\nirq: 2\nsoftirq: 3\nthread: 4\nhw: 5\n",
      "nmi 1\nirq: 2\nsoftirq: 3\nthread: 4\nhw: 5\n",
      "irq: 2\nsoftirq: 3\nthread: 4\nhw: 5\nnmi:\n"};
  for (const auto& shape : shapes) {
    TracingRootGuard guard;
    installStandingTree(guard.root(), "ffffffff");
    writeTracingFile(guard.root(), cpuCountFile(cpu), std::string(shape));
    const auto session =
        sg::detail::noiseTracerSetup(sg::TracerEngine::OSNOISE, cpus);
    check(session.has_value(), "the setup answers the session (R-07)");
    if (!session.has_value()) {
      continue;
    }
    const auto figures = sg::detail::noiseTracerFigures(*session);
    check(figures.size() == 1,
          "the read answers one entry per tracer cpu (R-07)");
    if (figures.size() != 1) {
      continue;
    }
    const bool everyAbsent = !figures.at(0).nmi.has_value()
        && !figures.at(0).irq.has_value() && !figures.at(0).softirq.has_value()
        && !figures.at(0).thread.has_value()
        && !figures.at(0).hardware.has_value();
    const bool blankLine =
        shape.front() == 'n' && shape.find("\n\n") != std::string::npos;
    check(
        everyAbsent != blankLine,
        ("the malformed line stands every figure absent and the blank " "line" " " "stands" " the " "figure" "s " "around" " it " "(R-07)" ": the " "shape "
         + std::string(shape))
            .c_str());
    if (everyAbsent == blankLine) {
      std::printf("the shape: %s\n", std::string(shape).c_str());
    }
  }
}

auto main() -> int
{
  // The scripted provider stands the in-process runs' time source: the
  // tsc series steps one tick per read, so every pre-check window keeps
  // no gap and the stats carry the zero fields, the deterministic shape
  // the suite requires (VI, the timing-margin record of specs/032). The
  // rows that run benchmarks name no figure of their own, so the quiet
  // series serves them all.
  scriptedProvider();
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
  theParserArmRows();
  theSessionArmRows();
  theFigureArmRows();
  theMissingTracingRootAnswersTheEmptySession();
  theUnwritableTracerWordAnswersTheEmptySession();
  theFailedEngineWriteAnswersTheEmptySession();
  theDeniedSetupWritesAnswerTheEmptySession(0);
  theDeniedSetupWritesAnswerTheEmptySession(1);
  theCommandLineSettlesOverTheOptionFile();
  theOptionFileSettlesOverTheSuiteCall();
  // The integration rows (T020 to T022) run in this process over the
  // injected layout, installed here before the first read of it: the
  // layout read caches once per process, so the rows share the one
  // injected shape (T011 of the pre-check suite).
  const auto integrationCpus = pickCpus(4);
  if (integrationCpus.size() == 4) {
    sg::detail::installTestLayout(twoGroupNoiseLayout(integrationCpus));
    auto pinned = sg::registerBenchmark(&bmPinnedNoise, "bmPinnedNoise");
    pinned.threads(2).placement("l3");
    const auto planned = sg::detail::resolvePreset(
                             sg::detail::Preset::L3, sg::detail::layout(), 2)
                             .cpu;
    const auto measured =
        sg::detail::precheckMeasuredSet(sg::detail::layout(), planned);
    std::vector<std::uint32_t> candidates;
    for (const auto cpu : measured.cpus) {
      if (std::ranges::find(planned, cpu) == planned.end()) {
        candidates.push_back(cpu);
      }
    }
    check(!candidates.empty() && !planned.empty(),
          "the injected layout resolves the planned cpus and the "
          "candidates (R-09)");
    theWindowLifecycleRows(planned, candidates);
    theTracerCpuRows(planned, candidates);
    theWholeRunRows();
  } else {
    std::printf("the noise integration rows: SKIPPED (the mask holds "
                "fewer than four cpus)\n");
  }
  theMissingWindowRefusalRows();
  std::printf(failures == 0 ? "ok\n" : "%d checks failed\n", failures);
  return failures == 0 ? 0 : 1;
}
