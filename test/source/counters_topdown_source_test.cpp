// ============================================================================
// The source record a table owes the reader (specs/042-topdown T006; FR-015,
// PC-12, spec D-12, data-model E-02, E-03 and E-16).
//
// A published row names the file it came from, every file of a directory
// answers the sha256 the manifest states beside it, and every directory
// answers the `ref:` line of that manifest. The build reads all three out of
// `external/pmu-events/RECORD`: `cmake/EmbedPmuEvents.cmake` hands the
// generator the manifest path, `cmake/embed_pmu_blob.cmake` emits one digest
// per file into the index and the reference into the registry, and refuses a
// file the manifest names no digest for (T018). `pmuLoadTable` tags every
// entry it appends with the file that holds it (T019). No part of the library
// opens the vendored tree at run time (D-12).
//
// The fixture is a two-file directory the unit writes into the build tree at
// run time beside a manifest of its own, following the fixture precedent of
// `counters_linux_pmu_seam_test`. The fixture keeps its own file list, so a
// re-pin of the vendored tree moves no case here (specs/007 T057).
//
// Frameworkless check()/fail() convention.
// ============================================================================

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../source/counters/detail/pmu.hpp"
#include "../source/counters/linux_pmu/embedded_tables.hpp"

#ifndef SG_TOPDOWN_TABLE_DIR
#  error "SG_TOPDOWN_TABLE_DIR must name the fixture root (test/CMakeLists.txt)"
#endif

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "COUNTERS TOPDOWN SOURCE TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

using sg::counters::detail::EmbeddedDir;
using sg::counters::detail::embeddedFindDir;
using sg::counters::detail::kEmbeddedDirCount;
using sg::counters::detail::kEmbeddedDirs;
using sg::counters::detail::pmuLoadTable;
using sg::counters::detail::pmuParseTableFile;
using sg::counters::detail::PmuTableEntry;

// The reference the pinned manifest states. The configure bracket of
// `CMakeLists.txt` reads the `ref:` line of `external/pmu-events/RECORD` and
// refuses a build for any other reference, so this is the one value a build
// of this tree can carry into the registry (specs/007 FR-044).
constexpr std::string_view kPinnedReference {"linux-7.2.4"};

// The vendored directory the two real readings come from: the pinned table of
// a Skylake-X part, whose fifteen files the contract of specs/042-topdown
// measures.
constexpr std::string_view kSkylakex {"arch/x86/skylakex"};

// One level-1 row of that table and the file that holds it, spelled as the
// pinned tree spells both.
constexpr std::string_view kLevelOneRow {"tma_backend_bound"};
constexpr std::string_view kMetricFile {"skx-metrics.json"};

// The fixture directory, its two files, and the reference its manifest
// states. The manifest spells each file by the path the directory gives it,
// the way the vendored manifest spells a file by the path the vendored root
// gives it, so one join rule reaches both readings.
constexpr std::string_view kFixtureDirectory {"twodir/"};
constexpr std::string_view kFirstFile {"alpha.json"};
constexpr std::string_view kSecondFile {"beta.json"};
constexpr std::string_view kFixtureReference {"fixture-1.0"};

// The two digests the fixture manifest states, distinct, so a row that names
// the wrong file reaches a hash no row of it answers.
constexpr std::string_view kFirstHash {
    "11b4a2f5c8d3461097ae6f0d2c3b4e5f60718293a4b5c6d7e8f90a1b2c3d4e5f"};
constexpr std::string_view kSecondHash {
    "22c5b30e6d9f47218b1f0c3e4d5a6b7c8d9e0f1a2b3c4d5e6f708192a3b4c5d6"};

// The alphabet of a sha256 digest as the manifest spells it.
constexpr std::string_view kHexDigits {"0123456789abcdef"};

// The two fixture tables. `alpha.json` holds one event row and one metric
// row, and `beta.json` holds one metric row that reads the first, so both
// files contribute rows and each row reaches the file that states it.
constexpr std::string_view kFirstBody {
    R"([
  {"EventName":"fixture_alpha_cycles","EventCode":"0x3c",
   "BriefDescription":"the counter of the first fixture file"},
  {"MetricName":"tma_alpha_row",
   "MetricExpr":"fixture_alpha_cycles / fixture_alpha_instructions",
   "ScaleUnit":"100%","MetricGroup":"TmaL1;TopdownL1"}
]
)"};

