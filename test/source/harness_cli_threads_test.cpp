// ============================================================================
// TDD test for the `--threads` list reader of
// specs/028-full-command-line (T039, T042; FR-044, FR-045, FR-046,
// FR-047, FR-050, FR-081, SC-003, SC-004,
// contracts/threads-and-interleave.md).
//
// The reader answers one count per item in item order, or one fault line.
// The entry point turns that line into one line on standard error at
// status 2 with no benchmark row printed, which is what SC-003 asks of a
// bad `--threads` list. This file compares the text the reader answers
// byte for byte and leaves the status to `harness_cli_test` and
// `base_cli_matrix`.
//
// The seam `detail::readThreadList` stands declared by T006 in
// `detail/options.hpp` and is defined in `source/harness/options.cpp` by
// T040, the task that registers this target in `test/CMakeLists.txt`. The
// declaration below carries the seam until that definition lands, so the
// link error naming that one symbol is the red state of this task. The
// expansion the range form feeds, `detail::buildThreadCounts`, already
// stands at `source/harness/registry.cpp:574`, so the range cases state
// their counts against that function.
//
// FR-050 asks that the option reach every family whatever thread call the
// family made. The family side of that rule is the registry write of T041;
// what stands here is the shape that write needs: one flat list, one count
// per comma-separated item, whatever the shapes of the items are.
//
// The reader cases parse count text and run nothing, so no cpu count of
// the machine enters their expectations. The over-affinity case of T042
// runs the entry point at one count above the cpus the affinity mask
// allows, and it reads that count from `sched_getaffinity` so no cpu
// count stands in this file's text (FR-081). `bmThreads` is the family
// the case runs, and the console must disclose the count (FR-047). The
// case stands red until the `--threads` scan arm of T064, the registry
// write of T041 and the disclosure of T043 land, and its assertions
// name what is missing. The filter keeps its `^` anchor and drops the
// `$`, because the instance name carries the `/threads:<n>` segment
// above one thread (FR-048).
//
// Check map:
//   aSingleCountAnswersOneCount  `--threads=6` answers the one count 6
//                                (FR-044, FR-045)
//   aRangeExpandsByDoubling      four bounds answer the counts of
//                                `detail::buildThreadCounts`: doubling from
//                                the low bound, every count at or below the
//                                high bound, and the next doubling past it
//                                (FR-045)
//   aListKeepsOneCountPerItem    `--threads=2,4:32,7` answers the counts of
//                                its three items in item order, the flat
//                                list one loop writes into every entry
//                                (FR-045, FR-050)
//   aCountBelowOneIsRefused      `--threads=0` answers the refusal line
//                                naming the text (FR-046, SC-003)
//   aRangeWithItsLowAboveItsHighIsRefused
//                                `--threads=8:2` answers the refusal line
//                                naming the text (FR-046, SC-003)
//   aMalformedItemIsRefused      `--threads=abc`, `--threads=2:` and
//                                `--threads=2,0` answer the same line, each
//                                naming its whole text (FR-046, SC-003)
//   aCountAboveTheAffinityCountRunsAndDisclosesItself
//                                one count above the `sched_getaffinity`
//                                cpu count runs at status 0, prints its
//                                row, and the console carries a line
//                                naming the option and the count
//                                (FR-047, FR-081, SC-004)
// ============================================================================

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <expected>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <sched.h>
#include <unistd.h>

#if __has_include("detail/options.hpp")
#  include "detail/options.hpp"
#else
namespace sg::detail
{
// The reader of E-11 as T006 declares it, so this file compiles on its own
// ahead of that header. T040 defines it in `source/harness/options.cpp`.
[[nodiscard]] auto readThreadList(std::string_view text)
    -> std::expected<std::vector<int>, std::string>;

// The doubling of FR-016 that the range form feeds, defined at
// `source/harness/registry.cpp:574`.
[[nodiscard]] auto buildThreadCounts(int minCount,
                                     int maxCount) -> std::vector<int>;
}  // namespace sg::detail
#endif

#include "speedgun-ng/benchmark.hpp"

