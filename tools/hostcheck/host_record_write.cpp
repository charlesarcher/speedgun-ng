// ============================================================================
// specs/031-host-validation T019: the writer program (FR-004, FR-007, FR-008,
// FR-012, FR-014, FR-037, R-04).
//
// The whole program is one call and one exit, the shape of its reader beside
// it. `main` parses nothing: the parse, the refusals, the refresh, the render
// and the one file stand in the library entry point the coverage gate measures,
// and the library reads no host of its own (R-04). What stands here is the seam
// that seam asks for, the one party allowed to reach the counters matcher: the
// identity `pmuIdentCurrent` answers, the key `mapfileKey` renders, the
// directory the mapfile of the root selects through
// `sg::tools::mapfileDirectory`, and the layout the standing topology read
// answers (FR-007, FR-008, FR-012, FR-014). The mapfile opens through
// `sg::tools::mapfileText`, the one open the two programs share, and its rows
// stand read whole: a row whose pattern the matcher cannot compile answers no
// directory, the throw staying inside the boundary that reads the tree
// (FR-004, FR-008, R-06).
// ============================================================================

#include <filesystem>
#include <utility>

#include "../source/harness/detail/host_record.hpp"

#include "../source/counters/detail/pmu.hpp"
#include "mapfile.hpp"

namespace
{

// The facts of one tree root, in the order the write path asks for them: the
// identity the host answers and the key the mapfile spells for it, the
// directory the rows of that root select, and the layout read once
// (FR-007, FR-008, FR-012, FR-014). A root whose mapfile no read opens, a
// root whose mapfile names no row, and a root whose rows name a pattern the
// matcher cannot compile each leave the directory unanswered, and an identity
// without a directory names no CPU: the refusal of FR-010 answers there, in the
// library that owns the lines.
auto hostFacts(const std::filesystem::path& treeRoot) -> sg::detail::HostFacts
{
  const auto pmu = sg::counters::detail::pmuIdentCurrent();
  auto ident = sg::detail::HostIdent {};
  ident.vendor = pmu.vendor;
  ident.family = pmu.family;
  ident.model = pmu.model;
  if (pmu.stepping >= 0) {
    ident.stepping = pmu.stepping;
  }
  ident.key = sg::counters::detail::mapfileKey(pmu);
  auto directory = std::string {};
  auto rowsWeighed = true;
  const auto bytes = sg::tools::mapfileText(treeRoot);
  if (bytes.has_value()) {
    const auto selected = sg::tools::mapfileDirectory(*bytes, pmu);
    rowsWeighed = selected.has_value();
    directory = selected.value_or(std::string {});
  } else {
    rowsWeighed = false;
  }
  ident.directory = std::move(directory);
  auto facts = sg::detail::HostFacts {};
  facts.ident = std::move(ident);
  facts.layout = sg::detail::layout();
  facts.mapfileReadable = rowsWeighed;
  return facts;
}

}  // namespace

auto main(int argc, char** argv) -> int
{
  return sg::detail::hostRecordWriteMain(argc, argv, &hostFacts);
}
