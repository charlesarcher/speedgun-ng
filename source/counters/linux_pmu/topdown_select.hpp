// ============================================================================
// The internal surface of the top-down table selector: the helper that fills a
// `TopdownSource` from the embedded registry, and the two readbacks of the
// postcondition of `topdownTree` (specs/042-topdown FR-008, FR-011, FR-013,
// FR-015, FR-016, FR-020, FR-033, FR-036; spec D-12; data-model E-02, E-03).
//
// The selector's answer, `topdownTable`, is public API in
// `speedgun-ng/counters_topdown.hpp`. The three calls below are not. The fill
// reads the generated registry, and a test drives it with a directory name no
// host has to provide, which is how both of its arms reach a coverage run
// (FR-051). Each readback states one clause of the postcondition the selection
// answers, and the pass that builds a tree gives each clause by construction,
// so a test holds each false return with a tree it builds by hand (FR-051).
//
// The seam is private: never installed, and read by exactly two places, the
// selector and that test.
// ============================================================================

#ifndef SG_COUNTERS_LINUX_PMU_TOPDOWN_SELECT_HPP
#define SG_COUNTERS_LINUX_PMU_TOPDOWN_SELECT_HPP

#include "speedgun-ng/counters_topdown.hpp"

namespace sg::counters::detail
{

#ifdef __linux__

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
#endif  // __linux__

/// Whether the shape the postcondition of `topdownTree` states stands over
/// `built`: every node stands at a level the answer reaches, between one and
/// `built.depth`, and a node that names a parent names an element of the
/// vector that stands earlier and one level above it (FR-008, FR-013, FR-020).
///
/// The pass that builds the vector gives each clause by construction, so the
/// false return of each one stands where
/// `test/source/counters_topdown_select_test.cpp` holds it with a tree the
/// test builds by hand (FR-051).
[[nodiscard]] auto topdownShapeHolds(const TopdownTree& built) -> bool;

/// Whether every node of `built` names a row that `table` published: the first
/// row of the vector that carries the node's own `vendorName` stands with its
/// `published` flag, and a name the table holds no row for answers false
/// (FR-011, FR-016).
///
/// The pass adds a node for a row only once that row's flag stands, and the
/// metric layer refuses the second row of a duplicated name, so a duplicated
/// name and an unpublished row reach the read out of a table a test composes
/// by hand (FR-016, FR-051).
[[nodiscard]] auto topdownPublishedHolds(const TopdownTree& built,
                                         const TopdownTable& table) -> bool;

}  // namespace sg::counters::detail

#endif  // SG_COUNTERS_LINUX_PMU_TOPDOWN_SELECT_HPP
