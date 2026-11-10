// ============================================================================
// The raw event-code address through the harness (specs/051-raw-events T007;
// US1, US2, US3; FR-002, FR-004, FR-005, FR-006, FR-008, FR-009, FR-011).
//
// One option carries the address: `--counter=<device>/r<hex>`. The leaf spells
// a config word, the device part names the PMU, and the address as typed is
// the column's label. Every scenario runs a scripted kernel under the real PMU
// provider, the shape of `harness_limit_disclosure_test`, so no case depends
// on what this host's kernel grants and no case names a made-up cpu or a
// made-up kernel figure.
//
// The scripted devices root publishes one `cpu` device of PMU type 4 with two
// named events and one `uncore` device that carries a `cpumask` file and is
// therefore scoped. The scripted open records the PMU type and the config word
// of every call it answers or refuses, so a scenario can weigh the opens a
// leaf did not pay for, which the harness's own text cannot show. Each
// scenario runs in a re-exec of this binary, because one process publishes one
// catalog and one scripted answer set, the way the one-script-per-mode re-exec
// of `harness_gap_test` states. The child prints the figure its own host
// published and the record of its own opens on machine-readable lines ahead of
// the harness text, so every figure a parent weighs came from the child that
// ran.
//
// Scenario map:
//   unknownDeviceRawLine              the unpublished device answers the
//                                     standing no-object line, and no scripted
//                                     open carries the word
//   wideLeafWidthLine                 a seventeen-digit leaf answers the width
//                                     line at the leaf-match stage and opens
//                                     nothing
//   wideLeafBeatsAbsentDevice         the width line stands ahead of the
//                                     device read
//   refusedProbeListedAsLines         a scripted encoding errno and a scripted
//                                     permission errno answer their own states
//   scopeRefusedRawLine               the scripted cpumask device relabels a
//                                     permission errno to its scope's own
//                                     state
//   refusedRawEndsRunBeforeMeasurement
//                                     one refused address among good ones ends
//                                     the run before a row prints
//   duplicateRawAddressTwoColumns     one address written twice prints two
//                                     labels over one open set for the word
//   rawColumnDisclosesEverywhere      the option line, the console label, the
//                                     metric labels and each row's label carry
//                                     the address, and the document validates
//   limitLineCountsRawColumn          the raw column seats against the
//                                     scripted figure and the standing limit
//                                     line keeps its bytes
//   rawRunConsoleAndJson              the scripted run prints the column with
//                                     its figure and writes the same figure
//   absentPmuPathRawRefusal           a raw address on the machine object,
//                                     which holds no PMU device, answers the
//                                     standing no-object line and opens
//                                     nothing
//   libraryRouteKeepsAbsentColumn     a library caller's raw address on that
//                                     path keeps an absent column and the run
//                                     proceeds
//
// Frameworkless check() convention, with main calling each case once: every
// failed check prints one line naming the scenario that weighed it and the
// behavior it asked for, and the run keeps weighing what is left, so one direct
// run names every scenario the base tree leaves red. main answers nonzero when
// any such line printed. fail() stands only where the script itself cannot
// stand: a scripted root that did not build, a provider that did not register,
// a child that did not spawn. The unit builds at the lane base and reads red at
// run time: the raw arm of the counters lookup and the harness arms of T016 and
// T017 stand behind the answers weighed here.
// ============================================================================
#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <linux/perf_event.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../source/counters/detail/limit_source_record.hpp"
#include "../source/counters/linux_pmu/kernel_calls.hpp"
#include "../source/harness/detail/internal.hpp"
#include "../source/harness/detail/json_report.hpp"
#include "detail/gcov_flush.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_clock.hpp"
#include "speedgun-ng/counters_pmu.hpp"
#include "speedgun-ng/counters_system.hpp"

