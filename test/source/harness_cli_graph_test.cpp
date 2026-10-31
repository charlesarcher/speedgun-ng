// ============================================================================
// specs/037-execution-graphs module M3 `cli-row`, wave 3: the `--graph` row
// of the option table, its grammar, its bare form, its option-file source, the
// count refusals and the parse-once rule (T020, T021; US3; FR-001, FR-002,
// FR-009, FR-011, FR-012; SC-003; R-01, R-02, R-05, R-11;
// contracts/cli-graph.md).
//
// Every scenario runs the entry point in a forked child with both streams
// captured to files the parent reads after the wait, the shape
// harness_cli_placement_test.cpp gives a grammar that can exit from inside the
// entry point. The graph stands in a planted file under the system temporary
// directory, so the reader reads the file system it reads in production and no
// scenario installs a graph through the seam (FR-009, FR-011). The row carries
// the environment flag off, so one scenario plants `SPEEDGUN_GRAPH` in the
// child's environment and states that the run reads no variable for the row
// (FR-009, R-02).
//
// The filters anchor at the start and drop the end: a threaded instance carries
// the `/threads:<n>` segment above one context, the standing naming rule of
// `family.cpp:246`.
//
// The start order is the run-visible fact that a graph file reached the gate:
// the body of each context records the rank at which its context started, and
// the child prints those ranks in start order. The planted chain names its
// nodes in an order other than their slots. The run answers in the order the
// graph's slots give, and the file's node order plays no part in it.
//
// Scenario map:
//   graphRowPrintsHelpAndOptions  `--help` prints the row's line, and an
//                                 option-file line settles the row and
//                                 discloses it (FR-001, FR-002, FR-035)
//   graphEqualsFileReads          `--graph=<file>` orders the run by the named
//                                 file (FR-009)
//   bareGraphTakesOptionFilePath  the bare row takes its path from the option
//                                 file line and reads no environment variable
//                                 (FR-009, R-02)
//   bareGraphNoPathRefuses        the bare row with no path in any source
//                                 prints one line and exits 2 before any
//                                 context starts (FR-009, SC-003)
//   countMismatchRefuses          a three-node graph over a two-context run
//                                 prints one line naming both counts and
//                                 exits 2 (FR-012, SC-003)
//   noThreadedRunRefuses          a batch with no threaded run prints one line
//                                 and exits 2 (FR-012, SC-003)
//   singleContextTakesNoGraph     a single-context run beside the graph keeps
//                                 the standing bytes, gains no record entry
//                                 and exits 0 (FR-012, D-2)
//   graphParsesOnceBeforeRun      the file reads once ahead of the first
//                                 context: a batch of two threaded runs over a
//                                 refused file answers one line, exits 2, and
//                                 runs no benchmark (FR-011, SC-003)
//   graphRunWritesRecord    the graph run beside a document: the console
//                                 block prints the nodes and edges line, and
//                                 the document carries the same entry with two
//                                 empty arrays (FR-022, FR-023)
//   failedRunCarriesHeldRecord
//                                 the run whose source fails: the standing
//                                 run-failure status, a FAILED line and no
//                                 row, no block beside no row, and the
//                                 document carrying the held words with their
//                                 gating node (FR-019, FR-020, FR-021, FR-023)
//   recordEntryCarriesHeldAndStopped
//                                 the assembly of one entry: a held word joins
//                                 the held array with the node that gates it,
//                                 a stopped word joins the stopped array, and
//                                 an empty settlement assembles no entry
//                                 (FR-022, FR-023)
//   graphTwiceOnOneLineKeepsLast  two graph words on one command line: the
//                                 last word of the source orders the run
//                                 (FR-035)
//   graphOptionFileLineDrops      the command line's path beside a file line:
//                                 the line sets nothing and discloses nothing
//                                 (FR-037, FR-038, FR-040)
//   bareGraphEmptyOptionFileRefuses
//                                 the bare row beside a file line of no path:
//                                 one line and exit 2 (FR-009, SC-003)
//
// `argv[1]` names one scenario, so a red arm answers its own line on its own;
// with no argument the list at the bottom runs the file, and that is what
// CTest runs.
//
// Frameworkless check()/fail() convention: every check stands in the parent,
// so a child's status and its two captures are the reading.
// ============================================================================

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "detail/graph.hpp"
#include "detail/options.hpp"
#include "speedgun-ng/barrier.hpp"
#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters.hpp"
#include "speedgun-ng/counters_fake.hpp"

// The record entry of one threaded run: the nodes with the word each settled,
// the edges in the file's order, one held entry per held node with the failed
// node that gates it, and one stopped name per stopped node. The definition
// stands in `source/harness/cli.cpp` (T023); this block carries its own
// declaration for this translation unit, the way
// `harness_graph_file_test.cpp:101` declares the graph file reader it drives.
namespace sg
{
[[nodiscard]] auto graphRunEntryOf(
    const std::string& benchmark,
    const detail::Graph& graph,
    const std::vector<detail::GraphSettlement>& settlement)
    -> std::optional<GraphRunRecord>;
}  // namespace sg

