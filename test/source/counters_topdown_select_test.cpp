// ============================================================================
// The identity a selection reads and the table it selects (specs/042-topdown
// T010; FR-001, FR-002, FR-003, FR-009, FR-048; data-model E-01 to E-05;
// specs/042-topdown/contracts/topdown-selection.md).
//
// `cpuIdentity()` answers the four fields `pmuIdentCurrent()` reads, and it
// leaves the core type and the `MIDR` absent, because no reader for either
// stands in this feature (FR-003, spec D-13). A test hands both in, and the
// record carries what it was handed.
//
// `topdownTable()` answers the directory the vendored mapping file gives the
// identity it is handed. Three directories stand in one table each: the
// model-85 stepping-4 part the acceptance host is, the family-25 model-17
// part and the family-26 model-17 part. The two AMD identities are read on
// this Intel host, so an answer that names an AMD table can only come from the
// identity handed in: the selection reads no cpu of its own (FR-002).
//
// The directory labels are the fixture's own, from
// `test/source/detail/topdown_fixture.hpp`, the header T009 wrote, so the
// labels one unit reads and the labels the next unit plants rows under stand in
// one place. No mapping-file row moves, and no host stands in for another.
//
// The two readers a case compares against are the seams the contract names:
// `pmuIdentCurrent()` and `pmuSelectDirectory()`
// (`source/counters/detail/pmu.hpp`).
//
// Frameworkless check()/fail() convention.
// ============================================================================

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

#include "speedgun-ng/counters_topdown.hpp"

#include "../source/counters/detail/pmu.hpp"
#include "../source/counters/linux_pmu/embedded_tables.hpp"
#include "detail/topdown_fixture.hpp"

