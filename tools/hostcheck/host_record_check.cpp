// ============================================================================
// specs/031-host-validation T026: the reader program (FR-024 to FR-030, FR-037,
// R-04, R-06, R-09).
//
// The whole program is one call and one exit. `main` parses nothing, so the
// parse, the schema under the tree root that parse names, the chooser built for
// that root and the lines the answer becomes all stand in the library entry
// point the coverage gate measures. This program hands over the chooser builder
// of its own header, the one party that may reach the catalog matcher, and it
// maps the answered status to the process exit the way the standing refusal
// shape leaves the exit to `main` (FR-028, R-09).
// ============================================================================

#include "../source/harness/detail/host_record_check.hpp"

#include "mapfile.hpp"

auto main(int argc, char** argv) -> int
{
  return sg::detail::hostRecordCheckMain(
      argc, argv, &sg::tools::makeDirectoryChooser);
}
