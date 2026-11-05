// ============================================================================
// The vendor grouping listing of the selected directory
// (specs/046-hunter-topdown T010; FR-004, FR-030, D-16;
// specs/046-hunter-topdown/contracts/ counters-grouping.md).
//
// `topdownEventGroups` answers the pairs the directory one identity selects
// holds: the bare name of every row beside the stem of the file that row stands
// in. The area mapper of specs/046 reads that listing as its fallback, so the
// stems are the vendor's own words and this unit pins them (FR-004, D-3).
//
// Every case reads the accessor beside the seam the listing comes from:
// `detail::pmuLoadTable` of `source/counters/detail/pmu.hpp`, the same seam
// `counters_topdown_select_test.cpp` reads. The seam answers the build-time
// data this build embedded, so a case states no fact about the cpu it runs on:
// the identity a case hands in decides the directory, and a Zen identity
// answers its own directory on this Intel host (FR-002 of specs/042-topdown).
//
// No case opens an event, samples a counter, reads a clock, opens a vendored
// path or names a device the host lacks (FR-030). A build that carries no
// vendored architecture directory answers no listing at all, so the binary
// reports its reason and exits 2, the skip code the `test/tier_labels.sh` row
// of T013 reserves. The accessor registers no metric host and plants no
// directory, so this binary answers 0 where `counters_topdown_select_test`
// answers 2 on an unregistered scripted host.
//
// Scenario map:
//   groups-carry-the-vendors-file-stems        the Skylake-X listing carries
//                                              the five named pairs and equals
//                                              the loaded rows, deduplicated by
//                                              first row, in load order
//                                              (FR-004, D-16)
//   accessor-reads-the-identitys-directory     the identity a case hands in
//                                              decides the listing, and an
//                                              identity that selects no
//                                              directory answers none (FR-004,
//                                              D-16)
//   zen-four-answers-the-amd-vendor-stems      the Zen 4 identity answers 575
//                                              pairs from the 577 rows of its
//                                              directory, reads two AMD stems
//                                              by name, and answers no pair of
//                                              the Intel-only stem of the
//                                              acceptance host (T091; FR-029,
//                                              D-3)
//   zen-five-answers-no-intel-file-stem        the Zen 5 identity answers its
//                                              own 12 files and 656 rows,
//                                              spells one shared row l2-cache,
//                                              and answers no pair of the stem
//                                              cache (T092; FR-029, D-3)
//   a-refused-directory-still-groups-its-rows  Zen 3 answers 256 pairs beside
//                                              the no-top-down-rows refusal its
//                                              tree answers, because a refusal
//                                              empties no listing (T093; D-16,
//                                              FR-005)
//   an-arm-identity-answers-no-pair            the `MIDR` beside four fields no
//                                              mapping-file row matches answers
//                                              an empty listing, and the tree
//                                              of that identity answers the Arm
//                                              refusal (T094; D-5, D-16)
//   a-handed-midr-changes-no-listing           the same `MIDR` beside the
//                                              Skylake-X four fields answers
//                                              the whole Skylake-X listing,
//                                              because the accessor reads no
//                                              `MIDR` (T094; D-16)
//   one-name-answers-one-pair                  one name reaches one pair, Arrow
//                                              Lake on the name its two core
//                                              scopes share and Zen 4 on two
//                                              metric names that repeat inside
//                                              one file (T095; D-16, FR-031)
//   one-call-reads-no-source-of-its-own        one call leaves the descriptors
//                                              and the mappings of this process
//                                              as it found them (FR-030)
// ============================================================================

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_set>
#include <vector>

#include "speedgun-ng/counters_topdown.hpp"

#include "../source/counters/detail/pmu.hpp"
#include "../source/counters/linux_pmu/embedded_tables.hpp"

