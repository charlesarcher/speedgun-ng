// ============================================================================
// specs/037-execution-graphs module M5 `record-disclosure`, wave 3: the graph
// console block, the record's keys, order and read-back, the schema arms, the
// byte identity and the ABI fact (T024 to T031; US2; FR-004, FR-005, FR-006,
// FR-022, FR-023, FR-024, FR-028; SC-001, SC-002, SC-005, SC-006; R-10, R-11,
// R-13; D-5, D-6 of research.md).
//
// The console scenarios print through `detail::printResult` in this process,
// the shape `harness_context_report_test.cpp` gives the standing disclosure
// line: the scenario builds the result of a threaded run, hands that run's
// record entry beside it, and reads the captured bytes. No benchmark runs, so
// no provider registers, no thread starts and no clock reads. The block
// follows R-10 of research.md, with the two-space indent of the standing
// disclosure:
//
//   graph nodes=a,b,c edges=a->b,b->c
//   graph held=b(a)
//   graph stopped=d
//
// The nodes line names the nodes in the ascending slot order FR-023 fixes and
// the edges as `from->to` pairs in the file's order; the held line stands only
// when a node stands held and names each held node with the failed node that
// gates it in parentheses; the stopped line stands only when a node stands
// stopped. The timed rows keep the bytes a graph-free run of the same
// benchmark gives (FR-006, FR-022, FR-023, SC-001).
//
// Scenario map:
//   consoleBlockBesideContextsLine
//                            the block stands directly after the run's
//                            `contexts=` line, in the one printed order of
//                            FR-023, with the held line, the stopped line, and
//                            the timed rows at a graph-free run's bytes
//                            (FR-022, FR-023, FR-006, SC-001, R-10)
//   blockOncePerRun          a batch of two threaded runs prints one block per
//                            run, each after its own run's `contexts=` line
//                            (FR-022, SC-001, SC-006, R-10, D-6)
//   recordKeysOrderAndReadBack
//                            the top-level record stands after `precheck` and
//                            before `benchmarks`, the entry carries
//                            `benchmark`, `nodes`, `edges`, `held` and
//                            `stopped` with their `x-order`, and `loadReport`
//                            reads the record back to fold against the run's
//                            facts (FR-023, FR-024, SC-002, SC-006, R-11, D-5)
//   nodeOrderAscendingSlots  the record's node array names the nodes in
//                            ascending slot order and the console prints that
//                            order (FR-023, SC-002)
//   reasonNamesFailedNode    the reason names a failed node, and a stopped
//                            node carries none (FR-023, FR-019, FR-020)
//   recordCarriesNoTiming    the record carries the contract's keys and no
//                            timing figure (FR-024)
//   schemaCarriesGraphKeysAndOrder
//                            the schema carries the record's keys, the entry
//                            and sub-object `x-order` lists and the moved
//                            top-level `x-order`, and the written document
//                            stamps the minor the version commit records
//                            (FR-005, FR-023, FR-028, SC-006, R-11, R-13)
//   plantedGraphDocumentsFailTheSchema
//                            a missing key, a key of the wrong kind, a
//                            duplicate key and an order the `x-order` lists
//                            state never each fail the standing schema check,
//                            and the refusal names the record's position
//                            (FR-005, FR-023, FR-028, SC-006)
//   noGraphBytesMatchMaster  a run with no graph writes master's document and
//                            prints master's console, the `schemaVersion`
//                            value and the `speedgun.version` value standing
//                            the only differences (FR-006, SC-005, R-11)
//   reportDataGraphMemberRaisesAbi
//                            `ReportData::graph` stands in the public
//                            loaded-report struct with the record's type, the
//                            growth that moves the harness `SOVERSION`, and a
//                            document with no record leaves it empty (FR-004,
//                            FR-023, D-8, R-13)
//
// Frameworkless check()/fail() convention. One process per scenario: the
// parent runs this binary once per name below and reads each child's status.
// ============================================================================

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "detail/internal.hpp"
#include "detail/json_fixture.hpp"
#include "detail/json_report.hpp"
#include "speedgun-ng/benchmark.hpp"

#ifndef SG_SCHEMA_PATH
#  error "the test target states SG_SCHEMA_PATH"
#endif

