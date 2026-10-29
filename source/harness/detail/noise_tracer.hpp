#ifndef SG_HARNESS_NOISE_TRACER_HPP
#define SG_HARNESS_NOISE_TRACER_HPP

#include <cstdint>
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

}  // namespace sg::detail

#endif  // SG_HARNESS_NOISE_TRACER_HPP
