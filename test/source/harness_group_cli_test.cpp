// ============================================================================
// TDD test for the `--group-layout` row of specs/039-context-groups (T010,
// T012; FR-011, FR-036, FR-037; specs/028 FR-001, FR-004, FR-008, FR-012,
// FR-015, FR-017, FR-018, FR-026, FR-035, FR-037, FR-039, FR-040, D-04,
// D-05).
//
// The row is one name, one internal value and one path. The scan recognizes
// the word, the settle arm carries the path through the resolved option set,
// the help answer prints the line the row builds, and the row carries no
// environment source, the shape `--compare-rules` keeps (FR-011). The value
// 299 stands in the free block above the noise tracer's 297, and the block
// comment of `detail/options.hpp` names the split that leaves it there.
//
// The six row cases open no file. The scan, the settle arm and the gather keep
// their rule of reading no file of their own (specs/028 D-05), so each of them
// hands the row a path that stands uncreated and asserts what the row answers.
// The file lines reach the gather as text, the way the entry point hands them.
// The ten entry-point cases run the entry point, because the fit walk is the
// first reader of the settled path: each writes its layout file, hands the
// entry point its path, and reads the two streams of a child. The machine
// shapes those runs need stand installed. No case reads `/sys` or the host
// topology, starts a thread or reads a clock (FR-036, FR-037, R-5).
//
// Check map:
//   theWordNamesTheRowAndSettlesItsPath
//                                the bare word answers the needs-a-value line a
//                                required row gives, the word with a path scans
//                                clean, and the resolved option set carries
//                                that path (FR-011, specs/028 FR-008)
//   theRowCarriesItsRecord       the row of the live table carries the value
//                                299, a required argument and the owner this
//                                feature names (FR-011, specs/028 FR-001,
//                                FR-004)
//   theHelpAnswerCarriesTheRowLine
//                                `--help` prints the line the live row builds,
//                                in the table's own words (FR-011, specs/028
//                                FR-015, FR-018)
//   theRowCarriesNoEnvironmentSource
//                                the row flags no environment source, and a
//                                variable that carries the name settles and
//                                discloses nothing (FR-011, specs/028 FR-026)
//   theWordsHoldThePathOverTheFile
//                                the word of the command line keeps its path
//                                over a file line that names one, and the
//                                dropped line discloses nothing (specs/028
//                                FR-037, FR-039)
//   theLastFileLineHoldsThePath  two lines of one file settle the path of the
//                                last of them, disclosed once (specs/028
//                                FR-035, FR-040)
//   theMissingFileRefusesOneLine a path no host opens answers the reader's
//                                  one line naming it and the reason the open
//                                  answered, at status 2, with no row (FR-018,
//                                  FR-021)
//   theDirectoryRefusesOneLine   a path that opens and then fails to read
//                                  answers the same shape with its reason, at
//                                  status 2, with no row (FR-018, FR-021)
//   theLayoutThatFitsRuns        a file whose slot count matches the batch lets
//                                  the run reach its row (FR-020)
//   theRunThatNamesNoLayoutRuns  a run no source named a layout for reaches
//                                  its row and names no partition (FR-007)
//   theOneContextRunWithTheFileRuns
//                                  one context reads the file that names its
//                                  one slot, carries no group, and keeps the
//                                  standing shape (FR-010, FR-019)
//   thePresetThatNamesGroupsRefusesThePairing
//                                  a layout beside `--placement=l3` refuses one
//                                  line naming the row, the preset and the path
//                                  (FR-019, FR-021, D-2)
//   theHandlePresetRefusesThePairing
//                                  a handle that states `l3` beside an option
//                                  that states `none` refuses the same line at
//                                  the fit checkpoint (FR-019)
//   thePresetThatNamesNoGroupsStandsAllowed
//                                  `--placement=core` over a read that names no
//                                  L3 unit runs beside the file, and its pins
//                                  stand (FR-019, FR-006)
//   theHuntPairingRefusesOneLine a layout beside `--hunt` answers the standing
//                                  combination line naming both (FR-019, D-2)
//   theBatchThatDoesNotFitStops  the counts 4, 2 and 8 beside a four-slot file
//                                  stop the batch one line naming the path and
//                                  the count, with no row (FR-020, FR-021)
// ============================================================================

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "detail/internal.hpp"
#include "detail/options.hpp"
#include "speedgun-ng/benchmark.hpp"

