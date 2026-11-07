// ============================================================================
// The disclosure of where a published counter limit came from
// (specs/041-counter-probe T043 to T045; FR-040a, FR-050, FR-051, FR-052,
// D-3, D-3a; specs/041-counter-probe/contracts/disclosure.md).
//
// One run answers two questions about a limit, and this unit reads both
// answers from one run's own text and its own document: the console line a
// probed device earns (FR-050), and the run-level `counterLimits` record every
// needed device earns (FR-051). A run whose device came from the decode prints
// no line, and a run that needs no device writes no record.
//
// The run is scripted, so no case depends on what the host's kernel grants.
// `installKernelCalls` stands a scripted kernel under the real PMU provider,
// and the scripted devices root publishes one `cpu` device with two named
// events: the provider's own enumeration seeds the catalog the run resolves,
// and every open the run issues reaches the script (FR-073). Each scenario
// runs in a re-exec of this binary, because one process publishes one limit
// and one finder per device path, the way `registerPassFixture` states for
// the pass fixture.
//
// A scenario seats a finder beside the figure its own host published, and the
// seat stands only where a figure stands. Registration enumerates the provider
// it takes, so the fill has settled by the time it returns: the child reads
// the pair from `objectLimitSource(kDevice)` and from that same object's
// `programmableCounters()`, prints it on one machine-readable line ahead of
// the harness's own output, and seats on it. That accessor answers `NONE`
// exactly where `programmableCounters()` answers no value
// (include/speedgun-ng/counters_system.hpp:222-224), so a seat beside an
// absent figure is the one pair the contract forbids: it hands the run a
// finder with no figure to name, and the disclosure of FR-050 dereferences
// that absent figure. Where the host publishes no figure, as on a runner whose
// registers answer nothing and whose kernel refuses a group, the scenario
// seats nothing and its assertion is the pairing rule itself.
//
// Scenario map:
//   theChildReportsItsPair        the child's own line, ahead of every byte the
//                                 harness prints, names the finder and the
//                                 figure its host answered
//   probedRunDisclosesOnce        a probed pair prints the line once, names the
//                                 device, the figure and `probe`, and the run's
//                                 own row and header keep their bytes (FR-050)
//   scriptedRunDisclosesOnce      a scripted device publishes the figure and
//   the
//                                 run seats `probe`, so the same line stands on
//                                 a host whose registers answer nothing
//                                 (FR-050)
//   probedRunRecordsItsSource     the document names the device, its figure and
//                                 the seated finder, and the figure matches the
//                                 printed one (FR-051)
//   decodedRunPrintsNothingNew    a decoded pair prints no `counter limit:`
//                                 line and its record names `decode` (FR-050,
//                                 FR-051)
//   aDeviceWithNoFigureNamesNone  a device that published no figure seats no
//                                 finder: no line, a null limit and `none` in
//                                 the entry (FR-040a)
//   aRunNeedingNoDeviceWritesNone a run with no metric column writes no record
//                                 at all (FR-051)
// ============================================================================

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <sys/mman.h>
#include <unistd.h>

#include "../source/counters/detail/limit_source_record.hpp"
#include "../source/counters/linux_pmu/kernel_calls.hpp"
#include "../source/harness/detail/json_report.hpp"
#include "detail/gcov_flush.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_clock.hpp"
#include "speedgun-ng/counters_fake.hpp"
#include "speedgun-ng/counters_metrics.hpp"
#include "speedgun-ng/counters_pmu.hpp"
#include "speedgun-ng/counters_system.hpp"