namespace
{

[[noreturn]] auto fail(const char* what) -> void
{
  std::fprintf(stderr, "HARNESS CLI GRAPH TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool condition, const char* what) -> void
{
  if (!condition) {
    fail(what);
  }
}

// The one failure reading of a comparison: the words the scenario names, the
// text it expected and the text the run answered.
[[noreturn]] auto report(const char* what,
                         const std::string& expected,
                         const std::string& actual) -> void
{
  std::fprintf(
      stderr,
      "HARNESS CLI GRAPH TEST FAIL: %s\n  expected: %s\n  " "actual:   %s\n",
      what,
      expected.c_str(),
      actual.c_str());
  std::exit(1);
}

// ---------------------------------------------------------------------------
// The planted files
// ---------------------------------------------------------------------------

// One node of a planted graph file, in the shape `docs/schema/graph-v1.json`
// names: the name, the slot and the dependency list, in that key order
// (FR-007, FR-008).
struct NodeSpec
{
  std::string name;
  int slot = 0;
  std::vector<std::string> deps;
};

// The document one node list spells. The text is the file's whole content, so
// a scenario states the shape the reader checks (FR-007).
auto graphText(const std::vector<NodeSpec>& nodes) -> std::string
{
  std::string text = R"({"nodes":[)";
  for (std::size_t index = 0; index < nodes.size(); ++index) {
    if (index != 0) {
      text += ",";
    }
    text += R"({"name":")" + nodes[index].name + R"(","slot":)"
        + std::to_string(nodes[index].slot) + R"(,"deps":[)";
    for (std::size_t dep = 0; dep < nodes[index].deps.size(); ++dep) {
      if (dep != 0) {
        text += ",";
      }
      text += "\"" + nodes[index].deps[dep] + "\"";
    }
    text += "]}";
  }
  text += "]}";
  return text;
}

// One planted file, named by the process number and the scenario, so two runs
// never share one and the tree keeps no file behind.
auto plant(const std::string& name, const std::string& text) -> std::string
{
  const auto path = std::filesystem::temp_directory_path()
      / ("speedgun-cli-graph-" + std::to_string(static_cast<long>(getpid()))
         + "-" + name);
  std::ofstream file(path);
  file << text;
  file.close();
  if (!file) {
    fail("the scenario's file can be planted");
  }
  return path.string();
}

// The chain whose nodes stand in an order other than their slots: `a` at slot
// 2, `b` at slot 1 behind `a`, `c` at slot 0 behind `b`. The gate releases the
// chain in dependency order, so the run starts slot 2, then 1, then 0: an
// order no file order and no dispatch order answers (FR-013, FR-009).
auto chainFileText() -> std::string
{
  return graphText({
      NodeSpec {"a", 2, {}},
      NodeSpec {"b", 1, {"a"}},
      NodeSpec {"c", 0, {"b"}},
  });
}

// Two nodes with no dependency between them, the shape a two-context run
// orders (FR-012).
auto pairFileText() -> std::string
{
  return graphText({
      NodeSpec {"a", 0, {}},
      NodeSpec {"b", 1, {}},
  });
}

// A file the reader refuses: one name carried twice, the fourth fault of
// FR-010. The fault stands ahead of the run-relative rules, so the file
// answers it whatever the batch's counts are.
auto repeatedNameFileText() -> std::string
{
  return graphText({
      NodeSpec {"a", 0, {}},
      NodeSpec {"a", 1, {}},
  });
}

// ---------------------------------------------------------------------------
// The benchmarks and the bodies that record the start order
// ---------------------------------------------------------------------------

// The scripted counter provider, the shape harness_graph_gate_test.cpp gives
// its run-level scenarios: the merged row of a run is then a fixed number the
// parent reads across two children, which is what lets a byte fold stand in a
// scenario that forks.
constexpr std::uint64_t kMonotonicStep = 1000;
constexpr std::uint64_t kThreadCpuStep = 800;

auto scriptProvider() -> void
{
  auto owned = std::make_unique<sg::counters::FakeProvider>();
  owned->addCounter("machine", "monotonic", "nanoseconds", "monotonic");
  owned->addCounter("machine", "thread_cpu", "nanoseconds", "thread cpu");
  owned->setPoints("machine", "monotonic", {0}, kMonotonicStep);
  owned->setPoints("machine", "thread_cpu", {0}, kThreadCpuStep);
  const auto registered =
      sg::counters::System::local().registerProvider(std::move(owned));
  check(registered.has_value(), "the scripted provider registers");
}

constexpr int kMaxContexts = 8;
std::atomic<int> startRank[kMaxContexts] {};
std::atomic<int> startCounter {0};
// The context count whose ranks the child prints, zero for a child that runs
// no threaded run of its own.
int printedContexts = 0;

auto noteStart(sg::State& state) -> void
{
  const int index = state.threadIndex();
  if (index >= 0 && index < kMaxContexts) {
    startRank[static_cast<std::size_t>(index)].store(
        startCounter.fetch_add(1, std::memory_order_relaxed),
        std::memory_order_relaxed);
  }
}

auto bmGraphChain(sg::State& state) -> void
{
  noteStart(state);
  for (auto index : state) {
    sg::doNotOptimize(index);
  }
}

auto bmGraphPair(sg::State& state) -> void
{
  noteStart(state);
  for (auto index : state) {
    sg::doNotOptimize(index);
  }
}

auto bmGraphSingle(sg::State& state) -> void
{
  for (auto index : state) {
    sg::doNotOptimize(index);
  }
}

auto setupChainThree() -> void
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmGraphChain, "bmGraphChain");
  handle.threads(3);
  printedContexts = 3;
}

auto setupPairTwo() -> void
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmGraphPair, "bmGraphPair");
  handle.threads(2);
  printedContexts = 2;
}