namespace
{

using sg::counters::detail::installKernelCalls;
using sg::counters::detail::KernelCalls;

[[noreturn]] auto fail(const char* const what) -> void
{
  std::fprintf(stderr, "HARNESS RAW EVENTS TEST ABORT: %s\n", what);
  std::exit(1);
}

// One failed check: the scenario that weighed it, the behavior it asked for,
// and the pair of answer and expectation where the check compares texts.
struct Failure
{
  std::string scenario;
  std::string what;
  std::string answer {};
  std::string expected {};
};

// The scenario under way and every failed check of the run. A child re-exec
// fills neither: a child answers its own exit status, and the parent's record
// of that status is where a child fault reaches the run.
std::vector<Failure> gFailures;
std::string gScenario;

auto record(const std::string_view what) -> void
{
  gFailures.push_back(
      Failure {.scenario = gScenario, .what = std::string(what)});
  std::printf("HARNESS RAW EVENTS TEST FAIL: %s: %s\n",
              gScenario.c_str(),
              std::string(what).c_str());
  std::fflush(stdout);
}

auto recordText(const std::string_view what,
                const std::string& answer,
                const std::string& expected) -> void
{
  gFailures.push_back(Failure {.scenario = gScenario,
                               .what = std::string(what),
                               .answer = answer,
                               .expected = expected});
  std::printf("HARNESS RAW EVENTS TEST FAIL: %s: %s\n  answer:   '%s'\n  "
              "expected: '%s'\n",
              gScenario.c_str(),
              std::string(what).c_str(),
              answer.c_str(),
              expected.c_str());
  std::fflush(stdout);
}

auto check(const bool condition, const std::string_view what) -> void
{
  if (!condition) {
    record(what);
  }
}

auto checkText(const std::string& answer,
               const std::string& expected,
               const std::string_view what) -> void
{
  if (answer != expected) {
    recordText(what, answer, expected);
  }
}

// The scripted devices, their events, and the addresses the scenarios name.
// The core device carries a host device's own name on purpose: the collision
// makes the entry point's own registration of a second PMU provider refuse for
// the run, so no case reads a counter the script did not answer.
constexpr std::string_view kDevice {"cpu"};
constexpr std::string_view kScopedDevice {"uncore"};
constexpr std::string_view kFirstEvent {"disc_a"};
constexpr std::string_view kSecondEvent {"disc_b"};
constexpr std::string_view kScopedEvent {"unc_a"};
constexpr std::string_view kFirstAddress {"cpu/disc_a"};
constexpr std::string_view kSecondAddress {"cpu/disc_b"};

// The PMU type each scripted device publishes and the config word of each of
// its events. The core device's type is the number the scripted pin names
// (SC-001), and every event word sits far below kWideBound.
constexpr std::uint32_t kDeviceType = 4;
constexpr std::uint32_t kScopedType = 7;
constexpr std::uint64_t kFirstWord = 0x2d;
constexpr std::uint64_t kSecondWord = 0x31;
constexpr std::uint64_t kScopedWord = 0x66;

// The raw addresses the scenarios name: `cpu/rc0` carries the word the scripted
// pin names beside the device type, `cpu/r4000` carries a word no scripted
// event shares, `uncore/r4000` reaches the scripted scoped device, and
// `machine/r4000` names the machine object, at which the scripted PMU
// provider holds no device.
constexpr std::string_view kSmallRaw {"cpu/rc0"};
constexpr std::string_view kRawAddress {"cpu/r4000"};
constexpr std::string_view kScopedRaw {"uncore/r4000"};
constexpr std::string_view kUnknownRaw {"nosuch/r4000"};
constexpr std::string_view kMachineRaw {"machine/r4000"};
constexpr std::uint64_t kSmallRawWord = 0xc0;
constexpr std::uint64_t kRawWord = 0x4000;

// A leaf seventeen hex digits wide, and the word the same text answers once cut
// to sixteen digits. kWideBound sits above every word the scripted fixture can
// open, so an open at or above it names a wide leaf that reached the kernel
// after all (FR-005).
constexpr std::string_view kWideAddress {"cpu/r1234567890abcdef0"};
constexpr std::string_view kWideUnknown {"nosuch/r1234567890abcdef0"};
constexpr std::uint64_t kTruncatedWord = 0x123456789abcdefULL;
constexpr std::uint64_t kWideBound = 0x1000000000000ULL;

// The scripted window read: one member value and one enabled/running pair per
// read, each one step larger than the last read answered, so the two endpoint
// reads a measuring window folds leave a positive delta of exactly this step
// beside a ratio of one. The header words stand ahead of the member values
// (source/counters/linux_pmu/group_io.cpp:243-245).
constexpr std::uint64_t kReadStep = 1000U;
constexpr std::uint64_t kPairStart = 5000U;
constexpr std::size_t kGroupHeaderWords = 3;

// The scripted kernel's state: the two paths its file reads answer, the record
// of its opens, the refusal one config word carries, the figures its reads
// step, and the run marker.
std::filesystem::path gDevicesRoot;
std::filesystem::path gParanoidPath;
int gNextFd = 100;
std::uint64_t gRefuseWord = 0;
int gRefuseErrno = 0;
std::uint64_t gReadValue = 0;
std::uint64_t gReadPair = kPairStart;
int gBenchmarkRuns = 0;

struct OpenedPair
{
  std::uint64_t type = 0;
  std::uint64_t config = 0;
};

std::vector<OpenedPair> gOpens;

auto hexOf(const std::uint64_t value) -> std::string
{
  char text[17];
  std::snprintf(
      text, sizeof(text), "%llx", static_cast<unsigned long long>(value));
  return text;
}

// The scripted `perf_event_open`: every call stands in the record under the
// device type and the config word it carries, and one config word answers the
// errno the scenario tuned.
auto scriptedOpen(void* attr, int, int, int, unsigned long) -> long
{
  const auto* const event = static_cast<const perf_event_attr*>(attr);
  gOpens.push_back(OpenedPair {.type = event->type, .config = event->config});
  if (gRefuseWord != 0 && event->config == gRefuseWord) {
    errno = gRefuseErrno;
    return -1;
  }
  return gNextFd++;
}

// The read of a group leader in the layout the group fold reads: the member
// count, the enabled and running pair, then one count per member. The window
// asks for the room of the group it opened, so the room names the member count,
// and the pair stands equal within one read so the ratio the fold carries is
// one. Each read steps the figures it answered, so the pair of endpoint reads a
// measuring window folds differs by one step.
auto scriptedRead(int, void* buffer, std::size_t count) -> long
{
  auto* const posted = static_cast<std::uint64_t*>(buffer);
  const auto room = count / sizeof(std::uint64_t);
  if (room <= kGroupHeaderWords) {
    return -1;
  }
  const auto members = room - kGroupHeaderWords;
  posted[0] = members;
  posted[1] = gReadPair;
  posted[2] = gReadPair;
  for (std::size_t member = 0; member < members; ++member) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    posted[kGroupHeaderWords + member] = gReadValue;
  }
  gReadValue += kReadStep;
  gReadPair += kReadStep;
  // The cast names the one signed type the seam's answer settles in.
  return static_cast<long>(count);
}

auto scriptedClose(int) -> int
{
  return 0;
}

// The words no vendored table row matches, so the scripted catalog holds the
// scripted device's own events and nothing a host's vendor table would add.
auto noRowWords() -> sg::counters::detail::CpuidWords
{
  return {.leaf0Eax = 0x15U,
          .leaf0Ebx = 0x75536F4EU,  // "Nosu"
          .leaf0Edx = 0x6E655663U,  // "cVen"
          .leaf0Ecx = 0x58726F64U,  // "dorX"
          .leaf1Answered = true,
          .leaf1Eax = 0x30650U};
}

auto scriptedCalls() -> KernelCalls
{
  KernelCalls calls {};
  calls.openEvent = &scriptedOpen;
  calls.mapPage = [](void*, std::size_t, int, int, int, long long) -> void*
  { return MAP_FAILED; };
  calls.unmapPage = [](void*, std::size_t) -> int { return 0; };
  calls.readGroup = &scriptedRead;
  calls.ioctlEvent = [](int, unsigned long, unsigned long) -> int { return 0; };
  calls.closeFd = &scriptedClose;
  calls.devicesRoot = []() -> std::filesystem::path { return gDevicesRoot; };
  calls.paranoidPath = []() -> std::filesystem::path { return gParanoidPath; };
  calls.cpuidWords = &noRowWords;
  return calls;
}