namespace
{

using Scan = std::expected<sg::detail::CommandLineOptions, std::string>;
using Gathered = std::expected<sg::detail::ResolvedOptions, std::string>;

constexpr int kGroupLayoutValue = 299;
constexpr std::string_view kGroupName = "group-layout";
constexpr std::string_view kGroupEnvironment = "SPEEDGUN_GROUP_LAYOUT";

// The paths no case creates. The row carries a path and opens nothing, so an
// absent file answers the same at this seam as one that stands (specs/028
// D-05).
const std::string kWordPath = "/tmp/harness_group_cli_word.yaml";
const std::string kFileFirst = "/tmp/harness_group_cli_first.yaml";
const std::string kFileSecond = "/tmp/harness_group_cli_second.yaml";
const std::string kFilePath = "/tmp/harness_group_cli_layouts.conf";

auto fail(const std::string& what) -> void
{
  std::fprintf(stderr, "SPEEDGUN-NG TEST FAIL: %s\n", what.c_str());
  std::exit(1);
}

auto check(const bool cond, const std::string& what) -> void
{
  if (!cond) {
    fail(what);
  }
}

auto checkText(const std::string& actual,
               const std::string& expected,
               const std::string& what) -> void
{
  if (actual == expected) {
    return;
  }
  fail(what + "\n  expected: " + expected + "\n  actual:   " + actual);
}

auto readText(const std::filesystem::path& path) -> std::string
{
  std::ifstream file(path);
  std::ostringstream text;
  text << file.rdbuf();
  return text.str();
}

// One scan of one command line through the seam the scan owns. The words stand
// in this process's memory alone, so the scan reads no environment and opens no
// file (specs/028 FR-012, D-05).
auto scanWords(const std::vector<std::string>& words) -> Scan
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_group_cli_test"));
  for (const std::string& word : words) {
    argv.push_back(const_cast<char*>(word.c_str()));
  }
  return sg::detail::scanCommandLine(static_cast<int>(argv.size()),
                                     argv.data());
}

// The gather of one scan over the sources a case hands as data (specs/028 D-04,
// D-05).
auto gather(const sg::detail::CommandLineOptions& scan,
            const std::vector<sg::detail::EnvironmentRow>& environment,
            const std::optional<sg::detail::OptionFileText>& optionFile)
    -> Gathered
{
  return sg::detail::gatherOptionSources(scan, environment, optionFile);
}

// The path the resolved option set carries.
auto pathOf(const sg::detail::ResolvedOptions& resolved) -> std::string
{
  return resolved.groupLayout;
}

// The uses that name the layout row.
auto groupUses(const sg::detail::ResolvedOptions& resolved) -> std::size_t
{
  std::size_t found = 0;
  for (const sg::detail::OptionUse& use : resolved.uses) {
    if (use.row->name == kGroupName) {
      ++found;
    }
  }
  return found;
}

// The row of the live table, or no row when the table holds none (specs/028
// FR-001).
auto groupRow() -> const sg::detail::OptionRow*
{
  return sg::detail::rowNamed(kGroupName);
}

// The printed line of the help answer that names `name`, with the standard
// output captured to a file beside this process number. The entry point prints
// the answer and stops at status 0, so no instance runs and no topology reads
// (specs/028 FR-017). An empty answer means no printed line names it.
auto helpAnswerLine(std::string_view name) -> std::string
{
  const std::filesystem::path outPath = std::filesystem::temp_directory_path()
      / ("harness_group_cli_" + std::to_string(static_cast<long>(getpid()))
         + ".out");
  const int savedOut = dup(STDOUT_FILENO);
  if (savedOut < 0 || std::freopen(outPath.c_str(), "w", stdout) == nullptr) {
    fail("the standard output of the entry point can be redirected");
    return {};
  }
  char program[] = "harness_group_cli_test";
  char word[] = "--help";
  char* argv[] = {program, word, nullptr};
  const int status = sg::speedgunMain(2, argv);
  std::fflush(stdout);
  if (dup2(savedOut, STDOUT_FILENO) < 0) {
    fail("the standard output of the entry point can be restored");
  }
  close(savedOut);
  const std::string printed = readText(outPath);
  std::filesystem::remove(outPath);
  check(status == 0,
        "the help answer stops at status 0 (specs/028 FR-017): "
            + std::to_string(status));
  const std::string prefix = "  --" + std::string(name);
  std::string current;
  std::string found;
  for (const char letter : printed) {
    if (letter != '\n') {
      current += letter;
      continue;
    }
    if (found.empty() && current.starts_with(prefix)) {
      found = current;
    }
    current.clear();
  }
  return found;
}

