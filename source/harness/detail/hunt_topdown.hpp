#ifndef SG_HARNESS_DETAIL_HUNT_TOPDOWN_HPP
#define SG_HARNESS_DETAIL_HUNT_TOPDOWN_HPP

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "speedgun-ng/benchmark.hpp"
#include "speedgun-ng/counters_topdown.hpp"

/**
 * @file hunt_topdown.hpp
 * @brief The top-down area mapper of specs/046-hunter-topdown: the deepest
 * node one catalog address reaches, the path above that node, the area of one
 * ranked event, and the stamp that settles the area of every record of one run
 * (FR-001 to FR-007, FR-009; D-1, D-3, D-4, D-7, D-10, D-16, D-17;
 * contracts/area-mapper.md).
 *
 * One call reads three values and nothing else: the address of one ranked
 * record, the tree answer of the run, and the vendor grouping of D-16. No
 * clock, no counter, no event, no path and no kernel interface enters one of
 * these calls, so two calls over one triple answer one text (FR-001, FR-006,
 * D-7).
 *
 * The mapper is internal. It stands outside the headers the doc gate reads, so
 * neither gate counts the tags below or the checks of the source file beside
 * them. `stripTopdownColumns` states that reading for its own pair at
 * `source/harness/runner.cpp:694-698`.
 */

namespace sg::detail
{

/// The level the area ask hands `TopdownSelection::level`: the deepest ask the
/// type takes, whose answer the table clamps to its own depth
/// (`include/speedgun-ng/counters_topdown.hpp:407-409`) and whose range
/// `topdownTree` enforces (FR-001, FR-008, D-17). The level-1 companion of D-9
/// carries its own level, so the two asks of one run differ in this figure
/// alone (FR-033).
inline constexpr int kTopdownAreaLevel {6};

/**
 * @brief The index of the deepest node of `tree` whose `events` holds
 * `address` as an equal text (FR-001, FR-002, FR-003, D-4).
 *
 * A node names an event with the spelling its own list carries, so a second
 * spelling of one address names no node (FR-002).
 *
 * \pre none
 * \post the answer holds an index of `tree.nodes` where some node names
 *       `address`, and it holds the lowest such index where two nodes of one
 *       depth name it; the answer stands empty where no node names `address`
 *       (FR-002, FR-003)
 * \invariant the answer reads `tree.nodes` and `address` alone: no clock, no
 *            counter and no kernel interface enters it (D-7)
 */
[[nodiscard]] auto topdownDeepestNode(const sg::counters::TopdownTree& tree,
                                      std::string_view address)
    -> std::optional<std::size_t>;

/**
 * @brief specs/046 FR-001, FR-003, D-4: whether every node of `tree` that
 * names `address` stands at a level at or below the level of the node at
 * `answer`.
 *
 * A node that names no such address takes no part in the answer. The assertion
 * of `topdownDeepestNode` reads this answer over the whole list, which is how
 * it settles that the node it chose stands deepest.
 *
 * \pre `answer` names an element of `tree.nodes`
 * \post the answer reads true when every node whose `events` holds `address`
 * as an equal text carries a level at or below the level of the node at
 * `answer`, and it reads false as soon as one such node stands deeper
 * \invariant the answer reads `tree.nodes` and `address` alone: no clock, no
 *            counter and no kernel interface enters it (D-7)
 */
[[nodiscard]] auto nodesNamingAddressStandAtOrBelow(
    const sg::counters::TopdownTree& tree,
    std::size_t answer,
    std::string_view address) -> bool;

/**
 * @brief The path of one node: the names the `parent` links reach above it,
 * root first, joined by " > " (FR-007, D-10).
 *
 * \pre `index` names an element of `tree.nodes`, and every `parent` text above
 *      that node names a node that stands ahead of it
 *      (specs/042-topdown FR-013)
 * \post the answer joins the name of the node at `index` with the names of the
 *       nodes its `parent` link reaches, root first, in the spelling the table
 *       publishes; a node at level 1 answers its own name alone (FR-007, D-10)
 * \invariant the answer reads `tree.nodes` and `index` alone: no clock, no
 *            counter and no kernel interface enters it (D-7)
 */
[[nodiscard]] auto topdownPathOf(const sg::counters::TopdownTree& tree,
                                 std::size_t index) -> std::string;

/**
 * @brief specs/046 FR-001 to FR-007, D-4, D-10: the area of one catalog
 * address over the three inputs of the run.
 *
 * The arms run in that order: no tree answers `unmapped`, a node that names the
 * address answers its path, the grouping answers the leaf part of the address,
 * and no row of it answers `unmapped` (FR-002 to FR-005).
 *
 * \pre none
 * \post the answer stands never empty; with no tree it reads `unmapped` for
 *       every vendor and every refusal kind (FR-005); with a tree it reads the
 *       path of the deepest node whose `events` holds `address` as an equal
 *       text (FR-002, FR-003, FR-007), else the group of the leaf part of
 *       `address` (FR-004), else `unmapped`; two calls with the same three
 *       inputs answer one text (FR-006); the call reads no counter, opens no
 *       event, reads no clock and asks no kernel interface (FR-001, D-7)
 * \invariant the answer reads its three arguments alone
 */
[[nodiscard]] auto topdownArea(
    std::string_view address,
    const std::optional<sg::counters::TopdownTree>& tree,
    const std::vector<sg::counters::TopdownEventGroup>& groups) -> std::string;

/**
 * @brief specs/046 FR-009: the area of every ranked record of every hunted
 * entry of one run.
 *
 * \pre `results` holds one record per benchmark entry of one run and the hunt
 *      records stand ranked
 * \post every record of `entries` of every hunt record carries the answer
 *       `topdownArea` gives for its address, the ranked records and the
 *       unranked ones alike; no other field of any record moves, no record is
 *       added, removed or reordered, and `skipped` gains nothing (FR-010)
 * \invariant the areas read the records' addresses and the three inputs alone
 */
auto stampTopdownAreas(
    std::vector<sg::BenchmarkResult>& results,
    const std::optional<sg::counters::TopdownTree>& tree,
    const std::vector<sg::counters::TopdownEventGroup>& groups) -> void;

}  // namespace sg::detail

#endif  // SG_HARNESS_DETAIL_HUNT_TOPDOWN_HPP
