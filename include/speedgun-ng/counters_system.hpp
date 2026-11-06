#ifndef SG_COUNTERS_SYSTEM_HPP
#define SG_COUNTERS_SYSTEM_HPP

#include <cstdint>
#include <expected>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "speedgun-ng/counters_core.hpp"
#include "speedgun-ng/counters_measurement.hpp"
#include "speedgun-ng/counters_provider.hpp"
#include "speedgun-ng/speedgun-ng_export.hpp"

/**
 * @file counters_system.hpp
 * @brief The system handle: the tree of named countable objects, the
 * merged catalog, provider registration with the open boundary, path
 * and alias resolution, and near-miss diagnostics (FR-001..FR-009).
 */

namespace sg::counters
{

/**
 * @brief One countable object in the system tree (E-02).
 *
 * Handles are stable pointers into the tree; the tree is immutable
 * after the system opens, so they stay valid (FR-009). Every result
 * spells the canonical path; alias strings never appear in output
 * (FR-002).
 */
class SPEEDGUN_NG_EXPORT Object
{
public:
  Object(const Object&) = default;
  auto operator=(const Object&) -> Object& = default;

  /**
   * @brief The canonical structured path, such as `package-1/core-3`.
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto path() const noexcept -> std::string_view;

  /**
   * @brief The platform instance alias, empty when the object has
   * none (FR-002).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto alias() const noexcept -> std::string_view;

  /**
   * @brief The object kind, such as `machine` or `core` (FR-001).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto kind() const noexcept -> std::string_view;

  /**
   * @brief The catalog description (FR-001).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto description() const noexcept -> std::string_view;

  /**
   * @brief How many programmable counters one context can hold open at
   * once on this object's event source (FR-005).
   *
   * The answer is the value the object's event source published at
   * `enumerate`. No value is the answer of a source that cannot answer:
   * such a source limits nothing for the counters it owns.
   *
   * Registration writes the table before its `System` opens, and a read
   * follows the open of that same `System`. Within one `System`, no
   * read races a write, because registration closes when the boundary
   * opens. Two `System` objects share nothing, so no lock spans the
   * two: each holds its own tree and its own table (FR-003).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto programmableCounters() const noexcept
      -> std::optional<std::size_t>;

  /**
   * @brief The parent object, or null for the machine root and for a
   * non-root object whose immediate ancestor path names no catalog
   * object (FR-001).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto parent() const noexcept -> const Object*;

  /**
   * @brief This object's catalog entries (FR-001, FR-004).
   *
   * \pre Every stored leaf carries a unit token the closed switch maps.
   *      Registration admits no other, so a violation is a provider
   *      contract breach and reports. The leaf is never presented as
   *      `Unit::NONE` (FR-017).
   * \post Every entry's `avail` and `targets` agree: an entry reported
   *       `countable` names at least one target bit, and an entry in
   *       every other state names none (FR-021).
   */
  [[nodiscard]] auto counters() const -> std::vector<CatalogEntry>;