// The word that names the row settles its path, and the bare word answers the
// needs-a-value line a required row gives (FR-011, specs/028 FR-008).
auto theWordNamesTheRowAndSettlesItsPath() -> void
{
  const Scan bare = scanWords({"--group-layout"});
  check(!bare.has_value(),
        "the bare word names a row that needs a value (specs/028 FR-008)");
  if (!bare.has_value()) {
    checkText(
        bare.error(),
        "option '--group-layout' needs a value",
        "the bare word answers the needs-a-value line of the row "
        "(specs/028 FR-008)");
  }

  const Scan given = scanWords({"--group-layout=" + kWordPath});
  check(given.has_value(),
        "the scan recognizes --group-layout (FR-011): " + given.error_or({}));
  if (!given.has_value()) {
    return;
  }
  checkText(
      pathOf(given->settled),
      kWordPath,
      "the resolved option set carries the path the word named " "(FR-011)");
}

// The row of the table carries the value, the argument kind and the owner this
// feature names (FR-011, specs/028 FR-001, FR-004).
auto theRowCarriesItsRecord() -> void
{
  const sg::detail::OptionRow* const row = groupRow();
  check(row != nullptr,
        "the option table carries the group-layout row (FR-011)");
  if (row == nullptr) {
    return;
  }
  check(row->value == kGroupLayoutValue,
        "the row answers the value 299 of the free block (FR-011): "
            + std::to_string(row->value));
  check(sg::detail::kGroupLayoutOptionValue == kGroupLayoutValue,
        "the constant of the row spells 299 (FR-011): "
            + std::to_string(sg::detail::kGroupLayoutOptionValue));
  check(row->argument == sg::detail::OptionArgument::REQUIRED,
        "the row takes a required argument (FR-011)");
  checkText(row->owner,
            "specs/039",
            "the row names the requirement that owns it (FR-011)");
}

// The help answer prints one line per row out of the row's own words, so the
// live row is the expectation (FR-011, specs/028 FR-015, FR-018).
auto theHelpAnswerCarriesTheRowLine() -> void
{
  const sg::detail::OptionRow* const row = groupRow();
  check(row != nullptr,
        "the option table carries the group-layout row (FR-011)");
  if (row == nullptr) {
    return;
  }
  checkText(
      helpAnswerLine(kGroupName),
      "  --" + std::string(kGroupName) + " <value>  " + row->help,
      "the help answer carries the line the row builds (FR-011, "
      "specs/028 FR-015)");
}

// The row carries no environment source: the gather never asks for a variable
// of its name, so such a variable settles nothing and discloses nothing
// (FR-011, specs/028 FR-026).
auto theRowCarriesNoEnvironmentSource() -> void
{
  const sg::detail::OptionRow* const row = groupRow();
  check(row != nullptr,
        "the option table carries the group-layout row (FR-011)");
  if (row == nullptr) {
    return;
  }
  check(!row->fromEnvironment, "the row flags no environment source (FR-011)");

  const Scan empty = scanWords({});
  check(empty.has_value(),
        "a line with no word scans clean (specs/028 FR-012): "
            + empty.error_or({}));
  if (!empty.has_value()) {
    return;
  }
  const Gathered gathered = gather(
      *empty, {{std::string(kGroupEnvironment), kWordPath}}, std::nullopt);
  check(
      gathered.has_value(),
      "the gather accepts a variable no flagged row claims (specs/028 FR-026): "
          + gathered.error_or({}));
  if (!gathered.has_value()) {
    return;
  }
  checkText(pathOf(*gathered),
            {},
            "the variable of the row's name settles no path (FR-011)");
  check(groupUses(*gathered) == 0,
        "the variable of the row's name discloses no use (specs/028 FR-039)");
}