// One scripted device directory: a `type` file, the `config` format field the
// composed attribute words read through, one file per named event, and the
// `cpumask` file that marks a device scoped.
auto writeDevice(
    const std::filesystem::path& root,
    std::string_view name,
    std::uint32_t type,
    const std::vector<std::pair<std::string_view, std::uint64_t>>& events,
    const bool scoped) -> void
{
  const auto dir = root / std::string(name);
  std::filesystem::create_directories(dir / "format");
  std::filesystem::create_directories(dir / "events");
  std::ofstream(dir / "type") << type;
  std::ofstream(dir / "format" / "config") << "config:0-47";
  for (const auto& [event, word] : events) {
    std::ofstream(dir / "events" / std::string(event))
        << "config=0x" + hexOf(word);
  }
  if (scoped) {
    std::ofstream(dir / "cpumask") << "0";
  }
}

auto writeDevices() -> std::filesystem::path
{
  const auto root = std::filesystem::temp_directory_path()
      / ("raw-events-devices-" + std::to_string(::getpid()));
  std::filesystem::remove_all(root);
  writeDevice(root,
              kDevice,
              kDeviceType,
              {{kFirstEvent, kFirstWord}, {kSecondEvent, kSecondWord}},
              false);
  writeDevice(
      root, kScopedDevice, kScopedType, {{kScopedEvent, kScopedWord}}, true);
  return root;
}

auto writeParanoid() -> std::filesystem::path
{
  const auto path = std::filesystem::temp_directory_path()
      / ("raw-events-paranoid-" + std::to_string(::getpid()));
  std::ofstream(path) << "0\n";
  return path;
}

// The refusal the scripted kernel answers for one config word, which is the
// scenario's whole answer for that leaf.
auto tuneScript(const std::string& mode) -> void
{
  if (mode == "einval" || mode == "mix") {
    gRefuseWord = kRawWord;
    gRefuseErrno = EINVAL;
  } else if (mode == "eacces" || mode == "scope") {
    gRefuseWord = kRawWord;
    gRefuseErrno = EACCES;
  }
}

// The clock provider stands first, the way the entry point stands it, so a run
// behind a scripted device that publishes no time leaf still reaches a time
// source. The scripted PMU provider then walks the scripted tree.
auto registerProviders() -> void
{
  const auto clock = sg::counters::System::local().registerProvider(
      std::make_unique<sg::counters::ClockProvider>());
  if (!clock.has_value()) {
    fail("the clock provider registers behind the script");
  }
  const auto registered = sg::counters::System::local().registerProvider(
      std::make_unique<sg::counters::PmuProvider>());
  if (!registered.has_value()) {
    fail("the scripted PMU provider registers");
  }
}

// The figure the scripted core device published, on the one line the parent
// reads ahead of the harness's own output.
auto reportFigure() -> void
{
  const auto object = sg::counters::System::local().object(kDevice);
  if (!object.has_value()) {
    fail("the scripted core device resolves");
  }
  const auto figure = object->programmableCounters();
  std::printf("figure device=%s limit=%s\n",
              std::string(kDevice).c_str(),
              figure.has_value() ? std::to_string(*figure).c_str() : "absent");
  std::fflush(stdout);
}

// The seat of the standing counter-limit disclosure. A finder needs the figure
// the host published beside it, so the seat stands only where a figure stands,
// the way the scripted mode of `harness_limit_disclosure_test` seats it: the
// line then answers on a host whose registers answer nothing as it does here.
auto seatProbeFinder() -> void
{
  const auto object = sg::counters::System::local().object(kDevice);
  if (!object.has_value()) {
    fail("the scripted core device resolves for the seat");
  }
  if (!object->programmableCounters().has_value()) {
    return;
  }
  sg::counters::detail::recordLimitSource(kDevice,
                                          sg::counters::LimitSource::PROBE);
}

// The opens this process answered, one line per device type and config word
// behind a total line. These are the opens a leaf did not pay for, which the
// harness's own text has no way to show.
auto reportOpens() -> void
{
  std::vector<OpenedPair> pairs;
  std::vector<std::size_t> counts;
  for (const auto& record : gOpens) {
    bool counted = false;
    for (std::size_t index = 0; index < pairs.size(); ++index) {
      if (pairs[index].type == record.type
          && pairs[index].config == record.config)
      {
        ++counts[index];
        counted = true;
        break;
      }
    }
    if (!counted) {
      pairs.push_back(record);
      counts.push_back(1);
    }
  }
  for (std::size_t index = 0; index < pairs.size(); ++index) {
    std::printf("opens type=%s word=%s count=%zu\n",
                hexOf(pairs[index].type).c_str(),
                hexOf(pairs[index].config).c_str(),
                counts[index]);
  }
  std::printf("opens total=%zu\n", gOpens.size());
  std::fflush(stdout);
}

auto bmRaw(sg::State& state) -> void
{
  ++gBenchmarkRuns;
  for (auto _ : state) {
  }
}

// The expanded instance of one name, null where the expansion left no such
// instance. The library route drives one instance of its own.
auto expandedInstance(const std::string& name) -> sg::Instance*
{
  for (auto& instance : sg::detail::instances()) {
    if (instance.name == name) {
      return &instance;
    }
  }
  return nullptr;
}

// The availability token the row of a library run carries under one address
// label, on the one line the parent reads: the runner keeps the column of a
// refused address and states the kind of its refusal (FR-023, FR-033).
auto reportColumn(const sg::BenchmarkResult& result,
                  const std::string_view address) -> void
{
  const auto label = std::find(result.metricLabels.begin(),
                               result.metricLabels.end(),
                               std::string(address));
  const auto position = label == result.metricLabels.end()
      ? result.metricLabels.size()
      : static_cast<std::size_t>(label - result.metricLabels.begin());
  const bool carries = label != result.metricLabels.end()
      && !result.rows.empty() && position < result.rows.front().metrics.size();
  const char* token = "";
  if (carries) {
    token = sg::detail::availabilityName(
        result.rows.front().metrics[position].availability);
  }
  std::printf("column address=%s availability=%s\n",
              std::string(address).c_str(),
              token);
  std::fflush(stdout);
}