// The same three-node chain over a four-context run: every slot stands inside
// the run's count, so the reader's own rules pass and the count rule of FR-012
// answers (FR-010 rule 6, FR-012).
auto setupChainFour() -> void
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmGraphChain, "bmGraphChain");
  handle.threads(4);
  printedContexts = 4;
}

auto setupSingle() -> void
{
  scriptProvider();
  (void)sg::registerBenchmark(&bmGraphSingle, "bmGraphSingle");
}

// Two single-context runs: the batch with no threaded run at all (FR-012).
auto setupTwoSingles() -> void
{
  scriptProvider();
  (void)sg::registerBenchmark(&bmGraphSingle, "bmGraphSingle");
  (void)sg::registerBenchmark(&bmGraphPair, "bmGraphPair");
}

// Two threaded runs of three contexts each: the batch the parse-once rule
// reads once (FR-011).
auto setupTwoThreaded() -> void
{
  scriptProvider();
  auto first = sg::registerBenchmark(&bmGraphChain, "bmGraphChain");
  first.threads(3);
  auto second = sg::registerBenchmark(&bmGraphPair, "bmGraphPair");
  second.threads(3);
  printedContexts = 0;
}

// The body that records an error on the context of slot 0, the shape
// `harness_graph_gate_test.cpp:924` gives the gate: the source settles its
// node failed, and the closure of its dependents settles held with it
// (FR-019, FR-020).
auto bmGraphChainFail(sg::State& state) -> void
{
  noteStart(state);
  if (state.threadIndex() == 0) {
    throw std::runtime_error("the scripted graph body failure");
  }
  for (auto index : state) {
    sg::doNotOptimize(index);
  }
}

// The chain of three contexts whose source fails: the run that settles one
// node failed and holds the closure behind it (FR-019, FR-020).
auto setupFailingChainThree() -> void
{
  scriptProvider();
  auto handle = sg::registerBenchmark(&bmGraphChainFail, "bmGraphChainFail");
  handle.threads(3);
  printedContexts = 3;
}

// ---------------------------------------------------------------------------
// The child: the entry point, both streams captured, the status answered
// ---------------------------------------------------------------------------

struct Captured
{
  int status = 0;
  std::string out;
  std::string err;
};

auto capturePath(const std::string& mode, const char* kind) -> std::string
{
  return std::string(std::filesystem::temp_directory_path().string())
      + "/speedgun-cli-graph-" + std::to_string(static_cast<long>(getpid()))
      + "-" + mode + kind;
}