// The words stand above the file: the path the command line named keeps its
// place, and the file's line sets nothing and discloses nothing (specs/028
// FR-037, FR-039).
auto theWordsHoldThePathOverTheFile() -> void
{
  const Scan given = scanWords({"--group-layout=" + kWordPath});
  check(given.has_value(),
        "the scan recognizes --group-layout (FR-011): " + given.error_or({}));
  if (!given.has_value()) {
    return;
  }
  const sg::detail::OptionFileText file {kFilePath,
                                         "group-layout=" + kFileFirst + "\n"};
  const Gathered gathered = gather(*given, {}, file);
  check(gathered.has_value(),
        "the gather reads a file line of the layout row: "
            + gathered.error_or({}));
  if (!gathered.has_value()) {
    return;
  }
  checkText(pathOf(*gathered),
            kWordPath,
            "the word of the command line holds the path over the file line "
            "(specs/028 FR-037)");
  check(groupUses(*gathered) == 0,
        "the dropped file line discloses no use (specs/028 FR-039)");
}

// Two lines of one file are one source: the last line's path stands, and the
// row discloses the text that took effect (specs/028 FR-035, FR-040).
auto theLastFileLineHoldsThePath() -> void
{
  const Scan empty = scanWords({});
  check(empty.has_value(),
        "a line with no word scans clean (specs/028 FR-012): "
            + empty.error_or({}));
  if (!empty.has_value()) {
    return;
  }
  const sg::detail::OptionFileText file {
      kFilePath,
      "group-layout=" + kFileFirst + "\ngroup-layout=" + kFileSecond + "\n"};
  const Gathered gathered = gather(*empty, {}, file);
  check(gathered.has_value(),
        "the gather reads two file lines of the layout row: "
            + gathered.error_or({}));
  if (!gathered.has_value()) {
    return;
  }
  checkText(pathOf(*gathered),
            kFileSecond,
            "the last line of one file settles the path (specs/028 FR-035)");
  check(groupUses(*gathered) == 1,
        "the row discloses one use for the settled path (specs/028 FR-040): "
            + std::to_string(groupUses(*gathered)));
}

// The paths of this half. Two of them stand written below before the child
// runs, and one of them stands uncreated on purpose: the case that reads the
// unreadable path is the only reader of it (FR-018).
const std::string kProcessTag = std::to_string(static_cast<long>(getpid()));
const std::string kPairPath =
    "/tmp/harness_group_cli_pair_" + kProcessTag + ".yaml";
const std::string kSoloPath =
    "/tmp/harness_group_cli_solo_" + kProcessTag + ".yaml";
const std::string kFourPath =
    "/tmp/harness_group_cli_four_" + kProcessTag + ".yaml";
const std::string kMissingPath =
    "/tmp/harness_group_cli_missing_" + kProcessTag + ".yaml";

// Two contexts' worth of slots in the grammar FR-012 fixes: the file fits a
// two-context instance and is the run's one partition source (FR-020).
constexpr std::string_view kPairBody =
    "groups:\n  - name: pair\n    slots: [0, 1]\n";

// One context's worth of slots, the file the one-context instance reads
// (FR-010).
constexpr std::string_view kSoloBody =
    "groups:\n  - name: solo\n    slots: [0]\n";

// One group of four slots, the count the batch of 4, 2 and 8 cannot all
// carry (FR-020, D-8).
constexpr std::string_view kFourBody =
    "groups:\n  - name: wide\n    slots: [0, 1, 2, 3]\n";