// The library route of FR-023 and FR-033: the caller hands the runner an
// address list of its own, so the identity checkpoint of the entry point never
// runs and the runner's own absent column stands in its place. The runner's
// refusal line rides on this child's stderr, which the parent's capture merges
// with the lines printed here.
auto runLibraryChild() -> int
{
  sg::detail::expandRegistry();
  sg::detail::RunOptions options;
  options.fixedIterations = 1;
  options.counterAddresses = {std::string(kMachineRaw)};
  sg::detail::Runner runner(options);
  sg::Instance* const instance = expandedInstance("bmRaw");
  if (instance == nullptr) {
    fail("the expansion produced the registered instance");
  }
  const sg::BenchmarkResult result = runner.run(*instance);
  reportColumn(result, kMachineRaw);
  sg::detail::printResult(result);
  std::fflush(stdout);
  std::fprintf(stderr,
               "%s\n",
               gBenchmarkRuns > 0 ? "BENCHMARK_RAN" : "BENCHMARK_NO_RUN");
  reportOpens();
  return result.rows.size() == 1 ? 0 : 1;
}

// One child run, scripted from the first call to the last line it printed. The
// mode stands in place of the child's own first argument, the way the
// disclosure and gap tests hand a mode to a re-exec.
auto runChild(int argc, char** argv, const std::string& mode) -> int
{
  tuneScript(mode);
  static auto calls = scriptedCalls();
  gDevicesRoot = writeDevices();
  gParanoidPath = writeParanoid();
  installKernelCalls(&calls);
  registerProviders();
  reportFigure();
  if (mode == "limitCatalog" || mode == "limitRaw") {
    seatProbeFinder();
  }
  auto handle = sg::registerBenchmark(&bmRaw, "bmRaw");
  handle.iterations(1);
  if (mode == "libraryAbsent") {
    return runLibraryChild();
  }
  std::vector<char*> arguments;
  arguments.push_back(argv[0]);
  for (int index = 2; index < argc; ++index) {
    arguments.push_back(argv[index]);
  }
  const int status =
      sg::speedgunMain(static_cast<int>(arguments.size()), arguments.data());
  std::fflush(stdout);
  std::fprintf(stderr,
               "%s\n",
               gBenchmarkRuns > 0 ? "BENCHMARK_RAN" : "BENCHMARK_NO_RUN");
  reportOpens();
  return status;
}

auto readText(const std::string& path) -> std::string
{
  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// The lines of one text that open with the needle.
auto linesWith(const std::string& text,
               const std::string& needle) -> std::vector<std::string>
{
  std::vector<std::string> found;
  std::istringstream stream(text);
  std::string line;
  while (std::getline(stream, line)) {
    if (line.rfind(needle, 0) == 0) {
      found.push_back(line);
    }
  }
  return found;
}

// The first line of one text that opens with the needle, empty where none does.
auto lineWith(const std::string& text, const std::string& needle) -> std::string
{
  const auto lines = linesWith(text, needle);
  return lines.empty() ? std::string {} : lines.front();
}

auto occurrences(const std::string& text,
                 const std::string& needle) -> std::size_t
{
  std::size_t count = 0;
  for (auto at = text.find(needle); at != std::string::npos;
       at = text.find(needle, at + needle.size()))
  {
    ++count;
  }
  return count;
}

// The arguments of one scripted run: the filter, one iteration count, the
// addresses the scenario names, and the carriers the scenario asks for.
auto runArguments(const std::vector<std::string>& addresses,
                  const std::string& json = {},
                  const std::vector<std::string>& extra = {})
    -> std::vector<std::string>
{
  std::vector<std::string> arguments {"--filter", "^bmRaw$", "--iterations=1"};
  for (const auto& address : addresses) {
    arguments.push_back("--counter=" + address);
  }
  for (const auto& option : extra) {
    arguments.push_back(option);
  }
  if (!json.empty()) {
    arguments.push_back("--json=" + json);
  }
  return arguments;
}

// Runs this binary in one mode and reports the merged output. The expected
// status is part of what each scenario weighs, so a child that answers another
// status ends the run with its own text in front of the claim.
auto childOutput(const std::string& self,
                 const std::string& mode,
                 const std::vector<std::string>& arguments,
                 const int expected,
                 const std::string& environment = {}) -> std::string
{
  std::string command = environment + "\"" + self + "\" " + mode;
  for (const auto& argument : arguments) {
    command += " \"" + argument + "\"";
  }
  FILE* const pipe = popen((command + " 2>&1").c_str(), "r");
  if (pipe == nullptr) {
    fail("the scripted child starts");
  }
  std::string output;
  char buffer[256];
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    output += buffer;
  }
  const int status = pclose(pipe);
  if (!WIFEXITED(status) || WEXITSTATUS(status) != expected) {
    const std::string answer = WIFEXITED(status)
        ? std::to_string(WEXITSTATUS(status))
        : std::string("no exit status");
    recordText("the '" + mode + "' child answers the status the scenario "
                   "expects",
               answer,
               std::to_string(expected));
    std::fprintf(stderr, "%s\n", output.c_str());
    std::fflush(stderr);
  }
  return output;
}

// The figure the child read from its own scripted device: the figure word of
// the line it printed ahead of the harness text.
auto figureOf(const std::string& output) -> std::string
{
  const std::string needle =
      "figure device=" + std::string(kDevice) + " limit=";
  const auto line = lineWith(output, needle);
  return line.rfind(needle, 0) == 0 ? line.substr(needle.size())
                                    : std::string {};
}

// The availability word the child read for the column of one address: the word
// of the line it printed for that column ahead of the harness text, empty where
// it printed none.
auto columnAvailabilityOf(const std::string& output,
                          const std::string_view address) -> std::string
{
  const std::string needle =
      "column address=" + std::string(address) + " availability=";
  const auto line = lineWith(output, needle);
  return line.rfind(needle, 0) == 0 ? line.substr(needle.size())
                                    : std::string {};
}

