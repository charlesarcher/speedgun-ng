#ifndef SG_HARNESS_DETAIL_HUNT_RANKS_HPP
#define SG_HARNESS_DETAIL_HUNT_RANKS_HPP

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "speedgun-ng/benchmark.hpp"

/**
 * @file hunt_ranks.hpp
 * @brief The family surface of specs/040-hunter-sensitivity (module M-C):
 * the family grouping and point assembly of B-4, the sensitivity stamp,
 * the baseline extraction and norm check the compare path reads (B-5),
 * the delta stamp the extraction feeds (B-6), and the rank assignment
 * under the two new keys (B-7).
 *
 * The run path calls the stamp after every entry's hunt stands ranked and
 * before the console table prints (B-10). Every figure of this surface reads
 * the run's collected records alone: no clock, no counter, no `/proc`, no
 * sysfs and no CPUID enters it (FR-006, A-1).
 */

namespace sg::detail
{

/**
 * @brief One family of one run (D-2): the points of one event address
 * over the entries one suite and case name group.
 *
 * A point stands the pair shape the rank helpers of B-2 and B-3 read:
 * the time per iteration first, the event's normalized rate under the
 * run's settled norm second, the point record data-model.md names. The
 * struct carries the point lists alone; the figures land on the
 * `RankedEvent` records the stamp fills.
 *
 * \pre none
 * \post `events` holds one list per event address the family's
 *       entries measured, one point per entry that measured the event
 * \invariant no event address stands twice in `events`, and no list
 *            in it stands empty
 */
struct HuntFamily
{
  std::map<std::string, std::vector<std::pair<double, double>>> events;
};

/**
 * @brief The family grouping and point assembly of B-4 (FR-001, D-2).
 *
 * \pre `results` holds one record per benchmark entry of one run, the
 *      hunt records ranked, the rates the run's settled norm produced
 * \post one family per suite and case name group, the key the suite
 *       and the case name with its trailing args segment dropped, an
 *       entry standing in one family alone; the points of one event
 *       stand over the entries that measured it, the time figure the
 *       entry's aggregate row median and the rate the entry's hunt
 *       record carries; the call measures nothing and reads no clock
 *       (FR-006)
 * \invariant no entry of `results` stands in two families
 */
[[nodiscard]] auto familyGroups(
    const std::vector<sg::BenchmarkResult>& results)
    -> std::vector<HuntFamily>;

/**
 * @brief The sensitivity stamp of B-4 (FR-001, FR-002): the
 * correlation and the slope of one family's points on the records the
 * family's entries carry.
 *
 * \pre `results` holds one record per benchmark entry of one run, the
 *      hunt records ranked, the rates the run's settled norm produced
 * \post an event whose point set answers both figures carries them on
 *       every record of the event the family's entries hold; a family
 *       under 3 points for the event, and a point set with no spread
 *       in either coordinate, leave both sensitivity fields empty; the
 *       call changes no other field of any record and reads no clock
 *       (FR-002, FR-006, D-2)
 * \invariant the figures read the point sets alone: no clock, no file
 *            and no counter enters them (FR-006)
 */
auto stampSensitivity(std::vector<sg::BenchmarkResult>& results) -> void;

/**
 * @brief The baseline extraction of B-5 (FR-003, D-6): the hunt rates
 *        of the matched entry of one loaded baseline document, by
 *        catalog address.
 *
 * \pre `baseline` stands a value `loadReport` answered, and
 *      `matchedName` names the contender entry the run's match paired
 *      with the baseline's entry under that name (R-2)
 * \post the answer holds one rate per event address the matched
 *       entry's hunt object carries, each rate the entry's median
 *       figure for its address; a matched entry whose hunt object
 *       stands absent, and a name the baseline document holds none of,
 *       answer an empty map; the call measures nothing and reads no
 *       clock (FR-006)
 * \invariant the answer reads the document value and `matchedName`
 *            alone: no file, no clock and no counter enters it
 *            (FR-006)
 */
[[nodiscard]] auto baselineHuntRates(const sg::ReportData& baseline,
                                     std::string_view matchedName)
    -> std::map<std::string, double>;

/**
 * @brief The norm-pair check of B-5 (FR-005, D-6): the mismatch text
 *        the baseline refusal prints.
 *
 * \pre `baseline` stands a value `loadReport` answered, and `settled`
 *      names the norm the contender run settled
 * \post a baseline whose top-level hunt object names a norm different
 *       from `settled` answers one line naming the mismatch and both
 *       norm words, the line the refusal prints before the entries
 *       run and the run exits 2 on; a baseline whose hunt object
 *       names `settled`, and one whose hunt object names no norm,
 *       answer empty; the call measures nothing and reads no clock
 *       (FR-006)
 * \invariant the answer reads the document value and `settled` alone:
 *            no file, no clock and no counter enters it (FR-006)
 */
[[nodiscard]] auto baselineNormMismatch(const sg::ReportData& baseline,
                                        sg::hunt::HuntNorm settled)
    -> std::optional<std::string>;

/**
 * @brief The delta stamp of B-6 (FR-004, D-5): the signed percent of
 *        A-5 on the records the baseline rates match.
 *
 * \pre `results` holds one record per benchmark entry of one run, the
 *      hunt records ranked, the rates the run's settled norm produced,
 *      and `baseline` stands the answer `baselineHuntRates` gave for
 *      the matched entry
 * \post a record whose address `baseline` holds at a rate above zero
 *       carries `delta`, the signed percent
 *       `100 x (contender - baseline) / baseline` of its rate, on
 *       every record of that address the run's entries hold; a
 *       contender address `baseline` holds none of, a baseline rate at
 *       or below zero, and an empty `baseline`, leave `delta` empty; a
 *       baseline address no record of `results` carries fills nothing;
 *       the call changes no other field of any record, measures
 *       nothing and reads no clock (FR-004, FR-006, D-5)
 * \invariant the delta reads the record rates and `baseline` alone: no
 *            file, no clock and no counter enters it (FR-006)
 */
auto stampDelta(std::vector<sg::BenchmarkResult>& results,
                const std::map<std::string, double>& baseline) -> void;

/**
 * @brief The rank assignment of B-7 (FR-011, D-3): the ranks of the
 *        run's entries under the key the run settled for
 *        `--hunt-rank`.
 *
 * \pre `results` holds one record per benchmark entry of one run, the
 *      hunt records ranked, and `key` names `SENSITIVITY` or `DELTA`;
 *      the key's figure stands on the records the key reads (the
 *      stamping pass of B-7 ran), and under `SENSITIVITY` a record
 *      that carries the correlation carries the slope, the pair the
 *      sensitivity stamp leaves (D-2)
 * \post under `SENSITIVITY` an event whose records carry the
 *       correlation carries a rank on every record of its address the
 *       run's entries hold, the ranks ordered by the correlation
 *       descending, ties by the slope and then the catalog address;
 *       under `DELTA` the ranks order by the magnitude of the delta
 *       descending, ties by the catalog address; a noisy event stands
 *       unranked under either key (A-3), an event whose records carry
 *       no figure of the key stands unranked (A-4), and the events
 *       that carry a rank carry the ranks `1` to `k` with no gap; a
 *       set that stands empty under the key leaves every rank field
 *       empty and the entries in the order they stand; the call
 *       changes no figure of any record and reorders no record
 *       (A-1, FR-006)
 * \invariant the ranks read the records' figures and `key` alone: no
 *            file, no clock and no counter enters them (FR-006)
 */
auto stampRank(std::vector<sg::BenchmarkResult>& results,
               const sg::hunt::HuntRank key) -> void;

}  // namespace sg::detail

#endif  // SG_HARNESS_DETAIL_HUNT_RANKS_HPP