namespace
{

using sg::counters::CpuIdentity;
using sg::counters::TopdownEventGroup;
using sg::counters::topdownEventGroups;
using sg::counters::TopdownSelection;
using sg::counters::topdownTree;
using sg::counters::TopdownUnsupport;
using sg::counters::detail::embeddedFindDir;
using sg::counters::detail::PmuIdent;
using sg::counters::detail::pmuLoadTable;
using sg::counters::detail::pmuSelectDirectory;
using sg::counters::detail::PmuTableEntry;

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "COUNTERS TOPDOWN GROUPS TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

// The identity an identity mapfile row matches no text of, so no directory
// stands behind any answer about it.
[[nodiscard]] auto unmatchedIdentity() -> CpuIdentity
{
  return CpuIdentity {
      .vendor = "NoSuchVendorXXXXX", .family = 999, .model = 999};
}

// One Intel identity. The mapping file spells a model in uppercase hex, so the
// decimal 85 is the row that reads `55`, and the stepping is the digit that row
// carries after the model (specs/007 FR-034).
[[nodiscard]] auto intelIdentity(const int family,
                                 const int model,
                                 const int stepping) -> CpuIdentity
{
  return CpuIdentity {
      .vendor = "GenuineIntel",
      .family = family,
      .model = model,
      .stepping = stepping,
  };
}

// One AMD identity of no stepping class: a mapping-file row that carries no
// stepping is matched against the model key alone (specs/007 FR-034).
[[nodiscard]] auto amdIdentity(const int family, const int model) -> CpuIdentity
{
  return CpuIdentity {
      .vendor = "AuthenticAMD", .family = family, .model = model};
}

// The `MIDR` a case hands in, spelled the way an Arm part publishes it. No
// production host answers it (specs/042-topdown D-13).
constexpr std::uint32_t kHandedMidr {0x410fd490U};

// The core class a hybrid table spells for its atom rows, the second field a
// case hands in beside the `MIDR`.
constexpr std::string_view kHandedCoreType {"cpu_atom"};

// The Arm identity of D-5: the `MIDR` a case hands in beside four fields no row
// of the mapping file matches, so the selection reads the `MIDR` and answers
// its refusal (FR-005, specs/042-topdown D-13).
[[nodiscard]] auto armIdentity() -> CpuIdentity
{
  return CpuIdentity {
      .vendor = "ARM",
      .family = 0,
      .model = 0,
      .stepping = -1,
      .coreType = std::string {kHandedCoreType},
      .midr = std::optional<std::uint32_t> {kHandedMidr},
  };
}

// One identity with the `MIDR` a case hands in beside the four fields the
// mapping file reads: the shape such a field gives an identity that stands
// matched (D-16, specs/042-topdown D-13).
[[nodiscard]] auto withHandedMidr(const CpuIdentity& identity) -> CpuIdentity
{
  auto handed = identity;
  handed.midr = std::optional<std::uint32_t> {kHandedMidr};
  return handed;
}

// The directory the mapping file selects for one identity, read through the
// seam the selection itself reads. The four fields build the whole key, so no
// `MIDR` and no core type stands in the answer (specs/007 FR-034, D-13).
[[nodiscard]] auto selectedDirectory(const CpuIdentity& identity) -> std::string
{
  return pmuSelectDirectory(PmuIdent {
      .vendor = identity.vendor,
      .family = identity.family,
      .model = identity.model,
      .stepping = identity.stepping,
  });
}

// The file stem a row groups under: the name of the file the loader tagged the
// row with, with its suffix dropped (FR-004, D-16).
[[nodiscard]] auto stemOf(const std::string& fileName) -> std::string
{
  return std::filesystem::path {fileName}.stem().string();
}

// The listing the seam answers for one directory, reduced the way the accessor
// reduces it: the first row that carries a name owns its pair, and a later row
// of that name adds nothing (D-16).
[[nodiscard]] auto loadedPairs(const std::string_view directory)
    -> std::vector<TopdownEventGroup>
{
  const std::string name {directory};
  const std::vector<PmuTableEntry>& rows = pmuLoadTable(name);
  std::vector<TopdownEventGroup> pairs;
  std::unordered_set<std::string> seen;
  for (const PmuTableEntry& row : rows) {
    if (seen.insert(row.name).second) {
      pairs.push_back(
          TopdownEventGroup {.event = row.name, .group = stemOf(row.fileName)});
    }
  }
  return pairs;
}

// The count of rows the seam holds for one directory.
[[nodiscard]] auto loadedRowCount(const std::string_view directory)
    -> std::size_t
{
  const std::string name {directory};
  return pmuLoadTable(name).size();
}

// One pair of a listing in the text a failure prints, so a wrong stem names
// itself.
[[nodiscard]] auto pairText(const TopdownEventGroup& pair) -> std::string
{
  return pair.event + " -> " + pair.group;
}

// Two listings compared pair by pair and in order, with the first difference
// printed. The record carries no comparison operator: it is data.
auto checkPairs(const std::vector<TopdownEventGroup>& actual,
                const std::vector<TopdownEventGroup>& expected,
                const char* what) -> void
{
  if (actual.size() != expected.size()) {
    std::fprintf(stderr,
                 "COUNTERS TOPDOWN GROUPS TEST FAIL: %s (%zu pairs answered, "
                 "%zu expected)\n",
                 what,
                 actual.size(),
                 expected.size());
    std::exit(1);
  }
  for (std::size_t index = 0; index != expected.size(); ++index) {
    if (actual[index].event != expected[index].event
        || actual[index].group != expected[index].group)
    {
      std::fprintf(stderr,
                   "COUNTERS TOPDOWN GROUPS TEST FAIL: %s (pair %zu reads "
                   "'%s', expected '%s')\n",
                   what,
                   index,
                   pairText(actual[index]).c_str(),
                   pairText(expected[index]).c_str());
      std::exit(1);
    }
  }
}

// The group the listing answers for one name, and an empty text where it holds
// no such pair.
[[nodiscard]] auto groupOf(const std::vector<TopdownEventGroup>& pairs,
                           const std::string_view name) -> std::string
{
  for (const TopdownEventGroup& pair : pairs) {
    if (pair.event == name) {
      return pair.group;
    }
  }
  return {};
}

// Whether any pair of a listing reads one group, so a case can state that one
// stem of a vendor's own file list stands in a listing or stands apart from it.
[[nodiscard]] auto holdsGroup(const std::vector<TopdownEventGroup>& pairs,
                              const std::string_view group) -> bool
{
  return std::ranges::any_of(pairs,
                             [group](const TopdownEventGroup& pair) -> bool
                             { return pair.group == group; });
}

// The count of pairs one name reaches. The accessor owns the first row of a
// repeated name, so a listing answers one at most (D-16).
[[nodiscard]] auto pairsForName(const std::vector<TopdownEventGroup>& pairs,
                                const std::string_view name) -> std::size_t
{
  std::size_t count = 0;
  for (const TopdownEventGroup& pair : pairs) {
    if (pair.event == name) {
      ++count;
    }
  }
  return count;
}

// Whether a listing holds one name twice. The accessor owns the first row of a
// repeated name, so no listing it answers holds one (D-16).
[[nodiscard]] auto holdsRepeatedName(
    const std::vector<TopdownEventGroup>& pairs) -> bool
{
  std::unordered_set<std::string> held;
  for (const TopdownEventGroup& pair : pairs) {
    if (!held.insert(pair.event).second) {
      return true;
    }
  }
  return false;
}

// The count of files the seam tagged the rows of one directory with, read as
// the stems the accessor answers.
[[nodiscard]] auto loadedFileCount(const std::string_view directory)
    -> std::size_t
{
  const std::string name {directory};
  std::unordered_set<std::string> files;
  for (const PmuTableEntry& row : pmuLoadTable(name)) {
    files.insert(stemOf(row.fileName));
  }
  return files.size();
}

// The kind one identity's ask refuses with, and the failure of a case that
// asked a refusal and got a tree (FR-005).
[[nodiscard]] auto askedRefusalKind(const CpuIdentity& identity)
    -> TopdownUnsupport
{
  auto selection = TopdownSelection {};
  selection.identity = identity;
  selection.level = 6;
  const auto answered = topdownTree(selection);
  if (answered.has_value()) {
    std::fprintf(stderr,
                 "COUNTERS TOPDOWN GROUPS TEST FAIL: the ask answered a tree "
                 "where its case asked a refusal\n");
    std::exit(1);
  }
  return answered.error().kind;
}

// The descriptors this process holds, counted through its own descriptor
// directory. An event the counters library opens hands back a descriptor, so a
// call that left one open moves this listing.
[[nodiscard]] auto openDescriptorCount() -> std::size_t
{
  std::error_code failure;
  std::size_t count = 0;
  for (const auto& entry :
       std::filesystem::directory_iterator("/proc/self/fd", failure))
  {
    static_cast<void>(entry);
    ++count;
  }
  if (failure) {
    fail("the process's own descriptor listing is readable");
  }
  return count;
}

// The mappings this process holds, counted through its own mapping listing. A
// sampled event maps a page, so a call that sampled a counter moves this
// listing.
//
// GCC answers `__SANITIZE_ADDRESS__` and `__SANITIZE_THREAD__`; Clang answers
// `__has_feature`. The allocator of a sanitizer maps its pages as it needs
// them, so the mapping count of such a build moves on calls that read nothing.
// The mapping claim of the case below stands where that count is the proxy for
// a page a call maps.
#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
#  define SG_TEST_TRACKS_MAPPINGS 0
#elif defined(__has_feature)
#  if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
#    define SG_TEST_TRACKS_MAPPINGS 0
#  else
#    define SG_TEST_TRACKS_MAPPINGS 1
#  endif
#else
#  define SG_TEST_TRACKS_MAPPINGS 1
#endif
[[maybe_unused]] [[nodiscard]] auto mappingCount() -> std::size_t
{
  std::ifstream maps("/proc/self/maps");
  if (!maps) {
    fail("the process's own mapping listing is readable");
  }
  std::size_t count = 0;
  for (std::string line; std::getline(maps, line);) {
    ++count;
  }
  return count;
}

// The Skylake-X directory, spelled the way the selection answers it.
constexpr std::string_view kSkylakexDirectory {"arch/x86/skylakex/"};

// The Zen 4 directory, the directory of the identity a case hands in on this
// Intel host.
constexpr std::string_view kZen4Directory {"arch/x86/amdzen4/"};

// The Zen 5 directory, the directory the family-26 model-17 identity selects.
constexpr std::string_view kZen5Directory {"arch/x86/amdzen5/"};

// The Zen 3 directory, the directory that holds no row of any top-down group,
// so its identity answers a tree refusal beside a full listing (FR-005).
constexpr std::string_view kZen3Directory {"arch/x86/amdzen3/"};

// The Arrow Lake directory, the hybrid directory that repeats one name under a
// second `Unit` device (D-16, FR-031).
constexpr std::string_view kArrowlakeDirectory {"arch/x86/arrowlake/"};

// FR-004 and D-16: the listing carries the vendor's own file stems, one pair
// per name, in the order the directory loads.
auto groupsCarryTheVendorsFileStems() -> int
{
  // The Skylake-X identity of the acceptance host: the model-85 stepping-4
  // part (D-14).
  const CpuIdentity skylakex = intelIdentity(6, 85, 4);
  const std::vector<TopdownEventGroup> pairs = topdownEventGroups(skylakex);
  check(!pairs.empty(), "the Skylake-X identity answers a listing (FR-004)");
  check(groupOf(pairs, "LONGEST_LAT_CACHE.MISS") == "cache",
        "the cache row answers the stem cache, the vendor's own file name "
        "(FR-004, D-3)");
  check(groupOf(pairs, "BR_INST_RETIRED.ALL_BRANCHES") == "pipeline",
        "the branch row answers the stem pipeline, the file that holds its row "
        "(FR-004, D-3)");
  check(groupOf(pairs, "MEM_TRANS_RETIRED.LOAD_LATENCY_GT_4") == "memory",
        "the load-latency row answers the stem memory (FR-004, D-3)");
  check(groupOf(pairs, "UNC_CHA_AG0_AD_CRD_ACQUIRED.TGR0") == "uncore-cache",
        "an uncore row answers the stem of its own uncore file, the group the "
        "mapper of specs/046 reads for it (FR-004, D-16)");
  check(groupOf(pairs, "tma_backend_bound") == "skx-metrics",
        "a metric row reaches the listing beside the event rows, the arm D-16 "
        "states for every row of the directory (FR-004, ruling R4)");

  // The same listing, read through the seam it comes from: every row of the
  // directory reaches one pair, the first row of a repeated name owns that
  // pair, and the order is the load order (D-16).
  checkPairs(pairs,
             loadedPairs(kSkylakexDirectory),
             "the listing equals the stems of the loaded rows, deduplicated by "
             "first row, in load order (FR-004, D-16)");
  check(loadedRowCount(kSkylakexDirectory) == pairs.size(),
        "the directory holds no repeated name, so every one of its rows owns a "
        "pair of its own here (D-16)");
  return 0;
}

// FR-004 and D-16: the identity a case hands in decides the listing, and an
// identity that selects no directory answers none.
auto accessorReadsTheIdentitysDirectory() -> int
{
  const std::vector<TopdownEventGroup> intel =
      topdownEventGroups(intelIdentity(6, 85, 4));
  const std::vector<TopdownEventGroup> zen4 =
      topdownEventGroups(amdIdentity(25, 17));
  checkPairs(intel,
             loadedPairs(kSkylakexDirectory),
             "the Skylake-X identity answers the listing of the directory that "
             "identity selects (FR-004, D-16)");
  checkPairs(zen4,
             loadedPairs(kZen4Directory),
             "the Zen 4 identity answers the listing of its own directory on "
             "this Intel host, so the answer follows the identity handed in "
             "(FR-004, FR-002 of specs/042-topdown)");
  check(groupOf(intel, "tma_backend_bound") == "skx-metrics"
            && groupOf(zen4, "tma_backend_bound").empty(),
        "the Intel-only stem stands in the Intel listing alone, so the two "
        "answers are two directories and not one (D-3, AP-7)");
  check(!zen4.empty(),
        "the Zen 4 directory answers a listing of its own (D-16)");

  const std::vector<TopdownEventGroup> none =
      topdownEventGroups(unmatchedIdentity());
  check(none.empty(),
        "an identity no row of the mapping file matches selects no directory "
        "and answers an empty listing, so every event of such a run reads "
        "unmapped (D-16, D-5)");
  return 0;
}

// FR-029 and D-3: the Zen 4 identity answers the AMD stems of its own directory
// on this Intel host. The Intel-only stem of the acceptance host stands in no
// pair of that listing.
auto zenFourAnswersTheAmdVendorStems() -> int
{
  const CpuIdentity zen4 = amdIdentity(25, 17);
  check(selectedDirectory(zen4) == std::string {kZen4Directory},
        "the family-25 model-17 identity of no stepping selects the Zen 4 "
        "directory (FR-029, specs/007 FR-034)");

  const std::vector<TopdownEventGroup> pairs = topdownEventGroups(zen4);
  checkPairs(pairs,
             loadedPairs(kZen4Directory),
             "the Zen 4 listing equals the stems of the loaded rows, "
             "deduplicated " "by first row, in load order (FR-004, D-16)");
  check(loadedRowCount(kZen4Directory) == 577,
        "the pinned Zen 4 directory holds 577 rows (FR-029, D-16)");
  check(pairs.size() == 575,
        "575 pairs answer those 577 rows, because two metric names stand twice "
        "inside one file of that directory (D-16, FR-031)");

  // Two AMD stems the Intel listings answer under another name: the vendor
  // directory holds the word, and the accessor moves none of it (D-3, AP-7).
  check(groupOf(pairs, "l2_request_g1.group2") == "cache",
        "the Zen 4 file that holds the shared L2 request row spells its stem "
        "cache (D-3, AP-7)");
  check(groupOf(pairs, "backend_bound_cpu") == "pipeline",
        "the AMD level-2 pipeline row reads the stem pipeline, the vendor's "
        "own " "file name (FR-004, D-3)");

  // The Intel-only stem of the acceptance host: `skx-metrics` is the file no
  // directory of AMD carries, so no pair of a Zen 4 listing can read it.
  check(!holdsGroup(pairs, "skx-metrics")
            && groupOf(pairs, "tma_backend_bound").empty(),
        "the Intel-only stem of the acceptance host stands in no Zen 4 pair, "
        "because no row of that directory carries the metric it stems from "
        "(FR-029, D-3)");
  return 0;
}

// FR-029 and D-3: the Zen 5 identity answers its own directory, spells one
// shared row under the Zen 5 file name, and answers no Intel `cache` stem at
// all.
auto zenFiveAnswersNoIntelFileStem() -> int
{
  const CpuIdentity zen5 = amdIdentity(26, 17);
  check(selectedDirectory(zen5) == std::string {kZen5Directory},
        "the family-26 model-17 identity of no stepping selects the Zen 5 "
        "directory (FR-029, specs/007 FR-034)");

  const std::vector<TopdownEventGroup> pairs = topdownEventGroups(zen5);
  checkPairs(pairs,
             loadedPairs(kZen5Directory),
             "the Zen 5 listing equals the stems of the loaded rows, "
             "deduplicated " "by first row, in load order (FR-004, D-16)");
  check(loadedRowCount(kZen5Directory) == 656,
        "the pinned Zen 5 directory holds 656 rows (FR-029, D-16)");
  check(pairs.size() == 656,
        "every row of that directory owns a pair, because it holds no repeated "
        "name (D-16)");
  check(loadedFileCount(kZen5Directory) == 12,
        "12 files hold those rows, and no file of them stems as cache (D-3)");

  // One name, two directories: Zen 4 holds this row inside `cache` and Zen 5
  // holds it inside `l2-cache`, so the listing answers each vendor's own word.
  check(groupOf(pairs, "l2_request_g1.group2") == "l2-cache",
        "the row that reads cache in the Zen 4 directory reads l2-cache here, "
        "so " "the vocabulary belongs to the directory (FR-004, D-3, AP-7)");
  check(!holdsGroup(pairs, "cache"),
        "no pair reads the stem cache, because that directory carries no file "
        "of " "that name (D-3, FR-029)");
  return 0;
}

// D-16 and FR-005: a tree refusal empties no listing. The Zen 3 directory holds
// no row of any top-down group, so the tree refuses it while the accessor
// answers every one of its rows.
auto aRefusedDirectoryStillGroupsItsRows() -> int
{
  const CpuIdentity zen3 = amdIdentity(25, 1);
  check(selectedDirectory(zen3) == std::string {kZen3Directory},
        "the family-25 model-1 identity of no stepping selects the Zen 3 "
        "directory (specs/007 FR-034)");
  check(askedRefusalKind(zen3) == TopdownUnsupport::NO_TOPDOWN_ROWS,
        "the tree ask of that identity answers the no-top-down-rows refusal, "
        "because no row of the directory carries a level token (specs/042-topdown "
        "FR-006, FR-005)");

  const std::vector<TopdownEventGroup> pairs = topdownEventGroups(zen3);
  check(pairs.size() == 256,
        "the refused directory still answers 256 pairs, because a refusal is a "
        "tree answer and no listing answer (D-16, FR-004)");
  checkPairs(pairs,
             loadedPairs(kZen3Directory),
             "the listing of a refused directory equals the stems of its "
             "loaded " "rows in load order (D-16)");
  return 0;
}

// D-5 and D-16: an identity that selects no directory answers an empty listing,
// so every event of such a run reads the unmapped word of the mapper.
auto anArmIdentityAnswersNoPair() -> int
{
  const CpuIdentity arm = armIdentity();
  check(selectedDirectory(arm).empty(),
        "no row of the mapping file names a directory for the four fields of "
        "an " "Arm identity (D-5, specs/042-topdown D-13)");
  check(topdownEventGroups(arm).empty(),
        "an identity that selects no directory answers an empty listing, so "
        "the " "mapper reads no group for it (D-16, FR-005)");
  check(askedRefusalKind(arm) == TopdownUnsupport::ARM,
        "the tree answers its Arm refusal for the same identity, which is "
        "where " "the mapper reads unmapped (FR-005, D-5)");
  return 0;
}

// D-16 and ruling R2: the mapping file reads the four fields alone, so a
// handed-in `MIDR` moves no pair of the listing a matched identity answers.
auto aHandedMidrChangesNoListing() -> int
{
  const CpuIdentity skylakex = intelIdentity(6, 85, 4);
  const std::vector<TopdownEventGroup> plain = topdownEventGroups(skylakex);
  const std::vector<TopdownEventGroup> handed =
      topdownEventGroups(withHandedMidr(skylakex));
  checkPairs(handed,
             loadedPairs(kSkylakexDirectory),
             "the same `MIDR` beside the Skylake-X four fields answers the "
             "whole " "Skylake-X listing (D-16, ruling R2)");
  checkPairs(handed,
             plain,
             "one listing stands unchanged under a handed-in `MIDR`, because no "
             "row of the mapping file reads either handed field (D-16, D-13)");
  check(askedRefusalKind(withHandedMidr(skylakex)) == TopdownUnsupport::ARM,
        "the tree reads the `MIDR` the accessor leaves alone, so one field "
        "splits " "the two answers (specs/042-topdown FR-007, D-13)");
  return 0;
}

// D-16 and FR-031: one name reaches one pair, owned by the row that came first.
// The hybrid shape and the AMD shape reach that arm through two files that
// repeat a name inside themselves.
auto oneNameAnswersOnePair() -> int
{
  const CpuIdentity arrowlake = intelIdentity(6, 198, 0);
  check(selectedDirectory(arrowlake) == std::string {kArrowlakeDirectory},
        "the family-6 model-198 identity selects the Arrow Lake directory "
        "(specs/007 FR-034)");

  const std::vector<TopdownEventGroup> pairs = topdownEventGroups(arrowlake);
  check(loadedRowCount(kArrowlakeDirectory) == 1166,
        "the pinned Arrow Lake directory holds 1166 rows (FR-031, D-16)");
  check(pairs.size() == 919,
        "919 pairs answer those 1166 rows, because 196 names stand in more "
        "than " "one row there (D-16)");
  check(groupOf(pairs, "DL1.DIRTY_EVICTION") == "cache",
        "the name the two core scopes share reads the stem of the file that "
        "holds " "both rows (D-16, FR-004)");
  check(pairsForName(pairs, "DL1.DIRTY_EVICTION") == 1,
        "the first row of that name owns its pair, so the listing answers it "
        "once " "(D-16, FR-031)");
  check(!holdsRepeatedName(pairs),
        "no name of the 919 pairs stands twice, so the answer holds one pair "
        "per " "name (D-16)");
  checkPairs(pairs,
             loadedPairs(kArrowlakeDirectory),
             "the listing equals the stems of the loaded rows, the first row "
             "of a " "repeated name owning its pair (FR-004, D-16)");

  // The same arm without a second `Unit` device: the Zen 4 file repeats two
  // metric names inside itself, so the dedup arm needs no hybrid host.
  const std::vector<TopdownEventGroup> zen4 =
      topdownEventGroups(amdIdentity(25, 17));
  check(groupOf(zen4, "umc_cas_cmd_rate") == "recommended",
        "the Zen 4 metric row that repeats inside one file reads the stem "
        "recommended (FR-031, D-16)");
  check(pairsForName(zen4, "umc_cas_cmd_rate") == 1
            && pairsForName(zen4, "umc_cas_cmd_read_ratio") == 1,
        "two metric names that repeat inside one file answer one pair each, so "
        "no " "hybrid host stands in this arm (FR-031)");
  return 0;
}

// FR-030 and D-16: one call answers from the data this build embedded, and
// nothing else moves.
auto oneCallReadsNoSourceOfItsOwn() -> int
{
  const std::size_t descriptors = openDescriptorCount();
#if SG_TEST_TRACKS_MAPPINGS
  const std::size_t mappings = mappingCount();
#endif
  const CpuIdentity skylakex = intelIdentity(6, 85, 4);
  const std::vector<TopdownEventGroup> pairs = topdownEventGroups(skylakex);
  check(openDescriptorCount() == descriptors,
        "one call leaves the process holding the descriptors it held before, "
        "so no event stands open behind the listing (FR-030, D-16)");
#if SG_TEST_TRACKS_MAPPINGS
  check(mappingCount() == mappings,
        "one call leaves the process holding the mappings it held before, so "
        "no sampled event page stands behind the listing (FR-030, D-16)");
#endif
  check(groupOf(topdownEventGroups(skylakex), "LONGEST_LAT_CACHE.MISS")
            == groupOf(pairs, "LONGEST_LAT_CACHE.MISS"),
        "two calls on one identity answer one listing, so nothing that moves "
        "between calls stands in it (D-16)");
  return 0;
}

struct Scenario
{
  const char* name;
  int (*run)();
};

constexpr std::array<Scenario, 9> kScenarios {{
    Scenario {"groups-carry-the-vendors-file-stems",
              &groupsCarryTheVendorsFileStems},
    Scenario {"accessor-reads-the-identitys-directory",
              &accessorReadsTheIdentitysDirectory},
    Scenario {"zen-four-answers-the-amd-vendor-stems",
              &zenFourAnswersTheAmdVendorStems},
    Scenario {"zen-five-answers-no-intel-file-stem",
              &zenFiveAnswersNoIntelFileStem},
    Scenario {"a-refused-directory-still-groups-its-rows",
              &aRefusedDirectoryStillGroupsItsRows},
    Scenario {"an-arm-identity-answers-no-pair", &anArmIdentityAnswersNoPair},
    Scenario {"a-handed-midr-changes-no-listing", &aHandedMidrChangesNoListing},
    Scenario {"one-name-answers-one-pair", &oneNameAnswersOnePair},
    Scenario {"one-call-reads-no-source-of-its-own",
              &oneCallReadsNoSourceOfItsOwn},
}};

}  // namespace

