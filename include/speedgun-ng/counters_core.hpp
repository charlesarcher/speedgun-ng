#ifndef SPEEDGUN_NG_COUNTERS_CORE_HPP
#define SPEEDGUN_NG_COUNTERS_CORE_HPP

#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

#include "speedgun-ng/speedgun-ng_export.hpp"

/**
 * @file counters_core.hpp
 * @brief Core vocabulary of the counters library: dimension tags, the
 * closed unit mapping, catalog entry and result shapes, and the
 * recoverable-error type.
 *
 * Feature 007-counters-and-timers. Namespace `sg::counters`; this header
 * carries no platform vocabulary (FR-010). The dimension algebra is
 * construction-time only and erased on the read path (FR-016).
 */

namespace sg::counters
{

/**
 * @brief Compile-time dimension tag: time^T x events^C.
 *
 * The pair of integer exponents is the whole tag. Tags live in types and
 * never reach the recorded point columns (FR-015, FR-016).
 *
 */
template<int T, int C>
struct dim
{
  static constexpr int time_exponent = T;
  static constexpr int events_exponent = C;
};

/**
 * @brief True when both tags name the same dimension.
 *
 * \pre none
 * \post none
 */
template<class D1, class D2>
inline constexpr bool dim_same = (D1::time_exponent == D2::time_exponent)
    && (D1::events_exponent == D2::events_exponent);

/**
 * @brief The quotient tag: exponents subtract (FR-015).
 *
 * \pre none
 * \post none
 */
template<class D1, class D2>
using dim_quotient = dim<D1::time_exponent - D2::time_exponent,
                         D1::events_exponent - D2::events_exponent>;

/**
 * @brief The recognized catalog units. The mapping from unit tokens to
 * this enumeration, and from here to a dimension, is a closed switch:
 * an unrecognized token is a resolution error naming the token and is
 * never guessed (FR-017).
 */
enum class unit : std::uint8_t
{
  seconds,
  nanoseconds,
  bytes,
  ops,
  none
};

/**
 * @brief Per-entry catalog state: described-ness and countability are
 * separate predicates carried by the state name (FR-006).
 */
enum class availability : std::uint8_t
{
  countable,
  permission_blocked,
  not_encodable,
  absent
};

/**
 * @brief The achieved read mechanism for a leaf, probed at plan compile
 * and disclosed per catalog entry (FR-023).
 */
enum class read_mode : std::uint8_t
{
  fast_tsc,
  fast_rdpmc,
  syscall,
  push_load
};

/**
 * @brief Tier-2 recoverable error: names the failing input and carries
 * near-miss suggestions (FR-008).
 *
 */
struct error
{
  std::string message;
  std::vector<std::string> suggestions;
};

/**
 * @brief Runtime mirror of a `dim` tag: the exponent pair as data, used
 * where a dimension travels as a value (catalog resolution, FR-017).
 *
 */
struct dimension
{
  int time = 0;
  int events = 0;
};

/**
 * @brief Maps a provider unit token through the closed switch to a
 * recognized unit.
 *
 * Recognized tokens: "seconds", "nanoseconds", "bytes", "ops", "none".
 * On failure the error message names the rejected token and the
 * suggestion list is empty; the `expected` return carries the failure
 * tier (FR-046), so no runtime check is added on top of the type.
 *
 * \pre none
 * \post none
 */
[[nodiscard]] inline auto unit_from_token(const std::string_view token)
    -> std::expected<unit, error>
{
  if (token == "seconds") {
    return unit::seconds;
  }
  if (token == "nanoseconds") {
    return unit::nanoseconds;
  }
  if (token == "bytes") {
    return unit::bytes;
  }
  if (token == "ops") {
    return unit::ops;
  }
  if (token == "none") {
    return unit::none;
  }
  return std::unexpected(
      error {.message = "unrecognized unit '" + std::string(token)
                 + "'; the unit-to-dimension mapping is closed (FR-017)",
             .suggestions = {}});
}

/**
 * @brief Maps a recognized unit to its dimension (FR-017).
 *
 * "seconds" and "nanoseconds" map to time^1. "bytes", "ops" and "none"
 * map to events^1. The enumeration is closed: every enumerator maps, so
 * the failure branch exists only as the defensive close of the switch.
 *
 * \pre none
 * \post none
 */
[[nodiscard]] inline auto dimension_of(const unit value)
    -> std::expected<dimension, error>
{
  switch (value) {
    case unit::seconds:
    case unit::nanoseconds:
      return dimension {.time = 1, .events = 0};
    case unit::bytes:
    case unit::ops:
    case unit::none:
      return dimension {.time = 0, .events = 1};
  }
  return std::unexpected(
      error {.message = "unit value outside the closed enumeration",
             .suggestions = {}});
}

/**
 * @brief The canonical token spelling of a recognized unit.
 *
 * \pre none
 * \post none
 */
[[nodiscard]] inline auto unit_name(const unit value) -> std::string_view
{
  switch (value) {
    case unit::seconds:
      return "seconds";
    case unit::nanoseconds:
      return "nanoseconds";
    case unit::bytes:
      return "bytes";
    case unit::ops:
      return "ops";
    case unit::none:
      return "none";
  }
  return "unknown";
}

/**
 * @brief One named counter on one object, as the catalog reports it
 * (FR-005, E-03).
 *
 * Views into provider-owned or system-owned storage; catalog strings
 * are immutable once the system is open (FR-009).
 *
 */
// NOLINTNEXTLINE(readability-identifier-naming)
struct catalog_entry
{
  std::string_view name;
  std::string_view description;
  sg::counters::unit unit;
  availability avail;
  read_mode mode;
  std::uint64_t frequency_hz = 0;  // fixed-rate calibration, 0 elsewhere
  bool scaled = false;  // platform-scaled tick source disclosure
};

/**
 * @brief The fold output: value, running-ratio disclosure, and the
 * scaled flag. The fields are members of the returned struct, so
 * omitting the disclosure is structurally impossible (FR-019).
 *
 */
// NOLINTNEXTLINE(readability-identifier-naming)
struct metric_result
{
  double value = 0.0;
  double running_ratio = 1.0;
  bool scaled = false;
};

}  // namespace sg::counters

#endif  // SPEEDGUN_NG_COUNTERS_CORE_HPP