  /**
   * @brief The child objects in tree order (US3 scenario 1).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto children() const -> std::vector<const Object*>;

  /**
   * @brief Resolves one named counter under the requested dimension
   * (FR-005, FR-008).
   *
   * The catalog unit of the named counter must map to `D`; a mismatch
   * is a recoverable error naming both, a unit token outside the closed
   * mapping is a recoverable error naming the unit, and a wrong name is
   * a recoverable error carrying near-miss suggestions (at most five,
   * names within edit distance two first, then description word
   * overlaps, catalog order breaking ties). Nothing is guessed
   * (FR-017).
   *
   * \pre none
   * \post The resolved entry's unit token is a recognized token
   *       (FR-017).
   * \post The recognized unit maps to a dimension (FR-017).
   */
  template<class D>
  [[nodiscard]] auto counter(std::string_view name) const
      -> std::expected<sg::counters::Counter<D>, Error>
  {
    auto leaf = detail::resolveLeafCore(*this, name);
    if (!leaf.has_value()) {
      return std::unexpected(leaf.error());
    }
    // Recognition first: dereferencing the token before testing it
    // reads the error branch of the inner `expected` (FR-017).
    const auto recognized = unitFromToken(leaf->unit);
    // A stored catalog unit is a recognized token by construction:
    // `registerProvider` refuses an unrecognized token, so the tree
    // never holds one, and the recognized set is closed. `dimensionOf`
    // maps every enumerator of that closed enumeration, so neither
    // inner `expected` carries a failing value to return; the two
    // postconditions hold at every return the function reaches.
    SG_ENSURE(recognized.has_value(),
              "a stored catalog unit is a recognized token (FR-017)");
    const auto mapped = dimensionOf(*recognized);
    SG_ENSURE(mapped.has_value(),
              "the closed unit set maps to a dimension (FR-017)");
    if (mapped->time != D::kTimeExponent
        || mapped->events != D::kEventsExponent)
    {
      return std::unexpected(Error {
          .message = "counter '" + std::string(name) + "' has unit '"
              + leaf->unit + "' (dimension time^" + std::to_string(mapped->time)
              + " x events^" + std::to_string(mapped->events)
              + "); requested dimension time^"
              + std::to_string(D::kTimeExponent) + " x events^"
              + std::to_string(D::kEventsExponent),
          .suggestions = {}});
    }
    return sg::counters::Counter<D> {std::move(*leaf)};
  }

private:
  friend class System;
  friend auto detail::resolveLeafCore(const Object& obj, std::string_view name)
      -> std::expected<detail::LeafCore, Error>;
  friend auto detail::compileCore(const System& sys,
                                  const Target& tg,
                                  const std::vector<const detail::ExprCore*>&
                                      exprs) -> std::expected<Plan, Error>;

  explicit Object(void* node) noexcept
      : m_node(node)
  {
  }

  void* m_node = nullptr;  // the tree node
};

/**
 * @brief Where the limit one object published came from (FR-040a, FR-043,
 * D-3).
 *
 * The published limit is one figure, and this label names the finder that
 * answered it. The set is closed: a figure came from the decode, from the
 * probe, or it stands absent.
 */
enum class LimitSource : std::uint8_t
{
  NONE,  ///< The path publishes no limit (FR-040a).
  DECODE,  ///< The CPUID or event-table decode answered the figure
           ///< (specs/041-counter-probe FR-030).
  PROBE,  ///< The group probe answered the figure (FR-018, FR-043).
};

/**
 * @brief The source of the limit `programmableCounters()` answers for
 * `objectPath` (FR-040a, FR-043, FR-053, D-3, D-3a).
 *
 * A provider records the source beside the limit it publishes, and this
 * accessor answers the pair, so a run can say which finder answered the
 * count it planned with (FR-050, FR-051). `objectPath` is the catalog
 * address: the same text `Object::programmableCounters()` reads. A path
 * no provider recorded answers `NONE`, and so does a path whose published
 * limit stands absent.
 *
 * The accessor states no claim about the catalog of the process. It reads
 * one record beside the tree, resolves no path, and opens nothing.
 *
 * \pre none
 * \post the answer is `NONE` exactly where
 *       `Object::programmableCounters()` answers no value for that path
 *       (FR-040a)
 * \post a recorded path answers the source that filled its limit, and the
 *       answer stands for the life of the process once the catalog is
 *       open (FR-043, FR-009)
 */
[[nodiscard]] SPEEDGUN_NG_EXPORT auto objectLimitSource(
    std::string_view objectPath) noexcept -> LimitSource;

/**
 * @brief The root handle to the local machine's counting world
 * (E-01).
 *
 * Registration is accepted before the first use; the first use opens
 * the boundary, closes registration, and freezes the catalog, which
 * makes concurrent catalog reads safe by construction (FR-009).
 *
 * Each `System` owns its tree, its merged catalog and its limit table,
 * and the limit table ends with its `System` (FR-005). A read through a
 * handle reaches the table of the `System` that owns that handle's node,
 * and no other table (FR-007). A program may build a `System` of its
 * own, and two such objects hold two trees and answer their own limits
 * (FR-008).
 */
class SPEEDGUN_NG_EXPORT System
{
public:
  System(const System&) = delete;
  auto operator=(const System&) -> System& = delete;