auto readFile(const std::string& path) -> std::string
{
  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// Fork a child that registers the scenario's benchmarks, plants the
// environment row the scenario names, runs the entry point with `arguments`,
// prints the start ranks of a threaded run, and exits with the entry point's
// status. A refusal can exit from inside the entry point, so the work stands
// in a child.
auto runChild(const std::string& mode,
              const std::vector<std::string>& arguments,
              void (*setup)(),
              const char* environmentName = nullptr,
              const char* environmentValue = nullptr) -> Captured
{
  const std::string outPath = capturePath(mode, ".out");
  const std::string errPath = capturePath(mode, ".err");

  const pid_t pid = fork();
  if (pid < 0) {
    fail("cannot fork the child");
  }
  if (pid == 0) {
    if (std::freopen(outPath.c_str(), "w", stdout) == nullptr
        || std::freopen(errPath.c_str(), "w", stderr) == nullptr)
    {
      _exit(3);
    }
    if (environmentName != nullptr) {
      (void)::setenv(environmentName, environmentValue, 1);
    }
    if (setup != nullptr) {
      setup();
    }
    std::vector<std::string> words {"harness_cli_graph_test"};
    for (const auto& argument : arguments) {
      words.push_back(argument);
    }
    std::vector<char*> argv;
    for (auto& word : words) {
      argv.push_back(word.data());
    }
    const int status =
        sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
    auto order = std::string {};
    for (int rank = 0; rank < printedContexts; ++rank) {
      for (int slot = 0; slot < printedContexts; ++slot) {
        if (startRank[static_cast<std::size_t>(slot)].load(
                std::memory_order_relaxed)
            == rank)
        {
          if (!order.empty()) {
            order += ",";
          }
          order += std::to_string(slot);
        }
      }
    }
    std::printf("graph-order=%s\n", order.c_str());
    (void)std::fflush(stdout);
    (void)std::fflush(stderr);
    // std::exit runs the atexit handlers, and the coverage preset writes its
    // counters from one, so the child's walk reaches it (SC-07).
    std::exit(status);
  }
  int waitStatus = 0;
  if (waitpid(pid, &waitStatus, 0) < 0) {
    fail("cannot wait for the child");
  }
  if (!WIFEXITED(waitStatus)) {
    fail("the child did not exit normally");
  }
  return Captured {
      WEXITSTATUS(waitStatus), readFile(outPath), readFile(errPath)};
}

auto linesOf(const std::string& text) -> std::vector<std::string>
{
  auto lines = std::vector<std::string> {};
  auto line = std::string {};
  for (const char current : text) {
    if (current == '\n') {
      lines.push_back(line);
      line.clear();
      continue;
    }
    line += current;
  }
  if (!line.empty()) {
    lines.push_back(line);
  }
  return lines;
}

// The lines that name `needle`, joined for a failure reading.
auto namingLines(const std::vector<std::string>& lines,
                 const std::string& needle) -> std::string
{
  auto text = std::string {};
  for (const auto& line : lines) {
    if (line.find(needle) != std::string::npos) {
      if (!text.empty()) {
        text += " | ";
      }
      text += line;
    }
  }
  return text;
}

// The line the child printed for the start order, empty when it printed none.
auto orderOf(const std::string& out) -> std::string
{
  for (const auto& line : linesOf(out)) {
    if (line.rfind("graph-order=", 0) == 0) {
      return line.substr(std::string_view("graph-order=").size());
    }
  }
  return {};
}

// The measured row of one instance: the line that names it and carries its
// iteration count.
auto rowOf(const std::string& out, const std::string& name) -> std::string
{
  for (const auto& line : linesOf(out)) {
    if (line.rfind(name, 0) == 0
        && line.find("iterations=") != std::string::npos)
    {
      return line;
    }
  }
  return {};
}

// The row's overhead floor is the plan's calibration of this host's sampling
// cost: a fresh reading in every process, so a fold that crosses two children
// masks it and keeps every field the scripted provider fixes. The mask is the
// shape `harness_hunt_json_test.cpp` gives the same column: the value span of
// the fixed-width column stands replaced by one token of its own width, so the
// two rows keep their shape and the fold reads the timed fields alone.
auto floorMasked(const std::string& row) -> std::string
{
  constexpr std::string_view kFloor = "overhead floor (ns)=";
  auto text = std::string(row);
  const auto at = text.find(kFloor);
  if (at == std::string::npos) {
    return text;
  }
  const auto value = at + kFloor.size();
  auto stop = value;
  while (stop < text.size()
         && (text[stop] == ' ' || text[stop] == '.'
             || (text[stop] >= '0' && text[stop] <= '9')))
  {
    ++stop;
  }
  text.replace(value, stop - value, std::string(stop - value, '?'));
  return text;
}

// The help line of one live table row, in the shape the contract fixes: two
// leading spaces, the long name with its dashes, the argument part its kind
// implies, two spaces, the row's help text (FR-015, FR-018).
auto helpLineOf(const sg::detail::OptionRow& row) -> std::string
{
  auto argument = std::string {};
  if (row.argument == sg::detail::OptionArgument::REQUIRED) {
    argument = " <value>";
  } else if (row.argument == sg::detail::OptionArgument::OPTIONAL) {
    argument = " [<value>]";
  }
  return "  --" + std::string(row.name) + argument + "  " + row.help;
}

// ---------------------------------------------------------------------------
// The grammar scenarios (T020)
// ---------------------------------------------------------------------------

// FR-001, FR-002, FR-035: the row prints in the help answer, and an
// option-file line settles the row and discloses it.
auto scenarioRowPrintsHelpAndOptions() -> int
{
  const auto help = runChild("help", {"--help"}, nullptr);
  check(help.status == 0, "`--help` beside the graph row exits 0 (FR-017)");

  const auto* const row = sg::detail::rowNamed("graph");
  if (row == nullptr) {
    report("the live table carries the graph row, the oracle of its help line "
           "(FR-001, FR-002)",
           "a row named graph",
           "no such row");
  }
  const std::string expected = helpLineOf(*row);
  if (namingLines(linesOf(help.out), expected).empty()) {
    report("the help answer prints the graph row's line (FR-001, FR-015)",
           expected,
           namingLines(linesOf(help.out), "--graph"));
  }

  const auto path = plant("pair", pairFileText());
  const auto optionFile = plant("pair.opts", "graph=" + path + "\n");
  const auto run = runChild(
      "options",
      {"--options=" + optionFile, "--filter=^bmGraphPair", "--iterations=2"},
      &setupPairTwo);
  check(run.status == 0,
        "an option-file line settles the graph row and the run exits 0 "
        "(FR-035, FR-009)");
  const std::string disclosure =
      "options from a file or the environment: --graph=" + path;
  if (namingLines(linesOf(run.out), disclosure).empty()) {
    report("the settled option-file line discloses the graph row (FR-039, "
           "FR-040)",
           disclosure,
           namingLines(linesOf(run.out), "options from"));
  }
  return 0;
}

// FR-009: `--graph=<file>` reads the named file, and the run starts by it.
auto scenarioEqualsFileReads() -> int
{
  const auto path = plant("chain", chainFileText());
  const auto run =
      runChild("equals",
               {"--graph=" + path, "--filter=^bmGraphChain", "--iterations=2"},
               &setupChainThree);
  check(run.status == 0, "a graph run of the named file exits 0 (FR-009)");
  const auto order = orderOf(run.out);
  if (order != "2,1,0") {
    report("the run starts its contexts in the order of the named file "
           "(FR-009, FR-013)",
           "2,1,0",
           order);
  }
  return 0;
}

// FR-009, R-02: the bare row takes its path from the option file line, and the
// row reads no environment variable.
auto scenarioBareGraphTakesOptionFilePath() -> int
{
  const auto path = plant("chain", chainFileText());
  const auto optionFile = plant("bare.opts", "graph=" + path + "\n");
  const auto run = runChild("bare",
                            {"--options=" + optionFile,
                             "--graph",
                             "--filter=^bmGraphChain",
                             "--iterations=2"},
                            &setupChainThree,
                            "SPEEDGUN_GRAPH",
                            "/nonexistent/speedgun-graph-row.json");
  check(run.status == 0,
        "the bare row takes the option file's path and exits 0 (FR-009)");
  const auto order = orderOf(run.out);
  if (order != "2,1,0") {
    report("the bare row orders the run by the option file's path (FR-009)",
           "2,1,0",
           order);
  }
  const std::string disclosure =
      "options from a file or the environment: --graph=" + path;
  if (namingLines(linesOf(run.out), disclosure).empty()) {
    report("the bare row discloses the option file's path as the row's value "
           "(FR-009, FR-039)",
           disclosure,
           namingLines(linesOf(run.out), "options from"));
  }
  return 0;
}

// FR-009, SC-003: the bare row with no path in any source prints one line
// naming the row and the missing path, and exits 2 before any context starts.
auto scenarioBareGraphNoPathRefuses() -> int
{
  const auto run =
      runChild("bareNoPath",
               {"--graph", "--filter=^bmGraphChain", "--iterations=2"},
               &setupChainThree);
  check(run.status == 2,
        "the bare row with no path in any source exits 2 (FR-009, SC-003)");
  const auto faults = linesOf(run.err);
  if (faults.size() != 1) {
    report(
        "the refusal prints one line alone (FR-009, SC-003)",
        "one line",
        std::to_string(faults.size()) + " lines: " + namingLines(faults, ""));
  }
  const auto& line = faults.front();
  check(line.find("--graph") != std::string::npos,
        "the refusal names the row (FR-009)");
  check(line.find("path") != std::string::npos
            && line.find("empty") != std::string::npos,
        "the refusal names the missing path (FR-009)");
  check(run.out.find("bmGraphChain") == std::string::npos,
        "the refusal runs no benchmark (FR-009, SC-003)");
  return 0;
}

// ---------------------------------------------------------------------------
// The count and parse-once scenarios (T021)
// ---------------------------------------------------------------------------

// FR-012, SC-003: a threaded run whose context count differs from the node
// count prints one line naming both counts and exits 2. The three-node chain
// stands over a four-context run, so every slot reads inside the count and the
// reader's own rules pass: the count rule is the fault the run meets (FR-010
// rule 6, FR-012).
auto scenarioCountMismatchRefuses() -> int
{
  const auto path = plant("chain", chainFileText());
  const auto run =
      runChild("count",
               {"--graph=" + path, "--filter=^bmGraphChain", "--iterations=2"},
               &setupChainFour);
  check(run.status == 2,
        "a run whose context count differs from the node count exits 2 "
        "(FR-012, SC-003)");
  const auto faults = linesOf(run.err);
  if (faults.size() != 1) {
    report(
        "the count refusal prints one line alone (FR-012, SC-003)",
        "one line",
        std::to_string(faults.size()) + " lines: " + namingLines(faults, ""));
  }
  const auto& line = faults.front();
  check(line.find("3 nodes") != std::string::npos,
        "the count refusal names the node count (FR-012)");
  check(line.find("4 contexts") != std::string::npos,
        "the count refusal names the context count (FR-012)");
  check(run.out.find("bmGraphChain") == std::string::npos,
        "the count refusal runs no benchmark (FR-012, SC-003)");
  return 0;
}

// FR-012, SC-003: a batch with no threaded run prints one line and exits 2.
auto scenarioNoThreadedRunRefuses() -> int
{
  const auto path = plant("pair", pairFileText());
  const auto run =
      runChild("noThreaded",
               {"--graph=" + path, "--filter=^bmGraph", "--iterations=2"},
               &setupTwoSingles);
  check(run.status == 2,
        "a batch with no threaded run exits 2 (FR-012, SC-003)");
  const auto faults = linesOf(run.err);
  if (faults.size() != 1) {
    report(
        "the no-threaded-run refusal prints one line alone (FR-012, SC-003)",
        "one line",
        std::to_string(faults.size()) + " lines: " + namingLines(faults, ""));
  }
  check(faults.front().find("threaded run") != std::string::npos,
        "the no-threaded-run refusal names the batch it refuses (FR-012)");
  check(run.out.find("bmGraphSingle") == std::string::npos
            && run.out.find("bmGraphPair") == std::string::npos,
        "the no-threaded-run refusal runs no benchmark (FR-012, SC-003)");
  return 0;
}

// FR-012, D-2: a single-context run beside the graph takes no graph, gains no
// record entry, keeps the standing bytes and exits 0. The standing bytes are
// the row of the same benchmark run with no graph at all, folded across two
// children over the scripted provider, the row's overhead floor masked as the
// fresh host reading it is.
auto scenarioSingleContextTakesNoGraph() -> int
{
  const auto path = plant("pair", pairFileText());
  const auto with = runChild(
      "singleWith",
      {"--graph=" + path, "--filter=^bmGraphSingle$", "--iterations=2"},
      &setupSingle);
  const auto without = runChild("singleWithout",
                                {"--filter=^bmGraphSingle$", "--iterations=2"},
                                &setupSingle);
  check(with.status == 0,
        "a single-context run beside the graph exits 0 (FR-012)");
  check(without.status == 0, "the same run with no graph exits 0 (FR-012)");
  const auto graphLines = namingLines(linesOf(with.out), "graph nodes=");
  check(graphLines.empty(),
        "a single-context run gains no graph block (FR-012, FR-022)");
  const auto row = rowOf(with.out, "bmGraphSingle");
  check(!row.empty(), "the single-context run prints its row (FR-012)");
  if (floorMasked(row) != floorMasked(rowOf(without.out, "bmGraphSingle"))) {
    report("a single-context run beside the graph keeps the standing bytes "
           "(FR-012, FR-006)",
           floorMasked(rowOf(without.out, "bmGraphSingle")),
           floorMasked(row));
  }
  return 0;
}

// FR-011, SC-003: the file reads once ahead of the first context, the
// parse-once shape of the command line's own parse. A batch of two threaded
// runs over a refused file answers the one fault line, exits 2, and runs no
// benchmark: a read per run would answer the line twice.
auto scenarioGraphParsesOnceBeforeRun() -> int
{
  const auto path = plant("repeated", repeatedNameFileText());
  const auto run =
      runChild("parseOnce",
               {"--graph=" + path, "--filter=^bmGraph", "--iterations=2"},
               &setupTwoThreaded);
  check(run.status == 2,
        "a refused graph file over a batch of two threaded runs exits 2 "
        "(FR-010, FR-011, SC-003)");
  const auto faults = linesOf(run.err);
  if (faults.size() != 1) {
    report("the refused file prints its first fault alone, once for the batch "
           "(FR-010, FR-011, SC-003)",
           "one line",
           std::to_string(faults.size()) + " lines: " + namingLines(faults, ""));
  }
  check(faults.front().find("stands twice") != std::string::npos,
        "the fault line names the repeated node name the file carries "
        "(FR-010 rule 4)");
  check(run.out.find("bmGraphChain") == std::string::npos
            && run.out.find("bmGraphPair") == std::string::npos,
        "the refused file starts no context (FR-011, SC-003)");
  return 0;
}

// ---------------------------------------------------------------------------
// The settlement's words, the record and the document (T023)
// ---------------------------------------------------------------------------

// The chain of three contexts in the order of its slots: `a` at slot 0, `b`
// at slot 1 behind `a`, `c` at slot 2 behind `b`. The failing body settles
// `a` failed, and the closure of its dependents settles held with `a` named
// as each one's gater (FR-019, FR-020, FR-023).
auto heldChainFileText() -> std::string
{
  return graphText({
      NodeSpec {"a", 0, {}},
      NodeSpec {"b", 1, {"a"}},
      NodeSpec {"c", 2, {"b"}},
  });
}

// FR-022, FR-023: the graph run with a document beside it. The console block
// and the document entry carry one fact, so the run that gates its attempt
// prints the nodes line and writes the entry: the nodes in the ascending slot
// order, the edges in the file's order, and two empty arrays for a run that
// holds and stops nothing.
auto scenarioGraphRunWritesRecord() -> int
{
  const auto path = plant("recordChain", heldChainFileText());
  const auto jsonPath = capturePath("record", ".json");
  const auto run = runChild("record",
                            {"--graph=" + path,
                             "--json=" + jsonPath,
                             "--filter=^bmGraphChain",
                             "--iterations=2"},
                            &setupChainThree);
  check(run.status == 0, "the graph run beside a document exits 0 (FR-023)");
  const auto block = namingLines(linesOf(run.out), "graph nodes=");
  check(block.find("graph nodes=a,b,c edges=a->b,b->c") != std::string::npos,
        "the console block names the nodes in the ascending slot order and the "
        "edges in the file's order (FR-022, FR-023)");

  const std::string document = readFile(jsonPath);
  check(document.find("\"graph\"") != std::string::npos,
        "the document carries the graph record of the run (FR-023)");
  check(document.find("\"benchmark\":\"bmGraphChain/threads:3\"")
            != std::string::npos,
        "the entry names the run it records, the instance name the console "
        "prints (FR-023)");
  check(document.find(
            "\"nodes\":[{\"name\":\"a\",\"slot\":0,\"outcome\":\"finished\"}"
            ",{\"name\":\"b\",\"slot\":1,\"outcome\":\"finished\"},"
            "{\"name\":\"c\",\"slot\":2,\"outcome\":\"finished\"}]")
            != std::string::npos,
        "the nodes carry the word the gate settled, in the ascending slot "
        "order (FR-023)");
  const std::string_view edgesNeedle =
      "\"edges\":[{\"from\":\"a\",\"to\":\"b\"},{\"from\":\"b\",\"to\":\"c\"}]";
  check(document.find(edgesNeedle) != std::string::npos,
        "the edges carry the file's order (FR-014, FR-023)");
  check(document.find("\"held\":[]") != std::string::npos
            && document.find("\"stopped\":[]") != std::string::npos,
        "a run that holds and stops nothing carries two empty arrays (FR-023)");
  return 0;
}

// FR-019, FR-020, FR-021, FR-023: the run whose source fails. The body of
// the context at slot 0 records an error, so the gate settles that node failed
// and the transitive closure of its dependents held, and the attempt answers
// the standing run-failure status of specs/015 with a FAILED line and no
// measured row. The gate's words ride the runner's channel ahead of that
// answer, so the document carries the entry of the failed attempt: the nodes
// with their words, and the held array naming for each held node the failed
// node that gates it. The console block stands after a threaded run's
// `contexts=` line, and the failed run prints no row to stand beside, so the
// block prints for no row.
auto scenarioFailedRunCarriesHeldRecord() -> int
{
  const auto path = plant("failedChain", heldChainFileText());
  const auto jsonPath = capturePath("failedRecord", ".json");
  const auto run = runChild("failed",
                            {"--graph=" + path,
                             "--json=" + jsonPath,
                             "--filter=^bmGraphChainFail",
                             "--iterations=2"},
                            &setupFailingChainThree);
  check(run.status == 1,
        "the run whose node fails answers the standing run-failure status "
        "(FR-019, FR-021)");
  check(namingLines(linesOf(run.out), "bmGraphChainFail").find("FAILED")
            != std::string::npos,
        "the failed run prints its FAILED line (FR-019)");
  check(namingLines(linesOf(run.out), "graph nodes=").empty()
            && namingLines(linesOf(run.out), "graph held=").empty(),
        "the failed run prints no graph block: the block stands beside a row, "
        "and the failed run prints no row (FR-022)");

  const std::string document = readFile(jsonPath);
  check(document.find("\"graph\"") != std::string::npos,
        "the document carries the graph entry of the failed attempt (FR-023)");
  check(document.find(
            "\"nodes\":[{\"name\":\"a\",\"slot\":0,\"outcome\":\"failed\"}"
            ",{\"name\":\"b\",\"slot\":1,\"outcome\":\"held\"},"
            "{\"name\":\"c\",\"slot\":2,\"outcome\":\"held\"}]")
            != std::string::npos,
        "the nodes carry the words the gate settled: the source failed and the "
        "closure of its dependents held (FR-019, FR-020, FR-023)");
  check(document.find(
            "\"held\":[{\"node\":\"b\",\"reason\":\"a\"},"
            "{\"node\":\"c\",\"reason\":\"a\"}]")
            != std::string::npos,
        "the held array names for each held node the failed node that gates it "
        "(FR-020, FR-023)");
  check(document.find("\"stopped\":[]") != std::string::npos,
        "a run no stop request settles carries an empty stopped array "
        "(FR-023)");
  return 0;
}

// The graph the record-entry arm assembles a settlement over: `a` at slot 0,
// `b` and `c` behind it, and `d` standing on its own (FR-023).
[[nodiscard]] auto entryGraph() -> sg::detail::Graph
{
  auto graph = sg::detail::Graph {};
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "a", .slot = 0, .deps = {}});
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "b", .slot = 1, .deps = {"a"}});
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "c", .slot = 2, .deps = {"a"}});
  graph.nodes.push_back(
      sg::detail::GraphNode {.name = "d", .slot = 3, .deps = {}});
  graph.edges.push_back(sg::detail::GraphEdge {.from = "a", .to = "b"});
  graph.edges.push_back(sg::detail::GraphEdge {.from = "a", .to = "c"});
  return graph;
}