// The refusal line of a run that answers one line for one address.
auto refusalOf(const std::string& output) -> std::string
{
  const auto lines = linesWith(output, "--counter: ");
  check(lines.size() == 1, "one refused address answers one line");
  return lines.empty() ? std::string {} : lines.front();
}

struct OpenLine
{
  std::uint64_t type = 0;
  std::uint64_t word = 0;
  std::size_t count = 0;
};

// The value one token of the child's line carries after its label, and an
// empty text where the token spells no such label.
auto valueAfter(const std::string& token,
                const std::string_view label) -> std::string
{
  if (token.rfind(label, 0) != 0) {
    return {};
  }
  return token.substr(label.size());
}

// The open record the child printed for its own run.
auto openLinesOf(const std::string& output) -> std::vector<OpenLine>
{
  std::vector<OpenLine> lines;
  for (const auto& text : linesWith(output, "opens type=")) {
    std::istringstream stream {text};
    std::string tag;
    std::string typeToken;
    std::string wordToken;
    std::string countToken;
    stream >> tag >> typeToken >> wordToken >> countToken;
    const auto type = valueAfter(typeToken, "type=");
    const auto word = valueAfter(wordToken, "word=");
    const auto total = valueAfter(countToken, "count=");
    if (type.empty() || word.empty() || total.empty()) {
      continue;
    }
    lines.push_back(OpenLine {.type = std::strtoull(type.c_str(), nullptr, 16),
                              .word = std::strtoull(word.c_str(), nullptr, 16),
                              .count = static_cast<std::size_t>(
                                  std::strtoull(total.c_str(), nullptr, 10))});
  }
  return lines;
}

// How many scripted opens carried one config word.
auto opensFor(const std::vector<OpenLine>& lines,
              const std::uint64_t word) -> std::size_t
{
  std::size_t total = 0;
  for (const auto& line : lines) {
    if (line.word == word) {
      total += line.count;
    }
  }
  return total;
}

// How many scripted opens carried a config word at or above a bound.
auto opensAbove(const std::vector<OpenLine>& lines,
                const std::uint64_t bound) -> std::size_t
{
  std::size_t total = 0;
  for (const auto& line : lines) {
    if (line.word >= bound) {
      total += line.count;
    }
  }
  return total;
}

// True where every scripted open of one config word carried it under one device
// type: the address reached the device it names and no other.
auto wordReachedOnlyType(const std::vector<OpenLine>& lines,
                         const std::uint64_t word,
                         const std::uint64_t type) -> bool
{
  for (const auto& line : lines) {
    if (line.word == word && line.type != type) {
      return false;
    }
  }
  return true;
}

// The figure one field of a printed row carries after its label. The value
// stands right-aligned in its field, so the token starts after the padding.
auto fieldOf(const std::string& text, const std::string& field) -> std::string
{
  const auto at = text.find(field);
  if (at == std::string::npos) {
    return {};
  }
  const auto start = text.find_first_not_of(' ', at + field.size());
  if (start == std::string::npos) {
    return {};
  }
  const auto end = text.find(' ', start);
  return end == std::string::npos ? text.substr(start)
                                  : text.substr(start, end - start);
}

// The measured row of a scripted run: the line naming the benchmark that
// carries the iterations field. An aggregate row and the statistic lines under
// it name the benchmark too, so the field tells the row from those.
auto rowLineOf(const std::string& output) -> std::string
{
  for (const auto& line : linesWith(output, "bmRaw")) {
    if (line.contains("iterations=")) {
      return line;
    }
  }
  return {};
}

// The repository file the walk finds: the directory the test runs in and the
// directory of this file's source, each climbed eight levels, the shape
// `writerSchemaVersion` of `test/source/harness_graph_report_test.cpp` runs.
auto treeFile(const std::string& relative) -> std::string
{
  const std::vector<std::filesystem::path> starts {
      std::filesystem::current_path(), std::filesystem::path {__FILE__}};
  for (auto start : starts) {
    for (int hop = 0; hop <= 8 && !start.empty(); ++hop) {
      const auto candidate = start / relative;
      std::error_code code;
      if (std::filesystem::exists(candidate, code)) {
        return candidate.string();
      }
      start = start.parent_path();
    }
  }
  return {};
}

// The schema minor the writer stamps, read from the tree so the check names the
// value the tree carries (FR-006, SC-003).
auto writerSchemaVersion() -> std::string
{
  const auto path = treeFile("source/harness/json_report.cpp");
  check(!path.empty(), "the writer's source reads from the tree");
  const std::string key = "kSchemaVersion = \"";
  const auto text = readText(path);
  const auto at = text.find(key);
  if (at == std::string::npos) {
    return {};
  }
  const auto start = at + key.size();
  const auto end = text.find('"', start);
  return end == std::string::npos ? std::string {}
                                  : text.substr(start, end - start);
}

// The standing console column of one address: the label the row prints and the
// figure behind it.
auto columnField(const std::string_view address) -> std::string
{
  return std::string(address) + "=";
}

auto unknownDeviceRawLine(const std::string& self) -> void
{
  const auto run =
      childOutput(self, "unknown", runArguments({std::string(kUnknownRaw)}), 2);
  checkText(refusalOf(run),
            "--counter: " + std::string(kUnknownRaw)
                + ": no object at path 'nosuch'",
            "a device the scripted root does not publish answers the standing "
            "no-object line (FR-002, US2-1)");
  check(opensFor(openLinesOf(run), kRawWord) == 0,
        "the absent device answers before any probe opens the word (FR-002)");
  check(run.find("BENCHMARK_NO_RUN") != std::string::npos
            && run.find("BENCHMARK_RAN") == std::string::npos,
        "the run of that address runs no benchmark (FR-002)");
}