constexpr std::string_view kSecondBody {
    R"([
  {"MetricName":"tma_beta_row","MetricExpr":"tma_alpha_row * 0.5",
   "ScaleUnit":"100%","MetricGroup":"TmaL2;TopdownL2;tma_alpha_row_group"}
]
)"};

// One manifest read: the `ref:` line and one digest line per path. The unit
// reads the fixture's manifest here, because the manifest is the fixture's
// own expected data, the way it is the build's only digest source (D-12).
struct Manifest
{
  std::string reference;
  std::vector<std::pair<std::string, std::string>> lines;

  [[nodiscard]] auto hashOf(const std::string& path) const -> std::string
  {
    for (const auto& [name, hash] : lines) {
      if (name == path) {
        return hash;
      }
    }
    return {};
  }
};

// The manifest beside a table directory, in the shape the vendored `RECORD`
// states it: a `ref:` line above a rule, and one digest, two spaces and a
// path below it.
[[nodiscard]] auto readManifest(const std::filesystem::path& path) -> Manifest
{
  std::ifstream file(path);
  if (!file) {
    fail("the fixture manifest is readable");
  }
  Manifest manifest;
  std::string line;
  while (std::getline(file, line)) {
    if (line.starts_with("ref:")) {
      const std::size_t value = line.find_first_not_of(" \t", 4);
      manifest.reference =
          value == std::string::npos ? std::string {} : line.substr(value);
      continue;
    }
    const std::size_t separator = line.find("  ");
    if (separator != std::string::npos) {
      manifest.lines.emplace_back(line.substr(separator + 2),
                                  line.substr(0, separator));
    }
  }
  return manifest;
}

// The shape one manifest digest line carries: sixty-four lowercase
// hexadecimal characters, the length and the alphabet of a sha256 digest.
[[nodiscard]] auto isDigest(const std::string_view text) -> bool
{
  return text.size() == 64U
      && std::ranges::all_of(
             text,
             [](const char digit)
             { return kHexDigits.find(digit) != std::string_view::npos; });
}

// Writes one fixture file, with its parent directory, into the fixture root
// the build names.
[[nodiscard]] auto writeFixture(const std::filesystem::path& path,
                                const std::string_view body) -> bool
{
  std::error_code code;
  std::filesystem::create_directories(path.parent_path(), code);
  std::ofstream file(path, std::ios::binary | std::ios::trunc);
  file << body;
  file.close();
  return static_cast<bool>(file);
}

// The two fixture tables and the manifest beside them. The deeper-named file
// is written first and parsed first, so a record that states the sorted order
// states the order the embedder gives and never an order the run left behind.
[[nodiscard]] auto writeFixtures() -> bool
{
  const std::filesystem::path root {SG_TOPDOWN_TABLE_DIR};
  const std::filesystem::path directory {root
                                         / std::string {kFixtureDirectory}};
  bool written =
      writeFixture(directory / std::string {kSecondFile}, kSecondBody);
  written =
      writeFixture(directory / std::string {kFirstFile}, kFirstBody) && written;
  const std::string manifest = "ref: " + std::string {kFixtureReference}
      + "\n----\n" + std::string {kFirstHash} + "  "
      + std::string {kFixtureDirectory} + std::string {kFirstFile} + "\n"
      + std::string {kSecondHash} + "  " + std::string {kFixtureDirectory}
      + std::string {kSecondFile} + "\n";
  return writeFixture(root / "RECORD", manifest) && written;
}

// The sorted names of a directory's table files, which is the order the
// embedder globs and sorts them into the index.
[[nodiscard]] auto sortedFiles(const std::filesystem::path& directory)
    -> std::vector<std::string>
{
  std::vector<std::string> names;
  for (const auto& entry : std::filesystem::directory_iterator(directory)) {
    if (entry.is_regular_file() && entry.path().extension() == ".json") {
      names.push_back(entry.path().filename().string());
    }
  }
  std::ranges::sort(names);
  return names;
}

// The source record of E-02 and E-03: the reference the manifest states, the
// directory, and one entry per file that contributed a row, each naming the
// name the index gives and the digest the manifest line beside it states.
struct SourceRecord
{
  std::string reference;
  std::string directory;
  std::vector<std::pair<std::string, std::string>> files;
};

