// Downstream consumer test project (FR-026, SC-005, SC-009).
// Driven by the downstream-consumer CI job: install speedgun-ng to a
// prefix, configure, build, and run this program against it. This
// file must never name the vendored topology library; that absence
// is the point.
//
// It uses the public API exactly as the README instructs: the
// exported class from <speedgun-ng/speedgun-ng.hpp> and the
// simulation_start marker from the trace-marker header. Constructing
// the class and calling name() pulls a real symbol out of the
// library, and the marker call resolves the trace-marker header from
// the install tree, so the link and that header's reachability are
// exercised on every machine.

#include <speedgun-ng/simulation.hpp>
#include <speedgun-ng/speedgun-ng.hpp>

auto main() -> int
{
  exported_class const library;
  sg::simulation_start();
  return library.name() == nullptr ? 1 : 0;
}