namespace
{

// The one family the over-affinity case runs. An empty body keeps the
// run short at any count.
auto bmThreads(sg::State& state) -> void
{
  for (auto _ : state) {
  }
}

SG_BENCHMARK(bmThreads);

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

auto report(const char* what,
            const std::string& expected,
            const std::string& actual) -> void
{
  ++failures;
  std::printf("FAILED: %s\n  expected: %s\n  actual:   %s\n",
              what,
              expected.c_str(),
              actual.c_str());
}

// The counts as one space separated line, the shape the contract table
// prints, so a failing case reports the list it saw.
auto render(const std::vector<int>& counts) -> std::string
{
  std::string text;
  for (const int count : counts) {
    if (!text.empty()) {
      text += ' ';
    }
    text += std::to_string(count);
  }
  return text;
}

// The refusal line the contract fixes for a list the reader turns away:
// the option, then the whole text as the operator typed it.
auto refusalLine(std::string_view text) -> std::string
{
  return "--threads: expected counts and min:max ranges, got '"
      + std::string(text) + "'";
}

// The value of one `--threads=<text>` word: the characters after the first
// `=`, which is what the scan hands the reader (FR-010).
auto valueOf(std::string_view word) -> std::string_view
{
  const auto split = word.find('=');
  return split == std::string_view::npos ? std::string_view {}
                                         : word.substr(split + 1);
}

// One command-line word through the reader.
auto readWord(const std::string& word)
    -> std::expected<std::vector<int>, std::string>
{
  return sg::detail::readThreadList(valueOf(word));
}

// One `min:max` word built from its bounds, so the same bounds can go to
// the reader and to `buildThreadCounts`.
auto rangeWord(const int minCount, const int maxCount) -> std::string
{
  return "--threads=" + std::to_string(minCount) + ':'
      + std::to_string(maxCount);
}

// One accepted list: the reader must answer, and its counts must read the
// expected line.
auto expectCounts(const std::string& word,
                  const std::string& expected,
                  const char* what) -> void
{
  const auto answer = readWord(word);
  if (!answer.has_value()) {
    report(what, expected, "the reader refused the text: " + answer.error());
    return;
  }
  if (render(*answer) != expected) {
    report(what, expected, render(*answer));
  }
}

// One refused word: the reader must answer the refusal line that names the
// whole text. The entry point prints that line at status 2 with no
// benchmark row, the reading SC-003 holds it to.
auto expectRefusal(const std::string& word, const char* what) -> void
{
  const auto answer = readWord(word);
  const std::string expected = refusalLine(valueOf(word));
  if (answer.has_value()) {
    report(
        what, expected, "the reader accepted the counts: " + render(*answer));
    return;
  }
  if (answer.error() != expected) {
    report(what, expected, answer.error());
  }
}

// The cpus this process may run on, read from its own affinity mask.
// FR-081 asks that every cpu count a test uses come from this read, so
// no case names a count of its own.
auto affinityCount() -> int
{
  cpu_set_t cpus {};
  CPU_ZERO(&cpus);
  if (sched_getaffinity(0, sizeof(cpus), &cpus) != 0) {
    fail("the affinity mask of this process is readable");
    return 0;
  }
  return CPU_COUNT(&cpus);
}

// One run of the entry point with both output streams captured to files
// in the system temporary directory under this process number: the
// console text and the answered status are read together, and a fault
// line stays out of this file's own verdict output. The scan calls no
// `std::exit` on the command lines these cases give.
struct Run
{
  int status = 0;
  std::string out;
  std::string err;
};

auto capturePath(const char* kind) -> std::filesystem::path
{
  return std::filesystem::temp_directory_path()
      / ("harness_cli_threads_" + std::to_string(static_cast<long>(getpid()))
         + kind);
}

auto readText(const std::filesystem::path& path) -> std::string
{
  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

auto runEntry(const std::vector<std::string>& words) -> Run
{
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("harness_cli_threads_test"));
  for (const std::string& word : words) {
    argv.push_back(const_cast<char*>(word.c_str()));
  }

  const std::filesystem::path outPath = capturePath(".out");
  const std::filesystem::path errPath = capturePath(".err");
  const int savedOut = dup(STDOUT_FILENO);
  const int savedErr = dup(STDERR_FILENO);
  if (savedOut < 0 || savedErr < 0
      || std::freopen(outPath.c_str(), "w", stdout) == nullptr
      || std::freopen(errPath.c_str(), "w", stderr) == nullptr)
  {
    fail("the two output streams of the entry point can be redirected");
    return {};
  }

  Run run;
  run.status = sg::speedgunMain(static_cast<int>(argv.size()), argv.data());
  std::fflush(stdout);
  std::fflush(stderr);
  if (dup2(savedOut, STDOUT_FILENO) < 0 || dup2(savedErr, STDERR_FILENO) < 0) {
    fail("the two output streams of the entry point can be restored");
  }
  close(savedOut);
  close(savedErr);
  run.out = readText(outPath);
  run.err = readText(errPath);
  std::filesystem::remove(outPath);
  std::filesystem::remove(errPath);
  return run;
}

// The lines one capture printed.
auto linesOf(const std::string& text) -> std::vector<std::string>
{
  std::vector<std::string> lines;
  std::string line;
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

// Whether the console discloses the count: one line naming the option
// and the count, on either stream. The benchmark row carries the count
// inside the instance name's `/threads:` segment (FR-048), and the
// noise warning of specs/026 names the spinning count beside the cpu
// count in words that hold no option name, so a line naming the family
// or missing the option word discloses nothing here.
auto disclosesTheCount(const Run& run, const std::string& countText) -> bool
{
  for (const std::string& line : linesOf(run.out + run.err)) {
    if (line.find("bmThreads") != std::string::npos) {
      continue;
    }
    if (line.find("threads") != std::string::npos
        && line.find(countText) != std::string::npos)
    {
      return true;
    }
  }
  return false;
}

auto aSingleCountAnswersOneCount() -> void
{
  expectCounts(
      "--threads=6", "6", "a single count answers that count (FR-044, FR-045)");
}

auto aRangeExpandsByDoubling() -> void
{
  for (const auto& bounds : {std::pair {3, 40},
                             std::pair {2, 16},
                             std::pair {5, 8},
                             std::pair {1, 1}})
  {
    const auto word = rangeWord(bounds.first, bounds.second);
    const auto answer = readWord(word);
    if (!answer.has_value()) {
      report("a min:max range answers a list of counts (FR-045)",
             "a list of counts",
             answer.error());
      continue;
    }
    const auto expansion =
        sg::detail::buildThreadCounts(bounds.first, bounds.second);
    if (render(*answer) != render(expansion)) {
      report("a min:max range answers the counts of detail::buildThreadCounts "
             "(FR-045)",
             render(expansion),
             render(*answer));
    }
    check(!answer->empty() && answer->front() == bounds.first,
          "the expansion starts at the low bound (FR-045)");
    for (std::size_t index = 1; index < answer->size(); ++index) {
      check((*answer)[index] == 2 * (*answer)[index - 1],
            "each count doubles the count before it (FR-045)");
    }
    check(answer->back() <= bounds.second,
          "every count stays at or below the high bound (FR-045)");
    check(static_cast<std::int64_t>(answer->back()) * 2 > bounds.second,
          "the next doubling would pass the high bound (FR-045)");
  }
}

auto aListKeepsOneCountPerItem() -> void
{
  std::vector<int> expected {2};
  const auto middle = sg::detail::buildThreadCounts(4, 32);
  expected.insert(expected.end(), middle.begin(), middle.end());
  expected.push_back(7);
  expectCounts("--threads=2,4:32,7",
               render(expected),
               "a mixed list answers one count per item, in item order "
               "(FR-045, FR-050)");
}

auto aCountBelowOneIsRefused() -> void
{
  expectRefusal("--threads=0",
                "a count below one answers the refusal line naming the text "
                "(FR-046, SC-003)");
}

auto aRangeWithItsLowAboveItsHighIsRefused() -> void
{
  expectRefusal("--threads=8:2",
                "a range whose low bound stands above its high bound answers "
                "the refusal line naming the text (FR-046, SC-003)");
}

auto aMalformedItemIsRefused() -> void
{
  expectRefusal("--threads=abc",
                "an item that is neither a count nor a range answers the "
                "refusal line naming the text (FR-046, SC-003)");
  expectRefusal("--threads=2:",
                "a range with no high bound answers the refusal line naming "
                "the text (FR-046, SC-003)");
  expectRefusal("--threads=2,0",
                "a list with one bad item names its whole text in the "
                "refusal line (FR-046, SC-003)");
}

// FR-047 sets no cpu cap: a count above the affinity count runs, and
// the console discloses it. The count is the `sched_getaffinity` count
// plus one, so no cpu count stands in this file (FR-081).
auto aCountAboveTheAffinityCountRunsAndDisclosesItself() -> void
{
  const std::string countText = std::to_string(affinityCount() + 1);
  const auto run = runEntry(
      {"--threads=" + countText, "--filter=^bmThreads", "--iterations=1"});
  check(run.status == 0,
        "a count above the affinity count runs at status 0 (FR-047)");
  check(run.out.find("bmThreads") != std::string::npos,
        "the over-affinity run prints its benchmark row (FR-047)");
  check(disclosesTheCount(run, countText),
        "the console discloses the over-affinity count in a line naming "
        "the option and the count (FR-047)");
}

}  // namespace

auto main() -> int
{
  aSingleCountAnswersOneCount();
  aRangeExpandsByDoubling();
  aListKeepsOneCountPerItem();
  aCountBelowOneIsRefused();
  aRangeWithItsLowAboveItsHighIsRefused();
  aMalformedItemIsRefused();
  aCountAboveTheAffinityCountRunsAndDisclosesItself();
  std::printf(failures == 0 ? "ok\n" : "%d checks failed\n", failures);
  return failures == 0 ? 0 : 1;
}
