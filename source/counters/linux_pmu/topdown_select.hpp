// ============================================================================
// The internal surface of the top-down table selector: the one helper that
// fills a `TopdownSource` from the embedded registry (specs/042-topdown
// FR-015, FR-033, FR-036; spec D-12; data-model E-02, E-03).
//
// The selector's answer, `topdownTable`, is public API in
// `speedgun-ng/counters_topdown.hpp`. The fill below is not. It reads the
// generated registry, and a test drives it with a directory name no host has
// to provide, which is how both of its arms reach a coverage run (FR-051).
//
// The seam is private: never installed, and read by exactly two places, the
// selector and that test.
// ============================================================================

#ifndef SG_COUNTERS_LINUX_PMU_TOPDOWN_SELECT_HPP
#define SG_COUNTERS_LINUX_PMU_TOPDOWN_SELECT_HPP

#ifdef __linux__
#  include "speedgun-ng/counters_topdown.hpp"

namespace sg::counters::detail
{

/// Fills the source record of the directory `source` already names, in place:
/// the reference the manifest states for the vendored tree, and one entry per
/// index entry that holds a row of the parsed table, each with the digest the
/// manifest line beside its name states. The embedder emitted the index in a
/// sorted order and the loader reads the directory in it, so the entries reach
/// the record in the embedder's order and a file that holds no row reaches no
/// entry (FR-015, FR-033, spec D-12).
///
/// A directory the registry holds no bytes for leaves `source` as it stands:
/// the reference stays empty and no file entry arrives. The mapping file names
/// only directories of the vendored tree, so every directory a host selects
/// stands in the registry of the archive this library links, and the sentence
/// above is the arm `test/source/counters_topdown_select_test.cpp` drives with
/// a name no mapping file carries (FR-036).
auto fillSourceRecord(TopdownSource& source) -> void;

}  // namespace sg::counters::detail
#endif  // __linux__

#endif  // SG_COUNTERS_LINUX_PMU_TOPDOWN_SELECT_HPP