auto bmGroupMissing(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmGroupDirectory(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmGroupFits(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmGroupNoLayout(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmGroupSolo(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmGroupPairing(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmGroupHandle(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmGroupNoL3(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmGroupHunt(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

auto bmGroupBatch(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

// The two-group read of the pairing runs: two L3 groups of one core and two
// threads each, so `l3` seats its two contexts and names the partition the
// pairing refuses (FR-019). The read states no memory node, which no case
// here reads.
auto l3Shape() -> sg::detail::MachineLayout
{
  sg::detail::MachineLayout installed;
  installed.readable = true;
  installed.packages = 1;
  for (std::uint32_t group = 0; group < 2; ++group) {
    sg::detail::LayoutCore layoutCore;
    layoutCore.groupOrdinal = group;
    for (std::uint32_t sibling = 0; sibling < 2; ++sibling) {
      sg::detail::LayoutThread thread;
      thread.cpuIndex = group * 2 + sibling;
      thread.coreOrdinal = static_cast<std::uint32_t>(installed.cores.size());
      thread.groupOrdinal = group;
      layoutCore.threads.push_back(
          static_cast<std::uint32_t>(installed.threads.size()));
      installed.threads.push_back(thread);
      installed.mask.push_back(thread.cpuIndex);
    }
    installed.cores.push_back(layoutCore);
    sg::detail::LayoutGroup layoutGroup;
    layoutGroup.lowestCpu = group * 2;
    layoutGroup.cores.push_back(group);
    installed.groups.push_back(layoutGroup);
  }
  return installed;
}

// The read that states cores and no L3 unit: four cores of two threads, the
// group of every thread left at `kNoGroup` the way the read leaves it, so
// `core` seats two contexts and names no partition (FR-006).
auto noL3Shape() -> sg::detail::MachineLayout
{
  sg::detail::MachineLayout installed;
  installed.readable = true;
  installed.packages = 1;
  for (std::uint32_t core = 0; core < 4; ++core) {
    sg::detail::LayoutCore layoutCore;
    for (std::uint32_t sibling = 0; sibling < 2; ++sibling) {
      sg::detail::LayoutThread thread;
      thread.cpuIndex = core * 2 + sibling;
      thread.coreOrdinal = core;
      layoutCore.threads.push_back(core * 2 + sibling);
      installed.threads.push_back(thread);
      installed.mask.push_back(thread.cpuIndex);
    }
    installed.cores.push_back(layoutCore);
  }
  return installed;
}

// Each child registers the one benchmark its scenario filters for and installs
// the shape its scenario reads, so a stop line can only come from the input
// that scenario names.
auto setupMissing() -> void
{
  (void)sg::registerBenchmark(&bmGroupMissing, "bmGroupMissing");
}

auto setupDirectory() -> void
{
  (void)sg::registerBenchmark(&bmGroupDirectory, "bmGroupDirectory");
}

auto setupFits() -> void
{
  sg::detail::installTestLayout(noL3Shape());
  auto handle = sg::registerBenchmark(&bmGroupFits, "bmGroupFits");
  handle.threads(2);
}

auto setupNoLayout() -> void
{
  (void)sg::registerBenchmark(&bmGroupNoLayout, "bmGroupNoLayout");
}

auto setupSolo() -> void
{
  (void)sg::registerBenchmark(&bmGroupSolo, "bmGroupSolo");
}

auto setupPairing() -> void
{
  sg::detail::installTestLayout(l3Shape());
  auto handle = sg::registerBenchmark(&bmGroupPairing, "bmGroupPairing");
  handle.threads(2);
}

auto setupHandle() -> void
{
  sg::detail::installTestLayout(l3Shape());
  auto handle = sg::registerBenchmark(&bmGroupHandle, "bmGroupHandle");
  handle.threads(2).placement("l3");
}

auto setupNoL3() -> void
{
  sg::detail::installTestLayout(noL3Shape());
  auto handle = sg::registerBenchmark(&bmGroupNoL3, "bmGroupNoL3");
  handle.threads(2);
}

auto setupHunt() -> void
{
  (void)sg::registerBenchmark(&bmGroupHunt, "bmGroupHunt");
}

auto setupBatch() -> void
{
  (void)sg::registerBenchmark(&bmGroupBatch, "bmGroupBatch");
}

auto writeLayout(const std::string& path, std::string_view body) -> void
{
  std::ofstream file(path, std::ios::trunc);
  file << body;
  file.flush();
  check(static_cast<bool>(file), "the case can write its layout file " + path);
}

// The count of lines a captured stream printed: one line per newline.
auto lineCount(const std::string& text) -> std::size_t
{
  std::size_t lines = 0;
  for (const char letter : text) {
    if (letter == '\n') {
      ++lines;
    }
  }
  return lines;
}

// The two streams of one entry-point run, captured to files beside this
// process number so the child writes them and the parent reads them.
struct Captured
{
  int status;
  std::string out;
  std::string err;
};

auto capturePath(const std::string& mode,
                 const std::string& suffix) -> std::string
{
  return (std::filesystem::temp_directory_path()
          / ("harness_group_cli_" + mode + "_" + kProcessTag + suffix))
      .string();
}

// Fork a child that registers its benchmark, runs the entry point with
// `arguments`, and leaves with its status: the hunt pairing refusal reaches
// `std::exit` from inside the entry point, the way every rejected hunt value
// does.
auto runChild(const std::string& mode,
              const std::vector<std::string>& arguments,
              void (*setup)()) -> Captured
{
  const std::string outPath = capturePath(mode, ".out");
  const std::string errPath = capturePath(mode, ".err");
  const pid_t child = fork();
  if (child < 0) {
    fail("the case can fork a child");
  }
  if (child == 0) {
    if (std::freopen(outPath.c_str(), "w", stdout) == nullptr
        || std::freopen(errPath.c_str(), "w", stderr) == nullptr)
    {
      _exit(3);
    }
    setup();
    std::vector<std::string> words {"harness_group_cli_test"};
    for (const std::string& argument : arguments) {
      words.push_back(argument);
    }
    std::vector<char*> argv;
    for (std::string& word : words) {
      argv.push_back(word.data());
    }
    const int status =
        sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
    std::fflush(stdout);
    std::fflush(stderr);
    // std::exit runs the atexit handlers, and the coverage preset writes its
    // counters from one, so the child's walk of the fit paths reaches that
    // handler. The two streams stand flushed first, so the captured files hold
    // exactly the child's bytes.
    std::exit(status);
  }
  int waited = 0;
  check(waitpid(child, &waited, 0) == child, "the case can wait for its child");
  check(WIFEXITED(waited), "the child exited normally");
  const Captured captured {
      WEXITSTATUS(waited), readText(outPath), readText(errPath)};
  std::filesystem::remove(outPath);
  std::filesystem::remove(errPath);
  return captured;
}

// FR-018, FR-021: a path no host opens answers the reader's own line naming
// it, at status 2, ahead of every run.
auto theMissingFileRefusesOneLine() -> void
{
  std::filesystem::remove(kMissingPath);
  const Captured run = runChild(
      "missing",
      {"--group-layout=" + kMissingPath, "--filter", "^bmGroupMissing$"},
      &setupMissing);
  check(
      run.status == 2,
      "a layout no host opens exits 2 (FR-018): " + std::to_string(run.status));
  check(lineCount(run.err) == 1,
        "the unreadable path answers one line (FR-018): " + run.err);
  check(run.err.find(kMissingPath) != std::string::npos,
        "the line names the layout path (FR-021): " + run.err);
  check(run.err.find("No such file or directory") != std::string::npos,
        "the line carries the reason the open answered (FR-018): " + run.err);
  check(run.out.find("bmGroupMissing") == std::string::npos,
        "no benchmark ran (FR-018): " + run.out);
}

// FR-018, FR-021: a path the host opens and then fails to read answers the
// reader's own line naming it and the reason that read answered, at status 2,
// ahead of every run. The document stands behind that read, so no stream
// failure of the loader reaches the entry point.
auto theDirectoryRefusesOneLine() -> void
{
  const Captured run =
      runChild("directory",
               {"--group-layout=/tmp", "--filter", "^bmGroupDirectory$"},
               &setupDirectory);
  check(
      run.status == 2,
      "a layout no host reads exits 2 (FR-018): " + std::to_string(run.status));
  check(lineCount(run.err) == 1,
        "the unreadable path answers one line (FR-018): " + run.err);
  check(run.err.find("/tmp") != std::string::npos,
        "the line names the layout path (FR-021): " + run.err);
  check(run.err.find("Is a directory") != std::string::npos,
        "the line carries the reason the read answered (FR-018): " + run.err);
  check(run.out.find("bmGroupDirectory") == std::string::npos,
        "no benchmark ran (FR-018): " + run.out);
}

// FR-020: the fit walk reads a file that fits and lets the run reach its row.
auto theLayoutThatFitsRuns() -> void
{
  writeLayout(kPairPath, kPairBody);
  const Captured run =
      runChild("fits",
               {"--group-layout=" + kPairPath, "--filter", "^bmGroupFits"},
               &setupFits);
  check(run.status == 0,
        "a layout that fits the batch runs (FR-020): "
            + std::to_string(run.status) + " " + run.err);
  check(run.err.find("--group-layout") == std::string::npos,
        "a file that fits answers no refusal line (FR-020): " + run.err);
  check(run.out.find("bmGroupFits") != std::string::npos,
        "the run reaches its row (FR-020): " + run.out);
}

// FR-007: a run no source named a layout for keeps its standing shape.
auto theRunThatNamesNoLayoutRuns() -> void
{
  const Captured run =
      runChild("noLayout", {"--filter", "^bmGroupNoLayout$"}, &setupNoLayout);
  check(run.status == 0,
        "a run with no layout word runs (FR-007): " + std::to_string(run.status)
            + " " + run.err);
  check(run.err.find("--group-layout") == std::string::npos,
        "the fit arm reads no file the run did not name (FR-007): " + run.err);
  check(run.out.find("bmGroupNoLayout") != std::string::npos,
        "the run reaches its row (FR-007): " + run.out);
}

// FR-010, FR-019: a one-context instance reads the file that names its one
// slot, carries no group, and keeps the standing shape.
auto theOneContextRunWithTheFileRuns() -> void
{
  writeLayout(kSoloPath, kSoloBody);
  const Captured run =
      runChild("solo",
               {"--group-layout=" + kSoloPath, "--filter", "^bmGroupSolo$"},
               &setupSolo);
  check(run.status == 0,
        "a one-context instance runs beside the file (FR-010): "
            + std::to_string(run.status) + " " + run.err);
  check(run.err.find("--group-layout") == std::string::npos,
        "one context answers no pairing line (FR-010): " + run.err);
  check(run.out.find("bmGroupSolo") != std::string::npos,
        "the run reaches its row (FR-010): " + run.out);
}

// FR-019, FR-021, D-2: the layout beside a preset that names a partition for
// the read in hand refuses one line naming the row, the preset and the path.
auto thePresetThatNamesGroupsRefusesThePairing() -> void
{
  writeLayout(kPairPath, kPairBody);
  const Captured run = runChild("pairing",
                                {"--group-layout=" + kPairPath,
                                 "--placement=l3",
                                 "--filter",
                                 "^bmGroupPairing"},
                                &setupPairing);
  check(run.status == 2,
        "the pairing with a partition-naming preset exits 2 (FR-019): "
            + std::to_string(run.status));
  checkText(run.err,
            "--group-layout '" + kPairPath
                + "' cannot combine with --placement='l3'\n",
            "one line names the row, the path and the preset (FR-019, FR-021, "
            "D-2)");
  check(run.out.find("bmGroupPairing") == std::string::npos,
        "no benchmark ran (FR-019): " + run.out);
}

// FR-019: the name a handle stated carries the same refusal as the name the
// option settled, and the fit checkpoint is where the pair meets.
auto theHandlePresetRefusesThePairing() -> void
{
  writeLayout(kPairPath, kPairBody);
  const Captured run = runChild("handle",
                                {"--group-layout=" + kPairPath,
                                 "--placement=none",
                                 "--filter",
                                 "^bmGroupHandle"},
                                &setupHandle);
  check(run.status == 2,
        "a handle that states `l3` exits 2 beside the file (FR-019): "
            + std::to_string(run.status));
  checkText(run.err,
            "--group-layout '" + kPairPath
                + "' cannot combine with --placement='l3'\n",
            "the line names the preset the handle stated (FR-019, FR-021)");
  check(run.out.find("bmGroupHandle") == std::string::npos,
        "no benchmark ran (FR-019): " + run.out);
}

// FR-019, FR-006: a preset that names no partition for the read in hand keeps
// its pins and stands allowed beside the file.
auto thePresetThatNamesNoGroupsStandsAllowed() -> void
{
  writeLayout(kPairPath, kPairBody);
  const Captured run = runChild("noL3",
                                {"--group-layout=" + kPairPath,
                                 "--placement=core",
                                 "--filter",
                                 "^bmGroupNoL3"},
                                &setupNoL3);
  check(run.status == 0,
        "`core` over a read that states no L3 unit runs beside the file "
        "(FR-019): "
            + std::to_string(run.status) + " " + run.err);
  check(run.err.find("--group-layout") == std::string::npos,
        "a preset that names no partition answers no pairing line (FR-019): "
            + run.err);
  check(run.out.find("bmGroupNoL3") != std::string::npos,
        "the run reaches its row (FR-019): " + run.out);
}

// FR-019, D-2: the layout beside `--hunt` answers the standing combination
// line, the shape `--hunt cannot combine with --placement` keeps.
auto theHuntPairingRefusesOneLine() -> void
{
  writeLayout(kPairPath, kPairBody);
  const Captured run = runChild("hunt",
                                {"--group-layout=" + kPairPath,
                                 "--hunt=all",
                                 "--filter",
                                 "^bmGroupHunt$"},
                                &setupHunt);
  check(run.status == 2,
        "the pairing with `--hunt` exits 2 (FR-019): "
            + std::to_string(run.status));
  checkText(run.err,
            "--hunt cannot combine with --group-layout\n",
            "the pairing keeps the standing combination line (FR-019, D-2)");
  check(run.out.find("bmGroupHunt") == std::string::npos,
        "no benchmark ran (FR-019): " + run.out);
}

// FR-020, FR-021: the slot count of the file is the count of one instance, so
// a batch of differing counts stops at the fit step, one line naming the path
// and the count it failed against, with no row printed.
auto theBatchThatDoesNotFitStops() -> void
{
  writeLayout(kFourPath, kFourBody);
  const Captured run = runChild("batch",
                                {"--group-layout=" + kFourPath,
                                 "--threads=4,2,8",
                                 "--filter",
                                 "^bmGroupBatch"},
                                &setupBatch);
  check(run.status == 2,
        "a batch the slot count cannot carry exits 2 (FR-020): "
            + std::to_string(run.status));
  checkText(run.err,
            "--group-layout '" + kFourPath
                + "' names 4 slots, 'bmGroupBatch/threads:2' runs 2 contexts\n",
            "one line names the path and the two counts it failed against "
            "(FR-020, FR-021)");
  check(run.out.find("bmGroupBatch") == std::string::npos,
        "no benchmark ran (FR-020): " + run.out);
}

auto run() -> int
{
  theWordNamesTheRowAndSettlesItsPath();
  theRowCarriesItsRecord();
  theHelpAnswerCarriesTheRowLine();
  theRowCarriesNoEnvironmentSource();
  theWordsHoldThePathOverTheFile();
  theLastFileLineHoldsThePath();
  theMissingFileRefusesOneLine();
  theDirectoryRefusesOneLine();
  theLayoutThatFitsRuns();
  theRunThatNamesNoLayoutRuns();
  theOneContextRunWithTheFileRuns();
  thePresetThatNamesGroupsRefusesThePairing();
  theHandlePresetRefusesThePairing();
  thePresetThatNamesNoGroupsStandsAllowed();
  theHuntPairingRefusesOneLine();
  theBatchThatDoesNotFitStops();
  std::filesystem::remove(kPairPath);
  std::filesystem::remove(kSoloPath);
  std::filesystem::remove(kFourPath);
  std::printf("harness_group_cli_test PASS\n");
  return 0;
}

}  // namespace

auto main() -> int
{
  try {
    return run();
  } catch (...) {
    std::fprintf(stderr, "SPEEDGUN-NG TEST FAIL: unhandled exception\n");
    return 1;
  }
}

// The split the row's own reach fixes. The six row cases answer at the scan,
// the settle arm and the gather, which open nothing. The ten entry-point
// cases answer at the fit walk, the first reader of the settled path: the
// path no host opens, the path no host reads, the file that fits, the run
// that named no layout, the one-context instance, the two pairings a
// partition-naming preset meets, the preset that names no partition for the
// read, the pairing beside `--hunt`, and the batch the slot count stops
// (FR-018 to FR-021, D-2, B-3).
