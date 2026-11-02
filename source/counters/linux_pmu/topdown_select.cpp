// ============================================================================
// The identity a top-down selection reads and the table it selects
// (specs/042-topdown T022; FR-001, FR-002, FR-003; data-model E-01 to E-05).
//
// Two calls live here, each over a seam of the private pmu header. The
// identity copies the four fields the library's own cpu reader answers and
// leaves the two fields no reader answers absent, because a reader for the
// core type of a hybrid part or for an Arm `MIDR` stands nowhere in this
// feature (spec D-13). The table hands the identity's four production fields
// to the same mapping-file selection the catalog uses, so an answer about
// another part comes out of the identity the caller handed in and never out of
// the cpu this process runs on (FR-002).
//
// Both calls read the bytes this build embedded: the mapping file is the
// arch/x86 directory's own file there, so the selection opens no path
// (specs/012 FR-036, spec D-12). The rows of a directory, their verdicts and
// the digests of the files they came from reach a listing through the metric
// layer, which resolves a row against the leaves of the open system. Neither
// call here opens a system, because an availability probe opens an event and
// the contract of the selection bars one (FR-002).
// ============================================================================

#include <string>

#include "../detail/pmu.hpp"
#include "speedgun-ng/counters_topdown.hpp"
#include "speedgun-ng/dbc.hpp"

namespace sg::counters
{

auto cpuIdentity() -> CpuIdentity
{
#ifdef __linux__
  const detail::PmuIdent read = detail::pmuIdentCurrent();
  CpuIdentity identity;
  identity.vendor = read.vendor;
  identity.family = read.family;
  identity.model = read.model;
  identity.stepping = read.stepping;
  SG_ENSURE(identity.vendor == read.vendor && identity.family == read.family
                && identity.model == read.model
                && identity.stepping == read.stepping
                && identity.coreType.empty() && !identity.midr.has_value(),
            "the four fields carry the reader's own answer and the two fields "
            "no reader answers stand absent (FR-003, spec D-13)");
  return identity;
#else
  // The reduced catalog of a platform that publishes no event table carries no
  // cpu to identify, so the identity names an unreadable cpu: no vendor text,
  // no family, no model and no stepping class (specs/007 FR-042).
  return {};
#endif
}

auto topdownTable(const CpuIdentity& identity) -> TopdownTable
{
  TopdownTable table;
#ifdef __linux__
  // The mapping file matches the vendor, the family, the model and the
  // stepping, and no row of it reads a core type or an `MIDR`, so the four
  // fields build the whole key (spec D-13).
  const detail::PmuIdent ident {
      .vendor = identity.vendor,
      .family = identity.family,
      .model = identity.model,
      .stepping = identity.stepping,
  };
  table.source.directory = detail::pmuSelectDirectory(ident);
#endif
  SG_ENSURE(!table.source.directory.empty() || table.rows.empty(),
            "an identity the mapping file matched no row for names no "
            "directory and answers no row (FR-001)");
  return table;
}

}  // namespace sg::counters
