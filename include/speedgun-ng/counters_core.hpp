#ifndef SPEEDGUN_NG_COUNTERS_CORE_HPP
#define SPEEDGUN_NG_COUNTERS_CORE_HPP

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
struct Dim
{
  static constexpr int kTimeExponent = T;
  static constexpr int kEventsExponent = C;
};

/**
 * @brief True when both tags name the same dimension.
 *
 * \pre none
 * \post none
 */
template<class D1, class D2>
inline constexpr bool kDimSame = (D1::kTimeExponent == D2::kTimeExponent)
    && (D1::kEventsExponent == D2::kEventsExponent);

/**
 * @brief The quotient tag: exponents subtract (FR-015).
 *
 * \pre none
 * \post none
 */
template<class D1, class D2>
using DimQuotient = Dim<D1::kTimeExponent - D2::kTimeExponent,
                        D1::kEventsExponent - D2::kEventsExponent>;

/**
 * @brief The recognized catalog units. The mapping from unit tokens to
 * this enumeration, and from here to a dimension, is a closed switch:
 * an unrecognized token is a resolution error naming the token and is
 * never guessed (FR-017).
 */
enum class Unit : std::uint8_t
{
  SECONDS,
  NANOSECONDS,
  BYTES,
  OPS,
  NONE
};

/**
 * @brief Per-entry catalog state: described-ness and countability are
 * separate predicates carried by the state name (FR-006).
 *
 * `SCOPE_REFUSED` separates a refusal the entry's own scope causes from
 * a refusal its encoding causes, so a caller that reads it knows a
 * cpu-target plan may still compile over the entry. `GAP` names the one
 * case the catalog never publishes for an entry: a gap is a property of
 * one sampling action and an entry spans many, so the value travels in
 * the plan's disclosure column beside the zero count the action produced
 * (FR-021).
 */
enum class Availability : std::uint8_t
{
  COUNTABLE,
  PERMISSION_BLOCKED,
  NOT_ENCODABLE,
  ABSENT,
  SCOPE_REFUSED,
  GAP
};

/**
 * @brief The target kinds one entry can be counted on, as a fixed-size
 * bitmask over `TargetKind` (FR-021).
 *
 * Bit 0 is `TargetKind::THREAD` and bit 1 is `TargetKind::CPU`. The
 * type is a fixed-size unsigned integer and allocates no memory, so a
 * caller reads an entry's targets without a container and without an
 * allocation. A new kernel target takes the next free bit: no enumerator
 * value changes and no stored bit moves.
 */
using TargetMask = std::uint32_t;

/// @brief `TargetKind::THREAD`, bit 0 of `TargetMask` (FR-021).
inline constexpr TargetMask kTargetThreadBit = 1U;

/// @brief `TargetKind::CPU`, bit 1 of `TargetMask` (FR-021).
inline constexpr TargetMask kTargetCpuBit = 2U;

/**
 * @brief The achieved read mechanism for a leaf, probed during provider
 * enumeration before the catalog freezes at the open boundary, and
 * disclosed per catalog entry (FR-023).
 */
enum class ReadMode : std::uint8_t
{
  FAST_TSC,
  FAST_RDPMC,
  SYSCALL,
  PUSH_LOAD
};

/**
 * @brief Tier-2 recoverable error: names the failing input and carries
 * near-miss suggestions (FR-008).
 *
 */
struct Error
{
  std::string message;
  std::vector<std::string> suggestions;
};

/**
 * @brief Runtime mirror of a `Dim` tag: the exponent pair as data, used
 * where a dimension travels as a value (catalog resolution, FR-017).
 *
 */
struct Dimension
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
[[nodiscard]] inline auto unitFromToken(const std::string_view token)
    -> std::expected<Unit, Error>
{
  if (token == "seconds") {
    return Unit::SECONDS;
  }
  if (token == "nanoseconds") {
    return Unit::NANOSECONDS;
  }
  if (token == "bytes") {
    return Unit::BYTES;
  }
  if (token == "ops") {
    return Unit::OPS;
  }
  if (token == "none") {
    return Unit::NONE;
  }
  return std::unexpected(
      Error {.message = "unrecognized unit '" + std::string(token)
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
[[nodiscard]] inline auto dimensionOf(const Unit value)
    -> std::expected<Dimension, Error>
{
  switch (value) {
    case Unit::SECONDS:
    case Unit::NANOSECONDS:
      return Dimension {.time = 1, .events = 0};
    case Unit::BYTES:
    case Unit::OPS:
    case Unit::NONE:
      return Dimension {.time = 0, .events = 1};
  }
  return std::unexpected(
      Error {.message = "unit value outside the closed enumeration",
             .suggestions = {}});
}

/**
 * @brief The canonical token spelling of a recognized unit.
 *
 * \pre none
 * \post none
 */
[[nodiscard]] inline auto unitName(const Unit value) -> std::string_view
{
  switch (value) {
    case Unit::SECONDS:
      return "seconds";
    case Unit::NANOSECONDS:
      return "nanoseconds";
    case Unit::BYTES:
      return "bytes";
    case Unit::OPS:
      return "ops";
    case Unit::NONE:
      return "none";
  }
  return "unknown";
}

/**
 * @brief One named counter on one object, as the catalog reports it
 * (FR-005, E-03).
 *
 * Views into provider-owned or system-owned storage; catalog strings
 * are immutable once the system is open (FR-009). `avail` and `targets`
 * agree: a state of `countable` names at least one target bit, and every
 * other state names none (FR-021).
 *
 */
struct CatalogEntry
{
  std::string_view name;
  std::string_view description;
  sg::counters::Unit unit;
  Availability avail;
  ReadMode mode;
  // The target kinds the entry can be counted on (FR-021).
  TargetMask targets = 0;
  std::uint64_t frequencyHz = 0;  // fixed-rate calibration, 0 elsewhere
  bool scaled = false;  // platform-scaled tick source disclosure
};

/**
 * @brief The fold output: value, running-ratio disclosure, the
 * availability the window discloses, and the scaled flag. The fields are
 * members of the returned struct, so omitting the disclosure is
 * structurally impossible (FR-019).
 *
 * `availability` names the state the folded window carries. A fold whose
 * start point or whose end point is an action that measured nothing
 * reports `Availability::GAP` and a `value` that is not a measurement, so
 * a caller reads the state before the number. A fold whose two end points
 * are both measured reports the state those leaves disclosed, which is
 * `Availability::COUNTABLE` for a leaf the host can count and one of the
 * other enumerators otherwise. A gap anywhere strictly inside the window
 * changes no fold, because the recorded counts are cumulative and a window
 * with two measured end points has an exact delta between them
 * (FR-001, FR-004, FR-005).
 *
 * `runningRatio` is the multiplex disclosure over the window. A window
 * that carries a gap publishes `1.0` and discloses nothing, because no
 * measured time covers an action that measured no count (FR-005).
 */
struct MetricResult
{
  double value = 0.0;
  double runningRatio = 1.0;
  // The field name is the contract name. The type spelling is Availability.
  Availability availability = Availability::COUNTABLE;
  bool scaled = false;
};

}  // namespace sg::counters

#endif  // SPEEDGUN_NG_COUNTERS_CORE_HPP