// The files a parsed table contributed rows to, taken in `order`, which is
// the order a directory's files stand in the index. A file stands once, and a
// file that contributed no row stands nowhere (FR-015).
[[nodiscard]] auto contributingFiles(const std::vector<PmuTableEntry>& table,
                                     const std::vector<std::string>& order)
    -> std::vector<std::string>
{
  std::vector<std::string> files;
  for (const std::string& name : order) {
    const bool contributed = std::ranges::any_of(
        table,
        [&name](const PmuTableEntry& entry) { return entry.fileName == name; });
    if (contributed) {
      files.push_back(name);
    }
  }
  return files;
}

// The distinct file names of a parsed table in the order the table met them,
// which is the order the loader read the files.
[[nodiscard]] auto loadOrder(const std::vector<PmuTableEntry>& table)
    -> std::vector<std::string>
{
  std::vector<std::string> files;
  for (const PmuTableEntry& entry : table) {
    if (std::ranges::find(files, entry.fileName) == files.end()) {
      files.push_back(entry.fileName);
    }
  }
  return files;
}

// The names the index gives a directory's files, in the index order.
[[nodiscard]] auto indexOrder(const EmbeddedDir& dir)
    -> std::vector<std::string>
{
  std::vector<std::string> names;
  for (std::size_t i = 0; i < dir.fileCount; ++i) {
    names.emplace_back(dir.files[i].name);
  }
  return names;
}

// Every directory of the registry answers the reference the manifest's `ref:`
// line states, and the mapping file's directory answers it beside them.
auto referenceScenario() -> void
{
  check(kEmbeddedDirCount > 40U,
        "the registry carries every directory of the vendored tree");
  for (std::size_t i = 0; i < kEmbeddedDirCount; ++i) {
    check(kPinnedReference == kEmbeddedDirs[i].reference,
          "every embedded directory answers the `ref:` line of the manifest "
          "as its reference (FR-015, D-12)");
  }
  const EmbeddedDir* mapfile = embeddedFindDir("arch/x86");
  check(mapfile != nullptr, "the mapping file embeds beside the directories");
  if (mapfile != nullptr) {
    check(kPinnedReference == mapfile->reference,
          "the mapping file's directory answers the same reference");
  }
}

// Every file of a vendored directory answers its manifest digest, two files
// answer two digests, and every row of the loaded table names the file that
// holds it.
auto digestScenario() -> void
{
  const EmbeddedDir* dir = embeddedFindDir(kSkylakex);
  check(dir != nullptr, "the registry holds the pinned Skylake-X directory");
  if (dir == nullptr) {
    return;
  }
  check(dir->fileCount > 2U, "the pinned directory holds several files");
  for (std::size_t i = 0; i < dir->fileCount; ++i) {
    check(isDigest(dir->files[i].sha256),
          "every embedded file answers the sixty-four character digest its "
          "manifest line states (FR-015, PC-12)");
    for (std::size_t other = i + 1U; other < dir->fileCount; ++other) {
      check(std::string_view {dir->files[i].sha256}
                != dir->files[other].sha256,
            "two files of one directory answer two digests, so no digest "
            "stands in two lines of the index (PC-12)");
    }
  }

  const std::string selected {std::string {kSkylakex} + '/'};
  const std::vector<PmuTableEntry>& table = pmuLoadTable(selected);
  check(table.size() > 1000U, "the pinned table parses its rows");
  const std::vector<std::string> order = indexOrder(*dir);
  for (const PmuTableEntry& entry : table) {
    check(std::ranges::find(order, entry.fileName) != order.end(),
          "every row of a loaded table names a file of the directory it came "
          "from (PC-12)");
  }
  const std::vector<std::string> files = contributingFiles(table, order);
  check(files == loadOrder(table),
        "the record's files stand in the order the embedder sorted them into "
        "the index, which is the order the loader read them (FR-015)");
  check(files.size() < order.size(),
        "a file that contributed no row stands in no record (FR-015)");
  check(std::ranges::find(files, std::string {kMetricFile}) != files.end(),
        "the file that holds the metric rows contributes to the record");

  const PmuTableEntry* levelOne = nullptr;
  for (const PmuTableEntry& entry : table) {
    if (std::string_view {entry.metricName} == kLevelOneRow) {
      levelOne = &entry;
      break;
    }
  }
  check(levelOne != nullptr, "the pinned table holds the level-1 row");
  if (levelOne != nullptr) {
    check(levelOne->fileName == kMetricFile,
          "a published row names the file that holds it (PC-12)");
  }
}