// FR-022, FR-023, FR-024: the two arms of the assembly. The entry point
// builds the entry from the words the runner read beside the merged row, and
// the stop request of specs/020 FR-050 settles a node stopped only on a run
// that stops, which no scripted run of this file reaches. The arm stands
// driven at the assembly, where the words enter: a held node joins the held
// array with the node that gates it, a stopped node joins the stopped array
// with no reason, and a finished node joins neither.
auto scenarioRecordEntryCarriesHeldAndStopped() -> int
{
  const auto graph = entryGraph();
  const auto settlement = std::vector<sg::detail::GraphSettlement> {
      sg::detail::GraphSettlement {
          .name = "a",
          .slot = 0,
          .outcome = sg::detail::NodeOutcome::FAILED,
          .reason = {},
      },
      sg::detail::GraphSettlement {
          .name = "b",
          .slot = 1,
          .outcome = sg::detail::NodeOutcome::HELD,
          .reason = "a",
      },
      sg::detail::GraphSettlement {
          .name = "c",
          .slot = 2,
          .outcome = sg::detail::NodeOutcome::STOPPED,
          .reason = {},
      },
      sg::detail::GraphSettlement {
          .name = "d",
          .slot = 3,
          .outcome = sg::detail::NodeOutcome::FINISHED,
          .reason = {},
      },
  };

  const auto entry = sg::graphRunEntryOf("bmGraphChain", graph, settlement);
  check(entry.has_value(),
        "a settlement of words assembles one entry (FR-023)");
  if (!entry.has_value()) {
    return 1;
  }
  check(entry->benchmark == "bmGraphChain",
        "the entry names the run it records (FR-023)");
  check(entry->nodes.size() == 4 && entry->nodes[0].outcome == "failed"
            && entry->nodes[1].outcome == "held"
            && entry->nodes[2].outcome == "stopped"
            && entry->nodes[3].outcome == "finished",
        "the nodes carry the four words in the settlement's order (FR-023)");
  check(entry->held.size() == 1 && entry->held[0].node == "b"
            && entry->held[0].reason == "a",
        "the held array carries the held node with the node that gates it "
        "(FR-023)");
  check(entry->stopped.size() == 1 && entry->stopped[0] == "c",
        "the stopped array carries the stopped node's name (FR-023)");
  check(entry->edges.size() == 2 && entry->edges[0].from == "a"
            && entry->edges[0].to == "b" && entry->edges[1].from == "a"
            && entry->edges[1].to == "c",
        "the edges carry the graph's own order (FR-014, FR-023)");

  // A run that gated no attempt settles no word and gains no entry: the
  // single-context run of FR-012 among them.
  check(!sg::graphRunEntryOf("bmGraphChain", graph, {}).has_value(),
        "an empty settlement assembles no entry (FR-023, FR-012)");
  return 0;
}