auto wideLeafWidthLine(const std::string& self) -> void
{
  const auto run =
      childOutput(self, "wide", runArguments({std::string(kWideAddress)}), 2);
  checkText(refusalOf(run),
            "--counter: " + std::string(kWideAddress)
                + ": raw event code exceeds 16 hex digits",
            "a seventeen-digit leaf answers the width line in the origin, "
            "address, message shape (FR-005, US2-2, SC-002)");
  check(occurrences(run, "no such leaf") == 0,
        "the too-wide leaf never reaches the catalog-name route (FR-005)");
  const auto opens = openLinesOf(run);
  check(opensFor(opens, kTruncatedWord) == 0,
        "the too-wide text opens nothing cut to sixteen digits (FR-005)");
  check(opensAbove(opens, kWideBound) == 0,
        "no scripted open carries the too-wide leaf's own word (FR-005)");
  check(run.find("BENCHMARK_NO_RUN") != std::string::npos,
        "the width line ends the run before any measurement window (FR-005)");
}

auto wideLeafBeatsAbsentDevice(const std::string& self) -> void
{
  const auto run = childOutput(
      self, "wideAbsent", runArguments({std::string(kWideUnknown)}), 2);
  checkText(refusalOf(run),
            "--counter: " + std::string(kWideUnknown)
                + ": raw event code exceeds 16 hex digits",
            "the width line answers ahead of the device read (FR-005, FR-011)");
  check(occurrences(run, "no object at path") == 0,
        "the width gate stands ahead of the device lookup (FR-005)");
}

auto refusedProbeListedAsLines(const std::string& self) -> void
{
  const auto encoding =
      childOutput(self, "einval", runArguments({std::string(kRawAddress)}), 2);
  checkText(refusalOf(encoding),
            "--counter: " + std::string(kRawAddress)
                + ": listed as not-encodable (FR-023)",
            "a scripted encoding errno settles the encoding state and the run "
            "names it (FR-004, US2-3)");
  const auto permission =
      childOutput(self, "eacces", runArguments({std::string(kRawAddress)}), 2);
  checkText(refusalOf(permission),
            "--counter: " + std::string(kRawAddress)
                + ": listed as permission-blocked (FR-023)",
            "a scripted permission errno settles the permission state and the "
            "run names it (FR-004, US2-3)");
}

auto scopeRefusedRawLine(const std::string& self) -> void
{
  const auto run =
      childOutput(self, "scope", runArguments({std::string(kScopedRaw)}), 2);
  checkText(refusalOf(run),
            "--counter: " + std::string(kScopedRaw)
                + ": listed as scope-refused (FR-023)",
            "the scripted cpumask device relabels a permission errno to its "
            "scope's own state at the standing status (FR-011 edge)");
  check(run.find("BENCHMARK_NO_RUN") != std::string::npos,
        "the scope's own refusal runs no benchmark (FR-011 edge)");
}

auto refusedRawEndsRunBeforeMeasurement(const std::string& self) -> void
{
  const auto run = childOutput(self,
                               "mix",
                               runArguments({std::string(kFirstAddress),
                                             std::string(kRawAddress),
                                             std::string(kSecondAddress)}),
                               2);
  checkText(refusalOf(run),
            "--counter: " + std::string(kRawAddress)
                + ": listed as not-encodable (FR-023)",
            "one refused raw address among good ones names itself (edge)");
  check(rowLineOf(run).empty(),
        "the refused address ends the run before a row prints (edge)");
  check(run.find("BENCHMARK_RAN") == std::string::npos,
        "no benchmark ran beside the refused address (edge)");
}

auto duplicateRawAddressTwoColumns(const std::string& self) -> void
{
  const auto single = childOutput(
      self, "dupSingle", runArguments({std::string(kRawAddress)}), 0);
  const std::string document = "harness_raw_duplicate.json";
  std::remove(document.c_str());
  const auto twice = childOutput(
      self,
      "dupTwice",
      runArguments({std::string(kRawAddress), std::string(kRawAddress)},
                   document),
      0);
  check(occurrences(rowLineOf(twice), columnField(kRawAddress)) == 2,
        "the twice-written address prints two labels over its row (FR-008, "
        "US1-3)");
  const auto loaded = sg::loadReport(document);
  check(loaded.has_value(), "the duplicated run wrote a document (FR-008)");
  if (loaded.has_value() && !loaded->results.empty()) {
    const auto& labels = loaded->results.front().metricLabels;
    check(labels.size() == 2 && labels[0] == labels[1]
              && labels[0] == std::string(kRawAddress),
          "the document names the address in both labels (FR-008)");
  }
  const auto singleOpens = openLinesOf(single);
  const auto twiceOpens = openLinesOf(twice);
  check(opensFor(twiceOpens, kRawWord) == opensFor(singleOpens, kRawWord)
            && opensFor(twiceOpens, kRawWord) > 0,
        "both labels stand on one scripted open set for the word (FR-008)");
  std::remove(document.c_str());
}