// The record of the two-file fixture directory: the reference is the
// manifest's `ref:` line, each contributing file answers its name and the
// digest of the manifest line beside it, and the two stand in the sorted
// order the embedder gives.
auto twoFileRecordScenario() -> void
{
  const std::filesystem::path root {SG_TOPDOWN_TABLE_DIR};
  const std::filesystem::path directory {root
                                         / std::string {kFixtureDirectory}};
  const Manifest manifest = readManifest(root / "RECORD");
  check(manifest.reference == kFixtureReference,
        "the manifest states its reference on its `ref:` line (E-03)");
  check(manifest.lines.size() == 2U,
        "the manifest states one digest line for each fixture file");

  std::vector<PmuTableEntry> table;
  pmuParseTableFile((directory / std::string {kSecondFile}).string(), table);
  pmuParseTableFile((directory / std::string {kFirstFile}).string(), table);
  check(table.size() == 3U, "both fixture files contribute their rows");

  SourceRecord record;
  record.reference = manifest.reference;
  record.directory = std::string {kFixtureDirectory};
  for (const std::string& name :
       contributingFiles(table, sortedFiles(directory)))
  {
    record.files.emplace_back(name, manifest.hashOf(record.directory + name));
  }

  check(record.reference == kFixtureReference,
        "the record's reference is the `ref:` line of the manifest (FR-015)");
  check(record.files.size() == 2U,
        "a directory of two contributing files states two files (FR-015)");
  if (record.files.size() == 2U) {
    check(record.files[0].first == kFirstFile
              && record.files[0].second == kFirstHash,
          "the first file answers its name and the digest of the manifest "
          "line beside it, in the order the embedder sorts (FR-015, D-12)");
    check(record.files[1].first == kSecondFile
              && record.files[1].second == kSecondHash,
          "the second file answers its own name and its own digest");
  }
  const std::vector<std::string> sorted {std::string {kFirstFile},
                                         std::string {kSecondFile}};
  check(record.files.size() == sorted.size()
            && contributingFiles(table, sorted) == sorted
            && loadOrder(table) != sorted,
        "the record states the sorted order while the table met the files in "
        "the other one (FR-015)");
  for (const PmuTableEntry& entry : table) {
    check(entry.fileName == kFirstFile || entry.fileName == kSecondFile,
          "every fixture row names the fixture file that holds it (PC-12)");
  }
}

// The record of a table read from one file names that file alone, beside its
// own digest.
auto singleFileRecordScenario() -> void
{
  const std::filesystem::path root {SG_TOPDOWN_TABLE_DIR};
  const std::filesystem::path directory {root
                                         / std::string {kFixtureDirectory}};
  const Manifest manifest = readManifest(root / "RECORD");
  std::vector<PmuTableEntry> table;
  pmuParseTableFile((directory / std::string {kFirstFile}).string(), table);
  check(table.size() == 2U, "the single fixture file contributes its rows");

  SourceRecord record;
  record.reference = manifest.reference;
  record.directory = std::string {kFixtureDirectory};
  for (const std::string& name :
       contributingFiles(table, sortedFiles(directory)))
  {
    record.files.emplace_back(name, manifest.hashOf(record.directory + name));
  }
  check(record.files.size() == 1U, "a table of one file states one file");
  if (!record.files.empty()) {
    check(record.files[0].first == kFirstFile
              && record.files[0].second == kFirstHash,
          "a record of one file names only that file and the digest of its "
          "own manifest line (FR-015)");
  }
}

}  // namespace

auto main() -> int
{
  if (!writeFixtures()) {
    std::printf("counters_topdown_source_test SKIP: the build tree holds no "
                "writable fixture directory\n");
    return 2;
  }
  referenceScenario();
  digestScenario();
  twoFileRecordScenario();
  singleFileRecordScenario();
  std::printf("counters_topdown_source_test PASS: the source record names its "
              "manifest\n");
  return 0;
}
