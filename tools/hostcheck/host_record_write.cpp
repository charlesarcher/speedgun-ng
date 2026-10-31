// ============================================================================
// specs/031-host-validation T019: the writer program (FR-004, FR-007, FR-008,
// FR-012, FR-014, FR-037, R-04).
//
// The whole program is one call and one exit, the shape of its reader beside
// it. `main` parses nothing: the parse, the refusals, the refresh, the render
// and the one file stand in the library entry point the coverage gate
// measures, and the library reads no host of its own (R-04). The host reads
// stand in the options struct `host_facts.hpp` carries: this program answers
// it with the host's own readers, the one party allowed to reach the counters
// matcher, and a test scripts the struct with its own answers, so a run of
// the tests reads no CPUID and no installed layout (FR-031, FR-032, FR-035).
// ============================================================================

#include "host_facts.hpp"

auto main(int argc, char** argv) -> int
{
  const sg::tools::HostFactsOptions reads {
      .pmuIdent = &sg::counters::detail::pmuIdentCurrent,
      .layout = &sg::detail::layout,
  };
  return sg::detail::hostRecordWriteMain(argc, argv,
                                         sg::tools::hostFactsSeam(reads));
}
