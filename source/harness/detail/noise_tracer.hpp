#ifndef SG_HARNESS_NOISE_TRACER_HPP
#define SG_HARNESS_NOISE_TRACER_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "speedgun-ng/benchmark.hpp"

/**
 * @file noise_tracer.hpp
 * @brief The noise tracer internals behind the option: the value
 * parser of contracts/cli-options.md, and the engine session and the
 * figure read the later phases land beside it. Nothing here is public
 * API (FR-002, R-06).
 */

namespace sg::detail
{

/// @brief The value grammar of contracts/cli-options.md (FR-002, FR-003,
/// D-4, R-06): the head names the engine word `osnoise` or `timerlat`,
/// then at most one optional `cpus` key follows, whose items the
/// standing cpu-list reader reads whole: indexes and closed `a-b`
/// ranges, every cpu inside the process affinity mask and the online
/// set, no cpu standing twice in the list, and the key standing once in
/// the value. A bare item before the key names no list the value asked
/// for. Every rejection names the token it read, quoted the way the
/// option module quotes a value, so the caller's one line carries it
/// (FR-003). Declared beside the probe's parser because the option
/// file's settle arm parses the value where it settles, with the
/// origin the settle knows (FR-027, R-06).
/// \post an answer of true stands `engine` at the word the value named
///       and `cpus` at the list the `cpus` key named, ascending
[[nodiscard]] auto parseOsNoiseValue(const std::string& value,
                                     TracerEngine& engine,
                                     std::vector<std::uint32_t>& cpus,
                                     std::string& message) -> bool;

/// @brief One file the detector run saved (data-model.md E-07, D-8,
/// P-8): the path the run wrote below the tracefs root, and the
/// standing text the setup read there before the write. The
/// `hwlat_detector/mode` value holds the bracketed word alone, the one
/// form the mode file's write handler compares (P-16).
struct TracerSetting
{
  std::string path;
  std::string value;
};

/// @brief The engine state one run owns across the pre-check window
/// (data-model.md E-18, E-07): the engine and the cpu list the run
/// chose, the standing `current_tracer` word and the standing
/// `tracing_cpumask` text the setup read for the restore (R-04), and
/// the saved set the spec `036` session shape grows to carry (D-8).
struct NoiseTracerSession
{
  TracerEngine engine = TracerEngine::NONE;
  std::vector<std::uint32_t> cpus;
  std::string previousTracer;
  std::string previousCpuMask;
  /// @brief The saved set in write order, one pair per file the setup
  /// saved: the six files FR-019 names for the `hwlat` engine (FR-021,
  /// D-8, P-8). The restore walks it in reverse. `tracing_on` stands
  /// outside the set: the restore writes `0` there, the standing shape
  /// spec SC-007 measures (D-8).
  std::vector<TracerSetting> saved;
};

/// @brief The word the engine writes into `current_tracer` (specs/036
/// R-02): `osnoise` or `timerlat`. The record's engine field carries
/// the same word (FR-014).
///
/// \pre `engine` names `OSNOISE` or `TIMERLAT`
/// \post the answer stands the engine's word
[[nodiscard]] auto engineWordOf(TracerEngine engine) -> std::string;

/// @brief The session setup (specs/036 FR-007, FR-016, R-02, R-04):
/// the standing `current_tracer` word and the standing
/// `tracing_cpumask` text read first, then the engine word written,
/// the tracer mask written rendered in the word width the standing
/// mask carried, and `tracing_on` written at `1`, in that order
/// (R-04). Every file resolves
/// `procRootPath("sys/kernel/tracing/<file>")` (R-01). The function
/// stands on the entry-point thread, outside every timed window
/// (R-05).
///
/// \pre `engine` names `OSNOISE` or `TIMERLAT`
/// \pre `cpus` names at least one cpu
/// \post the answer stands the session when every read and write
///       stood through, carrying `engine`, `cpus` and the standing
///       word and mask, and the empty optional when any read or
///       write failed, the not-available shape (FR-016, D-5); no
///       exception leaves the function
[[nodiscard]] auto noiseTracerSetup(TracerEngine engine,
                                    const std::vector<std::uint32_t>& cpus)
    -> std::optional<NoiseTracerSession>;

/// @brief The figure read of the pinned count-file format (specs/036
/// FR-010, R-03, R-07): one entry per cpu of `session.cpus`, in the
/// session order, each entry carrying the five figures the per-cpu
/// count file `osnoise/cpu<id>` states, one line per source the
/// engine reports in the form `<source-word>: <count>`, the source
/// words standing in the order `nmi`, `irq`, `softirq`, `thread`,
/// `hw`, and `hw` mapping to the record's `hardware` figure. A source
/// word the file omits stands that figure absent for that source
/// alone. A file that stands missing, unreadable, empty, or that
/// carries one malformed line stands every figure of that cpu absent
/// (R-07). The read never fails the run (FR-016). The function stands
/// on the entry-point thread, outside every timed window (R-05).
///
/// \pre `session` stands the setup's answer, the engine naming a
///       tracer
/// \post the answer carries one entry per cpu of `session.cpus`, in
///       the session order
[[nodiscard]] auto noiseTracerFigures(const NoiseTracerSession& session)
    -> std::vector<PrecheckTracerCpuRecord>;

/// @brief The restore (specs/036 FR-007, FR-016, R-04; specs/048
/// FR-021, FR-022, D-8, P-8): the function writes `tracing_on` at `0`,
/// then the saved set in the reverse of the write order, so the engine
/// word goes back before the threshold and the kernel's own reset runs
/// ahead of the host threshold. The two standing engines carry an empty
/// saved set and keep their own order: `tracing_on`, then the standing
/// word, then the standing mask. Every file resolves
/// `procRootPath("sys/kernel/tracing/<file>")` (R-01). A failed write
/// refuses nothing: the run never fails on the tracer (FR-016). The
/// function stands on the entry-point thread, outside every timed
/// window (R-05).
///
/// \pre `session` stands the setup's answer, the engine naming a
///       tracer
/// \post every file whose write stood through stands at the value the
///       session carries, and a file beside a failed write stands
///       unchanged
/// \post the answer carries one pair per saved file whose write
///       failed, in the order the restore wrote them, and stands empty
///       where every write stood through (FR-022, D-12); `tracing_on`
///       stands outside the answer, because no saved value backs it
///       (D-8)
auto noiseTracerRestore(const NoiseTracerSession& session)
    -> std::vector<TracerSetting>;

}  // namespace sg::detail

#endif  // SG_HARNESS_NOISE_TRACER_HPP