auto main(int argc, char** argv) -> int
{
  // The listing comes out of the bytes this build embedded. A build that
  // carries no architecture directory answers no directory at all, which leaves
  // this unit nothing to measure, so it reports the reason and exits 2: the
  // skip code the `test/tier_labels.sh` row reserves. The accessor registers no
  // scripted host, so no second skip arm stands here (FR-036 of
  // specs/042-topdown).
  if (embeddedFindDir("arch/x86") == nullptr) {
    std::printf("counters_topdown_groups_test SKIP: this build carries no "
                "vendored architecture mapping file, so an identity answers no "
                "directory (FR-004)\n");
    return 2;
  }
  // `argv[1]` selects one scenario, so one arm runs alone. No argument runs the
  // whole table.
  const std::string requested =
      argc > 1 && argv[1] != nullptr ? argv[1] : std::string();
  for (const Scenario& scenario : kScenarios) {
    if (requested == scenario.name) {
      return scenario.run();
    }
  }
  if (!requested.empty()) {
    std::fprintf(stderr,
                 "counters_topdown_groups_test: unknown scenario '%s'\n",
                 requested.c_str());
    return 2;
  }
  for (const Scenario& scenario : kScenarios) {
    const int verdict = scenario.run();
    if (verdict != 0) {
      return verdict;
    }
  }
  std::printf("counters_topdown_groups_test PASS: the accessor answers the "
              "file stems of the directory the identity it is handed selects, "
              "one pair per name and in load order\n");
  return 0;
}
