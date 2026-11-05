// ============================================================================
// The record beside one published limit, and the seam that clears it
// (specs/041-counter-probe FR-040a, FR-043, D-3, D-3a).
//
// The provider fills `ObjectSeed::programmableCounters` and records here
// which finder answered the figure. The exported accessor
// `sg::counters::objectLimitSource` reads this same record, so a figure
// and the word that says where it came from reach a reader as one pair
// (FR-050, FR-051, FR-053).
//
// The record reuses the public `sg::counters::LimitSource`, so the writer
// and the reader name one closed set of answers. The store is process
// state beside the tree, the shape `programmableLimitTable()` keeps at
// `source/counters/system.cpp:187-191`: one table keyed by the object
// path, filled while a provider registers and read after the catalog
// opens, so no read races a write (FR-009).
//
// The seam is private: never installed, and read by three places, the
// provider's enumeration, the accessor's own definition, and the unit
// that drives both.
// ============================================================================

#ifndef SG_COUNTERS_DETAIL_LIMIT_SOURCE_RECORD_HPP
#define SG_COUNTERS_DETAIL_LIMIT_SOURCE_RECORD_HPP

#include <string_view>

#include "speedgun-ng/counters_system.hpp"

namespace sg::counters::detail
{

/// Record the source of the limit `objectPath` publishes (FR-040a,
/// FR-043).
///
/// The provider of the path is the only writer, and it writes beside the
/// figure it publishes. `LimitSource::NONE` states that the path holds no
/// published limit and drops any record the path held.
///
/// \pre `objectPath` is the catalog address of the seed the provider
///      filled.
/// \post `objectLimitSource(objectPath)` answers `source` where `source`
///       names a finder, and answers `NONE` where `source` is
///       `LimitSource::NONE` (FR-040a)
auto recordLimitSource(std::string_view objectPath, LimitSource source) -> void;

/// Drop every record the process holds (FR-040a).
///
/// This is the test seam. One unit drives the writer and the reader
/// across several scenarios in one process, and this call returns the
/// store to its empty state between them. No production path calls it.
///
/// \pre none
/// \post `objectLimitSource` answers `NONE` for every path, and a record
///       written after the call stands on its own (FR-040a)
auto clearLimitSources() -> void;

}  // namespace sg::counters::detail

#endif  // SG_COUNTERS_DETAIL_LIMIT_SOURCE_RECORD_HPP