namespace
{

using sg::counters::LimitSource;
using sg::counters::detail::installKernelCalls;
using sg::counters::detail::KernelCalls;
using sg::counters::detail::recordLimitSource;

[[noreturn]] auto fail(const char* const what) -> void
{
  std::fprintf(stderr, "HARNESS LIMIT DISCLOSURE TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool condition, const char* const what) -> void
{
  if (!condition) {
    fail(what);
  }
}

auto checkText(const std::string& answer,
               const std::string& expected,
               const char* const what) -> void
{
  if (answer != expected) {
    std::fprintf(stderr,
                 "HARNESS LIMIT DISCLOSURE TEST FAIL: %s\n  answer:   '%s'\n  "
                 "expected: '%s'\n",
                 what,
                 answer.c_str(),
                 expected.c_str());
    std::exit(1);
  }
}

// The one device every scripted run needs, and its two events. The name is a
// host device's own name on purpose: the collision makes the entry point's own
// registration of a second PMU provider refuse for the run, so no case reads a
// counter the script did not answer.
constexpr std::string_view kDevice {"cpu"};
constexpr std::string_view kFirstEvent {"disc_a"};
constexpr std::string_view kSecondEvent {"disc_b"};

// The limit the scripted provider publishes for that device, the figure a run
// names without one register of the host behind it.
constexpr std::size_t kScriptedLimit = 4;

// The scripted kernel's state: the count the search grants, the record of the
// opens, and the two fixture paths the seam answers.
std::filesystem::path gDevicesRoot;
std::filesystem::path gParanoidPath;
int gNextFd = 100;
int gGroupSize = 0;
std::size_t gMaxFit = 6;

auto scriptedOpen(void*, int, int cpu, int groupFd, unsigned long) -> long
{
  gGroupSize = groupFd < 0 ? 1 : gGroupSize + 1;
  const int fd = gNextFd++;
  static_cast<void>(cpu);
  return fd;
}

// The read of a group leader: the count, the enabled window and the running
// window. A group at or below the fitted size ran its whole window, which is
// the pair F-3 states and the pair a search reads (F-1, F-3).
auto scriptedRead(int, void* buffer, std::size_t count) -> long
{
  const std::uint64_t enabled = 4000;
  const std::uint64_t words[3] = {
      17,
      enabled,
      static_cast<std::size_t>(gGroupSize) <= gMaxFit ? enabled : 1000};
  auto* const posted = static_cast<std::uint64_t*>(buffer);
  for (std::size_t index = 0; index < 3; ++index) {
    posted[index] = words[index];
  }
  const auto bytes = 3U * sizeof(std::uint64_t);
  // Both sides are a byte count, and the cast names the one signed type the
  // seam's answer settles in.
  // NOLINTNEXTLINE(modernize-use-integer-sign-comparison)
  return static_cast<long>(bytes < count ? bytes : count);
}

auto scriptedClose(int) -> int
{
  return 0;
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
  // The words no vendored table row matches: the catalog holds the fixture
  // device's own events and nothing the host's vendor table would add.
  calls.cpuidWords = []() -> sg::counters::detail::CpuidWords
  {
    return {.leaf0Eax = 0x15U,
            .leaf0Ebx = 0x75536F4EU,  // "Nosu"
            .leaf0Edx = 0x6E655663U,  // "cVen"
            .leaf0Ecx = 0x58726F64U,  // "dorX"
            .leaf1Answered = true,
            .leaf1Eax = 0x30650U};
  };
  return calls;
}

// The scripted device tree: one `cpu` device, one `config` field, and the two
// events the scripted run sums.
auto writeDevices() -> std::filesystem::path
{
  const auto root = std::filesystem::temp_directory_path()
      / ("disclosure-devices-" + std::to_string(::getpid()));
  std::filesystem::remove_all(root);
  const auto device = root / std::string(kDevice);
  std::filesystem::create_directories(device / "format");
  std::filesystem::create_directories(device / "events");
  std::ofstream(device / "type") << "4";
  std::ofstream(device / "format" / "config") << "config:0-47";
  std::ofstream(device / "events" / std::string(kFirstEvent)) << "config=0x2d";
  std::ofstream(device / "events" / std::string(kSecondEvent)) << "config=0x31";
  return root;
}

auto writeParanoid() -> std::filesystem::path
{
  const auto path = std::filesystem::temp_directory_path()
      / ("disclosure-paranoid-" + std::to_string(::getpid()));
  std::ofstream(path) << "0\n";
  return path;
}

auto bmDisclosure(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmPlain(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

// The word a child's line spells where its device published no figure, which
// is the shape the record writes as `null`.
constexpr std::string_view kAbsent {"absent"};

// The pair one child read from its own host: the finder's word, and the figure
// as the number the device published or as `absent`. The child reports the
// pair ahead of the seat it is about to apply, so the pair the parent weighs
// is the pair the host answered.
struct Earned
{
  std::string finder;
  std::string figure;
};

// The finder word the `counterLimits` record carries for one source, read from
// the table its writer and its reader share, so this unit says the words the
// document says (FR-051).
auto wordOf(const LimitSource source) -> std::string_view
{
  return sg::detail::enumWord(sg::detail::kLimitSourceWords,
                              static_cast<std::size_t>(source));
}

// The figure as the child's line spells it.
auto figureWord(const std::optional<std::size_t>& figure) -> std::string
{
  return figure.has_value() ? std::to_string(*figure) : std::string(kAbsent);
}

// The finder one mode seats beside a published figure, and nothing for the
// mode that states a host answering on its own.
auto seatOf(const std::string& mode) -> std::optional<LimitSource>
{
  if (mode == "probed") {
    return LimitSource::PROBE;
  }
  if (mode == "scripted") {
    // The scripted device carries the same finder, seated beside the figure the
    // script published.
    return LimitSource::PROBE;
  }
  if (mode == "decoded") {
    return LimitSource::DECODE;
  }
  return std::nullopt;
}

// Stands the scripted kernel, seeds the catalog through the real provider, and
// seats the finder the scenario is about beside the figure the provider
// published. The provider registers first, so the entry point's own provider
// reaches a path already published and stands refused. The pair the host
// answered comes back before any seat moves, and a seat that has no figure to
// stand on stays unseated.
auto scriptRun(const std::optional<LimitSource> seat) -> Earned
{
  static auto calls = scriptedCalls();
  gDevicesRoot = writeDevices();
  gParanoidPath = writeParanoid();
  installKernelCalls(&calls);
  // The clock provider stands first, the way the entry point stands it
  // (source/harness/cli.cpp:679): the scripted PMU provider publishes no
  // `machine` leaves, so a run behind it alone ends with no time source.
  // The entry point's own registration is then a refused pass (R-03).
  const auto clock = sg::counters::System::local().registerProvider(
      std::make_unique<sg::counters::ClockProvider>());
  check(clock.has_value(), "the clock provider registers behind the script");
  auto provider = std::make_unique<sg::counters::PmuProvider>();
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(provider));
  check(registered.has_value(), "the scripted PMU provider registers");
  const auto object = sg::counters::System::local().object(kDevice);
  check(object.has_value(), "the scripted device resolves");
  if (!object.has_value()) {
    return {};
  }
  const Earned earned {
      .finder = std::string(wordOf(sg::counters::objectLimitSource(kDevice))),
      .figure = figureWord(object->programmableCounters())};
  if (seat.has_value() && earned.figure != kAbsent) {
    // A finder needs the figure the host published beside it: the accessor
    // answers `NONE` exactly where no figure stands, so a seat that answers a
    // finder beside an absent figure is the pair the contract forbids and the
    // pair the disclosure then dereferences
    // (include/speedgun-ng/counters_system.hpp:222-224).
    recordLimitSource(kDevice, *seat);
    check(sg::counters::objectLimitSource(kDevice) == *seat,
          "the reader answers the finder the scenario seated");
  }
  return earned;
}

// Prints the pair the child read from its host, on the one line the parent
// reads, ahead of every byte the harness prints. The flush keeps the line
// first however the child's standard output is buffered.
auto reportEarned(const Earned& earned) -> void
{
  std::printf("earned source=%s figure=%s\n",
              earned.finder.c_str(),
              earned.figure.c_str());
  std::fflush(stdout);
}

// The sum of the scripted column: the shape of a metric column over the two
// events the scripted device lists.
auto sumLeaves() -> sg::counters::Expression<sg::counters::Dim<0, 1>>
{
  const auto object = sg::counters::System::local().object(kDevice);
  check(object.has_value(), "the scripted device resolves");
  const auto first = object->counter<sg::counters::Dim<0, 1>>(kFirstEvent);
  const auto second = object->counter<sg::counters::Dim<0, 1>>(kSecondEvent);
  check(first.has_value() && second.has_value(),
        "both scripted events resolve");
  return sg::counters::Expression<sg::counters::Dim<0, 1>>(*first) + *second;
}

// The device the host stands outside: a scripted provider publishes the figure
// and the run seats the finder by hand, so the pair the disclosure reads stands
// on every lane, including one whose kernel answers no figure at all (FR-040a,
// FR-050). The object takes the host device name, so the entry point stands its
// own provider refused for the run.
auto registerScriptedDevice() -> bool
{
  auto provider = std::make_unique<sg::counters::FakeProvider>();
  auto& script = *provider;
  script.addObject(kDevice, "pmu", "the disclosure script");
  script.setProgrammableCounters(kDevice, kScriptedLimit);
  script.addCounter(kDevice, kFirstEvent, "none", "scripted leaf");
  script.addCounter(kDevice, kSecondEvent, "none", "scripted leaf");
  script.setPoints(kDevice, kFirstEvent, {0}, 1);
  script.setPoints(kDevice, kSecondEvent, {0}, 1);
  return sg::counters::System::local()
      .registerProvider(std::move(provider))
      .has_value();
}

// One child run, scripted from the first call to the last line it printed.
auto runChild(int argc, char** argv, const std::string& mode) -> int
{
  std::vector<char*> arguments;
  arguments.push_back(argv[0]);
  for (int index = 2; index < argc; ++index) {
    arguments.push_back(argv[index]);
  }
  if (mode == "plain") {
    auto handle = sg::registerBenchmark(&bmPlain, "bmPlain");
    handle.iterations(1);
  } else if (mode == "scripted") {
    // The clock provider stands first, the way the entry point stands it
    // (source/harness/cli.cpp:679), so the run reaches a time source behind a
    // scripted device that publishes none.
    const auto clock = sg::counters::System::local().registerProvider(
        std::make_unique<sg::counters::ClockProvider>());
    check(clock.has_value(), "the clock provider registers for the script");
    check(registerScriptedDevice(), "the scripted device registers");
    const auto object = sg::counters::System::local().object(kDevice);
    check(object.has_value(), "the scripted device resolves");
    if (!object.has_value()) {
      return 1;
    }
    reportEarned(Earned {
        .finder = std::string(wordOf(sg::counters::objectLimitSource(kDevice))),
        .figure = figureWord(object->programmableCounters())});
    recordLimitSource(kDevice, LimitSource::PROBE);
    auto handle = sg::registerBenchmark(&bmDisclosure, "bmDisclosure");
    handle.iterations(1);
    handle.addMetric(sumLeaves(), "pair");
  } else {
    // The child states the pair its host answered before the run prints a
    // byte of its own, so the pair the parent weighs stands in the same text.
    const auto earned = scriptRun(seatOf(mode));
    reportEarned(earned);
    auto handle = sg::registerBenchmark(&bmDisclosure, "bmDisclosure");
    handle.iterations(1);
    handle.addMetric(sumLeaves(), "pair");
  }
  return sg::speedgunMain(static_cast<int>(arguments.size()), arguments.data());
}

auto readText(const std::string& path) -> std::string
{
  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// Runs this binary in one mode through a shell and reports its merged output.
auto childOutput(const std::string& self,
                 const std::string& mode,
                 const std::vector<std::string>& arguments) -> std::string
{
  std::string command = "\"" + self + "\" " + mode;
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
  if (status != 0) {
    std::fprintf(stderr, "%s\n", output.c_str());
    fail("the scripted child exits 0");
  }
  return output;
}

// The lines of one text that start with the needle.
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

// The pair the child reported, read from the one line it prints ahead of the
// harness's own output. A line that spells no word the record carries is the
// child's own fault, and no rule below has a reading left to weigh.
auto earnedOf(const std::string& output) -> Earned
{
  const auto lines = linesWith(output, "earned ");
  check(lines.size() == 1, "the child reports the pair it read once");
  if (lines.size() != 1) {
    return {};
  }
  std::istringstream stream {lines.front()};
  std::string tag;
  std::string finderToken;
  std::string figureToken;
  stream >> tag >> finderToken >> figureToken;
  check(tag == "earned", "the child's line opens with its own word");
  const auto finder = valueAfter(finderToken, "source=");
  const auto figure = valueAfter(figureToken, "figure=");
  check(!finder.empty() && !figure.empty(),
        "the child's line names a source and a figure");
  check(finder == wordOf(LimitSource::NONE)
            || finder == wordOf(LimitSource::DECODE)
            || finder == wordOf(LimitSource::PROBE),
        "the child names a finder the record carries (FR-051)");
  return Earned {.finder = finder, .figure = figure};
}

// One run over the scripted device: the entry the run wrote and the console
// text the same run printed. The record is read first, so the figure the line
// is weighed against is the figure the document carries. The seat of `mode`
// stands where the host published a figure, and the host's own finder answers
// where that seat stands nowhere, so one pair rule settles every host the same
// way (FR-040a, FR-050, FR-051).
auto checkRun(const std::string& mode,
              const std::string& output,
              const std::string& path) -> void
{
  const auto earned = earnedOf(output);
  const auto entries = sg::detail::loadCounterLimits(path);
  check(entries.has_value(), "the written record reads back (FR-051)");
  if (!entries.has_value()) {
    return;
  }
  check(entries->size() == 1,
        "the record names each needed device once (FR-051)");
  if (entries->empty()) {
    return;
  }
  const auto& entry = entries->front();
  checkText(entry.device,
            "cpu",
            "the entry names the device the run needed (FR-051)");
  checkText(figureWord(entry.limit),
            earned.figure,
            "the record carries the figure the host published (FR-051)");
  const auto seat = seatOf(mode);
  const bool seated = seat.has_value() && earned.figure != kAbsent;
  const std::string expectedFinder(seated ? wordOf(*seat) : earned.finder);
  const std::string finderClaim =
      mode + " run records the finder its own pair answers with (FR-051)";
  checkText(
      std::string(wordOf(entry.source)), expectedFinder, finderClaim.c_str());
  check(entry.limit.has_value() == (entry.source != LimitSource::NONE),
        "a figure stands exactly where the entry names a finder (FR-040a)");
  if (!entry.limit.has_value()) {
    checkText(std::string(wordOf(entry.source)),
              "none",
              "an entry with no figure names no finder (FR-040a)");
  }
  const std::size_t expectedLines =
      expectedFinder == wordOf(LimitSource::PROBE) ? 1 : 0;
  const auto lines = linesWith(output, "counter limit:");
  const std::string lineClaim =
      mode + " run prints the disclosure line its pair earns (FR-050)";
  check(lines.size() == expectedLines, lineClaim.c_str());
  if (lines.size() == 1 && entry.limit.has_value()) {
    checkText(
        lines.front(),
        "counter limit: device 'cpu' count " + std::to_string(*entry.limit)
            + " source probe",
        "the line names the device, the figure and the finder " "(FR-050)");
  }
  check(occurrences(output, "counter limit:") == expectedLines,
        "no further line names a limit's source (FR-050)");
  check(!linesWith(output, "speedgun-ng ").empty(),
        "the run prints its own header line (FR-043)");
  check(!linesWith(output, "bmDisclosure").empty(),
        "the run prints its own row (FR-043)");
}

auto checkPlainRun(const std::string& output, const std::string& path) -> void
{
  check(output.find("counter limit:") == std::string::npos,
        "a run with no needed device prints no line (FR-050)");
  check(readText(path).find("\"counterLimits\"") == std::string::npos,
        "a run with no needed device writes no record key (FR-051)");
  const auto entries = sg::detail::loadCounterLimits(path);
  check(entries.has_value() && entries->empty(),
        "the reader answers no entry for a document that carries none " "(FR-052)");
}

auto scenariosAreChecked(const std::string& self) -> void
{
  // The four modes that need the scripted device: `probed` and `decoded` seat
  // the finder they are about where the host published a figure, `scripted`
  // seats `probe` beside the figure its own provider published, and `silent`
  // seats nothing and states the host whose registers answer nothing and whose
  // kernel refuses a group. Each mode runs the one scripted shape, so a host
  // that answers no figure leaves every mode but `scripted` to the pair rule in
  // `checkRun`.
  constexpr std::array<std::string_view, 4> kDeviceModes {
      "probed",
      "scripted",
      "decoded",
      "silent",
  };
  for (const auto mode : kDeviceModes) {
    const std::string name(mode);
    const std::string path = "harness_limit_" + name + ".json";
    std::remove(path.c_str());
    const auto output = childOutput(
        self,
        name,
        {"--filter", "^bmDisclosure$", "--iterations=1", "--json=" + path});
    checkRun(name, output, path);
    std::remove(path.c_str());
  }

  const std::string plainPath = "harness_limit_plain.json";
  std::remove(plainPath.c_str());
  const auto plain = childOutput(
      self,
      "plain",
      {"--filter", "^bmPlain$", "--iterations=1", "--json=" + plainPath});
  checkPlainRun(plain, plainPath);
  std::remove(plainPath.c_str());
}

}  // namespace

auto main(int argc, char** argv) -> int
{
  sg::test::flushCountersOnAbort();
  const std::string mode = argc > 1 ? argv[1] : "";
  if (mode == "probed" || mode == "scripted" || mode == "decoded"
      || mode == "silent" || mode == "plain")
  {
    return runChild(argc, argv, mode);
  }
  scenariosAreChecked(argv[0]);
  std::puts("harness_limit_disclosure_test: ok");
  return 0;
}