  /**
   * @brief Builds a system the program owns, with its own tree, its own
   * catalog and its own limit table (FR-008).
   *
   * Registration stays open on a new object until its first use opens
   * the boundary and freezes the catalog (FR-009).
   *
   * \pre none
   * \post none
   */
  System();

  /**
   * @brief Ends one `System` and releases its tree, its merged catalog
   * and its limit table (FR-005).
   *
   * \pre none
   * \post none
   */
  ~System();

  /**
   * @brief The process-local root handle.
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] static auto local() -> System&;

  /**
   * @brief Registers a provider and merges its objects (FR-009).
   *
   * Registration after the system opened is a recoverable error. A
   * duplicate canonical path under one parent, a duplicate counter name
   * within one object, or a platform alias already resolving to another
   * object is a recoverable error naming the clash, and the whole
   * provider is refused with the tree left unchanged (FR-008, FR-002).
   * A provider unit token outside the closed mapping is a recoverable
   * error naming the unit (FR-017).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto registerProvider(std::unique_ptr<ProviderIface> provider)
      -> std::expected<void, Error>;

  /**
   * @brief Resolves a canonical structured path or a platform alias
   * to the same object (FR-002).
   *
   * A failure is a recoverable error whose diagnostics carry
   * near-miss names (FR-008).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto object(std::string_view path)
      -> std::expected<sg::counters::Object, Error>;

  /**
   * @brief One equality predicate of an `objects` selection (FR-003).
   */
  struct Filter
  {
    std::string_view key;
    std::string_view value;
  };

  /**
   * @brief Selects every object of one kind whose canonical path
   * satisfies all filters (FR-003, C-SYS-2); predicates combine with
   * AND. Every key is matched against the canonical-path components:
   * key `package`, value `1` matches component `package-1`. Defined
   * keys are the ancestor selectors `package` and `core`, which hold
   * for every kind, plus every key an object of the selected kind
   * spells as a component prefix, so a provider declares an attribute
   * key in the paths it seeds. A declared key whose value matches
   * nothing selects nothing. An unknown kind or an unknown filter key
   * is a recoverable error (FR-003, FR-008); results print canonical
   * spelling only (FR-002).
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto objects(std::string_view kind,
                             std::initializer_list<Filter> filters = {})
      -> std::expected<std::vector<const sg::counters::Object*>, Error>;

  /**
   * @brief Resolves the time-stamp counter as a shorter spelling of
   * resolving the `tsc` entry under the `machine` object
   * (specs/008-timestamp-counter FR-004).
   *
   * The returned counter is the same type the uniform lookup returns and
   * names the same canonical entry, so the two are interchangeable at the
   * call site and the result composes with every other counter. The call
   * resolves an entry and reads nothing: no hardware read, no plan, and no
   * recorder. It does not open the registration boundary, so a program
   * may call it and then register a provider.
   *
   * No clock provider registered is a recoverable error naming the absent
   * provider, and a build whose host does not execute the instruction is a
   * recoverable error naming the absent counter. Neither returns a counter.
   *
   * \pre none
   * \post none
   */
  [[nodiscard]] auto tsc() const -> std::expected<Counter<Dim<0, 1>>, Error>;

private:
  friend class Object;

  [[nodiscard]] auto handleFor(const std::string& canonical)
      -> sg::counters::Object&;

  friend auto detail::compileCore(const System& sys,
                                  const Target& tg,
                                  const std::vector<const detail::ExprCore*>&
                                      exprs) -> std::expected<Plan, Error>;
  friend auto detail::resolveLeafCore(const sg::counters::Object& obj,
                                      std::string_view name)
      -> std::expected<detail::LeafCore, Error>;

  struct Impl;  // the tree behind the handle
  std::unique_ptr<Impl> m_impl;
};

}  // namespace sg::counters

#endif  // SG_COUNTERS_SYSTEM_HPP