namespace
{

// The scratch file the captured print lands in, named for this test so a
// suite run in parallel holds no file in common with it.
constexpr auto kScratchFile = "harness_graph_report_out.txt";

// The prefix every line of the block carries, the two-space indent of the
// standing `contexts=` disclosure line (R-10).
constexpr std::string_view kBlockPrefix = "  graph ";
constexpr std::string_view kNodesPrefix = "  graph nodes=";
constexpr std::string_view kHeldPrefix = "  graph held=";
constexpr std::string_view kStoppedPrefix = "  graph stopped=";
constexpr std::string_view kContextsPrefix = "  contexts=";

[[noreturn]] auto fail(const char* const what) -> void
{
  std::fprintf(stderr, "HARNESS GRAPH REPORT TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool condition, const char* const what) -> void
{
  if (!condition) {
    fail(what);
  }
}

// A check whose message carries the text that answered: the printed line and
// the line the contract owes, so a red run states the difference. The call
// returns when the condition holds and ends the process when it does not.
auto checked(const bool condition, const std::string& what) -> void
{
  if (!condition) {
    std::fprintf(stderr, "HARNESS GRAPH REPORT TEST FAIL: %s\n", what.c_str());
    std::exit(1);
  }
}

// ---------------------------------------------------------------------------
// The result of a threaded run
// ---------------------------------------------------------------------------

// One measured row and one context disclosure per context, the shape
// `harness_context_report_test.cpp` builds for the disclosure line: the row
// gives the block something to stand behind, and the contexts give it the
// `contexts=` line it stands after. A `contexts` count of zero names a
// single-context run, the run that has no threaded context to order.
auto threadedResult(const std::string& name,
                    const std::size_t contexts) -> sg::BenchmarkResult
{
  auto result = sg::BenchmarkResult {};
  result.name = name;

  auto row = sg::ResultRow {};
  row.iterations = 1000;
  row.timePerIterationNs = 250.0;
  row.cpuTimePerIterationNs = 200.0;
  row.endpointSamples = 2;
  result.rows.push_back(row);

  for (std::size_t index = 0; index < contexts; ++index) {
    result.contexts.push_back(
        sg::ContextDisclosure {.waitMode = sg::WaitMode::SPIN,
                               .placement = "NO_PIN",
                               .applied = false});
  }
  return result;
}

// ---------------------------------------------------------------------------
// The record entries the console prints
// ---------------------------------------------------------------------------

// One node's settlement as the record carries it: the name, the slot and the
// word, the three keys of a `nodes` entry (FR-023).
auto settled(const char* const name,
             const std::uint32_t slot,
             const char* const outcome) -> sg::GraphNodeRecord
{
  return sg::GraphNodeRecord {
      .name = name, .slot = slot, .outcome = std::string(outcome)};
}

// The chain `top -> mid -> end` over the slots 0, 1 and 2, every node
// finished. The names stand in an order the alphabetical order of names does
// not answer, so the printed list states the ascending slot order of FR-023
// and not a sort of names.
auto chainEntry() -> sg::GraphRunRecord
{
  auto entry = sg::GraphRunRecord {};
  entry.benchmark = "bmGraphConsole";
  entry.nodes = {
      settled("top", 0, "finished"),
      settled("mid", 1, "finished"),
      settled("end", 2, "finished"),
  };
  entry.edges = {
      sg::GraphEdgeRecord {.from = "top", .to = "mid"},
      sg::GraphEdgeRecord {.from = "mid", .to = "end"},
  };
  return entry;
}

// The same chain with its first node failed: the failed node settles failed,
// and the transitive closure of its dependents settles held, each named with
// the failed node that gates it (FR-019, FR-020).
auto heldEntry() -> sg::GraphRunRecord
{
  auto entry = chainEntry();
  entry.nodes[0].outcome = "failed";
  entry.nodes[1].outcome = "held";
  entry.nodes[2].outcome = "held";
  entry.held = {
      sg::GraphHeldRecord {.node = "mid", .reason = "top"},
      sg::GraphHeldRecord {.node = "end", .reason = "top"},
  };
  return entry;
}

// The same chain with the stop request arriving at the second node: that node
// settles stopped and its dependent settles stopped with it, each carrying no
// reason (FR-019, FR-023).
auto stoppedEntry() -> sg::GraphRunRecord
{
  auto entry = chainEntry();
  entry.nodes[1].outcome = "stopped";
  entry.nodes[2].outcome = "stopped";
  entry.stopped = {"mid", "end"};
  return entry;
}

// The diamond of SC-002 over four slots, every node finished, the edges
// standing in a file order the sorted order of pairs does not answer: the
// record and the console print the file's order (FR-014, FR-023).
auto diamondEntry() -> sg::GraphRunRecord
{
  auto entry = sg::GraphRunRecord {};
  entry.benchmark = "bmGraphConsole";
  entry.nodes = {
      settled("a", 0, "finished"),
      settled("b", 1, "finished"),
      settled("c", 2, "finished"),
      settled("d", 3, "finished"),
  };
  entry.edges = {
      sg::GraphEdgeRecord {.from = "a", .to = "c"},
      sg::GraphEdgeRecord {.from = "a", .to = "b"},
      sg::GraphEdgeRecord {.from = "c", .to = "d"},
      sg::GraphEdgeRecord {.from = "b", .to = "d"},
  };
  return entry;
}

// The two-node chain of the batch's second run, the shape that names its own
// run's nodes apart from the first run's (SC-006).
auto pairEntry() -> sg::GraphRunRecord
{
  auto entry = sg::GraphRunRecord {};
  entry.benchmark = "bmGraphSecond";
  entry.nodes = {
      settled("left", 0, "finished"),
      settled("right", 1, "finished"),
  };
  entry.edges = {sg::GraphEdgeRecord {.from = "left", .to = "right"}};
  return entry;
}

// ---------------------------------------------------------------------------
// The captured print
// ---------------------------------------------------------------------------

// The print of `body`, captured to the end of the print, the shape
// `harness_context_report_test.cpp` gives the disclosure line.
auto captureThrough(const std::function<void()>& body)
    -> std::vector<std::string>
{
  const int saved = dup(STDOUT_FILENO);
  if (saved < 0 || std::freopen(kScratchFile, "w", stdout) == nullptr) {
    fail("cannot redirect the report");
  }
  body();
  std::fflush(stdout);
  if (dup2(saved, STDOUT_FILENO) < 0) {
    fail("cannot restore stdout");
  }
  close(saved);

  auto file = std::ifstream(kScratchFile);
  auto buffer = std::stringstream {};
  buffer << file.rdbuf();
  auto lines = std::vector<std::string> {};
  auto line = std::string {};
  while (std::getline(buffer, line)) {
    lines.push_back(line);
  }
  return lines;
}

// One run of a batch: the result beside the record entry of that run, the
// pair `printResult` takes (R-10, D-6).
using Run = std::pair<sg::BenchmarkResult, std::optional<sg::GraphRunRecord>>;

auto printCaptured(const std::vector<Run>& runs) -> std::vector<std::string>
{
  return captureThrough(
      [&runs]
      {
        for (const auto& [result, entry] : runs) {
          sg::detail::printResult(result, std::nullopt, entry);
        }
      });
}

// The index of the first line that starts with `prefix`, and `npos` where no
// line does.
auto indexOf(const std::vector<std::string>& lines,
             std::string_view prefix) -> std::size_t
{
  for (std::size_t index = 0; index < lines.size(); ++index) {
    if (lines[index].starts_with(prefix)) {
      return index;
    }
  }
  return std::string::npos;
}

auto countWith(const std::vector<std::string>& lines,
               std::string_view prefix) -> std::size_t
{
  auto count = std::size_t {0};
  for (const auto& line : lines) {
    if (line.starts_with(prefix)) {
      ++count;
    }
  }
  return count;
}

// The print with every block line held out: the bytes the run printed on its
// own, the side of the fold FR-006 and R-10 state.
auto withoutBlock(const std::vector<std::string>& lines)
    -> std::vector<std::string>
{
  auto kept = std::vector<std::string> {};
  for (const auto& line : lines) {
    if (!line.starts_with(kBlockPrefix)) {
      kept.push_back(line);
    }
  }
  return kept;
}

auto joined(const std::vector<std::string>& lines) -> std::string
{
  auto text = std::string {};
  for (const auto& line : lines) {
    text += line + "\n";
  }
  return text;
}

// The line at `index`, or the text that states its absence: the message of a
// red fold reads the line the print answered.
auto lineAt(const std::vector<std::string>& lines,
            const std::size_t index) -> std::string
{
  return index < lines.size() ? lines[index] : std::string("<absent>");
}

// ---------------------------------------------------------------------------
// T024: the console block
// ---------------------------------------------------------------------------

// FR-022, FR-023, FR-006, SC-001, R-10: the block stands directly after the
// run's `contexts=` line and prints the one order FR-023 fixes: the nodes in
// ascending slot order, the edges as `from->to` pairs in file order, the held
// line naming each held node with its gating failed node in parentheses in
// ascending slot order, and the stopped line naming each stopped node in
// ascending slot order. The held line stands for no run that holds no node and
// the stopped line for no run that stops none. The timed rows keep the bytes a
// graph-free run of the same benchmark gives, and a run with no graph prints
// no block at all.
auto consoleBlockBesideContextsLine() -> void
{
  const auto result = threadedResult("bmGraphConsole", 3);

  const auto plain = printCaptured({Run {result, std::nullopt}});
  check(indexOf(plain, kContextsPrefix) != std::string::npos,
        "a threaded run prints its contexts= line (FR-022)");
  check(countWith(plain, kBlockPrefix) == 0,
        "a run with no graph prints no block (FR-022)");

  const auto block = printCaptured({Run {result, chainEntry()}});
  const auto at = indexOf(block, kContextsPrefix);
  check(at != std::string::npos, "the graph run prints its contexts= line");
  checked(block.at(at + 1) == "  graph nodes=top,mid,end edges=top->mid,mid->end",
          "the nodes line stands directly after the contexts= line, the nodes "
          "in ascending slot order and the edges in file order, got '"
              + lineAt(block, at + 1) + "'");
  check(countWith(block, kHeldPrefix) == 0,
        "a run that holds no node prints no held line (FR-022)");
  check(countWith(block, kStoppedPrefix) == 0,
        "a run that stops no node prints no stopped line (FR-023)");
  check(countWith(block, kNodesPrefix) == 1,
        "one threaded run prints one nodes line (FR-022)");
  checked(withoutBlock(block) == plain,
          "the timed rows keep the bytes a graph-free run gives, got '"
              + joined(withoutBlock(block)) + "' want '" + joined(plain) + "'");

  const auto held = printCaptured({Run {result, heldEntry()}});
  const auto heldAt = indexOf(held, kContextsPrefix);
  checked(held.at(heldAt + 2) == "  graph held=mid(top),end(top)",
          "the held line names each held node with its gating failed node in "
          "parentheses in ascending slot order, got '" + lineAt(held, heldAt + 2)
              + "'");
  check(countWith(held, kStoppedPrefix) == 0,
        "a run that stops no node prints no stopped line (FR-023)");
  check(countWith(held, kHeldPrefix) == 1,
        "the held nodes stand on one line (FR-022, R-10)");
  checked(withoutBlock(held) == plain,
          "the held arm keeps the timed rows of a graph-free run, got '"
              + joined(withoutBlock(held)) + "' want '" + joined(plain) + "'");

  const auto stopped = printCaptured({Run {result, stoppedEntry()}});
  const auto stoppedAt = indexOf(stopped, kContextsPrefix);
  checked(stopped.at(stoppedAt + 2) == "  graph stopped=mid,end",
          "the stopped line names each stopped node in ascending slot order "
          "and carries no reason, got '" + lineAt(stopped, stoppedAt + 2) + "'");
  check(countWith(stopped, kHeldPrefix) == 0,
        "a run that holds no node prints no held line (FR-022)");
  checked(withoutBlock(stopped) == plain,
          "the stopped arm keeps the timed rows of a graph-free run, got '"
              + joined(withoutBlock(stopped)) + "' want '" + joined(plain)
              + "'");

  const auto diamond = printCaptured(
      {Run {threadedResult("bmGraphDiamond", 4), diamondEntry()}});
  const auto diamondAt = indexOf(diamond, kContextsPrefix);
  checked(diamond.at(diamondAt + 1)
              == "  graph nodes=a,b,c,d edges=a->c,a->b,c->d,b->d",
          "the edges print as from->to pairs in the file's order, got '"
              + lineAt(diamond, diamondAt + 1) + "'");

  const auto single =
      printCaptured({Run {threadedResult("bmGraphSingle", 0), chainEntry()}});
  check(indexOf(single, kContextsPrefix) == std::string::npos,
        "a single-context run prints no contexts= line (FR-022)");
  check(countWith(single, kBlockPrefix) == 0,
        "a single-context run prints no block (FR-022)");
}

// FR-022, SC-001, SC-006, R-10, D-6: a batch of two threaded runs prints one
// block per run, each directly after its own run's `contexts=` line and each
// naming the nodes of its own run.
auto blockOncePerRun() -> void
{
  const auto printed =
      printCaptured({Run {threadedResult("bmGraphFirst", 3), chainEntry()},
                     Run {threadedResult("bmGraphSecond", 2), pairEntry()}});

  check(countWith(printed, kContextsPrefix) == 2,
        "the batch prints one contexts= line per threaded run (FR-022)");
  check(countWith(printed, kNodesPrefix) == 2,
        "the batch prints one block per threaded run (FR-022)");

  auto contextsAt = std::vector<std::size_t> {};
  for (std::size_t index = 0; index < printed.size(); ++index) {
    if (printed[index].starts_with(kContextsPrefix)) {
      contextsAt.push_back(index);
    }
  }
  check(contextsAt.size() == 2, "the batch names both runs' disclosures");
  checked(lineAt(printed, contextsAt[0] + 1)
              == "  graph nodes=top,mid,end edges=top->mid,mid->end",
          "the first run's block stands directly after its own contexts= line, "
          "got '" + lineAt(printed, contextsAt[0] + 1) + "'");
  checked(lineAt(printed, contextsAt[1] + 1)
              == "  graph nodes=left,right edges=left->right",
          "the second run's block stands directly after its own contexts= "
          "line and names its own nodes, got '"
              + lineAt(printed, contextsAt[1] + 1) + "'");
}

// ---------------------------------------------------------------------------
// The written document, and the text the record folds against
// ---------------------------------------------------------------------------

// The scratch document the record scenarios write, named for this test so a
// suite run in parallel holds no file in common with it.
constexpr auto kGraphDocument = "harness_graph_report_record.json";

// The document one graph run writes: the record beside a report that carries
// no benchmark, the shape `harness_noise_report_test.cpp` gives the pre-check
// record so the graph object stands whole and readable. The pre-check record
// stands beside it because FR-023 fixes the graph's place after `precheck`
// and before `benchmarks`, and the fold reads that place.
auto graphDocument(const std::string& path,
                   const std::optional<sg::GraphRecord>& graph) -> std::string
{
  auto data = sg::ReportData {};
  auto precheck = sg::PrecheckRecord {};
  precheck.windowMs = 7;
  checked(
      sg::detail::writeReport(path, data, {}, std::nullopt, precheck, graph),
      "the writer wrote the document (FR-023)");
  return sg::test::readText(path);
}

// The balanced text that opens at or after `at`: an object from its `{` to its
// matching `}`, an array from its `[` to its matching `]`. The walk respects
// strings, so a brace inside a text does not move the balance.
auto balancedFrom(const std::string& text, const std::size_t at) -> std::string
{
  const auto open = text.find_first_of("{[", at);
  checked(open != std::string::npos,
          "the document carries the object the fold reads (FR-023)");
  auto depth = std::size_t {0};
  auto inString = false;
  auto escaped = false;
  for (auto index = open; index < text.size(); ++index) {
    const char c = text[index];
    if (inString) {
      if (escaped) {
        escaped = false;
      } else if (c == '\\') {
        escaped = true;
      } else if (c == '"') {
        inString = false;
      }
      continue;
    }
    if (c == '"') {
      inString = true;
    } else if (c == '{' || c == '[') {
      ++depth;
    } else if (c == '}' || c == ']') {
      --depth;
      if (depth == 0) {
        return text.substr(open, index - open + 1);
      }
    }
  }
  fail("the document's object never closes");
}

// The keys of one object, in the order they stand, at one depth: the order
// the `x-order` lists state, read from the written bytes. A `want` of zero
// takes the keys of every object the text holds, at every depth.
auto objectKeys(std::string_view object,
                const std::size_t want = 1) -> std::vector<std::string>
{
  auto keys = std::vector<std::string> {};
  auto depth = std::size_t {0};
  auto inString = false;
  auto escaped = false;
  auto textStart = std::size_t {0};
  for (auto index = std::size_t {0}; index < object.size(); ++index) {
    const char c = object[index];
    if (inString) {
      if (escaped) {
        escaped = false;
        continue;
      }
      if (c == '\\') {
        escaped = true;
        continue;
      }
      if (c != '"') {
        continue;
      }
      inString = false;
      auto next = index + 1;
      while (next < object.size()
             && std::isspace(static_cast<unsigned char>(object[next])))
      {
        ++next;
      }
      const bool atWanted = want == 0 || depth == want;
      if (atWanted && next < object.size() && object[next] == ':') {
        keys.push_back(
            std::string(object.substr(textStart + 1, index - textStart - 1)));
      }
      continue;
    }
    if (c == '"') {
      inString = true;
      textStart = index;
    } else if (c == '{' || c == '[') {
      ++depth;
    } else if (c == '}' || c == ']') {
      --depth;
    }
  }
  return keys;
}

// The objects one array text holds, in order: the entries of the `runs` array
// and of the `nodes`, `edges` and `held` arrays.
auto objectsIn(std::string_view array) -> std::vector<std::string>
{
  auto entries = std::vector<std::string> {};
  auto depth = std::size_t {0};
  auto inString = false;
  auto escaped = false;
  auto start = std::string::npos;
  for (auto index = std::size_t {0}; index < array.size(); ++index) {
    const char c = array[index];
    if (inString) {
      if (escaped) {
        escaped = false;
      } else if (c == '\\') {
        escaped = true;
      } else if (c == '"') {
        inString = false;
      }
      continue;
    }
    if (c == '"') {
      inString = true;
    } else if (c == '{' || c == '[') {
      ++depth;
      if (c == '{' && depth == 2) {
        start = index;
      }
    } else if (c == '}' || c == ']') {
      if (c == '}' && depth == 2 && start != std::string::npos) {
        entries.push_back(std::string(array.substr(start, index - start + 1)));
        start = std::string::npos;
      }
      --depth;
    }
  }
  return entries;
}

// The keys of every object of one text, in sorted order and once each: the
// whole key set the record carries, the set FR-024 keeps free of a figure.
auto allKeys(std::string_view text) -> std::vector<std::string>
{
  auto keys = objectKeys(text, 0);
  std::ranges::sort(keys);
  // The erase range is a subrange of `keys` itself: the erase-unique idiom,
  // one container under both iterators.
  // cppcheck-suppress mismatchingContainerExpression
  keys.erase(std::ranges::unique(keys).begin(), keys.end());
  return keys;
}

// The value of one integer key of one object, in the order the keys stand:
// the `slot` figures of a `nodes` array, read in written order.
auto numberValues(std::string_view object,
                  std::string_view key) -> std::vector<std::int64_t>
{
  auto values = std::vector<std::int64_t> {};
  const auto needle = std::string("\"") + std::string(key) + "\":";
  auto at = object.find(needle);
  while (at != std::string_view::npos) {
    auto index = at + needle.size();
    auto digits = std::string {};
    while (index < object.size()
           && std::isdigit(static_cast<unsigned char>(object[index])))
    {
      digits.push_back(object[index]);
      ++index;
    }
    checked(!digits.empty(), "the key carries an integer value (FR-023)");
    values.push_back(std::stoll(digits));
    at = object.find(needle, index);
  }
  return values;
}

// The record of a batch of two threaded runs: the diamond of SC-002 and the
// two-node chain, in the batch's order (FR-023, SC-006).
auto twoRunRecord() -> sg::GraphRecord
{
  auto record = sg::GraphRecord {};
  auto diamond = diamondEntry();
  diamond.benchmark = "bmGraphDiamond";
  record.runs.push_back(diamond);
  record.runs.push_back(pairEntry());
  return record;
}

// The run of two failures, two held nodes and two stopped nodes: each held
// node names the failed node that gates it, and each stopped node carries no
// reason (FR-019, FR-020, FR-023).
auto settlementRecord() -> sg::GraphRecord
{
  auto entry = sg::GraphRunRecord {};
  entry.benchmark = "bmGraphSettled";
  entry.nodes = {
      settled("a", 0, "failed"),
      settled("b", 1, "failed"),
      settled("c", 2, "held"),
      settled("d", 3, "held"),
      settled("e", 4, "stopped"),
      settled("f", 5, "stopped"),
  };
  entry.edges = {
      sg::GraphEdgeRecord {.from = "a", .to = "c"},
      sg::GraphEdgeRecord {.from = "b", .to = "d"},
      sg::GraphEdgeRecord {.from = "e", .to = "f"},
  };
  entry.held = {
      sg::GraphHeldRecord {.node = "c", .reason = "a"},
      sg::GraphHeldRecord {.node = "d", .reason = "b"},
  };
  entry.stopped = {"e", "f"};
  auto record = sg::GraphRecord {};
  record.runs.push_back(entry);
  return record;
}

// The record the run wrote, folded against the record the reader read: every
// key of the contract, in the order the arrays carry it (FR-023, SC-006).
auto foldRuns(const sg::GraphRunRecord& read,
              const sg::GraphRunRecord& wrote) -> void
{
  checked(read.benchmark == wrote.benchmark,
          "the entry names its run by the benchmark name, read '"
              + read.benchmark + "' wrote '" + wrote.benchmark + "'");
  checked(read.nodes.size() == wrote.nodes.size(),
          "the node array carries one entry per node of the run");
  for (std::size_t index = 0; index < wrote.nodes.size(); ++index) {
    const auto& got =
        index < read.nodes.size() ? read.nodes[index] : wrote.nodes[index];
    const auto& want = wrote.nodes[index];
    checked(got.name == want.name && got.slot == want.slot
                && got.outcome == want.outcome,
            "the node entry carries its name, slot and outcome at index "
                + std::to_string(index) + ", read '" + got.name + "/"
                + std::to_string(got.slot) + "/" + got.outcome + "' wrote '"
                + want.name + "/" + std::to_string(want.slot) + "/"
                + want.outcome + "'");
  }
  checked(read.edges.size() == wrote.edges.size(),
          "the edge array carries one entry per dependency");
  for (std::size_t index = 0; index < wrote.edges.size(); ++index) {
    const auto& got =
        index < read.edges.size() ? read.edges[index] : wrote.edges[index];
    const auto& want = wrote.edges[index];
    checked(got.from == want.from && got.to == want.to,
            "the edge entry carries its pair at index " + std::to_string(index)
                + ", read '" + got.from + "->" + got.to + "' wrote '"
                + want.from + "->" + want.to + "'");
  }
  checked(read.held.size() == wrote.held.size(),
          "the held array carries one entry per held node");
  for (std::size_t index = 0; index < wrote.held.size(); ++index) {
    const auto& got =
        index < read.held.size() ? read.held[index] : wrote.held[index];
    const auto& want = wrote.held[index];
    checked(got.node == want.node && got.reason == want.reason,
            "the held entry carries its node and its reason at index "
                + std::to_string(index) + ", read '" + got.node + "("
                + got.reason + ")' wrote '" + want.node + "(" + want.reason
                + ")'");
  }
  checked(read.stopped == wrote.stopped,
          "the stopped array carries the stopped names, in ascending slot "
          "order and with no reason");
}

auto foldRecord(const sg::GraphRecord& read,
                const sg::GraphRecord& wrote) -> void
{
  checked(read.runs.size() == wrote.runs.size(),
          "the record carries one entry per threaded run in batch order, read "
              + std::to_string(read.runs.size()) + " wrote "
              + std::to_string(wrote.runs.size()));
  for (std::size_t index = 0; index < wrote.runs.size(); ++index) {
    const auto& got =
        index < read.runs.size() ? read.runs[index] : wrote.runs[index];
    foldRuns(got, wrote.runs[index]);
  }
}

// ---------------------------------------------------------------------------
// T025: the record's keys, order and read-back
// ---------------------------------------------------------------------------

// FR-023, FR-024, SC-002, SC-006, R-11, D-5: the top-level `graph` record
// stands after `precheck` and before `benchmarks`; the entry carries
// `benchmark`, `nodes`, `edges`, `held` and `stopped` with their `x-order`;
// the sub-objects carry theirs; and `loadReport` reads the whole record back
// into `ReportData::graph`, folding equal against the run's facts.
auto recordKeysOrderAndReadBack() -> void
{
  const auto record = twoRunRecord();
  const auto text = graphDocument(kGraphDocument, record);

  const auto top = objectKeys(text);
  checked(top == std::vector<std::string> {"schemaVersion",
                                           "speedgun",
                                           "precheck",
                                           "graph",
                                           "benchmarks"},
          "the top-level record stands after precheck and before benchmarks, "
          "got '" + joined(top) + "'");

  const auto graphText = balancedFrom(text, text.find("\"graph\":"));
  checked(objectKeys(graphText) == std::vector<std::string> {"runs"},
          "the graph object carries its one key runs (FR-023)");

  const auto runsText = balancedFrom(graphText, graphText.find("\"runs\":"));
  const auto entries = objectsIn(runsText);
  check(entries.size() == 2,
        "a batch of two threaded runs gives two entries, in batch order "
        "(FR-023, SC-006)");
  for (const auto& entry : entries) {
    checked(objectKeys(entry)
                == std::vector<std::string> {"benchmark",
                                             "nodes",
                                             "edges",
                                             "held",
                                             "stopped"},
            "the entry keys stand in the x-order the contract states, got '"
                + joined(objectKeys(entry)) + "'");
  }

  const auto nodesText =
      balancedFrom(entries[0], entries[0].find("\"nodes\":"));
  for (const auto& node : objectsIn(nodesText)) {
    checked(objectKeys(node)
                == std::vector<std::string> {"name", "slot", "outcome"},
            "a nodes entry carries name, slot and outcome in that order");
  }
  const auto edgesText =
      balancedFrom(entries[0], entries[0].find("\"edges\":"));
  for (const auto& edge : objectsIn(edgesText)) {
    checked(objectKeys(edge) == std::vector<std::string> {"from", "to"},
            "an edges entry carries from and to in that order");
  }
  const auto heldText = balancedFrom(entries[1], entries[1].find("\"held\":"));
  for (const auto& held : objectsIn(heldText)) {
    checked(objectKeys(held) == std::vector<std::string> {"node", "reason"},
            "a held entry carries node and reason in that order");
  }

  const auto clean = sg::detail::checkReport(SG_SCHEMA_PATH, text);
  checked(clean.has_value(),
          "the written record passes the standing schema check (FR-005): "
              + (clean.has_value() ? std::string {} : clean.error()));

  const auto loaded = sg::loadReport(kGraphDocument);
  checked(loaded.has_value(),
          "the document loads: "
              + (loaded.has_value() ? std::string {} : loaded.error()));
  checked(loaded->graph.has_value(),
          "loadReport reads the record into ReportData::graph (FR-023)");
  if (loaded->graph.has_value()) {
    foldRecord(*loaded->graph, record);
    checked(loaded->graph->runs[0].benchmark == "bmGraphDiamond"
                && loaded->graph->runs[1].benchmark == "bmGraphSecond",
            "the entries stand in the batch's order (SC-006)");
  }
}

// FR-023, SC-002: the record's node array names the nodes in ascending slot
// order, and the console prints that order.
auto nodeOrderAscendingSlots() -> void
{
  const auto record = twoRunRecord();
  const auto text = graphDocument(kGraphDocument, record);
  const auto entries = objectsIn(balancedFrom(text, text.find("\"runs\":")));
  check(entries.size() == 2, "the batch gives two entries");
  for (const auto& entry : entries) {
    const auto slots =
        numberValues(balancedFrom(entry, entry.find("\"nodes\":")), "slot");
    check(slots.size() >= 2, "the node array carries the run's nodes");
    auto spelled = std::string {};
    for (const auto slot : slots) {
      spelled += (spelled.empty() ? "" : ",") + std::to_string(slot);
    }
    checked(std::ranges::is_sorted(slots) && slots.size() >= 2,
            "the record's node array names the nodes in ascending slot order, "
            "got " + spelled);
  }

  const auto printed = printCaptured(
      {Run {threadedResult("bmGraphDiamond", 4), diamondEntry()}});
  const auto at = indexOf(printed, kContextsPrefix);
  checked(lineAt(printed, at + 1)
              == "  graph nodes=a,b,c,d edges=a->c,a->b,c->d,b->d",
          "the console prints the ascending slot order (FR-022, SC-002), got '"
              + lineAt(printed, at + 1) + "'");
}

// FR-023, FR-019, FR-020: the reason names a failed node, and a stopped node
// carries none.
auto reasonNamesFailedNode() -> void
{
  const auto record = settlementRecord();
  const auto text = graphDocument(kGraphDocument, record);
  const auto entryText =
      objectsIn(balancedFrom(text, text.find("\"runs\":")))[0];

  const auto loaded = sg::loadReport(kGraphDocument);
  checked(loaded.has_value() && loaded->graph.has_value(),
          "the settled record reads back (FR-023)");
  if (!loaded->graph.has_value()) {
    return;
  }
  const auto& entry = loaded->graph->runs[0];
  check(entry.held.size() == 2, "the record carries one entry per held node");
  for (const auto& held : entry.held) {
    auto gater = std::optional<const sg::GraphNodeRecord*> {};
    for (const auto& node : entry.nodes) {
      if (node.name == held.reason) {
        gater = &node;
      }
    }
    checked(gater.has_value() && (*gater)->outcome == "failed",
            "the reason names a failed node of the same run: '" + held.node
                + "' names '" + held.reason + "'");
  }

  const auto stoppedText =
      balancedFrom(entryText, entryText.find("\"stopped\":"));
  check(stoppedText.starts_with('['),
        "the stopped list is an array of the stopped names (FR-023)");
  check(objectKeys(stoppedText).empty()
            && stoppedText.find("reason") == std::string::npos,
        "a stopped node carries no reason (FR-023)");
  checked(entry.stopped == std::vector<std::string> {"e", "f"},
          "the stopped names read back in ascending slot order");
}

// FR-024, FR-023: the record carries no timing figure. The key set of the
// whole record is the contract's set and nothing else, and no figure of the
// timed report stands inside the record's text.
auto recordCarriesNoTiming() -> void
{
  // The record of the run with its held nodes: the key set the contract
  // states carries `node` and `reason`, and only a run that holds a node
  // writes them.
  const auto text = graphDocument(kGraphDocument, settlementRecord());
  const auto graphText = balancedFrom(text, text.find("\"graph\":"));

  const auto keys = allKeys(graphText);
  const auto want = std::vector<std::string> {"benchmark",
                                              "edges",
                                              "from",
                                              "held",
                                              "name",
                                              "node",
                                              "nodes",
                                              "outcome",
                                              "reason",
                                              "runs",
                                              "slot",
                                              "stopped",
                                              "to"};
  checked(keys == want,
          "the record carries the contract's keys and no figure, got '"
              + joined(keys) + "'");

  for (const std::string_view figure : {"iterations",
                                        "time",
                                        "mean",
                                        "median",
                                        "samples",
                                        "percentile",
                                        "counters",
                                        "metric",
                                        "chunks",
                                        "label",
                                        "jitter",
                                        "hunt"})
  {
    checked(graphText.find(figure) == std::string::npos,
            "the record carries no timing figure: " + std::string(figure));
  }
}

// ---------------------------------------------------------------------------
// T026: the schema arms
// ---------------------------------------------------------------------------

// The document of a graph run, spelled by hand: each arm plants one
// disagreement into these bytes, so the fact under test is the schema and not
// the writer (FR-005, FR-023, FR-028). The document carries the record of one
// threaded run whose four nodes settle the four words.
auto schemaDocument() -> std::string
{
  return std::string {
      R"({"schemaVersion":"1.9","speedgun":{"version":"0.23.0","buildType":"Release"},)" R"("graph":{"runs":[{"benchmark":"bmGraphSchema",)" R"("nodes":[{"name":"a","slot":0,"outcome":"finished"},)" R"({"name":"b","slot":1,"outcome":"failed"},)" R"({"name":"c","slot":2,"outcome":"held"},)" R"({"name":"d","slot":3,"outcome":"stopped"}],)" R"("edges":[{"from":"a","to":"b"},{"from":"b","to":"c"}],)" R"("held":[{"node":"c","reason":"b"}],"stopped":["d"]}]},)" R"("benchmarks":[]})"};
}

// One planted document, the disagreement it plants, and the position and the
// words the standing checker owes for it: the shape
// `harness_json_schema_test.cpp` gives the standing arms.
struct Plant
{
  std::string text;
  const char* what;
  const char* pointer;
  const char* words;
};

// The schema minor the writer stamps, read from the tree: the helper stands
// in the byte-identity section below, and the schema scenario names the same
// stamp (FR-005, R-13).
auto writerSchemaVersion() -> std::string;

// The `x-order` list one schema object states, spelled the way the schema
// file carries it: the key names quoted and comma-separated in brackets.
auto orderText(const std::vector<std::string>& keys) -> std::string
{
  auto text = std::string("\"x-order\": [");
  for (const auto& key : keys) {
    if (text.back() != '[') {
      text += ", ";
    }
    text += "\"" + key + "\"";
  }
  return text + "]";
}

// The kind one schema property states, spelled the way the schema file
// carries it.
auto kindText(std::string_view key, std::string_view type) -> std::string
{
  return "\"" + std::string(key) + "\": { \"type\": \"" + std::string(type)
      + "\" }";
}

// The `enum` list one schema property states, spelled the way the schema
// file carries it.
auto enumText(const std::vector<std::string>& words) -> std::string
{
  auto text = std::string("\"enum\": [");
  for (const auto& word : words) {
    if (text.back() != '[') {
      text += ", ";
    }
    text += "\"" + word + "\"";
  }
  return text + "]";
}

// FR-005, FR-023, FR-028, SC-006, R-11, R-13: the schema carries the record's
// keys, the entry's `x-order`, the sub-object `x-order` lists and the moved
// top-level `x-order`, and the written document stamps the minor the version
// commit records.
auto schemaCarriesGraphKeysAndOrder() -> void
{
  const auto schema = sg::test::readText(SG_SCHEMA_PATH);
  checked(!schema.empty(), "the schema file stands where the target names it");

  const auto topOrder = orderText({"schemaVersion",
                                   "speedgun",
                                   "run",
                                   "jitter",
                                   "precheck",
                                   "graph",
                                   "benchmarks",
                                   "hunt"});
  check(schema.find(topOrder) != std::string::npos,
        "the top-level x-order carries graph after precheck and before "
        "benchmarks (FR-005, FR-023)");

  const auto graphAt = schema.find("\"graph\": {");
  checked(graphAt != std::string::npos,
          "the schema carries the graph object (FR-005)");
  const auto graphSchema = balancedFrom(schema, graphAt);

  for (const auto& order :
       {orderText({"runs"}),
        orderText({"benchmark", "nodes", "edges", "held", "stopped"}),
        orderText({"name", "slot", "outcome"}),
        orderText({"from", "to"}),
        orderText({"node", "reason"})})
  {
    checked(graphSchema.find(order) != std::string::npos,
            "the graph schema carries the order list " + order + " (FR-023)");
  }
  check(graphSchema.find(enumText({"finished", "failed", "held", "stopped"}))
            != std::string::npos,
        "the graph schema carries the four outcome words (FR-019, FR-023)");
  check(graphSchema.find(kindText("benchmark", "string")) != std::string::npos,
        "the graph schema carries the benchmark kind (FR-023)");
  check(graphSchema.find(kindText("slot", "integer")) != std::string::npos,
        "the graph schema carries the slot kind (FR-023)");
  check(graphSchema.find("\"stopped\": { \"type\": \"array\",")
                != std::string::npos
            && graphSchema.find("\"items\": { \"type\": \"string\" }")
                   != std::string::npos,
        "the graph schema carries the stopped list as an array of names "
        "(FR-023)");

  const auto text = graphDocument(kGraphDocument, twoRunRecord());
  const auto stamped = writerSchemaVersion();
  checked(!stamped.empty(),
          "the schema minor the writer stamps reads from the tree (FR-005)");
  checked(text.find("\"schemaVersion\":\"" + stamped + "\"")
              != std::string::npos,
          "the writer stamps the schema minor the version commit records "
          "(FR-005)");
  const auto clean = sg::detail::checkReport(SG_SCHEMA_PATH, text);
  checked(clean.has_value(),
          "the written record passes the schema (FR-005): "
              + (clean.has_value() ? std::string {} : clean.error()));
}

// FR-005, FR-023, FR-028, SC-006: a document with a missing key, a key of the
// wrong kind, a duplicate key and an order the `x-order` lists state never
// each fail the standing schema check, and the refusal names the position of
// the graph record.
auto plantedGraphDocumentsFailTheSchema() -> void
{
  const auto base = schemaDocument();
  const auto clean = sg::detail::checkReport(SG_SCHEMA_PATH, base);
  checked(clean.has_value(),
          "the graph document passes the schema (FR-005): "
              + (clean.has_value() ? std::string {} : clean.error()));

  const auto benchmarkKey = std::string {"\"benchmark\":\"bmGraphSchema\","};
  const auto at = base.find(benchmarkKey);
  checked(at != std::string::npos, "the document carries the benchmark key");

  auto missing = base;
  missing.erase(at, benchmarkKey.size());

  auto wrongKind = base;
  const auto slot = std::string {"\"slot\":0"};
  checked(wrongKind.find(slot) != std::string::npos,
          "the document carries the slot figure");
  wrongKind.replace(wrongKind.find(slot), slot.size(), "\"slot\":\"0\"");

  auto twice = base;
  twice.insert(twice.find('{') + 1, "\"graph\":{\"runs\":[]},");

  auto outOfOrder = base;
  outOfOrder.erase(at, benchmarkKey.size());
  // The entry's own `stopped` key, the last of that text in the document:
  // the word stands earlier as a node's outcome.
  const auto stoppedAt = outOfOrder.rfind("\"stopped\"");
  checked(stoppedAt != std::string::npos,
          "the document carries the stopped list");
  outOfOrder.insert(stoppedAt, benchmarkKey);

  for (const Plant& plant :
       {Plant {std::move(missing),
               "a key the record requires stands missing",
               "/graph/runs/0",
               "stands missing"},
        Plant {std::move(wrongKind),
               "a key of the record carries the wrong kind",
               "/graph/runs/0/nodes/0/slot",
               "asks for integer"},
        Plant {std::move(twice),
               "a key of the record stands twice in one object",
               "/graph",
               "stands twice"},
        Plant {std::move(outOfOrder),
               "a key of the record stands out of the stated order",
               "/graph/runs/0",
               "out of the order"}})
  {
    const auto answer = sg::detail::checkReport(SG_SCHEMA_PATH, plant.text);
    check(!answer.has_value(), plant.what);
    if (!answer.has_value()) {
      checked(answer.error().find(plant.pointer) != std::string::npos
                  && answer.error().find(plant.words) != std::string::npos,
              "the refusal names the planted graph disagreement at "
                  + std::string(plant.pointer) + ", got '" + answer.error()
                  + "'");
    }
  }
}

// ---------------------------------------------------------------------------
// T027: the byte identity and the ABI fact
// ---------------------------------------------------------------------------

// The document master's writer emits for a run that carries a pre-check and
// no graph, with the two values FR-006 names held out of the fold: the
// `schemaVersion` value, which the minor move carries, and the `speedgun`
// object, which names the build that wrote the file. Both stand masked, the
// way `harness_hunt_json_test.cpp` masks them, so the fold reads binary at
// the minor of the tree that runs it. The rest of the bytes stand as
// `harness_noise_report_test.cpp` spells the pre-check document.
constexpr std::string_view kMasterDocument =
    "{\"schemaVersion\":\"?\",\"speedgun\":\"?\",\"precheck\":{"
    "\"serial\":false,\"windowMs\":7,\"cpus\":[],"
    "\"hostSettings\":{\"isolcpus\":null,\"nohz_full\":null,"
    "\"rcu_nocbs\":null,\"governors\":{},\"irqAffinity\":{}},"
    "\"skippedGroups\":[]},\"benchmarks\":[]}";

// The console bytes master prints for a threaded run of the scenario's
// benchmark: the measured row and the standing context disclosure, the shape
// `harness_context_report_test.cpp` spells for the disclosure line.
constexpr auto kMasterRow =
    "bmGraphBytes             iterations=      1000  real time/iter (ns)=  "
    "250.000  cpu time/iter (ns)=  200.000  overhead floor (ns)=    0.000  "
    "endpoint samples=2";
constexpr auto kMasterDisclosure =
    "  contexts=2  ctx0=SPIN/NO_PIN  ctx1=SPIN/NO_PIN";

// The `speedgun` object, held out of the identity the way
// `harness_noise_report_test.cpp` holds it out: the object names the build
// that wrote the file, and says nothing about the run that wrote it.
auto maskSpeedgun(std::string text) -> std::string
{
  const std::string header = "\"speedgun\":";
  const auto at = text.find(header);
  if (at == std::string::npos) {
    return text;
  }
  const auto start = at + header.size();
  const auto end = text.find('}', start) + 1;
  text.replace(start, end - start, "\"?\"");
  return text;
}

// The `schemaVersion` value, the second value FR-006 names: the minor the
// writer moves for the key it adds.
auto maskSchemaVersion(std::string text) -> std::string
{
  const std::string key = "\"schemaVersion\":\"";
  const auto at = text.find(key);
  if (at == std::string::npos) {
    return text;
  }
  const auto start = at + key.size();
  const auto end = text.find('"', start);
  text.replace(start, end - start, "?");
  return text;
}

// The schema minor the writer stamps: the `kSchemaVersion` text of
// `source/harness/json_report.cpp`, read from the tree the way
// `projectVersion` at `test/source/harness_cli_table_test.cpp:1234` reads the
// project version out of `CMakeLists.txt`. The check then names the value the
// tree carries, so it stands binary at the minor of the tree that runs it
// (FR-005, R-13).
auto writerSchemaVersion() -> std::string
{
  const std::array<const char*, 3> prefixes {
      "source/harness/",
      "harness/",
      "source/",
  };
  const std::vector<std::filesystem::path> starts {
      std::filesystem::current_path(),
      std::filesystem::path {__FILE__},
  };
  for (auto start : starts) {
    for (int hop = 0; hop <= 8 && !start.empty(); ++hop) {
      for (const char* prefix : prefixes) {
        const auto path = start / prefix / "json_report.cpp";
        if (!std::filesystem::exists(path)) {
          continue;
        }
        const auto text = sg::test::readText(path.string());
        constexpr std::string_view kLead = "kSchemaVersion = \"";
        const auto at = text.find(kLead);
        if (at == std::string::npos) {
          return {};
        }
        const auto value = at + kLead.size();
        const auto end = text.find('"', value);
        return end == std::string::npos ? std::string {}
                                        : text.substr(value, end - value);
      }
      start = start.parent_path();
    }
  }
  return {};
}

// FR-006, SC-005, R-11: a run with no `--graph` writes the document master
// writes and prints the console master prints, the `schemaVersion` value and
// the `speedgun.version` value standing the only differences. A graph entry
// adds the block's lines and moves no row.
auto noGraphBytesMatchMaster() -> void
{
  const auto text = graphDocument(kGraphDocument, std::nullopt);
  check(text.find("\"graph\"") == std::string::npos,
        "a run with no graph writes no graph key (FR-006)");
  const auto stamped = writerSchemaVersion();
  checked(!stamped.empty(),
          "the schema minor the writer stamps reads from the tree (FR-005)");
  check(text.find("\"schemaVersion\":\"" + stamped + "\"") != std::string::npos,
        "the document stamps the minor the writer carries (FR-005)");
  const auto masked = maskSchemaVersion(maskSpeedgun(text));
  checked(masked == kMasterDocument,
          "the document of a run with no graph is master's bytes apart from "
          "the two values FR-006 names, got '" + masked + "'");

  const auto loaded = sg::loadReport(kGraphDocument);
  checked(loaded.has_value() && !loaded->speedgunVersion.empty(),
          "the document carries the project version the version move carries "
          "(FR-006)");

  const auto printed =
      printCaptured({Run {threadedResult("bmGraphBytes", 2), std::nullopt}});
  checked(printed.size() == 2 && printed[0] == kMasterRow
              && printed[1] == kMasterDisclosure,
          "the console of a run with no graph is master's bytes, got '"
              + joined(printed) + "'");

  const auto withGraph =
      printCaptured({Run {threadedResult("bmGraphBytes", 2), pairEntry()}});
  checked(withoutBlock(withGraph) == printed,
          "the timed rows keep master's bytes beside a graph, got '"
              + joined(withGraph) + "'");
}

// FR-004, FR-023, SC-005, D-8, R-13: `ReportData::graph` stands in the public
// loaded-report struct with the record's type, the growth that moves the
// harness `SOVERSION`; the number moves beside the check, in the version
// commit. A document with no record leaves the member empty.
auto reportDataGraphMemberRaisesAbi() -> void
{
  static_assert(std::is_same_v<decltype(sg::ReportData::graph),
                               std::optional<sg::GraphRecord>>);

  const auto filled = graphDocument(kGraphDocument, twoRunRecord());
  check(filled.find("\"graph\"") != std::string::npos,
        "the record stands in the document (FR-023)");
  const auto read = sg::loadReport(kGraphDocument);
  checked(read.has_value() && read->graph.has_value()
              && read->graph->runs.size() == 2,
          "loadReport fills ReportData::graph with the record's runs (FR-023)");

  const auto bare = graphDocument(kGraphDocument, std::nullopt);
  check(bare.find("\"graph\"") == std::string::npos,
        "a run with no graph writes no graph key (FR-006)");
  const auto empty = sg::loadReport(kGraphDocument);
  checked(empty.has_value() && !empty->graph.has_value(),
          "a document with no record leaves ReportData::graph empty (FR-023)");
}

// ---------------------------------------------------------------------------
// The scenario list
// ---------------------------------------------------------------------------

struct Scenario
{
  const char* name;
  void (*body)();
};

auto scenarios() -> const std::vector<Scenario>&
{
  static const std::vector<Scenario> kList {
      Scenario {"consoleBlockBesideContextsLine",
                consoleBlockBesideContextsLine},
      Scenario {"blockOncePerRun", blockOncePerRun},
      Scenario {"recordKeysOrderAndReadBack", recordKeysOrderAndReadBack},
      Scenario {"nodeOrderAscendingSlots", nodeOrderAscendingSlots},
      Scenario {"reasonNamesFailedNode", reasonNamesFailedNode},
      Scenario {"recordCarriesNoTiming", recordCarriesNoTiming},
      Scenario {"schemaCarriesGraphKeysAndOrder",
                schemaCarriesGraphKeysAndOrder},
      Scenario {"plantedGraphDocumentsFailTheSchema",
                plantedGraphDocumentsFailTheSchema},
      Scenario {"noGraphBytesMatchMaster", noGraphBytesMatchMaster},
      Scenario {"reportDataGraphMemberRaisesAbi",
                reportDataGraphMemberRaisesAbi},
  };
  return kList;
}

// The parent's driver: one child runs the scenario body, and the success
// status stands owed.
auto runChild(const std::string& name) -> int
{
  const pid_t pid = fork();
  if (pid < 0) {
    fail("cannot fork the child");
  }
  if (pid == 0) {
    for (const auto& scenario : scenarios()) {
      if (scenario.name == name) {
        scenario.body();
        return 0;
      }
    }
    fail("the scenario name is not one of this file's list");
  }
  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    fail("cannot wait for the child");
  }
  return WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
}

}  // namespace

auto main(const int argc, char** argv) -> int
{
  if (argc > 1) {
    const auto name = std::string(argv[1]);
    for (const auto& scenario : scenarios()) {
      if (scenario.name == name) {
        scenario.body();
        return 0;
      }
    }
    fail("the scenario name is not one of this file's list");
  }
  for (const auto& scenario : scenarios()) {
    check(runChild(scenario.name) == 0,
          "the scenario's process ended with the success status");
  }
  std::printf("harness_graph_report_test: ok\n");
  return 0;
}