// ---------------------------------------------------------------------------
// The row's sources, twice settled (T020)
// ---------------------------------------------------------------------------

// FR-035: the last word of one source stands. Two `--graph` words on one
// command line settle the row twice within one source, and the run reads the
// second file.
auto scenarioGraphTwiceOnOneLineKeepsLast() -> int
{
  const auto first = plant("twiceFirst", pairFileText());
  const auto last = plant("twiceLast", chainFileText());
  const auto run = runChild("twice",
                            {"--graph=" + first,
                             "--graph=" + last,
                             "--filter=^bmGraphChain",
                             "--iterations=2"},
                            &setupChainThree);
  check(run.status == 0,
        "the run of two graph words on one command line exits 0 (FR-035)");
  const auto order = orderOf(run.out);
  if (order != "2,1,0") {
    report("the last graph word of one source orders the run (FR-035, FR-009)",
           "2,1,0",
           order);
  }
  return 0;
}

// FR-037, FR-038: a higher source settled the row with a path of its own, so
// the file's line sets nothing and discloses nothing. The command line names
// the chain and the option file names the pair: the run reads the command
// line's file, and the file's line discloses no use.
auto scenarioGraphOptionFileLineDrops() -> int
{
  const auto commandLinePath = plant("dropChain", chainFileText());
  const auto filePath = plant("dropPair", pairFileText());
  const auto optionFile = plant("drop.opts", "graph=" + filePath + "\n");
  const auto run = runChild("drop",
                            {"--graph=" + commandLinePath,
                             "--options=" + optionFile,
                             "--filter=^bmGraphChain",
                             "--iterations=2"},
                            &setupChainThree);
  check(run.status == 0,
        "the command line's path beside a file line exits 0 (FR-037, FR-009)");
  const auto order = orderOf(run.out);
  if (order != "2,1,0") {
    report("the command line's path orders the run, and the file's line sets "
           "nothing (FR-037, FR-038)",
           "2,1,0",
           order);
  }
  const auto disclosed = namingLines(
      linesOf(run.out),
      "options from a file or the environment: --graph=" + filePath);
  check(disclosed.empty(),
        "the dropped file line discloses no use of the row (FR-040, FR-038)");
  return 0;
}

