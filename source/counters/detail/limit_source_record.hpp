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
// state beside the tree: one table keyed by the object path, filled
// while a provider registers and read after the catalog opens, so no
// read races a write (FR-009). The published limits stand in
// `System::Impl` since specs/050-limit-table-lifecycle moved them
// there, one table per System, and this store keeps the source of each
// figure for the life of the process (FR-043).
//
// The seam is private: never installed, and read by four places, the
// provider's enumeration, the accessor's own definition, the
// registration that takes back what a refused provider recorded, and the
// unit that drives them.
// ============================================================================

#ifndef SG_COUNTERS_DETAIL_LIMIT_SOURCE_RECORD_HPP
#define SG_COUNTERS_DETAIL_LIMIT_SOURCE_RECORD_HPP

#include <map>
#include <string>
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

/// The whole store, keyed by object path (T029, S-2).
using LimitSourceRecords = std::map<std::string, LimitSource>;

/// A copy of every record the process holds (T029, S-2).
///
/// `System::registerProvider` takes it ahead of the provider's
/// enumeration, because the provider records a source while it
/// enumerates and the registration validates the batch only after.
///
/// \pre none
/// \post the answer holds one entry per recorded path, each naming a
///       finder
[[nodiscard]] auto limitSourceSnapshot() -> LimitSourceRecords;

/// Put the store back at one snapshot (T029, S-2).
///
/// A refused provider leaves no source behind: the registration hands
/// back the snapshot it took, so the post of `objectLimitSource`, `NONE`
/// exactly where `programmableCounters()` answers no value, holds for
/// the records of a refused batch too.
///
/// \pre `records` came from `limitSourceSnapshot`
/// \post `objectLimitSource` answers what it answered when the snapshot
///       was taken, for every path
auto restoreLimitSources(LimitSourceRecords records) -> void;

}  // namespace sg::counters::detail

#endif  // SG_COUNTERS_DETAIL_LIMIT_SOURCE_RECORD_HPP