auto rawColumnDisclosesEverywhere(const std::string& self) -> void
{
  const std::string optionFile = "harness_raw_options.txt";
  const std::string document = "harness_raw_disclose.json";
  const std::string envDocument = "harness_raw_env.json";
  std::ofstream(optionFile) << "counter=" << std::string(kSmallRaw) << "\n";
  std::remove(document.c_str());
  std::remove(envDocument.c_str());
  const auto fileRun = childOutput(
      self, "file", runArguments({}, document, {"--options=" + optionFile}), 0);
  const auto envRun =
      childOutput(self,
                  "env",
                  runArguments({}, envDocument),
                  0,
                  "SPEEDGUN_COUNTER=" + std::string(kSmallRaw) + " ");
  const std::string lead = "options from a file or the environment: --counter="
      + std::string(kSmallRaw);
  const auto fromFile = linesWith(fileRun, "options from a file");
  check(fromFile.size() == 1 && fromFile.front() == lead,
        "the option line of the run whose option came from a file carries the "
        "raw address (FR-006, SC-003)");
  const auto fromEnvironment = linesWith(envRun, "options from a file");
  check(fromEnvironment.size() == 1 && fromEnvironment.front() == lead,
        "the option line of the run whose option came from the environment "
        "carries the raw address (FR-006, SC-003)");
  check(rowLineOf(fileRun).contains(columnField(kSmallRaw)),
        "the console label carries the raw address (FR-006, US3)");
  const auto text = readText(document);
  check(!text.empty(), "the file run wrote a document (FR-006)");
  const auto loaded = sg::loadReport(document);
  check(loaded.has_value(), "the written document reads back (FR-006)");
  std::size_t rows = 0;
  if (loaded.has_value() && !loaded->results.empty()) {
    const auto& result = loaded->results.front();
    const auto label = std::find(result.metricLabels.begin(),
                                 result.metricLabels.end(),
                                 std::string(kSmallRaw));
    check(label != result.metricLabels.end(),
          "the document's metric labels name the raw address (FR-006, SC-003)");
    if (label != result.metricLabels.end()) {
      const auto index =
          static_cast<std::size_t>(label - result.metricLabels.begin());
      for (const auto& one : loaded->results) {
        for (const auto& row : one.rows) {
          ++rows;
          check(index < row.metrics.size(),
                "each row carries a value under the raw address (FR-006)");
        }
      }
      const std::string labelKey =
          "\"label\":\"" + std::string(kSmallRaw) + "\"";
      check(occurrences(text, labelKey) == rows,
            "each row's metric entry labels the raw address (FR-006, SC-003)");
    }
  }
  const auto schema = treeFile("docs/schema/report-v1.json");
  check(!schema.empty(), "the schema file reads from the tree (FR-006)");
  const auto clean = sg::detail::checkReport(schema, text);
  if (!clean.has_value()) {
    std::fprintf(stderr, "  %s\n", clean.error().c_str());
    std::fflush(stderr);
  }
  check(clean.has_value(),
        "the written document passes the standing schema check (FR-006, "
        "SC-003)");
  if (loaded.has_value()) {
    checkText(loaded->schemaVersion,
              writerSchemaVersion(),
              "the document stamps the schema minor the tree carries (FR-006)");
  }
  std::remove(document.c_str());
  std::remove(envDocument.c_str());
  std::remove(optionFile.c_str());
}

auto limitLineCountsRawColumn(const std::string& self) -> void
{
  const std::string catalogPath = "harness_raw_limit_catalog.json";
  const std::string rawPath = "harness_raw_limit_raw.json";
  std::remove(catalogPath.c_str());
  std::remove(rawPath.c_str());
  const auto catalog =
      childOutput(self,
                  "limitCatalog",
                  runArguments({std::string(kFirstAddress)}, catalogPath),
                  0);
  const auto raw = childOutput(
      self,
      "limitRaw",
      runArguments({std::string(kFirstAddress), std::string(kRawAddress)},
                   rawPath),
      0);
  const auto figure = figureOf(catalog);
  checkText(figureOf(raw),
            figure,
            "the scripted device publishes one figure to both runs (FR-009)");
  const auto catalogLines = linesWith(catalog, "counter limit:");
  const auto rawLines = linesWith(raw, "counter limit:");
  if (figure != "absent") {
    const std::string expected = "counter limit: device '"
        + std::string(kDevice) + "' count " + figure + " source probe";
    check(catalogLines.size() == 1 && rawLines.size() == 1,
          "the seated finder prints the standing line once per run (FR-009)");
    if (!catalogLines.empty() && !rawLines.empty()) {
      checkText(rawLines.front(),
                expected,
                "the run carrying the raw column answers the standing "
                "counter-limit line unchanged (FR-009)");
      checkText(
          catalogLines.front(),
          expected,
          "the run of the catalog column answers the same bytes (FR-009)");
    }
  } else {
    check(catalogLines.empty() && rawLines.empty(),
          "a device that published no figure prints no limit line (FR-009)");
  }
  const auto catalogRecord = sg::detail::loadCounterLimits(catalogPath);
  const auto rawRecord = sg::detail::loadCounterLimits(rawPath);
  const bool seated = catalogRecord.has_value() && rawRecord.has_value();
  check(seated, "both runs write the run-level limit record (FR-009)");
  if (seated) {
    check(rawRecord->size() == catalogRecord->size() && !rawRecord->empty(),
          "one scripted device stands in each record (FR-009)");
    if (!catalogRecord->empty() && !rawRecord->empty()) {
      checkText(rawRecord->front().device,
                catalogRecord->front().device,
                "the raw run's record names the scripted device (FR-009)");
      check(rawRecord->front().limit == catalogRecord->front().limit
                && rawRecord->front().source == catalogRecord->front().source,
            "the seat the raw column joined kept its figure and its finder "
            "(FR-009)");
    }
  }
  std::remove(catalogPath.c_str());
  std::remove(rawPath.c_str());
}

auto rawRunConsoleAndJson(const std::string& self) -> void
{
  const std::string document = "harness_raw_run.json";
  std::remove(document.c_str());
  const auto run = childOutput(
      self, "plain", runArguments({std::string(kSmallRaw)}, document), 0);
  check(run.find("BENCHMARK_RAN") != std::string::npos,
        "the scripted run reached its benchmark (US1)");
  const auto row = rowLineOf(run);
  const auto field = columnField(kSmallRaw);
  check(row.contains(field), "the console prints the raw column (US1)");
  const auto loaded = sg::loadReport(document);
  check(loaded.has_value(), "the scripted run wrote its document (US1)");
  if (loaded.has_value() && !loaded->results.empty()) {
    const auto& result = loaded->results.front();
    check(!result.rows.empty(), "the scripted run wrote a row (US1)");
    const auto label = std::find(result.metricLabels.begin(),
                                 result.metricLabels.end(),
                                 std::string(kSmallRaw));
    check(label != result.metricLabels.end(),
          "the document names the raw column (US1, FR-006)");
    // The one index the two figures meet under, read only where the row
    // carries a metric position at it.
    const auto positions = result.rows.empty()
        ? std::size_t {0}
        : result.rows.front().metrics.size();
    const auto index = label == result.metricLabels.end()
        ? positions
        : static_cast<std::size_t>(label - result.metricLabels.begin());
    const bool stands = index < positions;
    check(stands, "the row carries a value under that label (US1)");
    if (stands) {
      const double printed = std::strtod(fieldOf(row, field).c_str(), nullptr);
      const double written = result.rows.front().metrics[index].value;
      check(printed > 0.0 && written > 0.0,
            "the scripted column measured a positive figure (US1)");
      check(std::abs(printed - written) < 5e-3,
            "the console figure and the document figure stand as one pair "
            "(US1)");
    }
  }
  const auto opens = openLinesOf(run);
  check(opensFor(opens, kSmallRawWord) > 0,
        "the scripted kernel opened the raw word (US1, FR-002)");
  check(wordReachedOnlyType(
            opens, kSmallRawWord, static_cast<std::uint64_t>(kDeviceType)),
        "the raw word reached the scripted device's own type number (FR-002)");
  std::remove(document.c_str());
}