// FR-009, SC-003: the bare row beside a file line that names no path. The
// command line's bare word leaves the mark empty, the file's empty word
// fills nothing, and the run answers the empty-path refusal with one line
// before any context starts.
auto scenarioBareGraphEmptyOptionFileRefuses() -> int
{
  const auto optionFile = plant("empty.opts", "graph=\n");
  const auto run = runChild("bareEmpty",
                            {"--options=" + optionFile,
                             "--graph",
                             "--filter=^bmGraphChain",
                             "--iterations=2"},
                            &setupChainThree);
  check(run.status == 2,
        "the bare row beside a file line of no path exits 2 (FR-009, SC-003)");
  const auto faults = linesOf(run.err);
  if (faults.size() != 1) {
    report(
        "the empty-path refusal prints one line alone (FR-009, SC-003)",
        "one line",
        std::to_string(faults.size()) + " lines: " + namingLines(faults, ""));
  }
  check(faults.front().find("--graph") != std::string::npos
            && faults.front().find("empty") != std::string::npos,
        "the refusal names the row and the missing path (FR-009)");
  check(run.out.find("bmGraphChain") == std::string::npos,
        "the refusal runs no benchmark (FR-009, SC-003)");
  return 0;
}

// ---------------------------------------------------------------------------
// The scenario list
// ---------------------------------------------------------------------------

}  // namespace