namespace
{

auto fail(const char* what) -> void
{
  std::fprintf(stderr, "COUNTERS TOPDOWN SELECT TEST FAIL: %s\n", what);
  std::exit(1);
}

auto check(const bool cond, const char* what) -> void
{
  if (!cond) {
    fail(what);
  }
}

using sg::counters::cpuIdentity;
using sg::counters::CpuIdentity;
using sg::counters::TableFile;
using sg::counters::topdownTable;
using sg::counters::TopdownTable;
using sg::counters::detail::embeddedFindDir;
using sg::counters::detail::PmuIdent;
using sg::counters::detail::pmuIdentCurrent;
using sg::counters::detail::pmuSelectDirectory;
using sg::test::kIntelDirectory;
using sg::test::kZen4Directory;
using sg::test::kZen5Directory;

// The `MIDR` a case hands in, spelled the way an Arm part publishes it: the
// implementer, the variant, the architecture, the part number and the revision
// in one 32-bit word. No production host answers it (spec D-13).
constexpr std::uint32_t kHandedMidr {0x410fd490U};

// The core type a case hands in, spelled the way a hybrid table spells the
// class its atom rows carry.
constexpr std::string_view kHandedCoreType {"cpu_atom"};

// The identity an identity mapfile row matches no text of, so the selection
// answers no directory at all.
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
// stepping is matched against the model key alone, so the field stays at its
// absent value (specs/007 FR-034).
[[nodiscard]] auto amdIdentity(const int family, const int model) -> CpuIdentity
{
  return CpuIdentity {
      .vendor = "AuthenticAMD", .family = family, .model = model};
}

// The whole source record as one text. The published record carries no
// comparison operator, so two answers of one identity compare field by field
// through this text.
[[nodiscard]] auto sourceText(const TopdownTable& table) -> std::string
{
  std::string text {table.source.reference};
  text += '\n';
  text += table.source.directory;
  for (const TableFile& file : table.source.files) {
    text += '\n';
    text += file.name;
    text += ' ';
    text += file.manifestHash;
  }
  return text;
}

// The descriptors this process holds, counted through its own descriptor
// directory. An event the counters library opens hands back a descriptor, so a
// call that left one open moves this listing. `counters_pmu_test.cpp` reads
// the same listing for the same claim.
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
[[nodiscard]] auto mappingCount() -> std::size_t
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

// FR-003 and spec D-13: the four fields production reads, the two fields no
// reader fills, and the two fields a test hands in.
auto cpuIdentityCarriesTheReadersFourFields() -> void
{
  const CpuIdentity published = cpuIdentity();
  const PmuIdent read = pmuIdentCurrent();
  check(published.vendor == read.vendor,
        "the vendor text is the one the library's own cpu reader answers "
        "(FR-003)");
  check(published.family == read.family,
        "the family is the reader's own answer, in the mapping file's decimal "
        "spelling (FR-003)");
  check(published.model == read.model,
        "the model is the reader's own answer, in the mapping file's spelling "
        "(FR-003)");
  check(published.stepping == read.stepping,
        "the stepping is the reader's own answer, and a negative value names "
        "no stepping class (FR-003)");
  check(published.coreType.empty(),
        "no reader fills the core type of a hybrid part, so a production "
        "identity leaves it empty (FR-003, spec D-13)");
  check(!published.midr.has_value(),
        "no reader fills an Arm `MIDR`, so a production identity leaves it "
        "unset (FR-003, spec D-13)");

  const CpuIdentity handed = CpuIdentity {
      .vendor = "ARM",
      .family = 0,
      .model = 0,
      .stepping = -1,
      .coreType = std::string {kHandedCoreType},
      .midr = std::optional<std::uint32_t> {kHandedMidr},
  };
  check(handed.coreType == kHandedCoreType,
        "the record carries the core type a test hands in (FR-003)");
  check(handed.midr.has_value() && *handed.midr == kHandedMidr,
        "the record carries the `MIDR` a test hands in (FR-003)");
}

// FR-001 and FR-003: the identity is the whole input to the selection, and the
// first mapping-file row that hits names the directory.
auto selectsTheTableOfTheIdentity() -> void
{
  check(topdownTable(intelIdentity(6, 85, 4)).source.directory
            == kIntelDirectory,
        "the model-85 stepping-4 part selects the Skylake-X table, the row the "
        "mapping file splits from the stepping-5 Cascade Lake row of the same "
        "model (FR-001)");
  check(topdownTable(amdIdentity(25, 17)).source.directory == kZen4Directory,
        "the family-25 model-17 part selects the Zen 4 pipeline table, the "
        "first of the two rows that model hits (FR-001)");
  check(topdownTable(amdIdentity(26, 17)).source.directory == kZen5Directory,
        "the family-26 model-17 part selects the Zen 5 pipeline table, ahead "
        "of the Zen 6 row that model also hits (FR-001)");

  const TopdownTable unmatched = topdownTable(unmatchedIdentity());
  check(unmatched.source.directory.empty(),
        "an identity no row of the mapping file matches selects no directory, "
        "and an empty directory names that answer (FR-001)");
  check(unmatched.rows.empty(),
        "an identity the mapping file matched no row for answers no row "
        "(FR-001)");

  // The identity the library's own reader answers selects the table its own
  // four fields match, so the record a test hands in and the record the reader
  // answers reach one directory (FR-003).
  check(topdownTable(cpuIdentity()).source.directory
            == pmuSelectDirectory(pmuIdentCurrent()),
        "the identity the library's own reader answers selects the directory "
        "that reader's four fields match (FR-003)");

  // The two fields no reader answers carry no weight in a selection: the
  // mapping file matches the vendor, the family, the model and the stepping,
  // and no row of it reads a core type or an `MIDR` (spec D-13).
  CpuIdentity hybrid = intelIdentity(6, 85, 4);
  hybrid.coreType = std::string {kHandedCoreType};
  hybrid.midr = std::optional<std::uint32_t> {kHandedMidr};
  check(topdownTable(hybrid).source.directory == kIntelDirectory,
        "a core type and a `MIDR` a test hands in move no selection, because "
        "no mapping-file row reads either (FR-003, spec D-13)");
}

// FR-002 and FR-009: one call answers from what it is handed, and nothing
// else moves the answer.
auto oneCallReadsNoSourceOfItsOwn() -> void
{
  const std::size_t descriptors = openDescriptorCount();
  const std::size_t mappings = mappingCount();
  const CpuIdentity intel = intelIdentity(6, 85, 4);
  const CpuIdentity amd = amdIdentity(25, 17);
  const TopdownTable table = topdownTable(amd);

  check(openDescriptorCount() == descriptors,
        "one selection leaves the process holding the descriptors it held "
        "before, so no event stands open behind the answer (FR-002)");
  check(mappingCount() == mappings,
        "one selection leaves the process holding the mappings it held before, "
        "so no sampled event page stands behind the answer (FR-002)");

  // Two identities that differ answer two directories. A call that read this
  // cpu of its own would answer one directory for both, whatever it was handed
  // (FR-002).
  check(table.source.directory == kZen4Directory,
        "a Zen 4 identity answers the Zen 4 table on this host (FR-002)");
  check(topdownTable(intel).source.directory == kIntelDirectory,
        "an Intel identity answers the Intel table in the same process, so the "
        "answer follows the identity and the cpu of this host entered nothing "
        "(FR-002)");

  // One answer for one input: a figure that moves between calls, a counter or
  // a clock, would part two calls over one identity (FR-009).
  check(sourceText(topdownTable(amd)) == sourceText(table),
        "two selections of one identity answer one source record, so nothing "
        "that moves between calls stands in it (FR-009)");
}

}  // namespace

auto main() -> int
{
  // The selection reads the mapping file out of the bytes this build
  // embedded. A build that carries no architecture directory answers no
  // selection at all, which leaves this unit nothing to measure, so it reports
  // the reason and exits 2, the skip code the `test/tier_labels.sh` row of T001
  // reserves.
  if (embeddedFindDir("arch/x86") == nullptr) {
    std::printf("counters_topdown_select_test SKIP: this build carries no "
                "vendored architecture mapping file, so a selection answers no "
                "directory (FR-001)\n");
    return 2;
  }
  cpuIdentityCarriesTheReadersFourFields();
  selectsTheTableOfTheIdentity();
  oneCallReadsNoSourceOfItsOwn();
  std::printf("counters_topdown_select_test PASS: the selection answers the "
              "table of the identity it is handed\n");
  return 0;
}