auto absentPmuPathRawRefusal(const std::string& self) -> void
{
  const auto run = childOutput(
      self, "machineRaw", runArguments({std::string(kMachineRaw)}), 2);
  checkText(refusalOf(run),
            "--counter: " + std::string(kMachineRaw)
                + ": no object at path 'machine'",
            "a raw address at a path the scripted PMU provider holds no device "
            "at answers the standing no-object line (FR-002, R-06)");
  check(opensFor(openLinesOf(run), kRawWord) == 0,
        "the refusal at a path holding no PMU device opens nothing (FR-002, "
        "R-06)");
  check(run.find("BENCHMARK_NO_RUN") != std::string::npos
            && run.find("BENCHMARK_RAN") == std::string::npos,
        "the run of that address runs no benchmark (FR-002)");
}

auto libraryRouteKeepsAbsentColumn(const std::string& self) -> void
{
  const auto run = childOutput(self, "libraryAbsent", {}, 0);
  checkText(lineWith(run, std::string(kMachineRaw) + ": "),
            std::string(kMachineRaw) + ": no object at path 'machine'",
            "the runner's own raw arm names the address and the counters "
            "wording of a path holding no PMU device (FR-002, FR-023, R-06)");
  checkText(columnAvailabilityOf(run, kMachineRaw),
            "absent",
            "the library route keeps the column of that address and states its "
            "refusal kind (FR-023, FR-033)");
  const auto row = rowLineOf(run);
  check(!row.empty(),
        "the library route measures a row beside the refused address (FR-023, "
        "FR-033)");
  check(row.contains(columnField(kMachineRaw)),
        "the refused raw address keeps its column in the measured row "
        "(FR-023, FR-033)");
  check(row.contains("refusal=absent"),
        "the row prints the standing refusal token for that column (FR-023)");
  check(run.find("BENCHMARK_RAN") != std::string::npos,
        "the library route reaches the benchmark instead of ending the run "
        "(FR-033)");
  check(opensFor(openLinesOf(run), kRawWord) == 0,
        "the runner's refusal opens nothing (FR-002, R-06)");
}

// One named scenario of the harness scenario table and the function that
// weighs it.
struct Case
{
  const char* name;
  void (*body)(const std::string& self);
};

// The twelve named scenarios: the ten the table of the plan names, then the
// two that drive the arms the coverage trace leaves dark.
auto allCases() -> const std::vector<Case>&
{
  static const std::vector<Case> kCases {
      Case {.name = "unknownDeviceRawLine", .body = unknownDeviceRawLine},
      Case {.name = "wideLeafWidthLine", .body = wideLeafWidthLine},
      Case {.name = "wideLeafBeatsAbsentDevice",
            .body = wideLeafBeatsAbsentDevice},
      Case {.name = "refusedProbeListedAsLines",
            .body = refusedProbeListedAsLines},
      Case {.name = "scopeRefusedRawLine", .body = scopeRefusedRawLine},
      Case {.name = "refusedRawEndsRunBeforeMeasurement",
            .body = refusedRawEndsRunBeforeMeasurement},
      Case {.name = "duplicateRawAddressTwoColumns",
            .body = duplicateRawAddressTwoColumns},
      Case {.name = "rawColumnDisclosesEverywhere",
            .body = rawColumnDisclosesEverywhere},
      Case {.name = "limitLineCountsRawColumn",
            .body = limitLineCountsRawColumn},
      Case {.name = "rawRunConsoleAndJson", .body = rawRunConsoleAndJson},
      Case {.name = "absentPmuPathRawRefusal", .body = absentPmuPathRawRefusal},
      Case {.name = "libraryRouteKeepsAbsentColumn",
            .body = libraryRouteKeepsAbsentColumn},
  };
  return kCases;
}

// The modes that run a scripted child, one scenario run per mode.
auto isChildMode(const std::string& mode) -> bool
{
  static const std::array<std::string_view, 16> kModes {"unknown",
                                                        "wide",
                                                        "wideAbsent",
                                                        "einval",
                                                        "eacces",
                                                        "scope",
                                                        "mix",
                                                        "dupSingle",
                                                        "dupTwice",
                                                        "file",
                                                        "env",
                                                        "limitCatalog",
                                                        "limitRaw",
                                                        "plain",
                                                        "machineRaw",
                                                        "libraryAbsent"};
  return std::any_of(kModes.begin(),
                     kModes.end(),
                     [&](const std::string_view name) { return name == mode; });
}

}  // namespace

auto main(int argc, char** argv) -> int
{
  sg::test::flushCountersOnAbort();
  const std::string mode = argc > 1 ? argv[1] : "";
  if (isChildMode(mode)) {
    return runChild(argc, argv, mode);
  }
  const std::string self = argv[0];
  for (const auto& entry : allCases()) {
    gScenario = entry.name;
    std::printf("case %s\n", entry.name);
    std::fflush(stdout);
    entry.body(self);
  }
  if (!gFailures.empty()) {
    std::printf("harness_raw_events_test: %zu FAILED CHECKS\n",
                gFailures.size());
    return 1;
  }
  std::puts("harness_raw_events_test: ok");
  return 0;
}