auto main(int argc, char** argv) -> int
{
  struct Scenario
  {
    const char* name;
    int (*run)();
  };

  const std::array<Scenario, 14> scenarios {
      Scenario {"graphRowPrintsHelpAndOptions",
                &scenarioRowPrintsHelpAndOptions},
      Scenario {"graphEqualsFileReads", &scenarioEqualsFileReads},
      Scenario {"bareGraphTakesOptionFilePath",
                &scenarioBareGraphTakesOptionFilePath},
      Scenario {"bareGraphNoPathRefuses", &scenarioBareGraphNoPathRefuses},
      Scenario {"countMismatchRefuses", &scenarioCountMismatchRefuses},
      Scenario {"noThreadedRunRefuses", &scenarioNoThreadedRunRefuses},
      Scenario {"singleContextTakesNoGraph",
                &scenarioSingleContextTakesNoGraph},
      Scenario {"graphParsesOnceBeforeRun", &scenarioGraphParsesOnceBeforeRun},
      Scenario {"graphRunWritesRecord", &scenarioGraphRunWritesRecord},
      Scenario {"failedRunCarriesHeldRecord",
                &scenarioFailedRunCarriesHeldRecord},
      Scenario {"recordEntryCarriesHeldAndStopped",
                &scenarioRecordEntryCarriesHeldAndStopped},
      Scenario {"graphTwiceOnOneLineKeepsLast",
                &scenarioGraphTwiceOnOneLineKeepsLast},
      Scenario {"graphOptionFileLineDrops", &scenarioGraphOptionFileLineDrops},
      Scenario {"bareGraphEmptyOptionFileRefuses",
                &scenarioBareGraphEmptyOptionFileRefuses},
  };

  // `argv[1]` names one scenario, so a red arm answers its own line and the
  // list at the bottom answers the whole file, the shape
  // harness_graph_gate_test.cpp gives its scenarios.
  if (argc > 1) {
    for (const auto& scenario : scenarios) {
      if (std::string_view(argv[1]) == scenario.name) {
        check(scenario.run() == 0, scenario.name);
        std::printf("harness_cli_graph_test: %s passed\n", scenario.name);
        return 0;
      }
    }
    std::fprintf(stderr, "no scenario named %s\n", argv[1]);
    return 1;
  }

  for (const auto& scenario : scenarios) {
    check(scenario.run() == 0, scenario.name);
  }

  std::printf("harness_cli_graph_test: all scenarios passed\n");
  return 0;
}
